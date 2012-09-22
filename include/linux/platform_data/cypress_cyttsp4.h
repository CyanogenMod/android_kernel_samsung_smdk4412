/*
 * Copyright (C) 2010 Motorola Mobility, Inc.
 * Modified by Cypress Semiconductor 2011-2012
 *    - increase touch_settings.size from uint8_t to uint32_t
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

/* Defines generic platform structures for touch drivers */
#ifndef _LINUX_TOUCH_PLATFORM_H
#define _LINUX_TOUCH_PLATFORM_H

#include <linux/types.h>

enum {
	GPIO_TOUCH_nINT = 0,
	GPIO_TOUCH_EN,
	GPIO_TOUCH_SCL,
	GPIO_TOUCH_SDA
};

struct touch_settings {
	const uint8_t   *data;
	uint32_t         size;
	uint8_t         tag;
} __attribute__ ((packed));

struct touch_firmware {
	const uint8_t   *img;
	uint32_t        size;
	const uint8_t   *ver;
	uint8_t         vsize;
} __attribute__ ((packed));

struct touch_framework {
	const uint16_t  *abs;
	uint8_t         size;
	uint8_t         enable_vkeys;
} __attribute__ ((packed));

struct touch_platform_data {
	struct touch_settings   *sett[256];
	struct touch_firmware   *fw;
	struct touch_framework  *frmwrk;

	uint8_t         addr[2];
	uint16_t        flags;

	int         (*hw_reset)(void);
	int        (*hw_power)(int);
	int         (*hw_recov)(int);
	int         (*irq_stat)(void);
} __attribute__ ((packed));

#endif /* _LINUX_TOUCH_PLATFORM_H */
