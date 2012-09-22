/*
 * mms_ts.c - Touchscreen driver for Melfas MMS-series touch controllers
 *
 * Copyright (C) 2011 Google Inc.
 * Author: Dima Zavin <dima@android.com>
 *         Simon Wilson <simonwilson@google.com>
 *
 * ISP reflashing code based on original code from Melfas.
 * ISC reflashing code based on original code from Melfas.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#define SHOW_COORD		1
#define FW_UPDATABLE		1
#define ISC_DL_MODE		1
#define TOUCH_BOOSTER		1
#define SEC_TSP_FACTORY_TEST	1
/* #define ESD_DEBUG */

#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <mach/gpio.h>
#include <linux/uaccess.h>
#include <linux/platform_data/mms_ts_gc.h>
#include <asm/unaligned.h>
#include <linux/fb.h>
#if TOUCH_BOOSTER
#include <mach/cpufreq.h>
#include <mach/dev.h>
#endif

#define EVENT_SZ_8_BYTES	8
#define EVENT_SZ_6_BYTES	6
#define MAX_FINGERS		10
#define MAX_WIDTH		30
#define MAX_PRESSURE		255
#define MAX_ANGLE		90
#define MIN_ANGLE		-90

/* Registers */
#define MMS_INPUT_EVENT_PKT_SZ	0x0F
#define MMS_INPUT_EVENT0	0x10

#define MMS_TSP_REVISION	0xF0
#define MMS_HW_REVISION		0xF1
#define MMS_COMPAT_GROUP	0xF2
#define MMS_FW_VERSION		0xF3

#define MMS_TA_REG	0x60
#define MMS_TA_OFF	0x00
#define MMS_TA_ON	0x01
#define MMS_NOISE_REG	0x61
#define MMS_NOISE_OFF	0x00
#define MMS_NOISE_ON	0x01

#if FW_UPDATABLE
#include "GC_BOOT.h"
#endif

enum {
	TSP_STATE_RELEASE = 0,
	TSP_STATE_PRESS,
	TSP_STATE_MOVE,
};

#if TOUCH_BOOSTER
#define TOUCH_BOOSTER_CPU_CLK		800000
#define TOUCH_BOOSTER_BUS_CLK_266	267160
#define TOUCH_BOOSTER_BUS_CLK_400	400200
#define TOUCH_BOOSTER_OFF_TIME		100
#define TOUCH_BOOSTER_CHG_TIME		200
enum {
	TOUCH_BOOSTER_DELAY_OFF = 0,
	TOUCH_BOOSTER_ON,
	TOUCH_BOOSTER_QUICK_OFF,
};
#endif

#if ISC_DL_MODE	/* ISC_DL_MODE start */
char *isc_dl_msg;
char *isc_dl_msg_temp;
#define	ISC_DL_MSG(args ...)	\
do {				\
	sprintf(isc_dl_msg_temp, args);	\
	strcat(isc_dl_msg, isc_dl_msg_temp);	\
} while (0)

#define MAX_FW_PATH	255
#define FW_DIRECTORY	"tsp_melfas/gc/"

enum {
	BUILT_IN = 0,
	UMS,
	REQ_FW,
};

enum {
	COMPARE_UPDATE = 0,
	FORCED_UPDATE,
};

enum {
	PANEL_MOREENS	= 'A',
	PANEL_SMAC	= 'B',
	PANEL_SMAC_NEW	= 'C',
};

enum {
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
};

enum {
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
};

enum {
	SEC_NONE = -1,
	SEC_BOOTLOADER = 0,
	SEC_CORE,
	SEC_PRIVATE_CONFIG,
	SEC_PUBLIC_CONFIG,
	SEC_LIMIT
};

struct tISCFWInfo_t {
	unsigned char version;
	unsigned char compatible_version;
	unsigned char start_addr;
	unsigned char end_addr;
};

/*
 *      Default configuration of ISC mode
 */
#define DEFAULT_SLAVE_ADDR	0x48

#define SECTION_NUM		4
#define SECTION_NAME_LEN	5

#define PAGE_HEADER		3
#define PAGE_DATA		1024
#define PAGE_TAIL		2
#define PACKET_SIZE		(PAGE_HEADER + PAGE_DATA + PAGE_TAIL)
#define TS_WRITE_REGS_LEN		1030

#define TIMEOUT_CNT		10
#define STRING_BUF_LEN		100

/* State Registers */
#define MIP_ADDR_INPUT_INFORMATION	0x01
#define ISC_ADDR_VERSION		0xE1
#define ISC_ADDR_SECTION_PAGE_INFO	0xE5

/* Config Update Commands */
#define ISC_CMD_ENTER_ISC		0x5F
#define ISC_CMD_ENTER_ISC_PARA1		0x01
#define ISC_CMD_UPDATE_MODE		0xAE
#define ISC_SUBCMD_ENTER_UPDATE		0x55
#define ISC_SUBCMD_DATA_WRITE		0XF1
#define ISC_SUBCMD_LEAVE_UPDATE_PARA1	0x0F
#define ISC_SUBCMD_LEAVE_UPDATE_PARA2	0xF0
#define ISC_CMD_CONFIRM_STATUS		0xAF

#define ISC_STATUS_UPDATE_MODE		0x01
#define ISC_STATUS_CRC_CHECK_SUCCESS	0x03

#define ISC_CHAR_2_BCD(num)	(((num/10)<<4) + (num%10))
#define ISC_MAX(x, y)		(((x) > (y)) ? (x) : (y))

static const char section_name[SECTION_NUM][SECTION_NAME_LEN] = {
	"BOOT", "CORE", "PRIV", "PUBL"
};

static const unsigned char crc0_buf[31] = {
	0x1D, 0x2C, 0x05, 0x34, 0x95, 0xA4, 0x8D, 0xBC,
	0x59, 0x68, 0x41, 0x70, 0xD1, 0xE0, 0xC9, 0xF8,
	0x3F, 0x0E, 0x27, 0x16, 0xB7, 0x86, 0xAF, 0x9E,
	0x7B, 0x4A, 0x63, 0x52, 0xF3, 0xC2, 0xEB
};

static const unsigned char crc1_buf[31] = {
	0x1E, 0x9C, 0xDF, 0x5D, 0x76, 0xF4, 0xB7, 0x35,
	0x2A, 0xA8, 0xEB, 0x69, 0x42, 0xC0, 0x83, 0x01,
	0x04, 0x86, 0xC5, 0x47, 0x6C, 0xEE, 0xAD, 0x2F,
	0x30, 0xB2, 0xF1, 0x73, 0x58, 0xDA, 0x99
};

static struct tISCFWInfo_t mbin_info[SECTION_NUM]; /* F/W ver from File */
static struct tISCFWInfo_t ts_info[SECTION_NUM]; /* F/W ver from IC */
static bool section_update_flag[SECTION_NUM];
const struct firmware *fw_mbin[SECTION_NUM];
static unsigned char g_wr_buf[1024 + 3 + 2];

#endif	/* ISC_DL_MODE end */

enum {
	ISP_MODE_FLASH_ERASE = 0x59F3,
	ISP_MODE_FLASH_WRITE = 0x62CD,
	ISP_MODE_FLASH_READ = 0x6AC9,
};

/* each address addresses 4-byte words */
#define ISP_MAX_FW_SIZE		(0x1F00 * 4)
#define ISP_IC_INFO_ADDR	0x1F00

#if SEC_TSP_FACTORY_TEST
#define TSP_BUF_SIZE	1024

/* VSC(Vender Specific Command)  */
#define MMS_VSC_CMD			0xB0	/* vendor specific command */
#define MMS_VSC_MODE			0x1A	/* mode of vendor */

#define MMS_VSC_CMD_ENTER		0X01
#define MMS_VSC_CMD_CM_DELTA		0X02
#define MMS_VSC_CMD_CM_ABS		0X03
#define MMS_VSC_CMD_EXIT		0X05
#define MMS_VSC_CMD_INTENSITY		0X04
#define MMS_VSC_CMD_RAW			0X06
#define MMS_VSC_CMD_REFER		0X07

#define TSP_CMD_STR_LEN			32
#define TSP_CMD_RESULT_STR_LEN		512
#define TSP_CMD_PARAM_NUM		8

enum {
	FAIL_PWR_CONTROL = -1,
	SUCCESS_PWR_CONTROL = 0,
};

enum {	/* this is using by cmd_state valiable. */
	WAITING = 0,
	RUNNING,
	OK,
	FAIL,
	NOT_APPLICABLE,
};
#endif /* SEC_TSP_FACTORY_TEST */

struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *tsp_cb, bool mode);
};

struct mms_ts_info {
	struct i2c_client *client;
	struct input_dev *input_dev;
	char	phys[32];
	int	max_x;
	int	max_y;
	bool	invert_x;
	bool	invert_y;
	int	irq;
	int	(*power) (int on);
	void	(*input_event)(void *data);
	bool	enabled;
	u8	fw_ic_ver;
	u8	panel_type;
	int	finger_byte;
	const u8	*config_fw_version;
	unsigned char	finger_state[MAX_FINGERS];

	struct melfas_tsi_platform_data *pdata;
	struct early_suspend early_suspend;
	struct mutex lock;

	void (*register_cb)(void *);
	struct tsp_callbacks callbacks;
	bool	ta_status;
	bool	noise_mode;

#if TOUCH_BOOSTER
	struct delayed_work work_dvfs_off;
	struct delayed_work work_dvfs_chg;
	bool dvfs_lock_status;
	int cpufreq_level;
	struct mutex dvfs_lock;
	struct device *bus_dev;
	struct device *sec_touchscreen;
#endif	/* TOUCH_BOOSTER */

#if SEC_TSP_FACTORY_TEST
	struct list_head	cmd_list_head;
	u8			cmd_state;
	char			cmd[TSP_CMD_STR_LEN];
	int			cmd_param[TSP_CMD_PARAM_NUM];
	char			cmd_result[TSP_CMD_RESULT_STR_LEN];
	struct mutex		cmd_lock;
	bool			cmd_is_running;
	bool			ft_flag;
	unsigned int *reference;
	unsigned int *cm_abs;
	unsigned int *cm_delta;
	unsigned int *intensity;
#endif	/* SEC_TSP_FACTORY_TEST */
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mms_ts_early_suspend(struct early_suspend *h);
static void mms_ts_late_resume(struct early_suspend *h);
#endif

#if SEC_TSP_FACTORY_TEST
#define TSP_CMD(name, func) .cmd_name = name, .cmd_func = func

struct tsp_cmd {
	struct list_head	list;
	const char	*cmd_name;
	void	(*cmd_func)(void *device_data);
};

static void fw_update(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_config_ver(void *device_data);
static void get_threshold(void *device_data);
static void module_off_master(void *device_data);
static void module_on_master(void *device_data);
static void module_off_slave(void *device_data);
static void module_on_slave(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void get_reference(void *device_data);
static void get_cm_abs(void *device_data);
static void get_cm_delta(void *device_data);
static void get_intensity(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void run_reference_read(void *device_data);
static void run_cm_abs_read(void *device_data);
static void run_cm_delta_read(void *device_data);
static void run_intensity_read(void *device_data);
static void not_support_cmd(void *device_data);

struct tsp_cmd tsp_cmds[] = {
	{TSP_CMD("fw_update", fw_update),},
	{TSP_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{TSP_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{TSP_CMD("get_config_ver", get_config_ver),},
	{TSP_CMD("get_threshold", get_threshold),},
	{TSP_CMD("module_off_master", module_off_master),},
	{TSP_CMD("module_on_master", module_on_master),},
	{TSP_CMD("module_off_slave", not_support_cmd),},
	{TSP_CMD("module_on_slave", not_support_cmd),},
	{TSP_CMD("get_chip_vendor", get_chip_vendor),},
	{TSP_CMD("get_chip_name", get_chip_name),},
	{TSP_CMD("get_x_num", get_x_num),},
	{TSP_CMD("get_y_num", get_y_num),},
	{TSP_CMD("get_reference", get_reference),},
	{TSP_CMD("get_cm_abs", get_cm_abs),},
	{TSP_CMD("get_cm_delta", get_cm_delta),},
	{TSP_CMD("get_intensity", get_intensity),},
	{TSP_CMD("run_reference_read", run_reference_read),},
	{TSP_CMD("run_cm_abs_read", run_cm_abs_read),},
	{TSP_CMD("run_cm_delta_read", run_cm_delta_read),},
	{TSP_CMD("run_intensity_read", run_intensity_read),},
	{TSP_CMD("not_support_cmd", not_support_cmd),},
};
#endif

#if ISC_DL_MODE

u8 *tsp_firmware_file[4] = {0, };

static int mms100_i2c_read(struct i2c_client *client,
		u16 addr, u16 length, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg;
	int ret = -1;

	msg.addr = client->addr;
	msg.flags = 0x00;
	msg.len = 1;
	msg.buf = (u8 *) &addr;

	ret = i2c_transfer(adapter, &msg, 1);

	if (ret >= 0) {
		msg.addr = client->addr;
		msg.flags = I2C_M_RD;
		msg.len = length;
		msg.buf = (u8 *) value;

		ret = i2c_transfer(adapter, &msg, 1);
	}

	if (ret < 0)
		ISC_DL_MSG("[TSP ISC] i2c read error : [%d]\n", ret);

	return ret;
}


static int mms100_reset(struct i2c_client *client)
{
	struct mms_ts_info *info = i2c_get_clientdata(client);

	info->pdata->power(false);
	msleep(30);
	info->pdata->power(true);
	msleep(300);

	return ISC_SUCCESS;
}
/*
static int mms100_check_operating_mode(struct i2c_client *_client,
		const int _error_code)
{
	int ret;
	unsigned char rd_buf = 0x00;
	unsigned char count = 0;

	if(_client == NULL)
		pr_err("[TSP ISC] _client is null");

	ret = mms100_i2c_read(_client, ISC_ADDR_VERSION, 1, &rd_buf);

	if (ret<0) {
		pr_info("[TSP ISC] %s,%d: i2c read fail[%d]\n",
			__func__, __LINE__, ret);
		return _error_code;
	}

	return ISC_SUCCESS;
}
*/
static int mms100_get_version_info(struct i2c_client *_client)
{
	int i, ret;
	unsigned char rd_buf[8];

	/* config version brust read (core, private, public) */
	ret = mms100_i2c_read(_client, ISC_ADDR_VERSION, 4, rd_buf);

	if (ret < 0) {
		ISC_DL_MSG("[TSP ISC] %s,%d: i2c read fail[%d]\n",
			__func__, __LINE__, ret);
		return ISC_I2C_ERROR;
	}

	for (i = 0; i < SECTION_NUM; i++)
		ts_info[i].version = rd_buf[i];

	ts_info[SEC_CORE].compatible_version =
		ts_info[SEC_BOOTLOADER].version;
	ts_info[SEC_PRIVATE_CONFIG].compatible_version =
		ts_info[SEC_PUBLIC_CONFIG].compatible_version =
		ts_info[SEC_CORE].version;

	ret = mms100_i2c_read(_client, ISC_ADDR_SECTION_PAGE_INFO, 8, rd_buf);

	if (ret < 0) {
		ISC_DL_MSG("[TSP ISC] %s,%d: i2c read fail[%d]\n",
			__func__, __LINE__, ret);
		return ISC_I2C_ERROR;
	}

	for (i = 0; i < SECTION_NUM; i++) {
		ts_info[i].start_addr = rd_buf[i];
		ts_info[i].end_addr = rd_buf[i + SECTION_NUM];
	}

	for (i = 0; i < SECTION_NUM; i++)
		ISC_DL_MSG("[TSP ISC] IC   (%d): Ver[0x%02X] Addr[0x%02X]~[0x%02X] Compatibility[0x%02X]\n",
			i, ts_info[i].version, ts_info[i].start_addr,
			ts_info[i].end_addr, ts_info[i].compatible_version);

	return ISC_SUCCESS;
}

static int mms100_seek_section_info(void)
{
	int i;
	char str_buf[STRING_BUF_LEN];
	char name_buf[SECTION_NAME_LEN];
	int version;
	int page_num;

	const unsigned char *buf;
	int next_ptr;

	for (i = 0; i < SECTION_NUM; i++) {
		if (tsp_firmware_file[i] == NULL) {
			buf = NULL;
			ISC_DL_MSG("[TSP ISC] F/W file[%d] is NULL\n", i);
		} else
			buf = tsp_firmware_file[i];

		if (buf == NULL) {
			mbin_info[i].version = ts_info[i].version;
			mbin_info[i].compatible_version =
				ts_info[i].compatible_version;
			mbin_info[i].start_addr = ts_info[i].start_addr;
			mbin_info[i].end_addr = ts_info[i].end_addr;
		} else {
			next_ptr = 0;

			do {
				sscanf(buf + next_ptr, "%s", str_buf);
				next_ptr += strlen(str_buf) + 1;
			} while (!strstr(str_buf, "SECTION_NAME"));

			sscanf(buf + next_ptr, "%s%s", str_buf, name_buf);

			if (strncmp(section_name[i], name_buf,
				SECTION_NAME_LEN))
				return ISC_FILE_FORMAT_ERROR;

			do {
				sscanf(buf + next_ptr, "%s", str_buf);
				next_ptr += strlen(str_buf) + 1;
			} while (!strstr(str_buf, "SECTION_VERSION"));

			sscanf(buf + next_ptr, "%s%d", str_buf, &version);
			mbin_info[i].version = ISC_CHAR_2_BCD(version);

			do {
				sscanf(buf + next_ptr, "%s", str_buf);
				next_ptr += strlen(str_buf) + 1;
			} while (!strstr(str_buf, "START_PAGE_ADDR"));

			sscanf(buf + next_ptr, "%s%d", str_buf, &page_num);
			mbin_info[i].start_addr = page_num;

			do {
				sscanf(buf + next_ptr, "%s", str_buf);
				next_ptr += strlen(str_buf) + 1;
			} while (!strstr(str_buf, "END_PAGE_ADDR"));

			sscanf(buf + next_ptr, "%s%d", str_buf, &page_num);
			mbin_info[i].end_addr = page_num;

			do {
				sscanf(buf + next_ptr, "%s", str_buf);
				next_ptr += strlen(str_buf) + 1;
			} while (!strstr(str_buf, "COMPATIBLE_VERSION"));

			sscanf(buf + next_ptr, "%s%d", str_buf, &version);
			mbin_info[i].compatible_version =
				ISC_CHAR_2_BCD(version);

			do {
				sscanf(buf + next_ptr, "%s", str_buf);
				next_ptr += strlen(str_buf) + 1;
			} while (!strstr(str_buf, "[Binary]"));

			if (mbin_info[i].version == 0xFF)
				return ISC_FILE_FORMAT_ERROR;
		}
	}

	for (i = 0; i < SECTION_NUM; i++)
		ISC_DL_MSG("[TSP ISC] mBin (%d): Ver[0x%02X] Addr[0x%02X]~[0x%02X] Compatibility[0x%02X]\n",
			i, mbin_info[i].version, mbin_info[i].start_addr,
			mbin_info[i].end_addr, mbin_info[i].compatible_version);

	return ISC_SUCCESS;
}

static int mms100_compare_version_info(struct i2c_client *_client,
							bool forced_update)
{
	int i, ret;
	unsigned char expected_compatibility[SECTION_NUM];

	if (mms100_get_version_info(_client) != ISC_SUCCESS)
		return ISC_I2C_ERROR;

	ret = mms100_seek_section_info();
	if (ret)
		return ret;

	section_update_flag[0] = false;
	/* Check update areas , 0 : bootloader 1: core 2: private 3: public */
	for (i = SEC_CORE; i < SECTION_NUM; i++) {
		if ((mbin_info[i].version == 0) ||
			(mbin_info[i].version > ts_info[i].version))
			section_update_flag[i] = true;
	}
	if (forced_update) {
		section_update_flag[SEC_CORE] = true;
		section_update_flag[SEC_PRIVATE_CONFIG] = true;
		section_update_flag[SEC_PUBLIC_CONFIG] = true;
		ISC_DL_MSG("[TSP ISC] forced_update enable!\n");
	}
	ISC_DL_MSG("[TSP ISC] Update_flag : Core[%d] PRIV[%d] PUBL[%d]\n",
			section_update_flag[1], section_update_flag[2],
			section_update_flag[3]);

	if (section_update_flag[SEC_BOOTLOADER]) {
		expected_compatibility[SEC_CORE] =
		mbin_info[SEC_BOOTLOADER].version;
	} else {
		expected_compatibility[SEC_CORE] =
		ts_info[SEC_BOOTLOADER].version;
	}

	if (section_update_flag[SEC_CORE]) {
		expected_compatibility[SEC_PUBLIC_CONFIG] =
		expected_compatibility[SEC_PRIVATE_CONFIG] =
		mbin_info[SEC_CORE].version;
	} else {
		expected_compatibility[SEC_PUBLIC_CONFIG] =
		expected_compatibility[SEC_PRIVATE_CONFIG] =
		ts_info[SEC_CORE].version;
	}

	for (i = SEC_CORE; i <= SEC_PUBLIC_CONFIG; i++) {
		if (expected_compatibility[i] != ts_info[i].compatible_version
			&& !forced_update) {
			ISC_DL_MSG("[TSP ISC] compatible error(%d)- expected:[0x%02x] mbin:[0x%02x]\n",
					i, expected_compatibility[i],
					mbin_info[i].compatible_version);
			return ISC_COMPATIVILITY_ERROR;
		}
	}
	return ISC_SUCCESS;
}

static int mms100_enter_ISC_mode(struct i2c_client *_client)
{
	int ret;
	unsigned char wr_buf[2];

	wr_buf[0] = ISC_CMD_ENTER_ISC;
	wr_buf[1] = ISC_CMD_ENTER_ISC_PARA1;

	ret = i2c_master_send(_client, wr_buf, 2);

	if (ret < 0) {
		ISC_DL_MSG("[TSP ISC] %s,%d: i2c write fail[%d]\n",
			__func__, __LINE__, ret);
		return ISC_I2C_ERROR;
	}

	msleep(50);

	return ISC_SUCCESS;
}

static int mms100_enter_config_update(struct i2c_client *_client)
{
	int ret;
	unsigned char wr_buf[10] = {0,};
	unsigned char rd_buf;

	wr_buf[0] = ISC_CMD_UPDATE_MODE;
	wr_buf[1] = ISC_SUBCMD_ENTER_UPDATE;

	ret = i2c_master_send(_client, wr_buf, 10);

	if (ret < 0) {
		ISC_DL_MSG("[TSP ISC] %s,%d: i2c write fail[%d]\n",
			__func__, __LINE__, ret);
		return ISC_I2C_ERROR;
	}

	ret = mms100_i2c_read(_client, ISC_CMD_CONFIRM_STATUS, 1, &rd_buf);
	if (ret < 0) {
		ISC_DL_MSG("[TSP ISC] %s,%d: i2c read fail[%d]\n",
			__func__, __LINE__, ret);
		return ISC_I2C_ERROR;
	}

	if (rd_buf != ISC_STATUS_UPDATE_MODE)
		return ISC_UPDATE_MODE_ENTER_ERROR;

	return ISC_SUCCESS;
}

static int mms100_ISC_clear_page(struct i2c_client *_client,
		unsigned char _page_addr)
{
	int ret;
	unsigned char rd_buf;

	memset(&g_wr_buf[3], 0xFF, PAGE_DATA);

	g_wr_buf[0] = ISC_CMD_UPDATE_MODE;	/* command */
	g_wr_buf[1] = ISC_SUBCMD_DATA_WRITE;	/* sub_command */
	g_wr_buf[2] = _page_addr;

	g_wr_buf[PAGE_HEADER + PAGE_DATA] = crc0_buf[_page_addr];
	g_wr_buf[PAGE_HEADER + PAGE_DATA + 1] = crc1_buf[_page_addr];

	ret = i2c_master_send(_client, g_wr_buf, PACKET_SIZE);

	if (ret < 0) {
		ISC_DL_MSG("[TSP ISC] %s,%d: i2c write fail[%d]\n",
			__func__, __LINE__, ret);
		return ISC_I2C_ERROR;
	}

	ret = mms100_i2c_read(_client, ISC_CMD_CONFIRM_STATUS, 1, &rd_buf);

	if (ret < 0) {
		ISC_DL_MSG("[TSP ISC] %s,%d: i2c read fail[%d]\n",
			__func__, __LINE__, ret);
		return ISC_I2C_ERROR;
	}

	if (rd_buf != ISC_STATUS_CRC_CHECK_SUCCESS)
		return ISC_UPDATE_MODE_ENTER_ERROR;

	return ISC_SUCCESS;
}

static int mms100_ISC_clear_validate_markers(struct i2c_client *_client)
{
	int ret_msg;
	int i, j;
	bool is_matched_address;

	for (i = SEC_CORE; i <= SEC_PUBLIC_CONFIG; i++) {
		if (section_update_flag[i]) {
			if (ts_info[i].end_addr <= 30 &&
				ts_info[i].end_addr > 0) {
				ret_msg = mms100_ISC_clear_page(_client,
					ts_info[i].end_addr);

				if (ret_msg != ISC_SUCCESS)
					return ret_msg;
			}
		}
	}

	for (i = SEC_CORE; i <= SEC_PUBLIC_CONFIG; i++) {
		if (section_update_flag[i]) {
			is_matched_address = false;
			for (j = SEC_CORE; j <= SEC_PUBLIC_CONFIG; j++) {
				if (mbin_info[i].end_addr ==
					ts_info[i].end_addr) {
					is_matched_address = true;
					break;
				}
			}

			if (!is_matched_address) {
				if (mbin_info[i].end_addr <= 30 &&
					mbin_info[i].end_addr > 0) {
					ret_msg = mms100_ISC_clear_page(_client,
						mbin_info[i].end_addr);

				if (ret_msg != ISC_SUCCESS)
					return ret_msg;
				}
			}
		}
	}

	return ISC_SUCCESS;
}

static int mms100_update_section_data(struct i2c_client *_client)
{
	int i, ret, next_ptr;
	unsigned char rd_buf;
	const unsigned char *ptr_fw;
	char str_buf[STRING_BUF_LEN];
	int page_addr;

	for (i = 0; i < SECTION_NUM; i++) {
		if (section_update_flag[i]) {
			ISC_DL_MSG("[TSP ISC] %d section data i2c flash\n", i);

			next_ptr = 0;
			ptr_fw = tsp_firmware_file[i];

			do {
				sscanf(ptr_fw + next_ptr, "%s", str_buf);
				next_ptr += strlen(str_buf) + 1;
			} while (!strstr(str_buf, "[Binary]"));
			ptr_fw = ptr_fw + next_ptr + 2;

			for (page_addr = mbin_info[i].start_addr;
				page_addr <= mbin_info[i].end_addr;
				page_addr++) {
				if (page_addr - mbin_info[i].start_addr > 0)
					ptr_fw += PACKET_SIZE;

				if ((ptr_fw[0] != ISC_CMD_UPDATE_MODE) ||
					(ptr_fw[1] != ISC_SUBCMD_DATA_WRITE) ||
						(ptr_fw[2] != page_addr))
					return ISC_WRITE_BUFFER_ERROR;

				ret = i2c_master_send(_client,
					ptr_fw, PACKET_SIZE);
				if (ret < 0) {
					ISC_DL_MSG("[TSP ISC] %s,%d: i2c write fail[%d]\n",
						__func__, __LINE__, ret);
					return ISC_I2C_ERROR;
				}

				ret = mms100_i2c_read(_client,
					ISC_CMD_CONFIRM_STATUS, 1, &rd_buf);
				if (ret < 0) {
					ISC_DL_MSG("[TSP ISC] %s,%d: i2c read fail[%d]\n",
						__func__, __LINE__, ret);
					return ISC_I2C_ERROR;
				}

				if (rd_buf != ISC_STATUS_CRC_CHECK_SUCCESS)
					return ISC_CRC_ERROR;

				section_update_flag[i] = false;
			}
		}
	}
	return ISC_SUCCESS;
}

static int mms100_open_mbinary(struct i2c_client *_client,
						int panel_type, int mode)
{
	int i;
	int ret = 0;
	mm_segment_t old_fs = {0};
	struct file *fp = NULL;
	long fsize = 0, nread = 0;
	char fw_path[MAX_FW_PATH+1];

	if (mode == REQ_FW) {
		for (i = SEC_CORE; i <= SEC_PUBLIC_CONFIG ; i++) {
			snprintf(fw_path, MAX_FW_PATH, "%s%c_%s.fw",
				FW_DIRECTORY, panel_type, section_name[i]);
			ret = request_firmware(&(fw_mbin[i]), fw_path,
								&_client->dev);
			if (ret) {
				ISC_DL_MSG("[TSP ISC] fail REQ_FW[%s]\n",
								fw_path);
				break;
			}

			tsp_firmware_file[i] = kzalloc((size_t)fw_mbin[i]->size,
							GFP_KERNEL);

			if (!tsp_firmware_file[i])
				ISC_DL_MSG("[TSP ISC] fail to alloc buffer for fw\n");
			else
				memcpy((void *)tsp_firmware_file[i],
					fw_mbin[i]->data, fw_mbin[i]->size);

			if (fw_mbin[i] != NULL)
				release_firmware(fw_mbin[i]);
		}
		if (!ret)
			ISC_DL_MSG("[TSP ISC] All REQ_FW is loaded!\n");
	} else if (mode == UMS) {

		old_fs = get_fs();
		set_fs(get_ds());

		for (i = SEC_CORE ; i <= SEC_PUBLIC_CONFIG ; i++) {
			snprintf(fw_path, MAX_FW_PATH, "/sdcard/%s.mbin",
							section_name[i]);
			fp = filp_open(fw_path, O_RDONLY, 0);
			if (IS_ERR(fp)) {
				ISC_DL_MSG("[TSP ISC] file %s open error:%d\n",
						fw_path, (s32)fp);
				set_fs(old_fs);
				return ISC_FILE_OPEN_ERROR;
			}

			fsize = fp->f_path.dentry->d_inode->i_size;

			tsp_firmware_file[i] = kzalloc((size_t)fsize,
							GFP_KERNEL);
			if (!tsp_firmware_file[i]) {
				ISC_DL_MSG("[TSP ISC] fail to alloc buffer for fw\n");
				ret = ISC_FILE_OPEN_ERROR;
			}

			nread = vfs_read(fp,
					(char __user *)tsp_firmware_file[i],
					fsize, &fp->f_pos);
			if (nread != fsize) {
				ISC_DL_MSG("[TSP ISC] nread != fsize error\n");
				ret = ISC_FILE_OPEN_ERROR;
			}

			filp_close(fp, current->files);
		}

		set_fs(old_fs);
		if (!ret)
			ISC_DL_MSG("[TSP ISC] ums fw is loaded!\n");
	} else {
		ISC_DL_MSG("[TSP ISC] Not support mode[%d]\n", mode);
		ret = ISC_FILE_OPEN_ERROR;
	}

	if (!ret)
		return ISC_SUCCESS;
	else {
		ISC_DL_MSG("[TSP ISC] mms100_open_mbinary fail\n");
		return ret;
	}
}

int mms100_ISC_download_mbinary(struct i2c_client *_client, int mode,
					int panel_type, bool forced_update)
{
	int ret_msg = ISC_NONE;
	isc_dl_msg = kzalloc(sizeof(char) * 8096 , GFP_KERNEL);
	isc_dl_msg_temp = kzalloc(sizeof(char) * 256 , GFP_KERNEL);

	if (isc_dl_msg == NULL || isc_dl_msg_temp == NULL) {
		pr_err("isc_dl_msg = kzalloc error!");
		return ISC_NONE;
	}

	pr_info("[TSP ISC] FIRMWARE_UPDATE_START!");

/*
	mms100_reset(_client);
	ret_msg = mms100_check_operating_mode(_client, EC_BOOT_ON_SUCCEEDED);
	if (ret_msg != ISC_SUCCESS)
		goto ISC_ERROR_HANDLE;
*/
	ret_msg = mms100_open_mbinary(_client, panel_type, mode);
	if (ret_msg != ISC_SUCCESS)
		goto ISC_ERROR_HANDLE;

	/*Config version Check*/
	ret_msg = mms100_compare_version_info(_client, forced_update);
	if (ret_msg != ISC_SUCCESS)
		goto ISC_ERROR_HANDLE;
	else if (!section_update_flag[1] && !section_update_flag[2]
						&& !section_update_flag[3]) {
		pr_info("[TSP ISC] FIRMWARE_UPDATE SKIP!");
		goto ISC_NEED_NOT_UPDATE;
	}

	ret_msg = mms100_enter_ISC_mode(_client);
	if (ret_msg != ISC_SUCCESS)
		goto ISC_ERROR_HANDLE;

	ret_msg = mms100_enter_config_update(_client);
	if (ret_msg != ISC_SUCCESS)
		goto ISC_ERROR_HANDLE;

	ret_msg = mms100_ISC_clear_validate_markers(_client);
	if (ret_msg != ISC_SUCCESS)
		goto ISC_ERROR_HANDLE;

	ret_msg = mms100_update_section_data(_client);
	if (ret_msg != ISC_SUCCESS)
		goto ISC_ERROR_HANDLE;

	ret_msg = ISC_SUCCESS;

ISC_ERROR_HANDLE:
	mms100_reset(_client);
	if (ret_msg != ISC_SUCCESS) {
		pr_err("[TSP ISC] FW update fail message start!");
		pr_err("%s", isc_dl_msg);
		pr_err("[TSP ISC] FW update fail message END!");
		pr_err("[TSP ISC] FIRMWARE_UPDATE ERROR : %d\n", ret_msg);
	} else
		pr_info("[TSP ISC] FIRMWARE_UPDATE SUCCESS!");

ISC_NEED_NOT_UPDATE:

	kfree(isc_dl_msg);
	kfree(isc_dl_msg_temp);

	if (ret_msg != ISC_FILE_OPEN_ERROR) {
		kfree(tsp_firmware_file[1]);
		kfree(tsp_firmware_file[2]);
		kfree(tsp_firmware_file[3]);
	}

	return ret_msg;
}
#endif	/* ISC_DL_MODE */


#if TOUCH_BOOSTER
static void change_dvfs_lock(struct work_struct *work)
{
	struct mms_ts_info *info = container_of(work,
				struct mms_ts_info, work_dvfs_chg.work);
	int ret;

	mutex_lock(&info->dvfs_lock);
	ret = dev_lock(info->bus_dev, info->sec_touchscreen,
						TOUCH_BOOSTER_BUS_CLK_266);

	if (ret < 0)
		dev_err(&info->client->dev,
					"%s dev change bud lock failed(%d)\n",\
					__func__, __LINE__);
	else
		dev_notice(&info->client->dev, "Change dvfs lock");
	mutex_unlock(&info->dvfs_lock);
}
static void set_dvfs_off(struct work_struct *work)
{

	struct mms_ts_info *info = container_of(work,
				struct mms_ts_info, work_dvfs_off.work);
	int ret;

	mutex_lock(&info->dvfs_lock);

	ret = dev_unlock(info->bus_dev, info->sec_touchscreen);
	if (ret < 0)
		dev_err(&info->client->dev, " %s: dev unlock failed(%d)\n",
							__func__, __LINE__);

	exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
	info->dvfs_lock_status = false;
	dev_notice(&info->client->dev, "dvfs off!");
	mutex_unlock(&info->dvfs_lock);
}

static void set_dvfs_lock(struct mms_ts_info *info, uint32_t mode)
{
	int ret;

	mutex_lock(&info->dvfs_lock);
	if (info->cpufreq_level <= 0) {
		ret = exynos_cpufreq_get_level(TOUCH_BOOSTER_CPU_CLK,
						&info->cpufreq_level);
		if (ret < 0)
			dev_err(&info->client->dev,
					"exynos_cpufreq_get_level error");
		goto out;
	}

	if (mode == TOUCH_BOOSTER_DELAY_OFF) {
		if (info->dvfs_lock_status) {
			cancel_delayed_work(&info->work_dvfs_chg);
			schedule_delayed_work(&info->work_dvfs_off,
				msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));
		}

	} else if (mode == TOUCH_BOOSTER_ON) {
		cancel_delayed_work(&info->work_dvfs_off);
		if (!info->dvfs_lock_status) {
			ret = dev_lock(info->bus_dev, info->sec_touchscreen,
						TOUCH_BOOSTER_BUS_CLK_400);
			if (ret < 0) {
				dev_err(&info->client->dev,
						"%s: dev lock failed(%d)",
							__func__, __LINE__);
			}

			ret = exynos_cpufreq_lock(DVFS_LOCK_ID_TSP,
							info->cpufreq_level);
			if (ret < 0)
				dev_err(&info->client->dev,
						"%s: cpu lock failed(%d)",
						__func__, __LINE__);

			schedule_delayed_work(&info->work_dvfs_chg,
				msecs_to_jiffies(TOUCH_BOOSTER_CHG_TIME));

			info->dvfs_lock_status = true;
			dev_notice(&info->client->dev, "dvfs on[%d]",
							info->cpufreq_level);
		}
	} else if (mode == TOUCH_BOOSTER_QUICK_OFF) {
		cancel_delayed_work(&info->work_dvfs_off);
		cancel_delayed_work(&info->work_dvfs_chg);
		schedule_work(&info->work_dvfs_off.work);
	}
out:
	mutex_unlock(&info->dvfs_lock);
}
#endif

static void release_all_fingers(struct mms_ts_info *info)
{
	int i;

	dev_notice(&info->client->dev, "%s\n", __func__);

	for (i = 0; i < MAX_FINGERS; i++) {
		input_mt_slot(info->input_dev, i);
		input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER,
					   false);

		if (info->finger_state[i] != TSP_STATE_RELEASE) {
			dev_notice(&info->client->dev,
						"finger %d up(force)\n", i);
		}
		info->finger_state[i] = TSP_STATE_RELEASE;
	}
	input_sync(info->input_dev);

#if TOUCH_BOOSTER
	set_dvfs_lock(info, TOUCH_BOOSTER_QUICK_OFF);
	dev_notice(&info->client->dev, "dvfs lock free.\n");
#endif
}

static void reset_mms_ts(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;

	dev_notice(&client->dev, "%s called, tsp state [%s]!\n",
			__func__, info->enabled ? "enable" : "disable");
	if (info->enabled == false)
		return;

	disable_irq_nosync(info->irq);
	info->enabled = false;

	info->pdata->power(0);
	msleep(30);

	release_all_fingers(info);

	info->pdata->power(1);
	msleep(120);

	if (info->ta_status) {
		dev_notice(&client->dev, "TA or USB connect!!!\n");
		i2c_smbus_write_byte_data(info->client, MMS_TA_REG, MMS_TA_ON);

		if (info->noise_mode) {
			i2c_smbus_write_byte_data(info->client, MMS_NOISE_REG,
								MMS_NOISE_ON);
			dev_notice(&client->dev, "reset & noise mode on!\n");
		}
	} else
		info->noise_mode = false;

	enable_irq(info->irq);
	info->enabled = true;
}

static void melfas_ta_cb(struct tsp_callbacks *cb, bool ta_status)
{
	struct mms_ts_info *info =
			container_of(cb, struct mms_ts_info, callbacks);
	struct i2c_client *client = info->client;

	dev_notice(&client->dev, "%s TA or USB %sconnect\n", __func__,
				ta_status ? "" : "dis");

	info->ta_status = ta_status;

	if (info->enabled) {
		if (info->ta_status)
			i2c_smbus_write_byte_data(info->client,
							MMS_TA_REG, MMS_TA_ON);
		else {
			i2c_smbus_write_byte_data(info->client,
							MMS_TA_REG, MMS_TA_OFF);

			if (info->noise_mode) {
				info->noise_mode = false;
				i2c_smbus_write_byte_data(info->client,
								MMS_NOISE_REG,
								MMS_NOISE_OFF);
				dev_notice(&client->dev,
						"ta_cb & noise mode off!\n");
			}
		}
	}
}

static irqreturn_t mms_ts_interrupt(int irq, void *dev_id)
{
	struct mms_ts_info *info = dev_id;
	struct i2c_client *client = info->client;
	u8 buf[MAX_FINGERS * EVENT_SZ_8_BYTES] = { 0, };
	int ret, i, sz;
	int id, state, posX, posY, strenth, width;
	int angle, palm, major_axis, minor_axis;
	int finger_event_sz;
	u8 *read_data;
	u8 reg = MMS_INPUT_EVENT0;
#if TOUCH_BOOSTER
	bool press_flag = false;
#endif
	struct i2c_msg msg[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .buf = &reg,
		 .len = 1,
		 }, {
		     .addr = client->addr,
		     .flags = I2C_M_RD,
		     .buf = buf,
		     },
	};
	finger_event_sz = info->finger_byte;

	sz = i2c_smbus_read_byte_data(client, MMS_INPUT_EVENT_PKT_SZ);

	if (sz < 0) {
		dev_err(&client->dev, "%s bytes=%d\n", __func__, sz);
		for (i = 0; i < 50; i++) {
			sz = i2c_smbus_read_byte_data(client,
						      MMS_INPUT_EVENT_PKT_SZ);
			if (sz > 0)
				break;
		}

		if (i == 50) {
			dev_dbg(&client->dev, "i2c failed... reset!!\n");
			reset_mms_ts(info);
			return IRQ_HANDLED;
		}
		dev_err(&client->dev, "success read touch info data\n");
	}
	if (sz == 0)
		return IRQ_HANDLED;

	if (sz > MAX_FINGERS*finger_event_sz || sz%finger_event_sz) {
		dev_err(&client->dev, "abnormal data inputed & reset IC[%d]\n",
									sz);
		reset_mms_ts(info);
		return IRQ_HANDLED;
	}

	msg[1].len = sz;
	ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));

	if (ret != ARRAY_SIZE(msg)) {
		dev_err(&client->dev,
			"failed to read %d bytes of touch data (%d)\n",
			sz, ret);

		for (i = 0; i < 5; i++) {
			ret = i2c_transfer(client->adapter, msg,
							ARRAY_SIZE(msg));
			if (ret == ARRAY_SIZE(msg))
				break;
		}

		if (i == 5) {
			dev_err(&client->dev,
				"failed to read touch data & reset IC[%d]\n",
									ret);
			reset_mms_ts(info);
			return IRQ_HANDLED;
		}
		dev_err(&client->dev, "success read touch data\n");
	}

	if (buf[0] == 0x0F) {	/* ESD */
		dev_dbg(&client->dev, "ESD DETECT.... reset!!\n");
		reset_mms_ts(info);
		return IRQ_HANDLED;
	}

	if (buf[0] == 0x0E) { /* NOISE MODE */
		dev_dbg(&client->dev, "Noise mode enter!!\n");

		info->noise_mode = true;
		i2c_smbus_write_byte_data(info->client, MMS_NOISE_REG,
							MMS_NOISE_ON);
		dev_notice(&client->dev, "interrupt & noise mode on!\n");
		return IRQ_HANDLED;
	}

	for (i = 0; i < sz; i += finger_event_sz) {
		read_data = &buf[i];
		id = (read_data[0] & 0xf) - 1;
		state = read_data[0] & 0x80;
		posX = read_data[2] | ((read_data[1] & 0xf) << 8);
		posY = read_data[3] | (((read_data[1] >> 4) & 0xf) << 8);
		strenth = read_data[4];

		if (finger_event_sz == EVENT_SZ_8_BYTES) {
			angle = (read_data[5] >= 127) ? \
				(-(256 - read_data[5])) : read_data[5];
			palm = (read_data[0] & 0x10) >> 4;
			major_axis = read_data[6];
			minor_axis = read_data[7];

		} else
			width = read_data[5];

		if (info->invert_x) {
			posX = info->max_x - posX;
			if (posX < 0)
				posX = 0;
		}
		if (info->invert_y) {
			posY = info->max_y - posY;
			if (posY < 0)
				posY = 0;
		}
		if (id >= MAX_FINGERS) {
			dev_notice(&client->dev, \
				"finger id error [%d]\n", id);
			reset_mms_ts(info);
			return IRQ_HANDLED;
		}

		if (state == TSP_STATE_RELEASE) {
			input_mt_slot(info->input_dev, id);
			input_mt_report_slot_state(info->input_dev,
						   MT_TOOL_FINGER, false);
#if SHOW_COORD
			if (finger_event_sz == EVENT_SZ_8_BYTES)
				dev_notice(&client->dev,
				"R [%2d],([%4d],[%3d])[%d][%d]",
				id, posX, posY,	palm, info->finger_state[id]);
			else
				dev_notice(&client->dev,
				"R [%2d],([%4d],[%3d])[%d]",
				id, posX, posY,	info->finger_state[id]);

#else
			dev_notice(&client->dev, "R [%2d][%d]", id,
					info->finger_state[id]);
#endif
			info->finger_state[id] = TSP_STATE_RELEASE;
		} else {
			input_mt_slot(info->input_dev, id);
			input_mt_report_slot_state(info->input_dev,
						   MT_TOOL_FINGER, true);
			input_report_abs(info->input_dev,
					ABS_MT_POSITION_X, posX);
			input_report_abs(info->input_dev,
					ABS_MT_POSITION_Y, posY);

			if (finger_event_sz == EVENT_SZ_8_BYTES) {
				input_report_abs(info->input_dev,
						ABS_MT_WIDTH_MAJOR, strenth);
				input_report_abs(info->input_dev,
						ABS_MT_TOUCH_MAJOR, major_axis);
				input_report_abs(info->input_dev,
						ABS_MT_TOUCH_MINOR, minor_axis);
				input_report_abs(info->input_dev,
						ABS_MT_ANGLE, angle);
				input_report_abs(info->input_dev,
						ABS_MT_PALM, palm);
			} else {
				input_report_abs(info->input_dev,
						ABS_MT_TOUCH_MAJOR, strenth);
				input_report_abs(info->input_dev,
						ABS_MT_WIDTH_MAJOR, width);
			}

			if (info->finger_state[id] == TSP_STATE_RELEASE) {
				info->finger_state[id] = TSP_STATE_PRESS;
#if SHOW_COORD
				if (finger_event_sz == EVENT_SZ_8_BYTES)
					dev_notice(&client->dev,
		"P [%2d],([%4d],[%3d]),S:%d A:%3d P:%d Mj_a:%d Mi_a:%d",\
						id, posX, posY, strenth, angle,
						palm, major_axis, minor_axis);
				else
					dev_notice(&client->dev,
		"P [%2d],([%4d],[%3d]),S:%d W:%d",
						id, posX, posY, strenth, width);
#else
				dev_notice(&client->dev, "P [%2d]", id);
#endif
			} else
				info->finger_state[id] = TSP_STATE_MOVE;
		}
	}
	input_sync(info->input_dev);

#if TOUCH_BOOSTER
	for (i = 0 ; i < MAX_FINGERS ; i++) {
		if (info->finger_state[i] == TSP_STATE_PRESS
			|| info->finger_state[i] == TSP_STATE_MOVE) {
			press_flag = TOUCH_BOOSTER_ON;
			break;
		}
	}

	set_dvfs_lock(info, press_flag);
#endif

	return IRQ_HANDLED;
}

static void hw_reboot(struct mms_ts_info *info, bool bootloader)
{
	info->pdata->power(0);
	gpio_direction_output(info->pdata->gpio_sda, bootloader ? 0 : 1);
	gpio_direction_output(info->pdata->gpio_scl, bootloader ? 0 : 1);
	gpio_direction_output(info->pdata->gpio_int, 0);
	msleep(30);
	info->pdata->power(1);
	msleep(30);

	if (bootloader) {
		gpio_set_value(info->pdata->gpio_scl, 0);
		gpio_set_value(info->pdata->gpio_sda, 1);
	} else {
		gpio_set_value(info->pdata->gpio_int, 1);
		gpio_direction_input(info->pdata->gpio_int);
		gpio_direction_input(info->pdata->gpio_scl);
		gpio_direction_input(info->pdata->gpio_sda);
	}
	msleep(40);
}

static inline void hw_reboot_bootloader(struct mms_ts_info *info)
{
	hw_reboot(info, true);
}

static inline void hw_reboot_normal(struct mms_ts_info *info)
{
	hw_reboot(info, false);
}

static void isp_toggle_clk(struct mms_ts_info *info, int start_lvl, int end_lvl,
			   int hold_us)
{
	gpio_set_value(info->pdata->gpio_scl, start_lvl);
	udelay(hold_us);
	gpio_set_value(info->pdata->gpio_scl, end_lvl);
	udelay(hold_us);
}

/* 1 <= cnt <= 32 bits to write */
static void isp_send_bits(struct mms_ts_info *info, u32 data, int cnt)
{
	gpio_direction_output(info->pdata->gpio_int, 0);
	gpio_direction_output(info->pdata->gpio_scl, 0);
	gpio_direction_output(info->pdata->gpio_sda, 0);

	/* clock out the bits, msb first */
	while (cnt--) {
		gpio_set_value(info->pdata->gpio_sda, (data >> cnt) & 1);
		udelay(3);
		isp_toggle_clk(info, 1, 0, 3);
	}
}

/* 1 <= cnt <= 32 bits to read */
static u32 isp_recv_bits(struct mms_ts_info *info, int cnt)
{
	u32 data = 0;

	gpio_direction_output(info->pdata->gpio_int, 0);
	gpio_direction_output(info->pdata->gpio_scl, 0);
	gpio_set_value(info->pdata->gpio_sda, 0);
	gpio_direction_input(info->pdata->gpio_sda);

	/* clock in the bits, msb first */
	while (cnt--) {
		isp_toggle_clk(info, 0, 1, 1);
		data = (data << 1) | (!!gpio_get_value(info->pdata->gpio_sda));
	}

	gpio_direction_output(info->pdata->gpio_sda, 0);
	return data;
}

static void isp_enter_mode(struct mms_ts_info *info, u32 mode)
{
	int cnt;
	unsigned long flags;

	local_irq_save(flags);
	gpio_direction_output(info->pdata->gpio_int, 0);
	gpio_direction_output(info->pdata->gpio_scl, 0);
	gpio_direction_output(info->pdata->gpio_sda, 1);

	mode &= 0xffff;
	for (cnt = 15; cnt >= 0; cnt--) {
		gpio_set_value(info->pdata->gpio_int, (mode >> cnt) & 1);
		udelay(3);
		isp_toggle_clk(info, 1, 0, 3);
	}

	gpio_set_value(info->pdata->gpio_int, 0);
	local_irq_restore(flags);
}

static void isp_exit_mode(struct mms_ts_info *info)
{
	int i;
	unsigned long flags;

	local_irq_save(flags);
	gpio_direction_output(info->pdata->gpio_int, 0);
	udelay(3);

	for (i = 0; i < 10; i++)
		isp_toggle_clk(info, 1, 0, 3);
	local_irq_restore(flags);
}

static void flash_set_address(struct mms_ts_info *info, u16 addr)
{
	/* Only 13 bits of addr are valid.
	 * The addr is in bits 13:1 of cmd */
	isp_send_bits(info, (u32) (addr & 0x1fff) << 1, 18);
}

static void flash_erase(struct mms_ts_info *info)
{
	isp_enter_mode(info, ISP_MODE_FLASH_ERASE);

	gpio_direction_output(info->pdata->gpio_int, 0);
	gpio_direction_output(info->pdata->gpio_scl, 0);
	gpio_direction_output(info->pdata->gpio_sda, 1);

	/* 4 clock cycles with different timings for the erase to
	 * get processed, clk is already 0 from above */
	udelay(7);
	isp_toggle_clk(info, 1, 0, 3);
	udelay(7);
	isp_toggle_clk(info, 1, 0, 3);
	usleep_range(25000, 35000);
	isp_toggle_clk(info, 1, 0, 3);
	usleep_range(150, 200);
	isp_toggle_clk(info, 1, 0, 3);

	gpio_set_value(info->pdata->gpio_sda, 0);

	isp_exit_mode(info);
}

static u32 flash_readl(struct mms_ts_info *info, u16 addr)
{
	int i;
	u32 val;
	unsigned long flags;

	local_irq_save(flags);
	isp_enter_mode(info, ISP_MODE_FLASH_READ);
	flash_set_address(info, addr);

	gpio_direction_output(info->pdata->gpio_scl, 0);
	gpio_direction_output(info->pdata->gpio_sda, 0);
	udelay(40);

	/* data load cycle */
	for (i = 0; i < 6; i++)
		isp_toggle_clk(info, 1, 0, 10);

	val = isp_recv_bits(info, 32);
	isp_exit_mode(info);
	local_irq_restore(flags);

	return val;
}

static void flash_writel(struct mms_ts_info *info, u16 addr, u32 val)
{
	unsigned long flags;

	local_irq_save(flags);
	isp_enter_mode(info, ISP_MODE_FLASH_WRITE);
	flash_set_address(info, addr);
	isp_send_bits(info, val, 32);

	gpio_direction_output(info->pdata->gpio_sda, 1);
	/* 6 clock cycles with different timings for the data to get written
	 * into flash */
	isp_toggle_clk(info, 0, 1, 3);
	isp_toggle_clk(info, 0, 1, 3);
	isp_toggle_clk(info, 0, 1, 6);
	isp_toggle_clk(info, 0, 1, 12);
	isp_toggle_clk(info, 0, 1, 3);
	isp_toggle_clk(info, 0, 1, 3);

	isp_toggle_clk(info, 1, 0, 1);

	gpio_direction_output(info->pdata->gpio_sda, 0);
	isp_exit_mode(info);
	local_irq_restore(flags);
	usleep_range(300, 400);
}

static bool flash_is_erased(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	u32 val;
	u16 addr;

	for (addr = 0; addr < (ISP_MAX_FW_SIZE / 4); addr++) {
		udelay(40);
		val = flash_readl(info, addr);

		if (val != 0xffffffff) {
			dev_dbg(&client->dev,
				"addr 0x%x not erased: 0x%08x != 0xffffffff\n",
				addr, val);
			return false;
		}
	}
	return true;
}

static int fw_write_image(struct mms_ts_info *info, const u8 * data, size_t len)
{
	struct i2c_client *client = info->client;
	u16 addr = 0;

	for (addr = 0; addr < (len / 4); addr++, data += 4) {
		u32 val = get_unaligned_le32(data);
		u32 verify_val;
		int retries = 3;

		while (retries--) {
			flash_writel(info, addr, val);
			verify_val = flash_readl(info, addr);
			if (val == verify_val)
				break;
			dev_err(&client->dev,
				"mismatch @ addr 0x%x: 0x%x != 0x%x\n",
				addr, verify_val, val);
			continue;
		}
		if (retries < 0)
			return -ENXIO;
	}

	return 0;
}

static int mms100_ISP_download(struct mms_ts_info *info, const u8 * data,
						size_t len)
{
	struct i2c_client *client = info->client;
	u32 val;
	int ret = 0;

	if (len % 4) {
		dev_err(&client->dev,
			"fw image size (%d) must be a multiple of 4 bytes\n",
			len);
		return -EINVAL;
	} else if (len > ISP_MAX_FW_SIZE) {
		dev_err(&client->dev,
			"fw image is too big, %d > %d\n", len, ISP_MAX_FW_SIZE);
		return -EINVAL;
	}

	dev_info(&client->dev, "fw download start\n");

	info->pdata->power(0);
	gpio_direction_output(info->pdata->gpio_sda, 0);
	gpio_direction_output(info->pdata->gpio_scl, 0);
	gpio_direction_output(info->pdata->gpio_int, 0);

	hw_reboot_bootloader(info);

	val = flash_readl(info, ISP_IC_INFO_ADDR);
	dev_info(&client->dev, "IC info before erase : [%x]\n", val);

	dev_info(&client->dev, "fw erase...\n");
	flash_erase(info);
	if (!flash_is_erased(info)) {
		ret = -ENXIO;
		goto err;
	}

	flash_writel(info, ISP_IC_INFO_ADDR, val);
	val = flash_readl(info, ISP_IC_INFO_ADDR);
	dev_info(&client->dev, "IC info after erase & write : [%x]\n", val);

	dev_info(&client->dev, "fw write...\n");
	usleep_range(1000, 1500);
	ret = fw_write_image(info, data, len);
	if (ret)
		goto err;
	usleep_range(1000, 1500);

	hw_reboot_normal(info);
	usleep_range(1000, 1500);
	dev_info(&client->dev, "fw download done...\n");
	return 0;

err:
	dev_err(&client->dev, "fw download failed...\n");
	hw_reboot_normal(info);
	return ret;
}

static int get_fw_version(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	int ret;
	int retries = 3;

	/* this seems to fail sometimes after a reset.. retry a few times */
	do {
		ret = i2c_smbus_read_byte_data(info->client, MMS_FW_VERSION);
	} while (ret < 0 && retries-- > 0);

	dev_info(&client->dev, "TSP_REVISION = [%c Type], HW_REVISION = [0x%02x], COMPAT_GROUP = [0x%02x]",
		i2c_smbus_read_byte_data(info->client, MMS_TSP_REVISION),
		i2c_smbus_read_byte_data(info->client, MMS_HW_REVISION),
		i2c_smbus_read_byte_data(info->client, MMS_COMPAT_GROUP));

	return ret;
}

static int get_panel_type(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	int ret;
	int retries = 3;

	/* this seems to fail sometimes after a reset.. retry a few times */
	do {
		ret = i2c_smbus_read_byte_data(info->client, MMS_TSP_REVISION);
	} while (ret < 0 && retries-- > 0);

	dev_info(&client->dev, "MMS_TSP_REVISION = [0x%02x],[%c]", ret, ret);

	return ret;
}

static int mms_ts_fw_load(struct mms_ts_info *info)
{

	struct i2c_client *client = info->client;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	int ret = 0, ret_isp = 0;
	int retries_isc = 3;
	int retries_isp = 3;

	/* firmware check */
	info->fw_ic_ver = get_fw_version(info);
	dev_info(&client->dev, "Before FW update : [0x%02x]", info->fw_ic_ver);

	if (system_rev < 2) {
		dev_err(&client->dev, "FW update skip[%d]\n", system_rev);
		return ret;
	}

	/* Add  panel type check logic */
	if (system_rev >= 2 && system_rev <= 5) {
		info->panel_type = get_panel_type(info);

		if (info->panel_type != PANEL_MOREENS && \
					info->panel_type != PANEL_SMAC) {
			dev_err(&client->dev, "abnormal panel type[%c]!\n",
							info->panel_type);
			if (system_rev == 2)
				info->panel_type = PANEL_MOREENS;
			else
				info->panel_type = PANEL_SMAC;
		}
	} else if (system_rev >= 6)
		info->panel_type = PANEL_SMAC_NEW;

	dev_info(&client->dev, "Rev = 0x%02x, %c type Panel\n",
						system_rev, info->panel_type);

	if (!info->pdata || !info->pdata->mux_fw_flash) {
		dev_err(&client->dev,
			"fw cannot be updated, missing platform data\n");
		return 1;
	}

#if FW_UPDATABLE

	while (retries_isc--) {
		/* ISC firmware update */
		if (info->fw_ic_ver == 0x00) {
			dev_err(&client->dev, "TSP panel info dismatched\n");
			ret = mms100_ISC_download_mbinary(client, REQ_FW,
							info->panel_type,
							FORCED_UPDATE);
		} else
			ret = mms100_ISC_download_mbinary(client, REQ_FW,
							info->panel_type,
							COMPARE_UPDATE);

		if (ret > 0) {
			dev_err(&client->dev, "ISC D/L mode fail\n");

			i2c_lock_adapter(adapter);
			info->pdata->mux_fw_flash(true);

			while (retries_isp--) {
				/* ISP firmware update */
				ret_isp = mms100_ISP_download(info,
							boot_binary,
							boot_binary_nLength);
				if (ret_isp < 0)
					dev_err(&client->dev,
						"ISP D/L mode fail\n");
				else {
					dev_err(&client->dev,
						"ISP D/L mode success\n");
					break;
				}
			}

			info->pdata->mux_fw_flash(false);
			i2c_unlock_adapter(adapter);

			if (retries_isp == 0) {
				dev_err(&client->dev,
						"ISP D/L mode all fail!\n");
				return ret_isp;
			}

			ret = mms100_ISC_download_mbinary(client, REQ_FW,
							info->panel_type,
							FORCED_UPDATE);
		}

		if (ret == ISC_SUCCESS) {
			info->fw_ic_ver = get_fw_version(info);
			info->pdata->fw_version = mbin_info[SEC_PUBLIC_CONFIG].version;
			dev_info(&client->dev, "After FW update : [0x%02x]\n",
							info->fw_ic_ver);
			return ret;
		}
		dev_err(&client->dev, "retrying flashing\n");
		continue;
	}

	if (retries_isc == 0) {
		dev_err(&client->dev, "tsp f/w update all fail!\n");
		return ret;
	}
#endif

	return ret;
}


#if SEC_TSP_FACTORY_TEST
static void set_cmd_result(struct mms_ts_info *info, char *buff, int len)
{
	strncat(info->cmd_result, buff, len);
}

static void get_raw_data_all(struct mms_ts_info *info, u8 cmd)
{
	u8 w_buf[6];
	u8 read_buffer[2];	/* 52 */
	int gpio = info->pdata->gpio_int;
	int ret;
	int i, j;
	u32 max_value = 0, min_value = 0;
	u32 raw_data;
	char buff[TSP_CMD_STR_LEN] = {0};
	int tx_num = info->pdata->tsp_tx;
	int rx_num = info->pdata->tsp_rx;

	disable_irq(info->irq);

	w_buf[0] = MMS_VSC_CMD;	/* vendor specific command id */
	w_buf[1] = MMS_VSC_MODE;	/* mode of vendor */
	w_buf[2] = 0;		/* tx line */
	w_buf[3] = 0;		/* rx line */
	w_buf[4] = 0;		/* reserved */
	w_buf[5] = 0;		/* sub command */

	if (cmd == MMS_VSC_CMD_EXIT) {
		w_buf[5] = MMS_VSC_CMD_EXIT;	/* exit test mode */

		ret = i2c_smbus_write_i2c_block_data(info->client,
						     w_buf[0], 5, &w_buf[1]);
		if (ret < 0)
			goto err_i2c;
		enable_irq(info->irq);
		msleep(200);
		return;
	}

	/* MMS_VSC_CMD_CM_DELTA or MMS_VSC_CMD_CM_ABS
	 * this two mode need to enter the test mode
	 * exit command must be followed by testing.
	 */
	if (cmd == MMS_VSC_CMD_CM_DELTA || cmd == MMS_VSC_CMD_CM_ABS) {
		/* enter the debug mode */
		w_buf[2] = 0x0;	/* tx */
		w_buf[3] = 0x0;	/* rx */
		w_buf[5] = MMS_VSC_CMD_ENTER;

		ret = i2c_smbus_write_i2c_block_data(info->client,
						     w_buf[0], 5, &w_buf[1]);
		if (ret < 0)
			goto err_i2c;

		/* wating for the interrupt */
		while (gpio_get_value(gpio))
			udelay(100);
	}

	for (i = 0; i < rx_num; i++) {
		for (j = 0; j < tx_num; j++) {

			w_buf[2] = j;	/* tx */
			w_buf[3] = i;	/* rx */
			w_buf[5] = cmd;

			ret = i2c_smbus_write_i2c_block_data(info->client,
					w_buf[0], 5, &w_buf[1]);
			if (ret < 0)
				goto err_i2c;

			usleep_range(1, 5);

			ret = i2c_smbus_read_i2c_block_data(info->client, 0xBF,
							    2, read_buffer);
			if (ret < 0)
				goto err_i2c;

			raw_data = ((u16) read_buffer[1] << 8) | read_buffer[0];
			if (i == 0 && j == 0) {
				max_value = min_value = raw_data;
			} else {
				max_value = max(max_value, raw_data);
				min_value = min(min_value, raw_data);
			}

			if (cmd == MMS_VSC_CMD_INTENSITY) {
				info->intensity[j * rx_num + i] = raw_data;
				dev_dbg(&info->client->dev, "[TSP] intensity[%d][%d] = %d\n",
					i, j, info->intensity[j * rx_num + i]);
			} else if (cmd == MMS_VSC_CMD_CM_DELTA) {
				info->cm_delta[j * rx_num + i] = raw_data;
				dev_dbg(&info->client->dev, "[TSP] delta[%d][%d] = %d\n",
					i, j, info->cm_delta[j * rx_num + i]);
			} else if (cmd == MMS_VSC_CMD_CM_ABS) {
				info->cm_abs[j * rx_num + i] = raw_data;
				dev_dbg(&info->client->dev, "[TSP] raw[%d][%d] = %d\n",
					i, j, info->cm_abs[j * rx_num + i]);
			} else if (cmd == MMS_VSC_CMD_REFER) {
				info->reference[j * rx_num + i] =
						raw_data >> 3;
				dev_dbg(&info->client->dev, "[TSP] reference[%d][%d] = %d\n",
					i, j, info->reference[j * rx_num + i]);
			}
		}

	}

	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	enable_irq(info->irq);

err_i2c:
	dev_err(&info->client->dev, "%s: fail to i2c (cmd=%d)\n",
		__func__, cmd);
}

static u32 get_raw_data_one(struct mms_ts_info *info, u16 rx_idx, u16 tx_idx,
			    u8 cmd)
{
	u8 w_buf[6];
	u8 read_buffer[2];
	int ret;
	u32 raw_data;

	w_buf[0] = MMS_VSC_CMD;	/* vendor specific command id */
	w_buf[1] = MMS_VSC_MODE;	/* mode of vendor */
	w_buf[2] = 0;		/* tx line */
	w_buf[3] = 0;		/* rx line */
	w_buf[4] = 0;		/* reserved */
	w_buf[5] = 0;		/* sub command */

	if (cmd != MMS_VSC_CMD_INTENSITY && cmd != MMS_VSC_CMD_RAW &&
	    cmd != MMS_VSC_CMD_REFER) {
		dev_err(&info->client->dev, "%s: not profer command(cmd=%d)\n",
			__func__, cmd);
		return -1;
	}

	w_buf[2] = tx_idx;	/* tx */
	w_buf[3] = rx_idx;	/* rx */
	w_buf[5] = cmd;		/* sub command */

	ret = i2c_smbus_write_i2c_block_data(info->client, w_buf[0], 5,
					     &w_buf[1]);
	if (ret < 0)
		goto err_i2c;

	ret = i2c_smbus_read_i2c_block_data(info->client, 0xBF, 2, read_buffer);
	if (ret < 0)
		goto err_i2c;

	raw_data = ((u16) read_buffer[1] << 8) | read_buffer[0];
	if (cmd == MMS_VSC_CMD_REFER)
		raw_data = raw_data >> 4;

	return raw_data;

err_i2c:
	dev_err(&info->client->dev, "%s: fail to i2c (cmd=%d)\n",
		__func__, cmd);
	return -1;
}

static ssize_t show_close_tsp_test(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);

	get_raw_data_all(info, MMS_VSC_CMD_EXIT);
	info->ft_flag = 0;

	return snprintf(buf, TSP_BUF_SIZE, "%u\n", 0);
}

static void set_default_result(struct mms_ts_info *info)
{
	char delim = ':';

	memset(info->cmd_result, 0x00, ARRAY_SIZE(info->cmd_result));
	memcpy(info->cmd_result, info->cmd, strlen(info->cmd));
	strncat(info->cmd_result, &delim, 1);
}

static int check_rx_tx_num(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[TSP_CMD_STR_LEN] = {0};
	int node;

	if (info->cmd_param[0] < 0 ||
			info->cmd_param[0] >= info->pdata->tsp_tx ||
			info->cmd_param[1] < 0 ||
			info->cmd_param[1] >= info->pdata->tsp_rx) {
		snprintf(buff, sizeof(buff) , "%s", "NG");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = 3;

		dev_info(&info->client->dev, "%s: parameter error: %u,%u\n",
				__func__, info->cmd_param[0],
				info->cmd_param[1]);
		node = -1;
		return node;
	}
	/* Model dependency */
	node = info->cmd_param[0] * info->pdata->tsp_rx + info->cmd_param[1];
	dev_info(&info->client->dev, "%s: node = %d\n", __func__,
			node);
	return node;

}

static void not_support_cmd(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};

	set_default_result(info);
	sprintf(buff, "%s", "NA");
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = 4;
	dev_info(&info->client->dev, "%s: \"%s(%d)\"\n", __func__,
				buff, strnlen(buff, sizeof(buff)));
	return;
}

static void fw_update(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	int ret = 0;
	int ver = 0;
	int retries = 5;

	set_default_result(info);

	dev_info(&client->dev,
		"fw_ic_ver = 0x%02x, fw_bin_ver = 0x%02x\n",
		info->fw_ic_ver, info->pdata->fw_version);

	switch (info->cmd_param[0]) {
	case BUILT_IN:
		info->cmd_param[0] = REQ_FW;
		dev_info(&client->dev, "BUILT_IN=>REQ_FW update mode!\n");
		break;

	case UMS:
		dev_info(&client->dev, "UMS update mode!\n");
		break;

	case REQ_FW:
		dev_info(&client->dev, "REQ_FW update mode!\n");
		break;

	default:
		dev_err(&client->dev, "invalid cmd_param[%d]\n",
					info->cmd_param[0]);
		goto not_support;
	}

	disable_irq(info->irq);
	while (retries--) {

#if ISC_DL_MODE
		ret = mms100_ISC_download_mbinary(client, info->cmd_param[0],
							info->panel_type,
							FORCED_UPDATE);
#endif
		if (ret) {
			dev_err(&client->dev, "retrying flashing\n");
			continue;
		}

		ver = get_fw_version(info);
		info->fw_ic_ver = ver;

		dev_info(&client->dev,
		  "After FW update : [0x%02x]\n", ver);
		info->cmd_state = OK;
		enable_irq(info->irq);
		return;
	}
	info->cmd_state = FAIL;
	enable_irq(info->irq);

not_support:
	return;
}

static void get_fw_ver_bin(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};

	set_default_result(info);
	snprintf(buff, sizeof(buff), "%#02x", info->pdata->fw_version);

	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_fw_ver_ic(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	int ver;

	set_default_result(info);

	ver = info->fw_ic_ver;
	snprintf(buff, sizeof(buff), "%#02x", ver);

	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_config_ver(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[20] = {0};

	set_default_result(info);

	snprintf(buff, sizeof(buff), "%s", info->config_fw_version);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = 2;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_threshold(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	int threshold;

	set_default_result(info);

	threshold = i2c_smbus_read_byte_data(info->client, 0x05);
	if (threshold < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = FAIL;
		return;
}
	snprintf(buff, sizeof(buff), "%d", threshold);

	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void module_off_master(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[3] = {0};

	mutex_lock(&info->lock);
	if (info->enabled) {
		disable_irq(info->irq);
		info->enabled = false;
	}
	mutex_unlock(&info->lock);

	if (info->pdata->power(0) == SUCCESS_PWR_CONTROL)
		snprintf(buff, sizeof(buff), "%s", "OK");
	else
		snprintf(buff, sizeof(buff), "%s", "NG");

	set_default_result(info);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	if (strncmp(buff, "OK", 2) == 0)
		info->cmd_state = OK;
	else
		info->cmd_state = FAIL;

	dev_info(&info->client->dev, "%s: %s\n", __func__, buff);
}

static void module_on_master(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[3] = {0};


	mutex_lock(&info->lock);
	if (!info->enabled) {
		enable_irq(info->irq);
		info->enabled = true;
	}
	mutex_unlock(&info->lock);

	if (info->pdata->power(1) == SUCCESS_PWR_CONTROL)
		snprintf(buff, sizeof(buff), "%s", "OK");
	else
		snprintf(buff, sizeof(buff), "%s", "NG");

	set_default_result(info);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	if (strncmp(buff, "OK", 2) == 0)
		info->cmd_state = OK;
	else
		info->cmd_state = FAIL;

	dev_info(&info->client->dev, "%s: %s\n", __func__, buff);

}

static void module_off_slave(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	not_support_cmd(info);
}

static void module_on_slave(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	not_support_cmd(info);
}

static void get_chip_vendor(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};

	set_default_result(info);

	snprintf(buff, sizeof(buff), "%s", info->pdata->tsp_vendor);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_chip_name(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};

	set_default_result(info);

	snprintf(buff, sizeof(buff), "%s", info->pdata->tsp_ic);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_reference(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	unsigned int val;
	int node;

	set_default_result(info);
	node = check_rx_tx_num(info);

	if (node < 0)
		return;

	val = info->reference[node];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = OK;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));

}

static void get_cm_abs(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	unsigned int val;
	int node;

	set_default_result(info);
	node = check_rx_tx_num(info);

	if (node < 0)
		return;

	val = info->cm_abs[node];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_cm_delta(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	unsigned int val;
	int node;

	set_default_result(info);
	node = check_rx_tx_num(info);

	if (node < 0)
		return;

	val = info->cm_delta[node];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_intensity(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	unsigned int val;
	int node;

	int i, j;

	set_default_result(info);
	node = check_rx_tx_num(info);

	if (node < 0)
		return;
#if 0
	for (i = 0 ; i < info->pdata->tsp_tx ; i++) {
		for (j = 0 ; j < info->pdata->tsp_rx ; j++) {
			printk(KERN_INFO "[%2d]",
				info->intensity[i*info->pdata->tsp_rx + j]);
		}
		printk("\n");
	}
#endif
	val = info->intensity[node];

	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_x_num(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	int val;
#if 1
	if (info->enabled)
		dev_info(&info->client->dev, "%s = [%d] from ic\n", __func__,
				i2c_smbus_read_byte_data(info->client, 0xEF));
#endif
	set_default_result(info);

	snprintf(buff, sizeof(buff), "%d", info->pdata->tsp_tx);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_y_num(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};
	int val;
#if 1
	if (info->enabled)
		dev_info(&info->client->dev, "%s = [%d] from ic\n", __func__,
				i2c_smbus_read_byte_data(info->client, 0xEE));
#endif

	set_default_result(info);

	snprintf(buff, sizeof(buff), "%d", info->pdata->tsp_rx);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void run_reference_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);
	get_raw_data_all(info, MMS_VSC_CMD_REFER);
	info->cmd_state = OK;

/*	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__); */
}

static void run_cm_abs_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);
	get_raw_data_all(info, MMS_VSC_CMD_CM_ABS);
	get_raw_data_all(info, MMS_VSC_CMD_EXIT);
	info->cmd_state = OK;

/*	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__); */
}

static void run_cm_delta_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);
	get_raw_data_all(info, MMS_VSC_CMD_CM_DELTA);
	get_raw_data_all(info, MMS_VSC_CMD_EXIT);
	info->cmd_state = OK;

/*	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__); */
}

static void run_intensity_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);
	get_raw_data_all(info, MMS_VSC_CMD_INTENSITY);
	info->cmd_state = OK;

/*	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__); */
}

static ssize_t store_cmd(struct device *dev, struct device_attribute
				  *devattr, const char *buf, size_t count)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;

	char *cur, *start, *end;
	char buff[TSP_CMD_STR_LEN] = {0};
	int len, i;
	struct tsp_cmd *tsp_cmd_ptr = NULL;
	char delim = ',';
	bool cmd_found = false;
	int param_cnt = 0;
	int ret;

	if (info->cmd_is_running == true) {
		dev_err(&info->client->dev, "tsp_cmd: other cmd is running.\n");
		goto err_out;
	}


	/* check lock  */
	mutex_lock(&info->cmd_lock);
	info->cmd_is_running = true;
	mutex_unlock(&info->cmd_lock);

	info->cmd_state = RUNNING;

	for (i = 0; i < ARRAY_SIZE(info->cmd_param); i++)
		info->cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;
	memset(info->cmd, 0x00, ARRAY_SIZE(info->cmd));
	memcpy(info->cmd, buf, len);

	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(buff, buf, cur - buf);
	else
		memcpy(buff, buf, len);

	/* find command */
	list_for_each_entry(tsp_cmd_ptr, &info->cmd_list_head, list) {
		if (!strcmp(buff, tsp_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(tsp_cmd_ptr, &info->cmd_list_head, list) {
			if (!strcmp("not_support_cmd", tsp_cmd_ptr->cmd_name))
				break;
		}
	}

	/* parsing parameters */
	if (cur && cmd_found) {
		cur++;
		start = cur;
		memset(buff, 0x00, ARRAY_SIZE(buff));
		do {
			if (*cur == delim || cur - buf == len) {
				end = cur;
				memcpy(buff, start, end - start);
				*(buff + strlen(buff)) = '\0';
				ret = kstrtoint(buff, 10,\
						info->cmd_param + param_cnt);
				start = cur + 1;
				memset(buff, 0x00, ARRAY_SIZE(buff));
				param_cnt++;
			}
			cur++;
		} while (cur - buf <= len);
	}

	dev_info(&client->dev, "cmd = %s\n", tsp_cmd_ptr->cmd_name);
	for (i = 0; i < param_cnt; i++)
		dev_info(&client->dev, "cmd param %d= %d\n", i,
							info->cmd_param[i]);

	tsp_cmd_ptr->cmd_func(info);


err_out:
	return count;
}

static ssize_t show_cmd_status(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	char buff[16] = {0};

	dev_info(&info->client->dev, "tsp cmd: status:%d\n",
			info->cmd_state);

	if (info->cmd_state == WAITING)
		snprintf(buff, sizeof(buff), "WAITING");

	else if (info->cmd_state == RUNNING)
		snprintf(buff, sizeof(buff), "RUNNING");

	else if (info->cmd_state == OK)
		snprintf(buff, sizeof(buff), "OK");

	else if (info->cmd_state == FAIL)
		snprintf(buff, sizeof(buff), "FAIL");

	else if (info->cmd_state == NOT_APPLICABLE)
		snprintf(buff, sizeof(buff), "NOT_APPLICABLE");

	return snprintf(buf, TSP_BUF_SIZE, "%s\n", buff);
}

static ssize_t show_cmd_result(struct device *dev, struct device_attribute
				    *devattr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);

	dev_info(&info->client->dev, "tsp cmd: result: %s\n", info->cmd_result);

	mutex_lock(&info->cmd_lock);
	info->cmd_is_running = false;
	mutex_unlock(&info->cmd_lock);

	info->cmd_state = WAITING;

	return snprintf(buf, TSP_BUF_SIZE, "%s\n", info->cmd_result);
}

#ifdef ESD_DEBUG

static bool intensity_log_flag;

static ssize_t show_intensity_logging_on(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	struct file *fp;
	char log_data[160] = { 0, };
	char buff[16] = { 0, };
	mm_segment_t old_fs;
	long nwrite;
	u32 val;
	int i, y, c;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

#define MELFAS_DEBUG_LOG_PATH "/sdcard/melfas_log"

	dev_info(&client->dev, "%s: start.\n", __func__);
	fp = filp_open(MELFAS_DEBUG_LOG_PATH, O_RDWR | O_CREAT,
		       S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR(fp)) {
		dev_err(&client->dev, "%s: fail to open log file\n", __func__);
		set_fs(old_fs);
		return -1;
	}

	intensity_log_flag = 1;
	do {
		for (y = 0; y < 3; y++) {
			/* for tx chanel 0~2 */
			memset(log_data, 0x00, 160);

			snprintf(buff, 16, "%1u: ", y);
			strncat(log_data, buff, strnlen(buff, 16));

			for (i = 0; i < info->pdata->tsp_rx ; i++) {
				val = get_raw_data_one(info, i, y,
						       MMS_VSC_CMD_INTENSITY);
				snprintf(buff, 16, "%5u, ", val);
				strncat(log_data, buff, strnlen(buff, 16));
			}
			memset(buff, '\n', 2);
			c = (y == 2) ? 2 : 1;
			strncat(log_data, buff, c);
			nwrite = vfs_write(fp, (const char __user *)log_data,
					   strnlen(log_data, 160), &fp->f_pos);
		}
		usleep_range(5000);
	} while (intensity_log_flag);

	filp_close(fp, current->files);
	set_fs(old_fs);

	return 0;
}

static ssize_t show_intensity_logging_off(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	intensity_log_flag = 0;
	usleep_range(10000);
	get_raw_data_all(info, MMS_VSC_CMD_EXIT);
	return 0;
}

#endif

static DEVICE_ATTR(close_tsp_test, S_IRUGO, show_close_tsp_test, NULL);
static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, store_cmd);
static DEVICE_ATTR(cmd_status, S_IRUGO, show_cmd_status, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, show_cmd_result, NULL);
#ifdef ESD_DEBUG
static DEVICE_ATTR(intensity_logging_on, S_IRUGO, show_intensity_logging_on,
		   NULL);
static DEVICE_ATTR(intensity_logging_off, S_IRUGO, show_intensity_logging_off,
		   NULL);
#endif

static struct attribute *sec_touch_facotry_attributes[] = {
		&dev_attr_close_tsp_test.attr,
		&dev_attr_cmd.attr,
		&dev_attr_cmd_status.attr,
		&dev_attr_cmd_result.attr,
#ifdef ESD_DEBUG
	&dev_attr_intensity_logging_on.attr,
	&dev_attr_intensity_logging_off.attr,
#endif
	NULL,
};

static struct attribute_group sec_touch_factory_attr_group = {
	.attrs = sec_touch_facotry_attributes,
};
#endif /* SEC_TSP_FACTORY_TEST */

static int __devinit mms_ts_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct mms_ts_info *info;
	struct input_dev *input_dev;
	int ret = 0;
	char buf[4] = { 0, };
	int i;

#if SEC_TSP_FACTORY_TEST
	struct device *fac_dev_ts;
	int rx_num;
	int tx_num;
#endif

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	info = kzalloc(sizeof(struct mms_ts_info), GFP_KERNEL);
	if (!info) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "Failed to allocate memory for input device\n");
		ret = -ENOMEM;
		goto err_input_alloc;
	}

	info->client = client;
	info->input_dev = input_dev;
	info->pdata = client->dev.platform_data;
	if (NULL == info->pdata) {
		dev_err(&client->dev, "failed to get platform data\n");
		goto err_config;
	}
	info->irq = -1;
	mutex_init(&info->lock);

	if (info->pdata) {
		info->max_x = info->pdata->max_x;
		info->max_y = info->pdata->max_y;
		info->invert_x = info->pdata->invert_x;
		info->invert_y = info->pdata->invert_y;
		info->config_fw_version = info->pdata->config_fw_version;
		info->input_event = info->pdata->input_event;
		info->register_cb = info->pdata->register_cb;
	} else {
		info->max_x = 720;
		info->max_y = 1280;
	}
	for (i = 0 ; i < MAX_FINGERS; i++)
		info->finger_state[i] = TSP_STATE_RELEASE;

	info->pdata->power(true);
	msleep(100);

	i2c_set_clientdata(client, info);
	ret = i2c_master_recv(client, buf, 1);
	if (ret < 0) {		/* tsp connect check */
		dev_err(&client->dev, "%s: tsp connect err [%d], Add[%d]\n",
			   __func__, ret, info->client->addr);
		goto err_config;
	}

	if (system_rev < 2) {
		info->pdata->tsp_ic = "MMS136";
		info->pdata->tsp_tx = 19;
		info->pdata->tsp_rx = 11;
		info->finger_byte = 6;
	} else
		info->finger_byte = 8;

	dev_info(&client->dev, "TSP Packet size %d", info->finger_byte);

	ret = mms_ts_fw_load(info);

	if (ret) {
		dev_err(&client->dev, "failed to initialize (%d)\n", ret);
		goto err_config;
	}

	info->enabled = true;

	snprintf(info->phys, sizeof(info->phys),
		 "%s/input0", dev_name(&client->dev));
	input_dev->name = "sec_touchscreen"; /*= "Melfas MMSxxx Touchscreen";*/
	input_dev->phys = info->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_mt_init_slots(input_dev, MAX_FINGERS);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			     0, (info->max_x)-1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
				0, (info->max_y)-1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR,
				0, MAX_WIDTH, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
				0, MAX_PRESSURE, 0, 0);

	if (info->finger_byte == 8) {
		input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR,
					0, MAX_PRESSURE, 0, 0);
		input_set_abs_params(input_dev, ABS_MT_ANGLE,
			     MIN_ANGLE, MAX_ANGLE, 0, 0);
		input_set_abs_params(input_dev, ABS_MT_PALM,
				0, 1, 0, 0);
	}
	input_set_drvdata(input_dev, info);

	ret = input_register_device(input_dev);
	if (ret) {
		dev_err(&client->dev, "failed to register input dev (%d)\n",
			ret);
		goto err_reg_input_dev;
	}

#if TOUCH_BOOSTER
	mutex_init(&info->dvfs_lock);
	INIT_DELAYED_WORK(&info->work_dvfs_off, set_dvfs_off);
	INIT_DELAYED_WORK(&info->work_dvfs_chg, change_dvfs_lock);
	info->bus_dev = dev_get("exynos-busfreq");
	info->sec_touchscreen = dev_get("sec_touchscreen");
	info->cpufreq_level = -1;
	info->dvfs_lock_status = false;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	info->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	info->early_suspend.suspend = mms_ts_early_suspend;
	info->early_suspend.resume = mms_ts_late_resume;
	register_early_suspend(&info->early_suspend);
#endif

	info->callbacks.inform_charger = melfas_ta_cb;
	if (info->register_cb)
		info->register_cb(&info->callbacks);

	ret = request_threaded_irq(client->irq, NULL, mms_ts_interrupt,
				   IRQF_TRIGGER_LOW  | IRQF_ONESHOT,
				   MELFAS_TS_NAME, info);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to register interrupt\n");
		goto err_reg_input_dev;
	}

	info->irq = client->irq;
	barrier();

	dev_info(&client->dev,
			"Melfas MMS-series touch controller initialized\n");

#if SEC_TSP_FACTORY_TEST
	rx_num = info->pdata->tsp_rx;
	tx_num = info->pdata->tsp_tx;

	info->reference = kzalloc(sizeof(int) * rx_num * tx_num, GFP_KERNEL);
	info->cm_abs = kzalloc(sizeof(int) * rx_num * tx_num, GFP_KERNEL);
	info->cm_delta = kzalloc(sizeof(int) * rx_num * tx_num, GFP_KERNEL);
	info->intensity = kzalloc(sizeof(int) * rx_num * tx_num, GFP_KERNEL);
	if (unlikely(info->reference == NULL ||
				info->cm_abs == NULL ||
				info->cm_delta == NULL ||
				info->intensity == NULL)) {
		ret = -ENOMEM;
		goto err_alloc_node_data_failed;
	}


	INIT_LIST_HEAD(&info->cmd_list_head);
	for (i = 0; i < ARRAY_SIZE(tsp_cmds); i++)
		list_add_tail(&tsp_cmds[i].list, &info->cmd_list_head);

	mutex_init(&info->cmd_lock);
	info->cmd_is_running = false;

	fac_dev_ts = device_create(sec_class,
			NULL, 0, info, "tsp");
	if (IS_ERR(fac_dev_ts))
		dev_err(&client->dev, "Failed to create device for the sysfs\n");

	ret = sysfs_create_group(&fac_dev_ts->kobj,
				 &sec_touch_factory_attr_group);
	if (ret)
		dev_err(&client->dev, "Failed to create sysfs group\n");
#endif	/* SEC_TSP_FACTORY_TEST */
	return 0;

#if SEC_TSP_FACTORY_TEST
err_alloc_node_data_failed:
	dev_err(&client->dev, "Err_alloc_node_data failed\n");
	kfree(info->reference);
	kfree(info->cm_abs);
	kfree(info->cm_delta);
	kfree(info->intensity);
#endif

err_reg_input_dev:
	input_unregister_device(input_dev);
	input_free_device(input_dev);
err_config:
err_input_alloc:
	kfree(info);
err_alloc:
	return ret;

}

static int __devexit mms_ts_remove(struct i2c_client *client)
{
	struct mms_ts_info *info = i2c_get_clientdata(client);

#if SEC_TSP_FACTORY_TEST
	dev_err(&client->dev, "Err_alloc_node_data failed\n");
	kfree(info->reference);
	kfree(info->cm_abs);
	kfree(info->cm_delta);
	kfree(info->intensity);
#endif

	unregister_early_suspend(&info->early_suspend);
	if (info->irq >= 0)
		free_irq(info->irq, info);
	input_unregister_device(info->input_dev);
	kfree(info);

	return 0;
}

#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
static int mms_ts_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mms_ts_info *info = i2c_get_clientdata(client);

	if (!info->enabled)
		return 0;

	dev_notice(&info->client->dev, "%s\n", __func__);

	disable_irq(info->irq);
	info->enabled = false;
	release_all_fingers(info);
	info->pdata->power(0);
	return 0;
}

static int mms_ts_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mms_ts_info *info = i2c_get_clientdata(client);

	if (info->enabled)
		return 0;

	dev_notice(&info->client->dev, "%s\n", __func__);
	info->pdata->power(1);
	msleep(120);

	if (info->ta_status) {
		dev_notice(&client->dev, "TA or USB connect!!!\n");
		i2c_smbus_write_byte_data(info->client, MMS_TA_REG, MMS_TA_ON);

		if (info->noise_mode) {
			i2c_smbus_write_byte_data(info->client, MMS_NOISE_REG,
								MMS_NOISE_ON);
			dev_notice(&client->dev, "resume & noise mode on!\n");
		}
	} else
		info->noise_mode = false;

	/* Because irq_type by EXT_INTxCON register is changed to low_level
	 *  after wakeup, irq_type set to falling edge interrupt again.
	 */
	 /* irq_set_irq_type(client->irq, IRQ_TYPE_EDGE_FALLING); */

	enable_irq(info->irq);
	info->enabled = true;
	return 0;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mms_ts_early_suspend(struct early_suspend *h)
{
	struct mms_ts_info *info;
	info = container_of(h, struct mms_ts_info, early_suspend);
	mms_ts_suspend(&info->client->dev);

}

static void mms_ts_late_resume(struct early_suspend *h)
{
	struct mms_ts_info *info;
	info = container_of(h, struct mms_ts_info, early_suspend);
	mms_ts_resume(&info->client->dev);
}
#endif

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static const struct dev_pm_ops mms_ts_pm_ops = {
	.suspend = mms_ts_suspend,
	.resume = mms_ts_resume,
};
#endif

static const struct i2c_device_id mms_ts_id[] = {
	{MELFAS_TS_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, mms_ts_id);

static struct i2c_driver mms_ts_driver = {
	.probe = mms_ts_probe,
	.remove = __devexit_p(mms_ts_remove),
	.driver = {
		   .name = MELFAS_TS_NAME,
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
		   .pm = &mms_ts_pm_ops,
#endif
		   },
	.id_table = mms_ts_id,
};

static int __init mms_ts_init(void)
{

	return i2c_add_driver(&mms_ts_driver);

}

static void __exit mms_ts_exit(void)
{
	i2c_del_driver(&mms_ts_driver);
}

module_init(mms_ts_init);
module_exit(mms_ts_exit);

/* Module information */
MODULE_DESCRIPTION("Touchscreen driver for Melfas MMS-series controllers");
MODULE_LICENSE("GPL");
