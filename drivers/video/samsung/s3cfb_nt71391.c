/* linux/drivers/video/samsung/s3cfb_nt71391.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * NT71391 : 8" WXGA Landscape LCD module driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/lcd.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <plat/regs-dsim.h>
#include <mach/dsim.h>
#include <mach/mipi_ddi.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "s3cfb.h"
#include "s5p-dsim.h"

#define NT71391_CHANGE_MINI_LVDS_FREQ_MIPI 1

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

struct lcd_info {
	struct device			*dev;
	unsigned int			ldi_enable;
	unsigned int			power;
	unsigned int			connected;
	struct mutex			lock;
	struct lcd_device		*ld;
	struct lcd_platform_data	*lcd_pd;
	struct dsim_global		*dsim;
};

#ifdef NT71391_CHANGE_MINI_LVDS_FREQ_MIPI

#define NT71391_TCON_REG_ADD		0xAC
#define NT71391_TCON_REG_CHECKSUM	0xFF
#define NT71931_N_16	0x18
#define NT71931_N_17	0x19
#define NT71931_N_18	0x1A
#define NT71931_N_19	0x1B
#define NT71931_N_20	0x1C
#define NT71931_N_21	0x1D
#define NT71931_N_22	0x1E
#define NT71931_N_23	0x1F
#define NT71931_N_24	0x10
#define NT71931_N_25	0x11


enum NT71391_COMMAND_TYPE {
	NT71391_LOCK_CMD2 = 0x03,
	NT71391_READ = 0x14,
	NT71391_WRITE = 0x23,
};


static const unsigned char NT71391_UNLOCK_PAGE0[] = {
	0xF3,0xA0
};

static const unsigned char NT71391_UNLOCK_PAGE1[] = {
	0xF3,0xA1
};

static const unsigned char NT71391_FREQ_SETTING[] = {
	NT71391_TCON_REG_ADD,NT71931_N_16
};

static const unsigned char TESTA[] = {
	0x2B,0xC0
};

static int _nt71391_write(struct lcd_info *lcd, const unsigned char *seq, enum NT71391_COMMAND_TYPE cmd_type)
{
	const unsigned char *wbuf;
	int ret = 0;

	if (!lcd->connected)
		return 0;

	mutex_lock(&lcd->lock);

	wbuf = seq;

	switch (cmd_type) {
	case NT71391_LOCK_CMD2:
		ret = lcd->dsim->ops->cmd_write(lcd->dsim, NT71391_LOCK_CMD2,0x0,0x0);
		break;
	case NT71391_READ:
		ret = lcd->dsim->ops->cmd_write(lcd->dsim, NT71391_READ,wbuf[0],0x0);
		break;
	case NT71391_WRITE:
		ret = lcd->dsim->ops->cmd_write(lcd->dsim, NT71391_WRITE, wbuf[0], wbuf[1]);
		break;
	default:
		dev_dbg(&lcd->ld->dev, "%s :: Invalid cmd type \n", __func__);
		break;
	}

	mutex_unlock(&lcd->lock);

	return ret;
}

static int nt71391_write(struct lcd_info *lcd, const unsigned char *seq, enum NT71391_COMMAND_TYPE cmd_type)
{
	int ret = 0;
	int retry_cnt = 1;

retry:
	ret = _nt71391_write(lcd, seq, cmd_type);
	if (!ret) {
		if (retry_cnt) {
			dev_dbg(&lcd->ld->dev, "%s :: retry: %d\n", __func__, retry_cnt);
			retry_cnt--;
			goto retry;
		} else
			dev_dbg(&lcd->ld->dev, "%s :: 0x%02x\n", __func__, seq[0]);
	}

	return ret;
}

static int _nt71391_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf)
{
	int ret = 0;

	if (!lcd->connected)
		return ret;

	mutex_lock(&lcd->lock);

	if (lcd->dsim->ops->cmd_read)
		ret = lcd->dsim->ops->cmd_dcs_read(lcd->dsim, addr, count, buf);

	mutex_unlock(&lcd->lock);

	return ret;
}

static int nt71391_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf, u8 retry_cnt)
{
	int ret = 0;

read_retry:
	ret = _nt71391_read(lcd, addr, count, buf);
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
#endif
static ssize_t lcdtype_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char temp[15];
	sprintf(temp, "BOE_BP080WX7\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0664,
	lcdtype_show, NULL);

static ssize_t window_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[15];

	sprintf(temp, "%x %x %x\n", 0x0, 0x0, 0x0);

	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(window_type, 0444,
	window_type_show, NULL);

static int nt71391_power_on(struct lcd_info *lcd)
{

#ifdef NT71391_CHANGE_MINI_LVDS_FREQ_MIPI
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	msleep(120); /* power on 50ms, i2c 70ms */
	nt71391_write(lcd, NT71391_UNLOCK_PAGE0, NT71391_WRITE);
	nt71391_write(lcd, NT71391_FREQ_SETTING, NT71391_WRITE);
	nt71391_write(lcd, NULL, NT71391_LOCK_CMD2);


	lcd->dsim->ops->cmd_write(lcd->dsim, TURN_ON, 0, 0);
#else
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	msleep(120); /* power on 50ms, i2c 70ms */

	lcd->dsim->ops->cmd_write(lcd->dsim, TURN_ON, 0, 0);
#endif

	lcd->ldi_enable = 1;

	return ret;
}

static int nt71391_power_off(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->ldi_enable = 0;

	msleep(135);

	return ret;
}

static int nt71391_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = nt71391_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = nt71391_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int nt71391_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return nt71391_power(lcd, power);
}

static int nt71391_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

static struct lcd_ops nt71391_lcd_ops = {
	.set_power = nt71391_set_power,
	.get_power = nt71391_get_power,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
extern void (*lcd_early_suspend)(void);
extern void (*lcd_late_resume)(void);

struct lcd_info *g_lcd;

void nt71391_early_suspend(void)
{
	struct lcd_info *lcd = g_lcd;
	int err = 0;

	set_dsim_lcd_enabled(0);

	dev_info(&lcd->ld->dev, "+%s\n", __func__);

	nt71391_power(lcd, FB_BLANK_POWERDOWN);

	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

void nt71391_late_resume(void)
{
	struct lcd_info *lcd = g_lcd;

	dev_info(&lcd->ld->dev, "+%s\n", __func__);

	nt71391_power(lcd, FB_BLANK_UNBLANK);

	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	set_dsim_lcd_enabled(1);

	return ;
}
#endif


static int __init nt71391_probe(struct device *dev)
{
	struct lcd_info *lcd;
	int ret = 0;

	lcd = kzalloc(sizeof(struct lcd_info), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	g_lcd = lcd;

	lcd->ld = lcd_device_register("panel", dev, lcd, &nt71391_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->dev = dev;
	lcd->connected = 1;
	lcd->dsim = (struct dsim_global *)dev_get_drvdata(dev->parent);
	lcd->power = FB_BLANK_UNBLANK;

	mutex_init(&lcd->lock);

	dev_set_drvdata(dev, lcd);

	dev_info(dev, "lcd panel driver has been probed.\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd_early_suspend = nt71391_early_suspend;
	lcd_late_resume = nt71391_late_resume;
#endif

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries\n");

	ret = device_create_file(&lcd->ld->dev, &dev_attr_window_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add window_type entries\n");

	return  0;

out_free_lcd:
	kfree(lcd);
err_alloc:
	return ret;
}

static int __devexit nt71391_remove(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	nt71391_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	kfree(lcd);

	return 0;
}

static void nt71391_shutdown(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	nt71391_power(lcd, FB_BLANK_POWERDOWN);
}

static struct mipi_lcd_driver nt71391_mipi_driver = {
	.name	= "nt71391",
	.probe		= nt71391_probe,
	.remove		= __devexit_p(nt71391_remove),
	.shutdown	= nt71391_shutdown,
};

static int __init nt71391_init(void)
{
	return  s5p_dsim_register_lcd_driver(&nt71391_mipi_driver);
}

static void __exit nt71391_exit(void)
{
	return;
}

module_init(nt71391_init);
module_exit(nt71391_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("NT71391 LCD driver");
MODULE_LICENSE("GPL");
