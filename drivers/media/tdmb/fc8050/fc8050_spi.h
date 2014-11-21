/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fc8050_spi.c

	Description : API of dmb baseband module

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
	2009/09/14	jason		initial
*******************************************************************************/

#ifndef __FC8050_SPI__
#define __FC8050_SPI__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8050_spi_init(HANDLE hDevice, u16 param1, u16 param2);
extern int fc8050_spi_byteread(HANDLE hDevice, u16 addr, u8 *data);
extern int fc8050_spi_wordread(HANDLE hDevice, u16 addr, u16 *data);
extern int fc8050_spi_longread(HANDLE hDevice, u16 addr, u32 *data);
extern int fc8050_spi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length);
extern int fc8050_spi_bytewrite(HANDLE hDevice, u16 addr, u8 data);
extern int fc8050_spi_wordwrite(HANDLE hDevice, u16 addr, u16 data);
extern int fc8050_spi_longwrite(HANDLE hDevice, u16 addr, u32 data);
extern int fc8050_spi_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 length);
extern int fc8050_spi_dataread(HANDLE hDevice, u16 addr, u8 *data, u16 length);
extern int fc8050_spi_deinit(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif	/* __FC8050_SPI__ */
