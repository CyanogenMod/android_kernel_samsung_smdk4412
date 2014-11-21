/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fc8000_hal.h

	Description : fc8000 host interface header

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
	2009/08/29	jason		initial
*******************************************************************************/

#ifndef __FCI_HAL_H__
#define __FCI_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int bbm_hostif_select(HANDLE hDevice, u8 hostif);
extern int bbm_hostif_deselect(HANDLE hDevice);
extern int bbm_hostif_get(HANDLE hDevice, u8 *hostif);

extern int bbm_read(HANDLE hDevice, u16 addr, u8 *data);
extern int bbm_byte_read(HANDLE hDevice, u16 addr, u8 *data);
extern int bbm_word_read(HANDLE hDevice, u16 addr, u16 *data);
extern int bbm_long_read(HANDLE hDevice, u16 addr, u32 *data);
extern int bbm_bulk_read(HANDLE hDevice, u16 addr, u8 *data, u16 length);

extern int bbm_write(HANDLE hDevice, u16 addr, u8 data);
extern int bbm_byte_write(HANDLE hDevice, u16 addr, u8 data);
extern int bbm_word_write(HANDLE hDevice, u16 addr, u16 data);
extern int bbm_long_write(HANDLE hDevice, u16 addr, u32 data);
extern int bbm_bulk_write(HANDLE hDevice, u16 addr, u8 *data, u16 length);

extern int bbm_data(HANDLE hDevice, u16 addr, u8 *data, u16 length);

#ifdef __cplusplus
}
#endif

#endif		/* __FCI_HAL_H__ */
