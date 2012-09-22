/*
 * tcpal_io_i2c.c
 *
 * Author:  <linux@telechips.com>
 * Description: Telechips broadcast driver
 *
 * Copyright (c) Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tcpal_os.h"
#include "tcpal_debug.h"

#include "tcbd_feature.h"
#include "tcbd_api_common.h"
#include "tcbd_drv_io.h"

#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>

#include <linux/io.h>
#include <asm/mach-types.h>

#define MAX_I2C_BURST		512
#define Bit7		  0x00000080

#define I2C_BUS				1
#define I2C_ADDR	   (0xA0>>1)

#if defined(__I2C_STS__)

static u8 static_buffer_i2c[MAX_I2C_BURST+4];
static struct i2c_client *tcpal_i2c_client;

static const struct i2c_device_id tcpal_i2c_id[] = {
	{"tc317x", 0},
};

static s32 tcpal_i2c_probe(
	struct i2c_client *i2c, const struct i2c_device_id *id)
{
	s32 ret = 0;
	tcpal_i2c_client = i2c;
	tcbd_debug(DEBUG_TCPAL_I2C, "tcpal_i2c_client : %p\n", i2c);
	return ret;
}

static s32 tcpal_i2c_remove(struct i2c_client *client)
{
	tcbd_debug(DEBUG_TCPAL_I2C, "tcpal_i2c_client : %p\n", client);
	return 0;
}

MODULE_DEVICE_TABLE(i2c, tcpal_i2c_id);

static struct i2c_driver tcpal_i2c_driver = {
	.driver = {
		.name  = "tc317x",
		.owner = THIS_MODULE,
	},
	.probe	= tcpal_i2c_probe,
	.remove   = tcpal_i2c_remove,
	.id_table = tcpal_i2c_id,
};

static s32 tcpal_i2c_add_device(void)
{
	s32 ret;
	struct i2c_board_info info;
	struct i2c_adapter *adapter;
	struct i2c_client *client;

	ret = i2c_add_driver(&tcpal_i2c_driver);
	if (ret < 0) {
		tcbd_debug(DEBUG_TCPAL_I2C, "can't add i2c driver\n");
		return ret;
	}

	memset(&info, 0, sizeof(struct i2c_board_info));
	info.addr = I2C_ADDR;
	strlcpy(info.type, "tc317x", I2C_NAME_SIZE);

	adapter = i2c_get_adapter(I2C_BUS);
	if (!adapter) {
		tcbd_debug(DEBUG_TCPAL_I2C,
			"can't get i2c adapter :%d\n", I2C_BUS);
		goto err_driver;
	}

	client = i2c_new_device(adapter, &info);
	i2c_put_adapter(adapter);
	if (!client) {
		tcbd_debug(DEBUG_TCPAL_I2C,
			"can't add i2c device at 0x%X\n", info.addr);
		goto err_driver;
	}

	return 0;

err_driver:
	i2c_del_driver(&tcpal_i2c_driver);
	return -ENODEV;
}

static s32 tcpal_i2c_close(void)
{
	i2c_unregister_device(tcpal_i2c_client);
	i2c_del_driver(&tcpal_i2c_driver);
	return 0;
}

static s32 tcpal_i2c_open(void)
{
	return tcpal_i2c_add_device();
}

static s32 tcpal_i2c_reg_read(
	u8 _reg_addr, u8 *_data)
{
	s32 ret = 0;

	if (!tcpal_i2c_client)
		return -TCERR_OS_DRIVER_FAIL;

	ret = i2c_master_send(tcpal_i2c_client, &_reg_addr, 1);
	ret |= i2c_master_recv(tcpal_i2c_client, _data, 1);
	if (ret < 0) {
		tcbd_debug(DEBUG_TCPAL_I2C,
			"I2C read error %d\n", ret);
		return -TCERR_OS_DRIVER_FAIL;
	}
	return 0;
}

static s32 tcpal_i2c_reg_write(u8 _reg_addr, u8 _data)
{
	s32 ret = 0;
	u8 buf[2];

	if (!tcpal_i2c_client)
		return -TCERR_OS_DRIVER_FAIL;

	buf[0] = _reg_addr;
	buf[1] = _data;

	ret = i2c_master_send(tcpal_i2c_client, buf, 2);

	if (ret < 0)
		return -TCERR_OS_DRIVER_FAIL;

	return 0;
}

static inline s32 tcpal_i2c_reg_read_burst(
	u8 _reg_addr, u8 *_data, s32 _size)
{
	s32 i, ret = 0;
	s32 num_chunk, remain, sent, recvd;

	num_chunk = _size / MAX_I2C_BURST;
	remain = _size % MAX_I2C_BURST;

	if (!tcpal_i2c_client)
		return -TCERR_OS_DRIVER_FAIL;

	for (i = 0; i < num_chunk; i++) {
		sent = i2c_master_send(tcpal_i2c_client, &_reg_addr, 1);
		if (sent < 1) {
			tcbd_debug(DEBUG_TCPAL_I2C,
				"I2C Multi read 8 Addr Error!! %d\n", sent);
			return -TCERR_OS_DRIVER_FAIL;
		}

		recvd = i2c_master_recv(
			tcpal_i2c_client,
			&_data[i*MAX_I2C_BURST],
			MAX_I2C_BURST);
		if (recvd < 0) {
			tcbd_debug(DEBUG_TCPAL_I2C,
				"I2C Multi read 8 data Error!! %d\n", recvd);
			return -TCERR_OS_DRIVER_FAIL;
		}
	}

	if (remain) {
		sent = i2c_master_send(tcpal_i2c_client, &_reg_addr, 1);
		if (sent < 1) {
			tcbd_debug(DEBUG_TCPAL_I2C,
				"I2C Multi read 8 Addr Error!!, %d\n", sent);
			return -TCERR_OS_DRIVER_FAIL;
		}
		ret = i2c_master_recv(
			tcpal_i2c_client,
			&_data[num_chunk * MAX_I2C_BURST],
			remain);
		if (ret < 0) {
			tcbd_debug(DEBUG_TCPAL_I2C,
				"I2C Multi read 8 data Error!!\n");
			return -TCERR_OS_DRIVER_FAIL;
		}
	}

	return 0;
}

static inline s32 tcpal_i2c_reg_write_burst(
	u8 _reg_addr, u8 *_data, s32 _size)
{
	s32 ret = 0;
	s32 num_chunk, remain, sent;
	s32 i;

	if (!tcpal_i2c_client)
		return -TCERR_OS_DRIVER_FAIL;

	num_chunk = _size/MAX_I2C_BURST;
	remain = _size%MAX_I2C_BURST;

	static_buffer_i2c[0] = _reg_addr;
	for (i = 0; i < num_chunk; i++) {
		memcpy(&static_buffer_i2c[1], &_data[i*MAX_I2C_BURST],
				MAX_I2C_BURST);
		sent = i2c_master_send(
				tcpal_i2c_client,
				static_buffer_i2c,
				MAX_I2C_BURST+1);
		if (ret < 0 || sent < 1) {
			tcbd_debug(DEBUG_TCPAL_I2C,
				"I2C write error %d\n", ret);
			return -TCERR_OS_DRIVER_FAIL;
		}
	}

	if (remain) {
		memcpy(&static_buffer_i2c[1],
				&_data[num_chunk * MAX_I2C_BURST],
				remain);
		ret = i2c_master_send(
				tcpal_i2c_client,
				static_buffer_i2c,
				remain+1);
		if (ret < 1) {
			tcbd_debug(DEBUG_TCPAL_I2C, "I2C write error!\n");
			return -TCERR_OS_DRIVER_FAIL;
		}
	}

	return 0;
}

static s32 tcpal_i2c_reg_read_burst_fix(
	u8 _reg_addr, u8 *_data, s32 _size)
{
	return tcpal_i2c_reg_read_burst(_reg_addr|Bit7, _data, _size);
}

static s32 tcpal_i2c_reg_write_burst_fix(
	u8 _reg_addr, u8 *_data, s32 _size)
{
	return tcpal_i2c_reg_write_burst(_reg_addr|Bit7, _data, _size);
}

static s32 tcpal_i2c_reg_read_burst_cont(
	u8 _reg_addr, u8 *_data, s32 _size)
{
	return tcpal_i2c_reg_read_burst(_reg_addr, _data, _size);
}

static s32 tcpal_i2c_reg_write_burst_cont(
	u8 _reg_addr, u8 *_data, s32 _size)
{
	return tcpal_i2c_reg_write_burst(_reg_addr, _data, _size);
}

void tcpal_set_i2c_io_function(void)
{
	struct tcbd_io_data *tcbd_i2c_io_funcs = tcbd_get_io_struct();

	tcbd_i2c_io_funcs->open = tcpal_i2c_open;
	tcbd_i2c_io_funcs->close = tcpal_i2c_close;
	tcbd_i2c_io_funcs->reg_write = tcpal_i2c_reg_write;
	tcbd_i2c_io_funcs->reg_read  = tcpal_i2c_reg_read;
	tcbd_i2c_io_funcs->reg_write_burst_cont =
		tcpal_i2c_reg_write_burst_cont;
	tcbd_i2c_io_funcs->reg_read_burst_cont =
		tcpal_i2c_reg_read_burst_cont;
	tcbd_i2c_io_funcs->reg_write_burst_fix =
		tcpal_i2c_reg_write_burst_fix;
	tcbd_i2c_io_funcs->reg_read_burst_fix =
		tcpal_i2c_reg_read_burst_fix;
}

#endif /*__I2C_STS__ */
