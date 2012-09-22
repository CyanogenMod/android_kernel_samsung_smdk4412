/*
 *  Copyright (C) 2010, Samsung Electronics Co. Ltd. All Rights Reserved.
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

#ifndef __MXT224_H__
#define __MXT224_H__

//#define MXT224_DEV_NAME "Atmel MXT224"

#define MXT224_DEV_NAME "qt602240_ts"


#define MXT224_THRESHOLD 40
#define MXT224E_THRESHOLD		50
#define MXT224_MAX_MT_FINGERS	10

#define MXT_OREINT				10
#define MXT224_ORIENT_SWAP_NN	1
#define MXT224_ORIENT_SWAP_XY	7



#ifdef CONFIG_TOUCHSCREEN_MXT768E
enum {
	RESERVED_T0 = 0,
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
	RESERVED_T19,
	RESERVED_T20,
	RESERVED_T21,
	RESERVED_T22,
	RESERVED_T23,
	PROCI_ONETOUCHGESTUREPROCESSOR_T24,
	SPT_SELFTEST_T25,
	DEBUG_CTERANGE_T26,
	PROCI_TWOTOUCHGESTUREPROCESSOR_T27,
	RESERVED_T28,
	SPT_GPI_T29,
	SPT_GATE_T30,
	TOUCH_KEYSET_T31,
	TOUCH_XSLIDERSET_T32,
	RESERVED_T33,
	GEN_MESSAGEBLOCK_T34,
	SPT_GENERICDATA_T35,
	RESERVED_T36,
	DEBUG_DIAGNOSTIC_T37,
	SPARE_T38,
	SPARE_T39,
	PROCI_GRIPSUPPRESSION_T40,
	SPARE_T41,
	PROCI_TOUCHSUPPRESSION_T42,
	SPT_DIGITIZER_T43,
	SPARE_T44,
	SPARE_T45,
	SPT_CTECONFIG_T46,
	SPARE_T47,
	PROCG_NOISESUPPRESSION_T48,
	SPARE_T49,
	SPARE_T50,
	RESERVED_T255 = 255,
};
#else
enum {
	RESERVED_T0 = 0,
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
	SPARE_T43,
	SPARE_T44,
	SPARE_T45,
	SPT_CTECONFIG_T46,
	PROCI_STYLUS_T47,
	PROCG_NOISESUPPRESSION_T48,
	SPARE_T49,
	SPARE_T50,
	RESERVED_T255 = 255,
};
#endif

struct mxt224_platform_data {
	int max_finger_touches;
	const u8 **config;
	const u8 **config_e;	
	const u8 *t46_ta_cfg;	
	const u8 *t48_ta_cfg;	
	int gpio_read_done;
	int min_x;
	int max_x;
	int min_y;
	int max_y;
	int min_z;
	int max_z;
	int min_w;
	int max_w;
	void (*power_on)(void);
	void (*power_off)(void);
	void (*register_cb)(void*);
	void (*read_ta_status)(void*);	
//	void (*orient_barnch)(int orient_swap);/*TSP TA noise 201107251133*/
};


typedef enum
{
    QT_PAGE_UP         = 0x01,
    QT_PAGE_DOWN       = 0x02,
    QT_DELTA_MODE      = 0x10,
    QT_REFERENCE_MODE  = 0x11,
    QT_CTE_MODE        = 0x31
}diagnostic_debug_command;


//110927 gumi noise
typedef struct
{
 bool median_on_flag;
 bool mferr_setting;
 uint8_t mferr_count;
 uint8_t t46_actvsyncsperx_for_mferr;
 uint8_t t48_mfinvlddiffthr_for_mferr;
 uint8_t t48_mferrorthr_for_mferr;
 uint8_t t48_thr_for_mferr;
 uint8_t t48_movfilter_for_mferr;
}__packed t48_median_config_t;



#endif
