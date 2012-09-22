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


struct s6c1372_lcd {
	unsigned int			power;
	struct device			*dev;
	struct lcd_device		*ld;
	struct lcd_platform_data	*lcd_pd;
};

static int lvds_lcd_set_power(struct lcd_device *ld, int power)
{
	struct s6c1372_lcd *lcd = lcd_get_data(ld);
	struct lcd_platform_data *pd = NULL;
	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}
	pd = lcd->lcd_pd;
	if (power)
		pd->power_on(lcd->ld, 0);
	else
		pd->power_on(lcd->ld, 1);
	lcd->power = power;
	return 0;
}
static int lvds_lcd_get_power(struct lcd_device *ld)
{
	struct s6c1372_lcd *lcd = lcd_get_data(ld);

	return lcd->power;
}
static struct lcd_ops s6c1372_ops = {
	.set_power = lvds_lcd_set_power,
	.get_power = lvds_lcd_get_power,
};

extern unsigned int lcdtype;

static ssize_t lcdtype_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[15];
#if defined(CONFIG_FB_S5P_S6F1202A)
	/*For P2*/
	if (lcdtype == 0)
		sprintf(temp, "HYDIS_NT51008\n");
	else if (lcdtype == 2)
		sprintf(temp, "BOE_NT51008\n");
	else
		sprintf(temp, "SMD_S6F1202A02\n");
#else
	/*For p4*/
		sprintf(temp, "SMD_S6C1372\n");
#endif
	strcat(buf, temp);
	return strlen(buf);
}
static DEVICE_ATTR(lcd_type, 0664, lcdtype_show, NULL);

void s5c1372_ldi_enable(void)
{
#if defined(CONFIG_FB_S5P_S6C1372)
	gpio_set_value(GPIO_LCD_EN, GPIO_LEVEL_HIGH);
	msleep(40);
#else   /* defined(CONFIG_FB_S5P_S6F1202A ) */
	gpio_set_value(GPIO_LCD_EN, GPIO_LEVEL_HIGH);
	gpio_set_value(GPIO_LCD_LDO_EN, GPIO_LEVEL_HIGH);
	msleep(40);

	/* Enable backlight PWM GPIO for P2 device. */
	gpio_set_value(GPIO_LCD_BACKLIGHT_PWM, 0);
	s3c_gpio_cfgpin(GPIO_LCD_BACKLIGHT_PWM, S3C_GPIO_SFN(3));
#endif
}

void s5c1372_ldi_disable(void)
{
#if defined(CONFIG_FB_S5P_S6C1372)
	s3c_gpio_cfgpin(GPIO_LCD_PCLK, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_LCD_PCLK, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_LCD_PCLK, GPIO_LEVEL_LOW);

	msleep(40);
	gpio_set_value(GPIO_LCD_EN, GPIO_LEVEL_LOW);

	msleep(600);
#else   /* defined(CONFIG_FB_S5P_S6F1202A ) */
	/* Disable backlight PWM GPIO for P2 device. */
	gpio_set_value(GPIO_LCD_BACKLIGHT_PWM, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_LCD_BACKLIGHT_PWM, S3C_GPIO_OUTPUT);

	/* Disable LVDS Panel Power, 1.2, 1.8, display 3.3V */
	gpio_set_value(GPIO_LCD_LDO_EN, GPIO_LEVEL_LOW);
	gpio_set_value(GPIO_LCD_EN, GPIO_LEVEL_LOW);
	msleep(300);
#endif
}

static int __init s6c1372_probe(struct platform_device *pdev)
{
	struct s6c1372_lcd *lcd;
	int ret = 0;

	lcd = kzalloc(sizeof(struct s6c1372_lcd), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	lcd->ld = lcd_device_register("panel", &pdev->dev, lcd, &s6c1372_ops);

	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}
	lcd->lcd_pd = pdev->dev.platform_data;
	if (IS_ERR(lcd->lcd_pd)) {
		pr_err("no platform data for lcd, cannot attach\n");
		ret = PTR_ERR(lcd->lcd_pd);
		goto out_free_lcd;
	}
	lcd->power = 1;
	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries\n");

	dev_set_drvdata(&pdev->dev, lcd);

	dev_info(&lcd->ld->dev, "lcd panel driver has been probed.\n");

	return  0;

out_free_lcd:
	kfree(lcd);
	return ret;
err_alloc:
	return ret;
}

static int __devexit s6c1372_remove(struct platform_device *pdev)
{
	struct s6c1372_lcd *lcd = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(lcd->ld);
	kfree(lcd);

	return 0;
}
#ifdef CONFIG_FB_S5P_S6C1372
static void s6c1372_shutdown(struct platform_device *pdev)
{
	gpio_set_value(GPIO_LED_BACKLIGHT_RESET, GPIO_LEVEL_LOW);
	msleep(200);

	s5c1372_ldi_disable();
}
#endif
static struct platform_driver s6c1372_driver = {
	.driver = {
		.name	= "s6c1372",
		.owner	= THIS_MODULE,
	},
	.probe		= s6c1372_probe,
	.remove		= __exit_p(s6c1372_remove),
	.suspend	= NULL,
	.resume		= NULL,
#ifdef CONFIG_FB_S5P_S6C1372
	.shutdown		= s6c1372_shutdown,
#endif
};

static int __init s6c1372_init(void)
{
	return  platform_driver_register(&s6c1372_driver);
}

static void __exit s6c1372_exit(void)
{
	platform_driver_unregister(&s6c1372_driver);
}

module_init(s6c1372_init);
module_exit(s6c1372_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("S6C1372 LCD driver");
MODULE_LICENSE("GPL");

