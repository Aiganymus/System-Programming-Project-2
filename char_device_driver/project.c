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
        wait_queue_head_t inq; /* read and write queues */
        int nreaders, nwriters; /* number of openings for r/w */
        int buffersize; 
        struct semaphore sem;
        char *buffer;
        struct cdev cdev; /* character device structure */
} my_dev;

dev_t dev; /* holds device numbers: major and minor */
int major_num, minor_num = 0;
int count = 1; /* total number of contiguous device numbers to request */
int ret; /* holds return value of functions */

int dev_open(struct inode *inode, struct file *filp) 
{
        printk(KERN_DEBUG "OPENING: CHAR DEVICE size %zu!\n", strlen(my_dev.buffer));

        if (down_interruptible(&my_dev.sem))
                return -ERESTARTSYS;
        if (filp->f_mode & FMODE_READ)
                my_dev.nreaders++;
        else if (filp->f_mode & FMODE_WRITE) 
                my_dev.nwriters++;
        up(&my_dev.sem);
        return 0;
}

ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *offset)
{
        printk(KERN_DEBUG "WRITING: TO CHAR DEVICE!\n");

        if (down_interruptible(&my_dev.sem))
                return -ERESTARTSYS;

        kfree(my_dev.buffer);
        my_dev.buffer = (char*) kmalloc(my_dev.buffersize, GFP_KERNEL);
        if (!my_dev.buffer) {
                printk(KERN_ERR "Can't allocate memory to buffer!\n");
                return -ENOMEM;
        }
        my_dev.buffer[0] = '\0';

        /* copy from buffer to device */
        memcpy(my_dev.buffer, buff, len);
        my_dev.buffer[len]='\0';

        if (strlen(my_dev.buffer)) { 
                printk(KERN_DEBUG "WRITING: Received %zu characters.\n", len);
        } else {
                printk(KERN_ERR "Failed to write %zu characters!\n", len);
                up (&my_dev.sem);
                return -EFAULT;
        }

        up(&my_dev.sem);

        /* awake any readers */
        wake_up_interruptible(&my_dev.inq);
        return len;
}

ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *offset) 
{
        size_t size;
        printk(KERN_DEBUG "READING: FROM CHAR DEVICE!\n");

        if (down_interruptible(&my_dev.sem))
                return -ERESTARTSYS;

        while (strlen(my_dev.buffer) == 0) { /* nothing to read */
                up(&my_dev.sem); /* release the lock */
                if (filp->f_flags & O_NONBLOCK)
                        return -EAGAIN;
                if (wait_event_interruptible(my_dev.inq, (strlen(my_dev.buffer) != 0)))
                        return -ERESTARTSYS; 
                if (down_interruptible(&my_dev.sem))
                        return -ERESTARTSYS;
        }

        /* copy from device to buffer */
        size = strlen(my_dev.buffer);
        memcpy(buff, my_dev.buffer, size);
        buff[size]='\0';

        if (strlen(buff)) { 
                printk(KERN_DEBUG "READING: Sent %zu characters.\n", size);
        } else {
                printk(KERN_ERR "Failed to read %zu characters!\n", size);
                up (&my_dev.sem);
                return -EFAULT;
        }

        /* clear the buffer */
        kfree(my_dev.buffer);
        my_dev.buffer = (char*) kmalloc(my_dev.buffersize, GFP_KERNEL);
        if (!my_dev.buffer) {
                printk(KERN_ERR "Can't allocate memory to buffer!\n");
                return -ENOMEM;
        }
        my_dev.buffer[0] = '\0';
        up (&my_dev.sem);

        return len;
}

int dev_release(struct inode *inode, struct file *filp) 
{
        printk(KERN_DEBUG "RELEASING: CHAR DEVICE!\n");

        down(&my_dev.sem);
        if (filp->f_mode & FMODE_READ)
                my_dev.nreaders--;
        else if (filp->f_mode & FMODE_WRITE)
                my_dev.nwriters--;
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

int setup_device(void) 
{
        init_waitqueue_head(&(my_dev.inq));
        sema_init(&my_dev.sem, 1);

        /* allocate memory to buffer */
        my_dev.buffersize = BUFF_SIZE;
        my_dev.buffer = (char*) kmalloc(my_dev.buffersize, GFP_KERNEL);
        if (!my_dev.buffer) {
                printk(KERN_ERR "Can't allocate memory to buffer!\n");
                return -ENOMEM;
        }
        my_dev.buffer[0] = '\0';

        cdev_init(&my_dev.cdev, &fops);
        my_dev.cdev.owner = THIS_MODULE;
        ret = cdev_add(&my_dev.cdev, dev, 1);
        if (ret < 0) {
                printk(KERN_ERR "Can't create char device!\n");
                return ret;
        }
        return 0;
}

int init_module(void) 
{
        printk(KERN_WARNING "Starting\n");
        ret = alloc_chrdev_region(&dev, minor_num, count, DEVICE_NAME);
        if (ret < 0) {
                printk(KERN_ERR  "Can't get major number!\n");
                return ret;
        }
        major_num = MAJOR(dev);
        printk(KERN_DEBUG "Major number: %d\n", major_num);
        ret = setup_device();
        if (ret != 0) 
                return ret;
        return 0;
}

void cleanup_module(void) 
{
        printk(KERN_WARNING "Cleanup\n");
        cdev_del(&my_dev.cdev);
        kfree(my_dev.buffer);
        unregister_chrdev_region(dev, count);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aiganym Zhandaulet");
MODULE_DESCRIPTION("Character device driver.");