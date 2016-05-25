/* /linux/drivers/misc/modem_if/modem_modemctl_device_xmm6260.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
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

static int xmm6260_on(struct modem_ctl *mc)
{
	mif_info("xmm6260_on()\n");

	if (!mc->gpio_cp_reset || !mc->gpio_cp_on || !mc->gpio_reset_req_n) {
		mif_err("no gpio data\n");
		return -ENXIO;
	}

	if (mc->gpio_revers_bias_clear)
		mc->gpio_revers_bias_clear();


	gpio_set_value(mc->gpio_cp_on, 0);
	gpio_set_value(mc->gpio_cp_reset, 0);
	udelay(160);
	gpio_set_value(mc->gpio_pda_active, 0);
	msleep(500); /* must be >500ms for CP can boot up under -20 degrees */
	gpio_set_value(mc->gpio_cp_reset, 1);
	udelay(160);
	gpio_set_value(mc->gpio_reset_req_n, 1);
	udelay(160);
	gpio_set_value(mc->gpio_cp_on, 1);
	udelay(60);
	gpio_set_value(mc->gpio_cp_on, 0);
	msleep(20);

	if (mc->gpio_revers_bias_restore)
		mc->gpio_revers_bias_restore();


	gpio_set_value(mc->gpio_pda_active, 1);

	mc->phone_state = STATE_BOOTING;

	return 0;

}

static int xmm6260_off(struct modem_ctl *mc)
{
	mif_info("xmm6260_off()\n");

	if (!mc->gpio_cp_reset || !mc->gpio_cp_on) {
		mif_err("no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_on, 0);
	gpio_set_value(mc->gpio_cp_reset, 0);

	mc->phone_state = STATE_OFFLINE;

	return 0;
}


static int xmm6260_reset(struct modem_ctl *mc)
{

	mif_info("xmm6260_reset()\n");

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

static int xmm6260_boot_on(struct modem_ctl *mc)
{
	mif_info("xmm6260_boot_on()\n");

	if (!mc->gpio_flm_uart_sel) {
		mif_err("no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_flm_uart_sel, 0);

	return 0;
}

static int xmm6260_boot_off(struct modem_ctl *mc)
{
	mif_info("xmm6260_boot_off()\n");

	if (!mc->gpio_flm_uart_sel) {
		mif_err("no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_flm_uart_sel, 1);

	return 0;
}

static irqreturn_t phone_active_irq_handler(int irq, void *_mc)
{
	int phone_reset = 0;
	int phone_active_value = 0;
	int cp_dump_value = 0;
	int phone_state = 0;
	struct modem_ctl *mc = (struct modem_ctl *)_mc;
	struct link_device *ld;

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

	if (phone_reset && phone_active_value)
		phone_state = STATE_BOOTING;
	else if (phone_reset && !phone_active_value) {
		if (mc->phone_state == STATE_BOOTING)
			goto set_type;
		if (cp_dump_value)
			phone_state = STATE_CRASH_EXIT;
		else
			phone_state = STATE_CRASH_RESET;
		if (mc->iod) {
			ld = get_current_link(mc->iod);
			if (ld->terminate_comm)
				ld->terminate_comm(ld, mc->iod);
		}
	} else
		phone_state = STATE_OFFLINE;

	if (mc->iod && mc->iod->modem_state_changed)
		mc->iod->modem_state_changed(mc->iod, phone_state);

	if (mc->bootd && mc->bootd->modem_state_changed)
		mc->bootd->modem_state_changed(mc->bootd, phone_state);

set_type:
	if (phone_active_value)
		irq_set_irq_type(mc->irq_phone_active, IRQ_TYPE_LEVEL_LOW);
	else
		irq_set_irq_type(mc->irq_phone_active, IRQ_TYPE_LEVEL_HIGH);
	enable_irq(mc->irq_phone_active);

	return IRQ_HANDLED;
}

static irqreturn_t sim_detect_irq_handler(int irq, void *_mc)
{
	struct modem_ctl *mc = (struct modem_ctl *)_mc;

	if (mc->iod && mc->iod->sim_state_changed)
		mc->iod->sim_state_changed(mc->iod,
				!gpio_get_value(mc->gpio_sim_detect));

	return IRQ_HANDLED;
}

static void xmm6260_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = xmm6260_on;
	mc->ops.modem_off = xmm6260_off;
	mc->ops.modem_reset = xmm6260_reset;
	mc->ops.modem_boot_on = xmm6260_boot_on;
	mc->ops.modem_boot_off = xmm6260_boot_off;
}

int xmm6260_init_modemctl_device(struct modem_ctl *mc,
			struct modem_data *pdata)
{
	int ret;
	struct platform_device *pdev;

	mc->gpio_cp_on = pdata->gpio_cp_on;
	mc->gpio_reset_req_n = pdata->gpio_reset_req_n;
	mc->gpio_cp_reset = pdata->gpio_cp_reset;
	mc->gpio_pda_active = pdata->gpio_pda_active;
	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_cp_dump_int = pdata->gpio_cp_dump_int;
	mc->gpio_flm_uart_sel = pdata->gpio_flm_uart_sel;
	mc->gpio_cp_warm_reset = pdata->gpio_cp_warm_reset;
	mc->gpio_revers_bias_clear = pdata->gpio_revers_bias_clear;
	mc->gpio_revers_bias_restore = pdata->gpio_revers_bias_restore;
	mc->gpio_sim_detect = pdata->gpio_sim_detect;

	pdev = to_platform_device(mc->dev);
	/* mc->irq_phone_active = platform_get_irq(pdev, 0); */
	mc->irq_phone_active = gpio_to_irq(mc->gpio_phone_active);

	if (mc->gpio_sim_detect)
		mc->irq_sim_detect = gpio_to_irq(mc->gpio_sim_detect);

	xmm6260_get_ops(mc);

	/* initialize phone active */
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
		mc->sim_state.online = !gpio_get_value(mc->gpio_sim_detect);
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
