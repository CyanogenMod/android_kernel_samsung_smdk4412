/*
 * LED driver for AAT1290A - leds-aat1290a.c
 *
 * Copyright (C) 2011 DongHyun Chang <dh348.chang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/leds-aat1290a.h>
int *aat1290a_ctrl;
struct aat1290a_led_platform_data *led_pdata;

extern struct class *camera_class; /*sys/class/camera*/
struct device *aat1290a_dev;

static int aat1290a_setPower(int onoff, int level)
{
	int i;
	if (led_pdata->status == STATUS_AVAILABLE) {
		led_pdata->torch_en(0);
		led_pdata->torch_set(0);
		led_pdata->switch_sel(!onoff);
		/* onoff = 0 - select aat1290a, onoff = 1 - select ISP*/
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

static int aat1290a_freeGpio(void)
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

static int aat1290a_setGpio(void)
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

static int aat1290a_open(struct inode *inode, struct file *file)
{
	int ret;
	led_pdata->brightness = TORCH_BRIGHTNESS_50; /*normal setting*/
	led_pdata->status = STATUS_INVALID;

	ret = aat1290a_setGpio();

	file->private_data = (int *) aat1290a_ctrl;

	return ret;
}

static int aat1290a_release(struct inode *inode, struct file *file)
{
	if (aat1290a_freeGpio()) {
		LED_ERROR("aat1290a_freeGpio failed!\n");
		return -ENODEV;
	}

	return 0;
}


static long aat1290a_ioctl(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	int *ctrl = (int *)file->private_data;

	if (!ctrl) {
		LED_ERROR("invalid input data\n");
		return -1;
	}

	switch (cmd) {
	case IOCTL_AAT1290A_SET_BRIGHTNESS:
		if (*ctrl < 0 || *ctrl > TORCH_BRIGHTNESS_0)
			return -EINVAL;
		led_pdata->brightness = *ctrl;
		break;
	case IOCTL_AAT1290A_GET_STATUS:
		*ctrl = led_pdata->status;
		break;
	case IOCTL_AAT1290A_SET_POWER:
		aat1290a_setPower(*ctrl, led_pdata->brightness);
		break;
	default:
		LED_ERROR("invalid input data\n");
		return -EINVAL;
	}

	return 0;
}

ssize_t aat1290a_power(struct device *dev,
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
		aat1290a_setPower(0, 0);
		/*
		if (aat1290a_freeGpio()) {
			LED_ERROR("aat1290a_freeGpio failed!\n");
			return count;
		}
		*/
	} else {
		if (aat1290a_setGpio()) {
			LED_ERROR("aat1290a_setGpio failed!\n");
			return count;
		}
		aat1290a_setPower(1, brightness);
	}

	return count;
}

ssize_t aat1290a_get_max_brightness(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf,
		sizeof(led_pdata->brightness), "%d", TORCH_BRIGHTNESS_100);
}

static DEVICE_ATTR(rear_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
	aat1290a_get_max_brightness, aat1290a_power);

static const struct file_operations aat1290a_fops = {
	.owner = THIS_MODULE,
	.open = aat1290a_open,
	.release = aat1290a_release,
	.unlocked_ioctl = aat1290a_ioctl,
};

static struct miscdevice aat1290a_miscdev = {
	.minor = 255,
	.name = "aat1290a_led",
	.fops = &aat1290a_fops,
};

static int aat1290a_led_probe(struct platform_device *pdev)
{
	LED_ERROR("Probe\n");
	led_pdata =
		(struct aat1290a_led_platform_data *) pdev->dev.platform_data;

	if (misc_register(&aat1290a_miscdev)) {
		LED_ERROR("Failed to register misc driver\n");
		return -ENODEV;
	}

	aat1290a_dev = device_create(camera_class, NULL, 0, NULL, "flash");
	if (IS_ERR(aat1290a_dev)) {
		LED_ERROR("Failed to create device(flash)!\n");
	} else {
		if (device_create_file(aat1290a_dev, &dev_attr_rear_flash) < 0) {
			LED_ERROR("failed to create device file, %s\n",
					dev_attr_rear_flash.attr.name);
		}
	}
	if (led_pdata)
		led_pdata->initGpio();
	aat1290a_setGpio();
	return 0;
}

static int __devexit aat1290a_led_remove(struct platform_device *pdev)
{
	led_pdata->freeGpio();
	misc_deregister(&aat1290a_miscdev);

	device_remove_file(aat1290a_dev, &dev_attr_rear_flash);
	device_destroy(camera_class, 0);
	class_destroy(camera_class);

	return 0;
}

static struct platform_driver aat1290a_led_driver = {
	.probe		= aat1290a_led_probe,
	.remove		= __devexit_p(aat1290a_led_remove),
	.driver		= {
		.name	= "aat1290a-led",
		.owner	= THIS_MODULE,
	},
};

static int __init aat1290a_led_init(void)
{
	return platform_driver_register(&aat1290a_led_driver);
}

static void __exit aat1290a_led_exit(void)
{
	platform_driver_unregister(&aat1290a_led_driver);
}

module_init(aat1290a_led_init);
module_exit(aat1290a_led_exit);

MODULE_AUTHOR("DongHyun Chang <dh348.chang@samsung.com.com>");
MODULE_DESCRIPTION("AAT1290A LED driver");
MODULE_LICENSE("GPL");
