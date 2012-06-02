#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <mach/gpio-exynos4.h>
#include <plat/gpio-cfg.h>
#include <plat/devs.h>
#include <plat/ehci.h>
#include <linux/msm_charm.h>
#include <mach/mdm2.h>
#include "mdm_private.h"

static struct resource mdm_resources[] = {
	{
		.start	= MDM2AP_ERRFATAL,
		.end	= MDM2AP_ERRFATAL,
		.name	= "MDM2AP_ERRFATAL",
		.flags	= IORESOURCE_IO,
	},
	{
		.start	= AP2MDM_ERRFATAL,
		.end	= AP2MDM_ERRFATAL,
		.name	= "AP2MDM_ERRFATAL",
		.flags	= IORESOURCE_IO,
	},
	{
		.start	= MDM2AP_STATUS,
		.end	= MDM2AP_STATUS,
		.name	= "MDM2AP_STATUS",
		.flags	= IORESOURCE_IO,
	},
	{
		.start	= AP2MDM_STATUS,
		.end	= AP2MDM_STATUS,
		.name	= "AP2MDM_STATUS",
		.flags	= IORESOURCE_IO,
	},
	{
		.start	= AP2MDM_PMIC_RESET_N,
		.end	= AP2MDM_PMIC_RESET_N,
		.name	= "AP2MDM_PMIC_RESET_N",
		.flags	= IORESOURCE_IO,
	},
};

static struct mdm_platform_data mdm_platform_data = {
	.mdm_version = "3.0",
	.ramdump_delay_ms = 2000,
	.peripheral_platform_device = &s5p_device_ehci,
};

struct platform_device mdm_device = {
	.name		= "mdm2_modem",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(mdm_resources),
	.resource	= mdm_resources,
};

static int __init init_mdm_modem(void)
{
	pr_err("%s	!!!	!!\n", __func__);
	mdm_device.dev.platform_data = &mdm_platform_data;
	return platform_device_register(&mdm_device);
}
module_init(init_mdm_modem);
