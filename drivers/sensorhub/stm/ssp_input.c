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
/* SSP Kernel -> HAL input evnet function                                */
/*************************************************************************/
void convert_acc_data(s16 *iValue)
{
	if (*iValue > MAX_ACCEL_2G)
		*iValue = ((MAX_ACCEL_4G - *iValue)) * (-1);
}

void report_acc_data(struct ssp_data *data, struct sensor_value *accdata)
{
	convert_acc_data(&accdata->x);
	convert_acc_data(&accdata->y);
	convert_acc_data(&accdata->z);

	data->buf[ACCELEROMETER_SENSOR].x = accdata->x - data->accelcal.x;
	data->buf[ACCELEROMETER_SENSOR].y = accdata->y - data->accelcal.y;
	data->buf[ACCELEROMETER_SENSOR].z = accdata->z - data->accelcal.z;

	if (!(data->buf[ACCELEROMETER_SENSOR].x >> 15 == accdata->x >> 15) &&\
		!(data->accelcal.x >> 15 == accdata->x >> 15)) {
		pr_debug("[SSP] : accel x is overflowed!\n");
		data->buf[ACCELEROMETER_SENSOR].x =
			(data->buf[ACCELEROMETER_SENSOR].x > 0 ? MIN_ACCEL_2G : MAX_ACCEL_2G);
	}
	if (!(data->buf[ACCELEROMETER_SENSOR].y >> 15 == accdata->y >> 15) &&\
		!(data->accelcal.y >> 15 == accdata->y >> 15)) {
		pr_debug("[SSP] : accel y is overflowed!\n");
		data->buf[ACCELEROMETER_SENSOR].y =
			(data->buf[ACCELEROMETER_SENSOR].y > 0 ? MIN_ACCEL_2G : MAX_ACCEL_2G);
	}
	if (!(data->buf[ACCELEROMETER_SENSOR].z >> 15 == accdata->z >> 15) &&\
		!(data->accelcal.z >> 15 == accdata->z >> 15)) {
		pr_debug("[SSP] : accel z is overflowed!\n");
		data->buf[ACCELEROMETER_SENSOR].z =
			(data->buf[ACCELEROMETER_SENSOR].z > 0 ? MIN_ACCEL_2G : MAX_ACCEL_2G);
	}

	input_report_rel(data->acc_input_dev, REL_X,
		data->buf[ACCELEROMETER_SENSOR].x);
	input_report_rel(data->acc_input_dev, REL_Y,
		data->buf[ACCELEROMETER_SENSOR].y);
	input_report_rel(data->acc_input_dev, REL_Z,
		data->buf[ACCELEROMETER_SENSOR].z);
	input_sync(data->acc_input_dev);
}

void report_gyro_data(struct ssp_data *data, struct sensor_value *gyrodata)
{
	long lTemp[3] = {0,};

	data->buf[GYROSCOPE_SENSOR].x = gyrodata->x - data->gyrocal.x;
	data->buf[GYROSCOPE_SENSOR].y = gyrodata->y - data->gyrocal.y;
	data->buf[GYROSCOPE_SENSOR].z = gyrodata->z - data->gyrocal.z;

	if (!(data->buf[GYROSCOPE_SENSOR].x >> 15 == gyrodata->x >> 15) &&\
		!(data->gyrocal.x >> 15 == gyrodata->x >> 15)) {
		pr_debug("[SSP] : gyro x is overflowed!\n");
		data->buf[GYROSCOPE_SENSOR].x =
			(data->buf[GYROSCOPE_SENSOR].x >= 0 ? MIN_GYRO : MAX_GYRO);
	}
	if (!(data->buf[GYROSCOPE_SENSOR].y >> 15 == gyrodata->y >> 15) &&\
		!(data->gyrocal.y >> 15 == gyrodata->y >> 15)) {
		pr_debug("[SSP] : gyro y is overflowed!\n");
		data->buf[GYROSCOPE_SENSOR].y =
			(data->buf[GYROSCOPE_SENSOR].y >= 0 ? MIN_GYRO : MAX_GYRO);
	}
	if (!(data->buf[GYROSCOPE_SENSOR].z >> 15 == gyrodata->z >> 15) &&\
		!(data->gyrocal.z >> 15 == gyrodata->z >> 15)) {
		pr_debug("[SSP] : gyro z is overflowed!\n");
		data->buf[GYROSCOPE_SENSOR].z =
			(data->buf[GYROSCOPE_SENSOR].z >= 0 ? MIN_GYRO : MAX_GYRO);
	}

	if (data->uGyroDps == GYROSCOPE_DPS500) {
		lTemp[0] = (long)data->buf[GYROSCOPE_SENSOR].x;
		lTemp[1] = (long)data->buf[GYROSCOPE_SENSOR].y;
		lTemp[2] = (long)data->buf[GYROSCOPE_SENSOR].z;
	} else if (data->uGyroDps == GYROSCOPE_DPS250)	{
		lTemp[0] = (long)data->buf[GYROSCOPE_SENSOR].x >> 1;
		lTemp[1] = (long)data->buf[GYROSCOPE_SENSOR].y >> 1;
		lTemp[2] = (long)data->buf[GYROSCOPE_SENSOR].z >> 1;
	} else if (data->uGyroDps == GYROSCOPE_DPS2000)	{
		lTemp[0] = (long)data->buf[GYROSCOPE_SENSOR].x << 2;
		lTemp[1] = (long)data->buf[GYROSCOPE_SENSOR].y << 2;
		lTemp[2] = (long)data->buf[GYROSCOPE_SENSOR].z << 2;
	} else {
		lTemp[0] = (long)data->buf[GYROSCOPE_SENSOR].x;
		lTemp[1] = (long)data->buf[GYROSCOPE_SENSOR].y;
		lTemp[2] = (long)data->buf[GYROSCOPE_SENSOR].z;
	}

	input_report_rel(data->gyro_input_dev, REL_RX, lTemp[0]);
	input_report_rel(data->gyro_input_dev, REL_RY, lTemp[1]);
	input_report_rel(data->gyro_input_dev, REL_RZ, lTemp[2]);
	input_sync(data->gyro_input_dev);
}

void report_mag_data(struct ssp_data *data, struct sensor_value *magdata)
{
	data->buf[GEOMAGNETIC_SENSOR].x = magdata->x;
	data->buf[GEOMAGNETIC_SENSOR].y = magdata->y;
	data->buf[GEOMAGNETIC_SENSOR].z = magdata->z;

	input_report_rel(data->mag_input_dev, REL_RX,
		data->buf[GEOMAGNETIC_SENSOR].x);
	input_report_rel(data->mag_input_dev, REL_RY,
		data->buf[GEOMAGNETIC_SENSOR].y);
	input_report_rel(data->mag_input_dev, REL_RZ,
		data->buf[GEOMAGNETIC_SENSOR].z);
	input_sync(data->mag_input_dev);
}

void report_gesture_data(struct ssp_data *data, struct sensor_value *gesdata)
{
	data->buf[GESTURE_SENSOR].data[0] = gesdata->data[0];
	data->buf[GESTURE_SENSOR].data[1] = gesdata->data[1];
	data->buf[GESTURE_SENSOR].data[2] = gesdata->data[2];
	data->buf[GESTURE_SENSOR].data[3] = gesdata->data[3];
	data->buf[GESTURE_SENSOR].data[4] = data->uTempCount;

	data->buf[GESTURE_SENSOR].data[5] = gesdata->data[5]; /* a_delta */
	data->buf[GESTURE_SENSOR].data[6] = gesdata->data[6]; /* b_delta */
	data->buf[GESTURE_SENSOR].data[7] = gesdata->data[7]; /* c_delta */
	data->buf[GESTURE_SENSOR].data[8] = gesdata->data[8]; /* d_delta */

	input_report_abs(data->gesture_input_dev,
		ABS_RUDDER, data->buf[GESTURE_SENSOR].data[0]);
	input_report_abs(data->gesture_input_dev,
		ABS_WHEEL, data->buf[GESTURE_SENSOR].data[1]);
	input_report_abs(data->gesture_input_dev,
		ABS_GAS, data->buf[GESTURE_SENSOR].data[2]);
	input_report_abs(data->gesture_input_dev,
		ABS_BRAKE, data->buf[GESTURE_SENSOR].data[3]);
	input_report_abs(data->gesture_input_dev,
		ABS_THROTTLE, data->buf[GESTURE_SENSOR].data[4]);
	input_report_abs(data->gesture_input_dev,
		ABS_X, data->buf[GESTURE_SENSOR].data[5]);
	input_report_abs(data->gesture_input_dev,
		ABS_Y, data->buf[GESTURE_SENSOR].data[6]);
	input_report_abs(data->gesture_input_dev,
		ABS_Z, data->buf[GESTURE_SENSOR].data[7]);
	input_report_abs(data->gesture_input_dev,
		ABS_RX, data->buf[GESTURE_SENSOR].data[8]);
	input_sync(data->gesture_input_dev);

	data->uTempCount++;

}

void report_pressure_data(struct ssp_data *data, struct sensor_value *predata)
{
	data->buf[PRESSURE_SENSOR].pressure[0] =
		predata->pressure[0] - data->iPressureCal;
	data->buf[PRESSURE_SENSOR].pressure[1] = predata->pressure[1];

	/* pressure */
	input_report_rel(data->pressure_input_dev, REL_HWHEEL,
		data->buf[PRESSURE_SENSOR].pressure[0]);
	/* temperature */
	input_report_rel(data->pressure_input_dev, REL_WHEEL,
		data->buf[PRESSURE_SENSOR].pressure[1]);
	input_sync(data->pressure_input_dev);
}

void report_light_data(struct ssp_data *data, struct sensor_value *lightdata)
{
	data->buf[LIGHT_SENSOR].r = lightdata->r;
	data->buf[LIGHT_SENSOR].g = lightdata->g;
	data->buf[LIGHT_SENSOR].b = lightdata->b;
	data->buf[LIGHT_SENSOR].w = lightdata->w;

	input_report_rel(data->light_input_dev, REL_HWHEEL,
		data->buf[LIGHT_SENSOR].r + 1);
	input_report_rel(data->light_input_dev, REL_DIAL,
		data->buf[LIGHT_SENSOR].g + 1);
	input_report_rel(data->light_input_dev, REL_WHEEL,
		data->buf[LIGHT_SENSOR].b + 1);
	input_report_rel(data->light_input_dev, REL_MISC,
		data->buf[LIGHT_SENSOR].w + 1);
	input_sync(data->light_input_dev);
}

void report_prox_data(struct ssp_data *data, struct sensor_value *proxdata)
{
	ssp_dbg("[SSP] Proximity Sensor Detect : %u, raw : %u\n",
		proxdata->prox[0], proxdata->prox[1]);

	data->buf[PROXIMITY_SENSOR].prox[0] = proxdata->prox[0];
	data->buf[PROXIMITY_SENSOR].prox[1] = proxdata->prox[1];

	input_report_abs(data->prox_input_dev, ABS_DISTANCE,
		(!proxdata->prox[0]));
	input_sync(data->prox_input_dev);

	wake_lock_timeout(&data->ssp_wake_lock, 3 * HZ);
}

void report_prox_raw_data(struct ssp_data *data,
	struct sensor_value *proxrawdata)
{
	if (data->uFactoryProxAvg[0]++ >= PROX_AVG_READ_NUM) {
		data->uFactoryProxAvg[2] /= PROX_AVG_READ_NUM;
		data->buf[PROXIMITY_RAW].prox[1] = (u8)data->uFactoryProxAvg[1];
		data->buf[PROXIMITY_RAW].prox[2] = (u8)data->uFactoryProxAvg[2];
		data->buf[PROXIMITY_RAW].prox[3] = (u8)data->uFactoryProxAvg[3];

		data->uFactoryProxAvg[0] = 0;
		data->uFactoryProxAvg[1] = 0;
		data->uFactoryProxAvg[2] = 0;
		data->uFactoryProxAvg[3] = 0;
	} else {
		data->uFactoryProxAvg[2] += proxrawdata->prox[0];

		if (data->uFactoryProxAvg[0] == 1)
			data->uFactoryProxAvg[1] = proxrawdata->prox[0];
		else if (proxrawdata->prox[0] < data->uFactoryProxAvg[1])
			data->uFactoryProxAvg[1] = proxrawdata->prox[0];

		if (proxrawdata->prox[0] > data->uFactoryProxAvg[3])
			data->uFactoryProxAvg[3] = proxrawdata->prox[0];
	}

	data->buf[PROXIMITY_RAW].prox[0] = proxrawdata->prox[0];
}

void report_geomagnetic_raw_data(struct ssp_data *data,
	struct sensor_value *magrawdata)
{
	data->buf[GEOMAGNETIC_RAW].x = magrawdata->x;
	data->buf[GEOMAGNETIC_RAW].y = magrawdata->y;
	data->buf[GEOMAGNETIC_RAW].z = magrawdata->z;
}

void report_temp_humidity_data(struct ssp_data *data,
	struct sensor_value *temp_humi_data)
{
	data->buf[TEMPERATURE_HUMIDITY_SENSOR].data[0] =
		temp_humi_data->data[0];
	data->buf[TEMPERATURE_HUMIDITY_SENSOR].data[1] =
		temp_humi_data->data[1];
	data->buf[TEMPERATURE_HUMIDITY_SENSOR].data[2] =
		temp_humi_data->data[2];

	/* Temperature */
	input_report_rel(data->temp_humi_input_dev, REL_HWHEEL,
		data->buf[TEMPERATURE_HUMIDITY_SENSOR].data[0]);
	/* Humidity */
	input_report_rel(data->temp_humi_input_dev, REL_DIAL,
		data->buf[TEMPERATURE_HUMIDITY_SENSOR].data[1]);

	/* Compensation engine cmd */
	if (data->comp_engine_cmd != SHTC1_CMD_NONE) {
		input_report_rel(data->temp_humi_input_dev, REL_WHEEL,
			data->comp_engine_cmd);
		data->comp_engine_cmd = SHTC1_CMD_NONE;
	}
	input_sync(data->temp_humi_input_dev);
	if (data->buf[TEMPERATURE_HUMIDITY_SENSOR].data[2])
		wake_lock_timeout(&data->ssp_wake_lock, 2 * HZ);
}

int initialize_event_symlink(struct ssp_data *data)
{
	int iRet = 0;

	iRet = sensors_create_symlink(&data->acc_input_dev->dev.kobj,
		data->acc_input_dev->name);
	if (iRet < 0)
		goto iRet_acc_sysfs_create_link;

	iRet = sensors_create_symlink(&data->gyro_input_dev->dev.kobj,
		data->gyro_input_dev->name);
	if (iRet < 0)
		goto iRet_gyro_sysfs_create_link;

	iRet = sensors_create_symlink(&data->pressure_input_dev->dev.kobj,
		data->pressure_input_dev->name);
	if (iRet < 0)
		goto iRet_prs_sysfs_create_link;

	iRet = sensors_create_symlink(&data->gesture_input_dev->dev.kobj,
		data->gesture_input_dev->name);
	if (iRet < 0)
		goto iRet_gesture_sysfs_create_link;

	iRet = sensors_create_symlink(&data->light_input_dev->dev.kobj,
		data->light_input_dev->name);
	if (iRet < 0)
		goto iRet_light_sysfs_create_link;

	iRet = sensors_create_symlink(&data->prox_input_dev->dev.kobj,
		data->prox_input_dev->name);
	if (iRet < 0)
		goto iRet_prox_sysfs_create_link;

	iRet = sensors_create_symlink(&data->temp_humi_input_dev->dev.kobj,
		data->temp_humi_input_dev->name);
	if (iRet < 0)
		goto iRet_temp_humi_sysfs_create_link;

	iRet = sensors_create_symlink(&data->mag_input_dev->dev.kobj,
		data->mag_input_dev->name);
	if (iRet < 0)
		goto iRet_mag_sysfs_create_link;

	return SUCCESS;
iRet_mag_sysfs_create_link:
	sensors_remove_symlink(&data->temp_humi_input_dev->dev.kobj,
		data->temp_humi_input_dev->name);
iRet_temp_humi_sysfs_create_link:
	sensors_remove_symlink(&data->prox_input_dev->dev.kobj,
		data->prox_input_dev->name);
iRet_prox_sysfs_create_link:
	sensors_remove_symlink(&data->light_input_dev->dev.kobj,
		data->light_input_dev->name);
iRet_light_sysfs_create_link:
	sensors_remove_symlink(&data->gesture_input_dev->dev.kobj,
		data->gesture_input_dev->name);
iRet_gesture_sysfs_create_link:
	sensors_remove_symlink(&data->pressure_input_dev->dev.kobj,
		data->pressure_input_dev->name);
iRet_prs_sysfs_create_link:
	sensors_remove_symlink(&data->gyro_input_dev->dev.kobj,
		data->gyro_input_dev->name);
iRet_gyro_sysfs_create_link:
	sensors_remove_symlink(&data->acc_input_dev->dev.kobj,
		data->acc_input_dev->name);
iRet_acc_sysfs_create_link:
	pr_err("[SSP]: %s - could not create event symlink\n", __func__);

	return FAIL;
}

void remove_event_symlink(struct ssp_data *data)
{
	sensors_remove_symlink(&data->acc_input_dev->dev.kobj,
		data->acc_input_dev->name);
	sensors_remove_symlink(&data->gyro_input_dev->dev.kobj,
		data->gyro_input_dev->name);
	sensors_remove_symlink(&data->pressure_input_dev->dev.kobj,
		data->pressure_input_dev->name);
	sensors_remove_symlink(&data->gesture_input_dev->dev.kobj,
		data->gesture_input_dev->name);
	sensors_remove_symlink(&data->light_input_dev->dev.kobj,
		data->light_input_dev->name);
	sensors_remove_symlink(&data->prox_input_dev->dev.kobj,
		data->prox_input_dev->name);
	sensors_remove_symlink(&data->temp_humi_input_dev->dev.kobj,
		data->temp_humi_input_dev->name);
	sensors_remove_symlink(&data->mag_input_dev->dev.kobj,
		data->mag_input_dev->name);
}

int initialize_input_dev(struct ssp_data *data)
{
	int iRet = 0;
	struct input_dev *acc_input_dev, *gyro_input_dev, *pressure_input_dev,
		*light_input_dev, *prox_input_dev, *temp_humi_input_dev,
		*mag_input_dev, *gesture_input_dev;

	/* allocate input_device */
	acc_input_dev = input_allocate_device();
	if (acc_input_dev == NULL)
		goto iRet_acc_input_free_device;

	gyro_input_dev = input_allocate_device();
	if (gyro_input_dev == NULL)
		goto iRet_gyro_input_free_device;

	pressure_input_dev = input_allocate_device();
	if (pressure_input_dev == NULL)
		goto iRet_pressure_input_free_device;

	gesture_input_dev = input_allocate_device();
	if (gesture_input_dev == NULL)
		goto iRet_gesture_input_free_device;

	light_input_dev = input_allocate_device();
	if (light_input_dev == NULL)
		goto iRet_light_input_free_device;

	prox_input_dev = input_allocate_device();
	if (prox_input_dev == NULL)
		goto iRet_proximity_input_free_device;

	temp_humi_input_dev = input_allocate_device();
	if (temp_humi_input_dev == NULL)
		goto iRet_temp_humidity_input_free_device;

	mag_input_dev = input_allocate_device();
	if (mag_input_dev == NULL)
		goto iRet_mag_input_free_device;

	input_set_drvdata(acc_input_dev, data);
	input_set_drvdata(gyro_input_dev, data);
	input_set_drvdata(pressure_input_dev, data);
	input_set_drvdata(gesture_input_dev, data);
	input_set_drvdata(light_input_dev, data);
	input_set_drvdata(prox_input_dev, data);
	input_set_drvdata(temp_humi_input_dev, data);
	input_set_drvdata(mag_input_dev, data);

	acc_input_dev->name = "accelerometer_sensor";
	gyro_input_dev->name = "gyro_sensor";
	pressure_input_dev->name = "pressure_sensor";
	gesture_input_dev->name = "gesture_sensor";
	light_input_dev->name = "light_sensor";
	prox_input_dev->name = "proximity_sensor";
	temp_humi_input_dev->name = "temp_humidity_sensor";
	mag_input_dev->name = "geomagnetic_sensor";

	input_set_capability(acc_input_dev, EV_REL, REL_X);
	input_set_capability(acc_input_dev, EV_REL, REL_Y);
	input_set_capability(acc_input_dev, EV_REL, REL_Z);

	input_set_capability(gyro_input_dev, EV_REL, REL_RX);
	input_set_capability(gyro_input_dev, EV_REL, REL_RY);
	input_set_capability(gyro_input_dev, EV_REL, REL_RZ);

	input_set_capability(pressure_input_dev, EV_REL, REL_HWHEEL);
	input_set_capability(pressure_input_dev, EV_REL, REL_DIAL);
	input_set_capability(pressure_input_dev, EV_REL, REL_WHEEL);

	input_set_capability(gesture_input_dev, EV_ABS, ABS_RUDDER);
	input_set_abs_params(gesture_input_dev, ABS_RUDDER, 0, 1024, 0, 0);
	input_set_capability(gesture_input_dev, EV_ABS, ABS_WHEEL);
	input_set_abs_params(gesture_input_dev, ABS_WHEEL, 0, 1024, 0, 0);
	input_set_capability(gesture_input_dev, EV_ABS, ABS_GAS);
	input_set_abs_params(gesture_input_dev, ABS_GAS, 0, 1024, 0, 0);
	input_set_capability(gesture_input_dev, EV_ABS, ABS_BRAKE);
	input_set_abs_params(gesture_input_dev, ABS_BRAKE, 0, 1024, 0, 0);
	input_set_capability(gesture_input_dev, EV_ABS, ABS_THROTTLE);
	input_set_abs_params(gesture_input_dev, ABS_THROTTLE, 0, 1024, 0, 0);
	input_set_capability(gesture_input_dev, EV_ABS, ABS_X);
	input_set_abs_params(gesture_input_dev, ABS_X, 0, 1024, 0, 0);
	input_set_capability(gesture_input_dev, EV_ABS, ABS_Y);
	input_set_abs_params(gesture_input_dev, ABS_Y, 0, 1024, 0, 0);
	input_set_capability(gesture_input_dev, EV_ABS, ABS_Z);
	input_set_abs_params(gesture_input_dev, ABS_Z, 0, 1024, 0, 0);
	input_set_capability(gesture_input_dev, EV_ABS, ABS_RX);
	input_set_abs_params(gesture_input_dev, ABS_RX, 0, 1024, 0, 0);

	input_set_capability(light_input_dev, EV_REL, REL_HWHEEL);
	input_set_capability(light_input_dev, EV_REL, REL_DIAL);
	input_set_capability(light_input_dev, EV_REL, REL_WHEEL);
	input_set_capability(light_input_dev, EV_REL, REL_MISC);

	input_set_capability(prox_input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(prox_input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	input_set_capability(temp_humi_input_dev, EV_REL, REL_HWHEEL);
	input_set_capability(temp_humi_input_dev, EV_REL, REL_DIAL);

	input_set_capability(mag_input_dev, EV_REL, REL_RX);
	input_set_capability(mag_input_dev, EV_REL, REL_RY);
	input_set_capability(mag_input_dev, EV_REL, REL_RZ);

	/* register input_device */
	iRet = input_register_device(acc_input_dev);
	if (iRet < 0)
		goto iRet_acc_input_unreg_device;

	iRet = input_register_device(gyro_input_dev);
	if (iRet < 0) {
		input_free_device(gyro_input_dev);
		input_free_device(pressure_input_dev);
		input_free_device(gesture_input_dev);
		input_free_device(light_input_dev);
		input_free_device(prox_input_dev);
		input_free_device(temp_humi_input_dev);
		input_free_device(mag_input_dev);
		goto iRet_gyro_input_unreg_device;
	}

	iRet = input_register_device(pressure_input_dev);
	if (iRet < 0) {
		input_free_device(pressure_input_dev);
		input_free_device(gesture_input_dev);
		input_free_device(light_input_dev);
		input_free_device(prox_input_dev);
		input_free_device(temp_humi_input_dev);
		input_free_device(mag_input_dev);
		goto iRet_pressure_input_unreg_device;
	}

	iRet = input_register_device(gesture_input_dev);
	if (iRet < 0) {
		input_free_device(gesture_input_dev);
		input_free_device(light_input_dev);
		input_free_device(prox_input_dev);
		input_free_device(temp_humi_input_dev);
		input_free_device(mag_input_dev);
		goto iRet_gesture_input_unreg_device;
	}

	iRet = input_register_device(light_input_dev);
	if (iRet < 0) {
		input_free_device(light_input_dev);
		input_free_device(prox_input_dev);
		input_free_device(temp_humi_input_dev);
		input_free_device(mag_input_dev);
		goto iRet_light_input_unreg_device;
	}

	iRet = input_register_device(prox_input_dev);
	if (iRet < 0) {
		input_free_device(prox_input_dev);
		input_free_device(temp_humi_input_dev);
		input_free_device(mag_input_dev);
		goto iRet_proximity_input_unreg_device;
	}

	iRet = input_register_device(temp_humi_input_dev);
	if (iRet < 0) {
		input_free_device(temp_humi_input_dev);
		input_free_device(mag_input_dev);
		goto iRet_tmep_humi_input_unreg_device;
	}

	iRet = input_register_device(mag_input_dev);
	if (iRet < 0) {
		input_free_device(mag_input_dev);
		goto iRet_mag_input_unreg_device;
	}

	data->acc_input_dev = acc_input_dev;
	data->gyro_input_dev = gyro_input_dev;
	data->pressure_input_dev = pressure_input_dev;
	data->gesture_input_dev = gesture_input_dev;
	data->light_input_dev = light_input_dev;
	data->prox_input_dev = prox_input_dev;
	data->temp_humi_input_dev = temp_humi_input_dev;
	data->mag_input_dev = mag_input_dev;

	return SUCCESS;

iRet_mag_input_unreg_device:
	input_unregister_device(temp_humi_input_dev);
iRet_tmep_humi_input_unreg_device:
	input_unregister_device(prox_input_dev);
iRet_proximity_input_unreg_device:
	input_unregister_device(light_input_dev);
iRet_light_input_unreg_device:
	input_unregister_device(pressure_input_dev);
iRet_pressure_input_unreg_device:
	input_unregister_device(gyro_input_dev);
iRet_gesture_input_unreg_device:
	input_unregister_device(gesture_input_dev);
iRet_gyro_input_unreg_device:
	input_unregister_device(acc_input_dev);
	return ERROR;
iRet_acc_input_unreg_device:
	pr_err("[SSP]: %s - could not register input device\n", __func__);
	input_free_device(mag_input_dev);
iRet_mag_input_free_device:
	input_free_device(temp_humi_input_dev);
iRet_temp_humidity_input_free_device:
	input_free_device(prox_input_dev);
iRet_proximity_input_free_device:
	input_free_device(light_input_dev);
iRet_light_input_free_device:
	input_free_device(pressure_input_dev);
iRet_gesture_input_free_device:
	input_free_device(gesture_input_dev);
iRet_pressure_input_free_device:
	input_free_device(gyro_input_dev);
iRet_gyro_input_free_device:
	input_free_device(acc_input_dev);
iRet_acc_input_free_device:
	pr_err("[SSP]: %s - could not allocate input device\n", __func__);
	return ERROR;
}

void remove_input_dev(struct ssp_data *data)
{
	input_unregister_device(data->acc_input_dev);
	input_unregister_device(data->gyro_input_dev);
	input_unregister_device(data->pressure_input_dev);
	input_unregister_device(data->gesture_input_dev);
	input_unregister_device(data->light_input_dev);
	input_unregister_device(data->prox_input_dev);
	input_unregister_device(data->temp_humi_input_dev);
	input_unregister_device(data->mag_input_dev);
}
