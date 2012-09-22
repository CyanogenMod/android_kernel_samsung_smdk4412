/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fci_tun.h

 Description : tuner control driver
*******************************************************************************/

#ifndef __FCI_TUN_H__
#define __FCI_TUN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

enum i2c_type {
	FCI_I2C_TYPE        = 0,
	FCI_BYPASS_TYPE     = 1
};

enum band_type {
	ISDBT_1_SEG_TYPE    = 2
};

enum product_type {
	FC8150_TUNER        = 8150,
	FC8151_TUNER        = 8151
};

extern int tuner_ctrl_select(HANDLE hDevice, enum i2c_type type);
extern int tuner_ctrl_deselect(HANDLE hDevice);
extern int tuner_select(HANDLE hDevice, u32 product, u32 band);
extern int tuner_deselect(HANDLE hDevice);

extern int tuner_i2c_read(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len);
extern int tuner_i2c_write(HANDLE hDevice, u8 addr, u8 alen, u8 *data, u8 len);
extern int tuner_set_freq(HANDLE hDevice, u32 freq);
extern int tuner_get_rssi(HANDLE hDevice, s32 *rssi);

#ifdef __cplusplus
}
#endif

#endif
