#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/slab.h>
#include <linux/gfp.h>

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
	// buffer = (char*) kmalloc(PAGE_SIZE, GFP_KERNEL);
	// sprintf(buffer, "%s", "");
	// buffer[0] = '\0';

	// size_t len = strlen(buffer);
	// printk(KERN_INFO "size: %zu", len);

	// char *user = (char*) kmalloc(PAGE_SIZE, GFP_KERNEL);
	// buffer = "Addr:35.224.99.156,Port:80,Type:6;Addr:35.224.99.156,Port:80,Type:6";
	// memcpy(buffer, user, strlen(user));
	// buffer[strlen(user)]='\0';

	// sprintf(user, "%s", "Addr:35.224.99.156,Port:80,Type:6");
	// sprintf(buffer, "%s", user);

	// printk(KERN_INFO "%s", buffer);
	// len = strlen(buffer);
	// printk(KERN_INFO "size: %zu", len);

	// printk(KERN_INFO "size: %zd", sizeof(struct packet_info));
	// printk(KERN_INFO "size: %zd", PAGE_SIZE);

	char *buffer, *found;
	char **words;
	int a,windex;

	buffer = (char*) kmalloc(PAGE_SIZE, GFP_KERNEL);
	words = (char**) kmalloc(PAGE_SIZE, GFP_KERNEL);
	sprintf(buffer, "%s", "Addr:35.224.99.156,Port:80,Type:6;Addr:35.224.99.156,Port:80,Type:6");
	windex = 0;
	while ((*(words+windex) = strsep(&buffer,";")) != NULL)
		windex++;

	for(a=windex-1;a>=0;a--) {
		printk(KERN_INFO "info: %s", *(words+a));
		while((found = strsep(words+a, ",")) != NULL)
        	printk(KERN_INFO "word: %s\n", found);
	}
	kfree(words);
	kfree(buffer);
	return 0;
}

void cleanup_module(void)
{
	printk(KERN_ALERT "Bye!\n");
}

MODULE_LICENSE("GPL"); 