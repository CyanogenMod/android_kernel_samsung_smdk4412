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
#include <linux/regulator/consumer.h>
#include <plat/gpio-cfg.h>
#include <plat/iic.h>

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1664S)
#include <linux/i2c/mxt1664s.h>
#endif

#if defined(CONFIG_KEYBOARD_GPIO)
#include <mach/sec_debug.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#endif

#define GPIO_TOUCH_EN	EXYNOS5_GPD1(1)

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1664S)

static int ts_power_on(void)
{
	struct regulator *regulator;

	/* touch reset pin */
	s3c_gpio_cfgpin(GPIO_TOUCH_RESET, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TOUCH_RESET, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TOUCH_RESET, 0);

	/* touch xvdd en pin */
	s3c_gpio_cfgpin(GPIO_TOUCH_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TOUCH_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TOUCH_EN, 0);

	regulator = regulator_get(NULL, "touch_vdd_1.8v");
	if (IS_ERR(regulator)) {
		printk(KERN_ERR "[TSP]ts_power_on : regulator_get failed\n");
		return -EIO;
	}

	regulator_enable(regulator);
	regulator_put(regulator);

	regulator = regulator_get(NULL, "touch_avdd");
	if (IS_ERR(regulator)) {
		printk(KERN_ERR "[TSP]ts_power_on : regulator_get failed\n");
		return -EIO;
	}
	regulator_enable(regulator);
	regulator_put(regulator);

	/* enable touch xvdd */
	gpio_set_value(GPIO_TOUCH_EN, 1);

	/* reset ic */
	mdelay(1);
	gpio_set_value(GPIO_TOUCH_RESET, 1);

	/* touch interrupt pin */
	/* s3c_gpio_cfgpin(GPIO_TOUCH_CHG, S3C_GPIO_INPUT); */

	s3c_gpio_cfgpin(GPIO_TOUCH_CHG, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_TOUCH_CHG, S3C_GPIO_PULL_NONE);

	msleep(MXT_1664S_HW_RESET_TIME);

	printk(KERN_ERR "mxt_power_on is finished\n");

	return 0;
}

static int ts_power_off(void)
{
	struct regulator *regulator;

	regulator = regulator_get(NULL, "touch_avdd");
	if (IS_ERR(regulator)) {
		printk(KERN_ERR "[TSP]ts_power_off : regulator_get failed\n");
		return -EIO;
	}

	if (regulator_is_enabled(regulator))
		regulator_force_disable(regulator);

	regulator_put(regulator);

	/* CAUTION : EVT1 board has CHG_INT problem
	* so it need a workaround code to ensure charging during sleep mode
	*/
	if (system_rev != 2) {
		regulator = regulator_get(NULL, "touch_vdd_1.8v");
		if (IS_ERR(regulator)) {
			printk(KERN_ERR "[TSP]ts_power_on : regulator_get failed\n");
			return -EIO;
		}

		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);

		regulator_put(regulator);
	}

	/* touch interrupt pin */
	s3c_gpio_cfgpin(GPIO_TOUCH_CHG, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TOUCH_CHG, S3C_GPIO_PULL_NONE);

	/* touch reset pin */
	s3c_gpio_cfgpin(GPIO_TOUCH_RESET, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TOUCH_RESET, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TOUCH_RESET, 0);

	/* touch xvdd en pin */
	s3c_gpio_cfgpin(GPIO_TOUCH_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TOUCH_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TOUCH_EN, 0);

	printk(KERN_ERR "mxt_power_off is finished\n");

	return 0;
}

/*
	Configuration for MXT1664-S
*/
#define MXT1664S_MAX_MT_FINGERS	10
#define MXT1664S_BLEN_BATT 208
#define MXT1664S_CHRGTIME_BATT  130
#define MXT1664S_THRESHOLD_BATT	70

static u8 t7_config_s[] = { GEN_POWERCONFIG_T7,
	255, 255, 50, 3
};

static u8 t8_config_s[] = { GEN_ACQUISITIONCONFIG_T8,
	MXT1664S_CHRGTIME_BATT, 0, 10, 10, 0, 0, 20, 35, 0, 0
};

static u8 t9_config_s[] = { TOUCH_MULTITOUCHSCREEN_T9,
	139, 0, 0, 32, 52, 0, MXT1664S_BLEN_BATT, MXT1664S_THRESHOLD_BATT, 2, 1,
	0, 5, 2, 0, MXT1664S_MAX_MT_FINGERS, 10, 20, 20, 63, 6,
	255, 9, 0, 0, 0, 0, 0, 0, 0, 0,
	15, 15, 42, 42, 0, 0
};

static u8 t15_config_s[] = { TOUCH_KEYARRAY_T15,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0
};

static u8 t18_config_s[] = { SPT_COMCONFIG_T18,
	0, 0
};

static u8 t24_config_s[] = {
	PROCI_ONETOUCHGESTUREPROCESSOR_T24,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 t25_config_s[] = {
	SPT_SELFTEST_T25,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 200
};

static u8 t27_config_s[] = {
	PROCI_TWOTOUCHGESTUREPROCESSOR_T27,
	0, 0, 0, 0, 0, 0, 0
};

static u8 t40_config_s[] = { PROCI_GRIPSUPPRESSION_T40,
	0, 0, 0, 0, 0
};

static u8 t42_config_s[] = { PROCI_TOUCHSUPPRESSION_T42,
	0, 60, 100, 60, 0, 20, 0, 0, 0, 0
};

static u8 t43_config_s[] = { SPT_DIGITIZER_T43,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0
};

static u8 t46_config_s[] = { SPT_CTECONFIG_T46,
	4, 0, 24, 24, 0, 0, 2, 0, 0, 0,
	0
};

static u8 t47_config_s[] = { PROCI_STYLUS_T47,
	73, 20, 45, 4, 5, 30, 1, 120, 3, 32,
	0, 0, 15, 0, 32, 230, 0, 0, 0, 0
};

static u8 t55_config_s[] = {ADAPTIVE_T55,
	0, 0, 0, 0, 0, 0
};

static u8 t56_config_s[] = {PROCI_SHIELDLESS_T56,
	1, 0, 1, 24, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 2, 16, 0, 2,
	0, 5, 0, 0, 0, 0, 0, 0, 0, 0,
	0
};

static u8 t57_config_s[] = {PROCI_EXTRATOUCHSCREENDATA_T57,
	226, 25, 0
};

static u8 t61_config_s[] = {SPT_TIMER_T61,
	0, 0, 0, 0, 0
};

static u8 t62_config_s[] = {PROCG_NOISESUPPRESSION_T62,
	3, 0, 0, 23, 2, 0, 0, 0, 50, 0,
	0, 0, 0, 0, 5, 0, 10, 3, 5, 144,
	50, 20, 48, 20, 100, 16, 16, 4, 255, 0,
	0, 0, 0, 0, 176, 80, 2, 5, 1, 48,
	10, 20, 30, 0, 0, 0, 0, 0, 0, 0,
	0, 16, 10, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};
static u8 end_config_s[] = { RESERVED_T255 };

static const u8 *MXT1644S_config[] = {
	t7_config_s,
	t8_config_s,
	t9_config_s,
	t15_config_s,
	t18_config_s,
	t24_config_s,
	t25_config_s,
	t27_config_s,
	t40_config_s,
	t42_config_s,
	t43_config_s,
	t46_config_s,
	t47_config_s,
	t55_config_s,
	t56_config_s,
	t57_config_s,
	t61_config_s,
	t62_config_s,
	end_config_s,
};

static struct mxt_platform_data mxt_data = {
	.max_finger_touches = MXT1664S_MAX_MT_FINGERS,
	.gpio_read_done = GPIO_TOUCH_CHG,
	.min_x = 0,
	.max_x = 2559,
	.min_y = 0,
	.max_y = 1599,
	.min_z = 0,
	.max_z = 255,
	.min_w = 0,
	.max_w = 255,
	.config = MXT1644S_config,
	.power_on = ts_power_on,
	.power_off = ts_power_off,
	.boot_address = 0x26,
};
#endif

static struct i2c_board_info i2c_devs3[] __initdata = {
	{
		I2C_BOARD_INFO(MXT_DEV_NAME, 0x4A),
		.platform_data = &mxt_data,
	}
};

void __init p10_tsp_init(void)
{
	int gpio;

	gpio = GPIO_TOUCH_CHG;
	gpio_request(gpio, "TSP_INT");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_INPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	s5p_register_gpio_interrupt(GPIO_TOUCH_CHG);
	i2c_devs3[0].irq = gpio_to_irq(gpio);

	s3c_i2c3_set_platdata(NULL);
	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));

	printk(KERN_ERR "%s touch : %d\n", __func__, i2c_devs3[0].irq);
}

#if defined(CONFIG_KEYBOARD_GPIO)
#if defined(CONFIG_SEC_DEBUG)
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

struct gpio_keys_button p10_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
};
#endif

struct gpio_keys_platform_data p10_gpiokeys_platform_data = {
	p10_buttons,
	ARRAY_SIZE(p10_buttons),
};

static struct platform_device p10_keypad = {
	.name	= "gpio-keys",
	.dev	= {
		.platform_data = &p10_gpiokeys_platform_data,
	},
};
#endif

void __init p10_key_init(void)
{
#if defined(CONFIG_KEYBOARD_GPIO)
	platform_device_register(&p10_keypad);
#endif
}

