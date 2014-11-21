/*
 *  Copyright (C) 2009 Samsung Electronics
 *
 *  based on max17040_battery.h
 *
 *  <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MAX17042_BATTERY_H_
#define __MAX17042_BATTERY_H_

/*#define NO_READ_I2C_FOR_MAXIM */
#if !defined(CONFIG_MACH_Q1_BD) && !defined(CONFIG_MACH_TRATS)
#define RECAL_SOC_FOR_MAXIM
#endif
/*#define LOG_REG_FOR_MAXIM */
#ifdef RECAL_SOC_FOR_MAXIM
#define NO_NEED_RECAL_SOC_HW_REV	0x0b	/*REV1.0 */
#endif
#ifdef LOG_REG_FOR_MAXIM
#undef RECAL_SOC_FOR_MAXIM
#endif

#define MAX17042_REG_STATUS		0x00
#define MAX17042_REG_VALRT_TH		0x01
#define MAX17042_REG_TALRT_TH		0x02
#define MAX17042_REG_SALRT_TH		0x03
#define MAX17042_REG_VCELL			0x09
#define MAX17042_REG_TEMPERATURE	0x08
#define MAX17042_REG_AVGVCELL		0x19
#define MAX17042_REG_CONFIG		0x1D
#define MAX17042_REG_VERSION		0x21
#define MAX17042_REG_LEARNCFG		0x28
#define MAX17042_REG_FILTERCFG	0x29
#define MAX17042_REG_MISCCFG		0x2B
#define MAX17042_REG_CGAIN		0x2E
#define MAX17042_REG_RCOMP		0x38
#define MAX17042_REG_VFOCV		0xFB
#define MAX17042_REG_SOC_VF		0xFF

#define MAX17042_LONG_DELAY		2000
#define MAX17042_SHORT_DELAY		0

#define MAX17042_BATTERY_FULL		95

#if defined(CONFIG_TARGET_LOCALE_KOR)
#if defined(CONFIG_MACH_U1_KOR_LGT)
#define MAX17042_NEW_RCOMP		0x0070
#else
#define MAX17042_NEW_RCOMP		0x0065
#endif
#endif

struct max17042_reg_data {
	u8 reg_addr;
	u8 reg_data1;
	u8 reg_data2;
};

struct max17042_platform_data {
	int (*battery_online)(void);
	int (*charger_online)(void);
	int (*charger_enable)(void);
	int (*low_batt_cb)(void);

	struct	max17042_reg_data *init;
	int init_size;
	struct	max17042_reg_data *alert_init;
	int alert_init_size;
	int alert_gpio;
	unsigned int alert_irq;

	bool enable_current_sense;
	bool enable_gauging_temperature;

#ifdef RECAL_SOC_FOR_MAXIM
	/*check need for re-calculation of soc */
	bool (*need_soc_recal)(void);
#endif
};

#endif
