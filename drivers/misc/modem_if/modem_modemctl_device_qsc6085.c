/* /linux/drivers/misc/modem_if/modem_modemctl_device_qsc6085.c
 *
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
#include "modem_utils.h"

#define IDPRAM_NORMAL_BOOT_MAGIC	0x4D4E

static irqreturn_t phone_active_irq_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	int phone_reset = gpio_get_value(mc->gpio_cp_reset);
	int phone_active = gpio_get_value(mc->gpio_phone_active);
	int phone_state = mc->phone_state;

	pr_info("MIF: <%s> state = %d, phone_reset = %d, phone_active = %d\n",
		__func__, phone_state, phone_reset, phone_active);

	if (phone_reset && phone_active) {
		phone_state = STATE_ONLINE;
		if (mc->phone_state != STATE_BOOTING)
			mc->iod->modem_state_changed(mc->iod, phone_state);
	} else if (phone_reset && !phone_active) {
		if (mc->phone_state == STATE_ONLINE) {
			phone_state = STATE_CRASH_EXIT;
			mc->iod->modem_state_changed(mc->iod, phone_state);
		}
	} else {
		phone_state = STATE_OFFLINE;
		if (mc->iod && mc->iod->modem_state_changed)
			mc->iod->modem_state_changed(mc->iod, phone_state);
	}

	if (phone_active)
		irq_set_irq_type(mc->irq_phone_active, IRQ_TYPE_LEVEL_LOW);
	else
		irq_set_irq_type(mc->irq_phone_active, IRQ_TYPE_LEVEL_HIGH);

	pr_info("MIF: <%s> phone_state = %d\n", __func__, phone_state);

	return IRQ_HANDLED;
}

static void set_idpram_boot_magic(struct dpram_link_device *dpld)
{
	dpld->set_access(dpld, 0);
	dpld->set_magic(dpld, IDPRAM_NORMAL_BOOT_MAGIC);
	dpld->set_access(dpld, 1);
}

static int qsc6085_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	mif_err("+++\n");

	if (!mc->gpio_cp_on || !mc->gpio_cp_reset) {
		mif_err("no gpio_cp_on or no gpio_cp_reset\n");
		return -ENXIO;
	}

	set_idpram_boot_magic(dpld);

	mc->iod->modem_state_changed(mc->iod, STATE_BOOTING);

	gpio_set_value(mc->gpio_cp_reset, 1);
	gpio_set_value(mc->gpio_cp_on, 0);

	msleep(100);

	gpio_set_value(mc->gpio_cp_on, 1);

	msleep(400);
	msleep(400);
	msleep(200);

	gpio_set_value(mc->gpio_cp_on, 0);

	mif_err("---\n");
	return 0;
}

static int qsc6085_off(struct modem_ctl *mc)
{
	int phone_wait_cnt = 0;

	pr_info("MIF: <%s+>\n", __func__);

	if (!mc->gpio_cp_on || !mc->gpio_cp_reset ||
		!mc->gpio_phone_active) {
		pr_err("MIF: <%s> no gpio data\n", __func__);
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_on, 0);

	/* confirm phone off */
	while (1) {
		if (gpio_get_value(mc->gpio_phone_active)) {
			pr_err("MIF: <%s> Try to Turn Phone Off by CP_RST\n",
				__func__);
			gpio_set_value(mc->gpio_cp_reset, 0);
			if (phone_wait_cnt > 10) {
				pr_emerg("MIF: <%s> OFF Failed\n", __func__);
				break;
			}
			phone_wait_cnt++;
			mdelay(100);
		} else {
			pr_emerg("MIF: <%s> OFF Success\n", __func__);
			break;
		}
	}

	mc->iod->modem_state_changed(mc->iod, STATE_OFFLINE);

	pr_info("MIF: <%s->\n", __func__);

	return 0;
}

static int qsc6085_reset(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	struct dpram_link_device *dpld = to_dpram_link_device(ld);

	mif_err("+++\n");

	set_idpram_boot_magic(dpld);

	gpio_set_value(mc->gpio_cp_reset, 0);
	msleep(100);
	gpio_set_value(mc->gpio_cp_reset, 1);

	mif_err("---\n");
	return 0;
}

static int qsc6085_modem_dump_reset(struct modem_ctl *mc)
{
	pr_info("MIF: <%s>\n", __func__);
	panic("CP Crashed");
}

static void qsc6085_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = qsc6085_on;
	mc->ops.modem_off = qsc6085_off;
	mc->ops.modem_reset = qsc6085_reset;
	mc->ops.modem_dump_reset = qsc6085_modem_dump_reset;
}

int qsc6085_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	int ret = 0;
	unsigned long flag = 0;

	mc->gpio_cp_on = pdata->gpio_cp_on;
	mc->gpio_cp_reset = pdata->gpio_cp_reset;
	mc->gpio_pda_active = pdata->gpio_pda_active;
	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_cp_dump_int = pdata->gpio_cp_dump_int;

	if (!mc->gpio_cp_on || !mc->gpio_cp_reset || !mc->gpio_phone_active) {
		mif_err("no GPIO data\n");
		return -ENXIO;
	}

	mc->irq_phone_active = gpio_to_irq(mc->gpio_phone_active);
		pr_err("MIF: <%s> PHONE_ACTIVE IRQ# = %d\n",
		__func__, mc->irq_phone_active);

	qsc6085_get_ops(mc);

	/*register phone_active_handler*/
	flag = IRQF_TRIGGER_HIGH;

	ret = request_irq(mc->irq_phone_active,
			phone_active_irq_handler,
			flag, "phone_active", mc);
	if (ret) {
		pr_err("MIF: failed to irq_phone_active request_irq: %d\n"
			, ret);
		return ret;
	}

	ret = enable_irq_wake(mc->irq_phone_active);
	if (ret) {
		pr_err("MIF: <%s> failed to enable_irq_wake:%d\n",
					__func__, ret);
		free_irq(mc->irq_phone_active, mc);
		return ret;
	}
	return ret;
}
