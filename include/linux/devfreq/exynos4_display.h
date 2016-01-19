/* include/linux/devfreq/exynos4_display.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 *	Chanwoo Choi <cw00.choi@samsung.com>
 *	Myungjoo Ham <myungjoo.ham@samsung.com>
 *	Kyungmin Park <kyungmin.park@samsung.com>
 *
 * EXYNOS4 - Dynamic LCD refresh rate support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_EXYNOS4_DISPLAY_H
#define __ASM_ARCH_EXYNOS4_DISPLAY_H __FILE__

#define EXYNOS4_DISPLAY_LV_HF		60
#define EXYNOS4_DISPLAY_LV_LF		40
#define EXYNOS4_DISPLAY_LV_DEFAULT	EXYNOS4_DISPLAY_LV_HF

/* Register/unregister display client to exynos4_display */
extern int exynos4_display_register_client(struct notifier_block *nb);
extern int exynos4_display_unregister_client(struct notifier_block *nb);

#endif	/* __ASM_ARCH_EXYNOS4_DISPLAY_H */
