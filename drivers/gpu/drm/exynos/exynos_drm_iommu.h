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

struct exynos_iommu_gem_data {
	unsigned int	gem_handle_in;
	void		*gem_obj_out;
};

/* get all pages to gem object and map them to iommu table. */
dma_addr_t exynos_drm_iommu_map_gem(struct drm_device *drm_dev,
					struct drm_gem_object *obj);

/* unmap device address space to gem object from iommu table. */
void exynos_drm_iommu_unmap_gem(struct drm_gem_object *obj);

/* map physical memory region pointed by paddr to iommu table. */
dma_addr_t exynos_drm_iommu_map(void *in_vmm, dma_addr_t paddr,
					size_t size);

/* unmap device address space pointed by dev_addr from iommu table. */
void exynos_drm_iommu_unmap(void *in_vmm, dma_addr_t dev_addr);

/* setup device address space for device iommu. */
void *exynos_drm_iommu_setup(unsigned long s_iova, unsigned long size);

int exynos_drm_iommu_activate(void *in_vmm, struct device *dev);

void exynos_drm_iommu_deactivate(void *in_vmm, struct device *dev);

/* clean up allocated device address space for device iommu. */
void exynos_drm_iommu_cleanup(void *in_vmm);

#endif
