/*
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 * Authors:
 *	Eunchul Kim <chulspro.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include "drmP.h"
#include "drm_backlight.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/cma.h>
#include <plat/map-base.h>

#include "regs-gsc.h"
#include <drm/exynos_drm.h>
#include "exynos_drm_drv.h"
#include "exynos_drm_gem.h"
#include "exynos_drm_ipp.h"
#include "exynos_drm_gsc.h"

/*
 * GSC is stand for General SCaler and
 * supports image scaler/rotator and input/output DMA operations.
 * input DMA reads image data from the memory.
 * output DMA writes image data to memory.
 * GSC supports image rotation and image effect functions.
 */

#define GSC_MAX_DEVS	4
#define GSC_MAX_SRC		8
#define GSC_MAX_DST		32
#define GSC_RESET_TIMEOUT	50
#ifdef CONFIG_SLP_DISP_DEBUG
#define GSC_MAX_REG		128
#define GSC_BASE_REG(id)	(0x13E00000 + (0x10000 * id))
#endif
#define GSC_CLK_RATE	166750000
#define GSC_BUF_STOP	1
#define GSC_BUF_START	2
#define GSC_REG_SZ		32
#define GSC_WIDTH_ITU_709	1280

#define get_gsc_context(dev)	platform_get_drvdata(to_platform_device(dev))
#define get_ctx_from_ippdrv(ippdrv)	container_of(ippdrv,\
					struct gsc_context, ippdrv);
#define gsc_read(offset)		readl(ctx->regs + (offset));
#define gsc_write(cfg, offset)	writel(cfg, ctx->regs + (offset));

enum gsc_wb {
	GSC_WB_NONE,
	GSC_WB_A,
	GSC_WB_B,
};

/*
 * A structure of scaler.
 *
 * @range: narrow, wide.
 * @pre_shfactor: pre sclaer shift factor.
 * @pre_hratio: horizontal ratio of the prescaler.
 * @pre_vratio: vertical ratio of the prescaler.
 * @main_hratio: the main scaler's horizontal ratio.
 * @main_vratio: the main scaler's vertical ratio.
 */
struct gsc_scaler {
	bool	range;
	u32	pre_shfactor;
	u32	pre_hratio;
	u32	pre_vratio;
	unsigned long main_hratio;
	unsigned long main_vratio;
};

/*
 * A structure of scaler capability.
 *
 * find user manual 49.2 features.
 * @tile_w: tile mode or rotation width.
 * @tile_h: tile mode or rotation height.
 * @w: other cases width.
 * @h: other cases height.
 */
struct gsc_capability {
	/* tile or rotation */
	u32	tile_w;
	u32	tile_h;
	/* other cases */
	u32	w;
	u32	h;
};

/*
 * A structure of gsc context.
 *
 * @ippdrv: prepare initialization using ippdrv.
 * @regs_res: register resources.
 * @regs: memory mapped io registers.
 * @lock: locking of operations.
 * @gsc_clk: gsc clock.
 * @sc: scaler infomations.
 * @capa: scaler capability.
 * @id: gsc id.
 * @irq: irq number.
 * @suspended: qos operations.
 */
struct gsc_context {
	struct exynos_drm_ippdrv	ippdrv;
	struct resource	*regs_res;
	void __iomem	*regs;
	struct mutex	lock;
	struct clk	*gsc_clk;
	struct gsc_scaler	sc;
	struct gsc_capability	*capa;
	int	id;
	int	irq;
	bool	suspended;
};

struct gsc_capability gsc51_capa[GSC_MAX_DEVS] = {
	{
		.tile_w = 2048,
		.tile_h = 2048,
		.w = 4800,
		.h = 3344,
	}, {
		.tile_w = 2048,
		.tile_h = 2048,
		.w = 4800,
		.h = 3344,
	}, {
		.tile_w = 2048,
		.tile_h = 2048,
		.w = 4800,
		.h = 3344,
	}, {
		.tile_w = 2048,
		.tile_h = 2048,
		.w = 4800,
		.h = 3344,
	},
};

static int gsc_sw_reset(struct gsc_context *ctx)
{
	u32 cfg;
	int count = GSC_RESET_TIMEOUT;

	DRM_DEBUG_KMS("%s\n", __func__);

	/* s/w reset */
	cfg = (GSC_SW_RESET_SRESET);
	gsc_write(cfg, GSC_SW_RESET);

	/* wait s/w reset complete */
	while (count--) {
		cfg = gsc_read(GSC_SW_RESET);
		if (!cfg)
			break;
		usleep_range(1000, 2000);
	}

	if (cfg) {
		DRM_ERROR("failed to reset gsc h/w.\n");
		return -EBUSY;
	}

	/* display fifo reset */
	cfg = readl(SYSREG_GSCBLK_CFG0);
	/*
	 * GSCBLK Pixel asyncy FIFO S/W reset sequence
	 * set PXLASYNC_SW_RESET as 0 then,
	 * set PXLASYNC_SW_RESET as 1 again
	 */
	cfg &= ~GSC_PXLASYNC_RST(ctx->id);
	writel(cfg, SYSREG_GSCBLK_CFG0);
	cfg |= GSC_PXLASYNC_RST(ctx->id);
	writel(cfg, SYSREG_GSCBLK_CFG0);

	/* pixel async reset */
	cfg = readl(SYSREG_DISP1BLK_CFG);
	/*
	 * DISPBLK1 FIFO S/W reset sequence
	 * set FIFORST_DISP1 as 0 then,
	 * set FIFORST_DISP1 as 1 again
	 */
	cfg &= ~FIFORST_DISP1;
	writel(cfg, SYSREG_DISP1BLK_CFG);
	cfg |= FIFORST_DISP1;
	writel(cfg, SYSREG_DISP1BLK_CFG);

	/* reset sequence */
	cfg = gsc_read(GSC_IN_BASE_ADDR_Y_MASK);
	cfg |= (GSC_IN_BASE_ADDR_MASK |
		GSC_IN_BASE_ADDR_PINGPONG(0));
	gsc_write(cfg, GSC_IN_BASE_ADDR_Y_MASK);
	gsc_write(cfg, GSC_IN_BASE_ADDR_CB_MASK);
	gsc_write(cfg, GSC_IN_BASE_ADDR_CR_MASK);

	cfg = gsc_read(GSC_OUT_BASE_ADDR_Y_MASK);
	cfg |= (GSC_OUT_BASE_ADDR_MASK |
		GSC_OUT_BASE_ADDR_PINGPONG(0));
	gsc_write(cfg, GSC_OUT_BASE_ADDR_Y_MASK);
	gsc_write(cfg, GSC_OUT_BASE_ADDR_CB_MASK);
	gsc_write(cfg, GSC_OUT_BASE_ADDR_CR_MASK);

	return 0;
}

static void gsc_set_gscblk_fimd_wb(struct gsc_context *ctx, bool enable)
{
	u32 gscblk_cfg;

	DRM_DEBUG_KMS("%s\n", __func__);

	gscblk_cfg = readl(SYSREG_GSCBLK_CFG1);

	if (enable)
		gscblk_cfg |= GSC_BLK_DISP1WB_DEST(ctx->id) |
				GSC_BLK_GSCL_WB_IN_SRC_SEL(ctx->id) |
				GSC_BLK_SW_RESET_WB_DEST(ctx->id);
	else
		gscblk_cfg |= GSC_BLK_PXLASYNC_LO_MASK_WB(ctx->id);

	writel(gscblk_cfg, SYSREG_GSCBLK_CFG1);
}

static void gsc_handle_irq(struct gsc_context *ctx, bool enable,
	bool overflow, bool done)
{
	u32 cfg;

	DRM_DEBUG_KMS("%s:enable[%d]overflow[%d]level[%d]\n", __func__,
			enable, overflow, done);

	cfg = gsc_read(GSC_IRQ);
	cfg |= (GSC_IRQ_OR_MASK | GSC_IRQ_FRMDONE_MASK);

	if (enable) {
		cfg |= GSC_IRQ_ENABLE;
		if (overflow)
			cfg &= ~GSC_IRQ_OR_MASK;
		if (done)
			cfg &= ~GSC_IRQ_FRMDONE_MASK;
	} else
		cfg &= ~GSC_IRQ_ENABLE;

	gsc_write(cfg, GSC_IRQ);
}

static int gsc_set_planar_addr(struct drm_exynos_ipp_buf_info *buf_info,
			       u32 fmt, struct drm_exynos_sz *sz)
{
	dma_addr_t *y_addr = &buf_info->base[EXYNOS_DRM_PLANAR_Y];
	dma_addr_t *cb_addr = &buf_info->base[EXYNOS_DRM_PLANAR_CB];
	dma_addr_t *cr_addr = &buf_info->base[EXYNOS_DRM_PLANAR_CR];
	uint64_t y_ofs, cb_ofs, cr_ofs;

	/*
	* ToDo: check the buffer size between gem allocated buffers
	* and each planar size.
	*/
	switch (fmt) {
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
		y_ofs = sz->hsize * sz->vsize;
		cb_ofs = y_ofs >> 1;
		cr_ofs = 0;
		break;
	case DRM_FORMAT_NV12M:
		y_ofs = ALIGN(ALIGN(sz->hsize, 16) *
				ALIGN(sz->vsize, 16), SZ_2K);
		cb_ofs = ALIGN(ALIGN(sz->hsize, 16) *
				ALIGN(sz->vsize >> 1, 16), SZ_2K);
		cr_ofs = 0;
		break;
	case DRM_FORMAT_NV12MT:
		y_ofs = ALIGN(ALIGN(sz->hsize, 128) *
				ALIGN(sz->vsize, 32), SZ_8K);
		cb_ofs = ALIGN(ALIGN(sz->hsize, 128) *
				ALIGN(sz->vsize >> 1, 32), SZ_8K);
		cr_ofs = 0;
		break;
	case DRM_FORMAT_YUV410:
	case DRM_FORMAT_YVU410:
	case DRM_FORMAT_YUV411:
	case DRM_FORMAT_YVU411:
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YUV422:
	case DRM_FORMAT_YVU422:
	case DRM_FORMAT_YUV444:
	case DRM_FORMAT_YVU444:
	case DRM_FORMAT_YUV420M:
		y_ofs = sz->hsize * sz->vsize;
		cb_ofs = cr_ofs = y_ofs >> 2;
		break;
	default:
		y_ofs = cb_ofs = cr_ofs = 0;
		break;
	}

	if (y_ofs && *y_addr) {
		*cb_addr = *y_addr + y_ofs;

		if (cb_ofs && *cb_addr)
			*cr_addr = *cb_addr + cb_ofs;
	}

	return 0;
}

static int gsc_src_set_fmt(struct device *dev, u32 fmt)
{
	struct gsc_context *ctx = get_gsc_context(dev);
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	u32 cfg;

	DRM_DEBUG_KMS("%s:fmt[0x%x]\n", __func__, fmt);

	cfg = gsc_read(GSC_IN_CON);
	cfg &= ~(GSC_IN_RGB_TYPE_MASK | GSC_IN_YUV422_1P_ORDER_MASK |
		 GSC_IN_CHROMA_ORDER_MASK | GSC_IN_FORMAT_MASK |
		 GSC_IN_TILE_TYPE_MASK | GSC_IN_TILE_MODE);

	switch (fmt) {
	case DRM_FORMAT_RGB565:
		cfg |= GSC_IN_RGB565;
		break;
	case DRM_FORMAT_XRGB8888:
		cfg |= GSC_IN_XRGB8888;
		break;
	case DRM_FORMAT_YUYV:
		cfg |= (GSC_IN_YUV422_1P |
			GSC_IN_YUV422_1P_ORDER_LSB_Y |
			GSC_IN_CHROMA_ORDER_CBCR);
		break;
	case DRM_FORMAT_YVYU:
		cfg |= (GSC_IN_YUV422_1P |
			GSC_IN_YUV422_1P_ORDER_LSB_Y |
			GSC_IN_CHROMA_ORDER_CRCB);
		break;
	case DRM_FORMAT_UYVY:
		cfg |= (GSC_IN_YUV422_1P |
			GSC_IN_YUV422_1P_OEDER_LSB_C |
			GSC_IN_CHROMA_ORDER_CBCR);
		break;
	case DRM_FORMAT_VYUY:
		cfg |= (GSC_IN_YUV422_1P |
			GSC_IN_YUV422_1P_OEDER_LSB_C |
			GSC_IN_CHROMA_ORDER_CRCB);
		break;
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV61:
		cfg |= (GSC_IN_CHROMA_ORDER_CRCB |
			GSC_IN_YUV420_2P);
		break;
	case DRM_FORMAT_YUV422:
		cfg |= GSC_IN_YUV422_3P;
		break;
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
		cfg |= GSC_IN_YUV420_3P;
		break;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV12M:
	case DRM_FORMAT_NV16:
		cfg |= (GSC_IN_CHROMA_ORDER_CBCR |
			GSC_IN_YUV420_2P);
		break;
	case DRM_FORMAT_NV12MT:
		cfg |= (GSC_IN_TILE_C_16x8 | GSC_IN_TILE_MODE);
		break;
	default:
		dev_err(ippdrv->dev, "inavlid target yuv order 0x%x.\n", fmt);
		return -EINVAL;
	}

	gsc_write(cfg, GSC_IN_CON);

	return 0;
}

static int gsc_src_set_transf(struct device *dev,
		enum drm_exynos_degree degree,
		enum drm_exynos_flip flip)
{
	struct gsc_context *ctx = get_gsc_context(dev);
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	u32 cfg;

	DRM_DEBUG_KMS("%s:degree[%d]flip[0x%x]\n", __func__,
		degree, flip);

	cfg = gsc_read(GSC_IN_CON);
	cfg &= ~GSC_IN_ROT_MASK;

	switch (degree) {
	case EXYNOS_DRM_DEGREE_0:
		if (flip & EXYNOS_DRM_FLIP_HORIZONTAL)
			cfg |= GSC_IN_ROT_XFLIP;
		if (flip & EXYNOS_DRM_FLIP_VERTICAL)
			cfg |= GSC_IN_ROT_YFLIP;
		break;
	case EXYNOS_DRM_DEGREE_90:
		if (flip & EXYNOS_DRM_FLIP_HORIZONTAL)
			cfg |= GSC_IN_ROT_90_XFLIP;
		else if (flip & EXYNOS_DRM_FLIP_VERTICAL)
			cfg |= GSC_IN_ROT_90_YFLIP;
		else
			cfg |= GSC_IN_ROT_90;
		break;
	case EXYNOS_DRM_DEGREE_180:
		cfg |= GSC_IN_ROT_180;
		break;
	case EXYNOS_DRM_DEGREE_270:
		cfg |= GSC_IN_ROT_270;
		break;
	default:
		dev_err(ippdrv->dev, "inavlid degree value %d.\n", degree);
		return -EINVAL;
	}

	gsc_write(cfg, GSC_IN_CON);

	return cfg ? 1 : 0;
}

static int gsc_src_set_size(struct device *dev, int swap,
	struct drm_exynos_pos *pos, struct drm_exynos_sz *sz)
{
	struct gsc_context *ctx = get_gsc_context(dev);
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	struct drm_exynos_ipp_property *property = ippdrv->property;
	struct drm_exynos_ipp_config *config =
		&property->config[EXYNOS_DRM_OPS_SRC];
	struct drm_exynos_pos img_pos = *pos;
	struct drm_exynos_sz img_sz = *sz;
	u32 cfg;

	/* ToDo: check width and height */
	if (swap) {
		img_pos.w = pos->h;
		img_pos.h = pos->w;
		img_sz.hsize = sz->vsize;
		img_sz.vsize = sz->hsize;
	}

	DRM_DEBUG_KMS("%s:x[%d]y[%d]w[%d]h[%d]\n",
		__func__, pos->x, pos->y, pos->w, pos->h);

	/* pixel offset */
	cfg = (GSC_SRCIMG_OFFSET_X(img_pos.x) |
		GSC_SRCIMG_OFFSET_Y(img_pos.y));
	gsc_write(cfg, GSC_SRCIMG_OFFSET);

	/* cropped size */
	cfg = (GSC_CROPPED_WIDTH(img_pos.w) |
		GSC_CROPPED_HEIGHT(img_pos.h));
	gsc_write(cfg, GSC_CROPPED_SIZE);

	DRM_DEBUG_KMS("%s:swap[%d]hsize[%d]vsize[%d]\n",
		__func__, swap, sz->hsize, sz->vsize);

	/* original size */
	cfg = gsc_read(GSC_SRCIMG_SIZE);
	cfg &= ~(GSC_SRCIMG_HEIGHT_MASK |
		GSC_SRCIMG_WIDTH_MASK);

	cfg |= (GSC_SRCIMG_WIDTH(sz->hsize) |
		GSC_SRCIMG_HEIGHT(sz->vsize));

	gsc_write(cfg, GSC_SRCIMG_SIZE);

	config->sz = img_sz;
	config->pos = img_pos;

	return 0;
}

static int gsc_src_set_buf_seq(struct gsc_context *ctx, u32 buf_id,
	enum drm_exynos_ipp_buf_ctrl buf_ctrl)
{
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	bool masked;
	u32 cfg;
	u32 mask = 0x00000001 << buf_id;

	DRM_DEBUG_KMS("%s:buf_id[%d]buf_ctrl[%d]\n", __func__,
		buf_id, buf_ctrl);

	/* mask register set */
	cfg = gsc_read(GSC_IN_BASE_ADDR_Y_MASK);

	switch (buf_ctrl) {
	case IPP_BUF_CTRL_QUEUE:
		masked = false;
		break;
	case IPP_BUF_CTRL_DEQUEUE:
		masked = true;
		break;
	default:
		dev_err(ippdrv->dev, "invalid buf ctrl parameter.\n");
		return -EINVAL;
	}

	/* sequence id */
	cfg &= (~mask);
	cfg |= masked << buf_id;
	gsc_write(cfg, GSC_IN_BASE_ADDR_Y_MASK);
	gsc_write(cfg, GSC_IN_BASE_ADDR_CB_MASK);
	gsc_write(cfg, GSC_IN_BASE_ADDR_CR_MASK);

	return 0;
}

static int gsc_src_set_addr(struct device *dev,
			struct drm_exynos_ipp_buf_info *buf_info, u32 buf_id,
			enum drm_exynos_ipp_buf_ctrl buf_ctrl)
{
	struct gsc_context *ctx = get_gsc_context(dev);
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	struct drm_exynos_ipp_property *property = ippdrv->property;
	struct drm_exynos_ipp_config *config =
		&property->config[EXYNOS_DRM_OPS_SRC];
	int ret;

	DRM_DEBUG_KMS("%s:buf_id[%d]buf_ctrl[%d]\n", __func__,
		buf_id, buf_ctrl);

	if (buf_id > GSC_MAX_SRC) {
		dev_info(ippdrv->dev, "inavlid buf_id %d.\n", buf_id);
		return -ENOMEM;
	}

	/* address register set */
	switch (buf_ctrl) {
	case IPP_BUF_CTRL_QUEUE:
	case IPP_BUF_CTRL_DEQUEUE:
		ret = gsc_set_planar_addr(buf_info, config->fmt, &config->sz);

		if (ret) {
			dev_err(dev, "failed to set plane addr.\n");
			return ret;
		}

		gsc_write(buf_info->base[EXYNOS_DRM_PLANAR_Y],
			GSC_IN_BASE_ADDR_Y(buf_id));
		gsc_write(buf_info->base[EXYNOS_DRM_PLANAR_CB],
			GSC_IN_BASE_ADDR_CB(buf_id));
		gsc_write(buf_info->base[EXYNOS_DRM_PLANAR_CR],
			GSC_IN_BASE_ADDR_CR(buf_id));
		break;
	default:
		/* bypass */
		break;
	}

	return gsc_src_set_buf_seq(ctx, buf_id, buf_ctrl);
}

static struct exynos_drm_ipp_ops gsc_src_ops = {
	.set_fmt = gsc_src_set_fmt,
	.set_transf = gsc_src_set_transf,
	.set_size = gsc_src_set_size,
	.set_addr = gsc_src_set_addr,
};

static int gsc_dst_set_fmt(struct device *dev, u32 fmt)
{
	struct gsc_context *ctx = get_gsc_context(dev);
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	u32 cfg;

	DRM_DEBUG_KMS("%s:fmt[0x%x]\n", __func__, fmt);

	cfg = gsc_read(GSC_OUT_CON);
	cfg &= ~(GSC_OUT_RGB_TYPE_MASK | GSC_OUT_YUV422_1P_ORDER_MASK |
		 GSC_OUT_CHROMA_ORDER_MASK | GSC_OUT_FORMAT_MASK |
		 GSC_OUT_TILE_TYPE_MASK | GSC_OUT_TILE_MODE);

	switch (fmt) {
	case DRM_FORMAT_RGB565:
		cfg |= GSC_OUT_RGB565;
		break;
	case DRM_FORMAT_XRGB8888:
		cfg |= GSC_OUT_XRGB8888;
		break;
	case DRM_FORMAT_YUYV:
		cfg |= (GSC_OUT_YUV422_1P |
			GSC_OUT_YUV422_1P_ORDER_LSB_Y |
			GSC_OUT_CHROMA_ORDER_CBCR);
		break;
	case DRM_FORMAT_YVYU:
		cfg |= (GSC_OUT_YUV422_1P |
			GSC_OUT_YUV422_1P_ORDER_LSB_Y |
			GSC_OUT_CHROMA_ORDER_CRCB);
		break;
	case DRM_FORMAT_UYVY:
		cfg |= (GSC_OUT_YUV422_1P |
			GSC_OUT_YUV422_1P_OEDER_LSB_C |
			GSC_OUT_CHROMA_ORDER_CBCR);
		break;
	case DRM_FORMAT_VYUY:
		cfg |= (GSC_OUT_YUV422_1P |
			GSC_OUT_YUV422_1P_OEDER_LSB_C |
			GSC_OUT_CHROMA_ORDER_CRCB);
		break;
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV61:
		cfg |= (GSC_OUT_CHROMA_ORDER_CRCB |
			GSC_OUT_YUV420_2P);
		break;
	case DRM_FORMAT_YUV422:
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
		cfg |= GSC_OUT_YUV420_3P;
		break;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV12M:
	case DRM_FORMAT_NV16:
		cfg |= (GSC_OUT_CHROMA_ORDER_CBCR |
			GSC_OUT_YUV420_2P);
		break;
	case DRM_FORMAT_NV12MT:
		cfg |= (GSC_OUT_TILE_C_16x8 | GSC_OUT_TILE_MODE);
		break;
	default:
		dev_err(ippdrv->dev, "inavlid target yuv order 0x%x.\n", fmt);
		return -EINVAL;
	}

	gsc_write(cfg, GSC_OUT_CON);

	return 0;
}

static int gsc_dst_set_transf(struct device *dev,
		enum drm_exynos_degree degree,
		enum drm_exynos_flip flip)
{
	struct gsc_context *ctx = get_gsc_context(dev);
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	u32 cfg;

	DRM_DEBUG_KMS("%s:degree[%d]flip[0x%x]\n", __func__,
		degree, flip);

	cfg = gsc_read(GSC_IN_CON);
	cfg &= ~GSC_IN_ROT_MASK;

	switch (degree) {
	case EXYNOS_DRM_DEGREE_0:
		if (flip & EXYNOS_DRM_FLIP_HORIZONTAL)
			cfg |= GSC_IN_ROT_XFLIP;
		if (flip & EXYNOS_DRM_FLIP_VERTICAL)
			cfg |= GSC_IN_ROT_YFLIP;
		break;
	case EXYNOS_DRM_DEGREE_90:
		if (flip & EXYNOS_DRM_FLIP_HORIZONTAL)
			cfg |= GSC_IN_ROT_90_XFLIP;
		else if (flip & EXYNOS_DRM_FLIP_VERTICAL)
			cfg |= GSC_IN_ROT_90_YFLIP;
		else
			cfg |= GSC_IN_ROT_90;
		break;
	case EXYNOS_DRM_DEGREE_180:
		cfg |= GSC_IN_ROT_180;
		break;
	case EXYNOS_DRM_DEGREE_270:
		cfg |= GSC_IN_ROT_270;
		break;
	default:
		dev_err(ippdrv->dev, "inavlid degree value %d.\n", degree);
		return -EINVAL;
	}

	gsc_write(cfg, GSC_IN_CON);

	return cfg ? 1 : 0;
}

static int gsc_get_ratio_shift(u32 src, u32 dst, u32 *ratio, u32 *shift)
{
	DRM_DEBUG_KMS("%s:src[%d]dst[%d]\n", __func__, src, dst);

	if (src >= dst * 64) {
		DRM_ERROR("failed to make ratio and shift.\n");
		return -EINVAL;
	} else if (src >= dst * 32) {
		*ratio = 32;
		*shift = 5;
	} else if (src >= dst * 16) {
		*ratio = 16;
		*shift = 4;
	} else if (src >= dst * 8) {
		*ratio = 8;
		*shift = 3;
	} else if (src >= dst * 4) {
		*ratio = 4;
		*shift = 2;
	} else if (src >= dst * 2) {
		*ratio = 2;
		*shift = 1;
	} else {
		*ratio = 1;
		*shift = 0;
	}

	return 0;
}

static int gsc_set_prescaler(struct gsc_context *ctx, struct gsc_scaler *sc,
		struct drm_exynos_pos *src, struct drm_exynos_pos *dst)
{
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	u32 cfg;
	u32 hfactor, vfactor;
	int ret = 0;

	ret = gsc_get_ratio_shift(src->w, dst->w, &sc->pre_hratio, &hfactor);
	if (ret) {
		dev_err(ippdrv->dev, "failed to get ratio horizontal.\n");
		return ret;
	}

	ret = gsc_get_ratio_shift(src->h, dst->h, &sc->pre_vratio, &vfactor);
	if (ret) {
		dev_err(ippdrv->dev, "failed to get ratio vertical.\n");
		return ret;
	}

	DRM_DEBUG_KMS("%s:pre_hratio[%d]hfactor[%d]pre_vratio[%d]vfactor[%d]\n",
		__func__, sc->pre_hratio, hfactor, sc->pre_vratio, vfactor);

	sc->main_hratio = (src->w << 16) / (dst->w << hfactor);
	sc->main_vratio = (src->h << 16) / (dst->h << vfactor);
	DRM_DEBUG_KMS("%s:main_hratio[%ld]main_vratio[%ld]\n",
	__func__, sc->main_hratio, sc->main_vratio);

	sc->pre_shfactor = 10 - (hfactor + vfactor);
	DRM_DEBUG_KMS("%s:pre_shfactor[%d]\n", __func__,
		sc->pre_shfactor);

	cfg = (GSC_PRESC_SHFACTOR(sc->pre_shfactor) |
		GSC_PRESC_H_RATIO(sc->pre_hratio) |
		GSC_PRESC_V_RATIO(sc->pre_vratio));
	gsc_write(cfg, GSC_PRE_SCALE_RATIO);

	return ret;
}

static void gsc_set_scaler(struct gsc_context *ctx, struct gsc_scaler *sc)
{
	u32 cfg;

	DRM_DEBUG_KMS("%s:main_hratio[%ld]main_vratio[%ld]\n",
		__func__, sc->main_hratio, sc->main_vratio);

	cfg = GSC_MAIN_H_RATIO_VALUE(sc->main_hratio);
	gsc_write(cfg, GSC_MAIN_H_RATIO);

	cfg = GSC_MAIN_V_RATIO_VALUE(sc->main_vratio);
	gsc_write(cfg, GSC_MAIN_V_RATIO);
}

static int gsc_dst_set_size(struct device *dev, int swap,
	struct drm_exynos_pos *pos, struct drm_exynos_sz *sz)
{
	struct gsc_context *ctx = get_gsc_context(dev);
	struct drm_exynos_pos img_pos = *pos;
	struct drm_exynos_sz img_sz = *sz;
	struct gsc_scaler *sc = &ctx->sc;
	u32 cfg;

	DRM_DEBUG_KMS("%s:swap[%d]x[%d]y[%d]w[%d]h[%d]\n",
		__func__, swap, pos->x, pos->y, pos->w, pos->h);

	if (swap) {
		img_pos.w = pos->h;
		img_pos.h = pos->w;
		img_sz.hsize = sz->vsize;
		img_sz.vsize = sz->hsize;
	}

	/* pixel offset */
	cfg = (GSC_DSTIMG_OFFSET_X(img_pos.x) |
		GSC_DSTIMG_OFFSET_Y(img_pos.y));
	gsc_write(cfg, GSC_DSTIMG_OFFSET);

	/* scaled size */
	cfg = (GSC_SCALED_WIDTH(pos->w) | GSC_SCALED_HEIGHT(pos->h));
	gsc_write(cfg, GSC_SCALED_SIZE);

	DRM_DEBUG_KMS("%s:hsize[%d]vsize[%d]\n",
		__func__, sz->hsize, sz->vsize);

	/* original size */
	cfg = gsc_read(GSC_DSTIMG_SIZE);
	cfg &= ~(GSC_DSTIMG_HEIGHT_MASK |
		GSC_DSTIMG_WIDTH_MASK);
	cfg |= (GSC_DSTIMG_WIDTH(img_sz.hsize) |
		GSC_DSTIMG_HEIGHT(img_sz.vsize));
	gsc_write(cfg, GSC_DSTIMG_SIZE);

	cfg = gsc_read(GSC_OUT_CON);
	cfg &= ~GSC_OUT_RGB_TYPE_MASK;

	if (pos->w >= GSC_WIDTH_ITU_709)
		if (sc->range)
			cfg |= GSC_OUT_RGB_HD_WIDE;
		else
			cfg |= GSC_OUT_RGB_HD_NARROW;
	else
		if (sc->range)
			cfg |= GSC_OUT_RGB_SD_WIDE;
		else
			cfg |= GSC_OUT_RGB_SD_NARROW;

	gsc_write(cfg, GSC_OUT_CON);

	return 0;
}

static int gsc_dst_get_buf_seq(struct gsc_context *ctx)
{
	u32 cfg, i, buf_num = GSC_REG_SZ;
	u32 mask = 0x00000001;

	cfg = gsc_read(GSC_OUT_BASE_ADDR_Y_MASK);

	for (i = 0; i < GSC_REG_SZ; i++)
		if (cfg & (mask << i))
			buf_num--;

	DRM_DEBUG_KMS("%s:buf_num[%d]\n", __func__, buf_num);

	return buf_num;
}

static int gsc_dst_set_buf_seq(struct gsc_context *ctx, u32 buf_id,
	enum drm_exynos_ipp_buf_ctrl buf_ctrl)
{
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	bool masked;
	u32 cfg;
	u32 mask = 0x00000001 << buf_id;

	DRM_DEBUG_KMS("%s:buf_id[%d]buf_ctrl[%d]\n", __func__,
		buf_id, buf_ctrl);

	/* mask register set */
	cfg = gsc_read(GSC_OUT_BASE_ADDR_Y_MASK);

	switch (buf_ctrl) {
	case IPP_BUF_CTRL_QUEUE:
		masked = false;
		break;
	case IPP_BUF_CTRL_DEQUEUE:
		masked = true;
		break;
	default:
		dev_err(ippdrv->dev, "invalid buf ctrl parameter.\n");
		return -EINVAL;
	}

	/* sequence id */
	cfg &= (~mask);
	cfg |= masked << buf_id;
	gsc_write(cfg, GSC_OUT_BASE_ADDR_Y_MASK);
	gsc_write(cfg, GSC_OUT_BASE_ADDR_CB_MASK);
	gsc_write(cfg, GSC_OUT_BASE_ADDR_CR_MASK);

	/* interrupt enable */
	if (buf_ctrl == IPP_BUF_CTRL_QUEUE &&
	    gsc_dst_get_buf_seq(ctx) >= GSC_BUF_START)
		gsc_handle_irq(ctx, true, false, true);

	return 0;
}

static int gsc_dst_set_addr(struct device *dev,
			struct drm_exynos_ipp_buf_info *buf_info, u32 buf_id,
			enum drm_exynos_ipp_buf_ctrl buf_ctrl)
{
	struct gsc_context *ctx = get_gsc_context(dev);
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	struct drm_exynos_ipp_property *property = ippdrv->property;
	struct drm_exynos_ipp_config *config =
		&property->config[EXYNOS_DRM_OPS_DST];
	int ret;

	DRM_DEBUG_KMS("%s:buf_id[%d]buf_ctrl[%d]\n", __func__,
		buf_id, buf_ctrl);

	if (buf_id > GSC_MAX_DST) {
		dev_info(ippdrv->dev, "inavlid buf_id %d.\n", buf_id);
		return -ENOMEM;
	}

	/* address register set */
	switch (buf_ctrl) {
	case IPP_BUF_CTRL_QUEUE:
	case IPP_BUF_CTRL_DEQUEUE:
		ret = gsc_set_planar_addr(buf_info, config->fmt, &config->sz);

		if (ret) {
			dev_err(dev, "failed to set plane addr.\n");
			return ret;
		}

		gsc_write(buf_info->base[EXYNOS_DRM_PLANAR_Y],
			GSC_OUT_BASE_ADDR_Y(buf_id));
		gsc_write(buf_info->base[EXYNOS_DRM_PLANAR_CB],
			GSC_OUT_BASE_ADDR_CB(buf_id));
		gsc_write(buf_info->base[EXYNOS_DRM_PLANAR_CR],
			GSC_OUT_BASE_ADDR_CR(buf_id));
		break;
	default:
		/* bypass */
		break;
	}

	return gsc_dst_set_buf_seq(ctx, buf_id, buf_ctrl);
}

static struct exynos_drm_ipp_ops gsc_dst_ops = {
	.set_fmt = gsc_dst_set_fmt,
	.set_transf = gsc_dst_set_transf,
	.set_size = gsc_dst_set_size,
	.set_addr = gsc_dst_set_addr,
};

static int gsc_power_on(struct gsc_context *ctx, bool enable)
{
	DRM_DEBUG_KMS("%s:\n", __func__);

	if (enable) {
		clk_enable(ctx->gsc_clk);
		/* ToDo : wb_b_clk */
		ctx->suspended = false;
	} else {
		clk_disable(ctx->gsc_clk);
		/* ToDo : wb_b_clk */
		ctx->suspended = true;
	}

	return 0;
}

static irqreturn_t gsc_irq_handler(int irq, void *dev_id)
{
	struct gsc_context *ctx = dev_id;
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	u32 cfg, status;
	int buf_id = 0;

	DRM_DEBUG_KMS("%s:gsc id[%d]\n", __func__, ctx->id);

	status = gsc_read(GSC_IRQ);
	if (status & GSC_IRQ_STATUS_OR_IRQ) {
		dev_err(ippdrv->dev, "occured overflow at %d, status 0x%x.\n",
			ctx->id, status);
		return IRQ_NONE;
	}

	if (status & GSC_IRQ_STATUS_OR_FRM_DONE) {
		dev_err(ippdrv->dev, "occured frame done at %d, status 0x%x.\n",
			ctx->id, status);
		/* ToDo: Frame control */
	}

	if (list_empty(&ippdrv->event_list)) {
		DRM_DEBUG_KMS("%s:event list empty.\n", __func__);

		return IRQ_HANDLED;
	}

	cfg = gsc_read(GSC_IN_BASE_ADDR_Y_MASK);
	buf_id = GSC_IN_CURR_GET_INDEX(cfg);
	if (buf_id < 0)
		return IRQ_HANDLED;

	DRM_DEBUG_KMS("%s:buf_id[%d]\n", __func__, buf_id);

	if (gsc_dst_set_buf_seq(ctx, buf_id,
		IPP_BUF_CTRL_DEQUEUE) < 0) {
		DRM_ERROR("failed to dequeue.\n");

		return IRQ_HANDLED;
	}

	ipp_send_event_handler(ippdrv, buf_id);

	if (gsc_dst_get_buf_seq(ctx) <= GSC_BUF_STOP)
		gsc_handle_irq(ctx, false, false, true);

	return IRQ_HANDLED;
}

static int gsc_ippdrv_check_property(struct device *dev,
				struct drm_exynos_ipp_property *property)
{
	/* ToDo: check valid using property information */
	DRM_DEBUG_KMS("%s\n", __func__);

	return 0;
}

static int gsc_ippdrv_reset(struct device *dev)
{
	struct gsc_context *ctx = get_gsc_context(dev);
	int ret;

	DRM_DEBUG_KMS("%s\n", __func__);

	/* reset h/w block */
	ret = gsc_sw_reset(ctx);
	if (ret < 0) {
		dev_err(dev, "failed to reset hardware.\n");
		return ret;
	}

	memset(&ctx->sc, 0x0, sizeof(ctx->sc));

	return 0;
}

static int gsc_check_prepare(struct gsc_context *ctx)
{
	/* ToDo: check prepare using read register */
	DRM_DEBUG_KMS("%s\n", __func__);

	return 0;
}

static int gsc_ippdrv_start(struct device *dev, enum drm_exynos_ipp_cmd cmd)
{
	struct gsc_context *ctx = get_gsc_context(dev);
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	struct drm_exynos_ipp_property *property = ippdrv->property;
	struct drm_exynos_ipp_config *config =
		&property->config[EXYNOS_DRM_OPS_DST];
	u32 cfg;
	int ret;
	int enable = 1;

	DRM_DEBUG_KMS("%s:cmd[%d]\n", __func__, cmd);

	ret = gsc_check_prepare(ctx);
	if (ret) {
		dev_err(dev, "failed to check prepare.\n");
		return ret;
	}

	ippdrv->cmd = cmd;
	gsc_handle_irq(ctx, true, false, true);

	switch (cmd) {
	case IPP_CMD_M2M:
		/* bypass */
		break;
	case IPP_CMD_WB:
		gsc_set_gscblk_fimd_wb(ctx, enable);
		exynos_drm_ippnb_send_event(IPP_SET_WRITEBACK, (void *)enable);
		break;
	case IPP_CMD_OUTPUT:
	default:
		ret = -EINVAL;
		dev_err(dev, "invalid operations.\n");
		return ret;
	}

	ret = gsc_set_prescaler(ctx, &ctx->sc, &config->pos,
		&config->pos);
	if (ret) {
		dev_err(dev, "failed to set precalser.\n");
		return ret;
	}

	gsc_set_scaler(ctx, &ctx->sc);

	cfg = gsc_read(GSC_ENABLE);
	cfg |= GSC_ENABLE_ON;
	gsc_write(cfg, GSC_ENABLE);

	return 0;
}

static void gsc_ippdrv_stop(struct device *dev, enum drm_exynos_ipp_cmd cmd)
{
	struct gsc_context *ctx = get_gsc_context(dev);
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;
	u32 cfg;
	int enable = 0;

	DRM_DEBUG_KMS("%s:cmd[%d]\n", __func__, cmd);

	switch (cmd) {
	case IPP_CMD_M2M:
		/* bypass */
		break;
	case IPP_CMD_WB:
		gsc_set_gscblk_fimd_wb(ctx, enable);
		exynos_drm_ippnb_send_event(IPP_SET_WRITEBACK, (void *)enable);
		break;
	case IPP_CMD_OUTPUT:
	default:
		dev_err(dev, "invalid operations.\n");
		break;
	}

	ippdrv->cmd = IPP_CMD_NONE;
	gsc_handle_irq(ctx, false, false, true);

	/* reset sequence */
	gsc_write(0xff, GSC_OUT_BASE_ADDR_Y_MASK);
	gsc_write(0xff, GSC_OUT_BASE_ADDR_CB_MASK);
	gsc_write(0xff, GSC_OUT_BASE_ADDR_CR_MASK);

	cfg = gsc_read(GSC_ENABLE);
	cfg &= ~GSC_ENABLE_ON;
	gsc_write(cfg, GSC_ENABLE);
}

#ifdef CONFIG_SLP_DISP_DEBUG
static int gsc_read_reg(struct gsc_context *ctx, char *buf)
{
	u32 cfg;
	int i;
	int pos = 0;

	pos += sprintf(buf+pos, "0x%.8x | ", GSC_BASE_REG(ctx->id));
	for (i = 1; i < GSC_MAX_REG + 1; i++) {
		cfg = gsc_read((i-1) * sizeof(u32));
		pos += sprintf(buf+pos, "0x%.8x ", cfg);
		if (i % 4 == 0)
			pos += sprintf(buf+pos, "\n0x%.8x | ",
				GSC_BASE_REG(ctx->id) + (i * sizeof(u32)));
	}

	pos += sprintf(buf+pos, "\n");

	return pos;
}

static ssize_t show_read_reg(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct gsc_context *ctx = get_gsc_context(dev);

	if (!ctx->regs) {
		dev_err(dev, "failed to get current register.\n");
		return -EINVAL;
	}

	return gsc_read_reg(ctx, buf);
}

static struct device_attribute device_attrs[] = {
	__ATTR(read_reg, S_IRUGO, show_read_reg, NULL),
};
#endif

static int __devinit gsc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct gsc_context *ctx;
	struct resource *res;
	struct exynos_drm_ippdrv *ippdrv;
	struct exynos_drm_gsc_pdata *pdata;
	int ret = -EINVAL;
#ifdef CONFIG_SLP_DISP_DEBUG
	int i;
#endif

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(dev, "no platform data specified.\n");
		return -EINVAL;
	}

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	/* clock control */
	ctx->gsc_clk = clk_get(dev, "gscl");
	if (IS_ERR(ctx->gsc_clk)) {
		dev_err(dev, "failed to get gsc clock.\n");
		ret = PTR_ERR(ctx->gsc_clk);
		goto err_ctx;
	}

	/* resource memory */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "failed to find registers.\n");
		ret = -ENOENT;
		goto err_clk;
	}

	ctx->regs_res = request_mem_region(res->start, resource_size(res),
					   dev_name(dev));
	if (!ctx->regs_res) {
		dev_err(dev, "failed to claim register region.\n");
		ret = -ENOENT;
		goto err_clk;
	}

	ctx->regs = ioremap(res->start, resource_size(res));
	if (!ctx->regs) {
		dev_err(dev, "failed to map registers.\n");
		ret = -ENXIO;
		goto err_req_region;
	}

	/* resource irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(dev, "failed to request irq resource.\n");
		goto err_get_regs;
	}

	ctx->irq = res->start;
	ret = request_threaded_irq(ctx->irq, NULL, gsc_irq_handler,
		IRQF_ONESHOT, "drm_gsc", ctx);
	if (ret < 0) {
		dev_err(dev, "failed to request irq.\n");
		goto err_get_regs;
	}

	/* context initailization */
	ctx->id = pdev->id;
	ctx->capa = gsc51_capa;
	if (!ctx->capa) {
		dev_err(dev, "failed to get capability.\n");
		goto err_get_irq;
	}

#ifdef CONFIG_SLP_DISP_DEBUG
	for (i = 0; i < ARRAY_SIZE(device_attrs); i++) {
		ret = device_create_file(&(pdev->dev),
					&device_attrs[i]);
		if (ret)
			break;
	}

	if (ret < 0)
		dev_err(&pdev->dev, "failed to add sysfs entries\n");
#endif

	DRM_DEBUG_KMS("%s:id[%d]\n", __func__, ctx->id);

	ippdrv = &ctx->ippdrv;
	ippdrv->dev = dev;
	ippdrv->ops[EXYNOS_DRM_OPS_SRC] = &gsc_src_ops;
	ippdrv->ops[EXYNOS_DRM_OPS_DST] = &gsc_dst_ops;
	ippdrv->check_property = gsc_ippdrv_check_property;
	ippdrv->reset = gsc_ippdrv_reset;
	ippdrv->start = gsc_ippdrv_start;
	ippdrv->stop = gsc_ippdrv_stop;

	mutex_init(&ctx->lock);
	platform_set_drvdata(pdev, ctx);

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	ret = exynos_drm_ippdrv_register(ippdrv);
	if (ret < 0) {
		dev_err(dev, "failed to register drm gsc device.\n");
		goto err_get_irq;
	}

	dev_info(&pdev->dev, "drm gsc registered successfully.\n");

	return 0;

err_get_irq:
	free_irq(ctx->irq, ctx);
err_get_regs:
	iounmap(ctx->regs);
err_req_region:
	release_resource(ctx->regs_res);
	kfree(ctx->regs_res);
err_clk:
	clk_put(ctx->gsc_clk);
err_ctx:
	kfree(ctx);
	return ret;
}

static int __devexit gsc_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct gsc_context *ctx = get_gsc_context(dev);
	struct exynos_drm_ippdrv *ippdrv = &ctx->ippdrv;

	exynos_drm_ippdrv_unregister(ippdrv);

	pm_runtime_set_suspended(dev);
	pm_runtime_disable(dev);

	free_irq(ctx->irq, ctx);
	iounmap(ctx->regs);
	release_resource(ctx->regs_res);
	kfree(ctx->regs_res);

	clk_put(ctx->gsc_clk);

	kfree(ctx);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int gsc_suspend(struct device *dev)
{
	struct gsc_context *ctx = get_gsc_context(dev);

	DRM_DEBUG_KMS("%s:id[%d]\n", __func__, ctx->id);
	if (pm_runtime_suspended(dev))
		return 0;
	/* ToDo */
	return gsc_power_on(ctx, false);
}

static int gsc_resume(struct device *dev)
{
	struct gsc_context *ctx = get_gsc_context(dev);

	DRM_DEBUG_KMS("%s:id[%d]\n", __func__, ctx->id);
	if (!pm_runtime_suspended(dev))
		return gsc_power_on(ctx, true);
	/* ToDo */
	return 0;
}
#endif

#ifdef CONFIG_PM_RUNTIME
static int gsc_runtime_suspend(struct device *dev)
{
	struct gsc_context *ctx = get_gsc_context(dev);

	DRM_DEBUG_KMS("%s:id[%d]\n", __func__, ctx->id);
	/* ToDo */
	return  gsc_power_on(ctx, false);
}

static int gsc_runtime_resume(struct device *dev)
{
	struct gsc_context *ctx = get_gsc_context(dev);

	DRM_DEBUG_KMS("%s:id[%d]\n", __FILE__, ctx->id);
	/* ToDo */
	return  gsc_power_on(ctx, true);
}
#endif

static const struct dev_pm_ops gsc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(gsc_suspend, gsc_resume)
	SET_RUNTIME_PM_OPS(gsc_runtime_suspend, gsc_runtime_resume, NULL)
};

/* ToDo: need to check use case platform_device_id */
struct platform_driver gsc_driver = {
	.probe		= gsc_probe,
	.remove		= __devexit_p(gsc_remove),
	.driver		= {
		.name	= "exynos-drm-gsc",
		.owner	= THIS_MODULE,
		.pm	= &gsc_pm_ops,
	},
};

