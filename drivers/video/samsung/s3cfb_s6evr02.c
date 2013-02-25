/* linux/drivers/video/samsung/s3cfb_s6evr02.c
 *
 * MIPI-DSI based AMS555HBxx AMOLED lcd panel driver.
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
#include <linux/rtc.h>
#include <linux/reboot.h>
#include <linux/syscalls.h> /* sys_sync */
#include <plat/gpio-cfg.h>
#include <plat/regs-dsim.h>
#include <mach/dsim.h>
#include <mach/mipi_ddi.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "s5p-dsim.h"
#include "s3cfb.h"
#include "s6evr02_param.h"

#define SMART_DIMMING
#undef SMART_DIMMING_DEBUG

#ifdef SMART_DIMMING
#include "smart_dimming_s6evr02.h"
#include "aid_s6evr02.h"
#endif


#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255
#define MAX_GAMMA			300
#define DEFAULT_BRIGHTNESS		130
#define DEFAULT_GAMMA_LEVEL		GAMMA_130CD

#define LDI_ID_REG			0xD7
#define LDI_ID_LEN			3
#ifdef SMART_DIMMING
#define LDI_MTP_LENGTH		33
#define LDI_MTP_ADDR			0xC8
#endif

struct lcd_info {
	unsigned int			bl;
	unsigned int			auto_brightness;
	unsigned int			acl_enable;
	unsigned int			current_acl;
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
	struct str_smart_dim		smart;
	unsigned char			aor[GAMMA_MAX][ARRAY_SIZE(SEQ_AOR_CONTROL)];
#endif
	unsigned int			irq;
	unsigned int			connected;

#if defined(GPIO_ERR_FG)
	struct delayed_work		err_fg_detection;
	unsigned int			err_fg_detection_count;
#endif
#if defined(GPIO_OLED_DET)
	struct delayed_work		oled_detection;
	unsigned int			oled_detection_count;
#endif

	struct dsim_global		*dsim;
};

static const unsigned int candela_table[GAMMA_MAX] = {
	 20,  30,  40,  50,  60,  70,  80,  90, 100,
	102, 104, 106, 108,
	110, 120, 130, 140, 150, 160, 170, 180,
	182, 184, 186, 188,
	190, 200, 210, 220, 230, 240, 250, MAX_GAMMA-1
};

#ifdef SMART_DIMMING
static unsigned int aid_candela_table[GAMMA_MAX] = {
	base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100,
	AOR40_BASE_102, AOR40_BASE_104, AOR40_BASE_106, AOR40_BASE_108,
	AOR40_BASE_110, AOR40_BASE_120, AOR40_BASE_130, AOR40_BASE_140, AOR40_BASE_150,
	AOR40_BASE_160, AOR40_BASE_170, AOR40_BASE_180,
	AOR40_BASE_182, AOR40_BASE_184, AOR40_BASE_186, AOR40_BASE_188,
	190, 200, 210, 220, 230, 240, 250, MAX_GAMMA-1
};
#endif

extern void (*lcd_early_suspend)(void);
extern void (*lcd_late_resume)(void);

#if defined(GPIO_ERR_FG)
static void err_fg_detection_work(struct work_struct *work)
{
	struct lcd_info *lcd =
		container_of(work, struct lcd_info, err_fg_detection.work);

	int err_fg_level = gpio_get_value(GPIO_ERR_FG);

	dev_info(&lcd->ld->dev, "%s, %d, %d\n", __func__, lcd->err_fg_detection_count, err_fg_level);

	if (!err_fg_level) {
		if (lcd->err_fg_detection_count < 10) {
			schedule_delayed_work(&lcd->err_fg_detection, HZ/8);
			lcd->err_fg_detection_count++;
			set_dsim_hs_clk_toggle_count(15);
		} else
			set_dsim_hs_clk_toggle_count(0);
	} else
		set_dsim_hs_clk_toggle_count(0);

}

static irqreturn_t err_fg_detection_int(int irq, void *_lcd)
{
	struct lcd_info *lcd = _lcd;

	dev_info(&lcd->ld->dev, "\t\t%s\n", __func__);

	lcd->err_fg_detection_count = 0;
	schedule_delayed_work(&lcd->err_fg_detection, HZ/16);

	return IRQ_HANDLED;
}
#endif

#if defined(GPIO_OLED_DET)
static void oled_detection_work(struct work_struct *work)
{
	struct lcd_info *lcd =
		container_of(work, struct lcd_info, oled_detection.work);

	struct file *fp;
	char name[128];
	struct timespec ts;
	struct rtc_time tm;

	int oled_det_level = gpio_get_value(GPIO_OLED_DET);

	dev_info(&lcd->ld->dev, "%s, GPIO_OLED_DET is %s\n", __func__, oled_det_level ? "high" : "low");

	if (!oled_det_level) {
		if (lcd->oled_detection_count < 3) {
			getnstimeofday(&ts);
			rtc_time_to_tm(ts.tv_sec, &tm);
			sprintf(name, "%s%02d-%02d_%02d:%02d:%02d_%02d",
				"/sdcard/", tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, lcd->oled_detection_count);
			fp = filp_open(name, O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);

			if (IS_ERR_OR_NULL(fp))
				dev_info(&lcd->ld->dev, "fail to create vgh detection log file, %s\n", name);

			schedule_delayed_work(&lcd->oled_detection, msecs_to_jiffies(10));
			lcd->oled_detection_count++;
		} else {
			dev_info(&lcd->ld->dev, "VGH IS NOT OK! LCD SMASH!!!\n");
			getnstimeofday(&ts);
			rtc_time_to_tm(ts.tv_sec, &tm);
			sprintf(name, "%s%02d-%02d_%02d:%02d:%02d_POWEROFF",
				"/sdcard/", tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			fp = filp_open(name, O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);

			if (IS_ERR_OR_NULL(fp))
				dev_info(&lcd->ld->dev, "fail to create vgh detection log file, %s\n", name);

			sys_sync();
			kernel_power_off();
		}
	} else
		dev_info(&lcd->ld->dev, "VGH IS OK\n");

}

static irqreturn_t oled_detection_int(int irq, void *_lcd)
{
	struct lcd_info *lcd = _lcd;

	dev_info(&lcd->ld->dev, "\t\t%s\n", __func__);

	schedule_delayed_work(&lcd->oled_detection, msecs_to_jiffies(10));

	return IRQ_HANDLED;
}
#endif

static int _s6evr02_write(struct lcd_info *lcd, const unsigned char *seq, int len)
{
	int size;
	const unsigned char *wbuf;
	int ret = 0;

	if (!lcd->connected)
		return 0;

	mutex_lock(&lcd->lock);

	size = len;
	wbuf = seq;

	if (size == 1)
		ret = lcd->dsim->ops->cmd_write(lcd->dsim, DCS_WR_NO_PARA, wbuf[0], 0);
	else if (size == 2)
		ret = lcd->dsim->ops->cmd_write(lcd->dsim, DCS_WR_1_PARA, wbuf[0], wbuf[1]);
	else
		ret = lcd->dsim->ops->cmd_write(lcd->dsim, DCS_LONG_WR, (unsigned int)wbuf, size);

	mutex_unlock(&lcd->lock);

	return ret;
}

static int s6evr02_write(struct lcd_info *lcd, const unsigned char *seq, int len)
{
	int ret = 0;
	int retry_cnt = 1;

retry:
	ret = _s6evr02_write(lcd, seq, len);
	if (!ret) {
		if (retry_cnt) {
			dev_dbg(&lcd->ld->dev, "%s :: retry: %d\n", __func__, retry_cnt);
			retry_cnt--;
			goto retry;
		} else
			dev_dbg(&lcd->ld->dev, "%s :: 0x%02x\n", __func__, seq[1]);
	}

	return ret;
}

static int _s6evr02_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf)
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

static int s6evr02_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf, u8 retry_cnt)
{
	int ret = 0;

retry:
	ret = _s6evr02_read(lcd, addr, count, buf);
	if (!ret) {
		if (retry_cnt) {
			dev_dbg(&lcd->ld->dev, "%s :: retry: %d\n", __func__, retry_cnt);
			retry_cnt--;
			goto retry;
		} else
			dev_dbg(&lcd->ld->dev, "%s :: 0x%02x\n", __func__, addr);
	}

	return ret;
}

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
	case 100 ... 101:
		backlightlevel = GAMMA_100CD;
		break;
	case 102 ... 103:
		backlightlevel = GAMMA_102CD;
		break;
	case 104 ... 105:
		backlightlevel = GAMMA_104CD;
		break;
	case 106 ... 107:
		backlightlevel = GAMMA_106CD;
		break;
	case 108 ... 109:
		backlightlevel = GAMMA_108CD;
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

#ifdef SMART_DIMMING
static int s6evr02_aid_parameter_ctl(struct lcd_info *lcd, u8 force)
{
	if ((aid_command_table[lcd->bl][0] != aid_command_table[lcd->current_bl][0]) || force)
		s6evr02_write(lcd, lcd->aor[lcd->bl], AID_PARAM_SIZE);

	return 0;
}
#endif

static int s6evr02_gamma_ctl(struct lcd_info *lcd)
{
	/* s6evr02_write(lcd, SEQ_APPLY_LEVEL_2_KEY, ARRAY_SIZE(SEQ_APPLY_LEVEL_2_KEY)); */
	s6evr02_write(lcd, lcd->gamma_table[lcd->bl], GAMMA_PARAM_SIZE);
	s6evr02_write(lcd, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));
	/* s6evr02_write(lcd, SEQ_BRIGHTNESS_CONTROL_ON, ARRAY_SIZE(SEQ_BRIGHTNESS_CONTROL_ON)); */

	return 0;
}

static int s6evr02_set_acl(struct lcd_info *lcd, u8 force)
{
	int ret = 0, level = 0;
	u32 candela = candela_table[lcd->bl];

	switch (candela) {
	case 0 ... 29:
		level = ACL_STATUS_0P;
		break;
	case 30 ... 39:
		level = ACL_STATUS_33P;
		break;
	default:
		level = ACL_STATUS_40P;
		break;
	}

	if (!lcd->acl_enable)
		level = ACL_STATUS_0P;

	if (force || lcd->current_acl != ACL_CUTOFF_TABLE[level][1]) {
		ret = s6evr02_write(lcd, ACL_CUTOFF_TABLE[level], ACL_PARAM_SIZE);
		lcd->current_acl = ACL_CUTOFF_TABLE[level][1];
		dev_dbg(&lcd->ld->dev, "current_acl = %d\n", lcd->current_acl);
	}

	if (ret)
		ret = -EPERM;

	return ret;
}

static int s6evr02_set_elvss(struct lcd_info *lcd, u8 force)
{
	int ret = 0, elvss_level = 0;
	u32 candela = candela_table[lcd->bl];

	switch (candela) {
	case 0 ... 49:
		elvss_level = ELVSS_STATUS_20;
		break;
	case 50 ... 79:
		elvss_level = ELVSS_STATUS_50;
		break;
	case 80 ... 99:
		elvss_level = ELVSS_STATUS_80;
		break;
	case 100 ... 109:
		elvss_level = ELVSS_STATUS_100;
		break;
	case 110 ... 119:
		elvss_level = ELVSS_STATUS_110;
		break;
	case 120 ... 129:
		elvss_level = ELVSS_STATUS_120;
		break;
	case 130 ... 139:
		elvss_level = ELVSS_STATUS_130;
		break;
	case 140 ... 149:
		elvss_level = ELVSS_STATUS_140;
		break;
	case 150 ... 159:
		elvss_level = ELVSS_STATUS_150;
		break;
	case 160 ... 169:
		elvss_level = ELVSS_STATUS_160;
		break;
	case 170 ... 179:
		elvss_level = ELVSS_STATUS_170;
		break;
	case 180 ... 189:
		elvss_level = ELVSS_STATUS_180;
		break;
	case 190 ... 199:
		elvss_level = ELVSS_STATUS_190;
		break;
	case 200 ... 209:
		elvss_level = ELVSS_STATUS_200;
		break;
	case 210 ... 219:
		elvss_level = ELVSS_STATUS_210;
		break;
	case 220 ... 229:
		elvss_level = ELVSS_STATUS_220;
		break;
	case 230 ... 239:
		elvss_level = ELVSS_STATUS_230;
		break;
	case 240 ... 250:
		elvss_level = ELVSS_STATUS_240;
		break;
	case 255 ... 299:
		elvss_level = ELVSS_STATUS_300;
		break;
	}

	if (force || lcd->current_elvss != lcd->elvss_table[elvss_level][2]) {
		ret = s6evr02_write(lcd, lcd->elvss_table[elvss_level], ELVSS_PARAM_SIZE);
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
		lcd->elvss_table[i][0] = 0xB6;
		lcd->elvss_table[i][1] = 0x08;
		lcd->elvss_table[i][2] = ELVSS_CONTROL_TABLE[i][2];
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

#ifdef SMART_DIMMING
static int init_gamma_table(struct lcd_info *lcd , const u8 *mtp_data)
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
		lcd->gamma_table[i][0] = 0xCA;
	}

	for (i = 0; i < GAMMA_MAX; i++) {
		if (candela_table[i] == 20)
			calc_gamma_table_210_20_100(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_21, mtp_data);
		else if (candela_table[i] == 30)
			calc_gamma_table_210_20_100(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_213, mtp_data);
		else if (candela_table[i] == 40)
			calc_gamma_table_210_20_100(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_215, mtp_data);
		else if (candela_table[i] == 50)
			calc_gamma_table_210_20_100(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_218, mtp_data);
		else if (candela_table[i] == 60)
			calc_gamma_table_210_20_100(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_22, mtp_data);
		else if (candela_table[i] == 70)
			calc_gamma_table_210_20_100(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_222, mtp_data);
		else if (candela_table[i] == 80)
			calc_gamma_table_210_20_100(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_223, mtp_data);
		else if (candela_table[i] == 90)
			calc_gamma_table_210_20_100(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_224, mtp_data);
		else if (candela_table[i] == 100)
			calc_gamma_table_210_20_100(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_225, mtp_data);
		else if (candela_table[i] == 102)
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_224, mtp_data);
		else if (candela_table[i] == 104)
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_223, mtp_data);
		else if (candela_table[i] == 106)
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_222, mtp_data);
		else if (candela_table[i] == 108)
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_221, mtp_data);
		else if (candela_table[i] == 182)
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_221, mtp_data);
		else if (candela_table[i] == 184)
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_222, mtp_data);
		else if (candela_table[i] == 186)
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_223, mtp_data);
		else if (candela_table[i] == 188)
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_224, mtp_data);
		else if (candela_table[i] == 190)
			calc_gamma_table_215_190(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_225, mtp_data);
		else if ((candela_table[i] > 190) && (candela_table[i] < MAX_GAMMA-1))
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_225 , mtp_data);
		else if (candela_table[i] == MAX_GAMMA-1)
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_22, mtp_data);
		else
			calc_gamma_table(&lcd->smart, aid_candela_table[i], &lcd->gamma_table[i][1], G_22, mtp_data);
	}


#ifdef SMART_DIMMING_DEBUG
	for (i = 0; i < GAMMA_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			printk("%d,", lcd->gamma_table[i][j]);
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
	unsigned int i, j, c;
	u16 reverse_seq[] = {0, 28, 29, 30, 31, 32, 33, 25, 26, 27, 22, 23, 24, 19, 20, 21, 16, 17, 18, 13, 14, 15, 10, 11, 12, 7, 8, 9, 4, 5, 6, 1, 2, 3};
	u16 temp[GAMMA_PARAM_SIZE];

	for (i = 0; i < ARRAY_SIZE(aid_rgb_fix_table); i++) {
		j = (aid_rgb_fix_table[i].gray * 3 + aid_rgb_fix_table[i].rgb) + 1;
		c = lcd->gamma_table[aid_rgb_fix_table[i].candela_idx][j] + aid_rgb_fix_table[i].offset;
		if (c > 0xff)
			lcd->gamma_table[aid_rgb_fix_table[i].candela_idx][j] = 0xff;
		else
			lcd->gamma_table[aid_rgb_fix_table[i].candela_idx][j] += aid_rgb_fix_table[i].offset;
	}

	for (i = 0; i < GAMMA_MAX; i++) {
		memcpy(lcd->aor[i], SEQ_AOR_CONTROL, AID_PARAM_SIZE);
		lcd->aor[i][0x01] = aid_command_table[i][0];
	}

#ifdef SMART_DIMMING_DEBUG
	for (i = 0; i < GAMMA_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			printk("%d,", lcd->gamma_table[i][j]);
		printk("\n");
	}
	printk("\n");
#endif

	for (i = 0; i < GAMMA_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			temp[j] = lcd->gamma_table[i][reverse_seq[j]];

		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			lcd->gamma_table[i][j] = temp[j];

		for (c = CI_RED; c < CI_MAX ; c++)
			lcd->gamma_table[i][31+c] = lcd->smart.default_gamma[30+c];
	}

#ifdef SMART_DIMMING_DEBUG
	for (i = 0; i < GAMMA_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			printk("%d,", lcd->gamma_table[i][j]);
		printk("\n");
	}
#endif

	return 0;
}
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
		s6evr02_gamma_ctl(lcd);

		s6evr02_aid_parameter_ctl(lcd, force);

		s6evr02_set_acl(lcd, force);

		s6evr02_set_elvss(lcd, force);

		lcd->current_bl = lcd->bl;

		dev_info(&lcd->ld->dev, "brightness=%d, bl=%d, candela=%d\n", brightness, lcd->bl, candela_table[lcd->bl]);
	}

	mutex_unlock(&lcd->bl_lock);

	return 0;
}

static int s6evr02_ldi_init(struct lcd_info *lcd)
{
	int ret = 0;
	s6evr02_write(lcd, SEQ_APPLY_LEVEL_2_KEY, ARRAY_SIZE(SEQ_APPLY_LEVEL_2_KEY));
	s6evr02_write(lcd, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));

	if (lcd->id[1] == 0x10) { /* for S.LSI UB(YOUM) */
		msleep(20);
		s6evr02_write(lcd, SEQ_GAMMA_CONDITION_SET_UB, ARRAY_SIZE(SEQ_GAMMA_CONDITION_SET_UB));
		s6evr02_write(lcd, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));
		s6evr02_write(lcd, SEQ_BRIGHTNESS_CONTROL_ON, ARRAY_SIZE(SEQ_BRIGHTNESS_CONTROL_ON));
		s6evr02_write(lcd, SEQ_AOR_CONTROL, ARRAY_SIZE(SEQ_AOR_CONTROL));
		s6evr02_write(lcd, SEQ_ELVSS_CONDITION_SET_UB, ARRAY_SIZE(SEQ_ELVSS_CONDITION_SET_UB));
		s6evr02_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
	} else {
		msleep(20);
		s6evr02_gamma_ctl(lcd);
		s6evr02_write(lcd, SEQ_BRIGHTNESS_CONTROL_ON, ARRAY_SIZE(SEQ_BRIGHTNESS_CONTROL_ON));
		s6evr02_write(lcd, SEQ_AOR_CONTROL, ARRAY_SIZE(SEQ_AOR_CONTROL));
		s6evr02_write(lcd, SEQ_ELVSS_CONDITION_SET, ARRAY_SIZE(SEQ_ELVSS_CONDITION_SET));
		s6evr02_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
	}

	return ret;
}

static int s6evr02_ldi_enable(struct lcd_info *lcd)
{
	int ret = 0;

	s6evr02_write(lcd, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));

	return ret;
}

static int s6evr02_ldi_disable(struct lcd_info *lcd)
{
	int ret = 0;

	s6evr02_write(lcd, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));

	msleep(35);

	s6evr02_write(lcd, SEQ_SLEEP_IN, ARRAY_SIZE(SEQ_SLEEP_IN));

	msleep(100);

	return ret;
}

static int s6evr02_power_on(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	ret = s6evr02_ldi_init(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}

	msleep(120);

	ret = s6evr02_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}

	lcd->ldi_enable = 1;

	update_brightness(lcd, 1);
err:
	return ret;
}

static int s6evr02_power_off(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->ldi_enable = 0;

	ret = s6evr02_ldi_disable(lcd);

	msleep(135);

	return ret;
}

static int s6evr02_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = s6evr02_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = s6evr02_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int s6evr02_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return s6evr02_power(lcd, power);
}

static int s6evr02_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}


static int s6evr02_set_brightness(struct backlight_device *bd)
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

static int s6evr02_get_brightness(struct backlight_device *bd)
{
	struct lcd_info *lcd = bl_get_data(bd);

	return candela_table[lcd->bl];
}

static int s6evr02_check_fb(struct lcd_device *ld, struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;
	struct lcd_info *lcd = lcd_get_data(ld);

	//dev_info(&lcd->ld->dev, "%s, fb%d\n", __func__, win->id);

	return 0;
}

static struct lcd_ops s6evr02_lcd_ops = {
	.set_power = s6evr02_set_power,
	.get_power = s6evr02_get_power,
	.check_fb  = s6evr02_check_fb,
};

static const struct backlight_ops s6evr02_backlight_ops  = {
	.get_brightness = s6evr02_get_brightness,
	.update_status = s6evr02_set_brightness,
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
			mutex_unlock(&lcd->bl_lock);
			if (lcd->ldi_enable)
				update_brightness(lcd, 1);
		}
	}
	return size;
}

static DEVICE_ATTR(power_reduce, 0664, power_reduce_show, power_reduce_store);

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[15];

	sprintf(temp, "SMD_AMS555HBxx\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0444, lcd_type_show, NULL);

static ssize_t window_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[15];

	sprintf(temp, "%x %x %x\n", lcd->id[0], lcd->id[1], lcd->id[2]);

	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(window_type, 0444, window_type_show, NULL);

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
				update_brightness(lcd, 1);
		}
	}
	return size;
}

static DEVICE_ATTR(auto_brightness, 0644, auto_brightness_show, auto_brightness_store);

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct lcd_info *g_lcd;

void s6evr02_early_suspend(void)
{
	struct lcd_info *lcd = g_lcd;

	set_dsim_lcd_enabled(0);

	dev_info(&lcd->ld->dev, "+%s\n", __func__);

#if defined(GPIO_ERR_FG)
	disable_irq(lcd->irq);
	gpio_request(GPIO_ERR_FG, "OLED_DET");
	s3c_gpio_cfgpin(GPIO_ERR_FG, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_ERR_FG, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_ERR_FG, GPIO_LEVEL_LOW);
	gpio_free(GPIO_ERR_FG);
#endif
#if defined(GPIO_OLED_DET)
	disable_irq(gpio_to_irq(GPIO_OLED_DET));
	gpio_request(GPIO_OLED_DET, "OLED_DET");
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_OLED_DET, GPIO_LEVEL_LOW);
	gpio_free(GPIO_OLED_DET);
#endif

	s6evr02_power(lcd, FB_BLANK_POWERDOWN);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

void s6evr02_late_resume(void)
{
	struct lcd_info *lcd = g_lcd;

	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	s6evr02_power(lcd, FB_BLANK_UNBLANK);

#if defined(GPIO_ERR_FG)
	s3c_gpio_cfgpin(GPIO_ERR_FG, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_ERR_FG, S3C_GPIO_PULL_NONE);
	enable_irq(lcd->irq);
#endif
#if defined(GPIO_OLED_DET) && defined(GPIO_OLED_ID)
	if (gpio_get_value(GPIO_OLED_ID)) {
		s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
		enable_irq(gpio_to_irq(GPIO_OLED_DET));
	}
#endif

	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	set_dsim_lcd_enabled(1);

	return ;
}
#endif


static void s6evr02_read_id(struct lcd_info *lcd, u8 *buf)
{
	int ret = 0;

	ret = s6evr02_read(lcd, LDI_ID_REG, LDI_ID_LEN, buf, 2);
	if (!ret) {
		lcd->connected = 0;
		dev_info(&lcd->ld->dev, "panel is not connected well\n");
	}
}

#ifdef SMART_DIMMING
static int s6evr02_read_mtp(struct lcd_info *lcd, u8 *mtp_data)
{
	int ret;
	s6evr02_write(lcd, SEQ_APPLY_LEVEL_2_KEY, ARRAY_SIZE(SEQ_APPLY_LEVEL_2_KEY));
	s6evr02_write(lcd, SEQ_APPLY_LEVEL_3_KEY, ARRAY_SIZE(SEQ_APPLY_LEVEL_3_KEY));
	msleep(20); /* one frame delay befor reading MTP. */
	ret = s6evr02_read(lcd, LDI_MTP_ADDR, LDI_MTP_LENGTH, mtp_data, 1);
	s6evr02_write(lcd, SEQ_APPLY_LEVEL_3_KEY_DISABLE, ARRAY_SIZE(SEQ_APPLY_LEVEL_3_KEY_DISABLE));
	/* s6evr02_write(lcd, SEQ_APPLY_LEVEL_2_KEY_DISABLE, ARRAY_SIZE(SEQ_APPLY_LEVEL_2_KEY_DISABLE)); */

	return ret;
}
#endif

static int s6evr02_probe(struct device *dev)
{
	int ret = 0, i;
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

	lcd->ld = lcd_device_register("panel", dev, lcd, &s6evr02_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("panel", dev, lcd, &s6evr02_backlight_ops, NULL);
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
	lcd->current_acl = 0;
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

	ret = device_create_file(&lcd->ld->dev, &dev_attr_window_type);
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

	s6evr02_read_id(lcd, lcd->id);

	dev_info(&lcd->ld->dev, "ID: %x, %x, %x\n", lcd->id[0], lcd->id[1], lcd->id[2]);

	dev_info(&lcd->ld->dev, "%s lcd panel driver has been probed.\n", dev_name(dev));

#ifdef SMART_DIMMING
	for (i = 0; i < LDI_ID_LEN; i++) {
		lcd->smart.panelid[i] = lcd->id[i];
	}

	init_table_info(&lcd->smart);

	ret = s6evr02_read_mtp(lcd, mtp_data);
/*
	for (i = 0; i < LDI_MTP_LENGTH ; i++)
		printk(" %dth mtp value is %x\n", i, mtp_data[i]);
*/
	if (!ret) {
		printk(KERN_ERR "[LCD:ERROR] : %s read mtp failed\n", __func__);
		/*return -EPERM;*/
	}

	calc_voltage_table(&lcd->smart, mtp_data);

	ret = init_elvss_table(lcd);
	ret += init_gamma_table(lcd, mtp_data);
	ret += init_aid_dimming_table(lcd);

	if (ret)
		printk(KERN_ERR "gamma table generation is failed\n");


	update_brightness(lcd, 1);
#endif

#if defined(GPIO_ERR_FG)
	if (lcd->connected) {
		INIT_DELAYED_WORK(&lcd->err_fg_detection, err_fg_detection_work);

		lcd->irq = gpio_to_irq(GPIO_ERR_FG);

		irq_set_irq_type(lcd->irq, IRQ_TYPE_EDGE_RISING);

		s3c_gpio_cfgpin(GPIO_ERR_FG, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(GPIO_ERR_FG, S3C_GPIO_PULL_NONE);

		if (request_irq(lcd->irq, err_fg_detection_int,
			IRQF_TRIGGER_RISING, "err_fg_detection", lcd))
			pr_err("failed to reqeust irq. %d\n", lcd->irq);
	}
#endif
#if defined(GPIO_OLED_DET)
	if (lcd->connected) {
		INIT_DELAYED_WORK(&lcd->oled_detection, oled_detection_work);

		s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);

		if (request_irq(gpio_to_irq(GPIO_OLED_DET), oled_detection_int,
			IRQF_TRIGGER_FALLING, "oled_detection", lcd))
			pr_err("failed to reqeust irq. %d\n", gpio_to_irq(GPIO_OLED_DET));
	}
#endif

	lcd_early_suspend = s6evr02_early_suspend;
	lcd_late_resume = s6evr02_late_resume;

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

static int __devexit s6evr02_remove(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	s6evr02_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	backlight_device_unregister(lcd->bd);
	kfree(lcd);

	return 0;
}

/* Power down all displays on reboot, poweroff or halt. */
static void s6evr02_shutdown(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	s6evr02_power(lcd, FB_BLANK_POWERDOWN);
}

static struct mipi_lcd_driver s6evr02_mipi_driver = {
	.name = "s6evr02",
	.probe			= s6evr02_probe,
	.remove			= __devexit_p(s6evr02_remove),
	.shutdown		= s6evr02_shutdown,
};

static int s6evr02_init(void)
{
	return s5p_dsim_register_lcd_driver(&s6evr02_mipi_driver);
}

static void s6evr02_exit(void)
{
	return;
}

module_init(s6evr02_init);
module_exit(s6evr02_exit);

MODULE_DESCRIPTION("MIPI-DSI S6EVER02:AMS555HBXX (720x1280) Panel Driver");
MODULE_LICENSE("GPL");
