/* exynos_drm_gem.c
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
#include <linux/shmem_fs.h>
#include <linux/dma-buf.h>

#include "exynos_drm_drv.h"
#include "exynos_drm_gem.h"
#include "exynos_drm_buf.h"
#include "exynos_drm_iommu.h"

#define USERPTR_MAX_SIZE		SZ_64M

static struct exynos_drm_private_cb *private_cb;

void exynos_drm_priv_cb_register(struct exynos_drm_private_cb *cb)
{
	if (cb)
		private_cb = cb;
}

int register_buf_to_priv_mgr(struct exynos_drm_gem_obj *obj,
		unsigned int *priv_handle, unsigned int *priv_id)
{
	if (private_cb && private_cb->add_buffer)
		return private_cb->add_buffer(obj, priv_handle, priv_id);

	return 0;
}

static unsigned int convert_to_vm_err_msg(int msg)
{
	unsigned int out_msg;

	switch (msg) {
	case 0:
	case -ERESTARTSYS:
	case -EINTR:
		out_msg = VM_FAULT_NOPAGE;
		break;

	case -ENOMEM:
		out_msg = VM_FAULT_OOM;
		break;

	default:
		out_msg = VM_FAULT_SIGBUS;
		break;
	}

	return out_msg;
}

static int check_gem_flags(unsigned int flags)
{
	if (flags & ~(EXYNOS_BO_MASK)) {
		DRM_ERROR("invalid flags.\n");
		return -EINVAL;
	}

	return 0;
}

static int check_cache_flags(unsigned int flags)
{
	if (flags & ~(EXYNOS_DRM_CACHE_SEL_MASK | EXYNOS_DRM_CACHE_OP_MASK)) {
		DRM_ERROR("invalid flags.\n");
		return -EINVAL;
	}

	return 0;
}

static struct vm_area_struct *get_vma(struct vm_area_struct *vma)
{
	struct vm_area_struct *vma_copy;

	vma_copy = kmalloc(sizeof(*vma_copy), GFP_KERNEL);
	if (!vma_copy)
		return NULL;

	if (vma->vm_ops && vma->vm_ops->open)
		vma->vm_ops->open(vma);

	if (vma->vm_file)
		get_file(vma->vm_file);

	memcpy(vma_copy, vma, sizeof(*vma));

	vma_copy->vm_mm = NULL;
	vma_copy->vm_next = NULL;
	vma_copy->vm_prev = NULL;

	return vma_copy;
}

static void put_vma(struct vm_area_struct *vma)
{
	if (!vma)
		return;

	if (vma->vm_ops && vma->vm_ops->close)
		vma->vm_ops->close(vma);

	if (vma->vm_file)
		fput(vma->vm_file);

	kfree(vma);
}

/*
 * lock_userptr_vma - lock VMAs within user address space
 *
 * this function locks vma within user address space to avoid pages
 * to the userspace from being swapped out.
 * if this vma isn't locked, the pages to the userspace could be swapped out
 * so unprivileged user might access different pages and dma of any device
 * could access physical memory region not intended once swap-in.
 */
static int lock_userptr_vma(struct exynos_drm_gem_buf *buf, unsigned int lock)
{
	struct vm_area_struct *vma;
	unsigned long start, end;

	start = buf->userptr;
	end = buf->userptr + buf->size - 1;

	down_write(&current->mm->mmap_sem);

	do {
		vma = find_vma(current->mm, start);
		if (!vma) {
			up_write(&current->mm->mmap_sem);
			return -EFAULT;
		}

		if (lock)
			vma->vm_flags |= VM_LOCKED;
		else
			vma->vm_flags &= ~VM_LOCKED;

		start = vma->vm_end + 1;
	} while (vma->vm_end < end);

	up_write(&current->mm->mmap_sem);

	return 0;
}

static void update_vm_cache_attr(struct exynos_drm_gem_obj *obj,
					struct vm_area_struct *vma)
{
	DRM_DEBUG_KMS("flags = 0x%x\n", obj->flags);

	/* non-cachable as default. */
	if (obj->flags & EXYNOS_BO_CACHABLE)
		vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
	else if (obj->flags & EXYNOS_BO_WC)
		vma->vm_page_prot =
			pgprot_writecombine(vm_get_page_prot(vma->vm_flags));
	else
		vma->vm_page_prot =
			pgprot_noncached(vm_get_page_prot(vma->vm_flags));
}

static unsigned long roundup_gem_size(unsigned long size, unsigned int flags)
{
	if (!IS_NONCONTIG_BUFFER(flags)) {
		if (size >= SZ_1M)
			return roundup(size, SECTION_SIZE);
		else if (size >= SZ_64K)
			return roundup(size, SZ_64K);
		else
			goto out;
	}
out:
	return roundup(size, PAGE_SIZE);
}

struct page **exynos_gem_get_pages(struct drm_gem_object *obj,
						gfp_t gfpmask)
{
	struct page *p, **pages;
	int i, npages;

	npages = obj->size >> PAGE_SHIFT;

	pages = drm_malloc_ab(npages, sizeof(struct page *));
	if (pages == NULL)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < npages; i++) {
		p = alloc_page(gfpmask);
		if (IS_ERR(p))
			goto fail;
		pages[i] = p;
	}

	return pages;

fail:
	while (--i)
		__free_page(pages[i]);

	drm_free_large(pages);
	return ERR_PTR(PTR_ERR(p));
}

static void exynos_gem_put_pages(struct drm_gem_object *obj,
					struct page **pages)
{
	int npages;

	npages = obj->size >> PAGE_SHIFT;

	while (--npages >= 0)
		__free_page(pages[npages]);

	drm_free_large(pages);
}

static int exynos_drm_gem_map_pages(struct drm_gem_object *obj,
					struct vm_area_struct *vma,
					unsigned long f_vaddr,
					pgoff_t page_offset)
{
	struct exynos_drm_gem_obj *exynos_gem_obj = to_exynos_gem_obj(obj);
	struct exynos_drm_gem_buf *buf = exynos_gem_obj->buffer;
	unsigned long pfn;

	if (exynos_gem_obj->flags & EXYNOS_BO_NONCONTIG) {
		if (!buf->pages)
			return -EINTR;

		pfn = page_to_pfn(buf->pages[page_offset++]);
	} else
		pfn = (buf->paddr >> PAGE_SHIFT) + page_offset;

	return vm_insert_mixed(vma, f_vaddr, pfn);
}

static int exynos_drm_gem_get_pages(struct drm_gem_object *obj)
{
	struct exynos_drm_gem_obj *exynos_gem_obj = to_exynos_gem_obj(obj);
	struct exynos_drm_gem_buf *buf = exynos_gem_obj->buffer;
	struct scatterlist *sgl;
	struct page **pages;
	unsigned int npages, i = 0;
	int ret;

	if (buf->pages) {
		DRM_DEBUG_KMS("already allocated.\n");
		return -EINVAL;
	}

	pages = exynos_gem_get_pages(obj, GFP_HIGHUSER_MOVABLE);
	if (IS_ERR(pages)) {
		DRM_ERROR("failed to get pages.\n");
		return PTR_ERR(pages);
	}

	npages = obj->size >> PAGE_SHIFT;
	buf->page_size = PAGE_SIZE;

	buf->sgt = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!buf->sgt) {
		DRM_ERROR("failed to allocate sg table.\n");
		ret = -ENOMEM;
		goto err;
	}

	ret = sg_alloc_table(buf->sgt, npages, GFP_KERNEL);
	if (ret < 0) {
		DRM_ERROR("failed to initialize sg table.\n");
		ret = -EFAULT;
		goto err1;
	}

	sgl = buf->sgt->sgl;

	/* set all pages to sg list. */
	while (i < npages) {
		sg_set_page(sgl, pages[i], PAGE_SIZE, 0);
		sg_dma_address(sgl) = page_to_phys(pages[i]);
		i++;
		sgl = sg_next(sgl);
	}

	buf->pages = pages;
	return ret;
err1:
	kfree(buf->sgt);
	buf->sgt = NULL;
err:
	exynos_gem_put_pages(obj, pages);
	return ret;

}

static void exynos_drm_gem_put_pages(struct drm_gem_object *obj)
{
	struct exynos_drm_gem_obj *exynos_gem_obj = to_exynos_gem_obj(obj);
	struct exynos_drm_gem_buf *buf = exynos_gem_obj->buffer;

	/*
	 * if buffer typs is EXYNOS_BO_NONCONTIG then release all pages
	 * allocated at gem fault handler.
	 */
	sg_free_table(buf->sgt);
	kfree(buf->sgt);
	buf->sgt = NULL;

	exynos_gem_put_pages(obj, buf->pages);
	buf->pages = NULL;

	/* add some codes for UNCACHED type here. TODO */
}

static void exynos_drm_put_userptr(struct drm_gem_object *obj)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct exynos_drm_gem_buf *buf;
	struct vm_area_struct *vma;
	int npages;

	exynos_gem_obj = to_exynos_gem_obj(obj);
	buf = exynos_gem_obj->buffer;
	vma = exynos_gem_obj->vma;

	if (vma && (vma->vm_flags & VM_PFNMAP) && (vma->vm_pgoff)) {
		put_vma(exynos_gem_obj->vma);
		goto out;
	}

	npages = buf->size >> PAGE_SHIFT;

	if (exynos_gem_obj->flags & EXYNOS_BO_USERPTR && !buf->pfnmap)
		lock_userptr_vma(buf, 0);

	npages--;
	while (npages >= 0) {
		if (buf->write)
			set_page_dirty_lock(buf->pages[npages]);

		put_page(buf->pages[npages]);
		npages--;
	}

out:
	kfree(buf->pages);
	buf->pages = NULL;

	kfree(buf->sgt);
	buf->sgt = NULL;
}

static int exynos_drm_gem_handle_create(struct drm_gem_object *obj,
					struct drm_file *file_priv,
					unsigned int *handle)
{
	int ret;

	/*
	 * allocate a id of idr table where the obj is registered
	 * and handle has the id what user can see.
	 */
	ret = drm_gem_handle_create(file_priv, obj, handle);
	if (ret)
		return ret;

	DRM_DEBUG_KMS("gem handle = 0x%x\n", *handle);

	/* drop reference from allocate - handle holds it now. */
	drm_gem_object_unreference_unlocked(obj);

	return 0;
}

void exynos_drm_gem_destroy(struct exynos_drm_gem_obj *exynos_gem_obj)
{
	struct drm_gem_object *obj;
	struct exynos_drm_gem_buf *buf;
	struct exynos_drm_private *private;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	obj = &exynos_gem_obj->base;
	private = obj->dev->dev_private;
	buf = exynos_gem_obj->buffer;

	DRM_DEBUG_KMS("handle count = %d\n", atomic_read(&obj->handle_count));

	/*
	 * release a private buffer from its table.
	 *
	 * this callback will release a ump object only if user requested
	 * ump export otherwise just return.
	 */
	if (private_cb->release_buffer)
		private_cb->release_buffer(exynos_gem_obj->priv_handle);

	if (!buf->pages)
		return;

	/*
	 * do not release memory region from exporter.
	 *
	 * the region will be released by exporter
	 * once dmabuf's refcount becomes 0.
	 */
	if (obj->import_attach)
		goto out;

	if (private->vmm)
		exynos_drm_iommu_unmap_gem(obj);

	if (exynos_gem_obj->flags & EXYNOS_BO_NONCONTIG)
		exynos_drm_gem_put_pages(obj);
	else if (exynos_gem_obj->flags & EXYNOS_BO_USERPTR)
		exynos_drm_put_userptr(obj);
	else
		exynos_drm_free_buf(obj->dev, exynos_gem_obj->flags, buf);

out:
	exynos_drm_fini_buf(obj->dev, buf);
	exynos_gem_obj->buffer = NULL;

	if (obj->map_list.map)
		drm_gem_free_mmap_offset(obj);

	/* release file pointer to gem object. */
	drm_gem_object_release(obj);

	kfree(exynos_gem_obj);
	exynos_gem_obj = NULL;
}

struct exynos_drm_gem_obj *exynos_drm_gem_get_obj(struct drm_device *dev,
						unsigned int gem_handle,
						struct drm_file *file_priv)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct drm_gem_object *obj;

	obj = drm_gem_object_lookup(dev, file_priv, gem_handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		return ERR_PTR(-EINVAL);
	}

	exynos_gem_obj = to_exynos_gem_obj(obj);

	drm_gem_object_unreference_unlocked(obj);

	return exynos_gem_obj;
}

unsigned long exynos_drm_gem_get_size(struct drm_device *dev,
						unsigned int gem_handle,
						struct drm_file *file_priv)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct drm_gem_object *obj;

	obj = drm_gem_object_lookup(dev, file_priv, gem_handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		return 0;
	}

	exynos_gem_obj = to_exynos_gem_obj(obj);

	drm_gem_object_unreference_unlocked(obj);

	return exynos_gem_obj->buffer->size;
}


struct exynos_drm_gem_obj *exynos_drm_gem_init(struct drm_device *dev,
						      unsigned long size)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct drm_gem_object *obj;
	int ret;

	exynos_gem_obj = kzalloc(sizeof(*exynos_gem_obj), GFP_KERNEL);
	if (!exynos_gem_obj) {
		DRM_ERROR("failed to allocate exynos gem object\n");
		return NULL;
	}

	exynos_gem_obj->size = size;
	obj = &exynos_gem_obj->base;

	ret = drm_gem_object_init(dev, obj, size);
	if (ret < 0) {
		DRM_ERROR("failed to initialize gem object\n");
		kfree(exynos_gem_obj);
		return NULL;
	}

	DRM_DEBUG_KMS("created file object = 0x%x\n", (unsigned int)obj->filp);

	return exynos_gem_obj;
}

struct exynos_drm_gem_obj *exynos_drm_gem_create(struct drm_device *dev,
						unsigned int flags,
						unsigned long size)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct exynos_drm_private *private = dev->dev_private;
	struct exynos_drm_gem_buf *buf;
	unsigned long packed_size = size;
	int ret;

	if (!size) {
		DRM_ERROR("invalid size.\n");
		return ERR_PTR(-EINVAL);
	}

	size = roundup_gem_size(size, flags);
	DRM_DEBUG_KMS("%s\n", __FILE__);

	ret = check_gem_flags(flags);
	if (ret)
		return ERR_PTR(ret);

	buf = exynos_drm_init_buf(dev, size);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	exynos_gem_obj = exynos_drm_gem_init(dev, size);
	if (!exynos_gem_obj) {
		ret = -ENOMEM;
		goto err_fini_buf;
	}

	exynos_gem_obj->packed_size = packed_size;
	exynos_gem_obj->buffer = buf;

	/* set memory type and cache attribute from user side. */
	exynos_gem_obj->flags = flags;

	/*
	 * allocate all pages as desired size if user wants to allocate
	 * physically non-continuous memory.
	 */
	if (flags & EXYNOS_BO_NONCONTIG) {
		ret = exynos_drm_gem_get_pages(&exynos_gem_obj->base);
		if (ret < 0) {
			drm_gem_object_release(&exynos_gem_obj->base);
			goto err_fini_buf;
		}
	} else {
		ret = exynos_drm_alloc_buf(dev, buf, flags);
		if (ret < 0) {
			drm_gem_object_release(&exynos_gem_obj->base);
			goto err_fini_buf;
		}
	}

	if (private->vmm) {
		exynos_gem_obj->vmm = private->vmm;

		buf->dev_addr = exynos_drm_iommu_map_gem(dev,
							&exynos_gem_obj->base);
		if (!buf->dev_addr) {
			DRM_ERROR("failed to map gem with iommu table.\n");
			ret = -EFAULT;

			if (flags & EXYNOS_BO_NONCONTIG)
				exynos_drm_gem_put_pages(&exynos_gem_obj->base);
			else
				exynos_drm_free_buf(dev, flags, buf);

			drm_gem_object_release(&exynos_gem_obj->base);

			goto err_fini_buf;
		}

		buf->dma_addr = buf->dev_addr;
	 } else
		buf->dma_addr = buf->paddr;

	DRM_DEBUG_KMS("dma_addr = 0x%x\n", buf->dma_addr);

	return exynos_gem_obj;

err_fini_buf:
	exynos_drm_fini_buf(dev, buf);
	return ERR_PTR(ret);
}

int exynos_drm_gem_create_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv)
{
	struct drm_exynos_gem_create *args = data;
	struct exynos_drm_gem_obj *exynos_gem_obj;
	int ret;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	exynos_gem_obj = exynos_drm_gem_create(dev, args->flags, args->size);
	if (IS_ERR(exynos_gem_obj))
		return PTR_ERR(exynos_gem_obj);

	ret = exynos_drm_gem_handle_create(&exynos_gem_obj->base, file_priv,
			&args->handle);
	if (ret) {
		exynos_drm_gem_destroy(exynos_gem_obj);
		return ret;
	}

	return ret;
}

void *exynos_drm_gem_get_dma_addr(struct drm_device *dev,
					unsigned int gem_handle,
					struct drm_file *filp,
					unsigned int *gem_obj)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct exynos_drm_gem_buf *buf;
	struct drm_gem_object *obj;

	obj = drm_gem_object_lookup(dev, filp, gem_handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		return ERR_PTR(-EINVAL);
	}

	exynos_gem_obj = to_exynos_gem_obj(obj);
	buf = exynos_gem_obj->buffer;

	*gem_obj = (unsigned int)obj;

	return &buf->dma_addr;
}

void exynos_drm_gem_put_dma_addr(struct drm_device *dev, void *gem_obj)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct drm_gem_object *obj;

	if (!gem_obj)
		return;

	/* use gem handle instead of object. TODO */

	obj = gem_obj;

	exynos_gem_obj = to_exynos_gem_obj(obj);

	/*
	 * unreference this gem object because this had already been
	 * referenced at exynos_drm_gem_get_dma_addr().
	 */
	drm_gem_object_unreference_unlocked(obj);
}

int exynos_drm_gem_map_offset_ioctl(struct drm_device *dev, void *data,
				    struct drm_file *file_priv)
{
	struct drm_exynos_gem_map_off *args = data;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	DRM_DEBUG_KMS("handle = 0x%x, offset = 0x%lx\n",
			args->handle, (unsigned long)args->offset);

	if (!(dev->driver->driver_features & DRIVER_GEM)) {
		DRM_ERROR("does not support GEM.\n");
		return -ENODEV;
	}

	return exynos_drm_gem_dumb_map_offset(file_priv, dev, args->handle,
			&args->offset);
}

static int exynos_drm_gem_mmap_buffer(struct file *filp,
				      struct vm_area_struct *vma)
{
	struct drm_gem_object *obj = filp->private_data;
	struct exynos_drm_gem_obj *exynos_gem_obj = to_exynos_gem_obj(obj);
	struct exynos_drm_gem_buf *buffer;
	unsigned long pfn, vm_size, usize, uaddr = vma->vm_start;
	int ret;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	vma->vm_flags |= (VM_IO | VM_RESERVED);

	update_vm_cache_attr(exynos_gem_obj, vma);

	vma->vm_file = filp;

	vm_size = usize = vma->vm_end - vma->vm_start;

	/*
	 * a buffer contains information to physically continuous memory
	 * allocated by user request or at framebuffer creation.
	 */
	buffer = exynos_gem_obj->buffer;

	/* check if user-requested size is valid. */
	if (vm_size > buffer->size)
		return -EINVAL;

	if (exynos_gem_obj->flags & EXYNOS_BO_NONCONTIG) {
		int i = 0;

		if (!buffer->pages)
			return -EINVAL;

		vma->vm_flags |= VM_MIXEDMAP;

		do {
			ret = vm_insert_page(vma, uaddr, buffer->pages[i++]);
			if (ret) {
				DRM_ERROR("failed to remap user space.\n");
				return ret;
			}

			uaddr += PAGE_SIZE;
			usize -= PAGE_SIZE;
		} while (usize > 0);
	} else {
		/*
		 * get page frame number to physical memory to be mapped
		 * to user space.
		 */
		pfn = ((unsigned long)exynos_gem_obj->buffer->paddr) >>
								PAGE_SHIFT;

		DRM_DEBUG_KMS("pfn = 0x%lx\n", pfn);

		if (remap_pfn_range(vma, vma->vm_start, pfn, vm_size,
					vma->vm_page_prot)) {
			DRM_ERROR("failed to remap pfn range.\n");
			return -EAGAIN;
		}
	}

	return 0;
}

static const struct file_operations exynos_drm_gem_fops = {
	.mmap = exynos_drm_gem_mmap_buffer,
};

int exynos_drm_gem_mmap_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct drm_exynos_gem_mmap *args = data;
	struct drm_gem_object *obj;
	unsigned int addr;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	if (!(dev->driver->driver_features & DRIVER_GEM)) {
		DRM_ERROR("does not support GEM.\n");
		return -ENODEV;
	}

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		return -EINVAL;
	}

	obj->filp->f_op = &exynos_drm_gem_fops;
	obj->filp->private_data = obj;

	down_write(&current->mm->mmap_sem);
	addr = do_mmap(obj->filp, 0, args->size,
			PROT_READ | PROT_WRITE, MAP_SHARED, 0);
	up_write(&current->mm->mmap_sem);

	drm_gem_object_unreference_unlocked(obj);

	if (IS_ERR((void *)addr))
		return PTR_ERR((void *)addr);

	args->mapped = addr;

	DRM_DEBUG_KMS("mapped = 0x%lx\n", (unsigned long)args->mapped);

	return 0;
}

static int exynos_drm_get_userptr(struct drm_device *dev,
				struct exynos_drm_gem_obj *obj,
				unsigned long userptr,
				unsigned int write)
{
	unsigned int get_npages;
	unsigned long npages = 0;
	struct vm_area_struct *vma;
	struct exynos_drm_gem_buf *buf = obj->buffer;
	int ret;

	down_read(&current->mm->mmap_sem);
	vma = find_vma(current->mm, userptr);

	/* the memory region mmaped with VM_PFNMAP. */
	if (vma && (vma->vm_flags & VM_PFNMAP) && (vma->vm_pgoff)) {
		unsigned long this_pfn, prev_pfn, pa;
		unsigned long start, end, offset;
		struct scatterlist *sgl;
		int ret;

		start = userptr;
		offset = userptr & ~PAGE_MASK;
		end = start + buf->size;
		sgl = buf->sgt->sgl;

		for (prev_pfn = 0; start < end; start += PAGE_SIZE) {
			ret = follow_pfn(vma, start, &this_pfn);
			if (ret)
				goto err;

			if (prev_pfn == 0) {
				pa = this_pfn << PAGE_SHIFT;
				buf->paddr = pa + offset;
			} else if (this_pfn != prev_pfn + 1) {
				ret = -EINVAL;
				goto err;
			}

			sg_dma_address(sgl) = (pa + offset);
			sg_dma_len(sgl) = PAGE_SIZE;
			prev_pfn = this_pfn;
			pa += PAGE_SIZE;
			npages++;
			sgl = sg_next(sgl);
		}

		obj->vma = get_vma(vma);
		if (!obj->vma) {
			ret = -ENOMEM;
			goto err;
		}

		up_read(&current->mm->mmap_sem);
		buf->pfnmap = true;

		return npages;
err:
		buf->paddr = 0;
		up_read(&current->mm->mmap_sem);

		return ret;
	}

	up_read(&current->mm->mmap_sem);

	/*
	 * lock the vma within userptr to avoid userspace buffer
	 * from being swapped out.
	 */
	ret = lock_userptr_vma(buf, 1);
	if (ret < 0) {
		DRM_ERROR("failed to lock vma for userptr.\n");
		lock_userptr_vma(buf, 0);
		return 0;
	}

	buf->write = write;
	npages = buf->size >> PAGE_SHIFT;

	down_read(&current->mm->mmap_sem);
	get_npages = get_user_pages(current, current->mm, userptr,
					npages, write, 1, buf->pages, NULL);
	up_read(&current->mm->mmap_sem);
	if (get_npages != npages)
		DRM_ERROR("failed to get user_pages.\n");

	buf->userptr = userptr;
	buf->pfnmap = false;

	return get_npages;
}

int exynos_drm_gem_userptr_ioctl(struct drm_device *dev, void *data,
				      struct drm_file *file_priv)
{
	struct exynos_drm_private *priv = dev->dev_private;
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct drm_exynos_gem_userptr *args = data;
	struct exynos_drm_gem_buf *buf;
	struct scatterlist *sgl;
	unsigned long size, userptr, packed_size;
	unsigned int npages;
	int ret, get_npages;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	if (!args->size) {
		DRM_ERROR("invalid size.\n");
		return -EINVAL;
	}

	ret = check_gem_flags(args->flags);
	if (ret)
		return ret;

	packed_size = args->size;

	size = roundup_gem_size(args->size, EXYNOS_BO_USERPTR);

	if (size > priv->userptr_limit) {
		DRM_ERROR("excessed maximum size of userptr.\n");
		return -EINVAL;
	}

	userptr = args->userptr;

	buf = exynos_drm_init_buf(dev, size);
	if (!buf)
		return -ENOMEM;

	exynos_gem_obj = exynos_drm_gem_init(dev, size);
	if (!exynos_gem_obj) {
		ret = -ENOMEM;
		goto err_free_buffer;
	}

	exynos_gem_obj->packed_size = packed_size;

	buf->sgt = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!buf->sgt) {
		DRM_ERROR("failed to allocate buf->sgt.\n");
		ret = -ENOMEM;
		goto err_release_gem;
	}

	npages = size >> PAGE_SHIFT;

	ret = sg_alloc_table(buf->sgt, npages, GFP_KERNEL);
	if (ret < 0) {
		DRM_ERROR("failed to initailize sg table.\n");
		goto err_free_sgt;
	}

	buf->pages = kzalloc(npages * sizeof(struct page *), GFP_KERNEL);
	if (!buf->pages) {
		DRM_ERROR("failed to allocate buf->pages\n");
		ret = -ENOMEM;
		goto err_free_table;
	}

	exynos_gem_obj->buffer = buf;

	get_npages = exynos_drm_get_userptr(dev, exynos_gem_obj, userptr, 1);
	if (get_npages != npages) {
		DRM_ERROR("failed to get user_pages.\n");
		ret = get_npages;
		goto err_release_userptr;
	}

	ret = exynos_drm_gem_handle_create(&exynos_gem_obj->base, file_priv,
						&args->handle);
	if (ret < 0) {
		DRM_ERROR("failed to create gem handle.\n");
		goto err_release_userptr;
	}

	sgl = buf->sgt->sgl;

	/*
	 * if buf->pfnmap is true then update sgl of sgt with pages but
	 * if buf->pfnmap is false then it means the sgl was updated already
	 * so it doesn't need to update the sgl.
	 */
	if (!buf->pfnmap) {
		unsigned int i = 0;

		/* set all pages to sg list. */
		while (i < npages) {
			sg_set_page(sgl, buf->pages[i], PAGE_SIZE, 0);
			sg_dma_address(sgl) = page_to_phys(buf->pages[i]);
			i++;
			sgl = sg_next(sgl);
		}
	}

	/* always use EXYNOS_BO_USERPTR as memory type for userptr. */
	exynos_gem_obj->flags |= EXYNOS_BO_USERPTR;

	if (priv->vmm) {
		exynos_gem_obj->vmm = priv->vmm;

		buf->dev_addr = exynos_drm_iommu_map_gem(dev,
							&exynos_gem_obj->base);
		if (!buf->dev_addr) {
			DRM_ERROR("failed to map gem with iommu table.\n");
			ret = -EFAULT;

			exynos_drm_free_buf(dev, exynos_gem_obj->flags, buf);

			drm_gem_object_release(&exynos_gem_obj->base);

			goto err_release_handle;
		}

		buf->dma_addr = buf->dev_addr;
	 } else
		buf->dma_addr = buf->paddr;

	return 0;

err_release_handle:
	drm_gem_handle_delete(file_priv, args->handle);
err_release_userptr:
	get_npages--;
	while (get_npages >= 0)
		put_page(buf->pages[get_npages--]);
	kfree(buf->pages);
	buf->pages = NULL;
err_free_table:
	sg_free_table(buf->sgt);
err_free_sgt:
	kfree(buf->sgt);
	buf->sgt = NULL;
err_release_gem:
	drm_gem_object_release(&exynos_gem_obj->base);
	kfree(exynos_gem_obj);
	exynos_gem_obj = NULL;
err_free_buffer:
	exynos_drm_free_buf(dev, 0, buf);
	return ret;
}

int exynos_drm_gem_get_ioctl(struct drm_device *dev, void *data,
				      struct drm_file *file_priv)
{	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct drm_exynos_gem_info *args = data;
	struct drm_gem_object *obj;

	mutex_lock(&dev->struct_mutex);

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		mutex_unlock(&dev->struct_mutex);
		return -EINVAL;
	}

	exynos_gem_obj = to_exynos_gem_obj(obj);

	args->flags = exynos_gem_obj->flags;
	args->size = exynos_gem_obj->size;

	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

int exynos_drm_gem_user_limit_ioctl(struct drm_device *dev, void *data,
				      struct drm_file *filp)
{
	struct exynos_drm_private *priv = dev->dev_private;
	struct drm_exynos_user_limit *limit = data;

	if (limit->userptr_limit < PAGE_SIZE ||
			limit->userptr_limit > USERPTR_MAX_SIZE) {
		DRM_DEBUG_KMS("invalid userptr_limit size.\n");
		return -EINVAL;
	}

	if (priv->userptr_limit == limit->userptr_limit)
		return 0;

	priv->userptr_limit = limit->userptr_limit;

	return 0;
}

int exynos_drm_gem_export_ump_ioctl(struct drm_device *dev, void *data,
		struct drm_file *file)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct drm_gem_object *obj;
	struct drm_exynos_gem_ump *ump = data;
	int ret;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	mutex_lock(&dev->struct_mutex);

	obj = drm_gem_object_lookup(dev, file, ump->gem_handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		mutex_unlock(&dev->struct_mutex);
		return -EINVAL;
	}

	exynos_gem_obj = to_exynos_gem_obj(obj);

	/* register gem buffer to private buffer. */
	ret = register_buf_to_priv_mgr(exynos_gem_obj,
				(unsigned int *)&exynos_gem_obj->priv_handle,
				(unsigned int *)&exynos_gem_obj->priv_id);
	if (ret < 0)
		goto err_unreference_gem;

	ump->secure_id = exynos_gem_obj->priv_id;
	drm_gem_object_unreference(obj);

	mutex_unlock(&dev->struct_mutex);

	DRM_DEBUG_KMS("got secure id = %d\n", ump->secure_id);

	return 0;

err_unreference_gem:
	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);
	return ret;

}

static int exynos_gem_l1_cache_ops(struct drm_device *drm_dev,
					struct drm_exynos_gem_cache_op *op) {
	if (op->flags & EXYNOS_DRM_CACHE_FSH_ALL) {
		/*
		 * cortex-A9 core has individual l1 cache so flush l1 caches
		 * for all cores but other cores should be considered later.
		 * TODO
		 */
		if (op->flags & EXYNOS_DRM_ALL_CORES)
			flush_all_cpu_caches();
		else
			__cpuc_flush_user_all();

	} else if (op->flags & EXYNOS_DRM_CACHE_FSH_RANGE) {
		struct vm_area_struct *vma;

		down_read(&current->mm->mmap_sem);
		vma = find_vma(current->mm, op->usr_addr);
		up_read(&current->mm->mmap_sem);

		if (!vma) {
			DRM_ERROR("failed to get vma.\n");
			return -EFAULT;
		}

		__cpuc_flush_user_range(op->usr_addr, op->usr_addr + op->size,
					vma->vm_flags);
	}

	return 0;
}

static int exynos_gem_l2_cache_ops(struct drm_device *drm_dev,
				struct drm_file *filp,
				struct drm_exynos_gem_cache_op *op)
{
	if (op->flags & EXYNOS_DRM_CACHE_FSH_RANGE ||
			op->flags & EXYNOS_DRM_CACHE_INV_RANGE ||
			op->flags & EXYNOS_DRM_CACHE_CLN_RANGE) {
		unsigned long virt_start = op->usr_addr, pfn;
		phys_addr_t phy_start, phy_end;
		struct vm_area_struct *vma;
		int ret;

		down_read(&current->mm->mmap_sem);
		vma = find_vma(current->mm, op->usr_addr);
		up_read(&current->mm->mmap_sem);

		if (!vma) {
			DRM_ERROR("failed to get vma.\n");
			return -EFAULT;
		}

		/*
		 * Range operation to l2 cache(PIPT)
		 */
		if (vma && (vma->vm_flags & VM_PFNMAP)) {
			ret = follow_pfn(vma, virt_start, &pfn);
			if (ret < 0) {
				DRM_ERROR("failed to get pfn.\n");
				return ret;
			}

			/*
			 * the memory region with VM_PFNMAP is contiguous
			 * physically so do range operagion just one time.
			 */
			phy_start = pfn << PAGE_SHIFT;
			phy_end = phy_start + op->size;

			if (op->flags & EXYNOS_DRM_CACHE_FSH_RANGE)
				outer_flush_range(phy_start, phy_end);
			else if (op->flags & EXYNOS_DRM_CACHE_INV_RANGE)
				outer_inv_range(phy_start, phy_end);
			else if (op->flags & EXYNOS_DRM_CACHE_CLN_RANGE)
				outer_clean_range(phy_start, phy_end);

			return 0;
		} else {
			struct exynos_drm_gem_obj *exynos_obj;
			struct exynos_drm_gem_buf *buf;
			struct drm_gem_object *obj;
			struct scatterlist *sgl;
			unsigned int npages, i = 0;

			mutex_lock(&drm_dev->struct_mutex);

			obj = drm_gem_object_lookup(drm_dev, filp,
							op->gem_handle);
			if (!obj) {
				DRM_ERROR("failed to lookup gem object.\n");
				mutex_unlock(&drm_dev->struct_mutex);
				return -EINVAL;
			}

			exynos_obj = to_exynos_gem_obj(obj);
			buf = exynos_obj->buffer;
			npages = buf->size >> PAGE_SHIFT;
			sgl = buf->sgt->sgl;

			drm_gem_object_unreference(obj);
			mutex_unlock(&drm_dev->struct_mutex);

			/*
			 * in this case, the memory region is non-contiguous
			 * physically  so do range operation to all the pages.
			 */
			while (i < npages) {
				phy_start = sg_dma_address(sgl);
				phy_end = phy_start + buf->page_size;

				if (op->flags & EXYNOS_DRM_CACHE_FSH_RANGE)
					outer_flush_range(phy_start, phy_end);
				else if (op->flags & EXYNOS_DRM_CACHE_INV_RANGE)
					outer_inv_range(phy_start, phy_end);
				else if (op->flags & EXYNOS_DRM_CACHE_CLN_RANGE)
					outer_clean_range(phy_start, phy_end);

				i++;
				sgl = sg_next(sgl);
			}

			return 0;
		}
	}

	if (op->flags & EXYNOS_DRM_CACHE_FSH_ALL)
		outer_flush_all();
	else if (op->flags & EXYNOS_DRM_CACHE_INV_ALL)
		outer_inv_all();
	else if (op->flags & EXYNOS_DRM_CACHE_CLN_ALL)
		outer_clean_all();
	else {
		DRM_ERROR("invalid l2 cache operation.\n");
		return -EINVAL;
	}


	return 0;
}

int exynos_drm_gem_cache_op_ioctl(struct drm_device *drm_dev, void *data,
		struct drm_file *file_priv)
{
	struct drm_exynos_gem_cache_op *op = data;
	int ret;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	ret = check_cache_flags(op->flags);
	if (ret)
		return -EINVAL;

	/*
	 * do cache operation for all cache range if op->size is bigger
	 * than SZ_1M because cache range operation with bit size has
	 * big cost.
	 */
	if (op->size >= SZ_1M) {
		if (op->flags & EXYNOS_DRM_CACHE_FSH_RANGE) {
			if (op->flags & EXYNOS_DRM_L1_CACHE)
				__cpuc_flush_user_all();

			if (op->flags & EXYNOS_DRM_L2_CACHE)
				outer_flush_all();

			return 0;
		} else if (op->flags & EXYNOS_DRM_CACHE_INV_RANGE) {
			if (op->flags & EXYNOS_DRM_L2_CACHE)
				outer_inv_all();

			return 0;
		} else if (op->flags & EXYNOS_DRM_CACHE_CLN_RANGE) {
			if (op->flags & EXYNOS_DRM_L2_CACHE)
				outer_clean_all();

			return 0;
		}
	}

	if (op->flags & EXYNOS_DRM_L1_CACHE ||
			op->flags & EXYNOS_DRM_ALL_CACHES) {
		ret = exynos_gem_l1_cache_ops(drm_dev, op);
		if (ret < 0)
			goto err;
	}

	if (op->flags & EXYNOS_DRM_L2_CACHE ||
			op->flags & EXYNOS_DRM_ALL_CACHES)
		ret = exynos_gem_l2_cache_ops(drm_dev, file_priv, op);
err:
	return ret;
}

/* temporary functions. */
int exynos_drm_gem_get_phy_ioctl(struct drm_device *drm_dev, void *data,
		struct drm_file *file_priv)
{
	struct drm_exynos_gem_get_phy *get_phy = data;
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct drm_gem_object *obj;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	mutex_lock(&drm_dev->struct_mutex);

	obj = drm_gem_object_lookup(drm_dev, file_priv, get_phy->gem_handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		mutex_unlock(&drm_dev->struct_mutex);
		return -EINVAL;
	}

	exynos_gem_obj = to_exynos_gem_obj(obj);

	/*
	 * we can get physical address only for EXYNOS_DRM_GEM_PC memory type.
	 */
	if (exynos_gem_obj->flags & EXYNOS_BO_NONCONTIG) {
		DRM_DEBUG_KMS("not physically continuous memory type.\n");
		drm_gem_object_unreference(obj);
		mutex_unlock(&drm_dev->struct_mutex);
		return -EINVAL;
	}

	get_phy->phy_addr = exynos_gem_obj->buffer->paddr;
	get_phy->size = exynos_gem_obj->buffer->size;

	drm_gem_object_unreference(obj);
	mutex_unlock(&drm_dev->struct_mutex);

	return 0;
}

int exynos_drm_gem_phy_imp_ioctl(struct drm_device *drm_dev, void *data,
				 struct drm_file *file_priv)
{
	struct drm_exynos_gem_phy_imp *args = data;
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct exynos_drm_private *private = drm_dev->dev_private;
	struct exynos_drm_gem_buf *buffer;
	unsigned long size, packed_size;
	unsigned int flags = EXYNOS_BO_CONTIG;
	unsigned int npages, i = 0;
	struct scatterlist *sgl;
	dma_addr_t start_addr;
	int ret = 0;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	packed_size = args->size;
	size = roundup_gem_size(args->size, flags);

	exynos_gem_obj = exynos_drm_gem_init(drm_dev, size);
	if (!exynos_gem_obj)
		return -ENOMEM;

	buffer = exynos_drm_init_buf(drm_dev, size);
	if (!buffer) {
		DRM_DEBUG_KMS("failed to allocate buffer\n");
		ret = -ENOMEM;
		goto err_release_gem_obj;
	}

	exynos_gem_obj->packed_size = packed_size;
	buffer->paddr = (dma_addr_t)args->phy_addr;
	buffer->size = size;

	/*
	 * if shared is true, this bufer wouldn't be released.
	 * this buffer was allocated by other so don't release it.
	 */
	buffer->shared = true;

	exynos_gem_obj->buffer = buffer;

	ret = exynos_drm_gem_handle_create(&exynos_gem_obj->base, file_priv,
			&args->gem_handle);
	if (ret)
		goto err_fini_buf;

	DRM_DEBUG_KMS("got gem handle = 0x%x\n", args->gem_handle);

	if (buffer->size >= SZ_1M) {
		npages = buffer->size >> SECTION_SHIFT;
		buffer->page_size = SECTION_SIZE;
	} else if (buffer->size >= SZ_64K) {
		npages = buffer->size >> 16;
		buffer->page_size = SZ_64K;
	} else {
		npages = buffer->size >> PAGE_SHIFT;
		buffer->page_size = PAGE_SIZE;
	}

	buffer->sgt = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!buffer->sgt) {
		DRM_ERROR("failed to allocate sg table.\n");
		ret = -ENOMEM;
		goto err_release_handle;
	}

	ret = sg_alloc_table(buffer->sgt, npages, GFP_KERNEL);
	if (ret < 0) {
		DRM_ERROR("failed to initialize sg table.\n");
		goto err_free_sgt;
	}

	buffer->pages = kzalloc(sizeof(struct page) * npages, GFP_KERNEL);
	if (!buffer->pages) {
		DRM_ERROR("failed to allocate pages.\n");
		ret = -ENOMEM;
		goto err_sg_free_table;
	}

	sgl = buffer->sgt->sgl;
	start_addr = buffer->paddr;

	while (i < npages) {
		buffer->pages[i] = phys_to_page(start_addr);
		sg_set_page(sgl, buffer->pages[i], buffer->page_size, 0);
		sg_dma_address(sgl) = start_addr;
		start_addr += buffer->page_size;
		sgl = sg_next(sgl);
		i++;
	}

	if (private->vmm) {
		exynos_gem_obj->vmm = private->vmm;

		buffer->dev_addr = exynos_drm_iommu_map_gem(drm_dev,
							&exynos_gem_obj->base);
		if (!buffer->dev_addr) {
			DRM_ERROR("failed to map gem with iommu table.\n");
			ret = -EFAULT;

			exynos_drm_free_buf(drm_dev, flags, buffer);

			drm_gem_object_release(&exynos_gem_obj->base);

			goto err_free_pages;
		}

		buffer->dma_addr = buffer->dev_addr;
	 } else
		buffer->dma_addr = buffer->paddr;

	DRM_DEBUG_KMS("dma_addr = 0x%x\n", buffer->dma_addr);

	return 0;

err_free_pages:
	kfree(buffer->pages);
	buffer->pages = NULL;
err_sg_free_table:
	sg_free_table(buffer->sgt);
err_free_sgt:
	kfree(buffer->sgt);
	buffer->sgt = NULL;
err_release_handle:
	drm_gem_handle_delete(file_priv, args->gem_handle);
err_fini_buf:
	exynos_drm_fini_buf(drm_dev, buffer);
err_release_gem_obj:
	drm_gem_object_release(&exynos_gem_obj->base);
	kfree(exynos_gem_obj);
	return ret;
}

int exynos_drm_gem_init_object(struct drm_gem_object *obj)
{
	DRM_DEBUG_KMS("%s\n", __FILE__);

	return 0;
}

void exynos_drm_gem_free_object(struct drm_gem_object *obj)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct exynos_drm_gem_buf *buf;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	exynos_gem_obj = to_exynos_gem_obj(obj);
	buf = exynos_gem_obj->buffer;

	if (obj->import_attach)
		drm_prime_gem_destroy(obj, buf->sgt);

	exynos_drm_gem_destroy(to_exynos_gem_obj(obj));
}

int exynos_drm_gem_dumb_create(struct drm_file *file_priv,
			       struct drm_device *dev,
			       struct drm_mode_create_dumb *args)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	int ret;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	/*
	 * alocate memory to be used for framebuffer.
	 * - this callback would be called by user application
	 *	with DRM_IOCTL_MODE_CREATE_DUMB command.
	 */

	args->pitch = args->width * args->bpp >> 3;
	args->size = PAGE_ALIGN(args->pitch * args->height);

	exynos_gem_obj = exynos_drm_gem_create(dev, args->flags, args->size);
	if (IS_ERR(exynos_gem_obj))
		return PTR_ERR(exynos_gem_obj);

	ret = exynos_drm_gem_handle_create(&exynos_gem_obj->base, file_priv,
			&args->handle);
	if (ret) {
		exynos_drm_gem_destroy(exynos_gem_obj);
		return ret;
	}

	return 0;
}

int exynos_drm_gem_dumb_map_offset(struct drm_file *file_priv,
				   struct drm_device *dev, uint32_t handle,
				   uint64_t *offset)
{
	struct drm_gem_object *obj;
	int ret = 0;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	mutex_lock(&dev->struct_mutex);

	/*
	 * get offset of memory allocated for drm framebuffer.
	 * - this callback would be called by user application
	 *	with DRM_IOCTL_MODE_MAP_DUMB command.
	 */

	obj = drm_gem_object_lookup(dev, file_priv, handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		ret = -EINVAL;
		goto unlock;
	}

	if (!obj->map_list.map) {
		ret = drm_gem_create_mmap_offset(obj);
		if (ret)
			goto out;
	}

	*offset = (u64)obj->map_list.hash.key << PAGE_SHIFT;
	DRM_DEBUG_KMS("offset = 0x%lx\n", (unsigned long)*offset);

out:
	drm_gem_object_unreference(obj);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

int exynos_drm_gem_dumb_destroy(struct drm_file *file_priv,
				struct drm_device *dev,
				unsigned int handle)
{
	int ret;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	/*
	 * obj->refcount and obj->handle_count are decreased and
	 * if both them are 0 then exynos_drm_gem_free_object()
	 * would be called by callback to release resources.
	 */
	ret = drm_gem_handle_delete(file_priv, handle);
	if (ret < 0) {
		DRM_ERROR("failed to delete drm_gem_handle.\n");
		return ret;
	}

	return 0;
}

void exynos_drm_gem_close_object(struct drm_gem_object *obj,
				struct drm_file *file)
{
	DRM_DEBUG_KMS("%s\n", __FILE__);

	/* TODO */
}

int exynos_drm_gem_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct drm_gem_object *obj = vma->vm_private_data;
	struct drm_device *dev = obj->dev;
	unsigned long f_vaddr;
	pgoff_t page_offset;
	int ret;

	page_offset = ((unsigned long)vmf->virtual_address -
			vma->vm_start) >> PAGE_SHIFT;
	f_vaddr = (unsigned long)vmf->virtual_address;

	mutex_lock(&dev->struct_mutex);

	ret = exynos_drm_gem_map_pages(obj, vma, f_vaddr, page_offset);
	if (ret < 0)
		DRM_ERROR("failed to map pages.\n");

	mutex_unlock(&dev->struct_mutex);

	return convert_to_vm_err_msg(ret);
}

int exynos_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct exynos_drm_gem_obj *exynos_gem_obj;
	struct drm_gem_object *obj;
	int ret;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	/* set vm_area_struct. */
	ret = drm_gem_mmap(filp, vma);
	if (ret < 0) {
		DRM_ERROR("failed to mmap.\n");
		return ret;
	}

	obj = vma->vm_private_data;
	exynos_gem_obj = to_exynos_gem_obj(obj);

	ret = check_gem_flags(exynos_gem_obj->flags);
	if (ret) {
		drm_gem_vm_close(vma);
		drm_gem_free_mmap_offset(obj);
		return ret;
	}

	vma->vm_flags &= ~VM_PFNMAP;
	vma->vm_flags |= VM_MIXEDMAP;

	update_vm_cache_attr(exynos_gem_obj, vma);

	return ret;
}
