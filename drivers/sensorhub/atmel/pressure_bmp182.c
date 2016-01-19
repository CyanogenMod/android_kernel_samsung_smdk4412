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

#define	VENDOR		"BOSCH"
#define	CHIP_ID		"BMP180"

#define CALIBRATION_FILE_PATH		"/efs/FactoryApp/baro_delta"

#define	PR_ABS_MAX	8388607		/* 24 bit 2'compl */
#define	PR_ABS_MIN	-8388608

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

static ssize_t sea_level_pressure_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iNewSeaLevelPressure;

	sscanf(buf, "%d", &iNewSeaLevelPressure);

	if (iNewSeaLevelPressure == 0) {
		pr_info("%s, our->temperature = 0\n", __func__);
		iNewSeaLevelPressure = -1;
	}

	input_report_rel(data->pressure_input_dev, REL_DIAL,
		iNewSeaLevelPressure);
	input_sync(data->pressure_input_dev);

	return size;
}

int pressure_open_calibration(struct ssp_data *data)
{
	char chBuf[10] = {0,};
	int iErr = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		iErr = PTR_ERR(cal_filp);
		if (iErr != -ENOENT)
			pr_err("[SSP]: %s - Can't open calibration file(%d)\n",
				__func__, iErr);
		set_fs(old_fs);
		return iErr;
	}
	iErr = cal_filp->f_op->read(cal_filp,
		chBuf, 10 * sizeof(char), &cal_filp->f_pos);
	if (iErr < 0) {
		pr_err("[SSP]: %s - Can't read the cal data from file (%d)\n",
			__func__, iErr);
		return iErr;
	}
	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	iErr = kstrtoint(chBuf, 10, &data->iPressureCal);
	if (iErr < 0) {
		pr_err("[SSP]: %s - kstrtoint failed. %d", __func__, iErr);
		return iErr;
	}

	ssp_dbg("[SSP]: open barometer calibration %d\n", data->iPressureCal);

	if (data->iPressureCal < PR_ABS_MIN || data->iPressureCal > PR_ABS_MAX)
		pr_err("[SSP]: %s - wrong offset value!!!\n", __func__);

	return iErr;
}

static ssize_t pressure_cabratioin_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iPressureCal = 0, iErr = 0;

	iErr = kstrtoint(buf, 10, &iPressureCal);
	if (iErr < 0) {
		pr_err("[SSP]: %s - kstrtoint failed.(%d)", __func__, iErr);
		return iErr;
	}

	if (iPressureCal < PR_ABS_MIN || iPressureCal > PR_ABS_MAX)
		return -EINVAL;

	data->iPressureCal = (s32)iPressureCal;

	return size;
}

static ssize_t pressure_cabratioin_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pressure_open_calibration(data);

	return sprintf(buf, "%d\n", data->iPressureCal);
}

static ssize_t eeprom_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bSuccess = false;
	char chTempBuf[2] = {0, 10};
	int iRet, iDelayCnt = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	data->uFactorydataReady = 0;
	memset(data->uFactorydata, 0, sizeof(char) * FACTORY_DATA_MAX);

	iRet = send_instruction(data, FACTORY_MODE, PRESSURE_FACTORY,
			chTempBuf, 2);

	while (!(data->uFactorydataReady & (1 << PRESSURE_FACTORY))
		&& (iDelayCnt++ < 150)
		&& (iRet == SUCCESS))
		msleep(20);

	if ((iDelayCnt >= 150) || (iRet != SUCCESS)) {
		pr_err("[SSP]: %s - Pressure Selftest Timeout!!\n",
			__func__);
		goto exit;
	}

	mdelay(5);

	bSuccess = (bool)(!!data->uFactorydata[0]);
	ssp_dbg("[SSP]: %s - %u\n", __func__, bSuccess);

exit:
	return snprintf(buf, PAGE_SIZE, "%d", bSuccess);
}

/* sysfs for vendor & name */
static ssize_t pressure_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t pressure_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static DEVICE_ATTR(vendor,  S_IRUGO, pressure_vendor_show, NULL);
static DEVICE_ATTR(name,  S_IRUGO, pressure_name_show, NULL);
static DEVICE_ATTR(eeprom_check, S_IRUGO, eeprom_check_show, NULL);
static DEVICE_ATTR(calibration,  S_IRUGO | S_IWUSR | S_IWGRP,
	pressure_cabratioin_show, pressure_cabratioin_store);
static DEVICE_ATTR(sea_level_pressure, S_IRUGO | S_IWUSR | S_IWGRP,
	NULL, sea_level_pressure_store);

static struct device_attribute *pressure_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_calibration,
	&dev_attr_sea_level_pressure,
	&dev_attr_eeprom_check,
	NULL,
};

void initialize_pressure_factorytest(struct ssp_data *data)
{
	sensors_register(data->prs_device, data, pressure_attrs,
		"barometer_sensor");
}

void remove_pressure_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->prs_device, pressure_attrs);
}
