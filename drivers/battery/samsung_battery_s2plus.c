/*
 * samsung_battery.c
 *
 * Copyright (C) 2011 Samsung Electronics
 * SangYoung Son <hello.son@samsung.com>
 *
 * based on sec_battery.c
 *
 * Copyright (C) 2012 Samsung Electronics
 * Jaecheol Kim <jc22.kim@samsung.com>
 *
 * add sub-charger based on samsung_battery.c
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/reboot.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/android_alarm.h>
#include <linux/battery/samsung_battery.h>
#include <mach/regs-pmu.h>
#include "battery-factory.h"
#if defined(CONFIG_S3C_ADC)
#include <plat/adc.h>
#endif

static char *supply_list[] = {
	"battery",
};

/* Get LP charging mode state */
unsigned int lpcharge;
static int battery_get_lpm_state(char *str)
{
	get_option(&str, &lpcharge);
	pr_info("%s: Low power charging mode: %d\n", __func__, lpcharge);

	return lpcharge;
}
__setup("lpcharge=", battery_get_lpm_state);

/* Temperature from fuelgauge or adc */
static int battery_get_temper(struct battery_info *info)
{
	union power_supply_propval value;
	int temper = 0;
	int retry_cnt = 0;
	pr_debug("%s\n", __func__);

	switch (info->pdata->temper_src) {
	case TEMPER_FUELGAUGE:
		info->psy_fuelgauge->get_property(info->psy_fuelgauge,
					  POWER_SUPPLY_PROP_TEMP, &value);
		temper = value.intval;
		break;
	case TEMPER_AP_ADC:
#if defined(CONFIG_S3C_ADC)
		do {
			info->battery_temper_adc =
				s3c_adc_read(info->adc_client,
					info->pdata->temper_ch);

			if (info->battery_temper_adc < 0) {
				pr_info("%s: adc read(%d), retry(%d)", __func__,
					info->battery_temper_adc, retry_cnt);
				retry_cnt++;
				msleep(100);
			}
		} while ((info->battery_temper_adc < 0) && (retry_cnt <= 5));

		if (info->battery_temper_adc < 0) {
			pr_info("%s: adc read error(%d), temper set as 30.0",
					__func__, info->battery_temper_adc);
			temper = 300;
		} else {
			temper = info->pdata->covert_adc(
					info->battery_temper_adc,
					info->pdata->temper_ch);
		}
#endif
		break;
	case TEMPER_EXT_ADC:
#if defined(CONFIG_STMPE811_ADC)
		temper = stmpe811_get_adc_value(info->pdata->temper_ch);
#endif
		break;
	case TEMPER_UNKNOWN:
	default:
		pr_info("%s: invalid temper src(%d)\n", __func__,
					info->pdata->temper_src);
		temper = 300;
		break;
	}

	pr_debug("%s: temper(%d), source(%d)\n", __func__,
			temper, info->pdata->temper_src);
	return temper;
}

/* Get info from power supply at realtime */
int battery_get_info(struct battery_info *info,
		     enum power_supply_property property)
{
	union power_supply_propval value;
	value.intval = 0;

	if (info->battery_error_test) {
		pr_info("%s: in test mode(%d), do not update\n", __func__,
			info->battery_error_test);
		return -EPERM;
	}

	switch (property) {
	/* Update from charger */
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_HEALTH:
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
		info->psy_charger->get_property(info->psy_charger,
						property, &value);
		break;
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (info->use_sub_charger) {
			info->psy_sub_charger->get_property(info->psy_sub_charger,
						property, &value);
		} else {
			info->psy_charger->get_property(info->psy_charger,
						property, &value);
		}
		break;
	/* Update from fuelgauge */
	case POWER_SUPPLY_PROP_CAPACITY:	/* Only Adjusted SOC */
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:	/* Only VCELL */
		info->psy_fuelgauge->get_property(info->psy_fuelgauge,
						  property, &value);
		break;
	/* Update from fuelgauge or adc */
	case POWER_SUPPLY_PROP_TEMP:
		value.intval = battery_get_temper(info);
		break;
	default:
		break;
	}

	return value.intval;
}

/* Update all values for battery */
void battery_update_info(struct battery_info *info)
{
	union power_supply_propval value;

	if (info->use_sub_charger) {
		/* Update from Charger */
		info->psy_sub_charger->get_property(info->psy_sub_charger,
						POWER_SUPPLY_PROP_STATUS, &value);
		info->charge_real_state = info->charge_virt_state = value.intval;

		info->psy_sub_charger->get_property(info->psy_sub_charger,
						POWER_SUPPLY_PROP_CURRENT_NOW, &value);
		info->charge_current = value.intval;

	} else {
		/* Update from Charger */
		info->psy_charger->get_property(info->psy_charger,
						POWER_SUPPLY_PROP_STATUS, &value);
		info->charge_real_state = info->charge_virt_state = value.intval;

		info->psy_charger->get_property(info->psy_charger,
						POWER_SUPPLY_PROP_CURRENT_NOW, &value);
		info->charge_current = value.intval;
	}

	info->psy_charger->get_property(info->psy_charger,
					POWER_SUPPLY_PROP_HEALTH, &value);
	info->battery_health = value.intval;

	info->psy_charger->get_property(info->psy_charger,
					POWER_SUPPLY_PROP_PRESENT, &value);
	info->battery_present = value.intval;

	info->psy_charger->get_property(info->psy_charger,
					POWER_SUPPLY_PROP_ONLINE, &value);
	info->cable_type = value.intval;

	info->psy_charger->get_property(info->psy_charger,
					POWER_SUPPLY_PROP_CHARGE_TYPE, &value);
	info->charge_type = value.intval;

	/* Fuelgauge power off state */
	if ((info->cable_type != POWER_SUPPLY_TYPE_BATTERY) &&
	    (info->battery_present == 0)) {
		pr_info("%s: Abnormal fuelgauge power state\n", __func__);
		goto update_finish;
	}

	/* Update from Fuelgauge */
	value.intval = SOC_TYPE_ADJUSTED;
	info->psy_fuelgauge->get_property(info->psy_fuelgauge,
					  POWER_SUPPLY_PROP_CAPACITY, &value);
	info->battery_soc = value.intval;

	value.intval = SOC_TYPE_RAW;
	info->psy_fuelgauge->get_property(info->psy_fuelgauge,
					  POWER_SUPPLY_PROP_CAPACITY, &value);
	info->battery_raw_soc = value.intval;

	value.intval = VOLTAGE_TYPE_VCELL;
	info->psy_fuelgauge->get_property(info->psy_fuelgauge,
					  POWER_SUPPLY_PROP_VOLTAGE_NOW,
					  &value);
	info->battery_vcell = value.intval;

	value.intval = VOLTAGE_TYPE_VFOCV;
	info->psy_fuelgauge->get_property(info->psy_fuelgauge,
					  POWER_SUPPLY_PROP_VOLTAGE_NOW,
					  &value);
	info->battery_vfocv = value.intval;

	info->battery_temper = battery_get_temper(info);

update_finish:
	switch (info->battery_error_test) {
	case 0:
		pr_debug("%s: error test: normal state\n", __func__);
		break;
	case 1:
		pr_info("%s: error test: full charged\n", __func__);
		info->charge_real_state = POWER_SUPPLY_STATUS_FULL;
		info->battery_vcell = 4200000;
		info->battery_soc = 100;
		break;
	case 2:
		pr_info("%s: error test: freezed\n", __func__);
		info->battery_temper = info->pdata->freeze_stop_temp - 10;
		break;
	case 3:
		pr_info("%s: error test: overheated\n", __func__);
		info->battery_temper = info->pdata->overheat_stop_temp + 10;
		break;
	case 4:
		pr_info("%s: error test: ovp\n", __func__);
		break;
	case 5:
		pr_info("%s: error test: vf error\n", __func__);
		info->battery_present = 0;
		break;
	default:
		pr_info("%s: error test: unknown state\n", __func__);
		break;
	}

	pr_debug("%s: state(%d), type(%d), "
		 "health(%d), present(%d), "
		 "cable(%d), curr(%d), "
		 "soc(%d), raw(%d), "
		 "vol(%d), ocv(%d), tmp(%d)\n", __func__,
		 info->charge_real_state, info->charge_type,
		 info->battery_health, info->battery_present,
		 info->cable_type, info->charge_current,
		 info->battery_soc, info->battery_raw_soc,
		 info->battery_vcell, info->battery_vfocv,
		 info->battery_temper);
}

/* Control charger and fuelgauge */
void battery_control_info(struct battery_info *info,
			  enum power_supply_property property, int intval)
{
	union power_supply_propval value;

	value.intval = intval;

	switch (property) {
		/* Control to charger */
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (info->use_sub_charger) {
			info->psy_sub_charger->set_property(info->psy_sub_charger,
						property, &value);
		} else {
			info->psy_charger->set_property(info->psy_charger,
						property, &value);
		}
		break;

		/* Control to fuelgauge */
	case POWER_SUPPLY_PROP_CAPACITY:
		info->psy_fuelgauge->set_property(info->psy_fuelgauge,
						  property, &value);
		break;
	default:
		break;
	}
}

/* Support property from battery */
static enum power_supply_property samsung_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
};

/* Support property from usb, ac */
static enum power_supply_property samsung_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int samsung_battery_get_property(struct power_supply *ps,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct battery_info *info = container_of(ps, struct battery_info,
						 psy_bat);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = info->charge_virt_state;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = info->charge_type;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = info->battery_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = info->battery_present;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = info->cable_type;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = info->battery_vcell;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = info->charge_current;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = info->battery_soc;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = info->battery_temper;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int samsung_battery_set_property(struct power_supply *ps,
					enum power_supply_property psp,
					const union power_supply_propval *val)
{
	struct battery_info *info = container_of(ps, struct battery_info,
						 psy_bat);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_HEALTH:
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_TECHNOLOGY:
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CAPACITY:
	case POWER_SUPPLY_PROP_TEMP:
		break;
	default:
		return -EINVAL;
	}

	wake_lock(&info->monitor_wake_lock);
	schedule_work(&info->monitor_work);

	return 0;
}

static int samsung_usb_get_property(struct power_supply *ps,
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
	struct battery_info *info = container_of(ps, struct battery_info,
						 psy_usb);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	/* Set enable=1 only if the USB charger is connected */
	val->intval = (info->cable_type == POWER_SUPPLY_TYPE_USB) ||
		(info->cable_type == POWER_SUPPLY_TYPE_USB_CDP);

	return 0;
}

static int samsung_ac_get_property(struct power_supply *ps,
				   enum power_supply_property psp,
				   union power_supply_propval *val)
{
	struct battery_info *info = container_of(ps, struct battery_info,
						 psy_ac);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	/* Set enable=1 only if the AC charger is connected */
	val->intval = (info->cable_type == POWER_SUPPLY_TYPE_MAINS) ||
		(info->cable_type == POWER_SUPPLY_TYPE_MISC) ||
		(info->cable_type == POWER_SUPPLY_TYPE_WIRELESS);

	return 0;
}

static void samsung_battery_alarm_start(struct alarm *alarm)
{
	struct battery_info *info = container_of(alarm, struct battery_info,
						 alarm);
	pr_debug("%s\n", __func__);

	wake_lock(&info->monitor_wake_lock);
	schedule_work(&info->monitor_work);
}

static void samsung_battery_next_monitor(struct battery_info *info)
{
	ktime_t interval, next;
	unsigned long flags;
	pr_debug("%s\n", __func__);

	local_irq_save(flags);

	info->last_poll = alarm_get_elapsed_realtime();

	switch (info->monitor_mode) {
	case MONITOR_CHNG:
		info->monitor_interval = info->pdata->chng_interval;
		break;
	case MONITOR_CHNG_SUSP:
		info->monitor_interval = info->pdata->chng_susp_interval;
		break;
	case MONITOR_NORM:
		info->monitor_interval = info->pdata->norm_interval;
		break;
	case MONITOR_NORM_SUSP:
		info->monitor_interval = info->pdata->norm_susp_interval;
		break;
	case MONITOR_EMER:
		info->monitor_interval = info->pdata->emer_interval;
		break;
	default:
		info->monitor_interval = info->pdata->norm_interval;
		break;
	}

	pr_debug("%s: monitor mode(%d), interval(%d)\n", __func__,
		info->monitor_mode, info->monitor_interval);

	interval = ktime_set(info->monitor_interval, 0);
	next = ktime_add(info->last_poll, interval);
	alarm_start_range(&info->alarm, next, next);

	local_irq_restore(flags);
}

static bool battery_recharge_cond(struct battery_info *info)
{
	pr_debug("%s\n", __func__);

	if (info->charge_real_state == POWER_SUPPLY_STATUS_CHARGING) {
		pr_debug("%s: not recharge cond., now charging\n", __func__);
		return false;
	}

	if (info->battery_vcell < info->pdata->recharge_voltage) {
		pr_info("%s: recharge start(%d ?? %d)\n", __func__,
			info->battery_vcell, info->pdata->recharge_voltage);
		return true;
	} else
		pr_debug("%s: not recharge cond., vcell is enough\n", __func__);

	return false;
}

static bool battery_abstimer_cond(struct battery_info *info)
{
	unsigned int abstimer_duration;
	ktime_t ktime;
	struct timespec current_time;
	pr_debug("%s\n", __func__);

	if ((info->cable_type != POWER_SUPPLY_TYPE_MAINS) ||
		(info->full_charged_state == true) ||
		(info->charge_start_time == 0)) {
		info->abstimer_state = false;
		return false;
	}

	ktime = alarm_get_elapsed_realtime();
	current_time = ktime_to_timespec(ktime);

	if (info->recharge_phase)
		abstimer_duration = info->pdata->abstimer_recharge_duration;
	else
		abstimer_duration = info->pdata->abstimer_charge_duration;

	if ((current_time.tv_sec - info->charge_start_time) >
	    abstimer_duration) {
		pr_info("%s: charge time out(%d - %d ?? %d)\n", __func__,
			(int)current_time.tv_sec,
			info->charge_start_time,
			abstimer_duration);
		info->abstimer_state = true;
	} else {
		pr_debug("%s: not abstimer condition\n", __func__);
		info->abstimer_state = false;
	}

	return info->abstimer_state;
}

static bool battery_fullcharged_cond(struct battery_info *info)
{
	pr_debug("%s\n", __func__);

	if ((info->charge_real_state == POWER_SUPPLY_STATUS_FULL) &&
	    (info->battery_vcell > 4150000) &&
	    (info->battery_soc > 95)) {
		pr_info("%s: real full charged(%d, %d)\n", __func__,
				info->battery_vcell, info->battery_soc);
		info->full_charged_state = true;
		return true;
	} else if (info->full_charged_state == true) {
		pr_debug("%s: already full charged\n", __func__);
	} else {
		pr_debug("%s: not full charged\n", __func__);
		info->full_charged_state = false;
	}

	/* Add some more full charged case(current sensing, etc...) */

	return false;
}

static bool battery_vf_cond(struct battery_info *info)
{
	pr_debug("%s\n", __func__);

#if defined(CONFIG_MACH_P11)
	/* FIXME: fix P11 build error temporarily */
#else
	/* jig detect by MUIC */
	if (is_jig_attached == JIG_ON) {
		pr_info("%s: JIG ON, do not check\n", __func__);
		info->vf_state = false;
		return false;
	}
#endif

	/* TODO: Check VF from ADC */

	/* Now, battery present from charger */
	info->battery_present =
		battery_get_info(info, POWER_SUPPLY_PROP_PRESENT);
	if (info->battery_present == 0) {
		pr_info("%s: battery is not detected.\n", __func__);
		info->vf_state = true;
	} else {
		pr_debug("%s: battery is detected.\n", __func__);
		info->vf_state = false;
	}

	return info->vf_state;
}

static bool battery_health_cond(struct battery_info *info)
{
	pr_debug("%s\n", __func__);

	if (info->battery_health == POWER_SUPPLY_HEALTH_DEAD) {
		pr_info("%s: battery dead(%d)\n", __func__,
					info->battery_health);
		info->health_state = true;
	} else if (info->battery_health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) {
		pr_info("%s: battery overvoltage(%d)\n", __func__,
					info->battery_health);
		info->health_state = true;
	} else {
		pr_debug("%s: battery good(%d)\n", __func__,
					info->battery_health);
		info->health_state = false;
	}

	return info->health_state;
}

static bool battery_temp_cond(struct battery_info *info)
{
	pr_debug("%s\n", __func__);

	if (info->temperature_state == false) {
		if (info->charge_real_state != POWER_SUPPLY_STATUS_CHARGING) {
			pr_debug("%s: not charging state\n", __func__);
			return false;
		}

		pr_debug("%s: check charging stop temp."
			 "cond: %d ?? %d ~ %d\n", __func__,
			 info->battery_temper,
			 info->pdata->freeze_stop_temp,
			 info->pdata->overheat_stop_temp);

		if (info->battery_temper >=
		    info->pdata->overheat_stop_temp) {
			pr_info("%s: stop by overheated temp\n", __func__);
			info->overheated_state = true;
		} else if (info->battery_temper <=
			   info->pdata->freeze_stop_temp) {
			pr_info("%s: stop by freezed temp\n", __func__);
			info->freezed_state = true;
		} else
			pr_debug("%s: normal charging temp\n", __func__);
	} else {
		pr_debug("%s: check charging recovery temp."
			 "cond: %d ?? %d ~ %d\n", __func__,
			 info->battery_temper,
			 info->pdata->freeze_recovery_temp,
			 info->pdata->overheat_recovery_temp);

		if ((info->overheated_state == true) &&
		    (info->battery_temper <=
		     info->pdata->overheat_recovery_temp)) {
			pr_info("%s: recovery from overheated\n",
				__func__);
			info->overheated_state = false;
		} else if ((info->freezed_state == true) &&
			   (info->battery_temper >=
			    info->pdata->freeze_recovery_temp)) {
			pr_info("%s: recovery from freezed\n",
				__func__);
			info->freezed_state = false;
		} else
			pr_info("%s: charge stopped temp\n", __func__);
	}

	if (info->overheated_state == true) {
		info->battery_health = POWER_SUPPLY_HEALTH_OVERHEAT;
		info->freezed_state = false;
		info->temperature_state = true;
	} else if (info->freezed_state == true) {
		info->battery_health = POWER_SUPPLY_HEALTH_COLD;
		info->overheated_state = false;
		info->temperature_state = true;
	} else {
		info->overheated_state = false;
		info->freezed_state = false;
		info->temperature_state = false;
	}

	return info->temperature_state;
}

static void battery_charge_control(struct battery_info *info,
				   int enable,
				   int set_current)
{
	ktime_t ktime;
	struct timespec current_time;
	pr_debug("%s\n", __func__);

	ktime = alarm_get_elapsed_realtime();
	current_time = ktime_to_timespec(ktime);

	if (set_current == CHARGER_KEEP_CURRENT)
		goto charge_state_control;

	if (!info->use_sub_charger) {
		if (info->siop_state == true) {
			pr_debug("%s: siop state, charge current is %dmA\n", __func__,
				 info->siop_charge_current);
			set_current = info->siop_charge_current;
		}

		/* check charge current before and after */
		if (info->charge_current == ((set_current * 3 / 100) * 333 / 10)) {
			/*
			 * (current * 3 / 100) is converted value
			 * for register setting.
			 * (register current * 333 / 10) is actual value
			 * for charging
			 */
			pr_debug("%s: same charge current: %dmA\n", __func__,
								set_current);
		} else {
			battery_control_info(info,
					     POWER_SUPPLY_PROP_CURRENT_NOW,
					     set_current);
			pr_info("%s: update charge current: %dmA\n", __func__,
								set_current);
		}
	}

	info->charge_current =
		battery_get_info(info, POWER_SUPPLY_PROP_CURRENT_NOW);

charge_state_control:
	/* check charge state before and after */
	if ((enable == CHARGE_ENABLE) &&
		(info->charge_start_time == 0)) {
		battery_control_info(info,
				     POWER_SUPPLY_PROP_STATUS,
				     CHARGE_ENABLE);

		info->charge_start_time = current_time.tv_sec;
		pr_info("%s: charge enabled, current as %dmA @%d\n", __func__,
				info->charge_current, info->charge_start_time);
	} else if ((enable == CHARGE_DISABLE) &&
		(info->charge_start_time != 0)) {
		battery_control_info(info,
				     POWER_SUPPLY_PROP_STATUS,
				     CHARGE_DISABLE);

		pr_info("%s: charge disabled, current as %dmA @%d\n", __func__,
				info->charge_current, (int)current_time.tv_sec);

		info->charge_start_time = 0;
	} else {
		pr_debug("%s: same charge state(%s), current as %dmA @%d\n",
				__func__, (enable ? "enabled" : "disabled"),
				info->charge_current, info->charge_start_time);
	}

	info->charge_real_state =
		battery_get_info(info, POWER_SUPPLY_PROP_STATUS);
}

static void battery_monitor_work(struct work_struct *work)
{
	struct battery_info *info = container_of(work, struct battery_info,
						 monitor_work);
	pr_debug("%s\n", __func__);

	/* If battery is not connected, clear flag for charge scenario */
	if (battery_vf_cond(info) == true) {
		pr_info("%s: battery error\n", __func__);
		info->overheated_state = false;
		info->freezed_state = false;
		info->temperature_state = false;
		info->full_charged_state = false;
		info->abstimer_state = false;
		info->recharge_phase = false;

		schedule_work(&info->error_work);
	}

	/* Check battery state from charger and fuelgauge */
	battery_update_info(info);

	/* If charger is not connected, do not check charge scenario */
	if (info->cable_type == POWER_SUPPLY_TYPE_BATTERY)
		goto charge_ok;

	/* Below is charger is connected state */
	if (battery_temp_cond(info) == true) {
		pr_info("%s: charge stopped by temperature\n", __func__);
		battery_charge_control(info,
				       CHARGE_DISABLE, CHARGER_OFF_CURRENT);
		goto monitor_finish;
	}

	if (battery_health_cond(info) == true) {
		pr_info("%s: bad health state\n", __func__);
		goto monitor_finish;
	}

	if (battery_fullcharged_cond(info) == true) {
		pr_info("%s: full charged state\n", __func__);
		battery_charge_control(info,
				       CHARGE_DISABLE, CHARGER_KEEP_CURRENT);
		info->recharge_phase = true;
		goto monitor_finish;
	}

	if (battery_abstimer_cond(info) == true) {
		pr_info("%s: abstimer state\n", __func__);
		battery_charge_control(info,
				       CHARGE_DISABLE, CHARGER_OFF_CURRENT);
		info->recharge_phase = true;
		goto monitor_finish;
	}

	if (info->recharge_phase == true) {
		if (battery_recharge_cond(info) == true) {
			pr_info("%s: recharge condition\n", __func__);
			goto charge_ok;
		} else {
			pr_debug("%s: not recharge\n", __func__);
			goto monitor_finish;
		}
	}

charge_ok:
	switch (info->cable_type) {
	case POWER_SUPPLY_TYPE_BATTERY:
		if (!info->pdata->suspend_chging)
			wake_unlock(&info->charge_wake_lock);
		battery_charge_control(info,
				       CHARGE_DISABLE, CHARGER_OFF_CURRENT);
		info->charge_virt_state = POWER_SUPPLY_STATUS_DISCHARGING;

		/* clear charge scenario state */
		info->overheated_state = false;
		info->freezed_state = false;
		info->temperature_state = false;
		info->full_charged_state = false;
		info->abstimer_state = false;
		info->recharge_phase = false;
		break;
	case POWER_SUPPLY_TYPE_MAINS:
		if (!info->pdata->suspend_chging)
			wake_lock(&info->charge_wake_lock);
		battery_charge_control(info, CHARGE_ENABLE, CHARGER_AC_CURRENT_S2PLUS);
		info->charge_virt_state = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case POWER_SUPPLY_TYPE_USB:
		if (!info->pdata->suspend_chging)
			wake_lock(&info->charge_wake_lock);
		battery_charge_control(info,
				       CHARGE_ENABLE, CHARGER_USB_CURRENT);
		info->charge_virt_state = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case POWER_SUPPLY_TYPE_USB_CDP:
		if (!info->pdata->suspend_chging)
			wake_lock(&info->charge_wake_lock);
		battery_charge_control(info,
				       CHARGE_ENABLE, CHARGER_CDP_CURRENT);
		info->charge_virt_state = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case POWER_SUPPLY_TYPE_WIRELESS:
		if (!info->pdata->suspend_chging)
			wake_lock(&info->charge_wake_lock);
		battery_charge_control(info,
				       CHARGE_ENABLE, CHARGER_WPC_CURRENT);
		info->charge_virt_state = POWER_SUPPLY_STATUS_CHARGING;
		break;
	default:
		break;
	}

monitor_finish:
	/* Overwrite charge state for UI(icon) */
	if (info->full_charged_state == true) {
		info->charge_virt_state = POWER_SUPPLY_STATUS_FULL;
		info->battery_soc = 100;
	} else if (info->abstimer_state == true) {
		info->charge_virt_state = POWER_SUPPLY_STATUS_CHARGING;
	} else if (info->recharge_phase == true) {
		info->charge_virt_state = POWER_SUPPLY_STATUS_CHARGING;
	}

	if (info->cable_type != POWER_SUPPLY_TYPE_BATTERY) {
		if (info->temperature_state == true)
			info->charge_virt_state =
				POWER_SUPPLY_STATUS_NOT_CHARGING;

		if (info->vf_state == true) {
			info->charge_virt_state =
				POWER_SUPPLY_STATUS_NOT_CHARGING;
			/* to be considered */
			info->battery_health =
				POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		}

		if (info->health_state == true)
			info->charge_virt_state =
				POWER_SUPPLY_STATUS_NOT_CHARGING;
	}

	/* monitoring interval */
	if (info->charge_virt_state == POWER_SUPPLY_STATUS_NOT_CHARGING) {
		pr_debug("%s: emergency(not charging) state\n", __func__);
		info->monitor_mode = MONITOR_EMER;
		wake_lock(&info->emer_wake_lock);
	} else {
		pr_debug("%s: normal state\n", __func__);
		info->monitor_mode = MONITOR_NORM;
		wake_unlock(&info->emer_wake_lock);
	}

	pr_info("bat: s(%d), v(%d, %d), b(%d), "
		"t(%d.%d), h(%d), "
		"ch(%d, %d), cb(%d), cr(%d), "
		"abs(%d), f(%d), rch(%d), t(%d)\n",
		info->battery_soc,
		info->battery_vcell / 1000,
		info->battery_vfocv / 1000,
		info->battery_present,
		info->battery_temper / 10, info->battery_temper % 10,
		info->battery_health,
		info->charge_real_state,
		info->charge_virt_state,
		info->cable_type,
		info->charge_current,
		info->abstimer_state,
		info->full_charged_state,
		info->recharge_phase, info->charge_start_time);

	/*
	 * WORKAROUND: Do not power off, if vell is over 3400mV
	 */
	if (info->battery_soc == 0) {
		if (info->battery_vcell > 3400) {
			pr_info("%s: soc 0%%, but vcell(%d) is over 3400mV, "
							"do not power off\n",
						__func__, info->battery_vcell);
			info->battery_soc = 1;
		}
	}

	power_supply_changed(&info->psy_bat);

	/* prevent suspend before starting the alarm */
	samsung_battery_next_monitor(info);

	wake_unlock(&info->monitor_wake_lock);

	return;
}

static void battery_error_work(struct work_struct *work)
{
	struct battery_info *info = container_of(work, struct battery_info,
						 error_work);
	int err_cnt;
	int old_vcell, new_vcell, vcell_diff;
	pr_info("%s\n", __func__);

	if (info->vf_state == true) {
		pr_info("%s: battery error state\n", __func__);
		old_vcell = info->battery_vcell;
		new_vcell = 0;
		for (err_cnt = 1; err_cnt <= VF_CHECK_COUNT; err_cnt++) {
			/* check jig first */
			if (is_jig_attached == JIG_ON) {
				pr_info("%s: JIG detected, return\n", __func__);
				return;
			}
			info->battery_present =
				battery_get_info(info,
					POWER_SUPPLY_PROP_PRESENT);
			if (info->battery_present == 0) {
				pr_info("%s: battery still error(%d)\n",
						__func__, err_cnt);
				msleep(VF_CHECK_DELAY);
			} else {
				pr_info("%s: battery detect ok, "
						"check soc\n", __func__);
				new_vcell = battery_get_info(info,
					POWER_SUPPLY_PROP_VOLTAGE_NOW);
				vcell_diff = abs(old_vcell - new_vcell);
				pr_info("%s: check vcell: %d -> %d, diff: %d\n",
						__func__, info->battery_vcell,
							new_vcell, vcell_diff);
				if (vcell_diff > RESET_SOC_DIFF_TH) {
					pr_info("%s: reset soc\n", __func__);
					battery_control_info(info,
						POWER_SUPPLY_PROP_CAPACITY, 1);
				} else
					pr_info("%s: keep soc\n", __func__);
				break;
			}

			if (err_cnt == VF_CHECK_COUNT) {
				pr_info("%s: battery error, power off\n",
								__func__);
				battery_charge_control(info,
				       CHARGE_DISABLE, CHARGER_OFF_CURRENT);
			}
		}
	}

	return;
}

static __devinit int samsung_battery_probe(struct platform_device *pdev)
{
	struct battery_info *info;
	int ret = 0;
	char *temper_src_name[] = { "fuelgauge", "ap adc",
		"ext adc", "unknown"
	};
	pr_info("%s: SAMSUNG Battery Driver Loading\n", __func__);

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	platform_set_drvdata(pdev, info);

	info->dev = &pdev->dev;
	info->pdata = pdev->dev.platform_data;

	/* Check charger name and fuelgauge name. */
	if (!info->pdata->charger_name || !info->pdata->fuelgauge_name) {
		pr_err("%s: no charger or fuel gauge name\n", __func__);
		goto err_kfree;
	}
	info->charger_name = info->pdata->charger_name;
	info->fuelgauge_name = info->pdata->fuelgauge_name;

	pr_info("%s: Charger name: %s\n", __func__, info->charger_name);
	pr_info("%s: Fuelgauge name: %s\n", __func__, info->fuelgauge_name);

	info->psy_charger = power_supply_get_by_name(info->charger_name);
	info->psy_fuelgauge = power_supply_get_by_name(info->fuelgauge_name);

	if (!info->psy_charger || !info->psy_fuelgauge) {
		pr_err("%s: fail to get power supply\n", __func__);
		goto err_kfree;
	}

	info->use_sub_charger = info->pdata->use_sub_charger;
	if (info->use_sub_charger) {
		info->sub_charger_name = info->pdata->sub_charger_name;
		pr_info("%s: subcharger name: %s\n", __func__,
			info->sub_charger_name);
		info->psy_sub_charger = power_supply_get_by_name(info->sub_charger_name);

		if (!info->psy_sub_charger) {
			pr_err("%s fail to get sub charger\n", __func__);
			goto err_kfree;
		}
	}
	/* force set S2PLUS recharge voltage */
	info->pdata->recharge_voltage = 4150000;

	pr_info("%s: Temperature source: %s\n", __func__,
		temper_src_name[info->pdata->temper_src]);
	pr_info("%s: Recharge voltage: %d\n", __func__,
				info->pdata->recharge_voltage);

#if defined(CONFIG_S3C_ADC)
	info->adc_client = s3c_adc_register(pdev, NULL, NULL, 0);
#endif

	/* init battery info */
	info->full_charged_state = false;
	info->abstimer_state = false;
	info->recharge_phase = false;
	info->siop_charge_current = CHARGER_USB_CURRENT;
	info->monitor_mode = MONITOR_NORM;

	/* LPM charging state */
	info->lpm_state = lpcharge;

	wake_lock_init(&info->monitor_wake_lock, WAKE_LOCK_SUSPEND,
		       "battery-monitor");
	wake_lock_init(&info->emer_wake_lock, WAKE_LOCK_SUSPEND,
		       "battery-emergency");
	if (!info->pdata->suspend_chging)
		wake_lock_init(&info->charge_wake_lock,
			       WAKE_LOCK_SUSPEND, "battery-charging");

	/* Init wq for battery */
	INIT_WORK(&info->error_work, battery_error_work);
	INIT_WORK(&info->monitor_work, battery_monitor_work);

	/* Init Power supply class */
	info->psy_bat.name = "battery";
	info->psy_bat.type = POWER_SUPPLY_TYPE_BATTERY;
	info->psy_bat.properties = samsung_battery_props;
	info->psy_bat.num_properties = ARRAY_SIZE(samsung_battery_props);
	info->psy_bat.get_property = samsung_battery_get_property;
	info->psy_bat.set_property = samsung_battery_set_property;

	info->psy_usb.name = "usb";
	info->psy_usb.type = POWER_SUPPLY_TYPE_USB;
	info->psy_usb.supplied_to = supply_list;
	info->psy_usb.num_supplicants = ARRAY_SIZE(supply_list);
	info->psy_usb.properties = samsung_power_props;
	info->psy_usb.num_properties = ARRAY_SIZE(samsung_power_props);
	info->psy_usb.get_property = samsung_usb_get_property;

	info->psy_ac.name = "ac";
	info->psy_ac.type = POWER_SUPPLY_TYPE_MAINS;
	info->psy_ac.supplied_to = supply_list;
	info->psy_ac.num_supplicants = ARRAY_SIZE(supply_list);
	info->psy_ac.properties = samsung_power_props;
	info->psy_ac.num_properties = ARRAY_SIZE(samsung_power_props);
	info->psy_ac.get_property = samsung_ac_get_property;

	ret = power_supply_register(&pdev->dev, &info->psy_bat);
	if (ret) {
		pr_err("%s: failed to register psy_bat\n", __func__);
		goto err_psy_reg_bat;
	}

	ret = power_supply_register(&pdev->dev, &info->psy_usb);
	if (ret) {
		pr_err("%s: failed to register psy_usb\n", __func__);
		goto err_psy_reg_usb;
	}

	ret = power_supply_register(&pdev->dev, &info->psy_ac);
	if (ret) {
		pr_err("%s: failed to register psy_ac\n", __func__);
		goto err_psy_reg_ac;
	}

	/* Using android alarm for gauging instead of workqueue */
	info->last_poll = alarm_get_elapsed_realtime();
	alarm_init(&info->alarm, ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP,
		   samsung_battery_alarm_start);

	/* update battery init status */
	schedule_work(&info->monitor_work);

	/* Create samsung detail attributes */
	battery_create_attrs(info->psy_bat.dev);

	pr_info("%s: SAMSUNG Battery Driver Loaded\n", __func__);
	return 0;

 err_psy_reg_ac:
	power_supply_unregister(&info->psy_usb);
 err_psy_reg_usb:
	power_supply_unregister(&info->psy_bat);
 err_psy_reg_bat:
	wake_lock_destroy(&info->monitor_wake_lock);
	wake_lock_destroy(&info->emer_wake_lock);
	if (!info->pdata->suspend_chging)
		wake_lock_destroy(&info->charge_wake_lock);
 err_kfree:
	kfree(info);

	return ret;
}

static int __devexit samsung_battery_remove(struct platform_device *pdev)
{
	struct battery_info *info = platform_get_drvdata(pdev);

	remove_proc_entry("battery_info_proc", NULL);

	cancel_work_sync(&info->error_work);
	cancel_work_sync(&info->monitor_work);

	power_supply_unregister(&info->psy_bat);
	power_supply_unregister(&info->psy_usb);
	power_supply_unregister(&info->psy_ac);

	wake_lock_destroy(&info->monitor_wake_lock);
	wake_lock_destroy(&info->emer_wake_lock);
	if (!info->pdata->suspend_chging)
		wake_lock_destroy(&info->charge_wake_lock);

	kfree(info);

	return 0;
}

#ifdef CONFIG_PM
static int samsung_battery_prepare(struct device *dev)
{
	struct battery_info *info = dev_get_drvdata(dev);
	pr_info("%s\n", __func__);

	if (info->monitor_mode != MONITOR_EMER) {
		if (info->charge_real_state == POWER_SUPPLY_STATUS_CHARGING)
			info->monitor_mode = MONITOR_CHNG_SUSP;
		else
			info->monitor_mode = MONITOR_NORM_SUSP;
	}

	samsung_battery_next_monitor(info);

	return 0;
}

static void samsung_battery_complete(struct device *dev)
{
	struct battery_info *info = dev_get_drvdata(dev);
	pr_info("%s\n", __func__);

	info->monitor_mode = MONITOR_NORM;
}

static int samsung_battery_suspend(struct device *dev)
{
	struct battery_info *info = dev_get_drvdata(dev);
	pr_info("%s\n", __func__);

	flush_work_sync(&info->monitor_work);

	return 0;
}

static int samsung_battery_resume(struct device *dev)
{
	struct battery_info *info = dev_get_drvdata(dev);
	pr_info("%s\n", __func__);

	schedule_work(&info->monitor_work);

	return 0;
}

static const struct dev_pm_ops samsung_battery_pm_ops = {
	.prepare = samsung_battery_prepare,
	.complete = samsung_battery_complete,
	.suspend = samsung_battery_suspend,
	.resume = samsung_battery_resume,
};
#endif

static struct platform_driver samsung_battery_driver = {
	.driver = {
		.name = "samsung-battery",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &samsung_battery_pm_ops,
#endif
	},
	.probe = samsung_battery_probe,
	.remove = __devexit_p(samsung_battery_remove),
};

static int __init samsung_battery_init(void)
{
	return platform_driver_register(&samsung_battery_driver);
}

static void __exit samsung_battery_exit(void)
{
	platform_driver_unregister(&samsung_battery_driver);
}

late_initcall(samsung_battery_init);
module_exit(samsung_battery_exit);

MODULE_AUTHOR("Jaecheol Kim <jc22.kim@samsung.com>");
MODULE_DESCRIPTION("SAMSUNG battery driver");
MODULE_LICENSE("GPL");
