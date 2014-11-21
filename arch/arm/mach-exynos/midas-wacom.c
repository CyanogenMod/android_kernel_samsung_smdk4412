/*
 * linux/arch/arm/mach-exynos/midas-wacom.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>

#include <linux/err.h>
#include <linux/gpio.h>

#include <linux/wacom_i2c.h>

#include <linux/regulator/consumer.h>
#include <plat/gpio-cfg.h>

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_FLEXRATE
#include <linux/cpufreq.h>
#endif

static struct wacom_g5_callbacks *wacom_callbacks;

#ifdef CONFIG_MACH_KONA
#define GPIO_WACOM_LDO_EN	GPIO_PEN_LDO_EN
#define GPIO_WACOM_SENSE	GPIO_PEN_DETECT
#endif

static int wacom_early_suspend_hw(void)
{
#ifndef CONFIG_MACH_KONA
	gpio_set_value(GPIO_PEN_RESET_N, 0);
#endif
#if defined(CONFIG_MACH_T0_EUR_OPEN) || defined(CONFIG_MACH_T0_CHN_OPEN)
	if (system_rev >= 10)
		gpio_direction_output(GPIO_WACOM_LDO_EN, 0);
	else
		gpio_direction_output(GPIO_WACOM_LDO_EN, 1);
#else
	gpio_direction_output(GPIO_WACOM_LDO_EN, 0);
#endif
	/* Set GPIO_PEN_IRQ to pull-up to reduce leakage */
	s3c_gpio_setpull(GPIO_PEN_IRQ, S3C_GPIO_PULL_UP);

	return 0;
}

static int wacom_late_resume_hw(void)
{
	s3c_gpio_setpull(GPIO_PEN_IRQ, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_WACOM_LDO_EN, 1);
	msleep(100);
#ifndef CONFIG_MACH_KONA
	gpio_set_value(GPIO_PEN_RESET_N, 1);
#endif
	return 0;
}

static int wacom_suspend_hw(void)
{
	return wacom_early_suspend_hw();
}

static int wacom_resume_hw(void)
{
	return wacom_late_resume_hw();
}

static int wacom_reset_hw(void)
{
	wacom_early_suspend_hw();
	msleep(100);
	wacom_late_resume_hw();

	return 0;
}

static void wacom_register_callbacks(struct wacom_g5_callbacks *cb)
{
	wacom_callbacks = cb;
};

#ifdef WACOM_HAVE_FWE_PIN
static void wacom_compulsory_flash_mode(bool en)
{
	gpio_set_value(GPIO_PEN_FWE1, en);
}
#endif

static struct wacom_g5_platform_data wacom_platform_data = {
	.x_invert = WACOM_X_INVERT,
	.y_invert = WACOM_Y_INVERT,
	.xy_switch = WACOM_XY_SWITCH,
	.min_x = 0,
	.max_x = WACOM_POSX_MAX,
	.min_y = 0,
	.max_y = WACOM_POSY_MAX,
	.min_pressure = 0,
	.max_pressure = WACOM_PRESSURE_MAX,
	.gpio_pendct = GPIO_PEN_PDCT,
#ifdef WACOM_STATE_CHECK
#if defined(CONFIG_TARGET_LOCALE_KOR)
#if defined(CONFIG_MACH_T0) && defined(CONFIG_TDMB_ANT_DET)
	.gpio_esd_check = GPIO_TDMB_ANT_DET_REV08,
#endif
#endif
#endif
	/*.init_platform_hw = midas_wacom_init,*/
	/*      .exit_platform_hw =,    */
	.suspend_platform_hw = wacom_suspend_hw,
	.resume_platform_hw = wacom_resume_hw,
	.early_suspend_platform_hw = wacom_early_suspend_hw,
	.late_resume_platform_hw = wacom_late_resume_hw,
	.reset_platform_hw = wacom_reset_hw,
	.register_cb = wacom_register_callbacks,
#ifdef WACOM_HAVE_FWE_PIN
	.compulsory_flash_mode = wacom_compulsory_flash_mode,
#endif
#ifdef WACOM_PEN_DETECT
	.gpio_pen_insert = GPIO_WACOM_SENSE,
#endif
};

/* I2C Setting */
static struct i2c_board_info wacom_i2c_devs[] __initdata = {
	{
		I2C_BOARD_INFO("wacom_g5sp_i2c", 0x56),
			.platform_data = &wacom_platform_data,
	},
};

void __init midas_wacom_init(void)
{
	int gpio;
	int ret;

#ifndef CONFIG_MACH_KONA
	/*RESET*/
	gpio = GPIO_PEN_RESET_N;
	ret = gpio_request(gpio, "PEN_RESET");
	if (ret) {
		printk(KERN_ERR "epen:failed to request PEN_RESET.(%d)\n",
			ret);
		return ;
	}
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	gpio_direction_output(gpio, 0);
#endif

	/*SLP & FWE1*/
#ifdef CONFIG_MACH_T0
	if (system_rev < WACOM_FWE1_HWID) {
		printk(KERN_INFO "epen:Use SLP\n");
		gpio = GPIO_PEN_SLP;
		ret = gpio_request(gpio, "PEN_SLP");
		if (ret) {
			printk(KERN_ERR "epen:failed to request PEN_SLP.(%d)\n",
				ret);
			return ;
		}
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0x1));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	} else {
		printk(KERN_INFO "epen:Use FWE\n");
		gpio = GPIO_PEN_FWE1;
		ret = gpio_request(gpio, "PEN_FWE1");
		if (ret) {
			printk(KERN_ERR "epen:failed to request PEN_FWE1.(%d)\n",
				ret);
			return ;
		}
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0x1));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
#elif defined(CONFIG_MACH_KONA)
	printk(KERN_INFO "epen:Use FWE\n");
	gpio = GPIO_PEN_FWE1;
	ret = gpio_request(gpio, "PEN_FWE1");
	if (ret) {
		printk(KERN_ERR "epen:failed to request PEN_FWE1.(%d)\n",
			ret);
		return ;
	}
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0x1));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
#endif
	gpio_direction_output(gpio, 0);

	/*PDCT*/
	gpio = GPIO_PEN_PDCT;
	ret = gpio_request(gpio, "PEN_PDCT");
	if (ret) {
		printk(KERN_ERR "epen:failed to request PEN_PDCT.(%d)\n",
			ret);
		return ;
	}
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	s5p_register_gpio_interrupt(gpio);
	gpio_direction_input(gpio);

	irq_set_irq_type(gpio_to_irq(gpio), IRQ_TYPE_EDGE_BOTH);

	/*IRQ*/
	gpio = GPIO_PEN_IRQ;
	ret = gpio_request(gpio, "PEN_IRQ");
	if (ret) {
		printk(KERN_ERR "epen:failed to request PEN_IRQ.(%d)\n",
			ret);
		return ;
	}
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	s5p_register_gpio_interrupt(gpio);
	gpio_direction_input(gpio);

	wacom_i2c_devs[0].irq = gpio_to_irq(gpio);
	irq_set_irq_type(wacom_i2c_devs[0].irq, IRQ_TYPE_EDGE_RISING);

	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));

	/*LDO_EN*/
	gpio = GPIO_WACOM_LDO_EN;
	ret = gpio_request(gpio, "PEN_LDO_EN");
	if (ret) {
		printk(KERN_ERR "epen:failed to request PEN_LDO_EN.(%d)\n",
			ret);
		return ;
	}
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	gpio_direction_output(gpio, 0);

#if defined(CONFIG_MACH_KONA)
	i2c_register_board_info(6, wacom_i2c_devs, ARRAY_SIZE(wacom_i2c_devs));
#elif defined(CONFIG_MACH_T0_EUR_OPEN) ||\
	(defined(CONFIG_TARGET_LOCALE_CHN) && !defined(CONFIG_MACH_T0_CHN_CTC))
	i2c_register_board_info(5, wacom_i2c_devs, ARRAY_SIZE(wacom_i2c_devs));
#else
	i2c_register_board_info(2, wacom_i2c_devs, ARRAY_SIZE(wacom_i2c_devs));
#endif

	printk(KERN_INFO "epen:: wacom IC initialized.\n");
}
