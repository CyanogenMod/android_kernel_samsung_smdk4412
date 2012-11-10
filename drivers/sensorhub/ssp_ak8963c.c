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
/* AKM Daemon Library ioctl						 */
/*************************************************************************/

static int akmd_copy_in(unsigned int cmd, void __user *argp,
			void *buf, size_t buf_size)
{
	if (!(cmd & IOC_IN))
		return 0;
	if (_IOC_SIZE(cmd) > buf_size)
		return -EINVAL;
	if (copy_from_user(buf, argp, _IOC_SIZE(cmd)))
		return -EFAULT;
	return 0;
}

static int akmd_copy_out(unsigned int cmd, void __user *argp,
			 void *buf, size_t buf_size)
{
	if (!(cmd & IOC_OUT))
		return 0;
	if (_IOC_SIZE(cmd) > buf_size)
		return -EINVAL;
	if (copy_to_user(argp, buf, _IOC_SIZE(cmd)))
		return -EFAULT;
	return 0;
}

static long akmd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int iRet;
	void __user *argp = (void __user *)arg;
	struct ssp_data *data = container_of(file->private_data,
					struct ssp_data, akmd_device);

	union {
		u8 uData[8];
		u8 uMagData[8];
		u8 uFuseData[3];
		int iAccData[3];
	} akmdbuf;

	iRet = akmd_copy_in(cmd, argp, akmdbuf.uData, sizeof(akmdbuf));
	if (iRet)
		return iRet;

	switch (cmd) {
	case ECS_IOCTL_GET_MAGDATA:
		if ((data->buf[GEOMAGNETIC_SENSOR].x == 0)
			&& (data->buf[GEOMAGNETIC_SENSOR].y == 0)
			&& (data->buf[GEOMAGNETIC_SENSOR].z == 0))
			akmdbuf.uMagData[0] = 0;
		else
			akmdbuf.uMagData[0] = 1;

		akmdbuf.uMagData[1] = data->buf[GEOMAGNETIC_SENSOR].x & 0xff;
		akmdbuf.uMagData[2] = data->buf[GEOMAGNETIC_SENSOR].x >> 8;
		akmdbuf.uMagData[3] = data->buf[GEOMAGNETIC_SENSOR].y & 0xff;
		akmdbuf.uMagData[4] = data->buf[GEOMAGNETIC_SENSOR].y >> 8;
		akmdbuf.uMagData[5] = data->buf[GEOMAGNETIC_SENSOR].z & 0xff;
		akmdbuf.uMagData[6] = data->buf[GEOMAGNETIC_SENSOR].z >> 8;
		akmdbuf.uMagData[7] = 0x10;
		break;
	case ECS_IOCTL_GET_ACCDATA:
		akmdbuf.iAccData[0] = data->buf[ACCELEROMETER_SENSOR].x;
		akmdbuf.iAccData[1] = data->buf[ACCELEROMETER_SENSOR].y;
		akmdbuf.iAccData[2] = data->buf[ACCELEROMETER_SENSOR].z;
		break;
	case ECS_IOCTL_GET_FUSEROMDATA:
		akmdbuf.uFuseData[0] = data->uFuseRomData[0];
		akmdbuf.uFuseData[1] = data->uFuseRomData[1];
		akmdbuf.uFuseData[2] = data->uFuseRomData[2];
		break;
	default:
		return -ENOTTY;
	}

	if (iRet < 0)
		return iRet;

	return akmd_copy_out(cmd, argp, akmdbuf.uData, sizeof(akmdbuf));
}

static const struct file_operations akmd_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.unlocked_ioctl = akmd_ioctl,
};

void initialize_magnetic(struct ssp_data *data)
{
	data->akmd_device.minor = MISC_DYNAMIC_MINOR;
	data->akmd_device.name = "akm8963";
	data->akmd_device.fops = &akmd_fops;
}
