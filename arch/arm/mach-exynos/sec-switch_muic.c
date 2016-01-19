/*
 * arch/arm/mach-exynos/sec-switch.c
 *
 * c source file supporting MUIC common platform device register
 *
 * Copyright (C) 2010 Samsung Electronics
 * Seung-Jin Hahn <sjin.hahn@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/err.h>

/* MUIC header */
#if defined(CONFIG_MUIC_FSA9485)
#include <linux/muic/fsa9485.h>
#endif /* CONFIG_MUIC_FSA9485 */

#if defined(CONFIG_MUIC_MAX14577)
#include <linux/muic/max14577-muic.h>
#endif /* CONFIG_MUIC_MAX14577 */

#include <linux/muic/muic.h>

/* switch device header */
#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif

/* USB header */
#include <plat/udc-hs.h>
#include <plat/devs.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

//#include "vps_table.h"

#ifdef CONFIG_SWITCH
static struct switch_dev switch_dock = {
	.name = "dock",
};
#endif

extern struct class *sec_class;

struct device *switch_device;

/* charger cable state */
extern bool is_cable_attached;

static void muic_init_cb(void)
{
#ifdef CONFIG_SWITCH
	int ret;

	/* for CarDock, DeskDock */
	ret = switch_dev_register(&switch_dock);
	if (ret < 0)
		pr_err("%s:%s Failed to register dock switch(%d)\n",
				MUIC_DEV_NAME, __func__, ret);

	pr_info("%s:%s switch_dock switch_dev_register\n", MUIC_DEV_NAME,
			__func__);
#endif
}

static int muic_filter_vps_cb(muic_data_t *muic_data)
{
	struct muic_platform_data *pdata = muic_data->pdata;
	struct i2c_client *i2c = muic_data->i2c;
	int intr = MUIC_INTR_DETACH;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	return intr;
}

/* usb cable call back function */
static void muic_usb_cb(u8 usb_mode)
{

	struct usb_gadget *gadget = platform_get_drvdata(&s3c_device_usbgadget);

	pr_info("%s:%s MUIC usb_cb:%d\n", MUIC_DEV_NAME, __func__, usb_mode);

	if (gadget) {
		switch (usb_mode) {
		case USB_CABLE_DETACHED:
			pr_info("usb: muic: USB_CABLE_DETACHED(%d)\n",
				usb_mode);
			usb_gadget_vbus_disconnect(gadget);
			break;
		case USB_CABLE_ATTACHED:
			pr_info("usb: muic: USB_CABLE_ATTACHED(%d)\n",
				usb_mode);
			usb_gadget_vbus_connect(gadget);
			break;
		default:
			pr_info("usb: muic: invalid mode%d\n", usb_mode);
		}
	}

}

EXPORT_SYMBOL(muic_usb_cb);

static int muic_charger_cb(muic_attached_dev_t cable_type)
{
	pr_info("%s:%s %d\n", MUIC_DEV_NAME, __func__, cable_type);

	switch (cable_type) {
	case ATTACHED_DEV_NONE_MUIC:
	case ATTACHED_DEV_OTG_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		break;
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_POGO_USB_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		break;
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_POGO_TA_MUIC:
#if defined(CONFIG_MUIC_ADC_ADD_PLATFORM_DEVICE)
	case ATTACHED_DEV_2A_TA_MUIC:
	case ATTACHED_DEV_UNKNOWN_TA_MUIC:
#endif /* CONFIG_MUIC_ADC_ADD_PLATFORM_DEVICE */
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_CARDOCK_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
		break;
	default:
		pr_err("%s:%s invalid type:%d\n", MUIC_DEV_NAME, __func__,
				cable_type);
		return -EINVAL;
	}

	return 0;
}

static void muic_dock_cb(int type)
{
	pr_info("%s:%s MUIC dock type=%d\n", MUIC_DEV_NAME, __func__, type);
#ifdef CONFIG_SWITCH
	switch_set_state(&switch_dock, type);
#endif
}

struct sec_switch_data sec_switch_data = {
	.init_cb = muic_init_cb,
	.filter_vps_cb = muic_filter_vps_cb,
	.usb_cb = muic_usb_cb,
	.charger_cb = muic_charger_cb,
	.dock_cb = muic_dock_cb,
};

static int __init sec_switch_init(void)
{
	int ret = 0;
	switch_device = device_create(sec_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_device)) {
		pr_err("%s:%s Failed to create device(switch)!\n",
				MUIC_DEV_NAME, __func__);
		return -ENODEV;
	}

	return ret;
};

device_initcall(sec_switch_init);
