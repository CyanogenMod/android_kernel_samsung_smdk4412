/*
 * Platform data for Android USB
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
#ifndef	__LINUX_USB_ANDROID_H
#define	__LINUX_USB_ANDROID_H

#include <linux/usb/composite.h>
#include <linux/if_ether.h>

struct android_usb_product {
	/* Vendor ID for this set of functions.
	 * Default vendor_id in platform data will be used if this is zero.
	 */
	__u16 vendor_id;

	/* Product ID for this set of functions. */
	__u16 product_id;

	/* List of function names associated with this product.
	 * This is used to compute the USB product ID dynamically
	 * based on which functions are enabled.
	 */
	int num_functions;
	char **functions;
};

struct android_usb_platform_data {
	/* USB device descriptor fields */
	__u16 vendor_id;

	/* Default product ID. */
	__u16 product_id;

	__u16 version;

	char *product_name;
	char *manufacturer_name;
	char *serial_number;

	/* List of available USB products.
	 * This is used to compute the USB product ID dynamically
	 * based on which functions are enabled.
	 * if num_products is zero or no match can be found,
	 * we use the default product ID
	 */
	int num_products;
	struct android_usb_product *products;

	/* List of all supported USB functions.
	 * This list is used to define the order in which
	 * the functions appear in the configuration's list of USB interfaces.
	 * This is necessary to avoid depending upon the order in which
	 * the individual function drivers are initialized.
	 */
	int num_functions;
	char **functions;

	/* Number of LUNs function have  [For USB Mass storage]
	 * (anywhere from 1 to FSG_MAX_LUNS which is 8).
	 */
	unsigned int            nluns;
	/* cdfs supprot : verizon require */
	unsigned int		cdfs_support;
};

/* Platform data for "usb_mass_storage" driver. */
struct usb_mass_storage_platform_data {
	/* Contains values for the SC_INQUIRY SCSI command. */
	char *vendor;
	char *product;
	int release;

	/* number of LUNS */
	int nluns;
};

/* Platform data for USB ethernet driver. */
struct usb_ether_platform_data {
	u8	ethaddr[ETH_ALEN];
	u32	vendorID;
	const char *vendorDescr;
};

/* Platform data for ACM driver. */
struct acm_platform_data {
	u8	num_inst;
};

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
int register_usb_composite_notifier(struct notifier_block *notifier);
int unregister_usb_composite_notifier(struct notifier_block *notifier);
#else
#define register_usb_composite_notifier		NULL
#define unregister_usb_composite_notifier	NULL
#endif

#endif	/* __LINUX_USB_ANDROID_H */
