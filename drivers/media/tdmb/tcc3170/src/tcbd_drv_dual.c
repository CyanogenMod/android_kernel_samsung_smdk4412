/*
 * tcbd_drv_dual.c
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

s32 tcbd_change_chip_addr(struct tcbd_device *_device, u8 addr)
{
	_device->chip_addr = addr;
	return tcbd_reg_write(_device, TCBD_CHIPADDR, addr);
}

s32 tcbd_enable_slave_command_ack(struct tcbd_device *_device)
{
	s32 ret = 0;
	u16 outData = SWAP16((0x1 << 15));
	u16 outEnable = SWAP16((0x1 << 15));

	ret |= tcbd_reg_write_burst_cont(
		_device, TCBD_GPIO_LR, (u8 *)&outData, 2);
	ret |= tcbd_reg_write_burst_cont(
		_device, TCBD_GPIO_DR, (u8 *)&outEnable, 2);

	return ret;
}

s32 tcbd_init_div_io(
	struct tcbd_device *_device, enum tcbd_div_io_type _divIo)
{
	return tcbd_reg_write(_device, TCBD_DIVIO, (u8)_divIo);
}
