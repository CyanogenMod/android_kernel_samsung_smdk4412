/*
 * tcpal_types.h
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

#ifndef __TCPAL_TYPES_H__
#define __TCPAL_TYPES_H__

#ifdef NULL
	#undef NULL
	#define NULL (void *)0
#else
	#define NULL (void *)0
#endif

#define TCBB_FUNC

#define SWAP16(x) \
	((u16)(\
	(((u16)(x)&(u16)0x00ffU) << 8) |\
	(((u16)(x)&(u16)0xff00U) >> 8)))

#define SWAP32(x)\
	((u32)(\
	(((u32)(x)&(u32)0x000000ffUL) << 24)| \
	(((u32)(x)&(u32)0x0000ff00UL) <<  8)| \
	(((u32)(x)&(u32)0x00ff0000UL) >>  8)| \
	(((u32)(x)&(u32)0xff000000UL) >> 24)))

#define MIN(x, y)			((x) < (y) ? (x) : (y))
#define MAX(x, y)			((x) > (y) ? (x) : (y))

#endif
