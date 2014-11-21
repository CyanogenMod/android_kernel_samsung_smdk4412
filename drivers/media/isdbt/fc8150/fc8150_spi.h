/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8150_spi.c

 Description : API of 1-SEG baseband module

*******************************************************************************/

#ifndef __FC8150_SPI__
#define __FC8150_SPI__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8150_spi_init(HANDLE hDevice, u16 param1, u16 param2);
extern int fc8150_spi_byteread(HANDLE hDevice, u16 addr, u8 *data);
extern int fc8150_spi_wordread(HANDLE hDevice, u16 addr, u16 *data);
extern int fc8150_spi_longread(HANDLE hDevice, u16 addr, u32 *data);
extern int fc8150_spi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length);
extern int fc8150_spi_bytewrite(HANDLE hDevice, u16 addr, u8 data);
extern int fc8150_spi_wordwrite(HANDLE hDevice, u16 addr, u16 data);
extern int fc8150_spi_longwrite(HANDLE hDevice, u16 addr, u32 data);
extern int fc8150_spi_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 length);
extern int fc8150_spi_dataread(HANDLE hDevice, u16 addr, u8 *data, u32 length);
extern int fc8150_spi_deinit(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif
