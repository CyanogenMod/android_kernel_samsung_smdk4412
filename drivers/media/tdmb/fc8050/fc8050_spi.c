/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fc8050_spi.c

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
#include <linux/input.h>
#include <linux/spi/spi.h>

#include "fci_types.h"
#include "fc8050_regs.h"
#include "fci_oal.h"
#include "fc8050_spi.h"

#include "tdmb.h"

#define DRIVER_NAME "fc8050_spi"

#define HPIC_READ			0x01	/* read command */
#define HPIC_WRITE			0x02	/* write command */
#define HPIC_AINC			0x04	/* address increment */
#define HPIC_BMODE			0x00	/* byte mode */
#define HPIC_WMODE			0x10	/* word mode */
#define HPIC_LMODE			0x20	/* long mode */
#define HPIC_ENDIAN			0x00	/* little endian */
#define HPIC_CLEAR			0x80	/* currently not used */

#define CHIPID 0
#if (CHIPID == 0)
#define SPI_CMD_WRITE                           0x0
#define SPI_CMD_READ                            0x1
#define SPI_CMD_BURST_WRITE                     0x2
#define SPI_CMD_BURST_READ                      0x3
#else
#define SPI_CMD_WRITE                           0x4
#define SPI_CMD_READ                            0x5
#define SPI_CMD_BURST_WRITE                     0x6
#define SPI_CMD_BURST_READ                      0x7
#endif

struct spi_device *fc8050_spi;

static u8 tx_data[10];

static DEFINE_MUTEX(lock);

int fc8050_spi_write_then_read(
	struct spi_device *spi
	, u8 *txbuf
	, u16 tx_length
	, u8 *rxbuf
	, u16 rx_length)
{
	s32 res;

	struct spi_message message;
	struct spi_transfer	x;

	spi_message_init(&message);
	memset(&x, 0, sizeof x);

	spi_message_add_tail(&x, &message);

	x.tx_buf = txbuf;
	x.rx_buf = txbuf;
	x.len = tx_length + rx_length;

	res = spi_sync(spi, &message);

	memcpy(rxbuf, x.rx_buf + tx_length, rx_length);

	return res;
}

int fc8050_spi_write_then_burstread(
	struct spi_device *spi
	, u8 *txbuf
	, u16 tx_length
	, u8 *rxbuf
	, u16 rx_length)
{
	s32 res;

	struct spi_message	message;
	struct spi_transfer	x;

	spi_message_init(&message);
	memset(&x, 0, sizeof x);

	spi_message_add_tail(&x, &message);

	x.tx_buf = txbuf;
	x.rx_buf = rxbuf;
	x.len = tx_length + rx_length;

	res = spi_sync(spi, &message);

	return res;
}

static int spi_bulkread(HANDLE hDevice, u8 addr, u8 *data, u16 length)
{
	s32 ret;

	tx_data[0] = SPI_CMD_BURST_READ;
	tx_data[1] = addr;

	fc8050_spi = tdmb_get_spi_handle();
	ret = fc8050_spi_write_then_read(
		fc8050_spi, &tx_data[0], 2, &data[0], length);

	if (ret) {
		print_log(0, "fc8050_spi_bulkread fail : %d\n", ret);
		return BBM_NOK;
	}

	return BBM_OK;
}

static int spi_bulkwrite(HANDLE hDevice, u8 addr, u8 *data, u16 length)
{
	s32 ret;
	s32 i;

	tx_data[0] = SPI_CMD_BURST_WRITE;
	tx_data[1] = addr;

	for (i = 0; i < length; i++)
		tx_data[2+i] = data[i];

	fc8050_spi = tdmb_get_spi_handle();
	ret = fc8050_spi_write_then_read(
		fc8050_spi, &tx_data[0], length+2, NULL, 0);

	if (ret) {
		print_log(0, "fc8050_spi_bulkwrite fail : %d\n", ret);
		return BBM_NOK;
	}

	return BBM_OK;
}

static int spi_dataread(HANDLE hDevice, u8 addr, u8 *data, u16 length)
{
	s32 ret = 0;

	tx_data[0] = SPI_CMD_BURST_READ;
	tx_data[1] = addr;

	fc8050_spi = tdmb_get_spi_handle();

	ret = fc8050_spi_write_then_burstread(
		fc8050_spi, &tx_data[0], 2, &data[0], length);

	if (ret) {
		print_log(0, "fc8050_spi_dataread fail : %d\n", ret);
		return BBM_NOK;
	}

	return BBM_OK;
}

int fc8050_spi_init(HANDLE hDevice, u16 param1, u16 param2)
{

	return BBM_OK;
}

int fc8050_spi_byteread(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;
	u8 command = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);
	res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8 *)&addr, 2);
	res |= spi_bulkread(hDevice, BBM_DATA_REG, data, 1);
	mutex_unlock(&lock);

	return res;
}

int fc8050_spi_wordread(HANDLE hDevice, u16 addr, u16 *data)
{
	int res;
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	if (BBM_SCI_DATA <= addr && BBM_SCI_SYNCRX >= addr)
		command = HPIC_READ | HPIC_WMODE | HPIC_ENDIAN;

	mutex_lock(&lock);
	res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8 *)&addr, 2);
	res |= spi_bulkread(hDevice, BBM_DATA_REG, (u8 *)data, 2);
	mutex_unlock(&lock);

	return res;
}

int fc8050_spi_longread(HANDLE hDevice, u16 addr, u32 *data)
{
	int res;
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);
	res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8 *)&addr, 2);
	res |= spi_bulkread(hDevice, BBM_DATA_REG, (u8 *)data, 4);
	mutex_unlock(&lock);

	return res;
}

int fc8050_spi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);
	res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8 *)&addr, 2);
	res |= spi_bulkread(hDevice, BBM_DATA_REG, data, length);
	mutex_unlock(&lock);

	return res;
}

int fc8050_spi_bytewrite(HANDLE hDevice, u16 addr, u8 data)
{
	int res;
	u8 command = HPIC_WRITE | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);
	res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8 *)&addr, 2);
	res |= spi_bulkwrite(hDevice, BBM_DATA_REG, (u8 *)&data, 1);
	mutex_unlock(&lock);

	return res;
}

int fc8050_spi_wordwrite(HANDLE hDevice, u16 addr, u16 data)
{
	int res;
	u8 command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	if (BBM_SCI_DATA <= addr && BBM_SCI_SYNCRX >= addr)
		command = HPIC_WRITE | HPIC_WMODE | HPIC_ENDIAN;

	mutex_lock(&lock);
	res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8 *)&addr, 2);
	res |= spi_bulkwrite(hDevice, BBM_DATA_REG, (u8 *)&data, 2);
	mutex_unlock(&lock);

	return res;
}

int fc8050_spi_longwrite(HANDLE hDevice, u16 addr, u32 data)
{
	int res;
	u8 command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);
	res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8 *)&addr, 2);
	res |= spi_bulkwrite(hDevice, BBM_DATA_REG, (u8 *)&data, 4);
	mutex_unlock(&lock);

	return res;
}

int fc8050_spi_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;
	u8 command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);
	res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8 *)&addr, 2);
	res |= spi_bulkwrite(hDevice, BBM_DATA_REG, data, length);
	mutex_unlock(&lock);

	return res;
}

int fc8050_spi_dataread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;
	u8 command = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);
	res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8 *)&addr, 2);
	res |= spi_dataread(hDevice, BBM_DATA_REG, data, length);
	mutex_unlock(&lock);

	return res;
}

int fc8050_spi_deinit(HANDLE hDevice)
{
	print_log(NULL, "fc8050_spi_deinit\n");
	return BBM_OK;
}
