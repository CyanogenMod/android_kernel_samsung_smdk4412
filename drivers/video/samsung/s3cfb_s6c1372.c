/* linux/drivers/video/samsung/s3cfb_s6c1372.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S6F1202A : 7" WSVGA Landscape LCD module driver
 * S6C1372 : 7.3" WXGA Landscape LCD module driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/lcd.h>
#include <plat/gpio-cfg.h>
#include "s3cfb.h"


struct lcd_info {
	unsigned int			power;
	struct device			*dev;
	struct lcd_device		*ld;
	struct lcd_platform_data	*lcd_pd;
};

static ssize_t lcdtype_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[15];

	sprintf(temp, "%s\n", (char *)lcd->lcd_pd->pdata);

	strcat(buf, temp);

	return strlen(buf);
}
static DEVICE_ATTR(lcd_type, 0444, lcdtype_show, NULL);

static int __init lvds_lcd_probe(struct platform_device *pdev)
{
	struct lcd_info *lcd;
	int ret = 0;

	lcd = kzalloc(sizeof(struct lcd_info), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	lcd->ld = lcd_device_register("panel", &pdev->dev, lcd, NULL);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->lcd_pd = pdev->dev.platform_data;
	if (IS_ERR(lcd->lcd_pd)) {
		pr_err("no platform data for lcd\n");
		ret = PTR_ERR(lcd->lcd_pd);
		goto out_free_lcd;
	}

	if (IS_ERR(lcd->lcd_pd->pdata))
		lcd->lcd_pd->pdata = "dummy";

	lcd->power = FB_BLANK_UNBLANK;

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries\n");

	dev_set_drvdata(&pdev->dev, lcd);

	dev_info(&lcd->ld->dev, "%s panel driver has been probed\n", (char *)lcd->lcd_pd->pdata);

	return  0;

out_free_lcd:
	kfree(lcd);
	return ret;
err_alloc:
	return ret;
}

static int __devexit lvds_lcd_remove(struct platform_device *pdev)
{
	struct lcd_info *lcd = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(lcd->ld);
	kfree(lcd);

	return 0;
}

static struct platform_driver lvds_lcd_driver = {
	.driver = {
		.name	= "lvds_lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= lvds_lcd_probe,
	.remove		= __exit_p(lvds_lcd_remove),
	.suspend	= NULL,
	.resume		= NULL,
};

static int __init lvds_lcd_init(void)
{
	return  platform_driver_register(&lvds_lcd_driver);
}

static void __exit lvds_lcd_exit(void)
{
	platform_driver_unregister(&lvds_lcd_driver);
}

module_init(lvds_lcd_init);
module_exit(lvds_lcd_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("LVDS LCD driver");
MODULE_LICENSE("GPL");

