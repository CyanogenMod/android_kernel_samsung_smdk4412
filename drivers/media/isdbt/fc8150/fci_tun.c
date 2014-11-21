/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fci_tun.c

 Description : tuner control driver
*******************************************************************************/

#include "fci_types.h"
#include "fci_oal.h"
#include "fci_hal.h"
#include "fci_tun.h"
#include "fci_i2c.h"
#include "fci_bypass.h"
#include "fc8150_regs.h"
#include "fc8150_bb.h"
#include "fc8150_tun.h"


#define FC8150_TUNER_ADDR	0x5b

static u8        tuner_addr = FC8150_TUNER_ADDR;
static enum band_type tuner_band = ISDBT_1_SEG_TYPE;
static enum i2c_type  tuner_i2c  = FCI_I2C_TYPE;

struct I2C_DRV {
	int		(*init)(HANDLE hDevice, int speed, int slaveaddr);
	int		(*read)(HANDLE hDevice, u8 chip, u8 addr
		, u8 alen, u8 *data, u8 len);
	int		(*write)(HANDLE hDevice, u8 chip, u8 addr
		, u8 alen, u8 *data, u8 len);
	int		(*deinit)(HANDLE hDevice);
};

static struct I2C_DRV fcii2c = {
	&fci_i2c_init,
	&fci_i2c_read,
	&fci_i2c_write,
	&fci_i2c_deinit
};

static struct I2C_DRV fcibypass = {
	&fci_bypass_init,
	&fci_bypass_read,
	&fci_bypass_write,
	&fci_bypass_deinit
};

struct TUNER_DRV {
	int		(*init)(HANDLE hDevice, enum band_type band);
	int		(*set_freq)(HANDLE hDevice
		, enum band_type band, u32 rf_Khz);
	int		(*get_rssi)(HANDLE hDevice, int *rssi);
	int		(*deinit)(HANDLE hDevice);
};

static struct TUNER_DRV fc8150_tuner = {
	&fc8150_tuner_init,
	&fc8150_set_freq,
	&fc8150_get_rssi,
	&fc8150_tuner_deinit
};

#if 0
static TUNER_DRV fc8151_tuner = {
	&fc8151_tuner_init,
	&fc8151_set_freq,
	&fc8151_get_rssi,
	&fc8151_tuner_deinit
};
#endif

static struct I2C_DRV *tuner_ctrl = &fcii2c;
static struct TUNER_DRV *tuner = &fc8150_tuner;

int tuner_ctrl_select(HANDLE hDevice, enum i2c_type type)
{
	switch (type) {
	case FCI_I2C_TYPE:
		tuner_ctrl = &fcii2c;
		break;
	case FCI_BYPASS_TYPE:
		tuner_ctrl = &fcibypass;
		break;
	default:
		return BBM_E_TN_CTRL_SELECT;
	}

	if (tuner_ctrl->init(hDevice, 600, 0))
		return BBM_E_TN_CTRL_INIT;

	tuner_i2c = type;

	return BBM_OK;
}

int tuner_ctrl_deselect(HANDLE hDevice)
{
	if (tuner_ctrl == NULL)
		return BBM_E_TN_CTRL_SELECT;

	tuner_ctrl->deinit(hDevice);

	tuner_i2c = FCI_I2C_TYPE;
	tuner_ctrl = &fcii2c;

	return BBM_OK;
}

int tuner_i2c_read(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len)
{
	if (tuner_ctrl == NULL)
		return BBM_E_TN_CTRL_SELECT;

	if (tuner_ctrl->read(hDevice, tuner_addr, addr, alen, data, len))
		return BBM_E_TN_READ;

	return BBM_OK;
}

int tuner_i2c_write(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len)
{
	if (tuner_ctrl == NULL)
		return BBM_E_TN_CTRL_SELECT;

	if (tuner_ctrl->write(hDevice, tuner_addr, addr, alen, data, len))
		return BBM_E_TN_WRITE;

	return BBM_OK;
}

int tuner_set_freq(HANDLE hDevice, u32 freq)
{
	if (tuner == NULL)
		return BBM_E_TN_SELECT;

#if (BBM_BAND_WIDTH == 8)
	freq -= 460;
#else
	freq -= 380;
#endif

	if (tuner->set_freq(hDevice, tuner_band, freq))
		return BBM_E_TN_SET_FREQ;

	fc8150_reset(hDevice);

	return BBM_OK;
}

int tuner_select(HANDLE hDevice, u32 product, u32 band)
{
	switch (product) {
	case FC8150_TUNER:
		tuner = &fc8150_tuner;
		tuner_addr = FC8150_TUNER_ADDR;
		tuner_band = band;
		break;
#if 0
	case FC8151_TUNER:
		tuner = &fc8151_tuner;
		tuner_addr = FC8150_TUNER_ADDR;
		tuner_band = band;
		break;
#endif
	default:
		return BBM_E_TN_SELECT;
	}

	if (tuner == NULL)
		return BBM_E_TN_SELECT;

	if (tuner_i2c == FCI_BYPASS_TYPE)
		bbm_write(hDevice, BBM_RF_DEVID, tuner_addr);

	if (tuner->init(hDevice, tuner_band))
		return BBM_E_TN_INIT;

	return BBM_OK;
}

int tuner_deselect(HANDLE hDevice)
{
	if (tuner == NULL)
		return BBM_E_TN_SELECT;

	if (tuner->deinit(hDevice))
		return BBM_NOK;

	tuner = NULL;

	return BBM_OK;
}

int tuner_get_rssi(HANDLE hDevice, s32 *rssi)
{
	if (tuner == NULL)
		return BBM_E_TN_SELECT;

	if (tuner->get_rssi(hDevice, rssi))
		return BBM_E_TN_RSSI;

	return BBM_OK;
}
