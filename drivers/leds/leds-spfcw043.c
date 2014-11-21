/*
 * LED driver for SPFCW043 - leds-spfcw043.c
 *
 * Copyright (C) 2011 DongHyun Chang <dh348.chang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/leds-spfcw043.h>
int *spfcw043_ctrl;
struct spfcw043_led_platform_data *led_pdata;

extern struct class *camera_class;
extern struct device *camera_rear;

static int spfcw043_setPower(int onoff, int level)
{
	int i;
	if (led_pdata->status == STATUS_AVAILABLE) {
		led_pdata->torch_en(0);
		led_pdata->torch_set(0);
		/* onoff = 0 - select spfcw043, onoff = 1 - select ISP*/
		udelay(10);
		if (onoff) {
			led_pdata->torch_set(1);
			for (i = 1; i < level; i++) {
				udelay(1);
				led_pdata->torch_set(0);
				udelay(1);
				led_pdata->torch_set(1);
			}
		}
	} else {
		LED_ERROR("GPIO is not available!");
		return -ENODEV;
	}

	return 0;
}

static int spfcw043_freeGpio(void)
{
	if (led_pdata->status == STATUS_AVAILABLE) {
		if (led_pdata->freeGpio()) {
			LED_ERROR("Can't free GPIO for led\n");
			return -ENODEV;
		}
		led_pdata->status = STATUS_UNAVAILABLE;
	} else {
		LED_ERROR("GPIO already free!");
	}

	return 0;
}

static int spfcw043_setGpio(void)
{
	if (led_pdata->status != STATUS_AVAILABLE) {
		if (led_pdata->setGpio()) {
			LED_ERROR("Can't set GPIO for led\n");
			return -ENODEV;
		}
		led_pdata->status = STATUS_AVAILABLE;
	} else {
		LED_ERROR("GPIO already set!");
	}

	return 0;
}

static int spfcw043_open(struct inode *inode, struct file *file)
{
	int ret;
	led_pdata->brightness = TORCH_BRIGHTNESS_50; /*normal setting*/
	led_pdata->status = STATUS_INVALID;

	ret = spfcw043_setGpio();

	file->private_data = (int *) spfcw043_ctrl;

	return ret;
}

static int spfcw043_release(struct inode *inode, struct file *file)
{
	if (spfcw043_freeGpio()) {
		LED_ERROR("spfcw043_freeGpio failed!\n");
		return -ENODEV;
	}

	return 0;
}

static ssize_t spfcw043_power(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	int brightness = 0;

	if (buf[0] < 0x30 || buf[0] > 0x39) {
		LED_ERROR("data is wrong!");
		return count;
	} else if (buf[1] < 0x30 || buf[1] > 0x39)
		brightness = (int) (buf[0] - 0x30);
	else {
		brightness = brightness + (int) (buf[0] - 0x30) * 10;
		brightness = brightness + (int) (buf[1] - 0x30);
	}

	if (brightness < 0 || brightness >= TORCH_BRIGHTNESS_INVALID) {
		LED_ERROR("brightness data is wrong! %d", brightness);
		return count;
	}

	if (brightness == 0) {
		spfcw043_setPower(0, 0);
	} else {
		if (spfcw043_setGpio()) {
			LED_ERROR("spfcw043_setGpio failed!\n");
			return count;
		}
		spfcw043_setPower(1, brightness);
	}

	return count;
}

static DEVICE_ATTR(rear_flash, S_IWUSR|S_IWGRP|S_IROTH, NULL, spfcw043_power);

static const struct file_operations spfcw043_fops = {
	.owner = THIS_MODULE,
	.open = spfcw043_open,
	.release = spfcw043_release,
};

static struct miscdevice spfcw043_miscdev = {
	.minor = 255,
	.name = "spfcw043_led",
	.fops = &spfcw043_fops,
};

static int spfcw043_led_probe(struct platform_device *pdev)
{
	LED_ERROR("Probe\n");
	led_pdata =
		(struct spfcw043_led_platform_data *) pdev->dev.platform_data;

	if (misc_register(&spfcw043_miscdev)) {
		LED_ERROR("Failed to register misc driver\n");
		return -ENODEV;
	}

	if (IS_ERR(camera_rear))
		LED_ERROR("Failed to create device(flash)!\n");

	if (device_create_file(camera_rear, &dev_attr_rear_flash) < 0) {
		LED_ERROR("failed to create device file, %s\n",
				dev_attr_rear_flash.attr.name);
	}
	spfcw043_setGpio();
	return 0;
}

static int __devexit spfcw043_led_remove(struct platform_device *pdev)
{
	led_pdata->freeGpio();
	misc_deregister(&spfcw043_miscdev);

	device_remove_file(camera_rear, &dev_attr_rear_flash);
	device_destroy(camera_class, 0);
	class_destroy(camera_class);

	return 0;
}

static struct platform_driver spfcw043_led_driver = {
	.probe		= spfcw043_led_probe,
	.remove		= __devexit_p(spfcw043_led_remove),
	.driver		= {
		.name	= "spfcw043-led",
		.owner	= THIS_MODULE,
	},
};

static int __init spfcw043_led_init(void)
{
	return platform_driver_register(&spfcw043_led_driver);
}

static void __exit spfcw043_led_exit(void)
{
	platform_driver_unregister(&spfcw043_led_driver);
}

module_init(spfcw043_led_init);
module_exit(spfcw043_led_exit);

MODULE_AUTHOR("DongHyun Chang <dh348.chang@samsung.com.com>");
MODULE_DESCRIPTION("SPFCW043 LED driver");
MODULE_LICENSE("GPL");
