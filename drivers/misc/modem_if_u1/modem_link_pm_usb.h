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

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_link_device_usb.h"

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
	bool hub_work_running;
	int hub_on_retry_cnt;
	struct device *root_hub;

	struct notifier_block pm_notifier;
	bool dpm_suspending;

	int (*port_enable)(int, int);

	int (*freq_lock)(struct device *dev);
	int (*freq_unlock)(struct device *dev);

	int autosuspend_delay_ms; /* if zero, the default value is used */
	bool autosuspend;
};

bool link_pm_set_active(struct usb_link_device *usb_ld);
bool link_pm_is_connected(struct usb_link_device *usb_ld);
void link_pm_preactive(struct link_pm_data *pm_data);
int link_pm_init(struct usb_link_device *usb_ld, void *data);

#endif
