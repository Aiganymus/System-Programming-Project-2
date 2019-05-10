#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/wait.h>

#include <linux/slab.h>
#include <linux/gfp.h>

#define DEVICE_NAME "my_device"
#define BUFF_SIZE PAGE_SIZE

struct virtual_device {
    wait_queue_head_t inq, outq; /* read and write queues */
    int nreaders, nwriters; /* number of openings for r/w */
    int buffersize; /* used in pointer arithmetic */
    struct semaphore sem;
    char *buffer;
    struct cdev cdev; // character device structure
} my_dev;

dev_t dev; // holds device numbers: major and minor
int major_num, minor_num = 0;
int count = 1; // total number of contiguous device numbers you are requesting
int ret;

int dev_open(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "OPENING CHAR DEVICE size %zu!\n", strlen(my_dev.buffer));
    // struct virtual_device *char_dev; // device information
    // char_dev = container_of(inode->i_cdev, struct virtual_device, cdev);
    // filp->private_data = char_dev;

    if (down_interruptible(&my_dev.sem))
        return -ERESTARTSYS;
    if (filp->f_mode & FMODE_READ)
        my_dev.nreaders++;
    else if (filp->f_mode & FMODE_WRITE) {
        my_dev.nwriters++;
        // if (down_interruptible(&char_dev->sem))
        //     return -ERESTARTSYS;
        // kfree(char_dev->buff);
        // char_dev->buff = (char*) kmalloc(BUFF_SIZE, GFP_KERNEL);
        // if (!char_dev->buff) {
        //     up(&char_dev->sem);
        //     return -ENOMEM;
        // }
        // buffer[0] = '\0';
        // char_dev->buffersize = BUFF_SIZE;
    }
    up(&my_dev.sem);
    return 0;
}

ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *offset){
    // struct virtual_device *dev; // device information
    // dev = filp->private_data;

    if (down_interruptible(&my_dev.sem))
        return -ERESTARTSYS;

    while (strlen(my_dev.buffer) != 0) { /* no space to write */
        up(&my_dev.sem); /* release the lock */
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

        if (wait_event_interruptible(my_dev.outq, (strlen(my_dev.buffer) == 0)))
            return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
        /* otherwise loop, but first reacquire the lock */
        if (down_interruptible(&my_dev.sem))
            return -ERESTARTSYS;
    }

    memcpy(my_dev.buffer, buff, len);
    my_dev.buffer[len]='\0';
    printk(KERN_INFO "Device: Received %zu characters from %s\n", len, my_dev.buffer);

    up(&my_dev.sem);

    /* finally, awake any reader */
    wake_up_interruptible(&my_dev.inq);  /* blocked in read() and select() */
    return len;
}

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *offset) {
    printk(KERN_INFO "READING FROM CHAR DEVICE!\n");
    // struct virtual_device *dev; // device information
    // dev = filp->private_data;

    if (down_interruptible(&my_dev.sem))
        return -ERESTARTSYS;

    while (strlen(my_dev.buffer) == 0) { /* nothing to read */
        up(&my_dev.sem); /* release the lock */
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

        if (wait_event_interruptible(my_dev.inq, (strlen(my_dev.buffer) != 0)))
            return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
        /* otherwise loop, but first reacquire the lock */
        if (down_interruptible(&my_dev.sem))
            return -ERESTARTSYS;
    }

    /* copy from device to buffer */
    memcpy(buff, my_dev.buffer, len);
    buff[len]='\0';

    if (strlen(buff)){            // if true then have success
        printk(KERN_INFO "Device: Sent %zu characters to the user %s\n", len, buff);
    }
    else {
        printk(KERN_INFO "Device: Failed to send %zu characters to the user\n", len);
        up (&my_dev.sem);
        return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
    }

    /* clear the buffer */
    kfree(my_dev.buffer);
    my_dev.buffer = (char*) kmalloc(BUFF_SIZE, GFP_KERNEL);
    if (!my_dev.buffer) {
        return -ENOMEM;
    }
    my_dev.buffer[0] = '\0';
    up (&my_dev.sem);

    /* finally, awake any writers and return */
    wake_up_interruptible(&my_dev.outq);
    return 0;
}

int dev_release(struct inode *inode, struct file *filp) {
    printk(KERN_INFO "RELEASING CHAR DEVICE!\n");
    // struct virtual_device *dev; // device information
    // dev = filp->private_data;
    
    down(&my_dev.sem);
    if (filp->f_mode & FMODE_READ)
        my_dev.nreaders--;
    else if (filp->f_mode & FMODE_WRITE)
        my_dev.nwriters--;
    // if (dev->nreaders + dev->nwriters == 0) {
    //     kfree(dev->buff);
    //     dev->buff = NULL; /* the other fields are not checked on open */
    // }
    up(&my_dev.sem);
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
    sema_init(&my_dev.sem, 1);
    // allocate memory to buffer
    my_dev.buffer = (char*) kmalloc(BUFF_SIZE, GFP_KERNEL);
    if (!my_dev.buffer) {
        return -ENOMEM;
    }
    my_dev.buffer[0] = '\0';
    my_dev.buffersize = BUFF_SIZE;

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
   kfree(my_dev.buffer);
   unregister_chrdev_region(dev, count);
}

MODULE_LICENSE("GPL");