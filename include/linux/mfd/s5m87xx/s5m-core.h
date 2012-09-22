/*
 * s5m-core.h
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __LINUX_MFD_S5M_CORE_H
#define __LINUX_MFD_S5M_CORE_H

#define NUM_IRQ_REGS	4

enum s5m_device_type {
	S5M8751X,
	S5M8763X,
	S5M8767X,
};

/* S5M8767 registers */
enum s5m8767_reg {
	S5M8767_REG_ID,
	S5M8767_REG_INT1,
	S5M8767_REG_INT2,
	S5M8767_REG_INT3,
	S5M8767_REG_INT1M,
	S5M8767_REG_INT2M,
	S5M8767_REG_INT3M,
	S5M8767_REG_STATUS1,
	S5M8767_REG_STATUS2,
	S5M8767_REG_STATUS3,
	S5M8767_REG_CTRL1,
	S5M8767_REG_CTRL2,
	S5M8767_REG_LOWBAT1,
	S5M8767_REG_LOWBAT2,
	S5M8767_REG_BUCHG,
	S5M8767_REG_DVSRAMP,
	S5M8767_REG_DVSTIMER2 = 0x10,
	S5M8767_REG_DVSTIMER3,
	S5M8767_REG_DVSTIMER4,
	S5M8767_REG_LDO1,
	S5M8767_REG_LDO2,
	S5M8767_REG_LDO3,
	S5M8767_REG_LDO4,
	S5M8767_REG_LDO5,
	S5M8767_REG_LDO6,
	S5M8767_REG_LDO7,
	S5M8767_REG_LDO8,
	S5M8767_REG_LDO9,
	S5M8767_REG_LDO10,
	S5M8767_REG_LDO11,
	S5M8767_REG_LDO12,
	S5M8767_REG_LDO13,
	S5M8767_REG_LDO14 = 0x20,
	S5M8767_REG_LDO15,
	S5M8767_REG_LDO16,
	S5M8767_REG_LDO17,
	S5M8767_REG_LDO18,
	S5M8767_REG_LDO19,
	S5M8767_REG_LDO20,
	S5M8767_REG_LDO21,
	S5M8767_REG_LDO22,
	S5M8767_REG_LDO23,
	S5M8767_REG_LDO24,
	S5M8767_REG_LDO25,
	S5M8767_REG_LDO26,
	S5M8767_REG_LDO27,
	S5M8767_REG_LDO28,
	S5M8767_REG_UVLO = 0x31,
	S5M8767_REG_BUCK1CTRL1,
	S5M8767_REG_BUCK1CTRL2,
	S5M8767_REG_BUCK2CTRL,
	S5M8767_REG_BUCK2DVS1,
	S5M8767_REG_BUCK2DVS2,
	S5M8767_REG_BUCK2DVS3,
	S5M8767_REG_BUCK2DVS4,
	S5M8767_REG_BUCK2DVS5,
	S5M8767_REG_BUCK2DVS6,
	S5M8767_REG_BUCK2DVS7,
	S5M8767_REG_BUCK2DVS8,
	S5M8767_REG_BUCK3CTRL,
	S5M8767_REG_BUCK3DVS1,
	S5M8767_REG_BUCK3DVS2,
	S5M8767_REG_BUCK3DVS3,
	S5M8767_REG_BUCK3DVS4,
	S5M8767_REG_BUCK3DVS5,
	S5M8767_REG_BUCK3DVS6,
	S5M8767_REG_BUCK3DVS7,
	S5M8767_REG_BUCK3DVS8,
	S5M8767_REG_BUCK4CTRL,
	S5M8767_REG_BUCK4DVS1,
	S5M8767_REG_BUCK4DVS2,
	S5M8767_REG_BUCK4DVS3,
	S5M8767_REG_BUCK4DVS4,
	S5M8767_REG_BUCK4DVS5,
	S5M8767_REG_BUCK4DVS6,
	S5M8767_REG_BUCK4DVS7,
	S5M8767_REG_BUCK4DVS8,
	S5M8767_REG_BUCK5CTRL1,
	S5M8767_REG_BUCK5CTRL2,
	S5M8767_REG_BUCK5CTRL3,
	S5M8767_REG_BUCK5CTRL4,
	S5M8767_REG_BUCK5CTRL5,
	S5M8767_REG_BUCK6CTRL1,
	S5M8767_REG_BUCK6CTRL2,
	S5M8767_REG_BUCK7CTRL1,
	S5M8767_REG_BUCK7CTRL2,
	S5M8767_REG_BUCK8CTRL1,
	S5M8767_REG_BUCK8CTRL2,
	S5M8767_REG_BUCK9CTRL1,
	S5M8767_REG_BUCK9CTRL2,
	S5M8767_REG_LDO1CTRL,
	S5M8767_REG_LDO2_1CTRL,
	S5M8767_REG_LDO2_2CTRL,
	S5M8767_REG_LDO2_3CTRL,
	S5M8767_REG_LDO2_4CTRL,
	S5M8767_REG_LDO3CTRL,
	S5M8767_REG_LDO4CTRL,
	S5M8767_REG_LDO5CTRL,
	S5M8767_REG_LDO6CTRL,
	S5M8767_REG_LDO7CTRL,
	S5M8767_REG_LDO8CTRL,
	S5M8767_REG_LDO9CTRL,
	S5M8767_REG_LDO10CTRL,
	S5M8767_REG_LDO11CTRL,
	S5M8767_REG_LDO12CTRL,
	S5M8767_REG_LDO13CTRL,
	S5M8767_REG_LDO14CTRL,
	S5M8767_REG_LDO15CTRL,
	S5M8767_REG_LDO16CTRL,
	S5M8767_REG_LDO17CTRL,
	S5M8767_REG_LDO18CTRL,
	S5M8767_REG_LDO19CTRL,
	S5M8767_REG_LDO20CTRL,
	S5M8767_REG_LDO21CTRL,
	S5M8767_REG_LDO22CTRL,
	S5M8767_REG_LDO23CTRL,
	S5M8767_REG_LDO24CTRL,
	S5M8767_REG_LDO25CTRL,
	S5M8767_REG_LDO26CTRL,
	S5M8767_REG_LDO27CTRL,
	S5M8767_REG_LDO28CTRL,
	S5M8767_REG_BUCK1DVS2 = 0xE2,
};

/* S5M8763 registers */
enum s5m8763_reg {
	S5M8763_REG_IRQ1,
	S5M8763_REG_IRQ2,
	S5M8763_REG_IRQ3,
	S5M8763_REG_IRQ4,
	S5M8763_REG_IRQM1,
	S5M8763_REG_IRQM2,
	S5M8763_REG_IRQM3,
	S5M8763_REG_IRQM4,
	S5M8763_REG_STATUS1,
	S5M8763_REG_STATUS2,
	S5M8763_REG_STATUSM1,
	S5M8763_REG_STATUSM2,
	S5M8763_REG_CHGR1,
	S5M8763_REG_CHGR2,
	S5M8763_REG_LDO_ACTIVE_DISCHARGE1,
	S5M8763_REG_LDO_ACTIVE_DISCHARGE2,
	S5M8763_REG_BUCK_ACTIVE_DISCHARGE3,
	S5M8763_REG_ONOFF1,
	S5M8763_REG_ONOFF2,
	S5M8763_REG_ONOFF3,
	S5M8763_REG_ONOFF4,
	S5M8763_REG_BUCK1_VOLTAGE1,
	S5M8763_REG_BUCK1_VOLTAGE2,
	S5M8763_REG_BUCK1_VOLTAGE3,
	S5M8763_REG_BUCK1_VOLTAGE4,
	S5M8763_REG_BUCK2_VOLTAGE1,
	S5M8763_REG_BUCK2_VOLTAGE2,
	S5M8763_REG_BUCK3,
	S5M8763_REG_BUCK4,
	S5M8763_REG_LDO1_LDO2,
	S5M8763_REG_LDO3,
	S5M8763_REG_LDO4,
	S5M8763_REG_LDO5,
	S5M8763_REG_LDO6,
	S5M8763_REG_LDO7,
	S5M8763_REG_LDO7_LDO8,
	S5M8763_REG_LDO9_LDO10,
	S5M8763_REG_LDO11,
	S5M8763_REG_LDO12,
	S5M8763_REG_LDO13,
	S5M8763_REG_LDO14,
	S5M8763_REG_LDO15,
	S5M8763_REG_LDO16,
	S5M8763_REG_BKCHR,
	S5M8763_REG_LBCNFG1,
	S5M8763_REG_LBCNFG2,
};

enum s5m8767_irq {
	S5M8767_IRQ_PWRR,
	S5M8767_IRQ_PWRF,
	S5M8767_IRQ_PWR1S,
	S5M8767_IRQ_JIGR,
	S5M8767_IRQ_JIGF,
	S5M8767_IRQ_LOWBAT2,
	S5M8767_IRQ_LOWBAT1,

	S5M8767_IRQ_MRB,
	S5M8767_IRQ_DVSOK2,
	S5M8767_IRQ_DVSOK3,
	S5M8767_IRQ_DVSOK4,

	S5M8767_IRQ_RTC60S,
	S5M8767_IRQ_RTCA1,
	S5M8767_IRQ_RTCA2,
	S5M8767_IRQ_SMPL,
	S5M8767_IRQ_RTC1S,
	S5M8767_IRQ_WTSR,

	S5M8767_IRQ_NR,
};

#define S5M8767_IRQ_PWRR_MASK		(1 << 0)
#define S5M8767_IRQ_PWRF_MASK		(1 << 1)
#define S5M8767_IRQ_PWR1S_MASK		(1 << 3)
#define S5M8767_IRQ_JIGR_MASK		(1 << 4)
#define S5M8767_IRQ_JIGF_MASK		(1 << 5)
#define S5M8767_IRQ_LOWBAT2_MASK	(1 << 6)
#define S5M8767_IRQ_LOWBAT1_MASK	(1 << 7)

#define S5M8767_IRQ_MRB_MASK		(1 << 2)
#define S5M8767_IRQ_DVSOK2_MASK		(1 << 3)
#define S5M8767_IRQ_DVSOK3_MASK		(1 << 4)
#define S5M8767_IRQ_DVSOK4_MASK		(1 << 5)

#define S5M8767_IRQ_RTC60S_MASK		(1 << 0)
#define S5M8767_IRQ_RTCA1_MASK		(1 << 1)
#define S5M8767_IRQ_RTCA2_MASK		(1 << 2)
#define S5M8767_IRQ_SMPL_MASK		(1 << 3)
#define S5M8767_IRQ_RTC1S_MASK		(1 << 4)
#define S5M8767_IRQ_WTSR_MASK		(1 << 5)

enum s5m8763_irq {
	S5M8763_IRQ_DCINF,
	S5M8763_IRQ_DCINR,
	S5M8763_IRQ_JIGF,
	S5M8763_IRQ_JIGR,
	S5M8763_IRQ_PWRONF,
	S5M8763_IRQ_PWRONR,

	S5M8763_IRQ_WTSREVNT,
	S5M8763_IRQ_SMPLEVNT,
	S5M8763_IRQ_ALARM1,
	S5M8763_IRQ_ALARM0,

	S5M8763_IRQ_ONKEY1S,
	S5M8763_IRQ_TOPOFFR,
	S5M8763_IRQ_DCINOVPR,
	S5M8763_IRQ_CHGRSTF,
	S5M8763_IRQ_DONER,
	S5M8763_IRQ_CHGFAULT,

	S5M8763_IRQ_LOBAT1,
	S5M8763_IRQ_LOBAT2,

	S5M8763_IRQ_NR,
};

#define S5M8763_IRQ_DCINF_MASK		(1 << 2)
#define S5M8763_IRQ_DCINR_MASK		(1 << 3)
#define S5M8763_IRQ_JIGF_MASK		(1 << 4)
#define S5M8763_IRQ_JIGR_MASK		(1 << 5)
#define S5M8763_IRQ_PWRONF_MASK		(1 << 6)
#define S5M8763_IRQ_PWRONR_MASK		(1 << 7)

#define S5M8763_IRQ_WTSREVNT_MASK	(1 << 0)
#define S5M8763_IRQ_SMPLEVNT_MASK	(1 << 1)
#define S5M8763_IRQ_ALARM1_MASK		(1 << 2)
#define S5M8763_IRQ_ALARM0_MASK		(1 << 3)

#define S5M8763_IRQ_ONKEY1S_MASK	(1 << 0)
#define S5M8763_IRQ_TOPOFFR_MASK	(1 << 2)
#define S5M8763_IRQ_DCINOVPR_MASK	(1 << 3)
#define S5M8763_IRQ_CHGRSTF_MASK	(1 << 4)
#define S5M8763_IRQ_DONER_MASK		(1 << 5)
#define S5M8763_IRQ_CHGFAULT_MASK	(1 << 7)

#define S5M8763_IRQ_LOBAT1_MASK		(1 << 0)
#define S5M8763_IRQ_LOBAT2_MASK		(1 << 1)

#define S5M8763_ENRAMP                  (1 << 4)

/**
 * struct s5m87xx_dev - s5m87xx master device for sub-drivers
 * @dev: master device of the chip (can be used to access platform data)
 * @i2c: i2c client private data for regulator
 * @rtc: i2c client private data for rtc
 * @iolock: mutex for serializing io access
 * @irqlock: mutex for buslock
 * @irq_base: base IRQ number for s5m87xx, required for IRQs
 * @irq: generic IRQ number for s5m87xx
 * @ono: power onoff IRQ number for s5m87xx
 * @irq_masks_cur: currently active value
 * @irq_masks_cache: cached hardware value
 * @type: indicate which s5m87xx "variant" is used
 */
struct s5m87xx_dev {
	struct device *dev;
	struct i2c_client *i2c;
	struct i2c_client *rtc;
	struct mutex iolock;
	struct mutex irqlock;

	int device_type;
	int irq_base;
	int irq;
	int ono;
	u8 irq_masks_cur[NUM_IRQ_REGS];
	u8 irq_masks_cache[NUM_IRQ_REGS];
	int type;
	bool wakeup;
	bool wtsr_smpl;
};

int s5m_irq_init(struct s5m87xx_dev *s5m87xx);
void s5m_irq_exit(struct s5m87xx_dev *s5m87xx);
int s5m_irq_resume(struct s5m87xx_dev *s5m87xx);

extern int s5m_reg_read(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int s5m_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf);
extern int s5m_reg_write(struct i2c_client *i2c, u8 reg, u8 value);
extern int s5m_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf);
extern int s5m_reg_update(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);

struct s5m_platform_data {
	struct s5m_regulator_data	*regulators;
	struct s5m_opmode_data		*opmode_data;

	int				device_type;
	int				num_regulators;
	int				(*cfg_pmic_irq)(void);

	/* IRQ */
	int				irq_gpio;
	int				irq_base;

	int				ono;
	bool				wakeup;
	bool				buck_voltage_lock;

	int				buck_gpios[3];
	int				buck_ds[3];

	int				buck2_voltage[8];
	bool				buck2_gpiodvs;
	int				buck3_voltage[8];
	bool				buck3_gpiodvs;
	int				buck4_voltage[8];
	bool				buck4_gpiodvs;

	int				buck_set1;
	int				buck_set2;
	int				buck_set3;
	int				buck2_enable;
	int				buck3_enable;
	int				buck4_enable;
	int				buck_default_idx;
	int				buck2_default_idx;
	int				buck3_default_idx;
	int				buck4_default_idx;

	int                             buck_ramp_delay;
	bool                            buck2_ramp_enable;
	bool                            buck3_ramp_enable;
	bool                            buck4_ramp_enable;

	bool				wtsr_smpl;

	int				buck1_init;
	int				buck2_init;
	int				buck3_init;
	int				buck4_init;
};

#endif /*  __LINUX_MFD_S5M_CORE_H */
