/*
 * tcpal_io_cspi.c
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
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#include <linux/io.h>
#include <asm/mach-types.h>

#include "tcbd_hal.h"

#define SPICMD_VALID_BITS	36
#define SPICMD_BUFF_LEN		8
#define SPICMD_ACK			0x47

#define SPI_SPEED_HZ	  10000000
#define SPI_BITS_PER_WORD 8

#define DMA_MAX_SIZE	(2048)

#define CSPI_READ  0
#define CSPI_WRITE 1

#define CONTINUOUS_MODE 0
#define FIXED_MODE	  1

#if defined(__CSPI_ONLY__)

struct tcpal_cspi_data {
	spinlock_t spin_lock;
	u8 buff_dummy[DMA_MAX_SIZE+(SPICMD_BUFF_LEN*2)+1];
	u8 buff_rw[DMA_MAX_SIZE+(SPICMD_BUFF_LEN*2)+1];
	u8 buff_init_cmd[SPICMD_BUFF_LEN]; /*Set all bit to 1*/
	struct spi_device *spi_dev;
};

static struct tcbd_io_data *tcbd_cspi_io_funcs;
static struct tcpal_cspi_data tcpal_cspi_io_data;

static u8 tcpal_calc_crc8(u8 *data, s32 len)
{
	u16 masking, carry;
	u16 crc;
	u32 i, loop, remain;

	crc = 0x0000;
	loop = len / 8;
	remain = len - loop * 8;

	for (i = 0; i < loop; i++) {
		masking = 1 << 8;
		while ((masking >>= 1)) {
			carry = crc & 0x40;
			crc <<= 1;
			if ((!carry) ^ (!(*data & masking)))
				crc ^= 0x9;
			crc &= 0x7f;
		}
		data++;
	}

	masking = 1 << 8;
	while (remain) {
		carry = crc & 0x40;
		crc <<= 1;
		masking >>= 1;
		if ((!carry) ^ (!(*data & masking)))
			crc ^= 0x9;
		crc &= 0x7f;
		remain--;
	}

	return (u8) crc;
}


#if defined(__USE_TC_CPU__)
static struct spi_device *tcpal_find_cspi_device(void)
{
	struct spi_master *spi_master;
	struct spi_device *spi_device;
	struct device *pdev;
	s8 buff[64];

	spi_master = spi_busnum_to_master(0);
	if (!spi_master) {
		tcbd_debug(DEBUG_ERROR,
			"spi_busnum_to_master(%d) returned NULL\n", 0);
		return NULL;
	}

	spi_device = spi_alloc_device(spi_master);
	if (!spi_device) {
		put_device(&spi_master->dev);
		tcbd_debug(DEBUG_TCPAL_CSPI,
			"spi_alloc_device() failed\n");
		return NULL;
	}

	/* specify a chip select line */
	spi_device->chip_select = 0;

	snprintf(buff, sizeof(buff), "%s.%u",
			dev_name(&spi_device->master->dev),
			spi_device->chip_select);

	pdev = bus_find_device_by_name(spi_device->dev.bus, NULL, buff);
	if (pdev)
		tcbd_debug(DEBUG_TCPAL_CSPI, "spi_device :0x%X\n",
			(u32)spi_device);
	put_device(&spi_master->dev);
	return spi_device;
}
#endif /*__USE_TC_CPU__*/

static s32 tcpal_cspi_close(void)
{
#ifdef __USE_TC_CPU__
	struct tcpal_cspi_data *spi_data = &tcpal_cspi_io_data;
	spi_tcc_close(spi_data->spi_dev);
	spi_dev_put(spi_data->spi_dev);
	spi_data->spi_dev = NULL;
#endif

	tcbd_debug(DEBUG_TCPAL_CSPI, "\n");
	return 0;
}

static s32 tcpal_cspi_open(void)
{
	s32 ret = 0;
	struct tcpal_cspi_data *spi_data = &tcpal_cspi_io_data;

	memset(&tcpal_cspi_io_data, 0, sizeof(tcpal_cspi_io_data));
	memset(spi_data->buff_init_cmd, 0xFF, SPICMD_BUFF_LEN);

#ifdef __USE_TC_CPU__
	spi_data->spi_dev = tcpal_find_cspi_device();

	if (spi_data->spi_dev) {
		ret = spi_tcc_open(spi_data->spi_dev);
		if (ret < 0)
			goto cspi_init_fail;
		spi_data->spi_dev->mode = SPI_MODE_0;
		spi_data->spi_dev->bits_per_word = SPI_BITS_PER_WORD;
		spi_data->spi_dev->max_speed_hz = SPI_SPEED_HZ;
		ret = spi_setup(spi_data->spi_dev);
		if (ret < 0) {
			tcbd_debug(DEBUG_ERROR,
				"spi_setup failed :%d\n", ret);
			goto cspi_init_fail;
		}
	}
#else
	spi_data->spi_dev = spi_dmb;
#endif /*__USE_TC_CPU__*/

	tcbd_debug(DEBUG_TCPAL_CSPI, "\n");
	return 0;

cspi_init_fail:
	return -1;
}

static inline s32 tcpal_cspi_write_and_read(
	u8 *_buffin,
	u8 *_buffout,
	u32 _length)
{
	struct spi_transfer xfer = {0, };
	struct spi_message msg;
	s32 ret = 0;
	struct tcpal_cspi_data *spi_data = &tcpal_cspi_io_data;

	if (!spi_data->spi_dev || !_length)
		return -EFAULT;
	if (!_buffin && !_buffout)
		return -EFAULT;

	xfer.tx_buf = _buffin;
	xfer.rx_buf = _buffout;
	xfer.len = _length;
/*	xfer.speed_hz = SPI_SPEED_HZ; */
/*	xfer.bits_per_word = SPI_BITS_PER_WORD; */

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(spi_data->spi_dev, &msg);

	if (ret < 0)
		return -TCERR_OS_DRIVER_FAIL;

	return 0;
}

static inline s32 tcpal_cspi_single_io(
	u8 _write_flag,
	u16 _reg_addr,
	u8 *_data)
{
	s32 ret = 0;
	u8 buffer[SPICMD_BUFF_LEN+1];
	u8 buffout[SPICMD_BUFF_LEN+1];
	u8 crc;

	/* start bit(1) + chip_id(7) */
	buffer[0] =  tcbd_cspi_io_funcs->chip_addr;
	/* mode(1) + rw(1) + fix(1) + addr(5) */
	buffer[1] = (0 << 7) | (_write_flag << 6) | (1 << 5) |
				((_reg_addr & 0x7c0) >> 6);
	/* addr(6bit) + NULL(2bit) */
	buffer[2] = (_reg_addr & 0x03f) << 2 | 0x0;

	if (_write_flag)
		buffer[3] = _data[0]; /* write */
	else
		buffer[3] = 0x0; /* null(8) */

	buffer[4] = 0x00;

	crc = tcpal_calc_crc8(buffer, 36);
	buffer[4] = 0x00 | ((crc & 0x7f) >> 3);   /* null(4) + crc(4) */
	buffer[5] = ((crc & 0x07) << 5) | 0x0f;   /* crc(3) + end bit(5) */
	buffer[6] = 0xff;
	buffer[7] = 0xff;

	ret = tcpal_cspi_write_and_read(buffer, buffout, SPICMD_BUFF_LEN);
	if (ret < 0)
		return ret;

	if (buffout[7] != SPICMD_ACK) { /* ack */
		tcbd_debug(DEBUG_ERROR,
			"# Single %s ACK error chip_addr:0x%X, regAddr:0x%X\n",
			_write_flag ? "Write" : "Read",
			tcbd_cspi_io_funcs->chip_addr, _reg_addr);
		tcbd_debug(DEBUG_ERROR,
			"# [%02x][%02x][%02x][%02x][%02x]"
			"[%02x][%02x][%02x]//[%02x][%02x][%02x]\n",
				buffer[0], buffer[1], buffer[2],
				buffer[3], buffer[4], buffer[5],
				buffer[6], buffer[7],
				buffout[6], buffout[7], buffout[8]);
		return -TCERR_ACK_FAIL;
	}

	if (_write_flag == 0)
		*_data = buffout[6];

	return 0;
}

static inline s32 tcpal_cspi_burst_io(
	u8 _write_flag,
	u16 _reg_addr,
	u8 *_data,
	s32 _size,
	u8 _fixedMode)
{
	s32 ret = 0;
	struct tcpal_cspi_data *spi_data = &tcpal_cspi_io_data;
	u8 crc;
	u8 *buffer;
	u8 *buffout;

	if (_write_flag == 0) {
		buffer = spi_data->buff_dummy;
		buffout = spi_data->buff_rw;
	} else {
		memcpy(spi_data->buff_rw+SPICMD_BUFF_LEN, _data, _size);
		buffer = spi_data->buff_rw;
		buffout = spi_data->buff_dummy;
	}
	memset(buffer+SPICMD_BUFF_LEN+_size, 0xFF, SPICMD_BUFF_LEN);

	if (_size > DMA_MAX_SIZE)
		return -TCERR_INVALID_ARG;

	/* MAX 16KB (Output buffer max size 7KB) (LENGTH + 1 Byte) */
	_size--;

	/* start bit(1) + chip_id(7) */
	buffer[0] = tcbd_cspi_io_funcs->chip_addr;
	/* mode(1) + rw(1) + fix(1) + addr(5) */
	buffer[1] = 1 << 7 | _write_flag << 6 | _fixedMode << 5 |
				((_reg_addr & 0x7c0) >> 6);
	/* addr(6bit) + length(2bit) */
	buffer[2] = (_reg_addr & 0x03f) << 2 | ((_size & 0x3000) >> 12);
	/* length(8bit) */
	buffer[3] = (_size & 0xff0) >> 4;

	buffer[4] = (_size & 0xf) << 4;
	crc = tcpal_calc_crc8(buffer, 36);
	/* length(4) + crc(4) */
	buffer[4] = ((_size & 0xf) << 4) | ((crc & 0x7f) >> 3);
	/* crc(3) + end bit(5) */
	buffer[5] = ((crc & 0x07) << 5) | 0x0f;
	buffer[6] = 0xff;
	buffer[7] = 0xff;

	_size++;

	ret =  tcpal_cspi_write_and_read(
				buffer, buffout, _size+SPICMD_BUFF_LEN*2);
	if (ret < 0)
		return ret;

	if (buffout[7] != SPICMD_ACK) {/* ack */
		tcbd_debug(DEBUG_ERROR,
			"# Burst %s ACK error, chip_addr:0x%X, "
			"reg_addr:0x%X, size:%d, mode:%d\n",
				_write_flag ? "Write" : "Read",
				tcbd_cspi_io_funcs->chip_addr,
				_reg_addr, _size, _fixedMode);
		tcbd_debug(DEBUG_ERROR,
			"# [%02x][%02x][%02x][%02x][%02x]"
			"[02%x][02%x][%02x]//[%02x][%02x][%02x]\n",
				buffer[0], buffer[1], buffer[2],
				buffer[3], buffer[4], buffer[5],
				buffer[6], buffer[7],
				buffout[6], buffout[7], buffout[8]);
		return -TCERR_ACK_FAIL;
	}

	if (_write_flag == 0)
		memcpy(_data, buffout + SPICMD_BUFF_LEN, _size);

	return 0;
}

static inline s32 tcpal_cspi_reg_read_burst(
	u8 _reg_addr,
	u8 *_data,
	s32 _size,
	u8 mode)
{
	u32 i;
	u32 cmax, cremain;
	s32 ret;

	cmax = _size / DMA_MAX_SIZE;
	cremain = _size % DMA_MAX_SIZE;

	for (i = 0; i < cmax; i++) {
		ret = tcpal_cspi_burst_io(
			CSPI_READ,
			_reg_addr,
			&_data[i * DMA_MAX_SIZE],
			DMA_MAX_SIZE,
			mode);
		if (ret < 0)
			return ret;
	}

	if (cremain != 0) {
		ret = tcpal_cspi_burst_io(
			CSPI_READ,
			_reg_addr,
			&_data[i * DMA_MAX_SIZE],
			cremain,
			mode);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static inline s32 tcpal_cspi_reg_write_burst(
	u8 _reg_addr,
	u8 *_data,
	s32 _size,
	u8 mode)
{
	u32 i;
	u32 cmax, cremain;
	s32 ret;

	cmax = _size / DMA_MAX_SIZE;
	cremain = _size % DMA_MAX_SIZE;

	for (i = 0; i < cmax; i++) {
		ret = tcpal_cspi_burst_io(
			CSPI_WRITE,
			_reg_addr,
			&_data[i * DMA_MAX_SIZE],
			DMA_MAX_SIZE,
			mode);
		if (ret < 0)
			return ret;
	}

	if (cremain) {
		ret = tcpal_cspi_burst_io(
			CSPI_WRITE,
			_reg_addr,
			&_data[i * DMA_MAX_SIZE],
			cremain,
			mode);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static s32 tcpal_cspi_reg_read(u8 _reg_addr, u8 *_data)
{
	return tcpal_cspi_single_io(CSPI_READ, _reg_addr, _data);
}

static s32 tcpal_cspi_reg_write(u8 _reg_addr, u8 _data)
{
	return tcpal_cspi_single_io(CSPI_WRITE, _reg_addr, &_data);
}

static s32 tcpal_cspi_reg_read_burst_cont(
	u8 _reg_addr, u8 *_data, s32 _size)
{
	return tcpal_cspi_reg_read_burst(
				_reg_addr, _data, _size, CONTINUOUS_MODE);
}

static s32 tcpal_cspi_reg_write_burst_cont(
	u8 _reg_addr, u8 *_data, s32 _size)
{
	return tcpal_cspi_reg_write_burst(
				_reg_addr, _data, _size, CONTINUOUS_MODE);
}

static s32 tcpal_cspi_reg_read_burst_fix(
	u8 _reg_addr, u8 *_data, s32 _size)
{
	return tcpal_cspi_reg_read_burst(
				_reg_addr, _data, _size, FIXED_MODE);
}

static s32 tcpal_cspi_reg_write_burst_fix(
	u8 _reg_addr, u8 *_data, s32 _size)
{
	return tcpal_cspi_reg_write_burst(
				_reg_addr, _data, _size, FIXED_MODE);
}

void tcpal_set_cspi_io_function(void)
{
	tcbd_cspi_io_funcs = tcbd_get_io_struct();

	tcbd_cspi_io_funcs->open = tcpal_cspi_open;
	tcbd_cspi_io_funcs->close = tcpal_cspi_close;
	tcbd_cspi_io_funcs->reg_write = tcpal_cspi_reg_write;
	tcbd_cspi_io_funcs->reg_read  = tcpal_cspi_reg_read;
	tcbd_cspi_io_funcs->reg_write_burst_cont =
		tcpal_cspi_reg_write_burst_cont;
	tcbd_cspi_io_funcs->reg_read_burst_cont =
		tcpal_cspi_reg_read_burst_cont;
	tcbd_cspi_io_funcs->reg_write_burst_fix =
		tcpal_cspi_reg_write_burst_fix;
	tcbd_cspi_io_funcs->reg_read_burst_fix =
		tcpal_cspi_reg_read_burst_fix;
}
#endif /*__CSPI_ONLY__*/
