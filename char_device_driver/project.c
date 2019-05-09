#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#define DEVICE_NAME "my_device"

struct virtual_device {
    char data[100];
    struct cdev cdev; // character device structure
} my_dev;

dev_t dev; // holds device numbers: major and minor
int major_num, minor_num = 0;
int count = 1; // total number of contiguous device numbers you are requesting
int ret;

int dev_open(struct inode *inode, struct file *filp) {
    struct virtual_device *dev; // device information
    dev = container_of(inode->i_cdev, struct virtual_device, cdev);
    filp->private_data = dev;
    return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	// .llseek = scull_llseek,
	// .read = scull_read,
	// .write = scull_write,
	// .ioctl = scull_ioctl,
	.open = dev_open,
	// .release = scull_release,
};

int init_module(void) {
    printk(KERN_INFO "Starting\n");
    ret = alloc_chrdev_region(&dev, minor_num, count, DEVICE_NAME);
    if (ret < 0) {
    	printk(KERN_WARNING "Can't get major number!\n");
    	return ret;
    }
    major_num = MAJOR(dev);
    printk(KERN_INFO "The major number is %d\n", major_num);

    cdev_init(&my_dev.cdev, &fops);
    my_dev.cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_dev.cdev, dev, 1);
    if (ret < 0) {
    	printk(KERN_WARNING "Can't create char device!\n");
    	return ret;
    }
    return 0;
}

void cleanup_module(void) {
   printk(KERN_INFO "Cleanup\n");
   cdev_del(&my_dev.cdev);
   unregister_chrdev_region(dev, count);
}

MODULE_LICENSE("GPL");