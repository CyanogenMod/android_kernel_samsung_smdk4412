/* linux/drivers/video/s5p_mipi_dsi.c
 *
 * Samsung SoC MIPI-DSIM driver.
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * InKi Dae, <inki.dae@samsung.com>
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
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/memory.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>

#include <plat/fb.h>
#include <plat/mipi_dsim2.h>

#include "s5p_mipi_dsi_common.h"
#include "s5p_mipi_dsi_lowlevel.h"

#define master_to_driver(a)	(a->dsim_lcd_drv)
#define master_to_device(a)	(a->dsim_lcd_dev)
#define dev_to_dsim(a)		platform_get_drvdata(to_platform_device(a))

struct mipi_dsim_ddi {
	int				bus_id;
	struct list_head		list;
	struct mipi_dsim_lcd_device	*dsim_lcd_dev;
	struct mipi_dsim_lcd_driver	*dsim_lcd_drv;
};

static LIST_HEAD(dsim_ddi_list);

static DEFINE_MUTEX(mipi_dsim_lock);

static struct s5p_platform_mipi_dsim *to_dsim_plat(struct platform_device *pdev)
{
	return (struct s5p_platform_mipi_dsim *)pdev->dev.platform_data;
}

static int s5p_mipi_regulator_enable(struct mipi_dsim_device *dsim)
{
	int ret = 0;

	mutex_lock(&dsim->lock);
	if (dsim->reg_vdd10) {
		ret = regulator_enable(dsim->reg_vdd10);
		if (ret < 0) {
			dev_err(dsim->dev, "failed to enable regulator.\n");
			goto err_vdd10;
		}
	}
	if (dsim->reg_vdd18) {
		ret = regulator_enable(dsim->reg_vdd18);
		if (ret < 0) {
			dev_err(dsim->dev, "failed to enable regulator.\n");
			goto err_vdd18;
		}
	}

	mutex_unlock(&dsim->lock);
	return ret;

err_vdd18:
	ret = regulator_disable(dsim->reg_vdd10);
	if (ret < 0)
		dev_err(dsim->dev, "failed to disable regulator.\n");
err_vdd10:
	mutex_unlock(&dsim->lock);
	return ret;
}

static int s5p_mipi_regulator_disable(struct mipi_dsim_device *dsim)
{
	int ret = 0;

	mutex_lock(&dsim->lock);
	if (dsim->reg_vdd18) {
		ret = regulator_disable(dsim->reg_vdd18);
		if (ret < 0) {
			dev_err(dsim->dev, "failed to disable regulator.\n");
			goto err_vdd18;
		}
	}

	if (dsim->reg_vdd10) {
		ret = regulator_disable(dsim->reg_vdd10);
		if (ret < 0) {
			dev_err(dsim->dev, "failed to disable regulator.\n");
			goto err_vdd10;
		}
	}

	mutex_unlock(&dsim->lock);
	return ret;

err_vdd10:
	ret = regulator_enable(dsim->reg_vdd18);
	if (ret < 0)
		dev_err(dsim->dev, "failed to enable regulator.\n");
err_vdd18:
	mutex_unlock(&dsim->lock);
	return ret;
}

/* update all register settings to MIPI DSI controller. */
static void s5p_mipi_update_cfg(struct mipi_dsim_device *dsim)
{
	/*
	 * data from Display controller(FIMD) is not transferred in video mode
	 * but in case of command mode, all settings is not updated to
	 * registers.
	 */
	s5p_mipi_dsi_stand_by(dsim, 0);

	s5p_mipi_dsi_init_dsim(dsim);
	s5p_mipi_dsi_init_link(dsim);

	s5p_mipi_dsi_set_hs_enable(dsim);

	/* set display timing. */
	s5p_mipi_dsi_set_display_mode(dsim, dsim->dsim_config);

	s5p_mipi_dsi_init_interrupt(dsim);

	/*
	 * data from Display controller(FIMD) is transferred in video mode
	 * but in case of command mode, all settigs is updated to registers.
	 */
	s5p_mipi_dsi_stand_by(dsim, 1);
}

static int s5p_mipi_dsi_early_blank_mode(struct mipi_dsim_device *dsim,
		int power)
{
	switch (power) {
	case FB_BLANK_POWERDOWN:
		if (!dsim->suspended)
			pm_runtime_put_sync(dsim->dev);

		atomic_set(&dsim->early_blank_used_t, 1);
		break;
	default:
		break;
	}

	return 0;
}

static int s5p_mipi_dsi_blank_mode(struct mipi_dsim_device *dsim, int power)
{
	switch (power) {
	case FB_BLANK_UNBLANK:
		if (dsim->suspended)
			pm_runtime_get_sync(dsim->dev);

		atomic_set(&dsim->early_blank_used_t, 0);
		break;
	case FB_BLANK_NORMAL:
		/* TODO. */
		break;
	case FB_BLANK_POWERDOWN:
		/*
		 * if this function is called after early fb blank event,
		 * ignor it.
		 */
		if (atomic_read(&dsim->early_blank_used_t)) {
			atomic_set(&dsim->early_blank_used_t, 0);
			break;
		}

		if (!dsim->suspended)
			pm_runtime_put_sync(dsim->dev);
	default:
		break;
	}

	return 0;
}

int s5p_mipi_dsi_register_lcd_device(struct mipi_dsim_lcd_device *lcd_dev)
{
	struct mipi_dsim_ddi *dsim_ddi;

	if (!lcd_dev) {
		printk(KERN_ERR "mipi_dsim_lcd_device is NULL.\n");
		return -EFAULT;
	}

	if (!lcd_dev->name) {
		printk(KERN_ERR "dsim_lcd_device name is NULL.\n");
		return -EFAULT;
	}

	dsim_ddi = kzalloc(sizeof(struct mipi_dsim_ddi), GFP_KERNEL);
	if (!dsim_ddi) {
		printk(KERN_ERR "failed to allocate dsim_ddi object.\n");
		return -EFAULT;
	}

	dsim_ddi->dsim_lcd_dev = lcd_dev;

	mutex_lock(&mipi_dsim_lock);
	list_add_tail(&dsim_ddi->list, &dsim_ddi_list);
	mutex_unlock(&mipi_dsim_lock);

	return 0;
}

struct mipi_dsim_ddi
	*s5p_mipi_dsi_find_lcd_device(struct mipi_dsim_lcd_driver *lcd_drv)
{
	struct mipi_dsim_ddi *dsim_ddi;
	struct mipi_dsim_lcd_device *lcd_dev;

	mutex_lock(&mipi_dsim_lock);

	list_for_each_entry(dsim_ddi, &dsim_ddi_list, list) {
		if (!dsim_ddi)
			goto out;

		lcd_dev = dsim_ddi->dsim_lcd_dev;
		if (!lcd_dev)
			continue;

		if (lcd_drv->id >= 0) {
			if ((strcmp(lcd_drv->name, lcd_dev->name)) == 0 &&
					lcd_drv->id == lcd_dev->id) {
				/**
				 * bus_id would be used to identify
				 * connected bus.
				 */
				dsim_ddi->bus_id = lcd_dev->bus_id;
				mutex_unlock(&mipi_dsim_lock);

				return dsim_ddi;
			}
		} else {
			if ((strcmp(lcd_drv->name, lcd_dev->name)) == 0) {
				/**
				 * bus_id would be used to identify
				 * connected bus.
				 */
				dsim_ddi->bus_id = lcd_dev->bus_id;
				mutex_unlock(&mipi_dsim_lock);

				return dsim_ddi;
			}
		}

		kfree(dsim_ddi);
		list_del(&dsim_ddi->list);
	}

out:
	mutex_unlock(&mipi_dsim_lock);

	return NULL;
}

int s5p_mipi_dsi_register_lcd_driver(struct mipi_dsim_lcd_driver *lcd_drv)
{
	struct mipi_dsim_ddi *dsim_ddi;

	if (!lcd_drv) {
		printk(KERN_ERR "mipi_dsim_lcd_driver is NULL.\n");
		return -EFAULT;
	}

	if (!lcd_drv->name) {
		printk(KERN_ERR "dsim_lcd_driver name is NULL.\n");
		return -EFAULT;
	}

	dsim_ddi = s5p_mipi_dsi_find_lcd_device(lcd_drv);
	if (!dsim_ddi) {
		printk(KERN_ERR "mipi_dsim_ddi object not found.\n");
		return -EFAULT;
	}

	dsim_ddi->dsim_lcd_drv = lcd_drv;

	printk(KERN_INFO "registered panel driver(%s) to mipi-dsi driver.\n",
		lcd_drv->name);

	return 0;

}

struct mipi_dsim_ddi
	*s5p_mipi_dsi_bind_lcd_ddi(struct mipi_dsim_device *dsim,
			const char *name)
{
	struct mipi_dsim_ddi *dsim_ddi;
	struct mipi_dsim_lcd_driver *lcd_drv;
	struct mipi_dsim_lcd_device *lcd_dev;
	int ret;

	mutex_lock(&dsim->lock);

	list_for_each_entry(dsim_ddi, &dsim_ddi_list, list) {
		lcd_drv = dsim_ddi->dsim_lcd_drv;
		lcd_dev = dsim_ddi->dsim_lcd_dev;
		if (!lcd_drv || !lcd_dev ||
			(dsim->id != dsim_ddi->bus_id))
				continue;

		dev_dbg(dsim->dev, "lcd_drv->id = %d, lcd_dev->id = %d\n",
				lcd_drv->id, lcd_dev->id);
		dev_dbg(dsim->dev, "lcd_dev->bus_id = %d, dsim->id = %d\n",
				lcd_dev->bus_id, dsim->id);

		if ((strcmp(lcd_drv->name, name) == 0)) {
			lcd_dev->master = dsim;

			lcd_dev->dev.parent = dsim->dev;
			dev_set_name(&lcd_dev->dev, "%s", lcd_drv->name);

			ret = device_register(&lcd_dev->dev);
			if (ret < 0) {
				dev_err(dsim->dev,
					"can't register %s, status %d\n",
					dev_name(&lcd_dev->dev), ret);
				mutex_unlock(&dsim->lock);

				return NULL;
			}

			dsim->dsim_lcd_dev = lcd_dev;
			dsim->dsim_lcd_drv = lcd_drv;

			mutex_unlock(&dsim->lock);

			return dsim_ddi;
		}
	}

	mutex_unlock(&dsim->lock);

	return NULL;
}

/* define MIPI-DSI Master operations. */
static struct mipi_dsim_master_ops master_ops = {
	.cmd_read			= s5p_mipi_dsi_rd_data,
	.cmd_write			= s5p_mipi_dsi_wr_data,
	.get_dsim_frame_done		= s5p_mipi_dsi_get_frame_done_status,
	.clear_dsim_frame_done		= s5p_mipi_dsi_clear_frame_done,
	.set_early_blank_mode		= s5p_mipi_dsi_early_blank_mode,
	.set_blank_mode			= s5p_mipi_dsi_blank_mode,
#ifdef CONFIG_MACH_SLP_NAPLES
	.trigger				= s5p_mipi_dsi_trigger,
#endif
};

static int s5p_mipi_dsi_notifier(unsigned int val, void *data)
{
	struct mipi_dsim_device *dsim = (struct mipi_dsim_device *)data;
	int ret;

	ret = s5p_mipi_regulator_enable(dsim);
	if (ret < 0)
		return ret;

	ret = clk_enable(dsim->clock);
	if (ret < 0) {
		dev_err(dsim->dev, "failed to enable clock.\n");
		/* it doesn't need error check. */
		s5p_mipi_regulator_disable(dsim);
		return ret;
	}

	/* it doesn't need error check. */
	s5p_mipi_dsi_fifo_clear(dsim, val);

	clk_disable(dsim->clock);
	return s5p_mipi_regulator_disable(dsim);
}

static int register_notif_to_fimd(struct mipi_dsim_device *dsim)
{
	return fimd_register_client(s5p_mipi_dsi_notifier,
					(void *)dsim);
}

static int s5p_mipi_dsi_power_on(struct mipi_dsim_device *dsim, bool enable)
{
	struct mipi_dsim_lcd_driver *client_drv = master_to_driver(dsim);
	struct mipi_dsim_lcd_device *client_dev = master_to_device(dsim);
	struct platform_device *pdev = to_platform_device(dsim->dev);

	if (enable != false && enable != true)
		return -EINVAL;

	if (enable) {
		int ret;

		if (!dsim->suspended)
			return 0;

		/* lcd panel power on. */
		if (client_drv && client_drv->power_on)
			client_drv->power_on(client_dev, 1);

		ret = s5p_mipi_regulator_enable(dsim);
		if (ret < 0) {
			client_drv->power_on(client_dev, 0);
			return ret;
		}

		ret = clk_enable(dsim->clock);
		if (ret < 0) {
			dev_err(dsim->dev, "failed to enable clock.\n");
			/* it doesn't need error check. */
			s5p_mipi_regulator_disable(dsim);
			client_drv->power_on(client_dev, 0);
			return ret;
		}

		/* enable MIPI-DSI PHY. */
		if (dsim->pd->phy_enable)
			dsim->pd->phy_enable(pdev, true);

		s5p_mipi_update_cfg(dsim);

		/* set lcd panel sequence commands. */
		if (client_drv && client_drv->set_sequence)
			client_drv->set_sequence(client_dev);

		dsim->suspended = false;
	} else {
		if (dsim->suspended)
			return 0;

		if (client_drv && client_drv->suspend)
			client_drv->suspend(client_dev);

		if (client_drv && client_drv->power_on)
			client_drv->power_on(client_dev, 0);

		clk_disable(dsim->clock);

		dsim->suspended = true;

		return s5p_mipi_regulator_disable(dsim);
	}

	return 0;
}

static int s5p_mipi_dsi_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct mipi_dsim_device *dsim;
	struct mipi_dsim_config *dsim_config;
	struct s5p_platform_mipi_dsim *dsim_pd;
	struct mipi_dsim_lcd_driver *client_drv;
	struct mipi_dsim_ddi *dsim_ddi;
	int ret = -EINVAL;

	dsim = kzalloc(sizeof(struct mipi_dsim_device), GFP_KERNEL);
	if (!dsim) {
		dev_err(&pdev->dev, "failed to allocate dsim object.\n");
		return -EFAULT;
	}

	dsim->pd = to_dsim_plat(pdev);
	dsim->dev = &pdev->dev;
	dsim->id = pdev->id;

	/* get s5p_platform_mipi_dsim. */
	dsim_pd = (struct s5p_platform_mipi_dsim *)dsim->pd;
	if (dsim_pd == NULL) {
		dev_err(&pdev->dev, "failed to get platform data for dsim.\n");
		ret = -EFAULT;
		goto err_free;
	}
	/* get mipi_dsim_config. */
	dsim_config = dsim_pd->dsim_config;
	if (dsim_config == NULL) {
		dev_err(&pdev->dev, "failed to get dsim config data.\n");
		ret = -EFAULT;
		goto err_free;
	}

	dsim->dsim_config = dsim_config;
	dsim->master_ops = &master_ops;

	mutex_init(&dsim->lock);

	dsim->reg_vdd10 = regulator_get(&pdev->dev, "VDD10");
	if (IS_ERR(dsim->reg_vdd10)) {
		ret = PTR_ERR(dsim->reg_vdd10);
		dev_err(&pdev->dev, "failed to get %s regulator (%d)\n",
				"VDD10", ret);
		dsim->reg_vdd10 = NULL;
	}

	dsim->reg_vdd18 = regulator_get(&pdev->dev, "VDD18");
	if (IS_ERR(dsim->reg_vdd18)) {
		ret = PTR_ERR(dsim->reg_vdd18);
		dev_err(&pdev->dev, "failed to get %s regulator (%d)\n",
				"VDD18", ret);
		dsim->reg_vdd18 = NULL;
	}
	dsim->clock = clk_get(&pdev->dev, "dsim0");
	if (IS_ERR(dsim->clock)) {
		dev_err(&pdev->dev, "failed to get dsim clock source\n");
		goto err_regulator;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "failed to get io memory region\n");
		goto err_put_clk;
	}

	dsim->res = request_mem_region(res->start, resource_size(res),
					dev_name(&pdev->dev));
	if (!dsim->res) {
		dev_err(&pdev->dev, "failed to request io memory region\n");
		ret = -ENOMEM;
		goto err_release_res;
	}

	dsim->reg_base = ioremap(res->start, resource_size(res));
	if (!dsim->reg_base) {
		dev_err(&pdev->dev, "failed to remap io region\n");
		ret = -EFAULT;
		goto err_release_mem;
	}

	dsim->irq = platform_get_irq(pdev, 0);
	if (dsim->irq < 0) {
		dev_err(&pdev->dev, "failed to request dsim irq resource\n");
		ret = -EINVAL;
		goto err_unmap;
	}

#ifdef CONFIG_MACH_SLP_NAPLES
	if (dsim_config->e_interface == DSIM_VIDEO) {
		ret = request_irq(dsim->irq, s5p_mipi_dsi_interrupt_handler,
				IRQF_SHARED, pdev->name, dsim);
		if (ret != 0) {
			dev_err(&pdev->dev, "failed to request dsim irq\n");
			ret = -EINVAL;
			goto err_unmap;
		}
	}
#else
	ret = request_irq(dsim->irq, s5p_mipi_dsi_interrupt_handler,
			IRQF_SHARED, pdev->name, dsim);
	if (ret != 0) {
		dev_err(&pdev->dev, "failed to request dsim irq\n");
		ret = -EINVAL;
		goto err_unmap;
	}
#endif

	/* bind lcd ddi matched with panel name. */
	dsim_ddi = s5p_mipi_dsi_bind_lcd_ddi(dsim, dsim_pd->lcd_panel_name);
	if (!dsim_ddi) {
		dev_err(&pdev->dev, "mipi_dsim_ddi object not found.\n");
		goto err_free_irq;
	}

	/* register a callback called by fimd. */
	ret = register_notif_to_fimd(dsim);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register fimd notifier.\n");
		fimd_unregister_client(s5p_mipi_dsi_notifier);

		goto err_free_irq;
	}

	init_completion(&dsim_wr_comp);
	init_completion(&dsim_rd_comp);
	mutex_init(&dsim->lock);

	client_drv = dsim_ddi->dsim_lcd_drv;

	/* initialize mipi-dsi client(lcd panel). */
	if (client_drv && client_drv->probe)
		client_drv->probe(dsim_ddi->dsim_lcd_dev);

	platform_set_drvdata(pdev, dsim);

	/* in case that mipi got enabled at bootloader. */
	if (dsim_pd->enabled) {
		ret = s5p_mipi_regulator_enable(dsim);
		if (ret < 0)
			goto err_regulator_enable;

		ret = pm_runtime_set_active(&pdev->dev);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to active pm runtime.\n");
			goto err_pm_runtime_active;
		}

		s5p_mipi_dsi_init_interrupt(dsim);

		/* set lcd panel sequence commands. */
		if (client_drv && client_drv->set_sequence)
			client_drv->set_sequence(dsim_ddi->dsim_lcd_dev);
	} else {
		pm_runtime_set_suspended(&pdev->dev);
		dsim->suspended = true;
	}

	pm_runtime_enable(&pdev->dev);
	ret = pm_runtime_get_sync(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get pm runtime.\n");
		pm_runtime_disable(&pdev->dev);
		goto err_regulator_enable;
	}
#ifdef CONFIG_MACH_SLP_NAPLES
	if (dsim_config->e_interface == DSIM_COMMAND)
		dsim->master_ops->trigger(NULL);
#endif

	dev_info(&pdev->dev, "mipi-dsi driver(%s mode) has been probed.\n",
		(dsim_config->e_interface == DSIM_COMMAND) ?
			"CPU" : "RGB");

	return 0;

err_pm_runtime_active:
	/* it doesn't need error check. */
	s5p_mipi_regulator_disable(dsim);

err_regulator_enable:
	if (client_drv && client_drv->remove)
		client_drv->remove(dsim_ddi->dsim_lcd_dev);

err_free_irq:
	free_irq(dsim->irq, dsim);

err_unmap:
	iounmap((void __iomem *) dsim->reg_base);

err_release_mem:
	release_mem_region(dsim->res->start, resource_size(dsim->res));

err_release_res:
	release_resource(dsim->res);

err_put_clk:
	clk_disable(dsim->clock);
	clk_put(dsim->clock);

err_regulator:
	regulator_put(dsim->reg_vdd18);
	regulator_put(dsim->reg_vdd10);

err_free:
	kfree(dsim);
	return ret;
}

static int __devexit s5p_mipi_dsi_remove(struct platform_device *pdev)
{
	struct mipi_dsim_device *dsim = platform_get_drvdata(pdev);
	struct mipi_dsim_ddi *dsim_ddi;
	struct mipi_dsim_lcd_driver *dsim_lcd_drv;

	iounmap(dsim->reg_base);

	s5p_mipi_regulator_disable(dsim);
	regulator_put(dsim->reg_vdd18);
	regulator_put(dsim->reg_vdd10);

	clk_disable(dsim->clock);
	clk_put(dsim->clock);

	release_resource(dsim->res);
	release_mem_region(dsim->res->start, resource_size(dsim->res));

	list_for_each_entry(dsim_ddi, &dsim_ddi_list, list) {
		if (dsim_ddi) {
			if (dsim->id != dsim_ddi->bus_id)
				continue;

			dsim_lcd_drv = dsim_ddi->dsim_lcd_drv;

			if (dsim_lcd_drv->remove)
				dsim_lcd_drv->remove(dsim_ddi->dsim_lcd_dev);

			kfree(dsim_ddi);
		}
	}

	kfree(dsim);

	return 0;
}

#ifdef CONFIG_PM
static int s5p_mipi_dsi_suspend(struct device *dev)
{
	struct mipi_dsim_device *dsim = dev_to_dsim(dev);

	if (pm_runtime_suspended(dev))
		return 0;

	/*
	 * do not use pm_runtime_suspend(). if pm_runtime_suspend() is
	 * called here, an error would be returned by that interface
	 * because the usage_count of pm runtime is more than 1.
	 */
	return s5p_mipi_dsi_power_on(dsim, false);
}

static int s5p_mipi_dsi_resume(struct device *dev)
{
	struct mipi_dsim_device *dsim = dev_to_dsim(dev);

	/*
	 * if entering to sleep when lcd panel is on, the usage_count
	 * of pm runtime would still be 1 so in this case, mipi dsi driver
	 * should be on directly not drawing on pm runtime interface.
	 */
	if (!pm_runtime_suspended(dev))
		return s5p_mipi_dsi_power_on(dsim, true);

	return 0;
}

static int s5p_mipi_dsi_runtime_suspend(struct device *dev)
{
	struct mipi_dsim_device *dsim = dev_to_dsim(dev);

	return s5p_mipi_dsi_power_on(dsim, false);
}

static int s5p_mipi_dsi_runtime_resume(struct device *dev)
{
	struct mipi_dsim_device *dsim = dev_to_dsim(dev);

	return s5p_mipi_dsi_power_on(dsim, true);
}

static const struct dev_pm_ops s5p_mipi_dsi_pm_ops = {
	.suspend		= s5p_mipi_dsi_suspend,
	.resume			= s5p_mipi_dsi_resume,
	.runtime_suspend	= s5p_mipi_dsi_runtime_suspend,
	.runtime_resume		= s5p_mipi_dsi_runtime_resume,
};
#endif

static struct platform_driver s5p_mipi_dsi_driver = {
	.probe			= s5p_mipi_dsi_probe,
	.remove			= __devexit_p(s5p_mipi_dsi_remove),
	.driver = {
		   .name = "s5p-mipi-dsim",
		   .owner = THIS_MODULE,
#ifdef CONFIG_PM
		   .pm		= &s5p_mipi_dsi_pm_ops,
#endif
	},
};

static int s5p_mipi_dsi_register(void)
{
	platform_driver_register(&s5p_mipi_dsi_driver);

	return 0;
}

static void s5p_mipi_dsi_unregister(void)
{
	platform_driver_unregister(&s5p_mipi_dsi_driver);
}

late_initcall(s5p_mipi_dsi_register);
module_exit(s5p_mipi_dsi_unregister);

MODULE_AUTHOR("InKi Dae <inki.dae@samsung.com>");
MODULE_DESCRIPTION("Samusung SoC MIPI-DSI driver");
MODULE_LICENSE("GPL");
