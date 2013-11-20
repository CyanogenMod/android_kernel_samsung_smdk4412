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

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

struct lcd_info {
	struct device			*dev;
	unsigned int			ldi_enable;
	unsigned int			power;
	struct lcd_device		*ld;
	struct lcd_platform_data	*lcd_pd;
	struct dsim_global		*dsim;
};

static ssize_t lcdtype_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char temp[15];
	sprintf(temp, "BOE_BP080WX7\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0664,
	lcdtype_show, NULL);
static int nt71391_power_on(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	msleep(120); /* power on 50ms, i2c 70ms */

	lcd->dsim->ops->cmd_write(lcd->dsim, TURN_ON, 0, 0);

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
	lcd->dsim = (struct dsim_global *)dev_get_drvdata(dev->parent);
	lcd->power = FB_BLANK_UNBLANK;


	dev_set_drvdata(dev, lcd);

	dev_info(dev, "lcd panel driver has been probed.\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd_early_suspend = nt71391_early_suspend;
	lcd_late_resume = nt71391_late_resume;
#endif
	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries\n");

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
