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

/* SSP -> AP Instruction */
#define MSG2AP_INST_BYPASS_DATA			0x00
#define MSG2AP_INST_LIBRARY_DATA		0x01
#define MSG2AP_INST_SELFTEST_DATA		0x02
#define MSG2AP_INST_DEBUG_DATA			0x03

/* Factory data length */
#define ACCEL_FACTORY_DATA_LENGTH		1
#define GYRO_FACTORY_DATA_LENGTH		27
#define MAGNETIC_FACTORY_DATA_LENGTH		6
#define PRESSURE_FACTORY_DATA_LENGTH		1
#define MCU_FACTORY_DATA_LENGTH			5
#define	GYRO_TEMP_FACTORY_DATA_LENGTH		1
#define	GYRO_DPS_FACTORY_DATA_LENGTH		1
#define MCU_SLEEP_FACTORY_DATA_LENGTH		39

/*************************************************************************/
/* SSP parsing the dataframe                                             */
/*************************************************************************/

static void get_3axis_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	int iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->x = iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->y = iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->z = iTemp;
}

static void get_light_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	int iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->r = iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->g = iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->b = iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->w = iTemp;
}

static void get_pressure_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	int iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 16;
	sensorsdata->pressure[0] = iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	sensorsdata->pressure[0] += iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->pressure[0] += iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += (int)pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->pressure[1] = (s16)iTemp;
}

static void get_gesture_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	int iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->data[0] = iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->data[1] = iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->data[2] = iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->data[3] = iTemp;
}

static void get_proximity_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	sensorsdata->prox[0] = (u8)pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->prox[1] = (u8)pchRcvDataFrame[(*iDataIdx)++];
}

static void get_proximity_rawdata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	sensorsdata->prox[0] = (u8)pchRcvDataFrame[(*iDataIdx)++];
}

static void get_factoty_data(struct ssp_data *data, int iSensorData,
	char *pchRcvDataFrame, int *iDataIdx)
{
	int iIdx, iTotalLenth = 0;
	unsigned int uTemp = 0;

	switch (iSensorData) {
	case ACCELEROMETER_FACTORY:
		uTemp = (1 << ACCELEROMETER_FACTORY);
		iTotalLenth = ACCEL_FACTORY_DATA_LENGTH;
		break;
	case GYROSCOPE_FACTORY:
		uTemp = (1 << GYROSCOPE_FACTORY);
		iTotalLenth = GYRO_FACTORY_DATA_LENGTH;
		break;
	case GEOMAGNETIC_FACTORY:
		uTemp = (1 << GEOMAGNETIC_FACTORY);
		iTotalLenth = MAGNETIC_FACTORY_DATA_LENGTH;
		break;
	case PRESSURE_FACTORY:
		uTemp = (1 << PRESSURE_FACTORY);
		iTotalLenth = PRESSURE_FACTORY_DATA_LENGTH;
		break;
	case MCU_FACTORY:
		uTemp = (1 << MCU_FACTORY);
		iTotalLenth = MCU_FACTORY_DATA_LENGTH;
		break;
	case GYROSCOPE_TEMP_FACTORY:
		uTemp = (1 << GYROSCOPE_TEMP_FACTORY);
		iTotalLenth = GYRO_TEMP_FACTORY_DATA_LENGTH;
		break;
	case GYROSCOPE_DPS_FACTORY:
		uTemp = (1 << GYROSCOPE_DPS_FACTORY);
		iTotalLenth = GYRO_DPS_FACTORY_DATA_LENGTH;
		break;
	case MCU_SLEEP_FACTORY:
		uTemp = (1 << MCU_SLEEP_FACTORY);
		iTotalLenth = MCU_SLEEP_FACTORY_DATA_LENGTH;
		break;
	}

	ssp_dbg("[SSP]: %s - Factory test data %d\n", __func__, iSensorData);
	for (iIdx = 0; iIdx < iTotalLenth; iIdx++)
		data->uFactorydata[iIdx] = (u8)pchRcvDataFrame[(*iDataIdx)++];

	data->uFactorydataReady = uTemp;
}

int parse_dataframe(struct ssp_data *data, char *pchRcvDataFrame, int iLength)
{
	int iDataIdx, iSensorData;
	struct sensor_value *sensorsdata;

	sensorsdata = kzalloc(sizeof(*sensorsdata), GFP_KERNEL);
	if (sensorsdata == NULL)
		return ERROR;

	for (iDataIdx = 0; iDataIdx < iLength;) {
		if (pchRcvDataFrame[iDataIdx] == MSG2AP_INST_BYPASS_DATA) {
			iDataIdx++;
			iSensorData = pchRcvDataFrame[iDataIdx++];
			if ((iSensorData < 0) ||
				(iSensorData >= (SENSOR_MAX - 1))) {
				pr_err("[SSP]: %s - Mcu data frame1 error %d\n",
					__func__, iSensorData);
				kfree(sensorsdata);
				return ERROR;
			}

			data->get_sensor_data[iSensorData](pchRcvDataFrame,
				&iDataIdx, sensorsdata);
			data->report_sensor_data[iSensorData](data,
				sensorsdata);
		} else if (pchRcvDataFrame[iDataIdx] ==
			MSG2AP_INST_SELFTEST_DATA) {
			iDataIdx++;
			iSensorData = pchRcvDataFrame[iDataIdx++];
			if ((iSensorData < 0) ||
				(iSensorData >= SENSOR_FACTORY_MAX)) {
				pr_err("[SSP]: %s - Mcu data frame2 error %d\n",
					__func__, iSensorData);
				kfree(sensorsdata);
				return ERROR;
			}
			get_factoty_data(data, iSensorData, pchRcvDataFrame,
				&iDataIdx);
		} else if (pchRcvDataFrame[iDataIdx] ==
			MSG2AP_INST_DEBUG_DATA) {
			iSensorData
				= print_mcu_debug(pchRcvDataFrame + iDataIdx+1,
						&iDataIdx, iLength);
			if (iSensorData) {
				pr_err("[SSP]: %s - Mcu data frame3 error %d\n",
					__func__, iSensorData);
				kfree(sensorsdata);
				return ERROR;
			}
#ifdef CONFIG_SENSORS_SSP_SENSORHUB
		} else if (pchRcvDataFrame[iDataIdx] ==
			MSG2AP_INST_LIBRARY_DATA) {
			ssp_sensorhub_handle_data(data,
					pchRcvDataFrame, iDataIdx, iLength);
			break;
#endif
		} else
			iDataIdx++;
	}
	kfree(sensorsdata);
	return SUCCESS;
}

void initialize_function_pointer(struct ssp_data *data)
{
	data->get_sensor_data[ACCELEROMETER_SENSOR] = get_3axis_sensordata;
	data->get_sensor_data[GYROSCOPE_SENSOR] = get_3axis_sensordata;
	data->get_sensor_data[GEOMAGNETIC_SENSOR] = get_3axis_sensordata;
	data->get_sensor_data[PRESSURE_SENSOR] = get_pressure_sensordata;
	data->get_sensor_data[GESTURE_SENSOR] = get_gesture_sensordata;
	data->get_sensor_data[PROXIMITY_SENSOR] = get_proximity_sensordata;
	data->get_sensor_data[PROXIMITY_RAW] = get_proximity_rawdata;
	data->get_sensor_data[LIGHT_SENSOR] = get_light_sensordata;

	data->report_sensor_data[ACCELEROMETER_SENSOR] = report_acc_data;
	data->report_sensor_data[GYROSCOPE_SENSOR] = report_gyro_data;
	data->report_sensor_data[GEOMAGNETIC_SENSOR] = report_mag_data;
	data->report_sensor_data[PRESSURE_SENSOR] = report_pressure_data;
	data->report_sensor_data[GESTURE_SENSOR] = report_gesture_data;
	data->report_sensor_data[PROXIMITY_SENSOR] = report_prox_data;
	data->report_sensor_data[PROXIMITY_RAW] = report_prox_raw_data;
	data->report_sensor_data[LIGHT_SENSOR] = report_light_data;
}
