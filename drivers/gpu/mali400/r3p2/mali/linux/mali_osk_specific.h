/*
 * Copyright (C) 2010, 2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_osk_specific.h
 * Defines per-OS Kernel level specifics, such as unusual workarounds for
 * certain OSs.
 */

#ifndef __MALI_OSK_SPECIFIC_H__
#define __MALI_OSK_SPECIFIC_H__

#include <asm/uaccess.h>

#include "mali_sync.h"

#define MALI_STATIC_INLINE static inline
#define MALI_NON_STATIC_INLINE inline

#ifdef CONFIG_SYNC
typedef struct sync_timeline mali_sync_tl;
typedef struct sync_pt mali_sync_pt;
#endif /* CONFIG_SYNC */

MALI_STATIC_INLINE u32 _mali_osk_copy_from_user(void *to, void *from, u32 n)
{
	return (u32)copy_from_user(to, from, (unsigned long)n);
}

#endif /* __MALI_OSK_SPECIFIC_H__ */
