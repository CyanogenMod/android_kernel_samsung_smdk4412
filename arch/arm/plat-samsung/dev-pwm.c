/* linux/arch/arm/plat-samsung/dev-pwm.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Copyright (c) 2007 Ben Dooks
 * Copyright (c) 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>, <ben-linux@fluff.org>
 *
 * S3C series device definition for the PWM timer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>

#include <mach/irqs.h>

#include <plat/devs.h>
#include <mach/gpio.h>

#define TIMER_RESOURCE_SIZE (1)

#define TIMER_RESOURCE(_tmr, _irq)			\
	(struct resource [TIMER_RESOURCE_SIZE]) {	\
		[0] = {					\
			.start	= _irq,			\
			.end	= _irq,			\
			.flags	= IORESOURCE_IRQ	\
		}					\
	}

#define DEFINE_S3C_TIMER(_tmr_no, _irq, _plat_data)		\
	.name       = "s3c24xx-pwm",				\
	.id     = _tmr_no,				\
	.num_resources  = TIMER_RESOURCE_SIZE,		\
	.resource   = TIMER_RESOURCE(_tmr_no, _irq),    \
	.dev        = {					\
		.platform_data  = _plat_data,			\
	}

#define GPD0_0_TOUT		(0x2 << 0)
#ifdef CONFIG_FB_MDNIE_PWM
#define GPD0_1_TOUT		(0x3 << 4)
#else
#define GPD0_1_TOUT		(0x2 << 4)
#endif
#define GPD0_2_TOUT		(0x2 << 8)
#define GPD0_3_TOUT		(0x2 << 12)

/* since we already have an static mapping for the timer, we do not
 * bother setting any IO resource for the base.
 */

struct s3c_pwm_pdata {
	/* PWM output port */
	int gpio_no;
	const char  *gpio_name;
	int gpio_set_value;
};

struct s3c_pwm_pdata pwm_data[] = {
#ifdef CONFIG_ARCH_EXYNOS5
	{
		.gpio_no = EXYNOS5_GPB2(0),
		.gpio_name = "GPB",
		.gpio_set_value = GPD0_0_TOUT,
	}, {
		.gpio_no = EXYNOS5_GPB2(1),
		.gpio_name = "GPB",
		.gpio_set_value = GPD0_1_TOUT,
	}, {
		.gpio_no = EXYNOS5_GPB2(2),
		.gpio_name = "GPB",
		.gpio_set_value = GPD0_2_TOUT,
	}, {
		.gpio_no = EXYNOS5_GPB2(3),
		.gpio_name = "GPB",
		.gpio_set_value = GPD0_3_TOUT,
	}, {
		.gpio_no = 0,
		.gpio_name = NULL,
		.gpio_set_value = 0,
	}
#else
	{
		.gpio_no = EXYNOS4_GPD0(0),
		.gpio_name = "GPD",
		.gpio_set_value = GPD0_0_TOUT,
	}, {
		.gpio_no = EXYNOS4_GPD0(1),
		.gpio_name = "GPD",
		.gpio_set_value = GPD0_1_TOUT,
	}, {
		.gpio_no = EXYNOS4_GPD0(2),
		.gpio_name = "GPD",
		.gpio_set_value = GPD0_2_TOUT,
	}, {
		.gpio_no = EXYNOS4_GPD0(3),
		.gpio_name = "GPD",
		.gpio_set_value = GPD0_3_TOUT,
	}, {
		.gpio_no = 0,
		.gpio_name = NULL,
		.gpio_set_value = 0,
	}
#endif
};

struct platform_device s3c_device_timer[] = {
	[0] = { DEFINE_S3C_TIMER(0, IRQ_TIMER0, &pwm_data[0]) },
	[1] = { DEFINE_S3C_TIMER(1, IRQ_TIMER1, &pwm_data[1]) },
	[2] = { DEFINE_S3C_TIMER(2, IRQ_TIMER2, &pwm_data[2]) },
	[3] = { DEFINE_S3C_TIMER(3, IRQ_TIMER3, &pwm_data[3]) },
	[4] = { DEFINE_S3C_TIMER(4, IRQ_TIMER4, &pwm_data[4]) },
};
EXPORT_SYMBOL(s3c_device_timer);
