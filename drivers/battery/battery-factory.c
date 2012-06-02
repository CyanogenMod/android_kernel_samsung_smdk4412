/*
 * battery-factory.c - factory mode for battery driver
 *
 * Copyright (C) 2011 Samsung Electronics
 * SangYoung Son <hello.son@samsung.com>
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

#include "battery-factory.h"

static ssize_t battery_show_property(struct device *dev,
				     struct device_attribute *attr, char *buf);

static ssize_t battery_store_property(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count);

#define BATTERY_ATTR(_name)			\
{						\
	.attr = { .name = #_name,		\
		  .mode = S_IRUGO | S_IWUSR | S_IWGRP,	\
		},					\
	.show = battery_show_property,		\
	.store = battery_store_property,		\
}

static struct device_attribute battery_attrs[] = {
	BATTERY_ATTR(batt_reset_soc),
	BATTERY_ATTR(batt_read_raw_soc),
	BATTERY_ATTR(batt_read_adj_soc),
	BATTERY_ATTR(batt_type),
	BATTERY_ATTR(batt_temp_adc),
	BATTERY_ATTR(batt_temp_aver),
	BATTERY_ATTR(batt_temp_adc_aver),
	BATTERY_ATTR(batt_vol_aver),
	BATTERY_ATTR(batt_vfocv),
	BATTERY_ATTR(batt_lp_charging),
	BATTERY_ATTR(batt_charging_source),
	BATTERY_ATTR(test_mode),
	BATTERY_ATTR(batt_error_test),
	BATTERY_ATTR(siop_activated),
	BATTERY_ATTR(wc_status),
	BATTERY_ATTR(wpc_pin_state),

	/* not use */
	BATTERY_ATTR(batt_vol_adc),
	BATTERY_ATTR(batt_vol_adc_cal),
	BATTERY_ATTR(batt_vol_adc_aver),
	BATTERY_ATTR(batt_temp_adc_cal),
	BATTERY_ATTR(batt_vf_adc),
	BATTERY_ATTR(auth_battery),

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	BATTERY_ATTR(batt_temp_adc_spec),
	BATTERY_ATTR(batt_sysrev),
#endif
};

enum {
	BATT_RESET_SOC = 0,
	BATT_READ_RAW_SOC,
	BATT_READ_ADJ_SOC,
	BATT_TYPE,
	BATT_TEMP_ADC,
	BATT_TEMP_AVER,
	BATT_TEMP_ADC_AVER,
	BATT_VOL_AVER,
	BATT_VFOCV,
	BATT_LP_CHARGING,
	BATT_CHARGING_SOURCE,
	TEST_MODE,
	BATT_ERROR_TEST,
	SIOP_ACTIVATED,
	WC_STATUS,
	WPC_PIN_STATE,

	/* not use */
	BATT_VOL_ADC,
	BATT_VOL_ADC_CAL,
	BATT_VOL_ADC_AVER,
	BATT_TEMP_ADC_CAL,
	BATT_VF_ADC,
	AUTH_BATTERY,

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	BATT_TEMP_ADC_SPEC,
	BATT_SYSREV,
#endif
};

static ssize_t battery_show_property(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct battery_info *info = dev_get_drvdata(dev->parent);
	int i;
	int cnt, dat, d_max, d_min, d_total;
	int val;
	const ptrdiff_t off = attr - battery_attrs;
	pr_debug("%s: %s\n", __func__, battery_attrs[off].attr.name);

	i = 0;
	val = 0;
	switch (off) {
	case BATT_READ_RAW_SOC:
		battery_update_info(info);
		val = info->battery_raw_soc;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_READ_ADJ_SOC:
		battery_get_info(info, POWER_SUPPLY_PROP_CAPACITY);
		val = info->battery_soc;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_TYPE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "SDI_SDI\n");
		break;
	case BATT_TEMP_ADC:
		battery_get_info(info, POWER_SUPPLY_PROP_TEMP);
		val = info->battery_temper_adc;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_TEMP_AVER:
		val = 0;
		for (cnt = 0; cnt < CNT_TEMPER_AVG; cnt++) {
			msleep(100);
			battery_get_info(info, POWER_SUPPLY_PROP_TEMP);
			val += info->battery_temper_adc;
			info->battery_temper_adc_avg = val / (cnt + 1);
		}
#ifdef CONFIG_S3C_ADC
		info->battery_temper_avg = info->pdata->covert_adc(
						info->battery_temper_adc_avg,
						info->pdata->temper_ch);
#else
		info->battery_temper_avg = info->battery_temper;
#endif
		val = info->battery_temper_avg;
		pr_info("%s: temper avg(%d)\n", __func__, val);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_TEMP_ADC_AVER:
		val = info->battery_temper_adc_avg;
		pr_info("%s: temper adc avg(%d)\n", __func__, val);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_VOL_AVER:	/* not use POWER_SUPPLY_PROP_VOLTAGE_AVG */
		val = dat = d_max = d_min = d_total = 0;
		for (cnt = 0; cnt < CNT_VOLTAGE_AVG; cnt++) {
			msleep(200);
			dat = battery_get_info(info,
				POWER_SUPPLY_PROP_VOLTAGE_NOW);

			if (cnt != 0) {
				d_max = max(dat, d_max);
				d_min = min(dat, d_min);
			} else
				d_max = d_min = dat;

			d_total += dat;
		}
		val = (d_total - d_max - d_min) / (CNT_VOLTAGE_AVG - 2);
		pr_info("%s: voltage avg(%d)\n", __func__, val);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_VFOCV:
		battery_update_info(info);
		val = info->battery_vfocv;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_LP_CHARGING:
		val = info->lpm_state;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_CHARGING_SOURCE:
		battery_get_info(info, POWER_SUPPLY_PROP_ONLINE);
		val = info->cable_type;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case TEST_MODE:
		val = info->battery_test_mode;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_ERROR_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			"(%d): 0: normal, 1: full charged, 2: freezed, 3: overheated, 4: ovp, 5: vf\n",
			info->battery_error_test);
		break;
	case SIOP_ACTIVATED:
		val = info->siop_state;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case WC_STATUS:
	case WPC_PIN_STATE:
#ifdef CONFIG_BATTERY_WPC_CHARGER
		val = !gpio_get_value(GPIO_WPC_INT);
#else
		val = false;
#endif
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_VOL_ADC:
	case BATT_VOL_ADC_CAL:
	case BATT_VOL_ADC_AVER:
	case BATT_TEMP_ADC_CAL:
	case BATT_VF_ADC:
	case AUTH_BATTERY:
		i += scnprintf(buf + i, PAGE_SIZE - i, "N/A\n");
		break;
#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_MACH_M0_CTC)
	case BATT_SYSREV:
		val = system_rev;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_TEMP_ADC_SPEC:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			"(HIGH: %d / %d,   LOW: %d / %d)\n",
			info->pdata->overheat_stop_temp,
			info->pdata->overheat_recovery_temp,
			info->pdata->freeze_stop_temp,
			info->pdata->freeze_recovery_temp);
		break;
#endif
	default:
		i = -EINVAL;
	}

	return i;
}

static ssize_t battery_store_property(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct battery_info *info = dev_get_drvdata(dev->parent);
	int x;
	int ret;
	const ptrdiff_t off = attr - battery_attrs;
	pr_info("%s: %s\n", __func__, battery_attrs[off].attr.name);

	x = 0;
	ret = 0;
	switch (off) {
	case BATT_RESET_SOC:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x == 1) {
				pr_info("%s: Reset SOC.\n", __func__);
				battery_control_info(info,
						POWER_SUPPLY_PROP_CAPACITY,
						1);
			} else
				pr_info("%s: Not supported param.\n", __func__);
			ret = count;
		}
		break;
	case TEST_MODE:
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->battery_test_mode = x;
			pr_info("%s: battery test mode: %d\n", __func__,
						info->battery_test_mode);
			ret = count;
		}
		break;
	case BATT_ERROR_TEST:
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->battery_error_test = x;
			pr_info("%s: battery error test: %d\n", __func__,
						info->battery_error_test);
			ret = count;
		}
		break;
	case SIOP_ACTIVATED:
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->siop_state = x;

			if (info->siop_state == SIOP_ACTIVE)
				info->siop_charge_current =
					info->pdata->chg_curr_usb;

			pr_info("%s: SIOP %s\n", __func__,
				(info->siop_state ?
				"activated" : "deactivated"));
			ret = count;
		}
		break;
	default:
		ret = -EINVAL;
	}

	schedule_work(&info->monitor_work);

	return ret;
}

void battery_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(battery_attrs); i++) {
		rc = device_create_file(dev, &battery_attrs[i]);
		pr_debug("%s: battery attr.: %s\n", __func__,
					battery_attrs[i].attr.name);
		if (rc)
			goto create_attrs_failed;
	}
	goto succeed;

create_attrs_failed:
	while (i--)
		device_remove_file(dev, &battery_attrs[i]);
succeed:
	return;
}

#if defined(CONFIG_TARGET_LOCALE_KOR)
int battery_info_proc(char *buf, char **start,
			off_t offset, int count, int *eof, void *data)
{
	struct battery_info *info = data;
	struct timespec cur_time;
	ktime_t ktime;
	int len = 0;
	/* Guess we need no more than 100 bytes. */
	int size = 100;

	ktime = alarm_get_elapsed_realtime();
	cur_time = ktime_to_timespec(ktime);

	len = snprintf(buf, size,
		"%lu\t%u\t%u\t%u\t%u\t%d\t%u\t%d\t%d\t%u\t"
		"%u\t%u\t%u\t%u\t%u\t%u\t%d\t%u\t%u\n",
		cur_time.tv_sec,
		info->battery_raw_soc,
		info->battery_soc,
		info->battery_vcell / 1000,
		info->battery_vfocv / 1000,
		info->battery_full_soc,
		info->battery_present,
		info->battery_temper,
		info->battery_temper_adc,
		info->battery_health,
		info->charge_real_state,
		info->charge_virt_state,
		info->cable_type,
		info->charge_current,
		info->full_charged_state,
		info->recharge_phase,
		info->abstimer_state,
		info->monitor_interval,
		info->charge_start_time);
	return len;
}
#elif defined(CONFIG_MACH_M0_CTC)
int battery_info_proc(char *buf, char **start,
			off_t offset, int count, int *eof, void *data)
{
	struct battery_info *info = data;
	struct timespec cur_time;
	ktime_t ktime;
	int len = 0;
	/* Guess we need no more than 100 bytes. */
	int size = 100;

	ktime = alarm_get_elapsed_realtime();
	cur_time = ktime_to_timespec(ktime);

	len = snprintf(buf, size,
		"%lu\t%u\t%u\t%u\t%u\t%u\t%d\t%d\t%u\t"
		"%u\t%u\t%u\t%u\t%u\t%u\t%d\t%u\t%u\n",
		cur_time.tv_sec,
		info->battery_raw_soc,
		info->battery_soc,
		info->battery_vcell / 1000,
		info->battery_vfocv / 1000,
		info->battery_present,
		info->battery_temper,
		info->battery_temper_adc,
		info->battery_health,
		info->charge_real_state,
		info->charge_virt_state,
		info->cable_type,
		info->charge_current,
		info->full_charged_state,
		info->recharge_phase,
		info->abstimer_state,
		info->monitor_interval,
		info->charge_start_time);
	return len;
}
#endif
