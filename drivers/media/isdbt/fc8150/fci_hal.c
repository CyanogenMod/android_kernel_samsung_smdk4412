/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fci_hal.c

 Description : fc8150 host interface

*******************************************************************************/

#include "fci_types.h"
#include "fci_hal.h"
#include "fc8150_hpi.h"
#include "fc8150_spi.h"
#include "fc8150_ppi.h"
#include "fc8150_i2c.h"
#include "fc8150_spib.h"

struct IF_PORT {
	int (*init)(HANDLE hDevice, u16 param1, u16 param2);

	int (*byteread)(HANDLE hDevice, u16 addr, u8  *data);
	int (*wordread)(HANDLE hDevice, u16 addr, u16 *data);
	int (*longread)(HANDLE hDevice, u16 addr, u32 *data);
	int (*bulkread)(HANDLE hDevice, u16 addr, u8  *data, u16 length);

	int (*bytewrite)(HANDLE hDevice, u16 addr, u8  data);
	int (*wordwrite)(HANDLE hDevice, u16 addr, u16 data);
	int (*longwrite)(HANDLE hDevice, u16 addr, u32 data);
	int (*bulkwrite)(HANDLE hDevice, u16 addr, u8 *data, u16 length);

	int (*dataread)(HANDLE hDevice, u16 addr, u8 *data, u32 length);

	int (*deinit)(HANDLE hDevice);
};

static struct IF_PORT hpiif = {
	&fc8150_hpi_init,

	&fc8150_hpi_byteread,
	&fc8150_hpi_wordread,
	&fc8150_hpi_longread,
	&fc8150_hpi_bulkread,

	&fc8150_hpi_bytewrite,
	&fc8150_hpi_wordwrite,
	&fc8150_hpi_longwrite,
	&fc8150_hpi_bulkwrite,

	&fc8150_hpi_dataread,

	&fc8150_hpi_deinit
};

static struct IF_PORT spiif = {
	&fc8150_spi_init,

	&fc8150_spi_byteread,
	&fc8150_spi_wordread,
	&fc8150_spi_longread,
	&fc8150_spi_bulkread,

	&fc8150_spi_bytewrite,
	&fc8150_spi_wordwrite,
	&fc8150_spi_longwrite,
	&fc8150_spi_bulkwrite,

	&fc8150_spi_dataread,

	&fc8150_spi_deinit
};

static struct IF_PORT spibif = {
	&fc8150_spib_init,

	&fc8150_spib_byteread,
	&fc8150_spib_wordread,
	&fc8150_spib_longread,
	&fc8150_spib_bulkread,

	&fc8150_spib_bytewrite,
	&fc8150_spib_wordwrite,
	&fc8150_spib_longwrite,
	&fc8150_spib_bulkwrite,

	&fc8150_spib_dataread,

	&fc8150_spib_deinit
};

static struct IF_PORT ppiif = {
	&fc8150_ppi_init,

	&fc8150_ppi_byteread,
	&fc8150_ppi_wordread,
	&fc8150_ppi_longread,
	&fc8150_ppi_bulkread,

	&fc8150_ppi_bytewrite,
	&fc8150_ppi_wordwrite,
	&fc8150_ppi_longwrite,
	&fc8150_ppi_bulkwrite,

	&fc8150_ppi_dataread,

	&fc8150_ppi_deinit
};

static struct IF_PORT i2cif = {
	&fc8150_i2c_init,

	&fc8150_i2c_byteread,
	&fc8150_i2c_wordread,
	&fc8150_i2c_longread,
	&fc8150_i2c_bulkread,

	&fc8150_i2c_bytewrite,
	&fc8150_i2c_wordwrite,
	&fc8150_i2c_longwrite,
	&fc8150_i2c_bulkwrite,

	&fc8150_i2c_dataread,

	&fc8150_i2c_deinit
};

static struct IF_PORT *ifport = &spiif;
u8 hostif_type = BBM_SPI;

int bbm_hostif_select(HANDLE hDevice, u8 hostif)
{
	hostif_type = hostif;

	switch (hostif) {
	case BBM_HPI:
		ifport = &hpiif;
		break;
	case BBM_SPI:
		ifport = &spiif;
		break;
	case BBM_I2C:
		ifport = &i2cif;
		break;
	case BBM_PPI:
		ifport = &ppiif;
		break;
	case BBM_SPIB:
		ifport = &spibif;
		break;
	default:
		return BBM_E_HOSTIF_SELECT;
	}

	if (ifport->init(hDevice, 0, 0))
		return BBM_E_HOSTIF_INIT;

	return BBM_OK;
}

int bbm_hostif_deselect(HANDLE hDevice)
{
	if (ifport->deinit(hDevice))
		return BBM_NOK;

	ifport = NULL;
	hostif_type = BBM_SPI;

	return BBM_OK;
}

int bbm_read(HANDLE hDevice, u16 addr, u8 *data)
{
	if (ifport->byteread(hDevice, addr, data))
		return BBM_E_BB_READ;
	return BBM_OK;
}

int bbm_byte_read(HANDLE hDevice, u16 addr, u8 *data)
{
	if (ifport->byteread(hDevice, addr, data))
		return BBM_E_BB_READ;
	return BBM_OK;
}

int bbm_word_read(HANDLE hDevice, u16 addr, u16 *data)
{
	if (ifport->wordread(hDevice, addr, data))
		return BBM_E_BB_READ;
	return BBM_OK;
}

int bbm_long_read(HANDLE hDevice, u16 addr, u32 *data)
{
	if (ifport->longread(hDevice, addr, data))
		return BBM_E_BB_READ;
	return BBM_OK;
}

int bbm_bulk_read(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	if (ifport->bulkread(hDevice, addr, data, length))
		return BBM_E_BB_READ;
	return BBM_OK;
}

int bbm_write(HANDLE hDevice, u16 addr, u8 data)
{
	if (ifport->bytewrite(hDevice, addr, data))
		return BBM_E_BB_WRITE;
	return BBM_OK;
}

int bbm_byte_write(HANDLE hDevice, u16 addr, u8 data)
{
	if (ifport->bytewrite(hDevice, addr, data))
		return BBM_E_BB_WRITE;
	return BBM_OK;
}

int bbm_word_write(HANDLE hDevice, u16 addr, u16 data)
{
	if (ifport->wordwrite(hDevice, addr, data))
		return BBM_E_BB_WRITE;
	return BBM_OK;
}

int bbm_long_write(HANDLE hDevice, u16 addr, u32 data)
{
	if (ifport->longwrite(hDevice, addr, data))
		return BBM_E_BB_WRITE;
	return BBM_OK;
}

int bbm_bulk_write(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	if (ifport->bulkwrite(hDevice, addr, data, length))
		return BBM_E_BB_WRITE;
	return BBM_OK;
}

int bbm_data(HANDLE hDevice, u16 addr, u8 *data, u32 length)
{
	if (ifport->dataread(hDevice, addr, data, length))
		return BBM_E_BB_WRITE;
	return BBM_OK;
}
