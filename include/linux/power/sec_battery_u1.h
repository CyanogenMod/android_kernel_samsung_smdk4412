/*
 * Copyright (C) 2010 Samsung Electronics, Inc.
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

#ifndef __MACH_SEC_BATTERY_H
#define __MACH_SEC_BATTERY_H __FILE__

#if defined(CONFIG_TARGET_LOCALE_KOR)
#if defined(CONFIG_MACH_U1_KOR_LGT)
#define HWREV_FOR_BATTERY	0x03
#else
#define HWREV_FOR_BATTERY	0x06
#endif
#elif defined(CONFIG_TARGET_LOCALE_NTT)
#define HWREV_FOR_BATTERY	0x0C
#elif defined(CONFIG_MACH_U1_NA_SPR_EPIC2_REV00)
#define HWREV_FOR_BATTERY	0x06
#elif defined(CONFIG_MACH_Q1_BD)
#define HWREV_FOR_BATTERY	0x02
#elif defined(CONFIG_MACH_TRATS)
#define HWREV_FOR_BATTERY	0x02
#else	/*U1 EUR OPEN */
#define HWREV_FOR_BATTERY	0x08
#endif

/*soc level for 3.6V */
#define SEC_BATTERY_SOC_3_6	7

/* #define SEC_BATTERY_TOPOFF_BY_CHARGER */
#define SEC_BATTERY_INDEPEDENT_VF_CHECK
#if defined(CONFIG_MACH_Q1_BD)
#define SEC_BATTERY_1ST_2ND_TOPOFF
#endif

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

/**
 * struct sec_bat_plaform_data - init data for sec batter driver
 * @fuel_gauge_name: power supply name of fuel gauge
 * @charger_name: power supply name of charger
 * @sub_charger_name: power supply name of sub-charger
 * @adc_table: array of adc to temperature data
 * @adc_arr_size: size of adc_table
 * @irq_topoff: IRQ number for top-off interrupt
 * @irq_lowbatt: IRQ number for low battery alert interrupt
 */
struct sec_bat_platform_data {
	char *fuel_gauge_name;
	char *charger_name;
	char *sub_charger_name;

	unsigned int adc_arr_size;
	struct sec_bat_adc_table_data *adc_table;
	unsigned int adc_channel;
	unsigned int adc_sub_arr_size;
	struct sec_bat_adc_table_data *adc_sub_table;
	unsigned int adc_sub_channel;
	unsigned int (*get_lpcharging_state) (void);
	void (*no_bat_cb) (void);
	void (*initial_check) (void);
#if defined(CONFIG_TARGET_LOCALE_NAATT) || \
	defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
	int adc_vf_channel;
#endif
};

#endif /* __MACH_SEC_BATTERY_H */
