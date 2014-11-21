/*
*
* File name : tcpal_queue.c
*
* Description : tdmb driver
*
* Copyright (C) (2012, Telechips. )
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/


#include "tcpal_os.h"
#include "tcpal_debug.h"

#include "tcbd_feature.h"
#include "tcpal_queue.h"

s32 tcbd_queue_is_full(struct tcbd_queue *_queue)
{
	if (_queue->front == ((_queue->rear+1)%_queue->qsize))
		return 1;
	return 0;
}

s32 tcbd_queue_is_empty(struct tcbd_queue *_queue)
{
	if (_queue->front == _queue->rear)
		return 1;
	return 0;
}

void tcbd_init_queue(
	struct tcbd_queue *_queue, u8* _buffer, s32 _buff_size)
{
	memset((void *)_queue->q, 0,
			sizeof(struct tcbd_queue_item) * TCBD_QUEUE_SIZE);
	_queue->front = 0;
	_queue->rear = 0;
	_queue->qsize = TCBD_QUEUE_SIZE;
	_queue->buff_size = _buff_size;
	_queue->global_buffer = _buffer;
	_queue->pointer = 0;

	tcpal_create_lock(&_queue->sem, "TcbdQueue", 0);
}

void tcbd_deinit_queue(struct tcbd_queue *_queue)
{
	_queue->front	= 0;
	_queue->rear	 = 0;
	_queue->qsize	= 0;
	_queue->buff_size = 0;
	tcpal_destroy_lock(&_queue->sem);
}

void tcbd_reset_queue(struct tcbd_queue *_queue)
{
	_queue->front   = 0;
	_queue->rear	= 0;
	_queue->pointer = 0;
	memset(_queue->q, 0,
		sizeof(struct tcbd_queue_item)*_queue->qsize);
}

s32 tcbd_enqueue(
	struct tcbd_queue *_queue, u8 *_chunk, s32 _size,
	u8 _subch_id, s32 _type)
{
	if (_chunk == NULL || _size <= 0) {
		tcbd_debug(DEBUG_ERROR, "Invalid argument!!\n");
		return -1;
	}

	tcpal_lock(&_queue->sem);

	if (tcbd_queue_is_full(_queue)) {
		tcbd_debug(DEBUG_ERROR, "Queue Full!!\n");
		_queue->pointer = 0;
	}

	if (_queue->q[_queue->rear].buffer <
			_queue->q[_queue->front].buffer) {
		u32 next_pos_rear =
			(u32)_queue->q[_queue->rear].buffer + _size;
		u32 curr_pos_front =
			(u32)_queue->q[_queue->front].buffer;

		if (next_pos_rear > curr_pos_front) {
			tcbd_debug(DEBUG_ERROR, "Buffer overflow!!\n");
			tcbd_reset_queue(_queue);
			tcpal_unlock(&_queue->sem);
			return -1;
		}
	}

	_queue->q[_queue->rear].buffer =
		_queue->global_buffer + _queue->pointer;

	if (_queue->pointer + _size >= _queue->buff_size)
		_queue->pointer = 0;
	else
		_queue->pointer += _size;
	memcpy(_queue->q[_queue->rear].buffer, _chunk, _size);
	_queue->q[_queue->rear].size = _size;
	_queue->q[_queue->rear].type = _type;
	_queue->q[_queue->rear].subch_id = _subch_id;

	_queue->rear = (_queue->rear + 1) % _queue->qsize;
	tcpal_unlock(&_queue->sem);
	return 0;
}

s32 tcbd_dequeue(
	struct tcbd_queue *_queue, u8 *_chunk, s32 *_size,
	u8 *_subch_id, s32 *_type)
{
	tcpal_lock(&_queue->sem);
	if (tcbd_queue_is_empty(_queue)) {
		tcbd_debug(0, "Queue Empty!!\n");
		tcpal_unlock(&_queue->sem);
		return -1;
	}

	if (_queue->q[_queue->front].size > *_size) {
		tcbd_debug(DEBUG_ERROR,
			"insufficient buffer!! size:%d, qsize:%d\n",
				*_size, _queue->q[_queue->front].size);

		tcpal_unlock(&_queue->sem);
		return -1;
	}

	memcpy(_chunk, _queue->q[_queue->front].buffer,
		_queue->q[_queue->front].size);

	*_size = _queue->q[_queue->front].size;
	if (_type)
		*_type = _queue->q[_queue->front].type;
	if (_subch_id)
		*_subch_id = _queue->q[_queue->front].subch_id;

	_queue->front = (_queue->front + 1) % _queue->qsize;
	tcbd_debug(0, "pos:%d, size:%d\n", _queue->pointer, *_size);
	tcpal_unlock(&_queue->sem);
	return 0;
}

s32 tcbd_dequeue_ptr(
		struct tcbd_queue *_queue, u8 **_chunk, s32 *_size, s32 *_type)
{
	tcpal_lock(&_queue->sem);
	if (tcbd_queue_is_empty(_queue)) {
		tcbd_debug(0, "Queue Empty!!\n");
		tcpal_unlock(&_queue->sem);
		return -1;
	}

	if (_queue->q[_queue->front].size > *_size) {
		tcbd_debug(DEBUG_ERROR,
			"insufficient buffer!! size:%d, qsize:%d\n",
				*_size, _queue->q[_queue->front].size);
		tcpal_unlock(&_queue->sem);
		return -1;
	}

	*_chunk = _queue->q[_queue->front].buffer;
	*_size = _queue->q[_queue->front].size;
	if (_type)
		*_type = _queue->q[_queue->front].type;
	_queue->front = (_queue->front + 1) % _queue->qsize;
	tcbd_debug(0, "pos:%d, size:%d\n", _queue->pointer, *_size);
	tcpal_unlock(&_queue->sem);
	return 0;
}

s32 tcbd_get_first_queue_ptr(
	struct tcbd_queue *_queue,	u8 **_chunk, s32 *_size, s32 *_type)
{
	tcpal_lock(&_queue->sem);
	if (tcbd_queue_is_empty(_queue)) {
		tcbd_debug(0, "Queue Empty!!\n");
		tcpal_unlock(&_queue->sem);
		return -1;
	}
	*_size = _queue->q[_queue->front].size;
	*_chunk = _queue->q[_queue->front].buffer;
	if (_type)
		*_type = _queue->q[_queue->front].type;
	tcbd_debug(0, "pos:%d, size:%d\n", _queue->pointer, *_size);
	tcpal_unlock(&_queue->sem);
	return 0;
}
