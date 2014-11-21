/*
 * Copyright (C) 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Wonguk Jeong <wonguk.jeong@samsung.com>
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

#ifndef __FSA9485_H__
#define __FSA9485_H__

#include <linux/muic/muic.h>

/* fsa9485 muic register read/write related information defines. */

/* Slave addr = 0x4A: MUIC */

/* FSA9485 I2C registers */
enum fsa9485_muic_reg {
	FSA9485_MUIC_REG_DEVID		= 0x01,
	FSA9485_MUIC_REG_CTRL		= 0x02,
	FSA9485_MUIC_REG_INT1		= 0x03,
	FSA9485_MUIC_REG_INT2		= 0x04,
	FSA9485_MUIC_REG_INTMASK1	= 0x05,
	FSA9485_MUIC_REG_INTMASK2	= 0x06,
	FSA9485_MUIC_REG_ADC		= 0x07,

	FSA9485_MUIC_REG_TIMING1	= 0x08,
	FSA9485_MUIC_REG_TIMING2	= 0x09,

	FSA9485_MUIC_REG_DEV_T1		= 0x0a,
	FSA9485_MUIC_REG_DEV_T2		= 0x0b,
	/* unused registers */
#if 0
	FSA9485_MUIC_REG_BUTTON1	= 0x0c,
	FSA9485_MUIC_REG_BUTTON2	= 0x0d,
	FSA9485_MUIC_REG_CK_STATUS	= 0x0e,
	FSA9485_MUIC_REG_CK_INT1	= 0x0f,
	FSA9485_MUIC_REG_CK_INT2	= 0x10,
	FSA9485_MUIC_REG_CK_INTMASK1	= 0x11,
	FSA9485_MUIC_REG_CK_INTMASK2	= 0x12,
#endif
	FSA9485_MUIC_REG_MANSW1		= 0x13,
	FSA9485_MUIC_REG_MANSW2		= 0x14,

	FSA9485_MUIC_REG_VBUS		= 0x1d,
	FSA9485_MUIC_REG_RESERV20	= 0x20,

	FSA9485_MUIC_REG_END,
};

/* FSA9485 REGISTER ENABLE or DISABLE bit */
#define FSA9485_ENABLE_BIT 1
#define FSA9485_DISABLE_BIT 0

/* FSA9485 Control register */
#define CTRL_SWITCH_OPEN_SHIFT		4
#define CTRL_RAW_DATA_SHIFT		3
#define CTRL_MANUAL_SW_SHIFT		2
#define CTRL_WAIT_SHIFT			1
#define CTRL_INT_MASK_SHIFT		0
#define CTRL_SWITCH_OPEN_MASK		(0x1 << CTRL_SWITCH_OPEN_SHIFT)
#define CTRL_RAW_DATA_MASK		(0x1 << CTRL_RAW_DATA_SHIFT)
#define CTRL_MANUAL_SW_MASK		(0x1 << CTRL_MANUAL_SW_SHIFT)
#define CTRL_WAIT_MASK			(0x1 << CTRL_WAIT_SHIFT)
#define CTRL_INT_MASK_MASK		(0x1 << CTRL_INT_MASK_SHIFT)
#define CTRL_MASK		(CTRL_SWITCH_OPEN_MASK | CTRL_RAW_DATA_MASK | \
				CTRL_MANUAL_SW_MASK | CTRL_WAIT_MASK | \
				CTRL_INT_MASK_MASK)

/* FSA9485 Interrupt 1 register */
#define INT_OVP_EN_SHIFT		5
#define INT_LKR_SHIFT			4
#define INT_LKP_SHIFT			3
#define INT_KP_SHIFT			2
#define INT_DETACH_SHIFT		1
#define INT_ATTACH_SHIFT		0
#define INT_OVP_EN_MASK			(0x1 << INT_OVP_EN_SHIFT)
#define INT_LKR_MASK			(0x1 << INT_LKR_SHIFT)
#define INT_LKP_MASK			(0x1 << INT_LKP_SHIFT)
#define INT_KP_MASK			(0x1 << INT_KP_SHIFT)
#define INT_DETACH_MASK			(0x1 << INT_DETACH_SHIFT)
#define INT_ATTACH_MASK			(0x1 << INT_ATTACH_SHIFT)

/* FSA9485 Interrupt 2 register */
#define INT_ADC_CHANGE_SHIFT		2
#define INT_RSRV_ATTACH_SHIFT		1
#define INT_AV_CHARGING_SHIFT		0
#define INT_ADC_CHANGE_MASK		(0x1 << INT_ADC_CHANGE_SHIFT)
#define INT_RSRV_ATTACH_MASK		(0x1 << INT_RSRV_ATTACH_SHIFT)
#define INT_AV_CHARGING_MASK		(0x1 << INT_AV_CHARGING_SHIFT)

/* FSA9485 ADC register */
#define ADC_ADC_SHIFT			0
#define ADC_ADC_MASK			(0x1f << ADC_ADC_SHIFT)

/* FSA9485 Timing Set 1 & 2 register Timing table */
#define ADC_DETECT_TIME_50MS		(0x00)
#define ADC_DETECT_TIME_100MS		(0x01)
#define ADC_DETECT_TIME_150MS		(0x02)
#define ADC_DETECT_TIME_200MS		(0x03)
#define ADC_DETECT_TIME_300MS		(0x04)
#define ADC_DETECT_TIME_400MS		(0x05)
#define ADC_DETECT_TIME_500MS		(0x06)
#define ADC_DETECT_TIME_600MS		(0x07)
#define ADC_DETECT_TIME_700MS		(0x08)
#define ADC_DETECT_TIME_800MS		(0x09)
#define ADC_DETECT_TIME_900MS		(0x0a)
#define ADC_DETECT_TIME_1000MS		(0x0b)

#define KEY_PRESS_TIME_100MS		(0x00)
#define KEY_PRESS_TIME_200MS		(0x10)
#define KEY_PRESS_TIME_300MS		(0x20)
#define KEY_PRESS_TIME_700MS		(0x60)

#define LONGKEY_PRESS_TIME_300MS	(0x00)
#define LONGKEY_PRESS_TIME_500MS	(0x02)
#define LONGKEY_PRESS_TIME_1000MS	(0x07)
#define LONGKEY_PRESS_TIME_1500MS	(0x0C)

#define SWITCHING_WAIT_TIME_10MS	(0x00)
#define SWITCHING_WAIT_TIME_210MS	(0xa0)

/* FSA9485 Device Type 1 register */
#define DEV_TYPE1_USB_OTG		(0x1 << 7)
#define DEV_TYPE1_DEDICATED_CHG		(0x1 << 6)
#define DEV_TYPE1_CDP			(0x1 << 5)
#define DEV_TYPE1_T1_T2_CHG		(0x1 << 4)
#define DEV_TYPE1_UART			(0x1 << 3)
#define DEV_TYPE1_USB			(0x1 << 2)
#define DEV_TYPE1_AUDIO_2		(0x1 << 1)
#define DEV_TYPE1_AUDIO_1		(0x1 << 0)

/* FSA9485 Device Type 2 register */
#define DEV_TYPE2_AV			(0x1 << 6)
#define DEV_TYPE2_TTY			(0x1 << 5)
#define DEV_TYPE2_PPD			(0x1 << 4)
#define DEV_TYPE2_JIG_UART_OFF		(0x1 << 3)
#define DEV_TYPE2_JIG_UART_ON		(0x1 << 2)
#define DEV_TYPE2_JIG_USB_OFF		(0x1 << 1)
#define DEV_TYPE2_JIG_USB_ON		(0x1 << 0)

/* FSA9485 Reserved_1D(VBUS) register */
#define RESERVED_1D_VBUS		(0x1 << 1)

/*
 * Manual Switch
 * D- [7:5] / D+ [4:2] / Vbus [1:0]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100: V_AUDIO
 * 00: Vbus to Open / 01: Vbus to Charger / 10: Vbus to MIC / 11: Vbus to Open
 *                                                            Just for FSA9485
 */
#define MANUAL_SW1_DM_SHIFT	5
#define MANUAL_SW1_DP_SHIFT	2
#define MANUAL_SW1_VBUS_SHIFT	0
#define MANUAL_SW1_D_OPEN	(0x0)
#define MANUAL_SW1_D_USB	(0x1)
#define MANUAL_SW1_D_AUDIO	(0x2)
#define MANUAL_SW1_D_UART	(0x3)
#define MANUAL_SW1_D_VAUDIO	(0x4)
#define MANUAL_SW1_D_MASK	(0x7)
#define MANUAL_SW1_V_OPEN	(0x0)
#define MANUAL_SW1_V_OTG	(0x1)
#define MANUAL_SW1_V_MIC	(0x2)
#define MANUAL_SW1_V_CHARGER	(0x3)

enum fsa9485_reg_manual_sw1_value {
	MANSW1_OPEN =		(MANUAL_SW1_D_OPEN << MANUAL_SW1_DM_SHIFT) | \
				(MANUAL_SW1_D_OPEN << MANUAL_SW1_DP_SHIFT) | \
				(MANUAL_SW1_V_OPEN << MANUAL_SW1_VBUS_SHIFT),
	MANSW1_OPEN_VB =	(MANUAL_SW1_D_OPEN << MANUAL_SW1_DM_SHIFT) | \
				(MANUAL_SW1_D_OPEN << MANUAL_SW1_DP_SHIFT) | \
				(MANUAL_SW1_V_CHARGER << MANUAL_SW1_VBUS_SHIFT),
	MANSW1_USB =		(MANUAL_SW1_D_USB << MANUAL_SW1_DM_SHIFT) | \
				(MANUAL_SW1_D_USB << MANUAL_SW1_DP_SHIFT) | \
				(MANUAL_SW1_V_CHARGER << MANUAL_SW1_VBUS_SHIFT),
	MANSW1_AUDIO =		(MANUAL_SW1_D_AUDIO << MANUAL_SW1_DM_SHIFT) | \
				(MANUAL_SW1_D_AUDIO << MANUAL_SW1_DP_SHIFT) | \
				(MANUAL_SW1_V_OPEN << MANUAL_SW1_VBUS_SHIFT),
	MANSW1_UART =		(MANUAL_SW1_D_UART << MANUAL_SW1_DM_SHIFT) | \
				(MANUAL_SW1_D_UART << MANUAL_SW1_DP_SHIFT) | \
				(MANUAL_SW1_V_OPEN << MANUAL_SW1_VBUS_SHIFT),
	MANSW1_VAUDIO =		(MANUAL_SW1_D_VAUDIO << MANUAL_SW1_DM_SHIFT) | \
				(MANUAL_SW1_D_VAUDIO << MANUAL_SW1_DP_SHIFT) | \
				(MANUAL_SW1_V_OPEN << MANUAL_SW1_VBUS_SHIFT),
};

enum fsa9485_muic_reg_init_value {
	REG_INTMASK1_VALUE		= (0xfc),
	REG_INTMASK2_VALUE		= (0x18),
	REG_TIMING1_VALUE		= (ADC_DETECT_TIME_500MS | \
					KEY_PRESS_TIME_100MS),
	REG_TIMING2_VALUE		= (LONGKEY_PRESS_TIME_300MS | \
					SWITCHING_WAIT_TIME_10MS),
};

#if defined(CONFIG_MUIC_ADC_ADD_PLATFORM_DEVICE)
enum charger_type_enum {
	CHARGER_TYPE_NONE		= 0,
	CHARGER_TYPE_TA,
	CHARGER_TYPE_2A_TA,
	CHARGER_TYPE_COUNT,		/* total count of charger_type_enum */
	CHARGER_TYPE_UNKNOWN,
	CHARGER_TYPE_ERR		= -1,
};

struct adc_charger_type_data {
	enum charger_type_enum chg_typ;
	unsigned int chg_adc_min;
	unsigned int chg_adc_max;
	const char *device_name;
};
#endif /* CONFIG_MUIC_ADC_ADD_PLATFORM_DEVICE */

/* muic chip specific internal data structure
 * that setted at muic-xxxx.c file
 */
struct fsa9485_muic_data {
	struct device *dev;
	struct i2c_client *i2c; /* i2c addr: 0x4A; MUIC */
	struct mutex muic_mutex;

	/* muic common callback driver internal data */
	struct sec_switch_data *switch_data;

	/* model dependant muic platform data */
	struct muic_platform_data *pdata;

	/* muic current attached device */
	muic_attached_dev_t	attached_dev;

	/* muic Device ID */
	u8 muic_vendor;			/* Vendor ID */
	u8 muic_version;		/* Version ID */

#if defined(CONFIG_MUIC_SUPPORT_11PIN_DEV_GENDER)
	int			vbus_irq;
#endif /* CONFIG_MUIC_SUPPORT_11PIN_DEV_GENDER */

	bool			is_usb_ready;
	bool			is_muic_ready;

	struct delayed_work	init_work;
	struct delayed_work	usb_work;
};

extern struct device *switch_device;

#endif /* __FSA9485_H__ */
