/* linux/drivers/video/samsung/mdnie_tuning.c
 *
 * Register interface file for Samsung mDNIe driver
 *
 * Copyright (c) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/lcd.h>
#include <linux/rtc.h>
#include <linux/fb.h>
#include <mach/gpio.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>
#include <linux/magic.h>

#include "mdnie.h"

#define KFREE(ptr)	do { if (ptr) kfree(ptr); (ptr) = NULL; } while (0)
#define CONSTANT_F1(x)			(((x << 10) * 23) / 21)
#define CONSTANT_F2(x)			(((x << 10) * 19) / 18)
#define CONSTANT_F3(x)			(((x << 10) * 5))
#define CONSTANT_F4(x)			(((x << 10) * 21) / 8)

#define COLOR_OFFSET_F1(x, y)		(((y << 10) - CONSTANT_F1(x) + (12 << 10)) >> 10)
#define COLOR_OFFSET_F2(x, y)		(((y << 10) - CONSTANT_F2(x) + (44 << 10)) >> 10)
#define COLOR_OFFSET_F3(x, y)		(((y << 10) + CONSTANT_F3(x) - (18365 << 10)) >> 10)
#define COLOR_OFFSET_F4(x, y)		(((y << 10) + CONSTANT_F4(x) - (10814 << 10)) >> 10)

int mdnie_calibration(unsigned short x, unsigned short y, int *result)
{
	int ret = 0;

	result[1] = COLOR_OFFSET_F1(x, y);
	result[2] = COLOR_OFFSET_F2(x, y);
	result[3] = COLOR_OFFSET_F3(x, y);
	result[4] = COLOR_OFFSET_F4(x, y);

	if (result[1] > 0) {
		if (result[3] > 0)
			ret = 3;
		else
			ret = (result[4] < 0) ? 1 : 2;
	} else {
		if (result[2] < 0) {
			if (result[3] > 0)
				ret = 9;
			else
				ret = (result[4] < 0) ? 7 : 8;
		} else {
			if (result[3] > 0)
				ret = 6;
			else
				ret = (result[4] < 0) ? 4 : 5;
		}
	}

	ret = (ret > 0) ? ret : 1;
	ret = (ret > 9) ? 1 : ret;

	pr_info("%d, %d, %d, %d, TUNE%d\n", result[1], result[2], result[3], result[4], ret);

	return ret;
}

int mdnie_open_file(const char *path, char **fp)
{
	struct file *filp;
	char	*dp;
	long	length;
	int     ret;
	struct super_block *sb;
	loff_t  pos = 0;

	if (!path) {
		pr_err("%s: path is invalid\n", __func__);
		return -EPERM;
	}

	filp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		pr_err("%s: filp_open skip: %s\n", __func__, path);
		return -EPERM;
	}

	length = i_size_read(filp->f_path.dentry->d_inode);
	if (length <= 0) {
		pr_err("%s: file length is %ld\n", __func__, length);
		return -EPERM;
	}

	dp = kzalloc(length, GFP_KERNEL);
	if (dp == NULL) {
		pr_err("%s: fail to alloc size %ld\n", __func__, length);
		filp_close(filp, current->files);
		return -EPERM;
	}

	ret = kernel_read(filp, pos, dp, length);
	if (ret != length) {
		/* check node is sysfs, bus this way is not safe */
		sb = filp->f_path.dentry->d_inode->i_sb;
		if ((sb->s_magic != SYSFS_MAGIC) && (length != sb->s_blocksize)) {
			pr_err("%s: read size= %d, length= %ld\n", __func__, ret, length);
			KFREE(dp);
			filp_close(filp, current->files);
			return -EPERM;
		}
	}

	filp_close(filp, current->files);

	*fp = dp;

	return ret;
}

int mdnie_check_firmware(const char *path, char *name)
{
	char *ptr = NULL;
	int ret = 0, size;

	size = mdnie_open_file(path, &ptr);
	if (IS_ERR_OR_NULL(ptr) || size <= 0) {
		pr_err("%s: file open skip %s\n", __func__, path);
		KFREE(ptr);
		return -EPERM;
	}

	ret = (strstr(ptr, name) != NULL) ? 1 : 0;

	KFREE(ptr);

	return ret;
}

int mdnie_request_firmware(const char *path, u16 **buf, const char *name)
{
	char *token, *ptr = NULL;
	unsigned short *dp;
	int ret = 0, size, i = 0;
	unsigned int data1, data2;

	size = mdnie_open_file(path, &ptr);
	if (IS_ERR_OR_NULL(ptr) || size <= 0) {
		pr_err("%s: file open skip %s\n", __func__, path);
		KFREE(ptr);
		return -EPERM;
	}

	dp = kzalloc(size, GFP_KERNEL);
	if (dp == NULL) {
		pr_err("%s: fail to alloc size %d\n", __func__, size);
		KFREE(ptr);
		return -ENOMEM;
	}

	if (name) {
		if (strstr(ptr, name) != NULL) {
			pr_info("found %s in %s\n", name, path);
			ptr = strstr(ptr, name);
		}
	}

	while ((token = strsep(&ptr, "\r\n")) != NULL) {
		if ((name) && (!strncmp(token, "};", 2))) {
			pr_info("found %s end in local, stop searching\n", name);
			break;
		}
		ret = sscanf(token, "%x, %x", &data1, &data2);
		if (ret == 2) {
			dp[i] = (u16)data1;
			dp[i+1] = (u16)data2;
			i += 2;
		}
	}

	dp[i] = END_SEQ;

	*buf = dp;

	KFREE(ptr);

	return i;
}

