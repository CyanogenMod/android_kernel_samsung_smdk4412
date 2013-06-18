/*
 * Copyright (C) 2010 Google, Inc.
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

#define DEBUG

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <linux/gpio.h>
#include <linux/if_arp.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_link_device_usb.h"
#include "modem_utils.h"
#include "modem_link_pm_usb.h"

#include <mach/regs-gpio.h>

#define URB_COUNT	4

static int usb_tx_urb_with_skb(struct usb_link_device *usb_ld,
		struct sk_buff *skb, struct if_usb_devdata *pipe_data);

static void
usb_free_urbs(struct usb_link_device *usb_ld, struct if_usb_devdata *pipe)
{
	struct usb_device *usbdev = usb_ld->usbdev;
	struct urb *urb;

	while ((urb = usb_get_from_anchor(&pipe->urbs))) {
		usb_poison_urb(urb);
		usb_free_coherent(usbdev, pipe->rx_buf_size,
				urb->transfer_buffer, urb->transfer_dma);
		urb->transfer_buffer = NULL;
		usb_put_urb(urb);
		usb_free_urb(urb);
	}
}

static int start_ipc(struct link_device *ld, struct io_device *iod)
{
	struct sk_buff *skb;
	char data[1] = {'a'};
	int err;
	struct usb_link_device *usb_ld = to_usb_link_device(ld);
	struct if_usb_devdata *pipe_data = &usb_ld->devdata[IF_USB_FMT_EP];

	if (has_hub(usb_ld) && usb_ld->link_pm_data->hub_handshake_done) {
		mif_err("Already send start ipc, skip start ipc\n");
		err = 0;
		goto exit;
	}

	if (!usb_ld->if_usb_connected) {
		mif_err("HSIC/USB not connected, skip start ipc\n");
		err = -ENODEV;
		goto exit;
	}

	if (has_hub(usb_ld) &&
		usb_ld->if_usb_initstates == INIT_IPC_START_DONE) {
		mif_debug("Already IPC started\n");
		err = 0;
		goto exit;
	}

	mif_info("send 'a'\n");

	skb = alloc_skb(16, GFP_ATOMIC);
	if (unlikely(!skb))
		return -ENOMEM;
	memcpy(skb_put(skb, 1), data, 1);

	skbpriv(skb)->iod = iod;
	skbpriv(skb)->ld = &usb_ld->ld;
	err = usb_tx_urb_with_skb(usb_ld, skb, pipe_data);
	if (err < 0) {
		mif_err("usb_tx_urb fail\n");
		goto exit;
	}
	usb_ld->link_pm_data->hub_handshake_done = true;
	usb_ld->if_usb_initstates = INIT_IPC_START_DONE;
exit:
	return err;
}

static int usb_init_communication(struct link_device *ld,
			struct io_device *iod)
{
	int err = 0;
	switch (iod->format) {
	case IPC_BOOT:
		ld->com_state = COM_BOOT;
		skb_queue_purge(&ld->sk_fmt_tx_q);
		break;

	case IPC_RAMDUMP:
		ld->com_state = COM_CRASH;
		break;

	case IPC_FMT:
		err = start_ipc(ld, iod);
		break;

	case IPC_RFS:
	case IPC_RAW:

	default:
		ld->com_state = COM_ONLINE;
		break;
	}

	mif_debug("com_state = %d\n", ld->com_state);
	return err;
}

static void usb_terminate_communication(
			struct link_device *ld, struct io_device *iod)
{
	mif_debug("com_state = %d\n", ld->com_state);
}

static int usb_rx_submit(struct if_usb_devdata *pipe, struct urb *urb,
	gfp_t gfp_flags)
{
	int ret;

	usb_anchor_urb(urb, &pipe->reading);
	ret = usb_submit_urb(urb, gfp_flags);
	if (ret) {
		usb_unanchor_urb(urb);
		usb_anchor_urb(urb, &pipe->urbs);
		mif_err("submit urb fail with ret (%d)\n", ret);
	}

	usb_mark_last_busy(urb->dev);
	return ret;
}

static void usb_rx_complete(struct urb *urb)
{
	struct if_usb_devdata *pipe_data = urb->context;
	struct usb_link_device *usb_ld = usb_get_intfdata(pipe_data->data_intf);
	struct io_device *iod;
	int iod_format = IPC_FMT;
	int ret;

	usb_mark_last_busy(urb->dev);

	switch (urb->status) {
	case 0:
	case -ENOENT:
		if (!urb->actual_length)
			goto re_submit;
		/* call iod recv */
		/* how we can distinguish boot ch with fmt ch ?? */
		switch (pipe_data->format) {
		case IF_USB_FMT_EP:
			iod_format = IPC_FMT;
			pr_buffer("rx", (char *)urb->transfer_buffer,
					(size_t)urb->actual_length, 16);
			break;
		case IF_USB_RAW_EP:
			iod_format = IPC_MULTI_RAW;
			break;
		case IF_USB_RFS_EP:
			iod_format = IPC_RFS;
			break;
		default:
			break;
		}

		/* during boot stage fmt end point */
		/* shared with boot io device */
		/* when we use fmt device only, at boot and ipc exchange
			it can be reduced to 1 device */
		if (iod_format == IPC_FMT &&
			usb_ld->ld.com_state == COM_BOOT)
			iod_format = IPC_BOOT;
		if (iod_format == IPC_FMT &&
			usb_ld->ld.com_state == COM_CRASH)
			iod_format = IPC_RAMDUMP;

		iod = link_get_iod_with_format(&usb_ld->ld, iod_format);
		if (iod) {
			ret = iod->recv(iod,
					&usb_ld->ld,
					(char *)urb->transfer_buffer,
					urb->actual_length);
			if (ret < 0)
				mif_err("io device recv error :%d\n", ret);
		}
re_submit:
		if (urb->status || atomic_read(&usb_ld->suspend_count))
			break;

		usb_mark_last_busy(urb->dev);
		usb_rx_submit(pipe_data, urb, GFP_ATOMIC);
		return;
	case -ESHUTDOWN:
	case -EPROTO:
		break;
	case -EOVERFLOW:
		mif_err("RX overflow\n");
		break;
	default:
		mif_err("RX complete Status (%d)\n", urb->status);
		break;
	}

	usb_anchor_urb(urb, &pipe_data->urbs);
}

static int usb_send(struct link_device *ld, struct io_device *iod,
			struct sk_buff *skb)
{
	struct sk_buff_head *txq;
	size_t tx_size;

	if (iod->format == IPC_RAW)
		txq = &ld->sk_raw_tx_q;
	else
		txq = &ld->sk_fmt_tx_q;

	/* store the tx size before run the tx_delayed_work*/
	tx_size = skb->len;

	/* en queue skb data */
	skb_queue_tail(txq, skb);

	queue_delayed_work(ld->tx_wq, &ld->tx_delayed_work, 0);

	return tx_size;
}

static void usb_tx_complete(struct urb *urb)
{
	int ret = 0;
	struct sk_buff *skb = urb->context;

	switch (urb->status) {
	case 0:
		break;
	default:
		mif_err("TX error (%d)\n", urb->status);
	}

	usb_mark_last_busy(urb->dev);
	ret = pm_runtime_put_autosuspend(&urb->dev->dev);
	if (ret < 0 && ret != -EAGAIN)
		mif_debug("pm_runtime_put_autosuspend failed: %d\n", ret);
	usb_free_urb(urb);
	dev_kfree_skb_any(skb);
}

static void if_usb_force_disconnect(struct work_struct *work)
{
	struct usb_link_device *usb_ld =
		container_of(work, struct usb_link_device, disconnect_work);
	struct usb_device *udev = usb_ld->usbdev;

	/* if already disconnected before run this workqueue */
	if (!udev || !(&udev->dev) || !usb_ld->if_usb_connected)
		return;

	/* disconnect udev's parent if usb hub used */
	if (has_hub(usb_ld))
		udev = udev->parent;

	pm_runtime_get_sync(&udev->dev);
	if (udev->state != USB_STATE_NOTATTACHED) {
		usb_force_disconnect(udev);
		mif_info("force disconnect\n");
	}
	pm_runtime_put_autosuspend(&udev->dev);
}

static void
usb_change_modem_state(struct usb_link_device *usb_ld, enum modem_state state)
{
	struct io_device *iod;

	iod = link_get_iod_with_format(&usb_ld->ld, IPC_FMT);
	if (iod)
		iod->modem_state_changed(iod, state);
}

static int usb_tx_urb_with_skb(struct usb_link_device *usb_ld,
		struct sk_buff *skb, struct if_usb_devdata *pipe_data)
{
	int ret, cnt = 0;
	struct urb *urb;
	struct usb_device *usbdev = usb_ld->usbdev;
	unsigned long flags;

	if (!usbdev || (usbdev->state == USB_STATE_NOTATTACHED) ||
			usb_ld->host_wake_timeout_flag)
		return -ENODEV;

	pm_runtime_get_noresume(&usbdev->dev);

	if (usbdev->dev.power.runtime_status == RPM_SUSPENDED ||
		usbdev->dev.power.runtime_status == RPM_SUSPENDING) {
		usb_ld->resume_status = AP_INITIATED_RESUME;
		SET_SLAVE_WAKEUP(usb_ld->pdata, 1);

		while (!wait_event_interruptible_timeout(usb_ld->l2_wait,
				usbdev->dev.power.runtime_status == RPM_ACTIVE
				|| pipe_data->disconnected,
				HOST_WAKEUP_TIMEOUT_JIFFIES)) {

			if (cnt == MAX_RETRY) {
				mif_err("host wakeup timeout !!\n");
				SET_SLAVE_WAKEUP(usb_ld->pdata, 0);
				pm_runtime_put_autosuspend(&usbdev->dev);
				schedule_work(&usb_ld->disconnect_work);
				usb_ld->host_wake_timeout_flag = 1;
				return -1;
			}
			mif_err("host wakeup timeout ! retry..\n");
			SET_SLAVE_WAKEUP(usb_ld->pdata, 0);
			udelay(100);
			SET_SLAVE_WAKEUP(usb_ld->pdata, 1);
			cnt++;
		}

		if (pipe_data->disconnected) {
			SET_SLAVE_WAKEUP(usb_ld->pdata, 0);
			pm_runtime_put_autosuspend(&usbdev->dev);
			return -ENODEV;
		}

		mif_debug("wait_q done (runtime_status=%d)\n",
				usbdev->dev.power.runtime_status);
	}

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		mif_err("alloc urb error\n");
		if (pm_runtime_put_autosuspend(&usbdev->dev) < 0)
			mif_debug("pm_runtime_put_autosuspend fail\n");
		return -ENOMEM;
	}

	urb->transfer_flags = URB_ZERO_PACKET;
	usb_fill_bulk_urb(urb, usbdev, pipe_data->tx_pipe, skb->data,
			skb->len, usb_tx_complete, (void *)skb);

	spin_lock_irqsave(&usb_ld->lock, flags);
	if (atomic_read(&usb_ld->suspend_count)) {
		/* transmission will be done in resume */
		usb_anchor_urb(urb, &usb_ld->deferred);
		usb_put_urb(urb);
		mif_debug("anchor urb (0x%p)\n", urb);
		spin_unlock_irqrestore(&usb_ld->lock, flags);
		return 0;
	}
	spin_unlock_irqrestore(&usb_ld->lock, flags);

	ret = usb_submit_urb(urb, GFP_KERNEL);
	if (ret < 0) {
		mif_err("usb_submit_urb with ret(%d)\n", ret);
		if (pm_runtime_put_autosuspend(&usbdev->dev) < 0)
			mif_debug("pm_runtime_put_autosuspend fail\n");
	}
	return ret;
}

static void usb_tx_work(struct work_struct *work)
{
	int ret = 0;
	struct link_device *ld =
		container_of(work, struct link_device, tx_delayed_work.work);
	struct usb_link_device *usb_ld = to_usb_link_device(ld);
	struct io_device *iod;
	struct sk_buff *skb;
	struct if_usb_devdata *pipe_data;
	struct link_pm_data *pm_data = usb_ld->link_pm_data;

	/*TODO: check the PHONE ACTIVE STATES */
	/* because tx data wait until hub on with wait_for_complettion, it
	 should queue to single_threaded work queue */
	if (!link_pm_set_active(usb_ld))
		return;

	while (ld->sk_fmt_tx_q.qlen || ld->sk_raw_tx_q.qlen) {
		/* send skb from fmt_txq and raw_txq,
		 * one by one for fair flow control */
		skb = skb_dequeue(&ld->sk_fmt_tx_q);
		if (skb) {
			iod = skbpriv(skb)->iod;
			switch (iod->format) {
			case IPC_BOOT:
			case IPC_RAMDUMP:
			case IPC_FMT:
				/* boot device uses same intf with fmt*/
				pipe_data = &usb_ld->devdata[IF_USB_FMT_EP];
				break;
			case IPC_RFS:
				pipe_data = &usb_ld->devdata[IF_USB_RFS_EP];
				break;
			default:
				/* wrong packet for fmt tx q , drop it */
				dev_kfree_skb_any(skb);
				continue;
			}

			ret = usb_tx_urb_with_skb(usb_ld, skb, pipe_data);
			if (ret < 0) {
				mif_err("usb_tx_urb_with_skb, ret(%d)\n",
					ret);
				skb_queue_head(&ld->sk_fmt_tx_q, skb);
				return;
			}
		}

		skb = skb_dequeue(&ld->sk_raw_tx_q);
		if (skb) {
			pipe_data = &usb_ld->devdata[IF_USB_RAW_EP];
			ret = usb_tx_urb_with_skb(usb_ld, skb, pipe_data);
			if (ret < 0) {
				mif_err("usb_tx_urb_with_skb "
						"for raw, ret(%d)\n",
						ret);
				skb_queue_head(&ld->sk_raw_tx_q, skb);
				return;
			}
		}
	}
}

static int if_usb_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct usb_link_device *usb_ld = usb_get_intfdata(intf);
	struct link_pm_data *pm_data = usb_ld->link_pm_data;
	int i;

	if (atomic_inc_return(&usb_ld->suspend_count) == IF_USB_DEVNUM_MAX) {
		mif_debug("L2\n");

		for (i = 0; i < IF_USB_DEVNUM_MAX; i++)
			usb_kill_anchored_urbs(&usb_ld->devdata[i].reading);

		if (pm_data->freq_unlock)
			pm_data->freq_unlock(&usb_ld->usbdev->dev);

		wake_unlock(&usb_ld->susplock);
	}

	return 0;
}

static void runtime_pm_work(struct work_struct *work)
{
	struct usb_link_device *usb_ld = container_of(work,
		struct usb_link_device, runtime_pm_work.work);
	int ret;

	ret = pm_request_autosuspend(&usb_ld->usbdev->dev);

	if (ret == -EAGAIN || ret == 1)
		queue_delayed_work(system_nrt_wq, &usb_ld->runtime_pm_work,
							msecs_to_jiffies(50));
}

static void post_resume_work(struct work_struct *work)
{
	struct usb_link_device *usb_ld = container_of(work,
			struct usb_link_device, post_resume_work.work);
	struct link_pm_data *pm_data = usb_ld->link_pm_data;
	struct usb_device *udev = usb_ld->usbdev;

	/* if already disconnected before run this workqueue */
	if (!udev || !(&udev->dev) || !usb_ld->if_usb_connected)
		return;

	/* lock cpu/bus frequency when L2->L0 */
	if (pm_data->freq_lock)
		pm_data->freq_lock(&udev->dev);
}

static void wait_enumeration_work(struct work_struct *work)
{
	struct usb_link_device *usb_ld = container_of(work,
		struct usb_link_device, wait_enumeration.work);
	if (usb_ld->if_usb_connected == 0) {
		mif_err("USB disconnected and not enumerated for long time\n");
		usb_change_modem_state(usb_ld, STATE_CRASH_EXIT);
	}
}

static int if_usb_resume(struct usb_interface *intf)
{
	int i, ret;
	struct sk_buff *skb;
	struct usb_link_device *usb_ld = usb_get_intfdata(intf);
	struct if_usb_devdata *pipe;
	struct urb *urb;

	spin_lock_irq(&usb_ld->lock);
	if (!atomic_dec_return(&usb_ld->suspend_count)) {
		spin_unlock_irq(&usb_ld->lock);

		mif_debug("\n");
		wake_lock(&usb_ld->susplock);

		/* HACK: Runtime pm does not allow requesting autosuspend from
		 * resume callback, delayed it after resume */
		queue_delayed_work(system_nrt_wq, &usb_ld->runtime_pm_work,
							msecs_to_jiffies(50));

		for (i = 0; i < IF_USB_DEVNUM_MAX; i++) {
			pipe = &usb_ld->devdata[i];
			while ((urb = usb_get_from_anchor(&pipe->urbs))) {
				ret = usb_rx_submit(pipe, urb, GFP_KERNEL);
				if (ret < 0) {
					usb_put_urb(urb);
					mif_err(
					"usb_rx_submit error with (%d)\n",
						ret);
					return ret;
				}
				usb_put_urb(urb);
			}
		}

		while ((urb = usb_get_from_anchor(&usb_ld->deferred))) {
			mif_debug("got urb (0x%p) from anchor & resubmit\n",
					urb);
			ret = usb_submit_urb(urb, GFP_KERNEL);
			if (ret < 0) {
				mif_err("resubmit failed\n");
				skb = urb->context;
				dev_kfree_skb_any(skb);
				usb_free_urb(urb);
				ret = pm_runtime_put_autosuspend(
						&usb_ld->usbdev->dev);
				if (ret < 0 && ret != -EAGAIN)
					mif_debug("pm_runtime_put_autosuspend "
							"failed: %d\n", ret);
			}
		}
		SET_SLAVE_WAKEUP(usb_ld->pdata, 1);
		udelay(100);
		SET_SLAVE_WAKEUP(usb_ld->pdata, 0);

		/* if_usb_resume() is atomic. post_resume_work is
		 * a kind of bottom halves
		 */
		queue_delayed_work(system_nrt_wq, &usb_ld->post_resume_work, 0);

		return 0;
	}

	spin_unlock_irq(&usb_ld->lock);
	return 0;
}

static int if_usb_reset_resume(struct usb_interface *intf)
{
	int ret;

	mif_debug("\n");
	ret = if_usb_resume(intf);
	return ret;
}

static struct usb_device_id if_usb_ids[] = {
	{ USB_DEVICE(0x04e8, 0x6999), /* CMC221 LTE Modem */
	/*.driver_info = 0,*/
	},
	{ } /* terminating entry */
};
MODULE_DEVICE_TABLE(usb, if_usb_ids);

static struct usb_driver if_usb_driver;
static void if_usb_disconnect(struct usb_interface *intf)
{
	struct usb_link_device *usb_ld  = usb_get_intfdata(intf);
	struct usb_device *usbdev = usb_ld->usbdev;
	struct link_pm_data *pm_data = usb_ld->link_pm_data;
	int dev_id = intf->altsetting->desc.bInterfaceNumber;
	struct if_usb_devdata *pipe_data = &usb_ld->devdata[dev_id];


	usb_set_intfdata(intf, NULL);

	pipe_data->disconnected = 1;
	smp_wmb();

	wake_up(&usb_ld->l2_wait);

	usb_ld->if_usb_connected = 0;
	usb_ld->flow_suspend = 1;

	dev_dbg(&usbdev->dev, "%s\n", __func__);
	usb_ld->dev_count--;
	usb_driver_release_interface(&if_usb_driver, pipe_data->data_intf);

	usb_kill_anchored_urbs(&pipe_data->reading);
	usb_free_urbs(usb_ld, pipe_data);

	if (usb_ld->dev_count == 0) {
		cancel_delayed_work_sync(&usb_ld->runtime_pm_work);
		cancel_delayed_work_sync(&usb_ld->post_resume_work);
		cancel_delayed_work_sync(&usb_ld->ld.tx_delayed_work);
		usb_put_dev(usbdev);
		usb_ld->usbdev = NULL;
		if (!has_hub(usb_ld))
			pm_runtime_forbid(pm_data->root_hub);
	}
}

static int __devinit if_usb_probe(struct usb_interface *intf,
					const struct usb_device_id *id)
{
	struct usb_host_interface *data_desc;
	struct usb_link_device *usb_ld =
			(struct usb_link_device *)id->driver_info;
	struct link_device *ld = &usb_ld->ld;
	struct usb_interface *data_intf;
	struct usb_device *usbdev = interface_to_usbdev(intf);
	struct device *dev, *ehci_dev, *root_hub;
	struct if_usb_devdata *pipe;
	struct urb *urb;
	int i;
	int j;
	int dev_id;
	int err;

	/* To detect usb device order probed */
	dev_id = intf->cur_altsetting->desc.bInterfaceNumber;

	if (dev_id >= IF_USB_DEVNUM_MAX) {
		dev_err(&intf->dev, "Device id %d cannot support\n",
								dev_id);
		return -EINVAL;
	}

	if (!usb_ld) {
		dev_err(&intf->dev,
		"if_usb device doesn't be allocated\n");
		err = ENOMEM;
		goto out;
	}

	mif_info("probe dev_id=%d usb_device_id(0x%p), usb_ld (0x%p)\n",
				dev_id, id, usb_ld);

	usb_ld->usbdev = usbdev;
	usb_get_dev(usbdev);

	for (i = 0; i < IF_USB_DEVNUM_MAX; i++) {
		data_intf = usb_ifnum_to_if(usbdev, i);

		/* remap endpoint of RAW to no.1 for LTE modem */
		if (i == 0)
			pipe = &usb_ld->devdata[1];
		else if (i == 1)
			pipe = &usb_ld->devdata[0];
		else
			pipe = &usb_ld->devdata[i];

		pipe->disconnected = 0;
		pipe->data_intf = data_intf;
		data_desc = data_intf->cur_altsetting;

		/* Endpoints */
		if (usb_pipein(data_desc->endpoint[0].desc.bEndpointAddress)) {
			pipe->rx_pipe = usb_rcvbulkpipe(usbdev,
				data_desc->endpoint[0].desc.bEndpointAddress);
			pipe->tx_pipe = usb_sndbulkpipe(usbdev,
				data_desc->endpoint[1].desc.bEndpointAddress);
			pipe->rx_buf_size = 1024*4;
		} else {
			pipe->rx_pipe = usb_rcvbulkpipe(usbdev,
				data_desc->endpoint[1].desc.bEndpointAddress);
			pipe->tx_pipe = usb_sndbulkpipe(usbdev,
				data_desc->endpoint[0].desc.bEndpointAddress);
			pipe->rx_buf_size = 1024*4;
		}

		if (i == 0) {
			dev_info(&usbdev->dev, "USB IF USB device found\n");
		} else {
			err = usb_driver_claim_interface(&if_usb_driver,
					data_intf, usb_ld);
			if (err < 0) {
				mif_err("failed to cliam usb interface\n");
				goto out;
			}
		}

		usb_set_intfdata(data_intf, usb_ld);
		usb_ld->dev_count++;
		pm_suspend_ignore_children(&data_intf->dev, true);

		for (j = 0; j < URB_COUNT; j++) {
			urb = usb_alloc_urb(0, GFP_KERNEL);
			if (!urb) {
				mif_err("alloc urb fail\n");
				err = -ENOMEM;
				goto out2;
			}

			urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
			urb->transfer_buffer = usb_alloc_coherent(usbdev,
				pipe->rx_buf_size, GFP_KERNEL,
				&urb->transfer_dma);
			if (!urb->transfer_buffer) {
				mif_err(
				"Failed to allocate transfer buffer\n");
				usb_free_urb(urb);
				err = -ENOMEM;
				goto out2;
			}

			usb_fill_bulk_urb(urb, usbdev, pipe->rx_pipe,
				urb->transfer_buffer, pipe->rx_buf_size,
				usb_rx_complete, pipe);
			usb_anchor_urb(urb, &pipe->urbs);
		}
	}

	/* temporary call reset_resume */
	atomic_set(&usb_ld->suspend_count, 1);
	if_usb_reset_resume(data_intf);
	atomic_set(&usb_ld->suspend_count, 0);

	SET_HOST_ACTIVE(usb_ld->pdata, 1);
	usb_ld->host_wake_timeout_flag = 0;

	if (gpio_get_value(usb_ld->pdata->gpio_phone_active)) {
		struct link_pm_data *pm_data = usb_ld->link_pm_data;
		int delay = pm_data->autosuspend_delay_ms ?:
				DEFAULT_AUTOSUSPEND_DELAY_MS;
		pm_runtime_set_autosuspend_delay(&usbdev->dev, delay);
		dev = &usbdev->dev;
		if (dev->parent) {
			dev_dbg(&usbdev->dev, "if_usb Runtime PM Start!!\n");
			usb_enable_autosuspend(usb_ld->usbdev);
			/* s5p-ehci runtime pm allow - usb phy suspend mode */
			root_hub = &usbdev->bus->root_hub->dev;
			ehci_dev = root_hub->parent;
			mif_debug("ehci device = %s, %s\n",
					dev_driver_string(ehci_dev),
					dev_name(ehci_dev));
			pm_runtime_allow(ehci_dev);

			if (!pm_data->autosuspend)
				pm_runtime_forbid(dev);

			if (has_hub(usb_ld))
				link_pm_preactive(pm_data);

			pm_data->root_hub = root_hub;
		}

		usb_ld->flow_suspend = 0;
		/* Queue work if skbs were pending before a disconnect/probe */
		if (ld->sk_fmt_tx_q.qlen || ld->sk_raw_tx_q.qlen)
			queue_delayed_work(ld->tx_wq, &ld->tx_delayed_work, 0);

		usb_ld->if_usb_connected = 1;
		/*USB3503*/
		mif_debug("hub active complete\n");

		usb_change_modem_state(usb_ld, STATE_ONLINE);
	} else {
		usb_change_modem_state(usb_ld, STATE_LOADER_DONE);
	}

	/* check dynamic switching gpio received
	 * before usb enumeration is completed
	 */
	if (ld->mc->need_switch_to_usb) {
		ld->mc->need_switch_to_usb = false;
		rawdevs_set_tx_link(ld->msd, LINKDEV_USB);
	}

	return 0;

out2:
	usb_ld->dev_count--;
	for (i = 0; i < IF_USB_DEVNUM_MAX; i++)
		usb_free_urbs(usb_ld, &usb_ld->devdata[i]);
out:
	usb_set_intfdata(intf, NULL);
	return err;
}

irqreturn_t usb_resume_irq(int irq, void *data)
{
	int ret;
	struct usb_link_device *usb_ld = data;
	int hwup;
	static int wake_status = -1;
	struct device *dev;

	hwup = gpio_get_value(usb_ld->pdata->gpio_host_wakeup);
	if (hwup == wake_status) {
		mif_err("Received spurious wake irq: %d", hwup);
		return IRQ_HANDLED;
	}
	wake_status = hwup;

	irq_set_irq_type(irq, hwup ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
	/*
	 * exynos BSP has problem when using level interrupt.
	 * If we change irq type from interrupt handler,
	 * we can get level interrupt twice.
	 * this is temporary solution until SYS.LSI resolve this problem.
	 */
	__raw_writel(eint_irq_to_bit(irq), S5P_EINT_PEND(EINT_REG_NR(irq)));
	wake_lock_timeout(&usb_ld->gpiolock, 100);

	mif_err("< H-WUP %d\n", hwup);

	if (!link_pm_is_connected(usb_ld))
		return IRQ_HANDLED;

	if (hwup) {
		dev = &usb_ld->usbdev->dev;
		mif_info("runtime status=%d\n",
				dev->power.runtime_status);

		/* if usb3503 was on, usb_if was resumed by probe */
		if (has_hub(usb_ld) &&
				(dev->power.runtime_status == RPM_ACTIVE ||
				dev->power.runtime_status == RPM_RESUMING))
			return IRQ_HANDLED;

		device_lock(dev);
		if (dev->power.is_prepared || dev->power.is_suspended) {
			pm_runtime_get_noresume(dev);
			ret = 0;
		} else {
			ret = pm_runtime_get_sync(dev);
		}
		device_unlock(dev);
		if (ret < 0) {
			mif_err("pm_runtime_get fail (%d)\n", ret);
			return IRQ_HANDLED;
		}
	} else {
		if (usb_ld->resume_status == AP_INITIATED_RESUME)
			wake_up(&usb_ld->l2_wait);
		usb_ld->resume_status = CP_INITIATED_RESUME;
		pm_runtime_mark_last_busy(&usb_ld->usbdev->dev);
		pm_runtime_put_autosuspend(&usb_ld->usbdev->dev);
	}

	return IRQ_HANDLED;
}

static int if_usb_init(struct usb_link_device *usb_ld)
{
	int ret;
	int i;
	struct if_usb_devdata *pipe;

	/* give it to probe, or global variable needed */
	if_usb_ids[0].driver_info = (unsigned long)usb_ld;

	for (i = 0; i < IF_USB_DEVNUM_MAX; i++) {
		pipe = &usb_ld->devdata[i];
		pipe->format = i;
		pipe->disconnected = 1;
		init_usb_anchor(&pipe->urbs);
		init_usb_anchor(&pipe->reading);
	}

	init_waitqueue_head(&usb_ld->l2_wait);
	init_usb_anchor(&usb_ld->deferred);

	ret = usb_register(&if_usb_driver);
	if (ret) {
		mif_err("usb_register_driver() fail : %d\n", ret);
		return ret;
	}

	return 0;
}

struct link_device *usb_create_link_device(void *data)
{
	int ret;
	struct modem_data *pdata;
	struct platform_device *pdev = (struct platform_device *)data;
	struct usb_link_device *usb_ld = NULL;
	struct link_device *ld = NULL;

	pdata = pdev->dev.platform_data;

	usb_ld = kzalloc(sizeof(struct usb_link_device), GFP_KERNEL);
	if (!usb_ld)
		goto err;

	INIT_LIST_HEAD(&usb_ld->ld.list);
	skb_queue_head_init(&usb_ld->ld.sk_fmt_tx_q);
	skb_queue_head_init(&usb_ld->ld.sk_raw_tx_q);
	spin_lock_init(&usb_ld->lock);

	ld = &usb_ld->ld;
	usb_ld->pdata = pdata;

	ld->name = "usb";
	ld->init_comm = usb_init_communication;
	ld->terminate_comm = usb_terminate_communication;
	ld->send = usb_send;
	ld->com_state = COM_NONE;

	/*ld->tx_wq = create_singlethread_workqueue("usb_tx_wq");*/
	ld->tx_wq = alloc_workqueue("usb_tx_wq",
		WQ_HIGHPRI | WQ_UNBOUND | WQ_RESCUER, 1);

	if (!ld->tx_wq) {
		mif_err("fail to create work Q.\n");
		goto err;
	}

	usb_ld->pdata->irq_host_wakeup = platform_get_irq(pdev, 1);
	wake_lock_init(&usb_ld->gpiolock, WAKE_LOCK_SUSPEND,
		"modem_usb_gpio_wake");
	wake_lock_init(&usb_ld->susplock, WAKE_LOCK_SUSPEND,
		"modem_usb_suspend_block");

	INIT_DELAYED_WORK(&ld->tx_delayed_work, usb_tx_work);
	INIT_DELAYED_WORK(&usb_ld->runtime_pm_work, runtime_pm_work);
	INIT_DELAYED_WORK(&usb_ld->post_resume_work, post_resume_work);
	INIT_DELAYED_WORK(&usb_ld->wait_enumeration, wait_enumeration_work);
	INIT_WORK(&usb_ld->disconnect_work, if_usb_force_disconnect);

	/* create link pm device */
	ret = link_pm_init(usb_ld, data);
	if (ret)
		goto err;

	ret = if_usb_init(usb_ld);
	if (ret)
		goto err;

	return ld;
err:
	if (ld && ld->tx_wq)
		destroy_workqueue(ld->tx_wq);

	kfree(usb_ld);

	return NULL;
}

static struct usb_driver if_usb_driver = {
	.name =		"if_usb_driver",
	.probe =	if_usb_probe,
	.disconnect =	if_usb_disconnect,
	.id_table =	if_usb_ids,
	.suspend =	if_usb_suspend,
	.resume =	if_usb_resume,
	.reset_resume =	if_usb_reset_resume,
	.supports_autosuspend = 1,
};

static void __exit if_usb_exit(void)
{
	usb_deregister(&if_usb_driver);
}

bool usb_is_enumerated(struct modem_shared *msd)
{
	struct link_device *ld = find_linkdev(msd, LINKDEV_USB);
	if (ld)
		return to_usb_link_device(ld)->usbdev != NULL;
	else
		return false;
}


/* lte specific functions */

static int lte_wake_resume(struct device *pdev)
{
	struct modem_data *pdata = pdev->platform_data;
	int val;

	val = gpio_get_value(pdata->gpio_host_wakeup);
	if (!val) {
		mif_debug("> S-WUP 1\n");
		gpio_set_value(pdata->gpio_slave_wakeup, 1);
	}

	return 0;
}

static const struct dev_pm_ops lte_wake_pm_ops = {
	.resume     = lte_wake_resume,
};

static struct platform_driver lte_wake_driver = {
	.driver = {
		.name = "modem_lte_wake",
		.pm   = &lte_wake_pm_ops,
	},
};

static int __init lte_wake_init(void)
{
	return platform_driver_register(&lte_wake_driver);
}
module_init(lte_wake_init);
