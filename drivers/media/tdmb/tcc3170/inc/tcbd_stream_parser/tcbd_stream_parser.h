/*
 * tcbd_stream_parser.h
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

#ifndef __TCBD_STREAM_PARSER_H__
#define __TCBD_STREAM_PARSER_H__

#undef __MERGE_EACH_TYPEOF_STREAM__
#define __FIND_HEADER_MORE__

#define SIZE_BUFF_HEADER 4 /*[TYPE(1)][SUBCH(1)][SIZE(2)]*/

enum DATA_TYPE {
	DATA_TYPE_MSC = 0,
	DATA_TYPE_FIC,
	DATA_TYPE_STATUS,
	DATA_TYPE_OTHER,
	DATA_TYPE_MAX
};

typedef s32 (*tcbd_stream_callback)(
	u8 *_stream, s32 _size,	u8 _subch_id, u8 _type);

TCBB_FUNC void tcbd_init_parser(tcbd_stream_callback _streamCallback);
TCBB_FUNC s32 tcbd_split_stream(u8 *_stream, s32 _size);

#endif /*__TCBD_STREAM_PARSER_H__*/
