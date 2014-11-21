/*
 * IR - LED driver - leds-lr-pwm.c
 *
 * Copyright (C) 2013 Sunggeun Yim <sunggeun.yim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/leds-ir-pwm.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/pwm.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-midas.h>

int *ir_led_ctrl;
struct ir_led_platform_data *led_pdata;

extern struct class *camera_class; /*sys/class/camera*/
struct device *ir_led_dev;
static int changedGpio = false;

struct ir_led_pwm_drvdata {
	struct work_struct work;
	struct pwm_device	*pwm;
	struct i2c_client *client;
	int (*power_en) (bool) ;
	spinlock_t lock;
	bool running;
	int timeout;
	int max_timeout;
	u8 duty;
	u8 period;
	int pwm_id;
	unsigned int pwm_duty;
	unsigned int pwm_period;
	int sysfs_input_data;
};
struct ir_led_pwm_drvdata *global_ir_led_pwm_data;

ssize_t ir_led_pwm_store(struct device *dev,
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
		pwm_disable(global_ir_led_pwm_data->pwm);
		pwm_config(global_ir_led_pwm_data->pwm,
			global_ir_led_pwm_data->pwm_duty,
			global_ir_led_pwm_data->pwm_period);
		global_ir_led_pwm_data->power_en(false);
		global_ir_led_pwm_data->sysfs_input_data = 0;
		global_ir_led_pwm_data->running = false;
	} else if (value == 100){
		global_ir_led_pwm_data->running = true;
		pwm_config(global_ir_led_pwm_data->pwm,
			1000000000UL>>3,
			1000000000UL);
		pwm_enable(global_ir_led_pwm_data->pwm);
		global_ir_led_pwm_data->power_en(true);
		global_ir_led_pwm_data->sysfs_input_data = value;
	} else {
		global_ir_led_pwm_data->running = true;
		pwm_config(global_ir_led_pwm_data->pwm,
			global_ir_led_pwm_data->pwm_duty,
			global_ir_led_pwm_data->pwm_period);
		pwm_enable(global_ir_led_pwm_data->pwm);
		global_ir_led_pwm_data->power_en(true);
		global_ir_led_pwm_data->sysfs_input_data = value;
	}

	printk(KERN_ERR "InputData = %d\n",value);
	return count;
}

ssize_t ir_led_pwm_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", global_ir_led_pwm_data->sysfs_input_data);
}

static DEVICE_ATTR(rear_flash, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
	ir_led_pwm_show, ir_led_pwm_store);

static int ir_led_led_probe(struct platform_device *pdev)
{
	struct ir_led_pwm_platform_data *pdata = pdev->dev.platform_data;
	struct ir_led_pwm_drvdata *ddata;
	int ret = 0;

	LED_ERROR("IR_LED Probe\n");
	ddata = kzalloc(sizeof(struct ir_led_pwm_drvdata), GFP_KERNEL);
	if (NULL == ddata) {
		printk(KERN_ERR "Failed to alloc memory\n");
		ret = -ENOMEM;
		goto err_free_mem;
	}

	ddata->pwm = pwm_request(pdata->pwm_id, "ir-led-pwm");
	if (IS_ERR(ddata->pwm)) {
		pr_err("Failed to request pwm.\n");
		ret = -EFAULT;
		goto err_request_clk;
	}

	ddata->pwm_id = pdata->pwm_id;
	ddata->pwm_duty = pdata->pwm_duty;
	ddata->pwm_period = pdata->pwm_period;
	ddata->max_timeout = pdata->max_timeout;
	ddata->power_en = pdata->power_en;
	ddata->running = false;
	ddata->sysfs_input_data = 0;

	if (gpio_is_valid(IR_LED_PWM)) {
		s3c_gpio_cfgpin(IR_LED_PWM, S3C_GPIO_SFN(2));
		gpio_free(IR_LED_PWM);
	} else {
		LED_ERROR("IR_LED_PWM port isn't ready\n");
	}

	LED_INFO("pwm_id=%d\n pwm_duty=%d\n pwm_period=%d\n",
		ddata->pwm_id, ddata->pwm_duty, ddata->pwm_period);
	ret = pwm_config(ddata->pwm,
		0,
		ddata->pwm_period);
	if (ret != 0)
		pr_err("Failed to pwm_config.\n");
	ddata->power_en(false);
	pwm_disable(ddata->pwm);

	//ddata->dev.name = "IR_LED_PWM";
	global_ir_led_pwm_data = ddata;

	platform_set_drvdata(pdev, ddata);

	ir_led_dev = device_create(camera_class, NULL, 0, NULL, "flash");
	if (IS_ERR(ir_led_dev))
		LED_ERROR("Failed to create device(flash)!\n");

	if (device_create_file(ir_led_dev, &dev_attr_rear_flash) < 0) {
		LED_ERROR("failed to create device file, %s\n",
				dev_attr_rear_flash.attr.name);
	}

	return 0;

err_request_clk:
	kfree(ddata);
err_free_mem:
	return ret;

}
static int __devexit ir_led_led_remove(struct platform_device *pdev)
{
	struct ir_led_pwm_drvdata *ddata = platform_get_drvdata(pdev);

	device_remove_file(ir_led_dev, &dev_attr_rear_flash);
	device_destroy(camera_class, 0);
	class_destroy(camera_class);

	dev_set_drvdata(&pdev->dev, NULL);
	kfree(ddata);
	return 0;
}

static struct platform_driver ir_led_pwm_driver = {
	.probe		= ir_led_led_probe,
	.remove		= __devexit_p(ir_led_led_remove),
	.driver		= {
		.name	= "ir-led-pwm",
		.owner	= THIS_MODULE,
	},
};

static int __init ir_led_pwm_init(void)
{
	return platform_driver_register(&ir_led_pwm_driver);
}

static void __exit ir_led_pwm_exit(void)
{
	platform_driver_unregister(&ir_led_pwm_driver);
}

module_init(ir_led_pwm_init);
module_exit(ir_led_pwm_exit);

MODULE_AUTHOR("sunggeun yim <sunggeun.yim@samsung.com.com>");
MODULE_DESCRIPTION("IR LED PWM driver");
MODULE_LICENSE("GPL");


