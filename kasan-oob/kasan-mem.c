#include <linux/fs.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mm.h>

static char global_test_char[10];

static void __attribute__((optimize("O0"))) test_oob(void){
  	printk("for memory out of bound");
	u8 * mem_buf = kmalloc(14,GFP_KERNEL);
	if(!mem_buf){
		printk(KERN_ERR "create kmalloc fail ");
		return;
	}
	mem_buf[15]='1';
	kfree(mem_buf);
}

static void __attribute__((optimize("O0"))) test_u_a_f(void){
	printk("for memory use after free");
	u8 * mem_buf = kmalloc(4,GFP_KERNEL);
	if(!mem_buf){
		printk(KERN_ERR "create kmalloc fail ");
		return;
	}
	kfree(mem_buf);
	mem_buf[0]='1';
}

static void __attribute__((optimize("O0"))) test_global_mem(void){
   printk("for memory global out of bound");
   global_test_char[10]='x';
}

static void test_invalid_read(void){
	char char1 = global_test_char[12];
	printk("read value %s",char1);
}

static void test_alloc_page(void){
    struct  page * page = alloc_page(GFP_KERNEL);
	u8 * buf =(u8*)page_address(page);
	buf[0]='1';
	int index = PAGE_SIZE+1;
	buf[index]='2';
	printk("alloc_page %s",buf);
	free_page(buf);
}

static void test_alloc_page_bulk_list(void){
	printk("test_alloc_page_bulk_list \n");
	struct list_head list;
	INIT_LIST_HEAD(&list);
	int bulk = 4;
	unsigned long n = alloc_pages_bulk_list(GFP_KERNEL,bulk,&list);
	if(unlikely(n<bulk)){
         printk(KERN_ERR "alloc page bulk list fail");	
		 return ;
	}

	struct page * page,*next;
	list_for_each_entry_safe(page,next,&list,lru){
		list_del(&page->lru);
		__free_pages(page,0);
	}
}

static void test_alloc_page_bulk_array(void){
	unsigned long pages = 3;
    struct page * page[128];
    unsigned long nr_pages= alloc_pages_bulk_array(GFP_KERNEL,pages,page);
    if(unlikely(!nr_pages)){
        return ;
    }
    int i =0;
    for (i =0;i<pages;i++){
        u8 * buf = (u8*)page_address(page[i]);
        buf[0]='A'+i;
    }
	for(i =0;i<pages;i++){
		__free_pages(page[i],0);
	}
}

static struct proc_dir_entry *entry;
static ssize_t  start_kasan_test(struct file * filep ,const char __user *input,size_t length,loff_t * off){
	u8* buf = kmalloc(length,GFP_KERNEL);
	copy_from_user(buf,input,length);
	if(buf[0]=='1'){
	 	test_oob();
	} else if(buf[0]=='2'){
		test_u_a_f();
	} else if(buf[0]=='3'){
		test_global_mem();			
	} else if(buf[0]=='4'){
		test_invalid_read();
	} else if(buf[0]=='5'){
		test_alloc_page();
	} else if(buf[0]=='6'){
		test_alloc_page_bulk_list();
	} else if(buf[0]=='7'){
		test_alloc_page_bulk_array();
	}
	return length;
}

static struct proc_ops kasan_test_fops={
	.proc_write = start_kasan_test
};

static int __init start_kasan_test_module_init(void){
	entry = proc_create("start_kasan_test",S_IWUSR,NULL,&kasan_test_fops);
	if(!entry){
		printk(KERN_ERR "create proce kasan test entry fail");
		return -ENOMEM;
	}else{
		printk("echo 1 > /proc/start_kasan_test | for memory out of bound");
		printk("echo 2 > /proc/start_kasan_test | for memory use after free");
		printk("echo 3 > /proc/start_kasan_test | for memory globa out of bound");
		printk("echo 4 > /proc/start_kasan_test | for invalid memory read ");

		printk("echo 5 > /proc/start_kasan_test | for alloc_page error ");
		printk("echo 6 > /proc/start_kasan_test | for test_alloc_page_bulk_list ");
		printk("echo 7 > /proc/start_kasan_test | for test_alloc_page_bulk_array ");
//		printk("echo 1 > /prco/start_kasan_test | for memory out of bound");
	}
	return 0;
}

static void start_kasan_test_module_exit(void){
	remove_proc_entry("start_kasan_test",NULL);
}

module_init(start_kasan_test_module_init);
module_exit(start_kasan_test_module_exit);
MODULE_LICENSE("GPL");
