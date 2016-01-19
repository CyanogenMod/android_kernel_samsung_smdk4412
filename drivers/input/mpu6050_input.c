/*
	$License:
	Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.

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
	$
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/wakelock.h>
#include <linux/sensor/sensors_core.h>
#include <linux/mpu6050_input.h>
#include "mpu6050_selftest.h"

#define LOG_RESULT_LOCATION(x) \
	pr_err("%s:%s:%d result=%d\n", __FILE__, __func__, __LINE__, x) \

#define CHECK_RESULT(x) \
	do { \
		if (unlikely(x  != 0)) \
			LOG_RESULT_LOCATION(x); \
		result = x; \
	} while (0)

struct motion_int_data {
	unsigned char pwr_mnt[2];
	unsigned char cfg;
	unsigned char accel_cfg;
	unsigned char gyro_cfg;
	unsigned char int_cfg;
	unsigned char smplrt_div;
	bool is_set;
};

struct mpu6050_input_data {
	struct i2c_client *client;
	struct input_dev *input;
	struct motion_int_data mot_data;
	struct mutex mutex;
	struct mpu6050_input_platform_data *pdata;
	struct device *accel_sensor_device;
	struct device *gyro_sensor_device;
#ifdef CONFIG_INPUT_MPU6050_POLLING
	struct delayed_work accel_work;
	struct delayed_work gyro_work;
#endif
	struct wake_lock reactive_wake_lock;
	atomic_t accel_enable;
	atomic_t accel_delay;
	atomic_t gyro_enable;
	atomic_t gyro_delay;
	atomic_t reactive_state;
	atomic_t reactive_enable;
	unsigned long motion_st_time;
	unsigned char gyro_pwr_mgnt[2];
	unsigned char int_pin_cfg;
	u16 enabled_sensors;
	u16 sleep_sensors;
	int current_delay;
	int gyro_bias[3];
	u8 mode;
	s16 acc_cal[3];
	bool factory_mode;
	struct delayed_work accel_off_work;
	bool accel_on_by_system;
};

static struct mpu6050_input_data *gb_mpu_data;

struct mpu6050_input_cfg {
	int dummy;
};
static int mpu6050_input_activate_devices(struct mpu6050_input_data *data,
					int sensors, bool enable);

int mpu6050_i2c_write(struct i2c_client *i2c_client,
			unsigned int len, unsigned char *data)
{
	struct i2c_msg msgs[1];
	int res;

	if (unlikely(NULL == data || NULL == i2c_client))
		return -EINVAL;

	msgs[0].addr = i2c_client->addr;
	msgs[0].flags = 0;	/* write */
	msgs[0].buf = (unsigned char *)data;
	msgs[0].len = len;

	res = i2c_transfer(i2c_client->adapter, msgs, 1);
	if (unlikely(res < 1))
		return res;
	else
		return 0;
}

int mpu6050_i2c_read(struct i2c_client *i2c_client,
			unsigned int len, unsigned char *data)
{
	struct i2c_msg msgs[2];
	int res;

	if (unlikely(NULL == data || NULL == i2c_client))
		return -EINVAL;

	msgs[0].addr = i2c_client->addr;
	msgs[0].flags = I2C_M_RD;
	msgs[0].buf = data;
	msgs[0].len = len;

	res = i2c_transfer(i2c_client->adapter, msgs, 1);
	if (unlikely(res < 1))
		return res;
	else
		return 0;
}

int mpu6050_i2c_write_single_reg(struct i2c_client *i2c_client,
					unsigned char reg, unsigned char value)
{

	unsigned char data[2];

	data[0] = reg;
	data[1] = value;

	return mpu6050_i2c_write(i2c_client, 2, data);
}

int mpu6050_i2c_read_reg(struct i2c_client *i2c_client,
			 unsigned char reg, unsigned int len,
			 unsigned char *data)
{
	struct i2c_msg msgs[2];
	int res;

	if (unlikely(NULL == data || NULL == i2c_client))
		return -EINVAL;

	msgs[0].addr = i2c_client->addr;
	msgs[0].flags = 0;	/* write */
	msgs[0].buf = &reg;
	msgs[0].len = 1;

	msgs[1].addr = i2c_client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].buf = data;
	msgs[1].len = len;

	res = i2c_transfer(i2c_client->adapter, msgs, 2);
	if (unlikely(res < 1))
		return res;
	else
		return 0;
}


int mpu6050_i2c_read_fifo(struct i2c_client *i2c_client,
				unsigned short length, unsigned char *data)
{
	int result;
	unsigned short bytes_read = 0;
	unsigned short this_len;

	while (bytes_read < length) {
		this_len = length - bytes_read;

		result =
			mpu6050_i2c_read_reg(i2c_client,
				MPUREG_FIFO_R_W, this_len, &data[bytes_read]);
		if (result)
			return result;
		bytes_read += this_len;
	}

	return 0;
}

static int mpu6050_input_set_mode(struct mpu6050_input_data *data, u8 mode)
{
	int err = 0;
	data->mode = mode;

	if (mode == MPU6050_MODE_SLEEP) {
		err = mpu6050_input_activate_devices(data,
				MPU6050_SENSOR_ACCEL |
					MPU6050_SENSOR_GYRO, false);
	}
	if (mode == MPU6050_MODE_NORMAL) {
		if (atomic_read(&data->accel_enable))
			err = mpu6050_input_activate_devices(data,
				MPU6050_SENSOR_ACCEL, true);
		if (atomic_read(&data->gyro_enable))
			err = mpu6050_input_activate_devices(data,
				MPU6050_SENSOR_GYRO, true);
	}
	return err;
}

static void mpu6050_input_report_accel_xyz(struct mpu6050_input_data *data)
{
	u8 regs[6];
	s16 raw[3], orien_raw[3];
	int i = 2;
	int result;

	result =
		mpu6050_i2c_read_reg(data->client,
			MPUREG_ACCEL_XOUT_H, 6, regs);

	raw[0] = ((s16) ((s16) regs[0] << 8)) | regs[1];
	raw[1] = ((s16) ((s16) regs[2] << 8)) | regs[3];
	raw[2] = ((s16) ((s16) regs[4] << 8)) | regs[5];

	do {
		orien_raw[i] = data->pdata->orientation[i*3] * raw[0]
			+ data->pdata->orientation[i*3+1] * raw[1]
			+ data->pdata->orientation[i*3+2] * raw[2];
	} while (i-- != 0);

	pr_debug("%s : x = %d, y = %d, z = %d\n", __func__,
		orien_raw[0], orien_raw[1], orien_raw[2]);

	input_report_rel(data->input, REL_X, orien_raw[0] - data->acc_cal[0]);
	input_report_rel(data->input, REL_Y, orien_raw[1] - data->acc_cal[1]);
	input_report_rel(data->input, REL_Z, orien_raw[2] - data->acc_cal[2]);
	input_sync(data->input);
}

static void mpu6050_input_report_gyro_xyz(struct mpu6050_input_data *data)
{
	u8 regs[6];
	s16 raw[3], orien_raw[3];
	int i = 2;
	int result;

	result =
		mpu6050_i2c_read_reg(data->client, MPUREG_GYRO_XOUT_H, 6, regs);

	raw[0] = (((s16) ((s16) regs[0] << 8)) | regs[1])
				- (s16)data->gyro_bias[0];
	raw[1] = (((s16) ((s16) regs[2] << 8)) | regs[3])
				- (s16)data->gyro_bias[1];
	raw[2] = (((s16) ((s16) regs[4] << 8)) | regs[5])
				- (s16)data->gyro_bias[2];

	do {
		orien_raw[i] = data->pdata->orientation[i*3] * raw[0]
			+ data->pdata->orientation[i*3+1] * raw[1]
			+ data->pdata->orientation[i*3+2] * raw[2];
	} while (i-- != 0);

	input_report_rel(data->input, REL_RX, orien_raw[0]);
	input_report_rel(data->input, REL_RY, orien_raw[1]);
	input_report_rel(data->input, REL_RZ, orien_raw[2]);
	input_sync(data->input);
}

static irqreturn_t mpu6050_input_irq_thread(int irq, void *dev)
{
	struct mpu6050_input_data *data = (struct mpu6050_input_data *)dev;
	struct motion_int_data *mot_data = &data->mot_data;
	unsigned char reg;
	unsigned long timediff = 0;
	int result;

	if (!atomic_read(&data->reactive_enable)) {
#ifndef CONFIG_INPUT_MPU6050_POLLING
		if (data->enabled_sensors & MPU6050_SENSOR_ACCEL)
			mpu6050_input_report_accel_xyz(data);

		if (data->enabled_sensors & MPU6050_SENSOR_GYRO)
			mpu6050_input_report_gyro_xyz(data);
#endif
	} else {
		result =
			mpu6050_i2c_read_reg(data->client,
			MPUREG_INT_STATUS, 1, &reg);
		if (result) {
			pr_err("%s: i2c_read err= %d\n", __func__, result);
			goto done;
		}
		/* skip erronous interrupt in 100ms */
		timediff = jiffies_to_msecs(jiffies - (data->motion_st_time));
		if (timediff < 150 && !(data->factory_mode)) {
			pr_debug("%s: timediff = %ld msec\n",
				__func__, timediff);
			goto done;
		}
		if (reg & (1 << 6) || data->factory_mode) {
			/*handle motion recognition*/
			atomic_set(&data->reactive_state, true);
			gb_mpu_data->factory_mode = false;
			pr_info("%s: motion interrupt happened\n",
				__func__);
			/* disable motion int */
			mpu6050_i2c_write_single_reg(data->client,
				MPUREG_INT_ENABLE, mot_data->int_cfg);
			wake_lock_timeout(&data->reactive_wake_lock,
				msecs_to_jiffies(2000));
		}
	}
done:
	return IRQ_HANDLED;
}

static int mpu6050_input_set_fsr(struct mpu6050_input_data *data, int fsr)
{
	unsigned char fsr_mask;
	int result;
	unsigned char reg;

	if (fsr <= 2000) {
		fsr_mask = 0x00;
	} else if (fsr <= 4000) {
		fsr_mask = 0x08;
	} else if (fsr <= 8000) {
		fsr_mask = 0x10;
	} else {		/* fsr = [8001, oo) */
		fsr_mask = 0x18;
	}

	result =
		mpu6050_i2c_read_reg(data->client,
			MPUREG_ACCEL_CONFIG, 1, &reg);
	if (result) {
		LOG_RESULT_LOCATION(result);
		return result;
	}
	result =
		mpu6050_i2c_write_single_reg(data->client, MPUREG_ACCEL_CONFIG,
					 reg | fsr_mask);
	if (result) {
		LOG_RESULT_LOCATION(result);
		return result;
	}

	return result;
}

static int mpu6050_input_set_fp_mode(struct mpu6050_input_data *data)
{
	unsigned char b;

	/* Resetting the cycle bit and LPA wake up freq */
	mpu6050_i2c_read_reg(data->client, MPUREG_PWR_MGMT_1, 1, &b);
	b &= ~BIT_CYCLE & ~BIT_PD_PTAT;
	mpu6050_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_1, b);
	mpu6050_i2c_read_reg(data->client, MPUREG_PWR_MGMT_2, 1, &b);
	b &= ~BITS_LPA_WAKE_CTRL;
	mpu6050_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_2, b);
	/* Resetting the duration setting for fp mode */
	b = (unsigned char)10 / ACCEL_MOT_DUR_LSB;
	mpu6050_i2c_write_single_reg(data->client, MPUREG_ACCEL_MOT_DUR, b);

	return 0;
}

static int mpu6050_input_set_odr(struct mpu6050_input_data *data, int odr)
{
	int result;
	unsigned char b;
	struct motion_int_data *mot_data = &data->mot_data;

	if (mot_data->is_set) {
		result = 0;
		goto done;
	}
	b = (unsigned char)(odr);

	CHECK_RESULT(mpu6050_i2c_write_single_reg
		(data->client, MPUREG_SMPLRT_DIV, b));

	mpu6050_i2c_read_reg(data->client, MPUREG_PWR_MGMT_1, 1, &b);
	b &= BIT_CYCLE;
	if (b == BIT_CYCLE) {
		pr_info("Accel LP - > FP mode.\n ");
		mpu6050_input_set_fp_mode(data);
	}
done:
	return result;
}

static int mpu6050_input_set_motion_interrupt(struct mpu6050_input_data *data,
				bool enable, bool factory_test)
{
	struct motion_int_data *mot_data = &data->mot_data;
	unsigned char reg;

	atomic_set(&data->reactive_state, false);

	if (enable) {
		/* 1) initialize */
		mpu6050_i2c_read_reg(data->client, MPUREG_INT_STATUS, 1, &reg);
		pr_info("%s: Initialize motion interrupt : INT_STATUS=%x\n",
			__func__, reg);

		reg = 0x01;
		mpu6050_i2c_write_single_reg(data->client,
				MPUREG_PWR_MGMT_1, reg);
		msleep(50);

		/* 2. mpu& accel config */
		if (factory_test)
			reg = 0x0; /*260Hz LPF */
		else
			reg = 0x1; /*44Hz LPF */
		mpu6050_i2c_write_single_reg(data->client, MPUREG_CONFIG, reg);
		reg = 0x0; /* Clear Accel Config. */
		mpu6050_i2c_write_single_reg(data->client,
				MPUREG_ACCEL_CONFIG, reg);

		/* 3. set motion thr & dur */
		if (factory_test)
			reg = 0x41;	/* Make the motion & drdy enable */
		else
			reg = 0x40;	/* Make the motion interrupt enable */
		mpu6050_i2c_write_single_reg(data->client,
				MPUREG_INT_ENABLE, reg);
		reg = 0x02;	/* Motion Duration =1 ms */
		mpu6050_i2c_write_single_reg(data->client,
				MPUREG_ACCEL_MOT_DUR, reg);
		/* Motion Threshold =1mg, based on the data sheet. */
		if (factory_test)
			reg = 0x00;
		else
			reg = 0x03;
		mpu6050_i2c_write_single_reg(data->client,
				MPUREG_ACCEL_MOT_THR, reg);

		if (!factory_test) {
			/* 5. */
			/* Steps to setup the lp mode for PWM-2 register */
			reg = mot_data->pwr_mnt[1];
			reg |= (BITS_LPA_WAKE_20HZ); /* the freq of wakeup */
			reg |= 0x07; /* put gyro in standby. */
			reg &= ~(BIT_STBY_XA | BIT_STBY_YA | BIT_STBY_ZA);
			mpu6050_i2c_write_single_reg(data->client,
				MPUREG_PWR_MGMT_2, reg);

			reg = 0x1;
			reg |= 0x20; /* Set the cycle bit to be 1. LP MODE */
			reg &= ~0x08; /* Clear the temp disp bit. */
			mpu6050_i2c_write_single_reg(data->client,
				MPUREG_PWR_MGMT_1,
				reg & ~BIT_SLEEP);
		}
		data->motion_st_time = jiffies;
	} else {
		if (mot_data->is_set) {
			mpu6050_i2c_write_single_reg(data->client,
				MPUREG_PWR_MGMT_1,
				mot_data->pwr_mnt[0]);
			msleep(50);
			mpu6050_i2c_write_single_reg(data->client,
				MPUREG_PWR_MGMT_2,
				mot_data->pwr_mnt[1]);
			mpu6050_i2c_write_single_reg(data->client,
				MPUREG_CONFIG,
				mot_data->cfg);
			mpu6050_i2c_write_single_reg(data->client,
				MPUREG_ACCEL_CONFIG,
				mot_data->accel_cfg);
			mpu6050_i2c_write_single_reg(data->client,
				MPUREG_GYRO_CONFIG,
				mot_data->gyro_cfg);
			mpu6050_i2c_write_single_reg(data->client,
				MPUREG_INT_ENABLE,
				mot_data->int_cfg);
			reg = 0xff; /* Motion Duration =1 ms */
			mpu6050_i2c_write_single_reg(data->client,
				MPUREG_ACCEL_MOT_DUR, reg);
			/* Motion Threshold =1mg, based on the data sheet. */
			reg = 0xff;
			mpu6050_i2c_write_single_reg(data->client,
				MPUREG_ACCEL_MOT_THR, reg);
			mpu6050_i2c_read_reg(data->client,
				MPUREG_INT_STATUS, 1, &reg);
			mpu6050_i2c_write_single_reg(data->client,
				MPUREG_SMPLRT_DIV,
				mot_data->smplrt_div);
			pr_info("%s: disable interrupt\n", __func__);
		}
	}
	mot_data->is_set = enable;
	return 0;
}

static int mpu6050_input_set_irq(struct mpu6050_input_data *data, int irq)
{
	int result;

	CHECK_RESULT(mpu6050_i2c_write_single_reg
		(data->client, MPUREG_INT_ENABLE, irq));

	return result;
}

static int mpu6050_input_suspend_accel(struct mpu6050_input_data *data)
{
	unsigned char reg;
	int result;

	CHECK_RESULT(mpu6050_i2c_read_reg
		(data->client, MPUREG_PWR_MGMT_2, 1, &reg));

	reg |= (BIT_STBY_XA | BIT_STBY_YA | BIT_STBY_ZA);
	CHECK_RESULT(mpu6050_i2c_write_single_reg
		(data->client, MPUREG_PWR_MGMT_2, reg));

	return result;
}

static int mpu6050_input_resume_accel(struct mpu6050_input_data *data)
{
	int result;
	unsigned char reg;

	CHECK_RESULT(mpu6050_i2c_read_reg
		(data->client, MPUREG_PWR_MGMT_1, 1, &reg));

	if (reg & BIT_SLEEP) {
		CHECK_RESULT(mpu6050_i2c_write_single_reg(data->client,
				MPUREG_PWR_MGMT_1,
				reg & ~BIT_SLEEP));
	}

	usleep_range(2000, 2100);

	CHECK_RESULT(mpu6050_i2c_read_reg
		(data->client, MPUREG_PWR_MGMT_2, 1, &reg));

	reg &= ~(BIT_STBY_XA | BIT_STBY_YA | BIT_STBY_ZA);
	CHECK_RESULT(mpu6050_i2c_write_single_reg
		(data->client, MPUREG_PWR_MGMT_2, reg));

	/* settings */

	/*----- LPF configuration  : 44hz ---->*/
	reg = MPU_FILTER_20HZ;
	CHECK_RESULT(mpu6050_i2c_write_single_reg
		(data->client, MPUREG_CONFIG, reg));
	/*<----- LPF configuration  : 44hz ---- */

	CHECK_RESULT(mpu6050_i2c_read_reg
		(data->client, MPUREG_ACCEL_CONFIG, 1, &reg));
	CHECK_RESULT(mpu6050_i2c_write_single_reg
		(data->client, MPUREG_ACCEL_CONFIG, reg | 0x0));
	CHECK_RESULT(mpu6050_input_set_fsr(data, 2000));

	return result;
}

static int mpu6050_input_actiave_accel(struct mpu6050_input_data *data,
				bool enable)
{
	int result;

	if (enable) {
		result = mpu6050_input_resume_accel(data);
		if (result) {
			LOG_RESULT_LOCATION(result);
			return result;
		} else {
			data->enabled_sensors |= MPU6050_SENSOR_ACCEL;
		}
	} else {
		result = mpu6050_input_suspend_accel(data);
		if (result == 0)
			data->enabled_sensors &= ~MPU6050_SENSOR_ACCEL;
	}

	return result;
}

static int mpu6050_input_suspend_gyro(struct mpu6050_input_data *data)
{
	int result;

	CHECK_RESULT(mpu6050_i2c_read_reg
		(data->client, MPUREG_PWR_MGMT_1, 2, data->gyro_pwr_mgnt));

	data->gyro_pwr_mgnt[1] |=
		(BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG);

	CHECK_RESULT(mpu6050_i2c_write_single_reg
		(data->client, MPUREG_PWR_MGMT_2, data->gyro_pwr_mgnt[1]));

	return result;
}

static int mpu6050_input_resume_gyro(struct mpu6050_input_data *data)
{
	int result;
	unsigned regs[2] = { 0, };

	CHECK_RESULT(mpu6050_i2c_read_reg
		(data->client, MPUREG_PWR_MGMT_1, 2, data->gyro_pwr_mgnt));

	CHECK_RESULT(mpu6050_i2c_write_single_reg
		(data->client, MPUREG_PWR_MGMT_1,
			data->gyro_pwr_mgnt[0] & ~BIT_SLEEP));

	data->gyro_pwr_mgnt[1] &= ~(BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG);

	CHECK_RESULT(mpu6050_i2c_write_single_reg
		(data->client, MPUREG_PWR_MGMT_2, data->gyro_pwr_mgnt[1]));

	regs[0] = 3 << 3;	/* 2000dps */
	CHECK_RESULT(mpu6050_i2c_write_single_reg
		(data->client, MPUREG_GYRO_CONFIG, regs[0]));

	regs[0] = MPU_FILTER_20HZ | 0x18;
	CHECK_RESULT(mpu6050_i2c_write_single_reg
		(data->client, MPUREG_CONFIG, regs[0]));

	return result;
}

static int mpu6050_input_activate_gyro(struct mpu6050_input_data *data,
				bool enable)
{
	int result;
	if (enable) {
		result = mpu6050_input_resume_gyro(data);
		if (result) {
			LOG_RESULT_LOCATION(result);
			return result;
		} else {
			data->enabled_sensors |= MPU6050_SENSOR_GYRO;
		}
	} else {
		result = mpu6050_input_suspend_gyro(data);
		if (result == 0)
			data->enabled_sensors &= ~MPU6050_SENSOR_GYRO;
	}

	return result;
}

static int mpu6050_set_delay(struct mpu6050_input_data *data)
{
	int result = 0;
	int delay = 200;

	if (data->enabled_sensors & MPU6050_SENSOR_ACCEL)
		delay = MIN(delay, atomic_read(&data->accel_delay));

	if (data->enabled_sensors & MPU6050_SENSOR_GYRO)
		delay = MIN(delay, atomic_read(&data->gyro_delay));

	data->current_delay = delay;

	if (data->enabled_sensors & MPU6050_SENSOR_ACCEL ||
		data->enabled_sensors & MPU6050_SENSOR_GYRO) {
		CHECK_RESULT(mpu6050_input_set_odr(data,
			data->current_delay));
#ifndef CONFIG_INPUT_MPU6050_POLLING
		if (!atomic_read(&data->reactive_enable))
			CHECK_RESULT(mpu6050_input_set_irq(data,
				BIT_RAW_RDY_EN));
#endif
	}
	return result;
}

static int mpu6050_input_activate_devices(struct mpu6050_input_data *data,
					int sensors, bool enable)
{
	int result;
	unsigned char reg;

	if (sensors & MPU6050_SENSOR_ACCEL)
		CHECK_RESULT(mpu6050_input_actiave_accel(data, enable));

	if (sensors & MPU6050_SENSOR_GYRO)
		CHECK_RESULT(mpu6050_input_activate_gyro(data, enable));

	if (data->enabled_sensors) {
		CHECK_RESULT(mpu6050_set_delay(data));
	} else {
		CHECK_RESULT(mpu6050_input_set_irq(data, 0x0));

		CHECK_RESULT(mpu6050_i2c_read_reg
			(data->client, MPUREG_PWR_MGMT_1, 1, &reg));
		if (!(reg & BIT_SLEEP)) {
			CHECK_RESULT(mpu6050_i2c_write_single_reg(data->client,
				MPUREG_PWR_MGMT_1,
				reg |
				BIT_SLEEP));
		}
	}

	return result;
}

static int mpu6050_input_initialize(struct mpu6050_input_data *data,
					const struct mpu6050_input_cfg
					*cfg)
{
	int result;
	data->int_pin_cfg = 0x10;
	data->current_delay = -1;
	data->enabled_sensors = 0;

	CHECK_RESULT(mpu6050_i2c_write_single_reg
		(data->client, MPUREG_PWR_MGMT_1, BIT_H_RESET));
	msleep(30);

	return mpu6050_input_set_mode(data, MPU6050_MODE_SLEEP);
}

static void accel_turn_off_work(struct work_struct *work)
{
	if (!(gb_mpu_data->enabled_sensors & MPU6050_SENSOR_ACCEL)) {
		mpu6050_input_suspend_accel(gb_mpu_data);
		gb_mpu_data->accel_on_by_system = false;
		pr_info("%s\n", __func__);
	}
}

static int read_accel_raw_xyz(s16 *x, s16 *y, s16 *z)
{
	u8 regs[6];
	s16 raw[3], orien_raw[3];
	int i = 2;
	int result;

	result =
		mpu6050_i2c_read_reg(gb_mpu_data->client,
			MPUREG_ACCEL_XOUT_H, 6, regs);

	raw[0] = ((s16) ((s16) regs[0] << 8)) | regs[1];
	raw[1] = ((s16) ((s16) regs[2] << 8)) | regs[3];
	raw[2] = ((s16) ((s16) regs[4] << 8)) | regs[5];

	do {
		orien_raw[i] = gb_mpu_data->pdata->orientation[i*3] * raw[0]
			+ gb_mpu_data->pdata->orientation[i*3+1] * raw[1]
			+ gb_mpu_data->pdata->orientation[i*3+2] * raw[2];
	} while (i-- != 0);

	pr_debug("%s : x = %d, y = %d, z = %d\n", __func__,
		orien_raw[0], orien_raw[1], orien_raw[2]);

	*x = orien_raw[0];
	*y = orien_raw[1];
	*z = orien_raw[2];

	return 0;
}

static int read_accel_raw_xyz_cal(s16 *x, s16 *y, s16 *z)
{
	s16 raw_x;
	s16 raw_y;
	s16 raw_z;

	if (!(gb_mpu_data->enabled_sensors & MPU6050_SENSOR_ACCEL)) {
		if (!gb_mpu_data->accel_on_by_system) {
			usleep_range(10000, 11000);
			mpu6050_input_resume_accel(gb_mpu_data);
			msleep(150);
			gb_mpu_data->accel_on_by_system = true;
		} else {
			cancel_delayed_work_sync(&gb_mpu_data->accel_off_work);
		}
		schedule_delayed_work(&gb_mpu_data->accel_off_work,
			msecs_to_jiffies(5000));
	}
	read_accel_raw_xyz(&raw_x, &raw_y, &raw_z);
	*x = raw_x - gb_mpu_data->acc_cal[0];
	*y = raw_y - gb_mpu_data->acc_cal[1];
	*z = raw_z - gb_mpu_data->acc_cal[2];
	return 0;
}

static int accel_open_calibration(void)
{
	struct file *cal_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(gb_mpu_data->pdata->acc_cal_path,
		O_RDONLY, S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->read(cal_filp,
		(char *)&gb_mpu_data->acc_cal,
			3 * sizeof(s16), &cal_filp->f_pos);
	if (err != 3 * sizeof(s16)) {
		pr_err("%s: Can't read the cal data from file\n", __func__);
		err = -EIO;
	}

	pr_info("%s: (%d,%d,%d)\n", __func__,
		gb_mpu_data->acc_cal[0], gb_mpu_data->acc_cal[1],
			gb_mpu_data->acc_cal[2]);

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}

static int accel_do_calibrate(int enable)
{
	struct file *cal_filp;
	int sum[3] = { 0, };
	int err;
	int i;
	s16 x;
	s16 y;
	s16 z;
	mm_segment_t old_fs;

	if (!(gb_mpu_data->enabled_sensors & MPU6050_SENSOR_ACCEL)) {
		mpu6050_input_resume_accel(gb_mpu_data);
		usleep_range(10000, 11000);
	}

	for (i = 0; i < ACC_CAL_TIME; i++) {
		err = read_accel_raw_xyz(&x, &y, &z);
		if (err < 0) {
			pr_err("%s: accel_read_accel_raw_xyz() "
				"failed in the %dth loop\n", __func__, i);
			goto done;
		}
		usleep_range(10000, 11000);
		sum[0] += x/ACC_CAL_DIV;
		sum[1] += y/ACC_CAL_DIV;
		sum[2] += z/ACC_CAL_DIV;
	}

	if (!(gb_mpu_data->enabled_sensors & MPU6050_SENSOR_ACCEL))
		mpu6050_input_suspend_accel(gb_mpu_data);

	if (enable) {
		gb_mpu_data->acc_cal[0] =
			(sum[0] / ACC_CAL_TIME) * ACC_CAL_DIV;
		gb_mpu_data->acc_cal[1] =
			(sum[1] / ACC_CAL_TIME) * ACC_CAL_DIV;
		gb_mpu_data->acc_cal[2] =
			((sum[2] / ACC_CAL_TIME) - ACC_IDEAL) * ACC_CAL_DIV;
	} else {
		gb_mpu_data->acc_cal[0] = 0;
		gb_mpu_data->acc_cal[1] = 0;
		gb_mpu_data->acc_cal[2] = 0;
	}

	pr_info("%s: cal data (%d,%d,%d)\n", __func__,
			gb_mpu_data->acc_cal[0], gb_mpu_data->acc_cal[1],
				gb_mpu_data->acc_cal[2]);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(gb_mpu_data->pdata->acc_cal_path,
			O_CREAT | O_TRUNC | O_WRONLY,
			S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&gb_mpu_data->acc_cal, 3 * sizeof(s16),
			&cal_filp->f_pos);
	if (err != 3 * sizeof(s16)) {
		pr_err("%s: Can't write the cal data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}

static ssize_t mpu6050_input_accel_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6050_input_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->accel_enable));

}

static ssize_t mpu6050_input_accel_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6050_input_data *data = input_get_drvdata(input_data);
	struct motion_int_data *mot_data = &data->mot_data;
	unsigned long value = 0;

	if (strict_strtoul(buf, 10, &value))
		return -EINVAL;

	accel_open_calibration();

	pr_info("%s : enable = %ld\n", __func__, value);

	mutex_lock(&data->mutex);
#ifdef CONFIG_INPUT_MPU6050_POLLING
	if (value && !atomic_read(&data->accel_enable)) {
		if (mot_data->is_set)
			mpu6050_input_set_motion_interrupt(
				data, false, data->factory_mode);
		mpu6050_input_activate_devices(data,
			MPU6050_SENSOR_ACCEL, true);
		schedule_delayed_work(&data->accel_work,
			msecs_to_jiffies(5));
	}
	if (!value && atomic_read(&data->accel_enable)) {
		cancel_delayed_work_sync(&data->accel_work);
		mpu6050_input_activate_devices(data,
			MPU6050_SENSOR_ACCEL, false);
		if (atomic_read(&data->reactive_enable))
			mpu6050_input_set_motion_interrupt(
				data, true, data->factory_mode);
	}
#endif
	atomic_set(&data->accel_enable, value);
	mutex_unlock(&data->mutex);
	return count;
}

static ssize_t mpu6050_input_accel_delay_show(struct device *dev,
				struct device_attribute *attr,
					char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6050_input_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->accel_delay));

}

static ssize_t mpu6050_input_accel_delay_store(struct device *dev,
				struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6050_input_data *data = input_get_drvdata(input_data);

	unsigned long value = 0;

	if (strict_strtoul(buf, 10, &value))
		return -EINVAL;

	pr_info("%s : delay = %ld\n", __func__, value);

	mutex_lock(&data->mutex);

	atomic_set(&data->accel_delay, value/1000);

	mpu6050_set_delay(data);

	mutex_unlock(&data->mutex);
	return count;
}

static int gyro_open_calibration(void)
{
	struct file *cal_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(gb_mpu_data->pdata->gyro_cal_path,
		O_RDONLY, S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->read(cal_filp,
		(char *)&gb_mpu_data->gyro_bias, 3 * sizeof(int),
			&cal_filp->f_pos);
	if (err != 3 * sizeof(int)) {
		pr_err("%s: Can't read the cal data from file\n", __func__);
		err = -EIO;
	}

	pr_info("%s: (%d,%d,%d)\n", __func__,
		gb_mpu_data->gyro_bias[0], gb_mpu_data->gyro_bias[1],
			gb_mpu_data->gyro_bias[2]);

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}

static int gyro_do_calibrate(void)
{
	struct file *cal_filp;
	int err;
	mm_segment_t old_fs;

	pr_info("%s: cal data (%d,%d,%d)\n", __func__,
			gb_mpu_data->gyro_bias[0], gb_mpu_data->gyro_bias[1],
				gb_mpu_data->gyro_bias[2]);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(gb_mpu_data->pdata->gyro_cal_path,
			O_CREAT | O_TRUNC | O_WRONLY,
			S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&gb_mpu_data->gyro_bias, 3 * sizeof(int),
			&cal_filp->f_pos);
	if (err != 3 * sizeof(int)) {
		pr_err("%s: Can't write the cal data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}

static ssize_t mpu6050_input_gyro_enable_show(struct device *dev,
					struct device_attribute *attr,
						char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6050_input_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->gyro_enable));

}

static ssize_t mpu6050_input_gyro_enable_store(struct device *dev,
					struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6050_input_data *data = input_get_drvdata(input_data);
	struct motion_int_data *mot_data = &data->mot_data;
	unsigned long value = 0;

	if (strict_strtoul(buf, 10, &value))
		return -EINVAL;

	gyro_open_calibration();

	pr_info("%s : enable = %ld\n", __func__, value);

	mutex_lock(&data->mutex);
#ifdef CONFIG_INPUT_MPU6050_POLLING
	if (value && !atomic_read(&data->gyro_enable)) {
		if (mot_data->is_set)
			mpu6050_input_set_motion_interrupt(
				data, false, data->factory_mode);
		mpu6050_input_activate_devices(data,
			MPU6050_SENSOR_GYRO, true);
		schedule_delayed_work(&data->gyro_work,
			msecs_to_jiffies(5));
	}
	if (!value && atomic_read(&data->gyro_enable)) {
		cancel_delayed_work_sync(&data->gyro_work);
		mpu6050_input_activate_devices(data,
			MPU6050_SENSOR_GYRO, false);
		if (atomic_read(&data->reactive_enable))
			mpu6050_input_set_motion_interrupt(
				data, true, data->factory_mode);
	}
#endif
	atomic_set(&data->gyro_enable, value);
	mutex_unlock(&data->mutex);
	return count;
}

static ssize_t mpu6050_input_gyro_delay_show(struct device *dev,
					struct device_attribute *attr,
						char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6050_input_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->gyro_delay));

}

static ssize_t mpu6050_input_gyro_delay_store(struct device *dev,
					struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6050_input_data *data = input_get_drvdata(input_data);

	unsigned long value = 0;

	if (strict_strtoul(buf, 10, &value))
		return -EINVAL;

	pr_info("%s : delay = %ld\n", __func__, value);

	mutex_lock(&data->mutex);
	atomic_set(&data->gyro_delay, value/1000);
	mpu6050_set_delay(data);
	mutex_unlock(&data->mutex);
	return count;
}

static ssize_t mpu6050_input_gyro_self_test_show(struct device *dev,
					struct device_attribute *attr,
						char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6050_input_data *data = input_get_drvdata(input_data);
	int scaled_gyro_bias[3] = {0};
	int scaled_gyro_rms[3] = {0};
	int packet_count[3] = {0};
	int result;

	mutex_lock(&data->mutex);

	result = mpu6050_selftest_run(data->client,
					packet_count,
					scaled_gyro_bias,
					scaled_gyro_rms,
					data->gyro_bias);
	if (!result) {
		gyro_do_calibrate();
	} else {
		data->gyro_bias[0] = 0;
		data->gyro_bias[1] = 0;
		data->gyro_bias[2] = 0;
		result = -1;
	}

	mutex_unlock(&data->mutex);

	return sprintf(buf, "%d "
		"%d %d %d "
		"%d.%03d %d.%03d %d.%03d "
		"%d.%03d %d.%03d %d.%03d\n",
		result,
		packet_count[0], packet_count[1], packet_count[2],
		(int)abs(scaled_gyro_bias[0]/1000),
		(int)abs(scaled_gyro_bias[0])%1000,
		(int)abs(scaled_gyro_bias[1]/1000),
		(int)abs(scaled_gyro_bias[1])%1000,
		(int)abs(scaled_gyro_bias[2]/1000),
		(int)abs(scaled_gyro_bias[2])%1000,
		scaled_gyro_rms[0]/1000,
		(int)abs(scaled_gyro_rms[0])%1000,
		scaled_gyro_rms[1]/1000,
		(int)abs(scaled_gyro_rms[1])%1000,
		scaled_gyro_rms[2]/1000,
		(int)abs(scaled_gyro_rms[2])%1000);
}



static DEVICE_ATTR(acc_enable, S_IRUGO | S_IWUSR | S_IWGRP,
		mpu6050_input_accel_enable_show,
			mpu6050_input_accel_enable_store);
static DEVICE_ATTR(acc_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		mpu6050_input_accel_delay_show,
			mpu6050_input_accel_delay_store);
static DEVICE_ATTR(gyro_enable, S_IRUGO | S_IWUSR | S_IWGRP,
		mpu6050_input_gyro_enable_show,
			mpu6050_input_gyro_enable_store);
static DEVICE_ATTR(gyro_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		mpu6050_input_gyro_delay_show,
			mpu6050_input_gyro_delay_store);
static DEVICE_ATTR(self_test, S_IRUGO | S_IWUSR | S_IWGRP,
		mpu6050_input_gyro_self_test_show, NULL);



static struct attribute *mpu6050_attributes[] = {
	&dev_attr_acc_enable.attr,
	&dev_attr_acc_delay.attr,
	&dev_attr_gyro_enable.attr,
	&dev_attr_gyro_delay.attr,
	&dev_attr_self_test.attr,
	NULL,
};

static struct attribute_group mpu6050_attribute_group = {
	.attrs = mpu6050_attributes
};

static ssize_t mpu6050_input_reactive_enable_show(struct device *dev,
					struct device_attribute
						*attr, char *buf)
{
	pr_debug("%s: state =%d\n", __func__,
		atomic_read(&gb_mpu_data->reactive_state));
	return sprintf(buf, "%d\n",
		atomic_read(&gb_mpu_data->reactive_state));
}

static ssize_t mpu6050_input_reactive_enable_store(struct device *dev,
					struct device_attribute
						*attr, const char *buf,
							size_t count)
{
	bool onoff = false;
	unsigned long value = 0;
	int err = count;
	struct motion_int_data *mot_data = &gb_mpu_data->mot_data;

	if (strict_strtoul(buf, 10, &value)) {
		err = -EINVAL;
		goto done;
	}
	switch (value) {
	case 0:
		break;
	case 1:
		onoff = true;
		break;
	case 2:
		onoff = true;
		gb_mpu_data->factory_mode = true;
		break;
	default:
		err = -EINVAL;
		pr_err("%s: invalid value %d\n", __func__, *buf);
		goto done;
	}

#ifdef CONFIG_INPUT_MPU6050_POLLING
	if (!value) {
		disable_irq_wake(gb_mpu_data->client->irq);
		disable_irq(gb_mpu_data->client->irq);
	} else {
		enable_irq(gb_mpu_data->client->irq);
		enable_irq_wake(gb_mpu_data->client->irq);
	}
#endif

	mutex_lock(&gb_mpu_data->mutex);
	if (onoff) {
		pr_info("enable from %s\n", __func__);
		atomic_set(&gb_mpu_data->reactive_enable, true);
		if (!mot_data->is_set) {
			mpu6050_i2c_read_reg(gb_mpu_data->client,
				MPUREG_PWR_MGMT_1, 2,
					mot_data->pwr_mnt);
			mpu6050_i2c_read_reg(gb_mpu_data->client,
				MPUREG_CONFIG, 1, &mot_data->cfg);
			mpu6050_i2c_read_reg(gb_mpu_data->client,
				MPUREG_ACCEL_CONFIG, 1,
					&mot_data->accel_cfg);
			mpu6050_i2c_read_reg(gb_mpu_data->client,
				MPUREG_GYRO_CONFIG, 1,
					&mot_data->gyro_cfg);
			mpu6050_i2c_read_reg(gb_mpu_data->client,
				MPUREG_INT_ENABLE, 1,
					&mot_data->int_cfg);
			mpu6050_i2c_read_reg(gb_mpu_data->client,
				MPUREG_SMPLRT_DIV, 1,
					&mot_data->smplrt_div);
		}
		mpu6050_input_set_motion_interrupt(gb_mpu_data,
			true, gb_mpu_data->factory_mode);
	} else {
		pr_info("%s disable\n", __func__);
		mpu6050_input_set_motion_interrupt(gb_mpu_data,
			false, gb_mpu_data->factory_mode);
		atomic_set(&gb_mpu_data->reactive_enable, false);
		if (gb_mpu_data->factory_mode)
			gb_mpu_data->factory_mode = false;
	}
	mutex_unlock(&gb_mpu_data->mutex);

	pr_info("%s: onoff = %d, state =%d\n", __func__,
		atomic_read(&gb_mpu_data->reactive_enable),
		atomic_read(&gb_mpu_data->reactive_state));
done:
	return err;
}
static struct device_attribute dev_attr_reactive_alert =
	__ATTR(reactive_alert, S_IRUGO | S_IWUSR | S_IWGRP,
		mpu6050_input_reactive_enable_show,
			mpu6050_input_reactive_enable_store);

static ssize_t mpu6050_power_on(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int count = 0;

	dev_dbg(dev, "this_client = %d\n", (int)gb_mpu_data->client);
	count = sprintf(buf, "%d\n",
		(gb_mpu_data->client != NULL ? 1 : 0));

	return count;
}
static struct device_attribute dev_attr_power_on =
	__ATTR(power_on, S_IRUSR | S_IRGRP, mpu6050_power_on,
		NULL);

static ssize_t mpu6050_get_temp(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int count = 0;
	short temperature = 0;
	unsigned char reg[2];
	int result;

	CHECK_RESULT(mpu6050_i2c_read_reg
		(gb_mpu_data->client, MPUREG_TEMP_OUT_H, 2, reg));

	temperature = (short) (((reg[0]) << 8) | reg[1]);
	temperature = (((temperature + 521) / 340) + 35);

	pr_info("read temperature = %d\n", temperature);

	count = sprintf(buf, "%d\n", temperature);

	return count;
}
static struct device_attribute dev_attr_temperature =
	__ATTR(temperature, S_IRUSR | S_IRGRP, mpu6050_get_temp,
		NULL);

static ssize_t acc_data_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	s16 x;
	s16 y;
	s16 z;
	read_accel_raw_xyz_cal(&x, &y, &z);
	pr_debug("%s : x = %d, y = %d, z = %d\n", __func__, x, y, z);
	return sprintf(buf, "%d, %d, %d\n", x, y, z);
}
static struct device_attribute dev_attr_raw_data =
	__ATTR(raw_data, S_IRUSR | S_IRGRP, acc_data_read,
		NULL);

static ssize_t accel_calibration_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int count = 0;
	s16 x;
	s16 y;
	s16 z;
	int err;

	x = gb_mpu_data->acc_cal[0];
	y = gb_mpu_data->acc_cal[1];
	z = gb_mpu_data->acc_cal[2];
	pr_info(" accel_calibration_show %d %d %d\n", x, y, z);

	if (!x && !y && !z)
		err = -1;
	else
		err = 1;

	count = sprintf(buf, "%d %d %d %d\n", err, x, y, z);

	return count;
}

static ssize_t accel_calibration_store(struct device *dev,
				struct device_attribute *attr,
					const char *buf, size_t size)
{
	int err;
	int count;
	unsigned long enable;
	s16 x;
	s16 y;
	s16 z;
	char tmp[64];

	if (strict_strtoul(buf, 10, &enable))
		return -EINVAL;
	err = accel_do_calibrate(enable);
	if (err < 0)
		pr_err("%s: accel_do_calibrate() failed\n", __func__);
	x = gb_mpu_data->acc_cal[0];
	y = gb_mpu_data->acc_cal[1];
	z = gb_mpu_data->acc_cal[2];

	pr_info("accel_calibration_store %d %d %d\n", x, y, z);
	if (err > 0)
		err = 0;
	count = sprintf(tmp, "%d\n", err);

	return count;
}
static struct device_attribute dev_attr_calibration =
	__ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
		accel_calibration_show, accel_calibration_store);

static ssize_t accel_vendor_show(struct device *dev,
				struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%s\n", "INVENSENSE");
}
static struct device_attribute dev_attr_accel_sensor_vendor =
	__ATTR(vendor, S_IRUSR | S_IRGRP, accel_vendor_show, NULL);
static struct device_attribute dev_attr_gyro_sensor_vendor =
	__ATTR(vendor, S_IRUSR | S_IRGRP, accel_vendor_show, NULL);

static ssize_t accel_name_show(struct device *dev,
				struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%s\n", "MPU6050");
}
static struct device_attribute dev_attr_accel_sensor_name =
	__ATTR(name, S_IRUSR | S_IRGRP, accel_name_show, NULL);
static struct device_attribute dev_attr_gyro_sensor_name =
	__ATTR(name, S_IRUSR | S_IRGRP, accel_name_show, NULL);

static struct device_attribute *gyro_sensor_attrs[] = {
	&dev_attr_power_on,
	&dev_attr_temperature,
	&dev_attr_gyro_sensor_vendor,
	&dev_attr_gyro_sensor_name,
	NULL,
};

static struct device_attribute *accel_sensor_attrs[] = {
	&dev_attr_raw_data,
	&dev_attr_calibration,
	&dev_attr_reactive_alert,
	&dev_attr_accel_sensor_vendor,
	&dev_attr_accel_sensor_name,
	NULL,
};

static int mpu6050_input_register_input_device(struct
				mpu6050_input_data
					*data)
{
	struct input_dev *idev;
	int error = 0;

	idev = input_allocate_device();
	if (!idev) {
		error = -ENOMEM;
		goto done;
	}

	idev->name = "mpu6050_input";
	idev->id.bustype = BUS_I2C;

	set_bit(EV_REL, idev->evbit);
	input_set_capability(idev, EV_REL, REL_X);
	input_set_capability(idev, EV_REL, REL_Y);
	input_set_capability(idev, EV_REL, REL_Z);

	input_set_capability(idev, EV_REL, REL_RX);
	input_set_capability(idev, EV_REL, REL_RY);
	input_set_capability(idev, EV_REL, REL_RZ);

	input_set_drvdata(idev, data);

	error = input_register_device(idev);
	if (error < 0) {
		input_free_device(idev);
		goto done;
	}

	error = sysfs_create_group(&idev->dev.kobj, &mpu6050_attribute_group);
	if (error < 0) {
		input_free_device(idev);
		goto done;
	}

	mutex_init(&data->mutex);

	atomic_set(&data->accel_enable, 0);
	atomic_set(&data->accel_delay, 200);
	atomic_set(&data->gyro_enable, 0);
	atomic_set(&data->gyro_delay, 200);
	atomic_set(&data->reactive_state, 0);
	atomic_set(&data->reactive_enable, 0);
	data->input = idev;

	INIT_DELAYED_WORK(&data->accel_off_work, accel_turn_off_work);
	data->accel_on_by_system = false;
done:
	return error;
}

#ifdef CONFIG_INPUT_MPU6050_POLLING
static void mpu6050_work_func_acc(struct work_struct *work)
{
	struct mpu6050_input_data *data =
		container_of((struct delayed_work *)work,
			struct mpu6050_input_data, accel_work);

	mpu6050_input_report_accel_xyz(data);

	pr_debug("%s: enable=%d, delay=%d\n", __func__,
		atomic_read(&data->accel_enable),
			atomic_read(&data->accel_delay));

	if (atomic_read(&data->accel_delay) < 60) {
		usleep_range(atomic_read(&data->accel_delay) * 1000,
			atomic_read(&data->accel_delay) * 1100);
		schedule_delayed_work(&data->accel_work, 0);
	} else {
		schedule_delayed_work(&data->accel_work,
			msecs_to_jiffies(
			atomic_read(&data->accel_delay)));
	}
}

static void mpu6050_work_func_gyro(struct work_struct *work)
{
	struct mpu6050_input_data *data =
		container_of((struct delayed_work *)work,
			struct mpu6050_input_data, gyro_work);

	mpu6050_input_report_gyro_xyz(data);

	pr_debug("%s: enable=%d, delay=%d\n", __func__,
		atomic_read(&data->gyro_enable),
			atomic_read(&data->gyro_delay));
	if (atomic_read(&data->gyro_delay) < 60) {
		usleep_range(atomic_read(&data->gyro_delay) * 1000,
			atomic_read(&data->gyro_delay) * 1100);
		schedule_delayed_work(&data->gyro_work, 0);
	} else {
		schedule_delayed_work(&data->gyro_work,
			msecs_to_jiffies(
			atomic_read(&data->gyro_delay)));
	}
}
#endif

static int __devinit mpu6050_input_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{

	struct mpu6050_input_platform_data *mpu_pdata =
		client->dev.platform_data;
	const struct mpu6050_input_cfg *cfg;
	struct mpu6050_input_data *data;
	int error;
	unsigned char reg;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "i2c_check_functionality error\n");
		return -EIO;
	}

	data = kzalloc(sizeof(struct mpu6050_input_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;
	data->pdata = mpu_pdata;
	gb_mpu_data = data;

	if (mpu_pdata->power_on)
		mpu_pdata->power_on(true);

	error = mpu6050_i2c_read_reg(data->client,
		MPUREG_WHOAMI, 1, &reg);
	if (error < 0) {
		pr_err("%s failed : threre is no such device.\n", __func__);
		goto err_free_mem;
	}
	pr_info("%s: WHOAMI (0x68) = 0x%x\n", __func__, reg);

	error = sensors_register(data->accel_sensor_device,
		NULL, accel_sensor_attrs,
			"accelerometer_sensor");
	if (error) {
		pr_err("%s: cound not register accelerometer sensor device(%d).\n",
			__func__, error);
		goto acc_sensor_register_failed;
	}

	error = sensors_register(data->gyro_sensor_device,
		NULL, gyro_sensor_attrs,
			"gyro_sensor");
	if (error) {
		pr_err("%s: cound not register gyro sensor device(%d).\n",
			__func__, error);
		goto gyro_sensor_register_failed;
	}

#ifdef CONFIG_INPUT_MPU6050_POLLING
	INIT_DELAYED_WORK(&data->accel_work, mpu6050_work_func_acc);
	INIT_DELAYED_WORK(&data->gyro_work, mpu6050_work_func_gyro);
#endif

	wake_lock_init(&data->reactive_wake_lock, WAKE_LOCK_SUSPEND,
		"reactive_wake_lock");

	error = mpu6050_input_initialize(data, cfg);
	if (error)
		goto err_input_initialize;

	if (client->irq > 0) {
		error = mpu6050_input_register_input_device(data);
		if (error < 0)
			goto err_input_initialize;

		error = request_threaded_irq(client->irq,
				NULL, mpu6050_input_irq_thread,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				MPU6050_INPUT_DRIVER, data);
		if (error < 0) {
			dev_err(&client->dev,
				"irq request failed %d, error %d\n",
				client->irq, error);
			input_unregister_device(data->input);
			goto err_input_initialize;
		}
#ifdef CONFIG_INPUT_MPU6050_POLLING
		disable_irq(client->irq);
#endif
	}

	i2c_set_clientdata(client, data);

	pr_info("mpu6050_input_probe success\n");

	return 0;

err_input_initialize:
	wake_lock_destroy(&data->reactive_wake_lock);
gyro_sensor_register_failed:
acc_sensor_register_failed:
err_free_mem:
	if (mpu_pdata->power_on)
		mpu_pdata->power_on(false);
	kfree(data);
	return error;
}

static int mpu6050_input_remove(struct i2c_client *client)
{
	struct mpu6050_input_data *data = i2c_get_clientdata(client);

	if (client->irq > 0) {
		free_irq(client->irq, data);
		input_unregister_device(data->input);
	}
	wake_lock_destroy(&data->reactive_wake_lock);
	kfree(data);

	return 0;
}

static void mpu6050_input_shutdown(struct i2c_client *client)
{
	struct mpu6050_input_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_INPUT_MPU6050_POLLING
	if (atomic_read(&data->accel_enable))
		cancel_delayed_work_sync(&data->accel_work);
	if (atomic_read(&data->gyro_enable))
		cancel_delayed_work_sync(&data->gyro_work);
#endif
	if (data->pdata->power_on)
		data->pdata->power_on(false);
}

static int mpu6050_input_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mpu6050_input_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_INPUT_MPU6050_POLLING
	if (atomic_read(&data->accel_enable))
		cancel_delayed_work_sync(&data->accel_work);
	if (atomic_read(&data->gyro_enable))
		cancel_delayed_work_sync(&data->gyro_work);
#endif

	if (!atomic_read(&data->reactive_enable)) {
#ifndef CONFIG_INPUT_MPU6050_POLLING
		disable_irq_wake(client->irq);
		disable_irq(client->irq);
#endif
		if (atomic_read(&data->accel_enable) ||
			atomic_read(&data->gyro_enable))
			mpu6050_input_set_mode(data, MPU6050_MODE_SLEEP);
	}
	return 0;
}

static int mpu6050_input_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mpu6050_input_data *data = i2c_get_clientdata(client);

	if (!atomic_read(&data->reactive_enable)) {
#ifndef CONFIG_INPUT_MPU6050_POLLING
		enable_irq(client->irq);
		enable_irq_wake(client->irq);
#endif
		if (atomic_read(&data->accel_enable) ||
			atomic_read(&data->gyro_enable))
			mpu6050_input_set_mode(data, MPU6050_MODE_NORMAL);
	}
#ifdef CONFIG_INPUT_MPU6050_POLLING
		if (atomic_read(&data->accel_enable))
			schedule_delayed_work(&data->accel_work, 0);
		if (atomic_read(&data->gyro_enable))
			schedule_delayed_work(&data->gyro_work, 0);
#endif
	return 0;
}

static const struct i2c_device_id mpu6050_input_id[] = {
	{"mpu6050_input", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, mpu6050_input_id);

static const struct dev_pm_ops mpu6050_dev_pm_ops = {
	.suspend = mpu6050_input_suspend,
	.resume = mpu6050_input_resume,
};

static struct i2c_driver mpu6050_input_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = MPU6050_INPUT_DRIVER,
		.pm = &mpu6050_dev_pm_ops,
	},
	.class = I2C_CLASS_HWMON,
	.id_table = mpu6050_input_id,
	.probe = mpu6050_input_probe,
	.remove = mpu6050_input_remove,
	.shutdown = mpu6050_input_shutdown,
};

static int __init mpu6050_init(void)
{
	int result = i2c_add_driver(&mpu6050_input_driver);

	pr_info("mpu6050_init()\n");

	return result;
}

static void __exit mpu6050_exit(void)
{
	pr_info("mpu6050_exit()\n");

	i2c_del_driver(&mpu6050_input_driver);
}

MODULE_AUTHOR("Tae-Soo Kim <tskim@invensense.com>");
MODULE_DESCRIPTION("MPU6050 driver");
MODULE_LICENSE("GPL");

module_init(mpu6050_init);
module_exit(mpu6050_exit);
