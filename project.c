#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

int init_module(void) {
    printk(KERN_INFO "Starting\n");
    return 0;
}

void cleanup_module(void) {
   printk(KERN_INFO "Cleanup\n");
}

MODULE_LICENSE("GPL");