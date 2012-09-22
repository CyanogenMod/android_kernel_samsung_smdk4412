/*
 *  max17042_battery.h
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2010 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MAX17042_BATTERY_H
#define _MAX17042_BATTERY_H

#if defined(CONFIG_TARGET_LOCALE_KOR)
#include <linux/power_supply.h>
#endif /* CONFIG_TARGET_LOCALE_KOR */

/* Register address */
#define STATUS_REG				0x00
#define VALRT_THRESHOLD_REG	0x01
#define TALRT_THRESHOLD_REG	0x02
#define SALRT_THRESHOLD_REG	0x03
#define REMCAP_REP_REG			0x05
#define SOCREP_REG				0x06
#define TEMPERATURE_REG		0x08
#define VCELL_REG				0x09
#define CURRENT_REG				0x0A
#define AVG_CURRENT_REG		0x0B
#define SOCMIX_REG				0x0D
#define SOCAV_REG				0x0E
#define REMCAP_MIX_REG			0x0F
#define FULLCAP_REG				0x10
#define RFAST_REG				0x15
#define AVR_TEMPERATURE_REG	0x16
#define CYCLES_REG				0x17
#define DESIGNCAP_REG			0x18
#define AVR_VCELL_REG			0x19
#define CONFIG_REG				0x1D
#define REMCAP_AV_REG			0x1F
#define FULLCAP_NOM_REG		0x23
#define MISCCFG_REG				0x2B
#if defined(CONFIG_TARGET_LOCALE_KOR)
#define FILTERCFG_REG			0x29
#define CGAIN_REG				0x2E
#endif /* CONFIG_TARGET_LOCALE_KOR */
#define RCOMP_REG				0x38
#define FSTAT_REG				0x3D
#define DQACC_REG				0x45
#define DPACC_REG				0x46
#define OCV_REG					0xEE
#define VFOCV_REG				0xFB
#define VFSOC_REG				0xFF

#define FG_LEVEL 0
#define FG_TEMPERATURE 1
#define FG_VOLTAGE 2
#define FG_CURRENT 3
#define FG_CURRENT_AVG 4
#define FG_BATTERY_TYPE 5
#define FG_CHECK_STATUS 6
#define FG_VF_SOC 7
#define FG_VOLTAGE_NOW 8

#define LOW_BATT_COMP_RANGE_NUM	5
#define LOW_BATT_COMP_LEVEL_NUM	2
#define MAX_LOW_BATT_CHECK_CNT	10
#define MAX17042_CURRENT_UNIT	15625 / 100000

struct max17042_platform_data {
	int sdi_capacity;
	int sdi_vfcapacity;
	int sdi_low_bat_comp_start_vol;
	int atl_capacity;
	int atl_vfcapacity;
	int atl_low_bat_comp_start_vol;
	int byd_capacity;
	int byd_vfcapacity;
	int byd_low_bat_comp_start_vol;
	int fuel_alert_line;

	int (*check_jig_status) (void);
};

struct fuelgauge_info {
	/* test print count */
	int pr_cnt;
	/* battery type */
	int battery_type;
	/* full charge comp */
	u32 prev_fullcap;
	u32 prev_vffcap;
	u32 full_charged_cap;
	/* capacity and vfcapacity */
	u16 capacity;
	u16 vfcapacity;
	int soc_restart_flag;
	/* cap corruption check */
	u32 prev_repsoc;
	u32 prev_vfsoc;
	u32 prev_remcap;
	u32 prev_mixcap;
	u32 prev_fullcapacity;
	u32 prev_vfcapacity;
	u32 prev_vfocv;
	/* low battery comp */
	int low_batt_comp_cnt[LOW_BATT_COMP_RANGE_NUM][LOW_BATT_COMP_LEVEL_NUM];
	int check_start_vol;
	int low_batt_comp_flag;
	int psoc;
};

struct max17042_chip {
	struct i2c_client		*client;
	struct max17042_platform_data	*pdata;
#if defined(CONFIG_TARGET_LOCALE_KOR)
	struct power_supply battery;
#endif /* CONFIG_TARGET_LOCALE_KOR */
	struct fuelgauge_info	info;
	struct mutex			fg_lock;
};

/* Battery parameter */
/* Current range for P2(not dependent on battery type */
#if defined(CONFIG_MACH_P2)
#define CURRENT_RANGE1	0
#define CURRENT_RANGE2	-100
#define CURRENT_RANGE3	-750
#define CURRENT_RANGE4	-1250
#define CURRENT_RANGE_MAX	CURRENT_RANGE4
#define CURRENT_RANGE_MAX_NUM	4
/* SDI type low battery compensation offset */
#define SDI_Range4_1_Offset		3320
#define SDI_Range4_3_Offset		3410
#define SDI_Range3_1_Offset		3451
#define SDI_Range3_3_Offset		3454
#define SDI_Range2_1_Offset		3461
#define SDI_Range2_3_Offset		3544
#define SDI_Range1_1_Offset		3456
#define SDI_Range1_3_Offset		3536
#define SDI_Range4_1_Slope		0
#define SDI_Range4_3_Slope		0
#define SDI_Range3_1_Slope		97
#define SDI_Range3_3_Slope		27
#define SDI_Range2_1_Slope		96
#define SDI_Range2_3_Slope		134
#define SDI_Range1_1_Slope		0
#define SDI_Range1_3_Slope		0
/* ATL type low battery compensation offset */
#define ATL_Range5_1_Offset		3277
#define ATL_Range5_3_Offset		3293
#define ATL_Range4_1_Offset		3312
#define ATL_Range4_3_Offset		3305
#define ATL_Range3_1_Offset		3310
#define ATL_Range3_3_Offset		3333
#define ATL_Range2_1_Offset		3335
#define ATL_Range2_3_Offset		3356
#define ATL_Range1_1_Offset		3325
#define ATL_Range1_3_Offset		3342
#define ATL_Range5_1_Slope		0
#define ATL_Range5_3_Slope		0
#define ATL_Range4_1_Slope		30
#define ATL_Range4_3_Slope		667
#define ATL_Range3_1_Slope		20
#define ATL_Range3_3_Slope		40
#define ATL_Range2_1_Slope		60
#define ATL_Range2_3_Slope		76
#define ATL_Range1_1_Slope		0
#define ATL_Range1_3_Slope		0
#elif defined(CONFIG_MACH_P4NOTE)	/* P4W battery parameter */
/* Current range for P4W(not dependent on battery type */
#define CURRENT_RANGE1	0
#define CURRENT_RANGE2	-200
#define CURRENT_RANGE3	-600
#define CURRENT_RANGE4	-1500
#define CURRENT_RANGE5	-2500
#define CURRENT_RANGE_MAX	CURRENT_RANGE5
#define CURRENT_RANGE_MAX_NUM	5
/* SDI type low battery compensation offset */
#define SDI_Range5_1_Offset		3318
#define SDI_Range5_3_Offset		3383
#define SDI_Range4_1_Offset		3451
#define SDI_Range4_3_Offset		3618
#define SDI_Range3_1_Offset		3453
#define SDI_Range3_3_Offset		3615
#define SDI_Range2_1_Offset		3447
#define SDI_Range2_3_Offset		3606
#define SDI_Range1_1_Offset		3438
#define SDI_Range1_3_Offset		3591
#define SDI_Range5_1_Slope		0
#define SDI_Range5_3_Slope		0
#define SDI_Range4_1_Slope		53
#define SDI_Range4_3_Slope		94
#define SDI_Range3_1_Slope		54
#define SDI_Range3_3_Slope		92
#define SDI_Range2_1_Slope		45
#define SDI_Range2_3_Slope		78
#define SDI_Range1_1_Slope		0
#define SDI_Range1_3_Slope		0
/* Default value for build */
/* ATL type low battery compensation offset */
#define ATL_Range4_1_Offset		3298
#define ATL_Range4_3_Offset		3330
#define ATL_Range3_1_Offset		3375
#define ATL_Range3_3_Offset		3445
#define ATL_Range2_1_Offset		3371
#define ATL_Range2_3_Offset		3466
#define ATL_Range1_1_Offset		3362
#define ATL_Range1_3_Offset		3443
#define ATL_Range4_1_Slope		0
#define ATL_Range4_3_Slope		0
#define ATL_Range3_1_Slope		50
#define ATL_Range3_3_Slope		77
#define ATL_Range2_1_Slope		40
#define ATL_Range2_3_Slope		111
#define ATL_Range1_1_Slope		0
#define ATL_Range1_3_Slope		0
/* BYD type low battery compensation offset */
#define BYD_Range5_1_Offset		3318
#define BYD_Range5_3_Offset		3383
#define BYD_Range4_1_Offset		3451
#define BYD_Range4_3_Offset		3618
#define BYD_Range3_1_Offset		3453
#define BYD_Range3_3_Offset		3615
#define BYD_Range2_1_Offset		3447
#define BYD_Range2_3_Offset		3606
#define BYD_Range1_1_Offset		3438
#define BYD_Range1_3_Offset		3591
#define BYD_Range5_1_Slope		0
#define BYD_Range5_3_Slope		0
#define BYD_Range4_1_Slope		53
#define BYD_Range4_3_Slope		94
#define BYD_Range3_1_Slope		54
#define BYD_Range3_3_Slope		92
#define BYD_Range2_1_Slope		45
#define BYD_Range2_3_Slope		78
#define BYD_Range1_1_Slope		0
#define BYD_Range1_3_Slope		0
#elif defined(CONFIG_MACH_P8) || defined(CONFIG_MACH_P8LTE)
/* Current range for P8(not dependent on battery type */
#define CURRENT_RANGE1	0
#define CURRENT_RANGE2	-200
#define CURRENT_RANGE3	-600
#define CURRENT_RANGE4	-1500
#define CURRENT_RANGE5	-2500
#define CURRENT_RANGE_MAX	CURRENT_RANGE5
#define CURRENT_RANGE_MAX_NUM	5
/* SDI type low battery compensation Slope & Offset for 1% SOC range*/
#define SDI_Range1_1_Slope		0
#define SDI_Range2_1_Slope		54
#define SDI_Range3_1_Slope		66
#define SDI_Range4_1_Slope		69
#define SDI_Range5_1_Slope		0

#define SDI_Range1_1_Offset		3391
#define SDI_Range2_1_Offset		3402
#define SDI_Range3_1_Offset		3409
#define SDI_Range4_1_Offset		3414
#define SDI_Range5_1_Offset		3240

/* SDI type low battery compensation Slope & Offset for 3% SOC range*/
#define SDI_Range1_3_Slope		0
#define SDI_Range2_3_Slope		92
#define SDI_Range3_3_Slope		125
#define SDI_Range4_3_Slope		110
#define SDI_Range5_3_Slope		0

#define SDI_Range1_3_Offset		3524
#define SDI_Range2_3_Offset		3542
#define SDI_Range3_3_Offset		3562
#define SDI_Range4_3_Offset		3539
#define SDI_Range5_3_Offset		3265

/* ATL type low battery compensation offset */
#define ATL_Range4_1_Offset		3298
#define ATL_Range4_3_Offset		3330
#define ATL_Range3_1_Offset		3375
#define ATL_Range3_3_Offset		3445
#define ATL_Range2_1_Offset		3371
#define ATL_Range2_3_Offset		3466
#define ATL_Range1_1_Offset		3362
#define ATL_Range1_3_Offset		3443

#define ATL_Range4_1_Slope		0
#define ATL_Range4_3_Slope		0
#define ATL_Range3_1_Slope		50
#define ATL_Range3_3_Slope		77
#define ATL_Range2_1_Slope		40
#define ATL_Range2_3_Slope		111
#define ATL_Range1_1_Slope		0
#define ATL_Range1_3_Slope		0
#else	/* default value */
/* Current range for default(not dependent on battery type */
#define CURRENT_RANGE1	0
#define CURRENT_RANGE2	-100
#define CURRENT_RANGE3	-750
#define CURRENT_RANGE4	-1250
#define CURRENT_RANGE_MIN	CURRENT_RANGE1
#define CURRENT_RANGE_MAX	CURRENT_RANGE4
#define CURRENT_RANGE_MAX_NUM	4
/* SDI type low battery compensation offset */
#define SDI_Range4_1_Offset		3371
#define SDI_Range4_3_Offset		3478
#define SDI_Range3_1_Offset		3453
#define SDI_Range3_3_Offset		3614
#define SDI_Range2_1_Offset		3447
#define SDI_Range2_3_Offset		3606
#define SDI_Range1_1_Offset		3438
#define SDI_Range1_3_Offset		3591
#define SDI_Range4_1_Slope		0
#define SDI_Range4_3_Slope		0
#define SDI_Range3_1_Slope		50
#define SDI_Range3_3_Slope		90
#define SDI_Range2_1_Slope		50
#define SDI_Range2_3_Slope		78
#define SDI_Range1_1_Slope		0
#define SDI_Range1_3_Slope		0
/* ATL type low battery compensation offset */
#define ATL_Range4_1_Offset		3298
#define ATL_Range4_3_Offset		3330
#define ATL_Range3_1_Offset		3375
#define ATL_Range3_3_Offset		3445
#define ATL_Range2_1_Offset		3371
#define ATL_Range2_3_Offset		3466
#define ATL_Range1_1_Offset		3362
#define ATL_Range1_3_Offset		3443
#define ATL_Range4_1_Slope		0
#define ATL_Range4_3_Slope		0
#define ATL_Range3_1_Slope		50
#define ATL_Range3_3_Slope		77
#define ATL_Range2_1_Slope		40
#define ATL_Range2_3_Slope		111
#define ATL_Range1_1_Slope		0
#define ATL_Range1_3_Slope		0
#endif

enum {
	POSITIVE = 0,
	NEGATIVE,
};

enum {
	UNKNOWN_TYPE = 0,
	SDI_BATTERY_TYPE,
	ATL_BATTERY_TYPE,
	BYD_BATTERY_TYPE,
};

#ifdef CONFIG_MACH_P8LTE
#define SEC_CURR_MEA_ADC_CH  6
/*N.B. For a given battery type and aboard type both R_ISET and I_topOff
are constant hence VtopOff = R_ISET * I_topOff should also be a constant
under the given condns.
Presently only implementing for P8-LTE, for other, consult with HW.
For P8-LTE the limit is defined as 0.05C , where C is the battery capacity
,5100mAh in this case.*/
#define COUNT_TOP_OFF   3
#define V_TOP_OFF   165 /* 200mA * 750ohms.(11_11_16) */
/* #define V_TOP_OFF   208 *//* 250mA(default value) * 750ohms.(11_10_10) */
#endif

/* Temperature adjust value */
#define SDI_TRIM1_1	122
#define SDI_TRIM1_2	8950
#define SDI_TRIM2_1	200
#define SDI_TRIM2_2	51000

void fg_periodic_read(void);

extern int fg_reset_soc(void);
extern int fg_reset_capacity(void);
extern int fg_adjust_capacity(void);
extern void fg_low_batt_compensation(u32 level);
extern int fg_alert_init(void);
extern void fg_fullcharged_compensation(u32 is_recharging, u32 pre_update);
extern void fg_check_vf_fullcap_range(void);
extern int fg_check_cap_corruption(void);
extern void fg_set_full_charged(void);
extern void fg_reset_fullcap_in_fullcharge(void);
#endif
