/*
 *  arch/arm/mach-exynos/include/mach/p4-input.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __kona_INPUT_H
#define __kona_INPUT_H __FILE__

void kona_tsp_init(u32 system_rev);
void kona_key_init(void);
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_S7301)
extern void synaptics_ts_charger_infom(bool en);
#endif

#endif /* __kona_INPUT_H */
