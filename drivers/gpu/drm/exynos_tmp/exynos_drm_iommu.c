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
	int (*setup)(struct device *dev);
	void (*cleanup)(struct device *dev);
	int (*activate)(struct device *dev);
	void (*deactivate)(struct device *dev);
	dma_addr_t (*map)(struct device *dev, struct scatterlist *sg,
				off_t offset, size_t size);
	void (*unmap)(struct device *dev, dma_addr_t iova);
};

static const struct exynos_iommu_ops iommu_ops = {
	.setup		= iovmm_setup,
	.cleanup	= iovmm_cleanup,
	.activate	= iovmm_activate,
	.deactivate	= iovmm_deactivate,
	.map		= iovmm_map,
	.unmap		= iovmm_unmap
};

static bool check_iommu_map_params(struct iommu_gem_map_params *params)
{
	if (!params) {
		DRM_ERROR("params is null.\n");
		return false;
	}

	if (!params->dev || !params->drm_dev || !params->file) {
		DRM_ERROR("invalid params.\n");
		return false;
	}

	return true;
}

void exynos_drm_remove_iommu_list(struct list_head *iommu_list,
					void *gem_obj)
{
	struct iommu_info_node *im, *t_im;

	list_for_each_entry_safe(im, t_im, iommu_list, list) {
		if (im->gem_obj == gem_obj) {
			list_del(&im->list);
			kfree(im);
			im = NULL;
			break;
		}
	}

}

dma_addr_t exynos_drm_iommu_map_gem(struct iommu_gem_map_params *params,
					struct list_head *iommu_list,
					unsigned int gem_handle,
					enum iommu_types type)
{
	struct sg_table *sgt;
	struct iommu_info_node *node;
	struct exynos_drm_gem_obj *obj;
	dma_addr_t dma_addr;

	if (!is_iommu_type_valid(type)) {
		DRM_ERROR("invalid iommu type.\n");
		return 0;
	}

	if (!check_iommu_map_params(params))
		return 0;

	/* get gem object from specific gem framework. */
	obj = exynos_drm_gem_get_obj(params->drm_dev, gem_handle,
					params->file);
	if (IS_ERR(obj))
		return 0;

	mutex_lock(&iommu_mutex);

	/*
	 * if this gem object had already been mapped to iommu then
	 * return dma address mapped before this time.
	 */
	if (obj->iommu_info.mapped & (1 << type)) {
		DRM_DEBUG_KMS("already mapped to iommu");
		mutex_unlock(&iommu_mutex);
		return obj->iommu_info.dma_addrs[type];
	}

	sgt = obj->buffer->sgt;

	/*
	 * if not using iommu, just return base address to physical
	 * memory region of the gem.
	 */
	if (!iommu_ops.map) {
		mutex_unlock(&iommu_mutex);
		return sg_dma_address(&sgt->sgl[0]);
	}
	mutex_unlock(&iommu_mutex);

	/*
	 * allocate device address space for this driver and then
	 * map all pages contained in sg list to iommu table.
	 */
	dma_addr = iommu_ops.map(params->dev, sgt->sgl, (off_t)0,
					(size_t)obj->size);
	if (!dma_addr) {
		mutex_unlock(&iommu_mutex);
		return dma_addr;
	}

	mutex_lock(&iommu_mutex);

	/*
	 * check map flag bit and device address mapped to iommu.
	 * this data would be used to avoid duplicated mapping.
	 */
	obj->iommu_info.mapped |= (1 << type);
	obj->iommu_info.dma_addrs[type] = dma_addr;
	obj->iommu_info.devs[type] = params->dev;
	obj->iommu_info.iommu_lists[type] = iommu_list;

	params->gem_obj = obj;

	/*
	 * this gem object is referenced by this driver so
	 * the object refcount should be increased.
	 */
	drm_gem_object_reference(&obj->base);

	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (!node) {
		DRM_ERROR("failed to allocate iommu node.\n");
		dma_addr = 0;
		goto err;
	}

	node->gem_obj = obj;
	node->dma_addr = dma_addr;
	mutex_unlock(&iommu_mutex);

	list_add_tail(&node->list, iommu_list);

	return dma_addr;
err:
	mutex_unlock(&iommu_mutex);
	iommu_ops.unmap(params->dev, dma_addr);
	return dma_addr;
}

void exynos_drm_iommu_unmap_gem(struct iommu_gem_map_params *params,
				dma_addr_t dma_addr,
				enum iommu_types type)
{
	struct exynos_drm_gem_obj *obj;

	if (!iommu_ops.unmap)
		return;

	if (!is_iommu_type_valid(type)) {
		DRM_ERROR("invalid iommu type.\n");
		return;
	}

	if (!check_iommu_map_params(params))
		return;

	if (!params->gem_obj)
		return;

	obj = (struct exynos_drm_gem_obj *)params->gem_obj;

	mutex_lock(&iommu_mutex);
	if (!(obj->iommu_info.mapped & (1 << type))) {
		DRM_DEBUG_KMS("not already mapped to iommu so just return\n");
		mutex_unlock(&iommu_mutex);
		return;
	}

	/* uncheck map flag bit. */
	obj->iommu_info.mapped &= ~(1 << type);
	obj->iommu_info.dma_addrs[type] = 0;
	mutex_unlock(&iommu_mutex);

	iommu_ops.unmap(params->dev, dma_addr);

	/*
	 * drop this gem object refcount to release allocated buffer
	 * and resources.
	 */
	drm_gem_object_unreference_unlocked(&obj->base);
}

dma_addr_t exynos_drm_iommu_map(struct device *dev, dma_addr_t paddr,
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
		dev_err(dev, "failed to allocate sg table.\n");
		return dma_addr;
	}

	ret = sg_alloc_table(sgt, npages, GFP_KERNEL);
	if (ret < 0) {
		dev_err(dev, "failed to initialize sg table.\n");
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
	dma_addr = iommu_ops.map(dev, sgt->sgl, (off_t)0, (size_t)size);
	if (!dma_addr)
		dev_err(dev, "failed to map cmdlist pool.\n");

	sg_free_table(sgt);
err:
	kfree(sgt);
	sgt = NULL;

	return dma_addr;
}


void exynos_drm_iommu_unmap(struct device *dev, dma_addr_t dma_addr)
{
	if (iommu_ops.unmap)
		iommu_ops.unmap(dev, dma_addr);
}

int exynos_drm_iommu_setup(struct device *dev)
{
	/*
	 * allocate device address space to this driver and add vmm object
	 * to s5p_iovmm_list. please know that each iommu will use
	 * 1GB as its own device address apace.
	 *
	 * the device address space : 0x80000000 ~ 0xA0000000
	 */
	if (iommu_ops.setup)
		return iommu_ops.setup(dev);

	return 0;
}

int exynos_drm_iommu_activate(struct device *dev)
{
	if (iommu_ops.activate)
		return iovmm_activate(dev);

	return 0;
}

void exynos_drm_iommu_deactivate(struct device *dev)
{
	if (iommu_ops.deactivate)
		iommu_ops.deactivate(dev);
}

void exynos_drm_iommu_cleanup(struct device *dev)
{
	if (iommu_ops.cleanup)
		iommu_ops.cleanup(dev);
}

MODULE_AUTHOR("Inki Dae <inki.dae@samsung.com>");
MODULE_DESCRIPTION("Samsung SoC DRM IOMMU Framework");
MODULE_LICENSE("GPL");
