/* linux/drivers/video/samsung/s3cfb_lms501xx.c
 *
 * MIPI-DSI based LMS501XX TFT lcd panel driver.
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
#include "lms501xx.h"

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255
#define MAX_GAMMA			255
#define DEFAULT_BRIGHTNESS		130
#define DEFAULT_GAMMA_LEVEL		GAMMA_130CD

#define LDI_ID_REG			0xDA
#define LDI_ID_LEN			3

#define DDI_STATUS_REG_PREVENTESD
struct lcd_info {
	unsigned int			bl;
	unsigned int			candela;
	unsigned char			pwm;
	unsigned int			auto_brightness;
	unsigned int			siop_enable;
	unsigned int			current_cabc;
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

	unsigned int			irq;
	unsigned int			connected;

#if defined(GPIO_OLED_DET)
	struct delayed_work		oled_detection;
	unsigned int			oled_detection_count;
#endif
#ifdef DDI_STATUS_REG_PREVENTESD
	struct delayed_work		check_ddi;
#endif
	struct dsim_global		*dsim;
};

static const unsigned int candela_table[GAMMA_MAX-1] = {
	 30,  40,  50,  60,  70,  80,  90, 100, 110, 120,
	130, 140, 150, 160, 170, 180, 190, 200, 210, 220,
	230, 240, MAX_BRIGHTNESS, MAX_GAMMA
};

extern void (*lcd_early_suspend)(void);
extern void (*lcd_late_resume)(void);
struct LCD_BRIGHTNESS {
	int off;
	int deflt;
	int dim;
	int min;
	int min_center;
	int center;
	int center_max;
	int max;
};

struct LCD_BRIGHTNESS tbl_plat = {
	.off = 0,
	.deflt = 150,
	.dim = 20,
	.min = 30,
	.min_center = 80,
	.center = 130,
	.center_max = 190,
	.max = 255,
};

struct LCD_BRIGHTNESS tbl_normal = {
	.off = 0,
	.deflt = 220,
	.dim = 15,
	.min = 15,
	.min_center = 110,
	.center = 220,
	.center_max = 320,
	.max = 380,
};


struct LCD_BRIGHTNESS pwm_normal = {
	.off = 0x00,
	.deflt = 0x61,
	.dim = 0x0d,
	.min = 0x0d,
	.min_center = 0x37,
	.center = 0x61,
	.center_max = 0x7d,
	.max = 0x9a,
};

struct LCD_BRIGHTNESS tbl_cabc = {
	.off = 0,
	.deflt = 180,
	.dim = 15,
	.min = 15,
	.min_center = 90,
	.center = 180,
	.center_max = 260,
	.max = 330,
};

struct LCD_BRIGHTNESS pwm_cabc = {
	.off = 0x00,
	.deflt = 0x55,
	.dim = 0x0d,
	.min = 0x0d,
	.min_center = 0x31,
	.center = 0x55,
	.center_max = 0x77,
	.max = 0x9a,
};
/*

Brightness Interval from Platform

0    A          B          C          D          E
+----+----------+----------+----------+----------+


Brightness Interval for LCD backlight

0'   A'         B'         C'         D'         E'
+----+----------+----------+----------+----------+

0 : tbl_plat.dim
A : tbl_plat.min
B : tbl_plat.min_center
C : tbl_plat.center
D : tbl_plat.center_max
E : tbl_plat.max

0' : tbl_XXXX.dim
A' : tbl_XXXX.min
B' : tbl_XXXX.min_center
C' : tbl_XXXX.center
D' : tbl_XXXX.center_max
E' : tbl_XXXX.max

(XXXX => normal or cabc)

We need to map the Brightness Interval from Platform to that of LCD.

*/
static int convert_brightness(struct lcd_info *lcd, const int plat_bl)
{
	struct LCD_BRIGHTNESS *tbl;
	struct LCD_BRIGHTNESS *pwm;
	int lcd_bl = plat_bl;
	int lcd_cvt_pwm;

	if (lcd->current_cabc || lcd->siop_enable) {
		tbl = &tbl_cabc;
		pwm = &pwm_cabc;
	} else{
		tbl = &tbl_normal;
		pwm = &pwm_normal;
	}

	if (plat_bl <= 0) {
		lcd_bl = tbl->off;
	} else if (plat_bl < tbl_plat.min) {
		lcd_bl = tbl->dim;
		lcd->pwm = pwm->dim;
	} else if (plat_bl >= tbl_plat.min && plat_bl < tbl_plat.min_center) {

		lcd_bl -= tbl_plat.min;
		lcd_cvt_pwm = ((pwm->min_center-pwm->min)*100)/
					(tbl_plat.min_center-tbl_plat.min);
		lcd->pwm = (lcd_cvt_pwm * lcd_bl)/100 + pwm->min;
		lcd_bl *= ((tbl->min_center-tbl->min)*100)/
					(tbl_plat.min_center-tbl_plat.min);
		lcd_bl /= 100;
		/* *100/100 is to prevent integer lcd_bl becomes 0 */
		lcd_bl += tbl->min;

	} else if (plat_bl >= tbl_plat.min_center &&
					plat_bl < tbl_plat.center) {

		lcd_bl -= tbl_plat.min_center;
		lcd_cvt_pwm = ((pwm->center - pwm->min_center)*100)/
					(tbl_plat.center-tbl_plat.min_center);
		lcd->pwm = (lcd_cvt_pwm * lcd_bl)/100 + pwm->min_center;
		lcd_bl *= ((tbl->center-tbl->min_center)*100)/
					(tbl_plat.center-tbl_plat.min_center);
		lcd_bl /= 100;
		lcd_bl += tbl->min_center;

	} else if (plat_bl >= tbl_plat.center &&
					plat_bl < tbl_plat.center_max) {

		lcd_bl -= tbl_plat.center;
		lcd_cvt_pwm = ((pwm->center_max - pwm->center)*100)/
					(tbl_plat.center_max-tbl_plat.center);
		lcd->pwm = (lcd_cvt_pwm * lcd_bl)/100 + pwm->center;
		lcd_bl *= ((tbl->center_max-tbl->center)*100)/
					(tbl_plat.center_max-tbl_plat.center);
		lcd_bl /= 100;
		lcd_bl += tbl->center;

	} else if (plat_bl >= tbl_plat.center_max &&
					plat_bl < tbl_plat.max) {

		lcd_bl -= tbl_plat.center_max;
		lcd_cvt_pwm = ((pwm->max - pwm->center_max)*100)/
					(tbl_plat.max-tbl_plat.center_max);
		lcd->pwm = (lcd_cvt_pwm * lcd_bl)/100 + pwm->center_max;
		lcd_bl *= ((tbl->max-tbl->center_max)*100)/
					(tbl_plat.max-tbl_plat.center_max);
		lcd_bl /= 100;
		lcd_bl += tbl->center_max;

	} else {
		lcd_bl = tbl->max;
		lcd->pwm = pwm->max;
	}

	/*printk("%s : platform : %dcd => brightness :
	%dcd, pwm : 0x%02x\n", __func__, plat_bl, lcd_bl, lcd->pwm);*/

	return lcd_bl;
}

#if defined(GPIO_OLED_DET)
static void oled_detection_work(struct work_struct *work)
{
	struct lcd_info *lcd =
		container_of(work, struct lcd_info, oled_detection.work);

	int oled_det_level = gpio_get_value(GPIO_OLED_DET);

	dev_info(&lcd->ld->dev, "%s, %d, %d\n",
			__func__, lcd->oled_detection_count, oled_det_level);

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

#ifdef DDI_STATUS_REG_PREVENTESD
#define DDI_DATA_LEG	0x02
#define MIPI_RESP_DCS_RD_LONG 3 /* max read data > 3*/
#define PW_MODE_REG_ADD 0x0a

static int lms501xx_read_ddi_status_reg(struct lcd_info *lcd, u8 *buf);
static void  lms501xx_reinitialize_lcd(void);
static int lms501xx_write(struct lcd_info *lcd,
			const unsigned char *seq, int len);
static int lms501xx_ldi_init(struct lcd_info *lcd);
static void check_ddi_work(struct work_struct *work)
{
	int ret;
	unsigned char	ddi_status[DDI_DATA_LEG] = {0,};
	unsigned long	ms_jiffies = msecs_to_jiffies(1000);
	struct lcd_info *lcd =
		container_of(work, struct lcd_info, check_ddi.work);

	if (!lcd->connected) {
		printk(KERN_INFO "%s, lcd is disconnected\n", __func__);
		return;
	}

	/* check ldi status - should be ldi enabled.*/
	if (lcd->ldi_enable != 1) {
		printk(KERN_INFO "%s, ldi is disabled\n", __func__);
		goto out;
	}

	ret = lms501xx_read_ddi_status_reg(lcd, ddi_status);

	if (!ret) {
		printk(KERN_INFO "%s, read failed\n", __func__);
		set_dsim_hs_clk_toggle_count(0);
		lms501xx_reinitialize_lcd();
		return;
	}

	if (0x00 != ddi_status[1]) {
		printk(KERN_INFO "%s, normal ddi_status 0x%02x\n",
			__func__, ddi_status[1]);
		if (lcd->oled_detection_count)
			set_dsim_hs_clk_toggle_count(0);

		lcd->oled_detection_count = 0;
		ms_jiffies = msecs_to_jiffies(3000);

	} else {
		printk(KERN_INFO "%s, invalid ddi_status [1]=0x%02x\n",
			__func__, ddi_status[1]);
		if (lcd->oled_detection_count < 3) {
			lcd->oled_detection_count++;
			set_dsim_hs_clk_toggle_count(15);
			ms_jiffies = msecs_to_jiffies(500);
		} else {
			set_dsim_hs_clk_toggle_count(0);
			lms501xx_reinitialize_lcd();
			return;
		}
	}
out:
	schedule_delayed_work(&lcd->check_ddi, ms_jiffies);
	return;
}
#endif
static int lms501xx_write(struct lcd_info *lcd,
			const unsigned char *seq, int len)
{
	int size;
	const unsigned char *wbuf;

/*	if (!lcd->connected)
		return 0;
*/
	mutex_lock(&lcd->lock);

	size = len;
	wbuf = seq;

	if (size == 1)
		lcd->dsim->ops->cmd_write(lcd->dsim,
				DCS_WR_NO_PARA, wbuf[0], 0);
	else if (size == 2)
		lcd->dsim->ops->cmd_write(lcd->dsim,
				DCS_WR_1_PARA, wbuf[0], wbuf[1]);
	else
		lcd->dsim->ops->cmd_write(lcd->dsim,
				DCS_LONG_WR, (unsigned int)wbuf, size);

	mutex_unlock(&lcd->lock);

	return 0;
}

static int _lms501xx_read(struct lcd_info *lcd,
			const u8 addr, u16 count, u8 *buf)
{
	int ret = 0;

	if (!lcd->connected)
		return ret;

	if (lcd->dsim->ops->cmd_dcs_read)
		ret = lcd->dsim->ops->cmd_dcs_read(lcd->dsim, addr, count, buf);

	return ret;
}

static int lms501xx_read(struct lcd_info *lcd,
			const u8 addr, u16 count, u8 *buf, u8 retry_cnt)
{
	int ret = 0;

read_retry:
	ret = _lms501xx_read(lcd, addr, count, buf);
	if (!ret) {
		if (retry_cnt) {
			printk(KERN_WARNING
				"[WARN:LCD] %s : retry cnt : %d\n",
				__func__, retry_cnt);
			retry_cnt--;
			goto read_retry;
		} else
			printk(KERN_ERR
				"[ERROR:LCD] %s : 0x%02x read failed\n",
				__func__, addr);
	}

	return ret;
}
#ifdef DDI_STATUS_REG_PREVENTESD
static void lms501xx_dsim_set_eot_mode(struct lcd_info *lcd, int enable)
{
	unsigned int reg;

	reg = readl(lcd->dsim->reg_base + S5P_DSIM_CONFIG);

	if (enable)
		reg |= 1<<28;	/*eot disable*/
	else
		reg &= ~(1<<28);

	writel(reg, lcd->dsim->reg_base + S5P_DSIM_CONFIG);

}
static int lms501xx_read_ddi_status_reg(struct lcd_info *lcd, u8 *buf)
{
	int ret, i;
	u8 read_data[MIPI_RESP_DCS_RD_LONG];

	mutex_lock(&lcd->lock);

	lms501xx_dsim_set_eot_mode(lcd, 1);

	ret = lms501xx_read(lcd, PW_MODE_REG_ADD,
		MIPI_RESP_DCS_RD_LONG, read_data, 1);

	if (!ret) {
		dev_info(&lcd->ld->dev, "read failed ddi_status_reg\n");
	} else {
		for (i = 0; i < DDI_DATA_LEG ; i++)
			buf[i] = read_data[i];
	}

	mutex_unlock(&lcd->lock);
	return ret;
}
#endif

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

static int lms501xx_gamma_ctl(struct lcd_info *lcd)
{
	SEQ_SET_BL[1] = lcd->pwm;
	lms501xx_write(lcd, SEQ_SET_BL, ARRAY_SIZE(SEQ_SET_BL));
	lms501xx_write(lcd, SEQ_SET_DISP, ARRAY_SIZE(SEQ_SET_DISP));
	return 0;
}

static int lms501xx_set_cabc(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s - %d\n", __func__, lcd->current_cabc);
	if ((lcd->current_cabc || lcd->siop_enable)
			&& (lcd->candela > tbl_normal.min))
		lms501xx_write(lcd,
			SEQ_SET_CABC_ON, ARRAY_SIZE(SEQ_SET_CABC_ON));
	else
		lms501xx_write(lcd,
			SEQ_SET_CABC_OFF, ARRAY_SIZE(SEQ_SET_CABC_OFF));

	mdelay(5);

	return ret;
}

static int update_brightness(struct lcd_info *lcd, u8 force)
{
	u32 brightness;

	mutex_lock(&lcd->bl_lock);

	brightness = lcd->bd->props.brightness;

	if (unlikely(!lcd->auto_brightness && brightness > MAX_BRIGHTNESS))
		brightness = MAX_BRIGHTNESS;

	lcd->bl = get_backlight_level_from_brightness(brightness);
	lcd->candela =
	convert_brightness(lcd, candela_table[lcd->bl]);

	if ((force) || ((lcd->ldi_enable) && (lcd->current_bl != lcd->bl))) {

		lms501xx_gamma_ctl(lcd);
		lms501xx_set_cabc(lcd);

		lcd->current_bl = lcd->bl;

		dev_info(&lcd->ld->dev, "brightness=%d, bl=%d, candela=%d\n",
				brightness, lcd->bl, lcd->candela);
	}

	mutex_unlock(&lcd->bl_lock);

	return 0;
}

static int lms501xx_ldi_init(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lms501xx_write(lcd, SEQ_SET_EXTC, ARRAY_SIZE(SEQ_SET_EXTC));
	mdelay(5);
	lms501xx_write(lcd, SEQ_SET_MIPI_DSI, ARRAY_SIZE(SEQ_SET_MIPI_DSI));
	lms501xx_write(lcd, SEQ_SET_GIP, ARRAY_SIZE(SEQ_SET_GIP));
	lms501xx_write(lcd, SEQ_SET_POWER, ARRAY_SIZE(SEQ_SET_POWER));
	mdelay(5);
	lms501xx_write(lcd, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	msleep(160);
	lms501xx_write(lcd, SEQ_SET_RGB, ARRAY_SIZE(SEQ_SET_RGB));
	lms501xx_write(lcd, SEQ_SET_CYC, ARRAY_SIZE(SEQ_SET_CYC));
	lms501xx_write(lcd, SEQ_SET_VCOM, ARRAY_SIZE(SEQ_SET_VCOM));
	lms501xx_write(lcd, SEQ_SET_PTBA, ARRAY_SIZE(SEQ_SET_PTBA));
	lms501xx_write(lcd, SEQ_SET_PANEL, ARRAY_SIZE(SEQ_SET_PANEL));
	lms501xx_write(lcd, SEQ_SET_DGC, ARRAY_SIZE(SEQ_SET_DGC));
	lms501xx_write(lcd, SEQ_SET_STBA, ARRAY_SIZE(SEQ_SET_STBA));
	lms501xx_write(lcd, SEQ_SET_EQ, ARRAY_SIZE(SEQ_SET_EQ));
	lms501xx_write(lcd, SEQ_SET_VCOM_POWER, ARRAY_SIZE(SEQ_SET_VCOM_POWER));
	lms501xx_write(lcd, SEQ_SET_ECO, ARRAY_SIZE(SEQ_SET_ECO));
	lms501xx_write(lcd, SEQ_SET_GAMMA, ARRAY_SIZE(SEQ_SET_GAMMA));
	lms501xx_write(lcd, SEQ_SET_CABC_PWM, ARRAY_SIZE(SEQ_SET_CABC_PWM));

	return ret;
}

static int lms501xx_ldi_enable(struct lcd_info *lcd)
{
	int ret = 0;

	lms501xx_write(lcd, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
	msleep(100);

	return ret;
}

static int lms501xx_ldi_disable(struct lcd_info *lcd)
{
	int ret = 0;

	lms501xx_write(lcd, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	lms501xx_write(lcd, SEQ_SLEEP_IN, ARRAY_SIZE(SEQ_SLEEP_IN));

	return ret;
}

static int lms501xx_power_on(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	ret = lms501xx_ldi_init(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}

	ret = lms501xx_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}

	lcd->ldi_enable = 1;

	update_brightness(lcd, 1);
err:
	return ret;
}

static int lms501xx_power_off(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->ldi_enable = 0;

	ret = lms501xx_ldi_disable(lcd);

	msleep(135);

	return ret;
}

static int lms501xx_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = lms501xx_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = lms501xx_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int lms501xx_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return lms501xx_power(lcd, power);
}

static int lms501xx_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

static int lms501xx_check_fb(struct lcd_device *ld, struct fb_info *fb)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	dev_info(&lcd->ld->dev, "%s, fb%d\n", __func__, fb->node);

	return 0;
}

static int lms501xx_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	int brightness = bd->props.brightness;
	struct lcd_info *lcd = bl_get_data(bd);

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

static int lms501xx_get_brightness(struct backlight_device *bd)
{
	struct lcd_info *lcd = bl_get_data(bd);

	return candela_table[lcd->bl];
}

static struct lcd_ops lms501xx_lcd_ops = {
	.set_power = lms501xx_set_power,
	.get_power = lms501xx_get_power,
	.check_fb  = lms501xx_check_fb,
};

static const struct backlight_ops lms501xx_backlight_ops  = {
	.get_brightness = lms501xx_get_brightness,
	.update_status = lms501xx_set_brightness,
};

static ssize_t power_reduce_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->current_cabc);
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
		if (lcd->current_cabc != value) {
			dev_info(dev, "%s - %d, %d\n",
				 __func__, lcd->current_cabc, value);
			mutex_lock(&lcd->bl_lock);
			lcd->current_cabc = value;
			mutex_unlock(&lcd->bl_lock);
			if (lcd->ldi_enable)
				update_brightness(lcd, 1);
		}
	}
	return size;
}

static DEVICE_ATTR(power_reduce, 0664, power_reduce_show, power_reduce_store);

static ssize_t siop_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->siop_enable);
	strcpy(buf, temp);

	return strlen(buf);
}

static ssize_t siop_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = strict_strtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (lcd->siop_enable != value) {
			dev_info(dev, "%s - %d, %d\n",
				__func__, lcd->siop_enable, value);
			mutex_lock(&lcd->bl_lock);
			lcd->siop_enable = value;
			mutex_unlock(&lcd->bl_lock);
			if (lcd->ldi_enable)
				update_brightness(lcd, 1);
		}
	}
	return size;
}

static DEVICE_ATTR(siop_enable, 0664, siop_enable_show, siop_enable_store);

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[16];
	sprintf(temp, "SMD_LMS501KF10\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0444, lcd_type_show, NULL);

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
			dev_info(dev, "%s - %d, %d\n",
				 __func__, lcd->auto_brightness, value);
			mutex_lock(&lcd->bl_lock);
			lcd->auto_brightness = value;
			mutex_unlock(&lcd->bl_lock);
			if (lcd->ldi_enable)
				update_brightness(lcd, 0);
		}
	}
	return size;
}

static void lms501xx_dsim_set_lp_mode(struct lcd_info *lcd)
{
	unsigned int reg = readl(lcd->dsim->reg_base + S5P_DSIM_ESCMODE);

	reg &= ~DSIM_TRANSFER_BYLCDC;
	reg |= DSIM_TRANSFER_BYCPU;
	writel(reg, lcd->dsim->reg_base + S5P_DSIM_ESCMODE);

	reg = (readl(lcd->dsim->reg_base + S5P_DSIM_CLKCTRL)) & ~(0XFFFF);
	reg |= DSIM_ESC_PRESCALER(0x0003);
	writel(reg, lcd->dsim->reg_base + S5P_DSIM_CLKCTRL);
}
static DEVICE_ATTR(auto_brightness, 0644,
		auto_brightness_show, auto_brightness_store);

#ifdef CONFIG_HAS_EARLYSUSPEND
struct lcd_info *g_lcd;

void lms501xx_early_suspend(void)
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
	lms501xx_power(lcd, FB_BLANK_POWERDOWN);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

#ifdef DDI_STATUS_REG_PREVENTESD
	if (lcd->connected) {
		bool ret;
		ret = cancel_delayed_work(&lcd->check_ddi);
		if (ret)
			printk(KERN_INFO "%s, success - cancel delayed work\n",
				__func__);
	}
#endif
	return ;
}

void lms501xx_late_resume(void)
{
	struct lcd_info *lcd = g_lcd;

	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	lms501xx_dsim_set_lp_mode(lcd);
	lms501xx_power(lcd, FB_BLANK_UNBLANK);
#if defined(GPIO_OLED_DET)
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
	enable_irq(lcd->irq);
#endif
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	set_dsim_lcd_enabled(1);
#ifdef DDI_STATUS_REG_PREVENTESD
	if (lcd->connected)
		schedule_delayed_work(&lcd->check_ddi, msecs_to_jiffies(3000));

#endif

	return ;
}
#ifdef DDI_STATUS_REG_PREVENTESD
static void  lms501xx_reinitialize_lcd(void)
{
	lms501xx_early_suspend();
	s5p_dsim_early_suspend();
	msleep(20);
	s5p_dsim_late_resume();

	msleep(20);
	lms501xx_late_resume();
	printk(KERN_INFO "%s, re-initialize LCD - Done\n", __func__);
}
#endif
#endif

static void lms501xx_read_id(struct lcd_info *lcd, u8 *buf)
{
	int ret = 0, i;
	u8 read_data[LDI_ID_LEN];

	lms501xx_dsim_set_eot_mode(lcd, 1);

	mutex_lock(&lcd->lock);
	ret = lms501xx_read(lcd, LDI_ID_REG, LDI_ID_LEN, read_data, 3);
	mutex_unlock(&lcd->lock);

	if (!ret) {
		lcd->connected = 0;
		dev_info(&lcd->ld->dev, "panel is not connected well\n");
	} else {
		for (i = 0; i < LDI_ID_LEN ; i++)
			buf[i] = read_data[i];
		dev_info(&lcd->ld->dev, "%s - module's manufacturer ID : %02x\n",
			__func__, read_data[1]);
	}
}

static int lms501xx_probe(struct device *dev)
{
	int ret = 0;
	struct lcd_info *lcd;

	lcd = kzalloc(sizeof(struct lcd_info), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	g_lcd = lcd;

	lcd->ld = lcd_device_register("panel", dev, lcd, &lms501xx_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("panel",
		 dev, lcd, &lms501xx_backlight_ops, NULL);
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
	lcd->siop_enable = 0;
	lcd->current_cabc = 0;

	lcd->power = FB_BLANK_UNBLANK;
	lcd->ldi_enable = 1;
	lcd->connected = 1;
	lcd->auto_brightness = 0;

	ret = device_create_file(&lcd->ld->dev, &dev_attr_power_reduce);
	if (ret < 0)
		dev_err(&lcd->ld->dev,
			 "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_siop_enable);
	if (ret < 0)
		dev_err(&lcd->ld->dev,
			"failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev,
			 "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->bd->dev, &dev_attr_auto_brightness);
	if (ret < 0)
		dev_err(&lcd->ld->dev,
			 "failed to add sysfs entries, %d\n", __LINE__);

	dev_set_drvdata(dev, lcd);

	mutex_init(&lcd->lock);
	mutex_init(&lcd->bl_lock);

	dev_info(&lcd->ld->dev, "lms501xx lcd panel driver has been probed.\n");

	update_brightness(lcd, 1);

	lms501xx_read_id(lcd, lcd->id);

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

#ifdef DDI_STATUS_REG_PREVENTESD
	if (lcd->connected) {
		INIT_DELAYED_WORK(&lcd->check_ddi, check_ddi_work);
		schedule_delayed_work(&lcd->check_ddi, msecs_to_jiffies(20000));
	}
#endif
	lcd_early_suspend = lms501xx_early_suspend;
	lcd_late_resume = lms501xx_late_resume;

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

static int __devexit lms501xx_remove(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	lms501xx_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	backlight_device_unregister(lcd->bd);
	kfree(lcd);

	return 0;
}

/* Power down all displays on reboot, poweroff or halt. */
static void lms501xx_shutdown(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lms501xx_power(lcd, FB_BLANK_POWERDOWN);
}

static struct mipi_lcd_driver lms501xx_mipi_driver = {
	.name = "lms501xx",
	.probe			= lms501xx_probe,
	.remove			= __devexit_p(lms501xx_remove),
	.shutdown		= lms501xx_shutdown,
};

static int lms501xx_init(void)
{
	return s5p_dsim_register_lcd_driver(&lms501xx_mipi_driver);
}

static void lms501xx_exit(void)
{
	return;
}

module_init(lms501xx_init);
module_exit(lms501xx_exit);

MODULE_DESCRIPTION("MIPI-DSI LMS501XX WVGA (480x800) Panel Driver");
MODULE_LICENSE("GPL");
