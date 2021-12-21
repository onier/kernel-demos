#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mm.h>

#define MAX_SIZE (PAGE_SIZE * 2) 
static unsigned int major; /* major number for device */
static struct class *dummy_class;
static struct cdev dummy_cdev;


int dummy_open(struct inode * inode, struct file * filp)
{
    pr_info("Someone tried to open me\n");
    return 0;
}

int dummy_release(struct inode * inode, struct file * filp)
{
    pr_info("Someone closed me\n");
    return 0;
}

ssize_t dummy_read (struct file *filp, char __user * buf, size_t count,
                                loff_t * offset)
{
    pr_info("Nothing to read guy\n");
    return 0;
}


ssize_t dummy_write(struct file * filp, const char __user * buf, size_t count,
                                loff_t * offset)
{
    pr_info("Can't accept any data guy\n");
    return count;
}

static int __attribute((optimize("O0"))) mchar_mmap(struct file *f, struct vm_area_struct *vma)
{
	 unsigned long pages = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
  unsigned long addr;
  struct page *page;
  int i,rc=-ENODEV;
  // TODO need any semaphore for vma manipulation?
  printk(KERN_DEBUG "vma->vm_end %lu vm_start %lu len %lu pages %lu vm_pgoff %lu\n",
    vma->vm_end, vma->vm_start, vma->vm_end - vma->vm_start, pages, vma->vm_pgoff);
  /* allocate and insert pages to fill the vma. */
  for(i=0; i < pages; i++) {
    page = alloc_page(GFP_KERNEL); // TODO IO RESERVE?
    if (!page) {
      // TODO free previous pages
      printk(KERN_DEBUG "alloc_page failed\n");
      goto done;
    }
    addr = vma->vm_start+i*PAGE_SIZE;
    if (vm_insert_page(vma,addr,page) < 0) {
      // TODO free previous pages
      printk(KERN_DEBUG "vm_insert_page failed\n");
      goto done;
    }
    printk(KERN_DEBUG "inserted page %d at %p\n",i,(void*)addr);
    // TODO __free_page now, should be ok, since vm_insert_page incremented its
    // refcount. that way, upon munmap, refcount hits zer0, pages get freed
    __free_page(page);
  }
  printk(KERN_DEBUG "completed inserting %lu pages\n", pages);

  rc = 0;

 done:
  return rc;
}


struct file_operations dummy_fops = {
//    open:       dummy_open,
//    release:    dummy_release,
//    read:       dummy_read,
    .mmap = mchar_mmap
};

static int __init dummy_char_init_module(void)
{
    struct device *dummy_device;
    int error;
    dev_t devt = 0;

    /* Get a range of minor numbers (starting with 0) to work with */
    error = alloc_chrdev_region(&devt, 0, 1, "dummy_char");
    if (error < 0) {
        pr_err("Can't get major number\n");
        return error;
    }
    major = MAJOR(devt);
    pr_info("dummy_char major number = %d\n",major);

    /* Create device class, visible in /sys/class */
    dummy_class = class_create(THIS_MODULE, "dummy_char_class");
    if (IS_ERR(dummy_class)) {
        pr_err("Error creating dummy char class.\n");
        unregister_chrdev_region(MKDEV(major, 0), 1);
        return PTR_ERR(dummy_class);
    }

    /* Initialize the char device and tie a file_operations to it */
    cdev_init(&dummy_cdev, &dummy_fops);
    dummy_cdev.owner = THIS_MODULE;
    /* Now make the device live for the users to access */
    cdev_add(&dummy_cdev, devt, 1);

    dummy_device = device_create(dummy_class,
                                NULL,   /* no parent device */
                                devt,    /* associated dev_t */
                                NULL,   /* no additional data */
                                "dummy_char");  /* device name */

    if (IS_ERR(dummy_device)) {
        pr_err("Error creating dummy char device.\n");
        class_destroy(dummy_class);
        unregister_chrdev_region(devt, 1);
        return -1;
    }

    pr_info("dummy char module loaded\n");
    return 0;
}

static void __exit dummy_char_cleanup_module(void)
{
    unregister_chrdev_region(MKDEV(major, 0), 1);
    device_destroy(dummy_class, MKDEV(major, 0));
    cdev_del(&dummy_cdev);
    class_destroy(dummy_class);

    pr_info("dummy char module Unloaded\n");
}

module_init(dummy_char_init_module);
module_exit(dummy_char_cleanup_module);

MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_DESCRIPTION("Dummy character driver");
MODULE_LICENSE("GPL");
