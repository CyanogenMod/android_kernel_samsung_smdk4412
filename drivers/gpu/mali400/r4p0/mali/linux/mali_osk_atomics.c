/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2010, 2013 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_osk_atomics.c
 * Implementation of the OS abstraction layer for the kernel device driver
 */

#include "mali_osk.h"
#include <asm/atomic.h>
#include "mali_kernel_common.h"

void _mali_osk_atomic_dec( _mali_osk_atomic_t *atom )
{
	atomic_dec((atomic_t *)&atom->u.val);
}

u32 _mali_osk_atomic_dec_return( _mali_osk_atomic_t *atom )
{
	return atomic_dec_return((atomic_t *)&atom->u.val);
}

void _mali_osk_atomic_inc( _mali_osk_atomic_t *atom )
{
	atomic_inc((atomic_t *)&atom->u.val);
}

u32 _mali_osk_atomic_inc_return( _mali_osk_atomic_t *atom )
{
	return atomic_inc_return((atomic_t *)&atom->u.val);
}

_mali_osk_errcode_t _mali_osk_atomic_init( _mali_osk_atomic_t *atom, u32 val )
{
	MALI_CHECK_NON_NULL(atom, _MALI_OSK_ERR_INVALID_ARGS);
	atomic_set((atomic_t *)&atom->u.val, val);
	return _MALI_OSK_ERR_OK;
}

u32 _mali_osk_atomic_read( _mali_osk_atomic_t *atom )
{
	return atomic_read((atomic_t *)&atom->u.val);
}

void _mali_osk_atomic_term( _mali_osk_atomic_t *atom )
{
	MALI_IGNORE(atom);
}

u32 _mali_osk_atomic_xchg( _mali_osk_atomic_t *atom, u32 val )
{
	return atomic_xchg((atomic_t*)&atom->u.val, val);
}
