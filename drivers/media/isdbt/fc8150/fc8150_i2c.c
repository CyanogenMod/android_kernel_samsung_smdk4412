/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8150_i2c.c

 Description : fc8150 host interface

*******************************************************************************/

#include "fci_types.h"
#include "fc8150_regs.h"
#include "fci_oal.h"
#include "fci_hal.h"

#define HPIC_READ           0x01 /* read command */
#define HPIC_WRITE          0x02 /* write command */
#define HPIC_AINC           0x04 /* address increment */
#define HPIC_BMODE          0x00 /* byte mode */
#define HPIC_WMODE          0x10 /* word mode */
#define HPIC_LMODE          0x20 /* long mode */
#define HPIC_ENDIAN         0x00 /* little endian */
#define HPIC_CLEAR          0x80 /* currently not used */

#define CHIP_ADDR           0x58

static int i2c_bulkread(HANDLE hDevice, u8 chip, u8 addr, u8 *data, u16 length)
{
	/* Write your own i2c driver code here for read operation. */

	return BBM_OK;
}

static int i2c_bulkwrite(HANDLE hDevice, u8 chip, u8 addr, u8 *data, u16 length)
{
	/* Write your own i2c driver code here for Write operation. */

	return BBM_OK;
}

static int i2c_dataread(HANDLE hDevice, u8 chip, u8 addr, u8 *data, u32 length)
{
	return i2c_bulkread(hDevice, chip, addr, data, length);
}

int fc8150_bypass_read(HANDLE hDevice, u8 chip, u8 addr, u8 *data, u16 length)
{
	int res;
	u8 bypass_addr = 0x03;
	u8 bypass_data = 1;
	u8 bypass_len  = 1;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, bypass_addr
		, &bypass_data, bypass_len);
	res |= i2c_bulkread(hDevice, chip, addr, data, length);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

int fc8150_bypass_write(HANDLE hDevice, u8 chip, u8 addr, u8 *data, u16 length)
{
	int res;
	u8 bypass_addr = 0x03;
	u8 bypass_data = 1;
	u8 bypass_len  = 1;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, bypass_addr
		, &bypass_data, bypass_len);
	res |= i2c_bulkwrite(hDevice, chip, addr, data, length);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

int fc8150_i2c_init(HANDLE hDevice, u16 param1, u16 param2)
{
	OAL_CREATE_SEMAPHORE();

	/* for TSIF, you can call here your own TSIF initialization function. */
	/* tsif_initialize(); */

	bbm_write(hDevice, BBM_TS_CLK_DIV, 0x04);
	bbm_write(hDevice, BBM_TS_PAUSE, 0x80);

	bbm_write(hDevice, BBM_TS_CTRL, 0x02);
	bbm_write(hDevice, BBM_TS_SEL, 0x84);

	return BBM_OK;
}

int fc8150_i2c_byteread(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;
	u8 command = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkread(hDevice, CHIP_ADDR, BBM_DATA_REG, data, 1);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

int fc8150_i2c_wordread(HANDLE hDevice, u16 addr, u16 *data)
{
	int res;
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkread(hDevice, CHIP_ADDR, BBM_DATA_REG, (u8 *)data, 2);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

int fc8150_i2c_longread(HANDLE hDevice, u16 addr, u32 *data)
{
	int res;
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkread(hDevice, CHIP_ADDR, BBM_DATA_REG, (u8 *)data, 4);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

int fc8150_i2c_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkread(hDevice, CHIP_ADDR, BBM_DATA_REG, data, length);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

int fc8150_i2c_bytewrite(HANDLE hDevice, u16 addr, u8 data)
{
	int res;
	u8 command = HPIC_WRITE | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_DATA_REG, (u8 *)&data, 1);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

int fc8150_i2c_wordwrite(HANDLE hDevice, u16 addr, u16 data)
{
	int res;
	u8 command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_DATA_REG, (u8 *)&data, 2);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

int fc8150_i2c_longwrite(HANDLE hDevice, u16 addr, u32 data)
{
	int res;
	u8 command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_DATA_REG, (u8 *)&data, 4);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

int fc8150_i2c_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;
	u8 command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_DATA_REG, data, length);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

int fc8150_i2c_dataread(HANDLE hDevice, u16 addr, u8 *data, u32 length)
{
	int res;
	u8 command = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_dataread(hDevice, CHIP_ADDR, BBM_DATA_REG, data, length);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

int fc8150_i2c_deinit(HANDLE hDevice)
{
	bbm_write(hDevice, BBM_TS_SEL, 0x00);

	/* tsif_disable(); */

	OAL_DELETE_SEMAPHORE();

	return BBM_OK;
}
