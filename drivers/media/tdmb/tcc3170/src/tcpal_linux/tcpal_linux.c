/*
 * tcpal_linux.c
 *
 * Author:  <linux@telechips.com>
 * Description: Telechips broadcast driver
 *
 * Copyright (c) Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tcpal_os.h"
#include "tcpal_debug.h"

#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

#include <linux/io.h>
#include <asm/mach-types.h>

u64 tcpal_get_time(void)
{
	u64 tick;
	struct timeval tv;

	do_gettimeofday(&tv);
	tick = (u64)tv.tv_sec*1000 + tv.tv_usec/1000;

	return tick;
}

u64 tcpal_diff_time(u64 _start_time)
{
	u64 interval;
	u64 curr_time = tcpal_get_time();

	if (curr_time > _start_time)
		interval = curr_time - _start_time;
	else
		interval = (TCPAL_MAX_TIMECNT - _start_time) + curr_time + 1;

	return interval;
}

void tcpal_msleep(s32 _ms)
{
	msleep(_ms);
}

void tcpal_usleep(s32 _us)
{
	struct timeval pre_tv, tv;
	s32 diff;

	do_gettimeofday(&pre_tv);
	do {
		do_gettimeofday(&tv);
		if (tv.tv_usec > pre_tv.tv_usec)
			diff = tv.tv_usec - pre_tv.tv_usec;
		else
			diff = (LONG_MAX - pre_tv.tv_usec) + tv.tv_usec;
	} while (diff < _us);
}

void *tcpal_malloc(u32 _size)
{
	return kmalloc(_size, GFP_KERNEL);
}

void tcpal_free(void *_ptr)
{
	kfree(_ptr);
}

s32 tcpal_create_lock(
	u32 *_semaphore,
	char *_name,
	u32 _initialCount)
{
	struct mutex *lock =
		(struct mutex *)tcpal_malloc(sizeof(struct mutex));
	tcbd_debug(DEBUG_TCPAL_OS, "\n");
	mutex_init(lock);

	*_semaphore = (u32)lock;
	return 0;
}

s32 tcpal_destroy_lock(u32 *_semaphore)
{
	struct mutex *lock = (struct mutex *)*_semaphore;

	if (lock == NULL)
		return -1;

	tcpal_free(lock);
	*_semaphore = 0;
	tcbd_debug(DEBUG_TCPAL_OS, "\n");
	return 0;
}

s32 tcpal_lock(u32 *_semaphore)
{
	struct mutex *lock = (struct mutex *)*_semaphore;

	if (lock == NULL) {
		tcbd_debug(DEBUG_ERROR, "\n");
		return -1;
	}
	mutex_lock(lock);
	return 0;
}

s32 tcpal_unlock(u32 *_semaphore)
{
	struct mutex *lock = (struct mutex *)*_semaphore;

	if (lock == NULL) {
		tcbd_debug(DEBUG_ERROR, "\n");
		return -1;
	}
	mutex_unlock(lock);
	return 0;
}
