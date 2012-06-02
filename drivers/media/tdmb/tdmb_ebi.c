/*
*
* drivers/media/tdmb/tdmb_ebi.c
*
* tdmb driver
*
* Copyright (C) (2011, Samsung Electronics)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include <linux/io.h>
#include "tdmb.h"

#define TDMB_BASE_ADDR_PHYS 0x98000000
void *addr_tdmb_cs4_v;

int tdmb_init_bus(void)
{
	addr_tdmb_cs4_v = ioremap(TDMB_BASE_ADDR_PHYS, PAGE_SIZE);
	DPRINTK("TDMB EBI2 Init addr_tdmb_cs4_v(0x%x)\n", addr_tdmb_cs4_v);
	return 0;
}

void tdmb_exit_bus(void)
{
	addr_tdmb_cs4_v = NULL;
}
