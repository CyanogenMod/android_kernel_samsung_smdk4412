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

/*#define DEBUG */

#if defined(DEBUG)
#define NOISY_DEBUG
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/jiffies.h>

#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/if.h>
#include <linux/if_arp.h>

#include <linux/if_phonet.h>
#include <linux/phonet.h>
#include <net/phonet/phonet.h>

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>

#define DEFAULT_RAW_WAKE_TIME (6*HZ)
#define DEFAULT_FMT_WAKE_TIME (HZ/2)
#endif

#if defined(NOISY_DEBUG)
#  define _dbg(dev, format, arg...) dev_dbg(dev, format, ## arg)
#else
#  define _dbg(dev, format, arg...) do { } while (0)
#endif

#include "sipc.h"
#include "pdp.h"

#define SVNET_DEV_ADDR 0xa0

enum {
	SVNET_NORMAL = 0,
	SVNET_RESET,
	SVNET_EXIT,
	SVNET_MAX,
};

struct svnet_stat {
	unsigned int st_wq_state;
	unsigned long st_recv_evt;
	unsigned long st_recv_pkt_ph;
	unsigned long st_recv_pkt_pdp;
	unsigned long st_do_write;
	unsigned long st_do_read;
	unsigned long st_do_rx;
};
static struct svnet_stat stat;

struct svnet_evt {
	struct list_head list;
	u32 event;
};

struct svnet_evt_head {
	struct list_head list;
	u32 len;
	spinlock_t lock;
};

struct svnet {
	struct net_device *ndev;
	const struct attribute_group *group;

	struct workqueue_struct *wq;
	struct work_struct work_read;
	struct delayed_work work_write;
	struct delayed_work work_rx;

	struct work_struct work_exit;
	int exit_flag;

	struct sk_buff_head txq;
	struct svnet_evt_head rxq;

	struct sipc *si;
#ifdef CONFIG_HAS_WAKELOCK
	struct wake_lock wlock;
	long wake_time; /* jiffies */ /* wake time for not fmt packet */
	long wake_process_time; /* jiffies */ /* processing wake time */
#endif
};

static struct svnet *svnet_dev;

#ifdef CONFIG_HAS_WAKELOCK
static inline void _wake_lock_init(struct svnet *sn)
{
	wake_lock_init(&sn->wlock, WAKE_LOCK_SUSPEND, "svnet");
	sn->wake_time = DEFAULT_RAW_WAKE_TIME;
	sn->wake_process_time = DEFAULT_FMT_WAKE_TIME;
}

static inline void _wake_lock_destroy(struct svnet *sn)
{
	wake_lock_destroy(&sn->wlock);
}

static inline void _wake_lock_timeout(struct svnet *sn)
{
	wake_lock_timeout(&sn->wlock, sn->wake_time);
}

void _non_fmt_wakelock_timeout(void)
{
	if (svnet_dev)
		_wake_lock_timeout(svnet_dev);
}

static inline void _wake_process_lock_timeout(struct svnet *sn)
{
	wake_lock_timeout(&sn->wlock, sn->wake_process_time);
}

void _fmt_wakelock_timeout(void)
{
	if (svnet_dev)
		_wake_process_lock_timeout(svnet_dev);
}

static inline void _wake_lock_settime(struct svnet *sn, long time)
{
	if (sn)
		sn->wake_time = time;
}

static inline long _wake_lock_gettime(struct svnet *sn)
{
	return sn ? sn->wake_time : DEFAULT_RAW_WAKE_TIME;
}
#else
#define _wake_lock_init(sn) do { } while (0)
#define _wake_lock_destroy(sn) do { } while (0)
#define _wake_lock_timeout(sn) do { } while (0)
#define _wake_process_lock_timeout(sn) do { } while (0)
#define _non_fmt_wakelock_timeout() do { } while (0)
#define _fmt_wakelock_timeout() do { } while (0)
#define _wake_lock_settime(sn, time) do { } while (0)
#define _wake_lock_gettime(sn) (0)
#endif

static unsigned long long tmp_itor;
static unsigned long long tmp_xtow;
static unsigned long long time_max_itor;
static unsigned long long time_max_xtow;
static unsigned long long time_max_read;
static unsigned long long time_max_write;

/*
extern unsigned long long time_max_semlat;
*/
static int _queue_evt(struct svnet_evt_head *h, u32 event);

static ssize_t show_version(struct device *d,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Samsung IPC version %s\n", sipc_version);
}

static ssize_t show_waketime(struct device *d,
		struct device_attribute *attr, char *buf)
{
	char *p = buf;
	unsigned int msec;
	unsigned long j;

	if (!svnet_dev)
		return 0;

	j = _wake_lock_gettime(svnet_dev);
	msec = jiffies_to_msecs(j);
	p += sprintf(p, "%u\n", msec);

	return p - buf;
}

static ssize_t store_waketime(struct device *d,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long msec;
	unsigned long j;
	int r;

	if (!svnet_dev)
		return count;

	r = strict_strtoul(buf, 10, &msec);
	if (r)
		return count;

	j = msecs_to_jiffies(msec);
	_wake_lock_settime(svnet_dev, j);

	return count;
}

static inline int _show_stat(char *buf)
{
	char *p = buf;

	p += sprintf(p, "Stat --------\n");
	p += sprintf(p, "\twork state: %d\n", stat.st_wq_state);
	p += sprintf(p, "\trecv mailbox: %lu\n", stat.st_recv_evt);
	p += sprintf(p, "\trecv phonet: %lu\n", stat.st_recv_pkt_ph);
	p += sprintf(p, "\trecv packet: %lu\n", stat.st_recv_pkt_pdp);
	p += sprintf(p, "\twrite count: %lu\n", stat.st_do_write);
	p += sprintf(p, "\tread count: %lu\n", stat.st_do_read);
	p += sprintf(p, "\trx count: %lu\n", stat.st_do_rx);
	p += sprintf(p, "\n");

	return p - buf;
}

static ssize_t show_latency(struct device *d,
		struct device_attribute *attr, char *buf)
{
	char *p = buf;

	p += sprintf(p, "Max read latency:  %12llu ns\n", time_max_itor);
	p += sprintf(p, "Max read time:     %12llu ns\n", time_max_read);
	p += sprintf(p, "Max write latency: %12llu ns\n", time_max_xtow);
	p += sprintf(p, "Max write time:    %12llu ns\n", time_max_write);
	/*p += sprintf(p, "Max sem. latency:  %12llu ns\n", time_max_semlat);*/

	return p - buf;
}

static ssize_t show_debug(struct device *d,
		struct device_attribute *attr, char *buf)
{
	char *p = buf;

	if (!svnet_dev)
		return 0;

	p += _show_stat(p);

	p += sprintf(p, "Event queue -----\n");
	p += sprintf(p, "\tTX queue\t%u\n", skb_queue_len(&svnet_dev->txq));
	p += sprintf(p, "\tRX queue\t%u\n", svnet_dev->rxq.len);

	p += sipc_debug_show(svnet_dev->si, p);

	return p - buf;
}

static ssize_t store_debug(struct device *d,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (!svnet_dev)
		return count;

	switch (buf[0]) {
	case 'R':
		sipc_debug(svnet_dev->si, buf);
		break;
	default:

		break;
	}

	return count;
}

static ssize_t store_whitelist(struct device *d,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (!svnet_dev)
		return count;

	switch (buf[0]) {
	case 0x7F:
		return sipc_whitelist(svnet_dev->si, buf, count);
		break;
	default:

		break;
	}

	return count;
}

static DEVICE_ATTR(version, S_IRUGO, show_version, NULL);
static DEVICE_ATTR(latency, S_IRUGO, show_latency, NULL);
static DEVICE_ATTR(waketime, S_IRUGO | S_IWUSR | S_IWGRP,
			show_waketime, store_waketime);
static DEVICE_ATTR(debug, S_IRUGO | S_IWUSR, show_debug, store_debug);
static DEVICE_ATTR(whitelist, S_IRUSR | S_IWUSR, NULL, store_whitelist);

static struct attribute *svnet_attributes[] = {
	&dev_attr_version.attr,
	&dev_attr_waketime.attr,
	&dev_attr_debug.attr,
	&dev_attr_latency.attr,
	&dev_attr_whitelist.attr,
	NULL
};

static const struct attribute_group svnet_group = {
	.attrs = svnet_attributes,
};


int vnet_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct svnet *sn;
	struct pdp_priv *priv;

	dev_dbg(&ndev->dev, "recv inet packet %p: %d bytes\n", skb, skb->len);
	stat.st_recv_pkt_pdp++;

	priv = netdev_priv(ndev);
	if (!priv)
		goto drop;

	sn = netdev_priv(priv->parent);
	if (!sn)
		goto drop;

	if (!tmp_xtow)
		tmp_xtow = cpu_clock(smp_processor_id());

	skb_queue_tail(&sn->txq, skb);

	_wake_process_lock_timeout(sn);
	queue_delayed_work(sn->wq, &sn->work_write, 0);

	return NETDEV_TX_OK;

drop:
	ndev->stats.tx_dropped++;

	return NETDEV_TX_OK;
}

static int svnet_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct svnet *sn;

	if (skb->protocol != __constant_htons(ETH_P_PHONET)) {
		dev_err(&ndev->dev, "recv not a phonet message\n");
		goto drop;
	}

	stat.st_recv_pkt_ph++;
	dev_dbg(&ndev->dev, "recv packet %p: %d bytes\n", skb, skb->len);

	sn = netdev_priv(ndev);

	if (sipc_check_skb(sn->si, skb)) {
		sipc_do_cmd(sn->si, skb);
		return NETDEV_TX_OK;
	}

	if (!tmp_xtow)
		tmp_xtow = cpu_clock(smp_processor_id());

	skb_queue_tail(&sn->txq, skb);

	_wake_process_lock_timeout(sn);
	queue_delayed_work(sn->wq, &sn->work_write, 0);

	return NETDEV_TX_OK;

drop:
	dev_kfree_skb(skb);
	ndev->stats.tx_dropped++;
	return NETDEV_TX_OK;
}

static int _queue_evt(struct svnet_evt_head *h, u32 event)
{
	unsigned long flags;
	struct svnet_evt *e;

	e = kmalloc(sizeof(struct svnet_evt), GFP_ATOMIC);
	if (!e)
		return -ENOMEM;

	e->event = event;

	spin_lock_irqsave(&h->lock, flags);
	list_add_tail(&e->list, &h->list);
	h->len++;
	spin_unlock_irqrestore(&h->lock, flags);

	return 0;
}

static void _queue_purge(struct svnet_evt_head *h)
{
	unsigned long flags;
	struct svnet_evt *e, *next;

	spin_lock_irqsave(&h->lock, flags);
	list_for_each_entry_safe(e, next, &h->list, list) {
		list_del(&e->list);
		h->len--;
		kfree(e);
	}
	spin_unlock_irqrestore(&h->lock, flags);
}

static u32 _dequeue_evt(struct svnet_evt_head *h)
{
	unsigned long flags;
	struct list_head *p;
	struct svnet_evt *e;
	u32 event;

	spin_lock_irqsave(&h->lock, flags);
	p = h->list.next;
	if (p == &h->list) {
		e = NULL;
		event = 0;
	} else {
		e = list_entry(p, struct svnet_evt, list);
		list_del(&e->list);
		h->len--;
		event = e->event;
	}
	spin_unlock_irqrestore(&h->lock, flags);

	if (e)
		kfree(NULL);

	return event;
}

static int _proc_private_event(struct svnet *sn, u32 evt)
{
	switch (evt) {
	case SIPC_EXIT_MB:
		dev_err(&sn->ndev->dev, "Modem crash message received\n");
		sn->exit_flag = SVNET_EXIT;
		break;
	case SIPC_RESET_MB:
		dev_err(&sn->ndev->dev, "Modem reset message received\n");
		sn->exit_flag = SVNET_RESET;
		break;
	default:
		return 0;
	}

	/* //block for pdp_activate fail - jongmoon.suh
	queue_work(sn->wq, &sn->work_exit);
	*/

	return 1;
}

static void svnet_queue_event(u32 evt, void *data)
{
	struct net_device *ndev = (struct net_device *)data;
	struct svnet *sn;
	int r;

	if (!tmp_itor)
		tmp_itor = cpu_clock(smp_processor_id());

	stat.st_recv_evt++;

	if (!ndev)
		return;

	sn = netdev_priv(ndev);
	if (!sn)
		return;

	r = _proc_private_event(sn, evt);
	if (r)
		return;

	r = _queue_evt(&sn->rxq, evt);
	if (r) {
		dev_err(&sn->ndev->dev, "Not enough memory: event skipped\n");
		return;
	}

	_wake_process_lock_timeout(sn);
	queue_work(sn->wq, &sn->work_read);
}

static int svnet_open(struct net_device *ndev)
{
	struct svnet *sn = netdev_priv(ndev);

	dev_dbg(&ndev->dev, "%s\n", __func__);

	/* TODO: check modem state */

	if (!sn->si) {
		sn->si = sipc_open(svnet_queue_event, ndev);
		if (IS_ERR(sn->si)) {
			dev_err(&ndev->dev, "IPC init error\n");
			return PTR_ERR(sn->si);
		}
		sn->exit_flag = SVNET_NORMAL;
	}

	netif_wake_queue(ndev);
	return 0;
}

static int svnet_close(struct net_device *ndev)
{
	struct svnet *sn = netdev_priv(ndev);

	dev_dbg(&ndev->dev, "%s\n", __func__);

	if (sn->wq)
		flush_workqueue(sn->wq);
	skb_queue_purge(&sn->txq);

	if (sn->si)
		sipc_close(&sn->si);

	netif_stop_queue(ndev);

	if (sn->wq)
		flush_workqueue(sn->wq);
	skb_queue_purge(&sn->txq);

	return 0;
}

static int svnet_ioctl(struct net_device *ndev, struct ifreq *ifr, int cmd)
{
	struct if_phonet_req *req = (struct if_phonet_req *)ifr;

	switch (cmd) {
	case SIOCPNGAUTOCONF:
		req->ifr_phonet_autoconf.device = SVNET_DEV_ADDR;
		return 0;
	}

	return -ENOIOCTLCMD;
}

static const struct net_device_ops svnet_ops = {
	.ndo_open = svnet_open,
	.ndo_stop = svnet_close,
	.ndo_start_xmit = svnet_xmit,
	.ndo_do_ioctl = svnet_ioctl,
};

static void svnet_setup(struct net_device *ndev)
{
	ndev->features = 0;
	ndev->netdev_ops = &svnet_ops;
	ndev->header_ops = &phonet_header_ops;
	ndev->type = ARPHRD_PHONET;
	ndev->flags = IFF_POINTOPOINT | IFF_NOARP;
	ndev->mtu = PHONET_MAX_MTU;
	ndev->hard_header_len = 1;
	ndev->dev_addr[0] = SVNET_DEV_ADDR;
	ndev->addr_len = 1;
	ndev->tx_queue_len = 1000;

/*	ndev->destructor = free_netdev; */
}

static void svnet_read_wq(struct work_struct *work)
{
	struct svnet *sn = container_of(work,
			struct svnet, work_read);
	u32 event;
	int r = 0;
	int contd = 0;
	unsigned long long t, d;

	t = cpu_clock(smp_processor_id());
	if (tmp_itor) {
		d = t - tmp_itor;
		_dbg(&sn->ndev->dev, "int_to_read %llu ns\n", d);
		tmp_itor = 0;
		if (time_max_itor < d)
			time_max_itor = d;
	}

	dev_dbg(&sn->ndev->dev, "%s\n", __func__);
	stat.st_do_read++;

	stat.st_wq_state = 1;
	event = _dequeue_evt(&sn->rxq);
	while (event) {
		/* isn't it possible that merge the events? */
		dev_dbg(&sn->ndev->dev, "event %x\n", event);

		if (sn->si) {
			r = sipc_read(sn->si, event, &contd);
			if (r < 0) {
				dev_err(&sn->ndev->dev, "ret %d -> queue %x\n",
						r, event);
				_queue_evt(&sn->rxq, event);
				break;
			}
		} else {
			dev_err(&sn->ndev->dev,
					"IPC not work, skip event %x\n", event);
		}
		event = _dequeue_evt(&sn->rxq);
	}

	if (contd > 0)
		queue_delayed_work(sn->wq, &sn->work_rx, 0);

	switch (r) {
	case -EINVAL:
		dev_err(&sn->ndev->dev, "Invalid argument\n");
		break;
	case -EBADMSG:
		dev_err(&sn->ndev->dev, "Bad message, purge the buffer\n");
		break;
	case -ETIMEDOUT:
		dev_err(&sn->ndev->dev, "Timed out\n");
		break;
	default:

		break;
	}

	stat.st_wq_state = 2;

	d = cpu_clock(smp_processor_id()) - t;
	_dbg(&sn->ndev->dev, "read_time %llu ns\n", d);
	if (d > time_max_read)
		time_max_read = d;
}

static void svnet_write_wq(struct work_struct *work)
{
	struct svnet *sn = container_of(work,
			struct svnet, work_write.work);
	int r;
	unsigned long long t, d;

	t = cpu_clock(smp_processor_id());
	if (tmp_xtow) {
		d = t - tmp_xtow;
		_dbg(&sn->ndev->dev, "xmit_to_write %llu ns\n", d);
		tmp_xtow = 0;
		if (d > time_max_xtow)
			time_max_xtow = d;
	}

	dev_dbg(&sn->ndev->dev, "%s\n", __func__);
	stat.st_do_write++;

	stat.st_wq_state = 3;
	if (sn->si)
		r = sipc_write(sn->si, &sn->txq);
	else {
		skb_queue_purge(&sn->txq);
		dev_err(&sn->ndev->dev, "IPC not work, drop packet\n");
		r = 0;
	}

	switch (r) {
	case -ENOSPC:
		dev_err(&sn->ndev->dev, "buffer is full, wait...\n");
		queue_delayed_work(sn->wq, &sn->work_write, HZ/10);
		break;
	case -EINVAL:
		dev_err(&sn->ndev->dev, "Invalid arugment\n");
		break;
	case -ENXIO:
		dev_err(&sn->ndev->dev, "IPC not work, purge the queue\n");
		break;
	case -ETIMEDOUT:
		dev_err(&sn->ndev->dev, "Timed out\n");
		break;
	default:
		/* do nothing */
		break;
	}

	stat.st_wq_state = 4;
	d = cpu_clock(smp_processor_id()) - t;
	_dbg(&sn->ndev->dev, "write_time %llu ns\n", d);
	if (d > time_max_write)
		time_max_write = d;
}

static void svnet_rx_wq(struct work_struct *work)
{
	struct svnet *sn = container_of(work,
			struct svnet, work_rx.work);
	int r = 0;

	dev_dbg(&sn->ndev->dev, "%s\n", __func__);
	stat.st_do_rx++;

	stat.st_wq_state = 5;
	if (sn->si)
		r = sipc_rx(sn->si);

	if (r > 0)
		queue_delayed_work(sn->wq, &sn->work_rx, HZ/10);

	stat.st_wq_state = 6;
}

static char *uevent_envs[SVNET_MAX] = {
	"",
	"MAILBOX=cp_reset", /* reset */
	"MAILBOX=cp_exit", /* exit */
};
static void svnet_exit_wq(struct work_struct *work)
{
	struct svnet *sn = container_of(work,
			struct svnet, work_exit);
	char *envs[2] = { NULL, NULL };

	dev_dbg(&sn->ndev->dev, "%s: %d\n", __func__, sn->exit_flag);

	if (sn->exit_flag == SVNET_NORMAL || sn->exit_flag >= SVNET_MAX)
		return;

	envs[0] = uevent_envs[sn->exit_flag];
	kobject_uevent_env(&sn->ndev->dev.kobj, KOBJ_OFFLINE, envs);

	_queue_purge(&sn->rxq);
	skb_queue_purge(&sn->txq);

	if (sn->exit_flag == SVNET_EXIT)
		sipc_ramdump(sn->si);

#if 0
	rtnl_lock();
	if (netif_running(sn->ndev))
		dev_close(sn->ndev);
	rtnl_unlock();
#endif
}

static inline void _init_data(struct svnet *sn)
{
	INIT_WORK(&sn->work_read, svnet_read_wq);
	INIT_DELAYED_WORK(&sn->work_write, svnet_write_wq);
	INIT_DELAYED_WORK(&sn->work_rx, svnet_rx_wq);
	INIT_WORK(&sn->work_exit, svnet_exit_wq);

	INIT_LIST_HEAD(&sn->rxq.list);
	spin_lock_init(&sn->rxq.lock);
	sn->rxq.len = 0;
	skb_queue_head_init(&sn->txq);
}

static void _free(struct svnet *sn)
{
	if (!sn)
		return;

	_wake_lock_destroy(sn);

	if (sn->group)
		sysfs_remove_group(&sn->ndev->dev.kobj, &svnet_group);

	if (sn->wq) {
		flush_workqueue(sn->wq);
		destroy_workqueue(sn->wq);
	}

	if (sn->si) {
		sipc_close(&sn->si);
		sipc_exit();
	}

	if (sn->ndev)
		unregister_netdev(sn->ndev);

	/* sn is ndev's priv */
	free_netdev(sn->ndev);
}

static int __init svnet_init(void)
{
	int r;
	struct svnet *sn = NULL;
	struct net_device *ndev;

	printk(KERN_ERR "[%s]\n", __func__);
	ndev = alloc_netdev(sizeof(struct svnet), "svnet%d", svnet_setup);
	if (!ndev) {
		r = -ENOMEM;
		goto err;
	}
	netif_stop_queue(ndev);
	sn = netdev_priv(ndev);

	_wake_lock_init(sn);

	r = register_netdev(ndev);
	if (r) {
		dev_err(&ndev->dev, "failed to register netdev\n");
		goto err;
	}
	sn->ndev = ndev;

	_init_data(sn);

	sn->wq = create_workqueue("svnetd");
	if (!sn->wq) {
		dev_err(&ndev->dev, "failed to create a workqueue\n");
		goto err;
	}

	r = sysfs_create_group(&sn->ndev->dev.kobj, &svnet_group);
	if (r) {
		dev_err(&ndev->dev, "failed to create sysfs group\n");
		goto err;
	}
	sn->group = &svnet_group;

	dev_dbg(&ndev->dev, "Svnet dev: %p\n", sn);
	svnet_dev = sn;

	return 0;

err:
	_free(sn);
	return r;
}

static void __exit svnet_exit(void)
{

	_free(svnet_dev);
	svnet_dev = NULL;
}

module_init(svnet_init);
module_exit(svnet_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Suchang Woo <suchang.woo@samsung.com>");
MODULE_DESCRIPTION("Samsung Virtual network interface");
