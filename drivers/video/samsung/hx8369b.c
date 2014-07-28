/* linux/drivers/video/samsung/hx8369b.c
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

	struct dsim_global		*dsim;
};


/* Enable extention command */
static const unsigned char SEQ_ENABLE_EXTCMD[] = {
	0xB9,
	0xFF, 0x83, 0x69
};

/* MIPI command */
static const unsigned char SEQ_MIPICMD[] = {
	0xBA,
	0x31, 0x00, 0x16, 0xCA, 0xB1, 0x0A, 0x00, 0x10, 0x28, 0x02,
	0x21, 0x21, 0x9A, 0x1A, 0x8F
};

/* Interface pixel format */
static const unsigned char SEQ_PIXEL_FORMAT[] = {
	0x3A,
	0x70, 0x00 /* meaningless 0x00 added */
};

/* GOA Timing Control */
static const unsigned char SEQ_GOA_TIMING[] = {
	0xD5,
	0x00, 0x00, 0x08, 0x00, 0x0A, 0x00, 0x00, 0x10, 0x01, 0x00,
	0x00, 0x00, 0x01, 0x49, 0x37, 0x00, 0x00, 0x0A, 0x0A, 0x0B,
	0x47, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x26, 0x00, 0x00, 0x91, 0x13,
	0x35, 0x57, 0x75, 0x18, 0x00, 0x00, 0x00, 0x86, 0x64, 0x42,
	0x20, 0x00, 0x49, 0x00, 0x00, 0x00, 0x90, 0x02, 0x24, 0x46,
	0x64, 0x08, 0x00, 0x00, 0x00, 0x87, 0x75, 0x53, 0x31, 0x11,
	0x59, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F,
	0x00, 0x0F, 0xFF, 0xFF, 0x0F, 0x00, 0x0F, 0xFF, 0xFF, 0x00,
	0x80, 0x5A
};

/* Power Control */
static const unsigned char SEQ_POWER_CTL[] = {
	0xB1,
	0x0C, 0x83, 0x77, 0x00, 0x0F, 0x0F, 0x18, 0x18, 0x0C, 0x02
};

/* Power Control */
static const unsigned char SEQ_POWER_CTL_DEGREE_0[] = {
	0xB1,
	0x11, 0x83, 0x78, 0x00, 0x0F, 0x0F, 0x18, 0x18, 0x0C, 0x02
};

/* Set Display */
static const unsigned char SEQ_DISP_CTL[] = {
	0xB2,
	0x00, 0x70
};

/* RGB Setting */
static const unsigned char SEQ_RGB_SETTING[] = {
	0xB3,
	0x83, 0x00, 0x31, 0x03
};

/* Display Inversion Setting */
static const unsigned char SEQ_DISP_INVERSION[] = {
	0xB4,
	0x02, 0x00 /* meaningless 0x00 added */
};

/* VCOM Voltage */
static const unsigned char SEQ_VCOM_VOLTAGE[] = {
	0xB6,
	0xA0, 0xA0
};

/* SET CLOCK */
static const unsigned char SEQ_SET_CLOCK[] = {
	0xCB,
	0x6D, 0x00 /* meaningless 0x00 added */
};

/* Display direction */
static const unsigned char SEQ_DISP_DIRECTION[] = {
	0xCC,
	0x02, 0x00 /* meaningless 0x00 added */
};

/* Internal used */
static const unsigned char SEQ_SOURCE_TIMING[] = {
	0xC6,
	0x41, 0xFF, 0x7A
};

/* CABC Control */
static const unsigned char SEQ_CABC_CTL[] = {
	0xEA,
	0x72, 0x00 /* meaningless 0x00 added */
};

/* Source EQ */
static const unsigned char SEQ_SOURCE_EQ[] = {
	0xE3,
	0x07, 0x0F, 0x07, 0x0F
};

/* Source EQ */
static const unsigned char SEQ_SOURCE_EQ_DEGREE_0[] = {
	0xE3,
	0x07, 0x0F, 0x07, 0x0F
};

/* Set Source Option */
static const unsigned char SEQ_SOURCE_OPTION[] = {
	0xC0,
	0x73, 0x50, 0x00, 0x34, 0xC4, 0x09
};

/* Digital Gamma */
static const unsigned char SEQ_DIGITAL_GAMMA[] = {
	0xC1,
	0x00, 0x00 /* meaningless 0x00 added */
};

/* Gamma Setting */
static const unsigned char SEQ_GAMMA_SETTING[] = {
	0xE0,
	0x00, 0x07, 0x0C, 0x30, 0x32, 0x3F, 0x1C, 0x3A, 0x08, 0x0D,
	0x10, 0x14, 0x16, 0x14, 0x15, 0x0E, 0x12, 0x00, 0x07, 0x0C,
	0x30, 0x32, 0x3F, 0x1C, 0x3A, 0x08, 0x0D, 0x10, 0x14, 0x16,
	0x14, 0x15, 0x0E, 0x12, 0x01
};

/* Sleep Out */
static const unsigned char SEQ_SLEEP_OUT[] = {
	0x11,
	0x00, 0x00 /* meaningless 0x00, 0x00 added */
};

/* Display On */
static const unsigned char SEQ_DISPLAY_ON[] = {
	0x29,
	0x00, 0x00 /* meaningless 0x00, 0x00 added */
};



/* BGP voltage */
static const unsigned char SEQ_BGP_VOLT[] = {
	0xB5,
	0x0B, 0x0B, 0x24
};

/* Memory access control */
static const unsigned char SEQ_MEM_ACCESS_CTL[] = {
	0x36,
	0xC0, 0x00
};

/* Display off */
static const unsigned char SEQ_DISPLAY_OFF[] = {
	0x28,
	0x00, 0x00
};

/* Sleep in */
static const unsigned char SEQ_SLEEP_IN[] = {
	0x10,
	0x00, 0x00
};

extern void (*lcd_early_suspend)(void);
extern void (*lcd_late_resume)(void);

static int _hx8369b_write(struct lcd_info *lcd, const unsigned char *seq, int len)
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

static int hx8369b_write(struct lcd_info *lcd, const unsigned char *seq, int len)
{
	int ret = 0;
	int retry_cnt = 1;

retry:
	ret = _hx8369b_write(lcd, seq, len);
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

static int _hx8369b_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf)
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

static int hx8369b_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf, u8 retry_cnt)
{
	int ret = 0;

read_retry:
	ret = _hx8369b_read(lcd, addr, count, buf);
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

static int hx8369b_ldi_init(struct lcd_info *lcd)
{
	int ret = 0;

	hx8369b_write(lcd, SEQ_ENABLE_EXTCMD, ARRAY_SIZE(SEQ_ENABLE_EXTCMD));
	hx8369b_write(lcd, SEQ_MIPICMD, ARRAY_SIZE(SEQ_MIPICMD));
	hx8369b_write(lcd, SEQ_PIXEL_FORMAT, ARRAY_SIZE(SEQ_PIXEL_FORMAT));
	hx8369b_write(lcd, SEQ_GOA_TIMING, ARRAY_SIZE(SEQ_GOA_TIMING));
	hx8369b_write(lcd, SEQ_POWER_CTL, ARRAY_SIZE(SEQ_POWER_CTL));
	hx8369b_write(lcd, SEQ_DISP_CTL, ARRAY_SIZE(SEQ_DISP_CTL));
	hx8369b_write(lcd, SEQ_RGB_SETTING, ARRAY_SIZE(SEQ_RGB_SETTING));
	hx8369b_write(lcd, SEQ_DISP_INVERSION, ARRAY_SIZE(SEQ_DISP_INVERSION));
	hx8369b_write(lcd, SEQ_VCOM_VOLTAGE, ARRAY_SIZE(SEQ_VCOM_VOLTAGE));
	hx8369b_write(lcd, SEQ_SET_CLOCK, ARRAY_SIZE(SEQ_SET_CLOCK));
	hx8369b_write(lcd, SEQ_DISP_DIRECTION, ARRAY_SIZE(SEQ_DISP_DIRECTION));
	hx8369b_write(lcd, SEQ_SOURCE_TIMING, ARRAY_SIZE(SEQ_SOURCE_TIMING));
	hx8369b_write(lcd, SEQ_CABC_CTL, ARRAY_SIZE(SEQ_CABC_CTL));
	hx8369b_write(lcd, SEQ_SOURCE_EQ, ARRAY_SIZE(SEQ_SOURCE_EQ));
	hx8369b_write(lcd, SEQ_SOURCE_OPTION, ARRAY_SIZE(SEQ_SOURCE_OPTION));
	hx8369b_write(lcd, SEQ_DIGITAL_GAMMA, ARRAY_SIZE(SEQ_DIGITAL_GAMMA));
	hx8369b_write(lcd, SEQ_GAMMA_SETTING, ARRAY_SIZE(SEQ_GAMMA_SETTING));
	hx8369b_write(lcd, SEQ_MEM_ACCESS_CTL, ARRAY_SIZE(SEQ_MEM_ACCESS_CTL));
	hx8369b_write(lcd, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));

	usleep_range(120000, 120000); /* 120 ms */

	return ret;
}

static int hx8369b_ldi_enable(struct lcd_info *lcd)
{
	int ret = 0;

	hx8369b_write(lcd, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));

	return ret;
}

static int hx8369b_ldi_disable(struct lcd_info *lcd)
{
	int ret = 0;

	hx8369b_write(lcd, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	usleep_range(50000, 50000); /* 50 ms */
	hx8369b_write(lcd, SEQ_SLEEP_IN, ARRAY_SIZE(SEQ_SLEEP_IN));

	return ret;
}

static int hx8369b_power_on(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	/* dev_info(&lcd->ld->dev, "%s\n", __func__); */

	ret = hx8369b_ldi_init(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}


	ret = hx8369b_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}

	lcd->ldi_enable = 1;

err:
	return ret;
}

static int hx8369b_power_off(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->ldi_enable = 0;

	ret = hx8369b_ldi_disable(lcd);

	msleep(135);

	return ret;
}

static int hx8369b_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = hx8369b_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = hx8369b_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int hx8369b_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return hx8369b_power(lcd, power);
}

static int hx8369b_check_fb(struct lcd_device *ld, struct fb_info *fb)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	dev_info(&lcd->ld->dev, "%s, fb%d\n", __func__, fb->node);

	return 0;
}

static int hx8369b_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

static struct lcd_ops hx8369b_panel_lcd_ops = {
	.set_power = hx8369b_set_power,
	.get_power = hx8369b_get_power,
	.check_fb  = hx8369b_check_fb,
};

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[15];

	sprintf(temp, "DTC_D0430WIBS\n");

	strcat(buf, temp);

	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0444, lcd_type_show, NULL);

#ifdef CONFIG_HAS_EARLYSUSPEND
struct lcd_info *g_lcd;

void hx8369b_early_suspend(void)
{
	struct lcd_info *lcd = g_lcd;

	set_dsim_lcd_enabled(0);

	dev_info(&lcd->ld->dev, "+%s\n", __func__);

	hx8369b_power(lcd, FB_BLANK_POWERDOWN);

	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

void hx8369b_late_resume(void)
{
	struct lcd_info *lcd = g_lcd;

	dev_info(&lcd->ld->dev, "+%s\n", __func__);

	hx8369b_power(lcd, FB_BLANK_UNBLANK);

	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	set_dsim_lcd_enabled(1);

	return ;
}
#endif

static int hx8369b_probe(struct device *dev)
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

	lcd->ld = lcd_device_register("panel", dev, lcd, &hx8369b_panel_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->dev = dev;
	lcd->dsim = (struct dsim_global *)dev_get_drvdata(dev->parent);

	lcd->bl = 0;
	lcd->current_bl = lcd->bl;

	lcd->power = FB_BLANK_UNBLANK;
	lcd->ldi_enable = 1;
	lcd->connected = 1;

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	dev_set_drvdata(dev, lcd);

	mutex_init(&lcd->lock);
	mutex_init(&lcd->bl_lock);

	dev_info(&lcd->ld->dev, "hx8369b lcd panel driver has been probed.\n");

	lcd_early_suspend = hx8369b_early_suspend;
	lcd_late_resume = hx8369b_late_resume;

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

static int __devexit hx8369b_remove(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	hx8369b_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);

	kfree(lcd);

	return 0;
}

/* Power down all displays on reboot, poweroff or halt. */
static void hx8369b_shutdown(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	hx8369b_power(lcd, FB_BLANK_POWERDOWN);
}

static struct mipi_lcd_driver hx8369b_mipi_driver = {
	.name = "hx8369b",
	.probe			= hx8369b_probe,
	.remove			= __devexit_p(hx8369b_remove),
	.shutdown		= hx8369b_shutdown,
};

static int hx8369b_init(void)
{
	return s5p_dsim_register_lcd_driver(&hx8369b_mipi_driver);
}

static void hx8369b_exit(void)
{
	return;
}

module_init(hx8369b_init);
module_exit(hx8369b_exit);

MODULE_DESCRIPTION("MIPI-DSI HX8369B: D0430WIBS-05 VER01 LCD (480x800) Panel Driver");
MODULE_LICENSE("GPL");

