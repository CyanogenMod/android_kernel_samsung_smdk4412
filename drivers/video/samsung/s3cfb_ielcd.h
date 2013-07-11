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

#ifndef __S3CFB_IELCD_H__
#define __S3CFB_IELCD_H__

#define S3C_IELCD_MAGIC_KEY		0x2ff47

#define S3C_IELCD_PHY_BASE		0x11C40000
#define S3C_IELCD_MAP_SIZE		0x00008000

#define S3C_IELCD_GPOUTCON0		0x0278

int ielcd_hw_init(void);
int ielcd_display_on(void);
int ielcd_display_off(void);
void ielcd_init_global(struct s3cfb_global *ctrl);

#endif
