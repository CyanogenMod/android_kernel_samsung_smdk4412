/*
 * drivers/usb/gadget/f_mtp_samsung.c
 *
 * Function Driver for USB MTP,
 * f_mtp_samsung.c -- MTP Driver, for MTP development,
 *
 * Copyright (C) 2009 by Samsung Electronics,
 * Author:Deepak M.G. <deepak.guru@samsung.com>,
 * Author:Madhukar.J  <madhukar.j@samsung.com>,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * f_mtp_samsung.c file is the driver for MTP device. Totally three
 * EndPoints will be configured in which 2 Bulk End Points
 * and 1 Interrupt End point. This driver will also register as
 * misc driver and exposes file operation funtions to user space.
 */

/* Includes */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/usb.h>
#include <linux/usb_usual.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/hardirq.h>
#include <linux/sched.h>
#include <linux/usb/f_accessory.h>
#include <asm-generic/siginfo.h>
#include <linux/usb/android_composite.h>
#include <linux/kernel.h>
#include "f_mtp.h"
#include "gadget_chips.h"

/*-------------------------------------------------------------------------*/
/*Only for Debug*/
#define DEBUG_MTP 0
/*#define CSY_TEST */

#if DEBUG_MTP
#define DEBUG_MTP_SETUP
#define DEBUG_MTP_READ
#define DEBUG_MTP_WRITE

#else
#undef DEBUG_MTP_SETUP
#undef DEBUG_MTP_READ
#undef DEBUG_MTP_WRITE
#endif

/*#define DEBUG_MTP_SETUP*/
/*#define DEBUG_MTP_READ*/
/*#define DEBUG_MTP_WRITE*/

#ifdef DEBUG_MTP_SETUP
#define DEBUG_MTPB(fmt, args...) printk(fmt, ##args)
#else
#define DEBUG_MTPB(fmt, args...) do {} while (0)
#endif

#ifdef DEBUG_MTP_READ
#define DEBUG_MTPR(fmt, args...) printk(fmt, ##args)
#else
#define DEBUG_MTPR(fmt, args...) do {} while (0)
#endif
#ifdef DEBUG_MTP_WRITE
#define DEBUG_MTPW(fmt, args...) printk(fmt, ##args)
#else
#define DEBUG_MTPW(fmt, args...) do {} while (0)
#endif
/*-------------------------------------------------------------------------*/

#define MTPG_BULK_BUFFER_SIZE	32768
#define MTPG_INTR_BUFFER_SIZE	28

/* number of rx and tx requests to allocate */
#define MTPG_RX_REQ_MAX				8
#define MTPG_MTPG_TX_REQ_MAX		8
#define MTPG_INTR_REQ_MAX	5

/* ID for Microsoft MTP OS String */
#define MTPG_OS_STRING_ID   0xEE

#define DRIVER_NAME		 "usb_mtp_gadget"

static const char mtpg_longname[] =	"mtp";
static const char shortname[] = DRIVER_NAME;
static int mtp_pid;

/* MTP Device Structure*/
struct mtpg_dev {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	struct usb_gadget *gadget;

	spinlock_t		lock;

	u8			config;
	int			online;
	int			error;
	int			read_ready;
	struct list_head	tx_idle;
	struct list_head	rx_idle;
	struct list_head	rx_done;
	struct list_head	intr_idle;
	wait_queue_head_t	read_wq;
	wait_queue_head_t	write_wq;
	wait_queue_head_t	intr_wq;

	struct usb_request	*read_req;
	unsigned char		*read_buf;
	unsigned		read_count;

	struct usb_ep		*bulk_in;
	struct usb_ep		*bulk_out;
	struct usb_ep		*int_in;
	struct usb_request	*notify_req;

	struct workqueue_struct *wq;
	struct work_struct read_send_work;
	struct file *read_send_file;

	int64_t read_send_length;

	uint16_t read_send_cmd;
	uint32_t read_send_id;
	int read_send_result;
	atomic_t		read_excl;
	atomic_t		write_excl;
	atomic_t		ioctl_excl;
	atomic_t		open_excl;
	atomic_t		wintfd_excl;
	char cancel_io_buf[USB_PTPREQUEST_CANCELIO_SIZE+1];
	int cancel_io;
};

/* Global mtpg_dev Structure
* the_mtpg variable be used between mtpg_open() and mtpg_function_bind() */
static struct mtpg_dev    *the_mtpg;

/* Three full-speed and high-speed endpoint descriptors: bulk-in, bulk-out,
 * and interrupt-in. */

struct usb_interface_descriptor mtpg_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints          = 3,
	.bInterfaceClass        = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass     = USB_SUBCLASS_VENDOR_SPEC,
	.bInterfaceProtocol     = 0,
};

static struct usb_interface_descriptor ptp_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints          = 3,
	.bInterfaceClass        = USB_CLASS_STILL_IMAGE,
	.bInterfaceSubClass     = 1,
	.bInterfaceProtocol     = 1,
};

static struct usb_endpoint_descriptor fs_mtpg_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	/* wMaxPacketSize set by autoconfiguration */
};

static struct usb_endpoint_descriptor fs_mtpg_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	/* wMaxPacketSize set by autoconfiguration */
};

static struct usb_endpoint_descriptor int_fs_notify_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= __constant_cpu_to_le16(MTPG_INTR_BUFFER_SIZE),
	.bInterval =		6,
};

static struct usb_descriptor_header *fs_mtpg_desc[] = {
	(struct usb_descriptor_header *) &mtpg_interface_desc,
	(struct usb_descriptor_header *) &fs_mtpg_in_desc,
	(struct usb_descriptor_header *) &fs_mtpg_out_desc,
	(struct usb_descriptor_header *) &int_fs_notify_desc,
	NULL,
};

static struct usb_endpoint_descriptor hs_mtpg_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	/*bEndpointAddress copied from fs_mtpg_in_desc
			during mtpg_function_bind()*/
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_mtpg_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	/*bEndpointAddress copied from fs_mtpg_out_desc
			during mtpg_function_bind()*/
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
	.bInterval =		1,	/* NAK every 1 uframe */
};

static struct usb_endpoint_descriptor int_hs_notify_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize = __constant_cpu_to_le16(MTPG_INTR_BUFFER_SIZE),
	.bInterval =		6,
};

static struct usb_descriptor_header *hs_mtpg_desc[] = {
	(struct usb_descriptor_header *) &mtpg_interface_desc,
	(struct usb_descriptor_header *) &hs_mtpg_in_desc,
	(struct usb_descriptor_header *) &hs_mtpg_out_desc,
	(struct usb_descriptor_header *) &int_hs_notify_desc,
	NULL
};

static struct usb_descriptor_header *fs_ptp_descs[] = {
	(struct usb_descriptor_header *) &ptp_interface_desc,
	(struct usb_descriptor_header *) &fs_mtpg_in_desc,
	(struct usb_descriptor_header *) &fs_mtpg_out_desc,
	(struct usb_descriptor_header *) &int_fs_notify_desc,
	NULL,
};

static struct usb_descriptor_header *hs_ptp_descs[] = {
	(struct usb_descriptor_header *) &ptp_interface_desc,
	(struct usb_descriptor_header *) &hs_mtpg_in_desc,
	(struct usb_descriptor_header *) &hs_mtpg_out_desc,
	(struct usb_descriptor_header *) &int_hs_notify_desc,
	NULL,
};

/* string IDs are assigned dynamically */
#define F_MTP_IDX			0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2

/* default serial number takes at least two packets */
static const char serial[] = "0123456789.0123456789.0123456789";

static struct usb_string strings_dev_mtp[] = {
	[F_MTP_IDX].s = "MTP",
	[STRING_PRODUCT_IDX].s = mtpg_longname,
	[STRING_SERIAL_IDX].s = serial,
	{  },			/* end of list */
};

static struct usb_gadget_strings stringtab_mtp = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev_mtp,
};

static struct usb_gadget_strings *mtpg_dev_strings[] = {
	&stringtab_mtp,
	NULL,
};

/* Microsoft MTP OS String */
static u8 mtpg_os_string[] = {
	18, /* sizeof(mtpg_os_string) */
	USB_DT_STRING,
	/* Signature field: "MSFT100" */
	'M', 0, 'S', 0, 'F', 0, 'T', 0, '1', 0, '0', 0, '0', 0,
	/* vendor code */
	1,
	/* padding */
	0
};

/* Microsoft Extended Configuration Descriptor Header Section */
struct mtpg_ext_config_desc_header {
	__le32	dwLength;
	__u16	bcdVersion;
	__le16	wIndex;
	__u8	bCount;
	__u8	reserved[7];
};

/* Microsoft Extended Configuration Descriptor Function Section */
struct mtpg_ext_config_desc_function {
	__u8	bFirstInterfaceNumber;
	__u8	bInterfaceCount;
	__u8	compatibleID[8];
	__u8	subCompatibleID[8];
	__u8	reserved[6];
};

/* MTP Extended Configuration Descriptor */
struct {
	struct mtpg_ext_config_desc_header	header;
	struct mtpg_ext_config_desc_function	function;
} mtpg_ext_config_desc = {
	.header = {
		.dwLength = __constant_cpu_to_le32
					(sizeof(mtpg_ext_config_desc)),
		.bcdVersion = __constant_cpu_to_le16(0x0100),
		.wIndex = __constant_cpu_to_le16(4),
		.bCount = __constant_cpu_to_le16(1),
	},
	.function = {
		.bFirstInterfaceNumber = 0,
		.bInterfaceCount = 1,
		.compatibleID = { 'M', 'T', 'P' },
	},
};

/* Function  : Change config for multi configuration
 * Parameter : int conf_num (config number)
 *             0 - use mtp only without Samsung USB Driver
 *             1 - use mtp + acm with Samsung USB Driver
 * Description
 *	Below function is for samsung multi configuration
 *	feature made by soonyong,cho.
 *	Please add below handler to set_config_desc of function.
 * Date : 2011-08-03
 */
static int mtp_set_config_desc(int conf_num)
{
	switch (conf_num) {
	case 0:
		mtpg_interface_desc.bInterfaceClass =
			USB_CLASS_VENDOR_SPEC;
		mtpg_interface_desc.bInterfaceSubClass =
			USB_SUBCLASS_VENDOR_SPEC;
		mtpg_interface_desc.bInterfaceProtocol =
			0x0;
		break;
	case 1:
		mtpg_interface_desc.bInterfaceClass =
			USB_CLASS_STILL_IMAGE;
		mtpg_interface_desc.bInterfaceSubClass =
			0x01;
		mtpg_interface_desc.bInterfaceProtocol =
			0x01;
		break;

	}
	return 1;
}

/* -------------------------------------------------------------------------
 *	Main Functionalities Start!
 * ------------------------------------------------------------------------- */
static inline struct mtpg_dev *mtpg_func_to_dev(struct usb_function *f)
{
	return container_of(f, struct mtpg_dev, function);
}

static inline int _lock(atomic_t *excl)
{

	DEBUG_MTPB("[%s] \tline = [%d]\n", __func__, __LINE__);

	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void _unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

/* add a request to the tail of a list */
static void mtpg_req_put(struct mtpg_dev *dev, struct list_head *head,
						struct usb_request *req)
{
	unsigned long flags;

	DEBUG_MTPB("[%s] \tline = [%d]\n", __func__, __LINE__);

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
static struct usb_request *mtpg_req_get(struct mtpg_dev *dev,
						struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	DEBUG_MTPB("[%s] \tline = [%d]\n", __func__, __LINE__);

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);

	return req;
}

static int mtp_send_signal(int value)
{
	int ret;
	struct siginfo info;
	struct task_struct *t;
	memset(&info, 0, sizeof(struct siginfo));
	info.si_signo = SIG_SETUP;
	info.si_code = SI_QUEUE;
	info.si_int = value;
	rcu_read_lock();

	if  (!current->nsproxy) {
		printk(KERN_DEBUG "process has gone\n");
		rcu_read_unlock();
		return -ENODEV;
	}

	t = pid_task(find_vpid(mtp_pid), PIDTYPE_PID);

	if (t == NULL) {
		printk(KERN_DEBUG "no such pid\n");
		rcu_read_unlock();
		return -ENODEV;
	}

	rcu_read_unlock();
	/*send the signal*/
	ret = send_sig_info(SIG_SETUP, &info, t);
	if (ret < 0) {
		printk(KERN_ERR "[%s]error sending signal\n", __func__);
		return ret;
	}
	return 0;

}

static int mtpg_open(struct inode *ip, struct file *fp)
{
	printk(KERN_DEBUG "[%s]\tline = [%d]\n", __func__, __LINE__);

	if (_lock(&the_mtpg->open_excl)) {
		printk(KERN_ERR "mtpg_open fn mtpg device busy\n");
		return -EBUSY;
	}

	fp->private_data = the_mtpg;

	/* clear the error latch */

	DEBUG_MTPB("[%s] mtpg_open and clearing the error = 0\n", __func__);

	the_mtpg->error = 0;

	return 0;
}

static ssize_t mtpg_read(struct file *fp, char __user *buf,
				size_t count, loff_t *pos)
{
	struct mtpg_dev *dev = fp->private_data;
	struct usb_request *req;
	int r = count, xfer;
	int ret;

	DEBUG_MTPR("[%s] and count = (%d)\n", __func__, count);

	if (_lock(&dev->read_excl))
		return -EBUSY;

	while (!((dev->online || dev->error) && dev->read_ready)) {
		DEBUG_MTPR("[%s] and line is = %d\n", __func__, __LINE__);
		ret = wait_event_interruptible(dev->read_wq,
			((dev->online || dev->error) && dev->read_ready));
		if (ret < 0) {
			_unlock(&dev->read_excl);
			printk(KERN_DEBUG "[%s]line is = %d,mtp_read ret<0\n",
							__func__, __LINE__);
			return ret;
		}
	}

	while (count > 0) {
		DEBUG_MTPR("[%s] and line is = %d\n", __func__, __LINE__);

		if (dev->error) {
			r = -EIO;
			printk(KERN_ERR "[%s]\t%d:dev->error so break r=%d\n",
						__func__, __LINE__, r);
			break;
		}

		/* if we have idle read requests, get them queued */
		DEBUG_MTPR("[%s]\t%d: get request\n", __func__, __LINE__);
		while ((req = mtpg_req_get(dev, &dev->rx_idle))) {
requeue_req:
			req->length = MTPG_BULK_BUFFER_SIZE;
			DEBUG_MTPR("[%s]\t%d:usb-ep-queue\n",
						__func__, __LINE__);
			ret = usb_ep_queue(dev->bulk_out, req, GFP_ATOMIC);

			DEBUG_MTPR("[%s]\t%d:Endpoint: %s\n",
				__func__, __LINE__, dev->bulk_out->name);

			if (ret < 0) {
				r = -EIO;
				dev->error = 1;
				mtpg_req_put(dev, &dev->rx_idle, req);
				printk(KERN_ERR "[%s]line[%d]FAIL r=%d\n",
						__func__, __LINE__, r);
				goto fail;
			} else {
				DEBUG_MTPR("[%s]rx req queue%p\n",
							 __func__, req);
			}
		}

		DEBUG_MTPR("[%s]\t%d:read_count = %d\n",
				__func__, __LINE__, dev->read_count);

		/* if we have data pending, give it to userspace */
		if (dev->read_count > 0) {
			DEBUG_MTPR("[%s]\t%d: read_count = %d\n",
					__func__, __LINE__, dev->read_count);
			if (dev->read_count < count)
				xfer = dev->read_count;
			else
				xfer = count;

			DEBUG_MTPR("[%s]copy_to_user 0x%x bytes on EP %p\n",
				__func__, dev->read_count, dev->bulk_out);

			if (copy_to_user(buf, dev->read_buf, xfer)) {
				r = -EFAULT;
				printk(KERN_ERR "[%s]%d:cpytouer fail r=%d\n",
						__func__, __LINE__, r);
				break;
			}

			dev->read_buf += xfer;
			dev->read_count -= xfer;
			buf += xfer;
			count -= xfer;

			/* if we've emptied the buffer, release the request */
			if (dev->read_count == 0) {
				DEBUG_MTPR("[%s] and line is = %d\n",
							__func__, __LINE__);
				mtpg_req_put(dev, &dev->rx_idle, dev->read_req);
				dev->read_req = 0;
			}

			/*Updating the buffer size and returnung
							from mtpg_read */
			r = xfer;
			DEBUG_MTPR("[%s] \t %d: returning lenght %d\n",
						__func__, __LINE__, r);
			goto fail;
		}

		/* wait for a request to complete */
		req = 0;
		DEBUG_MTPR("[%s] and line is = %d\n", __func__, __LINE__);
		ret = wait_event_interruptible(dev->read_wq,
				 ((req = mtpg_req_get(dev, &dev->rx_done))
							 || dev->error));
		DEBUG_MTPR("[%s]\t%d: dev->error %d and req = %p\n",
				 __func__, __LINE__, dev->error, req);

		if (req != 0) {
			/* if we got a 0-len one we need to put it back into
			** service.  if we made it the current read req we'd
			** be stuck forever
			*/
			if (req->actual == 0)
				goto requeue_req;

			dev->read_req = req;
			dev->read_count = req->actual;
			dev->read_buf = req->buf;

			DEBUG_MTPR("[%s]\t%d: rx_req=%p req->actual=%d\n",
					__func__, __LINE__, req, req->actual);
		}

		if (ret < 0) {
			r = ret;
			printk(KERN_DEBUG "[%s]\t%d after ret=%d brk ret=%d\n",
						 __func__, __LINE__, ret, r);
			break;
		}
	}

fail:
	_unlock(&dev->read_excl);
	DEBUG_MTPR("[%s]\t%d: RETURNING Back to USpace r=%d\n",
						 __func__, __LINE__, r);
	return r;

}

static ssize_t mtpg_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	struct mtpg_dev *dev = fp->private_data;
	struct usb_request *req = 0;
	int r = count, xfer;
	int ret;


	DEBUG_MTPW("[%s] \t%d ep bulk_out name = %s\n",
			__func__, __LINE__ , dev->bulk_out->name);

	if (_lock(&dev->write_excl))
			return -EBUSY;

	while (count > 0) {
		if (dev->error) {
			r = -EIO;
			printk(KERN_DEBUG "[%s]%d count>0 dev->error so brk\n",
							 __func__, __LINE__);
			break;
		}

		/* get an idle tx request to use */
		req = 0;
		ret = wait_event_interruptible(dev->write_wq,
				((req = mtpg_req_get(dev, &dev->tx_idle))
							|| dev->error));

		if (ret < 0) {
			r = ret;
			printk(KERN_DEBUG "[%s]\t%d ret = %d\n",
						 __func__, __LINE__, r);
			break;
		}

		if (req != 0) {
			if (count > MTPG_BULK_BUFFER_SIZE)
				xfer = MTPG_BULK_BUFFER_SIZE;
			else
				xfer = count;

			DEBUG_MTPW("[%s]\t%d copy_from_user length %d\n",
						__func__, __LINE__, xfer);

			if (copy_from_user(req->buf, buf, xfer)) {
				printk(KERN_ERR "mtpwrite cpyfrmusr error\n");
				r = -EFAULT;
				break;
			}

			req->length = xfer;
			ret = usb_ep_queue(dev->bulk_in, req, GFP_ATOMIC);
			if (ret < 0) {
				dev->error = 1;
				r = -EIO;
			printk(KERN_ERR "[%s]\t%d ep_que ret=%d brk ret=%d\n",
						__func__, __LINE__, ret, r);
				break;
			}

			buf += xfer;
			count -= xfer;

			/* zero this so we don't try to free it on error exit */
			req = 0;
			}
	}

	if (req) {
		DEBUG_MTPW("[%s] \t%d  mtpg_req_put\n", __func__, __LINE__);
		mtpg_req_put(dev, &dev->tx_idle, req);
	}

	_unlock(&dev->write_excl);

	DEBUG_MTPW("[%s]\t%d  RETURN back to USpace r=%d\n",
					 __func__, __LINE__, r);
	return r;
}
/*
static void interrupt_complete(struct usb_ep *ep, struct usb_request *req)
{
	printk(KERN_DEBUG "Finished Writing Interrupt Data\n");
}
*/
static ssize_t interrupt_write(struct file *fd,
			const char __user *buf, size_t count)
{
	struct mtpg_dev *dev = fd->private_data;
	struct usb_request *req = 0;
	int  ret;

	DEBUG_MTPB("[%s] \tline = [%d]\n", __func__, __LINE__);

	if (count > MTPG_INTR_BUFFER_SIZE)
			return -EINVAL;

	ret = wait_event_interruptible_timeout(dev->intr_wq,
		(req = mtpg_req_get(dev, &dev->intr_idle)),
						msecs_to_jiffies(1000));

	if (!req) {
		printk(KERN_ERR "[%s]Alloc has failed\n", __func__);
		return -ENOMEM;
	}

	if (copy_from_user(req->buf, buf, count)) {
		mtpg_req_put(dev, &dev->intr_idle, req);
		printk(KERN_ERR "[%s]copy from user has failed\n", __func__);
		return -EIO;
	}

	req->length = count;
	/*req->complete = interrupt_complete;*/

	ret = usb_ep_queue(dev->int_in, req, GFP_ATOMIC);

	if (ret) {
		printk(KERN_ERR "[%s:%d]\n", __func__, __LINE__);
		mtpg_req_put(dev, &dev->intr_idle, req);
	}

	DEBUG_MTPB("[%s] \tline = [%d] returning ret is %d\\n",
						__func__, __LINE__, ret);
	return ret;
}

static void read_send_work(struct work_struct *work)
{
	struct mtpg_dev	*dev = container_of(work, struct mtpg_dev,
							read_send_work);
	//struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req = 0;
	struct usb_container_header *hdr;
	struct file *file;
	loff_t file_pos = 0;
	int64_t count = 0;
	int xfer = 0;
	int ret = -1;
	int hdr_length = 0;
	int r = 0;
	int ZLP_flag = 0;

	/* read our parameters */
	smp_rmb();
	file = dev->read_send_file;
	count = dev->read_send_length;
	hdr_length = sizeof(struct usb_container_header);
	count += hdr_length;

	printk(KERN_DEBUG "[%s:%d] offset=[%lld]\t leth+hder=[%lld]\n",
					 __func__, __LINE__, file_pos, count);

	/* Zero Length Packet should be sent if the last trasfer
	 * size is equals to the max packet size.
	 */
	if ((count & (dev->bulk_in->maxpacket - 1)) == 0)
		ZLP_flag = 1;

	while (count > 0 || ZLP_flag) {
		/*Breaking the loop after sending Zero Length Packet*/
		if (count == 0)
			ZLP_flag = 0;

		if (dev->cancel_io == 1) {
			dev->cancel_io = 0; /*reported to user space*/
			r = -EIO;
			printk(KERN_DEBUG "[%s]\t%d ret = %d\n",
						__func__, __LINE__, r);
			break;
		}
		/* get an idle tx request to use */
		req = 0;
		ret = wait_event_interruptible(dev->write_wq,
				((req = mtpg_req_get(dev, &dev->tx_idle))
							|| dev->error));
		if (ret < 0 || !req) {
			r = ret;
			printk(KERN_DEBUG "[%s]\t%d ret = %d\n",
						__func__, __LINE__, r);
			break;
		}

		if (!req) {
			printk(KERN_ERR "[%s]Alloc has failed\n", __func__);
			break;
		}

		if (count > MTPG_BULK_BUFFER_SIZE)
			xfer = MTPG_BULK_BUFFER_SIZE;
		else
			xfer = count;

		if (hdr_length) {
			hdr = (struct usb_container_header *)req->buf;
			hdr->Length = __cpu_to_le32(count);
			hdr->Type = __cpu_to_le16(2);
			hdr->Code = __cpu_to_le16(dev->read_send_cmd);
			hdr->TransactionID = __cpu_to_le32(dev->read_send_id);
		}

		ret = vfs_read(file, req->buf + hdr_length,
					xfer - hdr_length, &file_pos);
		if (ret < 0 || !req) {
			r = ret;
			break;
		}
		xfer = ret + hdr_length;
		hdr_length = 0;

		req->length = xfer;
		ret = usb_ep_queue(dev->bulk_in, req, GFP_KERNEL);
		if (ret < 0 || !req) {
			dev->error = 1;
			r = -EIO;
			printk(KERN_DEBUG "[%s]\t%d ret = %d\n",
						 __func__, __LINE__, r);
			break;
		}

		count -= xfer;

		req = 0;
	}

	if (req)
		mtpg_req_put(dev, &dev->tx_idle, req);

	DEBUG_MTPB("[%s] \tline = [%d] \t r = [%d]\n", __func__, __LINE__, r);

	dev->read_send_result = r;
	smp_wmb();
}

static long  mtpg_ioctl(struct file *fd, unsigned int code, unsigned long arg)
{
	struct mtpg_dev		*dev = fd->private_data;
	struct usb_composite_dev *cdev;
	struct usb_request	*req;
	int status = 0;
	int size = 0;
	int ret_value = 0;
	int max_pkt = 0;
	char *buf_ptr = NULL;
	char buf[USB_PTPREQUEST_GETSTATUS_SIZE+1] = {0};

	cdev = dev->cdev;
	if (!cdev) {
		printk(KERN_ERR "usb: %s cdev not ready\n", __func__);
		return -EAGAIN;
	}
	req = cdev->req;
	if (!cdev->req) {
		printk(KERN_ERR "usb: %s cdev->req not ready\n", __func__);
		return -EAGAIN;
	}

	DEBUG_MTPB("[%s] \tline = [%d]\n", __func__, __LINE__);

	switch (code) {
	case MTP_ONLY_ENABLE:
		printk(KERN_DEBUG "[%s:%d] MTP_ONLY_ENABLE ioctl:\n",
							 __func__, __LINE__);
		if (dev->cdev && dev->cdev->gadget) {
			usb_gadget_disconnect(cdev->gadget);
			printk(KERN_DEBUG "[%s:%d] B4 disconectng gadget\n",
							__func__, __LINE__);
			msleep(20);
			usb_gadget_connect(cdev->gadget);
			printk(KERN_DEBUG "[%s:%d] after usb_gadget_connect\n",
							__func__, __LINE__);
		}
		status = 10;
		printk(KERN_DEBUG "[%s:%d] MTP_ONLY_ENABLE clearing error 0\n",
							__func__, __LINE__);
		the_mtpg->error = 0;
		break;
	case MTP_DISABLE:
		/*mtp_function_enable(mtp_disable_desc);*/
		if (dev->cdev && dev->cdev->gadget) {
			usb_gadget_disconnect(dev->cdev->gadget);
			mdelay(5);
			usb_gadget_connect(dev->cdev->gadget);
		}
		break;
	case MTP_CLEAR_HALT:
		status = usb_ep_clear_halt(dev->bulk_in);
		status = usb_ep_clear_halt(dev->bulk_out);
		break;
	case MTP_WRITE_INT_DATA:
		printk(KERN_INFO "[%s]\t%d MTP intrpt_Write no slep\n",
						__func__, __LINE__);
		ret_value = interrupt_write(fd, (const char *)arg,
					MTP_MAX_PACKET_LEN_FROM_APP);
		if (ret_value < 0) {
			printk(KERN_ERR "[%s]\t%d interptFD failed\n",
							 __func__, __LINE__);
			status = -EIO;
		} else {
			printk(KERN_DEBUG "[%s]\t%d intruptFD suces\n",
							 __func__, __LINE__);
			status = MTP_MAX_PACKET_LEN_FROM_APP;
		}
		break;

	case SET_MTP_USER_PID:
		mtp_pid = arg;
		printk(KERN_DEBUG "[%s]SET_MTP_USER_PID;pid=%d\tline=[%d]\n",
						 __func__, mtp_pid, __LINE__);
		break;

	case GET_SETUP_DATA:
		buf_ptr = (char *)arg;
		printk(KERN_DEBUG "[%s] GET_SETUP_DATA\tline = [%d]\n",
						__func__, __LINE__);
		if (copy_to_user(buf_ptr, dev->cancel_io_buf,
				USB_PTPREQUEST_CANCELIO_SIZE)) {
			status = -EIO;
			printk(KERN_ERR "[%s]\t%d:coptousr failed\n",
							 __func__, __LINE__);
		}
		break;

	case SEND_RESET_ACK:
		/*req->zero = 1;*/
		req->length = 0;
		/*printk(KERN_DEBUG "[%s]SEND_RESET_ACK and usb_ep_queu
				ZERO data size = %d\tline=[%d]\n",
					__func__, size, __LINE__);*/
		status = usb_ep_queue(cdev->gadget->ep0,
						req, GFP_ATOMIC);
		if (status < 0)
			printk(KERN_ERR "[%s]ep_queue line = [%d]\n",
							 __func__, __LINE__);
		break;

	case SET_SETUP_DATA:
		buf_ptr = (char *)arg;
		if (copy_from_user(buf, buf_ptr,
				USB_PTPREQUEST_GETSTATUS_SIZE)) {
			status = -EIO;
			printk(KERN_ERR "[%s]\t%d:copyfrmuser fail\n",
							 __func__, __LINE__);
			break;
		}
		size = buf[0];
		printk(KERN_DEBUG "[%s]SET_SETUP_DATA size=%d line=[%d]\n",
						 __func__, size, __LINE__);
		memcpy(req->buf, buf, size);
		req->zero = 0;
		req->length = size;
		status = usb_ep_queue(cdev->gadget->ep0, req,
							GFP_ATOMIC);
		if (status < 0)
			printk(KERN_ERR "[%s]usbepqueue line=[%d]\n",
							 __func__, __LINE__);
		break;

	case SET_ZLP_DATA:
		/*req->zero = 1;*/
		req = mtpg_req_get(dev, &dev->tx_idle);
		if (!req) {
			printk(KERN_DEBUG "[%s] Failed to get ZLP_DATA\n",
						 __func__);
			return -EAGAIN;
		}
		req->length = 0;
		printk(KERN_DEBUG "[%s]ZLP_DATA data=%d\tline=[%d]\n",
						 __func__, size, __LINE__);
		status = usb_ep_queue(dev->bulk_in, req, GFP_ATOMIC);
		if (status < 0) {
			printk(KERN_ERR "[%s]usbepqueue line=[%d]\n",
							 __func__, __LINE__);
		} else {
			printk(KERN_DEBUG "%sZLPstatus=%d\tline=%d\n",
						__func__, __LINE__, status);
			status = 20;
		}
		break;

	case GET_HIGH_FULL_SPEED:
		printk(KERN_DEBUG "[%s]GET_HIGH_FULLSPEED line=[%d]\n",
							 __func__, __LINE__);
		max_pkt = dev->bulk_in->maxpacket;
		printk(KERN_DEBUG "[%s] line = %d max_pkt = [%d]\n",
						 __func__, __LINE__, max_pkt);
		if (max_pkt == 64)
			status = 64;
		else
			status = 512;
		break;
	case SEND_FILE_WITH_HEADER:
	{
		struct read_send_info	info;
		struct work_struct *work;
		struct file *file = NULL;
		printk(KERN_DEBUG "[%s]SEND_FILE_WITH_HEADER line=[%d]\n",
							__func__, __LINE__);

		if (copy_from_user(&info, (void __user *)arg, sizeof(info))) {
			status = -EFAULT;
			goto exit;
		}

		file = fget(info.Fd);
		if (!file) {
			status = -EBADF;
			printk(KERN_DEBUG "[%s] line=[%d] bad file number\n",
							__func__, __LINE__);
			goto exit;
		}

		dev->read_send_file = file;
		dev->read_send_length = info.Length;
		smp_wmb();

		work = &dev->read_send_work;
		dev->read_send_cmd = info.Code;
		dev->read_send_id = info.TransactionID;
		queue_work(dev->wq, work);
		/* Wait for the work to be complted on work queue */
		flush_workqueue(dev->wq);

		fput(file);

		smp_rmb();
		status = dev->read_send_result;
		break;
	}
	case MTP_VBUS_DISABLE:
		printk(KERN_DEBUG "[%s] line=[%d] \n",
							__func__, __LINE__);
		if (dev->cdev && dev->cdev->gadget) {
			usb_gadget_vbus_disconnect(cdev->gadget);
			printk(KERN_DEBUG "Restricted policy so disconnecting mtp gadget\n");
		}
		break;
	default:
		status = -ENOTTY;
	}
exit:
	return status;
}

static int mtpg_release_device(struct inode *ip, struct file *fp)
{
	printk(KERN_DEBUG "[%s]\tline = [%d]\n", __func__, __LINE__);
	if (the_mtpg != NULL)
		_unlock(&the_mtpg->open_excl);
	return 0;
}

/* file operations for MTP device /dev/usb_mtp_gadget */
static const struct file_operations mtpg_fops = {
	.owner   = THIS_MODULE,
	.read    = mtpg_read,
	.write   = mtpg_write,
	.open    = mtpg_open,
	.unlocked_ioctl = mtpg_ioctl,
	.release = mtpg_release_device,
};

static struct miscdevice mtpg_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = shortname,
	.fops = &mtpg_fops,
};

struct usb_request *alloc_ep_req(struct usb_ep *ep,
			unsigned len, gfp_t kmalloc_flags)
{
	struct usb_request	*req;

	DEBUG_MTPB("[%s] \tline = [%d]\n", __func__, __LINE__);
	req = usb_ep_alloc_request(ep, GFP_ATOMIC);
	if (req) {
		req->length = len;
		req->buf = kmalloc(len, GFP_ATOMIC);
		if (!req->buf) {
			usb_ep_free_request(ep, req);
			req = NULL;
		}
	}
	return req;
}

static void mtpg_request_free(struct usb_request *req, struct usb_ep *ep)
{

	DEBUG_MTPB("[%s] \tline = [%d]\n", __func__, __LINE__);
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static struct usb_request *mtpg_request_new(struct usb_ep *ep, int buffer_size)
{

	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);

	DEBUG_MTPB("[%s] \tline = [%d]\n", __func__, __LINE__);
	if (!req) {
		printk(KERN_ERR "[%s]\tline %d ERROR\n", __func__, __LINE__);
		return NULL;
	}

	/* now allocate buffers for the requests */
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void mtpg_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct mtpg_dev *dev = the_mtpg;
	DEBUG_MTPB("[%s]\t %d req->status is = %d\n",
			__func__, __LINE__, req->status);

	if (req->status != 0)
		dev->error = 1;

	mtpg_req_put(dev, &dev->tx_idle, req);
	wake_up(&dev->write_wq);
}

static void mtpg_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct mtpg_dev *dev = the_mtpg;

	DEBUG_MTPB("[%s]\tline = [%d]req->status is = %d\n",
				__func__, __LINE__, req->status);
	if (req->status != 0) {
		dev->error = 1;

		DEBUG_MTPB("[%s]\t%d dev->error is=%d for rx_idle\n",
					 __func__, __LINE__, dev->error);
		mtpg_req_put(dev, &dev->rx_idle, req);
	} else {
		DEBUG_MTPB("[%s]\t%d for rx_done\n", __func__, __LINE__);
		mtpg_req_put(dev, &dev->rx_done, req);
	}
	wake_up(&dev->read_wq);
}

static void mtpg_complete_intr(struct usb_ep *ep, struct usb_request *req)
{
	struct mtpg_dev *dev = the_mtpg;
	/*printk(KERN_INFO "[%s]\tline = [%d]\n", __func__, __LINE__);*/

	if (req->status != 0)
		dev->error = 1;

	mtpg_req_put(dev, &dev->intr_idle, req);

	wake_up(&dev->intr_wq);
}

static void
mtpg_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct mtpg_dev	*dev = mtpg_func_to_dev(f);
	struct usb_request *req;

	printk(KERN_DEBUG "[%s]\tline = [%d]\n", __func__, __LINE__);

	while ((req = mtpg_req_get(dev, &dev->rx_idle)))
		mtpg_request_free(req, dev->bulk_out);

	while ((req = mtpg_req_get(dev, &dev->tx_idle)))
		mtpg_request_free(req, dev->bulk_in);

	while ((req = mtpg_req_get(dev, &dev->intr_idle)))
		mtpg_request_free(req, dev->int_in);
}

static int
mtpg_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev	= c->cdev;
	struct mtpg_dev	*mtpg	= mtpg_func_to_dev(f);
	struct usb_request	*req;
	struct usb_ep		*ep;
	int			i, id;

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */

	printk(KERN_DEBUG "[%s]\tline = [%d]\n", __func__, __LINE__);

	id = usb_interface_id(c, f);
	if (id < 0) {
		printk(KERN_ERR "[%s]Error in usb_interface_id\n", __func__);
		return id;
	}

	mtpg_interface_desc.bInterfaceNumber = id;

	ep = usb_ep_autoconfig(cdev->gadget, &fs_mtpg_in_desc);
	if (!ep) {
		printk(KERN_ERR "[%s]Error usb_ep_autoconfig IN\n", __func__);
		goto autoconf_fail;
	}
	ep->driver_data = mtpg;		/* claim the endpoint */
	mtpg->bulk_in = ep;
	the_mtpg->bulk_in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, &fs_mtpg_out_desc);
	if (!ep) {
		printk(KERN_ERR "[%s]Eror usb_ep_autoconfig OUT\n", __func__);
		goto autoconf_fail;
	}
	ep->driver_data = mtpg;		/* claim the endpoint */
	mtpg->bulk_out = ep;
	the_mtpg->bulk_out = ep;

	/* Interrupt Support for MTP */
	ep = usb_ep_autoconfig(cdev->gadget, &int_fs_notify_desc);
	if (!ep) {
		printk(KERN_ERR "[%s]Eror usb_ep_autoconfig INT\n", __func__);
		goto autoconf_fail;
	}
	ep->driver_data = mtpg;
	mtpg->int_in = ep;
	the_mtpg->int_in = ep;

	for (i = 0; i < MTPG_INTR_REQ_MAX; i++) {
		req = mtpg_request_new(mtpg->int_in, MTPG_INTR_BUFFER_SIZE);
		if (!req)
			goto out;
		req->complete = mtpg_complete_intr;
		mtpg_req_put(mtpg, &mtpg->intr_idle, req);
	}
	for (i = 0; i < MTPG_RX_REQ_MAX; i++) {
		req = mtpg_request_new(mtpg->bulk_out, MTPG_BULK_BUFFER_SIZE);
		if (!req)
			goto out;
		req->complete = mtpg_complete_out;
		mtpg_req_put(mtpg, &mtpg->rx_idle, req);
	}

	for (i = 0; i < MTPG_MTPG_TX_REQ_MAX; i++) {
		req = mtpg_request_new(mtpg->bulk_in, MTPG_BULK_BUFFER_SIZE);
		if (!req)
			goto out;
		req->complete = mtpg_complete_in;
		mtpg_req_put(mtpg, &mtpg->tx_idle, req);
	}

	if (gadget_is_dualspeed(cdev->gadget)) {

		DEBUG_MTPB("[%s]\tdual speed line = [%d]\n",
						__func__, __LINE__);

		/* Assume endpoint addresses are the same for both speeds */
		hs_mtpg_in_desc.bEndpointAddress =
				fs_mtpg_in_desc.bEndpointAddress;
		hs_mtpg_out_desc.bEndpointAddress =
				fs_mtpg_out_desc.bEndpointAddress;
		int_hs_notify_desc.bEndpointAddress =
				int_fs_notify_desc.bEndpointAddress;
	}

	mtpg->cdev = cdev;
	the_mtpg->cdev = cdev;

	return 0;

autoconf_fail:
	printk(KERN_ERR "mtpg unable to autoconfigure all endpoints\n");
	return -ENOTSUPP;
out:
	mtpg_function_unbind(c, f);
	return -1;
}

static int mtpg_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct mtpg_dev	*dev = mtpg_func_to_dev(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;

	if (dev->int_in->driver_data)
		usb_ep_disable(dev->int_in);

	ret = usb_ep_enable(dev->int_in,
			ep_choose(cdev->gadget, &int_hs_notify_desc,
						&int_fs_notify_desc));
	if (ret) {
		usb_ep_disable(dev->int_in);
		dev->int_in->driver_data = NULL;
		printk(KERN_ERR "[%s]Error in enabling INT EP\n", __func__);
		return ret;
	}
	dev->int_in->driver_data = dev;

	if (dev->bulk_in->driver_data)
		usb_ep_disable(dev->bulk_in);

	ret = usb_ep_enable(dev->bulk_in,
			ep_choose(cdev->gadget, &hs_mtpg_in_desc,
						&fs_mtpg_in_desc));
	if (ret) {
		usb_ep_disable(dev->bulk_in);
		dev->bulk_in->driver_data = NULL;
		 printk(KERN_ERR "[%s] Enable Bulk-IN EP error%d\n",
							__func__, __LINE__);
		 return ret;
	}
	dev->bulk_in->driver_data = dev;

	if (dev->bulk_out->driver_data)
		usb_ep_disable(dev->bulk_out);

	ret = usb_ep_enable(dev->bulk_out,
			ep_choose(cdev->gadget, &hs_mtpg_out_desc,
						&fs_mtpg_out_desc));
	if (ret) {
		usb_ep_disable(dev->bulk_out);
		dev->bulk_out->driver_data = NULL;
		 printk(KERN_ERR "[%s] Enable Bulk-Out EP error%d\n",
							__func__, __LINE__);
		return ret;
	}
	dev->bulk_out->driver_data = dev;

	dev->online = 1;
	dev->error = 0;
	dev->read_ready = 1;
	dev->cancel_io = 0;

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);

	return 0;
}

static void mtpg_function_disable(struct usb_function *f)
{
	struct mtpg_dev	*dev = mtpg_func_to_dev(f);

	printk(KERN_DEBUG "[%s]\tline = [%d]\n", __func__, __LINE__);
	dev->online = 0;
	dev->error = 1;

	usb_ep_disable(dev->int_in);
	dev->int_in->driver_data = NULL;

	usb_ep_disable(dev->bulk_in);
	dev->bulk_in->driver_data = NULL;

	usb_ep_disable(dev->bulk_out);
	dev->bulk_out->driver_data = NULL;

	wake_up(&dev->read_wq);
}


/*PIMA15740-2000 spec: Class specific setup request for MTP*/
static void
mtp_complete_cancel_io(struct usb_ep *ep, struct usb_request *req)
{
	int i;
	struct mtpg_dev	*dev = ep->driver_data;

	DEBUG_MTPB("[%s]\tline = [%d]\n", __func__, __LINE__);
	if (req->status != 0) {
		DEBUG_MTPB("[%s]req->status !=0\tline = [%d]\n",
						 __func__, __LINE__);
		return;
	}

	if (req->actual != USB_PTPREQUEST_CANCELIO_SIZE) {
		DEBUG_MTPB("[%s]USB_PTPREQUEST_CANCELIO_SIZE line = [%d]\n",
							__func__, __LINE__);
		usb_ep_set_halt(ep);

	} else {
		memset(dev->cancel_io_buf, 0, USB_PTPREQUEST_CANCELIO_SIZE+1);
		memcpy(dev->cancel_io_buf, req->buf,
					USB_PTPREQUEST_CANCELIO_SIZE);
		dev->cancel_io = 1;
		/*Debugging*/
		for (i = 0; i < USB_PTPREQUEST_CANCELIO_SIZE; i++)
			DEBUG_MTPB("[%s]cancel_io_buf[%d]=%x\tline = [%d]\n",
				__func__, i, dev->cancel_io_buf[i], __LINE__);
		mtp_send_signal(USB_PTPREQUEST_CANCELIO);
	}

}

static int mtp_ctrlrequest(struct usb_composite_dev *cdev,
				const struct usb_ctrlrequest *ctrl)
{
	struct mtpg_dev	*dev = the_mtpg;
	struct usb_request	*req = cdev->req;
	int signal_request = 0;
	int value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	if (ctrl->bRequestType ==
			(USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE)
			&& ctrl->bRequest == USB_REQ_GET_DESCRIPTOR
			&& (w_value >> 8) == USB_DT_STRING
			&& (w_value & 0xFF) == MTPG_OS_STRING_ID) {
		value = (w_length < sizeof(mtpg_os_string)
				? w_length : sizeof(mtpg_os_string));
		memcpy(cdev->req->buf, mtpg_os_string, value);
	if (value >= 0) {
		int rc;
		cdev->req->zero = value < w_length;
		cdev->req->length = value;

		rc = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
		if (rc < 0)
			printk(KERN_DEBUG "[%s:%d] setup queue error\n",
							__func__, __LINE__);
		}
		return value;
	} else if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR) {
		if ((ctrl->bRequest == 1 || ctrl->bRequest == 0x54 ||
			ctrl->bRequest == 0x6F || ctrl->bRequest == 0xFE)
				&& (ctrl->bRequestType & USB_DIR_IN)
				&& (w_index == 4 || w_index == 5)) {
			value = (w_length < sizeof(mtpg_ext_config_desc) ?
				w_length : sizeof(mtpg_ext_config_desc));
			memcpy(cdev->req->buf, &mtpg_ext_config_desc, value);

	if (value >= 0) {
		int rc;
		cdev->req->zero = value < w_length;
		cdev->req->length = value;
		rc = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
		if (rc < 0)
			printk(KERN_DEBUG "[%s:%d] setup queue error\n",
							__func__, __LINE__);
			}
			return value;
		}
		printk(KERN_DEBUG "mtp_ctrlrequest "
				"%02x.%02x v%04x i%04x l%u\n",
				ctrl->bRequestType, ctrl->bRequest,
				w_value, w_index, w_length);
	}

	switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_PTPREQUEST_CANCELIO:
		DEBUG_MTPB("[%s]\tline=[%d]w_v=%x, w_i=%x, w_l=%x\n",
				__func__, __LINE__, w_value, w_index, w_length);
		/* if (w_value == 0x00 && w_index ==
			mtpg_interface_desc.bInterfaceNumber
			&& w_length == 0x06) */
		 if (w_value == 0x00 && w_length == 0x06) {
			DEBUG_MTPB("[%s]PTPREQUESTCANCLIO line[%d]\n",
						__func__, __LINE__);
			value = w_length;
			cdev->gadget->ep0->driver_data = dev;
			req->complete = mtp_complete_cancel_io;
			req->zero = 0;
			req->length = value;
			value = usb_ep_queue(cdev->gadget->ep0,
						req, GFP_ATOMIC);
			if (value < 0) {
				printk(KERN_ERR "[%s:%d]Error usb_ep_queue\n",
							__func__, __LINE__);
			} else
			DEBUG_MTPB("[%s] ep-queue-sucecc line[%d]\n",
							__func__, __LINE__);
		}
		return value;
		break;

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_PTPREQUEST_RESET:
		DEBUG_MTPB("[%s] USB_PTPREQUEST_RESET\tline = [%d]\n",
						 __func__, __LINE__);
		signal_request = USB_PTPREQUEST_RESET;
		break;

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_PTPREQUEST_GETSTATUS:
		signal_request = USB_PTPREQUEST_GETSTATUS;
		break;

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
		| USB_PTPREQUEST_GETEVENT:
		signal_request = USB_PTPREQUEST_GETEVENT;
		break;

	default:
		DEBUG_MTPB("[%s] INVALID REQUEST \tline = [%d]\n",
							 __func__, __LINE__);
		return value;
		}

	value = mtp_send_signal(signal_request);
	return value;
}

static int mtp_bind_config(struct usb_configuration *c, bool ptp_config)
{
	struct mtpg_dev	*mtpg = the_mtpg;
	int		status = 0;

	if (strings_dev_mtp[F_MTP_IDX].id == 0) {
		status = usb_string_id(c->cdev);

		if (status < 0)
			return status;

			strings_dev_mtp[F_MTP_IDX].id = status;
			mtpg_interface_desc.iInterface = status;
		}

	mtpg->cdev = c->cdev;
	mtpg->function.name = mtpg_longname;
	mtpg->function.strings = mtpg_dev_strings;

	/*Test the switch */
	if (ptp_config) {
		mtpg->function.descriptors = fs_ptp_descs;
		mtpg->function.hs_descriptors = hs_ptp_descs;
	} else {
		mtpg->function.descriptors = fs_mtpg_desc;
		mtpg->function.hs_descriptors = hs_mtpg_desc;
	}

	mtpg->function.bind = mtpg_function_bind;
	mtpg->function.unbind = mtpg_function_unbind;
	mtpg->function.set_alt = mtpg_function_set_alt;
	mtpg->function.disable = mtpg_function_disable;
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	mtpg->function.set_config_desc = mtp_set_config_desc;
#endif

	return usb_add_function(c, &mtpg->function);
}

static int mtp_setup(void)
{
	struct mtpg_dev	*mtpg;
	int		rc;

	printk(KERN_DEBUG "[%s] \tline = [%d]\n", __func__, __LINE__);
	mtpg = kzalloc(sizeof(*mtpg), GFP_KERNEL);
	if (!mtpg) {
		printk(KERN_ERR "mtpg_dev_alloc memory failed\n");
		return -ENOMEM;
	}

	spin_lock_init(&mtpg->lock);
	init_waitqueue_head(&mtpg->intr_wq);
	init_waitqueue_head(&mtpg->read_wq);
	init_waitqueue_head(&mtpg->write_wq);

	atomic_set(&mtpg->open_excl, 0);
	atomic_set(&mtpg->read_excl, 0);
	atomic_set(&mtpg->write_excl, 0);
	atomic_set(&mtpg->wintfd_excl, 0);

	INIT_LIST_HEAD(&mtpg->rx_idle);
	INIT_LIST_HEAD(&mtpg->rx_done);
	INIT_LIST_HEAD(&mtpg->tx_idle);
	INIT_LIST_HEAD(&mtpg->intr_idle);
	mtpg->wq = create_singlethread_workqueue("mtp_read_send");
	if (!mtpg->wq) {
		printk(KERN_ERR "mtpg_dev_alloc work queue creation failed\n");
		rc =  -ENOMEM;
		goto err_work;
	}

	INIT_WORK(&mtpg->read_send_work, read_send_work);

	/* the_mtpg must be set before calling usb_gadget_register_driver */
	the_mtpg = mtpg;

	rc = misc_register(&mtpg_device);
	if (rc != 0) {
		printk(KERN_ERR " misc_register of mtpg Failed\n");
		goto err_misc_register;
	}

	return 0;
err_work:
err_misc_register:
	the_mtpg = NULL;
	kfree(mtpg);
	printk(KERN_ERR "mtp gadget driver failed to initialize\n");
	return rc;
}

static void mtp_cleanup(void)
{
	struct mtpg_dev	*mtpg = the_mtpg;
	printk(KERN_DEBUG "[%s:::%d]\n", __func__, __LINE__);

	if (!mtpg)
		return;

	misc_deregister(&mtpg_device);
	the_mtpg = NULL;
	kfree(mtpg);
}

MODULE_AUTHOR("Deepak And Madhukar");
MODULE_LICENSE("GPL");
