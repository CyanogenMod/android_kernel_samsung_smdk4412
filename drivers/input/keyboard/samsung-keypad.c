/*
 * Samsung keypad driver
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 * Author: Donghwa Lee <dh09.lee@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <plat/keypad.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>



#define SAMSUNG_KEYIFCON			0x00
#define SAMSUNG_KEYIFSTSCLR			0x04
#define SAMSUNG_KEYIFCOL			0x08
#define SAMSUNG_KEYIFROW			0x0c
#define SAMSUNG_KEYIFFC				0x10

/* SAMSUNG_KEYIFCON */
#define SAMSUNG_KEYIFCON_INT_F_EN		(1 << 0)
#define SAMSUNG_KEYIFCON_INT_R_EN		(1 << 1)
#define SAMSUNG_KEYIFCON_DF_EN			(1 << 2)
#define SAMSUNG_KEYIFCON_FC_EN			(1 << 3)
#define SAMSUNG_KEYIFCON_WAKEUPEN		(1 << 4)

/* SAMSUNG_KEYIFSTSCLR */
#define SAMSUNG_KEYIFSTSCLR_P_INT_MASK		(0xff << 0)
#define SAMSUNG_KEYIFSTSCLR_R_INT_MASK		(0xff << 8)
#define SAMSUNG_KEYIFSTSCLR_R_INT_OFFSET	8
#define S5PV210_KEYIFSTSCLR_P_INT_MASK		(0x3fff << 0)
#define S5PV210_KEYIFSTSCLR_R_INT_MASK		(0x3fff << 16)
#define S5PV210_KEYIFSTSCLR_R_INT_OFFSET	16

/* SAMSUNG_KEYIFCOL */
#define SAMSUNG_KEYIFCOL_MASK			(0xff << 0)
#define S5PV210_KEYIFCOLEN_MASK			(0xff << 8)

/* SAMSUNG_KEYIFROW */
#define SAMSUNG_KEYIFROW_MASK			(0xff << 0)
#define S5PV210_KEYIFROW_MASK			(0x3fff << 0)

/* SAMSUNG_KEYIFFC */
#define SAMSUNG_KEYIFFC_MASK			(0x3ff << 0)

extern struct class *sec_class;

static int key_suspend;
static int key_led_prev;

enum samsung_keypad_type {
	KEYPAD_TYPE_SAMSUNG,
	KEYPAD_TYPE_S5PV210,
};

struct samsung_keypad {
	struct input_dev *input_dev;
	struct clk *clk;
	struct device	*dev;
	void __iomem *base;
	wait_queue_head_t wait;
	bool stopped;
	int irq;
	unsigned int row_shift;
	unsigned int rows;
	unsigned int cols;
	unsigned int row_state[SAMSUNG_MAX_COLS];
	unsigned short keycodes[];
};

static int samsung_keypad_is_s5pv210(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	enum samsung_keypad_type type =
		platform_get_device_id(pdev)->driver_data;

	return type == KEYPAD_TYPE_S5PV210;
}

static void samsung_keypad_scan(struct samsung_keypad *keypad,
				unsigned int *row_state)
{
	struct device *dev = keypad->input_dev->dev.parent;
	unsigned int col;
	unsigned int val;

	for (col = 0; col < keypad->cols; col++) {
		if (samsung_keypad_is_s5pv210(dev)) {
			val = S5PV210_KEYIFCOLEN_MASK;
			val &= ~(1 << col) << 8;
		} else {
			val = SAMSUNG_KEYIFCOL_MASK;
			val &= ~(1 << col);
		}

		writel(val, keypad->base + SAMSUNG_KEYIFCOL);
		mdelay(1);

		val = readl(keypad->base + SAMSUNG_KEYIFROW);
		row_state[col] = ~val & ((1 << keypad->rows) - 1);
	}

	/* KEYIFCOL reg clear */
	writel(0, keypad->base + SAMSUNG_KEYIFCOL);
}

static bool samsung_keypad_report(struct samsung_keypad *keypad,
				  unsigned int *row_state)
{
	struct input_dev *input_dev = keypad->input_dev;
	unsigned int changed;
	unsigned int pressed;
	unsigned int key_down = 0;
	unsigned int val;
	unsigned int col, row;

	for (col = 0; col < keypad->cols; col++) {
		changed = row_state[col] ^ keypad->row_state[col];
		key_down |= row_state[col];
		if (!changed)
			continue;

		for (row = 0; row < keypad->rows; row++) {
			if (!(changed & (1 << row)))
				continue;

			pressed = row_state[col] & (1 << row);
			pressed = pressed ? 1 : 0;
			dev_dbg(&keypad->input_dev->dev,
				"key %s, row: %d, col: %d\n",
				pressed ? "pressed" : "released", row, col);

			val = MATRIX_SCAN_CODE(row, col, keypad->row_shift);

			input_event(input_dev, EV_KEY, keypad->keycodes[val], pressed);
			printk(KERN_INFO "[KEY]:%d, :%d, %d\n", keypad->keycodes[val], val, pressed);
		}
		input_sync(keypad->input_dev);
	}

	memcpy(keypad->row_state, row_state, sizeof(keypad->row_state));

	return key_down;
}

static irqreturn_t samsung_keypad_irq(int irq, void *dev_id)
{
	struct samsung_keypad *keypad = dev_id;
	unsigned int row_state[SAMSUNG_MAX_COLS];
	unsigned int val;
	bool key_down;


	printk(KERN_INFO "[KEY]samsung_keypad_irq()\n");

	do {
		val = readl(keypad->base + SAMSUNG_KEYIFSTSCLR);
		/* Clear interrupt. */
		writel(~0x0, keypad->base + SAMSUNG_KEYIFSTSCLR);
		samsung_keypad_scan(keypad, row_state);
		key_down = samsung_keypad_report(keypad, row_state);
		if (key_down)
			wait_event_timeout(keypad->wait, keypad->stopped,
					   msecs_to_jiffies(50));

	} while (key_down && !keypad->stopped);

	return IRQ_HANDLED;
}

static irqreturn_t samsung_keypad_xeint_irq(int irq, void *dev_id)
{
	struct samsung_keypad *keypad = dev_id;
	struct input_dev *input_dev = keypad->input_dev;
	unsigned int val;

	printk(KERN_INFO "[KEY]samsung_keypad_xeint_irq()\n");

	input_event(input_dev, EV_KEY, KEY_POWER, 1);
	input_event(input_dev, EV_KEY, KEY_POWER, 0);
	input_sync(input_dev);

	s3c_gpio_cfgpin(GPIO_KBR_0, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_KBR_0, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_KBR_1, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_KBR_1, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_KBR_2, S3C_GPIO_SFN(3));
	s3c_gpio_setpull(GPIO_KBR_2, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgall_range(GPIO_KBR_3, 2, S3C_GPIO_SFN(3), S3C_GPIO_PULL_UP);
	s3c_gpio_cfgall_range(GPIO_KBC_0, 5, S3C_GPIO_SFN(3), S3C_GPIO_PULL_NONE);

	return IRQ_HANDLED;
}

static void samsung_keypad_start(struct samsung_keypad *keypad)
{
	unsigned int val;

	printk(KERN_INFO "[KEY]samsung_keypad_start()\n");

	/* Tell IRQ thread that it may poll the device. */
	keypad->stopped = false;

	clk_enable(keypad->clk);

	/* Enable interrupt bits. */
	val = readl(keypad->base + SAMSUNG_KEYIFCON);
	val |= SAMSUNG_KEYIFCON_INT_F_EN | SAMSUNG_KEYIFCON_INT_R_EN;
	writel(val, keypad->base + SAMSUNG_KEYIFCON);

	/* KEYIFCOL reg clear. */
	writel(0, keypad->base + SAMSUNG_KEYIFCOL);
}

static void samsung_keypad_stop(struct samsung_keypad *keypad)
{
	unsigned int val;

	printk(KERN_INFO "[KEY]samsung_keypad_stop()\n");

	/* Signal IRQ thread to stop polling and disable the handler. */
	keypad->stopped = true;
	wake_up(&keypad->wait);
	disable_irq(keypad->irq);

	/* Clear interrupt. */
	writel(~0x0, keypad->base + SAMSUNG_KEYIFSTSCLR);

	/* Disable interrupt bits. */
	val = readl(keypad->base + SAMSUNG_KEYIFCON);
	val &= ~(SAMSUNG_KEYIFCON_INT_F_EN | SAMSUNG_KEYIFCON_INT_R_EN);
	writel(val, keypad->base + SAMSUNG_KEYIFCON);

	clk_disable(keypad->clk);

	/*
	 * Now that chip should not generate interrupts we can safely
	 * re-enable the handler.
	 */
	enable_irq(keypad->irq);
}

static int samsung_keypad_open(struct input_dev *input_dev)
{
	struct samsung_keypad *keypad = input_get_drvdata(input_dev);

	samsung_keypad_start(keypad);

	return 0;
}

static void samsung_keypad_close(struct input_dev *input_dev)
{
	struct samsung_keypad *keypad = input_get_drvdata(input_dev);

	samsung_keypad_stop(keypad);
}

static ssize_t key_disable_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
/*	struct samsung_keypad *keypad = dev_get_drvdata(dev);*/
	return 0;

}

static ssize_t key_disable_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
/*	struct samsung_keypad *keypad = dev_get_drvdata(dev);*/

	return 0;
}

static ssize_t key_pressed_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct samsung_keypad *keypad = dev_get_drvdata(dev);
	return 0;
}

static ssize_t key_led_onoff(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	int data;
	struct regulator *regulator;
	regulator = regulator_get(NULL, "VREG_KEY");
	if (IS_ERR(regulator))
		return 0;
	sscanf(buf, "%d\n", &data);

	if (key_led_prev == data || key_suspend == 1)
		return 0;
	if (data) {
		regulator_enable(regulator);
		printk(KERN_INFO "[KEY] key_led_on\n");
		}
	else {
		regulator_disable(regulator);
		printk(KERN_INFO "[KEY] key_led_off\n");
		}
	key_led_prev = data;
	regulator_put(regulator);
	mdelay(70);
	return 0;
}

static DEVICE_ATTR(disable_key, S_IRUGO | S_IWUSR | S_IWGRP,
		key_disable_show, key_disable_store);
static DEVICE_ATTR(key_pressed, S_IRUGO | S_IWUSR | S_IWGRP,
		key_pressed_show, NULL);
static DEVICE_ATTR(keyled_onoff, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, key_led_onoff);

static struct attribute *key_attributes[] = {
	&dev_attr_disable_key.attr,
	&dev_attr_key_pressed.attr,
	&dev_attr_keyled_onoff.attr,
	NULL,
};

static struct attribute_group key_attr_group = {
	.attrs = key_attributes,
};


static int __devinit samsung_keypad_probe(struct platform_device *pdev)
{
	const struct samsung_keypad_platdata *pdata;
	const struct matrix_keymap_data *keymap_data;
	struct samsung_keypad *keypad;
	struct resource *res;
	struct input_dev *input_dev;
	unsigned int row_shift;
	unsigned int keymap_size;
	int error;
	int ret;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "no platform data defined\n");
		return -EINVAL;
	}

	keymap_data = pdata->keymap_data;
	if (!keymap_data) {
		dev_err(&pdev->dev, "no keymap data defined\n");
		return -EINVAL;
	}

	if (!pdata->rows || pdata->rows > SAMSUNG_MAX_ROWS)
		return -EINVAL;

	if (!pdata->cols || pdata->cols > SAMSUNG_MAX_COLS)
		return -EINVAL;

	/* initialize the gpio */
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdata->rows, pdata->cols);

	row_shift = get_count_order(pdata->cols);
	keymap_size = (pdata->rows << row_shift) * sizeof(keypad->keycodes[0]);

	keypad = kzalloc(sizeof(*keypad) + keymap_size, GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!keypad || !input_dev) {
		error = -ENOMEM;
		goto err_free_mem;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		error = -ENODEV;
		goto err_free_mem;
	}

	keypad->base = ioremap(res->start, resource_size(res));
	if (!keypad->base) {
		error = -EBUSY;
		goto err_free_mem;
	}

	keypad->clk = clk_get(&pdev->dev, "keypad");
	if (IS_ERR(keypad->clk)) {
		dev_err(&pdev->dev, "failed to get keypad clk\n");
		error = PTR_ERR(keypad->clk);
		goto err_unmap_base;
	}

	keypad->input_dev = input_dev;
	keypad->row_shift = row_shift;
	keypad->rows = pdata->rows;
	keypad->cols = pdata->cols;
	init_waitqueue_head(&keypad->wait);

	input_dev->name = pdev->name;
	#if defined(CONFIG_MACH_GRANDE) || defined(CONFIG_MACH_IRON)
	input_dev->phys = "grande_3x4_keypad/input0";
	#else
	input_dev->phys = "samsung-keypad/input0";
	#endif
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &pdev->dev;
	input_set_drvdata(input_dev, keypad);

	input_dev->open = samsung_keypad_open;
	input_dev->close = samsung_keypad_close;


	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);
	set_bit(EV_KEY, input_dev->evbit);
	if (!pdata->no_autorepeat)
		set_bit(EV_REP, input_dev->evbit);


	/* when sleep => wakeup, we want to report key-value as KEY_POWER */
	input_set_capability(input_dev, EV_KEY, KEY_POWER);

	input_dev->keycode = keypad->keycodes;
	input_dev->keycodesize = sizeof(keypad->keycodes[0]);
	input_dev->keycodemax = pdata->rows << row_shift;

	matrix_keypad_build_keymap(keymap_data, row_shift,
			input_dev->keycode, input_dev->keybit);

	keypad->irq = platform_get_irq(pdev, 0);
	if (keypad->irq < 0) {
		error = keypad->irq;
		goto err_put_clk;
	}

	error = request_threaded_irq(keypad->irq, NULL, samsung_keypad_irq,
			IRQF_ONESHOT, dev_name(&pdev->dev), keypad);
	if (error) {
		dev_err(&pdev->dev, "failed to register keypad interrupt\n");
		goto err_put_clk;
	}

	error = input_register_device(keypad->input_dev);
	if (error)
		goto err_free_irq;

	device_init_wakeup(&pdev->dev, pdata->wakeup);
	platform_set_drvdata(pdev, keypad);
	/*sysfs*/
	keypad->dev = device_create(sec_class, NULL, 0, NULL, "sec_keypad");
	if (IS_ERR(keypad->dev)) {
		printk(KERN_ERR "Failed to create device(tkey_i2c->dev)!\n");
		input_unregister_device(input_dev);
	} else {
		dev_set_drvdata(keypad->dev, keypad);
		ret = sysfs_create_group(&keypad->dev->kobj,
					&key_attr_group);
		if (ret) {
			printk(KERN_ERR
				"[Key]: failed to create sysfs group\n");
		}
	}

	return 0;

err_free_irq:
	free_irq(keypad->irq, keypad);
err_put_clk:
	clk_put(keypad->clk);
err_unmap_base:
	iounmap(keypad->base);
err_free_mem:
	input_free_device(input_dev);
	kfree(keypad);

	return error;
}

static int __devexit samsung_keypad_remove(struct platform_device *pdev)
{
	struct samsung_keypad *keypad = platform_get_drvdata(pdev);

	device_init_wakeup(&pdev->dev, 0);
	platform_set_drvdata(pdev, NULL);

	input_unregister_device(keypad->input_dev);

	/*
	 * It is safe to free IRQ after unregistering device because
	 * samsung_keypad_close will shut off interrupts.
	 */
	free_irq(keypad->irq, keypad);

	clk_put(keypad->clk);

	iounmap(keypad->base);
	kfree(keypad);

	return 0;
}

#if  1 /*CONFIG_PM*/
static void samsung_keypad_toggle_wakeup(struct samsung_keypad *keypad,
					 bool enable)
{
	struct device *dev = keypad->input_dev->dev.parent;
	unsigned int val;

	clk_enable(keypad->clk);

	val = readl(keypad->base + SAMSUNG_KEYIFCON);
	if (enable) {
		val |= SAMSUNG_KEYIFCON_WAKEUPEN;
		if (device_may_wakeup(dev))
			enable_irq_wake(keypad->irq);
	} else {
		val &= ~SAMSUNG_KEYIFCON_WAKEUPEN;
		if (device_may_wakeup(dev))
			disable_irq_wake(keypad->irq);
	}
	writel(val, keypad->base + SAMSUNG_KEYIFCON);

	clk_disable(keypad->clk);
}

static int samsung_keypad_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct samsung_keypad *keypad = platform_get_drvdata(pdev);
	struct input_dev *input_dev = keypad->input_dev;
	int irq;
	int error;

	printk(KERN_INFO "[KEY] samsung_keypad_suspend()\n");

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		samsung_keypad_stop(keypad);

/*	samsung_keypad_toggle_wakeup(keypad, true);	*/

	s3c_gpio_cfgpin(GPIO_KBR_0, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_KBR_0, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_KBR_1, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_KBR_1, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_KBR_2, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_KBR_2, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgall_range(GPIO_KBR_3, 2, S3C_GPIO_SFN(0xf), S3C_GPIO_PULL_UP);
	s3c_gpio_cfgall_range(GPIO_KBC_0, 5, S3C_GPIO_OUTPUT, S3C_GPIO_PULL_DOWN);

	irq = gpio_to_irq(GPIO_KBR_0);
	/*printk(KERN_INFO "[KEY] KP_ROW[2] : %d\n", irq);  // KBR(0)*/
	error = request_threaded_irq(irq, NULL, samsung_keypad_xeint_irq,
			IRQF_TRIGGER_FALLING, dev_name(&pdev->dev), keypad);
	enable_irq_wake(irq);

	irq = gpio_to_irq(GPIO_KBR_1);
	/*printk(KERN_INFO "[KEY] KP_ROW[4]  : %d\n", irq); // KBR(3)*/
	error = request_threaded_irq(irq, NULL, samsung_keypad_xeint_irq,
			IRQF_TRIGGER_FALLING, dev_name(&pdev->dev), keypad);
	enable_irq_wake(irq);

	irq = gpio_to_irq(GPIO_KBR_2);
	/*printk(KERN_INFO "[KEY] KP_ROW[8]  : %d\n", irq); // KBR(1)*/
	error = request_threaded_irq(irq, NULL, samsung_keypad_xeint_irq,
			IRQF_TRIGGER_FALLING, dev_name(&pdev->dev), keypad);
	enable_irq_wake(irq);

	irq = gpio_to_irq(GPIO_KBR_3);
	/*printk(KERN_INFO "[KEY] KP_ROW[11]  : %d\n", irq); // KBR(4)*/
	error = request_threaded_irq(irq, NULL, samsung_keypad_xeint_irq,
			IRQF_TRIGGER_FALLING, dev_name(&pdev->dev), keypad);
	enable_irq_wake(irq);

	irq = gpio_to_irq(GPIO_KBR_4);
	/*printk(KERN_INFO "[KEY] KP_ROW[12]  : %d\n", irq); // KBR(2)*/
	error = request_threaded_irq(irq, NULL, samsung_keypad_xeint_irq,
			IRQF_TRIGGER_FALLING, dev_name(&pdev->dev), keypad);
	enable_irq_wake(irq);

	mutex_unlock(&input_dev->mutex);

	return 0;
}

static int samsung_keypad_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct samsung_keypad *keypad = platform_get_drvdata(pdev);
	struct input_dev *input_dev = keypad->input_dev;

	printk(KERN_INFO "[KEY]samsung_keypad_resume()\n");

	mutex_lock(&input_dev->mutex);

/*	samsung_keypad_toggle_wakeup(keypad, false);*/

	if (input_dev->users)
		samsung_keypad_start(keypad);

	mutex_unlock(&input_dev->mutex);

	key_suspend = 0;
	return 0;
}

static const struct dev_pm_ops samsung_keypad_pm_ops = {
	.suspend	= samsung_keypad_suspend,
	.resume		= samsung_keypad_resume,
};
#endif

static struct platform_device_id samsung_keypad_driver_ids[] = {
	{
		#if defined(CONFIG_MACH_GRANDE) || defined(CONFIG_MACH_IRON)
		.name	= "grande_3x4_keypad",
		#else
		.name		= "samsung-keypad",
		#endif
		.driver_data	= KEYPAD_TYPE_SAMSUNG,
	}, {
		.name		= "s5pv210-keypad",
		.driver_data	= KEYPAD_TYPE_S5PV210,
	},
	{ },
};
MODULE_DEVICE_TABLE(platform, samsung_keypad_driver_ids);

static struct platform_driver samsung_keypad_driver = {
	.probe		= samsung_keypad_probe,
	.remove		= __devexit_p(samsung_keypad_remove),
	.driver		= {
#if defined(CONFIG_MACH_GRANDE) || defined(CONFIG_MACH_IRON)
		.name	= "grande_3x4_keypad",
#else
		.name	= "samsung-keypad",
#endif
		.owner	= THIS_MODULE,
#if  1 /*CONFIG_PM*/
		.pm	= &samsung_keypad_pm_ops,
#endif
	},
	.id_table	= samsung_keypad_driver_ids,
};

static int __init samsung_keypad_init(void)
{
	return platform_driver_register(&samsung_keypad_driver);
}
module_init(samsung_keypad_init);

static void __exit samsung_keypad_exit(void)
{
	platform_driver_unregister(&samsung_keypad_driver);
}
module_exit(samsung_keypad_exit);

MODULE_DESCRIPTION("Samsung keypad driver");
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_AUTHOR("Donghwa Lee <dh09.lee@samsung.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:samsung-keypad");
