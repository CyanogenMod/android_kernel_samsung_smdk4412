/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fc8050_bb.c

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
				BB Config 1p0
*******************************************************************************/
#include <linux/kernel.h>
#include "fci_types.h"
#include "fci_oal.h"
#include "fci_hal.h"
#include "fci_tun.h"
#include "fc8050_regs.h"

#define POWER_SAVE_MODE
#define MSMCHIP

#define LOCK_TIME_TICK				5	/* 5ms */
#define SLOCK_MAX_TIME				200
#define FLOCK_MAX_TIME				300
#define DLOCK_MAX_TIME				500

static int fc8050_power_save_on(HANDLE hDevice)
{
	u8 tmp = 0x64;

	bbm_write(hDevice, BBM_DIDP_POWER_OPT0, 0x06);
	bbm_write(hDevice, BBM_DIDP_POWER_OPT1, 0x06);
	bbm_write(hDevice, BBM_DIDP_POWER_OPT2, 0x07);
	bbm_write(hDevice, BBM_FFT_ADC_CONTROL, 0x1c);

	tuner_i2c_write(hDevice, 0x61, 1, &tmp, 1);

	print_log(hDevice, "Power Save On\n");

	return BBM_OK;
}

static int fc8050_power_save_off(HANDLE hDevice)
{
	u8 tmp = 0x1e;

	bbm_write(hDevice, BBM_DIDP_POWER_OPT0, 0x04);
	bbm_write(hDevice, BBM_DIDP_POWER_OPT1, 0x05);
	bbm_write(hDevice, BBM_DIDP_POWER_OPT2, 0x05);
	bbm_write(hDevice, BBM_FFT_ADC_CONTROL, 0x9c);

	tuner_i2c_write(hDevice, 0x61, 1, &tmp, 1);

	print_log(hDevice, "Power Save Off\n");

	return BBM_OK;
}

static int fc8050_cu_size_check(HANDLE hDevice, u8 svcId, u16 *cuSize)
{
	int res = BBM_NOK;
	int i;
	u16 subch_info = 0;

	*cuSize = 0;

	for (i = 0; i < 40; i++) {
		bbm_word_read(hDevice, 0x192 + 12 * svcId, &subch_info);

		if (subch_info & 0x3ff) {
			*cuSize = subch_info & 0x3ff;
			res = BBM_OK;

			print_log(hDevice
				, "CU CHECK LOOP COUNT: %d ms\n", i * 10);
			break;
		}

		ms_wait(10);
	}

	return res;
}

static int fc8050_set_xtal(HANDLE hDevice)
{
#if (FC8050_FREQ_XTAL == 19200)
	/* Default XTAL */
#elif (FC8050_FREQ_XTAL == 16384)
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x0);
	bbm_write(hDevice, BBM_QDD_COEF, 0x2);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x1);
	bbm_write(hDevice, BBM_QDD_COEF, 0xff);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x2);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfd);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x3);
	bbm_write(hDevice, BBM_QDD_COEF, 0x0);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x4);
	bbm_write(hDevice, BBM_QDD_COEF, 0x4);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x5);
	bbm_write(hDevice, BBM_QDD_COEF, 0x3);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x6);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfc);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x7);
	bbm_write(hDevice, BBM_QDD_COEF, 0xf9);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x8);
	bbm_write(hDevice, BBM_QDD_COEF, 0x2);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x9);
	bbm_write(hDevice, BBM_QDD_COEF, 0xc);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xa);
	bbm_write(hDevice, BBM_QDD_COEF, 0x4);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xb);
	bbm_write(hDevice, BBM_QDD_COEF, 0xf0);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xc);
	bbm_write(hDevice, BBM_QDD_COEF, 0xed);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xd);
	bbm_write(hDevice, BBM_QDD_COEF, 0x13);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xe);
	bbm_write(hDevice, BBM_QDD_COEF, 0x4f);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xf);
	bbm_write(hDevice, BBM_QDD_COEF, 0x6b);
	bbm_write(hDevice, 0xe8, 0x00);
	bbm_write(hDevice, 0xe9, 0x00);
	bbm_write(hDevice, 0xea, 0x00);
	bbm_write(hDevice, 0xeb, 0x04);
	bbm_write(hDevice, 0xec, 0x80);
	bbm_write(hDevice, 0xed, 0x80);
	bbm_write(hDevice, 0xee, 0x06);
#elif (FC8050_FREQ_XTAL == 24576)
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x0);
	bbm_write(hDevice, BBM_QDD_COEF, 0x2);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x1);
	bbm_write(hDevice, BBM_QDD_COEF, 0xff);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x2);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfd);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x3);
	bbm_write(hDevice, BBM_QDD_COEF, 0x0);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x4);
	bbm_write(hDevice, BBM_QDD_COEF, 0x4);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x5);
	bbm_write(hDevice, BBM_QDD_COEF, 0x3);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x6);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfc);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x7);
	bbm_write(hDevice, BBM_QDD_COEF, 0xf9);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x8);
	bbm_write(hDevice, BBM_QDD_COEF, 0x2);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x9);
	bbm_write(hDevice, BBM_QDD_COEF, 0xc);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xa);
	bbm_write(hDevice, BBM_QDD_COEF, 0x4);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xb);
	bbm_write(hDevice, BBM_QDD_COEF, 0xf0);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xc);
	bbm_write(hDevice, BBM_QDD_COEF, 0xed);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xd);
	bbm_write(hDevice, BBM_QDD_COEF, 0x13);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xe);
	bbm_write(hDevice, BBM_QDD_COEF, 0x4f);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xf);
	bbm_write(hDevice, BBM_QDD_COEF, 0x6b);
	bbm_write(hDevice, 0xe8, 0x00);
	bbm_write(hDevice, 0xe9, 0x00);
	bbm_write(hDevice, 0xea, 0x00);
	bbm_write(hDevice, 0xeb, 0x04);
	bbm_write(hDevice, 0xec, 0x80);
	bbm_write(hDevice, 0xed, 0x80);
	bbm_write(hDevice, 0xee, 0x05);
#elif (FC8050_FREQ_XTAL == 27000)
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x0);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfe);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x1);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfe);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x2);
	bbm_write(hDevice, BBM_QDD_COEF, 0x1);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x3);
	bbm_write(hDevice, BBM_QDD_COEF, 0x4);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x4);
	bbm_write(hDevice, BBM_QDD_COEF, 0x3);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x5);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfd);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x6);
	bbm_write(hDevice, BBM_QDD_COEF, 0xf9);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x7);
	bbm_write(hDevice, BBM_QDD_COEF, 0xff);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x8);
	bbm_write(hDevice, BBM_QDD_COEF, 0x9);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x9);
	bbm_write(hDevice, BBM_QDD_COEF, 0x9);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xa);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfb);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xb);
	bbm_write(hDevice, BBM_QDD_COEF, 0xed);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xc);
	bbm_write(hDevice, BBM_QDD_COEF, 0xf5);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xd);
	bbm_write(hDevice, BBM_QDD_COEF, 0x1c);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xe);
	bbm_write(hDevice, BBM_QDD_COEF, 0x4b);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xf);
	bbm_write(hDevice, BBM_QDD_COEF, 0x61);
	bbm_write(hDevice, 0xe8, 0x4b);
	bbm_write(hDevice, 0xe9, 0x11);
	bbm_write(hDevice, 0xea, 0xa4);
	bbm_write(hDevice, 0xeb, 0x03);
	bbm_write(hDevice, 0xec, 0x8c);
	bbm_write(hDevice, 0xed, 0x75);
	bbm_write(hDevice, 0xee, 0x05);
#elif (FC8050_FREQ_XTAL == 27120)
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x0);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfe);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x1);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfe);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x2);
	bbm_write(hDevice, BBM_QDD_COEF, 0x1);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x3);
	bbm_write(hDevice, BBM_QDD_COEF, 0x4);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x4);
	bbm_write(hDevice, BBM_QDD_COEF, 0x2);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x5);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfc);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x6);
	bbm_write(hDevice, BBM_QDD_COEF, 0xf9);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x7);
	bbm_write(hDevice, BBM_QDD_COEF, 0xff);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x8);
	bbm_write(hDevice, BBM_QDD_COEF, 0x9);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x9);
	bbm_write(hDevice, BBM_QDD_COEF, 0x9);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xa);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfb);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xb);
	bbm_write(hDevice, BBM_QDD_COEF, 0xed);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xc);
	bbm_write(hDevice, BBM_QDD_COEF, 0xf5);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xd);
	bbm_write(hDevice, BBM_QDD_COEF, 0x1c);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xe);
	bbm_write(hDevice, BBM_QDD_COEF, 0x4b);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xf);
	bbm_write(hDevice, BBM_QDD_COEF, 0x61);
	bbm_write(hDevice, 0xe8, 0x80);
	bbm_write(hDevice, 0xe9, 0xf1);
	bbm_write(hDevice, 0xea, 0x9f);
	bbm_write(hDevice, 0xeb, 0x03);
	bbm_write(hDevice, 0xec, 0x8d);
	bbm_write(hDevice, 0xed, 0x74);
	bbm_write(hDevice, 0xee, 0x05);
#elif (FC8050_FREQ_XTAL == 38400)
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x0);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfe);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x1);
	bbm_write(hDevice, BBM_QDD_COEF, 0x0);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x2);
	bbm_write(hDevice, BBM_QDD_COEF, 0x3);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x3);
	bbm_write(hDevice, BBM_QDD_COEF, 0x3);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x4);
	bbm_write(hDevice, BBM_QDD_COEF, 0xff);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x5);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfa);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x6);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfb);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x7);
	bbm_write(hDevice, BBM_QDD_COEF, 0x3);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x8);
	bbm_write(hDevice, BBM_QDD_COEF, 0xa);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0x9);
	bbm_write(hDevice, BBM_QDD_COEF, 0x5);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xa);
	bbm_write(hDevice, BBM_QDD_COEF, 0xf7);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xb);
	bbm_write(hDevice, BBM_QDD_COEF, 0xed);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xc);
	bbm_write(hDevice, BBM_QDD_COEF, 0xfa);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xd);
	bbm_write(hDevice, BBM_QDD_COEF, 0x1f);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xe);
	bbm_write(hDevice, BBM_QDD_COEF, 0x49);
	bbm_write(hDevice, BBM_QDD_COEF_BANK_SEL, 0xf);
	bbm_write(hDevice, BBM_QDD_COEF, 0x5c);
	bbm_write(hDevice, 0xe8, 0x36);
	bbm_write(hDevice, 0xe9, 0xd0);
	bbm_write(hDevice, 0xea, 0x69);
	bbm_write(hDevice, 0xeb, 0x03);
	bbm_write(hDevice, 0xec, 0x96);
	bbm_write(hDevice, 0xed, 0x6d);
	bbm_write(hDevice, 0xee, 0x04);
#endif
	return BBM_OK;
}

int fc8050_reset(HANDLE hDevice)
{
	bbm_write(hDevice, BBM_COM_RESET, 0xFE);
	ms_wait(1);
	bbm_write(hDevice, BBM_COM_RESET, 0xFF);

	return BBM_OK;
}

int fc8050_probe(HANDLE hDevice)
{
	u16 ver;
	bbm_word_read(hDevice, BBM_QDD_CHIP_IDL, &ver);

	printk(KERN_DEBUG "tdmb %s : ver(0x%x)\n", __func__, ver);
	return (ver == 0x8050) ? BBM_OK : BBM_NOK;
}

int fc8050_init(HANDLE hDevice)
{
	u8 intMask;

	fc8050_reset(hDevice);
	fc8050_set_xtal(hDevice);

	bbm_write(hDevice, BBM_BUF_MISC_CTRL, 0x19);

	/* bbm_write(hDevice, BBM_24M_CLK_EN, 0xff); */
	bbm_write(hDevice, BBM_VT_CONTROL, 0x03);
	bbm_word_write(hDevice, BBM_SYNC_CNTRL, 0x0020);
	bbm_write(hDevice, BBM_FIC_CRC_CONTROL, 0x03);
	bbm_write(hDevice, BBM_BUF_TEST_MODE, 0x08);
	bbm_write(hDevice, 0x33c, 0x03);

	bbm_write(hDevice, BBM_FFT_MODEM_STSH, 0x03);
	bbm_write(hDevice, BBM_DIDP_MODE, 0x01);
	bbm_write(hDevice, BBM_SYNC_DET_CNTRL, 0x01);
	bbm_word_write(hDevice, BBM_SYNC_DET_MAX_THRL, 0x0A00);
	bbm_write(hDevice, BBM_SYNC_DET_MODE_ENABLE, 0x01);
	bbm_write(hDevice, BBM_BUF_CLOCK_EN, 0xff);
	bbm_write(hDevice, BBM_FFT_SCALEV_IFFT, 0xea);
	bbm_write(hDevice, BBM_SYNC_FT_RANGE, 0x20);
	bbm_write(hDevice, BBM_QDD_AGC530_EN, 0x53);
	bbm_write(hDevice, BBM_QDD_BLOCK_AVG_SIZE, 0x48);
	bbm_write(hDevice, BBM_QDD_BLOCK_AVG_SIZE_LOCK, 0x49);
	bbm_word_write(hDevice, BBM_QDD_GAIN_CONSTANT, 0x0303);
	bbm_write(hDevice, BBM_QDD_DET_CNT_BOUND, 0x60);
	bbm_write(hDevice, BBM_QDD_REF_AMPL, 0x00);
	bbm_write(hDevice, BBM_QDD_BW_CTRL_LOCK, 0x50);
	bbm_write(hDevice, BBM_QDD_DC_CTRL, 0x3f);

	bbm_write(hDevice, BBM_RS_CONTROL, 0x01);
	bbm_word_write(hDevice, BBM_RS_BER_PERIOD, 0x14e);

#if defined(POWER_SAVE_MODE)
	bbm_write(hDevice, BBM_DIDP_POWER_OPT0, 0x06);
	bbm_write(hDevice, BBM_DIDP_ADD_N_SHIFT0, 0x41);
	bbm_write(hDevice, BBM_DIDP_POWER_OPT1, 0x06);
	bbm_write(hDevice, BBM_DIDP_ADD_N_SHIFT1, 0xf1);
	bbm_write(hDevice, BBM_DIDP_POWER_OPT2, 0x07);
	bbm_write(hDevice, BBM_FFT_ADC_CONTROL, 0x1c);
#else
	bbm_write(hDevice, BBM_DIDP_POWER_OPT0, 0x04);
	bbm_write(hDevice, BBM_DIDP_ADD_N_SHIFT0, 0x21);
	bbm_write(hDevice, BBM_DIDP_POWER_OPT1, 0x05);
	bbm_write(hDevice, BBM_DIDP_ADD_N_SHIFT1, 0x21);
	bbm_write(hDevice, BBM_DIDP_POWER_OPT2, 0x05);
	bbm_write(hDevice, BBM_FFT_ADC_CONTROL, 0x9c);
#endif

	bbm_word_write(hDevice, BBM_BUF_FIC_START,	FIC_BUF_START);
	bbm_word_write(hDevice, BBM_BUF_FIC_END, FIC_BUF_END);
	bbm_word_write(hDevice, BBM_BUF_FIC_THR, FIC_BUF_THR);
	bbm_word_write(hDevice, BBM_BUF_CH0_START,	CH0_BUF_START);
	bbm_word_write(hDevice, BBM_BUF_CH0_END, CH0_BUF_END);
	bbm_word_write(hDevice, BBM_BUF_CH0_THR, CH0_BUF_THR);
	bbm_word_write(hDevice, BBM_BUF_CH1_START,	CH1_BUF_START);
	bbm_word_write(hDevice, BBM_BUF_CH1_END, CH1_BUF_END);
	bbm_word_write(hDevice, BBM_BUF_CH1_THR, CH1_BUF_THR);
	bbm_word_write(hDevice, BBM_BUF_CH2_START,	CH2_BUF_START);
	bbm_word_write(hDevice, BBM_BUF_CH2_END, CH2_BUF_END);
	bbm_word_write(hDevice, BBM_BUF_CH2_THR, CH2_BUF_THR);
	bbm_word_write(hDevice, BBM_BUF_CH3_START,	CH3_BUF_START);
	bbm_word_write(hDevice, BBM_BUF_CH3_END, CH3_BUF_END);
	bbm_word_write(hDevice, BBM_BUF_CH3_THR, CH3_BUF_THR);

	bbm_word_write(hDevice, BBM_BUF_INT, 0x01ff);
	bbm_word_write(hDevice, BBM_BUF_ENABLE, 0x01ff);

	intMask = BBM_MF_INT;
	bbm_write(hDevice, BBM_COM_INT_ENABLE, intMask);
	bbm_write(hDevice, BBM_COM_STATUS_ENABLE, intMask);

	return BBM_OK;
}

int fc8050_deinit(HANDLE hDevice)
{
	bbm_write(hDevice, BBM_COM_RESET, 0x00);
	return BBM_OK;
}

int fc8050_channel_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id)
{
	u16 cuSize = 0;

	bbm_write(hDevice
		, BBM_DIDP_CH0_SUBCH + service_channel_id
		, 0x40 | subchannel_id);

	if (fc8050_cu_size_check(hDevice, service_channel_id, &cuSize)) {
		fc8050_power_save_off(hDevice);
		return BBM_OK;
	}

	if (cuSize >= 672) {
		fc8050_power_save_off(hDevice);
		return BBM_OK;
	}

	fc8050_power_save_on(hDevice);

	return BBM_OK;
}

int fc8050_video_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id, u8 cdiId)
{
	if (fc8050_channel_select(
		hDevice, subchannel_id, service_channel_id) != BBM_OK)
		return BBM_NOK;

	bbm_write(hDevice
		, BBM_CDI0_SUBCH_EN+cdiId, 0x40 | subchannel_id);
	bbm_write(hDevice
		, BBM_BUF_CH0_SUBCH+service_channel_id, 0x40 | subchannel_id);

	return BBM_OK;
}

int fc8050_audio_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id)
{
	if (fc8050_channel_select(
		hDevice, subchannel_id, service_channel_id) != BBM_OK)
		return BBM_NOK;

	bbm_write(hDevice
		, BBM_BUF_CH0_SUBCH+service_channel_id, 0x40 | subchannel_id);

	return BBM_OK;
}

int fc8050_data_select(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id)
{
	if (fc8050_channel_select(
		hDevice, subchannel_id, service_channel_id) != BBM_OK)
		return BBM_NOK;

	bbm_write(hDevice
		, BBM_BUF_CH0_SUBCH+service_channel_id, 0x40 | subchannel_id);

	return BBM_OK;
}

int fc8050_channel_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id)
{
	int i;

	bbm_write(hDevice, BBM_DIDP_CH0_SUBCH + service_channel_id, 0);

	for (i = 0; i < 12; i++)
		bbm_write(hDevice, 0x190 + service_channel_id * 12 + i, 0);

	return BBM_OK;
}

int fc8050_video_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id, u8 cdiId)
{
	if (fc8050_channel_deselect(
		hDevice, subchannel_id, service_channel_id) != BBM_OK)
		return BBM_NOK;

	bbm_write(hDevice, BBM_BUF_CH0_SUBCH+service_channel_id, 0x00);
	bbm_write(hDevice, BBM_CDI0_SUBCH_EN+cdiId,   0x00);

	return BBM_OK;
}

int fc8050_audio_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id)
{
	if (fc8050_channel_deselect(
		hDevice, subchannel_id, service_channel_id) != BBM_OK)
		return BBM_NOK;

	bbm_write(hDevice, BBM_BUF_CH0_SUBCH+service_channel_id, 0);

	return BBM_OK;
}

int fc8050_data_deselect(
	HANDLE hDevice, u8 subchannel_id, u8 service_channel_id)
{
	if (fc8050_channel_deselect(
		hDevice, subchannel_id, service_channel_id) != BBM_OK)
		return BBM_NOK;

	bbm_write(hDevice, BBM_BUF_CH0_SUBCH+service_channel_id, 0);

	return BBM_OK;
}

int fc8050_scan_status(HANDLE hDevice)
{
	int i, res = BBM_NOK;
	u8  mode = 0, status = 0, sync_status = 0;
	int slock_cnt, flock_cnt, dlock_cnt;

	bbm_read(hDevice, BBM_SYNC_DET_CNTRL, &mode);

	if ((mode & 0x01) == 0x01) {
		slock_cnt = SLOCK_MAX_TIME / LOCK_TIME_TICK;
		flock_cnt = FLOCK_MAX_TIME / LOCK_TIME_TICK;
		dlock_cnt = DLOCK_MAX_TIME / LOCK_TIME_TICK;

		/* OFDM Detect */
		for (i = 0; i < slock_cnt; i++) {
			ms_wait(LOCK_TIME_TICK);

			bbm_read(hDevice, BBM_SYNC_DET_STATUS, &status);

			if (status & 0x01)
				break;
		}

		if (i == slock_cnt) {
			printk(KERN_DEBUG "tdmb:status(0x%x) s(%d)\n",
				status, slock_cnt);
			return BBM_NOK;
		}

		if ((status & 0x02) == 0x00) {
			printk(KERN_DEBUG "tdmb %s : status(0x%x)\n",
				__func__, status);
			return BBM_NOK;
		}

		/* FRS */
		for (i += 1; i < flock_cnt; i++) {
			ms_wait(LOCK_TIME_TICK);

			bbm_read(hDevice, BBM_SYNC_STATUS, &sync_status);

			if (sync_status & 0x01)
				break;
		}

		if (i == flock_cnt) {
			printk(KERN_DEBUG "tdmb %s : flock_cnt(0x%x)\n"
						, __func__
						, flock_cnt);
			return BBM_NOK;
		}

		/* Digital Lock */
		for (i += 1; i < dlock_cnt; i++) {
			ms_wait(LOCK_TIME_TICK);

			bbm_read(hDevice, BBM_SYNC_STATUS, &sync_status);

			if (sync_status & 0x20) {
				printk(KERN_DEBUG "tdmb:sync_status(0x%x)\n",
					sync_status);
				return BBM_OK;
		}
		}
	} else {
		dlock_cnt = DLOCK_MAX_TIME / LOCK_TIME_TICK;

		for (i = 0; i < dlock_cnt; i++) {
			ms_wait(LOCK_TIME_TICK);

			bbm_read(hDevice, BBM_SYNC_STATUS, &sync_status);
			if (sync_status & 0x20) {
				printk(KERN_DEBUG "tdmb:sync_status(0x%x)\n",
					sync_status);
				return BBM_OK;
		}
	}
	}

	printk(KERN_DEBUG "tdmb %s : res(0x%x)\n"
						, __func__, res);

	return res;
}
