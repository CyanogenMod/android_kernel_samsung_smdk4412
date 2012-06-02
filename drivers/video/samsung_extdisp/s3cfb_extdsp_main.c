/* linux/drivers/video/samsung/s3cfb_extdsp-main.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung Display Controller (FIMD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <plat/media.h>
#include <mach/media.h>
#include <mach/map.h>
#include "s3cfb_extdsp.h"
#ifdef CONFIG_BUSFREQ_OPP
#include <mach/dev.h>
#endif

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif

struct s3cfb_extdsp_extdsp_desc		*fbextdsp;

inline struct s3cfb_extdsp_global *get_extdsp_global(int id)
{
	struct s3cfb_extdsp_global *fbdev;

	fbdev = fbextdsp->fbdev[0];

	return fbdev;
}

int s3cfb_extdsp_register_framebuffer(struct s3cfb_extdsp_global *fbdev)
{
	int ret;

	/* on registering framebuffer, framebuffer of default window is registered at first. */
	ret = register_framebuffer(fbdev->fb[0]);
	if (ret) {
		dev_err(fbdev->dev, "failed to register	\
				framebuffer device\n");
		return -EINVAL;
	}
#ifndef CONFIG_FRAMEBUFFER_CONSOLE
	s3cfb_extdsp_check_var_window(fbdev, &fbdev->fb[0]->var,
			fbdev->fb[0]);
	s3cfb_extdsp_set_par_window(fbdev, fbdev->fb[0]);
#endif
	return 0;
}

#define to_extdsp_plat(d)		(to_platform_device(d)->dev.platform_data)

static int s3cfb_extdsp_probe(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = NULL;
	struct s3cfb_extdsp_global *fbdev[2];
	int ret = -1;

	fbextdsp = kzalloc(sizeof(struct s3cfb_extdsp_extdsp_desc), GFP_KERNEL);
	if (!fbextdsp)
		goto err;

	/* global structure */
	fbextdsp->fbdev[0] = kzalloc(sizeof(struct s3cfb_extdsp_global),
			GFP_KERNEL);
	fbdev[0] = fbextdsp->fbdev[0];
	if (!fbdev[0]) {
		dev_err(fbdev[0]->dev, "failed to allocate for "
				"global fb structure extdsp[%d]!\n", 0);
		goto err1;
	}

	fbdev[0]->dev = &pdev->dev;

	/* platform_data*/
	pdata = to_extdsp_plat(&pdev->dev);

	/* lcd setting */
	fbdev[0]->lcd = (struct s3cfb_extdsp_lcd *)pdata->lcd;

	/* hw setting */
	s3cfb_extdsp_init_global(fbdev[0]);

	fbdev[0]->system_state = POWER_ON;

	/* alloc fb_info */
	if (s3cfb_extdsp_alloc_framebuffer(fbdev[0], 0)) {
		dev_err(fbdev[0]->dev, "alloc error extdsp[%d]\n", 0);
		goto err2;
	}

	/* register fb_info */
	if (s3cfb_extdsp_register_framebuffer(fbdev[0])) {
		dev_err(fbdev[0]->dev, "register error extdsp[%d]\n", 0);
		goto err2;
	}

	/* disable display as default */
	s3cfb_extdsp_disable_window(fbdev[0], 0);

	s3cfb_extdsp_update_power_state(fbdev[0], 0,
			FB_BLANK_POWERDOWN);

#ifdef CONFIG_BUSFREQ_OPP
	/* To lock bus frequency in OPP mode */
	fbdev[0]->bus_dev = dev_get("exynos-busfreq");
#endif

#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
	fbdev[0]->early_suspend.suspend = s3cfb_extdsp_early_suspend;
	fbdev[0]->early_suspend.resume = s3cfb_extdsp_late_resume;
	fbdev[0]->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 5;

	register_early_suspend(&fbdev[0]->early_suspend);
#endif
#endif

	dev_info(fbdev[0]->dev, "registered successfully\n");
	return 0;

err2:
	kfree(fbextdsp->fbdev[0]);
err1:
	kfree(fbextdsp);
err:
	return ret;
}

static int s3cfb_extdsp_remove(struct platform_device *pdev)
{
	struct s3cfb_extdsp_window *win;
	struct fb_info *fb;
	struct s3cfb_extdsp_global *fbdev[2];
	int j;

	fbdev[0] = fbextdsp->fbdev[0];

#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&fbdev[0]->early_suspend);
#endif
#endif
	for (j = 0; j < 1; j++) {
		fb = fbdev[0]->fb[j];

		/* free if exists */
		if (fb) {
			win = fb->par;
			s3cfb_extdsp_unmap_default_video_memory(fbdev[0], fb);
			framebuffer_release(fb);
		}
	}

	kfree(fbdev[0]->fb);
	kfree(fbdev[0]);

	return 0;
}

#ifdef CONFIG_PM
#ifdef CONFIG_HAS_EARLYSUSPEND
void s3cfb_extdsp_early_suspend(struct early_suspend *h)
{
	struct s3cfb_extdsp_global *info = container_of(h, struct s3cfb_extdsp_global, early_suspend);
	struct s3cfb_extdsp_global *fbdev[2];

	printk("s3cfb_extdsp_early_suspend is called\n");

	info->system_state = POWER_OFF;

	fbdev[0] = fbextdsp->fbdev[0];
	return ;
}

void s3cfb_extdsp_late_resume(struct early_suspend *h)
{
	struct s3cfb_extdsp_global *info = container_of(h, struct s3cfb_extdsp_global, early_suspend);
	struct fb_info *fb;
	struct s3cfb_extdsp_window *win;
	struct s3cfb_extdsp_global *fbdev[2];
	int j;
/*
	printk("s3cfb_extdsp_late_resume is called\n");
*/
	dev_dbg(info->dev, "wake up from suspend\n");

	info->system_state = POWER_ON;

	fbdev[0] = fbextdsp->fbdev[0];

#if defined(CONFIG_FB_S5P_DUMMYLCD)
	max8698_ldo_enable_direct(MAX8698_LDO4);
#endif

	s3cfb_extdsp_init_global(fbdev[0]);

	for (j = 0; j < 1; j++) {
		fb = fbdev[0]->fb[j];
		win = fb->par;
		if (win->enabled) {
			s3cfb_extdsp_set_par(fb);
			s3cfb_extdsp_enable_window(fbdev[0], 0);
		}
	}

	info->system_state = POWER_ON;

	return;
}
#else /* else !CONFIG_HAS_EARLYSUSPEND */

int s3cfb_extdsp_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct s3cfb_extdsp_global *fbdev[2];

	fbdev[0] = fbextdsp->fbdev[0];

	return 0;
}

int s3cfb_extdsp_resume(struct platform_device *pdev)
{
	struct fb_info *fb;
	struct s3cfb_extdsp_window *win;
	struct s3cfb_extdsp_global *fbdev[2];
	int j;

	fbdev[0] = fbextdsp->fbdev[0];
	dev_dbg(fbdev[0]->dev, "wake up from suspend extdsp[%d]\n", 0);

	if (atomic_read(&fbdev[0]->enabled_win) > 0) {
		s3cfb_extdsp_init_global(fbdev[0]);

		for (j = 0; j < 1; j++) {
			fb = fbdev[0]->fb[j];
			win = fb->par;
			if (win->owner == DMA_MEM_FIMD) {
				if (win->enabled) {
					s3cfb_extdsp_enable_window(fbdev[0], 0);
				}
			}
		}
	}

	return 0;
}
#endif
#else
#define s3cfb_extdsp_suspend NULL
#define s3cfb_extdsp_resume NULL
#endif

static struct platform_driver s3cfb_extdsp_driver = {
	.probe		= s3cfb_extdsp_probe,
	.remove		= s3cfb_extdsp_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= s3cfb_extdsp_suspend,
	.resume		= s3cfb_extdsp_resume,
#endif
	.driver		= {
		.name	= S3CFB_EXTDSP_NAME,
		.owner	= THIS_MODULE,
	},
};

struct fb_ops s3cfb_extdsp_ops = {
	.owner		= THIS_MODULE,
	.fb_open	= s3cfb_extdsp_open,
	.fb_release	= s3cfb_extdsp_release,
	.fb_check_var	= s3cfb_extdsp_check_var,
	.fb_set_par	= s3cfb_extdsp_set_par,
	.fb_setcolreg	= s3cfb_extdsp_setcolreg,
	.fb_blank	= s3cfb_extdsp_blank,
	.fb_pan_display	= s3cfb_extdsp_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_cursor	= s3cfb_extdsp_cursor,
	.fb_ioctl	= s3cfb_extdsp_ioctl,
};

static int s3cfb_extdsp_register(void)
{
	return platform_driver_register(&s3cfb_extdsp_driver);
}

static void s3cfb_extdsp_unregister(void)
{
	platform_driver_unregister(&s3cfb_extdsp_driver);
}

#if defined(CONFIG_FB_EXYNOS_FIMD_MC) || defined(CONFIG_FB_EXYNOS_FIMD_MC_WB)
late_initcall(s3cfb_extdsp_register);
#else
module_init(s3cfb_extdsp_register);
#endif
module_exit(s3cfb_extdsp_unregister);

MODULE_AUTHOR("Jingoo Han <jg1.han@samsung.com>");
MODULE_AUTHOR("Jonghun, Han <jonghun.han@samsung.com>");
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Display Controller (FIMD) driver");
MODULE_LICENSE("GPL");
