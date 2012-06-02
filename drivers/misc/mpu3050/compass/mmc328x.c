/*
 * Copyright (C) 2010 MEMSIC, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/i2c.h>

#include "mpu.h"
#include "mlsl.h"
#include "mlos.h"

#include <log.h>

#define MMC328X_I2C_ADDR	0x30

#define MMC328X_REG_CTRL	0x07
#define MMC328X_REG_DATA	0x00
#define MMC328X_REG_DS		0x06

#define MMC328X_CTRL_TM		0x01
#define MMC328X_CTRL_RM		0x20

#define MMC328X_DELAY_TM	10	/* ms */
#define MMC328X_DELAY_RM	10	/* ms */
#define MMC328X_DELAY_STDN	1	/* ms */

static int mmc328x_i2c_rx_data(void *mlsl_handle,
	struct ext_slave_platform_data *pdata, char *buf, int len)
{
	uint8_t i;
	struct i2c_msg msgs[2] = { 0, };

	if (NULL == buf || NULL == mlsl_handle)
		return -EINVAL;

	msgs[0].addr = pdata->address;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = buf;

	msgs[1].addr = pdata->address;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = buf;

	if (i2c_transfer(mlsl_handle, msgs, 2) >= 0)
		return -1;

	return 0;
}

static int mmc328x_i2c_tx_data(void *mlsl_handle,
	struct ext_slave_platform_data *pdata, char *buf, int len)
{
	struct i2c_msg msgs[1];
	int res;

	if (NULL == buf || NULL == mlsl_handle)
		return -EINVAL;

	msgs[0].addr = pdata->address;
	msgs[0].flags = 0;	/* write */
	msgs[0].buf = (unsigned char *)buf;
	msgs[0].len = len;

	res = i2c_transfer(mlsl_handle, msgs, 1);
	if (res < 1)
		return res;
	else
		return 0;
}

int memsic_api_resume(void *mlsl_handle,
	struct ext_slave_platform_data *pdata)
{
#if 0
	unsigned char data[2] = { 0 };

	data[0] = MMC328X_REG_CTRL;
	data[1] = MMC328X_CTRL_RM;
	if (mmc328x_i2c_tx_data(mlsl_handle, pdata, data, 2) < 0)
		return -1;

	msleep(MMC328X_DELAY_RM);
#endif
	return 0;
}

int memsic_api_suspend(void *mlsl_handle,
	struct ext_slave_platform_data *pdata)
{
#if 0
	unsigned char data[2] = { 0 };

	data[0] = MMC328X_REG_CTRL;
	data[1] = 0;
	if (mmc328x_i2c_tx_data(mlsl_handle, pdata, data, 2) < 0)
		return -1;
	msleep(MMC328X_DELAY_RM);

	data[0] = MMC328X_REG_CTRL;
	data[1] = 0;
	mmc328x_i2c_tx_data(mlsl_handle, pdata, data, 2);
	msleep(MMC328X_DELAY_TM);
#endif
	return 0;
}

int memsic_api_read(void *mlsl_handle, struct ext_slave_platform_data *pdata,
		unsigned char *raw_data, int *accuracy)
{
	unsigned char data[6] = { 0 };
	int MD_times = 0;

	data[0] = MMC328X_REG_CTRL;
	data[1] = MMC328X_CTRL_TM;
	mmc328x_i2c_tx_data(mlsl_handle, pdata, data, 2);
	msleep(MMC328X_DELAY_TM);

	data[0] = MMC328X_REG_DS;
	if (mmc328x_i2c_rx_data(mlsl_handle, pdata, data, 1) < 0)
		return -EFAULT;

	while (!(data[0] & 0x01)) {
		msleep(20);
		data[0] = MMC328X_REG_DS;

		if (mmc328x_i2c_rx_data(mlsl_handle, pdata, data, 1) < 0)
			return -EFAULT;

		if (data[0] & 0x01)
			break;

		MD_times++;

		if (MD_times > 2)
			return -EFAULT;
	}

	data[0] = MMC328X_REG_DATA;
	if (mmc328x_i2c_rx_data(mlsl_handle, pdata, data, 6) < 0)
		return -EFAULT;

	memcpy(raw_data, data, sizeof(unsigned char) * 6);

	*accuracy = 0;

	return 0;
}

int mmc328x_read(void *mlsl_handle,
		 struct ext_slave_descr *slave,
		 struct ext_slave_platform_data *pdata, unsigned char *data)
{
	int xyz[3] = { 0, };
	int accuracy = 0;

	memsic_api_read(mlsl_handle, pdata, data, &accuracy);

	return ML_SUCCESS;
}

int mmc328x_suspend(void *mlsl_handle,
		    struct ext_slave_descr *slave,
		    struct ext_slave_platform_data *pdata)
{
	int result = ML_SUCCESS;
	memsic_api_suspend(mlsl_handle, pdata);
	return result;
}

int mmc328x_resume(void *mlsl_handle,
		   struct ext_slave_descr *slave,
		   struct ext_slave_platform_data *pdata)
{

	memsic_api_resume(mlsl_handle, pdata);
	return ML_SUCCESS;
}

struct ext_slave_descr mmc328x_descr = {
	/*.init             = */ NULL,
	/*.exit             = */ NULL,
	/*.suspend          = */ mmc328x_suspend,
	/*.resume           = */ mmc328x_resume,
	/*.read             = */ mmc328x_read,
	/*.config           = */ NULL,
	/*.get_config       = */ NULL,
	/*.name             = */ "mmc328x",
	/*.type             = */ EXT_SLAVE_TYPE_COMPASS,
	/*.id               = */ COMPASS_ID_MMC328X,
	/*.reg              = */ 0x01,
	/*.len              = */ 6,
	/*.endian           = */ EXT_SLAVE_BIG_ENDIAN,
	/*.range            = */ {400, 0},
};

struct ext_slave_descr *mmc328x_get_slave_descr(void)
{
	return &mmc328x_descr;
}
EXPORT_SYMBOL(mmc328x_get_slave_descr);

/**
 *  @}
**/
