#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/gfp.h>

#include "../fops.h"

struct file *filehandle1;

// struct file* file_open(const char* path, int flags, int rights)
// {
// 	struct file* filp = NULL;
// 	int err = 0;
// 	filp = filp_open(path, flags, rights);
// 	if (IS_ERR(filp)) {
// 		err = PTR_ERR(filp);
// 		return NULL;
// 	}
// 	return filp;
// }

// void file_close(struct file* file)
// {
// 	filp_close(file, NULL);
// }

// ssize_t file_read(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
// {
// 	ssize_t ret;
// 	ret = kernel_read(file, data, size, &offset);
// 	return ret;
// }

// ssize_t file_write(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
// {
// 	ssize_t ret;
// 	ret = kernel_write(file, data, size, &offset);
// 	return ret;
// }

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