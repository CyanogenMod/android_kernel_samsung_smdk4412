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

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define MODEL_NAME			"AT32UC3L0128"

ssize_t mcu_revision_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "AT01120%u,AT01120%u\n", get_module_rev(),
		data->uCurFirmRev);
}

ssize_t mcu_model_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", MODEL_NAME);
}

ssize_t mcu_update_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bSuccess = false;
	int iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: %s - mcu binany update!\n", __func__);

	if (data->bSspShutdown == false) {
		disable_irq(data->iIrq);
		disable_irq_wake(data->iIrq);
		data->bSspShutdown = true;
	}

	iRet = update_mcu_bin(data);
	if (iRet < 0) {
		ssp_dbg("[SSP]: %s - update_mcu_bin failed!\n", __func__);
		goto exit;
	}

	iRet = initialize_mcu(data);
	if (iRet < 0) {
		ssp_dbg("[SSP]: %s - initialize_mcu failed!\n", __func__);
		goto exit;
	}

	sync_sensor_state(data);

	enable_irq(data->iIrq);
	enable_irq_wake(data->iIrq);

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	ssp_report_sensorhub_notice(data, MSG2SSP_AP_STATUS_RESET);
#endif

	bSuccess = true;
exit:
	return sprintf(buf, "%s\n", (bSuccess ? "OK" : "NG"));
}

ssize_t mcu_update2_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bSuccess = false;
	int iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: %s - mcu binany update!\n", __func__);

	if (data->bSspShutdown == false) {
		disable_irq(data->iIrq);
		disable_irq_wake(data->iIrq);
		data->bSspShutdown = true;
	}

	iRet = update_crashed_mcu_bin(data);
	if (iRet < 0) {
		ssp_dbg("[SSP]: %s - update_mcu_bin failed!\n", __func__);
		goto exit;
	}

	iRet = initialize_mcu(data);
	if (iRet < 0) {
		ssp_dbg("[SSP]: %s - initialize_mcu failed!\n", __func__);
		goto exit;
	}

	sync_sensor_state(data);

	enable_irq(data->iIrq);
	enable_irq_wake(data->iIrq);

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	ssp_report_sensorhub_notice(data, MSG2SSP_AP_STATUS_RESET);
#endif

	bSuccess = true;
exit:
	return sprintf(buf, "%s\n", (bSuccess ? "OK" : "NG"));
}

ssize_t mcu_reset_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	reset_mcu(data);

	return sprintf(buf, "OK\n");
}

ssize_t mcu_factorytest_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char chTempBuf[2] = {0, 10};
	int iRet = 0;

	if (sysfs_streq(buf, "1")) {
		data->uFactorydataReady = 0;
		memset(data->uFactorydata, 0, sizeof(char) * FACTORY_DATA_MAX);

		data->bMcuIRQTestSuccessed = false;
		data->uTimeOutCnt = 0;

		iRet = send_instruction(data, FACTORY_MODE,
				MCU_FACTORY, chTempBuf, 2);
		if (data->uTimeOutCnt == 0)
			data->bMcuIRQTestSuccessed = true;
	} else {
		pr_err("[SSP]: %s - invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	ssp_dbg("[SSP]: MCU Factory Test Start! - %d\n", iRet);

	return size;
}

ssize_t mcu_factorytest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bMcuTestSuccessed = false;
	struct ssp_data *data = dev_get_drvdata(dev);

	if (data->bSspShutdown == true) {
		ssp_dbg("[SSP]: %s - MCU Bin is crashed\n", __func__);
		return sprintf(buf, "NG,NG,NG\n");
	}

	if (data->uFactorydataReady & (1 << MCU_FACTORY)) {
		ssp_dbg("[SSP] MCU Factory Test Data : %u, %u, %u, %u, %u\n",
			data->uFactorydata[0], data->uFactorydata[1],
			data->uFactorydata[2], data->uFactorydata[3],
			data->uFactorydata[4]);

		/* system clock, RTC, I2C Master, I2C Slave, externel pin */
		if ((data->uFactorydata[0] == SUCCESS)
			&& (data->uFactorydata[1] == SUCCESS)
			&& (data->uFactorydata[2] == SUCCESS)
			&& (data->uFactorydata[3] == SUCCESS)
			&& (data->uFactorydata[4] == SUCCESS))
			bMcuTestSuccessed = true;
	} else {
		pr_err("[SSP]: %s - The Sensorhub is not ready %u\n", __func__,
			data->uFactorydataReady);
	}

	ssp_dbg("[SSP]: MCU Factory Test Result - %s, %s, %s\n", MODEL_NAME,
		(data->bMcuIRQTestSuccessed ? "OK" : "NG"),
		(bMcuTestSuccessed ? "OK" : "NG"));

	return sprintf(buf, "%s,%s,%s\n", MODEL_NAME,
		(data->bMcuIRQTestSuccessed ? "OK" : "NG"),
		(bMcuTestSuccessed ? "OK" : "NG"));
}

ssize_t mcu_sleep_factorytest_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char chTempBuf[2] = {0, 10};
	int iRet = 0;

	if (sysfs_streq(buf, "1")) {
		data->uFactorydataReady = 0;
		memset(data->uFactorydata, 0, sizeof(char) * FACTORY_DATA_MAX);

		iRet = send_instruction(data, FACTORY_MODE,
				MCU_SLEEP_FACTORY, chTempBuf, 2);
	} else {
		pr_err("[SSP]: %s - invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	ssp_dbg("[SSP]: MCU Sleep Factory Test Start! - %d\n", iRet);

	return size;
}

ssize_t mcu_sleep_factorytest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int iDataIdx, iSensorData = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	struct sensor_value fsb[SENSOR_MAX];

	if (!(data->uFactorydataReady & (1 << MCU_SLEEP_FACTORY))) {
		pr_err("[SSP]: %s - The Sensorhub is not ready\n", __func__);
		goto exit;
	}

	for (iDataIdx = 0; iDataIdx < FACTORY_DATA_MAX;) {
		iSensorData = (int)data->uFactorydata[iDataIdx++];

		if ((iSensorData < 0) ||
			(iSensorData >= (SENSOR_MAX - 1))) {
			pr_err("[SSP]: %s - Mcu data frame error %d\n",
				__func__, iSensorData);
			goto exit;
		}

		data->get_sensor_data[iSensorData]((char *)data->uFactorydata,
			&iDataIdx, &(fsb[iSensorData]));
	}

	convert_acc_data(&fsb[ACCELEROMETER_SENSOR].x);
	convert_acc_data(&fsb[ACCELEROMETER_SENSOR].y);
	convert_acc_data(&fsb[ACCELEROMETER_SENSOR].z);

exit:
	ssp_dbg("[SSP]: %s Result - "
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%u\n", __func__,
		fsb[ACCELEROMETER_SENSOR].x, fsb[ACCELEROMETER_SENSOR].y,
		fsb[ACCELEROMETER_SENSOR].z, fsb[GYROSCOPE_SENSOR].x,
		fsb[GYROSCOPE_SENSOR].y, fsb[GYROSCOPE_SENSOR].z,
		fsb[GEOMAGNETIC_SENSOR].x, fsb[GEOMAGNETIC_SENSOR].y,
		fsb[GEOMAGNETIC_SENSOR].z, fsb[PRESSURE_SENSOR].pressure[0],
		fsb[PRESSURE_SENSOR].pressure[1], fsb[PROXIMITY_SENSOR].prox[1],
		fsb[LIGHT_SENSOR].r, fsb[LIGHT_SENSOR].g, fsb[LIGHT_SENSOR].b,
		fsb[LIGHT_SENSOR].w);

	return sprintf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%u\n",
		fsb[ACCELEROMETER_SENSOR].x, fsb[ACCELEROMETER_SENSOR].y,
		fsb[ACCELEROMETER_SENSOR].z, fsb[GYROSCOPE_SENSOR].x,
		fsb[GYROSCOPE_SENSOR].y, fsb[GYROSCOPE_SENSOR].z,
		fsb[GEOMAGNETIC_SENSOR].x, fsb[GEOMAGNETIC_SENSOR].y,
		fsb[GEOMAGNETIC_SENSOR].z, fsb[PRESSURE_SENSOR].pressure[0],
		fsb[PRESSURE_SENSOR].pressure[1], fsb[PROXIMITY_SENSOR].prox[1],
		fsb[LIGHT_SENSOR].r, fsb[LIGHT_SENSOR].g, fsb[LIGHT_SENSOR].b,
		fsb[LIGHT_SENSOR].w);
}
