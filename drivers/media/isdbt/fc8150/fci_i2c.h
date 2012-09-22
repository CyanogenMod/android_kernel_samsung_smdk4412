/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fci_i2c.h

 Description : fci i2c driver header
*******************************************************************************/

#ifndef __FCI_I2C_H__
#define __FCI_I2C_H__

#include "fci_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int fci_i2c_init(HANDLE hDevice, int speed, int slaveaddr);
extern int fci_i2c_read(HANDLE hDevice, u8 chip, u8 addr
	, u8 alen, u8 *data, u8 len);
extern int fci_i2c_write(HANDLE hDevice, u8 chip, u8 addr
	, u8 alen, u8 *data, u8 len);
extern int fci_i2c_deinit(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif
