/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fci_i2c.c

 Description : fci i2c repeater mapping driver for RF register control

 History :
 ----------------------------------------------------------------------
 2009/09/29	bruce		initial
*******************************************************************************/

#include "fci_types.h"
#include "fc8100_i2c.h"

int fci_i2c_init (HANDLE hDevice, int speed, int slaveaddr)
{
	return BBM_OK;
}

int fci_i2c_deinit (HANDLE hDevice)
{
	return BBM_OK;
}

int fci_i2c_read(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	int res = BBM_NOK;

	res = fc8100_rf_bulkread(hDevice, addr, data, len);

	return res;
}

int fci_i2c_write(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	int res = BBM_NOK;

	res = fc8100_rf_bulkwrite(hDevice, addr, data, len);

	return res;
}
