/*
 * Copyright (C) 2011 Samsung Electronics.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"

#define SPI_XMIT_DELEY	100

static int check_cp_status(unsigned int gpio_cp_status, unsigned int count)
{
	int ret = 0;
	int cnt = 0;

	while (1) {
		if (gpio_get_value(gpio_cp_status) != 0) {
			ret = 0;
			break;
		}

		cnt++;
		if (cnt >= count) {
			mif_err("ERR! gpio_cp_status == 0 (cnt %d)\n", cnt);
			ret = -EFAULT;
			break;
		}

		msleep(20);
	}

	return ret;
}

static inline int spi_send(struct modem_boot_spi *loader, const char val)
{
	char buff[1];
	int ret;
	struct spi_message msg;
	struct spi_transfer xfer = {
		.len = 1,
		.tx_buf = buff,
	};

	buff[0] = val;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(loader->spi_dev, &msg);
	if (ret < 0)
		mif_err("ERR! spi_sync fail (err %d)\n", ret);

	return ret;
}

static int spi_boot_write(struct modem_boot_spi *loader, const char *addr,
			const long len)
{
	int i;
	int ret = 0;
	char *buff = NULL;
	mif_err("+++\n");

	buff = kzalloc(len, GFP_KERNEL);
	if (!buff) {
		mif_err("ERR! kzalloc(%ld) fail\n", len);
		ret = -ENOMEM;
		goto exit;
	}

	ret = copy_from_user(buff, (const void __user *)addr, len);
	if (ret) {
		mif_err("ERR! copy_from_user fail (err %d)\n", ret);
		ret = -EFAULT;
		goto exit;
	}

	for (i = 0 ; i < len ; i++) {
		ret = spi_send(loader, buff[i]);
		if (ret < 0) {
			mif_err("ERR! spi_send fail (err %d)\n", ret);
			goto exit;
		}
	}

exit:
	if (buff)
		kfree(buff);

	mif_err("---\n");
	return ret;
}

static int spi_boot_open(struct inode *inode, struct file *filp)
{
	struct modem_boot_spi *loader = to_modem_boot_spi(filp->private_data);
	filp->private_data = loader;
	return 0;
}

static long spi_boot_ioctl(struct file *filp, unsigned int cmd,
			unsigned long arg)
{
	int ret = 0;
	struct modem_firmware img;
	struct modem_boot_spi *loader = filp->private_data;

	mutex_lock(&loader->lock);
	switch (cmd) {
	case IOCTL_MODEM_XMIT_BOOT:
		ret = copy_from_user(&img, (const void __user *)arg,
					sizeof(struct modem_firmware));
		if (ret) {
			mif_err("ERR! copy_from_user fail (err %d)\n", ret);
			ret = -EFAULT;
			goto exit_err;
		}
		mif_info("IOCTL_MODEM_XMIT_BOOT (size %d)\n", img.size);

		ret = spi_boot_write(loader, img.binary, img.size);
		if (ret < 0) {
			mif_err("ERR! spi_boot_write fail (err %d)\n", ret);
			break;
		}

		if (!loader->gpio_cp_status)
			break;

		ret = check_cp_status(loader->gpio_cp_status, 100);
		if (ret < 0)
			mif_err("ERR! check_cp_status fail (err %d)\n", ret);

		break;

	default:
		mif_err("ioctl cmd error\n");
		ret = -ENOIOCTLCMD;

		break;
	}
	mutex_unlock(&loader->lock);

exit_err:
	return ret;
}

static const struct file_operations modem_spi_boot_fops = {
	.owner = THIS_MODULE,
	.open = spi_boot_open,
	.unlocked_ioctl = spi_boot_ioctl,
};

static int __devinit modem_spi_boot_probe(struct spi_device *spi)
{
	int ret;
	struct modem_boot_spi *loader;
	struct modem_boot_spi_platform_data *pdata;
	mif_debug("+++\n");

	loader = kzalloc(sizeof(*loader), GFP_KERNEL);
	if (!loader) {
		mif_err("failed to allocate for modem_boot_spi\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	mutex_init(&loader->lock);

	spi->bits_per_word = 8;

	if (spi_setup(spi)) {
		mif_err("ERR! spi_setup fail\n");
		ret = -EINVAL;
		goto err_setup;
	}
	loader->spi_dev = spi;

	if (!spi->dev.platform_data) {
		mif_err("ERR! no platform_data\n");
		ret = -EINVAL;
		goto err_setup;
	}
	pdata = (struct modem_boot_spi_platform_data *)spi->dev.platform_data;

	loader->gpio_cp_status = pdata->gpio_cp_status;

	spi_set_drvdata(spi, loader);

	loader->dev.minor = MISC_DYNAMIC_MINOR;
	loader->dev.name = MODEM_BOOT_DEV_SPI;
	loader->dev.fops = &modem_spi_boot_fops;
	ret = misc_register(&loader->dev);
	if (ret) {
		mif_err("ERR! misc_register fail (err %d)\n", ret);
		goto err_setup;
	}

	mif_debug("---\n");
	return 0;

err_setup:
	mutex_destroy(&loader->lock);
	kfree(loader);

err_alloc:
	mif_err("xxx\n");
	return ret;
}

static int __devexit modem_spi_boot_remove(struct spi_device *spi)
{
	struct modem_boot_spi *loader = spi_get_drvdata(spi);

	misc_deregister(&loader->dev);
	mutex_destroy(&loader->lock);
	kfree(loader);

	return 0;
}

static struct spi_driver modem_boot_device_spi_driver = {
	.driver = {
		.name = MODEM_BOOT_DEV_SPI,
		.owner = THIS_MODULE,
	},
	.probe = modem_spi_boot_probe,
	.remove = __devexit_p(modem_spi_boot_remove),
};

static int __init modem_boot_device_spi_init(void)
{
	int err;

	err = spi_register_driver(&modem_boot_device_spi_driver);
	if (err) {
		mif_err("spi_register_driver fail (err %d)\n", err);
		return err;
	}

	return 0;
}

static void __exit modem_boot_device_spi_exit(void)
{
	spi_unregister_driver(&modem_boot_device_spi_driver);
}

module_init(modem_boot_device_spi_init);
module_exit(modem_boot_device_spi_exit);

MODULE_DESCRIPTION("SPI Driver for Downloading Modem Bootloader");
MODULE_LICENSE("GPL");

