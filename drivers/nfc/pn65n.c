/*
 * Copyright (C) 2010 Trusted Logic S.A.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/nfc/pn65n.h>

#define MAX_BUFFER_SIZE		512

#define NXP_KR_READ_IRQ_MODIFY

#ifdef NXP_KR_READ_IRQ_MODIFY
static bool do_reading;
static bool cancle_read;
#endif

#define NFC_DEBUG		0
#define MAX_TRY_I2C_READ	10
#define I2C_ADDR_READ_L		0x51
#define I2C_ADDR_READ_H		0x57


struct pn65n_dev	{
	wait_queue_head_t	read_wq;
	struct mutex		read_mutex;
	struct i2c_client	*client;
	struct miscdevice	pn65n_device;
	unsigned int		ven_gpio;
	unsigned int		firm_gpio;
	unsigned int		irq_gpio;
	atomic_t		irq_enabled;
};

static irqreturn_t pn65n_dev_irq_handler(int irq, void *dev_id)
{
	struct pn65n_dev *pn65n_dev = dev_id;

	if (!gpio_get_value(pn65n_dev->irq_gpio)) {
#if NFC_DEBUG
		pr_err("%s, irq_gpio = %d\n", __func__,
		gpio_get_value(pn65n_dev->irq_gpio));
#endif
		return IRQ_HANDLED;
	}

#ifdef NXP_KR_READ_IRQ_MODIFY
	do_reading = true;
#endif
	/* Wake up waiting readers */
	wake_up(&pn65n_dev->read_wq);

#if NFC_DEBUG
	pr_info("%s, IRQ_HANDLED\n", __func__);
#endif

	return IRQ_HANDLED;
}

static ssize_t pn65n_dev_read(struct file *filp, char __user *buf,
			      size_t count, loff_t *offset)
{
	struct pn65n_dev *pn65n_dev = filp->private_data;
	char tmp[MAX_BUFFER_SIZE];
	int ret = 0;
	int readingWatchdog = 0;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

#if NFC_DEBUG
	dev_info(&pn65n_dev->client->dev, "%s : reading %zu bytes. irq=%s\n",
		__func__, count,
		gpio_get_value(pn65n_dev->irq_gpio) ? "1" : "0");
	dev_info(&pn65n_dev->client->dev, "pn65n : + r\n");
#endif

	mutex_lock(&pn65n_dev->read_mutex);

wait_irq:
	if (!gpio_get_value(pn65n_dev->irq_gpio)) {
#ifdef NXP_KR_READ_IRQ_MODIFY
		do_reading = false;
#endif
		if (filp->f_flags & O_NONBLOCK) {
			dev_info(&pn65n_dev->client->dev, "%s : O_NONBLOCK\n",
				 __func__);
			ret = -EAGAIN;
			goto fail;
		}
#if NFC_DEBUG
		dev_info(&pn65n_dev->client->dev,
			"wait_event_interruptible : in\n");
#endif

#ifdef NXP_KR_READ_IRQ_MODIFY
		ret = wait_event_interruptible(pn65n_dev->read_wq,
			do_reading);
#else
		ret = wait_event_interruptible(pn65n_dev->read_wq,
			gpio_get_value(pn65n_dev->irq_gpio));
#endif

#if NFC_DEBUG
		dev_info(&pn65n_dev->client->dev,
			"wait_event_interruptible : out\n");
#endif

#ifdef NXP_KR_READ_IRQ_MODIFY
		if (cancle_read == true) {
			cancle_read = false;
			ret = -1;
			goto fail;
		}
#endif

		if (ret)
			goto fail;
	}

	/* Read data */
	ret = i2c_master_recv(pn65n_dev->client, tmp, count);

	/* If bad frame(from 0x51 to 0x57) is received from pn65n,
	* we need to read again after waiting that IRQ is down.
	* if data is not ready, pn65n will send from 0x51 to 0x57. */
	if ((I2C_ADDR_READ_L <= tmp[0] && tmp[0] <= I2C_ADDR_READ_H)
		&& readingWatchdog < MAX_TRY_I2C_READ) {
		pr_warn("%s: data is not ready yet.data = 0x%x, cnt=%d\n",
			__func__, tmp[0], readingWatchdog);
		usleep_range(2000, 2000); /* sleep 2ms to wait for IRQ */
		readingWatchdog++;
		goto wait_irq;
	}

	mutex_unlock(&pn65n_dev->read_mutex);

	if (ret < 0) {
		dev_err(&pn65n_dev->client->dev,
			"%s: i2c_master_recv returned %d\n",
				__func__, ret);
		return ret;
	}
	if (ret > count) {
		dev_err(&pn65n_dev->client->dev,
			"%s: received too many bytes from i2c (%d)\n",
			__func__, ret);
		return -EIO;
	}

	if (copy_to_user(buf, tmp, ret)) {
		dev_err(&pn65n_dev->client->dev,
			"%s : failed to copy to user space\n",
			__func__);
		return -EFAULT;
	}
	return ret;

fail:
	mutex_unlock(&pn65n_dev->read_mutex);
	return ret;
}

static ssize_t pn65n_dev_write(struct file *filp, const char __user *buf,
			       size_t count, loff_t *offset)
{
	struct pn65n_dev *pn65n_dev;
	char tmp[MAX_BUFFER_SIZE];
	int ret = 0, retry = 2;
#if NFC_DEBUG
	int i = 0;
#endif

	pn65n_dev = filp->private_data;

#if NFC_DEBUG
	dev_info(&pn65n_dev->client->dev, "pn65n : + w\n");
	for (i = 0; i < count; i++)
		dev_info(&pn65n_dev->client->dev, "buf[%d] = 0x%x\n",
			i, buf[i]);
#endif

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	if (copy_from_user(tmp, buf, count)) {
		dev_err(&pn65n_dev->client->dev,
			"%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}
#if NFC_DEBUG
	dev_info(&pn65n_dev->client->dev, "%s : writing %zu bytes.\n", __func__,
		count);
#endif
	/* Write data */
	do {
		retry--;
		ret = i2c_master_send(pn65n_dev->client, tmp, count);
		if (ret == count)
			break;
		usleep_range(6000, 10000); /* Retry, chip was in standby */
#if NFC_DEBUG
		dev_info(&pn65n_dev->client->dev, "retry = %d\n", retry);
#endif
	} while (retry);

#if NFC_DEBUG
	dev_info(&pn65n_dev->client->dev, "pn65n : - w\n");
#endif

	if (ret != count) {
		dev_err(&pn65n_dev->client->dev,
			"%s : i2c_master_send returned %d, %d\n",
			__func__, ret, retry);
		ret = -EIO;
	}

	return ret;
}

static int pn65n_dev_open(struct inode *inode, struct file *filp)
{
	struct pn65n_dev *pn65n_dev = container_of(filp->private_data,
						   struct pn65n_dev,
						   pn65n_device);

	filp->private_data = pn65n_dev;

	dev_info(&pn65n_dev->client->dev, "%s : %d,%d\n", __func__,
		imajor(inode), iminor(inode));

	return 0;
}

static long pn65n_dev_ioctl(struct file *filp,
			    unsigned int cmd, unsigned long arg)
{
	struct pn65n_dev *pn65n_dev = filp->private_data;

	switch (cmd) {
	case PN65N_SET_PWR:
		if (arg == 2) {
			/* power on with firmware download (requires hw reset)
			 */
			gpio_set_value(pn65n_dev->ven_gpio, 1);
			gpio_set_value(pn65n_dev->firm_gpio, 1);
			usleep_range(10000, 10000);
			gpio_set_value(pn65n_dev->ven_gpio, 0);
			usleep_range(10000, 10000);
			gpio_set_value(pn65n_dev->ven_gpio, 1);
			usleep_range(10000, 10000);
			if (atomic_read(&pn65n_dev->irq_enabled) == 0) {
				atomic_set(&pn65n_dev->irq_enabled, 1);
				enable_irq(pn65n_dev->client->irq);
				enable_irq_wake(pn65n_dev->client->irq);
			}
			dev_info(&pn65n_dev->client->dev,
				 "%s power on with firmware, irq=%d\n",
				 __func__,
				 atomic_read(&pn65n_dev->irq_enabled));
		} else if (arg == 1) {
			/* power on */
			gpio_set_value(pn65n_dev->firm_gpio, 0);
			gpio_set_value(pn65n_dev->ven_gpio, 1);
			usleep_range(10000, 10000);
			if (atomic_read(&pn65n_dev->irq_enabled) == 0) {
				atomic_set(&pn65n_dev->irq_enabled, 1);
				enable_irq(pn65n_dev->client->irq);
				enable_irq_wake(pn65n_dev->client->irq);
			}
			dev_info(&pn65n_dev->client->dev,
				"%s power on, irq=%d\n", __func__,
				atomic_read(&pn65n_dev->irq_enabled));
		} else if (arg == 0) {
			/* power off */
			if (atomic_read(&pn65n_dev->irq_enabled) == 1) {
				disable_irq_wake(pn65n_dev->client->irq);
				disable_irq_nosync(pn65n_dev->client->irq);
				atomic_set(&pn65n_dev->irq_enabled, 0);
			}
			dev_info(&pn65n_dev->client->dev, "%s power off, irq=%d\n",
				 __func__,
				 atomic_read(&pn65n_dev->irq_enabled));
			gpio_set_value(pn65n_dev->firm_gpio, 0);
			gpio_set_value(pn65n_dev->ven_gpio, 0);
			usleep_range(10000, 10000);
#ifdef NXP_KR_READ_IRQ_MODIFY
		} else if (arg == 3) {
			pr_info("%s Read Cancle\n", __func__);
			cancle_read = true;
			do_reading = true;
			wake_up(&pn65n_dev->read_wq);
#endif
		} else {
			dev_err(&pn65n_dev->client->dev, "%s bad arg %lu\n",
				__func__, arg);
			return -EINVAL;
		}
		break;
	default:
		dev_err(&pn65n_dev->client->dev, "%s bad ioctl %u\n", __func__,
			cmd);
		return -EINVAL;
	}

	return 0;
}

static const struct file_operations pn65n_dev_fops = {
	.owner	= THIS_MODULE,
	.llseek	= no_llseek,
	.read	= pn65n_dev_read,
	.write	= pn65n_dev_write,
	.open	= pn65n_dev_open,
	.unlocked_ioctl  = pn65n_dev_ioctl,
};

static int pn65n_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	int ret;
	struct pn65n_i2c_platform_data *platform_data;
	struct pn65n_dev *pn65n_dev;

	platform_data = client->dev.platform_data;

	if (platform_data == NULL) {
		dev_err(&client->dev, "%s : nfc probe fail\n", __func__);
		return -ENODEV;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "%s : need I2C_FUNC_I2C\n", __func__);
		return -ENODEV;
	}

	ret = gpio_request(platform_data->irq_gpio, "nfc_int");
	if (ret)
		return -ENODEV;
	ret = gpio_request(platform_data->ven_gpio, "nfc_ven");
	if (ret)
		goto err_ven;
	ret = gpio_request(platform_data->firm_gpio, "nfc_firm");
	if (ret)
		goto err_firm;

	pn65n_dev = kzalloc(sizeof(*pn65n_dev), GFP_KERNEL);
	if (pn65n_dev == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	dev_info(&client->dev, "%s : IRQ num %d\n", __func__, client->irq);

	pn65n_dev->irq_gpio = platform_data->irq_gpio;
	pn65n_dev->ven_gpio = platform_data->ven_gpio;
	pn65n_dev->firm_gpio = platform_data->firm_gpio;
	pn65n_dev->client = client;

	/* init mutex and queues */
	init_waitqueue_head(&pn65n_dev->read_wq);
	mutex_init(&pn65n_dev->read_mutex);

	pn65n_dev->pn65n_device.minor = MISC_DYNAMIC_MINOR;
	pn65n_dev->pn65n_device.name = "pn544";
	pn65n_dev->pn65n_device.fops = &pn65n_dev_fops;

	ret = misc_register(&pn65n_dev->pn65n_device);
	if (ret) {
		dev_err(&client->dev, "%s : misc_register failed. ret = %d\n",
			__FILE__, ret);
		goto err_misc_register;
	}

	i2c_set_clientdata(client, pn65n_dev);
	/* request irq.  the irq is set whenever the chip has data available
	 * for reading.  it is cleared when all data has been read.
	 */
	dev_info(&pn65n_dev->client->dev, "%s : requesting IRQ %d\n", __func__,
		 client->irq);
	ret = gpio_direction_input(pn65n_dev->irq_gpio);
	if (ret) {
		dev_err(&client->dev, "%s : gpio_direction_input failed. ret = %d\n",
			__FILE__, ret);
		goto err_request_irq_failed;
	}

	ret = request_irq(client->irq, pn65n_dev_irq_handler,
			  IRQF_TRIGGER_RISING, "pn65n", pn65n_dev);
	if (ret) {
		dev_err(&client->dev, "request_irq failed. ret = %d\n", ret);
		goto err_request_irq_failed;
	}
	disable_irq_nosync(pn65n_dev->client->irq);
	atomic_set(&pn65n_dev->irq_enabled, 0);

	return 0;

err_request_irq_failed:
	misc_deregister(&pn65n_dev->pn65n_device);
err_misc_register:
	mutex_destroy(&pn65n_dev->read_mutex);
	kfree(pn65n_dev);
err_exit:
	gpio_free(platform_data->firm_gpio);
err_firm:
	gpio_free(platform_data->ven_gpio);
err_ven:
	gpio_free(platform_data->irq_gpio);
	return ret;
}

static int pn65n_remove(struct i2c_client *client)
{
	struct pn65n_dev *pn65n_dev;

	pn65n_dev = i2c_get_clientdata(client);
	free_irq(client->irq, pn65n_dev);
	misc_deregister(&pn65n_dev->pn65n_device);
	mutex_destroy(&pn65n_dev->read_mutex);
	gpio_free(pn65n_dev->irq_gpio);
	gpio_free(pn65n_dev->ven_gpio);
	gpio_free(pn65n_dev->firm_gpio);
	kfree(pn65n_dev);

	return 0;
}

static const struct i2c_device_id pn65n_id[] = {
	{ "pn65n", 0 },
	{ }
};

static struct i2c_driver pn65n_driver = {
	.id_table	= pn65n_id,
	.probe		= pn65n_probe,
	.remove		= pn65n_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "pn65n",
	},
};

/*
 * module load/unload record keeping
 */

static int __init pn65n_dev_init(void)
{
	pr_info("Loading pn65n driver\n");
	return i2c_add_driver(&pn65n_driver);
}
module_init(pn65n_dev_init);

static void __exit pn65n_dev_exit(void)
{
	pr_info("Unloading pn65n driver\n");
	i2c_del_driver(&pn65n_driver);
}
module_exit(pn65n_dev_exit);

MODULE_AUTHOR("Sylvain Fonteneau");
MODULE_DESCRIPTION("NFC pn65n driver");
MODULE_LICENSE("GPL");
