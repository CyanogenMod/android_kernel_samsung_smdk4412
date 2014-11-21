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

#ifndef __K330_H__
#define __K330_H__

/* k330_gyro chip id */
#define DEVICE_ID	0xD4
/* k330_gyro gyroscope registers */
#define WHO_AM_I	0x0F
#define CTRL_REG1	0x20  /* power control reg */
#define CTRL_REG2	0x21  /* trigger & filter setting control reg */
#define CTRL_REG3	0x22  /* interrupt control reg */
#define CTRL_REG4	0x23  /* data control reg */
#define CTRL_REG5	0x24  /* fifo en & filter en control reg */
#define OUT_TEMP	0x26  /* Temperature data */
#define STATUS_REG	0x27
#define AXISDATA_REG	0x28
#define OUT_Y_L		0x2A
#define FIFO_CTRL_REG	0x2E
#define FIFO_SRC_REG	0x2F
#define PM_OFF		0x00
#define PM_NORMAL	0x08
#define ENABLE_ALL_AXES	0x07
#define BYPASS_MODE	0x00
#define FIFO_MODE	0x20
#define FIFO_EMPTY	0x20
#define AC		(1 << 7) /* register auto-increment bit */

/* odr settings */
#define FSS_MASK	0x1F
#define ODR_MASK	0xF0
#define ODR95_BW12_5	0x00  /* ODR = 95Hz; BW = 12.5Hz */
#define ODR95_BW25	0x11  /* ODR = 95Hz; BW = 25Hz   */
#define ODR190_BW12_5	0x40  /* ODR = 190Hz; BW = 12.5Hz */
#define ODR190_BW25	0x50  /* ODR = 190Hz; BW = 25Hz   */
#define ODR190_BW50	0x60  /* ODR = 190Hz; BW = 50Hz   */
#define ODR190_BW70	0x70  /* ODR = 190Hz; BW = 70Hz   */
#define ODR380_BW20	0x80  /* ODR = 380Hz; BW = 20Hz   */
#define ODR380_BW25	0x90  /* ODR = 380Hz; BW = 25Hz   */
#define ODR380_BW50	0xA0  /* ODR = 380Hz; BW = 50Hz   */
#define ODR380_BW100	0xB0  /* ODR = 380Hz; BW = 100Hz  */
#define ODR760_BW30	0xC0  /* ODR = 760Hz; BW = 30Hz   */
#define ODR760_BW35	0xD0  /* ODR = 760Hz; BW = 35Hz   */
#define ODR760_BW50	0xE0  /* ODR = 760Hz; BW = 50Hz   */
#define ODR760_BW100	0xF0  /* ODR = 760Hz; BW = 100Hz  */

/* full scale selection */
#define DPS250		250
#define DPS500		500
#define DPS2000		2000
#define FS_MASK		0x30
#define FS_250DPS	0x00
#define FS_500DPS	0x10
#define FS_2000DPS	0x20
#define DEFAULT_DPS	DPS500
#define FS_DEFULAT_DPS	FS_500DPS

#define K330_MAX_GYRO		32767
#define K330_MIN_GYRO		-32768

/* self tset settings */
#define MIN_ST		175
#define MAX_ST		875
#define FIFO_TEST_WTM	0x1F
#define MIN_ZERO_RATE	-3142
#define MAX_ZERO_RATE	3142 /* 55*1000/17.5 */

/* max and min entry */
#define MAX_ENTRY	20
#define MAX_DELAY	(MAX_ENTRY * 10000000LL) /* 200ms */

#define K330_GYRO_DEV_NAME	"k330_gyro"
#define K330_GYR_INPUT_NAME "gyro_sensor"

#endif /* __K330_H__ */
