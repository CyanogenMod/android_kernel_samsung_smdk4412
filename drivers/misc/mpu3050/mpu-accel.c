/*
    mpu-accel.c - mpu3050 input device interface

    Copyright (C) 2010 InvenSense Corporation, All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/signal.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/suspend.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/input.h>

#include "mpuirq.h"
#include "slaveirq.h"
#include "mlsl.h"
#include "mpu-i2c.h"
#include "mldl_cfg.h"
#include "mpu.h"
#include "mpu-accel.h"

#define MPUACC_DEBUG 0
#define MPUACC_DEBUG_CFG 0

#define MPUACCEL_INPUT_NAME "mpu-accel"

struct mpuaccel_data {
	struct input_dev *input_data;
	struct delayed_work work;
	struct mutex data_mutex;

	struct mldl_cfg *mldl_cfg;
	void *accel_handle;

	atomic_t enable;
	atomic_t poll_delay;
	int device_is_on;
#ifdef MPUACC_USES_CACHED_DATA
	unsigned char cached_data[6];
#endif				/* MPUACC_USES_CACHED_DATA */
};

static struct mpuaccel_data *pThisData;
extern struct acc_data cal_data;

static void mpu_accel_print_mldl_cfg(struct mldl_cfg *mldl_cfg)
{
	if (MPUACC_DEBUG_CFG) {
		pr_info("requested_sensors:%ld\n", mldl_cfg->requested_sensors);
/*  pr_info("ignore_system_suspend:%d\n", mldl_cfg->ignore_system_suspend); */
		pr_info("addr:%d\n", mldl_cfg->addr);
		pr_info("int_config:%d\n", mldl_cfg->int_config);
		pr_info("ext_sync:%d\n", mldl_cfg->ext_sync);
		pr_info("full_scale:%d\n", mldl_cfg->full_scale);
		pr_info("dmp_enable:%d\n", mldl_cfg->dmp_enable);
		pr_info("fifo_enable:%d\n", mldl_cfg->fifo_enable);
		pr_info("dmp_cfg1:%d\n", mldl_cfg->dmp_cfg1);
		pr_info("dmp_cfg2:%d\n", mldl_cfg->dmp_cfg2);
		pr_info("gyro_power:%d\n", mldl_cfg->gyro_power);
		pr_info("gyro_is_bypassed:%d\n", mldl_cfg->gyro_is_bypassed);
		pr_info("dmp_is_running:%d\n", mldl_cfg->dmp_is_running);
		pr_info("gyro_is_suspended:%d\n", mldl_cfg->gyro_is_suspended);
		pr_info("accel_is_suspended:%d\n",
			mldl_cfg->accel_is_suspended);
		pr_info("compass_is_suspended:%d\n",
			mldl_cfg->compass_is_suspended);
		pr_info("pressure_is_suspended:%d\n",
			mldl_cfg->pressure_is_suspended);
		pr_info("gyro_needs_reset:%d\n", mldl_cfg->gyro_needs_reset);
	}
}

static int mpu_accel_mutex_lock(struct mpuaccel_data *data)
{
	mutex_lock(&data->data_mutex);

	return ML_SUCCESS;
}

static int mpu_accel_mutex_unlock(struct mpuaccel_data *data)
{
	mutex_unlock(&data->data_mutex);

	return ML_SUCCESS;
}

static int mpu_accel_activate_device(struct mpuaccel_data *data, int enable)
{
	int result = ML_SUCCESS;
	struct mldl_cfg *mldl_cfg = data->mldl_cfg;

	if (enable) {
		/*turn on accel */
		if (NULL != mldl_cfg->accel
		 && NULL != mldl_cfg->accel->resume) {
			result = mldl_cfg->accel->resume(data->accel_handle,
							 mldl_cfg->accel,
							 &mldl_cfg->pdata->
							 accel);
		}
	} else {
		/*turn off accel */
		if (NULL != mldl_cfg->accel
		 && NULL != mldl_cfg->accel->suspend) {
			result = mldl_cfg->accel->suspend(data->accel_handle,
							  mldl_cfg->accel,
							  &mldl_cfg->pdata->
							  accel);
		}
	}

	if (result == ML_SUCCESS)
		data->device_is_on = enable;

	if (MPUACC_DEBUG)
		pr_info("activate device:%d, result=%d\n", enable, result);

	return result;
}

static int mpu_accel_get_data_from_device(struct mpuaccel_data *data,
					 unsigned char *buffer)
{
	int result = ML_SUCCESS;
	struct mldl_cfg *mldl_cfg = data->mldl_cfg;

	if (NULL != mldl_cfg->accel && NULL != mldl_cfg->accel->read) {
		result = mldl_cfg->accel->read(data->accel_handle,
							 mldl_cfg->accel,
							 &mldl_cfg->pdata->accel, buffer);
	}

	return result;
}

static int mpu_accel_get_data_from_mpu(struct mpuaccel_data *data, unsigned char *buffer)
{
	int result = ML_SUCCESS;
	struct mldl_cfg *mldl_cfg = data->mldl_cfg;
	result =
	    MLSLSerialRead(data->accel_handle, mldl_cfg->addr, 0x23, 6, buffer);
	return result;
}

static int mpu_accel_get_data(struct mpuaccel_data *data, unsigned char *buffer,
		   int *from_mpu)
{
	int res = ML_SUCCESS;
	struct mldl_cfg *mldl_cfg = data->mldl_cfg;

	if (mldl_cfg->accel_is_suspended == 1 ||
	   (mldl_cfg->dmp_is_running == 0
	   && mldl_cfg->accel_is_suspended == 0)) {

		if (from_mpu != NULL)
			*from_mpu = 0;

		/*
		   Retrieve accel data from accel device driver directly.
		 */
		res = mpu_accel_get_data_from_device(data, buffer);
	} else if (mldl_cfg->dmp_is_running &&
		   mldl_cfg->accel_is_suspended == 0) {

		if (from_mpu != NULL)
			*from_mpu = 1;

		/*
		   Retrieve accel data from MPU registers(0x23 to 0x28).
		 */
		res = mpu_accel_get_data_from_mpu(data, buffer);
	}

	return res;
}

static int mpu_accel_build_data(struct mpuaccel_data *data,
			const unsigned char *buffer, s16 *val)
{
	struct mldl_cfg *mldl_cfg = data->mldl_cfg;
	int endian = mldl_cfg->accel->endian;
	int dev_id = mldl_cfg->accel->id;

	if (endian == EXT_SLAVE_LITTLE_ENDIAN) {
		if (dev_id == ACCEL_ID_BMA150)
			*val = (*(s16 *)&buffer[0]) >> 6;
		else if (dev_id == ACCEL_ID_KXTF9) {
			*val =
			    (short)(((signed char)buffer[1] << 4) |
			    (buffer[0] >> 4));
		} else
			*val = (buffer[1] << 8) | buffer[0];
	} else if (endian == EXT_SLAVE_BIG_ENDIAN) {
		*val = (buffer[0] << 8) | buffer[1];
	}

	return ML_SUCCESS;
}

static void mpu_accel_input_work_func(struct work_struct *work)
{
	int res = 0;
	int poll_time = 0;
	int enable = 0;
	int i = 0;

	struct mpuaccel_data *data = container_of((struct delayed_work *)work,
						  struct mpuaccel_data, work);

	struct mldl_cfg *mldl_cfg = data->mldl_cfg;

	poll_time = atomic_read(&data->poll_delay);

	if (MPUACC_DEBUG)
		pr_info("________________START____________________\n");
	if (MPUACC_DEBUG_CFG)
		mpu_accel_print_mldl_cfg(mldl_cfg);

	mpu_accel_mutex_lock(data);
	enable = atomic_read(&data->enable);
	mpu_accel_mutex_unlock(data);

	if (enable) {
		unsigned char buffer[6] = { 0, };
		s16 raw[3] = { 0, };
		int data_is_avail = 0;
		int data_is_from_mpu = 0;

		mpu_accel_mutex_lock(data);
		mpu_accel_get_data(data, buffer, &data_is_from_mpu);
		mpu_accel_mutex_unlock(data);

		if (res == ML_SUCCESS)
			data_is_avail = 1;

		if (data_is_avail) {
			int data_is_valid = 0;

			for (i = 0; i < 3; i++) {
				mpu_accel_build_data(data, &buffer[i * 2],
						     &raw[i]);
			}
			raw[0] += cal_data.x;
			raw[1] += cal_data.y;
			raw[2] += cal_data.z;

			if (raw[0] && raw[1] && raw[2])
				data_is_valid = 1;

			if (data_is_valid) {
				int accel[3] = { 0, };

				/*apply mounting matrix */
				for (i = 0; i < 3; i++) {
#ifdef MPUACC_USES_MOUNTING_MATRIX
					int j = 0;
					for (j = 0; j < 3; j++) {
						accel[i] +=
						    mldl_cfg->pdata->accel.
						    orientation[i * 3 +
								j] * raw[j];
					}
#else
					accel[i] = raw[i];
#endif
				}

				if (MPUACC_DEBUG) {
					if (data_is_from_mpu == 1)
						pr_info
						 ("MPU_ACCEL:[%d][%d][%d]\n",
						  accel[0], accel[1],
						  accel[2]);
					else
						pr_info("ACCEL:[%d][%d][%d]\n",
							 accel[0], accel[1],
							 accel[2]);
				}
#ifdef MPUACC_USES_CACHED_DATA
				memcpy(data->cached_data, buffer,
						 sizeof(unsigned char) * 6);
#endif /* #ifdef MPUACC_USES_CACHED_DATA */
				input_report_rel(data->input_data, REL_X,
						 accel[0]);
				input_report_rel(data->input_data, REL_Y,
						 accel[1]);
				input_report_rel(data->input_data, REL_Z,
						 accel[2]);
				input_sync(data->input_data);

				if (MPUACC_DEBUG)
					pr_info("input device is updated\n");
			}
		}
	}

	if (MPUACC_DEBUG)
		pr_info("________________END____________________\n");

	mpu_accel_mutex_lock(data);
	enable = atomic_read(&data->enable);
	mpu_accel_mutex_unlock(data);

	if (enable) {
		if (poll_time > 0) {
			schedule_delayed_work(&data->work,
						msecs_to_jiffies(poll_time)
						/*+ 1 */);
		} else {
			schedule_delayed_work(&data->work, 0);
		}

	}

}

static int mpu_accel_enable(struct mpuaccel_data *data)
{
	int res = ML_SUCCESS;
	struct mldl_cfg *mldl_cfg = data->mldl_cfg;

	if (MPUACC_DEBUG)
		pr_info("mpu_accel_enable : %d\n", atomic_read(&data->enable));

	if (atomic_read(&data->enable) != 1) {

		if (MPUACC_DEBUG)
			pr_info("mpu_accel_enable : enabled\n");

		if (mldl_cfg->accel_is_suspended == 1) {
			if (MPUACC_DEBUG)
				pr_info("mpu_accel_enable : turn on accel\n");
			mpu_accel_activate_device(data, 1);
		}

		atomic_set(&data->enable, 1);
		schedule_delayed_work(&data->work, 0);

	}

	return res;
}

static int mpu_accel_disable(struct mpuaccel_data *data)
{
	int res = ML_SUCCESS;
	struct mldl_cfg *mldl_cfg = data->mldl_cfg;

	if (MPUACC_DEBUG)
		pr_info("mpu_accel_disable : %d\n", atomic_read(&data->enable));

	if (atomic_read(&data->enable) != 0) {
		atomic_set(&data->enable, 0);
		cancel_delayed_work(&data->work);

		if (MPUACC_DEBUG)
			pr_info("mpu_accel_disable : disabled\n");

		if (mldl_cfg->accel_is_suspended == 1) {
			if (MPUACC_DEBUG)
				pr_info("mpu_accel_disable : turn off accel\n");

			/*turn off accel */
			mpu_accel_activate_device(data, 0);
		}
	}

	return res;
}

static ssize_t mpu_accel_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpuaccel_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->poll_delay));
}

static ssize_t mpu_accel_delay_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpuaccel_data *data = input_get_drvdata(input_data);
	int value = simple_strtoul(buf, NULL, 10);

	atomic_set(&data->poll_delay, value);
	return count;
}

static ssize_t mpu_accel_enable_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpuaccel_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->enable));
}

static ssize_t
mpu_accel_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpuaccel_data *data = input_get_drvdata(input_data);
	int value;

	value = simple_strtoul(buf, NULL, 10);
	if (value != 0 && value != 1)
		return count;

	mpu_accel_mutex_lock(data);

	if (value)
		mpu_accel_enable(data);
	else
		mpu_accel_disable(data);

	mpu_accel_mutex_unlock(data);

	return count;
}

int mpu_accel_is_active_device(void)
{
	int is_active = 0;

	if (pThisData != NULL) {
		mpu_accel_mutex_lock(pThisData);
		is_active = pThisData->device_is_on;
		mpu_accel_mutex_unlock(pThisData);
	}

	return is_active;
}

#ifdef MPUACC_USES_CACHED_DATA
int mpu_accel_get_cached_data(unsigned char *cache)
{
	int res = ML_ERROR;

	if (pThisData != NULL) {
		if (pThisData->device_is_on == 1) {
			memcpy(cache, pThisData->cached_data,
					 sizeof(unsigned char) * 6);
			pr_info("cached data:[%d][%d][%d][%d][%d][%d]\n",
				cache[0], cache[1],
				cache[2], cache[3],
				cache[4], cache[5]);
			res = ML_SUCCESS;
		}

	}

	return res;
}
#endif /* MPUACC_USES_CACHED_DATA */

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
			mpu_accel_delay_show, mpu_accel_delay_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
			mpu_accel_enable_show, mpu_accel_enable_store);

static struct attribute *mpuaccel_attributes[] = {
	&dev_attr_poll_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group mpuaccel_attribute_group = {
	.attrs = mpuaccel_attributes
};

int mpu_accel_init(struct mldl_cfg *mldl_cfg, void *accel_handle)
{
	struct input_dev *input_data = NULL;
	struct mpuaccel_data *data = NULL;
	int res = 0;

	data = kzalloc(sizeof(struct mpuaccel_data), GFP_KERNEL);
	if (data == NULL) {
		res = -ENOMEM;
		goto err;
	}

	data->mldl_cfg = mldl_cfg;
	data->accel_handle = accel_handle;
	atomic_set(&data->enable, 0);
	atomic_set(&data->poll_delay, 20);	/* set 20ms to polling time */

	mutex_init(&data->data_mutex);

	INIT_DELAYED_WORK(&data->work, mpu_accel_input_work_func);

	input_data = input_allocate_device();
	if (input_data == NULL) {
		res = -ENOMEM;
		pr_err(
				 "mpu_accel_probe: Failed to allocate input_data device\n");
		goto err;
	}

	input_data->name = MPUACCEL_INPUT_NAME;
	input_data->id.bustype = BUS_I2C;

	set_bit(EV_REL, input_data->evbit);
	input_set_capability(input_data, EV_REL, REL_X);
	input_set_capability(input_data, EV_REL, REL_Y);
	input_set_capability(input_data, EV_REL, REL_Z);

	data->input_data = input_data;

	res = input_register_device(input_data);
	if (res) {
		pr_err(
				 "mpu_accel_init: Unable to register input_data device: %s\n",
				 input_data->name);
		goto err;
	}

	input_set_drvdata(input_data, data);
	mldl_cfg->ext.mpuacc_data = (void *)data;

	pThisData = data;

	res = sysfs_create_group(&input_data->dev.kobj,
				 &mpuaccel_attribute_group);
	if (res) {
		pr_err(
				 "mpu_accel_init: sysfs_create_group failed[%s]\n",
				 input_data->name);
		goto err;
	}

	return res;

err:
	sysfs_remove_group(&input_data->dev.kobj, &mpuaccel_attribute_group);
	input_free_device(input_data);
	kfree(data);
	return res;

}

int mpu_accel_exit(struct mldl_cfg *mldl_cfg)
{
	struct mpuaccel_data *data = NULL;

	if (mldl_cfg == NULL)
		return ML_ERROR;

	data = (struct mpuaccel_data *)mldl_cfg->ext.mpuacc_data;

	if (data != NULL) {
		sysfs_remove_group(&(data->input_data->dev.kobj),
					&mpuaccel_attribute_group);
		input_free_device(data->input_data);

		kfree(data);
		data = NULL;

		mldl_cfg->ext.mpuacc_data = NULL;
	}

	return ML_SUCCESS;
}

int mpu_accel_suspend(struct mldl_cfg *mldl_cfg)
{
	int result = ML_SUCCESS;
	int enable = 0;
	struct mpuaccel_data *data = NULL;

	if (mldl_cfg == NULL)
		return ML_ERROR;

	data = (struct mpuaccel_data *)mldl_cfg->ext.mpuacc_data;

	mpu_accel_mutex_lock(data);
	enable = atomic_read(&data->enable);

	pr_info("%s: device_is_on = %d, enable = %d\n",
		__func__, data->device_is_on, enable);

	if (data->device_is_on == 1 && enable == 0) {
		pr_info("%s: mpu_accel_activate_device 0\n", __func__);
		result = mpu_accel_activate_device(data, 0);
	}

	mpu_accel_mutex_unlock(data);

	return result;
}

int mpu_accel_resume(struct mldl_cfg *mldl_cfg)
{
	int result = ML_SUCCESS;
	int enable = 0;
	struct mpuaccel_data *data = NULL;

	if (mldl_cfg == NULL)
		return ML_ERROR;

	data = (struct mpuaccel_data *)mldl_cfg->ext.mpuacc_data;

	mpu_accel_mutex_lock(data);
	enable = atomic_read(&data->enable);

	pr_info("%s: device_is_on = %d, enable = %d\n",
		__func__, data->device_is_on, enable);

	if (data->device_is_on == 0 && enable == 0) {
		pr_info("%s: mpu_accel_activate_device 1\n", __func__);
		result = mpu_accel_activate_device(data, 1);
	}

	mpu_accel_mutex_unlock(data);

	return result;
}

int mpu_accel_read(struct mldl_cfg *mldl_cfg, unsigned char *buffer)
{
	int result = ML_SUCCESS;
	int enable = 0;
	struct mpuaccel_data *data = NULL;

	if (mldl_cfg == NULL)
		return ML_ERROR;

	data = (struct mpuaccel_data *)mldl_cfg->ext.mpuacc_data;

	mpu_accel_mutex_lock(data);
	enable = atomic_read(&data->enable);
#ifdef MPUACC_USES_CACHED_DATA
	if (enable == 1)
		memcpy(buffer, data->cached_data, sizeof(unsigned char) * 6);
	else
#endif /* MPUACC_USES_CACHED_DATA */
		result = mpu_accel_get_data_from_device(data, buffer);
	mpu_accel_mutex_unlock(data);

	return result;
}
