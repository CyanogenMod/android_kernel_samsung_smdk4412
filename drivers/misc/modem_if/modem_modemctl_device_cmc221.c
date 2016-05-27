/* /linux/drivers/misc/modem_if/modem_modemctl_device_cmc221.c
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
#include <linux/platform_device.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_link_device_usb.h"
#include "modem_link_device_dpram.h"
#include "modem_utils.h"

#define PIF_TIMEOUT		(180 * HZ)
#define DPRAM_INIT_TIMEOUT	(30 * HZ)

static void mc_state_fsm(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_reset  = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	int old_state = mc->phone_state;
	int new_state = mc->phone_state;

	mif_err("%s: old_state:%d cp_on:%d cp_reset:%d cp_active:%d\n",
		mc->name, old_state, cp_on, cp_reset, cp_active);

	if (!cp_active) {
		if (!cp_on) {
			gpio_set_value(mc->gpio_cp_reset, 0);
			new_state = STATE_OFFLINE;
			ld->mode = LINK_MODE_OFFLINE;
			mif_err("%s: new_state = PHONE_PWR_OFF\n", mc->name);
		} else if (old_state == STATE_ONLINE) {
			new_state = STATE_CRASH_EXIT;
			mif_err("%s: new_state = CRASH_EXIT\n", mc->name);
		} else {
			mif_err("%s: Don't care!!!\n", mc->name);
		}
	}

	if (old_state != new_state) {
		mc->bootd->modem_state_changed(mc->bootd, new_state);
		mc->iod->modem_state_changed(mc->iod, new_state);
	}
}

static irqreturn_t phone_active_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	int cp_reset  = gpio_get_value(mc->gpio_cp_reset);

	if (cp_reset)
		mc_state_fsm(mc);

	return IRQ_HANDLED;
}

/* TX dynamic switching between DPRAM and USB in one modem */
static irqreturn_t dynamic_switching_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	int txpath = gpio_get_value(mc->gpio_dynamic_switching);
	bool enumerated = usb_is_enumerated(mc->msd);

	mif_err("txpath=%d, enumeration=%d\n", txpath, enumerated);

	/* do not switch to USB, when USB is not enumerated. */
	if (!enumerated && txpath) {
		mc->need_switch_to_usb = true;
		return IRQ_HANDLED;
	}

	mc->need_switch_to_usb = false;
	rawdevs_set_tx_link(mc->msd, txpath ? LINKDEV_USB : LINKDEV_DPRAM);

	return IRQ_HANDLED;
}

static int cmc221_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);
	set_sromc_access(true);

	mc->phone_state = STATE_OFFLINE;
	ld->mode = LINK_MODE_OFFLINE;

	mif_err("%s\n", mc->name);

	disable_irq_nosync(mc->irq_phone_active);

	gpio_set_value(mc->gpio_cp_on, 0);
	msleep(100);

	gpio_set_value(mc->gpio_cp_reset, 0);
	msleep(500);

	gpio_set_value(mc->gpio_cp_on, 1);
	msleep(100);

	gpio_set_value(mc->gpio_cp_reset, 1);

	return 0;
}

static int cmc221_off(struct modem_ctl *mc)
{
	int cp_on = gpio_get_value(mc->gpio_cp_on);

	mif_err("%s\n", mc->name);

	if (mc->phone_state == STATE_OFFLINE || cp_on == 0)
		return 0;

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);
	set_sromc_access(true);

	gpio_set_value(mc->gpio_cp_on, 0);

	return 0;
}

static int cmc221_force_crash_exit(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);

	mif_err("%s\n", mc->name);

	/* Make DUMP start */
	ld->force_dump(ld, mc->bootd);

	return 0;
}

static int cmc221_dump_reset(struct modem_ctl *mc)
{
	mif_err("%s\n", mc->name);

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);
	set_sromc_access(true);

	gpio_set_value(mc->gpio_host_active, 0);
	gpio_set_value(mc->gpio_cp_reset, 0);

	udelay(200);

	gpio_set_value(mc->gpio_cp_reset, 1);

	msleep(300);

	return 0;
}

static int cmc221_reset(struct modem_ctl *mc)
{
	mif_err("%s\n", mc->name);

	if (cmc221_off(mc))
		return -ENXIO;

	msleep(100);

	if (cmc221_on(mc))
		return -ENXIO;

	return 0;
}

static int cmc221_boot_on(struct modem_ctl *mc)
{
	mif_err("%s\n", mc->name);

	gpio_set_value(mc->gpio_pda_active, 1);

	mc->bootd->modem_state_changed(mc->bootd, STATE_BOOTING);
	mc->iod->modem_state_changed(mc->iod, STATE_BOOTING);

	return 0;
}

static int cmc221_boot_off(struct modem_ctl *mc)
{
	int ret;
	struct link_device *ld = get_current_link(mc->bootd);
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	mif_err("%s\n", mc->name);

	ret = wait_for_completion_interruptible_timeout(&dpld->dpram_init_cmd,
			DPRAM_INIT_TIMEOUT);
	if (!ret) {
		/* ret == 0 on timeout, ret < 0 if interrupted */
		mif_err("%s: ERR! timeout (CP_START not arrived)\n", mc->name);
		return -ENXIO;
	}

	enable_irq(mc->irq_phone_active);

	return 0;
}

static int cmc221_boot_done(struct modem_ctl *mc)
{
	mif_err("%s\n", mc->name);

	set_sromc_access(false);
	if (wake_lock_active(&mc->mc_wake_lock))
		wake_unlock(&mc->mc_wake_lock);

	return 0;
}

static void cmc221_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = cmc221_on;
	mc->ops.modem_off = cmc221_off;
	mc->ops.modem_reset = cmc221_reset;
	mc->ops.modem_boot_on = cmc221_boot_on;
	mc->ops.modem_boot_off = cmc221_boot_off;
	mc->ops.modem_boot_done = cmc221_boot_done;
	mc->ops.modem_force_crash_exit = cmc221_force_crash_exit;
	mc->ops.modem_dump_reset = cmc221_dump_reset;
}

int cmc221_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	int ret = 0;
	int irq = 0;
	unsigned long flag = 0;
	struct platform_device *pdev = NULL;

	mc->gpio_cp_on        = pdata->gpio_cp_on;
	mc->gpio_cp_reset     = pdata->gpio_cp_reset;
	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_pda_active   = pdata->gpio_pda_active;
#if 0	/*TODO: check the GPIO map*/
	mc->gpio_cp_dump_int  = pdata->gpio_cp_dump_int;
	mc->gpio_flm_uart_sel = pdata->gpio_flm_uart_sel;
	mc->gpio_slave_wakeup = pdata->gpio_slave_wakeup;
	mc->gpio_host_active  = pdata->gpio_host_active;
	mc->gpio_host_wakeup  = pdata->gpio_host_wakeup;
#endif
	mc->gpio_dynamic_switching = pdata->gpio_dynamic_switching;
	mc->need_switch_to_usb = false;

	if (!mc->gpio_cp_on || !mc->gpio_cp_reset || !mc->gpio_phone_active) {
		mif_err("%s: ERR! no GPIO data\n", mc->name);
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_cp_on, 0);

	cmc221_get_ops(mc);
	dev_set_drvdata(mc->dev, mc);

	pdev = to_platform_device(mc->dev);
	mc->irq_phone_active = platform_get_irq_byname(pdev, "cp_active_irq");
	if (!mc->irq_phone_active) {
		mif_err("%s: ERR! get cp_active_irq fail\n", mc->name);
		return -1;
	}
	mif_err("%s: PHONE_ACTIVE IRQ# = %d\n", mc->name, mc->irq_phone_active);

	wake_lock_init(&mc->mc_wake_lock, WAKE_LOCK_SUSPEND, "cmc_wake_lock");

	flag = IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND;
	irq = mc->irq_phone_active;
	ret = request_irq(irq, phone_active_handler, flag, "cmc_active", mc);
	if (ret) {
		mif_err("%s: ERR! request_irq(#%d) fail (err %d)\n",
			mc->name, irq, ret);
		return ret;
	}
	ret = enable_irq_wake(irq);
	if (ret) {
		mif_err("%s: WARNING! enable_irq_wake(#%d) fail (err %d)\n",
			mc->name, irq, ret);
	}

	flag = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND;
	if (mc->gpio_dynamic_switching) {
		irq = gpio_to_irq(mc->gpio_dynamic_switching);
		mif_err("%s: DYNAMIC_SWITCH IRQ# = %d\n", mc->name, irq);
		ret = request_irq(irq, dynamic_switching_handler, flag,
				"dynamic_switching", mc);
		if (ret) {
			mif_err("%s: ERR! request_irq(#%d) fail (err %d)\n",
				mc->name, irq, ret);
			return ret;
		}
	}

	return 0;
}
