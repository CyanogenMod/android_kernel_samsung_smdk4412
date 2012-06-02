/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fci_tun.h

	Description : tuner control driver header

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

#ifndef __FCI_TUN_H__
#define __FCI_TUN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

enum i2c_type {
	FCI_I2C_TYPE = 0
};

enum band_type {
	BAND3_TYPE = 0,
	LBAND_TYPE
};

enum product_type {
	FC8000_TUNER = 0,
	FC8050_TUNER
};

extern int tuner_i2c_init(HANDLE hDevice, int speed, int slaveaddr);
extern int tuner_i2c_read(
	HANDLE hDevice, u8 addr, u8 address_len, u8 *data, u8 len);
extern int tuner_i2c_write(
	HANDLE hDevice, u8 addr, u8 address_len, u8 *data, u8 len);

extern int tuner_select(HANDLE hDevice, u32 product, u32 band);
extern int tuner_set_freq(HANDLE hDevice, u32 freq);
extern int tuner_get_rssi(HANDLE hDevice, s32 *rssi);

#ifdef __cplusplus
}
#endif

#endif		/*  __FCI_TUN_H__ */
