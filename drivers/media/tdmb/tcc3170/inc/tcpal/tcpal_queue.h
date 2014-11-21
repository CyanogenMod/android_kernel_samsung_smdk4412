/*
 * tcpal_queue.c
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
#ifndef __TCBD_QUEUE_H__
#define __TCBD_QUEUE_H__

#define TCBD_QUEUE_SIZE 50

struct tcbd_queue_item {
	u8 *buffer;
	u8 subch_id;
	s32 size;
	s32 type;
};

struct tcbd_queue {
	s32 front;
	s32 rear;
	s32 qsize;
	s32 pointer;
	s32 buff_size;
	u8 *global_buffer;
	u32 sem;
	struct tcbd_queue_item q[TCBD_QUEUE_SIZE];
};

TCBB_FUNC void tcbd_init_queue(
	struct tcbd_queue *_queue, u8* buffer, s32 _buff_size);
TCBB_FUNC void tcbd_deinit_queue(struct tcbd_queue *_queue);

TCBB_FUNC s32 tcbd_enqueue(
	struct tcbd_queue *_queue, u8 *_chunk, s32 _size,
	u8 _subch_id, s32 _type);

TCBB_FUNC s32 tcbd_dequeue(
	struct tcbd_queue *_queue, u8 *_chunk, s32 *_size,
	u8 *_subch_id, s32 *_type);

TCBB_FUNC s32 tcbd_dequeue_ptr(
	struct tcbd_queue *_queue, u8 **_chunk,	s32 *_size,	s32 *_type);

TCBB_FUNC s32 tcbd_get_first_queue_ptr(
	struct tcbd_queue *_queue, u8 **_chunk,	s32 *_size,	s32 *_type);

TCBB_FUNC s32 tcbd_queue_is_empty(struct tcbd_queue *_queue);
TCBB_FUNC s32 tcbd_queue_is_full(struct tcbd_queue *_queue);
TCBB_FUNC void tcbd_reset_queue(struct tcbd_queue *_queue);
#endif /*__TCBD_QUEUE_H__*/
