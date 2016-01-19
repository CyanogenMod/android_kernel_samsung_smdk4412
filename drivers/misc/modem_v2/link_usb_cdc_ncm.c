/*
 * cdc_ncm.c
 *
 * Copyright (C) ST-Ericsson 2010-2012
 * Contact: Alexey Orishko <alexey.orishko@stericsson.com>
 * Original author: Hans Petter Selasky <hans.petter.selasky@stericsson.com>
 *
 * USB Host Driver for Network Control Model (NCM)
 * http://www.usb.org/developers/devclass_docs/NCM10.zip
 *
 * The NCM encoding, decoding and initialization logic
 * derives from FreeBSD 8.x. if_cdce.c and if_cdcereg.h
 *
 * This software is available to you under a choice of one of two
 * licenses. You may choose this file to be licensed under the terms
 * of the GNU General Public License (GPL) Version 2 or the 2-clause
 * BSD license listed below:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/ctype.h>
#include <linux/ethtool.h>
#include <linux/workqueue.h>
#include <linux/mii.h>
#include <linux/crc32.h>
#include <linux/usb.h>
#include <linux/hrtimer.h>
#include <linux/atomic.h>
#include <linux/usb/usbnet.h>
#include <linux/usb/cdc.h>
#include <linux/rtc.h>

#include <linux/platform_data/modem_v2.h>
#include <linux/wakelock.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "modem_link_device_hsic_ncm.h"
#include "link_usb_cdc_ncm.h"

#define	DRIVER_VERSION				"14-Mar-2012"

static void cdc_ncm_txpath_bh(unsigned long param);
static void cdc_ncm_tx_timeout_start(struct cdc_ncm_ctx *ctx);
static enum hrtimer_restart cdc_ncm_tx_timer_cb(struct hrtimer *hr_timer);
#ifdef CONFIG_LINK_ETHERNET
/*-------------------------------------------------------------------------*/
/* ethtool methods; minidrivers may need to add some more, but
 * they'll probably want to use this base set.
 * get from usbnet
 */

static void cdc_ncm_get_drvinfo(struct net_device *net,
						struct ethtool_drvinfo *info)
{
	struct usbnet *dev = netdev_priv(net);

	strncpy(info->driver, dev->driver_name, sizeof(info->driver));
	strncpy(info->version, DRIVER_VERSION, sizeof(info->version));
	strncpy(info->fw_version, dev->driver_info->description,
		sizeof(info->fw_version));
	usb_make_path(dev->udev, info->bus_info, sizeof(info->bus_info));
}

static int cdc_ncm_get_settings(struct net_device *net, struct ethtool_cmd *cmd)
{
	struct usbnet *dev = netdev_priv(net);

	if (!dev->mii.mdio_read)
		return -EOPNOTSUPP;

	return mii_ethtool_gset(&dev->mii, cmd);
}

static int cdc_ncm_set_settings(struct net_device *net, struct ethtool_cmd *cmd)
{
	struct usbnet *dev = netdev_priv(net);
	int retval;

	if (!dev->mii.mdio_write)
		return -EOPNOTSUPP;

	retval = mii_ethtool_sset(&dev->mii, cmd);

	/* link speed/duplex might have changed */
	if (dev->driver_info->link_reset)
		dev->driver_info->link_reset(dev);

	return retval;
}

static u32 cdc_ncm_get_link(struct net_device *net)
{
	struct usbnet *dev = netdev_priv(net);

	/* If a check_connect is defined, return its result */
	if (dev->driver_info->check_connect)
		return dev->driver_info->check_connect(dev) == 0;

	/* if the device has mii operations, use those */
	if (dev->mii.mdio_read)
		return mii_link_ok(&dev->mii);

	/* Otherwise, dtrt for drivers calling netif_carrier_{on,off} */
	return ethtool_op_get_link(net);
}

static int cdc_ncm_nway_reset(struct net_device *net)
{
	struct usbnet *dev = netdev_priv(net);

	if (!dev->mii.mdio_write)
		return -EOPNOTSUPP;

	return mii_nway_restart(&dev->mii);
}

static u32 cdc_ncm_get_msglevel(struct net_device *net)
{
	struct usbnet *dev = netdev_priv(net);

	return dev->msg_enable;
}

static void cdc_ncm_set_msglevel(struct net_device *net, u32 level)
{
	struct usbnet *dev = netdev_priv(net);

	dev->msg_enable = level;
}

static struct ethtool_ops cdc_ncm_ethtool_ops = {
	.get_drvinfo = cdc_ncm_get_drvinfo,
	.get_link = cdc_ncm_get_link,
	.get_msglevel = cdc_ncm_get_msglevel,
	.set_msglevel = cdc_ncm_set_msglevel,
	.get_settings = cdc_ncm_get_settings,
	.set_settings = cdc_ncm_set_settings,
	.nway_reset = cdc_ncm_nway_reset,
};

static int cdc_ncm_get_ethernet_addr(struct if_usb_devdata *pipe_data,
		int iMACAddress)
{
	int tmp, i;
	unsigned char buf[13];

	tmp = usb_string(pipe_data->usbdev, iMACAddress, buf, sizeof buf);
	if (tmp != 12) {
		mif_debug("bad MAC string %d fetch, %d\n", iMACAddress, tmp);
		if (tmp >= 0)
			tmp = -EINVAL;
		return tmp;
	}
	for (i = tmp = 0; i < 6; i++, tmp += 2)
		pipe_data->iod->ndev->dev_addr[i] =
			(hex_to_bin(buf[tmp]) << 4) + hex_to_bin(buf[tmp + 1]);
	return 0;
}

static int cdc_ncm_setup_ethernet_address(struct if_usb_devdata *pipe_data)
{
	int ret = 0;
	struct cdc_ncm_ctx *ctx = (struct cdc_ncm_ctx *)pipe_data->sedata;

	pipe_data->iod->ndev->ethtool_ops = &cdc_ncm_ethtool_ops;
	ret = cdc_ncm_get_ethernet_addr(pipe_data,
						ctx->ether_desc->iMACAddress);
	if (ret < 0)
		return ret;

	mif_info("MAC-Address: %pM\n", pipe_data->iod->ndev->dev_addr);

	return 0;
}

static inline void cdc_ncm_post_tx_framing(struct sk_buff *skb) { return; }
static inline void cdc_ncm_pre_rx_framing(struct sk_buff *skb) { return; }

#else
static inline int cdc_ncm_setup_ethernet_address(
				struct if_usb_devdata *pipe_data) { return 0; }
static inline void cdc_ncm_post_tx_framing(struct sk_buff *skb)
{
	if (skb) {
		/*pr_skb("NCM-TX:", skb);*/
		skb_push(skb, sizeof(struct ethhdr));
	}
	return;
}
static inline void cdc_ncm_pre_rx_framing(struct sk_buff *skb)
{
	if (skb) {
		skb_pull_inline(skb, sizeof(struct ethhdr));
		/*pr_skb("NCM-RX:", skb);*/
	}
	return;
}
#endif

/* cdc-ncm specific functions */
static u8 cdc_ncm_setup(struct if_usb_devdata *pipe_data)
{
	u32 val;
	u8 flags;
	u8 iface_no;
	int err;
	u16 ntb_fmt_supported;
	struct cdc_ncm_ctx *ctx = (struct cdc_ncm_ctx *)pipe_data->sedata;
	struct usb_device *usbdev = pipe_data->usbdev;

	iface_no = ctx->control->cur_altsetting->desc.bInterfaceNumber;

	mif_debug("iface_no=%d\n", iface_no);

	err = usb_control_msg(usbdev,
				usb_rcvctrlpipe(usbdev, 0),
				USB_CDC_GET_NTB_PARAMETERS,
				USB_TYPE_CLASS | USB_DIR_IN
				 | USB_RECIP_INTERFACE,
				0, iface_no, &ctx->ncm_parm,
				sizeof(ctx->ncm_parm), 10000);
	if (err < 0) {
		mif_debug("failed GET_NTB_PARAMETERS\n");
		return 1;
	}

	/* read correct set of parameters according to device mode */
	ctx->rx_max = le32_to_cpu(ctx->ncm_parm.dwNtbInMaxSize);
	ctx->tx_max = le32_to_cpu(ctx->ncm_parm.dwNtbOutMaxSize);
	ctx->tx_remainder = le16_to_cpu(ctx->ncm_parm.wNdpOutPayloadRemainder);
	ctx->tx_modulus = le16_to_cpu(ctx->ncm_parm.wNdpOutDivisor);
	ctx->tx_ndp_modulus = le16_to_cpu(ctx->ncm_parm.wNdpOutAlignment);
	/* devices prior to NCM Errata shall set this field to zero */
	ctx->tx_max_datagrams = le16_to_cpu(ctx->ncm_parm.wNtbOutMaxDatagrams);
	ntb_fmt_supported = le16_to_cpu(ctx->ncm_parm.bmNtbFormatsSupported);

	if (ctx->func_desc != NULL)
		flags = ctx->func_desc->bmNetworkCapabilities;
	else
		flags = 0;

	mif_debug("dwNtbInMaxSize=%u dwNtbOutMaxSize=%u "
		"wNdpOutPayloadRemainder=%u wNdpOutDivisor=%u "
		"wNdpOutAlignment=%u wNtbOutMaxDatagrams=%u flags=0x%x\n",
		ctx->rx_max, ctx->tx_max, ctx->tx_remainder, ctx->tx_modulus,
		ctx->tx_ndp_modulus, ctx->tx_max_datagrams, flags);

	/* max count of tx datagrams */
	if ((ctx->tx_max_datagrams == 0) ||
			(ctx->tx_max_datagrams > CDC_NCM_DPT_DATAGRAMS_MAX))
		ctx->tx_max_datagrams = CDC_NCM_DPT_DATAGRAMS_MAX;

	/* verify maximum size of received NTB in bytes */
	if (ctx->rx_max < USB_CDC_NCM_NTB_MIN_IN_SIZE) {
		mif_debug("Using min receive length=%d\n",
						USB_CDC_NCM_NTB_MIN_IN_SIZE);
		ctx->rx_max = USB_CDC_NCM_NTB_MIN_IN_SIZE;
	}

	if (ctx->rx_max > CDC_NCM_NTB_MAX_SIZE_RX) {
		mif_debug("Using default maximum receive length=%d\n",
						CDC_NCM_NTB_MAX_SIZE_RX);
		ctx->rx_max = CDC_NCM_NTB_MAX_SIZE_RX;
	}

	/* inform device about NTB input size changes */
	if (ctx->rx_max != le32_to_cpu(ctx->ncm_parm.dwNtbInMaxSize)) {

		if (flags & USB_CDC_NCM_NCAP_NTB_INPUT_SIZE) {
			struct usb_cdc_ncm_ndp_input_size *ndp_in_sz;

			ndp_in_sz = kzalloc(sizeof(*ndp_in_sz), GFP_KERNEL);
			if (!ndp_in_sz) {
				err = -ENOMEM;
				goto size_err;
			}

			err = usb_control_msg(usbdev,
					usb_sndctrlpipe(usbdev, 0),
					USB_CDC_SET_NTB_INPUT_SIZE,
					USB_TYPE_CLASS | USB_DIR_OUT
					 | USB_RECIP_INTERFACE,
					0, iface_no, ndp_in_sz, 8, 1000);
			kfree(ndp_in_sz);
		} else {
			__le32 *dwNtbInMaxSize;
			dwNtbInMaxSize = kzalloc(sizeof(*dwNtbInMaxSize),
					GFP_KERNEL);
			if (!dwNtbInMaxSize) {
				err = -ENOMEM;
				goto size_err;
			}
			*dwNtbInMaxSize = cpu_to_le32(ctx->rx_max);

			err = usb_control_msg(usbdev,
					usb_sndctrlpipe(usbdev, 0),
					USB_CDC_SET_NTB_INPUT_SIZE,
					USB_TYPE_CLASS | USB_DIR_OUT
					 | USB_RECIP_INTERFACE,
					0, iface_no, dwNtbInMaxSize, 4, 1000);
			kfree(dwNtbInMaxSize);
		}
size_err:
		if (err < 0)
			mif_debug("Setting NTB Input Size failed\n");
	}

	/* verify maximum size of transmitted NTB in bytes */
	if ((ctx->tx_max <
	    (CDC_NCM_MIN_HDR_SIZE + CDC_NCM_MIN_DATAGRAM_SIZE)) ||
	    (ctx->tx_max > CDC_NCM_NTB_MAX_SIZE_TX)) {
		mif_debug("Using default maximum transmit length=%d\n",
						CDC_NCM_NTB_MAX_SIZE_TX);
		ctx->tx_max = CDC_NCM_NTB_MAX_SIZE_TX;
	}

	/*
	 * verify that the structure alignment is:
	 * - power of two
	 * - not greater than the maximum transmit length
	 * - not less than four bytes
	 */
	val = ctx->tx_ndp_modulus;

	if ((val < USB_CDC_NCM_NDP_ALIGN_MIN_SIZE) ||
	    (val != ((-val) & val)) || (val >= ctx->tx_max)) {
		mif_debug("Using default alignment: 4 bytes\n");
		ctx->tx_ndp_modulus = USB_CDC_NCM_NDP_ALIGN_MIN_SIZE;
	}

	/*
	 * verify that the payload alignment is:
	 * - power of two
	 * - not greater than the maximum transmit length
	 * - not less than four bytes
	 */
	val = ctx->tx_modulus;

	if ((val < USB_CDC_NCM_NDP_ALIGN_MIN_SIZE) ||
	    (val != ((-val) & val)) || (val >= ctx->tx_max)) {
		mif_debug("Using default transmit modulus: 4 bytes\n");
		ctx->tx_modulus = USB_CDC_NCM_NDP_ALIGN_MIN_SIZE;
	}

	/* verify the payload remainder */
	if (ctx->tx_remainder >= ctx->tx_modulus) {
		mif_debug("Using default transmit remainder: 0 bytes\n");
		ctx->tx_remainder = 0;
	}

	/* adjust TX-remainder according to NCM specification. */
	ctx->tx_remainder = ((ctx->tx_remainder - ETH_HLEN) &
						(ctx->tx_modulus - 1));

	/* additional configuration */

	/* set CRC Mode */
	if (flags & USB_CDC_NCM_NCAP_CRC_MODE) {
		err = usb_control_msg(usbdev, usb_sndctrlpipe(usbdev, 0),
				USB_CDC_SET_CRC_MODE,
				USB_TYPE_CLASS | USB_DIR_OUT
				 | USB_RECIP_INTERFACE,
				USB_CDC_NCM_CRC_NOT_APPENDED,
				iface_no, NULL, 0, 1000);
		if (err < 0)
			mif_debug("Setting CRC mode off failed\n");
	}

	/* set NTB format, if both formats are supported */
	if (ntb_fmt_supported & USB_CDC_NCM_NTH32_SIGN) {
		err = usb_control_msg(usbdev, usb_sndctrlpipe(usbdev, 0),
				USB_CDC_SET_NTB_FORMAT, USB_TYPE_CLASS
				 | USB_DIR_OUT | USB_RECIP_INTERFACE,
				USB_CDC_NCM_NTB16_FORMAT,
				iface_no, NULL, 0, 1000);
		if (err < 0)
			mif_debug("Setting NTB format to 16-bit failed\n");
	}

	ctx->max_datagram_size = CDC_NCM_MIN_DATAGRAM_SIZE;

	/* set Max Datagram Size (MTU) */
	if (flags & USB_CDC_NCM_NCAP_MAX_DATAGRAM_SIZE) {
		__le16 *max_datagram_size;
		u16 eth_max_sz = le16_to_cpu(ctx->ether_desc->wMaxSegmentSize);

		max_datagram_size = kzalloc(sizeof(*max_datagram_size),
				GFP_KERNEL);
		if (!max_datagram_size) {
			err = -ENOMEM;
			goto max_dgram_err;
		}

		err = usb_control_msg(usbdev, usb_rcvctrlpipe(usbdev, 0),
				USB_CDC_GET_MAX_DATAGRAM_SIZE,
				USB_TYPE_CLASS | USB_DIR_IN
				 | USB_RECIP_INTERFACE,
				0, iface_no, max_datagram_size,
				2, 1000);
		if (err < 0) {
			mif_debug("GET_MAX_DATAGRAM_SIZE failed, use size=%u\n",
						CDC_NCM_MIN_DATAGRAM_SIZE);
		} else {
			ctx->max_datagram_size =
				le16_to_cpu(*max_datagram_size);
			/* Check Eth descriptor value */
			if (ctx->max_datagram_size > eth_max_sz)
					ctx->max_datagram_size = eth_max_sz;

			if (ctx->max_datagram_size > CDC_NCM_MAX_DATAGRAM_SIZE)
				ctx->max_datagram_size =
						CDC_NCM_MAX_DATAGRAM_SIZE;

			if (ctx->max_datagram_size < CDC_NCM_MIN_DATAGRAM_SIZE)
				ctx->max_datagram_size =
					CDC_NCM_MIN_DATAGRAM_SIZE;

			/* if value changed, update device */
			if (ctx->max_datagram_size !=
					le16_to_cpu(*max_datagram_size)) {
				err = usb_control_msg(usbdev,
						usb_sndctrlpipe(usbdev, 0),
						USB_CDC_SET_MAX_DATAGRAM_SIZE,
						USB_TYPE_CLASS | USB_DIR_OUT
						 | USB_RECIP_INTERFACE,
						0,
						iface_no, max_datagram_size,
						2, 1000);
				if (err < 0)
					pr_debug("SET_MAX_DGRAM_SIZE failed\n");
			}
		}
		kfree(max_datagram_size);
	}

max_dgram_err:
	if (pipe_data->iod->ndev->mtu != (ctx->max_datagram_size - ETH_HLEN))
		pipe_data->iod->ndev->mtu = ctx->max_datagram_size - ETH_HLEN;

	return 0;
}

static void
cdc_ncm_find_endpoints(struct cdc_ncm_ctx *ctx, struct usb_interface *intf)
{
	struct usb_host_endpoint *e;
	u8 ep;

	for (ep = 0; ep < intf->cur_altsetting->desc.bNumEndpoints; ep++) {

		e = intf->cur_altsetting->endpoint + ep;
		switch (e->desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
		case USB_ENDPOINT_XFER_INT:
			if (usb_endpoint_dir_in(&e->desc)) {
				if (ctx->status_ep == NULL)
					ctx->status_ep = e;
			}
			break;

		case USB_ENDPOINT_XFER_BULK:
			if (usb_endpoint_dir_in(&e->desc)) {
				if (ctx->in_ep == NULL)
					ctx->in_ep = e;
			} else {
				if (ctx->out_ep == NULL)
					ctx->out_ep = e;
			}
			break;

		default:
			break;
		}
	}
}

static void cdc_ncm_free(struct cdc_ncm_ctx *ctx)
{
	if (ctx == NULL)
		return;

	if (ctx->tx_rem_skb != NULL) {
		dev_kfree_skb_any(ctx->tx_rem_skb);
		ctx->tx_rem_skb = NULL;
	}

	if (ctx->tx_curr_skb != NULL) {
		dev_kfree_skb_any(ctx->tx_curr_skb);
		ctx->tx_curr_skb = NULL;
	}

	kfree(ctx);
}

int cdc_ncm_bind(struct if_usb_devdata *pipe_data,
		struct usb_interface *intf, struct usb_link_device *usb_ld)
{
	struct cdc_ncm_ctx *ctx;
	struct usb_driver *usbdrv = to_usb_driver(intf->dev.driver);
	struct usb_device *usbdev = interface_to_usbdev(intf);
	unsigned char *buf = intf->cur_altsetting->extra;
	int buflen = intf->cur_altsetting->extralen;
	const struct usb_cdc_union_desc *union_desc;
	int temp;
	u8 iface_no;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (ctx == NULL)
		return -ENODEV;

	hrtimer_init(&ctx->tx_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ctx->tx_timer.function = &cdc_ncm_tx_timer_cb;
	ctx->bh.data = (unsigned long)pipe_data;
	ctx->bh.func = cdc_ncm_txpath_bh;
	atomic_set(&ctx->stop, 0);
	spin_lock_init(&ctx->mtx);

	/* store ctx pointer in device data field */
	pipe_data->sedata = (void *)ctx;

	ctx->intf = intf;

	/* parse through descriptors associated with control interface */
	while ((buflen > 0) && (buf[0] > 2) && (buf[0] <= buflen)) {

		if (buf[1] == USB_DT_CS_INTERFACE) {
			switch (buf[2]) {
			case USB_CDC_UNION_TYPE:
				if (buf[0] < sizeof(*union_desc))
					break;

				union_desc =
					(const struct usb_cdc_union_desc *)buf;

				ctx->control = usb_ifnum_to_if(usbdev,
					union_desc->bMasterInterface0);
				ctx->data = usb_ifnum_to_if(usbdev,
					union_desc->bSlaveInterface0);
				break;

			case USB_CDC_ETHERNET_TYPE:
				if (buf[0] < sizeof(*(ctx->ether_desc)))
					break;

				ctx->ether_desc =
					(const struct usb_cdc_ether_desc *)buf;

				break;

			case USB_CDC_NCM_TYPE:
				if (buf[0] < sizeof(*(ctx->func_desc)))
					break;

				ctx->func_desc =
					(const struct usb_cdc_ncm_desc *)buf;
				break;

			default:
				break;
			}
		}
		temp = buf[0];
		buf += temp;
		buflen -= temp;
	}

	/* check if we got everything */
	if ((ctx->control == NULL) || (ctx->data == NULL) ||
	    (ctx->ether_desc == NULL) || (ctx->control != intf))
		goto error;

	pipe_data->usbdev = usb_get_dev(usbdev);
	pipe_data->usb_ld = usb_ld;
	pipe_data->disconnected = 0;
	pipe_data->state = STATE_RESUMED;

	/* claim interfaces, if any */
	temp = usb_driver_claim_interface(usbdrv, ctx->data, pipe_data);
	if (temp)
		goto error;

	iface_no = ctx->data->cur_altsetting->desc.bInterfaceNumber;

	/* reset data interface */
	temp = usb_set_interface(usbdev, iface_no, 0);
	if (temp)
		goto error2;

	/* initialize data interface */
	if (cdc_ncm_setup(pipe_data))
		goto error2;

	/* configure data interface */
	temp = usb_set_interface(usbdev, iface_no, 1);
	if (temp)
		goto error2;

	cdc_ncm_find_endpoints(ctx, ctx->data);
	cdc_ncm_find_endpoints(ctx, ctx->control);

	if ((ctx->in_ep == NULL) || (ctx->out_ep == NULL) ||
	    (ctx->status_ep == NULL))
		goto error2;

	usb_set_intfdata(ctx->data, pipe_data);
	usb_set_intfdata(ctx->control, pipe_data);
	usb_set_intfdata(ctx->intf, pipe_data);

	pipe_data->rx_pipe = usb_rcvbulkpipe(usbdev,
		ctx->in_ep->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
	pipe_data->tx_pipe = usb_sndbulkpipe(usbdev,
		ctx->out_ep->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
	pipe_data->status = ctx->status_ep;
	pipe_data->rx_buf_size = ctx->rx_max;

	mif_debug("EP status: %x, tx:%x, rx:%x\n",
		ctx->status_ep->desc.bEndpointAddress,
		ctx->in_ep->desc.bEndpointAddress,
		ctx->out_ep->desc.bEndpointAddress);

	temp = cdc_ncm_setup_ethernet_address(pipe_data);
	if (temp)
		goto error2;

	/*
	 * We should get an event when network connection is "connected" or
	 * "disconnected". Set network connection in "disconnected" state
	 * (carrier is OFF) during attach, so the IP network stack does not
	 * start IPv6 negotiation and more.
	 */
	netif_carrier_off(pipe_data->iod->ndev);
	ctx->tx_speed = ctx->rx_speed = 0;
	if (pipe_data->iod->ndev->mtu != (ctx->max_datagram_size - ETH_HLEN))
		pipe_data->iod->ndev->mtu = ctx->max_datagram_size - ETH_HLEN;

	pipe_data->ntb_pool = &pipe_data->usb_ld->skb_pool;

	return 0;

error2:
	usb_set_intfdata(ctx->control, NULL);
	usb_set_intfdata(ctx->data, NULL);
	usb_driver_release_interface(usbdrv, ctx->data);
error:
	cdc_ncm_free((struct cdc_ncm_ctx *)pipe_data->sedata);
	pipe_data->sedata = NULL;
	mif_err("bind() failure\n");
	return -ENODEV;
}

void cdc_ncm_unbind(struct if_usb_devdata *pipe_data,
		struct usb_interface *intf)
{
	struct cdc_ncm_ctx *ctx = (struct cdc_ncm_ctx *)pipe_data->sedata;
	struct usb_driver *usbdrv = driver_of(intf);

	if (ctx == NULL)
		return;		/* no setup */

	netif_carrier_off(pipe_data->iod->ndev);
	atomic_set(&ctx->stop, 1);

	if (hrtimer_active(&ctx->tx_timer))
		hrtimer_cancel(&ctx->tx_timer);

	tasklet_kill(&ctx->bh);

	/* disconnect master --> disconnect slave */
	if (intf == ctx->control && ctx->data) {
		usb_set_intfdata(ctx->data, NULL);
		usb_driver_release_interface(usbdrv, ctx->data);
		ctx->data = NULL;
	} else if (intf == ctx->data && ctx->control) {
		usb_set_intfdata(ctx->control, NULL);
		usb_driver_release_interface(usbdrv, ctx->control);
		ctx->control = NULL;
	}
	pipe_data->usbdev = NULL;
	pipe_data->disconnected = 1;
	pipe_data->state = STATE_SUSPENDED;

	usb_set_intfdata(ctx->intf, NULL);
	cdc_ncm_free(ctx);

	pipe_data->sedata = NULL;
}

static void cdc_ncm_zero_fill(u8 *ptr, u32 first, u32 end, u32 max)
{
	if (first >= max)
		return;
	if (first >= end)
		return;
	if (end > max)
		end = max;
	memset(ptr + first, 0, end - first);
}

static struct sk_buff *
cdc_ncm_fill_tx_frame(struct if_usb_devdata *pipe_data, struct sk_buff *skb)
{
	struct sk_buff *skb_out;
	struct cdc_ncm_ctx *ctx = (struct cdc_ncm_ctx *)pipe_data->sedata;
	u32 rem;
	u32 offset;
	u32 last_offset;
	u16 n = 0, index;
	u8 ready2send = 0;

	/* if there is a remaining skb, it gets priority */
	if (skb != NULL)
		swap(skb, ctx->tx_rem_skb);
	else
		ready2send = 1;

	/*
	 * +----------------+
	 * | skb_out        |
	 * +----------------+
	 *           ^ offset
	 *        ^ last_offset
	 */

	/* check if we are resuming an OUT skb */
	if (ctx->tx_curr_skb != NULL) {
		/* pop variables */
		skb_out = ctx->tx_curr_skb;
		offset = ctx->tx_curr_offset;
		last_offset = ctx->tx_curr_last_offset;
		n = ctx->tx_curr_frame_num;

	} else {
#if 0
		skb_out = mif_skb_pool_alloc(devdata->ntb_pool);
		if (!skb_out) {
			mif_err("pool: get skb pool fail\n");
			if (skb)
				dev_kfree_skb_any(skb);
			goto exit_no_skb;
		}
#else
/* original code, we should compare and add to exception case
 reset variables */
		skb_out = alloc_skb((ctx->tx_max + 1), GFP_ATOMIC);
		if (skb_out == NULL) {
			if (skb != NULL)
				dev_kfree_skb_any(skb);
			goto exit_no_skb;
		}
#endif
		/* make room for NTH and NDP */
		offset = ALIGN(sizeof(struct usb_cdc_ncm_nth16),
					ctx->tx_ndp_modulus) +
					sizeof(struct usb_cdc_ncm_ndp16) +
					(ctx->tx_max_datagrams + 1) *
					sizeof(struct usb_cdc_ncm_dpe16);

		/* store last valid offset before alignment */
		last_offset = offset;
		/* align first Datagram offset correctly */
		offset = ALIGN(offset, ctx->tx_modulus) + ctx->tx_remainder;
		/* zero buffer till the first IP datagram */
		cdc_ncm_zero_fill(skb_out->data, 0, offset, offset);
		n = 0;
		ctx->tx_curr_frame_num = 0;
	}

	for (; n < ctx->tx_max_datagrams; n++) {
		/* check if end of transmit buffer is reached */
		if (offset >= ctx->tx_max) {
			ready2send = 1;
			break;
		}
		/* compute maximum buffer size */
		rem = ctx->tx_max - offset;

		if (skb == NULL) {
			skb = ctx->tx_rem_skb;
			ctx->tx_rem_skb = NULL;

			/* check for end of skb */
			if (skb == NULL)
				break;
		}

		if (skb->len > rem) {
			if (n == 0) {
				/* won't fit, MTU problem? */
				dev_kfree_skb_any(skb);
				skb = NULL;
			} else {
				/* no room for skb - store for later */
				if (ctx->tx_rem_skb != NULL)
					dev_kfree_skb_any(ctx->tx_rem_skb);
				ctx->tx_rem_skb = skb;
				skb = NULL;
				ready2send = 1;
			}
			break;
		}

		memcpy(((u8 *)skb_out->data) + offset, skb->data, skb->len);

		ctx->tx_ncm.dpe16[n].wDatagramLength = cpu_to_le16(skb->len);
		ctx->tx_ncm.dpe16[n].wDatagramIndex = cpu_to_le16(offset);

		/* update offset */
		offset += skb->len;

		/* store last valid offset before alignment */
		last_offset = offset;

		/* align offset correctly */
		offset = ALIGN(offset, ctx->tx_modulus) + ctx->tx_remainder;

		/* zero padding */
		cdc_ncm_zero_fill(skb_out->data, last_offset, offset,
								ctx->tx_max);
		dev_kfree_skb_any(skb);
		skb = NULL;
	}

	/* free up any dangling skb */
	if (skb != NULL) {
		dev_kfree_skb_any(skb);
		skb = NULL;
	}
	ctx->tx_curr_frame_num = n;

	if (n == 0) {
		/* wait for more frames */
		/* push variables */
		ctx->tx_curr_skb = skb_out;
		ctx->tx_curr_offset = offset;
		ctx->tx_curr_last_offset = last_offset;
		goto exit_no_skb;

	} else if ((n < ctx->tx_max_datagrams) && (ready2send == 0)) {
		/* wait for more frames */
		/* push variables */
		ctx->tx_curr_skb = skb_out;
		ctx->tx_curr_offset = offset;
		ctx->tx_curr_last_offset = last_offset;
		/* set the pending count */
		if (n < CDC_NCM_RESTART_TIMER_DATAGRAM_CNT)
			ctx->tx_timer_pending = CDC_NCM_TIMER_PENDING_CNT;
		goto exit_no_skb;

	} else {
		/* frame goes out */
		/* variables will be reset at next call */
	}

	/* check for overflow */
	if (last_offset > ctx->tx_max)
		last_offset = ctx->tx_max;

	/* revert offset */
	offset = last_offset;

	/*
	 * If collected data size is less or equal CDC_NCM_MIN_TX_PKT bytes,
	 * we send buffers as it is. If we get more data, it would be more
	 * efficient for USB HS mobile device with DMA engine to receive a full
	 * size NTB, than canceling DMA transfer and receiving a short packet.
	 */
	if (offset > CDC_NCM_MIN_TX_PKT)
		offset = ctx->tx_max;

	/* final zero padding */
	cdc_ncm_zero_fill(skb_out->data, last_offset, offset, ctx->tx_max);

	/* store last offset */
	last_offset = offset;

	if (((last_offset < ctx->tx_max) && ((last_offset %
			le16_to_cpu(ctx->out_ep->desc.wMaxPacketSize)) == 0)) ||
	    (((last_offset == ctx->tx_max) && ((ctx->tx_max %
		le16_to_cpu(ctx->out_ep->desc.wMaxPacketSize)) == 0)) &&
		(ctx->tx_max < le32_to_cpu(ctx->ncm_parm.dwNtbOutMaxSize)))) {
		/* force short packet */
		*(((u8 *)skb_out->data) + last_offset) = 0;
		last_offset++;
	}

	/* zero the rest of the DPEs plus the last NULL entry */
	for (; n <= CDC_NCM_DPT_DATAGRAMS_MAX; n++) {
		ctx->tx_ncm.dpe16[n].wDatagramLength = 0;
		ctx->tx_ncm.dpe16[n].wDatagramIndex = 0;
	}

	/* fill out 16-bit NTB header */
	ctx->tx_ncm.nth16.dwSignature = cpu_to_le32(USB_CDC_NCM_NTH16_SIGN);
	ctx->tx_ncm.nth16.wHeaderLength =
					cpu_to_le16(sizeof(ctx->tx_ncm.nth16));
	ctx->tx_ncm.nth16.wSequence = cpu_to_le16(ctx->tx_seq);
	ctx->tx_ncm.nth16.wBlockLength = cpu_to_le16(last_offset);
	index = ALIGN(sizeof(struct usb_cdc_ncm_nth16), ctx->tx_ndp_modulus);
	ctx->tx_ncm.nth16.wNdpIndex = cpu_to_le16(index);

	memcpy(skb_out->data, &(ctx->tx_ncm.nth16), sizeof(ctx->tx_ncm.nth16));
	ctx->tx_seq++;

	/* fill out 16-bit NDP table */
	ctx->tx_ncm.ndp16.dwSignature =
				cpu_to_le32(USB_CDC_NCM_NDP16_NOCRC_SIGN);
	rem = sizeof(ctx->tx_ncm.ndp16) + ((ctx->tx_curr_frame_num + 1) *
					sizeof(struct usb_cdc_ncm_dpe16));
	ctx->tx_ncm.ndp16.wLength = cpu_to_le16(rem);
	ctx->tx_ncm.ndp16.wNextNdpIndex = 0; /* reserved */

	memcpy(((u8 *)skb_out->data) + index,
						&(ctx->tx_ncm.ndp16),
						sizeof(ctx->tx_ncm.ndp16));

	memcpy(((u8 *)skb_out->data) + index + sizeof(ctx->tx_ncm.ndp16),
					&(ctx->tx_ncm.dpe16),
					(ctx->tx_curr_frame_num + 1) *
					sizeof(struct usb_cdc_ncm_dpe16));

	/* set frame length */
	skb_put(skb_out, last_offset);

	/* return skb */
	ctx->tx_curr_skb = NULL;
	pipe_data->iod->ndev->stats.tx_packets += ctx->tx_curr_frame_num;
	return skb_out;

exit_no_skb:
	/* Start timer, if there is a remaining skb */
	if (ctx->tx_curr_skb != NULL)
		cdc_ncm_tx_timeout_start(ctx);
	return NULL;
}

static void cdc_ncm_tx_timeout_start(struct cdc_ncm_ctx *ctx)
{
	if (!ctx)
		return;

	/* start timer, if not already started */
	if (!(hrtimer_active(&ctx->tx_timer) || atomic_read(&ctx->stop)))
		hrtimer_start(&ctx->tx_timer,
				ktime_set(0, CDC_NCM_TIMER_INTERVAL),
				HRTIMER_MODE_REL);
}

static enum hrtimer_restart cdc_ncm_tx_timer_cb(struct hrtimer *timer)
{
	struct cdc_ncm_ctx *ctx =
			container_of(timer, struct cdc_ncm_ctx, tx_timer);

	if (!atomic_read(&ctx->stop))
		tasklet_schedule(&ctx->bh);
	return HRTIMER_NORESTART;
}

static void cdc_ncm_txpath_bh(unsigned long param)
{
	struct if_usb_devdata *pipe_data = (struct if_usb_devdata *)param;
	struct cdc_ncm_ctx *ctx = (struct cdc_ncm_ctx *)pipe_data->sedata;

	if (!ctx)
		return;

	spin_lock_bh(&ctx->mtx);
	if (ctx->tx_timer_pending != 0) {
		ctx->tx_timer_pending--;
		cdc_ncm_tx_timeout_start(ctx);
		spin_unlock_bh(&ctx->mtx);
	} else if (pipe_data->iod->ndev != NULL) {
		spin_unlock_bh(&ctx->mtx);
		netif_tx_lock_bh(pipe_data->iod->ndev);
		usb_tx_skb(pipe_data, NULL);
		netif_tx_unlock_bh(pipe_data->iod->ndev);
	}
}

struct sk_buff *cdc_ncm_tx_fixup(struct if_usb_devdata *pipe_data,
					struct sk_buff *skb, gfp_t flags)
{
	struct sk_buff *skb_out = NULL;
	struct cdc_ncm_ctx *ctx = (struct cdc_ncm_ctx *)pipe_data->sedata;

	if (ctx == NULL)
		goto error;

	spin_lock_bh(&ctx->mtx);
	cdc_ncm_post_tx_framing(skb);
	skb_out = cdc_ncm_fill_tx_frame(pipe_data, skb);
	if (skb_out) {
		skbpriv(skb_out)->iod = pipe_data->iod;
		skbpriv(skb_out)->ld = &pipe_data->usb_ld->ld;
		skbpriv(skb_out)->context = pipe_data;
	}
	spin_unlock_bh(&ctx->mtx);

	return skb_out;
error:
	if (skb != NULL)
		dev_kfree_skb_any(skb);

	return NULL;
}

int cdc_ncm_rx_fixup(struct if_usb_devdata *pipe_data, struct sk_buff *skb_in)
{
	int err = -EINVAL;
	struct sk_buff *skb;
	struct cdc_ncm_ctx *ctx = (struct cdc_ncm_ctx *)pipe_data->sedata;
	int len;
	int nframes;
	int x;
	int offset;
	struct usb_cdc_ncm_nth16 *nth16;
	struct usb_cdc_ncm_ndp16 *ndp16;
	struct usb_cdc_ncm_dpe16 *dpe16;

	if (ctx == NULL)
		goto error;

	if (skb_in->len < (sizeof(struct usb_cdc_ncm_nth16) +
					sizeof(struct usb_cdc_ncm_ndp16))) {
		pr_debug("frame too short\n");
		goto error;
	}

	nth16 = (struct usb_cdc_ncm_nth16 *)skb_in->data;

	if (le32_to_cpu(nth16->dwSignature) != USB_CDC_NCM_NTH16_SIGN) {
		pr_debug("invalid NTH16 signature <%u>\n",
					le32_to_cpu(nth16->dwSignature));
		goto error;
	}

	len = le16_to_cpu(nth16->wBlockLength);
	if (len > ctx->rx_max) {
		pr_debug("unsupported NTB block length %u/%u\n", len,
								ctx->rx_max);
		goto error;
	}

	if ((ctx->rx_seq + 1) != le16_to_cpu(nth16->wSequence) &&
		(ctx->rx_seq || le16_to_cpu(nth16->wSequence)) &&
		!((ctx->rx_seq == 0xffff) && !le16_to_cpu(nth16->wSequence))) {
		pr_debug("sequence number glitch prev=%d curr=%d\n",
				ctx->rx_seq, le16_to_cpu(nth16->wSequence));
	}
	ctx->rx_seq = le16_to_cpu(nth16->wSequence);

	len = le16_to_cpu(nth16->wNdpIndex);
	if ((len + sizeof(struct usb_cdc_ncm_ndp16)) > skb_in->len) {
		pr_debug("invalid DPT16 index <%u>\n",
					le16_to_cpu(nth16->wNdpIndex));
		goto error;
	}

	ndp16 = (struct usb_cdc_ncm_ndp16 *)(((u8 *)skb_in->data) + len);

	if (le32_to_cpu(ndp16->dwSignature) != USB_CDC_NCM_NDP16_NOCRC_SIGN) {
		pr_debug("invalid DPT16 signature <%u>\n",
					le32_to_cpu(ndp16->dwSignature));
		goto error;
	}

	if (le16_to_cpu(ndp16->wLength) < USB_CDC_NCM_NDP16_LENGTH_MIN) {
		pr_debug("invalid DPT16 length <%u>\n",
					le32_to_cpu(ndp16->dwSignature));
		goto error;
	}

	nframes = ((le16_to_cpu(ndp16->wLength) -
					sizeof(struct usb_cdc_ncm_ndp16)) /
					sizeof(struct usb_cdc_ncm_dpe16));
	nframes--; /* we process NDP entries except for the last one */

	len += sizeof(struct usb_cdc_ncm_ndp16);

	if ((len + nframes * (sizeof(struct usb_cdc_ncm_dpe16))) >
								skb_in->len) {
		pr_debug("Invalid nframes = %d\n", nframes);
		goto error;
	}

	dpe16 = (struct usb_cdc_ncm_dpe16 *)(((u8 *)skb_in->data) + len);

	for (x = 0; x < nframes; x++, dpe16++) {
		offset = le16_to_cpu(dpe16->wDatagramIndex);
		len = le16_to_cpu(dpe16->wDatagramLength);

		/*
		 * CDC NCM ch. 3.7
		 * All entries after first NULL entry are to be ignored
		 */
		if ((offset == 0) || (len == 0)) {
			if (!x)
				goto error; /* empty NTB */
			break;
		}

		/* sanity checking */
		if (((offset + len) > skb_in->len) ||
				(len > ctx->rx_max) || (len < ETH_HLEN)) {
			mif_debug("invalid frame detected (ignored)"
					"offset[%u]=%u, length=%u, skb=%p\n",
					x, offset, len, skb_in);
			if (!x)
				goto error;
			break;

		} else {
			skb = skb_clone(skb_in, GFP_ATOMIC);
			if (!skb)
				goto error;
			skb->len = len;
			skb->truesize = SKB_TRUESIZE(len); /* tcp_rmem */
			skb->data = ((u8 *)skb_in->data) + offset;
			skb_set_tail_pointer(skb, len);
			cdc_ncm_pre_rx_framing(skb);
			skbpriv(skb)->real_iod = pipe_data->iod;
			skbpriv(skb)->iod = pipe_data->iod;

			err = pipe_data->iod->recv_skb(pipe_data->iod,
				&pipe_data->usb_ld->ld, skb);
			if (err < 0)
				mif_err("rx iod fail err=%d\n", err);
		}
	}
	dev_kfree_skb_any(skb_in);
	return 1;
error:
	return 0;
}

static void
cdc_ncm_speed_change(struct cdc_ncm_ctx *ctx,
		     struct usb_cdc_speed_change *data)
{
	/*
	 * Currently the USB-NET API does not support reporting the actual
	 * device speed. Do print it instead.
	 */
}

static void cdc_ncm_status(struct if_usb_devdata *pipe_data, struct urb *urb)
{
	struct cdc_ncm_ctx *ctx;
	struct usb_cdc_notification *event;

	ctx = (struct cdc_ncm_ctx *)pipe_data->sedata;

	if (urb->actual_length < sizeof(*event))
		return;

	/* test for split data in 8-byte chunks */
	if (test_and_clear_bit(EVENT_STS_SPLIT, &pipe_data->flags)) {
		cdc_ncm_speed_change(ctx,
		      (struct usb_cdc_speed_change *)urb->transfer_buffer);
		return;
	}

	event = urb->transfer_buffer;

	mif_debug("event: %d by ep=%d\n", event->bNotificationType,
		usb_pipeendpoint(urb->pipe));

	switch (event->bNotificationType) {
	case USB_CDC_NOTIFY_NETWORK_CONNECTION:
		/*
		 * According to the CDC NCM specification ch.7.1
		 * USB_CDC_NOTIFY_NETWORK_CONNECTION notification shall be
		 * sent by device after USB_CDC_NOTIFY_SPEED_CHANGE.
		 */
		ctx->connected = event->wValue;
		pipe_data->net_connected = (event->wValue) ? true : false;

		printk(KERN_INFO KBUILD_MODNAME
			": network connection: %s connected\n",
			ctx->connected ? "" : "dis");

		if (ctx->connected)
			netif_carrier_on(pipe_data->iod->ndev);
		else {
			netif_carrier_off(pipe_data->iod->ndev);
			ctx->tx_speed = ctx->rx_speed = 0;
		}
		break;

	case USB_CDC_NOTIFY_SPEED_CHANGE:
		if (urb->actual_length < (sizeof(*event) +
					sizeof(struct usb_cdc_speed_change)))
			set_bit(EVENT_STS_SPLIT, &pipe_data->flags);
		else
			cdc_ncm_speed_change(ctx,
				(struct usb_cdc_speed_change *) &event[1]);
		break;

	default:
		mif_err("unexpected notification 0x%02x!\n",
			event->bNotificationType);
		break;
	}
}

void cdc_ncm_intr_complete(struct urb *urb)
{
	struct if_usb_devdata *pipe_data = urb->context;
	int ret;

	mif_debug("status = %d\n", urb->status);

	switch (urb->status) {
	/* success */
	case -ENOENT:		/* urb killed by L2 suspend */
	case 0:
		if (urb->actual_length) {
			mif_info("ep=%d\n", usb_pipeendpoint(urb->pipe));
			pr_urb(__func__, urb);
		}
		cdc_ncm_status(pipe_data, urb);
		break;

	case -ESHUTDOWN:	/* hardware gone */
		mif_err("intr shutdown, code %d\n", urb->status);
		return;

	/* NOTE:  not throttling like RX/TX, since this endpoint
	 * already polls infrequently
	 */
	default:
		mif_err("intr status %d\n", urb->status);
		break;
	}

	if (!urb->status) { /*skip -ENOENT L2 enter status */
		memset(urb->transfer_buffer, 0, urb->transfer_buffer_length);
		ret = usb_submit_urb(urb, GFP_ATOMIC);
		mif_debug("status: usb_submit_urb ret=%d\n", ret);
		if (ret != 0)
			mif_err("intr resubmit --> %d\n", ret);
	}
}

