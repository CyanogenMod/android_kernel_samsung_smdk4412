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

#define VENDOR		"AKM"
#define CHIP_ID		"AK8963C"

#define GYROSCOPE_DATA_SPEC_MIN		-6500
#define GYROSCOPE_DATA_SPEC_MAX		6500

#define GYROSCOPE_SELFTEST_X_SPEC_MIN	-200
#define GYROSCOPE_SELFTEST_X_SPEC_MAX	200

#define GYROSCOPE_SELFTEST_Y_SPEC_MIN	-200
#define GYROSCOPE_SELFTEST_Y_SPEC_MAX	200

#define GYROSCOPE_SELFTEST_Z_SPEC_MIN	-3200
#define GYROSCOPE_SELFTEST_Z_SPEC_MAX	-800

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

static ssize_t raw_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->buf[GEOMAGNETIC_SENSOR].x,
		data->buf[GEOMAGNETIC_SENSOR].y,
		data->buf[GEOMAGNETIC_SENSOR].z);
}

static int check_data_spec(struct ssp_data *data)
{
	if ((data->buf[GEOMAGNETIC_SENSOR].x == 0) &&
		(data->buf[GEOMAGNETIC_SENSOR].y == 0) &&
		(data->buf[GEOMAGNETIC_SENSOR].z == 0))
		return FAIL;
	else if ((data->buf[GEOMAGNETIC_SENSOR].x > GYROSCOPE_DATA_SPEC_MAX) ||
		(data->buf[GEOMAGNETIC_SENSOR].x < GYROSCOPE_DATA_SPEC_MIN) ||
		(data->buf[GEOMAGNETIC_SENSOR].y > GYROSCOPE_DATA_SPEC_MAX) ||
		(data->buf[GEOMAGNETIC_SENSOR].y < GYROSCOPE_DATA_SPEC_MIN) ||
		(data->buf[GEOMAGNETIC_SENSOR].z > GYROSCOPE_DATA_SPEC_MAX) ||
		(data->buf[GEOMAGNETIC_SENSOR].z < GYROSCOPE_DATA_SPEC_MIN))
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
	int iRetries = 20;
	struct ssp_data *data = dev_get_drvdata(dev);

	data->buf[GEOMAGNETIC_SENSOR].x = 0;
	data->buf[GEOMAGNETIC_SENSOR].y = 0;
	data->buf[GEOMAGNETIC_SENSOR].z = 0;

	if (!(atomic_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_SENSOR)))
		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_SENSOR,
			chTempbuf, 2);

	do {
		msleep(50);
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

static ssize_t magnetic_get_asa(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d,%d,%d\n", (s16)data->uFuseRomData[0],
		(s16)data->uFuseRomData[1], (s16)data->uFuseRomData[2]);
}

static ssize_t magnetic_get_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	bool bSuccess;
	struct ssp_data *data = dev_get_drvdata(dev);

	if ((data->uFuseRomData[0] == 0) ||
		(data->uFuseRomData[0] == 0xff) ||
		(data->uFuseRomData[1] == 0) ||
		(data->uFuseRomData[1] == 0xff) ||
		(data->uFuseRomData[2] == 0) ||
		(data->uFuseRomData[2] == 0xff))
		bSuccess = false;
	else
		bSuccess = true;

	return sprintf(buf, "%s,%u\n", (bSuccess ? "OK" : "NG"), bSuccess);
}

static ssize_t magnetic_get_selftest(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	bool bSelftestPassed = false;
	char chTempBuf[2] = { 0, 10 };
	s16 iSF_X = 0, iSF_Y = 0, iSF_Z = 0;
	int iDelayCnt = 0, iRet = 0, iTimeoutReties = 0, iSpecOutReties = 0;
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

	iSF_X = (s16)((data->uFactorydata[0] << 8) + data->uFactorydata[1]);
	iSF_Y = (s16)((data->uFactorydata[2] << 8) + data->uFactorydata[3]);
	iSF_Z = (s16)((data->uFactorydata[4] << 8) + data->uFactorydata[5]);

	iSF_X = (s16)(((int)iSF_X * ((int)data->uFuseRomData[0] + 128)) >> 8);
	iSF_Y = (s16)(((int)iSF_Y * ((int)data->uFuseRomData[1] + 128)) >> 8);
	iSF_Z = (s16)(((int)iSF_Z * ((int)data->uFuseRomData[2] + 128)) >> 8);

	pr_info("[SSP] %s: self test x = %d, y = %d, z = %d\n",
		__func__, iSF_X, iSF_Y, iSF_Z);
	if ((iSF_X >= GYROSCOPE_SELFTEST_X_SPEC_MIN)
		&& (iSF_X <= GYROSCOPE_SELFTEST_X_SPEC_MAX))
		pr_info("[SSP] x passed self test, expect -200<=x<=200\n");
	else
		pr_info("[SSP] x failed self test, expect -200<=x<=200\n");
	if ((iSF_Y >= GYROSCOPE_SELFTEST_Y_SPEC_MIN)
		&& (iSF_Y <= GYROSCOPE_SELFTEST_Y_SPEC_MAX))
		pr_info("[SSP] y passed self test, expect -200<=y<=200\n");
	else
		pr_info("[SSP] y failed self test, expect -200<=y<=200\n");
	if ((iSF_Z >= GYROSCOPE_SELFTEST_Z_SPEC_MIN)
		&& (iSF_Z <= GYROSCOPE_SELFTEST_Z_SPEC_MAX))
		pr_info("[SSP] z passed self test, expect -3200<=z<=-800\n");
	else
		pr_info("[SSP] z failed self test, expect -3200<=z<=-800\n");

	if ((iSF_X >= GYROSCOPE_SELFTEST_X_SPEC_MIN)
		&& (iSF_X <= GYROSCOPE_SELFTEST_X_SPEC_MAX)
		&& (iSF_Y >= GYROSCOPE_SELFTEST_Y_SPEC_MIN)
		&& (iSF_Y <= GYROSCOPE_SELFTEST_Y_SPEC_MAX)
		&& (iSF_Z >= GYROSCOPE_SELFTEST_Z_SPEC_MIN)
		&& (iSF_Z <= GYROSCOPE_SELFTEST_Z_SPEC_MAX))
		bSelftestPassed = true;

	if ((bSelftestPassed == false) && (iSpecOutReties++ < 5))
		goto reties;
exit:
	return sprintf(buf, "%u,%d,%d,%d\n",
		bSelftestPassed, iSF_X, iSF_Y, iSF_Z);
}

static ssize_t magnetic_check_registers(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 uBuf[13] = {0,};

	return sprintf(buf, "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
			uBuf[0], uBuf[1], uBuf[2], uBuf[3], uBuf[4], uBuf[5],
			uBuf[6], uBuf[7], uBuf[8], uBuf[9], uBuf[10], uBuf[11],
			uBuf[12]);
}

static ssize_t magnetic_check_cntl(struct device *dev,
		struct device_attribute *attr, char *strbuf)
{
	bool bSuccess = false;

	return sprintf(strbuf, "%s,%d,%d,%d\n",
		(!bSuccess ? "OK" : "NG"), 0, 0, 0);
}

static DEVICE_ATTR(name, S_IRUGO, magnetic_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, magnetic_vendor_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO, raw_data_read, NULL);
static DEVICE_ATTR(status, S_IRUGO,  magnetic_get_status, NULL);
static DEVICE_ATTR(adc, S_IRUGO, adc_data_read, NULL);
static DEVICE_ATTR(dac, S_IRUGO, magnetic_check_cntl, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, magnetic_get_selftest, NULL);
static DEVICE_ATTR(ak8963_asa, S_IRUGO, magnetic_get_asa, NULL);
static DEVICE_ATTR(ak8963_chk_registers, S_IRUGO,
	magnetic_check_registers, NULL);

static struct device_attribute *mag_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_adc,
	&dev_attr_raw_data,
	&dev_attr_status,
	&dev_attr_selftest,
	&dev_attr_ak8963_asa,
	&dev_attr_ak8963_chk_registers,
	&dev_attr_dac,
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
