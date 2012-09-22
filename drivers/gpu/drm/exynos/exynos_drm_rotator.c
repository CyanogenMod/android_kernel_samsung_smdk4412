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

#include "drmP.h"
#include "exynos_drm.h"
#include "exynos_drm_drv.h"
#include "exynos_drm_iommu.h"
#include "exynos_drm_ipp.h"

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

/* Buffer Address */
#define ROT_SRC_BUF_ADDR(n)		(0x30 + ((n) << 2))
#define ROT_DST_BUF_ADDR(n)		(0x50 + ((n) << 2))

/* Buffer Size */
#define ROT_SRC_BUF_SIZE		0x3c
#define ROT_DST_BUF_SIZE		0x5c
#define ROT_SET_BUF_SIZE_H(x)		((x) << 16)
#define ROT_SET_BUF_SIZE_W(x)		((x) << 0)
#define ROT_GET_BUF_SIZE_H(x)		((x) >> 16)
#define ROT_GET_BUF_SIZE_W(x)		((x) & 0xffff)

/* Crop Position */
#define ROT_SRC_CROP_POS		0x40
#define ROT_DST_CROP_POS		0x60
#define ROT_CROP_POS_Y(x)		((x) << 16)
#define ROT_CROP_POS_X(x)		((x) << 0)

/* Source Crop Size */
#define ROT_SRC_CROP_SIZE		0x44
#define ROT_SRC_CROP_SIZE_H(x)		((x) << 16)
#define ROT_SRC_CROP_SIZE_W(x)		((x) << 0)

/* Round to nearest aligned value */
#define ROT_ALIGN(x, align, mask)	(((x) + (1 << ((align) - 1))) & (mask))
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
	struct exynos_drm_ippdrv	ippdrv;
	int				cur_buf_id[EXYNOS_DRM_OPS_MAX];
	bool				suspended;
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

static u32 rotator_reg_get_format(struct rot_context *rot)
{
	u32 value = readl(rot->regs + ROT_CONTROL);
	value &= ROT_CONTROL_FMT_MASK;

	return value;
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
	case DRM_FORMAT_XRGB8888:
		value |= ROT_CONTROL_FMT_RGB888;
		break;
	default:
		DRM_ERROR("invalid image format\n");
		return;
	}

	writel(value, rot->regs + ROT_CONTROL);
}

static void rotator_reg_set_flip(struct rot_context *rot,
						enum drm_exynos_flip flip)
{
	u32 value = readl(rot->regs + ROT_CONTROL);
	value &= ~ROT_CONTROL_FLIP_MASK;

	switch (flip) {
	case EXYNOS_DRM_FLIP_VERTICAL:
		value |= ROT_CONTROL_FLIP_VERTICAL;
		break;
	case EXYNOS_DRM_FLIP_HORIZONTAL:
		value |= ROT_CONTROL_FLIP_HORIZONTAL;
		break;
	default:
		/* Flip None */
		break;
	}

	writel(value, rot->regs + ROT_CONTROL);
}

static void rotator_reg_set_rotation(struct rot_context *rot,
					enum drm_exynos_degree degree)
{
	u32 value = readl(rot->regs + ROT_CONTROL);
	value &= ~ROT_CONTROL_ROT_MASK;

	switch (degree) {
	case EXYNOS_DRM_DEGREE_90:
		value |= ROT_CONTROL_ROT_90;
		break;
	case EXYNOS_DRM_DEGREE_180:
		value |= ROT_CONTROL_ROT_180;
		break;
	case EXYNOS_DRM_DEGREE_270:
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

static void rotator_reg_get_src_buf_size(struct rot_context *rot, u32 *w,
									u32 *h)
{
	u32 value = readl(rot->regs + ROT_SRC_BUF_SIZE);

	*w = ROT_GET_BUF_SIZE_W(value);
	*h = ROT_GET_BUF_SIZE_H(value);
}

static void rotator_reg_set_src_buf_size(struct rot_context *rot, u32 w, u32 h)
{
	u32 value = ROT_SET_BUF_SIZE_H(h) | ROT_SET_BUF_SIZE_W(w);

	writel(value, rot->regs + ROT_SRC_BUF_SIZE);
}

static void rotator_reg_set_src_crop_pos(struct rot_context *rot, u32 x, u32 y)
{
	u32 value = ROT_CROP_POS_Y(y) | ROT_CROP_POS_X(x);

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

static void rotator_reg_get_dst_buf_size(struct rot_context *rot, u32 *w,
									u32 *h)
{
	u32 value = readl(rot->regs + ROT_DST_BUF_SIZE);

	*w = ROT_GET_BUF_SIZE_W(value);
	*h = ROT_GET_BUF_SIZE_H(value);
}

static void rotator_reg_set_dst_buf_size(struct rot_context *rot, u32 w, u32 h)
{
	u32 value = ROT_SET_BUF_SIZE_H(h) | ROT_SET_BUF_SIZE_W(w);

	writel(value, rot->regs + ROT_DST_BUF_SIZE);
}

static void rotator_reg_set_dst_crop_pos(struct rot_context *rot, u32 x, u32 y)
{
	u32 value = ROT_CROP_POS_Y(y) | ROT_CROP_POS_X(x);

	writel(value, rot->regs + ROT_DST_CROP_POS);
}

static void rotator_reg_get_dump(struct rot_context *rot)
{
	u32 value, i;

	for (i = 0; i <= ROT_DST_CROP_POS; i += 0x4) {
		value = readl(rot->regs + i);
		DRM_INFO("[%s] [0x%x] : 0x%x\n", __func__, i, value);
	}
}

static irqreturn_t rotator_irq_handler(int irq, void *arg)
{
	struct rot_context *rot = arg;
	struct exynos_drm_ippdrv *ippdrv = &rot->ippdrv;
	enum rot_irq_status irq_status;

	/* Get execution result */
	irq_status = rotator_reg_get_irq_status(rot);
	rotator_reg_set_irq_status_clear(rot, irq_status);

	if (irq_status == ROT_IRQ_STATUS_COMPLETE)
		ipp_send_event_handler(ippdrv,
					rot->cur_buf_id[EXYNOS_DRM_OPS_DST]);
	else {
		DRM_ERROR("the SFR is set illegally\n");
		rotator_reg_get_dump(rot);
	}

	return IRQ_HANDLED;
}

static void rotator_align_size(struct rot_context *rot, u32 fmt, u32 *hsize,
								u32 *vsize)
{
	struct rot_limit_table *limit_tbl = rot->limit_tbl;
	struct rot_limit *limit;
	u32 mask, value;

	/* Get size limit */
	if (fmt == ROT_CONTROL_FMT_RGB888)
		limit = &limit_tbl->rgb888;
	else
		limit = &limit_tbl->ycbcr420_2p;

	/* Get mask for rounding to nearest aligned value */
	mask = ~((1 << limit->align) - 1);

	/* Set aligned width */
	value = ROT_ALIGN(*hsize, limit->align, mask);
	if (value < limit->min_w)
		*hsize = ROT_MIN(limit->min_w, mask);
	else if (value > limit->max_w)
		*hsize = ROT_MAX(limit->max_w, mask);
	else
		*hsize = value;

	/* Set aligned height */
	value = ROT_ALIGN(*vsize, limit->align, mask);
	if (value < limit->min_h)
		*vsize = ROT_MIN(limit->min_h, mask);
	else if (value > limit->max_h)
		*vsize = ROT_MAX(limit->max_h, mask);
	else
		*vsize = value;
}

static int rotator_src_set_fmt(struct device *dev, u32 fmt)
{
	struct rot_context *rot = dev_get_drvdata(dev);

	/* Set format configuration */
	rotator_reg_set_format(rot, fmt);

	return 0;
}

static int rotator_src_set_size(struct device *dev, int swap,
						struct drm_exynos_pos *pos,
						struct drm_exynos_sz *sz)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	u32 fmt, hsize, vsize;

	/* Get format */
	fmt = rotator_reg_get_format(rot);

	/* Align buffer size */
	hsize = sz->hsize;
	vsize = sz->vsize;
	rotator_align_size(rot, fmt, &hsize, &vsize);

	/* Set buffer size configuration */
	rotator_reg_set_src_buf_size(rot, hsize, vsize);

	/* Set crop image position configuration */
	rotator_reg_set_src_crop_pos(rot, pos->x, pos->y);
	rotator_reg_set_src_crop_size(rot, pos->w, pos->h);

	return 0;
}

static int rotator_src_set_addr(struct device *dev,
				struct drm_exynos_ipp_buf_info *buf_info,
				u32 buf_id, enum drm_exynos_ipp_buf_ctrl ctrl)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	dma_addr_t addr[EXYNOS_DRM_PLANAR_MAX];
	u32 fmt, hsize, vsize;
	int i;

	/* Set current buf_id */
	rot->cur_buf_id[EXYNOS_DRM_OPS_SRC] = buf_id;

	switch (ctrl) {
	case IPP_BUF_CTRL_QUEUE:
		/* Set address configuration */
		for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
			addr[i] = buf_info->base[i];

		/* Get format */
		fmt = rotator_reg_get_format(rot);

		/* Re-set cb planar for NV12 format */
		if ((fmt == ROT_CONTROL_FMT_YCBCR420_2P) &&
					(addr[EXYNOS_DRM_PLANAR_CB] == 0x00)) {
			/* Get buf size */
			rotator_reg_get_src_buf_size(rot, &hsize, &vsize);

			/* Set cb planar */
			addr[EXYNOS_DRM_PLANAR_CB] =
				addr[EXYNOS_DRM_PLANAR_Y] + hsize * vsize;
		}

		for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
			rotator_reg_set_src_buf_addr(rot, addr[i], i);
		break;
	case IPP_BUF_CTRL_DEQUEUE:
		for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
			rotator_reg_set_src_buf_addr(rot, buf_info->base[i], i);
		break;
	default:
		/* Nothing to do */
		break;
	}

	return 0;
}

static int rotator_dst_set_transf(struct device *dev,
					enum drm_exynos_degree degree,
					enum drm_exynos_flip flip)
{
	struct rot_context *rot = dev_get_drvdata(dev);

	/* Set transform configuration */
	rotator_reg_set_flip(rot, flip);
	rotator_reg_set_rotation(rot, degree);

	/* Check degree for setting buffer size swap */
	if ((degree == EXYNOS_DRM_DEGREE_90) ||
		(degree == EXYNOS_DRM_DEGREE_270))
		return 1;
	else
		return 0;
}

static int rotator_dst_set_size(struct device *dev, int swap,
						struct drm_exynos_pos *pos,
						struct drm_exynos_sz *sz)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	u32 fmt, hsize, vsize;

	/* Get format */
	fmt = rotator_reg_get_format(rot);

	/* Align buffer size */
	hsize = sz->hsize;
	vsize = sz->vsize;
	rotator_align_size(rot, fmt, &hsize, &vsize);

	/* Set buffer size configuration */
	rotator_reg_set_dst_buf_size(rot, hsize, vsize);

	/* Set crop image position configuration */
	rotator_reg_set_dst_crop_pos(rot, pos->x, pos->y);

	return 0;
}

static int rotator_dst_set_addr(struct device *dev,
				struct drm_exynos_ipp_buf_info *buf_info,
				u32 buf_id, enum drm_exynos_ipp_buf_ctrl ctrl)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	dma_addr_t addr[EXYNOS_DRM_PLANAR_MAX];
	u32 fmt, hsize, vsize;
	int i;

	/* Set current buf_id */
	rot->cur_buf_id[EXYNOS_DRM_OPS_DST] = buf_id;

	switch (ctrl) {
	case IPP_BUF_CTRL_QUEUE:
		/* Set address configuration */
		for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
			addr[i] = buf_info->base[i];

		/* Get format */
		fmt = rotator_reg_get_format(rot);

		/* Re-set cb planar for NV12 format */
		if ((fmt == ROT_CONTROL_FMT_YCBCR420_2P) &&
					(addr[EXYNOS_DRM_PLANAR_CB] == 0x00)) {
			/* Get buf size */
			rotator_reg_get_dst_buf_size(rot, &hsize, &vsize);

			/* Set cb planar */
			addr[EXYNOS_DRM_PLANAR_CB] =
				addr[EXYNOS_DRM_PLANAR_Y] + hsize * vsize;
		}

		for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
			rotator_reg_set_dst_buf_addr(rot, addr[i], i);
		break;
	case IPP_BUF_CTRL_DEQUEUE:
		for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
			rotator_reg_set_dst_buf_addr(rot, buf_info->base[i], i);
		break;
	default:
		/* Nothing to do */
		break;
	}

	return 0;
}

static struct exynos_drm_ipp_ops rot_src_ops = {
	.set_fmt	=	rotator_src_set_fmt,
	.set_size	=	rotator_src_set_size,
	.set_addr	=	rotator_src_set_addr,
};

static struct exynos_drm_ipp_ops rot_dst_ops = {
	.set_transf	=	rotator_dst_set_transf,
	.set_size	=	rotator_dst_set_size,
	.set_addr	=	rotator_dst_set_addr,
};

static int rotator_ippdrv_check_property(struct device *dev,
				struct drm_exynos_ipp_property *property)
{
	struct drm_exynos_ipp_config *src_config =
					&property->config[EXYNOS_DRM_OPS_SRC];
	struct drm_exynos_ipp_config *dst_config =
					&property->config[EXYNOS_DRM_OPS_DST];
	struct drm_exynos_pos *src_pos = &src_config->pos;
	struct drm_exynos_pos *dst_pos = &dst_config->pos;
	struct drm_exynos_sz *src_sz = &src_config->sz;
	struct drm_exynos_sz *dst_sz = &dst_config->sz;
	bool swap = false;

	/* Check format configuration */
	if (src_config->fmt != dst_config->fmt) {
		DRM_DEBUG_KMS("[%s]not support csc feature\n", __func__);
		return -EINVAL;
	}

	switch (src_config->fmt) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV12M:
		/* No problem */
		break;
	default:
		DRM_DEBUG_KMS("[%s]not support format\n", __func__);
		return -EINVAL;
	}

	/* Check transform configuration */
	if (src_config->degree != EXYNOS_DRM_DEGREE_0) {
		DRM_DEBUG_KMS("[%s]not support source-side rotation\n",
								__func__);
		return -EINVAL;
	}

	switch (dst_config->degree) {
	case EXYNOS_DRM_DEGREE_90:
	case EXYNOS_DRM_DEGREE_270:
		swap = true;
	case EXYNOS_DRM_DEGREE_0:
	case EXYNOS_DRM_DEGREE_180:
		/* No problem */
		break;
	default:
		DRM_DEBUG_KMS("[%s]invalid degree\n", __func__);
		return -EINVAL;
	}

	if (src_config->flip != EXYNOS_DRM_FLIP_NONE) {
		DRM_DEBUG_KMS("[%s]not support source-side flip\n", __func__);
		return -EINVAL;
	}

	switch (dst_config->flip) {
	case EXYNOS_DRM_FLIP_NONE:
	case EXYNOS_DRM_FLIP_VERTICAL:
	case EXYNOS_DRM_FLIP_HORIZONTAL:
		/* No problem */
		break;
	default:
		DRM_DEBUG_KMS("[%s]invalid flip\n", __func__);
		return -EINVAL;
	}

	/* Check size configuration */
	if ((src_pos->x + src_pos->w > src_sz->hsize) ||
		(src_pos->y + src_pos->h > src_sz->vsize)) {
		DRM_DEBUG_KMS("[%s]out of source buffer bound\n", __func__);
		return -EINVAL;
	}

	if (swap) {
		if ((dst_pos->x + dst_pos->h > dst_sz->vsize) ||
			(dst_pos->y + dst_pos->w > dst_sz->hsize)) {
			DRM_DEBUG_KMS("[%s]out of destination buffer bound\n",
								__func__);
			return -EINVAL;
		}

		if ((src_pos->w != dst_pos->h) || (src_pos->h != dst_pos->w)) {
			DRM_DEBUG_KMS("[%s]not support scale feature\n",
								__func__);
			return -EINVAL;
		}
	} else {
		if ((dst_pos->x + dst_pos->w > dst_sz->hsize) ||
			(dst_pos->y + dst_pos->h > dst_sz->vsize)) {
			DRM_DEBUG_KMS("[%s]out of destination buffer bound\n",
								__func__);
			return -EINVAL;
		}

		if ((src_pos->w != dst_pos->w) || (src_pos->h != dst_pos->h)) {
			DRM_DEBUG_KMS("[%s]not support scale feature\n",
								__func__);
			return -EINVAL;
		}
	}

	return 0;
}

static int rotator_ippdrv_start(struct device *dev, enum drm_exynos_ipp_cmd cmd)
{
	struct rot_context *rot = dev_get_drvdata(dev);

	if (rot->suspended) {
		DRM_ERROR("suspended state\n");
		return -EPERM;
	}

	if (cmd != IPP_CMD_M2M) {
		DRM_ERROR("not support cmd: %d\n", cmd);
		return -EINVAL;
	}

	/* Set interrupt enable */
	rotator_reg_set_irq(rot, true);

	/* start rotator operation */
	rotator_reg_set_start(rot);

	return 0;
}

static int __devinit rotator_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rot_context *rot;
	struct resource *res;
	struct exynos_drm_ippdrv *ippdrv;
	int ret;

	rot = kzalloc(sizeof(*rot), GFP_KERNEL);
	if (!rot) {
		dev_err(dev, "failed to allocate rot\n");
		return -ENOMEM;
	}

	rot->limit_tbl = (struct rot_limit_table *)
				platform_get_device_id(pdev)->driver_data;

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

	ret = request_threaded_irq(rot->irq, NULL, rotator_irq_handler,
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

	ippdrv = &rot->ippdrv;
	ippdrv->dev = dev;
	ippdrv->iommu_used = true;
	ippdrv->ops[EXYNOS_DRM_OPS_SRC] = &rot_src_ops;
	ippdrv->ops[EXYNOS_DRM_OPS_DST] = &rot_dst_ops;
	ippdrv->check_property = rotator_ippdrv_check_property;
	ippdrv->start = rotator_ippdrv_start;

	platform_set_drvdata(pdev, rot);

	ret = exynos_drm_ippdrv_register(ippdrv);
	if (ret < 0) {
		dev_err(dev, "failed to register drm rotator device\n");
		goto err_ippdrv_register;
	}

	dev_info(dev, "The exynos rotator is probed successfully\n");

	return 0;

err_ippdrv_register:
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
	kfree(rot);
	return ret;
}

static int __devexit rotator_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rot_context *rot = dev_get_drvdata(dev);

	exynos_drm_ippdrv_unregister(&rot->ippdrv);

	pm_runtime_disable(dev);
	clk_put(rot->clock);

	free_irq(rot->irq, rot);

	iounmap(rot->regs);

	release_resource(rot->regs_res);
	kfree(rot->regs_res);

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
	struct exynos_drm_ippdrv *ippdrv = &rot->ippdrv;
	struct drm_device *drm_dev = ippdrv->drm_dev;
	struct exynos_drm_private *drm_priv = drm_dev->dev_private;

	rot->suspended = true;

	exynos_drm_iommu_deactivate(drm_priv->vmm, dev);

	return 0;
}

static int rotator_resume(struct device *dev)
{
	struct rot_context *rot = dev_get_drvdata(dev);
	struct exynos_drm_ippdrv *ippdrv = &rot->ippdrv;
	struct drm_device *drm_dev = ippdrv->drm_dev;
	struct exynos_drm_private *drm_priv = drm_dev->dev_private;
	int ret;

	ret = exynos_drm_iommu_activate(drm_priv->vmm, dev);
	if (ret)
		DRM_ERROR("failed to activate iommu\n");

	rot->suspended = false;

	return ret;
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
