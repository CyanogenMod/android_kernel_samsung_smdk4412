/*
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_SI4705_PDATA_H
#define _LINUX_SI4705_PDATA_H

#include <linux/types.h>

#ifdef __KERNEL__

#define SI4705_PDATA_BIT_VOL_STEPS		(1 << 0)
#define SI4705_PDATA_BIT_VOL_TABLE		(1 << 1)
#define SI4705_PDATA_BIT_RSSI_THRESHOLD		(1 << 2)
#define SI4705_PDATA_BIT_SNR_THRESHOLD		(1 << 3)

struct si4705_pdata {
	void (*reset)(int enable);
	u16 pdata_values;
	int rx_vol_steps;
	u16 rx_vol_table[16];
	u16 rx_seek_tune_rssi_threshold;
	u16 rx_seek_tune_snr_threshold;
};

#endif

#endif
