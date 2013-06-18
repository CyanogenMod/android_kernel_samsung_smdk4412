/* /linux/drivers/misc/modem_if/sipc5_io_device.c
 *
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

#include <linux/platform_data/modem.h>
#ifdef CONFIG_LINK_DEVICE_C2C
#include <linux/platform_data/c2c.h>
#endif
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
		*p += sprintf(*p, "%s: %s\n", iod->name, ld->name);
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

/**
 * rx_check_frame_cfg
 * @cfg: configuration field of a link layer header
 * @frm: pointer to the sipc5_frame_data buffer
 *
 * 1) Checks whether or not an extended field exists
 * 2) Calculates the length of a link layer header
 *
 * Returns the size of a link layer header
 *
 * Must be invoked only when the configuration field of the link layer header
 * is validated with sipc5_start_valid() function
 */
static int rx_check_frame_cfg(u8 cfg, struct sipc5_frame_data *frm)
{
	frm->config = cfg;

	if (likely(cfg & SIPC5_PADDING_EXIST))
		frm->padding = true;

	if (unlikely(cfg & SIPC5_EXT_FIELD_EXIST)) {
		if (cfg & SIPC5_CTL_FIELD_EXIST) {
			frm->ctl_fld = true;
			frm->hdr_len = SIPC5_HEADER_SIZE_WITH_CTL_FLD;
		} else {
			frm->ext_len = true;
			frm->hdr_len = SIPC5_HEADER_SIZE_WITH_EXT_LEN;
		}
	} else {
		frm->hdr_len = SIPC5_MIN_HEADER_SIZE;
	}

	return frm->hdr_len;
}

/**
 * rx_build_meta_data
 * @ld: pointer to the link device
 * @frm: pointer to the sipc5_frame_data buffer
 *
 * Fills each field of sipc5_frame_data from a link layer header
 * 1) Extracts the channel ID
 * 2) Calculates the length of a link layer frame
 * 3) Extracts a control field if exists
 * 4) Calculates the length of an IPC message packet in the link layer frame
 *
 */
static void rx_build_meta_data(struct link_device *ld,
		struct sipc5_frame_data *frm)
{
	u16 *sz16 = (u16 *)(frm->hdr + SIPC5_LEN_OFFSET);
	u32 *sz32 = (u32 *)(frm->hdr + SIPC5_LEN_OFFSET);

	frm->ch_id = frm->hdr[SIPC5_CH_ID_OFFSET];

	if (unlikely(frm->ext_len))
		frm->len = *sz32;
	else
		frm->len = *sz16;

	if (unlikely(frm->ctl_fld))
		frm->control = frm->hdr[SIPC5_CTL_OFFSET];

	frm->data_len = frm->len - frm->hdr_len;

	mif_debug("%s: FRM ch:%d len:%d ctl:%02X data.len:%d\n",
		ld->name, frm->ch_id, frm->len, frm->control, frm->data_len);
}

/**
 * tx_build_link_header
 * @frm: pointer to the sipc5_frame_data buffer
 * @iod: pointer to the IO device
 * @ld: pointer to the link device
 * @count: length of the data to be transmitted
 *
 * Builds the meta data for an SIPC5 frame and the link layer header of it
 * Returns the link layer header length for an SIPC5 frame or 0 for other frame
 */
static unsigned tx_build_link_header(struct sipc5_frame_data *frm,
		struct io_device *iod, struct link_device *ld, ssize_t count)
{
	u8 *buff = frm->hdr;
	u16 *sz16 = (u16 *)(buff + SIPC5_LEN_OFFSET);
	u32 *sz32 = (u32 *)(buff + SIPC5_LEN_OFFSET);

	memset(frm, 0, sizeof(struct sipc5_frame_data));

	if (iod->format == IPC_CMD ||
	    iod->format == IPC_BOOT ||
	    iod->format == IPC_RAMDUMP) {
		frm->len = count;
		return 0;
	}

	frm->config = SIPC5_START_MASK;

	if (iod->format == IPC_FMT && count > 2048) {
		frm->ctl_fld = true;
		frm->config |= SIPC5_EXT_FIELD_EXIST;
		frm->config |= SIPC5_CTL_FIELD_EXIST;
	}

	if (iod->id >= SIPC5_CH_ID_RFS_0 && count > 0xFFFF) {
		frm->ext_len = true;
		frm->config |= SIPC5_EXT_FIELD_EXIST;
	}

	if (ld->aligned)
		frm->config |= SIPC5_PADDING_EXIST;

	frm->ch_id = iod->id;

	frm->hdr_len = sipc5_get_hdr_len(frm->config);
	frm->data_len = count;
	frm->len = frm->hdr_len + frm->data_len;

	buff[SIPC5_CONFIG_OFFSET] = frm->config;
	buff[SIPC5_CH_ID_OFFSET] = frm->ch_id;

	if (unlikely(frm->ext_len))
		*sz32 = (u32)frm->len;
	else
		*sz16 = (u16)frm->len;

	if (unlikely(frm->ctl_fld))
		buff[SIPC5_CTL_OFFSET] = frm->control;

	return frm->hdr_len;
}

static inline int enqueue_skb_to_iod(struct sk_buff *skb, struct io_device *iod)
{
	struct sk_buff_head *rxq = &iod->sk_rx_q;
	struct sk_buff *victim;

	skb_queue_tail(rxq, skb);
	if (unlikely(rxq->qlen > MAX_IOD_RXQ_LEN)) {
		mif_info("%s: %s application may be dead (rxq->qlen %d > %d)\n",
			iod->name, iod->app ? iod->app : "corresponding",
			rxq->qlen, MAX_IOD_RXQ_LEN);
		victim = skb_dequeue(rxq);
		if (victim)
			dev_kfree_skb_any(victim);
		return -ENOSPC;
	} else {
		mif_debug("%s: rxq->qlen = %d\n", iod->name, rxq->qlen);
		return 0;
	}
}

static int rx_fmt_frame(struct sk_buff *skb)
{
	struct io_device *iod = skbpriv(skb)->iod;
	struct link_device *ld = skbpriv(skb)->ld;
	struct sipc_fmt_hdr *fh;
	struct sk_buff *rx_skb;
	u8 ctrl = skbpriv(skb)->control;
	unsigned id = ctrl & 0x7F;

	if (iod->skb[id] == NULL) {
		/*
		** There has been no multiple frame with this ID.
		*/
		if ((ctrl & 0x80) == 0) {
			/*
			** It is a single frame because the "more" bit is 0.
			*/
			enqueue_skb_to_iod(skb, iod);
			wake_up(&iod->wq);
			return 0;
		}

		/*
		** The start of multiple frames
		*/
		fh = (struct sipc_fmt_hdr *)skb->data;
		mif_debug("%s: start multi-frame (ID:%d len:%d)\n",
			iod->name, id, fh->len);

		rx_skb = rx_alloc_skb(fh->len, iod, ld);
		if (!rx_skb) {
			mif_info("%s: ERR! rx_alloc_skb fail\n", iod->name);
			return -ENOMEM;
		}

		iod->skb[id] = rx_skb;
	} else {
		rx_skb = iod->skb[id];
	}

	/*
	** Start multi-frame processing
	*/
	memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);
	dev_kfree_skb_any(skb);

	if (ctrl & 0x80) {
		/* The last frame has not arrived yet. */
		mif_debug("%s: recv multi-frame (ID:%d rcvd:%d)\n",
			iod->name, id, rx_skb->len);
	} else {
		/* It is the last frame because the "more" bit is 0. */
		mif_debug("%s: end multi-frame (ID:%d rcvd:%d)\n",
			iod->name, id, rx_skb->len);
		enqueue_skb_to_iod(rx_skb, iod);
		iod->skb[id] = NULL;
		wake_up(&iod->wq);
	}

	return 0;
}

static int rx_raw_misc(struct sk_buff *skb)
{
	struct io_device *iod = skbpriv(skb)->iod; /* same with real_iod */

	enqueue_skb_to_iod(skb, iod);
	wake_up(&iod->wq);

	return 0;
}

static int rx_multi_pdp(struct sk_buff *skb)
{
	struct io_device *iod = skbpriv(skb)->iod; /* same with real_iod */
	struct net_device *ndev;
	struct iphdr *iphdr;
	struct ethhdr *ehdr;
	int ret;
	const char source[ETH_ALEN] = SOURCE_MAC_ADDR;

	ndev = iod->ndev;
	if (!ndev) {
		mif_info("%s: ERR! no iod->ndev\n", iod->name);
		return -ENODEV;
	}

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
		skb_push(skb, sizeof(struct ethhdr));
		ehdr = (void *)skb->data;
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

	if (ret != NET_RX_SUCCESS)
		mif_info("%s: ERR! netif_rx fail (err %d)\n", iod->name, ret);

	return ret;
}

static int rx_loopback(struct sk_buff *skb)
{
	struct io_device *iod = skbpriv(skb)->iod;
	struct link_device *ld = get_current_link(iod);
	struct sipc5_frame_data frm;
	unsigned headroom;
	unsigned tailroom = 0;
	int ret;

	headroom = tx_build_link_header(&frm, iod, ld, skb->len);

	if (ld->aligned)
		tailroom = sipc5_calc_padding_size(headroom + skb->len);

	/* We need not to expand skb in here. dev_alloc_skb (in rx_alloc_skb)
	 * already alloc 32bytes padding in headroom. 32bytes are enough.
	 */

	/* store IPC link header to start of skb
	 * this is skb_push not skb_put. different with misc_write.
	 */
	memcpy(skb_push(skb, headroom), frm.hdr, headroom);

	/* store padding */
	if (tailroom)
		skb_put(skb, tailroom);

	/* forward */
	ret = ld->send(ld, iod, skb);
	if (ret < 0)
		mif_err("%s->%s: ld->send fail: %d\n", iod->name,
				ld->name, ret);
	return ret;
}

static int rx_netif_flow_ctrl(struct link_device *ld, struct sk_buff *skb)
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

static int rx_demux(struct link_device *ld, struct sk_buff *skb)
{
	struct io_device *iod = NULL;
	char *link = ld->name;
	u8 ch = skbpriv(skb)->ch_id;

	if (unlikely(ch == 0)) {
		mif_info("%s: ERR! invalid ch# %d\n", link, ch);
		return -ENODEV;
	}

	if (unlikely(ch == SIPC5_CH_ID_FLOW_CTRL))
		return rx_netif_flow_ctrl(ld, skb);

	/* IP loopback */
	if (ch == DATA_LOOPBACK_CHANNEL && ld->msd->loopback_ipaddr)
		ch = RMNET0_CH_ID;

	iod = link_get_iod_with_channel(ld, ch);
	if (unlikely(!iod)) {
		mif_info("%s: ERR! no iod for ch# %d\n", link, ch);
		return -ENODEV;
	}

	skbpriv(skb)->ld = ld;
	skbpriv(skb)->iod = iod;
	skbpriv(skb)->real_iod = iod;

	/* don't care about CP2AP_LOOPBACK_CHANNEL is opened */
	if (unlikely(iod->id == CP2AP_LOOPBACK_CHANNEL))
		return rx_loopback(skb);

	if (atomic_read(&iod->opened) <= 0) {
		mif_info("%s: ERR! %s is not opened\n", link, iod->name);
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

/* Check and store link layer header, then alloc an skb */
static int rx_header_from_serial(struct io_device *iod, struct link_device *ld,
		u8 *buff, unsigned size, struct sipc5_frame_data *frm)
{
	char *link = ld->name;
	struct sk_buff *skb;
	int len;
	u8 cfg = buff[0];

	mif_debug("%s: size %d\n", link, size);

	if (!frm->config) {
		if (unlikely(!sipc5_start_valid(cfg))) {
			mif_info("%s: ERR! wrong start (0x%02x)\n", link, cfg);
			return -EBADMSG;
		}
		rx_check_frame_cfg(cfg, frm);

		/* Copy the link layer header to the header buffer */
		len = min(frm->hdr_len, size);
		memcpy(frm->hdr, buff, len);
	} else {
		/* Copy the link layer header to the header buffer */
		len = min((frm->hdr_len - frm->hdr_rcvd), size);
		memcpy((frm->hdr + frm->hdr_rcvd), buff, len);
	}

	frm->hdr_rcvd += len;

	mif_debug("%s: FRM hdr_len:%d, hdr_rcvd:%d\n",
		link, frm->hdr_len, frm->hdr_rcvd);

	if (frm->hdr_rcvd >= frm->hdr_len) {
		rx_build_meta_data(ld, frm);
		skb = rx_alloc_skb(frm->data_len, iod, ld);
		fragdata(iod, ld)->skb_recv = skb;
		skbpriv(skb)->ch_id = frm->ch_id;
		skbpriv(skb)->control = frm->control;
	}

	return len;
}

/* copy data to skb */
static int rx_payload_from_serial(struct io_device *iod, struct link_device *ld,
		u8 *buff, unsigned size, struct sipc5_frame_data *frm)
{
	struct sk_buff *skb = fragdata(iod, ld)->skb_recv;
	char *link = ld->name;
	unsigned rest = frm->data_len - frm->data_rcvd;
	unsigned len;

	/* rest == (frm->data_len - frm->data_rcvd) == tailroom of skb */
	rest = frm->data_len - frm->data_rcvd;
	mif_debug("%s: FRM data.len:%d data.rcvd:%d rest:%d size:%d\n",
		link, frm->data_len, frm->data_rcvd, rest, size);

	/* If there is no skb, data must be dropped. */
	len = min(rest, size);
	if (skb)
		memcpy(skb_put(skb, len), buff, len);

	frm->data_rcvd += len;

	mif_debug("%s: FRM data_len:%d, data_rcvd:%d\n",
		link, frm->data_len, frm->data_rcvd);

	return len;
}

static int rx_frame_from_serial(struct io_device *iod, struct link_device *ld,
		const char *data, unsigned size)
{
	struct sipc5_frame_data *frm = &fragdata(iod, ld)->f_data;
	struct sk_buff *skb;
	char *link = ld->name;
	u8 *buff = (u8 *)data;
	int rest = (int)size;
	int err = 0;
	int done = 0;

	mif_debug("%s: size = %d\n", link, size);

	if (frm->hdr_rcvd >= frm->hdr_len && frm->data_rcvd < frm->data_len) {
		/*
		** There is an skb that is waiting for more SIPC5 data.
		** In this case, rx_header_from_serial() must be skipped.
		*/
		mif_debug("%s: FRM data.len:%d data.rcvd:%d -> recv_data\n",
			link, frm->data_len, frm->data_rcvd);
		goto recv_data;
	}

next_frame:
	/* Receive and analyze header, then prepare an akb */
	err = done = rx_header_from_serial(iod, ld, buff, rest, frm);
	if (err < 0)
		goto err_exit;

	buff += done;
	rest -= done;
	mif_debug("%s: rx_header() -> done:%d rest:%d\n", link, done, rest);
	if (rest < 0)
		goto err_range;

	if (rest == 0)
		return size;

recv_data:
	err = 0;

	mif_debug("%s: done:%d rest:%d -> rx_payload()\n", link, done, rest);

	done = rx_payload_from_serial(iod, ld, buff, rest, frm);
	buff += done;
	rest -= done;

	mif_debug("%s: rx_payload() -> done:%d rest:%d\n", link, done, rest);

	if (rest == 0 && frm->data_rcvd < frm->data_len) {
		/*
		  Data is being received and more data will come within the next
		  frame from the link device.
		*/
		return size;
	}

	/* At this point, one complete link layer frame has been received. */

	/* A padding size is applied to access the next IPC frame. */
	if (frm->padding) {
		done = sipc5_calc_padding_size(frm->len);
		if (done > rest) {
			mif_info("%s: ERR! padding %d > rest %d\n",
				link, done, rest);
			goto err_exit;
		}

		buff += done;
		rest -= done;

		mif_debug("%s: padding:%d -> rest:%d\n", link, done, rest);

		if (rest < 0)
			goto err_range;

	}

	skb = fragdata(iod, ld)->skb_recv;
	if (likely(skb)) {
		mif_debug("%s: len:%d -> rx_demux()\n", link, skb->len);
		err = rx_demux(ld, skb);
		if (err < 0)
			dev_kfree_skb_any(skb);
	} else {
		mif_debug("%s: len:%d -> drop\n", link, skb->len);
	}

	/* initialize the skb_recv and the frame_data buffer */
	fragdata(iod, ld)->skb_recv = NULL;
	memset(frm, 0, sizeof(struct sipc5_frame_data));

	if (rest > 0)
		goto next_frame;

	if (rest <= 0)
		return size;

err_exit:
	if (fragdata(iod, ld)->skb_recv &&
	    frm->hdr_rcvd >= frm->hdr_len && frm->data_rcvd >= frm->data_len) {
		dev_kfree_skb_any(fragdata(iod, ld)->skb_recv);
		memset(frm, 0, sizeof(struct sipc5_frame_data));
		fragdata(iod, ld)->skb_recv = NULL;
		mif_info("%s: ERR! clear frag\n", link);
	}
	return err;

err_range:
	mif_info("%s: ERR! size:%d vs. rest:%d\n", link, size, rest);
	return size;
}

/**
 * rx_header_from_mem
 * @ld: pointer to the link device
 * @buff: pointer to the frame
 * @rest: size of the frame
 * @frm: pointer to the sipc5_frame_data buffer
 *
 * 1) Verifies a link layer header configuration of a frame
 * 2) Stores the link layer header to the header buffer
 * 3) Builds and stores the meta data of the frame into a meta data buffer
 * 4) Verifies the length of the frame
 *
 * Returns SIPC5 header length
 */
static int rx_header_from_mem(struct link_device *ld, u8 *buff, unsigned rest,
		struct sipc5_frame_data *frm)
{
	char *link = ld->name;
	u8 cfg = buff[0];

	/* Verify link layer header configuration */
	if (unlikely(!sipc5_start_valid(cfg))) {
		mif_info("%s: ERR! wrong start (0x%02x)\n", link, cfg);
		return -EBADMSG;
	}
	rx_check_frame_cfg(cfg, frm);

	/* Store the link layer header to the header buffer */
	memcpy(frm->hdr, buff, frm->hdr_len);
	frm->hdr_rcvd = frm->hdr_len;

	/* Build and store the meta data of this frame */
	rx_build_meta_data(ld, frm);

	/* Verify frame length */
	if (unlikely(frm->len > rest)) {
		mif_info("%s: ERR! frame length %d > rest %d\n",
			link, frm->len, rest);
		return -EBADMSG;
	}

	return frm->hdr_rcvd;
}

/* copy data to skb */
static int rx_payload_from_mem(struct sk_buff *skb, u8 *buff, unsigned len)
{
	/* If there is no skb, data must be dropped. */
	if (skb)
		memcpy(skb_put(skb, len), buff, len);
	return len;
}

static int rx_frame_from_mem(struct io_device *iod, struct link_device *ld,
		const char *data, unsigned size)
{
	struct sipc5_frame_data *frm = &fragdata(iod, ld)->f_data;
	struct sk_buff *skb;
	char *link = ld->name;
	u8 *buff = (u8 *)data;
	int rest = (int)size;
	int len;
	int done;

	mif_debug("%s: size = %d\n", link, size);

	while (rest > 0) {
		/* Initialize the frame data buffer */
		memset(frm, 0, sizeof(struct sipc5_frame_data));
		skb = NULL;

		/* Receive and analyze link layer header */
		done = rx_header_from_mem(ld, buff, rest, frm);
		if (unlikely(done < 0))
			return -EBADMSG;

		/* Verify rest size */
		rest -= done;
		if (rest < 0) {
			mif_info("%s: ERR! rx_header -> rest %d\n", link, rest);
			return -ERANGE;
		}

		/* Move buff pointer to the payload */
		buff += done;

		/* Prepare an akb */
		len = frm->data_len;
		skb = rx_alloc_skb(len, iod, ld);

		/* Store channel ID and control fields to the CB of the skb */
		skbpriv(skb)->ch_id = frm->ch_id;
		skbpriv(skb)->control = frm->control;

		/* Receive payload */
		mif_debug("%s: done:%d rest:%d len:%d -> rx_payload()\n",
			link, done, rest, len);
		done = rx_payload_from_mem(skb, buff, len);
		rest -= done;
		if (rest < 0) {
			mif_info("%s: ERR! rx_payload() -> rest %d\n",
				link, rest);
			if (skb)
				dev_kfree_skb_any(skb);
			return -ERANGE;
		}
		buff += done;

		/* A padding size is applied to access the next IPC frame. */
		if (frm->padding) {
			done = sipc5_calc_padding_size(frm->len);
			if (done > rest) {
				mif_info("%s: ERR! padding %d > rest %d\n",
					link, done, rest);
				if (skb)
					dev_kfree_skb_any(skb);
				return -ERANGE;
			}
			buff += done;
			rest -= done;
		}

		if (likely(skb)) {
			mif_debug("%s: len:%d -> rx_demux()\n", link, skb->len);
			if (rx_demux(ld, skb) < 0)
				dev_kfree_skb_any(skb);
		} else {
			mif_debug("%s: len:%d -> drop\n", link, skb->len);
		}
	}

	return 0;
}

/* called from link device when a packet arrives for this io device */
static int io_dev_recv_data_from_link_dev(struct io_device *iod,
		struct link_device *ld, const char *data, unsigned int len)
{
	struct sk_buff *skb;
	char *link = ld->name;
	int err;

	if (!data) {
		mif_info("%s: ERR! !data\n", link);
		return -EINVAL;
	}

	if (len <= 0) {
		mif_info("%s: ERR! len %d <= 0\n", link, len);
		return -EINVAL;
	}

	switch (iod->format) {
	case IPC_FMT:
	case IPC_RAW:
	case IPC_RFS:
	case IPC_MULTI_RAW:
		if (iod->waketime)
			wake_lock_timeout(&iod->wakelock, iod->waketime);

		if (ld->link_type == LINKDEV_DPRAM && ld->aligned)
			err = rx_frame_from_mem(iod, ld, data, len);
		else
			err = rx_frame_from_serial(iod, ld, data, len);

		if (err < 0)
			mif_info("%s: ERR! rx_frame_from_link fail (err %d)\n",
				link, err);

		return err;

	case IPC_CMD:
	case IPC_BOOT:
	case IPC_RAMDUMP:
		/* save packet to sk_buff */
		skb = rx_alloc_skb(len, iod, ld);
		if (!skb) {
			mif_info("%s: ERR! rx_alloc_skb fail\n", link);
			return -ENOMEM;
		}

		mif_debug("%s: len:%d -> iod:%s\n", link, len, iod->name);

		memcpy(skb_put(skb, len), data, len);

		enqueue_skb_to_iod(skb, iod);
		wake_up(&iod->wq);

		return len;

	default:
		mif_info("%s: ERR! unknown format %d\n", link, iod->format);
		return -EINVAL;
	}
}

static int rx_frame_from_skb(struct io_device *iod, struct link_device *ld,
		struct sk_buff *skb)
{
	struct sipc5_frame_data *frm = &fragdata(iod, ld)->f_data;
	u8 cfg = skb->data[0];

	/* Initialize the frame data buffer */
	memset(frm, 0, sizeof(struct sipc5_frame_data));

	/*
	** The start of a link layer header has already been checked in the
	** link device.
	*/

	/* Analyze the configuration of the link layer header */
	rx_check_frame_cfg(cfg, frm);

	/* Store the link layer header to the header buffer */
	memcpy(frm->hdr, skb->data, frm->hdr_len);
	frm->hdr_rcvd = frm->hdr_len;

	/* Build and store the meta data of this frame */
	rx_build_meta_data(ld, frm);

	/*
	** The length of the frame has already been checked in the link device.
	*/

	/* Trim the link layer header off the frame */
	skb_pull(skb, frm->hdr_len);

	/* Store channel ID and control fields to the CB of the skb */
	skbpriv(skb)->ch_id = frm->ch_id;
	skbpriv(skb)->control = frm->control;

	/* Demux the frame */
	if (rx_demux(ld, skb) < 0) {
		mif_info("%s: ERR! rx_demux fail\n", ld->name);
		return -EINVAL;
	}

	return 0;
}

/* called from link device when a packet arrives for this io device */
static int io_dev_recv_skb_from_link_dev(struct io_device *iod,
		struct link_device *ld, struct sk_buff *skb)
{
	char *link = ld->name;
	enum dev_format dev = iod->format;
	int err;

	switch (dev) {
	case IPC_FMT:
	case IPC_RAW:
	case IPC_RFS:
	case IPC_MULTI_RAW:
		if (iod->waketime)
			wake_lock_timeout(&iod->wakelock, iod->waketime);

		err = rx_frame_from_skb(iod, ld, skb);
		if (err < 0) {
			dev_kfree_skb_any(skb);
			mif_info("%s: ERR! rx_frame_from_skb fail (err %d)\n",
				link, err);
		}

		return err;

	default:
		mif_info("%s: ERR! unknown device %d\n", link, dev);
		return -EINVAL;
	}
}

/* inform the IO device that the modem is now online or offline or
 * crashing or whatever...
 */
static void io_dev_modem_state_changed(struct io_device *iod,
			enum modem_state state)
{
	mif_info("%s: %s state changed (state %d)\n",
		iod->name, iod->mc->name, state);

	iod->mc->phone_state = state;

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
	int ret;
	filp->private_data = (void *)iod;

	atomic_inc(&iod->opened);

	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld) && ld->init_comm) {
			ret = ld->init_comm(ld, iod);
			if (ret < 0) {
				mif_info("%s: init_comm fail(%d)\n",
					ld->name, ret);
				return ret;
			}
		}
	}

	mif_err("%s (opened %d)\n", iod->name, atomic_read(&iod->opened));

	return 0;
}

static int misc_release(struct inode *inode, struct file *filp)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct modem_shared *msd = iod->msd;
	struct link_device *ld;

	atomic_dec(&iod->opened);
	skb_queue_purge(&iod->sk_rx_q);

	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld) && ld->terminate_comm)
			ld->terminate_comm(ld, iod);
	}

	mif_err("%s (opened %d)\n", iod->name, atomic_read(&iod->opened));

	return 0;
}

static unsigned int misc_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct io_device *iod = (struct io_device *)filp->private_data;

	poll_wait(filp, &iod->wq, wait);

	if (!skb_queue_empty(&iod->sk_rx_q) &&
	    iod->mc->phone_state != STATE_OFFLINE) {
		return POLLIN | POLLRDNORM;
	} else if ((iod->mc->phone_state == STATE_CRASH_RESET) ||
		   (iod->mc->phone_state == STATE_CRASH_EXIT) ||
		   (iod->mc->phone_state == STATE_NV_REBUILDING) ||
		   (iod->mc->sim_state.changed)) {
		if (iod->format == IPC_RAW) {
			msleep(20);
			return 0;
		}
		return POLLHUP;
	} else {
		return 0;
	}
}

static long misc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int p_state;
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct link_device *ld = get_current_link(iod);
	char cpinfo_buf[530] = "CP Crash ";
	unsigned long size;
	int ret;

	switch (cmd) {
	case IOCTL_MODEM_ON:
		mif_info("%s: IOCTL_MODEM_ON\n", iod->name);
		return iod->mc->ops.modem_on(iod->mc);

	case IOCTL_MODEM_OFF:
		mif_info("%s: IOCTL_MODEM_OFF\n", iod->name);
		return iod->mc->ops.modem_off(iod->mc);

	case IOCTL_MODEM_RESET:
		mif_info("%s: IOCTL_MODEM_RESET\n", iod->name);
		return iod->mc->ops.modem_reset(iod->mc);

	case IOCTL_MODEM_BOOT_ON:
		mif_info("%s: IOCTL_MODEM_BOOT_ON\n", iod->name);
		return iod->mc->ops.modem_boot_on(iod->mc);

	case IOCTL_MODEM_BOOT_OFF:
		mif_info("%s: IOCTL_MODEM_BOOT_OFF\n", iod->name);
		return iod->mc->ops.modem_boot_off(iod->mc);

	case IOCTL_MODEM_BOOT_DONE:
		mif_err("%s: IOCTL_MODEM_BOOT_DONE\n", iod->name);
		if (iod->mc->ops.modem_boot_done)
			return iod->mc->ops.modem_boot_done(iod->mc);
		else
			return 0;

	case IOCTL_MODEM_STATUS:
		mif_debug("%s: IOCTL_MODEM_STATUS\n", iod->name);

		p_state = iod->mc->phone_state;
		if ((p_state == STATE_CRASH_RESET) ||
		    (p_state == STATE_CRASH_EXIT)) {
			mif_info("%s: IOCTL_MODEM_STATUS (state %d)\n",
				iod->name, p_state);
		} else if (iod->mc->sim_state.changed) {
			int s_state = iod->mc->sim_state.online ?
					STATE_SIM_ATTACH : STATE_SIM_DETACH;
			iod->mc->sim_state.changed = false;
			return s_state;
		} else if (p_state == STATE_NV_REBUILDING) {
			mif_info("%s: IOCTL_MODEM_STATUS (state %d)\n",
				iod->name, p_state);
			iod->mc->phone_state = STATE_ONLINE;
		}
		return p_state;

	case IOCTL_MODEM_PROTOCOL_SUSPEND:
		mif_debug("%s: IOCTL_MODEM_PROTOCOL_SUSPEND\n",
			iod->name);

		if (iod->format != IPC_MULTI_RAW)
			return -EINVAL;

		iodevs_for_each(iod->msd, iodev_netif_stop, 0);
		return 0;

	case IOCTL_MODEM_PROTOCOL_RESUME:
		mif_info("%s: IOCTL_MODEM_PROTOCOL_RESUME\n",
			iod->name);

		if (iod->format != IPC_MULTI_RAW)
			return -EINVAL;

		iodevs_for_each(iod->msd, iodev_netif_wake, 0);
		return 0;

	case IOCTL_MODEM_DUMP_START:
		mif_info("%s: IOCTL_MODEM_DUMP_START\n", iod->name);
		return ld->dump_start(ld, iod);

	case IOCTL_MODEM_DUMP_UPDATE:
		mif_debug("%s: IOCTL_MODEM_DUMP_UPDATE\n", iod->name);
		return ld->dump_update(ld, iod, arg);

	case IOCTL_MODEM_FORCE_CRASH_EXIT:
		mif_info("%s: IOCTL_MODEM_FORCE_CRASH_EXIT\n", iod->name);
		if (iod->mc->ops.modem_force_crash_exit)
			return iod->mc->ops.modem_force_crash_exit(iod->mc);
		return -EINVAL;

	case IOCTL_MODEM_CP_UPLOAD:
		mif_info("%s: IOCTL_MODEM_CP_UPLOAD\n", iod->name);
		if (copy_from_user(cpinfo_buf + strlen(cpinfo_buf),
			(void __user *)arg, MAX_CPINFO_SIZE) != 0)
			return -EFAULT;
		panic(cpinfo_buf);
		return 0;

	case IOCTL_MODEM_DUMP_RESET:
		mif_info("%s: IOCTL_MODEM_DUMP_RESET\n", iod->name);
		return iod->mc->ops.modem_dump_reset(iod->mc);

	case IOCTL_MIF_LOG_DUMP:
		iodevs_for_each(iod->msd, iodev_dump_status, 0);
		size = MAX_MIF_BUFF_SIZE;
		ret = copy_to_user((void __user *)arg, &size,
			sizeof(unsigned long));
		if (ret < 0)
			return -EFAULT;

		mif_dump_log(iod->mc->msd, iod);
		return 0;

	case IOCTL_MIF_DPRAM_DUMP:
#ifdef CONFIG_LINK_DEVICE_DPRAM
		if (iod->mc->mdm_data->link_types & LINKTYPE(LINKDEV_DPRAM)) {
			size = iod->mc->mdm_data->dpram_ctl->dp_size;
			ret = copy_to_user((void __user *)arg, &size,
				sizeof(unsigned long));
			if (ret < 0)
				return -EFAULT;
			mif_dump_dpram(iod);
			return 0;
		}
#endif
		return -EINVAL;

	default:
		 /* If you need to handle the ioctl for specific link device,
		  * then assign the link ioctl handler to ld->ioctl
		  * It will be call for specific link ioctl */
		if (ld->ioctl)
			return ld->ioctl(ld, iod, cmd, arg);

		mif_info("%s: ERR! cmd 0x%X not defined.\n", iod->name, cmd);
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
	int ret;
	unsigned headroom = 0;
	unsigned tailroom = 0;
	size_t tx_size;
	struct sipc5_frame_data frm;

	if (iod->format <= IPC_RFS && iod->id == 0)
		return -EINVAL;

	headroom = tx_build_link_header(&frm, iod, ld, count);

	if (ld->aligned)
		tailroom = sipc5_calc_padding_size(headroom + count);

	tx_size = headroom + count + tailroom;

	skb = alloc_skb(tx_size, GFP_KERNEL);
	if (!skb) {
		mif_info("%s: ERR! alloc_skb fail (tx_size:%d)\n",
			iod->name, tx_size);
		return -ENOMEM;
	}

	/* store IPC link header*/
	memcpy(skb_put(skb, headroom), frm.hdr, headroom);

	/* store IPC message */
	if (copy_from_user(skb_put(skb, count), data, count) != 0) {
		if (skb)
			dev_kfree_skb_any(skb);
		return -EFAULT;
	}

	if (iod->format == IPC_FMT) {
		struct timespec epoch;
		u8 *msg = (skb->data + headroom);
#if 0
		char str[MIF_MAX_STR_LEN];
		snprintf(str, MIF_MAX_STR_LEN, "%s: RL2MIF", iod->mc->name);
		pr_ipc(str, msg, (count > 16 ? 16 : count));
#endif
		getnstimeofday(&epoch);
		mif_time_log(iod->mc->msd, epoch, NULL, 0);
		mif_ipc_log(MIF_IPC_RL2AP, iod->mc->msd, msg, count);
	}

	/* store padding */
	if (tailroom)
		skb_put(skb, tailroom);

	/* send data with sk_buff, link device will put sk_buff
	 * into the specific sk_buff_q and run work-q to send data
	 */
	skbpriv(skb)->iod = iod;
	skbpriv(skb)->ld = ld;

	ret = ld->send(ld, iod, skb);
	if (ret < 0) {
		mif_info("%s: ERR! ld->send fail (err %d)\n", iod->name, ret);
		return ret;
	}

	if (ret != tx_size)
		mif_info("%s: wrong tx size (count:%d tx_size:%d ret:%d)\n",
			iod->name, count, tx_size, ret);

	return count;
}

static ssize_t misc_read(struct file *filp, char *buf, size_t count,
			loff_t *fpos)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct sk_buff_head *rxq = &iod->sk_rx_q;
	struct sk_buff *skb;
	int copied = 0;

	skb = skb_dequeue(rxq);
	if (!skb) {
		mif_info("%s: ERR! no data in rxq\n", iod->name);
		return 0;
	}

	if (iod->format == IPC_FMT) {
		struct timespec epoch;
#if 0
		char str[MIF_MAX_STR_LEN];
		snprintf(str, MIF_MAX_STR_LEN, "%s: MIF2RL", iod->mc->name);
		pr_ipc(str, skb->data, (skb->len > 16 ? 16 : skb->len));
#endif
		getnstimeofday(&epoch);
		mif_time_log(iod->mc->msd, epoch, NULL, 0);
		mif_ipc_log(MIF_IPC_AP2RL, iod->mc->msd, skb->data, skb->len);
	}

	copied = skb->len > count ? count : skb->len;

	if (copy_to_user(buf, skb->data, copied)) {
		mif_info("%s: ERR! copy_to_user fail\n", iod->name);
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

#ifdef CONFIG_LINK_DEVICE_C2C
static int misc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int r = 0;
	unsigned long size = 0;
	unsigned long pfn = 0;
	unsigned long offset = 0;
	struct io_device *iod = (struct io_device *)filp->private_data;

	if (!vma)
		return -EFAULT;

	size = vma->vm_end - vma->vm_start;
	offset = vma->vm_pgoff << PAGE_SHIFT;
	if (offset + size > (C2C_CP_RGN_SIZE + C2C_SH_RGN_SIZE)) {
		mif_info("ERR: offset + size > C2C_CP_RGN_SIZE\n");
		return -EINVAL;
	}

	/* Set the noncacheable property to the region */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED | VM_IO;

	pfn = __phys_to_pfn(C2C_CP_RGN_ADDR + offset);
	r = remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot);
	if (r) {
		mif_info("ERR: Failed in remap_pfn_range()!!!\n");
		return -EAGAIN;
	}

	mif_info("%s: VA = 0x%08lx, offset = 0x%lx, size = %lu\n",
		iod->name, vma->vm_start, offset, size);

	return 0;
}
#endif

static const struct file_operations misc_io_fops = {
	.owner = THIS_MODULE,
	.open = misc_open,
	.release = misc_release,
	.poll = misc_poll,
	.unlocked_ioctl = misc_ioctl,
	.write = misc_write,
	.read = misc_read,
#ifdef CONFIG_LINK_DEVICE_C2C
	.mmap = misc_mmap,
#endif
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
	unsigned headroom = 0;
	unsigned tailroom = 0;
	unsigned long tx_bytes = skb->len;
	struct iphdr *ip_header = NULL;
	struct sipc5_frame_data frm;

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

	headroom = tx_build_link_header(&frm, iod, ld, skb->len);

	/* ip loop-back */
	ip_header = (struct iphdr *)skb->data;
	if (iod->msd->loopback_ipaddr &&
		ip_header->daddr == iod->msd->loopback_ipaddr) {
		swap(ip_header->saddr, ip_header->daddr);
		frm.ch_id = DATA_LOOPBACK_CHANNEL;
		frm.hdr[SIPC5_CH_ID_OFFSET] = DATA_LOOPBACK_CHANNEL;
	}

	if (ld->aligned)
		tailroom = sipc5_calc_padding_size(frm.len);

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

	memcpy(skb_push(skb_new, headroom), frm.hdr, headroom);
	if (tailroom)
		skb_put(skb_new, tailroom);

	skbpriv(skb_new)->iod = iod;
	skbpriv(skb_new)->ld = ld;

	ret = ld->send(ld, iod, skb_new);
	if (ret < 0) {
		netif_stop_queue(ndev);
		mif_info("%s: ERR! ld->send fail (err %d)\n", iod->name, ret);
		return NETDEV_TX_BUSY;
	}

	ndev->stats.tx_packets++;
	ndev->stats.tx_bytes += tx_bytes;

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

