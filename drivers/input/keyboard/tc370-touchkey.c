/*
 * CORERIVER TOUCHCORE touchkey driver
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 * Author: Taeyoon Yoon <tyoony.yoon@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

//#define LED_DEBUG
#define ISP_DEBUG
//#define ISP_VERBOSE_DEBUG
//#define ISP_VERY_VERBOSE_DEBUG
#define ISP_SPEED_UP

#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/i2c/tc370-touchkey.h>

#define TC370_FW_NAME		"tc370"
#define TC370_FW_BUILTIN_PATH	"coreriver"
#define TC370_FW_HEADER_PATH
#define TC370_FW_IN_SDCARD_PATH	"/mnt/sdcard/firmware/coreriver"
#define TC370_FW_EX_SDCARD_PATH	"/mnt/sdcard/external_sd/firmware/coreriver"

#define TC370_FW_FLASH_RETRY	3
#define TC370_POWERON_DELAY	100
#define TC370_DCR_RD_RETRY	50
#define TC370_FW_VER_READ	5

#define TC370_KEY_DATA		0x00
#define TC370_KEY_INDEX_MASK	0x03
#define TC370_KEY_PRESS_MASK	0x08

#define TC370_CMD		0x00
#define TC370_CMD_FW_VER	0x01
#define TC370_CMD_RAW_MENU	0x04
#define TC370_CMD_RAW_BACK	0x0C

#define TC370_RAW_DIFFDATA_OFFSET	0x04
#define TC370_RAW_CHPCT_OFFSET		0x02
#define TC370_RAW_RAWDATA_OFFSET	0x06
#define TC370_RAW_DATA_SIZE		0x08

#define TC370_ISP_ACK			1
#define TC370_ISP_SP_SIGNAL		0b010101011111000
#define TC370_NUM_OF_ISP_SP_SIGNAL	15

#define TC370_CMD_LED_ON		0x10
#define TC370_CMD_LED_OFF		0x20
#define TC370_CMD_SLEEP			0x80

#define TC370_FW_ER_MAX_LEN		0X8000
#define TC370_FW_WT_MAX_LEN		0X3000

enum {
	FW_BUILT_IN = 0,
	FW_HEADER,
	FW_IN_SDCARD,
	FW_EX_SDCARD,
};

enum {
	HAVE_LATEST_FW = 1,
	FW_UPDATE_RUNNING,
};

enum {
	STATE_NORMAL = 1,
	STATE_FLASH,
	STATE_FLASH_FAIL,
};

#if defined(SEC_FAC_TK)

#define TO_STRING(x) #x

enum {
	DOWNLOADING = 1,
	FAIL,
	PASS,
};

struct fdata_struct {
	struct device			*dummy_dev;
	u8				fw_flash_status;
};
#endif

struct fw_image {
	u8 hdr_ver;
	u8 hdr_len;
	u16 first_fw_ver;
	u16 second_fw_ver;
	u16 third_ver;
	u32 fw_len;
	u8 data[0];
} __attribute__ ((packed));

struct tc370_data {
	struct i2c_client		*client;
	struct input_dev		*input_dev;
	char				phys[32];
	struct tc370_platform_data	*pdata;
	struct early_suspend		early_suspend;
	struct mutex			lock;
	struct fw_image			*fw_img;
	bool				enabled;
	u8				suspend_type;
	u32				scl;
	u32				sda;
	int				udelay;
	int				num_key;
	int				*keycodes;
	u8				*touchscreen_is_pressed;
	/* variables for fw update*/
	const struct firmware		*fw;
	u8				cur_fw_path;
	struct workqueue_struct		*fw_wq;
	struct work_struct		fw_work;
	struct wake_lock		fw_wake_lock;
	u8				fw_flash_state;

	/* variables for LED*/
	struct led_classdev		led;
	struct workqueue_struct		*led_wq;
	struct work_struct		led_work;
	u8				led_brightness;
#if defined(SEC_FAC_TK)
	struct fdata_struct		*fdata;
#endif
};

/*temporary*/
int get_tsp_status(void)
{
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tc370_early_suspend(struct early_suspend *h);
static void tc370_late_resume(struct early_suspend *h);
#endif

static irqreturn_t tc370_interrupt(int irq, void *dev_id)
{
	struct tc370_data *data = dev_id;
	struct i2c_client *client = data->client;
	u8 key_val;
	u8 key_index;
	bool press;
	int ret;

	ret = i2c_smbus_read_byte_data(client, TC370_KEY_DATA);
	if (ret < 0) {
		dev_err(&client->dev, "failed to read key data (%d)\n", ret);
		goto out;
	}
	key_val = (u8)ret;
	key_index = key_val & TC370_KEY_INDEX_MASK;

	switch (key_index) {
	case 0:
		dev_err(&client->dev, "no button press interrupt(%#x)\n",
			key_val);
		break;

	case 1 ... 2:
		press = !(key_val & TC370_KEY_PRESS_MASK);

		if (get_tsp_status() > 0 && press) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			dev_info(&client->dev, "key[%3d] is pressed,"
				 "But blocked (%d).\n",
				 data->keycodes[key_index - 1],
				 (int)*data->touchscreen_is_pressed);
#else
			dev_info(&client->dev, "key is pressed,"
				 "But blocked (%d).\n",
				 (int)*data->touchscreen_is_pressed);
#endif
			goto out;
		}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		dev_info(&client->dev, "key[%3d] is %s\n",
			data->keycodes[key_index - 1],
			(press) ? "pressed" : "releaseed");
#else
		dev_info(&client->dev, "key is %s\n",
			(press) ? "pressed" : "releaseed");
#endif
		input_report_key(data->input_dev,
				data->keycodes[key_index - 1], press);
		input_sync(data->input_dev);

		break;
	default:
		dev_err(&client->dev, "wrong interrupt(%#x)\n", key_val);
		break;
	}

out:
	return IRQ_HANDLED;
}

static inline void setsda(struct tc370_data *data, int state)
{
	if (state)
		gpio_direction_input(data->sda);
	else
		gpio_direction_output(data->sda, 0);
}

static inline void setscl(struct tc370_data *data, int state)
{
	if (state)
		gpio_direction_input(data->scl);
	else
		gpio_direction_output(data->scl, 0);
}

static inline int getsda(struct tc370_data *data)
{
	return gpio_get_value(data->sda);
}

static inline int getscl(struct tc370_data *data)
{
	return gpio_get_value(data->scl);
}

static inline void sdalo(struct tc370_data *data)
{
	setsda(data, 0);
	udelay((data->udelay + 1) / 2);
}

static inline void sdahi(struct tc370_data *data)
{
	setsda(data, 1);
	udelay((data->udelay + 1) / 2);
}

static inline void scllo(struct tc370_data *data)
{
	setscl(data, 0);
	udelay((data->udelay + 1) / 2);
}

static int sclhi(struct tc370_data *data)
{
	int i;
	
	setscl(data, 1);

	for (i = 0 ; i < 20; ++i) {
		if (getscl(data))
			break;
		udelay(10);
	}

	udelay(data->udelay);

	return 0;
}

static void isp_start(struct tc370_data *data)
{
	setsda(data, 0);
	udelay(data->udelay);
	scllo(data);
}

static void isp_stop(struct tc370_data *data)
{
	sdalo(data);
	sclhi(data);
	setsda(data, 1);
	udelay(data->udelay);
}

static int isp_recvdbyte(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	int i;
	u8 indata = 0;
	sdahi(data);
	for (i = 0; i < 8 ; i++) {
		if (sclhi(data) < 0) { /* timed out */
			dev_err(&client->dev, "%s: timeout at bit "
				"#%d\n", __func__, 7 - i);
			return -ETIMEDOUT;
		}

		indata = indata << 1;
		if (getsda(data))
			indata |= 0x01;

		setscl(data, 0);

		udelay(i == 7 ? data->udelay / 2 : data->udelay);
	}
	return indata;
}

static int isp_sendbyte(struct tc370_data *data, u8 c)
{
	struct i2c_client *client = data->client;
	int i;
	int sb;
	int ack = 0;

	/* assert: scl is low */
	for (i = 7; i >= 0; i--) {
		sb = (c >> i) & 0x1;
		setsda(data, sb);
		udelay((data->udelay + 1) / 2);

		if (sclhi(data) < 0) { /* timed out */
			dev_err(&client->dev, "%s: %#x, timeout at bit #%d\n",
				__func__, (int)c, i);
			return -ETIMEDOUT;
		}
		scllo(data);
	}
	sdahi(data);

	if (sclhi(data) < 0) { /* timed out */
		dev_err(&client->dev, "%s: %#x, timeout at bit #%d\n",
			__func__, (int)c, i);
		return -ETIMEDOUT;
	}

	ack = !getsda(data);

	scllo(data);

#if defined(ISP_VERY_VERBOSE_DEBUG)
	dev_info(&client->dev, "%s: %#x %s\n", __func__, (int)c,
		 ack ? "A" : "NA");
#endif
	return ack;
}

static int isp_master_recv(struct tc370_data *data, u8 addr, u8 *val)
{
	struct i2c_client *client = data->client;
	int ret;
	int retries = 2;

retry:
	isp_start(data);

	ret = isp_sendbyte(data, addr);
	if (ret != TC370_ISP_ACK) {
		dev_err(&client->dev, "%s: %#x %s\n", __func__, addr, "NA");
		if (retries-- > 0) {
			dev_err(&client->dev, "%s: retry (%d)\n", __func__,
				retries);
			goto retry;
		}
		return -EIO;
	}
	*val = isp_recvdbyte(data);
	isp_stop(data);

	return 0;
}

static int isp_master_send(struct tc370_data *data, u8 msg_1, u8 msg_2)
{
	struct i2c_client *client = data->client;
	int ret;
	int retries = 2;

retry:
	isp_start(data);
	ret = isp_sendbyte(data, msg_1);
	if (ret != TC370_ISP_ACK) {
		dev_err(&client->dev, "%s: %#x %s\n", __func__, msg_1, "NA");
		if (retries-- > 0) {
			dev_err(&client->dev, "%s: retry (%d)\n", __func__,
				retries);
			goto retry;
		}
		return -EIO;
	}
	ret = isp_sendbyte(data, msg_2);
	if (ret != TC370_ISP_ACK) {
		dev_err(&client->dev, "%s: %#x %s\n", __func__, msg_2, "NA");
		if (retries-- > 0) {
			dev_err(&client->dev, "%s: retry (%d)\n", __func__,
				retries);
			goto retry;
		}
		return -EIO;
	}
	isp_stop(data);

	return 0;
}

static void isp_sp_signal(struct tc370_data *data)
{
	int i;
	unsigned long flags;

	local_irq_save(flags);
	for (i = TC370_NUM_OF_ISP_SP_SIGNAL - 1; i >= 0; i--) {
		int sb = (TC370_ISP_SP_SIGNAL >> i) & 0x1;
		setscl(data, sb);
		udelay(3);
		setsda(data, 0);
		udelay(10);
		setsda(data, 1);
		udelay(10);

		if (i == 5)
			udelay(30);
	}

	sclhi(data);
	local_irq_restore(flags);
}

static int raw_dbgir3(struct tc370_data *data, u8 data2, u8 data1, u8 data0)
{
	struct i2c_client *client = data->client;
	int ret = 0;

	ret = ret | isp_master_send(data, 0xc2, data2);
	if (ret < 0)
		goto err_isp_master_send;
	ret = ret | isp_master_send(data, 0xc4, data1);
	if (ret < 0)
		goto err_isp_master_send;
	ret = ret | isp_master_send(data, 0xc6, data0);
	if (ret < 0)
		goto err_isp_master_send;
	ret = ret | isp_master_send(data, 0xc0, 0x80);
	if (ret < 0)
		goto err_isp_master_send;

	return 0;
err_isp_master_send:
	dev_err(&client->dev, "fail to dbgir3 %#x,%#x,%#x (%d)\n",
		data2, data1, data0, ret);
	return ret;
}

static int raw_dbgir2(struct tc370_data *data, u8 data1, u8 data0)
{
	struct i2c_client *client = data->client;
	int ret = 0;

	ret = ret | isp_master_send(data, 0xc2, data1);
	if (ret < 0)
		goto err_raw_dbir2;
	ret = ret | isp_master_send(data, 0xc4, data0);
	if (ret < 0)
		goto err_raw_dbir2;
	ret = ret | isp_master_send(data, 0xc0, 0x80);
	if (ret < 0)
		goto err_raw_dbir2;

	return 0;
err_raw_dbir2:
	dev_err(&client->dev, "fail to dbgir2 %#x,%#x (%d)\n",
		data1, data0, ret);
	return ret;
}

static int raw_spchl(struct tc370_data *data, u8 data1, u8 data0)
{
	struct i2c_client *client = data->client;
	int ret = 0;

	ret = ret | isp_master_send(data, 0xd0, data1);
	ret = ret | isp_master_send(data, 0xd2, data0);

	if (ret < 0) {
		dev_err(&client->dev, "fail to spchl %#x,%#x (%d)\n",
			data1, data0, ret);
		return ret;
	}

	return 0;
}

static int isp_common_set(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	int ret = 0;

	ret = ret | raw_dbgir3(data, 0x75 , 0x8f, 0x25);
	if (ret < 0)
		goto err_write_common_set;
	ret = ret | raw_dbgir3(data, 0x75 , 0xc6, 0x0e);
	if (ret < 0)
		goto err_write_common_set;
	ret = ret | raw_dbgir3(data, 0x75 , 0xf7, 0xc1);
	if (ret < 0)
		goto err_write_common_set;
	ret = ret | raw_dbgir3(data, 0x75 , 0xf7, 0x1e);
	if (ret < 0)
		goto err_write_common_set;
	ret = ret | raw_dbgir3(data, 0x75 , 0xf7, 0xec);
	if (ret < 0)
		goto err_write_common_set;
	ret = ret | raw_dbgir3(data, 0x75 , 0xf7, 0x81);
	if (ret < 0)
		goto err_write_common_set;

	return 0;

err_write_common_set:
	dev_err(&client->dev, "fail to %s (%d)\n", __func__, ret);
	return ret;
	
}

static int isp_ers_timing_set(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	int ret = 0;

	ret = ret | raw_dbgir3(data, 0x75, 0xf2, 0x20);
	if (ret < 0)
		goto err_ers_timing_set;
	ret = ret | raw_dbgir3(data, 0x75, 0xf3, 0xa1);
	if (ret < 0)
		goto err_ers_timing_set;
	ret = ret | raw_dbgir3(data, 0x75, 0xf4, 0x07);
	if (ret < 0)
		goto err_ers_timing_set;
	ret = ret | raw_dbgir3(data, 0x75, 0xf1, 0x80);
	if (ret < 0)
		goto err_ers_timing_set;

	return 0;
err_ers_timing_set:
	dev_err(&client->dev, "fail to %s (%d)\n", __func__, ret);
	return ret;
}

static int isp_pgm_timing_set(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	int ret = 0;

	ret = ret | raw_dbgir3(data, 0x75, 0xf2, 0xe8);
	if (ret < 0)
		goto err_pgm_timing_set;
	ret = ret | raw_dbgir3(data, 0x75, 0xf3, 0x03);
	if (ret < 0)
		goto err_pgm_timing_set;
	ret = ret | raw_dbgir3(data, 0x75, 0xf4, 0x00); 
	if (ret < 0)
		goto err_pgm_timing_set;

	return 0;
err_pgm_timing_set:
	dev_err(&client->dev, "fail to %s (%d)\n", __func__, ret);
	return ret;
}

static void reset_for_isp(struct tc370_data *data)
{
	data->pdata->power(false);

	gpio_direction_output(data->pdata->gpio_scl, 0);
	gpio_direction_output(data->pdata->gpio_sda, 0);
	gpio_direction_output(data->pdata->gpio_int, 0);

	msleep(TC370_POWERON_DELAY);

	gpio_direction_output(data->pdata->gpio_scl, 1);
	gpio_direction_output(data->pdata->gpio_sda, 1);
	gpio_direction_input(data->pdata->gpio_int);

	data->pdata->power(true);
	usleep_range(5000, 6000);
	dev_info(&data->client->dev, "%s: end\n", __func__);
}

static int tc370_erase_fw(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	int ret;
	u16 addr = 0;
	int dcr_rd_cnt;
	u8 val;

	reset_for_isp(data);

	isp_sp_signal(data);
	ret = isp_common_set(data);
	if (ret < 0) {
		dev_err(&client->dev, "fail to %s (%d)\n", __func__, ret);
		return ret;
	}

	ret = isp_ers_timing_set(data);

	while (addr < TC370_FW_ER_MAX_LEN) {
#if defined(ISP_DEBUG)
		dev_info(&client->dev, "fw erase addr=x0%4x\n", addr);
#endif
		raw_dbgir3(data, 0x90, (u8)(addr >> 8), (u8)(addr & 0xff));
		isp_master_send(data, 0xf8, 0x01);
		isp_master_send(data, 0xc8, 0xff);
		isp_master_send(data, 0xca, 0x42);
		raw_dbgir3(data, 0x02, 0xff, 0x3a);
		raw_spchl(data, 0xff, 0x3a);
		isp_master_send(data, 0xc0, 0x14);


		val = 0;
		dcr_rd_cnt = TC370_DCR_RD_RETRY;
		do {
			isp_master_recv(data, 0xc1, &val);
			if (dcr_rd_cnt-- < 0) {
				dev_err(&client->dev, "%s: fail to update "
					"dcr\n", __func__);
				return -ENOSYS;
			}
			usleep_range(10000, 15000);
		} while (val != 0x12);
#if defined(ISP_VERBOSE_DEBUG)
			dev_info(&client->dev, "dcr_rd_cnt=%d\n", dcr_rd_cnt);
#endif
		addr += 0x400;
	}

	return 0;
}

static int tc370_write_fw(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	u16 addr = 0;
	int dcr_rd_cnt;
	u8 val;

	reset_for_isp(data);

	isp_sp_signal(data);
	isp_common_set(data);

	dev_info(&client->dev, "%#x %#x\n", addr, data->fw_img->fw_len);

	while (addr < data->fw_img->fw_len) {
#if defined(ISP_DEBUG)
		dev_info(&client->dev, "fw write addr=%#x\n", addr);
#endif
		isp_pgm_timing_set(data);

		raw_dbgir3(data, 0x90, (u8)(addr >> 8), (u8)(addr & 0xff));
		isp_master_send(data, 0xf8, 0x01);
		isp_master_send(data, 0xc8, 0xff);
		isp_master_send(data, 0xca, 0x20);
		raw_dbgir3(data, 0x02, 0xff, 0x1e);
		raw_spchl(data, 0xff, 0x1e);

		do {
			u8 __fw_data = data->fw_img->data[addr++];
			raw_dbgir2(data, 0x74, __fw_data);
			raw_dbgir3(data, 0x75, 0xf1, 0x80);
			isp_master_send(data, 0xc0, 0x14);

#if !defined(ISP_SPEED_UP)
			val = 0;
			dcr_rd_cnt = TC370_DCR_RD_RETRY;
			do {
				isp_master_recv(data, 0xc1, &val);
				if (dcr_rd_cnt-- < 0) {
					dev_err(&client->dev, "%s: fail to "
						"update dcr\n", __func__);
					return -ENOSYS;
				}
				usleep_range(900, 1000);
			} while (val != 0x12);
#endif
#if defined(ISP_VERBOSE_DEBUG)
			dev_info(&client->dev, "dcr_rd_cnt=%d\n", dcr_rd_cnt);
#endif

			/* increase address */
			isp_master_send(data, 0xc0, 0x08);

		} while (addr % 0x20);
	}

	return 0;
}

static int tc370_verify_fw(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	u16 addr = 0;
	int dcr_rd_cnt;
	u8 val;

	reset_for_isp(data);

	isp_sp_signal(data);
	isp_common_set(data);

	while (addr < data->fw_img->fw_len) {

#if defined(ISP_DEBUG)
		dev_info(&client->dev, "fw read addr=%#x\n", addr);
#endif

		isp_master_send(data, 0xf8, 0x01);
		isp_master_send(data, 0xc8, 0xff);
		isp_master_send(data, 0xca, 0x2a);
		raw_dbgir3(data, 0x90, (u8)(addr >> 8), (u8)(addr & 0xff));
		raw_dbgir3(data, 0x02, 0xff, 0x28);
		raw_spchl(data, 0xff, 0x28);

		do {
			isp_master_send(data, 0xc0, 0x14);

#if !defined(ISP_SPEED_UP)
			val = 0;
			dcr_rd_cnt = TC370_DCR_RD_RETRY;
			do {
				isp_master_recv(data, 0xc1, &val);
				if (dcr_rd_cnt-- < 0) {
					dev_err(&client->dev, "%s: fail to "
						"update dcr\n", __func__);
					return -ENOSYS;
				}
				usleep_range(900, 1000);
			} while (val != 0x12);
#endif
#if defined(ISP_VERBOSE_DEBUG)
			dev_info(&client->dev, "dcr_rd_cnt=%d\n", dcr_rd_cnt);
#endif
			isp_master_send(data, 0xc0, 0x08);
			isp_master_recv(data, 0xd9, &val);

			if (data->fw_img->data[addr] != val) {
				dev_err(&client->dev, "fail to verify at "
					"%#x (%#x)\n", addr,
					data->fw_img->data[addr]);
				return -EIO;
			}
			addr++;
		} while (addr % 0x20);
	}

	return 0;
}

static int tc370_get_fw_ver(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	int ver;
	int retries = 2;

read_version:
	ver = i2c_smbus_read_byte_data(client, TC370_CMD_FW_VER);
	if (ver < 0) {
		dev_err(&client->dev, "failed to read fw ver (%d)\n", ver);
		if (retries-- > 0) {
			data->pdata->power(false);
			msleep(TC370_POWERON_DELAY);
			data->pdata->power(true);
			msleep(TC370_POWERON_DELAY);
			goto read_version;
		}
	}

	return ver;
}

static void tc370_fw_update_worker(struct work_struct *work)
{
	struct tc370_data *data = container_of(work, struct tc370_data,
					       fw_work);
	struct i2c_client *client = data->client;
	int retries;
	int ret;

	wake_lock(&data->fw_wake_lock);

	data->pdata->pin_configure(true);

	retries = TC370_FW_FLASH_RETRY;
erase_fw:
	ret = tc370_erase_fw(data);
	if (ret < 0) {
		dev_err(&client->dev, "fail to erase fw (%d)\n", ret);
		if (retries-- > 0) {
			dev_info(&client->dev, "retry esasing fw (%d)\n",
				 retries);
			goto erase_fw;
		} else {
			goto err;
		}
	}
	dev_info(&client->dev, "succeed in erasing fw\n");

	retries = TC370_FW_FLASH_RETRY;
write_fw:
	ret = tc370_write_fw(data);
	if (ret < 0) {
		dev_err(&client->dev, "fail to write fw (%d)\n", ret);
		if (retries-- > 0) {
			dev_info(&client->dev, "retry writing fw (%d)\n",
				 retries);
			goto write_fw;
		} else {
			goto err;
		}
	}
	dev_info(&client->dev, "succeed in writing fw\n");

	retries = TC370_FW_FLASH_RETRY;
verify_fw:
	ret = tc370_verify_fw(data);
	if (ret < 0) {
		dev_err(&client->dev, "fail to verify fw (%d)\n", ret);
		if (retries-- > 0) {
			dev_info(&client->dev, "retry verifing fw (%d)\n",
				 retries);
			goto verify_fw;
		} else {
			goto err;
		}
	}
	dev_info(&client->dev, "succeed in verifing fw\n");

	data->pdata->power(false);
	msleep(TC370_POWERON_DELAY);

	data->pdata->int_set_pull(true);

	data->pdata->power(true);
	msleep(TC370_POWERON_DELAY);

	retries = TC370_FW_VER_READ;
read_flashed_fw_ver:
	ret = tc370_get_fw_ver(data);
	if (ret < 0) {
		dev_err(&client->dev, "fail to read fw ver (%d)\n", ret);
		if (retries-- > 0) {
			dev_info(&client->dev, "retry read flash fw ver (%d)\n",
				 retries);
			goto read_flashed_fw_ver;
		} else {
			goto err;
		}
	}

	dev_info(&client->dev, "succeed in reading fw ver %#x\n", (u8)ret);

	ret = 0;

	data->pdata->pin_configure(false);

	if (data->cur_fw_path == FW_BUILT_IN)
		release_firmware(data->fw);
	else if (data->cur_fw_path == FW_IN_SDCARD ||
		 data->cur_fw_path == FW_EX_SDCARD)
		kfree(data->fw_img);

	data->fw_flash_state = STATE_FLASH;
	enable_irq(client->irq);
	data->enabled = true;

	wake_unlock(&data->fw_wake_lock);

#if defined(SEC_FAC_TK)
	if (data->fdata->fw_flash_status == DOWNLOADING)
		data->fdata->fw_flash_status = PASS;
#endif

	dev_info(&client->dev, "succeed in flashing fw\n");

	return;

err:
	data->pdata->pin_configure(false);

	if (data->cur_fw_path == FW_BUILT_IN)
		release_firmware(data->fw);
	else if (data->cur_fw_path == FW_IN_SDCARD ||
		 data->cur_fw_path == FW_EX_SDCARD)
		kfree(data->fw_img);

	wake_unlock(&data->fw_wake_lock);

#if defined(SEC_FAC_TK)
	if (data->fdata->fw_flash_status == DOWNLOADING) {
		dev_err(&client->dev, "fail to fw flash.\n");
		data->fdata->fw_flash_status = FAIL;
		return;
	}
#endif

/* early suspend is not removed for debugging. */
/*
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif
*/
	data->fw_flash_state = STATE_FLASH_FAIL;
	wake_lock_destroy(&data->fw_wake_lock);
	free_irq(client->irq, data);
	gpio_free(data->pdata->gpio_int);
	led_classdev_unregister(&data->led);
	destroy_workqueue(data->led_wq);
	data->led_wq = NULL;
/* fw_wq (workqueue for firmware flash) will be destroyed in suspend function */
/*	destroy_workqueue(data->fw_wq); */
	input_unregister_device(data->input_dev);
	/*input_free_device(data->input_dev);*/
	data->pdata->setup_power(NULL, false);
/* data is not deallocated for debugging. */
/*	kfree(data); */
	dev_err(&client->dev, "fail to fw flash. driver is removed\n");
}

static int load_fw_built_in(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	int ret;
	char *fw_name = kasprintf(GFP_KERNEL, "%s/%s.fw",
				  TC370_FW_BUILTIN_PATH, TC370_FW_NAME);

	ret = request_firmware(&data->fw, fw_name, &client->dev);
	if (ret) {
		dev_err(&client->dev, "error requesting built-in firmware (%d)"
			"\n", ret);
		goto out;
	}

	data->fw_img = (struct fw_image *)data->fw->data;

	dev_info(&client->dev, "the fw 0x%x is loaded (size=%d)\n",
		 data->fw_img->first_fw_ver, data->fw_img->fw_len);

out:
	kfree(fw_name);
	return ret;
}

static int load_fw_header(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	int ret = 0;

	/*
	 * to do : implemete
	 */
	dev_info(&client->dev, "%s:", __func__);
	return ret;
}

static int load_fw_in_sdcard(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	int ret;
	mm_segment_t old_fs;
	struct file *fp;
	long nread;
	int len;

	char *fw_name = kasprintf(GFP_KERNEL, "%s/%s.in.fw",
				  TC370_FW_IN_SDCARD_PATH, TC370_FW_NAME);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(fw_name, O_RDONLY, S_IRUSR);
	if (!fp) {
		dev_err(&client->dev, "%s: fail to open fw in %s\n",
			__func__, fw_name);
		ret = -ENOENT;
		goto err_open_fw;
	}
	len = fp->f_path.dentry->d_inode->i_size;

	data->fw_img = kzalloc(len, GFP_KERNEL);
	if (!data->fw_img) {
		dev_err(&client->dev, "%s: fail to alloc mem for fw\n",
			__func__);
		ret = -ENOMEM;
		goto err_alloc;
	}
	nread = vfs_read(fp, (char __user *)data->fw_img, len, &fp->f_pos);

	dev_info(&client->dev, "%s: load fw in internal sd (%ld)\n",
		 __func__, nread);

	ret = 0;

err_alloc:
	filp_close(fp, NULL);
err_open_fw:
	set_fs(old_fs);
	kfree(fw_name);
	return ret;
}

static int load_fw_ex_sdcard(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	int ret = 0;
	/*
	 * to do : implemete
	 */
	dev_info(&client->dev, "%s:", __func__);
	return ret;
}

static int tc370_load_fw(struct tc370_data *data, u8 fw_path)
{
	struct i2c_client *client = data->client;
	int ret;

	switch (fw_path) {
	case FW_BUILT_IN:
		ret = load_fw_built_in(data);
		break;

	case FW_HEADER:
		ret = load_fw_header(data);
		break;

	case FW_IN_SDCARD:
		ret = load_fw_in_sdcard(data);
		break;

	case FW_EX_SDCARD:
		ret = load_fw_ex_sdcard(data);
		break;

	default:
		dev_err(&client->dev, "%s: invalid fw path (%d)\n",
			__func__, fw_path);
		return -ENOENT;
	}
	if (ret < 0) {
		dev_err(&client->dev, "fail to load fw in %d (%d)\n",
			fw_path, ret);
		return ret;
	}
	return 0;
}

static int tc370_unload_fw(struct tc370_data *data, u8 fw_path)
{
	struct i2c_client *client = data->client;

	switch (fw_path) {
	case FW_BUILT_IN:
		release_firmware(data->fw);
		break;

	case FW_HEADER:
		break;

	case FW_IN_SDCARD:
	case FW_EX_SDCARD:
		kfree(data->fw_img);
		break;

	default:
		dev_err(&client->dev, "%s: invalid fw path (%d)\n",
			__func__, fw_path);
		return -ENOENT;
	}

	return 0;
}

static int tc370_flash_fw(struct tc370_data *data, u8 fw_path, bool force)
{
	struct i2c_client *client = data->client;
	int ret;
	int fw_ver;

	ret = tc370_load_fw(data, fw_path);
	if (ret < 0) {
		dev_err(&client->dev, "fail to load fw (%d)\n", ret);
		return ret;
	}
	data->cur_fw_path = fw_path;

	/* firmware version compare */
	fw_ver = tc370_get_fw_ver(data);
	if (fw_ver >= data->fw_img->first_fw_ver && !force) {
		ret = HAVE_LATEST_FW;
		dev_info(&client->dev, "IC aleady have latest firmware (%#x)\n",
			 fw_ver);
		goto out;
	}
	dev_info(&client->dev, "fw update to %#x (from %#x) (%s)\n",
		 data->fw_img->first_fw_ver, fw_ver,
		 (force) ? "force" : "ver mismatch");

	queue_work(data->fw_wq, &data->fw_work);

	return FW_UPDATE_RUNNING;

out:
	data->fw_flash_state = STATE_NORMAL;
	tc370_unload_fw(data, fw_path);
	return ret;
}

static int tc370_initialize(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	int ret;

	ret = tc370_flash_fw(data, FW_BUILT_IN, false);
	if (ret < 0)
		dev_err(&client->dev, "fail to flash fw (%d)\n", ret);

	return ret;
}

static void tc370_led_worker(struct work_struct *work)
{
	struct tc370_data *data = container_of(work, struct tc370_data,
					       led_work);
	struct i2c_client *client = data->client;
	u8 buf;
	int ret;
	u8 br;
	int cnt = 20;

	br = data->led_brightness;

#if defined(LED_DEBUG)
	dev_info(&client->dev, "%s: turn %s LED\n", __func__,
		(br == LED_OFF) ? "off" : "on");
#endif

	if (br == LED_OFF)
		buf = TC370_CMD_LED_OFF;
	else /* LED_FULL*/
		buf = TC370_CMD_LED_ON;

	if (br == LED_OFF && !data->enabled) {
		dev_info(&client->dev, "%s: ignore LED control "
			 "(IC is disabled)\n", __func__);
		return;
	}

	while (!data->enabled) {
		msleep(TC370_POWERON_DELAY);
#if defined(LED_DEBUG)
		dev_err(&client->dev, "%s: waiting for device (%d)\n",
			__func__, cnt);
#endif
		if (--cnt <  0) {
			dev_err(&client->dev, "%s: fail to tc360 led %s\n",
				__func__,
				(br == LED_OFF) ? "OFF" : "ON");
			return;
		}
	}

	ret = i2c_smbus_write_byte_data(client, TC370_CMD, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: failed to wt led data (%d)\n",
			__func__, ret);
}

static void tc370_led_set(struct led_classdev *led_cdev,
			  enum led_brightness value)
{
	struct tc370_data *data =
			container_of(led_cdev, struct tc370_data, led);
	struct i2c_client *client = data->client;

	if (unlikely(wake_lock_active(&data->fw_wake_lock))) {
		dev_info(&client->dev, "fw is being updated."
			 "led control is ignored.\n");
		return;
	}

	if (data->led_brightness == value) {
		dev_info(&client->dev, "%s: ignore LED control "
			 "(set same brightness)\n", __func__);
		return;
	}

	data->led_brightness = value;
	if (data->led_wq)
		queue_work(data->led_wq, &data->led_work);
}

#if defined(SEC_FAC_TK)
static ssize_t fac_fw_ver_ic_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct tc370_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ver;
	int ret;

	if (!data->enabled) {
		dev_err(&client->dev, "%s: device is disabled\n.", __func__);
		ret = -EPERM;
		goto out;
	}

	ver = tc370_get_fw_ver(data);
	if (ver < 0) {
		dev_err(&client->dev, "%s: fail to read fw ver (%d)\n.",
			__func__, ver);
		ret = sprintf(buf, "%s\n", "error");
		goto out;
	}

	dev_info(&client->dev, "%s: %#x\n", __func__, (u8)ver);
	ret = sprintf(buf, "%#x\n", (u8)ver);
out:
	return ret;
}

static ssize_t fac_fw_ver_src_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct tc370_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u16 ver;

	if (!data->enabled) {
		dev_err(&client->dev, "%s: device is disabled\n.", __func__);
		ver = 0;
		ret = -EPERM;
		goto out;
	}

	ret = tc370_load_fw(data, FW_BUILT_IN);
	if (ret < 0) {
		dev_err(&client->dev, "%s: fail to load fw (%d)\n.", __func__,
			ret);
		ver = 0;
		goto out;
	}

	ver = data->fw_img->first_fw_ver;

	ret = tc370_unload_fw(data, FW_BUILT_IN);
	if (ret < 0) {
		dev_err(&client->dev, "%s: fail to unload fw (%d)\n.",
			__func__, ret);
		ver = 0;
		goto out;
	}

out:
	ret = sprintf(buf, "%#x\n", ver);
	dev_info(&client->dev, "%s: %#x\n", __func__, ver);
	return ret;
}

static ssize_t fac_fw_update_store(struct device *dev,
				   struct device_attribute *devattr,
				   const char *buf, size_t count)
{
	struct tc370_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 fw_path;

	if (!data->enabled) {
		dev_err(&client->dev, "%s: device is disabled\n.", __func__);
		return -EPERM;
	}

	switch (*buf) {
	case 's':
	case 'S':
		fw_path = FW_BUILT_IN;
		break;

	case 'i':
	case 'I':
		fw_path = FW_IN_SDCARD;
		break;

	default:
		dev_err(&client->dev, "%s: invalid parameter %c\n.", __func__,
			*buf);
		return -EINVAL;
	}

	data->fdata->fw_flash_status = DOWNLOADING;
	disable_irq(client->irq);
	data->enabled = false;

	ret = tc370_flash_fw(data, fw_path, true);
	if (ret < 0) {
		data->fdata->fw_flash_status = FAIL;
		dev_err(&client->dev, "%s: fail to flash fw (%d)\n.", __func__,
			ret);
		return ret;
	}

	return count;
}

static ssize_t fac_fw_update_status_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct tc370_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;

	switch (data->fdata->fw_flash_status) {
	case DOWNLOADING:
		ret = sprintf(buf, "%s\n", TO_STRING(DOWNLOADING));
		break;
	case FAIL:
		ret = sprintf(buf, "%s\n", TO_STRING(FAIL));
		break;
	case PASS:
		ret = sprintf(buf, "%s\n", TO_STRING(PASS));
		break;
	default:
		dev_err(&client->dev, "%s: invalid status\n", __func__);
		ret = 0;
		goto out;
	}

	dev_info(&client->dev, "%s: %#x\n", __func__,
		 data->fdata->fw_flash_status);

out:
	return ret;
}

static ssize_t fac_read_chpct_menu_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct tc370_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u16 val;
	u8 buff[TC370_RAW_DATA_SIZE] = {0, };

	if (!data->enabled) {
		dev_err(&client->dev, "%s: device is disabled\n.", __func__);
		return -EPERM;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC370_CMD_RAW_MENU,
					    TC370_RAW_DATA_SIZE, buff);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read raw data (%d)\n",
			__func__, ret);
		return ret;
	}

	val = ntohs(*(unsigned short *)(buff + TC370_RAW_CHPCT_OFFSET));

	print_hex_dump(KERN_INFO, "tc360: ", DUMP_PREFIX_OFFSET, 16, 1,
		       buff, TC370_RAW_DATA_SIZE, false);

	ret = sprintf(buf, "%d\n", val);

	dev_info(&client->dev, "%s: %d\n", __func__, val);

	return ret;
}

static ssize_t fac_read_chpct_back_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct tc370_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u16 val;
	u8 buff[TC370_RAW_DATA_SIZE] = {0, };

	if (!data->enabled) {
		dev_err(&client->dev, "%s: device is disabled\n.", __func__);
		return -EPERM;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC370_CMD_RAW_BACK,
					    TC370_RAW_DATA_SIZE, buff);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read raw data (%d)\n",
			__func__, ret);
		return ret;
	}

	val = ntohs(*(unsigned short *)(buff + TC370_RAW_CHPCT_OFFSET));

	print_hex_dump(KERN_INFO, "tc360: ", DUMP_PREFIX_OFFSET, 16, 1,
		       buff, TC370_RAW_DATA_SIZE, false);

	ret = sprintf(buf, "%d\n", val);

	dev_info(&client->dev, "%s: %d\n", __func__, val);

	return ret;
}

static ssize_t fac_read_threshold_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct tc370_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u16 thr_menu, thr_back;
	u8 buff[TC370_RAW_DATA_SIZE] = {0, };

	if (!data->enabled) {
		dev_err(&client->dev, "%s: device is disabled\n.", __func__);
		return -EPERM;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC370_CMD_RAW_MENU,
					    TC370_RAW_DATA_SIZE, buff);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read raw data (%d)\n",
			__func__, ret);
		return ret;
	}
	print_hex_dump(KERN_INFO, "tc360: ", DUMP_PREFIX_OFFSET, 16, 1,
		       buff, TC370_RAW_DATA_SIZE, false);

	thr_menu = ntohs(*(unsigned short *)buff);

	ret = i2c_smbus_read_i2c_block_data(client, TC370_CMD_RAW_BACK,
					    TC370_RAW_DATA_SIZE, buff);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read raw data (%d)\n",
			__func__, ret);
		return ret;
	}
	print_hex_dump(KERN_INFO, "tc360: ", DUMP_PREFIX_OFFSET, 16, 1,
		       buff, TC370_RAW_DATA_SIZE, false);

	thr_back = ntohs(*(unsigned short *)buff);

	ret = sprintf(buf, "%d, %d\n", thr_menu, thr_back);

	dev_info(&client->dev, "%s: %d, %d\n", __func__, thr_menu, thr_back);

	return ret;
}

static ssize_t fac_read_raw_menu_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct tc370_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u16 val;
	u8 buff[TC370_RAW_DATA_SIZE] = {0, };

	if (!data->enabled) {
		dev_err(&client->dev, "%s: device is disabled\n.", __func__);
		return -EPERM;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC370_CMD_RAW_MENU,
					    TC370_RAW_DATA_SIZE, buff);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read raw data (%d)\n",
			__func__, ret);
		return ret;
	}

	val = ntohs(*(unsigned short *)(buff + TC370_RAW_RAWDATA_OFFSET));

	print_hex_dump(KERN_INFO, "tc360: ", DUMP_PREFIX_OFFSET, 16, 1,
		       buff, TC370_RAW_DATA_SIZE, false);

	ret = sprintf(buf, "%d\n", val);

	dev_info(&client->dev, "%s: %d\n", __func__, val);

	return ret;

}

static ssize_t fac_read_raw_back_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct tc370_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u16 val;
	u8 buff[TC370_RAW_DATA_SIZE] = {0, };

	if (!data->enabled) {
		dev_err(&client->dev, "%s: device is disabled\n.", __func__);
		return -EPERM;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC370_CMD_RAW_BACK,
					    TC370_RAW_DATA_SIZE, buff);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read raw data (%d)\n",
			__func__, ret);
		return ret;
	}

	val = ntohs(*(unsigned short *)(buff + TC370_RAW_RAWDATA_OFFSET));

	print_hex_dump(KERN_INFO, "tc360: ", DUMP_PREFIX_OFFSET, 16, 1,
		       buff, TC370_RAW_DATA_SIZE, false);

	ret = sprintf(buf, "%d\n", val);

	dev_info(&client->dev, "%s: %d\n", __func__, val);

	return ret;

}

static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO | S_IWUSR | S_IWGRP,
		   fac_fw_ver_ic_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO | S_IWUSR | S_IWGRP,
		   fac_fw_ver_src_show, NULL);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO | S_IWUSR | S_IWGRP,
		   NULL, fac_fw_update_store);
static DEVICE_ATTR(touchkey_firm_update_status, S_IRUGO,
		   fac_fw_update_status_show, NULL);
static DEVICE_ATTR(touchkey_menu, S_IRUGO,
		   fac_read_chpct_menu_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO,
		   fac_read_chpct_back_show, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO,
		   fac_read_threshold_show, NULL);
static DEVICE_ATTR(touchkey_raw_data0, S_IRUGO, fac_read_raw_menu_show, NULL);
static DEVICE_ATTR(touchkey_raw_data1, S_IRUGO, fac_read_raw_back_show, NULL);

static struct attribute *fac_attributes[] = {
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_touchkey_menu.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_touchkey_raw_data0.attr,
	&dev_attr_touchkey_raw_data1.attr,
	NULL,
};

static struct attribute_group fac_attr_group = {
	.attrs = fac_attributes,
};
#endif

static int tc370_init_interface(struct tc370_data *data)
{
	struct i2c_client *client = data->client;
	int ret;

#if defined(SEC_FAC_TK)
	data->fdata->dummy_dev = device_create(sec_class, NULL, (dev_t)NULL,
					       data, "sec_touchkey");
	if (IS_ERR(data->fdata->dummy_dev)) {
		dev_err(&client->dev, "Failed to create fac tsp temp dev\n");
		ret = -ENODEV;
		data->fdata->dummy_dev = NULL;
		goto err_create_sec_class_dev;
	}

	ret = sysfs_create_group(&data->fdata->dummy_dev->kobj,
				 &fac_attr_group);
	if (ret) {
		dev_err(&client->dev, "%s: failed to create fac_attr_group "
			"(%d)\n", __func__, ret);
		ret = (ret > 0) ? -ret : ret;
		goto err_create_fac_attr_group;
	}
#endif

	return 0;

#if defined(SEC_FAC_TK)
	sysfs_remove_group(&data->fdata->dummy_dev->kobj,
			   &fac_attr_group);
err_create_fac_attr_group:
	device_destroy(sec_class, (dev_t)NULL);
err_create_sec_class_dev:
#endif
	return ret;
}

static void tc370_destroy_interface(struct tc370_data *data)
{
#if defined(SEC_FAC_TK)
	sysfs_remove_group(&data->fdata->dummy_dev->kobj,
		&fac_attr_group);
	/*device_destroy(sec_class, (dev_t)NULL);*/
#endif
}

static int __devinit tc370_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct tc370_data *data;
	struct input_dev *input_dev;
	int ret;
	int i;

	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "no platform data\n");
		return -EINVAL;
	} else {
		struct tc370_platform_data *pdata =
			client->dev.platform_data;
		if (!pdata->enable) {
			dev_info(&client->dev, "terminate driver\n");
			return -EINVAL;
		}
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	data = kzalloc(sizeof(struct tc370_data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev, "failed to alloc memory\n");
		ret = -ENOMEM;
		goto err_data_alloc;
	}

	data->pdata = client->dev.platform_data;
	if (data->pdata->exit_flag) {
		dev_err(&client->dev, "exit flag is setted(%d)\n",
			data->pdata->exit_flag);
		ret = -ENODEV;
		goto exit_flag_set;
	}

#if defined(SEC_FAC_TK)
	data->fdata = kzalloc(sizeof(struct fdata_struct), GFP_KERNEL);
	if (!data->fdata) {
		dev_err(&client->dev, "failed to alloc memory for fdata\n");
		ret = -ENOMEM;
		goto err_data_alloc_fdata;
	}
#endif
	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "failed to allocate input device\n");
		ret = -ENOMEM;
		goto err_input_devalloc;
	}

	data->client = client;

	data->input_dev = input_dev;
	i2c_set_clientdata(client, data);

	data->num_key = data->pdata->num_key;
	dev_info(&client->dev, "number of keys= %d\n", data->num_key);

	data->keycodes = data->pdata->keycodes;
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	for (i = 0; i < data->num_key; i++)
		dev_info(&client->dev, "keycode[%d]= %3d\n", i,
			data->keycodes[i]);
#endif

	data->pdata = client->dev.platform_data;
	data->suspend_type = data->pdata->suspend_type;
	/*data->touchscreen_is_pressed = &touch_is_pressed;*/
	data->scl = data->pdata->gpio_scl;
	data->sda = data->pdata->gpio_sda;
	data->udelay = data->pdata->udelay;

	mutex_init(&data->lock);
	wake_lock_init(&data->fw_wake_lock, WAKE_LOCK_SUSPEND,
		       "tc370_fw_wake_lock");
	client->irq = client->irq;

	snprintf(data->phys, sizeof(data->phys), "%s/input0",
		dev_name(&client->dev));
	input_dev->name = "sec_touchkey";
	input_dev->phys = data->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->keycode = data->keycodes;
	input_dev->keycodesize = sizeof(data->keycodes[0]);
	input_dev->keycodemax = data->num_key;

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);
	for (i = 0; i < data->num_key; i++) {
		input_set_capability(input_dev, EV_KEY, data->keycodes[i]);
		set_bit(data->keycodes[i], input_dev->keybit);
	}

	i2c_set_clientdata(client, data);
	input_set_drvdata(input_dev, data);
	ret = input_register_device(data->input_dev);
	if (ret) {
		dev_err(&client->dev, "fail to register input_dev (%d).\n",
			ret);
		goto err_register_input_dev;
	}

	ret = tc370_init_interface(data);
	if (ret < 0) {
		dev_err(&client->dev, "failed to init interface (%d)\n", ret);
		goto err_init_interface;
	}

#if 0
	ret = data->pdata->setup_power(&client->dev, true);
	if (ret < 0) {
		dev_err(&client->dev, "fail to setup power (%d).\n", ret);
		goto err_setup_power;
	}
#endif

	data->pdata->power(true);
	msleep(TC370_POWERON_DELAY * 2);
	data->pdata->led_power(true);

	/*check status, for debugging*/
#if 0
	ret = tc370_get_fw_ver(data);
	if (ret < 0) {
		dev_info(&client->dev, "assume that touchkey is not connected\n");
		goto err_read_fw_ver;
	}
#endif

	data->led_wq = create_singlethread_workqueue(client->name);
	if (!data->led_wq) {
		dev_err(&client->dev, "fail to create workqueue for led_wq\n");
		ret = -ENOMEM;
		goto err_create_led_wq;
	}
	INIT_WORK(&data->led_work, tc370_led_worker);

	data->led.name = "button-backlight";
	data->led.brightness = LED_OFF;
	data->led.max_brightness = LED_FULL;
	data->led.brightness_set = tc370_led_set;

	ret = led_classdev_register(&client->dev, &data->led);
	if (ret) {
		dev_err(&client->dev, "fail to register led_classdev (%d).\n",
			ret);
		goto err_register_led;
	}

	data->fw_wq = create_singlethread_workqueue(client->name);
	if (!data->fw_wq) {
		dev_err(&client->dev, "fail to create workqueue for fw_wq\n");
		ret = -ENOMEM;
		goto err_create_fw_wq;
	}
	INIT_WORK(&data->fw_work, tc370_fw_update_worker);

	if (client->irq) {
		ret = request_threaded_irq(client->irq, NULL, tc370_interrupt,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					TC370_DEVICE, data);
		if (ret) {
			dev_err(&client->dev, "fail to request irq (%d).\n",
				client->irq);
			goto err_request_irq;
		}
	}
	disable_irq(client->irq);
	/*msleep(TC370_POWERON_DELAY);*/

	/*
	 * The tc370_initialize function create singlethread_workqueue. If the
	 * probe function is failed while the thread is running, the driver
	 * cause kernel panic. So, the tc370_initialize function must be on
	 * the bottom on probe function.
	 */
	ret = tc370_initialize(data);
	if (ret < 0) {
		dev_err(&client->dev, "fail to tc360 initialize (%d).\n",
			ret);
		goto err_initialize;
	}

	switch (ret) {
	case HAVE_LATEST_FW:
		data->enabled = true;
		enable_irq(client->irq);
		break;
	case FW_UPDATE_RUNNING:
		/*
		 * now fw update thread is running.
		 * this thread will enable interrupt
		 */
		data->enabled = false;
		break;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	if (data->fw_flash_state != STATE_FLASH_FAIL) {
		data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
		data->early_suspend.suspend = tc370_early_suspend;
		data->early_suspend.resume = tc370_late_resume;
		register_early_suspend(&data->early_suspend);
	}	
#endif

	dev_info(&client->dev, "successfully probed.\n");

	return 0;

err_initialize:
	data->pdata->setup_power(NULL, false);
err_setup_power:
	free_irq(client->irq, data);
err_request_irq:
	destroy_workqueue(data->fw_wq);
err_create_fw_wq:
	led_classdev_unregister(&data->led);
err_register_led:
	destroy_workqueue(data->led_wq);
err_create_led_wq:
err_read_fw_ver:
	tc370_destroy_interface(data);
err_init_interface:
	input_unregister_device(input_dev);
err_register_input_dev:
	/*input_free_device(input_dev);*/
	wake_lock_destroy(&data->fw_wake_lock);
err_input_devalloc:
#if defined(SEC_FAC_TK)
	kfree(data->fdata);
err_data_alloc_fdata:
#endif
exit_flag_set:
	kfree(data);
err_data_alloc:
	return ret;
}

static int __devexit tc370_remove(struct i2c_client *client)
{
	struct tc370_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif
	free_irq(client->irq, data);
	gpio_free(data->pdata->gpio_int);
	led_classdev_unregister(&data->led);
	destroy_workqueue(data->led_wq);
	destroy_workqueue(data->fw_wq);
	input_unregister_device(data->input_dev);
	input_free_device(data->input_dev);
	data->pdata->setup_power(NULL, false);
#if defined(SEC_FAC_TK)
	kfree(data->fdata);
#endif
	kfree(data);
	return 0;
}

#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
static int tc370_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tc370_data *data = i2c_get_clientdata(client);
	int i;
	int ret;

	mutex_lock(&data->lock);

	/* now the firmware is being updated. IC will keep power state. */
	if (unlikely(wake_lock_active(&data->fw_wake_lock))) {
		dev_info(&data->client->dev, "%s, now fw updating. suspend "
			 "control is ignored.\n", __func__);
		goto out;
	}

	/* fw flash is failed. and the driver is removed */
	if (unlikely(data->fw_flash_state == STATE_FLASH_FAIL)) {
		dev_info(&data->client->dev, "%s: fail to fw flaash. "
			 "driver is removed\n", __func__);
		if (data->fw_wq) {
			dev_err(&client->dev, "%s: destroy wq for fw flash\n",
				__func__);
			destroy_workqueue(data->fw_wq);
			data->fw_wq = NULL;
		}
		goto out;
	}

	if (unlikely(!data->enabled)) {
		dev_info(&client->dev, "%s, already disabled.\n", __func__);
		goto out;
	}

	data->enabled = false;
	disable_irq(client->irq);

	/* report not released key */
	for (i = 0; i < data->num_key; i++)
		input_report_key(data->input_dev, data->keycodes[i], 0);
	input_sync(data->input_dev);

	data->pdata->led_power(false);
	if (data->suspend_type == TC370_SUSPEND_WITH_POWER_OFF) {
		data->pdata->power(false);
	} else if (data->suspend_type == TC370_SUSPEND_WITH_SLEEP_CMD) {
		ret = i2c_smbus_write_byte_data(client, TC370_CMD,
						TC370_CMD_SLEEP);
		if (ret < 0) {
			dev_err(&client->dev, "%s: failed to wt sleep cmd (%d)"
				"\n", __func__, ret);
			goto out;
		}
	}

out:
	mutex_unlock(&data->lock);
	dev_info(&client->dev, "%s (%#x)\n", __func__, data->fw_flash_state);
	return 0;
}

static int tc370_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tc370_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);

	if (unlikely(wake_lock_active(&data->fw_wake_lock))) {
		dev_info(&data->client->dev, "%s, now fw updating. resume "
			 "control is ignored.\n", __func__);
		goto out;
	}

	/* fw flash is failed. and the driver is removed */
	if (unlikely(data->fw_flash_state == STATE_FLASH_FAIL)) {
		dev_info(&data->client->dev, "%s: fail to fw flaash. "
			 "driver is removed\n", __func__);
		goto out;
	}

	if (unlikely(data->enabled)) {
		dev_info(&client->dev, "%s, already enabled.\n", __func__);
		goto out;
	}

	data->pdata->int_set_pull(true);

	if (data->suspend_type == TC370_SUSPEND_WITH_POWER_OFF) {
		data->pdata->power(true);
		msleep(TC370_POWERON_DELAY);
	} else if (data->suspend_type == TC370_SUSPEND_WITH_SLEEP_CMD) {
		gpio_set_value(data->pdata->gpio_int, 0);
		usleep_range(1, 2);
		gpio_set_value(data->pdata->gpio_int, 1);
		gpio_direction_input(data->pdata->gpio_int);
	}
	data->pdata->led_power(true);

	enable_irq(client->irq);
	data->enabled = true;

out:
	mutex_unlock(&data->lock);
	dev_info(&client->dev, "%s\n", __func__);
	return 0;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tc370_early_suspend(struct early_suspend *h)
{
	struct tc370_data *data;
	data = container_of(h, struct tc370_data, early_suspend);
	tc370_suspend(&data->client->dev);
}

static void tc370_late_resume(struct early_suspend *h)
{
	struct tc370_data *data;
	data = container_of(h, struct tc370_data, early_suspend);
	tc370_resume(&data->client->dev);
}
#endif

#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
static const struct dev_pm_ops tc370_pm_ops = {
	.suspend	= tc370_suspend,
	.resume		= tc370_resume,
};
#endif

static const struct i2c_device_id tc370_id[] = {
	{ TC370_DEVICE, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tc370_id);

static struct i2c_driver tc370_driver = {
	.probe		= tc370_probe,
	.remove		= __devexit_p(tc370_remove),
	.driver = {
		.name	= TC370_DEVICE,
		.owner	= THIS_MODULE,
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
		.pm	= &tc370_pm_ops,
#endif
	},
	.id_table	= tc370_id,
};

static int __devinit tc370_init(void)
{
	return i2c_add_driver(&tc370_driver);
}

static void __exit tc370_exit(void)
{
	i2c_del_driver(&tc370_driver);
}

module_init(tc370_init);
module_exit(tc370_exit);

MODULE_DESCRIPTION("CORERIVER TC370 touchkey driver");
MODULE_AUTHOR("Taeyoon Yoon <tyoony.yoon@samsung.com>");
MODULE_LICENSE("GPL");
