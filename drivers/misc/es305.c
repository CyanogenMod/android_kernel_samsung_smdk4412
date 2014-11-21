/* drivers/misc/es305.c - audience ES305 voice processor driver
 *
 * Copyright (C) 2012 Samsung Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/firmware.h>
#include <linux/i2c/es305.h>

#define ES305_FIRMWARE_NAME	"audience/es305_fw.bin"

static struct i2c_client *this_client;
static struct es305_platform_data *pdata;
static struct task_struct *task;

unsigned char es305_reset_cmd[] = {
	0x80, 0x02, 0x00, 0x00,
};

unsigned char es305_sync_cmd[] = {
	0x80, 0x00, 0x00, 0x00,
};

unsigned char es305_boot_cmd[] = {
	0x00, 0x01,
};

unsigned char es305_sleep_cmd[] = {
	0x80, 0x10, 0x00, 0x01,
};

unsigned char es305_bypass_data[] = {
	0x80, 0x52, 0x00, 0x4C,
	0x80, 0x10, 0x00, 0x01,
};

static int es305_i2c_read(char *rxData, int length)
{
	int rc;
	struct i2c_msg msgs[] = {
		{
			.addr = this_client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = rxData,
		},
	};

	rc = i2c_transfer(this_client->adapter, msgs, 1);
	if (rc < 0) {
		pr_err("%s: transfer error %d\n", __func__, rc);
		return rc;
	}
	return 0;
}

static int es305_i2c_write(char *txData, int length)
{
	int rc;
	struct i2c_msg msg[] = {
		{
			.addr = this_client->addr,
			.flags = 0,
			.len = length,
			.buf = txData,
		},
	};

	rc = i2c_transfer(this_client->adapter, msg, 1);
	if (rc < 0) {
		pr_err("%s: transfer error %d\n", __func__, rc);
		return rc;
	}

	return 0;
}

int es305_set_cmd(enum es305_cmd cmd)
{
#if DEBUG
	int i = 0;
#endif
	int rc = 0, size = 0;
	unsigned char *i2c_cmds = NULL;

	pr_info(MODULE_NAME "%s : cmd %d\n", __func__, cmd);
	switch (cmd) {
	case ES305_SW_RESET:
		i2c_cmds = es305_reset_cmd;
		size = sizeof(es305_reset_cmd);
		break;
	case ES305_SYNC:
		i2c_cmds = es305_sync_cmd;
		size = sizeof(es305_sync_cmd);
		break;
	case ES305_BOOT:
		i2c_cmds = es305_boot_cmd;
		size = sizeof(es305_boot_cmd);
		break;
	case ES305_SLEEP:
		i2c_cmds = es305_sleep_cmd;
		size = sizeof(es305_sleep_cmd);
		break;
	case ES305_BYPASS_DATA:
		i2c_cmds = es305_bypass_data;
		size = sizeof(es305_bypass_data);
		break;
	default:
		pr_err(MODULE_NAME "%s : unknown cmd\n", __func__);
		break;
	}

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif

	rc = es305_i2c_write(i2c_cmds, size);
	if (rc < 0)
		pr_err(MODULE_NAME "%s failed %d\n", __func__, rc);

	return rc;
}


int es305_sw_reset(void)
{
	int rc = 0, size = 0;
	unsigned char *i2c_cmds;

	pr_info(MODULE_NAME "%s\n", __func__);

	i2c_cmds = es305_reset_cmd;
	size = sizeof(es305_reset_cmd);

	rc = es305_i2c_write(i2c_cmds, size);
	if (rc < 0)
		pr_err(MODULE_NAME "%s failed %d\n", __func__, rc);

	msleep(50);

	return rc;
}

int es305_hw_reset(void)
{
	int rc = 0;
	int retry = RETRY_CNT;
	unsigned char msgbuf[4] = {0,};

	pr_info(MODULE_NAME "%s\n", __func__);

	while (retry--) {
		/* Reset ES305 chip */
		gpio_set_value(pdata->gpio_reset, 0);

		/* Enable ES305 clock */
		if (pdata->set_mclk != NULL)
			pdata->set_mclk(true, false);
		mdelay(1);

		gpio_set_value(pdata->gpio_reset, 1);

		msleep(50); /* Delay before send I2C command */

		rc = es305_set_cmd(ES305_BOOT);
		if (rc < 0) {
			pr_err(MODULE_NAME "%s: set boot mode error\n",
					__func__);
			continue;
		}

		rc = es305_i2c_read(msgbuf, 1);
		if (rc < 0) {
			pr_err(MODULE_NAME "%s: boot mode ack error (%d retries left)\n",
					__func__, retry);
			continue;
		}

		mdelay(1);
		rc = es305_load_firmware();
		if (rc < 0) {
			pr_err(MODULE_NAME "%s: load firmware error\n",
					__func__);
			continue;
		}

		msleep(20); /* Delay time before issue a Sync Cmd */

		rc = es305_set_cmd(ES305_SYNC);
		if (rc < 0) {
			pr_err(MODULE_NAME "%s: set sync error\n",
					__func__);
			continue;
		}

		rc = es305_i2c_read(msgbuf, 4);
		if (rc < 0) {
			pr_err(MODULE_NAME "%s: boot mode ack error (%d retries left)\n",
					__func__, retry);
			continue;
		}

		break;
	}
	return rc;
}

static int es305_bootup_init(void *dummy)
{
	int rc = 0;

	pr_info(MODULE_NAME "%s\n", __func__);

	rc = es305_hw_reset();
	if (rc < 0) {
		pr_err(MODULE_NAME "%s: reset error %d\n",
					__func__, rc);
		return rc;
	}

	rc = es305_set_cmd(ES305_BYPASS_DATA);
	if (rc < 0) {
		pr_err(MODULE_NAME "%s: set bypass error %d\n",
					__func__, rc);
		return rc;
	}
	return rc;
}

static void es305_gpio_init(void)
{
	if (pdata->gpio_wakeup) {
		gpio_request(pdata->gpio_wakeup, "ES305_WAKEUP");
		gpio_direction_output(pdata->gpio_wakeup, 1);
		gpio_free(pdata->gpio_wakeup);
		gpio_set_value(pdata->gpio_wakeup, 1);
	}

	if (pdata->gpio_reset) {
		gpio_request(pdata->gpio_reset, "ES305_RESET");
		gpio_direction_output(pdata->gpio_reset, 1);
		gpio_free(pdata->gpio_reset);
		gpio_set_value(pdata->gpio_reset, 1);
	}
}

int es305_load_firmware(void)
{
	int rc = 0, i = 0;
	size_t size = 0;
	const struct firmware *fw = NULL;
	unsigned char *i2c_cmds;

	pr_info(MODULE_NAME "%s : start\n", __func__);

	rc = request_firmware(&fw, ES305_FIRMWARE_NAME,
				  &this_client->dev);
	if (rc < 0) {
		pr_err(MODULE_NAME "%s : unable to open firmware ret %d\n",
				__func__, rc);
		release_firmware(fw);
		return rc;
	}
	i2c_cmds = (unsigned char *)fw->data;
	size = fw->size;

	for (i = 0; i < (size/32); i++, i2c_cmds += 32) {
		rc = es305_i2c_write(i2c_cmds, 32);
		if (rc < 0) {
			pr_err(MODULE_NAME "%s failed %d\n", __func__, rc);
			break;
		}
	}

	if (size%32) {
		rc = es305_i2c_write(i2c_cmds, size%32);
		if (rc < 0)
			pr_err(MODULE_NAME "%s failed %d\n", __func__, rc);
	}

	release_firmware(fw);

	pr_info(MODULE_NAME "%s : end\n", __func__);
	return rc;
}


static int es305_probe(
		struct i2c_client *client, const struct i2c_device_id *id)
{
	int rc = 0;
	pdata = client->dev.platform_data;

	pr_info(MODULE_NAME "%s : start\n", __func__);

	if (pdata == NULL) {
		pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
		if (pdata == NULL) {
			rc = -ENOMEM;
			pr_err(MODULE_NAME "%s: platform data is NULL\n",
					__func__);
		}
	}

	this_client = client;

	es305_gpio_init();
	task = kthread_run(es305_bootup_init, NULL, "es305_bootup_init");
	if (IS_ERR(task)) {
		rc = PTR_ERR(task);
		task = NULL;
	}

	pr_info(MODULE_NAME "%s : finish\n", __func__);

	return rc;
}

static int es305_remove(struct i2c_client *client)
{
	struct es305_platform_data *pes305data = i2c_get_clientdata(client);
	kfree(pes305data);

	return 0;
}

static int es305_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return 0;
}

static int es305_resume(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id es305_id[] = {
	{ "audience_es305", 0 },
	{ }
};

static struct i2c_driver es305_driver = {
	.probe = es305_probe,
	.remove = es305_remove,
	.suspend = es305_suspend,
	.resume	= es305_resume,
	.id_table = es305_id,
	.driver = {
		.name = "audience_es305",
	},
};

static int __init es305_init(void)
{
	pr_info("%s\n", __func__);

	return i2c_add_driver(&es305_driver);
}

static void __exit es305_exit(void)
{
	i2c_del_driver(&es305_driver);
}

module_init(es305_init);
module_exit(es305_exit);

MODULE_DESCRIPTION("audience es305 voice processor driver");
MODULE_LICENSE("GPL");
