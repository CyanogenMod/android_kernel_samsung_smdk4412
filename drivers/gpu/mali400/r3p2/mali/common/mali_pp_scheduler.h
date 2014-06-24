/*
 * Copyright (C) 2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __MALI_PP_SCHEDULER_H__
#define __MALI_PP_SCHEDULER_H__

#include "mali_osk.h"
#include "mali_pp_job.h"
#include "mali_group.h"
#include "linux/mali/mali_utgard.h"

/** Initalize the HW independent parts of the  PP scheduler
 */
_mali_osk_errcode_t mali_pp_scheduler_initialize(void);
void mali_pp_scheduler_terminate(void);

/** Poplulate the PP scheduler with groups
 */
void mali_pp_scheduler_populate(void);
void mali_pp_scheduler_depopulate(void);

void mali_pp_scheduler_job_done(struct mali_group *group, struct mali_pp_job *job, u32 sub_job, mali_bool success);

void mali_pp_scheduler_suspend(void);
void mali_pp_scheduler_resume(void);

/** @brief Abort all PP jobs from session running or queued
 *
 * This functions aborts all PP jobs from the specified session. Queued jobs are removed from the queue and jobs
 * currently running on a core will be aborted.
 *
 * @param session Pointer to session whose jobs should be aborted
 */
void mali_pp_scheduler_abort_session(struct mali_session_data *session);

/**
 * @brief Reset all groups
 *
 * This function resets all groups known by the PP scheuduler. This must be
 * called after the Mali HW has been powered on in order to reset the HW.
 *
 * This function is intended for power on reset of all cores.
 * No locking is done, which can only be safe if the scheduler is paused and
 * all cores idle. That is always the case on init and power on.
 */
void mali_pp_scheduler_reset_all_groups(void);

/**
 * @brief Zap TLB on all groups with \a session active
 *
 * The scheculer will zap the session on all groups it owns.
 */
void mali_pp_scheduler_zap_all_active(struct mali_session_data *session);

int mali_pp_scheduler_get_queue_depth(void);
u32 mali_pp_scheduler_dump_state(char *buf, u32 size);

void mali_pp_scheduler_enable_group(struct mali_group *group);
void mali_pp_scheduler_disable_group(struct mali_group *group);

int mali_pp_scheduler_set_perf_level(u32 cores);

u32 mali_pp_scheduler_get_num_cores_total(void);
u32 mali_pp_scheduler_get_num_cores_enabled(void);

/**
 * @brief Returns the number of Pixel Processors in the system irrespective of the context
 *
 * @return number of physical Pixel Processor cores in the system
 */
u32 mali_pp_scheduler_get_num_cores_total(void);

#endif /* __MALI_PP_SCHEDULER_H__ */
