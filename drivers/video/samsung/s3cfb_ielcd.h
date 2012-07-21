/* linux/drivers/video/samsung/s3cfb_ielcd.h
 *
 * Header file for Samsung (IELCD) driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S3CFB_IELCD_H
#define _S3CFB_IELCD_H

#define S3C_IELCD_MAGIC_KEY		0x2ff47

#define S3C_IELCD_PHY_BASE		0x11C40000
#define S3C_IELCD_MAP_SIZE		0x00008000

#define S3C_IELCD_GPOUTCON0		(0x0278)

int s3c_ielcd_hw_init(void);
int s3c_ielcd_logic_start(void);
int s3c_ielcd_logic_stop(void);
int s3c_ielcd_display_on(void);
int s3c_ielcd_display_off(void);


int s3c_ielcd_init_global(struct s3cfb_global *ctrl);

#endif
