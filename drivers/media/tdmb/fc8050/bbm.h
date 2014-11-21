/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : bbm.h

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
	2009/08/29	jason		initial
*******************************************************************************/



#ifndef __BBM_H__
#define __BBM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

#define DRIVER_VER	"VER 3.6.3"

#define BBM_HPI		0		/* HPI */
#define BBM_SPI		1		/* SPI */
#define BBM_USB		2		/* USB */
#define BBM_I2C		3		/* I2C */
#define BBM_PPI		4		/* PPI */

extern int bbm_com_reset(HANDLE hDevice);
extern int bbm_com_probe(HANDLE hDevice);
extern int bbm_com_init(HANDLE hDevice);
extern int bbm_com_deinit(HANDLE hDevice);
extern int bbm_com_read(HANDLE hDevice, u16 addr, u8 *data);
extern int bbm_com_byte_read(HANDLE hDevice, u16 addr, u8 *data);
extern int bbm_com_word_read(HANDLE hDevice, u16 addr, u16 *data);
extern int bbm_com_long_read(HANDLE hDevice, u16 addr, u32 *data);
extern int bbm_com_bulk_read(HANDLE hDevice, u16 addr, u8 *data, u16 size);
extern int bbm_com_data(HANDLE hDevice, u16 addr, u8 *data, u16 size);
extern int bbm_com_write(HANDLE hDevice, u16 addr, u8 data);
extern int bbm_com_byte_write(HANDLE hDevice, u16 addr, u8 data);
extern int bbm_com_word_write(HANDLE hDevice, u16 addr, u16 data);
extern int bbm_com_long_write(HANDLE hDevice, u16 addr, u32 data);
extern int bbm_com_bulk_write(HANDLE hDevice, u16 addr, u8 *data, u16 size);
extern int bbm_com_tuner_read(
	HANDLE hDevice, u8 addr, u8 address_len, u8 *buffer, u8 len);
extern int bbm_com_tuner_write(
	HANDLE hDevice, u8 addr, u8 address_len, u8 *buffer, u8 len);
extern int bbm_com_tuner_set_freq(HANDLE hDevice, u32 freq);
extern int bbm_com_tuner_select(HANDLE hDevice, u32 product, u32 band);
extern int bbm_com_tuner_get_rssi(HANDLE hDevice, s32 *rssi);
extern int bbm_com_scan_status(HANDLE hDevice);
extern int bbm_com_channel_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id);
extern int bbm_com_video_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id, u8 cdiId);
extern int bbm_com_audio_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id);
extern int bbm_com_data_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id);
extern int bbm_com_channel_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id);
extern int bbm_com_video_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id, u8 cdiId);
extern int bbm_com_audio_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id);
extern int bbm_com_data_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id);
extern int bbm_com_hostif_select(HANDLE hDevice, u8 hostif);
extern int bbm_com_hostif_deselect(HANDLE hDevice);
extern int bbm_com_fic_callback_register(
	u32 userdata
	, int (*callback)(u32 userdata, u8 *data, int length));
extern int bbm_com_msc_callback_register(
	u32 userdata
	, int (*callback)(
	u32 userdata, u8 subchannel_id, u8 *data, int length));
extern int bbm_com_fic_callback_deregister(HANDLE hDevice);
extern int bbm_com_msc_callback_deregister(HANDLE hDevice);
extern void bbm_com_isr(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif /* __BBM_H__ */
