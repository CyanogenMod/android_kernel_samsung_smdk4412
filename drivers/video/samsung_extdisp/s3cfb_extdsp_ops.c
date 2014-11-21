/* linux/drivers/video/samsung/s3cfb_extdsp-ops.c
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

#if defined(CONFIG_S5P_MEM_CMA)
#include <linux/cma.h>
#endif

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif

#ifdef CONFIG_BUSFREQ_OPP
#include <mach/dev.h>
#endif

#include "s3cfb_extdsp.h"
#define NOT_DEFAULT_WINDOW 99

int s3cfb_extdsp_enable_window(struct s3cfb_extdsp_global *fbdev, int id)
{
	struct s3cfb_extdsp_window *win = fbdev->fb[id]->par;

#ifdef CONFIG_BUSFREQ_OPP
	dev_lock(fbdev->bus_dev, fbdev->dev, 133133);
#endif
	if (!win->enabled)
		atomic_inc(&fbdev->enabled_win);

	win->enabled = 1;
	return 0;
}

int s3cfb_extdsp_disable_window(struct s3cfb_extdsp_global *fbdev, int id)
{
	struct s3cfb_extdsp_window *win = fbdev->fb[id]->par;

	if (win->enabled)
		atomic_dec(&fbdev->enabled_win);

#ifdef CONFIG_BUSFREQ_OPP
	dev_unlock(fbdev->bus_dev, fbdev->dev);
#endif

	win->enabled = 0;
	return 0;
}

int s3cfb_extdsp_update_power_state(struct s3cfb_extdsp_global *fbdev, int id,
		int state)
{
	struct s3cfb_extdsp_window *win = fbdev->fb[id]->par;

	win->power_state = state;
	return 0;
}

int s3cfb_extdsp_init_global(struct s3cfb_extdsp_global *fbdev)
{
	fbdev->output = OUTPUT_RGB;
	fbdev->rgb_mode = MODE_RGB_P;

	mutex_init(&fbdev->lock);
	return 0;
}

int s3cfb_extdsp_map_default_video_memory(struct s3cfb_extdsp_global *fbdev,
					struct fb_info *fb, int extdsp_id)
{
	struct fb_fix_screeninfo *fix = &fb->fix;

#ifdef CONFIG_S5P_MEM_CMA
	struct cma_info mem_info;
	int err;
#endif

#ifdef CONFIG_S5P_MEM_CMA
	err = cma_info(&mem_info, fbdev->dev, 0);
	if (ERR_PTR(err))
		return -ENOMEM;
	fix->smem_start = (dma_addr_t)cma_alloc
		(fbdev->dev, "extdsp", (size_t)PAGE_ALIGN(fix->smem_len), 0);
	fb->screen_base = cma_get_virt(fix->smem_start,
			PAGE_ALIGN(fix->smem_len), 1);
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
			"size: 0x%08x\n", 0,
			(unsigned int)fix->smem_start,
			(unsigned int)fb->screen_base, fix->smem_len);

	memset(fb->screen_base, 0, fix->smem_len);

	return 0;
}

int s3cfb_extdsp_unmap_default_video_memory(struct s3cfb_extdsp_global *fbdev,
					struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;

	if (fix->smem_start) {
		iounmap(fb->screen_base);

		fix->smem_start = 0;
		fix->smem_len = 0;
		dev_info(fbdev->dev, "[fb%d] video memory released\n", 0);
	}

	return 0;
}

int s3cfb_extdsp_check_var_window(struct s3cfb_extdsp_global *fbdev,
			struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct s3cfb_extdsp_window *win = fb->par;
	struct s3cfb_extdsp_lcd *lcd = fbdev->lcd;

	dev_dbg(fbdev->dev, "[fb%d] check_var\n", 0);

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

	if (var->xoffset > (var->xres_virtual - var->xres))
		var->xoffset = var->xres_virtual - var->xres;

	if (var->yoffset + var->yres > var->yres_virtual)
		var->yoffset = var->yres_virtual - var->yres;

	if (win->x + var->xres > lcd->width)
		win->x = lcd->width - var->xres;

	if (win->y + var->yres > lcd->height)
		win->y = lcd->height - var->yres;

	if (var->pixclock != fbdev->fb[0]->var.pixclock) {
		dev_info(fbdev->dev, "pixclk is changed from %d Hz to %d Hz\n",
			fbdev->fb[0]->var.pixclock, var->pixclock);
	}

	return 0;
}

int s3cfb_extdsp_check_var(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct s3cfb_extdsp_global *fbdev = get_extdsp_global(0);

	s3cfb_extdsp_check_var_window(fbdev, var, fb);

	return 0;
}

int s3cfb_extdsp_set_par_window(struct s3cfb_extdsp_global *fbdev, struct fb_info *fb)
{
	dev_dbg(fbdev->dev, "[fb%d] set_par\n", 0);

	return 0;
}

int s3cfb_extdsp_set_par(struct fb_info *fb)
{
	struct s3cfb_extdsp_global *fbdev = get_extdsp_global(0);

	s3cfb_extdsp_set_par_window(fbdev, fb);

	return 0;
}

int s3cfb_extdsp_init_fbinfo(struct s3cfb_extdsp_global *fbdev, int id)
{
	struct fb_info *fb = fbdev->fb[id];
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct fb_var_screeninfo *var = &fb->var;
	struct s3cfb_extdsp_window *win = fb->par;
	struct s3cfb_extdsp_lcd *lcd = fbdev->lcd;

	memset(win, 0, sizeof(struct s3cfb_extdsp_window));
	platform_set_drvdata(to_platform_device(fbdev->dev), fb);
	strcpy(fix->id, S3CFB_EXTDSP_NAME);

	/* extdsp specific */
	s3cfb_extdsp_update_power_state(fbdev, 0, FB_BLANK_POWERDOWN);

	/* fbinfo */
	fb->fbops = &s3cfb_extdsp_ops;
	fb->flags = FBINFO_FLAG_DEFAULT;
	fb->pseudo_palette = &win->pseudo_pal;
#if (CONFIG_FB_S5P_EXTDSP_NR_BUFFERS != 1)
	fix->xpanstep = 2;
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

	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres * CONFIG_FB_S5P_EXTDSP_NR_BUFFERS;

	var->bits_per_pixel = 16;
	var->xoffset = 0;
	var->yoffset = 0;
	var->width = 0;
	var->height = 0;
	var->transp.length = 0;

	fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
	fix->smem_len = fix->line_length * var->yres_virtual;

	var->nonstd = 0;
	var->activate = FB_ACTIVATE_NOW;
	var->vmode = FB_VMODE_NONINTERLACED;
	var->hsync_len = 0;
	var->vsync_len = 0;
	var->left_margin = 0;
	var->right_margin = 0;
	var->upper_margin = 0;
	var->lower_margin = 0;
	var->pixclock = 0;

	return 0;
}

int s3cfb_extdsp_alloc_framebuffer(struct s3cfb_extdsp_global *fbdev, int extdsp_id)
{
	int ret = 0;

	fbdev->fb = kmalloc(sizeof(struct fb_info *), GFP_KERNEL);
	if (!fbdev->fb) {
		dev_err(fbdev->dev, "not enough memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	fbdev->fb[0] = framebuffer_alloc(sizeof(struct s3cfb_extdsp_window),
			fbdev->dev);
	if (!fbdev->fb[0]) {
		dev_err(fbdev->dev, "not enough memory\n");
		ret = -ENOMEM;
		goto err_alloc_fb;
	}

	ret = s3cfb_extdsp_init_fbinfo(fbdev, 0);
	if (ret) {
		dev_err(fbdev->dev,
				"failed to allocate memory for fb%d\n", 0);
		ret = -ENOMEM;
		goto err_alloc_fb;
	}

#if 0
	if (s3cfb_extdsp_map_default_video_memory(fbdev,
				fbdev->fb[0], extdsp_id)) {
		dev_err(fbdev->dev,
				"failed to map video memory "
				"for default window (%d)\n", 0);
		ret = -ENOMEM;
		goto err_alloc_fb;
	}
#endif

	return 0;

err_alloc_fb:
	if (fbdev->fb[0])
		framebuffer_release(fbdev->fb[0]);
	kfree(fbdev->fb);

err_alloc:
	return ret;
}

int s3cfb_extdsp_open(struct fb_info *fb, int user)
{
	struct s3cfb_extdsp_window *win = fb->par;
	struct s3cfb_extdsp_global *fbdev = get_extdsp_global(0);
	int ret = 0;

	mutex_lock(&fbdev->lock);

	if (atomic_read(&win->in_use)) {
		dev_dbg(fbdev->dev, "mutiple open fir default window\n");
		ret = 0;
	} else {
		atomic_inc(&win->in_use);
	}

	mutex_unlock(&fbdev->lock);

	return ret;
}

int s3cfb_extdsp_release_window(struct fb_info *fb)
{
	struct s3cfb_extdsp_window *win = fb->par;

	win->x = 0;
	win->y = 0;

	return 0;
}

int s3cfb_extdsp_release(struct fb_info *fb, int user)
{
	struct s3cfb_extdsp_window *win = fb->par;
	struct s3cfb_extdsp_global *fbdev = get_extdsp_global(0);

	s3cfb_extdsp_release_window(fb);

	mutex_lock(&fbdev->lock);
	atomic_dec(&win->in_use);
	mutex_unlock(&fbdev->lock);

	return 0;
}

inline unsigned int __chan_to_field_extdsp(unsigned int chan, struct fb_bitfield bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf.length;

	return chan << bf.offset;
}

int s3cfb_extdsp_setcolreg(unsigned int regno, unsigned int red,
			unsigned int green, unsigned int blue,
			unsigned int transp, struct fb_info *fb)
{
	unsigned int *pal = (unsigned int *)fb->pseudo_palette;
	unsigned int val = 0;

	if (regno < 16) {
		/* fake palette of 16 colors */
		val |= __chan_to_field_extdsp(red, fb->var.red);
		val |= __chan_to_field_extdsp(green, fb->var.green);
		val |= __chan_to_field_extdsp(blue, fb->var.blue);
		val |= __chan_to_field_extdsp(transp, fb->var.transp);
		pal[regno] = val;
	}

	return 0;
}

int s3cfb_extdsp_blank(int blank_mode, struct fb_info *fb)
{
	struct s3cfb_extdsp_window *win = fb->par;
	struct s3cfb_extdsp_window *tmp_win;
	struct s3cfb_extdsp_global *fbdev = get_extdsp_global(0);
	int enabled_win = 0;
	int i;

	dev_dbg(fbdev->dev, "change blank mode\n");

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		if (!fb->fix.smem_start) {
			dev_info(fbdev->dev, "[fb%d] no allocated memory \
				for unblank\n", 0);
			break;
		}

		if (win->power_state == FB_BLANK_UNBLANK) {
			dev_info(fbdev->dev, "[fb%d] already in \
				FB_BLANK_UNBLANK\n", 0);
			break;
		} else {
			s3cfb_extdsp_update_power_state(fbdev, 0,
						FB_BLANK_UNBLANK);
		}

		enabled_win = atomic_read(&fbdev->enabled_win);
		if (enabled_win == 0) {
			s3cfb_extdsp_init_global(fbdev);
				for (i = 0; i < 1; i++) {
					tmp_win = fbdev->fb[i]->par;
				}
		}

		if (!win->enabled)	/* from FB_BLANK_NORMAL */
			s3cfb_extdsp_enable_window(fbdev, 0);
		break;

	case FB_BLANK_NORMAL:
		if (win->power_state == FB_BLANK_NORMAL) {
			dev_info(fbdev->dev, "[fb%d] already in FB_BLANK_NORMAL\n", 0);
			break;
		} else {
			s3cfb_extdsp_update_power_state(fbdev, 0,
						FB_BLANK_NORMAL);
		}

		enabled_win = atomic_read(&fbdev->enabled_win);
		if (enabled_win == 0) {
			s3cfb_extdsp_init_global(fbdev);

			for (i = 0; i < 1; i++) {
				tmp_win = fbdev->fb[i]->par;
			}
		}

		if (!win->enabled)	/* from FB_BLANK_POWERDOWN */
			s3cfb_extdsp_enable_window(fbdev, 0);
		break;

	case FB_BLANK_POWERDOWN:
		if (win->power_state == FB_BLANK_POWERDOWN) {
			dev_info(fbdev->dev, "[fb%d] already in FB_BLANK_POWERDOWN\n", 0);
			break;
		} else {
			s3cfb_extdsp_update_power_state(fbdev, 0,
						FB_BLANK_POWERDOWN);
		}

		s3cfb_extdsp_disable_window(fbdev, 0);
		break;

	case FB_BLANK_VSYNC_SUSPEND:	/* fall through */
	case FB_BLANK_HSYNC_SUSPEND:	/* fall through */
	default:
		dev_dbg(fbdev->dev, "unsupported blank mode\n");
		return -EINVAL;
	}

	return NOT_DEFAULT_WINDOW;
}

int s3cfb_extdsp_pan_display(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct s3cfb_extdsp_global *fbdev = get_extdsp_global(0);

	if (var->yoffset + var->yres > var->yres_virtual) {
		dev_err(fbdev->dev, "invalid yoffset value\n");
		return -EINVAL;
	}

	fb->var.yoffset = var->yoffset;

	dev_dbg(fbdev->dev, "[fb%d] yoffset for pan display: %d\n", 0,
		var->yoffset);

	return 0;
}

int s3cfb_extdsp_cursor(struct fb_info *fb, struct fb_cursor *cursor)
{
	/* nothing to do for removing cursor */
	return 0;
}

int s3cfb_extdsp_ioctl(struct fb_info *fb, unsigned int cmd, unsigned long arg)
{
	struct s3cfb_extdsp_global *fbdev = get_extdsp_global(0);
	struct fb_var_screeninfo *var = &fb->var;
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s3cfb_extdsp_window *win = fb->par;
	struct s3cfb_extdsp_lcd *lcd = fbdev->lcd;
	struct s3cfb_extdsp_time_stamp time_stamp;
	void *argp = (void *)arg;
	int ret = 0;
	dma_addr_t start_addr = 0;
	dma_addr_t cur_nrbuffer = 0;
	dma_addr_t next_nrbuffer = 0;
	int i;

	union {
		struct s3cfb_extdsp_user_window user_window;
		int vsync;
		int lock;
		struct s3cfb_extdsp_buf_list buf_list;
	} p;

	switch (cmd) {
	case S3CFB_EXTDSP_LOCK_BUFFER:
		if (get_user(p.lock, (int __user *)arg))
			ret = -EFAULT;
		else
			win->lock_status = p.lock;

		cur_nrbuffer = var->yoffset /
			(var->yres_virtual / CONFIG_FB_S5P_EXTDSP_NR_BUFFERS);
		win->lock_buf_idx = cur_nrbuffer;

		if (win->lock_status == 1) { /* Locked */
			win->lock_buf_offset = fix->smem_start +
				((var->xres_virtual * var->yoffset
				  + var->xoffset) * (var->bits_per_pixel / 8));
			for (i = 0; i < CONFIG_FB_S5P_EXTDSP_NR_BUFFERS; i++) {
				if (fbdev->buf_list[i].phys_addr ==
					(unsigned int)win->lock_buf_offset)
					fbdev->buf_list[i].buf_status =
						BUF_LOCKED;
			}
		} else { /* Unlocked */
			for (i = 0; i < CONFIG_FB_S5P_EXTDSP_NR_BUFFERS; i++) {
				if (fbdev->buf_list[i].phys_addr ==
					(unsigned int)win->lock_buf_offset)
					fbdev->buf_list[i].buf_status =
						BUF_FREE;
			}
			win->lock_buf_offset = 0;
		}
		break;
	case S3CFB_EXTDSP_LOCK_AND_GET_BUF:
		if (copy_from_user(&p.buf_list,
				   (struct s3cfb_extdsp_buf_list __user *)arg,
				   sizeof(p.buf_list)))
			return -EFAULT;

		if (p.buf_list.buf_status == BUF_LOCKED) { /* Locked */
			memset(&p.buf_list, 0, sizeof(p.buf_list));
			for (i = 0; i < CONFIG_FB_S5P_EXTDSP_NR_BUFFERS; i++) {
				if (fbdev->buf_list[i].buf_status ==
						BUF_ACTIVE) {
					fbdev->buf_list[i].buf_status =
						BUF_LOCKED;

					p.buf_list.phys_addr =
						fbdev->buf_list[i].phys_addr;
					p.buf_list.time_marker =
						fbdev->buf_list[i].time_marker;
					p.buf_list.buf_status =
						fbdev->buf_list[i].buf_status;
					break;
				}
			}
		} else if (p.buf_list.buf_status == BUF_FREE) { /* Unlocked */
			for (i = 0; i < CONFIG_FB_S5P_EXTDSP_NR_BUFFERS; i++) {
				if (fbdev->buf_list[i].phys_addr ==
						p.buf_list.phys_addr) {
					fbdev->buf_list[i].buf_status =
						BUF_FREE;
					break;
				}
			}
		}
		if (copy_to_user((void *)arg, &(p.buf_list),
					sizeof(unsigned int))) {
			dev_err(fbdev->dev, "copy_to_user error\n");
			return -EFAULT;
		}
		break;

	case S3CFB_EXTDSP_GET_LOCKED_BUFFER:
		if (copy_to_user((void *)arg, &(win->lock_buf_offset),
					sizeof(unsigned int))) {
			dev_err(fbdev->dev, "copy_to_user error\n");
			return -EFAULT;
		}
		break;

	case S3CFB_EXTDSP_GET_FREE_BUFFER:
		win->free_buf_offset = 0;
		for (i = 0; i < CONFIG_FB_S5P_EXTDSP_NR_BUFFERS; i++) {
			if (fbdev->buf_list[i].buf_status == BUF_FREE) {
				win->free_buf_offset =
					fbdev->buf_list[i].phys_addr;
				break;
			}
		}
		if (copy_to_user((void *)arg, &(win->free_buf_offset),
					sizeof(unsigned int))) {
			dev_err(fbdev->dev, "copy_to_user error\n");
			return -EFAULT;
		}
		break;

	case S3CFB_EXTDSP_GET_NEXT_INDEX:
		cur_nrbuffer = var->yoffset /
			(var->yres_virtual / CONFIG_FB_S5P_EXTDSP_NR_BUFFERS);

		if ((win->lock_status == 1) && /* Locked */
			(CONFIG_FB_S5P_EXTDSP_NR_BUFFERS > 2) &&
			(win->lock_buf_idx == ((cur_nrbuffer + 1)
				% CONFIG_FB_S5P_EXTDSP_NR_BUFFERS))) {
			next_nrbuffer = ((cur_nrbuffer + 2)
					% CONFIG_FB_S5P_EXTDSP_NR_BUFFERS);
		} else /* Unlocked */
			next_nrbuffer = ((cur_nrbuffer + 1)
					% CONFIG_FB_S5P_EXTDSP_NR_BUFFERS);

		if (copy_to_user((void *)arg, &next_nrbuffer,
					sizeof(unsigned int))) {
			dev_err(fbdev->dev, "copy_to_user error\n");
			return -EFAULT;
		}
		break;

	case S3CFB_EXTDSP_WIN_POSITION:
		if (copy_from_user(&p.user_window,
				   (struct s3cfb_extdsp_user_window __user *)arg,
				   sizeof(p.user_window)))
			ret = -EFAULT;
		else {
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
		}
		break;

	case S3CFB_EXTDSP_GET_FB_PHY_ADDR:
		if (win->power_state == FB_BLANK_POWERDOWN)
			start_addr = 0;
		else
			start_addr = fix->smem_start + ((var->xres_virtual *
					var->yoffset + var->xoffset) *
					(var->bits_per_pixel / 8));

		dev_dbg(fbdev->dev, "framebuffer_addr: 0x%08x\n", start_addr);

		if (copy_to_user((void *)arg, &start_addr, sizeof(unsigned int))) {
			dev_err(fbdev->dev, "copy_to_user error\n");
			return -EFAULT;
		}

		break;

	case S3CFB_EXTDSP_PUT_TIME_STAMP:
		if (copy_from_user(&time_stamp,
				   (struct s3cfb_extdsp_time_stamp __user *)arg,
				   sizeof(time_stamp))) {
			dev_err(fbdev->dev, "copy_from_user error\n");
			return -EFAULT;
		}
		do_gettimeofday(&time_stamp.time_marker);
		for(i = 0; i < CONFIG_FB_S5P_EXTDSP_NR_BUFFERS; i++) {
			if (fbdev->buf_list[i].phys_addr == time_stamp.phys_addr) {
				fbdev->buf_list[i].time_marker = time_stamp.time_marker;
				break;
			}
			if (!fbdev->buf_list[i].phys_addr) {
				fbdev->buf_list[i].phys_addr = time_stamp.phys_addr;
				fbdev->buf_list[i].time_marker = time_stamp.time_marker;
				break;
			}
			if (i == (CONFIG_FB_S5P_EXTDSP_NR_BUFFERS -1)) {
				fbdev->buf_list[0].phys_addr = time_stamp.phys_addr;
				fbdev->buf_list[0].time_marker = time_stamp.time_marker;
				for(i = 1; i < CONFIG_FB_S5P_EXTDSP_NR_BUFFERS; i++)
					fbdev->buf_list[i].phys_addr = 0;
				break;
			}
		}
		break;

	case S3CFB_EXTDSP_GET_TIME_STAMP:
		if (copy_from_user(&time_stamp,
				(struct s3cfb_extdsp_time_stamp __user *)arg,
				sizeof(time_stamp))) {
			dev_err(fbdev->dev, "copy_from_user error\n");
			return -EFAULT;
		}
		for(i = 0; i < CONFIG_FB_S5P_EXTDSP_NR_BUFFERS; i++) {
			if (fbdev->buf_list[i].phys_addr == time_stamp.phys_addr) {
				time_stamp.time_marker = fbdev->buf_list[i].time_marker;
				break;
			}
		}
		if (copy_to_user((void *)arg,
				   &time_stamp,
				   sizeof(time_stamp))) {
			dev_err(fbdev->dev, "copy_to_user error\n");
			return -EFAULT;
		}
		break;

	case S3CFB_EXTDSP_GET_LOCKED_NUMBER:
		fbdev->lock_cnt = 0;
		for (i = 0; i < CONFIG_FB_S5P_EXTDSP_NR_BUFFERS; i++) {
			if (fbdev->buf_list[i].buf_status == BUF_LOCKED)
				fbdev->lock_cnt++;
		}
		ret = memcpy(argp, &fbdev->lock_cnt, sizeof(fbdev->lock_cnt)) ? 0 : -EFAULT;
		break;

	case FBIOGET_FSCREENINFO:
		ret = memcpy(argp, &fb->fix, sizeof(fb->fix)) ? 0 : -EFAULT;
		break;

	case FBIOGET_VSCREENINFO:
		ret = memcpy(argp, &fb->var, sizeof(fb->var)) ? 0 : -EFAULT;
		break;

	case FBIOPUT_VSCREENINFO:
		ret = s3cfb_extdsp_check_var((struct fb_var_screeninfo *)argp, fb);
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
		break;

	case S3CFB_EXTDSP_GET_LCD_WIDTH:
		ret = memcpy(argp, &lcd->width, sizeof(int)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to S3CFB_EXTDSP_GET_LCD_WIDTH\n");
			break;
		}

		break;

	case S3CFB_EXTDSP_GET_LCD_HEIGHT:
		ret = memcpy(argp, &lcd->height, sizeof(int)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to S3CFB_EXTDSP_GET_LCD_HEIGHT\n");
			break;
		}

		break;

	case S3CFB_EXTDSP_SET_WIN_ON:
		s3cfb_extdsp_enable_window(fbdev, 0);
		break;

	case S3CFB_EXTDSP_SET_WIN_OFF:
		s3cfb_extdsp_disable_window(fbdev, 0);
		break;

	case S3CFB_EXTDSP_SET_WIN_ADDR:
		fix->smem_start = (unsigned long)argp;
		for (i = 0; i < CONFIG_FB_S5P_EXTDSP_NR_BUFFERS; i++) {
			if (fbdev->buf_list[i].phys_addr ==
					(unsigned int)fix->smem_start)
				fbdev->buf_list[i].buf_status = BUF_ACTIVE;
			else if (fbdev->buf_list[i].buf_status != BUF_LOCKED)
				fbdev->buf_list[i].buf_status = BUF_FREE;
		}
		break;

	case S3CFB_EXTDSP_GET_TZ_MODE:
		if (copy_to_user((unsigned int *)arg,
				   &fbdev->enabled_tz,
				   sizeof(unsigned int))) {
			dev_err(fbdev->dev,
				"failed to S3CFB_EXTDSP_GET_TZ_MODE: copy_to_user error\n");
			return -EFAULT;
		}
		break;

	case S3CFB_EXTDSP_SET_TZ_MODE:
		if (copy_from_user(&fbdev->enabled_tz,
				   (unsigned int *)arg,
				   sizeof(unsigned int))) {
			dev_err(fbdev->dev,
				"failed to S3CFB_EXTDSP_SET_TZ_MODE: copy_from_user error\n");
			return -EFAULT;
		}
		break;

	default:
		break;
	}

	return ret;
}

MODULE_AUTHOR("Jingoo Han <jg1.han@samsung.com>");
MODULE_AUTHOR("Jonghun, Han <jonghun.han@samsung.com>");
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Display Controller (FIMD) driver");
MODULE_LICENSE("GPL");
