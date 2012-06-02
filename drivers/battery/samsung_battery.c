/*
 * samsung_battery.c
 *
 * Copyright (C) 2011 Samsung Electronics
 * SangYoung Son <hello.son@samsung.com>
 *
 * based on sec_battery.c
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
#if defined(CONFIG_STMPE811_ADC)
#include <linux/stmpe811-adc.h>
#endif

static char *supply_list[] = {
	"battery",
};

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
static void battery_notify_full_state(struct battery_info *info);
static bool battery_terminal_check_support(struct battery_info *info);
static void battery_error_control(struct battery_info *info);
#endif

/* Get LP charging mode state */
unsigned int lpcharge;
static int battery_get_lpm_state(char *str)
{
	get_option(&str, &lpcharge);
	pr_info("%s: Low power charging mode: %d\n", __func__, lpcharge);

	return lpcharge;
}
__setup("lpcharge=", battery_get_lpm_state);
#if defined(CONFIG_RTC_ALARM_BOOT)
EXPORT_SYMBOL(lpcharge);
#endif

/* Cable type from charger or adc */
static int battery_get_cable(struct battery_info *info)
{
	union power_supply_propval value;
	int cable_type = 0;

	pr_debug("%s\n", __func__);

	mutex_lock(&info->ops_lock);

	switch (info->pdata->cb_det_src) {
	case CABLE_DET_CHARGER:
		info->psy_charger->get_property(info->psy_charger,
				POWER_SUPPLY_PROP_ONLINE, &value);
		cable_type = value.intval;
		break;
	default:
		pr_err("%s: not support src(%d)\n", __func__,
				info->pdata->cb_det_src);
		cable_type = POWER_SUPPLY_TYPE_BATTERY;
		break;
	}

	mutex_unlock(&info->ops_lock);

	return cable_type;
}

/* Temperature from fuelgauge or adc */
static int battery_get_temper(struct battery_info *info)
{
	union power_supply_propval value;
	int cnt, adc, adc_max, adc_min, adc_total;
	int temper = 300;
	int retry_cnt;
	pr_debug("%s\n", __func__);

	mutex_lock(&info->ops_lock);

	switch (info->pdata->temper_src) {
	case TEMPER_FUELGAUGE:
		info->psy_fuelgauge->get_property(info->psy_fuelgauge,
					  POWER_SUPPLY_PROP_TEMP, &value);
		temper = value.intval;
		break;
	case TEMPER_AP_ADC:
#if defined(CONFIG_MACH_S2PLUS)
		if (system_rev < 2) {
			pr_info("%s: adc fixed as 30.0\n", __func__);
			temper = 300;
			mutex_unlock(&info->ops_lock);
			return temper;
		}
#endif
#if defined(CONFIG_S3C_ADC)
		adc = adc_max = adc_min = adc_total = 0;
		for (cnt = 0; cnt < CNT_ADC_SAMPLE; cnt++) {
			retry_cnt = 0;
			do {
				adc = s3c_adc_read(info->adc_client,
							info->pdata->temper_ch);
				if (adc < 0) {
					pr_info("%s: adc read(%d), retry(%d)",
						__func__, adc, retry_cnt++);
					msleep(ADC_ERR_DELAY);
				}
			} while (((adc < 0) && (retry_cnt <= ADC_ERR_CNT)));

			if (cnt != 0) {
				adc_max = MAX(adc, adc_max);
				adc_min = MIN(adc, adc_min);
			} else {
				adc_max = adc_min = adc;
			}

			adc_total += adc;
			pr_debug("%s: adc(%d), total(%d), max(%d), min(%d), "
					"avg(%d), cnt(%d)\n", __func__,
					adc, adc_total, adc_max, adc_min,
					adc_total / (cnt + 1),  cnt + 1);
		}

		info->battery_temper_adc =
			(adc_total - adc_max - adc_min) / (CNT_ADC_SAMPLE - 2);

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

	mutex_unlock(&info->ops_lock);
	return temper;
}

/* Get info from power supply at realtime */
int battery_get_info(struct battery_info *info,
		     enum power_supply_property property)
{
	union power_supply_propval value;
	value.intval = 0;

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	/* do nothing */
#else
	if (info->battery_error_test) {
		pr_info("%s: in test mode(%d), do not update\n", __func__,
			info->battery_error_test);
		return -EPERM;
	}
#endif

	switch (property) {
	/* Update from charger */
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_HEALTH:
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	case POWER_SUPPLY_PROP_CHARGE_FULL:
#endif
		info->psy_charger->get_property(info->psy_charger,
						property, &value);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		value.intval = battery_get_cable(info);
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
	int temper;

	/* Update from Charger */
	info->cable_type = battery_get_cable(info);

	info->psy_charger->get_property(info->psy_charger,
					POWER_SUPPLY_PROP_STATUS, &value);
	info->charge_real_state = info->charge_virt_state = value.intval;

	info->psy_charger->get_property(info->psy_charger,
					POWER_SUPPLY_PROP_CHARGE_TYPE, &value);
	info->charge_type = value.intval;

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)\
	|| defined(CONFIG_MACH_M0_CMCC)
	/* temperature error is higher priority */
	if (!info->temper_state) {
		info->psy_charger->get_property(info->psy_charger,
					POWER_SUPPLY_PROP_HEALTH, &value);
		info->battery_health = value.intval;
	}
#else
	info->psy_charger->get_property(info->psy_charger,
					POWER_SUPPLY_PROP_HEALTH, &value);
	info->battery_health = value.intval;
#endif

	info->psy_charger->get_property(info->psy_charger,
					POWER_SUPPLY_PROP_PRESENT, &value);
	info->battery_present = value.intval;

	info->psy_charger->get_property(info->psy_charger,
					POWER_SUPPLY_PROP_CURRENT_NOW, &value);
	info->charge_current = value.intval;

	info->psy_charger->get_property(info->psy_charger,
					POWER_SUPPLY_PROP_CURRENT_MAX, &value);
	info->input_current = value.intval;

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	if (info->cable_type == POWER_SUPPLY_TYPE_BATTERY) {
		info->battery_health = POWER_SUPPLY_HEALTH_GOOD;
		info->battery_present = 1;
	}
#endif

	/* Fuelgauge power off state */
	if ((info->cable_type != POWER_SUPPLY_TYPE_BATTERY) &&
	    (info->battery_present == 0)) {
		pr_info("%s: abnormal fuelgauge power state\n", __func__);
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
	info->battery_r_s_delta = value.intval - info->battery_raw_soc;
	info->battery_raw_soc = value.intval;

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	value.intval = SOC_TYPE_FULL;
	info->psy_fuelgauge->get_property(info->psy_fuelgauge,
					  POWER_SUPPLY_PROP_CAPACITY, &value);
	info->battery_full_soc = value.intval;
#endif

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
	info->battery_v_diff = info->battery_vcell - info->battery_vfocv;

	temper = battery_get_temper(info);
	info->battery_t_delta = temper - info->battery_temper;
	info->battery_temper = temper;

update_finish:
	switch (info->battery_error_test) {
	case 0:
		pr_debug("%s: error test: not test modde\n", __func__);
		break;
	case 1:
		pr_info("%s: error test: full charged\n", __func__);
		info->charge_real_state = POWER_SUPPLY_STATUS_FULL;
		info->battery_vcell = info->pdata->voltage_max;
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
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
#if defined(CONFIG_CHARGER_MAX8922_U1)
#if defined(CONFIG_MACH_S2PLUS)
		if (system_rev >= 2) {
			info->psy_sub_charger->set_property(
						info->psy_sub_charger,
						property, &value);
		} else {
			info->psy_charger->set_property(info->psy_charger,
						property, &value);
		}
#else
		info->psy_sub_charger->set_property(info->psy_sub_charger,
						property, &value);
#endif
#else
		info->psy_charger->set_property(info->psy_charger,
						property, &value);
#endif
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

static void samsung_battery_alarm_start(struct alarm *alarm)
{
	struct battery_info *info = container_of(alarm, struct battery_info,
						 alarm);
	pr_debug("%s\n", __func__);

	wake_lock(&info->monitor_wake_lock);
	schedule_work(&info->monitor_work);
}

static void battery_monitor_interval(struct battery_info *info)
{
	ktime_t interval, next, slack;
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
	case MONITOR_EMER_LV1:
		info->monitor_interval = info->pdata->emer_lv1_interval;
		break;
	case MONITOR_EMER_LV2:
		info->monitor_interval = info->pdata->emer_lv2_interval;
		break;
	default:
		info->monitor_interval = info->pdata->norm_interval;
		break;
	}

	/* apply monitor interval weight */
	if (info->monitor_weight != 100) {
		pr_info("%s: apply weight(%d), %d -> %d\n", __func__,
			info->monitor_weight, info->monitor_interval,
			(info->monitor_interval * info->monitor_weight / 100));
		info->monitor_interval *= info->monitor_weight;
		info->monitor_interval /= 100;
	}

	pr_debug("%s: monitor mode(%d), interval(%d)\n", __func__,
		info->monitor_mode, info->monitor_interval);

	interval = ktime_set(info->monitor_interval, 0);
	next = ktime_add(info->last_poll, interval);
	slack = ktime_set(20, 0);

	alarm_start_range(&info->alarm, next, ktime_add(next, slack));

	local_irq_restore(flags);
}

static bool battery_recharge_cond(struct battery_info *info)
{
	pr_debug("%s\n", __func__);

	if (info->charge_real_state == POWER_SUPPLY_STATUS_CHARGING) {
		pr_debug("%s: r_state chargng, cs(%d)\n", __func__,
					info->charge_real_state);
		return false;
	}

	if (info->battery_vcell < info->pdata->recharge_voltage) {
		pr_info("%s: recharge state, vcell(%d ? %d)\n", __func__,
			info->battery_vcell, info->pdata->recharge_voltage);
		return true;
	} else
		pr_debug("%s: not recharge state, vcell(%d ? %d)\n", __func__,
			info->battery_vcell, info->pdata->recharge_voltage);

	return false;
}

static bool battery_abstimer_cond(struct battery_info *info)
{
	unsigned int abstimer_duration;
	ktime_t ktime;
	struct timespec current_time;
	pr_debug("%s\n", __func__);

	if ((info->cable_type == POWER_SUPPLY_TYPE_USB) ||
		(info->full_charged_state == true) ||
		(info->charge_start_time == 0)) {
		pr_debug("%s: not abstimer state, cb(%d), f(%d), t(%d)\n",
						__func__, info->cable_type,
						info->full_charged_state,
						info->charge_start_time);
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
		pr_info("%s: abstimer state, t(%d - %d ?? %d)\n", __func__,
			(int)current_time.tv_sec, info->charge_start_time,
							abstimer_duration);
		info->abstimer_state = true;
	} else {
		pr_debug("%s: not abstimer state, t(%d - %d ?? %d)\n", __func__,
			(int)current_time.tv_sec, info->charge_start_time,
							abstimer_duration);
		info->abstimer_state = false;
	}

	return info->abstimer_state;
}

static bool battery_fullcharged_cond(struct battery_info *info)
{
	int f_cond_soc;
	int f_cond_vcell;
	pr_debug("%s\n", __func__);

	/* max voltage - 50mV */
	f_cond_vcell = info->pdata->voltage_max - 50000;
	/* max soc - 5% */
	f_cond_soc = 95;

	pr_debug("%s: cs(%d ? %d), v(%d ? %d), s(%d ? %d)\n", __func__,
			info->charge_real_state, POWER_SUPPLY_STATUS_FULL,
			info->battery_vcell, f_cond_vcell,
			info->battery_soc, f_cond_soc);

	if (info->charge_real_state == POWER_SUPPLY_STATUS_FULL) {
		if ((info->battery_vcell > f_cond_vcell) &&
		    (info->battery_soc > f_cond_soc)) {
			pr_info("%s: real full charged, v(%d), s(%d)\n",
					__func__, info->battery_vcell,
						info->battery_soc);
			info->full_charged_state = true;
			return true;
		} else {
			pr_info("%s: charger full charged, v(%d), s(%d)\n",
					__func__, info->battery_vcell,
						info->battery_soc);

			/* restart charger */
			battery_control_info(info,
					     POWER_SUPPLY_PROP_STATUS,
					     DISABLE);
			msleep(100);
			battery_control_info(info,
					     POWER_SUPPLY_PROP_STATUS,
					     ENABLE);

			info->charge_real_state = info->charge_virt_state =
						battery_get_info(info,
						POWER_SUPPLY_PROP_STATUS);
			return false;
		}
	} else if (info->full_charged_state == true) {
		pr_debug("%s: already full charged, v(%d), s(%d)\n", __func__,
				info->battery_vcell, info->battery_soc);
	} else {
		pr_debug("%s: not full charged, v(%d), s(%d)\n", __func__,
				info->battery_vcell, info->battery_soc);
		info->full_charged_state = false;
	}

	return false;
}

static bool battery_vf_cond(struct battery_info *info)
{
	pr_debug("%s\n", __func__);

#if defined(CONFIG_MACH_P11) || defined(CONFIG_MACH_P10)
	/* FIXME: fix P11 build error temporarily */
#else
	/* jig detect by MUIC */
	if (is_jig_attached == JIG_ON) {
		pr_info("%s: JIG ON, do not check\n", __func__);
		info->vf_state = false;
		return false;
	}
#endif

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	if (info->cable_type == POWER_SUPPLY_TYPE_BATTERY) {
		info->vf_state = false;
		return false;
	}
#endif

	/* real time state */
	info->battery_present =
		battery_get_info(info, POWER_SUPPLY_PROP_PRESENT);

	if (info->battery_present == 0) {
		pr_info("%s: battery(%d) is not detected\n", __func__,
						info->battery_present);
		info->vf_state = true;
	} else {
		pr_debug("%s: battery(%d) is detected\n", __func__,
						info->battery_present);
		info->vf_state = false;
	}

	return info->vf_state;
}

static bool battery_health_cond(struct battery_info *info)
{
	pr_debug("%s\n", __func__);

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	if (info->cable_type == POWER_SUPPLY_TYPE_BATTERY) {
		info->health_state = false;
		info->is_unspec_phase = false;
		return false;
	}
#endif

	/* temperature error is higher priority */
	if (info->temper_state == true) {
		pr_info("%s: temper stop state, do not check\n", __func__);
		return false;
	}

	/* real time state */
	info->battery_health =
		battery_get_info(info, POWER_SUPPLY_PROP_HEALTH);

	if (info->battery_health == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
		pr_info("%s: battery unspec(%d)\n", __func__,
					info->battery_health);
		info->health_state = true;
	} else if (info->battery_health == POWER_SUPPLY_HEALTH_DEAD) {
		pr_info("%s: battery dead(%d)\n", __func__,
					info->battery_health);
		info->health_state = true;
	} else if (info->battery_health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) {
		pr_info("%s: battery overvoltage(%d)\n", __func__,
					info->battery_health);
		info->health_state = true;
	} else if (info->battery_health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE) {
		pr_info("%s: battery undervoltage(%d)\n", __func__,
					info->battery_health);
		info->health_state = true;
	} else {
		pr_debug("%s: battery good(%d)\n", __func__,
					info->battery_health);
		info->health_state = false;

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
		if (battery_terminal_check_support(info))
			pr_err("%s: health support, error state!\n", __func__);
#endif
	}

	return info->health_state;
}

static bool battery_temper_cond(struct battery_info *info)
{
	pr_debug("%s\n", __func__);

	if (info->temper_state == false) {
		if (info->charge_real_state != POWER_SUPPLY_STATUS_CHARGING) {
			pr_debug("%s: r_state !charging, cs(%d)\n",
					__func__, info->charge_real_state);
			return false;
		}

		pr_debug("%s: check charging stop temper "
			 "cond: %d ?? %d ~ %d\n", __func__,
			 info->battery_temper,
			 info->pdata->freeze_stop_temp,
			 info->pdata->overheat_stop_temp);

		if (info->battery_temper >=
		    info->pdata->overheat_stop_temp) {
			pr_info("%s: stop by overheated, t(%d ? %d)\n",
					__func__, info->battery_temper,
					info->pdata->overheat_stop_temp);
			info->overheated_state = true;
		} else if (info->battery_temper <=
			   info->pdata->freeze_stop_temp) {
			pr_info("%s: stop by overheated, t(%d ? %d)\n",
					__func__, info->battery_temper,
					info->pdata->freeze_stop_temp);
			info->freezed_state = true;
		} else
			pr_debug("%s: normal charging, t(%d)\n", __func__,
							info->battery_temper);
	} else {
		pr_debug("%s: check charging recovery temper "
			 "cond: %d ?? %d ~ %d\n", __func__,
			 info->battery_temper,
			 info->pdata->freeze_recovery_temp,
			 info->pdata->overheat_recovery_temp);

		if ((info->overheated_state == true) &&
		    (info->battery_temper <=
		     info->pdata->overheat_recovery_temp)) {
			pr_info("%s: recovery from overheated, t(%d ? %d)\n",
					__func__, info->battery_temper,
					info->pdata->overheat_recovery_temp);
			info->overheated_state = false;
		} else if ((info->freezed_state == true) &&
			   (info->battery_temper >=
			    info->pdata->freeze_recovery_temp)) {
			pr_info("%s: recovery from freezed, t(%d ? %d)\n",
					__func__, info->battery_temper,
					info->pdata->freeze_recovery_temp);
			info->freezed_state = false;
		} else
			pr_info("%s: charge stopped, t(%d)\n", __func__,
							info->battery_temper);
	}

	if (info->overheated_state == true) {
		info->battery_health = POWER_SUPPLY_HEALTH_OVERHEAT;
		info->freezed_state = false;
		info->temper_state = true;
	} else if (info->freezed_state == true) {
		info->battery_health = POWER_SUPPLY_HEALTH_COLD;
		info->overheated_state = false;
		info->temper_state = true;
	} else {
#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
		info->battery_health = POWER_SUPPLY_HEALTH_GOOD;
#endif
		info->overheated_state = false;
		info->freezed_state = false;
		info->temper_state = false;
	}

	return info->temper_state;
}

static void battery_charge_control(struct battery_info *info,
				unsigned int chg_curr, unsigned int in_curr)
{
	int charge_state;
	ktime_t ktime;
	struct timespec current_time;
	pr_debug("%s, chg(%d), in(%d)\n", __func__, chg_curr, in_curr);

	mutex_lock(&info->ops_lock);

	ktime = alarm_get_elapsed_realtime();
	current_time = ktime_to_timespec(ktime);

	if ((chg_curr != 0) && (info->siop_state == true)) {
		pr_info("%s: siop state, charge current is %dmA\n", __func__,
			 info->siop_charge_current);
		chg_curr = info->siop_charge_current;
	}

	if (in_curr == KEEP_CURR)
		goto charge_current_con;

	/* input current limit */
	in_curr = min(in_curr, info->pdata->in_curr_limit);

	/* check charge input before and after */
	if (info->input_current == ((in_curr / 20) * 20)) {
		/*
		 * (current / 20) is converted value
		 * for register setting.
		 * (register current * 20) is actual value
		 * for input current
		 */
		pr_debug("%s: same input current: %dmA\n", __func__,  in_curr);
	} else {
		battery_control_info(info,
				     POWER_SUPPLY_PROP_CURRENT_MAX,
				     in_curr);
		info->input_current =
			battery_get_info(info, POWER_SUPPLY_PROP_CURRENT_MAX);

		pr_debug("%s: update input current: %dmA\n", __func__, in_curr);
	}

charge_current_con:
	if (chg_curr == KEEP_CURR)
		goto charge_state_con;

	/* check charge current before and after */
	if (info->charge_current == ((chg_curr * 3 / 100) * 333 / 10)) {
		/*
		 * (current * 3 / 100) is converted value
		 * for register setting.
		 * (register current * 333 / 10) is actual value
		 * for charge current
		 */
		pr_debug("%s: same charge current: %dmA\n",
					__func__, chg_curr);
	} else {
		battery_control_info(info,
				     POWER_SUPPLY_PROP_CURRENT_NOW,
				     chg_curr);
		info->charge_current =
			battery_get_info(info, POWER_SUPPLY_PROP_CURRENT_NOW);

		pr_debug("%s: update charge current: %dmA\n",
					__func__, chg_curr);
	}

charge_state_con:
	/* control charger control only, buck is on by default */
	if ((chg_curr != 0) && (info->charge_start_time == 0)) {
		battery_control_info(info, POWER_SUPPLY_PROP_STATUS, ENABLE);

		info->charge_start_time = current_time.tv_sec;
		pr_err("%s: charge enabled, current as %d/%dmA @%d\n",
			__func__, info->charge_current, info->input_current,
			info->charge_start_time);

		charge_state = battery_get_info(info, POWER_SUPPLY_PROP_STATUS);

		if (charge_state != POWER_SUPPLY_STATUS_CHARGING) {
			pr_info("%s: force set v_state as charging\n",
								__func__);
			info->charge_real_state = charge_state;
			info->charge_virt_state = POWER_SUPPLY_STATUS_CHARGING;
			info->ambiguous_state = true;
		} else {
			info->charge_real_state = info->charge_virt_state =
								charge_state;
			info->ambiguous_state = false;
		}
	} else if ((chg_curr == 0) && (info->charge_start_time != 0)) {
		battery_control_info(info, POWER_SUPPLY_PROP_STATUS, DISABLE);

		pr_err("%s: charge disabled, current as %d/%dmA @%d\n",
			__func__, info->charge_current, info->input_current,
			(int)current_time.tv_sec);

		info->charge_start_time = 0;

		charge_state = battery_get_info(info, POWER_SUPPLY_PROP_STATUS);

		if (charge_state != POWER_SUPPLY_STATUS_DISCHARGING) {
			pr_info("%s: force set v_state as discharging\n",
								__func__);
			info->charge_real_state = charge_state;
			info->charge_virt_state =
						POWER_SUPPLY_STATUS_DISCHARGING;
			info->ambiguous_state = true;
		} else {
			info->charge_real_state = info->charge_virt_state =
								charge_state;
			info->ambiguous_state = false;
		}
	} else {
		pr_debug("%s: same charge state(%s), current as %d/%dmA @%d\n",
			__func__, ((chg_curr != 0) ? "enabled" : "disabled"),
				info->charge_current, info->input_current,
				info->charge_start_time);

		/* release ambiguous state */
		if ((info->ambiguous_state == true) &&
			(info->charge_real_state == info->charge_virt_state)) {
			pr_debug("%s: release ambiguous state, s(%d)\n",
					__func__, info->charge_real_state);
			info->ambiguous_state = false;
		}
	}

	mutex_unlock(&info->ops_lock);
}

/* charge state for UI(icon) */
static void battery_indicator_icon(struct battery_info *info)
{
	if (info->cable_type != POWER_SUPPLY_TYPE_BATTERY) {
		if (info->full_charged_state == true) {
			info->charge_virt_state =
				POWER_SUPPLY_STATUS_FULL;
			info->battery_soc = 100;
		} else if (info->abstimer_state == true) {
			info->charge_virt_state =
				POWER_SUPPLY_STATUS_CHARGING;
		} else if (info->recharge_phase == true) {
			info->charge_virt_state =
				POWER_SUPPLY_STATUS_CHARGING;
		}

		if (info->temper_state == true) {
			info->charge_virt_state =
				POWER_SUPPLY_STATUS_NOT_CHARGING;
		}

		if (info->vf_state == true) {
			info->charge_virt_state =
				POWER_SUPPLY_STATUS_NOT_CHARGING;
			info->battery_health =
				POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		}

		if (info->health_state == true) {
			/* ovp is not 'NOT_CHARGING' */
			if (info->battery_health ==
						POWER_SUPPLY_HEALTH_OVERVOLTAGE)
				info->charge_virt_state =
					POWER_SUPPLY_STATUS_DISCHARGING;
			else
				info->charge_virt_state =
					POWER_SUPPLY_STATUS_NOT_CHARGING;
		}
	}
}

/* charge state for LED */
static void battery_indicator_led(struct battery_info *info)
{

	if (info->charge_virt_state ==
			POWER_SUPPLY_STATUS_CHARGING) {
		if (info->led_state != BATT_LED_CHARGING) {
			/* TODO: for kernel LED control: CHARGING */
			info->led_state = BATT_LED_CHARGING;
		}
	} else if (info->charge_virt_state ==
			POWER_SUPPLY_STATUS_NOT_CHARGING) {
		if (info->led_state != BATT_LED_NOT_CHARGING) {
			/* TODO: for kernel LED control: NOT CHARGING */
			info->led_state = BATT_LED_NOT_CHARGING;
		}
	} else if (info->charge_virt_state ==
			POWER_SUPPLY_STATUS_FULL) {
		if (info->led_state != BATT_LED_FULL) {
			/* TODO: for kernel LED control: FULL */
			info->led_state = BATT_LED_FULL;
		}
	} else {
		if (info->led_state != BATT_LED_DISCHARGING) {
			/* TODO: for kernel LED control: DISCHARGING */
			info->led_state = BATT_LED_DISCHARGING;
		}
	}
}

/* dynamic battery polling interval */
static void battery_interval_calulation(struct battery_info *info)
{
	pr_debug("%s\n", __func__);

	/* init monitor interval weight */
	info->monitor_weight = 100;

	/* ambiguous state */
	if (info->ambiguous_state == true) {
		pr_info("%s: ambiguous state\n", __func__);
		info->monitor_mode = MONITOR_EMER_LV2;
		wake_lock(&info->emer_wake_lock);
		return;
	} else {
		pr_debug("%s: not ambiguous state\n", __func__);
		wake_unlock(&info->emer_wake_lock);
	}

	/* prevent critical low raw soc factor */
	if (info->battery_raw_soc < 100) {
		pr_info("%s: soc(%d) too low state\n", __func__,
						info->battery_raw_soc);
		info->monitor_mode = MONITOR_EMER_LV2;
		wake_lock(&info->emer_wake_lock);
		return;
	} else {
		pr_debug("%s: soc(%d) not too low state\n", __func__,
						info->battery_raw_soc);
		wake_unlock(&info->emer_wake_lock);
	}

	/* prevent critical low voltage factor */
	if ((info->battery_vcell < (info->pdata->voltage_min - 100000)) ||
		(info->battery_vfocv < info->pdata->voltage_min)) {
		pr_info("%s: voltage(%d) too low state\n", __func__,
						info->battery_vcell);
		info->monitor_mode = MONITOR_EMER_LV2;
		wake_lock(&info->emer_wake_lock);
		return;
	} else {
		pr_debug("%s: voltage(%d) not too low state\n", __func__,
						info->battery_vcell);
		wake_unlock(&info->emer_wake_lock);
	}

	/* charge state factor */
	if (info->charge_virt_state ==
				POWER_SUPPLY_STATUS_CHARGING) {
		pr_debug("%s: v_state charging\n", __func__);
		info->monitor_mode = MONITOR_CHNG;
		wake_unlock(&info->emer_wake_lock);
#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
		if ((info->prev_cable_type == POWER_SUPPLY_TYPE_BATTERY &&
			info->cable_type != POWER_SUPPLY_TYPE_BATTERY) &&
			(info->battery_temper >=
				 info->pdata->overheat_stop_temp ||
			info->battery_temper <=
				info->pdata->freeze_stop_temp)) {
			pr_info("%s : re-check temper condition\n", __func__);
			info->monitor_mode = MONITOR_EMER_LV2;
		}
#endif
	} else if (info->charge_virt_state ==
				POWER_SUPPLY_STATUS_NOT_CHARGING) {
		pr_debug("%s: emergency(not charging) state\n", __func__);
		info->monitor_mode = MONITOR_EMER_LV2;
		wake_lock(&info->emer_wake_lock);
		return;
	} else {
		pr_debug("%s: normal state\n", __func__);
		info->monitor_mode = MONITOR_NORM;
		wake_unlock(&info->emer_wake_lock);
	}

	/*
	 * in LPM state, set default weight as 200%
	 */
	 if (info->lpm_state == true)
		info->monitor_weight *= 2;

	/* 2 times after boot(about 1min), apply charging interval(30sec) */
	 if (info->monitor_count < 2) {
		pr_info("%s: now in booting, set 30s\n", __func__);
		info->monitor_mode = MONITOR_EMER_LV1;
		return;
	 }

	/*
	 * prevent low voltage phase
	 * default, vcell is lower than min_voltage + 50mV, -20%
	 */
	if (info->battery_vcell < (info->pdata->voltage_min + 50000)) {
		info->monitor_mode = MONITOR_EMER_LV1;
		info->monitor_weight -= 30;
		pr_info("%s: low v(%d), weight(%d)\n", __func__,
				info->battery_vcell, info->monitor_weight);
	}

	/*
	 * prevent high current state(both charging and discharging
	 * default, (v diff = vcell - vfocv)
	 * charging, v_diff is higher than 250mV, (%dmV / 1000)%
	 * discharging, v_diff is higher than 100mV, (%dmV / 1000)%
	 * both, v_diff is lower than 10mV, +20%
	 */
	if ((info->battery_v_diff > 250000) ||
		(info->battery_v_diff < -100000)) {
		info->monitor_weight += (info->battery_v_diff / 10000);
		pr_info("%s: v diff(%d), weight(%d)\n", __func__,
			ABS(info->battery_v_diff), info->monitor_weight);
	} else if ((ABS(info->battery_v_diff)) < 50000) {
		info->monitor_weight += 20;
		pr_info("%s: v diff(%d), weight(%d)\n", __func__,
			ABS(info->battery_v_diff), info->monitor_weight);
	}

	/*
	 * prevent raw soc changable phase
	 * default, raw soc X.YY%, YY% is in under 0.1% or upper 0.9%, -10%
	 */
	if ((((info->battery_raw_soc % 100) < 10) &&
					(info->battery_v_diff < 0)) ||
		(((info->battery_raw_soc % 100) > 90) &&
					(info->battery_v_diff > 0))) {
		info->monitor_weight -= 10;
		pr_info("%s: raw soc(%d), weight(%d)\n", __func__,
			info->battery_raw_soc, info->monitor_weight);
	}

	/*
	 * prevent high slope raw soc change
	 * default, raw soc delta is higher than 0.5%, -(raw soc delta / 5)%
	 */
	if (ABS(info->battery_r_s_delta) > 50) {
		info->monitor_weight -= (ABS(info->battery_r_s_delta)) / 5;
		pr_info("%s: raw soc delta(%d), weight(%d)\n", __func__,
			ABS(info->battery_r_s_delta), info->monitor_weight);
	}

	/*
	 * prevent high/low temper phase
	 * default, temper is in (overheat temp - 5'C) or (freeze temp + 5'C)
	 */
	if ((info->battery_temper > (info->pdata->overheat_stop_temp - 50)) ||
		(info->battery_temper < (info->pdata->freeze_stop_temp + 50))) {
		info->monitor_weight -= 20;
		pr_info("%s: temper(%d ? %d - %d), weight(%d)\n", __func__,
					info->battery_temper,
					(info->pdata->overheat_stop_temp - 50),
					(info->pdata->freeze_stop_temp + 50),
					info->monitor_weight);
	}

	/*
	 * prevent high slope temper change
	 * default, temper delta is higher than 2.00'C
	 */
	if (ABS(info->battery_t_delta) > 20) {
		info->monitor_weight -= 20;
		pr_info("%s: temper delta(%d), weight(%d)\n", __func__,
			ABS(info->battery_t_delta), info->monitor_weight);
	}

	/* prevent too low or too high weight, 10 ~ 150% */
	info->monitor_weight = MIN(MAX(info->monitor_weight, 10), 150);

	if (info->monitor_weight != 100)
		pr_info("%s: weight(%d)\n", __func__, info->monitor_weight);
}

static void battery_monitor_work(struct work_struct *work)
{
	struct battery_info *info = container_of(work, struct battery_info,
						 monitor_work);
	pr_debug("%s\n", __func__);

	mutex_lock(&info->mon_lock);

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	/* first, check cable-type */
	info->cable_type = battery_get_cable(info);
#endif

	/* If battery is not connected, clear flag for charge scenario */
	if ((battery_vf_cond(info) == true) ||
		(battery_health_cond(info) == true)) {
		pr_info("%s: battery error\n", __func__);
		info->overheated_state = false;
		info->freezed_state = false;
		info->temper_state = false;
		info->full_charged_state = false;
		info->abstimer_state = false;
		info->recharge_phase = false;

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
		pr_info("%s: not support standever...\n", __func__);
		battery_error_control(info);
#else
		if (info->pdata->battery_standever == true) {
			pr_info("%s: support standever\n", __func__);
			schedule_work(&info->error_work);
		} else {
			pr_info("%s: not support standever\n", __func__);
			battery_charge_control(info, OFF_CURR, OFF_CURR);
		}
#endif
	}

	/* Check battery state from charger and fuelgauge */
	battery_update_info(info);

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	/* check unspec recovery */
	if (info->is_unspec_phase) {
		if ((info->battery_health !=
			POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) &&
			(battery_terminal_check_support(info) == false)) {
			pr_info("%s: recover from unspec phase!\n", __func__);
			info->is_unspec_recovery = true;
		}
	}

	/* If it's recovery phase from unspec state, go to charge_ok */
	if (info->is_unspec_recovery) {
		pr_info("%s: recovered from unspec phase"
			": re-setting charge current!\n", __func__);
		battery_charge_control(info, OFF_CURR, OFF_CURR);
		info->is_unspec_recovery = false;
		info->is_unspec_phase = false;
		goto charge_ok;
	}
#endif

	/* if battery is missed state, do not check charge scenario */
	if (info->battery_present == 0)
		goto monitor_finish;

	/* If charger is not connected, do not check charge scenario */
	if (info->cable_type == POWER_SUPPLY_TYPE_BATTERY)
		goto charge_ok;

	/* Below is charger is connected state */
	if (battery_temper_cond(info) == true) {
		pr_info("%s: charge stopped by temperature\n", __func__);
		battery_charge_control(info, OFF_CURR, OFF_CURR);
		goto monitor_finish;
	}

	if (battery_fullcharged_cond(info) == true) {
		pr_info("%s: full charged state\n", __func__);
		battery_charge_control(info, OFF_CURR, KEEP_CURR);
		info->recharge_phase = true;
		goto monitor_finish;
	}

	if (battery_abstimer_cond(info) == true) {
		pr_info("%s: abstimer state\n", __func__);
		battery_charge_control(info, OFF_CURR, OFF_CURR);
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
	pr_err("%s: Updated Cable State(%d)\n", __func__, info->cable_type);
	switch (info->cable_type) {
	case POWER_SUPPLY_TYPE_BATTERY:
		if (!info->pdata->suspend_chging)
			wake_unlock(&info->charge_wake_lock);
		battery_charge_control(info, OFF_CURR, OFF_CURR);

		/* clear charge scenario state */
		info->overheated_state = false;
		info->freezed_state = false;
		info->temper_state = false;
		info->full_charged_state = false;
		info->abstimer_state = false;
		info->recharge_phase = false;
		break;
	case POWER_SUPPLY_TYPE_MAINS:
		if (!info->pdata->suspend_chging)
			wake_lock(&info->charge_wake_lock);
		battery_charge_control(info, info->pdata->chg_curr_ta,
						info->pdata->chg_curr_ta);
		break;
	case POWER_SUPPLY_TYPE_USB:
		if (!info->pdata->suspend_chging)
			wake_lock(&info->charge_wake_lock);
		battery_charge_control(info, info->pdata->chg_curr_usb,
						info->pdata->chg_curr_usb);
		break;
	case POWER_SUPPLY_TYPE_USB_CDP:
		if (!info->pdata->suspend_chging)
			wake_lock(&info->charge_wake_lock);
		battery_charge_control(info, info->pdata->chg_curr_cdp,
						info->pdata->chg_curr_cdp);
		break;
	case POWER_SUPPLY_TYPE_DOCK:
		if (!info->pdata->suspend_chging)
			wake_lock(&info->charge_wake_lock);
		battery_charge_control(info, info->pdata->chg_curr_dock,
						info->pdata->chg_curr_dock);
		break;
	case POWER_SUPPLY_TYPE_WIRELESS:
		if (!info->pdata->suspend_chging)
			wake_lock(&info->charge_wake_lock);
		battery_charge_control(info, info->pdata->chg_curr_wpc,
						info->pdata->chg_curr_wpc);
		break;
	default:
		break;
	}

monitor_finish:
	/* icon indicator */
	battery_indicator_icon(info);

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	battery_notify_full_state(info);
#endif

	/* dynamic battery polling interval */
	battery_interval_calulation(info);

	/* prevent suspend before starting the alarm */
	battery_monitor_interval(info);

	/* led indictor */
	if (info->pdata->led_indicator == true)
		battery_indicator_led(info);

	pr_info("[%d] bat: s(%d, %d), v(%d, %d), b(%d), "
		"t(%d.%d), h(%d), "
		"cs(%d, %d), cb(%d), cr(%d, %d), "
		"a(%d), f(%d), r(%d), t(%d)\n",
		++info->monitor_count,
		info->battery_soc,
		info->battery_r_s_delta,
		info->battery_vcell / 1000,
		info->battery_v_diff / 1000,
		info->battery_present,
		info->battery_temper / 10, info->battery_temper % 10,
		info->battery_health,
		info->charge_real_state,
		info->charge_virt_state,
		info->cable_type,
		info->charge_current,
		info->input_current,
		info->abstimer_state,
		info->full_charged_state,
		info->recharge_phase,
		info->charge_start_time);

	power_supply_changed(&info->psy_bat);

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	/* prevent suspend for ui-update */
	if (info->prev_cable_type != info->cable_type ||
		info->prev_battery_health != info->battery_health ||
		info->prev_charge_virt_state != info->charge_virt_state ||
		info->prev_battery_soc != info->battery_soc) {
		/* TBD : timeout value */
		pr_info("%s : update wakelock (%d)\n", __func__, 3 * HZ);
		wake_lock_timeout(&info->update_wake_lock, 3 * HZ);
	}

	info->prev_cable_type = info->cable_type;
	info->prev_battery_health = info->battery_health;
	info->prev_charge_virt_state = info->charge_virt_state;
	info->prev_battery_soc = info->battery_soc;
#endif

	/* if cable is detached in lpm, guarantee some secs for playlpm */
	if ((info->lpm_state == true) &&
		(info->cable_type == POWER_SUPPLY_TYPE_BATTERY)) {
		pr_info("%s: lpm with battery, maybe power off\n", __func__);
		wake_lock_timeout(&info->monitor_wake_lock, 10 * HZ);
	} else
		wake_lock_timeout(&info->monitor_wake_lock, HZ);

	mutex_unlock(&info->mon_lock);

	return;
}

static void battery_error_work(struct work_struct *work)
{
	struct battery_info *info = container_of(work, struct battery_info,
						 error_work);
	int err_cnt;
	int old_vcell, new_vcell, vcell_diff;
	pr_info("%s\n", __func__);

	mutex_lock(&info->err_lock);

	if ((info->vf_state == true) || (info->health_state == true)) {
		pr_info("%s: battery error state\n", __func__);
		old_vcell = info->battery_vcell;
		new_vcell = 0;
		for (err_cnt = 1; err_cnt <= VF_CHECK_COUNT; err_cnt++) {
#if defined(CONFIG_MACH_P11) || defined(CONFIG_MACH_P10)
			/* FIXME: fix P11 build error temporarily */
#else
			if (is_jig_attached == JIG_ON) {
				pr_info("%s: JIG detected, return\n", __func__);
				mutex_unlock(&info->err_lock);
				return;
			}
#endif
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
				battery_charge_control(info, OFF_CURR,
								OFF_CURR);
			}
		}
	} else
		pr_info("%s: unexpected error\n", __func__);

	mutex_unlock(&info->err_lock);
	return;
}

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
static void battery_notify_full_state(struct battery_info *info)
{
	union power_supply_propval value;

	if ((info->recharge_phase && info->full_charged_state) ||
		((info->battery_raw_soc > info->battery_full_soc) &&
		(info->battery_soc == 100))) {
		/* notify full state to fuel guage */
		value.intval = POWER_SUPPLY_STATUS_FULL;
		info->psy_fuelgauge->set_property(info->psy_fuelgauge,
			POWER_SUPPLY_PROP_STATUS, &value);
	}
}

static bool battery_terminal_check_support(struct battery_info *info)
{
	int full_mode;
	int vcell;
	bool ret = false;
	pr_debug("%s\n", __func__);

	full_mode = battery_get_info(info, POWER_SUPPLY_PROP_CHARGE_FULL);
	vcell = battery_get_info(info, POWER_SUPPLY_PROP_VOLTAGE_NOW);
	pr_debug("%s: chg_status = %d, vcell = %d\n",
			__func__, full_mode, vcell);

	if (full_mode && (vcell <= 3800000)) {
		pr_info("%s: top-off or done mode, but low voltage(%d)\n",
				__func__, vcell / 1000);
		/* check again */
		vcell = battery_get_info(info, POWER_SUPPLY_PROP_VOLTAGE_NOW);
		if (vcell <= 3800000) {
			pr_info("%s: top-off or done mode, but low voltage(%d), "
				"set health error!\n",
				__func__, vcell / 1000);
			info->health_state = true;
			ret = true;
		}
	}

	return ret;
}

static void battery_error_control(struct battery_info *info)
{
	pr_info("%s\n", __func__);

	if (info->vf_state == true) {
		pr_info("%s: battery vf error state\n", __func__);

		/* check again */
		info->battery_present =	battery_get_info(info,
					POWER_SUPPLY_PROP_PRESENT);

		if (info->battery_present == 0) {
			pr_info("%s: battery vf error, "
				"disable charging and off the system path!\n",
				__func__);
			battery_charge_control(info, OFF_CURR, OFF_CURR);
			info->charge_virt_state =
				POWER_SUPPLY_STATUS_NOT_CHARGING;
		}
	} else if (info->health_state == true) {
		if ((info->battery_health ==
			POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) ||
			battery_terminal_check_support(info)) {
			pr_info("%s: battery unspec, "
				"disable charging and off the "
				"system path!\n", __func__);

			/* invalid top-off state,
				assume terminals(+/-) open */
			battery_charge_control(info, OFF_CURR, OFF_CURR);
			pr_info("%s: set unspec phase!\n", __func__);
			info->is_unspec_phase = true;
		} else if (info->battery_health ==
				POWER_SUPPLY_HEALTH_OVERVOLTAGE)
			pr_info("%s: vbus ovp state!", __func__);
	}

	return;
}
#endif

/* Support property from battery */
static enum power_supply_property samsung_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_MAX,
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

	/* re-update indicator icon */
	battery_indicator_icon(info);

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
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = info->input_current;
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
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = info->pdata->voltage_max;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = info->pdata->voltage_min;
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

	if (info->is_suspended) {
		pr_info("%s: now in suspend\n", __func__);
		return 0;
	}

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_HEALTH:
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_TECHNOLOGY:
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CAPACITY:
	case POWER_SUPPLY_PROP_TEMP:
		break;
	default:
		return -EINVAL;
	}

	cancel_work_sync(&info->monitor_work);
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
		(info->cable_type == POWER_SUPPLY_TYPE_DOCK) ||
		(info->cable_type == POWER_SUPPLY_TYPE_WIRELESS);

	return 0;
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
		goto err_psy_get;
	}
	info->charger_name = info->pdata->charger_name;
	info->fuelgauge_name = info->pdata->fuelgauge_name;
#if defined(CONFIG_CHARGER_MAX8922_U1)
	if (system_rev >= 2)
		info->sub_charger_name = info->pdata->sub_charger_name;
#endif
	pr_info("%s: Charger name: %s\n", __func__, info->charger_name);
	pr_info("%s: Fuelgauge name: %s\n", __func__, info->fuelgauge_name);
#if defined(CONFIG_CHARGER_MAX8922_U1)
	if (system_rev >= 2)
		pr_info("%s: SubCharger name: %s\n", __func__,
			info->sub_charger_name);
#endif

	info->psy_charger = power_supply_get_by_name(info->charger_name);
	info->psy_fuelgauge = power_supply_get_by_name(info->fuelgauge_name);
#if defined(CONFIG_CHARGER_MAX8922_U1)
	if (system_rev >= 2)
		info->psy_sub_charger =
		    power_supply_get_by_name(info->sub_charger_name);
#endif
	if (!info->psy_charger || !info->psy_fuelgauge) {
		pr_err("%s: fail to get power supply\n", __func__);
		goto err_psy_get;
	}

	/* WORKAROUND: set battery pdata in driver */
	if (system_rev == 3) {
		info->pdata->temper_src = TEMPER_EXT_ADC;
		info->pdata->temper_ch = 7;
	}
	pr_info("%s: Temperature source: %s\n", __func__,
		temper_src_name[info->pdata->temper_src]);

	/* recalculate recharge voltage, it depends on max voltage value */
	info->pdata->recharge_voltage = info->pdata->voltage_max -
							RECHG_DROP_VALUE;
	pr_info("%s: Recharge voltage: %d\n", __func__,
				info->pdata->recharge_voltage);
#if defined(CONFIG_S3C_ADC)
#if defined(CONFIG_MACH_S2PLUS)
	if (system_rev >= 2)
#endif
		/* adc register */
		info->adc_client = s3c_adc_register(pdev, NULL, NULL, 0);

	if (IS_ERR(info->adc_client)) {
		pr_err("%s: fail to register adc\n", __func__);
		goto err_adc_reg;
	}
#endif

	/* init battery info */
	info->charge_real_state = battery_get_info(info,
				POWER_SUPPLY_PROP_STATUS);
	if ((info->charge_real_state == POWER_SUPPLY_STATUS_CHARGING) ||
		(info->charge_real_state == POWER_SUPPLY_STATUS_FULL)) {
		pr_info("%s: boot with charging, s(%d)\n", __func__,
						info->charge_real_state);
		info->charge_start_time = 1;
	} else {
		pr_info("%s: boot without charging, s(%d)\n", __func__,
						info->charge_real_state);
		info->charge_start_time = 0;
	}
	info->full_charged_state = false;
	info->abstimer_state = false;
	info->recharge_phase = false;
	info->siop_charge_current = info->pdata->chg_curr_usb;
	info->monitor_mode = MONITOR_NORM;
	info->led_state = BATT_LED_DISCHARGING;
	info->monitor_count = 0;

	/* LPM charging state */
	info->lpm_state = lpcharge;

	mutex_init(&info->mon_lock);
	mutex_init(&info->ops_lock);
	mutex_init(&info->err_lock);

	wake_lock_init(&info->monitor_wake_lock, WAKE_LOCK_SUSPEND,
		       "battery-monitor");
	wake_lock_init(&info->emer_wake_lock, WAKE_LOCK_SUSPEND,
		       "battery-emergency");
	if (!info->pdata->suspend_chging)
		wake_lock_init(&info->charge_wake_lock,
			       WAKE_LOCK_SUSPEND, "battery-charging");
#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	wake_lock_init(&info->update_wake_lock, WAKE_LOCK_SUSPEND,
		       "battery-update");
#endif

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

#if defined(CONFIG_TARGET_LOCALE_KOR)
	info->entry = create_proc_entry("batt_info_proc", S_IRUGO, NULL);
	if (!info->entry)
		pr_err("%s: failed to create proc_entry\n", __func__);
	else {
		info->entry->read_proc = battery_info_proc;
		info->entry->data = (struct battery_info *)info;
	}
#endif

	pr_info("%s: SAMSUNG Battery Driver Loaded\n", __func__);
	return 0;

err_psy_reg_ac:
	power_supply_unregister(&info->psy_usb);
err_psy_reg_usb:
	power_supply_unregister(&info->psy_bat);
err_psy_reg_bat:
	wake_lock_destroy(&info->monitor_wake_lock);
	wake_lock_destroy(&info->emer_wake_lock);
#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	wake_lock_destroy(&info->update_wake_lock);
#endif
	mutex_destroy(&info->mon_lock);
	mutex_destroy(&info->ops_lock);
	mutex_destroy(&info->err_lock);
	if (!info->pdata->suspend_chging)
		wake_lock_destroy(&info->charge_wake_lock);

err_adc_reg:
err_psy_get:
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
#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	wake_lock_destroy(&info->update_wake_lock);
#endif
	if (!info->pdata->suspend_chging)
		wake_lock_destroy(&info->charge_wake_lock);

	mutex_destroy(&info->mon_lock);
	mutex_destroy(&info->ops_lock);
	mutex_destroy(&info->err_lock);

	kfree(info);

	return 0;
}

#ifdef CONFIG_PM
static int samsung_battery_prepare(struct device *dev)
{
	struct battery_info *info = dev_get_drvdata(dev);
	pr_info("%s\n", __func__);

	if ((info->monitor_mode != MONITOR_EMER_LV1) &&
		(info->monitor_mode != MONITOR_EMER_LV2)) {
		if ((info->charge_real_state ==
					POWER_SUPPLY_STATUS_CHARGING) ||
			(info->charge_virt_state ==
					POWER_SUPPLY_STATUS_CHARGING))
			info->monitor_mode = MONITOR_CHNG_SUSP;
		else
			info->monitor_mode = MONITOR_NORM_SUSP;
	}

	battery_monitor_interval(info);

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

	info->is_suspended = true;

	cancel_work_sync(&info->monitor_work);

	return 0;
}

static int samsung_battery_resume(struct device *dev)
{
	struct battery_info *info = dev_get_drvdata(dev);
	pr_info("%s\n", __func__);

	schedule_work(&info->monitor_work);

	info->is_suspended = false;

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

MODULE_AUTHOR("SangYoung Son <hello.son@samsung.com>");
MODULE_DESCRIPTION("SAMSUNG battery driver");
MODULE_LICENSE("GPL");
