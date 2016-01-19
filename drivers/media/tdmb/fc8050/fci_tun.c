/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : tuner.c

	Description : tuner driver

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
#include "fci_oal.h"
#include "fci_hal.h"
#include "fci_tun.h"
#include "fci_i2c.h"
#include "fc8050_regs.h"
#include "fc8050_bb.h"
#include "fc8050_tun.h"

#define FC8000_TUNER_ADDR	0x56
#define FC2501_TUNER_ADDR	0x60
#define FC2507_TUNER_ADDR	0x60
#define FC2580_TUNER_ADDR	0x56
#define FC2582_TUNER_ADDR	0x56
#define FC8050_TUNER_ADDR	0x5F

static u8 tuner_addr = FC8050_TUNER_ADDR;
static enum band_type tuner_band = BAND3_TYPE;

struct tuner_i2c_driver {
	int		(*init)
		(HANDLE hDevice, int speed, int slaveaddr);
	int		(*read)
		(HANDLE hDevice, u8 chip, u8 addr
		, u8 address_len, u8 *data, u8 len);
	int		(*write)
		(HANDLE hDevice, u8 chip, u8 addr
		, u8 address_len, u8 *data, u8 len);
};

struct tuner_driver {
	int		(*init)(HANDLE hDevice, enum band_type band);
	int		(*set_freq)
		(HANDLE hDevice, enum band_type band, u32 f_lo);
	int		(*get_rssi)(HANDLE hDevice, int *rssi);
};

static struct tuner_i2c_driver tuner_ctrl;
static struct tuner_driver tuner;

int tuner_ctrl_select(HANDLE hDevice, enum i2c_type type)
{
	switch (type) {
	case FCI_I2C_TYPE:
		tuner_ctrl.init = fci_i2c_init;
		tuner_ctrl.read = fci_i2c_read;
		tuner_ctrl.write = fci_i2c_write;
		break;
	default:
		return BBM_E_TN_CTRL_SELECT;
	}

	if (tuner_ctrl.init(hDevice, 400, 0))
		return BBM_E_TN_CTRL_INIT;
	return BBM_OK;
}

int tuner_i2c_read(HANDLE hDevice, u8 addr, u8 address_len, u8 *data, u8 len)
{
	if (tuner_ctrl.read(hDevice, tuner_addr, addr, address_len, data, len))
		return BBM_E_TN_REG_READ;
	return BBM_OK;
}

int tuner_i2c_write(HANDLE hDevice, u8 addr, u8 address_len, u8 *data, u8 len)
{
	if (tuner_ctrl.write(hDevice, tuner_addr, addr, address_len, data, len))
		return BBM_E_TN_REG_WRITE;
	return BBM_OK;
}

int tuner_type(HANDLE hDevice, u32 *type)
{
	*type = tuner_band;

	return BBM_OK;
}

int tuner_set_freq(HANDLE hDevice, u32 freq)
{
	int res = BBM_NOK;
	if (tuner.init == NULL) {
		print_log(hDevice, "TUNER NULL ERROR\n");
		return BBM_NOK;
	}

	res = tuner.set_freq(hDevice, tuner_band, freq);
	if (res != BBM_OK) {
		print_log(hDevice, "TUNER res ERROR\n");
		return BBM_NOK;
	}

#if (FC8050_FREQ_XTAL == 19200) || (FC8050_FREQ_XTAL == 27000) \
	|| (FC8050_FREQ_XTAL == 27120) || (FC8050_FREQ_XTAL == 38400)
	u8  tmp;
	tmp = (u8)(33554432/freq);
	bbm_write(hDevice, 0xf1, tmp);
#endif

	fc8050_reset(hDevice);

	return res;
}

int tuner_select(HANDLE hDevice, u32 product, u32 band)
{
	switch (product) {
	case FC8050_TUNER:
		tuner_ctrl_select(hDevice, FCI_I2C_TYPE);

		tuner.init = fc8050_tuner_init;
		tuner.set_freq = fc8050_set_freq;
		tuner.get_rssi = fc8050_get_rssi;

		tuner_band = (enum band_type) band;
		tuner_addr = FC8050_TUNER_ADDR;
		break;
	default:
		return BBM_E_TN_SELECT;
	}

	if (tuner.init == NULL) {
		print_log(hDevice, "[ERROR] Can not supported Tuner(%d,%d)\n"
			, product, band);
		return BBM_E_TN_SELECT;
	}

	if (tuner.init(hDevice, tuner_band))
		return BBM_E_TN_INIT;
	return BBM_OK;
}

int tuner_get_rssi(HANDLE hDevice, s32 *rssi)
{
	if (tuner.get_rssi(hDevice, rssi))
		return BBM_E_TN_RSSI;
	return BBM_OK;
}
