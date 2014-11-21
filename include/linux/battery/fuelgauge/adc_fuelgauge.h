/*
 * adc_fuelgauge.h
 * Samsung ADC Fuel Gauge Header
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

#ifndef __ADC_FUELGAUGE_H
#define __ADC_FUELGAUGE_H __FILE__

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */
#define SEC_FUELGAUGE_I2C_SLAVEADDR (0x6D >> 1)

#define MAX17048_VCELL_MSB	0x02
#define MAX17048_VCELL_LSB	0x03

#define ADC_HISTORY_SIZE	30
enum {
	ADC_CHANNEL_VOLTAGE_AVG = 0,
	ADC_CHANNEL_VOLTAGE_OCV,
	ADC_CHANNEL_NUM
};
		
struct adc_sample_data {
	unsigned int cnt;
	int total_adc;
	int average_adc;
	int adc_arr[ADC_HISTORY_SIZE];
	int index;
};

struct battery_data_t {
	unsigned int adc_check_count;
	unsigned int monitor_polling_time;
	unsigned int channel_voltage;
	unsigned int channel_current;
	const sec_bat_adc_table_data_t *ocv2soc_table;
	unsigned int ocv2soc_table_size;
	const sec_bat_adc_table_data_t *adc2vcell_table;
	unsigned int adc2vcell_table_size;
	u8 *type_str;
};

struct sec_fg_info {
	int voltage_now;
	int voltage_avg;
	int voltage_ocv;
	int current_now;
	int current_avg;
	int capacity;
	struct adc_sample_data	adc_sample[ADC_CHANNEL_NUM];

	struct mutex adclock;
	struct delayed_work monitor_work;
};

#endif /* __ADC_FUELGAUGE_H */
