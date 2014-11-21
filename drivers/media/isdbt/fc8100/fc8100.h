/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : bbm.c

 Description : API of dmb baseband module

 History :
 ----------------------------------------------------------------------
 2009/08/29	jason		initial
*******************************************************************************/

#ifndef __DMB_H__
#define __DMB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fci_types.h"

#define MAX_OPEN_NUM		8

#define IOCTL_MAGIC	't'

typedef struct {
	unsigned long size;
	unsigned long buff[128];
} ioctl_info;

#define IOCTL_MAXNR			22

#define IOCTL_DMB_RESET			_IO(IOCTL_MAGIC, 0)
#define IOCTL_DMB_PROBE			_IO(IOCTL_MAGIC, 1)
#define IOCTL_DMB_INIT			_IO(IOCTL_MAGIC, 2)
#define IOCTL_DMB_DEINIT		_IO(IOCTL_MAGIC, 3)

#define IOCTL_DMB_BYTE_READ		_IOWR(IOCTL_MAGIC, 4, ioctl_info)
#define IOCTL_DMB_WORD_READ		_IOWR(IOCTL_MAGIC, 5, ioctl_info)
#define IOCTL_DMB_LONG_READ		_IOWR(IOCTL_MAGIC, 6, ioctl_info)
#define IOCTL_DMB_BULK_READ		_IOWR(IOCTL_MAGIC, 7, ioctl_info)

#define IOCTL_DMB_BYTE_WRITE		_IOW(IOCTL_MAGIC, 8, ioctl_info)
#define IOCTL_DMB_WORD_WRITE		_IOW(IOCTL_MAGIC, 9, ioctl_info)
#define IOCTL_DMB_LONG_WRITE		_IOW(IOCTL_MAGIC, 10, ioctl_info)
#define IOCTL_DMB_BULK_WRITE		_IOW(IOCTL_MAGIC, 11, ioctl_info)

#define IOCTL_DMB_TUNER_READ		_IOWR(IOCTL_MAGIC, 12, ioctl_info)
#define IOCTL_DMB_TUNER_WRITE		_IOW(IOCTL_MAGIC, 13, ioctl_info)

#define IOCTL_DMB_TUNER_SET_FREQ	_IOW(IOCTL_MAGIC, 14, ioctl_info)
#define IOCTL_DMB_TUNER_SELECT		_IOW(IOCTL_MAGIC, 15, ioctl_info)
#define IOCTL_DMB_TUNER_DESELECT	_IO(IOCTL_MAGIC, 16)
#define IOCTL_DMB_TUNER_GET_RSSI	_IOWR(IOCTL_MAGIC, 17, ioctl_info)

#define IOCTL_DMB_HOSTIF_SELECT		_IOW(IOCTL_MAGIC, 18, ioctl_info)
#define IOCTL_DMB_HOSTIF_DESELECT	_IO(IOCTL_MAGIC, 19)

#define IOCTL_DMB_POWER_ON		_IO(IOCTL_MAGIC, 20)
#define IOCTL_DMB_POWER_OFF		_IO(IOCTL_MAGIC, 21)

#ifdef __cplusplus
}
#endif

#endif
