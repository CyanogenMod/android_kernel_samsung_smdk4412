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

#define LIMIT_DELAY_CNT		200

int waiting_wakeup_mcu(struct ssp_data *data)
{
	int iDelaycnt = 0;

	while (!data->check_mcu_busy() && (iDelaycnt++ < LIMIT_DELAY_CNT)
		&& (data->bSspShutdown == false))
		mdelay(5);

	if (iDelaycnt >= LIMIT_DELAY_CNT) {
		pr_err("[SSP]: %s - MCU Irq Timeout!!\n", __func__);
		data->uBusyCnt++;
	} else {
		data->uBusyCnt = 0;
	}

	iDelaycnt = 0;
	data->wakeup_mcu();
	while (data->check_mcu_ready() && (iDelaycnt++ < LIMIT_DELAY_CNT)
		&& (data->bSspShutdown == false))
		mdelay(5);

	if (iDelaycnt >= LIMIT_DELAY_CNT) {
		pr_err("[SSP]: %s - MCU Wakeup Timeout!!\n", __func__);
		data->uTimeOutCnt++;
	} else {
		data->uTimeOutCnt = 0;
	}

	if (data->bSspShutdown == true)
		return FAIL;

	return SUCCESS;
}

int ssp_i2c_read(struct ssp_data *data, char *pTxData, u16 uTxLength,
	char *pRxData, u16 uRxLength, int iRetries)
{
	int iRet = 0, iDiffTime = 0, iTimeTemp;
	struct timeval cur_time;
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

	do_gettimeofday(&cur_time);
	iTimeTemp = (int)cur_time.tv_sec;
	do {
		iRet = i2c_transfer(client->adapter, msgs, 2);
		if (iRet < 0) {
			do_gettimeofday(&cur_time);
			iDiffTime = (int)cur_time.tv_sec - iTimeTemp;
			iTimeTemp = (int)cur_time.tv_sec;
			if (iDiffTime >= 4) {
				pr_err("[SSP]: %s - i2c time out %d!\n",
					__func__, iDiffTime);
				break;
			}
			pr_err("[SSP]: %s - i2c transfer error %d! retry...\n",
				__func__, iRet);
			mdelay(10);
		} else {
			return SUCCESS;
		}
	} while (--iRetries);

	return ERROR;
}

int ssp_sleep_mode(struct ssp_data *data)
{
	char chRxBuf = 0;
	char chTxBuf = MSG2SSP_AP_STATUS_SLEEP;
	int iRet = 0, iRetries = DEFAULT_RETRIES;

	if (waiting_wakeup_mcu(data) < 0)
		return ERROR;

	/* send to AP_STATUS_SLEEP */
	iRet = ssp_i2c_read(data, &chTxBuf, 1, &chRxBuf, 1, DEFAULT_RETRIES);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - MSG2SSP_AP_STATUS_SLEEP CMD fail %d\n",
			__func__, iRet);
		return ERROR;
	} else if (chRxBuf != MSG_ACK) {
		while (iRetries--) {
			mdelay(10);
			pr_err("[SSP]: %s - MSG2SSP_AP_STATUS_SLEEP CMD "
				"retry...\n", __func__);
			iRet = ssp_i2c_read(data, &chTxBuf, 1,
				&chRxBuf, 1, DEFAULT_RETRIES);
			if ((iRet == SUCCESS) && (chRxBuf == MSG_ACK))
				break;
		}

		if (iRetries < 0) {
			data->uInstFailCnt++;
			return FAIL;
		}
	}

	data->uInstFailCnt = 0;
	ssp_dbg("[SSP]: %s - MSG2SSP_AP_STATUS_SLEEP CMD\n", __func__);

	return SUCCESS;
}

int ssp_resume_mode(struct ssp_data *data)
{
	char chRxBuf = 0;
	char chTxBuf = MSG2SSP_AP_STATUS_WAKEUP;
	int iRet = 0, iRetries = DEFAULT_RETRIES;

	if (waiting_wakeup_mcu(data) < 0)
		return ERROR;

	/* send to MSG2SSP_AP_STATUS_WAKEUP */
	iRet = ssp_i2c_read(data, &chTxBuf, 1, &chRxBuf, 1, DEFAULT_RETRIES);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - MSG2SSP_AP_STATUS_WAKEUP CMD fail %d\n",
				__func__, iRet);
		return ERROR;
	} else if (chRxBuf != MSG_ACK) {
		while (iRetries--) {
			mdelay(10);
			pr_err("[SSP]: %s - MSG2SSP_AP_STATUS_WAKEUP CMD "
				"retry...\n", __func__);
			iRet = ssp_i2c_read(data, &chTxBuf, 1, &chRxBuf, 1,
				DEFAULT_RETRIES);
			if ((iRet == SUCCESS) && (chRxBuf == MSG_ACK))
				break;
		}

		if (iRetries < 0) {
			data->uInstFailCnt++;
			return FAIL;
		}
	}

	data->uInstFailCnt = 0;
	ssp_dbg("[SSP]: %s - MSG2SSP_AP_STATUS_WAKEUP CMD\n", __func__);

	return SUCCESS;
}

int send_instruction(struct ssp_data *data, u8 uInst,
	u8 uSensorType, u8 *uSendBuf, u8 uLength)
{
	char chTxbuf[uLength + 3];
	char chRxbuf = 0;
	int iRet = 0, iRetries = DEFAULT_RETRIES;

	if ((!(data->uSensorState & (1 << uSensorType)))
		&& (uInst <= CHANGE_DELAY)) {
		pr_err("[SSP]: %s - Bypass Inst Skip! - %u\n",
			__func__, uSensorType);
		return FAIL;
	} else if ((!((data->uSensorState | 0xf0) & (1 << uSensorType)))
		&& (uInst == FACTORY_MODE)) {
		pr_err("[SSP]: %s - Factory Inst Skip! - %u\n",
			__func__, uSensorType);
		return FAIL;
	}

	if (waiting_wakeup_mcu(data) < 0)
		return ERROR;

	chTxbuf[0] = MSG2SSP_SSM;

	switch (uInst) {
	case REMOVE_SENSOR:
		chTxbuf[1] = MSG2SSP_INST_BYPASS_SENSOR_REMOVE;
		break;
	case ADD_SENSOR:
		chTxbuf[1] = MSG2SSP_INST_BYPASS_SENSOR_ADD;
		break;
	case CHANGE_DELAY:
		chTxbuf[1] = MSG2SSP_INST_CHANGE_DELAY;
		break;
	case GO_SLEEP:
		chTxbuf[1] = MSG2SSP_AP_STATUS_SLEEP;
		break;
	case FACTORY_MODE:
		chTxbuf[1] = MSG2SSP_INST_SENSOR_SELFTEST;
		break;
	case REMOVE_LIBRARY:
		chTxbuf[1] = MSG2SSP_INST_LIBRARY_REMOVE;
		break;
	case ADD_LIBRARY:
		chTxbuf[1] = MSG2SSP_INST_LIBRARY_ADD;
		break;
	default:
		chTxbuf[1] = uInst;
		break;
	}

	chTxbuf[2] = uSensorType;
	memcpy(&chTxbuf[3], uSendBuf, uLength);

	iRet = ssp_i2c_read(data, &(chTxbuf[0]), uLength + 3, &chRxbuf, 1,
		DEFAULT_RETRIES);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Instruction CMD Fail %d\n", __func__, iRet);
		return ERROR;
	} else if (chRxbuf != MSG_ACK) {
		while (iRetries--) {
			mdelay(10);
			pr_err("[SSP]: %s - Instruction CMD retry...\n",
				__func__);
			if (waiting_wakeup_mcu(data) < 0)
				return ERROR;
			iRet = ssp_i2c_read(data, &(chTxbuf[0]),
				uLength + 3, &chRxbuf, 1, DEFAULT_RETRIES);
			if ((iRet == SUCCESS) && (chRxbuf == MSG_ACK))
				break;
		}

		if (iRetries < 0) {
			data->uInstFailCnt++;
			return FAIL;
		}
	}

	data->uInstFailCnt = 0;
	ssp_dbg("[SSP]: %s - Inst = 0x%x, Sensor Type = 0x%x, "
		"data = %u, %u\n", __func__, chTxbuf[1], chTxbuf[2],
		chTxbuf[3], chTxbuf[4]);
	return SUCCESS;
}

int get_chipid(struct ssp_data *data)
{
	int iRet;

	if (waiting_wakeup_mcu(data) < 0)
		return ERROR;

	/* read chip id */
	iRet = i2c_smbus_read_byte_data(data->client, MSG2SSP_AP_WHOAMI);

	return iRet;
}

int set_sensor_position(struct ssp_data *data)
{
	char chTxBuf[5] = { 0, };
	char chRxData = 0;
	int iRet = 0;

	if (waiting_wakeup_mcu(data) < 0)
		return ERROR;

	chTxBuf[0] = MSG2SSP_AP_SENSOR_FORMATION;

	chTxBuf[1] = CONFIG_SENSORS_SSP_ACCELEROMETER_POSITION;
	chTxBuf[2] = CONFIG_SENSORS_SSP_GYROSCOPE_POSITION;
	chTxBuf[3] = CONFIG_SENSORS_SSP_MAGNETOMETER_POSITION;
	chTxBuf[4] = 0;

	pr_info("[SSP] Sensor Posision A : %u, G : %u, M: %u, P: %u\n",
		chTxBuf[1], chTxBuf[2], chTxBuf[3], chTxBuf[4]);

	iRet = ssp_i2c_read(data, chTxBuf, 5, &chRxData, 1, DEFAULT_RETRIES);
	if ((chRxData != MSG_ACK) || (iRet != SUCCESS)) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	return iRet;
}

void set_proximity_threshold(struct ssp_data *data,
	unsigned char uData1, unsigned char uData2)
{
	char chTxBuf[3] = { 0, };
	char chRxBuf = 0;
	int iRet = 0, iRetries = DEFAULT_RETRIES;

	if (waiting_wakeup_mcu(data) < 0)
		return;

	chTxBuf[0] = MSG2SSP_AP_SENSOR_PROXTHRESHOLD;
	chTxBuf[1] = uData1;
	chTxBuf[2] = uData2;

	iRet = ssp_i2c_read(data, chTxBuf, 3, &chRxBuf, 1, DEFAULT_RETRIES);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - SENSOR_PROXTHRESHOLD CMD fail %d\n",
			__func__, iRet);
		return;
	} else if (chRxBuf != MSG_ACK) {
		while (iRetries--) {
			mdelay(10);
			pr_err("[SSP]: %s - MSG2SSP_AP_SENSOR_PROXTHRESHOLD CMD"
				" retry...\n", __func__);
			iRet = ssp_i2c_read(data, chTxBuf, 3, &chRxBuf, 1,
				DEFAULT_RETRIES);
			if ((iRet == SUCCESS) && (chRxBuf == MSG_ACK))
				break;
		}

		if (iRetries < 0) {
			data->uInstFailCnt++;
			return;
		}
	}

	data->uInstFailCnt = 0;
	pr_info("[SSP]: Proximity Threshold - %u, %u\n", uData1, uData2);
}

void set_proximity_barcode_enable(struct ssp_data *data, bool bEnable)
{
	char chTxBuf[2] = { 0, };
	char chRxBuf = 0;
	int iRet = 0, iRetries = DEFAULT_RETRIES;

	if (waiting_wakeup_mcu(data) < 0)
		return ;

	chTxBuf[0] = MSG2SSP_AP_SENSOR_BARCODE_EMUL;
	chTxBuf[1] = bEnable;

	data->bBarcodeEnabled = bEnable;

	iRet = ssp_i2c_read(data, chTxBuf, 2, &chRxBuf, 1, DEFAULT_RETRIES);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - SENSOR_BARCODE_EMUL CMD fail %d\n",
				__func__, iRet);
		return;
	} else if (chRxBuf != MSG_ACK) {
		while (iRetries--) {
			mdelay(10);
			pr_err("[SSP]: %s - MSG2SSP_AP_SENSOR_BARCODE_EMUL CMD "
				"retry...\n", __func__);
			iRet = ssp_i2c_read(data, chTxBuf, 2, &chRxBuf, 1,
				DEFAULT_RETRIES);
			if ((iRet == SUCCESS) && (chRxBuf == MSG_ACK))
				break;
		}

		if (iRetries < 0) {
			data->uInstFailCnt++;
			return;
		}
	}

	data->uInstFailCnt = 0;
	pr_info("[SSP] Proximity Barcode En : %u\n", bEnable);
}

unsigned int get_sensor_scanning_info(struct ssp_data *data)
{
	char chTxBuf = MSG2SSP_AP_SENSOR_SCANNING;
	char chRxData[2] = {0,};
	int iRet = 0;

	if (waiting_wakeup_mcu(data) < 0)
		return ERROR;

	iRet = ssp_i2c_read(data, &chTxBuf, 1, chRxData, 2, DEFAULT_RETRIES);
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

	if (waiting_wakeup_mcu(data) < 0)
		return ERROR;

	iRet = ssp_i2c_read(data, &chTxData, 1, chRxBuf, 3, DEFAULT_RETRIES);
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

	if (waiting_wakeup_mcu(data) < 0)
		return ERROR;

	chTxBuf[0] = MSG2SSP_AP_STT;
	chTxBuf[1] = MSG2SSP_AP_FUSEROM;

	/* chRxBuf is Two Byte because msg is large */
	iRet = ssp_i2c_read(data, chTxBuf, 2, chRxBuf, 2, DEFAULT_RETRIES);
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
			(u16)data->iLibraryLength, DEFAULT_RETRIES);
		if (iRet != SUCCESS) {
			pr_err("[SSP]: %s - Fail to receive SSP data %d\n",
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

static int ssp_receive_msg(struct ssp_data *data,  u8 uLength)
{
	char chTxBuf = 0;
	char *pchRcvDataFrame;	/* SSP-AP Massage data buffer */
	int iRet = 0;

	if (uLength > 0) {
		pchRcvDataFrame = kzalloc((uLength * sizeof(char)), GFP_KERNEL);
		chTxBuf = MSG2SSP_SRM;
		iRet = ssp_i2c_read(data, &chTxBuf, 1, pchRcvDataFrame,
				(u16)uLength, 0);
		if (iRet != SUCCESS) {
			pr_err("[SSP]: %s - Fail to receive data %d\n",
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

int select_irq_msg(struct ssp_data *data)
{
	u8 chLength = 0;
	char chTxBuf = 0;
	char chRxBuf[2] = { 0, };
	int iRet = 0;

	chTxBuf = MSG2SSP_SSD;
	iRet = ssp_i2c_read(data, &chTxBuf, 1, chRxBuf, 2, 0);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - MSG2SSP_SSD error %d\n", __func__, iRet);
		return ERROR;
	} else {
		if (chRxBuf[0] == MSG2SSP_RTS) {
			chLength = (u8)chRxBuf[1];
			ssp_receive_msg(data, chLength);
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
