#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/sched.h>
#include <linux/kthread.h>  
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/time.h>

#include "../fops.h"

#define FILE_NAME "/dev/my_device0"
#define BUFF_SIZE 11

struct timer_list mytimer;

void calc(struct timer_list *t) 
{
	char *info;
	char **packets;
	char *buffer;
	struct file *file;
	int i, pind = 0, j = 0;
	int tcp = 0, udp = 0, dns = 0;
	int ret;
	buffer = (char*) kmalloc(PAGE_SIZE, GFP_KERNEL);
	packets = (char**) kmalloc(PAGE_SIZE, GFP_KERNEL);

	if (!buffer || !packets) {
		printk(KERN_ERR "Can't allocate memory!\n");
		return;
    	}
	file = file_open(FILE_NAME, 0);
	if (!file) 
                return;
        
	file_read(file, buffer, BUFF_SIZE);
	file_close(file);

	// printk(KERN_DEBUG "Value: %s\n", buffer);
	
	while ((*(packets+pind) = strsep(&buffer,";")) != NULL)
		pind++;

	for (i = pind-1; i >= 0; i--) {
		j = 0;
		// printk(KERN_INFO "Packet info: %s\n", *(packets+i));
		while ((info = strsep(packets+i, ",")) != NULL) {
			j++;
			if (j == 1)
				printk(KERN_INFO "Source IP address: %s\n", info);
			else if (j == 2) {
				// printk(KERN_INFO "Port: %s\n", info);
				if (strcmp(info, "53") == 0) 
					dns++;
			}
        		else if (j == 3) {
        			// printk(KERN_INFO "Protocol: %s\n", info);
        			if (strcmp(info, "6") == 0) 
        				tcp++;
        			else 
        				udp++;

        		} 

		}
	}
	printk(KERN_INFO "UDP packets: %d\n", udp);
	printk(KERN_INFO "TCP packets: %d\n", tcp);
	printk(KERN_INFO "DNS packets: %d\n", dns);
	kfree(packets);
	kfree(buffer);
	ret = mod_timer(&mytimer, jiffies + msecs_to_jiffies(1000));
	if (ret) {
		printk("Error in mod_timer\n");
	}
}

int init_module(void)
{
	printk(KERN_WARNING "Hello!\n");
	mytimer.expires = jiffies + msecs_to_jiffies(1000);
   	timer_setup(&mytimer, calc, 0);
	add_timer(&mytimer);
	return 0;
}

void cleanup_module(void)
{
	del_timer(&mytimer);
	printk(KERN_WARNING "Bye!\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aiganym Zhandaulet");
MODULE_DESCRIPTION("Module for analyzing network packets.");