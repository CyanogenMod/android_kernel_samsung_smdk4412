/*
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 * Authors: YoungJun Cho <yj44.cho@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundationr
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos_params.h>

#include "drmP.h"
#include "exynos_drm.h"
#include "exynos_drm_drv.h"
#include "exynos_drm_gem.h"
#include "exynos_drm_iommu.h"

/* Configuration */
#define ROT_CONFIG			0x00
#define ROT_CONFIG_IRQ			(3 << 8)

/* Image Control */
#define ROT_CONTROL			0x10
#define ROT_CONTROL_PATTERN_WRITE	(1 << 16)
#define ROT_CONTROL_FMT_YCBCR420_2P	(1 << 8)
#define ROT_CONTROL_FMT_RGB888		(6 << 8)
#define ROT_CONTROL_FMT_MASK		(7 << 8)
#define ROT_CONTROL_FLIP_VERTICAL	(2 << 6)
#define ROT_CONTROL_FLIP_HORIZONTAL	(3 << 6)
#define ROT_CONTROL_FLIP_MASK		(3 << 6)
#define ROT_CONTROL_ROT_90		(1 << 4)
#define ROT_CONTROL_ROT_180		(2 << 4)
#define ROT_CONTROL_ROT_270		(3 << 4)
#define ROT_CONTROL_ROT_MASK		(3 << 4)
#define ROT_CONTROL_START		(1 << 0)

/* Status */
#define ROT_STATUS			0x20
#define ROT_STATUS_IRQ_PENDING(x)	(1 << (x))
#define ROT_STATUS_IRQ(x)		(((x) >> 8) & 0x3)
#define ROT_STATUS_IRQ_VAL_COMPLETE	1
#define ROT_STATUS_IRQ_VAL_ILLEGAL	2

/* Sourc Buffer Address */
#define ROT_SRC_BUF_ADDR(n)		(0x30 + ((n) << 2))

/* Source Buffer Size */
#define ROT_SRC_BUF_SIZE		0x3c
#define ROT_SRC_BUF_SIZE_H(x)		((x) << 16)
#define ROT_SRC_BUF_SIZE_W(x)		((x) << 0)

/* Source Crop Position */
#define ROT_SRC_CROP_POS		0x40
#define ROT_SRC_CROP_POS_Y(x)		((x) << 16)
#define ROT_SRC_CROP_POS_X(x)		((x) << 0)

/* Source Crop Size */
#define ROT_SRC_CROP_SIZE		0x44
#define ROT_SRC_CROP_SIZE_H(x)		((x) << 16)
#define ROT_SRC_CROP_SIZE_W(x)		((x) << 0)

/* Destination Buffer Address */
#define ROT_DST_BUF_ADDR(n)		(0x50 + ((n) << 2))

/* Destination Buffer Size */
#define ROT_DST_BUF_SIZE		0x5c
#define ROT_DST_BUF_SIZE_H(x)		((x) << 16)
#define ROT_DST_BUF_SIZE_W(x)		((x) << 0)

/* Destination Crop Position */
#define ROT_DST_CROP_POS		0x60
#define ROT_DST_CROP_POS_Y(x)		((x) << 16)
#define ROT_DST_CROP_POS_X(x)		((x) << 0)

/* Round to nearest aligned value */
#define ROT_ALIGN(x, align, mask)	((*(x) + (1 << ((align) - 1))) & (mask))
/* Minimum limit value */
#define ROT_MIN(min, mask)		(((min) + ~(mask)) & (mask))
/* Maximum limit value */
#define ROT_MAX(max, mask)		((max) & (mask))

enum rot_irq_status {
	ROT_IRQ_STATUS_COMPLETE	= 8,
	ROT_IRQ_STATUS_ILLEGAL	= 9,
};

struct rot_limit {
	u32	min_w;
	u32	min_h;
	u32	max_w;
	u32	max_h;
	u32	align;
};

struct rot_limit_table {
	struct rot_limit	ycbcr420_2p;
	struct rot_limit	rgb888;
};

struct rot_context {
	struct rot_limit_table		*limit_tbl;
	struct clk			*clock;
	struct resource			*regs_res;
	void __iomem			*regs;
	int				irq;
	int				exec_ret;
	struct exynos_drm_subdrv	subdrv;
	struct completion		complete;
	struct mutex			exec_mutex;
	spinlock_t			irq_lock;
	struct pm_qos_request_list	pm_qos;
	bool				suspended;
};

struct rot_buffer {
	dma_addr_t	src_addr[DRM_EXYNOS_ROT_MAX_BUF];
	dma_addr_t	dst_addr[DRM_EXYNOS_ROT_MAX_BUF];
	void		*src_gem_obj[DRM_EXYNOS_ROT_MAX_BUF];
	void		*dst_gem_obj[DRM_EXYNOS_ROT_MAX_BUF];
	u32		src_cnt;
	u32		dst_cnt;
	u32		src_w;
	u32		src_h;
	u32		dst_w;
	u32		dst_h;
};

static void rotator_reg_set_irq(struct rot_context *rot, bool enable)
{
	u32 value = readl(rot->regs + ROT_CONFIG);

	if (enable == true)
		value |= ROT_CONFIG_IRQ;
	else
		value &= ~ROT_CONFIG_IRQ;

	writel(value, rot->regs + ROT_CONFIG);
}

static void rotator_reg_set_format(struct rot_context *rot, u32 img_fmt)
{
	u32 value = readl(rot->regs + ROT_CONTROL);
	value &= ~ROT_CONTROL_FMT_MASK;

	switch (img_fmt) {
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV12M:
		value |= ROT_CONTROL_FMT_YCBCR420_2P;
		break;
	case DRM_FORMAT_RGB888:
		value |= ROT_CONTROL_FMT_RGB888;
		break;
	default:
		DRM_ERROR("invalid image format\n");
		return;
	}

	writel(value, rot->regs + ROT_CONTROL);
}

static void rotator_reg_set_flip(struct rot_context *rot,
						enum drm_exynos_rot_flip flip)
{
	u32 value = readl(rot->regs + ROT_CONTROL);
	value &= ~ROT_CONTROL_FLIP_MASK;

	switch (flip) {
	case ROT_FLIP_VERTICAL:
		value |= ROT_CONTROL_FLIP_VERTICAL;
		break;
	case ROT_FLIP_HORIZONTAL:
		value |= ROT_CONTROL_FLIP_HORIZONTAL;
		break;
	default:
		/* Flip None */
		break;
	}

	writel(value, rot->regs + ROT_CONTROL);
}

static void rotator_reg_set_rotation(struct rot_context *rot,
					enum drm_exynos_rot_degree degree)
{
	u32 value = readl(rot->regs + ROT_CONTROL);
	value &= ~ROT_CONTROL_ROT_MASK;

	switch (degree) {
	case ROT_DEGREE_90:
		value |= ROT_CONTROL_ROT_90;
		break;
	case ROT_DEGREE_180:
		value |= ROT_CONTROL_ROT_180;
		break;
	case ROT_DEGREE_270:
		value |= ROT_CONTROL_ROT_270;
		break;
	default:
		/* Rotation 0 Degree */
		break;
	}

	writel(value, rot->regs + ROT_CONTROL);
}

static void rotator_reg_set_start(struct rot_context *rot)
{
	u32 value = readl(rot->regs + ROT_CONTROL);

	value |= ROT_CONTROL_START;

	writel(value, rot->regs + ROT_CONTROL);
}

static enum rot_irq_status rotator_reg_get_irq_status(struct rot_context *rot)
{
	u32 value = readl(rot->regs + ROT_STATUS);
	value = ROT_STATUS_IRQ(value);

	if (value == ROT_STATUS_IRQ_VAL_COMPLETE)
		return ROT_IRQ_STATUS_COMPLETE;
	else
		return ROT_IRQ_STATUS_ILLEGAL;
}

static void rotator_reg_set_irq_status_clear(struct rot_context *rot,
						enum rot_irq_status status)
{
	u32 value = readl(rot->regs + ROT_STATUS);

	value |= ROT_STATUS_IRQ_PENDING((u32)status);

	writel(value, rot->regs + ROT_STATUS);
}

static void rotator_reg_set_src_buf_addr(struct rot_context *rot,
							dma_addr_t addr, int i)
{
	writel(addr, rot->regs + ROT_SRC_BUF_ADDR(i));
}

static void rotator_reg_set_src_buf_size(struct rot_context *rot, u32 w, u32 h)
{
	u32 value = ROT_SRC_BUF_SIZE_H(h) | ROT_SRC_BUF_SIZE_W(w);

	writel(value, rot->regs + ROT_SRC_BUF_SIZE);
}

static void rotator_reg_set_src_crop_pos(struct rot_context *rot, u32 x, u32 y)
{
	u32 value = ROT_SRC_CROP_POS_Y(y) | ROT_SRC_CROP_POS_X(x);

	writel(value, rot->regs + ROT_SRC_CROP_POS);
}

static void rotator_reg_set_src_crop_size(struct rot_context *rot, u32 w, u32 h)
{
	u32 value = ROT_SRC_CROP_SIZE_H(h) | ROT_SRC_CROP_SIZE_W(w);

	writel(value, rot->regs + ROT_SRC_CROP_SIZE);
}

static void rotator_reg_set_dst_buf_addr(struct rot_context *rot,
							dma_addr_t addr, int i)
{
	writel(addr, rot->regs + ROT_DST_BUF_ADDR(i));
}

static void rotator_reg_set_dst_buf_size(struct rot_context *rot, u32 w, u32 h)
{
	u32 value = ROT_DST_BUF_SIZE_H(h) | ROT_DST_BUF_SIZE_W(w);

	writel(value, rot->regs + ROT_DST_BUF_SIZE);
}

static void rotator_reg_set_dst_crop_pos(struct rot_context *rot, u32 x, u32 y)
{
	u32 value = ROT_DST_CROP_POS_Y(y) | ROT_DST_CROP_POS_X(x);

	writel(value, rot->regs + ROT_DST_CROP_POS);
}

static void rotator_reg_get_dump(struct rot_context *rot)
{
	u32 value, i;

	for (i = 0; i <= ROT_DST_CROP_POS; i += 0x4) {
		value = readl(rot->regs + i);
		DRM_INFO("+0x%x: 0x%x", i, value);
	}
}

static bool rotator_check_format_n_handle_valid(u32 img_fmt,
							u32 src_buf_handle_cnt,
							u32 dst_buf_handle_cnt)
{
	bool ret = false;

	if ((src_buf_handle_cnt != dst_buf_handle_cnt)
						|| (src_buf_handle_cnt == 0))
		return ret;

	switch (img_fmt) {
	case DRM_FORMAT_NV12M:
		if (src_buf_handle_cnt == 2)
			ret = true;
		break;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_RGB888:
		if (src_buf_handle_cnt == 1)
			ret = true;
		break;
	default:
		DRM_ERROR("invalid image format\n");
		break;
	}

	return ret;
}

static void rotator_align_size(struct rot_limit *limit, u32 mask, u32 *w,
									u32 *h)
{
	u32 value;

	value = ROT_ALIGN(w, limit->align, mask);
	if (value < limit->min_w)
		*w = ROT_MIN(limit->min_w, mask);
	else if (value > limit->max_w)
		*w = ROT_MAX(limit->max_w, mask);
	else
		*w = value;

	value = ROT_ALIGN(h, limit->align, mask);
	if (value < limit->min_h)
		*h = ROT_MIN(limit->min_h, mask);
	else if (value > limit->max_h)
		*h = ROT_MAX(limit->max_h, mask);
	else
		*h = value;
}

static void rotator_align_buffer(struct rot_context *rot,
					struct rot_buffer *buf,
					struct drm_exynos_rot_buffer *req_buf,
					struct drm_exynos_rot_control *control)
{
	struct rot_limit_table *limit_tbl = rot->limit_tbl;
	struct rot_limit *limit;
	u32 mask;

	/* Get size limit */
	if (control->img_fmt == DRM_FORMAT_RGB888)
		limit = &limit_tbl->rgb888;
	else
		limit = &limit_tbl->ycbcr420_2p;

	/* Get mask for rounding to nearest aligned value */
	mask = ~((1 << limit->align) - 1);

	/* For source buffer */
	buf->src_w = req_buf->src_w;
	buf->src_h = req_buf->src_h;
	rotator_align_size(limit, mask, &buf->src_w, &buf->src_h);

	/* For destination buffer */
	buf->dst_w = req_buf->dst_w;
	buf->dst_h = req_buf->dst_h;
	rotator_align_size(limit, mask, &buf->dst_w, &buf->dst_h);
}

static bool rotator_check_crop_boundary(struct rot_buffer *buf,
					struct drm_exynos_rot_control *control,
					struct drm_exynos_rot_crop *crop)
{
	bool ret = true;

	/* Check source crop position */
	if ((crop->src_x + crop->src_w > buf->src_w)
				|| (crop->src_y + crop->src_h > buf->src_h))
		return false;

	/* Check destination crop position */
	switch (control->degree) {
	case ROT_DEGREE_90:
	case ROT_DEGREE_270:
		if ((crop->dst_x + crop->src_h > buf->dst_w)
				|| (crop->dst_y + crop->src_w > buf->dst_h))
			ret = false;
		break;
	default:
		if ((crop->dst_x + crop->src_w > buf->dst_w)
				|| (crop->dst_y + crop->src_h > buf->dst_h))
			ret = false;
		break;
	}

	return ret;
}

static int rotator_iommu_map(struct rot_buffer *buf,
					struct drm_exynos_rot_buffer *req_buf,
					struct iommu_gem_map_params *params,
					struct list_head *iommu_list)
{
	/* For source buffer */
	buf->src_cnt = 0;
	while (buf->src_cnt < req_buf->src_cnt) {
		buf->src_addr[buf->src_cnt] = exynos_drm_iommu_map_gem(params,
					iommu_list,
					req_buf->src_handle[buf->src_cnt],
					IOMMU_ROTATOR);
		if (!buf->src_addr[buf->src_cnt]) {
			DRM_ERROR("failed to map src handle[%u]\n",
								buf->src_cnt);
			return -EINVAL;
		}
		buf->src_gem_obj[(buf->src_cnt)++] = params->gem_obj;
	}

	/* For destination buffer */
	buf->dst_cnt = 0;
	while (buf->dst_cnt < req_buf->dst_cnt) {
		buf->dst_addr[buf->dst_cnt] = exynos_drm_iommu_map_gem(params,
					iommu_list,
					req_buf->dst_handle[buf->dst_cnt],
					IOMMU_ROTATOR);
		if (!buf->dst_addr[buf->dst_cnt]) {
			DRM_ERROR("failed to map dst handle[%u]\n",
								buf->dst_cnt);
			return -EINVAL;
		}
		buf->dst_gem_obj[(buf->dst_cnt)++] = params->gem_obj;
	}

	return 0;
}

static void rotator_iommu_unmap(struct rot_buffer *buf,
					struct iommu_gem_map_params *params)
{
	/* For destination buffer */
	while (buf->dst_cnt > 0) {
		params->gem_obj = buf->dst_gem_obj[--(buf->dst_cnt)];
		exynos_drm_iommu_unmap_gem(params,
						buf->dst_addr[buf->dst_cnt],
						IOMMU_ROTATOR);
	}

	/* For source buffer */
	while (buf->src_cnt > 0) {
		params->gem_obj = buf->src_gem_obj[--(buf->src_cnt)];
		exynos_drm_iommu_unmap_gem(params,
						buf->src_addr[buf->src_cnt],
						IOMMU_ROTATOR);
	}
}

static void rotator_execute(struct rot_context *rot,
					struct rot_buffer *buf,
					struct drm_exynos_rot_control *control,
					struct drm_exynos_rot_crop *crop)
{
	int i;

	pm_runtime_get_sync(rot->subdrv.dev);

	/* Set interrupt enable */
	rotator_reg_set_irq(rot, true);

	/* Set control registers */
	rotator_reg_set_format(rot, control->img_fmt);
	rotator_reg_set_flip(rot, control->flip);
	rotator_reg_set_rotation(rot, control->degree);

	/* Set source buffer address */
	for (i = 0; i < DRM_EXYNOS_ROT_MAX_BUF; i++)
		rotator_reg_set_src_buf_addr(rot, buf->src_addr[i], i);

	/* Set source buffer size */
	rotator_reg_set_src_buf_size(rot, buf->src_w, buf->src_h);

	/* Set destination buffer address */
	for (i = 0; i < DRM_EXYNOS_ROT_MAX_BUF; i++)
		rotator_reg_set_dst_buf_addr(rot, buf->dst_addr[i], i);

	/* Set destination buffer size */
	rotator_reg_set_dst_buf_size(rot, buf->dst_w, buf->dst_h);

	/* Set source crop image position */
	rotator_reg_set_src_crop_pos(rot, crop->src_x, crop->src_y);

	/* Set source crop image size */
	rotator_reg_set_src_crop_size(rot, crop->src_w, crop->src_h);

	/* Set destination crop image position */
	rotator_reg_set_dst_crop_pos(rot, crop->dst_x, crop->dst_y);

	/* Start rotator operation */
	rotator_reg_set_start(rot);
}

int exynos_drm_rotator_exec_ioctl(struct drm_device *drm_dev, void *data,
							struct drm_file *file)
{
	struct drm_exynos_file_private *file_priv = file->driver_priv;
	struct exynos_drm_rot_private *priv = file_priv->rot_priv;
	struct device *dev = priv->dev;
	struct rot_context *rot;
	struct drm_exynos_rot_exec_data *req = data;
	struct drm_exynos_rot_buffer *req_buf = &req->buf;
	struct drm_exynos_rot_control *control = &req->control;
	struct drm_exynos_rot_crop *crop = &req->crop;
	struct rot_buffer buf;
	struct iommu_gem_map_params params;

	if (!dev) {
		DRM_ERROR("failed to get dev\n");
		return -ENODEV;
	}

	rot = dev_get_drvdata(dev);
	if (!rot) {
		DRM_ERROR("failed to get drvdata\n");
		return -EFAULT;
	}

	if (rot->suspended) {
		DRM_ERROR("suspended state\n");
		return -EPERM;
	}

	if (!rotator_check_format_n_handle_valid(control->img_fmt,
							req_buf->src_cnt,
							req_buf->dst_cnt)) {
		DRM_ERROR("format or handles are invalid\n");
		return -EINVAL;
	}

	init_completion(&rot->complete);

	/* Align buffer */
	rotator_align_buffer(rot, &buf, req_buf, control);

	/* Check crop boundary */
	if (!rotator_check_crop_boundary(&buf, control, crop)) {
		DRM_ERROR("boundary errror\n");
		return -EINVAL;
	}

	params.dev = dev;
	params.drm_dev = drm_dev;
	params.file = file;

	/* Map IOMMU */
	rot->exec_ret = rotator_iommu_map(&buf, req_buf, &params,
							&priv->iommu_list);
	if (rot->exec_ret < 0)
		goto err_iommu_map;

	/* Assign another src/dst_addr for NV12 image format */
	if (control->img_fmt == DRM_FORMAT_NV12) {
		u32 size = crop->src_w * crop->src_h;

		buf.src_addr[buf.src_cnt + 1] =
					buf.src_addr[buf.src_cnt] + size;
		buf.dst_addr[buf.dst_cnt + 1] =
					buf.dst_addr[buf.dst_cnt] + size;
	}

	/* Execute */
	mutex_lock(&rot->exec_mutex);
	rotator_execute(rot, &buf, control, crop);
	if (!wait_for_completion_timeout(&rot->complete, 2 * HZ)) {
		DRM_ERROR("timeout error\n");
		rot->exec_ret = -ETIMEDOUT;
		mutex_unlock(&rot->exec_mutex);
		goto err_iommu_map;
	}
	mutex_unlock(&rot->exec_mutex);

	/* Unmap IOMMU */
	rotator_iommu_unmap(&buf, &params);

	return rot->exec_ret;

err_iommu_map:
	rotator_iommu_unmap(&buf, &params);
	return rot->exec_ret;
}
EXPORT_SYMBOL_GPL(exynos_drm_rotator_exec_ioctl);

static irqreturn_t rotator_irq_thread(int irq, void *arg)
{
	struct rot_context *rot = (struct rot_context *)arg;
	enum rot_irq_status irq_status;
	unsigned long flags;

	pm_qos_update_request(&rot->pm_qos, 0);

	/* Get execution result */
	spin_lock_irqsave(&rot->irq_lock, flags);
	irq_status = rotator_reg_get_irq_status(rot);
	rotator_reg_set_irq_status_clear(rot, irq_status);
	spin_unlock_irqrestore(&rot->irq_lock, flags);

	rot->exec_ret = 0;
	if (irq_status != ROT_IRQ_STATUS_COMPLETE) {
		DRM_ERROR("the SFR is set illegally\n");
		rot->exec_ret = -EINVAL;
		rotator_reg_get_dump(rot);
	}

	pm_runtime_put(rot->subdrv.dev);

	complete(&rot->complete);

	return IRQ_HANDLED;
}

static int rotator_subdrv_open(struct drm_device *drm_dev, struct device *dev,
							struct drm_file *file)
{
	struct drm_exynos_file_private *file_priv = file->driver_priv;
	struct exynos_drm_rot_private *priv;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(dev, "failed to allocate priv\n");
		return -ENOMEM;
	}

	priv->dev = dev;
	INIT_LIST_HEAD(&priv->iommu_list);

	file_priv->rot_priv = priv;

	return 0;
}

static void rotator_subdrv_close(struct drm_device *drm_dev, struct device *dev,
							struct drm_file *file)
{
	struct drm_exynos_file_private *file_priv = file->driver_priv;
	struct exynos_drm_rot_private *priv = file_priv->rot_priv;
	struct iommu_gem_map_params params;
	struct iommu_info_node *node, *n;

	params.dev = dev;
	params.drm_dev = drm_dev;
	params.file = file;

	list_for_each_entry_safe(node, n, &priv->iommu_list, list) {
		params.gem_obj = node->gem_obj;
		exynos_drm_iommu_unmap_gem(&params, node->dma_addr,
								IOMMU_ROTATOR);
		list_del(&node->list);
		kfree(node);
		node = NULL;
	}

	kfree(priv);

	return;
}

static int __devinit rotator_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rot_context *rot;
	struct resource *res;
	struct exynos_drm_subdrv *subdrv;
	int ret;

	rot = kzalloc(sizeof(*rot), GFP_KERNEL);
	if (!rot) {
		dev_err(dev, "failed to allocate rot\n");
		return -ENOMEM;
	}

	rot->limit_tbl = (struct rot_limit_table *)
				platform_get_device_id(pdev)->driver_data;

	mutex_init(&rot->exec_mutex);
	spin_lock_init(&rot->irq_lock);

	ret = exynos_drm_iommu_setup(dev);
	if (ret < 0) {
		dev_err(dev, "failed to setup iommu\n");
		goto err_iommu_setup;
	}

	ret = exynos_drm_iommu_activate(dev);
	if (ret < 0) {
		dev_err(dev, "failed to activate iommu\n");
		goto err_iommu_activate;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "failed to find registers\n");
		ret = -ENOENT;
		goto err_get_resource;
	}

	rot->regs_res = request_mem_region(res->start, resource_size(res),
								dev_name(dev));
	if (!rot->regs_res) {
		dev_err(dev, "failed to claim register region\n");
		ret = -ENOENT;
		goto err_get_resource;
	}

	rot->regs = ioremap(res->start, resource_size(res));
	if (!rot->regs) {
		dev_err(dev, "failed to map register\n");
		ret = -ENXIO;
		goto err_ioremap;
	}

	rot->irq = platform_get_irq(pdev, 0);
	if (rot->irq < 0) {
		dev_err(dev, "faild to get irq\n");
		ret = rot->irq;
		goto err_get_irq;
	}

	ret = request_threaded_irq(rot->irq, NULL, rotator_irq_thread,
					IRQF_ONESHOT, "drm_rotator", rot);
	if (ret < 0) {
		dev_err(dev, "failed to request irq\n");
		goto err_get_irq;
	}

	rot->clock = clk_get(dev, "rotator");
	if (IS_ERR_OR_NULL(rot->clock)) {
		dev_err(dev, "faild to get clock\n");
		ret = PTR_ERR(rot->clock);
		goto err_clk_get;
	}

	pm_runtime_enable(dev);
	pm_qos_add_request(&rot->pm_qos, PM_QOS_BUS_DMA_THROUGHPUT, 0);

	subdrv = &rot->subdrv;
	subdrv->dev = dev;
	subdrv->open = rotator_subdrv_open;
	subdrv->close = rotator_subdrv_close;

	platform_set_drvdata(pdev, rot);

	ret = exynos_drm_subdrv_register(subdrv);
	if (ret < 0) {
		dev_err(dev, "failed to register drm rotator device\n");
		goto err_subdrv_register;
	}

	dev_info(dev, "The exynos rotator is probed successfully\n");

	return 0;

err_subdrv_register:
	pm_runtime_disable(dev);
	clk_put(rot->clock);
err_clk_get:
	free_irq(rot->irq, rot);
err_get_irq:
	iounmap(rot->regs);
err_ioremap:
	release_resource(rot->regs_res);
	kfree(rot->regs_res);
err_get_resource:
	exynos_drm_iommu_deactivate(dev);
err_iommu_activate:
	exynos_drm_iommu_cleanup(dev);
err_iommu_setup:
	kfree(rot);
	return ret;
}

static int __devexit rotator_remove(struct platform_device *pdev)
{
	struct rot_context *rot = platform_get_drvdata(pdev);

	pm_qos_remove_request(&rot->pm_qos);

	exynos_drm_subdrv_unregister(&rot->subdrv);

	pm_runtime_disable(&pdev->dev);
	clk_put(rot->clock);

	free_irq(rot->irq, rot);

	iounmap(rot->regs);

	release_resource(rot->regs_res);
	kfree(rot->regs_res);

	exynos_drm_iommu_deactivate(&pdev->dev);
	exynos_drm_iommu_cleanup(&pdev->dev);

	kfree(rot);

	return 0;
}

struct rot_limit_table rot_limit_tbl = {
	.ycbcr420_2p = {
		.min_w = 32,
		.min_h = 32,
		.max_w = SZ_32K,
		.max_h = SZ_32K,
		.align = 3,
	},
	.rgb888 = {
		.min_w = 8,
		.min_h = 8,
		.max_w = SZ_8K,
		.max_h = SZ_8K,
		.align = 2,
	},
};

struct platform_device_id rotator_driver_ids[] = {
	{
		.name		= "exynos-rot",
		.driver_data	= (unsigned long)&rot_limit_tbl,
	},
	{},
};

#ifdef CONFIG_PM_SLEEP
static int rotator_suspend(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);

	/* Check & wait for running state */
	mutex_lock(&rot->exec_mutex);
	mutex_unlock(&rot->exec_mutex);

	rot->suspended = true;

	exynos_drm_iommu_deactivate(dev);

	return 0;
}

static int rotator_resume(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);

	rot->suspended = false;

	exynos_drm_iommu_activate(dev);

	return 0;
}
#endif

#ifdef CONFIG_PM_RUNTIME
static int rotator_runtime_suspend(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);

	clk_disable(rot->clock);

	return 0;
}

static int rotator_runtime_resume(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);

	clk_enable(rot->clock);
	pm_qos_update_request(&rot->pm_qos, 400000);

	return 0;
}
#endif

static const struct dev_pm_ops rotator_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(rotator_suspend, rotator_resume)
	SET_RUNTIME_PM_OPS(rotator_runtime_suspend, rotator_runtime_resume,
									NULL)
};

struct platform_driver rotator_driver = {
	.probe		= rotator_probe,
	.remove		= __devexit_p(rotator_remove),
	.id_table	= rotator_driver_ids,
	.driver		= {
		.name	= "exynos-rot",
		.owner	= THIS_MODULE,
		.pm	= &rotator_pm_ops,
	},
};
