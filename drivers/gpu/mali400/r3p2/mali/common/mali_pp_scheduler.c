/*
 * Copyright (C) 2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mali_pp_scheduler.h"
#include "mali_kernel_common.h"
#include "mali_kernel_core.h"
#include "mali_osk.h"
#include "mali_osk_list.h"
#include "mali_scheduler.h"
#include "mali_pp.h"
#include "mali_pp_job.h"
#include "mali_group.h"
#include "mali_pm.h"
#include "mali_kernel_utilization.h"
#include "mali_session.h"
#include "mali_pm_domain.h"
#include "linux/mali/mali_utgard.h"

#if defined(CONFIG_DMA_SHARED_BUFFER)
#include "mali_dma_buf.h"
#endif
#if defined(CONFIG_GPU_TRACEPOINTS) && defined(CONFIG_TRACEPOINTS)
#include <linux/sched.h>
#include <trace/events/gpu.h>
#endif


/* With certain configurations, job deletion involves functions which cannot be called from atomic context.
 * This #if checks for those cases and enables job deletion to be deferred and run in a different context. */
#if defined(CONFIG_SYNC) || !defined(CONFIG_MALI_DMA_BUF_MAP_ON_ATTACH)
#define MALI_PP_SCHEDULER_USE_DEFERRED_JOB_DELETE 1
#endif

/* Maximum of 8 PP cores (a group can only have maximum of 1 PP core) */
#define MALI_MAX_NUMBER_OF_PP_GROUPS 9

static mali_bool mali_pp_scheduler_is_suspended(void);
static void mali_pp_scheduler_do_schedule(void *arg);
#if defined(MALI_PP_SCHEDULER_USE_DEFERRED_JOB_DELETE)
static void mali_pp_scheduler_do_job_delete(void *arg);
#endif
static void mali_pp_scheduler_job_queued(void);
static void mali_pp_scheduler_job_completed(void);

static u32 pp_version = 0;

/* Physical job queue */
static _MALI_OSK_LIST_HEAD_STATIC_INIT(job_queue);              /* List of physical jobs with some unscheduled work */
static u32 job_queue_depth = 0;

/* Physical groups */
static _MALI_OSK_LIST_HEAD_STATIC_INIT(group_list_working);     /* List of physical groups with working jobs on the pp core */
static _MALI_OSK_LIST_HEAD_STATIC_INIT(group_list_idle);        /* List of physical groups with idle jobs on the pp core */
static _MALI_OSK_LIST_HEAD_STATIC_INIT(group_list_disabled);    /* List of disabled physical groups */

/* Virtual job queue (Mali-450 only) */
static _MALI_OSK_LIST_HEAD_STATIC_INIT(virtual_job_queue);      /* List of unstarted jobs for the virtual group */
static u32 virtual_job_queue_depth = 0;

/* Virtual group (Mali-450 only) */
static struct mali_group *virtual_group = NULL;                 /* Virtual group (if any) */
static enum
{
	VIRTUAL_GROUP_IDLE,
	VIRTUAL_GROUP_WORKING,
	VIRTUAL_GROUP_DISABLED,
}
virtual_group_state = VIRTUAL_GROUP_IDLE;            /* Flag which indicates whether the virtual group is working or idle */

/* Number of physical cores */
static u32 num_cores = 0;
static u32 enabled_cores = 0;

/* Variables to allow safe pausing of the scheduler */
static _mali_osk_wait_queue_t *pp_scheduler_working_wait_queue = NULL;
static u32 pause_count = 0;

static _mali_osk_lock_t *pp_scheduler_lock = NULL;
/* Contains tid of thread that locked the scheduler or 0, if not locked */
MALI_DEBUG_CODE(static u32 pp_scheduler_lock_owner = 0);

static _mali_osk_wq_work_t *pp_scheduler_wq_schedule = NULL;

#if defined(MALI_PP_SCHEDULER_USE_DEFERRED_JOB_DELETE)
static _mali_osk_wq_work_t *pp_scheduler_wq_job_delete = NULL;
static _mali_osk_lock_t *pp_scheduler_job_delete_lock = NULL;
static _MALI_OSK_LIST_HEAD_STATIC_INIT(pp_scheduler_job_deletion_queue);
#endif

MALI_STATIC_INLINE mali_bool mali_pp_scheduler_has_virtual_group(void)
{
	return NULL != virtual_group;
}

_mali_osk_errcode_t mali_pp_scheduler_initialize(void)
{
	_mali_osk_lock_flags_t lock_flags;

#if defined(MALI_UPPER_HALF_SCHEDULING)
	lock_flags = _MALI_OSK_LOCKFLAG_ORDERED | _MALI_OSK_LOCKFLAG_SPINLOCK_IRQ | _MALI_OSK_LOCKFLAG_NONINTERRUPTABLE;
#else
	lock_flags = _MALI_OSK_LOCKFLAG_ORDERED | _MALI_OSK_LOCKFLAG_SPINLOCK | _MALI_OSK_LOCKFLAG_NONINTERRUPTABLE;
#endif

	pp_scheduler_lock = _mali_osk_lock_init(lock_flags, 0, _MALI_OSK_LOCK_ORDER_SCHEDULER);
	if (NULL == pp_scheduler_lock)
	{
		return _MALI_OSK_ERR_NOMEM;
	}

	pp_scheduler_working_wait_queue = _mali_osk_wait_queue_init();
	if (NULL == pp_scheduler_working_wait_queue)
	{
		_mali_osk_lock_term(pp_scheduler_lock);
		return _MALI_OSK_ERR_NOMEM;
	}

	pp_scheduler_wq_schedule = _mali_osk_wq_create_work(mali_pp_scheduler_do_schedule, NULL);
	if (NULL == pp_scheduler_wq_schedule)
	{
		_mali_osk_wait_queue_term(pp_scheduler_working_wait_queue);
		_mali_osk_lock_term(pp_scheduler_lock);
		return _MALI_OSK_ERR_NOMEM;
	}

#if defined(MALI_PP_SCHEDULER_USE_DEFERRED_JOB_DELETE)
	pp_scheduler_wq_job_delete = _mali_osk_wq_create_work(mali_pp_scheduler_do_job_delete, NULL);
	if (NULL == pp_scheduler_wq_job_delete)
	{
		_mali_osk_wq_delete_work(pp_scheduler_wq_schedule);
		_mali_osk_wait_queue_term(pp_scheduler_working_wait_queue);
		_mali_osk_lock_term(pp_scheduler_lock);
		return _MALI_OSK_ERR_NOMEM;
	}

	pp_scheduler_job_delete_lock = _mali_osk_lock_init(_MALI_OSK_LOCKFLAG_ORDERED | _MALI_OSK_LOCKFLAG_SPINLOCK_IRQ |_MALI_OSK_LOCKFLAG_NONINTERRUPTABLE, 0, _MALI_OSK_LOCK_ORDER_SCHEDULER_DEFERRED);
	if (NULL == pp_scheduler_job_delete_lock)
	{
		_mali_osk_wq_delete_work(pp_scheduler_wq_job_delete);
		_mali_osk_wq_delete_work(pp_scheduler_wq_schedule);
		_mali_osk_wait_queue_term(pp_scheduler_working_wait_queue);
		_mali_osk_lock_term(pp_scheduler_lock);
		return _MALI_OSK_ERR_NOMEM;
	}
#endif

	return _MALI_OSK_ERR_OK;
}

void mali_pp_scheduler_terminate(void)
{
#if defined(MALI_PP_SCHEDULER_USE_DEFERRED_JOB_DELETE)
	_mali_osk_lock_term(pp_scheduler_job_delete_lock);
	_mali_osk_wq_delete_work(pp_scheduler_wq_job_delete);
#endif

	_mali_osk_wq_delete_work(pp_scheduler_wq_schedule);
	_mali_osk_wait_queue_term(pp_scheduler_working_wait_queue);
	_mali_osk_lock_term(pp_scheduler_lock);
}

void mali_pp_scheduler_populate(void)
{
	struct mali_group *group;
	struct mali_pp_core *pp_core;
	u32 num_groups;
	u32 i;

	num_groups = mali_group_get_glob_num_groups();

	/* Do we have a virtual group? */
	for (i = 0; i < num_groups; i++)
	{
		group = mali_group_get_glob_group(i);

		if (mali_group_is_virtual(group))
		{
			MALI_DEBUG_PRINT(3, ("Found virtual group %p\n", group));

			virtual_group = group;
			break;
		}
	}

	/* Find all the available PP cores */
	for (i = 0; i < num_groups; i++)
	{
		group = mali_group_get_glob_group(i);
		pp_core = mali_group_get_pp_core(group);

		if (NULL != pp_core && !mali_group_is_virtual(group))
		{
			if (0 == pp_version)
			{
				/* Retrieve PP version from the first available PP core */
				pp_version = mali_pp_core_get_version(pp_core);
			}

			if (mali_pp_scheduler_has_virtual_group())
			{
				/* Add all physical PP cores to the virtual group */
				mali_group_lock(virtual_group);
				group->state = MALI_GROUP_STATE_JOINING_VIRTUAL;
				mali_group_add_group(virtual_group, group, MALI_TRUE);
				mali_group_unlock(virtual_group);
			}
			else
			{
				_mali_osk_list_add(&group->pp_scheduler_list, &group_list_idle);
			}

			num_cores++;
		}
	}
	enabled_cores = num_cores;
}

void mali_pp_scheduler_depopulate(void)
{
	struct mali_group *group, *temp;

	MALI_DEBUG_ASSERT(_mali_osk_list_empty(&group_list_working));
	MALI_DEBUG_ASSERT(VIRTUAL_GROUP_WORKING != virtual_group_state);

	/* Delete all groups owned by scheduler */
	if (mali_pp_scheduler_has_virtual_group())
	{
		mali_group_delete(virtual_group);
	}

	_MALI_OSK_LIST_FOREACHENTRY(group, temp, &group_list_idle, struct mali_group, pp_scheduler_list)
	{
		mali_group_delete(group);
	}
	_MALI_OSK_LIST_FOREACHENTRY(group, temp, &group_list_disabled, struct mali_group, pp_scheduler_list)
	{
		mali_group_delete(group);
	}
}

MALI_STATIC_INLINE void mali_pp_scheduler_lock(void)
{
	if(_MALI_OSK_ERR_OK != _mali_osk_lock_wait(pp_scheduler_lock, _MALI_OSK_LOCKMODE_RW))
	{
		/* Non-interruptable lock failed: this should never happen. */
		MALI_DEBUG_ASSERT(0);
	}
	MALI_DEBUG_PRINT(5, ("Mali PP scheduler: PP scheduler lock taken\n"));
	MALI_DEBUG_ASSERT(0 == pp_scheduler_lock_owner);
	MALI_DEBUG_CODE(pp_scheduler_lock_owner = _mali_osk_get_tid());
}

MALI_STATIC_INLINE void mali_pp_scheduler_unlock(void)
{
	MALI_DEBUG_PRINT(5, ("Mali PP scheduler: Releasing PP scheduler lock\n"));
	MALI_DEBUG_ASSERT(_mali_osk_get_tid() == pp_scheduler_lock_owner);
	MALI_DEBUG_CODE(pp_scheduler_lock_owner = 0);
	_mali_osk_lock_signal(pp_scheduler_lock, _MALI_OSK_LOCKMODE_RW);
}

MALI_STATIC_INLINE void mali_pp_scheduler_disable_empty_virtual(void)
{
	MALI_ASSERT_GROUP_LOCKED(virtual_group);

	if (mali_group_virtual_disable_if_empty(virtual_group))
	{
		MALI_DEBUG_PRINT(4, ("Disabling empty virtual group\n"));

		MALI_DEBUG_ASSERT(VIRTUAL_GROUP_IDLE == virtual_group_state);

		virtual_group_state = VIRTUAL_GROUP_DISABLED;
	}
}

MALI_STATIC_INLINE void mali_pp_scheduler_enable_empty_virtual(void)
{
	MALI_ASSERT_GROUP_LOCKED(virtual_group);

	if (mali_group_virtual_enable_if_empty(virtual_group))
	{
		MALI_DEBUG_PRINT(4, ("Re-enabling empty virtual group\n"));

		MALI_DEBUG_ASSERT(VIRTUAL_GROUP_DISABLED == virtual_group_state);

		virtual_group_state = VIRTUAL_GROUP_IDLE;
	}
}

#ifdef DEBUG
MALI_STATIC_INLINE void mali_pp_scheduler_assert_locked(void)
{
	MALI_DEBUG_ASSERT(_mali_osk_get_tid() == pp_scheduler_lock_owner);
}
#define MALI_ASSERT_PP_SCHEDULER_LOCKED() mali_pp_scheduler_assert_locked()
#else
#define MALI_ASSERT_PP_SCHEDULER_LOCKED()
#endif

/**
 * Returns a physical job if a physical job is ready to run (no barrier present)
 */
MALI_STATIC_INLINE struct mali_pp_job *mali_pp_scheduler_get_physical_job(void)
{
	MALI_ASSERT_PP_SCHEDULER_LOCKED();

	if (!_mali_osk_list_empty(&job_queue))
	{
		struct mali_pp_job *job;

		MALI_DEBUG_ASSERT(job_queue_depth > 0);
		job = _MALI_OSK_LIST_ENTRY(job_queue.next, struct mali_pp_job, list);

		if (!mali_pp_job_has_active_barrier(job))
		{
			return job;
		}
	}

	return NULL;
}

MALI_STATIC_INLINE void mali_pp_scheduler_dequeue_physical_job(struct mali_pp_job *job)
{
	MALI_ASSERT_PP_SCHEDULER_LOCKED();
	MALI_DEBUG_ASSERT(job_queue_depth > 0);

	/* Remove job from queue */
	if (!mali_pp_job_has_unstarted_sub_jobs(job))
	{
		/* All sub jobs have been started: remove job from queue */
		_mali_osk_list_delinit(&job->list);
	}

	--job_queue_depth;
}

/**
 * Returns a virtual job if a virtual job is ready to run (no barrier present)
 */
MALI_STATIC_INLINE struct mali_pp_job *mali_pp_scheduler_get_virtual_job(void)
{
	MALI_ASSERT_PP_SCHEDULER_LOCKED();
	MALI_DEBUG_ASSERT_POINTER(virtual_group);

	if (!_mali_osk_list_empty(&virtual_job_queue))
	{
		struct mali_pp_job *job;

		MALI_DEBUG_ASSERT(virtual_job_queue_depth > 0);
		job = _MALI_OSK_LIST_ENTRY(virtual_job_queue.next, struct mali_pp_job, list);

		if (!mali_pp_job_has_active_barrier(job))
		{
			return job;
		}
	}

	return NULL;
}

MALI_STATIC_INLINE void mali_pp_scheduler_dequeue_virtual_job(struct mali_pp_job *job)
{
	MALI_ASSERT_PP_SCHEDULER_LOCKED();
	MALI_DEBUG_ASSERT(virtual_job_queue_depth > 0);

	/* Remove job from queue */
	_mali_osk_list_delinit(&job->list);
	--virtual_job_queue_depth;
}

/**
 * Checks if the criteria is met for removing a physical core from virtual group
 */
MALI_STATIC_INLINE mali_bool mali_pp_scheduler_can_move_virtual_to_physical(void)
{
	MALI_ASSERT_PP_SCHEDULER_LOCKED();
	MALI_DEBUG_ASSERT(mali_pp_scheduler_has_virtual_group());
	MALI_ASSERT_GROUP_LOCKED(virtual_group);
	/*
	 * The criteria for taking out a physical group from a virtual group are the following:
	 * - There virtual group is idle
	 * - There are currently no physical groups (idle and working)
	 * - There are physical jobs to be scheduled (without a barrier)
	 */
	return (VIRTUAL_GROUP_IDLE == virtual_group_state) &&
	       _mali_osk_list_empty(&group_list_idle) &&
	       _mali_osk_list_empty(&group_list_working) &&
	       (NULL != mali_pp_scheduler_get_physical_job());
}

MALI_STATIC_INLINE struct mali_group *mali_pp_scheduler_acquire_physical_group(void)
{
	MALI_ASSERT_PP_SCHEDULER_LOCKED();

	if (!_mali_osk_list_empty(&group_list_idle))
	{
		MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Acquiring physical group from idle list\n"));
		return _MALI_OSK_LIST_ENTRY(group_list_idle.next, struct mali_group, pp_scheduler_list);
	}
	else if (mali_pp_scheduler_has_virtual_group())
	{
		MALI_ASSERT_GROUP_LOCKED(virtual_group);
		if (mali_pp_scheduler_can_move_virtual_to_physical())
		{
			struct mali_group *group;
			MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Acquiring physical group from virtual group\n"));
			group = mali_group_acquire_group(virtual_group);

			if (mali_pp_scheduler_has_virtual_group())
			{
				mali_pp_scheduler_disable_empty_virtual();
			}

			return group;
		}
	}

	return NULL;
}

static void mali_pp_scheduler_schedule(void)
{
	struct mali_group* physical_groups_to_start[MALI_MAX_NUMBER_OF_PP_GROUPS-1];
	struct mali_pp_job* physical_jobs_to_start[MALI_MAX_NUMBER_OF_PP_GROUPS-1];
	u32 physical_subjobs_to_start[MALI_MAX_NUMBER_OF_PP_GROUPS-1];
	int num_physical_jobs_to_start = 0;
	int i;

	if (mali_pp_scheduler_has_virtual_group())
	{
		/* Need to lock the virtual group because we might need to grab a physical group from it */
		mali_group_lock(virtual_group);
	}

	mali_pp_scheduler_lock();
	if (pause_count > 0)
	{
		/* Scheduler is suspended, don't schedule any jobs */
		mali_pp_scheduler_unlock();
		if (mali_pp_scheduler_has_virtual_group())
		{
			mali_group_unlock(virtual_group);
		}
		return;
	}

	/* Find physical job(s) to schedule first */
	while (1)
	{
		struct mali_group *group;
		struct mali_pp_job *job;
		u32 subjob;

		job = mali_pp_scheduler_get_physical_job();
		if (NULL == job)
		{
			break; /* No job, early out */
		}

		MALI_DEBUG_ASSERT(!mali_pp_job_is_virtual(job));
		MALI_DEBUG_ASSERT(mali_pp_job_has_unstarted_sub_jobs(job));
		MALI_DEBUG_ASSERT(1 <= mali_pp_job_get_sub_job_count(job));

		/* Acquire a physical group, either from the idle list or from the virtual group.
		 * In case the group was acquired from the virtual group, it's state will be
		 * LEAVING_VIRTUAL and must be set to IDLE before it can be used. */
		group = mali_pp_scheduler_acquire_physical_group();
		if (NULL == group)
		{
			/* Could not get a group to run the job on, early out */
			MALI_DEBUG_PRINT(4, ("Mali PP scheduler: No more physical groups available.\n"));
			break;
		}

		MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Acquired physical group %p\n", group));

		/* Mark subjob as started */
		subjob = mali_pp_job_get_first_unstarted_sub_job(job);
		mali_pp_job_mark_sub_job_started(job, subjob);

		/* Remove job from queue (if we now got the last subjob) */
		mali_pp_scheduler_dequeue_physical_job(job);

		/* Move group to working list */
		_mali_osk_list_move(&(group->pp_scheduler_list), &group_list_working);

		/* Keep track of this group, so that we actually can start the job once we are done with the scheduler lock we are now holding */
		physical_groups_to_start[num_physical_jobs_to_start] = group;
		physical_jobs_to_start[num_physical_jobs_to_start] = job;
		physical_subjobs_to_start[num_physical_jobs_to_start] = subjob;
		++num_physical_jobs_to_start;

		MALI_DEBUG_ASSERT(num_physical_jobs_to_start < MALI_MAX_NUMBER_OF_PP_GROUPS);
	}

	/* See if we have a virtual job to schedule */
	if (mali_pp_scheduler_has_virtual_group())
	{
		if (VIRTUAL_GROUP_IDLE == virtual_group_state)
		{
			struct mali_pp_job *job = mali_pp_scheduler_get_virtual_job();
			if (NULL != job)
			{
				MALI_DEBUG_ASSERT(mali_pp_job_is_virtual(job));
				MALI_DEBUG_ASSERT(mali_pp_job_has_unstarted_sub_jobs(job));
				MALI_DEBUG_ASSERT(1 == mali_pp_job_get_sub_job_count(job));

				/* Mark the one and only subjob as started */
				mali_pp_job_mark_sub_job_started(job, 0);

				/* Remove job from queue */
				mali_pp_scheduler_dequeue_virtual_job(job);

				/* Virtual group is now working */
				virtual_group_state = VIRTUAL_GROUP_WORKING;

				/*
				 * We no longer need the scheduler lock,
				 * but we still need the virtual lock in order to start the virtual job
				 */
				mali_pp_scheduler_unlock();

				/* Start job */
				mali_group_start_pp_job(virtual_group, job, 0);
				MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Virtual job %u (0x%08X) part %u/%u started (from schedule)\n",
				                     mali_pp_job_get_id(job), job, 1,
				                     mali_pp_job_get_sub_job_count(job)));

				/* And now we are all done with the virtual_group lock as well */
				mali_group_unlock(virtual_group);
			}
			else
			{
				/* No virtual job, release the two locks we are holding */
				mali_pp_scheduler_unlock();
				mali_group_unlock(virtual_group);
			}
		}
		else
		{
			/* Virtual core busy, release the two locks we are holding */
			mali_pp_scheduler_unlock();
			mali_group_unlock(virtual_group);
		}

	}
	else
	{
		/* There is no virtual group, release the only lock we are holding */
		mali_pp_scheduler_unlock();
	}

	/*
	 * Now we have released the scheduler lock, and we are ready to kick of the actual starting of the
	 * physical jobs.
	 * The reason we want to wait until we have released the scheduler lock is that job start actually
	 * may take quite a bit of time (quite many registers needs to be written). This will allow new jobs
	 * from user space to come in, and post processing of other PP jobs to happen at the same time as we
	 * start jobs.
	 */
	for (i = 0; i < num_physical_jobs_to_start; i++)
	{
		struct mali_group *group = physical_groups_to_start[i];
		struct mali_pp_job *job  = physical_jobs_to_start[i];
		u32 sub_job              = physical_subjobs_to_start[i];

		MALI_DEBUG_ASSERT_POINTER(group);
		MALI_DEBUG_ASSERT_POINTER(job);

		mali_group_lock(group);

		/* In case this group was acquired from a virtual core, update it's state to IDLE */
		group->state = MALI_GROUP_STATE_IDLE;

		mali_group_start_pp_job(group, job, sub_job);
		MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Physical job %u (0x%08X) part %u/%u started (from schedule)\n",
		                     mali_pp_job_get_id(job), job, sub_job + 1,
		                     mali_pp_job_get_sub_job_count(job)));

		mali_group_unlock(group);

		/* remove the return value from mali_group_start_xx_job, since we can't fail on Mali-300++ */
	}
}

static void mali_pp_scheduler_return_job_to_user(struct mali_pp_job *job, mali_bool deferred)
{
	if (MALI_FALSE == mali_pp_job_use_no_notification(job))
	{
		u32 i;
		u32 num_counters_to_copy;
		mali_bool success = mali_pp_job_was_success(job);

		_mali_uk_pp_job_finished_s *jobres = job->finished_notification->result_buffer;
		_mali_osk_memset(jobres, 0, sizeof(_mali_uk_pp_job_finished_s)); /* @@@@ can be removed once we initialize all members in this struct */
		jobres->user_job_ptr = mali_pp_job_get_user_id(job);
		if (MALI_TRUE == success)
		{
			jobres->status = _MALI_UK_JOB_STATUS_END_SUCCESS;
		}
		else
		{
			jobres->status = _MALI_UK_JOB_STATUS_END_UNKNOWN_ERR;
		}

		if (mali_pp_job_is_virtual(job))
		{
			num_counters_to_copy = num_cores; /* Number of physical cores available */
		}
		else
		{
			num_counters_to_copy = mali_pp_job_get_sub_job_count(job);
		}

		for (i = 0; i < num_counters_to_copy; i++)
		{
			jobres->perf_counter0[i] = mali_pp_job_get_perf_counter_value0(job, i);
			jobres->perf_counter1[i] = mali_pp_job_get_perf_counter_value1(job, i);
		}

		mali_session_send_notification(mali_pp_job_get_session(job), job->finished_notification);
		job->finished_notification = NULL;
	}

#if defined(MALI_PP_SCHEDULER_USE_DEFERRED_JOB_DELETE)
	if (MALI_TRUE == deferred)
	{
		/* The deletion of the job object (releasing sync refs etc) must be done in a different context */
		_mali_osk_lock_wait(pp_scheduler_job_delete_lock, _MALI_OSK_LOCKMODE_RW);

		MALI_DEBUG_ASSERT(_mali_osk_list_empty(&job->list)); /* This job object should not be on any list */
		_mali_osk_list_addtail(&job->list, &pp_scheduler_job_deletion_queue);

		_mali_osk_lock_signal(pp_scheduler_job_delete_lock, _MALI_OSK_LOCKMODE_RW);

		_mali_osk_wq_schedule_work(pp_scheduler_wq_job_delete);
	}
	else
	{
		mali_pp_job_delete(job);
	}
#else
	MALI_DEBUG_ASSERT(MALI_FALSE == deferred); /* no use cases need this in this configuration */
	mali_pp_job_delete(job);
#endif
}

static void mali_pp_scheduler_do_schedule(void *arg)
{
	MALI_IGNORE(arg);

	mali_pp_scheduler_schedule();
}

#if defined(MALI_PP_SCHEDULER_USE_DEFERRED_JOB_DELETE)
static void mali_pp_scheduler_do_job_delete(void *arg)
{
	_MALI_OSK_LIST_HEAD_STATIC_INIT(list);
	struct mali_pp_job *job;
	struct mali_pp_job *tmp;

	MALI_IGNORE(arg);

	_mali_osk_lock_wait(pp_scheduler_job_delete_lock, _MALI_OSK_LOCKMODE_RW);

	/*
	 * Quickly "unhook" the jobs pending to be deleted, so we can release the lock before
	 * we start deleting the job objects (without any locks held
	 */
	_mali_osk_list_move_list(&pp_scheduler_job_deletion_queue, &list);

	_mali_osk_lock_signal(pp_scheduler_job_delete_lock, _MALI_OSK_LOCKMODE_RW);

	_MALI_OSK_LIST_FOREACHENTRY(job, tmp, &list, struct mali_pp_job, list)
	{
		mali_pp_job_delete(job); /* delete the job object itself */
	}
}
#endif

void mali_pp_scheduler_job_done(struct mali_group *group, struct mali_pp_job *job, u32 sub_job, mali_bool success)
{
	mali_bool job_is_done;
	mali_bool barrier_enforced = MALI_FALSE;

	MALI_DEBUG_PRINT(3, ("Mali PP scheduler: %s job %u (0x%08X) part %u/%u completed (%s)\n",
	                     mali_pp_job_is_virtual(job) ? "Virtual" : "Physical",
	                     mali_pp_job_get_id(job),
	                     job, sub_job + 1,
	                     mali_pp_job_get_sub_job_count(job),
	                     success ? "success" : "failure"));
	MALI_ASSERT_GROUP_LOCKED(group);
	mali_pp_scheduler_lock();

	mali_pp_job_mark_sub_job_completed(job, success);

	MALI_DEBUG_ASSERT(mali_pp_job_is_virtual(job) == mali_group_is_virtual(group));

	job_is_done = mali_pp_job_is_complete(job);

	if (job_is_done)
	{
		struct mali_session_data *session = mali_pp_job_get_session(job);
		struct mali_pp_job *job_head;

		/* Remove job from session list */
		_mali_osk_list_del(&job->session_list);

		MALI_DEBUG_PRINT(4, ("Mali PP scheduler: All parts completed for %s job %u (0x%08X)\n",
		                     mali_pp_job_is_virtual(job) ? "virtual" : "physical",
		                     mali_pp_job_get_id(job), job));
#if defined(CONFIG_SYNC)
		if (job->sync_point)
		{
			int error;
			if (success) error = 0;
			else error = -EFAULT;
			MALI_DEBUG_PRINT(4, ("Sync: Signal %spoint for job %d\n",
			                     success ? "" : "failed ",
					     mali_pp_job_get_id(job)));
			mali_sync_signal_pt(job->sync_point, error);
		}
#endif

		/* Send notification back to user space */
#if defined(MALI_PP_SCHEDULER_USE_DEFERRED_JOB_DELETE)
		mali_pp_scheduler_return_job_to_user(job, MALI_TRUE);
#else
		mali_pp_scheduler_return_job_to_user(job, MALI_FALSE);
#endif

		mali_pp_scheduler_job_completed();

		/* Resolve any barriers */
		if (!_mali_osk_list_empty(&session->job_list))
		{
			job_head = _MALI_OSK_LIST_ENTRY(session->job_list.next, struct mali_pp_job, session_list);
			if (mali_pp_job_has_active_barrier(job_head))
			{
				barrier_enforced = MALI_TRUE;
				mali_pp_job_barrier_enforced(job_head);
			}
		}
	}

	/* If paused, then this was the last job, so wake up sleeping workers */
	if (pause_count > 0)
	{
		/* Wake up sleeping workers. Their wake-up condition is that
		 * num_slots == num_slots_idle, so unless we are done working, no
		 * threads will actually be woken up.
		 */
		if (mali_group_is_virtual(group))
		{
			virtual_group_state = VIRTUAL_GROUP_IDLE;
		}
		else
		{
			_mali_osk_list_move(&(group->pp_scheduler_list), &group_list_idle);
		}
#if defined(CONFIG_GPU_TRACEPOINTS) && defined(CONFIG_TRACEPOINTS)
			trace_gpu_sched_switch(mali_pp_get_hw_core_desc(group->pp_core), sched_clock(), 0, 0, 0);
#endif
		_mali_osk_wait_queue_wake_up(pp_scheduler_working_wait_queue);
		mali_pp_scheduler_unlock();
		return;
	}

	if (barrier_enforced)
	{
		/* A barrier was resolved, so schedule previously blocked jobs */
		_mali_osk_wq_schedule_work(pp_scheduler_wq_schedule);
	}

	/* Recycle variables */
	job = NULL;
	sub_job = 0;

	if (mali_group_is_virtual(group))
	{
		/* Virtual group */

		/* Now that the virtual group is idle, check if we should reconfigure */
		struct mali_pp_job *physical_job = NULL;
		struct mali_group *physical_group = NULL;

		/* Obey the policy */
		virtual_group_state = VIRTUAL_GROUP_IDLE;

		if (mali_pp_scheduler_can_move_virtual_to_physical())
		{
			/* There is a runnable physical job and we can acquire a physical group */
			physical_job = mali_pp_scheduler_get_physical_job();
			MALI_DEBUG_ASSERT(mali_pp_job_has_unstarted_sub_jobs(physical_job));

			/* Mark subjob as started */
			sub_job = mali_pp_job_get_first_unstarted_sub_job(physical_job);
			mali_pp_job_mark_sub_job_started(physical_job, sub_job);

			/* Remove job from queue (if we now got the last subjob) */
			mali_pp_scheduler_dequeue_physical_job(physical_job);

			/* Acquire a physical group from the virtual group
			 * It's state will be LEAVING_VIRTUAL and must be set to IDLE before it can be used */
			physical_group = mali_group_acquire_group(virtual_group);

			/* Move physical group to the working list, as we will soon start a job on it */
			_mali_osk_list_move(&(physical_group->pp_scheduler_list), &group_list_working);

			mali_pp_scheduler_disable_empty_virtual();
		}

		/* Start the next virtual job */
		job = mali_pp_scheduler_get_virtual_job();
		if (NULL != job && VIRTUAL_GROUP_IDLE == virtual_group_state)
		{
			/* There is a runnable virtual job */
			MALI_DEBUG_ASSERT(mali_pp_job_is_virtual(job));
			MALI_DEBUG_ASSERT(mali_pp_job_has_unstarted_sub_jobs(job));
			MALI_DEBUG_ASSERT(1 == mali_pp_job_get_sub_job_count(job));

			mali_pp_job_mark_sub_job_started(job, 0);

			/* Remove job from queue */
			mali_pp_scheduler_dequeue_virtual_job(job);

			/* Virtual group is now working */
			virtual_group_state = VIRTUAL_GROUP_WORKING;

			mali_pp_scheduler_unlock();

			/* Start job */
			mali_group_start_pp_job(group, job, 0);
			MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Virtual job %u (0x%08X) part %u/%u started (from job_done)\n",
			                     mali_pp_job_get_id(job), job, 1,
			                     mali_pp_job_get_sub_job_count(job)));
		}
		else
		{
#if defined(CONFIG_GPU_TRACEPOINTS) && defined(CONFIG_TRACEPOINTS)
			trace_gpu_sched_switch("Mali_Virtual_PP", sched_clock(), 0, 0, 0);
#endif
			mali_pp_scheduler_unlock();
		}

		/* Start a physical job (if we acquired a physical group earlier) */
		if (NULL != physical_job && NULL != physical_group)
		{
			mali_group_lock(physical_group);

			/* Set the group state from LEAVING_VIRTUAL to IDLE to complete the transition */
			physical_group->state = MALI_GROUP_STATE_IDLE;

			/* Start job on core */
			mali_group_start_pp_job(physical_group, physical_job, sub_job);
			MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Physical job %u (0x%08X) part %u/%u started (from job_done)\n",
			                     mali_pp_job_get_id(physical_job), physical_job, sub_job + 1,
			                     mali_pp_job_get_sub_job_count(physical_job)));

			mali_group_unlock(physical_group);
		}
	}
	else
	{
		/* Physical group */
		job = mali_pp_scheduler_get_physical_job();
		if (NULL != job)
		{
			/* There is a runnable physical job */
			MALI_DEBUG_ASSERT(mali_pp_job_has_unstarted_sub_jobs(job));

			/* Mark subjob as started */
			sub_job = mali_pp_job_get_first_unstarted_sub_job(job);
			mali_pp_job_mark_sub_job_started(job, sub_job);

			/* Remove job from queue (if we now got the last subjob) */
			mali_pp_scheduler_dequeue_physical_job(job);

			mali_pp_scheduler_unlock();

			/* Group is already on the working list, so start the job */
			mali_group_start_pp_job(group, job, sub_job);
			MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Physical job %u (0x%08X) part %u/%u started (from job_done)\n",
			                     mali_pp_job_get_id(job), job, sub_job + 1,
			                     mali_pp_job_get_sub_job_count(job)));
		}
		else if (mali_pp_scheduler_has_virtual_group())
		{
			/* Rejoin virtual group */
			/* In the future, a policy check might be useful here */

			/* We're no longer needed on the scheduler list */
			_mali_osk_list_delinit(&(group->pp_scheduler_list));

			/* Make sure no interrupts are handled for this group during
			 * the transition from physical to virtual */
			group->state = MALI_GROUP_STATE_JOINING_VIRTUAL;

			mali_pp_scheduler_unlock();
			mali_group_unlock(group);

			mali_group_lock(virtual_group);

			if (mali_pp_scheduler_has_virtual_group())
			{
				mali_pp_scheduler_enable_empty_virtual();
			}

			/* We need to recheck the group state since it is possible that someone has
			 * modified the group before we locked the virtual group. */
			if (MALI_GROUP_STATE_JOINING_VIRTUAL == group->state)
			{
				mali_group_add_group(virtual_group, group, MALI_TRUE);
			}

			mali_group_unlock(virtual_group);

			if (mali_pp_scheduler_has_virtual_group() && VIRTUAL_GROUP_IDLE == virtual_group_state)
			{
				_mali_osk_wq_schedule_work(pp_scheduler_wq_schedule);
			}

			/* We need to return from this function with the group lock held */
			mali_group_lock(group);
		}
		else
		{
			_mali_osk_list_move(&(group->pp_scheduler_list), &group_list_idle);
#if defined(CONFIG_GPU_TRACEPOINTS) && defined(CONFIG_TRACEPOINTS)
			trace_gpu_sched_switch(mali_pp_get_hw_core_desc(group->pp_core), sched_clock(), 0, 0, 0);
#endif
			mali_pp_scheduler_unlock();
		}
	}
}

void mali_pp_scheduler_suspend(void)
{
	mali_pp_scheduler_lock();
	pause_count++; /* Increment the pause_count so that no more jobs will be scheduled */
	mali_pp_scheduler_unlock();

	/* Go to sleep. When woken up again (in mali_pp_scheduler_job_done), the
	 * mali_pp_scheduler_suspended() function will be called. This will return true
	 * iff state is idle and pause_count > 0, so if the core is active this
	 * will not do anything.
	 */
	_mali_osk_wait_queue_wait_event(pp_scheduler_working_wait_queue, mali_pp_scheduler_is_suspended);
}

void mali_pp_scheduler_resume(void)
{
	mali_pp_scheduler_lock();
	pause_count--; /* Decrement pause_count to allow scheduling again (if it reaches 0) */
	mali_pp_scheduler_unlock();
	if (0 == pause_count)
	{
		mali_pp_scheduler_schedule();
	}
}

MALI_STATIC_INLINE void mali_pp_scheduler_queue_job(struct mali_pp_job *job, struct mali_session_data *session)
{
	MALI_DEBUG_ASSERT_POINTER(job);

#if defined(CONFIG_GPU_TRACEPOINTS) && defined(CONFIG_TRACEPOINTS)
	trace_gpu_job_enqueue(mali_pp_job_get_tid(job), mali_pp_job_get_id(job), "PP");
#endif

#if defined(CONFIG_DMA_SHARED_BUFFER) && !defined(CONFIG_MALI_DMA_BUF_MAP_ON_ATTACH)
	/* Map buffers attached to job */
	if (0 != job->num_memory_cookies)
	{
		mali_dma_buf_map_job(job);
	}
#endif /* CONFIG_DMA_SHARED_BUFFER */

	mali_pp_scheduler_job_queued();

	mali_pp_scheduler_lock();

	if (mali_pp_job_is_virtual(job))
	{
		/* Virtual job */
		virtual_job_queue_depth += 1;
		_mali_osk_list_addtail(&job->list, &virtual_job_queue);
	}
	else
	{
		job_queue_depth += mali_pp_job_get_sub_job_count(job);
		_mali_osk_list_addtail(&job->list, &job_queue);
	}

	if (mali_pp_job_has_active_barrier(job) && _mali_osk_list_empty(&session->job_list))
	{
		/* No running jobs on this session, so barrier condition already met */
		mali_pp_job_barrier_enforced(job);
	}

	/* Add job to session list */
	_mali_osk_list_addtail(&job->session_list, &session->job_list);

	MALI_DEBUG_PRINT(3, ("Mali PP scheduler: %s job %u (0x%08X) with %u parts queued\n",
	                     mali_pp_job_is_virtual(job) ? "Virtual" : "Physical",
	                     mali_pp_job_get_id(job), job, mali_pp_job_get_sub_job_count(job)));

	mali_pp_scheduler_unlock();
}

#if defined(CONFIG_SYNC)
static void sync_callback(struct sync_fence *fence, struct sync_fence_waiter *waiter)
{
	struct mali_pp_job *job = _MALI_OSK_CONTAINER_OF(waiter, struct mali_pp_job, sync_waiter);

	/* Schedule sync_callback_work */
	_mali_osk_wq_schedule_work(job->sync_work);
}

static void sync_callback_work(void *arg)
{
	struct mali_pp_job *job = (struct mali_pp_job *)arg;
	struct mali_session_data *session;
	int err;

	MALI_DEBUG_ASSERT_POINTER(job);

	session = job->session;

	/* Remove job from session pending job list */
	_mali_osk_lock_wait(session->pending_jobs_lock, _MALI_OSK_LOCKMODE_RW);
	_mali_osk_list_delinit(&job->list);
	_mali_osk_lock_signal(session->pending_jobs_lock, _MALI_OSK_LOCKMODE_RW);

	err = sync_fence_wait(job->pre_fence, 0);
	if (likely(0 == err))
	{
		MALI_DEBUG_PRINT(3, ("Mali sync: Job %d ready to run\n", mali_pp_job_get_id(job)));

		mali_pp_scheduler_queue_job(job, session);

		mali_pp_scheduler_schedule();
	}
	else
	{
		/* Fence signaled error */
		MALI_DEBUG_PRINT(3, ("Mali sync: Job %d abort due to sync error\n", mali_pp_job_get_id(job)));

		if (job->sync_point) mali_sync_signal_pt(job->sync_point, err);

		mali_pp_job_mark_sub_job_completed(job, MALI_FALSE); /* Flagging the job as failed. */
		mali_pp_scheduler_return_job_to_user(job, MALI_FALSE); /* This will also delete the job object */
	}
}
#endif

_mali_osk_errcode_t _mali_ukk_pp_start_job(void *ctx, _mali_uk_pp_start_job_s *uargs, int *fence)
{
	struct mali_session_data *session;
	struct mali_pp_job *job;

	MALI_DEBUG_ASSERT_POINTER(uargs);
	MALI_DEBUG_ASSERT_POINTER(ctx);

	session = (struct mali_session_data*)ctx;

	job = mali_pp_job_create(session, uargs, mali_scheduler_get_new_id());
	if (NULL == job)
	{
		MALI_PRINT_ERROR(("Failed to create job!\n"));
		return _MALI_OSK_ERR_NOMEM;
	}

	if (_MALI_OSK_ERR_OK != mali_pp_job_check(job))
	{
		/* Not a valid job, return to user immediately */
		mali_pp_job_mark_sub_job_completed(job, MALI_FALSE); /* Flagging the job as failed. */
		mali_pp_scheduler_return_job_to_user(job, MALI_FALSE); /* This will also delete the job object */
		return _MALI_OSK_ERR_OK; /* User is notified via a notification, so this call is ok */
	}

#if PROFILING_SKIP_PP_JOBS || PROFILING_SKIP_PP_AND_GP_JOBS
#warning PP jobs will not be executed
	mali_pp_scheduler_return_job_to_user(job, MALI_FALSE);
	return _MALI_OSK_ERR_OK;
#endif

#if defined(CONFIG_SYNC)
	if (_MALI_PP_JOB_FLAG_FENCE & job->uargs.flags)
	{
		int post_fence = -1;

		job->sync_point = mali_stream_create_point(job->uargs.stream);

		if (unlikely(NULL == job->sync_point))
		{
			/* Fence creation failed. */
			MALI_DEBUG_PRINT(2, ("Failed to create sync point for job %d\n", mali_pp_job_get_id(job)));
			mali_pp_job_mark_sub_job_completed(job, MALI_FALSE); /* Flagging the job as failed. */
			mali_pp_scheduler_return_job_to_user(job, MALI_FALSE); /* This will also delete the job object */
			return _MALI_OSK_ERR_OK; /* User is notified via a notification, so this call is ok */
		}

		post_fence = mali_stream_create_fence(job->sync_point);

		if (unlikely(0 > post_fence))
		{
			/* Fence creation failed. */
			/* mali_stream_create_fence already freed the sync_point */
			MALI_DEBUG_PRINT(2, ("Failed to create fence for job %d\n", mali_pp_job_get_id(job)));
			mali_pp_job_mark_sub_job_completed(job, MALI_FALSE); /* Flagging the job as failed. */
			mali_pp_scheduler_return_job_to_user(job, MALI_FALSE); /* This will also delete the job object */
			return _MALI_OSK_ERR_OK; /* User is notified via a notification, so this call is ok */
		}

		/* Grab a reference to the fence. It must be around when the
		 * job is completed, so the point can be signalled. */
		sync_fence_fdget(post_fence);

		*fence = post_fence;

		MALI_DEBUG_PRINT(3, ("Sync: Created fence %d for job %d\n", post_fence, mali_pp_job_get_id(job)));
	}
	else if (_MALI_PP_JOB_FLAG_EMPTY_FENCE & job->uargs.flags)
	{
		int empty_fence_fd = job->uargs.stream;
		struct sync_fence *empty_fence;
		struct sync_pt *pt;
		int ret;

		/* Grab and keep a reference to the fence. It must be around
		 * when the job is completed, so the point can be signalled. */
		empty_fence = sync_fence_fdget(empty_fence_fd);

		if (unlikely(NULL == empty_fence))
		{
			MALI_DEBUG_PRINT_ERROR(("Failed to accept empty fence: %d\n", empty_fence_fd));
			mali_pp_job_mark_sub_job_completed(job, MALI_FALSE); /* Flagging the job as failed. */
			mali_pp_scheduler_return_job_to_user(job, MALI_FALSE); /* This will also delete the job object */
			return _MALI_OSK_ERR_OK;
		}

		if (unlikely(list_empty(&empty_fence->pt_list_head)))
		{
			MALI_DEBUG_PRINT_ERROR(("Failed to accept empty fence: %d\n", empty_fence_fd));
			sync_fence_put(empty_fence);
			mali_pp_job_mark_sub_job_completed(job, MALI_FALSE); /* Flagging the job as failed. */
			mali_pp_scheduler_return_job_to_user(job, MALI_FALSE); /* This will also delete the job object */
			return _MALI_OSK_ERR_OK;
		}

		pt = list_first_entry(&empty_fence->pt_list_head, struct sync_pt, pt_list);

		ret = mali_sync_timed_commit(pt);

		if (unlikely(0 != ret))
		{
			MALI_DEBUG_PRINT_ERROR(("Empty fence not valid: %d\n", empty_fence_fd));
			sync_fence_put(empty_fence);
			mali_pp_job_mark_sub_job_completed(job, MALI_FALSE); /* Flagging the job as failed. */
			mali_pp_scheduler_return_job_to_user(job, MALI_FALSE); /* This will also delete the job object */
			return _MALI_OSK_ERR_OK;
		}

		job->sync_point = pt;

		*fence = empty_fence_fd;

		MALI_DEBUG_PRINT(3, ("Sync: Job %d now backs fence %d\n", mali_pp_job_get_id(job), empty_fence_fd));
	}

	if (0 < job->uargs.fence)
	{
		int pre_fence_fd = job->uargs.fence;
		int err;

		MALI_DEBUG_PRINT(3, ("Sync: Job %d waiting for fence %d\n", mali_pp_job_get_id(job), pre_fence_fd));

		job->pre_fence = sync_fence_fdget(pre_fence_fd); /* Reference will be released when job is deleted. */
		if (NULL == job->pre_fence)
		{
			MALI_DEBUG_PRINT(2, ("Failed to import fence %d\n", pre_fence_fd));
			if (job->sync_point) mali_sync_signal_pt(job->sync_point, -EINVAL);
			mali_pp_job_mark_sub_job_completed(job, MALI_FALSE); /* Flagging the job as failed. */
			mali_pp_scheduler_return_job_to_user(job, MALI_FALSE); /* This will also delete the job object */
			return _MALI_OSK_ERR_OK; /* User is notified via a notification, so this call is ok */
		}

		job->sync_work = _mali_osk_wq_create_work(sync_callback_work, (void*)job);
		if (NULL == job->sync_work)
		{
			if (job->sync_point) mali_sync_signal_pt(job->sync_point, -ENOMEM);
			mali_pp_job_mark_sub_job_completed(job, MALI_FALSE); /* Flagging the job as failed. */
			mali_pp_scheduler_return_job_to_user(job, MALI_FALSE); /* This will also delete the job object */
			return _MALI_OSK_ERR_OK; /* User is notified via a notification, so this call is ok */
		}

		/* Add pending job to session pending job list */
		_mali_osk_lock_wait(session->pending_jobs_lock, _MALI_OSK_LOCKMODE_RW);
		_mali_osk_list_addtail(&job->list, &session->pending_jobs);
		_mali_osk_lock_signal(session->pending_jobs_lock, _MALI_OSK_LOCKMODE_RW);

		sync_fence_waiter_init(&job->sync_waiter, sync_callback);
		err = sync_fence_wait_async(job->pre_fence, &job->sync_waiter);

		if (0 != err)
		{
			/* No async wait started, remove job from session pending job list */
			_mali_osk_lock_wait(session->pending_jobs_lock, _MALI_OSK_LOCKMODE_RW);
			_mali_osk_list_delinit(&job->list);
			_mali_osk_lock_signal(session->pending_jobs_lock, _MALI_OSK_LOCKMODE_RW);
		}

		if (1 == err)
		{
			/* Fence has already signalled */
			mali_pp_scheduler_queue_job(job, session);
			if (!_mali_osk_list_empty(&group_list_idle) || VIRTUAL_GROUP_IDLE == virtual_group_state)
			{
				mali_pp_scheduler_schedule();
			}
			return _MALI_OSK_ERR_OK;
		}
		else if (0 > err)
		{
			/* Sync fail */
			if (job->sync_point) mali_sync_signal_pt(job->sync_point, err);
			mali_pp_job_mark_sub_job_completed(job, MALI_FALSE); /* Flagging the job as failed. */
			mali_pp_scheduler_return_job_to_user(job, MALI_FALSE); /* This will also delete the job object */
			return _MALI_OSK_ERR_OK; /* User is notified via a notification, so this call is ok */
		}

	}
	else
#endif /* CONFIG_SYNC */
	{
		mali_pp_scheduler_queue_job(job, session);

		if (!_mali_osk_list_empty(&group_list_idle) || VIRTUAL_GROUP_IDLE == virtual_group_state)
		{
			mali_pp_scheduler_schedule();
		}
	}

	return _MALI_OSK_ERR_OK;
}

_mali_osk_errcode_t _mali_ukk_get_pp_number_of_cores(_mali_uk_get_pp_number_of_cores_s *args)
{
	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_DEBUG_ASSERT_POINTER(args->ctx);
	args->number_of_total_cores = num_cores;
	args->number_of_enabled_cores = enabled_cores;
	return _MALI_OSK_ERR_OK;
}

u32 mali_pp_scheduler_get_num_cores_total(void)
{
	return num_cores;
}

u32 mali_pp_scheduler_get_num_cores_enabled(void)
{
	return enabled_cores;
}

_mali_osk_errcode_t _mali_ukk_get_pp_core_version(_mali_uk_get_pp_core_version_s *args)
{
	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_DEBUG_ASSERT_POINTER(args->ctx);
	args->version = pp_version;
	return _MALI_OSK_ERR_OK;
}

void _mali_ukk_pp_job_disable_wb(_mali_uk_pp_disable_wb_s *args)
{
	struct mali_session_data *session;
	struct mali_pp_job *job;
	struct mali_pp_job *tmp;

	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_DEBUG_ASSERT_POINTER(args->ctx);

	session = (struct mali_session_data*)args->ctx;

	/* Check queue for jobs that match */
	mali_pp_scheduler_lock();
	_MALI_OSK_LIST_FOREACHENTRY(job, tmp, &job_queue, struct mali_pp_job, list)
	{
		if (mali_pp_job_get_session(job) == session &&
		    mali_pp_job_get_frame_builder_id(job) == (u32)args->fb_id &&
		    mali_pp_job_get_flush_id(job) == (u32)args->flush_id)
		{
			if (args->wbx & _MALI_UK_PP_JOB_WB0)
			{
				mali_pp_job_disable_wb0(job);
			}
			if (args->wbx & _MALI_UK_PP_JOB_WB1)
			{
				mali_pp_job_disable_wb1(job);
			}
			if (args->wbx & _MALI_UK_PP_JOB_WB2)
			{
				mali_pp_job_disable_wb2(job);
			}
			break;
		}
	}
	mali_pp_scheduler_unlock();
}

void mali_pp_scheduler_abort_session(struct mali_session_data *session)
{
	struct mali_pp_job *job, *tmp_job;
	struct mali_group *group, *tmp_group;
	struct mali_group *groups[MALI_MAX_NUMBER_OF_GROUPS];
	s32 i = 0;
#if defined(MALI_PP_SCHEDULER_USE_DEFERRED_JOB_DELETE)
	_MALI_OSK_LIST_HEAD_STATIC_INIT(deferred_deletion_list);
#endif

	mali_pp_scheduler_lock();
	MALI_DEBUG_PRINT(3, ("Mali PP scheduler: Aborting all jobs from session 0x%08x\n", session));

	_MALI_OSK_LIST_FOREACHENTRY(job, tmp_job, &session->job_list, struct mali_pp_job, session_list)
	{
		/* Remove job from queue (if it's not queued, list_del has no effect) */
		_mali_osk_list_delinit(&job->list);

		if (mali_pp_job_is_virtual(job))
		{
			MALI_DEBUG_ASSERT(1 == mali_pp_job_get_sub_job_count(job));
			if (0 == mali_pp_job_get_first_unstarted_sub_job(job))
			{
				--virtual_job_queue_depth;
			}
		}
		else
		{
			job_queue_depth -= mali_pp_job_get_sub_job_count(job) - mali_pp_job_get_first_unstarted_sub_job(job);
		}

		/* Mark all unstarted jobs as failed */
		mali_pp_job_mark_unstarted_failed(job);

		if (mali_pp_job_is_complete(job))
		{
			_mali_osk_list_del(&job->session_list);

			/* It is safe to delete the job, since it won't land in job_done() */
			MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Aborted PP job 0x%08x\n", job));

#if defined(MALI_PP_SCHEDULER_USE_DEFERRED_JOB_DELETE)
			MALI_DEBUG_ASSERT(_mali_osk_list_empty(&job->list));
			_mali_osk_list_addtail(&job->list, &deferred_deletion_list);
#else
			mali_pp_job_delete(job);
#endif

			mali_pp_scheduler_job_completed();
		}
		else
		{
			MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Keeping partially started PP job 0x%08x in session\n", job));
		}
	}

	_MALI_OSK_LIST_FOREACHENTRY(group, tmp_group, &group_list_working, struct mali_group, pp_scheduler_list)
	{
		groups[i++] = group;
	}

	_MALI_OSK_LIST_FOREACHENTRY(group, tmp_group, &group_list_idle, struct mali_group, pp_scheduler_list)
	{
		groups[i++] = group;
	}

	mali_pp_scheduler_unlock();

#if defined(MALI_PP_SCHEDULER_USE_DEFERRED_JOB_DELETE)
	_MALI_OSK_LIST_FOREACHENTRY(job, tmp_job, &deferred_deletion_list, struct mali_pp_job, list)
	{
		mali_pp_job_delete(job);
	}
#endif

	/* Abort running jobs from this session */
	while (i > 0)
	{
		mali_group_abort_session(groups[--i], session);
	}

	if (mali_pp_scheduler_has_virtual_group())
	{
		mali_group_abort_session(virtual_group, session);
	}
}

static mali_bool mali_pp_scheduler_is_suspended(void)
{
	mali_bool ret;

	mali_pp_scheduler_lock();

	ret = pause_count > 0
	      && _mali_osk_list_empty(&group_list_working)
	      && VIRTUAL_GROUP_WORKING != virtual_group_state;

	mali_pp_scheduler_unlock();

	return ret;
}

int mali_pp_scheduler_get_queue_depth(void)
{
	return job_queue_depth;
}

#if MALI_STATE_TRACKING
u32 mali_pp_scheduler_dump_state(char *buf, u32 size)
{
	int n = 0;
	struct mali_group *group;
	struct mali_group *temp;

	n += _mali_osk_snprintf(buf + n, size - n, "PP:\n");
	n += _mali_osk_snprintf(buf + n, size - n, "\tQueue is %s\n", _mali_osk_list_empty(&job_queue) ? "empty" : "not empty");
	n += _mali_osk_snprintf(buf + n, size - n, "\n");

	_MALI_OSK_LIST_FOREACHENTRY(group, temp, &group_list_working, struct mali_group, pp_scheduler_list)
	{
		n += mali_group_dump_state(group, buf + n, size - n);
	}

	_MALI_OSK_LIST_FOREACHENTRY(group, temp, &group_list_idle, struct mali_group, pp_scheduler_list)
	{
		n += mali_group_dump_state(group, buf + n, size - n);
	}

	_MALI_OSK_LIST_FOREACHENTRY(group, temp, &group_list_disabled, struct mali_group, pp_scheduler_list)
	{
		n += mali_group_dump_state(group, buf + n, size - n);
	}

	if (mali_pp_scheduler_has_virtual_group())
	{
		n += mali_group_dump_state(virtual_group, buf + n, size -n);
	}

	n += _mali_osk_snprintf(buf + n, size - n, "\n");
	return n;
}
#endif

/* This function is intended for power on reset of all cores.
 * No locking is done for the list iteration, which can only be safe if the
 * scheduler is paused and all cores idle. That is always the case on init and
 * power on. */
void mali_pp_scheduler_reset_all_groups(void)
{
	struct mali_group *group, *temp;
	struct mali_group *groups[MALI_MAX_NUMBER_OF_GROUPS];
	s32 i = 0;

	if (mali_pp_scheduler_has_virtual_group())
	{
		mali_group_lock(virtual_group);
		mali_group_reset(virtual_group);
		mali_group_unlock(virtual_group);
	}

	MALI_DEBUG_ASSERT(_mali_osk_list_empty(&group_list_working));
	MALI_DEBUG_ASSERT(VIRTUAL_GROUP_WORKING != virtual_group_state);
	mali_pp_scheduler_lock();
	_MALI_OSK_LIST_FOREACHENTRY(group, temp, &group_list_idle, struct mali_group, pp_scheduler_list)
	{
		groups[i++] = group;
	}
	mali_pp_scheduler_unlock();

	while (i > 0)
	{
		group = groups[--i];

		mali_group_lock(group);
		mali_group_reset(group);
		mali_group_unlock(group);
	}
}

void mali_pp_scheduler_zap_all_active(struct mali_session_data *session)
{
	struct mali_group *group, *temp;
	struct mali_group *groups[MALI_MAX_NUMBER_OF_GROUPS];
	s32 i = 0;

	if (mali_pp_scheduler_has_virtual_group())
	{
		mali_group_zap_session(virtual_group, session);
	}

	mali_pp_scheduler_lock();
	_MALI_OSK_LIST_FOREACHENTRY(group, temp, &group_list_working, struct mali_group, pp_scheduler_list)
	{
		groups[i++] = group;
	}
	mali_pp_scheduler_unlock();

	while (i > 0)
	{
		mali_group_zap_session(groups[--i], session);
	}
}

/* A pm reference must be taken with _mali_osk_pm_dev_ref_add_no_power_on
 * before calling this function to avoid Mali powering down as HW is accessed.
 */
static void mali_pp_scheduler_enable_group_internal(struct mali_group *group)
{
	MALI_DEBUG_ASSERT_POINTER(group);

	mali_group_lock(group);

	if (MALI_GROUP_STATE_DISABLED != group->state)
	{
		mali_group_unlock(group);
		MALI_DEBUG_PRINT(4, ("Mali PP scheduler: PP group %p already enabled\n", group));
		return;
	}

	MALI_DEBUG_PRINT(3, ("Mali PP scheduler: Enabling PP group %p\n", group));

	mali_pp_scheduler_lock();

	MALI_DEBUG_ASSERT(MALI_GROUP_STATE_DISABLED == group->state);
	++enabled_cores;

	if (mali_pp_scheduler_has_virtual_group())
	{
		mali_bool update_hw;

		/* Add group to virtual group */
		_mali_osk_list_delinit(&(group->pp_scheduler_list));
		group->state = MALI_GROUP_STATE_JOINING_VIRTUAL;

		mali_pp_scheduler_unlock();
		mali_group_unlock(group);

		mali_group_lock(virtual_group);

		update_hw = mali_pm_is_power_on();

		mali_pm_domain_ref_get(group->pm_domain);
		MALI_DEBUG_ASSERT(NULL == group->pm_domain ||
		                  MALI_PM_DOMAIN_ON == mali_pm_domain_state_get(group->pm_domain));

		if (update_hw)
		{
			mali_group_lock(group);
			mali_group_power_on_group(group);
			mali_group_reset(group);
			mali_group_unlock(group);
		}

		mali_pp_scheduler_enable_empty_virtual();
		mali_group_add_group(virtual_group, group, update_hw);
		MALI_DEBUG_PRINT(4, ("Done enabling group %p. Added to virtual group.\n", group));

		mali_group_unlock(virtual_group);
	}
	else
	{
		mali_pm_domain_ref_get(group->pm_domain);
		MALI_DEBUG_ASSERT(NULL == group->pm_domain ||
		                  MALI_PM_DOMAIN_ON == mali_pm_domain_state_get(group->pm_domain));

		/* Put group on idle list */
		if (mali_pm_is_power_on())
		{
			mali_group_power_on_group(group);
			mali_group_reset(group);
		}

		_mali_osk_list_move(&(group->pp_scheduler_list), &group_list_idle);
		group->state = MALI_GROUP_STATE_IDLE;

		MALI_DEBUG_PRINT(4, ("Done enabling group %p. Now on idle list.\n", group));
		mali_pp_scheduler_unlock();
		mali_group_unlock(group);
	}
}

void mali_pp_scheduler_enable_group(struct mali_group *group)
{
	MALI_DEBUG_ASSERT_POINTER(group);

	_mali_osk_pm_dev_ref_add_no_power_on();

	mali_pp_scheduler_enable_group_internal(group);

	_mali_osk_pm_dev_ref_dec_no_power_on();

	/* Pick up any jobs that might have been queued if all PP groups were disabled. */
	mali_pp_scheduler_schedule();
}

static void mali_pp_scheduler_disable_group_internal(struct mali_group *group)
{
	if (mali_pp_scheduler_has_virtual_group())
	{
		mali_group_lock(virtual_group);

		MALI_DEBUG_ASSERT(VIRTUAL_GROUP_WORKING != virtual_group_state);
		if (MALI_GROUP_STATE_JOINING_VIRTUAL == group->state)
		{
			/* The group was in the process of being added to the virtual group.  We
			 * only need to change the state to reverse this. */
			group->state = MALI_GROUP_STATE_LEAVING_VIRTUAL;
		}
		else if (MALI_GROUP_STATE_IN_VIRTUAL == group->state)
		{
			/* Remove group from virtual group.  The state of the group will be
			 * LEAVING_VIRTUAL and the group will not be on any scheduler list. */
			mali_group_remove_group(virtual_group, group);

			mali_pp_scheduler_disable_empty_virtual();
		}

		mali_group_unlock(virtual_group);
	}

	mali_group_lock(group);
	mali_pp_scheduler_lock();

	MALI_DEBUG_ASSERT(   MALI_GROUP_STATE_IDLE            == group->state
	                  || MALI_GROUP_STATE_LEAVING_VIRTUAL == group->state
	                  || MALI_GROUP_STATE_DISABLED        == group->state);

	if (MALI_GROUP_STATE_DISABLED == group->state)
	{
		MALI_DEBUG_PRINT(4, ("Mali PP scheduler: PP group %p already disabled\n", group));
	}
	else
	{
		MALI_DEBUG_PRINT(3, ("Mali PP scheduler: Disabling PP group %p\n", group));

		--enabled_cores;
		_mali_osk_list_move(&(group->pp_scheduler_list), &group_list_disabled);
		group->state = MALI_GROUP_STATE_DISABLED;

		mali_group_power_off_group(group);
		mali_pm_domain_ref_put(group->pm_domain);
	}

	mali_pp_scheduler_unlock();
	mali_group_unlock(group);
}

void mali_pp_scheduler_disable_group(struct mali_group *group)
{
	MALI_DEBUG_ASSERT_POINTER(group);

	mali_pp_scheduler_suspend();
	_mali_osk_pm_dev_ref_add_no_power_on();

	mali_pp_scheduler_disable_group_internal(group);

	_mali_osk_pm_dev_ref_dec_no_power_on();
	mali_pp_scheduler_resume();
}


static void mali_pp_scheduler_notify_core_change(u32 num_cores)
{
	if (!mali_is_mali450())
	{
		/* Notify all user space sessions about the change, so number of master tile lists can be adapter */
		struct mali_session_data *session, *tmp;

		mali_session_lock();
		MALI_SESSION_FOREACH(session, tmp, link)
		{
			_mali_osk_notification_t *notobj = _mali_osk_notification_create(_MALI_NOTIFICATION_PP_NUM_CORE_CHANGE, sizeof(_mali_uk_pp_num_cores_changed_s));
			if (NULL != notobj)
			{
				_mali_uk_pp_num_cores_changed_s *data = notobj->result_buffer;
				data->number_of_enabled_cores = num_cores;
				mali_session_send_notification(session, notobj);
			}
			else
			{
				MALI_PRINT_ERROR(("Failed to notify user space session about num PP core change\n"));
			}
		}
		mali_session_unlock();
	}
}

static void mali_pp_scheduler_set_perf_level_mali400(u32 target_core_nr)
{
	struct mali_group *group;
	MALI_DEBUG_ASSERT(mali_is_mali400());

	if (target_core_nr > enabled_cores)
	{
		MALI_DEBUG_PRINT(2, ("Requesting %d cores: enabling %d cores\n", target_core_nr, target_core_nr - enabled_cores));

		_mali_osk_pm_dev_ref_add_no_power_on();
		_mali_osk_pm_dev_barrier();

		while (target_core_nr > enabled_cores)
		{
			mali_pp_scheduler_lock();

			MALI_DEBUG_ASSERT(!_mali_osk_list_empty(&group_list_disabled));

			group = _MALI_OSK_LIST_ENTRY(group_list_disabled.next, struct mali_group, pp_scheduler_list);

			MALI_DEBUG_ASSERT_POINTER(group);
			MALI_DEBUG_ASSERT(MALI_GROUP_STATE_DISABLED == group->state);

			mali_pp_scheduler_unlock();

			mali_pp_scheduler_enable_group_internal(group);
		}

		_mali_osk_pm_dev_ref_dec_no_power_on();

		mali_pp_scheduler_schedule();
	}
	else if (target_core_nr < enabled_cores)
	{
		MALI_DEBUG_PRINT(2, ("Requesting %d cores: disabling %d cores\n", target_core_nr, enabled_cores - target_core_nr));

		mali_pp_scheduler_suspend();

		MALI_DEBUG_ASSERT(_mali_osk_list_empty(&group_list_working));

		while (target_core_nr < enabled_cores)
		{
			mali_pp_scheduler_lock();

			MALI_DEBUG_ASSERT(!_mali_osk_list_empty(&group_list_idle));
			MALI_DEBUG_ASSERT(_mali_osk_list_empty(&group_list_working));

			group = _MALI_OSK_LIST_ENTRY(group_list_idle.next, struct mali_group, pp_scheduler_list);

			MALI_DEBUG_ASSERT_POINTER(group);
			MALI_DEBUG_ASSERT(   MALI_GROUP_STATE_IDLE            == group->state
					  || MALI_GROUP_STATE_LEAVING_VIRTUAL == group->state
					  || MALI_GROUP_STATE_DISABLED        == group->state);

			mali_pp_scheduler_unlock();

			mali_pp_scheduler_disable_group_internal(group);
		}

		mali_pp_scheduler_resume();
	}

	mali_pp_scheduler_notify_core_change(target_core_nr);
}

static void mali_pp_scheduler_set_perf_level_mali450(u32 target_core_nr)
{
	struct mali_group *group;
	MALI_DEBUG_ASSERT(mali_is_mali450());

	if (target_core_nr > enabled_cores)
	{
		/* Enable some cores */
		struct mali_pm_domain *domain;

		MALI_DEBUG_PRINT(2, ("Requesting %d cores: enabling %d cores\n", target_core_nr, target_core_nr - enabled_cores));

		_mali_osk_pm_dev_ref_add_no_power_on();
		_mali_osk_pm_dev_barrier();

		domain = mali_pm_domain_get(MALI_PMU_M450_DOM2);

		MALI_PM_DOMAIN_FOR_EACH_GROUP(group, domain)
		{
			mali_pp_scheduler_enable_group_internal(group);
			if (target_core_nr == enabled_cores) break;
		}

		if (target_core_nr > enabled_cores)
		{
			domain = mali_pm_domain_get(MALI_PMU_M450_DOM3);
			MALI_PM_DOMAIN_FOR_EACH_GROUP(group, domain)
			{
				mali_pp_scheduler_enable_group_internal(group);
				if (target_core_nr == enabled_cores) break;
			}
		}

		MALI_DEBUG_ASSERT(target_core_nr == enabled_cores);

		_mali_osk_pm_dev_ref_dec_no_power_on();

		mali_pp_scheduler_schedule();
	}
	else if (target_core_nr < enabled_cores)
	{
		/* Disable some cores */
		struct mali_pm_domain *domain;

		MALI_DEBUG_PRINT(2, ("Requesting %d cores: disabling %d cores\n", target_core_nr, enabled_cores - target_core_nr));

		mali_pp_scheduler_suspend();

		domain = mali_pm_domain_get(MALI_PMU_M450_DOM3);
		if (NULL != domain)
		{
			MALI_PM_DOMAIN_FOR_EACH_GROUP(group, domain)
			{
				mali_pp_scheduler_disable_group_internal(group);
				if (target_core_nr == enabled_cores) break;
			}
		}

		if (target_core_nr < enabled_cores)
		{
			domain = mali_pm_domain_get(MALI_PMU_M450_DOM2);
			MALI_DEBUG_ASSERT_POINTER(domain);
			MALI_PM_DOMAIN_FOR_EACH_GROUP(group, domain)
			{
				mali_pp_scheduler_disable_group_internal(group);
				if (target_core_nr == enabled_cores) break;
			}
		}

		MALI_DEBUG_ASSERT(target_core_nr == enabled_cores);

		mali_pp_scheduler_resume();
	}
}

int mali_pp_scheduler_set_perf_level(unsigned int cores)
{
	if (cores == enabled_cores) return 0;
	if (cores > num_cores) return -EINVAL;
	if (0 == cores) return -EINVAL;

	if (!mali_pp_scheduler_has_virtual_group())
	{
		/* Mali-400 */
		mali_pp_scheduler_set_perf_level_mali400(cores);
	}
	else
	{
		/* Mali-450 */
		mali_pp_scheduler_set_perf_level_mali450(cores);
	}

	return 0;
}

static void mali_pp_scheduler_job_queued(void)
{
	/* We hold a PM reference for every job we hold queued (and running) */
	_mali_osk_pm_dev_ref_add();

	if (mali_utilization_enabled())
	{
		/*
		 * We cheat a little bit by counting the PP as busy from the time a PP job is queued.
		 * This will be fine because we only loose the tiny idle gap between jobs, but
		 * we will instead get less utilization work to do (less locks taken)
		 */
		mali_utilization_pp_start();
	}
}

static void mali_pp_scheduler_job_completed(void)
{
	/* Release the PM reference we got in the mali_pp_scheduler_job_queued() function */
	_mali_osk_pm_dev_ref_dec();

	if (mali_utilization_enabled())
	{
		mali_utilization_pp_end();
	}
}
