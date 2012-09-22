/* exynos_drm_gem.h
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
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

#ifndef _EXYNOS_DRM_GEM_H_
#define _EXYNOS_DRM_GEM_H_

#define to_exynos_gem_obj(x)	container_of(x,\
			struct exynos_drm_gem_obj, base)

/* FIMD/HDMI/G2D/FIMC/G3D */
#define MAX_IOMMU_NR	5

#define IS_NONCONTIG_BUFFER(f)	((f & EXYNOS_BO_NONCONTIG) ||\
					(f & EXYNOS_BO_USERPTR))

struct exynos_drm_private_cb {
	unsigned int (*get_handle)(unsigned int id);
	int (*add_buffer)(void *obj, unsigned int *handle, unsigned int *id);
	void (*release_buffer)(unsigned int handle);
};

/*
 * exynos drm iommu information structure.
 *
 * @mapped: flag a bit of indicating whether any driver's device address
 *	is mapped to its own iommu or not.
 * @dma_addrs: contain device address to each device driver using iommu.
 * @devs: device objects that requested mapping to iommu.
 */
struct exynos_drm_iommu_info {
	unsigned int		mapped;
	dma_addr_t		dma_addrs[MAX_IOMMU_NR];
	struct device		*devs[MAX_IOMMU_NR];
	struct list_head	*iommu_lists[MAX_IOMMU_NR];
	/* TODO. */
};

/*
 * exynos drm gem buffer structure.
 *
 * @kvaddr: kernel virtual address to allocated memory region.
 * *userptr: user space address.
 * @dma_addr: bus address(accessed by dma) to allocated memory region.
 * @dev_addr: device address for IOMMU.
 * @paddr: physical address to allocated buffer.
 * @write: whether pages will be written to by the caller.
 * @sgt: sg table to transfer page data.
 * @pages: contain all pages to allocated memory region.
 * @page_size: could be 4K, 64K or 1MB.
 * @size: size of allocated memory region.
 * @shared: indicate shared mfc memory region.
 *	(temporarily used and it should be removed later.)
 * @pfnmap: indicate whether memory region from userptr is mmaped with
 *	VM_PFNMAP or not.
 */
struct exynos_drm_gem_buf {
	struct device		*dev;
	void __iomem		*kvaddr;
	unsigned long		userptr;
	dma_addr_t		dma_addr;
	dma_addr_t		dev_addr;
	dma_addr_t		paddr;
	unsigned int		write;
	struct sg_table		*sgt;
	struct page		**pages;
	unsigned long		page_size;
	unsigned long		size;
	bool			shared;
	bool			pfnmap;
};

/*
 * exynos drm buffer structure.
 *
 * @base: a gem object.
 *	- a new handle to this gem object would be created
 *	by drm_gem_handle_create().
 * @buffer: a pointer to exynos_drm_gem_buffer object.
 *	- contain the information to memory region allocated
 *	by user request or at framebuffer creation.
 *	continuous memory region allocated by user request
 *	or at framebuffer creation.
 * @iommu_info: contain iommu mapping information to each device driver
 *	using its own iommu.
 * @size: size requested from user, in bytes and this size is aligned
 *	in page unit.
 * @packed_size: real size of the gem object, in bytes and
 *	this size isn't aligned in page unit.
 * @flags: indicate memory type to allocated buffer and cache attruibute.
 * @vmm: vmm object for iommu framework.
 * @priv_handle: handle to specific buffer object.
 * @priv_id: unique id to specific buffer object.
 *
 * P.S. this object would be transfered to user as kms_bo.handle so
 *	user can access the buffer through kms_bo.handle.
 */
struct exynos_drm_gem_obj {
	struct drm_gem_object		base;
	struct exynos_drm_gem_buf	*buffer;
	struct exynos_drm_iommu_info	iommu_info;
	unsigned long			size;
	unsigned long			packed_size;
	struct vm_area_struct		*vma;
	unsigned int			flags;
	void				*vmm;
	unsigned int			priv_handle;
	unsigned int			priv_id;
};

/* register private callback. */
void exynos_drm_priv_cb_register(struct exynos_drm_private_cb *cb);

/* register a buffer object to private buffer manager. */
int register_buf_to_priv_mgr(struct exynos_drm_gem_obj *obj,
		unsigned int *priv_handle, unsigned int *priv_id);

struct page **exynos_gem_get_pages(struct drm_gem_object *obj, gfp_t gfpmask);

int exynos_drm_gem_user_limit_ioctl(struct drm_device *dev, void *data,
				      struct drm_file *filp);

/* destroy a buffer with gem object */
void exynos_drm_gem_destroy(struct exynos_drm_gem_obj *exynos_gem_obj);

/* create a private gem object and initialize it. */
struct exynos_drm_gem_obj *exynos_drm_gem_init(struct drm_device *dev,
						      unsigned long size);

/* create a new buffer with gem object */
struct exynos_drm_gem_obj *exynos_drm_gem_create(struct drm_device *dev,
						unsigned int flags,
						unsigned long size);

/*
 * request gem object creation and buffer allocation as the size
 * that it is calculated with framebuffer information such as width,
 * height and bpp.
 */
int exynos_drm_gem_create_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv);

/*
 * get dma address from gem handle and this function could be used for
 * other drivers such as 2d/3d acceleration drivers.
 * with this function call, gem object reference count would be increased.
 */
void *exynos_drm_gem_get_dma_addr(struct drm_device *dev,
					unsigned int gem_handle,
					struct drm_file *filp,
					unsigned int *gem_obj);

/*
 * put dma address from gem handle and this function could be used for
 * other drivers such as 2d/3d acceleration drivers.
 * with this function call, gem object reference count would be decreased.
 */
void exynos_drm_gem_put_dma_addr(struct drm_device *dev, void *gem_obj);

/* get buffer offset to map to user space. */
int exynos_drm_gem_map_offset_ioctl(struct drm_device *dev, void *data,
				    struct drm_file *file_priv);

/*
 * mmap the physically continuous memory that a gem object contains
 * to user space.
 */
int exynos_drm_gem_mmap_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv);

/* map user space allocated by malloc to pages. */
int exynos_drm_gem_userptr_ioctl(struct drm_device *dev, void *data,
				      struct drm_file *file_priv);

/* get buffer information to memory region allocated by gem. */
int exynos_drm_gem_get_ioctl(struct drm_device *dev, void *data,
				      struct drm_file *file_priv);

/* get buffer size to gem handle. */
unsigned long exynos_drm_gem_get_size(struct drm_device *dev,
						unsigned int gem_handle,
						struct drm_file *file_priv);

/* initialize gem object. */
int exynos_drm_gem_init_object(struct drm_gem_object *obj);

/* free gem object. */
void exynos_drm_gem_free_object(struct drm_gem_object *gem_obj);

/* create memory region for drm framebuffer. */
int exynos_drm_gem_dumb_create(struct drm_file *file_priv,
			       struct drm_device *dev,
			       struct drm_mode_create_dumb *args);

/* map memory region for drm framebuffer to user space. */
int exynos_drm_gem_dumb_map_offset(struct drm_file *file_priv,
				   struct drm_device *dev, uint32_t handle,
				   uint64_t *offset);

/*
 * destroy memory region allocated.
 *	- a gem handle and physical memory region pointed by a gem object
 *	would be released by drm_gem_handle_delete().
 */
int exynos_drm_gem_dumb_destroy(struct drm_file *file_priv,
				struct drm_device *dev,
				unsigned int handle);

/* page fault handler and mmap fault address(virtual) to physical memory. */
int exynos_drm_gem_fault(struct vm_area_struct *vma, struct vm_fault *vmf);

/* set vm_flags and we can change the vm attribute to other one at here. */
int exynos_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma);

/* get ump sequre id for UMP. */
int exynos_drm_gem_export_ump_ioctl(struct drm_device *dev, void *data,
		struct drm_file *file);

/* do user desired cache operation. */
int exynos_drm_gem_cache_op_ioctl(struct drm_device *drm_dev, void *data,
		struct drm_file *file_priv);

/* temporary functions. */
/* get physical address from a gem. */
int exynos_drm_gem_get_phy_ioctl(struct drm_device *drm_dev, void *data,
		struct drm_file *file_priv);
/* import physical memory to a gem. */
int exynos_drm_gem_phy_imp_ioctl(struct drm_device *drm_dev, void *data,
		struct drm_file *file_priv);

void exynos_drm_gem_close_object(struct drm_gem_object *obj,
				struct drm_file *file);

struct exynos_drm_gem_obj *exynos_drm_gem_get_obj(struct drm_device *dev,
						unsigned int gem_handle,
						struct drm_file *file_priv);

#endif
