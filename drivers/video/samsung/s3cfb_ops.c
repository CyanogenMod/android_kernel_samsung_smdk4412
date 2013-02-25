/* linux/drivers/video/samsung/s3cfb-ops.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Middle layer file for Samsung Display Controller (FIMD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/poll.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include <linux/file.h>
#include <linux/fs.h>
#include <linux/sw_sync.h>
#include <plat/regs-fb.h>
#include <plat/regs-fb-s5p.h>

#if defined(CONFIG_CMA)
#include <linux/cma.h>
#elif defined(CONFIG_S5P_MEM_BOOTMEM)
#include <plat/media.h>
#include <mach/media.h>
#endif

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif

#include <mach/sec_debug.h>
#include <linux/bootmem.h>
#include "s3cfb.h"
#define NOT_DEFAULT_WINDOW 99
#define CMA_REGION_FIMD 	"fimd"
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
#define CMA_REGION_VIDEO	"video"
#else
#define CMA_REGION_VIDEO	"fimd"
#endif

#ifdef CONFIG_BUSFREQ_OPP
#include <mach/dev.h>
#endif

#ifdef CONFIG_FB_S5P_SYSMMU
#include <plat/s5p-sysmmu.h>
#endif

struct s3c_platform_fb *to_fb_plat(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	return (struct s3c_platform_fb *)pdev->dev.platform_data;
}

#ifndef CONFIG_FRAMEBUFFER_CONSOLE
#define LPDDR1_BASE_ADDR		0x50000000
#define BOOT_FB_BASE_ADDR		(LPDDR1_BASE_ADDR   + 0x0EC00000)	/* 0x5EC00000 from Bootloader */

static unsigned int bootloaderfb;
module_param_named(bootloaderfb, bootloaderfb, uint, 0444);
MODULE_PARM_DESC(bootloaderfb, "Address of booting logo image in Bootloader");

int s3cfb_draw_logo(struct fb_info *fb)
{
#ifdef CONFIG_FB_S5P_SPLASH_SCREEN
#ifdef RGB_BOOTSCREEN
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct fb_var_screeninfo *var = &fb->var;
#endif
#if 0
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	memcpy(fbdev->fb[pdata->default_win]->screen_base,
	       LOGO_RGB24, fix->line_length * var->yres);
#else
#ifdef RGB_BOOTSCREEN
	u32 height = var->yres / 3;
	u32 line = fix->line_length;
	u32 i, j;

	for (i = 0; i < height; i++) {
		for (j = 0; j < var->xres; j++) {
			memset(fb->screen_base + i * line + j * 4 + 0, 0x00, 1);
			memset(fb->screen_base + i * line + j * 4 + 1, 0x00, 1);
			memset(fb->screen_base + i * line + j * 4 + 2, 0xff, 1);
			memset(fb->screen_base + i * line + j * 4 + 3, 0x00, 1);
		}
	}

	for (i = height; i < height * 2; i++) {
		for (j = 0; j < var->xres; j++) {
			memset(fb->screen_base + i * line + j * 4 + 0, 0x00, 1);
			memset(fb->screen_base + i * line + j * 4 + 1, 0xff, 1);
			memset(fb->screen_base + i * line + j * 4 + 2, 0x00, 1);
			memset(fb->screen_base + i * line + j * 4 + 3, 0x00, 1);
		}
	}

	for (i = height * 2; i < height * 3; i++) {
		for (j = 0; j < var->xres; j++) {
			memset(fb->screen_base + i * line + j * 4 + 0, 0xff, 1);
			memset(fb->screen_base + i * line + j * 4 + 1, 0x00, 1);
			memset(fb->screen_base + i * line + j * 4 + 2, 0x00, 1);
			memset(fb->screen_base + i * line + j * 4 + 3, 0x00, 1);
		}
	}
#else /* #ifdef RGB_BOOTSCREEN */
	u8 *logo_virt_buf;

	if (bootloaderfb) {
		logo_virt_buf = phys_to_virt(bootloaderfb);
		memcpy(fb->screen_base, logo_virt_buf, fb->var.yres * fb->fix.line_length);
		printk(KERN_INFO "Bootloader sent 'bootloaderfb' : %08X\n", bootloaderfb);
	}

#endif /* #ifdef RGB_BOOTSCREEN */
#endif
#endif

	return 0;
}
#else
int fb_is_primary_device(struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);

	dev_dbg(fbdev->dev, "[fb%d] checking for primary device\n", win->id);

	if (win->id == pdata->default_win)
		return 1;
	else
		return 0;
}
#endif

#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)\
	|| defined(CONFIG_CPU_EXYNOS4210)
int window_on_off_status(struct s3cfb_global *fbdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct s3cfb_window *win;
	int i;
	int ret = 0;

	for (i = 0; i < pdata->nr_wins; i++) {
		win = fbdev->fb[i]->par;
		if (win->enabled)
			ret |= (1 << i);
	}

	return ret;
}
#endif

#ifdef FEATURE_BUSFREQ_LOCK
void s3cfb_busfreq_lock(struct s3cfb_global *fbdev, unsigned int lock)
{
	if (lock) {
		if (atomic_read(&fbdev->busfreq_lock_cnt) == 0) {
			exynos4_busfreq_lock(DVFS_LOCK_ID_LCD, BUS_L1);
			dev_info(fbdev->dev, "[%s] Bus Freq Locked L1\n", __func__);
		}
		atomic_inc(&fbdev->busfreq_lock_cnt);
		fbdev->busfreq_flag = true;
	} else {
		if (fbdev->busfreq_flag == true) {
			atomic_dec(&fbdev->busfreq_lock_cnt);
			fbdev->busfreq_flag = false;
			if (atomic_read(&fbdev->busfreq_lock_cnt) == 0) {
				/* release Freq lock back to normal */
				exynos4_busfreq_lock_free(DVFS_LOCK_ID_LCD);
				dev_info(fbdev->dev, "[%s] Bus Freq lock Released Normal !!\n", __func__);
			}
		}
	}
}
#endif

int s3cfb_enable_window(struct s3cfb_global *fbdev, int id)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;
#ifdef FEATURE_BUSFREQ_LOCK
	int enabled_win = 0;
#endif
#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
#ifdef CONFIG_BUSFREQ_OPP
	if (CONFIG_FB_S5P_DEFAULT_WINDOW == 3 &&
		id == CONFIG_FB_S5P_DEFAULT_WINDOW-1)
		dev_lock(fbdev->bus_dev, fbdev->dev, 267160);
	else if (id != CONFIG_FB_S5P_DEFAULT_WINDOW) {
		if (id == CONFIG_FB_S5P_DEFAULT_WINDOW-1)
			dev_lock(fbdev->bus_dev, fbdev->dev, 267160);
		else if (id == 3)
			dev_lock(fbdev->bus_dev, fbdev->dev, 267160);
		else
			dev_lock(fbdev->bus_dev, fbdev->dev, 133133);
	}
#endif
#endif

	if (!win->enabled)
		atomic_inc(&fbdev->enabled_win);

#ifdef FEATURE_BUSFREQ_LOCK
	enabled_win = atomic_read(&fbdev->enabled_win);
	if (enabled_win >= 2)
		s3cfb_busfreq_lock(fbdev, 1);
#endif

	if (s3cfb_window_on(fbdev, id)) {
		win->enabled = 0;
		return -EFAULT;
	} else {
		win->enabled = 1;
		return 0;
	}
}

int s3cfb_disable_window(struct s3cfb_global *fbdev, int id)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;
#ifdef FEATURE_BUSFREQ_LOCK
	int enabled_win = 0;
#endif
#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
#ifdef CONFIG_BUSFREQ_OPP
	int win_status;
#endif
#endif

	if (win->enabled)
		atomic_dec(&fbdev->enabled_win);

	if (fbdev->regs != 0 && s3cfb_window_off(fbdev, id)) {
		win->enabled = 1;
		return -EFAULT;
	} else {
#ifdef FEATURE_BUSFREQ_LOCK
		enabled_win = atomic_read(&fbdev->enabled_win);
		if (enabled_win < 2)
			s3cfb_busfreq_lock(fbdev, 0);
#endif
		win->enabled = 0;
#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
#ifdef CONFIG_BUSFREQ_OPP
		win_status = window_on_off_status(fbdev);
		if ((win_status & ~(1 << CONFIG_FB_S5P_DEFAULT_WINDOW)) == 0)
			dev_unlock(fbdev->bus_dev, fbdev->dev);
#endif
#endif
		return 0;
	}
}

int s3cfb_update_power_state(struct s3cfb_global *fbdev, int id, int state)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;
	win->power_state = state;

	return 0;
}

int s3cfb_init_global(struct s3cfb_global *fbdev)
{
	fbdev->output = OUTPUT_RGB;
	fbdev->rgb_mode = MODE_RGB_P;

	fbdev->wq_count = 0;
	init_waitqueue_head(&fbdev->wq);
	mutex_init(&fbdev->lock);

	s3cfb_set_output(fbdev);
	s3cfb_set_display_mode(fbdev);
	s3cfb_set_polarity(fbdev);
	s3cfb_set_timing(fbdev);
	s3cfb_set_lcd_size(fbdev);

	return 0;
}

int s3cfb_unmap_video_memory(struct s3cfb_global *fbdev, struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s3cfb_window *win = fb->par;

#ifdef CONFIG_CMA
	struct cma_info mem_info;
	int err;
#endif

#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
	return 0;
#endif

	if (fix->smem_start) {
#ifdef CONFIG_CMA
		err = cma_info(&mem_info, fbdev->dev, 0);
		if (ERR_PTR(err))
			return -ENOMEM;

		if (fix->smem_start >= mem_info.lower_bound &&
				fix->smem_start <= mem_info.upper_bound)
			cma_free(fix->smem_start);
#else
		dma_free_coherent(fbdev->dev, fix->smem_len, fb->screen_base, fix->smem_start);
#endif
		fix->smem_start = 0;
		fix->smem_len = 0;
		dev_info(fbdev->dev, "[fb%d] video memory released\n", win->id);
	}
	return 0;
}

int s3cfb_map_video_memory(struct s3cfb_global *fbdev, struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s3cfb_window *win = fb->par;
#ifdef CONFIG_CMA
	struct cma_info mem_info;
	int err;
#endif

#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
	return 0;
#endif

	if (win->owner == DMA_MEM_OTHER)
		return 0;

#ifdef CONFIG_CMA
	err = cma_info(&mem_info, fbdev->dev, CMA_REGION_VIDEO);
	if (err)
		return err;
	fix->smem_start = (dma_addr_t)cma_alloc
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
		(fbdev->dev, "fimd_video", (size_t)PAGE_ALIGN(fix->smem_len), 0);
#else
		(fbdev->dev, "fimd", (size_t)PAGE_ALIGN(fix->smem_len), 0);
#endif
	if (IS_ERR_OR_NULL((char *)fix->smem_start)) {
		printk(KERN_ERR "fix->smem_start allocation fail (%x)\n",
				(int)fix->smem_start);
		return -1;
	}

	fb->screen_base = NULL;
#else
	fb->screen_base = dma_alloc_writecombine(fbdev->dev,
						 PAGE_ALIGN(fix->smem_len),
						 (unsigned int *)
						 &fix->smem_start, GFP_KERNEL);
#endif

	dev_info(fbdev->dev, "[fb%d] Alloc dma: 0x%08x, "
			"size: 0x%08x\n", win->id,
			(unsigned int)fix->smem_start,
			fix->smem_len);

	win->owner = DMA_MEM_FIMD;

	return 0;
}

int s3cfb_map_default_video_memory(struct s3cfb_global *fbdev,
					struct fb_info *fb, int fimd_id)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s3cfb_window *win = fb->par;
#ifdef CONFIG_CMA
	struct cma_info mem_info;
	int err;
#endif

	if (win->owner == DMA_MEM_OTHER)
		return 0;

#ifdef CONFIG_CMA
	err = cma_info(&mem_info, fbdev->dev, CMA_REGION_FIMD);
	if (err)
		return err;
	fix->smem_start = (dma_addr_t)cma_alloc
		(fbdev->dev, "fimd", (size_t)PAGE_ALIGN(fix->smem_len), 0);
	fb->screen_base = cma_get_virt(fix->smem_start, PAGE_ALIGN(fix->smem_len), 1);
#elif defined(CONFIG_S5P_MEM_BOOTMEM)
	fix->smem_start = s5p_get_media_memory_bank(S5P_MDEV_FIMD, 1);
	fix->smem_len = s5p_get_media_memsize_bank(S5P_MDEV_FIMD, 1);
	fb->screen_base = ioremap_wc(fix->smem_start, fix->smem_len);
#else
	fb->screen_base = dma_alloc_writecombine(fbdev->dev,
						PAGE_ALIGN(fix->smem_len),
						(unsigned int *)
						&fix->smem_start, GFP_KERNEL);
#endif

	if (!fb->screen_base)
		return -ENOMEM;
	else
		dev_info(fbdev->dev, "[fb%d] dma: 0x%08x, cpu: 0x%08x, "
			"size: 0x%08x\n", win->id,
			(unsigned int)fix->smem_start,
			(unsigned int)fb->screen_base, fix->smem_len);

	if (bootloaderfb)
		memset(fb->screen_base, 0, fix->smem_len);
	win->owner = DMA_MEM_FIMD;

#ifdef CONFIG_FB_S5P_SYSMMU
	fbdev->sysmmu.default_fb_addr = fix->smem_start;
#endif

	return 0;
}

int s3cfb_unmap_default_video_memory(struct s3cfb_global *fbdev,
					struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s3cfb_window *win = fb->par;

	if (fix->smem_start) {
		iounmap(fb->screen_base);

		fix->smem_start = 0;
		fix->smem_len = 0;
		dev_info(fbdev->dev, "[fb%d] video memory released\n", win->id);
	}

	return 0;
}

int s3cfb_set_bitfield(struct fb_var_screeninfo *var)
{
	switch (var->bits_per_pixel) {
	case 16:
		if (var->transp.length == 1) {
			var->red.offset = 10;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 5;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 15;
		} else if (var->transp.length == 4) {
			var->red.offset = 8;
			var->red.length = 4;
			var->green.offset = 4;
			var->green.length = 4;
			var->blue.offset = 0;
			var->blue.length = 4;
			var->transp.offset = 12;
		} else {
			var->red.offset = 11;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 6;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 0;
		}
		break;

	case 24:
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;

	case 32:
#if defined(CONFIG_FB_RGBA_ORDER)
		var->red.offset = 0;
#else
		var->red.offset = 16;
#endif
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
#if defined(CONFIG_FB_RGBA_ORDER)
		var->blue.offset = 16;
#else
		var->blue.offset = 0;
#endif
		var->blue.length = 8;
		var->transp.offset = 24;
		var->transp.length = 8; /* added for LCD RGB32 */
		break;
	}

	return 0;
}

int s3cfb_set_alpha_info(struct fb_var_screeninfo *var,
				struct s3cfb_window *win)
{
	if (var->transp.length > 0)
		win->alpha.mode = PIXEL_BLENDING;
	else {
		win->alpha.mode = PLANE_BLENDING;
		win->alpha.channel = 0;
		win->alpha.value = S3CFB_AVALUE(0xf, 0xf, 0xf);
	}

	return 0;
}

int s3cfb_check_var_window(struct s3cfb_global *fbdev,
			struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct s3cfb_window *win = fb->par;
	struct s3cfb_lcd *lcd = fbdev->lcd;

	dev_dbg(fbdev->dev, "[fb%d] check_var\n", win->id);

	if (var->bits_per_pixel != 16 && var->bits_per_pixel != 24 &&
	    var->bits_per_pixel != 32) {
		dev_err(fbdev->dev, "invalid bits per pixel\n");
		return -EINVAL;
	}

	if (var->xres > lcd->width)
		var->xres = lcd->width;

	if (var->yres > lcd->height)
		var->yres = lcd->height;

	if (var->xres_virtual < var->xres)
		var->xres_virtual = var->xres;

#if defined(CONFIG_FB_S5P_VIRTUAL)
	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres * CONFIG_FB_S5P_NR_BUFFERS;
#endif

	if (var->xoffset > (var->xres_virtual - var->xres))
		var->xoffset = var->xres_virtual - var->xres;

	if (var->yoffset + var->yres > var->yres_virtual)
		var->yoffset = var->yres_virtual - var->yres;

	if (win->x + var->xres > lcd->width)
		win->x = lcd->width - var->xres;

	if (win->y + var->yres > lcd->height)
		win->y = lcd->height - var->yres;

	if (var->pixclock != fbdev->fb[pdata->default_win]->var.pixclock) {
		dev_info(fbdev->dev, "pixclk is changed from %d Hz to %d Hz\n",
			fbdev->fb[pdata->default_win]->var.pixclock, var->pixclock);
	}

	s3cfb_set_bitfield(var);
	s3cfb_set_alpha_info(var, win);

	return 0;
}

int s3cfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);

	s3cfb_check_var_window(fbdev, var, fb);

	return 0;
}

void s3cfb_set_win_params(struct s3cfb_global *fbdev, int id)
{
	s3cfb_set_window_control(fbdev, id);
#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
	s3cfb_set_oneshot(fbdev, id);
#else
	s3cfb_set_window_position(fbdev, id);
	s3cfb_set_window_size(fbdev, id);
	s3cfb_set_buffer_address(fbdev, id);
	s3cfb_set_buffer_size(fbdev, id);
#endif

	if (id > 0) {
		s3cfb_set_alpha_blending(fbdev, id);
		s3cfb_set_chroma_key(fbdev, id);
		s3cfb_set_alpha_value_width(fbdev, id);
		/* Set to premultiplied mode as default */
		s3cfb_set_alpha_mode(fbdev, id, BLENDING_PREMULT);
	}
}

int s3cfb_set_par_window(struct s3cfb_global *fbdev, struct fb_info *fb)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct s3cfb_window *win = fb->par;
	int ret;

	dev_dbg(fbdev->dev, "[fb%d] set_par\n", win->id);

#if (!defined(CONFIG_CPU_EXYNOS4210))
	if ((win->id != pdata->default_win) && fb->fix.smem_start)
		s3cfb_unmap_video_memory(fbdev, fb);
#endif

	/* modify the fix info */
	if (win->id != pdata->default_win) {
		fb->fix.line_length = fb->var.xres_virtual *
			fb->var.bits_per_pixel / 8;
		fb->fix.smem_len = fb->fix.line_length * fb->var.yres_virtual;
	}

	if (win->id != pdata->default_win && !fb->fix.smem_start) {
		ret = s3cfb_map_video_memory(fbdev, fb);
		if (ret != 0)
			return ret;
	}

	s3cfb_set_win_params(fbdev, win->id);

	return 0;
}

int s3cfb_set_par(struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	int ret = 0;

#ifdef CONFIG_EXYNOS_DEV_PD
	if (unlikely(fbdev->system_state == POWER_OFF)) {
		dev_err(fbdev->dev, "%s::system_state is POWER_OFF, fb%d\n", __func__, win->id);
		return 0;
	}
#endif

	ret = s3cfb_set_par_window(fbdev, fb);

	return ret;
}

int s3cfb_init_fbinfo(struct s3cfb_global *fbdev, int id)
{
	struct fb_info *fb = fbdev->fb[id];
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct fb_var_screeninfo *var = &fb->var;
	struct s3cfb_window *win = fb->par;
	struct s3cfb_alpha *alpha = &win->alpha;
	struct s3cfb_lcd *lcd = fbdev->lcd;
	struct s3cfb_lcd_timing *timing = &lcd->timing;

	memset(win, 0, sizeof(struct s3cfb_window));
	platform_set_drvdata(to_platform_device(fbdev->dev), fb);
	strcpy(fix->id, S3CFB_NAME);

	/* fimd specific */
	win->id = id;
	win->path = DATA_PATH_DMA;
	win->dma_burst = 16;
	s3cfb_update_power_state(fbdev, win->id, FB_BLANK_POWERDOWN);
	alpha->mode = PLANE_BLENDING;

	/* fbinfo */
	fb->fbops = &s3cfb_ops;
	fb->flags = FBINFO_FLAG_DEFAULT;
	fb->pseudo_palette = &win->pseudo_pal;
#if (CONFIG_FB_S5P_NR_BUFFERS != 1)
#if defined(CONFIG_CPU_EXYNOS4210)
	fix->xpanstep = 1; /*  xpanstep can be 1 if bits_per_pixel is 32 */
#else
	fix->xpanstep = 2;
#endif
	fix->ypanstep = 1;
#else
	fix->xpanstep = 0;
	fix->ypanstep = 0;
#endif
	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->accel = FB_ACCEL_NONE;
	fix->visual = FB_VISUAL_TRUECOLOR;
	var->xres = lcd->width;
	var->yres = lcd->height;

#if defined(CONFIG_FB_S5P_VIRTUAL)
	var->xres_virtual = CONFIG_FB_S5P_X_VRES;
	var->yres_virtual = CONFIG_FB_S5P_Y_VRES * CONFIG_FB_S5P_NR_BUFFERS;
#else
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres * CONFIG_FB_S5P_NR_BUFFERS;
#endif

	var->bits_per_pixel = 32;
	var->xoffset = 0;
	var->yoffset = 0;
	var->width = lcd->p_width;
	var->height = lcd->p_height;
	var->transp.length = 8;

	fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
	fix->smem_len = fix->line_length * var->yres_virtual;

	var->nonstd = 0;
	var->activate = FB_ACTIVATE_NOW;
	var->vmode = FB_VMODE_NONINTERLACED;
	var->hsync_len = timing->h_sw;
	var->vsync_len = timing->v_sw;
	var->left_margin = timing->h_bp;
	var->right_margin = timing->h_fp;
	var->upper_margin = timing->v_bp;
	var->lower_margin = timing->v_fp;
	var->pixclock = (lcd->freq *
		(var->left_margin + var->right_margin
		+ var->hsync_len + var->xres) *
		(var->upper_margin + var->lower_margin
		+ var->vsync_len + var->yres));
	var->pixclock = KHZ2PICOS(var->pixclock/1000);

	s3cfb_set_bitfield(var);
	s3cfb_set_alpha_info(var, win);

	return 0;
}

int s3cfb_alloc_framebuffer(struct s3cfb_global *fbdev, int fimd_id)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	int ret = 0;
	int i;

	fbdev->fb = kmalloc(pdata->nr_wins *
				sizeof(struct fb_info *), GFP_KERNEL);
	if (!fbdev->fb) {
		dev_err(fbdev->dev, "not enough memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	for (i = 0; i < pdata->nr_wins; i++) {
		fbdev->fb[i] = framebuffer_alloc(sizeof(struct s3cfb_window),
						fbdev->dev);
		if (!fbdev->fb[i]) {
			dev_err(fbdev->dev, "not enough memory\n");
			ret = -ENOMEM;
			goto err_alloc_fb;
		}

		ret = s3cfb_init_fbinfo(fbdev, i);
		if (ret) {
			dev_err(fbdev->dev,
				"failed to allocate memory for fb%d\n", i);
			ret = -ENOMEM;
			goto err_alloc_fb;
		}

		if (i == pdata->default_win)
			if (s3cfb_map_default_video_memory(fbdev,
						fbdev->fb[i], fimd_id)) {
				dev_err(fbdev->dev,
				"failed to map video memory "
				"for default window (%d)\n", i);
			ret = -ENOMEM;
			goto err_alloc_fb;
		}
		sec_getlog_supply_fbinfo((void *)fbdev->fb[i]->fix.smem_start,
					 fbdev->fb[i]->var.xres,
					 fbdev->fb[i]->var.yres,
					 fbdev->fb[i]->var.bits_per_pixel, 2);
	}

	return 0;

err_alloc_fb:
	for (i = 0; i < pdata->nr_wins; i++) {
		if (fbdev->fb[i])
			framebuffer_release(fbdev->fb[i]);
	}
	kfree(fbdev->fb);

err_alloc:
	return ret;
}

int s3cfb_open(struct fb_info *fb, int user)
{
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	int ret = 0;

	mutex_lock(&fbdev->lock);

	if (atomic_read(&win->in_use)) {
		if (win->id == pdata->default_win) {
			dev_dbg(fbdev->dev,
				"mutiple open fir default window\n");
			ret = 0;
		} else {
			dev_dbg(fbdev->dev,
				"do not allow multiple open	\
				for non-default window\n");
			ret = -EBUSY;
		}
	} else {
		atomic_inc(&win->in_use);
	}

	mutex_unlock(&fbdev->lock);

	return ret;
}

int s3cfb_release_window(struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);

#ifdef CONFIG_EXYNOS_DEV_PD
	if (unlikely(fbdev->system_state == POWER_OFF)) {
		dev_err(fbdev->dev, "%s::system_state is POWER_OFF, fb%d\n", __func__, win->id);
		return 0;
	}
#endif
	if (win->id != pdata->default_win) {
		s3cfb_disable_window(fbdev, win->id);
		s3cfb_unmap_video_memory(fbdev, fb);
#if !defined(CONFIG_CPU_EXYNOS4212) && !defined(CONFIG_CPU_EXYNOS4412)
		s3cfb_set_buffer_address(fbdev, win->id);
#endif
	}

	win->x = 0;
	win->y = 0;

	return 0;
}

int s3cfb_release(struct fb_info *fb, int user)
{
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);

	s3cfb_release_window(fb);

	mutex_lock(&fbdev->lock);
	atomic_dec(&win->in_use);
	mutex_unlock(&fbdev->lock);

	return 0;
}

inline unsigned int __chan_to_field(unsigned int chan, struct fb_bitfield bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf.length;

	return chan << bf.offset;
}

int s3cfb_setcolreg(unsigned int regno, unsigned int red,
			unsigned int green, unsigned int blue,
			unsigned int transp, struct fb_info *fb)
{
	unsigned int *pal = (unsigned int *)fb->pseudo_palette;
	unsigned int val = 0;

	if (regno < 16) {
		/* fake palette of 16 colors */
		val |= __chan_to_field(red, fb->var.red);
		val |= __chan_to_field(green, fb->var.green);
		val |= __chan_to_field(blue, fb->var.blue);
		val |= __chan_to_field(transp, fb->var.transp);
		pal[regno] = val;
	}

	return 0;
}

int s3cfb_blank(int blank_mode, struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;
	struct s3cfb_window *tmp_win;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	struct platform_device *pdev = to_platform_device(fbdev->dev);
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	int enabled_win = 0;
	int i;
#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412) || defined(CONFIG_CPU_EXYNOS4210)
	int win_status;
#endif

	dev_info(fbdev->dev, "change blank mode=%d, fb%d\n", blank_mode, win->id);

#ifdef CONFIG_EXYNOS_DEV_PD
	if (unlikely(fbdev->system_state == POWER_OFF)) {
		dev_err(fbdev->dev, "%s::system_state is POWER_OFF, fb%d\n", __func__, win->id);
		win->power_state = blank_mode;
		if (win->id != pdata->default_win)
			return NOT_DEFAULT_WINDOW;
		else
			return 0;
	}
#endif

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		if (!fb->fix.smem_start) {
			dev_info(fbdev->dev, "[fb%d] no allocated memory \
				for unblank\n", win->id);
			break;
		}

		if (win->power_state == FB_BLANK_UNBLANK) {
			dev_info(fbdev->dev, "[fb%d] already in \
				FB_BLANK_UNBLANK\n", win->id);
			break;
		} else {
			s3cfb_update_power_state(fbdev, win->id,
						FB_BLANK_UNBLANK);
		}

		enabled_win = atomic_read(&fbdev->enabled_win);
		if (enabled_win == 0) {
			/* temporarily nonuse for recovery, will modify code */
			/* pdata->clk_on(pdev, &fbdev->clock); */
			s3cfb_init_global(fbdev);
			s3cfb_set_clock(fbdev);
				for (i = 0; i < pdata->nr_wins; i++) {
					tmp_win = fbdev->fb[i]->par;
					if (tmp_win->owner == DMA_MEM_FIMD)
						s3cfb_set_win_params(fbdev,
								tmp_win->id);
				}
		}

		if (win->enabled)	/* from FB_BLANK_NORMAL */
			s3cfb_win_map_off(fbdev, win->id);
		else			/* from FB_BLANK_POWERDOWN */
			s3cfb_enable_window(fbdev, win->id);

		if (enabled_win == 0) {
			s3cfb_display_on(fbdev);

			if (pdata->backlight_on)
				pdata->backlight_on(pdev);
			if (pdata->lcd_on)
				pdata->lcd_on(pdev);
			if (fbdev->lcd->init_ldi)
				fbdev->lcd->init_ldi();
		}
#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)\
	|| defined(CONFIG_CPU_EXYNOS4210)
		win_status = window_on_off_status(fbdev);
		if (win_status == 0)
#else
		if (win->id != pdata->default_win)
#endif
			return NOT_DEFAULT_WINDOW;

		break;

	case FB_BLANK_NORMAL:
		if (win->power_state == FB_BLANK_NORMAL) {
			dev_info(fbdev->dev, "[fb%d] already in FB_BLANK_NORMAL\n", win->id);
			break;
		} else {
			s3cfb_update_power_state(fbdev, win->id,
						FB_BLANK_NORMAL);
		}

		enabled_win = atomic_read(&fbdev->enabled_win);
		if (enabled_win == 0) {
			pdata->clk_on(pdev, &fbdev->clock);
			s3cfb_init_global(fbdev);
			s3cfb_set_clock(fbdev);

			for (i = 0; i < pdata->nr_wins; i++) {
				tmp_win = fbdev->fb[i]->par;
				if (tmp_win->owner == DMA_MEM_FIMD)
					s3cfb_set_win_params(fbdev,
							tmp_win->id);
			}
		}

		s3cfb_win_map_on(fbdev, win->id, 0x0);

		if (!win->enabled)	/* from FB_BLANK_POWERDOWN */
			s3cfb_enable_window(fbdev, win->id);

		if (enabled_win == 0) {
			s3cfb_display_on(fbdev);

			if (pdata->backlight_on)
				pdata->backlight_on(pdev);
			if (pdata->lcd_on)
				pdata->lcd_on(pdev);
			if (fbdev->lcd->init_ldi)
				fbdev->lcd->init_ldi();
		}
#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)\
	|| defined(CONFIG_CPU_EXYNOS4210)
		win_status = window_on_off_status(fbdev);
		if (win_status == 0)
#else
		if (win->id != pdata->default_win)
#endif
			return NOT_DEFAULT_WINDOW;

		break;

	case FB_BLANK_POWERDOWN:
		if (win->power_state == FB_BLANK_POWERDOWN) {
			dev_info(fbdev->dev, "[fb%d] already in FB_BLANK_POWERDOWN\n", win->id);
			break;
		} else {
			s3cfb_update_power_state(fbdev, win->id,
						FB_BLANK_POWERDOWN);
		}

		s3cfb_disable_window(fbdev, win->id);
		s3cfb_win_map_off(fbdev, win->id);

		if (atomic_read(&fbdev->enabled_win) == 0) {
			if (pdata->backlight_off)
				pdata->backlight_off(pdev);
			if (fbdev->lcd->deinit_ldi)
				fbdev->lcd->deinit_ldi();
			if (pdata->lcd_off)
				pdata->lcd_off(pdev);
			/* temporaily nonuse for recovery , will modify code */
			/* s3cfb_display_off(fbdev);
			pdata->clk_off(pdev, &fbdev->clock); */
		}
#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)\
	|| defined(CONFIG_CPU_EXYNOS4210)
		win_status = window_on_off_status(fbdev);
		if (win_status != 0)
#else
		if (win->id != pdata->default_win)
#endif
			return NOT_DEFAULT_WINDOW;
		break;

	case FB_BLANK_VSYNC_SUSPEND:	/* fall through */
	case FB_BLANK_HSYNC_SUSPEND:	/* fall through */
	default:
		dev_dbg(fbdev->dev, "unsupported blank mode\n");
		return -EINVAL;
	}

	return 0;
}

int s3cfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);

	if (win->id == pdata->default_win)
		spin_lock(&fbdev->slock);

#ifdef CONFIG_EXYNOS_DEV_PD
	if (unlikely(fbdev->system_state == POWER_OFF) || fbdev->regs == 0) {
		dev_err(fbdev->dev, "%s::system_state is POWER_OFF, fb%d\n", __func__, win->id);
		if (win->id == pdata->default_win)
			spin_unlock(&fbdev->slock);
		return -EINVAL;
	}
#endif

	if (var->yoffset + var->yres > var->yres_virtual) {
		dev_err(fbdev->dev, "invalid yoffset value\n");
		if (win->id == pdata->default_win)
			spin_unlock(&fbdev->slock);
		return -EINVAL;
	}

#if defined(CONFIG_CPU_EXYNOS4210)
	if (unlikely(var->xoffset + var->xres > var->xres_virtual)) {
		dev_err(fbdev->dev, "invalid xoffset value\n");
		if (win->id == pdata->default_win)
			spin_unlock(&fbdev->slock);
		return -EINVAL;
	}
	fb->var.xoffset = var->xoffset;
#endif

	fb->var.yoffset = var->yoffset;

	dev_dbg(fbdev->dev, "[fb%d] yoffset for pan display: %d\n", win->id,
		var->yoffset);

	s3cfb_set_buffer_address(fbdev, win->id);

	if (win->id == pdata->default_win)
		spin_unlock(&fbdev->slock);
	return 0;
}

int s3cfb_cursor(struct fb_info *fb, struct fb_cursor *cursor)
{
	/* nothing to do for removing cursor */
	return 0;
}

#if !defined(CONFIG_FB_S5P_VSYNC_THREAD)
int s3cfb_wait_for_vsync(struct s3cfb_global *fbdev)
{
	dev_dbg(fbdev->dev, "waiting for VSYNC interrupt\n");

	sleep_on_timeout(&fbdev->wq, HZ / 10);

	dev_dbg(fbdev->dev, "got a VSYNC interrupt\n");

	return 0;
}
#endif


/**
 * s3c_fb_align_word() - align pixel count to word boundary
 * @bpp: The number of bits per pixel
 * @pix: The value to be aligned.
 *
 * Align the given pixel count so that it will start on an 32bit word
 * boundary.
 */
static int s3c_fb_align_word(unsigned int bpp, unsigned int pix)
{
	int pix_per_word;

	if (bpp > 16)
		return pix;

	pix_per_word = (8 * 32) / bpp;
	return ALIGN(pix, pix_per_word);
}

static u32 s3c_fb_red_length(int format)
{
	switch (format) {
	case S3C_FB_PIXEL_FORMAT_RGBA_8888:
	case S3C_FB_PIXEL_FORMAT_RGBX_8888:
	case S3C_FB_PIXEL_FORMAT_RGB_888:
	case S3C_FB_PIXEL_FORMAT_BGRA_8888:
		return 8;

	case S3C_FB_PIXEL_FORMAT_RGB_565:
	case S3C_FB_PIXEL_FORMAT_RGBA_5551:
		return 5;

	case S3C_FB_PIXEL_FORMAT_RGBA_4444:
		return 4;

	default:
		pr_warn("s3c-fb: unrecognized pixel format %u\n", format);
		return 0;
	}
}

static u32 s3c_fb_red_offset(int format)
{
	switch (format) {
	case S3C_FB_PIXEL_FORMAT_RGBA_8888:
	case S3C_FB_PIXEL_FORMAT_RGBX_8888:
	case S3C_FB_PIXEL_FORMAT_RGB_888:
	case S3C_FB_PIXEL_FORMAT_RGB_565:
	case S3C_FB_PIXEL_FORMAT_RGBA_5551:
	case S3C_FB_PIXEL_FORMAT_RGBA_4444:
		return 0;

	case S3C_FB_PIXEL_FORMAT_BGRA_8888:
		return 16;

	default:
		pr_warn("s3c-fb: unrecognized pixel format %u\n", format);
		return 0;
	}
}

static u32 s3c_fb_green_length(int format)
{
	if (format == S3C_FB_PIXEL_FORMAT_RGB_565)
		return 6;

	return s3c_fb_red_length(format);
}

static u32 s3c_fb_green_offset(int format)
{
	switch (format) {
	case S3C_FB_PIXEL_FORMAT_RGBA_8888:
	case S3C_FB_PIXEL_FORMAT_RGB_888:
	case S3C_FB_PIXEL_FORMAT_BGRA_8888:
	case S3C_FB_PIXEL_FORMAT_RGBX_8888:
		return 8;

	case S3C_FB_PIXEL_FORMAT_RGB_565:
	case S3C_FB_PIXEL_FORMAT_RGBA_5551:
		return 5;

	case S3C_FB_PIXEL_FORMAT_RGBA_4444:
		return 4;

	default:
		pr_warn("s3c-fb: unrecognized pixel format %u\n", format);
		return 0;
	}
}

static u32 s3c_fb_blue_length(int format)
{
	return s3c_fb_red_length(format);
}

static u32 s3c_fb_blue_offset(int format)
{
	switch (format) {
	case S3C_FB_PIXEL_FORMAT_RGBA_8888:
	case S3C_FB_PIXEL_FORMAT_RGB_888:
	case S3C_FB_PIXEL_FORMAT_RGBX_8888:
		return 16;

	case S3C_FB_PIXEL_FORMAT_RGB_565:
		return 11;

	case S3C_FB_PIXEL_FORMAT_RGBA_5551:
		return 10;

	case S3C_FB_PIXEL_FORMAT_RGBA_4444:
		return 8;

	case S3C_FB_PIXEL_FORMAT_BGRA_8888:
		return 0;

	default:
		pr_warn("s3c-fb: unrecognized pixel format %u\n", format);
		return 0;
	}
}

static u32 s3c_fb_transp_length(int format)
{
	switch (format) {
	case S3C_FB_PIXEL_FORMAT_RGBA_8888:
	case S3C_FB_PIXEL_FORMAT_BGRA_8888:
		return 8;

	case S3C_FB_PIXEL_FORMAT_RGBA_5551:
		return 1;

	case S3C_FB_PIXEL_FORMAT_RGBA_4444:
		return 4;

	case S3C_FB_PIXEL_FORMAT_RGB_888:
	case S3C_FB_PIXEL_FORMAT_RGB_565:
	case S3C_FB_PIXEL_FORMAT_RGBX_8888:
		return 0;

	default:
		pr_warn("s3c-fb: unrecognized pixel format %u\n", format);
		return 0;
	}
}

static u32 s3c_fb_transp_offset(int format)
{
	switch (format) {
	case S3C_FB_PIXEL_FORMAT_RGBA_8888:
	case S3C_FB_PIXEL_FORMAT_BGRA_8888:
		return 24;

	case S3C_FB_PIXEL_FORMAT_RGBA_5551:
		return 15;

	case S3C_FB_PIXEL_FORMAT_RGBA_4444:
		return 12;

	case S3C_FB_PIXEL_FORMAT_RGB_888:
	case S3C_FB_PIXEL_FORMAT_RGB_565:
	case S3C_FB_PIXEL_FORMAT_RGBX_8888:
		return s3c_fb_blue_offset(format);

	default:
		pr_warn("s3c-fb: unrecognized pixel format %u\n", format);
		return 0;
	}
}

static u32 s3c_fb_padding(int format)
{
	switch (format) {
	case S3C_FB_PIXEL_FORMAT_RGBX_8888:
		return 8;

	case S3C_FB_PIXEL_FORMAT_RGB_565:
	case S3C_FB_PIXEL_FORMAT_RGBA_8888:
	case S3C_FB_PIXEL_FORMAT_BGRA_8888:
	case S3C_FB_PIXEL_FORMAT_RGBA_5551:
	case S3C_FB_PIXEL_FORMAT_RGBA_4444:
		return 0;

	default:
		pr_warn("s3c-fb: unrecognized pixel format %u\n", format);
		return 0;
	}

}

static inline u32 fb_visual(u32 bits_per_pixel, unsigned short palette_sz)
{
	switch (bits_per_pixel) {
	case 32:
	case 24:
	case 16:
	case 12:
		return FB_VISUAL_TRUECOLOR;
	case 8:
		if (palette_sz >= 256)
			return FB_VISUAL_PSEUDOCOLOR;
		else
			return FB_VISUAL_TRUECOLOR;
	case 1:
		return FB_VISUAL_MONO01;
	default:
		return FB_VISUAL_PSEUDOCOLOR;
	}
}


static inline u32 fb_linelength(u32 xres_virtual, u32 bits_per_pixel)
{
	return (xres_virtual * bits_per_pixel) / 8;
}

static inline u16 fb_panstep(u32 res, u32 res_virtual)
{
	return res_virtual > res ? 1 : 0;
}

static inline u32 vidw_buf_size(u32 xres, u32 line_length, u32 bits_per_pixel)
{
	u32 pagewidth = (xres * bits_per_pixel) >> 3;
	return VIDW_BUF_SIZE_OFFSET(line_length - pagewidth) |
	       VIDW_BUF_SIZE_PAGEWIDTH(pagewidth) |
	       VIDW_BUF_SIZE_OFFSET_E(line_length - pagewidth) |
	       VIDW_BUF_SIZE_PAGEWIDTH_E(pagewidth);
}

inline u32 vidosd_a(int x, int y)
{
	return VIDOSDxA_TOPLEFT_X(x) |
			VIDOSDxA_TOPLEFT_Y(y) |
			VIDOSDxA_TOPLEFT_X_E(x) |
			VIDOSDxA_TOPLEFT_Y_E(y);
}

inline u32 vidosd_b(int x, int y, u32 xres, u32 yres, u32 bits_per_pixel)
{
	return VIDOSDxB_BOTRIGHT_X(s3c_fb_align_word(bits_per_pixel,
			x + xres - 1)) |
		VIDOSDxB_BOTRIGHT_Y(y + yres - 1) |
		VIDOSDxB_BOTRIGHT_X_E(s3c_fb_align_word(bits_per_pixel,
			x + xres - 1)) |
		VIDOSDxB_BOTRIGHT_Y_E(y + yres - 1);
}

static inline u32 wincon(u32 bits_per_pixel, u32 transp_length, u32 red_length)
{
	u32 data = 0;

	switch (bits_per_pixel) {
	case 1:
		data |= WINCON0_BPPMODE_1BPP;
		data |= WINCONx_BITSWP;
		data |= WINCONx_BURSTLEN_4WORD;
		break;
	case 2:
		data |= WINCON0_BPPMODE_2BPP;
		data |= WINCONx_BITSWP;
		data |= WINCONx_BURSTLEN_8WORD;
		break;
	case 4:
		data |= WINCON0_BPPMODE_4BPP;
		data |= WINCONx_BITSWP;
		data |= WINCONx_BURSTLEN_8WORD;
		break;
	case 8:
		if (transp_length != 0)
			data |= WINCON1_BPPMODE_8BPP_1232;
		else
			data |= WINCON0_BPPMODE_8BPP_PALETTE;
		data |= WINCONx_BURSTLEN_8WORD;
		data |= WINCONx_BYTSWP;
		break;
	case 16:
		if (transp_length != 0)
			data |= WINCON1_BPPMODE_16BPP_A1555;
		else
			data |= WINCON0_BPPMODE_16BPP_565;
		data |= WINCONx_HAWSWP;
		data |= WINCONx_BURSTLEN_16WORD;
		break;
	case 24:
	case 32:
		if (red_length == 6) {
			if (transp_length != 0)
				data |= WINCON1_BPPMODE_19BPP_A1666;
			else
				data |= WINCON1_BPPMODE_18BPP_666;
		} else if (transp_length == 1)
			data |= WINCON1_BPPMODE_25BPP_A1888
				| WINCON1_BLD_PIX;
		else if ((transp_length == 4) ||
			(transp_length == 8))
			data |= WINCON1_BPPMODE_28BPP_A4888
				| WINCON1_BLD_PIX | WINCON1_ALPHA_SEL;
		else
			data |= WINCON0_BPPMODE_24BPP_888;

		data |= WINCONx_WSWP;
		data |= WINCONx_BURSTLEN_16WORD;
		break;
	}

	return data;
}

void s3c_fb_update_regs(struct s3cfb_global *fbdev, struct s3c_reg_data *regs)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	unsigned short i;
	bool wait_for_vsync;
	struct s3cfb_window *win;

#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
#ifdef CONFIG_BUSFREQ_OPP
	unsigned int new_num_of_win = 0;
	unsigned int pre_num_of_win = 0;
	unsigned int shadow_regs = 0;
	unsigned int clkval = 0;

	for (i = 0; i < pdata->nr_wins; i++)
		if (regs->shadowcon & SHADOWCON_CHx_ENABLE(i))
			new_num_of_win++;
	shadow_regs = readl(fbdev->regs + S3C_WINSHMAP);
	for (i = 0; i < pdata->nr_wins; i++)
		if (shadow_regs & SHADOWCON_CHx_ENABLE(i))
			pre_num_of_win++;

	if (pre_num_of_win < new_num_of_win) {
		switch(new_num_of_win) {
		case 0:
		case 1:
			clkval = 100100;
			break;
		case 2:
			clkval = 133133;
			break;
		case 3:
			clkval = 133133;
			break;
		case 4:
			clkval = 133133;
			break;
		case 5:
			clkval = 133133;
			break;
		}
		dev_lock(fbdev->bus_dev, fbdev->dev, clkval);
	}
#endif
#endif

	for (i = 0; i < pdata->nr_wins; i++)
		s3cfb_set_window_protect(fbdev, i, 1);

	for (i = 0; i < pdata->nr_wins; i++) {
		win = fbdev->fb[i]->par;
		writel(regs->wincon[i], fbdev->regs + S3C_WINCON(i));
		writel(regs->vidosd_a[i], fbdev->regs + S3C_VIDOSD_A(i));
		writel(regs->vidosd_b[i], fbdev->regs + S3C_VIDOSD_B(i));
		writel(regs->vidosd_c[i], fbdev->regs + S3C_VIDOSD_C(i));
		if (i == 1 || i == 2)
			writel(regs->vidosd_d[i], fbdev->regs + S3C_VIDOSD_D(i));
		writel(regs->vidw_buf_start[i],
				fbdev->regs + S3C_VIDADDR_START0(i));
		writel(regs->vidw_buf_end[i],
				fbdev->regs + S3C_VIDADDR_END0(i));
		writel(regs->vidw_buf_size[i],
				fbdev->regs + S3C_VIDADDR_SIZE(i));

		win->enabled = !!(regs->wincon[i] & WINCONx_ENWIN);
	}

	writel(regs->shadowcon, fbdev->regs + S3C_WINSHMAP);

	for (i = 0; i < pdata->nr_wins; i++)
		s3cfb_set_window_protect(fbdev, i, 0);

	do {
#if defined(CONFIG_FB_S5P_VSYNC_THREAD)
		s3cfb_wait_for_vsync(fbdev, 0);
#else
		s3cfb_wait_for_vsync(fbdev);
#endif
		wait_for_vsync = false;

		for (i = 0; i < pdata->nr_wins; i++) {
			u32 new_start = regs->vidw_buf_start[i];
			u32 shadow_start = s3cfb_get_win_cur_buf_addr(fbdev, i);
			if (unlikely(new_start != shadow_start)) {
				wait_for_vsync = true;
				break;
			}
		}
	} while (wait_for_vsync);

	sw_sync_timeline_inc(fbdev->timeline, 1);

#ifdef CONFIG_FB_S5P_SYSMMU
       if ((fbdev->sysmmu.enabled == false) &&
                       (fbdev->sysmmu.pgd)) {
               fbdev->sysmmu.enabled = true;
               s5p_sysmmu_enable(fbdev->dev,
                               (unsigned long)virt_to_phys((unsigned int*)fbdev->sysmmu.pgd));
       }
#endif

#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
#ifdef CONFIG_BUSFREQ_OPP
	if (pre_num_of_win > new_num_of_win) {
		switch(new_num_of_win) {
		case 0:
		case 1:
			clkval = 100100;
			break;
		case 2:
			clkval = 133133;
			break;
		case 3:
			clkval = 133133;
			break;
		case 4:
			clkval = 133133;
			break;
		case 5:
			clkval = 133133;
			break;
		}
		dev_lock(fbdev->bus_dev, fbdev->dev, clkval);
	}
#endif
#endif
}

static int s3c_fb_set_win_buffer(struct s3cfb_global *fbdev,
		struct fb_info *fb, struct s3c_fb_win_config *win_config,
		struct s3c_reg_data *regs)
{
	struct s3cfb_window *win = fb->par;
	struct fb_fix_screeninfo prev_fix = fb->fix;
	struct fb_var_screeninfo prev_var = fb->var;
	unsigned short win_no = win->id;
	int ret;
	size_t window_size;
	u32 alpha, size;

	if (win_config->format >= S3C_FB_PIXEL_FORMAT_MAX) {
		dev_err(fbdev->dev, "unknown pixel format %u\n",
				win_config->format);
		return -EINVAL;
	}

	fb->var.red.length = s3c_fb_red_length(win_config->format);
	fb->var.red.offset = s3c_fb_red_offset(win_config->format);
	fb->var.green.length = s3c_fb_green_length(win_config->format);
	fb->var.green.offset = s3c_fb_green_offset(win_config->format);
	fb->var.blue.length = s3c_fb_blue_length(win_config->format);
	fb->var.blue.offset = s3c_fb_blue_offset(win_config->format);
	fb->var.transp.length = s3c_fb_transp_length(win_config->format);
	fb->var.transp.offset = s3c_fb_transp_offset(win_config->format);
	fb->var.bits_per_pixel = fb->var.red.length +
			fb->var.green.length +
			fb->var.blue.length +
			fb->var.transp.length +
			s3c_fb_padding(win_config->format);

	window_size = win_config->stride * win_config->h;

	if (win_config->phys_addr != 0)
		fb->fix.smem_start = win_config->phys_addr + win_config->offset;
	else
		fb->fix.smem_start = win_config->virt_addr + win_config->offset;
	fb->fix.smem_len = window_size;
	fb->var.xres = win_config->w;
	fb->var.xres_virtual = win_config->stride * 8 /
			fb->var.bits_per_pixel;
	fb->var.yres = fb->var.yres_virtual = win_config->h;
	fb->var.xoffset = win_config->offset % win_config->stride;
	fb->var.yoffset = win_config->offset / win_config->stride;

	fb->fix.visual = fb_visual(fb->var.bits_per_pixel, 256);
	fb->fix.line_length = win_config->stride;
	fb->fix.xpanstep = fb_panstep(win_config->w,
			fb->var.xres_virtual);
	fb->fix.ypanstep = fb_panstep(win_config->h, win_config->h);

#ifdef CONFIG_FB_S5P_SYSMMU
	if ((fbdev->sysmmu.enabled == false) &&
			(current->mm))
		fbdev->sysmmu.pgd = (unsigned int)current->mm->pgd;
#endif

#ifdef CONFIG_FB_S5P_SYSMMU
	if (win_config->phys_addr != 0)
		regs->vidw_buf_start[win_no] = (u32)phys_to_virt(fb->fix.smem_start);
	else
#endif
		regs->vidw_buf_start[win_no] = fb->fix.smem_start;

	regs->vidw_buf_end[win_no] = regs->vidw_buf_start[win_no] +
			window_size;

	regs->vidw_buf_size[win_no] = vidw_buf_size(win_config->w,
			fb->fix.line_length,
			fb->var.bits_per_pixel);

#ifdef CONFIG_FB_S5P_SYSMMU
	if ((fb->fix.smem_start) &&
			(fb->fix.smem_start != fbdev->sysmmu.default_fb_addr))
		s3cfb_clean_outer_pagetable(regs->vidw_buf_start[win_no],
				regs->vidw_buf_end[win_no] - regs->vidw_buf_start[win_no]);
#endif


	regs->vidosd_a[win_no] = vidosd_a(win_config->x, win_config->y);
	regs->vidosd_b[win_no] = vidosd_b(win_config->x, win_config->y,
			win_config->w, win_config->h,
			fb->var.bits_per_pixel);

	alpha = VIDISD14C_ALPHA1_R(0xf) |
		VIDISD14C_ALPHA1_G(0xf) |
		VIDISD14C_ALPHA1_B(0xf);
	regs->vidosd_c[win_no] = alpha;

	if (win_no <= 2) {
		size = win_config->w * win_config->h;
		regs->vidosd_d[win_no] = size;
	}

	regs->shadowcon |= SHADOWCON_CHx_ENABLE(win_no);

	regs->wincon[win_no] = wincon(fb->var.bits_per_pixel,
			fb->var.transp.length,
			fb->var.red.length);

	return 0;
}

static int s3c_fb_set_win_config(struct s3cfb_global *fbdev,
		struct s3c_fb_win_config_data *win_data)
{
	struct fb_info *fb;
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct s3c_fb_win_config *win_config = win_data->config;
	int ret = 0;
	unsigned short i;
	struct s3c_reg_data *regs = kzalloc(sizeof(struct s3c_reg_data),
			GFP_KERNEL);
	struct sync_fence *fence;
	struct sync_pt *pt;
	int fd;

	if (!regs) {
		dev_err(fbdev->dev, "could not allocate s3c_reg_data");
		return -ENOMEM;
	}

	fd = get_unused_fd();

	for (i = 0; i < pdata->nr_wins && !ret; i++) {
		struct s3c_fb_win_config *config = &win_config[i];
		bool enabled = 0;
		u32 color_map = WINxMAP_MAP | WINxMAP_MAP_COLOUR(0);

		fb = fbdev->fb[i];

		switch (config->state) {
		case S3C_FB_WIN_STATE_DISABLED:
			break;
		case S3C_FB_WIN_STATE_COLOR:
			enabled = 1;
			color_map |= WINxMAP_MAP_COLOUR(config->color);
			break;
		case S3C_FB_WIN_STATE_BUFFER:
			ret = s3c_fb_set_win_buffer(fbdev, fb, config, regs);
			if (!ret) {
				enabled = 1;
				color_map = 0;
			}
			break;
		default:
			dev_warn(fbdev->dev, "unrecognized window state %u",
					config->state);
			ret = -EINVAL;
			break;
		}

		if (enabled)
			regs->wincon[i] |= WINCONx_ENWIN;
		else
			regs->wincon[i] &= ~WINCONx_ENWIN;
		regs->winmap[i] = color_map;
	}

	if (ret) {
		put_unused_fd(fd);
		kfree(regs);
	} else {
		mutex_lock(&fbdev->update_regs_list_lock);
		fbdev->timeline_max++;
		pt = sw_sync_pt_create(fbdev->timeline, fbdev->timeline_max);
		fence = sync_fence_create("display", pt);
		sync_fence_install(fence, fd);
		win_data->fence = fd;

		list_add_tail(&regs->list, &fbdev->update_regs_list);
		mutex_unlock(&fbdev->update_regs_list_lock);
		queue_kthread_work(&fbdev->update_regs_worker, &fbdev->update_regs_work);
	}

	return ret;
}

int s3cfb_ioctl(struct fb_info *fb, unsigned int cmd, unsigned long arg)
{
	struct fb_var_screeninfo *var = &fb->var;
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	struct s3cfb_lcd *lcd = fbdev->lcd;
	void *argp = (void *)arg;
	int ret = 0;
#if defined(CONFIG_CPU_EXYNOS4210)
	unsigned int addr = 0;
#endif
	dma_addr_t start_addr = 0;
	struct fb_fix_screeninfo *fix = &fb->fix;

	union {
		struct s3cfb_user_window user_window;
		struct s3cfb_user_plane_alpha user_alpha;
		struct s3cfb_user_chroma user_chroma;
		struct s3c_fb_win_config_data win_data;
		int vsync;
		unsigned int alpha_mode;
	} p;

#ifdef CONFIG_EXYNOS_DEV_PD
	if (unlikely(fbdev->system_state == POWER_OFF)) {
		dev_err(fbdev->dev, "%s::system_state is POWER_OFF cmd is 0x%08x, fb%d\n", __func__, cmd, win->id);
		return -EFAULT;
	}
#endif

	switch (cmd) {
	case S3CFB_SET_WIN_ADDR:
		fix->smem_start = (unsigned long)argp;
		s3cfb_set_buffer_address(fbdev, win->id);
		break;

	case FBIO_WAITFORVSYNC:
		if (fbdev->regs == 0)
			return 0;
#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
#ifdef CONFIG_CPU_EXYNOS4412
		if (!fbdev->regs)
			return ret;
#endif
#if !defined(CONFIG_FB_S5P_VSYNC_THREAD)
		/* Enable Vsync */
		s3cfb_set_global_interrupt(fbdev, 1);
		s3cfb_set_vsync_interrupt(fbdev, 1);
#endif
#endif
		/* Wait for Vsync */
#if defined(CONFIG_FB_S5P_VSYNC_THREAD)
		s3cfb_wait_for_vsync(fbdev, HZ/10);
#else
		s3cfb_wait_for_vsync(fbdev);
#endif
		if (fbdev->regs == 0)
			return 0;
#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
#if !defined(CONFIG_FB_S5P_VSYNC_THREAD)
		/* Disable Vsync */
		s3cfb_set_global_interrupt(fbdev, 0);
		s3cfb_set_vsync_interrupt(fbdev, 0);
#endif
#endif
		break;

	case S3CFB_WIN_POSITION:
		if (copy_from_user(&p.user_window,
				   (struct s3cfb_user_window __user *)arg,
				   sizeof(p.user_window)))
			ret = -EFAULT;
		else {
#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
			win->x = p.user_window.x;
			win->y = p.user_window.y;
#else
			if (p.user_window.x < 0)
				p.user_window.x = 0;

			if (p.user_window.y < 0)
				p.user_window.y = 0;

			if (p.user_window.x + var->xres > lcd->width)
				win->x = lcd->width - var->xres;
			else
				win->x = p.user_window.x;

			if (p.user_window.y + var->yres > lcd->height)
				win->y = lcd->height - var->yres;
			else
				win->y = p.user_window.y;

			s3cfb_set_window_position(fbdev, win->id);
#endif
		}
		break;

	case S3CFB_WIN_SET_PLANE_ALPHA:
		if (copy_from_user(&p.user_alpha,
				   (struct s3cfb_user_plane_alpha __user *)arg,
				   sizeof(p.user_alpha)))
			ret = -EFAULT;
		else {
			win->alpha.mode = PLANE_BLENDING;
			win->alpha.channel = p.user_alpha.channel;
			win->alpha.value =
			    S3CFB_AVALUE(p.user_alpha.red,
					 p.user_alpha.green, p.user_alpha.blue);

			s3cfb_set_alpha_blending(fbdev, win->id);
		}
		break;

	case S3CFB_WIN_SET_CHROMA:
		if (copy_from_user(&p.user_chroma,
				   (struct s3cfb_user_chroma __user *)arg,
				   sizeof(p.user_chroma)))
			ret = -EFAULT;
		else {
			win->chroma.enabled = p.user_chroma.enabled;
			win->chroma.key = S3CFB_CHROMA(p.user_chroma.red,
						       p.user_chroma.green,
						       p.user_chroma.blue);

			s3cfb_set_chroma_key(fbdev, win->id);
		}
		break;

	case S3CFB_SET_VSYNC_INT:
		if (get_user(p.vsync, (int __user *)arg))
			ret = -EFAULT;
		else {
#ifdef CONFIG_CPU_EXYNOS4412
			if (!fbdev->regs)
				return ret;
#endif
#if defined(CONFIG_FB_S5P_VSYNC_THREAD)
			ret = s3cfb_set_vsync_int(fb, p.vsync);
#else
			s3cfb_set_global_interrupt(fbdev, p.vsync);
			s3cfb_set_vsync_interrupt(fbdev, p.vsync);
#endif
		}
		break;

	case S3CFB_GET_FB_PHY_ADDR:
		start_addr = fix->smem_start + ((var->xres_virtual *
				var->yoffset + var->xoffset) *
				(var->bits_per_pixel / 8));

		dev_dbg(fbdev->dev, "framebuffer_addr: 0x%08x\n", start_addr);

		if (copy_to_user((void *)arg, &start_addr, sizeof(unsigned int))) {
			dev_err(fbdev->dev, "copy_to_user error\n");
			return -EFAULT;
		}

		break;

	case S3CFB_GET_CUR_WIN_BUF_ADDR:
		start_addr = s3cfb_get_win_cur_buf_addr(fbdev, win->id);
		dev_dbg(fbdev->dev, "win_cur_buf_addr: 0x%08x\n", start_addr);
		if (copy_to_user((void *)arg, &start_addr, sizeof(unsigned int))) {
			dev_err(fbdev->dev, "copy_to_user error\n");
			return -EFAULT;
		}
		break;

#if defined(CONFIG_CPU_EXYNOS4210)
	case S3CFB_SET_WIN_MEM_START:
		if (copy_from_user(&addr, (unsigned int *)arg, sizeof(unsigned int)))
			ret = -EFAULT;
		else {
			fix->smem_start = (unsigned long)addr;
			printk("\n\n######smem_start %x \n\n", (unsigned int)fix->smem_start);
		}
		break;
#endif

	case S3CFB_SET_WIN_ON:
		s3cfb_enable_window(fbdev, win->id);
		break;

	case S3CFB_SET_WIN_OFF:
		s3cfb_disable_window(fbdev, win->id);
		break;

	case S3CFB_SET_ALPHA_MODE:
		if (copy_from_user(&p.alpha_mode,
				   (struct s3cfb_user_window __user *)arg,
				   sizeof(p.alpha_mode)))
			ret = -EFAULT;
		else
			s3cfb_set_alpha_mode(fbdev, win->id, p.alpha_mode);
		break;

#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
	case S3CFB_WIN_CONFIG:
		if (copy_from_user(&p.win_data,
				   (struct s3c_fb_win_config_data __user *)arg,
				   sizeof(p.win_data))) {
			ret = -EFAULT;
			break;
		}

		ret = s3c_fb_set_win_config(fbdev, &p.win_data);
		if (ret)
			break;

		if (copy_to_user((struct s3c_fb_win_config_data __user *)arg,
				 &p.win_data,
				 sizeof(p.win_data))) {
			ret = -EFAULT;
			break;
		}
		break;
#endif
	}

	return ret;
}

int s3cfb_enable_localpath(struct s3cfb_global *fbdev, int id)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;

	if (s3cfb_channel_localpath_on(fbdev, id)) {
		win->enabled = 0;
		return -EFAULT;
	} else {
		win->enabled = 1;
		return 0;
	}
}

int s3cfb_disable_localpath(struct s3cfb_global *fbdev, int id)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;

	if (s3cfb_channel_localpath_off(fbdev, id)) {
		win->enabled = 1;
		return -EFAULT;
	} else {
		win->enabled = 0;
		return 0;
	}
}

int s3cfb_open_fifo(int id, int ch, int (*do_priv) (void *), void *param)
{
	struct s3cfb_global *fbdev = get_fimd_global(id);
	struct s3cfb_window *win = fbdev->fb[id]->par;

	dev_dbg(fbdev->dev, "[fb%d] open fifo\n", win->id);

	if (win->path == DATA_PATH_DMA) {
		dev_err(fbdev->dev, "WIN%d is busy.\n", id);
		return -EFAULT;
	}

	win->local_channel = ch;

	if (do_priv) {
		if (do_priv(param)) {
			dev_err(fbdev->dev, "failed to run for "
				"private fifo open\n");
			s3cfb_enable_window(fbdev, id);
			return -EFAULT;
		}
	}

	s3cfb_set_window_control(fbdev, id);
	s3cfb_enable_window(fbdev, id);
	s3cfb_enable_localpath(fbdev, id);

	return 0;
}
EXPORT_SYMBOL(s3cfb_open_fifo);

int s3cfb_close_fifo(int id, int (*do_priv) (void *), void *param)
{
	struct s3cfb_global *fbdev = get_fimd_global(id);
	struct s3cfb_window *win = fbdev->fb[id]->par;
	win->path = DATA_PATH_DMA;

	dev_dbg(fbdev->dev, "[fb%d] close fifo\n", win->id);

	if (do_priv) {
		s3cfb_display_off(fbdev);

		if (do_priv(param)) {
			dev_err(fbdev->dev, "failed to run for"
				"private fifo close\n");
			s3cfb_enable_window(fbdev, id);
			s3cfb_display_on(fbdev);
			return -EFAULT;
		}

		s3cfb_disable_window(fbdev, id);
		s3cfb_disable_localpath(fbdev, id);
		s3cfb_display_on(fbdev);
	} else {
		s3cfb_disable_window(fbdev, id);
		s3cfb_disable_localpath(fbdev, id);
	}

	return 0;
}
EXPORT_SYMBOL(s3cfb_close_fifo);

int s3cfb_direct_ioctl(int id, unsigned int cmd, unsigned long arg)
{
	struct s3cfb_global *fbdev = get_fimd_global(id);
	struct fb_info *fb = fbdev->fb[id];
	struct fb_var_screeninfo *var = &fb->var;
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s3cfb_window *win = fb->par;
	struct s3cfb_lcd *lcd = fbdev->lcd;
	struct s3cfb_user_window user_win;
#ifdef CONFIG_EXYNOS_DEV_PD
	struct platform_device *pdev = to_platform_device(fbdev->dev);
#endif
	void *argp = (void *)arg;
	int ret = 0;

#ifdef CONFIG_EXYNOS_DEV_PD
	/* enable the power domain */
	if (fbdev->system_state == POWER_OFF) {
		/* This IOCTLs are came from fimc irq.
		 * To avoid scheduling problem in irq mode,
		 * runtime get/put sync shold be not called.
		 */
		switch (cmd) {
		case S3CFB_SET_WIN_ADDR:
			fix->smem_start = (unsigned long)argp;
			return ret;

		case S3CFB_SET_WIN_ON:
			if (!win->enabled)
				atomic_inc(&fbdev->enabled_win);
			win->enabled = 1;

			return ret;
		}

		pm_runtime_get_sync(&pdev->dev);
	}
#endif

	switch (cmd) {
	case FBIOGET_FSCREENINFO:
		ret = memcpy(argp, &fb->fix, sizeof(fb->fix)) ? 0 : -EFAULT;
		break;

	case FBIOGET_VSCREENINFO:
		ret = memcpy(argp, &fb->var, sizeof(fb->var)) ? 0 : -EFAULT;
		break;

	case FBIOPUT_VSCREENINFO:
		ret = s3cfb_check_var((struct fb_var_screeninfo *)argp, fb);
		if (ret) {
			dev_err(fbdev->dev, "invalid vscreeninfo\n");
			break;
		}

		ret = memcpy(&fb->var, (struct fb_var_screeninfo *)argp,
			     sizeof(fb->var)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to put new vscreeninfo\n");
			break;
		}

		fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
		fix->smem_len = fix->line_length * var->yres_virtual;

		s3cfb_set_win_params(fbdev, id);
		break;

	case S3CFB_WIN_POSITION:
		ret = memcpy(&user_win, (struct s3cfb_user_window __user *)arg,
			     sizeof(user_win)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to S3CFB_WIN_POSITION\n");
			break;
		}

		if (user_win.x < 0)
			user_win.x = 0;

		if (user_win.y < 0)
			user_win.y = 0;

		if (user_win.x + var->xres > lcd->width)
			win->x = lcd->width - var->xres;
		else
			win->x = user_win.x;

		if (user_win.y + var->yres > lcd->height)
			win->y = lcd->height - var->yres;
		else
			win->y = user_win.y;

		s3cfb_set_window_position(fbdev, win->id);
		break;

	case S3CFB_GET_LCD_WIDTH:
		ret = memcpy(argp, &lcd->width, sizeof(int)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to S3CFB_GET_LCD_WIDTH\n");
			break;
		}

		break;

	case S3CFB_GET_LCD_HEIGHT:
		ret = memcpy(argp, &lcd->height, sizeof(int)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to S3CFB_GET_LCD_HEIGHT\n");
			break;
		}

		break;

	case S3CFB_SET_WRITEBACK:
		if ((u32)argp == 1)
			fbdev->output = OUTPUT_WB_RGB;
		else
			fbdev->output = OUTPUT_RGB;

		s3cfb_set_output(fbdev);

		break;

	case S3CFB_SET_WIN_ON:
		s3cfb_enable_window(fbdev, id);
		break;

	case S3CFB_SET_WIN_OFF:
		s3cfb_disable_window(fbdev, id);
		break;

	case S3CFB_SET_WIN_PATH:
		win->path = (enum s3cfb_data_path_t)argp;
		break;

	case S3CFB_SET_WIN_ADDR:
		fix->smem_start = (unsigned long)argp;
		s3cfb_set_buffer_address(fbdev, id);
		break;

	case S3CFB_SET_WIN_MEM:
		win->owner = (enum s3cfb_mem_owner_t)argp;
		break;

	case S3CFB_SET_VSYNC_INT:
		if (argp)
			s3cfb_set_global_interrupt(fbdev, 1);

		s3cfb_set_vsync_interrupt(fbdev, (int)argp);
		break;

	case S3CFB_GET_VSYNC_INT_STATUS:
		ret = s3cfb_get_vsync_interrupt(fbdev);
		break;

	default:
#ifdef CONFIG_EXYNOS_DEV_PD
		fbdev->system_state = POWER_ON;
#endif
		ret = s3cfb_ioctl(fb, cmd, arg);
#ifdef CONFIG_EXYNOS_DEV_PD
		fbdev->system_state = POWER_OFF;
#endif
		break;
	}

#ifdef CONFIG_EXYNOS_DEV_PD
	/* disable the power domain */
	if (fbdev->system_state == POWER_OFF)
		pm_runtime_put(&pdev->dev);
#endif

	return ret;
}
EXPORT_SYMBOL(s3cfb_direct_ioctl);

MODULE_AUTHOR("Jingoo Han <jg1.han@samsung.com>");
MODULE_AUTHOR("Jonghun, Han <jonghun.han@samsung.com>");
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Display Controller (FIMD) driver");
MODULE_LICENSE("GPL");
