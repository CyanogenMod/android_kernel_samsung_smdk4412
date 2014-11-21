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
#include <linux/regulator/consumer.h>

static u32 hw_rev;

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
	struct regulator *regulator;

	if (!have_tsp_ldo)
		return -1;
	printk(KERN_DEBUG "[TSP] %s(%d)\n", __func__, en);

	regulator = regulator_get(NULL, "tsp_3.3v");
	if (IS_ERR(regulator))
		return PTR_ERR(regulator);

	if (en) {
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(0x3));
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(0x3));
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);

		if (regulator_is_enabled(regulator)) {
			printk(KERN_DEBUG "[TSP] regulator force disabled before enabling\n");
			regulator_force_disable(regulator);
			msleep(100);
		}

		regulator_enable(regulator);
		gpio_set_value(GPIO_TSP_LDO_ON, 1);

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
		s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_TSP_LDO_ON, 0);

		if (regulator_is_enabled(regulator))
			regulator_disable(regulator);
	}

	regulator_put(regulator);
	return 0;
}

static void synaptics_ts_reset(void)
{
	printk(KERN_DEBUG "[TSP] %s\n", __func__);
	synaptics_ts_set_power(false);
	msleep(100);
	synaptics_ts_set_power(true);
	msleep(200);
}

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYLED)
static void synaptics_ts_led_control(int on_off)
{
	printk(KERN_DEBUG "[TSP] %s [%d]\n", __func__, on_off);

	if (hw_rev < 1)
		return ;

	if (on_off == 1)
		gpio_direction_output(GPIO_TSP_2TOUCH_EN, 1);
	else
		gpio_direction_output(GPIO_TSP_2TOUCH_EN, 0);
}
#endif

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYS)
static u8 synaptics_button_codes[] = {KEY_MENU, KEY_BACK};
static u8 synaptics_extend_button_codes[] =
	{KEY_DUMMY_1, KEY_MENU, KEY_DUMMY_2, KEY_BACK, KEY_DUMMY_3};

static struct synaptics_button_map synpatics_button_map = {
	.nbuttons = ARRAY_SIZE(synaptics_button_codes),
	.map = synaptics_button_codes,
};

static struct synaptics_extend_button_map synptics_extend_button_map = {
	.nbuttons = ARRAY_SIZE(synaptics_extend_button_codes),
	.map = synaptics_extend_button_codes,
	.button_mask = BUTTON_0_MASK | BUTTON_2_MASK | BUTTON_4_MASK,
};
#endif

static struct synaptics_platform_data synaptics_ts_pdata = {
	.gpio_attn = GPIO_TSP_INT,
	.max_x = 799,
	.max_y = 1279,
	.max_pressure = 255,
	.max_width = 100,
	.x_line = 26,
	.y_line = 41,
	.swap_xy = false,
	.invert_x = false,
	.invert_y = false,
#if defined(CONFIG_SEC_TOUCHSCREEN_SURFACE_TOUCH)
	.palm_threshold = 28,
#endif
	.set_power = synaptics_ts_set_power,
	.hw_reset = synaptics_ts_reset,
	.register_cb = synaptics_ts_register_callback,
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYLED)
	.led_control = synaptics_ts_led_control,
	.led_event = false,
#endif
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYS)
	.button_map = &synpatics_button_map,
	.extend_button_map = &synptics_extend_button_map,
	.support_extend_button = false,
	.enable_extend_button_event = false,
#endif
};

static struct i2c_board_info i2c_synaptics[] __initdata = {
	{
		I2C_BOARD_INFO(SYNAPTICS_TS_NAME,
			SYNAPTICS_TS_ADDR),
		.platform_data = &synaptics_ts_pdata,
	},
};
#elif defined(CONFIG_RMI4_I2C)
#include <linux/interrupt.h>
#include <linux/rmi4.h>
#include <linux/input.h>

#define TOUCH_ON 1
#define TOUCH_OFF 0

#define RMI4_DEFAULT_ATTN_GPIO GPIO_TSP_INT
#define RMI4_DEFAULT_ATTN_NAME "TSP_INT"

struct syna_gpio_data {
	u16 gpio_number;
	char *gpio_name;
};

static bool have_tsp_ldo;

static struct syna_gpio_data rmi4_default_gpio_data = {
	.gpio_number = RMI4_DEFAULT_ATTN_GPIO,
	.gpio_name = RMI4_DEFAULT_ATTN_NAME,
};

#define SYNA_ADDR 0x20

static unsigned char SYNA_f1a_button_codes[] =  {KEY_DUMMY_1, KEY_MENU, KEY_DUMMY_2, KEY_BACK, KEY_DUMMY_3};

static struct rmi_button_map SYNA_f1a_button_map = {
	.nbuttons = ARRAY_SIZE(SYNA_f1a_button_codes),
	.map = SYNA_f1a_button_codes,
};

static int SYNA_ts_power(bool on_off)
{
	struct regulator *regulator;

	if (!have_tsp_ldo)
		return -1;
	printk(KERN_DEBUG "[TSP] %s(%d)\n", __func__, on_off);

	regulator = regulator_get(NULL, "tsp_3.3v");
	if (IS_ERR(regulator))
		return PTR_ERR(regulator);

	if (on_off) {
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(0x3));
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(0x3));
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_TSP_LDO_ON, 1);

		regulator_enable(regulator);

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
		s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_TSP_LDO_ON, 0);

		if (regulator_is_enabled(regulator))
			regulator_disable(regulator);
	}

	regulator_put(regulator);
	msleep(200);
	return 0;
}

static int synaptics_touchpad_gpio_setup(void *gpio_data, bool configure)
{
	return SYNA_ts_power(configure);
}

int SYNA_post_suspend(void *pm_data) {
	pr_info("%s: RMI4 callback.\n", __func__);
	return SYNA_ts_power(TOUCH_OFF);
}

int SYNA_pre_resume(void *pm_data) {
	pr_info("%s: RMI4 callback.\n", __func__);
	return SYNA_ts_power(TOUCH_ON);
}

static struct rmi_device_platform_data SYNA_platformdata = {
	.sensor_name = "s7301",
	.attn_gpio = RMI4_DEFAULT_ATTN_GPIO,
	.attn_polarity = RMI_ATTN_ACTIVE_LOW,
	.gpio_data = &rmi4_default_gpio_data,
	.gpio_config = synaptics_touchpad_gpio_setup,
	.f1a_button_map = &SYNA_f1a_button_map,
//	.reset_delay_ms = 200,
#ifdef CONFIG_PM
	.post_suspend = SYNA_post_suspend,
	.pre_resume = SYNA_pre_resume,
#endif
#ifdef CONFIG_RMI4_FWLIB
	.firmware_name = "KONA-E036",
#endif
};

static struct i2c_board_info __initdata i2c_synaptics[] = {
	{
         I2C_BOARD_INFO("rmi_i2c", SYNA_ADDR),
        .platform_data = &SYNA_platformdata,
	},
};

#endif	/* CONFIG_RMI4_I2C */

void __init kona_tsp_init(u32 system_rev)
{
	int gpio = 0, irq = 0, err = 0;
	hw_rev = system_rev;

	printk(KERN_DEBUG "[TSP] %s rev : %u\n",
		__func__, hw_rev);

	gpio = GPIO_TSP_LDO_ON;
	gpio_request(gpio, "TSP_LDO_ON");
    gpio_direction_output(gpio, 0);
	gpio_export(gpio, 0);

	have_tsp_ldo = true;

	gpio = GPIO_TSP_INT;
	gpio_request(gpio, "TSP_INT");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	s5p_register_gpio_interrupt(gpio);
	irq = gpio_to_irq(gpio);

#ifdef CONFIG_S3C_DEV_I2C3
	s3c_i2c3_set_platdata(NULL);
	i2c_synaptics[0].irq = irq;
	i2c_register_board_info(3, i2c_synaptics,
		ARRAY_SIZE(i2c_synaptics));
#endif	/* CONFIG_S3C_DEV_I2C3 */

#if defined(CONFIG_MACH_KONA_EUR_OPEN)
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYS)
	/* rev01 touch button0 & button1 position change */
	if (system_rev == 1) {
		synaptics_ts_pdata.button_map->map[0] = KEY_BACK;
		synaptics_ts_pdata.button_map->map[1] = KEY_MENU;
	}
#endif
#endif
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301_KEYLED)
	if (system_rev > 0) {
		synaptics_ts_pdata.led_event = true;
		err = gpio_request(GPIO_TSP_2TOUCH_EN, "GPIO_TSP_2TOUCH_EN");
		if (err)
			printk(KERN_DEBUG "%s gpio_request error\n", __func__);
		else {
			s3c_gpio_cfgpin(GPIO_TSP_2TOUCH_EN, S3C_GPIO_OUTPUT);
			s3c_gpio_setpull(GPIO_TSP_2TOUCH_EN, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_TSP_2TOUCH_EN, 0);
		}
	}

	/*
     * button changed 2button -> 5button
     * KONA 3G, WIFI: gpio >= 3
     * KONA LTE : gpio >=2
     */

#if defined(CONFIG_MACH_KONA_EUR_LTE) ||	\
	defined(CONFIG_MACH_KONALTE_USA_ATT)
    if (system_rev >= 2) {
#else
    if (system_rev >= 3) {
#endif
		synaptics_ts_pdata.support_extend_button = true;
        synaptics_ts_pdata.enable_extend_button_event = true;
	}
#endif
}

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

struct gpio_keys_button kona_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_HOMEPAGE, GPIO_OK_KEY_ANDROID,
		  1, 1, sec_debug_check_crash_key),
	{
		.code = SW_FLIP,
		.gpio = GPIO_HALL_SENSOR_INT,
		.active_low = 0,
		.type = EV_SW,
		.wakeup = 1,
		.debounce_interval = 10,
		.value = 1,
		.isr_hook = sec_debug_check_crash_key,
	},
};

struct gpio_keys_platform_data kona_gpiokeys_platform_data = {
	kona_buttons,
	ARRAY_SIZE(kona_buttons),
};

static struct platform_device kona_keypad = {
	.name	= "gpio-keys",
	.dev	= {
		.platform_data = &kona_gpiokeys_platform_data,
	},
};
#endif
void __init kona_key_init(void)
{
#if defined(CONFIG_KEYBOARD_GPIO)
	platform_device_register(&kona_keypad);
#endif



}
