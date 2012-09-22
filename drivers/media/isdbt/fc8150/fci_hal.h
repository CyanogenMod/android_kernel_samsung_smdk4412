/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8150_hal.h

 Description : fc8150 host interface
*******************************************************************************/

#ifndef __FCI_HAL_H__
#define __FCI_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int bbm_hostif_select(HANDLE hDevice, u8 hostif);
extern int bbm_hostif_deselect(HANDLE hDevice);

extern int bbm_read(HANDLE hDevice, u16 addr, u8 *data);
extern int bbm_byte_read(HANDLE hDevice, u16 addr, u8 *data);
extern int bbm_word_read(HANDLE hDevice, u16 addr, u16 *data);
extern int bbm_long_read(HANDLE hDevice, u16 addr, u32 *data);
extern int bbm_bulk_read(HANDLE hDevice, u16 addr, u8 *data, u16 length);

extern int bbm_write(HANDLE hDevice, u16 addr, u8 data);
extern int bbm_byte_write(HANDLE hDevice, u16 addr, u8 data);
extern int bbm_word_write(HANDLE hDevice, u16 addr, u16 data);
extern int bbm_long_write(HANDLE hDevice, u16 addr, u32 data);
extern int bbm_bulk_write(HANDLE hDevice, u16 addr, u8 *data, u16 length);

extern int bbm_data(HANDLE hDevice, u16 addr, u8 *data, u32 length);

#ifdef __cplusplus
}
#endif

#endif
