/* drivers/gpu/mali400/mali/platform/pegasus-m400/exynos4_pmm.h
 *
 * Copyright 2011 by S.LSI. Samsung Electronics Inc.
 * San#24, Nongseo-Dong, Giheung-Gu, Yongin, Korea
 *
 * Samsung SoC Mali400 DVFS driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software FoundatIon.
 */

/**
 * @file exynos4_pmm.h
 * Platform specific Mali driver functions for the exynos 4XXX based platforms
 */

#ifndef __EXYNOS4_PMM_H__
#define __EXYNOS4_PMM_H__

#include "mali_osk.h"
#include <linux/mali/mali_utgard.h>
#include <linux/platform_device.h>
/* @Enable or Disable Mali GPU Bottom Lock feature */
#define MALI_GPU_BOTTOM_LOCK 1
#define MALI_VOLTAGE_LOCK 1

#ifdef __cplusplus
extern "C" {
#endif

/** @brief description of power change reasons
 */
typedef enum mali_power_mode_tag
{
	MALI_POWER_MODE_ON,
	MALI_POWER_MODE_LIGHT_SLEEP,
	MALI_POWER_MODE_DEEP_SLEEP,
} mali_power_mode;

/** @brief Platform specific setup and initialisation of MALI
 *
 * This is called from the entrypoint of the driver to initialize the platform
 *
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_platform_init(struct device *dev);

/** @brief Platform specific deinitialisation of MALI
 *
 * This is called on the exit of the driver to terminate the platform
 *
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_platform_deinit(struct device *dev);

/** @brief Platform specific powerdown sequence of MALI
 *
 * Call as part of platform init if there is no PMM support, else the
 * PMM will call it.
 * There are three power modes defined:
 *  1) MALI_POWER_MODE_ON
 *  2) MALI_POWER_MODE_LIGHT_SLEEP
 *  3) MALI_POWER_MODE_DEEP_SLEEP
 * MALI power management module transitions to MALI_POWER_MODE_LIGHT_SLEEP mode when MALI is idle
 * for idle timer (software timer defined in mali_pmm_policy_jobcontrol.h) duration, MALI transitions
 * to MALI_POWER_MODE_LIGHT_SLEEP mode during timeout if there are no more jobs queued.
 * MALI power management module transitions to MALI_POWER_MODE_DEEP_SLEEP mode when OS does system power
 * off.
 * Customer has to add power down code when MALI transitions to MALI_POWER_MODE_LIGHT_SLEEP or MALI_POWER_MODE_DEEP_SLEEP
 * mode.
 * MALI_POWER_MODE_ON mode is entered when the MALI is to powered up. Some customers want to control voltage regulators during
 * the whole system powers on/off. Customer can track in this function whether the MALI is powered up from
 * MALI_POWER_MODE_LIGHT_SLEEP or MALI_POWER_MODE_DEEP_SLEEP mode and manage the voltage regulators as well.
 * @param power_mode defines the power modes
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_platform_power_mode_change(struct device *dev, mali_power_mode power_mode);


/** @brief Platform specific handling of GPU utilization data
 *
 * When GPU utilization data is enabled, this function will be
 * periodically called.
 *
 * @param utilization The workload utilization of the Mali GPU. 0 = no utilization, 256 = full utilization.
 */
void mali_gpu_utilization_handler(struct mali_gpu_utilization_data *data);

_mali_osk_errcode_t g3d_power_domain_control(int bpower_on);

#ifdef CONFIG_REGULATOR
void mali_regulator_disable(void);
void mali_regulator_enable(void);
void mali_regulator_set_voltage(int min_uV, int max_uV);
#endif

#ifdef CONFIG_MALI_DVFS
ssize_t show_time_in_state(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t set_time_in_state(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
#ifdef CONFIG_CPU_EXYNOS4210
#if MALI_GPU_BOTTOM_LOCK
int mali_dvfs_bottom_lock_push(void);
int mali_dvfs_bottom_lock_pop(void);
#endif
#else
int mali_dvfs_bottom_lock_push(int lock_step);
int mali_dvfs_bottom_lock_pop(void);
#endif
#endif

#if MALI_VOLTAGE_LOCK
int mali_voltage_lock_push(int lock_vol);
int mali_voltage_lock_pop(void);
int mali_voltage_lock_init(void);
int mali_vol_get_from_table(int vol);
#endif
#ifdef __cplusplus
}
#endif
#endif
