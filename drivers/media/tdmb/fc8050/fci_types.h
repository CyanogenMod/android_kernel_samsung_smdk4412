/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fci_types.h

	Description :

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA


	History :
	----------------------------------------------------------------------
	2009/08/31	jason		initial
*******************************************************************************/

#ifndef __FCI_TYPES_H__
#define __FCI_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HANDLE
#define HANDLE void *
#endif

#define s8		char
#define s16		short int
#define s32		int
#define u8		unsigned char
#define u16		unsigned short
#define u32		unsigned int
#define TRUE			1
#define FALSE		0

#ifndef NULL
#define NULL		0
#endif

#define BBM_OK		0
#define BBM_NOK		1

#define BBM_E_FAIL              0x00000001
#define BBM_E_HOSTIF_SELECT     0x00000002
#define BBM_E_HOSTIF_INIT       0x00000003
#define BBM_E_BB_REG_WRITE      0x00000100
#define BBM_E_BB_REG_READ       0x00000101
#define BBM_E_TN_REG_WRITE      0x00000200
#define BBM_E_TN_REG_READ       0x00000201
#define BBM_E_TN_CTRL_SELECT    0x00000202
#define BBM_E_TN_CTRL_INIT      0x00000203
#define BBM_E_TN_SELECT         0x00000204
#define BBM_E_TN_INIT           0x00000205
#define BBM_E_TN_RSSI           0x00000206
#define BBM_E_TN_SET_FREQ       0x00000207
#define BBM_E_MUX_SYNC          0x00010000
#define BBM_E_MUX_DATA_MASK     0x00010001
#define BBM_E_MUX_SUBCHANNEL    0x00010002
#define BBM_E_MUX_INDICATOR     0x00010003

#ifdef __cplusplus
}
#endif

#endif /* __FCI_TYPES_H__ */
