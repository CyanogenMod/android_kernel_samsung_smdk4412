/*
 *
 * File name: drivers/media/tdmb/mtv318/src/raontv_rf.h
 *
 * Description : RAONTECH TV RF services header file.
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

#ifndef __RAONTV_RF_H__
#define __RAONTV_RF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "raontv_internal.h"

INT rtvRF_SetFrequency(enum E_RTV_TV_MODE_TYPE eTvMode,
	UINT nChNum,
	U32 dwFreqKHz);
INT rtvRF_ChangeAdcClock(
	enum E_RTV_TV_MODE_TYPE eTvMode,
	enum E_RTV_ADC_CLK_FREQ_TYPE eAdcClkFreqType);
INT rtvRF_ConfigureAdcClock(
	enum E_RTV_TV_MODE_TYPE eTvMode,
	enum E_RTV_ADC_CLK_FREQ_TYPE eAdcClkFreqType);
void rtvRF_ConfigurePowerType(enum E_RTV_TV_MODE_TYPE eTvMode);
INT rtvRF_Initilize(enum E_RTV_TV_MODE_TYPE eTvMode);

#ifdef __cplusplus
}
#endif

#endif /* __RAONTV_RF_H__ */

