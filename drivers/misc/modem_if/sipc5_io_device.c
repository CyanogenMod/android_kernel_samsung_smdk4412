/*
 * Copyright (C) 2010 Samsung Electronics.
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

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/etherdevice.h>
#include <linux/device.h>
#include <linux/module.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_utils.h"

static ssize_t show_waketime(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int msec;
	char *p = buf;
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct io_device *iod = container_of(miscdev, struct io_device,
			miscdev);

	msec = jiffies_to_msecs(iod->waketime);

	p += sprintf(buf, "raw waketime : %ums\n", msec);

	return p - buf;
}

static ssize_t store_waketime(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long msec;
	int ret;
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct io_device *iod = container_of(miscdev, struct io_device,
			miscdev);

	ret = strict_strtoul(buf, 10, &msec);
	if (ret)
		return count;

	iod->waketime = msecs_to_jiffies(msec);

	return count;
}

static struct device_attribute attr_waketime =
	__ATTR(waketime, S_IRUGO | S_IWUSR, show_waketime, store_waketime);

static ssize_t show_loopback(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct modem_shared *msd =
		container_of(miscdev, struct io_device, miscdev)->msd;
	unsigned char *ip = (unsigned char *)&msd->loopback_ipaddr;
	char *p = buf;

	p += sprintf(buf, "%u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);

	return p - buf;
}

static ssize_t store_loopback(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct modem_shared *msd =
		container_of(miscdev, struct io_device, miscdev)->msd;

	msd->loopback_ipaddr = ipv4str_to_be32(buf, count);

	return count;
}

static struct device_attribute attr_loopback =
	__ATTR(loopback, S_IRUGO | S_IWUSR, show_loopback, store_loopback);

static void iodev_showtxlink(struct io_device *iod, void *args)
{
	char **p = (char **)args;
	struct link_device *ld = get_current_link(iod);

	if (iod->io_typ == IODEV_NET && IS_CONNECTED(iod, ld))
		*p += sprintf(*p, "%s<->%s\n", iod->name, ld->name);
}

static ssize_t show_txlink(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct modem_shared *msd =
		container_of(miscdev, struct io_device, miscdev)->msd;
	char *p = buf;

	iodevs_for_each(msd, iodev_showtxlink, &p);

	return p - buf;
}

static ssize_t store_txlink(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	/* don't change without gpio dynamic switching */
	return -EINVAL;
}

static struct device_attribute attr_txlink =
	__ATTR(txlink, S_IRUGO | S_IWUSR, show_txlink, store_txlink);

static int netif_flow_ctrl(struct link_device *ld, struct sk_buff *skb)
{
	u8 cmd = skb->data[0];

	if (cmd == FLOW_CTRL_SUSPEND) {
		if (ld->suspend_netif_tx)
			goto exit;
		ld->suspend_netif_tx = true;
		mif_netif_stop(ld);
		mif_info("%s: FLOW_CTRL_SUSPEND\n", ld->name);
	} else if (cmd == FLOW_CTRL_RESUME) {
		if (!ld->suspend_netif_tx)
			goto exit;
		ld->suspend_netif_tx = false;
		mif_netif_wake(ld);
		mif_info("%s: FLOW_CTRL_RESUME\n", ld->name);
	} else {
		mif_info("%s: ERR! invalid command %02X\n", ld->name, cmd);
	}

exit:
	dev_kfree_skb_any(skb);
	return 0;
}

static inline int queue_skb_to_iod(struct sk_buff *skb, struct io_device *iod)
{
	struct sk_buff_head *rxq = &iod->sk_rx_q;

	skb_queue_tail(rxq, skb);

	if (iod->format < IPC_MULTI_RAW && rxq->qlen > MAX_IOD_RXQ_LEN) {
		struct sk_buff *victim = skb_dequeue(rxq);
		mif_err("%s: %s application may be dead (rxq->qlen %d > %d)\n",
			iod->name, iod->app ? iod->app : "corresponding",
			rxq->qlen, MAX_IOD_RXQ_LEN);
		if (victim)
			dev_kfree_skb_any(victim);
		return -ENOSPC;
	} else {
		mif_debug("%s: rxq->qlen = %d\n", iod->name, rxq->qlen);
		return 0;
	}
}

static int rx_drain(struct sk_buff *skb)
{
	dev_kfree_skb_any(skb);
	return 0;
}

static int rx_loopback(struct sk_buff *skb)
{
	struct io_device *iod = skbpriv(skb)->iod;
	struct link_device *ld = skbpriv(skb)->ld;
	int ret;

	ret = ld->send(ld, iod, skb);
	if (ret < 0) {
		mif_err("%s->%s: ERR! ld->send fail (err %d)\n",
			iod->name, ld->name, ret);
	}

	return ret;
}

static int rx_fmt_frame(struct sk_buff *skb)
{
	struct link_device *ld = skbpriv(skb)->ld;
	struct io_device *iod = skbpriv(skb)->iod;
	struct sk_buff *rx_skb;
	int hdr_len = sipc5_get_hdr_len(skb->data);
	u8 ctrl;
	u8 id;

	if (!sipc5_multi_frame(skb->data)) {
		skb_pull(skb, hdr_len);
		queue_skb_to_iod(skb, iod);
		wake_up(&iod->wq);
		return 0;
	}

	/* Get the control field */
	ctrl = sipc5_get_ctrl_field(skb->data);

	/* Extract the control ID from the control field */
	id = ctrl & 0x7F;

	/* Remove SIPC5 link header */
	skb_pull(skb, hdr_len);

	/* If there has been no multiple frame with this ID, ... */
	if (iod->skb[id] == NULL) {
		struct sipc_fmt_hdr *fh = (struct sipc_fmt_hdr *)skb->data;

		mif_err("%s->%s: start of multi-frame (ID:%d len:%d)\n",
			ld->name, iod->name, id, fh->len);

		rx_skb = rx_alloc_skb(fh->len, iod, ld);
		if (!rx_skb) {
			mif_err("%s: ERR! rx_alloc_skb fail\n", iod->name);
			return -ENOMEM;
		}

		iod->skb[id] = rx_skb;
	} else {
		rx_skb = iod->skb[id];
	}

	/* Perform multi-frame processing */
	memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);
	dev_kfree_skb_any(skb);

	if (ctrl & 0x80) {
		/* The last frame has not arrived yet. */
		mif_info("%s->%s: recv multi-frame (ID:%d rcvd:%d)\n",
			ld->name, iod->name, id, rx_skb->len);
	} else {
		/* It is the last frame because the "more" bit is 0. */
		mif_err("%s->%s: end of multi-frame (ID:%d rcvd:%d)\n",
			ld->name, iod->name, id, rx_skb->len);
		queue_skb_to_iod(rx_skb, iod);
		iod->skb[id] = NULL;
		wake_up(&iod->wq);
	}

	return 0;
}

static int rx_raw_misc(struct sk_buff *skb)
{
	struct io_device *iod = skbpriv(skb)->iod;

	/* Remove the SIPC5 link header */
	skb_pull(skb, sipc5_get_hdr_len(skb->data));

	queue_skb_to_iod(skb, iod);
	wake_up(&iod->wq);

	return 0;
}

static int rx_multi_pdp(struct sk_buff *skb)
{
	struct link_device *ld = skbpriv(skb)->ld;
	struct io_device *iod = skbpriv(skb)->iod;
	struct net_device *ndev;
	struct iphdr *iphdr;
	int ret;

	ndev = iod->ndev;
	if (!ndev) {
		mif_info("%s: ERR! no iod->ndev\n", iod->name);
		return -ENODEV;
	}

	/* Remove the SIPC5 link header */
	skb_pull(skb, sipc5_get_hdr_len(skb->data));

	skb->dev = ndev;
	ndev->stats.rx_packets++;
	ndev->stats.rx_bytes += skb->len;

	/* check the version of IP */
	iphdr = (struct iphdr *)skb->data;
	if (iphdr->version == IP6VERSION)
		skb->protocol = htons(ETH_P_IPV6);
	else
		skb->protocol = htons(ETH_P_IP);

	if (iod->use_handover) {
		struct ethhdr *ehdr;
		const char source[ETH_ALEN] = SOURCE_MAC_ADDR;

		ehdr = (struct ethhdr *)skb_push(skb, sizeof(struct ethhdr));
		memcpy(ehdr->h_dest, ndev->dev_addr, ETH_ALEN);
		memcpy(ehdr->h_source, source, ETH_ALEN);
		ehdr->h_proto = skb->protocol;
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		skb_reset_mac_header(skb);
		skb_pull(skb, sizeof(struct ethhdr));
	}

	if (in_interrupt())
		ret = netif_rx(skb);
	else
		ret = netif_rx_ni(skb);

	if (ret != NET_RX_SUCCESS) {
		mif_err("%s->%s: ERR! netif_rx fail (err %d)\n",
			ld->name, iod->name, ret);
	}

	return ret;
}

static int rx_demux(struct link_device *ld, struct sk_buff *skb)
{
	struct io_device *iod;
	u8 ch = sipc5_get_ch_id(skb->data);
#ifdef DEBUG_MODEM_IF
	struct modem_ctl *mc = ld->mc;
	size_t len = (skb->len > 20) ? 20 : skb->len;
	char tag[MIF_MAX_STR_LEN];
#endif

	if (unlikely(ch == 0)) {
		mif_err("%s: ERR! invalid ch# %d\n", ld->name, ch);
		return -ENODEV;
	}

	if (unlikely(ch == SIPC5_CH_ID_FLOW_CTRL))
		return netif_flow_ctrl(ld, skb);

	/* IP loopback */
	if (ch == DATA_LOOPBACK_CHANNEL && ld->msd->loopback_ipaddr)
		ch = RMNET0_CH_ID;

	iod = link_get_iod_with_channel(ld, ch);
	if (unlikely(!iod)) {
		mif_err("%s: ERR! no iod with ch# %d\n", ld->name, ch);
		return -ENODEV;
	}

	skbpriv(skb)->ld = ld;
	skbpriv(skb)->iod = iod;

	/* Don't care whether or not DATA_DRAIN_CHANNEL is opened */
	if (iod->id == DATA_DRAIN_CHANNEL)
		return rx_drain(skb);

	/* Don't care whether or not DATA_LOOPBACK_CHANNEL is opened */
	if (iod->id == DATA_LOOPBACK_CHANNEL)
		return rx_loopback(skb);

#ifdef DEBUG_MODEM_IF
	snprintf(tag, MIF_MAX_STR_LEN, "LNK: %s->%s", mc->name, iod->name);
	if (unlikely(iod->format == IPC_FMT))
		pr_ipc(1, tag, skb->data, len);
#if 0
	if (iod->format == IPC_RAW)
		pr_ipc(0, tag, skb->data, len);
#endif
#if 0
	if (iod->format == IPC_BOOT)
		pr_ipc(0, tag, skb->data, len);
#endif
#if 0
	if (iod->format == IPC_RAMDUMP)
		pr_ipc(0, tag, skb->data, len);
#endif
#if 0
	if (ch == 28)
		pr_ipc(0, tag, skb->data, len);
#endif
#endif /*DEBUG_MODEM_IF*/

	if (atomic_read(&iod->opened) <= 0) {
		mif_err("%s: ERR! %s is not opened\n", ld->name, iod->name);
		return -ENODEV;
	}

	if (ch >= SIPC5_CH_ID_RFS_0)
		return rx_raw_misc(skb);
	else if (ch >= SIPC5_CH_ID_FMT_0)
		return rx_fmt_frame(skb);
	else if (iod->io_typ == IODEV_MISC)
		return rx_raw_misc(skb);
	else
		return rx_multi_pdp(skb);
}

/**
 * rx_frame_config
 * @iod: pointer to an instance of io_device structure
 * @ld: pointer to an instance of link_device structure
 * @buff: pointer to a buffer in which incoming data is stored
 * @size: size of data in the buffer
 * @frm: pointer to an instance of sipc5_frame_data structure
 *
 * 1) Checks a config field
 * 2) Calculates the length of link layer header in an incoming frame and stores
 *    the value to "frm->hdr_len"
 * 3) Stores the config field to "frm->hdr" and add the size of config field to
 *    "frm->hdr_rcvd"
 *
 * Returns the length of a config field that was copied to "frm"
 */
static int rx_frame_config(struct io_device *iod, struct link_device *ld,
		u8 *buff, int size, struct sipc5_frame_data *frm)
{
	int rest;
	int rcvd;

	if (unlikely(!sipc5_start_valid(buff))) {
		mif_err("%s->%s: ERR! INVALID config 0x%02x\n",
			ld->name, iod->name, buff[0]);
		return -EBADMSG;
	}

	frm->hdr_len = sipc5_get_hdr_len(buff);

	/* Calculate the size of a segment that will be copied */
	rest = frm->hdr_len;
	rcvd = SIPC5_CONFIG_SIZE;
	mif_debug("%s->%s: hdr_len:%d hdr_rcvd:%d rest:%d size:%d rcvd:%d\n",
		ld->name, iod->name, frm->hdr_len, frm->hdr_rcvd, rest, size,
		rcvd);

	/* Copy the config field of an SIPC5 link header to the header buffer */
	memcpy(frm->hdr, buff, rcvd);
	frm->hdr_rcvd += rcvd;

	return rcvd;
}

/**
 * rx_frame_prepare_skb
 * @iod: pointer to an instance of io_device structure
 * @ld: pointer to an instance of link_device structure
 * @frm: pointer to an instance of sipc5_frame_data structure
 *
 * 1) Extracts the length of a link frame from the link header in "frm->hdr"
 * 2) Allocates an skb
 * 3) Calculates the payload size in the link frame
 * 4) Calculates the padding size in the link frame
 *
 * Returns the pointer to an skb
 */
static struct sk_buff *rx_frame_prepare_skb(struct io_device *iod,
		struct link_device *ld, struct sipc5_frame_data *frm)
{
	struct sk_buff *skb;

	/* Get the frame length */
	frm->len = sipc5_get_frame_len(frm->hdr);

	/* Allocate an skb */
	skb = rx_alloc_skb(frm->len, iod, ld);
	if (!skb) {
		mif_err("%s->%s: ERR! rx_alloc_skb fail (size %d)\n",
			ld->name, iod->name, frm->len);
		return NULL;
	}

	/* Calculates the payload size */
	frm->pay_len = frm->len - frm->hdr_len;

	/* Calculates the padding size */
	if (sipc5_padding_exist(frm->hdr))
		frm->pad_len = sipc5_calc_padding_size(frm->len);

	mif_debug("%s->%s: size %d (header:%d payload:%d padding:%d)\n",
		ld->name, iod->name, frm->len, frm->hdr_len, frm->pay_len,
		frm->pad_len);

	return skb;
}

/**
 * rx_frame_header
 * @iod: pointer to an instance of io_device structure
 * @ld: pointer to an instance of link_device structure
 * @buff: pointer to a buffer in which incoming data is stored
 * @size: size of data in the buffer
 * @frm: pointer to an instance of sipc5_frame_data structure
 *
 * 1) Stores a link layer header to "frm->hdr" temporarily while "frm->hdr_rcvd"
 *    is less than "frm->hdr_len"
 * 2) Then,
 *      Allocates an skb
 *      Copies the link header from "frm" to "skb"
 *      Register the skb to receive payload
 *
 * Returns the size of a segment that was copied to "frm"
 */
static int rx_frame_header(struct io_device *iod, struct link_device *ld,
		u8 *buff, int size, struct sipc5_frame_data *frm)
{
	struct sk_buff *skb;
	int rest;
	int rcvd;

	/* Calculate the size of a segment that will be copied */
	rest = frm->hdr_len - frm->hdr_rcvd;
	rcvd = min(rest, size);
	mif_debug("%s->%s: hdr_len:%d hdr_rcvd:%d rest:%d size:%d rcvd:%d\n",
		ld->name, iod->name, frm->hdr_len, frm->hdr_rcvd, rest, size,
		rcvd);

	/* Copy a segment of an SIPC5 link header to "frm" */
	memcpy((frm->hdr + frm->hdr_rcvd), buff, rcvd);
	frm->hdr_rcvd += rcvd;

	if (frm->hdr_rcvd >= frm->hdr_len) {
		/* Prepare an skb with the information in {iod, ld, frm} */
		skb = rx_frame_prepare_skb(iod, ld, frm);
		if (!skb) {
			mif_err("%s->%s: ERR! rx_frame_prepare_skb fail\n",
				ld->name, iod->name);
			return -ENOMEM;
		}

		/* Copy an SIPC5 link header from "frm" to "skb" */
		memcpy(skb_put(skb, frm->hdr_len), frm->hdr, frm->hdr_len);

		/* Register the skb to receive payload */
		fragdata(iod, ld)->skb_recv = skb;
	}

	return rcvd;
}

/**
 * rx_frame_payload
 * @iod: pointer to an instance of io_device structure
 * @ld: pointer to an instance of link_device structure
 * @buff: pointer to a buffer in which incoming data is stored
 * @size: size of data in the buffer
 * @frm: pointer to an instance of sipc5_frame_data structure
 *
 * Stores a link layer payload to "skb"
 *
 * Returns the size of a segment that was copied to "skb"
 */
static int rx_frame_payload(struct io_device *iod, struct link_device *ld,
		u8 *buff, int size, struct sipc5_frame_data *frm)
{
	struct sk_buff *skb = fragdata(iod, ld)->skb_recv;
	int rest;
	int rcvd;

	/* Calculate the size of a segment that will be copied */
	rest = frm->pay_len - frm->pay_rcvd;
	rcvd = min(rest, size);
	mif_debug("%s->%s: pay_len:%d pay_rcvd:%d rest:%d size:%d rcvd:%d\n",
		ld->name, iod->name, frm->pay_len, frm->pay_rcvd, rest, size,
		rcvd);

	/* Copy an SIPC5 link payload to "skb" */
	memcpy(skb_put(skb, rcvd), buff, rcvd);
	frm->pay_rcvd += rcvd;

	return rcvd;
}

static int rx_frame_padding(struct io_device *iod, struct link_device *ld,
		u8 *buff, int size, struct sipc5_frame_data *frm)
{
	struct sk_buff *skb = fragdata(iod, ld)->skb_recv;
	int rest;
	int rcvd;

	/* Calculate the size of a segment that will be dropped as padding */
	rest = frm->pad_len - frm->pad_rcvd;
	rcvd = min(rest, size);
	mif_debug("%s->%s: pad_len:%d pad_rcvd:%d rest:%d size:%d rcvd:%d\n",
		ld->name, iod->name, frm->pad_len, frm->pad_rcvd, rest, size,
		rcvd);

	/* Copy an SIPC5 link padding to "skb" */
	memcpy(skb_put(skb, rcvd), buff, rcvd);
	frm->pad_rcvd += rcvd;

	return rcvd;
}

static int rx_frame_done(struct io_device *iod, struct link_device *ld,
		struct sk_buff *skb)
{
	/* Cut off the padding of the current frame */
	skb_trim(skb, sipc5_get_frame_len(skb->data));
	mif_debug("%s->%s: frame length = %d\n", ld->name, iod->name, skb->len);

	return rx_demux(ld, skb);
}

static int recv_frame_from_buff(struct io_device *iod, struct link_device *ld,
		const char *data, unsigned size)
{
	struct sipc5_frame_data *frm = &fragdata(iod, ld)->f_data;
	struct sk_buff *skb;
	u8 *buff = (u8 *)data;
	int rest = (int)size;
	int done = 0;
	int err = 0;

	mif_debug("%s->%s: size %d (RX state = %s)\n", ld->name, iod->name,
		size, get_rx_state_str(iod->curr_rx_state));

	while (rest > 0) {
		switch (iod->curr_rx_state) {
		case IOD_RX_ON_STANDBY:
			fragdata(iod, ld)->skb_recv = NULL;
			memset(frm, 0, sizeof(struct sipc5_frame_data));

			done = rx_frame_config(iod, ld, buff, rest, frm);
			if (done < 0) {
				err = done;
				goto err_exit;
			}

			iod->next_rx_state = IOD_RX_HEADER;

			break;

		case IOD_RX_HEADER:
			done = rx_frame_header(iod, ld, buff, rest, frm);
			if (done < 0) {
				err = done;
				goto err_exit;
			}

			if (frm->hdr_rcvd >= frm->hdr_len)
				iod->next_rx_state = IOD_RX_PAYLOAD;
			else
				iod->next_rx_state = IOD_RX_HEADER;

			break;

		case IOD_RX_PAYLOAD:
			done = rx_frame_payload(iod, ld, buff, rest, frm);
			if (done < 0) {
				err = done;
				goto err_exit;
			}

			if (frm->pay_rcvd >= frm->pay_len) {
				if (frm->pad_len > 0)
					iod->next_rx_state = IOD_RX_PADDING;
				else
					iod->next_rx_state = IOD_RX_ON_STANDBY;
			} else {
				iod->next_rx_state = IOD_RX_PAYLOAD;
			}

			break;

		case IOD_RX_PADDING:
			done = rx_frame_padding(iod, ld, buff, rest, frm);
			if (done < 0) {
				err = done;
				goto err_exit;
			}

			if (frm->pad_rcvd >= frm->pad_len)
				iod->next_rx_state = IOD_RX_ON_STANDBY;
			else
				iod->next_rx_state = IOD_RX_PADDING;

			break;

		default:
			mif_err("%s->%s: ERR! INVALID RX state %d\n",
				ld->name, iod->name, iod->curr_rx_state);
			err = -EINVAL;
			goto err_exit;
		}

		if (iod->next_rx_state == IOD_RX_ON_STANDBY) {
			/*
			** A complete frame is in fragdata(iod, ld)->skb_recv.
			*/
			skb = fragdata(iod, ld)->skb_recv;
			err = rx_frame_done(iod, ld, skb);
			if (err < 0)
				goto err_exit;
		}

		buff += done;
		rest -= done;
		if (rest < 0)
			goto err_range;

		iod->curr_rx_state = iod->next_rx_state;
	}

	return size;

err_exit:
	if (fragdata(iod, ld)->skb_recv) {
		mif_err("%s->%s: ERR! clear frag (size:%d done:%d rest:%d)\n",
			ld->name, iod->name, size, done, rest);
		dev_kfree_skb_any(fragdata(iod, ld)->skb_recv);
		fragdata(iod, ld)->skb_recv = NULL;
	}
	iod->curr_rx_state = IOD_RX_ON_STANDBY;
	return err;

err_range:
	mif_err("%s->%s: ERR! size:%d done:%d rest:%d\n",
		ld->name, iod->name, size, done, rest);
	iod->curr_rx_state = IOD_RX_ON_STANDBY;
	return size;
}

/* called from link device when a packet arrives for this io device */
static int io_dev_recv_data_from_link_dev(struct io_device *iod,
		struct link_device *ld, const char *data, unsigned int len)
{
	struct sk_buff *skb;
	int err;

	switch (iod->format) {
	case IPC_FMT:
	case IPC_RAW:
	case IPC_RFS:
	case IPC_MULTI_RAW:
		if (iod->waketime)
			wake_lock_timeout(&iod->wakelock, iod->waketime);

		err = recv_frame_from_buff(iod, ld, data, len);
		if (err < 0) {
			mif_err("%s->%s: ERR! recv_frame_from_buff fail "
				"(err %d)\n", ld->name, iod->name, err);
		}

		return err;

	default:
		mif_debug("%s->%s: len %d\n", ld->name, iod->name, len);

		/* save packet to sk_buff */
		skb = rx_alloc_skb(len, iod, ld);
		if (!skb) {
			mif_info("%s->%s: ERR! rx_alloc_skb fail\n",
				ld->name, iod->name);
			return -ENOMEM;
		}

		memcpy(skb_put(skb, len), data, len);

		queue_skb_to_iod(skb, iod);

		wake_up(&iod->wq);

		return len;
	}
}

static int recv_frame_from_skb(struct io_device *iod, struct link_device *ld,
		struct sk_buff *skb)
{
	struct sk_buff *clone;
	unsigned int rest;
	unsigned int rcvd;
	int tot;		/* total length including padding */
	int err = 0;

	/*
	** If there is only one SIPC5 frame in @skb, receive the SIPC5 frame and
	** return immediately. In this case, the frame verification must already
	** have been done at the link device.
	*/
	if (skbpriv(skb)->single_frame) {
		err = rx_frame_done(iod, ld, skb);
		if (err < 0)
			goto exit;
		return 0;
	}

	/*
	** The routine from here is used only if there may be multiple SIPC5
	** frames in @skb.
	*/

	/* Check the config field of the first frame in @skb */
	if (!sipc5_start_valid(skb->data)) {
		mif_err("%s->%s: ERR! INVALID config 0x%02X\n",
			ld->name, iod->name, skb->data[0]);
		err = -EINVAL;
		goto exit;
	}

	/* Get the total length of the frame with a padding */
	tot = sipc5_get_total_len(skb->data);

	/* Verify the total length of the first frame */
	rest = skb->len;
	if (unlikely(tot > rest)) {
		mif_err("%s->%s: ERR! tot %d > skb->len %d)\n",
			ld->name, iod->name, tot, rest);
		err = -EINVAL;
		goto exit;
	}

	/* If there is only one SIPC5 frame in @skb, */
	if (likely(tot == rest)) {
		/* Receive the SIPC5 frame and return immediately */
		err = rx_frame_done(iod, ld, skb);
		if (err < 0)
			goto exit;
		return 0;
	}

	/*
	** This routine is used only if there are multiple SIPC5 frames in @skb.
	*/
	rcvd = 0;
	while (rest > 0) {
		clone = skb_clone(skb, GFP_ATOMIC);
		if (unlikely(!clone)) {
			mif_err("%s->%s: ERR! skb_clone fail\n",
				ld->name, iod->name);
			err = -ENOMEM;
			goto exit;
		}

		/* Get the start of an SIPC5 frame */
		skb_pull(clone, rcvd);
		if (!sipc5_start_valid(clone->data)) {
			mif_err("%s->%s: ERR! INVALID config 0x%02X\n",
				ld->name, iod->name, clone->data[0]);
			dev_kfree_skb_any(clone);
			err = -EINVAL;
			goto exit;
		}

		/* Get the total length of the current frame with a padding */
		tot = sipc5_get_total_len(clone->data);
		if (unlikely(tot > rest)) {
			mif_err("%s->%s: ERR! dirty frame (tot %d > rest %d)\n",
				ld->name, iod->name, tot, rest);
			dev_kfree_skb_any(clone);
			err = -EINVAL;
			goto exit;
		}

		/* Cut off the padding of the current frame */
		skb_trim(clone, sipc5_get_frame_len(clone->data));

		/* Demux the frame */
		err = rx_demux(ld, clone);
		if (err < 0) {
			mif_err("%s->%s: ERR! rx_demux fail (err %d)\n",
				ld->name, iod->name, err);
			dev_kfree_skb_any(clone);
			goto exit;
		}

		/* Calculate the start of the next frame */
		rcvd += tot;

		/* Calculate the rest size of data in @skb */
		rest -= tot;
	}

exit:
	dev_kfree_skb_any(skb);
	return err;
}

/* called from link device when a packet arrives for this io device */
static int io_dev_recv_skb_from_link_dev(struct io_device *iod,
		struct link_device *ld, struct sk_buff *skb)
{
	enum dev_format dev = iod->format;
	int err;

	switch (dev) {
	case IPC_FMT:
	case IPC_RAW:
	case IPC_RFS:
	case IPC_MULTI_RAW:
		if (iod->waketime)
			wake_lock_timeout(&iod->wakelock, iod->waketime);

		err = recv_frame_from_skb(iod, ld, skb);
		if (err < 0) {
			mif_err("%s->%s: ERR! recv_frame_from_skb fail "
				"(err %d)\n", ld->name, iod->name, err);
		}

		return err;

	case IPC_BOOT:
	case IPC_RAMDUMP:
		if (!iod->id) {
			mif_err("%s->%s: ERR! invalid iod\n",
				ld->name, iod->name);
			return -ENODEV;
		}

		if (iod->waketime)
			wake_lock_timeout(&iod->wakelock, iod->waketime);

		err = recv_frame_from_skb(iod, ld, skb);
		if (err < 0) {
			mif_err("%s->%s: ERR! recv_frame_from_skb fail "
				"(err %d)\n", ld->name, iod->name, err);
		}

		return err;

	default:
		mif_err("%s->%s: ERR! invalid iod\n", ld->name, iod->name);
		return -EINVAL;
	}
}

/* inform the IO device that the modem is now online or offline or
 * crashing or whatever...
 */
static void io_dev_modem_state_changed(struct io_device *iod,
			enum modem_state state)
{
	struct modem_ctl *mc = iod->mc;
	int old_state = mc->phone_state;

	if (old_state != state) {
		mc->phone_state = state;
		mif_err("%s state changed (%s -> %s)\n", mc->name,
			get_cp_state_str(old_state), get_cp_state_str(state));
	}

	if (state == STATE_CRASH_RESET || state == STATE_CRASH_EXIT ||
	    state == STATE_NV_REBUILDING)
		wake_up(&iod->wq);
}

/**
 * io_dev_sim_state_changed
 * @iod:	IPC's io_device
 * @sim_online: SIM is online?
 */
static void io_dev_sim_state_changed(struct io_device *iod, bool sim_online)
{
	if (atomic_read(&iod->opened) == 0) {
		mif_info("%s: ERR! not opened\n", iod->name);
	} else if (iod->mc->sim_state.online == sim_online) {
		mif_info("%s: SIM state not changed\n", iod->name);
	} else {
		iod->mc->sim_state.online = sim_online;
		iod->mc->sim_state.changed = true;
		mif_info("%s: SIM state changed {online %d, changed %d}\n",
			iod->name, iod->mc->sim_state.online,
			iod->mc->sim_state.changed);
		wake_up(&iod->wq);
	}
}

static void iodev_dump_status(struct io_device *iod, void *args)
{
	if (iod->format == IPC_RAW && iod->io_typ == IODEV_NET) {
		struct link_device *ld = get_current_link(iod);
		mif_com_log(iod->mc->msd, "%s: %s\n", iod->name, ld->name);
	}
}

static int misc_open(struct inode *inode, struct file *filp)
{
	struct io_device *iod = to_io_device(filp->private_data);
	struct modem_shared *msd = iod->msd;
	struct link_device *ld;
	int ref_cnt;
	int ret;
	filp->private_data = (void *)iod;

	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld) && ld->init_comm) {
			ret = ld->init_comm(ld, iod);
			if (ret < 0) {
				mif_err("%s<->%s: ERR! init_comm fail(%d)\n",
					iod->name, ld->name, ret);
				return ret;
			}
		}
	}

	ref_cnt = atomic_inc_return(&iod->opened);

	if (iod->format == IPC_BOOT || iod->format == IPC_RAMDUMP)
		mif_err("%s (opened %d)\n", iod->name, ref_cnt);
	else
		mif_info("%s (opened %d)\n", iod->name, ref_cnt);

	return 0;
}

static int misc_release(struct inode *inode, struct file *filp)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct modem_shared *msd = iod->msd;
	struct link_device *ld;
	int ref_cnt;

	skb_queue_purge(&iod->sk_rx_q);

	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld) && ld->terminate_comm)
			ld->terminate_comm(ld, iod);
	}

	ref_cnt = atomic_dec_return(&iod->opened);

	if (iod->format == IPC_BOOT || iod->format == IPC_RAMDUMP)
		mif_err("%s (opened %d)\n", iod->name, ref_cnt);
	else
		mif_info("%s (opened %d)\n", iod->name, ref_cnt);

	return 0;
}

static unsigned int misc_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct modem_ctl *mc = iod->mc;

	poll_wait(filp, &iod->wq, wait);

	if (!skb_queue_empty(&iod->sk_rx_q) && mc->phone_state != STATE_OFFLINE)
		return POLLIN | POLLRDNORM;

	if (mc->phone_state == STATE_CRASH_RESET
	    || mc->phone_state == STATE_CRASH_EXIT
	    || mc->phone_state == STATE_NV_REBUILDING
	    || mc->sim_state.changed) {
		if (iod->format == IPC_RAW) {
			msleep(20);
			return 0;
		}
		if (iod->format == IPC_RAMDUMP)
			return 0;
		return POLLHUP;
	} else {
		return 0;
	}
}

static long misc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct link_device *ld = get_current_link(iod);
	struct modem_ctl *mc = iod->mc;
	int p_state;
	char *buff;
	void __user *user_buff;
	unsigned long size;

	switch (cmd) {
	case IOCTL_MODEM_ON:
		if (mc->ops.modem_on) {
			mif_err("%s: IOCTL_MODEM_ON\n", iod->name);
			return mc->ops.modem_on(mc);
		}
		mif_err("%s: !mc->ops.modem_on\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_OFF:
		if (mc->ops.modem_off) {
			mif_err("%s: IOCTL_MODEM_OFF\n", iod->name);
			return mc->ops.modem_off(mc);
		}
		mif_err("%s: !mc->ops.modem_off\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_RESET:
		if (mc->ops.modem_reset) {
			mif_err("%s: IOCTL_MODEM_RESET\n", iod->name);
			return mc->ops.modem_reset(mc);
		}
		mif_err("%s: !mc->ops.modem_reset\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_BOOT_ON:
		if (mc->ops.modem_boot_on) {
			mif_err("%s: IOCTL_MODEM_BOOT_ON\n", iod->name);
			return mc->ops.modem_boot_on(mc);
		}
		mif_err("%s: !mc->ops.modem_boot_on\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_BOOT_OFF:
		if (mc->ops.modem_boot_off) {
			mif_err("%s: IOCTL_MODEM_BOOT_OFF\n", iod->name);
			return mc->ops.modem_boot_off(mc);
		}
		mif_err("%s: !mc->ops.modem_boot_off\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_BOOT_DONE:
		mif_err("%s: IOCTL_MODEM_BOOT_DONE\n", iod->name);
		if (mc->ops.modem_boot_done)
			return mc->ops.modem_boot_done(mc);
		return 0;

	case IOCTL_MODEM_STATUS:
		mif_debug("%s: IOCTL_MODEM_STATUS\n", iod->name);

		p_state = mc->phone_state;
		if ((p_state == STATE_CRASH_RESET) ||
		    (p_state == STATE_CRASH_EXIT)) {
			mif_info("%s: IOCTL_MODEM_STATUS (state %s)\n",
				iod->name, get_cp_state_str(p_state));
		} else if (mc->sim_state.changed) {
			int s_state = mc->sim_state.online ?
					STATE_SIM_ATTACH : STATE_SIM_DETACH;
			mc->sim_state.changed = false;
			return s_state;
		} else if (p_state == STATE_NV_REBUILDING) {
			mif_info("%s: IOCTL_MODEM_STATUS (state %s)\n",
				iod->name, get_cp_state_str(p_state));
			mc->phone_state = STATE_ONLINE;
		}
		return p_state;

	case IOCTL_MODEM_XMIT_BOOT:
		if (ld->xmit_boot) {
			mif_info("%s: IOCTL_MODEM_XMIT_BOOT\n", iod->name);
			return ld->xmit_boot(ld, iod, arg);
		}
		mif_err("%s: !ld->xmit_boot\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_DL_START:
		if (ld->dload_start) {
			mif_info("%s: IOCTL_MODEM_DL_START\n", iod->name);
			return ld->dload_start(ld, iod);
		}
		mif_err("%s: !ld->dload_start\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_FW_UPDATE:
		if (ld->firm_update) {
			mif_info("%s: IOCTL_MODEM_FW_UPDATE\n", iod->name);
			return ld->firm_update(ld, iod, arg);
		}
		mif_err("%s: !ld->firm_update\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_FORCE_CRASH_EXIT:
		if (mc->ops.modem_force_crash_exit) {
			mif_err("%s: IOCTL_MODEM_FORCE_CRASH_EXIT\n",
				iod->name);
			return mc->ops.modem_force_crash_exit(mc);
		}
		mif_err("%s: !mc->ops.modem_force_crash_exit\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_DUMP_RESET:
		if (mc->ops.modem_dump_reset) {
			mif_info("%s: IOCTL_MODEM_DUMP_RESET\n", iod->name);
			return mc->ops.modem_dump_reset(mc);
		}
		mif_err("%s: !mc->ops.modem_dump_reset\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_DUMP_START:
		if (ld->dump_start) {
			mif_err("%s: IOCTL_MODEM_DUMP_START\n", iod->name);
			return ld->dump_start(ld, iod);
		}
		mif_err("%s: !ld->dump_start\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_RAMDUMP_START:
		if (ld->dump_start) {
			mif_info("%s: IOCTL_MODEM_RAMDUMP_START\n", iod->name);
			return ld->dump_start(ld, iod);
		}
		mif_err("%s: !ld->dump_start\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_DUMP_UPDATE:
		if (ld->dump_update) {
			mif_info("%s: IOCTL_MODEM_DUMP_UPDATE\n", iod->name);
			return ld->dump_update(ld, iod, arg);
		}
		mif_err("%s: !ld->dump_update\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_RAMDUMP_STOP:
		if (ld->dump_finish) {
			mif_info("%s: IOCTL_MODEM_RAMDUMP_STOP\n", iod->name);
			return ld->dump_finish(ld, iod, arg);
		}
		mif_err("%s: !ld->dump_finish\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_CP_UPLOAD:
		mif_info("%s: IOCTL_MODEM_CP_UPLOAD\n", iod->name);
		strcpy(iod->msd->cp_crash_info, CP_CRASH_TAG);
		if (arg) {
			buff = iod->msd->cp_crash_info + strlen(CP_CRASH_TAG);
			user_buff = (void __user *)arg;
			if (copy_from_user(buff, user_buff, MAX_CPINFO_SIZE))
				return -EFAULT;
		}
		panic(iod->msd->cp_crash_info);
		return 0;

	case IOCTL_MODEM_PROTOCOL_SUSPEND:
		mif_info("%s: IOCTL_MODEM_PROTOCOL_SUSPEND\n", iod->name);
		if (iod->format == IPC_MULTI_RAW) {
			iodevs_for_each(iod->msd, iodev_netif_stop, 0);
			return 0;
		}
		return -EINVAL;

	case IOCTL_MODEM_PROTOCOL_RESUME:
		mif_info("%s: IOCTL_MODEM_PROTOCOL_RESUME\n", iod->name);
		if (iod->format != IPC_MULTI_RAW) {
			iodevs_for_each(iod->msd, iodev_netif_wake, 0);
			return 0;
		}
		return -EINVAL;

	case IOCTL_MIF_LOG_DUMP:
		iodevs_for_each(iod->msd, iodev_dump_status, 0);
		user_buff = (void __user *)arg;
		size = MAX_MIF_BUFF_SIZE;
		if (copy_to_user(user_buff, &size, sizeof(unsigned long)))
			return -EFAULT;
		mif_dump_log(mc->msd, iod);
		return 0;

	default:
		 /* If you need to handle the ioctl for specific link device,
		  * then assign the link ioctl handler to ld->ioctl
		  * It will be call for specific link ioctl */
		if (ld->ioctl)
			return ld->ioctl(ld, iod, cmd, arg);

		mif_info("%s: ERR! undefined cmd 0x%X\n", iod->name, cmd);
		return -EINVAL;
	}

	return 0;
}

static ssize_t misc_write(struct file *filp, const char __user *data,
			size_t count, loff_t *fpos)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct link_device *ld = get_current_link(iod);
	struct sk_buff *skb;
	u8 *buff;
	int ret;
	size_t headroom;
	size_t tailroom;
	size_t tx_bytes;
	u8 cfg;

	if (iod->format <= IPC_RFS && iod->id == 0)
		return -EINVAL;

	cfg = sipc5_build_config(iod, ld, count);

	if (cfg)
		headroom = sipc5_get_hdr_len(&cfg);
	else
		headroom = 0;

	if (ld->aligned)
		tailroom = sipc5_calc_padding_size(headroom + count);
	else
		tailroom = 0;

	tx_bytes = headroom + count + tailroom;

	skb = alloc_skb(tx_bytes, GFP_KERNEL);
	if (!skb) {
		mif_info("%s: ERR! alloc_skb fail (tx_bytes:%d)\n",
			iod->name, tx_bytes);
		return -ENOMEM;
	}

	/* Build SIPC5 link header*/
	if (cfg) {
		buff = skb_put(skb, headroom);
		sipc5_build_header(iod, ld, buff, cfg, 0, count);
	}

	/* Store IPC message */
	buff = skb_put(skb, count);
	if (copy_from_user(buff, data, count)) {
		mif_err("%s->%s: ERR! copy_from_user fail (count %d)\n",
			iod->name, ld->name, count);
		dev_kfree_skb_any(skb);
		return -EFAULT;
	}

	/* Apply padding */
	if (tailroom)
		skb_put(skb, tailroom);

	if (iod->format == IPC_FMT) {
		struct timespec epoch;
		u8 *msg = (skb->data + headroom);
#if 0
		char tag[MIF_MAX_STR_LEN];
		snprintf(tag, MIF_MAX_STR_LEN, "%s: RIL2MIF", iod->mc->name);
		pr_ipc(1, tag, msg, (count > 20 ? 20 : count));
#endif
		getnstimeofday(&epoch);
		mif_time_log(iod->mc->msd, epoch, NULL, 0);
		mif_ipc_log(MIF_IPC_RL2AP, iod->mc->msd, msg, count);
	}

#if 0
	if (iod->format == IPC_RAMDUMP) {
		char tag[MIF_MAX_STR_LEN];
		snprintf(tag, MIF_MAX_STR_LEN, "%s: DUMP2MIF", iod->name);
		pr_ipc(1, tag, skb->data, (skb->len > 20 ? 20 : skb->len));
	}
#endif

	/* send data with sk_buff, link device will put sk_buff
	 * into the specific sk_buff_q and run work-q to send data
	 */
	skbpriv(skb)->iod = iod;
	skbpriv(skb)->ld = ld;

	ret = ld->send(ld, iod, skb);
	if (ret < 0) {
		mif_info("%s->%s: ERR! ld->send fail (err %d, tx_bytes %d)\n",
			iod->name, ld->name, ret, tx_bytes);
		return ret;
	}

	if (ret != tx_bytes) {
		mif_info("%s->%s: WARNING! ret %d != tx_bytes %d (count %d)\n",
			iod->name, ld->name, ret, tx_bytes, count);
	}

	return count;
}

static ssize_t misc_read(struct file *filp, char *buf, size_t count,
			loff_t *fpos)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct sk_buff_head *rxq = &iod->sk_rx_q;
	struct sk_buff *skb;
	int copied = 0;

	if (skb_queue_empty(rxq)) {
		mif_info("%s: ERR! no data in rxq\n", iod->name);
		return 0;
	}

	skb = skb_dequeue(rxq);

	if (iod->format == IPC_FMT) {
		struct timespec epoch;
#if 0
		char tag[MIF_MAX_STR_LEN];
		snprintf(tag, MIF_MAX_STR_LEN, "%s: MIF2RIL", iod->mc->name);
		pr_ipc(0, tag, skb->data, (skb->len > 20 ? 20 : skb->len));
#endif
		getnstimeofday(&epoch);
		mif_time_log(iod->mc->msd, epoch, NULL, 0);
		mif_ipc_log(MIF_IPC_AP2RL, iod->mc->msd, skb->data, skb->len);
	}

#if 0
	if (iod->format == IPC_RAMDUMP) {
		char tag[MIF_MAX_STR_LEN];
		snprintf(tag, MIF_MAX_STR_LEN, "%s: MIF2DUMP", iod->name);
		pr_ipc(1, tag, skb->data, (skb->len > 20 ? 20 : skb->len));
	}
#endif

	copied = skb->len > count ? count : skb->len;

	if (copy_to_user(buf, skb->data, copied)) {
		mif_err("%s: ERR! copy_to_user fail\n", iod->name);
		dev_kfree_skb_any(skb);
		return -EFAULT;
	}

	mif_debug("%s: data:%d copied:%d qlen:%d\n",
		iod->name, skb->len, copied, rxq->qlen);

	if (skb->len > count) {
		skb_pull(skb, count);
		skb_queue_head(rxq, skb);
	} else {
		dev_kfree_skb_any(skb);
	}

	return copied;
}

static const struct file_operations misc_io_fops = {
	.owner = THIS_MODULE,
	.open = misc_open,
	.release = misc_release,
	.poll = misc_poll,
	.unlocked_ioctl = misc_ioctl,
	.write = misc_write,
	.read = misc_read,
};

static int vnet_open(struct net_device *ndev)
{
	struct vnet *vnet = netdev_priv(ndev);

	mif_err("%s\n", vnet->iod->name);

	netif_start_queue(ndev);
	atomic_inc(&vnet->iod->opened);
	return 0;
}

static int vnet_stop(struct net_device *ndev)
{
	struct vnet *vnet = netdev_priv(ndev);

	mif_err("%s\n", vnet->iod->name);

	atomic_dec(&vnet->iod->opened);
	netif_stop_queue(ndev);
	skb_queue_purge(&vnet->iod->sk_rx_q);
	return 0;
}

static int vnet_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct vnet *vnet = netdev_priv(ndev);
	struct io_device *iod = vnet->iod;
	struct link_device *ld = get_current_link(iod);
	struct sk_buff *skb_new;
	int ret;
	unsigned headroom;
	unsigned tailroom;
	size_t count;
	size_t tx_bytes;
	struct iphdr *ip_header = NULL;
	u8 *buff;
	u8 cfg;

	/* When use `handover' with Network Bridge,
	 * user -> bridge device(rmnet0) -> real rmnet(xxxx_rmnet0) -> here.
	 * bridge device is ethernet device unlike xxxx_rmnet(net device).
	 * We remove the an ethernet header of skb before using skb->len,
	 * because bridge device added an ethernet header to skb.
	 */
	if (iod->use_handover) {
		if (iod->id >= PS_DATA_CH_0 && iod->id <= PS_DATA_CH_LAST)
			skb_pull(skb, sizeof(struct ethhdr));
	}

	count = skb->len;

	cfg = sipc5_build_config(iod, ld, count);

	headroom = sipc5_get_hdr_len(&cfg);

	if (ld->aligned)
		tailroom = sipc5_calc_padding_size(headroom + count);
	else
		tailroom = 0;

	tx_bytes = headroom + count + tailroom;

	if (skb_headroom(skb) < headroom || skb_tailroom(skb) < tailroom) {
		mif_debug("%s: skb_copy_expand needed\n", iod->name);
		skb_new = skb_copy_expand(skb, headroom, tailroom, GFP_ATOMIC);
		/* skb_copy_expand success or not, free old skb from caller */
		dev_kfree_skb_any(skb);
		if (!skb_new) {
			mif_info("%s: ERR! skb_copy_expand fail\n", iod->name);
			return NETDEV_TX_BUSY;
		}
	} else {
		skb_new = skb;
	}

	/* Build SIPC5 link header*/
	buff = skb_push(skb_new, headroom);
	sipc5_build_header(iod, ld, buff, cfg, 0, count);

	/* IP loop-back */
	ip_header = (struct iphdr *)skb->data;
	if (iod->msd->loopback_ipaddr &&
		ip_header->daddr == iod->msd->loopback_ipaddr) {
		swap(ip_header->saddr, ip_header->daddr);
		buff[SIPC5_CH_ID_OFFSET] = DATA_LOOPBACK_CHANNEL;
	}

	if (tailroom)
		skb_put(skb_new, tailroom);

	skbpriv(skb_new)->iod = iod;
	skbpriv(skb_new)->ld = ld;

	ret = ld->send(ld, iod, skb_new);
	if (ret < 0) {
		netif_stop_queue(ndev);
		mif_info("%s->%s: ERR! ld->send fail (err %d, tx_bytes %d)\n",
			iod->name, ld->name, ret, tx_bytes);
		return NETDEV_TX_BUSY;
	}

	if (ret != tx_bytes) {
		mif_info("%s->%s: WARNING! ret %d != tx_bytes %d (count %d)\n",
			iod->name, ld->name, ret, tx_bytes, count);
	}

	ndev->stats.tx_packets++;
	ndev->stats.tx_bytes += count;

	return NETDEV_TX_OK;
}

static struct net_device_ops vnet_ops = {
	.ndo_open = vnet_open,
	.ndo_stop = vnet_stop,
	.ndo_start_xmit = vnet_xmit,
};

static void vnet_setup(struct net_device *ndev)
{
	ndev->netdev_ops = &vnet_ops;
	ndev->type = ARPHRD_PPP;
	ndev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
	ndev->addr_len = 0;
	ndev->hard_header_len = 0;
	ndev->tx_queue_len = 1000;
	ndev->mtu = ETH_DATA_LEN;
	ndev->watchdog_timeo = 5 * HZ;
}

static void vnet_setup_ether(struct net_device *ndev)
{
	ndev->netdev_ops = &vnet_ops;
	ndev->type = ARPHRD_ETHER;
	ndev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST | IFF_SLAVE;
	ndev->addr_len = ETH_ALEN;
	random_ether_addr(ndev->dev_addr);
	ndev->hard_header_len = 0;
	ndev->tx_queue_len = 1000;
	ndev->mtu = ETH_DATA_LEN;
	ndev->watchdog_timeo = 5 * HZ;
}

int sipc5_init_io_device(struct io_device *iod)
{
	int ret = 0;
	struct vnet *vnet;

	/* Get modem state from modem control device */
	iod->modem_state_changed = io_dev_modem_state_changed;

	iod->sim_state_changed = io_dev_sim_state_changed;

	/* Get data from link device */
	mif_debug("%s: SIPC version = %d\n", iod->name, iod->ipc_version);
	iod->recv = io_dev_recv_data_from_link_dev;
	iod->recv_skb = io_dev_recv_skb_from_link_dev;

	/* Register misc or net device */
	switch (iod->io_typ) {
	case IODEV_MISC:
		init_waitqueue_head(&iod->wq);
		skb_queue_head_init(&iod->sk_rx_q);

		iod->miscdev.minor = MISC_DYNAMIC_MINOR;
		iod->miscdev.name = iod->name;
		iod->miscdev.fops = &misc_io_fops;

		ret = misc_register(&iod->miscdev);
		if (ret)
			mif_info("%s: ERR! misc_register failed\n", iod->name);

		break;

	case IODEV_NET:
		skb_queue_head_init(&iod->sk_rx_q);
		if (iod->use_handover)
			iod->ndev = alloc_netdev(0, iod->name,
						vnet_setup_ether);
		else
			iod->ndev = alloc_netdev(0, iod->name, vnet_setup);

		if (!iod->ndev) {
			mif_info("%s: ERR! alloc_netdev fail\n", iod->name);
			return -ENOMEM;
		}

		ret = register_netdev(iod->ndev);
		if (ret) {
			mif_info("%s: ERR! register_netdev fail\n", iod->name);
			free_netdev(iod->ndev);
		}

		mif_debug("iod 0x%p\n", iod);
		vnet = netdev_priv(iod->ndev);
		mif_debug("vnet 0x%p\n", vnet);
		vnet->iod = iod;

		break;

	case IODEV_DUMMY:
		skb_queue_head_init(&iod->sk_rx_q);

		iod->miscdev.minor = MISC_DYNAMIC_MINOR;
		iod->miscdev.name = iod->name;
		iod->miscdev.fops = &misc_io_fops;

		ret = misc_register(&iod->miscdev);
		if (ret)
			mif_info("%s: ERR! misc_register fail\n", iod->name);

		ret = device_create_file(iod->miscdev.this_device,
					&attr_waketime);
		if (ret)
			mif_info("%s: ERR! device_create_file fail\n",
				iod->name);

		ret = device_create_file(iod->miscdev.this_device,
				&attr_loopback);
		if (ret)
			mif_err("failed to create `loopback file' : %s\n",
					iod->name);

		ret = device_create_file(iod->miscdev.this_device,
				&attr_txlink);
		if (ret)
			mif_err("failed to create `txlink file' : %s\n",
					iod->name);
		break;

	default:
		mif_info("%s: ERR! wrong io_type %d\n", iod->name, iod->io_typ);
		return -EINVAL;
	}

	return ret;
}

