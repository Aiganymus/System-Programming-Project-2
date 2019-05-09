#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/semaphore.h>
#include <linux/wait.h>

#define DEVICE_NAME "my_device"
#define BUFF_SIZE PAGE_SIZE

struct virtual_device {
    wait_queue_head_t inq, outq; /* read and write queues */
    int nreaders, nwriters; /* number of openings for r/w */
    int buffersize; /* used in pointer arithmetic */
    struct semaphore sem;
    char *buff;
    struct cdev cdev; // character device structure
} my_dev;

dev_t dev; // holds device numbers: major and minor
int major_num, minor_num = 0;
int count = 1; // total number of contiguous device numbers you are requesting
int ret;

int dev_open(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "OPENING CHAR DEVICE!\n");
    struct virtual_device *char_dev; // device information
    char_dev = container_of(inode->i_cdev, struct virtual_device, cdev);
    filp->private_data = char_dev;

    if (filp->f_mode & FMODE_READ)
        char_dev->nreaders++;
    else if (filp->f_mode & FMODE_WRITE) {
        char_dev->nwriters++;
        if (down_interruptible(&char_dev->sem))
            return -ERESTARTSYS;
        kfree(char_dev->buff);
        char_dev->buff = (char*) kmalloc(BUFF_SIZE, GFP_KERNEL);
        if (!char_dev->buff) {
            up(&char_dev->sem);
            return -ENOMEM;
        }
        sprintf(char_dev->buff, "%s", "");
        char_dev->buffersize = BUFF_SIZE;
        up(&char_dev->sem);
    }
    return 0;
}

ssize_t dev_write(struct file *filp, const char *buffer, size_t len, loff_t *offset){
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    sprintf(message, "%s", buffer, len);   // appending received string with its length
    // size_of_message = strlen(message);                 // store the length of the stored message
    printk(KERN_INFO "EBBChar: Received %zu characters from\n", len);
    printk(KERN_INFO "Message: \'%s\'", message);
    up(&dev->sem);
    return len;
}

static ssize_t dev_read(struct file *filp, char *buffer, size_t len, loff_t *offset) {
    printk(KERN_INFO "READING FROM CHAR DEVICE!\n");

    struct virtual_device *char_dev; 
    int error_count = 0;

    dev = container_of(inode->i_cdev, struct virtual_device, cdev);
    filp->private_data = char_dev;

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

    struct virtual_device *char_dev; // device information
    dev = container_of(inode->i_cdev, struct virtual_device, cdev);
    filp->private_data = dev;
    
    down(&dev->sem);
    if (filp->f_mode & FMODE_READ)
        dev->nreaders--;
    if (filp->f_mode & FMODE_WRITE)
        dev->nwriters--;
    if (dev->nreaders + dev->nwriters == 0) {
        kfree(dev->buff);
        dev->buff = NULL; /* the other fields are not checked on open */
    }
    up(&dev->sem);
    return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = dev_read,
	.write = dev_write,
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

    
    init_waitqueue_head(&(my_dev.inq));
    init_waitqueue_head(&(my_dev.outq));
    init_MUTEX(&my_dev.sem);
    my_dev.buff = (char*) kmalloc(BUFF_SIZE, GFP_KERNEL);
    if (!my_dev->buff) {
        return -ENOMEM;
    }
    sprintf(my_dev->buff, "%s", "");
    my_dev->buffersize = BUFF_SIZE;

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