/* /linux/drivers/misc/modem_if/modem_modemctl_device_xmm6262.c
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

#define DEBUG

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cma.h>
#include <plat/devs.h>
#include <linux/platform_data/modem.h>
#include "modem_prj.h"

static int xmm6262_on(struct modem_ctl *mc)
{
	mif_info("\n");

	if (!mc->gpio_cp_reset || !mc->gpio_cp_on || !mc->gpio_reset_req_n) {
		mif_err("no gpio data\n");
		return -ENXIO;
	}

	if (mc->gpio_revers_bias_clear)
		mc->gpio_revers_bias_clear();

#ifdef CONFIG_SEC_DUAL_MODEM_MODE
	gpio_set_value(mc->gpio_sim_io_sel, 0);
	gpio_set_value(mc->gpio_cp_ctrl1, 1);
	gpio_set_value(mc->gpio_cp_ctrl2, 0);
#endif

	/* TODO */
	gpio_set_value(mc->gpio_reset_req_n, 0);
	gpio_set_value(mc->gpio_cp_on, 0);
	gpio_set_value(mc->gpio_cp_reset, 0);
	msleep(100);
	gpio_set_value(mc->gpio_cp_reset, 1);
	/* If XMM6262 was connected with C2C, AP wait 50ms to BB Reset*/
	msleep(50);
	gpio_set_value(mc->gpio_reset_req_n, 1);

	gpio_set_value(mc->gpio_cp_on, 1);
	udelay(60);
	gpio_set_value(mc->gpio_cp_on, 0);
	msleep(20);

	mc->phone_state = STATE_BOOTING;

	if (mc->gpio_revers_bias_restore)
		mc->gpio_revers_bias_restore();
	gpio_set_value(mc->gpio_pda_active, 1);


	return 0;
}

static int xmm6262_off(struct modem_ctl *mc)
{
	mif_info("\n");

	if (!mc->gpio_cp_reset || !mc->gpio_cp_on) {
		mif_err("no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_on, 0);
	gpio_set_value(mc->gpio_cp_reset, 0);

	return 0;
}

static int xmm6262_reset(struct modem_ctl *mc)
{
	mif_info("\n");

	if (!mc->gpio_cp_reset || !mc->gpio_reset_req_n)
		return -ENXIO;

	if (mc->gpio_revers_bias_clear)
		mc->gpio_revers_bias_clear();

	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_reset_req_n, 0);

	mc->phone_state = STATE_OFFLINE;

	msleep(20);

	gpio_set_value(mc->gpio_cp_reset, 1);
	/* TODO: check the reset timming with C2C connection */
	udelay(160);

	gpio_set_value(mc->gpio_reset_req_n, 1);
	udelay(100);

	gpio_set_value(mc->gpio_cp_on, 1);
	udelay(60);
	gpio_set_value(mc->gpio_cp_on, 0);
	msleep(20);

	if (mc->gpio_revers_bias_restore)
		mc->gpio_revers_bias_restore();

	mc->phone_state = STATE_BOOTING;

	return 0;
}

static irqreturn_t phone_active_irq_handler(int irq, void *_mc)
{
	int phone_reset = 0;
	int phone_active_value = 0;
	int cp_dump_value = 0;
	int phone_state = 0;
	struct modem_ctl *mc = (struct modem_ctl *)_mc;

	disable_irq_nosync(mc->irq_phone_active);

	if (!mc->gpio_cp_reset || !mc->gpio_phone_active ||
			!mc->gpio_cp_dump_int) {
		mif_err("no gpio data\n");
		return IRQ_HANDLED;
	}

	phone_reset = gpio_get_value(mc->gpio_cp_reset);
	phone_active_value = gpio_get_value(mc->gpio_phone_active);
	cp_dump_value = gpio_get_value(mc->gpio_cp_dump_int);

	mif_info("PA EVENT : reset =%d, pa=%d, cp_dump=%d\n",
				phone_reset, phone_active_value, cp_dump_value);

	if (phone_reset && phone_active_value) {
		phone_state = STATE_BOOTING;
	} else if (mc->dev->power.is_suspended && !phone_active_value) {
		/*fixing dpm timeout by port2 resume retry*/
		mif_err("CP reset while dpm resume\n");
		xmm6262_off(mc);
		phone_state = STATE_CRASH_RESET;
	} else if (phone_reset && !phone_active_value) {
		phone_state =
			(cp_dump_value) ? STATE_CRASH_EXIT : STATE_CRASH_RESET;
	} else {
		phone_state = STATE_OFFLINE;
	}

	if (mc->iod && mc->iod->modem_state_changed)
		mc->iod->modem_state_changed(mc->iod, phone_state);

	if (mc->bootd && mc->bootd->modem_state_changed)
		mc->bootd->modem_state_changed(mc->bootd, phone_state);

	if (phone_active_value)
		irq_set_irq_type(mc->irq_phone_active, IRQ_TYPE_LEVEL_LOW);
	else
		irq_set_irq_type(mc->irq_phone_active, IRQ_TYPE_LEVEL_HIGH);
	enable_irq(mc->irq_phone_active);

	return IRQ_HANDLED;
}

#ifdef CONFIG_FAST_BOOT
#include <linux/reboot.h>
extern bool fake_shut_down;
static void mif_sim_detect_complete(struct modem_ctl *mc)
{
	if (mc->sim_shutdown_req) {
		mif_info("fake shutdown sim changed shutdown\n");
		kernel_power_off();
		/*kernel_restart(NULL);*/
		mc->sim_shutdown_req = false;
	}
}

static int mif_init_sim_shutdown(struct modem_ctl *mc)
{
	mc->sim_shutdown_req = false;
	mc->modem_complete = mif_sim_detect_complete;

	return 0;
}

static void mif_check_fake_shutdown(struct modem_ctl *mc, bool online)
{
	if (fake_shut_down && mc->sim_state.online != online)
		mc->sim_shutdown_req = true;
}

#else
static inline int mif_init_sim_shutdown(struct modem_ctl *mc) { return 0; }
#define mif_check_fake_shutdown(a, b) do {} while (0)
#endif


#define SIM_DETECT_DEBUG
static irqreturn_t sim_detect_irq_handler(int irq, void *_mc)
{
	struct modem_ctl *mc = (struct modem_ctl *)_mc;
#ifdef SIM_DETECT_DEBUG
	int val = gpio_get_value(mc->gpio_sim_detect);
	static int unchange;
	static int prev_val;

	if (mc->phone_state == STATE_BOOTING) {
		mif_info("BOOTING, reset unchange\n");
		unchange = 0;
	}

	if (prev_val == val) {
		if (unchange++ > 50) {
			mif_err("Abnormal SIM detect GPIO irqs");
			disable_irq_nosync(mc->gpio_sim_detect);
			panic("SIM detect IRQ Error");
		}
	} else {
		unchange = 0;
	}
	prev_val = val;
#endif
	if (mc->iod && mc->iod->sim_state_changed) {
		mif_check_fake_shutdown(mc,
			gpio_get_value(mc->gpio_sim_detect) == mc->sim_polarity
			);
		mc->iod->sim_state_changed(mc->iod,
			gpio_get_value(mc->gpio_sim_detect) == mc->sim_polarity
			);
	}

	return IRQ_HANDLED;
}

static void xmm6262_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = xmm6262_on;
	mc->ops.modem_off = xmm6262_off;
	mc->ops.modem_reset = xmm6262_reset;
}

int xmm6262_init_modemctl_device(struct modem_ctl *mc,
			struct modem_data *pdata)
{
	int ret = 0;
	struct platform_device *pdev;

	mc->gpio_reset_req_n = pdata->gpio_reset_req_n;
	mc->gpio_cp_on = pdata->gpio_cp_on;
	mc->gpio_cp_reset = pdata->gpio_cp_reset;
	mc->gpio_pda_active = pdata->gpio_pda_active;
	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_cp_dump_int = pdata->gpio_cp_dump_int;
	mc->gpio_ap_dump_int = pdata->gpio_ap_dump_int;
	mc->gpio_flm_uart_sel = pdata->gpio_flm_uart_sel;
	mc->gpio_cp_warm_reset = pdata->gpio_cp_warm_reset;
	mc->gpio_sim_detect = pdata->gpio_sim_detect;
	mc->sim_polarity = pdata->sim_polarity;

	mc->gpio_revers_bias_clear = pdata->gpio_revers_bias_clear;
	mc->gpio_revers_bias_restore = pdata->gpio_revers_bias_restore;

#ifdef CONFIG_SEC_DUAL_MODEM_MODE
	mc->gpio_sim_io_sel = pdata->gpio_sim_io_sel;
	mc->gpio_cp_ctrl1 = pdata->gpio_cp_ctrl1;
	mc->gpio_cp_ctrl2 = pdata->gpio_cp_ctrl2;
#endif


	pdev = to_platform_device(mc->dev);
	mc->irq_phone_active = gpio_to_irq(mc->gpio_phone_active);

	if (mc->gpio_sim_detect)
		mc->irq_sim_detect = gpio_to_irq(mc->gpio_sim_detect);

	xmm6262_get_ops(mc);

	ret = request_irq(mc->irq_phone_active, phone_active_irq_handler,
				IRQF_NO_SUSPEND | IRQF_TRIGGER_HIGH,
				"phone_active", mc);
	if (ret) {
		mif_err("failed to request_irq:%d\n", ret);
		goto err_phone_active_request_irq;
	}

	ret = enable_irq_wake(mc->irq_phone_active);
	if (ret) {
		mif_err("failed to enable_irq_wake:%d\n", ret);
		goto err_phone_active_set_wake_irq;
	}

	/* initialize sim_state if gpio_sim_detect exists */
	mc->sim_state.online = false;
	mc->sim_state.changed = false;
	if (mc->gpio_sim_detect) {
		ret = request_irq(mc->irq_sim_detect, sim_detect_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"sim_detect", mc);
		if (ret) {
			mif_err("failed to request_irq: %d\n", ret);
			goto err_sim_detect_request_irq;
		}

		ret = enable_irq_wake(mc->irq_sim_detect);
		if (ret) {
			mif_err("failed to enable_irq_wake: %d\n", ret);
			goto err_sim_detect_set_wake_irq;
		}

		/* initialize sim_state => insert: gpio=0, remove: gpio=1 */
		mc->sim_state.online =
			gpio_get_value(mc->gpio_sim_detect) == mc->sim_polarity;

		ret = mif_init_sim_shutdown(mc);
		if (ret) {
			mif_err("failed to sim fake shutdown init: %d\n", ret);
			goto err_sim_detect_set_wake_irq;
		}
	}

	return ret;

err_sim_detect_set_wake_irq:
	free_irq(mc->irq_sim_detect, mc);
err_sim_detect_request_irq:
	mc->sim_state.online = false;
	mc->sim_state.changed = false;
err_phone_active_set_wake_irq:
	free_irq(mc->irq_phone_active, mc);
err_phone_active_request_irq:
	return ret;
}
