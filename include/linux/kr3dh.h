/*
 *  kr3dh.c - ST Microelectronics three-axes accelerometer
 *
 *  Copyright (C) 2010 Samsung Electronics
 *  Donggeun Kim <dg77.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _KR3DH_H_
#define _KR3DH_H_

enum kr3dh_ouput_data_rate {
	KR3DH_ODR_50HZ,
	KR3DH_ODR_100HZ,
	KR3DH_ODR_400HZ,
	KR3DH_ODR_1000HZ,
};

enum kr3dh_power_mode {
	KR3DH_POWER_DOWN,
	KR3DH_NORMAL_MODE,
	KR3DH_LOW_POWER_ONE_HALF_HZ,
	KR3DH_LOW_POWER_1HZ,
	KR3DH_LOW_POWER_2HZ,
	KR3DH_LOW_POWER_5HZ,
	KR3DH_LOW_POWER_10HZ,
};

enum kr3dh_int_hl_active {
	KR3DH_HIGH,
	KR3DH_LOW,
};

enum kr3dh_int_pp_od {
	KR3DH_PUSH_PULL,
	KR3DH_OPEN_DRAIN,
};

enum kr3dh_int_cfg {
	KR3DH_INT_SOURCE,
	KR3DH_INT_SOURCE_OR,
	KR3DH_DATA_READY,
	KR3DH_BOOT_RUNNING,
};

enum kr3dh_spi_mode {
	KR3DH_FOUR_WIRE,
	KR3DH_THREE_WIRE,
};

enum kr3dh_scale_range {
	KR3DH_RANGE_2G,
	KR3DH_RANGE_4G,
	KR3DH_RANGE_8G = 3,
};

enum kr3dh_turn_on_mode {
	KR3DH_SLEEP_TO_WAKE_DISABLE,
	KR3DH_SLEEP_TO_WAKE_ENABLE = 3,
};

enum kr3dh_int_combination {
	KR3DH_OR_COMBINATION,
	KR3DH_AND_COMBINATION,
};

struct kr3dh_platform_data {
	int irq2;
	int negate_x;
	int negate_y;
	int negate_z;
	int change_xy;
	u8 power_mode;
	u8 data_rate;
	u8 zen;
	u8 yen;
	u8 xen;
	u8 reboot;
	u8 hpmode;
	u8 filter_sel;
	u8 hp_enable_1;
	u8 hp_enable_2;
	u8 hpcf;
	u8 int_hl_active;
	u8 int_pp_od;
	u8 int2_latch;
	u8 int2_cfg;
	u8 int1_latch;
	u8 int1_cfg;
	u8 block_data_update;
	u8 endian;
	u8 fullscale;
	u8 selftest_sign;
	u8 selftest;
	u8 spi_mode;
	u8 turn_on_mode;
	u8 int1_combination;
	u8 int1_6d_enable;
	u8 int1_z_high_enable;
	u8 int1_z_low_enable;
	u8 int1_y_high_enable;
	u8 int1_y_low_enable;
	u8 int1_x_high_enable;
	u8 int1_x_low_enable;
	u8 int1_threshold;
	u8 int1_duration;
	u8 int2_combination;
	u8 int2_6d_enable;
	u8 int2_z_high_enable;
	u8 int2_z_low_enable;
	u8 int2_y_high_enable;
	u8 int2_y_low_enable;
	u8 int2_x_high_enable;
	u8 int2_x_low_enable;
	u8 int2_threshold;
	u8 int2_duration;
};

#endif
