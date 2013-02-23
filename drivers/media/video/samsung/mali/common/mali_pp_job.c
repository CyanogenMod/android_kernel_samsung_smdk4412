/*
 * Copyright (C) 2011-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mali_pp_job.h"
#include "mali_osk.h"
#include "mali_osk_list.h"
#include "mali_kernel_common.h"
#include "mali_uk_types.h"

struct mali_pp_job *mali_pp_job_create(struct mali_session_data *session, _mali_uk_pp_start_job_s *uargs, u32 id)
{
	struct mali_pp_job *job;

	job = _mali_osk_malloc(sizeof(struct mali_pp_job));
	if (NULL != job)
	{
		u32 i;

		if (0 != _mali_osk_copy_from_user(&job->uargs, uargs, sizeof(_mali_uk_pp_start_job_s)))
		{
			_mali_osk_free(job);
			return NULL;
		}

		if (job->uargs.num_cores > _MALI_PP_MAX_SUB_JOBS)
		{
			MALI_PRINT_ERROR(("Mali PP job: Too many sub jobs specified in job object\n"));
			_mali_osk_free(job);
			return NULL;
		}

		_mali_osk_list_init(&job->list);
		job->session = session;
		job->id = id;
		for (i = 0; i < job->uargs.num_cores; i++)
		{
			job->perf_counter_value0[i] = 0;
			job->perf_counter_value1[i] = 0;
		}
		job->sub_jobs_started = 0;
		job->sub_jobs_completed = 0;
		job->sub_job_errors = 0;
		job->pid = _mali_osk_get_pid();
		job->tid = _mali_osk_get_tid();

		return job;
	}

	return NULL;
}

void mali_pp_job_delete(struct mali_pp_job *job)
{
	_mali_osk_free(job);
}
