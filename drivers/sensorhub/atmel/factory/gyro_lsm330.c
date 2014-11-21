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
#include "../ssp.h"

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define VENDOR		"STM"
#define CHIP_ID		"LSM330"

#define CALIBRATION_FILE_PATH	"/efs/gyro_cal_data"
#define CALIBRATION_DATA_AMOUNT	20

static ssize_t gyro_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t gyro_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

int gyro_open_calibration(struct ssp_data *data)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);

		data->gyrocal.x = 0;
		data->gyrocal.y = 0;
		data->gyrocal.z = 0;

		return iRet;
	}

	iRet = cal_filp->f_op->read(cal_filp, (char *)&data->gyrocal,
		3 * sizeof(int), &cal_filp->f_pos);
	if (iRet != 3 * sizeof(int))
		iRet = -EIO;

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	ssp_dbg("[SSP]: open gyro calibration %d, %d, %d\n",
		data->gyrocal.x, data->gyrocal.y, data->gyrocal.z);
	return iRet;
}

static int save_gyro_caldata(struct ssp_data *data, s16 *iCalData)
{
	int iRet = 0;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;

	data->gyrocal.x = iCalData[0];
	data->gyrocal.y = iCalData[1];
	data->gyrocal.z = iCalData[2];

	ssp_dbg("[SSP]: do gyro calibrate %d, %d, %d\n",
		data->gyrocal.x, data->gyrocal.y, data->gyrocal.z);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP]: %s - Can't open calibration file\n", __func__);
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		return -EIO;
	}

	iRet = cal_filp->f_op->write(cal_filp, (char *)&data->gyrocal,
		3 * sizeof(int), &cal_filp->f_pos);
	if (iRet != 3 * sizeof(int)) {
		pr_err("[SSP]: %s - Can't write gyro cal to file\n", __func__);
		iRet = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return iRet;
}

static ssize_t gyro_power_off(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssp_dbg("[SSP]: %s\n", __func__);

	return sprintf(buf, "%d\n", 1);
}

static ssize_t gyro_power_on(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssp_dbg("[SSP]: %s\n", __func__);

	return sprintf(buf, "%d\n", 1);
}

static ssize_t gyro_get_temp(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char chTempBuf[2] = { 0, 10}, chTemp = 0;
	int iDelayCnt = 0, iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	if (!(data->uSensorState & (1 << GYROSCOPE_SENSOR)))
		goto exit;

	data->uFactorydataReady = 0;
	memset(data->uFactorydata, 0, sizeof(char) * FACTORY_DATA_MAX);

	iRet = send_instruction(data, FACTORY_MODE, GYROSCOPE_TEMP_FACTORY,
		chTempBuf, 2);

	while (!(data->uFactorydataReady & (1 << GYROSCOPE_TEMP_FACTORY))
		&& (iDelayCnt++ < 150)
		&& (iRet == SUCCESS))
		msleep(20);

	if ((iDelayCnt >= 150) || (iRet != SUCCESS)) {
		pr_err("[SSP]: %s - Gyro Temp Timeout!!\n", __func__);
		goto exit;
	}

	mdelay(5);

	chTemp = (char)data->uFactorydata[0];
	ssp_dbg("[SSP]: %s - %d\n", __func__, chTemp);
exit:
	return sprintf(buf, "%d\n", chTemp);
}

static ssize_t gyro_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char chTempBuf[2] = { 3, 200};
	u8 uFifoPass = 2;
	u8 uBypassPass = 2;
	u8 uCalPass = 0;
	s16 iNOST[3] = {0,}, iST[3] = {0,}, iCalData[3] = {0,};
	int iZeroRateData[3] = {0,};
	int iDelayCnt = 0, iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	data->uFactorydataReady = 0;
	memset(data->uFactorydata, 0, sizeof(char) * FACTORY_DATA_MAX);

	iRet = send_instruction(data, FACTORY_MODE, GYROSCOPE_FACTORY,
		chTempBuf, 2);

	while (!(data->uFactorydataReady & (1 << GYROSCOPE_FACTORY))
		&& (iDelayCnt++ < 250)
		&& (iRet == SUCCESS))
		msleep(20);

	if ((iDelayCnt >= 250) || (iRet != SUCCESS)) {
		pr_err("[SSP]: %s - Gyro Selftest Timeout!!\n", __func__);
		goto exit;
	}
	mdelay(5);

	iNOST[0] = (s16)((data->uFactorydata[0] << 8) + data->uFactorydata[1]);
	iNOST[1] = (s16)((data->uFactorydata[2] << 8) + data->uFactorydata[3]);
	iNOST[2] = (s16)((data->uFactorydata[4] << 8) + data->uFactorydata[5]);

	iST[0] = (s16)((data->uFactorydata[6] << 8) + data->uFactorydata[7]);
	iST[1] = (s16)((data->uFactorydata[8] << 8) + data->uFactorydata[9]);
	iST[2] = (s16)((data->uFactorydata[10] << 8) + data->uFactorydata[11]);

	iCalData[0] =
		(s16)((data->uFactorydata[12] << 8) + data->uFactorydata[13]);
	iCalData[1] =
		(s16)((data->uFactorydata[14] << 8) + data->uFactorydata[15]);
	iCalData[2] =
		(s16)((data->uFactorydata[16] << 8) + data->uFactorydata[17]);

	iZeroRateData[0] =
		(s16)((data->uFactorydata[18] << 8) + data->uFactorydata[19]);
	iZeroRateData[1] =
		(s16)((data->uFactorydata[20] << 8) + data->uFactorydata[21]);
	iZeroRateData[2] =
		(s16)((data->uFactorydata[22] << 8) + data->uFactorydata[23]);

	uCalPass = data->uFactorydata[24];
	uFifoPass = data->uFactorydata[25];
	uBypassPass = data->uFactorydata[26];

	if (uFifoPass && uBypassPass && uCalPass)
		save_gyro_caldata(data, iCalData);

exit:
	ssp_dbg("[SSP]: Gyro Selftest - %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		iNOST[0], iNOST[1], iNOST[2], iST[0], iST[1], iST[2],
		iZeroRateData[0], iZeroRateData[1], iZeroRateData[2],
		uFifoPass & uBypassPass & uCalPass, uFifoPass, uCalPass);

	return sprintf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		iNOST[0], iNOST[1], iNOST[2], iST[0], iST[1], iST[2],
		iZeroRateData[0], iZeroRateData[1], iZeroRateData[2],
		uFifoPass & uBypassPass & uCalPass, uFifoPass, uCalPass);
}

static ssize_t gyro_selftest_dps_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int iNewDps = 0;
	int iDelayCnt = 0, iRet = 0;
	char chTempBuf[2] = { 0, 10 };

	struct ssp_data *data = dev_get_drvdata(dev);

	if (!(data->uSensorState & (1 << GYROSCOPE_SENSOR)))
		goto exit;

	sscanf(buf, "%d", &iNewDps);

	if (iNewDps == GYROSCOPE_DPS250)
		chTempBuf[0] = 0;
	else if (iNewDps == GYROSCOPE_DPS500)
		chTempBuf[0] = 1;
	else if (iNewDps == GYROSCOPE_DPS2000)
		chTempBuf[0] = 2;
	else {
		chTempBuf[0] = 1;
		iNewDps = GYROSCOPE_DPS500;
	}

	data->uFactorydataReady = 0;
	memset(data->uFactorydata, 0, sizeof(char) * FACTORY_DATA_MAX);

	iRet = send_instruction(data, FACTORY_MODE, GYROSCOPE_DPS_FACTORY,
		chTempBuf, 2);

	while (!(data->uFactorydataReady & (1 << GYROSCOPE_DPS_FACTORY))
		&& (iDelayCnt++ < 150)
		&& (iRet == SUCCESS))
		msleep(20);

	if ((iDelayCnt >= 150) || (iRet != SUCCESS)) {
		pr_err("[SSP]: %s - Gyro Selftest DPS Timeout!!\n", __func__);
		goto exit;
	}

	mdelay(5);

	if (data->uFactorydata[0] != SUCCESS) {
		pr_err("[SSP]: %s - Gyro Selftest DPS Error!!\n", __func__);
		goto exit;
	}

	data->uGyroDps = (unsigned int)iNewDps;
	pr_err("[SSP]: %s - %u dps stored\n", __func__, data->uGyroDps);
exit:
	return count;
}

static ssize_t gyro_selftest_dps_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", data->uGyroDps);
}

static DEVICE_ATTR(name, S_IRUGO, gyro_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, gyro_vendor_show, NULL);
static DEVICE_ATTR(power_off, S_IRUGO, gyro_power_off, NULL);
static DEVICE_ATTR(power_on, S_IRUGO, gyro_power_on, NULL);
static DEVICE_ATTR(temperature, S_IRUGO, gyro_get_temp, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, gyro_selftest_show, NULL);
static DEVICE_ATTR(selftest_dps, S_IRUGO | S_IWUSR | S_IWGRP,
	gyro_selftest_dps_show, gyro_selftest_dps_store);

static struct device_attribute *gyro_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_selftest,
	&dev_attr_power_on,
	&dev_attr_power_off,
	&dev_attr_temperature,
	&dev_attr_selftest_dps,
	NULL,
};

void initialize_gyro_factorytest(struct ssp_data *data)
{
	sensors_register(data->gyro_device, data, gyro_attrs, "gyro_sensor");
}

void remove_gyro_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->gyro_device, gyro_attrs);
}
