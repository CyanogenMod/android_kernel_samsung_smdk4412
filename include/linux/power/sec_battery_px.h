/*
 *  sec_battery.h
 *  charger systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2010 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_SEC_BATTERY_H
#define _LINUX_SEC_BATTERY_H

enum charger_type {
	CHARGER_BATTERY = 0,
	CHARGER_USB,
	CHARGER_AC,
	CHARGER_DOCK,
	CHARGER_MISC,
	CHARGER_DISCHARGE
};

struct max8903_charger_data {
	int enable_line;
	int connect_line;
	int fullcharge_line;
	int currentset_line;
	int accessory_line;
};

struct sec_battery_platform_data {
	struct max8903_charger_data charger;

	void (*set_charging_state) (int, int);
	int (*get_charging_state) (void);
	void (*set_charging_current) (int);
	int (*get_charging_current) (void);
	int (*get_charger_is_full)(void);

#if defined(CONFIG_SMB347_CHARGER)
	int (*get_aicl_current)(void);
	int (*get_input_current)(void);
	void (*set_aicl_state)(int);
#endif

	void (*init_charger_gpio) (void);
#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE) || defined(CONFIG_MACH_KONA) || defined(CONFIG_MACH_TAB3)
	void (*inform_charger_connection) (int, int);
#else
	void (*inform_charger_connection) (int);
#endif
	int temp_high_threshold;
	int temp_high_recovery;
	int temp_low_recovery;
	int temp_low_threshold;

#if defined(CONFIG_TARGET_LOCALE_USA)
	int temp_event_threshold;
	int temp_lpm_high_threshold;
	int temp_lpm_high_recovery;
	int temp_lpm_low_recovery;
	int temp_lpm_low_threshold;
#endif /* CONFIG_TARGET_LOCALE_USA */

	int charge_duration;
	int recharge_duration;
	int recharge_voltage;
	int (*check_lp_charging_boot) (void);
	int (*check_jig_status) (void);

};

/* for test driver */
#define __TEST_DEVICE_DRIVER__

enum capacity_type {
	CAPACITY_TYPE_FULL = 0,
	CAPACITY_TYPE_MIX,
	CAPACITY_TYPE_AV,
	CAPACITY_TYPE_REP,
};

enum dock_type {
	DOCK_NONE = 0,
	DOCK_DESK,
	DOCK_KEYBOARD,
};

extern int low_batt_compensation(int fg_soc, int fg_vcell, int fg_current);
extern void reset_low_batt_comp_cnt(void);
extern int get_fuelgauge_value(int data);
extern struct max17042_chip *max17042_chip_data;
extern int get_fuelgauge_capacity(enum capacity_type type);

#if defined(CONFIG_STMPE811_ADC)
u16 stmpe811_get_adc_data(u8 channel);
#endif

#endif
