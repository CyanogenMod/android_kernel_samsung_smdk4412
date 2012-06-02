/*
 *  max8922-charger.c
 *  MAXIM 8922 charger interface driver
 *
 *  Copyright (C) 2011 Samsung Electronics
 *
 *  <ms925.kim@samsung.com>
 *
 *  Copyright (C) 2012 Samsung Electronics
 *  Jaecheol kim <jc22.kim@samsung.com>
 *
 *  modify S2PLUS usb charger based on max8922_charger_u1.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/power/max8922_charger_u1.h>
#include <linux/battery/samsung_battery.h>

struct max8922_info {
	struct device *dev;
	struct power_supply psy_bat;
	struct max8922_platform_data *pdata;
	bool is_usb_cable;
	int irq_chg_ing;
};

static enum power_supply_property max8922_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
};

static inline int max8922_is_charging(struct max8922_info *info)
{
	int ta_nconnected = gpio_get_value(info->pdata->gpio_ta_nconnected);
	int chg_ing = gpio_get_value(info->pdata->gpio_chg_ing);

	dev_info(info->dev, "%s: charging state = 0x%x\n", __func__,
		 (ta_nconnected << 1) | chg_ing);

	/*return (ta_nconnected == 0 && chg_ing == 0); */
	return (ta_nconnected << 1) | chg_ing;
}

static int max8922_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct max8922_info *info =
	    container_of(psy, struct max8922_info, psy_bat);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		switch (max8922_is_charging(info)) {
		case 0:
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
			break;
		case 1:
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		default:
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			break;
		}
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		/* battery is always online */
		val->intval = 1;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int max8922_enable_charging(struct max8922_info *info, bool enable)
{
	int gpio_chg_en = info->pdata->gpio_chg_en;
	unsigned long flags;

	dev_info(info->dev, "%s: %s charging,%s\n", __func__,
		 enable ? "enable" : "disable",
		 info->is_usb_cable ? "USB" : "TA");

	if (enable) {
		if (info->is_usb_cable) {
			/* Charging by USB cable */
			gpio_direction_output(gpio_chg_en, GPIO_LEVEL_HIGH);
		} else {
			/* Charging by TA cable */
			gpio_direction_output(gpio_chg_en, GPIO_LEVEL_HIGH);
			mdelay(5);

			local_irq_save(flags);
			gpio_direction_output(gpio_chg_en, GPIO_LEVEL_LOW);
			udelay(300);
			gpio_direction_output(gpio_chg_en, GPIO_LEVEL_HIGH);
			local_irq_restore(flags);
		}
	} else
		gpio_direction_output(gpio_chg_en, GPIO_LEVEL_LOW);

	msleep(300);
	return max8922_is_charging(info);
}

static int max8922_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct max8922_info *info =
	    container_of(psy, struct max8922_info, psy_bat);
	bool enable;

	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_NOW:	/* Set charging current */
		info->is_usb_cable = (val->intval <= CHARGER_USB_CURRENT);
		break;
	case POWER_SUPPLY_PROP_STATUS:	/* Enable/Disable charging */
		enable = (val->intval == POWER_SUPPLY_STATUS_CHARGING);
		max8922_enable_charging(info, enable);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static irqreturn_t max8922_chg_ing_irq(int irq, void *data)
{
	struct max8922_info *info = data;
	int ret = 0;

	dev_info(info->dev, "chg_ing IRQ occurred!\n");

	if (gpio_get_value(info->pdata->gpio_ta_nconnected))
		return IRQ_HANDLED;

	if (info->pdata->topoff_cb)
		ret = info->pdata->topoff_cb();

	if (ret) {
		dev_err(info->dev, "%s: error from topoff_cb(%d)\n", __func__,
			ret);
		return IRQ_HANDLED;
	}

	return IRQ_HANDLED;
}

static __devinit int max8922_probe(struct platform_device *pdev)
{
	struct max8922_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct max8922_info *info;
	int ret;

	dev_info(&pdev->dev, "%s : MAX8922 Charger Driver Loading\n", __func__);

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	platform_set_drvdata(pdev, info);

	info->dev = &pdev->dev;
	info->pdata = pdata;

	info->psy_bat.name = "max8922-charger",
	    info->psy_bat.type = POWER_SUPPLY_TYPE_BATTERY,
	    info->psy_bat.properties = max8922_battery_props,
	    info->psy_bat.num_properties = ARRAY_SIZE(max8922_battery_props),
	    info->psy_bat.get_property = max8922_get_property,
	    info->psy_bat.set_property = max8922_set_property,
	    ret = power_supply_register(&pdev->dev, &info->psy_bat);
	if (ret) {
		dev_err(info->dev, "Failed to register psy_bat\n");
		goto err_kfree;
	}

	if (pdata->cfg_gpio) {
		ret = pdata->cfg_gpio();
		if (ret) {
			dev_err(info->dev, "failed to configure GPIO\n");
			goto err_kfree;
		}
	}

	if (gpio_is_valid(pdata->gpio_chg_en)) {
		if (!pdata->gpio_chg_en) {
			dev_err(info->dev, "gpio_chg_en defined as 0\n");
			WARN_ON(!pdata->gpio_chg_en);
			ret = -EIO;
			goto err_kfree;
		}
		gpio_request(pdata->gpio_chg_en, "MAX8922 CHG_EN");
	}

	if (gpio_is_valid(pdata->gpio_chg_ing)) {
		if (!pdata->gpio_chg_ing) {
			dev_err(info->dev, "gpio_chg_ing defined as 0\n");
			WARN_ON(!pdata->gpio_chg_ing);
			ret = -EIO;
			goto err_kfree;
		}
		gpio_request(pdata->gpio_chg_ing, "MAX8922 CHG_ING");
	}

	if (gpio_is_valid(pdata->gpio_ta_nconnected)) {
		if (!pdata->gpio_ta_nconnected) {
			dev_err(info->dev, "gpio_ta_nconnected defined as 0\n");
			WARN_ON(!pdata->gpio_ta_nconnected);
			ret = -EIO;
			goto err_kfree;
		}
		gpio_request(pdata->gpio_ta_nconnected,
			     "MAX8922 TA_nCONNECTED");
	}
#if 0
	info->irq_chg_ing = gpio_to_irq(pdata->gpio_chg_ing);

	ret = request_threaded_irq(info->irq_chg_ing, NULL,
				   max8922_chg_ing_irq,
				   IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				   "chg_ing", info);
	if (ret)
		dev_err(&pdev->dev, "%s: fail to request chg_ing IRQ:"
			" %d: %d\n", __func__, info->irq_chg_ing, ret);
#endif

	return 0;
err_kfree:
	power_supply_unregister(&info->psy_bat);
	platform_set_drvdata(pdev, NULL);
	kfree(info);
	return ret;
}

static int __devexit max8922_remove(struct platform_device *pdev)
{
	struct max8922_info *info = platform_get_drvdata(pdev);

	power_supply_unregister(&info->psy_bat);

	gpio_free(info->pdata->gpio_chg_en);
	gpio_free(info->pdata->gpio_chg_ing);
	gpio_free(info->pdata->gpio_ta_nconnected);

	kfree(info);

	return 0;
}

#ifdef CONFIG_PM
static int max8922_suspend(struct device *dev)
{
	struct max8922_info *info = dev_get_drvdata(dev);

	if (info && info->irq_chg_ing)
		disable_irq(info->irq_chg_ing);

	return 0;
}

static int max8922_resume(struct device *dev)
{
	struct max8922_info *info = dev_get_drvdata(dev);

	if (info && info->irq_chg_ing)
		enable_irq(info->irq_chg_ing);

	return 0;
}
#else
#define max8922_charger_suspend	NULL
#define max8922_charger_resume	NULL
#endif

static const struct dev_pm_ops max8922_pm_ops = {
	.suspend = max8922_suspend,
	.resume = max8922_resume,
};

static struct platform_driver max8922_driver = {
	.driver = {
		   .name = "max8922-charger",
		   .owner = THIS_MODULE,
		   .pm = &max8922_pm_ops,
		   },
	.probe = max8922_probe,
	.remove = __devexit_p(max8922_remove),
};

static int __init max8922_init(void)
{
	return platform_driver_register(&max8922_driver);
}

static void __exit max8922_exit(void)
{
	platform_driver_register(&max8922_driver);
}

module_init(max8922_init);
module_exit(max8922_exit);

MODULE_DESCRIPTION("MAXIM 8922 charger control driver");
MODULE_AUTHOR("<ms925.kim@samsung.com>");
MODULE_LICENSE("GPL");
