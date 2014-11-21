/*
 * tcbd_drv_io.c
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
#include "tcbd_drv_ip.h"
#include "tcbd_drv_io.h"

static struct tcbd_io_data tcbd_io_funcs;

struct tcbd_io_data *tcbd_get_io_struct(void)
{
	return &tcbd_io_funcs;
}

s32 tcbd_io_open(struct tcbd_device *_device)
{
	s32 ret = 0;

	if (tcbd_io_funcs.open == NULL)
		return -TCERR_IO_NOT_INITIALIZED;

	tcpal_create_lock(&tcbd_io_funcs.sem, "tcbd_io_lock", 0);

	ret = tcbd_io_funcs.open();
	if (ret < 0)
		return ret;

	_device->chip_addr = DEFAULT_CHIP_ADDRESS;

	/*Initialize internal processor */
	tcbd_reset_ip(_device, TCBD_SYS_COMP_EP, TCBD_SYS_COMP_ALL);

	return ret;
}

s32 tcbd_io_close(struct tcbd_device *_device)
{
	s32 ret = 0;

	if (tcbd_io_funcs.close == NULL)
		return -TCERR_IO_NOT_INITIALIZED;

	tcpal_destroy_lock(&tcbd_io_funcs.sem);

	ret = tcbd_io_funcs.close();
	memset(_device, 0, sizeof(struct tcbd_device));

	return ret;
}

s32 tcbd_reg_read(
	struct tcbd_device *_device, u8 _addr, u8 *_data)
{
	s32 ret;

	if (tcbd_io_funcs.reg_read == NULL)
		return -TCERR_IO_NOT_INITIALIZED;

	if (_device == NULL || _data == NULL)
		return -TCERR_INVALID_ARG;

	tcpal_lock(&tcbd_io_funcs.sem);
	tcbd_io_funcs.chip_addr = _device->chip_addr;
	ret = tcbd_io_funcs.reg_read(_addr, _data);
	tcpal_unlock(&tcbd_io_funcs.sem);
	return ret;
}

s32 tcbd_reg_write(
	struct tcbd_device *_device, u8 _addr, u8 _data)
{
	s32 ret;

	if (tcbd_io_funcs.reg_write == NULL)
		return -TCERR_IO_NOT_INITIALIZED;

	tcpal_lock(&tcbd_io_funcs.sem);
	tcbd_io_funcs.chip_addr = _device->chip_addr;
	ret = tcbd_io_funcs.reg_write(_addr, _data);
	tcpal_unlock(&tcbd_io_funcs.sem);

	return ret;
}

s32 tcbd_reg_read_burst_cont(
	struct tcbd_device *_device, u8 _addr, u8 *_data, s32 _size)
{
	s32 ret;

	if (tcbd_io_funcs.reg_read_burst_cont == NULL)
		return -TCERR_IO_NOT_INITIALIZED;

	if (_device == NULL || _data == NULL)
		return TCERR_INVALID_ARG;

	tcpal_lock(&tcbd_io_funcs.sem);
	tcbd_io_funcs.chip_addr = _device->chip_addr;
	ret = tcbd_io_funcs.reg_read_burst_cont(_addr, _data, _size);
	tcpal_unlock(&tcbd_io_funcs.sem);

	return ret;
}

s32 tcbd_reg_write_burst_cont(
	struct tcbd_device *_device, u8 _addr, u8 *_data, s32 _size)
{
	s32 ret;

	if (tcbd_io_funcs.reg_write_burst_cont == NULL)
		return -TCERR_IO_NOT_INITIALIZED;

	if (_device == NULL || _data == NULL || _size <= 0)
		return TCERR_INVALID_ARG;

	tcpal_lock(&tcbd_io_funcs.sem);
	tcbd_io_funcs.chip_addr = _device->chip_addr;
	ret = tcbd_io_funcs.reg_write_burst_cont(_addr, _data, _size);
	tcpal_unlock(&tcbd_io_funcs.sem);

	return ret;
}

s32 tcbd_reg_read_burst_fix(
	struct tcbd_device *_device, u8 _addr, u8 *_data, s32 _size)
{
	s32 ret;

	if (tcbd_io_funcs.reg_read_burst_fix == NULL)
		return -TCERR_IO_NOT_INITIALIZED;

	if (_device == NULL || _data == NULL || _size <= 0)
		return TCERR_INVALID_ARG;

	tcpal_lock(&tcbd_io_funcs.sem);
	tcbd_io_funcs.chip_addr = _device->chip_addr;
	ret = tcbd_io_funcs.reg_read_burst_fix(_addr, _data, _size);
	tcpal_unlock(&tcbd_io_funcs.sem);

	return ret;
}

s32 tcbd_reg_write_burst_fix(
	struct tcbd_device *_device, u8 _addr, u8 *_data, s32 _size)
{
	s32 ret;

	if (tcbd_io_funcs.reg_write_burst_fix == NULL)
		return -TCERR_IO_NOT_INITIALIZED;

	if (_device == NULL || _data == NULL || _size <= 0)
		return TCERR_INVALID_ARG;

	tcpal_lock(&tcbd_io_funcs.sem);
	tcbd_io_funcs.chip_addr = _device->chip_addr;
	ret = tcbd_io_funcs.reg_write_burst_fix(_addr, _data, _size);
	tcpal_unlock(&tcbd_io_funcs.sem);

	return ret;
}

s32 tcbd_mem_write(
	struct tcbd_device *_device, u32 _addr,	u8 *_data, u32 _size)
{
	s32 ret = 0;
	u32 addr = SWAP32(_addr);
	u16 size = SWAP16((u16)(_size>>2));
	u8 ctrl_data;

	if (_size <= 0 || _data == NULL || _device == NULL)
		return -TCERR_INVALID_ARG;

	tcpal_lock(&tcbd_io_funcs.sem);
	tcbd_io_funcs.chip_addr = _device->chip_addr;

	ctrl_data =
		TCBD_CMDDMA_DMAEN |
		TCBD_CMDDMA_WRITEMODE |
		TCBD_CMDDMA_CRC32EN;
	ret |= tcbd_io_funcs.reg_write(TCBD_CMDDMA_CTRL, ctrl_data);
	ret |= tcbd_io_funcs.reg_write_burst_cont(
		TCBD_CMDDMA_SADDR, (u8 *)&addr, sizeof(u32));
	ret |= tcbd_io_funcs.reg_write_burst_cont(
		TCBD_CMDDMA_SIZE, (u8 *)&size, sizeof(u16));
	ctrl_data =
		TCBD_CMDDMA_START_AUTOCLR |
		TCBD_CMDDMA_INIT_AUTOCLR |
		TCBD_CMDDMA_CRC32INIT_AUTOCLR |
		TCBD_CMDDMA_CRC32INIT_AUTOCLR;
	ret |= tcbd_io_funcs.reg_write(TCBD_CMDDMA_STARTCTRL, ctrl_data);
	ret |= tcbd_io_funcs.reg_write_burst_fix(
			TCBD_CMDDMA_DATA_WIND, _data, _size);

	tcpal_unlock(&tcbd_io_funcs.sem);
	return ret;
}

s32 tcbd_mem_read(
	struct tcbd_device *_device, u32 _addr,	u8 *_data, u32 _size)
{
	s32 ret = 0;
	u8 ctrl_data;
	u32 addr = SWAP32(_addr);
	u16 size = SWAP16((u16)(_size >> 2));

	if (_size <= 0 || _data == NULL || _device == NULL)
		return -TCERR_INVALID_ARG;

	tcpal_lock(&tcbd_io_funcs.sem);
	tcbd_io_funcs.chip_addr = _device->chip_addr;

	ctrl_data = TCBD_CMDDMA_DMAEN | TCBD_CMDDMA_READMODE;
	ret |= tcbd_io_funcs.reg_write(TCBD_CMDDMA_CTRL, ctrl_data);
	ret |= tcbd_io_funcs.reg_write_burst_cont(
		TCBD_CMDDMA_SADDR, (u8 *)&addr, sizeof(u32));
	ret |= tcbd_io_funcs.reg_write_burst_cont(
		TCBD_CMDDMA_SIZE, (u8 *)&size, sizeof(u16));
	ctrl_data =
		TCBD_CMDDMA_START_AUTOCLR |
		TCBD_CMDDMA_INIT_AUTOCLR |
		TCBD_CMDDMA_CRC32INIT_AUTOCLR;
	ret |= tcbd_io_funcs.reg_write(TCBD_CMDDMA_STARTCTRL, ctrl_data);
	ret |= tcbd_io_funcs.reg_read_burst_fix(
			TCBD_CMDDMA_DATA_WIND, _data, _size);

	tcpal_unlock(&tcbd_io_funcs.sem);
	return ret;
}

s32 tcbd_rf_reg_write(
	struct tcbd_device *_device, u8 _addr, u32 _data)
{
	s32 ret = 0;
	u32 data = SWAP32(_data);

	if (_device == NULL)
		return TCERR_INVALID_ARG;

	tcpal_lock(&tcbd_io_funcs.sem);
	tcbd_io_funcs.chip_addr = _device->chip_addr;

	ret |= tcbd_io_funcs.reg_write(
			TCBD_RF_CFG0,
			TCBD_RF_MANAGE_ENABLE|TCBD_RF_WRITE);
	ret |= tcbd_io_funcs.reg_write(TCBD_RF_CFG2, _addr);
	ret |= tcbd_io_funcs.reg_write_burst_cont(
		TCBD_RF_CFG3, (u8 *)&data, sizeof(u32));
	ret |= tcbd_io_funcs.reg_write(TCBD_RF_CFG1, TCBD_RF_ACTION);
	ret |= tcbd_io_funcs.reg_write(TCBD_RF_CFG0, TCBD_RF_MANAGE_DISABLE);

	tcpal_unlock(&tcbd_io_funcs.sem);
	return ret;
}

s32 tcbd_rf_reg_read(
	struct tcbd_device *_device, u32 _addr, u32 *_data)
{
	s32 ret = 0;
	u32 data = 0;

	if (_device == NULL)
		return TCERR_INVALID_ARG;

	tcpal_lock(&tcbd_io_funcs.sem);
	tcbd_io_funcs.chip_addr = _device->chip_addr;

	ret |= tcbd_io_funcs.reg_write(
			TCBD_RF_CFG0, TCBD_RF_MANAGE_ENABLE|TCBD_RF_READ);
	ret |= tcbd_io_funcs.reg_write(TCBD_RF_CFG2, _addr);
	ret |= tcbd_io_funcs.reg_write(TCBD_RF_CFG1, TCBD_RF_ACTION);
	ret |= tcbd_io_funcs.reg_read_burst_cont(
		TCBD_RF_CFG3, (u8 *)&data, sizeof(u32));
	ret |= tcbd_io_funcs.reg_write(TCBD_RF_CFG0, TCBD_RF_MANAGE_DISABLE);

	*_data = SWAP32(data);

	tcpal_unlock(&tcbd_io_funcs.sem);
	return ret;
}

s32 tcbd_send_mail(
	struct tcbd_device *_device, struct tcbd_mail_data *_mail)
{
	s32 ret = 0, count;
	u8 mail_data[32];
	u8 reg_data = 0;
	u64 time_tick, elapsed;

	tcpal_lock(&tcbd_io_funcs.sem);

	ret = tcbd_io_funcs.reg_write(TCBD_MAIL_CTRL, TCBD_MAIL_INIT);

	*((u32 *)mail_data) =
		(MB_HOSTMAIL << 24) | (_mail->count << 20) |
		(_mail->flag << 19) | (MB_ERR_OK << 16) | _mail->cmd;

	count = MIN(_mail->count, MAX_MAIL_COUNT);
	if (count > 0)
		memcpy(mail_data + sizeof(u32) ,
				_mail->data, count * sizeof(u32));

	ret |= tcbd_io_funcs.reg_write_burst_fix(
		TCBD_MAIL_FIFO_WIND, mail_data,
		sizeof(u32) + count * sizeof(u32));
	ret |= tcbd_io_funcs.reg_write(TCBD_MAIL_CTRL, TCBD_MAIL_HOSTMAILPOST);
	if (ret < 0)
		goto exit_send_mail;

	time_tick = tcpal_get_time();
	do {
		elapsed = tcpal_diff_time(time_tick);
		if (elapsed > (u64)MAX_TIME_TO_WAIT_MAIL) {
			ret = -TCERR_WAIT_MAIL_TIMEOUT;
			goto exit_send_mail;
		}
		/* latch mail status to register	  */
		ret = tcbd_io_funcs.reg_write(TCBD_MAIL_FIFO_W, 0x5E);
		/* read ratched status from register  */
		ret |= tcbd_io_funcs.reg_read(TCBD_MAIL_FIFO_W, &reg_data);
		if (ret < 0)
			break;
	} while (!(reg_data & 0x1)); /* check fifo status */

	tcbd_debug(DEBUG_DRV_IO, "cmd:0x%X, count:%d, elapsed time:%llu\n",
		_mail->cmd, count, elapsed);

exit_send_mail:
	tcpal_unlock(&tcbd_io_funcs.sem);
	return ret;
}

s32 tcbd_recv_mail(
	struct tcbd_device *_device, struct tcbd_mail_data *_mail)
{
	s32 ret = 0;
	s32 mail_hdr;
	u8 mail_data[32] = {0, };
	u8 reg_data = 0;
	u8 bytes_read;
	u64 time_tick, elapsed;

	tcpal_lock(&tcbd_io_funcs.sem);

	time_tick = tcpal_get_time();
	do {
		/* latch mail status to register	  */
		ret = tcbd_io_funcs.reg_write(TCBD_MAIL_FIFO_R, 0x5E);
		/* read ratched status from register  */
		ret |= tcbd_io_funcs.reg_read(TCBD_MAIL_FIFO_R, &reg_data);

		elapsed = tcpal_diff_time(time_tick);
		if (elapsed > (u64)MAX_TIME_TO_WAIT_MAIL)
			ret = -TCERR_WAIT_MAIL_TIMEOUT;

		if (ret < 0)
			goto exit_recv_mail;
	} while ((reg_data & 0xFC) < 3); /* check fifo status */
	bytes_read = (reg_data >> 2) & 0x3F;
	tcbd_debug(DEBUG_DRV_IO, "cmd:0x%X, bytes_read:%d, elapsed time:%llu\n",
			_mail->cmd, bytes_read, elapsed);

	ret = tcbd_io_funcs.reg_read_burst_fix(
			TCBD_MAIL_FIFO_WIND, mail_data, bytes_read);
	/*only warm boot cmd */
	if (bytes_read == 4) {
		memcpy(_mail->data, mail_data, bytes_read);
		goto exit_recv_mail;
	}

	mail_hdr = *((u32 *)mail_data);
	if ((mail_hdr >> 24) != MB_SLAVEMAIL) {
		tcbd_debug(DEBUG_ERROR, "Error : cmd=0x%X bytes_read=%d\n",
				_mail->cmd, bytes_read);
		tcbd_debug(DEBUG_ERROR, " [0x%02X][0x%02X][0x%02X][0x%02X]"
				"[0x%02X][0x%02X][0x%02X][0x%02X]\n",
				mail_data[0], mail_data[1], mail_data[2],
				mail_data[3], mail_data[4], mail_data[5],
				mail_data[6], mail_data[7]);

		ret = -TCERR_BROKEN_MAIL_HEADER;
		goto exit_recv_mail;
	}
	_mail->cmd = mail_hdr & 0xFFFF;
	_mail->status = (mail_hdr >> 16) & 0x7;
	if (_mail->status) {
		tcbd_debug(DEBUG_ERROR, "Mail Error : status=0x%X, cmd=0x%X\n",
				_mail->status, _mail->cmd);
		ret = -TCERR_UNKNOWN_MAIL_STATUS;
		goto exit_recv_mail;
	}
	_mail->count = (bytes_read >> 2) - 1;
	memcpy(_mail->data, mail_data + 4, bytes_read - 4);

exit_recv_mail:
	tcpal_unlock(&tcbd_io_funcs.sem);
	return ret;
}

s32 tcbd_read_mail_box(struct tcbd_device *_device, u16 _cmd, s32 _cnt,
								u32 *_data)
{
	s32 ret = 0;
	struct tcbd_mail_data mail = {0, };

	mail.flag = MB_CMD_READ;
	mail.cmd = _cmd;
	mail.count = _cnt;
	ret = tcbd_send_mail(_device, &mail);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "failed to send mail! %d\n", ret);
		goto exit_read_mail_box;
	}

	ret = tcbd_recv_mail(_device, &mail);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "failed to recv mail! %d\n", ret);
		goto exit_read_mail_box;
	}

	memcpy((void *)_data, (void *)mail.data, _cnt * sizeof(u32));

exit_read_mail_box:
	return ret;
}

s32 tcbd_write_mail_box(struct tcbd_device *_device, u16 _cmd, s32 _cnt,
								u32 *_data)
{
	u32 *data = NULL;
	struct tcbd_mail_data mail = {0, };

	if (_cnt > MAX_MAIL_COUNT || _data == NULL) {
		tcbd_debug(DEBUG_ERROR, "invalid param, cnt:%d\n", _cnt);
		return -TCERR_INVALID_ARG;
	}

	mail.flag = MB_CMD_WRITE;
	mail.cmd = _cmd;
	mail.count = _cnt;
	memcpy((void *)mail.data, (void *)_data, _cnt * sizeof(u32));
#if defined(__DEBUG_DSP_ROM__)
	tcbd_debug_mbox_tx(&_cmd, &_cnt, &data);
	if (data != NULL) {
		int i, sz = 0;
		char print_buff[80];
		mail.cmd = _cmd;
		mail.count = _cnt;
		memcpy((void *)mail.data, (void *)data, _cnt * sizeof(u32));
		for (i = 0; i < _cnt; i++)
			sz += sprintf(print_buff + sz, "[%08X]", data[i]);

		tcbd_debug(DEBUG_ERROR, "cmd 0x%X, cnt:%d, %s\n",
					_cmd, _cnt, print_buff);
	}
#endif /*__DEBUG_DSP_ROM__*/
	return tcbd_send_mail(_device, &mail);
}

#if defined(__DEBUG_DSP_ROM__)
s32 tcbd_read_file(struct tcbd_device *_device, char *_path, u8 *_buff,
								s32 _size)
{
	int ret;
	struct file *flip = NULL;
	mm_segment_t old_fs;

	if (_path == NULL) {
		tcbd_debug(DEBUG_ERROR, "invalid filename! %s\n", _path);
		return -1;
	}

	if (_buff == NULL) {
		tcbd_debug(DEBUG_ERROR, "Invaild pointer! 0x%X\n", (u32)_buff);
		return -1;
	}

	flip = filp_open(_path, O_RDWR, 0);
	if (IS_ERR(flip)) {
		tcbd_debug(DEBUG_ERROR, "%s open failed\n", _path);
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	flip->f_pos = 0;
	ret = vfs_read(flip, _buff, _size, &flip->f_pos);

	filp_close(flip, NULL);
	set_fs(old_fs);

	return ret;
}
#endif /*__DEBUG_DSP_ROM__*/
