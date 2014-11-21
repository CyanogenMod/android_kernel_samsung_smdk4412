/*
 * arch/arm/mach-exynos/board-exynos4412-adc.c
 *
 * c source file supporting MUIC board specific register
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
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

#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
//#include <mach/irqs.h>
#include <linux/interrupt.h>

#include <plat/gpio-cfg.h>
#include <plat/adc.h>

#include <mach/ap_adc_read.h>

#include "board-exynos4212.h"

extern struct ap_adc_read_platform_data exynos4_adc_pdata;

static int ap_adc_get_resistance_id_adc_value(struct s3c_adc_client *adc,
						unsigned int ch)
{
	int adc_val;
	int i;
	int ret = -1;

	for (i = 0; (ret < 0) && (i < 10); i++) {
		ret = s3c_adc_read(adc, ch);
		if (ret < 0) {
			pr_err("%s:%s err-%d(%d)\n", ADC_DEV_NAME, __func__, i,
					ret);
			msleep(5);
		}
	}

	if (ret < 0) {
		pr_err("%s:%s adc read retry fail!\n", ADC_DEV_NAME, __func__);
		return ret;
	}

	adc_val = ret;

	pr_info("%s:%s adc_val = %d\n", ADC_DEV_NAME, __func__, adc_val);

	return adc_val;
}

static int ap_adc_get_adc_value(void)
{
	struct ap_adc_read_platform_data *pdata = &exynos4_adc_pdata;
	struct s3c_adc_client *adc;

	u32 adc_buff[5] = {0};
	u32 adc_sum = 0;
	u32 adc_val = 0;
	u32 adc_min = -1;
	u32 adc_max = -1;
	int valid_adc_count = 0;
	int i;

	if (!pdata) {
		pr_err("ap_adc_read_platform_data is not registered!\n");
		return -ENODEV;
	}

	mutex_lock(&pdata->adc_mutex);
	adc = pdata->adc;
	if (!adc) {
		pr_err("adc client is not registered!\n");
		return -ENODEV;
	}

	for (i = 0; i < 5; i++) {
		adc_val = ap_adc_get_resistance_id_adc_value(adc,
				pdata->adc_check_channel);
		pr_info("%s:%s adc_val[%d] value = %d", ADC_DEV_NAME, __func__,
				i, adc_val);

		adc_buff[i] = adc_val;
		if (adc_buff[i] >= 0) {
			adc_sum += adc_buff[i];
			valid_adc_count++;

			if (adc_min == -1)
				adc_min = adc_buff[i];

			if (adc_max == -1)
				adc_max = adc_buff[i];

			adc_min = min(adc_min, adc_buff[i]);
			adc_max = max(adc_max, adc_buff[i]);
		}
		msleep(20);
	}
	mutex_unlock(&pdata->adc_mutex);

	adc_val = (adc_sum - adc_max - adc_min)/(valid_adc_count - 2);

	pr_info("%s:%s decided ADC val = %d", ADC_DEV_NAME, __func__, adc_val);
	return (int)adc_val;
}

struct ap_adc_read_platform_data exynos4_adc_pdata = {
#if defined(CONFIG_MUIC_SUPPORT_11PIN_DEV_GENDE)
	.adc_check_channel	= 0,
#endif /* CONFIG_MUIC_SUPPORT_11PIN_DEV_GENDE */

	.get_adc_value		= ap_adc_get_adc_value,
};

struct platform_device sec_adc_reader = {
	.name = ADC_DEV_NAME,
	.id = -1,
	.dev.platform_data = &exynos4_adc_pdata,
};

void __init exynos4_exynos4212_adc_init(void)
{
	struct platform_device *pdevice = &sec_adc_reader;
	struct ap_adc_read_platform_data *pdata = pdevice->dev.platform_data;

	pr_info("%s:%s\n", ADC_DEV_NAME, __func__);

	platform_device_register(pdevice);

	/* Register adc client */
	pdata->adc = s3c_adc_register(pdevice, NULL, NULL, 0);

	/* adc mutex init */
	mutex_init(&pdata->adc_mutex);
}
