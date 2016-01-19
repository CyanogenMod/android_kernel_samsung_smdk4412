/* arch/arm/mach-exynos/tab3-jack.c
 *
 * Copyright (C) 2012 Samsung Electronics Co, Ltd
 *
 * Based on mach-exynos/mach-p4notepq.c
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <mach/gpio-midas.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/sec_jack.h>

#define GPIO_EAR_DET_35			EXYNOS4_GPX0(4)

static void sec_set_jack_micbias(bool on)
{
	gpio_set_value(GPIO_MIC_BIAS_EN, on);
}

static struct sec_jack_zone sec_jack_zones[] = {
	{
		/* adc == 0, unstable zone, default to 3pole if it stays
		 * in this range for 100ms (10ms delays, 10 samples)
		 */
		.adc_high = 0,
		.delay_ms = 10,
		.check_count = 10,
		.jack_type = SEC_HEADSET_3POLE,
	},
	{
		/* 0 < adc <= 1420, unstable zone, default to 3pole if it stays
		 * in this range for 100ms (10ms delays, 10 samples)
		 */
		.adc_high = 1420,
		.delay_ms = 10,
		.check_count = 10,
		.jack_type = SEC_HEADSET_3POLE,
	},
	{
		/* 1420 < adc <= 2000, unstable zone, default to 4pole if it
		 * stays in this range for 100ms (10ms delays, 10 samples)
		 */
		.adc_high = 2000,
		.delay_ms = 10,
		.check_count = 10,
		.jack_type = SEC_HEADSET_4POLE,
	},
	{
		/* 2000 < adc <= 4000, 4 pole zone, default to 4pole if it
		 * stays in this range for 50ms (10ms delays, 5 samples)
		 */
		.adc_high = 4000,
		.delay_ms = 10,
		.check_count = 5,
		.jack_type = SEC_HEADSET_4POLE,
	},
	{
		/* adc > 4000, unstable zone, default to 3pole if it stays
		 * in this range for two seconds (10ms delays, 200 samples)
		 */
		.adc_high = 0x7fffffff,
		.delay_ms = 10,
		.check_count = 200,
		.jack_type = SEC_HEADSET_3POLE,
	},
};

/* To support 3-buttons earjack */
static struct sec_jack_buttons_zone sec_jack_buttons_zones[] = {
	{
		/* 0 <= adc <= 260, stable zone */
		.code = KEY_MEDIA,
		.adc_low = 0,
		.adc_high = 260,
	},
	{
		/* 261 <= adc <= 530, stable zone */
		.code = KEY_VOLUMEUP,
		.adc_low = 261,
		.adc_high = 530,
	},
	{
		/* 531 <= adc <= 1800, stable zone */
		.code = KEY_VOLUMEDOWN,
		.adc_low = 531,
		.adc_high = 1800,
	},
};

static struct sec_jack_platform_data sec_jack_data = {
	.set_micbias_state = sec_set_jack_micbias,
	.zones = sec_jack_zones,
	.num_zones = ARRAY_SIZE(sec_jack_zones),
	.buttons_zones = sec_jack_buttons_zones,
	.num_buttons_zones = ARRAY_SIZE(sec_jack_buttons_zones),
	.det_gpio = GPIO_DET_35,
	.send_end_gpio = GPIO_EAR_SEND_END,
	.send_end_active_high	= false,
};

static struct platform_device sec_device_jack = {
	.name = "sec_jack",
	.id = 1,		/* will be used also for gpio_event id */
	.dev.platform_data = &sec_jack_data,
};
void __init tab3_jack_init(void)
{
	/* Ear Microphone BIAS */
	int err;
	err = gpio_request(GPIO_MIC_BIAS_EN, "EAR MIC");
	if (err) {
		pr_err(KERN_ERR "GPIO_EAR_MIC_BIAS_EN GPIO set error!\n");
		return;
	}
	gpio_direction_output(GPIO_MIC_BIAS_EN, 1);
	gpio_set_value(GPIO_MIC_BIAS_EN, 0);
	gpio_free(GPIO_MIC_BIAS_EN);
	platform_device_register(&sec_device_jack);

#if defined(CONFIG_TARGET_TAB3_3G8) || defined(CONFIG_TARGET_TAB3_WIFI8) \
	|| defined(CONFIG_TARGET_TAB3_LTE8)
	sec_jack_data.det_gpio = GPIO_EAR_DET_35;
	if (system_rev <= 1)
		sec_jack_data.send_end_active_high = true;
#else
	/* Santos7 HW Version Check
	 * 0000 : GT-P3200 REV0.0
	 * 0001 : GT-P3200 REV0.1
	 * 0010 : GT-P3200 REV0.1 CURRENT
	 * 0011 : GT-P3200 REV0.2
	 * 0100 : GT-P3200 REV0.3
	 */
	if(system_rev <= 3)
		sec_jack_data.det_gpio = GPIO_DET_35;
	else
		sec_jack_data.det_gpio  = GPIO_EAR_DET_35;

	if(system_rev == 4)
		sec_jack_data.send_end_active_high = true;

#endif
}
