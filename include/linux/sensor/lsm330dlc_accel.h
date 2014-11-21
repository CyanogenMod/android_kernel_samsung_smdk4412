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

#ifndef __LSM330DLC_ACCEL_HEADER__
#define __LSM330DLC_ACCEL_HEADER__

#include <linux/types.h>
#include <linux/ioctl.h>

/*lsm330dlc_accel registers */
#define STATUS_AUX		0x07
#define OUT_1_L			0x08
#define OUT_1_H			0x09
#define OUT_2_L			0x0A
#define OUT_2_H			0x0B
#define OUT_3_L			0x0C
#define OUT_3_H			0x0D
#define INT_COUNTER		0x0E
#define WHO_AM_I		0x0F
#define TEMP_CFG_REG		0x1F
#define CTRL_REG1		0x20	/* power control reg */
#define CTRL_REG2		0x21	/* power control reg */
#define CTRL_REG3		0x22	/* power control reg */
#define CTRL_REG4		0x23	/* interrupt control reg */
#define CTRL_REG5		0x24	/* interrupt control reg */
#define CTRL_REG6		0x25
#define REFERENCE		0x26
#define STATUS_REG		0x27
#define OUT_X_L			0x28
#define OUT_X_H			0x29
#define OUT_Y_L			0x2A
#define OUT_Y_H			0x2B
#define OUT_Z_L			0x2C
#define OUT_Z_H			0x2D
#define FIFO_CTRL_REG		0x2E
#define FIFO_SRC_REG		0x2F
#define INT1_CFG		0x30
#define INT1_SRC		0x31
#define INT1_THS		0x32
#define INT1_DURATION		0x33
#define INT2_CFG		0x34
#define INT2_SRC		0x35
#define INT2_THS		0x36
#define INT2_DURATION		0x37
#define CLICK_CFG		0x38
#define CLICK_SRC		0x39
#define CLICK_THS		0x3A
#define TIME_LIMIT		0x3B
#define TIME_LATENCY		0x3C
#define TIME_WINDOW		0x3D

/* CTRL_REG1 */
#define CTRL_REG1_ODR3		(1 << 7)
#define CTRL_REG1_ODR2		(1 << 6)
#define CTRL_REG1_ODR1		(1 << 5)
#define CTRL_REG1_ODR0		(1 << 4)
#define CTRL_REG1_LPEN		(1 << 3)
#define CTRL_REG1_Zen		(1 << 2)
#define CTRL_REG1_Yen		(1 << 1)
#define CTRL_REG1_Xen		(1 << 0)

#define PM_OFF			0x00
#define LOW_PWR_MODE		0x4F /* 50HZ */
#define FASTEST_MODE		0x9F /* 1344Hz */
#define ENABLE_ALL_AXES		0x07

#define ODR1			0x10	/* 1Hz output data rate */
#define ODR10			0x20	/* 10Hz output data rate */
#define ODR25			0x30	/* 25Hz output data rate */
#define ODR50			0x40	/* 50Hz output data rate */
#define ODR100			0x50	/* 100Hz output data rate */
#define ODR200			0x60	/* 100Hz output data rate */
#define ODR400			0x70	/* 400Hz output data rate */
#define ODR1344			0x90	/* 1344Hz output data rate */
#define ODR_MASK		0xf0

/* CTRL_REG2 */
#define CTRL_REG2_HPM1		(1 << 7)
#define CTRL_REG2_HPM0		(1 << 6)
#define CTRL_REG2_HPCF2		(1 << 5)
#define CTRL_REG2_HPCF1		(1 << 4)
#define CTRL_REG2_FDS		(1 << 3)
#define CTRL_REG2_HPPCLICK	(1 << 2)
#define CTRL_REG2_HPIS2		(1 << 1)
#define CTRL_REG2_HPIS1		(1 << 0)

#define HPM_Normal		(CTRL_REG2_HPM1)
#define HPM_Filter		(CTRL_REG2_HPM0)

/* CTRL_REG3 */
#define I1_CLICK		(1 << 7)
#define I1_AOI1			(1 << 6)
#define I1_AOI2			(1 << 5)
#define I1_DRDY1		(1 << 4)
#define I1_DRDY2		(1 << 3)
#define I1_WTM			(1 << 2)
#define I1_OVERRUN		(1 << 1)

/* CTRL_REG4 */
#define CTRL_REG4_BLE		(1 << 6)
#define CTRL_REG4_FS1		(1 << 5)
#define CTRL_REG4_FS0		(1 << 4)
#define CTRL_REG4_HR		(1 << 3)
#define CTRL_REG4_ST1		(1 << 2)
#define CTRL_REG4_ST0		(1 << 1)
#define CTRL_REG4_SIM		(1 << 0)

#define FS2g			0x00
#define FS4g			(CTRL_REG4_FS0)
#define FS8g			(CTRL_REG4_FS1)
#define FS16g			(CTRL_REG4_FS1|CTRL_REG4_FS0)

/* CTRL_REG5 */
#define BOOT			(1 << 7)
#define FIFO_EN			(1 << 6)
#define LIR_INT1		(1 << 3)
#define D4D_INT1		(1 << 2)

/* STATUS_REG */
#define ZYXOR			(1 << 7)
#define ZOR			(1 << 6)
#define YOR			(1 << 5)
#define XOR			(1 << 4)
#define ZYXDA			(1 << 3)
#define ZDA			(1 << 2)
#define YDA			(1 << 1)
#define XDA			(1 << 0)

/* INT1_CFG */
#define INT_CFG_AOI		(1 << 7)
#define INT_CFG_6D		(1 << 6)
#define INT_CFG_ZHIE		(1 << 5)
#define INT_CFG_ZLIE		(1 << 4)
#define INT_CFG_YHIE		(1 << 3)
#define INT_CFG_YLIE		(1 << 2)
#define INT_CFG_XHIE		(1 << 1)
#define INT_CFG_XLIE		(1 << 0)

/* INT1_SRC */
#define IA			(1 << 6)
#define ZH			(1 << 5)
#define ZL			(1 << 4)
#define YH			(1 << 3)
#define YL			(1 << 2)
#define XH			(1 << 1)
#define XL			(1 << 0)

/* Register Auto-increase */
#define AC			(1 << 7)

/* dev info */
#define ACC_DEV_NAME "accelerometer"

struct lsm330dlc_acc {
	s16 x;
	s16 y;
	s16 z;
};

/* For movement recognition*/
#define USES_MOVEMENT_RECOGNITION

/* LSM330DLC_ACCEL ioctl command label */
#define LSM330DLC_ACCEL_IOCTL_BASE 'a'
#define LSM330DLC_ACCEL_IOCTL_SET_DELAY   \
	_IOW(LSM330DLC_ACCEL_IOCTL_BASE, 0, int64_t)
#define LSM330DLC_ACCEL_IOCTL_GET_DELAY  \
	_IOR(LSM330DLC_ACCEL_IOCTL_BASE, 1, int64_t)
#define LSM330DLC_ACCEL_IOCTL_READ_XYZ\
	_IOR(LSM330DLC_ACCEL_IOCTL_BASE, 8, struct lsm330dlc_acc)
#define LSM330DLC_ACCEL_IOCTL_SET_ENABLE   \
	_IOW(LSM330DLC_ACCEL_IOCTL_BASE, 9, int)
#endif
