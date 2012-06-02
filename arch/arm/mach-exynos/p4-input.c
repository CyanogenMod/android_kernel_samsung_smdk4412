/*
 *  arch/arm/mach-exynos/p4-input.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <plat/gpio-cfg.h>
#include <plat/iic.h>

#if defined(CONFIG_RMI4_I2C)
#include <linux/rmi.h>
static int synaptics_tsp_pre_suspend(const void *pm_data)
{
	if (NULL == pm_data)
		return -1;
	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_SDA_18V, 0);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_SCL_18V, 0);

	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_INT, 0);
	s3c_gpio_cfgpin(GPIO_TSP_RST, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_RST, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_RST, 0);
	s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_LDO_ON, 0);

	return 0;
}

static int synaptics_tsp_post_resume(const void *pm_data)
{
	if (NULL == pm_data)
		return -1;
	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(0x3));
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(0x3));
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);

	s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_LDO_ON, 1);
	s3c_gpio_cfgpin(GPIO_TSP_RST, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_RST, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_RST, 1);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));

	return 0;
}

static void synaptics_tsp_reset(void)
{
	printk(KERN_DEBUG "[TSP] %s\n", __func__);
	s3c_gpio_cfgpin(GPIO_TSP_RST, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_RST, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_RST, 0);
	msleep(100);
	gpio_set_value(GPIO_TSP_RST, 1);
}

static struct rmi_device_platform_data synaptics_pdata = {
	.driver_name = "rmi-generic",
	.sensor_name = "s7301",
	.attn_gpio = GPIO_TSP_INT,
	.attn_polarity = RMI_ATTN_ACTIVE_LOW,
	.axis_align = { },
	.pm_data = NULL,
	.pre_suspend = synaptics_tsp_pre_suspend,
	.post_resume = synaptics_tsp_post_resume,
	.hw_reset = synaptics_tsp_reset,
};
#endif	/* CONFIG_RMI4_I2C */

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301)
#include <linux/synaptics_s7301.h>
static bool have_tsp_ldo;
static struct charger_callbacks *charger_callbacks;

void synaptics_ts_charger_infom(bool en)
{
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, en);
}

static void synaptics_ts_register_callback(struct charger_callbacks *cb)
{
	charger_callbacks = cb;
	printk(KERN_DEBUG "[TSP] %s\n", __func__);
}

static int synaptics_ts_set_power(bool en)
{
	if (!have_tsp_ldo)
		return -1;
	printk(KERN_DEBUG "[TSP] %s(%d)\n", __func__, en);

	if (en) {
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(0x3));
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(0x3));
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);

		s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_TSP_LDO_ON, 1);
		s3c_gpio_cfgpin(GPIO_TSP_RST, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_RST, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_TSP_RST, 1);
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	} else {
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_TSP_SDA_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_TSP_SCL_18V, 0);

		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_TSP_INT, 0);
		s3c_gpio_cfgpin(GPIO_TSP_RST, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_RST, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_TSP_RST, 0);
		s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_TSP_LDO_ON, 0);
	}

	return 0;
}

static void synaptics_ts_reset(void)
{
	printk(KERN_DEBUG "[TSP] %s\n", __func__);
	s3c_gpio_cfgpin(GPIO_TSP_RST, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_RST, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_RST, 0);
	msleep(100);
	gpio_set_value(GPIO_TSP_RST, 1);
}

static struct synaptics_platform_data synaptics_ts_pdata = {
	.gpio_attn = GPIO_TSP_INT,
	.max_x = 1279,
	.max_y = 799,
	.max_pressure = 255,
	.max_width = 100,
	.x_line = 27,
	.y_line = 42,
	.set_power = synaptics_ts_set_power,
	.hw_reset = synaptics_ts_reset,
	.register_cb = synaptics_ts_register_callback,
};
#endif	/* CONFIG_TOUCHSCREEN_SYNAPTICS_S7301 */

static struct i2c_board_info i2c_devs3[] __initdata = {
	{
#if defined(CONFIG_RMI4_I2C)
		I2C_BOARD_INFO(SYNAPTICS_RMI_NAME,
			SYNAPTICS_RMI_ADDR),
		.platform_data = &synaptics_pdata,
#endif	/* CONFIG_RMI4_I2C */
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301)
		I2C_BOARD_INFO(SYNAPTICS_TS_NAME,
			SYNAPTICS_TS_ADDR),
		.platform_data = &synaptics_ts_pdata,
#endif	/* CONFIG_TOUCHSCREEN_SYNAPTICS_S7301 */
	},
};

void __init p4_tsp_init(u32 system_rev)
{
	int gpio;

	printk(KERN_DEBUG "[TSP] %s rev : %u\n",
		__func__, system_rev);

	gpio = GPIO_TSP_RST;
	gpio_request(gpio, "TSP_RST");
	gpio_direction_output(gpio, 1);
	gpio_export(gpio, 0);

	gpio = GPIO_TSP_LDO_ON;
	gpio_request(gpio, "TSP_LDO_ON");
	gpio_direction_output(gpio, 1);
	gpio_export(gpio, 0);

	gpio = GPIO_TSP_INT;
	gpio_request(gpio, "TSP_INT");

	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	s5p_register_gpio_interrupt(gpio);
	i2c_devs3[0].irq = gpio_to_irq(gpio);
	if (1 <= system_rev)
#if defined(CONFIG_RMI4_I2C)
		synaptics_pdata.pm_data =
			(char *)synaptics_pdata.sensor_name;
#else
		have_tsp_ldo = true;
#endif

#ifdef CONFIG_S3C_DEV_I2C3
	s3c_i2c3_set_platdata(NULL);
	i2c_register_board_info(3, i2c_devs3,
		ARRAY_SIZE(i2c_devs3));
#endif
}

#if defined(CONFIG_EPEN_WACOM_G5SP)
#include <linux/wacom_i2c.h>
static struct wacom_g5_callbacks *wacom_callbacks;
static int wacom_init_hw(void);
static int wacom_suspend_hw(void);
static int wacom_resume_hw(void);
static int wacom_early_suspend_hw(void);
static int wacom_late_resume_hw(void);
static int wacom_reset_hw(void);
static void wacom_register_callbacks(struct wacom_g5_callbacks *cb);

static struct wacom_g5_platform_data wacom_platform_data = {
	.x_invert = 0,
	.y_invert = 0,
	.xy_switch = 0,
	.gpio_pendct = GPIO_PEN_PDCT_18V,
	.init_platform_hw = wacom_init_hw,
	.suspend_platform_hw = wacom_suspend_hw,
	.resume_platform_hw = wacom_resume_hw,
	.early_suspend_platform_hw = wacom_early_suspend_hw,
	.late_resume_platform_hw = wacom_late_resume_hw,
	.reset_platform_hw = wacom_reset_hw,
	.register_cb = wacom_register_callbacks,
};

static struct i2c_board_info i2c_devs6[] __initdata = {
	{
		I2C_BOARD_INFO("wacom_g5sp_i2c", 0x56),
		.platform_data = &wacom_platform_data,
	},
};

static void wacom_register_callbacks(struct wacom_g5_callbacks *cb)
{
	wacom_callbacks = cb;
};

static int wacom_init_hw(void)
{
	int ret;
	ret = gpio_request(GPIO_PEN_LDO_EN, "PEN_LDO_EN");
	if (ret) {
		printk(KERN_ERR "[E-PEN] faile to request gpio(GPIO_PEN_LDO_EN)\n");
		return ret;
	}
	s3c_gpio_cfgpin(GPIO_PEN_LDO_EN, S3C_GPIO_SFN(0x1));
	s3c_gpio_setpull(GPIO_PEN_LDO_EN, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_PEN_LDO_EN, 1);

	ret = gpio_request(GPIO_PEN_PDCT_18V, "PEN_PDCT");
	if (ret) {
		printk(KERN_ERR "[E-PEN] faile to request gpio(GPIO_PEN_PDCT_18V)\n");
		return ret;
	}
	s3c_gpio_cfgpin(GPIO_PEN_PDCT_18V, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_PEN_PDCT_18V, S3C_GPIO_PULL_UP);

	ret = gpio_request(GPIO_PEN_IRQ_18V, "PEN_IRQ");
	if (ret) {
		printk(KERN_ERR "[E-PEN] faile to request gpio(GPIO_PEN_IRQ_18V)\n");
		return ret;
	}
	s3c_gpio_cfgpin(GPIO_PEN_IRQ_18V, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_PEN_IRQ_18V, S3C_GPIO_PULL_DOWN);
	s5p_register_gpio_interrupt(GPIO_PEN_IRQ_18V);
	i2c_devs6[0].irq = gpio_to_irq(GPIO_PEN_IRQ_18V);
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

static int wacom_early_suspend_hw(void)
{
	gpio_set_value(GPIO_PEN_LDO_EN, 0);
	return 0;
}

static int wacom_late_resume_hw(void)
{
	gpio_set_value(GPIO_PEN_LDO_EN, 1);
	return 0;
}

static int wacom_reset_hw(void)
{
	return 0;
}

void __init p4_wacom_init(void)
{
	wacom_init_hw();
#ifdef CONFIG_S3C_DEV_I2C6
	s3c_i2c6_set_platdata(NULL);
	i2c_register_board_info(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));
#endif
}
#endif	/* CONFIG_EPEN_WACOM_G5SP */

#if defined(CONFIG_KEYBOARD_GPIO)
#include <mach/sec_debug.h>
#include <linux/gpio_keys.h>
#define GPIO_KEYS(_code, _gpio, _active_low, _iswake, _hook)	\
{							\
	.code = _code,					\
	.gpio = _gpio,					\
	.active_low = _active_low,			\
	.type = EV_KEY,					\
	.wakeup = _iswake,				\
	.debounce_interval = 10,			\
	.isr_hook = _hook,				\
	.value = 1					\
}

struct gpio_keys_button p4_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
};

struct gpio_keys_platform_data p4_gpiokeys_platform_data = {
	p4_buttons,
	ARRAY_SIZE(p4_buttons),
};

static struct platform_device p4_keypad = {
	.name	= "gpio-keys",
	.dev	= {
		.platform_data = &p4_gpiokeys_platform_data,
	},
};
#endif
void __init p4_key_init(void)
{
#if defined(CONFIG_KEYBOARD_GPIO)
	platform_device_register(&p4_keypad);
#endif
}

