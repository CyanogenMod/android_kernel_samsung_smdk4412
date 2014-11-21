/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fc8100_i2c.h

 Description : API of dmb baseband module

 History :
 ----------------------------------------------------------------------
 2009/10/01	bruce		initial
*******************************************************************************/

#ifndef __FC8100_I2C__
#define __FC8100_I2C__

#ifdef __cplusplus
extern "C" {
#endif

extern int fc8100_i2c_init(HANDLE hDevice, u16 param1, u16 param2);

extern int fc8100_i2c_byteread(HANDLE hDevice, u16 param1, u8 *param2);
extern int fc8100_i2c_wordread(HANDLE hDevice, u16 param1, u16 *param2);
extern int fc8100_i2c_longread(HANDLE hDevice, u16 param1, u32 *param2);
extern int fc8100_i2c_bulkread(HANDLE hDevice, u16 param1, u8 *param2, u16 param3);
extern int fc8100_i2c_bytewrite(HANDLE hDevice, u16 param1, u8 param2);
extern int fc8100_i2c_wordwrite(HANDLE hDevice, u16 param1, u16 param2);
extern int fc8100_i2c_longwrite(HANDLE hDevice, u16 param1, u32 param2);
extern int fc8100_i2c_bulkwrite(HANDLE hDevice, u16 param1, u8 *param2, u16 param3);

extern int fc8100_i2c_deinit(HANDLE hDevice);

extern int fc8100_rf_bulkread(HANDLE hDevice, u8 addr, u8 *data, u8 len);
extern int fc8100_rf_bulkwrite(HANDLE hDevice, u8 addr, u8 *data, u8 len);

extern int fc8100_spi_init(HANDLE hDevice, u16 param1, u16 param2);
extern int fc8100_spi_dataread(HANDLE hDevice, u16 addr, u8 *pBuf, u16 nLength);

#ifdef __cplusplus
}
#endif

#endif
