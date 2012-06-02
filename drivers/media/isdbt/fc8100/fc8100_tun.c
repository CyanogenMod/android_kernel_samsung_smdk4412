/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fc8100_tun.c

 Description : fc8100 tuner controller

 History :
 ----------------------------------------------------------------------
 2009/09/29	bruce		initial
*******************************************************************************/
#include "fci_types.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fc8100_regs.h"
#include "fci_hal.h"

#define TUN_IF_FREQ              500    /*   500 or 1000 (Khz) */
/*
select BB Xtal frequency
16Mhz: 16000  18Mhz:18000   19.2Mhz:19200   24Mhz:24000
26Mhz:26000   32Mhz:32000   40Mhz:40000
*/
#define FC8100_FREQ_XTAL_TUNER    26000	/* 26MHz */
#define DEFAULT_CH_FREQ                  617143 /* 37 ch, freq : 617.143Mhz */
#define CP_BOOT_CURRENT   0  /* 0:250uA   1: 500uA */

static int fc8100_write(HANDLE hDevice, u8 addr, u8 data)
{
	int res = BBM_NOK;

	res = tuner_i2c_write(hDevice, addr, 1, &data, 1);

	return res;
}

static int fc8100_read(HANDLE hDevice, u8 addr, u8 *data)
{
	int res = BBM_NOK;
	res = tuner_i2c_read(hDevice, addr, 1, data, 1);
	return res;
}

static int fc8100_tuner_set_init(HANDLE hDevice)
{
	int res = BBM_NOK;

	res = fc8100_write(hDevice, RF_MODULE_RESET, 0x00);

	/* PLL & VCO Setting */
	/* if ghost channel scan occurred, change charge pump currnet 500uA -> 250uA */
	if (CP_BOOT_CURRENT == 1) {
		/* 500uA */
		res |= fc8100_write(hDevice, RF_PLL_CTRL4, 0x91);
	} else {
		/* 250 uA */
		res |= fc8100_write(hDevice, RF_PLL_CTRL4, 0x11);
	}

	res |= fc8100_write(hDevice, RF_PLL_CTRL11, 0x00);
	res |= fc8100_write(hDevice, RF_PLL_MODE1, 0x40);
	res |= fc8100_write(hDevice, RF_LO_BIAS_MODE1, 0x27);
	res |= fc8100_write(hDevice, RF_BIAS_TOP_CTRL, 0x19);
	res |= fc8100_write(hDevice, RF_PLL_CTRL6, 0x13);
	/* LNA & MIX Setting */
	res |= fc8100_write(hDevice, RF_LNA_ICTRL, 0x30);
	res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL1, 0x33);
	res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL2, 0xAF);
	res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL3, 0x06);
	res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL5, 0x00);
	/* AGC_ADC Setting */
	res |= fc8100_write(hDevice, RF_ADC_BIAS1, 0xB2);
	res |= fc8100_write(hDevice, RF_W_FACTOR1, 0x40);
	res |= fc8100_write(hDevice, RF_ADC_BIAS2, 0x32);
	res |= fc8100_write(hDevice, RF_W_FACTOR2, 0x40);
	res |= fc8100_write(hDevice, RF_PD1_MAX, 0x36);
	res |= fc8100_write(hDevice, RF_PD1_MIN, 0x2D);
	res |= fc8100_write(hDevice, RF_PD2_MAX, 0x1F);
	res |= fc8100_write(hDevice, RF_PD2_MIN, 0x1B);
	res |= fc8100_write(hDevice, RF_AGC_CLK_MODE, 0x73);
	res |= fc8100_write(hDevice, RF_AGC_PD_OFFSET, 0x88);
	res |= fc8100_write(hDevice, RF_RFAGC_WAIT_CLK_RFVGA, 0x28);
	res |= fc8100_write(hDevice, RF_RFAGC_WAIT_CLK_PD_CHANGE, 0x1F);
	res |= fc8100_write(hDevice, RF_IFAGC_WAIT_CLK_FILTER, 0x14);
	res |= fc8100_write(hDevice, RF_RFVGA_L0_MAX, 0x0C);
	res |= fc8100_write(hDevice, RF_RFVGA_L1_MAX, 0x0C);
	res |= fc8100_write(hDevice, RF_RFVGA_L2_MAX, 0x0C);
	res |= fc8100_write(hDevice, RF_RFVGA_L3_MAX, 0x0C);
	res |= fc8100_write(hDevice, RF_RFVGA_L4_MAX, 0x0C);
	res |= fc8100_write(hDevice, RF_RFVGA_L5_MAX, 0x0C);
	res |= fc8100_write(hDevice, RF_RFVGA_L6_MAX, 0x20);
	res |= fc8100_write(hDevice, RF_IFAGC_PGA_MAX, 0x78);
	res |= fc8100_write(hDevice, RF_IFAGC_PGA_MIN, 0x3c);
	res |= fc8100_write(hDevice, RF_LNA_ELNA_Control, 0x11);

	return res;
}

static int fc8100_tuner_set_filter(HANDLE hDevice, unsigned int xtal)
{
	int res = BBM_NOK;
	u8 reg_3D, reg_3E;
	float value;

	value = (float)xtal / 1000;
	reg_3D = (unsigned char)(value * 5.375f);
	reg_3E =  (unsigned char)(value * 4.3125f);

	/* Filter Setting for defined x-tal */
	res = fc8100_write(hDevice, RF_IFAGC_TEST_MODE2, 0x50);
	res |= fc8100_write(hDevice, RF_CSFA_I2C_CRNT, 0x3F);
	res |= fc8100_write(hDevice, RF_CSF_I2C_CORE_CRNT_AH, 0xFF);
	res |= fc8100_write(hDevice, RF_CSF_I2C_OUT_CRNT_AH, 0xFF);
	res |= fc8100_write(hDevice, RF_CSF_RX_CRNT, 0xEA);
	res |= fc8100_write(hDevice, RF_CSF_PRE_CUR, 0x54);
	res |= fc8100_write(hDevice, RF_CSF_CAL_CRNT, 0x40);
	res |= fc8100_write(hDevice, RF_CSF_CF_CLK_DIV_LSB, reg_3D);
	res |= fc8100_write(hDevice, RF_CSF_CLK_DIV_LSB, reg_3E);
	res |= fc8100_write(hDevice, RF_CSF_AGC_CRNT_LOW, 0x00);
	res |= fc8100_write(hDevice, RF_CSF_MODE, 0x09);
	res |= fc8100_write(hDevice, RF_CSF_MODE, 0x01);

	return res;
}


/*=======================================================
  FUNCTION fc8100_tuner_set_etc

  DESCRIPTION
      Set Default Channel

  PARAMETERS
      unsigned ing xtal : xtal value, ex)if 32MHz, xtal = 32000

  DEPENDENCIES
      None

  RETURN VALUE
	0		if successful
	others	if fail

  SIDE EFFECTS
    None.
  =========================================================*/
static int fc8100_tuner_set_etc(HANDLE hDevice, unsigned int xtal)
{
	int res = BBM_NOK;
	unsigned int f_rf = DEFAULT_CH_FREQ;
	unsigned int f_if = TUN_IF_FREQ;
	unsigned int f_diff, f_diff_shifted, n_val, k_val;
	unsigned int r_val;
	unsigned int f_comp;
	unsigned int f_lo = f_rf - f_if;
	unsigned int f_vco = f_lo * 4;
	unsigned char pre_shift_bits, data_0x16;
	unsigned char i;
	unsigned char filtercal;

	r_val = (f_vco >= 25 * xtal) ? 1 : 2;
	f_comp = xtal/r_val;

	pre_shift_bits = 4;

	n_val =	f_vco / f_comp;

	f_diff = f_vco -  f_comp * n_val;
	f_diff_shifted = f_diff << (20 - pre_shift_bits);
	k_val = (f_diff_shifted + (f_comp >> (pre_shift_bits+1)))/(f_comp >> pre_shift_bits);

	data_0x16 = ((r_val == 1) ? 0x00 : 0x10) + (unsigned char)(k_val >> 16);

	/* ISDBT_ON_AGC_AUTO_0p1 - for defined x-tal */
	res = fc8100_write(hDevice, RF_MODE_CTL, 0x06);
	res |= fc8100_write(hDevice, RF_ADC_BIAS3, 0x80);
	res |= fc8100_write(hDevice, RF_RFAGC_MODE, 0xC1);
	res |= fc8100_write(hDevice, RF_RFAGC_MODE2, 0x20);
	/* Default Channel Setting */
	res |= fc8100_write(hDevice, RF_PLL_K2, data_0x16);
	res |= fc8100_write(hDevice, RF_PLL_K1, (unsigned char)(k_val >> 8));
	res |= fc8100_write(hDevice, RF_PLL_K0, (unsigned char)(k_val));
	res |= fc8100_write(hDevice, RF_PLL_N, (unsigned char)(n_val));
	/* Filter Cal */
	res |= fc8100_write(hDevice, RF_CSF_MODE, 0x09);

	msWait(10);
	for (i = 0; i < 10; i++) {
		fc8100_read(hDevice, RF_CSF_CAPTUNE_MON1, &filtercal);
		if ((filtercal & 0xC0) != 0xC0) {
			fc8100_write(hDevice, RF_CSF_MODE, 0x09);
			msWait(10);
			fc8100_write(hDevice, RF_CSF_MODE, 0x01);
		}
	}
	res |= fc8100_write(hDevice, RF_CSF_MODE, 0x01);

	return res;
}

/*=======================================================
  FUNCTION  fc8100_tuner_init

  DESCRIPTION
      FC8100 RF Tuner Register Init

  PARAMETERS
      None

  DEPENDENCIES
      None

  RETURN VALUE
	0            if successful.
	others		if fail.

  SIDE EFFECTS
    None.
  =========================================================*/
int fc8100_tuner_init(HANDLE hDevice, u8 band_type)
{
	int res = BBM_NOK;

	res = fc8100_tuner_set_init(hDevice);
	res |= fc8100_tuner_set_filter(hDevice, FC8100_FREQ_XTAL_TUNER);
	res |= fc8100_tuner_set_etc(hDevice, FC8100_FREQ_XTAL_TUNER);

	return res;
}

int fc8100_tuner_deinit(HANDLE hDevice)
{
	int res = BBM_NOK;

	return res;
}

int fc8100_set_freq(HANDLE hDevice, u8 ch_num)
{
	int res = BBM_NOK;

	unsigned int f_diff, f_diff_shifted, n_val, k_val;
	unsigned int r_val;
	unsigned int f_comp;
	unsigned int f_lo;
	unsigned int f_vco;
	u8 pre_shift_bits, data_0x16;
	unsigned int xtal = FC8100_FREQ_XTAL_TUNER;

	unsigned int f_rf;
	unsigned int f_if;

	PRINTF(0, "Channel Nummber : %d\n", ch_num);

	f_rf = (ch_num - 13) * 6000 + 473143;
	f_if = TUN_IF_FREQ;

	f_lo = f_rf - f_if;
	f_vco = f_lo * 4;

	r_val = (f_vco >= 25*xtal) ? 1 : 2;
	f_comp = xtal/r_val;

	pre_shift_bits = 4;

	n_val = f_vco / f_comp;

	f_diff = f_vco -  f_comp * n_val;
	f_diff_shifted = f_diff << (20 - pre_shift_bits);
	k_val = (f_diff_shifted + (f_comp >> (pre_shift_bits+1))) / (f_comp >> pre_shift_bits);

	if (f_vco < 2180000) {
		res = fc8100_write(hDevice, RF_LO_BIAS_MODE1, 0xE7);
		res |= fc8100_write(hDevice, RF_BIAS_TOP_CTRL, 0x99);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL2, 0xBF);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL3, 0x1F);
	} else if (f_vco < 2276000) {
		res = fc8100_write(hDevice, RF_LO_BIAS_MODE1, 0xE7);
		res |= fc8100_write(hDevice, RF_BIAS_TOP_CTRL, 0x99);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL2, 0xAF);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL3, 0x1F);
	} else if (f_vco < 2420000) {
		res = fc8100_write(hDevice, RF_LO_BIAS_MODE1, 0xE7);
		res |= fc8100_write(hDevice, RF_BIAS_TOP_CTRL, 0x99);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL2, 0xAF);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL3, 0x0F);
	} else if (f_vco < 2516000) {
		res = fc8100_write(hDevice, RF_LO_BIAS_MODE1, 0x27);
		res |= fc8100_write(hDevice, RF_BIAS_TOP_CTRL, 0x19);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL2, 0xAF);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL3, 0x06);
	} else if (f_vco < 2588000) {
		res = fc8100_write(hDevice, RF_LO_BIAS_MODE1, 0x27);
		res |= fc8100_write(hDevice, RF_BIAS_TOP_CTRL, 0x19);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL2, 0xA6);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL3, 0x06);
	} else if (f_vco < 2636000) {
		res = fc8100_write(hDevice, RF_LO_BIAS_MODE1, 0x27);
		res |= fc8100_write(hDevice, RF_BIAS_TOP_CTRL, 0x19);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL2, 0xA6);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL3, 0x00);
	} else {
		res = fc8100_write(hDevice, RF_LO_BIAS_MODE1, 0x27);
		res |= fc8100_write(hDevice, RF_BIAS_TOP_CTRL, 0x19);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL2, 0xA0);
		res |= fc8100_write(hDevice, RF_LNAMIX_ICTRL3, 0x00);
	}

	if (res == BBM_NOK)
		return res;

	data_0x16 = ((r_val == 1) ? 0x00 : 0x10) + (unsigned char)(k_val >> 16);
	res = fc8100_write(hDevice, RF_PLL_K2, data_0x16);
	res |= fc8100_write(hDevice, RF_PLL_K1, (unsigned char)(k_val >> 8));
	res |= fc8100_write(hDevice, RF_PLL_K0, (unsigned char)(k_val));
	res |= fc8100_write(hDevice, RF_PLL_N, (unsigned char)(n_val));

	if (res)
		return res;

	msWait(3);
	/* SQSTBY is need */
	bbm_write(hDevice, BBM_SYSRST, 0x02);

	return res;
}

int fc8100_get_rssi(HANDLE hDevice, s32 *rssi)
{
	int res = BBM_NOK;
	u8 LNA, RFVGA, GSF, PGA;
	/* float RSSI = 0.0f; */
	s32  K = -73;

	res = fc8100_read(hDevice, 0x79, &LNA);
	res |= fc8100_read(hDevice, 0x7A, &RFVGA);
	res |= fc8100_read(hDevice, 0x7B, &GSF);
	res |= fc8100_read(hDevice, 0x7C, &PGA);

	if (res) {
		*rssi = (s32)0xFFFFFFFF;
		return res;
	}

	/* RSSI = (LNA * 6) + (RFVGA) + ((GSF & 0x7) * 6) - (PGA * 0.25f) + K; */

	/* *rssi = (RSSI); */

	*rssi = (LNA * 6) + (RFVGA) + ((GSF & 0x7) * 6) - (PGA >> 2) + K;

	return res;
}
