#include <linux/fs.h>

ssize_t retval;
unsigned long long offset = 0;
int err;

extern struct file* file_open(char *path, int flags)
{
	struct file* filp = NULL;
	filp = filp_open(path, flags, 0);
	err = PTR_ERR_OR_ZERO(filp);

	if (err) {
		printk(KERN_ERR "Can't open the device %d!", err);
		return NULL;
	}
	return filp;
}

extern int file_close(struct file *file)
{
	err = filp_close(file, NULL);
	if (err < 0) 
		printk(KERN_ERR "Can't close the device %d!", err);
	return err;
}

extern ssize_t file_read(struct file *file, char *data, size_t size)
{
	retval = kernel_read(file, data, size, &offset);
	if (retval < 0) 
		printk(KERN_ERR "Couldn't read the device %ld!", retval);
	return retval;
}

extern ssize_t file_write(struct file *file, char *data, size_t size)
{
	retval = kernel_write(file, data, size, &offset);
	if (retval < 0) 
		printk(KERN_ERR "Couldn't write to the device %ld!", retval);
	return retval;
}