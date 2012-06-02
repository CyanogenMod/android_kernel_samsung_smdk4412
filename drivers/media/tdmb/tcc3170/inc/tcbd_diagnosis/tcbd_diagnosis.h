/*
 * tcbd_diagnosis.h
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

#ifndef __TCBD_DIAGNOSIS_H__
#define __TCBD_DIAGNOSIS_H__

/* 0 agc, 1 cto, 2 cfo, 3 fto, 4 sync, 5, ofdm */
#define TCBD_LOCK_AGC   0x01
#define TCBD_LOCK_CTO   0x02
#define TCBD_LOCK_CFO   0x03
#define TCBD_LOCK_FTO   0x04
#define TCBD_LOCK_SYNC  0x10
#define TCDD_LOCK_OFDM  0x20

struct tcbd_status_data {
	u8 lock;
	s32 rssi;
	s32 snr;
	s32 snr_moving_avg;
	s32 pcber;
	s32 pcber_moving_avg;
	s32 vber;
	s32 vber_moving_avg;
	s32 ts_error_count;
	s32 tsper;
	s32 tsper_moving_avg;
};

TCBB_FUNC void tcbd_update_status(
	u8 *_rawData, u32 size, struct tcbd_status_data *_status_data);
TCBB_FUNC void tcbd_init_status_manager(void);

#endif /*__TCBD_SIGNAL_INFO_H__*/
