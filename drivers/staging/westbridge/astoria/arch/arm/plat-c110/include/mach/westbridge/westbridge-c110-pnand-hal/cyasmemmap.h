/*
 OMAP3430 ZOOM MDK astoria interface defs(cyasmemmap.h)
## ===========================
## Copyright (C) 2010  Cypress Semiconductor
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor
## Boston, MA  02110-1301, USA.
## ===========================
*/
/* include does not seem to work
 * moving for patch submission
#include <mach/gpmc.h>
#include <mach/mux.h>
*/
#include <mach/gpio.h>
#include <mach/irqs.h>
#include <mach/map.h>

#ifndef _INCLUDED_CYASMEMMAP_H_
#define _INCLUDED_CYASMEMMAP_H_

#define HIGH 1
#define LOW  0

#if 0
#define WB_RESET	S5PV210_GPG3(3)
#define WB_WAKEUP		S5PV210_GPG3(4)
#define WB_CLK_EN		S5PV210_GPG3(6)

#define WB_CYAS_INT	S5PV210_GPG3(5)
#endif

#define WB_RESET	EXYNOS4_GPY1(3)
#define WB_WAKEUP		EXYNOS4_GPY4(4)
#define WB_CLK_EN		EXYNOS4_GPY2(5)

#define WB_CYAS_INT	EXYNOS4_GPC0(3)

#define WB_CYAS_IRQ_INT gpio_to_irq(EXYNOS4_GPC0(3))
#define WB_SDCD_INT	EXYNOS4_GPX3(4)
#define WB_SDCD_IRQ_INT	gpio_to_irq(EXYNOS4_GPX3(4))

#define WB_AP_T_FLASH_DETECT	EXYNOS4_GPC0(1)
#define S5PC110_PA_SROMC 0xE8000000
/*
 * Physical address
 * we use CS For mapping in C110 RAM space
 */
#define CYAS_DEV_BASE_ADDR  (S5P_PA_NAND)
#define CYAS_DEV_MAX_ADDR   (0xFF)
#define CYAS_DEV_ADDR_RANGE (CYAS_DEV_MAX_ADDR << 1)

/* in CRAM or PSRAM mode OMAP A1..An wires-> Astoria, there is no A0 line */
#define CYAS_DEV_CALC_ADDR(cyas_addr) (cyas_addr << 1)
#define CYAS_DEV_CALC_EP_ADDR(ep) (ep << 1)

/*
 * i/o access macros
 */
#define IORD32(addr) (*(volatile u32  *)(addr))
#define IOWR32(addr, val) (*(volatile u32 *)(addr) = val)

#define IORD16(addr) (*(volatile u16  *)(addr))
#define IOWR16(addr, val) (*(volatile u16 *)(addr) = val)

#define IORD8(addr) (*(volatile u8  *)(addr))
#define IOWR8(addr, val) (*(volatile u8 *)(addr) = val)


#endif /* _INCLUDED_CYASMEMMAP_H_ */

/*[]*/
