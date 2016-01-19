/*
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/platform_data/modem.h>

#include <plat/gpio-cfg.h>
#include <plat/devs.h>

#include <mach/dev.h>
#include <mach/gpio.h>
#include <mach/gpio-exynos4.h>
#include <mach/regs-mem.h>

static struct spi_gpio_platform_data modem_boot_spi_gpio_pdata = {
	.sck = GPIO_MODEM_SPI_CLK,
	.mosi = GPIO_MODEM_SPI_MOSI,
	.miso = GPIO_MODEM_SPI_MISO,
	.num_chipselect = 1,
};

static struct platform_device samsung_device_modem_boot_spi = {
	.name = "spi_gpio",
	.id = GPIO_MODEM_SPI_ID,
	.dev = {
		.platform_data = &modem_boot_spi_gpio_pdata,
	},
};

static void __init config_boot_spi_gpio(void)
{
	s3c_gpio_cfgpin(GPIO_MODEM_SPI_CLK, GPIO_SPI_SFN);
	s3c_gpio_setpull(GPIO_MODEM_SPI_CLK, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_MODEM_SPI_CSN, GPIO_SPI_SFN);
	s3c_gpio_setpull(GPIO_MODEM_SPI_CSN, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_MODEM_SPI_MISO, GPIO_SPI_SFN);
	s3c_gpio_setpull(GPIO_MODEM_SPI_MISO, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_MODEM_SPI_MOSI, GPIO_SPI_SFN);
	s3c_gpio_setpull(GPIO_MODEM_SPI_MOSI, S3C_GPIO_PULL_NONE);
}

static int __init spi_boot_device_init(void)
{
	int err = 0;

	config_boot_spi_gpio();

	err = platform_device_register(&samsung_device_modem_boot_spi);
	if (err) {
		mif_err("ERR! %s:%d platform_device_register fail (err %d)\n",
			samsung_device_modem_boot_spi.name,
			samsung_device_modem_boot_spi.id,
			err);
		goto exit;
	}

exit:
	return err;
}
device_initcall(spi_boot_device_init);

