/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : bbm.h

 Description : API of 1-SEG baseband module

*******************************************************************************/
#ifndef __BBM_H__
#define __BBM_H__

#include "fci_types.h"

#define DRIVER_VER	"VER 2.20.1"

#ifdef __cplusplus
extern "C" {
#endif

extern int BBM_RESET(HANDLE hDevice);
extern int BBM_PROBE(HANDLE hDevice);
extern int BBM_INIT(HANDLE hDevice);
extern int BBM_DEINIT(HANDLE hDevice);
extern int BBM_READ(HANDLE hDevice, u16 addr, u8 *data);
extern int BBM_BYTE_READ(HANDLE hDevice, u16 addr, u8 *data);
extern int BBM_WORD_READ(HANDLE hDevice, u16 addr, u16 *data);
extern int BBM_LONG_READ(HANDLE hDevice, u16 addr, u32 *data);
extern int BBM_BULK_READ(HANDLE hDevice, u16 addr, u8 *data, u16 size);
extern int BBM_DATA(HANDLE hDevice, u16 addr, u8 *data, u32 size);
extern int BBM_WRITE(HANDLE hDevice, u16 addr, u8 data);
extern int BBM_BYTE_WRITE(HANDLE hDevice, u16 addr, u8 data);
extern int BBM_WORD_WRITE(HANDLE hDevice, u16 addr, u16 data);
extern int BBM_LONG_WRITE(HANDLE hDevice, u16 addr, u32 data);
extern int BBM_BULK_WRITE(HANDLE hDevice, u16 addr, u8 *data, u16 size);
extern int BBM_I2C_INIT(HANDLE hDevice, u32 type);
extern int BBM_I2C_DEINIT(HANDLE hDevice);
extern int BBM_TUNER_SELECT(HANDLE hDevice, u32 product, u32 band);
extern int BBM_TUNER_DESELECT(HANDLE hDevice);
extern int BBM_TUNER_READ(HANDLE hDevice, u8 addr
	, u8 alen, u8 *buffer, u8 len);
extern int BBM_TUNER_WRITE(HANDLE hDevice, u8 addr
	, u8 alen, u8 *buffer, u8 len);
extern int BBM_TUNER_SET_FREQ(HANDLE hDevice, u32 freq);
extern int BBM_TUNER_GET_RSSI(HANDLE hDevice, s32 *rssi);
extern int BBM_SCAN_STATUS(HANDLE hDevice);
extern int BBM_HOSTIF_SELECT(HANDLE hDevice, u8 hostif);
extern int BBM_HOSTIF_DESELECT(HANDLE hDevice);
extern int BBM_TS_CALLBACK_REGISTER(u32 userdata
	, int (*callback)(u32 userdata, u8 *data, int length));
extern int BBM_TS_CALLBACK_DEREGISTER(void);
extern int BBM_AC_CALLBACK_REGISTER(u32 userData
	, int (*callback)(u32 userdata, u8 *data, int length));
extern int BBM_AC_CALLBACK_DEREGISTER(void);
extern void BBM_ISR(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif /* __BBM_H__ */
