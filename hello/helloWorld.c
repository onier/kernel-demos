#include <linux/types.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/moduleparam.h>



MODULE_LICENSE("GPL");
static char * type = "";
module_param(type, charp, S_IRUGO);

static int __attribute__((optimize("O0"))) hello_init(void){
	printk("Hello Module init ........... %s",type);
	return 0;
}

static void __attribute__((optimize("O0"))) hello_exit(void){
	printk("Hello Module exit .........");
}

module_init(hello_init);
module_exit(hello_exit);
