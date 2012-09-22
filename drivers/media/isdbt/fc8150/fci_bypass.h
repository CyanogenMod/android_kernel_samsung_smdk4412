/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fci_bypass.h

 Description : fci i2c driver header
*******************************************************************************/

#ifndef __FCI_BYPASS_H__
#define __FCI_BYPASS_H__

#include "fci_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int fci_bypass_init(HANDLE hDevice, int speed, int slaveaddr);
extern int fci_bypass_read(HANDLE hDevice, u8 chip, u8 addr
	, u8 alen, u8 *data, u8 len);
extern int fci_bypass_write(HANDLE hDevice, u8 chip, u8 addr
	, u8 alen, u8 *data, u8 len);
extern int fci_bypass_deinit(HANDLE hDevice);

extern int fc8150_bypass_read(HANDLE hDevice, u8 chip
	, u8 addr, u8 *data, u16 length);
extern int fc8150_bypass_write(HANDLE hDevice, u8 chip
	, u8 addr, u8 *data, u16 length);

#ifdef __cplusplus
}
#endif

#endif
