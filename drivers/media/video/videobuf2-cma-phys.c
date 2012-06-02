/* linux/drivers/media/video/videobuf2-cma-phys.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * CMA-phys memory allocator for videobuf2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/cma.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/scatterlist.h>
#include <linux/dma-buf.h>

#include <media/videobuf2-core.h>
#include <media/videobuf2-memops.h>

#include <asm/cacheflush.h>

#define SIZE_THRESHOLD SZ_1M

struct vb2_cma_phys_conf {
	struct device		*dev;
	const char		*type;
	unsigned long		alignment;
	bool			cacheable;
};

struct vb2_cma_phys_buf {
	struct vb2_cma_phys_conf		*conf;
	dma_addr_t			dma_addr;
	unsigned long			size;
	struct vm_area_struct		*vma;
	/* fd exported from this buf object. */
	int				export_fd;
	/* dma buf exported from this buf object. */
	struct dma_buf			*export_dma_buf;
	struct dma_buf_attachment	*db_attach;
	atomic_t			refcount;
	struct vb2_vmarea_handler	handler;
	bool				cacheable;
};

struct vb2_cma_phys_db_attach {
	struct vb2_dc_buf		*buf;
	struct dma_buf_attachment	db_attach;
};

static void vb2_cma_phys_put(void *buf_priv);

#ifdef CONFIG_DMA_SHARED_BUFFER
static int cma_phys_attach(struct dma_buf *dmabuf, struct device *dev,
				struct dma_buf_attachment *attach)
{
	/* TODO */

	return 0;
}

static void cma_phys_detach(struct dma_buf *dmabuf,
				struct dma_buf_attachment *attach)
{
	/* TODO */

	/*
	 * when vb2_cma_phys_export_dmabuf() is called, file->f_count of this
	 * dmabuf will be increased by dma_buf_get() so drop the reference here.
	 */
	dma_buf_put(dmabuf);
}

static struct sg_table *
	cma_phys_map_dmabuf(struct dma_buf_attachment *attach,
				enum dma_data_direction direction)
{
	struct vb2_cma_phys_buf *buf = attach->dmabuf->priv;
	struct sg_table *sgt;
	int ret;

	sgt = kzalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt) {
		printk(KERN_ERR "failed to allocate sg table.\n");
		return ERR_PTR(-ENOMEM);
	}

	ret = sg_alloc_table(sgt, 1, GFP_KERNEL);
	if (ret < 0) {
		printk(KERN_ERR "failed to allocate scatter list.\n");
		kfree(sgt);
		sgt = NULL;
		return ERR_PTR(-ENOMEM);
	}

	sg_init_table(sgt->sgl, 1);
	sg_dma_len(sgt->sgl) = buf->size;
	sg_set_page(sgt->sgl, pfn_to_page(PFN_DOWN(buf->dma_addr)),
			buf->size, 0);
	sg_dma_address(sgt->sgl) = buf->dma_addr;

	/*
	 * increase reference count of this buf object.
	 *
	 * Note:
	 * alloated physical memory region is being shared with others so
	 * this region shouldn't be released until all references of this
	 * region will be dropped by vb2_cma_phys_unmap_dmabuf().
	 */
	atomic_inc(&buf->refcount);

	return sgt;
}

static void cma_phys_unmap_dmabuf(struct dma_buf_attachment *attach,
					struct sg_table *sgt,
					enum dma_data_direction dir)
{
	struct vb2_cma_phys_buf *buf = attach->dmabuf->priv;

	sg_free_table(sgt);
	kfree(sgt);
	sgt = NULL;

	if (atomic_read(&buf->refcount) <= 9)
		BUG();

	atomic_dec(&buf->refcount);
}

static void cma_phys_release(struct dma_buf *dmabuf)
{
	struct vb2_cma_phys_buf *buf = dmabuf->priv;

	/*
	 * vb2_cma_phys_release() call means that file object's f_count is
	 * 0 and it calls vb2_cma_phys_put() to drop the reference that it
	 * had been increased at vb2_cma_phys_export_dmabuf().
	 */
	if (buf->export_dma_buf == dmabuf) {
		buf->export_fd = -1;
		buf->export_dma_buf = NULL;

		/*
		 * drop this buf object reference to release allocated buffer
		 * and resource.
		 */
		vb2_cma_phys_put(buf);
	}
}

static struct dma_buf_ops cma_phys_dmabuf_ops = {
	.attach		= cma_phys_attach,
	.detach		= cma_phys_detach,
	.map_dma_buf	= cma_phys_map_dmabuf,
	.unmap_dma_buf	= cma_phys_unmap_dmabuf,
	.release	= cma_phys_release,
};
#else
#define cma_phys_attach		NULL
#define cma_phys_detach		NULL
#define cma_phys_map_dmabuf	NULL
#define cma_phys_unmap_dmabuf	NULL
#define cma_phys_release	NULL
#endif

static void *vb2_cma_phys_alloc(void *alloc_ctx, unsigned long size)
{
	struct vb2_cma_phys_conf *conf = alloc_ctx;
	struct vb2_cma_phys_buf *buf;

	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->dma_addr = cma_alloc(conf->dev, conf->type, size, conf->alignment);
	if (IS_ERR((void *)buf->dma_addr)) {
		printk(KERN_ERR "cma_alloc of size %ld failed\n", size);
		kfree(buf);
		return ERR_PTR(-ENOMEM);
	}

	buf->conf = conf;
	buf->size = size;
	buf->cacheable = conf->cacheable;

	buf->handler.refcount = &buf->refcount;
	buf->handler.put = vb2_cma_phys_put;
	buf->handler.arg = buf;

	atomic_inc(&buf->refcount);

	return buf;
}

static void vb2_cma_phys_put(void *buf_priv)
{
	struct vb2_cma_phys_buf *buf = buf_priv;

	if (atomic_dec_and_test(&buf->refcount)) {
		cma_free(buf->dma_addr);
		kfree(buf);
	}
}

static void *vb2_cma_phys_cookie(void *buf_priv)
{
	struct vb2_cma_phys_buf *buf = buf_priv;

	return (void *)buf->dma_addr;
}

static unsigned int vb2_cma_phys_num_users(void *buf_priv)
{
	struct vb2_cma_phys_buf *buf = buf_priv;

	return atomic_read(&buf->refcount);
}

/**
 * vb2_cma_mmap_pfn_range() - map physical pages to userspace
 * @vma:	virtual memory region for the mapping
 * @dma_addr:	starting dma address of the memory to be mapped
 * @size:	size of the memory to be mapped
 * @vm_ops:	vm operations to be assigned to the created area
 * @priv:	private data to be associated with the area
 *
 * Returns 0 on success.
 */
int vb2_cma_phys_mmap_pfn_range(struct vm_area_struct *vma,
				unsigned long dma_addr,
				unsigned long size,
				const struct vm_operations_struct *vm_ops,
				void *priv)
{
	int ret;

	size = min_t(unsigned long, vma->vm_end - vma->vm_start, size);

	ret = remap_pfn_range(vma, vma->vm_start, dma_addr >> PAGE_SHIFT,
				size, vma->vm_page_prot);
	if (ret) {
		printk(KERN_ERR "Remapping memory failed, error: %d\n", ret);
		return ret;
	}

	vma->vm_flags		|= VM_DONTEXPAND | VM_RESERVED;
	vma->vm_private_data	= priv;
	vma->vm_ops		= vm_ops;

	vma->vm_ops->open(vma);

	printk(KERN_DEBUG "%s: mapped dma addr 0x%08lx at 0x%08lx, size %ld\n",
			__func__, dma_addr, vma->vm_start, size);

	return 0;
}

static int vb2_cma_phys_mmap(void *buf_priv, struct vm_area_struct *vma)
{
	struct vb2_cma_phys_buf *buf = buf_priv;

	if (!buf) {
		printk(KERN_ERR "No buffer to map\n");
		return -EINVAL;
	}

	if (!buf->cacheable)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	return vb2_cma_phys_mmap_pfn_range(vma, buf->dma_addr, buf->size,
					   &vb2_common_vm_ops, &buf->handler);
}

static void *vb2_cma_phys_get_userptr(void *alloc_ctx, unsigned long vaddr,
				 unsigned long size, int write)
{
	struct vb2_cma_phys_buf *buf;

	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	printk(KERN_DEBUG "[%s] dma_addr(0x%08lx)\n", __func__, vaddr);
	buf->size = size;
	buf->dma_addr = vaddr;	/* drv directly gets dma addr. from user. */

	return buf;
}

static void vb2_cma_phys_put_userptr(void *mem_priv)
{
	struct vb2_cma_phys_buf *buf = mem_priv;

	if (!buf)
		return;

	kfree(buf);
}

static void *vb2_cma_phys_vaddr(void *mem_priv)
{
	struct vb2_cma_phys_buf *buf = mem_priv;
	if (!buf)
		return 0;

	return phys_to_virt(buf->dma_addr);
}

static void vb2_cma_phys_map_dmabuf(void *mem_priv)
{
	struct vb2_cma_phys_buf *buf = mem_priv;
	struct dma_buf *dmabuf;
	struct sg_table *sg;
	enum dma_data_direction dir;

	if (!buf || !buf->db_attach)
		return;

	WARN_ON(buf->dma_addr);

	dmabuf = buf->db_attach->dmabuf;

	/* TODO need a way to know if we are camera or display, etc.. */
	dir = DMA_BIDIRECTIONAL;

	/* get the associated sg for this buffer */
	sg = dma_buf_map_attachment(buf->db_attach, dir);
	if (!sg)
		return;

	/*
	 *  convert sglist to dma_addr:
	 *  Assumption: for dma-contig, dmabuf would map to single entry
	 *  Will print a warning if it has more than one.
	 */
	if (sg->nents > 1)
		printk(KERN_WARNING
			"dmabuf scatterlist has more than 1 entry\n");

	buf->dma_addr = sg_dma_address(sg->sgl);
	buf->size = sg_dma_len(sg->sgl);

	/* save this sg in dmabuf for put_scatterlist */
	dmabuf->priv = sg;
}

static void vb2_cma_phys_unmap_dmabuf(void *mem_priv)
{
	struct vb2_cma_phys_buf *buf = mem_priv;
	struct dma_buf *dmabuf;
	struct sg_table *sg;

	if (!buf || !buf->db_attach)
		return;

	WARN_ON(!buf->dma_addr);

	dmabuf = buf->db_attach->dmabuf;
	sg = dmabuf->priv;

	/*
	 * Put the sg for this buffer:
	 */
	dma_buf_unmap_attachment(buf->db_attach, sg, DMA_FROM_DEVICE);

	buf->dma_addr = 0;
	buf->size = 0;
}

#ifdef CONFIG_DMA_SHARED_BUFFER
static int vb2_cma_phys_export_dmabuf(void *alloc_ctx, void *buf_priv,
					int *export_fd)
{
	struct vb2_cma_phys_buf *buf = buf_priv;
	unsigned int flags = O_RDWR;

	buf->export_dma_buf = dma_buf_export(buf, &cma_phys_dmabuf_ops,
						buf->size, 0600);
	if (!buf->export_dma_buf)
		return PTR_ERR(buf->export_dma_buf);

	/* FIXME!!! */
	flags |= O_CLOEXEC;

	buf->export_fd = dma_buf_fd(buf->export_dma_buf, flags);
	if (buf->export_fd < 0) {
		printk(KERN_ERR "fail to get fd from dmabuf.\n");
		dma_buf_put(buf->export_dma_buf);
		return buf->export_fd;
	}

	/*
	 * this buf object is referenced by buf->export_fd so
	 * the object refcount should be increased. thereafter,
	 * when vb2_cma_phys_put() is called, it will be decreased again.
	 */
	atomic_inc(&buf->refcount);

	*export_fd = buf->export_fd;

	return 0;
}
#else
#define vb2_cma_phys_export_dmabuf	NULL
#endif

static void *vb2_cma_phys_attach_dmabuf(void *alloc_ctx, struct dma_buf *dbuf)
{
	struct vb2_cma_phys_conf *conf = alloc_ctx;
	struct vb2_cma_phys_buf *buf;
	struct dma_buf_attachment *dba;

	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	/* create attachment for the dmabuf with the user device */
	dba = dma_buf_attach(dbuf, conf->dev);
	if (IS_ERR(dba)) {
		printk(KERN_ERR "failed to attach dmabuf\n");
		kfree(buf);
		return dba;
	}

	buf->conf = conf;
	buf->size = dba->dmabuf->size;
	buf->db_attach = dba;
	buf->dma_addr = 0; /* dma_addr is available only after acquire */

	return buf;
}

static void vb2_cma_phys_detach_dmabuf(void *mem_priv)
{
	struct vb2_cma_phys_buf *buf = mem_priv;

	if (!buf)
		return;

	if (buf->dma_addr)
		vb2_cma_phys_unmap_dmabuf(buf);

	/* detach this attachment */
	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	buf->db_attach = NULL;

	kfree(buf);
}

const struct vb2_mem_ops vb2_cma_phys_memops = {
	.alloc		= vb2_cma_phys_alloc,
	.put		= vb2_cma_phys_put,
	.cookie		= vb2_cma_phys_cookie,
	.mmap		= vb2_cma_phys_mmap,
	.get_userptr	= vb2_cma_phys_get_userptr,
	.put_userptr	= vb2_cma_phys_put_userptr,
	.map_dmabuf	= vb2_cma_phys_map_dmabuf,
	.unmap_dmabuf	= vb2_cma_phys_unmap_dmabuf,
	.export_dmabuf	= vb2_cma_phys_export_dmabuf,
	.attach_dmabuf	= vb2_cma_phys_attach_dmabuf,
	.detach_dmabuf	= vb2_cma_phys_detach_dmabuf,
	.num_users	= vb2_cma_phys_num_users,
	.vaddr		= vb2_cma_phys_vaddr,
};
EXPORT_SYMBOL_GPL(vb2_cma_phys_memops);

void *vb2_cma_phys_init(struct device *dev, const char *type,
			unsigned long alignment, bool cacheable)
{
	struct vb2_cma_phys_conf *conf;

	conf = kzalloc(sizeof *conf, GFP_KERNEL);
	if (!conf)
		return ERR_PTR(-ENOMEM);

	conf->dev = dev;
	conf->type = type;
	conf->alignment = alignment;
	conf->cacheable = cacheable;

	return conf;
}
EXPORT_SYMBOL_GPL(vb2_cma_phys_init);

void vb2_cma_phys_cleanup(void *conf)
{
	if (conf)
		kfree(conf);
	else
		printk(KERN_ERR "fail to cleanup\n");
}
EXPORT_SYMBOL_GPL(vb2_cma_phys_cleanup);

void **vb2_cma_phys_init_multi(struct device *dev,
			  unsigned int num_planes,
			  const char *types[],
			  unsigned long alignments[],
			  bool cacheable)
{
	struct vb2_cma_phys_conf *cma_conf;
	void **alloc_ctxes;
	unsigned int i;

	alloc_ctxes = kzalloc((sizeof *alloc_ctxes + sizeof *cma_conf)
				* num_planes, GFP_KERNEL);
	if (!alloc_ctxes)
		return ERR_PTR(-ENOMEM);

	cma_conf = (void *)(alloc_ctxes + num_planes);

	for (i = 0; i < num_planes; ++i, ++cma_conf) {
		alloc_ctxes[i] = cma_conf;
		cma_conf->dev = dev;
		cma_conf->type = types[i];
		cma_conf->alignment = alignments[i];
		cma_conf->cacheable = cacheable;
	}

	return alloc_ctxes;
}
EXPORT_SYMBOL_GPL(vb2_cma_phys_init_multi);

void vb2_cma_phys_cleanup_multi(void **alloc_ctxes)
{
	if (alloc_ctxes)
		kfree(alloc_ctxes);
	else
		printk(KERN_ERR "fail to cleanup_multi\n");
}
EXPORT_SYMBOL_GPL(vb2_cma_phys_cleanup_multi);

void vb2_cma_phys_set_cacheable(void *alloc_ctx, bool cacheable)
{
	((struct vb2_cma_phys_conf *)alloc_ctx)->cacheable = cacheable;
}

bool vb2_cma_phys_get_cacheable(void *alloc_ctx)
{
	return ((struct vb2_cma_phys_conf *)alloc_ctx)->cacheable;
}

static void _vb2_cma_phys_cache_flush_all(void)
{
	flush_cache_all();	/* L1 */
	smp_call_function((smp_call_func_t)__cpuc_flush_kern_all, NULL, 1);
	outer_flush_all();	/* L2 */
}

static void _vb2_cma_phys_cache_flush_range(struct vb2_cma_phys_buf *buf,
					    unsigned long size)
{
	phys_addr_t start = buf->dma_addr;
	phys_addr_t end = start + size - 1;

	if (size > SZ_64K ) {
		flush_cache_all();	/* L1 */
		smp_call_function((smp_call_func_t)__cpuc_flush_kern_all, NULL, 1);
	} else {
		dmac_flush_range(phys_to_virt(start), phys_to_virt(end));
	}

	outer_flush_range(start, end);	/* L2 */
}

int vb2_cma_phys_cache_flush(struct vb2_buffer *vb, u32 num_planes)
{
	struct vb2_cma_phys_buf *buf;
	unsigned long size = 0;
	int i;

	for (i = 0; i < num_planes; i++) {
		buf = vb->planes[i].mem_priv;
		if (!buf->cacheable) {
			pr_warning("This is non-cacheable buffer allocator\n");
			return -EINVAL;
		}

		size += buf->size;
	}

	if (size > (unsigned long)SIZE_THRESHOLD) {
		_vb2_cma_phys_cache_flush_all();
	} else {
		for (i = 0; i < num_planes; i++) {
			buf = vb->planes[i].mem_priv;
			_vb2_cma_phys_cache_flush_range(buf, size);
		}
	}

	return 0;
}

int vb2_cma_phys_cache_inv(struct vb2_buffer *vb, u32 num_planes)
{
	struct vb2_cma_phys_buf *buf;
	phys_addr_t start;
	size_t size;
	int i;

	for (i = 0; i < num_planes; i++) {
		buf = vb->planes[i].mem_priv;
		start = buf->dma_addr;
		size = buf->size;

		if (!buf->cacheable) {
			pr_warning("This is non-cacheable buffer allocator\n");
			return -EINVAL;
		}

		dmac_unmap_area(phys_to_virt(start), size, DMA_FROM_DEVICE);
		outer_inv_range(start, start + size);	/* L2 */
	}

	return 0;
}

int vb2_cma_phys_cache_clean(struct vb2_buffer *vb, u32 num_planes)
{
	struct vb2_cma_phys_buf *buf;
	phys_addr_t start;
	size_t size;
	int i;

	for (i = 0; i < num_planes; i++) {
		buf = vb->planes[i].mem_priv;
		start = buf->dma_addr;
		size = buf->size;

		if (!buf->cacheable) {
			pr_warning("This is non-cacheable buffer allocator\n");
			return -EINVAL;
		}

		dmac_unmap_area(phys_to_virt(start), size, DMA_TO_DEVICE);
		outer_clean_range(start, start + size - 1);	/* L2 */
	}

	return 0;
}

/* FIXME: l2 cache clean all should be implemented */
int vb2_cma_phys_cache_clean2(struct vb2_buffer *vb, u32 num_planes)
{
	struct vb2_cma_phys_buf *buf;
	unsigned long t_size = 0;
	phys_addr_t start;
	size_t size;
	int i;

	for (i = 0; i < num_planes; i++) {
		buf = vb->planes[i].mem_priv;
		if (!buf->cacheable) {
			pr_warning("This is non-cacheable buffer allocator\n");
			return -EINVAL;
		}

		t_size += buf->size;
	}

	if (t_size > (unsigned long)SIZE_THRESHOLD) {
		for (i = 0; i < num_planes; i++) {
			buf = vb->planes[i].mem_priv;
			start = buf->dma_addr;
			size = buf->size;

			dmac_unmap_area(phys_to_virt(start), size, DMA_TO_DEVICE);
		}
	} else {
		for (i = 0; i < num_planes; i++) {
			buf = vb->planes[i].mem_priv;
			start = buf->dma_addr;
			size = buf->size;

			dmac_unmap_area(phys_to_virt(start), size, DMA_TO_DEVICE);
			outer_clean_range(start, start + size - 1);	/* L2 */
		}
	}

	return 0;
}

MODULE_AUTHOR("Jonghun, Han <jonghun.han@samsung.com>");
MODULE_DESCRIPTION("CMA-phys allocator handling routines for videobuf2");
MODULE_LICENSE("GPL");
