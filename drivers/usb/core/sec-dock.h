/*
 * drivers/usb/core/sec-dock.h
 *
 * Copyright (C) 2012 Samsung Electronics
 * Author: Woo-kwang Lee <wookwang.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
#include <linux/mfd/max77693.h>
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */
#include <linux/power_supply.h>

#define PSY_CHG_NAME "max77693-charger"

int usb_open_count;
bool is_smartdock;

static struct usb_device_id battery_notify_exception_table[] = {
/* add exception table list */
{ USB_DEVICE(0x1d6b, 0x0001), }, /* OHCI Host Controller */
{ USB_DEVICE(0x1d6b, 0x0002), }, /* EHCI Host Controller */
{ USB_DEVICE(0x1519, 0x0020), }, /* HSIC Device */
{ USB_DEVICE(0x05c6, 0x904c), }, /* Qualcomm modem */
{ USB_DEVICE(0x05c6, 0x9008), }, /* Qualcomm modem */
{ USB_DEVICE(0x08bb, 0x27c4), }, /* TI USB Audio DAC */
{ }	/* Terminating entry */
};

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
static struct usb_device_id audio_dock_table[] = {
/* add exception table list */
{ USB_DEVICE(0x04e8, 0x1220), }, /* Samsung Audio Dock */
{ USB_DEVICE(0x08bb, 0x27c4), }, /* TI USB Audio DAC */
{ }	/* Terminating entry */
};

static void call_audiodock_notify(struct usb_device *dev)
{
	struct usb_device_id	*id = audio_dock_table;
	/* check VID, PID */
	for (id = audio_dock_table; id->match_flags; id++) {
		if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
			(id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
		    id->idVendor == le16_to_cpu(dev->descriptor.idVendor) &&
		    id->idProduct == le16_to_cpu(dev->descriptor.idProduct)) {
			dev_info(&dev->dev, "Audio Dock is connected!\n");
			max77693_muic_attach_audio_dock();
			return;
		}
	}
}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */

/* real battery driver notification function */
static void set_online(int host_state)
{
	struct power_supply *psy = power_supply_get_by_name(PSY_CHG_NAME);
	union power_supply_propval value;
	int sub_type;

	if (!psy) {
		pr_err("%s: fail to get %s psy\n", __func__, PSY_CHG_NAME);
		return;
	}
	if (host_state)
		sub_type = ONLINE_SUB_TYPE_SMART_OTG;
	else
		sub_type = ONLINE_SUB_TYPE_SMART_NOTG;

	value.intval = 0;
	value.intval = (sub_type << 8);
	psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	return;
}

static int call_battery_notify(struct usb_device *dev, bool bOnOff)
{
	struct usb_device_id	*id = battery_notify_exception_table;

	/* Smart Dock hub must be skipped */
	if ((le16_to_cpu(dev->descriptor.idVendor) == 0x1a40 &&
	     le16_to_cpu(dev->descriptor.idProduct) == 0x0101)) {
		if (bOnOff)
			is_smartdock = 1;
		else
			is_smartdock = 0;
		return 0;
	}

	/* check VID, PID */
	for (id = battery_notify_exception_table; id->match_flags; id++) {
		if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
			(id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
		    id->idVendor == le16_to_cpu(dev->descriptor.idVendor) &&
		    id->idProduct == le16_to_cpu(dev->descriptor.idProduct)) {
				pr_info("%s : VID : 0x%x, PID : 0x%x skipped.\n",
					__func__, id->idVendor, id->idProduct);
			return 0;
		}
	}
	if (bOnOff)
		usb_open_count++;
	else
		usb_open_count--;

	/* battery driver notification */
	if (usb_open_count == 1 && bOnOff && is_smartdock) {
			pr_info("%s : VID : 0x%x, PID : 0x%x set 1000mA.\n",
					__func__,
					le16_to_cpu(dev->descriptor.idVendor),
					le16_to_cpu(dev->descriptor.idProduct));
			set_online(1);
	} else if (usb_open_count == 0 && !bOnOff) {
			pr_info("%s : VID : 0x%x, PID : 0x%x set 1700mA.\n",
					__func__,
					le16_to_cpu(dev->descriptor.idVendor),
					le16_to_cpu(dev->descriptor.idProduct));
			set_online(0);
	} else {
			pr_info("%s : VID : 0x%x, PID : 0x%x no action.\n",
					__func__,
					le16_to_cpu(dev->descriptor.idVendor),
					le16_to_cpu(dev->descriptor.idProduct));
			/* Nothing to do */
	}

	return 1;
}

