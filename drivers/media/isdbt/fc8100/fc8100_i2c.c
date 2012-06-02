/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fc8100_i2c.c

 Description : fc8100 host interface

 History :
 ----------------------------------------------------------------------
 2009/09/29	bruce		initial
*******************************************************************************/
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-core.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>
#include "fci_types.h"
#include "fci_oal.h"

#define FC8100_I2C_ADDR 0x77
#define I2C_M_FCIRD 1
#define I2C_M_FCIWR 0
#define I2C_MAX_SEND_LENGTH 300

struct i2c_ts_driver{
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
};

struct i2c_client *fc8100_i2c;

struct i2c_driver fc8100_i2c_driver;

static int i2c_bb_read(HANDLE hDevice, u16 addr, u8 *data, u16 length);

static int fc8100_i2c_probe(struct i2c_client *i2c_client,
	const struct i2c_device_id *id)
{
	fc8100_i2c = i2c_client;
	i2c_set_clientdata(i2c_client, NULL);

	return 0;
}

static int fc8100_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id fc8100_id[] = {
	{ "isdbti2c", 0 },
	{ },
};

struct i2c_driver fc8100_i2c_driver = {
	.driver = {
			.name = "fc8100_i2c",
			.owner = THIS_MODULE,
		},
	.probe    = fc8100_i2c_probe,
	.remove   = fc8100_remove,
	.id_table = fc8100_id,
};

static int i2c_init(void)
{
	int res;

	printk("fc8100_i2c_drv_init ENTER...\n");
	fc8100_i2c = kzalloc(sizeof(struct i2c_ts_driver), GFP_KERNEL);

	if (fc8100_i2c == NULL)
		return -ENOMEM;

	res = i2c_add_driver(&fc8100_i2c_driver);

	return res;
}

static int i2c_deinit(void)
{
	printk("fc8100_i2c_drv_deinit ENTER...\n");

	i2c_del_driver(&fc8100_i2c_driver);

	return 0;
}

static int i2c_bb_read(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;
	struct i2c_msg rmsg[2];
	unsigned char i2c_data[1];

	rmsg[0].addr = fc8100_i2c->addr;
	rmsg[0].flags = I2C_M_FCIWR;
	rmsg[0].len = 1;
	rmsg[0].buf = i2c_data;
	i2c_data[0] = addr & 0xff;

	rmsg[1].addr = fc8100_i2c->addr;
	rmsg[1].flags = I2C_M_FCIRD;
	rmsg[1].len = length;
	rmsg[1].buf = data;
	res = i2c_transfer(fc8100_i2c->adapter, &rmsg[0], 2);

	/* return status */
	if (res >= 0)
		res = 0;

	return res;
}

static int i2c_bb_write(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;
	struct i2c_msg wmsg;
	unsigned char i2c_data[I2C_MAX_SEND_LENGTH];

	if (length+1 > I2C_MAX_SEND_LENGTH) {
		printk(".......error");
		return -ENODEV;
	}
	wmsg.addr = fc8100_i2c->addr;
	wmsg.flags = I2C_M_FCIWR;
	wmsg.len = length + 1;
	wmsg.buf = i2c_data;

	i2c_data[0] = addr & 0xff;
	memcpy(&i2c_data[1], data, length);

	res = i2c_transfer(fc8100_i2c->adapter, &wmsg, 1);

	/* return status */
	if (res >= 0)
		res = 0;

	return res;
}

static int i2c_rf_read(HANDLE hDevice, u8 addr, u8 *data, u8 length)
{
	int res;
	struct i2c_msg rmsg[3];
	unsigned char i2c_rp_data[2];
	unsigned char i2c_data[1];

	/* printk("i2c_rf_read ENTER  addr : %x, length : %d\n", addr, length); */
	rmsg[0].addr = fc8100_i2c->addr;
	rmsg[0].flags = I2C_M_FCIWR;
	rmsg[0].len = 2;
	rmsg[0].buf = i2c_rp_data;
	i2c_rp_data[0] = 0x02;
	i2c_rp_data[1] = 0x01;

	rmsg[1].addr = fc8100_i2c->addr;
	rmsg[1].flags = I2C_M_FCIWR;
	rmsg[1].len = 1;
	rmsg[1].buf = i2c_data;
	i2c_data[0] = addr & 0xff;

	rmsg[2].addr = fc8100_i2c->addr;
	rmsg[2].flags = I2C_M_FCIRD;
	rmsg[2].len = length;
	rmsg[2].buf = data;

	res = i2c_transfer(fc8100_i2c->adapter, &rmsg[0], 3);
	/* printk("i2c_rf_read data : %x\n", data[0]); */

	/* return status */
	if (res >= 0)
		res = 0;

	return res;
}

static int i2c_rf_write(HANDLE hDevice, u8 addr, u8 *data, u8 length)
{
	int res;
	struct i2c_msg wmsg[2];
	unsigned char i2c_rp_data[2];
	unsigned char i2c_data[I2C_MAX_SEND_LENGTH];

	/* printk("i2c_rf_write ENTER  addr : %x, data : %x, length : %d\n", addr, data[0], length); */
	if (length+1 > I2C_MAX_SEND_LENGTH) {
		printk(".......error");
		return -ENODEV;
	}

	wmsg[0].addr = fc8100_i2c->addr;
	wmsg[0].flags = I2C_M_FCIWR;
	wmsg[0].len = 2;
	wmsg[0].buf = i2c_rp_data;
	i2c_rp_data[0] = 0x02;
	i2c_rp_data[1] = 0x01;

	wmsg[1].addr = fc8100_i2c->addr;
	wmsg[1].flags = I2C_M_FCIWR;
	wmsg[1].len = length+1;
	wmsg[1].buf = i2c_data;
	i2c_data[0] = addr & 0xff;
	memcpy(&i2c_data[1], data, length);

	res = i2c_transfer(fc8100_i2c->adapter, &wmsg[0], 2);

	/* return status */
	if (res >= 0)
		res = 0;

	return res;
}

int fc8100_i2c_init(HANDLE hDevice, u16 param1, u16 param2)
{
	int res;

	res = i2c_init();

	return res;
}

int fc8100_i2c_byteread(HANDLE hDevice, u16 addr, u8 *data)
{
	int res = BBM_NOK;

	/* PRINTF(0, "fc8100_i2c_byteread 0x%x\n", addr); */
	res =  i2c_bb_read(hDevice, addr, (u8 *)data, 1);

	return res;
}

int fc8100_i2c_wordread(HANDLE hDevice, u16 addr, u16 *data)
{
	int res = BBM_NOK;

	/* PRINTF(0, "fc8100_i2c_wordread 0x%x\n", addr); */
	res =  i2c_bb_read(hDevice, addr, (u8 *)data, 2);

	return res;
}

int fc8100_i2c_longread(HANDLE hDevice, u16 addr, u32 *data)
{
	int res = BBM_NOK;

	/* PRINTF(0, "fc8100_i2c_longread 0x%x\n", addr); */
	res =  i2c_bb_read(hDevice, addr, (u8 *)data, 4);

	return res;
}

int fc8100_i2c_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 size)
{
	int res = BBM_NOK;

	/* PRINTF(0, "fc8100_i2c_bulkread 0x%x\n", addr); */
	res =  i2c_bb_read(hDevice, addr, (u8 *)data, size);

	return res;
}

int fc8100_i2c_bytewrite(HANDLE hDevice, u16 addr, u8 data)
{
	int res = BBM_NOK;

	res = i2c_bb_write(hDevice, addr, (u8 *)&data, 1);

	return res;
}

int fc8100_i2c_wordwrite(HANDLE hDevice, u16 addr, u16 data)
{
	int res = BBM_NOK;

	res = i2c_bb_write(hDevice, addr, (u8 *)&data, 2);

	return res;
}

int fc8100_i2c_longwrite(HANDLE hDevice, u16 addr, u32 data)
{
	int res = BBM_NOK;

	res = i2c_bb_write(hDevice, addr, (u8 *)&data, 4);

	return res;
}

int fc8100_i2c_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 size)
{
	int res = BBM_NOK;

	res = i2c_bb_write(hDevice, addr, (u8 *)&data, size);

	return res;
}

int fc8100_i2c_dataread(HANDLE hDevice, u16 addr, u8 *data, u16 size)
{
	int res = BBM_NOK;

	return res;
}

int fc8100_rf_bulkread(HANDLE hDevice, u8 addr, u8 *data, u8 size)
{
	int res = BBM_NOK;

	res = i2c_rf_read(hDevice, addr, (u8 *)data, size);

	return res;
}

int fc8100_rf_bulkwrite(HANDLE hDevice, u8 addr, u8 *data, u8 size)
{
	int res = BBM_NOK;

	res = i2c_rf_write(hDevice, addr, (u8 *)data, size);

	return res;
}

int fc8100_i2c_deinit(HANDLE hDevice)
{
	int res = BBM_NOK;

	res = i2c_deinit();

	return res;
}

int fc8100_spi_init(HANDLE hDevice, u16 param1, u16 param2)
{

	return BBM_OK;
}

int fc8100_spi_dataread(HANDLE hDevice, u16 addr, u8 *pBuf, u16 nLength)
{
	return BBM_OK;
}
