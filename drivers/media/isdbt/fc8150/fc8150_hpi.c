/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8150_hpi.c

 Description : fc8150 host interface

*******************************************************************************/
#include "linux/io.h"

#include "fci_types.h"
#include "fc8150_regs.h"
#include "fci_oal.h"

#define HPIC_READ           0x01 /* read command */
#define HPIC_WRITE          0x02 /* write command */
#define HPIC_AINC           0x04 /* address increment */
#define HPIC_BMODE          0x00 /* byte mode */
#define HPIC_WMODE          0x10 /* word mode */
#define HPIC_LMODE          0x20 /* long mode */
#define HPIC_ENDIAN         0x00 /* little endian */
#define HPIC_CLEAR          0x80 /* currently not used */

#define BBM_BASE_ADDR       0
#define BBM_BASE_OFFSET     0

#define FC8150_ADDR_REG(x)     outb(x, (BBM_BASE_ADDR	\
				+ (BBM_ADDRESS_REG << BBM_BASE_OFFSET)))
#define FC8150_CMD_REG(x)     outb(x, (BBM_BASE_ADDR	\
				+ (BBM_COMMAND_REG << BBM_BASE_OFFSET)))
#define FC8150_DATA_REG_OUT(x)     outb(x, (BBM_BASE_ADDR	\
				+ (BBM_DATA_REG << BBM_BASE_OFFSET)))
#define FC8150_DATA_REG_IN     inb(BBM_BASE_ADDR	\
				+ (BBM_DATA_REG << BBM_BASE_OFFSET))

int fc8150_hpi_init(HANDLE hDevice, u16 param1, u16 param2)
{
	OAL_CREATE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_hpi_byteread(HANDLE hDevice, u16 addr, u8 *data)
{
	u8 command = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_ADDR_REG(addr & 0xff);
	FC8150_ADDR_REG((addr & 0xff00) >> 8);
	FC8150_CMD_REG(command);

	*data = FC8150_DATA_REG_IN;
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_hpi_wordread(HANDLE hDevice, u16 addr, u16 *data)
{
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_ADDR_REG(addr & 0xff);
	FC8150_ADDR_REG((addr & 0xff00) >> 8);
	FC8150_CMD_REG(command);

	*data = FC8150_DATA_REG_IN;
	*data |= FC8150_DATA_REG_IN << 8;
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_hpi_longread(HANDLE hDevice, u16 addr, u32 *data)
{
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_ADDR_REG(addr & 0xff);
	FC8150_ADDR_REG((addr & 0xff00) >> 8);
	FC8150_CMD_REG(command);

	*data = FC8150_DATA_REG_IN;
	*data |= FC8150_DATA_REG_IN << 8;
	*data |= FC8150_DATA_REG_IN << 16;
	*data |= FC8150_DATA_REG_IN << 24;
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_hpi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	s32 i;
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_ADDR_REG(addr & 0xff);
	FC8150_ADDR_REG((addr & 0xff00) >> 8);
	FC8150_CMD_REG(command);

	for (i = 0; i < length; i++)
		data[i] = FC8150_DATA_REG_IN;

	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_hpi_bytewrite(HANDLE hDevice, u16 addr, u8 data)
{
	u8 command = HPIC_WRITE | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_ADDR_REG(addr & 0xff);
	FC8150_ADDR_REG((addr & 0xff00) >> 8);
	FC8150_CMD_REG(command);
	FC8150_DATA_REG_OUT(data);
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_hpi_wordwrite(HANDLE hDevice, u16 addr, u16 data)
{
	u8 command = HPIC_WRITE | HPIC_BMODE | HPIC_ENDIAN | HPIC_AINC;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_ADDR_REG(addr & 0xff);
	FC8150_ADDR_REG((addr & 0xff00) >> 8);
	FC8150_CMD_REG(command);
	FC8150_DATA_REG_OUT(data & 0xff);
	FC8150_DATA_REG_OUT((data & 0xff00) >> 8);
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_hpi_longwrite(HANDLE hDevice, u16 addr, u32 data)
{
	u8 command = HPIC_WRITE | HPIC_BMODE | HPIC_ENDIAN | HPIC_AINC;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_ADDR_REG(addr & 0xff);
	FC8150_ADDR_REG((addr & 0xff00) >> 8);
	FC8150_CMD_REG(command);

	FC8150_DATA_REG_OUT(data & 0xff);
	FC8150_DATA_REG_OUT((data & 0xff00) >> 8);
	FC8150_DATA_REG_OUT((data & 0xff0000) >> 16);
	FC8150_DATA_REG_OUT((data & 0xff000000) >> 24);
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_hpi_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	s32 i;
	u8 command = HPIC_WRITE | HPIC_BMODE | HPIC_ENDIAN | HPIC_AINC;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_ADDR_REG(addr & 0xff);
	FC8150_ADDR_REG((addr & 0xff00) >> 8);
	FC8150_CMD_REG(command);
	for (i = 0; i < length; i++)
		FC8150_DATA_REG_OUT(data[i]);

	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_hpi_dataread(HANDLE hDevice, u16 addr, u8 *data, u32 length)
{
	s32 i;
	u8 command = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_ADDR_REG(addr & 0xff);
	FC8150_ADDR_REG((addr & 0xff00) >> 8);
	FC8150_CMD_REG(command);

	for (i = 0; i < length; i++)
		data[i] = FC8150_DATA_REG_IN;

	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_hpi_deinit(HANDLE hDevice)
{
	OAL_DELETE_SEMAPHORE();

	return BBM_OK;
}
