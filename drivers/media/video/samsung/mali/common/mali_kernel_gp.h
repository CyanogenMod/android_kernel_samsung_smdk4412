/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __MALI_KERNEL_GP2_H__
#define __MALI_KERNEL_GP2_H__

extern struct mali_kernel_subsystem mali_subsystem_gp2;

#if USING_MALI_PMM
_mali_osk_errcode_t maligp_signal_power_up( mali_bool queue_only );
_mali_osk_errcode_t maligp_signal_power_down( mali_bool immediate_only );
#endif

#endif /* __MALI_KERNEL_GP2_H__ */
