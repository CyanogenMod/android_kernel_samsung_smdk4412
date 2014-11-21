/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8151_tun.c

 Description : fc8151 tuner control driver

*******************************************************************************/

#ifndef __FC8151_TUN__
#define __FC8151_TUN__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8151_tuner_init(HANDLE hDevice, band_type band);
extern int fc8151_set_freq(HANDLE hDevice, band_type band, u32 f_lo);
extern int fc8151_get_rssi(HANDLE hDevice, int *rssi);
extern int fc8151_tuner_deinit(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif
