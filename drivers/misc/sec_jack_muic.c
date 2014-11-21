/*  drivers/misc/sec_jack_muic.c
 *
 *  Copyright (C) 2010 Samsung Electronics Co.Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/switch.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/sec_jack.h>

#include <plat/adc.h>

/* keep this value if you support double-pressed concept */
#if defined(CONFIG_TARGET_LOCALE_KOR)
#define SEND_KEY_CHECK_TIME_MS	20		/* 20ms - GB VOC in KOR*/
#elif defined(CONFIG_MACH_Q1_BD)
/* 27ms, total delay is approximately double more
   because hrtimer is called twice by gpio input driver,
   new sec spec total delay is 60ms +/-10ms */
#define SEND_KEY_CHECK_TIME_MS	27
#else
#define SEND_KEY_CHECK_TIME_MS	40		/* 40ms */
#endif
#define WAKE_LOCK_TIME		(HZ * 5)	/* 5 sec */
#define WAKE_LOCK_TIME_IN_SENDKEY (HZ * 3)

#define JACK_CLASS_NAME "audio"
#define JACK_DEV_NAME "earjack"

static struct class *jack_class;
static struct device *jack_dev;

struct sec_jack_info {
	struct s3c_adc_client *padc;
	struct sec_jack_platform_data *pdata;
	struct delayed_work jack_detect_work;
	struct work_struct buttons_work;
	struct workqueue_struct *queue;
	struct input_dev *input_dev;
	struct wake_lock det_wake_lock;
	struct sec_jack_zone *zone;
	struct input_handler handler;
	struct input_handle handle;
	struct input_device_id ids[2];
	int det_irq;
	int dev_id;
	int pressed;
	int pressed_code;
	struct platform_device *send_key_dev;
	unsigned int cur_jack_type;
	int det_status;
};

struct sec_jack_info *hi;
static unsigned int send_end_pressed;

/* with some modifications like moving all the gpio structs inside
 * the platform data and getting the name for the switch and
 * gpio_event from the platform data, the driver could support more than
 * one headset jack, but currently user space is looking only for
 * one key file and switch for a headset so it'd be overkill and
 * untestable so we limit to one instantiation for now.
 */
static atomic_t instantiated = ATOMIC_INIT(0);

/* sysfs name HeadsetObserver.java looks for to track headset state
 */
struct switch_dev switch_jack_detection = {
	.name = "h2w",
};

/* To support AT+FCESTEST=1 */
struct switch_dev switch_sendend = {
		.name = "send_end",
};

static struct gpio_event_direct_entry sec_jack_key_map[] = {
	{
		.code	= KEY_UNKNOWN,
	},
};

static struct gpio_event_input_info sec_jack_key_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = true,
	.type = EV_KEY,
	.debounce_time.tv64 = SEND_KEY_CHECK_TIME_MS * NSEC_PER_MSEC,
	.keymap = sec_jack_key_map,
	.keymap_size = ARRAY_SIZE(sec_jack_key_map)
};

static struct gpio_event_info *sec_jack_input_info[] = {
	&sec_jack_key_info.info,
};

static struct gpio_event_platform_data sec_jack_input_data = {
	.name = "sec_jack",
	.info = sec_jack_input_info,
	.info_count = ARRAY_SIZE(sec_jack_input_info),
};

static void sec_jack_set_type(struct sec_jack_info *hi, int jack_type)
{
	struct sec_jack_platform_data *pdata = hi->pdata;

	/* this can happen during slow inserts where we think we identified
	 * the type but then we get another interrupt and do it again
	 */
	if (jack_type == hi->cur_jack_type) {
		if (jack_type != SEC_HEADSET_4POLE)
			pdata->set_micbias_state(false);

		return;
	}

	if (jack_type == SEC_HEADSET_4POLE) {
		/* for a 4 pole headset, enable detection of send/end key */
		if (hi->send_key_dev == NULL)
			/* enable to get events again */
			hi->send_key_dev = platform_device_register_data(NULL,
					GPIO_EVENT_DEV_NAME,
					hi->dev_id,
					&sec_jack_input_data,
					sizeof(sec_jack_input_data));
	} else {
		/* for all other jacks, disable send/end key detection */
		if (hi->send_key_dev != NULL) {
			/* disable to prevent false events on next insert */
			platform_device_unregister(hi->send_key_dev);
			hi->send_key_dev = NULL;
		}
		/* micbias is left enabled for 4pole and disabled otherwise */
		pdata->set_micbias_state(false);
	}
	/* if user inserted ear jack slowly, different jack event can occur
	 * sometimes because irq_thread is defined IRQ_ONESHOT, detach status
	 * can be ignored sometimes so in that case, driver inform detach
	 * event to user side
	 */
	switch_set_state(&switch_jack_detection, SEC_JACK_NO_DEVICE);

	hi->cur_jack_type = jack_type;
	pr_info("%s : jack_type = %d\n", __func__, jack_type);

	switch_set_state(&switch_jack_detection, jack_type);
}

static void handle_jack_not_inserted(struct sec_jack_info *hi)
{
	sec_jack_set_type(hi, SEC_JACK_NO_DEVICE);
	hi->pdata->set_micbias_state(false);
}

static ssize_t select_jack_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("%s : operate nothing\n", __func__);

	return 0;
}

static ssize_t select_jack_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct sec_jack_info *hi = dev_get_drvdata(dev);
	struct sec_jack_platform_data *pdata = hi->pdata;
	int value = 0;


	sscanf(buf, "%d", &value);
	pr_err("%s: User  selection : 0X%x", __func__, value);
	if (value == SEC_HEADSET_4POLE) {
		pdata->set_micbias_state(true);
		msleep(100);
	}

	sec_jack_set_type(hi, value);

	return size;
}

static ssize_t earjack_key_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_jack_info *hi = dev_get_drvdata(dev);
	int value = 0;

	if (hi->pressed <= 0)
		value = 0;
	else
		value = 1;

	return sprintf(buf, "%d\n", value);
}

static ssize_t earjack_key_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("%s : operate nothing\n", __func__);

	return size;
}

static ssize_t earjack_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_jack_info *hi = dev_get_drvdata(dev);
	int value = 0;

	if (hi->cur_jack_type == SEC_HEADSET_4POLE)
		value = 1;
	else
		value = 0;

	return sprintf(buf, "%d\n", value);
}

static ssize_t earjack_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("%s : operate nothing\n", __func__);

	return size;
}
static DEVICE_ATTR(key_state, S_IRUGO | S_IWUSR | S_IWGRP,
		   earjack_key_state_show, earjack_key_state_store);

static DEVICE_ATTR(state, S_IRUGO | S_IWUSR | S_IWGRP,
		   earjack_state_show, earjack_state_store);

static DEVICE_ATTR(select_jack, S_IRUGO | S_IWUSR | S_IWGRP,
		select_jack_show, select_jack_store);

void jack_status_change(int status)
{
	int jack_type = SEC_JACK_NO_DEVICE;

	pr_info("%s status=%d\n", __func__, status);

	wake_lock_timeout(&hi->det_wake_lock, WAKE_LOCK_TIME);

	if (status) {
		jack_type = SEC_HEADSET_4POLE;
	} else {
		jack_type = SEC_JACK_NO_DEVICE;
		if (send_end_pressed == 1)	{
			input_report_key(hi->input_dev, KEY_MEDIA, 0);
			input_sync(hi->input_dev);
			switch_set_state(&switch_sendend, 0);
			send_end_pressed = 0;
		}
	}
	switch_set_state(&switch_jack_detection, jack_type);
	hi->cur_jack_type = jack_type;
}
EXPORT_SYMBOL(jack_status_change);

void earkey_status_change(int pressed, int kcode)
{
	wake_lock_timeout(&hi->det_wake_lock, WAKE_LOCK_TIME_IN_SENDKEY);
	hi->pressed_code = kcode;
	if (pressed) {
		input_report_key(hi->input_dev, hi->pressed_code, 1);
		switch_set_state(&switch_sendend, 1);
		input_sync(hi->input_dev);
		send_end_pressed = 1;
	} else {
		input_report_key(hi->input_dev, hi->pressed_code, 0);
		switch_set_state(&switch_sendend, 0);
		input_sync(hi->input_dev);
		send_end_pressed = 0;
	}
	hi->pressed = send_end_pressed;
}
EXPORT_SYMBOL(earkey_status_change);

static int sec_jack_probe(struct platform_device *pdev)
{
	struct sec_jack_platform_data *pdata = pdev->dev.platform_data;
	int ret;

	pr_info("%s : Registering jack driver\n", __func__);
	if (!pdata) {
		pr_err("%s : pdata is NULL.\n", __func__);
		return -ENODEV;
	}

	if (atomic_xchg(&instantiated, 1)) {
		pr_err("%s : already instantiated, can only have one\n",
			__func__);
		return -ENODEV;
	}

	hi = kzalloc(sizeof(struct sec_jack_info), GFP_KERNEL);
	if (hi == NULL) {
		pr_err("%s : Failed to allocate memory.\n", __func__);
		ret = -ENOMEM;
		goto err_kzalloc;
	}

	hi->pdata = pdata;
	send_end_pressed = 0;

	/* make the id of our gpio_event device the same as our platform device,
	 * which makes it the responsiblity of the board file to make sure
	 * it is unique relative to other gpio_event devices
	 */
	hi->dev_id = pdev->id;

	hi->input_dev = input_allocate_device();
	if (hi->input_dev == NULL) {
		ret = -ENOMEM;
		pr_err("%s : Failed to allocate input device.\n", __func__);
	}
	pr_info("%s input device is [%s]\n", __func__, hi->input_dev->name);

	hi->input_dev->name = "sec_jack";
	input_set_capability(hi->input_dev , EV_KEY, KEY_MEDIA);
	input_set_capability(hi->input_dev , EV_KEY, KEY_VOLUMEDOWN);
	input_set_capability(hi->input_dev , EV_KEY, KEY_VOLUMEUP);
	ret = input_register_device(hi->input_dev);
	if (ret) {
		pr_err("%s : Failed to register driver\n", __func__);
		goto err_register_input_dev;
	}

	ret = switch_dev_register(&switch_jack_detection);
	if (ret < 0) {
		pr_err("%s : Failed to register switch device\n", __func__);
		goto err_register_input_dev;
	}

	ret = switch_dev_register(&switch_sendend);
	if (ret < 0) {
		printk(KERN_ERR "SEC JACK: Failed to register switch device\n");
		goto err_switch_dev_register_send_end;
	}
	wake_lock_init(&hi->det_wake_lock, WAKE_LOCK_SUSPEND, "sec_jack_det");

	hi->queue = create_singlethread_workqueue("sec_jack_wq");
	if (hi->queue == NULL) {
		ret = -ENOMEM;
		pr_err("%s: Failed to create workqueue\n", __func__);
		goto err_create_wq_failed;
	}

	jack_class = class_create(THIS_MODULE, JACK_CLASS_NAME);
	if (IS_ERR(jack_class))
		pr_err("Failed to create class(sec_jack)\n");

	/* support PBA function test */
	jack_dev = device_create(jack_class, NULL, 0, hi, JACK_DEV_NAME);
	if (IS_ERR(jack_dev))
		pr_err("Failed to create device(sec_jack)!= %ld\n",
			IS_ERR(jack_dev));

	if (device_create_file(jack_dev, &dev_attr_select_jack) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_select_jack.attr.name);

	if (device_create_file(jack_dev, &dev_attr_key_state) < 0)
		pr_err("Failed to create device file (%s)!\n",
			dev_attr_key_state.attr.name);

	if (device_create_file(jack_dev, &dev_attr_state) < 0)
		pr_err("Failed to create device file (%s)!\n",
			dev_attr_state.attr.name);

	return 0;

err_create_wq_failed:
	destroy_workqueue(hi->queue);
	wake_lock_destroy(&hi->det_wake_lock);
	switch_dev_unregister(&switch_sendend);
err_switch_dev_register_send_end:
	switch_dev_unregister(&switch_jack_detection);
err_register_input_dev:
	input_free_device(hi->input_dev);
err_kzalloc:
	atomic_set(&instantiated, 0);

	return ret;
}

static int sec_jack_remove(struct platform_device *pdev)
{

	struct sec_jack_info *hi = dev_get_drvdata(&pdev->dev);

	pr_info("%s :\n", __func__);
	destroy_workqueue(hi->queue);
	if (hi->send_key_dev) {
		platform_device_unregister(hi->send_key_dev);
		hi->send_key_dev = NULL;
	}
	input_unregister_handler(&hi->handler);
	wake_lock_destroy(&hi->det_wake_lock);
	switch_dev_unregister(&switch_sendend);
	switch_dev_unregister(&switch_jack_detection);
	kfree(hi);
	atomic_set(&instantiated, 0);

	return 0;
}


static struct platform_driver sec_jack_driver = {
	.probe	= sec_jack_probe,
	.remove	= sec_jack_remove,
	.driver	= {
		.name = "sec_jack",
		.owner = THIS_MODULE,
	},
};

static int __init sec_jack_init(void)
{
	int ret;

	ret =  platform_driver_register(&sec_jack_driver);

	if (ret)
		pr_err("%s: Failed to add sec jack driver\n", __func__);

	return ret;
}

static void __exit sec_jack_exit(void)
{
	platform_driver_unregister(&sec_jack_driver);
}

module_init(sec_jack_init);
module_exit(sec_jack_exit);

MODULE_AUTHOR("sopia.kim@samsung.com");
MODULE_DESCRIPTION("Samsung Electronics Corp Ear-Jack detection driver");
MODULE_LICENSE("GPL");
