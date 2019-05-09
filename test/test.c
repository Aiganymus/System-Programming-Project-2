#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>

struct file *filehandle1;
char buff[10];

struct file* file_open(const char* path, int flags, int rights)
{
	struct file* filp = NULL;
	mm_segment_t oldfs;
	int err = 0;
	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, rights);
	set_fs(oldfs);
	if (IS_ERR(filp))
	{
		err = PTR_ERR(filp);
		return NULL;
	}
	return filp;
}

void file_close(struct file* file)
{
	filp_close(file, NULL);
}

int file_write(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;
	oldfs = get_fs();
	set_fs(get_ds());
	ret = vfs_write(file, data, size, &offset);
	set_fs(oldfs);
	return ret;
}

int file_read(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;
	oldfs = get_fs();
	set_fs(get_ds());
	ret = vfs_read(file, data, size, &offset);
	set_fs(oldfs);
	return ret;
}

int init_module(void)
{
	printk(KERN_ALERT "Hello!\n");

	filehandle1 = file_open("/dev/my_device0", 1, 0);
	file_write((struct file*)filehandle1, 0, "hello", 5);
	file_close((struct file*)filehandle1);


	filehandle1 = file_open("/dev/my_device0", 0, 0);
	file_read((struct file*)filehandle1, 0, buff, 5);
	file_close((struct file*)filehandle1);

	printk(KERN_INFO "Value: %s\n", buff);

	return 0;
}

void cleanup_module(void)
{
	printk(KERN_ALERT "Bye!\n");
}

MODULE_LICENSE("GPL"); 