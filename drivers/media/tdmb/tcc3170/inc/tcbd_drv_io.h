/*
 * tcbd_drv_io.h
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

#ifndef __TCBD_DRV_COMMON_H__
#define __TCBD_DRV_COMMON_H__
#include "tcbd_drv_register.h"
#include "tcbd_error.h"

#define MB_HOSTMAIL	 0x47
#define MB_SLAVEMAIL	0x74

#define DEFAULT_CHIP_ADDRESS 0x50
struct tcbd_io_data {
	s32 chip_addr;
	u32 sem;
	s32 (*open)(void);
	s32 (*close)(void);
	s32 (*reg_write)(u8 _addr, u8 _data);
	s32 (*reg_read)(u8 _addr, u8 *_data);
	s32 (*reg_write_burst_cont)(u8 _addr, u8 *_data, s32 _size);
	s32 (*reg_read_burst_cont)(u8 _addr, u8 *_data, s32 _size);
	s32 (*reg_write_burst_fix)(u8 _addr, u8 *_data, s32 _size);
	s32 (*reg_read_burst_fix)(u8 _addr, u8 *_data, s32 _size);
};

#define MAX_MAIL_COUNT 7
#define MAX_TIME_TO_WAIT_MAIL 1000
struct tcbd_mail_data {
	u8 flag;
	u8 count;
	u16 cmd;
	u32 status;
	u32 data[MAX_MAIL_COUNT];
	u8 pad[2];
};

TCBB_FUNC struct tcbd_io_data *tcbd_get_io_struct(void);
TCBB_FUNC void tcpal_set_i2c_io_function(void);
TCBB_FUNC void tcpal_set_cspi_io_function(void);

TCBB_FUNC s32 tcbd_io_open(struct tcbd_device *_device);
TCBB_FUNC s32 tcbd_io_close(struct tcbd_device *_device);

TCBB_FUNC s32 tcbd_reg_read(
	struct tcbd_device *_handle, u8 _addr, u8 *_data);
TCBB_FUNC s32 tcbd_reg_write(
	struct tcbd_device *_handle, u8 _addr, u8 _data);
TCBB_FUNC s32 tcbd_reg_read_burst_cont(
	struct tcbd_device *_handle, u8 _addr, u8 *_data, s32 _size);
TCBB_FUNC s32 tcbd_reg_write_burst_cont(
	struct tcbd_device *_handle, u8 _addr, u8 *_data, s32 _size);
TCBB_FUNC s32 tcbd_reg_read_burst_fix(
	struct tcbd_device *_handle, u8 _addr, u8 *_data, s32 _size);
TCBB_FUNC s32 tcbd_reg_write_burst_fix(
	struct tcbd_device *_handle, u8 _addr, u8 *_data, s32 _size);
TCBB_FUNC s32 tcbd_mem_write(
	struct tcbd_device *_handle, u32 _addr,	u8 *_data, u32 _size);
TCBB_FUNC s32 tcbd_mem_read(
	struct tcbd_device *_handle, u32 _addr,	u8 *_data, u32 _size);
TCBB_FUNC s32 tcbd_rf_reg_write(
	struct tcbd_device *_handle, u8 _addr, u32 _data);
TCBB_FUNC s32 tcbd_rf_reg_read(
	struct tcbd_device *_handle, u32 _addr,	u32 *_data);

TCBB_FUNC s32 tcbd_send_mail(struct tcbd_device *_device,
				struct tcbd_mail_data *_mail);
TCBB_FUNC s32 tcbd_recv_mail(struct tcbd_device *_device,
				struct tcbd_mail_data *_mail);
TCBB_FUNC s32 tcbd_read_mail_box(struct tcbd_device *_device, u16 cmd,
						s32 len, u32 *data);
TCBB_FUNC s32 tcbd_write_mail_box(struct tcbd_device *_device, u16 _cmd,
						s32 _cnt, u32 *_data);
TCBB_FUNC s32 tcbd_read_file(struct tcbd_device *_device, char *_path,
						u8 *_buff, s32 _size);
extern struct spi_device *spi_dmb; /* sukjoon_temp */
#endif /*__TCBD_DRV_COMMON_H__*/
