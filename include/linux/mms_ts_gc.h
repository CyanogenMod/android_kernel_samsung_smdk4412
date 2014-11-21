/*
 * include/linux/mms152.h - platform data structure for MCS Series sensor
 *
 * Copyright (C) 2010 Melfas, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_MELFAS_TS_H
#define _LINUX_MELFAS_TS_H

#define TS_DEV_NAME "melfas_ts"
#define TS_DEV_ADDR 0x48

#define ADDR_CHANGE_PROTOCOL	0x01
#define ADDR_LOGTYPE_ON			0x1B
#define ADDR_LOGTYPE_OFF		0x1C
#define ADDR_CHANGE_OPMODE		0x1D
#define ADDR_ENTER_LOGGING		0x5E
#define LOG_READ_ADDR			0x86

extern struct class *sec_class;

typedef enum {
	PTC_NONE = -1,
	PTC_TUNE,
	PTC_CALCSIMUL,
	PTC_TEST,
	PTC_PRIMITIVE,
	PTC_MTSI_1_2_0,
	PTC_STSI_1_0_0,
	PTC_LIM
} eProtocol_t;

typedef struct {
	char cCommand;
	char sDescription[100];
} tCommandInfo_t;

/*	LOG_HEADER */
typedef enum {
	HEADER_NONE = -1,
	HEADER_U08,
	HEADER_U16,
	HEADER_U32,
	HEADER_FINGER,
	HEADER_S08,
	HEADER_U16_NOCR,
	HEADER_S16,
	HEADER_S16_NOCR,
	HEADER_U32_NOCR,
	HEADER_S32,
	HEADER_S32_NOCR,
	HEADER_TEXT,
	HEADER_S12,
	HEADER_S12_NOCR,
	HEADER_S08_NOCR,
	HEADER_U08_NOCR,
	HEADER_U12,
	HEADER_U12_NOCR,
	HEADER_PRIVATE,
	HEADER_LIM
} eHeader_t;

typedef enum {
	LT_NONE = -1,
	LT_DIAGNOSIS_IMG,
	LT_RAW_IMG,
	LT_REF_IMG,
	LT_INTENSITY_IMG,
	LT_GROUP_IMG,
	LT_DELAY_IMG,
	LT_POS,
	LT_DEBUG,
	LT_DEBUG2,
	LT_PROFILING,
	LT_LIMIT
} eLogType_t;

typedef enum {
	OM_NONE = -1,
	OM_NORMAL_OPERATION,
	OM_TUNE_ANALOG_SETTINGS,
	OM_OPTIMIZE_DELAY,
	OM_NORMALIZE_INTENSITY,
	OM_HANDLER_TEST,
	OM_WAIT,
	OM_LIMIT
} eOperationMode_t;

struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *tsp_cb, int mode);
};

struct ts_platform_data {
	int gpio_read_done;
	int gpio_int;
	int gpio_touch_id;
	void (*power_on)(void);
	void (*power_off)(void);
	void (*register_cb)(struct tsp_callbacks *);
	void (*read_ta_status)(bool *);
	void (*set_touch_i2c)(void);
	void (*set_touch_i2c_to_gpio)(void);
};

#endif /* _LINUX_MELFAS_TS_H */
