/*
 *  sec_battery.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2010 Samsung Electronics
 *
 *  <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/android_alarm.h>
#include <plat/adc.h>
#include <linux/power/sec_battery_u1.h>

#if defined(CONFIG_TARGET_LOCALE_NA) || defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
#define POLLING_INTERVAL	(10 * 1000)
#else
#define POLLING_INTERVAL	(40 * 1000)
#endif				/* CONFIG_TARGET_LOCALE_NA */

#ifdef SEC_BATTERY_INDEPEDENT_VF_CHECK
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
#define VF_CHECK_INTERVAL	(5 * 1000)

#if defined(CONFIG_MACH_U1_NA_SPR) || defined(CONFIG_MACH_U1_NA_USCC)
#define	MAX_VF	1500
#define	MIN_VF	1350
#else
#define	MAX_VF	1800
#define	MIN_VF	1100
#endif
#define	VF_COUNT	1
#elif defined(CONFIG_MACH_Q1_BD)
#define VF_CHECK_INTERVAL	(5 * 1000)
#define	MAX_VF_ADC	2500
#define	MIN_VF_ADC	1800
#define	ADC_CH_VF	1
#else
#define VF_CHECK_INTERVAL	(10 * 1000)
#endif
#endif
#define FULL_CHARGING_TIME	(6 * 60 *  60 * HZ)	/* 6hr */
#ifdef CONFIG_TARGET_LOCALE_NA
#define RECHARGING_TIME		(2 * 60 * 60 * HZ)	/* 2hr */
#else
#define RECHARGING_TIME		(90 * 60 * HZ)	/* 1.5hr */
#endif
#define RESETTING_CHG_TIME	(10 * 60 * HZ)	/* 10Min */
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
#define EVENT_OVER_TIME		(10 * 60 * HZ)	/* 10Min */
#endif
#ifdef CONFIG_TARGET_LOCALE_NA
#define BAT_USE_TIMER_EXPIRE	(10 * 60 * HZ)	/* 10 min */
#endif

#if defined(CONFIG_TARGET_LOCALE_NA) || defined(CONFIG_MACH_Q1_BD)
#define RECHARGING_VOLTAGE	(4130 * 1000)	/* 4.13 V */
#else
#define RECHARGING_VOLTAGE	(4150 * 1000)	/* 4.15 V */
#endif

#ifdef CONFIG_TARGET_LOCALE_NA
#define RECHARGING_CND_COUNT    2
#endif

#define FG_T_SOC		0
#define FG_T_VCELL		1
#define FG_T_TEMPER		2
#define FG_T_PSOC		3
#define FG_T_VFOCV		4
#define FG_T_AVGVCELL	5

#define ADC_SAMPLING_CNT	7
#if defined(CONFIG_MACH_Q1_BD)
#define ADC_CH_CHGCURRENT	0
#else
#define ADC_CH_CHGCURRENT	1
#endif
#define ADC_TOTAL_COUNT		5

#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
#define OFFSET_VIDEO_PLAY	(0x1 << 0)
#define OFFSET_MP3_PLAY	(0x1 << 1)
#define OFFSET_VOICE_CALL_2G	(0x1 << 2)
#define OFFSET_VOICE_CALL_3G	(0x1 << 3)
#define OFFSET_DATA_CALL	(0x1 << 4)
#define OFFSET_WIFI	(0x1 << 5)
#define OFFSET_GPS	(0x1 << 6)
#define OFFSET_CAMERA_ON	(0x1 << 7)
#define OFFSET_RECORDING_ON	(0x1 << 8)
#endif

/* put off current during charging*/
#ifdef CONFIG_TARGET_LOCALE_NA
#define USE_CALL			(0x1 << 0)
#define USE_VIDEO			(0x1 << 1)
#define USE_MUSIC			(0x1 << 2)
#define USE_BROWSER			(0x1 << 3)
#define USE_HOTSPOT			(0x1 << 4)
#define	USE_CAMERA			(0x1 << 5)
#define USE_DATA_CALL			(0x1 << 6)
#define USE_WIMAX			(0x1 << 7)
#endif

#ifdef CONFIG_TARGET_LOCALE_NA
#ifdef CONFIG_MACH_U1_NA_SPR_EPIC2_REV00
#define EVENT_BLOCK_TEMP_ADC	       755
#define HIGH_BLOCK_TEMP_ADC            715
#define LOW_BLOCK_TEMP_ADC             521
#define HIGH_RECOVER_TEMP_ADC          714
#define LOW_RECOVER_TEMP_ADC           522

#define HIGH_BLOCK_TEMP_ADC_LPM        715
#define LOW_BLOCK_TEMP_ADC_LPM         521
#define HIGH_RECOVER_TEMP_ADC_LPM      714
#define LOW_RECOVER_TEMP_ADC_LPM       522
#elif defined(CONFIG_MACH_U1_NA_USCC)
#define EVENT_BLOCK_TEMP_ADC		770
#define HIGH_BLOCK_TEMP_ADC		770
#define LOW_BLOCK_TEMP_ADC		502
#define HIGH_RECOVER_TEMP_ADC		699
#define LOW_RECOVER_TEMP_ADC		516

#define HIGH_BLOCK_TEMP_ADC_LPM		710
#define LOW_BLOCK_TEMP_ADC_LPM		506
#define HIGH_RECOVER_TEMP_ADC_LPM	702
#define LOW_RECOVER_TEMP_ADC_LPM	512
#else
#define EVENT_BLOCK_TEMP_ADC           777
#define HIGH_BLOCK_TEMP_ADC            729
#define LOW_BLOCK_TEMP_ADC             502
#define HIGH_RECOVER_TEMP_ADC          699
#define LOW_RECOVER_TEMP_ADC           516

#define HIGH_BLOCK_TEMP_ADC_LPM        710
#define LOW_BLOCK_TEMP_ADC_LPM         506
#define HIGH_RECOVER_TEMP_ADC_LPM      702
#define LOW_RECOVER_TEMP_ADC_LPM       512
#endif
#elif defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
#define EVENT_BLOCK_TEMP			620
#define HIGH_BLOCK_TEMP			490
#define LOW_BLOCK_TEMP			(-30)

#define EVENT_RECOVER_TEMP		430
#define HIGH_RECOVER_TEMP		430
#define LOW_RECOVER_TEMP		0
#elif defined(CONFIG_MACH_Q1_BD)
#define HIGH_BLOCK_TEMP			650
#define LOW_BLOCK_TEMP			(-50)
#define HIGH_RECOVER_TEMP		430
#define LOW_RECOVER_TEMP		0
#else
#define HIGH_BLOCK_TEMP			650
#define LOW_BLOCK_TEMP			(-30)
#define HIGH_RECOVER_TEMP		430
#define LOW_RECOVER_TEMP		0
#endif

#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
#define HIGH_DEC_CURR_TEMP			410
#define HIGH_INC_CURR_TEMP			380
#endif

#ifdef CONFIG_TARGET_LOCALE_NA
#if defined(CONFIG_MACH_U1_NA_SPR_EPIC2_REV00)
#define CURRENT_OF_FULL_CHG                     1
#else
#define CURRENT_OF_FULL_CHG			350
#endif				/* CONFIG_MACH_U1_NA_SPR_EPIC2_REV00 */
#elif defined(CONFIG_MACH_Q1_BD)
#ifdef SEC_BATTERY_1ST_2ND_TOPOFF
#if 0
#define CURRENT_1ST_FULL_CHG		1000	/* 360mA */
#define CURRENT_2ND_FULL_CHG		650	/* 220mA */
#endif
#define CURRENT_1ST_FULL_CHG		580		/* 190mA */
#define CURRENT_2ND_FULL_CHG		540		/* 170mA */
#else
#define CURRENT_OF_FULL_CHG			850
#endif
#else
#define CURRENT_OF_FULL_CHG			520
#endif

#define TEMP_BLOCK_COUNT			3
#ifdef SEC_BATTERY_INDEPEDENT_VF_CHECK
#define BAT_DET_COUNT				0
#else
#define BAT_DET_COUNT				1
#endif

#ifdef CONFIG_TARGET_LOCALE_NA
#define FULL_CHG_COND_COUNT			2
#else
#define FULL_CHG_COND_COUNT			3
#endif

#ifdef CONFIG_TARGET_LOCALE_NA
#define FULL_CHARGE_COND_VOLTAGE    (4000 * 1000)	/* 4.00 V */
#else
#define FULL_CHARGE_COND_VOLTAGE    (4150 * 1000)	/* 4.15 V */
#endif
#define INIT_CHECK_COUNT	4

#ifdef CONFIG_TARGET_LOCALE_NA
#define SPRINT_SLATE_TEST
#endif

enum tmu_status_t {
	TMU_STATUS_NORMAL = 0,
	TMU_STATUS_TRIPPED,
	TMU_STATUS_THROTTLED,
	TMU_STATUS_WARNING,
};

enum cable_type_t {
	CABLE_TYPE_NONE = 0,
	CABLE_TYPE_USB,
	CABLE_TYPE_AC,
	CABLE_TYPE_MISC,
};

enum batt_full_t {
	BATT_NOT_FULL = 0,
	BATT_FULL,
};

enum {
	BAT_NOT_DETECTED,
	BAT_DETECTED
};

#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
enum batt_temp_extra {
	BATT_TEMP_EXT_NONE = 0,
	BATT_TEMP_EXT_CAMCORDING_NORMAL,
	BATT_TEMP_EXT_CAMCORDING_HIGH,
};
#endif

static ssize_t sec_bat_show_property(struct device *dev,
				     struct device_attribute *attr, char *buf);

static ssize_t sec_bat_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count);

struct adc_sample {
	int average_adc;
	int adc_arr[ADC_TOTAL_COUNT];
	int index;
};

struct sec_bat_info {
	struct device *dev;

	char *fuel_gauge_name;
	char *charger_name;
	char *sub_charger_name;

	unsigned int adc_arr_size;
	struct sec_bat_adc_table_data *adc_table;
	unsigned int adc_channel;
	struct adc_sample temper_adc_sample;
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	int vf_adc_channel;
#endif

	struct power_supply psy_bat;
	struct power_supply psy_usb;
	struct power_supply psy_ac;

	struct wake_lock vbus_wake_lock;
	struct wake_lock monitor_wake_lock;
	struct wake_lock cable_wake_lock;

	enum cable_type_t cable_type;
	enum batt_full_t batt_full_status;

	unsigned int batt_temp;	/* Battery Temperature (C) */
	int batt_temp_high_cnt;
	int batt_temp_low_cnt;
	int batt_temp_recover_cnt;
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	enum batt_temp_extra batt_temp_ext;
	enum batt_temp_extra batt_temp_ext_pre;
#endif
	unsigned int batt_health;
	unsigned int batt_vcell;
	unsigned int batt_vfocv;
	unsigned int batt_soc;
	unsigned int batt_raw_soc;
	unsigned int polling_interval;
	int charging_status;
	int charging_int_full_count;
	int charging_adc_full_count;
#ifdef CONFIG_TARGET_LOCALE_NA
	int recharging_int_threshold_count;
#endif

	unsigned int batt_temp_adc;
#ifdef CONFIG_TARGET_LOCALE_NA
	unsigned int batt_temp_radc;
#endif
	unsigned int batt_current_adc;
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	int batt_vf_adc;
	int batt_event_status;
	unsigned long event_end_time;
#elif defined(CONFIG_TARGET_LOCALE_NA)
	int use_call;
	int use_video;
	int use_music;
	int use_browser;
	int use_hotspot;
	int use_camera;
	int use_data_call;
	int use_wimax;
	int batt_event_status;
	unsigned long event_expired_time;
#endif
	struct s3c_adc_client *padc;

	struct workqueue_struct *monitor_wqueue;
	struct work_struct monitor_work;
	struct work_struct cable_work;
	struct delayed_work polling_work;

	unsigned long charging_start_time;
	unsigned long charging_passed_time;
	unsigned long charging_next_time;
	unsigned int recharging_status;
	unsigned int batt_lpm_state;
#ifdef SPRINT_SLATE_TEST
	bool slate_test_mode;
#endif

	struct mutex adclock;

	unsigned int (*get_lpcharging_state) (void);
	int present;
	int present_count;
	bool use_sub_charger;

	int test_value;
	int initial_check_count;
	struct proc_dir_entry *entry;

#ifdef SEC_BATTERY_INDEPEDENT_VF_CHECK
	unsigned int vf_check_interval;
	struct delayed_work vf_check_work;
#endif
	int batt_tmu_status;
};

static char *supply_list[] = {
	"battery",
};

static enum power_supply_property sec_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
#ifdef CONFIG_SLP
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL,
#endif
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CURRENT_AVG,
};

static enum power_supply_property sec_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

#ifdef CONFIG_TARGET_LOCALE_NA
static struct sec_bat_info *pchg;
#endif

struct power_supply *get_power_supply_by_name(char *name)
{
	if (!name)
		return (struct power_supply *)NULL;
	else
		return power_supply_get_by_name(name);
}

static int sec_bat_enable_charging(struct sec_bat_info *info, bool enable);

static int calculate_average_adc(struct sec_bat_info *info,
				 struct adc_sample *sample, int adc)
{
	int i, total_adc = 0;
	int average_adc = sample->average_adc;
	int index = sample->index;

	if (adc < 0 || adc == 0) {
		dev_err(info->dev, "%s: invalid adc : %d\n", __func__, adc);
		return 0;
	}

	if (!average_adc) {
		average_adc = adc;
		for (i = 0; i < ADC_TOTAL_COUNT; i++)
			sample->adc_arr[i] = adc;
	} else {
		sample->index = ++index >= ADC_TOTAL_COUNT ? 0 : index;
		sample->adc_arr[sample->index] = adc;
		for (i = 0; i < ADC_TOTAL_COUNT; i++)
			total_adc += sample->adc_arr[i];

		average_adc = total_adc / ADC_TOTAL_COUNT;
	}

	sample->average_adc = average_adc;
	dev_dbg(info->dev, "%s: i(%d) adc=%d, avg_adc=%d\n", __func__,
		sample->index, adc, average_adc);

	return average_adc;
}

static int sec_bat_get_fuelgauge_data(struct sec_bat_info *info, int type)
{
	struct power_supply *psy
	    = get_power_supply_by_name(info->fuel_gauge_name);
	union power_supply_propval value;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get fuel gauge ps\n", __func__);
		return -ENODEV;
	}

	switch (type) {
	case FG_T_VCELL:
		value.intval = 0;	/*vcell */
		psy->get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &value);
		break;
	case FG_T_VFOCV:
		value.intval = 1;	/*vfocv */
		psy->get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &value);
		break;
	case FG_T_AVGVCELL:
		psy->get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_AVG, &value);
		break;
	case FG_T_SOC:
		value.intval = 0;	/*normal soc */
		psy->get_property(psy, POWER_SUPPLY_PROP_CAPACITY, &value);
		break;
	case FG_T_PSOC:
		value.intval = 1;	/*raw soc */
		psy->get_property(psy, POWER_SUPPLY_PROP_CAPACITY, &value);
		break;
	case FG_T_TEMPER:
		psy->get_property(psy, POWER_SUPPLY_PROP_TEMP, &value);
		break;
	default:
		return -ENODEV;
	}

	return value.intval;
}

static int sec_bat_get_property(struct power_supply *ps,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_bat_info *info = container_of(ps, struct sec_bat_info,
						 psy_bat);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = info->charging_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = info->batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = info->batt_temp;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		/* battery is always online */
		/* val->intval = 1; */
		val->intval = info->cable_type;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = info->batt_vcell;
		if (val->intval == -1)
			return -EINVAL;
		break;
#ifdef CONFIG_SLP
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		if (info->charging_status == POWER_SUPPLY_STATUS_FULL)
			val->intval = true;
		else
			val->intval = false;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		if (info->charging_status == POWER_SUPPLY_STATUS_CHARGING)
			val->intval = true;
		else
			val->intval = false;
		break;
#endif
	case POWER_SUPPLY_PROP_CAPACITY:
		if (info->charging_status == POWER_SUPPLY_STATUS_FULL) {
			val->intval = 100;
			break;
		}
		val->intval = info->batt_soc;
		if (val->intval == -1)
			return -EINVAL;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = 1;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void sec_bat_handle_charger_topoff(struct sec_bat_info *info)
{
#ifdef SEC_BATTERY_1ST_2ND_TOPOFF
	if (info->charging_status != POWER_SUPPLY_STATUS_FULL) {
		/* 1st top-off */
		info->charging_status = POWER_SUPPLY_STATUS_FULL;

		dev_info(info->dev, "%s: Charging 1st Top-off\n", __func__);
	} else {
		/* 2nd top-off, recharging top-off */
		if (!sec_bat_enable_charging(info, false)) {
			info->batt_full_status = BATT_FULL;
			info->recharging_status = false;
		}

		dev_info(info->dev, "%s: Charging 2nd Top-off\n", __func__);
	}
#else
	if (!sec_bat_enable_charging(info, false)) {
		info->charging_status = POWER_SUPPLY_STATUS_FULL;
		info->batt_full_status = BATT_FULL;
		info->recharging_status = false;

		dev_info(info->dev, "%s: Charging Top-off\n", __func__);
	}
#endif
}

static int sec_bat_set_property(struct power_supply *ps,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sec_bat_info *info = container_of(ps, struct sec_bat_info,
						 psy_bat);
	struct power_supply *psy = get_power_supply_by_name(info->charger_name);
	union power_supply_propval value;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get %s ps\n",
			__func__, info->charger_name);
		return -EINVAL;
	}

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		dev_info(info->dev, "%s: topoff intr\n", __func__);
		if (val->intval != POWER_SUPPLY_STATUS_FULL)
			return -EINVAL;

		if (info->use_sub_charger) {
#if !defined(CONFIG_MACH_Q1_BD)
			if (info->cable_type == CABLE_TYPE_USB ||
			    info->cable_type == CABLE_TYPE_MISC)
#endif
				sec_bat_handle_charger_topoff(info);
			break;
		}

		if (info->batt_full_status == BATT_NOT_FULL) {
			info->recharging_status = false;
			info->batt_full_status = BATT_FULL;
			info->charging_status = POWER_SUPPLY_STATUS_FULL;

			info->charging_start_time = 0;
			info->charging_passed_time = 0;
			info->charging_next_time = 0;

			/* disable charging */
			value.intval = POWER_SUPPLY_STATUS_DISCHARGING;
			psy->set_property(psy, POWER_SUPPLY_PROP_STATUS,
					  &value);
		}
#if 0				/* for reference */
		if (info->batt_full_status == BATT_NOT_FULL) {
			info->batt_full_status = BATT_1ST_FULL;
			info->charging_status = POWER_SUPPLY_STATUS_FULL;
			/* TODO: set topoff current 60mA */
			value.intval = 120;
			psy->set_property(psy, POWER_SUPPLY_PROP_CHARGE_FULL,
					  &value);
		} else {
			info->batt_full_status = BATT_2ND_FULL;
			info->recharging_status = false;
			/* disable charging */
			value.intval = POWER_SUPPLY_STATUS_DISCHARGING;
			psy->set_property(psy, POWER_SUPPLY_PROP_STATUS,
					  &value);
		}
#endif
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		dev_info(info->dev, "%s: refresh battery data\n", __func__);
		wake_lock(&info->monitor_wake_lock);
		queue_work(info->monitor_wqueue, &info->monitor_work);

		break;
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		/* TODO: lowbatt interrupt: called by fuel gauge */
		dev_info(info->dev, "%s: lowbatt intr\n", __func__);
		if (val->intval != POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL)
			return -EINVAL;
		wake_lock(&info->monitor_wake_lock);
		queue_work(info->monitor_wqueue, &info->monitor_work);

		break;
	case POWER_SUPPLY_PROP_ONLINE:
		/* cable is attached or detached. called by USB switch(MUIC) */
		dev_info(info->dev, "%s: cable was changed(%d)\n", __func__,
			 val->intval);
		switch (val->intval) {
		case POWER_SUPPLY_TYPE_BATTERY:
			info->cable_type = CABLE_TYPE_NONE;
			break;
		case POWER_SUPPLY_TYPE_MAINS:
			info->cable_type = CABLE_TYPE_AC;
			break;
		case POWER_SUPPLY_TYPE_USB:
			info->cable_type = CABLE_TYPE_USB;
			break;
		case POWER_SUPPLY_TYPE_MISC:
			info->cable_type = CABLE_TYPE_MISC;
			break;
		default:
			return -EINVAL;
		}
		wake_lock(&info->cable_wake_lock);
		queue_work(info->monitor_wqueue, &info->cable_work);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		/* call by TMU driver
		 * alarm for abnormal temperature increasement
		 */
		info->batt_tmu_status = val->intval;

		wake_lock(&info->monitor_wake_lock);
		queue_work(info->monitor_wqueue, &info->monitor_work);

		dev_info(info->dev, "%s: TMU status has been changed(%d)\n",
			 __func__, info->batt_tmu_status);

		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sec_usb_get_property(struct power_supply *ps,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_bat_info *info = container_of(ps, struct sec_bat_info,
						 psy_usb);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	/* Set enable=1 only if the USB charger is connected */
	val->intval = (info->cable_type == CABLE_TYPE_USB);

	return 0;
}

static int sec_ac_get_property(struct power_supply *ps,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	struct sec_bat_info *info = container_of(ps, struct sec_bat_info,
						 psy_ac);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	/* Set enable=1 only if the AC charger is connected */
	val->intval = (info->cable_type == CABLE_TYPE_AC) ||
	    (info->cable_type == CABLE_TYPE_MISC);

	return 0;
}

static int sec_bat_get_adc_data(struct sec_bat_info *info, int adc_ch)
{
	int adc_data;
	int adc_max = 0;
	int adc_min = 0;
	int adc_total = 0;
	int i;
	int err_value;

	for (i = 0; i < ADC_SAMPLING_CNT; i++) {
		adc_data = s3c_adc_read(info->padc, adc_ch);
		if (adc_data < 0) {
			dev_err(info->dev, "%s : err(%d) returned, skip read\n",
				__func__, adc_data);
			err_value = adc_data;
			goto err;
		}

		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}
		adc_total += adc_data;
	}

	return (adc_total - adc_max - adc_min) / (ADC_SAMPLING_CNT - 2);
 err:
	return err_value;
}

#ifdef CONFIG_TARGET_LOCALE_NA
static inline int s3c_read_temper_adc(struct sec_bat_info *info)
{
	int adc;

	mutex_lock(&info->adclock);
	adc = sec_bat_get_adc_data(info, info->adc_channel);
	mutex_unlock(&info->adclock);
	if (adc <= 0)
		adc = info->batt_temp_adc;

	info->batt_temp_adc =
	    calculate_average_adc(info, &info->temper_adc_sample, adc);

	return info->batt_temp_adc;
}
#else
static inline int s3c_read_temper_adc(struct sec_bat_info *info)
{
	int adc;

	mutex_lock(&info->adclock);
	adc = sec_bat_get_adc_data(info, info->adc_channel);
	mutex_unlock(&info->adclock);
	if (adc <= 0)
		adc = info->batt_temp_adc;
	info->batt_temp_adc = adc;

	return adc;
}
#endif

#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
static int is_event_end_timer_running(struct sec_bat_info *info)
{
	unsigned long passed_time = 0;

	if (info->event_end_time == 0xFFFFFFFF)	/*initial value */
		return false;

	if (jiffies >= info->event_end_time)
		passed_time = jiffies - info->event_end_time;
	else
		passed_time = 0xFFFFFFFF - info->event_end_time + jiffies;

	if (time_after(passed_time, (unsigned long)EVENT_OVER_TIME)) {
		info->event_end_time = 0xFFFFFFFF;
		#ifndef PRODUCT_SHIP
		dev_info(info->dev, "%s: Event timer is over 10 min\n",
			 __func__);
		#endif
		return false;
	} else {
	#ifndef PRODUCT_SHIP
		dev_info(info->dev, "%s: Event timer is running(%u s)\n",
			 __func__, jiffies_to_msecs(passed_time) / 1000);
	#endif
		return true;
	}

	return false;
}

static int get_recording_state(struct sec_bat_info *info)
{
	return info->batt_event_status & OFFSET_RECORDING_ON;
}
#endif

#ifdef CONFIG_TARGET_LOCALE_NA
static int is_event_end_timer_running(struct sec_bat_info *info)
{
	unsigned long passed_time = 0;

	if (info->event_expired_time == 0xFFFFFFFF)	/*initial value */
		return false;

	if (jiffies >= info->event_expired_time)
		passed_time = jiffies - info->event_expired_time;
	else
		passed_time = 0xFFFFFFFF - info->event_expired_time + jiffies;

	if (time_after(passed_time, (unsigned long)BAT_USE_TIMER_EXPIRE)) {
		info->event_expired_time = 0xFFFFFFFF;
		#ifndef PRODUCT_SHIP
		dev_info(info->dev, "[SPR_NA] %s: Event timer is over 10 min\n",
			 __func__);
		#endif
		return false;
	} else {
#ifndef PRODUCT_SHIP
		dev_info(info->dev,
			 "[SPR_NA] %s: Event timer is running(%u s)\n",
			 __func__, jiffies_to_msecs(passed_time) / 1000);
#endif
		return true;
	}

	return false;
}

static unsigned long sec_rescale_temp_adc(struct sec_bat_info *info)
{
	int adc_tmp = info->batt_temp_adc;
	int adc_tmp1 = 0;
	int adc_tmp2 = 0;

	adc_tmp1 = adc_tmp * 10;
	adc_tmp2 = (40950 - adc_tmp1);
	adc_tmp = adc_tmp2 / 50;
	if ((adc_tmp2 % 10) >= 5)
		adc_tmp += 1;

	info->batt_temp_radc = adc_tmp;

	dev_dbg(info->dev, "deb***: [battery] bat temper : %d, rescale : %d\n",
	       info->batt_temp_adc, adc_tmp);

	return adc_tmp;
}
#endif

#ifdef CONFIG_TARGET_LOCALE_NA
static int sec_bat_check_temper(struct sec_bat_info *info)
{
	struct power_supply *psy
	    = get_power_supply_by_name(info->fuel_gauge_name);
	union power_supply_propval value;
	int ret;

	int temp;
	int temp_adc = s3c_read_temper_adc(info);
	int temp_radc = sec_rescale_temp_adc(info);
	int health = info->batt_health;
	int low = 0;
	int high = 0;
	int mid = 0;

	/*calculate_average_adc(info, &info->temper_adc_sample, temp_adc);*/

	if (!info->adc_table || !info->adc_arr_size) {
		/* using fake temp */
		temp = 300;
		info->batt_temp = temp;
		return temp;
	}
	high = info->adc_arr_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (info->adc_table[mid].adc > temp_adc)
			high = mid - 1;
		else if (info->adc_table[mid].adc < temp_adc)
			low = mid + 1;
		else
			break;
	}
	temp = info->adc_table[mid].temperature;

	info->batt_temp = temp;

	if (info->get_lpcharging_state) {
		if (info->get_lpcharging_state()) {
			if (temp_radc >= HIGH_BLOCK_TEMP_ADC_LPM) {
				if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
				    health !=
				    POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
					if (info->batt_temp_high_cnt <
					    TEMP_BLOCK_COUNT)
						info->batt_temp_high_cnt++;
				dev_info(info->dev, "%s: high count = %d\n",
					 __func__, info->batt_temp_high_cnt);
			} else if (temp_radc <= HIGH_RECOVER_TEMP_ADC_LPM &&
				   temp_radc >= LOW_RECOVER_TEMP_ADC_LPM) {
				if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
				    health == POWER_SUPPLY_HEALTH_COLD) {
					info->batt_temp_high_cnt = 0;
					info->batt_temp_low_cnt = 0;

					if (info->batt_temp_recover_cnt <
					    TEMP_BLOCK_COUNT)
						info->batt_temp_recover_cnt++;
					dev_info(info->dev,
						 "%s: recovery count = %d\n",
						 __func__,
						 info->batt_temp_recover_cnt);
				}
			} else if (temp_radc <= LOW_BLOCK_TEMP_ADC_LPM) {
				if (health != POWER_SUPPLY_HEALTH_COLD &&
				    health !=
				    POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
					if (info->batt_temp_low_cnt <
					    TEMP_BLOCK_COUNT)
						info->batt_temp_low_cnt++;
				dev_info(info->dev, "%s: low count = %d\n",
					 __func__, info->batt_temp_low_cnt);
			} else {
				info->batt_temp_high_cnt = 0;
				info->batt_temp_low_cnt = 0;
				info->batt_temp_recover_cnt = 0;
			}
		} else {
			if ((info->batt_event_status)
			    || (is_event_end_timer_running(info))) {
#ifndef PRODUCT_SHIP
				dev_info(info->dev,
					 "%s: [NA_SPR] Changed Put off Current",
					 __func__);
#endif
				if (temp_radc >= EVENT_BLOCK_TEMP_ADC) {
					if (health !=
					    POWER_SUPPLY_HEALTH_OVERHEAT
					    && health !=
					    POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
						if (info->batt_temp_high_cnt <
						    TEMP_BLOCK_COUNT)
							info->
							batt_temp_high_cnt++;
					dev_info(info->dev,
						 "%s: high count = %d\n",
						 __func__,
						 info->batt_temp_high_cnt);
				} else if (temp_radc <= HIGH_RECOVER_TEMP_ADC
					   && temp_radc >=
					   LOW_RECOVER_TEMP_ADC) {
					if (health ==
					    POWER_SUPPLY_HEALTH_OVERHEAT
					    || health ==
					    POWER_SUPPLY_HEALTH_COLD) {
						info->batt_temp_high_cnt = 0;
						info->batt_temp_low_cnt = 0;

						if (info->
						    batt_temp_recover_cnt <
						    TEMP_BLOCK_COUNT)
							info->
							batt_temp_recover_cnt++;
						dev_info(info->dev,
							 "%s: recovery count = %d\n",
							 __func__,
							 info->
							 batt_temp_recover_cnt);
					}
				} else if (temp_radc <= LOW_BLOCK_TEMP_ADC) {
					if (health != POWER_SUPPLY_HEALTH_COLD
					    && health !=
					    POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
						if (info->batt_temp_low_cnt <
						    TEMP_BLOCK_COUNT)
							info->
							    batt_temp_low_cnt++;
					dev_info(info->dev,
						 "%s: low count = %d\n",
						 __func__,
						 info->batt_temp_low_cnt);
				} else {
					info->batt_temp_high_cnt = 0;
					info->batt_temp_low_cnt = 0;
					info->batt_temp_recover_cnt = 0;
				}
			} else {
				if (temp_radc >= HIGH_BLOCK_TEMP_ADC) {
					if (health !=
					    POWER_SUPPLY_HEALTH_OVERHEAT
					    && health !=
					    POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
						if (info->batt_temp_high_cnt <
						    TEMP_BLOCK_COUNT)
							info->
							batt_temp_high_cnt++;
					dev_info(info->dev,
						 "%s: high count = %d\n",
						 __func__,
						 info->batt_temp_high_cnt);
				} else if (temp_radc <= HIGH_RECOVER_TEMP_ADC
					   && temp_radc >=
					   LOW_RECOVER_TEMP_ADC) {
					if (health ==
					    POWER_SUPPLY_HEALTH_OVERHEAT
					    || health ==
					    POWER_SUPPLY_HEALTH_COLD) {

						info->batt_temp_high_cnt = 0;
						info->batt_temp_low_cnt = 0;

						if (info->
						    batt_temp_recover_cnt <
						    TEMP_BLOCK_COUNT)
							info->
							batt_temp_recover_cnt++;
						dev_info(info->dev,
							 "%s: recovery count = %d\n",
							 __func__,
							 info->
							 batt_temp_recover_cnt);
					}
				} else if (temp_radc <= LOW_BLOCK_TEMP_ADC) {
					if (health != POWER_SUPPLY_HEALTH_COLD
					    && health !=
					    POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
						if (info->batt_temp_low_cnt <
						    TEMP_BLOCK_COUNT)
							info->
							    batt_temp_low_cnt++;
					dev_info(info->dev,
						 "%s: low count = %d\n",
						 __func__,
						 info->batt_temp_low_cnt);
				} else {
					info->batt_temp_high_cnt = 0;
					info->batt_temp_low_cnt = 0;
					info->batt_temp_recover_cnt = 0;
				}
			}
		}
	}

	if (info->cable_type == CABLE_TYPE_NONE) {
		if (info->batt_health == POWER_SUPPLY_HEALTH_OVERHEAT ||
		    info->batt_health == POWER_SUPPLY_HEALTH_COLD) {
			info->batt_health = POWER_SUPPLY_HEALTH_GOOD;
			goto skip;
		}
	}

	if (info->batt_temp_high_cnt >= TEMP_BLOCK_COUNT)
		info->batt_health = POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (info->batt_temp_low_cnt >= TEMP_BLOCK_COUNT)
		info->batt_health = POWER_SUPPLY_HEALTH_COLD;
	else if (info->batt_temp_recover_cnt >= TEMP_BLOCK_COUNT)
		info->batt_health = POWER_SUPPLY_HEALTH_GOOD;

	/*else
	   info->batt_health = POWER_SUPPLY_HEALTH_GOOD; */

	/*else if (info->batt_temp_recover_cnt >= TEMP_BLOCK_COUNT)
	   info->batt_health = POWER_SUPPLY_HEALTH_GOOD; */
 skip:
	/* Set temperature to fuel gauge */
	if (info->fuel_gauge_name) {
		value.intval = info->batt_temp / 10;
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_TEMP, &value);
		if (ret) {
			dev_err(info->dev, "%s: fail to set temperature(%d)\n",
				__func__, ret);
		}
	}
#ifndef PRODUCT_SHIP
	dev_info(info->dev, "%s: temp=%d, adc=%d\n", __func__, temp, temp_adc);
#endif
	return temp;
}

static void sec_bat_get_dcinovp(struct sec_bat_info *info)
{
	struct power_supply *psy = get_power_supply_by_name(info->charger_name);
	union power_supply_propval value;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get %s ps\n",
			__func__, info->charger_name);
		return;
	}

	if (info->slate_test_mode) {
		info->charging_status = POWER_SUPPLY_STATUS_DISCHARGING;
		info->cable_type = CABLE_TYPE_NONE;
	} else {
		psy->get_property(psy, POWER_SUPPLY_PROP_STATUS, &value);
		info->charging_status = value.intval;
	}
}

#elif defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
static int sec_bat_check_temper(struct sec_bat_info *info)
{
	struct power_supply *psy
	    = get_power_supply_by_name(info->fuel_gauge_name);
	union power_supply_propval value;
	int ret;

	int temp;
	int temp_adc = s3c_read_temper_adc(info);
	int health = info->batt_health;
	int low = 0;
	int high = 0;
	int mid = 0;

	calculate_average_adc(info, &info->temper_adc_sample, temp_adc);

	if (!info->adc_table || !info->adc_arr_size) {
		/* using fake temp */
		temp = 300;
		info->batt_temp = temp;
		return temp;
	}
	high = info->adc_arr_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (info->adc_table[mid].adc > temp_adc)
			high = mid - 1;
		else if (info->adc_table[mid].adc < temp_adc)
			low = mid + 1;
		else
			break;
	}
	temp = info->adc_table[mid].temperature;

	info->batt_temp = temp;

	if ((info->batt_event_status) || (is_event_end_timer_running(info))) {
		/*printk("[%s] event temp is applied\n", __func__); */
		if (temp >= EVENT_BLOCK_TEMP) {
			if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
			    health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
				if (info->batt_temp_high_cnt < TEMP_BLOCK_COUNT)
					info->batt_temp_high_cnt++;
			dev_info(info->dev,
				 "%s: event high count = %d(in event)\n",
				 __func__, info->batt_temp_high_cnt);
		} else if (temp <= EVENT_RECOVER_TEMP
			   && temp >= LOW_RECOVER_TEMP) {
			if (health == POWER_SUPPLY_HEALTH_OVERHEAT
			    || health == POWER_SUPPLY_HEALTH_COLD) {

				info->batt_temp_high_cnt = 0;
				info->batt_temp_low_cnt = 0;

				if (info->batt_temp_recover_cnt <
				    TEMP_BLOCK_COUNT)
					info->batt_temp_recover_cnt++;
				dev_info(info->dev,
					 "%s: event recovery count = %d\n",
					 __func__, info->batt_temp_recover_cnt);
			}
		} else if (temp <= LOW_BLOCK_TEMP) {
			if (health != POWER_SUPPLY_HEALTH_COLD &&
			    health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
				if (info->batt_temp_low_cnt < TEMP_BLOCK_COUNT)
					info->batt_temp_low_cnt++;
			dev_info(info->dev,
				 "%s: event low count = %d(in event)\n",
				 __func__, info->batt_temp_low_cnt);
		} else {
			info->batt_temp_high_cnt = 0;
			info->batt_temp_low_cnt = 0;
			info->batt_temp_recover_cnt = 0;
		}
	} else {
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
		info->batt_temp_ext_pre = info->batt_temp_ext;
		switch (info->batt_temp_ext) {
		case BATT_TEMP_EXT_NONE:
			if (get_recording_state(info))
				info->batt_temp_ext =
				    BATT_TEMP_EXT_CAMCORDING_NORMAL;
			break;
		case BATT_TEMP_EXT_CAMCORDING_NORMAL:
			if (!get_recording_state(info))
				info->batt_temp_ext = BATT_TEMP_EXT_NONE;
			else if (temp >= HIGH_DEC_CURR_TEMP)
				info->batt_temp_ext =
				    BATT_TEMP_EXT_CAMCORDING_HIGH;
			break;
		case BATT_TEMP_EXT_CAMCORDING_HIGH:
			if (!get_recording_state(info))
				info->batt_temp_ext = BATT_TEMP_EXT_NONE;
			else if (temp <= HIGH_INC_CURR_TEMP)
				info->batt_temp_ext =
				    BATT_TEMP_EXT_CAMCORDING_NORMAL;
			break;
		default:
			break;
		}
		dev_info(info->dev,
			 "%s: batt_temp_ext_pre=%d, batt_temp_ext=%d\n",
			 __func__, info->batt_temp_ext_pre,
			 info->batt_temp_ext);
#endif
		/*printk("[%s] nomal temp is applied\n", __func__); */
		if (temp >= HIGH_BLOCK_TEMP) {
			if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
			    health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
				if (info->batt_temp_high_cnt < TEMP_BLOCK_COUNT)
					info->batt_temp_high_cnt++;
			dev_info(info->dev, "%s: high count = %d\n",
				 __func__, info->batt_temp_high_cnt);
		} else if (temp <= HIGH_RECOVER_TEMP
			   && temp >= LOW_RECOVER_TEMP) {
			if (health == POWER_SUPPLY_HEALTH_OVERHEAT
			    || health == POWER_SUPPLY_HEALTH_COLD) {

				info->batt_temp_high_cnt = 0;
				info->batt_temp_low_cnt = 0;

				if (info->batt_temp_recover_cnt <
				    TEMP_BLOCK_COUNT)
					info->batt_temp_recover_cnt++;
				dev_info(info->dev, "%s: recovery count = %d\n",
					 __func__, info->batt_temp_recover_cnt);
			}
		} else if (temp <= LOW_BLOCK_TEMP) {
			if (health != POWER_SUPPLY_HEALTH_COLD &&
			    health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
				if (info->batt_temp_low_cnt < TEMP_BLOCK_COUNT)
					info->batt_temp_low_cnt++;
			dev_info(info->dev, "%s: low count = %d\n",
				 __func__, info->batt_temp_low_cnt);
		} else {
			info->batt_temp_high_cnt = 0;
			info->batt_temp_low_cnt = 0;
			info->batt_temp_recover_cnt = 0;
		}
	}

	if (info->cable_type == CABLE_TYPE_NONE) {
		info->batt_temp_high_cnt = 0;
		info->batt_temp_low_cnt = 0;
	}

	if (info->batt_temp_high_cnt >= TEMP_BLOCK_COUNT)
		info->batt_health = POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (info->batt_temp_low_cnt >= TEMP_BLOCK_COUNT)
		info->batt_health = POWER_SUPPLY_HEALTH_COLD;
	else if (info->batt_temp_recover_cnt >= TEMP_BLOCK_COUNT)
		info->batt_health = POWER_SUPPLY_HEALTH_GOOD;

	/* Set temperature to fuel gauge */
	if (info->fuel_gauge_name) {
		value.intval = info->batt_temp / 10;
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_TEMP, &value);
		if (ret) {
			dev_err(info->dev, "%s: fail to set temperature(%d)\n",
				__func__, ret);
		}
	}
#ifndef PRODUCT_SHIP
	dev_info(info->dev, "%s: temp=%d, adc=%d\n", __func__, temp, temp_adc);
#endif

	return temp;
}

#else
static int sec_bat_check_temper(struct sec_bat_info *info)
{
	struct power_supply *psy
	    = get_power_supply_by_name(info->fuel_gauge_name);
	union power_supply_propval value;
	int ret;

	int temp;
	int temp_adc = s3c_read_temper_adc(info);
	int health = info->batt_health;
	int low = 0;
	int high = 0;
	int mid = 0;

	calculate_average_adc(info, &info->temper_adc_sample, temp_adc);

	if (!info->adc_table || !info->adc_arr_size) {
		/* using fake temp */
		temp = 300;
		info->batt_temp = temp;
		return temp;
	}
	high = info->adc_arr_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (info->adc_table[mid].adc > temp_adc)
			high = mid - 1;
		else if (info->adc_table[mid].adc < temp_adc)
			low = mid + 1;
		else
			break;
	}
	temp = info->adc_table[mid].temperature;

	info->batt_temp = temp;

	if (temp >= HIGH_BLOCK_TEMP) {
		if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
		    health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
			if (info->batt_temp_high_cnt < TEMP_BLOCK_COUNT)
				info->batt_temp_high_cnt++;
		dev_info(info->dev, "%s: high count = %d\n",
			 __func__, info->batt_temp_high_cnt);
	} else if (temp <= HIGH_RECOVER_TEMP && temp >= LOW_RECOVER_TEMP) {
		if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
		    health == POWER_SUPPLY_HEALTH_COLD)
			if (info->batt_temp_recover_cnt < TEMP_BLOCK_COUNT)
				info->batt_temp_recover_cnt++;
		dev_info(info->dev, "%s: recovery count = %d\n",
			 __func__, info->batt_temp_recover_cnt);
	} else if (temp <= LOW_BLOCK_TEMP) {
		if (health != POWER_SUPPLY_HEALTH_COLD &&
		    health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
			if (info->batt_temp_low_cnt < TEMP_BLOCK_COUNT)
				info->batt_temp_low_cnt++;
		dev_info(info->dev, "%s: low count = %d\n",
			 __func__, info->batt_temp_low_cnt);
	} else {
		info->batt_temp_high_cnt = 0;
		info->batt_temp_low_cnt = 0;
		info->batt_temp_recover_cnt = 0;
	}

	if (info->batt_temp_high_cnt >= TEMP_BLOCK_COUNT)
		info->batt_health = POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (info->batt_temp_low_cnt >= TEMP_BLOCK_COUNT)
		info->batt_health = POWER_SUPPLY_HEALTH_COLD;
	else if (info->batt_temp_recover_cnt >= TEMP_BLOCK_COUNT)
		info->batt_health = POWER_SUPPLY_HEALTH_GOOD;

	/* Set temperature to fuel gauge */
	if (info->fuel_gauge_name) {
		value.intval = info->batt_temp / 10;
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_TEMP, &value);
		if (ret) {
			dev_err(info->dev, "%s: fail to set temperature(%d)\n",
				__func__, ret);
		}
	}
#ifndef PRODUCT_SHIP
	dev_info(info->dev, "%s: temp=%d, adc=%d\n", __func__, temp, temp_adc);
#endif
	return temp;
}
#endif
static void check_chgcurrent(struct sec_bat_info *info)
{
	unsigned long chg_current_adc = 0;

	mutex_lock(&info->adclock);
	chg_current_adc = sec_bat_get_adc_data(info, ADC_CH_CHGCURRENT);
	mutex_unlock(&info->adclock);
	if (chg_current_adc < 0)
		chg_current_adc = info->batt_current_adc;
	info->batt_current_adc = chg_current_adc;

	dev_dbg(info->dev,
		"[battery] charging current = %d\n", info->batt_current_adc);
}

static bool sec_check_chgcurrent(struct sec_bat_info *info)
{
	switch (info->charging_status) {
	case POWER_SUPPLY_STATUS_FULL:
		info->batt_current_adc = 0;
		if ((info->batt_full_status == BATT_FULL) &&
		    (info->recharging_status == false))
			break;
	case POWER_SUPPLY_STATUS_CHARGING:
		check_chgcurrent(info);

		if (info->batt_vfocv >= FULL_CHARGE_COND_VOLTAGE / 1000) {

#ifdef SEC_BATTERY_1ST_2ND_TOPOFF
			if (((info->charging_status ==
			      POWER_SUPPLY_STATUS_CHARGING)
			     && (info->batt_current_adc <=
				 CURRENT_1ST_FULL_CHG))
			    ||
			    ((info->charging_status == POWER_SUPPLY_STATUS_FULL)
			     && (info->batt_current_adc <=
				 CURRENT_2ND_FULL_CHG))) {
#else
			if (info->batt_current_adc <= CURRENT_OF_FULL_CHG) {
#endif
				/*TA full charged */
				info->charging_adc_full_count++;
				if (info->charging_adc_full_count >=
				    FULL_CHG_COND_COUNT) {
					info->charging_adc_full_count = 0;
					sec_bat_handle_charger_topoff(info);

					return true;
				}
				dev_info(info->dev, "%s : ADC full cnt = %d\n",
					 __func__,
					 info->charging_adc_full_count);
			} else
				info->charging_adc_full_count = 0;
		}
		break;
	default:
		info->charging_adc_full_count = 0;
		info->batt_current_adc = 0;
		break;
	}

	return false;
}

static void sec_bat_update_info(struct sec_bat_info *info)
{
	info->batt_raw_soc = sec_bat_get_fuelgauge_data(info, FG_T_PSOC);
	info->batt_soc = sec_bat_get_fuelgauge_data(info, FG_T_SOC);
	info->batt_vcell = sec_bat_get_fuelgauge_data(info, FG_T_VCELL);
	info->batt_vfocv = sec_bat_get_fuelgauge_data(info, FG_T_VFOCV);
}

static int sec_bat_enable_charging_main(struct sec_bat_info *info, bool enable)
{
	struct power_supply *psy = get_power_supply_by_name(info->charger_name);
	union power_supply_propval val_type, val_chg_current, val_topoff;
	int ret;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return -ENODEV;
	}

	info->batt_full_status = BATT_NOT_FULL;

	if (enable) {		/* Enable charging */
		switch (info->cable_type) {
		case CABLE_TYPE_USB:
			val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
			val_chg_current.intval = 450;	/* mA */
			break;
		case CABLE_TYPE_AC:
			val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
			val_chg_current.intval = 650;	/* mA */
			break;
		case CABLE_TYPE_MISC:
			val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
			val_chg_current.intval = 450;	/* mA */
			break;
		default:
			dev_err(info->dev, "%s: Invalid func use\n", __func__);
			return -EINVAL;
		}

		/* Set charging current */
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_CURRENT_NOW,
					&val_chg_current);
		if (ret) {
			dev_err(info->dev, "%s: fail to set charging cur(%d)\n",
				__func__, ret);
			return ret;
		}

		/* Set topoff current */
		/* mA: TODO: should change 200->160 */
		val_topoff.intval = 200;
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_CHARGE_FULL,
					&val_topoff);
		if (ret) {
			dev_err(info->dev, "%s: fail to set topoff cur(%d)\n",
				__func__, ret);
			return ret;
		}

		/*Reset charging start time only in initial charging start */
		if (info->charging_start_time == 0) {
			info->charging_start_time = jiffies;
			info->charging_next_time = RESETTING_CHG_TIME;
		}
	} else {		/* Disable charging */
		val_type.intval = POWER_SUPPLY_STATUS_DISCHARGING;

		info->charging_start_time = 0;
		info->charging_passed_time = 0;
		info->charging_next_time = 0;
	}

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_STATUS, &val_type);
	if (ret) {
		dev_err(info->dev, "%s: fail to set charging status(%d)\n",
			__func__, ret);
		return ret;
	}
	return 0;
}

static int sec_bat_enable_charging_sub(struct sec_bat_info *info, bool enable)
{
	struct power_supply *psy_main =
	    get_power_supply_by_name(info->charger_name);
	struct power_supply *psy_sub =
	    get_power_supply_by_name(info->sub_charger_name);
	union power_supply_propval val_type, val_chg_current;
	int ret;

	if (!psy_main || !psy_sub) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return -ENODEV;
	}

	info->batt_full_status = BATT_NOT_FULL;

	if (enable) {		/* Enable charging */
		val_type.intval = POWER_SUPPLY_STATUS_DISCHARGING;
		ret = psy_main->set_property(psy_main, POWER_SUPPLY_PROP_STATUS,
					     &val_type);
		if (ret) {
			dev_err(info->dev, "%s: fail to set charging"
				" status-main(%d)\n", __func__, ret);
			return ret;
		}
#if defined(CONFIG_MACH_Q1_BD)
		/* Set charging current only in charging start time */
		if (info->charging_start_time == 0) {
#endif
			switch (info->cable_type) {
			case CABLE_TYPE_USB:
				val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
				val_chg_current.intval = 450;	/* mA */
				break;
			case CABLE_TYPE_AC:
				val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
				if (info->batt_temp_ext ==
				    BATT_TEMP_EXT_CAMCORDING_HIGH)
					val_chg_current.intval = 450;	/* mA */
				else
#endif
					val_chg_current.intval = 650;	/* mA */
				break;
			case CABLE_TYPE_MISC:
				val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
				val_chg_current.intval = 450;	/* mA */
				break;
			default:
				dev_err(info->dev, "%s: Invalid func use\n",
					__func__);
				return -EINVAL;
			}

			/* Set charging current */
			ret = psy_sub->set_property(psy_sub,
				POWER_SUPPLY_PROP_CURRENT_NOW,
				&val_chg_current);
			if (ret) {
				dev_err(info->dev,
					"%s: fail to set charging cur(%d)\n",
					__func__, ret);
				return ret;
			}
#if defined(CONFIG_MACH_Q1_BD)
			/* Reset charging */
		} else
			val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
#endif

		/*Reset charging start time only in initial charging start */
		if (info->charging_start_time == 0) {
			info->charging_start_time = jiffies;
			info->charging_next_time = RESETTING_CHG_TIME;
		}
	} else {		/* Disable charging */
		val_type.intval = POWER_SUPPLY_STATUS_DISCHARGING;
		ret = psy_main->set_property(psy_main, POWER_SUPPLY_PROP_STATUS,
					     &val_type);
		if (ret) {
			dev_err(info->dev, "%s: fail to set charging"
				" status-main(%d)\n", __func__, ret);
			return ret;
		}
		info->charging_start_time = 0;
		info->charging_passed_time = 0;
		info->charging_next_time = 0;
	}

	ret = psy_sub->set_property(psy_sub, POWER_SUPPLY_PROP_STATUS,
				    &val_type);
	if (ret) {
		dev_err(info->dev, "%s: fail to set charging status(%d)\n",
			__func__, ret);
		return ret;
	}

	return 0;
}

static int sec_bat_enable_charging(struct sec_bat_info *info, bool enable)
{
	if (enable && (info->batt_health != POWER_SUPPLY_HEALTH_GOOD)) {
		info->charging_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		dev_info(info->dev, "%s: Battery is NOT good!!!\n", __func__);
		return -EPERM;
	}

	if (info->use_sub_charger)
		return sec_bat_enable_charging_sub(info, enable);
	else
		return sec_bat_enable_charging_main(info, enable);
}

static void sec_bat_cable_work(struct work_struct *work)
{
	struct sec_bat_info *info = container_of(work, struct sec_bat_info,
						 cable_work);

	switch (info->cable_type) {
	case CABLE_TYPE_NONE:
		if (!sec_bat_enable_charging(info, false)) {
			info->batt_full_status = BATT_NOT_FULL;
			info->recharging_status = false;
			info->charging_status = POWER_SUPPLY_STATUS_DISCHARGING;
			info->batt_health = POWER_SUPPLY_HEALTH_GOOD;
			info->batt_temp_high_cnt = 0;
			info->batt_temp_low_cnt = 0;
			info->batt_temp_recover_cnt = 0;
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
			info->batt_temp_ext = BATT_TEMP_EXT_NONE;
			info->batt_temp_ext_pre = BATT_TEMP_EXT_NONE;
#endif
			wake_lock_timeout(&info->vbus_wake_lock, HZ * 5);
		}
		break;
	case CABLE_TYPE_USB:
	case CABLE_TYPE_AC:
	case CABLE_TYPE_MISC:
		if (!sec_bat_enable_charging(info, true)) {
			info->charging_status = POWER_SUPPLY_STATUS_CHARGING;
			wake_lock(&info->vbus_wake_lock);
		}
		break;
	default:
		dev_err(info->dev, "%s: Invalid cable type\n", __func__);
		break;
	}

	power_supply_changed(&info->psy_ac);
	power_supply_changed(&info->psy_usb);

	wake_unlock(&info->cable_wake_lock);
}

static bool sec_bat_charging_time_management(struct sec_bat_info *info)
{
	if (info->charging_start_time == 0) {
		dev_info(info->dev,
			"%s: charging_start_time has never been used since initializing\n",
			__func__);
		return false;
	}

	if (jiffies >= info->charging_start_time)
		info->charging_passed_time =
		    jiffies - info->charging_start_time;
	else
		info->charging_passed_time =
		    0xFFFFFFFF - info->charging_start_time + jiffies;

	switch (info->charging_status) {
	case POWER_SUPPLY_STATUS_FULL:
		if (info->recharging_status == true &&
		    time_after(info->charging_passed_time,
			       (unsigned long)RECHARGING_TIME)) {
			if (!sec_bat_enable_charging(info, false)) {
				info->recharging_status = false;
				dev_info(info->dev,
					 "%s: Recharging timer expired\n",
					 __func__);
				return true;
			}
		}
#ifndef SEC_BATTERY_1ST_2ND_TOPOFF
		break;
#endif
	case POWER_SUPPLY_STATUS_CHARGING:
#if defined(CONFIG_MACH_Q1_BD)
		if (info->cable_type != CABLE_TYPE_USB) {
#endif
			if (time_after(info->charging_passed_time,
				       (unsigned long)FULL_CHARGING_TIME)) {
				if (!sec_bat_enable_charging(info, false)) {
					info->charging_status =
					    POWER_SUPPLY_STATUS_FULL;
					info->batt_full_status = BATT_FULL;
					info->recharging_status = false;

					dev_info(info->dev,
						 "%s: Charging timer expired\n",
						 __func__);
					return true;
				}
			}
#if defined(CONFIG_MACH_Q1_BD)
		}
#endif
		if (time_after(info->charging_passed_time,
			       info->charging_next_time)) {
			/*reset current in charging status */
			if (!sec_bat_enable_charging(info, true)) {
				info->charging_next_time =
				    info->charging_passed_time +
				    RESETTING_CHG_TIME;

				dev_info(info->dev,
					 "%s: Reset charging current\n",
					 __func__);
			}
		}
		break;
	default:
		dev_info(info->dev, "%s: Undefine Battery Status\n", __func__);
		return false;
	}
#ifndef PRODUCT_SHIP
	dev_info(info->dev, "Time past : %u secs\n",
		 jiffies_to_msecs(info->charging_passed_time) / 1000);
#endif

	return false;
}

#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
static void sec_bat_check_vf_adc(struct sec_bat_info *info)
{
	int adc;
	static int invalid_vf_count;

	if (system_rev == 11)
		return;

	mutex_lock(&info->adclock);
	adc = sec_bat_get_adc_data(info, info->vf_adc_channel);
	mutex_unlock(&info->adclock);

	if (adc <= 0)
		adc = info->batt_vf_adc;

	info->batt_vf_adc = adc;

	if ((info->batt_vf_adc > MAX_VF || info->batt_vf_adc < MIN_VF)
	    && ((info->cable_type == CABLE_TYPE_AC)
		|| info->cable_type == CABLE_TYPE_USB)) {
		invalid_vf_count++;

		printk(KERN_DEBUG
		       "[%s] invalid VF is detected... (vf = %d, count = %d)\n",
		       __func__, info->batt_vf_adc, invalid_vf_count);

		if (invalid_vf_count >= VF_COUNT) {
			printk(KERN_DEBUG
			       "[%s] invalid VF is detected over %d times and now power off.\n",
			       __func__, invalid_vf_count);
			if (pm_power_off)
				pm_power_off();
		}
	} else {
		invalid_vf_count = 0;
	}

	return;
}
#endif

static void sec_bat_check_vf(struct sec_bat_info *info)
{
	struct power_supply *psy = get_power_supply_by_name(info->charger_name);
	union power_supply_propval value;
	int ret;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get %s ps\n",
			__func__, info->charger_name);
		return;
	}

#if defined(CONFIG_MACH_Q1_BD)
	if (system_rev <= HWREV_FOR_BATTERY) {
		int adc;

		mutex_lock(&info->adclock);
		adc = sec_bat_get_adc_data(info, ADC_CH_VF);
		mutex_unlock(&info->adclock);

		dev_info(info->dev, "%s: adc (%d/%d)\n", __func__, adc,
			 HWREV_FOR_BATTERY);

		if (adc > MAX_VF_ADC || adc < MIN_VF_ADC)
			value.intval = BAT_NOT_DETECTED;
		else
			value.intval = BAT_DETECTED;
	} else {
#endif
		ret = psy->get_property(psy, POWER_SUPPLY_PROP_PRESENT, &value);

		if (ret < 0) {
			dev_err(info->dev, "%s: fail to get status(%d)\n",
				__func__, ret);
			return;
		}
#if defined(CONFIG_MACH_Q1_BD)
	}
#endif

	if (value.intval == BAT_NOT_DETECTED) {
		if (info->present_count < BAT_DET_COUNT)
			info->present_count++;
		else {
			info->present = BAT_NOT_DETECTED;
			if (info->batt_health == POWER_SUPPLY_HEALTH_GOOD)
				info->batt_health =
				    POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		}
	} else {
		info->present = BAT_DETECTED;
		if ((info->batt_health == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) ||
		    (info->batt_health == POWER_SUPPLY_HEALTH_UNKNOWN))
			info->batt_health = POWER_SUPPLY_HEALTH_GOOD;
		info->present_count = 0;
	}

	dev_info(info->dev, "%s: Battery Health (%d)\n",
		 __func__, info->batt_health);
	return;
}

#if (defined(CONFIG_MACH_Q1_BD) && defined(CONFIG_SMB328_CHARGER))
static void sec_bat_check_ovp(struct sec_bat_info *info)
{
	struct power_supply *psy =
	    get_power_supply_by_name(info->sub_charger_name);
	union power_supply_propval value;
	int ret;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get %s ps\n",
			__func__, info->sub_charger_name);
		return;
	}

	ret = psy->get_property(psy, POWER_SUPPLY_PROP_HEALTH, &value);

	if (ret < 0) {
		dev_err(info->dev, "%s: fail to get health(%d)\n",
			__func__, ret);
		return;
	}

	if (value.intval == POWER_SUPPLY_HEALTH_OVERVOLTAGE)
		info->batt_health = value.intval;
}
#endif

static bool sec_bat_check_ing_level_trigger(struct sec_bat_info *info)
{
	struct power_supply *psy;
	union power_supply_propval value;
	int ret;

	if (info->use_sub_charger && info->sub_charger_name) {
		psy = get_power_supply_by_name(info->sub_charger_name);

		if (!psy) {
			dev_err(info->dev, "%s: fail to get %s ps\n",
				__func__, info->sub_charger_name);
			return false;
		}

		ret = psy->get_property(psy, POWER_SUPPLY_PROP_STATUS, &value);

		if (ret < 0) {
			dev_err(info->dev, "%s: fail to get status(%d)\n",
				__func__, ret);
			return false;
		}

		switch (info->charging_status) {
		case POWER_SUPPLY_STATUS_FULL:
			if (info->recharging_status == false)
				break;
		case POWER_SUPPLY_STATUS_CHARGING:
			switch (value.intval) {
#ifdef SEC_BATTERY_TOPOFF_BY_CHARGER
			case POWER_SUPPLY_STATUS_FULL:
				/* top-off by full charging */
				if (info->batt_vcell >=
				    FULL_CHARGE_COND_VOLTAGE) {
					sec_bat_handle_charger_topoff(info);
					dev_info(info->dev,
						 "%s : top-off by full charging\n",
						 __func__);
					return true;
				} else {
					/*reactivate charging in */
					/*next monitor work */
					/*for abnormal */
					/*full-charged status */
					info->charging_next_time =
					    info->charging_passed_time + HZ;
				}
				break;
#endif
#if defined(CONFIG_MACH_Q1_BD)
			case POWER_SUPPLY_STATUS_NOT_CHARGING:
				ret =
				    psy->get_property(psy,
						      POWER_SUPPLY_PROP_HEALTH,
						      &value);

				if (ret < 0) {
					dev_err(info->dev,
						"%s: fail to get health(%d)\n",
						__func__, ret);
					return false;
				}
				info->batt_health = value.intval;
#else
			case POWER_SUPPLY_STATUS_DISCHARGING:
				if (info->cable_type != CABLE_TYPE_NONE)
					info->batt_health =
					    POWER_SUPPLY_HEALTH_OVERVOLTAGE;
				break;
			case POWER_SUPPLY_STATUS_NOT_CHARGING:
#if defined(CONFIG_MACH_U1_NA_SPR_EPIC2_REV00)
				if (info->cable_type == CABLE_TYPE_USB ||
					info->cable_type == CABLE_TYPE_MISC ||
					info->cable_type == CABLE_TYPE_AC) {
#else
				if (info->cable_type == CABLE_TYPE_USB ||
					info->cable_type == CABLE_TYPE_MISC) {
#endif
					if (info->batt_vcell >=
					    FULL_CHARGE_COND_VOLTAGE) {
						/* USB full charged */
						info->charging_int_full_count++;
						if (info->
						    charging_int_full_count >=
						    FULL_CHG_COND_COUNT) {
							info->
						charging_int_full_count
							    = 0;
						sec_bat_handle_charger_topoff
							    (info);
							return true;
						}
						dev_info(info->dev,
							 "%s : full interrupt cnt = %d\n",
							 __func__,
							 info->
						 charging_int_full_count);
					} else {
						info->charging_int_full_count =
						    0;
						/*reactivate charging in */
						/*next monitor work */
						/*for abnormal */
						/*full-charged status */
						info->charging_next_time =
						    info->charging_passed_time +
						    HZ;
					}
				}
				break;
#endif
			}
			break;
		case POWER_SUPPLY_STATUS_NOT_CHARGING:
#if defined(CONFIG_MACH_Q1_BD)
			if (info->batt_health ==
			    POWER_SUPPLY_HEALTH_OVERVOLTAGE) {
				ret =
				    psy->get_property(psy,
						      POWER_SUPPLY_PROP_HEALTH,
						      &value);

				if (ret < 0) {
					dev_err(info->dev,
						"%s: fail to get health(%d)\n",
						__func__, ret);
					return false;
				}
				info->batt_health = value.intval;
			}
#else
			if (info->batt_health == POWER_SUPPLY_HEALTH_OVERVOLTAGE
			    && value.intval == POWER_SUPPLY_STATUS_NOT_CHARGING)
				info->batt_health = POWER_SUPPLY_HEALTH_GOOD;
#endif
			break;
		default:
			info->charging_int_full_count = 0;
			break;
		}
		return false;
	} else {
		psy = get_power_supply_by_name(info->charger_name);

		if (!psy) {
			dev_err(info->dev, "%s: fail to get %s ps\n",
				__func__, info->charger_name);
			return false;
		}

		ret = psy->get_property(psy, POWER_SUPPLY_PROP_STATUS, &value);

		if (ret < 0) {
			dev_err(info->dev, "%s: fail to get status(%d)\n",
				__func__, ret);
			return false;
		}

		switch (info->charging_status) {
		case POWER_SUPPLY_STATUS_FULL:
			if (info->recharging_status == false)
				break;
		case POWER_SUPPLY_STATUS_CHARGING:
			switch (value.intval) {
			case POWER_SUPPLY_STATUS_DISCHARGING:
				if (info->cable_type == CABLE_TYPE_USB ||
				    info->cable_type == CABLE_TYPE_MISC) {
					if (info->batt_vcell >=
					    FULL_CHARGE_COND_VOLTAGE) {
						/*USB full charged */
						info->charging_int_full_count++;
						if (info->
						    charging_int_full_count >=
						    FULL_CHG_COND_COUNT) {
							info->
						charging_int_full_count
							    = 0;
						sec_bat_handle_charger_topoff
							    (info);
							return true;
						}
						dev_info(info->dev,
							 "%s : full interrupt cnt = %d\n",
							 __func__,
							 info->
						charging_int_full_count);
					} else {
						info->charging_int_full_count =
						    0;
						/*reactivate charging in */
						/*next monitor work */
						/*for abnormal
						full-charged status */
						info->charging_next_time =
						    info->charging_passed_time +
						    HZ;
					}
				}
				break;
			}
			break;
		default:
			info->charging_int_full_count = 0;
			break;
		}
		return false;
	}
}

static void sec_bat_monitor_work(struct work_struct *work)
{
	struct sec_bat_info *info = container_of(work, struct sec_bat_info,
						 monitor_work);

	sec_bat_check_temper(info);
#ifndef SEC_BATTERY_INDEPEDENT_VF_CHECK
	sec_bat_check_vf(info);
#endif
	sec_bat_update_info(info);
#ifdef SPRINT_SLATE_TEST
	if (info->charging_status != POWER_SUPPLY_STATUS_FULL &&
	    info->batt_health == POWER_SUPPLY_HEALTH_GOOD)
		if (info->batt_temp_recover_cnt == 0
		    || info->batt_temp_recover_cnt > 3)
			sec_bat_get_dcinovp(info);
#endif
	if (sec_bat_charging_time_management(info))
		goto full_charged;

	if (sec_bat_check_ing_level_trigger(info))
		goto full_charged;

#if !defined(CONFIG_MACH_Q1_BD)
	if (info->cable_type == CABLE_TYPE_AC)
#endif
		if (sec_check_chgcurrent(info))
			goto full_charged;

	if (info->cable_type == CABLE_TYPE_NONE &&
	    info->charging_status == POWER_SUPPLY_STATUS_FULL) {
		dev_err(info->dev, "invalid full state\n");
		info->charging_status = POWER_SUPPLY_STATUS_DISCHARGING;
	}

	if (info->test_value) {
		switch (info->test_value) {
		case 1:	/*full status */
			info->charging_status = POWER_SUPPLY_STATUS_FULL;
			break;
		case 2:	/*low temperature */
			info->batt_health = POWER_SUPPLY_HEALTH_COLD;
			break;
		case 3:	/*high temperature */
			info->batt_health = POWER_SUPPLY_HEALTH_OVERHEAT;
			break;
		case 4:	/*over voltage */
			info->batt_health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
			break;
		case 5:	/*abnormal battery */
			info->batt_health = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
			break;
		case 6:	/*tmu tripped */
			info->batt_tmu_status = TMU_STATUS_TRIPPED;
			break;
		}
	}

	switch (info->charging_status) {
	case POWER_SUPPLY_STATUS_FULL:

#ifdef CONFIG_TARGET_LOCALE_NA
		if (info->batt_vcell < RECHARGING_VOLTAGE &&
		    info->recharging_status == false) {
			info->recharging_int_threshold_count++;
			if (info->recharging_int_threshold_count
			    >= RECHARGING_CND_COUNT) {
				if (!sec_bat_enable_charging(info, true)) {
					info->recharging_int_threshold_count =
					    0;
					info->recharging_status = true;

					dev_info(info->dev,
						 "%s: Start Recharging, Vcell = %d\n",
						 __func__, info->batt_vcell);
				}
			}
		} else {
			info->recharging_int_threshold_count = 0;
		}
#else
		if (info->batt_vcell < RECHARGING_VOLTAGE &&
		    info->recharging_status == false) {
			if (!sec_bat_enable_charging(info, true)) {
				info->recharging_status = true;

				dev_info(info->dev,
					 "%s: Start Recharging, Vcell = %d\n",
					 __func__, info->batt_vcell);
			}
		}
#endif
		break;
	case POWER_SUPPLY_STATUS_CHARGING:
		switch (info->batt_health) {
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
		case POWER_SUPPLY_HEALTH_GOOD:
			if (info->batt_temp_ext_pre != info->batt_temp_ext) {
				sec_bat_enable_charging(info, false);

				if (!sec_bat_enable_charging(info, true)) {
					dev_info(info->dev,
						 "%s: Changed charging current\n",
						 __func__);
				}
			}
			break;
#endif
		case POWER_SUPPLY_HEALTH_OVERHEAT:
		case POWER_SUPPLY_HEALTH_COLD:
		case POWER_SUPPLY_HEALTH_OVERVOLTAGE:
		case POWER_SUPPLY_HEALTH_DEAD:
		case POWER_SUPPLY_HEALTH_UNSPEC_FAILURE:
			if (!sec_bat_enable_charging(info, false)) {
				info->charging_status =
				    POWER_SUPPLY_STATUS_NOT_CHARGING;
				info->recharging_status = false;

				dev_info(info->dev, "%s: Not charging\n",
					 __func__);
			}
			break;
		default:
			break;
		}
		break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
		dev_dbg(info->dev, "%s: Discharging\n", __func__);
		break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		if (info->batt_health == POWER_SUPPLY_HEALTH_GOOD) {
			dev_info(info->dev, "%s: recover health state\n",
				 __func__);
			if (info->cable_type != CABLE_TYPE_NONE) {
				if (!sec_bat_enable_charging(info, true))
					info->charging_status =
					    POWER_SUPPLY_STATUS_CHARGING;
			} else
				info->charging_status
				    = POWER_SUPPLY_STATUS_DISCHARGING;
		}
		break;
	default:
		dev_info(info->dev, "%s: Undefined Battery Status\n", __func__);
		return;
	}

 full_charged:
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	dev_info(info->dev,
		 "soc(%d), vfocv(%d), vcell(%d), temp(%d), charging(%d), health(%d), vf(%d)\n",
		 info->batt_soc, info->batt_vfocv, info->batt_vcell / 1000,
		 info->batt_temp / 10, info->charging_status, info->batt_health,
		 info->batt_vf_adc);
#else
#ifndef PRODUCT_SHIP
	dev_info(info->dev,
		 "soc(%d), vfocv(%d), vcell(%d), temp(%d), charging(%d), health(%d), chg_adc(%d)\n",
		 info->batt_soc, info->batt_vfocv, info->batt_vcell / 1000,
		 info->batt_temp / 10, info->charging_status,
		 info->batt_health, info->batt_current_adc);
#endif
#endif

	power_supply_changed(&info->psy_bat);

	wake_unlock(&info->monitor_wake_lock);

	return;
}

#ifdef SEC_BATTERY_INDEPEDENT_VF_CHECK
static void sec_bat_vf_check_work(struct work_struct *work)
{
	struct sec_bat_info *info;
	info = container_of(work, struct sec_bat_info, vf_check_work.work);

#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	sec_bat_check_vf_adc(info);
#else
	sec_bat_check_vf(info);
#endif

#if (defined(CONFIG_MACH_Q1_BD) && defined(CONFIG_SMB328_CHARGER))
	sec_bat_check_ovp(info);

	if ((info->batt_health == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) ||
	    (info->batt_health == POWER_SUPPLY_HEALTH_OVERVOLTAGE)) {
		dev_info(info->dev,
			 "%s: Battery Disconnected or OVP activated\n",
			 __func__);
#else
	if (info->batt_health == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
		dev_info(info->dev, "%s: Battery Disconnected\n", __func__);
#endif
		wake_lock(&info->monitor_wake_lock);
		queue_work(info->monitor_wqueue, &info->monitor_work);
	}

	schedule_delayed_work(&info->vf_check_work,
			      msecs_to_jiffies(VF_CHECK_INTERVAL));
}
#endif

static void sec_bat_polling_work(struct work_struct *work)
{
	struct sec_bat_info *info;
	info = container_of(work, struct sec_bat_info, polling_work.work);

	wake_lock(&info->monitor_wake_lock);
	queue_work(info->monitor_wqueue, &info->monitor_work);

	if (info->initial_check_count) {
		schedule_delayed_work(&info->polling_work, HZ);
		info->initial_check_count--;
	} else
		schedule_delayed_work(&info->polling_work,
				      msecs_to_jiffies(info->polling_interval));
}

#define SEC_BATTERY_ATTR(_name)			\
{						\
	.attr = { .name = #_name,		\
		  .mode = 0664 },		\
	.show = sec_bat_show_property,		\
	.store = sec_bat_store,			\
}

static struct device_attribute sec_battery_attrs[] = {
	SEC_BATTERY_ATTR(batt_vol),
	SEC_BATTERY_ATTR(batt_soc),
	SEC_BATTERY_ATTR(batt_vfocv),
	SEC_BATTERY_ATTR(batt_temp),
	SEC_BATTERY_ATTR(batt_temp_adc),
#ifdef CONFIG_TARGET_LOCALE_NA
	SEC_BATTERY_ATTR(batt_temp_radc),
#endif
	SEC_BATTERY_ATTR(batt_temp_adc_avg),
	SEC_BATTERY_ATTR(charging_source),
	SEC_BATTERY_ATTR(batt_lp_charging),
	SEC_BATTERY_ATTR(video),
	SEC_BATTERY_ATTR(mp3),
	SEC_BATTERY_ATTR(batt_type),
	SEC_BATTERY_ATTR(batt_full_check),
	SEC_BATTERY_ATTR(batt_temp_check),
	SEC_BATTERY_ATTR(batt_temp_adc_spec),
	SEC_BATTERY_ATTR(batt_test_value),
	SEC_BATTERY_ATTR(batt_current_now),
	SEC_BATTERY_ATTR(batt_current_adc),
	SEC_BATTERY_ATTR(siop_activated),
	SEC_BATTERY_ATTR(system_rev),
#ifdef CONFIG_TARGET_LOCALE_NA
	SEC_BATTERY_ATTR(fg_soc),
#else
	SEC_BATTERY_ATTR(fg_psoc),
#endif				/* CONFIG_TARGET_LOCALE_NA */
	SEC_BATTERY_ATTR(batt_lpm_state),
	SEC_BATTERY_ATTR(batt_tmu_status),
#ifdef SPRINT_SLATE_TEST
	SEC_BATTERY_ATTR(slate_test_mode),
#endif
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	SEC_BATTERY_ATTR(batt_v_f_adc),
	SEC_BATTERY_ATTR(batt_voice_call_2g),
	SEC_BATTERY_ATTR(batt_voice_call_3g),
	SEC_BATTERY_ATTR(batt_data_call),
	SEC_BATTERY_ATTR(wifi),
	SEC_BATTERY_ATTR(batt_gps),
	SEC_BATTERY_ATTR(camera),
	SEC_BATTERY_ATTR(recording),
#elif defined(CONFIG_TARGET_LOCALE_NA)
	SEC_BATTERY_ATTR(call),
	/*SEC_BATTERY_ATTR(video),*/
	/*SEC_BATTERY_ATTR(music),*/
	SEC_BATTERY_ATTR(browser),
	SEC_BATTERY_ATTR(hotspot),
	SEC_BATTERY_ATTR(camera),
	SEC_BATTERY_ATTR(data_call),
	SEC_BATTERY_ATTR(wimax),
#endif
};

enum {
	BATT_VOL = 0,
	BATT_SOC,
	BATT_VFOCV,
	BATT_TEMP,
	BATT_TEMP_ADC,
#ifdef CONFIG_TARGET_LOCALE_NA
	BATT_TEMP_RADC,
#endif
	BATT_TEMP_ADC_AVG,
	CHARGING_SOURCE,
	BATT_LP_CHARGING,
	BATT_VIDEO,
	BATT_MP3,
	BATT_TYPE,
	BATT_FULL_CHECK,
	BATT_TEMP_CHECK,
	BATT_TEMP_ADC_SPEC,
	BATT_TEST_VALUE,
	BATT_CURRENT_NOW,
	BATT_CURRENT_ADC,
	BATT_SIOP_ACTIVATED,
	BATT_SYSTEM_REV,
	BATT_FG_PSOC,
	BATT_LPM_STATE,
	BATT_TMU_STATUS,
#ifdef SPRINT_SLATE_TEST
	SLATE_TEST_MODE,
#endif
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	BATT_V_F_ADC,
	BATT_VOICE_CALL_2G,
	BATT_VOICE_CALL_3G,
	BATT_DATA_CALL,
	BATT_WIFI,
	BATT_GPS,
	BATT_CAMERA,
	BATT_RECORDING,
#endif
#if defined(CONFIG_TARGET_LOCALE_NA)
	BATT_CALL,
	BATT_BROWSER,
	BATT_HOTSOPT,
	BATT_CAMERA,
	BATT_DATA_CALL,
	BATT_WIMAX,
#endif
};

#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
static void sec_bat_check_event_status(struct sec_bat_info *info, int mode,
				       int offset)
{
	int is_event_running = 0;

	if ((info->batt_event_status != 0)
	    /*&& (info->cable_type != CABLE_TYPE_NONE) */
	    )
		is_event_running = 1;

	if (mode) {
		if (!(info->batt_event_status & offset))
			info->batt_event_status |= offset;
	} else {
		if (info->batt_event_status & offset)
			info->batt_event_status &= ~offset;
	}

	printk(KERN_DEBUG "[%s] current batt_event_status = 0x%x\n", __func__,
	       info->batt_event_status);

	if ((info->batt_event_status == 0) && (is_event_running == 1))
		info->event_end_time = jiffies;

	return;
}
#elif defined(CONFIG_TARGET_LOCALE_NA)
static void sec_bat_check_event_status(struct sec_bat_info *info, int mode,
				       int offset)
{
	int is_event_running = 0;

	if ((info->batt_event_status != 0)
	    /*&& (info->cable_type != CABLE_TYPE_NONE) */
	    )
		is_event_running = 1;

	if (mode) {
		if (!(info->batt_event_status & offset))
			info->batt_event_status |= offset;
	} else {
		if (info->batt_event_status & offset)
			info->batt_event_status &= ~offset;
	}
#ifndef PRODUCT_SHIP
	printk(KERN_DEBUG "[%s] current batt_event_status = 0x%x\n", __func__,
	       info->batt_event_status);
#endif
	if ((info->batt_event_status == 0) && (is_event_running == 1))
		info->event_expired_time = jiffies;

	return;
}

int sec_bat_use_wimax(int onoff)
{
	struct sec_bat_info *info = pchg;
	sec_bat_check_event_status(info, onoff, USE_WIMAX);
	return 0;
}
EXPORT_SYMBOL(sec_bat_use_wimax);
#endif

static ssize_t sec_bat_show_property(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct sec_bat_info *info = dev_get_drvdata(dev->parent);
	int i = 0, val;
	const ptrdiff_t off = attr - sec_battery_attrs;

	switch (off) {
	case BATT_VOL:
		val = sec_bat_get_fuelgauge_data(info, FG_T_AVGVCELL);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_SOC:
		val = sec_bat_get_fuelgauge_data(info, FG_T_SOC);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_VFOCV:
		val = sec_bat_get_fuelgauge_data(info, FG_T_VFOCV);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_TEMP:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", info->batt_temp);
		break;
	case BATT_TEMP_ADC:
		val = s3c_read_temper_adc(info);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
#ifdef CONFIG_TARGET_LOCALE_NA
	case BATT_TEMP_RADC:
		val = s3c_read_temper_adc(info);
		val = sec_rescale_temp_adc(info);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->batt_temp_radc);
		break;
#endif
	case BATT_TEMP_ADC_AVG:
		val = info->temper_adc_sample.average_adc;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case CHARGING_SOURCE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->cable_type);
		break;
	case BATT_LP_CHARGING:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->get_lpcharging_state());
		break;
	case BATT_VIDEO:
		/* TODO */
#ifdef CONFIG_TARGET_LOCALE_NA
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", info->use_video);
#else
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
#endif
		break;
	case BATT_MP3:
		/* TODO */
#ifdef CONFIG_TARGET_LOCALE_NA
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", info->use_music);
#else
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
#endif
		break;
	case BATT_TYPE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", "SDI_SDI");
		break;
	case BATT_FULL_CHECK:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       (info->charging_status ==
				POWER_SUPPLY_STATUS_FULL)
			       ? 1 : 0);
		break;
	case BATT_TEMP_CHECK:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			       "%d\n", info->batt_health);
		break;
	case BATT_TEMP_ADC_SPEC:
#ifdef CONFIG_TARGET_LOCALE_NA
		i += scnprintf(buf + i, PAGE_SIZE - i,
			       "(HIGH: %d - %d,   LOW: %d - %d)\n",
			       HIGH_BLOCK_TEMP_ADC, HIGH_RECOVER_TEMP_ADC,
			       LOW_BLOCK_TEMP_ADC, LOW_RECOVER_TEMP_ADC);
#else
		i += scnprintf(buf + i, PAGE_SIZE - i,
			       "(HIGH: %d - %d,   LOW: %d - %d)\n",
			       HIGH_BLOCK_TEMP / 10, HIGH_RECOVER_TEMP / 10,
			       LOW_BLOCK_TEMP / 10, LOW_RECOVER_TEMP / 10);
#endif
		break;
	case BATT_TEST_VALUE:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			       "0-normal, 1-full, 2-low, 3-high, 4-over, 5-cf, 6-trip (%d)\n",
			       info->test_value);
		break;
	case BATT_CURRENT_NOW:
	case BATT_SIOP_ACTIVATED:
		{
			struct power_supply *psy =
			    get_power_supply_by_name(info->sub_charger_name);
			union power_supply_propval value;

			if (!psy) {
				pr_err("%s: fail to get battery ps\n",
				       __func__);
				return -ENODEV;
			}

			/* initialized as NOT charging */
			value.intval = 0;

			if ((info->charging_status ==
			     POWER_SUPPLY_STATUS_CHARGING)
			    || (info->charging_status ==
				POWER_SUPPLY_STATUS_FULL))
				psy->get_property(psy,
						  POWER_SUPPLY_PROP_CURRENT_NOW,
						  &value);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       value.intval);
		}
		break;
	case BATT_CURRENT_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->batt_current_adc);
		break;
	case BATT_SYSTEM_REV:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", system_rev);
		break;
	case BATT_FG_PSOC:
		val = sec_bat_get_fuelgauge_data(info, FG_T_PSOC);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_LPM_STATE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->batt_lpm_state);
		break;
	case BATT_TMU_STATUS:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->batt_tmu_status);
		break;
#ifdef SPRINT_SLATE_TEST
	case SLATE_TEST_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->slate_test_mode);
		break;
#endif
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	case BATT_V_F_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->batt_vf_adc);
		break;
	case BATT_VOICE_CALL_2G:
		/* TODO */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		break;
	case BATT_VOICE_CALL_3G:
		/* TODO */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		break;
	case BATT_DATA_CALL:
		/* TODO */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		break;
	case BATT_WIFI:
		/* TODO */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		break;
	case BATT_GPS:
		/* TODO */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		break;
	case BATT_CAMERA:
		/* TODO */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		break;
	case BATT_RECORDING:
		/* TODO */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		break;
#endif
#ifdef CONFIG_TARGET_LOCALE_NA
	case BATT_CALL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", info->use_call);
		break;
	case BATT_BROWSER:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			       "%d\n", info->use_browser);
		break;
	case BATT_HOTSOPT:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			       "%d\n", info->use_hotspot);
		break;
	case BATT_CAMERA:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			       "%d\n", info->use_camera);
		break;
	case BATT_DATA_CALL:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			       "%d\n", info->use_data_call);
		break;
	case BATT_WIMAX:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", info->use_wimax);
		break;
#endif
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

static ssize_t sec_bat_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int ret = 0, x = 0;
	const ptrdiff_t off = attr - sec_battery_attrs;
	struct sec_bat_info *info = dev_get_drvdata(dev->parent);

	switch (off) {
	case BATT_CURRENT_NOW:
		if (sscanf(buf, "%d\n", &x) == 1) {
			ret = count;
			if ((info->charging_status ==
			     POWER_SUPPLY_STATUS_CHARGING)
			    || (info->charging_status ==
				POWER_SUPPLY_STATUS_FULL)) {
				struct power_supply *psy =
				    get_power_supply_by_name(info->
							     sub_charger_name);
				union power_supply_propval value;

				if (!psy) {
					pr_err("%s: fail to get battery ps\n",
					       __func__);
					return -ENODEV;
				}

				value.intval = x;	/* charging current */
				psy->set_property(psy,
						  POWER_SUPPLY_PROP_CURRENT_NOW,
						  &value);

				/* reflect changing current */
				value.intval = true;
				psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,
						  &value);
			} else
				pr_err("%s: NOT charging status\n", __func__);
		}
		break;
	case BATT_SIOP_ACTIVATED:
		if (sscanf(buf, "%d\n", &x) == 1) {
			ret = count;
			if ((info->charging_status ==
			     POWER_SUPPLY_STATUS_CHARGING) ||
			    (info->charging_status ==
			     POWER_SUPPLY_STATUS_FULL)) {
				struct power_supply *psy =
				    get_power_supply_by_name(info->
							     sub_charger_name);
				union power_supply_propval value;

				if (!psy) {
					pr_err("%s: fail to get battery ps\n",
					       __func__);
					return -ENODEV;
				}

				if (x) {
					/* charging current for SIOP */
					value.intval = 450;
				} else {
					switch (info->cable_type) {
					case CABLE_TYPE_USB:
					case CABLE_TYPE_MISC:
						value.intval = 450;	/* mA */
						break;
					case CABLE_TYPE_AC:
						value.intval = 650;	/* mA */
						break;
					default:
						dev_err(info->dev,
							"%s: Invalid func use\n",
							__func__);
						return -EINVAL;
					}
				}
				psy->set_property(psy,
						  POWER_SUPPLY_PROP_CURRENT_NOW,
						  &value);

				/* reflect changing current */
				value.intval = true;
				psy->set_property(psy,
						  POWER_SUPPLY_PROP_ONLINE,
						  &value);
			} else
				pr_err("%s: NOT charging status\n", __func__);
		}
		break;
	case BATT_VIDEO:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			dev_info(info->dev, "%s: video(%d)\n", __func__, x);
			ret = count;
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
			sec_bat_check_event_status(info, x, OFFSET_VIDEO_PLAY);
#elif defined(CONFIG_TARGET_LOCALE_NA)
			info->use_video = x;
			sec_bat_check_event_status(info, x, USE_VIDEO);
#endif
		}
		break;
	case BATT_MP3:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			dev_info(info->dev, "%s: mp3(%d)\n", __func__, x);
			ret = count;
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
			sec_bat_check_event_status(info, x, OFFSET_MP3_PLAY);
#elif defined(CONFIG_TARGET_LOCALE_NA)
			info->use_music = x;
			sec_bat_check_event_status(info, x, USE_MUSIC);
#endif
		}
		break;
	case BATT_TEST_VALUE:
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->test_value = x;
			printk(KERN_DEBUG "%s : test case : %d\n", __func__,
			       info->test_value);
			ret = count;
		}
		break;
	case BATT_LPM_STATE:
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->batt_lpm_state = x;
			ret = count;
		}
		break;
#ifdef SPRINT_SLATE_TEST
	case SLATE_TEST_MODE:
		if (strncmp(buf, "1", 1) == 0) {
			info->slate_test_mode = true;
			ret = count;
		} else {
			info->slate_test_mode = false;
			ret = count;
		}
		break;
#endif
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	case BATT_VOICE_CALL_2G:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			dev_info(info->dev, "%s: call 2G(%d)\n", __func__, x);
			ret = count;
			sec_bat_check_event_status(info, x,
						   OFFSET_VOICE_CALL_2G);
		}
		break;
	case BATT_VOICE_CALL_3G:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			dev_info(info->dev, "%s: call 3G(%d)\n", __func__, x);
			ret = count;
			sec_bat_check_event_status(info, x,
						   OFFSET_VOICE_CALL_3G);
		}
		break;
	case BATT_DATA_CALL:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			dev_info(info->dev, "%s: data call(%d)\n", __func__, x);
			ret = count;
			sec_bat_check_event_status(info, x, OFFSET_DATA_CALL);
		}
		break;
	case BATT_WIFI:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			dev_info(info->dev, "%s: wifi(%d)\n", __func__, x);
			ret = count;
			sec_bat_check_event_status(info, x, OFFSET_WIFI);
		}
		break;
	case BATT_GPS:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			dev_info(info->dev, "%s: gps(%d)\n", __func__, x);
			ret = count;
			sec_bat_check_event_status(info, x, OFFSET_GPS);
		}
		break;
	case BATT_CAMERA:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			dev_info(info->dev, "%s: camera(%d)\n", __func__, x);
			ret = count;
			sec_bat_check_event_status(info, x, OFFSET_CAMERA_ON);
		}
		break;
	case BATT_RECORDING:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			dev_info(info->dev, "%s: recording(%d)\n", __func__, x);
			ret = count;
			sec_bat_check_event_status(info, x,
						   OFFSET_RECORDING_ON);
		}
		break;
#endif
#ifdef CONFIG_TARGET_LOCALE_NA
	case BATT_CALL:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->use_call = x;
			dev_info(info->dev, "[NA_SPR]%s: call (%d)\n", __func__,
				 x);
			ret = count;
			sec_bat_check_event_status(info, x, USE_CALL);
		}
		break;
	case BATT_BROWSER:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->use_browser = x;
			dev_info(info->dev, "[NA_SPR]%s: BROWSER(%d)\n",
				 __func__, x);
			ret = count;
			sec_bat_check_event_status(info, x, USE_BROWSER);
		}
		break;
	case BATT_HOTSOPT:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->use_hotspot = x;
			dev_info(info->dev, "[NA_SPR]%s: HOTSPOT(%d)\n",
				 __func__, x);
			ret = count;
			sec_bat_check_event_status(info, x, USE_HOTSPOT);
		}
		break;
	case BATT_CAMERA:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->use_camera = x;
			dev_info(info->dev, "[NA_SPR]%s: CAMERA(%d)\n",
				 __func__, x);
			ret = count;
			sec_bat_check_event_status(info, x, USE_CAMERA);
		}
		break;
	case BATT_DATA_CALL:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->use_data_call = x;
			dev_info(info->dev, "[NA_SPR]%s: DATA CALL(%d)\n",
				 __func__, x);
			ret = count;
			sec_bat_check_event_status(info, x, USE_DATA_CALL);
		}
		break;
	case BATT_WIMAX:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->use_wimax = x;
			dev_info(info->dev, "[NA_SPR]%s: WIMAX(%d)\n", __func__,
				 x);
			ret = count;
			sec_bat_check_event_status(info, x, USE_WIMAX);
		}
		break;
#endif
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int sec_bat_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(sec_battery_attrs); i++) {
		rc = device_create_file(dev, &sec_battery_attrs[i]);
		if (rc)
			goto sec_attrs_failed;
	}
	goto succeed;

 sec_attrs_failed:
	while (i--)
		device_remove_file(dev, &sec_battery_attrs[i]);
 succeed:
	return rc;
}

static int sec_bat_is_charging(struct sec_bat_info *info)
{
	struct power_supply *psy = get_power_supply_by_name(info->charger_name);
	union power_supply_propval value;
	int ret;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return -ENODEV;
	}

	ret = psy->get_property(psy, POWER_SUPPLY_PROP_STATUS, &value);
	if (ret < 0) {
		dev_err(info->dev, "%s: fail to get status(%d)\n", __func__,
			ret);
		return ret;
	}

	return value.intval;
}

static int sec_bat_read_proc(char *buf, char **start,
			     off_t offset, int count, int *eof, void *data)
{
	struct sec_bat_info *info = data;
	struct timespec cur_time;
	ktime_t ktime;
	int len = 0;

	ktime = alarm_get_elapsed_realtime();
	cur_time = ktime_to_timespec(ktime);

#ifdef CONFIG_TARGET_LOCALE_NA
	len = sprintf(buf,
		      "%lu, %u, %u, %u, %u, %u, %d, %d, %d, %u, %u, %u, %u, %u, %u, %u, %d, %lu\n",
		      cur_time.tv_sec,
		      info->batt_raw_soc,
		      info->batt_soc,
		      info->batt_vfocv,
		      info->batt_vcell,
		      info->batt_current_adc,
		      info->batt_full_status,
		      info->charging_int_full_count,
		      info->charging_adc_full_count,
		      info->recharging_status,
		      info->batt_temp_adc,
		      info->batt_temp_radc,
		      info->batt_health,
		      info->charging_status,
		      info->batt_tmu_status,
		      info->present,
		      info->cable_type, info->charging_passed_time);

	return len;
}
#else
	len = sprintf(buf,
		      "%lu, %u, %u, %u, %u, %u, %d, %d, %d, %u, %u, %u, %u, %u, %u, %d, %lu\n",
		      cur_time.tv_sec,
		      info->batt_raw_soc,
		      info->batt_soc,
		      info->batt_vfocv,
		      info->batt_vcell,
		      info->batt_current_adc,
		      info->batt_full_status,
		      info->charging_int_full_count,
		      info->charging_adc_full_count,
		      info->recharging_status,
		      info->batt_temp_adc,
		      info->batt_health,
		      info->charging_status,
		      info->batt_tmu_status,
		      info->present,
		      info->cable_type, info->charging_passed_time);

	return len;
}
#endif

static __devinit int sec_bat_probe(struct platform_device *pdev)
{
	struct sec_bat_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct sec_bat_info *info;
	struct power_supply *psy;
	union power_supply_propval value;
	int ret = 0;

	dev_info(&pdev->dev, "%s: SEC Battery Driver Loading\n", __func__);

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	platform_set_drvdata(pdev, info);

	info->dev = &pdev->dev;
	if (!pdata->fuel_gauge_name || !pdata->charger_name) {
		dev_err(info->dev, "%s: no fuel gauge or charger name\n",
			__func__);
		goto err_kfree;
	}
	info->fuel_gauge_name = pdata->fuel_gauge_name;
	info->charger_name = pdata->charger_name;
	info->sub_charger_name = pdata->sub_charger_name;
#if defined(CONFIG_MACH_U1_NA_SPR_EPIC2_REV00)
	if (system_rev <= HWREV_FOR_BATTERY) {
#else
	if (system_rev >= HWREV_FOR_BATTERY) {
#endif
		dev_info(&pdev->dev, "%s: use sub-charger (%s)\n",
			 __func__, info->sub_charger_name);
		info->adc_arr_size = pdata->adc_sub_arr_size;
		info->adc_table = pdata->adc_sub_table;
		info->adc_channel = pdata->adc_sub_channel;
		info->use_sub_charger = true;
	} else {
		dev_info(&pdev->dev, "%s: use main charger (%s)\n",
			 __func__, info->charger_name);
		info->adc_arr_size = pdata->adc_arr_size;
		info->adc_table = pdata->adc_table;
		info->adc_channel = pdata->adc_channel;
		info->use_sub_charger = false;
	}
	info->get_lpcharging_state = pdata->get_lpcharging_state;
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	info->vf_adc_channel = pdata->adc_vf_channel;
#endif

	info->psy_bat.name = "battery",
	    info->psy_bat.type = POWER_SUPPLY_TYPE_BATTERY,
	    info->psy_bat.properties = sec_battery_props,
	    info->psy_bat.num_properties = ARRAY_SIZE(sec_battery_props),
	    info->psy_bat.get_property = sec_bat_get_property,
	    info->psy_bat.set_property = sec_bat_set_property,
	    info->psy_usb.name = "usb",
	    info->psy_usb.type = POWER_SUPPLY_TYPE_USB,
	    info->psy_usb.supplied_to = supply_list,
	    info->psy_usb.num_supplicants = ARRAY_SIZE(supply_list),
	    info->psy_usb.properties = sec_power_props,
	    info->psy_usb.num_properties = ARRAY_SIZE(sec_power_props),
	    info->psy_usb.get_property = sec_usb_get_property,
	    info->psy_ac.name = "ac",
	    info->psy_ac.type = POWER_SUPPLY_TYPE_MAINS,
	    info->psy_ac.supplied_to = supply_list,
	    info->psy_ac.num_supplicants = ARRAY_SIZE(supply_list),
	    info->psy_ac.properties = sec_power_props,
	    info->psy_ac.num_properties = ARRAY_SIZE(sec_power_props),
	    info->psy_ac.get_property = sec_ac_get_property;

	wake_lock_init(&info->vbus_wake_lock, WAKE_LOCK_SUSPEND,
		       "vbus_present");
	wake_lock_init(&info->monitor_wake_lock, WAKE_LOCK_SUSPEND,
		       "sec-battery-monitor");
	wake_lock_init(&info->cable_wake_lock, WAKE_LOCK_SUSPEND,
		       "sec-battery-cable");

	psy = get_power_supply_by_name(info->charger_name);

	if (!psy) {
		dev_err(info->dev, "%s: fail to get charger\n", __func__);
		return -ENODEV;
	}

	ret = psy->get_property(psy, POWER_SUPPLY_PROP_PRESENT, &value);

	if (ret < 0) {
		dev_err(info->dev, "%s: fail to get status(%d)\n",
			__func__, ret);
		return -ENODEV;
	}
	info->present = value.intval;

	if (info->present == BAT_NOT_DETECTED)
		info->batt_health = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
	else
		info->batt_health = POWER_SUPPLY_HEALTH_GOOD;

#if defined(CONFIG_MACH_Q1_BD)
	info->charging_status = POWER_SUPPLY_STATUS_DISCHARGING;
#else
	info->charging_status = sec_bat_is_charging(info);
	if (info->charging_status < 0)
		info->charging_status = POWER_SUPPLY_STATUS_DISCHARGING;
#endif

	info->present_count = 0;
	info->charging_int_full_count = 0;
	info->charging_adc_full_count = 0;
#ifdef CONFIG_TARGET_LOCALE_NA
	info->recharging_int_threshold_count = 0;
#endif
	info->batt_temp_high_cnt = 0;
	info->batt_temp_low_cnt = 0;
	info->batt_temp_recover_cnt = 0;

	info->initial_check_count = INIT_CHECK_COUNT;

	mutex_init(&info->adclock);

	info->padc = s3c_adc_register(pdev, NULL, NULL, 0);
	info->charging_start_time = 0;

#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	info->batt_vf_adc = 0;
	info->batt_event_status = 0;
	info->event_end_time = 0xFFFFFFFF;
#elif defined(CONFIG_TARGET_LOCALE_NA)
	info->use_call = 0;
	info->use_video = 0;
	info->use_music = 0;
	info->use_browser = 0;
	info->use_hotspot = 0;
	info->use_camera = 0;
	info->use_data_call = 0;
	info->use_wimax = 0;
	info->batt_event_status = 0;
	info->event_expired_time = 0xFFFFFFFF;
#endif
	if (info->get_lpcharging_state) {
		if (info->get_lpcharging_state())
			info->polling_interval = POLLING_INTERVAL / 4;
		else
			info->polling_interval = POLLING_INTERVAL;
	}

	info->test_value = 0;

	info->batt_tmu_status = TMU_STATUS_NORMAL;

	/* init power supplier framework */
	ret = power_supply_register(&pdev->dev, &info->psy_bat);
	if (ret) {
		dev_err(info->dev, "%s: failed to register psy_bat\n",
			__func__);
		goto err_wake_lock;
	}

	ret = power_supply_register(&pdev->dev, &info->psy_usb);
	if (ret) {
		dev_err(info->dev, "%s: failed to register psy_usb\n",
			__func__);
		goto err_supply_unreg_bat;
	}

	ret = power_supply_register(&pdev->dev, &info->psy_ac);
	if (ret) {
		dev_err(info->dev, "%s: failed to register psy_ac\n", __func__);
		goto err_supply_unreg_usb;
	}

	/* create sec detail attributes */
	sec_bat_create_attrs(info->psy_bat.dev);

	info->entry = create_proc_entry("batt_info_proc", S_IRUGO, NULL);
	if (!info->entry)
		dev_err(info->dev, "%s: failed to create proc_entry\n",
			__func__);
	else {
		info->entry->read_proc = sec_bat_read_proc;
		info->entry->data = (struct sec_bat_info *)info;
	}

	info->monitor_wqueue = create_freezable_workqueue(dev_name(&pdev->dev));
	if (!info->monitor_wqueue) {
		dev_err(info->dev, "%s: fail to create workqueue\n", __func__);
		goto err_supply_unreg_ac;
	}

	INIT_WORK(&info->monitor_work, sec_bat_monitor_work);
	INIT_WORK(&info->cable_work, sec_bat_cable_work);

	INIT_DELAYED_WORK_DEFERRABLE(&info->polling_work, sec_bat_polling_work);
	schedule_delayed_work(&info->polling_work, 0);

#ifdef CONFIG_TARGET_LOCALE_NA
	pchg = info;		/* using pointer - for wimax */
#endif

#ifdef SEC_BATTERY_INDEPEDENT_VF_CHECK
	INIT_DELAYED_WORK_DEFERRABLE(&info->vf_check_work,
				     sec_bat_vf_check_work);
	schedule_delayed_work(&info->vf_check_work, 0);
#endif

#if defined(CONFIG_MACH_Q1_BD)
	if (pdata->initial_check)
		pdata->initial_check();
#endif

	dev_info(info->dev, "%s: SEC Battery Driver Loaded\n", __func__);
	return 0;

 err_supply_unreg_ac:
	power_supply_unregister(&info->psy_ac);
 err_supply_unreg_usb:
	power_supply_unregister(&info->psy_usb);
 err_supply_unreg_bat:
	power_supply_unregister(&info->psy_bat);
 err_wake_lock:
	wake_lock_destroy(&info->vbus_wake_lock);
	wake_lock_destroy(&info->monitor_wake_lock);
	wake_lock_destroy(&info->cable_wake_lock);
	s3c_adc_release(info->padc);
	mutex_destroy(&info->adclock);
 err_kfree:
	kfree(info);

	return ret;
}

static int __devexit sec_bat_remove(struct platform_device *pdev)
{
	struct sec_bat_info *info = platform_get_drvdata(pdev);

	remove_proc_entry("batt_info_proc", NULL);

	flush_workqueue(info->monitor_wqueue);
	destroy_workqueue(info->monitor_wqueue);

	cancel_delayed_work(&info->polling_work);
#ifdef SEC_BATTERY_INDEPEDENT_VF_CHECK
	cancel_delayed_work(&info->vf_check_work);
#endif

	power_supply_unregister(&info->psy_bat);
	power_supply_unregister(&info->psy_usb);
	power_supply_unregister(&info->psy_ac);

	wake_lock_destroy(&info->vbus_wake_lock);
	wake_lock_destroy(&info->monitor_wake_lock);
	wake_lock_destroy(&info->cable_wake_lock);
	mutex_destroy(&info->adclock);

	s3c_adc_release(info->padc);

	kfree(info);

	return 0;
}

static int sec_bat_suspend(struct device *dev)
{
	struct sec_bat_info *info = dev_get_drvdata(dev);

	cancel_work_sync(&info->monitor_work);
	cancel_delayed_work(&info->polling_work);
#ifdef SEC_BATTERY_INDEPEDENT_VF_CHECK
	cancel_delayed_work(&info->vf_check_work);
#endif

	return 0;
}

static int sec_bat_resume(struct device *dev)
{
	struct sec_bat_info *info = dev_get_drvdata(dev);

	wake_lock(&info->monitor_wake_lock);
	queue_work(info->monitor_wqueue, &info->monitor_work);

	schedule_delayed_work(&info->polling_work,
			      msecs_to_jiffies(info->polling_interval));

#ifdef SEC_BATTERY_INDEPEDENT_VF_CHECK
	schedule_delayed_work(&info->vf_check_work,
			      msecs_to_jiffies(VF_CHECK_INTERVAL));
#endif

	return 0;
}

#if defined(CONFIG_TARGET_LOCALE_NA)
static void sec_bat_shutdown(struct device *dev)
{
	struct sec_bat_info *info = dev_get_drvdata(dev);
	struct power_supply *psy_main, *psy_sub;
	union power_supply_propval val_type;
	int ret;

	if (!info->use_sub_charger)
		return;

	psy_main = get_power_supply_by_name(info->charger_name);
	psy_sub = get_power_supply_by_name(info->sub_charger_name);

	if (!psy_main || !psy_sub) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return;
	}

	val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
	ret =
	    psy_main->set_property(psy_main, POWER_SUPPLY_PROP_STATUS,
				   &val_type);
	if (ret) {
		dev_err(info->dev, "%s: fail to set charging status(%d)\n",
			__func__, ret);
		return;
	}

	val_type.intval = POWER_SUPPLY_STATUS_DISCHARGING;
	ret =
	    psy_sub->set_property(psy_sub, POWER_SUPPLY_PROP_STATUS, &val_type);
	if (ret) {
		dev_err(info->dev, "%s: fail to set charging status(%d)\n",
			__func__, ret);
		return;
	}
}
#elif defined(CONFIG_MACH_Q1_BD)
static void sec_bat_shutdown(struct device *dev)
{
	struct sec_bat_info *info = dev_get_drvdata(dev);
#if defined(CONFIG_SMB328_CHARGER)
	struct power_supply *psy_sub =
	    get_power_supply_by_name("smb328-charger");
	union power_supply_propval value;
	int ret;

	if (!psy_sub) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return;
	}

	/* only for OTG */
	value.intval = false;
	ret = psy_sub->set_property(psy_sub,
				    POWER_SUPPLY_PROP_CHARGE_TYPE, &value);
	if (ret) {
		dev_info(info->dev, "%s: fail to set OTG (%d)\n",
			 __func__, ret);
		return;
	}
#endif
}
#else
#define sec_bat_shutdown	NULL
#endif

static const struct dev_pm_ops sec_bat_pm_ops = {
	.suspend = sec_bat_suspend,
	.resume = sec_bat_resume,
};

static struct platform_driver sec_bat_driver = {
	.driver = {
		   .name = "sec-battery",
		   .owner = THIS_MODULE,
		   .pm = &sec_bat_pm_ops,
		   .shutdown = sec_bat_shutdown,
		   },
	.probe = sec_bat_probe,
	.remove = __devexit_p(sec_bat_remove),
};

static int __init sec_bat_init(void)
{
	return platform_driver_register(&sec_bat_driver);
}

static void __exit sec_bat_exit(void)
{
	platform_driver_unregister(&sec_bat_driver);
}

late_initcall(sec_bat_init);
module_exit(sec_bat_exit);

MODULE_DESCRIPTION("SEC battery driver");
MODULE_AUTHOR("<ms925.kim@samsung.com>");
MODULE_AUTHOR("<joshua.chang@samsung.com>");
MODULE_LICENSE("GPL");
