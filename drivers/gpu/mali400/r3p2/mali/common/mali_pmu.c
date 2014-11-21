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
 * @file mali_pmu.c
 * Mali driver functions for Mali 400 PMU hardware
 */
#include "mali_hw_core.h"
#include "mali_pmu.h"
#include "mali_pp.h"
#include "mali_kernel_common.h"
#include "mali_osk.h"
#include "mali_pm.h"
#include "mali_osk_mali.h"

static u32 mali_pmu_detect_mask(u32 number_of_pp_cores, u32 number_of_l2_caches);

/** @brief MALI inbuilt PMU hardware info and PMU hardware has knowledge of cores power mask
 */
struct mali_pmu_core
{
	struct mali_hw_core hw_core;
	_mali_osk_lock_t *lock;
	u32 registered_cores_mask;
	u32 active_cores_mask;
	u32 switch_delay;
};

static struct mali_pmu_core *mali_global_pmu_core = NULL;

/** @brief Register layout for hardware PMU
 */
typedef enum {
	PMU_REG_ADDR_MGMT_POWER_UP                  = 0x00,     /*< Power up register */
	PMU_REG_ADDR_MGMT_POWER_DOWN                = 0x04,     /*< Power down register */
	PMU_REG_ADDR_MGMT_STATUS                    = 0x08,     /*< Core sleep status register */
	PMU_REG_ADDR_MGMT_INT_MASK                  = 0x0C,     /*< Interrupt mask register */
	PMU_REG_ADDR_MGMT_INT_RAWSTAT               = 0x10,     /*< Interrupt raw status register */
	PMU_REG_ADDR_MGMT_INT_CLEAR                 = 0x18,     /*< Interrupt clear register */
	PMU_REG_ADDR_MGMT_SW_DELAY                  = 0x1C,     /*< Switch delay register */
	PMU_REGISTER_ADDRESS_SPACE_SIZE             = 0x28,     /*< Size of register space */
} pmu_reg_addr_mgmt_addr;

#define PMU_REG_VAL_IRQ 1

struct mali_pmu_core *mali_pmu_create(_mali_osk_resource_t *resource, u32 number_of_pp_cores, u32 number_of_l2_caches)
{
	struct mali_pmu_core* pmu;

	MALI_DEBUG_ASSERT(NULL == mali_global_pmu_core);
	MALI_DEBUG_PRINT(2, ("Mali PMU: Creating Mali PMU core\n"));

	pmu = (struct mali_pmu_core *)_mali_osk_malloc(sizeof(struct mali_pmu_core));
	if (NULL != pmu)
	{
		pmu->lock = _mali_osk_lock_init(_MALI_OSK_LOCKFLAG_SPINLOCK | _MALI_OSK_LOCKFLAG_NONINTERRUPTABLE,
		                                0, _MALI_OSK_LOCK_ORDER_PMU);
		if (NULL != pmu->lock)
		{
			pmu->registered_cores_mask = mali_pmu_detect_mask(number_of_pp_cores, number_of_l2_caches);
			pmu->active_cores_mask = pmu->registered_cores_mask;

			if (_MALI_OSK_ERR_OK == mali_hw_core_create(&pmu->hw_core, resource, PMU_REGISTER_ADDRESS_SPACE_SIZE))
			{
				_mali_osk_errcode_t err;
				struct _mali_osk_device_data data = { 0, };

				err = _mali_osk_device_data_get(&data);
				if (_MALI_OSK_ERR_OK == err)
				{
					pmu->switch_delay = data.pmu_switch_delay;
					mali_global_pmu_core = pmu;
					return pmu;
				}
				mali_hw_core_delete(&pmu->hw_core);
			}
			_mali_osk_lock_term(pmu->lock);
		}
		_mali_osk_free(pmu);
	}

	return NULL;
}

void mali_pmu_delete(struct mali_pmu_core *pmu)
{
	MALI_DEBUG_ASSERT_POINTER(pmu);
	MALI_DEBUG_ASSERT(pmu == mali_global_pmu_core);
	MALI_DEBUG_PRINT(2, ("Mali PMU: Deleting Mali PMU core\n"));

	_mali_osk_lock_term(pmu->lock);
	mali_hw_core_delete(&pmu->hw_core);
	_mali_osk_free(pmu);
	mali_global_pmu_core = NULL;
}

static void mali_pmu_lock(struct mali_pmu_core *pmu)
{
	_mali_osk_lock_wait(pmu->lock, _MALI_OSK_LOCKMODE_RW);
}
static void mali_pmu_unlock(struct mali_pmu_core *pmu)
{
	_mali_osk_lock_signal(pmu->lock, _MALI_OSK_LOCKMODE_RW);
}

static _mali_osk_errcode_t mali_pmu_send_command_internal(struct mali_pmu_core *pmu, const u32 command, const u32 mask)
{
	u32 rawstat;
	u32 timeout = MALI_REG_POLL_COUNT_SLOW;

	MALI_DEBUG_ASSERT_POINTER(pmu);
	MALI_DEBUG_ASSERT(0 == (mali_hw_core_register_read(&pmu->hw_core, PMU_REG_ADDR_MGMT_INT_RAWSTAT)
	                        & PMU_REG_VAL_IRQ));

	mali_hw_core_register_write(&pmu->hw_core, command, mask);

	/* Wait for the command to complete */
	do
	{
		rawstat = mali_hw_core_register_read(&pmu->hw_core, PMU_REG_ADDR_MGMT_INT_RAWSTAT);
		--timeout;
	} while (0 == (rawstat & PMU_REG_VAL_IRQ) && 0 < timeout);

	MALI_DEBUG_ASSERT(0 < timeout);
	if (0 == timeout)
	{
		return _MALI_OSK_ERR_TIMEOUT;
	}

	mali_hw_core_register_write(&pmu->hw_core, PMU_REG_ADDR_MGMT_INT_CLEAR, PMU_REG_VAL_IRQ);

	return _MALI_OSK_ERR_OK;
}

static _mali_osk_errcode_t mali_pmu_send_command(struct mali_pmu_core *pmu, const u32 command, const u32 mask)
{
	u32 stat;

	if (0 == mask) return _MALI_OSK_ERR_OK;

	stat = mali_hw_core_register_read(&pmu->hw_core, PMU_REG_ADDR_MGMT_STATUS);
	stat &= pmu->registered_cores_mask;

	switch (command)
	{
		case PMU_REG_ADDR_MGMT_POWER_DOWN:
			if (mask == stat) return _MALI_OSK_ERR_OK;
			break;
		case PMU_REG_ADDR_MGMT_POWER_UP:
			if (0 == (stat & mask)) return _MALI_OSK_ERR_OK;
			break;
		default:
			MALI_DEBUG_ASSERT(0);
			break;
	}

	mali_pmu_send_command_internal(pmu, command, mask);

#if defined(DEBUG)
	{
		/* Get power status of cores */
		stat = mali_hw_core_register_read(&pmu->hw_core, PMU_REG_ADDR_MGMT_STATUS);
		stat &= pmu->registered_cores_mask;

		switch (command)
		{
			case PMU_REG_ADDR_MGMT_POWER_DOWN:
				MALI_DEBUG_ASSERT(mask == (stat & mask));
				MALI_DEBUG_ASSERT(0 == (stat & pmu->active_cores_mask));
				MALI_DEBUG_ASSERT((pmu->registered_cores_mask & ~pmu->active_cores_mask) == stat);
				break;
			case PMU_REG_ADDR_MGMT_POWER_UP:
				MALI_DEBUG_ASSERT(0 == (stat & mask));
				MALI_DEBUG_ASSERT(0 == (stat & pmu->active_cores_mask));
				break;
			default:
				MALI_DEBUG_ASSERT(0);
				break;
		}
	}
#endif /* defined(DEBUG) */

	return _MALI_OSK_ERR_OK;
}

_mali_osk_errcode_t mali_pmu_reset(struct mali_pmu_core *pmu)
{
	_mali_osk_errcode_t err;
	u32 cores_off_mask, cores_on_mask, stat;

	mali_pmu_lock(pmu);

	/* Setup the desired defaults */
	mali_hw_core_register_write_relaxed(&pmu->hw_core, PMU_REG_ADDR_MGMT_INT_MASK, 0);
	mali_hw_core_register_write_relaxed(&pmu->hw_core, PMU_REG_ADDR_MGMT_SW_DELAY, pmu->switch_delay);

	/* Get power status of cores */
	stat = mali_hw_core_register_read(&pmu->hw_core, PMU_REG_ADDR_MGMT_STATUS);

	cores_off_mask = pmu->registered_cores_mask & ~(stat | pmu->active_cores_mask);
	cores_on_mask  = pmu->registered_cores_mask &  (stat & pmu->active_cores_mask);

	if (0 != cores_off_mask)
	{
		err = mali_pmu_send_command_internal(pmu, PMU_REG_ADDR_MGMT_POWER_DOWN, cores_off_mask);
		if (_MALI_OSK_ERR_OK != err) return err;
	}

	if (0 != cores_on_mask)
	{
		err = mali_pmu_send_command_internal(pmu, PMU_REG_ADDR_MGMT_POWER_UP, cores_on_mask);
		if (_MALI_OSK_ERR_OK != err) return err;
	}

#if defined(DEBUG)
	{
		stat = mali_hw_core_register_read(&pmu->hw_core, PMU_REG_ADDR_MGMT_STATUS);
		stat &= pmu->registered_cores_mask;

		MALI_DEBUG_ASSERT(stat == (pmu->registered_cores_mask & ~pmu->active_cores_mask));
	}
#endif /* defined(DEBUG) */

	mali_pmu_unlock(pmu);

	return _MALI_OSK_ERR_OK;
}

_mali_osk_errcode_t mali_pmu_power_down(struct mali_pmu_core *pmu, u32 mask)
{
	_mali_osk_errcode_t err;

	MALI_DEBUG_ASSERT_POINTER(pmu);
	MALI_DEBUG_ASSERT(pmu->registered_cores_mask != 0 );

	/* Make sure we have a valid power domain mask */
	if (mask > pmu->registered_cores_mask)
	{
		return _MALI_OSK_ERR_INVALID_ARGS;
	}

	mali_pmu_lock(pmu);

	MALI_DEBUG_PRINT(4, ("Mali PMU: Power down (0x%08X)\n", mask));

	pmu->active_cores_mask &= ~mask;

	_mali_osk_pm_dev_ref_add_no_power_on();
	if (!mali_pm_is_power_on())
	{
		/* Don't touch hardware if all of Mali is powered off. */
		_mali_osk_pm_dev_ref_dec_no_power_on();
		mali_pmu_unlock(pmu);

		MALI_DEBUG_PRINT(4, ("Mali PMU: Skipping power down (0x%08X) since Mali is off\n", mask));

		return _MALI_OSK_ERR_BUSY;
	}

	err = mali_pmu_send_command(pmu, PMU_REG_ADDR_MGMT_POWER_DOWN, mask);

	_mali_osk_pm_dev_ref_dec_no_power_on();
	mali_pmu_unlock(pmu);

	return err;
}

_mali_osk_errcode_t mali_pmu_power_up(struct mali_pmu_core *pmu, u32 mask)
{
	_mali_osk_errcode_t err;

	MALI_DEBUG_ASSERT_POINTER(pmu);
	MALI_DEBUG_ASSERT(pmu->registered_cores_mask != 0 );

	/* Make sure we have a valid power domain mask */
	if (mask & ~pmu->registered_cores_mask)
	{
		return _MALI_OSK_ERR_INVALID_ARGS;
	}

	mali_pmu_lock(pmu);

	MALI_DEBUG_PRINT(4, ("Mali PMU: Power up (0x%08X)\n", mask));

	pmu->active_cores_mask |= mask;

	_mali_osk_pm_dev_ref_add_no_power_on();
	if (!mali_pm_is_power_on())
	{
		/* Don't touch hardware if all of Mali is powered off. */
		_mali_osk_pm_dev_ref_dec_no_power_on();
		mali_pmu_unlock(pmu);

		MALI_DEBUG_PRINT(4, ("Mali PMU: Skipping power up (0x%08X) since Mali is off\n", mask));

		return _MALI_OSK_ERR_BUSY;
	}

	err = mali_pmu_send_command(pmu, PMU_REG_ADDR_MGMT_POWER_UP, mask);

	_mali_osk_pm_dev_ref_dec_no_power_on();
	mali_pmu_unlock(pmu);

	return err;
}

_mali_osk_errcode_t mali_pmu_power_down_all(struct mali_pmu_core *pmu)
{
	_mali_osk_errcode_t err;

	MALI_DEBUG_ASSERT_POINTER(pmu);
	MALI_DEBUG_ASSERT(pmu->registered_cores_mask != 0);

	mali_pmu_lock(pmu);

	/* Setup the desired defaults in case we were called before mali_pmu_reset() */
	mali_hw_core_register_write_relaxed(&pmu->hw_core, PMU_REG_ADDR_MGMT_INT_MASK, 0);
	mali_hw_core_register_write_relaxed(&pmu->hw_core, PMU_REG_ADDR_MGMT_SW_DELAY, pmu->switch_delay);

	err = mali_pmu_send_command(pmu, PMU_REG_ADDR_MGMT_POWER_DOWN, pmu->registered_cores_mask);

	mali_pmu_unlock(pmu);

	return err;
}

_mali_osk_errcode_t mali_pmu_power_up_all(struct mali_pmu_core *pmu)
{
	_mali_osk_errcode_t err;

	MALI_DEBUG_ASSERT_POINTER(pmu);
	MALI_DEBUG_ASSERT(pmu->registered_cores_mask != 0);

	mali_pmu_lock(pmu);

	/* Setup the desired defaults in case we were called before mali_pmu_reset() */
	mali_hw_core_register_write_relaxed(&pmu->hw_core, PMU_REG_ADDR_MGMT_INT_MASK, 0);
	mali_hw_core_register_write_relaxed(&pmu->hw_core, PMU_REG_ADDR_MGMT_SW_DELAY, pmu->switch_delay);

	err = mali_pmu_send_command(pmu, PMU_REG_ADDR_MGMT_POWER_UP, pmu->active_cores_mask);

	mali_pmu_unlock(pmu);
	return err;
}

struct mali_pmu_core *mali_pmu_get_global_pmu_core(void)
{
	return mali_global_pmu_core;
}

static u32 mali_pmu_detect_mask(u32 number_of_pp_cores, u32 number_of_l2_caches)
{
	u32 mask = 0;

	if (number_of_l2_caches == 1)
	{
		/* Mali-300 or Mali-400 */
		u32 i;

		/* GP */
		mask = 0x01;

		/* L2 cache */
		mask |= 0x01<<1;

		/* Set bit for each PP core */
		for (i = 0; i < number_of_pp_cores; i++)
		{
			mask |= 0x01<<(i+2);
		}
	}
	else if (number_of_l2_caches > 1)
	{
		/* Mali-450 */

		/* GP (including its L2 cache) */
		mask = 0x01;

		/* There is always at least one PP (including its L2 cache) */
		mask |= 0x01<<1;

		/* Additional PP cores in same L2 cache */
		if (number_of_pp_cores >= 2)
		{
			mask |= 0x01<<2;
		}

		/* Additional PP cores in a third L2 cache */
		if (number_of_pp_cores >= 5)
		{
			mask |= 0x01<<3;
		}
	}

	MALI_DEBUG_PRINT(4, ("Mali PMU: Power mask is 0x%08X (%u + %u)\n", mask, number_of_pp_cores, number_of_l2_caches));

	return mask;
}
