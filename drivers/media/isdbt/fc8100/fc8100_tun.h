/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

  File name : fc8100_tun.h

 Description : fc8100 tuner controller header file

 History :
 ----------------------------------------------------------------------
 2009/09/29	bruce		initial
*******************************************************************************/

#ifndef __FC8100_TUNER__
#define __FC8100_TUNER__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8100_tuner_init(HANDLE hDevice, u8 band_type);
extern int fc8100_tuner_deinit(HANDLE hDevice);
extern int fc8100_set_freq(HANDLE hDevice, u8 ch_num);
extern int fc8100_get_rssi(HANDLE hDevice, s32 *i32RSSI);

#ifdef __cplusplus
}
#endif

#endif
