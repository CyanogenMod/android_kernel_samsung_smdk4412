/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __MALI_PM_DOMAIN_H__
#define __MALI_PM_DOMAIN_H__

#include "mali_kernel_common.h"
#include "mali_osk.h"

#include "mali_l2_cache.h"
#include "mali_group.h"
#include "mali_pmu.h"

typedef enum
{
	MALI_PM_DOMAIN_ON,
	MALI_PM_DOMAIN_OFF,
} mali_pm_domain_state;

struct mali_pm_domain
{
	mali_pm_domain_state state;
	_mali_osk_lock_t *lock;

	s32 use_count;

	u32 pmu_mask;

	int group_count;
	struct mali_group *group_list;

	struct mali_l2_cache_core *l2;
};

struct mali_pm_domain *mali_pm_domain_create(u32 id, u32 pmu_mask);
void mali_pm_domain_add_group(u32 id, struct mali_group *group);
void mali_pm_domain_add_l2(u32 id, struct mali_l2_cache_core *l2);
void mali_pm_domain_delete(struct mali_pm_domain *domain);

void mali_pm_domain_terminate(void);

/** Get PM domain from domain ID
 */
struct mali_pm_domain *mali_pm_domain_get(u32 id);

/* Ref counting */
void mali_pm_domain_ref_get(struct mali_pm_domain *domain);
void mali_pm_domain_ref_put(struct mali_pm_domain *domain);

MALI_STATIC_INLINE struct mali_l2_cache_core *mali_pm_domain_l2_get(struct mali_pm_domain *domain)
{
	return domain->l2;
}

MALI_STATIC_INLINE mali_pm_domain_state mali_pm_domain_state_get(struct mali_pm_domain *domain)
{
	return domain->state;
}

mali_bool mali_pm_domain_lock_state(struct mali_pm_domain *domain);
void mali_pm_domain_unlock_state(struct mali_pm_domain *domain);

#define MALI_PM_DOMAIN_FOR_EACH_GROUP(group, domain) for ((group) = (domain)->group_list;\
		NULL != (group); (group) = (group)->pm_domain_list)

#endif /* __MALI_PM_DOMAIN_H__ */
