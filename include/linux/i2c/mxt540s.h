/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifndef __MXT_H__
#define __MXT_H__

#define MXT_DEV_NAME "Atmel MXT540S"

#define MXT_540S_SW_RESET_TIME		300	/* msec */
#define MXT_540S_HW_RESET_TIME		300	/* msec */
#define MXT_540S_FW_RESET_TIME		500	/* msec */

enum { RESERVED_T0 = 0,
	RESERVED_T1,
	DEBUG_DELTAS_T2,
	DEBUG_REFERENCES_T3,
	DEBUG_SIGNALS_T4,
	GEN_MESSAGEPROCESSOR_T5,
	GEN_COMMANDPROCESSOR_T6,
	GEN_POWERCONFIG_T7,
	GEN_ACQUISITIONCONFIG_T8,
	TOUCH_MULTITOUCHSCREEN_T9,
	TOUCH_SINGLETOUCHSCREEN_T10,
	TOUCH_XSLIDER_T11,
	TOUCH_YSLIDER_T12,
	TOUCH_XWHEEL_T13,
	TOUCH_YWHEEL_T14,
	TOUCH_KEYARRAY_T15,
	PROCG_SIGNALFILTER_T16,
	PROCI_LINEARIZATIONTABLE_T17,
	SPT_COMCONFIG_T18,
	SPT_GPIOPWM_T19,
	PROCI_GRIPFACESUPPRESSION_T20,
	RESERVED_T21,
	PROCG_NOISESUPPRESSION_T22,
	TOUCH_PROXIMITY_T23,
	PROCI_ONETOUCHGESTUREPROCESSOR_T24,
	SPT_SELFTEST_T25,
	DEBUG_CTERANGE_T26,
	PROCI_TWOTOUCHGESTUREPROCESSOR_T27,
	SPT_CTECONFIG_T28,
	SPT_GPI_T29,
	SPT_GATE_T30,
	TOUCH_KEYSET_T31,
	TOUCH_XSLIDERSET_T32,
	RESERVED_T33,
	GEN_MESSAGEBLOCK_T34,
	SPT_GENERICDATA_T35,
	RESERVED_T36,
	DEBUG_DIAGNOSTIC_T37,
	SPT_USERDATA_T38,
	SPARE_T39,
	PROCI_GRIPSUPPRESSION_T40,
	SPARE_T41,
	PROCI_TOUCHSUPPRESSION_T42,
	SPT_DIGITIZER_T43,
	SPARE_T44,
	SPARE_T45,
	SPT_CTECONFIG_T46,
	PROCI_STYLUS_T47,
	PROCG_NOISESUPPRESSION_T48,
	SPARE_T49,
	SPARE_T50,
	SPARE_T51,
	TOUCH_PROXIMITY_KEY_T52,
	GEN_DATASOURCE_T53,
	SPARE_T54,
	ADAPTIVE_T55,
	PROCI_SHIELDLESS_T56,
	PROCI_EXTRATOUCHSCREENDATA_T57,
	SPARE_T58,
	SPARE_T59,
	SPARE_T60,
	SPT_TIMER_T61,
	PROCG_NOISESUPPRESSION_T62,
	PROCI_ACTIVESTYLUS_T63,
	RESERVED_T255 = 255,
};

struct mxt_callbacks {
	void (*inform_charger)(struct mxt_callbacks *, bool);
};

struct mxt_platform_data {
	int max_finger_touches;
	int gpio_read_done;
	const u8 **config;
	int min_x;
	int max_x;
	int min_y;
	int max_y;
	int min_z;
	int max_z;
	int min_w;
	int max_w;
	int (*power_on) (void);
	int (*power_off) (void);
	int (*power_reset) (void);
	void (*register_cb) (void *);
	u8 boot_address;
	const char *firmware_name;
};

#endif				/*  */
