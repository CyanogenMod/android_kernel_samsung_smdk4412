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

/*************************************************************************/
/* SSP Debug timer function                                              */
/*************************************************************************/

void reset_mcu(struct ssp_data *data)
{
	disable_irq(data->iIrq);
	disable_irq_wake(data->iIrq);

	toggle_mcu_reset(data);
	msleep(SSP_SW_RESET_TIME);
	initialize_mcu(data);

	sync_sensor_state(data);

	enable_irq(data->iIrq);
	enable_irq_wake(data->iIrq);

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	ssp_report_sensorhub_notice(data, MSG2SSP_AP_STATUS_RESET);
#endif
}

void sync_sensor_state(struct ssp_data *data)
{
	unsigned char uBuf[2] = {0,};
	unsigned int uSensorCnt;

	set_proximity_threshold(data);

	udelay(10);

	for (uSensorCnt = 0; uSensorCnt < (SENSOR_MAX - 1); uSensorCnt++) {
		if (atomic_read(&data->aSensorEnable) & (1 << uSensorCnt)) {
			uBuf[1] = (u8)get_msdelay(data->adDelayBuf[uSensorCnt]);
			uBuf[0] = (u8)get_delay_cmd(uBuf[1]);
			send_instruction(data, ADD_SENSOR, uSensorCnt, uBuf, 2);
			udelay(10);
		}
	}

	data->uTimeOutCnt = 0;
	data->uBusyCnt = 0;
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
	case LIGHT_SENSOR:
		ssp_dbg(" %u : %u, %u, %u, %u (%ums)\n", uSensor,
			data->buf[uSensor].r, data->buf[uSensor].g,
			data->buf[uSensor].b, data->buf[uSensor].w,
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
	case PROXIMITY_SENSOR:
		ssp_dbg(" %u : %d %d(%ums)\n", uSensor,
			data->buf[uSensor].prox[0], data->buf[uSensor].prox[1],
			get_msdelay(data->adDelayBuf[uSensor]));
	}
}

void print_mcu_debug(char *pchRcvDataFrame, int *pDataIdx)
{
	int iLength;

	iLength = pchRcvDataFrame[0];
	pchRcvDataFrame[iLength] = 0;
	*pDataIdx = *pDataIdx + iLength + 2;

	ssp_dbg("[SSP] MSG From MCU : %s\n", pchRcvDataFrame + 1);
}

static void debug_work_func(struct work_struct *work)
{
	unsigned int uSensorCnt;
	struct ssp_data *data = container_of(work, struct ssp_data, work_debug);

	ssp_dbg("[SSP]: %s - Sensor state: 0x%x, TO: %u, BC: %u, RC: %u\n",
		__func__, data->uAliveSensorDebug, data->uTimeOutCnt,
		data->uBusyCnt, data->uResetCnt);
	for (uSensorCnt = 0; uSensorCnt < (SENSOR_MAX - 1); uSensorCnt++)
		if (atomic_read(&data->aSensorEnable) & (1 << uSensorCnt))
			print_sensordata(data, uSensorCnt);

	if ((data->uSsdFailCnt >= 3) || (data->uI2cFailCnt >= 1)) {
		if (data->uResetCnt < 20) {
			reset_mcu(data);
			data->uResetCnt++;
		}

		data->uSsdFailCnt = 0;
		data->uI2cFailCnt = 0;
	}
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
