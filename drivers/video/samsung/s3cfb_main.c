/* linux/drivers/video/samsung/s3cfb-main.c
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
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <linux/sw_sync.h>
#include <plat/clock.h>
#include <plat/media.h>
#include <mach/media.h>
#include <mach/map.h>
#include "s3cfb.h"

#ifdef CONFIG_BUSFREQ_OPP
#include <mach/dev.h>
#endif

#ifdef CONFIG_FB_S5P_MDNIE
#ifdef CONFIG_MACH_KONA
#include "s3cfb_mdnie_kona.h"
#include "mdnie_kona.h"
#else
#include "s3cfb_mdnie.h"
#include "mdnie.h"
#endif
#endif
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif

#if defined(CONFIG_MACH_U1_BD) && defined(CONFIG_TARGET_LOCALE_KOR)
#include <mach/regs-clock.h>
#include "boot_progressbar.h"
#define DISPLAY_BOOT_PROGRESS
#endif
#include <mach/regs-pmu.h>
#include <plat/regs-fb-s5p.h>

#ifdef CONFIG_FB_S5P_SYSMMU
#include <plat/s5p-sysmmu.h>
#endif

struct s3cfb_fimd_desc		*fbfimd;

struct s3cfb_global *get_fimd_global(int id)
{
	struct s3cfb_global *fbdev;

	if (id < 5)
		fbdev = fbfimd->fbdev[0];
	else
		fbdev = fbfimd->fbdev[1];

	return fbdev;
}

int s3cfb_vsync_status_check(void)
{
	struct s3cfb_global *fbdev[2];
	fbdev[0] = fbfimd->fbdev[0];

	if (fbdev[0]->regs != 0 && fbdev[0]->system_state == POWER_ON)
		return s3cfb_check_vsync_status(fbdev[0]);
	else
		return 0;
}

#if defined(CONFIG_FB_S5P_VSYNC_THREAD)
static void s3cfb_activate_vsync(struct s3cfb_global *fbdev)
{
	int prev_refcount;

	mutex_lock(&fbdev->vsync_info.irq_lock);
	prev_refcount = fbdev->vsync_info.irq_refcount++;
	if (!prev_refcount) {
		s3cfb_set_global_interrupt(fbdev, 1);
		s3cfb_set_vsync_interrupt(fbdev, 1);
	}

	mutex_unlock(&fbdev->vsync_info.irq_lock);
}

static void s3cfb_deactivate_vsync(struct s3cfb_global *fbdev)
{
	int new_refcount;

	mutex_lock(&fbdev->vsync_info.irq_lock);

	new_refcount = --fbdev->vsync_info.irq_refcount;
	WARN_ON(new_refcount < 0);
	if (!new_refcount) {
		s3cfb_set_global_interrupt(fbdev, 0);
		s3cfb_set_vsync_interrupt(fbdev, 0);
	}

	mutex_unlock(&fbdev->vsync_info.irq_lock);
}
#endif

static irqreturn_t s3cfb_irq_frame(int irq, void *dev_id)
{
	struct s3cfb_global *fbdev[2];
	fbdev[0] = fbfimd->fbdev[0];

	spin_lock(&fbdev[0]->vsync_slock);

	if (fbdev[0]->regs != 0)
		s3cfb_clear_interrupt(fbdev[0]);

#if defined(CONFIG_FB_S5P_VSYNC_THREAD)
	fbdev[0]->vsync_info.timestamp = ktime_get();
	wake_up_interruptible_all(&fbdev[0]->vsync_info.wait);
#endif

	fbdev[0]->wq_count++;
	wake_up(&fbdev[0]->wq);

	spin_unlock(&fbdev[0]->vsync_slock);

	return IRQ_HANDLED;
}

#ifdef CONFIG_FB_S5P_TRACE_UNDERRUN
static irqreturn_t s3cfb_irq_fifo(int irq, void *dev_id)
{
	struct s3cfb_global *fbdev[2];
	fbdev[0] = fbfimd->fbdev[0];

	if (fbdev[0]->regs != 0)
		s3cfb_clear_interrupt(fbdev[0]);

	return IRQ_HANDLED;
}
#endif

#if defined(CONFIG_FB_S5P_VSYNC_THREAD)
int s3cfb_set_vsync_int(struct fb_info *info, bool active)
{
	struct s3cfb_global *fbdev = fbfimd->fbdev[0];
	bool prev_active = fbdev->vsync_info.active;

	fbdev->vsync_info.active = active;

	if (active && !prev_active)
		s3cfb_activate_vsync(fbdev);
	else if (!active && prev_active)
		s3cfb_deactivate_vsync(fbdev);

	return 0;
}

/**
 * s3cfb_wait_for_vsync() - sleep until next VSYNC interrupt or timeout
 * @sfb: main hardware state
 * @timeout: timeout in msecs, or 0 to wait indefinitely.
 */
int s3cfb_wait_for_vsync(struct s3cfb_global *fbdev, u32 timeout)
{
	ktime_t timestamp;
	int ret;

	pm_runtime_get_sync(fbdev->dev);

	timestamp = fbdev->vsync_info.timestamp;
	s3cfb_activate_vsync(fbdev);
	if (timeout) {
		ret = wait_event_interruptible_timeout(fbdev->vsync_info.wait,
						!ktime_equal(timestamp,
						fbdev->vsync_info.timestamp),
						msecs_to_jiffies(timeout));
	} else {
		ret = wait_event_interruptible(fbdev->vsync_info.wait,
						!ktime_equal(timestamp,
						fbdev->vsync_info.timestamp));
	}
	s3cfb_deactivate_vsync(fbdev);

	pm_runtime_put_sync(fbdev->dev);

	if (timeout && ret == 0)
		return -ETIMEDOUT;

	return 0;
}
#endif

int s3cfb_register_framebuffer(struct s3cfb_global *fbdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	int ret, i, j;

	/* on registering framebuffer, framebuffer of default window is registered at first. */
	for (i = pdata->default_win; i < pdata->nr_wins + pdata->default_win; i++) {
		j = i % pdata->nr_wins;
		ret = register_framebuffer(fbdev->fb[j]);
		if (ret) {
			dev_err(fbdev->dev, "failed to register	\
				framebuffer device\n");
			goto err;
		}
#ifndef CONFIG_FRAMEBUFFER_CONSOLE
		if (j == pdata->default_win) {
			s3cfb_check_var_window(fbdev, &fbdev->fb[j]->var,
					fbdev->fb[j]);
			ret = s3cfb_set_par_window(fbdev, fbdev->fb[j]);
			if (ret != 0)
				BUG();
			s3cfb_draw_logo(fbdev->fb[j]);
		}
#endif
	}
	return 0;

err:
	while (--i >= pdata->default_win) {
		j = i % pdata->nr_wins;
		unregister_framebuffer(fbdev->fb[j]);
	}
	return -EINVAL;
}

void read_lcd_register(void)
{
	struct s3cfb_global *fbdev[1];
	u32 reg_val[156] = {0,};
	u32 reg;
	u32 i;

	fbdev[0] = fbfimd->fbdev[0];

	/*11C00000 ~ 11C00260*/
	reg = readl(fbdev[0]->regs_org + S3C_VIDCON1);
	dev_info(fbdev[0]->dev, "11C000%02X| %08X", S3C_VIDCON1, reg);
	for (i = 0; i < 156; i++)
		reg_val[i] = readl(fbdev[0]->regs_org + (i*4));

	for (i = 0; i < 39; i++)
		dev_info(fbdev[0]->dev, "11C0%04X| %08X %08X %08X %08X",
		(i*16), reg_val[i*4], reg_val[i*4+1], reg_val[i*4+2], reg_val[i*4+3]);

	/*11C040A0 ~ 11C04110*/
	for (i = 0; i < 32; i++)
		reg_val[i] = readl(fbdev[0]->regs_org + (0x40A0)+(i*4));

	for (i = 0; i < 8; i++)
		dev_info(fbdev[0]->dev, "11C0%04X| %08X %08X %08X %08X",
		(0x40A0)+(i*16), reg_val[i*4], reg_val[i*4+1], reg_val[i*4+2], reg_val[i*4+3]);
#ifdef CONFIG_FB_S5P_MDNIE
	reg = readl(fbdev[0]->ielcd_regs + S3C_VIDCON1);
	dev_info(fbdev[0]->dev, "11C400%02X| %08X", S3C_VIDCON1, reg);

	for (i = 0; i < 4; i++)
		reg_val[i] = readl(fbdev[0]->ielcd_regs + (i*4));

	for (i = 0; i < 1; i++)
		dev_info(fbdev[0]->dev, "11C400%02X| %08X %08X %08X %08X",
		(i*16), reg_val[i*4], reg_val[i*4+1], reg_val[i*4+2], reg_val[i*4+3]);
#endif
#ifdef CONFIG_FB_S5P_MIPI_DSIM
	reg = read_dsim_register(0);
	dev_info(fbdev[0]->dev, "11C80000| %08X", reg);
	for (i = 0; i < 4; i++)
		reg_val[i] = read_dsim_register(i);

	for (i = 0; i < 1; i++)
		dev_info(fbdev[0]->dev, "11C800%02X| %08X %08X %08X %08X",
		(i*16), reg_val[i*4], reg_val[i*4+1], reg_val[i*4+2], reg_val[i*4+3]);
#endif
}

static int s3cfb_sysfs_show_win_power(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);
	struct s3cfb_window *win;
	char temp[16];
	int i;
	struct s3cfb_global *fbdev[1];
	fbdev[0] = fbfimd->fbdev[0];

	for (i = 0; i < pdata->nr_wins; i++) {
		win = fbdev[0]->fb[i]->par;
		sprintf(temp, "[fb%d] %s\n", i, win->enabled ? "on" : "off");
		strcat(buf, temp);
	}

	return strlen(buf);
}

static int s3cfb_sysfs_store_win_power(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t len)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);
	char temp[4] = { 0, };
	const char *p = buf;
	int id, to;
	struct s3cfb_global *fbdev[1];
	fbdev[0] = fbfimd->fbdev[0];

	while (*p != '\0') {
		if (!isspace(*p))
			strncat(temp, p, 1);
		p++;
	}

	if (strlen(temp) != 2)
		return -EINVAL;

	id = simple_strtoul(temp, NULL, 10) / 10;
	to = simple_strtoul(temp, NULL, 10) % 10;

	if (id < 0 || id > pdata->nr_wins)
		return -EINVAL;

	if (to != 0 && to != 1)
		return -EINVAL;

	if (to == 0)
		s3cfb_disable_window(fbdev[0], id);
	else
		s3cfb_enable_window(fbdev[0], id);

	return len;
}

static DEVICE_ATTR(win_power, 0644,
	s3cfb_sysfs_show_win_power, s3cfb_sysfs_store_win_power);

static ssize_t ielcd_dump_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int reg_val, i;
	char temp[50];
	struct s3cfb_global *fbdev[1];

	fbdev[0] = fbfimd->fbdev[0];

	for (i = 0; i < 25; i++) {
		reg_val = readl(fbdev[0]->ielcd_regs + i*4);
		sprintf(temp, "[IELCD]0x11C4_00%02X = 0x%08X\n", (i*4), reg_val);
		strcat(buf, temp);
	}

	return strlen(buf);
}
static DEVICE_ATTR(ielcd_dump, 0444, ielcd_dump_show, NULL);

static ssize_t fimd_dump_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int reg_val, i;
	char temp[50];
	struct s3cfb_global *fbdev[1];

	fbdev[0] = fbfimd->fbdev[0];

	for (i = 0; i < 25; i++) {
		reg_val = readl(fbdev[0]->regs_org + i*4);
		sprintf(temp, "[FIMD]0x11C0_00%02X = 0x%08X\n", (i*4), reg_val);
		strcat(buf, temp);
	}

	return strlen(buf);
}
static DEVICE_ATTR(fimd_dump, 0444, fimd_dump_show, NULL);

#ifdef CONFIG_FB_S5P_VSYNC_SYSFS
static ssize_t s3c_fb_vsync_time(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    struct s3cfb_global *fbdev = fbfimd->fbdev[0];

    return snprintf(buf, PAGE_SIZE, "%llu", ktime_to_ns(fbdev->vsync_info.timestamp));
}

static DEVICE_ATTR(vsync_time, S_IRUGO, s3c_fb_vsync_time, NULL);
#endif

#if 0 /* def CONFIG_FB_S5P_MIPI_DSIM */
void s3cfb_display_on_remote(void)
{
	struct s3cfb_global *fbdev = fbfimd->fbdev[0];
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);

	if (pdata == NULL) {
		dev_err(fbdev->dev, "failed to get defualt window number.\n");
		return;
	}
	s3cfb_display_on(fbdev);
}
EXPORT_SYMBOL(s3cfb_display_on_remote);

void s3cfb_trigger(void)
{
	struct s3cfb_global *fbdev = fbfimd->fbdev[0];
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);

	if (pdata == NULL) {
		dev_err(fbdev->dev, "failed to get defualt window number.\n");
		return;
	}
	s3cfb_set_trigger(fbdev);
}
EXPORT_SYMBOL(s3cfb_trigger);
#endif

#if defined(CONFIG_FB_S5P_VSYNC_THREAD)
static int s3cfb_wait_for_vsync_thread(void *data)
{
	struct s3cfb_global *fbdev = data;

	while (!kthread_should_stop()) {
		ktime_t timestamp = fbdev->vsync_info.timestamp;
		int ret = wait_event_interruptible_timeout(
						fbdev->vsync_info.wait,
						!ktime_equal(timestamp,
						fbdev->vsync_info.timestamp) &&
						fbdev->vsync_info.active,
						msecs_to_jiffies(VSYNC_TIMEOUT_MSEC));

		if (ret > 0) {
#if defined(CONFIG_FB_S5P_VSYNC_SEND_UEVENTS)
			char *envp[2];
			char buf[64];
			snprintf(buf, sizeof(buf), "VSYNC=%llu",
					ktime_to_ns(fbdev->vsync_info.timestamp));
			envp[0] = buf;
			envp[1] = NULL;
			kobject_uevent_env(&fbdev->dev->kobj, KOBJ_CHANGE,
							envp);
#endif
#if defined(CONFIG_FB_S5P_VSYNC_SYSFS)
            sysfs_notify(&fbdev->dev->kobj, NULL, "vsync_time");
#endif
		}
	}

	return 0;
}
#endif

static void s3c_fb_update_regs_handler(struct kthread_work *work)
{
	struct s3cfb_global *fbdev =
		container_of(work, struct s3cfb_global, update_regs_work);
	struct s3c_reg_data *data, *next;
	struct list_head saved_list;

	mutex_lock(&fbdev->update_regs_list_lock);
	saved_list = fbdev->update_regs_list;
	list_replace_init(&fbdev->update_regs_list, &saved_list);
	mutex_unlock(&fbdev->update_regs_list_lock);

	list_for_each_entry_safe(data, next, &saved_list, list) {
		s3c_fb_update_regs(fbdev, data);
		list_del(&data->list);
		kfree(data);
	}
}

static int s3cfb_probe(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = NULL;
	struct resource *res = NULL;
	struct s3cfb_global *fbdev[2];
	int ret = 0;
	int i = 0;

#ifdef CONFIG_EXYNOS_DEV_PD
	/* to use the runtime PM helper functions */
	pm_runtime_enable(&pdev->dev);
	/* enable the power domain */
	pm_runtime_get_sync(&pdev->dev);
#endif

	fbfimd = kzalloc(sizeof(struct s3cfb_fimd_desc), GFP_KERNEL);
	if (!fbfimd) {
		printk(KERN_ERR "failed to allocate for fimd fb descriptor\n");
		ret = -ENOMEM;
		goto err_fimd_desc;
	}

	if (FIMD_MAX == 2)
		fbfimd->dual = 1;
	else
		fbfimd->dual = 0;

	for (i = 0; i < FIMD_MAX; i++) {
		/* global structure */
		fbfimd->fbdev[i] = kzalloc(sizeof(struct s3cfb_global), GFP_KERNEL);
		fbdev[i] = fbfimd->fbdev[i];
		if (!fbdev[i]) {
			dev_err(fbdev[i]->dev, "failed to allocate for	\
				global fb structure fimd[%d]!\n", i);
				ret = -ENOMEM;
			goto err0;
		}

		fbdev[i]->dev = &pdev->dev;

#if defined(CONFIG_MACH_SMDK4X12) || defined(CONFIG_FB_S5P_AMS369FG06)
		s3cfb_set_lcd_info(fbdev[i]);
#endif
		/* platform_data*/
		pdata = to_fb_plat(&pdev->dev);

		if (pdata->lcd)
			fbdev[i]->lcd = (struct s3cfb_lcd *)pdata->lcd;

		if (pdata->cfg_gpio)
			pdata->cfg_gpio(pdev);

		if (pdata->clk_on)
			pdata->clk_on(pdev, &fbdev[i]->clock);

		/* io memory */
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			dev_err(fbdev[i]->dev,
				"failed to get io memory region\n");
			ret = -EINVAL;
			goto err1;
		}
		res = request_mem_region(res->start,
					res->end - res->start + 1, pdev->name);
		if (!res) {
			dev_err(fbdev[i]->dev,
				"failed to request io memory region\n");
			ret = -EINVAL;
			goto err1;
		}
		fbdev[i]->regs = ioremap(res->start, res->end - res->start + 1);
		fbdev[i]->regs_org = fbdev[i]->regs;
		if (!fbdev[i]->regs) {
			dev_err(fbdev[i]->dev, "failed to remap io region\n");
			ret = -EINVAL;
			goto err_ioremap;
		}

		spin_lock_init(&fbdev[i]->vsync_slock);

#if defined(CONFIG_FB_S5P_VSYNC_THREAD)
		INIT_LIST_HEAD(&fbdev[i]->update_regs_list);
		mutex_init(&fbdev[i]->update_regs_list_lock);
		init_kthread_worker(&fbdev[i]->update_regs_worker);

		fbdev[i]->update_regs_thread = kthread_run(kthread_worker_fn,
				&fbdev[i]->update_regs_worker, "s3c-fb");
		if (IS_ERR(fbdev[i]->update_regs_thread)) {
			int err = PTR_ERR(fbdev[i]->update_regs_thread);
			fbdev[i]->update_regs_thread = NULL;

			dev_err(fbdev[i]->dev, "failed to run update_regs thread\n");
			return err;
		}
		init_kthread_work(&fbdev[i]->update_regs_work, s3c_fb_update_regs_handler);
		fbdev[i]->timeline = sw_sync_timeline_create("s3c-fb");
		fbdev[i]->timeline_max = 0;
#endif

		/* irq */
		fbdev[i]->irq = platform_get_irq(pdev, 0);
		if (request_irq(fbdev[i]->irq, s3cfb_irq_frame, IRQF_SHARED,
				pdev->name, fbdev[i])) {
			dev_err(fbdev[i]->dev, "request_irq failed\n");
			ret = -EINVAL;
			goto err2;
		}

#ifdef CONFIG_FB_S5P_TRACE_UNDERRUN
		if (request_irq(platform_get_irq(pdev, 1), s3cfb_irq_fifo,
				IRQF_DISABLED, pdev->name, fbdev[i])) {
			dev_err(fbdev[i]->dev, "request_irq failed\n");
			ret = -EINVAL;
			goto err2;
		}

		s3cfb_set_fifo_interrupt(fbdev[i], 1);
		dev_info(fbdev[i]->dev, "fifo underrun trace\n");
#endif
#ifdef CONFIG_FB_S5P_MDNIE
		/*  only FIMD0 is supported */
		if (i == 0)
#ifdef CONFIG_MACH_KONA
			mdnie_setup();
#else
			s3c_mdnie_setup();
#endif
#endif
		/* hw setting */
		s3cfb_init_global(fbdev[i]);

		fbdev[i]->system_state = POWER_ON;

		spin_lock_init(&fbdev[i]->slock);

		/* alloc fb_info */
		if (s3cfb_alloc_framebuffer(fbdev[i], i)) {
			dev_err(fbdev[i]->dev, "alloc error fimd[%d]\n", i);
			ret = -ENOMEM;
			goto err3;
		}

		/* register fb_info */
		if (s3cfb_register_framebuffer(fbdev[i])) {
			dev_err(fbdev[i]->dev, "register error fimd[%d]\n", i);
			ret = -EINVAL;
			goto err3;
		}

		/* enable display */
		s3cfb_set_clock(fbdev[i]);
#ifdef CONFIG_FB_S5P_MDNIE
		/*  only FIMD0 is supported */
		if (i == 0) {
			if (pdata->set_display_path)
				pdata->set_display_path();

			s3cfb_set_dualrgb(fbdev[i], S3C_DUALRGB_MDNIE);
#ifdef CONFIG_MACH_KONA
			mdnie_display_on(fbdev[i]);
#else
			s3c_mdnie_init_global(fbdev[i]);
			s3c_mdnie_display_on(fbdev[i]);
#endif
		}
#endif
		s3cfb_enable_window(fbdev[0], pdata->default_win);

		s3cfb_update_power_state(fbdev[i], pdata->default_win,
					FB_BLANK_UNBLANK);

		/* Set alpha value width to 8-bit */
		s3cfb_set_alpha_value_width(fbdev[i], i);

		s3cfb_display_on(fbdev[i]);

#if defined(CONFIG_CPU_EXYNOS4212) || defined(CONFIG_CPU_EXYNOS4412)
#ifdef CONFIG_BUSFREQ_OPP
		/* To lock bus frequency in OPP mode */
		fbdev[i]->bus_dev = dev_get("exynos-busfreq");
#endif
#endif

#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
		fbdev[i]->early_suspend.suspend = s3cfb_early_suspend;
		fbdev[i]->early_suspend.resume = s3cfb_late_resume;
		fbdev[i]->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;

		register_early_suspend(&fbdev[i]->early_suspend);
#endif
#endif
#if defined(CONFIG_FB_S5P_VSYNC_THREAD)
		init_waitqueue_head(&fbdev[i]->vsync_info.wait);

		/* Create vsync thread */
		mutex_init(&fbdev[i]->vsync_info.irq_lock);

		fbdev[i]->vsync_info.thread = kthread_run(
						s3cfb_wait_for_vsync_thread,
						fbdev[i], "s3c-fb-vsync");
		if (fbdev[i]->vsync_info.thread == ERR_PTR(-ENOMEM)) {
			dev_err(fbdev[i]->dev, "failed to run vsync thread\n");
			fbdev[i]->vsync_info.thread = NULL;
		}
#endif
		ret = device_create_file(fbdev[i]->dev, &dev_attr_fimd_dump);
		if (ret < 0)
			dev_err(fbdev[0]->dev, "failed to add sysfs entries\n");

#ifdef CONFIG_FB_S5P_VSYNC_SYSFS
        ret = device_create_file(fbdev[i]->dev, &dev_attr_vsync_time);
        if (ret < 0)
            dev_err(fbdev[0]->dev, "failed to add sysfs entries\n");
#endif

		ret = device_create_file(fbdev[i]->dev, &dev_attr_ielcd_dump);
		if (ret < 0)
			dev_err(fbdev[0]->dev, "failed to add sysfs entries\n");
	}

#ifdef CONFIG_FB_S5P_LCD_INIT
	/* panel control */
	if (pdata->backlight_on)
		pdata->backlight_on(pdev);

	if (pdata->lcd_on)
		pdata->lcd_on(pdev);
#endif

	ret = device_create_file(&(pdev->dev), &dev_attr_win_power);
	if (ret < 0)
		dev_err(fbdev[0]->dev, "failed to add sysfs entries\n");

#ifdef DISPLAY_BOOT_PROGRESS
	if (!(readl(S5P_INFORM2)))
		s3cfb_start_progress(fbdev[0]->fb[pdata->default_win]);
#endif

#ifdef FEATURE_BUSFREQ_LOCK
	atomic_set(&fbdev[0]->busfreq_lock_cnt, 0);
#endif

#if defined(CONFIG_LCD_FREQ_SWITCH)
	lcdfreq_init(fbdev[0]->fb[pdata->default_win]);
#endif

	dev_info(fbdev[0]->dev, "registered successfully\n");

	return 0;

err3:
	for (i = 0; i < FIMD_MAX; i++)
		free_irq(fbdev[i]->irq, fbdev[i]);
err2:
	for (i = 0; i < FIMD_MAX; i++)
		iounmap(fbdev[i]->regs);

err_ioremap:
	release_mem_region(res->start, res->end - res->start + 1);

err1:
	for (i = 0; i < FIMD_MAX; i++) {
		pdata->clk_off(pdev, &fbdev[i]->clock);
		kfree(fbfimd->fbdev[i]);
	}
err0:
	kfree(fbfimd);
err_fimd_desc:
	return ret;
}

static int s3cfb_remove(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct s3cfb_window *win;
	struct fb_info *fb;
	struct s3cfb_global *fbdev[2];
	int i;
	int j;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];

#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
		unregister_early_suspend(&fbdev[i]->early_suspend);
#endif
#endif
		free_irq(fbdev[i]->irq, fbdev[i]);
		iounmap(fbdev[i]->regs);
		pdata->clk_off(pdev, &fbdev[i]->clock);

		for (j = 0; j < pdata->nr_wins; j++) {
			fb = fbdev[i]->fb[j];

			/* free if exists */
			if (fb) {
				win = fb->par;
				if (win->id == pdata->default_win)
					s3cfb_unmap_default_video_memory(fbdev[i], fb);
				else
					s3cfb_unmap_video_memory(fbdev[i], fb);

				s3cfb_set_buffer_address(fbdev[i], j);
				framebuffer_release(fb);
			}
		}
#if defined(CONFIG_FB_S5P_VSYNC_THREAD)
		if (fbdev[i]->vsync_info.thread)
			kthread_stop(fbdev[i]->vsync_info.thread);
#endif

		kfree(fbdev[i]->fb);
		kfree(fbdev[i]);
	}
#ifdef CONFIG_EXYNOS_DEV_PD
	/* disable the power domain */
	pm_runtime_put(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
#endif
	return 0;
}

void s3cfb_lcd0_power_domain_start(void)
{
	int timeout;
	writel(S5P_INT_LOCAL_PWR_EN, S5P_PMU_LCD0_CONF);

	/* Wait max 1ms */
	timeout = 1000;
	while ((readl(S5P_PMU_LCD0_CONF + 0x4) & S5P_INT_LOCAL_PWR_EN)\
		!= S5P_INT_LOCAL_PWR_EN) {
		if (timeout == 0) {
			printk(KERN_ERR "Power domain lcd0 enable failed.\n");
			break;
		}
		timeout--;
		udelay(1);
	}

	if (timeout == 0) {
		timeout = 1000;
		writel(0x1, S5P_PMU_LCD0_CONF + 0x8);
		writel(S5P_INT_LOCAL_PWR_EN, S5P_PMU_LCD0_CONF);
		while ((readl(S5P_PMU_LCD0_CONF + 0x4) & S5P_INT_LOCAL_PWR_EN)\
			!= S5P_INT_LOCAL_PWR_EN) {

			if (timeout == 0) {
				printk(KERN_ERR "Power domain lcd0 enable failed 2nd.\n");
				break;
			}

			timeout--;
			udelay(1);
		}
		writel(0x2, S5P_PMU_LCD0_CONF + 0x8);
	}
}

void s3cfb_lcd0_power_domain_stop(void)
{
	int timeout;

	writel(0, S5P_PMU_LCD0_CONF);

	/* Wait max 1ms */
	timeout = 1000;
	while (readl(S5P_PMU_LCD0_CONF + 0x4) & S5P_INT_LOCAL_PWR_EN) {
		if (timeout == 0) {
			printk(KERN_ERR "Power domain lcd0 disable failed.\n");
			break;
		}
		timeout--;
		udelay(1);
	}

	if (timeout == 0) {
		timeout = 1000;
		writel(0x1, S5P_PMU_LCD0_CONF + 0x8);
		writel(0, S5P_PMU_LCD0_CONF);
		while (readl(S5P_PMU_LCD0_CONF + 0x4) & S5P_INT_LOCAL_PWR_EN) {
			if (timeout == 0) {
				printk(KERN_ERR "Power domain lcd0 disable failed 2nd.\n");
				break;
			}
			timeout--;
			udelay(1);
		}
		writel(0x2, S5P_PMU_LCD0_CONF + 0x8);
	}
}

void s3cfb_lcd0_pmu_off(void)
{
	s3cfb_lcd0_power_domain_stop();
	usleep_range(5000, 5000);
	s3cfb_lcd0_power_domain_start();
	printk(KERN_WARNING "lcd0 pmu re_start!!!\n");
}

#ifdef CONFIG_PM
#ifdef CONFIG_HAS_EARLYSUSPEND
void (*lcd_early_suspend)(void);
void (*lcd_late_resume)(void);

void s3cfb_early_suspend(struct early_suspend *h)
{
	struct s3cfb_global *info = container_of(h, struct s3cfb_global, early_suspend);
	struct s3c_platform_fb *pdata = to_fb_plat(info->dev);
	struct platform_device *pdev = to_platform_device(info->dev);
	struct s3cfb_global *fbdev[2];
	int i, ret;

	printk(KERN_INFO "+%s\n", __func__);

#ifdef CONFIG_FB_S5P_MIPI_DSIM
	if (lcd_early_suspend)
		lcd_early_suspend();
#endif

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];

		if (pdata->backlight_off)
			pdata->backlight_off(pdev);

		if (pdata->lcd_off)
			pdata->lcd_off(pdev);

		/* Disable Vsync */
		s3cfb_set_global_interrupt(fbdev[i], 0);
		s3cfb_set_vsync_interrupt(fbdev[i], 0);

#ifdef CONFIG_FB_S5P_AMS369FG06
		ams369fg06_ldi_disable();
#elif defined(CONFIG_FB_S5P_LMS501KF03)
		lms501kf03_ldi_disable();
#endif
		ret = s3cfb_display_off(fbdev[i]);

#ifdef CONFIG_FB_S5P_MDNIE
#ifdef CONFIG_MACH_KONA
		ret += mdnie_display_off();
#else
		ret += s3c_mdnie_display_off();
#endif
#endif

		if (ret > 0)
			s3cfb_lcd0_pmu_off();

		info->system_state = POWER_OFF;

		if (fbdev[i]->regs) {
			fbdev[i]->regs_org = fbdev[i]->regs;
			spin_lock(&fbdev[i]->slock);
			fbdev[i]->regs = 0;
			spin_unlock(&fbdev[i]->slock);
		}

		if (pdata->clk_off)
			pdata->clk_off(pdev, &fbdev[i]->clock);
	}

#if defined(CONFIG_FB_S5P_S6C1372) || defined(CONFIG_FB_S5P_S6F1202A)
	s5c1372_ldi_disable();
#endif

#ifdef CONFIG_FB_S5P_MIPI_DSIM
	s5p_dsim_early_suspend();
#endif
#ifdef CONFIG_EXYNOS_DEV_PD
	/* disable the power domain */
	printk(KERN_DEBUG "s3cfb - disable power domain\n");
	pm_runtime_put_sync(&pdev->dev);
#endif

#ifdef CONFIG_FB_S5P_SYSMMU
	if (fbdev[0]->sysmmu.enabled == true) {
		fbdev[0]->sysmmu.enabled = false;
		fbdev[0]->sysmmu.pgd = 0;
		s5p_sysmmu_disable(fbdev[0]->dev);
	}
#endif

	printk(KERN_INFO "-%s\n", __func__);
	return ;
}

void s3cfb_late_resume(struct early_suspend *h)
{
	struct s3cfb_global *info = container_of(h, struct s3cfb_global, early_suspend);
	struct s3c_platform_fb *pdata = to_fb_plat(info->dev);
	struct fb_info *fb;
	struct s3cfb_window *win;
	struct s3cfb_global *fbdev[2];
	int i, j;
	struct platform_device *pdev = to_platform_device(info->dev);

	dev_info(info->dev, "+%s\n", __func__);

	dev_dbg(info->dev, "wake up from suspend\n");

#ifdef CONFIG_EXYNOS_DEV_PD
	/* enable the power domain */
	dev_dbg(info->dev, "s3cfb - enable power domain\n");
	pm_runtime_get_sync(&pdev->dev);
#endif

#ifdef CONFIG_FB_S5P_MIPI_DSIM
	s5p_dsim_late_resume();
#endif

#if defined(CONFIG_FB_S5P_DUMMYLCD)
		max8698_ldo_enable_direct(MAX8698_LDO4);
#endif
	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];

		if (pdata->clk_on)
			pdata->clk_on(pdev, &fbdev[i]->clock);

		if (fbdev[i]->regs_org)
			fbdev[i]->regs = fbdev[i]->regs_org;
		else
			/* fbdev[i]->regs_org should be non-zero value */
			BUG();

#if defined(CONFIG_FB_MDNIE_PWM)
		set_mdnie_pwm_value(g_mdnie, 0);
#endif

		if (pdata->set_display_path)
			pdata->set_display_path();

#ifdef CONFIG_FB_S5P_MDNIE
		s3cfb_set_dualrgb(fbdev[i], S3C_DUALRGB_MDNIE);
#endif

		info->system_state = POWER_ON;

		s3cfb_init_global(fbdev[i]);
		s3cfb_set_clock(fbdev[i]);
		/* Set Alpha value width to 8-bit alpha value
		 * 1 : 8bit mode
		 * 2 : 4bit mode
		 */
		s3cfb_set_alpha_value(fbdev[i], 1);
#ifdef CONFIG_FB_S5P_MDNIE
#if defined(CONFIG_FB_S5P_S6C1372) || defined(CONFIG_FB_S5P_S6F1202A)
		s5c1372_ldi_enable();
#endif
#ifdef CONFIG_MACH_KONA
		mdnie_display_on(fbdev[i]);
#else
		s3c_mdnie_init_global(fbdev[i]);
		set_mdnie_value(g_mdnie, 1);
		s3c_mdnie_display_on(fbdev[i]);
#endif
#endif
		s3cfb_display_on(fbdev[i]);

		/* Set alpha value width to 8-bit */
		s3cfb_set_alpha_value_width(fbdev[i], i);

#if defined(CONFIG_FB_RGBA_ORDER)
		s3cfb_set_output(fbdev[i]);
#else
		for (j = 0; j < pdata->nr_wins; j++) {
			fb = fbdev[i]->fb[j];
			win = fb->par;
			if ((win->path == DATA_PATH_DMA) && (win->enabled)) {
				s3cfb_set_par(fb);
				s3cfb_set_buffer_address(fbdev[i], win->id);
				s3cfb_enable_window(fbdev[i], win->id);
			}
		}
#endif

		if (pdata->cfg_gpio)
			pdata->cfg_gpio(pdev);

		if (pdata->lcd_on)
			pdata->lcd_on(pdev);

		if (info->lcd->init_ldi)
			fbdev[i]->lcd->init_ldi();

		if (pdata->backlight_on)
			pdata->backlight_on(pdev);

#if defined(CONFIG_FB_S5P_VSYNC_THREAD)
		mutex_lock(&fbdev[i]->vsync_info.irq_lock);
		if (fbdev[i]->vsync_info.irq_refcount) {
			s3cfb_set_global_interrupt(fbdev[i], 1);
			s3cfb_set_vsync_interrupt(fbdev[i], 1);
		}
		mutex_unlock(&fbdev[i]->vsync_info.irq_lock);
#endif
	}

#ifdef CONFIG_FB_S5P_MIPI_DSIM
	if (lcd_late_resume)
		lcd_late_resume();
#endif

	dev_info(info->dev, "-%s\n", __func__);

	return;
}
#else /* else !CONFIG_HAS_EARLYSUSPEND */

int s3cfb_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct s3cfb_global *fbdev[2];
	int i;
	u32 reg;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];

#ifdef CONFIG_FB_S5P_MDNIE
		writel(0, fbdev[i]->regs + 0x27c);
		msleep(20);
		reg = readl(S3C_VA_SYS + 0x0210);
		reg |= (1<<1);
		writel(reg, S3C_VA_SYS + 0x0210);
		s3c_mdnie_display_off();
#endif
		if (atomic_read(&fbdev[i]->enabled_win) > 0) {
			/* lcd_off and backlight_off isn't needed. */
			if (fbdev[i]->lcd->deinit_ldi)
				fbdev[i]->lcd->deinit_ldi();

			s3cfb_display_off(fbdev[i]);
			if (fbdev[i]->regs) {
				fbdev[i]->regs_org = fbdev[i]->regs;
				fbdev[i]->regs = 0;
			} else
				/* fbdev[i]->regs should be non-zero value */
				BUG();
			pdata->clk_off(pdev, &fbdev[i]->clock);
		}
#ifdef CONFIG_FB_S5P_MDNIE
		s3c_mdnie_off();
#endif
	}

	return 0;
}

int s3cfb_resume(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct fb_info *fb;
	struct s3cfb_window *win;
	struct s3cfb_global *fbdev[2];
	int i, j;
	u32 reg;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];
		dev_dbg(fbdev[i]->dev, "wake up from suspend fimd[%d]\n", i);

		if (pdata->cfg_gpio)
			pdata->cfg_gpio(pdev);

		if (pdata->backlight_on)
			pdata->backlight_on(pdev);
		if (pdata->lcd_on)
			pdata->lcd_on(pdev);
		if (fbdev[i]->lcd->init_ldi)
			fbdev[i]->lcd->init_ldi();

		if (pdata->backlight_off)
			pdata->backlight_off(pdev);
		if (pdata->lcd_off)
			pdata->lcd_off(pdev);
		if (fbdev[i]->lcd->deinit_ldi)
			fbdev[i]->lcd->deinit_ldi();

		if (atomic_read(&fbdev[i]->enabled_win) > 0) {
#ifdef CONFIG_FB_S5P_MDNIE
			reg = readl(S3C_VA_SYS + 0x0210);
			reg &= ~(1<<13);
			reg &= ~(1<<12);
			reg &= ~(3<<10);
			reg |= (1<<0);
			reg &= ~(1<<1);
			writel(reg, S3C_VA_SYS + 0x0210);
			writel(3, fbdev[i]->regs + 0x27c);
#endif
			pdata->clk_on(pdev, &fbdev[i]->clock);
			if (fbdev[i]->regs_org)
				fbdev[i]->regs = fbdev[i]->regs_org;
			else
				/* fbdev[i]->regs should be non-zero value */
				BUG();

			s3cfb_init_global(fbdev[i]);
			s3cfb_set_clock(fbdev[i]);
#ifdef CONFIG_FB_S5P_MDNIE
			s3c_mdnie_init_global(fbdev[i]);
			s3c_mdnie_display_on(fbdev[i]);
#endif
			for (j = 0; j < pdata->nr_wins; j++) {
				fb = fbdev[i]->fb[j];
				win = fb->par;
				if (win->owner == DMA_MEM_FIMD) {
					s3cfb_set_win_params(fbdev[i], win->id);
					if (win->enabled) {
						if (win->power_state == FB_BLANK_NORMAL)
							s3cfb_win_map_on(fbdev[i], win->id, 0x0);

						s3cfb_enable_window(fbdev[i], win->id);
					}
				}
			}

			s3cfb_set_alpha_value_width(fbdev[i], i);

			s3cfb_display_on(fbdev[i]);

			if (pdata->backlight_on)
				pdata->backlight_on(pdev);

			if (pdata->lcd_on)
				pdata->lcd_on(pdev);

			if (fbdev[i]->lcd->init_ldi)
				fbdev[i]->lcd->init_ldi();
		}
	}

	return 0;
}
#endif
#else
#define s3cfb_suspend NULL
#define s3cfb_resume NULL
#endif

#ifdef CONFIG_EXYNOS_DEV_PD
static int s3cfb_runtime_suspend(struct device *dev)
{
	return 0;
}

static int s3cfb_runtime_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops s3cfb_pm_ops = {
	.runtime_suspend = s3cfb_runtime_suspend,
	.runtime_resume = s3cfb_runtime_resume,
};
#endif

static struct platform_driver s3cfb_driver = {
	.probe		= s3cfb_probe,
	.remove		= s3cfb_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= s3cfb_suspend,
	.resume		= s3cfb_resume,
#endif
	.driver		= {
		.name	= S3CFB_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_EXYNOS_DEV_PD
		.pm	= &s3cfb_pm_ops,
#endif
	},
};

struct fb_ops s3cfb_ops = {
	.owner		= THIS_MODULE,
	.fb_open	= s3cfb_open,
	.fb_release	= s3cfb_release,
	.fb_check_var	= s3cfb_check_var,
	.fb_set_par	= s3cfb_set_par,
	.fb_setcolreg	= s3cfb_setcolreg,
	.fb_blank	= s3cfb_blank,
	.fb_pan_display	= s3cfb_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_cursor	= s3cfb_cursor,
	.fb_ioctl	= s3cfb_ioctl,
};

static int s3cfb_register(void)
{
	return platform_driver_register(&s3cfb_driver);
}

static void s3cfb_unregister(void)
{
	platform_driver_unregister(&s3cfb_driver);
}

module_init(s3cfb_register);
module_exit(s3cfb_unregister);

MODULE_AUTHOR("Jingoo Han <jg1.han@samsung.com>");
MODULE_AUTHOR("Jonghun, Han <jonghun.han@samsung.com>");
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Display Controller (FIMD) driver");
MODULE_LICENSE("GPL");
