/*
 * Copyright (C) 2010 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_SAMSUNG_BATTERY_H
#define __MACH_SAMSUNG_BATTERY_H __FILE__

#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/android_alarm.h>
#if defined(CONFIG_S3C_ADC)
#include <plat/adc.h>
#endif
#if defined(CONFIG_STMPE811_ADC)
#include <linux/stmpe811-adc.h>
#endif


/* macro */
#define MAX(x, y)	((x) > (y) ? (x) : (y))
#define MIN(x, y)	((x) < (y) ? (x) : (y))
#define ABS(x)		((x) < 0 ? (-1 * (x)) : (x))
#define INRANGE(val, x, y)	(((x <= val) && (val <= y)) ||	\
				 ((y <= val) && (val <= x)) ? 1 : 0)


/* common */
enum {
	DISABLE = 0,
	ENABLE,
};

struct battery_info {
	struct device				*dev;
	struct samsung_battery_platform_data	*pdata;
#if defined(CONFIG_S3C_ADC)
	struct s3c_adc_client			*adc_client;
#endif

	bool use_sub_charger;
	char *fuelgauge_name;
	char *charger_name;
	char *sub_charger_name;

	/* android common power supply */
	struct power_supply psy_bat;
	struct power_supply psy_usb;
	struct power_supply psy_ac;

	/* charger, fuelgauge psy depends on machine */
	struct power_supply *psy_charger;
	struct power_supply *psy_fuelgauge;
	struct power_supply *psy_sub_charger;

	/* workqueue */
	struct work_struct monitor_work;
	unsigned int	monitor_mode;
	unsigned int	monitor_interval;
	int				monitor_weight;
	unsigned int	monitor_count;
	struct work_struct error_work;

	/* mutex */
	struct mutex mon_lock;
	struct mutex ops_lock;
	struct mutex err_lock;

	/* wakelock */
	struct wake_lock charge_wake_lock;
	struct wake_lock monitor_wake_lock;
	struct wake_lock emer_wake_lock;

	/* is_suspended */
	bool	is_suspended;

	/* charge state */
	unsigned int charge_real_state;
	unsigned int charge_virt_state;
	unsigned int charge_type;
	unsigned int charge_current;
	int charge_current_avg;
	unsigned int input_current;

	/* battery state */
	unsigned int battery_health;
	unsigned int battery_present;
	unsigned int battery_vcell;
	unsigned int battery_vfocv;
	int battery_v_diff;
	unsigned int battery_soc;
	unsigned int battery_raw_soc;
	int battery_r_s_delta;
	int battery_full_soc;
	int battery_vf_adc;

	/* temperature */
	int battery_temper;
	int battery_temper_adc;
	int battery_t_delta;
	int battery_temper_avg;
	int battery_temper_adc_avg;

	/* cable type */
	unsigned int cable_type;
	unsigned int cable_sub_type;
	unsigned int cable_pwr_type;
	int online_prop;

	/* For SAMSUNG charge spec */
	unsigned int vf_state;
	unsigned int temper_state;
	unsigned int overheated_state;
	unsigned int freezed_state;
	unsigned int full_charged_state;
	unsigned int abstimer_state;
	unsigned int abstimer_active;
	unsigned int recharge_phase;
	unsigned int recharge_start;
	unsigned int health_state;

	/* SIOP */
	unsigned int siop_state;
	unsigned int siop_charge_current;
	unsigned int siop_lv;

	/* etc... */
	unsigned int lpm_state;
	unsigned int led_state;
	unsigned int slate_mode;

	/* ambiguous state */
	unsigned int ambiguous_state;

	/* event sceanario */
	unsigned int event_state;
	unsigned int event_type;

	/* time management */
	unsigned int charge_start_time;
	struct timespec current_time;
	struct alarm	monitor_alarm;
	struct alarm	event_alarm;
	bool		slow_poll;
	ktime_t		last_poll;

	/* irq */
	int batdet_gpio;
	int batdet_irq;
	bool batdet_irq_st;

	/* adc power */
	bool adc_pwr_st;

	struct proc_dir_entry *entry;

	/* For debugging */
	unsigned int battery_test_mode;
	unsigned int battery_error_test;

	/* factory mode */
	bool factory_mode;

#if defined(CONFIG_TARGET_LOCALE_KOR)
	/* error test charging off mode */
	bool errortest_stopcharging;
#endif

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	bool is_unspec_phase;
	bool is_unspec_recovery;
	unsigned int prev_cable_type;
	unsigned int prev_battery_health;
	unsigned int prev_charge_virt_state;
	unsigned int prev_battery_soc;
	struct wake_lock update_wake_lock;
#endif
};

/* jig state */
extern bool is_jig_attached;

/* charger detect source */
#if defined(CONFIG_MACH_C1_KOR_SKT) || \
	defined(CONFIG_MACH_C1_KOR_KT) || defined(CONFIG_MACH_C1_KOR_LGT) || \
	defined(CONFIG_MACH_BAFFIN)
#undef USE_CHGIN_INTR
#else
#define USE_CHGIN_INTR
#endif

/* extended online type */
#if defined(CONFIG_MACH_T0)
#define EXTENDED_ONLINE_TYPE
#else
#undef EXTENDED_ONLINE_TYPE
#endif

enum online_property {
	ONLINE_PROP_UNKNOWN = 0,
	ONLINE_PROP_AC,
	ONLINE_PROP_USB,
};

/* use 2step charge termination */
#if defined(CONFIG_MACH_T0)
#define USE_2STEP_TERM
#else
#undef USE_2STEP_TERM
#endif

/*
 * Use for charger
 */
enum charger_state {
	CHARGER_STATE_ENABLE	= 0,
	CHARGER_STATE_DISABLE	= 1
};

enum current_type {
	CURRENT_TYPE_CHRAGE	= 0,
	CURRENT_TYPE_INPUT	= 1,
};

/*
 * Use for fuelgauge
 */
enum voltage_type {
	VOLTAGE_TYPE_VCELL	= 0,
	VOLTAGE_TYPE_VFOCV	= 1,
};

enum soc_type {
	SOC_TYPE_ADJUSTED	= 0,
	SOC_TYPE_RAW		= 1,
	SOC_TYPE_FULL		= 2,
};

/*
 * Use for battery
 */

enum status_full_type {
	STATUS_NOT_FULL		= 0,
	STATUS_1ST_FULL,
	STATUS_2ND_FULL,
};

#define OFF_CURR	0	/* charger off current */
#define KEEP_CURR	-1	/* keep previous current */

/* VF error check */
#define VF_CHECK_COUNT		10
#define VF_CHECK_DELAY		1000
#define RESET_SOC_DIFF_TH	100000

/* average count */
#define CNT_VOLTAGE_AVG	5
#define CNT_CURRENT_AVG	5
#define CNT_TEMPER_AVG	5
#define CNT_ADC_SAMPLE	6

/* adc error retry */
#define ADC_ERR_CNT	5
#define ADC_ERR_DELAY	200

/* WORKAROUND: define audio dock current */
#define DOCK_TYPE_AUDIO_CURR		1000
#if defined(CONFIG_MACH_T0)
#define DOCK_TYPE_SMART_NOTG_CURR	1700
#else
#define DOCK_TYPE_SMART_NOTG_CURR	1000
#endif
#define DOCK_TYPE_SMART_OTG_CURR	1000
#define DOCK_TYPE_LOW_CURR		475

/* voltage diff for recharge voltage calculation */
#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
/* KOR model spec : max-voltage minus 60mV */
#define RECHG_DROP_VALUE	60000
#else
#define RECHG_DROP_VALUE	50000	/* 4300mV */
#endif

/* power off condition, low %duV than VOLTAGE_MIN_DESIGN & SOC 0% */
#define PWROFF_MARGIN		100000
#define PWROFF_SOC		0

enum {
	CHARGE_DISABLE = 0,
	CHARGE_ENABLE,
};

enum {
	JIG_OFF = 0,
	JIG_ON,
};

/* cable detect source */
enum {
	CABLE_DET_MUIC = 0,
	CABLE_DET_ADC,
	CABLE_DET_CHARGER,

	CABLE_DET_UNKNOWN,
};


/* temperature source */
enum {
	TEMPER_FUELGAUGE = 0,
	TEMPER_AP_ADC,
	TEMPER_EXT_ADC,

	TEMPER_UNKNOWN,
};

/* vf detect source */
enum {
	VF_DET_ADC = 0,
	VF_DET_CHARGER,
	VF_DET_GPIO,

	VF_DET_UNKNOWN,
};


/* siop state */
enum {
	SIOP_DEACTIVE = 0,
	SIOP_ACTIVE,
};

enum siop_level {
	SIOP_LV0 = 0,
	SIOP_LV1,
	SIOP_LV2,
	SIOP_LV3,
};

/* monitoring mode */
enum {
	MONITOR_CHNG = 0,
	MONITOR_CHNG_SUSP,
	MONITOR_NORM,
	MONITOR_NORM_SUSP,
	MONITOR_EMER_LV1,
	MONITOR_EMER_LV2,
};

/* Temperature from adc */
#if defined(CONFIG_STMPE811_ADC)
#define BATTERY_TEMPER_CH 7
u16 stmpe811_get_adc_data(u8 channel);
int stmpe811_get_adc_value(u8 channel);
#endif

/* LED control */
enum led_state {
	BATT_LED_CHARGING = 0,
	BATT_LED_DISCHARGING,
	BATT_LED_NOT_CHARGING,
	BATT_LED_FULL,
};

enum led_color {
	BATT_LED_RED = 0,
	BATT_LED_GREEN,
	BATT_LED_BLUE,
};

enum led_pattern {
	BATT_LED_PATT_OFF = 0,
	BATT_LED_PATT_CHG,
	BATT_LED_PATT_NOT_CHG,
};

/* event case */
enum event_type {
	EVENT_TYPE_WCDMA_CALL = 0,
	EVENT_TYPE_GSM_CALL,
	EVENT_TYPE_CALL,
	EVENT_TYPE_VIDEO,
	EVENT_TYPE_MUSIC,
	EVENT_TYPE_BROWSER,
	EVENT_TYPE_HOTSPOT,
	EVENT_TYPE_CAMERA,
	EVENT_TYPE_DATA_CALL,
	EVENT_TYPE_GPS,
	EVENT_TYPE_LTE,
	EVENT_TYPE_WIFI,
	EVENT_TYPE_USE,

	EVENT_TYPE_MAX,
};

enum event_state {
	EVENT_STATE_CLEAR = 0,
	EVENT_STATE_IN_TIMER,
	EVENT_STATE_SET,
};

/**
 * struct sec_bat_plaform_data - init data for sec batter driver
 * @fuel_gauge_name: power supply name of fuel gauge
 * @charger_name: power supply name of charger
 */
struct samsung_battery_platform_data {
	char *charger_name;
	char *fuelgauge_name;
	char *sub_charger_name;
	bool use_sub_charger;

	/* battery voltage design */
	unsigned int voltage_max;
	unsigned int voltage_min;

	/* charge current */
	unsigned int in_curr_limit;
	unsigned int chg_curr_ta;
	unsigned int chg_curr_usb;
	unsigned int chg_curr_cdp;
	unsigned int chg_curr_wpc;
	unsigned int chg_curr_dock;
	unsigned int chg_curr_etc;
	unsigned int chg_curr_siop_lv1;
	unsigned int chg_curr_siop_lv2;
	unsigned int chg_curr_siop_lv3;

	/* variable monitoring interval */
	unsigned int chng_interval;
	unsigned int chng_susp_interval;
	unsigned int norm_interval;
	unsigned int norm_susp_interval;
	unsigned int emer_lv1_interval;
	unsigned int emer_lv2_interval;

	/* Recharge sceanario */
	unsigned int recharge_voltage;
	unsigned int abstimer_charge_duration;
	unsigned int abstimer_charge_duration_wpc;
	unsigned int abstimer_recharge_duration;

	/* cable detect */
	int cb_det_src;
	int cb_det_gpio;
	int cb_det_adc_ch;

	/* Temperature scenario */
	int overheat_stop_temp;
	int overheat_recovery_temp;
	int freeze_stop_temp;
	int freeze_recovery_temp;

	/* CTIA spec */
	bool ctia_spec;

	/* event sceanario */
	unsigned int event_time;

	/* CTIA temperature */
	int event_overheat_stop_temp;
	int event_overheat_recovery_temp;
	int event_freeze_stop_temp;
	int event_freeze_recovery_temp;
	int lpm_overheat_stop_temp;
	int lpm_overheat_recovery_temp;
	int lpm_freeze_stop_temp;
	int lpm_freeze_recovery_temp;

	/* temperature source 0: fuelgauge, 1: ap adc, 2: ex. adc */
	int temper_src;
	int temper_ch;
#ifdef CONFIG_S3C_ADC
	int (*covert_adc) (int, int);
#endif

	/* battery vf source 0: adc(polling), 1: charger(interrupt) */
	int vf_det_src;
	int vf_det_ch;
	int vf_det_th_l;
	int vf_det_th_h;
	int batt_present_gpio;

	/* suspend in charging */
	bool suspend_chging;

	/* support led indicator */
	bool led_indicator;

	/* support battery_standever */
	bool battery_standever;
};

#endif /* __MACH_SAMSUNG_BATTERY_H */
