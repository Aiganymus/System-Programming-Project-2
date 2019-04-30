#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>

static struct file_operations fops = {
	.owner = THIS_MODULE,
	// .llseek = scull_llseek,
	// .read = scull_read,
	// .write = scull_write,
	// .ioctl = scull_ioctl,
	// .open = scull_open,
	// .release = scull_release,
};

static struct cdev my_cdev;

dev_t dev;
int major_num, minor_num = 0, count = 1;

int init_module(void) {
	int ret;
    printk(KERN_INFO "Starting\n");
    ret = alloc_chrdev_region(&dev, minor_num, count, "my device");
    if (ret < 0) {
    	printk(KERN_WARNING "Can't get major number!\n");
    	return ret;
    }
    major_num = MAJOR(dev);
    printk(KERN_INFO "The major number is %d\n", major_num);

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_cdev, &dev, count);
    if (ret < 0) {
    	printk(KERN_WARNING "Can't create char device!\n");
    	return ret;
    }
    return 0;
}

void cleanup_module(void) {
   printk(KERN_INFO "Cleanup\n");
   cdev_del(&my_cdev);
   unregister_chrdev_region(dev, count);
}

MODULE_LICENSE("GPL");