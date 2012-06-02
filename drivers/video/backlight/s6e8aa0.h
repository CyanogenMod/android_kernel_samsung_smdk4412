/* linux/drivers/video/samsung/s6e8aa0.h
 *
 * MIPI-DSI based s6e8aa0 AMOLED LCD Panel definitions.
 *
 * Copyright (c) 2011 Samsung Electronics
 *
 * Inki Dae, <inki.dae@samsung.com>
 * Donghwa Lee <dh09.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S6E8AA0_H
#define _S6E8AA0_H

extern void s6e8aa0_init(void);
extern void s6e8aa0_set_link(void *pd, unsigned int dsim_base,
	unsigned char (*cmd_write) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2),
	unsigned char (*cmd_read) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2));

#endif

