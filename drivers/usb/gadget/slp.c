/*
 * Gadget Driver for SLP based on Android
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
#include <linux/mutex.h>

#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/usb/android_composite.h>
#include <linux/usb/slp_multi.h>

#include "gadget_chips.h"

/*
 * When the feature "CONFIG_USB_G_SLP" is selected, we cannot choose
 * "CONFIG_USB_DUN_SUPPORT" feature. Because that feature is tightly
 * linked to "CONFIG_USB_G_ANDROID" feature in this project.
 * But we have to use that feature, so we just defined it in this code.
 * That feature is only for SAMSUNG, and it works only in "f_acm.c"
 */
#define CONFIG_USB_DUN_SUPPORT	/* for data-router */

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

#include "f_mass_storage.c"
#include "u_serial.c"
#ifdef CONFIG_USB_DUN_SUPPORT
#include "serial_acm_slp.c"
#endif
#include "f_sdb.c"
#include "f_acm.c"
#include "f_mtp_slp.c"
#include "f_accessory.c"
#include "f_diag.c"
#define USB_ETH_RNDIS y
#include "f_rndis.c"
#include "rndis.c"
#include "u_ether.c"

MODULE_AUTHOR("Mike Lockwood");
MODULE_DESCRIPTION("SLP Composite USB Driver based on Android");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

#define USB_MODE_VERSION	"1.0"

static const char slp_longname[] = "Gadget SLP";

/* Default vendor and product IDs, overridden by userspace */
#define VENDOR_ID		0x04E8	/* Samsung VID */
#define PRODUCT_ID		0x6860	/* KIES mode PID */

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

	/* for android_dev.available_functions */
	struct list_head available_list;

	/* Manndatory: initialization during gadget bind */
	int (*init) (struct android_usb_function *, struct usb_composite_dev *);
	/* Optional: cleanup during gadget unbind */
	void (*cleanup) (struct android_usb_function *);
	/* Mandatory: called when the usb enabled */
	int (*bind_config) (struct android_usb_function *,
			    struct usb_configuration *);
	/* Optional: called when the configuration is removed */
	void (*unbind_config) (struct android_usb_function *,
			       struct usb_configuration *);
	/* Optional: handle ctrl requests before the device is configured */
	int (*ctrlrequest) (struct android_usb_function *,
			    struct usb_composite_dev *,
			    const struct usb_ctrlrequest *);

	enum slp_multi_config_id usb_config_id;
};

struct android_dev {
	struct list_head available_functions;
	struct list_head enabled_functions;
	struct usb_composite_dev *cdev;
	struct device *dev;

	bool enabled;
	bool dual_config;
};

static unsigned slp_multi_nluns;
static struct class *android_class;
static struct android_dev *_android_dev;
static int android_bind_config(struct usb_configuration *c);
static void android_unbind_config(struct usb_configuration *c);

static DEFINE_MUTEX(enable_lock);

/* string IDs are assigned dynamically */
#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2

static char manufacturer_string[256];
static char product_string[256];
static char serial_string[256];

/* String Table */
static struct usb_string strings_dev[] = {
	[STRING_MANUFACTURER_IDX].s = manufacturer_string,
	[STRING_PRODUCT_IDX].s = product_string,
	[STRING_SERIAL_IDX].s = serial_string,
	{}			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language = 0x0409,	/* en-us */
	.strings = strings_dev,
};

static struct usb_gadget_strings *slp_dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
	.bLength = sizeof(device_desc),
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = __constant_cpu_to_le16(0x0200),
	.idVendor = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice = __constant_cpu_to_le16(0xffff),
};

static struct usb_configuration first_config_driver = {
	.label = "slp_first_config",
	.unbind = android_unbind_config,
	.bConfigurationValue = USB_CONFIGURATION_1,
	.bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower = 0x30,	/* 96ma */
};

static struct usb_configuration second_config_driver = {
	.label = "slp_second_config",
	.unbind = android_unbind_config,
	.bConfigurationValue = USB_CONFIGURATION_2,
	.bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower = 0x30,	/* 96ma */
};

/*-------------------------------------------------------------------------*/
/* Supported functions initialization */

static int sdb_function_init(struct android_usb_function *f,
			     struct usb_composite_dev *cdev)
{
	return sdb_setup(cdev);
}

static void sdb_function_cleanup(struct android_usb_function *f)
{
	sdb_cleanup();
}

static int sdb_function_bind_config(struct android_usb_function *f,
				    struct usb_configuration *c)
{
	return sdb_bind_config(c);
}

static struct android_usb_function sdb_function = {
	.name = "sdb",
	.init = sdb_function_init,
	.cleanup = sdb_function_cleanup,
	.bind_config = sdb_function_bind_config,
};

#define MAX_ACM_INSTANCES 4
struct acm_function_config {
	int instances;
};

static int acm_function_init(struct android_usb_function *f,
			     struct usb_composite_dev *cdev)
{
	struct acm_function_config *config;
	int status;

	config = kzalloc(sizeof(struct acm_function_config), GFP_KERNEL);
	if (!config)
		return -ENOMEM;

	/* default setting */
	config->instances = 1;

#ifdef CONFIG_USB_DUN_SUPPORT
	status = modem_misc_register();
	if (status) {
		kfree(config);
		return status;
	}
#endif

	/* maybe allocate device-global string IDs, and patch descriptors */
	if (acm_string_defs[ACM_CTRL_IDX].id == 0) {
		status = usb_string_id(cdev);
		if (status < 0)
			goto acm_init_error;
		acm_string_defs[ACM_CTRL_IDX].id = status;

		acm_control_interface_desc.iInterface = status;

		status = usb_string_id(cdev);
		if (status < 0)
			goto acm_init_error;
		acm_string_defs[ACM_DATA_IDX].id = status;

		acm_data_interface_desc.iInterface = status;

		status = usb_string_id(cdev);
		if (status < 0)
			goto acm_init_error;
		acm_string_defs[ACM_IAD_IDX].id = status;

		acm_iad_descriptor.iFunction = status;
	}

	status = gserial_setup(cdev->gadget, config->instances);
	if (status)
		goto acm_init_error;

	f->config = config;
	return 0;

 acm_init_error:
	kfree(config);
	return status;
}

static void acm_function_cleanup(struct android_usb_function *f)
{
	gserial_cleanup();
	kfree(f->config);
	f->config = NULL;
#ifdef CONFIG_USB_DUN_SUPPORT
	modem_misc_unregister();
#endif
}

static int acm_function_bind_config(struct android_usb_function *f,
				    struct usb_configuration *c)
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
				   struct device_attribute *attr,
				   const char *buf, size_t size)
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

static DEVICE_ATTR(instances, S_IRUGO | S_IWUSR,
		   acm_instances_show, acm_instances_store);
static struct device_attribute *acm_function_attributes[] = {
				&dev_attr_instances, NULL };

static struct android_usb_function acm_function = {
	.name = "acm",
	.init = acm_function_init,
	.cleanup = acm_function_cleanup,
	.bind_config = acm_function_bind_config,
	.attributes = acm_function_attributes,
};

static int mtp_function_init(struct android_usb_function *f,
			     struct usb_composite_dev *cdev)
{
	return mtp_setup(cdev);
}

static void mtp_function_cleanup(struct android_usb_function *f)
{
	mtp_cleanup();
}

static int mtp_function_bind_config(struct android_usb_function *f,
				    struct usb_configuration *c)
{
	return mtp_bind_config(c);
}

static int mtp_function_ctrlrequest(struct android_usb_function *f,
				    struct usb_composite_dev *cdev,
				    const struct usb_ctrlrequest *ctrl)
{
	struct usb_request *req = cdev->req;
	struct usb_gadget *gadget = cdev->gadget;
	int value = -EOPNOTSUPP;
	u16 w_length = le16_to_cpu(ctrl->wLength);
	struct usb_string_descriptor *os_func_desc = req->buf;
	char ms_descriptor[38] = {
		/* Header section */
		/* Upper 2byte of dwLength */
		0x00, 0x00,
		/* bcd Version */
		0x00, 0x01,
		/* wIndex, Extended compatID index */
		0x04, 0x00,
		/* bCount, we use only 1 function(MTP) */
		0x01,
		/* RESERVED */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

		/* First Function section for MTP */
		/* bFirstInterfaceNumber,
		 * we always use it by 0 for MTP
		 */
		0x00,
		/* RESERVED, fixed value 1 */
		0x01,
		/* CompatibleID for MTP */
		0x4D, 0x54, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00,
		/* Sub-compatibleID for MTP */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		/* RESERVED */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	switch (ctrl->bRequest) {
		/* Added handler to respond to host about MS OS Descriptors.
		 * Below handler is requirement if you use MTP.
		 * So, If you set composite included MTP,
		 * you have to respond to host about 0x54 or 0x64 request
		 * refer to following site.
		 * http://msdn.microsoft.com/en-us/windows/hardware/gg463179
		 */
	case 0x54:
	case 0x6F:
		os_func_desc->bLength = 0x28;
		os_func_desc->bDescriptorType = 0x00;
		value = min(w_length, (u16) (sizeof(ms_descriptor) + 2));
		memcpy(os_func_desc->wData, &ms_descriptor, value);

		if (value >= 0) {
			req->length = value;
			req->zero = value < w_length;
			value = usb_ep_queue(gadget->ep0, req, GFP_ATOMIC);
			if (value < 0) {
				req->status = 0;
				composite_setup_complete(gadget->ep0, req);
			}
		}
		break;
	}

	return value;
}

static struct android_usb_function mtp_function = {
	.name = "mtp",
	.init = mtp_function_init,
	.cleanup = mtp_function_cleanup,
	.bind_config = mtp_function_bind_config,
	.ctrlrequest = mtp_function_ctrlrequest,
};

struct rndis_function_config {
	u8 ethaddr[ETH_ALEN];
	u32 vendorID;
	char manufacturer[256];
	bool wceis;
	u8 rndis_string_defs0_id;
};
static char host_addr_string[18];

static int rndis_function_init(struct android_usb_function *f,
			       struct usb_composite_dev *cdev)
{
	struct rndis_function_config *config;
	int status, i;

	config = kzalloc(sizeof(struct rndis_function_config), GFP_KERNEL);
	if (!config)
		return -ENOMEM;

	/* maybe allocate device-global string IDs */
	if (rndis_string_defs[0].id == 0) {

		/* control interface label */
		status = usb_string_id(cdev);
		if (status < 0)
			goto rndis_init_error;
		config->rndis_string_defs0_id = status;
		rndis_string_defs[0].id = status;
		rndis_control_intf.iInterface = status;

		/* data interface label */
		status = usb_string_id(cdev);
		if (status < 0)
			goto rndis_init_error;
		rndis_string_defs[1].id = status;
		rndis_data_intf.iInterface = status;

		/* IAD iFunction label */
		status = usb_string_id(cdev);
		if (status < 0)
			goto rndis_init_error;
		rndis_string_defs[2].id = status;
		rndis_iad_descriptor.iFunction = status;
	}

	/* create a fake MAC address from our serial number. */
	for (i = 0; (i < 256) && serial_string[i]; i++) {
		/* XOR the USB serial across the remaining bytes */
		config->ethaddr[i % (ETH_ALEN - 1) + 1] ^= serial_string[i];
	}
	config->ethaddr[0] &= 0xfe;	/* clear multicast bit */
	config->ethaddr[0] |= 0x02;	/* set local assignment bit (IEEE802) */

	snprintf(host_addr_string, sizeof(host_addr_string),
		"%02x:%02x:%02x:%02x:%02x:%02x",
		config->ethaddr[0],	config->ethaddr[1],
		config->ethaddr[2],	config->ethaddr[3],
		config->ethaddr[4], config->ethaddr[5]);

	/* The "host_addr" is a module_param variable of u_ether.c
	 *  for "host ethernet address". So we use it directly.
	 */
	host_addr = host_addr_string;

	f->config = config;
	return 0;

 rndis_init_error:
	kfree(config);
	return status;
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

	ret = gether_setup(c->cdev->gadget, rndis->ethaddr);
	if (ret) {
		pr_err("%s: gether_setup failed(ret:%d)\n", __func__, ret);
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
		rndis_control_intf.bInterfaceSubClass = 0x01;
		rndis_control_intf.bInterfaceProtocol = 0x03;
	}

	/* ... and setup RNDIS itself */
	ret = rndis_init();
	if (ret < 0) {
		pr_err("%s: rndis_init failed(ret:%d)\n", __func__, ret);
		gether_cleanup();
		return ret;
	}

	/* Android team reset "rndis_string_defs[0].id" when RNDIS unbinded
	 * in f_rndis.c but, that makes failure of rndis_bind_config() by
	 * the overflow of "next_string_id" value in usb_string_id().
	 * So, Android team also reset "next_string_id" value in android.c
	 * but SLP does not reset "next_string_id" value. And we decided to
	 * re-update "rndis_string_defs[0].id" by old value.
	 * 20120224 yongsul96.oh@samsung.com
	 */
	if (rndis_string_defs[0].id == 0)
		rndis_string_defs[0].id = rndis->rndis_string_defs0_id;

	ret = rndis_bind_config(c, rndis->ethaddr, rndis->vendorID,
				 rndis->manufacturer);
	if (ret) {
		rndis_exit();
		gether_cleanup();
		pr_err("%s: rndis_bind_config failed(ret:%d)\n", __func__, ret);
	}

	return ret;
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
					struct device_attribute *attr,
					const char *buf, size_t size)
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
				 struct device_attribute *attr, const char *buf,
				 size_t size)
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

static DEVICE_ATTR(ethaddr, S_IRUGO | S_IWUSR, rndis_ethaddr_show,
		   NULL);

static ssize_t rndis_vendorID_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	return sprintf(buf, "%04x\n", config->vendorID);
}

static ssize_t rndis_vendorID_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
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
	.name = "rndis",
	.init = rndis_function_init,
	.cleanup = rndis_function_cleanup,
	.bind_config = rndis_function_bind_config,
	.unbind_config = rndis_function_unbind_config,
	.attributes = rndis_function_attributes,
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

	config = kzalloc(sizeof(struct mass_storage_function_config),
			 GFP_KERNEL);
	if (!config)
		return -ENOMEM;

	if (slp_multi_nluns != 0) {
		/* Some device use sd card or not.
		 * If you want to modify nluns,
		 * please change nluns of standard android USB platform data
		 * Please do not modify nluns directly in this function.
		 * Every model uses same android file.
		 */
		printk(KERN_DEBUG "usb: %s nluns=%d\n", __func__,
		       slp_multi_nluns);
		config->fsg.nluns = slp_multi_nluns;
		for (i = 0; i < slp_multi_nluns; i++)
			config->fsg.luns[i].removable = 1;

		common = fsg_common_init(NULL, cdev, &config->fsg);
		if (IS_ERR(common)) {
			kfree(config);
			return PTR_ERR(common);
		}

		for (i = 0; i < slp_multi_nluns; i++) {
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
	} else {
		/* original mainline code */
		printk(KERN_DEBUG "usb: %s pdata is not available. nluns=1\n",
		       __func__);
		config->fsg.nluns = 1;
		config->fsg.luns[0].removable = 1;

		common = fsg_common_init(NULL, cdev, &config->fsg);
		if (IS_ERR(common)) {
			kfree(config);
			return PTR_ERR(common);
		}

		err = sysfs_create_link(&f->dev->kobj,
					&common->luns[0].dev.kobj, "lun");
		if (err) {
			kfree(config);
			return err;
		}
	}

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
					 struct device_attribute *attr,
					 char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	return sprintf(buf, "%s\n", config->common->inquiry_string);
}

static ssize_t mass_storage_inquiry_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
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
		   mass_storage_inquiry_show, mass_storage_inquiry_store);

static struct device_attribute *mass_storage_function_attributes[] = {
	&dev_attr_inquiry_string,
	NULL
};

static struct android_usb_function mass_storage_function = {
	.name = "mass_storage",
	.init = mass_storage_function_init,
	.cleanup = mass_storage_function_cleanup,
	.bind_config = mass_storage_function_bind_config,
	.attributes = mass_storage_function_attributes,
};

static int accessory_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	int ret;

	/* pre-allocate a string ID for f-accessory interface */
	if (acc_string_defs[INTERFACE_STRING_INDEX].id == 0) {
		ret = usb_string_id(cdev);
		if (ret < 0)
			return ret;
		acc_string_defs[INTERFACE_STRING_INDEX].id = ret;
		acc_interface_desc.iInterface = ret;
	}

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

	return size < (sizeof(diag_clients)-1) ?
				size : sizeof(diag_clients)-1;
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
	int err = -1;
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

static struct android_usb_function *supported_functions[] = {
	&sdb_function,
	&acm_function,
	&mtp_function,
	&rndis_function,
	&mass_storage_function,
	&accessory_function,
	&diag_function,
	NULL
};

static int android_init_functions(struct android_dev *adev,
				  struct usb_composite_dev *cdev)
{
	struct android_usb_function *f;
	struct device_attribute **attrs;
	struct device_attribute *attr;
	int err = 0;
	int index = 0;

	list_for_each_entry(f, &adev->available_functions, available_list) {
		f->dev_name = kasprintf(GFP_KERNEL, "f_%s", f->name);
		f->dev = device_create(android_class, adev->dev,
				       MKDEV(0, index++), f, f->dev_name);
		if (IS_ERR(f->dev)) {
			pr_err("%s: Failed to create dev %s", __func__,
			       f->dev_name);
			err = PTR_ERR(f->dev);
			goto init_func_err_create;
		}

		if (f->init) {
			err = f->init(f, cdev);
			if (err) {
				pr_err("%s: Failed to init %s", __func__,
				       f->name);
				goto init_func_err_out;
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
			goto init_func_err_out;
		}
	}
	return 0;

 init_func_err_out:
	device_destroy(android_class, f->dev->devt);
 init_func_err_create:
	kfree(f->dev_name);
	return err;
}

static void android_cleanup_functions(struct android_dev *adev)
{
	struct android_usb_function *f;

	list_for_each_entry(f, &adev->available_functions, available_list) {
		if (f->dev) {
			device_destroy(android_class, f->dev->devt);
			kfree(f->dev_name);
		}

		if (f->cleanup)
			f->cleanup(f);
	}
}

static int
android_bind_enabled_functions(struct android_dev *adev,
			       struct usb_configuration *c)
{
	struct android_usb_function *f;
	int ret;

	list_for_each_entry(f, &adev->enabled_functions, enabled_list) {
		if ((f->usb_config_id == c->bConfigurationValue) ||
		    (f->usb_config_id == USB_CONFIGURATION_DUAL)) {
			printk(KERN_DEBUG "usb: %s f:%s\n", __func__, f->name);
			ret = f->bind_config(f, c);
			if (ret) {
				pr_err("%s: %s failed", __func__, f->name);
				return ret;
			}
		}
	}
	return 0;
}

static void
android_unbind_enabled_functions(struct android_dev *adev,
				 struct usb_configuration *c)
{
	struct android_usb_function *f;

	list_for_each_entry(f, &adev->enabled_functions, enabled_list) {
		if ((f->usb_config_id == c->bConfigurationValue) ||
		    (f->usb_config_id == USB_CONFIGURATION_DUAL))
			if (f->unbind_config)
				f->unbind_config(f, c);
	}
}

static int android_enable_function(struct android_dev *adev, char *name)
{
	struct android_usb_function *av_f, *en_f;

	printk(KERN_DEBUG "usb: %s name=%s\n", __func__, name);
	list_for_each_entry(av_f, &adev->available_functions, available_list) {
		if (!strcmp(name, av_f->name)) {
			list_for_each_entry(en_f, &adev->enabled_functions,
					    enabled_list) {
				if (av_f == en_f) {
					printk(KERN_INFO
					       "usb:%s already enabled!\n",
					       name);
					return 0;
				}
			}
			list_add_tail(&av_f->enabled_list,
				      &adev->enabled_functions);
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
	struct android_dev *adev = dev_get_drvdata(pdev);
	struct android_usb_function *f;
	char *buff = buf;

	list_for_each_entry(f, &adev->enabled_functions, enabled_list) {
		printk(KERN_DEBUG "usb: %s enabled_func=%s\n",
		       __func__, f->name);
		buff += sprintf(buff, "%s,", f->name);
	}
	if (buff != buf)
		*(buff - 1) = '\n';

	return buff - buf;
}

static ssize_t
functions_store(struct device *pdev, struct device_attribute *attr,
		const char *buff, size_t size)
{
	struct android_dev *adev = dev_get_drvdata(pdev);
	char *name;
	char buf[256], *b;
	int err;

	if (adev->enabled) {
		printk(KERN_INFO
		       "can't change usb functions(already enabled)!!\n");
		return 0;
	}

	INIT_LIST_HEAD(&adev->enabled_functions);

	printk(KERN_DEBUG "usb: %s buff=%s\n", __func__, buff);
	strncpy(buf, buff, sizeof(buf));
	b = strim(buf);

	while (b) {
		name = strsep(&b, ",");
		if (name) {
			err = android_enable_function(adev, name);
			if (err)
				pr_err("android_usb: Cannot enable '%s'", name);

		}
	}

	return size;
}

static ssize_t enable_show(struct device *pdev,
			   struct device_attribute *attr, char *buf)
{
	struct android_dev *adev = dev_get_drvdata(pdev);
	printk(KERN_DEBUG "usb: %s adev->enabled=%d\n",
	       __func__, adev->enabled);
	return sprintf(buf, "%d\n", adev->enabled);
}

static ssize_t enable_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct android_dev *adev = dev_get_drvdata(pdev);
	struct usb_composite_dev *cdev = adev->cdev;
	int enabled = 0;
	int err;

	sscanf(buff, "%d", &enabled);
	printk(KERN_DEBUG "usb: %s enabled=%d, !adev->enabled=%d\n",
	       __func__, enabled, !adev->enabled);

	mutex_lock(&enable_lock);

	if (enabled && !adev->enabled) {
		struct android_usb_function *f;

		/* update values in composite driver's
		 * copy of device descriptor
		 */
		cdev->desc.idVendor = device_desc.idVendor;
		cdev->desc.idProduct = device_desc.idProduct;
		cdev->desc.bcdDevice = device_desc.bcdDevice;

		list_for_each_entry(f, &adev->enabled_functions, enabled_list) {
			printk(KERN_DEBUG "usb: %s f:%s\n", __func__, f->name);
			if (!strcmp(f->name, "acm")) {
				printk(KERN_DEBUG
				       "usb: acm is enabled. (bcdDevice=0x400)\n");
				/* Samsung KIES needs fixed bcdDevice number */
				cdev->desc.bcdDevice = cpu_to_le16(0x0400);
			}
			if (!adev->dual_config
			    && ((f->usb_config_id == USB_CONFIGURATION_2)
				|| (f->usb_config_id ==
				    USB_CONFIGURATION_DUAL)))
				adev->dual_config = true;
		}

		cdev->desc.bDeviceClass = device_desc.bDeviceClass;
		cdev->desc.bDeviceSubClass = device_desc.bDeviceSubClass;
		cdev->desc.bDeviceProtocol = device_desc.bDeviceProtocol;

		printk(KERN_DEBUG "usb: %s vendor=%x,product=%x,bcdDevice=%x",
		       __func__, cdev->desc.idVendor,
		       cdev->desc.idProduct, cdev->desc.bcdDevice);
		printk(KERN_DEBUG ",Class=%x,SubClass=%x,Protocol=%x\n",
		       cdev->desc.bDeviceClass,
		       cdev->desc.bDeviceSubClass, cdev->desc.bDeviceProtocol);
		printk(KERN_DEBUG "usb: %s next cmd : usb_add_config\n",
		       __func__);

		err = usb_add_config(cdev,
				&first_config_driver, android_bind_config);
		if (err) {
			pr_err("usb_add_config fail-1st(%d)\n", err);
			adev->dual_config = false;
			goto done;
		}

		if (adev->dual_config) {
			err = usb_add_config(cdev, &second_config_driver,
				       android_bind_config);
			if (err) {
				pr_err("usb_add_config fail-2nd(%d)\n", err);
				usb_remove_config(cdev, &first_config_driver);
				adev->dual_config = false;
				goto done;
			}
		}

		usb_gadget_connect(cdev->gadget);
		adev->enabled = true;
	} else if (!enabled && adev->enabled) {
		usb_gadget_disconnect(cdev->gadget);
		/* Cancel pending control requests */
		usb_ep_dequeue(cdev->gadget->ep0, cdev->req);
		usb_remove_config(cdev, &first_config_driver);
		if (adev->dual_config)
			usb_remove_config(cdev, &second_config_driver);
		adev->enabled = false;
		adev->dual_config = false;
	} else if (!enabled) {
		usb_gadget_disconnect(cdev->gadget);
		adev->enabled = false;
	} else {
		pr_err("android_usb: already %s\n",
		       adev->enabled ? "enabled" : "disabled");
	}

done:
	mutex_unlock(&enable_lock);
	return size;
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
		const char *buf, size_t size)	\
{									\
	int value;	\
	if (sscanf(buf, format_string, &value) == 1) {			\
		device_desc.field = value;				\
		return size;						\
	}								\
	return -1;							\
}									\
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);

#define DESCRIPTOR_STRING_ATTR(field, buffer)	\
static ssize_t	\
field ## _show(struct device *dev, struct device_attribute *attr,	\
		char *buf)	\
{	\
	return sprintf(buf, "%s", buffer);	\
}	\
static ssize_t	\
field ## _store(struct device *dev, struct device_attribute *attr,	\
		const char *buf, size_t size)	\
{	\
	if (size >= sizeof(buffer))	\
		return -EINVAL;	\
	if (sscanf(buf, "%s", buffer) == 1) {	\
		return size;	\
	}	\
	return -1;	\
}	\
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

static DEVICE_ATTR(functions, S_IRUGO | S_IWUSR, functions_show,
		   functions_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, enable_show, enable_store);

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
	NULL
};

/*-------------------------------------------------------------------------*/
/* Composite driver */

static int android_bind_config(struct usb_configuration *c)
{
	struct android_dev *adev = _android_dev;
	int ret = 0;

	ret = android_bind_enabled_functions(adev, c);
	if (ret)
		return ret;

	return 0;
}

static void android_unbind_config(struct usb_configuration *c)
{
	struct android_dev *adev = _android_dev;

	android_unbind_enabled_functions(adev, c);
}

static int android_bind(struct usb_composite_dev *cdev)
{
	struct android_dev *adev = _android_dev;
	struct usb_gadget *gadget = cdev->gadget;
	int gcnum, id, ret;

	printk(KERN_DEBUG "usb: %s disconnect\n", __func__);
	usb_gadget_disconnect(gadget);

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
	strncpy(manufacturer_string, "Samsung\0",
		sizeof(manufacturer_string) - 1);
	strncpy(product_string, "SLP\0", sizeof(product_string) - 1);
	snprintf(serial_string, sizeof(serial_string),
		 "%08x%08x", system_serial_high, system_serial_low);

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_SERIAL_IDX].id = id;
	device_desc.iSerialNumber = id;

	ret = android_init_functions(adev, cdev);
	if (ret)
		return ret;

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
			   slp_longname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}

	usb_gadget_set_selfpowered(gadget);
	adev->cdev = cdev;

	return 0;
}

static int android_usb_unbind(struct usb_composite_dev *cdev)
{
	struct android_dev *adev = _android_dev;
	printk(KERN_DEBUG "usb: %s\n", __func__);
	android_cleanup_functions(adev);
	return 0;
}

static struct usb_composite_driver android_usb_driver = {
	.name = "android_usb",
	.dev = &device_desc,
	.strings = slp_dev_strings,
	.unbind = android_usb_unbind,
};

static int
android_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	struct android_dev *adev = _android_dev;
	struct usb_composite_dev *cdev = get_gadget_data(gadget);
	u8 b_requestType = ctrl->bRequestType;
	struct android_usb_function *f;
	int value = -EOPNOTSUPP;

	if ((b_requestType & USB_TYPE_MASK) == USB_TYPE_VENDOR) {
		struct usb_request *req = cdev->req;

		req->zero = 0;
		req->complete = composite_setup_complete;
		req->length = 0;
		gadget->ep0->driver_data = cdev;

		/* To check & report it to platform , we check it all */
		list_for_each_entry(f, &adev->available_functions,
			available_list) {
			if (f->ctrlrequest) {
				value = f->ctrlrequest(f, cdev, ctrl);
				if (value >= 0)
					break;
			}
		}
	}

	if (value < 0)
		value = composite_setup(gadget, ctrl);

	return value;
}

static int android_create_device(struct android_dev *adev)
{
	struct device_attribute **attrs = android_usb_attributes;
	struct device_attribute *attr;
	int err;

	adev->dev = device_create(android_class, NULL,
				  MKDEV(0, 0), NULL, "usb0");
	if (IS_ERR(adev->dev))
		return PTR_ERR(adev->dev);

	dev_set_drvdata(adev->dev, adev);

	while ((attr = *attrs++)) {
		err = device_create_file(adev->dev, attr);
		if (err) {
			device_destroy(android_class, adev->dev->devt);
			return err;
		}
	}
	return 0;
}

static void android_destroy_device(struct android_dev *adev)
{
	struct device_attribute **attrs = android_usb_attributes;
	struct device_attribute *attr;

	while ((attr = *attrs++))
		device_destroy(android_class, adev->dev->devt);

	dev_set_drvdata(adev->dev, NULL);

	device_unregister(adev->dev);
}

static CLASS_ATTR_STRING(version, S_IRUSR | S_IRGRP | S_IROTH,
			 USB_MODE_VERSION);

static int __devinit slp_multi_probe(struct platform_device *pdev)
{
	struct android_dev *adev;
	struct android_usb_function *f;
	struct android_usb_function **functions = supported_functions;
	struct slp_multi_platform_data *pdata = pdev->dev.platform_data;
	int err, i;

	printk(KERN_DEBUG "%s\n", __func__);
	android_class = class_create(THIS_MODULE, "usb_mode");
	if (IS_ERR(android_class))
		return PTR_ERR(android_class);

	err = class_create_file(android_class, &class_attr_version.attr);
	if (err) {
		printk(KERN_ERR "usb_mode: can't create sysfs version file\n");
		goto err_class;
	}

	adev = kzalloc(sizeof(*adev), GFP_KERNEL);
	if (!adev) {
		printk(KERN_ERR "usb_mode: can't alloc for adev\n");
		err = -ENOMEM;
		goto err_attr;
	}

	INIT_LIST_HEAD(&adev->available_functions);
	INIT_LIST_HEAD(&adev->enabled_functions);

	while ((f = *functions++)) {
		for (i = 0; i < pdata->nfuncs; i++) {
			struct slp_multi_func_data *funcs = &pdata->funcs[i];
			if (!strcmp(funcs->name, f->name)) {
				list_add_tail(&f->available_list,
					      &adev->available_functions);
				f->usb_config_id = funcs->usb_config_id;
			}
		}
	}

	err = android_create_device(adev);
	if (err) {
		printk(KERN_ERR "usb_mode: can't create device\n");
		goto err_alloc;
	}

	slp_multi_nluns = pdata->nluns;
	_android_dev = adev;

	/* Override composite driver functions */
	composite_driver.setup = android_setup;

	err = usb_composite_probe(&android_usb_driver, android_bind);
	if (err) {
		printk(KERN_ERR "usb_mode: can't probe composite\n");
		goto err_create;
	}

	printk(KERN_INFO "usb_mode driver, version:" USB_MODE_VERSION
	       "," " init Ok\n");

	return 0;

 err_create:
	android_destroy_device(adev);
 err_alloc:
	kfree(adev);
 err_attr:
	class_remove_file(android_class, &class_attr_version.attr);
 err_class:
	class_destroy(android_class);

	return err;
}

static int __devexit slp_multi_remove(struct platform_device *pdev)
{
	usb_composite_unregister(&android_usb_driver);
	android_destroy_device(_android_dev);
	kfree(_android_dev);
	_android_dev = NULL;
	class_remove_file(android_class, &class_attr_version.attr);
	class_destroy(android_class);

	return 0;
}

static struct platform_driver slp_multi_driver = {
	.probe = slp_multi_probe,
	.remove = __devexit_p(slp_multi_remove),
	.driver = {
		   .name = "slp_multi",
		   .owner = THIS_MODULE,
		   },
};

static int __init slp_multi_init(void)
{
	return platform_driver_register(&slp_multi_driver);
}

module_init(slp_multi_init);

static void __exit slp_multi_exit(void)
{
	platform_driver_unregister(&slp_multi_driver);
}

module_exit(slp_multi_exit);
