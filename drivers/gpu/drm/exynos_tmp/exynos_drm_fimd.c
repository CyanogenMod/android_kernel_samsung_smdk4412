/* exynos_drm_fimd.c
 *
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * Authors:
 *	Joonyoung Shim <jy0922.shim@samsung.com>
 *	Inki Dae <inki.dae@samsung.com>
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

#include <drm/exynos_drm.h>
#include <plat/regs-fb-v4.h>

#include <plat/fimd_lite_ext.h>

#include <mach/map.h>

#include "exynos_drm_drv.h"
#include "exynos_drm_fbdev.h"
#include "exynos_drm_crtc.h"

#ifdef CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ
#include <linux/devfreq/exynos4_display.h>
#endif

#ifdef CONFIG_MACH_SLP_NAPLES
#define VIDCON0_DSI_DISABLE			(0 << 30)
#define VIDCON0_DSI_ENABLE			(1 << 30)

/* I80/RGB Trigger control */
#define TRIGCON		(0x01A4)
/* I80 interface control 0 for main LDI */
#define I80IFCONA0	(0x01B0)
#define I80IFCONA1	(0x01B4)
#define I80IFCONB0	(0x01B8)
#define I80IFCONB1	(0x01B8)

/* I80IFCONA0 and I80IFCONA1 */
#define LCD_CS_SETUP(x)			(((x) & 0xf) << 16)
#define LCD_WR_SETUP(x)			(((x) & 0xf) << 12)
#define LCD_WR_ACT(x)			(((x) & 0xf) << 8)
#define LCD_WR_HOLD(x)			(((x) & 0xf) << 4)
#define RSPOL_LOW				(0 << 2)
#define RSPOL_HIGH				(1 << 2)
#define I80IFEN_DISABLE			(0 << 0)
#define I80IFEN_ENABLE			(1 << 0)

enum s3cfb_cpu_auto_cmd_rate {
	DISABLE_AUTO_FRM,
	PER_TWO_FRM,
	PER_FOUR_FRM,
	PER_SIX_FRM,
	PER_EIGHT_FRM,
	PER_TEN_FRM,
	PER_TWELVE_FRM,
	PER_FOURTEEN_FRM,
	PER_SIXTEEN_FRM,
	PER_EIGHTEEN_FRM,
	PER_TWENTY_FRM,
	PER_TWENTY_TWO_FRM,
	PER_TWENTY_FOUR_FRM,
	PER_TWENTY_SIX_FRM,
	PER_TWENTY_EIGHT_FRM,
	PER_THIRTY_FRM,
};
#endif
#ifdef CONFIG_DRM_EXYNOS_FIMD_WB
#include <plat/fimc.h>
#ifdef CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ
#include <plat/pd.h>
#include <linux/pm_qos_params.h>
#endif
#define FIMD_GET_LCD_WIDTH	_IOR('F', 302, int)
#define FIMD_GET_LCD_HEIGHT	_IOR('F', 303, int)
#define FIMD_SET_WRITEBACK	_IOW('F', 304, u32)
#endif

/*
 * FIMD is stand for Fully Interactive Mobile Display and
 * as a display controller, it transfers contents drawn on memory
 * to a LCD Panel through Display Interfaces such as RGB or
 * CPU Interface.
 */

/* position control register for hardware window 0, 2 ~ 4.*/
#define VIDOSD_A(win)		(VIDOSD_BASE + 0x00 + (win) * 16)
#define VIDOSD_B(win)		(VIDOSD_BASE + 0x04 + (win) * 16)
/* size control register for hardware window 0. */
#define VIDOSD_C_SIZE_W0	(VIDOSD_BASE + 0x08)
/* alpha control register for hardware window 1 ~ 4. */
#define VIDOSD_C(win)		(VIDOSD_BASE + 0x18 + (win) * 16)
/* size control register for hardware window 1 ~ 4. */
#define VIDOSD_D(win)		(VIDOSD_BASE + 0x0C + (win) * 16)

#define VIDWx_BUF_START(win, buf)	(VIDW_BUF_START(buf) + (win) * 8)
#define VIDWx_BUF_END(win, buf)		(VIDW_BUF_END(buf) + (win) * 8)
#define VIDWx_BUF_SIZE(win, buf)	(VIDW_BUF_SIZE(buf) + (win) * 4)

/* color key control register for hardware window 1 ~ 4. */
#define WKEYCON0_BASE(x)		((WKEYCON0 + 0x140) + (x * 8))
/* color key value register for hardware window 1 ~ 4. */
#define WKEYCON1_BASE(x)		((WKEYCON1 + 0x140) + (x * 8))

/* FIMD has totally five hardware windows. */
#define WINDOWS_NR		5

#define get_fimd_context(dev)	platform_get_drvdata(to_platform_device(dev))

static struct s5p_fimd_ext_device *fimd_lite_dev, *mdnie;
static struct s5p_fimd_dynamic_refresh *fimd_refresh;

struct fimd_notifier_block {
	struct list_head	list;
	void			*data;
	int (*client_notifier)(unsigned int val, void *data);
};

static LIST_HEAD(fimd_notifier_list);
static DEFINE_MUTEX(fimd_notifier_lock);

struct fimd_win_data {
	unsigned int		offset_x;
	unsigned int		offset_y;
	unsigned int		ovl_width;
	unsigned int		ovl_height;
	unsigned int		fb_width;
	unsigned int		fb_height;
	unsigned int		bpp;
	dma_addr_t		dma_addr;
	void __iomem		*vaddr;
	unsigned int		buf_offsize;
	unsigned int		line_size;	/* bytes */
	bool			enabled;
};

struct fimd_context {
	struct exynos_drm_subdrv	subdrv;
	int				irq;
	struct drm_crtc			*crtc;
	struct clk			*bus_clk;
	struct clk			*lcd_clk;
	struct resource			*regs_res;
	void __iomem			*regs;
	struct fimd_win_data		win_data[WINDOWS_NR];
	unsigned int			clkdiv;
	unsigned int			default_win;
	unsigned long			irq_flags;
	u32				vidcon0;
	u32				vidcon1;
	bool				suspended;
	struct mutex			lock;

	struct exynos_drm_panel_info *panel;
	unsigned int			high_freq;
	unsigned int			dynamic_refresh;
	struct notifier_block		nb_exynos_display;

	struct work_struct		work;
	bool				errata;
#ifdef CONFIG_DRM_EXYNOS_FIMD_WB
	struct notifier_block	nb_ctrl;
#ifdef CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ
	struct pm_qos_request_list	pm_qos;
#endif
#endif
};
#ifdef CONFIG_MACH_SLP_NAPLES
struct fimd_context *ctx_global;
#endif

static bool fimd_display_is_connected(struct device *dev)
{
	DRM_DEBUG_KMS("%s\n", __FILE__);

	/* TODO. */

	return true;
}

static void *fimd_get_panel(struct device *dev)
{
	struct fimd_context *ctx = get_fimd_context(dev);

	DRM_DEBUG_KMS("%s\n", __FILE__);

	return ctx->panel;
}

static int fimd_check_timing(struct device *dev, void *timing)
{
	DRM_DEBUG_KMS("%s\n", __FILE__);

	/* TODO. */

	return 0;
}

static int fimd_display_power_on(struct device *dev, int mode)
{
	DRM_DEBUG_KMS("%s\n", __FILE__);

	/* TODO */

	drm_bl_dpms(mode);

	return 0;
}

static struct exynos_drm_display_ops fimd_display_ops = {
	.type = EXYNOS_DISPLAY_TYPE_LCD,
	.is_connected = fimd_display_is_connected,
	.get_panel = fimd_get_panel,
	.check_timing = fimd_check_timing,
	.power_on = fimd_display_power_on,
};

static void exynos_drm_mdnie_mode_stop(struct fimd_context *ctx)
{
	struct s5p_fimd_ext_driver *fimd_lite_drv;
	u32 cfg;

	fimd_lite_drv = to_fimd_ext_driver(fimd_lite_dev->dev.driver);

	/* set dualrgb register to mDNIe mode. */
	cfg = readl(ctx->regs + DUALRGB);
	cfg &= ~(0x3 << 0);
	writel(cfg, ctx->regs + DUALRGB);
	msleep(20);

	/* change display path. */
	cfg = readl(S3C_VA_SYS + 0x210);
	cfg |= 1 << 1;
	writel(cfg, S3C_VA_SYS + 0x210);

	if (fimd_lite_drv->stop)
		fimd_lite_drv->stop(fimd_lite_dev);

	if (fimd_lite_drv->setup)
		fimd_lite_drv->setup(fimd_lite_dev, 0);

	/* clock off */
	if (fimd_lite_drv->power_off)
		fimd_lite_drv->power_off(fimd_lite_dev);

	fimd_lite_dev->enabled = false;
}

static void exynos_drm_set_mdnie_mode(struct fimd_context *ctx)
{
	u32 cfg;

	/* change display path. */
	cfg = readl(S3C_VA_SYS + 0x210);
	/* MIE_LBLK0 is mDNIe. */
	cfg |= 1 << 0;
	/* FIMDBYPASS_LBLK0 is MIE/mDNIe. */
	cfg &= ~(1 << 1);
	writel(cfg, S3C_VA_SYS + 0x210);

	/* all polarity values should be 0 for mDNIe. */
	cfg = readl(ctx->regs + VIDCON1);
	cfg &= ~(VIDCON1_INV_VCLK | VIDCON1_INV_HSYNC |
		VIDCON1_INV_VSYNC | VIDCON1_INV_VDEN |
		VIDCON1_VCLK_MASK);

	writel(cfg, ctx->regs + VIDCON1);

	/* set dualrgb register to mDNIe mode. */
	cfg = readl(ctx->regs + DUALRGB);
	cfg &= ~(0x3 << 0);
	cfg |= 0x3 << 0;
	writel(cfg, ctx->regs + DUALRGB);
}

static int exynos_drm_change_to_mdnie(struct fimd_context *ctx)
{
	u32 cfg;
	struct s5p_fimd_ext_driver *mdnie_drv, *fimd_lite_drv;

	mdnie_drv = to_fimd_ext_driver(mdnie->dev.driver);
	fimd_lite_drv = to_fimd_ext_driver(fimd_lite_dev->dev.driver);

	/**
	 * path change sequence for mDNIe.
	 *
	 * 1. FIMD-LITE DMA stop.
	 * 2. FIMD DMA stop.
	 * 3. change DISPLAY_CONTROL and DUALRGB registers to mDNIe mode.
	 * 4. change FIMD VCLKFREE to freerun mode.
	 * 5. initialize mDNIe module.
	 * 6. initialize FIMD-LITE module.
	 * 7. FIMD-LITE logic start.
	 * 8. FIMD-LITE DMA start.
	 * 9. FIMD DMA start.
	 *
	 * ps. FIMD polarity values should be 0.
	 *     lcd polarity values should be set to FIMD-LITE.
	 *     FIMD and FIMD-LITE DMA should be started at same time.
	 */
	/* set fimd to mDNIe mode.(WB/mDNIe) */
	exynos_drm_set_mdnie_mode(ctx);

	/* enable FIMD-LITE. clk */
	if (fimd_lite_drv && fimd_lite_drv->power_on)
		fimd_lite_drv->power_on(fimd_lite_dev);

	/* setup mDNIe. */
	if (mdnie_drv)
		mdnie_drv->setup(mdnie, 1);

	/* setup FIMD-LITE. */
	if (fimd_lite_drv)
		fimd_lite_drv->setup(fimd_lite_dev, 1);

	cfg = readl(ctx->regs + VIDCON0);
	cfg |= VIDCON0_ENVID | VIDCON0_ENVID_F;
	writel(cfg, ctx->regs + VIDCON0);

	if (fimd_lite_drv->start)
		fimd_lite_drv->start(fimd_lite_dev);
	return 0;
}

static void fimd_dpms(struct device *subdrv_dev, int mode)
{
	struct fimd_context *ctx = get_fimd_context(subdrv_dev);

	DRM_DEBUG_KMS("%s, %d\n", __FILE__, mode);

	mutex_lock(&ctx->lock);

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		/*
		 * enable fimd hardware only if suspended status.
		 *
		 * P.S. fimd_dpms function would be called at booting time so
		 * clk_enable could be called double time.
		 */
		if (ctx->suspended)
			pm_runtime_get_sync(subdrv_dev);
		break;
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		if (!ctx->suspended)
			pm_runtime_put_sync(subdrv_dev);
		break;
	default:
		DRM_DEBUG_KMS("unspecified mode %d\n", mode);
		break;
	}

	mutex_unlock(&ctx->lock);
}

static void fimd_apply(struct device *subdrv_dev)
{
	struct fimd_context *ctx = get_fimd_context(subdrv_dev);
	struct exynos_drm_manager *mgr = ctx->subdrv.manager;
	struct exynos_drm_manager_ops *mgr_ops = mgr->ops;
	struct exynos_drm_overlay_ops *ovl_ops = mgr->overlay_ops;
	struct fimd_win_data *win_data;
	int i;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	for (i = 0; i < WINDOWS_NR; i++) {
		win_data = &ctx->win_data[i];
		if (win_data->enabled && (ovl_ops && ovl_ops->commit))
			ovl_ops->commit(subdrv_dev, i);
	}

	if (mgr_ops && mgr_ops->commit)
		mgr_ops->commit(subdrv_dev);
}

#ifdef CONFIG_MACH_SLP_NAPLES
void _fimd_set_auto_cmd_rate(struct fimd_context *ctx, unsigned char cmd_rate)
{
	unsigned int cmd_rate_val;
	unsigned int i80_if_con_reg_val;

	cmd_rate_val = (cmd_rate == DISABLE_AUTO_FRM) ? (0x0 << 0) :
		(cmd_rate == PER_TWO_FRM) ? (0x1 << 0) :
		(cmd_rate == PER_FOUR_FRM) ? (0x2 << 0) :
		(cmd_rate == PER_SIX_FRM) ? (0x3 << 0) :
		(cmd_rate == PER_EIGHT_FRM) ? (0x4 << 0) :
		(cmd_rate == PER_TEN_FRM) ? (0x5 << 0) :
		(cmd_rate == PER_TWELVE_FRM) ? (0x6 << 0) :
		(cmd_rate == PER_FOURTEEN_FRM) ? (0x7 << 0) :
		(cmd_rate == PER_SIXTEEN_FRM) ? (0x8 << 0) :
		(cmd_rate == PER_EIGHTEEN_FRM) ? (0x9 << 0) :
		(cmd_rate == PER_TWENTY_FRM) ? (0xa << 0) :
		(cmd_rate == PER_TWENTY_TWO_FRM) ? (0xb << 0) :
		(cmd_rate == PER_TWENTY_FOUR_FRM) ? (0xc << 0) :
		(cmd_rate == PER_TWENTY_SIX_FRM) ? (0xd << 0) :
		(cmd_rate == PER_TWENTY_EIGHT_FRM) ? (0xe << 0) : (0xf << 0);

	i80_if_con_reg_val = readl(ctx->regs + I80IFCONB0);
	i80_if_con_reg_val &= ~(0xf << 0);
	i80_if_con_reg_val |= cmd_rate_val;
	writel(i80_if_con_reg_val, ctx->regs + I80IFCONB0);
}
#endif
static void fimd_commit(struct device *dev)
{
	struct fimd_context *ctx = get_fimd_context(dev);
	struct exynos_drm_panel_info *panel = ctx->panel;
	struct fb_videomode *timing = &panel->timing;
	u32 val;

	if (ctx->suspended)
		return;

	DRM_DEBUG_KMS("%s\n", __FILE__);

#ifdef CONFIG_MACH_SLP_NAPLES
		/* setup horizontal and vertical display size. */
		val = VIDTCON2_LINEVAL(timing->yres - 1) |
		       VIDTCON2_HOZVAL(timing->xres - 1);
		writel(val, ctx->regs + VIDTCON2);

		/* cpu_interface_timing */
		val = readl(ctx->regs + I80IFCONA0);
	val = LCD_CS_SETUP(0) |
		LCD_WR_SETUP(0) |
		LCD_WR_ACT(1) |
		LCD_WR_HOLD(0) |
			RSPOL_LOW | /* in case of LCD MIPI module */
			I80IFEN_ENABLE;
		writel(val, ctx->regs + I80IFCONA0);

		_fimd_set_auto_cmd_rate(ctx, DISABLE_AUTO_FRM);

		/* setup clock source, clock divider, enable dma. */
		val = ctx->vidcon0;
		val &= ~(VIDCON0_CLKVAL_F_MASK | VIDCON0_CLKDIR);

		if (ctx->clkdiv > 1)
			val |= VIDCON0_CLKVAL_F(ctx->clkdiv - 1)
					| VIDCON0_CLKDIR;
		else
			val &= ~VIDCON0_CLKDIR;	/* 1:1 clock */

		/*
		 * fields of register with prefix '_F' would be updated
		 * at vsync(same as dma start)
		 */
		val |= VIDCON0_ENVID | VIDCON0_ENVID_F;
		writel(val, ctx->regs + VIDCON0);

		/*
		 * Workaround: After power domain is turned off
		 * then when it is turned on, this needs.
		 */
		val &= ~(VIDCON0_ENVID | VIDCON0_ENVID_F);
		writel(val, ctx->regs + VIDCON0);

		val |= VIDCON0_ENVID | VIDCON0_ENVID_F;
		writel(val, ctx->regs + VIDCON0);
#else
	/* setup polarity values from machine code. */
	writel(ctx->vidcon1, ctx->regs + VIDCON1);

	/* setup vertical timing values. */
	val = VIDTCON0_VBPD(timing->upper_margin - 1) |
		VIDTCON0_VFPD(timing->lower_margin - 1) |
		VIDTCON0_VSPW(timing->vsync_len - 1);
	writel(val, ctx->regs + VIDTCON0);

	/* setup horizontal timing values.  */
	val = VIDTCON1_HBPD(timing->left_margin - 1) |
		VIDTCON1_HFPD(timing->right_margin - 1) |
		VIDTCON1_HSPW(timing->hsync_len - 1);
	writel(val, ctx->regs + VIDTCON1);

	/* setup horizontal and vertical display size. */
	val = VIDTCON2_LINEVAL(timing->yres - 1) |
	       VIDTCON2_HOZVAL(timing->xres - 1);
	writel(val, ctx->regs + VIDTCON2);

	/* setup clock source, clock divider, enable dma. */
	val = ctx->vidcon0;
	val &= ~(VIDCON0_CLKVAL_F_MASK | VIDCON0_CLKDIR);

	if (ctx->clkdiv > 1)
		val |= VIDCON0_CLKVAL_F(ctx->clkdiv - 1) | VIDCON0_CLKDIR;
	else
		val &= ~VIDCON0_CLKDIR;	/* 1:1 clock */

	/*
	 * fields of register with prefix '_F' would be updated
	 * at vsync(same as dma start)
	 */
	val |= VIDCON0_ENVID | VIDCON0_ENVID_F;
	writel(val, ctx->regs + VIDCON0);

	/*
	 * fix fimd errata with mDNIe.
	 *
	 * this code fixes a issue that mDNIe unfunctions properly
	 * when fimd power off goes to on. this issue is because dma
	 * is enabled two times with setcrtc call once a process is
	 * ternimated(at this thime, fimd goes to on from off for back
	 * to console fb) so this condition would avoid the situation.
	 */
	if (!ctx->errata) {
		/*
		* Workaround: After power domain is turned off then
		* when it is turned on, this needs.
		 */
		val &= ~(VIDCON0_ENVID | VIDCON0_ENVID_F);
		writel(val, ctx->regs + VIDCON0);

		val |= VIDCON0_ENVID | VIDCON0_ENVID_F;
		writel(val, ctx->regs + VIDCON0);

		ctx->errata = true;
	}
#endif
}

static int fimd_enable_vblank(struct device *dev)
{
	struct fimd_context *ctx = get_fimd_context(dev);
	u32 val;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	if (ctx->suspended)
		return -EPERM;

	if (!test_and_set_bit(0, &ctx->irq_flags)) {
		val = readl(ctx->regs + VIDINTCON0);

		val |= VIDINTCON0_INT_ENABLE;
		val |= VIDINTCON0_INT_FRAME;

		val &= ~VIDINTCON0_FRAMESEL0_MASK;
		val |= VIDINTCON0_FRAMESEL0_VSYNC;
		val &= ~VIDINTCON0_FRAMESEL1_MASK;
		val |= VIDINTCON0_FRAMESEL1_NONE;

		writel(val, ctx->regs + VIDINTCON0);
	}

	return 0;
}

static void fimd_disable_vblank(struct device *dev)
{
	struct fimd_context *ctx = get_fimd_context(dev);
	u32 val;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	if (ctx->suspended)
		return;

	if (test_and_clear_bit(0, &ctx->irq_flags)) {
		val = readl(ctx->regs + VIDINTCON0);

		val &= ~VIDINTCON0_INT_FRAME;
		val &= ~VIDINTCON0_INT_ENABLE;

		writel(val, ctx->regs + VIDINTCON0);
	}
}

static struct exynos_drm_manager_ops fimd_manager_ops = {
	.dpms = fimd_dpms,
	.apply = fimd_apply,
	.commit = fimd_commit,
	.enable_vblank = fimd_enable_vblank,
	.disable_vblank = fimd_disable_vblank,
};

static void fimd_win_mode_set(struct device *dev,
			      struct exynos_drm_overlay *overlay)
{
	struct fimd_context *ctx = get_fimd_context(dev);
	struct fimd_win_data *win_data;
	int win;
	unsigned long offset;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	if (!overlay) {
		dev_err(dev, "overlay is NULL\n");
		return;
	}

	win = overlay->zpos;
	if (win == DEFAULT_ZPOS)
		win = ctx->default_win;

	if (win < 0 || win > WINDOWS_NR)
		return;

	offset = overlay->fb_x * (overlay->bpp >> 3);
	offset += overlay->fb_y * overlay->pitch;

	DRM_DEBUG_KMS("offset = 0x%lx, pitch = %x\n", offset, overlay->pitch);

	win_data = &ctx->win_data[win];

	win_data->offset_x = overlay->crtc_x;
	win_data->offset_y = overlay->crtc_y;
	win_data->ovl_width = overlay->crtc_width;
	win_data->ovl_height = overlay->crtc_height;
	win_data->fb_width = overlay->fb_width;
	win_data->fb_height = overlay->fb_height;
	win_data->dma_addr = overlay->dma_addr[0] + offset;
	win_data->vaddr = overlay->vaddr[0] + offset;
	win_data->bpp = overlay->bpp;
	win_data->buf_offsize = (overlay->fb_width - overlay->crtc_width) *
				(overlay->bpp >> 3);
	win_data->line_size = overlay->crtc_width * (overlay->bpp >> 3);

	DRM_DEBUG_KMS("offset_x = %d, offset_y = %d\n",
			win_data->offset_x, win_data->offset_y);
	DRM_DEBUG_KMS("ovl_width = %d, ovl_height = %d\n",
			win_data->ovl_width, win_data->ovl_height);
	DRM_DEBUG_KMS("paddr = 0x%lx, vaddr = 0x%lx\n",
			(unsigned long)win_data->dma_addr,
			(unsigned long)win_data->vaddr);
	DRM_DEBUG_KMS("fb_width = %d, crtc_width = %d\n",
			overlay->fb_width, overlay->crtc_width);
}

static void fimd_win_set_pixfmt(struct device *dev, unsigned int win)
{
	struct fimd_context *ctx = get_fimd_context(dev);
	struct fimd_win_data *win_data = &ctx->win_data[win];
	unsigned long val;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	val = WINCONx_ENWIN;

	switch (win_data->bpp) {
	case 1:
		val |= WINCON0_BPPMODE_1BPP;
		val |= WINCONx_BITSWP;
		val |= WINCONx_BURSTLEN_4WORD;
		break;
	case 2:
		val |= WINCON0_BPPMODE_2BPP;
		val |= WINCONx_BITSWP;
		val |= WINCONx_BURSTLEN_8WORD;
		break;
	case 4:
		val |= WINCON0_BPPMODE_4BPP;
		val |= WINCONx_BITSWP;
		val |= WINCONx_BURSTLEN_8WORD;
		break;
	case 8:
		val |= WINCON0_BPPMODE_8BPP_PALETTE;
		val |= WINCONx_BURSTLEN_8WORD;
		val |= WINCONx_BYTSWP;
		break;
	case 16:
		val |= WINCON0_BPPMODE_16BPP_565;
		val |= WINCONx_HAWSWP;
		val |= WINCONx_BURSTLEN_16WORD;
		break;
	case 24:
		val |= WINCON0_BPPMODE_24BPP_888;
		val |= WINCONx_WSWP;
		val |= WINCONx_BURSTLEN_16WORD;
		break;
	case 32:
		val |= WINCON1_BPPMODE_28BPP_A4888
			| WINCON1_BLD_PIX | WINCON1_ALPHA_SEL;
		val |= WINCONx_WSWP;
		val |= WINCONx_BURSTLEN_16WORD;
		break;
	default:
		DRM_DEBUG_KMS("invalid pixel size so using unpacked 24bpp.\n");

		val |= WINCON0_BPPMODE_24BPP_888;
		val |= WINCONx_WSWP;
		val |= WINCONx_BURSTLEN_16WORD;
		break;
	}

	DRM_DEBUG_KMS("bpp = %d\n", win_data->bpp);

	writel(val, ctx->regs + WINCON(win));
}

static void fimd_win_set_colkey(struct device *dev, unsigned int win)
{
	struct fimd_context *ctx = get_fimd_context(dev);
	unsigned int keycon0 = 0, keycon1 = 0;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	keycon0 = ~(WxKEYCON0_KEYBL_EN | WxKEYCON0_KEYEN_F |
			WxKEYCON0_DIRCON) | WxKEYCON0_COMPKEY(0);

	keycon1 = WxKEYCON1_COLVAL(0xffffffff);

	writel(keycon0, ctx->regs + WKEYCON0_BASE(win));
	writel(keycon1, ctx->regs + WKEYCON1_BASE(win));
}

static void fimd_win_commit(struct device *dev, int zpos)
{
	struct fimd_context *ctx = get_fimd_context(dev);
	struct fimd_win_data *win_data;
	int win = zpos;
	unsigned long val, alpha, size;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	if (ctx->suspended)
		return;

	if (win == DEFAULT_ZPOS)
		win = ctx->default_win;

	if (win < 0 || win > WINDOWS_NR)
		return;

	win_data = &ctx->win_data[win];

	/*
	 * SHADOWCON register is used for enabling timing.
	 *
	 * for example, once only width value of a register is set,
	 * if the dma is started then fimd hardware could malfunction so
	 * with protect window setting, the register fields with prefix '_F'
	 * wouldn't be updated at vsync also but updated once unprotect window
	 * is set.
	 */

	/* protect windows */
	val = readl(ctx->regs + SHADOWCON);
	val |= SHADOWCON_WINx_PROTECT(win);
	writel(val, ctx->regs + SHADOWCON);

	/* buffer start address */
	val = (unsigned long)win_data->dma_addr;
	writel(val, ctx->regs + VIDWx_BUF_START(win, 0));

	/* buffer end address */
	size = win_data->fb_width * win_data->ovl_height * (win_data->bpp >> 3);
	val = (unsigned long)(win_data->dma_addr + size);
	writel(val, ctx->regs + VIDWx_BUF_END(win, 0));

	DRM_DEBUG_KMS("start addr = 0x%lx, end addr = 0x%lx, size = 0x%lx\n",
			(unsigned long)win_data->dma_addr, val, size);
	DRM_DEBUG_KMS("ovl_width = %d, ovl_height = %d\n",
			win_data->ovl_width, win_data->ovl_height);

	/* buffer size */
	val = VIDW_BUF_SIZE_OFFSET(win_data->buf_offsize) |
		VIDW_BUF_SIZE_PAGEWIDTH(win_data->line_size);
	writel(val, ctx->regs + VIDWx_BUF_SIZE(win, 0));

	/* OSD position */
	val = VIDOSDxA_TOPLEFT_X(win_data->offset_x) |
		VIDOSDxA_TOPLEFT_Y(win_data->offset_y);
	writel(val, ctx->regs + VIDOSD_A(win));

	val = VIDOSDxB_BOTRIGHT_X(win_data->offset_x +
					win_data->ovl_width - 1) |
		VIDOSDxB_BOTRIGHT_Y(win_data->offset_y +
					win_data->ovl_height - 1);
	writel(val, ctx->regs + VIDOSD_B(win));

	DRM_DEBUG_KMS("osd pos: tx = %d, ty = %d, bx = %d, by = %d\n",
			win_data->offset_x, win_data->offset_y,
			win_data->offset_x + win_data->ovl_width - 1,
			win_data->offset_y + win_data->ovl_height - 1);

	/* hardware window 0 doesn't support alpha channel. */
	if (win != 0) {
		/* OSD alpha */
		alpha = VIDISD14C_ALPHA1_R(0xf) |
			VIDISD14C_ALPHA1_G(0xf) |
			VIDISD14C_ALPHA1_B(0xf);

		writel(alpha, ctx->regs + VIDOSD_C(win));
	}

	/* OSD size */
	if (win != 3 && win != 4) {
		u32 offset = VIDOSD_D(win);
		if (win == 0)
			offset = VIDOSD_C_SIZE_W0;
		val = win_data->ovl_width * win_data->ovl_height;
		writel(val, ctx->regs + offset);

		DRM_DEBUG_KMS("osd size = 0x%x\n", (unsigned int)val);
	}

	fimd_win_set_pixfmt(dev, win);

	/* hardware window 0 doesn't support color key. */
	if (win != 0)
		fimd_win_set_colkey(dev, win);

	/* wincon */
	val = readl(ctx->regs + WINCON(win));
	val |= WINCONx_ENWIN;
	writel(val, ctx->regs + WINCON(win));

	/* Enable DMA channel and unprotect windows */
	val = readl(ctx->regs + SHADOWCON);
	val |= SHADOWCON_CHx_ENABLE(win);
	val &= ~SHADOWCON_WINx_PROTECT(win);
	writel(val, ctx->regs + SHADOWCON);

	win_data->enabled = true;
}

static void fimd_win_disable(struct device *dev, int zpos)
{
	struct fimd_context *ctx = get_fimd_context(dev);
	struct fimd_win_data *win_data;
	int win = zpos;
	u32 val;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	if (win == DEFAULT_ZPOS)
		win = ctx->default_win;

	if (win < 0 || win > WINDOWS_NR)
		return;

	win_data = &ctx->win_data[win];

	/* protect windows */
	val = readl(ctx->regs + SHADOWCON);
	val |= SHADOWCON_WINx_PROTECT(win);
	writel(val, ctx->regs + SHADOWCON);

	/* wincon */
	val = readl(ctx->regs + WINCON(win));
	val &= ~WINCONx_ENWIN;
	writel(val, ctx->regs + WINCON(win));

	/* unprotect windows */
	val = readl(ctx->regs + SHADOWCON);
	val &= ~SHADOWCON_CHx_ENABLE(win);
	val &= ~SHADOWCON_WINx_PROTECT(win);
	writel(val, ctx->regs + SHADOWCON);

	win_data->enabled = false;
}

static struct exynos_drm_overlay_ops fimd_overlay_ops = {
	.mode_set = fimd_win_mode_set,
	.commit = fimd_win_commit,
	.disable = fimd_win_disable,
};

static struct exynos_drm_manager fimd_manager = {
	.pipe		= -1,
	.ops		= &fimd_manager_ops,
	.overlay_ops	= &fimd_overlay_ops,
	.display_ops	= &fimd_display_ops,
};

static void fimd_finish_pageflip(struct drm_device *drm_dev, int crtc)
{
	struct exynos_drm_private *dev_priv = drm_dev->dev_private;
	struct drm_pending_vblank_event *e, *t;
	struct timeval now;
	unsigned long flags;
	bool is_checked = false;

	spin_lock_irqsave(&drm_dev->event_lock, flags);

	list_for_each_entry_safe(e, t, &dev_priv->pageflip_event_list,
			base.link) {
		/* if event's pipe isn't same as crtc then ignore it. */
		if (crtc != e->pipe)
			continue;

		is_checked = true;

		do_gettimeofday(&now);
		e->event.sequence = 0;
		e->event.tv_sec = now.tv_sec;
		e->event.tv_usec = now.tv_usec;

		list_move_tail(&e->base.link, &e->base.file_priv->event_list);
		wake_up_interruptible(&e->base.file_priv->event_wait);
	}

	if (is_checked) {
		/*
		 * call drm_vblank_put only in case that drm_vblank_get was
		 * called.
		 */
		if (atomic_read(&drm_dev->vblank_refcount[crtc]) > 0)
			drm_vblank_put(drm_dev, crtc);

		/*
		 * don't off vblank if vblank_disable_allowed is 1,
		 * because vblank would be off by timer handler.
		 */
		if (!drm_dev->vblank_disable_allowed)
			drm_vblank_off(drm_dev, crtc);
	}

	spin_unlock_irqrestore(&drm_dev->event_lock, flags);
}

static void exynos_fimd_schedule_work(struct work_struct *work)
{
	struct fimd_context *ctx = container_of(work, struct fimd_context,
						work);
	u32 ret;

	/* Change mdnie mode after irq handler */
	if (mdnie && fimd_lite_dev) {
		if (!fimd_lite_dev->enabled) {
			while (1) {
				ret = (__raw_readl(ctx->regs + VIDCON1)) &
							VIDCON1_VSTATUS_MASK;
				if (ret == VIDCON1_VSTATUS_BACKPORCH) {
					exynos_drm_change_to_mdnie(ctx);
					fimd_lite_dev->enabled = true;
					break;
				}
			}
		}
	}

}

#ifdef CONFIG_MACH_SLP_NAPLES
void fimd_set_trigger(void)
{
	u32 reg = 0;
	struct exynos_drm_subdrv *subdrv = &ctx_global->subdrv;
	struct drm_device *drm_dev = subdrv->drm_dev;
	struct exynos_drm_manager *manager = subdrv->manager;
	u32 val;

	val = readl(ctx_global->regs + VIDINTCON1);

	if (val & VIDINTCON1_INT_FRAME)
		/* VSYNC interrupt */
		writel(VIDINTCON1_INT_FRAME, ctx_global->regs + VIDINTCON1);

	/* check the crtc is detached already from encoder */
	if (manager->pipe < 0)
		return;

	drm_handle_vblank(drm_dev, manager->pipe);
	fimd_finish_pageflip(drm_dev, manager->pipe);

	schedule_work(&ctx_global->work);

	reg = readl(ctx_global->regs + TRIGCON);

	reg |= 1 << 0 | 1 << 1;

	writel(reg, ctx_global->regs  + TRIGCON);
}
#endif
static irqreturn_t fimd_irq_handler(int irq, void *dev_id)
{
	struct fimd_context *ctx = (struct fimd_context *)dev_id;
	struct exynos_drm_subdrv *subdrv = &ctx->subdrv;
	struct drm_device *drm_dev = subdrv->drm_dev;
	struct exynos_drm_manager *manager = subdrv->manager;
	u32 val;

	val = readl(ctx->regs + VIDINTCON1);

	if (val & VIDINTCON1_INT_FRAME)
		/* VSYNC interrupt */
		writel(VIDINTCON1_INT_FRAME, ctx->regs + VIDINTCON1);

	/* check the crtc is detached already from encoder */
	if (manager->pipe < 0)
		goto out;

	drm_handle_vblank(drm_dev, manager->pipe);
	fimd_finish_pageflip(drm_dev, manager->pipe);

		schedule_work(&ctx->work);

out:
	return IRQ_HANDLED;
}

static int fimd_subdrv_probe(struct drm_device *drm_dev, struct device *dev)
{
	DRM_DEBUG_KMS("%s\n", __FILE__);

	/*
	 * enable drm irq mode.
	 * - with irq_enabled = 1, we can use the vblank feature.
	 *
	 * P.S. note that we wouldn't use drm irq handler but
	 *	just specific driver own one instead because
	 *	drm framework supports only one irq handler.
	 */
	drm_dev->irq_enabled = 1;

	/*
	 * with vblank_disable_allowed = 1, vblank interrupt will be disabled
	 * by drm timer once a current process gives up ownership of
	 * vblank event.(after drm_vblank_put function is called)
	 */
	drm_dev->vblank_disable_allowed = 1;

	return 0;
}

static void fimd_subdrv_remove(struct drm_device *drm_dev)
{
	DRM_DEBUG_KMS("%s\n", __FILE__);

	/* TODO. */
}

static int fimd_calc_clkdiv(struct fimd_context *ctx,
			    struct fb_videomode *timing)
{
	unsigned long clk = clk_get_rate(ctx->lcd_clk);
	u32 retrace;
	u32 clkdiv;
	u32 best_framerate = 0;
	u32 framerate;

	DRM_DEBUG_KMS("%s\n", __FILE__);

#ifdef CONFIG_MACH_SLP_NAPLES
	retrace = timing->xres * timing->yres * 2;
#else
	retrace = timing->left_margin + timing->hsync_len +
				timing->right_margin + timing->xres;
	retrace *= timing->upper_margin + timing->vsync_len +
				timing->lower_margin + timing->yres;
#endif

	/* default framerate is 60Hz */
	if (!timing->refresh)
		timing->refresh = 60;

	clk /= retrace;

	for (clkdiv = 1; clkdiv < 0x100; clkdiv++) {
		int tmp;

		/* get best framerate */
		framerate = clk / clkdiv;
		tmp = timing->refresh - framerate;
		if (tmp < 0) {
			best_framerate = framerate;
			continue;
		} else {
			if (!best_framerate)
				best_framerate = framerate;
			else if (tmp < (best_framerate - framerate))
				best_framerate = framerate;
			break;
		}
	}

	return clkdiv;
}

static void fimd_clear_win(struct fimd_context *ctx, int win)
{
	u32 val;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	writel(0, ctx->regs + WINCON(win));
	writel(0, ctx->regs + VIDOSD_A(win));
	writel(0, ctx->regs + VIDOSD_B(win));
	writel(0, ctx->regs + VIDOSD_C(win));

	if (win == 1 || win == 2)
		writel(0, ctx->regs + VIDOSD_D(win));

	val = readl(ctx->regs + SHADOWCON);
	val &= ~SHADOWCON_WINx_PROTECT(win);
	writel(val, ctx->regs + SHADOWCON);
}

int fimd_register_client(int (*client_notifier)(unsigned int val, void *data),
				void *data)
{
	struct fimd_notifier_block *fimd_block;

	fimd_block = kzalloc(sizeof(*fimd_block), GFP_KERNEL);
	if (!fimd_block) {
		printk(KERN_ERR "failed to allocate fimd_notifier_block\n");
		return -ENOMEM;
	}

	fimd_block->client_notifier = client_notifier;
	fimd_block->data = data;

	mutex_lock(&fimd_notifier_lock);
	list_add_tail(&fimd_block->list, &fimd_notifier_list);
	mutex_unlock(&fimd_notifier_lock);

	return 0;
}
EXPORT_SYMBOL(fimd_register_client);

void fimd_unregister_client(int (*client_notifier)(unsigned int val,
							void *data))
{
	struct fimd_notifier_block *fimd_block;

	mutex_lock(&fimd_notifier_lock);
	list_for_each_entry(fimd_block, &fimd_notifier_list, list) {
		if (!fimd_block)
			continue;

		if (fimd_block->client_notifier == client_notifier) {
			list_del(&fimd_block->list);
			kfree(fimd_block);
			fimd_block = NULL;
			break;
		}
	}
	mutex_unlock(&fimd_notifier_lock);
}
EXPORT_SYMBOL(fimd_unregister_client);

static int fimd_notifier_call_chain(void)
{
	struct fimd_notifier_block *fimd_block;

	mutex_lock(&fimd_notifier_lock);
	list_for_each_entry(fimd_block, &fimd_notifier_list, list) {
		if (fimd_block && fimd_block->client_notifier)
			fimd_block->client_notifier(0, fimd_block->data);
	}
	mutex_unlock(&fimd_notifier_lock);

	return 0;
}

static int fimd_power_on(struct fimd_context *ctx, bool enable)
{
	struct exynos_drm_subdrv *subdrv = &ctx->subdrv;
	struct device *dev = subdrv->dev;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	if (enable != false && enable != true)
		return -EINVAL;

	if (enable) {
		int ret;

		/* fimd power should be off to clear mipi-dsi fifo. */
		fimd_notifier_call_chain();

		ret = clk_enable(ctx->bus_clk);
		if (ret < 0)
			return ret;

		ret = clk_enable(ctx->lcd_clk);
		if  (ret < 0) {
			clk_disable(ctx->bus_clk);
			return ret;
		}

		ctx->suspended = false;

		/* if vblank was enabled status, enable it again. */
		if (test_and_clear_bit(0, &ctx->irq_flags))
			fimd_enable_vblank(dev);

		fimd_apply(dev);
	} else {
		if (fimd_lite_dev)
			exynos_drm_mdnie_mode_stop(ctx);

		clk_disable(ctx->lcd_clk);
		clk_disable(ctx->bus_clk);

		ctx->suspended = true;
		ctx->errata = false;
	}

	return 0;
}

static void exynos_drm_change_clock(struct fimd_context *ctx)
{
	unsigned int cfg = 0;
	struct s5p_fimd_ext_driver *fimd_lite_drv;
	struct exynos_drm_panel_info *panel = ctx->panel;
	struct fb_videomode *timing = &panel->timing;

	fimd_lite_drv = to_fimd_ext_driver(fimd_lite_dev->dev.driver);

	if (!ctx->dynamic_refresh) {
		timing->refresh = 60;
		ctx->clkdiv = fimd_calc_clkdiv(ctx, timing);
#ifdef CONFIG_LCD_S6E8AA0
		/* workaround: To apply dynamic refresh rate */
		s6e8aa0_panel_cond(1);
#endif
		if (fimd_lite_dev && fimd_lite_dev->enabled) {
			fimd_refresh->clkdiv = ctx->clkdiv;
			fimd_lite_drv->change_clock(fimd_refresh,
						fimd_lite_dev);
		} else {
			cfg = readl(ctx->regs + VIDCON0);
			cfg &= ~VIDCON0_CLKVAL_F(0xFF);
			cfg |= VIDCON0_CLKVAL_F(ctx->clkdiv - 1);
			writel(cfg, ctx->regs + VIDCON0);
		}
	} else {
		ctx->clkdiv = fimd_calc_clkdiv(ctx, timing);
#ifdef CONFIG_LCD_S6E8AA0
		/* workaround: To apply dynamic refresh rate */
		s6e8aa0_panel_cond(ctx->high_freq);
#endif
		if (fimd_lite_dev && fimd_lite_dev->enabled) {
			fimd_refresh->clkdiv = ctx->clkdiv;
			fimd_lite_drv->change_clock(fimd_refresh,
						fimd_lite_dev);
		} else {
			cfg = readl(ctx->regs + VIDCON0);
			cfg &= ~VIDCON0_CLKVAL_F(0xFF);
			cfg |= VIDCON0_CLKVAL_F(ctx->clkdiv - 1);
			writel(cfg, ctx->regs + VIDCON0);
		}
	}
}

#ifdef CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ
static int exynos_display_notifier_callback(struct notifier_block *this,
			unsigned long event, void *_data)
{
	struct fimd_context *ctx
		= container_of(this, struct fimd_context, nb_exynos_display);
	struct exynos_drm_panel_info *panel = ctx->panel;
	struct fb_videomode *timing = &panel->timing;

	if (ctx->suspended)
		return NOTIFY_DONE;

	switch (event) {
	case EXYNOS4_DISPLAY_LV_HF:
		timing->refresh = EXYNOS4_DISPLAY_LV_HF;
		ctx->high_freq = 1;
		break;
	case EXYNOS4_DISPLAY_LV_LF:
		timing->refresh = EXYNOS4_DISPLAY_LV_LF;
		ctx->high_freq = 0;
		break;
	default:
		return NOTIFY_BAD;
	}

	exynos_drm_change_clock(ctx);

	return NOTIFY_DONE;
}
#endif

static ssize_t store_refresh(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct fimd_context *ctx = get_fimd_context(dev);
	struct exynos_drm_panel_info *panel = ctx->panel;
	struct fb_videomode *timing = &panel->timing;
	unsigned long refresh;
	int ret;

	if (ctx->dynamic_refresh) {
		ret = kstrtoul(buf, 0, &refresh);
		timing->refresh = refresh;
		if (refresh == 60)
			ctx->high_freq = 1;
		else
			ctx->high_freq = 0;

		exynos_drm_change_clock(ctx);
	}

	return count;
}

static ssize_t show_refresh(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct fimd_context *ctx = get_fimd_context(dev);
	struct exynos_drm_panel_info *panel = ctx->panel;
	struct fb_videomode *timing = &panel->timing;

	return snprintf(buf, PAGE_SIZE, "%d\n", timing->refresh);
}

static struct device_attribute device_attrs[] = {
	__ATTR(refresh, S_IRUGO|S_IWUSR, show_refresh, store_refresh),
};

#ifdef CONFIG_DRM_EXYNOS_FIMD_WB
static void fimd_set_writeback(struct fimd_context *ctx, int enable)
{
	u32 vidcon0 = readl(ctx->regs + VIDCON0);
	u32 vidcon2 = readl(ctx->regs + VIDCON2);

	vidcon0 &= ~VIDCON0_VIDOUT_MASK;
	vidcon2 &= ~(VIDCON2_WB_MASK |
			VIDCON2_WB_SKIP_MASK |
			VIDCON2_TVFORMATSEL_HW_SW_MASK |
			VIDCON2_TVFORMATSEL_MASK);

	if (enable) {
		vidcon0 |= VIDCON0_VIDOUT_WB;
		vidcon2 |= (VIDCON2_WB_ENABLE |
				VIDCON2_TVFORMATSEL_SW |
				VIDCON2_TVFORMATSEL_YUV444);
	} else {
		vidcon0 |= VIDCON0_VIDOUT_RGB;
		vidcon2 |= VIDCON2_WB_DISABLE;
	}

	writel(vidcon0, ctx->regs + VIDCON0);
	writel(vidcon2, ctx->regs + VIDCON2);
}

static int fimd_notifier_ctrl(struct notifier_block *this,
			unsigned long event, void *_data)
{
	struct fimd_context *ctx = container_of(this,
				struct fimd_context, nb_ctrl);

	switch (event) {
	case FIMD_GET_LCD_WIDTH: {
		struct exynos_drm_panel_info *panel = ctx->panel;
		struct fb_videomode *timing = &panel->timing;
		int *width = (int *)_data;

		*width = timing->xres;
	}
		break;
	case FIMD_GET_LCD_HEIGHT: {
		struct exynos_drm_panel_info *panel = ctx->panel;
		struct fb_videomode *timing = &panel->timing;
		int *height = (int *)_data;

		*height = timing->yres;
	}
		break;
	case FIMD_SET_WRITEBACK: {
		unsigned int refresh;
		int *enable = (int *)&_data;

#ifdef CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ
		if (*enable)
			refresh = EXYNOS4_DISPLAY_LV_HF;
		else
			refresh = EXYNOS4_DISPLAY_LV_LF;
		pm_qos_update_request(&ctx->pm_qos,
						refresh);
#endif
		fimd_set_writeback(ctx, *enable);
	}
		break;
	default:
		/* ToDo : for checking use case */
		DRM_INFO("%s:event[0x%x]\n", __func__, (unsigned int)event);
		break;
	}

	return NOTIFY_DONE;
}
#endif

static int __devinit fimd_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fimd_context *ctx;
	struct exynos_drm_subdrv *subdrv;
	struct exynos_drm_fimd_pdata *pdata;
	struct exynos_drm_panel_info *panel;
	struct resource *res;
	int win;
	int i;
	int ret = -EINVAL;

	DRM_DEBUG_KMS("%s\n", __FILE__);

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(dev, "no platform data specified\n");
		return -EINVAL;
	}

	panel = &pdata->panel;
	if (!panel) {
		dev_err(dev, "panel is null.\n");
		return -EINVAL;
	}

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->bus_clk = clk_get(dev, "lcd");
	if (IS_ERR(ctx->bus_clk)) {
		dev_err(dev, "failed to get bus clock\n");
		ret = PTR_ERR(ctx->bus_clk);
		goto err_clk_get;
	}

	clk_enable(ctx->bus_clk);

	ctx->lcd_clk = clk_get(dev, "sclk_fimd");
	if (IS_ERR(ctx->lcd_clk)) {
		dev_err(dev, "failed to get lcd clock\n");
		ret = PTR_ERR(ctx->lcd_clk);
		goto err_bus_clk;
	}

	clk_enable(ctx->lcd_clk);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "failed to find registers\n");
		ret = -ENOENT;
		goto err_clk;
	}

	ctx->regs_res = request_mem_region(res->start, resource_size(res),
					   dev_name(dev));
	if (!ctx->regs_res) {
		dev_err(dev, "failed to claim register region\n");
		ret = -ENOENT;
		goto err_clk;
	}

	ctx->regs = ioremap(res->start, resource_size(res));
	if (!ctx->regs) {
		dev_err(dev, "failed to map registers\n");
		ret = -ENXIO;
		goto err_req_region_io;
	}
#ifdef CONFIG_MACH_SLP_NAPLES
	/* Temporary code for trigger*/
	ctx_global = ctx;
#endif

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(dev, "irq request failed.\n");
		goto err_get_resource;
	}

	ctx->irq = res->start;

	ret = request_irq(ctx->irq, fimd_irq_handler, 0, "drm_fimd", ctx);
	if (ret < 0) {
		dev_err(dev, "irq request failed.\n");
		goto err_get_resource;
	}

	ctx->clkdiv = fimd_calc_clkdiv(ctx, &panel->timing);
	ctx->vidcon0 = pdata->vidcon0;
	ctx->vidcon1 = pdata->vidcon1;
	ctx->default_win = pdata->default_win;
	ctx->dynamic_refresh = pdata->dynamic_refresh;
	ctx->panel = panel;

	INIT_WORK(&ctx->work, exynos_fimd_schedule_work);

	panel->timing.pixclock = clk_get_rate(ctx->lcd_clk) / ctx->clkdiv;

	DRM_DEBUG_KMS("pixel clock = %d, clkdiv = %d\n",
			panel->timing.pixclock, ctx->clkdiv);

	/* mdnie support. */
	mdnie = s5p_fimd_ext_find_device("mdnie");
	fimd_lite_dev = s5p_fimd_ext_find_device("fimd_lite");
	if (mdnie && fimd_lite_dev) {
		fimd_refresh = kzalloc(sizeof(*fimd_refresh), GFP_KERNEL);
		if (!fimd_refresh) {
			dev_err(dev, "failed to allocate fimd_refresh.\n");
			ret = -ENOMEM;
			goto err_alloc_fail;
		}

		fimd_refresh->dynamic_refresh = pdata->dynamic_refresh;
		fimd_refresh->regs = ctx->regs;
		fimd_refresh->clkdiv = ctx->clkdiv;
	}

	for (i = 0; i < ARRAY_SIZE(device_attrs); i++) {
		ret = device_create_file(&(pdev->dev),
					&device_attrs[i]);
		if (ret)
			break;
	}

	if (ret < 0)
		dev_err(&pdev->dev, "failed to add sysfs entries\n");

#ifdef CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ
	ctx->nb_exynos_display.notifier_call = exynos_display_notifier_callback;
	ret = exynos4_display_register_client(&ctx->nb_exynos_display);
	if (ret < 0)
		dev_warn(dev, "failed to register exynos-display notifier\n");
#endif

	dev_info(&pdev->dev, "registered successfully\n");

	subdrv = &ctx->subdrv;

	subdrv->dev = dev;
	subdrv->manager = &fimd_manager;
	subdrv->probe = fimd_subdrv_probe;
	subdrv->remove = fimd_subdrv_remove;

#ifdef CONFIG_DRM_EXYNOS_FIMD_WB
	ctx->nb_ctrl.notifier_call = fimd_notifier_ctrl;
	ret = fimc_register_client(&ctx->nb_ctrl);
	if (ret) {
		dev_err(dev, "could not register fimd notify callback\n");
		goto err_alloc_fail;
	}
#ifdef CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ
	pm_qos_add_request(&ctx->pm_qos,
		PM_QOS_DISPLAY_FREQUENCY, EXYNOS4_DISPLAY_LV_LF);
#endif
#endif

	mutex_init(&ctx->lock);

	platform_set_drvdata(pdev, ctx);

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);

	for (win = 0; win < WINDOWS_NR; win++)
		if (win != ctx->default_win)
			fimd_clear_win(ctx, win);

	exynos_drm_subdrv_register(subdrv);

	return 0;

err_alloc_fail:
	free_irq(ctx->irq, ctx);

err_get_resource:
	iounmap(ctx->regs);

err_req_region_io:
	release_resource(ctx->regs_res);
	kfree(ctx->regs_res);

err_clk:
	clk_disable(ctx->lcd_clk);
	clk_put(ctx->lcd_clk);

err_bus_clk:
	clk_disable(ctx->bus_clk);
	clk_put(ctx->bus_clk);

err_clk_get:
	kfree(ctx);
	return ret;
}

static int __devexit fimd_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fimd_context *ctx = platform_get_drvdata(pdev);

	DRM_DEBUG_KMS("%s\n", __FILE__);

	exynos_drm_subdrv_unregister(&ctx->subdrv);
#ifdef CONFIG_DRM_EXYNOS_FIMD_WB
	fimc_unregister_client(&ctx->nb_ctrl);
#ifdef CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ
	pm_qos_remove_request(&ctx->pm_qos);
#endif
#endif

	if (ctx->suspended)
		goto out;

	pm_runtime_set_suspended(dev);
	pm_runtime_put_sync(dev);

out:
	pm_runtime_disable(dev);

	clk_put(ctx->lcd_clk);
	clk_put(ctx->bus_clk);


#ifdef CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ
	exynos4_display_unregister_client(&ctx->nb_exynos_display);
#endif

	iounmap(ctx->regs);
	release_resource(ctx->regs_res);
	kfree(ctx->regs_res);
	free_irq(ctx->irq, ctx);

	kfree(ctx);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int fimd_suspend(struct device *dev)
{
	struct fimd_context *ctx = get_fimd_context(dev);

	if (pm_runtime_suspended(dev))
		return 0;

	/*
	 * do not use pm_runtime_suspend(). if pm_runtime_suspend() is
	 * called here, an error would be returned by that interface
	 * because the usage_count of pm runtime is more than 1.
	 */
	return fimd_power_on(ctx, false);
}

static int fimd_resume(struct device *dev)
{
	struct fimd_context *ctx = get_fimd_context(dev);

	/*
	 * if entered to sleep when lcd panel was on, the usage_count
	 * of pm runtime would still be 1 so in this case, fimd driver
	 * should be on directly not drawing on pm runtime interface.
	 */
	if (!pm_runtime_suspended(dev))
		return fimd_power_on(ctx, true);

	return 0;
}
#endif

#ifdef CONFIG_PM_RUNTIME
static int fimd_runtime_suspend(struct device *dev)
{
	struct fimd_context *ctx = get_fimd_context(dev);

	DRM_DEBUG_KMS("%s\n", __FILE__);

	return fimd_power_on(ctx, false);
}

static int fimd_runtime_resume(struct device *dev)
{
	struct fimd_context *ctx = get_fimd_context(dev);

	DRM_DEBUG_KMS("%s\n", __FILE__);

	return fimd_power_on(ctx, true);
}
#endif

static const struct dev_pm_ops fimd_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(fimd_suspend, fimd_resume)
	SET_RUNTIME_PM_OPS(fimd_runtime_suspend, fimd_runtime_resume, NULL)
};

struct platform_driver fimd_driver = {
	.probe		= fimd_probe,
	.remove		= __devexit_p(fimd_remove),
	.driver		= {
		.name	= "s3cfb",
		.owner	= THIS_MODULE,
		.pm	= &fimd_pm_ops,
	},
};
