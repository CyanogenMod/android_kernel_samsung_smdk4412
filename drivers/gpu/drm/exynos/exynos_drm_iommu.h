/* exynos_drm_iommu.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 * Authoer: Inki Dae <inki.dae@samsung.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _EXYNOS_DRM_IOMMU_H_
#define _EXYNOS_DRM_IOMMU_H_

enum iommu_types {
	IOMMU_FIMD	= 0,
	IOMMU_HDMI,
	IOMMU_G2D,
	IOMMU_FIMC,
	IOMMU_G3D,
	IOMMU_ROTATOR,
	IOMMU_MASK	= 0x3f
};

struct iommu_gem_map_params {
	struct device		*dev;
	struct drm_device	*drm_dev;
	struct drm_file		*file;
	void			*gem_obj;
};

#define is_iommu_type_valid(t)	(((1 << (t)) & ~(IOMMU_MASK)) ? false : true)

void exynos_drm_remove_iommu_list(struct list_head *iommu_list,
					void *gem_obj);

/* get all pages to gem object and map them to iommu table. */
dma_addr_t exynos_drm_iommu_map_gem(struct iommu_gem_map_params *params,
					struct list_head *iommu_list,
					unsigned int gem_handle,
					enum iommu_types type);

/* unmap device address space to gem object from iommu table. */
void exynos_drm_iommu_unmap_gem(struct iommu_gem_map_params *params,
				dma_addr_t dma_addr,
				enum iommu_types type);

/* map physical memory region pointed by paddr to iommu table. */
dma_addr_t exynos_drm_iommu_map(struct device *dev, dma_addr_t paddr,
					size_t size);

/* unmap device address space pointed by dma_addr from iommu table. */
void exynos_drm_iommu_unmap(struct device *dev, dma_addr_t dma_addr);

/* setup device address space for device iommu. */
int exynos_drm_iommu_setup(struct device *dev);

int exynos_drm_iommu_activate(struct device *dev);

void exynos_drm_iommu_deactivate(struct device *dev);

/* clean up allocated device address space for device iommu. */
void exynos_drm_iommu_cleanup(struct device *dev);

#endif
