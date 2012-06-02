/*
 * tcbd_hal.h
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

#ifndef __TCBD_HAL_H__
#define __TCBD_HAL_H__

#define IRQ_TC317X 6
/* #define __USE_TC_CPU__ */

void tchal_init(void);
void tchal_reset_device(void);
void tchal_power_on_device(void);
void tchal_power_down_device(void);
void tchal_irq_setup(void);

#endif /*__TCBD_HAL_H__*/
