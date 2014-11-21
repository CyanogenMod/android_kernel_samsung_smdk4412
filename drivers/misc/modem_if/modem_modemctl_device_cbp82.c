/*
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
#include "modem_utils.h"

#define DPRAM_INIT_TIMEOUT	(30 * HZ)
#define PIF_TIMEOUT		(180 * HZ)

static irqreturn_t phone_active_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_reset = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	int old_state = mc->phone_state;
	int new_state = mc->phone_state;

	mif_info("old_state:%s cp_on:%d cp_reset:%d cp_active:%d\n",
		get_cp_state_str(old_state), cp_on, cp_reset, cp_active);

	if (cp_reset && cp_active) {
		if (mc->phone_state == STATE_BOOTING) {
			new_state = STATE_ONLINE;
			mc->bootd->modem_state_changed(mc->bootd, new_state);
		}
	} else if (cp_reset && !cp_active) {
		if (mc->phone_state == STATE_ONLINE) {
			new_state = STATE_CRASH_EXIT;
			mc->bootd->modem_state_changed(mc->bootd, new_state);
		}
	} else {
		new_state = STATE_OFFLINE;
		if (mc->bootd && mc->bootd->modem_state_changed)
			mc->bootd->modem_state_changed(mc->bootd, new_state);
	}

	if (new_state != old_state) {
		mif_err("%s: phone_state changed (%s -> %s\n)",
			mc->name, get_cp_state_str(old_state),
			get_cp_state_str(new_state));
	}

	return IRQ_HANDLED;
}

static int cbp82_on(struct modem_ctl *mc)
{
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_off = gpio_get_value(mc->gpio_cp_off);
	int cp_reset = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	mif_err("+++\n");

	mif_err("phone_state:%s cp_on:%d cp_off:%d cp_reset:%d cp_active:%d\n",
		get_cp_state_str(mc->phone_state), cp_on, cp_off, cp_reset,
		cp_active);

	/* prevent sleep during bootloader downloading */
	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	gpio_set_value(mc->gpio_cp_on, 0);
	gpio_set_value(mc->gpio_cp_off, 1);
	gpio_set_value(mc->gpio_cp_reset, 0);

	msleep(500);

	cp_on = gpio_get_value(mc->gpio_cp_on);
	cp_off = gpio_get_value(mc->gpio_cp_off);
	cp_reset = gpio_get_value(mc->gpio_cp_reset);
	cp_active = gpio_get_value(mc->gpio_phone_active);
	mif_err("phone_state:%s cp_on:%d cp_off:%d cp_reset:%d cp_active:%d\n",
		get_cp_state_str(mc->phone_state), cp_on, cp_off, cp_reset,
		cp_active);

	gpio_set_value(mc->gpio_cp_off, 0);
	gpio_set_value(mc->gpio_cp_on, 1);

	msleep(100);

	gpio_set_value(mc->gpio_cp_reset, 1);

	msleep(300);

	cp_on = gpio_get_value(mc->gpio_cp_on);
	cp_off = gpio_get_value(mc->gpio_cp_off);
	cp_reset = gpio_get_value(mc->gpio_cp_reset);
	cp_active = gpio_get_value(mc->gpio_phone_active);
	mif_err("phone_state:%s cp_on:%d cp_off:%d cp_reset:%d cp_active:%d\n",
		get_cp_state_str(mc->phone_state), cp_on, cp_off, cp_reset,
		cp_active);

	if (mc->gpio_pda_active)
		gpio_set_value(mc->gpio_pda_active, 1);

	if (mc->bootd)
		mc->bootd->modem_state_changed(mc->bootd, STATE_BOOTING);
	else
		mif_err("no mc->bootd\n");

	mif_err("---\n");
	return 0;
}

static int cbp82_off(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	mif_err("+++\n");

	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_cp_on, 0);
	gpio_set_value(mc->gpio_cp_off, 1);

	mc->bootd->modem_state_changed(mc->bootd, STATE_OFFLINE);
	ld->mode = LINK_MODE_OFFLINE;

	mif_err("---\n");
	return 0;
}

static int cbp82_reset(struct modem_ctl *mc)
{
	int ret = 0;

	mif_debug("cbp82_reset()\n");

	ret = cbp82_off(mc);
	if (ret)
		return -ENXIO;

	msleep(100);

	ret = cbp82_on(mc);
	if (ret)
		return -ENXIO;

	return 0;
}

static int cbp82_boot_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	mif_info("+++\n");

	ld->mode = LINK_MODE_BOOT;

	mif_info("---\n");
	return 0;
}

static int cbp82_boot_off(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	int ret;
	mif_err("+++\n");

	/* Wait here until the PHONE is up.
	 * Waiting as the this called from IOCTL->UM thread */
	mif_err("Waiting for PHONE_START\n");
	ret = wait_for_completion_timeout(&ld->init_cmpl, DPRAM_INIT_TIMEOUT);
	if (!ret) {
		/* ret == 0 on timeout */
		mif_err("T-I-M-E-O-U-T (PHONE_START)\n");
		cbp82_off(mc);
		ret = -EIO;
		goto exit;
	}
	mif_err("recv PHONE_START\n");

	mif_err("Waiting for PIF_INIT_DONE\n");
	ret = wait_for_completion_timeout(&ld->pif_cmpl, PIF_TIMEOUT);
	if (!ret) {
		/* ret == 0 on timeout */
		mif_err("T-I-M-E-O-U-T (PIF_INIT_DONE)!!!\n");
		cbp82_off(mc);
		ret = -EIO;
		goto exit;
	}
	mif_err("recv PIF_INIT_DONE\n");

	mc->bootd->modem_state_changed(mc->bootd, STATE_ONLINE);
	ret = 0;

exit:
	wake_unlock(&mc->mc_wake_lock);
	mif_err("---\n");
	return ret;
}

static int cbp82_force_crash_exit(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);

	mif_err("device = %s\n", mc->bootd->name);

	/* Make DUMP start */
	ld->force_dump(ld, mc->bootd);

	return 0;
}

static void cbp82_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = cbp82_on;
	mc->ops.modem_off = cbp82_off;
	mc->ops.modem_reset = cbp82_reset;
	mc->ops.modem_boot_on = cbp82_boot_on;
	mc->ops.modem_boot_off = cbp82_boot_off;
	mc->ops.modem_force_crash_exit = cbp82_force_crash_exit;
}

int cbp82_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	int ret = 0;
	int irq = 0;
	unsigned long flag = 0;
	mif_err("+++\n");

	mc->gpio_cp_on = pdata->gpio_cp_on;
	mc->gpio_cp_off = pdata->gpio_cp_off;
	mc->gpio_cp_reset = pdata->gpio_cp_reset;
	mc->gpio_phone_active = pdata->gpio_phone_active;

	if (!mc->gpio_cp_on || !mc->gpio_cp_off || !mc->gpio_cp_reset
	    || !mc->gpio_phone_active) {
		mif_err("no GPIO data\n");
		mif_err("---\n");
		return -ENXIO;
	}

	mc->gpio_pda_active = pdata->gpio_pda_active;

	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_cp_off, 1);
	gpio_set_value(mc->gpio_cp_on, 0);

	cbp82_get_ops(mc);

	wake_lock_init(&mc->mc_wake_lock, WAKE_LOCK_SUSPEND, "cbp82_wake_lock");

	mc->irq_phone_active = pdata->irq_phone_active;
	if (!mc->irq_phone_active) {
		mif_err("get irq fail\n");
		mif_err("---\n");
		return -1;
	}
	mif_info("PHONE_ACTIVE IRQ# = %d\n", mc->irq_phone_active);

	irq = mc->irq_phone_active;
	flag = IRQF_NO_SUSPEND | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
	ret = request_irq(irq, phone_active_handler, flag, "cdma_active", mc);
	if (ret) {
		mif_err("request_irq fail (%d)\n", ret);
		mif_err("---\n");
		return ret;
	}

	ret = enable_irq_wake(irq);
	if (ret)
		mif_err("enable_irq_wake fail (%d)\n", ret);

	mif_err("---\n");
	return 0;
}

