/*
 * max17047_fuelgauge.h
 *
 * Copyright (C) 2011 Samsung Electronics
 * SangYoung Son <hello.son@samsung.com>
 *
 * based on max17042_battery.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MAX17047_BATTERY_H_
#define __MAX17047_BATTERY_H_

//#if defined (CONFIG_FUELGAUGE_MAX17047_COULOMB_COUNTING)
/* jig state */
extern bool is_jig_attached;

#define CURRENT_RANGE_MAX_NUM	5
#define TEMP_RANGE_MAX_NUM	3

enum {
	SDI = 0,
};

enum {
	RANGE = 0,
	SLOPE,
	OFFSET,
	TABLE_MAX
};


struct battery_data_t {
	u16 Capacity;
	u16 low_battery_comp_voltage;
	s32 low_battery_table[CURRENT_RANGE_MAX_NUM][TABLE_MAX];
	s32 temp_adjust_table[TEMP_RANGE_MAX_NUM][TABLE_MAX];
	u8	*type_str;
};

enum {
	FG_LEVEL = 0,
	FG_TEMPERATURE,
	FG_VOLTAGE,
	FG_CURRENT,
	FG_CURRENT_AVG,
	FG_CHECK_STATUS,
	FG_RAW_SOC,
	FG_VF_SOC,
	FG_AV_SOC,
	FG_FULLCAP,
	FG_MIXCAP,
	FG_AVCAP,
	FG_REPCAP,
};

enum {
	POSITIVE = 0,
	NEGATIVE,
};

#define LOW_BATT_COMP_RANGE_NUM 5
#define LOW_BATT_COMP_LEVEL_NUM 2
#define MAX_LOW_BATT_CHECK_CNT	10

/* FullCap learning setting */
#define VFFULLCAP_CHECK_INTERVAL	300 /* sec */
/* soc should be 0.1% unit */
#define VFSOC_FOR_FULLCAP_LEARNING	950
#define LOW_CURRENT_FOR_FULLCAP_LEARNING	20
#define HIGH_CURRENT_FOR_FULLCAP_LEARNING	120
#define LOW_AVGCURRENT_FOR_FULLCAP_LEARNING 20
#define HIGH_AVGCURRENT_FOR_FULLCAP_LEARNING	100

/* power off margin */
/* soc should be 0.1% unit */
#define POWER_OFF_SOC_HIGH_MARGIN	20
#define POWER_OFF_VOLTAGE_HIGH_MARGIN	3500
#define POWER_OFF_VOLTAGE_LOW_MARGIN	3400

/* FG recovery handler */
/* soc should be 0.1% unit */
#define STABLE_LOW_BATTERY_DIFF 30
#define STABLE_LOW_BATTERY_DIFF_LOWBATT 10
#define LOW_BATTERY_SOC_REDUCE_UNIT 10


//#endif

struct max17047_platform_data {
	int irq_gpio;

	bool enable_current_sense;
	bool enable_gauging_temperature;

	const char *psy_name;
};

#endif

