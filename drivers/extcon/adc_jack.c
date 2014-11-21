/*
 * drivers/extcon/adc_jack.c
 *
 * Analog Jack extcon driver with ADC-based detection capability.
 *
 * Copyright (C) 2012 Samsung Electronics
 * MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/slab.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/extcon.h>
#include <linux/extcon/adc_jack.h>

static void adc_jack_handler(struct work_struct *work)
{
	struct adc_jack_data *data = container_of(to_delayed_work(work),
						  struct adc_jack_data,
						  handler);
	u32 adc, state = 0;
	int ret;
	int i;

	if (!data->ready)
		return;

	ret = data->get_adc(&adc);
	if (ret) {
		dev_err(data->edev.dev, "get_adc() error: %d\n", ret);
		return;
	}

	/* Get state from adc value with adc_condition */
	for (i = 0; i < data->num_conditions; i++) {
		struct adc_jack_cond *def = &data->adc_condition[i];
		if (!def->state)
			break;
		if (def->min_adc <= adc && def->max_adc >= adc) {
			state = def->state;
			break;
		}
	}
	/* if no def has met, it means state = 0 (no cables attached) */

	extcon_set_state(&data->edev, state);
}

static irqreturn_t adc_jack_irq_thread(int irq, void *_data)
{
	struct adc_jack_data *data = _data;

	schedule_delayed_work(&data->handler, data->handling_delay);

	return IRQ_HANDLED;
}

static int adc_jack_probe(struct platform_device *pdev)
{
	struct adc_jack_data *data;
	struct adc_jack_pdata *pdata = pdev->dev.platform_data;
	int i, err = 0;

	data = kzalloc(sizeof(struct adc_jack_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->edev.name = pdata->name;

	if (pdata->cable_names)
		data->edev.supported_cable = pdata->cable_names;
	else
		data->edev.supported_cable = extcon_cable_name;

	/* Check the length of array and set num_cables */
	for (i = 0; data->edev.supported_cable[i]; i++)
		;
	if (i == 0 || i > SUPPORTED_CABLE_MAX) {
		err = -EINVAL;
		dev_err(&pdev->dev, "error: pdata->cable_names size = %d, which uses u32 bitmask and cannot exceed 32.\n",
			i - 1);
		goto err_alloc;
	}
	data->num_cables = i;

	if (!pdata->adc_condition ||
	    !pdata->adc_condition[0].state) {
		err = -EINVAL;
		dev_err(&pdev->dev, "error: adc_condition not defined.\n");
		goto err_alloc;
	}
	data->adc_condition = pdata->adc_condition;

	/* Check the length of array and set num_conditions */
	for (i = 0; data->adc_condition[i].state; i++)
		;
	data->num_conditions = i;

	if (!pdata->get_adc) {
		dev_err(&pdev->dev, "error: get_adc() is mandatory.\n");
		err = -EINVAL;
		goto err_alloc;
	}
	data->get_adc = pdata->get_adc;
	data->handling_delay = msecs_to_jiffies(pdata->handling_delay_ms);

	INIT_DELAYED_WORK_DEFERRABLE(&data->handler, adc_jack_handler);

	platform_set_drvdata(pdev, data);

	if (pdata->irq) {
		data->irq = pdata->irq;

		err = request_threaded_irq(data->irq, NULL,
					   adc_jack_irq_thread,
					   pdata->irq_flags, pdata->name,
					   data);

		if (err) {
			dev_err(&pdev->dev, "error: irq %d\n", data->irq);
			err = -EINVAL;
			goto err_initwork;
		}
	}
	err = extcon_dev_register(&data->edev, &pdev->dev);
	if (err)
		goto err_irq;

	data->ready = true;

	goto out;

err_irq:
	if (data->irq)
		free_irq(data->irq, data);
err_initwork:
	cancel_delayed_work_sync(&data->handler);
err_alloc:
	kfree(data);
out:
	return err;
}

static int __devexit adc_jack_remove(struct platform_device *pdev)
{
	struct adc_jack_data *data = platform_get_drvdata(pdev);

	extcon_dev_unregister(&data->edev);
	if (data->irq)
		free_irq(data->irq, data);
	platform_set_drvdata(pdev, NULL);
	kfree(data);

	return 0;
}

static struct platform_driver adc_jack_driver = {
	.probe		= adc_jack_probe,
	.remove		= __devexit_p(adc_jack_remove),
	.driver		= {
		.name	= "adc-jack",
		.owner	= THIS_MODULE,
	},
};

#define module_platform_driver(__platform_driver) \
	module_driver(__platform_driver, platform_driver_register, \
			platform_driver_unregister)
#define module_driver(__driver, __register, __unregister) \
static int __init __driver##_init(void) \
{ \
	return __register(&(__driver)); \
} \
module_init(__driver##_init); \
static void __exit __driver##_exit(void) \
{ \
	__unregister(&(__driver)); \
} \
module_exit(__driver##_exit);
module_platform_driver(adc_jack_driver);

MODULE_AUTHOR("MyungJoo Ham <myungjoo.ham@samsung.com>");
MODULE_DESCRIPTION("ADC jack extcon driver");
MODULE_LICENSE("GPL");
