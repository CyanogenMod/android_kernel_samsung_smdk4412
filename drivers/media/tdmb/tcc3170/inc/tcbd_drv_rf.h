/*
 * tcbd_drv_rf.h
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

#ifndef __TCBD_DRV_RF_H__
#define __TCBD_DRV_RF_H__

TCBB_FUNC s32 tcbd_rf_init(
	struct tcbd_device *_device, enum tcbd_band_type _band);
TCBB_FUNC s32 tcbd_rf_tune_frequency(
	struct tcbd_device *_device, u32 _freq_khz, s32 _bw_khz);

#endif /*__TCBD_DRV_RF_H__*/
