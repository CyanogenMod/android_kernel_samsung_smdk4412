/* /linux/drivers/misc/modem_if/modem_modemctl_device_esc6270.c
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
#include <linux/regulator/consumer.h>

#include <plat/gpio-cfg.h>

#if defined(CONFIG_LINK_DEVICE_DPRAM)
#include "modem_link_device_dpram.h"
#elif defined(CONFIG_LINK_DEVICE_PLD)
#include "modem_link_device_pld.h"
#endif

#if defined(CONFIG_LINK_DEVICE_DPRAM) || defined(CONFIG_LINK_DEVICE_PLD)
#include <linux/mfd/max77693.h>

#define PIF_TIMEOUT		(180 * HZ)
#define DPRAM_INIT_TIMEOUT	(30 * HZ)

static int esc6270_on(struct modem_ctl *mc)
{
	int ret;
	struct link_device *ld = get_current_link(mc->iod);

	pr_info("[MODEM_IF:ESC] <%s> start!!!\n", __func__);

	if (!mc->gpio_cp_reset) {
		pr_err("[MODEM_IF:ESC] no gpio data\n");
		return -ENXIO;
	}

	if (mc->gpio_reset_req_n)
		gpio_set_value(mc->gpio_reset_req_n, 1);

	gpio_set_value(mc->gpio_cp_reset, 1);
	msleep(30);

	gpio_set_value(mc->gpio_cp_on, 1);
	msleep(500);

	gpio_set_value(mc->gpio_cp_on, 0);
	msleep(500);

	gpio_set_value(mc->gpio_pda_active, 1);

	mc->iod->modem_state_changed(mc->iod, STATE_BOOTING);
	ld->mode = LINK_MODE_BOOT;

	return 0;
}

static int esc6270_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF:ESC] esc6270_off()\n");

#if 1
	if (!mc->gpio_cp_reset) {
		pr_err("[MODEM_IF:ESC] no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_cp_on, 0);
#endif

	mc->iod->modem_state_changed(mc->iod, STATE_OFFLINE);

	return 0;
}

static int esc6270_reset(struct modem_ctl *mc)
{
	int ret = 0;

	pr_debug("[MODEM_IF:ESC] esc6270_reset()\n");

	ret = esc6270_off(mc);
	if (ret)
		return -ENXIO;

	msleep(100);

	ret = esc6270_on(mc);
	if (ret)
		return -ENXIO;

	return 0;
}

int esc6270_boot_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
#if defined(CONFIG_LINK_DEVICE_DPRAM)
	/* clear intr */
	struct dpram_link_device *dpld = to_dpram_link_device(ld);
	u16 recv_msg = dpld->recv_intr(dpld);

	pr_info("[MODEM_IF:ESC] dpram intr: %x\n", recv_msg);
#endif

	pr_info("[MODEM_IF:ESC] <%s>\n", __func__);

	/* Need to init uart byt gpio_flm_uart_sel GPIO */
	if (!mc->gpio_cp_reset || !mc->gpio_flm_uart_sel) {
		pr_err("[MODEM_IF:ESC] no gpio data\n");
		return -ENXIO;
	}
	gpio_set_value(mc->gpio_flm_uart_sel, 1);

	pr_info("  - ESC_PHONE_ON : %d, ESC_RESET_N : %d\n",
			gpio_get_value(mc->gpio_cp_on),
			gpio_get_value(mc->gpio_cp_reset));

	gpio_set_value(mc->gpio_cp_on, 0);
	gpio_direction_output(mc->gpio_cp_reset, 0);
	msleep(100);

	gpio_direction_output(mc->gpio_cp_on, 1);
	msleep(44);

	pr_info("  - ESC_PHONE_ON : %d, ESC_RESET_N : %d\n",
			gpio_get_value(mc->gpio_cp_on),
			gpio_get_value(mc->gpio_cp_reset));

	gpio_direction_input(mc->gpio_cp_reset);
	msleep(600);
	gpio_direction_output(mc->gpio_cp_on, 0);

	msleep(20);
	pr_info("  - ESC_PHONE_ON : %d, ESC_RESET_N : %d\n",
			gpio_get_value(mc->gpio_cp_on),
			gpio_get_value(mc->gpio_cp_reset));

#if defined(CONFIG_LINK_DEVICE_PLD)
	gpio_direction_output(mc->gpio_fpga_cs_n, 1);
#endif

	mc->iod->modem_state_changed(mc->iod, STATE_BOOTING);
	ld->mode = LINK_MODE_BOOT;

	return 0;
}

static int esc6270_boot_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF:ESC] <%s>\n", __func__);

	if (!mc->gpio_flm_uart_sel) {
		pr_err("[MODEM_IF:ESC] no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_flm_uart_sel, 0);

	mc->iod->modem_state_changed(mc->iod, STATE_OFFLINE);

	return 0;
}

static int esc6270_active_count;

static irqreturn_t phone_active_irq_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	int phone_reset = 0;
	int phone_active = 0;
	int phone_state = 0;
	int cp_dump_int = 0;

	if (!mc->gpio_cp_reset ||
		!mc->gpio_phone_active) { /* || !mc->gpio_cp_dump_int) { */
		pr_err("[MODEM_IF:ESC] no gpio data\n");
		return IRQ_HANDLED;
	}

	phone_reset = gpio_get_value(mc->gpio_cp_reset);
	phone_active = gpio_get_value(mc->gpio_phone_active);
	cp_dump_int = gpio_get_value(mc->gpio_cp_dump_int);

	pr_info("[MODEM_IF:ESC] <%s> phone_reset=%d, phone_active=%d, cp_dump_int=%d\n",
		__func__, phone_reset, phone_active, cp_dump_int);

	if (phone_reset && phone_active) {
		phone_state = STATE_ONLINE;
		if (mc->iod && mc->iod->modem_state_changed)
			mc->iod->modem_state_changed(mc->iod, phone_state);
	} else if (phone_reset && !phone_active) {
		if (mc->phone_state == STATE_ONLINE) {
				phone_state = STATE_CRASH_EXIT;
			if (mc->iod && mc->iod->modem_state_changed)
				mc->iod->modem_state_changed(mc->iod,
							     phone_state);
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

	pr_info("[MODEM_IF::ESC] <%s> phone_state = %d\n",
			__func__, phone_state);

	return IRQ_HANDLED;
}

#if defined(CONFIG_SIM_DETECT)
static irqreturn_t sim_detect_irq_handler(int irq, void *_mc)
{
	struct modem_ctl *mc = (struct modem_ctl *)_mc;

	pr_info("[MODEM_IF:ESC] <%s> gpio_sim_detect = %d\n",
		__func__, gpio_get_value(mc->gpio_sim_detect));

	if (mc->iod && mc->iod->sim_state_changed)
		mc->iod->sim_state_changed(mc->iod,
		!gpio_get_value(mc->gpio_sim_detect));

	return IRQ_HANDLED;
}
#endif

static void esc6270_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = esc6270_on;
	mc->ops.modem_off = esc6270_off;
	mc->ops.modem_reset = esc6270_reset;
	mc->ops.modem_boot_on = esc6270_boot_on;
	mc->ops.modem_boot_off = esc6270_boot_off;
}

int esc6270_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
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
	mc->gpio_sim_detect = pdata->gpio_sim_detect;

#if defined(CONFIG_LINK_DEVICE_PLD)
	mc->gpio_fpga_cs_n = pdata->gpio_fpga1_cs_n;
#endif

	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_cp_on, 0);

	mc->irq_phone_active = pdata->irq_phone_active;
	if (!mc->irq_phone_active) {
		mif_err("%s: ERR! get irq_phone_active fail\n", mc->name);
		return -1;
	}
	pr_info("[MODEM_IF:ESC] <%s> PHONE_ACTIVE IRQ# = %d\n",
		__func__, mc->irq_phone_active);

	esc6270_get_ops(mc);

	if (mc->irq_phone_active) {
		ret = request_irq(mc->irq_phone_active,
				  phone_active_irq_handler,
				  IRQF_TRIGGER_HIGH,
				  "esc_active",
				  mc);
		if (ret) {
			pr_err("[MODEM_IF:ESC] <%s> failed to request_irq IRQ# %d (err=%d)\n",
				__func__, mc->irq_phone_active, ret);
			return ret;
		}

		ret = enable_irq_wake(mc->irq_phone_active);
		if (ret) {
			pr_err("[MODEM_IF:ESC] %s: failed to enable_irq_wake IRQ# %d (err=%d)\n",
				__func__, mc->irq_phone_active, ret);
			free_irq(mc->irq_phone_active, mc);
			return ret;
		}
	}

#if defined(CONFIG_SIM_DETECT)
	mc->irq_sim_detect = pdata->irq_sim_detect;
	pr_info("[MODEM_IF:ESC] <%s> SIM_DECTCT IRQ# = %d\n",
		__func__, mc->irq_sim_detect);

	if (mc->irq_sim_detect) {
		ret = request_irq(mc->irq_sim_detect, sim_detect_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"esc_sim_detect", mc);
		if (ret) {
			mif_err("failed to request_irq: %d\n", ret);
				mc->sim_state.online = false;
				mc->sim_state.changed = false;
			return ret;
		}

		ret = enable_irq_wake(mc->irq_sim_detect);
		if (ret) {
			mif_err("failed to enable_irq_wake: %d\n", ret);
			free_irq(mc->irq_sim_detect, mc);
			mc->sim_state.online = false;
			mc->sim_state.changed = false;
			return ret;
		}

		/* initialize sim_state => insert: gpio=0, remove: gpio=1 */
		mc->sim_state.online = !gpio_get_value(mc->gpio_sim_detect);
	}
#endif

	return ret;
}
#endif
