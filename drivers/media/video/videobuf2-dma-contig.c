/*
 * videobuf2-dma-contig.c - DMA contig memory allocator for videobuf2
 *
 * Copyright (C) 2010 Samsung Electronics
 *
 * Author: Pawel Osciak <pawel@osciak.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/dma-buf.h>

#include <media/videobuf2-core.h>
#include <media/videobuf2-memops.h>

struct vb2_dc_conf {
	struct device		*dev;
};

struct vb2_dc_buf {
	struct vb2_dc_conf		*conf;
	void				*vaddr;
	dma_addr_t			dma_addr;
	unsigned long			size;
	struct vm_area_struct		*vma;
	struct dma_buf_attachment	*db_attach;
	atomic_t			refcount;
	struct vb2_vmarea_handler	handler;
};

struct vb2_dc_db_attach {
	struct vb2_dc_buf		*buf;
	struct dma_buf_attachment	db_attach;
};

static void vb2_dma_contig_put(void *buf_priv);

static void *vb2_dma_contig_alloc(void *alloc_ctx, unsigned long size)
{
	struct vb2_dc_conf *conf = alloc_ctx;
	struct vb2_dc_buf *buf;
	/* TODO: add db_attach processing while adding DMABUF as exporter */

	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->vaddr = dma_alloc_coherent(conf->dev, size, &buf->dma_addr,
					GFP_KERNEL);
	if (!buf->vaddr) {
		dev_err(conf->dev, "dma_alloc_coherent of size %ld failed\n",
			size);
		kfree(buf);
		return ERR_PTR(-ENOMEM);
	}

	buf->conf = conf;
	buf->size = size;

	buf->handler.refcount = &buf->refcount;
	buf->handler.put = vb2_dma_contig_put;
	buf->handler.arg = buf;

	atomic_inc(&buf->refcount);

	return buf;
}

static void vb2_dma_contig_put(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	if (atomic_dec_and_test(&buf->refcount)) {
		dma_free_coherent(buf->conf->dev, buf->size, buf->vaddr,
				  buf->dma_addr);
		kfree(buf);
	}
}

static void *vb2_dma_contig_cookie(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	return &buf->dma_addr;
}

static void *vb2_dma_contig_vaddr(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;
	if (!buf)
		return 0;

	return buf->vaddr;
}

static unsigned int vb2_dma_contig_num_users(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	return atomic_read(&buf->refcount);
}

static int vb2_dma_contig_mmap(void *buf_priv, struct vm_area_struct *vma)
{
	struct vb2_dc_buf *buf = buf_priv;

	if (!buf) {
		printk(KERN_ERR "No buffer to map\n");
		return -EINVAL;
	}

	WARN_ON(buf->db_attach);

	return vb2_mmap_pfn_range(vma, buf->dma_addr, buf->size,
				  &vb2_common_vm_ops, &buf->handler);
}

static void *vb2_dma_contig_get_userptr(void *alloc_ctx, unsigned long vaddr,
					unsigned long size, int write)
{
	struct vb2_dc_buf *buf;
	struct vm_area_struct *vma;
	dma_addr_t dma_addr = 0;
	int ret;

	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	ret = vb2_get_contig_userptr(vaddr, size, &vma, &dma_addr);
	if (ret) {
		printk(KERN_ERR "Failed acquiring VMA for vaddr 0x%08lx\n",
				vaddr);
		kfree(buf);
		return ERR_PTR(ret);
	}

	buf->size = size;
	buf->dma_addr = dma_addr;
	buf->vma = vma;

	return buf;
}

static void vb2_dma_contig_put_userptr(void *mem_priv)
{
	struct vb2_dc_buf *buf = mem_priv;

	if (!buf)
		return;

	vb2_put_vma(buf->vma);
	kfree(buf);
}

static void vb2_dma_contig_map_dmabuf(void *mem_priv)
{
	struct vb2_dc_buf *buf = mem_priv;
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
	 *  convert sglist to paddr:
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

static void vb2_dma_contig_unmap_dmabuf(void *mem_priv)
{
	struct vb2_dc_buf *buf = mem_priv;
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

static void *vb2_dma_contig_attach_dmabuf(void *alloc_ctx, struct dma_buf *dbuf)
{
	struct vb2_dc_conf *conf = alloc_ctx;
	struct vb2_dc_buf *buf;
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

static void vb2_dma_contig_detach_dmabuf(void *mem_priv)
{
	struct vb2_dc_buf *buf = mem_priv;

	if (!buf)
		return;

	if (buf->dma_addr)
		vb2_dma_contig_unmap_dmabuf(buf);

	/* detach this attachment */
	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	buf->db_attach = NULL;

	kfree(buf);
}

const struct vb2_mem_ops vb2_dma_contig_memops = {
	.alloc		= vb2_dma_contig_alloc,
	.put		= vb2_dma_contig_put,
	.cookie		= vb2_dma_contig_cookie,
	.vaddr		= vb2_dma_contig_vaddr,
	.mmap		= vb2_dma_contig_mmap,
	.get_userptr	= vb2_dma_contig_get_userptr,
	.put_userptr	= vb2_dma_contig_put_userptr,
	.map_dmabuf	= vb2_dma_contig_map_dmabuf,
	.unmap_dmabuf	= vb2_dma_contig_unmap_dmabuf,
	.attach_dmabuf	= vb2_dma_contig_attach_dmabuf,
	.detach_dmabuf	= vb2_dma_contig_detach_dmabuf,
	.num_users	= vb2_dma_contig_num_users,
};
EXPORT_SYMBOL_GPL(vb2_dma_contig_memops);

void *vb2_dma_contig_init_ctx(struct device *dev)
{
	struct vb2_dc_conf *conf;

	conf = kzalloc(sizeof *conf, GFP_KERNEL);
	if (!conf)
		return ERR_PTR(-ENOMEM);

	conf->dev = dev;

	return conf;
}
EXPORT_SYMBOL_GPL(vb2_dma_contig_init_ctx);

void vb2_dma_contig_cleanup_ctx(void *alloc_ctx)
{
	kfree(alloc_ctx);
}
EXPORT_SYMBOL_GPL(vb2_dma_contig_cleanup_ctx);

MODULE_DESCRIPTION("DMA-contig memory handling routines for videobuf2");
MODULE_AUTHOR("Pawel Osciak <pawel@osciak.com>");
MODULE_LICENSE("GPL");
