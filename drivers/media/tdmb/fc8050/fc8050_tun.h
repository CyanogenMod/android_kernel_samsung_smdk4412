/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fc8050_tun.c

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
	2009/09/11	jason		initial
*******************************************************************************/

#ifndef __FC8050_TUNER__
#define __FC8050_TUNER__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8050_tuner_init(HANDLE hDevice, enum band_type band);
extern int fc8050_set_freq(HANDLE hDevice, enum band_type band, u32 f_lo);
extern int fc8050_get_rssi(HANDLE hDevice, int *rssi);

#ifdef __cplusplus
}
#endif

#endif	/* __FC8050_TUNER__ */
