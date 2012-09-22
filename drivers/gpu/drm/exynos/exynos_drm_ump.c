/* exynos_drm_ump.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
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

#include "exynos_drm_gem.h"
#include "ump_kernel_interface_ref_drv.h"

static unsigned int exynos_drm_ump_get_handle(unsigned int id)
{
	return (unsigned int)ump_dd_handle_get((ump_secure_id)id);
}

static int exynos_drm_ump_add_buffer(void *obj,
		unsigned int *handle, unsigned int *id)
{
	struct exynos_drm_gem_obj *gem_obj = obj;
	struct exynos_drm_gem_buf *buf = gem_obj->buffer;
	ump_dd_physical_block *ump_mem_desc;
	unsigned int nblocks;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	if (IS_NONCONTIG_BUFFER(gem_obj->flags)) {
		unsigned int i = 0;

		if (!buf->pages)
			return -EFAULT;

		nblocks = gem_obj->size >> PAGE_SHIFT;
		ump_mem_desc = kzalloc(sizeof(*ump_mem_desc) * nblocks,
				GFP_KERNEL);
		if (!ump_mem_desc) {
			DRM_ERROR("failed to alloc ump_mem_desc.\n");
			return -ENOMEM;
		}

		/*
		 * if EXYNOS_BO_NONCONTIG type, gem object would already
		 * have pages allocated by gem creation so contain page
		 * frame numbers of all pages into ump descriptors.
		 */
		while (i < nblocks) {
			ump_mem_desc[i].addr =
				page_to_pfn(buf->pages[i]) << PAGE_SHIFT;
			ump_mem_desc[i].size = PAGE_SIZE;
			i++;
		}
	} else {
		nblocks = 1;

		ump_mem_desc = kzalloc(sizeof(*ump_mem_desc), GFP_KERNEL);
		if (!ump_mem_desc) {
			DRM_ERROR("failed to alloc ump_mem_desc.\n");
			return -ENOMEM;
		}

		/*
		 * it EXYNOS_DRM_GEM_PC type, gem would have just one
		 * physically continuous buffer so let a ump descriptor
		 * have one buffer address.
		 */
		ump_mem_desc[0].addr = (unsigned long)buf->paddr;
		ump_mem_desc[0].size = buf->size;
	}

	/*
	 * register memory information to ump descriptor table through
	 * the ump descriptor data and then return ump handle to it so that
	 * user can access the memory region through it.
	 */
	*handle = (unsigned int)
		ump_dd_handle_create_from_phys_blocks(ump_mem_desc, nblocks);
	if (!(*handle)) {
		DRM_ERROR("failed to create ump handle.\n");
		kfree(ump_mem_desc);
		return -EINVAL;
	}

	*id = ump_dd_secure_id_get((ump_dd_handle)*handle);

	kfree(ump_mem_desc);

	DRM_DEBUG_KMS("ump handle : 0x%x, secure id = %d\n", *handle, *id);
	DRM_INFO("ump handle : 0x%x, secure id = %d\n", *handle, *id);

	return 0;
}

static void exynos_drm_ump_release_buffer(unsigned int handle)
{
	DRM_DEBUG_KMS("%s\n", __FILE__);
}

static struct exynos_drm_private_cb ump_callback = {
	.get_handle	= exynos_drm_ump_get_handle,
	.add_buffer	= exynos_drm_ump_add_buffer,
	.release_buffer = exynos_drm_ump_release_buffer,
};

static int exynos_drm_ump_init(void)
{
	DRM_DEBUG_KMS("%s\n", __FILE__);

	exynos_drm_priv_cb_register(&ump_callback);

	return 0;
}

static void exynos_drm_ump_exit(void)
{
}

subsys_initcall(exynos_drm_ump_init);
module_exit(exynos_drm_ump_exit);

MODULE_AUTHOR("Inki Dae <inki.dae@samsung.com>");
MODULE_DESCRIPTION("Samsung SoC DRM UMP Backend Module");
MODULE_LICENSE("GPL");
