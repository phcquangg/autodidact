#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>

static struct net_device *vnet_dev;

static netdev_tx_t vnet_xmit(struct sk_buff *skb, struct net_device *dev)
{
	pr_info("Intercepted packet! Size: %d bytes\n", skb->len);
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

static const struct net_device_ops vnet_netdev_ops = {
	.ndo_start_xmit = vnet_xmit,
};

static int __init net_init( void)
{
	vnet_dev = alloc_netdev( 0, "tun_custom%d", NET_NAME_UNKNOWN, ether_setup);
	if (!vnet_dev) return -ENOMEM;

	vnet_dev->netdev_ops = &vnet_netdev_ops;

	if (register_netdev(vnet_dev)) {
		free_netdev(vnet_dev);
		return -EIO;
	}

	return 0;
}

static void __exit net_exit( void)
{
	unregister_netdev(vnet_dev);
	free_netdev(vnet_dev);
}

module_init(net_init);
module_exit(net_exit);
MODULE_LICENSE("GPL");

