/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fc8050_bb.h

	Description : baseband header file

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

#ifndef __FC8050_BB_H__
#define __FC8050_BB_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int  fc8050_reset(HANDLE hDevice);
extern int  fc8050_probe(HANDLE hDevice);
extern int  fc8050_init(HANDLE hDevice);
extern int  fc8050_deinit(HANDLE hDevice);

extern int  fc8050_scan_status(HANDLE hDevice);

extern int  fc8050_channel_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id);
extern int  fc8050_video_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id, u8 cdiId);
extern int  fc8050_audio_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id);
extern int  fc8050_data_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id);

extern int  fc8050_channel_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id);
extern int  fc8050_video_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id, u8 cdiId);
extern int  fc8050_audio_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id);
extern int  fc8050_data_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id);

#ifdef __cplusplus
}
#endif

#endif	/* __FC8050_BB_H__ */
