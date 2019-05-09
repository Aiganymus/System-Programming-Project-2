#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/slab.h>
#include <linux/gfp.h>

char *buffer;

struct packet_info {
    struct work_struct work;
    int protocol;
    u32 *source_address;
    uint16_t source_port;
};

int init_module(void)
{
	printk(KERN_ALERT "Hello!\n");
	// char user[3] = "xyz";
	buffer = (char*) kmalloc(PAGE_SIZE, GFP_KERNEL);
	sprintf(buffer, "%s", "");

	size_t len = strlen(buffer);
	printk(KERN_INFO "size: %zu", len);

	char *user = (char*) kmalloc(PAGE_SIZE, GFP_KERNEL);
	sprintf(user, "%s", "Addr:35.224.99.156,Port:80,Type:6");
	sprintf(buffer, "%s", user);
	printk(KERN_INFO "%s", buffer);
	len = strlen(buffer);
	printk(KERN_INFO "size: %zu", len);

	// printk(KERN_INFO "size: %zd", sizeof(struct packet_info));
	// printk(KERN_INFO "size: %zd", PAGE_SIZE);
	return 0;
}

void cleanup_module(void)
{
	kfree(buffer);
	printk(KERN_ALERT "Bye!\n");
}

MODULE_LICENSE("GPL"); 