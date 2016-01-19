/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fc8050_tun.c

	Description : fc8050 host interface

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
	2009/09/14	jason   initial
	2009/11/26              MPW Config1p0
	2010/02/24              SLR Config1p0
	2010/05/04              SLR Config1p2
*******************************************************************************/

#include "fci_types.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fci_hal.h"
#include "fc8050_regs.h"

#if !defined(CONFIG_TDMB_EBI)
#define FEATURE_TUNER_BURST_MODE
#endif

static int fc8050_write(HANDLE hDevice, u8 addr, u8 data)
{
	int res;
	u8 tmp;

	tmp = data;
	res = tuner_i2c_write(hDevice, addr, 1, &tmp, 1);

	return res;
}

static int fc8050_read(HANDLE hDevice, u8 addr, u8 *data)
{
	int res;

	res = tuner_i2c_read(hDevice, addr, 1, data, 1);

	return res;
}

static int fc8050_set_filter(HANDLE hDevice)
{
	int i;
	u8 cal_mon = 0;

#if (FC8050_FREQ_XTAL == 19200)
	fc8050_write(hDevice, 0x02, 0x86);

	fc8050_write(hDevice, 0x3B, 0x52);
	fc8050_write(hDevice, 0x32, 0x09);
#elif (FC8050_FREQ_XTAL == 16384)
	fc8050_write(hDevice, 0x02, 0x86);

	fc8050_write(hDevice, 0x3B, 0x45);
	fc8050_write(hDevice, 0x32, 0x09);
#elif (FC8050_FREQ_XTAL == 24576)
	fc8050_write(hDevice, 0x02, 0x86);

	fc8050_write(hDevice, 0x3B, 0x68);
	fc8050_write(hDevice, 0x32, 0x09);
#elif (FC8050_FREQ_XTAL == 27000)
	fc8050_write(hDevice, 0x02, 0x86);

	fc8050_write(hDevice, 0x3B, 0x71);
	fc8050_write(hDevice, 0x32, 0x09);
#elif (FC8050_FREQ_XTAL == 27120)
	fc8050_write(hDevice, 0x02, 0x86);

	fc8050_write(hDevice, 0x3B, 0x74);
	fc8050_write(hDevice, 0x32, 0x09);
#elif (FC8050_FREQ_XTAL == 38400)
	fc8050_write(hDevice, 0x02, 0x86);

	fc8050_write(hDevice, 0x3B, 0xA1);
	fc8050_write(hDevice, 0x32, 0x09);
#else
	return BBM_NOK;
#endif

	for (i = 0; i < 10; i++) {
		ms_wait(5);
		fc8050_read(hDevice, 0x33, &cal_mon);
		if ((cal_mon & 0xC0) == 0xC0)
			break;
		fc8050_write(hDevice, 0x32, 0x01);
		fc8050_write(hDevice, 0x32, 0x09);
	}

	fc8050_write(hDevice, 0x32, 0x01);

	return BBM_OK;
}

static int fc8050_lband_init(HANDLE hDevice)
{
	print_log(hDevice, "fc8050_lband_init\n");
	return BBM_NOK;
}

static int fc8050_band3_init(HANDLE hDevice)
{
	print_log(hDevice, "fc8050_band3_init\n");

	fc8050_write(hDevice, 0x00, 0x00);

	fc8050_write(hDevice, 0x00, 0x00);
	fc8050_write(hDevice, 0x02, 0x86);

	fc8050_write(hDevice, 0x05, 0xD8);
	fc8050_write(hDevice, 0x0A, 0x83);
	fc8050_write(hDevice, 0x16, 0x0d);
	fc8050_write(hDevice, 0x13, 0x88);
	fc8050_write(hDevice, 0x15, 0x00);
	fc8050_write(hDevice, 0x21, 0x73);

	fc8050_write(hDevice, 0x57, 0x40);
	fc8050_write(hDevice, 0x69, 0x8C);
	fc8050_write(hDevice, 0x51, 0x04);
	fc8050_write(hDevice, 0x53, 0x00);
	fc8050_write(hDevice, 0x54, 0x28);
	fc8050_write(hDevice, 0x45, 0x40);
	fc8050_write(hDevice, 0x46, 0x32);
	fc8050_write(hDevice, 0x48, 0x40);
	fc8050_write(hDevice, 0x49, 0x32);
	fc8050_write(hDevice, 0x7A, 0x88);
	fc8050_write(hDevice, 0x53, 0x01);
	fc8050_write(hDevice, 0x58, 0x34);
	fc8050_write(hDevice, 0x59, 0x2A);
	fc8050_write(hDevice, 0x5A, 0x1D);
	fc8050_write(hDevice, 0x5B, 0x14);
	fc8050_write(hDevice, 0x61, 0x64);
	fc8050_write(hDevice, 0x74, 0x3A);
	fc8050_write(hDevice, 0x75, 0x1E);
	fc8050_write(hDevice, 0x6A, 0x0C);
	fc8050_write(hDevice, 0x6C, 0x0C);
	fc8050_write(hDevice, 0x6E, 0x0C);
	fc8050_write(hDevice, 0x70, 0x0C);
	fc8050_write(hDevice, 0x72, 0x0C);
	fc8050_write(hDevice, 0x7C, 0x0C);
	fc8050_write(hDevice, 0x4E, 0x26);
	fc8050_write(hDevice, 0x31, 0x13);
	fc8050_write(hDevice, 0x34, 0x53);
	fc8050_write(hDevice, 0x43, 0x20);
	fc8050_write(hDevice, 0x2e, 0x70);

	fc8050_set_filter(hDevice);
	return BBM_OK;
}

int fc8050_tuner_init(HANDLE hDevice, u32 band)
{
	int res = BBM_NOK;

	bbm_write(hDevice, BBM_QDD_COMMAN, 0x5C);
	bbm_write(hDevice, BBM_QDD_AGC_STEP, 0x03);
	bbm_write(hDevice, BBM_QDD_TUN_COMMA, 0x40);
	bbm_write(hDevice, BBM_QDD_TUN_GAIN, 0x24);
	bbm_write(hDevice, BBM_QDD_AGC_PERIOD, 0x14);
	bbm_write(hDevice, BBM_QDD_TRAGET_RMS, 0x60);
	bbm_write(hDevice, BBM_QDD_TUN_GAIN_LOC, 0x44);
	bbm_write(hDevice, BBM_QDD_GAIN_MAX, 0x38);

	if (band == LBAND_TYPE)
		res = fc8050_lband_init(hDevice);
	else if (band == BAND3_TYPE)
		res = fc8050_band3_init(hDevice);
	else
		return BBM_NOK;

	if (res != BBM_OK)
		return res;

	return res;
}

int fc8050_set_freq(HANDLE hDevice, u32 band, u32 f_lo)
{
	u32 f_diff, f_diff_shifted, n_val, k_val;
	u32 f_vco = f_lo * 12;
	u32 r_val = (f_vco >= 25 * FC8050_FREQ_XTAL) ? 1 : 2;
	u32 f_comp = FC8050_FREQ_XTAL/r_val;
	u8 pre_shift_bits = 4;
	u8 data_0x0E;

	fc8050_write(hDevice, 0x0a, 0x85);
	fc8050_write(hDevice, 0x16, 0x0d);

	n_val = f_vco / f_comp;

	f_diff = f_vco -  f_comp * n_val;
	f_diff_shifted = f_diff << (20 - pre_shift_bits);
	k_val = f_diff_shifted / ((f_comp) >> pre_shift_bits);

	k_val = (f_diff_shifted + (f_comp >> (pre_shift_bits+1)))
		/ (f_comp >> pre_shift_bits);

	data_0x0E = ((r_val == 1) ? 0x40 : 0x50) + (unsigned char)(k_val >> 16);
	fc8050_write(hDevice, 0x0E, data_0x0E);
	fc8050_write(hDevice, 0x0F, (unsigned char)(k_val >> 8));
	fc8050_write(hDevice, 0x10, (unsigned char)(k_val));
	fc8050_write(hDevice, 0x11, (unsigned char)(n_val));

	fc8050_write(hDevice, 0x0a, 0x83);

	return BBM_OK;
}

int fc8050_get_rssi(HANDLE hDevice, int *rssi)
{
	int res = BBM_OK;
	u8  LNA, RFVGA, PREAMP_PGA, CSF = 0x00;
	int K = -66;
#ifdef FEATURE_TUNER_BURST_MODE
	u32 burst_data;

	res = tuner_i2c_read(hDevice, 0x76, 1, (unsigned char *)&burst_data, 4);

	LNA = burst_data&0x000000ff;
	RFVGA = (burst_data&0x0000ff00)>>8;
	CSF = (burst_data&0x00ff0000)>>16;
	PREAMP_PGA = (burst_data&0xff000000)>>24;
#else
	res  = fc8050_read(hDevice, 0x76, &LNA);
	res |= fc8050_read(hDevice, 0x77, &RFVGA);
	res |= fc8050_read(hDevice, 0x78, &CSF);
	res |= fc8050_read(hDevice, 0x79, &PREAMP_PGA);
#endif

	if (res != BBM_OK)
		return res;

	*rssi = (((LNA & 0x07) * 5) + (RFVGA * 7 / 10)
		+ ((PREAMP_PGA >> 7) * 6) + ((CSF & 0x7) * 6)
		- ((PREAMP_PGA & 0x7F) >> 1) + K);

	return BBM_OK;
}
