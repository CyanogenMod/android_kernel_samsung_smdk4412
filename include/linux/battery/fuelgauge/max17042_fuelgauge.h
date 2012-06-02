/*
 * max17042_fuelgauge.h
 * Samsung MAX17042 Fuel Gauge Header
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

#ifndef __MAX17042_FUELGAUGE_H
#define __MAX17042_FUELGAUGE_H __FILE__

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */
#define SEC_FUELGAUGE_I2C_SLAVEADDR 0x36

#if defined(CONFIG_FUELGAUGE_MAX17042_VOLTAGE_TRACKING)
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

struct sec_fg_info {
	bool dummy;
};

#endif

#if defined(CONFIG_FUELGAUGE_MAX17042_COULOMB_COUNTING)
#define PRINT_COUNT	10

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

#define LOW_BATT_COMP_RANGE_NUM	5
#define LOW_BATT_COMP_LEVEL_NUM	2
#define MAX_LOW_BATT_CHECK_CNT	10

enum {
	POSITIVE = 0,
	NEGATIVE,
};

enum {
	UNKNOWN_TYPE = 0,
	SDI_BATTERY_TYPE,
	ATL_BATTERY_TYPE,
};

struct max17042_platform_data {
	int sdi_capacity;
	int sdi_vfcapacity;
	int sdi_low_bat_comp_start_vol;
	int atl_capacity;
	int atl_vfcapacity;
	int atl_low_bat_comp_start_vol;
};

struct sec_fg_info {
	/* test print count */
	int pr_cnt;
	/* battery type */
	int battery_type;
	/* capacity and vfcapacity */
	u16 capacity;
	u16 vfcapacity;
	int soc_restart_flag;
	/* full charge comp */
	struct delayed_work	full_comp_work;
	u32 previous_fullcap;
	u32 previous_vffullcap;
	u32 full_charged_cap;
	/* cap corruption check */
	u32 previous_repsoc;
	u32 previous_vfsoc;
	u32 previous_remcap;
	u32 previous_mixcap;
	u32 previous_fullcapacity;
	u32 previous_vfcapacity;
	u32 previous_vfocv;
	/* low battery comp */
	int low_batt_comp_cnt[LOW_BATT_COMP_RANGE_NUM][LOW_BATT_COMP_LEVEL_NUM];
	int low_batt_comp_flag;
	int check_start_vol;
	/* low battery boot */
	int low_batt_boot_flag;

	/* battery info */
	u32 soc;

	/* miscellaneous */
	int fg_chk_cnt;
	int fg_skip;
	int fg_skip_cnt;
	int full_check_flag;
	bool is_first_check;
};

/* Battery parameter */
#define sdi_capacity		0x3730
#define sdi_vfcapacity		0x4996
#define sdi_low_bat_comp_start_vol	3600
#define atl_capacity		0x3022
#define atl_vfcapacity		0x4024
#define atl_low_bat_comp_start_vol	3450

/* Current range for P2(not dependent on battery type */
#if defined(CONFIG_MACH_P2_REV00) || defined(CONFIG_MACH_P2_REV01) || \
	defined(CONFIG_MACH_P2_REV02)
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
#elif defined(CONFIG_MACH_P4W_REV00) || defined(CONFIG_MACH_P4W_REV01) || \
	defined(CONFIG_MACH_P11)	/* P4W battery parameter */
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
#elif defined(CONFIG_MACH_P8_REV00) || defined(CONFIG_MACH_P8_REV01) \
	|| defined(CONFIG_MACH_P8LTE_REV00) /* P8 battery parameter */
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
/* added as dummy value to fix build error */
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

/* Temperature adjust value */
#define SDI_TRIM1_1	122
#define SDI_TRIM1_2	8950
#define SDI_TRIM2_1	200
#define SDI_TRIM2_2	51000

/* FullCap learning setting */
#define VFSOC_FOR_FULLCAP_LEARNING	90
#define LOW_CURRENT_FOR_FULLCAP_LEARNING	20
#define HIGH_CURRENT_FOR_FULLCAP_LEARNING	120
#define LOW_AVGCURRENT_FOR_FULLCAP_LEARNING	20
#define HIGH_AVGCURRENT_FOR_FULLCAP_LEARNING	100

/* power off margin */
#define POWER_OFF_SOC_HIGH_MARGIN	0x200
#define POWER_OFF_VOLTAGE_HIGH_MARGIN	3500

#endif

#endif /* __MAX17042_FUELGAUGE_H */

