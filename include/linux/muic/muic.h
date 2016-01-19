/*
 * include/linux/muic/muic.h
 *
 * header file supporting MUIC common information
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

#ifndef __MUIC_H__
#define __MUIC_H__

#if defined(CONFIG_MUIC_FSA9485)
typedef struct fsa9485_muic_data muic_data_t;

#define MUIC_DEV_NAME	"muic-fsa9485"
#endif /* CONFIG_MUIC_FSA9485 */

#if defined(CONFIG_MUIC_MAX14577)
typedef struct max14577_muic_data muic_data_t;

#define MUIC_DEV_NAME	"muic-max14577"
#endif /* CONFIG_MUIC_FSA9485 */

/* MUIC Interrupt */
enum {
	MUIC_INTR_DETACH	= 0,
	MUIC_INTR_ATTACH
};

/* MUIC Dock Observer Callback parameter */
enum {
	MUIC_DOCK_DETACHED	= 0,
	MUIC_DOCK_DESKDOCK	= 1,
	MUIC_DOCK_CARDOCK	= 2,
	MUIC_DOCK_AUDIODOCK	= 7,
	MUIC_DOCK_SMARTDOCK	= 8,

//#if defined(CONFIG_MUIC_SUPPORT_FACTORY_BUTTON)
	MUIC_DOCK_FACTORY	= 11,
//#endif /* CONFIG_MUIC_SUPPORT_FACTORY_BUTTON */

};

/* MUIC Path */
enum {
	MUIC_PATH_USB_AP	= 0,
	MUIC_PATH_USB_CP,
	MUIC_PATH_UART_AP,
	MUIC_PATH_UART_CP,
	MUIC_PATH_OPEN,
	MUIC_PATH_AUDIO,
#if defined(CONFIG_MUIC_USE_11PIN_MHL_TO_USB)
	MUIC_PATH_DP_DM,
	MUIC_PATH_MHL,
#endif /* CONFIG_MUIC_USE_11PIN_MHL_TO_USB */
};

/* bootparam SWITCH_SEL */
enum {
	SWITCH_SEL_USB_MASK	= 0x1,
	SWITCH_SEL_UART_MASK	= 0x2,
};

/* MUIC ADC table */
typedef enum {
	ADC_GND			= 0x00,
	ADC_SEND_END		= 0x01,
	ADC_REMOTE_S11		= 0x0c,
	ADC_SMARTDOCK		= 0x10, /* 0x10000 40.2K ohm */
	ADC_AUDIODOCK		= 0x12, /* 0x10010 64.9K ohm */
	ADC_CEA936ATYPE1_CHG	= 0x17,	/* 0x10111 200K ohm */
	ADC_JIG_USB_OFF		= 0x18, /* 0x11000 255K ohm */
	ADC_JIG_USB_ON		= 0x19, /* 0x11001 301K ohm */
	ADC_DESKDOCK		= 0x1a, /* 0x11010 365K ohm */
	ADC_CEA936ATYPE2_CHG	= 0x1b, /* 0x11011 442K ohm */
	ADC_JIG_UART_OFF	= 0x1c, /* 0x11100 523K ohm */
	ADC_JIG_UART_ON		= 0x1d, /* 0x11101 619K ohm */
	ADC_OPEN		= 0x1f,

	ADC_ERROR		= 0xff, /* ADC value read error */
} muic_adc_t;


/* MUIC attached device type */
typedef enum {
	ATTACHED_DEV_NONE_MUIC = 0,
	ATTACHED_DEV_USB_MUIC,
	ATTACHED_DEV_OTG_MUIC,
	ATTACHED_DEV_TA_MUIC,
	ATTACHED_DEV_DESKDOCK_MUIC,
	ATTACHED_DEV_CARDOCK_MUIC,
	ATTACHED_DEV_JIG_UART_OFF_MUIC,
	ATTACHED_DEV_JIG_UART_OFF_VB_MUIC,	/* VBUS enabled */
	ATTACHED_DEV_JIG_UART_ON_MUIC,
	ATTACHED_DEV_JIG_USB_OFF_MUIC,
	ATTACHED_DEV_JIG_USB_ON_MUIC,
	ATTACHED_DEV_UNKNOWN_MUIC,

#if defined(CONFIG_MUIC_ADC_ADD_PLATFORM_DEVICE)
	ATTACHED_DEV_2A_TA_MUIC,
	ATTACHED_DEV_UNKNOWN_TA_MUIC,
#endif /* CONFIG_MUIC_ADC_ADD_PLATFORM_DEVICE */

//#if defined(CONFIG_MUIC_SUPPORT_FACTORY_BUTTON)
	ATTACHED_DEV_FACTORY_BUTTON,
//#endif /* CONFIG_MUIC_SUPPORT_FACTORY_BUTTON */

//#if defined(CONFIG_MUIC_SUPPORT_POGO_TA)
	ATTACHED_DEV_POGO_TA_MUIC,
//#endif /* CONFIG_MUIC_SUPPORT_POGO_TA */

//#if defined(CONFIG_MUIC_SUPPORT_POGO_USB)
	ATTACHED_DEV_POGO_USB_MUIC,
//#endif /* CONFIG_MUIC_SUPPORT_POGO_USB */

} muic_attached_dev_t;

/* muic common callback driver internal data structure
 * that setted at sec-switch.c file
 */
struct sec_switch_data {
	void (*init_cb) (void);
	int (*filter_vps_cb) (muic_data_t *muic_data);
	void (*usb_cb) (u8 attached);
	int (*set_safeout) (int usb_path);
	int (*host_notify_cb) (int enable);
	int (*charger_cb) (muic_attached_dev_t attached_dev);
	void (*dock_cb) (int type);
};

/* model dependant muic platform data structure
 * that setted at board-xxxx-muic.c file
 */
struct muic_platform_data {
	int irq_gpio;

	/* muic GPIO USB/UART SEL */
	int gpio_usb_sel;
	int gpio_uart_sel;

	/* muic current USB/UART path */
	int usb_path;
	int uart_path;

	/* muic GPIO control function */
	int (*init_gpio_cb) (int switch_sel);
	int (*set_gpio_usb_sel) (int usb_path);
	int (*set_gpio_uart_sel) (int uart_path);

#if defined(CONFIG_MUIC_SUPPORT_11PIN_DEV_GENDER)
	struct mutex muic_plat_mutex;

	bool need_usb_detach;

	int vbus_irq_gpio;
	bool (*is_vbus_high) (void);
#endif /* CONFIG_MUIC_SUPPORT_11PIN_DEV_GENDER */

#if defined(CONFIG_MUIC_USE_11PIN_MHL_TO_USB)
	int gpio_usb_mhl_sel;
	int (*set_gpio_usb_mhl_sel) (int usb_mhl_sel);
#endif /* CONFIG_MUIC_USE_11PIN_MHL_TO_USB */
};

#endif /* __MUIC_H__ */
