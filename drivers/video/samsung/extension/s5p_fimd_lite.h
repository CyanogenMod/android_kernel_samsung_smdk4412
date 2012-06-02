/* linux/arch/arm/plat-s5p/s5p_fimd_lite.h
 *
 * FIMD Lite Platform Specific Header Definitions.
 *
 * Copyright (c) 2010 Samsung Electronics
 * InKi Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S5P_FIMD_LITE_H_
#define _S5P_FIMD_LITE_H_

#include <drm/exynos_drm.h>

struct s5p_fimd_lite_platform_data {
	unsigned int reg_base;
	unsigned int reg_map_size;
	unsigned int irq;
};

struct s5p_fimd_lite {
	struct device		*dev;
	struct clk		*clk;
	void __iomem		*iomem_base;
	unsigned int		irq;
	unsigned int		dynamic_refresh;

	struct exynos_drm_fimd_pdata	*lcd;
};

void s5p_fimd_lite_get_vsync_interrupt(struct s5p_fimd_lite *fimd_lite,
					unsigned int enable);

#endif /* _S5P_FIMD_LITE_H_ */
