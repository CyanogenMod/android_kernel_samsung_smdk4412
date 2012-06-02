/*
 * tcbd_drv_ip.h
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

#ifndef __TCBD_DRV_IP_H__
#define __TCBD_DRV_IP_H__

enum tcbd_remap_type {
	EP_RAM0_RAM1 = 0, /**< Ep can access RAM0 and RAM1 */
	EP_RAM0_RAM1_PC0,  /**< Ep can access RAM0, RAM1 and initial pc is 0 */
	OP_RAM0_RAM1_PC2,  /**< Op can access RAM0, RAM1 and initial pc is 0x2000*/
	OP_RAM0_EP_RAM1_PC2 /**< Op can access RAM0. Ep can access RAM1.
						 *  Initial pc is 0x2000 */
};

TCBB_FUNC s32 tcbd_send_spur_data(
	struct tcbd_device *_device, s32 _freq_khz);
TCBB_FUNC s32 tcbd_send_agc_data(
	struct tcbd_device *_device, enum tcbd_band_type _band_type);
TCBB_FUNC s32 tcbd_send_frequency(
	struct tcbd_device *_device, s32 _freq_khz);
TCBB_FUNC s32 tcbd_send_service_info(struct tcbd_device *_device);

TCBB_FUNC s32 tcbd_get_rom_version(
	struct tcbd_device *_device, u32 *_bootVersion);

TCBB_FUNC s32 tcbd_enable_buffer(struct tcbd_device *_device);
TCBB_FUNC s32 tcbd_disable_buffer(struct tcbd_device *_device);
TCBB_FUNC s32 tcbd_init_buffer_region(struct tcbd_device *_device);
TCBB_FUNC s32 tcbd_change_memory_view(
	struct tcbd_device *_device, enum tcbd_remap_type _remap);

TCBB_FUNC s32 tcbd_demod_tune_frequency(
	struct tcbd_device *_device, u32 _freq_khz,	s32 _bw_khz);

TCBB_FUNC s32 tcbd_dsp_cold_start(struct tcbd_device *_device);
TCBB_FUNC s32 tcbd_dsp_warm_start(struct tcbd_device *_device);

TCBB_FUNC s32 tcbd_change_chip_addr(
	struct tcbd_device *_device, u8 addr);
TCBB_FUNC s32 tcbd_enable_slave_command_ack(
	struct tcbd_device *_device);

TCBB_FUNC s32 tcbd_enable_peri(struct tcbd_device *_device);
TCBB_FUNC s32 tcbd_disable_peri(struct tcbd_device *_device);
TCBB_FUNC s32 tcbd_init_bias_key(struct tcbd_device *_device);
TCBB_FUNC u32 tcbd_get_osc_clock(struct tcbd_device *_device);
TCBB_FUNC s32 tcbd_init_pll(
	struct tcbd_device *_device, enum tcbd_clock_type _clock_type);
TCBB_FUNC s32 tcbd_init_div_io(
	struct tcbd_device *_device, enum tcbd_div_io_type _div_io);
TCBB_FUNC s32 tcbd_init_dsp(
	struct tcbd_device *_device, u8 *_boot_code, s32 _size);

TCBB_FUNC s32 tcbd_check_dsp_status(struct tcbd_device *_device);
#endif /*__TCBD_DRV_IP_H__*/
