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

#define SSP_DEBUG_TIMER_SEC		(10 * HZ)

#define LIMIT_RESET_CNT		20
#define LIMIT_SSD_FAIL_CNT		3
#define LIMIT_INSTRUCTION_FAIL_CNT	1
#define LIMIT_IRQ_FAIL_CNT		2
#define LIMIT_TIMEOUT_CNT		5

/*************************************************************************/
/* SSP Debug timer function                                              */
/*************************************************************************/

int print_mcu_debug(char *pchRcvDataFrame, int *pDataIdx,
			int iRcvDataFrameLength)
{
	int iLength = pchRcvDataFrame[0];

	if (iLength >= iRcvDataFrameLength - *pDataIdx - 1 || iLength <= 0) {
		ssp_dbg("[SSP]: MSG From MCU - invalid debug length(%d/%d)\n",
			iLength, iRcvDataFrameLength);
		return iLength ? iLength : ERROR;
	}

	pchRcvDataFrame[iLength] = 0;
	*pDataIdx += iLength + 2;
	ssp_dbg("[SSP]: MSG From MCU - %s\n", pchRcvDataFrame + 1);

	return 0;
}

void reset_mcu(struct ssp_data *data)
{
	ssp_enable(data, false);

	toggle_mcu_reset(data);
	msleep(SSP_SW_RESET_TIME);

	if (initialize_mcu(data) < 0)
		return;

	ssp_enable(data, true);
	sync_sensor_state(data);

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	ssp_sensorhub_report_notice(data, MSG2SSP_AP_STATUS_RESET);
#endif
}

void sync_sensor_state(struct ssp_data *data)
{
	unsigned char uBuf[2] = {0,};
	unsigned int uSensorCnt;
	int iRet = 0;
	proximity_open_calibration(data);
	iRet = set_hw_offset(data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - set_hw_offset failed\n", __func__);
	}

	udelay(10);

	for (uSensorCnt = 0; uSensorCnt < (SENSOR_MAX - 1); uSensorCnt++) {
		if (atomic_read(&data->aSensorEnable) & (1 << uSensorCnt)) {
			uBuf[1] = (u8)get_msdelay(data->adDelayBuf[uSensorCnt]);
			uBuf[0] = (u8)get_delay_cmd(uBuf[1]);
			send_instruction(data, ADD_SENSOR, uSensorCnt, uBuf, 2);
			udelay(10);
		}
	}

	if (data->bProximityRawEnabled == true) {
		uBuf[0] = 1;
		uBuf[1] = 20;
		send_instruction(data, ADD_SENSOR, PROXIMITY_RAW, uBuf, 2);
	}
}

static void print_sensordata(struct ssp_data *data, unsigned int uSensor)
{
	switch (uSensor) {
	case ACCELEROMETER_SENSOR:
	case GYROSCOPE_SENSOR:
	case GEOMAGNETIC_SENSOR:
		ssp_dbg(" %u : %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].x, data->buf[uSensor].y,
			data->buf[uSensor].z,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case PRESSURE_SENSOR:
		ssp_dbg(" %u : %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].pressure[0],
			data->buf[uSensor].pressure[1],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case GESTURE_SENSOR:
		ssp_dbg(" %u : %d %d %d %d (%ums)\n", uSensor,
			data->buf[uSensor].data[0], data->buf[uSensor].data[1],
			data->buf[uSensor].data[2], data->buf[uSensor].data[3],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case TEMPERATURE_HUMIDITY_SENSOR:
		ssp_dbg(" %u : %d %d %d(%ums)\n", uSensor,
			data->buf[uSensor].data[0], data->buf[uSensor].data[1],
			data->buf[uSensor].data[2], get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case LIGHT_SENSOR:
		ssp_dbg(" %u : %u, %u, %u, %u (%ums)\n", uSensor,
			data->buf[uSensor].r, data->buf[uSensor].g,
			data->buf[uSensor].b, data->buf[uSensor].w,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case PROXIMITY_SENSOR:
		ssp_dbg(" %u : %d %d(%ums)\n", uSensor,
			data->buf[uSensor].prox[0], data->buf[uSensor].prox[1],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	default:
		ssp_dbg("Wrong sensorCnt: %u\n", uSensor);
		break;
	}
}

static void debug_work_func(struct work_struct *work)
{
	unsigned int uSensorCnt;
	struct ssp_data *data = container_of(work, struct ssp_data, work_debug);

	ssp_dbg("[SSP]: %s(%u) - Sensor state: 0x%x, RC: %u, MS: %u\n",
		__func__, data->uIrqCnt, data->uSensorState, data->uResetCnt,
		data->uMissSensorCnt);

	if (data->fw_dl_state >= FW_DL_STATE_DOWNLOADING &&
		data->fw_dl_state < FW_DL_STATE_DONE) {
		pr_info("[SSP] : %s firmware downloading state = %d\n",
			__func__, data->fw_dl_state);
		return;
	} else if (data->fw_dl_state == FW_DL_STATE_FAIL) {
		pr_err("[SSP] : %s firmware download failed = %d\n",
			__func__, data->fw_dl_state);
		return;
	}

	for (uSensorCnt = 0; uSensorCnt < (SENSOR_MAX - 1); uSensorCnt++)
		if (atomic_read(&data->aSensorEnable) & (1 << uSensorCnt))
			print_sensordata(data, uSensorCnt);

	if ((atomic_read(&data->aSensorEnable) & SSP_BYPASS_SENSORS_EN_ALL)\
			&& (data->uIrqCnt == 0))
		data->uIrqFailCnt++;
	else
		data->uIrqFailCnt = 0;

	if (((data->uSsdFailCnt >= LIMIT_SSD_FAIL_CNT)
		|| (data->uInstFailCnt >= LIMIT_INSTRUCTION_FAIL_CNT)
		|| (data->uIrqFailCnt >= LIMIT_IRQ_FAIL_CNT)
		|| ((data->uTimeOutCnt + data->uBusyCnt) > LIMIT_TIMEOUT_CNT))
		&& (data->bSspShutdown == false)) {

		if (data->uResetCnt < LIMIT_RESET_CNT) {
			pr_info("[SSP] : %s - uSsdFailCnt(%u), uInstFailCnt(%u),"\
				"uIrqFailCnt(%u), uTimeOutCnt(%u), uBusyCnt(%u)\n",
				__func__, data->uSsdFailCnt, data->uInstFailCnt, data->uIrqFailCnt,
				data->uTimeOutCnt, data->uBusyCnt);
			reset_mcu(data);
			data->uResetCnt++;
		} else
			ssp_enable(data, false);

		data->uSsdFailCnt = 0;
		data->uInstFailCnt = 0;
		data->uTimeOutCnt = 0;
		data->uBusyCnt = 0;
		data->uIrqFailCnt = 0;
	}

	data->uIrqCnt = 0;
}

static void debug_timer_func(unsigned long ptr)
{
	struct ssp_data *data = (struct ssp_data *)ptr;

	queue_work(data->debug_wq, &data->work_debug);
	mod_timer(&data->debug_timer,
		round_jiffies_up(jiffies + SSP_DEBUG_TIMER_SEC));
}

void enable_debug_timer(struct ssp_data *data)
{
	mod_timer(&data->debug_timer,
		round_jiffies_up(jiffies + SSP_DEBUG_TIMER_SEC));
}

void disable_debug_timer(struct ssp_data *data)
{
	del_timer_sync(&data->debug_timer);
	cancel_work_sync(&data->work_debug);
}

int initialize_debug_timer(struct ssp_data *data)
{
	setup_timer(&data->debug_timer, debug_timer_func, (unsigned long)data);

	data->debug_wq = create_singlethread_workqueue("ssp_debug_wq");
	if (!data->debug_wq)
		return ERROR;

	INIT_WORK(&data->work_debug, debug_work_func);
	return SUCCESS;
}
