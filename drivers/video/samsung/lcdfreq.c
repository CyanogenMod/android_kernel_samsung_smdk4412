/* linux/driver/video/samsung/lcdfreq.c
 *
 * EXYNOS4 - support LCD PixelClock change at runtime
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/reboot.h>
#include <linux/suspend.h>
#include <linux/cpufreq.h>
#include <linux/sysfs.h>
#include <linux/gcd.h>
#include <linux/clk.h>
#include <linux/spinlock.h>

#include <plat/clock.h>
#include <plat/clock-clksrc.h>
#include <plat/regs-fb-s5p.h>

#include "s3cfb.h"

#define LCDFREQ_LIMIT_HZ	40

enum lcdfreq_level_idx {
	LEVEL_NORMAL,
	LEVEL_LIMIT,
	LCDFREQ_LEVEL_END
};

struct lcdfreq_t {
	u32 level;
	u32 hz;
	u32 vclk;
	u32 cmu_clkdiv;
	u32 pixclock;
};

struct lcdfreq_info {
	struct lcdfreq_t	table[LCDFREQ_LEVEL_END];
	enum lcdfreq_level_idx	level;
	atomic_t		usage;
	struct mutex		lock;
	spinlock_t		slock;

	u32			enable;
	struct device		*dev;

	struct notifier_block	pm_noti;
	struct notifier_block	reboot_noti;

	struct delayed_work	work;

	struct early_suspend	early_suspend;
};

int get_div(struct s3cfb_global *ctrl)
{
	struct clksrc_clk *src_clk;
	u32 clkdiv;

	src_clk = container_of(ctrl->clock, struct clksrc_clk, clk);
	clkdiv = __raw_readl(src_clk->reg_div.reg);
	clkdiv &= 0xf;

	return clkdiv;
}

int set_div(struct s3cfb_global *ctrl, u32 div)
{
	struct lcdfreq_info *lcdfreq = ctrl->data;
	struct clksrc_clk *src_clk;
	unsigned long flags;
	u32 cfg, clkdiv, count = 1000000;
/*	unsigned long timeout = jiffies + msecs_to_jiffies(500); */

	src_clk = container_of(ctrl->clock, struct clksrc_clk, clk);

	do {
		spin_lock_irqsave(&lcdfreq->slock, flags);
		clkdiv = __raw_readl(src_clk->reg_div.reg);

		if ((clkdiv & 0xf) == div) {
			spin_unlock_irqrestore(&lcdfreq->slock, flags);
			return -EINVAL;
		}

		clkdiv &= ~(0xff);
		clkdiv |= (div << 4 | div);
		cfg = (readl(ctrl->ielcd_regs + S3C_VIDCON1) & S3C_VIDCON1_VSTATUS_MASK);
		if (cfg == S3C_VIDCON1_VSTATUS_ACTIVE) {
			cfg = (readl(ctrl->ielcd_regs + S3C_VIDCON1) & S3C_VIDCON1_VSTATUS_MASK);
			if (cfg == S3C_VIDCON1_VSTATUS_FRONT) {
				writel(clkdiv, src_clk->reg_div.reg);
				spin_unlock_irqrestore(&lcdfreq->slock, flags);
				dev_info(ctrl->dev, "\t%x, count=%d\n", __raw_readl(src_clk->reg_div.reg), 1000000-count);
				return 0;
			}
		}
		spin_unlock_irqrestore(&lcdfreq->slock, flags);
		count--;
	} while (count);
/*	} while (time_before(jiffies, timeout)); */

	dev_err(ctrl->dev, "%s fail, div=%d\n", __func__, div);

	return -EINVAL;
}

static int set_lcdfreq_div(struct device *dev, enum lcdfreq_level_idx level)
{
	struct fb_info *fb = dev_get_drvdata(dev);
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);

	struct lcdfreq_info *lcdfreq = fbdev->data;

	u32 div, ret;

	mutex_lock(&lcdfreq->lock);

	if (unlikely(fbdev->system_state == POWER_OFF || !lcdfreq->enable)) {
		dev_err(dev, "%s reject. %d, %d\n", __func__, fbdev->system_state, lcdfreq->enable);
		ret = -EINVAL;
		goto exit;
	}

	div = lcdfreq->table[level].cmu_clkdiv;

	ret = set_div(fbdev, div);

	if (ret) {
		dev_err(dev, "fail to change lcd freq\n");
		goto exit;
	}

	lcdfreq->level = level;

exit:
	mutex_unlock(&lcdfreq->lock);

	return ret;
}

static int lcdfreq_lock(struct device *dev)
{
	struct fb_info *fb = dev_get_drvdata(dev);
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	struct lcdfreq_info *lcdfreq = fbdev->data;

	int ret;

	if (!atomic_read(&lcdfreq->usage))
		ret = set_lcdfreq_div(dev, LEVEL_LIMIT);
	else {
		dev_err(dev, "lcd freq is already limit state\n");
		return -EINVAL;
	}

	if (!ret) {
		mutex_lock(&lcdfreq->lock);
		atomic_inc(&lcdfreq->usage);
		mutex_unlock(&lcdfreq->lock);
		schedule_delayed_work(&lcdfreq->work, 0);
	}

	return ret;
}

static int lcdfreq_lock_free(struct device *dev)
{
	struct fb_info *fb = dev_get_drvdata(dev);
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	struct lcdfreq_info *lcdfreq = fbdev->data;

	int ret;

	if (atomic_read(&lcdfreq->usage))
		ret = set_lcdfreq_div(dev, LEVEL_NORMAL);
	else {
		dev_err(dev, "lcd freq is already normal state\n");
		return -EINVAL;
	}

	if (!ret) {
		mutex_lock(&lcdfreq->lock);
		atomic_dec(&lcdfreq->usage);
		mutex_unlock(&lcdfreq->lock);
		cancel_delayed_work(&lcdfreq->work);
	}

	return ret;
}

int get_divider(struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	struct lcdfreq_info *lcdfreq = fbdev->data;
	struct clksrc_clk *sclk;
	struct clk *clk;
	u32 rate, reg, i;
	u8 fimd_div;

	sclk = container_of(fbdev->clock, struct clksrc_clk, clk);
	clk = clk_get_parent(clk_get_parent(fbdev->clock));
	rate = clk_get_rate(clk);

	lcdfreq->table[LEVEL_NORMAL].cmu_clkdiv =
		DIV_ROUND_CLOSEST(rate, lcdfreq->table[LEVEL_NORMAL].vclk);

	lcdfreq->table[LEVEL_LIMIT].cmu_clkdiv =
		DIV_ROUND_CLOSEST(rate, lcdfreq->table[LEVEL_LIMIT].vclk);

	fimd_div = gcd(lcdfreq->table[LEVEL_NORMAL].cmu_clkdiv, lcdfreq->table[LEVEL_LIMIT].cmu_clkdiv);

	lcdfreq->table[LEVEL_NORMAL].cmu_clkdiv /= fimd_div;
	lcdfreq->table[LEVEL_LIMIT].cmu_clkdiv /= fimd_div;

	dev_info(fb->dev, "%s rate is %d, fimd divider=%d\n", clk->name, rate, fimd_div);

	for (i = 0; i < LCDFREQ_LEVEL_END; i++) {
		lcdfreq->table[i].cmu_clkdiv--;
		dev_info(fb->dev, "%dHZ divider is %d\n",
		lcdfreq->table[i].hz, lcdfreq->table[i].cmu_clkdiv);
	}

	reg = (readl(fbdev->regs + S3C_VIDCON0) & (S3C_VIDCON0_CLKVAL_F(0xff))) >> 6;
	reg++;

	if (fimd_div != reg)
		return -EINVAL;

	reg = (readl(sclk->reg_div.reg)) >> sclk->reg_div.shift;
	reg &= 0xf;

	if (lcdfreq->table[LEVEL_NORMAL].cmu_clkdiv != reg)
		return -EINVAL;

	return 0;
}

static ssize_t level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fb_info *fb = dev_get_drvdata(dev);
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	struct lcdfreq_info *lcdfreq = fbdev->data;

	if (unlikely(fbdev->system_state == POWER_OFF || !lcdfreq->enable)) {
		dev_err(dev, "%s reject. %d, %d\n", __func__, fbdev->system_state, lcdfreq->enable);
		return -EINVAL;
	}

	return sprintf(buf, "%dHZ, div=%d\n", lcdfreq->table[lcdfreq->level].hz, get_div(fbdev));
}

static ssize_t level_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;
	int ret;

	ret = strict_strtoul(buf, 0, (unsigned long *)&value);

	dev_info(dev, "\t%s :: value=%d\n", __func__, value);

	if (value >= LCDFREQ_LEVEL_END)
		return -EINVAL;

	if (value)
		ret = lcdfreq_lock(dev);
	else
		ret = lcdfreq_lock_free(dev);

	if (ret) {
		dev_err(dev, "%s fail\n", __func__);
		return -EINVAL;
	}

	return count;
}

static ssize_t usage_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fb_info *fb = dev_get_drvdata(dev);
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	struct lcdfreq_info *lcdfreq = fbdev->data;

	return sprintf(buf, "%d\n", atomic_read(&lcdfreq->usage));
}

static DEVICE_ATTR(level, S_IRUGO|S_IWUSR, level_show, level_store);
static DEVICE_ATTR(usage, S_IRUGO, usage_show, NULL);

static struct attribute *lcdfreq_attributes[] = {
	&dev_attr_level.attr,
	&dev_attr_usage.attr,
	NULL,
};

static struct attribute_group lcdfreq_attr_group = {
	.name = "lcdfreq",
	.attrs = lcdfreq_attributes,
};

static void lcdfreq_early_suspend(struct early_suspend *h)
{
	struct lcdfreq_info *lcdfreq =
		container_of(h, struct lcdfreq_info, early_suspend);

	dev_info(lcdfreq->dev, "%s\n", __func__);

	mutex_lock(&lcdfreq->lock);
	lcdfreq->enable = false;
	lcdfreq->level = LEVEL_NORMAL;
	atomic_set(&lcdfreq->usage, 0);
	mutex_unlock(&lcdfreq->lock);

	return ;
}

static void lcdfreq_late_resume(struct early_suspend *h)
{
	struct lcdfreq_info *lcdfreq =
		container_of(h, struct lcdfreq_info, early_suspend);

	dev_info(lcdfreq->dev, "%s\n", __func__);

	mutex_lock(&lcdfreq->lock);
	lcdfreq->enable = true;
	mutex_unlock(&lcdfreq->lock);

	return;
}

static int lcdfreq_pm_notifier_event(struct notifier_block *this,
	unsigned long event, void *ptr)
{
	struct lcdfreq_info *lcdfreq =
		container_of(this, struct lcdfreq_info, pm_noti);

	dev_info(lcdfreq->dev, "%s :: event=%ld\n", __func__, event);

	switch (event) {
	case PM_SUSPEND_PREPARE:
		mutex_lock(&lcdfreq->lock);
		lcdfreq->enable = false;
		lcdfreq->level = LEVEL_NORMAL;
		atomic_set(&lcdfreq->usage, 0);
		mutex_unlock(&lcdfreq->lock);
		return NOTIFY_OK;
	case PM_POST_RESTORE:
	case PM_POST_SUSPEND:
		mutex_lock(&lcdfreq->lock);
		lcdfreq->enable = true;
		mutex_unlock(&lcdfreq->lock);
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static int lcdfreq_reboot_notify(struct notifier_block *this,
		unsigned long code, void *unused)
{
	struct lcdfreq_info *lcdfreq =
		container_of(this, struct lcdfreq_info, reboot_noti);

	mutex_lock(&lcdfreq->lock);
	lcdfreq->enable = false;
	lcdfreq->level = LEVEL_NORMAL;
	atomic_set(&lcdfreq->usage, 0);
	mutex_unlock(&lcdfreq->lock);

	dev_info(lcdfreq->dev, "%s\n", __func__);

	return NOTIFY_DONE;
}

static void lcdfreq_status_work(struct work_struct *work)
{
	struct lcdfreq_info *lcdfreq =
		container_of(work, struct lcdfreq_info, work.work);

	u32 hz = lcdfreq->table[lcdfreq->level].hz;

	cancel_delayed_work(&lcdfreq->work);

	dev_info(lcdfreq->dev, "\tHZ=%d, usage=%d\n", hz, atomic_read(&lcdfreq->usage));

	schedule_delayed_work(&lcdfreq->work, HZ*120);
}

int lcdfreq_init(struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;
	struct s3cfb_global *fbdev = get_fimd_global(win->id);
	struct s3cfb_lcd *lcd = fbdev->lcd;
	struct fb_var_screeninfo *var = &fb->var;

	struct lcdfreq_info *lcdfreq = NULL;
	u32 vclk = 0;
	int ret;

	lcdfreq = kzalloc(sizeof(struct lcdfreq_info), GFP_KERNEL);
	if (!lcdfreq) {
		pr_err("fail to allocate for lcdfreq\n");
		ret = -ENOMEM;
		goto err_1;
	}

	fbdev->data = lcdfreq;

	lcdfreq->dev = fb->dev;
	lcdfreq->level = LEVEL_NORMAL;

	vclk = (lcd->freq *
			(var->left_margin + var->right_margin
			+ var->hsync_len + var->xres) *
			(var->upper_margin + var->lower_margin
			+ var->vsync_len + var->yres));
	lcdfreq->table[LEVEL_NORMAL].level = LEVEL_NORMAL;
	lcdfreq->table[LEVEL_NORMAL].vclk = vclk;
	lcdfreq->table[LEVEL_NORMAL].pixclock = var->pixclock;
	lcdfreq->table[LEVEL_NORMAL].hz = lcd->freq;

	vclk = (LCDFREQ_LIMIT_HZ *
			(var->left_margin + var->right_margin
			+ var->hsync_len + var->xres) *
			(var->upper_margin + var->lower_margin
			+ var->vsync_len + var->yres));
	lcdfreq->table[LEVEL_LIMIT].level = LEVEL_LIMIT;
	lcdfreq->table[LEVEL_LIMIT].vclk = vclk;
	lcdfreq->table[LEVEL_LIMIT].pixclock = KHZ2PICOS(vclk/1000);
	lcdfreq->table[LEVEL_LIMIT].hz = LCDFREQ_LIMIT_HZ;

	ret = get_divider(fb);
	if (ret < 0) {
		pr_err("fail to init lcd freq switch");
		fbdev->data = NULL;
		goto err_1;
	}

	atomic_set(&lcdfreq->usage, 0);
	mutex_init(&lcdfreq->lock);
	spin_lock_init(&lcdfreq->slock);

	INIT_DELAYED_WORK_DEFERRABLE(&lcdfreq->work, lcdfreq_status_work);

	ret = sysfs_create_group(&fb->dev->kobj, &lcdfreq_attr_group);
	if (ret < 0) {
		pr_err("fail to add sysfs entries, %d\n", __LINE__);
		goto err_2;
	}

	lcdfreq->early_suspend.suspend = lcdfreq_early_suspend;
	lcdfreq->early_suspend.resume = lcdfreq_late_resume;
	lcdfreq->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING + 1;

	register_early_suspend(&lcdfreq->early_suspend);

	lcdfreq->pm_noti.notifier_call = lcdfreq_pm_notifier_event;
	lcdfreq->reboot_noti.notifier_call = lcdfreq_reboot_notify;

	if (register_reboot_notifier(&lcdfreq->reboot_noti)) {
		pr_err("fail to setup reboot notifier\n");
		goto err_3;
	}

	lcdfreq->enable = true;

	dev_info(lcdfreq->dev, "%s is done\n", __func__);

	return 0;

err_3:
	sysfs_remove_group(&fb->dev->kobj, &lcdfreq_attr_group);

err_2:
	kfree(lcdfreq);

err_1:
	return ret;

}

