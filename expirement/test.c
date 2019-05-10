#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/gfp.h>

#include "../fops.h"

struct file *filehandle1;

int init_module(void)
{
	printk(KERN_ALERT "Hello!\n");
	filehandle1 = file_open("/dev/my_device0", 1);
	char *data = "hello world";
	file_write((struct file*)filehandle1, data, strlen(data));
	file_close((struct file*)filehandle1);

	char *buff = (char*) kmalloc(PAGE_SIZE, GFP_KERNEL);
	filehandle1 = file_open("/dev/my_device0", 0);
	file_read((struct file*)filehandle1, buff, strlen(data));
	file_close((struct file*)filehandle1);

	printk(KERN_INFO "Value: %s\n", buff);

	return 0;
}

void cleanup_module(void)
{
	printk(KERN_ALERT "Bye!\n");
}

MODULE_LICENSE("GPL"); 