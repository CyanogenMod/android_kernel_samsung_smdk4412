/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fci_hal.c

	Description : fc8050 host interface

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA


	History :
	----------------------------------------------------------------------
	2009/08/29	jason		initial
*******************************************************************************/

#include "fci_types.h"
#include "bbm.h"
#include "fci_hal.h"
/* #include "fc8050_hpi.h" */
#include "fc8050_i2c.h"
#if defined(CONFIG_TDMB_SPI)
#include "fc8050_spi.h"
#endif
#if defined(CONFIG_TDMB_EBI)
#include "fc8050_ppi.h"
#endif

#define FEATURE_INTERFACE_DEBUG
struct interface_port {
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
} ;

static struct interface_port ifport;
static u8 hostif_type = BBM_SPI;

int bbm_hostif_get(HANDLE hDevice, u8 *hostif)
{
	*hostif = hostif_type;

	return BBM_OK;
}

int bbm_hostif_select(HANDLE hDevice, u8 hostif)
{
	hostif_type = hostif;

	switch (hostif) {
#if defined(CONFIG_TDMB_SPI)
	case BBM_SPI:
		ifport.init = fc8050_spi_init;
		ifport.byteread = fc8050_spi_byteread;
		ifport.wordread = fc8050_spi_wordread;
		ifport.longread = fc8050_spi_longread;
		ifport.bulkread = fc8050_spi_bulkread;

		ifport.bytewrite = fc8050_spi_bytewrite;
		ifport.wordwrite = fc8050_spi_wordwrite;
		ifport.longwrite = fc8050_spi_longwrite;
		ifport.bulkwrite = fc8050_spi_bulkwrite;

		ifport.dataread = fc8050_spi_dataread;

		ifport.deinit = fc8050_spi_deinit;
		break;
#endif
#ifndef FEATURE_INTERFACE_DEBUG
	case BBM_HPI:
		ifport.init = fc8050_hpi_init;
		ifport.byteread = fc8050_hpi_byteread;
		ifport.wordread = fc8050_hpi_wordread;
		ifport.longread = fc8050_hpi_longread;
		ifport.bulkread = fc8050_hpi_bulkread;

		ifport.bytewrite = fc8050_hpi_bytewrite;
		ifport.wordwrite = fc8050_hpi_wordwrite;
		ifport.longwrite = fc8050_hpi_longwrite;
		ifport.bulkwrite = fc8050_hpi_bulkwrite;

		ifport.dataread = fc8050_hpi_dataread;

		ifport.deinit = fc8050_hpi_deinit;
		break;
	case BBM_I2C:
		ifport.init = fc8050_i2c_init;
		ifport.byteread = fc8050_i2c_byteread;
		ifport.wordread = fc8050_i2c_wordread;
		ifport.longread = fc8050_i2c_longread;
		ifport.bulkread = fc8050_i2c_bulkread;

		ifport.bytewrite = fc8050_i2c_bytewrite;
		ifport.wordwrite = fc8050_i2c_wordwrite;
		ifport.longwrite = fc8050_i2c_longwrite;
		ifport.bulkwrite = fc8050_i2c_bulkwrite;

		ifport.dataread = fc8050_i2c_dataread;

		ifport.deinit = fc8050_i2c_deinit;
		break;
#endif
#if defined(CONFIG_TDMB_EBI)
	case BBM_PPI:
		ifport.init = fc8050_ppi_init;
		ifport.byteread = fc8050_ppi_byteread;
		ifport.wordread = fc8050_ppi_wordread;
		ifport.longread = fc8050_ppi_longread;
		ifport.bulkread = fc8050_ppi_bulkread;

		ifport.bytewrite = fc8050_ppi_bytewrite;
		ifport.wordwrite = fc8050_ppi_wordwrite;
		ifport.longwrite = fc8050_ppi_longwrite;
		ifport.bulkwrite = fc8050_ppi_bulkwrite;

		ifport.dataread = fc8050_ppi_dataread;

		ifport.deinit = fc8050_ppi_deinit;
		break;
#endif
	default:
		return BBM_E_HOSTIF_SELECT;
	}

	if (ifport.init(hDevice, 0, 0))
		return BBM_E_HOSTIF_INIT;

	return BBM_OK;
}

int bbm_hostif_deselect(HANDLE hDevice)
{
	if (ifport.deinit(hDevice))
		return BBM_NOK;

	hostif_type = BBM_HPI;

	return BBM_OK;
}

int bbm_read(HANDLE hDevice, u16 addr, u8 *data)
{
	if (ifport.byteread(hDevice, addr, data))
		return BBM_E_BB_REG_READ;
	return BBM_OK;
}

int bbm_byte_read(HANDLE hDevice, u16 addr, u8 *data)
{
	if (ifport.byteread(hDevice, addr, data))
		return BBM_E_BB_REG_READ;
	return BBM_OK;
}

int bbm_word_read(HANDLE hDevice, u16 addr, u16 *data)
{
	if (ifport.wordread(hDevice, addr, data))
		return BBM_E_BB_REG_READ;
	return BBM_OK;
}

int bbm_long_read(HANDLE hDevice, u16 addr, u32 *data)
{
	if (ifport.longread(hDevice, addr, data))
		return BBM_E_BB_REG_READ;
	return BBM_OK;
}

int bbm_bulk_read(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	if (ifport.bulkread(hDevice, addr, data, length))
		return BBM_E_BB_REG_READ;
	return BBM_OK;
}

int bbm_write(HANDLE hDevice, u16 addr, u8 data)
{
	if (ifport.bytewrite(hDevice, addr, data))
		return BBM_E_BB_REG_WRITE;
	return BBM_OK;
}

int bbm_byte_write(HANDLE hDevice, u16 addr, u8 data)
{
	if (ifport.bytewrite(hDevice, addr, data))
		return BBM_E_BB_REG_WRITE;
	return BBM_OK;
}

int bbm_word_write(HANDLE hDevice, u16 addr, u16 data)
{
	if (ifport.wordwrite(hDevice, addr, data))
		return BBM_E_BB_REG_WRITE;
	return BBM_OK;
}

int bbm_long_write(HANDLE hDevice, u16 addr, u32 data)
{
	if (ifport.longwrite(hDevice, addr, data))
		return BBM_E_BB_REG_WRITE;
	return BBM_OK;
}

int bbm_bulk_write(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	if (ifport.bulkwrite(hDevice, addr, data, length))
		return BBM_E_BB_REG_WRITE;
	return BBM_OK;
}

int bbm_data(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	if (ifport.dataread(hDevice, addr, data, length))
		return BBM_E_BB_REG_WRITE;
	return BBM_OK;
}
