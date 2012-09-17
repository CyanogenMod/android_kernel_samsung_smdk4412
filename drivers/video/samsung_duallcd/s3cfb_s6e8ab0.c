/* linux/drivers/video/samsung/s3cfb_s6e8ab0.c
 *
 * MIPI-DSI based AMS767KC01 AMOLED lcd panel driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/backlight.h>
#include <linux/lcd.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-dsim.h>
#include <mach/dsim.h>
#include <mach/mipi_ddi.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define SMART_DIMMING

#include "s3cfb.h"
#include "s5p-dsim.h"
#include "s6e8ab0_param.h"
#include "s6e8ab0_gamma.h"
#ifdef SMART_DIMMING
#include "smart_dimming_s6e8ab0.h"
#endif

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

#define MIN_BRIGHTNESS	0
#define MAX_BRIGHTNESS	255
#define DEFAULT_BRIGHTNESS	150	/* this is not candela */
#define DEFAULT_GAMMA_LEVEL	GAMMA_150CD

#define LDI_ID_REG			0xD1
#define LDI_ID_LEN			3

#ifdef SMART_DIMMING
#define LDI_MTP_LENGTH		24
#define LDI_MTP_ADDR			0xD3

#define ELVSS_OFFSET_MAX		0x00
#define ELVSS_OFFSET_2		0x04
#define ELVSS_OFFSET_1		0x08
#define ELVSS_OFFSET_MIN		0x0C

#define ELVSS_MIN_VALUE		0x81
#define ELVSS_MAX_VALUE_MODE0	0xA1
#define ELVSS_MAX_VALUE_MODE1	0xA9

#define ELVSS_MODE0_MIN_VOLTAGE		64
#define ELVSS_MODE1_MIN_VOLTAGE		64

struct str_elvss {
	u8 reference;
	u8 limit;
};
#endif

struct lcd_info {
	unsigned int			bl;
	unsigned int			acl_enable;
	unsigned int			cur_acl;
	unsigned int			current_bl;

	unsigned int			ldi_enable;
	unsigned int			power;
	struct mutex			lock;
	struct mutex			bl_lock;

	struct device			*dev;
	struct lcd_device		*ld;
	struct backlight_device		*bd;
	struct lcd_platform_data	*lcd_pd;
	struct early_suspend		early_suspend;

	unsigned char			id[LDI_ID_LEN];

	unsigned char			**gamma_table;
	unsigned char			**elvss_table;
	unsigned char			elvss_cmd_length;

#ifdef SMART_DIMMING
	unsigned int			support_elvss;
	struct str_smart_dim		smart;
	struct str_elvss		elvss;
#endif
	unsigned int			connected;

	struct dsim_global		*dsim;
};

static int s6e8ax0_write(struct lcd_info *lcd, const unsigned char *seq, int len)
{
	int size;
	const unsigned char *wbuf;

	if (!lcd->connected)
		return 0;

	mutex_lock(&lcd->lock);

	size = len;
	wbuf = seq;

	if (size == 1)
		lcd->dsim->ops->cmd_write(lcd->dsim, DCS_WR_NO_PARA, wbuf[0], 0);
	else if (size == 2)
		lcd->dsim->ops->cmd_write(lcd->dsim, DCS_WR_1_PARA, wbuf[0], wbuf[1]);
	else
		lcd->dsim->ops->cmd_write(lcd->dsim, DCS_LONG_WR, (unsigned int)wbuf, size);

	mutex_unlock(&lcd->lock);

	return 0;
}

static int _s6e8ax0_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf)
{
	int ret = 0;

	if (!lcd->connected)
		return ret;

	mutex_lock(&lcd->lock);

	if (lcd->dsim->ops->cmd_read)
		ret = lcd->dsim->ops->cmd_read(lcd->dsim, addr, count, buf);

	mutex_unlock(&lcd->lock);

	return ret;
}

static int s6e8ax0_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf, u8 retry_cnt)
{
	int ret = 0;

read_retry:
	ret = _s6e8ax0_read(lcd, addr, count, buf);
	if (!ret) {
		if (retry_cnt) {
			printk(KERN_WARNING "[WARN:LCD] %s : retry cnt : %d\n", __func__, retry_cnt);
			retry_cnt--;
			goto read_retry;
		} else
			printk(KERN_ERR "[ERROR:LCD] %s : 0x%02x read failed\n", __func__, addr);
	}

	return ret;
}

#if 0
static int get_backlight_level_from_brightness(int brightness)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is only supported from 0 to 24 */

	switch (brightness) {
	case 0:
		backlightlevel = GAMMA_30CD;
		break;
	case 1 ... 29:
		backlightlevel = GAMMA_30CD;
		break;
	case 30 ... 34:
		backlightlevel = GAMMA_40CD;
		break;
	case 35 ... 44:
		backlightlevel = GAMMA_50CD;
		break;
	case 45 ... 54:
		backlightlevel = GAMMA_60CD;
		break;
	case 55 ... 64:
		backlightlevel = GAMMA_70CD;
		break;
	case 65 ... 74:
		backlightlevel = GAMMA_80CD;
		break;
	case 75 ... 83:
		backlightlevel = GAMMA_90CD;
		break;
	case 84 ... 93:
		backlightlevel = GAMMA_100CD;
		break;
	case 94 ... 103:
		backlightlevel = GAMMA_105CD;
		break;
	case 104 ... 113:
		backlightlevel = GAMMA_110CD;
		break;
	case 114 ... 122:
		backlightlevel = GAMMA_120CD;
		break;
	case 123 ... 132:
		backlightlevel = GAMMA_130CD;
		break;
	case 133 ... 142:
		backlightlevel = GAMMA_140CD;
		break;
	case 143 ... 152:
		backlightlevel = GAMMA_150CD;
		break;
	case 153 ... 162:
		backlightlevel = GAMMA_160CD;
		break;
	case 163 ... 171:
		backlightlevel = GAMMA_170CD;
		break;
	case 172 ... 181:
		backlightlevel = GAMMA_180CD;
		break;
	case 182 ... 191:
		backlightlevel = GAMMA_190CD;
		break;
	case 192 ... 201:
		backlightlevel = GAMMA_200CD;
		break;
	case 202 ... 210:
		backlightlevel = GAMMA_205CD;
		break;
	case 211 ... 220:
		backlightlevel = GAMMA_210CD;
		break;
	case 221 ... 230:
		backlightlevel = GAMMA_220CD;
		break;
	case 231 ... 240:
		backlightlevel = GAMMA_230CD;
		break;
	case 241 ... 250:
		backlightlevel = GAMMA_240CD;
		break;
	case 251 ... 255:
		backlightlevel = GAMMA_250CD;
		break;
	default:
		backlightlevel = DEFAULT_GAMMA_LEVEL;
		break;
	}
	return backlightlevel;
}
#endif

static int s6e8ax0_gamma_ctl(struct lcd_info *lcd)
{
	/* Gamma Select */
	s6e8ax0_write(lcd, SEQ_GAMMA_SELECT, ARRAY_SIZE(SEQ_GAMMA_SELECT));

	s6e8ax0_write(lcd, lcd->gamma_table[lcd->bl], GAMMA_PARAM_SIZE);

	/* Gamma Set Update */
	s6e8ax0_write(lcd, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));

	return 0;
}

static int s6e8ax0_set_acl(struct lcd_info *lcd)
{
	int ret = 0;

	if (lcd->acl_enable) {
		if (lcd->cur_acl == 0) {
			if (lcd->bl == 0 || lcd->bl == 1) {
				s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d, acl_off\n", __func__, lcd->cur_acl);
			} else
				s6e8ax0_write(lcd, SEQ_ACL_ON, ARRAY_SIZE(SEQ_ACL_ON));
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d, acl_on\n", __func__, lcd->cur_acl);
		}
		switch (lcd->bl) {
		case GAMMA_30CD ... GAMMA_40CD: /* 30cd ~ 40cd */
			if (lcd->cur_acl != 0) {
				s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				lcd->cur_acl = 0;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		default:
			if (lcd->cur_acl != 40) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[1], ACL_PARAM_SIZE);
				lcd->cur_acl = 40;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		}
	} else {
		s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
		lcd->cur_acl = 0;
		dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d, acl_off\n", __func__, lcd->cur_acl);
	}

	if (ret) {
		ret = -EPERM;
		goto acl_err;
	}

acl_err:
	return ret;
}

static int s6e8ax0_set_elvss(struct lcd_info *lcd)
{
	int ret = 0;
	u32 candela = candela_table[lcd->bl];

	switch (candela) {
	case 0 ... 100:
		ret = s6e8ax0_write(lcd, lcd->elvss_table[0], lcd->elvss_cmd_length);
		break;
	case 101 ... 150:
		ret = s6e8ax0_write(lcd, lcd->elvss_table[1], lcd->elvss_cmd_length);
		break;
	case 151 ... 200:
		ret = s6e8ax0_write(lcd, lcd->elvss_table[2], lcd->elvss_cmd_length);
		break;
	case 201 ... 250:
		ret = s6e8ax0_write(lcd, lcd->elvss_table[3], lcd->elvss_cmd_length);
		break;
	default:
		break;
	}

	dev_dbg(&lcd->ld->dev, "level  = %d\n", lcd->bl);

	if (ret) {
		ret = -EPERM;
		goto elvss_err;
	}

elvss_err:
	return ret;
}

#ifdef SMART_DIMMING
static u8 get_elvss_offset(u32 elvss_level)
{
	u8 offset = 0;

	switch (elvss_level) {
	case 0:
		offset = ELVSS_OFFSET_MIN;
		break;
	case 1:
		offset = ELVSS_OFFSET_1;
		break;
	case 2:
		offset = ELVSS_OFFSET_2;
		break;
	case 3:
		offset = ELVSS_OFFSET_MAX;
		break;
	default:
		offset = ELVSS_OFFSET_MAX;
		break;
	}
	return offset;
}

static u8 get_elvss_value(struct lcd_info *lcd, u8 elvss_level)
{
	u8 ref = 0;
	u8 offset;
	u8 elvss_max_value;

	ref = (lcd->elvss.reference | 0x80);

	if (lcd->elvss.limit == 0x00)
		elvss_max_value = ELVSS_MAX_VALUE_MODE0;
	else if (lcd->elvss.limit == 0x01)
		elvss_max_value = ELVSS_MAX_VALUE_MODE1;
	else {
		printk(KERN_ERR "[ERROR:ELVSS]:%s undefined elvss limit value :%x\n", __func__, lcd->elvss.limit);
		return 0;
	}

	offset = get_elvss_offset(elvss_level);
	ref += offset;

	if (ref < ELVSS_MIN_VALUE)
		ref = ELVSS_MIN_VALUE;
	else if (ref > elvss_max_value)
		ref = elvss_max_value;

	return ref;
}

static int init_elvss_table(struct lcd_info *lcd)
{
	int i, ret = 0;

	lcd->elvss_table = kzalloc(ELVSS_STATUS_MAX * sizeof(u8 *), GFP_KERNEL);

	if (IS_ERR_OR_NULL(lcd->elvss_table)) {
		pr_err("failed to allocate elvss table\n");
		ret = -ENOMEM;
		goto err_alloc_elvss_table;
	}

	for (i = 0; i < ELVSS_STATUS_MAX; i++) {
		lcd->elvss_table[i] = kzalloc(3 * sizeof(u8), GFP_KERNEL);
		if (IS_ERR_OR_NULL(lcd->elvss_table[i])) {
			pr_err("failed to allocate elvss\n");
			ret = -ENOMEM;
			goto err_alloc_elvss;
		}
		lcd->elvss_table[i][0] = 0xB1;
		lcd->elvss_table[i][1] = 0x84;
		lcd->elvss_table[i][2] = get_elvss_value(lcd, i);
	}

	lcd->elvss_cmd_length = 3;

	return 0;

err_alloc_elvss:
	while (i > 0) {
		kfree(lcd->elvss_table[i-1]);
		i--;
	}
	kfree(lcd->elvss_table);
err_alloc_elvss_table:
	return ret;
}

static int init_gamma_table(struct lcd_info *lcd)
{
	int i, ret = 0;

	lcd->gamma_table = kzalloc(GAMMA_MAX * sizeof(u8 *), GFP_KERNEL);
	if (IS_ERR_OR_NULL(lcd->gamma_table)) {
		pr_err("failed to allocate gamma table\n");
		ret = -ENOMEM;
		goto err_alloc_gamma_table;
	}

	for (i = 0; i < GAMMA_MAX; i++) {
		lcd->gamma_table[i] = kzalloc(GAMMA_PARAM_SIZE * sizeof(u8), GFP_KERNEL);
		if (IS_ERR_OR_NULL(lcd->gamma_table[i])) {
			pr_err("failed to allocate gamma\n");
			ret = -ENOMEM;
			goto err_alloc_gamma;
		}
		lcd->gamma_table[i][0] = 0xFA;
		calc_gamma_table(&lcd->smart, candela_table[i]-1, lcd->gamma_table[i]+1);
	}
#if 0
	for (i = 0; i < GAMMA_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			printk("0x%02x, ", lcd->gamma_table[i][j]);
		printk("\n");
	}
#endif
	return 0;

err_alloc_gamma:
	while (i > 0) {
		kfree(lcd->gamma_table[i-1]);
		i--;
	}
	kfree(lcd->gamma_table);
err_alloc_gamma_table:
	return ret;
}
#endif

static int update_brightness(struct lcd_info *lcd, u8 force)
{
	int ret;
	u32 brightness;

	mutex_lock(&lcd->bl_lock);

	brightness = lcd->bd->props.brightness;

	lcd->bl = (brightness - candela_table[0]) / 10;

	lcd->bl = (lcd->bl >= ARRAY_SIZE(candela_table)) ? 0 : lcd->bl;

	if ((force) || ((lcd->ldi_enable) && (lcd->current_bl != lcd->bl))) {

		ret = s6e8ax0_gamma_ctl(lcd);

		ret = s6e8ax0_set_acl(lcd);

		ret = s6e8ax0_set_elvss(lcd);

		lcd->current_bl = lcd->bl;

		dev_info(&lcd->ld->dev, "brightness=%d, bl=%d\n", brightness, lcd->bl);
	}

	mutex_unlock(&lcd->bl_lock);

	return 0;
}

static int s6e8ax0_ldi_init(struct lcd_info *lcd)
{
	int ret = 0;

	s6e8ax0_write(lcd, SEQ_APPLY_LEVEL_2_KEY, ARRAY_SIZE(SEQ_APPLY_LEVEL_2_KEY));
	s6e8ax0_write(lcd, SEQ_POWER_CONTROL, ARRAY_SIZE(SEQ_POWER_CONTROL));
	s6e8ax0_write(lcd, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	msleep(22);	/* 1 frame + 5ms */

	s6e8ax0_write(lcd, SEQ_APPLY_MTP_KEY, ARRAY_SIZE(SEQ_APPLY_MTP_KEY));
	s6e8ax0_write(lcd, SEQ_PANEL_CONDITION_SET, ARRAY_SIZE(SEQ_PANEL_CONDITION_SET));
	s6e8ax0_write(lcd, SEQ_GAMMA_SELECT, ARRAY_SIZE(SEQ_GAMMA_SELECT));
	s6e8ax0_write(lcd, lcd->gamma_table[GAMMA_30CD], GAMMA_PARAM_SIZE);
	s6e8ax0_write(lcd, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));
	s6e8ax0_write(lcd, SEQ_ETC_PWRCTL, ARRAY_SIZE(SEQ_ETC_PWRCTL));
	s6e8ax0_write(lcd, SEQ_ETC_SOURCE_CONTROL, ARRAY_SIZE(SEQ_ETC_SOURCE_CONTROL));
	s6e8ax0_write(lcd, SEQ_ELVSS_DEFAULT, ARRAY_SIZE(SEQ_ELVSS_DEFAULT));
	s6e8ax0_write(lcd, SEQ_ETC_NVM_SETTING, ARRAY_SIZE(SEQ_ETC_NVM_SETTING));
	s6e8ax0_write(lcd, SEQ_ELVSS_ON, ARRAY_SIZE(SEQ_ELVSS_ON));
	s6e8ax0_write(lcd, SEQ_ELVSS_44, ARRAY_SIZE(SEQ_ELVSS_44));

	s6e8ax0_write(lcd, SEQ_ACL_CUTOFF_40, ACL_PARAM_SIZE);

	s6e8ax0_write(lcd, SEQ_VREGOUT_SET, ARRAY_SIZE(SEQ_VREGOUT_SET));

	return ret;
}

static int s6e8ax0_ldi_enable(struct lcd_info *lcd)
{
	int ret = 0;

	s6e8ax0_write(lcd, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));

	return ret;
}

static int s6e8ax0_ldi_disable(struct lcd_info *lcd)
{
	int ret = 0;

	s6e8ax0_write(lcd, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	s6e8ax0_write(lcd, SEQ_STANDBY_ON, ARRAY_SIZE(SEQ_STANDBY_ON));

	return ret;
}

static int s6e8ax0_power_on(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	/* dev_info(&lcd->ld->dev, "%s\n", __func__); */

	ret = s6e8ax0_ldi_init(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}

	msleep(120);

	ret = s6e8ax0_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}

	lcd->ldi_enable = 1;

	update_brightness(lcd, 1);
err:
	return ret;
}

static int s6e8ax0_power_off(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->ldi_enable = 0;

	ret = s6e8ax0_ldi_disable(lcd);

	msleep(120);

	return ret;
}

static int s6e8ax0_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = s6e8ax0_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = s6e8ax0_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int s6e8ax0_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return s6e8ax0_power(lcd, power);
}

static int s6e8ax0_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

static int s6e8ax0_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	int brightness = bd->props.brightness;
	struct lcd_info *lcd = bl_get_data(bd);

	/* dev_info(&lcd->ld->dev, "%s: brightness=%d\n", __func__, brightness); */

	if (brightness < MIN_BRIGHTNESS ||
		brightness > bd->props.max_brightness) {
		dev_err(&bd->dev, "lcd brightness should be %d to %d. now %d\n",
			MIN_BRIGHTNESS, MAX_BRIGHTNESS, brightness);
		return -EINVAL;
	}

	if (lcd->ldi_enable) {
		ret = update_brightness(lcd, 0);
		if (ret < 0) {
			dev_err(lcd->dev, "err in %s\n", __func__);
			return -EINVAL;
		}
	}

	return ret;
}

static int s6e8ax0_get_brightness(struct backlight_device *bd)
{
	struct lcd_info *lcd = bl_get_data(bd);

	return candela_table[lcd->bl];
}

static struct lcd_ops s6e8ax0_lcd_ops = {
	.set_power = s6e8ax0_set_power,
	.get_power = s6e8ax0_get_power,
};

static const struct backlight_ops s6e8ax0_backlight_ops  = {
	.get_brightness = s6e8ax0_get_brightness,
	.update_status = s6e8ax0_set_brightness,
};

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[15];
	sprintf(temp, "SMD_AMS767KC01\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0444, lcd_type_show, NULL);

static ssize_t power_reduce_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->acl_enable);
	strcpy(buf, temp);

	return strlen(buf);
}

static ssize_t power_reduce_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = strict_strtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (lcd->acl_enable != value) {
			dev_info(dev, "%s - %d, %d\n", __func__, lcd->acl_enable, value);
			lcd->acl_enable = value;
			if (lcd->ldi_enable)
				s6e8ax0_set_acl(lcd);
		}
	}
	return size;
}

static DEVICE_ATTR(power_reduce, 0664, power_reduce_show, power_reduce_store);

static ssize_t gamma_table_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int i, j;

	for (i = 0; i < GAMMA_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			printk("0x%02x, ", lcd->gamma_table[i][j]);
		printk("\n");
	}

	return strlen(buf);
}
static DEVICE_ATTR(gamma_table, 0444, gamma_table_show, NULL);

#ifdef CONFIG_HAS_EARLYSUSPEND
struct lcd_info *g_lcd;

void s6e8ax0_early_suspend(void)
{
	struct lcd_info *lcd = g_lcd;

	set_dsim_lcd_enabled(0);

	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

void s6e8ax0_late_resume(void)
{
	struct lcd_info *lcd = g_lcd;

	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	s6e8ax0_power(lcd, FB_BLANK_UNBLANK);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	set_dsim_lcd_enabled(1);

	return ;
}
#endif

static void s6e8ax0_read_id(struct lcd_info *lcd, u8 *buf)
{
	int ret = 0;

	ret = s6e8ax0_read(lcd, LDI_ID_REG, LDI_ID_LEN, buf, 3);
	if (!ret) {
		/* To protect ELVSS Wrong Operation */
		buf[0] = 0xA2;
		buf[1] = 0x15;
		buf[2] = 0x44;
		lcd->connected = 0;
		dev_info(&lcd->ld->dev, "panel is not connected well\n");
	}

	SEQ_VREGOUT_SET[1] = buf[0];
	SEQ_VREGOUT_SET[2] = buf[1];
	SEQ_VREGOUT_SET[3] = buf[2];
}

#ifdef SMART_DIMMING
static int s6e8ax0_read_mtp(struct lcd_info *lcd, u8 *mtp_data)
{
	int ret;

	ret = s6e8ax0_read(lcd, LDI_MTP_ADDR, LDI_MTP_LENGTH, mtp_data, 0);

	return ret;
}

static void s6e8ab0_check_id(struct lcd_info *lcd, u8 *idbuf)
{
	u32 i;

	if ((idbuf[0] == 0xa2) && (idbuf[2] != 0x44))
		lcd->support_elvss = 1;

	if (lcd->support_elvss) {
		lcd->elvss.limit = (idbuf[2] & 0xc0) >> 6;
		lcd->elvss.reference = idbuf[2] & 0x3f;
		lcd->elvss_cmd_length = 3;
		printk(KERN_INFO "ID-3 is 0x%x support dynamic elvss\n", idbuf[2]);
		printk(KERN_INFO "Dynamic ELVSS Information\n");
		printk(KERN_INFO "limit    : %02x\n", lcd->elvss.limit);
	}

	for (i = 0; i < LDI_ID_LEN; i++)
		lcd->smart.panelid[i] = idbuf[i];
}
#endif

static int s6e8ax0_probe(struct device *dev)
{
	int ret = 0;
	struct lcd_info *lcd;
#ifdef SMART_DIMMING
	u8 mtp_data[LDI_MTP_LENGTH] = {0,};
#endif

	lcd = kzalloc(sizeof(struct lcd_info), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	g_lcd = lcd;

	lcd->ld = lcd_device_register("panel", dev, lcd, &s6e8ax0_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("panel", dev, lcd, &s6e8ax0_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		pr_err("failed to register backlight device\n");
		ret = PTR_ERR(lcd->bd);
		goto out_free_backlight;
	}

	lcd->dev = dev;
	lcd->dsim = (struct dsim_global *)dev_get_drvdata(dev->parent);
	lcd->bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd->bd->props.brightness = DEFAULT_BRIGHTNESS;
	lcd->bl = DEFAULT_GAMMA_LEVEL;
	lcd->current_bl = lcd->bl;

	lcd->acl_enable = 0;
	lcd->cur_acl = 0;

	lcd->power = FB_BLANK_UNBLANK;
	lcd->ldi_enable = 1;
	lcd->connected = 1;

	ret = device_create_file(&lcd->ld->dev, &dev_attr_power_reduce);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_gamma_table);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	dev_set_drvdata(dev, lcd);

	mutex_init(&lcd->lock);
	mutex_init(&lcd->bl_lock);

	s6e8ax0_read_id(lcd, lcd->id);

	dev_info(&lcd->ld->dev, "ID: %x, %x, %x\n", lcd->id[0], lcd->id[1], lcd->id[2]);

	dev_info(&lcd->ld->dev, "s6e8ab0 lcd panel driver has been probed.\n");

	lcd->gamma_table = (unsigned char **)gamma22_table_sm2;
	lcd->elvss_table = (unsigned char **)ELVSS_TABLE;
	lcd->elvss_cmd_length = ELVSS_PARAM_SIZE;

#ifdef SMART_DIMMING
	s6e8ab0_check_id(lcd, lcd->id);

	init_table_info(&lcd->smart);

	ret = s6e8ax0_read_mtp(lcd, mtp_data);
	if (!ret) {
		printk(KERN_ERR "[LCD:ERROR] : %s read mtp failed\n", __func__);
		/*return -EPERM;*/
	}

	calc_voltage_table(&lcd->smart, mtp_data);

	if (lcd->connected) {
		ret = init_gamma_table(lcd);
		if (lcd->support_elvss)
			ret += init_elvss_table(lcd);

		if (ret) {
			lcd->gamma_table = (unsigned char **)gamma22_table_sm2;
			lcd->elvss_table = (unsigned char **)ELVSS_TABLE;
			lcd->elvss_cmd_length = ELVSS_PARAM_SIZE;
		}
	}

	update_brightness(lcd, 1);
#endif

	return 0;

out_free_backlight:
	lcd_device_unregister(lcd->ld);
	kfree(lcd);
	return ret;

out_free_lcd:
	kfree(lcd);
	return ret;

err_alloc:
	return ret;
}

static int __devexit s6e8ax0_remove(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	backlight_device_unregister(lcd->bd);
	kfree(lcd);

	return 0;
}

/* Power down all displays on reboot, poweroff or halt. */
static void s6e8ax0_shutdown(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);
}

static struct mipi_lcd_driver s6e8ax0_mipi_driver = {
	.name = "s6e8ab0",
	.probe			= s6e8ax0_probe,
	.remove			= __devexit_p(s6e8ax0_remove),
	.shutdown		= s6e8ax0_shutdown,
};

static int s6e8ax0_init(void)
{
	return s5p_dsim_register_lcd_driver(&s6e8ax0_mipi_driver);
}

static void s6e8ax0_exit(void)
{
	return;
}

module_init(s6e8ax0_init);
module_exit(s6e8ax0_exit);

MODULE_DESCRIPTION("MIPI-DSI S6E8AB0:AMS767KC01 (1280x800) Panel Driver");
MODULE_LICENSE("GPL");
