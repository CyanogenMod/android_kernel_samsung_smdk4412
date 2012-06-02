/*
 *
 * File name: drivers/media/tdmb/mtv318/src/raontv_ficdec.h
 *
 * Description : RAONTECH FIC Decoder API header file.
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

#ifndef __RAONTV_FICDEC_H__
#define __RAONTV_FICDEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "raontv.h"
#include "tdmb.h"

struct ensemble_info_type *rtvFICDEC_GetEnsembleInfo(unsigned long freq);
void rtvFICDEC_Stop(void);
BOOL rtvFICDEC_Decode(U32 ch_freq_khz);

#ifdef __cplusplus
}
#endif

#endif /* __RAONTV_FICDEC_H__ */

