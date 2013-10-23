/*
 * exynos_sound.h - audio platform data for exynos
 *
 *  Copyright (C) 2012 Samsung Electrnoics
 *  Uk Kim <w0806.kim@samsung.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LINUX_EXYNOS_T0_SOUND_H
#define _LINUX_EXYNOS_T0_SOUND_H

#ifdef CONFIG_USE_ADC_DET
struct jack_zone {
	unsigned int adc_high;
	unsigned int delay_ms;
	unsigned int check_count;
	unsigned int jack_type;
};
#endif

struct exynos_sound_platform_data {
	void (*set_lineout_switch) (int on);
	void (*set_ext_main_mic) (int on);
	void (*set_ext_sub_mic) (int on);
	int (*get_ground_det_value) (void);
	int (*get_ground_det_irq_num) (void);
#if defined(CONFIG_SND_DUOS_MODEM_SWITCH)
	void (*set_modem_switch) (int on);
#endif
	int dcs_offset_l;
	int dcs_offset_r;
#ifdef CONFIG_USE_ADC_DET
	struct jack_zone *zones;
	int	num_zones;
	int use_jackdet_type;
#endif
};

#ifdef CONFIG_EXYNOS_SOUND_PLATFORM_DATA

int exynos_sound_set_platform_data(
		const struct exynos_sound_platform_data *data);

#else

static inline
int exynos_sound_set_platform_data(
		const struct exynos_sound_platform_data *data)
{
	return -ENOSYS;
}

#endif

const struct exynos_sound_platform_data *exynos_sound_get_platform_data(void);
#endif
