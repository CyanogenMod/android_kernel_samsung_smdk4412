/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/earlysuspend.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

#include "issp_extern.h"
#include "cypress-touchkey.h"

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT540E
#include <linux/i2c/mxt540e.h>
#else
#include <linux/i2c/mxt224_u1.h>
#endif
#include <linux/i2c/touchkey_i2c.h>

/* M0 Touchkey temporary setting */

#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3)
#define CONFIG_MACH_Q1_BD
#elif defined(CONFIG_MACH_C1) && !defined(CONFIG_TARGET_LOCALE_KOR)
#define CONFIG_MACH_Q1_BD
#elif defined(CONFIG_MACH_C1) && defined(CONFIG_TARGET_LOCALE_KOR)
/* C1 KOR doesn't use Q1_BD */
#endif

#if defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
/* Temp Fix NAGSM_SEL_ANDROID_MOHAMMAD_ANSARI_20111224*/
#define CONFIG_TARGET_LOCALE_NAATT
#endif

static int touchkey_keycode[] = { 0,
#if defined(TK_USE_4KEY_TYPE_ATT)
	KEY_MENU, KEY_ENTER, KEY_BACK, KEY_END,

#elif defined(TK_USE_4KEY_TYPE_NA)
	KEY_SEARCH, KEY_BACK, KEY_HOME, KEY_MENU,

#elif defined(TK_USE_2KEY_TYPE_M0)
	KEY_BACK, KEY_MENU,

#else
	KEY_MENU, KEY_BACK,

#endif
};
static const int touchkey_count = sizeof(touchkey_keycode) / sizeof(int);

struct touchkey_i2c *tkey_i2c_local;
struct timer_list touch_led_timer;
int touch_led_timeout = 3; // timeout for the touchkey backlight in secs
int touch_led_disabled = 0; // 1= force disable the touchkey backlight

#if defined(TK_HAS_AUTOCAL)
static u16 raw_data0;
static u16 raw_data1;
static u16 raw_data2;
static u16 raw_data3;
static u8 idac0;
static u8 idac1;
static u8 idac2;
static u8 idac3;
static u8 touchkey_threshold;

static int touchkey_autocalibration(struct touchkey_i2c *tkey_i2c);
#endif

#if defined(CONFIG_TARGET_LOCALE_KOR)
#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#if defined(SEC_TKEY_EVENT_DEBUG)
static bool g_debug_tkey = TRUE;
#else
static bool g_debug_tkey = FALSE;
#endif
#endif

static int touchkey_i2c_check(struct touchkey_i2c *tkey_i2c);

static u8 menu_sensitivity;
static u8 back_sensitivity;
#if defined(TK_USE_4KEY)
static u8 home_sensitivity;
static u8 search_sensitivity;
#endif

static int touchkey_enable;
static bool touchkey_probe = true;

static const struct i2c_device_id sec_touchkey_id[] = {
	{"sec_touchkey", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sec_touchkey_id);

extern int get_touchkey_firmware(char *version);
static int touchkey_led_status;
static int touchled_cmd_reversed;

static int touchkey_debug_count;
static char touchkey_debug[104];

#ifdef LED_LDO_WITH_REGULATOR
static void change_touch_key_led_voltage(int vol_mv)
{
	struct regulator *tled_regulator;

	tled_regulator = regulator_get(NULL, "touch_led");
	if (IS_ERR(tled_regulator)) {
		pr_err("%s: failed to get resource %s\n", __func__,
		       "touch_led");
		return;
	}
	regulator_set_voltage(tled_regulator, vol_mv * 1000, vol_mv * 1000);
	regulator_put(tled_regulator);
}

static ssize_t brightness_control(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	int data;

	if (sscanf(buf, "%d\n", &data) == 1) {
		pr_err("[TouchKey] touch_led_brightness: %d\n", data);
		change_touch_key_led_voltage(data);
	} else {
		pr_err("[TouchKey] touch_led_brightness Error\n");
	}

	return size;
}
#endif

static void set_touchkey_debug(char value)
{
	if (touchkey_debug_count == 100)
		touchkey_debug_count = 0;

	touchkey_debug[touchkey_debug_count] = value;
	touchkey_debug_count++;
}

static int i2c_touchkey_read(struct i2c_client *client,
		u8 reg, u8 *val, unsigned int len)
{
	int err = 0;
	int retry = 3;
#if !defined(TK_USE_GENERAL_SMBUS)
	struct i2c_msg msg[1];
#endif

	if ((client == NULL) || !(touchkey_enable == 1)
	    || !touchkey_probe) {
		pr_err("[TouchKey] touchkey is not enabled. %d\n",
		       __LINE__);
		return -ENODEV;
	}

	while (retry--) {
#if defined(TK_USE_GENERAL_SMBUS)
		err = i2c_smbus_read_i2c_block_data(client,
				KEYCODE_REG, len, val);
#else
		msg->addr = client->addr;
		msg->flags = I2C_M_RD;
		msg->len = len;
		msg->buf = val;
		err = i2c_transfer(client->adapter, msg, 1);
#endif

		if (err >= 0)
			return 0;
		pr_err("[TouchKey] %s %d i2c transfer error\n",
		       __func__, __LINE__);
		mdelay(10);
	}
	return err;

}

static int i2c_touchkey_write(struct i2c_client *client,
		u8 *val, unsigned int len)
{
	int err = 0;
	int retry = 3;
#if !defined(TK_USE_GENERAL_SMBUS)
	struct i2c_msg msg[1];
#endif

	if ((client == NULL) || !(touchkey_enable == 1)
	    || !touchkey_probe) {
		pr_err("[TouchKey] touchkey is not enabled. %d\n",
		       __LINE__);
		return -ENODEV;
	}

	while (retry--) {
#if defined(TK_USE_GENERAL_SMBUS)
		err = i2c_smbus_write_i2c_block_data(client,
				KEYCODE_REG, len, val);
#else
		msg->addr = client->addr;
		msg->flags = I2C_M_WR;
		msg->len = len;
		msg->buf = val;
		err = i2c_transfer(client->adapter, msg, 1);
#endif

		if (err >= 0)
			return 0;

		pr_debug("[TouchKey] %s %d i2c transfer error\n",
		       __func__, __LINE__);
		mdelay(10);
	}
	return err;
}

#if defined(TK_HAS_AUTOCAL)
static int touchkey_autocalibration(struct touchkey_i2c *tkey_i2c)
{
	u8 data[6] = { 0, };
	int count = 0;
	int ret = 0;
	unsigned short retry = 0;

#if defined(CONFIG_TARGET_LOCALE_NA)
	if (tkey_i2c->module_ver < 8)
		return -1;
#endif

	while (retry < 3) {
		ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 4);
		if (ret < 0) {
			pr_err("[TouchKey]i2c read fail.\n");
			return ret;
		}
		pr_debug("[TouchKey] data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
				data[0], data[1], data[2], data[3]);

		/* Send autocal Command */
		data[0] = 0x50;
		data[3] = 0x01;

		count = i2c_touchkey_write(tkey_i2c->client, data, 4);

		msleep(100);

		/* Check autocal status */
		ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 6);

		if ((data[5] & TK_BIT_AUTOCAL)) {
			pr_debug("[Touchkey] autocal Enabled\n");
			break;
		} else
			pr_debug("[Touchkey] autocal disabled, retry %d\n",
			       retry);

		retry = retry + 1;
	}

	if (retry == 3)
		pr_debug("[Touchkey] autocal failed\n");

	return count;
}
#endif

#if 0 /* CONFIG_TARGET_LOCALE_NAATT */
static ssize_t set_touchkey_autocal_testmode(struct device *dev,
					     struct device_attribute *attr,
					     const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int count = 0;
	u8 set_data;
	int on_off;

	if (sscanf(buf, "%d\n", &on_off) == 1) {
		pr_err("[TouchKey] Test Mode : %d\n", on_off);

		if (on_off == 1) {
			set_data = 0x40;
			count = i2c_touchkey_write(tkey_i2c->client,
					&set_data, 1);
		} else {
			tkey_i2c->pdata->power_on(0);
			msleep(50);
			tkey_i2c->pdata->power_on(1);
			msleep(50);
#if defined(TK_HAS_AUTOCAL)
			touchkey_autocalibration();
#endif
		}
	} else {
		pr_err("[TouchKey] touch_led_brightness Error\n");
	}

	return count;
}
#endif

#if defined(TK_HAS_AUTOCAL)
static ssize_t touchkey_raw_data0_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[26] = { 0, };
	int ret;

	pr_debug("called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 26);
#if defined(CONFIG_TARGET_LOCALE_NA)
	pr_debug("called %s data[18] =%d,data[19] = %d\n", __func__,
	       data[18], data[19]);
	raw_data0 = ((0x00FF & data[18]) << 8) | data[19];
#elif defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_C1)\
|| defined(CONFIG_MACH_M3)\
	 || defined(CONFIG_MACH_T0)
	pr_debug("called %s data[16] =%d,data[17] = %d\n", __func__,
	       data[16], data[17]);
	raw_data0 = ((0x00FF & data[16]) << 8) | data[17]; /* menu*/
#elif defined(CONFIG_MACH_Q1_BD)
	pr_debug("called %s data[16] =%d,data[17] = %d\n", __func__,
	       data[16], data[17]);
	raw_data0 = ((0x00FF & data[14]) << 8) | data[15];
#else
	pr_debug("called %s data[18] =%d,data[19] = %d\n", __func__,
	       data[10], data[11]);
	raw_data0 = ((0x00FF & data[10]) << 8) | data[11];
#endif
	return sprintf(buf, "%d\n", raw_data0);
}

static ssize_t touchkey_raw_data1_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[26] = { 0, };
	int ret;

	pr_debug("called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 26);
#if defined(CONFIG_TARGET_LOCALE_NA)
	pr_debug("called %s data[20] =%d,data[21] = %d\n", __func__,
	       data[20], data[21]);
	raw_data1 = ((0x00FF & data[20]) << 8) | data[21];
#elif defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_C1)\
|| defined(CONFIG_MACH_M3)\
	 || defined(CONFIG_MACH_T0)
	pr_debug("called %s data[14] =%d,data[15] = %d\n", __func__,
	       data[14], data[15]);
	raw_data1 = ((0x00FF & data[14]) << 8) | data[15]; /*back*/
#elif defined(CONFIG_MACH_Q1_BD)
	pr_debug("called %s data[14] =%d,data[15] = %d\n", __func__,
			   data[14], data[15]);
	raw_data1 = ((0x00FF & data[16]) << 8) | data[17];
#else
	pr_debug("called %s data[20] =%d,data[21] = %d\n", __func__,
	       data[12], data[13]);
	raw_data1 = ((0x00FF & data[12]) << 8) | data[13];
#endif				/* CONFIG_TARGET_LOCALE_NA */
	return sprintf(buf, "%d\n", raw_data1);
}

static ssize_t touchkey_raw_data2_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[26] = { 0, };
	int ret;

	pr_debug("called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 26);
#if defined(CONFIG_TARGET_LOCALE_NA)
	pr_debug("called %s data[22] =%d,data[23] = %d\n", __func__,
	       data[22], data[23]);
	raw_data2 = ((0x00FF & data[22]) << 8) | data[23];
#else
	pr_debug("called %s data[22] =%d,data[23] = %d\n", __func__,
	       data[14], data[15]);
	raw_data2 = ((0x00FF & data[14]) << 8) | data[15];
#endif				/* CONFIG_TARGET_LOCALE_NA */
	return sprintf(buf, "%d\n", raw_data2);
}

static ssize_t touchkey_raw_data3_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[26] = { 0, };
	int ret;

	pr_debug("called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 26);
#if defined(CONFIG_TARGET_LOCALE_NA)
	pr_debug("called %s data[24] =%d,data[25] = %d\n", __func__,
	       data[24], data[25]);
	raw_data3 = ((0x00FF & data[24]) << 8) | data[25];
#else
	pr_debug("called %s data[24] =%d,data[25] = %d\n", __func__,
	       data[16], data[17]);
	raw_data3 = ((0x00FF & data[16]) << 8) | data[17];
#endif				/* CONFIG_TARGET_LOCALE_NA */
	return sprintf(buf, "%d\n", raw_data3);
}

static ssize_t touchkey_idac0_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[10];
	int ret;
#ifdef CONFIG_TARGET_LOCALE_NA
	if (tkey_i2c->module_ver < 8)
		return 0;
#endif

	pr_debug("called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 10);
	pr_debug("called %s data[6] =%d\n", __func__, data[6]);
	idac0 = data[6];
	return sprintf(buf, "%d\n", idac0);
}

static ssize_t touchkey_idac1_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[10];
	int ret;
#ifdef CONFIG_TARGET_LOCALE_NA
	if (tkey_i2c->module_ver < 8)
		return 0;
#endif

	pr_debug("called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 10);
	pr_debug("called %s data[7] = %d\n", __func__, data[7]);
	idac1 = data[7];
	return sprintf(buf, "%d\n", idac1);
}

static ssize_t touchkey_idac2_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[10];
	int ret;
#ifdef CONFIG_TARGET_LOCALE_NA
	if (tkey_i2c->module_ver < 8)
		return 0;
#endif

	pr_debug("called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 10);
	pr_debug("called %s data[8] =%d\n", __func__, data[8]);
	idac2 = data[8];
	return sprintf(buf, "%d\n", idac2);
}

static ssize_t touchkey_idac3_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[10];
	int ret;
#ifdef CONFIG_TARGET_LOCALE_NA
	if (tkey_i2c->module_ver < 8)
		return 0;
#endif

	pr_debug("called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 10);
	pr_debug("called %s data[9] = %d\n", __func__, data[9]);
	idac3 = data[9];
	return sprintf(buf, "%d\n", idac3);
}

static ssize_t touchkey_threshold_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[10];
	int ret;

	pr_debug("called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 10);
	pr_debug("called %s data[4] = %d\n", __func__, data[4]);
	touchkey_threshold = data[4];
	return sprintf(buf, "%d\n", touchkey_threshold);
}
#endif

#if defined(TK_HAS_FIRMWARE_UPDATE)
static int touchkey_firmware_update(struct touchkey_i2c *tkey_i2c)
{
	int retry = 3;
	int ret = 0;
	char data[3];

	disable_irq(tkey_i2c->irq);


	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 3);
	if (ret < 0) {
		pr_debug("[TouchKey] i2c read fail. do not excute firm update.\n");
		data[1] = 0;
		data[2] = 0;
	}

	pr_err("%s F/W version: 0x%x, Module version:0x%x\n", __func__,
	data[1], data[2]);

	tkey_i2c->firmware_ver = data[1];
	tkey_i2c->module_ver = data[2];

#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_C1) \
|| defined(CONFIG_MACH_M3) || defined(CONFIG_MACH_T0)
	if ((tkey_i2c->firmware_ver < TK_FIRMWARE_VER) &&
	    (tkey_i2c->module_ver <= TK_MODULE_VER)) {
#else
	if ((tkey_i2c->firmware_ver < TK_FIRMWARE_VER) &&
		(tkey_i2c->module_ver == TK_MODULE_VER)) {
#endif
		pr_debug("[TouchKey] firmware auto update excute\n");

		tkey_i2c->update_status = TK_UPDATE_DOWN;

		while (retry--) {
			if (ISSP_main(tkey_i2c) == 0) {
				pr_debug("[TouchKey]firmware update succeeded\n");
				tkey_i2c->update_status = TK_UPDATE_PASS;
				msleep(50);
				break;
			}
			msleep(50);
			pr_debug("[TouchKey] firmware update failed. retry\n");
		}
		if (retry <= 0) {
			tkey_i2c->pdata->power_on(0);
			tkey_i2c->update_status = TK_UPDATE_FAIL;
			pr_debug("[TouchKey] firmware update failed.\n");
		}
		ret = touchkey_i2c_check(tkey_i2c);
		if (ret < 0) {
			pr_debug("[TouchKey] i2c read fail.\n");
			return TK_UPDATE_FAIL;
		}
#if defined(CONFIG_TARGET_LOCALE_KOR)
		ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 3);
		if (ret < 0) {
			pr_debug("[TouchKey] i2c read fail. do not excute firm update.\n");
		}
		tkey_i2c->firmware_ver = data[1];
		tkey_i2c->module_ver = data[2];
#endif
		printk(KERN_DEBUG "[TouchKey] firm ver = %d, module ver = %d\n",
			tkey_i2c->firmware_ver, tkey_i2c->module_ver);
	} else {
		pr_debug("[TouchKey] firmware auto update do not excute\n");
		pr_debug("[TouchKey] firmware_ver(banary=%d, current=%d)\n",
		       TK_FIRMWARE_VER, tkey_i2c->firmware_ver);
		pr_debug("[TouchKey] module_ver(banary=%d, current=%d)\n",
		       TK_MODULE_VER, tkey_i2c->module_ver);
	}
	enable_irq(tkey_i2c->irq);
	return TK_UPDATE_PASS;
}
#else
static int touchkey_firmware_update(struct touchkey_i2c *tkey_i2c)
{
	char data[3];
	int retry;
	int ret = 0;

	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 3);
	if (ret < 0) {
		pr_debug("[TouchKey] i2c read fail. do not excute firm update.\n");
		return ret;
	}

	pr_err("%s F/W version: 0x%x, Module version:0x%x\n", __func__,
	       data[1], data[2]);
	retry = 3;

	tkey_i2c->firmware_ver = data[1];
	tkey_i2c->module_ver = data[2];

	if (tkey_i2c->firmware_ver < 0x0A) {
		tkey_i2c->update_status = TK_UPDATE_DOWN;
		while (retry--) {
			if (ISSP_main(tkey_i2c) == 0) {
				pr_err("[TOUCHKEY]Touchkey_update succeeded\n");
				tkey_i2c->update_status = TK_UPDATE_PASS;
				break;
			}
			pr_err("touchkey_update failed...retry...\n");
		}
		if (retry <= 0) {
			tkey_i2c->pdata->power_on(0);
			tkey_i2c->update_status = TK_UPDATE_FAIL;
			ret = TK_UPDATE_FAIL;
		}
	} else {
		if (tkey_i2c->firmware_ver >= 0x0A) {
			pr_err("[TouchKey] Not F/W update. Cypess touch-key F/W version is latest\n");
		} else {
			pr_err("[TouchKey] Not F/W update. Cypess touch-key version(module or F/W) is not valid\n");
		}
	}
	return ret;
}
#endif

#ifndef TEST_JIG_MODE
static irqreturn_t touchkey_interrupt(int irq, void *dev_id)
{
	struct touchkey_i2c *tkey_i2c = dev_id;
    static const int ledCmd[] = {TK_CMD_LED_ON, TK_CMD_LED_OFF};
	u8 data[3];
	int ret;
	int retry = 10;
	int keycode_type = 0;
	int pressed;

	set_touchkey_debug('a');

	retry = 3;
	while (retry--) {
		ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 3);
		if (!ret)
			break;
		else {
			pr_debug("[TouchKey] i2c read failed, ret:%d, retry: %d\n",
			       ret, retry);
			continue;
		}
	}
	if (ret < 0)
		return IRQ_HANDLED;

	set_touchkey_debug(data[0]);

	keycode_type = (data[0] & TK_BIT_KEYCODE);
	pressed = !(data[0] & TK_BIT_PRESS_EV);

	if (keycode_type <= 0 || keycode_type >= touchkey_count) {
		pr_debug("[Touchkey] keycode_type err\n");
		return IRQ_HANDLED;
	}

	if (pressed) {
		set_touchkey_debug('P');

        // enable lights on keydown
        if (touch_led_disabled == 0) {
            if (touchkey_led_status == TK_CMD_LED_OFF) {
                pr_debug("[Touchkey] %s: keydown - LED ON\n", __func__);
                i2c_touchkey_write(tkey_i2c->client, (u8 *) &ledCmd[0], 1);
                touchkey_led_status = TK_CMD_LED_ON;
            }
            if (timer_pending(&touch_led_timer) == 1) {
                mod_timer(&touch_led_timer, jiffies + (HZ * touch_led_timeout));
            }
        }
        
    } else {
        // touch led timeout on keyup
        if (touch_led_disabled == 0) {
            if (timer_pending(&touch_led_timer) == 0) {
                pr_debug("[Touchkey] %s: keyup - add_timer\n", __func__);
                touch_led_timer.expires = jiffies + (HZ * touch_led_timeout);
                add_timer(&touch_led_timer);
            } else {
                mod_timer(&touch_led_timer, jiffies + (HZ * touch_led_timeout));
            }
        }
    }

	if (get_tsp_status() && pressed)
		pr_debug("[TouchKey] touchkey pressed but don't send event because touch is pressed.\n");
	else {
		input_report_key(tkey_i2c->input_dev,
				 touchkey_keycode[keycode_type], pressed);
		input_sync(tkey_i2c->input_dev);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		pr_debug("[TouchKey] keycode:%d pressed:%d\n",
		   touchkey_keycode[keycode_type], pressed);
#else
		pr_debug("[TouchKey] pressed:%d\n",
			pressed);
#endif

		#if defined(CONFIG_TARGET_LOCALE_KOR)
		if (g_debug_tkey == true) {
			pr_debug("[TouchKey] keycode[%d]=%d pressed:%d\n",
			keycode_type, touchkey_keycode[keycode_type], pressed);
		} else {
			pr_debug("[TouchKey] pressed:%d\n", pressed);
		}
		#endif
	}
	set_touchkey_debug('A');
	return IRQ_HANDLED;
}
#else
static irqreturn_t touchkey_interrupt(int irq, void *dev_id)
{
	struct touchkey_i2c *tkey_i2c = dev_id;
	u8 data[18];
	int ret;
	int retry = 10;
	int keycode_type = 0;
	int pressed;

#if 0
	if (gpio_get_value(_3_GPIO_TOUCH_INT)) {
		pr_debug("[TouchKey] Unknown state.\n", __func__);
		return IRQ_HANDLED;
	}
#endif

	set_touchkey_debug('a');

#ifdef CONFIG_CPU_FREQ
	/* set_dvfs_target_level(LEV_800MHZ); */
#endif

	retry = 3;
	while (retry--) {
#if defined(CONFIG_TARGET_LOCALE_NA) || defined(CONFIG_MACH_Q1_BD)\
	 || defined(CONFIG_MACH_C1)
		ret = i2c_touchkey_read(tkey_i2c->client,
				KEYCODE_REG, data, 18);
#else
		ret = i2c_touchkey_read(tkey_i2c->client,
				KEYCODE_REG, data, 10);
#endif
		if (!ret)
			break;
		else {
			pr_debug("[TouchKey] i2c read failed, ret:%d, retry: %d\n",
			       ret, retry);
			continue;
		}
	}
	if (ret < 0)
		return IRQ_HANDLED;

#if defined(CONFIG_TARGET_LOCALE_NA)
#if defined(CONFIG_MACH_C1_NA_SPR_EPIC2_REV00)
	menu_sensitivity = data[11];
	home_sensitivity = data[13];
	search_sensitivity = data[15];
	back_sensitivity = data[17];
#else
	if (tkey_i2c->module_ver >= 8) {
		menu_sensitivity = data[17];
		home_sensitivity = data[15];
		search_sensitivity = data[11];
		back_sensitivity = data[13];
	} else {
		menu_sensitivity = data[6];
		home_sensitivity = data[7];
		search_sensitivity = data[8];
		back_sensitivity = data[9];
	}
#endif
#elif defined(CONFIG_MACH_Q1_BD) || defined(CONFIG_MACH_C1)
	menu_sensitivity = data[13];
	back_sensitivity = data[11];
#else
	menu_sensitivity = data[7];
	back_sensitivity = data[9];
#endif				/* CONFIG_TARGET_LOCALE_NA  */

	set_touchkey_debug(data[0]);

	keycode_type = (data[0] & TK_BIT_KEYCODE);
	pressed = !(data[0] & TK_BIT_PRESS_EV);

	if (keycode_type <= 0 || keycode_type >= touchkey_count) {
		pr_debug("[Touchkey] keycode_type err\n");
		return IRQ_HANDLED;
	}

	if (pressed) {
		set_touchkey_debug('P');
    }

	if (get_tsp_status() && pressed)
		pr_debug("[TouchKey] touchkey pressed"
		       " but don't send event because touch is pressed.\n");
	else {
		input_report_key(touchkey_driver->input_dev,
				 touchkey_keycode[keycode_type], pressed);
		input_sync(touchkey_driver->input_dev);
		/* printk(KERN_DEBUG "[TouchKey] keycode:%d pressed:%d\n",
		   touchkey_keycode[keycode_index], pressed); */
	}

	if (keycode_type == 1)
		printk(KERN_DEBUG "search key sensitivity = %d\n",
		       search_sensitivity);
	if (keycode_type == 2)
		pr_debug("back key sensitivity = %d\n",
		       back_sensitivity);
#ifdef CONFIG_TARGET_LOCALE_NA
	if (keycode_type == 3)
		pr_debug("home key sensitivity = %d\n",
		       home_sensitivity);
	if (keycode_type == 4)
		pr_debug("menu key sensitivity = %d\n",
		       menu_sensitivity);
#endif

	set_touchkey_debug('A');
	return IRQ_HANDLED;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static int sec_touchkey_early_suspend(struct early_suspend *h)
{
	struct touchkey_i2c *tkey_i2c =
		container_of(h, struct touchkey_i2c, early_suspend);
	int ret;
	int i;

	disable_irq(tkey_i2c->irq);
	ret = cancel_work_sync(&tkey_i2c->update_work);
	if (ret) {
		pr_debug("[Touchkey] enable_irq ret=%d\n", ret);
		enable_irq(tkey_i2c->irq);
	}

	/* release keys */
	for (i = 1; i < touchkey_count; ++i) {
		input_report_key(tkey_i2c->input_dev,
				 touchkey_keycode[i], 0);
	}
	input_sync(tkey_i2c->input_dev);

	touchkey_enable = 0;
	set_touchkey_debug('S');
	pr_debug("[TouchKey] sec_touchkey_early_suspend\n");
	if (touchkey_enable < 0) {
		pr_debug("[TouchKey] ---%s---touchkey_enable: %d\n",
		       __func__, touchkey_enable);
		return 0;
	}

	/* disable ldo18 */
	tkey_i2c->pdata->led_power_on(0);

	/* disable ldo11 */
	tkey_i2c->pdata->power_on(0);

	return 0;
}

static int sec_touchkey_late_resume(struct early_suspend *h)
{
	struct touchkey_i2c *tkey_i2c =
		container_of(h, struct touchkey_i2c, early_suspend);
#ifdef TEST_JIG_MODE
	unsigned char get_touch = 0x40;
#endif

	set_touchkey_debug('R');
	pr_debug("[TouchKey] sec_touchkey_late_resume\n");

	/* enable ldo11 */
	tkey_i2c->pdata->power_on(1);

	if (touchkey_enable < 0) {
		pr_debug("[TouchKey] ---%s---touchkey_enable: %d\n",
		       __func__, touchkey_enable);
		return 0;
	}
	msleep(50);
	tkey_i2c->pdata->led_power_on(1);

	touchkey_enable = 1;

#if defined(TK_HAS_AUTOCAL)
	touchkey_autocalibration(tkey_i2c);
#endif

	if (touchled_cmd_reversed) {
		touchled_cmd_reversed = 0;
		i2c_touchkey_write(tkey_i2c->client,
			(u8 *) &touchkey_led_status, 1);
		pr_debug("[Touchkey] LED returned on\n");
	}
#ifdef TEST_JIG_MODE
	i2c_touchkey_write(tkey_i2c->client, &get_touch, 1);
#endif

	enable_irq(tkey_i2c->irq);

	return 0;
}
#endif

static int touchkey_i2c_check(struct touchkey_i2c *tkey_i2c)
{
	char data[3] = { 0, };
	int ret = 0;

	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 3);
	if (ret < 0) {
		pr_err("[TouchKey] module version read fail\n");
		return ret;
	}

	tkey_i2c->firmware_ver = data[1];
	tkey_i2c->module_ver = data[2];

	return ret;
}

ssize_t touchkey_update_read(struct file *filp, char *buf, size_t count,
			     loff_t *f_pos)
{
	char data[3] = { 0, };

	get_touchkey_firmware(data);
	put_user(data[1], buf);

	return 1;
}

static ssize_t touch_version_read(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	char data[3] = { 0, };
	int count;

	i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 3);

	count = sprintf(buf, "0x%x\n", data[1]);

	pr_debug("[TouchKey] touch_version_read 0x%x\n", data[1]);
	pr_debug("[TouchKey] module_version_read 0x%x\n", data[2]);

	return count;
}

static ssize_t touch_version_write(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	printk(KERN_DEBUG "[TouchKey] input data --> %s\n", buf);

	return size;
}

void touchkey_update_func(struct work_struct *work)
{
	struct touchkey_i2c *tkey_i2c =
		container_of(work, struct touchkey_i2c, update_work);
	int retry = 3;
#if defined(CONFIG_TARGET_LOCALE_NAATT)
	char data[3];
	i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 3);
	pr_debug("[Touchkey] %s: F/W version: 0x%x, Module version:0x%x\n",
	       __func__, data[1], data[2]);
#endif
	tkey_i2c->update_status = TK_UPDATE_DOWN;
	pr_debug("[Touchkey] %s: start\n", __func__);
	touchkey_enable = 0;
	while (retry--) {
		if (ISSP_main(tkey_i2c) == 0) {
			pr_debug("[TouchKey] touchkey_update succeeded\n");
			msleep(50);
			touchkey_enable = 1;
#if defined(TK_HAS_AUTOCAL)
			touchkey_autocalibration(tkey_i2c);
#endif
			tkey_i2c->update_status = TK_UPDATE_PASS;
			enable_irq(tkey_i2c->irq);
			return;
		}
		tkey_i2c->pdata->power_on(0);
	}
	enable_irq(tkey_i2c->irq);
	tkey_i2c->update_status = TK_UPDATE_FAIL;
	printk(KERN_DEBUG "[TouchKey] touchkey_update failed\n");
	return;
}

static ssize_t touch_update_write(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
#ifdef CONFIG_TARGET_LOCALE_NA
	if (tkey_i2c->module_ver < 8) {
		printk(KERN_DEBUG
		       "[TouchKey] Skipping f/w update : module_version =%d\n",
		       tkey_i2c->module_ver);
		tkey_i2c->update_status = TK_UPDATE_PASS;
		return 1;
	} else {
#endif				/* CONFIG_TARGET_LOCALE_NA */
		printk(KERN_DEBUG "[TouchKey] touchkey firmware update\n");

		if (*buf == 'S') {
			disable_irq(tkey_i2c->irq);
			schedule_work(&tkey_i2c->update_work);
		}
		return size;
#ifdef CONFIG_TARGET_LOCALE_NA
	}
#endif				/* CONFIG_TARGET_LOCALE_NA */
}

static ssize_t touch_update_read(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int count = 0;

	printk(KERN_DEBUG
	       "[TouchKey] touch_update_read: update_status %d\n",
	       tkey_i2c->update_status);

	if (tkey_i2c->update_status == TK_UPDATE_PASS)
		count = sprintf(buf, "PASS\n");
	else if (tkey_i2c->update_status == TK_UPDATE_DOWN)
		count = sprintf(buf, "Downloading\n");
	else if (tkey_i2c->update_status == TK_UPDATE_FAIL)
		count = sprintf(buf, "Fail\n");

	return count;
}

static ssize_t touchkey_led_control(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int data;
	int ret;
	static const int ledCmd[] = {TK_CMD_LED_ON, TK_CMD_LED_OFF};

#if defined(CONFIG_TARGET_LOCALE_KOR)
	if (touchkey_probe == false)
		return size;
#endif
	ret = sscanf(buf, "%d", &data);
	if (ret != 1) {
		printk(KERN_DEBUG "[Touchkey] %s: %d err\n",
			__func__, __LINE__);
		return size;
	}

	if (data != 1 && data != 2) {
		printk(KERN_DEBUG "[Touchkey] %s: wrong cmd %x\n",
			__func__, data);
		return size;
	}

#if defined(CONFIG_TARGET_LOCALE_NA)
	if (tkey_i2c->module_ver >= 8)
		data = ledCmd[data-1];
#else
	data = ledCmd[data-1];
#endif

    if (touch_led_disabled == 0) {
        ret = i2c_touchkey_write(tkey_i2c->client, (u8 *) &data, 1);
    }

    if(data == ledCmd[0]) {
        if (touch_led_disabled == 0) {
            if (timer_pending(&touch_led_timer) == 0) {
                pr_debug("[Touchkey] %s: add_timer\n", __func__);
                touch_led_timer.expires = jiffies + (HZ * touch_led_timeout);
                add_timer(&touch_led_timer);
            } else {
                mod_timer(&touch_led_timer, jiffies + (HZ * touch_led_timeout));
            }
        }
    } else {
        if (timer_pending(&touch_led_timer) == 1) {
            pr_debug("[Touchkey] %s: del_timer\n", __func__);
            del_timer(&touch_led_timer);
        }
    }

	if (ret == -ENODEV) {
		pr_err("[Touchkey] error to write i2c\n");
		touchled_cmd_reversed = 1;
	}

    pr_debug("[Touchkey] %s: touchkey_led_status=%d\n", __func__, data);
	touchkey_led_status = data;

	return size;
}

static ssize_t touch_led_force_disable_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int ret;

    ret = sprintf(buf, "%d\n", touch_led_disabled);
    pr_info("[Touchkey] %s: touch_led_disabled=%d\n", __func__, touch_led_disabled);

    return ret;
}

static ssize_t touch_led_force_disable_store(struct device *dev,
        struct device_attribute *attr, const char *buf,
        size_t size)
{
    struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	static const int ledCmd[] = {TK_CMD_LED_ON, TK_CMD_LED_OFF};
    int data, ret;

    ret = sscanf(buf, "%d\n", &data);
    if (unlikely(ret != 1)) {
        pr_err("[Touchkey] %s: err\n", __func__);
        return -EINVAL;
    }
    pr_info("[Touchkey] %s: value=%d\n", __func__, data);
    
    if (data == 1) {
        i2c_touchkey_write(tkey_i2c->client, (u8 *) &ledCmd[1], 1);
        touchkey_led_status = TK_CMD_LED_OFF;
    }
    touch_led_disabled = data;

    return size;
}
static DEVICE_ATTR(force_disable, S_IRUGO | S_IWUSR | S_IWGRP,
        touch_led_force_disable_show, touch_led_force_disable_store);

static ssize_t touch_led_timeout_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int ret;

    ret = sprintf(buf, "%d\n", touch_led_timeout);
    pr_info("[Touchkey] %s: touch_led_timeout=%d\n", __func__, touch_led_timeout);

    return ret;
}

static ssize_t touch_led_timeout_store(struct device *dev,
        struct device_attribute *attr, const char *buf,
        size_t size)
{
    int data;
    int ret;

    ret = sscanf(buf, "%d\n", &data);
    if (unlikely(ret != 1)) {
        pr_err("[Touchkey] %s: err\n", __func__);
        return -EINVAL;
    }
    pr_info("[Touchkey] %s: new timeout=%d\n", __func__, data);
    touch_led_timeout = data;

    return size;
}
static DEVICE_ATTR(timeout, S_IRUGO | S_IWUSR | S_IWGRP,
        touch_led_timeout_show, touch_led_timeout_store);

void touch_led_timedout(unsigned long ptr)
{
    queue_work(tkey_i2c_local->wq, &tkey_i2c_local->work);
}

void touch_led_timedout_work(struct work_struct *work)
{
    struct touchkey_i2c *tkey_i2c = container_of(work, struct touchkey_i2c, work);
    static const int ledCmd[] = {TK_CMD_LED_ON, TK_CMD_LED_OFF};

    if (touch_led_timeout != 0)
    {
        pr_debug("[Touchkey] %s: disabling touchled\n", __func__);
        i2c_touchkey_write(tkey_i2c->client, (u8 *) &ledCmd[1], 1);
        touchkey_led_status = TK_CMD_LED_OFF;
    }
}

void touchscreen_state_report(int state)
{
    static const int ledCmd[] = {TK_CMD_LED_ON, TK_CMD_LED_OFF};

    if (touch_led_disabled == 0) {
        if (state == 1) {
            if(touchkey_led_status == TK_CMD_LED_OFF) {
                pr_debug("[Touchkey] %s: enable touchleds\n", __func__);
                i2c_touchkey_write(tkey_i2c_local->client, (u8 *) &ledCmd[0], 1);
                touchkey_led_status = TK_CMD_LED_ON;
            } else {
                if (timer_pending(&touch_led_timer) == 1) {
                    pr_debug("[Touchkey] %s: mod_timer\n", __func__);
                    mod_timer(&touch_led_timer, jiffies + (HZ * touch_led_timeout));
                }
            }
        } else if (state == 0) {
            if (timer_pending(&touch_led_timer) == 1) {
                pr_debug("[Touchkey] %s: mod_timer\n", __func__);
                mod_timer(&touch_led_timer, jiffies + (HZ * touch_led_timeout));
            } else if (touchkey_led_status == TK_CMD_LED_ON){
                pr_debug("[Touchkey] %s: add_timer\n", __func__);
                touch_led_timer.expires = jiffies + (HZ * touch_led_timeout);
                add_timer(&touch_led_timer);
            }
        }
    }
}

#if defined(TK_USE_4KEY) || defined(CONFIG_TARGET_LOCALE_NAATT) || defined(CONFIG_TARGET_LOCALE_NA)
static ssize_t touchkey_menu_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[18] = { 0, };
	int ret;

	pr_debug("[Touchkey] %s called\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 18);
#ifdef CONFIG_TARGET_LOCALE_NA
	if (tkey_i2c->module_ver < 8) {
		pr_debug("[Touchkey] %s: data[12] =%d,data[13] = %d\n",
		       __func__, data[12], data[13]);
		menu_sensitivity = ((0x00FF & data[12]) << 8) | data[13];
	} else {
		pr_debug("[Touchkey] %s: data[17] =%d\n", __func__,
		       data[17]);
		menu_sensitivity = data[17];
	}
#else
	pr_debug("[Touchkey] %s: data[10] =%d,data[11] = %d\n", __func__,
	       data[10], data[11]);
	menu_sensitivity = ((0x00FF & data[10]) << 8) | data[11];
#endif				/* CONFIG_TARGET_LOCALE_NA */
	return sprintf(buf, "%d\n", menu_sensitivity);
}

static ssize_t touchkey_home_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[18] = { 0, };
	int ret;

	pr_debug("[TouchKey] %s called\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 18);
#ifdef CONFIG_TARGET_LOCALE_NA
	if (tkey_i2c->module_ver < 8) {
		pr_debug("[Touchkey] %s: data[10] =%d,data[11] = %d\n",
		       __func__, data[10], data[11]);
		home_sensitivity = ((0x00FF & data[10]) << 8) | data[11];
	} else {
		pr_debug("[Touchkey] %s: data[15] =%d\n", __func__,
		       data[15]);
		home_sensitivity = data[15];
	}
#else
	pr_debug("[Touchkey] %s: data[12] =%d,data[13] = %d\n", __func__,
	       data[12], data[13]);
	home_sensitivity = ((0x00FF & data[12]) << 8) | data[13];
#endif				/* CONFIG_TARGET_LOCALE_NA */
	return sprintf(buf, "%d\n", home_sensitivity);
}

static ssize_t touchkey_back_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[18] = { 0, };
	int ret;

	pr_debug("[TouchKey] %s called\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 18);
#ifdef CONFIG_TARGET_LOCALE_NA
	if (tkey_i2c->module_ver < 8) {
		pr_debug("[Touchkey] %s: data[8] =%d,data[9] = %d\n",
		       __func__, data[8], data[9]);
		back_sensitivity = ((0x00FF & data[8]) << 8) | data[9];
	} else {
		pr_debug("[Touchkey] %s: data[13] =%d\n", __func__,
		       data[13]);
		back_sensitivity = data[13];
	}
#else
	pr_debug("[Touchkey] %s: data[14] =%d,data[15] = %d\n", __func__,
	       data[14], data[15]);
	back_sensitivity = ((0x00FF & data[14]) << 8) | data[15];
#endif				/* CONFIG_TARGET_LOCALE_NA */
	return sprintf(buf, "%d\n", back_sensitivity);
}

static ssize_t touchkey_search_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[18] = { 0, };
	int ret;

	pr_debug("[TouchKey] %s called\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 18);
#ifdef CONFIG_TARGET_LOCALE_NA
	if (tkey_i2c->module_ver < 8) {
		pr_debug("[Touchkey] %s: data[6] =%d,data[7] = %d\n",
		       __func__, data[6], data[7]);
		search_sensitivity = ((0x00FF & data[6]) << 8) | data[7];
	} else {
		pr_debug("[Touchkey] %s: data[11] =%d\n", __func__,
		       data[11]);
		search_sensitivity = data[11];
	}
#else
	pr_debug("[Touchkey] %s: data[16] =%d,data[17] = %d\n", __func__,
	       data[16], data[17]);
	search_sensitivity = ((0x00FF & data[16]) << 8) | data[17];
#endif				/* CONFIG_TARGET_LOCALE_NA */
	return sprintf(buf, "%d\n", search_sensitivity);
}
#else
static ssize_t touchkey_menu_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
#if defined(CONFIG_MACH_Q1_BD) \
|| (defined(CONFIG_MACH_C1) && defined(CONFIG_TARGET_LOCALE_KOR))\
	|| defined(CONFIG_MACH_T0)
	u8 data[14] = { 0, };
	int ret;

	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 14);

	pr_debug("[Touchkey] %s: data[13] =%d\n", __func__, data[13]);
	menu_sensitivity = data[13];
#else
	u8 data[10];
	int ret;

	pr_debug("[TouchKey] %s called\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 10);
	menu_sensitivity = data[7];
#endif
	return sprintf(buf, "%d\n", menu_sensitivity);
}

static ssize_t touchkey_back_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
#if defined(CONFIG_MACH_Q1_BD) \
	|| (defined(CONFIG_MACH_C1) && defined(CONFIG_TARGET_LOCALE_KOR))\
	|| defined(CONFIG_MACH_T0)
	u8 data[14] = { 0, };
	int ret;

	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 14);

	pr_debug("[Touchkey] %s: data[11] =%d\n", __func__, data[11]);
	back_sensitivity = data[11];
#else
	u8 data[10];
	int ret;

	pr_debug("[TouchKey] %s called\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 10);
	back_sensitivity = data[9];
#endif
	return sprintf(buf, "%d\n", back_sensitivity);
}
#endif

#if defined(TK_HAS_AUTOCAL)
static ssize_t autocalibration_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int data;

	sscanf(buf, "%d\n", &data);

	if (data == 1)
		touchkey_autocalibration(tkey_i2c);

	return size;
}

static ssize_t autocalibration_status(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	u8 data[6];
	int ret;
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);

	pr_debug("[Touchkey] %s\n", __func__);

	ret = i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 6);
	if ((data[5] & TK_BIT_AUTOCAL))
		return sprintf(buf, "Enabled\n");
	else
		return sprintf(buf, "Disabled\n");

}
#endif				/* CONFIG_TARGET_LOCALE_NA */

static ssize_t touch_sensitivity_control(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	unsigned char data = 0x40;
	i2c_touchkey_write(tkey_i2c->client, &data, 1);
	return size;
}

static ssize_t set_touchkey_firm_version_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	return sprintf(buf, "0x%x\n", TK_FIRMWARE_VER);
}

static ssize_t set_touchkey_update_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int count = 0;
	int retry = 3;

	tkey_i2c->update_status = TK_UPDATE_DOWN;

	disable_irq(tkey_i2c->irq);

#ifdef TEST_JIG_MODE
	unsigned char get_touch = 0x40;
#endif

	while (retry--) {
		if (ISSP_main(tkey_i2c) == 0) {
			pr_err("[TouchKey] Touchkey_update succeeded\n");
			tkey_i2c->update_status = TK_UPDATE_PASS;
			count = 1;
			msleep(50);
			break;
		}
		pr_err("[TouchKey] touchkey_update failed... retry...\n");
	}
	if (retry <= 0) {
		/* disable ldo11 */
		tkey_i2c->pdata->power_on(0);
		count = 0;
		pr_err("[TouchKey] Touchkey_update fail\n");
		tkey_i2c->update_status = TK_UPDATE_FAIL;
		enable_irq(tkey_i2c->irq);
		return count;
	}

#ifdef TEST_JIG_MODE
	i2c_touchkey_write(tkey_i2c->client, &get_touch, 1);
#endif

#if defined(TK_HAS_AUTOCAL)
	touchkey_autocalibration(tkey_i2c);
#endif

	enable_irq(tkey_i2c->irq);

	return count;

}

static ssize_t set_touchkey_firm_version_read_show(struct device *dev,
						   struct device_attribute
						   *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	char data[3] = { 0, };
	int count;

	i2c_touchkey_read(tkey_i2c->client, KEYCODE_REG, data, 3);
	count = sprintf(buf, "0x%x\n", data[1]);

	pr_debug("[TouchKey] touch_version_read 0x%x\n", data[1]);
	pr_debug("[TouchKey] module_version_read 0x%x\n", data[2]);
	return count;
}

static ssize_t set_touchkey_firm_status_show(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int count = 0;

	pr_debug("[TouchKey] touch_update_read: update_status %d\n",
	       tkey_i2c->update_status);

	if (tkey_i2c->update_status == TK_UPDATE_PASS)
		count = sprintf(buf, "PASS\n");
	else if (tkey_i2c->update_status == TK_UPDATE_DOWN)
		count = sprintf(buf, "Downloading\n");
	else if (tkey_i2c->update_status == TK_UPDATE_FAIL)
		count = sprintf(buf, "Fail\n");

	return count;
}

static DEVICE_ATTR(recommended_version, S_IRUGO | S_IWUSR | S_IWGRP,
		   touch_version_read, touch_version_write);
static DEVICE_ATTR(updated_version, S_IRUGO | S_IWUSR | S_IWGRP,
		   touch_update_read, touch_update_write);
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   touchkey_led_control);
static DEVICE_ATTR(touchkey_menu, S_IRUGO | S_IWUSR | S_IWGRP,
		   touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO | S_IWUSR | S_IWGRP,
		   touchkey_back_show, NULL);

#if defined(TK_USE_4KEY)
static DEVICE_ATTR(touchkey_home, S_IRUGO, touchkey_home_show, NULL);
static DEVICE_ATTR(touchkey_search, S_IRUGO, touchkey_search_show, NULL);
#endif

static DEVICE_ATTR(touch_sensitivity, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   touch_sensitivity_control);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO | S_IWUSR | S_IWGRP,
	set_touchkey_update_show, NULL);
static DEVICE_ATTR(touchkey_firm_update_status, S_IRUGO | S_IWUSR | S_IWGRP,
	set_touchkey_firm_status_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO | S_IWUSR | S_IWGRP,
	set_touchkey_firm_version_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_touchkey_firm_version_read_show, NULL);
#ifdef LED_LDO_WITH_REGULATOR
static DEVICE_ATTR(touchkey_brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   brightness_control);
#endif

#if 0 /* #if defined(CONFIG_TARGET_LOCALE_NAATT) */
static DEVICE_ATTR(touchkey_autocal_start, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   set_touchkey_autocal_testmode);
#endif

#if defined(TK_HAS_AUTOCAL)
static DEVICE_ATTR(touchkey_raw_data0, S_IRUGO, touchkey_raw_data0_show, NULL);
static DEVICE_ATTR(touchkey_raw_data1, S_IRUGO, touchkey_raw_data1_show, NULL);
static DEVICE_ATTR(touchkey_raw_data2, S_IRUGO, touchkey_raw_data2_show, NULL);
static DEVICE_ATTR(touchkey_raw_data3, S_IRUGO, touchkey_raw_data3_show, NULL);
static DEVICE_ATTR(touchkey_idac0, S_IRUGO, touchkey_idac0_show, NULL);
static DEVICE_ATTR(touchkey_idac1, S_IRUGO, touchkey_idac1_show, NULL);
static DEVICE_ATTR(touchkey_idac2, S_IRUGO, touchkey_idac2_show, NULL);
static DEVICE_ATTR(touchkey_idac3, S_IRUGO, touchkey_idac3_show, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO, touchkey_threshold_show, NULL);
static DEVICE_ATTR(autocal_enable, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   autocalibration_enable);
static DEVICE_ATTR(autocal_stat, S_IRUGO | S_IWUSR | S_IWGRP,
		   autocalibration_status, NULL);
#endif

static struct attribute *touchkey_attributes[] = {
	&dev_attr_recommended_version.attr,
	&dev_attr_updated_version.attr,
	&dev_attr_brightness.attr,
	&dev_attr_touchkey_menu.attr,
	&dev_attr_touchkey_back.attr,
#if defined(TK_USE_4KEY)
	&dev_attr_touchkey_home.attr,
	&dev_attr_touchkey_search.attr,
#endif
	&dev_attr_touch_sensitivity.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
#ifdef LED_LDO_WITH_REGULATOR
	&dev_attr_touchkey_brightness.attr,
#endif
#if 0/* defined(CONFIG_TARGET_LOCALE_NAATT) */
	&dev_attr_touchkey_autocal_start.attr,
#endif
#if defined(TK_HAS_AUTOCAL)
	&dev_attr_touchkey_raw_data0.attr,
	&dev_attr_touchkey_raw_data1.attr,
	&dev_attr_touchkey_raw_data2.attr,
	&dev_attr_touchkey_raw_data3.attr,
	&dev_attr_touchkey_idac0.attr,
	&dev_attr_touchkey_idac1.attr,
	&dev_attr_touchkey_idac2.attr,
	&dev_attr_touchkey_idac3.attr,
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_autocal_enable.attr,
	&dev_attr_autocal_stat.attr,
#endif
	&dev_attr_timeout.attr,
    &dev_attr_force_disable.attr,
	NULL,
};

static struct attribute_group touchkey_attr_group = {
	.attrs = touchkey_attributes,
};

static int i2c_touchkey_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct touchkey_platform_data *pdata = client->dev.platform_data;
	struct touchkey_i2c *tkey_i2c;

	struct input_dev *input_dev;
	int err = 0;
	unsigned char data;
	int i;
	int ret;

	pr_debug("[TouchKey] i2c_touchkey_probe\n");

	if (pdata == NULL) {
		printk(KERN_ERR "%s: no pdata\n", __func__);
		return -ENODEV;
	}

	/*Check I2C functionality */
	ret = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if (ret == 0) {
		pr_err("[Touchkey] No I2C functionality found\n");
		ret = -ENODEV;
		return ret;
	}

	/*Obtain kernel memory space for touchkey i2c */
	tkey_i2c = kzalloc(sizeof(struct touchkey_i2c), GFP_KERNEL);
	if (NULL == tkey_i2c) {
		pr_err("[Touchkey] failed to allocate tkey_i2c.\n");
		return -ENOMEM;
	}
    tkey_i2c_local = tkey_i2c;

	input_dev = input_allocate_device();

	if (!input_dev) {
		pr_err("[Touchkey] failed to allocate input device\n");
		kfree(tkey_i2c);
		return -ENOMEM;
	}

	input_dev->name = "sec_touchkey";
	input_dev->phys = "sec_touchkey/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &client->dev;

	/*tkey_i2c*/
	tkey_i2c->pdata = pdata;
	tkey_i2c->input_dev = input_dev;
	tkey_i2c->client = client;
	tkey_i2c->irq = client->irq;
	tkey_i2c->name = "sec_touchkey";

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);
	set_bit(EV_KEY, input_dev->evbit);

	for (i = 1; i < touchkey_count; i++)
		set_bit(touchkey_keycode[i], input_dev->keybit);

	input_set_drvdata(input_dev, tkey_i2c);

	ret = input_register_device(input_dev);
	if (ret) {
		pr_err("[Touchkey] failed to register input device\n");
		input_free_device(input_dev);
		kfree(tkey_i2c);
		return err;
	}

	INIT_WORK(&tkey_i2c->update_work, touchkey_update_func);

	tkey_i2c->pdata->power_on(1);
	msleep(50);

	touchkey_enable = 1;
	data = 1;

	/*sysfs*/
	tkey_i2c->dev = device_create(sec_class, NULL, 0, NULL, "sec_touchkey");

	if (IS_ERR(tkey_i2c->dev)) {
		pr_err("[TouchKey] Failed to create device(tkey_i2c->dev)!\n");
		input_unregister_device(input_dev);
	} else {
		dev_set_drvdata(tkey_i2c->dev, tkey_i2c);
		ret = sysfs_create_group(&tkey_i2c->dev->kobj,
					&touchkey_attr_group);
		if (ret) {
			pr_err("[TouchKey]: failed to create sysfs group\n");
		}
	}

#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_C1)
	gpio_request(GPIO_OLED_DET, "OLED_DET");
	ret = gpio_get_value(GPIO_OLED_DET);
	pr_debug("[TouchKey] OLED_DET = %d\n", ret);

	if (ret == 0) {
		pr_debug("[TouchKey] device wasn't connected to board\n");

		input_unregister_device(input_dev);
		touchkey_probe = false;
		return -EBUSY;
	}
#else
	ret = touchkey_i2c_check(tkey_i2c);
	if (ret < 0) {
		pr_debug("[TouchKey] probe failed\n");
		input_unregister_device(input_dev);
		touchkey_probe = false;
		return -EBUSY;
	}
#endif

	ret =
		request_threaded_irq(tkey_i2c->irq, NULL, touchkey_interrupt,
				IRQF_DISABLED | IRQF_TRIGGER_FALLING |
				IRQF_ONESHOT, tkey_i2c->name, tkey_i2c);
	if (ret < 0) {
		pr_err("[Touchkey]: failed to request irq(%d) - %d\n",
			tkey_i2c->irq, ret);
		input_unregister_device(input_dev);
		touchkey_probe = false;
		return -EBUSY;
	}

	tkey_i2c->pdata->led_power_on(1);

#if defined(TK_HAS_FIRMWARE_UPDATE)
	ret = touchkey_firmware_update(tkey_i2c);
	if (ret < 0) {
		pr_err("[Touchkey]: failed firmware updating process (%d)\n",
			ret);
		input_unregister_device(input_dev);
		touchkey_probe = false;
		return -EBUSY;
	}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	tkey_i2c->early_suspend.suspend =
		(void *)sec_touchkey_early_suspend;
	tkey_i2c->early_suspend.resume =
		(void *)sec_touchkey_late_resume;
	register_early_suspend(&tkey_i2c->early_suspend);
#endif

#if defined(TK_HAS_AUTOCAL)
	touchkey_autocalibration(tkey_i2c);
#endif
	set_touchkey_debug('K');

    // init workqueue
    tkey_i2c->wq = create_singlethread_workqueue("tkey_i2c_wq");
    if (!tkey_i2c->wq) {
        ret = -ENOMEM;
        pr_err("[Touchkey] %s: could not create workqueue\n", __func__);
    }

    /* this is the thread function we run on the work queue */
	INIT_WORK(&tkey_i2c->work, touch_led_timedout_work);

	return 0;
}

struct i2c_driver touchkey_i2c_driver = {
	.driver = {
		.name = "sec_touchkey_driver",
	},
	.id_table = sec_touchkey_id,
	.probe = i2c_touchkey_probe,
};

static int __init touchkey_init(void)
{
	int ret = 0;

#if defined(CONFIG_MACH_M0)
	if (system_rev < TOUCHKEY_FW_UPDATEABLE_HW_REV) {
		pr_debug("[Touchkey] Doesn't support this board rev %d\n",
				system_rev);
		return 0;
	}
#elif defined(CONFIG_MACH_C1)
	if (system_rev < TOUCHKEY_FW_UPDATEABLE_HW_REV) {
		pr_debug("[Touchkey] Doesn't support this board rev %d\n",
				system_rev);
		return 0;
	}
#endif

#ifdef TEST_JIG_MODE
	unsigned char get_touch = 0x40;
#endif

	ret = i2c_add_driver(&touchkey_i2c_driver);

	if (ret) {
		pr_err("[TouchKey] registration failed, module not inserted.ret= %d\n",
	       ret);
	}
#ifdef TEST_JIG_MODE
	i2c_touchkey_write(tkey_i2c->client, &get_touch, 1);
#endif

    // init the touchled timer
    init_timer(&touch_led_timer);
    touch_led_timer.function = touch_led_timedout;

	return ret;
}

static void __exit touchkey_exit(void)
{
	pr_debug("[TouchKey] %s\n", __func__);
	i2c_del_driver(&touchkey_i2c_driver);
}

late_initcall(touchkey_init);
module_exit(touchkey_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("@@@");
MODULE_DESCRIPTION("touch keypad");
