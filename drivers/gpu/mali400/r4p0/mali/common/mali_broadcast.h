/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2012-2013 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/*
 *  Interface for the broadcast unit on Mali-450.
 *
 * - Represents up to 8 × (MMU + PP) pairs.
 * - Supports dynamically changing which (MMU + PP) pairs receive the broadcast by
 *   setting a mask.
 */

#include "mali_hw_core.h"
#include "mali_group.h"

struct mali_bcast_unit;

struct mali_bcast_unit *mali_bcast_unit_create(const _mali_osk_resource_t *resource);
void mali_bcast_unit_delete(struct mali_bcast_unit *bcast_unit);

/* Add a group to the list of (MMU + PP) pairs broadcasts go out to. */
void mali_bcast_add_group(struct mali_bcast_unit *bcast_unit, struct mali_group *group);

/* Remove a group to the list of (MMU + PP) pairs broadcasts go out to. */
void mali_bcast_remove_group(struct mali_bcast_unit *bcast_unit, struct mali_group *group);

/* Re-set cached mask. This needs to be called after having been suspended. */
void mali_bcast_reset(struct mali_bcast_unit *bcast_unit);

/**
 * Disable broadcast unit
 *
 * mali_bcast_enable must be called to re-enable the unit. Cores may not be
 * added or removed when the unit is disabled.
 */
void mali_bcast_disable(struct mali_bcast_unit *bcast_unit);

/**
 * Re-enable broadcast unit
 *
 * This resets the masks to include the cores present when mali_bcast_disable was called.
 */
MALI_STATIC_INLINE void mali_bcast_enable(struct mali_bcast_unit *bcast_unit)
{
	mali_bcast_reset(bcast_unit);
}
