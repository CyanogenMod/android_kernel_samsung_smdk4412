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

#ifndef __MODEM_LINK_DEVICE_USB_H__
#define __MODEM_LINK_DEVICE_USB_H__

#include <linux/usb.h>
#include <linux/wakelock.h>

#define IF_USB_DEVNUM_MAX	3

#define IF_USB_FMT_EP		0
#define IF_USB_RAW_EP		1
#define IF_USB_RFS_EP		2

#define DEFAULT_AUTOSUSPEND_DELAY_MS		500
#define HOST_WAKEUP_TIMEOUT_JIFFIES		msecs_to_jiffies(500)
#define WAIT_ENUMURATION_TIMEOUT_JIFFIES	msecs_to_jiffies(15000)
#define MAX_RETRY	30

#define IOCTL_LINK_CONTROL_ENABLE	_IO('o', 0x30)
#define IOCTL_LINK_CONTROL_ACTIVE	_IO('o', 0x31)
#define IOCTL_LINK_GET_HOSTWAKE		_IO('o', 0x32)
#define IOCTL_LINK_CONNECTED		_IO('o', 0x33)
#define IOCTL_LINK_SET_BIAS_CLEAR	_IO('o', 0x34)

#define IOCTL_LINK_PORT_ON		_IO('o', 0x35)
#define IOCTL_LINK_PORT_OFF		_IO('o', 0x36)

enum RESUME_STATUS {
	CP_INITIATED_RESUME,
	AP_INITIATED_RESUME,
};

enum IPC_INIT_STATUS {
	INIT_IPC_NOT_READY,
	INIT_IPC_START_DONE,	/* send 'a' done */
};

enum hub_status {
	HUB_STATE_OFF,		/* usb3503 0ff*/
	HUB_STATE_RESUMMING,	/* usb3503 on, but enummerattion was not yet*/
	HUB_STATE_ACTIVE,	/* hub and CMC221 enumerate */
};

struct if_usb_devdata {
	struct usb_interface *data_intf;
	unsigned int tx_pipe;
	unsigned int rx_pipe;
	u8 disconnected;

	int format;
	struct usb_anchor urbs;
	struct usb_anchor reading;
	unsigned int rx_buf_size;
};

struct usb_link_device {
	/*COMMON LINK DEVICE*/
	struct link_device ld;

	struct modem_data *pdata;

	/*USB SPECIFIC LINK DEVICE*/
	struct usb_device	*usbdev;
	struct if_usb_devdata	devdata[IF_USB_DEVNUM_MAX];
	struct delayed_work	runtime_pm_work;
	struct delayed_work	post_resume_work;
	struct delayed_work     wait_enumeration;
	struct work_struct	disconnect_work;

	struct wake_lock	gpiolock;
	struct wake_lock	susplock;

	unsigned int		dev_count;
	unsigned int		suspended;
	atomic_t		suspend_count;
	enum RESUME_STATUS	resume_status;
	int if_usb_connected;
	int if_usb_initstates;
	int flow_suspend;
	int host_wake_timeout_flag;

	unsigned gpio_slave_wakeup;
	unsigned gpio_host_wakeup;
	unsigned gpio_host_active;
	int irq_host_wakeup;
	struct delayed_work dwork;
	struct work_struct resume_work;
	int cpcrash_flag;
	wait_queue_head_t l2_wait;

	spinlock_t		lock;
	struct usb_anchor	deferred;

	/* LINK PM DEVICE DATA */
	struct link_pm_data *link_pm_data;
};
/* converts from struct link_device* to struct xxx_link_device* */
#define to_usb_link_device(linkdev) \
			container_of(linkdev, struct usb_link_device, ld)

#define SET_SLAVE_WAKEUP(_pdata, _value)			\
do {								\
	gpio_set_value(_pdata->gpio_slave_wakeup, _value);	\
	mif_debug("> S-WUP %s\n", _value ? "1" : "0");	\
} while (0)

#define SET_HOST_ACTIVE(_pdata, _value)			\
do {								\
	gpio_set_value(_pdata->gpio_host_active, _value);	\
	mif_debug("> H-ACT %s\n", _value ? "1" : "0");	\
} while (0)

#define has_hub(usb_ld) ((usb_ld)->link_pm_data->has_usbhub)

irqreturn_t usb_resume_irq(int irq, void *data);
bool usb_is_enumerated(struct modem_shared *msd);

#endif
