/*
 * stmpe811-adc.h
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

#ifndef __STMPE811_ADC_H_
#define __STMPE811_ADC_H_

/*
 * struct sec_bat_adc_table_data - adc to temperature table for sec battery
 * driver
 * @adc_table_chX: adc table for channel X
 * @table_size_chX: size of chX adc table
 */
struct stmpe811_platform_data {
	struct adc_table_data *adc_table_ch0;
	unsigned int table_size_ch0;
	struct adc_table_data *adc_table_ch1;
	unsigned int table_size_ch1;
	struct adc_table_data *adc_table_ch2;
	unsigned int table_size_ch2;
	struct adc_table_data *adc_table_ch3;
	unsigned int table_size_ch3;
	struct adc_table_data *adc_table_ch4;
	unsigned int table_size_ch4;
	struct adc_table_data *adc_table_ch5;
	unsigned int table_size_ch5;
	struct adc_table_data *adc_table_ch6;
	unsigned int table_size_ch6;
	struct adc_table_data *adc_table_ch7;
	unsigned int table_size_ch7;

	int irq_gpio;
};

u16 stmpe811_get_adc_data(u8 channel);
int stmpe811_get_adc_value(u8 channel);

#endif

