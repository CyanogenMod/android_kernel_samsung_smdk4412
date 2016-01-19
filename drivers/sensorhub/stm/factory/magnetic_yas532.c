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

#define VENDOR		"YAMAHA"
#define CHIP_ID		"YAS532"
#define MAG_HW_OFFSET_FILE_PATH	"/efs/hw_offset"

int mag_open_hwoffset(struct ssp_data *data)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(MAG_HW_OFFSET_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP] %s: filp_open failed\n", __func__);
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);

		data->magoffset.x = 0;
		data->magoffset.y = 0;
		data->magoffset.z = 0;

		return iRet;
	}

	iRet = cal_filp->f_op->read(cal_filp, (char *)&data->magoffset,
		3 * sizeof(char), &cal_filp->f_pos);
	if (iRet != 3 * sizeof(char)) {
		pr_err("[SSP] %s: filp_open failed\n", __func__);
		iRet = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	ssp_dbg("[SSP]: %s: %d, %d, %d\n", __func__,
		(s8)data->magoffset.x,
		(s8)data->magoffset.y,
		(s8)data->magoffset.z);

	if ((data->magoffset.x == 0) && (data->magoffset.y == 0)
		&& (data->magoffset.z == 0))
		return ERROR;

	return iRet;
}

int mag_store_hwoffset(struct ssp_data *data)
{
	int iRet = 0;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;

	if (get_hw_offset(data) < 0) {
		pr_err("[SSP]: %s - get_hw_offset failed\n", __func__);
		return ERROR;
	} else {
		old_fs = get_fs();
		set_fs(KERNEL_DS);

		cal_filp = filp_open(MAG_HW_OFFSET_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, 0666);
		if (IS_ERR(cal_filp)) {
			pr_err("[SSP]: %s - Can't open hw_offset file\n",
				__func__);
			set_fs(old_fs);
			iRet = PTR_ERR(cal_filp);
			return iRet;
		}
		iRet = cal_filp->f_op->write(cal_filp,
			(char *)&data->magoffset,
			3 * sizeof(char), &cal_filp->f_pos);
		if (iRet != 3 * sizeof(char)) {
			pr_err("[SSP]: %s - Can't write the hw_offset"
				" to file\n", __func__);
			iRet = -EIO;
		}
		filp_close(cal_filp, current->files);
		set_fs(old_fs);
		return iRet;
	}
}

int set_hw_offset(struct ssp_data *data)
{
	char chTxBuf[4] = { 0, };
	char chRxBuf = 0;
	int iRet = 0;

	if (waiting_wakeup_mcu(data) < 0 ||
		data->fw_dl_state == FW_DL_STATE_DOWNLOADING) {
		pr_info("[SSP] : %s, skip DL state = %d\n", __func__,
			data->fw_dl_state);
		return FAIL;
	}

	chTxBuf[0] = MSG2SSP_AP_SET_MAGNETIC_HWOFFSET;
	chTxBuf[1] = data->magoffset.x;
	chTxBuf[2] = data->magoffset.y;
	chTxBuf[3] = data->magoffset.z;

	iRet = ssp_read_data(data, chTxBuf, 4, &chRxBuf, 1, DEFAULT_RETRIES);
	if ((chRxBuf != MSG_ACK) || (iRet != SUCCESS)) {
		pr_err("[SSP]: %s - spi fail %d, %d\n",
			__func__, iRet, chRxBuf);
		iRet = ERROR;
	}

	pr_info("[SSP]: %s: x: %d, y: %d, z: %d\n", __func__,
		(s8)chTxBuf[1], (s8)chTxBuf[2], (s8)chTxBuf[3]);
	return iRet;
}

int get_hw_offset(struct ssp_data *data)
{
	char chTxBuf = 0;
	char chRxBuf[3] = { 0, };
	int iRet = 0;

	if (waiting_wakeup_mcu(data) < 0)
		return ERROR;

	chTxBuf = MSG2SSP_AP_GET_MAGNETIC_HWOFFSET;

	data->magoffset.x = 0;
	data->magoffset.y = 0;
	data->magoffset.z = 0;

	iRet = ssp_read_data(data, &chTxBuf, 1, chRxBuf, 3, DEFAULT_RETRIES);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - spi fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	data->magoffset.x = chRxBuf[0];
	data->magoffset.y = chRxBuf[1];
	data->magoffset.z = chRxBuf[2];

	pr_info("[SSP]: %s: x: %d, y: %d, z: %d\n", __func__,
		(s8)data->magoffset.x,
		(s8)data->magoffset.y,
		(s8)data->magoffset.z);
	return iRet;
}

static ssize_t magnetic_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t magnetic_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static int check_rawdata_spec(struct ssp_data *data)
{
	if ((data->buf[GEOMAGNETIC_SENSOR].x == 0) &&
		(data->buf[GEOMAGNETIC_SENSOR].y == 0) &&
		(data->buf[GEOMAGNETIC_SENSOR].z == 0))
		return FAIL;
	else
		return SUCCESS;
}

static ssize_t raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s - %d,%d,%d\n", __func__,
		data->buf[GEOMAGNETIC_SENSOR].x,
		data->buf[GEOMAGNETIC_SENSOR].y,
		data->buf[GEOMAGNETIC_SENSOR].z);

	if (data->bGeomagneticRawEnabled == false) {
		data->buf[GEOMAGNETIC_SENSOR].x = -1;
		data->buf[GEOMAGNETIC_SENSOR].y = -1;
		data->buf[GEOMAGNETIC_SENSOR].z = -1;
	} else {
		if (data->buf[GEOMAGNETIC_SENSOR].x > 18000)
			data->buf[GEOMAGNETIC_SENSOR].x = 18000;
		else if (data->buf[GEOMAGNETIC_SENSOR].x < -18000)
			data->buf[GEOMAGNETIC_SENSOR].x = -18000;
		if (data->buf[GEOMAGNETIC_SENSOR].y > 18000)
			data->buf[GEOMAGNETIC_SENSOR].y = 18000;
		else if (data->buf[GEOMAGNETIC_SENSOR].y < -18000)
			data->buf[GEOMAGNETIC_SENSOR].y = -18000;
		if (data->buf[GEOMAGNETIC_SENSOR].z > 18000)
			data->buf[GEOMAGNETIC_SENSOR].z = 18000;
		else if (data->buf[GEOMAGNETIC_SENSOR].z < -18000)
			data->buf[GEOMAGNETIC_SENSOR].z = -18000;
	}
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->buf[GEOMAGNETIC_SENSOR].x,
		data->buf[GEOMAGNETIC_SENSOR].y,
		data->buf[GEOMAGNETIC_SENSOR].z);
}

static ssize_t raw_data_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	char chTempbuf[2] = { 1, 20};
	int iRet;
	int64_t dEnable;
	int iRetries = 50;
	struct ssp_data *data = dev_get_drvdata(dev);

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	if (dEnable) {
		data->buf[GEOMAGNETIC_SENSOR].x = 0;
		data->buf[GEOMAGNETIC_SENSOR].y = 0;
		data->buf[GEOMAGNETIC_SENSOR].z = 0;

		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_RAW,
			chTempbuf, 2);

		do {
			msleep(20);
			if (check_rawdata_spec(data) == SUCCESS)
				break;
		} while (--iRetries);

		if (iRetries > 0) {
			pr_info("[SSP] %s - success, %d\n", __func__, iRetries);
			data->bGeomagneticRawEnabled = true;
		} else {
			pr_err("[SSP] %s - wait timeout, %d\n", __func__,
				iRetries);
			data->bGeomagneticRawEnabled = false;
		}
	} else {
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_RAW,
			chTempbuf, 2);
		data->bGeomagneticRawEnabled = false;
	}

	return size;
}

static int check_data_spec(struct ssp_data *data)
{
	if ((data->buf[GEOMAGNETIC_SENSOR].x == 0) &&
		(data->buf[GEOMAGNETIC_SENSOR].y == 0) &&
		(data->buf[GEOMAGNETIC_SENSOR].z == 0))
		return FAIL;
	else if ((data->buf[GEOMAGNETIC_SENSOR].x > 6500) ||
		(data->buf[GEOMAGNETIC_SENSOR].x < -6500) ||
		(data->buf[GEOMAGNETIC_SENSOR].y > 6500) ||
		(data->buf[GEOMAGNETIC_SENSOR].y < -6500) ||
		(data->buf[GEOMAGNETIC_SENSOR].z > 6500) ||
		(data->buf[GEOMAGNETIC_SENSOR].z < -6500))
		return FAIL;
	else
		return SUCCESS;
}

static ssize_t adc_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bSuccess = false;
	u8 chTempbuf[2] = {1, 20};
	s16 iSensorBuf[3] = {0, };
	int iRetries = 10;
	struct ssp_data *data = dev_get_drvdata(dev);

	data->buf[GEOMAGNETIC_SENSOR].x = 0;
	data->buf[GEOMAGNETIC_SENSOR].y = 0;
	data->buf[GEOMAGNETIC_SENSOR].z = 0;

	if (!(atomic_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_SENSOR)))
		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_SENSOR,
			chTempbuf, 2);

	do {
		msleep(60);
		if (check_data_spec(data) == SUCCESS)
			break;
	} while (--iRetries);

	if (iRetries > 0)
		bSuccess = true;

	iSensorBuf[0] = data->buf[GEOMAGNETIC_SENSOR].x;
	iSensorBuf[1] = data->buf[GEOMAGNETIC_SENSOR].y;
	iSensorBuf[2] = data->buf[GEOMAGNETIC_SENSOR].z;

	if (!(atomic_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_SENSOR)))
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_SENSOR,
			chTempbuf, 2);

	pr_info("[SSP]: %s - x = %d, y = %d, z = %d\n", __func__,
		iSensorBuf[0], iSensorBuf[1], iSensorBuf[2]);

	return sprintf(buf, "%s,%d,%d,%d\n", (bSuccess ? "OK" : "NG"),
		iSensorBuf[0], iSensorBuf[1], iSensorBuf[2]);
}

static ssize_t magnetic_get_selftest(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char chTempBuf[2] = { 0, 10 };
	int iDelayCnt = 0, iRet = 0, iTimeoutReties = 0;
	s8 id = 0, x = 0, y1 = 0, y2 = 0, dir = 0;
	s16 sx = 0, sy = 0, ohx = 0, ohy = 0, ohz = 0;
	s8 err[7] = {0, };
	struct ssp_data *data = dev_get_drvdata(dev);

reties:
	iDelayCnt = 0;
	data->uFactorydataReady = 0;
	memset(data->uFactorydata, 0, sizeof(char) * FACTORY_DATA_MAX);

	iRet = send_instruction(data, FACTORY_MODE, GEOMAGNETIC_FACTORY,
			chTempBuf, 2);

	while (!(data->uFactorydataReady & (1 << GEOMAGNETIC_FACTORY))
		&& (iDelayCnt++ < 50)
		&& (iRet == SUCCESS))
		msleep(20);

	if ((iDelayCnt >= 50) || (iRet != SUCCESS)) {
		pr_err("[SSP]: %s - Magnetic Selftest Timeout!! %d\n",
			__func__, iRet);
		if (iTimeoutReties++ < 3)
			goto reties;
		else
			goto exit;
	}

	mdelay(5);

	id = (s8)(data->uFactorydata[0]);
	err[0] = (s8)(data->uFactorydata[1]);
	err[1] = (s8)(data->uFactorydata[2]);
	err[2] = (s8)(data->uFactorydata[3]);
	x = (s8)(data->uFactorydata[4]);
	y1 = (s8)(data->uFactorydata[5]);
	y2 = (s8)(data->uFactorydata[6]);
	err[3] = (s8)(data->uFactorydata[7]);
	dir = (s8)(data->uFactorydata[8]);
	err[4] = (s8)(data->uFactorydata[9]);
	ohx = (s16)((data->uFactorydata[10] << 8) + data->uFactorydata[11]);
	ohy = (s16)((data->uFactorydata[12] << 8) + data->uFactorydata[13]);
	ohz = (s16)((data->uFactorydata[14] << 8) + data->uFactorydata[15]);
	err[6] = (s8)(data->uFactorydata[16]);
	sx = (s16)((data->uFactorydata[17] << 8) + data->uFactorydata[18]);
	sy = (s16)((data->uFactorydata[19] << 8) + data->uFactorydata[20]);
	err[5] = (s8)(data->uFactorydata[21]);

	if (unlikely(id != 0x2))
		err[0] = -1;
	if (unlikely(x < -30 || x > 30))
		err[3] = -1;
	if (unlikely(y1 < -30 || y1 > 30))
		err[3] = -1;
	if (unlikely(y2 < -30 || y2 > 30))
		err[3] = -1;
	if (unlikely(sx < 17 || sy < 22))
		err[5] = -1;
	if (unlikely(ohx < -600 || ohx > 600))
		err[6] = -1;
	if (unlikely(ohy < -600 || ohy > 600))
		err[6] = -1;
	if (unlikely(ohz < -600 || ohz > 600))
		err[6] = -1;

	pr_info("[SSP] %s\n"
		"[SSP] Test1 - err = %d, id = %d\n"
		"[SSP] Test3 - err = %d\n"
		"[SSP] Test4 - err = %d, offset = %d,%d,%d\n"
		"[SSP] Test5 - err = %d, direction = %d\n"
		"[SSP] Test6 - err = %d, sensitivity = %d,%d\n"
		"[SSP] Test7 - err = %d, offset = %d,%d,%d\n"
		"[SSP] Test2 - err = %d\n",
		__func__, err[0], id, err[2], err[3], x, y1, y2, err[4], dir,
		err[5], sx, sy, err[6], ohx, ohy, ohz, err[1]);

exit:
	return sprintf(buf,
			"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			err[0], id, err[2], err[3], x, y1, y2, err[4], dir,
			err[5], sx, sy, err[6], ohx, ohy, ohz, err[1]);
}

static ssize_t hw_offset_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	mag_open_hwoffset(data);

	pr_info("[SSP] %s: %d %d %d\n", __func__,
		(s8)data->magoffset.x,
		(s8)data->magoffset.y,
		(s8)data->magoffset.z);

	return sprintf(buf, "%d %d %d\n",
		(s8)data->magoffset.x,
		(s8)data->magoffset.y,
		(s8)data->magoffset.z);
}

static DEVICE_ATTR(name, S_IRUGO, magnetic_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, magnetic_vendor_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO | S_IWUSR | S_IWGRP,
	raw_data_show, raw_data_store);
static DEVICE_ATTR(adc, S_IRUGO, adc_data_read, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, magnetic_get_selftest, NULL);
static DEVICE_ATTR(hw_offset, S_IRUGO, hw_offset_show, NULL);

static struct device_attribute *mag_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_adc,
	&dev_attr_raw_data,
	&dev_attr_selftest,
	&dev_attr_hw_offset,
	NULL,
};

void initialize_magnetic_factorytest(struct ssp_data *data)
{
	sensors_register(data->mag_device, data, mag_attrs, "magnetic_sensor");
}

void remove_magnetic_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->mag_device, mag_attrs);
}
