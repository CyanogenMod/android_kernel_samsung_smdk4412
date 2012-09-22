/*
 * firmware.c
 *
 * functions for Linux filesystem access
 * Firmware binary file is on the filesystem, read fild and send it through SDIO
 */
#include "firmware.h"
#include <linux/wimax/samsung/wimax732.h>

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#define WIMAX_BAT_SYSPATH \
	"/sys/devices/platform/sec-battery/power_supply/battery/wimax"

/******************************************************************************
 *                           Library Functions
 ******************************************************************************/

/*!
 *************************************************************************
 * \brief	file open.
 *
 * user context.
 *
 * \return	file pointer, if error, return NULL.
 *************************************************************************/
struct file *klib_fopen(
		const char *filename,	/*!< filename to open */
		/*!< O_RDONLY, O_WRONLY, O_RDWR, O_CREAT,
		O_EXCL, O_TRUNC, O_APPEND, O_NONBLOCK, O_SYNC,...*/
		int flags,
		/*!< file creation permisstion. S_IRxxx
		S_IWxxx S_IXxxx (xxx=USR,GRP,OTH), S_IRWXx(x=U,G,O)*/
		int mode
		)
{
	struct file *filp = filp_open(filename, flags, mode);
	return (IS_ERR(filp)) ? NULL : filp;
}

/*!
 *************************************************************************
 * \brief	file close.
 *
 * user context.
 *
 * \return	none.
 *************************************************************************/
void klib_fclose(
		struct file *filp	/*!< file pointer */
		)
{
	if (filp)
		fput(filp);
}

/*!
 *************************************************************************
 * \brief	move file pointer to the request.
 *
 * user context
 * do not support SEEK_END
 * no boundary check (file position may exceed file size).
 *
 * \return	.
 *************************************************************************/
int klib_fseek(
		struct file *filp,	/*!< file pointer */
		int offset,		/*!<  */
		int whence		/*!< SEEK_SET, SEEK_CUR */
		)
{
	int pos = filp->f_pos;

	if (filp) {
		if (whence == SEEK_SET)
			pos = offset;

		else if (whence == SEEK_CUR)
			pos += offset;

		if (pos < 0)
			pos = 0;

		filp->f_pos = pos;
		return pos;
	} else
		return -ENOENT;
}



/*!
 *************************************************************************
 * \brief	file read function.
 *
 * user context.
 *
 * \return	actually read number, 0 = EOF, negative = error.
 *************************************************************************/
int
klib_fread(
		char *buf,		/*!< buffer to read into */
		int len,		/*!< number of bytes to read */
		struct file *filp	/*!< file pointer */
		)
{
	int readlen;
	mm_segment_t oldfs;

	if (filp == NULL) {
		printk(KERN_INFO "filp == NULL\n");
		return -ENOENT;
	}

	if (filp->f_op->read == NULL) {
		printk(KERN_INFO "filp->f_op->read == NULL\n");
		return -ENOSYS;
	}

	if (((filp->f_flags & O_ACCMODE) & O_RDONLY) != 0) {
		printk(KERN_INFO "((filp->f_flags & O_ACCMODE) & O_RDONLY) != 0\n");
		return -EACCES;
	}

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	readlen = filp->f_op->read(filp, buf, len, &filp->f_pos);
	set_fs(oldfs);

	return readlen;
}

int klib_flen_fcopy(char *buf, int len, struct file *filp)
{
	int readlen;
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	readlen = filp->f_op->read(filp, buf, len, &filp->f_pos);
	set_fs(oldfs);

	return readlen;
}

/*!
 *************************************************************************
 * \brief	.
 *
 * user context.
 *
 * \return	read character, EOF if end of file.
 *************************************************************************/
int klib_fgetc(
		struct file *filp	/*!< file pointer */
		)
{
	int len;
	u_char buf[4];

	len = klib_fread((char *)buf, 1, filp);
	if (len > 0)
		return buf[0];
	else if (len == 0)
		return -1;
	else
		return len;
}

/*!
 *************************************************************************
 * \brief	to get the length of file.
 *
 * detailed decrtiption.
 *
 * \return	.
 *************************************************************************/
int klib_flength(
		struct file *filp	/*!< file pointer */
		)
{
	int total_len = 0;
	int buf;

	do {
		buf = klib_fgetc(filp);
		if (buf == -1)
			break;
		total_len++;
	} while (1);

	klib_fseek(filp, 0, SEEK_SET);

	return total_len;
}

/*!
 *************************************************************************
 * \brief	file write function.
 *
 * user context.
 *
 * \return	actually write number, 0 = EOF, negative = error.
 *************************************************************************/
int klib_fwrite(
		char *buf,			/*!< buffer to write into */
		int len,			/*!< number of bytes to write */
		struct file *filp		/*!< file pointer */
		)
{
	int writelen;
	mm_segment_t oldfs;

	if (filp == NULL) {
		printk(KERN_INFO "filp == NULL\n");
		return -ENOENT;
	}

	if (filp->f_op->write == NULL) {
		printk(KERN_INFO "filp->f_op->write == NULL\n");
		return -ENOSYS;
	}

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	writelen = filp->f_op->write(filp, buf, len, &filp->f_pos);
	set_fs(oldfs);

	return writelen;
}

void s3c_bat_use_wimax(int onoff)
{
	struct file     *fp;
	fp = klib_fopen(WIMAX_BAT_SYSPATH, O_RDWR, 0);

	if (!fp)
		pr_err("open fail");
	if (onoff)
			klib_fwrite("1", 1, fp);
	else
		klib_fwrite("0", 1, fp);
		klib_fclose(fp);
}
EXPORT_SYMBOL(s3c_bat_use_wimax);
