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

#define	VENDOR		"MAXIM"
#define	CHIP_ID		"MAX88920"

static ssize_t gestrue_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t gestrue_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t raw_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n",
		data->buf[GESTURE_SENSOR].data[0],
		data->buf[GESTURE_SENSOR].data[1],
		data->buf[GESTURE_SENSOR].data[2],
		data->buf[GESTURE_SENSOR].data[3]);
}

static ssize_t gesture_get_selftest_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	s16 raw_A = 0, raw_B = 0, raw_C = 0, raw_D = 0;
	int iDelayCnt = 0, iRet = 0;
	char chTempBuf[2] = { 0, 10 };
	struct ssp_data *data = dev_get_drvdata(dev);

	iDelayCnt = 0;
	data->uFactorydataReady = 0;
	memset(data->uFactorydata, 0, sizeof(char) * FACTORY_DATA_MAX);

	iRet = send_instruction(data, FACTORY_MODE, GESTURE_FACTORY,
			chTempBuf, 2);

	while (!(data->uFactorydataReady & (1 << GESTURE_FACTORY))
		&& (iDelayCnt++ < 100)
		&& (iRet == SUCCESS))
		msleep(20);

	if ((iDelayCnt >= 100) || (iRet != SUCCESS)) {
		pr_err("[SSP]: %s - Gesture Selftest Timeout!!\n", __func__);
		goto exit;
	}

	raw_A = data->uFactorydata[0];
	raw_B = data->uFactorydata[1];
	raw_C = data->uFactorydata[2];
	raw_D = data->uFactorydata[3];

	pr_info("[SSP] %s: self test A = %d, B = %d, C = %d, D = %d\n",
		__func__, raw_A, raw_B, raw_C, raw_D);

exit:
	return sprintf(buf, "%d,%d,%d,%d\n",
            raw_A, raw_B, raw_C, raw_D);
}

static ssize_t ir_current_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: %s - Ir_Current Setting = %d\n",
		__func__, data->uIr_Current);

	return sprintf(buf, "%d\n", data->uIr_Current);
}

static ssize_t ir_current_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	u16 uNewIrCurrent = DEFUALT_IR_CURRENT;
	int iRet = 0;
	u16 current_index = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	static u16 set_current[2][16] = { {0, 25, 50, 75, 100, 125, 150, 175, 225, 250, 275, 300, 325, 350, 375, 400},
					  {2, 28, 34, 50, 66,  82,  98,  114, 130, 146, 162, 178, 194, 210, 226, 242} };

	iRet = kstrtou16(buf, 10, &uNewIrCurrent);
	if (iRet < 0)
		pr_err("[SSP]: %s - kstrtoint failed.(%d)\n", __func__, iRet);
	else {
		for(current_index = 0; current_index < 16; current_index++) {
			if (set_current[0][current_index] == uNewIrCurrent) {
				data->uIr_Current = set_current[1][current_index];
			}	
		}
		set_gesture_current(data, data->uIr_Current);
		data->uIr_Current = uNewIrCurrent;
	}

	ssp_dbg("[SSP]: %s - new Ir_Current Setting : %d\n",
        __func__, data->uIr_Current);

	return size;
}

static DEVICE_ATTR(vendor, S_IRUGO, gestrue_vendor_show, NULL);
static DEVICE_ATTR(name, S_IRUGO, gestrue_name_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO, raw_data_read, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, gesture_get_selftest_show, NULL);
static DEVICE_ATTR(ir_current, S_IRUGO | S_IWUSR | S_IWGRP,
    ir_current_show, ir_current_store);

static struct device_attribute *gesture_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_raw_data,
	&dev_attr_selftest,
	&dev_attr_ir_current,
	NULL,
};

void initialize_gesture_factorytest(struct ssp_data *data)
{
	sensors_register(data->ges_device, data,
		gesture_attrs, "gesture_sensor");
}

void remove_gesture_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->ges_device, gesture_attrs);
}
