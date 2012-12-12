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
#include "s6e8aa0_gamma_l.h"

#if defined(CONFIG_S6E8AA0_AMS529HA01)
#include "s6e8aa0_gamma_q1.h"
#define SMART_DIMMING
#else
#include "s6e8aa0_gamma_c1m0.h"
#define SMART_DIMMING
#endif

#ifdef SMART_DIMMING
#include "smart_dimming.h"
#ifdef CONFIG_AID_DIMMING
#include "aid_s6e8aa0.h"
#endif
#endif

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255
#if defined(CONFIG_S6E8AA0_AMS529HA01)
#define MAX_GAMMA			290
#define DEFAULT_BRIGHTNESS		150
#define DEFAULT_GAMMA_LEVEL		GAMMA_150CD
#else
#define MAX_GAMMA			300
#define DEFAULT_BRIGHTNESS		160
#define DEFAULT_GAMMA_LEVEL		GAMMA_160CD
#endif

#define LDI_ID_REG			0xD1
#define LDI_ID_LEN			3

#ifdef SMART_DIMMING
#define	PANEL_A1_M3			0xA1

#define LDI_MTP_LENGTH		24
#define LDI_MTP_ADDR			0xD3

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
	unsigned int			auto_brightness;
	unsigned int			acl_enable;
	unsigned int			cur_acl;
	unsigned int			current_bl;
	unsigned int			current_elvss;

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

#ifdef SMART_DIMMING
	unsigned int			support_elvss;

	struct str_smart_dim		smart;
	struct str_elvss		elvss;
#endif
#ifdef CONFIG_AID_DIMMING
	unsigned int			support_aid;
	unsigned char			f8[GAMMA_MAX][ARRAY_SIZE(SEQ_PANEL_CONDITION_SET_500MBPS)];
#endif
	unsigned int			irq;
	unsigned int			connected;

#if defined(GPIO_OLED_DET)
	struct delayed_work		oled_detection;
	unsigned int			oled_detection_count;
#endif
	struct dsim_global		*dsim;
};

#ifdef CONFIG_AID_DIMMING
static const unsigned int candela_table[GAMMA_MAX] = {
	 20,  30,  40,  50,  60,  70,  80,  90, 100,
	110, 120, 130, 140, 150, 160, 170, 180,
	182, 184, 186, 188,
	190, 200, 210, 220, 230, 240, 250, MAX_GAMMA-1
};

static unsigned int aid_candela_table[GAMMA_MAX] = {
	base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100,
	AOR40_BASE_110, AOR40_BASE_120, AOR40_BASE_130, AOR40_BASE_140, AOR40_BASE_150,
	AOR40_BASE_160, AOR40_BASE_170, AOR40_BASE_180, AOR40_BASE_182, AOR40_BASE_184,
	AOR40_BASE_186, AOR40_BASE_188,
	190, 200, 210, 220, 230, 240, 250, MAX_GAMMA-1
};

static unsigned int elvss_offset_table[ELVSS_STATUS_MAX] = {
	ELVSS_OFFSET_110,
	ELVSS_OFFSET_120,
	ELVSS_OFFSET_130,
	ELVSS_OFFSET_140,
	ELVSS_OFFSET_150,
	ELVSS_OFFSET_160,
	ELVSS_OFFSET_170,
	ELVSS_OFFSET_180,
	ELVSS_OFFSET_190,
	ELVSS_OFFSET_200,
	ELVSS_OFFSET_210,
	ELVSS_OFFSET_220,
	ELVSS_OFFSET_230,
	ELVSS_OFFSET_240,
	ELVSS_OFFSET_250,
	ELVSS_OFFSET_260,
	ELVSS_OFFSET_270,
	ELVSS_OFFSET_280,
	ELVSS_OFFSET_290,
	ELVSS_OFFSET_300
};
#else
static const unsigned int candela_table[GAMMA_MAX] = {
	 30,  40,  50,  60,  70,  80,  90, 100, 110, 120,
	130, 140, 150, 160, 170, 180, 190, 200, 210, 220,
	230, 240, 250, MAX_GAMMA
};

static unsigned int elvss_offset_table[ELVSS_STATUS_MAX] = {
	ELVSS_OFFSET_MIN,
	ELVSS_OFFSET_1,
	ELVSS_OFFSET_2,
	ELVSS_OFFSET_MAX
};
#endif

#if defined(GPIO_OLED_DET)
static void oled_detection_work(struct work_struct *work)
{
	struct lcd_info *lcd =
		container_of(work, struct lcd_info, oled_detection.work);

	int oled_det_level = gpio_get_value(GPIO_OLED_DET);

	dev_info(&lcd->ld->dev, "%s, %d, %d\n", __func__, lcd->oled_detection_count, oled_det_level);

	if (!oled_det_level) {
		if (lcd->oled_detection_count < 10) {
			schedule_delayed_work(&lcd->oled_detection, HZ/8);
			lcd->oled_detection_count++;
			set_dsim_hs_clk_toggle_count(15);
		} else
			set_dsim_hs_clk_toggle_count(0);
	} else
		set_dsim_hs_clk_toggle_count(0);

}

static irqreturn_t oled_detection_int(int irq, void *_lcd)
{
	struct lcd_info *lcd = _lcd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->oled_detection_count = 0;
	schedule_delayed_work(&lcd->oled_detection, HZ/16);

	return IRQ_HANDLED;
}
#endif

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

#ifdef CONFIG_AID_DIMMING
static int get_backlight_level_from_brightness(int brightness)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is only supported from 0 to 24 */

	switch (brightness) {
	case 0 ... 29:
		backlightlevel = GAMMA_20CD;
		break;
	case 30 ... 39:
		backlightlevel = GAMMA_30CD;
		break;
	case 40 ... 49:
		backlightlevel = GAMMA_40CD;
		break;
	case 50 ... 59:
		backlightlevel = GAMMA_50CD;
		break;
	case 60 ... 69:
		backlightlevel = GAMMA_60CD;
		break;
	case 70 ... 79:
		backlightlevel = GAMMA_70CD;
		break;
	case 80 ... 89:
		backlightlevel = GAMMA_80CD;
		break;
	case 90 ... 99:
		backlightlevel = GAMMA_90CD;
		break;
	case 100 ... 109:
		backlightlevel = GAMMA_100CD;
		break;
	case 110 ... 119:
		backlightlevel = GAMMA_110CD;
		break;
	case 120 ... 129:
		backlightlevel = GAMMA_120CD;
		break;
	case 130 ... 139:
		backlightlevel = GAMMA_130CD;
		break;
	case 140 ... 149:
		backlightlevel = GAMMA_140CD;
		break;
	case 150 ... 159:
		backlightlevel = GAMMA_150CD;
		break;
	case 160 ... 169:
		backlightlevel = GAMMA_160CD;
		break;
	case 170 ... 179:
		backlightlevel = GAMMA_170CD;
		break;
	case 180 ... 181:
		backlightlevel = GAMMA_180CD;
		break;
	case 182 ... 183:
		backlightlevel = GAMMA_182CD;
		break;
	case 184 ... 185:
		backlightlevel = GAMMA_184CD;
		break;
	case 186 ... 187:
		backlightlevel = GAMMA_186CD;
		break;
	case 188 ... 189:
		backlightlevel = GAMMA_188CD;
		break;
	case 190 ... 199:
		backlightlevel = GAMMA_190CD;
		break;
	case 200 ... 209:
		backlightlevel = GAMMA_200CD;
		break;
	case 210 ... 219:
		backlightlevel = GAMMA_210CD;
		break;
	case 220 ... 229:
		backlightlevel = GAMMA_220CD;
		break;
	case 230 ... 239:
		backlightlevel = GAMMA_230CD;
		break;
	case 240 ... 249:
		backlightlevel = GAMMA_240CD;
		break;
	case 250 ... 254:
		backlightlevel = GAMMA_250CD;
		break;
	case 255:
		backlightlevel = GAMMA_300CD;
		break;
	default:
		backlightlevel = DEFAULT_GAMMA_LEVEL;
		break;
	}
	return backlightlevel;
}

static int s6e8ax0_aid_parameter_ctl(struct lcd_info *lcd , u8 force)
{
	if (likely(lcd->support_aid)) {
		if ((lcd->f8[lcd->bl][0x12] != lcd->f8[lcd->current_bl][0x12]) ||
			(lcd->f8[lcd->bl][0x01] != lcd->f8[lcd->current_bl][0x01]) || (force))
			s6e8ax0_write(lcd, lcd->f8[lcd->bl], AID_PARAM_SIZE);
	}

	return 0;
}
#else
static int get_backlight_level_from_brightness(int brightness)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is only supported from 0 to 24 */

	switch (brightness) {
	case 0 ... 29:
		backlightlevel = GAMMA_30CD;
		break;
	case 30 ... 254:
		backlightlevel = (brightness - candela_table[0]) / 10;
		break;
	case 255:
		backlightlevel = ARRAY_SIZE(candela_table) - 1;
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
	s6e8ax0_write(lcd, lcd->gamma_table[lcd->bl], GAMMA_PARAM_SIZE);

	/* Gamma Set Update */
	s6e8ax0_write(lcd, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));

	return 0;
}

#if defined(CONFIG_S6E8AA0_AMS529HA01)
static int s6e8ax0_set_acl(struct lcd_info *lcd)
{
	int ret = 0;

	if (lcd->acl_enable) {
		if (lcd->cur_acl == 0) {
			if (lcd->bl == 0 || lcd->bl == 1) {
				s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d, acl_off\n", __func__, lcd->cur_acl);
			} else {
				s6e8ax0_write(lcd, SEQ_ACL_ON, ARRAY_SIZE(SEQ_ACL_ON));
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d, acl_on\n", __func__, lcd->cur_acl);
			}
		}
		switch (lcd->bl) {
		case GAMMA_30CD... GAMMA_40CD:
			if (lcd->cur_acl != 0) {
				s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				lcd->cur_acl = 0;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case GAMMA_50CD:
			if (lcd->cur_acl != 200) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_20P], ACL_PARAM_SIZE);
				lcd->cur_acl = 200;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case GAMMA_60CD:
			if (lcd->cur_acl != 330) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_33P], ACL_PARAM_SIZE);
				lcd->cur_acl = 330;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case GAMMA_70CD:
			if (lcd->cur_acl != 430) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_43P], ACL_PARAM_SIZE);
				lcd->cur_acl = 430;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case GAMMA_80CD:
			if (lcd->cur_acl != 450) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_45P_80CD], ACL_PARAM_SIZE);
				lcd->cur_acl = 450;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case GAMMA_90CD ... GAMMA_150CD:
			if (lcd->cur_acl != 451) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_45P], ACL_PARAM_SIZE);
				lcd->cur_acl = 451;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case GAMMA_160CD: /* 160cd */
			if (lcd->cur_acl != 460) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_46P_160CD], ACL_PARAM_SIZE);
				lcd->cur_acl = 460;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case GAMMA_170CD ... GAMMA_250CD:
			if (lcd->cur_acl != 461) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_46P], ACL_PARAM_SIZE);
				lcd->cur_acl = 461;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		default:
			if (lcd->cur_acl != 550) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_50P], ACL_PARAM_SIZE);
				lcd->cur_acl = 550;
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
#else
static int s6e8ax0_set_acl(struct lcd_info *lcd)
{
	if (lcd->acl_enable) {
		if (lcd->cur_acl == 0) {
			if (lcd->bl == 0) {
				s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d, acl_off\n", __func__, lcd->cur_acl);
			} else {
				s6e8ax0_write(lcd, SEQ_ACL_ON, ARRAY_SIZE(SEQ_ACL_ON));
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d, acl_on\n", __func__, lcd->cur_acl);
			}
		}
		switch (lcd->bl) {
		case 0:
			if (lcd->cur_acl != 0) {
				s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				lcd->cur_acl = 0;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 1:
			if (lcd->cur_acl != 33) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_33P], ACL_PARAM_SIZE);
				lcd->cur_acl = 33;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 2 ... GAMMA_250CD:
			if (lcd->cur_acl != 40) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_40P], ACL_PARAM_SIZE);
				lcd->cur_acl = 40;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		default:
			if (lcd->cur_acl != 50) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_50P], ACL_PARAM_SIZE);
				lcd->cur_acl = 50;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		}
	} else {
		s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
		lcd->cur_acl = 0;
		dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d, acl_off\n", __func__, lcd->cur_acl);
	}

	return 0;
}
#endif

#ifdef SMART_DIMMING
#ifdef CONFIG_AID_DIMMING
static int s6e8ax0_set_elvss(struct lcd_info *lcd, u8 force)
{
	int ret = 0, elvss_level = 0;
	u32 candela = candela_table[lcd->bl];

	switch (candela) {
	case 0 ... 110:
		elvss_level = ELVSS_110;
		break;
	case 111 ... 120:
		elvss_level = ELVSS_120;
		break;
	case 121 ... 130:
		elvss_level = ELVSS_130;
		break;
	case 131 ... 140:
		elvss_level = ELVSS_140;
		break;
	case 141 ... 150:
		elvss_level = ELVSS_150;
		break;
	case 151 ... 160:
		elvss_level = ELVSS_160;
		break;
	case 161 ... 170:
		elvss_level = ELVSS_170;
		break;
	case 171 ... 180:
		elvss_level = ELVSS_180;
		break;
	case 181 ... 190:
		elvss_level = ELVSS_190;
		break;
	case 191 ... 200:
		elvss_level = ELVSS_200;
		break;
	case 201 ... 210:
		elvss_level = ELVSS_210;
		break;
	case 211 ... 220:
		elvss_level = ELVSS_220;
		break;
	case 221 ... 230:
		elvss_level = ELVSS_230;
		break;
	case 231 ... 240:
		elvss_level = ELVSS_240;
		break;
	case 241 ... 250:
		elvss_level = ELVSS_250;
		break;
	case 251 ... 260:
		elvss_level = ELVSS_260;
		break;
	case 261 ... 270:
		elvss_level = ELVSS_270;
		break;
	case 271 ... 280:
		elvss_level = ELVSS_280;
		break;
	case 281 ... 290:
		elvss_level = ELVSS_290;
		break;
	case 291 ... 300:
		elvss_level = ELVSS_300;
		break;
	}

	if ((lcd->current_elvss != lcd->elvss_table[elvss_level][2]) || force) {
		ret = s6e8ax0_write(lcd, lcd->elvss_table[elvss_level], ELVSS_PARAM_SIZE);
		lcd->current_elvss = lcd->elvss_table[elvss_level][2];
	}

	dev_dbg(&lcd->ld->dev, "elvss = %x\n", lcd->elvss_table[elvss_level][2]);

	if (ret) {
		ret = -EPERM;
		goto elvss_err;
	}

elvss_err:
	return ret;
}
#else
static int s6e8ax0_set_elvss(struct lcd_info *lcd, u8 force)
{
	int ret = 0, elvss_level = 0;
	u32 candela = candela_table[lcd->bl];

	switch (candela) {
	case 0 ... 100:
		elvss_level = ELVSS_MIN;
		break;
	case 101 ... 160:
		elvss_level = ELVSS_1;
		break;
	case 161 ... 200:
		elvss_level = ELVSS_2;
		break;
	case 201 ... 300:
		elvss_level = ELVSS_MAX;
		break;
	default:
		break;
	}

	if ((lcd->current_elvss != lcd->elvss_table[elvss_level][2]) || force) {
		ret = s6e8ax0_write(lcd, lcd->elvss_table[elvss_level], ELVSS_PARAM_SIZE);
		lcd->current_elvss = lcd->elvss_table[elvss_level][2];
	}

	dev_dbg(&lcd->ld->dev, "elvss = %x\n", lcd->elvss_table[elvss_level][2]);

	if (ret) {
		ret = -EPERM;
		goto elvss_err;
	}

elvss_err:
	return ret;
}
#endif
static u8 get_elvss_value(struct lcd_info *lcd, u8 elvss_level)
{
	u8 ref = 0;
	u8 offset;

	if (lcd->elvss.limit == 0x00)
		ref = (lcd->elvss.reference | 0x80);
	else if (lcd->elvss.limit == 0x01)
		ref = (lcd->elvss.reference + 0x40);
	else {
		printk(KERN_ERR "[ERROR:ELVSS]:%s undefined elvss limit value :%x\n", __func__, lcd->elvss.limit);
		return 0;
	}

	offset = elvss_offset_table[elvss_level];
	ref += offset;

	if (ref < DYNAMIC_ELVSS_MIN_VALUE)
		ref = DYNAMIC_ELVSS_MIN_VALUE;
	else if (ref > DYNAMIC_ELVSS_MAX_VALUE)
		ref = DYNAMIC_ELVSS_MAX_VALUE;

	return ref;
}

static int init_elvss_table(struct lcd_info *lcd)
{
	int i, ret = 0;
#ifdef SMART_DIMMING_DEBUG
	int j;
#endif

	lcd->elvss_table = kzalloc(ELVSS_STATUS_MAX * sizeof(u8 *), GFP_KERNEL);

	if (IS_ERR_OR_NULL(lcd->elvss_table)) {
		pr_err("failed to allocate elvss table\n");
		ret = -ENOMEM;
		goto err_alloc_elvss_table;
	}

	for (i = 0; i < ELVSS_STATUS_MAX; i++) {
		lcd->elvss_table[i] = kzalloc(ELVSS_PARAM_SIZE * sizeof(u8), GFP_KERNEL);
		if (IS_ERR_OR_NULL(lcd->elvss_table[i])) {
			pr_err("failed to allocate elvss\n");
			ret = -ENOMEM;
			goto err_alloc_elvss;
		}
		lcd->elvss_table[i][0] = 0xB1;
		lcd->elvss_table[i][1] = 0x04;
		lcd->elvss_table[i][2] = get_elvss_value(lcd, i);
	}

#ifdef SMART_DIMMING_DEBUG
	for (i = 0; i < ELVSS_STATUS_MAX; i++) {
		for (j = 0; j < ELVSS_PARAM_SIZE; j++)
			printk("0x%02x, ", lcd->elvss_table[i][j]);
		printk("\n");
	}
#endif

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

#ifdef CONFIG_AID_DIMMING
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
		lcd->gamma_table[i][1] = 0x01;
	}

	for (i = 0; i < GAMMA_MAX; i++) {
		if (candela_table[i] <= 180)
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][2], G_21);
		else if (candela_table[i] > 250)
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][2], G_22);
		else if (candela_table[i] == 190)
			calc_gamma_table_215_190(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][2]);
		else
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][2], G_215);
	}

#ifdef SMART_DIMMING_DEBUG
	for (i = 0; i < GAMMA_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			printk("%d, ", lcd->gamma_table[i][j]);
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

static int init_aid_dimming_table(struct lcd_info *lcd)
{
	unsigned int i, j;

	for (i = 0; i < ARRAY_SIZE(aid_rgb_fix_table); i++) {
		j = (aid_rgb_fix_table[i].gray * 3 + aid_rgb_fix_table[i].rgb) + 2;
		lcd->gamma_table[aid_rgb_fix_table[i].candela_idx][j] += aid_rgb_fix_table[i].offset;
	}

	for (i = 0; i < GAMMA_MAX; i++) {
		memcpy(lcd->f8[i], SEQ_PANEL_CONDITION_SET_500MBPS, AID_PARAM_SIZE);
		lcd->f8[i][0x12] = aid_command_table[i][0];
		lcd->f8[i][0x01] = aid_command_table[i][1];
	}


#ifdef SMART_DIMMING_DEBUG
	for (i = 0; i < GAMMA_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			printk("%d, ", lcd->gamma_table[i][j]);
		printk("\n");
	}
#endif
	return 0;
}
#else
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
		lcd->gamma_table[i][1] = 0x01;
		calc_gamma_table(&lcd->smart, candela_table[i]-1, lcd->gamma_table[i]+2);
	}

#ifdef SMART_DIMMING_DEBUG
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
#endif

static int update_brightness(struct lcd_info *lcd, u8 force)
{
	u32 brightness;

	mutex_lock(&lcd->bl_lock);

	brightness = lcd->bd->props.brightness;

	if (unlikely(!lcd->auto_brightness && brightness > 250))
		brightness = 250;

	lcd->bl = get_backlight_level_from_brightness(brightness);

	if ((force) || ((lcd->ldi_enable) && (lcd->current_bl != lcd->bl))) {

#ifdef CONFIG_AID_DIMMING
		if ((force) || unlikely(aid_candela_table[lcd->bl] != aid_candela_table[lcd->current_bl]))
#endif
			s6e8ax0_gamma_ctl(lcd);

#ifdef CONFIG_AID_DIMMING
		s6e8ax0_aid_parameter_ctl(lcd , force);
#endif
		s6e8ax0_set_acl(lcd);

		s6e8ax0_set_elvss(lcd, force);

		lcd->current_bl = lcd->bl;

		dev_info(&lcd->ld->dev, "brightness=%d, bl=%d, candela=%d\n", brightness, lcd->bl, candela_table[lcd->bl]);
	}

	mutex_unlock(&lcd->bl_lock);

	return 0;
}

static int s6e8ax0_ldi_init(struct lcd_info *lcd)
{
	int ret = 0;

#if defined(CONFIG_S6E8AA0_AMS529HA01)
	s6e8ax0_write(lcd, SEQ_APPLY_LEVEL_2_KEY, ARRAY_SIZE(SEQ_APPLY_LEVEL_2_KEY));
	s6e8ax0_write(lcd, SEQ_LTPS_DELAY, ARRAY_SIZE(SEQ_LTPS_DELAY));
	s6e8ax0_write(lcd, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	msleep(22);
	s6e8ax0_write(lcd, SEQ_PANEL_CONDITION_SET, ARRAY_SIZE(SEQ_PANEL_CONDITION_SET));
	s6e8ax0_write(lcd, SEQ_DISPLAY_CONDITION_SET, ARRAY_SIZE(SEQ_DISPLAY_CONDITION_SET));
	s6e8ax0_gamma_ctl(lcd);
	s6e8ax0_write(lcd, SEQ_ETC_SOURCE_CONTROL, ARRAY_SIZE(SEQ_ETC_SOURCE_CONTROL));
	s6e8ax0_write(lcd, SEQ_ETC_PENTILE_CONTROL, ARRAY_SIZE(SEQ_ETC_PENTILE_CONTROL));
	s6e8ax0_write(lcd, SEQ_ETC_POWER_CONTROL, ARRAY_SIZE(SEQ_ETC_POWER_CONTROL));
	s6e8ax0_write(lcd, SEQ_ELVSS_NVM_SETTING, ARRAY_SIZE(SEQ_ELVSS_NVM_SETTING));
	s6e8ax0_write(lcd, SEQ_ELVSS_CONTROL, ARRAY_SIZE(SEQ_ELVSS_CONTROL));
#else
	s6e8ax0_write(lcd, SEQ_APPLY_LEVEL_2, ARRAY_SIZE(SEQ_APPLY_LEVEL_2));
	s6e8ax0_write(lcd, SEQ_APPLY_MTP_KEY_ENABLE, ARRAY_SIZE(SEQ_APPLY_MTP_KEY_ENABLE));
	s6e8ax0_write(lcd, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	msleep(22);

	/* 4.8" HD for M0/C1*/
	if (lcd->id[1] == 0x20 || lcd->id[1] == 0x40 || lcd->id[1] == 0x60) {
		s6e8ax0_write(lcd, SEQ_PANEL_CONDITION_SET_500MBPS,\
			ARRAY_SIZE(SEQ_PANEL_CONDITION_SET_500MBPS));
		s6e8ax0_write(lcd, SEQ_DISPLAY_CONDITION_SET, ARRAY_SIZE(SEQ_DISPLAY_CONDITION_SET));
		s6e8ax0_gamma_ctl(lcd);
		s6e8ax0_write(lcd, SEQ_ETC_SOURCE_CONTROL, ARRAY_SIZE(SEQ_ETC_SOURCE_CONTROL));
		s6e8ax0_write(lcd, SEQ_ETC_PENTILE_CONTROL, ARRAY_SIZE(SEQ_ETC_PENTILE_CONTROL));
		s6e8ax0_write(lcd, SEQ_ETC_NVM_SETTING, ARRAY_SIZE(SEQ_ETC_NVM_SETTING));
		s6e8ax0_write(lcd, SEQ_ETC_POWER_CONTROL, ARRAY_SIZE(SEQ_ETC_POWER_CONTROL));
	} else if (lcd->id[1] == 0xae) {
		s6e8ax0_write(lcd, SEQ_PANEL_CONDITION_SET_480MBPS_46, ARRAY_SIZE(SEQ_PANEL_CONDITION_SET_480MBPS_46));
		s6e8ax0_write(lcd, SEQ_DISPLAY_CONDITION_SET, ARRAY_SIZE(SEQ_DISPLAY_CONDITION_SET));
		s6e8ax0_gamma_ctl(lcd);
		s6e8ax0_write(lcd, SEQ_ETC_SOURCE_CONTROL, ARRAY_SIZE(SEQ_ETC_SOURCE_CONTROL));
		s6e8ax0_write(lcd, SEQ_ETC_PENTILE_CONTROL_46, ARRAY_SIZE(SEQ_ETC_PENTILE_CONTROL_46));
		s6e8ax0_write(lcd, SEQ_ETC_NVM_SETTING_46, ARRAY_SIZE(SEQ_ETC_NVM_SETTING_46));
		s6e8ax0_write(lcd, SEQ_ETC_POWER_CONTROL_46, ARRAY_SIZE(SEQ_ETC_POWER_CONTROL_46));
	} else {
		s6e8ax0_write(lcd, SEQ_PANEL_CONDITION_SET_500MBPS_46, ARRAY_SIZE(SEQ_PANEL_CONDITION_SET_500MBPS_46));
		s6e8ax0_write(lcd, SEQ_DISPLAY_CONDITION_SET, ARRAY_SIZE(SEQ_DISPLAY_CONDITION_SET));
		s6e8ax0_gamma_ctl(lcd);
		s6e8ax0_write(lcd, SEQ_ETC_SOURCE_CONTROL, ARRAY_SIZE(SEQ_ETC_SOURCE_CONTROL));
		s6e8ax0_write(lcd, SEQ_ETC_PENTILE_CONTROL_46, ARRAY_SIZE(SEQ_ETC_PENTILE_CONTROL_46));
		s6e8ax0_write(lcd, SEQ_ETC_NVM_SETTING_46, ARRAY_SIZE(SEQ_ETC_NVM_SETTING_46));
		s6e8ax0_write(lcd, SEQ_ETC_POWER_CONTROL_46, ARRAY_SIZE(SEQ_ETC_POWER_CONTROL_46));
	}

	s6e8ax0_write(lcd, SEQ_ELVSS_CONTROL, ARRAY_SIZE(SEQ_ELVSS_CONTROL));
#endif

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

	dev_info(&lcd->ld->dev, "%s\n", __func__);

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

	msleep(135);

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

static int s6e8ax0_check_fb(struct lcd_device *ld, struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;
	struct lcd_info *lcd = lcd_get_data(ld);

	/* dev_info(&lcd->ld->dev, "%s, fb%d\n", __func__, win->id); */

	return 0;
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
	.check_fb  = s6e8ax0_check_fb,
};

static const struct backlight_ops s6e8ax0_backlight_ops  = {
	.get_brightness = s6e8ax0_get_brightness,
	.update_status = s6e8ax0_set_brightness,
};

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
			mutex_lock(&lcd->bl_lock);
			lcd->acl_enable = value;
			if (lcd->ldi_enable)
				s6e8ax0_set_acl(lcd);
			mutex_unlock(&lcd->bl_lock);
		}
	}
	return size;
}

static DEVICE_ATTR(power_reduce, 0664, power_reduce_show, power_reduce_store);

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[15];
#if defined(CONFIG_S6E8AA0_AMS529HA01)
	sprintf(temp, "SMD_AMS529HA01\n");
#else
	sprintf(temp, "SMD_AMS480GYXX\n");
#endif
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0444, lcd_type_show, NULL);

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

	for (i = 0; i < ELVSS_STATUS_MAX; i++) {
		for (j = 0; j < ELVSS_PARAM_SIZE; j++)
			printk("0x%02x, ", lcd->elvss_table[i][j]);
		printk("\n");
	}

	return strlen(buf);
}
static DEVICE_ATTR(gamma_table, 0444, gamma_table_show, NULL);

static ssize_t auto_brightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->auto_brightness);
	strcpy(buf, temp);

	return strlen(buf);
}

static ssize_t auto_brightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = strict_strtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (lcd->auto_brightness != value) {
			dev_info(dev, "%s - %d, %d\n", __func__, lcd->auto_brightness, value);
			mutex_lock(&lcd->bl_lock);
			lcd->auto_brightness = value;
			mutex_unlock(&lcd->bl_lock);
			if (lcd->ldi_enable)
				update_brightness(lcd, 0);
		}
	}
	return size;
}

static DEVICE_ATTR(auto_brightness, 0644, auto_brightness_show, auto_brightness_store);

#ifdef CONFIG_HAS_EARLYSUSPEND
struct lcd_info *g_lcd;

void s6e8ax0_early_suspend(void)
{
	struct lcd_info *lcd = g_lcd;

	set_dsim_lcd_enabled(0);

	dev_info(&lcd->ld->dev, "+%s\n", __func__);
#if defined(GPIO_OLED_DET)
	disable_irq(lcd->irq);
	gpio_request(GPIO_OLED_DET, "OLED_DET");
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_OLED_DET, GPIO_LEVEL_LOW);
	gpio_free(GPIO_OLED_DET);
#endif
	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

void s6e8ax0_late_resume(void)
{
	struct lcd_info *lcd = g_lcd;

	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	s6e8ax0_power(lcd, FB_BLANK_UNBLANK);
#if defined(GPIO_OLED_DET)
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
	enable_irq(lcd->irq);
#endif
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
		lcd->connected = 0;
		dev_info(&lcd->ld->dev, "panel is not connected well\n");
	}
}

#ifdef SMART_DIMMING
static int s6e8ax0_read_mtp(struct lcd_info *lcd, u8 *mtp_data)
{
	int ret;

	ret = s6e8ax0_read(lcd, LDI_MTP_ADDR, LDI_MTP_LENGTH, mtp_data, 0);

	return ret;
}

#if defined(CONFIG_S6E8AA0_AMS529HA01)
static void s6e8aa0_check_id(struct lcd_info *lcd, u8 *idbuf)
{
	u32 i;

	for (i = 0; i < LDI_ID_LEN; i++)
		lcd->smart.panelid[i] = idbuf[i];

	if (idbuf[2] == 0x33)
		lcd->support_elvss = 0;
	else {
		lcd->support_elvss = 1;
		lcd->elvss.limit = (idbuf[2] & 0xc0) >> 6;
		lcd->elvss.reference = idbuf[2] & 0x3f;
		printk(KERN_DEBUG "Dynamic ELVSS Information, 0x%x\n", lcd->elvss.reference);
	}
}
#else
static void s6e8aa0_check_id(struct lcd_info *lcd, u8 *idbuf)
{
	int i;

	for (i = 0; i < LDI_ID_LEN; i++)
		lcd->smart.panelid[i] = idbuf[i];

	if (idbuf[0] == PANEL_A1_M3)
		lcd->support_elvss = 0;
	else {
		lcd->support_elvss = 1;
		lcd->elvss.reference = idbuf[2] & (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4));
		printk(KERN_DEBUG "Dynamic ELVSS Information, 0x%x\n", lcd->elvss.reference);
	}
}
#endif
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
	lcd->auto_brightness = 0;

	ret = device_create_file(&lcd->ld->dev, &dev_attr_power_reduce);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_gamma_table);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->bd->dev, &dev_attr_auto_brightness);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	dev_set_drvdata(dev, lcd);

	mutex_init(&lcd->lock);
	mutex_init(&lcd->bl_lock);

	s6e8ax0_read_id(lcd, lcd->id);

	dev_info(&lcd->ld->dev, "ID: %x, %x, %x\n", lcd->id[0], lcd->id[1], lcd->id[2]);

	dev_info(&lcd->ld->dev, "s6e8aa0 lcd panel driver has been probed.\n");

#ifdef SMART_DIMMING
	s6e8aa0_check_id(lcd, lcd->id);

	init_table_info(&lcd->smart);

	ret = s6e8ax0_read_mtp(lcd, mtp_data);
	if (!ret) {
		printk(KERN_ERR "[LCD:ERROR] : %s read mtp failed\n", __func__);
		/*return -EPERM;*/
	}

	calc_voltage_table(&lcd->smart, mtp_data);

	if (lcd->support_elvss)
		ret = init_elvss_table(lcd);
	else {
		lcd->elvss_table = (unsigned char **)ELVSS_TABLE;
		ret = 0;
	}

	ret += init_gamma_table(lcd);

#ifdef CONFIG_AID_DIMMING
	if (lcd->id[1] == 0x20 || lcd->id[1] == 0x40 || lcd->id[1] == 0x60) {
		printk(KERN_INFO "AID Dimming is started. %d\n", lcd->id[1]);
		lcd->support_aid = 1;
		ret += init_aid_dimming_table(lcd);
	}
#endif

	if (ret) {
		printk(KERN_ERR "gamma table generation is failed\n");
		lcd->gamma_table = (unsigned char **)gamma22_table;
		lcd->elvss_table = (unsigned char **)ELVSS_TABLE;
	}

	update_brightness(lcd, 1);
#endif

#if defined(GPIO_OLED_DET)
	if (lcd->connected) {
		INIT_DELAYED_WORK(&lcd->oled_detection, oled_detection_work);

		lcd->irq = gpio_to_irq(GPIO_OLED_DET);

		s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
		if (request_irq(lcd->irq, oled_detection_int,
			IRQF_TRIGGER_FALLING, "oled_detection", lcd))
			pr_err("failed to reqeust irq. %d\n", lcd->irq);
	}
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
