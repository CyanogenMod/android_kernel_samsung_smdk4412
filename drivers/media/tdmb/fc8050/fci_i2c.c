/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fci_i2c.c

	Description : fci i2c driver

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
	2009/09/11	jason		initial
*******************************************************************************/

#include <linux/input.h>


#include "fci_types.h"
#include "fci_oal.h"
#include "fc8050_regs.h"
#include "fci_hal.h"

/* #define FEATURE_FCI_I2C_CHECK_STATUS */

#define I2CSTAT_TIP	0x02	/* Tip bit */
#define I2CSTAT_NACK	0x80	/* Nack bit */

#define I2C_TIMEOUT	1	/* 1 second */

#define I2C_CR_STA		0x80
#define I2C_CR_STO		0x40
#define I2C_CR_RD		0x20
#define I2C_CR_WR		0x10
#define I2C_CR_NACK		0x08
#define I2C_CR_IACK		0x01

#define I2C_WRITE		0
#define I2C_READ		1

#define I2C_OK		0
#define I2C_NOK		1
#define I2C_NACK		2
#define I2C_NOK_LA		3		/* Lost arbitration */
#define I2C_NOK_TOUT		4		/* time out */

static int wait_for_xfer(HANDLE hDevice)
{
	int i;
	int res = I2C_OK;
	u8 status;

	i = I2C_TIMEOUT * 10000;
	/* wait for transfer complete */
	do {
		bbm_read(hDevice, BBM_I2C_SR, &status);
		i--;
	} while ((i > 0) && (status & I2CSTAT_TIP));

	/* check time out or nack */
	if (status & I2CSTAT_TIP)
		res = I2C_NOK_TOUT;

#ifdef FEATURE_FCI_I2C_CHECK_STATUS
	else {
		bbm_read(hDevice, BBM_I2C_SR, &status);
		if (status & I2CSTAT_NACK)
			res = I2C_NACK;
		else
			res = I2C_OK;
	}
#endif
	return res;
}

static int fci_i2c_transfer(
	HANDLE hDevice, u8 cmd_type, u8 chip
	, u8 addr[], u8 addr_len, u8 data[], u8 data_len)
{
	int i;
	int result = I2C_OK;

	switch (cmd_type) {
	case I2C_WRITE:
		bbm_write(hDevice, BBM_I2C_TXR, chip | cmd_type);
		bbm_write(hDevice, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR);
#ifdef FEATURE_FCI_I2C_CHECK_STATUS
		result = wait_for_xfer(hDevice);
		if (result != I2C_OK)
			return result;
#endif
		if (addr && addr_len) {
			i = 0;
			while ((i < addr_len) && (result == I2C_OK)) {
				bbm_write(hDevice, BBM_I2C_TXR, addr[i]);
				bbm_write(hDevice, BBM_I2C_CR, I2C_CR_WR);
#ifdef FEATURE_FCI_I2C_CHECK_STATUS
				result = wait_for_xfer(hDevice);
				if (result != I2C_OK)
					return result;
#endif
				i++;
			}
		}

		i = 0;
		while ((i < data_len) && (result == I2C_OK)) {
			bbm_write(hDevice, BBM_I2C_TXR, data[i]);
			bbm_write(hDevice, BBM_I2C_CR, I2C_CR_WR);
#ifdef FEATURE_FCI_I2C_CHECK_STATUS
			result = wait_for_xfer(hDevice);
			if (result != I2C_OK)
				return result;
#endif
			i++;
		}

		bbm_write(hDevice, BBM_I2C_CR, I2C_CR_STO);
#ifdef FEATURE_FCI_I2C_CHECK_STATUS
		result = wait_for_xfer(hDevice);
		if (result != I2C_OK)
			return result;
#endif
		break;
	case I2C_READ:
		if (addr && addr_len) {
			bbm_write(hDevice, BBM_I2C_TXR, chip | I2C_WRITE);
			bbm_write(hDevice, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR);
#ifdef FEATURE_FCI_I2C_CHECK_STATUS
			result = wait_for_xfer(hDevice);
			if (result != I2C_OK)
				return result;
#endif

			i = 0;
			while ((i < addr_len) && (result == I2C_OK)) {
				bbm_write(hDevice, BBM_I2C_TXR, addr[i]);
				bbm_write(hDevice, BBM_I2C_CR, I2C_CR_WR);
#ifdef FEATURE_FCI_I2C_CHECK_STATUS
				result = wait_for_xfer(hDevice);
				if (result != I2C_OK)
					return result;
#endif
				i++;
			}
		}

		bbm_write(hDevice, BBM_I2C_TXR, chip | I2C_READ);
		bbm_write(hDevice, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR);
#ifdef FEATURE_FCI_I2C_CHECK_STATUS
		result = wait_for_xfer(hDevice);
		if (result != I2C_OK)
			return result;
#endif

		i = 0;
		while ((i < data_len) && (result == I2C_OK)) {
			if (i == data_len - 1) {
				bbm_write(hDevice, BBM_I2C_CR
					, I2C_CR_RD|I2C_CR_NACK);
				result = wait_for_xfer(hDevice);
				if ((result != I2C_NACK)
					&& (result != I2C_OK)) {
					print_log(hDevice, "NACK4-0[%02x]\n"
						, result);
					return result;
				}
			} else {
				bbm_write(hDevice, BBM_I2C_CR, I2C_CR_RD);
				result = wait_for_xfer(hDevice);
				if (result != I2C_OK) {
					print_log(hDevice, "NACK4-1[%02x]\n"
						, result);
					return result;
				}
			}
			bbm_read(hDevice, BBM_I2C_RXR, &data[i]);
			i++;
		}

		bbm_write(hDevice, BBM_I2C_CR, I2C_CR_STO);
#ifdef FEATURE_FCI_I2C_CHECK_STATUS
		result = wait_for_xfer(hDevice);
		if ((result != I2C_NACK) && (result != I2C_OK)) {
			print_log(hDevice, "NACK5[%02X]\n", result);
			return result;
		}
#endif
		break;
	default:
		return I2C_NOK;
	}

	return I2C_OK;
}

int fci_i2c_init(HANDLE hDevice, int speed, int slaveaddr)
{
	u16 pr, rpr = 0;

	pr = (u16)((4800 / speed) - 1);
	/* pr=400; */
	bbm_word_write(hDevice, BBM_I2C_PR, pr);

	bbm_word_read(hDevice, BBM_I2C_PR, &rpr);
	if (pr != rpr)
		return BBM_NOK;

	/* i2c master core enable & interrupt enable */
	bbm_write(hDevice, BBM_I2C_CTR, 0xC0);

	return BBM_OK;
}

int fci_i2c_read(
	HANDLE hDevice, u8 chip, u8 addr, u8 address_len, u8 *data, u8 len)
{
	int ret;
	u8 tmp[4] = {0xcc, 0xcc, 0xcc, 0xcc};

	ret = fci_i2c_transfer(hDevice, I2C_READ, chip << 1
		, &addr, address_len, &tmp[0], len);
	if (ret != I2C_OK) {
		print_log(hDevice
			, "fci_i2c_read() result=%d, addr = %x, data=%x\n"
			, ret, addr, *data);
		return ret;
	}

	/* *data = tmp[0]; */
	memcpy(data, tmp, len);

	return ret;
}

int fci_i2c_write(
	HANDLE hDevice, u8 chip, u8 addr, u8 address_len, u8 *data, u8 len)
{
	int ret;
	u8 *paddr = &addr;

	ret = fci_i2c_transfer(hDevice, I2C_WRITE, chip << 1
		, paddr, address_len, data, len);

	if (ret != I2C_OK)
		print_log(hDevice
			, "fci_i2c_write() result=%d, addr= %x, data=%x\n"
		, ret, addr, *data);

	return ret;
}
