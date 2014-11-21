/*
 * This file holds the definitions of quirks found in USB devices.
 * Only quirks that affect the whole device, not an interface,
 * belong here.
 */

#ifndef __LINUX_USB_QUIRKS_H
#define __LINUX_USB_QUIRKS_H

/* string descriptors must not be fetched using a 255-byte read */
#define USB_QUIRK_STRING_FETCH_255	0x00000001

/* device can't resume correctly so reset it instead */
#define USB_QUIRK_RESET_RESUME		0x00000002

/* device can't handle Set-Interface requests */
#define USB_QUIRK_NO_SET_INTF		0x00000004

/* device can't handle its Configuration or Interface strings */
#define USB_QUIRK_CONFIG_INTF_STRINGS	0x00000008

/*device will morph if reset, don't use reset for handling errors */
#define USB_QUIRK_RESET_MORPHS		0x00000010

/* device has more interface descriptions than the bNumInterfaces count,
   and can't handle talking to these interfaces */
#define USB_QUIRK_HONOR_BNUMINTERFACES	0x00000020

/* device needs a pause during initialization, after we read the device
   descriptor */
#define USB_QUIRK_DELAY_INIT		0x00000040

/* device does not support reset-resume */
#define USB_QUIRK_NO_RESET_RESUME	0x00000080

/* device does not need GET_STATUS request */
#define USB_QUIRK_NO_GET_STATUS		0x00000100

/* device needs hsic specific tunning */
#define USB_QUIRK_HSIC_TUNE		0x00000200

/* resume bus driver after dpm resume  */
#define USB_QUIRK_NO_DPM_RESUME         0x00000400

#endif /* __LINUX_USB_QUIRKS_H */
