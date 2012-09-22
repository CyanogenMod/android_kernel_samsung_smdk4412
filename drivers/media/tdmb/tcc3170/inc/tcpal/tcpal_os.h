/*
 * tcpal_os.h
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

#ifndef __TCPAL_OS_H__
#define __TCPAL_OS_H__
#include "tcpal_types.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

/* For TimeCheck */
#define TCPAL_MAX_TIMECNT 0xFFFFFFFFFFFFFFFFULL
TCBB_FUNC u64 tcpal_get_time(void);
TCBB_FUNC u64 tcpal_diff_time(u64 _start_timeCount);

/* for sleep */
TCBB_FUNC void tcpal_msleep(s32 _ms);
TCBB_FUNC void tcpal_usleep(s32 _us);

/* for memory allocation, free, set */
TCBB_FUNC void *tcpal_malloc(u32 _size);
TCBB_FUNC void  tcpal_free(void *_ptr);

/* For Semaphore */
#define TCPAL_INFINITE_SEMAPHORE  0xFFFFFFFFUL

TCBB_FUNC s32 tcpal_create_lock(
	u32 *_semaphore,
	char *_name,
	u32 _initialCount);
TCBB_FUNC s32 tcpal_destroy_lock(u32 *_semaphore);
TCBB_FUNC s32 tcpal_lock(u32 *_semaphore);
TCBB_FUNC s32 tcpal_unlock(u32 *_semaphore);

TCBB_FUNC s32 tcpal_irq_register_handler(void *_device);
TCBB_FUNC s32 tcpal_irq_unregister_handler(void);
TCBB_FUNC s32 tcpal_irq_enable(void);
TCBB_FUNC s32 tcpal_irq_disable(void);

#endif /*__TCPAL_OS_H__*/
