/*
 * tcbd_stream_parser.c
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

#include "tcbd_feature.h"
#include "tcbd_stream_parser.h"

#define SIZE_MSC_CACHE_BUFF	(188*20)
#define SIZE_FIC_CACHE_BUFF	(388*4)
#define SIZE_STATUS_CACHE_BUFF  (32*4)

#if defined(__MERGE_EACH_TYPEOF_STREAM__)
#define SIZE_MSC_STACK_BUFF     (1024*16)
#define SIZE_FIC_STACK_BUFF     (388*10)
#define SIZE_STATUS_STACK_BUFF  (32*10)
#endif /*!__MERGE_EACH_TYPEOF_STREAM__*/

#define MAX_SUBCH_ID            (0x1<<6)
enum tcbd_split_stream_state {
	STATE_NO_SYNC = 0,
	STATE_READ_NEXT,
	STATE_OTHER,
	STATE_STATUS,
	STATE_MSC,
	STATE_FIC,
	STATE_GARBAGE,
};

#define HEADER_SIZE 4
#define SYNC_BYTE 0x5C
struct op_header {
	u8 sync;		/**< sync byte. must be 0x5C */
	u8 subch;	   /**< sub channel id */
	u16 data_size;
	u8 parity;
	enum DATA_TYPE type;
};

#if defined(__MERGE_EACH_TYPEOF_STREAM__)
struct merged_stream_info {
	s32 curr_pos;  /**< Current position of buffer of merged stream */
	u8 *buffer;  /**< Buffer for stacking stream chunk */
	u8 subch_id;
};
#endif /* __MERGE_EACH_TYPEOF_STREAM__ */

struct tcbd_split_stream_data {
	enum tcbd_split_stream_state state;
	s32 header_cnt;
	s32 remain;
	s32 next_read;
	u8 *buffer;
	struct op_header header;
#if defined(__MERGE_EACH_TYPEOF_STREAM__)
	struct merged_stream_info merged_msc[TCBD_MAX_NUM_SERVICE];
	struct merged_stream_info merged_fic;
	struct merged_stream_info merged_status;
	u8 quick_msc_idx[MAX_SUBCH_ID];
	u8 num_subch;
#endif /*__MERGE_EACH_TYPEOF_STREAM__ */
	tcbd_stream_callback stream_callback;
};

static struct tcbd_split_stream_data tcbd_stream_spliter;

static inline u8 tcbd_parity_check(u8 *parity)
{
	u32 i, k;
	u8 p = 0;

	for (i = 0; i < 4; i++) {
		for (k = 0; k < 8; k++)
			p = (p + ((parity[i] >> k) & 1)) & 1;
	}

	if (p == 0)
		tcbd_debug(DEBUG_ERROR, "Odd parity error\n");

	return p;
}

static s32 tcbd_find_sync(struct tcbd_split_stream_data *_parser)
{
	s32 i;
	s32 next_hdr, size = _parser->remain;
	u8 *stream = _parser->buffer;

#if defined(__FIND_HEADER_MORE__)
	s32 j, first = 0, num_found = 0, data_size;
#endif /*__FIND_HEADER_MORE__ */

	/* 1. SYNC_BYTE must exist at stream[3] */
	if ((stream[3] == SYNC_BYTE) &&
		(tcbd_parity_check(stream) == 1))
		return 0;

	/* 2. if SYNC_BYTE doesn't exist at stream[3]
		  then search SYNC_BYTE at whole stream*/
	for (i = 0; i < size; i++) {
		if ((i < 3) || (stream[i] != SYNC_BYTE))
			continue;

		if (tcbd_parity_check(&stream[i - 3]) != 1) {
			next_hdr = SWAP32(*((u32 *)&stream[i-3]));
			tcbd_debug(DEBUG_ERROR,
				"parity error!! find next byte, "
				"offset:%d, header:0x%08X\n",
					i, next_hdr);
			continue;
		}
#if !defined(__FIND_HEADER_MORE__)
		tcbd_debug(DEBUG_STREAM_PARSER,
			"found header offset %d\n", i-3);
		return i - 3;
#else /* __FIND_HEADER_MORE__ */
		num_found = 1;
		first = j = i - 3;
		do {
			data_size =
				(stream[j + 1] << 7 | (stream[j+0] >> 1)) << 2;
			j += (data_size + 4);
			if (j >= size)
				break;

			if ((stream[j + 3] == SYNC_BYTE) &&
				(tcbd_parity_check(stream + j) == 1)) {
				tcbd_debug(DEBUG_STREAM_PARSER,
					"header ok pos:%d,"
					"remain:%d\n", j, size);
				num_found++;
			} else {
				next_hdr =
					SWAP32(*((u32 *)(stream + j)));
				tcbd_debug(DEBUG_ERROR,
					"Header ERR!! [%02X] j = %d,"
					" size=%d, data_size:%d\n",
						next_hdr, j, size, data_size);
				num_found--;
				break;
			}
		} while (j < size);

		if (num_found > 1) {
			next_hdr = SWAP32(*((u32 *)(stream+first)));
			tcbd_debug(DEBUG_STREAM_PARSER,
				"offset:%d, header ok count : %d, 0x%08X\n",
					first, num_found, next_hdr);
			return first;
		}
#endif /*__FIND_HEADER_MORE__*/
	}
	return -i;
}

static inline s32 tcbd_parse_header(struct tcbd_split_stream_data *_parser)
{
	u8 *stream = _parser->buffer;
	struct op_header *header = &_parser->header;

	if ((stream[3] != SYNC_BYTE) ||
		(tcbd_parity_check(stream) != 1)) {
		tcbd_debug(DEBUG_ERROR, "wrong header! header:0x%08X\n",
			SWAP32(*(u32 *)stream));
		memset(header, 0, sizeof(struct op_header));
		return HEADER_SIZE;
	}
	_parser->header_cnt++;

	header->sync = stream[3];
	header->type = (stream[2] & 0xC0) >> 6;
	header->data_size = ((stream[1]<<7) | (stream[0]>>1)) << 2;
	header->subch = stream[2] & 0x3F;

	_parser->state = STATE_NO_SYNC;
	switch ((enum DATA_TYPE)header->type) {
	case DATA_TYPE_MSC:
		if (header->data_size != TCBD_FIC_SIZE)
			_parser->state = STATE_MSC;
		break;
	case DATA_TYPE_FIC:
		if (header->subch)
			_parser->state = STATE_GARBAGE;
		else
			_parser->state = STATE_FIC;
		break;
	case DATA_TYPE_STATUS:
		_parser->state = STATE_STATUS;
		break;
	case DATA_TYPE_OTHER:
		_parser->state = STATE_OTHER;
		break;
	default:
		break;
	}
	return HEADER_SIZE;
}

static inline void tcbd_stack_chunk(
	struct tcbd_split_stream_data *_parser, u8 *_buffer)
{
	u8 *chunk_buff = (_buffer) ? _buffer : _parser->buffer;
	struct op_header *header = &_parser->header;

#if defined(__MERGE_EACH_TYPEOF_STREAM__)
	static u8 msc_buff[TCBD_MAX_NUM_SERVICE][SIZE_MSC_STACK_BUFF];
	static u8 fic_buff[SIZE_FIC_STACK_BUFF];
	static u8 status_buff[SIZE_STATUS_STACK_BUFF];
	struct merged_stream_info *merged;
	s32 sz_stack_buff[] = {
		SIZE_MSC_STACK_BUFF,
		SIZE_FIC_STACK_BUFF,
		SIZE_STATUS_STACK_BUFF};
	s32 quick_msc_idx = 0;

	switch (header->type) {
	case DATA_TYPE_MSC:
		if (_parser->num_subch >= TCBD_MAX_NUM_SERVICE) {
			tcbd_debug(DEBUG_ERROR, "error!\n");
			return;
		}
		if (_parser->quick_msc_idx[header->subch] == 0xFF)
			_parser->quick_msc_idx[header->subch] =
				_parser->num_subch++;

		quick_msc_idx = _parser->quick_msc_idx[header->subch];
		if (quick_msc_idx >= TCBD_MAX_NUM_SERVICE)
			tcbd_debug(DEBUG_ERROR, "quick_msc_idx:%d, "
						"header->subch:%d\n",
						quick_msc_idx,
						header->subch);

		merged = &_parser->merged_msc[quick_msc_idx];
		merged->buffer = msc_buff[quick_msc_idx];
		merged->subch_id = header->subch;
		break;
	case DATA_TYPE_FIC:
		merged = &_parser->merged_fic;
		merged->buffer = fic_buff;
		break;
	case DATA_TYPE_STATUS:
		merged = &_parser->merged_status;
		merged->buffer = status_buff;
		break;
	default:
		tcbd_debug(DEBUG_ERROR, "unknown stream type!\n");
		return;
	}

	if (merged->curr_pos + header->data_size >
		sz_stack_buff[header->type]) {
		tcbd_debug(DEBUG_ERROR, "overflow stack buffer!!\n");
		return;
	}

	tcbd_debug(DEBUG_STREAM_STACK,
		"type:%d, subchid:%d, buffer:%p currpos:%d, size:%d\n",
			header->type,
			header->subch,
			merged->buffer,
			merged->curr_pos,
			header->data_size);
	memcpy(merged->buffer + merged->curr_pos,
		chunk_buff, header->data_size);
	merged->curr_pos += header->data_size;

#else  /* __MERGE_EACH_TYPEOF_STREAM__ */
	if (_parser->stream_callback) {
		_parser->stream_callback(
			chunk_buff,
			header->data_size,
			header->subch,
			header->type);
	}
#endif /* !__MERGE_EACH_TYPEOF_STREAM__ */
}

static s32 tcbd_push_chunk(
	struct tcbd_split_stream_data *_parser, u8 *_cached_buff)
{
	s32 move, pre_copied;
	u8 *buffer = _parser->buffer;
	s8 *type[] = {"msc", "fic", "status", "other"};
	s32 size_cache_buff[] = {
		SIZE_MSC_CACHE_BUFF,
		SIZE_FIC_CACHE_BUFF,
		SIZE_STATUS_CACHE_BUFF};
	struct op_header *header = &_parser->header;

	if (_parser->next_read) {
		if (_parser->state != STATE_GARBAGE) {
			pre_copied = header->data_size - _parser->next_read;

			tcbd_debug(DEBUG_PARSING_PROC,
				"send %s data %d bytes, pre:%d,"
				" curr:%d, buffer:%p\n",
					type[header->type],
					header->data_size,
					pre_copied,
					_parser->next_read,
					_cached_buff);

			if (header->data_size > size_cache_buff[header->type])
				tcbd_debug(DEBUG_ERROR,
					"overflow %s cache buffer!!\n",
						type[header->type]);
			memcpy(_cached_buff + pre_copied, buffer,
				_parser->next_read);
			tcbd_stack_chunk(_parser, _cached_buff);
		}
		move = _parser->next_read;
		_parser->state = STATE_NO_SYNC;
		_parser->next_read = 0;
	} else if (_parser->remain >= header->data_size) {
		if (_parser->state != STATE_GARBAGE) {
			tcbd_debug(DEBUG_PARSING_PROC,
				"send %s data %d bytes\n",
					type[header->type],
					header->data_size);
			tcbd_stack_chunk(_parser, NULL);
		}
		_parser->state = STATE_NO_SYNC;
		move = header->data_size;
	} else {
		if (_parser->state != STATE_GARBAGE) {
			tcbd_debug(DEBUG_PARSING_PROC,
				"keep %s data %d bytes buff:%p\n",
					type[header->type],
					_parser->remain,
					_cached_buff);

			if (header->data_size > size_cache_buff[header->type])
				tcbd_debug(DEBUG_ERROR,
					"overflow %s cache buffer!!\n",
						type[header->type]);
			memcpy(_cached_buff, buffer, _parser->remain);
		}
		_parser->next_read = header->data_size - _parser->remain;
		move = _parser->remain;
	}
	return move;
}

#if defined(__MERGE_EACH_TYPEOF_STREAM__)
static s32 tcbd_push_stream(struct tcbd_split_stream_data *_parser)
{
	register s32 i;
	struct merged_stream_info *merged = NULL;
	struct merged_stream_info *list_stream[] = {
		NULL,
		&_parser->merged_fic,
		&_parser->merged_status,
		NULL
	};

	tcbd_debug(DEBUG_STREAM_STACK, "header num:%d, num subch :%d\n",
		tcbd_stream_spliter.header_cnt, _parser->num_subch);
	for (i = 0; i < _parser->num_subch; i++) {
		merged = &_parser->merged_msc[i];
		if (!merged->buffer || !merged->curr_pos)
			continue;

		/* send merged data to user space */
		if (_parser->stream_callback) {
			_parser->stream_callback(merged->buffer,
						merged->curr_pos,
						merged->subch_id,
						DATA_TYPE_MSC);
			merged->buffer = NULL;
			merged->curr_pos = 0;
		}
	}

	for (i = 1; i < (s32)DATA_TYPE_MAX; i++) {
		merged = list_stream[i];
		if (!merged || !merged->buffer || !merged->curr_pos)
			continue;

		/* send merged data to user space */
		if (_parser->stream_callback) {
			_parser->stream_callback(
				merged->buffer, merged->curr_pos,
				merged->subch_id, i);
			merged->buffer = NULL;
			merged->curr_pos = 0;
		}
	}
	return 0;
}
#endif /*__MERGE_EACH_TYPEOF_STREAM__ */

s32 tcbd_split_stream(u8 *_stream, s32 _size)
{
	s32 ret = 0;
	register s32 point, move;

	/* buffer for un-handled spare data of each interrupt */
	static u8 cache_buff_msc[SIZE_MSC_CACHE_BUFF];
	static u8 cache_buff_fic[SIZE_FIC_CACHE_BUFF];
	static u8 buff_cache_status[SIZE_STATUS_CACHE_BUFF];

	u64 time;

	point = move = 0;
	tcbd_stream_spliter.remain = _size;
	tcbd_stream_spliter.header_cnt = 0;

	time = tcpal_get_time();
	while (tcbd_stream_spliter.remain > 0) {
		tcbd_stream_spliter.buffer = _stream + point;
		switch (tcbd_stream_spliter.state) {
		case STATE_NO_SYNC:
			ret = tcbd_find_sync(&tcbd_stream_spliter);
			if (ret < 0) {
				tcbd_debug(DEBUG_STREAM_PARSER,
					"could not find sync byte!! %d\n", ret);
				tcbd_init_parser(NULL);
				return ret;
			} else if (ret > 0) {
				point += ret;
				tcbd_stream_spliter.buffer += ret;
				tcbd_stream_spliter.remain -= ret;
			}
			move = tcbd_parse_header(&tcbd_stream_spliter);
			break;
		case STATE_OTHER:
			move = tcbd_parse_header(&tcbd_stream_spliter);
			tcbd_debug(DEBUG_ERROR, "State Other!! size:%d\n",
				tcbd_stream_spliter.header.data_size);
			break;
		case STATE_MSC:
			move = tcbd_push_chunk(
					&tcbd_stream_spliter,
					cache_buff_msc);
			break;
		case STATE_FIC:
			move = tcbd_push_chunk(
					&tcbd_stream_spliter,
					cache_buff_fic);
			break;
		case STATE_STATUS:
			move = tcbd_push_chunk(
					&tcbd_stream_spliter,
					buff_cache_status);
			break;
		case STATE_GARBAGE:
			move = tcbd_push_chunk(&tcbd_stream_spliter, NULL);
			tcbd_debug(DEBUG_STREAM_PARSER, "State Garbage!:%d\n",
				tcbd_stream_spliter.header.data_size);
			break;
		default:
			move = 0; point = 0;
			tcbd_debug(DEBUG_ERROR, "something wrong!\n");
			break;
		}
		tcbd_stream_spliter.remain -= move;
		point += move;
		tcbd_debug(0, "remain:%d, point:%d, move:%d\n",
			tcbd_stream_spliter.remain, point, move);
	}
#if defined(__MERGE_EACH_TYPEOF_STREAM__)
	ret = tcbd_push_stream(&tcbd_stream_spliter);
#endif /*__MERGE_EACH_TYPEOF_STREAM__*/
	tcbd_debug(DEBUG_PARSING_TIME,
		"%lldms elapsed!\n", tcpal_diff_time(time));
	return ret;
}

void tcbd_init_parser(tcbd_stream_callback _stream_callback)
{
	tcbd_stream_callback bak = NULL;

	if (tcbd_stream_spliter.stream_callback)
		bak = tcbd_stream_spliter.stream_callback;

	memset(&tcbd_stream_spliter, 0, sizeof(tcbd_stream_spliter));
#if defined(__MERGE_EACH_TYPEOF_STREAM__)
	memset(tcbd_stream_spliter.quick_msc_idx, 0xFF,
		sizeof(tcbd_stream_spliter.quick_msc_idx));
#endif /*__MERGE_EACH_TYPEOF_STREAM__*/
	if (_stream_callback)
		tcbd_stream_spliter.stream_callback = _stream_callback;
	else if (bak)
		tcbd_stream_spliter.stream_callback = bak;
}
