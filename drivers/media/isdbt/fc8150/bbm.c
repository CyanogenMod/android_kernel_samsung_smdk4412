/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : bbm.c

 Description : API of 1-SEG baseband module

*******************************************************************************/

#include "fci_types.h"
#include "fci_tun.h"
#include "fci_hal.h"
#include "fc8150_bb.h"
#include "fc8150_isr.h"

int BBM_RESET(HANDLE hDevice)
{
	int res;

	res = fc8150_reset(hDevice);

	return res;
}

int BBM_PROBE(HANDLE hDevice)
{
	int res;

	res = fc8150_probe(hDevice);

	return res;
}

int BBM_INIT(HANDLE hDevice)
{
	int res;

	res = fc8150_init(hDevice);

	return res;
}

int BBM_DEINIT(HANDLE hDevice)
{
	int res;

	res = fc8150_deinit(hDevice);

	return res;
}

int BBM_READ(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;

	res = bbm_read(hDevice, addr, data);

	return res;
}

int BBM_BYTE_READ(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;

	res = bbm_byte_read(hDevice, addr, data);

	return res;
}

int BBM_WORD_READ(HANDLE hDevice, u16 addr, u16 *data)
{
	int res;

	res = bbm_word_read(hDevice, addr, data);

	return res;
}

int BBM_LONG_READ(HANDLE hDevice, u16 addr, u32 *data)
{
	int res;

	res = bbm_long_read(hDevice, addr, data);

	return BBM_OK;
}

int BBM_BULK_READ(HANDLE hDevice, u16 addr, u8 *data, u16 size)
{
	int res;

	res = bbm_bulk_read(hDevice, addr, data, size);

	return res;
}

int BBM_DATA(HANDLE hDevice, u16 addr, u8 *data, u32 size)
{
	int res;

	res = bbm_data(hDevice, addr, data, size);

	return res;
}

int BBM_WRITE(HANDLE hDevice, u16 addr, u8 data)
{
	int res;

	res = bbm_write(hDevice, addr, data);

	return res;
}

int BBM_BYTE_WRITE(HANDLE hDevice, u16 addr, u8 data)
{
	int res;

	res = bbm_byte_write(hDevice, addr, data);

	return res;
}

int BBM_WORD_WRITE(HANDLE hDevice, u16 addr, u16 data)
{
	int res;

	res = bbm_word_write(hDevice, addr, data);

	return res;
}

int BBM_LONG_WRITE(HANDLE hDevice, u16 addr, u32 data)
{
	int res;

	res = bbm_long_write(hDevice, addr, data);

	return res;
}

int BBM_BULK_WRITE(HANDLE hDevice, u16 addr, u8 *data, u16 size)
{
	int res;

	res = bbm_bulk_write(hDevice, addr, data, size);

	return res;
}

int BBM_I2C_INIT(HANDLE hDevice, u32 type)
{
	int res;

	res = tuner_ctrl_select(hDevice, type);

	return res;
}

int BBM_I2C_DEINIT(HANDLE hDevice)
{
	int res;

	res = tuner_ctrl_deselect(hDevice);

	return res;
}

int BBM_TUNER_READ(HANDLE hDevice, u8 addr, u8 alen, u8 *buffer, u8 len)
{
	int res;

	res = tuner_i2c_read(hDevice, addr, alen, buffer, len);

	return res;
}

int BBM_TUNER_WRITE(HANDLE hDevice, u8 addr, u8 alen, u8 *buffer, u8 len)
{
	int res;

	res = tuner_i2c_write(hDevice, addr, alen, buffer, len);

	return res;
}

int BBM_TUNER_SET_FREQ(HANDLE hDevice, u32 freq)
{
	int res;

	res = tuner_set_freq(hDevice, freq);

	return res;
}

int BBM_TUNER_SELECT(HANDLE hDevice, u32 product, u32 band)
{
	int res;

	res = tuner_select(hDevice, product, band);

	return res;
}

int BBM_TUNER_DESELECT(HANDLE hDevice)
{
	int res;

	res = tuner_deselect(hDevice);

	return res;
}

int BBM_TUNER_GET_RSSI(HANDLE hDevice, s32 *rssi)
{
	int res;

	res = tuner_get_rssi(hDevice, rssi);

	return res;
}

int BBM_SCAN_STATUS(HANDLE hDevice)
{
	int res;

	res = fc8150_scan_status(hDevice);

	return res;
}

void BBM_ISR(HANDLE hDevice)
{
	fc8150_isr(hDevice);
}

int BBM_HOSTIF_SELECT(HANDLE hDevice, u8 hostif)
{
	int res;

	res = bbm_hostif_select(hDevice, hostif);

	return res;
}

int BBM_HOSTIF_DESELECT(HANDLE hDevice)
{
	int res;

	res = bbm_hostif_deselect(hDevice);

	return res;
}

int BBM_TS_CALLBACK_REGISTER(u32 userdata
	, int (*callback)(u32 userdata, u8 *data, int length))
{
	gTSUserData = userdata;
	pTSCallback = callback;

	return BBM_OK;
}

int BBM_TS_CALLBACK_DEREGISTER(void)
{
	gTSUserData = 0;
	pTSCallback = NULL;

	return BBM_OK;
}

int BBM_AC_CALLBACK_REGISTER(u32 userData
	, int (*callback)(u32 userdata, u8 *data, int length))
{
	gACUserData = userData;
	pACCallback = callback;

	return BBM_OK;
}

int BBM_AC_CALLBACK_DEREGISTER(void)
{
	gACUserData = 0;
	pACCallback = NULL;

	return BBM_OK;
}
