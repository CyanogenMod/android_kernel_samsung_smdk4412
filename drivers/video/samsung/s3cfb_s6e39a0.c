/* linux/drivers/video/samsung/s3cfb_s6e8aa0.c
 *
 * MIPI-DSI based AMS529HA01 AMOLED lcd panel driver.
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

#include "s5p-dsim.h"
#include "s3cfb.h"

#ifdef SMART_DIMMING
#include "smart_dimming.h"
#endif

#define DCS_PARTIAL_MODE_ON	0x12
#define DCS_PARTIAL_AREA	0x30

#define MIN_BRIGHTNESS	0
#define MAX_BRIGHTNESS	255
#define DEFAULT_BRIGHTNESS	150
#define DEFAULT_GAMMA_LEVEL	10

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

#define LDI_ID_REG		0xD1
#define LDI_ID_LEN		3

#ifdef SMART_DIMMING
#define	PANEL_A1_M3			0xA1
#define	PANEL_A2_M3			0xA2
#define	PANEL_A1_SM2			0x12

#define LDI_MTP_LENGTH		24
#define LDI_MTP_ADDR			0xD3

#define ELVSS_OFFSET_MAX		0x00
#define ELVSS_OFFSET_2		0x08
#define ELVSS_OFFSET_1		0x0D
#define ELVSS_OFFSET_MIN		0x11

#define DYNAMIC_ELVSS_MIN_VALUE	0x81
#define DYNAMIC_ELVSS_MAX_VALUE	0x9F

#define ELVSS_MODE0_MIN_VOLTAGE	62
#define ELVSS_MODE1_MIN_VOLTAGE	52

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

	struct device			*dev;
	struct lcd_device		*ld;
	struct backlight_device		*bd;
	struct lcd_platform_data	*lcd_pd;
	struct early_suspend		early_suspend;

	unsigned int			lcd_id;

	const unsigned char		**gamma_table;

#ifdef SMART_DIMMING
	unsigned int			support_elvss;
	unsigned int			aid;
	unsigned int			manual_version;

	struct str_smart_dim		smart;
	struct str_elvss		elvss;
	struct mutex			bl_lock;
#endif
	unsigned int			irq;
	unsigned int			connected;
};

static struct mipi_ddi_platform_data *ddi_pd;

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
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, wbuf[0], 0);
	else if (size == 2)
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, wbuf[0], wbuf[1]);
	else
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int)wbuf, size);

	mutex_unlock(&lcd->lock);

	return 0;
}

static int s6e8ax0_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf)
{
	int ret = 0;

	if (!lcd->connected)
		return ret;

	mutex_lock(&lcd->lock);

	if (ddi_pd->cmd_read)
		ret = ddi_pd->cmd_read(ddi_pd->dsim_base, addr, count, buf);

	mutex_unlock(&lcd->lock);

	return ret;
}

static int s6e8ax0_set_link(void *pd, unsigned int dsim_base,
	unsigned char (*cmd_write) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2),
	int (*cmd_read) (unsigned int reg_base, u8 addr, u16 count, u8 *buf))
{
	struct mipi_ddi_platform_data *temp_pd = NULL;

	temp_pd = (struct mipi_ddi_platform_data *) pd;
	if (temp_pd == NULL) {
		printk(KERN_ERR "mipi_ddi_platform_data is null.\n");
		return -EPERM;
	}

	ddi_pd = temp_pd;

	ddi_pd->dsim_base = dsim_base;

	if (cmd_write)
		ddi_pd->cmd_write = cmd_write;
	else
		printk(KERN_WARNING "cmd_write function is null.\n");

	if (cmd_read)
		ddi_pd->cmd_read = cmd_read;
	else
		printk(KERN_WARNING "cmd_read function is null.\n");

	return 0;
}

static int get_backlight_level_from_brightness(int brightness)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is only supported from 0 to 24 */

	switch (brightness) {
	case 0:
		backlightlevel = 0;
		break;
	case 1 ... 29:
		backlightlevel = 0;
		break;
	case 30 ... 34:
		backlightlevel = 1;
		break;
	case 35 ... 44:
		backlightlevel = 2;
		break;
	case 45 ... 54:
		backlightlevel = 3;
		break;
	case 55 ... 64:
		backlightlevel = 4;
		break;
	case 65 ... 74:
		backlightlevel = 5;
		break;
	case 75 ... 83:
		backlightlevel = 6;
		break;
	case 84 ... 93:
		backlightlevel = 7;
		break;
	case 94 ... 103:
		backlightlevel = 8;
		break;
	case 104 ... 113:
		backlightlevel = 9;
		break;
	case 114 ... 122:
		backlightlevel = 10;
		break;
	case 123 ... 132:
		backlightlevel = 11;
		break;
	case 133 ... 142:
		backlightlevel = 12;
		break;
	case 143 ... 152:
		backlightlevel = 13;
		break;
	case 153 ... 162:
		backlightlevel = 14;
		break;
	case 163 ... 171:
		backlightlevel = 15;
		break;
	case 172 ... 181:
		backlightlevel = 16;
		break;
	case 182 ... 191:
		backlightlevel = 17;
		break;
	case 192 ... 201:
		backlightlevel = 18;
		break;
	case 202 ... 210:
		backlightlevel = 19;
		break;
	case 211 ... 220:
		backlightlevel = 20;
		break;
	case 221 ... 230:
		backlightlevel = 21;
		break;
	case 231 ... 240:
		backlightlevel = 22;
		break;
	case 241 ... 250:
		backlightlevel = 23;
		break;
	case 251 ... 255:
		backlightlevel = 24;
		break;
	default:
		backlightlevel = 10;
		break;
	}
	return backlightlevel;
}

static int s6e8ax0_set_acl(struct lcd_info *lcd)
{
	if (lcd->acl_enable) {
		if (lcd->cur_acl == 0) {
			if (lcd->bl == 0 || lcd->bl == 1) {
				s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			} else
				s6e8ax0_write(lcd, SEQ_ACL_ON, ARRAY_SIZE(SEQ_ACL_ON));
		}
		switch (lcd->bl) {
		case 0 ... 1: /* 30cd ~ 40cd - 0%*/
			if (lcd->cur_acl != 0) {
				s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				lcd->cur_acl = 0;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 2 ... 12: /* 70cd ~ 180cd -40%*/
			if (lcd->cur_acl != 40) {
				s6e8ax0_write(lcd, acl_cutoff_table[1], ACL_PARAM_SIZE);
				lcd->cur_acl = 40;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 13: /* 190cd - 43% */
			if (lcd->cur_acl != 43) {
				s6e8ax0_write(lcd, acl_cutoff_table[2], ACL_PARAM_SIZE);
				lcd->cur_acl = 43;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 14: /* 200cd - 45% */
			if (lcd->cur_acl != 45) {
				s6e8ax0_write(lcd, acl_cutoff_table[3], ACL_PARAM_SIZE);
				lcd->cur_acl = 45;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 15: /* 210cd - 47% */
			if (lcd->cur_acl != 47) {
				s6e8ax0_write(lcd, acl_cutoff_table[4], ACL_PARAM_SIZE);
				lcd->cur_acl = 47;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 16: /* 220cd - 48% */
			if (lcd->cur_acl != 48) {
				s6e8ax0_write(lcd, acl_cutoff_table[5], ACL_PARAM_SIZE);
				lcd->cur_acl = 48;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		default:
			if (lcd->cur_acl != 50) {
				s6e8ax0_write(lcd, acl_cutoff_table[6], ACL_PARAM_SIZE);
				lcd->cur_acl = 50;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		}
	} else {
		s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
		lcd->cur_acl = 0;
		dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
	}

	return 0;
}

#ifndef SMART_DIMMING
static int s6e8ax0_gamma_ctl(struct lcd_info *lcd)
{
	s6e8ax0_write(lcd, gamma22_table[lcd->bl], GAMMA_PARAM_SIZE);

	/* Gamma Set Update */
	s6e8ax0_write(lcd, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));

	lcd->current_bl = lcd->bl;

	return 0;
}

static int update_brightness(struct lcd_info *lcd)
{
	int ret;

#if defined(CONFIG_MACH_MIDAS_01_BD) || defined(CONFIG_MACH_MIDAS_02_BD) || \
	defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_SLP_PQ) || \
	defined(CONFIG_MACH_SLP_PQ_LTE) || defined(CONFIG_MACH_M3)
#else
	ret = s6e8ax0_gamma_ctl(lcd);
	if (ret)
		return -EPERM;

	/*
	ret = s6e8ax0_set_elvss(lcd);
	if (ret)
		return -EPERM;

	ret = s6e8ax0_set_acl(lcd);
	if (ret)
		return -EPERM;
		*/
#endif

	return 0;
}
#endif

#ifdef SMART_DIMMING
static int s6e8ax0_read_mtp(struct lcd_info *lcd, u8 *mtp_data)
{
	int ret;
	u8 retry_cnt = 3;

read_retry:
	ret = s6e8ax0_read(lcd, LDI_MTP_ADDR, LDI_MTP_LENGTH, mtp_data);
	if (!ret) {
		if (retry_cnt) {
			printk(KERN_WARNING "[WARN:LCD] : %s : retry cnt : %d\n", __func__, retry_cnt);
			retry_cnt--;
			goto read_retry;
		} else
			printk(KERN_ERR "ERROR:MTP read failed\n");
	}

	return ret;
}

static u32 transform_gamma(u32 brightness)
{
	u32 gamma;

	switch (brightness) {
	case 0:
		gamma = 30;
		break;
	case 1 ... 29:
		gamma = 30;
		break;
	case 30 ... 34:
		gamma = 40;
		break;
	case 35 ... 44:
		gamma = 70;
		break;
	case 45 ... 54:
		gamma = 80;
		break;
	case 55 ... 64:
		gamma = 90;
		break;
	case 65 ... 74:
		gamma = 100;
		break;
	case 75 ... 83:
		gamma = 110;
		break;
	case 84 ... 93:
		gamma = 120;
		break;
	case 94 ... 103:
		gamma = 130;
		break;
	case 104 ... 113:
		gamma = 140;
		break;
	case 114 ... 122:
		gamma = 150;
		break;
	case 123 ... 132:
		gamma = 160;
		break;
	case 133 ... 142:
		gamma = 170;
		break;
	case 143 ... 152:
		gamma = 180;
		break;
	case 153 ... 162:
		gamma = 190;
		break;
	case 163 ... 171:
		gamma = 200;
		break;
	case 172 ... 181:
		gamma = 210;
		break;
	case 182 ... 191:
		gamma = 220;
		break;
	case 192 ... 201:
		gamma = 230;
		break;
	case 202 ... 210:
		gamma = 240;
		break;
	case 211 ... 220:
		gamma = 250;
		break;
	case 221 ... 230:
		gamma = 260;
		break;
	case 231 ... 240:
		gamma = 270;
		break;
	case 241 ... 250:
		gamma = 280;
		break;
	case 251 ... 255:
		gamma = 290;
		break;
	default:
		gamma = 150;
		break;
	}
	return gamma - 1;
}

static int s6e8aa0_update_brightness(struct lcd_info *lcd, u32 brightness)
{
	int ret = 0;
	u8 gamma_regs[GAMMA_PARAM_SIZE] = {0,};
	u32 gamma;
#if 0
	int i;
#endif

	gamma_regs[0] = 0xFA;
	gamma_regs[1] = 0x01;

	gamma = brightness;

	calc_gamma_table(&lcd->smart, gamma, gamma_regs+2);

	s6e8ax0_write(lcd, gamma_regs, GAMMA_PARAM_SIZE);

	s6e8ax0_write(lcd, SEQ_GAMMA_UPDATE, sizeof(SEQ_GAMMA_UPDATE));

#if 0
	printk(KERN_INFO "##### print gamma reg #####\n");
	for (i = 0; i < 26; i++)
		printk(KERN_INFO "[%02d] : %02x\n", i, gamma_regs[i]);
#endif

	return ret;
}

static u8 get_offset_brightness(u32 candela)
{
	u8 offset = 0;

	switch (candela) {
	case 0 ... 100:
		offset = ELVSS_OFFSET_MIN;
		break;
	case 101 ... 160:
		offset = ELVSS_OFFSET_1;
		break;
	case 161 ... 200:
		offset = ELVSS_OFFSET_2;
		break;
	case 201 ...  300:
		offset = ELVSS_OFFSET_MAX;
		break;
	default:
		offset = ELVSS_OFFSET_MAX;
		break;
	}
	return offset;
}

static u8 get_elvss_value(struct lcd_info *lcd, u32 candela)
{
	u8 ref = 0;
	u8 offset;

	ref = (lcd->elvss.reference | 0x80);
	offset = get_offset_brightness(candela);
	ref += offset;

	printk(KERN_DEBUG "%s ref =0x%x , offset = 0x%x\n", __func__, ref, offset);

	if (ref < DYNAMIC_ELVSS_MIN_VALUE)
		ref = DYNAMIC_ELVSS_MIN_VALUE;
	else if (ref > DYNAMIC_ELVSS_MAX_VALUE)
		ref = DYNAMIC_ELVSS_MAX_VALUE;

	return ref;
}

static int s6e8aa0_update_elvss(struct lcd_info *lcd, u32 candela)
{
	u8 elvss_cmd[3];
	u8 elvss;

	elvss = get_elvss_value(lcd, candela);
	if (!elvss) {
		printk(KERN_ERR "[ERROR:LCD]:%s:get_elvss_value() failed\n", __func__);
		return -EPERM;
	}

	elvss_cmd[0] = 0xb1;
	elvss_cmd[1] = 0x04;
	elvss_cmd[2] = elvss;

	printk(KERN_DEBUG "elvss reg : %02x\n", elvss_cmd[2]);
	s6e8ax0_write(lcd, elvss_cmd, sizeof(elvss_cmd));

	return 0;
}

static int s6e8ax0_adb_brightness_update(struct lcd_info *lcd, u32 br, u32 force)
{
	u32 gamma;
	int ret = 0;

	mutex_lock(&lcd->bl_lock);

	lcd->bl = get_backlight_level_from_brightness(br);

	if ((force) || ((lcd->ldi_enable) && (lcd->current_bl != lcd->bl))) {
		gamma = transform_gamma(br);

		ret = s6e8aa0_update_brightness(lcd, gamma);

		ret = s6e8ax0_set_acl(lcd);

		if (lcd->support_elvss)
			ret = s6e8aa0_update_elvss(lcd, gamma);

		lcd->current_bl = lcd->bl;
		dev_info(&lcd->ld->dev, "brightness=%d, gamma=%d\n", br, gamma);
	}

	mutex_unlock(&lcd->bl_lock);

	return ret;
}
#endif

static int s6e8ax0_ldi_init(struct lcd_info *lcd)
{
	int ret = 0;
	s6e8ax0_write(lcd, SEQ_APPLY_LEVEL_2_KEY, \
		ARRAY_SIZE(SEQ_APPLY_LEVEL_2_KEY));
	s6e8ax0_write(lcd, SEQ_APPLY_LEVEL_3_KEY, \
		ARRAY_SIZE(SEQ_APPLY_LEVEL_3_KEY));
	s6e8ax0_write(lcd, SEQ_APPLY_LEVEL_4_KEY, \
		ARRAY_SIZE(SEQ_APPLY_LEVEL_4_KEY));
	s6e8ax0_write(lcd, SEQ_GAMMA_CONDITION_SET, \
		ARRAY_SIZE(SEQ_GAMMA_CONDITION_SET));
	s6e8ax0_write(lcd, SEQ_GAMMA_UPDATE,\
		ARRAY_SIZE(SEQ_GAMMA_UPDATE));
	s6e8ax0_write(lcd, SEQ_PANEL_CONDITION_SET, \
		ARRAY_SIZE(SEQ_PANEL_CONDITION_SET));
	s6e8ax0_write(lcd, SEQ_ETC_CONDITION_SET_2_0, \
		ARRAY_SIZE(SEQ_ETC_CONDITION_SET_2_0));
	s6e8ax0_write(lcd, SEQ_ETC_CONDITION_SET_2_1, \
		ARRAY_SIZE(SEQ_ETC_CONDITION_SET_2_1));
	s6e8ax0_write(lcd, SEQ_ETC_CONDITION_SET_2_2, \
		ARRAY_SIZE(SEQ_ETC_CONDITION_SET_2_2));
	s6e8ax0_write(lcd, SEQ_ETC_CONDITION_SET_2_3, \
		ARRAY_SIZE(SEQ_ETC_CONDITION_SET_2_3));
	s6e8ax0_write(lcd, SEQ_ETC_CONDITION_SET_2_4, \
		ARRAY_SIZE(SEQ_ETC_CONDITION_SET_2_4));
	s6e8ax0_write(lcd, SEQ_ETC_CONDITION_SET_2_5, \
		ARRAY_SIZE(SEQ_ETC_CONDITION_SET_2_5));
	s6e8ax0_write(lcd, SEQ_ETC_CONDITION_SET_2_6, \
		ARRAY_SIZE(SEQ_ETC_CONDITION_SET_2_6));
	s6e8ax0_write(lcd, SEQ_ETC_CONDITION_SET_2_7, \
		ARRAY_SIZE(SEQ_ETC_CONDITION_SET_2_7));
	s6e8ax0_write(lcd, SEQ_ETC_CONDITION_SET_2_8, \
		ARRAY_SIZE(SEQ_ETC_CONDITION_SET_2_8));
	s6e8ax0_write(lcd, SEQ_ETC_CONDITION_SET_2_9, \
		ARRAY_SIZE(SEQ_ETC_CONDITION_SET_2_9));
	s6e8ax0_write(lcd, SEQ_ETC_CONDITION_SET_2_10, \
		ARRAY_SIZE(SEQ_ETC_CONDITION_SET_2_10));
	s6e8ax0_write(lcd, SEQ_SLEEP_OUT, \
		ARRAY_SIZE(SEQ_SLEEP_OUT));
	mdelay(120);
	s6e8ax0_write(lcd, SEQ_MEMORY_WINDOW_SET_1_0, \
		ARRAY_SIZE(SEQ_MEMORY_WINDOW_SET_1_0));
	s6e8ax0_write(lcd, SEQ_MEMORY_WINDOW_SET_1_1, \
		ARRAY_SIZE(SEQ_MEMORY_WINDOW_SET_1_1));
	mdelay(1);
	s6e8ax0_write(lcd, SEQ_MEMORY_WINDOW_SET_2_1, \
		ARRAY_SIZE(SEQ_MEMORY_WINDOW_SET_2_1));
	s6e8ax0_write(lcd, SEQ_MEMORY_WINDOW_SET_2_2, \
		ARRAY_SIZE(SEQ_MEMORY_WINDOW_SET_2_2));
	s6e8ax0_write(lcd, SEQ_MEMORY_WINDOW_SET_2_3, \
		ARRAY_SIZE(SEQ_MEMORY_WINDOW_SET_2_3));
	s6e8ax0_write(lcd, SEQ_MEMORY_WINDOW_SET_2_4, \
		ARRAY_SIZE(SEQ_MEMORY_WINDOW_SET_2_4));
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

#ifdef SMART_DIMMING
	lcd->ldi_enable = 1;

	s6e8ax0_adb_brightness_update(lcd, lcd->bd->props.brightness, 1);
#else
	update_brightness(lcd);

	lcd->ldi_enable = 1;
#endif

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

#ifdef SMART_DIMMING
static int s6e8ax0_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	u32 brightness = (u32)bd->props.brightness;

	struct lcd_info *lcd = bl_get_data(bd);

	if (brightness < MIN_BRIGHTNESS ||
		brightness > bd->props.max_brightness) {
		dev_err(&bd->dev, "lcd brightness should be %d to %d. now %d\n",
		MIN_BRIGHTNESS, MAX_BRIGHTNESS, brightness);
		return -EINVAL;
	}

	/* dev_info(&lcd->ld->dev, "[%s] brightness=%d\n", __func__, brightness); */

	ret = s6e8ax0_adb_brightness_update(lcd, brightness, 0);

	return ret;
}
#else
static int s6e8ax0_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	int brightness = bd->props.brightness;
	struct lcd_info *lcd = bl_get_data(bd);

	/* dev_info(&lcd->ld->dev, "[%s] brightness=%d\n", __func__, brightness); */

	if (brightness < MIN_BRIGHTNESS ||
		brightness > bd->props.max_brightness) {
		dev_err(&bd->dev, "lcd brightness should be %d to %d. now %d\n",
			MIN_BRIGHTNESS, MAX_BRIGHTNESS, brightness);
		return -EINVAL;
	}

	lcd->bl = get_backlight_level_from_brightness(brightness);

	if ((lcd->ldi_enable) && (lcd->current_bl != lcd->bl)) {
		ret = update_brightness(lcd);
		dev_info(&lcd->ld->dev, "brightness=%d, bl=%d\n",\
			bd->props.brightness, lcd->bl);
		if (ret < 0) {
			dev_err(lcd->dev, "err in %s\n", __func__);
			return -EINVAL;
		}
	}

	return ret;
}
#endif

static int s6e8ax0_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
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
	sprintf(temp, "SMD_AMS465GS0x\n");
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

	if (lcd->acl_enable != value) {
		dev_info(dev, "%s - %d, %d\n", __func__, \
			lcd->acl_enable, value);
		lcd->acl_enable = value;
		if (lcd->ldi_enable)
			s6e8ax0_set_acl(lcd);
	}
	return size;
}

static DEVICE_ATTR(power_reduce, 0664, power_reduce_show, power_reduce_store);

#ifdef CONFIG_HAS_EARLYSUSPEND
struct lcd_info *g_lcd;

void s6e8ax0_early_suspend(void)
{
	struct lcd_info *lcd = g_lcd;

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

	return ;
}
#endif

static void s6e8ax0_read_id(struct lcd_info *lcd, u8 *buf)
{
	int ret = 0;
	u8 retry_cnt = 3;

read_retry:
	ret = s6e8ax0_read(lcd, LDI_ID_REG, LDI_ID_LEN, buf);
	if (!ret) {
		if (retry_cnt) {
			printk(KERN_WARNING "[WARN:LCD] : %s : retry cnt : %d\n",\
				__func__, retry_cnt);
			retry_cnt--;
			goto read_retry;
		} else {
			printk(KERN_ERR "[ERROR:LCD] : %s : Read ID Failed\n", __func__);
			lcd->connected = 0;
			dev_info(&lcd->ld->dev, "panel is not connected well\n");
		}
	}
}

#ifdef SMART_DIMMING
static void s6e8aa0_check_id(struct lcd_info *lcd, u8 *idbuf)
{
	int i;

	for (i = 0; i < LDI_ID_LEN; i++)
		lcd->smart.panelid[i] = idbuf[i];

	lcd->aid = lcd->smart.panelid[2] & 0xe0 >> 5;

	if (idbuf[0] == PANEL_A1_SM2) {
		lcd->support_elvss = 1;
		lcd->aid = lcd->smart.panelid[2] & 0xe0 >> 5;
		lcd->elvss.reference = lcd->smart.panelid[2];

		printk(KERN_DEBUG "Dynamic ELVSS Information\n");
		printk(KERN_DEBUG "Refrence : %02x , manual_version = %02x,\
			lcd->aid= %02x\n", lcd->elvss.reference, lcd->manual_version, lcd->aid);
	} else if ((idbuf[0] == PANEL_A1_M3) || (idbuf[0] == PANEL_A2_M3)) {
		lcd->support_elvss = 0;
		printk(KERN_DEBUG "ID-3 is 0xff does not support dynamic elvss\n");
	} else {
		lcd->support_elvss = 0;
		printk(KERN_DEBUG "No valid panel id\n");
	}
}
#endif

static int s6e8ax0_probe(struct device *dev)
{
	int ret = 0;
	struct lcd_info *lcd;
#ifdef SMART_DIMMING
	u8 mtp_data[LDI_MTP_LENGTH] = {0,};
#endif
	u8 idbuf[LDI_ID_LEN] = {0,};

	lcd = kzalloc(sizeof(struct lcd_info), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	g_lcd = lcd;

	lcd->ld = lcd_device_register("panel", dev, \
		lcd, &s6e8ax0_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("panel", \
		dev, lcd, &s6e8ax0_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		pr_err("failed to register backlight device\n");
		ret = PTR_ERR(lcd->bd);
		goto out_free_backlight;
	}

	lcd->dev = dev;
	lcd->bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd->bd->props.brightness = DEFAULT_BRIGHTNESS;
	lcd->bl = DEFAULT_GAMMA_LEVEL;
	lcd->current_bl = lcd->bl;

	lcd->acl_enable = 0;
	lcd->cur_acl = 0;

	lcd->power = FB_BLANK_UNBLANK;
	lcd->ldi_enable = 1;
	lcd->connected = 1;

	ret = device_create_file(&lcd->ld->dev, \
		&dev_attr_power_reduce);
	if (ret < 0)
		dev_err(&lcd->ld->dev, \
		"failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, \
		"failed to add sysfs entries, %d\n", __LINE__);

	dev_set_drvdata(dev, lcd);

#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd->early_suspend.suspend = s6e8ax0_early_suspend;
	lcd->early_suspend.resume = s6e8ax0_late_resume;
	lcd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 2;
	register_early_suspend(&lcd->early_suspend);
#endif
#endif

	mutex_init(&lcd->lock);

	s6e8ax0_read_id(lcd, idbuf);

	dev_info(&lcd->ld->dev, "ID : %x, %x, %x\n", idbuf[0], idbuf[1], idbuf[2]);

	dev_info(&lcd->ld->dev, \
		"s6e8ax0 lcd panel driver has been probed.\n");

#ifdef SMART_DIMMING
	s6e8aa0_check_id(lcd, idbuf);

	init_table_info(&lcd->smart);

	ret = s6e8ax0_read_mtp(lcd, mtp_data);
	if (!ret) {
		printk(KERN_ERR "[LCD:ERROR] : %s read mtp failed\n", __func__);
		/*return -EPERM;*/
	}

	calc_voltage_table(&lcd->smart, mtp_data);

	mutex_init(&lcd->bl_lock);

	s6e8ax0_adb_brightness_update(lcd, lcd->bd->props.brightness, 1);
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
	.name = "s6e8aa0",
	.set_link		= s6e8ax0_set_link,
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

MODULE_DESCRIPTION("MIPI-DSI S6E8AA0:AMS529HA01 (800x1280) Panel Driver");
MODULE_LICENSE("GPL");
