/* linux/driver/video/samsung/s5p_fimd_ext.h
 *
 * Samsung SoC FIMD Extension Framework Header.
 *
 * InKi Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S5P_FIMD_EXT_H
#define _S5P_FIMD_EXT_H

#include <linux/device.h>

struct s5p_fimd_ext_device
		*to_fimd_ext_device(struct device *dev);

struct resource *s5p_fimd_ext_get_resource(struct s5p_fimd_ext_device *fx_dev,
		unsigned int type, unsigned int num);

int s5p_fimd_ext_get_irq(struct s5p_fimd_ext_device *fx_dev,
		unsigned int num);

int s5p_fimd_ext_device_register(struct s5p_fimd_ext_device *fx_dev);

int s5p_fimd_ext_driver_register(struct s5p_fimd_ext_driver *fx_drv);

#endif /* _S5P_FIMD_EXT_H */
