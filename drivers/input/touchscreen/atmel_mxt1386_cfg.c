/*
 *  atmel_maxtouch.c - Atmel maXTouch Touchscreen Controller
 *
 *  Version 0.2a
 *
 *  An early alpha version of the maXTouch Linux driver.
 *
 *
 *  Copyright (C) 2010 Iiro Valkonen <iiro.valkonen@atmel.com>
 *  Copyright (C) 2009 Ulf Samuelsson <ulf.samuelsson@atmel.com>
 *  Copyright (C) 2009 Raphael Derosso Pereira <raphaelpereira@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <linux/firmware.h>
#include <linux/wakelock.h>

#include <linux/delay.h>
#include <linux/i2c.h>

#include <linux/atmel_mxt1386.h>
#include "atmel_mxt1386_cfg.h"

#define READ_FW_FROM_HEADER 1

u8 firmware_latest[] = {
	/*#include "mxt1386_fw_ver07.h"*/
	/*#include "mxt1386_fw_ver09.h"*/
	#include "mxt1386_fw_ver10.h"
};

int mxt_get_object_values(struct mxt_data *mxt, u8 *buf, int obj_type)
{
	u8 *obj = NULL;
	u16 obj_size = 0;

	switch (obj_type) {
	case MXT_GEN_POWERCONFIG_T7:
		obj = (u8 *)&mxt->pdata->power_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;
	case MXT_GEN_ACQUIRECONFIG_T8:
		obj = (u8 *)&mxt->pdata->acquisition_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;
	case MXT_TOUCH_MULTITOUCHSCREEN_T9:
		obj = (u8 *)&mxt->pdata->touchscreen_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;
	case MXT_PROCG_NOISESUPPRESSION_T22:
		obj = (u8 *)&mxt->pdata->noise_suppression_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;
	case MXT_SPT_CTECONFIG_T28:
		obj = (u8 *)&mxt->pdata->cte_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;
	default:
		pr_err("Not supporting object type (object type: %d)",
			obj_type);
		return -1;
	}
	pr_info("obj type: %d, obj size: %d", obj_type, obj_size);

	if (memcpy(buf, obj, obj_size) == NULL) {
		pr_err("memcpy failed!");
		return -1;
	}
	return 0;
}

int mxt_power_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;

	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7);
	obj_size = MXT_GET_SIZE(MXT_GEN_POWERCONFIG_T7);
	/*pr_info("address: 0x%x, size: %d", obj_addr, obj_size);*/
	/*power_config initial values are got from platform_data*/

	error = mxt_write_block(client, obj_addr,
			obj_size, (u8 *)&mxt->pdata->power_config);
	if (error < 0) {
		dev_err(&client->dev, "mxt_write_byte failed!\n");
		return -EIO;
	}
	return 0;
}

static int mxt_acquisition_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8);
	obj_size = MXT_GET_SIZE(MXT_GEN_ACQUIRECONFIG_T8);
	/*pr_info("address: 0x%x, size: %d", obj_addr, obj_size);*/
	/*acquisition_config initial values are got from platform_data*/

	error = mxt_write_block(client, obj_addr,
		obj_size, (u8 *)&mxt->pdata->acquisition_config);
	if (error < 0) {
		dev_err(&client->dev, "mxt_write_byte failed!\n");
		return -EIO;
	}
	return 0;
}

int mxt_multitouch_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9);
	obj_size = MXT_GET_SIZE(MXT_TOUCH_MULTITOUCHSCREEN_T9);
	/*pr_info("address: 0x%x, size: %d", obj_addr, obj_size);*/
	/*touchscreen_config initial values are got from platform_data*/

	error = mxt_write_block(client, obj_addr,
		obj_size, (u8 *)&mxt->pdata->touchscreen_config);
	if (error < 0) {
		dev_err(&client->dev, "mxt_write_byte failed!\n");
		return -EIO;
	}
	return 0;
}

static int mxt_noise_suppression_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T22);
	obj_size = MXT_GET_SIZE(MXT_PROCG_NOISESUPPRESSION_T22);
	/*pr_info("address: 0x%x, size: %d", obj_addr, obj_size);*/
	/*noise_suppression_config initial values are got from platform_data*/

	error = mxt_write_block(client, obj_addr,
			obj_size,
			(u8 *)&mxt->pdata->noise_suppression_config);
	if (error < 0) {
		dev_err(&client->dev, "mxt_write_byte failed!\n");
		return -EIO;
	}
	return 0;
}

static int mxt_cte_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T28);
	obj_size = MXT_GET_SIZE(MXT_SPT_CTECONFIG_T28);
	/*pr_info("address: 0x%x, size: %d", obj_addr, obj_size);*/
	/*cte_config initial values are got from platform_data*/

	/*mxt_write_block(client, 0x01b0, 6, (u8*)&mxt->pdata->cte_config);*/
	error = mxt_write_block(client, obj_addr,
			obj_size,
			(u8 *)&mxt->pdata->cte_config);
	if (error < 0) {
		dev_err(&client->dev, "mxt_write_byte failed!\n");
		return -EIO;
	}

	return 0;
}

static int mxt_gripsuppression_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_PROCI_GRIPSUPPRESSION_T40);
	obj_size = MXT_GET_SIZE(MXT_PROCI_GRIPSUPPRESSION_T40);
	/*pr_info("address: 0x%x, size: %d", obj_addr, obj_size);*/
	/*gripsupression_config initial values are got from platform_data*/

	error = mxt_write_block(client, obj_addr,
		obj_size, (u8 *)&mxt->pdata->gripsupression_config);
	if (error < 0) {
		dev_err(&client->dev, "mxt_write_byte failed!\n");
		return -EIO;
	}

	return 0;
}

static int mxt_palmsuppression_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_PROCI_PALMSUPPRESSION_T41);
	obj_size = MXT_GET_SIZE(MXT_PROCI_PALMSUPPRESSION_T41);
	/*pr_info("address: 0x%x, size: %d", obj_addr, obj_size);*/
	/*palmsupression_config initial values are got from platform_data*/

	error = mxt_write_block(client, obj_addr,
		obj_size, (u8 *)&mxt->pdata->palmsupression_config);
	if (error < 0) {
		dev_err(&client->dev, "mxt_write_byte failed!\n");
		return -EIO;
	}

	return 0;
}

static int mxt_other_configs(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error = 0, i = 0;
	u8 addr = 0;
	u8 obj_data[50];
	u8 obj_type[MXT_MAX_OBJECT_TYPES] = {
		MXT_TOUCH_KEYARRAY_T15,
		MXT_SPT_COMMSCONFIG_T18,
		MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24,
		MXT_SPT_SELFTEST_T25,
		MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27,
		MXT_SPT_DIGITIZER_T43,
		0, };

	for (i = 0; i < 50; i++)
		obj_data[i] = 0;

	for (i = 0; i < MXT_MAX_OBJECT_TYPES; i++) {
		if (0 == obj_type[i])
			break;
		else
			addr = obj_type[i];
		obj_addr = MXT_BASE_ADDR(addr);
		obj_size = MXT_GET_SIZE(addr);

		if (MXT_TOUCH_KEYARRAY_T15 == addr)
			obj_size *= 2;

		error = mxt_write_block(client, obj_addr,
			obj_size, (u8 *)&obj_data);

		if (error < 0) {
			dev_err(&client->dev, "mxt_write_byte failed!\n");
			return -EIO;
		}
	}
	return 0;
}

int mxt_config_settings(struct mxt_data *mxt)
{
	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	if (mxt_power_config(mxt) < 0)
		return -1;
	if (mxt_acquisition_config(mxt) < 0)
		return -1;
	if (mxt_multitouch_config(mxt) < 0)
		return -1;
	if (mxt_noise_suppression_config(mxt) < 0)
		return -1;
	if (mxt_cte_config(mxt) < 0)
		return -1;
	if (mxt_gripsuppression_config(mxt) < 0)
		return -1;
	if (mxt_palmsuppression_config(mxt) < 0)
		return -1;
	if (mxt_other_configs(mxt) < 0)
		return -1;

	/* backup to nv memory */
	backup_to_nv(mxt);
	/* forces a reset of the chipset */
	reset_chip(mxt, RESET_TO_NORMAL);
	msleep(250); /*mxt1386 need 250ms*/

	return 0;
}

/*
 * Bootloader functions
 */

static void bootloader_status(u8 value)
{
	u8 *str = NULL;

	switch (value) {
	case 0xC0:
		str = "WAITING_BOOTLOAD_CMD"; break;
	case 0x80:
		str = "WAITING_FRAME_DATA"; break;
	case 0x40:
		str = "APP_CRC_FAIL"; break;
	case 0x02:
		str = "FRAME_CRC_CHECK"; break;
	case 0x03:
		str = "FRAME_CRC_FAIL"; break;
	case 0x04:
		str = "FRAME_CRC_PASS"; break;
	default:
		str = "Unknown Status";
	}
	pr_info("bootloader status: %s (0x%02X)\n", str, value);
}

static int check_bootloader(struct i2c_client *client, unsigned int status)
{
	u8 val = 0;
	u16 retry = 0;

	usleep_range(10000, 20000);  /* recommendation from ATMEL*/

recheck:
	if (i2c_master_recv(client, &val, 1) != 1) {
		pr_err("i2c recv failed");
		return -EIO;
	}

	switch (status) {
	case WAITING_BOOTLOAD_COMMAND:
	case WAITING_FRAME_DATA:
		val &= ~BOOTLOAD_STATUS_MASK;
		bootloader_status(val);
		if (val == APP_CRC_FAIL) {
			pr_info("We've got a APP_CRC_FAIL, so try again (count=%d)",
				++retry);
			goto recheck;
		}
		break;

	case FRAME_CRC_PASS:
		bootloader_status(val);
		if (val == FRAME_CRC_CHECK)
			goto recheck;
		break;

	default:
		return -EINVAL;
	}

	if (val != status) {
		pr_err("Invalid status: 0x%02X ", val);
		return -EINVAL;
	}

	return 0;
}

static int unlock_bootloader(struct i2c_client *client)
{
	u8 cmd[2] = {0};

	cmd[0] = 0xdc;  /*MXT_CMD_UNLOCK_BL_LSB*/
	cmd[1] = 0xaa;  /*MXT_CMD_UNLOCK_BL_MSB*/

	return i2c_master_send(client, cmd, 2);
}

int mxt_check_firmware(struct device *dev, int *ver)
{
#if READ_FW_FROM_HEADER
	struct firmware *fw = NULL;
	fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);

	fw->data = firmware_latest;
	fw->size = sizeof(firmware_latest);
	/*pr_info("size of firmware: %d", fw->size);*/
#else
	int ret;
	const struct firmware *fw = NULL;

	ret = request_firmware(&fw, fn, dev);
	if (ret < 0) {
		pr_err("Unable to open firmware - %s : %s\n", __func__, fn);
		return -ENOMEM;
	}
#endif
	*ver++ = fw->data[0];
	*ver = fw->data[1];

	kfree(fw);

	return 0;
}
int mxt_load_firmware(struct device *dev, const char *fn)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);

	unsigned int frame_size;
	unsigned int pos = 0;
	unsigned int retry;
	int ret;

#if READ_FW_FROM_HEADER
	struct firmware *fw = NULL;
	fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);

	fw->data = firmware_latest;
	fw->size = sizeof(firmware_latest);
	/*pr_info("size of firmware: %d", fw->size);*/
#else
	const struct firmware *fw = NULL;

	ret = request_firmware(&fw, fn, dev);
	if (ret < 0) {
		dev_err(&client->dev, "Unable to open firmware %s\n", fn);
		return -ENOMEM;
	}
#endif

	/* set resets into bootloader mode */
	reset_chip(mxt, RESET_TO_BOOTLOADER);
	msleep(250);/*mdelay(100);*/

	/* change to slave address of bootloader */
	if (mxt->client->addr == MXT_I2C_APP_ADDR) {
		pr_info("I2C address: 0x%02X --> 0x%02X",
			MXT_I2C_APP_ADDR, MXT_I2C_BOOTLOADER_ADDR);
		mxt->client->addr = MXT_I2C_BOOTLOADER_ADDR;
	}

	ret = check_bootloader(mxt->client, WAITING_BOOTLOAD_COMMAND);
	if (ret < 0) {
		pr_err("... Waiting bootloader command: Failed");
		goto err_fw;
	}

	/* unlock bootloader */
	unlock_bootloader(mxt->client);
	msleep(200);  /*mdelay(100);*/

	/* reading the information of the firmware */
	pr_info("Firmware info: version [0x%02X], build [0x%02X]",
					fw->data[0], fw->data[1]);
	pr_info("Updating progress: ");
	pos += 2;

	while (pos < fw->size) {
		retry = 0;
		ret = check_bootloader(mxt->client, WAITING_FRAME_DATA);
		if (ret < 0) {
			pr_err("... Waiting frame data: Failed");
			goto err_fw;
		}

		frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));

		/* We should add 2 at frame size as the the firmware data is not
		 * included the CRC bytes.
		 */
		frame_size += 2;

		/* write one frame to device */
try_to_resend_the_last_frame:
		i2c_master_send(mxt->client,
			(u8 *)(fw->data + pos), frame_size);

		ret = check_bootloader(mxt->client, FRAME_CRC_PASS);
		if (ret < 0) {
			if (++retry < 10) {
				/* recommendation from ATMEL*/
				check_bootloader(mxt->client,
					WAITING_FRAME_DATA);
				pr_info("We've got a FRAME_CRC_FAIL, so try again up to 10 times (count=%d)",
					retry);
				goto try_to_resend_the_last_frame;
			}
			pr_err("... CRC on the frame failed after 10 trials!");
			goto err_fw;
		}

		pos += frame_size;

		pr_info("#");
		pr_info("%zd / %zd (bytes) updated...", pos, fw->size);
	}
	pr_info("\nUpdating firmware completed!\n");
	pr_info("note: You may need to reset this target.\n");

err_fw:
	/* change to slave address of application */
	if (mxt->client->addr == MXT_I2C_BOOTLOADER_ADDR) {
		pr_info("I2C address: 0x%02X --> 0x%02X",
			MXT_I2C_BOOTLOADER_ADDR, MXT_I2C_APP_ADDR);
		mxt->client->addr = MXT_I2C_APP_ADDR;
	}

#if READ_FW_FROM_HEADER
	kfree(fw);
#endif

	return ret;
}

