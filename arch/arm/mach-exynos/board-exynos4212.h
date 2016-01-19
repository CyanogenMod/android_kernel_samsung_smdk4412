/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
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

#ifndef __MACH_EXYNOS_BOARD_EXYNOS4212_H
#define __MACH_EXYNOS_BOARD_EXYNOS4212_H

#ifdef CONFIG_CHARGER_MAX77693_BAT
#include <linux/battery/sec_charging_common.h>
#endif

#if defined(CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE)
void exynos4_exynos4212_mfd_init(void);
#endif /* CONFIG_MFD_MUIC_ADD_PLATFORM_DEVICE */

#if defined(CONFIG_MUIC_ADD_PLATFORM_DEVICE)
void exynos4_exynos4212_muic_init(void);
#endif /* CONFIG_MUIC_ADD_PLATFORM_DEVICE */

#if defined(CONFIG_MUIC_ADC_ADD_PLATFORM_DEVICE)
void exynos4_exynos4212_adc_init(void);
#endif /* CONFIG_MUIC_ADC_ADD_PLATFORM_DEVICE */

#ifdef CONFIG_CHARGER_MAX77693_BAT
extern sec_battery_platform_data_t sec_battery_pdata;
extern unsigned int lpcharge;
extern int current_cable_type;
#endif

#endif
