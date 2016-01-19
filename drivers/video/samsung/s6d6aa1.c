/* linux/drivers/video/samsung/s6d6aa1.c
 *
 * MIPI-DSI based AMS529HA01 Super Clear LCD panel driver.
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

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255
#define DEFAULT_BRIGHTNESS		160


struct lcd_info {
	unsigned int			bl;
	unsigned int			current_bl;
	unsigned int			auto_brightness;
	unsigned int			ldi_enable;
	unsigned int			power;
	struct mutex			lock;
	struct mutex			bl_lock;

	struct device			*dev;
	struct lcd_device		*ld;
	struct backlight_device		*bd;
	struct lcd_platform_data	*lcd_pd;
	struct early_suspend		early_suspend;

	unsigned int			irq;
	unsigned int			connected;
#if defined(GPIO_VGH_DET)
	struct delayed_work		vgh_detection;
	unsigned int			vgh_detection_count;

#endif
	struct dsim_global		*dsim;
};

static const unsigned char SEQ_PASSWD1[] = {
	0xF0,
	0x5A, 0x5A
};

static const unsigned char SEQ_PASSWD2[] = {
	0xF1,
	0x5A, 0x5A
};

static const unsigned char SEQ_PANELCTL[] = {
	0xF6,
	0x0B, 0x11, 0x0F, 0x25, 0x0A, 0x00, 0x13, 0x22, 0x1B, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x24, 0x3F, 0x3C, 0x51,
};

static const unsigned char SEQ_SONY_IP_SET1[] = {
	0xC4,
	0x7C, 0xE6, 0x7C, 0xE6, 0x7C, 0xE6, 0x7C, 0x7C,
	0x05, 0x0F, 0x1F, 0x01, 0x00, 0x00,
};

static const unsigned char SEQ_SONY_IP_SET2[] = {
	0xC5,
	0x80, 0x80, 0x80, 0x41, 0x43, 0x34, 0x80, 0x80,
	0x01, 0xFF, 0xA5, 0x58, 0x50,
};

/* Should be removed when panel nvm is updated */
static const unsigned char SEQ_PGAMMACTL[] = {
	0xFA,	0x9C,	0xBF,	0x1A,	0xD6,	0xE3,	0xE3,	0x1B,
	0xDA,	0x9B,	0x16,	0x51,	0x12,	0x15,	0xD9,	0x9B,
	0x1A,	0xDD,	0x62,	0x2D,	0x79,	0x6A,	0x2C,	0x7F,
	0x11,	0x09,	0x53,	0x91,	0x09,	0x08,	0xCA,	0x06,
	0x47,	0x09,	0x0D,	0x92,	0x55,	0x16,	0xD9,	0x5C,
	0x5E,	0x1E,	0x01,	0x4F,	0x3F,	0x97,	0x53,	0x21,
	0x62,	0x5A,	0x1A,	0x1C,	0x1A,	0xD7,	0x99,	0x5D,
	0x21,	0x25,	0x66,	0xA9,	0xAC,	0x2A,	0xEA,	0x8F,
};

static const unsigned char SEQ_NGAMMACTL[] = {
	0xFB,	0x9C,	0xBF,	0x1A,	0xD6,	0xE3,	0xE3,	0x1B,
	0xDA,	0x9B,	0x16,	0x51,	0x12,	0x15,	0xD9,	0x9B,
	0x1A,	0xDD,	0x62,	0x2D,	0x79,	0x6A,	0x2C,	0x7F,
	0x11,	0x09,	0x53,	0x91,	0x09,	0x08,	0xCA,	0x06,
	0x47,	0x09,	0x0D,	0x92,	0x55,	0x16,	0xD9,	0x5C,
	0x5E,	0x1E,	0x01,	0x4F,	0x3F,	0x97,	0x53,	0x21,
	0x62,	0x5A,	0x1A,	0x1C,	0x1A,	0xD7,	0x99,	0x5D,
	0x21,	0x25,	0x66,	0xA9,	0xAC,	0x2A,	0xEA,	0x8F,
};

static const unsigned char SEQ_PASSWD1_DISABLE[] = {
	0xF0,
	0xA5, 0xA5
};

static const unsigned char SEQ_PASSWD2_DISABLE[] = {
	0xF1,
	0xA5, 0xA5
};

static const unsigned char SEQ_SLPOUT[] = {
	0x11,
	0x00,
	0x00
};

static const unsigned char SEQ_DSCTL[] = {
	0x36,
	0x00,
	0x00
};

static const unsigned char SEQ_WRCTRLD[] = {
	0x53,
	0x2C,
	0x00
};

static const unsigned char SEQ_WRCABC[] = {
	0x55,
	0x01,
	0x00
};

static const unsigned char SEQ_WRCABC_OUTDOOR[] = {
	0x55,
	0x04,
	0x00
};

static const unsigned char SEQ_DISPON[] = {
	0x29,
	0x00,
	0x00
};

static const unsigned char SEQ_DISPOFF[] = {
	0x28,
	0x00,
	0x00
};

static const unsigned char SEQ_SLPIN[] = {
	0x10,
	0x00,
	0x00
};

static unsigned char SEQ_WRDISBV_CTL[] = {
	0x51,
	0x78,
	0x00
};

static unsigned char TRANS_BRIGHTNESS[] = {
	0,	1,	1,	2,	2,	3,	3,	4,
	4,	5,	5,	6,	6,	7,	7,	8,
	8,	9,	9,	10,	11,	12,	13,	14,
	15,	16,	17,	18,	19,	20,	21,	23,
	24,	25,	26,	27,	28,	29,	30,	31,
	32,	33,	34,	35,	36,	37,	38,	39,
	40,	41,	42,	43,	44,	45,	46,	48,
	49,	50,	51,	52,	53,	54,	55,	56,
	57,	58,	59,	60,	61,	62,	63,	64,
	65,	66,	67,	68,	69,	70,	72,	73,
	74,	75,	76,	77,	78,	79,	80,	81,
	82,	83,	84,	85,	86,	87,	88,	89,
	90,	91,	92,	93,	94,	95,	97,	98,
	99,	100,	101,	102,	103,	104,	105,	106,
	107,	108,	109,	110,	111,	112,	113,	114,
	115,	116,	117,	118,	119,	121,	122,	123,
	124,	125,	126,	127,	128,	129,	130,	131,
	132,	133,	134,	135,	136,	137,	138,	139,
	140,	141,	142,	143,	144,	146,	147,	148,
	149,	150,	151,	152,	153,	154,	155,	156,
	157,	158,	159,	160,	161,	162,	163,	164,
	165,	166,	167,	168,	170,	171,	172,	173,
	174,	175,	176,	177,	178,	179,	180,	181,
	182,	183,	184,	185,	186,	187,	188,	189,
	190,	191,	192,	193,	195,	196,	197,	198,
	199,	200,	201,	202,	203,	204,	205,	206,
	207,	208,	209,	210,	211,	212,	213,	214,
	215,	216,	217,	219,	220,	221,	222,	223,
	224,	225,	226,	227,	228,	229,	230,	231,
	232,	233,	234,	235,	236,	237,	238,	239,
	240,	241,	242,	244,	245,	246,	247,	248,
	249,	250,	251,	252,	253,	254,	255,	255,
};

extern void (*lcd_early_suspend)(void);
extern void (*lcd_late_resume)(void);

#if defined(GPIO_VGH_DET)
static void esd_reset_lcd(struct lcd_info *lcd)
{
	dev_info(&lcd->ld->dev, "++%s\n", __func__);
	if (lcd_early_suspend)
		lcd_early_suspend();
	lcd->dsim->ops->suspend();

	lcd->dsim->ops->resume();
	if (lcd_late_resume)
		lcd_late_resume();
	dev_info(&lcd->ld->dev, "--%s\n", __func__);
}

static void vgh_detection_work(struct work_struct *work)
{
	struct lcd_info *lcd =
		container_of(work, struct lcd_info, vgh_detection.work);

	int oled_det_level = gpio_get_value(GPIO_VGH_DET);

	dev_info(&lcd->ld->dev, "%s, %d, %d\n", __func__, lcd->vgh_detection_count, oled_det_level);
	if (!oled_det_level)
		esd_reset_lcd(lcd);
}

static irqreturn_t vgh_detection_int(int irq, void *_lcd)
{
	struct lcd_info *lcd = _lcd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->vgh_detection_count = 0;
	schedule_delayed_work(&lcd->vgh_detection, HZ/16);

	return IRQ_HANDLED;
}
#endif


static int _s6d6aa1_write(struct lcd_info *lcd, const unsigned char *seq, int len)
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

static int s6d6aa1_write(struct lcd_info *lcd, const unsigned char *seq, int len)
{
	int ret = 0;
	int retry_cnt = 1;

retry:
	ret = _s6d6aa1_write(lcd, seq, len);
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

static int _s6d6aa1_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf)
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

static int s6d6aa1_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf, u8 retry_cnt)
{
	int ret = 0;

read_retry:
	ret = _s6d6aa1_read(lcd, addr, count, buf);
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

static int get_backlight_level_from_brightness(int brightness)
{
	int backlightlevel;

	backlightlevel = TRANS_BRIGHTNESS[brightness];

	return backlightlevel;
}

static int update_brightness(struct lcd_info *lcd, u8 force)
{
	u32 brightness;

	mutex_lock(&lcd->bl_lock);

	brightness = lcd->bd->props.brightness;

	lcd->bl = get_backlight_level_from_brightness(brightness);

	if ((force) || ((lcd->ldi_enable) && (lcd->current_bl != lcd->bl))) {

		lcd->current_bl = lcd->bl;
		SEQ_WRDISBV_CTL[1] = lcd->bl;
		s6d6aa1_write(lcd, SEQ_WRDISBV_CTL, \
			ARRAY_SIZE(SEQ_WRDISBV_CTL));

		dev_info(&lcd->ld->dev, "brightness=%d, bl=%d\n", brightness, lcd->bl);
	}

	mutex_unlock(&lcd->bl_lock);

	return 0;
}

static int s6d6aa1_ldi_init(struct lcd_info *lcd)
{
	int ret = 0;

	usleep_range(20000, 20000);

	s6d6aa1_write(lcd, SEQ_SLPOUT, ARRAY_SIZE(SEQ_SLPOUT));

	usleep_range(145000, 145000);

	s6d6aa1_write(lcd, SEQ_PASSWD1, ARRAY_SIZE(SEQ_PASSWD1));
	s6d6aa1_write(lcd, SEQ_PASSWD2, ARRAY_SIZE(SEQ_PASSWD2));
	s6d6aa1_write(lcd, SEQ_PANELCTL, ARRAY_SIZE(SEQ_PANELCTL));
	s6d6aa1_write(lcd, SEQ_SONY_IP_SET1, ARRAY_SIZE(SEQ_SONY_IP_SET1));
	s6d6aa1_write(lcd, SEQ_SONY_IP_SET2, ARRAY_SIZE(SEQ_SONY_IP_SET2));
	s6d6aa1_write(lcd, SEQ_PGAMMACTL, ARRAY_SIZE(SEQ_PGAMMACTL));
	s6d6aa1_write(lcd, SEQ_NGAMMACTL, ARRAY_SIZE(SEQ_NGAMMACTL));
	s6d6aa1_write(lcd, SEQ_PASSWD1_DISABLE, ARRAY_SIZE(SEQ_PASSWD1_DISABLE));
	s6d6aa1_write(lcd, SEQ_PASSWD2_DISABLE, ARRAY_SIZE(SEQ_PASSWD2_DISABLE));
	s6d6aa1_write(lcd, SEQ_DSCTL, ARRAY_SIZE(SEQ_DSCTL));
	s6d6aa1_write(lcd, SEQ_WRCTRLD, ARRAY_SIZE(SEQ_WRCTRLD));
	s6d6aa1_write(lcd, SEQ_WRCABC, ARRAY_SIZE(SEQ_WRCABC));


	return ret;
}

static int s6d6aa1_ldi_enable(struct lcd_info *lcd)
{
	int ret = 0;

	s6d6aa1_write(lcd, SEQ_DISPON, ARRAY_SIZE(SEQ_DISPON));

	return ret;
}

static int s6d6aa1_ldi_disable(struct lcd_info *lcd)
{
	int ret = 0;

	s6d6aa1_write(lcd, SEQ_DISPOFF, ARRAY_SIZE(SEQ_DISPOFF));
	s6d6aa1_write(lcd, SEQ_SLPIN, ARRAY_SIZE(SEQ_SLPIN));

	return ret;
}

static int s6d6aa1_power_on(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	/* dev_info(&lcd->ld->dev, "%s\n", __func__); */

	ret = s6d6aa1_ldi_init(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}


	ret = s6d6aa1_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}

	lcd->ldi_enable = 1;

	update_brightness(lcd, 1);
err:
	return ret;
}

static int s6d6aa1_power_off(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->ldi_enable = 0;

	ret = s6d6aa1_ldi_disable(lcd);

	msleep(135);

	return ret;
}

static int s6d6aa1_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = s6d6aa1_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = s6d6aa1_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int s6d6aa1_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return s6d6aa1_power(lcd, power);
}

  static int s6d6aa1_check_fb(struct lcd_device *ld, struct fb_info *fb)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	dev_info(&lcd->ld->dev, "%s, fb%d\n", __func__, fb->node);

	return 0;
}

static int s6d6aa1_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

static int s6d6aa1_set_brightness(struct backlight_device *bd)
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

static int s6d6aa1_get_brightness(struct backlight_device *bd)
{
	struct lcd_info *lcd = bl_get_data(bd);

	return lcd->bl;
}

static struct lcd_ops panel_lcd_ops = {
	.set_power = s6d6aa1_set_power,
	.get_power = s6d6aa1_get_power,
	.check_fb  = s6d6aa1_check_fb,
};

static const struct backlight_ops panel_backlight_ops  = {
	.get_brightness = s6d6aa1_get_brightness,
	.update_status = s6d6aa1_set_brightness,
};

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[15];

	sprintf(temp, "JDI_ACX445BLN\n");

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
		lcd->auto_brightness = value;
		if (lcd->auto_brightness < 4)
			s6d6aa1_write(lcd, SEQ_WRCABC, ARRAY_SIZE(SEQ_WRCABC));
		else
			s6d6aa1_write(lcd, SEQ_WRCABC_OUTDOOR, ARRAY_SIZE(SEQ_WRCABC_OUTDOOR));
	}
	return size;
}

static DEVICE_ATTR(auto_brightness, 0644, auto_brightness_show, auto_brightness_store);

#ifdef CONFIG_HAS_EARLYSUSPEND
struct lcd_info *g_lcd;

void s6d6aa1_early_suspend(void)
{
	struct lcd_info *lcd = g_lcd;

	set_dsim_lcd_enabled(0);

	dev_info(&lcd->ld->dev, "+%s\n", __func__);

#if defined(GPIO_VGH_DET)
	disable_irq(lcd->irq);
	gpio_request(GPIO_VGH_DET, "VGH_DET");
	s3c_gpio_cfgpin(GPIO_VGH_DET, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_VGH_DET, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_VGH_DET, GPIO_LEVEL_LOW);
	gpio_free(GPIO_VGH_DET);
#endif
	s6d6aa1_power(lcd, FB_BLANK_POWERDOWN);

	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

void s6d6aa1_late_resume(void)
{
	struct lcd_info *lcd = g_lcd;

	dev_info(&lcd->ld->dev, "+%s\n", __func__);

	s6d6aa1_power(lcd, FB_BLANK_UNBLANK);
#if defined(GPIO_VGH_DET)
	s3c_gpio_cfgpin(GPIO_VGH_DET, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_VGH_DET, S3C_GPIO_PULL_NONE);
	enable_irq(lcd->irq);
#endif
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	set_dsim_lcd_enabled(1);

	return ;
}
#endif

static int s6d6aa1_probe(struct device *dev)
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

	lcd->ld = lcd_device_register("panel", dev, lcd, &panel_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("panel", dev, lcd, &panel_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		pr_err("failed to register backlight device\n");
		ret = PTR_ERR(lcd->bd);
		goto out_free_backlight;
	}

	lcd->dev = dev;
	lcd->dsim = (struct dsim_global *)dev_get_drvdata(dev->parent);
	lcd->bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd->bd->props.brightness = DEFAULT_BRIGHTNESS;
	lcd->bl = 0;
	lcd->current_bl = lcd->bl;

	lcd->power = FB_BLANK_UNBLANK;
	lcd->ldi_enable = 1;
	lcd->connected = 1;

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->bd->dev, &dev_attr_auto_brightness);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	dev_set_drvdata(dev, lcd);

	mutex_init(&lcd->lock);
	mutex_init(&lcd->bl_lock);

	dev_info(&lcd->ld->dev, "s6d6aa1 lcd panel driver has been probed.\n");

#if defined(GPIO_VGH_DET)
	if (lcd->connected) {
		INIT_DELAYED_WORK(&lcd->vgh_detection, vgh_detection_work);

		lcd->irq = gpio_to_irq(GPIO_VGH_DET);

		s3c_gpio_cfgpin(GPIO_VGH_DET, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(GPIO_VGH_DET, S3C_GPIO_PULL_NONE);
		if (request_irq(lcd->irq, vgh_detection_int,
			IRQF_TRIGGER_FALLING, "vgh_detection", lcd))
			pr_err("failed to reqeust irq. %d\n", lcd->irq);
		}
#endif

	lcd_early_suspend = s6d6aa1_early_suspend;
	lcd_late_resume = s6d6aa1_late_resume;

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

static int __devexit s6d6aa1_remove(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	s6d6aa1_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	backlight_device_unregister(lcd->bd);
	kfree(lcd);

	return 0;
}

/* Power down all displays on reboot, poweroff or halt. */
static void s6d6aa1_shutdown(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	s6d6aa1_power(lcd, FB_BLANK_POWERDOWN);
}

static struct mipi_lcd_driver s6d6aa1_mipi_driver = {
	.name = "s6d6aa1",
	.probe			= s6d6aa1_probe,
	.remove			= __devexit_p(s6d6aa1_remove),
	.shutdown		= s6d6aa1_shutdown,
};

static int s6d6aa1_init(void)
{
	return s5p_dsim_register_lcd_driver(&s6d6aa1_mipi_driver);
}

static void s6d6aa1_exit(void)
{
	return;
}

module_init(s6d6aa1_init);
module_exit(s6d6aa1_exit);

MODULE_DESCRIPTION("MIPI-DSI S6D6AA1:ACX445BLN SCLCD (720x1280) Panel Driver");
MODULE_LICENSE("GPL");

