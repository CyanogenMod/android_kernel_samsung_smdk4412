/*
 * IR - LED driver - leds-lr.c
 *
 * Copyright (C) 2013 Sunggeun Yim <sunggeun.yim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/leds-ir.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
int *ir_led_ctrl;
struct ir_led_platform_data *led_pdata;

extern struct class *camera_class; /*sys/class/camera*/
struct device *ir_led_dev;
struct task_struct *ir_thread_id = NULL;
static int changedGpio = false;

static int ir_led_freeGpio(void)
{
	if (1) {//led_pdata->status == STATUS_AVAILABLE) {
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

static int ir_led_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	/* Normal Setting */
	led_pdata->Period = LEDS_IR_PWM_PERIOD;
	led_pdata->onTime = 0;
	led_pdata->status = STATUS_INVALID;

	file->private_data = (int *) ir_led_ctrl;

	return ret;
}

static int ir_led_release(struct inode *inode, struct file *file)
{
	if (ir_thread_id) {
		kthread_stop(ir_thread_id);
		ir_thread_id == NULL;
	}
	if (ir_led_freeGpio()) {
		LED_ERROR("ir_led_freeGpio failed!\n");
		return -ENODEV;
	}

	return 0;
}

static int ir_led_onoff(void *arg)
{
	while(!kthread_should_stop()) {
		if (led_pdata->onTime > 0 && led_pdata->onTime < LEDS_IR_PWM_PERIOD){
			int onTime = led_pdata->onTime;
			int offTime = LEDS_IR_PWM_PERIOD-onTime;
			led_pdata->setGpio(1);
			mdelay(onTime);
			led_pdata->setGpio(0);
			mdelay(offTime);
		} else {
			if (!changedGpio) {
				if (led_pdata->onTime == 0)
					led_pdata->setGpio(0);
				else
					led_pdata->setGpio(1);
				changedGpio = true;
			}
		}
	}
	return 0;
}

static long ir_led_ioctl(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	return 0;
}

ssize_t ir_led_power(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	int value = 0;
	int cnt = 0;

	if (buf == NULL)
		return -1;

	while (buf[cnt] && buf[cnt] >= '0' && buf[cnt] <= '9') {
		value = value * 10 + buf[cnt] - '0';
		++cnt;
	}

	if (value <= 0) {
		led_pdata->onTime = 0;
	} else {
		led_pdata->onTime = value;
		if(ir_thread_id == NULL) {
			ir_thread_id = (struct task_struct *)kthread_run(ir_led_onoff,
						NULL,
						"ir_led_work_thread");
		}
	}
	changedGpio = false;

	printk(KERN_ERR "onTime = %d\n",value);
	return count;
}

ssize_t ir_led_get_max_brightness(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf,
		sizeof(led_pdata->onTime), "%d", 0);
}

static DEVICE_ATTR(rear_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
	ir_led_get_max_brightness, ir_led_power);

static const struct file_operations ir_led_fops = {
	.owner = THIS_MODULE,
	.open = ir_led_open,
	.release = ir_led_release,
	.unlocked_ioctl = ir_led_ioctl,
};

static struct miscdevice ir_led_miscdev = {
	.minor = 255,
	.name = "ir_led_led",
	.fops = &ir_led_fops,
};

static int ir_led_led_probe(struct platform_device *pdev)
{
	LED_ERROR("IR_LED Probe\n");
	led_pdata =
		(struct ir_led_platform_data *) pdev->dev.platform_data;

	if (misc_register(&ir_led_miscdev)) {
		LED_ERROR("Failed to register misc driver\n");
		return -ENODEV;
	}

	ir_led_dev = device_create(camera_class, NULL, 0, NULL, "flash");
	if (IS_ERR(ir_led_dev))
		LED_ERROR("Failed to create device(flash)!\n");

	if (device_create_file(ir_led_dev, &dev_attr_rear_flash) < 0) {
		LED_ERROR("failed to create device file, %s\n",
				dev_attr_rear_flash.attr.name);
	}

	if (led_pdata)
		led_pdata->initGpio();

	if(ir_thread_id == NULL) {
		ir_thread_id = (struct task_struct *)kthread_run(ir_led_onoff,
					NULL,
					"ir_led_work_thread");
	}
	return 0;
}

static int __devexit ir_led_led_remove(struct platform_device *pdev)
{
	if (ir_thread_id) {
		kthread_stop(ir_thread_id);
		ir_thread_id == NULL;
	}

	led_pdata->freeGpio();
	misc_deregister(&ir_led_miscdev);

	device_remove_file(ir_led_dev, &dev_attr_rear_flash);
	device_destroy(camera_class, 0);
	class_destroy(camera_class);

	return 0;
}

static struct platform_driver ir_led_led_driver = {
	.probe		= ir_led_led_probe,
	.remove		= __devexit_p(ir_led_led_remove),
	.driver		= {
		.name	= "ir-led",
		.owner	= THIS_MODULE,
	},
};

static int __init ir_led_led_init(void)
{
	return platform_driver_register(&ir_led_led_driver);
}

static void __exit ir_led_led_exit(void)
{
	platform_driver_unregister(&ir_led_led_driver);
}

module_init(ir_led_led_init);
module_exit(ir_led_led_exit);

MODULE_AUTHOR("sunggeun yim <sunggeun.yim@samsung.com.com>");
MODULE_DESCRIPTION("IR LED driver");
MODULE_LICENSE("GPL");

