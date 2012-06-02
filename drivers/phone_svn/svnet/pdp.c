/**
 * Samsung Virtual Network driver using OneDram device
 *
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

/* #define DEBUG */

#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/if.h>
#include <linux/if_arp.h>

#include "pdp.h"

static int vnet_open(struct net_device *ndev)
{
	netif_start_queue(ndev);
	return 0;
}

static int vnet_stop(struct net_device *ndev)
{
	netif_stop_queue(ndev);
	return 0;
}

static struct net_device_ops vnet_ops = {
	.ndo_open = vnet_open,
	.ndo_stop = vnet_stop,
/*	.ndo_tx_timeout = vnet_tx_timeout,
	.ndo_start_xmit = vnet_start_xmit,*/
};

static void vnet_setup(struct net_device *ndev)
{
	vnet_ops.ndo_start_xmit = vnet_start_xmit;

	ndev->netdev_ops = &vnet_ops;
	ndev->type = ARPHRD_PPP;
	ndev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
	ndev->hard_header_len = 0;
	ndev->addr_len = 0;
	ndev->tx_queue_len = 1000;
	ndev->mtu = ETH_DATA_LEN;
	ndev->watchdog_timeo = 5 * HZ;
}

struct net_device *create_pdp(int channel, struct net_device *parent)
{
	int r;
	struct pdp_priv *priv;
	struct net_device *ndev;
	char devname[IFNAMSIZ];

	if (!parent)
		return ERR_PTR(-EINVAL);

	sprintf(devname, "pdp%d", channel - 1);
	ndev = alloc_netdev(sizeof(struct pdp_priv), devname, vnet_setup);
	if (!ndev)
		return ERR_PTR(-ENOMEM);

	priv = netdev_priv(ndev);
	priv->channel = channel;
	priv->parent = parent;

	r = register_netdev(ndev);
	if (r) {
		free_netdev(ndev);
		return ERR_PTR(r);
	}

	return ndev;
}

void destroy_pdp(struct net_device **ndev)
{
	if (!ndev || !*ndev)
		return;

	unregister_netdev(*ndev);
	free_netdev(*ndev);
	*ndev = NULL;
}
