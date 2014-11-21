/*
 * Copyright (C) 2011 Samsung Electronics, Inc.
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

#ifndef _SII9234_H_
#define _SII9234_H_

#ifdef __KERNEL__

struct sii9234_platform_data {
	void (*hw_reset)(void);
	void (*hw_off)(void);
};

#endif

#ifdef	CONFIG_SAMSUNG_WORKAROUND_HPD_GLANCE
extern	void mhl_hpd_handler(bool onoff);
#endif
#endif
