/*
 * Copyright (C) 2012 Samsung Electronics.
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

#ifndef __MODEM_LINK_PM_USB_H__
#define __MODEM_LINK_PM_USB_H__

#include <linux/platform_data/modem_na.h>
#include "modem_prj.h"
#include "modem_link_device_usb.h"

#define IOCTL_LINK_CONTROL_ENABLE	_IO('o', 0x30)
#define IOCTL_LINK_CONTROL_ACTIVE	_IO('o', 0x31)
#define IOCTL_LINK_GET_HOSTWAKE		_IO('o', 0x32)
#define IOCTL_LINK_CONNECTED		_IO('o', 0x33)
#define IOCTL_LINK_SET_BIAS_CLEAR	_IO('o', 0x34)

#define IOCTL_LINK_PORT_ON		_IO('o', 0x35)
#define IOCTL_LINK_PORT_OFF		_IO('o', 0x36)
#define IOCTL_LINK_BLOCK_AUTOSUSPEND	_IO('o', 0x37)
#define IOCTL_LINK_ENABLE_AUTOSUSPEND	_IO('o', 0x38)

enum hub_status {
	HUB_STATE_OFF,		/* usb3503 0ff*/
	HUB_STATE_RESUMMING,	/* usb3503 on, but enummerattion was not yet*/
	HUB_STATE_PREACTIVE,
	HUB_STATE_ACTIVE,	/* hub and CMC221 enumerate */
};

struct link_pm_data {
	struct miscdevice miscdev;
	struct usb_link_device *usb_ld;
	unsigned gpio_link_active;
	unsigned gpio_link_hostwake;
	unsigned gpio_link_slavewake;
	int (*link_reconnect)(void);
	int link_reconnect_cnt;

	struct workqueue_struct *wq;
	struct completion active_done;
/*USB3503*/
	struct completion hub_active;
	int hub_status;
	bool has_usbhub;
	/* ignore hub on by host wakeup irq before cp power on*/
	int hub_init_lock;
	/* C1 stay disconnect status after send 'a', skip 'a' next enumeration*/
	int hub_handshake_done;
	struct wake_lock hub_lock;
	struct delayed_work link_pm_hub;
	int hub_on_retry_cnt;
	struct device *root_hub;

	struct delayed_work link_pm_work;
	struct delayed_work link_pm_start;
	struct delayed_work link_reconnect_work;
	bool resume_requested;
	bool link_pm_active;

	struct wake_lock l2_wake;
	struct wake_lock boot_wake;
	struct notifier_block pm_notifier;
	bool dpm_suspending;

	int (*port_enable)(int, int);

	int (*cpufreq_lock)(void);
	int (*cpufreq_unlock)(void);

	int autosuspend_delay_ms; /* if zero, the default value is used */
	bool block_autosuspend;
};

bool link_pm_set_active(struct usb_link_device *usb_ld);
bool link_pm_is_connected(struct usb_link_device *usb_ld);
int link_pm_init(struct usb_link_device *usb_ld, void *data);

#endif
