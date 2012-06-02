/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fc8100_hal.c

 Description : fc8100 host interface

 History :
 ----------------------------------------------------------------------
 2009/08/29	jason		initial
*******************************************************************************/

#include "fci_types.h"
#include "fci_oal.h"
#include "bbm.h"
#include "fci_hal.h"
#include "fc8100_i2c.h"

typedef struct {
	int (*init)(HANDLE hDevice, u16 param1, u16 param2);

	int (*byteread)(HANDLE hDevice, u16 addr, u8  *data);
	int (*wordread)(HANDLE hDevice, u16 addr, u16 *data);
	int (*longread)(HANDLE hDevice, u16 addr, u32 *data);
	int (*bulkread)(HANDLE hDevice, u16 addr, u8  *data, u16 size);

	int (*bytewrite)(HANDLE hDevice, u16 addr, u8  data);
	int (*wordwrite)(HANDLE hDevice, u16 addr, u16 data);
	int (*longwrite)(HANDLE hDevice, u16 addr, u32 data);
	int (*bulkwrite)(HANDLE hDevice, u16 addr, u8 *data, u16 size);
	int (*dataread)(HANDLE hDevice, u16 addr, u8 *data, u16 size);

	int (*deinit)(HANDLE hDevice);
} IF_PORT;

static IF_PORT i2cif = {
	&fc8100_i2c_init,

	&fc8100_i2c_byteread,
	&fc8100_i2c_wordread,
	&fc8100_i2c_longread,
	&fc8100_i2c_bulkread,

	&fc8100_i2c_bytewrite,
	&fc8100_i2c_wordwrite,
	&fc8100_i2c_longwrite,
	&fc8100_i2c_bulkwrite,
	&fc8100_spi_dataread,

	&fc8100_i2c_deinit
};

static IF_PORT *ifport = &i2cif;
static u8 hostif_type = BBM_I2C;

int bbm_hostif_get(HANDLE hDevice, u8 *hostif)
{
	*hostif = hostif_type;

	return BBM_OK;
}

int bbm_hostif_select(HANDLE hDevice, u8 hostif)
{
	int  res = BBM_OK;

	hostif_type = hostif;

	switch (hostif) {
#if 0
		case BBM_HPI:
			ifport = &hpiif;
			break;
		case BBM_SPI:
			ifport = &spiif;
			break;
#endif
		case BBM_I2C:
			ifport = &i2cif;
			break;
		default:
			res = BBM_NOK;
			break;
	}

	if (res == BBM_OK)
		ifport->init(hDevice, 0, 0);

	return res;
}

int bbm_hostif_deselect(HANDLE hDevice)
{
	int  res = BBM_OK;

	ifport->deinit(hDevice);

	hostif_type = 0;
	ifport = NULL;

	return res;
}

int bbm_read(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;

	res  = ifport->byteread(hDevice, addr, data);

	return res;
}

int bbm_byte_read(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;

	res  = ifport->byteread(hDevice, addr, data);

	return res;
}

int bbm_word_read(HANDLE hDevice, u16 addr, u16 *data)
{
	int res;

	res  = ifport->wordread(hDevice, addr, data);

	return res;
}

int bbm_long_read(HANDLE hDevice, u16 addr, u32 *data)
{
	int res;

	res  = ifport->longread(hDevice, addr, data);

	return res;
}

int bbm_bulk_read(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;

	res  = ifport->bulkread(hDevice, addr, data, length);

	return res;
}

int bbm_write(HANDLE hDevice, u16 addr, u8 data)
{
	int res;

	res  = ifport->bytewrite(hDevice, addr, data);

	return res;
}

int bbm_byte_write(HANDLE hDevice, u16 addr, u8 data)
{
	int res;

	res  = ifport->bytewrite(hDevice, addr, data);

	return res;
}

int bbm_word_write(HANDLE hDevice, u16 addr, u16 data)
{
	int res;

	res  = ifport->wordwrite(hDevice, addr, data);

	return res;
}

int bbm_long_write(HANDLE hDevice, u16 addr, u32 data)
{
	int res;

	res  = ifport->longwrite(hDevice, addr, data);

	return res;
}

int bbm_bulk_write(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;

	res  = ifport->bulkwrite(hDevice, addr, data, length);

	return res;
}

int bbm_data(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res = BBM_NOK;
	res = ifport->dataread(hDevice, addr, data, length);
	return res;
}
