/* drivers/devfreq/exynos4_display.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 *	Chanwoo Choi <cw00.choi@samsung.com>
 *	Myungjoo Ham <myungjoo.ham@samsung.com>
 *	Kyungmin Park <kyungmin.park@samsung.com>
 *
 * EXYNOS4 - Dynamic LCD refresh rate support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/opp.h>
#include <linux/mutex.h>
#include <linux/suspend.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/devfreq.h>
#include <linux/devfreq/exynos4_display.h>
#include <linux/pm_qos_params.h>

#define EXYNOS4_DISPLAY_ON	1
#define EXYNOS4_DISPLAY_OFF	0

#define DEFAULT_DELAY_TIME	1000	/* us (millisecond) */

enum exynos4_display_type {
	TYPE_DISPLAY_EXYNOS4210,
	TYPE_DISPLAY_EXYNOS4x12,
};

struct exynos4_display_data {
	enum exynos4_display_type type;
	struct devfreq *devfreq;
	struct opp *curr_opp;
	struct device *dev;

	struct delayed_work wq_lowfreq;

	struct notifier_block nb_pm;

	unsigned int state;
	struct mutex lock;
};

/* Define frequency level */
enum display_clk_level_idx {
	LV_0 = 0,
	LV_1,
	_LV_END
};

/* Define opp table which include various frequency level */
struct display_opp_table {
	unsigned int idx;
	unsigned long clk;
	unsigned long volt;
};

static struct display_opp_table exynos4_display_clk_table[] = {
	{LV_0, 40, 0 },
	{LV_1, 60, 0 },
	{0, 0, 0 },
};

/*
 * The exynos-display driver send newly frequency to display client
 * if it receive event from sender device.
 *	List of display client device
 *	- FIMD and so on
 */
static BLOCKING_NOTIFIER_HEAD(exynos4_display_notifier_client_list);

int exynos4_display_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(
			&exynos4_display_notifier_client_list, nb);
}
EXPORT_SYMBOL(exynos4_display_register_client);

int exynos4_display_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(
			&exynos4_display_notifier_client_list, nb);
}
EXPORT_SYMBOL(exynos4_display_unregister_client);

static int exynos4_display_send_event_to_display(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(
			&exynos4_display_notifier_client_list, val, v);
}

/*
 * Register exynos-display as client to pm notifer
 * - This callback gets called when something important happens in pm state.
 */
static int exynos4_display_pm_notifier_callback(struct notifier_block *this,
				 unsigned long event, void *_data)
{
	struct exynos4_display_data *data = container_of(this,
			struct exynos4_display_data, nb_pm);

	if (data->state == EXYNOS4_DISPLAY_OFF)
		return NOTIFY_OK;

	switch (event) {
	case PM_SUSPEND_PREPARE:
		mutex_lock(&data->lock);
		data->state = EXYNOS4_DISPLAY_OFF;
		mutex_unlock(&data->lock);

		if (delayed_work_pending(&data->wq_lowfreq))
			cancel_delayed_work(&data->wq_lowfreq);

		return NOTIFY_OK;
	case PM_POST_RESTORE:
	case PM_POST_SUSPEND:
		mutex_lock(&data->lock);
		data->state = EXYNOS4_DISPLAY_ON;
		mutex_unlock(&data->lock);

		return NOTIFY_OK;
	}

	return NOTIFY_DONE;
}

/*
 * Enable/disable exynos-display operation
 */
static void exynos4_display_disable(struct exynos4_display_data *data)
{
	struct opp *opp;
	unsigned long freq = EXYNOS4_DISPLAY_LV_DEFAULT;

	/* Cancel workqueue which set low frequency of display client
	 * if it is pending state before executing workqueue. */
	if (delayed_work_pending(&data->wq_lowfreq))
		cancel_delayed_work(&data->wq_lowfreq);

	/* Set high frequency(default) of display client */
	exynos4_display_send_event_to_display(freq, NULL);

	mutex_lock(&data->lock);
	data->state = EXYNOS4_DISPLAY_OFF;
	mutex_unlock(&data->lock);

	/* Find opp object with high frequency */
	opp = opp_find_freq_floor(data->dev, &freq);
	if (IS_ERR(opp)) {
		dev_err(data->dev,
			"invalid initial frequency %lu kHz.\n", freq);
	} else
		data->curr_opp = opp;
}

static void exynos4_display_enable(struct exynos4_display_data *data)
{
	data->state = EXYNOS4_DISPLAY_ON;
}

/*
 * Timer to set display with low frequency state after 1 second
 */
static void exynos4_display_set_lowfreq(struct work_struct *work)
{
	exynos4_display_send_event_to_display(EXYNOS4_DISPLAY_LV_LF, NULL);
}

/*
 * Send event to display client for changing frequency when DVFREQ framework
 * update state of device
 */
static int exynos4_display_profile_target(struct device *dev,
					unsigned long *_freq, u32 options)
{
	/* Inform display client of new frequency */
	struct exynos4_display_data *data = dev_get_drvdata(dev);
	struct opp *opp = devfreq_recommended_opp(dev, _freq, options &
						  DEVFREQ_OPTION_FREQ_GLB);
	unsigned long old_freq = opp_get_freq(data->curr_opp);
	unsigned long new_freq = opp_get_freq(opp);

	/* TODO: No longer use fb notifier to identify LCD on/off state and
	   have yet alternative feature of it. So, exynos4-display change
	   refresh rate of display clinet irrespective of LCD state until
	   proper feature will be implemented. */
	if (old_freq == new_freq)
		return 0;

	opp = opp_find_freq_floor(dev, &new_freq);
	data->curr_opp = opp;

	switch (new_freq) {
	case EXYNOS4_DISPLAY_LV_HF:
		if (delayed_work_pending(&data->wq_lowfreq))
			cancel_delayed_work(&data->wq_lowfreq);

		exynos4_display_send_event_to_display(
				EXYNOS4_DISPLAY_LV_HF, NULL);
		break;
	case EXYNOS4_DISPLAY_LV_LF:
		schedule_delayed_work(&data->wq_lowfreq,
				msecs_to_jiffies(DEFAULT_DELAY_TIME));
		break;
	}

	printk(KERN_DEBUG "exynos4-display: request %ldHz from \'%s\'\n",
			new_freq, dev_name(dev));

	return 0;
}

static void exynos4_display_profile_exit(struct device *dev)
{
	/* TODO */
}

static struct devfreq_dev_profile exynos4_display_profile = {
	.initial_freq	= EXYNOS4_DISPLAY_LV_DEFAULT,
	.target		= exynos4_display_profile_target,
	.exit		= exynos4_display_profile_exit,
};

static int exynos4_display_probe(struct platform_device *pdev)
{
	struct exynos4_display_data *data;
	struct device *dev = &pdev->dev;
	struct opp *opp;
	int ret = 0;
	int i;
	struct devfreq_pm_qos_table *qos_list;

	data = kzalloc(sizeof(struct exynos4_display_data), GFP_KERNEL);
	if (!data) {
		dev_err(dev, "cannot allocate memory.\n");
		return -ENOMEM;
	}
	data->dev = dev;
	data->state = EXYNOS4_DISPLAY_ON;
	mutex_init(&data->lock);

	/* Register OPP entries */
	for (i = 0 ; i < _LV_END ; i++) {
		ret = opp_add(dev, exynos4_display_clk_table[i].clk,
			      exynos4_display_clk_table[i].volt);
		if (ret) {
			dev_err(dev, "cannot add opp entries.\n");
			goto err_alloc_mem;
		}
	}

	/* Find opp object with init frequency */
	opp = opp_find_freq_floor(dev, &exynos4_display_profile.initial_freq);
	if (IS_ERR(opp)) {
		dev_err(dev, "invalid initial frequency %lu kHz.\n",
		       exynos4_display_profile.initial_freq);
		ret = PTR_ERR(opp);
		goto err_alloc_mem;
	}
	data->curr_opp = opp;

	/* Initialize QoS */
	qos_list = kzalloc(sizeof(struct devfreq_pm_qos_table) * _LV_END,
			GFP_KERNEL);
	for (i = 0 ; i < _LV_END ; i++) {
		qos_list[i].freq = exynos4_display_clk_table[i].clk;
		qos_list[i].qos_value = exynos4_display_clk_table[i].clk;
	}
	exynos4_display_profile.qos_type = PM_QOS_DISPLAY_FREQUENCY;
	exynos4_display_profile.qos_use_max = true;
	exynos4_display_profile.qos_list = qos_list;

	/* Register exynos4_display to DEVFREQ framework */
	data->devfreq = devfreq_add_device(dev, &exynos4_display_profile,
			&devfreq_powersave, NULL);
	if (IS_ERR(data->devfreq)) {
		ret = PTR_ERR(data->devfreq);
		dev_err(dev,
			"failed to add exynos4 lcd to DEVFREQ : %d\n", ret);
		goto err_alloc_mem;
	}
	devfreq_register_opp_notifier(dev, data->devfreq);

	/* Register exynos4_display as client to pm notifier */
	memset(&data->nb_pm, 0, sizeof(data->nb_pm));
	data->nb_pm.notifier_call = exynos4_display_pm_notifier_callback;
	ret = register_pm_notifier(&data->nb_pm);
	if (ret < 0) {
		dev_err(dev, "failed to get pm notifier: %d\n", ret);
		goto err_add_devfreq;
	}

	INIT_DELAYED_WORK(&data->wq_lowfreq, exynos4_display_set_lowfreq);

	platform_set_drvdata(pdev, data);

	return 0;

err_add_devfreq:
	devfreq_remove_device(data->devfreq);
err_alloc_mem:
	kfree(data);

	return ret;
}

static int __devexit exynos4_display_remove(struct platform_device *pdev)
{
	struct exynos4_display_data *data = pdev->dev.platform_data;

	unregister_pm_notifier(&data->nb_pm);
	exynos4_display_disable(data);

	devfreq_remove_device(data->devfreq);

	kfree(exynos4_display_profile.qos_list);
	kfree(data);

	return 0;
}

#ifdef CONFIG_PM
static int exynos4_display_suspend(struct device *dev)
{
	/* TODO */
	return 0;
}

static int exynos4_display_resume(struct device *dev)
{
	/* TODO */
	return 0;
}

static const struct dev_pm_ops exynos4_display_dev_pm_ops = {
	.suspend	= exynos4_display_suspend,
	.resume		= exynos4_display_resume,
};

#define exynos4_display_DEV_PM_OPS	(&exynos4_display_dev_pm_ops)
#else
#define exynos4_display_DEV_PM_OPS	NULL
#endif	/* CONFIG_PM */

static struct platform_device_id exynos4_display_ids[] = {
	{ "exynos4210-display", TYPE_DISPLAY_EXYNOS4210 },
	{ "exynos4212-display", TYPE_DISPLAY_EXYNOS4x12 },
	{ "exynos4412-display", TYPE_DISPLAY_EXYNOS4x12 },
	{ },
};

static struct platform_driver exynos4_display_driver = {
	.probe = exynos4_display_probe,
	.remove	= __devexit_p(exynos4_display_remove),
	.id_table = exynos4_display_ids,
	.driver = {
		.name	= "exynos4-display",
		.owner	= THIS_MODULE,
		.pm	= exynos4_display_DEV_PM_OPS,
	},
};

static int __init exynos4_display_init(void)
{
	return platform_driver_register(&exynos4_display_driver);
}
late_initcall(exynos4_display_init);

static void __exit exynos4_display_exit(void)
{
	platform_driver_unregister(&exynos4_display_driver);
}
module_exit(exynos4_display_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("EXYNOS4 display driver with devfreq framework");
MODULE_AUTHOR("Chanwoo Choi <cw00.choi@samsung.com>");
MODULE_AUTHOR("Myungjoo Ham <myungjoo.ham@samsung.com>");
MODULE_AUTHOR("Kyungmin Park <kyungmin.park@samsung.com>");
MODULE_ALIAS("exynos-display");
