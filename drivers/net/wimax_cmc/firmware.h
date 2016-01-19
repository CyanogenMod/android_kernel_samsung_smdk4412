/*
 * firmware.h
 *
 * functions for Linux filesystem access
 * Firmware binary file is on the filesystem, read fild and send it through SDIO
 */
#ifndef _WIMAX_FIRMWARE_H
#define _WIMAX_FIRMWARE_H

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/uaccess.h>

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

/******************************************************************************
 *                          Function Prototypes
 ******************************************************************************/
struct file *klib_fopen(const char *filename, int flags, int mode);
void klib_fclose(struct file *filp);
int klib_fseek(struct file *filp, int offset, int whence);
int klib_fread(char *buf, int len, struct file *filp);
int klib_fgetc(struct file *filp);
int klib_flength(struct file *filp);
int klib_flen_fcopy(char *buf, int len, struct file *filp);
int klib_fwrite(char *buf, int len, struct file *filp);
#endif	/* _WIMAX_FIRMWARE_H */
