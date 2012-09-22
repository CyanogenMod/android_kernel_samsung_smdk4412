/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fci_bypass.c

 Description : fci i2c driver
*******************************************************************************/
#include "fci_bypass.h"
#include "fci_types.h"


int fci_bypass_init(HANDLE hDevice, int speed, int slaveaddr)
{
	return BBM_OK;
}

int fci_bypass_read(HANDLE hDevice, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	int res;

	res = fc8150_bypass_read(hDevice, chip, addr, data, len);

	return res;
}

int fci_bypass_write(HANDLE hDevice, u8 chip, u8 addr
	, u8 alen, u8 *data, u8 len)
{
	int res;

	res = fc8150_bypass_write(hDevice, chip, addr, data, len);

	return res;
}

int fci_bypass_deinit(HANDLE hDevice)
{
	return BBM_OK;
}
