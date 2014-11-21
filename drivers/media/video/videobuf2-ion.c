/* linux/drivers/media/video/videobuf2-ion.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Implementation of Android ION memory allocator for videobuf2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/highmem.h>

#include <media/videobuf2-core.h>
#include <media/videobuf2-memops.h>
#include <media/videobuf2-ion.h>

#include <asm/cacheflush.h>

#include <plat/iovmm.h>
#include <plat/cpu.h>

extern struct ion_device *ion_exynos; /* drivers/gpu/ion/exynos/exynos-ion.c */

struct vb2_ion_context {
	struct device		*dev;
	struct ion_client	*client;
	unsigned long		alignment;
	long			flags;
};

struct vb2_ion_buf {
	struct vm_area_struct		**vmas;
	int				vma_count;
	struct vb2_ion_context		*ctx;
	struct vb2_vmarea_handler	handler;
	struct ion_handle		*handle;
	void				*kva;
	unsigned long			size;
	atomic_t			ref;
	bool				cached;
	struct vb2_ion_cookie		cookie;
};

#define ctx_cached(ctx) (!(ctx->flags & VB2ION_CTX_UNCACHED))
#define ctx_iommu(ctx) (!!(ctx->flags & VB2ION_CTX_IOMMU))

void vb2_ion_set_cached(void *ctx, bool cached)
{
	struct vb2_ion_context *vb2ctx = ctx;

	if (cached)
		vb2ctx->flags &= ~VB2ION_CTX_UNCACHED;
	else
		vb2ctx->flags |= VB2ION_CTX_UNCACHED;
}
EXPORT_SYMBOL(vb2_ion_set_cached);

int vb2_ion_set_alignment(void *ctx, size_t alignment)
{
	struct vb2_ion_context *vb2ctx = ctx;

	if ((alignment != 0) && (alignment < PAGE_SIZE))
		return -EINVAL;

	if (alignment & ~alignment)
		return -EINVAL;

	if (alignment == 0)
		vb2ctx->alignment = PAGE_SIZE;
	else
		vb2ctx->alignment = alignment;

	return 0;
}
EXPORT_SYMBOL(vb2_ion_set_alignment);

void *vb2_ion_create_context(struct device *dev, size_t alignment, long flags)
{
	struct vb2_ion_context *ctx;
	int ret;
	unsigned int heapmask = ION_HEAP_EXYNOS_USER_MASK;

	/* ion_client_create() expects the current thread to be a kernel thread
	 * to create a new ion_client
	 */
	WARN_ON(!(current->group_leader->flags & PF_KTHREAD));

	if (flags & VB2ION_CTX_PHCONTIG)
		heapmask |= ION_HEAP_EXYNOS_CONTIG_MASK;
	if (flags & VB2ION_CTX_VMCONTIG)
		heapmask |= ION_HEAP_EXYNOS_MASK;

	 /* non-contigous memory without H/W virtualization is not supported */
	if ((flags & VB2ION_CTX_VMCONTIG) && !(flags & VB2ION_CTX_IOMMU))
		return ERR_PTR(-EINVAL);

	ctx = kmalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	ctx->dev = dev;
	ctx->client = ion_client_create(ion_exynos, heapmask, dev_name(dev));
	if (IS_ERR(ctx->client)) {
		ret = PTR_ERR(ctx->client);
		goto err_ion;
	}

	if (flags & VB2ION_CTX_IOMMU) {
		ret = iovmm_setup(dev);
		if (ret)
			goto err_iovmm;
	}

	vb2_ion_set_alignment(ctx, alignment);
	ctx->flags = flags;

	return ctx;

err_iovmm:
	ion_client_destroy(ctx->client);
err_ion:
	kfree(ctx);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL(vb2_ion_create_context);

void vb2_ion_destroy_context(void *ctx)
{
	struct vb2_ion_context *vb2ctx = ctx;

	ion_client_destroy(vb2ctx->client);
	iovmm_cleanup(vb2ctx->dev);
	kfree(vb2ctx);
}
EXPORT_SYMBOL(vb2_ion_destroy_context);

void *vb2_ion_private_alloc(void *alloc_ctx, size_t size)
{
	struct vb2_ion_context *ctx = alloc_ctx;
	struct vb2_ion_buf *buf;
	struct scatterlist *sg;
	int ret = 0;

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	size = PAGE_ALIGN(size);

	buf->handle = ion_alloc(ctx->client, size, ctx->alignment,
				ion_heapflag(ctx->flags));
	if (IS_ERR(buf->handle)) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	buf->cookie.sg = ion_map_dma(ctx->client, buf->handle);
	if (IS_ERR(buf->cookie.sg)) {
		ret = -ENOMEM;
		goto err_map_dma;
	}

	buf->ctx = ctx;
	buf->size = size;
	buf->cached = ctx_cached(ctx);

	sg = buf->cookie.sg;
	do {
		buf->cookie.nents++;
	} while ((sg = sg_next(sg)));

	buf->kva = ion_map_kernel(ctx->client, buf->handle);
	if (IS_ERR(buf->kva)) {
		ret = PTR_ERR(buf->kva);
		buf->kva = NULL;
		goto err_map_kernel;
	}

	if (ctx_iommu(ctx)) {
		buf->cookie.ioaddr = iovmm_map(ctx->dev,
						buf->cookie.sg, 0, size);
		if (!buf->cookie.ioaddr) {
			ret = -EFAULT;
			goto err_ion_map_io;
		}
	}

	return &buf->cookie;

err_ion_map_io:
	ion_unmap_kernel(ctx->client, buf->handle);
err_map_kernel:
	ion_unmap_dma(ctx->client, buf->handle);
err_map_dma:
	ion_free(ctx->client, buf->handle);
err_alloc:
	kfree(buf);

	return ERR_PTR(ret);
}

void vb2_ion_private_free(void *cookie)
{
	struct vb2_ion_buf *buf =
			container_of(cookie, struct vb2_ion_buf, cookie);
	struct vb2_ion_context *ctx;

	if (WARN_ON(IS_ERR_OR_NULL(cookie)))
		return;

	ctx = buf->ctx;
	if (ctx_iommu(ctx))
		iovmm_unmap(ctx->dev, buf->cookie.ioaddr);

	ion_unmap_kernel(ctx->client, buf->handle);
	ion_unmap_dma(ctx->client, buf->handle);
	ion_free(ctx->client, buf->handle);

	kfree(buf);
}

static void vb2_ion_put(void *buf_priv)
{
	struct vb2_ion_buf *buf = buf_priv;

	if (atomic_dec_and_test(&buf->ref))
		vb2_ion_private_free(&buf->cookie);
}

static void *vb2_ion_alloc(void *alloc_ctx, unsigned long size)
{
	struct vb2_ion_buf *buf;
	void *cookie;

	cookie = vb2_ion_private_alloc(alloc_ctx, size);
	if (IS_ERR(cookie))
		return cookie;

	buf = container_of(cookie, struct vb2_ion_buf, cookie);

	buf->handler.refcount = &buf->ref;
	buf->handler.put = vb2_ion_put;
	buf->handler.arg = buf;
	atomic_set(&buf->ref, 1);

	return buf;
}

void *vb2_ion_private_vaddr(void *cookie)
{
	if (WARN_ON(IS_ERR_OR_NULL(cookie)))
		return NULL;

	return container_of(cookie, struct vb2_ion_buf, cookie)->kva;
}

/**
 * _vb2_ion_get_vma() - lock userspace mapped memory
 * @vaddr:	starting virtual address of the area to be verified
 * @size:	size of the area
 * @vma_num:	number of returned vma copies
 *
 * This function will go through memory area of size @size mapped at @vaddr
 * If they are contiguous, the virtual memory area is locked, this function
 * returns the array of vma copies of the given area and vma_num becomes
 * the number of vmas returned.
 *
 * Returns 0 on success.
 */
static struct vm_area_struct **_vb2_ion_get_vma(unsigned long vaddr,
					unsigned long size, int *vma_num)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma, *vma0;
	struct vm_area_struct **vmas;
	unsigned long prev_end = 0;
	unsigned long end;
	int i;

	end = vaddr + size;

	down_read(&mm->mmap_sem);
	vma0 = find_vma(mm, vaddr);
	if (!vma0) {
		vmas = ERR_PTR(-EINVAL);
		goto done;
	}

	for (*vma_num = 1, vma = vma0->vm_next, prev_end = vma0->vm_end;
		vma && (end > vma->vm_start) && (prev_end == vma->vm_start);
				prev_end = vma->vm_end, vma = vma->vm_next) {
		*vma_num += 1;
	}

	if (prev_end < end) {
		vmas = ERR_PTR(-EINVAL);
		goto done;
	}

	vmas = kmalloc(sizeof(*vmas) * *vma_num, GFP_KERNEL);
	if (!vmas) {
		vmas = ERR_PTR(-ENOMEM);
		goto done;
	}

	for (i = 0; i < *vma_num; i++, vma0 = vma0->vm_next) {
		vmas[i] = vb2_get_vma(vma0);
		if (!vmas[i])
			break;
	}

	if (i < *vma_num) {
		while (i-- > 0)
			vb2_put_vma(vmas[i]);

		kfree(vmas);
		vmas = ERR_PTR(-ENOMEM);
	}

done:
	up_read(&mm->mmap_sem);
	return vmas;
}

static void *vb2_ion_get_userptr(void *alloc_ctx, unsigned long vaddr,
				 unsigned long size, int write)
{
	struct vb2_ion_context *ctx = alloc_ctx;
	struct vb2_ion_buf *buf = NULL;
	int ret = 0;
	struct scatterlist *sg;
	off_t offset;

	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->handle = ion_import_uva(ctx->client, vaddr, &offset);
	if (IS_ERR(buf->handle)) {
		if (PTR_ERR(buf->handle) == -ENXIO) {
			int flags = ION_HEAP_EXYNOS_USER_MASK;

			if (write)
				flags |= ION_EXYNOS_WRITE_MASK;

			buf->handle = ion_exynos_get_user_pages(ctx->client,
							vaddr, size, flags);
			if (IS_ERR(buf->handle))
				ret = PTR_ERR(buf->handle);
		} else {
			ret = -EINVAL;
		}

		if (ret) {
			pr_err("%s: Failed to retrieving non-ion user buffer @ "
				"0x%lx (size:0x%lx, dev:%s, errno %ld)\n",
				__func__, vaddr, size, dev_name(ctx->dev),
					PTR_ERR(buf->handle));
			goto err_import_uva;
		}

		offset = 0;
	}

	buf->cookie.sg = ion_map_dma(ctx->client, buf->handle);
	if (IS_ERR(buf->cookie.sg)) {
		ret = -ENOMEM;
		goto err_map_dma;
	}

	sg = buf->cookie.sg;
	do {
		buf->cookie.nents++;
	} while ((sg = sg_next(sg)));

	if (ctx_iommu(ctx)) {
		buf->cookie.ioaddr = iovmm_map(ctx->dev, buf->cookie.sg,
						offset, size);
		if (!buf->cookie.ioaddr) {
			ret = -EFAULT;
			goto err_ion_map_io;
		}
	}

	buf->vmas = _vb2_ion_get_vma(vaddr, size, &buf->vma_count);
	if (IS_ERR(buf->vmas)) {
		ret = PTR_ERR(buf->vmas);
		goto err_get_vma;
	}

	buf->kva = ion_map_kernel(ctx->client, buf->handle);
	if (IS_ERR(buf->kva)) {
		ret = PTR_ERR(buf->kva);
		buf->kva = NULL;
		goto err_map_kernel;
	}

	if ((pgprot_noncached(buf->vmas[0]->vm_page_prot)
				== buf->vmas[0]->vm_page_prot)
			|| (pgprot_writecombine(buf->vmas[0]->vm_page_prot)
				== buf->vmas[0]->vm_page_prot))
		buf->cached = false;
	else
		buf->cached = true;

	buf->cookie.offset = offset;
	buf->ctx = ctx;
	buf->size = size;

	return buf;

err_map_kernel:
	while (buf->vma_count-- > 0)
		vb2_put_vma(buf->vmas[buf->vma_count]);
	kfree(buf->vmas);
err_get_vma:
	if (ctx_iommu(ctx))
		iovmm_unmap(ctx->dev, buf->cookie.ioaddr);
err_ion_map_io:
	ion_unmap_dma(ctx->client, buf->handle);
err_map_dma:
	ion_free(ctx->client, buf->handle);
err_import_uva:
	kfree(buf);

	return ERR_PTR(ret);
}

static void vb2_ion_put_userptr(void *mem_priv)
{
	struct vb2_ion_buf *buf = mem_priv;
	struct vb2_ion_context *ctx = buf->ctx;
	int i;

	if (ctx_iommu(ctx))
		iovmm_unmap(ctx->dev, buf->cookie.ioaddr);

	ion_unmap_dma(ctx->client, buf->handle);
	if (buf->kva)
		ion_unmap_kernel(ctx->client, buf->handle);

	ion_free(ctx->client, buf->handle);

	for (i = 0; i < buf->vma_count; i++)
		vb2_put_vma(buf->vmas[i]);

	kfree(buf->vmas);
	kfree(buf);
}

static void *vb2_ion_cookie(void *buf_priv)
{
	struct vb2_ion_buf *buf = buf_priv;

	if (WARN_ON(!buf))
		return NULL;

	return (void *)&buf->cookie;
}

static void *vb2_ion_vaddr(void *buf_priv)
{
	struct vb2_ion_buf *buf = buf_priv;

	if (WARN_ON(!buf))
		return NULL;

	return buf->kva;
}

static unsigned int vb2_ion_num_users(void *buf_priv)
{
	struct vb2_ion_buf *buf = buf_priv;

	if (WARN_ON(!buf))
		return 0;

	return atomic_read(&buf->ref);
}

static int vb2_ion_mmap(void *buf_priv, struct vm_area_struct *vma)
{
	struct vb2_ion_buf *buf = buf_priv;
	unsigned long vm_start = vma->vm_start;
	unsigned long vm_end = vma->vm_end;
	struct scatterlist *sg = buf->cookie.sg;
	unsigned long size;
	int ret = -EINVAL;

	if (buf->size  < (vm_end - vm_start))
		return ret;

	if (!buf->cached)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	size = min_t(size_t, vm_end - vm_start, sg_dma_len(sg));

	ret = remap_pfn_range(vma, vm_start, page_to_pfn(sg_page(sg)),
				size, vma->vm_page_prot);

	for (sg = sg_next(sg), vm_start += size;
			!ret && sg && (vm_start < vm_end);
			vm_start += size, sg = sg_next(sg)) {
		size = min_t(size_t, vm_end - vm_start, sg_dma_len(sg));
		ret = remap_pfn_range(vma, vm_start, page_to_pfn(sg_page(sg)),
						size, vma->vm_page_prot);
	}

	if (ret)
		return ret;

	if (vm_start < vm_end)
		return -EINVAL;

	vma->vm_flags		|= VM_DONTEXPAND;
	vma->vm_private_data	= &buf->handler;
	vma->vm_ops		= &vb2_common_vm_ops;

	vma->vm_ops->open(vma);

	return ret;
}

const struct vb2_mem_ops vb2_ion_memops = {
	.alloc		= vb2_ion_alloc,
	.put		= vb2_ion_put,
	.cookie		= vb2_ion_cookie,
	.vaddr		= vb2_ion_vaddr,
	.mmap		= vb2_ion_mmap,
	.get_userptr	= vb2_ion_get_userptr,
	.put_userptr	= vb2_ion_put_userptr,
	.num_users	= vb2_ion_num_users,
};
EXPORT_SYMBOL_GPL(vb2_ion_memops);

void vb2_ion_sync_for_device(void *cookie, off_t offset, size_t size,
						enum dma_data_direction dir)
{
	struct vb2_ion_cookie *vb2cookie = cookie;
	struct vb2_ion_context *ctx =
			container_of(cookie, struct vb2_ion_buf, cookie)->ctx;
	struct scatterlist *sg;

	for (sg = vb2cookie->sg; sg != NULL; sg = sg_next(sg)) {
		if (sg_dma_len(sg) <= offset)
			offset -= sg_dma_len(sg);
		else
			break;
	}

	if (WARN_ON(sg == NULL)) /* Too big offset */
		return;

	while ((size != 0) && (sg != NULL)) {
		size_t sg_size;

		sg_size = min_t(size_t, size, sg_dma_len(sg) - offset);
		dma_map_page(ctx->dev, sg_page(sg) + PFN_DOWN(offset),
				offset & ~PAGE_MASK, sg_size, dir);

		offset = 0;
		size -= sg_size;
		sg = sg_next(sg);
	}

	WARN_ON(size != 0); /* Too big size */
}
EXPORT_SYMBOL_GPL(vb2_ion_sync_for_device);

void vb2_ion_sync_for_cpu(void *cookie, off_t offset, size_t size,
						enum dma_data_direction dir)
{
	struct vb2_ion_cookie *vb2cookie = cookie;
	struct vb2_ion_context *ctx =
			container_of(cookie, struct vb2_ion_buf, cookie)->ctx;
	struct scatterlist *sg;

	for (sg = vb2cookie->sg; sg != NULL; sg = sg_next(sg)) {
		if (sg_dma_len(sg) <= offset)
			offset -= sg_dma_len(sg);
		else
			break;
	}

	if (WARN_ON(sg == NULL)) /* Too big offset */
		return;

	while ((size != 0) && (sg != NULL)) {
		size_t sg_size;

		sg_size = min_t(size_t, size, sg_dma_len(sg) - offset);
		dma_unmap_page(ctx->dev, sg_phys(sg) + offset, sg_size, dir);

		offset = 0;
		size -= sg_size;
		sg = sg_next(sg);
	}

	WARN_ON(size != 0); /* Too big size */
}
EXPORT_SYMBOL_GPL(vb2_ion_sync_for_cpu);

static void flush_entire_dcache(void *p)
{
	flush_cache_all();
}

int vb2_ion_cache_flush(struct vb2_buffer *vb, u32 num_planes)
{
	int i;
	size_t sz = 0;
	struct vb2_ion_buf *buf;
	enum dma_data_direction dir;

	for (i = 0; i < num_planes; i++) {
		buf = vb->planes[i].mem_priv;
		if (!buf->cached)
			continue;

		sz += buf->size;
	}

	if (sz >= SZ_2M) { /* performance tuning */
		smp_call_function(&flush_entire_dcache, NULL, 1);
		outer_flush_all();
		return 0;
	}

	dir = V4L2_TYPE_IS_OUTPUT(vb->v4l2_buf.type) ?
					DMA_TO_DEVICE : DMA_FROM_DEVICE;

	while (num_planes-- > 0) {
		struct scatterlist *sg;
		off_t start_off;
		int num_pages;
		struct page *page;

		buf = vb->planes[num_planes].mem_priv;
		if (!buf->cached)
			continue;

		sg = buf->cookie.sg;
		start_off = buf->cookie.offset;
		sz = buf->size;

		while (sg && (start_off >= sg_dma_len(sg))) {
			start_off -= sg_dma_len(sg);
			sg = sg_next(sg);
		}

		for (; (sg != NULL) && (sz > 0); sg = sg_next(sg)) {
			int pages_to_pass;
			void *addr;
			bool kmapped = false;

			start_off += sg->offset;

			num_pages = PFN_DOWN(
				PAGE_ALIGN(sg_dma_len(sg) + sg->offset));
			pages_to_pass = PFN_DOWN(
					round_down(start_off, PAGE_SIZE));
			num_pages -= pages_to_pass;

			page = sg_page(sg) + pages_to_pass;
			start_off &= ~PAGE_MASK;

			for (i = 0; (i < num_pages) && (sz > 0); i++, page++) {
				size_t szflush;

				szflush = min_t(size_t,
						PAGE_SIZE - start_off, sz);

				addr = page_address(page);
				if (!addr) {
					addr = kmap_atomic(page);
					kmapped = true;
				}

				dmac_map_area(addr + start_off, szflush, dir);

				if (dir == DMA_TO_DEVICE)
					outer_clean_range(
						page_to_phys(page) + start_off,
						page_to_phys(page) + szflush);
				else
					outer_inv_range(
						page_to_phys(page) + start_off,
						page_to_phys(page) + szflush);

				if (kmapped) {
					kunmap_atomic(addr);
					kmapped = false;
				}

				sz -= szflush;
				start_off = 0;
			}
		}

		WARN_ON(sz > 0);
	}

	return 0;
}

int vb2_ion_cache_inv(struct vb2_buffer *vb, u32 num_planes)
{
	struct vb2_ion_buf *buf;
	int i;

	for (i = 0; i < num_planes; i++) {
		buf = vb->planes[i].mem_priv;
		if (!buf->cached)
			continue;

		vb2_ion_sync_for_device(&buf->cookie, buf->cookie.offset,
						buf->size, DMA_FROM_DEVICE);
	}

	return 0;
}

void vb2_ion_detach_iommu(void *alloc_ctx)
{
	struct vb2_ion_context *ctx = alloc_ctx;

	if (!ctx_iommu(ctx))
		return;

	iovmm_deactivate(ctx->dev);
}
EXPORT_SYMBOL_GPL(vb2_ion_detach_iommu);

int vb2_ion_attach_iommu(void *alloc_ctx)
{
	struct vb2_ion_context *ctx = alloc_ctx;

	if (!ctx_iommu(ctx))
		return -ENOENT;

	return iovmm_activate(ctx->dev);
}
EXPORT_SYMBOL_GPL(vb2_ion_attach_iommu);

MODULE_AUTHOR("Jonghun,	Han <jonghun.han@samsung.com>");
MODULE_DESCRIPTION("Android ION allocator handling routines for videobuf2");
MODULE_LICENSE("GPL");
