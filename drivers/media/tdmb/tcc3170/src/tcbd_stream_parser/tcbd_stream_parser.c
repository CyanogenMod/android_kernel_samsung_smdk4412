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
#define SIZE_FIC_CACHE_BUFF	(388*2)
#define SIZE_STATUS_CACHE_BUFF  (32*2)

#if defined(__MERGE_EACH_TYPEOF_STREAM__)
#define SIZE_MSC_STACK_BUFF     (1024*16)
#define SIZE_FIC_STACK_BUFF     (388*10)
#define SIZE_STATUS_STACK_BUFF  (32*10)
#endif /*!__MERGE_EACH_TYPEOF_STREAM__*/

#define MAX_SUBCH_ID            (0x1<<6)
enum tcbd_split_stream_state {
	STATE_NO_SYNC = 0,
	STATE_OTHER,
	STATE_STATUS,
	STATE_MSC,
	STATE_FIC,
	STATE_GARBAGE,
	STATE_ERROR
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
	s32 dev_idx;
	s32 header_err[2];
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

static struct tcbd_split_stream_data tcbd_stream_spliter[2];

static inline u8 tcbd_parity_check(u8 *parity)
{
	u32 i, k;
	u8 p = 0;

	for (i = 0; i < 4; i++) {
		for (k = 0; k < 8; k++)
			p = (p + ((parity[i] >> k) & 1)) & 1;
	}

	return p;
}

#define FIC_ID        0x00
#define STATUS_ID     0x3F
#define GARBAGE_ID    0x04
#define OTHER_CHIP_ID 0x3D

static inline void tcbd_change_parser_state(struct op_header *header,
					struct tcbd_split_stream_data *_parser)
{
	s32 size_limit[] = { -1, 4096, 32, 4096, 388, 4096};
	s8 *type[] = {NULL, "other", "status", "msc", "fic", "garbage"};

	switch ((enum DATA_TYPE)header->type) {
	case DATA_TYPE_MSC:
		if (header->data_size == 388) {
			_parser->state = STATE_FIC;
			header->type = DATA_TYPE_FIC;
		} else
			_parser->state = STATE_MSC;
		break;
	case DATA_TYPE_FIC:
		if (header->subch == GARBAGE_ID)
			_parser->state = STATE_GARBAGE;
		else if (header->subch == FIC_ID)
			_parser->state = STATE_FIC;
		else
			_parser->state = STATE_ERROR;
		break;
	case DATA_TYPE_STATUS:
		if (header->subch == STATUS_ID)
			_parser->state = STATE_STATUS;
		else
			_parser->state = STATE_ERROR;
		break;
	case DATA_TYPE_OTHER:
		if (header->subch == OTHER_CHIP_ID)
			_parser->state = STATE_OTHER;
		else
			_parser->state = STATE_ERROR;
		break;
	default:
		tcbd_debug(DEBUG_ERROR, "unknown data type!!");
		_parser->state = STATE_ERROR;
		break;
	}
	switch (_parser->state) {
	case STATE_ERROR:
		_parser->header_err[1]++;
		_parser->state = STATE_NO_SYNC;
		break;
	default:
		if (size_limit[_parser->state] < header->data_size) {
			tcbd_debug(DEBUG_ERROR, "wrong data size %s:%d!\n",
				type[_parser->state], header->data_size);
			_parser->header_err[1]++;
			_parser->state = STATE_NO_SYNC;

		} else
			_parser->header_cnt++;
		break;
	}
}

static s32 tcbd_parse_header(struct tcbd_split_stream_data *_parser)
{
	u8 *stream = _parser->buffer;
	struct op_header *header = &_parser->header;

	_parser->state = STATE_NO_SYNC;
	if ((stream[3] != SYNC_BYTE) || (tcbd_parity_check(stream) != 1)) {
		_parser->header_err[0]++;
		return HEADER_SIZE;
	}

	header->sync = stream[3];
	header->type = (stream[2] & 0xC0) >> 6;
	header->data_size = ((stream[1]<<7) | (stream[0]>>1)) << 2;
	header->subch = stream[2] & 0x3F;

	tcbd_debug(DEBUG_PARSE_HEADER, "sync:0x%02X, type:%d, size:%d\n",
					header->sync, header->type,
					header->data_size);
	tcbd_change_parser_state(header, _parser);

	return HEADER_SIZE;
}

#if defined(__MERGE_EACH_TYPEOF_STREAM__)
static void tcbd_merge_each_stream(struct tcbd_split_stream_data *_parser,
							u8 *chunk_buff)
{
	static u8 msc_buff[TCBD_MAX_NUM_SERVICE][SIZE_MSC_STACK_BUFF];
	static u8 fic_buff[SIZE_FIC_STACK_BUFF];
	static u8 status_buff[SIZE_STATUS_STACK_BUFF];

	struct merged_stream_info *merged;
	struct op_header *header = &_parser->header;

	s32 sz_stack_buff[] = {
		SIZE_MSC_STACK_BUFF,
		SIZE_FIC_STACK_BUFF,
		SIZE_STATUS_STACK_BUFF};
	s32 quick_msc_idx = 0;

	switch (header->type) {
	case DATA_TYPE_MSC:
		if (_parser->num_subch >= TCBD_MAX_NUM_SERVICE) {
			tcbd_debug(DEBUG_ERROR, "exceeded max num service!\n");
			return;
		}
		if (_parser->quick_msc_idx[header->subch] == 0xFF)
			_parser->quick_msc_idx[header->subch] =
				_parser->num_subch++;

		quick_msc_idx = _parser->quick_msc_idx[header->subch];
		if (quick_msc_idx >= TCBD_MAX_NUM_SERVICE)
			tcbd_debug(DEBUG_ERROR, "quick_msc_idx:%d, header->"
				"subch:%d\n", quick_msc_idx, header->subch);

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

	if (merged->curr_pos+header->data_size > sz_stack_buff[header->type]) {
		tcbd_debug(DEBUG_ERROR, "overflow stack buffer!!\n");
		return;
	}

	tcbd_debug(DEBUG_STREAM_STACK, "type:%d, subchid:%u, buffer:%p "
			"currpos:%d, size:%d\n", header->type, header->subch,
			merged->buffer, merged->curr_pos, header->data_size);
	memcpy(merged->buffer + merged->curr_pos, chunk_buff,
			header->data_size);
	merged->curr_pos += header->data_size;
}

static s32 tcbd_push_merged_stream(struct tcbd_split_stream_data *_parser)
{
	register s32 i;
	struct merged_stream_info *merged = NULL;
	struct merged_stream_info *list_stream[] = {
		NULL, &_parser->merged_fic, &_parser->merged_status, NULL
	};

	tcbd_debug(DEBUG_STREAM_STACK, "header num:%d, num subch :%d\n",
				_parser->header_cnt, _parser->num_subch);
	for (i = 0; i < _parser->num_subch; i++) {
		merged = &_parser->merged_msc[i];
		if (!merged->buffer || !merged->curr_pos)
			continue;

		/* send merged data to user space */
		if (_parser->stream_callback) {
			_parser->stream_callback(_parser->dev_idx,
				merged->buffer, merged->curr_pos,
				merged->subch_id, DATA_TYPE_MSC);
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
			_parser->stream_callback(_parser->dev_idx,
				merged->buffer, merged->curr_pos,
				merged->subch_id, i);
			merged->buffer = NULL;
			merged->curr_pos = 0;
		}
	}
	return 0;
}
#endif /*__MERGE_EACH_TYPEOF_STREAM__ */

static void tcbd_stack_chunk(struct tcbd_split_stream_data *_parser,
							u8 *_buffer)
{
	u8 *chunk_buff = (_buffer) ? _buffer : _parser->buffer;
#if defined(__MERGE_EACH_TYPEOF_STREAM__)
	tcbd_merge_each_stream(_parser, chunk_buff);
#else  /* __MERGE_EACH_TYPEOF_STREAM__ */
	struct op_header *header = &_parser->header;
	if (_parser->stream_callback) {
		_parser->stream_callback(_parser->dev_idx, chunk_buff,
			header->data_size, header->subch, header->type);
	}
#endif /* !__MERGE_EACH_TYPEOF_STREAM__ */
}

static inline s32 tcbd_push_concat(struct tcbd_split_stream_data *_parser,
							u8 *_cache_buff)
{
	s32 pre_copied, ret;
	u8 *buffer = _parser->buffer;
	struct op_header *header = &_parser->header;

	s8 *type[] = {"msc", "fic", "status", "other"};
	s32 size_cache_buff[] = {
		SIZE_MSC_CACHE_BUFF,
		SIZE_FIC_CACHE_BUFF,
		SIZE_STATUS_CACHE_BUFF};

	if (header->data_size > size_cache_buff[header->type]) {
		tcbd_debug(DEBUG_ERROR, "overflow %s cache buffer!! size:%d\n",
					type[header->type], header->data_size);
		_parser->state = STATE_ERROR;
		ret = HEADER_SIZE;
		goto exit_func;
	}

	pre_copied = header->data_size - _parser->next_read;
	if (_parser->next_read > _parser->remain) {
		memcpy(_cache_buff + pre_copied, buffer, _parser->remain);
		_parser->next_read -= _parser->remain;
		ret = _parser->remain;
		tcbd_debug(DEBUG_PARSING_PROC, "keep %s data %d bytes, pre:%d,"
				"next:%d, buffer:%p\n", type[header->type],
				_parser->remain, pre_copied, _parser->next_read,
				_cache_buff);
	} else {
		memcpy(_cache_buff + pre_copied, buffer, _parser->next_read);
		tcbd_stack_chunk(_parser, _cache_buff);
		ret = _parser->next_read;
		tcbd_debug(DEBUG_PARSING_PROC, "send %s data %d bytes, pre:%d,"
				"curr:%d, buffer:%p\n", type[header->type],
				header->data_size, pre_copied,
				_parser->next_read, _cache_buff);
		_parser->state = STATE_NO_SYNC;
	}

exit_func:
	return ret;
}

static inline void tcbd_push_medium(struct tcbd_split_stream_data *_parser)
{
	struct op_header *header = &_parser->header;
	s8 *type[] = {"msc", "fic", "status", "other"};

	tcbd_debug(DEBUG_PARSING_PROC, "send %s data %d bytes\n",
				type[header->type], header->data_size);
	tcbd_stack_chunk(_parser, NULL);
}

static inline s32 tcbd_cache_ramnant(struct tcbd_split_stream_data *_parser,
							u8 *_cache_buff)
{
	struct op_header *header = &_parser->header;
	u8 *buffer = _parser->buffer;
	s8 *type[] = {"msc", "fic", "status", "other"};
	s32 size_cache_buff[] = {
		SIZE_MSC_CACHE_BUFF,
		SIZE_FIC_CACHE_BUFF,
		SIZE_STATUS_CACHE_BUFF};

	tcbd_debug(DEBUG_PARSING_PROC, "keep %s data %d bytes buff:%p\n",
			type[header->type], _parser->remain, _cache_buff);

	if (header->data_size > size_cache_buff[header->type]) {
		tcbd_debug(DEBUG_ERROR, "overflow %s cache buffer!! size:%d\n",
			type[header->type], header->data_size);
		_parser->state = STATE_ERROR;
		return HEADER_SIZE;
	} else {
		memcpy(_cache_buff, buffer, _parser->remain);
		return _parser->remain;
	}
}

static s32 tcbd_push_chunk(struct tcbd_split_stream_data *_parser,
							u8 *_cached_buff)
{
	s32 move;
	struct op_header *header = &_parser->header;

	if (_parser->next_read) {
		if (_parser->state != STATE_GARBAGE)
			move = tcbd_push_concat(_parser, _cached_buff);
		else {
			if (_parser->next_read > _parser->remain) {
				_parser->next_read -= _parser->remain;
				move = _parser->remain;
			} else {
				move = _parser->next_read;
				_parser->state = STATE_NO_SYNC;
			}
		}
	} else if (_parser->remain >= header->data_size) {
		if (_parser->state != STATE_GARBAGE)
			tcbd_push_medium(_parser);

		_parser->state = STATE_NO_SYNC;
		move = header->data_size;
	} else {
		if (_parser->state != STATE_GARBAGE)
			move = tcbd_cache_ramnant(_parser, _cached_buff);
		else
			move = _parser->remain;

		_parser->next_read = header->data_size - _parser->remain;
	}

	switch (_parser->state) {
	case STATE_NO_SYNC:
#if defined(__USING_TS_IF__)
		if (_parser->next_read == 0)
			move += 188 - ((move + SIZE_BUFF_HEADER) % 188);
		else
			move += 188 - (move % 188);
#endif /*__USING_TS_IF__*/
	case STATE_ERROR:
		_parser->state = STATE_NO_SYNC;
		_parser->next_read = 0;
	default:
		break;
	}

	return move;
}

s32 tcbd_split_stream(s32 _dev_idx, u8 *_stream, s32 _size)
{
	s32 ret = 0;
	register s32 point, move;
	/* buffer for un-handled spare data of each interrupt */
	static u8 cache_buff_msc[SIZE_MSC_CACHE_BUFF];
	static u8 cache_buff_fic[SIZE_FIC_CACHE_BUFF];
	static u8 buff_cache_status[SIZE_STATUS_CACHE_BUFF];

	struct tcbd_split_stream_data *spliter = &tcbd_stream_spliter[_dev_idx];

	u64 time = 0;
	point = move = 0;
	spliter->remain = _size;
	spliter->header_cnt = 0;
	memset(spliter->header_err, 0, sizeof(spliter->header_err));

	time = tcpal_get_time();
	while (spliter->remain > 0) {
		spliter->buffer = _stream + point;
		switch (spliter->state) {
		case STATE_NO_SYNC:
			move = tcbd_parse_header(spliter);
			break;
		case STATE_OTHER:
			move = tcbd_parse_header(spliter);
			tcbd_debug(DEBUG_ERROR, "State Other!! size:%d\n",
						spliter->header.data_size);
			break;
		case STATE_MSC:
			move = tcbd_push_chunk(spliter, cache_buff_msc);
			break;
		case STATE_FIC:
			move = tcbd_push_chunk(spliter, cache_buff_fic);
			break;
		case STATE_STATUS:
			move = tcbd_push_chunk(spliter, buff_cache_status);
			break;

		case STATE_GARBAGE:
			move = tcbd_push_chunk(spliter, NULL);
			tcbd_debug(DEBUG_STREAM_PARSER, "State Garbage!:%d\n",
						spliter->header.data_size);
			break;
		default:
			move = 0; point = 0;
			spliter->state = STATE_NO_SYNC;
			spliter->next_read = 0;
			tcbd_debug(DEBUG_ERROR, "something wrong!\n");
			goto exit_func;
		}
		spliter->remain -= move;
		point += move;
		tcbd_debug(0, "remain:%d, point:%d, move:%d\n",
				spliter->remain, point, move);
	}
exit_func:
#if defined(__MERGE_EACH_TYPEOF_STREAM__)
	ret = tcbd_push_merged_stream(spliter);
#endif /*__MERGE_EACH_TYPEOF_STREAM__*/
	tcbd_debug(DEBUG_PARSING_TIME, "%lldms elapsed to parse!\n",
					tcpal_diff_time(time));

	if (spliter->header_err[0] || spliter->header_err[1])
		tcbd_debug(DEBUG_ERROR, "header err, parity:%d, state:%d\n",
			spliter->header_err[0], spliter->header_err[1]);
	return ret;
}

void tcbd_init_parser(s32 _dev_idx, tcbd_stream_callback _stream_callback)
{
	tcbd_stream_callback bak = NULL;
	struct tcbd_split_stream_data *spliter = &tcbd_stream_spliter[_dev_idx];

	if (spliter->stream_callback)
		bak = spliter->stream_callback;

	memset(spliter, 0, sizeof(struct tcbd_split_stream_data));
	spliter->dev_idx = _dev_idx;
#if defined(__MERGE_EACH_TYPEOF_STREAM__)
	memset(spliter->quick_msc_idx, 0xFF, sizeof(spliter->quick_msc_idx));
#endif /*__MERGE_EACH_TYPEOF_STREAM__*/
	if (_stream_callback)
		spliter->stream_callback = _stream_callback;
	else if (bak)
		spliter->stream_callback = bak;
}
