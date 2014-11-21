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

#define POLLING_INTERVAL	(40 * 1000)
#define MEASURE_DSG_INTERVAL	(20 * 1000)
#define MEASURE_CHG_INTERVAL	(3 * 1000)
#define FULL_CHARGING_TIME	(6 * 60 * 60 * HZ)	/* 6hr */
#define RECHARGING_TIME		(2 * 60 * 60 * HZ)	/* 2hr */
#define RESETTING_CHG_TIME		(10 * 60 * HZ)	/* 10Min */
#define RECHARGING_VOLTAGE	(4130 * 1000)		/* 4.13 V */
#if defined(CONFIG_MACH_U1_KOR_LGT)
#define HIGH_BLOCK_TEMP_ADC		395
#define HIGH_RECOVER_TEMP_ADC	345
#define LOW_BLOCK_TEMP_ADC		253
#define LOW_RECOVER_TEMP_ADC	258
#else
#define HIGH_BLOCK_TEMP_ADC		390
#define HIGH_RECOVER_TEMP_ADC	348
#define LOW_BLOCK_TEMP_ADC		247
#define LOW_RECOVER_TEMP_ADC	256
#endif

#define FG_T_SOC		0
#define FG_T_VCELL		1
#define FG_T_TEMPER		2
#define FG_T_PSOC		3
#define FG_T_VFOCV		4
#define FG_T_AVGVCELL	5
#define FG_T_FSOC		6

#define ADC_SAMPLING_CNT	6
#define ADC_CH_CHGCURRENT_MAIN		0
#define ADC_CH_CHGCURRENT_SUB		1
#define ADC_CH_TEMPERATURE_MAIN		7
#define ADC_CH_TEMPERATURE_SUB		6 /* 6 is near AP side */
/* #define ADC_TOTAL_COUNT		5 */

#define CURRENT_OF_FULL_CHG_MAIN	300
#define CURRENT_OF_FULL_CHG_SUB		500
/* all count duration = (count - 1) * poll interval */
#define RE_CHG_COND_COUNT		4
#define RE_CHG_MIN_COUNT		2
#define TEMP_BLOCK_COUNT		3
#define BAT_DET_COUNT			2
#define FULL_CHG_COND_COUNT		2
#define OVP_COND_COUNT			2
#define USB_FULL_COND_COUNT		3
#define USB_FULL_COND_VOLTAGE    4180000
#define FULL_CHARGE_COND_VOLTAGE    4100000
#define INIT_CHECK_COUNT	4
#define CALL_EXCEPTION_VOLTAGE_UP    3400000
#define CALL_EXCEPTION_VOLTAGE_DN    3200000

/* for backup
extern int get_proximity_activation_state(void);
extern int get_proximity_approach_state(void);
*/

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

static ssize_t sec_bat_show_property(struct device *dev,
				     struct device_attribute *attr, char *buf);

static ssize_t sec_bat_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count);

struct battest_info {
	int rechg_count;
	int full_count;
	int full_count_sub;
	int test_value;
	int test_esuspend;
	bool is_rechg_state;
};

/*
struct adc_sample {
	int average_adc;
	int adc_arr[ADC_TOTAL_COUNT];
	int index;
};
*/

struct sec_bat_info {
	struct device *dev;

	char *fuel_gauge_name;
	char *charger_name;
	char *sub_charger_name;

	unsigned int adc_arr_size;
	struct sec_bat_adc_table_data *adc_table;
	/*
	unsigned int adc_channel;
	struct adc_sample temper_adc_sample;
	*/

	struct power_supply psy_bat;
	struct power_supply psy_usb;
	struct power_supply psy_ac;

	struct wake_lock vbus_wake_lock;
	struct wake_lock monitor_wake_lock;
	struct wake_lock cable_wake_lock;
	struct wake_lock test_wake_lock;
	struct wake_lock measure_wake_lock;

	enum cable_type_t cable_type;
	enum batt_full_t batt_full_status;

	unsigned int batt_temp;	/* Battery Temperature (C) */
	int batt_temp_high_cnt;
	int batt_temp_low_cnt;
	unsigned int batt_health;
	unsigned int batt_vcell;
	unsigned int batt_vfocv;
	unsigned int batt_soc;
	unsigned int batt_raw_soc;
	unsigned int batt_full_soc;
	unsigned int batt_presoc;
	unsigned int polling_interval;
	unsigned int measure_interval;
	int charging_status;
	/* int charging_full_count; */

	unsigned int batt_temp_adc;
	unsigned int batt_temp_adc_sub;
	unsigned int batt_temp_radc;
	unsigned int batt_temp_radc_sub;
	unsigned int batt_current_adc;
	unsigned int present;
	struct battest_info	test_info;

	struct s3c_adc_client *padc;

	struct workqueue_struct *monitor_wqueue;
	struct work_struct monitor_work;
	struct delayed_work	cable_work;
	struct delayed_work polling_work;
	struct delayed_work measure_work;

	unsigned long charging_start_time;
	unsigned long charging_passed_time;
	unsigned long next_check_time;
	unsigned int recharging_status;
	unsigned int batt_lpm_state;
	unsigned int sub_chg_status;
	unsigned int voice_call_state;
	unsigned int is_call_except;
	unsigned int charging_set_current;

	struct mutex adclock;

	unsigned int (*get_lpcharging_state) (void);
	bool charging_enabled;
	bool is_timeout_chgstop;
	bool use_sub_charger;
	bool sub_chg_ovp;
	int initial_check_count;
	struct proc_dir_entry *entry;

	int batt_tmu_status;
};

static char *supply_list[] = {
	"battery",
};

static enum power_supply_property sec_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TECHNOLOGY,
};

static enum power_supply_property sec_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

struct power_supply *get_power_supply_by_name(char *name)
{
	if (!name)
		return (struct power_supply *)NULL;
	else
		return power_supply_get_by_name(name);
}

#if 0
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
#endif

static int sec_bat_check_vf(struct sec_bat_info *info)
{
	int health = info->batt_health;

	if (info->present == 0) {
		if (info->test_info.test_value == 999) {
			pr_info("test case : %d\n", info->test_info.test_value);
			health = POWER_SUPPLY_HEALTH_UNKNOWN;
		} else
			health = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
	} else {
		health = POWER_SUPPLY_HEALTH_GOOD;
	}

	/* update health */
	if (health != info->batt_health) {
		if (health == POWER_SUPPLY_HEALTH_UNKNOWN ||
			health == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE){
			info->batt_health = health;
			pr_info("vf error update\n");
		} else if (info->batt_health != POWER_SUPPLY_HEALTH_OVERHEAT &&
			info->batt_health != POWER_SUPPLY_HEALTH_COLD &&
			health == POWER_SUPPLY_HEALTH_GOOD) {
			info->batt_health = health;
			pr_info("recovery form vf error\n");
		}
	}

	return 0;
}

static int sec_bat_check_detbat(struct sec_bat_info *info)
{
	struct power_supply *psy = get_power_supply_by_name(info->charger_name);
	union power_supply_propval value;
	static int cnt;
	int vf_state = BAT_DETECTED;
	int ret;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return -ENODEV;
	}

	ret = psy->get_property(psy, POWER_SUPPLY_PROP_PRESENT, &value);
	if (ret < 0) {
		dev_err(info->dev, "%s: fail to get status(%d)\n",
			__func__, ret);
		return -ENODEV;
	}

	if ((info->cable_type != CABLE_TYPE_NONE) &&
		(value.intval == BAT_NOT_DETECTED)) {
		if (cnt <= BAT_DET_COUNT)
			cnt++;
		if (cnt >= BAT_DET_COUNT)
			vf_state = BAT_NOT_DETECTED;
		else
			vf_state = BAT_DETECTED;
	} else {
		vf_state = BAT_DETECTED;
		cnt = 0;
	}

	if (info->present == 1 &&
		vf_state == BAT_NOT_DETECTED) {
		pr_info("detbat state(->%d) changed\n",	vf_state);
		info->present = 0;
		cancel_work_sync(&info->monitor_work);
		wake_lock(&info->monitor_wake_lock);
		queue_work(info->monitor_wqueue, &info->monitor_work);
	} else if (info->present == 0 &&
		vf_state == BAT_DETECTED) {
		pr_info("detbat state(->%d) changed\n", vf_state);
		info->present = 1;
		cancel_work_sync(&info->monitor_work);
		wake_lock(&info->monitor_wake_lock);
		queue_work(info->monitor_wqueue, &info->monitor_work);
	}

	return value.intval;
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
	case FG_T_FSOC:
		value.intval = 2;	/*full soc */
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
		if (info->test_info.test_value == 999) {
			pr_info("batt test case : %d\n",
					info->test_info.test_value);
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		} else if (info->is_timeout_chgstop &&
			info->charging_status == POWER_SUPPLY_STATUS_FULL &&
			info->batt_soc != 100) {
			/* new concept : in case of time-out charging stop,
			Do not update FULL for UI except soc 100%,
			Use same time-out value for first charing and
			re-charging
			*/
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		} else
			val->intval = info->charging_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = info->batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = info->present;
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
	case POWER_SUPPLY_PROP_CAPACITY:
		if (!info->is_timeout_chgstop &&
			info->charging_status == POWER_SUPPLY_STATUS_FULL) {
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
	default:
		return -EINVAL;
	}
	return 0;
}

static int sec_bat_handle_sub_charger_topoff(struct sec_bat_info *info)
{
	struct power_supply *psy_sub =
	    get_power_supply_by_name(info->sub_charger_name);
	union power_supply_propval value;
	int ret = 0;

	if (!psy_sub) {
		dev_err(info->dev, "%s: fail to get sub charger ps\n",
					__func__);
		return -ENODEV;
	}

	if (info->batt_full_status == BATT_NOT_FULL) {
		info->charging_status = POWER_SUPPLY_STATUS_FULL;
		info->batt_full_status = BATT_FULL;
		info->recharging_status = false;
		info->charging_passed_time = 0;
		info->charging_start_time = 0;
		/* disable charging */
		value.intval = POWER_SUPPLY_STATUS_DISCHARGING;
		ret = psy_sub->set_property(psy_sub, POWER_SUPPLY_PROP_STATUS,
			&value);
		info->charging_enabled = false;
	}
	return ret;
}

static int sec_bat_set_property(struct power_supply *ps,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sec_bat_info *info = container_of(ps, struct sec_bat_info,
						 psy_bat);
	struct power_supply *psy = get_power_supply_by_name(info->charger_name);
	union power_supply_propval value;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		dev_info(info->dev, "%s: topoff intr\n", __func__);
		if (val->intval != POWER_SUPPLY_STATUS_FULL)
			return -EINVAL;

		if (info->use_sub_charger) {
			if (info->cable_type == CABLE_TYPE_USB ||
					info->cable_type == CABLE_TYPE_MISC)
				sec_bat_handle_sub_charger_topoff(info);
			break;
		}

		if (info->batt_full_status == BATT_NOT_FULL) {
			info->recharging_status = false;
			info->batt_full_status = BATT_FULL;
			info->charging_status = POWER_SUPPLY_STATUS_FULL;
			/* disable charging */
			value.intval = POWER_SUPPLY_STATUS_DISCHARGING;
			psy->set_property(psy, POWER_SUPPLY_PROP_STATUS,
					  &value);
			info->charging_enabled = false;
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
		/* TODO : fix delay time */
		queue_delayed_work(info->monitor_wqueue, &info->cable_work, 0);
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
			pr_err("err(%d) returned, skip adc read\n", adc_data);
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

static inline int s3c_read_temper_adc(struct sec_bat_info *info)
{
	int adc;

	mutex_lock(&info->adclock);
	adc = sec_bat_get_adc_data(info, ADC_CH_TEMPERATURE_MAIN);
	mutex_unlock(&info->adclock);
	if (adc <= 0)
		adc = info->batt_temp_adc;
	info->batt_temp_adc = adc;

	return adc;
}

static inline int s3c_read_temper_adc_sub(struct sec_bat_info *info)
{
	int adc;
	int adc_tmp1 = 0;
	int adc_tmp2 = 0;
	unsigned int temp_radc_sub = 0;

	mutex_lock(&info->adclock);
	adc = sec_bat_get_adc_data(info, ADC_CH_TEMPERATURE_SUB);
	mutex_unlock(&info->adclock);
	if (adc <= 0)
		adc = info->batt_temp_adc_sub;
	info->batt_temp_adc_sub = adc;

	temp_radc_sub = info->batt_temp_adc_sub;
	adc_tmp1 = temp_radc_sub * 10;
	adc_tmp2 = (40950 - adc_tmp1);
	temp_radc_sub = adc_tmp2 / 100;
	if ((adc_tmp2 % 10) >= 5)
		temp_radc_sub += 1;

	info->batt_temp_radc_sub = temp_radc_sub;
	/* pr_info("[battery] lcd temper : %d, rescale : %d\n",
			info->batt_temp_adc_sub, temp_radc_sub); */

	return adc;
}

static unsigned long sec_rescale_temp_adc(struct sec_bat_info *info)
{
	int adc_tmp = info->batt_temp_adc;
	int adc_tmp1 = 0;
	int adc_tmp2 = 0;

	adc_tmp1 = adc_tmp * 10;
	adc_tmp2 = (40950 - adc_tmp1);
	adc_tmp = adc_tmp2 / 100;
	if ((adc_tmp2 % 10) >= 5)
		adc_tmp += 1;

	info->batt_temp_radc = adc_tmp;

	/* pr_info("[battery] bat temper : %d, rescale : %d\n",
			info->batt_temp_adc, adc_tmp); */

	return adc_tmp;
}

static int sec_bat_check_temper(struct sec_bat_info *info)
{
	struct power_supply *psy
	    = get_power_supply_by_name(info->fuel_gauge_name);
	union power_supply_propval value;
	int ret;
	int temp;
	int low = 0;
	int high = 0;
	int mid = 0;

	if (!info->adc_table || !info->adc_arr_size) {
		/* using fake temp */
		temp = 300;
		info->batt_temp = temp;
		goto skip_tupdate;
	}

	high = info->adc_arr_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (info->adc_table[mid].adc > info->batt_temp_adc)
			high = mid - 1;
		else if (info->adc_table[mid].adc < info->batt_temp_adc)
			low = mid + 1;
		else
			break;
	}
	temp = info->adc_table[mid].temperature;
	/* pr_info("%s : temperature form adc table : %d\n", __func__, temp); */
	info->batt_temp = temp;

skip_tupdate:
	/* Set temperature to fuel gauge */
	if (info->fuel_gauge_name) {
		value.intval = info->batt_temp / 10;
		/* value.intval = temp / 10; */
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_TEMP, &value);
		if (ret) {
			dev_err(info->dev, "%s: fail to set temperature(%d)\n",
				__func__, ret);
			return ret;
		}
	}

	return 0;
}

static int sec_bat_check_temper_adc(struct sec_bat_info *info)
{
	int temp_adc = s3c_read_temper_adc(info);
	int rescale_adc = 0;
	int health = info->batt_health;

	rescale_adc = sec_rescale_temp_adc(info);

	if (info->test_info.test_value == 1) {
		pr_info("test case : %d\n", info->test_info.test_value);
		rescale_adc = HIGH_BLOCK_TEMP_ADC + 1;
		if (info->cable_type == CABLE_TYPE_NONE)
			rescale_adc = HIGH_RECOVER_TEMP_ADC - 1;
		info->batt_temp_radc = rescale_adc;
	}

	if (info->cable_type == CABLE_TYPE_NONE ||
		info->test_info.test_value == 999) {
		info->batt_temp_high_cnt = 0;
		info->batt_temp_low_cnt = 0;
		health = POWER_SUPPLY_HEALTH_GOOD;
		goto skip_hupdate;
	}

	if (rescale_adc >= HIGH_BLOCK_TEMP_ADC) {
		if (health != POWER_SUPPLY_HEALTH_OVERHEAT)
			if (info->batt_temp_high_cnt <= TEMP_BLOCK_COUNT)
				info->batt_temp_high_cnt++;
	} else if (rescale_adc <= HIGH_RECOVER_TEMP_ADC &&
		rescale_adc >= LOW_RECOVER_TEMP_ADC) {
		if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
		    health == POWER_SUPPLY_HEALTH_COLD) {
			info->batt_temp_high_cnt = 0;
			info->batt_temp_low_cnt = 0;
		}
	} else if (rescale_adc <= LOW_BLOCK_TEMP_ADC) {
		if (health != POWER_SUPPLY_HEALTH_COLD)
			if (info->batt_temp_low_cnt <= TEMP_BLOCK_COUNT)
				info->batt_temp_low_cnt++;
	}

	if (info->batt_temp_high_cnt >= TEMP_BLOCK_COUNT)
		health = POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (info->batt_temp_low_cnt >= TEMP_BLOCK_COUNT)
		health = POWER_SUPPLY_HEALTH_COLD;
	else
		health = POWER_SUPPLY_HEALTH_GOOD;

skip_hupdate:
	if (info->batt_health != POWER_SUPPLY_HEALTH_UNKNOWN &&
		info->batt_health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE &&
		health != info->batt_health) {
		info->batt_health = health;
		cancel_work_sync(&info->monitor_work);
		wake_lock(&info->monitor_wake_lock);
		queue_work(info->monitor_wqueue, &info->monitor_work);
	}

	return 0;
}



static void check_chgcurrent_sub(struct sec_bat_info *info)
{
	int chg_current_adc = 0;

	mutex_lock(&info->adclock);
	chg_current_adc = sec_bat_get_adc_data(info, ADC_CH_CHGCURRENT_SUB);
	mutex_unlock(&info->adclock);
	if (chg_current_adc < 0)
		chg_current_adc = info->batt_current_adc;
	info->batt_current_adc = chg_current_adc;

	dev_dbg(info->dev,
		"[battery] charging current = %d\n", info->batt_current_adc);
}

/* only for sub-charger */
static void sec_check_chgcurrent(struct sec_bat_info *info)
{
	static int cnt;

	if (info->charging_enabled && info->cable_type == CABLE_TYPE_AC) {
		check_chgcurrent_sub(info);
		/* AGAIN_FEATURE */
		if (info->batt_current_adc <= CURRENT_OF_FULL_CHG_SUB)
			check_chgcurrent_sub(info);

		/* if (info->test_info.test_value == 2)
		{
			pr_info("batt test case : %d\n",
						info->test_info.test_value);
			info->batt_current_adc = CURRENT_OF_FULL_CHG_SUB - 1;
			cnt = FULL_CHG_COND_COUNT;
		} else */
		if (info->test_info.test_value == 3) {
			pr_info("batt test case : %d\n",
						info->test_info.test_value);
			info->batt_current_adc = CURRENT_OF_FULL_CHG_SUB + 1;
			cnt = 0;
		}

		if (info->batt_vcell >= FULL_CHARGE_COND_VOLTAGE) {
			if (info->batt_current_adc <= CURRENT_OF_FULL_CHG_SUB) {
				cnt++;
				pr_info("full state? %d, %d\n",
						info->batt_current_adc, cnt);
				if (cnt >= FULL_CHG_COND_COUNT) {
					pr_info("full state!! %d/%d\n",
						cnt, FULL_CHG_COND_COUNT);
					sec_bat_handle_sub_charger_topoff(info);
					cnt = 0;
				}
			}
		} else
			cnt = 0;
	} else {
		cnt = 0;
		info->batt_current_adc = 0;
	}
	info->test_info.full_count = cnt;
}


static int sec_check_recharging(struct sec_bat_info *info)
{
	static int cnt;

	if (info->batt_vcell > RECHARGING_VOLTAGE) {
		cnt = 0;
		return 0;
	} else {
		/* AGAIN_FEATURE */
		info->batt_vcell = sec_bat_get_fuelgauge_data(info, FG_T_VCELL);
		if (info->batt_vcell <= RECHARGING_VOLTAGE) {
			cnt++;
			if (cnt >= RE_CHG_COND_COUNT) {
				cnt = 0;
				info->test_info.is_rechg_state = true;
				return 1;
			} else if (cnt >= RE_CHG_MIN_COUNT &&
				info->batt_vcell <= FULL_CHARGE_COND_VOLTAGE) {
				cnt = 0;
				info->test_info.is_rechg_state = true;
				return 1;
			} else
				return 0;
		} else {
			cnt = 0;
			return 0;
		}
	}
	info->test_info.rechg_count = cnt;
}

static void sec_bat_update_info(struct sec_bat_info *info)
{
	info->batt_presoc = info->batt_soc;
	info->batt_raw_soc = sec_bat_get_fuelgauge_data(info, FG_T_PSOC);
	info->batt_soc = sec_bat_get_fuelgauge_data(info, FG_T_SOC);
	info->batt_vcell = sec_bat_get_fuelgauge_data(info, FG_T_VCELL);
	info->batt_vfocv = sec_bat_get_fuelgauge_data(info, FG_T_VFOCV);
	info->batt_full_soc = sec_bat_get_fuelgauge_data(info, FG_T_FSOC);
	/* info->batt_temp = sec_bat_get_fuelgauge_data(info, FG_T_TEMPER); */
	/* pr_info("temperature form FG : %d\n", info->batt_temp); */
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
		/* mA */
		val_topoff.intval = 200;
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_CHARGE_FULL,
					&val_topoff);
		if (ret) {
			dev_err(info->dev, "%s: fail to set topoff cur(%d)\n",
				__func__, ret);
			return ret;
		}

		info->charging_start_time = jiffies;
	} else {		/* Disable charging */
		val_type.intval = POWER_SUPPLY_STATUS_DISCHARGING;
		info->charging_passed_time = 0;
		info->charging_start_time = 0;
	}

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_STATUS, &val_type);
	if (ret) {
		dev_err(info->dev, "%s: fail to set charging status(%d)\n",
			__func__, ret);
		return ret;
	}

	info->charging_enabled = enable;

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

		info->charging_set_current = val_chg_current.intval;

		/* Set charging current */
		ret = psy_sub->set_property(psy_sub,
					    POWER_SUPPLY_PROP_CURRENT_NOW,
					    &val_chg_current);
		if (ret) {
			dev_err(info->dev, "%s: fail to set charging cur(%d)\n",
				__func__, ret);
			return ret;
		}

		info->charging_start_time = jiffies;
		info->next_check_time =
			info->charging_start_time + RESETTING_CHG_TIME;
	} else {		/* Disable charging */
		val_type.intval = POWER_SUPPLY_STATUS_DISCHARGING;
		ret = psy_main->set_property(psy_main, POWER_SUPPLY_PROP_STATUS,
					     &val_type);
		if (ret) {
			dev_err(info->dev, "%s: fail to set charging"
				" status-main(%d)\n", __func__, ret);
			return ret;
		}
		info->next_check_time = 0;
		info->charging_passed_time = 0;
		info->charging_set_current = 0;
		info->charging_start_time = 0;
	}

	ret = psy_sub->set_property(psy_sub, POWER_SUPPLY_PROP_STATUS,
				    &val_type);
	if (ret) {
		dev_err(info->dev, "%s: fail to set charging status(%d)\n",
			__func__, ret);
		return ret;
	}

	info->charging_enabled = enable;

	return 0;
}

static int sec_bat_enable_charging(struct sec_bat_info *info, bool enable)
{
	if (info->use_sub_charger)
		return sec_bat_enable_charging_sub(info, enable);
	else
		return sec_bat_enable_charging_main(info, enable);
}

static void sec_bat_cable_work(struct work_struct *work)
{
	struct sec_bat_info *info = container_of(work, struct sec_bat_info,
							cable_work.work);

	switch (info->cable_type) {
	case CABLE_TYPE_NONE:
		/* TODO : check DCIN state again*/
		info->sub_chg_ovp = false;
		info->batt_full_status = BATT_NOT_FULL;
		info->recharging_status = false;
		info->test_info.is_rechg_state = false;
		info->charging_start_time = 0;
		info->charging_status = POWER_SUPPLY_STATUS_DISCHARGING;
		info->is_timeout_chgstop = false;
		sec_bat_enable_charging(info, false);
		wake_lock_timeout(&info->vbus_wake_lock, HZ * 5);
		cancel_delayed_work(&info->measure_work);
		info->measure_interval = MEASURE_DSG_INTERVAL;
		wake_lock(&info->measure_wake_lock);
		queue_delayed_work(info->monitor_wqueue,
				&info->measure_work, 0);
		/* schedule_delayed_work(&info->measure_work, 0); */
		break;
	case CABLE_TYPE_USB:
	case CABLE_TYPE_AC:
	case CABLE_TYPE_MISC:
		/* TODO : check DCIN state again*/
		info->charging_status = POWER_SUPPLY_STATUS_CHARGING;
		sec_bat_enable_charging(info, true);
		wake_lock(&info->vbus_wake_lock);
		cancel_delayed_work(&info->measure_work);
		info->measure_interval = MEASURE_CHG_INTERVAL;
		wake_lock(&info->measure_wake_lock);
		queue_delayed_work(info->monitor_wqueue,
				&info->measure_work, 0);
		/* schedule_delayed_work(&info->measure_work, 0); */
		break;
	default:
		dev_err(info->dev, "%s: Invalid cable type\n", __func__);
		break;
	}

	power_supply_changed(&info->psy_ac);
	power_supply_changed(&info->psy_usb);
	/* TBD */
	/*
	wake_lock(&info->monitor_wake_lock);
	queue_work(info->monitor_wqueue, &info->monitor_work);
	*/

	wake_unlock(&info->cable_wake_lock);
}

static void sec_bat_charging_time_management(struct sec_bat_info *info)
{
	unsigned long charging_time;

	if (info->charging_start_time == 0) {
		dev_dbg(info->dev, "%s: charging_start_time has never"
			" been used since initializing\n", __func__);
		return;
	}

	if (jiffies >= info->charging_start_time)
		charging_time = jiffies - info->charging_start_time;
	else
		charging_time = 0xFFFFFFFF - info->charging_start_time
		    + jiffies;

	info->charging_passed_time = charging_time;

	switch (info->charging_status) {
	case POWER_SUPPLY_STATUS_FULL:
		if (time_after(charging_time, (unsigned long)RECHARGING_TIME) &&
		    info->recharging_status == true) {
			sec_bat_enable_charging(info, false);
			info->recharging_status = false;
			info->is_timeout_chgstop = true;
			dev_info(info->dev, "%s: Recharging timer expired\n",
				 __func__);
		}
		break;
	case POWER_SUPPLY_STATUS_CHARGING:
		if (time_after(charging_time,
			       (unsigned long)FULL_CHARGING_TIME)) {
			sec_bat_enable_charging(info, false);
			info->charging_status = POWER_SUPPLY_STATUS_FULL;
			info->is_timeout_chgstop = true;

			dev_info(info->dev, "%s: Charging timer expired\n",
				 __func__);
		}
		break;
	default:
		dev_info(info->dev, "%s: Undefine Battery Status\n", __func__);
		info->is_timeout_chgstop = false;
		return;
	}

	dev_dbg(info->dev, "Time past : %u secs\n",
		jiffies_to_msecs(charging_time) / 1000);

	return;
}


static void sec_bat_check_ing_level_trigger(struct sec_bat_info *info)
{
	struct power_supply *psy_sub =
	    get_power_supply_by_name(info->sub_charger_name);
	union power_supply_propval value;
	union power_supply_propval val_type, val_chg_current;
	static int full_cnt;
	static int ovp_cnt;
	int ret;

	ret = psy_sub->get_property(psy_sub, POWER_SUPPLY_PROP_STATUS, &value);

	if (ret < 0) {
		dev_err(info->dev, "%s: fail to get status(%d)\n",
			__func__, ret);
		return;
	}

	info->sub_chg_status = value.intval;

	if (info->charging_enabled) {
		switch (value.intval) {
		case POWER_SUPPLY_STATUS_DISCHARGING: /* overvoltage */
			ovp_cnt++;
			if (ovp_cnt >= OVP_COND_COUNT) {
				pr_info("over/under voltage detected!! (%d)\n",
					ovp_cnt);
			    info->sub_chg_ovp = true;
				info->batt_full_status = BATT_NOT_FULL;
				info->recharging_status = false;
				info->test_info.is_rechg_state = false;
				info->charging_start_time = 0;
				info->charging_status =
					POWER_SUPPLY_STATUS_DISCHARGING;
				info->is_timeout_chgstop = false;
				sec_bat_enable_charging(info, false);
			}
			break;
		case POWER_SUPPLY_STATUS_NOT_CHARGING: /* full */
			if (info->cable_type == CABLE_TYPE_USB ||
					info->cable_type == CABLE_TYPE_MISC) {
				pr_info("usb full cnt = %d\n", full_cnt);
				full_cnt++;
				if (full_cnt >= USB_FULL_COND_COUNT &&
					info->batt_vcell >=
						USB_FULL_COND_VOLTAGE) {
					full_cnt = 0;
					sec_bat_handle_sub_charger_topoff(info);
				}
			} else {
				full_cnt = 0;
			}
			break;
		case POWER_SUPPLY_STATUS_CHARGING: /* charging */
			dev_dbg(info->dev, "%s : passed_time(%lu) > next_time(%lu)\n",
					__func__, info->charging_passed_time,
					info->next_check_time);
			if (time_after(info->charging_passed_time,
					info->next_check_time)) {
				info->next_check_time =
					info->charging_passed_time +
							RESETTING_CHG_TIME;
				if (info->is_call_except) {
					pr_info("call exception case, skip resetting\n");
					full_cnt = 0;
					ovp_cnt = 0;
					break;
				}

				if (info->cable_type == CABLE_TYPE_AC) {
					val_type.intval =
						POWER_SUPPLY_STATUS_CHARGING;
					val_chg_current.intval = 650;	/* mA */

					ret = psy_sub->set_property(psy_sub,
						POWER_SUPPLY_PROP_CURRENT_NOW,
						&val_chg_current);
					if (ret) {
						dev_err(info->dev, "%s: fail to set charging cur(%d)\n",
							__func__, ret);
					}

					ret = psy_sub->set_property(psy_sub,
						POWER_SUPPLY_PROP_STATUS,
						&val_type);
					if (ret) {
						dev_err(info->dev, "%s: fail to set charging status(%d)\n",
							__func__, ret);
					}

					dev_info(info->dev, "%s: reset charging current\n",
						 __func__);
				}
			}
			full_cnt = 0;
			ovp_cnt = 0;
			break;
		default:
			full_cnt = 0;
			ovp_cnt = 0;
		}
	} else {
		if (info->charging_status == POWER_SUPPLY_STATUS_DISCHARGING &&
				info->sub_chg_status ==
					POWER_SUPPLY_STATUS_NOT_CHARGING &&
				info->sub_chg_ovp == true) {
			pr_info("recover from ovp, charge again!!\n");
			info->sub_chg_ovp = false;
			info->charging_status = POWER_SUPPLY_STATUS_CHARGING;
			sec_bat_enable_charging(info, true);
		}
		full_cnt = 0;
		ovp_cnt = 0;
	}
}

static int sec_main_charger_disable(struct sec_bat_info *info)
{
	struct power_supply *psy_main =
		get_power_supply_by_name(info->charger_name);
	union power_supply_propval val_type;
	int ret;

	val_type.intval = POWER_SUPPLY_STATUS_DISCHARGING;
	ret = psy_main->set_property(psy_main, POWER_SUPPLY_PROP_STATUS,
								&val_type);
	if (ret) {
		dev_err(info->dev, "%s: fail to set charging"
			" status-main(%d)\n", __func__, ret);
	}

	return ret;
}

static int sec_main_charger_default(struct sec_bat_info *info)
{
	struct power_supply *psy = get_power_supply_by_name(info->charger_name);
	union power_supply_propval val_type, val_chg_current;
	int ret;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return -ENODEV;
	}

	val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
	val_chg_current.intval = 450;	/* mA */

	/* Set charging current */
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_CURRENT_NOW,
				&val_chg_current);
	if (ret) {
		dev_err(info->dev, "%s: fail to set charging cur(%d)\n",
			__func__, ret);
		return ret;
	}

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_STATUS, &val_type);
	if (ret) {
		dev_err(info->dev, "%s: fail to set charging status(%d)\n",
			__func__, ret);
		return ret;
	}

	return 0;
}

#if 0 /* for backup */
static void sec_batt_check_call_exception_test(struct sec_bat_info *info)
{
	struct power_supply *psy_sub =
	    get_power_supply_by_name(info->sub_charger_name);
	union power_supply_propval val_type, val_chg_current;
	int proximity_activation = 0;
	int proximity_approach = 0;
	int ret;

	if (info->cable_type != CABLE_TYPE_AC) {
		info->is_call_except = 0;
		return;
	}

	if (!info->charging_enabled) {
		if (info->is_call_except)
			info->is_call_except = 0;
		return;
	}

	if (!info->is_call_except)
		info->is_call_except = 1;
	else
		info->is_call_except = 0;

	val_type.intval = POWER_SUPPLY_STATUS_UNKNOWN;

	if (info->charging_set_current == 650 &&
		info->is_call_except == 1) {
		val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
		val_chg_current.intval = 450;
		info->charging_set_current = val_chg_current.intval;
		pr_info("call exception enable!\n");
	} else if (info->charging_set_current == 450 &&
		info->is_call_except == 0) {
		val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
		val_chg_current.intval = 650;
		info->charging_set_current = val_chg_current.intval;
		pr_info("call exception disable!\n");
	}

	if (val_type.intval == POWER_SUPPLY_STATUS_CHARGING) {
		ret = psy_sub->set_property(psy_sub,
					    POWER_SUPPLY_PROP_CURRENT_NOW,
					    &val_chg_current);
		if (ret) {
			dev_err(info->dev, "%s: fail to set charging cur(%d)\n",
				__func__, ret);
		}

		ret = psy_sub->set_property(psy_sub,
						POWER_SUPPLY_PROP_STATUS,
					    &val_type);
		if (ret) {
			dev_err(info->dev, "%s: fail to set charging status(%d)\n",
				__func__, ret);
		}

		dev_info(info->dev, "%s: reset charging current\n",
			 __func__);
	}
}
#endif

#if 0 /* for backup */
static void sec_batt_check_call_exception(struct sec_bat_info *info)
{
	struct power_supply *psy_sub =
	    get_power_supply_by_name(info->sub_charger_name);
	union power_supply_propval val_type, val_chg_current;
	int proximity_activation = 0;
	int proximity_approach = 0;
	int ret;

	if (info->cable_type != CABLE_TYPE_AC) {
		info->is_call_except = 0;
		return;
	}

	if (!info->charging_enabled) {
		if (info->is_call_except)
			info->is_call_except = 0;
		return;
	}

	proximity_activation = get_proximity_activation_state();
	proximity_approach = get_proximity_approach_state();

	if (info->voice_call_state &&
		proximity_activation &&
		proximity_approach) {
		/* info->is_call_except = 1; */
		info->batt_vcell = sec_bat_get_fuelgauge_data(info, FG_T_VCELL);
		if (info->batt_vcell >= CALL_EXCEPTION_VOLTAGE_DN &&
			info->batt_vcell < CALL_EXCEPTION_VOLTAGE_UP &&
			info->is_call_except == 0) {
			info->is_call_except = 0;
		} else if (info->batt_vcell < CALL_EXCEPTION_VOLTAGE_DN) {
			info->is_call_except = 0;
		} else {
			info->is_call_except = 1;
		}
	} else {
		info->is_call_except = 0;
	}

	val_type.intval = POWER_SUPPLY_STATUS_UNKNOWN;

	if (info->charging_set_current == 650 &&
		info->is_call_except == 1) {
		val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
		val_chg_current.intval = 450;
		info->charging_set_current = val_chg_current.intval;
		pr_info("call exception enable! (%d, %d, %d)\n",
			info->voice_call_state, proximity_activation,
			proximity_approach);
	} else if (info->charging_set_current == 450 &&
		info->is_call_except == 0) {
		val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
		val_chg_current.intval = 650;
		info->charging_set_current = val_chg_current.intval;
		pr_info("call exception disable! (%d, %d, %d)\n",
			info->voice_call_state, proximity_activation,
			proximity_approach);
	}

	if (val_type.intval == POWER_SUPPLY_STATUS_CHARGING) {
		ret = psy_sub->set_property(psy_sub,
					    POWER_SUPPLY_PROP_CURRENT_NOW,
					    &val_chg_current);
		if (ret) {
			dev_err(info->dev, "%s: fail to set charging cur(%d)\n",
				__func__, ret);
		}

		ret = psy_sub->set_property(psy_sub,
						POWER_SUPPLY_PROP_STATUS,
					    &val_type);
		if (ret) {
			dev_err(info->dev, "%s: fail to set charging status(%d)\n",
				__func__, ret);
		}

		dev_info(info->dev, "%s: reset charging current\n",
			 __func__);
	}
}
#endif

static void sec_bat_monitor_work(struct work_struct *work)
{
	struct sec_bat_info *info = container_of(work, struct sec_bat_info,
						 monitor_work);
	struct power_supply *psy_fg =
		power_supply_get_by_name(info->fuel_gauge_name);
	union power_supply_propval value;
	int ret = 0;

	wake_lock(&info->monitor_wake_lock);

	sec_bat_charging_time_management(info);

	sec_bat_update_info(info);
	sec_bat_check_temper(info);
	sec_bat_check_vf(info);

	if (info->use_sub_charger) {
		sec_bat_check_ing_level_trigger(info);
		sec_check_chgcurrent(info);

		/*if (info->sub_chg_status == POWER_SUPPLY_STATUS_CHARGING &&
				info->test_info.test_value == 2) {
			pr_info("batt test case : %d\n",
					info->test_info.test_value);
			sec_bat_handle_sub_charger_topoff(info);
		}*/
	}

	if (info->test_info.test_value == 6)
		info->batt_tmu_status = TMU_STATUS_WARNING;

	switch (info->charging_status) {
	case POWER_SUPPLY_STATUS_FULL:
		/* notify full state to fuel guage */
		if (!info->is_timeout_chgstop) {
			value.intval = POWER_SUPPLY_STATUS_FULL;
			ret = psy_fg->set_property(psy_fg,
				POWER_SUPPLY_PROP_STATUS, &value);
		}

		if (sec_check_recharging(info) &&
		    info->recharging_status == false) {
			info->recharging_status = true;
			sec_bat_enable_charging(info, true);

			dev_info(info->dev,
				 "%s: Start Recharging, Vcell = %d\n", __func__,
				 info->batt_vcell);
		}
		break;
	case POWER_SUPPLY_STATUS_CHARGING:
		if (info->batt_health == POWER_SUPPLY_HEALTH_OVERHEAT
		    || info->batt_health == POWER_SUPPLY_HEALTH_COLD) {
			sec_bat_enable_charging(info, false);
			info->charging_status =
			    POWER_SUPPLY_STATUS_NOT_CHARGING;
			info->test_info.is_rechg_state = false;

			dev_info(info->dev, "%s: Not charging\n", __func__);
		} else if (info->batt_health ==
				POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
			sec_bat_enable_charging(info, false);
			info->charging_status =
					POWER_SUPPLY_STATUS_NOT_CHARGING;
			info->test_info.is_rechg_state = false;
			dev_info(info->dev, "%s: Not charging (VF err!)\n",
								__func__);
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
				sec_bat_enable_charging(info, true);
				info->charging_status
				    = POWER_SUPPLY_STATUS_CHARGING;
			} else
				info->charging_status
				    = POWER_SUPPLY_STATUS_DISCHARGING;
		}
		break;
	default:
		dev_info(info->dev, "%s: Undefined Battery Status\n", __func__);
		return;
	}

	if (info->batt_soc != info->batt_presoc)
		pr_info("[fg] p:%d, s1:%d, s2:%d, v:%d, t:%d\n",
			info->batt_raw_soc,
			info->batt_soc, info->batt_presoc,
			info->batt_vcell, info->batt_temp_radc);

	power_supply_changed(&info->psy_bat);

	wake_unlock(&info->monitor_wake_lock);

	return;
}

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

static void sec_bat_measure_work(struct work_struct *work)
{
	struct sec_bat_info *info;
	info = container_of(work, struct sec_bat_info, measure_work.work);

	wake_lock(&info->measure_wake_lock);
	sec_bat_check_temper_adc(info);
	if (sec_bat_check_detbat(info) == BAT_NOT_DETECTED
			&& info->present == 1) {
		msleep(100);
		sec_bat_check_detbat(info); /* AGAIN_FEATURE */
	}
	/* TBD */
	/*
	sec_batt_check_call_exception(info);
	sec_batt_check_call_exception_test(info); */ /* for test */

	if (info->initial_check_count) {
		queue_delayed_work(info->monitor_wqueue, &info->measure_work,
		      HZ);
	} else {
		queue_delayed_work(info->monitor_wqueue, &info->measure_work,
		      msecs_to_jiffies(info->measure_interval));
	}
	/*
	schedule_delayed_work(&info->measure_work,
		      msecs_to_jiffies(info->measure_interval));
	*/
	wake_unlock(&info->measure_wake_lock);
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
	SEC_BATTERY_ATTR(batt_temp_adc_avg),
	SEC_BATTERY_ATTR(batt_temp_adc_sub),
	SEC_BATTERY_ATTR(batt_charging_source),
	SEC_BATTERY_ATTR(batt_lp_charging),
	SEC_BATTERY_ATTR(video),
	SEC_BATTERY_ATTR(mp3),
	SEC_BATTERY_ATTR(batt_type),
	SEC_BATTERY_ATTR(batt_full_check),
	SEC_BATTERY_ATTR(batt_temp_check),
	SEC_BATTERY_ATTR(batt_temp_adc_spec),
	SEC_BATTERY_ATTR(batt_test_value),
	SEC_BATTERY_ATTR(batt_temp_radc),
	SEC_BATTERY_ATTR(batt_current_adc),
	SEC_BATTERY_ATTR(batt_esus_test),
	SEC_BATTERY_ATTR(system_rev),
	SEC_BATTERY_ATTR(batt_read_raw_soc),
	SEC_BATTERY_ATTR(batt_lpm_state),
	SEC_BATTERY_ATTR(sub_chg_state),
	SEC_BATTERY_ATTR(lpm_reboot_event),
	SEC_BATTERY_ATTR(talk_wcdma),
	SEC_BATTERY_ATTR(talk_gsm),
	SEC_BATTERY_ATTR(batt_tmu_status),
	SEC_BATTERY_ATTR(current_avg),
};

enum {
	BATT_VOL = 0,
	BATT_SOC,
	BATT_VFOCV,
	BATT_TEMP,
	BATT_TEMP_ADC,
	BATT_TEMP_ADC_AVG,
	BATT_TEMP_ADC_SUB,
	BATT_CHARGING_SOURCE,
	BATT_LP_CHARGING,
	BATT_VIDEO,
	BATT_MP3,
	BATT_TYPE,
	BATT_FULL_CHECK,
	BATT_TEMP_CHECK,
	BATT_TEMP_ADC_SPEC,
	BATT_TEST_VALUE,
	BATT_TEMP_RADC,
	BATT_CURRENT_ADC,
	BATT_ESUS_TEST,
	BATT_SYSTEM_REV,
	BATT_READ_RAW_SOC,
	BATT_LPM_STATE,
	BATT_SUB_CHG_STATE,
	LPM_REBOOT_EVENT,
	BATT_WCDMA_CALL,
	BATT_GSM_CALL,
	BATT_TMU_STATUS,
	CURRENT_AVG,
};

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
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
							info->batt_temp);
		break;
	case BATT_TEMP_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
							info->batt_temp_adc);
		break;
	case BATT_TEMP_ADC_AVG:
		/*
		val = info->temper_adc_sample.average_adc;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		*/
		break;
	case BATT_TEMP_ADC_SUB:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				info->batt_temp_radc_sub);
		break;
	case BATT_CHARGING_SOURCE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->cable_type);
		break;
	case BATT_LP_CHARGING:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->get_lpcharging_state());
		break;
	case BATT_VIDEO:
		/* TODO */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		break;
	case BATT_MP3:
		/* TODO */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		break;
	case BATT_TYPE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", "SDI_SDI");
		break;
	case BATT_FULL_CHECK:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(info->charging_status ==
					POWER_SUPPLY_STATUS_FULL) ? 1 : 0);
		break;
	case BATT_TEMP_CHECK:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			"%d\n", info->batt_health);
		break;
	case BATT_TEMP_ADC_SPEC:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			"(HIGH: %d - %d,   LOW: %d - %d)\n",
				HIGH_BLOCK_TEMP_ADC, HIGH_RECOVER_TEMP_ADC,
				LOW_BLOCK_TEMP_ADC, LOW_RECOVER_TEMP_ADC);
		break;
	case BATT_TEST_VALUE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			info->test_info.test_value);
		break;
	case BATT_TEMP_RADC:
		val = s3c_read_temper_adc(info);
		val = sec_rescale_temp_adc(info);
		i += scnprintf(buf + i, PAGE_SIZE - i,
						"%d\n", info->batt_temp_radc);
		break;
	case BATT_CURRENT_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			info->batt_current_adc);
		break;
	case BATT_ESUS_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			info->test_info.test_esuspend);
		break;
	case BATT_SYSTEM_REV:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", system_rev);
		break;
	case BATT_READ_RAW_SOC:
		val = sec_bat_get_fuelgauge_data(info, FG_T_PSOC);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_LPM_STATE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			info->batt_lpm_state);
		break;
	case BATT_SUB_CHG_STATE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			info->sub_chg_status);
		break;
	case BATT_TMU_STATUS:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			info->batt_tmu_status);
		break;
	case CURRENT_AVG:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		break;
	default:
		i = -EINVAL;
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
	case BATT_VIDEO:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			dev_info(info->dev, "%s: video(%d)\n", __func__, x);
			ret = count;
		}
		break;
	case BATT_MP3:
		/* TODO */
		if (sscanf(buf, "%d\n", &x) == 1) {
			dev_info(info->dev, "%s: mp3(%d)\n", __func__, x);
			ret = count;
		}
		break;
	case BATT_ESUS_TEST: /* early_suspend test */
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x == 0) {
				info->test_info.test_esuspend = 0;
				wake_lock_timeout(&info->test_wake_lock,
							5 * HZ);
				cancel_delayed_work(&info->measure_work);
				info->measure_interval = MEASURE_DSG_INTERVAL;
				wake_lock(&info->measure_wake_lock);
				queue_delayed_work(info->monitor_wqueue,
							&info->measure_work, 0);
				/* schedule_delayed_work(&info->measure_work,
							0); */
			} else {
				info->test_info.test_esuspend = 1;
				wake_lock(&info->test_wake_lock);
				cancel_delayed_work(&info->measure_work);
				info->measure_interval = MEASURE_CHG_INTERVAL;
				wake_lock(&info->measure_wake_lock);
				queue_delayed_work(info->monitor_wqueue,
							&info->measure_work, 0);
				/* schedule_delayed_work(&info->measure_work,
							0); */
			}
			ret = count;
		}
		break;
	case BATT_TEST_VALUE:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x == 0)
				info->test_info.test_value = 0;
			else if (x == 1)
				/* for temp warning event */
				info->test_info.test_value = 1;
			else if (x == 2)
				/* for full event */
				/* info->test_info.test_value = 2; */
				/* disable full test interface. */
				info->test_info.test_value = 0;
			else if (x == 3)
				/* for abs time event */
				info->test_info.test_value = 3;
			else if (x == 999) {
				/* for pop-up disable */
				info->test_info.test_value = 999;
				if ((info->batt_health ==
						POWER_SUPPLY_HEALTH_OVERHEAT) ||
					(info->batt_health ==
						POWER_SUPPLY_HEALTH_COLD)) {
					info->batt_health =
						POWER_SUPPLY_HEALTH_GOOD;
					wake_lock(&info->monitor_wake_lock);
					queue_work(info->monitor_wqueue,
							&info->monitor_work);
				}
			} else if (x == 6)
				/* for tmu test */
				info->test_info.test_value = 6;
			else
				info->test_info.test_value = 0;
			pr_info("batt test case : %d\n",
						info->test_info.test_value);
			ret = count;
		}
		break;
	case BATT_LPM_STATE:
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->batt_lpm_state = x;
			ret = count;
		}
		break;
	case LPM_REBOOT_EVENT:
		if (sscanf(buf, "%d\n", &x) == 1) {
			/* disable sub-charger and enable main-charger */
			pr_info("lpm reboot event is trigered\n");
			if (info->use_sub_charger) {
				sec_bat_enable_charging(info, false);
				sec_main_charger_default(info);
			}
			ret = count;
		}
		break;
	case BATT_WCDMA_CALL:
	case BATT_GSM_CALL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->voice_call_state = x;
			pr_info("voice call = %d, %d\n",
					x, info->voice_call_state);
		}
		break;
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

	len = sprintf(buf, "%lu	%u	%u	%u	%u	"
			"%u	%d	%d	%d	%d	"
			"%d	%u	%u	%u	%u	"
			"%u	%u	%d	%d	%d	"
			"%u	%lu\n",
			cur_time.tv_sec,
			info->batt_raw_soc,
			info->batt_soc,
			info->batt_vcell,
			info->batt_current_adc,
			info->charging_enabled,
			info->batt_full_status,
			info->test_info.full_count,
			info->test_info.full_count_sub,
			info->test_info.rechg_count,
			info->test_info.is_rechg_state,
			info->recharging_status,
			info->batt_temp_radc,
			info->batt_temp_radc_sub,
			info->batt_health,
			info->charging_status,
			info->present,
			info->cable_type,
			info->batt_tmu_status,
			info->is_timeout_chgstop,
			info->batt_full_soc,
			info->charging_passed_time);
	return len;
}

static __devinit int sec_bat_probe(struct platform_device *pdev)
{
	struct sec_bat_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct sec_bat_info *info;
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
	info->adc_arr_size = pdata->adc_arr_size;
	info->adc_table = pdata->adc_table;

	info->get_lpcharging_state = pdata->get_lpcharging_state;

	if (pdata->sub_charger_name) {
		info->sub_charger_name = pdata->sub_charger_name;
		if (system_rev >= HWREV_FOR_BATTERY)
			info->use_sub_charger = true;
	}

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
	wake_lock_init(&info->test_wake_lock, WAKE_LOCK_SUSPEND,
				"bat_esus_test");
	wake_lock_init(&info->measure_wake_lock, WAKE_LOCK_SUSPEND,
				"sec-battery-measure");

	info->batt_health = POWER_SUPPLY_HEALTH_GOOD;
	info->charging_status = sec_bat_is_charging(info);
	if (info->charging_status < 0)
		info->charging_status = POWER_SUPPLY_STATUS_DISCHARGING;
	else if (info->charging_status == POWER_SUPPLY_STATUS_CHARGING)
		info->cable_type = CABLE_TYPE_USB; /* default */
	info->batt_soc = 100;
	info->recharging_status = false;
	info->is_timeout_chgstop = false;
	info->present = 1;
	info->initial_check_count = INIT_CHECK_COUNT;

	mutex_init(&info->adclock);

	info->padc = s3c_adc_register(pdev, NULL, NULL, 0);
	info->charging_start_time = 0;
	info->is_call_except = 0;

	if (info->get_lpcharging_state) {
		if (info->get_lpcharging_state())
			info->polling_interval = POLLING_INTERVAL / 4;
		else
			info->polling_interval = POLLING_INTERVAL;
	}

	info->batt_tmu_status = TMU_STATUS_NORMAL;

	if (info->charging_status == POWER_SUPPLY_STATUS_CHARGING)
		info->measure_interval = MEASURE_CHG_INTERVAL;
	else
		info->measure_interval = MEASURE_DSG_INTERVAL;

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

	info->monitor_wqueue =
	    create_freezable_workqueue(dev_name(&pdev->dev));
	if (!info->monitor_wqueue) {
		dev_err(info->dev, "%s: fail to create workqueue\n", __func__);
		goto err_supply_unreg_ac;
	}

	if (info->charging_status == POWER_SUPPLY_STATUS_DISCHARGING)
		sec_main_charger_disable(info);

	INIT_WORK(&info->monitor_work, sec_bat_monitor_work);
	INIT_DELAYED_WORK_DEFERRABLE(&info->cable_work, sec_bat_cable_work);

	INIT_DELAYED_WORK_DEFERRABLE(&info->polling_work, sec_bat_polling_work);
	schedule_delayed_work(&info->polling_work, 0);

	INIT_DELAYED_WORK_DEFERRABLE(&info->measure_work, sec_bat_measure_work);
	schedule_delayed_work(&info->measure_work, 0);

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
	wake_lock_destroy(&info->test_wake_lock);
	wake_lock_destroy(&info->measure_wake_lock);
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

	cancel_delayed_work(&info->cable_work);
	cancel_delayed_work(&info->polling_work);
	cancel_delayed_work(&info->measure_work);

	power_supply_unregister(&info->psy_bat);
	power_supply_unregister(&info->psy_usb);
	power_supply_unregister(&info->psy_ac);

	wake_lock_destroy(&info->vbus_wake_lock);
	wake_lock_destroy(&info->monitor_wake_lock);
	wake_lock_destroy(&info->cable_wake_lock);
	wake_lock_destroy(&info->test_wake_lock);
	wake_lock_destroy(&info->measure_wake_lock);
	mutex_destroy(&info->adclock);

	s3c_adc_release(info->padc);

	kfree(info);

	return 0;
}

static int sec_bat_suspend(struct device *dev)
{
	struct sec_bat_info *info = dev_get_drvdata(dev);

	cancel_work_sync(&info->monitor_work);
	cancel_delayed_work(&info->cable_work);
	cancel_delayed_work(&info->polling_work);
	cancel_delayed_work(&info->measure_work);

	return 0;
}

static int sec_bat_resume(struct device *dev)
{
	struct sec_bat_info *info = dev_get_drvdata(dev);

	queue_delayed_work(info->monitor_wqueue,
		&info->measure_work, 0);

	wake_lock(&info->monitor_wake_lock);
	queue_work(info->monitor_wqueue, &info->monitor_work);

	schedule_delayed_work(&info->polling_work,
		msecs_to_jiffies(info->polling_interval));
	/*
	schedule_delayed_work(&info->measure_work,
		msecs_to_jiffies(info->measure_interval));
	*/

	return 0;
}

#define sec_bat_shutdown	NULL

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
