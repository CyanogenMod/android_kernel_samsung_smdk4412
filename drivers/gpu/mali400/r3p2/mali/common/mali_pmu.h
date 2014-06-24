/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.h
 * Platform specific Mali driver functions
 */

#ifndef __MALI_PMU_H__
#define __MALI_PMU_H__

#include "mali_osk.h"

#define MALI_PMU_M450_DOM1      0
#define MALI_PMU_M450_DOM1_MASK (1 << 1)
#define MALI_PMU_M450_DOM2      1
#define MALI_PMU_M450_DOM2_MASK (1 << 2)
#define MALI_PMU_M450_DOM3      2
#define MALI_PMU_M450_DOM3_MASK (1 << 3)

#define MALI_PMU_M400_PP0      0
#define MALI_PMU_M400_PP0_MASK (1 << 2)
#define MALI_PMU_M400_PP1      1
#define MALI_PMU_M400_PP1_MASK (1 << 3)
#define MALI_PMU_M400_PP2      2
#define MALI_PMU_M400_PP2_MASK (1 << 4)
#define MALI_PMU_M400_PP3      3
#define MALI_PMU_M400_PP3_MASK (1 << 5)

struct mali_pmu_core;

/** @brief Initialisation of MALI PMU
 * 
 * This is called from entry point of the driver in order to create and intialize the PMU resource
 *
 * @param resource it will be a pointer to a PMU resource
 * @param number_of_pp_cores Number of found PP resources in configuration
 * @param number_of_l2_caches Number of found L2 cache resources in configuration
 * @return The created PMU object, or NULL in case of failure.
 */
struct mali_pmu_core *mali_pmu_create(_mali_osk_resource_t *resource, u32 number_of_pp_cores, u32 number_of_l2_caches);

/** @brief It deallocates the PMU resource
 * 
 * This is called on the exit of the driver to terminate the PMU resource
 *
 * @param pmu Pointer to PMU core object to delete
 */
void mali_pmu_delete(struct mali_pmu_core *pmu);

/** @brief Reset PMU core
 *
 * @param pmu Pointer to PMU core object to reset
 * @return _MALI_OSK_ERR_OK on success, otherwise failure.
 */
_mali_osk_errcode_t mali_pmu_reset(struct mali_pmu_core *pmu);

/** @brief MALI GPU power down using MALI in-built PMU
 *
 * Called to power down the specified cores. The mask will be saved so that \a
 * mali_pmu_power_up_all will bring the PMU back to the previous state set with
 * this function or \a mali_pmu_power_up.
 *
 * @param pmu Pointer to PMU core object to power down
 * @param mask Mask specifying which power domains to power down
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_pmu_power_down(struct mali_pmu_core *pmu, u32 mask);

/** @brief MALI GPU power up using MALI in-built PMU
 *
 * Called to power up the specified cores. The mask will be saved so that \a
 * mali_pmu_power_up_all will bring the PMU back to the previous state set with
 * this function or \a mali_pmu_power_down.
 *
 * @param pmu Pointer to PMU core object to power up
 * @param mask Mask specifying which power domains to power up
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_pmu_power_up(struct mali_pmu_core *pmu, u32 mask);

/** @brief MALI GPU power down using MALI in-built PMU
 *
 * called to power down all cores
 *
 * @param pmu Pointer to PMU core object to power down
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_pmu_power_down_all(struct mali_pmu_core *pmu);

/** @brief MALI GPU power up using MALI in-built PMU
 *
 * called to power up all cores
 *
 * @param pmu Pointer to PMU core object to power up
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_pmu_power_up_all(struct mali_pmu_core *pmu);

/** @brief Retrieves the Mali PMU core object (if any)
 * 
 * @return The Mali PMU object, or NULL if no PMU exists.
 */
struct mali_pmu_core *mali_pmu_get_global_pmu_core(void);

#endif /* __MALI_PMU_H__ */
