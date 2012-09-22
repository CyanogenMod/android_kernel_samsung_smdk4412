/* /linux/drivers/misc/modem_if/modem_modemctl_device_cbp7.1.c
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
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/platform_device.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_link_device_dpram.h"

#define PIF_TIMEOUT		(180 * HZ)
#define DPRAM_INIT_TIMEOUT	(30 * HZ)


static irqreturn_t phone_active_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	int phone_reset = gpio_get_value(mc->gpio_cp_reset);
	int phone_active = gpio_get_value(mc->gpio_phone_active);
	int phone_state = mc->phone_state;

	mif_info("state = %d, phone_reset = %d, phone_active = %d\n",
		phone_state, phone_reset, phone_active);

	if (phone_reset && phone_active) {
		if (mc->phone_state == STATE_BOOTING) {
			phone_state = STATE_ONLINE;
			mc->bootd->modem_state_changed(mc->bootd, phone_state);
		}
	} else if (phone_reset && !phone_active) {
		if (mc->phone_state == STATE_ONLINE) {
			phone_state = STATE_CRASH_EXIT;
			mc->bootd->modem_state_changed(mc->bootd, phone_state);
		}
	} else {
		phone_state = STATE_OFFLINE;
		if (mc->bootd && mc->bootd->modem_state_changed)
			mc->bootd->modem_state_changed(mc->bootd, phone_state);
	}

	if (phone_active)
		irq_set_irq_type(mc->irq_phone_active, IRQ_TYPE_LEVEL_LOW);
	else
		irq_set_irq_type(mc->irq_phone_active, IRQ_TYPE_LEVEL_HIGH);

	mif_info("phone_state = %d\n", phone_state);

	return IRQ_HANDLED;
}

static int cbp72_on(struct modem_ctl *mc)
{
	mif_info("start!!!\n");

	/* prevent sleep during bootloader downloading */
	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	gpio_set_value(mc->gpio_cp_on, 0);
	if (mc->gpio_cp_off)
		gpio_set_value(mc->gpio_cp_off, 1);
	gpio_set_value(mc->gpio_cp_reset, 0);

	msleep(500);

	gpio_set_value(mc->gpio_cp_on, 1);
	if (mc->gpio_cp_off)
		gpio_set_value(mc->gpio_cp_off, 0);

	msleep(100);

	gpio_set_value(mc->gpio_cp_reset, 1);

	msleep(300);

	gpio_set_value(mc->gpio_pda_active, 1);

	mc->bootd->modem_state_changed(mc->bootd, STATE_BOOTING);

	mif_info("complete!!!\n");

	return 0;
}

static int cbp72_off(struct modem_ctl *mc)
{
	mif_info("cbp72_off()\n");

	if (!mc->gpio_cp_off || !mc->gpio_cp_reset) {
		mif_err("no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_cp_on, 0);
	gpio_set_value(mc->gpio_cp_off, 1);

	mc->bootd->modem_state_changed(mc->bootd, STATE_OFFLINE);

	return 0;
}

static int cbp72_reset(struct modem_ctl *mc)
{
	int ret = 0;

	mif_debug("cbp72_reset()\n");

	ret = cbp72_off(mc);
	if (ret)
		return -ENXIO;

	msleep(100);

	ret = cbp72_on(mc);
	if (ret)
		return -ENXIO;

	return 0;
}

static int cbp72_boot_on(struct modem_ctl *mc)
{
	mif_info("\n");

	if (!mc->gpio_cp_reset) {
		mif_err("no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_reset, 0);

	msleep(600);

	gpio_set_value(mc->gpio_cp_reset, 1);

	mc->bootd->modem_state_changed(mc->bootd, STATE_BOOTING);

	return 0;
}

static int cbp72_boot_off(struct modem_ctl *mc)
{
	int ret;
	struct link_device *ld = get_current_link(mc->bootd);
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	mif_debug("\n");
	/* Wait here until the PHONE is up.
	 * Waiting as the this called from IOCTL->UM thread */
	mif_info("Waiting for INT_CMD_PHONE_START\n");
	ret = wait_for_completion_interruptible_timeout(
			&dpld->dpram_init_cmd, DPRAM_INIT_TIMEOUT);
	if (!ret) {
		/* ret == 0 on timeout, ret < 0 if interrupted */
		mif_err("Timeout!!! (PHONE_START was not arrived.)\n");
		return -ENXIO;
	}

	mif_info("Waiting for INT_CMD_PIF_INIT_DONE\n");
	ret = wait_for_completion_interruptible_timeout(
			&dpld->modem_pif_init_done, PIF_TIMEOUT);
	if (!ret) {
		mif_err("Timeout!!! (PIF_INIT_DONE was not arrived.)\n");
		return -ENXIO;
	}
	mc->bootd->modem_state_changed(mc->bootd, STATE_ONLINE);

	wake_unlock(&mc->mc_wake_lock);

	return 0;
}

static int cbp72_force_crash_exit(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);

	mif_err("device = %s\n", mc->bootd->name);

	/* Make DUMP start */
	ld->force_dump(ld, mc->bootd);

	msleep_interruptible(1000);

	mc->bootd->modem_state_changed(mc->bootd, STATE_CRASH_EXIT);

	return 0;
}

static void cbp72_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = cbp72_on;
	mc->ops.modem_off = cbp72_off;
	mc->ops.modem_reset = cbp72_reset;
	mc->ops.modem_boot_on = cbp72_boot_on;
	mc->ops.modem_boot_off = cbp72_boot_off;
	mc->ops.modem_force_crash_exit = cbp72_force_crash_exit;
}

int cbp72_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	int ret = 0;
	int irq = 0;
	unsigned long flag = 0;
	struct platform_device *pdev = NULL;

	mc->gpio_cp_on         = pdata->gpio_cp_on;
	mc->gpio_cp_off        = pdata->gpio_cp_off;
	mc->gpio_reset_req_n   = pdata->gpio_reset_req_n;
	mc->gpio_cp_reset      = pdata->gpio_cp_reset;
	mc->gpio_pda_active    = pdata->gpio_pda_active;
	mc->gpio_phone_active  = pdata->gpio_phone_active;
	mc->gpio_cp_dump_int   = pdata->gpio_cp_dump_int;
	mc->gpio_flm_uart_sel  = pdata->gpio_flm_uart_sel;
	mc->gpio_cp_warm_reset = pdata->gpio_cp_warm_reset;

	if (!mc->gpio_cp_on || !mc->gpio_cp_reset || !mc->gpio_phone_active) {
		mif_err("no GPIO data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_reset, 0);
	if (mc->gpio_cp_off)
		gpio_set_value(mc->gpio_cp_off, 1);
	gpio_set_value(mc->gpio_cp_on, 0);

	cbp72_get_ops(mc);

	pdev = to_platform_device(mc->dev);
	mc->irq_phone_active = platform_get_irq_byname(pdev, "cp_active_irq");
	if (!mc->irq_phone_active) {
		mif_err("get irq fail\n");
		return -1;
	}

	irq = mc->irq_phone_active;
	mif_info("PHONE_ACTIVE IRQ# = %d\n", irq);

	flag = IRQF_TRIGGER_HIGH;
	ret = request_irq(irq, phone_active_handler, flag, "cbp_active", mc);
	if (ret) {
		mif_err("request_irq fail (%d)\n", ret);
		return ret;
	}

	wake_lock_init(&mc->mc_wake_lock, WAKE_LOCK_SUSPEND, "cbp72_wake_lock");

	ret = enable_irq_wake(irq);
	if (ret)
		mif_err("enable_irq_wake fail (%d)\n", ret);

	return 0;
}

