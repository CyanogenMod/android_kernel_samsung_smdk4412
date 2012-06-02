/*
 $License:
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
  $
 */

/**
 *  @defgroup   COMPASSDL (Motion Library - Accelerometer Driver Layer)
 *  @brief      Provides the interface to setup and handle an accelerometers
 *              connected to the secondary I2C interface of the gyroscope.
 *
 *  @{
 *     @file   ami304.c
 *     @brief  Magnetometer setup and handling methods for Aichi ami304 compass.
*/

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#ifdef __KERNEL__
#include <linux/module.h>
#endif

#include "mpu.h"
#include "mlsl.h"
#include "mlos.h"

#include <log.h>
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-compass"

#define AMI304_REG_DATAX (0x10)
#define AMI304_REG_STAT1 (0x18)
#define AMI304_REG_CNTL1 (0x1B)
#define AMI304_REG_CNTL2 (0x1C)
#define AMI304_REG_CNTL3 (0x1D)

#define AMI304_BIT_CNTL1_PC1  (0x80)
#define AMI304_BIT_CNTL1_ODR1 (0x10)
#define AMI304_BIT_CNTL1_FS1  (0x02)

#define AMI304_BIT_CNTL2_IEN  (0x10)
#define AMI304_BIT_CNTL2_DREN (0x08)
#define AMI304_BIT_CNTL2_DRP  (0x04)
#define AMI304_BIT_CNTL3_F0RCE (0x40)

int ami304_suspend(void *mlsl_handle,
		   struct ext_slave_descr *slave,
		   struct ext_slave_platform_data *pdata)
{
	int result;
	unsigned char reg;
	result =
	    MLSLSerialRead(mlsl_handle, pdata->address, AMI304_REG_CNTL1,
			   1, &reg);
	ERROR_CHECK(result);

	reg &= ~(AMI304_BIT_CNTL1_PC1|AMI304_BIT_CNTL1_FS1);
	result =
	    MLSLSerialWriteSingle(mlsl_handle, pdata->address,
				  AMI304_REG_CNTL1, reg);
	ERROR_CHECK(result);

	return result;
}

int ami304_resume(void *mlsl_handle,
		  struct ext_slave_descr *slave,
		  struct ext_slave_platform_data *pdata)
{
	int result = ML_SUCCESS;

	/* Set CNTL1 reg to power model active */
	result =
	    MLSLSerialWriteSingle(mlsl_handle, pdata->address,
				  AMI304_REG_CNTL1,
				  AMI304_BIT_CNTL1_PC1|AMI304_BIT_CNTL1_FS1);
	ERROR_CHECK(result);
	/* Set CNTL2 reg to DRDY active high and enabled */
	result =
	    MLSLSerialWriteSingle(mlsl_handle, pdata->address,
				  AMI304_REG_CNTL2,
				  AMI304_BIT_CNTL2_DREN |
				  AMI304_BIT_CNTL2_DRP);
	ERROR_CHECK(result);
	/* Set CNTL3 reg to forced measurement period */
	result =
		MLSLSerialWriteSingle(mlsl_handle, pdata->address,
				  AMI304_REG_CNTL3, AMI304_BIT_CNTL3_F0RCE);

	return result;
}

int ami304_read(void *mlsl_handle,
		struct ext_slave_descr *slave,
		struct ext_slave_platform_data *pdata, unsigned char *data)
{
	unsigned char stat;
	int result = ML_SUCCESS;

	/* Read status reg and check if data ready (DRDY) */
	result =
	    MLSLSerialRead(mlsl_handle, pdata->address, AMI304_REG_STAT1,
			   1, &stat);
	ERROR_CHECK(result);

	if (stat & 0x40) {
		result =
		    MLSLSerialRead(mlsl_handle, pdata->address,
				   AMI304_REG_DATAX, 6,
				   (unsigned char *) data);
		ERROR_CHECK(result);
		/* start another measurement */
		result =
			MLSLSerialWriteSingle(mlsl_handle, pdata->address,
					      AMI304_REG_CNTL3,
					      AMI304_BIT_CNTL3_F0RCE);
		ERROR_CHECK(result);

		return ML_SUCCESS;
	}

	return ML_ERROR_COMPASS_DATA_NOT_READY;
}

struct ext_slave_descr ami304_descr = {
	/*.suspend          = */ ami304_suspend,
	/*.resume           = */ ami304_resume,
	/*.read             = */ ami304_read,
	/*.name             = */ "ami304",
	/*.type             = */ EXT_SLAVE_TYPE_COMPASS,
	/*.id               = */ COMPASS_ID_AICHI,
	/*.reg              = */ 0x06,
	/*.len              = */ 6,
	/*.endian           = */ EXT_SLAVE_LITTLE_ENDIAN,
	/*.range            = */ {5461, 3333}
};

struct ext_slave_descr *ami304_get_slave_descr(void)
{
	return &ami304_descr;
}

#ifdef __KERNEL__
EXPORT_SYMBOL(ami304_get_slave_descr);
#endif

/**
 *  @}
**/
