/*
 * Copyright (C) 2011 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef _SSP_PLATFORMDATA_H_
#define _SSP_PLATFORMDATA_H_

struct ssp_platform_data {
	int (*wakeup_mcu)(void);
	int (*check_mcu_ready)(void);
	int (*check_mcu_busy)(void);
	int (*set_mcu_reset)(int);
	int (*check_ap_rev)(void);
};
#endif
