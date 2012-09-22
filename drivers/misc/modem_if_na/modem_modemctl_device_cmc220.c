/* /linux/drivers/misc/modem_if/modem_modemctl_device_cmc220.c
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

#include <linux/platform_data/modem_na.h>
#include "modem_prj.h"
#include "modem_link_device_usb.h"

static int cmc220_on(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s()\n", __func__);

	if (!mc->gpio_cp_off || !mc->gpio_cp_on || !mc->gpio_cp_reset) {
		pr_err("[MODEM_IF] no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_on, 1);
	msleep(300);
	gpio_set_value(mc->gpio_cp_reset, 1);
	msleep(100);
	gpio_set_value(mc->gpio_cp_off, 0);
	msleep(300);
	mc->phone_state = STATE_BOOTING;
	return 0;
}

static int cmc220_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s()\n", __func__);

	if (!mc->gpio_cp_off || !mc->gpio_cp_on || !mc->gpio_cp_reset) {
		pr_err("[MODEM_IF] no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_on, 0);
	msleep(100);
	gpio_set_value(mc->gpio_cp_off, 1);
	msleep(100);
	gpio_set_value(mc->gpio_cp_reset, 0);

	mc->phone_state = STATE_OFFLINE;

	return 0;
}

static int cmc220_force_crash_exit(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s: # %d\n", __func__, ++(mc->crash_cnt));

	mc->phone_state = STATE_CRASH_EXIT;/* DUMP START */

	if (mc->iod && mc->iod->modem_state_changed)
		mc->iod->modem_state_changed(mc->iod, mc->phone_state);

	return 0;
}

static int cmc220_dump_reset(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s()\n", __func__);

	if (!mc->gpio_cp_reset)
		return -ENXIO;

	gpio_set_value(mc->gpio_host_active, 0);
	mc->cpcrash_flag = 1;

	gpio_set_value(mc->gpio_cp_reset, 0);
	msleep(100);
	gpio_set_value(mc->gpio_cp_reset, 1);
	msleep(300);

	mc->phone_state = STATE_BOOTING;

	return 0;
}

static int cmc220_reset(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s()\n", __func__);

	if (!mc->gpio_cp_reset)
		return -ENXIO;

	if (cmc220_off(mc))
		return -ENXIO;
	msleep(100);
	if (cmc220_on(mc))
		return -ENXIO;

	mc->phone_state = STATE_BOOTING;

	return 0;
}

static int cmc220_boot_on(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s()\n", __func__);
	return 0;
}

static int cmc220_boot_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s()\n", __func__);
	return 0;
}

static int cmc220_get_active(struct modem_ctl *mc)
{
	if (!mc->gpio_phone_active || !mc->gpio_cp_reset)
		return -ENXIO;

	pr_debug("cp %d phone %d\n",
			gpio_get_value(mc->gpio_cp_reset),
			gpio_get_value(mc->gpio_phone_active));

	if (gpio_get_value(mc->gpio_cp_reset))
		return gpio_get_value(mc->gpio_phone_active);

	return 0;
}


static void mc_work(struct work_struct *work_arg)
{

	struct modem_ctl *mc = container_of(work_arg, struct modem_ctl,
		dwork.work);

	int phone_active;

	phone_active = cmc220_get_active(mc);
	if (phone_active < 0) {
		pr_err("[MODEM_IF] gpio not initialized\n");
		return;
	}

	switch (mc->phone_state) {
	case STATE_CRASH_EXIT:
	case STATE_BOOTING:
	case STATE_LOADER_DONE:
		if (phone_active) {
			if (mc->cpcrash_flag) {
				pr_info("[MODEM_IF] LTE DUMP END!!\n");
				mc->cpcrash_flag = 0;
			}
		}
		break;
	case STATE_ONLINE:
		if (!phone_active) {
			pr_info("[MODEM_IF] LTE CRASHED!! LTE DUMP START!!\n");
			mc->phone_state = STATE_CRASH_EXIT;
			if (mc->iod && mc->iod->modem_state_changed)
				mc->iod->modem_state_changed(mc->iod,
						mc->phone_state);
		}
		break;
	default:
		mc->phone_state = STATE_OFFLINE;
		pr_err("[MODEM_IF], phone_status changed to invalid!!\n");
		break;
	}
}



static irqreturn_t phone_active_irq_handler(int irq, void *_mc)
{
	struct modem_ctl *mc = (struct modem_ctl *)_mc;

	schedule_delayed_work(&mc->dwork, 20);

	return IRQ_HANDLED;
}

static void cmc220_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = cmc220_on;
	mc->ops.modem_off = cmc220_off;
	mc->ops.modem_reset = cmc220_reset;
	mc->ops.modem_boot_on = cmc220_boot_on;
	mc->ops.modem_boot_off = cmc220_boot_off;
	mc->ops.modem_force_crash_exit = cmc220_force_crash_exit;
	mc->ops.modem_dump_reset = cmc220_dump_reset;
}

int cmc220_init_modemctl_device(struct modem_ctl *mc,
			struct modem_data *pdata)
{
	int ret = 0;
	struct platform_device *pdev;

	mc->gpio_cp_on = pdata->gpio_cp_on;
	mc->gpio_reset_req_n = pdata->gpio_reset_req_n;
	mc->gpio_cp_reset = pdata->gpio_cp_reset;
	mc->gpio_pda_active = pdata->gpio_pda_active;
	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_cp_dump_int = pdata->gpio_cp_dump_int;
	mc->gpio_flm_uart_sel = pdata->gpio_flm_uart_sel;
	mc->gpio_cp_warm_reset = pdata->gpio_cp_warm_reset;
	mc->gpio_cp_off = pdata->gpio_cp_off;
	mc->gpio_slave_wakeup = pdata->gpio_slave_wakeup;
	mc->gpio_host_active = pdata->gpio_host_active;
	mc->gpio_host_wakeup = pdata->gpio_host_wakeup;

	pdev = to_platform_device(mc->dev);
	mc->irq_phone_active = gpio_to_irq(mc->gpio_phone_active);

	cmc220_get_ops(mc);

	dev_set_drvdata(mc->dev, mc);

	INIT_DELAYED_WORK(&mc->dwork, mc_work);

	ret = request_irq(mc->irq_phone_active, phone_active_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"lte_phone_active", mc);
	if (ret) {
		pr_err("[MODEM_IF] Failed to allocate an interrupt(%d)\n",
							mc->irq_phone_active);
		goto irq_fail;
	}
	mc->irq[0] = mc->irq_phone_active;
	enable_irq_wake(mc->irq_phone_active);
	/*disable_irq(mc->irq_phone_active);*/

	return ret;

irq_fail:
	kfree(mc);
	return ret;
}
