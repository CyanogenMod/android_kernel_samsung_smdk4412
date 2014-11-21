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

#ifndef _MPU6050_SELFTEST_H_
#define _MPU6050_SELFTEST_H_

#define VERBOSE_OUT 1

#define DEF_MPU_ADDR	(0x68)
#define DEF_GYRO_FULLSCALE	(2000)
#define DEF_GYRO_SENS	(32768 / DEF_GYRO_FULLSCALE)
#define DEF_PACKET_THRESH	(75)
#define DEF_TOTAL_TIMING_TOL	(3)
#define DEF_BIAS_THRESH_SELF	(20)
#define DEF_BIAS_THRESH_CAL	(20)

#define DEF_BIAS_LSB_THRESH_SELF	(DEF_BIAS_THRESH_SELF*DEF_GYRO_SENS)
#define DEF_BIAS_LSB_THRESH_CAL	(DEF_BIAS_THRESH_CAL*DEF_GYRO_SENS)

#define DEF_RMS_THRESH_SELF     (5)
#define DEF_RMS_LSB_THRESH_SELF (DEF_RMS_THRESH_SELF*DEF_GYRO_SENS)
#define DEF_TESTS_PER_AXIS       (1)
#define DEF_N_ACCEL_SAMPLES      (20)
#define ML_INIT_CAL_LEN          (36)
#define DEF_PERIOD_SELF          (75)
#define DEF_PERIOD_CAL           (600)
#define DEF_SCALE_FOR_FLOAT (1000)
#define DEF_RMS_SCALE_FOR_RMS (10000)
#define DEF_SQRT_SCALE_FOR_RMS (100)

int mpu6050_selftest_run(struct i2c_client *client, int packet_cnt[3],
			  int gyro_bias[3], int gyro_rms[3],
			  int gyro_lsb_bias[3]);

#endif /*_MPU6050_SELFTEST_H_*/
