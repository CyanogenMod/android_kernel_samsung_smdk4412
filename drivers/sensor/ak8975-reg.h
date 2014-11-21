/* linux/drivers/misc/ak8975-reg.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __AK8975_REG__
#define __AK8975_REG__

/* Compass device dependent definition */
#define AK8975_MODE_SNG_MEASURE		0x01
#define AK8975_MODE_SELF_TEST		0x08
#define AK8975_MODE_FUSE_ACCESS		0x0F
#define AK8975_MODE_POWER_DOWN		0x00

/* Rx buffer size. i.e ST,TMPS,H1X,H1Y,H1Z*/
#define SENSOR_DATA_SIZE		8

/* Read/Write buffer size.*/
#define RWBUF_SIZE			16

/* AK8975 register address */
#define AK8975_REG_WIA			0x00
#define AK8975_REG_INFO			0x01
#define AK8975_REG_ST1			0x02
#define AK8975_REG_HXL			0x03
#define AK8975_REG_HXH			0x04
#define AK8975_REG_HYL			0x05
#define AK8975_REG_HYH			0x06
#define AK8975_REG_HZL			0x07
#define AK8975_REG_HZH			0x08
#define AK8975_REG_ST2			0x09
#define AK8975_REG_CNTL			0x0A
#define AK8975_REG_RSV			0x0B
#define AK8975_REG_ASTC			0x0C
#define AK8975_REG_TS1			0x0D
#define AK8975_REG_TS2			0x0E
#define AK8975_REG_I2CDIS		0x0F

/* AK8975 fuse-rom address */
#define AK8975_FUSE_ASAX		0x10
#define AK8975_FUSE_ASAY		0x11
#define AK8975_FUSE_ASAZ		0x12

#endif /* __AK8975_REG__ */
