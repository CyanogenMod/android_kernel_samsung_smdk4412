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

int waiting_wakeup_mcu(struct ssp_data *data)
{
	int iDelaycnt = 0;

	while (!data->check_mcu_busy() && (iDelaycnt++ < 100))
		mdelay(5);

	if (iDelaycnt >= 100) {
		pr_err("[SSP]: %s - MCU Irq Timeout!!\n", __func__);
		data->uBusyCnt++;
	}

	iDelaycnt = 0;
	data->wakeup_mcu();
	while (data->check_mcu_ready() && (iDelaycnt++ < 40))
		udelay(50);

	if (iDelaycnt >= 40) {
		pr_err("[SSP]: %s - MCU Wakeup Timeout!!\n", __func__);
		data->uTimeOutCnt++;
		return ERROR;
	}

	return SUCCESS;
}

int ssp_i2c_read(struct ssp_data *data, char *pTxData, u16 uTxLength,
	char *pRxData, u16 uRxLength)
{
	int iRetries = 3, iRet = 0;
	struct i2c_client *client = data->client;
	struct i2c_msg msgs[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = uTxLength,
		 .buf = pTxData,
		},
		{
		 .addr = client->addr,
		 .flags = I2C_M_RD,
		 .len = uRxLength,
		 .buf = pRxData,
		},
	};

	while (iRetries--) {
		iRet = i2c_transfer(client->adapter, msgs, 2);
		if (iRet < 0) {
			pr_err("[SSP]: %s - i2c transfer error %d! retry...\n",
				__func__, iRet);
			mdelay(10);
		} else {
			return SUCCESS;
		}
	}
	return ERROR;
}

int ssp_sleep_mode(struct ssp_data *data)
{
	char chRxData = 0;
	char chTxData = MSG2SSP_AP_STATUS_SLEEP;
	int iRet = 0;

	waiting_wakeup_mcu(data);

	/* send to AP_STATUS_SLEEP */
	iRet = ssp_i2c_read(data, &chTxData, 1, &chRxData, 1);
	if ((chRxData != MSG_ACK) || (iRet != SUCCESS)) {
		pr_err("[SSP]: %s - MSG2SSP_AP_STATUS_SLEEP CMD fail %d\n",
				__func__, iRet);
		return ERROR;
	}
	ssp_dbg("[SSP]: %s - MSG2SSP_AP_STATUS_SLEEP CMD\n", __func__);

	return SUCCESS;
}

int ssp_resume_mode(struct ssp_data *data)
{
	char chRxData = 0;
	char chTxData = MSG2SSP_AP_STATUS_WAKEUP;
	int iRet = 0;

	waiting_wakeup_mcu(data);

	/* send to AP_STATUS_WAKEUP */
	iRet = ssp_i2c_read(data, &chTxData, 1, &chRxData, 1);
	if ((chRxData != MSG_ACK) || (iRet != SUCCESS)) {
		pr_err("[SSP]: %s - MSG2SSP_AP_STATUS_WAKEUP CMD fail %d\n",
		       __func__, iRet);
		return ERROR;
	}
	ssp_dbg("[SSP]: %s - MSG2SSP_AP_STATUS_WAKEUP CMD\n", __func__);

	return SUCCESS;
}

int send_instruction(struct ssp_data *data, u8 uInst,
	u8 uSensorType, u8 *uSendBuf, u8 uLength)
{
	char aTxData[uLength + 5];
	char chRxbuf = 0;
	int iRet = 0, iRetries = 3;

	waiting_wakeup_mcu(data);

	aTxData[0] = MSG2SSP_STS;
	aTxData[1] = 3 + uLength;
	aTxData[2] = MSG2SSP_SSM;

	switch (uInst) {
	case REMOVE_SENSOR:
		aTxData[3] = MSG2SSP_INST_BYPASS_SENSOR_REMOVE;
		break;
	case ADD_SENSOR:
		aTxData[3] = MSG2SSP_INST_BYPASS_SENSOR_ADD;
		break;
	case CHANGE_DELAY:
		aTxData[3] = MSG2SSP_INST_CHANGE_DELAY;
		break;
	case GO_SLEEP:
		aTxData[3] = MSG2SSP_AP_STATUS_SLEEP;
		break;
	case FACTORY_MODE:
		aTxData[3] = MSG2SSP_INST_SENSOR_SELFTEST;
		break;
	case REMOVE_LIBRARY:
		aTxData[3] = MSG2SSP_INST_LIBRARY_REMOVE;
		break;
	case ADD_LIBRARY:
		aTxData[3] = MSG2SSP_INST_LIBRARY_ADD;
		break;
	default:
		aTxData[3] = uInst;
		break;
	}

	aTxData[4] = uSensorType;
	memcpy(&aTxData[5], uSendBuf, uLength);

#if 0
	iRet = ssp_i2c_read(data, aTxData, 2, &chRxbuf, 1);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - MSG2SSP_STS - i2c fail %d\n",
		       __func__, iRet);
		return ERROR;
	} else if (chRxbuf != MSG_ACK) {
		while (iRetries--) {
			mdelay(10);
			pr_err("[SSP]: %s - MSG2SSP_STS retry...\n", __func__);
			iRet = ssp_i2c_read(data, aTxData, 2, &chRxbuf, 1);
			if ((iRet == SUCCESS) && (chRxbuf == MSG_ACK))
				break;
		}

		if (iRetries <= 0)
			return FAIL;
	}
#endif

	chRxbuf = 0;
	iRetries = 3;
	iRet = ssp_i2c_read(data, &(aTxData[2]), (u16)aTxData[1], &chRxbuf, 1);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Instruction CMD Fail %d\n",
			__func__, iRet);
		return ERROR;
	} else if (chRxbuf != MSG_ACK) {
		while (iRetries--) {
			mdelay(10);
			pr_err("[SSP]: %s - Instruction CMD retry...\n",
				__func__);
			iRet = ssp_i2c_read(data, &(aTxData[2]),
				(u16)aTxData[1], &chRxbuf, 1);
			if ((iRet == SUCCESS) && (chRxbuf == MSG_ACK))
				break;
		}

		if (iRetries <= 0) {
			data->uI2cFailCnt++;
			return FAIL;
		}
	}

	data->uI2cFailCnt = 0;
	ssp_dbg("[SSP]: %s - Inst = 0x%x, Sensor Type = 0x%x, "
		"data = %u, %u\n", __func__, aTxData[3], aTxData[4],
		aTxData[5], aTxData[6]);
	return SUCCESS;
}

static int ssp_recieve_msg(struct ssp_data *data,  u8 uLength)
{
	char chTxBuf = 0;
	char *pchRcvDataFrame;	/* SSP-AP Massage data buffer */
	int iRet = 0;

	if (uLength > 0) {
		pchRcvDataFrame = kzalloc((uLength * sizeof(char)), GFP_KERNEL);
		chTxBuf = MSG2SSP_SRM;
		iRet = ssp_i2c_read(data, &chTxBuf, 1, pchRcvDataFrame,
				(u16)uLength);
		if (iRet != SUCCESS) {
			pr_err("[SSP]: %s - Fail to recieve data %d\n",
				__func__, iRet);
			kfree(pchRcvDataFrame);
			return ERROR;
		}
	} else {
		pr_err("[SSP]: %s - No ready data. length = %d\n",
			__func__, uLength);
		return FAIL;
	}

	parse_dataframe(data, pchRcvDataFrame, uLength);

	kfree(pchRcvDataFrame);
	return uLength;
}

int get_chipid(struct ssp_data *data)
{
	int iRet;

	waiting_wakeup_mcu(data);

	/* read chip id */
	iRet = i2c_smbus_read_byte_data(data->client, MSG2SSP_AP_WHOAMI);

	return iRet;
}

int set_sensor_position(struct ssp_data *data)
{
	char chTxBuf[5] = { 0, };
	char chRxData = 0;
	int iRet = 0;

	waiting_wakeup_mcu(data);

	chTxBuf[0] = MSG2SSP_AP_SENSOR_FORMATION;

	chTxBuf[1] = CONFIG_SENSORS_SSP_ACCELEROMETER_POSITION;
	chTxBuf[2] = CONFIG_SENSORS_SSP_GYROSCOPE_POSITION;
	chTxBuf[3] = CONFIG_SENSORS_SSP_MAGNETOMETER_POSITION;
	chTxBuf[4] = 0;

#if defined(CONFIG_MACH_T0_EUR_OPEN)
	if (data->check_ap_rev() == 0x03) {
		chTxBuf[1] = 7;
		chTxBuf[2] = 7;
		chTxBuf[3] = CONFIG_SENSORS_SSP_MAGNETOMETER_POSITION;
	}
#endif

#if defined(CONFIG_MACH_T0_USA_SPR) || defined(CONFIG_MACH_T0_USA_USCC)\
	|| defined(CONFIG_MACH_T0_USA_VZW)
	if (data->check_ap_rev() <= 0x04) {
		chTxBuf[1] = 4;
		chTxBuf[2] = 4;
		chTxBuf[3] = CONFIG_SENSORS_SSP_MAGNETOMETER_POSITION;
	}
#endif

	pr_info("[SSP] Sensor Posision A : %u, G : %u, M: %u, P: %u\n",
		chTxBuf[1], chTxBuf[2], chTxBuf[3], chTxBuf[4]);

	iRet = ssp_i2c_read(data, chTxBuf, 5, &chRxData, 1);
	if ((chRxData != MSG_ACK) || (iRet != SUCCESS)) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	return iRet;
}

void set_proximity_threshold(struct ssp_data *data)
{
	char chTxBuf[3] = { 0, };
	char chRxData = 0;
	int iRet = 0;

	waiting_wakeup_mcu(data);

	chTxBuf[0] = MSG2SSP_AP_SENSOR_PROXTHRESHOLD;
	chTxBuf[1] = data->uProxThresh;
	chTxBuf[2] = data->uProxCanc;

	pr_info("[SSP] Proximity Threshold : %u, Cancelation : %u\n",
		data->uProxThresh, data->uProxCanc);

	iRet = ssp_i2c_read(data, chTxBuf, 3, &chRxData, 1);
	if ((chRxData != MSG_ACK) || (iRet != SUCCESS))
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
}

void set_proximity_barcode_enable(struct ssp_data *data, bool bEnable)
{
	char chTxBuf[2] = { 0, };
	char chRxData = 0;
	int iRet = 0;

	waiting_wakeup_mcu(data);

	chTxBuf[0] = MSG2SSP_AP_SENSOR_BARCODE_EMUL;
	chTxBuf[1] = bEnable;

	data->bBarcodeEnabled = bEnable;
	pr_info("[SSP] Proximity Barcode En : %u\n", bEnable);

	iRet = ssp_i2c_read(data, chTxBuf, 2, &chRxData, 1);
	if ((chRxData != MSG_ACK) || (iRet != SUCCESS))
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
}

unsigned int get_sensor_scanning_info(struct ssp_data *data)
{
	char chTxBuf = MSG2SSP_AP_SENSOR_SCANNING;
	char chRxData[2] = {0,};
	int iRet = 0;

	waiting_wakeup_mcu(data);

	iRet = ssp_i2c_read(data, &chTxBuf, 1, chRxData, 2);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c failed %d\n", __func__, iRet);
		return 0;
	}
	return ((unsigned int)chRxData[0] << 8) | chRxData[1];
}

unsigned int get_firmware_rev(struct ssp_data *data)
{
	char chTxData = MSG2SSP_AP_FIRMWARE_REV;
	char chRxBuf[3] = { 0, };
	unsigned int uRev = 99999;
	int iRet;

	waiting_wakeup_mcu(data);

	iRet = ssp_i2c_read(data, &chTxData, 1, chRxBuf, 3);
	if (iRet != SUCCESS)
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
	else
		uRev = ((unsigned int)chRxBuf[0] << 16)
			| ((unsigned int)chRxBuf[1] << 8) | chRxBuf[2];
	return uRev;
}

int get_fuserom_data(struct ssp_data *data)
{
	char chTxBuf[2] = { 0, };
	char chRxBuf[2] = { 0, };
	int iRet = 0;
	unsigned int uLength = 0;

	waiting_wakeup_mcu(data);

	chTxBuf[0] = MSG2SSP_AP_STT;
	chTxBuf[1] = MSG2SSP_AP_FUSEROM;

	/* chRxBuf is Two Byte because msg is large */
	iRet = ssp_i2c_read(data, chTxBuf, 2, chRxBuf, 2);
	uLength = ((unsigned int)chRxBuf[0] << 8) + (unsigned int)chRxBuf[1];

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - MSG2SSP_AP_STT - i2c fail %d\n",
				__func__, iRet);
		goto err_read_fuserom;
	} else if (uLength <= 0) {
		pr_err("[SSP]: %s - No ready data. length = %u\n",
				__func__, uLength);
		goto err_read_fuserom;
	} else {
		if (data->iLibraryLength != 0)
			kfree(data->pchLibraryBuf);

		data->iLibraryLength = (int)uLength;
		data->pchLibraryBuf =
		    kzalloc((data->iLibraryLength * sizeof(char)), GFP_KERNEL);

		chTxBuf[0] = MSG2SSP_SRM;
		iRet = ssp_i2c_read(data, chTxBuf, 1, data->pchLibraryBuf,
			(u16)data->iLibraryLength);
		if (iRet != SUCCESS) {
			pr_err("[SSP]: %s - Fail to recieve SSP data %d\n",
				__func__, iRet);
			kfree(data->pchLibraryBuf);
			data->iLibraryLength = 0;
			goto err_read_fuserom;
		}
	}

	data->uFuseRomData[0] = data->pchLibraryBuf[0];
	data->uFuseRomData[1] = data->pchLibraryBuf[1];
	data->uFuseRomData[2] = data->pchLibraryBuf[2];

	pr_info("[SSP] FUSE ROM Data %d , %d, %d\n", data->uFuseRomData[0],
		data->uFuseRomData[1], data->uFuseRomData[2]);

	data->iLibraryLength = 0;
	kfree(data->pchLibraryBuf);
	return SUCCESS;

err_read_fuserom:
	data->uFuseRomData[0] = 0;
	data->uFuseRomData[1] = 0;
	data->uFuseRomData[2] = 0;

	return FAIL;
}

int select_irq_msg(struct ssp_data *data)
{
	u8 chLength = 0;
	char chTxBuf = 0;
	char chRxBuf[2] = { 0, };
	int iRet = 0;

	chTxBuf = MSG2SSP_SSD;
	iRet = ssp_i2c_read(data, &chTxBuf, 1, chRxBuf, 2);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - MSG2SSP_SSD error %d\n", __func__, iRet);
		return ERROR;
	} else {
		if (chRxBuf[0] == MSG2SSP_RTS) {
			chLength = (u8)chRxBuf[1];
			ssp_recieve_msg(data, chLength);
			data->uSsdFailCnt = 0;
		}
#ifdef CONFIG_SENSORS_SSP_SENSORHUB
		else if (chRxBuf[0] == MSG2SSP_STT) {
			pr_info("%s: MSG2SSP_STT irq", __func__);
			iRet = ssp_handle_sensorhub_large_data(data,
					(u8)chRxBuf[1]);
			if (iRet < 0) {
				pr_err("%s: ssp_handle_sensorhub_data(%d)",
					__func__, iRet);
			}
			data->uSsdFailCnt = 0;
		}
#endif
		else {
			pr_err("[SSP]: %s - MSG2SSP_SSD Data fail "
				"[0]: 0x%x, [1]: 0x%x\n", __func__,
				chRxBuf[0], chRxBuf[1]);
			if ((chRxBuf[0] == 0) && (chRxBuf[1] == 0))
				data->uSsdFailCnt++;
		}
	}
	return SUCCESS;
}
