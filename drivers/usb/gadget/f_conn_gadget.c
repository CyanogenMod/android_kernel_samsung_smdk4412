/*
 * Gadget Driver for Android Connectivity Gadget
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 * Copyright (C) 2013 DEVGURU CO.,LTD.
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
 * ChangeLog:
 *      20130819 - rename 'adb' to 'conn_gadget', shortname to 'android_ssusbconn'
 *      20130821 - fixed return actual read size
 *      20130822 - rework with 3.4.39 version's adb base source.
 *      20130822 - remove unused callbacks
 *      20130913 - add async read logic
 *      20130913 - use kfifo as read buffer queue
 *      20130913 - add polling handler
 *      20130914 - fix ep read condition check mistake
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>

#define CONN_GADGET_BULK_BUFFER_SIZE           4096

/* fifo size in elements (bytes) */
#define FIFO_SIZE       (CONN_GADGET_BULK_BUFFER_SIZE * 10)

/* number of tx requests to allocate */
#define TX_REQ_MAX 4

static const char conn_gadget_shortname[] = "android_ssusbcon";

struct conn_gadget_dev {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;

	int online;
	int error;

	int opened;

	atomic_t read_excl;
	atomic_t write_excl;
	atomic_t open_excl;

	struct list_head tx_idle;

	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;
	struct usb_request *rx_req;
	int rx_done;
	struct kfifo rd_queue;

	int tx_done;
};

static struct usb_interface_descriptor conn_gadget_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints          = 2,
	.bInterfaceClass        = 0xFF,
	.bInterfaceSubClass     = 0x40,
	.bInterfaceProtocol     = 1,
};

static struct usb_endpoint_descriptor conn_gadget_highspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor conn_gadget_highspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor conn_gadget_fullspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor conn_gadget_fullspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *fs_conn_gadget_descs[] = {
	(struct usb_descriptor_header *) &conn_gadget_interface_desc,
	(struct usb_descriptor_header *) &conn_gadget_fullspeed_in_desc,
	(struct usb_descriptor_header *) &conn_gadget_fullspeed_out_desc,
	NULL,
};

static struct usb_descriptor_header *hs_conn_gadget_descs[] = {
	(struct usb_descriptor_header *) &conn_gadget_interface_desc,
	(struct usb_descriptor_header *) &conn_gadget_highspeed_in_desc,
	(struct usb_descriptor_header *) &conn_gadget_highspeed_out_desc,
	NULL,
};

/* temporary variable used between conn_gadget_open() and conn_gadget_gadget_bind() */
static struct conn_gadget_dev *_conn_gadget_dev;

static inline struct conn_gadget_dev *func_to_conn_gadget(struct usb_function *f)
{
	return container_of(f, struct conn_gadget_dev, function);
}


static struct usb_request *conn_gadget_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		return NULL;

	/* now allocate buffers for the requests */
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void conn_gadget_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static inline int conn_gadget_lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void conn_gadget_unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

/* add a request to the tail of a list */
void conn_gadget_req_put(struct conn_gadget_dev *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
struct usb_request *conn_gadget_req_get(struct conn_gadget_dev *dev, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

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

static int conn_gadget_low_buffer(struct conn_gadget_dev *dev)
{
	if (kfifo_len(&dev->rd_queue) < (FIFO_SIZE / 3))
		return 1;
	return 0;
}

static int conn_gadget_request_ep_out(struct conn_gadget_dev *dev)
{
	struct usb_request *req;
	int ret = 0;

	if (conn_gadget_low_buffer(dev))
	{
		req = dev->rx_req;
		req->length = CONN_GADGET_BULK_BUFFER_SIZE;

		ret = usb_ep_queue(dev->ep_out, req, GFP_ATOMIC);
		if (ret < 0) {
			dev->error = 1;
			pr_debug("%s: error %d\n", __func__, dev->error);
			printk(KERN_ERR "%s: failed to queue req %p (%d)\n", __func__, req, ret);
			return -EINVAL;
		} else {
			pr_debug("%s: rx %p queue\n", __func__, req);
		}
	}

	return 0;
}



static void conn_gadget_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct conn_gadget_dev *dev = _conn_gadget_dev;

	pr_debug("%s: status %d\n", __func__, req->status);

	if (req->status != 0) {
		dev->error = 1;
		pr_debug("%s: error %d\n", __func__, dev->error);
		printk(KERN_INFO "%s: req->status %d\n", __func__, req->status);
	}

	conn_gadget_req_put(dev, &dev->tx_idle, req);

	wake_up(&dev->write_wq);
}

static void conn_gadget_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct conn_gadget_dev *dev = _conn_gadget_dev;
	int ret;

	pr_debug("%s: status %d, actual %d\n", __func__, req->status, req->actual);

	if (req->status != 0)
	{
		if (req->status != -ECONNRESET) {
			dev->error = 1;
			pr_debug("%s: error %d\n", __func__, dev->error);
		}

		printk(KERN_INFO "%s: req->status %d\n", __func__, req->status);
		goto done;
	}

	if (dev->online == 0) {
		dev->rx_done = 0;
		dev->error = 1;
		pr_debug("%s: error %d\n", __func__, dev->error);
	} else {
		if (req->actual == 0) {
			pr_debug("%s: got ZLP\n", __func__);

			req->length = CONN_GADGET_BULK_BUFFER_SIZE;

			ret = usb_ep_queue(dev->ep_out, req, GFP_ATOMIC);
			if (ret < 0) {
				dev->error = 1;
				pr_debug("%s: error %d\n", __func__, dev->error);
				printk(KERN_ERR "%s: failed to queue req %p (%d)\n", __func__, req, ret);
				goto done;
			} else {
				pr_debug("rx %p queue\n", req);
			}
		} else {
			kfifo_in(&dev->rd_queue, req->buf, req->actual);
			dev->rx_done = 1;
		}
	}

done:
	wake_up(&dev->read_wq);
}

static int conn_gadget_create_bulk_endpoints(struct conn_gadget_dev *dev,
				struct usb_endpoint_descriptor *in_desc,
				struct usb_endpoint_descriptor *out_desc)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req;
	struct usb_ep *ep;
	int i;

	pr_debug("create_bulk_endpoints dev: %p\n", dev);

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		printk(KERN_ERR "usb_ep_autoconfig for ep_in failed\n");
		return -ENODEV;
	}
	pr_debug("usb_ep_autoconfig for ep_in got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		printk(KERN_ERR "usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}
	pr_debug("usb_ep_autoconfig for conn_gadget ep_out got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_out = ep;

	/* now allocate requests for our endpoints */
	req = conn_gadget_request_new(dev->ep_out, CONN_GADGET_BULK_BUFFER_SIZE);
	if (!req)
		goto fail;
	req->complete = conn_gadget_complete_out;
	dev->rx_req = req;

	for (i = 0; i < TX_REQ_MAX; i++) {
		req = conn_gadget_request_new(dev->ep_in, CONN_GADGET_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = conn_gadget_complete_in;
		conn_gadget_req_put(dev, &dev->tx_idle, req);
	}

	return 0;

fail:
	printk(KERN_ERR "%s: could not allocate requests\n", __func__);
	return -1;
}

unsigned int conn_gadget_poll(struct file* fp, poll_table *wait)
{
	unsigned int mask = 0;
	struct conn_gadget_dev *dev = fp->private_data;

	if (!_conn_gadget_dev) {
		printk(KERN_ERR "%s: _conn_gadget_dev is NULL\n", __func__);
		return -ENODEV;
	}

	poll_wait(fp, &dev->read_wq, wait);
	if (dev->rx_done == 1) {
		pr_debug("%s: rd\n", __func__);
		mask |= (POLLIN | POLLRDNORM);
	}

	poll_wait(fp, &dev->write_wq, wait);
	if (dev->tx_done == 1) {
		pr_debug("%s: wr\n", __func__);
		mask |= (POLLOUT | POLLWRNORM);
	}

	pr_debug("conn_gadget_poll ok\n");
	return mask;
}

static ssize_t conn_gadget_read(struct file *fp, char __user *buf,
				size_t count, loff_t *pos)
{
	struct conn_gadget_dev *dev = fp->private_data;
	int r = count, xfer;
	int ret;
	int lock = 0;

	pr_debug("conn_gadget_read(%d)\n", count);

	if (!_conn_gadget_dev) {
		printk(KERN_ERR "%s: _conn_gadget_dev is NULL\n", __func__);
		return -ENODEV;
	}

	if (count > CONN_GADGET_BULK_BUFFER_SIZE) {
		printk(KERN_ERR "%s: count %d > BLK_BUF_SIZ %d\n", __func__, count, CONN_GADGET_BULK_BUFFER_SIZE);
		return -EINVAL;
	}

	if (kfifo_is_empty(&dev->rd_queue)) {
		if (conn_gadget_lock(&dev->read_excl)) {
			printk(KERN_INFO "%s: lock read_excl failed\n", __func__);
			return -EBUSY;
		}
		else lock = 1;

		/* we will block until we're online */
		while (!(dev->online || dev->error)) {
			pr_debug("%s: waiting for online state\n", __func__);
			ret = wait_event_interruptible(dev->read_wq,
					(dev->online || dev->error));
			if (ret < 0) {
				conn_gadget_unlock(&dev->read_excl);
				printk(KERN_ERR "%s: wait_event_interruptible(rdwq,online) failed %d\n", __func__, ret);
				return ret;
			}
		}

		if (dev->error) {
			pr_debug("%s: dev error after wait_event_interruptible\n", __func__);
			r = -EIO;
			goto done;
		}

		/* wait for a request to complete */
		ret = wait_event_interruptible(dev->read_wq, dev->rx_done);
		if (ret < 0) {
			if (ret != -ERESTARTSYS) {
				dev->error = 1;
				pr_debug("%s: error %d\n", __func__, dev->error);
			}
			r = ret;
			usb_ep_dequeue(dev->ep_out, dev->rx_req);
			printk(KERN_ERR "%s: wait_event_interruptible(rdwq,rxdone) failed %d\n", __func__, ret);
			goto done;
		}
	}

	if (!dev->error) {
		xfer = kfifo_len(&dev->rd_queue);
		pr_debug("rx %d\n", xfer);

		xfer = (xfer < count) ? xfer : count;
		ret = kfifo_to_user(&dev->rd_queue, buf, xfer, &r);
		if (ret < 0) {
			r = -EFAULT;
			printk(KERN_ERR "%s: kfifo_to_user failed\n", __func__);
			goto done;
		}
		else r = xfer;

		if (kfifo_is_empty(&dev->rd_queue)) {
			dev->rx_done = 0;

			if (conn_gadget_request_ep_out(dev) < 0) {
				printk(KERN_ERR "%s: conn_gadget_request_ep_out faild\n", __func__);
				r = -EIO;
				goto done;
			}
		}

	} else
		r = -EIO;

done:
	//if already locked, then unlock it
	if (lock) conn_gadget_unlock(&dev->read_excl);

	pr_debug("conn_gadget_read ret %d\n", r);
	return r;
}

static ssize_t conn_gadget_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	struct conn_gadget_dev *dev = fp->private_data;
	struct usb_request *req = 0;
	int r = count, xfer;
	int ret;

	if (!_conn_gadget_dev) {
		printk(KERN_ERR "%s: _conn_gadget_dev is NULL\n", __func__);
		return -ENODEV;
	}

	pr_debug("conn_gadget_write(%d)\n", count);

	if (conn_gadget_lock(&dev->write_excl)) {
		printk(KERN_INFO "%s: lock write_excl failed\n", __func__);
		return -EBUSY;
	}

	dev->tx_done = 0;

	while (count > 0) {
		pr_debug("%s: in the loop (user count %d)\n", __func__, count);

		if (dev->error) {
			r = -EIO;
			printk(KERN_INFO "%s: dev->error 1\n", __func__);
			break;
		}

		/* get an idle tx request to use */
		req = 0;
		ret = wait_event_interruptible(dev->write_wq,
			(req = conn_gadget_req_get(dev, &dev->tx_idle)) || dev->error);

		if (ret < 0) {
			r = ret;
			printk(KERN_ERR "%s: wait_event_interruptible(wrwq,reqget) failed %d\n", __func__, ret);
			break;
		}

		if (req != 0) {
			if (count > CONN_GADGET_BULK_BUFFER_SIZE)
				xfer = CONN_GADGET_BULK_BUFFER_SIZE;
			else
				xfer = count;

			if (copy_from_user(req->buf, buf, xfer)) {
				r = -EFAULT;
				printk(KERN_ERR "%s: copy_from_user failed\n", __func__);
				break;
			}

			req->length = xfer;

			ret = usb_ep_queue(dev->ep_in, req, GFP_ATOMIC);
			if (ret < 0) {
				dev->error = 1;
				pr_debug("%s: error %d\n", __func__, dev->error);
				r = -EIO;
				printk(KERN_ERR "%s: xfer error %d\n", __func__, ret);
				break;
			}

			buf += xfer;
			count -= xfer;

			/* zero this so we don't try to free it on error exit */
			req = 0;
		}
	}

	if (req) {
		pr_debug("%s: req_put\n", __func__);
		conn_gadget_req_put(dev, &dev->tx_idle, req);
	}

	conn_gadget_unlock(&dev->write_excl);
	pr_debug("conn_gadget_write ret %d\n", r);

	dev->tx_done = 1;

	return r;
}

static int conn_gadget_open(struct inode *ip, struct file *fp)
{
	struct conn_gadget_dev *dev = fp->private_data;

	pr_debug("conn_gadget_open\n");
	if (!_conn_gadget_dev)
		return -ENODEV;

	if (conn_gadget_lock(&_conn_gadget_dev->open_excl))
		return -EBUSY;

	fp->private_data = _conn_gadget_dev;
	dev = fp->private_data;

	dev->opened = 1;

	if (dev->online) {
		if (kfifo_is_empty(&dev->rd_queue)) {
			dev->rx_done = 0;
		}

		if (conn_gadget_request_ep_out(dev) < 0) {
			printk(KERN_ERR "%s: conn_gadget_request_ep_out faild\n", __func__);
		}
		dev->tx_done = 1;
	} else { dev->tx_done = 0; }

	/* clear the error latch */
	_conn_gadget_dev->error = 0;

	return 0;
}

static int conn_gadget_release(struct inode *ip, struct file *fp)
{
	struct conn_gadget_dev *dev = fp->private_data;

	pr_debug("conn_gadget_release\n");

	dev->opened = 0;

	kfifo_reset(&dev->rd_queue);

	if (dev->online) {
		usb_ep_dequeue(dev->ep_out, dev->rx_req);
	}

	conn_gadget_unlock(&_conn_gadget_dev->open_excl);
	return 0;
}

/* file operations for conn_gadget device /dev/android_conn_gadget */
static const struct file_operations conn_gadget_fops = {
	.owner = THIS_MODULE,
	.read = conn_gadget_read,
	.write = conn_gadget_write,
	.poll = conn_gadget_poll,
	.open = conn_gadget_open,
	.release = conn_gadget_release,
};

static struct miscdevice conn_gadget_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = conn_gadget_shortname,
	.fops = &conn_gadget_fops,
};




static int
conn_gadget_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct conn_gadget_dev	*dev = func_to_conn_gadget(f);
	int			id;
	int			ret;

	dev->cdev = cdev;
	pr_debug("conn_gadget_function_bind dev: %p\n", dev);

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	conn_gadget_interface_desc.bInterfaceNumber = id;

	/* allocate endpoints */
	ret = conn_gadget_create_bulk_endpoints(dev, &conn_gadget_fullspeed_in_desc,
			&conn_gadget_fullspeed_out_desc);
	if (ret)
		return ret;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		conn_gadget_highspeed_in_desc.bEndpointAddress =
			conn_gadget_fullspeed_in_desc.bEndpointAddress;
		conn_gadget_highspeed_out_desc.bEndpointAddress =
			conn_gadget_fullspeed_out_desc.bEndpointAddress;
	}

	pr_debug("%s speed %s: IN/%s, OUT/%s\n",
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			f->name, dev->ep_in->name, dev->ep_out->name);
	return 0;
}

static void
conn_gadget_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct conn_gadget_dev	*dev = func_to_conn_gadget(f);
	struct usb_request *req;


	dev->online = 0;
	dev->error = 1;
	pr_debug("%s: error %d\n", __func__, dev->error);

	wake_up(&dev->read_wq);

	conn_gadget_request_free(dev->rx_req, dev->ep_out);

	while ((req = conn_gadget_req_get(dev, &dev->tx_idle)))
		conn_gadget_request_free(req, dev->ep_in);
}

static int conn_gadget_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct conn_gadget_dev	*dev = func_to_conn_gadget(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;

	pr_debug("%s: intf: %d alt: %d\n", __func__, intf, alt);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_in);
	if (ret)
		return ret;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
	ret = usb_ep_enable(dev->ep_in,
			ep_choose(cdev->gadget,
				&conn_gadget_highspeed_in_desc,
				&conn_gadget_fullspeed_in_desc));
#else
	ret = usb_ep_enable(dev->ep_in);
#endif
	if (ret)
		return ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_out);
	if (ret)
		return ret;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
	ret = usb_ep_enable(dev->ep_out,
			ep_choose(cdev->gadget,
				&conn_gadget_highspeed_out_desc,
				&conn_gadget_fullspeed_out_desc));
#else
	ret = usb_ep_enable(dev->ep_out);
#endif
	if (ret) {
		usb_ep_disable(dev->ep_in);
		return ret;
	}
	dev->online = 1;

	if (dev->opened) {
		dev->error = 0;
		pr_debug("%s: error %d\n", __func__, dev->error);

		if (kfifo_is_empty(&dev->rd_queue)) {
			dev->rx_done = 0;
		}

		ret = conn_gadget_request_ep_out(dev);
		if (ret < 0) {
			printk(KERN_ERR "%s: conn_gadget_request_ep_out faild\n", __func__);
			return ret;
		}
		dev->tx_done = 1;
	}

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);
	return 0;
}

static void conn_gadget_function_disable(struct usb_function *f)
{
	struct conn_gadget_dev	*dev = func_to_conn_gadget(f);
	struct usb_composite_dev	*cdev = dev->cdev;

	pr_debug("conn_gadget_function_disable cdev %p\n", cdev);
	dev->online = 0;
	dev->error = 1;
	pr_debug("%s: error %d\n", __func__, dev->error);
	usb_ep_disable(dev->ep_in);
	usb_ep_disable(dev->ep_out);

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);

	pr_debug("%s disabled\n", dev->function.name);
}

static int conn_gadget_bind_config(struct usb_configuration *c)
{
	struct conn_gadget_dev *dev = _conn_gadget_dev;

	pr_debug("conn_gadget_bind_config\n");

	dev->cdev = c->cdev;
	dev->function.name = "conn_gadget";
	dev->function.descriptors = fs_conn_gadget_descs;
	dev->function.hs_descriptors = hs_conn_gadget_descs;
	dev->function.bind = conn_gadget_function_bind;
	dev->function.unbind = conn_gadget_function_unbind;
	dev->function.set_alt = conn_gadget_function_set_alt;
	dev->function.disable = conn_gadget_function_disable;

	return usb_add_function(c, &dev->function);
}

static int conn_gadget_setup(void)
{
	struct conn_gadget_dev *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;
	memset(dev, 0x00, sizeof(*dev));

	spin_lock_init(&dev->lock);

	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);

	atomic_set(&dev->open_excl, 0);
	atomic_set(&dev->read_excl, 0);
	atomic_set(&dev->write_excl, 0);

	INIT_LIST_HEAD(&dev->tx_idle);

    ret = kfifo_alloc(&dev->rd_queue, FIFO_SIZE, GFP_KERNEL);
    if (ret) {
            printk(KERN_ERR "error kfifo_alloc\n");
		    goto err;
    }

	dev->opened = 0;

	_conn_gadget_dev = dev;

	ret = misc_register(&conn_gadget_device);
	if (ret)
		goto err;

	return 0;

err:
	kfree(dev);
	printk(KERN_ERR "conn_gadget gadget driver failed to initialize\n");
	return ret;
}

static void conn_gadget_cleanup(void)
{
	misc_deregister(&conn_gadget_device);

	kfifo_free(&_conn_gadget_dev->rd_queue);

	kfree(_conn_gadget_dev);
	_conn_gadget_dev = NULL;
}

