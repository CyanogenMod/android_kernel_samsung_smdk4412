/*
 *
 * File name: drivers/media/tdmb/mtv318/src/raontv.c
 *
 * Description : RAONTECH TV device driver.
 *
 * Copyright (C) (2011, RAONTECH)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "raontv_rf.h"

BOOL g_fRtvChannelChange;

enum E_RTV_ADC_CLK_FREQ_TYPE g_eRtvAdcClkFreqType;
BOOL g_fRtvStreamEnabled;

#if defined(RTV_TDMB_ENABLE) || defined(RTV_ISDBT_ENABLE)
enum E_RTV_COUNTRY_BAND_TYPE g_eRtvCountryBandType;
#endif

#ifdef RTV_DAB_ENABLE
enum E_RTV_TV_MODE_TYPE g_curDabSetType;
#endif

#ifdef RTV_IF_EBI2
VU8 g_bRtvEbiMapSelData = 0x7;
#endif

UINT g_nRtvMscThresholdSize;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
U8 g_bRtvIntrMaskRegL;

#else
#if !defined(RTV_CIF_MODE_ENABLED) || !defined(RTV_FIC_POLLING_MODE)
U8 g_bRtvIntrMaskRegL;
#endif
#endif

void rtv_ConfigureHostIF(void)
{
#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x77, 0x15); /* TSIF Enable */
	RTV_REG_SET(0x22, 0x48);

#if defined(RTV_IF_MPEG2_PARALLEL_TSIF)
	RTV_REG_SET(0x04, 0x01); /* I2C + TSIF Mode Enable*/
#else
	RTV_REG_SET(0x04, 0x29); /* I2C + TSIF Mode Enable*/
#endif

	RTV_REG_SET(0x0C, 0xF4); /* TSIF Enable*/

#elif defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x77, 0x14); /*SPI Mode Enable*/
	RTV_REG_SET(0x04, 0x28); /* SPI Mode Enable*/
	RTV_REG_SET(0x0C, 0xF5);

#else
#error "Code not present"
#endif
}

INT rtv_InitSystem(enum E_RTV_TV_MODE_TYPE eTvMode,
		enum E_RTV_ADC_CLK_FREQ_TYPE eAdcClkFreqType)
{
	INT nRet;
	int i;

	g_fRtvChannelChange = FALSE;

	g_fRtvStreamEnabled = FALSE;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	g_bRtvIntrMaskRegL = 0xFF;
#else
#ifndef RTV_CIF_MODE_ENABLED /* Individual Mode */
	g_bRtvIntrMaskRegL = 0xFF;
#endif
#endif

	for (i = 1; i <= 100; i++) {
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x7D, 0x06);
		if (RTV_REG_GET(0x7D) == 0x06)
			goto RTV_POWER_ON_SUCCESS;

		RTV_DBGMSG1("[rtv_InitSystem] Power On wait: %d\n", i);

		RTV_DELAY_MS(5);
	}

	RTV_DBGMSG1("rtv_InitSystem: Power On Check error: %d\n", i);
	return RTV_POWER_ON_CHECK_ERROR;

RTV_POWER_ON_SUCCESS:

	rtvRF_ConfigurePowerType(eTvMode);
	nRet = rtvRF_ConfigureAdcClock(eTvMode, eAdcClkFreqType);

	if (nRet != RTV_SUCCESS)
		return nRet;

	return RTV_SUCCESS;
}
