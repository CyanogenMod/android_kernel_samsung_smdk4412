/*
 * max14577-private.h - Common API for the Maxim 14577 internal sub chip
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __MAX14577_PRIVATE_H__
#define __MAX14577_PRIVATE_H__

#include <linux/i2c.h>

#define MAX14577_I2C_ADDR		(0x4A >> 1)
#define MAX14577_REG_INVALID		(0xff)

/* Slave addr = 0x4A: Interrupt */
enum max14577_reg {
	MAX14577_REG_DEVICEID		= 0x00,
	MAX14577_REG_INT1		= 0x01,
	MAX14577_REG_INT2		= 0x02,
	MAX14577_REG_INT3		= 0x03,
	MAX14577_REG_INTMASK1		= 0x07,
	MAX14577_REG_INTMASK2		= 0x08,
	MAX14577_REG_INTMASK3		= 0x09,
	MAX14577_REG_CDETCTRL1		= 0x0A,
	MAX14577_REG_CONTROL2		= 0x0D,

	MAX14577_REG_END,
};

/* Slave addr = 0x4A: MUIC */
enum max14577_muic_reg {
	MAX14577_MUIC_REG_STATUS1	= 0x04,
	MAX14577_MUIC_REG_STATUS2	= 0x05,
	MAX14577_MUIC_REG_CONTROL1	= 0x0C,
	MAX14577_MUIC_REG_CONTROL3	= 0x0E,

	MAX14577_MUIC_REG_END,
};

/* Slave addr = 0x4A: Charger */
enum max14577_charger_reg {
	MAX14577_CHG_REG_STATUS3	= 0x06,
	MAX14577_CHG_REG_CHG_CTRL1	= 0x0F,
	MAX14577_CHG_REG_CHG_CTRL2	= 0x10,
	MAX14577_CHG_REG_CHG_CTRL3	= 0x11,
	MAX14577_CHG_REG_CHG_CTRL4	= 0x12,
	MAX14577_CHG_REG_CHG_CTRL5	= 0x13,
	MAX14577_CHG_REG_CHG_CTRL6	= 0x14,
	MAX14577_CHG_REG_CHG_CTRL7	= 0x15,

	MAX14577_CHG_REG_END,
};

/* MAX14577 REGISTER ENABLE or DISABLE bit */
enum max14577_reg_bit_control {
	MAX14577_DISABLE_BIT		= 0,
	MAX14577_ENABLE_BIT,
};

/* MAX14577 CDETCTRL1 register */
#define CHGDETEN_SHIFT			0
#define CHGTYPMAN_SHIFT			1
#define DCHKTM_SHIFT			4
#define CHGDETEN_MASK			(0x1 << CHGDETEN_SHIFT)
#define CHGTYPMAN_MASK			(0x1 << CHGTYPMAN_SHIFT)
#define DCHKTM_MASK			(0x1 << DCHKTM_SHIFT)

/* MAX14577 CONTROL2 register */
#define CTRL2_LOWPWR_SHIFT		0
#define CTRL2_CPEN_SHIFT		2
#define CTRL2_ACCDET_SHIFT		5
#define CTRL2_LOWPWR_MASK		(0x1 << CTRL2_LOWPWR_SHIFT)
#define CTRL2_CPEN_MASK			(0x1 << CTRL2_CPEN_SHIFT)
#define CTRL2_ACCDET_MASK		(0x1 << CTRL2_ACCDET_SHIFT)
#define CTRL2_CPEN1_LOWPWR0 ((MAX14577_ENABLE_BIT << CTRL2_CPEN_SHIFT) | \
				(MAX14577_DISABLE_BIT << CTRL2_LOWPWR_SHIFT))
#define CTRL2_CPEN0_LOWPWR1 ((MAX14577_DISABLE_BIT << CTRL2_CPEN_SHIFT) | \
				(MAX14577_ENABLE_BIT << CTRL2_LOWPWR_SHIFT))

enum max14577_irq_source {
	MAX14577_IRQ_INT1 = 0,
	MAX14577_IRQ_INT2,
	MAX14577_IRQ_INT3,

	MAX14577_IRQ_REGS_NUM,
};

enum max14577_irq {
	/* INT1 */
	MAX14577_IRQ_INT1_ADC,
	MAX14577_IRQ_INT1_ADCLOW,
	MAX14577_IRQ_INT1_ADCERR,

	/* INT2 */
	MAX14577_IRQ_INT2_CHGTYP,
	MAX14577_IRQ_INT2_CHGDETREUN,
	MAX14577_IRQ_INT2_DCDTMR,
	MAX14577_IRQ_INT2_DBCHG,
	MAX14577_IRQ_INT2_VBVOLT,

	/* INT3 */
	MAX14577_IRQ_INT3_EOC,
	MAX14577_IRQ_INT3_CGMBC,
	MAX14577_IRQ_INT3_OVP,
	MAX14577_IRQ_INT3_MBCCHGERR,

	MAX14577_IRQ_NUM,
};

struct max14577_dev {
	struct device *dev;
	struct i2c_client *i2c; /* Slave addr = 0x4A */
	struct mutex i2c_lock;

	int irq;
	struct mutex irq_lock;
	int irq_masks_cur[MAX14577_IRQ_REGS_NUM];
	int irq_masks_cache[MAX14577_IRQ_REGS_NUM];

	/* Device ID */
	u8 vendor_id;	/* Vendor Identification */
	u8 device_id;	/* Chip Version */

	struct max14577_platform_data *pdata;
};

extern int max14577_irq_init(struct max14577_dev *max14577);
extern void max14577_irq_exit(struct max14577_dev *max14577);
extern int max14577_irq_resume(struct max14577_dev *max14577);

/* MAX14577 shared i2c API function */
extern int max14577_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int max14577_bulk_read(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int max14577_write_reg(struct i2c_client *i2c, u8 reg, u8 value);
extern int max14577_bulk_write(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int max14577_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);

#endif /* __MAX14577_PRIVATE_H__ */
