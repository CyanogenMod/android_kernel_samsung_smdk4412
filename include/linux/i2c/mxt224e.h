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

#define MXT224E_DEV_NAME "atmel mxt224e"

#define MXT224_MAX_MT_FINGERS		10

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

struct mxt224_platform_data {
	int max_finger_touches;
	const u8 **config;
	const u8 **config_e;
	const u8 *t48_config_batt_e;
	const u8 *t48_config_chrg_e;
	const char *firmware_name;
	int gpio_read_done;
	unsigned char num_xnode;
	unsigned char num_ynode;
	int min_x;
	int max_x;
	int min_y;
	int max_y;
	int min_z;
	int max_z;
	int min_w;
	int max_w;
	u8 atchcalst_e;
	u8 atchcalsthr_e;
	u8 tchthr_batt_e;
	u8 tchthr_charging_e;
	u8 calcfg_batt_e;
	u8 calcfg_charging_e;
	u8 atchfrccalthr_e;
	u8 atchfrccalratio_e;
	u8 chrgtime_batt_e;
	u8 chrgtime_charging_e;
	u8 blen_batt_e;
	u8 blen_charging_e;
	u8 movfilter_batt_e;
	u8 movfilter_charging_e;
	u8 actvsyncsperx_e;
	u8 nexttchdi_e;
	int (*power_on) (void);
	int (*power_off) (void);
	void (*register_cb) (void *);
	void (*read_ta_status) (void *);
};

#define TSP_CMD_STR_LEN		32
#define TSP_CMD_RESULT_STR_LEN	512
#define TSP_CMD_PARAM_NUM	8

struct mxt_fac_data {
	struct device *fac_dev_ts;
	struct list_head cmd_list_head;
	u8 cmd_state;
	char cmd[TSP_CMD_STR_LEN];
	int cmd_param[TSP_CMD_PARAM_NUM];
	char cmd_result[TSP_CMD_RESULT_STR_LEN];
	struct mutex cmd_lock;
	bool cmd_is_running;

	u8 num_xnode;
	u8 num_ynode;
	u16 num_nodes;
	u16 *reference;
	s16 *delta;

	u32 ref_max_data;
	u32 ref_min_data;
	s16 delta_max_data;
	u16 delta_max_node;

	u8 fw_ver;
	u8 build_ver;
};

enum {
	ERR_RTN_CONDITION_T9,
	ERR_RTN_CONDITION_T48,
	ERR_RTN_CONDITION_IDLE
};

#define ERR_RTN_CONDITION_MAX	(ERR_RTN_CONDITION_IDLE + 1)

struct t48_median_config_t {
	bool median_on_flag;
	bool mferr_setting;
	u8 mferr_count;
	u8 t46_actvsyncsperx_for_mferr;
	u8 t48_mfinvlddiffthr_for_mferr;
	u8 t48_mferrorthr_for_mferr;
	u8 t48_thr_for_mferr;
	u8 t48_movfilter_for_mferr;
};

int get_tsp_status(void);
extern struct class *sec_class;
#endif
