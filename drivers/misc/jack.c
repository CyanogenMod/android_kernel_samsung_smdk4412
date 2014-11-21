/*
 *  Jack Monitoring Interface
 *
 *  Copyright (C) 2009 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/jack.h>
#include <linux/slab.h>

struct jack_data {
	struct jack_platform_data	*pdata;
};

static struct platform_device *jack_dev;

static void jack_set_data(struct jack_platform_data *pdata,
		const char *name, int value)
{
	if (!strcmp(name, "usb"))
		pdata->usb_online = value;
	else if (!strcmp(name, "charger"))
		pdata->charger_online = value;
	else if (!strcmp(name, "hdmi"))
		pdata->hdmi_online = value;
	else if (!strcmp(name, "earjack"))
		pdata->earjack_online = value;
	else if (!strcmp(name, "earkey"))
		pdata->earkey_online = value;
	else if (!strcmp(name, "ums"))
		pdata->ums_online = value;
	else if (!strcmp(name, "cdrom"))
		pdata->cdrom_online = value;
	else if (!strcmp(name, "jig"))
		pdata->jig_online = value;
	else if (!strcmp(name, "host"))
		pdata->host_online = value;
	else if (!strcmp(name, "cradle"))
		pdata->cradle_online = value;
}

int jack_get_data(const char *name)
{
	struct jack_data *jack = platform_get_drvdata(jack_dev);

	if (!strcmp(name, "usb"))
		return jack->pdata->usb_online;
	else if (!strcmp(name, "charger"))
		return jack->pdata->charger_online;
	else if (!strcmp(name, "hdmi"))
		return jack->pdata->hdmi_online;
	else if (!strcmp(name, "earjack"))
		return jack->pdata->earjack_online;
	else if (!strcmp(name, "earkey"))
		return jack->pdata->earkey_online;
	else if (!strcmp(name, "ums"))
		return jack->pdata->ums_online;
	else if (!strcmp(name, "cdrom"))
		return jack->pdata->cdrom_online;
	else if (!strcmp(name, "jig"))
		return jack->pdata->jig_online;
	else if (!strcmp(name, "host"))
		return jack->pdata->host_online;
	else if (!strcmp(name, "cradle"))
		return jack->pdata->cradle_online;


	return -EINVAL;
}
EXPORT_SYMBOL_GPL(jack_get_data);

void jack_event_handler(const char *name, int value)
{
	struct jack_data *jack;
	char env_str[16];
	char *envp[] = { env_str, NULL };

	if (!jack_dev) {
		printk(KERN_ERR "jack device is not allocated\n");
		return;
	}

	jack = platform_get_drvdata(jack_dev);
	jack_set_data(jack->pdata, name, value);
	sprintf(env_str, "CHGDET=%s", name);
	dev_info(&jack_dev->dev, "jack event %s\n", env_str);
	kobject_uevent_env(&jack_dev->dev.kobj, KOBJ_CHANGE, envp);
}
EXPORT_SYMBOL_GPL(jack_event_handler);

#define JACK_OUTPUT(name)						\
static ssize_t jack_show_##name(struct device *dev,			\
		struct device_attribute *attr, char *buf)		\
{									\
	struct jack_data *chip = dev_get_drvdata(dev);			\
	return sprintf(buf, "%d\n", chip->pdata->name);			\
}									\
static DEVICE_ATTR(name, S_IRUGO, jack_show_##name, NULL);

JACK_OUTPUT(usb_online);
JACK_OUTPUT(charger_online);
JACK_OUTPUT(hdmi_online);
JACK_OUTPUT(earjack_online);
JACK_OUTPUT(earkey_online);
JACK_OUTPUT(jig_online);
JACK_OUTPUT(host_online);
JACK_OUTPUT(cradle_online);

static int jack_device_init(struct jack_data *jack)
{
	struct jack_platform_data *pdata = jack->pdata;
	int ret;

	if (pdata->usb_online != -1)
		ret = device_create_file(&jack_dev->dev,
				&dev_attr_usb_online);
	if (pdata->charger_online != -1)
		ret = device_create_file(&jack_dev->dev,
				&dev_attr_charger_online);
	if (pdata->hdmi_online != -1)
		ret = device_create_file(&jack_dev->dev,
				&dev_attr_hdmi_online);
	if (pdata->earjack_online != -1)
		ret = device_create_file(&jack_dev->dev,
				&dev_attr_earjack_online);
	if (pdata->earkey_online != -1)
		ret = device_create_file(&jack_dev->dev,
				&dev_attr_earkey_online);
	if (pdata->jig_online != -1)
		ret = device_create_file(&jack_dev->dev,
				&dev_attr_jig_online);
	if (pdata->host_online != -1)
		ret = device_create_file(&jack_dev->dev,
				&dev_attr_host_online);
	if (pdata->cradle_online != -1)
		ret = device_create_file(&jack_dev->dev,
				&dev_attr_cradle_online);

	return 0;
}

static int __devinit jack_probe(struct platform_device *pdev)
{
	struct jack_platform_data *pdata = pdev->dev.platform_data;
	struct jack_data *jack;

	jack = kzalloc(sizeof(struct jack_data), GFP_KERNEL);
	if (!jack) {
		dev_err(&pdev->dev, "failed to allocate driver data\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, jack);
	jack_dev = pdev;
	jack->pdata = pdata;

	jack_device_init(jack);

	return 0;
}

static int __devexit jack_remove(struct platform_device *pdev)
{
	struct jack_platform_data *pdata = pdev->dev.platform_data;

	if (pdata->usb_online != -1)
		device_remove_file(&jack_dev->dev, &dev_attr_usb_online);
	if (pdata->charger_online != -1)
		device_remove_file(&jack_dev->dev, &dev_attr_charger_online);
	if (pdata->hdmi_online != -1)
		device_remove_file(&jack_dev->dev, &dev_attr_hdmi_online);
	if (pdata->earjack_online != -1)
		device_remove_file(&jack_dev->dev, &dev_attr_earjack_online);
	if (pdata->earkey_online != -1)
		device_remove_file(&jack_dev->dev, &dev_attr_earkey_online);
	if (pdata->jig_online != -1)
		device_remove_file(&jack_dev->dev, &dev_attr_jig_online);
	if (pdata->host_online != -1)
		device_remove_file(&jack_dev->dev, &dev_attr_host_online);
	if (pdata->cradle_online != -1)
		device_remove_file(&jack_dev->dev, &dev_attr_cradle_online);


	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver jack_driver = {
	.probe		= jack_probe,
	.remove		= __devexit_p(jack_remove),
	.driver		= {
		.name	= "jack",
		.owner	= THIS_MODULE,
	},
};

static int __init jack_init(void)
{
	return platform_driver_register(&jack_driver);
}
module_init(jack_init);

static void __exit jack_exit(void)
{
	platform_driver_unregister(&jack_driver);
}
module_exit(jack_exit);

MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
MODULE_DESCRIPTION("Jack Monitoring Interface");
MODULE_LICENSE("GPL");
