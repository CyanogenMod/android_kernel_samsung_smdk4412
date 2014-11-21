/*
 *  drivers/usb/notify/host_notify_class.c
 *
 * Copyright (C) 2011 Samsung, Inc.
 * Author: Dongrak Shin <dongrak.shin@samsung.com>
 *
*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/host_notify.h>

struct notify_data {
	struct class *host_notify_class;
	atomic_t device_count;
};

static struct notify_data host_notify;

static ssize_t mode_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct host_notify_dev *ndev = (struct host_notify_dev *)
	    dev_get_drvdata(dev);
	char *mode;

	switch (ndev->mode) {
	case NOTIFY_HOST_MODE:
		mode = "HOST";
		break;
	case NOTIFY_PERIPHERAL_MODE:
		mode = "PERIPHERAL";
		break;
	case NOTIFY_TEST_MODE:
		mode = "TEST";
		break;
	case NOTIFY_NONE_MODE:
	default:
		mode = "NONE";
		break;
	}

	printk(KERN_INFO "host_notify: read mode %s\n", mode);
	return sprintf(buf, "%s\n", mode);
}

static ssize_t mode_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t size)
{
	struct host_notify_dev *ndev = (struct host_notify_dev *)
	    dev_get_drvdata(dev);

	char *mode;
	size_t ret = -ENOMEM;

	mode = kzalloc(size + 1, GFP_KERNEL);
	if (!mode)
		goto error;

	sscanf(buf, "%s", mode);

	if (ndev->set_mode) {
		printk(KERN_INFO "host_notify: set mode %s\n", mode);
		if (!strcmp(mode, "HOST"))
			ndev->set_mode(NOTIFY_SET_ON);
		else if (!strcmp(mode, "NONE"))
			ndev->set_mode(NOTIFY_SET_OFF);
	}
	ret = size;
	kfree(mode);
 error:
	return ret;
}

static ssize_t booster_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct host_notify_dev *ndev = (struct host_notify_dev *)
	    dev_get_drvdata(dev);
	char *booster;

	switch (ndev->booster) {
	case NOTIFY_POWER_ON:
		booster = "ON";
		break;
	case NOTIFY_POWER_OFF:
	default:
		booster = "OFF";
		break;
	}

	printk(KERN_INFO "host_notify: read booster %s\n", booster);
	return sprintf(buf, "%s\n", booster);
}

static ssize_t booster_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t size)
{
	struct host_notify_dev *ndev = (struct host_notify_dev *)
	    dev_get_drvdata(dev);

	char *booster;
	size_t ret = -ENOMEM;

	booster = kzalloc(size + 1, GFP_KERNEL);
	if (!booster)
		goto error;

	sscanf(buf, "%s", booster);

	if (ndev->set_booster) {
		printk(KERN_INFO "host_notify: set booster %s\n", booster);
		if (!strcmp(booster, "ON")) {
			ndev->mode = NOTIFY_TEST_MODE;
			ndev->set_booster(NOTIFY_SET_ON);
		} else if (!strcmp(booster, "OFF")) {
			ndev->mode = NOTIFY_NONE_MODE;
			ndev->set_booster(NOTIFY_SET_OFF);
		}
	}
	ret = size;
	kfree(booster);
 error:
	return ret;
}

static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR | S_IWGRP, mode_show, mode_store);
static DEVICE_ATTR(booster, S_IRUGO | S_IWUSR | S_IWGRP,
		   booster_show, booster_store);

static struct attribute *host_notify_attrs[] = {
	&dev_attr_mode.attr,
	&dev_attr_booster.attr,
	NULL,
};

static struct attribute_group host_notify_attr_grp = {
	.attrs = host_notify_attrs,
};

void host_state_notify(struct host_notify_dev *ndev, int state)
{
	printk(KERN_INFO
	       "host_notify: ndev name=%s: from state=%d -> to state=%d\n",
	       ndev->name, ndev->state, state);
	if (ndev->state != state) {
		ndev->state = state;
		kobject_uevent(&ndev->dev->kobj, KOBJ_CHANGE);
	}
}
EXPORT_SYMBOL_GPL(host_state_notify);

static int host_notify_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct host_notify_dev *ndev = (struct host_notify_dev *)
	    dev_get_drvdata(dev);
	char *state;

	if (!ndev) {
		/* this happens when the device is first created */
		return 0;
	}
	switch (ndev->state) {
	case NOTIFY_HOST_ADD:
		state = "ADD";
		break;
	case NOTIFY_HOST_REMOVE:
		state = "REMOVE";
		break;
	case NOTIFY_HOST_OVERCURRENT:
		state = "OVERCURRENT";
		break;
	case NOTIFY_HOST_LOWBATT:
		state = "LOWBATT";
		break;
	case NOTIFY_HOST_UNKNOWN:
		state = "UNKNOWN";
		break;
	case NOTIFY_HOST_NONE:
	default:
		return 0;
	}
	if (add_uevent_var(env, "DEVNAME=%s", ndev->dev->kobj.name))
		return -ENOMEM;
	if (add_uevent_var(env, "STATE=%s", state))
		return -ENOMEM;
	return 0;
}

static int create_notify_class(void)
{
	if (!host_notify.host_notify_class) {
		host_notify.host_notify_class
		    = class_create(THIS_MODULE, "host_notify");
		if (IS_ERR(host_notify.host_notify_class))
			return PTR_ERR(host_notify.host_notify_class);
		atomic_set(&host_notify.device_count, 0);
		host_notify.host_notify_class->dev_uevent = host_notify_uevent;
	}

	return 0;
}

int host_notify_dev_register(struct host_notify_dev *ndev)
{
	int ret;

	if (!host_notify.host_notify_class) {
		ret = create_notify_class();
		if (ret < 0)
			return ret;
	}

	ndev->index = atomic_inc_return(&host_notify.device_count);
	ndev->dev = device_create(host_notify.host_notify_class, NULL,
				  MKDEV(0, ndev->index), NULL, ndev->name);
	if (IS_ERR(ndev->dev))
		return PTR_ERR(ndev->dev);

	ret = sysfs_create_group(&ndev->dev->kobj, &host_notify_attr_grp);
	if (ret < 0) {
		device_destroy(host_notify.host_notify_class,
			       MKDEV(0, ndev->index));
		return ret;
	}

	dev_set_drvdata(ndev->dev, ndev);
	ndev->state = 0;
	return 0;
}
EXPORT_SYMBOL_GPL(host_notify_dev_register);

void host_notify_dev_unregister(struct host_notify_dev *ndev)
{
	ndev->state = NOTIFY_HOST_NONE;
	sysfs_remove_group(&ndev->dev->kobj, &host_notify_attr_grp);
	device_destroy(host_notify.host_notify_class, MKDEV(0, ndev->index));
	dev_set_drvdata(ndev->dev, NULL);
}
EXPORT_SYMBOL_GPL(host_notify_dev_unregister);

static int __init notify_class_init(void)
{
	return create_notify_class();
}

static void __exit notify_class_exit(void)
{
	class_destroy(host_notify.host_notify_class);
}

module_init(notify_class_init);
module_exit(notify_class_exit);

MODULE_AUTHOR("Dongrak Shin <dongrak.shin@samsung.com>");
MODULE_DESCRIPTION("Usb host notify driver");
MODULE_LICENSE("GPL");
