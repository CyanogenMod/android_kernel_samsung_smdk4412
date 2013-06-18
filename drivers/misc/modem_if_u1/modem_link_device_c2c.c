/* /linux/drivers/new_modem_if/link_dev_c2c.c
 *
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

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/if_arp.h>
#include <linux/platform_device.h>

#include <linux/platform_data/modem.h>
#include <linux/platform_data/c2c.h>
#include "modem_prj.h"
#include "modem_link_device_c2c.h"

struct link_device *c2c_create_link_device(struct platform_device *pdev)
{
	struct c2c_link_device *dpld;
	struct link_device *ld;
	struct modem_data *pdata;

	pdata = pdev->dev.platform_data;

	dpld = kzalloc(sizeof(struct c2c_link_device), GFP_KERNEL);
	if (!dpld) {
		mif_err("dpld == NULL\n");
		return NULL;
	}

	wake_lock_init(&dpld->c2c_wake_lock, WAKE_LOCK_SUSPEND, "c2c_wakelock");
	wake_lock(&dpld->c2c_wake_lock);

	ld = &dpld->ld;
	dpld->pdata = pdata;

	ld->name = "c2c";

	mif_info("%s is created!!!\n", dpld->ld.name);

	return ld;
}
