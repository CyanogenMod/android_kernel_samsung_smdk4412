/* Lte modem bootloader support for Samsung Tuna Board.
 *
 * Copyright (C) 2011 Google, Inc.
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

#include <linux/platform_data/lte_modem_bootloader.h>

#define LEN_XMIT_DELEY	10
#define MAX_XMIT_SIZE	16

int factory_mode;

enum xmit_bootloader_status {
	XMIT_BOOT_READY,
	XMIT_LOADER_READY,
};

struct lte_modem_bootloader {
	struct spi_device *spi_dev;
	struct miscdevice dev;

	struct mutex lock;

	unsigned int gpio_lte2ap_status;
	enum xmit_bootloader_status xmit_status;
};
#define to_loader(misc)	container_of(misc, struct lte_modem_bootloader, dev);

static inline
int spi_xmit(struct lte_modem_bootloader *loader,
		const char *buf, int size_per_xmit)
{
	int i;
	int ret;
	unsigned char xmit_buf[MAX_XMIT_SIZE];
	struct spi_message msg;
	struct spi_transfer xfers[MAX_XMIT_SIZE];

	memcpy(xmit_buf, buf, sizeof(xmit_buf));
	spi_message_init(&msg);
	memset(xfers, 0, sizeof(xfers));
	for (i = 0; i < size_per_xmit ; i++) {
		xfers[i].cs_change = 1;
		xfers[i].len = 1;
		xfers[i].tx_buf = xmit_buf + i;
		spi_message_add_tail(&xfers[i], &msg);
	}
	ret = spi_sync(loader->spi_dev, &msg);

	if (ret < 0)
		dev_err(&loader->spi_dev->dev,
				"%s - error %d\n", __func__, ret);

	return ret;
}


static
int bootloader_write(struct lte_modem_bootloader *loader,
		const char *addr, const int len)
{
	int i;
	int ret = 0;
	unsigned char lenbuf[4];

	if (loader->xmit_status == XMIT_LOADER_READY) {
		memcpy(lenbuf, &len, ARRAY_SIZE(lenbuf));
		ret = spi_xmit(loader, lenbuf,
				ARRAY_SIZE(lenbuf));
		if (ret < 0)
			return ret;
		msleep(LEN_XMIT_DELEY);
	}

	for (i = 0 ; i < len / MAX_XMIT_SIZE ; i++) {
		ret = spi_xmit(loader,
				addr + i * MAX_XMIT_SIZE,
				MAX_XMIT_SIZE);
		if (ret < 0)
			return ret;
	}
	ret = spi_xmit(loader, addr + i * MAX_XMIT_SIZE , len % MAX_XMIT_SIZE);

	return 0;
}


static
int bootloader_open(struct inode *inode, struct file *flip)
{
	struct lte_modem_bootloader *loader = to_loader(flip->private_data);
	flip->private_data = loader;

	return 0;
}

static
long bootloader_ioctl(struct file *flip,
		unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int status;
	struct lte_modem_bootloader_param param;
	struct lte_modem_bootloader *loader = flip->private_data;

	mutex_lock(&loader->lock);
	switch (cmd) {
	case IOCTL_LTE_MODEM_XMIT_BOOT:

		ret = copy_from_user(&param, (const void __user *)arg,
						sizeof(param));
		if (ret) {
			dev_err(&loader->spi_dev->dev, "%s - can not copy userdata\n",
					__func__);
			ret = -EFAULT;
			goto exit_err;
		}

		dev_info(&loader->spi_dev->dev,
				"IOCTL_LTE_MODEM_XMIT_BOOT - bin size: %d\n",
				param.len);

		ret = bootloader_write(loader, param.buf, param.len);
		if (ret < 0) {
			dev_err(&loader->spi_dev->dev, "failed to xmit boot bin\n");
		} else {
			if (loader->xmit_status == XMIT_BOOT_READY)
				loader->xmit_status = XMIT_LOADER_READY;
			else
				loader->xmit_status = XMIT_BOOT_READY;
		}

		break;
	case IOCTL_LTE_MODEM_LTE2AP_STATUS:
		status = gpio_get_value(loader->gpio_lte2ap_status);
		pr_debug("LTE2AP status :%d\n", status);
		ret = copy_to_user((unsigned int *)arg, &status,
				sizeof(status));

		break;

	case IOCTL_LTE_MODEM_FACTORY_MODE_ON:
		factory_mode = 1;
		pr_info("usb %s, Factory Mode On\n", __func__);
		break;

	case IOCTL_LTE_MODEM_FACTORY_MODE_OFF:
		factory_mode = 0;
		pr_info("usb %s, Factory Mode Off\n", __func__);
		break;

	default:
		dev_err(&loader->spi_dev->dev,
						"%s - ioctl cmd error\n",
						__func__);
		ret = -ENOIOCTLCMD;

		break;
	}
	mutex_unlock(&loader->lock);

exit_err:
	return ret;
}

static const struct file_operations lte_modem_bootloader_fops = {
	.owner = THIS_MODULE,
	.open = bootloader_open,
	.unlocked_ioctl = bootloader_ioctl,
};

static
int bootloader_gpio_setup(struct lte_modem_bootloader *loader)
{
	if (!loader->gpio_lte2ap_status)
		return -EINVAL;

	gpio_request(loader->gpio_lte2ap_status, "GPIO_LTE2AP_STATUS");
	gpio_direction_input(loader->gpio_lte2ap_status);

	return 0;
}

static
int __devinit lte_modem_bootloader_probe(struct spi_device *spi)
{
	int ret;

	struct lte_modem_bootloader *loader;
	struct lte_modem_bootloader_platform_data *pdata;

	loader = kzalloc(sizeof(*loader), GFP_KERNEL);
	if (!loader) {
		pr_err("failed to allocate for lte_modem_bootloader\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	mutex_init(&loader->lock);

	spi->bits_per_word = 8;

	if (spi_setup(spi)) {
		pr_err("failed to setup spi for lte_modem_bootloader\n");
		ret = -EINVAL;
		goto err_setup;
	}

	loader->spi_dev = spi;

	if (!spi->dev.platform_data) {
		pr_err("failed to get platform data for lte_modem_bootloader\n");
		ret = -EINVAL;
		goto err_setup;
	}
	pdata = (struct lte_modem_bootloader_platform_data *) \
		spi->dev.platform_data;
		loader->gpio_lte2ap_status = pdata->gpio_lte2ap_status;

	ret = bootloader_gpio_setup(loader);
	if (ret) {
		pr_err("failed to set gpio for lte_modem_boot_loader\n");
		goto err_setup;
	}

	loader->gpio_lte2ap_status = pdata->gpio_lte2ap_status;
	loader->xmit_status = XMIT_BOOT_READY;

	spi_set_drvdata(spi, loader);

	loader->dev.minor = MISC_DYNAMIC_MINOR;
	loader->dev.name = "lte_spi";
	loader->dev.fops = &lte_modem_bootloader_fops;
	ret = misc_register(&loader->dev);
	if (ret) {
		pr_err("failed to register misc dev for lte_modem_bootloader\n");
		goto err_setup;
	}
	pr_info("lte_modem_bootloader successfully probed\n");

	factory_mode = 0;

	return 0;

err_setup:
	mutex_destroy(&loader->lock);
	kfree(loader);

err_alloc:

	return ret;
}

static
int __devexit lte_modem_bootloader_remove(struct spi_device *spi)
{
	struct lte_modem_bootloader *loader = spi_get_drvdata(spi);

	misc_deregister(&loader->dev);
	mutex_destroy(&loader->lock);
	kfree(loader);

	return 0;
}

static
struct spi_driver lte_modem_bootloader_driver = {
	.driver = {
		.name = "lte_modem_spi",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
	},
	.probe = lte_modem_bootloader_probe,
	.remove = __devexit_p(lte_modem_bootloader_remove),
};

static
int __init lte_modem_bootloader_init(void)
{
	return spi_register_driver(&lte_modem_bootloader_driver);
}

static
void __exit lte_modem_bootloader_exit(void)
{
	spi_unregister_driver(&lte_modem_bootloader_driver);
}

module_init(lte_modem_bootloader_init);
module_exit(lte_modem_bootloader_exit);

MODULE_DESCRIPTION("LTE Modem Bootloader driver");
MODULE_LICENSE("GPL");
