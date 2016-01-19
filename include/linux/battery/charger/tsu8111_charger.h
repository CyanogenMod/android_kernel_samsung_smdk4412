/*
 * tsu8111_charger.h
 * Samsung TSU8111 Charger Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
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

#ifndef __TSU8111_CHARGER_H
#define __TSU8111_CHARGER_H __FILE__

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */
#define SEC_CHARGER_I2C_SLAVEADDR	(0x4A >> 1)

/* Register define */
#define TSU8111_DEVICEID		0x01
#define TSU8111_CONTROL			0x02
#define TSU8111_INTERRUPT1		0x03
#define TSU8111_INTERRUPT2		0x04
#define TSU8111_INTERRUPT_MASK1		0x05
#define TSU8111_INTERRUPT_MASK2		0x06
#define TSU8111_ADC			0x07
#define TSU8111_TIMING_SET1		0x08
#define TSU8111_TIMING_SET2		0x09
#define TSU8111_DEVICE_TYPE1		0x0A
#define TSU8111_DEVICE_TYPE2		0x0B
#define TSU8111_BUTTON1			0x0C
#define TSU8111_BUTTON2			0x0D
#define TSU8111_MANUALSW1		0x13
#define TSU8111_MANUALSW2		0x14
#define TSU8111_DEVICE_TYPE3		0x15
#define TSU8111_RESET			0x1B
#define TSU8111_CHARGER_CONTROL1	0x20
#define TSU8111_CHARGER_CONTROL2	0x21
#define TSU8111_CHARGER_CONTROL3	0x22
#define TSU8111_CHARGER_CONTROL4	0x23
#define TSU8111_CHARGER_INTERRUPT	0x24
#define TSU8111_CHARGER_INTMASK		0x25
#define TSU8111_CHARGER_STATUS		0x26

#define CH_IDLE		0x01
#define CH_PC		0x02
#define CH_FC		0x04
#define CH_CV		0x08
#define CH_DONE		0x10
#define CH_PTE		0x40
#define CH_FTE		0x80

#endif /* __TSU8111_CHARGER_H */
