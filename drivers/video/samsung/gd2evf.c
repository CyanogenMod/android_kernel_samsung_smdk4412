/* linux/drivers/video/samsung/gd2evf.c
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *  http://www.samsung.com/
 *
 * 0.46" SVGA Electrical Viewfinder module driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/lcd.h>
#include <linux/backlight.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define spidelay(nsecs)	udelay(nsecs) //do {} while (0)
#define SPI_DELAY		1

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define	DEFMASK		0xFF00
#define COMMAND_ONLY		0xFE
#define DATA_ONLY		0xFF

#define MIN_BRIGHTNESS		0
#define DEFAULT_BRIGHTNESS	160
#define MAX_BRIGHTNESS		255
#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

struct lcd_info  {
	unsigned int			bl;
	unsigned int			current_bl;
	unsigned int			auto_brightness;

	struct device			*dev;
	struct spi_device		*spi;
	unsigned int			power;
	unsigned int			ldi_enable;
	struct mutex			lock;
	struct mutex			bl_lock;
	struct lcd_device		*ld;
	struct backlight_device		*bd;
	struct lcd_platform_data	*lcd_pd;
	struct early_suspend		early_suspend;
};

static struct lcd_info *g_lcd;

static const unsigned short init_seq[][2] = {
	{0x00, 0x01},
	{0x01, 0x00},
	{0x02, 0xC1},
	{0x03, 0xE0},	/* LED Brightness[5:0] */
	{0x04, 0x01},
	{0x05, 0x02},	/* Gamma, 0x0A: 2.2 0x02: 1.4 */
	{0x06, 0x60},
	{0x07, 0x22},	/* HScaleCycle[9:8], HScaleStep[9:8], VScaleCycle[9:8], VScaleStep[9:8] */
	{0x08, 0x00},	/* VScaleStep[7:0] */
	{0x09, 0x01},	/* VScaleCycle[7:0] */
	{0x0A, 0x00},	/* HScaleStep[7:0] */
	{0x0B, 0x01},	/* HScaleCycle[7:0] */
	{0x0C, 0x17},	/* VVldDelay: v_bp + v_sw */
	{0x0D, 0x00},
	{0x0E, 0x9C},	/* HVldDelay: h_bp + h_sw */
	{0x0F, 0x50},	/* PCLK: 40MHz */
	{0x10, 0x00},
	{0x11, 0x00},
	{0x12, 0x00},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x00},
	{0x16, 0x00},
	{0x17, 0x00},
	{0x18, 0x00},
	{0x19, 0x00},
	{0x1A, 0x00},
	{0x1B, 0x00},
	{0x1C, 0x00},
	{0x1D, 0x00},
	{0x1E, 0x02},
};

static const unsigned short enable_seq[][2] = {
	{0x06, 0x68},
};

static const unsigned short disable_seq[][2] = {
	{0x06, 0x60},
};

static int gd2evf_spi_write_byte(struct lcd_info *lcd, int addr, int data)
{
	u8 buf[2];

	buf[0] = addr;
	buf[1] = data;

	return spi_write(lcd->spi, buf, 2);
}

static int gd2evf_panel_send_sequence(struct lcd_info *lcd,
	const unsigned short *seq)
{
	int ret;

	mutex_lock(&lcd->lock);

	ret = gd2evf_spi_write_byte(lcd, seq[0], seq[1]);

	mutex_unlock(&lcd->lock);

	if (ret)
		dev_info(&lcd->ld->dev, "%s (%x) ret = %d\n", __func__, seq[0], ret);

	return ret;
}

static int gd2evf_ldi_init(struct lcd_info *lcd)
{
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(init_seq); i++) {
		ret = gd2evf_panel_send_sequence(lcd, init_seq[i]);
		if (ret)
			break;
	}

	return ret;
}

static int gd2evf_ldi_enable(struct lcd_info *lcd)
{
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(enable_seq); i++) {
		ret = gd2evf_panel_send_sequence(lcd, enable_seq[i]);
		if (ret)
			break;
	}

	return ret;
}

static int gd2evf_ldi_disable(struct lcd_info *lcd)
{
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(disable_seq); i++) {
		ret = gd2evf_panel_send_sequence(lcd, disable_seq[i]);
		if (ret)
			break;
	}
	usleep_range(1500, 1500);

	lcd->ldi_enable = 0;

	return ret;
}

static int update_brightness(struct lcd_info *lcd, u8 force)
{
	u32 brightness;

	mutex_lock(&lcd->bl_lock);

	brightness = lcd->bd->props.brightness;
	lcd->bl = brightness;

	if ((force) || ((lcd->ldi_enable) && (lcd->current_bl != lcd->bl))) {

		lcd->current_bl = lcd->bl;

		// update brightness
		dev_info(&lcd->ld->dev, "brightness=%d, bl=%d\n", brightness, lcd->bl);
	}

	mutex_unlock(&lcd->bl_lock);

	return 0;
}

static int gd2evf_power_on(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd;
	pd = lcd->lcd_pd;

	dev_info(&lcd->ld->dev, "+%s\n", __func__);

	if (!pd) {
		dev_err(&lcd->ld->dev, "platform data is NULL.\n");
		return -EFAULT;
	}

	if (!pd->gpio_cfg_lateresume) {
		dev_err(&lcd->ld->dev, "gpio_cfg_lateresume is NULL.\n");
		ret = -EFAULT;
		goto err;
	} else
		pd->gpio_cfg_lateresume(lcd->ld);

	if (!pd->power_on) {
		dev_err(&lcd->ld->dev, "power_on is NULL.\n");
		return -EFAULT;
	} else {
		pd->power_on(lcd->ld, 1);
		msleep(pd->power_on_delay);
	}

	if (!pd->reset) {
		dev_err(&lcd->ld->dev, "reset is NULL.\n");
		return -EFAULT;
	} else {
		pd->reset(lcd->ld);
		msleep(pd->reset_delay);
	}

	ret = gd2evf_ldi_init(lcd);

	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}

	ret = gd2evf_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}

	lcd->ldi_enable = 1;

	dev_info(&lcd->ld->dev, "-%s\n", __func__);
	update_brightness(lcd, 1);

err:
	return ret;
}

static int gd2evf_power_off(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	pd = lcd->lcd_pd;
	if (!pd) {
		dev_err(&lcd->ld->dev, "platform data is NULL.\n");
		return -EFAULT;
	}

	ret = gd2evf_ldi_disable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "lcd setting failed.\n");
		ret = -EIO;
		goto err;
	}

	if (!pd->gpio_cfg_earlysuspend) {
		dev_err(&lcd->ld->dev, "gpio_cfg_earlysuspend is NULL.\n");
		ret = -EFAULT;
		goto err;
	} else
		pd->gpio_cfg_earlysuspend(lcd->ld);

	if (!pd->power_on) {
		dev_err(&lcd->ld->dev, "power_on is NULL.\n");
		ret = -EFAULT;
		goto err;
	} else {
		msleep(pd->power_off_delay);
		pd->power_on(lcd->ld, 0);
	}

err:
	return ret;
}

static int gd2evf_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = gd2evf_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = gd2evf_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

int gd2evf_power_ext(int onoff)
{
	struct lcd_info *lcd = g_lcd;

	if (onoff)
		gd2evf_power(lcd, FB_BLANK_UNBLANK);
	else
		gd2evf_power(lcd, FB_BLANK_POWERDOWN);

	return 0;
}

#if 1
static struct lcd_ops gd2evf_lcd_ops;
#else
static int gd2evf_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return gd2evf_power(lcd, power);
}

static int gd2evf_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

static struct lcd_ops gd2evf_lcd_ops = {
	.set_power = gd2evf_set_power,
	.get_power = gd2evf_get_power,
};
#endif

static int gd2evf_get_brightness(struct backlight_device *bd)
{
	return 150;
}

static int gd2evf_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	struct lcd_info *lcd = bl_get_data(bd);

	if (lcd->ldi_enable) {
		ret = update_brightness(lcd, 0);
		if (ret < 0)
			return -EINVAL;
	}

	return ret;
}

static const struct backlight_ops gd2evf_backlight_ops  = {
	.get_brightness = gd2evf_get_brightness,
	.update_status = gd2evf_set_brightness,
};

static ssize_t lcdtype_show(struct device *dev, struct
device_attribute *attr, char *buf)
{

	char temp[15];
	sprintf(temp, "SMD_AMS427G03\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0664,
		lcdtype_show, NULL);

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
	}
	return size;
}

static DEVICE_ATTR(auto_brightness, 0644, auto_brightness_show, auto_brightness_store);

#if defined(CONFIG_PM)
#ifdef CONFIG_HAS_EARLYSUSPEND
void gd2evf_early_suspend(struct early_suspend *h)
{
	struct lcd_info *lcd = container_of(h, struct lcd_info ,
								early_suspend);
	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	gd2evf_power(lcd, FB_BLANK_POWERDOWN);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

void gd2evf_late_resume(struct early_suspend *h)
{
	struct lcd_info *lcd = container_of(h, struct lcd_info ,
								early_suspend);
	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	gd2evf_power(lcd, FB_BLANK_UNBLANK);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}
#endif
#endif

static int gd2evf_probe(struct spi_device *spi)
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

	/* gd2evf lcd panel uses 3-wire 8bits SPI Mode. */
	spi->bits_per_word = 8;

	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "spi setup failed.\n");
		goto out_free_lcd;
	}

	lcd->spi = spi;
	lcd->dev = &spi->dev;

	lcd->lcd_pd = (struct lcd_platform_data *)spi->dev.platform_data;
	if (!lcd->lcd_pd) {
		dev_err(&spi->dev, "platform data is NULL.\n");
		goto out_free_lcd;
	}

	lcd->ld = lcd_device_register("panel_evf", &spi->dev,
		lcd, &gd2evf_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("panel_evf", &spi->dev,
		lcd, &gd2evf_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		ret = PTR_ERR(lcd->bd);
		goto out_free_backlight;
	}

	lcd->bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd->bd->props.brightness = DEFAULT_BRIGHTNESS;
	lcd->power = FB_BLANK_POWERDOWN;
	lcd->lcd_pd->gpio_cfg_earlysuspend(lcd->ld);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->bd->dev, &dev_attr_auto_brightness);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	dev_set_drvdata(lcd->dev, lcd);

	mutex_init(&lcd->lock);
	mutex_init(&lcd->bl_lock);

	dev_set_drvdata(&spi->dev, lcd);

#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd->early_suspend.suspend = gd2evf_early_suspend;
	lcd->early_suspend.resume = NULL;	/* gd2evf_late_resume; */
	lcd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	register_early_suspend(&lcd->early_suspend);
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

static int __devexit gd2evf_remove(struct spi_device *spi)
{
	struct lcd_info *lcd = dev_get_drvdata(&spi->dev);

	gd2evf_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	backlight_device_unregister(lcd->bd);
	kfree(lcd);

	return 0;
}

static struct spi_driver gd2evf_driver = {
	.driver = {
		.name	= "gd2evf",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe		= gd2evf_probe,
	.remove		= __devexit_p(gd2evf_remove),
};

static int __init gd2evf_init(void)
{
	return spi_register_driver(&gd2evf_driver);
}

static void __exit gd2evf_exit(void)
{
	spi_unregister_driver(&gd2evf_driver);
}

module_init(gd2evf_init);
module_exit(gd2evf_exit);

MODULE_DESCRIPTION("GD2 EVF LCD Driver");
MODULE_LICENSE("GPL");

