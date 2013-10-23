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

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>
#include <linux/magic.h>

#include "s3cfb.h"
#include "mdnie_kona.h"

#define KFREE(ptr)	do { if (ptr) kfree(ptr); (ptr) = NULL; } while (0)
#define CONSTANT_F1	((23<<10)/21)
#define CONSTANT_F2	((19<<10)/18)
#define CONSTANT_F3	(5<<10)
#define CONSTANT_F4	((21<<10)/8)

int mdnie_calibration(unsigned short x, unsigned short y, int *result)
{
	u8 ret = 0;

	result[1] = ((y << 10) - (CONSTANT_F1 * x) + (12 << 10)) >> 10;
	result[2] = ((y << 10) - (CONSTANT_F2 * x) + (44 << 10)) >> 10;
	result[3] = ((y << 10) + (CONSTANT_F3 * x) - (18365 << 10)) >> 10;
	result[4] = ((y << 10) + (CONSTANT_F4 * x) - (10814 << 10)) >> 10;

	pr_info("%d, %d, %d, %d\n", result[1], result[2], result[3], result[4]);

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

	return ret;
}

static int parse_text(char *src, int len, u16 *buf, char *name)
{
	int i, count, ret;
	int index = 0;
	char *str_line[100];
	char *sstart;
	char *c;
	unsigned int data1, data2;

	c = src;
	count = 0;
	sstart = c;

	for (i = 0; i < len; i++, c++) {
		char a = *c;
		if (a == '\r' || a == '\n') {
			if (c > sstart) {
				str_line[count] = sstart;
				count++;
			}
			*c = '\0';
			sstart = c+1;
		}
	}

	if (c > sstart) {
		str_line[count] = sstart;
		count++;
	}

	/* printk(KERN_INFO "----------------------------- Total number of lines:%d\n", count); */

	for (i = 0; i < count; i++) {
		/* printk(KERN_INFO "line:%d, [start]%s[end]\n", i, str_line[i]); */
		ret = sscanf(str_line[i], "0x%x, 0x%x\n", &data1, &data2);
		/* printk(KERN_INFO "Result => [0x%2x 0x%4x] %s\n", data1, data2, (ret == 2) ? "Ok" : "Not available"); */
		if (ret == 2) {
			buf[index++] = (unsigned short)data1;
			buf[index++] = (unsigned short)data2;
		}
	}

	buf[index] = END_SEQ;

	return index;
}

int mdnie_txtbuf_to_parsing(char *path, u16 size, u16 *buf, char *name)
{
	struct file *filp;
	char	*dp;
	long	l;
	loff_t  pos;
	int     ret, num;
	mm_segment_t fs;

	fs = get_fs();
	set_fs(get_ds());

	if (!path) {
		pr_err("%s: invalid filepath\n", __func__);
		goto parse_err;
	}

	filp = filp_open(path, O_RDONLY, 0);

	if (IS_ERR(filp)) {
		pr_err("file open error: %s\n", path);
		goto parse_err;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	dp = kmalloc(l, GFP_KERNEL);
	if (dp == NULL) {
		pr_err("Out of Memory!\n");
		filp_close(filp, current->files);
		goto parse_err;
	}
	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);

	if (ret != l) {
		pr_info("read size = %d, l = %ld, size=%d\n", ret, l, size);
		l = (l > ret) ? ret : l;
		if (size < l) {
			KFREE(dp);
			filp_close(filp, current->files);
			goto parse_err;
		}
	}

	filp_close(filp, current->files);
	set_fs(fs);

	num = parse_text(dp, l, buf, name);

	if (!num) {
		pr_err("Nothing to parse!\n");
		KFREE(dp);
		goto parse_err;
	}

	KFREE(dp);

	return num;

parse_err:
	return -EPERM;
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
		pr_err("%s: file open err: %s\n", __func__, path);
		return -EPERM;
	}

	length = i_size_read(filp->f_path.dentry->d_inode);
	if (length <= 0) {
		pr_err("%s: file size %ld error\n", __func__, length);
		return -EPERM;
	}

	dp = kzalloc(length, GFP_KERNEL);
	if (dp == NULL) {
		pr_err("%s: Out of Memory\n", __func__);
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
		pr_err("%s: file open fail %s\n", __func__, path);
		KFREE(ptr);
		return 0;
	}

	ret = (strstr(ptr, name) != NULL) ? 1 : 0;

	KFREE(ptr);

	return ret;
}

int mdnie_request_firmware(const char *path, u16 **buf, char *name)
{
	char *token, *ptr = NULL;
	unsigned short *dp;
	int ret = 0, size, i = 0;
	unsigned int data1, data2;

	size = mdnie_open_file(path, &ptr);
	if (IS_ERR_OR_NULL(ptr) || size <= 0) {
		pr_err("%s: file open fail %s\n", __func__, path);
		KFREE(ptr);
		return ret;
	}

	dp = kzalloc(size, GFP_KERNEL);
	if (dp == NULL) {
		pr_err("%s: Out of Memory\n", __func__);
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

