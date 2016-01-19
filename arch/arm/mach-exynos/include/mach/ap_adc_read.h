/*
 * arch/arm/mach-exynos/include/mach/ap_adc_read.h
 *
 * header file supporting AP's ADC read driver.
 *
 * Copyright (C) 2010 Samsung Electronics
 * Seung-Jin Hahn <sjin.hahn@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __AP_ADC_READ_H__
#define __AP_ADC_READ_H__

#define ADC_DEV_NAME			"exynos-adc-ic"

struct ap_adc_read_platform_data {
	int adc_check_channel;
	struct s3c_adc_client *adc;
	struct mutex adc_mutex;

	int (*get_adc_value)(void);
};
#endif /* __AP_ADC_READ_H__ */
