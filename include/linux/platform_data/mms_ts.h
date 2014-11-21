/*
 * mms_ts.h - Platform data for Melfas MMS-series touch driver
 *
 * Copyright (C) 2011 Google Inc.
 * Author: Dima Zavin <dima@android.com>
 *
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef _LINUX_MMS_TOUCH_H
#define _LINUX_MMS_TOUCH_H
#define MELFAS_TS_NAME			"melfas-ts"

struct melfas_tsi_platform_data {
	int max_x;
	int max_y;

	bool invert_x;
	bool invert_y;

	int gpio_int;
	int gpio_sda;
	int gpio_scl;
	int (*mux_fw_flash) (bool to_gpios);
	int (*power) (bool on);
	int (*is_vdd_on) (void);
	const char *fw_name;
	bool use_touchkey;
	const u8 *touchkey_keycode;
	const u8 *config_fw_version;
#ifdef CONFIG_INPUT_FBSUSPEND
	struct notifier_block fb_notif;
#endif
	void (*input_event) (void *data);
	int (*lcd_type) (void);
	void (*register_cb) (void *);
};
extern struct class *sec_class;
void tsp_charger_infom(bool en);

#endif /* _LINUX_MMS_TOUCH_H */

#ifndef __MMS100_ISC_H__
#define __MMS100_ISC_H__

typedef enum {
	ISC_NONE = -1,
	ISC_SUCCESS = 0,
	ISC_FILE_OPEN_ERROR,
	ISC_FILE_CLOSE_ERROR,
	ISC_FILE_FORMAT_ERROR,
	ISC_WRITE_BUFFER_ERROR,
	ISC_I2C_ERROR,
	ISC_UPDATE_MODE_ENTER_ERROR,
	ISC_CRC_ERROR,
	ISC_VALIDATION_ERROR,
	ISC_COMPATIVILITY_ERROR,
	ISC_UPDATE_SECTION_ERROR,
	ISC_SLAVE_ERASE_ERROR,
	ISC_SLAVE_DOWNLOAD_ERROR,
	ISC_DOWNLOAD_WHEN_SLAVE_IS_UPDATED_ERROR,
	ISC_INITIAL_PACKET_ERROR,
	ISC_NO_NEED_UPDATE_ERROR,
	ISC_LIMIT
} eISCRet_t;

typedef enum {
	EC_NONE = -1,
	EC_DEPRECATED = 0,
	EC_BOOTLOADER_RUNNING = 1,
	EC_BOOT_ON_SUCCEEDED = 2,
	EC_ERASE_END_MARKER_ON_SLAVE_FINISHED = 3,
	EC_SLAVE_DOWNLOAD_STARTS = 4,
	EC_SLAVE_DOWNLOAD_FINISHED = 5,
	EC_2CHIP_HANDSHAKE_FAILED = 0x0E,
	EC_ESD_PATTERN_CHECKED = 0x0F,
	EC_LIMIT
} eErrCode_t;

typedef enum {
	SEC_NONE = -1,
	SEC_BOOTLOADER = 0,
	SEC_CORE,
	SEC_PRIVATE_CONFIG,
	SEC_PUBLIC_CONFIG,
	SEC_LIMIT
} eSectionType_t;

typedef struct {
	unsigned char version;
	unsigned char compatible_version;
	unsigned char start_addr;
	unsigned char end_addr;
} tISCFWInfo_t;

/*eISCRet_t mms100_ISC_download_mbinary(struct i2c_client *_client); */
#endif /* __MMS100_ISC_H__ */


