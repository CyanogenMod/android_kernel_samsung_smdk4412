/* exynos_drm_iommu.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 * Author: Inki Dae <inki.dae@samsung.com>
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

#include "drmP.h"
#include "drm.h"

#include <drm/exynos_drm.h>

#include <plat/s5p-iovmm.h>

#include "exynos_drm_drv.h"
#include "exynos_drm_gem.h"
#include "exynos_drm_iommu.h"

static DEFINE_MUTEX(iommu_mutex);

struct exynos_iommu_ops {
	void *(*setup)(unsigned long s_iova, unsigned long size);
	void (*cleanup)(void *in_vmm);
	int (*activate)(void *in_vmm, struct device *dev);
	void (*deactivate)(void *in_vmm, struct device *dev);
	dma_addr_t (*map)(void *in_vmm, struct scatterlist *sg,
				off_t offset, size_t size);
	void (*unmap)(void *in_vmm, dma_addr_t iova);
};

static const struct exynos_iommu_ops iommu_ops = {
	.setup		= iovmm_setup,
	.cleanup	= iovmm_cleanup,
	.activate	= iovmm_activate,
	.deactivate	= iovmm_deactivate,
	.map		= iovmm_map,
	.unmap		= iovmm_unmap
};

dma_addr_t exynos_drm_iommu_map_gem(struct drm_device *drm_dev,
					struct drm_gem_object *obj)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct exynos_drm_gem_buf *buf;
	struct sg_table *sgt;
	dma_addr_t dev_addr;

	mutex_lock(&iommu_mutex);

	exynos_gem_obj = to_exynos_gem_obj(obj);

	buf = exynos_gem_obj->buffer;
	sgt = buf->sgt;

	/*
	 * if not using iommu, just return base address to physical
	 * memory region of the gem.
	 */
	if (!iommu_ops.map) {
		mutex_unlock(&iommu_mutex);
		return sg_dma_address(&sgt->sgl[0]);
	}

	/*
	 * if a gem buffer was already mapped with iommu table then
	 * just return dev_addr;
	 *
	 * Note: device address is unique to system globally.
	 */
	if (buf->dev_addr) {
		mutex_unlock(&iommu_mutex);
		return buf->dev_addr;
	}

	/*
	 * allocate device address space for this driver and then
	 * map all pages contained in sg list to iommu table.
	 */
	dev_addr = iommu_ops.map(exynos_gem_obj->vmm, sgt->sgl, (off_t)0,
					(size_t)obj->size);
	if (!dev_addr) {
		mutex_unlock(&iommu_mutex);
		return dev_addr;
	}

	mutex_unlock(&iommu_mutex);

	return dev_addr;
}

void exynos_drm_iommu_unmap_gem(struct drm_gem_object *obj)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct exynos_drm_gem_buf *buf;

	if (!iommu_ops.unmap || !obj)
		return;

	exynos_gem_obj = to_exynos_gem_obj(obj);
	buf = exynos_gem_obj->buffer;

	/* workaround */
	usleep_range(15000, 20000);

	mutex_lock(&iommu_mutex);

	if (!buf->dev_addr) {
		mutex_unlock(&iommu_mutex);
		DRM_DEBUG_KMS("not mapped with iommu table.\n");
		return;
	}

	if (exynos_gem_obj->vmm)
		iommu_ops.unmap(exynos_gem_obj->vmm, buf->dev_addr);

	buf->dev_addr = 0;
	mutex_unlock(&iommu_mutex);
}

dma_addr_t exynos_drm_iommu_map(void *in_vmm, dma_addr_t paddr,
				size_t size)
{
	struct sg_table *sgt;
	struct scatterlist *sgl;
	dma_addr_t dma_addr = 0, tmp_addr;
	unsigned int npages, i = 0;
	int ret;

	 /* if not using iommu, just return paddr. */
	if (!iommu_ops.map)
		return paddr;

	npages = size >> PAGE_SHIFT;

	sgt = kzalloc(sizeof(struct sg_table) * npages, GFP_KERNEL);
	if (!sgt) {
		DRM_ERROR("failed to allocate sg table.\n");
		return dma_addr;
	}

	ret = sg_alloc_table(sgt, npages, GFP_KERNEL);
	if (ret < 0) {
		DRM_ERROR("failed to initialize sg table.\n");
		goto err;
	}

	sgl = sgt->sgl;
	tmp_addr = paddr;

	while (i < npages) {
		struct page *page = phys_to_page(tmp_addr);
		sg_set_page(sgl, page, PAGE_SIZE, 0);
		sg_dma_len(sgl) = PAGE_SIZE;
		tmp_addr += PAGE_SIZE;
		i++;
		sgl = sg_next(sgl);
	}

	/*
	 * allocate device address space for this driver and then
	 * map all pages contained in sg list to iommu table.
	 */
	dma_addr = iommu_ops.map(in_vmm, sgt->sgl, (off_t)0, (size_t)size);
	if (!dma_addr)
		DRM_ERROR("failed to map cmdlist pool.\n");

	sg_free_table(sgt);
err:
	kfree(sgt);
	sgt = NULL;

	return dma_addr;
}


void exynos_drm_iommu_unmap(void *in_vmm, dma_addr_t dma_addr)
{
	if (iommu_ops.unmap)
		iommu_ops.unmap(in_vmm, dma_addr);
}

void *exynos_drm_iommu_setup(unsigned long s_iova, unsigned long size)
{
	/*
	 * allocate device address space to this driver and add vmm object
	 * to s5p_iovmm_list. please know that each iommu will use
	 * 1GB as its own device address apace.
	 *
	 * the device address space : s_iova ~ s_iova + size
	 */
	if (iommu_ops.setup)
		return iommu_ops.setup(s_iova, size);

	return ERR_PTR(-EINVAL);
}

int exynos_drm_iommu_activate(void *in_vmm, struct device *dev)
{
	if (iommu_ops.activate)
		return iovmm_activate(in_vmm, dev);

	return 0;
}

void exynos_drm_iommu_deactivate(void *in_vmm, struct device *dev)
{
	if (iommu_ops.deactivate)
		iommu_ops.deactivate(in_vmm, dev);
}

void exynos_drm_iommu_cleanup(void *in_vmm)
{
	if (iommu_ops.cleanup)
		iommu_ops.cleanup(in_vmm);
}

MODULE_AUTHOR("Inki Dae <inki.dae@samsung.com>");
MODULE_DESCRIPTION("Samsung SoC DRM IOMMU Framework");
MODULE_LICENSE("GPL");
