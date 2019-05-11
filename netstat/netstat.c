#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/sched.h>
#include <linux/kthread.h>  
#include <linux/signal.h>
#include <linux/sched/signal.h>

#include "../fops.h"

#define FILE_NAME "/dev/my_device0"
#define BUFF_SIZE 11

struct timer_list mytimer;
struct task_struct *thread;

int calc(void *data)
{
	allow_signal(SIGKILL);
    	while (!kthread_should_stop()) {
		char *info;
		char **packets;
		char *buffer;
		struct file *file;
		int i, pind = 0, j = 0;
		int tcp = 0, udp = 0, dns = 0;
		buffer = (char*) kmalloc(PAGE_SIZE, GFP_KERNEL);
		packets = (char**) kmalloc(PAGE_SIZE, GFP_KERNEL);

		if (!buffer || !packets) {
			printk(KERN_ERR "Can't allocate memory!\n");
			return -ENOMEM;
	        }
		file = file_open(FILE_NAME, 0);
		if (!file) 
	                return -EIO;
	        
		file_read(file, buffer, BUFF_SIZE);
		file_close(file);

		// printk(KERN_DEBUG "Value: %s\n", buffer);
		
		while ((*(packets+pind) = strsep(&buffer,";")) != NULL)
			pind++;

		for (i = pind-1; i >= 0; i--) {
			j = 0;
			// printk(KERN_INFO "Packet info: %s\n", *(packets+i));
			if (strlen(*(packets+i)) == 0)
				continue;
			while ((info = strsep(packets+i, ",")) != NULL) {
				j++;
				if (j == 1) {
					printk(KERN_INFO "Source IP address: %s\n", info);
				} else if (j == 2) {
					// printk(KERN_INFO "Port: %s\n", info);
					if (strcmp(info, "53") == 0) 
						dns++;
				} else if (j == 3) {
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

		if (signal_pending(thread))
            		break;
	}
    	do_exit(0);
	return 0;
}

int init_module(void)
{
	printk(KERN_WARNING "Hello!\n");
	thread = kthread_run(&calc, NULL, "my_thread");
	if (thread) 
		printk(KERN_INFO "Thread ARE created!\n");
	else
		printk(KERN_INFO "Thread are Not created!\n");
	return 0;
}

void cleanup_module(void)
{
	if (thread) {
		kthread_stop(thread);
		printk(KERN_INFO "Thread stopped\n");
	}
	printk(KERN_WARNING "Bye!\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aiganym Zhandaulet");
MODULE_DESCRIPTION("Module for analyzing network packets.");