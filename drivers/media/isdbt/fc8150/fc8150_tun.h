/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8150_tun.c

 Description : fc8150 tuner control driver

*******************************************************************************/

#ifndef __FC8150_TUN__
#define __FC8150_TUN__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8150_tuner_init(HANDLE hDevice, enum band_type band);
extern int fc8150_set_freq(HANDLE hDevice, enum band_type band, u32 f_lo);
extern int fc8150_get_rssi(HANDLE hDevice, int *rssi);
extern int fc8150_tuner_deinit(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif
