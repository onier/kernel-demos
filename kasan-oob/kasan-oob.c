#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

static int type = 1;
//module_param(type, int, S_IRUGO);

static struct proc_dir_entry *entry;

static u8 * test_kasan_buffer;

static ssize_t test_kasan_write(struct file * filep,const char __user *buf,size_t length,loff_t *off){
	unsigned long n;
	n = copy_from_user(test_kasan_buffer,buf,length);
	return length;
}

static struct proc_ops test_kasan_fops={
	.proc_write = test_kasan_write
};

static int test_kasan_local_init(void){
    switch(type){
        case 1:
            printk("we are test kasan out of bound mode");
            break;
        case 2:
            printk("we are test kasan use after free mode");
            break;
        case 3:
            printk("we are test kasan global of bounds mode");
            break;
    }
    test_kasan_buffer = kmalloc(14,GFP_KERNEL);
    if(!test_kasan_buffer){
        return -ENOMEM;
    }
    entry = proc_create("test_kasan",S_IWUSR,NULL,&test_kasan_fops);
    if(!entry){
        return -ENOMEM;
    }
    if(type==2){
        kfree(test_kasan_buffer);
    }
    printk("echo \"Hello, KASAN error\" > /proc/test_kasan ");
    return 0;	
}

static int __init test_kasan_init(void){
	return test_kasan_local_init();
}

static void test_kasan_exit(void){
	kfree(test_kasan_buffer);
	remove_proc_entry("test_kasan",NULL);
}

static int kasan_setup(const char * val){
	printk("module parameter %s" ,val);
	return 0;
}

static int kasan_oob_param_call(const char *val,const struct kernel_param *kp){
	return 	kasan_setup(val);
}

MODULE_LICENSE("GPL");
module_param_call(kasan_oob,kasan_oob_param_call,NULL,NULL,0200);
module_init(test_kasan_init);
module_exit(test_kasan_exit);
