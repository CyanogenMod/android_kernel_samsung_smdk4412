/*
 * max17048_fuelgauge.h
 * Samsung MAX17048 Fuel Gauge Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#ifndef __MAX17048_FUELGAUGE_H
#define __MAX17048_FUELGAUGE_H __FILE__

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */
#define SEC_FUELGAUGE_I2C_SLAVEADDR (0x6D >> 1)

#define MAX17048_VCELL_MSB	0x02
#define MAX17048_VCELL_LSB	0x03
#define MAX17048_SOC_MSB	0x04
#define MAX17048_SOC_LSB	0x05
#define MAX17048_MODE_MSB	0x06
#define MAX17048_MODE_LSB	0x07
#define MAX17048_VER_MSB	0x08
#define MAX17048_VER_LSB	0x09
#define MAX17048_RCOMP_MSB	0x0C
#define MAX17048_RCOMP_LSB	0x0D
#define MAX17048_CMD_MSB	0xFE
#define MAX17048_CMD_LSB	0xFF

#define RCOMP0_TEMP	20

struct battery_data_t {
	u8 RCOMP0;
	u8 RCOMP_charging;
	int temp_cohot;
	int temp_cocold;
	u8 *type_str;
};

struct sec_fg_info {
	bool dummy
};

#endif /* __MAX17048_FUELGAUGE_H */
