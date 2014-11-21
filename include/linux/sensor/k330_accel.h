/*
 *	Copyright (C) 2011, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 */

#ifndef __K330_ACCEL_HEADER__
#define __K330_ACCEL_HEADER__

#include <linux/types.h>
#include <linux/ioctl.h>

/*k330_accel registers */
#define WHO_AM_I		0x0F
#define K330ACCEL_ENABLE_FLASH	0x01
#define K330ACCEL_CTRL_REG4	0x20	/* control reg4(ODR) */
#define K330ACCEL_CTRL_REG1	0x21	/* SM1 control reg */
#define K330ACCEL_CTRL_REG2	0x22	/* SM2 control reg */
#define K330ACCEL_CTRL_REG3	0x23	/* control reg3 */
#define K330ACCEL_CTRL_REG5	0x24	/* control reg5(full scale) */
#define K330ACCEL_CTRL_REG6	0x25	/* control reg6 */
#define STATUS_REG		0x27	/* Status register */
#define OUT_X_L			0x28
#define OUT_X_H			0x29
#define OUT_Y_L			0x2A
#define OUT_Y_H			0x2B
#define OUT_Z_L			0x2C
#define OUT_Z_H			0x2D
#define FIFO_CTRL_REG		0x2E
#define FIFO_SRC_REG		0x2F
#define THRS1_1			0x57
#define THRS1_2			0x77
#define MASKA_1			0x5A
#define MASKA_2			0x7A
#define SM1_SETT		0x5B
#define SM2_SETT		0x7B
#define INT1_SRC		0x5F
#define INT2_SRC		0x7F
#define SM1_S0			0x40
#define SM1_S1			0x41
#define K330_ACC_OFFSET_ADDR	0x50
#define SM2_S0			0x60
#define SM2_S1			0x61

#define PM_OFF			0x00
#define FLASH_ENALBE		0x80
#define LOW_PWR_MODE		0x5F /* 50HZ */
#define FASTEST_MODE		0x9F /* 1600Hz */
#define K330ACCEL_ALL_AXES	0x0F

/*
 * when opcode != 0xac or != 0xca will be use default values.
 */
#define K330_ACC_OPCODE_1	0xac
#define K330_ACC_OPCODE_2	0xca
#define K330_ACC_OFFSET_X	0	// mg
#define K330_ACC_OFFSET_Y	0	// mg
#define K330_ACC_OFFSET_Z	-20	// mg

#define ODR3_125		0x10	/* 3.125Hz output data rate */
#define ODR6_25			0x20	/* 6.25Hz output data rate */
#define ODR12_5			0x30	/* 12.5Hz output data rate */
#define ODR25			0x40	/* 25Hz output data rate */
#define ODR50			0x50	/* 50Hz output data rate */
#define ODR100			0x60	/* 100Hz output data rate */
#define ODR400			0x70	/* 400Hz output data rate */
#define ODR800			0x80	/* 800Hz output data rate */
#define ODR1600			0x90	/* 1600Hz output data rate */
#define K330ACCEL_ODR_MASK	0xF0

enum {
	K330_RES_LC_L = 0,
	K330_RES_LC_H,
	K330_RES_CTRL_REG1,
	K330_RES_CTRL_REG2,
	K330_RES_CTRL_REG3,
	K330_RES_CTRL_REG4,
	K330_RES_CTRL_REG5,
	K330_RESUME_ENTRIES,
};

#define K330_INT_ACT_H		(0x01 << 6)

/* Register Auto-increase */
#define K330ACCEL_AC		0x80

/* 1G */
#define ACCEL_1G	16384
#define MAX_ACCEL_2G	32767
#define MIN_ACCEL_2G	-32768

/* dev info */
#define ACC_DEV_NAME "accelerometer"

struct k330_acc {
	s16 x;
	s16 y;
	s16 z;
};

/* K330_ACCEL ioctl command label */
#define K330_ACCEL_IOCTL_BASE 'a'
#define K330_ACCEL_IOCTL_SET_DELAY   \
	_IOW(K330_ACCEL_IOCTL_BASE, 0, int64_t)
#define K330_ACCEL_IOCTL_GET_DELAY  \
	_IOR(K330_ACCEL_IOCTL_BASE, 1, int64_t)
#define K330_ACCEL_IOCTL_READ_XYZ\
	_IOR(K330_ACCEL_IOCTL_BASE, 8, struct k330_acc)
#define K330_ACCEL_IOCTL_SET_ENABLE   \
	_IOW(K330_ACCEL_IOCTL_BASE, 9, int)
#endif
