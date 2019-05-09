#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#define DEVICE_NAME "my_device"

struct virtual_device {
    struct scull_qset *data;
    struct semaphore sem;
    char data[100];
    struct cdev cdev; // character device structure
} my_dev;

// struct scull_dev {
//     struct scull_qset *data; /* Pointer to first quantum set */
//     int quantum; /* the current quantum size */
//     int qset; /* the current array size */
//     unsigned long size;  amount of data stored here 
//     unsigned int access_key; /* used by sculluid and scullpriv */
//     struct semaphore sem; /* mutual exclusion semaphore */
//     struct cdev cdev; /* Char device structure */
// };

struct scull_qset {
 void **data;
 struct scull_qset *next;
};

dev_t dev; // holds device numbers: major and minor
int major_num, minor_num = 0;
int count = 1; // total number of contiguous device numbers you are requesting
int ret;
static char message[256] = {0};    

int dev_open(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "OPENING CHAR DEVICE!\n");
    struct virtual_device *char_dev; // device information
    char_dev = container_of(inode->i_cdev, struct virtual_device, cdev);
    filp->private_data = char_dev;
    return 0;
}

ssize_t dev_write(struct file *filp, const char *buffer, size_t len, loff_t *offset){
    sprintf(message, "%s", buffer, len);   // appending received string with its length
    // size_of_message = strlen(message);                 // store the length of the stored message
    printk(KERN_INFO "EBBChar: Received %zu characters from\n", len);
    printk(KERN_INFO "Message: \'%s\'", message);
    return len;
}

static ssize_t dev_read(struct file *filp, char *buffer, size_t len, loff_t *offset){
    int error_count = 0;
    // copy_to_user has the format ( * to, *from, size) and returns 0 on success
    error_count = sprintf(buffer, "%s", message, strlen(message));

    if (error_count != 0){            // if true then have success
        printk(KERN_INFO "EBBChar: Sent %ld characters to the user\n", len);
        return -1;  // clear the position to the start and return 0
    }
    else {
        printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
        return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
    }
}

int dev_release(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "RELEASING CHAR DEVICE!\n");
    return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	// .llseek = scull_llseek,
	.read = dev_read,
	.write = dev_write,
	// .ioctl = scull_ioctl,
	.open = dev_open,
	.release = dev_release
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