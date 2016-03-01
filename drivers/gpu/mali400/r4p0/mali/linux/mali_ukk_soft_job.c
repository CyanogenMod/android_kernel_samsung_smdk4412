/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2013 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */
#include <linux/fs.h>       /* file system operations */
#include <asm/uaccess.h>    /* user space access */

#include "mali_ukk.h"
#include "mali_osk.h"
#include "mali_kernel_common.h"
#include "mali_session.h"
#include "mali_ukk_wrappers.h"

#include "mali_soft_job.h"
#include "mali_timeline.h"

int soft_job_start_wrapper(struct mali_session_data *session, _mali_uk_soft_job_start_s __user *uargs)
{
	u32 type, user_job, point;
	_mali_uk_fence_t uk_fence;
	struct mali_timeline_fence fence;
	struct mali_soft_job *job = NULL;
	u32 __user *job_id_ptr = NULL;

	/* If the job was started successfully, 0 is returned.  If there was an error, but the job
	 * was started, we return -ENOENT.  For anything else returned, the job was not started. */

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session, -EINVAL);

	MALI_DEBUG_ASSERT_POINTER(session->soft_job_system);

	if (0 != get_user(type, &uargs->type))                 return -EFAULT;
	if (0 != get_user(user_job, &uargs->user_job))         return -EFAULT;
	if (0 != get_user(job_id_ptr, &uargs->job_id_ptr))     return -EFAULT;

	if (0 != copy_from_user(&uk_fence, &uargs->fence, sizeof(_mali_uk_fence_t))) return -EFAULT;
	mali_timeline_fence_copy_uk_fence(&fence, &uk_fence);

	if (MALI_SOFT_JOB_TYPE_USER_SIGNALED < type) {
		MALI_DEBUG_PRINT_ERROR(("Invalid soft job type specified\n"));
		return -EINVAL;
	}

	/* Create soft job. */
	job = mali_soft_job_create(session->soft_job_system, (enum mali_soft_job_type)type, user_job);
	if (unlikely(NULL == job)) {
		return map_errcode(_MALI_OSK_ERR_NOMEM);
	}

	/* Write job id back to user space. */
	if (0 != put_user(job->id, job_id_ptr)) {
		MALI_PRINT_ERROR(("Mali Soft Job: failed to put job id"));
		mali_soft_job_destroy(job);
		return map_errcode(_MALI_OSK_ERR_NOMEM);
	}

	/* Start soft job. */
	point = mali_soft_job_start(job, &fence);

	if (0 != put_user(point, &uargs->point)) {
		/* Let user space know that something failed after the job was started. */
		return -ENOENT;
	}

	return 0;
}

int soft_job_signal_wrapper(struct mali_session_data *session, _mali_uk_soft_job_signal_s __user *uargs)
{
	u32 job_id;
	_mali_osk_errcode_t err;

	MALI_DEBUG_ASSERT_POINTER(session);

	if (0 != get_user(job_id, &uargs->job_id)) return -EFAULT;

	err = mali_soft_job_system_signal_job(session->soft_job_system, job_id);

	return map_errcode(err);
}
