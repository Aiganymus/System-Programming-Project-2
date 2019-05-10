#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */

#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/workqueue.h>

#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/kfifo.h>
#include "../fops.h"

#define BUFF_SIZE PAGE_SIZE
#define FIFO_SIZE PAGE_SIZE

struct packet_info {    
        struct work_struct work;
        int protocol;
        u32 *source_address;
        uint16_t source_port;
};

static struct nf_hook_ops hook_ops;
static struct workqueue_struct *wq;
static char *buff;
static struct kfifo fifo;
static bool done = false;
struct file *file;

void write_data(void) 
{
        size_t offset;
        struct packet_info *packet_info;
        printk(KERN_INFO "ENTER!\n");
        while(!kfifo_is_empty(&fifo)) {
                packet_info = kmalloc(sizeof(struct packet_info), GFP_KERNEL);
                kfifo_out(&fifo, packet_info, sizeof(struct packet_info));
                offset = strlen(buff);
                sprintf(&buff[offset], "%pI4,%hu,%d;", 
                                packet_info->source_address, packet_info->source_port, packet_info->protocol);                
                kfree(packet_info);
        }
        kfifo_reset(&fifo);
        // if (printk_ratelimit()) 
        //         printk(KERN_INFO "buffer: %s\n", buff);

        file = file_open("/dev/my_device0", 1);
        file_write(file, buff, strlen(buff));
        file_close(file);

        buff = (char*) kmalloc(BUFF_SIZE, GFP_KERNEL);
        buff[0] = '\0';
        // done = true;
}

static void wq_func(struct work_struct *work) 
{
        struct packet_info *packet_info;
        packet_info = (struct packet_info*) work;

        if (!done) {
                if (kfifo_size(&fifo)-kfifo_len(&fifo) < sizeof(struct packet_info))
                        write_data();
                kfifo_in(&fifo, packet_info, sizeof(struct packet_info));
        }
}

static unsigned int hook_func(void* priv, struct sk_buff *skb, const struct nf_hook_state* state)
{
        struct iphdr *iph;
        struct tcphdr *tcph;
        struct udphdr *udph;
        struct packet_info *packet_info;

        if(!skb)
                return NF_ACCEPT;

        packet_info = kmalloc(sizeof(struct packet_info), GFP_KERNEL);
        iph = ip_hdr(skb);

        if(iph->protocol == IPPROTO_TCP) {
                tcph = tcp_hdr(skb);
                INIT_WORK((struct work_struct*) packet_info, wq_func);       
                packet_info->source_port = ntohs(tcph->source);
                packet_info->source_address = &iph->saddr;
                packet_info->protocol = IPPROTO_TCP;
                queue_work(wq, (struct work_struct*) packet_info);       
        }
        else if(iph->protocol == IPPROTO_UDP) {
                udph = udp_hdr(skb);
                INIT_WORK((struct work_struct*) packet_info, wq_func);       
                packet_info->source_port = ntohs(udph->source);
                packet_info->source_address = &iph->saddr;
                packet_info->protocol = IPPROTO_UDP;
                queue_work(wq, (struct work_struct*) packet_info); 
        }
        return NF_ACCEPT; /* accept the packet */
}

int init_module(void) 
{
        int retval;
        buff = (char*) kmalloc(BUFF_SIZE, GFP_KERNEL);
        buff[0] = '\0';
        wq = create_workqueue("my work queue");

        retval = kfifo_alloc(&fifo, FIFO_SIZE, GFP_KERNEL);
        if (retval)
                return retval;
        hook_ops.hook = hook_func;
        hook_ops.hooknum = NF_INET_LOCAL_IN; /* the packets destined for this machine */
        hook_ops.pf = PF_INET;
        hook_ops.priority = NF_IP_PRI_FIRST;
        retval = nf_register_net_hook(&init_net, &hook_ops);
        printk(KERN_WARNING "nf_register_net_hook returned %d\n", retval);

        return retval;
}

void cleanup_module(void)
{
        flush_workqueue(wq);
        destroy_workqueue(wq);
        nf_unregister_net_hook(&init_net, &hook_ops);
        kfifo_free(&fifo);
        printk(KERN_WARNING "Unregistered the net hook.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aiganym Zhandaulet");
MODULE_DESCRIPTION("Module for capturing network packets and writing them to char device.");