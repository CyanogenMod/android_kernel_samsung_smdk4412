/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8150_spib.c

 Description : fc8150 host interface

*******************************************************************************/
#include "fci_types.h"
#include "fc8150_regs.h"
#include "fci_oal.h"

#define SPI_BMODE           0x00
#define SPI_WMODE           0x10
#define SPI_LMODE           0x20
#define SPI_READ            0x40
#define SPI_WRITE           0x00
#define SPI_AINC            0x80
#define CHIPID              (0 << 3)

static int spi_bulkread(HANDLE hDevice, u16 addr
, u8 command, u8 *data, u16 length)
{
	/*unsigned char *cmd;

	cmd = g_SpiCmd;

	cmd[0] = addr & 0xff;
	cmd[1] = (addr >> 8) & 0xff;
	cmd[2] = (command & 0xf0) | CHIPID | ((length >> 16) & 0x07);
	cmd[3] = (length >> 8) & 0xff;
	cmd[4] = length & 0xff;

	spi_cmd.pCmd			= cmd;
	spi_cmd.cmdSize			= 5;
	spi_cmd.pData			= g_SpiData;
	spi_cmd.dataSize		= length;

	// Send Command and data through the SPI
	if (SPID_SendCommand_ByteRead(&spid, &spi_cmd))
		return BBM_NOK;

	memcpy(data, g_SpiData, length);*/

	return BBM_OK;
}

static int spi_bulkwrite(HANDLE hDevice, u16 addr
	, u8 command, u8 *data, u16 length)
{
	/*unsigned char *cmd;

	cmd = g_SpiCmd;

	cmd[0] = addr & 0xff;
	cmd[1] = (addr >> 8) & 0xff;
	cmd[2] = (command & 0xf0) | CHIPID | ((length >> 16) & 0x07);
	cmd[3] = (length >> 8) & 0xff;
	cmd[4] = length & 0xff;

	spi_cmd.pCmd			= cmd;
	spi_cmd.cmdSize			= 5;
	spi_cmd.pData			= g_SpiData;
	memcpy(g_SpiData, data, length);
	spi_cmd.dataSize		= length;

	// Send Command and data through the SPI
	if (SPID_SendCommand_ByteWrite(&spid, &spi_cmd))
		return BBM_NOK;*/

	return BBM_OK;
}

static int spi_dataread(HANDLE hDevice, u16 addr
	, u8 command, u8 *data, u32 length)
{
	/*unsigned char *cmd;

	cmd = g_SpiCmd;

	cmd[0] = addr & 0xff;
	cmd[1] = (addr >> 8) & 0xff;
	cmd[2] = (command & 0xf0) | CHIPID | ((length >> 16) & 0x07);
	cmd[3] = (length >> 8) & 0xff;
	cmd[4] = length & 0xff;

	spi_cmd.pCmd			= cmd;
	spi_cmd.cmdSize			= 5;
	spi_cmd.pData			= data;
	spi_cmd.dataSize		= length;

	// Send Command and data through the SPI
	if (SPID_SendCommand_ByteRead(&spid, &spi_cmd))
		return BBM_NOK;*/

	return BBM_OK;
}

int fc8150_spib_init(HANDLE hDevice, u16 param1, u16 param2)
{
	OAL_CREATE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_spib_byteread(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;
	u8 command = SPI_READ;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkread(hDevice, addr, command, data, 1);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

int fc8150_spib_wordread(HANDLE hDevice, u16 addr, u16 *data)
{
	int res;
	u8 command = SPI_READ | SPI_AINC;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkread(hDevice, addr, command, (u8 *)data, 2);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

int fc8150_spib_longread(HANDLE hDevice, u16 addr, u32 *data)
{
	int res;
	u8 command = SPI_READ | SPI_AINC;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkread(hDevice, addr, command, (u8 *)data, 4);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

int fc8150_spib_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;
	u8 command = SPI_READ | SPI_AINC;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkread(hDevice, addr, command, data, length);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

int fc8150_spib_bytewrite(HANDLE hDevice, u16 addr, u8 data)
{
	int res;
	u8 command = SPI_WRITE;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkwrite(hDevice, addr, command, (u8 *)&data, 1);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

int fc8150_spib_wordwrite(HANDLE hDevice, u16 addr, u32 data)
{
	int res;
	u8 command = SPI_WRITE | SPI_AINC;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkwrite(hDevice, addr, command, (u8 *)&data, 2);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

int fc8150_spib_longwrite(HANDLE hDevice, u16 addr, u32 data)
{
	int res;
	u8 command = SPI_WRITE | SPI_AINC;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkwrite(hDevice, addr, command, (u8 *)&data, 4);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

int fc8150_spib_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;
	u8 command = SPI_WRITE | SPI_AINC;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkwrite(hDevice, addr, command, data, length);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

int fc8150_spib_dataread(HANDLE hDevice, u16 addr, u8 *data, u32 length)
{
	int res;
	u8 command = SPI_READ;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_dataread(hDevice, addr, command, data, length);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

int fc8150_spib_deinit(HANDLE hDevice)
{
	OAL_DELETE_SEMAPHORE();

	return BBM_OK;
}
