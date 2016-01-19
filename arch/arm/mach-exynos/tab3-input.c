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
#include <linux/input.h>
#include <plat/gpio-cfg.h>
#include <plat/iic.h>
#include <linux/regulator/consumer.h>

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT1188S
#include <linux/interrupt.h>
#include <linux/i2c/mxt1188s.h>

/*
  * Please locate model dependent define at below position
  * such as bootloader address, app address and firmware name
  */
#define MXT_BOOT_ADDRESS	0x26
#define MXT_APP_ADDRESS		0x4A

/* We need to support two types of fw at once,
 * So two firmwwares are loaded, and we need to add proper firmware name
 * to platform data according to system revision.
 *
 * REV_BASE : not support touchkey.
 * REV          : support touchkey.
 */

//#define MXT_FIRMWARE_NAME_REVISION_BASE	"mXT1664s_R_base.fw"
#if !defined(CONFIG_QC_MODEM)
#define MXT_FIRMWARE_NAME_REVISION	"mXT1188S.fw"
#define MXT_FIRMWARE_NAME_REVISION_OLD "mXT1188S_old.fw"
#else
#define MXT_FIRMWARE_NAME_REVISION	"mXT1188S_4G.fw"
#endif
/* To display configuration version on *#2663# */
#define MXT_PROJECT_NAME	"GT-P52XX"

static struct mxt_callbacks *charger_callbacks;

void tsp_charger_infom(bool en)
{
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, en);
}

static void mxt_register_callback(void *cb)
{
	charger_callbacks = cb;
}

static bool mxt_read_chg(void)
{
	return gpio_get_value(GPIO_TSP_INT);
}

static int mxt_power_on(void)
{
	int gpio = 0;

	gpio = GPIO_TSP_SDA_18V;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_set_value(gpio, 1);

	gpio = GPIO_TSP_SCL_18V;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_set_value(gpio, 1);

	gpio = GPIO_TSP_LDO_ON;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_set_value(gpio, 1);

	gpio = GPIO_TSP_LDO_ON1;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_set_value(gpio, 1);

	msleep(20);

	gpio = GPIO_TSP_LDO_ON2;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_set_value(gpio, 1);

	msleep(20);
	gpio = GPIO_TSP_RST;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_set_value(gpio, 1);

	/* touch interrupt pin */
	gpio = GPIO_TSP_INT;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);

	gpio = GPIO_TSP_SDA_18V;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0x3));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);

	gpio = GPIO_TSP_SCL_18V;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0x3));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);

	printk(KERN_ERR "mxt_power_on is finished\n");

	return 0;
}

static int mxt_power_off(void)
{
	int gpio = 0;

	gpio = GPIO_TSP_SDA_18V;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_set_value(gpio, 0);

	gpio = GPIO_TSP_SCL_18V;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_set_value(gpio, 0);

	/* touch xvdd en pin */
	gpio = GPIO_TSP_LDO_ON2;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_set_value(gpio, 0);

	gpio = GPIO_TSP_LDO_ON1;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_set_value(gpio, 0);

	gpio = GPIO_TSP_LDO_ON;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_set_value(gpio, 0);

	/* touch interrupt pin */
	gpio = GPIO_TSP_INT;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_DOWN);

	/* touch reset pin */
	gpio = GPIO_TSP_RST;
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_set_value(gpio, 0);

	printk(KERN_ERR "mxt_power_off is finished\n");

	return 0;
}

#if ENABLE_TOUCH_KEY
struct mxt_touchkey mxt_touchkeys[] = {
	{
		.value = 0x01,
		.keycode = KEY_BACK,
		.name = "back",
		.x_node = 25,
		.y_node = 43,
	},
	{
		.value = 0x02,
		.keycode = KEY_MENU,
		.name = "menu",
		.x_node = 26,
		.y_node = 43,
	},
};

static int mxt_led_power_on(void)
{

	gpio_direction_output(GPIO_TSK_EN, 1);

	printk(KERN_ERR "mxt_led_power_on is finished\n");
	return 0;
}

static int mxt_led_power_off(void)
{
	gpio_direction_output(GPIO_TSK_EN, 0);
	
	printk(KERN_ERR "mxt_led_power_off is finished\n");
	return 0;
}
#endif

static void mxt_gpio_init(void)
{

	int gpio = 0;

	gpio = GPIO_TSP_LDO_ON2;
	gpio_request(gpio, "TSP_LDO_ON2");
	gpio_direction_output(gpio, 0);
	gpio_export(gpio, 0);

	gpio = GPIO_TSP_LDO_ON1;
	gpio_request(gpio, "TSP_LDO_ON1");
	gpio_direction_output(gpio, 0);
	gpio_export(gpio, 0);

	gpio = GPIO_TSP_LDO_ON;
	gpio_request(gpio, "TSP_LDO_ON");
	gpio_direction_output(gpio, 0);
	gpio_export(gpio, 0);

	gpio = GPIO_TSP_RST;
	gpio_request(gpio, "TSP_RST");
	gpio_direction_output(gpio, 0);
	gpio_export(gpio, 0);
	
	gpio = GPIO_TSP_INT;
	gpio_request(gpio, "TSP_INT");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	s5p_register_gpio_interrupt(gpio);

	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

}

static struct mxt_platform_data mxt_data = {
	.num_xnode = 27,
	.num_ynode = 44,
	.max_x = 4095,
	.max_y = 4095,
	.irqflags = IRQF_TRIGGER_LOW | IRQF_ONESHOT,
	.boot_address = MXT_BOOT_ADDRESS,
	.project_name = MXT_PROJECT_NAME,
	.revision = MXT_REVISION_G,
	.read_chg = mxt_read_chg,
	.power_on = mxt_power_on,
	.power_off = mxt_power_off,
	.register_cb = mxt_register_callback,
#if ENABLE_TOUCH_KEY
	.num_touchkey = 2,
	.touchkey = mxt_touchkeys,
	.led_power_on = mxt_led_power_on,
	.led_power_off = mxt_led_power_off,
#endif
};

static struct i2c_board_info mxt_i2c_devs3[] __initdata = {
	{
		I2C_BOARD_INFO(MXT_DEV_NAME, MXT_APP_ADDRESS),
		.platform_data = &mxt_data,
	}
};

void __init tab3_tsp_init(u32 system_rev)
{
	mxt_gpio_init();

	mxt_i2c_devs3[0].irq = gpio_to_irq(GPIO_TSP_INT);

	s3c_i2c3_set_platdata(NULL);
	i2c_register_board_info(3, mxt_i2c_devs3,
			ARRAY_SIZE(mxt_i2c_devs3));

#if !defined(CONFIG_QC_MODEM)
	if(system_rev < 2)
		mxt_data.firmware_name = MXT_FIRMWARE_NAME_REVISION_OLD;
	else
		mxt_data.firmware_name = MXT_FIRMWARE_NAME_REVISION;
#else
	mxt_data.firmware_name = MXT_FIRMWARE_NAME_REVISION;
#endif

	printk(KERN_ERR "%s touch : %d [%d]\n",
		 __func__, mxt_i2c_devs3[0].irq, system_rev);
}
#endif

#if defined(CONFIG_TOUCHSCREEN_MMS152_TAB3)
#include <linux/i2c/mms152_tab3.h>
static bool tsp_power_enabled;
struct tsp_callbacks *tsp_callbacks;
struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *, bool);
};

void tsp_charger_infom(bool en)
{
	if (tsp_callbacks && tsp_callbacks->inform_charger)
		tsp_callbacks->inform_charger(tsp_callbacks, en);
}

static void melfas_register_charger_callback(void *cb)
{
	tsp_callbacks = cb;
	pr_debug("[TSP] melfas_register_lcd_callback\n");
}

int melfas_power(bool on)
{
	struct regulator *regulator_vdd;
	struct regulator *regulator_pullup;

	if (tsp_power_enabled == on)
		return 0;

	printk(KERN_DEBUG "[TSP] %s %s\n",
		__func__, on ? "on" : "off");

	regulator_vdd = regulator_get(NULL, "tsp_vdd_3.3v");
	if (IS_ERR(regulator_vdd))
		return PTR_ERR(regulator_vdd);
	regulator_pullup = regulator_get(NULL, "tsp_vdd_1.8v");
	if (IS_ERR(regulator_pullup))
		return PTR_ERR(regulator_pullup);

	if (on) {
		regulator_enable(regulator_vdd);
		regulator_enable(regulator_pullup);
	} else {
		if (regulator_is_enabled(regulator_vdd))
			regulator_disable(regulator_vdd);
		else
			regulator_force_disable(regulator_vdd);

		if (regulator_is_enabled(regulator_pullup))
			regulator_disable(regulator_pullup);
		else
			regulator_force_disable(regulator_pullup);
	}
	regulator_put(regulator_vdd);
	regulator_put(regulator_pullup);

	tsp_power_enabled = on;

	return 0;
}

int is_melfas_vdd_on(void)
{
	static struct regulator *regulator;
	int ret;

	if (!regulator) {
		regulator = regulator_get(NULL, "tsp_vdd_3.3v");
		if (IS_ERR(regulator)) {
			ret = PTR_ERR(regulator);
			pr_err("could not get touch, rc = %d\n", ret);
			return ret;
		}
	}

	if (regulator_is_enabled(regulator))
		return 1;
	else
		return 0;
}

int melfas_mux_fw_flash(bool to_gpios)
{
	pr_info("%s:to_gpios=%d\n", __func__, to_gpios);

	/* TOUCH_EN is always an output */
	if (to_gpios) {
		if (gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL"))
			pr_err("failed to request gpio(GPIO_TSP_SCL)\n");
		if (gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA"))
			pr_err("failed to request gpio(GPIO_TSP_SDA)\n");

		gpio_direction_output(GPIO_TSP_INT, 0);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SCL_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

	} else {
		gpio_direction_output(GPIO_TSP_INT, 1);
		gpio_direction_input(GPIO_TSP_INT);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
		/*S3C_GPIO_PULL_UP */

		gpio_direction_output(GPIO_TSP_SCL_18V, 1);
		gpio_direction_input(GPIO_TSP_SCL_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 1);
		gpio_direction_input(GPIO_TSP_SDA_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

		gpio_free(GPIO_TSP_SCL_18V);
		gpio_free(GPIO_TSP_SDA_18V);
	}
	return 0;
}

void melfas_set_touch_i2c(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	gpio_free(GPIO_TSP_SDA_18V);
	gpio_free(GPIO_TSP_SCL_18V);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	/* s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP); */
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
}

void melfas_set_touch_i2c_to_gpio(void)
{
	int ret;
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	ret = gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SDA)\n");
	ret = gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SCL)\n");

}

static struct melfas_tsi_platform_data mms_ts_pdata = {
	.max_x = 800,
	.max_y = 1280,
#if 0
	.invert_x = 800,
	.invert_y = 1280,
#else
	.invert_x = 0,
	.invert_y = 0,
#endif
	.gpio_int = GPIO_TSP_INT,
	.gpio_scl = GPIO_TSP_SCL_18V,
	.gpio_sda = GPIO_TSP_SDA_18V,
	.power = melfas_power,
	.mux_fw_flash = melfas_mux_fw_flash,
	.is_vdd_on = is_melfas_vdd_on,
/*	.set_touch_i2c		= melfas_set_touch_i2c, */
/*	.set_touch_i2c_to_gpio	= melfas_set_touch_i2c_to_gpio, */
/*	.lcd_type = melfas_get_lcdtype,*/
	.register_cb = melfas_register_charger_callback,
#ifdef CONFIG_LCD_FREQ_SWITCH
	.register_lcd_cb = melfas_register_lcd_callback,
#endif
};

static struct i2c_board_info i2c_devs3[] = {
	{
	 I2C_BOARD_INFO(MELFAS_TS_NAME, 0x48),
	 .platform_data = &mms_ts_pdata},
};

void __init midas_tsp_set_platdata(struct melfas_tsi_platform_data *pdata)
{
	if (!pdata)
		pdata = &mms_ts_pdata;

	i2c_devs3[0].platform_data = pdata;
}

void __init tab3_tsp_init(u32 system_rev)
{
	int gpio;
	int ret;
	printk(KERN_INFO "[TSP] midas_tsp_init() is called\n");

	gpio_request(GPIO_TSP_VENDOR1, "GPIO_TSP_VENDOR1");
	gpio_request(GPIO_TSP_VENDOR2, "GPIO_TSP_VENDOR2");
	gpio_request(GPIO_TSP_VENDOR3, "GPIO_TSP_VENDOR3");

	gpio_direction_input(GPIO_TSP_VENDOR1);
	gpio_direction_input(GPIO_TSP_VENDOR2);
	gpio_direction_input(GPIO_TSP_VENDOR3);

	s3c_gpio_setpull(GPIO_TSP_VENDOR1, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_VENDOR2, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_VENDOR3, S3C_GPIO_PULL_NONE);

	/* touchkey led */
	gpio_request(GPIO_TSK_EN, "GPIO_TSK_EN");
	gpio_direction_output(GPIO_TSK_EN, 0);

	/* TSP_INT: XEINT_4 */
	gpio = GPIO_TSP_INT;
	ret = gpio_request(gpio, "TSP_INT");
	if (ret)
		pr_err("failed to request gpio(TSP_INT)\n");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

	s5p_register_gpio_interrupt(gpio);
	i2c_devs3[0].irq = gpio_to_irq(gpio);

	printk(KERN_INFO "%s touch : %d\n", __func__, i2c_devs3[0].irq);

	melfas_power(0);

	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
}

#endif

#if defined(CONFIG_TOUCHSCREEN_MMS252)
#include <linux/i2c/mms252.h>
static bool tsp_power_enabled;
struct tsp_callbacks *tsp_callbacks;
struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *, bool);
};

void tsp_charger_infom(bool en)
{
	if (tsp_callbacks && tsp_callbacks->inform_charger)
		tsp_callbacks->inform_charger(tsp_callbacks, en);
}

static void melfas_register_charger_callback(void *cb)
{
	tsp_callbacks = cb;
	pr_debug("[TSP] melfas_register_lcd_callback\n");
}

int melfas_power(bool on)
{
	struct regulator *regulator_vdd;
	struct regulator *regulator_pullup;

	if (tsp_power_enabled == on)
		return 0;

	printk(KERN_DEBUG "[TSP] %s %s\n",
		__func__, on ? "on" : "off");

	regulator_vdd = regulator_get(NULL, "tsp_vdd_3.3v");
	if (IS_ERR(regulator_vdd))
		return PTR_ERR(regulator_vdd);
	regulator_pullup = regulator_get(NULL, "tsp_vdd_1.8v");
	if (IS_ERR(regulator_pullup))
		return PTR_ERR(regulator_pullup);

	if (on) {
		regulator_enable(regulator_vdd);
		usleep_range(2500, 3000);
		regulator_enable(regulator_pullup);
	} else {
		if (regulator_is_enabled(regulator_vdd))
			regulator_disable(regulator_vdd);
		else
			regulator_force_disable(regulator_vdd);

		if (regulator_is_enabled(regulator_pullup))
			regulator_disable(regulator_pullup);
		else
			regulator_force_disable(regulator_pullup);
	}
	regulator_put(regulator_vdd);
	regulator_put(regulator_pullup);

	tsp_power_enabled = on;

	return 0;
}

#if TOUCHKEY
static bool tsp_keyled_enabled;
int key_led_control(bool on)
{
	struct regulator *regulator;

	if (tsp_keyled_enabled == on)
		return 0;

	printk(KERN_DEBUG "[TSP] %s %s\n",
		__func__, on ? "on" : "off");

	regulator = regulator_get(NULL, "vtouch_3.3v");
	if (IS_ERR(regulator))
		return PTR_ERR(regulator);

	if (on) {
		regulator_enable(regulator);
	} else {
		if (regulator_is_enabled(regulator))
			regulator_disable(regulator);
		else
			regulator_force_disable(regulator);
	}
	regulator_put(regulator);

	tsp_keyled_enabled = on;

	return 0;
}

#endif

int is_melfas_vdd_on(void)
{
	static struct regulator *regulator;
	int ret;

	if (!regulator) {
		regulator = regulator_get(NULL, "tsp_vdd_3.3v");
		if (IS_ERR(regulator)) {
			ret = PTR_ERR(regulator);
			pr_err("could not get touch, rc = %d\n", ret);
			return ret;
		}
	}

	if (regulator_is_enabled(regulator))
		return 1;
	else
		return 0;
}

int melfas_mux_fw_flash(bool to_gpios)
{
	pr_info("%s:to_gpios=%d\n", __func__, to_gpios);

	/* TOUCH_EN is always an output */
	if (to_gpios) {
		if (gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL"))
			pr_err("failed to request gpio(GPIO_TSP_SCL)\n");
		if (gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA"))
			pr_err("failed to request gpio(GPIO_TSP_SDA)\n");

		gpio_direction_output(GPIO_TSP_INT, 0);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SCL_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

	} else {
		gpio_direction_output(GPIO_TSP_INT, 1);
		gpio_direction_input(GPIO_TSP_INT);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
		/*S3C_GPIO_PULL_UP */

		gpio_direction_output(GPIO_TSP_SCL_18V, 1);
		gpio_direction_input(GPIO_TSP_SCL_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 1);
		gpio_direction_input(GPIO_TSP_SDA_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

		gpio_free(GPIO_TSP_SCL_18V);
		gpio_free(GPIO_TSP_SDA_18V);
	}
	return 0;
}

void melfas_set_touch_i2c(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	gpio_free(GPIO_TSP_SDA_18V);
	gpio_free(GPIO_TSP_SCL_18V);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	/* s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP); */
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
}

void melfas_set_touch_i2c_to_gpio(void)
{
	int ret;
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	ret = gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SDA)\n");
	ret = gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SCL)\n");

}

static struct melfas_tsi_platform_data mms_ts_pdata = {
	.max_x = 800,
	.max_y = 1280,
#if 0
	.invert_x = 800,
	.invert_y = 1280,
#else
	.invert_x = 0,
	.invert_y = 0,
#endif
	.gpio_int = GPIO_TSP_INT,
	.gpio_scl = GPIO_TSP_SCL_18V,
	.gpio_sda = GPIO_TSP_SDA_18V,
	.power = melfas_power,
	.mux_fw_flash = melfas_mux_fw_flash,
	.is_vdd_on = is_melfas_vdd_on,
#if TOUCHKEY
	.keyled = key_led_control,
#endif
/*	.set_touch_i2c		= melfas_set_touch_i2c, */
/*	.set_touch_i2c_to_gpio	= melfas_set_touch_i2c_to_gpio, */
	.register_cb = melfas_register_charger_callback,
};

static struct i2c_board_info i2c_devs3[] = {
	{
		I2C_BOARD_INFO(MELFAS_TS_NAME, 0x48),
		.platform_data = &mms_ts_pdata
	},
};

void __init midas_tsp_set_platdata(struct melfas_tsi_platform_data *pdata)
{
	if (!pdata)
		pdata = &mms_ts_pdata;

	i2c_devs3[0].platform_data = pdata;
}

static int get_panel_version(void)
{
	u8 panel_version;

	if (gpio_get_value(GPIO_TSP_VENDOR1))
		panel_version = 1;
	else
		panel_version = 0;
	if (gpio_get_value(GPIO_TSP_VENDOR2))
		panel_version = panel_version | (1 << 1);

	if (gpio_get_value(GPIO_TSP_VENDOR3))
		panel_version = panel_version | (1 << 2);

	return (int)panel_version;
}

void __init tab3_tsp_init(u32 system_rev)
{
	int gpio;
	int ret;
	u8 panel;
	printk(KERN_INFO "[TSP] midas_tsp_init() is called\n");

	gpio_request(GPIO_TSP_VENDOR1, "GPIO_TSP_VENDOR1");
	gpio_request(GPIO_TSP_VENDOR2, "GPIO_TSP_VENDOR2");
	gpio_request(GPIO_TSP_VENDOR3, "GPIO_TSP_VENDOR3");

	gpio_direction_input(GPIO_TSP_VENDOR1);
	gpio_direction_input(GPIO_TSP_VENDOR2);
	gpio_direction_input(GPIO_TSP_VENDOR3);

	s3c_gpio_setpull(GPIO_TSP_VENDOR1, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_VENDOR2, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_VENDOR3, S3C_GPIO_PULL_NONE);

	/* TSP_INT: XEINT_4 */
	gpio = GPIO_TSP_INT;
	ret = gpio_request(gpio, "TSP_INT");
	if (ret)
		pr_err("failed to request gpio(TSP_INT)\n");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

	s5p_register_gpio_interrupt(gpio);
	i2c_devs3[0].irq = gpio_to_irq(gpio);

	printk(KERN_INFO "%s touch irq : %d (%d)\n",
		__func__, i2c_devs3[0].irq, system_rev);

	panel = get_panel_version();
	printk(KERN_INFO "TSP ID : %d\n", panel);
	if (system_rev < 3) {
		if (panel == 1) /* yongfast */
			mms_ts_pdata.panel = 0x8;
		else if (panel == 2) /* wintec */
			mms_ts_pdata.panel = 0x9;
		else
			mms_ts_pdata.panel = panel;
	} else
		mms_ts_pdata.panel = panel;

	printk(KERN_INFO "touch panel : %d\n",
		mms_ts_pdata.panel);

	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
}

#endif

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_BT532)
#include <linux/i2c/bt532_ts.h>
static bool tsp_power_enabled;
struct tsp_callbacks *tsp_callbacks;
struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *, bool);
};

void tsp_charger_infom(bool en)
{
	if (tsp_callbacks && tsp_callbacks->inform_charger)
		tsp_callbacks->inform_charger(tsp_callbacks, en);
}

static void zinitix_register_charger_callback(void *cb)
{
	tsp_callbacks = cb;
	pr_debug("[TSP] zinitix_register_lcd_callback\n");
}

int zinitix_power(bool on)
{
	struct regulator *regulator_vdd;
	struct regulator *regulator_pullup;

	if (tsp_power_enabled == on)
		return 0;

	printk(KERN_DEBUG "[TSP] %s %s\n",
		__func__, on ? "on" : "off");

	regulator_vdd = regulator_get(NULL, "tsp_vdd_3.3v");
	if (IS_ERR(regulator_vdd))
		return PTR_ERR(regulator_vdd);
	regulator_pullup = regulator_get(NULL, "tsp_vdd_1.8v");
	if (IS_ERR(regulator_pullup))
		return PTR_ERR(regulator_pullup);

	if (on) {
		regulator_enable(regulator_vdd);
		regulator_enable(regulator_pullup);
	} else {
		if (regulator_is_enabled(regulator_vdd))
			regulator_disable(regulator_vdd);
		else
			regulator_force_disable(regulator_vdd);

		if (regulator_is_enabled(regulator_pullup))
			regulator_disable(regulator_pullup);
		else
			regulator_force_disable(regulator_pullup);
	}
	regulator_put(regulator_vdd);
	regulator_put(regulator_pullup);

	tsp_power_enabled = on;

	return 0;
}

static void zinitix_led_power(bool on)
{
	int ret = 0;
	struct regulator *regulator_led;

	regulator_led = regulator_get(NULL, "vtouch_3.3v");
	if (IS_ERR(regulator_led)) {
		pr_err("%s, failed to get regulator vtouch_3.3v\n",
			__func__);
		return ;
	}

	if (on)
		ret = regulator_enable(regulator_led);
	else {
		if (regulator_is_enabled(regulator_led))
			ret = regulator_disable(regulator_led);
		else
			regulator_force_disable(regulator_led);
	}
	regulator_put(regulator_led);

	printk(KERN_INFO "%s: %s (%d)\n", __func__, (on) ? "on" : "off", ret);
}

int is_zinitix_vdd_on(void)
{
	static struct regulator *regulator;
	int ret;

	if (!regulator) {
		regulator = regulator_get(NULL, "tsp_vdd_3.3v");
		if (IS_ERR(regulator)) {
			ret = PTR_ERR(regulator);
			pr_err("could not get touch, rc = %d\n", ret);
			return ret;
		}
	}

	if (regulator_is_enabled(regulator))
		return 1;
	else
		return 0;
}

int zinitix_mux_fw_flash(bool to_gpios)
{
	pr_info("%s:to_gpios=%d\n", __func__, to_gpios);

	/* TOUCH_EN is always an output */
	if (to_gpios) {
		if (gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL"))
			pr_err("failed to request gpio(GPIO_TSP_SCL)\n");
		if (gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA"))
			pr_err("failed to request gpio(GPIO_TSP_SDA)\n");

		gpio_direction_output(GPIO_TSP_INT, 0);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SCL_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 0);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

	} else {
		gpio_direction_output(GPIO_TSP_INT, 1);
		gpio_direction_input(GPIO_TSP_INT);
		s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
		/*S3C_GPIO_PULL_UP */

		gpio_direction_output(GPIO_TSP_SCL_18V, 1);
		gpio_direction_input(GPIO_TSP_SCL_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

		gpio_direction_output(GPIO_TSP_SDA_18V, 1);
		gpio_direction_input(GPIO_TSP_SDA_18V);
		s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);

		gpio_free(GPIO_TSP_SCL_18V);
		gpio_free(GPIO_TSP_SDA_18V);
	}
	return 0;
}

void zinitix_set_touch_i2c(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	gpio_free(GPIO_TSP_SDA_18V);
	gpio_free(GPIO_TSP_SCL_18V);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	/* s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP); */
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
}

void zinitix_set_touch_i2c_to_gpio(void)
{
	int ret;
	s3c_gpio_cfgpin(GPIO_TSP_SDA_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TSP_SCL_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_UP);
	ret = gpio_request(GPIO_TSP_SDA_18V, "GPIO_TSP_SDA");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SDA)\n");
	ret = gpio_request(GPIO_TSP_SCL_18V, "GPIO_TSP_SCL");
	if (ret)
		pr_err("failed to request gpio(GPIO_TSP_SCL)\n");

}

struct i2c_client *bt404_i2c_client = NULL;

void put_isp_i2c_client(struct i2c_client *client)
{
	bt404_i2c_client = client;
}

struct i2c_client *get_isp_i2c_client(void)
{
	return bt404_i2c_client;
}

static void bt532_ts_int_set_pull(bool to_up)
{
	int ret;
#if 0
	int pull = (to_up) ? NMK_GPIO_PULL_UP : NMK_GPIO_PULL_DOWN;

	ret = nmk_gpio_set_pull(TSP_INT_CODINA_R0_0, pull);
	if (ret < 0)
		printk(KERN_ERR "%s: fail to set pull xx on interrupt pin\n",
		__func__);
#endif
}

static int bt532_ts_pin_configure(bool to_gpios)
{
#if 0
if (to_gpios) {
		nmk_gpio_set_mode(TSP_SCL_CODINA_R0_0, NMK_GPIO_ALT_GPIO);
		gpio_direction_output(TSP_SCL_CODINA_R0_0, 0);

		nmk_gpio_set_mode(TSP_SDA_CODINA_R0_0, NMK_GPIO_ALT_GPIO);
		gpio_direction_output(TSP_SDA_CODINA_R0_0, 0);

	} else {
		gpio_direction_output(TSP_SCL_CODINA_R0_0, 1);
		nmk_gpio_set_mode(TSP_SCL_CODINA_R0_0, NMK_GPIO_ALT_C);

		gpio_direction_output(TSP_SDA_CODINA_R0_0, 1);
		nmk_gpio_set_mode(TSP_SDA_CODINA_R0_0, NMK_GPIO_ALT_C);
	}
#endif
	return 0;
}

static struct bt532_ts_platform_data zxt_ts_pdata = {
	.gpio_int = GPIO_TSP_INT,
	.gpio_scl = GPIO_TSP_SCL_18V,
	.gpio_sda = GPIO_TSP_SDA_18V,
	.x_resolution = 800,
	.y_resolution = 1280,
	.orientation = 0,
	.power = zinitix_power,
	.led_power = zinitix_led_power,
	/*.mux_fw_flash = zinitix_mux_fw_flash,
	.is_vdd_on = is_zinitix_vdd_on,*/
	/*	.set_touch_i2c		= zinitix_set_touch_i2c, */
	/*	.set_touch_i2c_to_gpio	= zinitix_set_touch_i2c_to_gpio, */
	/*	.lcd_type = zinitix_get_lcdtype,*/
	.register_cb = zinitix_register_charger_callback,
#ifdef CONFIG_LCD_FREQ_SWITCH
	.register_lcd_cb = zinitix_register_lcd_callback,
#endif
};

static struct i2c_board_info i2c_devs3[] = {
	{
		I2C_BOARD_INFO(BT532_TS_NAME, 0x20),
		.platform_data = &zxt_ts_pdata
	},
};

void __init midas_tsp_set_platdata(struct bt532_ts_platform_data *pdata)
{
	if (!pdata)
		pdata = &zxt_ts_pdata;

	i2c_devs3[0].platform_data = pdata;
}

void __init tab3_tsp_init(u32 system_rev)
{
	int gpio;
	int ret;

	printk(KERN_ERR "[TSP] midas_tsp_init() is called\n");

	gpio_request(GPIO_TSP_VENDOR1, "GPIO_TSP_VENDOR1");
	gpio_request(GPIO_TSP_VENDOR2, "GPIO_TSP_VENDOR2");
	gpio_request(GPIO_TSP_VENDOR3, "GPIO_TSP_VENDOR3");

	gpio_direction_input(GPIO_TSP_VENDOR1);
	gpio_direction_input(GPIO_TSP_VENDOR2);
	gpio_direction_input(GPIO_TSP_VENDOR3);

	s3c_gpio_setpull(GPIO_TSP_VENDOR1, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_VENDOR2, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_VENDOR3, S3C_GPIO_PULL_NONE);

	/* touchkey led */
#if 0
	gpio_request(GPIO_TSK_EN, "GPIO_TSK_EN");
	gpio_direction_output(GPIO_TSK_EN, 0);
#endif
	/* TSP_INT: XEINT_4 */
	gpio = GPIO_TSP_INT;
	ret = gpio_request(gpio, "TSP_INT");
	if (ret)
		pr_err("failed to request gpio(TSP_INT)\n");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_SDA_18V, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_TSP_SCL_18V, S3C_GPIO_PULL_NONE);

	s5p_register_gpio_interrupt(gpio);
	i2c_devs3[0].irq = gpio_to_irq(gpio);

	printk(KERN_ERR"%s touch : %d\n", __func__, i2c_devs3[0].irq);

	zinitix_power(0);

	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
}

#endif

#if defined(CONFIG_EPEN_WACOM_G5SP)
#include <linux/wacom_i2c.h>
static struct wacom_g5_callbacks *wacom_callbacks;
static int wacom_init_hw(void);
static int wacom_suspend_hw(void);
static int wacom_resume_hw(void);
static int wacom_early_suspend_hw(void);
static int wacom_late_resume_hw(void);
static int wacom_reset_hw(void);
static void wacom_compulsory_flash_mode(bool en);
static void wacom_register_callbacks(struct wacom_g5_callbacks *cb);

static struct wacom_g5_platform_data wacom_platform_data = {
	.x_invert = 0,
	.y_invert = 0,
	.xy_switch = 0,
	.gpio_pendct = GPIO_PEN_PDCT_18V,
#ifdef WACOM_PEN_DETECT
	.gpio_pen_insert = GPIO_S_PEN_IRQ,
#endif
#ifdef WACOM_HAVE_FWE_PIN
	.compulsory_flash_mode = wacom_compulsory_flash_mode,
#endif
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
	gpio_direction_output(GPIO_PEN_LDO_EN, 0);

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

#ifdef WACOM_PEN_DETECT
	s3c_gpio_cfgpin(GPIO_S_PEN_IRQ, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_S_PEN_IRQ, S3C_GPIO_PULL_UP);
#endif

#ifdef WACOM_HAVE_FWE_PIN
	ret = gpio_request(GPIO_PEN_FWE0, "GPIO_PEN_FWE0");
	if (ret) {
		printk(KERN_ERR "[E-PEN] faile to request gpio(GPIO_PEN_FWE0)\n");
		return ret;
	}
	s3c_gpio_cfgpin(GPIO_PEN_FWE0, S3C_GPIO_SFN(0x1));
	s3c_gpio_setpull(GPIO_PEN_FWE0, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_PEN_FWE0, 0);
#endif

	return 0;
}

#ifdef WACOM_HAVE_FWE_PIN
static void wacom_compulsory_flash_mode(bool en)
{
	gpio_set_value(GPIO_PEN_FWE0, en);
}

#endif

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

struct gpio_keys_button tab3_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_HOMEPAGE, GPIO_OK_KEY,
		  1, 1, sec_debug_check_crash_key),
   	{
		.code = SW_FLIP,
		.gpio = GPIO_HALL_INT_N,
		.active_low = 0,
		.type = EV_SW,
		.wakeup = 1,
		.debounce_interval = 10,
		.value = 1,
		.isr_hook = sec_debug_check_crash_key,
	},
};

struct gpio_keys_platform_data tab3_gpiokeys_platform_data = {
	tab3_buttons,
	ARRAY_SIZE(tab3_buttons),
};

static struct platform_device tab3_keypad = {
	.name	= "gpio-keys",
	.dev	= {
		.platform_data = &tab3_gpiokeys_platform_data,
	},
};
#endif
void __init tab3_key_init(void)
{
#if defined(CONFIG_KEYBOARD_GPIO)
	platform_device_register(&tab3_keypad);
#endif
}

