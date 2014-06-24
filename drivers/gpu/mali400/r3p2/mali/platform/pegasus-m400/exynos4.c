/* drivers/gpu/mali400/mali/platform/pegasus-m400/exynos4.c
 *
 * Copyright 2011 by S.LSI. Samsung Electronics Inc.
 * San#24, Nongseo-Dong, Giheung-Gu, Yongin, Korea
 *
 * Samsung SoC Mali400 DVFS driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software FoundatIon.
 */

/**
 * @file exynos4.c
 * Platform specific Mali driver functions for the exynos 4XXX based platforms
 */
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/pm.h>
#include <linux/suspend.h>

#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif

#ifdef CONFIG_MALI_DVFS
#include "mali_kernel_utilization.h"
#endif /* CONFIG_MALI_DVFS */

#include <linux/mali/mali_utgard.h>
#include "mali_kernel_common.h"
#include "mali_kernel_linux.h"
#include "mali_pm.h"

#include <plat/pd.h>

#include "exynos4_pmm.h"

#if defined(CONFIG_PM_RUNTIME)
/* We does not need PM NOTIFIER in r3p2 DDK */
//#define USE_PM_NOTIFIER
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
struct exynos_pm_domain;
extern struct exynos_pm_domain exynos4_pd_g3d;
void exynos_pm_add_dev_to_genpd(struct platform_device *pdev, struct exynos_pm_domain *pd);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
extern struct platform_device exynos4_device_pd[];
#else
extern struct platform_device s5pv310_device_pd[];
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0) */

static void mali_platform_device_release(struct device *device);

#if defined(CONFIG_PM_RUNTIME)
#if defined(USE_PM_NOTIFIER)
static int mali_os_suspend(struct device *device);
static int mali_os_resume(struct device *device);
static int mali_os_freeze(struct device *device);
static int mali_os_thaw(struct device *device);

static int mali_runtime_suspend(struct device *device);
static int mali_runtime_resume(struct device *device);
static int mali_runtime_idle(struct device *device);
#endif
#endif

#if defined(CONFIG_ARCH_S5PV310) && !defined(CONFIG_BOARD_HKDKC210)

/* This is for other SMDK boards */
#define MALI_BASE_IRQ 232

#else

/* This is for the Odroid boards */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0))
#define MALI_BASE_IRQ 182
#else
#define MALI_BASE_IRQ 150
#endif

#endif

#define MALI_GP_IRQ	   MALI_BASE_IRQ + 9
#define MALI_PP0_IRQ	  MALI_BASE_IRQ + 5
#define MALI_PP1_IRQ	  MALI_BASE_IRQ + 6
#define MALI_PP2_IRQ	  MALI_BASE_IRQ + 7
#define MALI_PP3_IRQ	  MALI_BASE_IRQ + 8
#define MALI_GP_MMU_IRQ   MALI_BASE_IRQ + 4
#define MALI_PP0_MMU_IRQ  MALI_BASE_IRQ + 0
#define MALI_PP1_MMU_IRQ  MALI_BASE_IRQ + 1
#define MALI_PP2_MMU_IRQ  MALI_BASE_IRQ + 2
#define MALI_PP3_MMU_IRQ  MALI_BASE_IRQ + 3

static struct resource mali_gpu_resources[] =
{
	MALI_GPU_RESOURCES_MALI400_MP4(0x13000000,
								   MALI_GP_IRQ, MALI_GP_MMU_IRQ,
								   MALI_PP0_IRQ, MALI_PP0_MMU_IRQ,
								   MALI_PP1_IRQ, MALI_PP1_MMU_IRQ,
								   MALI_PP2_IRQ, MALI_PP2_MMU_IRQ,
								   MALI_PP3_IRQ, MALI_PP3_MMU_IRQ)
};

#ifdef CONFIG_PM_RUNTIME
#if defined(USE_PM_NOTIFIER)
static int mali_pwr_suspend_notifier(struct notifier_block *nb,unsigned long event,void* dummy);

static struct notifier_block mali_pwr_notif_block = {
	.notifier_call = mali_pwr_suspend_notifier
};
#endif
#endif /* CONFIG_PM_RUNTIME */

#if 0
static struct dev_pm_ops mali_gpu_device_type_pm_ops =
{
#ifndef CONFIG_PM_RUNTIME
	.suspend = mali_os_suspend,
	.resume = mali_os_resume,
#endif
	.freeze = mali_os_freeze,
	.thaw = mali_os_thaw,
#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend = mali_runtime_suspend,
	.runtime_resume = mali_runtime_resume,
	.runtime_idle = mali_runtime_idle,
#endif
};
#endif

#if defined(USE_PM_NOTIFIER)
static struct device_type mali_gpu_device_device_type =
{
	.pm = &mali_gpu_device_type_pm_ops,
};
#endif

static struct platform_device mali_gpu_device =
{
	.name = "mali_dev", /* MALI_SEC MALI_GPU_NAME_UTGARD, */
	.id = 0,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
	/* Set in mali_platform_device_register() for these kernels */
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
	.dev.parent = &exynos4_device_pd[PD_G3D].dev,
#else
	.dev.parent = &s5pv310_device_pd[PD_G3D].dev,
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0) */
	.dev.release = mali_platform_device_release,
#if 0
	/*
	 * We temporarily make use of a device type so that we can control the Mali power
	 * from within the mali.ko (since the default platform bus implementation will not do that).
	 * Ideally .dev.pm_domain should be used instead, as this is the new framework designed
	 * to control the power of devices.
	 */
	.dev.type = &mali_gpu_device_device_type, /* We should probably use the pm_domain instead of type on newer kernels */
#endif
};

static struct mali_gpu_device_data mali_gpu_data =
{
	.shared_mem_size = 256 * 1024 * 1024, /* 256MB */
	.fb_start = 0x40000000,
	.fb_size = 0xb1000000,
	.utilization_interval = 100, /* 100ms */
	.utilization_callback = mali_gpu_utilization_handler,
};

int mali_platform_device_register(void)
{
	int err;

	MALI_DEBUG_PRINT(4, ("mali_platform_device_register() called\n"));

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
	exynos_pm_add_dev_to_genpd(&mali_gpu_device, &exynos4_pd_g3d);
#endif

	/* Connect resources to the device */
	err = platform_device_add_resources(&mali_gpu_device, mali_gpu_resources, sizeof(mali_gpu_resources) / sizeof(mali_gpu_resources[0]));
	if (0 == err)
	{
		err = platform_device_add_data(&mali_gpu_device, &mali_gpu_data, sizeof(mali_gpu_data));
		if (0 == err)
		{
#ifdef CONFIG_PM_RUNTIME
#if defined(USE_PM_NOTIFIER)
			err = register_pm_notifier(&mali_pwr_notif_block);
			if (err)
			{
				goto plat_init_err;
			}
#endif
#endif /* CONFIG_PM_RUNTIME */

			/* Register the platform device */
			err = platform_device_register(&mali_gpu_device);
			if (0 == err)
			{
				mali_platform_init(&(mali_gpu_device.dev));

#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
				pm_runtime_set_autosuspend_delay(&(mali_gpu_device.dev), 1000);
				pm_runtime_use_autosuspend(&(mali_gpu_device.dev));
#endif
				pm_runtime_enable(&(mali_gpu_device.dev));
#endif

				return 0;
			}
		}

#ifdef CONFIG_PM_RUNTIME
#if defined(USE_PM_NOTIFIER)
plat_init_err:
		unregister_pm_notifier(&mali_pwr_notif_block);
#endif
#endif /* CONFIG_PM_RUNTIME */
		platform_device_unregister(&mali_gpu_device);
	}

	return err;
}

void mali_platform_device_unregister(void)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_unregister() called\n"));

#ifdef CONFIG_PM_RUNTIME
#if defined(USE_PM_NOTIFIER)
	unregister_pm_notifier(&mali_pwr_notif_block);
#endif
#endif /* CONFIG_PM_RUNTIME */

	mali_platform_deinit(&(mali_gpu_device.dev));

	platform_device_unregister(&mali_gpu_device);
}

static void mali_platform_device_release(struct device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_release() called\n"));
}

#ifdef CONFIG_PM_RUNTIME
#if defined(USE_PM_NOTIFIER)
static int mali_pwr_suspend_notifier(struct notifier_block *nb,unsigned long event,void* dummy)
{
	int err = 0;
	switch (event)
	{
		case PM_SUSPEND_PREPARE:
			mali_pm_os_suspend();
			err = mali_os_suspend(&(mali_platform_device->dev));
			break;

		case PM_POST_SUSPEND:
			err = mali_os_resume(&(mali_platform_device->dev));
			mali_pm_os_resume();
			break;
		default:
			break;
	}
	return err;
}

static int mali_os_suspend(struct device *device)
{
	int ret = 0;
	MALI_DEBUG_PRINT(4, ("mali_os_suspend() called\n"));

#ifdef CONFIG_MALI_DVFS
	mali_utilization_suspend();
#endif

	if (NULL != device &&
		NULL != device->driver &&
		NULL != device->driver->pm &&
		NULL != device->driver->pm->suspend)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->suspend(device);
	}

	mali_platform_power_mode_change(device, MALI_POWER_MODE_DEEP_SLEEP);

	return ret;
}

static int mali_os_resume(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_os_resume() called\n"));
#ifdef CONFIG_REGULATOR
	mali_regulator_enable();
	g3d_power_domain_control(1);
#endif
	mali_platform_power_mode_change(device, MALI_POWER_MODE_ON);

	if (NULL != device &&
		NULL != device->driver &&
		NULL != device->driver->pm &&
		NULL != device->driver->pm->resume)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->resume(device);
	}

	return ret;
}

static int mali_os_freeze(struct device *device)
{
	int ret = 0;
	MALI_DEBUG_PRINT(4, ("mali_os_freeze() called\n"));

	if (NULL != device->driver &&
		NULL != device->driver->pm &&
		NULL != device->driver->pm->freeze)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->freeze(device);
	}

	return ret;
}

static int mali_os_thaw(struct device *device)
{
	int ret = 0;
	MALI_DEBUG_PRINT(4, ("mali_os_thaw() called\n"));

	if (NULL != device->driver &&
		NULL != device->driver->pm &&
		NULL != device->driver->pm->thaw)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->thaw(device);
	}

	return ret;
}

static int mali_runtime_suspend(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_runtime_suspend() called\n"));
	if (NULL != device->driver &&
		NULL != device->driver->pm &&
		NULL != device->driver->pm->runtime_suspend)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->runtime_suspend(device);
	}

	mali_platform_power_mode_change(device, MALI_POWER_MODE_LIGHT_SLEEP);

	return ret;
}

static int mali_runtime_resume(struct device *device)
{
	int ret = 0;
	MALI_DEBUG_PRINT(4, ("mali_runtime_resume() called\n"));

	mali_platform_power_mode_change(device, MALI_POWER_MODE_ON);

	if (NULL != device->driver &&
		NULL != device->driver->pm &&
		NULL != device->driver->pm->runtime_resume)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->runtime_resume(device);
	}

	return ret;
}

static int mali_runtime_idle(struct device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_runtime_idle() called\n"));
	if (NULL != device->driver &&
		NULL != device->driver->pm &&
		NULL != device->driver->pm->runtime_idle)
	{
		int ret = 0;
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->runtime_idle(device);
		if (0 != ret)
		{
			return ret;
		}
	}

	return 1;
}

#endif /* USE_PM_NOTIFIER */
#endif /* CONFIG_PM_RUNTIME */
