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
#include <linux/platform_device.h>

#include <mach/c2c.h>
#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_utils.h"

#define MIF_INIT_TIMEOUT	(30 * HZ)

static void ss222_mc_state_fsm(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_reset  = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	int old_state = mc->phone_state;
	int new_state = mc->phone_state;

	mif_err("old_state:%s cp_on:%d cp_reset:%d cp_active:%d\n",
		get_cp_state_str(old_state), cp_on, cp_reset, cp_active);

	if (cp_active) {
		if (!cp_on) {
			new_state = STATE_OFFLINE;
			ld->mode = LINK_MODE_OFFLINE;
		} else if (old_state == STATE_ONLINE) {
			new_state = STATE_CRASH_EXIT;
			ld->mode = LINK_MODE_ULOAD;
		} else {
			mif_err("don't care!!!\n");
		}
	}

	if (old_state != new_state) {
		mif_err("new_state = %s\n", get_cp_state_str(new_state));
		mc->bootd->modem_state_changed(mc->bootd, new_state);
		mc->iod->modem_state_changed(mc->iod, new_state);
	}
}

static irqreturn_t phone_active_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	int cp_reset = gpio_get_value(mc->gpio_cp_reset);

	if (cp_reset)
		ss222_mc_state_fsm(mc);

	return IRQ_HANDLED;
}

static inline void make_gpio_floating(int gpio, bool floating)
{
	if (floating)
		gpio_direction_input(gpio);
	else
		gpio_direction_output(gpio, 0);
}

static int ss222_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_off = gpio_get_value(mc->gpio_cp_off);
	int cp_reset  = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	int cp_status = gpio_get_value(mc->gpio_cp_status);
	mif_err("+++\n");
	mif_err("cp_on:%d cp_reset:%d ps_hold:%d cp_active:%d cp_status:%d\n",
		cp_on, cp_reset, cp_off, cp_active, cp_status);

	gpio_set_value(mc->gpio_pda_active, 1);

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	mc->phone_state = STATE_OFFLINE;
	ld->mode = LINK_MODE_OFFLINE;

	/* Make PS_HOLD floating (Hi-Z) for CP ON */
	make_gpio_floating(mc->gpio_cp_off, true);

	gpio_set_value(mc->gpio_cp_on, 0);
	msleep(100);

	gpio_set_value(mc->gpio_cp_reset, 0);
	msleep(500);

	gpio_set_value(mc->gpio_cp_on, 1);
	msleep(100);

	c2c_reload();
	gpio_set_value(mc->gpio_cp_reset, 1);
	msleep(300);

	mif_err("---\n");
	return 0;
}

static int ss222_off(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	mif_err("+++\n");

	if (mc->phone_state == STATE_OFFLINE || cp_on == 0)
		return 0;

	mc->phone_state = STATE_OFFLINE;
	ld->mode = LINK_MODE_OFFLINE;

	gpio_set_value(mc->gpio_cp_reset, 0);

	/* Make PS_HOLD LOW for CP OFF */
	make_gpio_floating(mc->gpio_cp_off, false);
	gpio_set_value(mc->gpio_cp_on, 0);

	mif_err("---\n");
	return 0;
}

static int ss222_reset(struct modem_ctl *mc)
{
	mif_err("+++\n");

	if (ss222_off(mc))
		return -EIO;

	msleep(100);

	if (ss222_on(mc))
		return -EIO;

	mif_err("---\n");
	return 0;
}

static int ss222_force_crash_exit(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	mif_err("+++\n");

	/* Make DUMP start */
	ld->force_dump(ld, mc->bootd);

	mif_err("---\n");
	return 0;
}

static int ss222_dump_reset(struct modem_ctl *mc)
{
	unsigned int gpio_cp_reset = mc->gpio_cp_reset;
	mif_err("+++\n");

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	gpio_set_value(gpio_cp_reset, 0);
	udelay(200);

	c2c_reload();
	gpio_set_value(gpio_cp_reset, 1);
	msleep(300);

	gpio_set_value(mc->gpio_ap_status, 1);

	mif_err("---\n");
	return 0;
}

static int ss222_boot_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	mif_debug("+++\n");

	disable_irq_nosync(mc->irq_phone_active);

	gpio_set_value(mc->gpio_ap_status, 1);

	ld->mode = LINK_MODE_BOOT;

	mc->bootd->modem_state_changed(mc->bootd, STATE_BOOTING);
	mc->iod->modem_state_changed(mc->iod, STATE_BOOTING);

	INIT_COMPLETION(ld->init_cmpl);

	mif_debug("---\n");
	return 0;
}

static int ss222_boot_off(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	unsigned long remain;
	mif_debug("+++\n");

	ld->mode = LINK_MODE_IPC;

	remain = wait_for_completion_timeout(&ld->init_cmpl, MIF_INIT_TIMEOUT);
	if (remain == 0) {
		mif_err("T-I-M-E-O-U-T\n");
		mif_err("xxx\n");
		return -EAGAIN;
	}

	mif_debug("---\n");
	return 0;
}

static int ss222_boot_done(struct modem_ctl *mc)
{
	mif_debug("+++\n");

	if (wake_lock_active(&mc->mc_wake_lock))
		wake_unlock(&mc->mc_wake_lock);

	enable_irq(mc->irq_phone_active);

	mif_debug("---\n");
	return 0;
}

static void ss222_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = ss222_on;
	mc->ops.modem_off = ss222_off;
	mc->ops.modem_reset = ss222_reset;
	mc->ops.modem_boot_on = ss222_boot_on;
	mc->ops.modem_boot_off = ss222_boot_off;
	mc->ops.modem_boot_done = ss222_boot_done;
	mc->ops.modem_force_crash_exit = ss222_force_crash_exit;
	mc->ops.modem_dump_reset = ss222_dump_reset;
}

int ss222_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	int ret = 0;
	int irq = 0;
	unsigned long flag = 0;
	mif_debug("+++\n");

	if (!pdata->gpio_cp_on || !pdata->gpio_cp_off || !pdata->gpio_cp_reset
	    || !pdata->gpio_pda_active || !pdata->gpio_phone_active
	    || !pdata->gpio_ap_wakeup || !pdata->gpio_ap_status
	    || !pdata->gpio_cp_wakeup || !pdata->gpio_cp_status) {
		mif_err("ERR! no GPIO data\n");
		mif_err("xxx\n");
		return -ENXIO;
	}

	mc->gpio_cp_on = pdata->gpio_cp_on;
	mc->gpio_cp_off = pdata->gpio_cp_off;
	mc->gpio_cp_reset = pdata->gpio_cp_reset;
	mc->gpio_pda_active = pdata->gpio_pda_active;
	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_ap_wakeup = pdata->gpio_ap_wakeup;
	mc->gpio_ap_status = pdata->gpio_ap_status;
	mc->gpio_cp_wakeup = pdata->gpio_cp_wakeup;
	mc->gpio_cp_status = pdata->gpio_cp_status;

	gpio_set_value(mc->gpio_cp_reset, 0);

	gpio_set_value(mc->gpio_cp_on, 0);

	ss222_get_ops(mc);
	dev_set_drvdata(mc->dev, mc);

	wake_lock_init(&mc->mc_wake_lock, WAKE_LOCK_SUSPEND, "umts_wake_lock");

	mc->irq_phone_active = pdata->irq_phone_active;
	if (!mc->irq_phone_active) {
		mif_err("ERR! no irq_phone_active\n");
		mif_err("xxx\n");
		return -1;
	}
	mif_err("PHONE_ACTIVE IRQ# = %d\n", mc->irq_phone_active);

	irq = mc->irq_phone_active;
	flag = IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND;
	ret = request_irq(irq, phone_active_handler, flag, "umts_active", mc);
	if (ret) {
		mif_err("ERR! request_irq(#%d) fail (err %d)\n", irq, ret);
		mif_err("xxx\n");
		return ret;
	}
	ret = enable_irq_wake(irq);
	if (ret)
		mif_err("enable_irq_wake(#%d) fail (err %d)\n", irq, ret);

	mif_debug("---\n");
	return 0;
}

