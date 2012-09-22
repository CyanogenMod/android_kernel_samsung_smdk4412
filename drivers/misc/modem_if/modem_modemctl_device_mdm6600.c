/* /linux/drivers/misc/modem_if/modem_modemctl_device_mdm6600.c
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

#if defined(CONFIG_MACH_M0_CTC)
#include <linux/mfd/max77693.h>
#endif

#if defined(CONFIG_MACH_U1_KOR_LGT)
#include <linux/mfd/max8997.h>

static int mdm6600_on(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] mdm6600_on()\n");

	if (!mc->gpio_cp_reset || !mc->gpio_cp_reset_msm || !mc->gpio_cp_on) {
		pr_err("[MODEM_IF] no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_pda_active, 0);
	gpio_set_value(mc->gpio_cp_reset, 1);
	gpio_set_value(mc->gpio_cp_reset_msm, 1);
	msleep(30);
	gpio_set_value(mc->gpio_cp_on, 1);
	msleep(300);
	gpio_set_value(mc->gpio_cp_on, 0);
	msleep(500);

	gpio_set_value(mc->gpio_pda_active, 1);

	mc->iod->modem_state_changed(mc->iod, STATE_BOOTING);

	return 0;
}

static int mdm6600_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] mdm6600_off()\n");

	if (!mc->gpio_cp_reset || !mc->gpio_cp_reset_msm || !mc->gpio_cp_on) {
		pr_err("[MODEM_IF] no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_on, 0);
	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_cp_reset_msm, 0);

	mc->iod->modem_state_changed(mc->iod, STATE_OFFLINE);

	return 0;
}

static int mdm6600_reset(struct modem_ctl *mc)
{
	int ret;

	pr_info("[MODEM_IF] mdm6600_reset()\n");

	if (!mc->gpio_cp_reset || !mc->gpio_cp_reset_msm || !mc->gpio_cp_on) {
		pr_err("[MODEM_IF] no gpio data\n");
		return -ENXIO;
	}

	if (system_rev >= 0x05) {
		dev_err(mc->dev, "[%s] system_rev: %d\n", __func__, system_rev);

		gpio_set_value(mc->gpio_cp_reset_msm, 0);
		msleep(100);	/* no spec, confirm later exactly how much time
				   needed to initialize CP with RESET_PMU_N */
		gpio_set_value(mc->gpio_cp_reset_msm, 1);
		msleep(40);	/* > 37.2 + 2 msec */
	} else {
		dev_err(mc->dev, "[%s] system_rev: %d\n", __func__, system_rev);

		gpio_set_value(mc->gpio_cp_reset, 0);
		msleep(500);	/* no spec, confirm later exactly how much time
				   needed to initialize CP with RESET_PMU_N */
		gpio_set_value(mc->gpio_cp_reset, 1);
		msleep(40);	/* > 37.2 + 2 msec */
	}

	return 0;
}

static int mdm6600_boot_on(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] mdm6600_boot_on()\n");

	if (!mc->gpio_boot_sw_sel) {
		pr_err("[MODEM_IF] no gpio data\n");
		return -ENXIO;
	}

	if (mc->vbus_on)
		mc->vbus_on();

	if (mc->gpio_boot_sw_sel)
		gpio_set_value(mc->gpio_boot_sw_sel, 0);
	mc->usb_boot = true;

	return 0;
}

static int mdm6600_boot_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] mdm6600_boot_off()\n");

	if (!mc->gpio_boot_sw_sel) {
		pr_err("[MODEM_IF] no gpio data\n");
		return -ENXIO;
	}

	if (mc->vbus_off)
		mc->vbus_off();

	if (mc->gpio_boot_sw_sel)
		gpio_set_value(mc->gpio_boot_sw_sel, 1);
	mc->usb_boot = false;

	return 0;
}

static int count;

static irqreturn_t phone_active_irq_handler(int irq, void *_mc)
{
	int phone_reset = 0;
	int phone_active_value = 0;
	int cp_dump_value = 0;
	int phone_state = 0;
	struct modem_ctl *mc = (struct modem_ctl *)_mc;

	if (!mc->gpio_cp_reset || !mc->gpio_phone_active
/*|| !mc->gpio_cp_dump_int */) {
		pr_err("[MODEM_IF] no gpio data\n");
		return IRQ_HANDLED;
	}

	phone_reset = gpio_get_value(mc->gpio_cp_reset);
	phone_active_value = gpio_get_value(mc->gpio_phone_active);

	pr_info("[MODEM_IF] PA EVENT : reset =%d, pa=%d, cp_dump=%d\n",
		phone_reset, phone_active_value, cp_dump_value);

	if (phone_reset && phone_active_value) {
		phone_state = STATE_ONLINE;
		if (mc->iod && mc->iod->modem_state_changed)
			mc->iod->modem_state_changed(mc->iod, phone_state);
	} else if (phone_reset && !phone_active_value) {
		if (count == 1) {
			phone_state = STATE_CRASH_EXIT;
			if (mc->iod) {
				ld = get_current_link(mc->iod);
				if (ld->terminate_comm)
					ld->terminate_comm(ld, mc->iod);
			}
			if (mc->iod && mc->iod->modem_state_changed)
				mc->iod->modem_state_changed
				    (mc->iod, phone_state);
			count = 0;
		} else {
			count++;
		}
	} else {
		phone_state = STATE_OFFLINE;
		if (mc->iod && mc->iod->modem_state_changed)
			mc->iod->modem_state_changed(mc->iod, phone_state);
	}

	pr_info("phone_active_irq_handler : phone_state=%d\n", phone_state);

	return IRQ_HANDLED;
}

static void mdm6600_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = mdm6600_on;
	mc->ops.modem_off = mdm6600_off;
	mc->ops.modem_reset = mdm6600_reset;
	mc->ops.modem_boot_on = mdm6600_boot_on;
	mc->ops.modem_boot_off = mdm6600_boot_off;
}

int mdm6600_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	int ret;
	struct platform_device *pdev;

	mc->gpio_cp_on = pdata->gpio_cp_on;
	mc->gpio_cp_reset = pdata->gpio_cp_reset;
	mc->gpio_pda_active = pdata->gpio_pda_active;
	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_cp_reset_msm = pdata->gpio_cp_reset_msm;
	mc->gpio_boot_sw_sel = pdata->gpio_boot_sw_sel;

	mc->vbus_on = pdata->vbus_on;
	mc->vbus_off = pdata->vbus_off;

	pdev = to_platform_device(mc->dev);
	mc->irq_phone_active = platform_get_irq_byname(pdev, "cp_active_irq");
	pr_info("[MODEM_IF] <%s> PHONE_ACTIVE IRQ# = %d\n",
		__func__, mc->irq_phone_active);

	mdm6600_get_ops(mc);

	ret = request_irq(mc->irq_phone_active, phone_active_irq_handler,
			  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			  "phone_active", mc);
	if (ret) {
		pr_err("[MODEM_IF] %s: failed to request_irq:%d\n",
		       __func__, ret);
		goto err_request_irq;
	}

	ret = enable_irq_wake(mc->irq_phone_active);
	if (ret) {
		pr_err("[MODEM_IF] %s: failed to enable_irq_wake:%d\n",
		       __func__, ret);
		goto err_set_wake_irq;
	}

	return ret;

 err_set_wake_irq:
	free_irq(mc->irq_phone_active, mc);
 err_request_irq:
	return ret;
}
#endif				/* CONFIG_MACH_U1_KOR_LGT */

#if defined(CONFIG_MACH_M0_CTC) || defined(CONFIG_MACH_T0_CHN_CTC)

#if defined(CONFIG_LINK_DEVICE_DPRAM)
#include "modem_link_device_dpram.h"
#elif defined(CONFIG_LINK_DEVICE_PLD)
#include "modem_link_device_pld.h"
#endif

#define PIF_TIMEOUT		(180 * HZ)
#define DPRAM_INIT_TIMEOUT	(30 * HZ)

#if defined(CONFIG_MACH_M0_DUOSCTC) || defined(CONFIG_MACH_M0_GRANDECTC) || \
	defined(CONFIG_MACH_T0_CHN_CTC)
static void mdm6600_vbus_on(void)
{
	struct regulator *regulator;

	pr_info("[MSM] <%s>\n", __func__);

#if defined(CONFIG_MACH_T0_CHN_CTC)
	if (system_rev == 4)
		regulator = regulator_get(NULL, "vcc_1.8v_lcd");
	else
		regulator = regulator_get(NULL, "vcc_1.8v_usb");
#else
	regulator = regulator_get(NULL, "vusbhub_osc_1.8v");
#endif
	if (IS_ERR(regulator)) {
		pr_err("[MSM] error getting regulator_get <%s>\n", __func__);
		return ;
	}
	regulator_enable(regulator);
	regulator_put(regulator);

	pr_info("[MSM] <%s> enable\n", __func__);
}

static void mdm6600_vbus_off(void)
{
	struct regulator *regulator;

	pr_info("[MSM] <%s>\n", __func__);

#if defined(CONFIG_MACH_T0_CHN_CTC)
	if (system_rev == 4)
		regulator = regulator_get(NULL, "vcc_1.8v_lcd");
	else
		regulator = regulator_get(NULL, "vcc_1.8v_usb");
#else
	regulator = regulator_get(NULL, "vusbhub_osc_1.8v");
#endif
	if (IS_ERR(regulator)) {
		pr_err("[MSM] error getting regulator_get <%s>\n", __func__);
		return ;
	}
	regulator_disable(regulator);
	regulator_put(regulator);

	pr_info("[MSM] <%s> disable\n", __func__);
}
#endif

static int mdm6600_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);

	pr_info("[MSM] <%s>\n", __func__);

	if (!mc->gpio_reset_req_n || !mc->gpio_cp_reset
	    || !mc->gpio_cp_on || !mc->gpio_pda_active) {
		pr_err("[MSM] no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_pda_active, 0);

	gpio_set_value(mc->gpio_cp_on, 1);
	msleep(500);

	gpio_set_value(mc->gpio_reset_req_n, 1);
	msleep(50);

	gpio_set_value(mc->gpio_cp_reset, 1);
	msleep(50);

	gpio_set_value(mc->gpio_cp_on, 0);
	msleep(500);

	gpio_set_value(mc->gpio_pda_active, 1);

#if defined(CONFIG_LINK_DEVICE_PLD)
	gpio_set_value(mc->gpio_fpga_cs_n, 1);
#endif

	mc->iod->modem_state_changed(mc->iod, STATE_BOOTING);
	ld->mode = LINK_MODE_BOOT;

	return 0;
}

static int mdm6600_off(struct modem_ctl *mc)
{
	pr_info("[MSM] <%s>\n", __func__);

	if (!mc->gpio_cp_reset || !mc->gpio_cp_on) {
		pr_err("[MSM] no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_reset_req_n, 0);
	gpio_set_value(mc->gpio_cp_on, 0);

	msleep(200);

	mc->iod->modem_state_changed(mc->iod, STATE_OFFLINE);

	return 0;
}

static int mdm6600_reset(struct modem_ctl *mc)
{
	int ret = 0;
	struct link_device *ld = get_current_link(mc->iod);

	pr_info("[MSM] <%s>\n", __func__);

	if (!mc->gpio_reset_req_n || !mc->gpio_cp_reset
	    || !mc->gpio_cp_on || !mc->gpio_pda_active) {
		pr_err("[MSM] no gpio data\n");
		return -ENXIO;
	}

	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_reset_req_n, 0);
	gpio_set_value(mc->gpio_cp_on, 0);

	msleep(100);

	gpio_set_value(mc->gpio_cp_on, 1);
	msleep(300);

	gpio_set_value(mc->gpio_reset_req_n, 1);
	msleep(50);

	gpio_set_value(mc->gpio_cp_reset, 1);
	msleep(50);

	gpio_set_value(mc->gpio_cp_on, 0);
	msleep(100);

	gpio_set_value(mc->gpio_pda_active, 1);

	mc->iod->modem_state_changed(mc->iod, STATE_BOOTING);
	ld->mode = LINK_MODE_BOOT;

	return 0;
}

static int mdm6600_boot_on(struct modem_ctl *mc)
{
	struct regulator *regulator;

	pr_info("[MSM] <%s>\n", __func__);

	if (!mc->gpio_flm_uart_sel) {
		pr_err("[MSM] no gpio data\n");
		return -ENXIO;
	}

#if defined(CONFIG_MACH_M0_DUOSCTC) || defined(CONFIG_MACH_T0_CHN_CTC)
	mdm6600_vbus_on();
#elif defined(CONFIG_MACH_M0_GRANDECTC)
	if (system_rev >= 14)
		mdm6600_vbus_on();
#endif

	pr_info("[MSM] <%s> %s\n", __func__, "USB_BOOT_EN initializing");
	if (system_rev < 11) {

		gpio_direction_output(GPIO_USB_BOOT_EN, 0);
		s3c_gpio_setpull(GPIO_USB_BOOT_EN, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_USB_BOOT_EN, 0);

		gpio_direction_output(GPIO_BOOT_SW_SEL, 0);
		s3c_gpio_setpull(GPIO_BOOT_SW_SEL, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BOOT_SW_SEL, 0);

		msleep(100);

		gpio_direction_output(GPIO_USB_BOOT_EN, 1);
		gpio_set_value(GPIO_USB_BOOT_EN, 1);

		pr_info("[MSM] <%s> USB_BOOT_EN:[%d]\n", __func__,
			gpio_get_value(GPIO_USB_BOOT_EN));

		gpio_direction_output(GPIO_BOOT_SW_SEL, 1);
		gpio_set_value(GPIO_BOOT_SW_SEL, 1);

		pr_info("[MSM] <%s> BOOT_SW_SEL : [%d]\n", __func__,
			gpio_get_value(GPIO_BOOT_SW_SEL));
	} else if (system_rev == 11) {
		gpio_direction_output(GPIO_USB_BOOT_EN, 0);
		s3c_gpio_setpull(GPIO_USB_BOOT_EN, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_USB_BOOT_EN, 0);

		gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 0);
		s3c_gpio_setpull(GPIO_USB_BOOT_EN_REV06, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_USB_BOOT_EN_REV06, 0);

		msleep(100);

		gpio_direction_output(GPIO_USB_BOOT_EN, 1);
		gpio_set_value(GPIO_USB_BOOT_EN, 1);

		pr_info("[MSM] <%s> USB_BOOT_EN:[%d]\n", __func__,
			gpio_get_value(GPIO_USB_BOOT_EN));

		gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 1);
		gpio_set_value(GPIO_USB_BOOT_EN_REV06, 1);

		pr_info("[MSM(%d)] <%s> USB_BOOT_EN:[%d]\n", system_rev,
			__func__, gpio_get_value(GPIO_USB_BOOT_EN_REV06));

		gpio_direction_output(GPIO_BOOT_SW_SEL, 0);
		s3c_gpio_setpull(GPIO_BOOT_SW_SEL, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BOOT_SW_SEL, 0);

		gpio_direction_output(GPIO_BOOT_SW_SEL_REV06, 0);
		s3c_gpio_setpull(GPIO_BOOT_SW_SEL_REV06, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BOOT_SW_SEL_REV06, 0);

		msleep(100);

		gpio_direction_output(GPIO_BOOT_SW_SEL, 1);
		gpio_set_value(GPIO_BOOT_SW_SEL, 1);

		pr_info("[MSM] <%s> BOOT_SW_SEL : [%d]\n", __func__,
			gpio_get_value(GPIO_BOOT_SW_SEL));

		gpio_direction_output(GPIO_BOOT_SW_SEL_REV06, 1);
		gpio_set_value(GPIO_BOOT_SW_SEL_REV06, 1);

		pr_info("[MSM(%d)] <%s> BOOT_SW_SEL : [%d]\n", system_rev,
			__func__, gpio_get_value(GPIO_BOOT_SW_SEL_REV06));

	} else {	/* system_rev>11 */
		gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 0);
		s3c_gpio_setpull(GPIO_USB_BOOT_EN_REV06, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_USB_BOOT_EN_REV06, 0);

		gpio_direction_output(GPIO_BOOT_SW_SEL_REV06, 0);
		s3c_gpio_setpull(GPIO_BOOT_SW_SEL_REV06, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BOOT_SW_SEL_REV06, 0);

		msleep(100);

		gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 1);
		gpio_set_value(GPIO_USB_BOOT_EN_REV06, 1);

		pr_info("[MSM] <%s> USB_BOOT_EN:[%d]\n", __func__,
			gpio_get_value(GPIO_USB_BOOT_EN_REV06));

		gpio_direction_output(GPIO_BOOT_SW_SEL_REV06, 1);
		gpio_set_value(GPIO_BOOT_SW_SEL_REV06, 1);

	pr_info("[MSM] <%s> BOOT_SW_SEL : [%d]\n", __func__,
			gpio_get_value(GPIO_BOOT_SW_SEL_REV06));

	}

	mc->iod->modem_state_changed(mc->iod, STATE_BOOTING);

	return 0;
}

static int mdm6600_boot_off(struct modem_ctl *mc)
{
	pr_info("[MSM] <%s>\n", __func__);

	if (!mc->gpio_flm_uart_sel
#if defined(CONFIG_MACH_M0_CTC)
		|| !mc->gpio_flm_uart_sel_rev06
#endif
	) {
		pr_err("[MSM] no gpio data\n");
		return -ENXIO;
	}

#if defined(CONFIG_MACH_M0_DUOSCTC) || defined(CONFIG_MACH_T0_CHN_CTC)
	mdm6600_vbus_off();
#elif defined(CONFIG_MACH_M0_GRANDECTC)
	if (system_rev >= 14)
		mdm6600_vbus_off();
#endif

	if (system_rev < 11) {
		gpio_direction_output(GPIO_USB_BOOT_EN, 0);
		s3c_gpio_setpull(GPIO_USB_BOOT_EN, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_USB_BOOT_EN, 0);
		gpio_direction_output(GPIO_BOOT_SW_SEL, 0);
		s3c_gpio_setpull(GPIO_BOOT_SW_SEL, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BOOT_SW_SEL, 0);

	} else if (system_rev == 11) {
		gpio_direction_output(GPIO_USB_BOOT_EN, 0);
		s3c_gpio_setpull(GPIO_USB_BOOT_EN, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_USB_BOOT_EN, 0);

		gpio_direction_output(GPIO_BOOT_SW_SEL, 0);
		s3c_gpio_setpull(GPIO_BOOT_SW_SEL, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BOOT_SW_SEL, 0);

#if defined(CONFIG_MACH_M0_CTC)
		gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 0);
		s3c_gpio_setpull(GPIO_USB_BOOT_EN_REV06, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_USB_BOOT_EN_REV06, 0);

		gpio_direction_output(GPIO_BOOT_SW_SEL_REV06, 0);
		s3c_gpio_setpull(GPIO_BOOT_SW_SEL_REV06, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BOOT_SW_SEL_REV06, 0);
#endif

	} else {	/* system_rev>11 */
#if defined(CONFIG_MACH_M0_CTC)
		gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 0);
		s3c_gpio_setpull(GPIO_USB_BOOT_EN_REV06, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_USB_BOOT_EN_REV06, 0);

		gpio_direction_output(GPIO_BOOT_SW_SEL_REV06, 0);
		s3c_gpio_setpull(GPIO_BOOT_SW_SEL_REV06, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BOOT_SW_SEL_REV06, 0);
#endif
	}

#if defined(CONFIG_MACH_M0_CTC)
	if (max7693_muic_cp_usb_state()) {
		msleep(30);
		gpio_direction_output(GPIO_USB_BOOT_EN, 1);
		s3c_gpio_setpull(GPIO_USB_BOOT_EN, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_USB_BOOT_EN, 1);
		gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 1);
		s3c_gpio_setpull(GPIO_USB_BOOT_EN_REV06, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_USB_BOOT_EN_REV06, 1);
	}
#endif

	gpio_set_value(GPIO_BOOT_SW_SEL, 0);

	return 0;
}


static int mdm6600_force_crash_exit(struct modem_ctl *mc)
{
	pr_info("[MSM] <%s>\n", __func__);

	if (!mc->gpio_cp_reset || !mc->gpio_cp_on) {
		pr_err("[MSM] no gpio data\n");
		return -ENXIO;
	}

	s3c_gpio_cfgpin(mc->gpio_cp_dump_int, S3C_GPIO_OUTPUT);
	gpio_direction_output(mc->gpio_cp_dump_int, 1);

	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_cp_on, 0);
	gpio_set_value(mc->gpio_cp_reset, 1);

	return 0;
}

static irqreturn_t phone_active_irq_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	int phone_reset = 0;
	int phone_active = 0;
	int phone_state = 0;
	int cp_dump_int = 0;

	if (!mc->gpio_cp_reset ||
		!mc->gpio_phone_active || !mc->gpio_cp_dump_int) {
		pr_err("[MSM] no gpio data\n");
		return IRQ_HANDLED;
	}

	phone_reset = gpio_get_value(mc->gpio_cp_reset);
	phone_active = gpio_get_value(mc->gpio_phone_active);
	cp_dump_int = gpio_get_value(mc->gpio_cp_dump_int);

	pr_info("[MSM] <%s> phone_reset=%d, phone_active=%d, cp_dump_int=%d\n",
		__func__, phone_reset, phone_active, cp_dump_int);

	if (phone_reset && phone_active) {
		phone_state = STATE_ONLINE;
		if (mc->iod && mc->iod->modem_state_changed)
			mc->iod->modem_state_changed(mc->iod, phone_state);
	} else if (phone_reset && !phone_active) {
		if (mc->phone_state == STATE_ONLINE) {
			if (cp_dump_int)
				phone_state = STATE_CRASH_EXIT;
			else
				phone_state = STATE_CRASH_RESET;
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

	pr_info("[MSM] <%s> phone_state = %d\n", __func__, phone_state);

	return IRQ_HANDLED;
}

#if defined(CONFIG_SIM_DETECT)
static irqreturn_t sim_detect_irq_handler(int irq, void *_mc)
{
	struct modem_ctl *mc = (struct modem_ctl *)_mc;

	pr_info("[MSM] <%s> gpio_sim_detect = %d\n",
		__func__, gpio_get_value(mc->gpio_sim_detect));

	if (mc->iod && mc->iod->sim_state_changed)
		mc->iod->sim_state_changed(mc->iod,
		!gpio_get_value(mc->gpio_sim_detect));

	return IRQ_HANDLED;
}
#endif

static void mdm6600_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = mdm6600_on;
	mc->ops.modem_off = mdm6600_off;
	mc->ops.modem_reset = mdm6600_reset;
	mc->ops.modem_boot_on = mdm6600_boot_on;
	mc->ops.modem_boot_off = mdm6600_boot_off;
	mc->ops.modem_force_crash_exit = mdm6600_force_crash_exit;
}

int mdm6600_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
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
#if defined(CONFIG_MACH_M0_CTC)
	mc->gpio_flm_uart_sel_rev06 = pdata->gpio_flm_uart_sel_rev06;
#endif
	mc->gpio_cp_warm_reset = pdata->gpio_cp_warm_reset;
	mc->gpio_sim_detect = pdata->gpio_sim_detect;

#if defined(CONFIG_LINK_DEVICE_PLD)
	mc->gpio_fpga_cs_n = pdata->gpio_fpga2_cs_n;
#endif

	gpio_set_value(mc->gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_cp_on, 0);

	pdev = to_platform_device(mc->dev);
	mc->irq_phone_active = platform_get_irq_byname(pdev, "cp_active_irq");
	pr_info("[MSM] <%s> PHONE_ACTIVE IRQ# = %d\n",
		__func__, mc->irq_phone_active);

	mdm6600_get_ops(mc);

	ret = request_irq(mc->irq_phone_active,
			  phone_active_irq_handler,
			  IRQF_TRIGGER_HIGH, "msm_active", mc);
	if (ret) {
		pr_err("[MSM] <%s> failed to request_irq IRQ# %d (err=%d)\n",
		       __func__, mc->irq_phone_active, ret);
		return ret;
	}

	ret = enable_irq_wake(mc->irq_phone_active);
	if (ret) {
		pr_err("[MSM] %s: failed to enable_irq_wake IRQ# %d (err=%d)\n",
		       __func__, mc->irq_phone_active, ret);
		free_irq(mc->irq_phone_active, mc);
		return ret;
	}

#if defined(CONFIG_SIM_DETECT)
	mc->irq_sim_detect = platform_get_irq_byname(pdev, "sim_irq");
	pr_info("[MSM] <%s> SIM_DECTCT IRQ# = %d\n",
		__func__, mc->irq_sim_detect);

	if (mc->irq_sim_detect) {
		ret = request_irq(mc->irq_sim_detect, sim_detect_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"msm_sim_detect", mc);
		if (ret) {
			mif_err("[MSM] failed to request_irq: %d\n", ret);
			mc->sim_state.online = false;
			mc->sim_state.changed = false;
			return ret;
		}

		ret = enable_irq_wake(mc->irq_sim_detect);
		if (ret) {
			mif_err("[MSM] failed to enable_irq_wake: %d\n", ret);
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
