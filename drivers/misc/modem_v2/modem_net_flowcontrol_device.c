/* /linux/drivers/misc/modem_if/modem_net_flowcontrol_device.c
 *
 * Copyright (C) 2011 Google, Inc.
 * Copyright (C) 2011 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/module.h>
#include <linux/platform_data/modem_v2.h>

#include "modem_prj.h"


#define NET_FLOWCONTROL_DEV_NAME_LEN 8

static int modem_net_flowcontrol_device_open(
			struct inode *inode, struct file *filp)
{
	return 0;
}

static int modem_net_flowcontrol_device_release(
			struct inode *inode, struct file *filp)
{
	return 0;
}

static long modem_net_flowcontrol_device_ioctl(
			struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct net *this_net;
	struct net_device *ndev;
	char dev_name[NET_FLOWCONTROL_DEV_NAME_LEN];
	u8 chan;

	if (copy_from_user(&chan, (void __user *)arg, sizeof(char)))
		return -EFAULT;

	if (chan > 15)
		return -ENODEV;

	snprintf(dev_name, NET_FLOWCONTROL_DEV_NAME_LEN, "rmnet%d", (int)chan);
	this_net = get_net_ns_by_pid(current->pid);
	ndev = __dev_get_by_name(this_net, dev_name);
	if (ndev == NULL) {
		mif_err("device = %s not exist\n", dev_name);
		return -ENODEV;
	}

	switch (cmd) {
	case IOCTL_MODEM_NET_SUSPEND:
		netif_stop_queue(ndev);
		mif_info("NET SUSPEND(%s)\n", dev_name);
		break;
	case IOCTL_MODEM_NET_RESUME:
		netif_wake_queue(ndev);
		mif_info("NET RESUME(%s)\n", dev_name);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct file_operations modem_net_flowcontrol_device_fops = {
	.owner = THIS_MODULE,
	.open = modem_net_flowcontrol_device_open,
	.release = modem_net_flowcontrol_device_release,
	.unlocked_ioctl = modem_net_flowcontrol_device_ioctl,
};

static int __init modem_net_flowcontrol_device_init(void)
{
	int ret = 0;
	struct io_device *net_flowcontrol_dev;

	net_flowcontrol_dev = kzalloc(sizeof(struct io_device), GFP_KERNEL);
	if (!net_flowcontrol_dev) {
		mif_err("net_flowcontrol_dev io device memory alloc fail\n");
		return -ENOMEM;
	}

	net_flowcontrol_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	net_flowcontrol_dev->miscdev.name = "modem_br";
	net_flowcontrol_dev->miscdev.fops = &modem_net_flowcontrol_device_fops;

	ret = misc_register(&net_flowcontrol_dev->miscdev);
	if (ret) {
		mif_err("failed to register misc br device : %s\n",
			net_flowcontrol_dev->miscdev.name);
		kfree(net_flowcontrol_dev);
	}

	return ret;
}

module_init(modem_net_flowcontrol_device_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung Modem IF Net Flowcontrol Driver");
