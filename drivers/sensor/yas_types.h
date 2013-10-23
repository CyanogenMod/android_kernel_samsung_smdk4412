/*
 * Copyright (c) 2010-2011 Yamaha Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

/*
 * File     yas_types.h
 * Date     2012/10/05
 * Revision 1.4.1
 */

#ifndef __YAS_TYPES_H__
#define __YAS_TYPES_H__

/* macro */
#ifndef	NULL
#define	NULL			((void *)0)
#endif

#if defined(__KERNEL__)
#include <linux/types.h>
#else
#include <stdint.h>
/*typedef signed char	int8_t;*/
/*typedef unsigned char	uint8_t;*/
/*typedef signed short	int16_t;*/
/*typedef unsigned short	uint16_t;*/
/*typedef signed int	int32_t;*/
/*typedef unsigned int	uint32_t;*/
#endif

#endif	/* __YASTYPES_H__ */

/* end of file */
