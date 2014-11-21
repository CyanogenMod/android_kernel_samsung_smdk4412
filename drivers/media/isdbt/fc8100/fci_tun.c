/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : tuner.c

 Description : tuner driver

 History :
 ----------------------------------------------------------------------
 2009/08/29	jason		initial
*******************************************************************************/

#include "fci_types.h"
#include "fci_oal.h"
#include "fci_hal.h"
#include "fci_tun.h"
#include "fci_i2c.h"
#include "fc8100_regs.h"
#include "fc8100_tun.h"

#define FC8100_TUNER_ADDR	0x56

static u8 tuner_addr = FC8100_TUNER_ADDR;
static band_type tuner_band = ISDBT_1_SEG_TYPE;

typedef struct {
	int	(*init)(HANDLE hDevice, int speed, int slaveaddr);
	int	(*read)(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len);
	int	(*write)(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len);
	int	(*deinit)(HANDLE hDevice);
} I2C_DRV;

static I2C_DRV fcii2c = {
	&fci_i2c_init,
	&fci_i2c_read,
	&fci_i2c_write,
	&fci_i2c_deinit
};

typedef struct {
	int	(*init)(HANDLE hDevice, u8 band);
	int	(*set_freq)(HANDLE hDevice, u8 ch_num);
	int	(*get_rssi)(HANDLE hDevice, s32 *rssi);
	int	(*deinit)(HANDLE hDevice);
} TUNER_DRV;

static TUNER_DRV fc8100_tuner = {
	&fc8100_tuner_init,
	&fc8100_set_freq,
	&fc8100_get_rssi,
	&fc8100_tuner_deinit
};

static I2C_DRV *tuner_ctrl = &fcii2c;
static TUNER_DRV *tuner = &fc8100_tuner;

int tuner_ctrl_select(HANDLE hDevice, i2c_type type)
{
	int res = BBM_OK;

	switch (type) {
		case FCI_I2C_TYPE:
			tuner_ctrl = &fcii2c;
			break;
		default:
			res = BBM_NOK;
			break;
	}
	if (res)
		return BBM_NOK;

	res = tuner_ctrl->init(hDevice, 400, 0);

	return res;
}

int tuner_i2c_read(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len)
{
	int res = BBM_OK;

	res = tuner_ctrl->read(hDevice, tuner_addr, addr, alen, data, len);

	return res;
}

int tuner_i2c_write(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len)
{
	int res = BBM_OK;

	res = tuner_ctrl->write(hDevice, tuner_addr, addr, alen, data, len);

	return res;
}

int tuner_type(HANDLE hDevice, u32 *type)
{
	*type = tuner_band;

	return BBM_OK;
}

int tuner_set_freq(HANDLE hDevice, u8 ch_num)
{
	int res = BBM_NOK;

	if (tuner == NULL) {
		PRINTF(NULL, "TUNER NULL ERROR \r\n");
		return BBM_NOK;
	}

	res = tuner->set_freq(hDevice, ch_num);
	if (res != BBM_OK) {
		PRINTF(NULL, "TUNER res ERROR \r\n");
		return BBM_NOK;
	}

	return res;
}

int tuner_select(HANDLE hDevice, u32 product, u32 band)
{
	int res = BBM_OK;

	switch (product) {
		case FC8100_TUNER:
			tuner_band = band;
			tuner_addr = FC8100_TUNER_ADDR;
			tuner = &fc8100_tuner;
			res = tuner_ctrl_select(hDevice, FCI_I2C_TYPE);
			PRINTF(0, "FC8100 Tuner Select\n");
			break;
		default:
			res = BBM_NOK;
			break;
	}

	if (tuner == NULL || res) {
		PRINTF(NULL, "[ERROR] Can not supported Tuner(%d,%d)\n\r", product, band);
		return BBM_NOK;
	}

	res = tuner->init(hDevice, band);
	PRINTF(NULL, "TUNER_SELECT(%s) \n\r", res == BBM_OK ? "SUCCESS" : "FAIL");

	return res;
}

int tuner_deselect(HANDLE hDevice)
{
	int res = BBM_OK;

	return res;
}

int tuner_get_rssi(HANDLE hDevice, s32 *rssi)
{
	int res = BBM_OK;

	res = tuner->get_rssi(hDevice, rssi);

	return res;
}
