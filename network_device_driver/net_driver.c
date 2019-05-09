#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/netdevice.h>
#include <linux/if.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/string.h>

#define DEVICE_NAME "net_device"
#define IF_NUM 2

struct net_device *net_devs[IF_NUM]; // interfaces
static int res, i = 0;

struct nd_priv {
	 struct net_device_stats stats; // interface statistics
	 int status;
	 struct nd_packet *ppool;
	 struct nd_packet *rx_queue; /* List of incoming packets */
	 int rx_int_enabled;
	 int tx_packetlen;
	 u8 *tx_packetdata;
	 struct sk_buff *skb;
	 spinlock_t lock;
};


// initializaion function to setup the net device
void nd_init(struct net_device *dev) {
	ether_setup(dev); /* assign some of the fields */
	dev->open = nd_open;
	dev->stop = nd_release;
	dev->set_config = nd_config;
	dev->hard_start_xmit = nd_tx;
	dev->do_ioctl = nd_ioctl;
	dev->get_stats = nd_stats;
	dev->rebuild_header = nd_rebuild_header;
	dev->hard_header = nd_header;
	dev->tx_timeout = nd_tx_timeout;
	dev->watchdog_timeo = timeout;

	/* keep the default flags, just add NOARP */
	dev->flags |= IFF_NOARP;
	dev->features |= NETIF_F_NO_CSUM;
	dev->hard_header_cache = NULL; /* Disable caching */

	struct nd_priv *priv;
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct nd_priv));
	spin_lock_init(&priv->lock);
	nd_rx_ints(dev, 1); /* enable receive interrupts */
}

int init_module(void) {
    printk(KERN_INFO "Starting\n");

    for (i = 0; i < IF_NUM; ++i) {
    	// allocate net device
    	net_devs[i] = alloc_netdev(sizeof(struct nd_priv), "nd%d", nd_init);
    	if (net_devs[i] == NULL) {
    		printk(KERN_ERR "Couldn't allocate net device!\n");
    		return -1;
    	}
    	// register net device
    	if (res = register_netdev(net_devs[i])) {
			printk(KERN_ERR "Couldn't register %i net device \"%s\"\n", res, net_devs[i]->name);
			return res;
    	}
    }

    return 0;
}

void cleanup_module(void) {
   printk(KERN_INFO "Cleanup\n");
	for(i = 0; i < IF_NUM; i++) {
		if(net_devs[i]) {
			unregister_netdev(net_devs[i]);
			nd_teardown_pool(net_devs[i]);
			free_netdev(net_devs[i]);
		}
	}

}

MODULE_LICENSE("GPL");