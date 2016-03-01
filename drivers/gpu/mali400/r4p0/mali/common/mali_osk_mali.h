/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2013 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_osk_mali.h
 * Defines the OS abstraction layer which is specific for the Mali kernel device driver (OSK)
 */

#ifndef __MALI_OSK_MALI_H__
#define __MALI_OSK_MALI_H__

#include <linux/mali/mali_utgard.h>
#include <mali_osk.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup _mali_osk_miscellaneous
 * @{ */

/** @brief Struct with device specific configuration data
 */
struct _mali_osk_device_data {
	/* Dedicated GPU memory range (physical). */
	u32 dedicated_mem_start;
	u32 dedicated_mem_size;

	/* Shared GPU memory */
	u32 shared_mem_size;

	/* Frame buffer memory to be accessible by Mali GPU (physical) */
	u32 fb_start;
	u32 fb_size;

	/* Max runtime [ms] for jobs */
	int max_job_runtime;

	/* Report GPU utilization in this interval (specified in ms) */
	u32 utilization_interval;

	/* Function that will receive periodic GPU utilization numbers */
	void (*utilization_callback)(struct mali_gpu_utilization_data *data);

	/*
	 * Mali PMU switch delay.
	 * Only needed if the power gates are connected to the PMU in a high fanout
	 * network. This value is the number of Mali clock cycles it takes to
	 * enable the power gates and turn on the power mesh.
	 * This value will have no effect if a daisy chain implementation is used.
	 */
	u32 pmu_switch_delay;

	/* Mali Dynamic power domain configuration in sequence from 0-11
	 *  GP  PP0 PP1  PP2  PP3  PP4  PP5  PP6  PP7, L2$0 L2$1 L2$2
	 */
	u16 pmu_domain_config[12];

	/* Fuction that platform callback for freq tunning, needed when MALI400_POWER_PERFORMANCE_POLICY enabled */
	int (*set_freq_callback)(unsigned int mhz);
};

/** @brief Find Mali GPU HW resource
 *
 * @param addr Address of Mali GPU resource to find
 * @param res Storage for resource information if resource is found.
 * @return _MALI_OSK_ERR_OK on success, _MALI_OSK_ERR_ITEM_NOT_FOUND if resource is not found
 */
_mali_osk_errcode_t _mali_osk_resource_find(u32 addr, _mali_osk_resource_t *res);


/** @brief Find Mali GPU HW base address
 *
 * @return 0 if resources are found, otherwise the Mali GPU component with lowest address.
 */
u32 _mali_osk_resource_base_address(void);

/** @brief Retrieve the Mali GPU specific data
 *
 * @return _MALI_OSK_ERR_OK on success, otherwise failure.
 */
_mali_osk_errcode_t _mali_osk_device_data_get(struct _mali_osk_device_data *data);

/** @brief Determines if Mali GPU has been configured with shared interrupts.
 *
 * @return MALI_TRUE if shared interrupts, MALI_FALSE if not.
 */
mali_bool _mali_osk_shared_interrupts(void);

/** @} */ /* end group _mali_osk_miscellaneous */

/** @addtogroup _mali_osk_low_level_memory
 * @{ */

/** @brief Copy as much data as possible from src to dest, do not crash if src or dest isn't available.
 *
 * @param dest Destination buffer (limited to user space mapped Mali memory)
 * @param src Source buffer
 * @param size Number of bytes to copy
 * @return Number of bytes actually copied
 */
u32 _mali_osk_mem_write_safe(void *dest, const void *src, u32 size);

/** @} */ /* end group _mali_osk_low_level_memory */


#ifdef __cplusplus
}
#endif

#endif /* __MALI_OSK_MALI_H__ */
