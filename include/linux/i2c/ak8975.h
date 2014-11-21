/*
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */
#ifndef AKM8975_H
#define AKM8975_H

#include <linux/ioctl.h>

#define AKM8975_I2C_NAME "ak8975"

#define SENSOR_DATA_SIZE	8 /* Rx buffer size, i.e from ST1 to ST2 */
#define AKMIO			0xA1

/* IOCTLs for AKM library */
/* WRITE and READ sizes don't include data.  On WRITE, the first value is data
 * size plus one and the second value is the register address.  On READ
 * the first value is the data size and second value is the register
 * address and the data is written back into the buffer starting at
 * the second byte (the length is unchanged).
 */
#define ECS_IOCTL_WRITE			_IOW(AKMIO, 0x01, char*)
#define ECS_IOCTL_READ			_IOWR(AKMIO, 0x02, char*)
#define ECS_IOCTL_RESET			_IO(AKMIO, 0x03)
#define ECS_IOCTL_SET_MODE		_IOW(AKMIO, 0x04, short)
#define ECS_IOCTL_GETDATA		_IOR(AKMIO, 0x05, \
						char[SENSOR_DATA_SIZE])
#define ECS_IOCTL_SET_YPR		_IOW(AKMIO, 0x06, short[12])
#define ECS_IOCTL_GET_OPEN_STATUS	_IOR(AKMIO, 0x07, int)
#define ECS_IOCTL_GET_CLOSE_STATUS	_IOR(AKMIO, 0x08, int)
#define ECS_IOCTL_GET_DELAY		_IOR(AKMIO, 0x30, int64_t)
#define ECS_IOCTL_GET_PROJECT_NAME	_IOR(AKMIO, 0x0D, char[64])
#define ECS_IOCTL_GET_MATRIX		_IOR(AKMIO, 0x0E, short[4][3][3])

/* IOCTLs for APPs */
#define ECS_IOCTL_APP_SET_MODE		_IOW(AKMIO, 0x10, short)
#define ECS_IOCTL_APP_SET_MFLAG		_IOW(AKMIO, 0x11, short)
#define ECS_IOCTL_APP_GET_MFLAG		_IOR(AKMIO, 0x12, short)
#define ECS_IOCTL_APP_SET_AFLAG		_IOW(AKMIO, 0x13, short)
#define ECS_IOCTL_APP_GET_AFLAG		_IOR(AKMIO, 0x14, short)
#define ECS_IOCTL_APP_SET_TFLAG		_IOW(AKMIO, 0x15, short)
#define ECS_IOCTL_APP_GET_TFLAG		_IOR(AKMIO, 0x16, short)
#define ECS_IOCTL_APP_RESET_PEDOMETER	_IO(AKMIO, 0x17)
#define ECS_IOCTL_APP_SET_DELAY		_IOW(AKMIO, 0x18, int64_t)
#define ECS_IOCTL_APP_GET_DELAY		ECS_IOCTL_GET_DELAY

/* Set raw magnetic vector flag */
#define ECS_IOCTL_APP_SET_MVFLAG	_IOW(AKMIO, 0x19, short)

/* Get raw magnetic vector flag */
#define ECS_IOCTL_APP_GET_MVFLAG	_IOR(AKMIO, 0x1A, short)

#ifdef __KERNEL__
struct akm8975_platform_data {
	int gpio_data_ready_int;
};
#endif

#endif
