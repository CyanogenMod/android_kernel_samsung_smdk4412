/*
 * Gadget Driver for Android
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* #define DEBUG */
/* #define VERBOSE_DEBUG */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/platform_device.h>

#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/usb/android_composite.h>

#include "gadget_chips.h"

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"
#include "composite.c"

#include "f_audio_source.c"
#include "f_mass_storage.c"
#include "u_serial.c"
#ifdef CONFIG_USB_DUN_SUPPORT
#include "serial_acm.c"
#endif
#include "f_acm.c"
#include "f_adb.c"
#ifdef CONFIG_USB_ANDROID_SAMSUNG_MTP
#include "f_mtp_samsung.c"
#else
#include "f_mtp.c"
#endif
#include "f_accessory.c"
#define USB_ETH_RNDIS y
#include "f_rndis.c"
#include "rndis.c"
#include "f_diag.c"
#include "f_dm.c"

MODULE_AUTHOR("Mike Lockwood");
MODULE_DESCRIPTION("Android Composite USB Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

static const char longname[] = "Gadget Android";

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
static int composite_string_index;
#endif
/* Default vendor and product IDs, overridden by userspace */
#define VENDOR_ID		0x18D1
#define PRODUCT_ID		0x0001

/* DM_PORT NUM : /dev/ttyGS* port number */
#define DM_PORT_NUM            1

struct android_usb_function {
	char *name;
	void *config;

	struct device *dev;
	char *dev_name;
	struct device_attribute **attributes;

	/* for android_dev.enabled_functions */
	struct list_head enabled_list;

	/* Optional: initialization during gadget bind */
	int (*init)(struct android_usb_function *, struct usb_composite_dev *);
	/* Optional: cleanup during gadget unbind */
	void (*cleanup)(struct android_usb_function *);
	/* Optional: called when the function is added the list of
	 * enabled functions */
	void (*enable)(struct android_usb_function *);
	/* Optional: called when it is removed */
	void (*disable)(struct android_usb_function *);

	int (*bind_config)(struct android_usb_function *, struct usb_configuration *);

	/* Optional: called when the configuration is removed */
	void (*unbind_config)(struct android_usb_function *, struct usb_configuration *);
	/* Optional: handle ctrl requests before the device is configured */
	int (*ctrlrequest)(struct android_usb_function *,
					struct usb_composite_dev *,
					const struct usb_ctrlrequest *);
};

struct android_dev {
	struct android_usb_function **functions;
	struct list_head enabled_functions;
	struct usb_composite_dev *cdev;
	struct device *dev;

	bool enabled;
	int disable_depth;
	struct mutex mutex;
	bool connected;
	bool sw_connected;
	struct work_struct work;
};

static struct class *android_class;
static struct android_dev *_android_dev;
static int android_bind_config(struct usb_configuration *c);
static void android_unbind_config(struct usb_configuration *c);
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
static struct android_usb_platform_data *android_usb_pdata;
#endif
/* string IDs are assigned dynamically */
#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2

static char manufacturer_string[256];
static char product_string[256];
static char serial_string[256];

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
#include "u_ncm.c"
#include "u_composite_notifier.c"
#endif
#include "u_ether.c"

/* String Table */
static struct usb_string strings_dev[] = {
	[STRING_MANUFACTURER_IDX].s = manufacturer_string,
	[STRING_PRODUCT_IDX].s = product_string,
	[STRING_SERIAL_IDX].s = serial_string,
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
	.bLength              = sizeof(device_desc),
	.bDescriptorType      = USB_DT_DEVICE,
	.bcdUSB               = __constant_cpu_to_le16(0x0200),
	.bDeviceClass         = USB_CLASS_PER_INTERFACE,
	.idVendor             = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct            = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice            = __constant_cpu_to_le16(0xffff),
	.bNumConfigurations   = 1,
};

static struct usb_configuration android_config_driver = {
	.label		= "android",
	.unbind		= android_unbind_config,
	.bConfigurationValue = 1,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower	= 0x30, /* 96ma */
};

static void android_work(struct work_struct *data)
{
	struct android_dev *dev = container_of(data, struct android_dev, work);
	struct usb_composite_dev *cdev = dev->cdev;
	char *disconnected[2] = { "USB_STATE=DISCONNECTED", NULL };
	char *connected[2]    = { "USB_STATE=CONNECTED", NULL };
	char *configured[2]   = { "USB_STATE=CONFIGURED", NULL };
	char **uevent_envp = NULL;
	unsigned long flags;

	printk(KERN_DEBUG "usb: %s config=%p,connected=%d,sw_connected=%d\n",
			__func__, cdev->config, dev->connected,
			dev->sw_connected);
	spin_lock_irqsave(&cdev->lock, flags);
        if (cdev->config)
		uevent_envp = configured;
	else if (dev->connected != dev->sw_connected)
		uevent_envp = dev->connected ? connected : disconnected;
	dev->sw_connected = dev->connected;
	spin_unlock_irqrestore(&cdev->lock, flags);

	if (uevent_envp) {
		kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE, uevent_envp);
		printk(KERN_DEBUG "usb: %s sent uevent %s\n",
			 __func__, uevent_envp[0]);
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
		if (uevent_envp != connected)
			blocking_notifier_call_chain(
				&usb_composite_notifier_list,
				(unsigned long)dev->connected,
				(void *)function_lists_string);
#endif

	} else {
		printk(KERN_DEBUG "usb: %s did not send uevent (%d %d %p)\n",
		 __func__, dev->connected, dev->sw_connected, cdev->config);
	}
}

static void android_enable(struct android_dev *dev)
{
	struct usb_composite_dev *cdev = dev->cdev;

	BUG_ON(!mutex_is_locked(&dev->mutex));
	BUG_ON(!dev->disable_depth);

	if (--dev->disable_depth == 0) {
		usb_add_config(cdev, &android_config_driver,
					android_bind_config);
		usb_gadget_connect(cdev->gadget);
	}
}

static void android_disable(struct android_dev *dev)
{
	struct usb_composite_dev *cdev = dev->cdev;

	BUG_ON(!mutex_is_locked(&dev->mutex));

	if (dev->disable_depth++ == 0) {
		usb_gadget_disconnect(cdev->gadget);
		/* Cancel pending control requests */
		usb_ep_dequeue(cdev->gadget->ep0, cdev->req);
		usb_remove_config(cdev, &android_config_driver);
	}
}

/*-------------------------------------------------------------------------*/
/* Supported functions initialization */

struct adb_data {
	bool opened;
	bool enabled;
};

static int adb_function_init(struct android_usb_function *f, struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct adb_data), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;

	return adb_setup();
}

static void adb_function_cleanup(struct android_usb_function *f)
{
	adb_cleanup();
	kfree(f->config);
}

static int adb_function_bind_config(struct android_usb_function *f, struct usb_configuration *c)
{
	return adb_bind_config(c);
}

static void adb_android_function_enable(struct android_usb_function *f)
{
	struct android_dev *dev = _android_dev;
	struct adb_data *data = f->config;

	data->enabled = true;

	/* Disable the gadget until adbd is ready */
	if (!data->opened)
		android_disable(dev);
}

static void adb_android_function_disable(struct android_usb_function *f)
{
	struct android_dev *dev = _android_dev;
	struct adb_data *data = f->config;

	data->enabled = false;

	/* Balance the disable that was called in closed_callback */
	if (!data->opened)
		android_enable(dev);
}

static struct android_usb_function adb_function = {
	.name		= "adb",
	.enable		= adb_android_function_enable,
	.disable	= adb_android_function_disable,
	.init		= adb_function_init,
	.cleanup	= adb_function_cleanup,
	.bind_config	= adb_function_bind_config,
};

static void adb_ready_callback(void)
{
	struct android_dev *dev = _android_dev;
	struct adb_data *data = adb_function.config;

	mutex_lock(&dev->mutex);

	data->opened = true;

	if (data->enabled)
		android_enable(dev);

	mutex_unlock(&dev->mutex);
}

static void adb_closed_callback(void)
{
	struct android_dev *dev = _android_dev;
	struct adb_data *data = adb_function.config;

	mutex_lock(&dev->mutex);

	data->opened = false;

	if (data->enabled)
		android_disable(dev);

	mutex_unlock(&dev->mutex);
}


#define MAX_ACM_INSTANCES 4
struct acm_function_config {
	int instances;
};

static int acm_function_init(struct android_usb_function *f, struct usb_composite_dev *cdev)
{
	int ret;
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	struct acm_function_config *config;
#endif
	f->config = kzalloc(sizeof(struct acm_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;

	ret = gserial_setup(cdev->gadget, MAX_ACM_INSTANCES);
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	/* default setting */
	config = f->config;
	config->instances = 1;
#endif
	return ret;
}

static void acm_function_cleanup(struct android_usb_function *f)
{
	gserial_cleanup();
	kfree(f->config);
	f->config = NULL;
}

static int acm_function_bind_config(struct android_usb_function *f, struct usb_configuration *c)
{
	int i;
	int ret = 0;
	struct acm_function_config *config = f->config;

	for (i = 0; i < config->instances; i++) {
		ret = acm_bind_config(c, i);
		if (ret) {
			pr_err("Could not bind acm%u config\n", i);
			break;
		}
	}

	return ret;
}

static ssize_t acm_instances_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct acm_function_config *config = f->config;
	return sprintf(buf, "%d\n", config->instances);
}

static ssize_t acm_instances_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct acm_function_config *config = f->config;
	int value;

	sscanf(buf, "%d", &value);
	if (value > MAX_ACM_INSTANCES)
		value = MAX_ACM_INSTANCES;
	config->instances = value;
	return size;
}

static DEVICE_ATTR(instances, S_IRUGO | S_IWUSR, acm_instances_show, acm_instances_store);
static struct device_attribute *acm_function_attributes[] = { &dev_attr_instances, NULL };

static struct android_usb_function acm_function = {
	.name		= "acm",
	.init		= acm_function_init,
	.cleanup	= acm_function_cleanup,
	.bind_config	= acm_function_bind_config,
	.attributes	= acm_function_attributes,
};


static int mtp_function_init(struct android_usb_function *f, struct usb_composite_dev *cdev)
{
	return mtp_setup();
}

static void mtp_function_cleanup(struct android_usb_function *f)
{
	mtp_cleanup();
}

static int mtp_function_bind_config(struct android_usb_function *f, struct usb_configuration *c)
{
	return mtp_bind_config(c, false);
}

static int ptp_function_init(struct android_usb_function *f, struct usb_composite_dev *cdev)
{
	/* nothing to do - initialization is handled by mtp_function_init */
	return 0;
}

static void ptp_function_cleanup(struct android_usb_function *f)
{
	/* nothing to do - cleanup is handled by mtp_function_cleanup */
}

static int ptp_function_bind_config(struct android_usb_function *f, struct usb_configuration *c)
{
	return mtp_bind_config(c, true);
}

static int mtp_function_ctrlrequest(struct android_usb_function *f,
						struct usb_composite_dev *cdev,
						const struct usb_ctrlrequest *c)
{
	return mtp_ctrlrequest(cdev, c);
}

static struct android_usb_function mtp_function = {
	.name		= "mtp",
	.init		= mtp_function_init,
	.cleanup	= mtp_function_cleanup,
	.bind_config	= mtp_function_bind_config,
	.ctrlrequest	= mtp_function_ctrlrequest,
};

/* PTP function is same as MTP with slightly different interface descriptor */
static struct android_usb_function ptp_function = {
	.name		= "ptp",
	.init		= ptp_function_init,
	.cleanup	= ptp_function_cleanup,
	.bind_config	= ptp_function_bind_config,
};


struct rndis_function_config {
	u8      ethaddr[ETH_ALEN];
	u32     vendorID;
	char	manufacturer[256];
	bool	wceis;
};

static int rndis_function_init(struct android_usb_function *f, struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct rndis_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;
	return 0;
}

static void rndis_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
	f->config = NULL;
}

static int rndis_function_bind_config(struct android_usb_function *f,
					struct usb_configuration *c)
{
	int ret;
	struct rndis_function_config *rndis = f->config;

	if (!rndis) {
		pr_err("%s: rndis_pdata\n", __func__);
		return -1;
	}

	ret = gether_setup_name(c->cdev->gadget, rndis->ethaddr, "rndis");
	if (ret) {
		pr_err("%s: gether_setup failed\n", __func__);
		return ret;
	}

	if (rndis->wceis) {
		/* "Wireless" RNDIS; auto-detected by Windows */
		rndis_iad_descriptor.bFunctionClass =
						USB_CLASS_WIRELESS_CONTROLLER;
		rndis_iad_descriptor.bFunctionSubClass = 0x01;
		rndis_iad_descriptor.bFunctionProtocol = 0x03;
		rndis_control_intf.bInterfaceClass =
						USB_CLASS_WIRELESS_CONTROLLER;
		rndis_control_intf.bInterfaceSubClass =	 0x01;
		rndis_control_intf.bInterfaceProtocol =	 0x03;
	}

	return rndis_bind_config(c, rndis->ethaddr, rndis->vendorID,
				    rndis->manufacturer);
}

static void rndis_function_unbind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	gether_cleanup();
}

static ssize_t rndis_manufacturer_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	return sprintf(buf, "%s\n", config->manufacturer);
}

static ssize_t rndis_manufacturer_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;

	if (size >= sizeof(config->manufacturer))
		return -EINVAL;
	if (sscanf(buf, "%s", config->manufacturer) == 1)
		return size;
	return -1;
}

static DEVICE_ATTR(manufacturer, S_IRUGO | S_IWUSR, rndis_manufacturer_show,
						    rndis_manufacturer_store);

static ssize_t rndis_wceis_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	return sprintf(buf, "%d\n", config->wceis);
}

static ssize_t rndis_wceis_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	int value;

	if (sscanf(buf, "%d", &value) == 1) {
		config->wceis = value;
		return size;
	}
	return -EINVAL;
}

static DEVICE_ATTR(wceis, S_IRUGO | S_IWUSR, rndis_wceis_show,
					     rndis_wceis_store);

static ssize_t rndis_ethaddr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *rndis = f->config;
	return sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		rndis->ethaddr[0], rndis->ethaddr[1], rndis->ethaddr[2],
		rndis->ethaddr[3], rndis->ethaddr[4], rndis->ethaddr[5]);
}

static ssize_t rndis_ethaddr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *rndis = f->config;

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	int i;
	char *src;
	for (i = 0; i < ETH_ALEN; i++)
		rndis->ethaddr[i] = 0;
	/* create a fake MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis->ethaddr[0] = 0x02;
	src = serial_string;
	for (i = 0; (i < 256) && *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis->ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}
	return size;
#else
	if (sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		    (int *)&rndis->ethaddr[0], (int *)&rndis->ethaddr[1],
		    (int *)&rndis->ethaddr[2], (int *)&rndis->ethaddr[3],
		    (int *)&rndis->ethaddr[4],
		    (int *)&rndis->ethaddr[5]) == 6)
		return size;
	return -EINVAL;
#endif
}

static DEVICE_ATTR(ethaddr, S_IRUGO | S_IWUSR, rndis_ethaddr_show,
					       rndis_ethaddr_store);

static ssize_t rndis_vendorID_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	return sprintf(buf, "%04x\n", config->vendorID);
}

static ssize_t rndis_vendorID_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	int value;

	if (sscanf(buf, "%04x", &value) == 1) {
		config->vendorID = value;
		return size;
	}
	return -EINVAL;
}

static DEVICE_ATTR(vendorID, S_IRUGO | S_IWUSR, rndis_vendorID_show,
						rndis_vendorID_store);

static struct device_attribute *rndis_function_attributes[] = {
	&dev_attr_manufacturer,
	&dev_attr_wceis,
	&dev_attr_ethaddr,
	&dev_attr_vendorID,
	NULL
};

static struct android_usb_function rndis_function = {
	.name		= "rndis",
	.init		= rndis_function_init,
	.cleanup	= rndis_function_cleanup,
	.bind_config	= rndis_function_bind_config,
	.unbind_config	= rndis_function_unbind_config,
	.attributes	= rndis_function_attributes,
};


struct mass_storage_function_config {
	struct fsg_config fsg;
	struct fsg_common *common;
};

static int mass_storage_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	struct mass_storage_function_config *config;
	struct fsg_common *common;
	int err, i;
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	int i;
	unsigned int cdfs = 0;
#endif
	config = kzalloc(sizeof(struct mass_storage_function_config),
								GFP_KERNEL);
	if (!config)
		return -ENOMEM;

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	if (android_usb_pdata && (android_usb_pdata->nluns != 0
		|| android_usb_pdata->cdfs_support != 0)) {
		/* Some device use sd card or not.
		 * If you want to modify nluns,
		 * please change nluns of standard android USB platform data
		 * Please do not modify nluns directly in this function.
		 * Every model uses same android file.
		 */
		printk(KERN_DEBUG "usb: %s pdata->nluns=%d, cdfs = %d\n",
					__func__, android_usb_pdata->nluns,
					android_usb_pdata->cdfs_support);
		cdfs = android_usb_pdata->cdfs_support;
		config->fsg.nluns = android_usb_pdata->nluns + cdfs;

		for (i = 0; i < android_usb_pdata->nluns; i++) {

			config->fsg.luns[i].removable = 1;
			config->fsg.luns[i].nofua = 1;
		}

		if (cdfs) {
			config->fsg.luns[i].cdrom = 1;
			config->fsg.luns[i].removable = 1;
			config->fsg.luns[i].nofua = 1;
		}

		common = fsg_common_init(NULL, cdev, &config->fsg);
		if (IS_ERR(common)) {
			kfree(config);
			return PTR_ERR(common);
		}

		for (i = 0; i < android_usb_pdata->nluns; i++) {
			char luns[5];
			err = snprintf(luns, 5, "lun%d", i);
			if (err == 0) {
				printk(KERN_ERR "usb: %s snprintf error\n",
						__func__);
				kfree(config);
				return err;
			}
			err = sysfs_create_link(&f->dev->kobj,
					&common->luns[i].dev.kobj,
					luns);
			if (err) {
				kfree(config);
				return err;
			}
		}
		if (cdfs) {
			char luns[4];
			err = snprintf(luns, 4, "lun");
			if (err == 0) {
				printk(KERN_ERR "usb: %s cdfs snprintf error\n",
						__func__);
				kfree(config);
				return err;
			}
			err = sysfs_create_link(&f->dev->kobj,
					&common->luns[i].dev.kobj,
					luns);
			if (err) {
				kfree(config);
				return err;
			}
		}
	} else {
#endif
		// default number of luns
#ifdef CONFIG_MACH_U1
		config->fsg.nluns = 2;
#else
		config->fsg.nluns = 1;
#endif
		for (i = 0; i < config->fsg.nluns; i++) {
			config->fsg.luns[i].removable = 1;
			config->fsg.luns[i].nofua = 1;
		}

		common = fsg_common_init(NULL, cdev, &config->fsg);
		if (IS_ERR(common)) {
			kfree(config);
			return PTR_ERR(common);
		}

		for (i = 0; i < config->fsg.nluns; i++) {
			char luns[5];
			err = snprintf(luns, 5, "lun%d", i);
			if (err == 0) {
				printk(KERN_ERR "usb: %s snprintf error\n", __func__);
				return err;
			}
			err = sysfs_create_link(&f->dev->kobj,
						&common->luns[i].dev.kobj,
						luns);
			if (err) {
				kfree(config);
				return err;
			}
		}
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	}
#endif
	config->common = common;
	f->config = config;
	return 0;
}

static void mass_storage_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
	f->config = NULL;
}

static int mass_storage_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	struct mass_storage_function_config *config = f->config;
	return fsg_bind_config(c->cdev, c, config->common);
}

static ssize_t mass_storage_inquiry_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	return sprintf(buf, "%s\n", config->common->inquiry_string);
}

static ssize_t mass_storage_inquiry_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	if (size >= sizeof(config->common->inquiry_string))
		return -EINVAL;
	if (sscanf(buf, "%s", config->common->inquiry_string) != 1)
		return -EINVAL;
	return size;
}

static DEVICE_ATTR(inquiry_string, S_IRUGO | S_IWUSR,
					mass_storage_inquiry_show,
					mass_storage_inquiry_store);

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
static ssize_t mass_storage_vendor_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	return sprintf(buf, "%s\n", config->common->vendor_string);
}

static ssize_t mass_storage_vendor_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;

	if (size >= sizeof(config->common->vendor_string))
		return -EINVAL;
	if (sscanf(buf, "%s", config->common->vendor_string) != 1)
		return -EINVAL;

	printk(KERN_DEBUG "%s: vendor %s", __func__,
				config->common->vendor_string);
	return size;
}

static DEVICE_ATTR(vendor_string, S_IRUGO | S_IWUSR,
					mass_storage_vendor_show,
					mass_storage_vendor_store);

static ssize_t mass_storage_product_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	return sprintf(buf, "%s\n", config->common->product_string);
}

static ssize_t mass_storage_product_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;

	if (size >= sizeof(config->common->product_string))
		return -EINVAL;
	if (sscanf(buf, "%s", config->common->product_string) != 1)
		return -EINVAL;

	printk(KERN_DEBUG "%s: product %s", __func__,
				config->common->product_string);
	return size;
}

static DEVICE_ATTR(product_string, S_IRUGO | S_IWUSR,
					mass_storage_product_show,
					mass_storage_product_store);
#endif

static struct device_attribute *mass_storage_function_attributes[] = {
	&dev_attr_inquiry_string,
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	&dev_attr_vendor_string,
	&dev_attr_product_string,
#endif
	NULL
};

static struct android_usb_function mass_storage_function = {
	.name		= "mass_storage",
	.init		= mass_storage_function_init,
	.cleanup	= mass_storage_function_cleanup,
	.bind_config	= mass_storage_function_bind_config,
	.attributes	= mass_storage_function_attributes,
};


static int accessory_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	return acc_setup();
}

static void accessory_function_cleanup(struct android_usb_function *f)
{
	acc_cleanup();
}

static int accessory_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	return acc_bind_config(c);
}

static int accessory_function_ctrlrequest(struct android_usb_function *f,
						struct usb_composite_dev *cdev,
						const struct usb_ctrlrequest *c)
{
	return acc_ctrlrequest(cdev, c);
}

static struct android_usb_function accessory_function = {
	.name		= "accessory",
	.init		= accessory_function_init,
	.cleanup	= accessory_function_cleanup,
	.bind_config	= accessory_function_bind_config,
	.ctrlrequest	= accessory_function_ctrlrequest,
};

/* DIAG : enabled DIAG clients- "diag[,diag_mdm]" */
static char diag_clients[32];
static ssize_t clients_store(
		struct device *device, struct device_attribute *attr,
		const char *buff, size_t size)
{
	strlcpy(diag_clients, buff, sizeof(diag_clients));

	return size;
}

static DEVICE_ATTR(clients, S_IWUSR, NULL, clients_store);
static struct device_attribute *diag_function_attributes[] = {
				&dev_attr_clients, NULL };

static int diag_function_init(struct android_usb_function *f,
				 struct usb_composite_dev *cdev)
{
	return diag_setup();
}

static void diag_function_cleanup(struct android_usb_function *f)
{
	diag_cleanup();
}

static int diag_function_bind_config(struct android_usb_function *f,
					struct usb_configuration *c)
{
	char *name;
	char buf[32], *b;
	int once = 0, err = -1;
	int (*notify)(uint32_t, const char *) = NULL;

	strlcpy(buf, diag_clients, sizeof(buf));
	b = strim(buf);
	while (b) {
		notify = NULL;
		name = strsep(&b, ",");

		if (name) {
			err = diag_function_add(c, name, notify);
			if (err)
				pr_err("%s : usb: diag: Cannot open channel '%s\r\n",
						 __func__, name);
		}
	}
	return err;
}

static struct android_usb_function diag_function = {
	.name		= "diag",
	.init		= diag_function_init,
	.cleanup	= diag_function_cleanup,
	.bind_config	= diag_function_bind_config,
	.attributes	= diag_function_attributes,
};

static int dm_function_bind_config(struct android_usb_function *f,
					struct usb_configuration *c)
{
	return dm_bind_config(c, DM_PORT_NUM);
}

static struct android_usb_function dm_function = {
	.name           = "dm",
	.bind_config    = dm_function_bind_config,
};

static int audio_source_function_init(struct android_usb_function *f,
			struct usb_composite_dev *cdev)
{
	struct audio_source_config *config;

	config = kzalloc(sizeof(struct audio_source_config), GFP_KERNEL);
	if (!config)
		return -ENOMEM;
	config->card = -1;
	config->device = -1;
	f->config = config;
	return 0;
}

static void audio_source_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
}

static int audio_source_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	struct audio_source_config *config = f->config;

	return audio_source_bind_config(c, config);
}

static void audio_source_function_unbind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	struct audio_source_config *config = f->config;

	config->card = -1;
	config->device = -1;
}

static ssize_t audio_source_pcm_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct audio_source_config *config = f->config;

	/* print PCM card and device numbers */
	return sprintf(buf, "%d %d\n", config->card, config->device);
}

static DEVICE_ATTR(pcm, S_IRUGO | S_IWUSR, audio_source_pcm_show, NULL);

static struct device_attribute *audio_source_function_attributes[] = {
	&dev_attr_pcm,
	NULL
};

static struct android_usb_function audio_source_function = {
	.name		= "audio_source",
	.init		= audio_source_function_init,
	.cleanup	= audio_source_function_cleanup,
	.bind_config	= audio_source_function_bind_config,
	.unbind_config	= audio_source_function_unbind_config,
	.attributes	= audio_source_function_attributes,
};

static struct android_usb_function *supported_functions[] = {
	&adb_function,
	&acm_function,
	&mtp_function,
	&ptp_function,
	&rndis_function,
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	&ncm_function,
#endif
	&mass_storage_function,
	&accessory_function,
	&diag_function,
	&dm_function,
	&audio_source_function,
	NULL
};


static int android_init_functions(struct android_usb_function **functions,
				  struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct android_usb_function *f;
	struct device_attribute **attrs;
	struct device_attribute *attr;
	int err = 0;
	int index = 0;

	for (; (f = *functions++); index++) {
		f->dev_name = kasprintf(GFP_KERNEL, "f_%s", f->name);
		f->dev = device_create(android_class, dev->dev,
				MKDEV(0, index), f, f->dev_name);
		if (IS_ERR(f->dev)) {
			pr_err("%s: Failed to create dev %s", __func__,
							f->dev_name);
			err = PTR_ERR(f->dev);
			goto err_create;
		}

		if (f->init) {
			err = f->init(f, cdev);
			if (err) {
				pr_err("%s: Failed to init %s", __func__,
								f->name);
				goto err_out;
			}
		}

		attrs = f->attributes;
		if (attrs) {
			while ((attr = *attrs++) && !err)
				err = device_create_file(f->dev, attr);
		}
		if (err) {
			pr_err("%s: Failed to create function %s attributes",
					__func__, f->name);
			goto err_out;
		}
	}
	return 0;

err_out:
	device_destroy(android_class, f->dev->devt);
err_create:
	kfree(f->dev_name);
	return err;
}

static void android_cleanup_functions(struct android_usb_function **functions)
{
	struct android_usb_function *f;

	while (*functions) {
		f = *functions++;

		if (f->dev) {
			device_destroy(android_class, f->dev->devt);
			kfree(f->dev_name);
		}

		if (f->cleanup)
			f->cleanup(f);
	}
}

static int
android_bind_enabled_functions(struct android_dev *dev,
			       struct usb_configuration *c)
{
	struct android_usb_function *f;
	int ret;

	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		printk(KERN_DEBUG "usb: %s f:%s\n", __func__, f->name);
		ret = f->bind_config(f, c);
		if (ret) {
			pr_err("%s: %s failed", __func__, f->name);
			return ret;
		}
	}
	return 0;
}

static void
android_unbind_enabled_functions(struct android_dev *dev,
			       struct usb_configuration *c)
{
	struct android_usb_function *f;

	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		if (f->unbind_config)
			f->unbind_config(f, c);
	}
}

static int android_enable_function(struct android_dev *dev, char *name)
{
	struct android_usb_function **functions = dev->functions;
	struct android_usb_function *f;
	printk(KERN_DEBUG "usb: %s name=%s\n", __func__, name);
	while ((f = *functions++)) {
		if (!strcmp(name, f->name)) {
			list_add_tail(&f->enabled_list, &dev->enabled_functions);
			return 0;
		}
	}
	return -EINVAL;
}

/*-------------------------------------------------------------------------*/
/* /sys/class/android_usb/android%d/ interface */

static ssize_t
functions_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct android_usb_function *f;
	char *buff = buf;

	mutex_lock(&dev->mutex);

	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		printk(KERN_DEBUG "usb: %s enabled_func=%s\n",
				__func__, f->name);
		buff += sprintf(buff, "%s,", f->name);
	}

	mutex_unlock(&dev->mutex);

	if (buff != buf)
		*(buff-1) = '\n';

	return buff - buf;
}

static ssize_t
functions_store(struct device *pdev, struct device_attribute *attr,
			       const char *buff, size_t size)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	char *name;
	char buf[256], *b;
	int err;

	mutex_lock(&dev->mutex);

	if (dev->enabled) {
		mutex_unlock(&dev->mutex);
		return -EBUSY;
	}

	INIT_LIST_HEAD(&dev->enabled_functions);

	printk(KERN_DEBUG "usb: %s buff=%s\n", __func__, buff);
	strncpy(buf, buff, sizeof(buf));
	b = strim(buf);
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	strncpy(function_lists_string, b, sizeof(buf));
#endif

	while (b) {
		name = strsep(&b, ",");
		if (name) {
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
			/* Enable ncm function */
			if (is_ncm_ready(name)) {
				printk(KERN_DEBUG "usb: %s ncm on\n",
						__func__);
				err = android_enable_function(dev,
						"ncm");
				continue;
			}
#endif
			err = android_enable_function(dev, name);
			if (err)
				pr_err("android_usb: Cannot enable '%s'", name);
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
			/* Enable ACM function, if MTP is enabled. */
			if (!strcmp(name, "mtp")) {
				err = android_enable_function(dev, "acm");
				if (err)
					pr_err(
					"android_usb: Cannot enable '%s'",
					name);
			}
#endif
		}
	}

	mutex_unlock(&dev->mutex);

	return size;
}

static ssize_t enable_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	printk(KERN_DEBUG "usb: %s dev->enabled=%d\n", __func__,  dev->enabled);
	return sprintf(buf, "%d\n", dev->enabled);
}

static ssize_t enable_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct usb_composite_dev *cdev = dev->cdev;
	struct android_usb_function *f;
	int enabled = 0;

	mutex_lock(&dev->mutex);

	sscanf(buff, "%d", &enabled);
	printk(KERN_DEBUG "usb: %s enabled=%d, !dev->enabled=%d\n",
			__func__, enabled, !dev->enabled);
	if (enabled && !dev->enabled) {
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
		struct android_usb_function *f;
		cdev->next_string_id = composite_string_index;
#else
		cdev->next_string_id = 0;
#endif
		/* update values in composite driver's copy of device descriptor */
		cdev->desc.idVendor = device_desc.idVendor;
		cdev->desc.idProduct = device_desc.idProduct;
		cdev->desc.bcdDevice = device_desc.bcdDevice;
		cdev->desc.bDeviceClass = device_desc.bDeviceClass;
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
		list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
			printk(KERN_DEBUG "usb: %s f:%s\n", __func__, f->name);
			if (!strcmp(f->name, "acm")) {
				printk(KERN_DEBUG "usb: acm is enabled. (bcdDevice=0x400)\n");
				/* Samsung KIES needs fixed bcdDevice number */
				cdev->desc.bcdDevice = cpu_to_le16(0x0400);
			}
			if (is_ncm_ready(f->name))
				set_ncm_device_descriptor(&cdev->desc);
		}
		strncpy(manufacturer_string, "SAMSUNG",
				sizeof(manufacturer_string) - 1);
		strncpy(product_string, "SAMSUNG_Android",
				sizeof(product_string) - 1);
#endif
		cdev->desc.bDeviceSubClass = device_desc.bDeviceSubClass;
		cdev->desc.bDeviceProtocol = device_desc.bDeviceProtocol;
		printk(KERN_DEBUG "usb: %s vendor=%x,product=%x,bcdDevice=%x",
				__func__, cdev->desc.idVendor,
				cdev->desc.idProduct, cdev->desc.bcdDevice);
		printk(KERN_DEBUG ",Class=%x,SubClass=%x,Protocol=%x\n",
				cdev->desc.bDeviceClass,
				cdev->desc.bDeviceSubClass,
				cdev->desc.bDeviceProtocol);
		printk(KERN_DEBUG "usb: %s next cmd : usb_add_config\n",
				__func__);
		list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
			if (f->enable)
				f->enable(f);
		}
		android_enable(dev);
		dev->enabled = true;
	} else if (!enabled && dev->enabled) {
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
		/* avoid sending a disconnect switch event
		 * until after we disconnect.
		 */
		cdev->mute_switch = true;
#endif
		android_disable(dev);
		list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
			if (f->disable)
				f->disable(f);
		}
		dev->enabled = false;
		mdelay(5);
	} else if (!enabled) {
		usb_gadget_disconnect(cdev->gadget);
		dev->enabled = false;
	} else {
		pr_err("android_usb: already %s\n",
				dev->enabled ? "enabled" : "disabled");
	}

	mutex_unlock(&dev->mutex);
	return size;
}

static ssize_t state_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct usb_composite_dev *cdev = dev->cdev;
	char *state = "DISCONNECTED";
	unsigned long flags;

	if (!cdev)
		goto out;

	spin_lock_irqsave(&cdev->lock, flags);
        if (cdev->config)
		state = "CONFIGURED";
	else if (dev->connected)
		state = "CONNECTED";
	spin_unlock_irqrestore(&cdev->lock, flags);
out:
	printk(KERN_DEBUG "usb: %s buf=%s\n", __func__, state);
	return sprintf(buf, "%s\n", state);
}

#define DESCRIPTOR_ATTR(field, format_string)				\
static ssize_t								\
field ## _show(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	return sprintf(buf, format_string, device_desc.field);		\
}									\
static ssize_t								\
field ## _store(struct device *dev, struct device_attribute *attr,	\
		const char *buf, size_t size)				\
{									\
	int value;							\
	if (sscanf(buf, format_string, &value) == 1) {			\
		device_desc.field = value;				\
		return size;						\
	}								\
	return -1;							\
}									\
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);

#define DESCRIPTOR_STRING_ATTR(field, buffer)				\
static ssize_t								\
field ## _show(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	return sprintf(buf, "%s", buffer);				\
}									\
static ssize_t								\
field ## _store(struct device *dev, struct device_attribute *attr,	\
		const char *buf, size_t size)				\
{									\
	if (size >= sizeof(buffer)) return -EINVAL;			\
	return strlcpy(buffer, buf, sizeof(buffer));			\
}									\
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);


DESCRIPTOR_ATTR(idVendor, "%04x\n")
DESCRIPTOR_ATTR(idProduct, "%04x\n")
DESCRIPTOR_ATTR(bcdDevice, "%04x\n")
DESCRIPTOR_ATTR(bDeviceClass, "%d\n")
DESCRIPTOR_ATTR(bDeviceSubClass, "%d\n")
DESCRIPTOR_ATTR(bDeviceProtocol, "%d\n")
DESCRIPTOR_STRING_ATTR(iManufacturer, manufacturer_string)
DESCRIPTOR_STRING_ATTR(iProduct, product_string)
DESCRIPTOR_STRING_ATTR(iSerial, serial_string)

static DEVICE_ATTR(functions, S_IRUGO | S_IWUSR, functions_show, functions_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, enable_show, enable_store);
static DEVICE_ATTR(state, S_IRUGO, state_show, NULL);

static struct device_attribute *android_usb_attributes[] = {
	&dev_attr_idVendor,
	&dev_attr_idProduct,
	&dev_attr_bcdDevice,
	&dev_attr_bDeviceClass,
	&dev_attr_bDeviceSubClass,
	&dev_attr_bDeviceProtocol,
	&dev_attr_iManufacturer,
	&dev_attr_iProduct,
	&dev_attr_iSerial,
	&dev_attr_functions,
	&dev_attr_enable,
	&dev_attr_state,
	NULL
};

/*-------------------------------------------------------------------------*/
/* Composite driver */

static int android_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;
	int ret = 0;

	ret = android_bind_enabled_functions(dev, c);
	if (ret)
		return ret;

	return 0;
}

static void android_unbind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;

	android_unbind_enabled_functions(dev, c);
}

static int android_bind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct usb_gadget	*gadget = cdev->gadget;
	int			gcnum, id, ret;

	printk(KERN_DEBUG "usb: %s disconnect\n", __func__);
	usb_gadget_disconnect(gadget);

	ret = android_init_functions(dev->functions, cdev);
	if (ret)
		return ret;

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_MANUFACTURER_IDX].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_PRODUCT_IDX].id = id;
	device_desc.iProduct = id;

	/* Default strings - should be updated by userspace */
	strncpy(manufacturer_string, "Android", sizeof(manufacturer_string) - 1);
	strncpy(product_string, "Android", sizeof(product_string) - 1);

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	sprintf(serial_string,
			"%08X%08X", system_serial_high, system_serial_low);
#else
	strncpy(serial_string, "0123456789ABCDEF", sizeof(serial_string) - 1);
#endif

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_SERIAL_IDX].id = id;
	device_desc.iSerialNumber = id;

	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum);
	else {
		/* gadget zero is so simple (for now, no altsettings) that
		 * it SHOULD NOT have problems with bulk-capable hardware.
		 * so just warn about unrcognized controllers -- don't panic.
		 *
		 * things like configuration and altsetting numbering
		 * can need hardware-specific attention though.
		 */
		pr_warning("%s: controller '%s' not recognized\n",
			longname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	composite_string_index = 4;
#endif
	usb_gadget_set_selfpowered(gadget);
	dev->cdev = cdev;

	return 0;
}

static int android_usb_unbind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	printk(KERN_DEBUG "usb: %s\n", __func__);
	cancel_work_sync(&dev->work);
	android_cleanup_functions(dev->functions);
	return 0;
}

static struct usb_composite_driver android_usb_driver = {
	.name		= "android_usb",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.unbind		= android_usb_unbind,
};

static int
android_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *c)
{
	struct android_dev		*dev = _android_dev;
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);
	struct usb_request		*req = cdev->req;
	struct android_usb_function	*f;
	int value = -EOPNOTSUPP;
	unsigned long flags;

	req->zero = 0;
	req->complete = composite_setup_complete;
	req->length = 0;
	gadget->ep0->driver_data = cdev;

	list_for_each_entry(f, &dev->enabled_functions, enabled_list) {
		if (f->ctrlrequest) {
			value = f->ctrlrequest(f, cdev, c);
			if (value >= 0)
				break;
		}
	}

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	if (value < 0)
		value = terminal_ctrl_request(cdev, c);
#endif

	/* Special case the accessory function.
	 * It needs to handle control requests before it is enabled.
	 */
	if (value < 0)
		value = acc_ctrlrequest(cdev, c);

	if (value < 0)
		value = composite_setup(gadget, c);

	spin_lock_irqsave(&cdev->lock, flags);
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	if (c->bRequest == USB_REQ_SET_CONFIGURATION &&
			cdev->mute_switch == true)
		cdev->mute_switch = false;
#endif
	if (!dev->connected) {
		dev->connected = 1;
		schedule_work(&dev->work);
	}
	else if (c->bRequest == USB_REQ_SET_CONFIGURATION && cdev->config) {
		schedule_work(&dev->work);
	}
	spin_unlock_irqrestore(&cdev->lock, flags);

	return value;
}

static void android_disconnect(struct usb_gadget *gadget)
{
	struct android_dev *dev = _android_dev;
	struct usb_composite_dev *cdev = get_gadget_data(gadget);
	unsigned long flags;

	composite_disconnect(gadget);
	/* accessory HID support can be active while the
	   accessory function is not actually enabled,
	   so we need to inform it when we are disconnected.
	 */
	acc_disconnect();

	spin_lock_irqsave(&cdev->lock, flags);
	dev->connected = 0;
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	printk(KERN_DEBUG "usb: %s con(%d), sw(%d)\n",
		 __func__, dev->connected, dev->sw_connected);
	/* avoid sending a disconnect switch event
	 * until after we disconnect.
	 */
	if (cdev->mute_switch) {
		dev->sw_connected = dev->connected;
		printk(KERN_DEBUG "usb: %s mute_switch con(%d) sw(%d)\n",
			 __func__, dev->connected, dev->sw_connected);
	} else {
		set_ncm_ready(false);
		if (cdev->force_disconnect) {
			dev->sw_connected = 1;
			printk(KERN_DEBUG "usb: %s force_disconnect\n",
				 __func__);
			cdev->force_disconnect = 0;
		}
		printk(KERN_DEBUG "usb: %s schedule_work con(%d) sw(%d)\n",
			 __func__, dev->connected, dev->sw_connected);
		schedule_work(&dev->work);
	}
#else
	schedule_work(&dev->work);
#endif
	spin_unlock_irqrestore(&cdev->lock, flags);
}

static int android_create_device(struct android_dev *dev)
{
	struct device_attribute **attrs = android_usb_attributes;
	struct device_attribute *attr;
	int err;

	dev->dev = device_create(android_class, NULL,
					MKDEV(0, 0), NULL, "android0");
	if (IS_ERR(dev->dev))
		return PTR_ERR(dev->dev);

	dev_set_drvdata(dev->dev, dev);

	while ((attr = *attrs++)) {
		err = device_create_file(dev->dev, attr);
		if (err) {
			device_destroy(android_class, dev->dev->devt);
			return err;
		}
	}
	return 0;
}

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
static int android_probe(struct platform_device *pdev)
{
	android_usb_pdata = pdev->dev.platform_data;

	printk(KERN_INFO "usb: %s pdata: %p\n", __func__, android_usb_pdata);

	if (android_usb_pdata) {
		printk(KERN_INFO "usb: %s nluns=%d\n", __func__,
				android_usb_pdata->nluns);
	} else {
		printk(KERN_INFO "usb: %s pdata is not available\n", __func__);
	}
	return 0;
}

static struct platform_driver android_platform_driver = {
	.driver = { .name = "android_usb", },
	.probe = android_probe,
};
#endif

static int __init init(void)
{
	struct android_dev *dev;
	int err;

	printk(KERN_DEBUG "%s\n", __func__);
	android_class = class_create(THIS_MODULE, "android_usb");
	if (IS_ERR(android_class))
		return PTR_ERR(android_class);

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->disable_depth = 1;
	dev->functions = supported_functions;
	INIT_LIST_HEAD(&dev->enabled_functions);
	INIT_WORK(&dev->work, android_work);
	mutex_init(&dev->mutex);

	err = android_create_device(dev);
	if (err) {
		class_destroy(android_class);
		kfree(dev);
		return err;
	}

	_android_dev = dev;

	/* Override composite driver functions */
	composite_driver.setup = android_setup;
	composite_driver.disconnect = android_disconnect;
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	/* Create below sysfs
	 *   /sys/class/android_usb/android0/terminal_version
	 */
	err = create_terminal_attribute(&dev->dev);
	if (err) {
		printk(KERN_ERR "usb: %s To create terminal_atttrr is failed\n",
				__func__);
		return err;
	}
	err = platform_driver_register(&android_platform_driver);
	if (err) {
		printk(KERN_ERR "usb: %s platform register is failed\n",
				__func__);
		return err;
	}
#endif

#ifdef CONFIG_USB_DUN_SUPPORT
	err = modem_misc_register();
	if (err) {
		printk(KERN_ERR "usb: %s modem misc register is failed\n",
				__func__);
		return err;
	}
#endif

	return usb_composite_probe(&android_usb_driver, android_bind);
}
module_init(init);

static void __exit cleanup(void)
{
	usb_composite_unregister(&android_usb_driver);
	class_destroy(android_class);
	kfree(_android_dev);
	_android_dev = NULL;
}
module_exit(cleanup);
