/* linux/drivers/video/samsung/mdnie.c
 *
 * Register interface file for Samsung mDNIe driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/backlight.h>
#include <linux/platform_device.h>
#include <linux/mdnie.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/delay.h>
#include <linux/lcd.h>

#include "s3cfb.h"
#include "s3cfb_mdnie_kona.h"

#if defined(CONFIG_FB_S5P_NT71391)
#include "mdnie_table_kona.h"
#endif
#include "mdnie_color_tone_4412.h"

#if defined(CONFIG_FB_EBOOK_PANEL_SCENARIO)
#include "mdnie_table_ebook.h"
#endif

#if defined(CONFIG_FB_MDNIE_PWM)
#define MIN_BRIGHTNESS		0
#define DEFAULT_BRIGHTNESS		150
#if defined(CONFIG_FB_S5P_S6F1202A)
#define CABC_CUTOFF_BACKLIGHT_VALUE	40	/* 2.5% */
#elif defined(CONFIG_FB_S5P_S6C1372)
#define CABC_CUTOFF_BACKLIGHT_VALUE	34
#elif defined(CONFIG_FB_S5P_NT71391)
#define CABC_CUTOFF_BACKLIGHT_VALUE 31
#endif
#define MAX_BRIGHTNESS_LEVEL		255
#define MID_BRIGHTNESS_LEVEL		150
#define LOW_BRIGHTNESS_LEVEL		30
#define DIM_BRIGHTNESS_LEVEL		20
#endif

#define MDNIE_SYSFS_PREFIX		"/sdcard/mdnie/"
#define PANEL_COLOR_OFFSET_PATH	"/sys/class/lcd/panel/color_offset"

#if defined(CONFIG_TDMB) || defined(CONFIG_TARGET_LOCALE_NTT)
#define SCENARIO_IS_DMB(scenario)	((scenario >= DMB_NORMAL_MODE) && (scenario < DMB_MODE_MAX))
#else
#define SCENARIO_IS_DMB(scenario)	NULL
#endif

#define SCENARIO_IS_COLOR(scenario)			((scenario >= COLOR_TONE_1) && (scenario < COLOR_TONE_MAX))
#define SCENARIO_IS_VIDEO(scenario)			((scenario >= VIDEO_MODE) && (scenario <= VIDEO_COLD_MODE))
#define SCENARIO_IS_VALID(scenario)			(SCENARIO_IS_COLOR(scenario) || SCENARIO_IS_DMB(scenario) || scenario < SCENARIO_MAX)

#define ACCESSIBILITY_IS_VALID(accessibility)	(accessibility && (accessibility < ACCESSIBILITY_MAX))

#define ADDRESS_IS_SCR_BLACK(address)		(address >= MDNIE_REG_BLACK_R && address <= MDNIE_REG_BLACK_B)
#define ADDRESS_IS_SCR_RGB(address)			(address >= MDNIE_REG_RED_R && address <= MDNIE_REG_GREEN_B)

#define SCR_BLACK_MASK(value)				(value % MDNIE_REG_BLACK_R)
#define SCR_RGB_MASK(value)				(value % MDNIE_REG_RED_R)

struct class *mdnie_class;
struct mdnie_info *g_mdnie;

static int mdnie_send_sequence(struct mdnie_info *mdnie, const unsigned short *seq)
{
	int ret = 0, i = 0;
	const unsigned short *wbuf = NULL;

	if (IS_ERR_OR_NULL(seq)) {
		dev_err(mdnie->dev, "mdnie sequence is null\n");
		return -EPERM;
	}

	mutex_lock(&mdnie->dev_lock);

	wbuf = seq;

	mdnie_mask();

	while (wbuf[i] != END_SEQ) {
		ret += mdnie_write(wbuf[i], wbuf[i+1]);
		i += 2;
	}

	mdnie_unmask();

	mutex_unlock(&mdnie->dev_lock);

	return ret;
}

static struct mdnie_tuning_info *mdnie_request_table(struct mdnie_info *mdnie)
{
	struct mdnie_tuning_info *table = NULL;

	mutex_lock(&mdnie->lock);

#if defined(CONFIG_FB_EBOOK_PANEL_SCENARIO)
	if (mdnie->ebook == EBOOK_ON) {
		table = &ebook_table[mdnie->cabc];
		goto exit;
	}
#endif

	/* it will be removed next year */
	if (mdnie->negative == NEGATIVE_ON) {
		table = &negative_table[mdnie->cabc];
		goto exit;
	}

	if (ACCESSIBILITY_IS_VALID(mdnie->accessibility)) {
		table = &accessibility_table[mdnie->cabc][mdnie->accessibility];
		goto exit;
	} else if (SCENARIO_IS_DMB(mdnie->scenario)) {
#if defined(CONFIG_TDMB) || defined(CONFIG_TARGET_LOCALE_NTT)
		table = &tune_dmb[mdnie->mode];
#endif
		goto exit;
	} else if (SCENARIO_IS_COLOR(mdnie->scenario)) {
		table = &color_tone_table[mdnie->scenario % COLOR_TONE_1];
		goto exit;
	} else if (mdnie->scenario == CAMERA_MODE) {
		table = &camera_table[mdnie->outdoor];
		goto exit;
	} else if (mdnie->scenario < SCENARIO_MAX) {
		table = &tuning_table[mdnie->cabc][mdnie->mode][mdnie->scenario];
		goto exit;
	}

exit:
	mutex_unlock(&mdnie->lock);

	return table;
}

static struct mdnie_tuning_info *mdnie_request_etc_table(struct mdnie_info *mdnie)
{
	struct mdnie_tuning_info *table = NULL;

	mutex_lock(&mdnie->lock);

	if (SCENARIO_IS_VIDEO(mdnie->scenario))
		mdnie->tone = mdnie->scenario - VIDEO_MODE;
	else if (SCENARIO_IS_DMB(mdnie->scenario))
		mdnie->tone = mdnie->scenario % DMB_NORMAL_MODE;

	table =	&etc_table[mdnie->cabc][mdnie->outdoor][mdnie->tone];

	mutex_unlock(&mdnie->lock);

	return table;
}

static void mdnie_update_sequence(struct mdnie_info *mdnie, struct mdnie_tuning_info *table)
{
	unsigned short *wbuf = NULL;
	int ret;

	if (unlikely(mdnie->tuning)) {
		ret = mdnie_request_firmware(mdnie->path, &wbuf, table->name);
		if (ret < 0 && IS_ERR_OR_NULL(wbuf))
			goto exit;
		mdnie_send_sequence(mdnie, wbuf);
		kfree(wbuf);
	} else
		mdnie_send_sequence(mdnie, table->sequence);

exit:
	return;
}

static void mdnie_update(struct mdnie_info *mdnie)
{
	struct mdnie_tuning_info *table = NULL;

	if (!mdnie->enable) {
		dev_err(mdnie->dev, "mdnie state is off\n");
		return;
	}

	table = mdnie_request_table(mdnie);
	if (!IS_ERR_OR_NULL(table) && !IS_ERR_OR_NULL(table->sequence)) {
		mdnie_update_sequence(mdnie, table);
		dev_info(mdnie->dev, "%s\n", table->name);
	}

	if (!(SCENARIO_IS_VIDEO(mdnie->scenario) || SCENARIO_IS_DMB(mdnie->scenario)))
		goto exit;

	table = mdnie_request_etc_table(mdnie);
	if (!IS_ERR_OR_NULL(table) && !IS_ERR_OR_NULL(table->sequence)) {
		mdnie_update_sequence(mdnie, table);
		dev_info(mdnie->dev, "%s\n", table->name);
	}

exit:
	return;
}

#if defined(CONFIG_FB_MDNIE_PWM)
static int get_backlight_level_from_brightness(struct mdnie_info *mdnie, unsigned int brightness)
{
	unsigned int value;
	struct mdnie_backlight_value *pwm = mdnie->backlight;

	/* brightness tuning*/
	if (brightness >= MID_BRIGHTNESS_LEVEL) {
		value = (brightness - MID_BRIGHTNESS_LEVEL) *
		(pwm->max - pwm->mid) / (MAX_BRIGHTNESS_LEVEL-MID_BRIGHTNESS_LEVEL) + pwm->mid;
	} else if (brightness >= LOW_BRIGHTNESS_LEVEL) {
		value = (brightness - LOW_BRIGHTNESS_LEVEL) *
		(pwm->mid - pwm->low) / (MID_BRIGHTNESS_LEVEL-LOW_BRIGHTNESS_LEVEL) + pwm->low;
	} else if (brightness >= DIM_BRIGHTNESS_LEVEL) {
		value = (brightness - DIM_BRIGHTNESS_LEVEL) *
		(pwm->low - pwm->dim) / (LOW_BRIGHTNESS_LEVEL-DIM_BRIGHTNESS_LEVEL) + pwm->dim;
	} else if (brightness > 0)
		value = pwm->dim;
	else
		return 0;

	if (value > 1600)
		value = 1600;

	if (value < 16)
		value = 1;
	else
		value = value >> 4;

	return value;
}

static void mdnie_pwm_control(struct mdnie_info *mdnie, int value)
{
	mutex_lock(&mdnie->dev_lock);
	mdnie_write(MDNIE_REG_BANK_SEL, MDNIE_PWM_BANK);
	mdnie_write(MDNIE_REG_PWM_CONTROL, 0xC000 | value);
	mdnie_write(MDNIE_REG_MASK, 0);
	mutex_unlock(&mdnie->dev_lock);
}

static void mdnie_pwm_control_cabc(struct mdnie_info *mdnie, int value)
{
	int reg;
	const unsigned char *p_plut;
	u16 min_duty;
	unsigned idx;

	mutex_lock(&mdnie->dev_lock);

	idx = SCENARIO_IS_VIDEO(mdnie->scenario) ? LUT_VIDEO : LUT_DEFAULT;
	p_plut = power_lut[mdnie->power_lut_idx][idx];
	min_duty = p_plut[7] * value / 100;

	mdnie_write(MDNIE_REG_BANK_SEL, MDNIE_PWM_BANK);

	if (min_duty < 4)
		reg = 0xC000 | (max(1, (value * p_plut[3] / 100)));
	else {
		/*PowerLUT*/
		mdnie_write(MDNIE_REG_POWER_LUT0, (p_plut[0] * value / 100) << 8 | (p_plut[1] * value / 100));
		mdnie_write(MDNIE_REG_POWER_LUT2, (p_plut[2] * value / 100) << 8 | (p_plut[3] * value / 100));
		mdnie_write(MDNIE_REG_POWER_LUT4, (p_plut[4] * value / 100) << 8 | (p_plut[5] * value / 100));
		mdnie_write(MDNIE_REG_POWER_LUT6, (p_plut[6] * value / 100) << 8 | (p_plut[7] * value / 100));
		mdnie_write(MDNIE_REG_POWER_LUT8, (p_plut[8] * value / 100) << 8);

		reg = 0x5000 | (value << 4);
	}

	mdnie_write(MDNIE_REG_PWM_CONTROL, reg);
	mdnie_write(MDNIE_REG_MASK, 0);

	mutex_unlock(&mdnie->dev_lock);
}

void set_mdnie_pwm_value(struct mdnie_info *mdnie, int value)
{
	mdnie_pwm_control(mdnie, value);
}

static int update_brightness(struct mdnie_info *mdnie)
{
	unsigned int value;
	unsigned int brightness = mdnie->bd->props.brightness;

	value = get_backlight_level_from_brightness(mdnie, brightness);

	if (!mdnie->enable) {
		dev_err(mdnie->dev, "mdnie states is off\n");
		return 0;
	}

	if (brightness <= CABC_CUTOFF_BACKLIGHT_VALUE) {
		mdnie_pwm_control(mdnie, value);
	} else {
		if ((mdnie->cabc) && (mdnie->scenario != CAMERA_MODE) && !(mdnie->tuning))
			mdnie_pwm_control_cabc(mdnie, value);
		else
			mdnie_pwm_control(mdnie, value);
	}
	return 0;
}

static int mdnie_set_brightness(struct backlight_device *bd)
{
	struct mdnie_info *mdnie = bl_get_data(bd);
	int ret = 0;
	unsigned int brightness = bd->props.brightness;

	if (brightness < MIN_BRIGHTNESS ||
		brightness > bd->props.max_brightness) {
		dev_err(&bd->dev, "lcd brightness should be %d to %d. now %d\n",
			MIN_BRIGHTNESS, bd->props.max_brightness, brightness);
		brightness = bd->props.max_brightness;
	}

	if ((mdnie->enable) && (mdnie->bd_enable)) {
		ret = update_brightness(mdnie);
		dev_info(&bd->dev, "brightness=%d\n", bd->props.brightness);
		if (ret < 0)
			return -EINVAL;
	}

	return ret;
}

static int mdnie_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}

static const struct backlight_ops mdnie_backlight_ops  = {
	.get_brightness = mdnie_get_brightness,
	.update_status = mdnie_set_brightness,
};
#endif

static void update_color_position(struct mdnie_info *mdnie, u16 idx)
{
	u8 cabc, mode, scenario, outdoor, i;
	unsigned short *wbuf;

	dev_info(mdnie->dev, "%s: idx=%d\n", __func__, idx);

	mutex_lock(&mdnie->lock);

	for (cabc = 0; cabc < CABC_MAX; cabc++) {
		for (mode = 0; mode <= STANDARD; mode++) {
			for (scenario = 0; scenario < SCENARIO_MAX; scenario++) {
				wbuf = tuning_table[cabc][mode][scenario].sequence;
				if (IS_ERR_OR_NULL(wbuf))
					continue;
				i = 0;
				while (wbuf[i] != END_SEQ) {
					if (ADDRESS_IS_SCR_BLACK(wbuf[i]))
						wbuf[i+1] = tune_scr_setting[idx][SCR_BLACK_MASK(wbuf[i])];
					i += 2;
				}
			}
		}
	}

	for (outdoor = 0; outdoor < OUTDOOR_MAX; outdoor++) {
		wbuf = camera_table[outdoor].sequence;
		if (IS_ERR_OR_NULL(wbuf))
			continue;
		i = 0;
		while (wbuf[i] != END_SEQ) {
			if (ADDRESS_IS_SCR_BLACK(wbuf[i]))
				wbuf[i+1] = tune_scr_setting[idx][SCR_BLACK_MASK(wbuf[i])];
			i += 2;
		}
	}

	mutex_unlock(&mdnie->lock);
}

static int get_panel_color_position(struct mdnie_info *mdnie, int *result)
{
	int ret = 0;
	char *fp = NULL;
	unsigned int offset[2] = {0,};

	if (likely(mdnie->color_correction))
		goto skip_color_correction;

	ret = mdnie_open_file(PANEL_COLOR_OFFSET_PATH, &fp);
	if (IS_ERR_OR_NULL(fp) || ret <= 0) {
		dev_info(mdnie->dev, "%s: open fail: %s, %d\n", __func__, PANEL_COLOR_OFFSET_PATH, ret);
		ret = -EINVAL;
		goto skip_color_correction;
	}

	ret = sscanf(fp, "0x%x, 0x%x", &offset[0], &offset[1]);
	if (!(offset[0] + offset[1]) || ret != 2) {
		dev_info(mdnie->dev, "%s: 0x%x, 0x%x\n", __func__, offset[0], offset[1]);
		ret = -EINVAL;
		goto skip_color_correction;
	}

	ret = mdnie_calibration(offset[0], offset[1], result);
	dev_info(mdnie->dev, "%s: %x, %x, idx=%d\n", __func__, offset[0], offset[1], ret - 1);

skip_color_correction:
	mdnie->color_correction = 1;
	if (!IS_ERR_OR_NULL(fp))
		kfree(fp);

	return ret;
}

static ssize_t mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", mdnie->mode);
}

static ssize_t mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value = 0;
	int result[5] = {0,};
	int ret;

	ret = kstrtoul(buf, 0, (unsigned long *)&value);
	if (ret)
		return -EINVAL;

	dev_info(dev, "%s :: value=%d\n", __func__, value);

	if (value >= MODE_MAX) {
		value = STANDARD;
		return -EINVAL;
	}

	mutex_lock(&mdnie->lock);
	mdnie->mode = value;
	mutex_unlock(&mdnie->lock);

	if (!mdnie->color_correction) {
		ret = get_panel_color_position(mdnie, result);
		if (ret > 0)
			update_color_position(mdnie, ret - 1);
	}

	mdnie_update(mdnie);
#if defined(CONFIG_FB_MDNIE_PWM)
	if ((mdnie->enable) && (mdnie->bd_enable))
		update_brightness(mdnie);
#endif

	return count;
}


static ssize_t scenario_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", mdnie->scenario);
}

static ssize_t scenario_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

	ret = kstrtoul(buf, 0, (unsigned long *)&value);

	dev_info(dev, "%s :: value=%d\n", __func__, value);

	if (!SCENARIO_IS_VALID(value))
		value = UI_MODE;

#if defined(CONFIG_FB_MDNIE_PWM)
	if (value >= SCENARIO_MAX)
		value = UI_MODE;
#endif

	mutex_lock(&mdnie->lock);
	mdnie->scenario = value;
	mutex_unlock(&mdnie->lock);

	mdnie_update(mdnie);
#if defined(CONFIG_FB_MDNIE_PWM)
	if ((mdnie->enable) && (mdnie->bd_enable))
		update_brightness(mdnie);
#endif

	return count;
}


static ssize_t outdoor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", mdnie->outdoor);
}

static ssize_t outdoor_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

	ret = kstrtoul(buf, 0, (unsigned long *)&value);

	dev_info(dev, "%s :: value=%d\n", __func__, value);

	if (value >= OUTDOOR_MAX)
		value = OUTDOOR_OFF;

	value = (value) ? OUTDOOR_ON : OUTDOOR_OFF;

	mutex_lock(&mdnie->lock);
	mdnie->outdoor = value;
	mutex_unlock(&mdnie->lock);

	mdnie_update(mdnie);

	return count;
}


#if defined(CONFIG_FB_MDNIE_PWM)
static ssize_t cabc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", mdnie->cabc);
}

static ssize_t cabc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

#if defined(CONFIG_FB_S5P_S6C1372)
	if (mdnie->auto_brightness)
		return -EINVAL;
#endif

	ret = strict_strtoul(buf, 0, (unsigned long *)&value);

	dev_info(dev, "%s :: value=%d\n", __func__, value);

	if (value >= CABC_MAX)
		value = CABC_OFF;

	value = (value) ? CABC_ON : CABC_OFF;

	mutex_lock(&mdnie->lock);
	mdnie->cabc = value;
	mutex_unlock(&mdnie->lock);

	mdnie_update(mdnie);
	if ((mdnie->enable) && (mdnie->bd_enable))
		update_brightness(mdnie);

	return count;
}

static ssize_t auto_brightness_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	char *pos = buf;
	int i;

	pos += sprintf(pos, "%d, %d, ", mdnie->auto_brightness, mdnie->power_lut_idx);
	for (i = 0; i < 5; i++)
		pos += sprintf(pos, "0x%02x, ", power_lut[mdnie->power_lut_idx][0][i]);
	pos += sprintf(pos, "\n");

	return pos - buf;
}

static ssize_t auto_brightness_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = strict_strtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (mdnie->auto_brightness != value) {
			dev_info(dev, "%s - %d -> %d\n", __func__, mdnie->auto_brightness, value);
			mutex_lock(&mdnie->dev_lock);
			mdnie->auto_brightness = value;
#if defined(CONFIG_FB_S5P_S6C1372)
			mutex_lock(&mdnie->lock);
			mdnie->cabc = (value) ? CABC_ON : CABC_OFF;
			mutex_unlock(&mdnie->lock);
#endif
			if (mdnie->auto_brightness >= 5)
				mdnie->power_lut_idx = LUT_LEVEL_OUTDOOR_2;
			else if (mdnie->auto_brightness == 4)
				mdnie->power_lut_idx = LUT_LEVEL_OUTDOOR_1;
			else
				mdnie->power_lut_idx = LUT_LEVEL_MANUAL_AND_INDOOR;
			mutex_unlock(&mdnie->dev_lock);
			mdnie_update(mdnie);
			if (mdnie->bd_enable)
				update_brightness(mdnie);
		}
	}
	return size;
}

static DEVICE_ATTR(auto_brightness, 0644, auto_brightness_show, auto_brightness_store);
#endif

static ssize_t tuning_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	char *pos = buf;
	struct mdnie_tuning_info *table;
	int ret, i;
	unsigned short *wbuf;

	pos += sprintf(pos, "++ %s: %s\n", __func__, mdnie->path);

	if (!mdnie->tuning) {
		pos += sprintf(pos, "tunning mode is off\n");
		goto exit;
	}

	if (strncmp(mdnie->path, MDNIE_SYSFS_PREFIX, sizeof(MDNIE_SYSFS_PREFIX) - 1)) {
		pos += sprintf(pos, "file path is invalid, %s\n", mdnie->path);
		goto exit;
	}

	table = mdnie_request_table(mdnie);
	if (!IS_ERR_OR_NULL(table)) {
		ret = mdnie_request_firmware(mdnie->path, &wbuf, table->name);
		i = 0;
		while (wbuf[i] != END_SEQ) {
			pos += sprintf(pos, "0x%04x, 0x%04x\n", wbuf[i], wbuf[i+1]);
			i += 2;
		}
		if (!IS_ERR_OR_NULL(wbuf))
			kfree(wbuf);
		pos += sprintf(pos, "%s\n", table->name);
	}

	if (!(SCENARIO_IS_VIDEO(mdnie->scenario) || SCENARIO_IS_DMB(mdnie->scenario)))
		goto exit;

	table = mdnie_request_etc_table(mdnie);
	if (!IS_ERR_OR_NULL(table)) {
		ret = mdnie_request_firmware(mdnie->path, &wbuf, table->name);
		i = 0;
		while (wbuf[i] != END_SEQ) {
			pos += sprintf(pos, "0x%04x, 0x%04x\n", wbuf[i], wbuf[i+1]);
			i += 2;
		}
		if (!IS_ERR_OR_NULL(wbuf))
			kfree(wbuf);
		pos += sprintf(pos, "%s\n", table->name);
	}

exit:
	pos += sprintf(pos, "-- %s\n", __func__);

	return pos - buf;
}

static ssize_t tuning_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	int ret;

	if (sysfs_streq(buf, "0") || sysfs_streq(buf, "1")) {
		ret = kstrtoul(buf, 0, (unsigned long *)&mdnie->tuning);
		if (!mdnie->tuning)
			memset(mdnie->path, 0, sizeof(mdnie->path));
		dev_info(dev, "%s :: %s\n", __func__, mdnie->tuning ? "enable" : "disable");
	} else {
		if (!mdnie->tuning)
			return count;

		if (count > (sizeof(mdnie->path) - sizeof(MDNIE_SYSFS_PREFIX))) {
			dev_err(dev, "file name %s is too long\n", mdnie->path);
			return -ENOMEM;
		}

		memset(mdnie->path, 0, sizeof(mdnie->path));
		snprintf(mdnie->path, sizeof(MDNIE_SYSFS_PREFIX) + count-1, "%s%s", MDNIE_SYSFS_PREFIX, buf);
		dev_info(dev, "%s :: %s\n", __func__, mdnie->path);

		mdnie_update(mdnie);
	}

	return count;
}

static ssize_t negative_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", mdnie->negative);
}

static ssize_t negative_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

	ret = kstrtoul(buf, 0, (unsigned long *)&value);

	dev_info(dev, "%s :: value=%d\n", __func__, value);

	if (ret < 0)
		return ret;
	else {
		if (mdnie->negative == value)
			return count;

		if (value >= NEGATIVE_MAX)
			value = NEGATIVE_OFF;

		value = (value) ? NEGATIVE_ON : NEGATIVE_OFF;

		mutex_lock(&mdnie->lock);
		mdnie->negative = value;
		mutex_unlock(&mdnie->lock);

		mdnie_update(mdnie);
	}
	return count;
}

static ssize_t accessibility_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	char *pos = buf;
	unsigned short *wbuf;
	int i = 0;

	pos += sprintf(pos, "%d, ", mdnie->accessibility);
	if (mdnie->accessibility == COLOR_BLIND) {
		if (!IS_ERR_OR_NULL(accessibility_table[mdnie->cabc][COLOR_BLIND].sequence)) {
			wbuf = accessibility_table[mdnie->cabc][COLOR_BLIND].sequence;
			while (wbuf[i] != END_SEQ) {
				if (ADDRESS_IS_SCR_RGB(wbuf[i]))
					pos += sprintf(pos, "0x%04x, ", wbuf[i+1]);
				i += 2;
			}
		}
	}
	pos += sprintf(pos, "\n");

	return pos - buf;
}

static ssize_t accessibility_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value, s[9], cabc, i, len = 0;
	int ret;
	unsigned short *wbuf;
	char str[100] = {0,};

	ret = sscanf(buf, "%d %x %x %x %x %x %x %x %x %x",
		&value, &s[0], &s[1], &s[2], &s[3],
		&s[4], &s[5], &s[6], &s[7], &s[8]);

	dev_info(dev, "%s :: value=%d\n", __func__, value);

	if (ret < 0)
		return ret;
	else {
		if (value >= ACCESSIBILITY_MAX)
			value = ACCESSIBILITY_OFF;

		mutex_lock(&mdnie->lock);
		mdnie->accessibility = value;
		if (value == COLOR_BLIND) {
			if (ret != 10) {
				mutex_unlock(&mdnie->lock);
				return -EINVAL;
			}

			for (cabc = 0; cabc < CABC_MAX; cabc++) {
				wbuf = accessibility_table[cabc][COLOR_BLIND].sequence;
				if (IS_ERR_OR_NULL(wbuf))
					continue;
				i = 0;
				while (wbuf[i] != END_SEQ) {
					if (ADDRESS_IS_SCR_RGB(wbuf[i]))
						wbuf[i+1] = s[SCR_RGB_MASK(wbuf[i])];
					i += 2;
				}
			}

			i = 0;
			len = sprintf(str + len, "%s :: ", __func__);
			while (len < sizeof(str) && i < ARRAY_SIZE(s)) {
				len += sprintf(str + len, "0x%04x, ", s[i]);
				i++;
			}
			dev_info(dev, "%s\n", str);
		}
		mutex_unlock(&mdnie->lock);

		mdnie_update(mdnie);
	}
	return count;
}

static ssize_t color_correct_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	char *pos = buf;
	int i;
	int result[5] = {0,};

	if (!mdnie->color_correction)
		return -EINVAL;

	for (i = 1; i < ARRAY_SIZE(result); i++)
		pos += sprintf(pos, "F%d= %d, ", i, result[i]);
	pos += sprintf(pos, "idx=%d\n", get_panel_color_position(mdnie, result));

	return pos - buf;
}

#if defined(CONFIG_FB_EBOOK_PANEL_SCENARIO)
static ssize_t ebook_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", mdnie->ebook);
}

static ssize_t ebook_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

	ret = strict_strtoul(buf, 0, (unsigned long *)&value);

	dev_info(dev, "%s :: value=%d\n", __func__, value);

	if (ret < 0)
		return ret;
	else {
		if (mdnie->ebook == value)
			return count;

		if (value >= EBOOK_MAX)
			value = EBOOK_OFF;

		value = (value) ? EBOOK_ON : EBOOK_OFF;

		mutex_lock(&mdnie->lock);
		mdnie->ebook = value;
		mutex_unlock(&mdnie->lock);

		mdnie_update(mdnie);
	}
	return count;
}
#endif

static struct device_attribute mdnie_attributes[] = {
	__ATTR(mode, 0664, mode_show, mode_store),
	__ATTR(scenario, 0664, scenario_show, scenario_store),
	__ATTR(outdoor, 0664, outdoor_show, outdoor_store),
#if defined(CONFIG_FB_MDNIE_PWM)
	__ATTR(cabc, 0664, cabc_show, cabc_store),
#endif
	__ATTR(tuning, 0664, tuning_show, tuning_store),
	__ATTR(negative, 0664, negative_show, negative_store),
	__ATTR(accessibility, 0664, accessibility_show, accessibility_store),
	__ATTR(color_correct, 0444, color_correct_show, NULL),
#if defined(CONFIG_FB_EBOOK_PANEL_SCENARIO)
	__ATTR(ebook, 0664, ebook_show, ebook_store),
#endif
	__ATTR_NULL,
};

#ifdef CONFIG_PM
#if defined(CONFIG_HAS_EARLYSUSPEND)
#if defined(CONFIG_FB_MDNIE_PWM)
static void mdnie_early_suspend(struct early_suspend *h)
{
	struct mdnie_info *mdnie = container_of(h, struct mdnie_info, early_suspend);
	struct lcd_platform_data *pd = mdnie->lcd_pd;

	dev_info(mdnie->dev, "+%s\n", __func__);

	mdnie->bd_enable = FALSE;

	if (mdnie->enable)
		mdnie_pwm_control(mdnie, 0);

	if (pd && pd->power_on)
		pd->power_on(NULL, 0);

	dev_info(mdnie->dev, "-%s\n", __func__);

	return;
}
#endif

static void mdnie_late_resume(struct early_suspend *h)
{
	struct mdnie_info *mdnie = container_of(h, struct mdnie_info, early_suspend);
#if defined(CONFIG_FB_MDNIE_PWM)
	struct lcd_platform_data *pd = mdnie->lcd_pd;
#endif

	dev_info(mdnie->dev, "+%s\n", __func__);

#if defined(CONFIG_FB_MDNIE_PWM)
	if (mdnie->enable)
		mdnie_pwm_control(mdnie, 0);

	if (pd && pd->power_on)
		pd->power_on(NULL, 1);

	if (mdnie->enable) {
		dev_info(&mdnie->bd->dev, "brightness=%d\n", mdnie->bd->props.brightness);
		update_brightness(mdnie);
	}

	mdnie->bd_enable = TRUE;
#endif

	mdnie_update(mdnie);

	dev_info(mdnie->dev, "-%s\n", __func__);

	return;
}
#endif
#endif

static int mdnie_probe(struct platform_device *pdev)
{
#if defined(CONFIG_FB_MDNIE_PWM)
	struct platform_mdnie_data *pdata = pdev->dev.platform_data;
#endif
	struct mdnie_info *mdnie;
	int ret = 0;

	mdnie_class = class_create(THIS_MODULE, dev_name(&pdev->dev));
	if (IS_ERR_OR_NULL(mdnie_class)) {
		pr_err("failed to create mdnie class\n");
		ret = -EINVAL;
		goto error0;
	}

	mdnie_class->dev_attrs = mdnie_attributes;

	mdnie = kzalloc(sizeof(struct mdnie_info), GFP_KERNEL);
	if (!mdnie) {
		pr_err("failed to allocate mdnie\n");
		ret = -ENOMEM;
		goto error1;
	}

	mdnie->dev = device_create(mdnie_class, &pdev->dev, 0, &mdnie, "mdnie");
	if (IS_ERR_OR_NULL(mdnie->dev)) {
		pr_err("failed to create mdnie device\n");
		ret = -EINVAL;
		goto error2;
	}

#if defined(CONFIG_FB_MDNIE_PWM)
	if (!pdata) {
		pr_err("no platform data specified\n");
		ret = -EINVAL;
		goto error2;
	}

	mdnie->bd = backlight_device_register("panel", mdnie->dev,
		mdnie, &mdnie_backlight_ops, NULL);
	mdnie->bd->props.max_brightness = MAX_BRIGHTNESS_LEVEL;
	mdnie->bd->props.brightness = DEFAULT_BRIGHTNESS;
	mdnie->bd_enable = TRUE;
	mdnie->lcd_pd = pdata->lcd_pd;

	ret = device_create_file(&mdnie->bd->dev, &dev_attr_auto_brightness);
	if (ret < 0)
		dev_err(&mdnie->bd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif

	mdnie->scenario = UI_MODE;
	mdnie->mode = STANDARD;
	mdnie->tone = TONE_NORMAL;
	mdnie->outdoor = OUTDOOR_OFF;
	mdnie->enable = TRUE;
	mdnie->tuning = FALSE;
	mdnie->negative = NEGATIVE_OFF;
	mdnie->accessibility = ACCESSIBILITY_OFF;
	mdnie->cabc = CABC_OFF;

#if defined(CONFIG_FB_MDNIE_PWM)
#if defined(CONFIG_FB_S5P_S6F1202A)
	mdnie->cabc = CABC_ON;
#endif
	mdnie->power_lut_idx = LUT_LEVEL_MANUAL_AND_INDOOR;
	mdnie->auto_brightness = 0;
#endif

#if defined(CONFIG_FB_EBOOK_PANEL_SCENARIO)
	mdnie->ebook = EBOOK_OFF;
#endif

	mutex_init(&mdnie->lock);
	mutex_init(&mdnie->dev_lock);

	platform_set_drvdata(pdev, mdnie);
	dev_set_drvdata(mdnie->dev, mdnie);

#ifdef CONFIG_HAS_EARLYSUSPEND
#if defined(CONFIG_FB_MDNIE_PWM)
	mdnie->early_suspend.suspend = mdnie_early_suspend;
#endif
	mdnie->early_suspend.resume = mdnie_late_resume;
	mdnie->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	register_early_suspend(&mdnie->early_suspend);
#endif


#if defined(CONFIG_FB_MDNIE_PWM)
	dev_info(mdnie->dev, "lcdtype = %d\n", pdata->display_type);
	if (pdata->display_type > ARRAY_SIZE(backlight_table))
		pdata->display_type = 0;
	mdnie->backlight = &backlight_table[pdata->display_type];
#endif

#if defined(CONFIG_FB_S5P_S6F1202A)
	if (pdata->display_type == 0) {
		memcpy(tuning_table, tuning_table_hydis, sizeof(tuning_table));
		memcpy(etc_table, etc_table_hydis, sizeof(etc_table));
		memcpy(camera_table, camera_table_hydis, sizeof(camera_table));
	} else if (pdata->display_type == 1) {
		memcpy(tuning_table, tuning_table_sec, sizeof(tuning_table));
		memcpy(etc_table, etc_table_sec, sizeof(etc_table));
		memcpy(camera_table, camera_table_sec, sizeof(camera_table));
	} else if (pdata->display_type == 2) {
		memcpy(tuning_table, tuning_table_boe, sizeof(tuning_table));
		memcpy(etc_table, etc_table_boe, sizeof(etc_table));
		memcpy(camera_table, camera_table_boe, sizeof(camera_table));
	}
#endif

	g_mdnie = mdnie;

	mdnie_update(mdnie);

	dev_info(mdnie->dev, "registered successfully\n");

	return 0;

error2:
	kfree(mdnie);
error1:
	class_destroy(mdnie_class);
error0:
	return ret;
}

static int mdnie_remove(struct platform_device *pdev)
{
	struct mdnie_info *mdnie = dev_get_drvdata(&pdev->dev);

#if defined(CONFIG_FB_MDNIE_PWM)
	backlight_device_unregister(mdnie->bd);
#endif
	class_destroy(mdnie_class);
	kfree(mdnie);

	return 0;
}

static void mdnie_shutdown(struct platform_device *pdev)
{
#if defined(CONFIG_FB_MDNIE_PWM)
	struct mdnie_info *mdnie = dev_get_drvdata(&pdev->dev);
	struct lcd_platform_data *pd = NULL;
	pd = mdnie->lcd_pd;

	dev_info(mdnie->dev, "+%s\n", __func__);

	mdnie->bd_enable = FALSE;

	if (mdnie->enable)
		mdnie_pwm_control(mdnie, 0);

	if (pd && pd->power_on)
		pd->power_on(NULL, 0);

	dev_info(mdnie->dev, "-%s\n", __func__);
#endif
}


static struct platform_driver mdnie_driver = {
	.driver		= {
		.name	= "mdnie",
		.owner	= THIS_MODULE,
	},
	.probe		= mdnie_probe,
	.remove		= mdnie_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= mdnie_suspend,
	.resume		= mdnie_resume,
#endif
	.shutdown	= mdnie_shutdown,
};

static int __init mdnie_init(void)
{
	return platform_driver_register(&mdnie_driver);
}
module_init(mdnie_init);

static void __exit mdnie_exit(void)
{
	platform_driver_unregister(&mdnie_driver);
}
module_exit(mdnie_exit);

MODULE_DESCRIPTION("mDNIe Driver");
MODULE_LICENSE("GPL");

