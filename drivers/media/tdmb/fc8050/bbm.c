/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : bbm.c

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


#include "fci_types.h"
#include "fci_tun.h"
#include "fc8050_regs.h"
#include "fc8050_bb.h"
#include "fci_hal.h"
#include "fc8050_isr.h"

int bbm_com_reset(HANDLE hDevice)
{
	int res;

	res = fc8050_reset(hDevice);

	return res;
}

int bbm_com_probe(HANDLE hDevice)
{
	int res;

	res = fc8050_probe(hDevice);

	return res;
}

int bbm_com_init(HANDLE hDevice)
{
	int res;

	res = fc8050_init(hDevice);

	return res;
}

int bbm_com_deinit(HANDLE hDevice)
{
	int res;

	res = fc8050_deinit(hDevice);

	return res;
}

int bbm_com_read(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;

	res = bbm_read(hDevice, addr, data);

	return res;
}

int bbm_com_byte_read(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;

	res = bbm_byte_read(hDevice, addr, data);

	return res;
}

int bbm_com_word_read(HANDLE hDevice, u16 addr, u16 *data)
{
	int res;

	res = bbm_word_read(hDevice, addr, data);

	return res;
}

int bbm_com_long_read(HANDLE hDevice, u16 addr, u32 *data)
{
	int res;

	res = bbm_long_read(hDevice, addr, data);

	return res;
}

int bbm_com_bulk_read(HANDLE hDevice, u16 addr, u8 *data, u16 size)
{
	int res;

	res = bbm_bulk_read(hDevice, addr, data, size);

	return res;
}

int bbm_com_data(HANDLE hDevice, u16 addr, u8 *data, u16 size)
{
	int res;

	res = bbm_data(hDevice, addr, data, size);

	return res;
}

int bbm_com_write(HANDLE hDevice, u16 addr, u8 data)
{
	int res;

	res = bbm_write(hDevice, addr, data);

	return res;
}

int bbm_com_byte_write(HANDLE hDevice, u16 addr, u8 data)
{
	int res;

	res = bbm_byte_write(hDevice, addr, data);

	return res;
}

int bbm_com_word_write(HANDLE hDevice, u16 addr, u16 data)
{
	int res;

	res = bbm_word_write(hDevice, addr, data);

	return res;
}

int bbm_com_long_write(HANDLE hDevice, u16 addr, u32 data)
{
	int res;

	res = bbm_long_write(hDevice, addr, data);

	return res;
}

int bbm_com_bulk_write(HANDLE hDevice, u16 addr, u8 *data, u16 size)
{
	int res;

	res = bbm_bulk_write(hDevice, addr, data, size);

	return res;
}

int bbm_com_tuner_read(
	HANDLE hDevice, u8 addr, u8 address_len, u8 *buffer, u8 len)
{
	int res;

	res = tuner_i2c_read(hDevice, addr, address_len, buffer, len);

	return res;
}

int bbm_com_tuner_write(
	HANDLE hDevice, u8 addr, u8 address_len, u8 *buffer, u8 len)
{
	int res;

	res = tuner_i2c_write(hDevice, addr, address_len, buffer, len);

	return res;
}

int bbm_com_tuner_set_freq(HANDLE hDevice, u32 freq)
{
	int res = BBM_OK;

	res = tuner_set_freq(hDevice, freq);

	return res;
}

int bbm_com_tuner_select(HANDLE hDevice, u32 product, u32 band)
{
	int res = BBM_OK;

	res = tuner_select(hDevice, product, band);

	return res;
}

int bbm_com_tuner_get_rssi(HANDLE hDevice, s32 *rssi)
{
	int res = BBM_OK;

	res = tuner_get_rssi(hDevice, rssi);

	return res;
}

int bbm_com_scan_status(HANDLE hDevice)
{
	int res = BBM_OK;

	res = fc8050_scan_status(hDevice);

	return res;
}

int bbm_com_channel_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id)
{
	int res;

	res = fc8050_channel_select(hDevice, subchannel_id, service_channel_id);

	return res;
}

int bbm_com_video_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id, u8 cdiId)
{
	int res;

	res = fc8050_video_select(
		hDevice, subchannel_id, service_channel_id, cdiId);

	return res;
}

int bbm_com_audio_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id)
{
	int res;

	res = fc8050_audio_select(hDevice, subchannel_id, service_channel_id);

	return res;
}

int bbm_com_data_select(HANDLE hDevice, u8 subchannel_id, u8 service_channel_id)
{
	int res;

	res = fc8050_data_select(hDevice, subchannel_id, service_channel_id);

	return res;
}

int bbm_com_channel_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id)
{
	int res;

	res = fc8050_channel_deselect(
		hDevice, subchannel_id, service_channel_id);

	return res;
}

int bbm_com_video_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id, u8 cdiId)
{
	int res;

	res = fc8050_video_deselect(
		hDevice, subchannel_id, service_channel_id, cdiId);

	return res;
}

int bbm_com_audio_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id)
{
	int res;

	res = fc8050_audio_deselect(hDevice, subchannel_id, service_channel_id);

	return res;
}

int bbm_com_data_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id)
{
	int res;

	res = fc8050_data_deselect(hDevice, subchannel_id, service_channel_id);

	return res;
}

void bbm_com_isr(HANDLE hDevice)
{
	fc8050_isr(hDevice);
}

int bbm_com_hostif_select(HANDLE hDevice, u8 hostif)
{
	int res = BBM_NOK;

	res = bbm_hostif_select(hDevice, hostif);

	return res;
}

int bbm_com_hostif_deselect(HANDLE hDevice)
{
	int res = BBM_NOK;

	res = bbm_hostif_deselect(hDevice);

	return res;
}

int bbm_com_fic_callback_register(
	u32 userdata
	, int (*callback)(u32 userdata, u8 *data, int length))
{
	fic_user_data = userdata;
	fic_callback = callback;

	return BBM_OK;
}

int bbm_com_msc_callback_register(
	u32 userdata
	, int (*callback)(u32 userdata, u8 subchannel_id, u8 *data, int length))
{
	msc_user_data = userdata;
	msc_callback = callback;

	return BBM_OK;
}

int bbm_com_fic_callback_deregister(HANDLE hDevice)
{
	fic_user_data = 0;
	fic_callback = NULL;

	return BBM_OK;
}

int bbm_com_msc_callback_deregister(HANDLE hDevice)
{
	msc_user_data = 0;
	msc_callback = NULL;

	return BBM_OK;
}
