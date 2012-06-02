/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : s3c-otg-transfer-transfer.h
 *  [Description] : The Header file defines the external
 *		and internal functions of Transfer.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/03
 *  [Revision History]
 *		(1) 2008/06/03   by Yang Soon Yeal { syatom.yang@samsung.com }
 *		- Created this file and defines functions of Transfer
 *		-# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com)
 *		: Optimizing for performance
 *
 ****************************************************************************/
/****************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ****************************************************************************/

#ifndef _TRANSFER_H
#define _TRANSFER_H

/*
 transfer-common.c
 transferchecker_common.c
 */
int delete_td(struct sec_otghost *otghost, struct td * delete_td);

static inline u32 calc_packet_cnt(u32 data_size, u16 max_packet_size)
{
	if (data_size != 0) {
		return (data_size % max_packet_size == 0) ?
			data_size/max_packet_size :
			data_size/max_packet_size + 1;
	}
	return 1;
}
#endif

