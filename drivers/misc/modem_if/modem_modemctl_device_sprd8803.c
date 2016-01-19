/* /linux/drivers/misc/modem_if/modem_modemctl_device_sprd8803.c
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
#include <linux/cma.h>
#include <plat/devs.h>
#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include <plat/gpio-cfg.h>

spinlock_t irq_lock;
int irq_lock_flag;

int sprd_boot_done;
extern int spi_thread_restart(void);

static int sprd8803_on(struct modem_ctl *mc)
{
	if (!mc->gpio_cp_on || !mc->gpio_pda_active) {
		pr_err("[MODEM_IF] no gpio data\n");
		return -ENXIO;
	}

	s3c_gpio_cfgpin(EXYNOS4_GPA1(4), S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(EXYNOS4_GPA1(5), S3C_GPIO_SFN(2));

#ifdef CONFIG_SEC_DUAL_MODEM_MODE
	gpio_set_value(mc->gpio_sim_io_sel, 1);
	gpio_set_value(mc->gpio_cp_ctrl1, 0);
	gpio_set_value(mc->gpio_cp_ctrl2, 1);
#endif
	msleep(100);
	pr_info("[MODEM_IF] %s\n", __func__);
	gpio_set_value(mc->gpio_cp_on, 1);
	gpio_set_value(mc->gpio_pda_active, 1);

	spin_lock(&irq_lock);
	if (!irq_lock_flag) {
		enable_irq(mc->irq_phone_active);
		enable_irq(gpio_to_irq(mc->gpio_cp_dump_int));
		enable_irq_wake(mc->irq_phone_active);
		enable_irq_wake(gpio_to_irq(mc->gpio_cp_dump_int));
		irq_lock_flag = 1;
	}
	spin_unlock(&irq_lock);

	mc->phone_state = STATE_BOOTING;

	return 0;
}

static int sprd8803_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s\n", __func__);

	if (!mc->gpio_cp_on) {
		mif_err("no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_on, 0);
	gpio_set_value(mc->gpio_pda_active, 0);

	spin_lock(&irq_lock);
	if (irq_lock_flag) {
		disable_irq(mc->irq_phone_active);
		disable_irq(gpio_to_irq(mc->gpio_cp_dump_int));
		disable_irq_wake(mc->irq_phone_active);
		disable_irq_wake(gpio_to_irq(mc->gpio_cp_dump_int));
		irq_lock_flag = 0;
	}
	spin_unlock(&irq_lock);

	mc->phone_state = STATE_OFFLINE;

	return 0;
}

static int sprd8803_reset(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s\n", __func__);

	spi_thread_restart();

	return 0;
}

static int sprd8803_boot_on(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s %d\n", __func__, mc->phone_state);
	return mc->phone_state;
}

static int sprd8803_boot_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s\n", __func__);
	spi_sema_init();
	return 0;
}

static int sprd8803_dump_reset(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s\n", __func__);

	if (!mc->gpio_ap_cp_int2)
		return -ENXIO;

	gpio_set_value(mc->gpio_ap_cp_int2, 0);
	mc->phone_state = STATE_OFFLINE;
	msleep(100);
	gpio_set_value(mc->gpio_ap_cp_int2, 1);

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

	if (!mc->gpio_phone_active ||
			!mc->gpio_cp_dump_int) {
		pr_err("[MODEM_IF] no gpio data\n");
		goto exit;
	}

	if (!sprd_boot_done)
		goto exit;

	phone_active_value = gpio_get_value(mc->gpio_phone_active);
	cp_dump_value = gpio_get_value(mc->gpio_cp_dump_int);

	pr_err("PA EVENT : pa=%d, cp_dump=%d\n",
				phone_active_value, cp_dump_value);

	if (phone_active_value)
		phone_state = STATE_ONLINE;
	else
		phone_state = STATE_OFFLINE;

	if (phone_active_value && cp_dump_value)
		phone_state = STATE_CRASH_EXIT;

	if (mc->iod && mc->iod->modem_state_changed)
		mc->iod->modem_state_changed(mc->iod, phone_state);

	if (mc->bootd && mc->bootd->modem_state_changed)
		mc->bootd->modem_state_changed(mc->bootd, phone_state);

exit:
	enable_irq(mc->irq_phone_active);

	return IRQ_HANDLED;
}

static void sprd8803_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = sprd8803_on;
	mc->ops.modem_off = sprd8803_off;
	mc->ops.modem_reset = sprd8803_reset;
	mc->ops.modem_boot_on = sprd8803_boot_on;
	mc->ops.modem_boot_off = sprd8803_boot_off;
	mc->ops.modem_dump_reset = sprd8803_dump_reset;
	mc->ops.modem_force_crash_exit = sprd8803_dump_reset;
}

int sprd8803_init_modemctl_device(struct modem_ctl *mc,
			struct modem_data *pdata)
{
	int ret = 0;
	int irq_cp_dump_int;
	struct platform_device *pdev;

	mc->gpio_cp_on = pdata->gpio_cp_on;
	mc->gpio_pda_active = pdata->gpio_pda_active;
	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_cp_dump_int = pdata->gpio_cp_dump_int;
	mc->gpio_ap_cp_int1 = pdata->gpio_ap_cp_int1;
	mc->gpio_ap_cp_int2 = pdata->gpio_ap_cp_int2;

#ifdef CONFIG_SEC_DUAL_MODEM_MODE
	mc->gpio_sim_io_sel = pdata->gpio_sim_io_sel;
	mc->gpio_cp_ctrl1 = pdata->gpio_cp_ctrl1;
	mc->gpio_cp_ctrl2 = pdata->gpio_cp_ctrl2;
#endif


	pdev = to_platform_device(mc->dev);
	mc->irq_phone_active = gpio_to_irq(mc->gpio_phone_active);
	irq_cp_dump_int = gpio_to_irq(mc->gpio_cp_dump_int);

	sprd8803_get_ops(mc);

	ret = request_irq(mc->irq_phone_active, phone_active_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"phone_active", mc);
	if (ret) {
		pr_err("[MODEM_IF] %s: failed to request_irq:%d\n",
			__func__, ret);
		return ret;
	}

	ret = enable_irq_wake(mc->irq_phone_active);
	if (ret) {
		pr_err("[MODEM_IF] %s: failed to enable_irq_wake:%d\n",
					__func__, ret);
		free_irq(mc->irq_phone_active, mc);
	}

	ret = request_irq(irq_cp_dump_int, phone_active_irq_handler,
				IRQF_TRIGGER_RISING,
				"cp_dump_int", mc);
	if (ret) {
		pr_err("[MODEM_IF] %s: failed to request_irq:%d\n",
			__func__, ret);
		return ret;
	}

	ret = enable_irq_wake(irq_cp_dump_int);
	if (ret) {
		pr_err("[MODEM_IF] %s: failed to enable_irq_wake:%d\n",
					__func__, ret);
		free_irq(irq_cp_dump_int, mc);
	}

	irq_lock_flag = 1;
	spin_lock_init(&irq_lock);

	spin_lock(&irq_lock);
	if (irq_lock_flag) {
		disable_irq(mc->irq_phone_active);
		disable_irq(gpio_to_irq(mc->gpio_cp_dump_int));
		disable_irq_wake(mc->irq_phone_active);
		disable_irq_wake(gpio_to_irq(mc->gpio_cp_dump_int));
		irq_lock_flag = 0;
	}
	spin_unlock(&irq_lock);

	return ret;
}
