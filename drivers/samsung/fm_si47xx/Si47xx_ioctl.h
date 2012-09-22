/*
 * Si47xx_ioctl.h  --  Si47xx FM Radio driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _Si47xx_IOCTL_H
#define _Si47xx_IOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>

#include "Si47xx_dev.h"

/*****************IOCTLS******************/
/*magic no*/
#define Si47xx_IOC_MAGIC		0xFA
/*max seq no*/
#define Si47xx_IOC_NR_MAX		40

/*commands*/

#define Si47xx_IOC_POWERUP		_IO(Si47xx_IOC_MAGIC, 0)

#define Si47xx_IOC_POWERDOWN		_IO(Si47xx_IOC_MAGIC, 1)

#define Si47xx_IOC_BAND_SET		_IOW(Si47xx_IOC_MAGIC, 2, int)

#define Si47xx_IOC_CHAN_SPACING_SET	_IOW(Si47xx_IOC_MAGIC, 3, int)

#define Si47xx_IOC_CHAN_SELECT		_IOW(Si47xx_IOC_MAGIC, 4, u32)

#define Si47xx_IOC_CHAN_GET		_IOR(Si47xx_IOC_MAGIC, 5, u32)

#define Si47xx_IOC_SEEK_UP		_IOR(Si47xx_IOC_MAGIC, 6, u32)

#define Si47xx_IOC_SEEK_DOWN		_IOR(Si47xx_IOC_MAGIC, 7, u32)

/*VNVS:28OCT'09---- Si47xx_IOC_SEEK_AUTO is disabled as of now*/
/* #define Si47xx_IOC_SEEK_AUTO		_IOR(Si47xx_IOC_MAGIC, 8, u32) */

#define Si47xx_IOC_RSSI_SEEK_TH_SET	_IOW(Si47xx_IOC_MAGIC, 9, u8)

#define Si47xx_IOC_SEEK_SNR_SET		_IOW(Si47xx_IOC_MAGIC, 10, u8)

#define Si47xx_IOC_SEEK_CNT_SET		_IOW(Si47xx_IOC_MAGIC, 11, u8)

#define Si47xx_IOC_CUR_RSSI_GET		\
_IOR(Si47xx_IOC_MAGIC, 12, struct rssi_snr_t)

#define Si47xx_IOC_VOLEXT_ENB		_IO(Si47xx_IOC_MAGIC, 13)

#define Si47xx_IOC_VOLEXT_DISB		_IO(Si47xx_IOC_MAGIC, 14)

#define Si47xx_IOC_VOLUME_SET		_IOW(Si47xx_IOC_MAGIC, 15, u8)

#define Si47xx_IOC_VOLUME_GET		_IOR(Si47xx_IOC_MAGIC, 16, u8)

#define Si47xx_IOC_MUTE_ON		_IO(Si47xx_IOC_MAGIC, 17)

#define Si47xx_IOC_MUTE_OFF		_IO(Si47xx_IOC_MAGIC, 18)

#define Si47xx_IOC_MONO_SET		_IO(Si47xx_IOC_MAGIC, 19)

#define Si47xx_IOC_STEREO_SET		_IO(Si47xx_IOC_MAGIC, 20)

#define Si47xx_IOC_RSTATE_GET		\
_IOR(Si47xx_IOC_MAGIC, 21, struct dev_state_t)

#define Si47xx_IOC_RDS_DATA_GET		\
_IOR(Si47xx_IOC_MAGIC, 22, struct radio_data_t)

#define Si47xx_IOC_RDS_ENABLE		_IO(Si47xx_IOC_MAGIC, 23)

#define Si47xx_IOC_RDS_DISABLE		_IO(Si47xx_IOC_MAGIC, 24)

#define Si47xx_IOC_RDS_TIMEOUT_SET	_IOW(Si47xx_IOC_MAGIC, 25, u32)

#define Si47xx_IOC_SEEK_CANCEL		_IO(Si47xx_IOC_MAGIC, 26)

/*VNVS:START 13-OCT'09 :
 Added IOCTLs for reading the device-id,chip-id,power configuration,
 system configuration2 registers*/
#define Si47xx_IOC_DEVICE_ID_GET	\
_IOR(Si47xx_IOC_MAGIC, 27, struct device_id)

#define Si47xx_IOC_CHIP_ID_GET		\
_IOR(Si47xx_IOC_MAGIC, 28, struct chip_id)

#define Si47xx_IOC_SYS_CONFIG2_GET	\
_IOR(Si47xx_IOC_MAGIC, 29, struct sys_config2)

#define Si47xx_IOC_POWER_CONFIG_GET	\
_IO(Si47xx_IOC_MAGIC, 30)

/* For reading AFCRL bit, to check for a valid channel */
#define Si47xx_IOC_AFCRL_GET		_IOR(Si47xx_IOC_MAGIC, 31, u8)

/* Setting DE-emphasis Time Constant.
For DE=0,TC=50us(Europe,Japan,Australia) and DE=1,TC=75us(USA) */
#define Si47xx_IOC_DE_SET		_IOW(Si47xx_IOC_MAGIC, 32, u8)
/*VNVS:END*/

#define Si47xx_IOC_SYS_CONFIG3_GET	\
_IOR(Si47xx_IOC_MAGIC, 33, struct sys_config3)

#define Si47xx_IOC_STATUS_RSSI_GET	\
_IOR(Si47xx_IOC_MAGIC, 34, struct status_rssi)

#define Si47xx_IOC_SYS_CONFIG2_SET	\
_IOW(Si47xx_IOC_MAGIC, 35, struct sys_config2)

#define Si47xx_IOC_SYS_CONFIG3_SET	\
_IOW(Si47xx_IOC_MAGIC, 36, struct sys_config3)

#define Si47xx_IOC_DSMUTE_ON		_IO(Si47xx_IOC_MAGIC, 37)

#define Si47xx_IOC_DSMUTE_OFF		_IO(Si47xx_IOC_MAGIC, 38)

#define Si47xx_IOC_RESET_RDS_DATA	_IO(Si47xx_IOC_MAGIC, 39)

#define Si47xx_IOC_SEEK_FULL		_IOR(Si47xx_IOC_MAGIC, 40, u32)


/*****************************************/

#endif
