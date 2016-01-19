/*
 * max14577-muic.h - MUIC for the Maxim 14577
 *
 * Copyright (C) 2011 Samsung Electrnoics
 * SeungJin Hahn <sjin.hahn@samsung.com>
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

#ifndef __MAX14577_MUIC_H__
#define __MAX14577_MUIC_H__

#include <linux/muic/muic.h>

/* max14577 muic register read/write related information defines. */

/* MAX14577 STATUS1 register */
#define STATUS1_ADC_SHIFT		0
#define STATUS1_ADCLOW_SHIFT		5
#define STATUS1_ADCERR_SHIFT		6
#define STATUS1_ADC_MASK		(0x1f << STATUS1_ADC_SHIFT)
#define STATUS1_ADCLOW_MASK		(0x1 << STATUS1_ADCLOW_SHIFT)
#define STATUS1_ADCERR_MASK		(0x1 << STATUS1_ADCERR_SHIFT)

/* MAX14577 STATUS2 register */
#define STATUS2_CHGTYP_SHIFT		0
#define STATUS2_CHGDETRUN_SHIFT		3
#define STATUS2_DBCHG_SHIFT		5
#define STATUS2_VBVOLT_SHIFT		6
#define STATUS2_CHGTYP_MASK		(0x7 << STATUS2_CHGTYP_SHIFT)
#define STATUS2_CHGDETRUN_MASK		(0x1 << STATUS2_CHGDETRUN_SHIFT)
#define STATUS2_DBCHG_MASK		(0x1 << STATUS2_DBCHG_SHIFT)
#define STATUS2_VBVOLT_MASK		(0x1 << STATUS2_VBVOLT_SHIFT)

/* MAX14577 CONTROL1 register */
#define COMN1SW_SHIFT			0
#define COMP2SW_SHIFT			3
#define MICEN_SHIFT			6
#define IDBEN_SHIFT			7
#define COMN1SW_MASK			(0x7 << COMN1SW_SHIFT)
#define COMP2SW_MASK			(0x7 << COMP2SW_SHIFT)
#define MICEN_MASK			(0x1 << MICEN_SHIFT)
#define IDBEN_MASK			(0x1 << IDBEN_SHIFT)
#define CLEAR_IDBEN_MICEN_MASK		(COMN1SW_MASK | COMP2SW_MASK)

/* MAX14577 CONTROL3 register */
#define CTRL3_JIGSET_SHIFT		0
#define CTRL3_BOOTSET_SHIFT		2
#define CTRL3_ADCDBSET_SHIFT		4
#define CTRL3_JIGSET_MASK		(0x3 << CTRL3_JIGSET_SHIFT)
#define CTRL3_BOOTSET_MASK		(0x3 << CTRL3_BOOTSET_SHIFT)
#define CTRL3_ADCDBSET_MASK		(0x3 << CTRL3_ADCDBSET_SHIFT)

/* MAX14577 MUIC Charger Type Detection Output values */
typedef enum {
	/* No Valid voltage at VB (Vvb < Vvbdet) */
	CHGTYP_NOTHING_ATTACH		= 0x00,
	/* Unknown (D+/D- does not present a valid USB charger signature) */
	CHGTYP_USB			= 0x01,
	/* Charging Downstream Port */
	CHGTYP_CDP			= 0x02,
	/* Dedicated Charger (D+/D- shorted) */
	CHGTYP_DEDICATED_CHARGER	= 0x03,
	/* Special 500mA charger, max current 500mA */
	CHGTYP_500MA_CHARGER		= 0x04,
	/* Special 1A charger, max current 1A */
	CHGTYP_1A_CHARGER		= 0x05,
	/* Reserved for Future Use */
	CHGTYP_RFU			= 0x06,
	/* Dead Battery Charging, max current 100mA */
	CHGTYP_DB_100MA_CHARGER		= 0x07,
} chgtyp_t;

/* muic register value for COMN1, COMN2 in CTRL1 reg  */

/*
 * MAX14577 CONTROL1 register
 * ID Bypass [7] / Mic En [6] / D+ [5:3] / D- [2:0]
 * 0: ID Bypass Open / 1: IDB connect to UID
 * 0: Mic En Open / 1: Mic connect to VB
 * 000: Open / 001: USB / 010: Audio / 011: UART
 */
enum max14577_reg_ctrl1_val {
	MAX14577_MUIC_CTRL1_ID_OPEN	= 0x0,
	MAX14577_MUIC_CTRL1_ID_BYPASS	= 0x1,
	MAX14577_MUIC_CTRL1_MIC_OPEN	= 0x0,
	MAX14577_MUIC_CTRL1_MIC_VB	= 0x1,
	MAX14577_MUIC_CTRL1_COM_OPEN	= 0x00,
	MAX14577_MUIC_CTRL1_COM_USB	= 0x01,
	MAX14577_MUIC_CTRL1_COM_AUDIO	= 0x02,
	MAX14577_MUIC_CTRL1_COM_UART	= 0x03,
};

typedef enum {
	CTRL1_OPEN	= (MAX14577_MUIC_CTRL1_ID_OPEN << IDBEN_SHIFT) | \
			(MAX14577_MUIC_CTRL1_MIC_OPEN << MICEN_SHIFT) | \
			(MAX14577_MUIC_CTRL1_COM_OPEN << COMP2SW_SHIFT) | \
			(MAX14577_MUIC_CTRL1_COM_OPEN << COMN1SW_SHIFT),
	CTRL1_USB	= (MAX14577_MUIC_CTRL1_ID_OPEN << IDBEN_SHIFT) | \
			(MAX14577_MUIC_CTRL1_MIC_OPEN << MICEN_SHIFT) | \
			(MAX14577_MUIC_CTRL1_COM_USB << COMP2SW_SHIFT) | \
			(MAX14577_MUIC_CTRL1_COM_USB << COMN1SW_SHIFT),
	CTRL1_AUDIO	= (MAX14577_MUIC_CTRL1_ID_OPEN << IDBEN_SHIFT) | \
			(MAX14577_MUIC_CTRL1_MIC_OPEN << MICEN_SHIFT) | \
			(MAX14577_MUIC_CTRL1_COM_AUDIO << COMP2SW_SHIFT) | \
			(MAX14577_MUIC_CTRL1_COM_AUDIO << COMN1SW_SHIFT),
	CTRL1_UART	= (MAX14577_MUIC_CTRL1_ID_OPEN << IDBEN_SHIFT) | \
			(MAX14577_MUIC_CTRL1_MIC_OPEN << MICEN_SHIFT) | \
			(MAX14577_MUIC_CTRL1_COM_UART << COMP2SW_SHIFT) | \
			(MAX14577_MUIC_CTRL1_COM_UART << COMN1SW_SHIFT),
} max14577_reg_ctrl1_t;

enum max14577_muic_reg_init_value {
	REG_CONTROL3_VALUE		= (0x20),
};

struct max14577_muic_vps_data {
	u8			adcerr;
	u8			adclow;
	muic_adc_t		adc;
	u8			vbvolt;
	u8			chgdetrun;
	chgtyp_t		chgtyp;
	max14577_reg_ctrl1_t	control1;
	const char		*vps_name;
	const muic_attached_dev_t attached_dev;
};

/* muic chip specific internal data structure
 * that setted at max14577-muic.c file
 */
struct max14577_muic_data {
	struct device *dev;
	struct i2c_client *i2c; /* i2c addr: 0x4A; MUIC */
	struct mutex muic_mutex;

	/* model dependant mfd platform data */
	struct max14577_platform_data *mfd_pdata;

	int			irq_adcerr;
	int			irq_adc;
	int			irq_chgtyp;
	int			irq_vbvolt;

	/* muic common callback driver internal data */
	struct sec_switch_data *switch_data;

	/* model dependant muic platform data */
	struct muic_platform_data *pdata;

	/* muic current attached device */
	muic_attached_dev_t	attached_dev;

	bool			is_usb_ready;
	bool			is_muic_ready;

	struct delayed_work	init_work;
	struct delayed_work	usb_work;
};

extern struct device *switch_device;

#endif /* __MAX14577_MUIC_H__ */
