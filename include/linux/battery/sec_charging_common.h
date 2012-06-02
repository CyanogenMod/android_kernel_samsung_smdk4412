/*
 * sec_charging_common.h
 * Samsung Mobile Charging Common Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
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

#ifndef __SEC_CHARGING_COMMON_H
#define __SEC_CHARGING_COMMON_H __FILE__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/device.h>

/* definitions */
#define	SEC_SIZEOF_POWER_SUPPLY_TYPE	11	/* P10 */

enum sec_battery_voltage_mode {
	/* average voltage */
	SEC_BATTEY_VOLTAGE_AVERAGE = 0,
	/* open circuit voltage */
	SEC_BATTEY_VOLTAGE_OCV,
};

/* ADC type */
#define SEC_BATTERY_ADC_TYPE_NUM	3

enum sec_battery_adc_type {
	/* NOT using this ADC channel */
	SEC_BATTERY_ADC_TYPE_NONE = 0,
	/* ADC in AP */
	SEC_BATTERY_ADC_TYPE_AP,
	/* ADC by additional IC */
	SEC_BATTERY_ADC_TYPE_IC,
};

enum sec_battery_adc_channel {
	SEC_BAT_ADC_CHANNEL_CABLE_CHECK = 0,
	SEC_BAT_ADC_CHANNEL_BAT_CHECK,
	SEC_BAT_ADC_CHANNEL_TEMP,
	SEC_BAT_ADC_CHANNEL_TEMP_AMBIENT,
	SEC_BAT_ADC_CHANNEL_FULL_CHECK,
};

/* charging mode */
enum sec_battery_charging_mode {
	/* no charging */
	SEC_BATTERY_CHARGING_NONE = 0,
	/* normal charging */
	SEC_BATTERY_CHARGING_NORMAL,
	/* charging after 1st full-check*/
	SEC_BATTERY_CHARGING_2ND,
	/* recharging */
	SEC_BATTERY_CHARGING_RECHARGING,
};

struct sec_bat_adc_api {
	bool (*init)(struct platform_device *);
	bool (*exit)(void);
	int (*read)(unsigned int);
};
#define sec_bat_adc_api_t struct sec_bat_adc_api

/* monitor activation */
enum sec_battery_polling_time_type {
	/* same order with power supply status */
	SEC_BATTERY_POLLING_TIME_BASIC = 0,
	SEC_BATTERY_POLLING_TIME_CHARGING,
	SEC_BATTERY_POLLING_TIME_DISCHARGING,
	SEC_BATTERY_POLLING_TIME_NOT_CHARGING,
	SEC_BATTERY_POLLING_TIME_SLEEP,
};

enum sec_battery_monitor_polling {
	/* polling work queue */
	SEC_BATTERY_MONITOR_WORKQUEUE,
	/* alarm polling */
	SEC_BATTERY_MONITOR_ALARM,
	/* timer polling */
	SEC_BATTERY_MONITOR_TIMER,
};
#define sec_battery_monitor_polling_t \
	enum sec_battery_monitor_polling

/* full charged check : POWER_SUPPLY_PROP_STATUS */
enum sec_battery_full_charged {
	/* current check by ADC */
	SEC_BATTERY_FULLCHARGED_ADC,
	/* current check by ADC and dual check (1st, 2nd top-off) */
	SEC_BATTERY_FULLCHARGED_ADC_DUAL,
	/* fuel gauge current check */
	SEC_BATTERY_FULLCHARGED_FG_CURRENT,
	/* fuel gauge current check and dual check (1st, 2nd top-off) */
	SEC_BATTERY_FULLCHARGED_FG_CURRENT_DUAL,
	/* charger GPIO */
	SEC_BATTERY_FULLCHARGED_CHGGPIO,
	/* charger interrupt */
	SEC_BATTERY_FULLCHARGED_CHGINT,
	/* charger power supply property */
	SEC_BATTERY_FULLCHARGED_CHGPSY,
};
#define sec_battery_full_charged_t \
	enum sec_battery_full_charged

/* full check condition type (can be used overlapped) */
#define sec_battery_full_condition_t unsigned int
/* SEC_BATTERY_FULL_CONDITION_NOTIMEFULL
  * no full-charged by absolute-timer
  */
#define SEC_BATTERY_FULL_CONDITION_NOTIMEFULL		1
/* SEC_BATTERY_FULL_CONDITION_SOC
  * use capacity for full-charged check
  */
#define SEC_BATTERY_FULL_CONDITION_SOC		2
/* SEC_BATTERY_FULL_CONDITION_AVGVCELL
  * use average VCELL for full-charged check
  */
#define SEC_BATTERY_FULL_CONDITION_AVGVCELL		4
/* SEC_BATTERY_FULL_CONDITION_OCV
  * use OCV for full-charged check
  */
#define SEC_BATTERY_FULL_CONDITION_OCV		8

/* recharge check condition type (can be used overlapped) */
#define sec_battery_recharge_condition_t unsigned int
/* SEC_BATTERY_RECHARGE_CONDITION_SOC
  * use capacity for recharging check
  */
#define SEC_BATTERY_RECHARGE_CONDITION_SOC		1
/* SEC_BATTERY_RECHARGE_CONDITION_AVGVCELL
  * use average VCELL for recharging check
  */
#define SEC_BATTERY_RECHARGE_CONDITION_AVGVCELL		2
/* SEC_BATTERY_RECHARGE_CONDITION_VCELL
  * use VCELL for recharging check
  */
#define SEC_BATTERY_RECHARGE_CONDITION_VCELL		4

/* battery check : POWER_SUPPLY_PROP_PRESENT */
enum sec_battery_check {
	/* No Check for internal battery */
	SEC_BATTERY_CHECK_NONE,
	/* by ADC */
	SEC_BATTERY_CHECK_ADC,
	/* by callback function (battery certification by 1 wired)*/
	SEC_BATTERY_CHECK_CALLBACK,
	/* by PMIC */
	SEC_BATTERY_CHECK_PMIC,
	/* by fuel gauge */
	SEC_BATTERY_CHECK_FUELGAUGE,
	/* by charger */
	SEC_BATTERY_CHECK_CHARGER,
	/* by interrupt (use check_battery_callback() to check battery) */
	SEC_BATTERY_CHECK_INT,
};
#define sec_battery_check_t \
	enum sec_battery_check

/* OVP, UVLO check : POWER_SUPPLY_PROP_HEALTH */
enum sec_battery_ovp_uvlo {
	/* by callback function */
	SEC_BATTERY_OVP_UVLO_CALLBACK,
	/* by PMIC polling */
	SEC_BATTERY_OVP_UVLO_PMICPOLLING,
	/* by PMIC interrupt */
	SEC_BATTERY_OVP_UVLO_PMICINT,
	/* by charger polling */
	SEC_BATTERY_OVP_UVLO_CHGPOLLING,
	/* by charger interrupt */
	SEC_BATTERY_OVP_UVLO_CHGINT,
};
#define sec_battery_ovp_uvlo_t \
	enum sec_battery_ovp_uvlo

/* thermal source */
enum sec_battery_thermal_source {
	/* by fuel gauge */
	SEC_BATTERY_THERMAL_SOURCE_FG,
	/* by external source */
	SEC_BATTERY_THERMAL_SOURCE_CALLBACK,
	/* by ADC */
	SEC_BATTERY_THERMAL_SOURCE_ADC,
};
#define sec_battery_thermal_source_t \
	enum sec_battery_thermal_source

/* temperature check type */
enum sec_battery_temp_check {
	SEC_BATTERY_TEMP_CHECK_NONE = 0,	/* no temperature check */
	SEC_BATTERY_TEMP_CHECK_ADC,	/* by ADC value */
	SEC_BATTERY_TEMP_CHECK_TEMP,	/* by temperature */
};
#define sec_battery_temp_check_t \
	enum sec_battery_temp_check

/* cable check (can be used overlapped) */
#define sec_battery_cable_check_t unsigned int
/* SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE
  * for USB cable in tablet model,
  * status is stick to discharging,
  * but internal charging logic is working
  */
#define SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE		1
/* SEC_BATTERY_CABLE_CHECK_PSY
  * check cable by power supply set_property
  */
#define SEC_BATTERY_CABLE_CHECK_PSY				2
/* SEC_BATTERY_CABLE_CHECK_INT
  * check cable by interrupt
  */
#define SEC_BATTERY_CABLE_CHECK_INT				4
/* SEC_BATTERY_CABLE_CHECK_POLLING
  * check cable by GPIO polling
  */
#define SEC_BATTERY_CABLE_CHECK_POLLING			8

/* check cable source */
enum sec_battery_cable_source {
	/* already given by external argument */
	SEC_BATTERY_CABLE_SOURCE_EXTERNAL,
	/* by callback (MUIC, USB switch) */
	SEC_BATTERY_CABLE_SOURCE_CALLBACK,
	/* by ADC */
	SEC_BATTERY_CABLE_SOURCE_ADC,
};
#define sec_battery_cable_source_t \
	enum sec_battery_cable_source

/* capacity calculation type (can be used overlapped) */
#define sec_fuelgauge_capacity_type_t unsigned int
/* SEC_FUELGAUGE_CAPACITY_TYPE_RESET
  * use capacity information to reset fuel gauge
  * (only for driver algorithm, can NOT be set by user)
  */
#define SEC_FUELGAUGE_CAPACITY_TYPE_RESET	(-1)
/* SEC_FUELGAUGE_CAPACITY_TYPE_RAW
  * use capacity information from fuel gauge directly
  */
#define SEC_FUELGAUGE_CAPACITY_TYPE_RAW		0
/* SEC_FUELGAUGE_CAPACITY_TYPE_SCALE
  * rescale capacity by scaling, need min and max value for scaling
  */
#define SEC_FUELGAUGE_CAPACITY_TYPE_SCALE	1
/* SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC
  * change capacity value by only -1 or +1
  * no sudden change of capacity
  */
#define SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC	2

/**
 * struct sec_bat_adc_table_data - adc to temperature table for sec battery
 * driver
 * @adc: adc value
 * @temperature: temperature(C) * 10
 */
struct sec_bat_adc_table_data {
	int adc;
	int temperature;
};
#define sec_bat_adc_table_data_t \
	struct sec_bat_adc_table_data

struct sec_bat_adc_region {
	int min;
	int max;
};
#define sec_bat_adc_region_t \
	struct sec_bat_adc_region

struct sec_charging_current {
	int input_current_limit;
	int fast_charging_current;
	int full_check_current_1st;
	int full_check_current_2nd;
};
#define sec_charging_current_t \
	struct sec_charging_current

struct sec_battery_platform_data {
	/* NO NEED TO BE CHANGED */
	/* callback functions */
	void (*initial_check)(void);
	bool (*bat_gpio_init)(void);
	bool (*fg_gpio_init)(void);
	bool (*chg_gpio_init)(void);
	bool (*is_lpm)(void);
	bool (*check_jig_status) (void);
	int (*check_cable_callback)(void);
	void (*cable_switch_check)(void);
	void (*cable_switch_normal)(void);
	bool (*check_cable_result_callback)(int);
	bool (*check_battery_callback)(void);
	bool (*check_battery_result_callback)(void);
	int (*ovp_uvlo_callback)(void);
	bool (*ovp_uvlo_result_callback)(int);
	bool (*fuelalert_process)(bool);
	bool (*get_temperature_callback)(
			enum power_supply_property,
			union power_supply_propval*);

	/* ADC API for each ADC type */
	sec_bat_adc_api_t adc_api[SEC_BATTERY_ADC_TYPE_NUM];
	/* ADC region by power supply type
	 * ADC region should be exclusive
	 */
	sec_bat_adc_region_t *cable_adc_value;
	/* charging current for type (0: not use) */
	sec_charging_current_t *charging_current;
	int *polling_time;
	/* NO NEED TO BE CHANGED */

	char *pmic_name;

	/* battery */
	char *vendor;
	int technology;
	int battery_type;
	void *battery_data;

	int bat_gpio_ta_nconnected;
	/* 1 : active high, 0 : active low */
	int bat_polarity_ta_nconnected;
	int bat_irq;
	unsigned long bat_irq_attr;
	sec_battery_cable_check_t cable_check_type;
	sec_battery_cable_source_t cable_source_type;

	bool use_LED;				/* use charging LED */

	bool event_check;
	/* sustaining event after deactivated (second) */
	unsigned int event_waiting_time;

	/* Monitor setting */
	sec_battery_monitor_polling_t polling_type;
	/* for initial check */
	unsigned int monitor_initial_count;

	/* Battery check */
	sec_battery_check_t battery_check_type;
	/* how many times do we need to check battery */
	unsigned int check_count;
	/* ADC */
	/* battery check ADC maximum value */
	unsigned int check_adc_max;
	/* battery check ADC minimum value */
	unsigned int check_adc_min;

	/* OVP/UVLO check */
	sec_battery_ovp_uvlo_t ovp_uvlo_check_type;

	sec_battery_thermal_source_t thermal_source;
	sec_bat_adc_table_data_t *temp_adc_table;
	unsigned int temp_adc_table_size;
	sec_bat_adc_table_data_t *temp_amb_adc_table;
	unsigned int temp_amb_adc_table_size;

	sec_battery_temp_check_t temp_check_type;
	unsigned int temp_check_count;
	/*
	 * limit can be ADC value or Temperature
	 * depending on temp_check_type
	 * temperature should be temp x 10 (0.1 degree)
	 */
	int temp_high_threshold_event;
	int temp_high_recovery_event;
	int temp_low_threshold_event;
	int temp_low_recovery_event;
	int temp_high_threshold_normal;
	int temp_high_recovery_normal;
	int temp_low_threshold_normal;
	int temp_low_recovery_normal;
	int temp_high_threshold_lpm;
	int temp_high_recovery_lpm;
	int temp_low_threshold_lpm;
	int temp_low_recovery_lpm;

	sec_battery_full_charged_t full_check_type;
	unsigned int full_check_count;
	/* ADC for single termination */
	unsigned int full_check_adc_1st;
	/* ADC for dual termination */
	unsigned int full_check_adc_2nd;
	int chg_gpio_full_check;
	/* 1 : active high, 0 : active low */
	int chg_polarity_full_check;
	sec_battery_full_condition_t full_condition_type;
	unsigned int full_condition_soc;
	unsigned int full_condition_avgvcell;
	unsigned int full_condition_ocv;

	sec_battery_recharge_condition_t recharge_condition_type;
	unsigned int recharge_condition_soc;
	unsigned int recharge_condition_avgvcell;
	unsigned int recharge_condition_vcell;

	/* for absolute timer (second) */
	unsigned long charging_total_time;
	/* for recharging timer (second) */
	unsigned long recharging_total_time;
	/* reset charging for abnormal malfunction (0: not use) */
	unsigned long charging_reset_time;

	/* fuel gauge */
	char *fuelgauge_name;
	int fg_irq;
	unsigned long fg_irq_attr;
	/* fuel alert SOC (-1: not use) */
	int fuel_alert_soc;
	/* fuel alert can be repeated */
	bool repeated_fuelalert;
	sec_fuelgauge_capacity_type_t capacity_calculation_type;
	/* soc should be soc x 10 (0.1% degree)
	 * only for scaling
	 */
	unsigned int capacity_max;
	unsigned int capacity_min;

	/* charger */
	char *charger_name;
	int chg_gpio_en;
	/* 1 : active high, 0 : active low */
	int chg_polarity_en;
	int chg_gpio_curr_adj;
	/* 1 : active high, 0 : active low */
	int chg_polarity_curr_adj;
	int chg_gpio_status;
	/* 1 : active high, 0 : active low */
	int chg_polarity_status;
	int chg_irq;
	unsigned long chg_irq_attr;
	/* float voltage (mV) */
	int chg_float_voltage;

	/* ADC setting */
	unsigned int adc_check_count;
	/* ADC type for each channel */
	unsigned int adc_type[];
};
#define sec_battery_platform_data_t \
	struct sec_battery_platform_data

static inline struct power_supply *get_power_supply_by_name(char *name)
{
	if (!name)
		return (struct power_supply *)NULL;
	else
		return power_supply_get_by_name(name);
}

#define psy_do_property(name, function, property, value) \
{	\
	struct power_supply *psy;	\
	int ret;	\
	psy = get_power_supply_by_name((name));	\
	if (!psy) {	\
		pr_err("%s: Fail to "#function" psy (%s)\n",	\
			__func__, (name));	\
		value.intval = 0;	\
	} else {	\
		ret = psy->function##_property(psy, (property), &(value)); \
		if (ret < 0) {	\
			pr_err("%s: Fail to "#name" "#function" (%d=>%d)\n", \
				__func__, (property), ret);	\
			value.intval = 0;	\
		}	\
	}	\
}

#define adc_init(pdev, pdata, channel)	\
	((pdata)->adc_api)[((pdata)->adc_type[(channel)])].init((pdev))

#define adc_exit(pdata, channel)	\
	((pdata)->adc_api)[((pdata)->adc_type[(channel)])].exit()

#define adc_read(pdata, channel)	\
	((pdata)->adc_api)[((pdata)->adc_type[(channel)])].read((channel))

#define get_battery_data(driver)	\
	(((struct battery_data_t *)(driver)->pdata->battery_data)	\
	[(driver)->pdata->battery_type])

#endif /* __SEC_CHARGING_COMMON_H */
