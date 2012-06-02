/*
 * tcbd_drv_rf.c
 *
 * Author:  <linux@telechips.com>
 * Description: Telechips broadcast driver
 *
 * Copyright (c) Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tcpal_os.h"
#include "tcbd_feature.h"
#include "tcpal_debug.h"

#include "tcbd_api_common.h"
#include "tcbd_drv_ip.h"
#include "tcbd_drv_io.h"
#include "tcbd_drv_rf.h"

struct tcbd_rf_data {
	u8 addr;
	u32 data;
};

static struct tcbd_rf_data tcc3170_rf_table_band3[] = {
	{0x02, 0x00030000},
	{0x04, 0x00002002},
	{0x05, 0x5500640B},
	{0x06, 0x1AADA754},
	{0x07, 0x00002000},
	{0x08, 0x42222249},
	{0x09, 0x000D2299},
	{0x0A, 0xD87060DD},
	{0x0B, 0x00000E72},
	{0x0C, 0x02AE7077},
	{0x0D, 0x00240376},
	{0x0E, 0X7F7F0711}
};

static struct tcbd_rf_data tcc3170_rf_table_lband[] = {
	{0x02, 0x00030000},
	{0x04, 0x00004001},
	{0x06, 0x0AADA754},
	{0x07, 0x00002000},
	{0x08, 0x42222249},
	{0x09, 0x000D2299},
	{0x0A, 0xD87060DD},
	{0x0B, 0x00000E72},
	{0x0C, 0x02AE7077},
	{0x0D, 0x00240376},
	{0x0E, 0X7F7F0907}
};

static u64 tcbd_div64(u64 x, u64 y)
{
	u64 a, b, q, counter;

	q = 0;
	if (y != 0) {
		while (x >= y) {
			a = x >> 1;
			b = y;
			counter = 1;
			while (a >= b) {
				b <<= 1;
				counter <<= 1;
			}
			x -= b;
			q += counter;
		}
	}
	return q;
}


s32 tcbd_rf_init(struct tcbd_device *_device, enum tcbd_band_type _band)
{
	s32 i = 0;
	s32 size;
	s32 ret = 0;
	struct tcbd_rf_data *rf_data;

	switch (_band) {
	case BAND_TYPE_BAND3:
		size = ARRAY_SIZE(tcc3170_rf_table_band3);
		rf_data = tcc3170_rf_table_band3;
		break;
	case BAND_TYPE_LBAND:
		size = ARRAY_SIZE(tcc3170_rf_table_lband);
		rf_data = tcc3170_rf_table_lband;
		break;
	default:
		return -TCERR_UNKNOWN_BAND;
		break;
	}

	for (i = 0; i < size; i++)
		ret |= tcbd_rf_reg_write(
				_device, rf_data[i].addr, rf_data[i].data);
	return ret;
}

#define SCALE_FACTOR	   22
#define DIV(A, B) (tcbd_div64((A<<SCALE_FACTOR), B))
static inline s32 tcbd_rf_set_frequency(
	struct tcbd_device *_device, s32 _freq_khz, s32 _bw_khz)
{
	s32 ret = 0;
	u64 N, F;
	u64 Flo, VCO_DIV, FOffset, Fvco, FR, PLL_MODE;
	u64 N_int, intF, intVCO_DIV;
	u64 osc_clock, fpfd, f_freq_khz;
	u64 Temp1, Temp2;
	u32 DIV_MODE;
	s32 RST_PLL = 1, band = 0;
	u32 rf_cfg2 = 0, rf_cfg4 = 0;

	FOffset = 0;

	/* l-band */
	if (_freq_khz > 1000000)
		band = 1;

	if (band == 0) {
		/* band III */
		ret |= tcbd_rf_reg_write(_device, 0x04, 0x00002002);
		ret |= tcbd_rf_reg_write(_device, 0x0e, 0x7F7F0711);
	} else {
		/* LBAND */
		ret |= tcbd_rf_reg_write(_device, 0x04, 0x00004001);
		ret |= tcbd_rf_reg_write(_device, 0x0e, 0x7F7F0907);
	}

	ret |= tcbd_rf_reg_read(_device, 0x06, (u32 *)&rf_cfg2);
	if (((rf_cfg2 >> 28) & 0x01) == 0)
		FR = 0;
	else
		FR = 1;

	if (((rf_cfg2 >> 31) & 0x01) == 0)
		PLL_MODE = 2;
	else
		PLL_MODE = 4;

	osc_clock = tcbd_get_osc_clock(_device);
	f_freq_khz = _freq_khz;

	/* Calculate PLL */
	if (f_freq_khz < 250000) {
		/* VHF */
		DIV_MODE = 0x00;
		fpfd = osc_clock >> FR;
		VCO_DIV = 16;

		Flo = f_freq_khz - FOffset;
		Fvco = Flo * VCO_DIV;

		Temp1 = Fvco << FR;
		Temp2 = PLL_MODE * osc_clock;
		N = DIV(Temp1, Temp2);
		N_int = (N >> SCALE_FACTOR) << SCALE_FACTOR;
		F = ((N - N_int) * (2 << 21)) >> SCALE_FACTOR;

		if (N_int < (19 << SCALE_FACTOR)) {
			FR = 1;
			fpfd = osc_clock >> FR;
			VCO_DIV = 16;
			Flo = f_freq_khz - FOffset;
			Fvco = Flo * VCO_DIV;

			Temp1 = Fvco * FR;
			Temp2 = PLL_MODE * osc_clock;
			N = DIV(Temp1, Temp2);
			N_int = (N >> SCALE_FACTOR) << SCALE_FACTOR;
			F = ((N - N_int) * (2 << 21)) >> SCALE_FACTOR;
		}
		intF = F;
		intVCO_DIV = VCO_DIV;
	} else {
		/* LBAND */
		DIV_MODE = 0x01;
		fpfd = osc_clock >> FR;
		VCO_DIV = 2;

		Flo = f_freq_khz - FOffset;
		Fvco = Flo * VCO_DIV;

		Temp1 = Fvco << FR;
		Temp2 = PLL_MODE * osc_clock;
		N = DIV(Temp1, Temp2);
		N_int = (N >> SCALE_FACTOR) << SCALE_FACTOR;
		F = ((N - N_int) * (2 << 21)) >> SCALE_FACTOR;

		if (N_int < (19 << SCALE_FACTOR)) {
			FR = 1;

			VCO_DIV = 2;
			Flo = f_freq_khz - FOffset;
			Fvco = Flo * VCO_DIV;

			Temp1 = Fvco << FR;
			Temp2 = PLL_MODE * osc_clock;
			N = DIV(Temp1, Temp2);
			N_int = (N >> SCALE_FACTOR) << SCALE_FACTOR;
			F = ((N - N_int) * (2<<21)) >> SCALE_FACTOR;
		}
		intF = F;
		intVCO_DIV = VCO_DIV;
	}

	rf_cfg4 = (u32)((N_int >> SCALE_FACTOR) & 0xFF) ;
	rf_cfg4 |= ((intF & 0x3FFFFF) << 8);
	rf_cfg4 |= ((RST_PLL & 0x01) << 30);
	rf_cfg4 |= ((DIV_MODE & 0x01) << 31);
	ret |= tcbd_rf_reg_write(_device, 0x08, rf_cfg4);

	ret |= tcbd_rf_reg_read(_device, 0x06, (u32 *)&rf_cfg2);
	if (FR == 0)
		BITCLR(rf_cfg2, Bit28);
	else
		BITSET(rf_cfg2, Bit28);

	ret |= tcbd_rf_reg_write(_device, 0x06, rf_cfg2);

	return ret;
}

s32 tcbd_rf_tune_frequency(
	struct tcbd_device *_device, u32 _freq_khz, s32 _bw_khz)
{
	s32 ret = 0;

	if (_device->curr_band != _device->prev_band)
		ret |= tcbd_rf_init(_device, _device->curr_band);

	ret |= tcbd_rf_set_frequency(_device, _freq_khz, _bw_khz);

	if (ret < 0)
		tcbd_debug(DEBUG_ERROR,
			"Failed to set frequency to RF, err:%d\n", ret);

	return ret;
}
