/* drivers/misc/fm34_we395.c - fm34_we395 voice processor driver
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

#include <linux/i2c/fm34_we395.h>
#include "2mic/fm34_we395.h"

#define MODULE_NAME "[FM34_WE395] :"
#define DEBUG	0

static struct i2c_client *this_client;
static struct fm34_platform_data *pdata;

#if defined(CONFIG_MACH_C1_KOR_LGT) || defined(CONFIG_MACH_BAFFIN_KOR_LGT)
unsigned char bypass_cmd[] = {
/*0xC0,*/
	0xFC, 0xF3, 0x3B, 0x22, 0xC0, 0x00, 0x00,
	0xFC, 0xF3, 0x3B, 0x22, 0xC1, 0x00, 0x01,
	0xFC, 0xF3, 0x3B, 0x22, 0xC2, 0x00, 0x02,
	0xFC, 0xF3, 0x3B, 0x22, 0xC3, 0x00, 0x02,
	0xFC, 0xF3, 0x3B, 0x22, 0xC6, 0x00, 0x7D,
	0xFC, 0xF3, 0x3B, 0x22, 0xC7, 0x00, 0x00,
	0xFC, 0xF3, 0x3B, 0x22, 0xC8, 0x00, 0x18,
	0xFC, 0xF3, 0x3B, 0x22, 0xD2, 0x82, 0x94,
	0xFC, 0xF3, 0x3B, 0x22, 0xEE, 0x00, 0x01,
	0xFC, 0xF3, 0x3B, 0x22, 0xF5, 0x00, 0x03,
	0xFC, 0xF3, 0x3B, 0x22, 0xF6, 0x00, 0x00,
	0xFC, 0xF3, 0x3B, 0x22, 0xF8, 0x80, 0x01,
	0xFC, 0xF3, 0x3B, 0x22, 0xF9, 0x08, 0x7F,
	0xFC, 0xF3, 0x3B, 0x22, 0xFA, 0x24, 0x8B,
	0xFC, 0xF3, 0x3B, 0x23, 0x07, 0x00, 0x00,
	0xFC, 0xF3, 0x3B, 0x23, 0x0A, 0x1A, 0x00,
	0xFC, 0xF3, 0x3B, 0x23, 0x0C, 0x00, 0xB8,
	0xFC, 0xF3, 0x3B, 0x23, 0x0D, 0x02, 0x00,
	0xFC, 0xF3, 0x3B, 0x23, 0x65, 0x08, 0x00,
	0xFC, 0xF3, 0x3B, 0x22, 0xFB, 0x00, 0x00
};
#else
unsigned char bypass_cmd[] = {
/*0xC0,*/
	0xFC, 0xF3, 0x3B, 0x22, 0xF5, 0x00, 0x03,
	0xFC, 0xF3, 0x3B, 0x22, 0xF8, 0x80, 0x03,
	0xFC, 0xF3, 0x3B, 0x22, 0xC6, 0x00, 0x7D,
	0xFC, 0xF3, 0x3B, 0x22, 0xC7, 0x00, 0x00,
	0xFC, 0xF3, 0x3B, 0x22, 0xC8, 0x00, 0x18,
	0xFC, 0xF3, 0x3B, 0x23, 0x0A, 0x1A, 0x00,
	0xFC, 0xF3, 0x3B, 0x22, 0xFA, 0x24, 0x8B,
	0xFC, 0xF3, 0x3B, 0x22, 0xF9, 0x00, 0x7F,
	0xFC, 0xF3, 0x3B, 0x22, 0xF6, 0x00, 0x00,
	0xFC, 0xF3, 0x3B, 0x22, 0xD2, 0x82, 0x94,
	0xFC, 0xF3, 0x3B, 0x22, 0xEE, 0x00, 0x01,
	0xFC, 0xF3, 0x3B, 0x22, 0xFB, 0x00, 0x00,
};
#endif

static int fm34_i2c_read(char *rxData, int length)
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

static int fm34_i2c_write(char *txData, int length)
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

#if defined(CONFIG_MACH_C1_KOR_LGT) || defined(CONFIG_MACH_BAFFIN_KOR_LGT)
void fm34_parameter_reset(void)
{
	pr_info(MODULE_NAME "%s\n", __func__);

	if (pdata->gpio_rst) {
		gpio_set_value(pdata->gpio_rst, 0);
		usleep_range(5000, 5000);
		gpio_set_value(pdata->gpio_rst, 1);
	}
	usleep_range(5000, 5000);

}

int fm34_set_bypass_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;

	pr_info(MODULE_NAME "%s\n", __func__);

	fm34_parameter_reset();

	i2c_cmds = bypass_cmd;
	size = sizeof(bypass_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);

	if (rc < 0)
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);

	return rc;
}

int fm34_set_hw_bypass_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;

	pr_info(MODULE_NAME "%s\n", __func__);

	usleep_range(20000, 20000);
	gpio_set_value(pdata->gpio_pwdn, 0);

	return rc;
}


int fm34_set_loopback_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;
	int val = gpio_get_value(pdata->gpio_pwdn);

	pr_info(MODULE_NAME "%s\n", __func__);

	if (val == 0)
		gpio_set_value(pdata->gpio_pwdn, 1);

	fm34_parameter_reset();

	i2c_cmds = loopback_cmd;
	size = sizeof(loopback_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);

	if (rc < 0)
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);

	return rc;
}

int fm34_set_HS_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;
	int val = gpio_get_value(pdata->gpio_pwdn);

	pr_info(MODULE_NAME "%s\n", __func__);

	if (val == 0)
		gpio_set_value(pdata->gpio_pwdn, 1);

	fm34_parameter_reset();

	i2c_cmds = HS_cmd;
	size = sizeof(HS_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);

	if (rc < 0)
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);

	return rc;
}

int fm34_set_SPK_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;
	int val = gpio_get_value(pdata->gpio_pwdn);

	pr_info(MODULE_NAME "%s\n", __func__);

	if (val == 0)
		gpio_set_value(pdata->gpio_pwdn, 1);

	fm34_parameter_reset();

	i2c_cmds = HF_cmd;
	size = sizeof(HF_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);

	if (rc < 0)
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);

	return rc;
}

int fm34_set_HS_NS_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;
	int val = gpio_get_value(pdata->gpio_pwdn);

	pr_info(MODULE_NAME "%s\n", __func__);

	if (val == 0)
		gpio_set_value(pdata->gpio_pwdn, 1);

	fm34_parameter_reset();

	i2c_cmds = HS_NS_cmd;
	size = sizeof(HS_NS_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);

	if (rc < 0)
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);

	return rc;
}

int fm34_set_HS_ExtraVol_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;
	int val = gpio_get_value(pdata->gpio_pwdn);

	pr_info(MODULE_NAME "%s\n", __func__);

	if (val == 0)
		gpio_set_value(pdata->gpio_pwdn, 1);

	fm34_parameter_reset();

	i2c_cmds = HS_ExtraVol_cmd;
	size = sizeof(HS_ExtraVol_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);

	if (rc < 0)
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);

	return rc;
}

int fm34_set_SPK_ExtraVol_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;
	int val = gpio_get_value(pdata->gpio_pwdn);

	pr_info(MODULE_NAME "%s\n", __func__);

	if (val == 0)
		gpio_set_value(pdata->gpio_pwdn, 1);

	fm34_parameter_reset();

	i2c_cmds = HF_ExtraVol_cmd;
	size = sizeof(HF_ExtraVol_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);

	if (rc < 0)
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);

	return rc;
}

int fm34_set_ExtraVol_NS_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;
	int val = gpio_get_value(pdata->gpio_pwdn);

	pr_info(MODULE_NAME "%s\n", __func__);

	if (val == 0)
		gpio_set_value(pdata->gpio_pwdn, 1);

	fm34_parameter_reset();

	i2c_cmds = HS_ExtraVol_NS_cmd;
	size = sizeof(HS_ExtraVol_NS_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);

	if (rc < 0)
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);

	return rc;
}

int fm34_set_EP_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;
	int val = gpio_get_value(pdata->gpio_pwdn);

	pr_info(MODULE_NAME "%s\n", __func__);

	if (val == 0)
		gpio_set_value(pdata->gpio_pwdn, 1);

	fm34_parameter_reset();

	i2c_cmds = EP_cmd;
	size = sizeof(EP_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);

	if (rc < 0)
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);

	return rc;
}

int fm34_set_BTSCO_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;
	int val = gpio_get_value(pdata->gpio_pwdn);

	pr_info(MODULE_NAME "%s\n", __func__);

	if (val == 0)
		gpio_set_value(pdata->gpio_pwdn, 1);

	fm34_parameter_reset();

	i2c_cmds = BT_SCO_cmd;
	size = sizeof(BT_SCO_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);

	if (rc < 0)
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);

	return rc;
}

int fm34_set_factory_rcv_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;
	int val = gpio_get_value(pdata->gpio_pwdn);

	pr_info(MODULE_NAME "%s\n", __func__);

	if (val == 0)
		gpio_set_value(pdata->gpio_pwdn, 1);

	fm34_parameter_reset();

	i2c_cmds = HS_FACTORY_RCV_cmd;
	size = sizeof(HS_FACTORY_RCV_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);

	if (rc < 0)
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);

	return rc;
}

int fm34_set_factory_spk_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;
	int val = gpio_get_value(pdata->gpio_pwdn);

	pr_info(MODULE_NAME "%s\n", __func__);

	if (val == 0)
		gpio_set_value(pdata->gpio_pwdn, 1);

	fm34_parameter_reset();

	i2c_cmds = HS_FACTORY_SPK_cmd;
	size = sizeof(HS_FACTORY_SPK_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);

	if (rc < 0)
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);

	return rc;
}

int fm34_set_mode(int mode)
{
	int ret = 0;

	pr_info(MODULE_NAME "%s : fm34_set_mode mode[%d]\n", __func__, mode);

	if (mode == 0)
		ret = fm34_set_bypass_mode();  /* OFF,Bypass */
	else if (mode == 1)
		ret = fm34_set_HS_mode(); /* Receiver */
	else if (mode == 2)
		ret = fm34_set_bypass_mode(); /* S/W Bypass */
	else if (mode == 3)
		ret = fm34_set_SPK_mode(); /* Speaker */
	else if (mode == 4)
		ret = fm34_set_hw_bypass_mode(); /* PwrDn H/W Bypass */
	else if (mode == 5)
		ret = fm34_set_HS_NS_mode(); /* Receiver+NS */
	else if (mode == 6)
		ret = fm34_set_HS_ExtraVol_mode(); /* Receiver+ExtraVol */
	else if (mode == 7)
		ret = fm34_set_SPK_ExtraVol_mode(); /* Speaker+ExtraVol */
	else if (mode == 8)
		ret = fm34_set_EP_mode(); /* Headset */
	else if (mode == 9)
		ret = fm34_set_BTSCO_mode(); /* BT SCO */
	else if (mode == 11)
		ret = fm34_set_factory_rcv_mode(); /* Factory Mode RCV */
	else if (mode == 12)
		ret = fm34_set_factory_spk_mode(); /* Factory Mode SPK */
	else if (mode == 13)
		ret = fm34_set_ExtraVol_NS_mode(); /* Receiver+ExtraVol+NS */
	else
		pr_err(MODULE_NAME"fm34_set_mode : INVALID mode[%d]\n", mode);

	return ret;
}
EXPORT_SYMBOL_GPL(fm34_set_mode);
#else
int fm34_set_bypass_mode(void)
{
	int i = 0, rc = 0, size = 0;
	unsigned char *i2c_cmds;

	pr_info(MODULE_NAME "%s\n", __func__);

	i2c_cmds = bypass_cmd;
	size = sizeof(bypass_cmd);

#if DEBUG
	for (i = 0; i < size; i += 1)
		pr_info(MODULE_NAME "%s : i2c_cmds[%d/%d] = 0x%x\n",
			__func__, i, size, i2c_cmds[i]);
#endif
	rc = fm34_i2c_write(i2c_cmds, size);


	if (rc < 0) {
		pr_err(MODULE_NAME "%s failed return %d\n", __func__, rc);
	} else if (pdata->gpio_pwdn) {
		msleep(20);
		gpio_set_value(pdata->gpio_pwdn, 0);
	}

	return rc;
}
#endif

static struct miscdevice fm34_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "fm34_we395",
};

static void fm34_gpio_init(void)
{
	if (pdata->gpio_rst) {
		gpio_request(pdata->gpio_rst, "FM34_RESET");
		gpio_direction_output(pdata->gpio_rst, 1);
		gpio_free(pdata->gpio_rst);
		gpio_set_value(pdata->gpio_rst, 0);
	}

	usleep_range(10000, 10000);

	if (pdata->gpio_pwdn) {
		gpio_request(pdata->gpio_pwdn, "FM34_PWDN");
		gpio_direction_output(pdata->gpio_pwdn, 1);
		gpio_free(pdata->gpio_pwdn);
		gpio_set_value(pdata->gpio_pwdn, 1);
	}

	if (pdata->gpio_bp) {
		gpio_request(pdata->gpio_bp, "FM34_BYPASS");
		gpio_direction_output(pdata->gpio_bp, 1);
		gpio_free(pdata->gpio_bp);
		gpio_set_value(pdata->gpio_bp, 1);
	}
}

static void fm34_bootup_init(void)
{
	if (pdata->set_mclk != NULL)
		pdata->set_mclk(true, false);

	msleep(20);
	if (pdata->gpio_rst) {
		gpio_set_value(pdata->gpio_rst, 0);
		msleep(20);
		gpio_set_value(pdata->gpio_rst, 1);
	}
	msleep(50);
}


static int fm34_probe(
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

	fm34_gpio_init();
	fm34_bootup_init();

	if (fm34_set_bypass_mode() < 0)
		pr_err(MODULE_NAME "bypass setting failed %d\n", rc);

	pr_info(MODULE_NAME "%s : finish\n", __func__);

	return rc;
}

static int fm34_remove(struct i2c_client *client)
{
	struct fm34_platform_data *pfm34data = i2c_get_clientdata(client);
	kfree(pfm34data);

	return 0;
}

static int fm34_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return 0;
}

static int fm34_resume(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id fm34_id[] = {
	{ "fm34_we395", 0 },
	{ }
};

static struct i2c_driver fm34_driver = {
	.probe = fm34_probe,
	.remove = fm34_remove,
	.suspend = fm34_suspend,
	.resume	= fm34_resume,
	.id_table = fm34_id,
	.driver = {
		.name = "fm34_we395",
	},
};

static int __init fm34_init(void)
{
	pr_info("%s\n", __func__);

	return i2c_add_driver(&fm34_driver);
}

static void __exit fm34_exit(void)
{
	i2c_del_driver(&fm34_driver);
}

module_init(fm34_init);
module_exit(fm34_exit);

MODULE_DESCRIPTION("fm34 voice processor driver");
MODULE_LICENSE("GPL");
