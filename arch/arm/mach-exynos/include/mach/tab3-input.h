/*
 *  arch/arm/mach-exynos/include/mach/p4-input.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __P4_INPUT_H
#define __P4_INPUT_H __FILE__

void tab3_tsp_init(u32 system_rev);
void p4_wacom_init(void);
void tab3_key_init(void);

#if defined(CONFIG_TOUCHSCREEN_MMS152_TAB3) || \
	defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1188S)
extern void tsp_charger_infom(bool en);
#endif

#endif /* __P4_INPUT_H */
