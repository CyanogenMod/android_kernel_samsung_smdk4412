/* linux/drivers/video/samsung/s5p_fimd_ext.c
 *
 * Samsung SoC FIMD Extension Device Framework.
 *
 * Inki Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include <plat/fimd_lite_ext.h>

#include "s5p_fimd_ext.h"

struct s5p_fimd_ext {
	struct list_head		list;
	struct device			*dev;
};

static struct class_compat *s5p_fimd_ext_cls;
static LIST_HEAD(fimd_ext_list);
static DEFINE_MUTEX(fimd_ext_lock);

struct s5p_fimd_ext_device *to_fimd_ext_device(struct device *dev)
{
	return dev ? container_of(dev, struct s5p_fimd_ext_device, dev) : NULL;
}
EXPORT_SYMBOL(to_fimd_ext_device);

struct s5p_fimd_ext_driver *to_fimd_ext_driver(struct device_driver *drv)
{
	return drv ? container_of(drv, struct s5p_fimd_ext_driver, driver) :
		NULL;
}
EXPORT_SYMBOL(to_fimd_ext_driver);

static ssize_t modalias_show(struct device *dev, struct device_attribute *a,
			     char *buf)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	int len = snprintf(buf, PAGE_SIZE, "platform:%s\n", fx_dev->name);

	return (len >= PAGE_SIZE) ? (PAGE_SIZE - 1) : len;
}

static struct device_attribute s5p_fimd_ext_dev_attrs[] = {
	__ATTR_RO(modalias),
	__ATTR_NULL,
};

static int s5p_fimd_ext_match_device(struct device *dev,
					struct device_driver *drv)
{
	const struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	const struct s5p_fimd_ext_driver *fx_drv = to_fimd_ext_driver(drv);

	return strcmp(fx_dev->name, fx_drv->driver.name) == 0;
}

struct bus_type s5p_fimd_ext_bus_type = {
	.name		= "fimd_ext",
	.dev_attrs	= s5p_fimd_ext_dev_attrs,
	.match		= s5p_fimd_ext_match_device,
};

static int s5p_fimd_ext_drv_probe(struct device *dev)
{
	const struct s5p_fimd_ext_driver *fx_drv =
			to_fimd_ext_driver(dev->driver);

	return fx_drv->probe(to_fimd_ext_device(dev));
}

struct resource *s5p_fimd_ext_get_resource(struct s5p_fimd_ext_device *fx_dev,
		unsigned int type, unsigned int num)
{
	int i;

	for (i = 0; i < fx_dev->num_resources; i++) {
		struct resource *r = &fx_dev->resource[i];

		if (type == resource_type(r) && num-- == 0)
			return r;
	}

	return NULL;
}

int s5p_fimd_ext_get_irq(struct s5p_fimd_ext_device *fx_dev, unsigned int num)
{
	struct resource *r = s5p_fimd_ext_get_resource(fx_dev,
					IORESOURCE_IRQ, num);

	return r ? r->start : -ENXIO;
}

int s5p_fimd_ext_device_register(struct s5p_fimd_ext_device *fx_dev)
{
	struct s5p_fimd_ext *fimd_ext;
	int i, ret = 0;

	fimd_ext = kzalloc(sizeof(struct s5p_fimd_ext), GFP_KERNEL);
	if (!fimd_ext) {
		printk(KERN_ERR "failed to allocate fimd_ext object.\n");
		return -EFAULT;
	}

	fimd_ext->dev = &fx_dev->dev;

	device_initialize(&fx_dev->dev);

	if (!fx_dev->dev.parent)
		fx_dev->dev.parent = &platform_bus;

	fx_dev->dev.bus = &s5p_fimd_ext_bus_type;

	if (fx_dev->id != -1)
		dev_set_name(&fx_dev->dev, "%s.%d", fx_dev->name, fx_dev->id);
	else
		dev_set_name(&fx_dev->dev, "%s", fx_dev->name);

	for (i = 0; i < fx_dev->num_resources; i++) {
		struct resource *r = &fx_dev->resource[i];

		if (r->name == NULL)
			r->name = dev_name(&fx_dev->dev);
	}

	ret = device_add(&fx_dev->dev);
	if (ret)
		goto err_clear;

	mutex_lock(&fimd_ext_lock);
	list_add_tail(&fimd_ext->list, &fimd_ext_list);
	mutex_unlock(&fimd_ext_lock);

	ret = class_compat_create_link(s5p_fimd_ext_cls, &fx_dev->dev,
			fx_dev->dev.parent);
	if (ret) {
		dev_err(&fx_dev->dev, "failed to create compatibility link");
		goto err_del;
	}

	return 0;

err_del:
	list_del(&fimd_ext->list);
err_clear:
	kfree(fimd_ext);

	return ret;
}
EXPORT_SYMBOL(s5p_fimd_ext_device_register);

int s5p_fimd_ext_driver_register(struct s5p_fimd_ext_driver *fx_drv)
{
	fx_drv->driver.bus = &s5p_fimd_ext_bus_type;
	if (fx_drv->probe)
		fx_drv->driver.probe = s5p_fimd_ext_drv_probe;

	/* add some callbacks here. */

	printk(KERN_DEBUG "registered a driver(%s) to fimd_ext driver.\n",
			fx_drv->driver.name);

	return driver_register(&fx_drv->driver);
}
EXPORT_SYMBOL(s5p_fimd_ext_driver_register);

struct s5p_fimd_ext_device *s5p_fimd_ext_find_device(const char *name)
{
	struct s5p_fimd_ext *fimd_ext;
	struct s5p_fimd_ext_device *fx_dev;
	struct device *dev;

	mutex_lock(&fimd_ext_lock);

	printk(KERN_DEBUG "find ext driver (%s).\n", name);

	list_for_each_entry(fimd_ext, &fimd_ext_list, list) {
		dev = fimd_ext->dev;
		fx_dev = to_fimd_ext_device(dev);

		if ((strcmp(fx_dev->name, name)) == 0) {
			mutex_unlock(&fimd_ext_lock);
			printk(KERN_DEBUG "found!!!(%s).\n", fx_dev->name);
			return fx_dev;
		}
	}

	printk(KERN_WARNING "failed to find ext device(%s).\n", name);

	mutex_unlock(&fimd_ext_lock);

	return NULL;
}

static int __init s5p_fimd_ext_init(void)
{
	int ret;

	ret = bus_register(&s5p_fimd_ext_bus_type);
	if (ret)
		return ret;

	s5p_fimd_ext_cls = class_compat_register("extension");
	if (!s5p_fimd_ext_cls) {
		ret = -ENOMEM;
		goto err_unreg;
	}

	return 0;

err_unreg:
	bus_unregister(&s5p_fimd_ext_bus_type);

	return ret;
}

static void __exit s5p_fimd_ext_exit(void)
{
	class_compat_unregister(s5p_fimd_ext_cls);
	bus_unregister(&s5p_fimd_ext_bus_type);
}

postcore_initcall(s5p_fimd_ext_init);
module_exit(s5p_fimd_ext_exit);

MODULE_AUTHOR("InKi Dae <inki.dae@samsung.com>");
MODULE_DESCRIPTION("Samsung SoC FIMD Extension Framework");
MODULE_LICENSE("GPL");
