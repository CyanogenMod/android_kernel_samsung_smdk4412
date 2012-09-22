/*
 * tcpal_debug.c
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

static s32 tcbd_spur_dbg;

static s32 tcbd_spur_clock_config[5] = {0x60, 0x00, 0x0F, 0x02, 76800};
static s32 clock_config_count = 5;

static s32 tcbd_rom_from_fs;
static char *tcbd_rom_path = "/tmp/tcc3170.rom";

static u32 tcbd_mbox_rx[9] = {0, };
static u32 tcbd_mbox_tx[9*10] = {0, };

static s32 mbox_rx_data_len = 9;
static s32 mbox_tx_data_len = 9*10;

module_param(tcbd_rom_from_fs, int, 0664);
module_param(tcbd_rom_path, charp, 0664);
module_param(tcbd_spur_dbg, int, 0664);
module_param_array(tcbd_spur_clock_config, int, &clock_config_count, 0664);
module_param_array(tcbd_mbox_rx, int, &mbox_rx_data_len, 0664);
module_param_array(tcbd_mbox_tx, int, &mbox_tx_data_len, 0664);

void tcbd_debug_mbox_rx(u16 *_cmd, s32 *_cnt, u32 **_data)
{
	*_cmd = tcbd_mbox_rx[0];
	*_cnt = tcbd_mbox_rx[1];
	*_data = &tcbd_mbox_rx[2];
}

void tcbd_debug_mbox_tx(u16 *_cmd, s32 *_cnt, u32 **_data)
{
	s32 i;

	for (i = 0; i < 10; i++) {
		if (tcbd_mbox_tx[i * 9] != *_cmd)
			continue;
		*_cmd = tcbd_mbox_tx[i * 9 + 0];
		*_cnt = tcbd_mbox_tx[i * 9 + 1];
		*_data = &tcbd_mbox_tx[i * 9 + 2];
		break;
	}
}

s32 tcbd_debug_spur_dbg(void)
{
	return tcbd_spur_dbg;
}

s32 tcbd_debug_rom_from_fs(void)
{
	return tcbd_rom_from_fs;
}

s32 *tcbd_debug_spur_clk_cfg(void)
{
	return tcbd_spur_clock_config;
}

char *tcbd_debug_rom_path(void)
{
	return tcbd_rom_path;
}

u32 tcbd_debug_class =
	DEBUG_API_COMMON |
	/*DEBUG_DRV_PERI | */
	/*DEBUG_DRV_IO |   */
	/*DEBUG_DRV_COMP | */
	/*DEBUG_DRV_RF |   */
	/*DEBUG_TCPAL_OS |   */
	/*DEBUG_TCPAL_CSPI | */
	/*DEBUG_TCPAL_I2C |  */
	/*DEBUG_TCHAL |      */
	/*DEBUG_STREAM_READ | */
	/*DEBUG_STREAM_PARSER |*/
	/*DEBUG_PARSING_PROC | */
	/*DEBUG_INTRRUPT | */
	DEBUG_INFO |
	DEBUG_ERROR;

module_param(tcbd_debug_class, int, 0664);

