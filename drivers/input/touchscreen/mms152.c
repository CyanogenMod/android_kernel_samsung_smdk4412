/* drivers/input/touchscreen/mms152.c
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


#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/mms152.h>
#include <linux/gpio.h>
#include <mach/cpufreq.h>

/* firmware version start */
#define HW_VERSION_EMPTY	0x00

#define GFS_HW_BASE_VER		0x03
#define GFS_SW_BASE_VER		0x08

#define G2M_HW_BASE_VER		0x12
#define G2M_SW_BASE_VER		0x09

#define GFD_HW_BASE_VER		0x26
#define GFD_SW_BASE_VER		0x04

#define G2W_HW_BASE_VER		0x32
#define G2W_SW_BASE_VER		0x01

/* ISP mode Ver */
#define GFS_HW_VER		0x03
#define GFS_SW_VER		0x14

#define G2M_HW_VER		0x12
#define G2M_SW_VER		0x09

#define GFD_HW_VER		0x26
#define GFD_SW_VER		0x07

#define G2W_HW_VER		0x32
#define G2W_SW_VER		0x01

/* ISC mode Ver */
#define CORE_VER		0x20

#define GFS_PRIVATE_VER		0x07
#define GFS_PUBLIC_VER		0x08

#define G2M_PRIVATE_VER		0x00
#define G2M_PUBLIC_VER		0x01

#define GFD_PRIVATE_VER		0x04
#define GFD_PUBLIC_VER		0x05

#define G2W_PRIVATE_VER		0x00
#define G2W_PUBLIC_VER		0x01
/* firmware version end */

#define MELFAS_MAX_TOUCH	10

#define TS_MAX_X_COORD		600
#define TS_MAX_Y_COORD		1024
#define TS_MAX_Z_TOUCH		255
#define TS_MAX_W_TOUCH		30

#define TS_READ_TSPCONNECT_ADDR		0x62
#define TS_READ_VERSION_ADDR		0x63
#define TS_READ_CORE_VERSION_ADDR	0x66
#define TS_READ_PRIVATE_VERSION_ADDR	0x67
#define TS_READ_PUBLIC_VERSION_ADDR	0x68
#define TS_READ_REGS_LEN		66

#define TS_READ_START_ADDR		0x0F
#define TS_READ_START_ADDR2		0x10

#define TS_WRITE_REGS_LEN		16
#define TS_THRESHOLD			0x70
/* #define TSP_FACTORY_TEST */
#define ENABLE_NOISE_TEST_MODE
#define TS_TA_STAT_ADDR			0x60
/* #define DEBUG_LOW_DATA */

#define I2C_RETRY_CNT			50
#define P2_MAX_I2C_FAIL			50
#define P2_MAX_INFO_READ_FAIL		3

#define	SET_DOWNLOAD_BY_GPIO		1

#define PRESS_KEY		1
#define RELEASE_KEY		0

#define SHOW_COORD		1
#define DEBUG_PRINT		0
#define DEBUG_MODE

#define TOUCH_BOOSTER		1
#define SEC_DVFS_LOCK_TIMEOUT	3

#define	X_LINE			20
#define	Y_LINE			31
#define	TSP_CHIP_VENDER_NAME	"MELFAS,MMS152"

enum {
	TSP_STATE_RELEASE = 0,
	TSP_STATE_PRESS,
	TSP_STATE_MOVE,
};

#if SET_DOWNLOAD_BY_GPIO
#include "mms152_download.h"
#endif

struct melfas_ts_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct ts_platform_data *pdata;
	struct work_struct  work;
	struct tsp_callbacks cb;
	struct mutex m_lock;
#if TOUCH_BOOSTER
	struct delayed_work  dvfs_work;
	bool dvfs_lock_status;
	int cpufreq_level;
#endif
	u8 finger_state[MELFAS_MAX_TOUCH];
	uint32_t flags;
	bool charging_status;
	bool tsp_status;
	int (*power)(int on);
	struct early_suspend early_suspend;
	void (*power_on)(void);
	void (*power_off)(void);
	void (*register_cb)(void *);
	void (*read_ta_status)(bool *);
	void (*set_touch_i2c)(void);
	void (*set_touch_i2c_to_gpio)(void);
	int touch_id;
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_ts_early_suspend(struct early_suspend *h);
static void melfas_ts_late_resume(struct early_suspend *h);
#endif

static struct multi_touch_info g_Mtouch_info[MELFAS_MAX_TOUCH];

static bool debug_print;
static int firm_status_data;

#ifdef DEBUG_MODE
static bool debug_on;

static tCommandInfo_t tCommandInfo[] = {
	{ '?', "Help" },
	{ 'T', "Go to LOGGING mode" },
	{ 'M', "Go to MTSI_1_2_0 mode" },
	{ 'R', "Toggle LOG ([R]awdata)" },
	{ 'F', "Toggle LOG (Re[f]erence)" },
	{ 'I', "Toggle LOG ([I]ntensity)" },
	{ 'G', "Toggle LOG ([G]roup Image)" },
	{ 'D', "Toggle LOG ([D]elay Image)" },
	{ 'P', "Toggle LOG ([P]osition)" },
	{ 'B', "Toggle LOG (De[b]ug)" },
	{ 'V', "Toggle LOG (Debug2)" },
	{ 'L', "Toggle LOG (Profi[l]ing)" },
	{ 'O', "[O]ptimize Delay" },
	{ 'N', "[N]ormalize Intensity" }
};

static bool vbLogType[LT_LIMIT] = {0, };
static const char mcLogTypeName[LT_LIMIT][20] = {
	"LT_DIAGNOSIS_IMG",
	"LT_RAW_IMG",
	"LT_REF_IMG",
	"LT_INTENSITY_IMG",
	"LT_GROUP_IMG",
	"LT_DELAY_IMG",
	"LT_POS",
	"LT_DEBUG",
	"LT_DEBUG2",
	"LT_PROFILING",
};

static void toggle_log(struct melfas_ts_data *ts, eLogType_t _eLogType);
static void print_command_list(void);
static int melfas_i2c_read(struct i2c_client *client, u16 addr,
				u16 length, u8 *value);
static void debug_i2c_read(struct i2c_client *client, u16 addr,
				u8 *value, u16 length)
{
	melfas_i2c_read(client, addr, length, value);
}

static int debug_i2c_write(struct i2c_client *client, u8 *value, u16 length)
{
	return i2c_master_send(client, value, length);
}

static void key_handler(struct melfas_ts_data *ts, char key_val)
{
	u8 write_buf[2];
	int ret = 0;
	pr_info("[TSP] %s - %c\n", __func__, key_val);
	switch (key_val) {
	case '?':
	case '/':
		print_command_list();
		break;
	case 'T':
	case 't':
		write_buf[0] = ADDR_ENTER_LOGGING;
		write_buf[1] = 1;
		ret = debug_i2c_write(ts->client, write_buf, 2);
		debug_on = true;
		pr_info("result = %d", ret);
		break;
	case 'M':
	case 'm':
		write_buf[0] = ADDR_CHANGE_PROTOCOL;
		write_buf[1] = 11;
		debug_i2c_write(ts->client, write_buf, 2);
		debug_on = false;
		break;
	case 'R':
	case 'r':
		toggle_log(ts, LT_RAW_IMG);
		break;
	case 'F':
	case 'f':
		toggle_log(ts, LT_REF_IMG);
		break;
	case 'I':
	case 'i':
		toggle_log(ts, LT_INTENSITY_IMG);
		break;
	case 'G':
	case 'g':
		toggle_log(ts, LT_GROUP_IMG);
		break;
	case 'D':
	case 'd':
		toggle_log(ts, LT_DELAY_IMG);
		break;
	case 'P':
	case 'p':
		toggle_log(ts, LT_POS);
		break;
	case 'B':
	case 'b':
		toggle_log(ts, LT_DEBUG);
		break;
	case 'V':
	case 'v':
		toggle_log(ts, LT_DEBUG2);
		break;
	case 'L':
	case 'l':
		toggle_log(ts, LT_PROFILING);
		break;
	case 'O':
	case 'o':
		pr_info("Enter 'Optimize Delay' mode!!!\n");
		write_buf[0] = ADDR_CHANGE_OPMODE;
		write_buf[1] = OM_OPTIMIZE_DELAY;
		if (!debug_i2c_write(ts->client, write_buf, 2))
			goto ERROR_HANDLE;
		break;
	case 'N':
	case 'n':
		pr_info("Enter 'Normalize Intensity' mode!!!\n");
		write_buf[0] = ADDR_CHANGE_OPMODE;
		write_buf[1] = OM_NORMALIZE_INTENSITY;
		if (!debug_i2c_write(ts->client, write_buf, 2))
			goto ERROR_HANDLE;
		break;
	default:
		;
	}
	return;
ERROR_HANDLE:
	pr_info("ERROR!!!\n");
}

static void print_command_list(void)
{
	int i;
	pr_info("######################################################\n");
	for (i = 0; i < sizeof(tCommandInfo) / sizeof(tCommandInfo_t); i++)
		pr_info("[%c]: %s\n", tCommandInfo[i].cCommand,
				tCommandInfo[i].sDescription);
	pr_info("######################################################\n");
}

static void toggle_log(struct melfas_ts_data *ts, eLogType_t _eLogType)
{
	u8 write_buf[2];
	vbLogType[_eLogType] ^= 1;
	if (vbLogType[_eLogType]) {
		write_buf[0] = ADDR_LOGTYPE_ON;
		pr_info("%s ON\n", mcLogTypeName[_eLogType]);
	} else {
		write_buf[0] = ADDR_LOGTYPE_OFF;
		pr_info("%s OFF\n", mcLogTypeName[_eLogType]);
	}
	write_buf[1] = _eLogType;
	debug_i2c_write(ts->client, write_buf, 2);
}

static void logging_function(struct melfas_ts_data *ts)
{
	u8 read_buf[100];
	u8 read_mode, read_num;
	int FingerX, FingerY, FingerID;
	int i;
	static int past_read_mode = HEADER_NONE;
	static char *ps;
	static char s[500];

	debug_i2c_read(ts->client, LOG_READ_ADDR, read_buf, 2);

	read_mode = read_buf[0];
	read_num = read_buf[1];

	switch (read_mode) {
	case HEADER_U08:
	{
		unsigned char* p = (unsigned char *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf, read_num + 2);
		ps = s;
		s[0] = '\0';

		for (i = 0; i < read_num - 1; i++) {
			sprintf(ps, "%4d,", p[i]);
			ps = s + strlen(s);
		}
		sprintf(ps, "%4d\n", p[i]);
		ps = s + strlen(s);
		printk(KERN_DEBUG "%s", s);
		break;
	}
	case HEADER_S08:
	{
		signed char* p = (signed char *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf, read_num + 2);
		ps = s;
		s[0] = '\0';

		for (i = 0; i < read_num - 1; i++) {
			sprintf(ps, "%4d,", p[i]);
			ps = s + strlen(s);
		}
		sprintf(ps, "%4d\n", p[i]);
		ps = s + strlen(s);
		printk(KERN_DEBUG "%s", s);
		break;
	}
	case HEADER_U16:
	{
		unsigned short* p = (unsigned short *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf, read_num * 2 + 2);
		if (past_read_mode != HEADER_U16_NOCR) {
			ps = s;
			s[0] = '\0';
		}

		for (i = 0; i < read_num - 1; i++) {
			sprintf(ps, "%5d,", p[i]);
			ps = s + strlen(s);
		}
		sprintf(ps, "%5d\n", p[i]);
		ps = s + strlen(s);
		printk(KERN_DEBUG "%s", s);
		break;
	}
	case HEADER_U16_NOCR:
	{
		unsigned short* p = (unsigned short *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf, read_num * 2 + 2);

		if (past_read_mode != HEADER_U16_NOCR) {
			ps = s;
			s[0] = '\0';
		}
		for (i = 0; i < read_num; i++) {
			sprintf(ps, "%5d,", p[i]);
			ps = s + strlen(s);
		}
		break;
	}
	case HEADER_S16:
	{
		signed short* p = (signed short *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf, read_num * 2 + 2);

		if (past_read_mode != HEADER_S16_NOCR) {
			ps = s;
			s[0] = '\0';
		}

		for (i = 0; i < read_num - 1; i++) {
			sprintf(ps, "%5d,", p[i]);
			ps = s + strlen(s);
		}
		sprintf(ps, "%5d\n", p[i]);
		ps = s + strlen(s);
		printk(KERN_DEBUG "%s", s);
		break;
	}
	case HEADER_S16_NOCR:
	{
		signed short* p = (signed short *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf, read_num * 2 + 2);

		if (past_read_mode != HEADER_S16_NOCR) {
			ps = s;
			s[0] = '\0';
		}
		for (i = 0; i < read_num; i++) {
			sprintf(ps, "%5d,", p[i]);
			ps = s + strlen(s);
		}
		break;
	}
	case HEADER_U32:
	{
		unsigned long* p = (unsigned long *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf, read_num * 4 + 4);

		if (past_read_mode != HEADER_U32_NOCR) {
			ps = s;
			s[0] = '\0';
		}

		for (i = 0; i < read_num - 1; i++) {
			sprintf(ps, "%10ld,", p[i]);
			ps = s + strlen(s);
		}
		sprintf(ps, "%10ld\n", p[i]);
		ps = s + strlen(s);
		printk(KERN_DEBUG "%s", s);
		break;
	}
	case HEADER_U32_NOCR:
	{
		unsigned long* p = (unsigned long *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf, read_num * 4 + 4);

		if (past_read_mode != HEADER_U32_NOCR) {
			ps = s;
			s[0] = '\0';
		}
		for (i = 0; i < read_num; i++) {
			sprintf(ps, "%10ld,", p[i]);
			ps = s + strlen(s);
		}
		break;
	}
	case HEADER_S32:
	{
		signed long* p = (signed long *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf, read_num * 4 + 4);

		if (past_read_mode != HEADER_S32_NOCR) {
			ps = s;
			s[0] = '\0';
		}

		for (i = 0; i < read_num - 1; i++) {
			sprintf(ps, "%10ld,", p[i]);
			ps = s + strlen(s);
		}
		sprintf(ps, "%10ld\n", p[i]);
		ps = s + strlen(s);
		printk(KERN_DEBUG "%s", s);
		break;
	}
	case HEADER_S32_NOCR:
	{
		signed long* p = (signed long *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf, read_num * 4 + 4);

		if (past_read_mode != HEADER_S32_NOCR) {
			ps = s;
			s[0] = '\0';
		}
		for (i = 0; i < read_num; i++) {
			sprintf(ps, "%10ld,", p[i]);
			ps = s + strlen(s);
		}
		break;
	}
	case HEADER_TEXT:
	{
		i2c_master_recv(ts->client, read_buf, read_num + 2);

		ps = s;
		s[0] = '\0';

		for (i = 2; i < read_num + 2; i++) {
			sprintf(ps, "%c", read_buf[i]);
			ps = s + strlen(s);
		}
		printk(KERN_DEBUG "%s\n", s);
		break;
	}
	case HEADER_FINGER:
	{
		i2c_master_recv(ts->client, read_buf, read_num * 4 + 2);

		ps = s;
		s[0] = '\0';
		for (i = 2; i < read_num * 4 + 2; i = i + 4) {
			FingerX = (read_buf[i + 1] & 0x07) << 8 | read_buf[i];
			FingerY = (read_buf[i + 3] & 0x07) << 8
					| read_buf[i + 2];

			FingerID = (read_buf[i + 1] & 0xF8) >> 3;
			sprintf(ps, "%2d (%4d,%4d) | ",
					FingerID, FingerX, FingerY);
			ps = s + strlen(s);
		}
		printk(KERN_DEBUG "%s\n", s);
		break;
	}
	case HEADER_S12:
	{
		signed short* p = (signed short *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf, read_num * 2 + 2);

		if (past_read_mode != HEADER_S12_NOCR) {
			ps = s;
			s[0] = '\0';
		}
		for (i = 0; i < read_num; i++) {
			if (p[i] > 4096 / 2)
				p[i] -= 4096;
		}

		for (i = 0; i < read_num - 1; i++) {
			sprintf(ps, "%5d,", p[i]);
			ps = s + strlen(s);
		}
		sprintf(ps, "%5d\n", p[i]);
		ps = s + strlen(s);
		printk(KERN_DEBUG "%s", s);
		break;
	}
	case HEADER_S12_NOCR:
	{
		signed short* p = (signed short *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf, read_num * 2 + 2);

		if (past_read_mode != HEADER_S12_NOCR) {
			ps = s;
			s[0] = '\0';
		}
		for (i = 0; i < read_num; i++) {
			if (p[i] > 4096 / 2)
				p[i] -= 4096;
		}
		for (i = 0; i < read_num; i++) {
			sprintf(ps, "%5d,", p[i]);
			ps = s + strlen(s);
		}
		break;
	}
	case HEADER_PRIVATE:
	{
		unsigned char* p = (unsigned char *) &read_buf[2];
		i2c_master_recv(ts->client, read_buf,
					read_num + 2 + read_num % 2);

		ps = s;
		s[0] = '\0';
		sprintf(ps, "################## CUSTOM_PRIVATE LOG: ");
		ps = s + strlen(s);
		for (i = 0; i < read_num - 1; i++) {
			sprintf(ps, "%5d,", p[i]);
			ps = s + strlen(s);
		}
		sprintf(ps, "%5d\n", p[i]);
		ps = s + strlen(s);
		printk(KERN_DEBUG "%s", s);
		break;
	}
	default:
		break;
	}

	past_read_mode = read_mode;
}
#endif /* DEBUG_MODE */

static int melfas_i2c_read(struct i2c_client *client,
				u16 addr, u16 length, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	int i;

	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 2;
	msg[0].buf   = (u8 *) &addr;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = length;
	msg[1].buf   = (u8 *) value;

	i = i2c_transfer(adapter, msg, 2);
	if (i == 2)
		return 0;
	else{
		pr_err("[TSP] melfas_i2c_read error : [%d]", i);
		return -EIO;
	}

}

static int melfas_i2c_write(struct i2c_client *client, char *buf, int length)
{
	int i;
	char data[TS_WRITE_REGS_LEN];

	if (length > TS_WRITE_REGS_LEN) {
		pr_err("[TSP] size error - %s\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < length; i++)
		data[i] = *buf++;

	i = i2c_master_send(client, (char *)data, length);

	if (i == length)
		return length;
	else{
		pr_err("[TSP] melfas_i2c_write error : [%d]", i);
		return -EIO;
	}
}

static int read_input_info(struct melfas_ts_data *ts,
				u8 *val, u8 start_addr, int read_length)
{
	return melfas_i2c_read(ts->client, start_addr, read_length, val);
}

static int check_detail_firmware(struct melfas_ts_data *ts, u8 *val)
{
	return melfas_i2c_read(ts->client, TS_READ_VERSION_ADDR, 6, val);
}

static int check_tsp_connect(struct melfas_ts_data *ts, u8 *val)
{
	return melfas_i2c_read(ts->client, TS_READ_TSPCONNECT_ADDR, 1, val);
}

static int firmware_update(struct melfas_ts_data *ts)
{

	int ret = 0, i = 0;
	int touch_id = ts->touch_id;
	uint8_t fw_ver[6] = {0,};
	uint8_t tsp_connect_stat = 0;
	uint8_t fw_isc_update = 0x00;
	bool fw_isp_update = false;

#if SET_DOWNLOAD_BY_GPIO

	msleep(200);

	for (i = 0 ; i < P2_MAX_INFO_READ_FAIL ; i++) {
		ret = check_tsp_connect(ts, &tsp_connect_stat);
		if (!ret)
			break;
		msleep(100);
	}
	if (i == P2_MAX_INFO_READ_FAIL) {
		pr_err("[TSP] check_tsp_connect check fail! [%d]", ret);
		fw_isp_update |= true;
	}
	pr_info("[TSP] TSP panel is %sconnected [%d]",
				tsp_connect_stat ? "" : "dis", i);

	if (touch_id == 3)
		return 0;

	for (i = 0 ; i < P2_MAX_INFO_READ_FAIL ; i++) {
		ret = check_detail_firmware(ts, fw_ver);
		if (!ret)
			break;
	}

	if (i == P2_MAX_INFO_READ_FAIL) {
		pr_err("[TSP] check_firmware fail! [%d]", ret);
		fw_isp_update |= true;
	} else {
		pr_info("[TSP] Chk HW:[%x],SW:[%x],Core:[%x],Pri:[%x],Pub:[%x]",
			fw_ver[0], fw_ver[1], fw_ver[3], fw_ver[4], fw_ver[5]);

		/* basic status for ISP D/L	*/
		if (ret > 0 || fw_ver[0] == HW_VERSION_EMPTY) {
			fw_isp_update |= true;
		} else if (fw_ver[0]>>4 != touch_id) {
			if (fw_ver[3] == CORE_VER) {
				fw_isc_update |= 0x06;
				pr_info("[TSP] bin & panel dismatch ISC partial update!");
			} else {
				fw_isp_update |= true;
				pr_info("[TSP] bin & panel dismatch ISP Full update!");
			}
		} else {
			if (touch_id == 0) {		/* GFF S-mac */
				if (fw_ver[0] < GFS_HW_BASE_VER ||
					(fw_ver[0] == GFS_HW_BASE_VER &&
					fw_ver[1] < GFS_SW_BASE_VER))
					fw_isp_update |= true;
			} else if (touch_id == 1) {	/* G2 -Morins */
				if (fw_ver[0] < G2M_HW_BASE_VER ||
					(fw_ver[0] == G2M_HW_BASE_VER &&
					fw_ver[1] < G2M_SW_BASE_VER))
					fw_isp_update |= true;
			} else if (touch_id == 2) {	/* GFF Digitec */
				if (fw_ver[0] < GFD_HW_BASE_VER ||
					(fw_ver[0] == GFD_HW_BASE_VER &&
					fw_ver[1] < GFD_SW_BASE_VER))
					fw_isp_update |= true;
			} else if (touch_id == 3) {
				pr_info("[TSP] touch_id=3 pannel is detached");
				return 0;
			}
		}

		if (!fw_isp_update && system_rev >= 2 && !fw_isc_update) {
			if (fw_ver[3] < CORE_VER)
				fw_isc_update |= 0x01;

			if ((touch_id == 0 && fw_ver[4] < GFS_PRIVATE_VER) ||
				(touch_id == 1
					&& fw_ver[4] < G2M_PRIVATE_VER) ||
				(touch_id == 2
					&& fw_ver[4] < GFD_PRIVATE_VER) ||
				(touch_id == 3 && fw_ver[4] < G2W_PRIVATE_VER))
				fw_isc_update |= 0x02;

			if ((touch_id == 0 && fw_ver[5] < GFS_PUBLIC_VER) ||
				(touch_id == 1 && fw_ver[5] < G2M_PUBLIC_VER) ||
				(touch_id == 2 && fw_ver[5] < GFD_PUBLIC_VER) ||
				(touch_id == 3 && fw_ver[5] < G2W_PUBLIC_VER))
				fw_isc_update |= 0x04;
		}
	}

	if (!fw_isp_update && !fw_isc_update) {
		pr_info("[TSP] ISC & ISP Download ALL skip ");
		return 0;
	} else
		pr_info("[TSP] ISP D/L mode %s & ISC D/L mode %s [%x]",
				fw_isp_update ? "ON" : "OFF",
				fw_isc_update ? "ON" : "OFF", fw_isc_update);

	ts->set_touch_i2c_to_gpio();

	if (fw_isp_update)
		ret = mcsdl_download_binary_data(touch_id);
	else if (fw_isc_update) {
		pr_info("[TSP] fw_isc_update bits = [%x]", fw_isc_update);
		ret = mms100_ISC_download_binary_data(touch_id, fw_isc_update);
		if (ret) {
			pr_info("[TSP] ISC Fail & Try ISP mode D/L[%d]", ret);
			ret = mcsdl_download_binary_data(touch_id);
			if (ret)
				pr_info("[TSP] ISC & ISP D/L Fail [%d]", ret);
		}
	}

	ts->set_touch_i2c();
	msleep(100);

	/* reset chip */
	ts->power_off();
	msleep(200);
	ts->power_on();
	msleep(100);

	ret = check_detail_firmware(ts, fw_ver);
	if (ret)
		pr_err("[TSP] check_firmware fail! [%d]", ret);
	else
		pr_info("[TSP] FW HW:[%x],SW:[%x],Core:[%x],Pri:[%x],Pub:[%x]",
			fw_ver[0], fw_ver[1], fw_ver[3], fw_ver[4], fw_ver[5]);

#endif

	return ret;
}

#if TOUCH_BOOSTER
static void free_dvfs_lock(struct work_struct *work)
{

	struct melfas_ts_data *ts = container_of(work,
			struct melfas_ts_data, dvfs_work.work);

	mutex_lock(&ts->m_lock);
	exynos4_busfreq_lock_free(DVFS_LOCK_ID_TSP);
	exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
	ts->dvfs_lock_status = false;
	pr_info("[TSP] DVFS Off!");
	mutex_unlock(&ts->m_lock);
}

static void set_dvfs_lock(struct melfas_ts_data *ts, uint32_t on)
{
	mutex_lock(&ts->m_lock);
	if (ts->cpufreq_level <= 0) {
#ifdef CONFIG_TARGET_LOCALE_P2TMO_TEMP
		/*dvfs freq is temp modified to resolve dvfs kernel panic*/
		exynos_cpufreq_get_level(800000, &ts->cpufreq_level);
#else
		exynos_cpufreq_get_level(500000, &ts->cpufreq_level);
#endif
	}
	if (on == 0) {
		if (ts->dvfs_lock_status)
			schedule_delayed_work(&ts->dvfs_work,
						SEC_DVFS_LOCK_TIMEOUT * HZ);
	} else if (on == 1) {
		cancel_delayed_work(&ts->dvfs_work);
		if (!ts->dvfs_lock_status) {
			exynos4_busfreq_lock(DVFS_LOCK_ID_TSP, BUS_L1);
			exynos_cpufreq_lock(DVFS_LOCK_ID_TSP,
						ts->cpufreq_level);
			ts->dvfs_lock_status = true;
			pr_info("[TSP] DVFS On![%d]", ts->cpufreq_level);
		}
	} else if (on == 2) {
		cancel_delayed_work(&ts->dvfs_work);
		schedule_work(&ts->dvfs_work.work);
	}
	mutex_unlock(&ts->m_lock);
}
#endif

static void release_all_fingers(struct melfas_ts_data *ts)
{
	int i;

	for (i = 0; i < MELFAS_MAX_TOUCH; i++) {
		ts->finger_state[i] = TSP_STATE_RELEASE;
		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER,
									false);
	}
	input_sync(ts->input_dev);

#if TOUCH_BOOSTER
	set_dvfs_lock(ts, 2);
	pr_info("[TSP] release_all_fingers ");
#endif
}

static void inform_charger_connection(struct tsp_callbacks *_cb, int mode)
{
	struct melfas_ts_data *ts = container_of(_cb,
			struct melfas_ts_data, cb);
	char buf[2];
	buf[0] = TS_TA_STAT_ADDR;
	buf[1] = !!mode;

	if (ts->charging_status == !!mode) {
		pr_info("[TSP] %s call but not change status", __func__);
	} else {
		ts->charging_status = !!mode;

		pr_info("[TSP] %s : TSP %s & TA %sconnect ", __func__,
					ts->tsp_status ? "ON" : "OFF",
					ts->charging_status ? "" : "dis");

		if (ts->tsp_status) {
			melfas_i2c_write(ts->client, (char *)buf, 2);
			msleep(150);
		}
	}
}


static void reset_tsp(struct melfas_ts_data *ts)
{
	int	ta_status = ts->charging_status;
	char	buf[2];

	buf[0] = TS_TA_STAT_ADDR;
	buf[1] = ta_status;

	release_all_fingers(ts);

	ts->power_off();
	msleep(200);
	ts->power_on();
	msleep(200);

	pr_info("[TSP] reset_tsp & TA/USB %sconnect", ta_status ? "" : "dis");
	melfas_i2c_write(ts->client, (char *)buf, 2);
	msleep(150);

}

static void melfas_ts_read_input(struct melfas_ts_data *ts)
{
	int ret = 0, i;
	uint8_t buf[TS_READ_REGS_LEN];
	int touchStatus = 0;
	int read_num, id, posX, posY, str, width;
	int press_flag = 0;

#if DEBUG_PRINT
	pr_err("[TSP] melfas_ts_read_input\n");

	if (ts == NULL)
		pr_err("[TSP] melfas_ts_read_input : TS NULL\n");
#endif

#ifdef DEBUG_MODE
	if (debug_on) {
		logging_function(ts);
		return;
	}
#endif

	ret = read_input_info(ts, buf, TS_READ_START_ADDR, 1);
	if (ret < 0) {
		pr_err("[TSP] Failed to read the touch info\n");

		for (i = 0; i < P2_MAX_I2C_FAIL; i++) {
			ret = read_input_info(ts, buf, TS_READ_START_ADDR, 1);
			if (ret >= 0)
				break;
		}

		if (i == P2_MAX_I2C_FAIL) {	/* ESD Detection - I2c Fail */
			pr_err("[TSP] Melfas_ESD I2C FAIL\n");
			reset_tsp(ts);
			return;
		}
	}

	read_num = buf[0];
#if DEBUG_PRINT
	pr_info("[TSP]touch count :[%d]", read_num/6);
#endif

	if (read_num <= 0) {
		pr_err("[TSP] read_num error [%d]\n", read_num);
		return;
	}

	ret = read_input_info(ts, buf, TS_READ_START_ADDR2, read_num);
	if (ret < 0) {
		pr_err("[TSP] Failed to read the touch info");
		for (i = 0; i < P2_MAX_I2C_FAIL; i++) {
			ret = read_input_info(ts, buf,
				TS_READ_START_ADDR2, read_num);
			if (ret >= 0)
				break;
		}
		if (i == P2_MAX_I2C_FAIL) {
			pr_err("[TSP] Melfas_ESD I2C FAIL\n");
			reset_tsp(ts);
			return ;
		}
	}

	touchStatus = buf[0] & 0xFF;

	if (touchStatus == 0x0F) {
		pr_info("[TSP] TSP ESD Detection [%x]", buf[0]);
		reset_tsp(ts);
		return ;
	} else if (touchStatus == 0x1F) {
		pr_info("[TSP] TSP RF Noise Detection [%x]", buf[0]);
		return ;
	}

	for (i = 0; i < read_num; i = i+6) {
		id = (buf[i] & 0x0F)-1;
		posX = TS_MAX_X_COORD -
				((u16)(buf[i+1] & 0xF0) << 4 | buf[i+3]);
		posY = (u16)(buf[i+1] & 0x0F) << 8 | buf[i+2];
		str = buf[i + 4];
		width = buf[i+5];

		if ((buf[i] & 0x80) == TSP_STATE_RELEASE) {
			if (ts->finger_state[id] == TSP_STATE_RELEASE) {
				pr_err("[TSP] abnormal release");
				continue;
			}
			input_mt_slot(ts->input_dev, id);
			input_mt_report_slot_state(ts->input_dev,
						MT_TOOL_FINGER, false);
#if SHOW_COORD
			pr_info("[TSP] R [%d],([%4d],[%3d]),S:%d W:%d (%d)",
					id, posX, posY, str, width,
					ts->finger_state[id]);
#else
			pr_info("[TSP] R [%d] (%d)", id, ts->finger_state[id]);
#endif
			ts->finger_state[id] = TSP_STATE_RELEASE;
		} else {
			input_mt_slot(ts->input_dev, id);
			input_mt_report_slot_state(ts->input_dev,
						MT_TOOL_FINGER, true);
			input_report_abs(ts->input_dev,
						ABS_MT_POSITION_X, posX);
			input_report_abs(ts->input_dev,
						ABS_MT_POSITION_Y, posY);
			input_report_abs(ts->input_dev,
						ABS_MT_TOUCH_MAJOR, str);
			input_report_abs(ts->input_dev,
						ABS_MT_WIDTH_MAJOR, width);

			if (ts->finger_state[id] == TSP_STATE_RELEASE) {
#if SHOW_COORD
				pr_info("[TSP] P [%d],([%4d],[%3d]),S:%d W:%d",
						id, posX, posY, str, width);
#else
				pr_info("[TSP] P [%d]", id);
#endif
				ts->finger_state[id] = TSP_STATE_PRESS;
			} else if (ts->finger_state[id] == TSP_STATE_PRESS)
				ts->finger_state[id] = TSP_STATE_MOVE;
		}
	}
	input_sync(ts->input_dev);

	for (i = 0 ; i < MELFAS_MAX_TOUCH ; i++) {
		if (ts->finger_state[i] == TSP_STATE_PRESS
			|| ts->finger_state[i] == TSP_STATE_MOVE) {
			press_flag = 1;
			break;
		}
	}

#if TOUCH_BOOSTER
	set_dvfs_lock(ts, press_flag);
#endif
}

static irqreturn_t melfas_ts_irq_handler(int irq, void *handle)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)handle;
#if DEBUG_PRINT
	pr_err("[TSP] melfas_ts_irq_handler");
#endif

	melfas_ts_read_input(ts);
	return IRQ_HANDLED;
}

static ssize_t show_firm_version_panel(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	u8 fw_ver[6] = {0,};
	int ret;

	ret = check_detail_firmware(ts, fw_ver);
	if (ret)
		pr_err("[TSP] show_firm_version_panel fail! [%d]", ret);
	else
		pr_info("[TSP] show_firm_version_panel [%x][%x],[%x][%x][%x]",
			fw_ver[0], fw_ver[1], fw_ver[3], fw_ver[4], fw_ver[5]);

	if (ts->touch_id == 0)
		return snprintf(buf, PAGE_SIZE,
				"GFS_%2.2Xx%2.2X\n", fw_ver[0], fw_ver[1]);
	else if (ts->touch_id == 1)
		return snprintf(buf, PAGE_SIZE,
				"G2M_%2.2Xx%2.2X\n", fw_ver[0], fw_ver[1]);
	else if (ts->touch_id == 2)
		return snprintf(buf, PAGE_SIZE,
				"GFD_%2.2Xx%2.2X\n", fw_ver[0], fw_ver[1]);
	else if (ts->touch_id == 3)
		return snprintf(buf, PAGE_SIZE,
				"G2W_%2.2Xx%2.2X\n", fw_ver[0], fw_ver[1]);
	else
		return snprintf(buf, PAGE_SIZE,
				"MEL_%2.2Xx%2.2X\n", fw_ver[0], fw_ver[1]);
	return 0;
}

static ssize_t show_firm_version_phone(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);

	if (ts->touch_id == 0)
		return snprintf(buf, PAGE_SIZE,
				"GFS_%2.2Xx%2.2X\n", GFS_HW_VER, GFS_SW_VER);
	else if (ts->touch_id == 1)
		return snprintf(buf, PAGE_SIZE,
				"G2M_%2.2Xx%2.2X\n", G2M_HW_VER, G2M_SW_VER);
	else if (ts->touch_id == 2)
		return snprintf(buf, PAGE_SIZE,
				"GFD_%2.2Xx%2.2X\n", GFD_HW_VER, GFD_SW_VER);
	else if (ts->touch_id == 3)
		return snprintf(buf, PAGE_SIZE,
				"G2W_%2.2Xx%2.2X\n", G2W_HW_VER, G2W_SW_VER);
	return 0;
}

static ssize_t show_firm_update_status(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{

	int count;
	pr_info("[TSP] Enter firmware_status_show by Factory command\n");

	if (firm_status_data == 1)
		count = sprintf(buf, "DOWNLOADING\n");
	else if (firm_status_data == 2)
		count = sprintf(buf, "PASS\n");
	else if (firm_status_data == 3)
		count = sprintf(buf, "FAIL\n");
	else
		count = sprintf(buf, "PASS\n");

	return count;
}

static ssize_t tsp_firm_update(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf,
			       size_t count)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	u8 fw_ver[6] = {0,};
	int ret = 0;

#if SET_DOWNLOAD_BY_GPIO

	disable_irq(ts->client->irq);

	firm_status_data = 1;

	ts->set_touch_i2c_to_gpio();

	pr_info("[TSP] ADB F/W UPDATE MODE ENTER! :%s", buf);
	if (*buf == 'F')
		ret = mcsdl_download_binary_data(ts->touch_id);
	else if (*buf == '0') {
		pr_info("[TSP] GFS F/W UPDATE !");
		ret = mcsdl_download_binary_data(0);
	} else if (*buf == '1') {
		pr_info("[TSP] G2M F/W UPDATE !");
		ret = mcsdl_download_binary_data(1);
	} else if (*buf == '2') {
		pr_info("[TSP] GFD F/W UPDATE !");
		ret = mcsdl_download_binary_data(2);
	} else if (*buf == '3') {
		pr_info("[TSP] G2W F/W UPDATE !");
		ret = mcsdl_download_binary_data(3);
	} else
		ret = mcsdl_download_binary_file();

	pr_info("[TSP] ADB F/W UPDATE MODE FROM %s END! %s",
		(*buf == 'F' ? "BINARY" : "FILE"), (ret ? "fail" : "success"));

	firm_status_data = (ret ? 3 : 2);

	ts->set_touch_i2c();

	reset_tsp(ts);

	enable_irq(ts->client->irq);
#endif

	ret = check_detail_firmware(ts, fw_ver);
	if (ret)
		pr_err("[TSP] check_firmware fail! [%d]", ret);
	else
		pr_info("[TSP] FW HW:[%x],SW:[%x],Core:[%x],Pri:[%x],Pub:[%x]",
			fw_ver[0], fw_ver[1], fw_ver[3], fw_ver[4], fw_ver[5]);

	return count;
}

static ssize_t tsp_firm_verify(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf,
			       size_t count)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	int ret = 0;

#if SET_DOWNLOAD_BY_GPIO

	disable_irq(ts->client->irq);

	ts->set_touch_i2c_to_gpio();

	pr_info("[TSP] ADB F/W Verify MODE ENTER! :%s", buf);
	if (*buf == 'F')
		ret = mcsdl_download_binary_data_verify(ts->touch_id);
	else if (*buf == '0') {
		pr_info("[TSP] GFS F/W Verify !");
		ret = mcsdl_download_binary_data_verify(0);
	} else if (*buf == '1') {
		pr_info("[TSP] G2M F/W Verify !");
		ret = mcsdl_download_binary_data_verify(1);
	} else if (*buf == '2') {
		pr_info("[TSP] GFD F/W Verify !");
		ret = mcsdl_download_binary_data_verify(2);
	} else if (*buf == '3') {
		pr_info("[TSP] G2W F/W Verify !");
		ret = mcsdl_download_binary_data_verify(3);
	} else
		pr_info("[TSP] ADB F/W Verify MODE file error");

	pr_info("[TSP] ADB F/W Verify %s", (ret ? "Fail" : "Success"));

	ts->set_touch_i2c();

	reset_tsp(ts);

	enable_irq(ts->client->irq);
#endif
	return count;
}


static ssize_t store_debug_mode(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	char ch;

	if (sscanf(buf, "%c", &ch) != 1)
		return -EINVAL;

	key_handler(ts, ch);

	return count;
}

static ssize_t show_debug_mode(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, debug_on ? "ON\n" : "OFF\n");
}

static ssize_t store_debug_log(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	int i;

	if (sscanf(buf, "%d", &i) != 1)
		return -EINVAL;

	if (i)
		debug_print = 1;
	else
		debug_print = 0;

	pr_info("[TSP] debug log %s", i ? "ON" : "OFF");

	return count;
}

static ssize_t show_threshold(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);
	u8 threshold;
	melfas_i2c_read(ts->client, TS_THRESHOLD, 1, &threshold);

	return sprintf(buf, "%d\n", threshold);
}


static DEVICE_ATTR(tsp_threshold, S_IRUGO, show_threshold, NULL);
static DEVICE_ATTR(tsp_firm_update, S_IWUSR | S_IWGRP,
					NULL, tsp_firm_update);
static DEVICE_ATTR(tsp_firm_verify, S_IWUSR | S_IWGRP,
					NULL, tsp_firm_verify);
static DEVICE_ATTR(tsp_firm_update_status, S_IRUGO,
					show_firm_update_status, NULL);
static DEVICE_ATTR(tsp_firm_version_phone, S_IRUGO,
					show_firm_version_phone, NULL);
static DEVICE_ATTR(tsp_firm_version_panel, S_IRUGO,
					show_firm_version_panel, NULL);
static DEVICE_ATTR(debug_mode, S_IWUSR|S_IRUGO,
					show_debug_mode, store_debug_mode);
static DEVICE_ATTR(debug_log, S_IWUSR|S_IRUGO, NULL, store_debug_log);


#ifdef ENABLE_NOISE_TEST_MODE
static DEVICE_ATTR(set_threshould, S_IRUGO, show_threshold, NULL);
#else
static DEVICE_ATTR(threshold, S_IRUGO, show_threshold, NULL);
#endif

static u16 index_reference;
static u16 reference_data[X_LINE*Y_LINE] = { 0, };
static u16 intensity_data[X_LINE*Y_LINE] = { 0, };
static u16 inspection_data[X_LINE*Y_LINE] = { 0, };

static int check_debug_data(struct melfas_ts_data *ts)
{
	u8 write_buffer[6];
	u8 read_buffer[2];
	int sensing_line, exciting_line;
	int gpio = ts->pdata->gpio_int;
	int count = 0;

	disable_irq(ts->client->irq);

	/* enter the debug mode */
	write_buffer[0] = 0xA0;
	write_buffer[1] = 0x1A;
	write_buffer[2] = 0x0;
	write_buffer[3] = 0x0;
	write_buffer[4] = 0x0;
	write_buffer[5] = 0x01;
	melfas_i2c_write(ts->client, (char *)write_buffer, 6);

	/* wating for the interrupt*/
	while (gpio_get_value(gpio)) {
		printk(".");
		udelay(100);
		count++;
		if (count == 1000) {
			enable_irq(ts->client->irq);
			return -1;
		}
	}

	if (debug_print)
		pr_info("[TSP] read dummy\n");

	/* read the dummy data */
	melfas_i2c_read(ts->client, 0xA8, 2, read_buffer);

	if (debug_print)
		pr_info("[TSP] read inspenction data\n");
	write_buffer[5] = 0x02;
	for (sensing_line = 0; sensing_line < X_LINE; sensing_line++) {
		for (exciting_line = 0; exciting_line < Y_LINE;
							exciting_line++) {
			write_buffer[2] = exciting_line;
			write_buffer[3] = sensing_line;
			melfas_i2c_write(ts->client, (char *)write_buffer, 6);
			melfas_i2c_read(ts->client, 0xA8, 2, read_buffer);
			reference_data[exciting_line + sensing_line * Y_LINE] =
				(read_buffer[1] & 0xf) << 8 | read_buffer[0];
		}
	}

	reset_tsp(ts);
	msleep(200);
	enable_irq(ts->client->irq);
	return 0;
}

static ssize_t all_refer_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	int status = 0;
	int i;
	struct melfas_ts_data *ts  = dev_get_drvdata(dev);

	for (i = 0; i < 3; i++) {
		if (!check_debug_data(ts)) {
			status = 0;
			break;
		} else {
			pr_info("[TSP] check_debug_data Error try=%d", i);
			reset_tsp(ts);
			msleep(300);
			status = 1;
		}
	}

	if (!status) {
		for (i = 0; i < X_LINE*Y_LINE; i++) {
			/* out of range */
			if (reference_data[i] < 30) {
				status |= 1;
				break;
			}

			if (debug_print) {
				if (0 == i % Y_LINE)
					printk("\n");
				printk(KERN_INFO "%5u ", reference_data[i]);
			}
		}
	} else {
		pr_info("[TSP] all_refer_show& check_debug_data error[%d]",
					status);
		return sprintf(buf, "%u\n", status);
	}

	pr_info("[TSP] all_refer_show func [%d]", status);
	return sprintf(buf, "%u\n", status);
}

static void check_intesity_data(struct melfas_ts_data *ts)
{
	u8 write_buffer[6];
	u8 read_buffer[2];
	int sensing_line, exciting_line;
	int gpio = ts->pdata->gpio_int;

	if (0 == reference_data[0]) {
		disable_irq(ts->client->irq);

		/* enter the debug mode */
		write_buffer[0] = 0xA0;
		write_buffer[1] = 0x1A;
		write_buffer[2] = 0x0;
		write_buffer[3] = 0x0;
		write_buffer[4] = 0x0;
		write_buffer[5] = 0x01;
		melfas_i2c_write(ts->client, (char *)write_buffer, 6);

		/* wating for the interrupt*/
		while (gpio_get_value(gpio)) {
			printk(".");
			udelay(100);
		}

		/* read the dummy data */
		melfas_i2c_read(ts->client, 0xA8, 2, read_buffer);

		if (debug_print)
			pr_info("[TSP] read the dummy data\n");

		write_buffer[5] = 0x02;
		for (sensing_line = 0; sensing_line < X_LINE; sensing_line++) {
			for (exciting_line = 0; exciting_line < Y_LINE;
							exciting_line++) {
				write_buffer[2] = exciting_line;
				write_buffer[3] = sensing_line;
				melfas_i2c_write(ts->client,
						(char *)write_buffer, 6);
				melfas_i2c_read(ts->client, 0xA8, 2,
								read_buffer);
				reference_data[exciting_line +
						sensing_line * Y_LINE] =
					(read_buffer[1] & 0xf) << 8
						| read_buffer[0];
			}
		}
		reset_tsp(ts);
		msleep(100);
		enable_irq(ts->client->irq);
		msleep(100);
	}

	disable_irq(ts->client->irq);
	release_all_fingers(ts);

	write_buffer[0] = 0xA0;
	write_buffer[1] = 0x1A;
	write_buffer[4] = 0x0;
	write_buffer[5] = 0x04;
	for (sensing_line = 0; sensing_line < X_LINE; sensing_line++) {
		for (exciting_line = 0; exciting_line < Y_LINE;
							exciting_line++) {
			write_buffer[2] = exciting_line;
			write_buffer[3] = sensing_line;
			melfas_i2c_write(ts->client, (char *)write_buffer, 6);
			melfas_i2c_read(ts->client, 0xA8, 2, read_buffer);
			intensity_data[exciting_line + sensing_line * Y_LINE] =
				(read_buffer[1] & 0xf) << 8 | read_buffer[0];
		}
	}
	enable_irq(ts->client->irq);
}

static ssize_t set_refer0_mode_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	u16 refrence = 0;
	struct melfas_ts_data *ts = dev_get_drvdata(dev);

	check_intesity_data(ts);

	refrence = reference_data[95];
	return sprintf(buf, "%u\n", refrence);
}

static ssize_t set_refer1_mode_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	u16 refrence = 0;
	refrence = reference_data[529];
	return sprintf(buf, "%u\n", refrence);
}

static ssize_t set_refer2_mode_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	u16 refrence = 0;
	refrence = reference_data[294];
	return sprintf(buf, "%u\n", refrence);
}

static ssize_t set_refer3_mode_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	u16 refrence = 0;
	refrence = reference_data[89];
	return sprintf(buf, "%u\n", refrence);
}

static ssize_t set_refer4_mode_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	u16 refrence = 0;
	refrence = reference_data[554];
	return sprintf(buf, "%u\n", refrence);
}

static ssize_t set_intensity0_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 intensity = 0;
	intensity = intensity_data[95];
	return sprintf(buf, "%u\n", intensity);
}

static ssize_t set_intensity1_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 intensity = 0;
	intensity = intensity_data[529];
	return sprintf(buf, "%u\n", intensity);
}

static ssize_t set_intensity2_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 intensity = 0;
	intensity = intensity_data[294];
	return sprintf(buf, "%u\n", intensity);
}

static ssize_t set_intensity3_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 intensity = 0;
	intensity = intensity_data[89];
	return sprintf(buf, "%u\n", intensity);
}

static ssize_t set_intensity4_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u16 intensity = 0;
	intensity = intensity_data[554];
	return sprintf(buf, "%u\n", intensity);
}

static ssize_t tsp_power_control(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	struct melfas_ts_data *ts = dev_get_drvdata(dev);

	int		ta_status = ts->charging_status;
	char	writebuf[2];

	writebuf[0] = TS_TA_STAT_ADDR;
	writebuf[1] = ta_status;

	if (*buf == '0' && ts->tsp_status == true) {
		ts->tsp_status = false;
		release_all_fingers(ts);
		ts->power_off();
		msleep(200);
	} else if (*buf == '1' && ts->tsp_status == false) {
		ts->power_on();
		msleep(200);
		melfas_i2c_write(ts->client, (char *)writebuf, 2);
		msleep(150);
		ts->tsp_status = true;
	} else
		pr_info("[TSP]tsp_power_control bad command!");
	return count;
}

static ssize_t show_tsp_info(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%s\n", TSP_CHIP_VENDER_NAME);
}

static ssize_t show_tsp_x_line_info(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", X_LINE);
}

static ssize_t show_tsp_y_line_info(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", Y_LINE);
}

static int atoi(const char *str)
{
	int result = 0;
	int count = 0;
	char count_val = str[count];

	if (str == NULL)
		return -1;
	while (str != NULL && count_val >= '0' && count_val <= '9') {
		result = result * 10 + count_val - '0';
		++count;
	}
	return result;
}

static ssize_t set_debug_data1(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{

	struct melfas_ts_data *ts = dev_get_drvdata(dev);

	u8 write_buffer[6];
	u8 read_buffer[2];
	int sensing_line, exciting_line;
	int gpio = ts->pdata->gpio_int;

	if (!ts->tsp_status) {
		pr_info("[TSP] call set_debug_data1 but TSP status OFF!");
		return count;
	}

	disable_irq(ts->client->irq);

	/* enter the debug mode */
	write_buffer[0] = 0xA0;
	write_buffer[1] = 0x1A;
	write_buffer[2] = 0x0;
	write_buffer[3] = 0x0;
	write_buffer[4] = 0x0;
	write_buffer[5] = 0x01;
	melfas_i2c_write(ts->client, (char *)write_buffer, 6);

	/* wating for the interrupt*/
	while (gpio_get_value(gpio)) {
		printk(".");
		udelay(100);
	}

	/* read the dummy data */
	melfas_i2c_read(ts->client, 0xA8, 2, read_buffer);

	pr_info("[TSP] read Reference data\n");
	write_buffer[5] = 0x02;
	for (sensing_line = 0; sensing_line < X_LINE; sensing_line++) {
		for (exciting_line = 0; exciting_line < Y_LINE;
							exciting_line++) {
			write_buffer[2] = exciting_line;
			write_buffer[3] = sensing_line;
			melfas_i2c_write(ts->client, (char *)write_buffer, 6);
			melfas_i2c_read(ts->client, 0xA8, 2, read_buffer);
			reference_data[exciting_line + sensing_line * Y_LINE] =
				(read_buffer[1] & 0xf) << 8 | read_buffer[0];
		}
	}

	reset_tsp(ts);
	msleep(200);
	enable_irq(ts->client->irq);
	return count;
}

static ssize_t set_debug_data2(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{

	struct melfas_ts_data *ts = dev_get_drvdata(dev);

	u8 write_buffer[6];
	u8 read_buffer[2];
	int sensing_line, exciting_line;
	int gpio = ts->pdata->gpio_int;

	if (!ts->tsp_status) {
		pr_info("[TSP] call set_debug_data2 but TSP status OFF!");
		return count;
	}

	disable_irq(ts->client->irq);

	/* enter the debug mode */
	write_buffer[0] = 0xA0;
	write_buffer[1] = 0x1A;
	write_buffer[2] = 0x0;
	write_buffer[3] = 0x0;
	write_buffer[4] = 0x0;
	write_buffer[5] = 0x01;
	melfas_i2c_write(ts->client, (char *)write_buffer, 6);

	/* wating for the interrupt*/
	while (gpio_get_value(gpio)) {
		printk(".");
		udelay(100);
	}

	/* read the dummy data */
	melfas_i2c_read(ts->client, 0xA8, 2, read_buffer);

	pr_info("[TSP] read Inspection data\n");
	write_buffer[5] = 0x03;
	for (sensing_line = 0; sensing_line < X_LINE; sensing_line++) {
		for (exciting_line = 0; exciting_line < Y_LINE;
							exciting_line++) {
			write_buffer[2] = exciting_line;
			write_buffer[3] = sensing_line;
			melfas_i2c_write(ts->client, (char *)write_buffer, 6);
			melfas_i2c_read(ts->client, 0xA8, 2, read_buffer);
			inspection_data[exciting_line +
						sensing_line * Y_LINE] =
				(read_buffer[1] & 0xf) << 8 | read_buffer[0];
		}
	}
	reset_tsp(ts);
	msleep(200);
	enable_irq(ts->client->irq);
	return count;
}

static ssize_t set_debug_data3(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{

	struct melfas_ts_data *ts = dev_get_drvdata(dev);

	u8 write_buffer[6];
	u8 read_buffer[2];
	int sensing_line, exciting_line;

	if (!ts->tsp_status) {
		pr_info("[TSP] call set_debug_data3 but TSP status OFF!");
		return count;
	}

	pr_info("[TSP] read lntensity data\n");

	disable_irq(ts->client->irq);

	/* enter the debug mode */
	write_buffer[0] = 0xA0;
	write_buffer[1] = 0x1A;
	write_buffer[2] = 0x0;
	write_buffer[3] = 0x0;
	write_buffer[4] = 0x0;
	write_buffer[5] = 0x04;
	for (sensing_line = 0; sensing_line < X_LINE; sensing_line++) {
		for (exciting_line = 0; exciting_line < Y_LINE;
							exciting_line++) {
			write_buffer[2] = exciting_line;
			write_buffer[3] = sensing_line;
			melfas_i2c_write(ts->client, (char *)write_buffer, 6);
			melfas_i2c_read(ts->client, 0xA8, 2, read_buffer);
			intensity_data[exciting_line + sensing_line * Y_LINE] =
				(read_buffer[1] & 0xf) << 8 | read_buffer[0];
		}
	}

	reset_tsp(ts);
	msleep(200);
	enable_irq(ts->client->irq);
	return count;
}



static ssize_t set_index_reference(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	index_reference = atoi(buf);
	if (index_reference < 0 || index_reference >= X_LINE*Y_LINE) {
		pr_info("[TSP] input bad index_reference value");
		return -1;
	} else {
		pr_info("[TSP]index_reference =%d ", index_reference);
		return count;
	}
}
static ssize_t show_reference_info(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int i = 0;
	if (debug_print) {
		for (i = 0; i < X_LINE*Y_LINE; i++) {
			if (0 == i % Y_LINE)
				printk(KERN_INFO "\n");
			printk(KERN_INFO "%4u", reference_data[i]);
		}
	}
	return sprintf(buf, "%d\n", reference_data[index_reference]);
}
static ssize_t show_inspection_info(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int i = 0;
	if (debug_print) {
		for (i = 0; i < X_LINE*Y_LINE; i++) {
			if (0 == i % Y_LINE)
				printk(KERN_INFO "\n");
			printk(KERN_INFO "%5u", inspection_data[i]);
		}
	}
	return sprintf(buf, "%d\n", inspection_data[index_reference]);
}
static ssize_t show_intensity_info(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int i = 0, max_idx = 0, max_val = 0;
	if (debug_print) {
		for (i = 0; i < X_LINE*Y_LINE; i++) {
			if (max_val < intensity_data[i]) {
				max_val = intensity_data[i];
				max_idx = i;
			}
			if (0 == i % Y_LINE)
				printk(KERN_INFO "\n");
			printk(KERN_INFO "%4u", intensity_data[i]);
		}
		pr_info("[TSP] max val=[%d] , index=[%d] ", max_val, max_idx);
	}
	return sprintf(buf, "%d\n", intensity_data[index_reference]);
}

static DEVICE_ATTR(set_all_refer, S_IRUGO, all_refer_show, NULL);
static DEVICE_ATTR(set_refer0, S_IRUGO, set_refer0_mode_show, NULL);
static DEVICE_ATTR(set_delta0, S_IRUGO, set_intensity0_mode_show, NULL);
static DEVICE_ATTR(set_refer1, S_IRUGO, set_refer1_mode_show, NULL);
static DEVICE_ATTR(set_delta1, S_IRUGO, set_intensity1_mode_show, NULL);
static DEVICE_ATTR(set_refer2, S_IRUGO, set_refer2_mode_show, NULL);
static DEVICE_ATTR(set_delta2, S_IRUGO, set_intensity2_mode_show, NULL);
static DEVICE_ATTR(set_refer3, S_IRUGO, set_refer3_mode_show, NULL);
static DEVICE_ATTR(set_delta3, S_IRUGO, set_intensity3_mode_show, NULL);
static DEVICE_ATTR(set_refer4, S_IRUGO, set_refer4_mode_show, NULL);
static DEVICE_ATTR(set_delta4, S_IRUGO, set_intensity4_mode_show, NULL);
static DEVICE_ATTR(tsp_info, S_IRUGO, show_tsp_info, NULL);
static DEVICE_ATTR(tsp_x_line, S_IRUGO, show_tsp_x_line_info, NULL);
static DEVICE_ATTR(tsp_y_line, S_IRUGO, show_tsp_y_line_info, NULL);
static DEVICE_ATTR(tsp_power, S_IWUSR | S_IWGRP, NULL, tsp_power_control);
static DEVICE_ATTR(set_debug_data1, S_IWUSR | S_IWGRP, NULL, set_debug_data1);
static DEVICE_ATTR(set_debug_data2, S_IWUSR | S_IWGRP, NULL, set_debug_data2);
static DEVICE_ATTR(set_debug_data3, S_IWUSR | S_IWGRP, NULL, set_debug_data3);
static DEVICE_ATTR(set_index_ref, S_IWUSR | S_IWGRP,
				NULL, set_index_reference);
static DEVICE_ATTR(show_reference_info, S_IRUGO, show_reference_info, NULL);
static DEVICE_ATTR(show_inspection_info, S_IRUGO, show_inspection_info, NULL);
static DEVICE_ATTR(show_intensity_info, S_IRUGO, show_intensity_info, NULL);


static struct attribute *sec_touch_attributes[] = {
	&dev_attr_tsp_threshold.attr,
	&dev_attr_tsp_firm_update.attr,
	&dev_attr_tsp_firm_verify.attr,
	&dev_attr_tsp_firm_update_status.attr,
	&dev_attr_tsp_firm_version_phone.attr,
	&dev_attr_tsp_firm_version_panel.attr,
	&dev_attr_debug_mode.attr,
	&dev_attr_debug_log.attr,
#ifndef ENABLE_NOISE_TEST_MODE
	&dev_attr_threshold.attr,
	&dev_attr_set_all_refer.attr,
	&dev_attr_set_refer0.attr,
	&dev_attr_set_delta0.attr,
	&dev_attr_set_refer1.attr,
	&dev_attr_set_delta1.attr,
	&dev_attr_set_refer2.attr,
	&dev_attr_set_delta2.attr,
	&dev_attr_set_refer3.attr,
	&dev_attr_set_delta3.attr,
	&dev_attr_set_refer4.attr,
	&dev_attr_set_delta4.attr,
#endif
	NULL,
};

static struct attribute_group sec_touch_attr_group = {
	.attrs = sec_touch_attributes,
};

#ifdef ENABLE_NOISE_TEST_MODE
static struct attribute *sec_touch_facotry_attributes[] = {
	&dev_attr_set_all_refer.attr,
	&dev_attr_set_refer0.attr,
	&dev_attr_set_delta0.attr,
	&dev_attr_set_refer1.attr,
	&dev_attr_set_delta1.attr,
	&dev_attr_set_refer2.attr,
	&dev_attr_set_delta2.attr,
	&dev_attr_set_refer3.attr,
	&dev_attr_set_delta3.attr,
	&dev_attr_set_refer4.attr,
	&dev_attr_set_delta4.attr,
	&dev_attr_set_threshould.attr,
	&dev_attr_tsp_power.attr,
	&dev_attr_tsp_info.attr,
	&dev_attr_tsp_x_line.attr,
	&dev_attr_tsp_y_line.attr,
	&dev_attr_set_debug_data1.attr,
	&dev_attr_set_debug_data2.attr,
	&dev_attr_set_debug_data3.attr,
	&dev_attr_set_index_ref.attr,
	&dev_attr_show_reference_info.attr,
	&dev_attr_show_inspection_info.attr,
	&dev_attr_show_intensity_info.attr,
	NULL,
};

static struct attribute_group sec_touch_factory_attr_group = {
	.attrs = sec_touch_facotry_attributes,
};
#endif


static int melfas_ts_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{

	struct ts_platform_data *pdata = client->dev.platform_data;
	struct melfas_ts_data *ts;
	struct device *tsp_dev;

#ifdef ENABLE_NOISE_TEST_MODE
	struct device *test_dev;
#endif

	int ret = 0, i, irq;
	const char buf;

#if DEBUG_PRINT
	pr_err("[TSP] kim ms : melfas_ts_probe\n");
#endif

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[TSP] melfas_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kmalloc(sizeof(struct melfas_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		pr_err("[TSP] failed to create a state of melfas-ts");
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	ts->pdata = client->dev.platform_data;

	ts->power_on = pdata->power_on;
	ts->power_off = pdata->power_off;
	ts->read_ta_status = pdata->read_ta_status;
	ts->set_touch_i2c = pdata->set_touch_i2c;
	ts->set_touch_i2c_to_gpio = pdata->set_touch_i2c_to_gpio;
	ts->touch_id = pdata->gpio_touch_id;
	ts->tsp_status = true;

	mutex_init(&ts->m_lock);

	ts->cb.inform_charger = inform_charger_connection;
	if (pdata->register_cb)
		pdata->register_cb(&ts->cb);

	/* reset chip */
	gpio_set_value(GPIO_TSP_RST, 0);
	msleep(200);
	gpio_set_value(GPIO_TSP_RST, 1);
	msleep(100);


	ts->client = client;
	i2c_set_clientdata(client, ts);
	ret = i2c_master_send(ts->client, &buf, 1);


#if DEBUG_PRINT
	pr_err("[TSP] melfas_ts_probe: i2c_master_send() [%d], Add[%d]\n",
						ret, ts->client->addr);
#endif

	ret = firmware_update(ts);

	ts->input_dev = input_allocate_device();
	if (!ts->input_dev) {
		pr_err("[TSP] melfas_ts_probe: Not enough memory\n");
		ret = -ENOMEM;
		goto err_input_dev_alloc_failed;
	}

	ts->input_dev->name = "sec_touchscreen" ;

	__set_bit(EV_ABS, ts->input_dev->evbit);
	__set_bit(EV_KEY, ts->input_dev->evbit);

	input_mt_init_slots(ts->input_dev, MELFAS_MAX_TOUCH);
	input_set_abs_params(ts->input_dev,
				ABS_MT_POSITION_X, 0, TS_MAX_X_COORD, 0, 0);
	input_set_abs_params(ts->input_dev,
				ABS_MT_POSITION_Y, 0, TS_MAX_Y_COORD, 0, 0);
	input_set_abs_params(ts->input_dev,
				ABS_MT_TOUCH_MAJOR, 0, TS_MAX_Z_TOUCH, 0, 0);
	input_set_abs_params(ts->input_dev,
				ABS_MT_WIDTH_MAJOR, 0, TS_MAX_W_TOUCH, 0, 0);

	__set_bit(MT_TOOL_FINGER, ts->input_dev->keybit);
	__set_bit(EV_SYN, ts->input_dev->evbit);
	__set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);



	ret = input_register_device(ts->input_dev);
	if (ret) {
		pr_err("[TSP] melfas_ts_probe: Failed to register device\n");
		ret = -ENOMEM;
		goto err_input_register_device_failed;
	}

	if (ts->client->irq) {
		irq = ts->client->irq;

		ret = request_threaded_irq(irq, NULL, melfas_ts_irq_handler,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					ts->client->name, ts);
		if (ret) {
			pr_err("[TSP] %s: Can't allocate irq %d, ret %d\n",
					__func__, irq, ret);
			ret = -EBUSY;
			goto err_request_irq;
		}
	}

#if TOUCH_BOOSTER
	INIT_DELAYED_WORK(&ts->dvfs_work, free_dvfs_lock);
	ts->cpufreq_level = -1;
	ts->dvfs_lock_status = false;
#endif

	for (i = 0; i < MELFAS_MAX_TOUCH; i++)
		ts->finger_state[i] = TSP_STATE_RELEASE;

#if DEBUG_PRINT
	pr_err("[TSP] melfas_ts_probe: succeed to register input device\n");
#endif

/*  ---------------------- */
	tsp_dev  = device_create(sec_class, NULL, 0, ts, "sec_touchscreen");
	if (IS_ERR(tsp_dev))
		pr_err("[TSP] Failed to create device for the sysfs\n");

	ret = sysfs_create_group(&tsp_dev->kobj, &sec_touch_attr_group);
	if (ret)
		pr_err("[TSP] Failed to create sysfs group\n");
/*  ----------------------- */

#ifdef ENABLE_NOISE_TEST_MODE
	test_dev = device_create(sec_class, NULL, 0, ts, "tsp_noise_test");
	if (IS_ERR(test_dev)) {
		pr_err("Failed to create device for the factory test\n");
		ret = -ENODEV;
	}

	ret = sysfs_create_group(&test_dev->kobj,
				&sec_touch_factory_attr_group);
	if (ret)
		pr_err("Failed to create sysfs group for the factory test\n");
#endif


#if CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = melfas_ts_early_suspend;
	ts->early_suspend.resume = melfas_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

#if DEBUG_PRINT
	pr_info("melfas_ts_probe: Start touchscreen. name: %s, irq: %d\n",
				ts->client->name, ts->client->irq);
#endif
	return 0;

err_request_irq:
	pr_err("[TSP] melfas-ts: err_request_irq failed\n");
	free_irq(client->irq, ts);
err_input_register_device_failed:
	pr_err("[TSP] melfas-ts: err_input_register_device failed\n");
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:
	pr_err("[TSP] melfas-ts: err_input_dev_alloc failed\n");
err_alloc_data_failed:
	pr_err("[TSP] melfas-ts: err_alloc_data failed_\n");
	kfree(ts);
err_check_functionality_failed:
	pr_err("[TSP] melfas-ts: err_check_functionality failed_\n");

	return ret;
}

static int melfas_ts_remove(struct i2c_client *client)
{
	struct melfas_ts_data *ts = i2c_get_clientdata(client);

	unregister_early_suspend(&ts->early_suspend);
	free_irq(client->irq, ts);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	return 0;
}

static int melfas_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	char buf[2];
	struct melfas_ts_data *ts = i2c_get_clientdata(client);
	ts->tsp_status = false;

#ifdef DEBUG_MODE
	if (debug_on) {
		pr_info("[TSP] Out of debug-mode before suspend ");
		buf[0] = ADDR_CHANGE_PROTOCOL;
		buf[1] = 11;
		debug_i2c_write(ts->client, buf, 2);
		debug_on = false;
		msleep(150);
	}
#endif

	disable_irq(ts->client->irq);
	release_all_fingers(ts);

	ts->power_off();

	return 0;
}

static int melfas_ts_resume(struct i2c_client *client)
{
	struct melfas_ts_data *ts = i2c_get_clientdata(client);
	int ta_status = ts->charging_status;
	char buf[2];

	ts->power_on();
	msleep(100);

	if (ta_status) {
		buf[0] = TS_TA_STAT_ADDR;
		buf[1] = ta_status;
		melfas_i2c_write(ts->client, (char *)buf, 2);
		msleep(150);
	}

	enable_irq(ts->client->irq);
	pr_info("[TSP] melfas_ts_resume TA %sconnection",
				ta_status ? "" : "dis");

	ts->tsp_status = true;

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_ts_early_suspend(struct early_suspend *h)
{
	struct melfas_ts_data *ts;
	ts = container_of(h, struct melfas_ts_data, early_suspend);
	melfas_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void melfas_ts_late_resume(struct early_suspend *h)
{
	struct melfas_ts_data *ts;
	ts = container_of(h, struct melfas_ts_data, early_suspend);
	melfas_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id melfas_ts_id[] = {
	{ TS_DEV_NAME, 0 },
	{ }
};

static struct i2c_driver melfas_ts_driver = {
	.driver = {
		.name = TS_DEV_NAME,
	},
	.id_table = melfas_ts_id,
	.probe = melfas_ts_probe,
	.remove = __devexit_p(melfas_ts_remove),
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = melfas_ts_suspend,
	.resume = melfas_ts_resume,
#endif
};

static int __devinit melfas_ts_init(void)
{
	return i2c_add_driver(&melfas_ts_driver);
}

static void __exit melfas_ts_exit(void)
{
	i2c_del_driver(&melfas_ts_driver);
}

MODULE_DESCRIPTION("Driver for Melfas MTSI Touchscreen Controller");
MODULE_AUTHOR("MinSang, Kim <kimms@melfas.com>");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");

module_init(melfas_ts_init);
module_exit(melfas_ts_exit);
