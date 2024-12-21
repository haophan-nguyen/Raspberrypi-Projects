#include <linux/module.h>
#include <linux/init.h>

/* Meta info */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phan Hao");
MODULE_DESCRIPTION("A hello world");

/**
 * @brief This function is called when the module is loaded into the kernel
 */
static int __init ModuleInit(void){
	printk(KERN_INFO "Hello kernel\n");
	return 0;
}

/**
 * @brief This function is called when the module is removed from the kernel
 */
static void __exit ModuleExit(void){
	printk(KERN_INFO "Goodbye kernel\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);
