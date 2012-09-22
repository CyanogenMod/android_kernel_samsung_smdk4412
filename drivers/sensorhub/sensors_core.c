/*
 *  Universal sensors core class
 *
 *  Author : Ryunkyun Park <ryun.park@samsung.com>
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>

struct class *sensors_class;
static atomic_t sensor_count;
static DEFINE_MUTEX(sensors_mutex);

/*
 * Create sysfs interface
 */
static void set_sensor_attr(struct device *dev,
	struct device_attribute *attributes[])
{
	int i;

	for (i = 0; attributes[i] != NULL; i++)
		if ((device_create_file(dev, attributes[i])) < 0)
			printk(KERN_INFO "[SENSOR CORE] fail device_create_file"
				"(dev, attributes[%d])\n", i);
}

int sensors_register(struct device *dev, void * drvdata,
	struct device_attribute *attributes[], char *name)
{
	int ret = 0;

	if (!sensors_class) {
		sensors_class = class_create(THIS_MODULE, "sensors");
		if (IS_ERR(sensors_class))
			return PTR_ERR(sensors_class);
	}

	mutex_lock(&sensors_mutex);

	dev = device_create(sensors_class, NULL, 0, drvdata, "%s", name);

	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		printk(KERN_ERR "[SENSORS CORE] device_create failed!"
			"[%d]\n", ret);
		return ret;
	}

	set_sensor_attr(dev, attributes);
	atomic_inc(&sensor_count);

	mutex_unlock(&sensors_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(sensors_register);

void sensors_unregister(struct device *dev)
{
	/* TODO : Unregister device */
}
EXPORT_SYMBOL_GPL(sensors_unregister);

static int __init sensors_class_init(void)
{
	printk(KERN_INFO "[SENSORS CORE] sensors_class_init\n");
	sensors_class = class_create(THIS_MODULE, "sensors");

	if (IS_ERR(sensors_class))
		return PTR_ERR(sensors_class);

	atomic_set(&sensor_count, 0);
	sensors_class->dev_uevent = NULL;

	return 0;
}

static void __exit sensors_class_exit(void)
{
	class_destroy(sensors_class);
}

/* exported for the APM Power driver, APM emulation */
EXPORT_SYMBOL_GPL(sensors_class);

subsys_initcall(sensors_class_init);
module_exit(sensors_class_exit);

MODULE_DESCRIPTION("Universal sensors core class");
MODULE_AUTHOR("Ryunkyun Park <ryun.park@samsung.com>");
MODULE_LICENSE("GPL");
