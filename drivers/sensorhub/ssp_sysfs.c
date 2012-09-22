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
/* SSP data delay function                                              */
/*************************************************************************/

unsigned int get_msdelay(int64_t dDelayRate)
{
	if (dDelayRate <= SENSOR_NS_DELAY_FASTEST)
		return SENSOR_MS_DELAY_FASTEST;
	else if (dDelayRate <= SENSOR_NS_DELAY_GAME)
		return SENSOR_MS_DELAY_GAME;
	else if (dDelayRate <= SENSOR_NS_DELAY_UI)
		return SENSOR_MS_DELAY_UI;
	else
		return SENSOR_MS_DELAY_NORMAL;
}

unsigned int get_delay_cmd(u8 uDelayRate)
{
	if (uDelayRate <= SENSOR_MS_DELAY_FASTEST)
		return SENSOR_CMD_DELAY_FASTEST;
	else if (uDelayRate <= SENSOR_MS_DELAY_GAME)
		return SENSOR_CMD_DELAY_GAME;
	else if (uDelayRate <= SENSOR_MS_DELAY_UI)
		return SENSOR_CMD_DELAY_UI;
	else
		return SENSOR_CMD_DELAY_NORMAL;
}

static void change_sensor_delay(struct ssp_data *data,
	int iSensorType, int64_t dNewDelay)
{
	u8 uBuf[2];
	int64_t dTempDelay = data->adDelayBuf[iSensorType];

	data->adDelayBuf[iSensorType] = dNewDelay;

	if (iSensorType == ORIENTATION_SENSOR)
		iSensorType = ACCELEROMETER_SENSOR;

	switch (data->aiCheckStatus[iSensorType]) {
	case ADD_SENSOR_STATE:
		ssp_dbg("[SSP]: %s - add %u, New = %lldns\n",
			 __func__, 1 << iSensorType, dNewDelay);

		uBuf[1] = (u8)get_msdelay(dNewDelay);
		uBuf[0] = (u8)get_delay_cmd(uBuf[1]);
		send_instruction(data, ADD_SENSOR, iSensorType, uBuf, 2);

		data->aiCheckStatus[iSensorType] = RUNNING_SENSOR_STATE;

		if (iSensorType == PROXIMITY_SENSOR) {
			input_report_abs(data->prox_input_dev, ABS_DISTANCE, 1);
			input_sync(data->prox_input_dev);
		}
		break;
	case RUNNING_SENSOR_STATE:
		if (dTempDelay == data->adDelayBuf[iSensorType])
			break;

		ssp_dbg("[SSP]: %s - Change %u, New = %lldns\n",
			__func__, 1 << iSensorType, dNewDelay);

		uBuf[1] = (u8)get_msdelay(dNewDelay);
		uBuf[0] = (u8)get_delay_cmd(uBuf[1]);
		send_instruction(data, CHANGE_DELAY, iSensorType, uBuf, 2);
		break;
	default:
		data->aiCheckStatus[iSensorType] = ADD_SENSOR_STATE;
	}
}

/*************************************************************************/
/* SSP data enable function                                              */
/*************************************************************************/

static int ssp_add_sensor(struct ssp_data *data, unsigned int uChangedSensor)
{
	if ((data->aiCheckStatus[uChangedSensor] != INITIALIZATION_STATE)
		&& (!atomic_read(&data->aSensorEnable))) {
		if (data->bCheckSuspend == false)
			data->bDebugEnabled = true;
	}

	return 0;
}

static int ssp_remove_sensor(struct ssp_data *data,
	unsigned int uChangedSensor, unsigned int uNewEnable)
{
	u8 uBuf[2];
	int64_t dSensorDelay = data->adDelayBuf[uChangedSensor];

	ssp_dbg("[SSP]: %s - remove sensor = %d, current state = %d\n",
		__func__, (1 << uChangedSensor), uNewEnable);

	data->adDelayBuf[uChangedSensor] = DEFUALT_POLLING_DELAY;

	if (data->aiCheckStatus[uChangedSensor] == INITIALIZATION_STATE) {
		data->aiCheckStatus[uChangedSensor] = NO_SENSOR_STATE;
		if (uChangedSensor == ACCELEROMETER_SENSOR)
			accel_open_calibration(data);
		else if (uChangedSensor == GYROSCOPE_SENSOR)
			gyro_open_calibration(data);
		else if (uChangedSensor == PRESSURE_SENSOR)
			pressure_open_calibration(data);
		else if (uChangedSensor == PROXIMITY_SENSOR) {
			proximity_open_lcd_ldi(data);
			proximity_open_calibration(data);
		}
		return 0;
	} else if (uChangedSensor == ORIENTATION_SENSOR) {
		if (!(atomic_read(&data->aSensorEnable)
			& (1 << ACCELEROMETER_SENSOR))) {
			uChangedSensor = ACCELEROMETER_SENSOR;
		} else {
			change_sensor_delay(data, ACCELEROMETER_SENSOR,
				data->adDelayBuf[ACCELEROMETER_SENSOR]);
			return 0;
		}
	} else if (uChangedSensor == ACCELEROMETER_SENSOR) {
		if (atomic_read(&data->aSensorEnable)
			& (1 << ORIENTATION_SENSOR)) {
			change_sensor_delay(data, ORIENTATION_SENSOR,
				data->adDelayBuf[ORIENTATION_SENSOR]);
			return 0;
		}
	}

	if (!uNewEnable) {
		if (data->bCheckSuspend == false)
			data->bDebugEnabled = false;
	}

	uBuf[1] = (u8)get_msdelay(dSensorDelay);
	uBuf[0] = (u8)get_delay_cmd(uBuf[1]);

	send_instruction(data, REMOVE_SENSOR, uChangedSensor, uBuf, 2);
	data->aiCheckStatus[uChangedSensor] = NO_SENSOR_STATE;
	return 0;
}

/*************************************************************************/
/* ssp Sysfs                                                             */
/*************************************************************************/

static ssize_t show_sensors_enable(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: %s - cur_enable = %d\n", __func__,
		 atomic_read(&data->aSensorEnable));

	return sprintf(buf, "%10u", atomic_read(&data->aSensorEnable));
}

static ssize_t set_sensors_enable(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int64_t dTemp;
	unsigned int uNewEnable = 0, uChangedSensor = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	if (strict_strtoll(buf, 10, &dTemp) < 0)
		return -1;

	uNewEnable = (unsigned int)dTemp;
	ssp_dbg("[SSP]: %s - new_enable = %u, old_enable = %u\n", __func__,
		 uNewEnable, atomic_read(&data->aSensorEnable));

	if (uNewEnable == atomic_read(&data->aSensorEnable))
		return 0;

	for (uChangedSensor = 0; uChangedSensor < SENSOR_MAX; uChangedSensor++)
		if ((atomic_read(&data->aSensorEnable) & (1 << uChangedSensor))
			!= (uNewEnable & (1 << uChangedSensor))) {

			if (uNewEnable & (1 << uChangedSensor))
				ssp_add_sensor(data, uChangedSensor);
			else
				ssp_remove_sensor(data, uChangedSensor,
					uNewEnable);
			break;
		}

	atomic_set(&data->aSensorEnable, uNewEnable);

	return size;
}

static ssize_t show_acc_delay(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n", data->adDelayBuf[ACCELEROMETER_SENSOR]);
}

static ssize_t set_acc_delay(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int64_t dNewDelay;
	struct ssp_data *data = dev_get_drvdata(dev);

	if (strict_strtoll(buf, 10, &dNewDelay) < 0)
		return -1;

	if ((atomic_read(&data->aSensorEnable) & (1 << ORIENTATION_SENSOR)) &&
		(data->adDelayBuf[ORIENTATION_SENSOR] < dNewDelay))
		data->adDelayBuf[ACCELEROMETER_SENSOR] = dNewDelay;
	else
		change_sensor_delay(data, ACCELEROMETER_SENSOR, dNewDelay);

	return size;
}

static ssize_t show_ori_delay(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n", data->adDelayBuf[ORIENTATION_SENSOR]);
}

static ssize_t set_ori_delay(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int64_t dNewDelay;
	struct ssp_data *data = dev_get_drvdata(dev);

	if (strict_strtoll(buf, 10, &dNewDelay) < 0)
		return -1;

	if (data->aiCheckStatus[ACCELEROMETER_SENSOR] == NO_SENSOR_STATE) {
		data->aiCheckStatus[ACCELEROMETER_SENSOR] = ADD_SENSOR_STATE;
		change_sensor_delay(data, ORIENTATION_SENSOR, dNewDelay);
	} else if (data->aiCheckStatus[ACCELEROMETER_SENSOR] ==
			RUNNING_SENSOR_STATE) {
		if (dNewDelay < data->adDelayBuf[ACCELEROMETER_SENSOR])
			change_sensor_delay(data,
				ORIENTATION_SENSOR, dNewDelay);
		else
			data->adDelayBuf[ORIENTATION_SENSOR] = dNewDelay;
	}
	return size;
}

static ssize_t show_gyro_delay(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n", data->adDelayBuf[GYROSCOPE_SENSOR]);
}

static ssize_t set_gyro_delay(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int64_t dNewDelay;
	struct ssp_data *data = dev_get_drvdata(dev);

	if (strict_strtoll(buf, 10, &dNewDelay) < 0)
		return -1;

	change_sensor_delay(data, GYROSCOPE_SENSOR, dNewDelay);
	return size;
}

static ssize_t show_mag_delay(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n", data->adDelayBuf[GEOMAGNETIC_SENSOR]);
}

static ssize_t set_mag_delay(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int64_t dNewDelay;
	struct ssp_data *data = dev_get_drvdata(dev);

	if (strict_strtoll(buf, 10, &dNewDelay) < 0)
		return -1;

	change_sensor_delay(data, GEOMAGNETIC_SENSOR, dNewDelay);

	return size;
}

static ssize_t show_pressure_delay(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data  = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n", data->adDelayBuf[PRESSURE_SENSOR]);
}

static ssize_t set_pressure_delay(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int64_t dNewDelay;
	struct ssp_data *data  = dev_get_drvdata(dev);

	if (strict_strtoll(buf, 10, &dNewDelay) < 0)
		return -1;

	change_sensor_delay(data, PRESSURE_SENSOR, dNewDelay);
	return size;
}

static ssize_t show_light_delay(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data  = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n", data->adDelayBuf[LIGHT_SENSOR]);
}

static ssize_t set_light_delay(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int64_t dNewDelay;
	struct ssp_data *data  = dev_get_drvdata(dev);

	if (strict_strtoll(buf, 10, &dNewDelay) < 0)
		return -1;

	change_sensor_delay(data, LIGHT_SENSOR, dNewDelay);
	return size;
}

static ssize_t show_prox_delay(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data  = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n", data->adDelayBuf[PROXIMITY_SENSOR]);
}

static ssize_t set_prox_delay(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int64_t dNewDelay;
	struct ssp_data *data  = dev_get_drvdata(dev);

	if (strict_strtoll(buf, 10, &dNewDelay) < 0)
		return -1;

	change_sensor_delay(data, PROXIMITY_SENSOR, dNewDelay);
	return size;
}

static DEVICE_ATTR(mcu_rev, S_IRUGO, mcu_revision_show, NULL);
static DEVICE_ATTR(mcu_name, S_IRUGO, mcu_model_name_show, NULL);
static DEVICE_ATTR(mcu_update, S_IRUGO, mcu_update_show, NULL);
static DEVICE_ATTR(mcu_update2, S_IRUGO, mcu_update2_show, NULL);
static DEVICE_ATTR(mcu_reset, S_IRUGO, mcu_reset_show, NULL);

static DEVICE_ATTR(mcu_test, S_IRUGO | S_IWUSR | S_IWGRP,
	mcu_factorytest_show, mcu_factorytest_store);
static DEVICE_ATTR(mcu_sleep_test, S_IRUGO | S_IWUSR | S_IWGRP,
	mcu_sleep_factorytest_show, mcu_sleep_factorytest_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	show_sensors_enable, set_sensors_enable);
static DEVICE_ATTR(acc_poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
	show_acc_delay, set_acc_delay);
static DEVICE_ATTR(gyro_poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
	show_gyro_delay, set_gyro_delay);
static DEVICE_ATTR(mag_poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
	show_mag_delay, set_mag_delay);
static DEVICE_ATTR(ori_poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
	show_ori_delay, set_ori_delay);
static DEVICE_ATTR(pressure_poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
	show_pressure_delay, set_pressure_delay);
static DEVICE_ATTR(light_poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
	show_light_delay, set_light_delay);
static DEVICE_ATTR(prox_poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
	show_prox_delay, set_prox_delay);

static struct device_attribute *mcu_attrs[] = {
	&dev_attr_enable,
	&dev_attr_mcu_rev,
	&dev_attr_mcu_name,
	&dev_attr_mcu_test,
	&dev_attr_mcu_reset,
	&dev_attr_mcu_update,
	&dev_attr_mcu_update2,
	&dev_attr_mcu_sleep_test,
	&dev_attr_mag_poll_delay,
	&dev_attr_ori_poll_delay,
	NULL,
};

static void initialize_mcu_factorytest(struct ssp_data *data)
{
	struct device *mcu_device = NULL;

	sensors_register(mcu_device, data, mcu_attrs, "ssp_sensor");
}

int initialize_sysfs(struct ssp_data *data)
{
	if (device_create_file(&data->acc_input_dev->dev,
		&dev_attr_acc_poll_delay))
		goto err_acc_input_dev;

	if (device_create_file(&data->gyro_input_dev->dev,
		&dev_attr_gyro_poll_delay))
		goto err_gyro_input_dev;

	if (device_create_file(&data->pressure_input_dev->dev,
		&dev_attr_pressure_poll_delay))
		goto err_pressure_input_dev;

	if (device_create_file(&data->light_input_dev->dev,
		&dev_attr_light_poll_delay))
		goto err_light_input_dev;

	if (device_create_file(&data->prox_input_dev->dev,
		&dev_attr_prox_poll_delay))
		goto err_prox_input_dev;

	initialize_accel_factorytest(data);
	initialize_gyro_factorytest(data);
	initialize_prox_factorytest(data);
	initialize_light_factorytest(data);
	initialize_pressure_factorytest(data);
	initialize_magnetic_factorytest(data);
	initialize_mcu_factorytest(data);

	return SUCCESS;

err_prox_input_dev:
	device_remove_file(&data->light_input_dev->dev,
		&dev_attr_light_poll_delay);
err_light_input_dev:
	device_remove_file(&data->pressure_input_dev->dev,
		&dev_attr_pressure_poll_delay);
err_pressure_input_dev:
	device_remove_file(&data->gyro_input_dev->dev,
		&dev_attr_gyro_poll_delay);
err_gyro_input_dev:
	device_remove_file(&data->acc_input_dev->dev,
		&dev_attr_acc_poll_delay);
err_acc_input_dev:
	return ERROR;
}

void remove_sysfs(struct ssp_data *data)
{
	device_remove_file(&data->acc_input_dev->dev,
		&dev_attr_acc_poll_delay);
	device_remove_file(&data->gyro_input_dev->dev,
		&dev_attr_gyro_poll_delay);
	device_remove_file(&data->pressure_input_dev->dev,
		&dev_attr_pressure_poll_delay);
	device_remove_file(&data->light_input_dev->dev,
		&dev_attr_light_poll_delay);
	device_remove_file(&data->prox_input_dev->dev,
		&dev_attr_prox_poll_delay);
}
