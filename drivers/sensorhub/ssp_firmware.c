/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
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
 */
#include "ssp.h"

#define SSP_FIRMWARE_REVISION		92800

/* Bootload mode cmd */
#define BL_FW_NAME			"ssp.fw"
#define BL_CRASHED_FW_NAME		"ssp_crashed.fw"

#define APP_SLAVE_ADDR			0x18
#define BOOTLOADER_SLAVE_ADDR		0x26

/* Bootloader mode status */
#define BL_WAITING_BOOTLOAD_CMD		0xc0	/* valid 7 6 bit only */
#define BL_WAITING_FRAME_DATA		0x80	/* valid 7 6 bit only */
#define BL_FRAME_CRC_CHECK		0x02
#define BL_FRAME_CRC_FAIL		0x03
#define BL_FRAME_CRC_PASS		0x04
#define BL_APP_CRC_FAIL			0x40	/* valid 7 8 bit only */
#define BL_BOOT_STATUS_MASK		0x3f

/* Command to unlock bootloader */
#define BL_UNLOCK_CMD_MSB		0xaa
#define BL_UNLOCK_CMD_LSB		0xdc

unsigned int get_module_rev(void)
{
	return SSP_FIRMWARE_REVISION;
}

static int check_bootloader(struct i2c_client *client, unsigned int uState)
{
	u8 uVal;
	u8 uTemp;

recheck:
	if (i2c_master_recv(client, &uVal, 1) != 1)
		return -EIO;

	if (uVal & 0x20) {
		if (i2c_master_recv(client, &uTemp, 1) != 1) {
			pr_err("[SSP]: %s - i2c recv fail\n", __func__);
			return -EIO;
		}

		if (i2c_master_recv(client, &uTemp, 1) != 1) {
			pr_err("[SSP]: %s - i2c recv fail\n", __func__);
			return -EIO;
		}

		uVal &= ~0x20;
	}

	if ((uVal & 0xF0) == BL_APP_CRC_FAIL) {
		pr_info("[SSP] SSP_APP_CRC_FAIL - There is no bootloader.\n");
		if (i2c_master_recv(client, &uVal, 1) != 1) {
			pr_err("[SSP]: %s - i2c recv fail\n", __func__);
			return -EIO;
		}

		if (uVal & 0x20) {
			if (i2c_master_recv(client, &uTemp, 1) != 1) {
				pr_err("[SSP]: %s - i2c recv fail\n", __func__);
				return -EIO;
			}

			if (i2c_master_recv(client, &uTemp, 1) != 1) {
				pr_err("[SSP]: %s - i2c recv fail\n", __func__);
				return -EIO;
			}

			uVal &= ~0x20;
		}
	}

	switch (uState) {
	case BL_WAITING_BOOTLOAD_CMD:
	case BL_WAITING_FRAME_DATA:
		uVal &= ~BL_BOOT_STATUS_MASK;
		break;
	case BL_FRAME_CRC_PASS:
		if (uVal == BL_FRAME_CRC_CHECK)
			goto recheck;
		break;
	default:
		return -EINVAL;
	}

	if (uVal != uState) {
		pr_err("[SSP]: %s - Unvalid bootloader mode state\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int unlock_bootloader(struct i2c_client *client)
{
	u8 uBuf[2];

	uBuf[0] = BL_UNLOCK_CMD_LSB;
	uBuf[1] = BL_UNLOCK_CMD_MSB;

	if (i2c_master_send(client, uBuf, 2) != 2) {
		pr_err("[SSP]: %s - i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int fw_write(struct i2c_client *client,
	const u8 *pData, unsigned int uFrameSize)
{
	if (i2c_master_send(client, pData, uFrameSize) != uFrameSize) {
		pr_err("[SSP]: %s - i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int load_fw_bootmode(struct i2c_client *client, const char *pFn)
{
	const struct firmware *fw = NULL;
	unsigned int uFrameSize;
	unsigned int uPos = 0;
	int iRet;
	int iCheckFrameCrcError = 0;
	int iCheckWatingFrameDataError = 0;

	pr_info("[SSP] ssp_load_fw start!!!\n");

	iRet = request_firmware(&fw, pFn, &client->dev);
	if (iRet) {
		pr_err("[SSP]: %s - Unable to open firmware %s\n",
			__func__, pFn);
		return iRet;
	}

	/* Unlock bootloader */
	unlock_bootloader(client);

	while (uPos < fw->size) {
		iRet = check_bootloader(client, BL_WAITING_FRAME_DATA);
		if (iRet) {
			iCheckWatingFrameDataError++;
			if (iCheckWatingFrameDataError > 10) {
				pr_err("[SSP]: %s - F/W update fail\n",
					__func__);
				goto out;
			} else {
				pr_err("[SSP]: %s - F/W data_error %d, retry\n",
					__func__, iCheckWatingFrameDataError);
				continue;
			}
		}

		uFrameSize = ((*(fw->data + uPos) << 8) |
			*(fw->data + uPos + 1));

		/* We should add 2 at frame size as the the firmware data is not
		*  included the CRC bytes.
		*/
		uFrameSize += 2;

		/* Write one frame to device */
		fw_write(client, fw->data + uPos, uFrameSize);
		iRet = check_bootloader(client, BL_FRAME_CRC_PASS);
		if (iRet) {
			iCheckFrameCrcError++;
			if (iCheckFrameCrcError > 10) {
				pr_err("[SSP]: %s - F/W Update Fail. crc err\n",
					__func__);
				goto out;
			} else {
				pr_err("[SSP]: %s - F/W crc_error %d, retry\n",
					__func__, iCheckFrameCrcError);
				continue;
			}
		}

		uPos += uFrameSize;

		pr_info("[SSP] Updated %d bytes / %zd bytes\n", uPos, fw->size);
		mdelay(1);
	}

out:
	release_firmware(fw);
	return iRet;
}

static void change_to_bootmode(struct ssp_data *data)
{
	int iCnt = 0;

	for (iCnt = 0; iCnt < 10; iCnt++) {
		data->set_mcu_reset(0);
		udelay(10);

		data->set_mcu_reset(1);
		msleep(100);
	}

	msleep(50);
}

void toggle_mcu_reset(struct ssp_data *data)
{
	u8 uBuf[2];

	uBuf[0] = 0x00;
	uBuf[1] = 0x00;

	data->set_mcu_reset(0);
	udelay(10);
	data->set_mcu_reset(1);
	mdelay(50);

	data->client->addr = BOOTLOADER_SLAVE_ADDR;

	if (i2c_master_send(data->client, uBuf, 2) != 2)
		pr_err("[SSP]: %s - ssp_Normal Mode\n", __func__);
	else
		pr_err("[SSP]: %s - ssp_load_fw_bootmode\n", __func__);

	data->client->addr = APP_SLAVE_ADDR;
}

int update_mcu_bin(struct ssp_data *data)
{
	int iRet = 0;

	pr_info("[SSP] ssp_change_to_bootmode\n");
	change_to_bootmode(data);
	data->client->addr = BOOTLOADER_SLAVE_ADDR;
	iRet = load_fw_bootmode(data->client, BL_FW_NAME);

	msleep(SSP_SW_RESET_TIME);

	data->client->addr = APP_SLAVE_ADDR;

	if (iRet < 0)
		data->bSspShutdown = true;
	else
		data->bSspShutdown = false;

	return iRet;
}

int update_crashed_mcu_bin(struct ssp_data *data)
{
	int iRet = 0;
	pr_info("[SSP] ssp_change_to_bootmode\n");
	change_to_bootmode(data);
	data->client->addr = BOOTLOADER_SLAVE_ADDR;
	iRet = load_fw_bootmode(data->client, BL_CRASHED_FW_NAME);

	msleep(SSP_SW_RESET_TIME);

	data->client->addr = APP_SLAVE_ADDR;
	data->bSspShutdown = true;
	return iRet;
}

void check_fwbl(struct ssp_data *data)
{
	int iRet;

	data->client->addr = BOOTLOADER_SLAVE_ADDR;
	iRet = check_bootloader(data->client, BL_WAITING_BOOTLOAD_CMD);

	if (iRet >= 0) {
		pr_info("[SSP] ssp_load_fw_bootmode\n");
		load_fw_bootmode(data->client, BL_FW_NAME);
		msleep(SSP_SW_RESET_TIME);
	} else {
		data->client->addr = APP_SLAVE_ADDR;
		data->uCurFirmRev = get_firmware_rev(data);
		if (data->uCurFirmRev != SSP_FIRMWARE_REVISION) {
			pr_info("[SSP] MPU Firm Rev. : Old = %8u, New = %8u\n",
				data->uCurFirmRev, SSP_FIRMWARE_REVISION);
			update_mcu_bin(data);
		}
	}

	data->client->addr = APP_SLAVE_ADDR;
	data->uCurFirmRev = get_firmware_rev(data);
	pr_info("[SSP] MPU Firm Rev. : Old = %8u, New = %8u\n",
		data->uCurFirmRev, SSP_FIRMWARE_REVISION);
}
