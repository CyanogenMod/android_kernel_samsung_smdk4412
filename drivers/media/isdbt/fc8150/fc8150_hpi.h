/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8150_hpi.c

 Description : API of 1-SEG baseband module

*******************************************************************************/

#ifndef __FC8150_HPI_H__
#define __FC8150_HPI_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8150_hpi_init(HANDLE hDevice, u16 param1, u16 param2);
extern int fc8150_hpi_byteread(HANDLE hDevice, u16 addr, u8 *data);
extern int fc8150_hpi_wordread(HANDLE hDevice, u16 addr, u16 *data);
extern int fc8150_hpi_longread(HANDLE hDevice, u16 addr, u32 *data);
extern int fc8150_hpi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length);
extern int fc8150_hpi_bytewrite(HANDLE hDevice, u16 addr, u8 data);
extern int fc8150_hpi_wordwrite(HANDLE hDevice, u16 addr, u16 data);
extern int fc8150_hpi_longwrite(HANDLE hDevice, u16 addr, u32 data);
extern int fc8150_hpi_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 length);
extern int fc8150_hpi_dataread(HANDLE hDevice, u16 addr, u8 *data, u32 length);
extern int fc8150_hpi_deinit(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif
