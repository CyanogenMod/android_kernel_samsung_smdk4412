/*
 *  wacom_i2c.c - Wacom G5 Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/wacom_i2c.h>
#include <linux/earlysuspend.h>
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include "wacom_i2c_func.h"
#include "wacom_i2c_flash.h"
#ifdef COOR_WORK_AROUND
#include "wacom_i2c_coord_tables.h"
#endif

#define WACOM_UMS_UPDATE
#define WACOM_FW_PATH "/sdcard/firmware/wacom_firm.bin"

unsigned char screen_rotate;
unsigned char user_hand = 1;
static bool epen_reset_result;
static bool epen_checksum_result;

static void wacom_i2c_reset_hw(struct wacom_g5_platform_data *wac_pdata)
{
	/* Reset IC */
	wac_pdata->suspend_platform_hw();
	msleep(200);
	wac_pdata->resume_platform_hw();
	msleep(200);
}

int wacom_i2c_load_fw(struct i2c_client *client)
{
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int ret;
	struct wacom_i2c *wac_i2c = i2c_get_clientdata(client);
	unsigned char *Binary_UMS = NULL;

#if defined(CONFIG_MACH_P4NOTE)
	Binary_UMS = kmalloc(MAX_ADDR_514 + 1, GFP_KERNEL);
#else
	Binary_UMS = kmalloc(WACOM_FW_SIZE, GFP_KERNEL);
#endif

	if (Binary_UMS == NULL) {
		printk(KERN_DEBUG "[E-PEN] %s, kmalloc failed\n", __func__);
		return -ENOENT;
	}

	wacom_i2c_set_firm_data(Binary_UMS);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(WACOM_FW_PATH, O_RDONLY, S_IRUSR);

	if (IS_ERR(fp)) {
		printk(KERN_ERR "[E-PEN] failed to open %s.\n", WACOM_FW_PATH);
		goto open_err;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	printk(KERN_NOTICE "[E-PEN] start, file path %s, size %ld Bytes\n",
	       WACOM_FW_PATH, fsize);

	nread = vfs_read(fp, (char __user *)Binary, fsize, &fp->f_pos);
	printk(KERN_ERR "[E-PEN] nread %ld Bytes\n", nread);
	if (nread != fsize) {
		printk(KERN_ERR
		       "[E-PEN] failed to read firmware file, nread %ld Bytes\n",
		       nread);
		goto read_err;
	}

	ret = wacom_i2c_flash(wac_i2c);
	if (ret < 0) {
		printk(KERN_ERR "[E-PEN] failed to write firmware(%d)\n", ret);
		goto fw_write_err;
	}

	wacom_i2c_set_firm_data(NULL);

	kfree(Binary_UMS);

	filp_close(fp, current->files);
	set_fs(old_fs);

	return 0;

 open_err:
	kfree(Binary_UMS);
	set_fs(old_fs);
	return -ENOENT;

 read_err:
	kfree(Binary_UMS);
	filp_close(fp, current->files);
	set_fs(old_fs);
	return -EIO;

 fw_write_err:
	kfree(Binary_UMS);
	filp_close(fp, current->files);
	set_fs(old_fs);
	return -1;
}

#if defined(CONFIG_MACH_Q1_BD)
int wacom_i2c_firm_update(struct wacom_i2c *wac_i2c)
{
	int ret;
	int retry = 3;
	const struct firmware *firm_data = NULL;

	while (retry--) {
		ret =
		    request_firmware(&firm_data, firmware_name,
				     &wac_i2c->client->dev);
		if (ret < 0) {
			printk(KERN_ERR
			       "[E-PEN] Unable to open firmware. ret %d retry %d\n",
			       ret, retry);
			continue;
		}
		wacom_i2c_set_firm_data((unsigned char *)firm_data->data);

		ret = wacom_i2c_flash(wac_i2c);

		if (ret == 0) {
			release_firmware(firm_data);
			break;
		}

		printk(KERN_ERR "[E-PEN] failed to write firmware(%d)\n", ret);
		release_firmware(firm_data);

		/* Reset IC */
		wacom_i2c_reset_hw(wac_i2c->wac_pdata);
	}

	if (ret < 0)
		return -1;

	return 0;
}
#endif

static void update_work_func(struct work_struct *work)
{
	int ret = 0;
	int retry = 2;
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, update_work);

	printk(KERN_DEBUG "[E-PEN] %s\n", __func__);

	disable_irq(wac_i2c->irq);
#ifdef WACOM_PDCT_WORK_AROUND
	disable_irq(wac_i2c->irq_pdct);
#endif

	while (retry--) {
		printk(KERN_DEBUG "[E-PEN] INIT_FIRMWARE_FLASH is enabled.\n");
		ret = wacom_i2c_flash(wac_i2c);
		if (ret == 0)
			break;

		printk(KERN_DEBUG "[E-PEN] update failed. retry %d, ret %d\n",
		       retry, ret);

		/* Reset IC */
		wacom_i2c_reset_hw(wac_i2c->wac_pdata);
	}
	msleep(800);
	printk(KERN_DEBUG "[E-PEN] flashed.(%d)\n", ret);

	wacom_i2c_query(wac_i2c);

	enable_irq(wac_i2c->irq);
#ifdef WACOM_PDCT_WORK_AROUND
	enable_irq(wac_i2c->irq_pdct);
#endif
}

static irqreturn_t wacom_interrupt(int irq, void *dev_id)
{
	struct wacom_i2c *wac_i2c = dev_id;
	wacom_i2c_coord(wac_i2c);
	return IRQ_HANDLED;
}

#if defined(WACOM_PDCT_WORK_AROUND)
static irqreturn_t wacom_interrupt_pdct(int irq, void *dev_id)
{
	struct wacom_i2c *wac_i2c = dev_id;

	wac_i2c->pen_pdct = gpio_get_value(wac_i2c->wac_pdata->gpio_pendct);

	printk(KERN_DEBUG "[E-PEN] pdct %d(%d)\n",
		wac_i2c->pen_pdct, wac_i2c->pen_prox);

	if (wac_i2c->pen_pdct == PDCT_NOSIGNAL) {
		/* If rdy is 1, pen is still working*/
		if (wac_i2c->pen_prox == 0)
			forced_release(wac_i2c);
	} else if (wac_i2c->pen_prox == 0)
		forced_hover(wac_i2c);

	return IRQ_HANDLED;
}
#endif

static void wacom_i2c_set_input_values(struct i2c_client *client,
				       struct wacom_i2c *wac_i2c,
				       struct input_dev *input_dev)
{
	/*Set input values before registering input device */

	input_dev->name = "sec_e-pen";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->evbit[0] |= BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

	__set_bit(ABS_X, input_dev->absbit);
	__set_bit(ABS_Y, input_dev->absbit);
	__set_bit(ABS_PRESSURE, input_dev->absbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(BTN_TOOL_PEN, input_dev->keybit);
	__set_bit(BTN_TOOL_RUBBER, input_dev->keybit);
	__set_bit(BTN_STYLUS, input_dev->keybit);
	__set_bit(KEY_UNKNOWN, input_dev->keybit);
	/*  __set_bit(BTN_STYLUS2, input_dev->keybit); */
	/*  __set_bit(ABS_MISC, input_dev->absbit); */
}

static int wacom_check_emr_prox(struct wacom_g5_callbacks *cb)
{
	struct wacom_i2c *wac = container_of(cb, struct wacom_i2c, callbacks);
	printk(KERN_DEBUG "[E-PEN] %s:\n", __func__);

	return wac->pen_prox;
}

static int wacom_i2c_remove(struct i2c_client *client)
{
	struct wacom_i2c *wac_i2c = i2c_get_clientdata(client);
	free_irq(client->irq, wac_i2c);
	input_unregister_device(wac_i2c->input_dev);
	kfree(wac_i2c);

	return 0;
}

static void wacom_i2c_early_suspend(struct early_suspend *h)
{
	struct wacom_i2c *wac_i2c =
	    container_of(h, struct wacom_i2c, early_suspend);

	disable_irq(wac_i2c->client->irq);
#ifdef WACOM_PDCT_WORK_AROUND
	disable_irq(wac_i2c->irq_pdct);
#endif

	/* release pen, if it is pressed */
#ifdef WACOM_PDCT_WORK_AROUND
	if (wac_i2c->pen_pdct == PDCT_DETECT_PEN)
#else
	if (wac_i2c->pen_pressed || wac_i2c->side_pressed
		|| wac_i2c->pen_prox)
#endif
		forced_release(wac_i2c);

	if (!wake_lock_active(&wac_i2c->wakelock))
		wac_i2c->wac_pdata->early_suspend_platform_hw();
	printk(KERN_DEBUG "[E-PEN] %s.\n", __func__);
}

static void wacom_i2c_resume_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, resume_work.work);

#if defined(WACOM_PDCT_WORK_AROUND)
	irq_set_irq_type(wac_i2c->irq_pdct, IRQ_TYPE_EDGE_BOTH);
#elif defined(CONFIG_MACH_P4NOTE)
	irq_set_irq_type(wac_i2c->client->irq, IRQ_TYPE_EDGE_RISING);
#endif
	enable_irq(wac_i2c->client->irq);
#ifdef WACOM_PDCT_WORK_AROUND
	enable_irq(wac_i2c->irq_pdct);
#endif
	printk(KERN_DEBUG "[E-PEN] %s\n", __func__);
}

static void wacom_i2c_late_resume(struct early_suspend *h)
{
	struct wacom_i2c *wac_i2c =
	    container_of(h, struct wacom_i2c, early_suspend);
	wac_i2c->wac_pdata->late_resume_platform_hw();
	schedule_delayed_work(&wac_i2c->resume_work, HZ / 5);	/* 200ms */
	printk(KERN_DEBUG "[E-PEN] %s.\n", __func__);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
#define wacom_i2c_suspend	NULL
#define wacom_i2c_resume	NULL

#else
static int wacom_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	wacom_i2c_early_suspend();
	printk(KERN_DEBUG "[E-PEN] %s.\n", __func__);
	return 0;
}

static int wacom_i2c_resume(struct i2c_client *client)
{
	wacom_i2c_late_resume();
	printk(KERN_DEBUG "[E-PEN] %s.\n", __func__);
	return 0;
}
#endif

static ssize_t epen_firm_update_status_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	printk(KERN_DEBUG "[E-PEN] %s:(%d)\n", __func__,
	       wac_i2c->wac_feature->firm_update_status);

	if (wac_i2c->wac_feature->firm_update_status == 2)
		return sprintf(buf, "PASS\n");
	else if (wac_i2c->wac_feature->firm_update_status == 1)
		return sprintf(buf, "DOWNLOADING\n");
	else if (wac_i2c->wac_feature->firm_update_status == -1)
		return sprintf(buf, "FAIL\n");
	else
		return 0;
}

static ssize_t epen_firm_version_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	printk(KERN_DEBUG "[E-PEN] %s: 0x%x|0x%X\n", __func__,
	       wac_i2c->wac_feature->fw_version, Firmware_version_of_file);

#if defined(CONFIG_MACH_P4NOTE)
	return sprintf(buf, "%04X\t%04X\n",
#else
	return sprintf(buf, "0x%X\t0x%X\n",
#endif
			wac_i2c->wac_feature->fw_version,
			Firmware_version_of_file);
}

static ssize_t epen_firmware_update_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	u8 *data;
	u8 buf_if;
	int ret = 1;

	disable_irq(wac_i2c->irq);
#ifdef WACOM_PDCT_WORK_AROUND
	disable_irq(wac_i2c->irq_pdct);
#endif

	buf_if = COM_QUERY;
	data = wac_i2c->wac_feature->data;
	wac_i2c->wac_feature->firm_update_status = 1;

	printk(KERN_DEBUG "[E-PEN] %s:\n", __func__);

	/* F and B are used by Factory app, and R is used when boot */
	switch (*buf) {
#ifdef WACOM_UMS_UPDATE
		/* Block UMS update for MR */
	case 'F':
		printk(KERN_ERR "[E-PEN] Start firmware flashing (UMS).\n");
		ret = wacom_i2c_load_fw(wac_i2c->client);
		break;
#endif
	case 'B':
		printk(KERN_ERR
		       "[E-PEN] Start firmware flashing (kernel image).\n");
#ifdef CONFIG_MACH_Q1_BD
		ret = wacom_i2c_firm_update(wac_i2c);
#else
		ret = wacom_i2c_flash(wac_i2c);
#endif
		break;

#ifdef CONFIG_MACH_Q1_BD
	case 'R':
		/* Q1 board rev 0.3 */
		if (system_rev >= 6) {
			if (wac_i2c->wac_feature->fw_version <
			    Firmware_version_of_file)
				ret = wacom_i2c_firm_update(wac_i2c);
		} else {
			if (wac_i2c->wac_feature->fw_version !=
			    Firmware_version_of_file)
				ret = wacom_i2c_firm_update(wac_i2c);
		}
		if (ret > 0) {
			enable_irq(wac_i2c->irq);
#ifdef WACOM_PDCT_WORK_AROUND
			enable_irq(wac_i2c->irq_pdct);
#endif
			return count;
		}
		break;
#endif
	default:
		printk(KERN_ERR "[E-PEN] wrong parameter.\n");
		goto param_err;
		break;
	}

	if (ret < 0) {
		printk(KERN_ERR "[E-PEN] failed to flash firmware.\n");
		goto failure;
	}

	printk(KERN_ERR "[E-PEN] Finish firmware flashing.\n");

	msleep(800);
	wacom_i2c_query(wac_i2c);
	wac_i2c->wac_feature->firm_update_status = 2;

	enable_irq(wac_i2c->irq);
#ifdef WACOM_PDCT_WORK_AROUND
	enable_irq(wac_i2c->irq_pdct);
#endif

	return count;

 param_err:

 failure:
	wac_i2c->wac_feature->firm_update_status = -1;
	enable_irq(wac_i2c->irq);
#ifdef WACOM_PDCT_WORK_AROUND
	enable_irq(wac_i2c->irq_pdct);
#endif
	return -1;
}

#if defined(CONFIG_MACH_P4NOTE)
static ssize_t epen_sampling_rate_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	int value;
	char mode;

	if (sscanf(buf, "%d", &value) == 1) {
		switch (value) {
		case 40:
			mode = COM_SAMPLERATE_40;
			break;
		case 80:
			mode = COM_SAMPLERATE_80;
			break;
		case 133:
			mode = COM_SAMPLERATE_133;
			break;
		default:
			pr_err("[E-PEN] Invalid sampling rate value\n");
			count = -1;
			goto fail;
		}

		disable_irq(wac_i2c->irq);
		if (1 == i2c_master_send(wac_i2c->client, &mode, 1)) {
			printk(KERN_DEBUG "[E-PEN] sampling rate %d\n", value);
			msleep(100);
		} else {
			pr_err("[E-PEN] I2C write error\n");
			enable_irq(wac_i2c->irq);
			count = -1;
		}
		enable_irq(wac_i2c->irq);

	} else {
		pr_err("[E-PEN] can't get sampling rate data\n");
		count = -1;
	}
 fail:
	return count;
}
#else
static ssize_t epen_rotation_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	static bool factory_test;
	static unsigned char last_rotation;
	unsigned int val;

	sscanf(buf, "%u", &val);

	/* Fix the rotation value to 0(Portrait) when factory test(15 mode) */
	if (val == 100 && !factory_test) {
		factory_test = true;
		screen_rotate = 0;
		printk(KERN_DEBUG "[E-PEN] %s, enter factory test mode\n",
		       __func__);
	} else if (val == 200 && factory_test) {
		factory_test = false;
		screen_rotate = last_rotation;
		printk(KERN_DEBUG "[E-PEN] %s, exit factory test mode\n",
		       __func__);
	}

	/* Framework use index 0, 1, 2, 3 for rotation 0, 90, 180, 270 */
	/* Driver use same rotation index */
	if (val >= 0 && val <= 3) {
		if (factory_test)
			last_rotation = val;
		else
			screen_rotate = val;
	}

	/* 0: Portrait 0, 1: Landscape 90, 2: Portrait 180 3: Landscape 270 */
	printk(KERN_DEBUG "[E-PEN] %s: rotate=%d\n", __func__, screen_rotate);

	return count;
}

static ssize_t epen_hand_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	unsigned int val;

	sscanf(buf, "%u", &val);

	if (val == 0 || val == 1)
		user_hand = (unsigned char)val;

	/* 0:Left hand, 1:Right Hand */
	printk(KERN_DEBUG "[E-PEN] %s: hand=%u\n", __func__, user_hand);

	return count;
}
#endif

static ssize_t epen_reset_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	int ret;
	int val;

	sscanf(buf, "%d", &val);

	if (val == 1) {
		disable_irq(wac_i2c->irq);
#ifdef WACOM_PDCT_WORK_AROUND
		disable_irq(wac_i2c->irq_pdct);
#endif

		/* Reset IC */
		wacom_i2c_reset_hw(wac_i2c->wac_pdata);

		/* I2C Test */
		ret = wacom_i2c_query(wac_i2c);

		enable_irq(wac_i2c->irq);
#ifdef WACOM_PDCT_WORK_AROUND
		enable_irq(wac_i2c->irq_pdct);
#endif

		if (ret < 0)
			epen_reset_result = false;
		else
			epen_reset_result = true;

		printk(KERN_DEBUG "[E-PEN] %s, result %d\n", __func__,
		       epen_reset_result);
	}

	return count;
}

static ssize_t epen_reset_result_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	if (epen_reset_result) {
		printk(KERN_DEBUG "[E-PEN] %s, PASS\n", __func__);
		return sprintf(buf, "PASS\n");
	} else {
		printk(KERN_DEBUG "[E-PEN] %s, FAIL\n", __func__);
		return sprintf(buf, "FAIL\n");
	}
}

static ssize_t epen_checksum_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	bool check_version = false;
	int ret;
	int val;
	int i;
	int retry = 3;
	unsigned char data[6] = { 0, };

	sscanf(buf, "%d", &val);

#if defined(CONFIG_MACH_P4NOTE)
	check_version = true;
#else
	if (wac_i2c->wac_feature->fw_version >= 0x31E)
		check_version = true;
#endif

	if (val == 1 && check_version) {
		disable_irq(wac_i2c->client->irq);
#ifdef WACOM_PDCT_WORK_AROUND
		disable_irq(wac_i2c->irq_pdct);
#endif


		data[0] = COM_CHECKSUM;

		while (retry--) {
			ret = i2c_master_send(wac_i2c->client, &data[0], 1);
			if (ret < 0) {
				printk(KERN_DEBUG
				       "[E-PEN] i2c fail, retry, %d\n",
				       __LINE__);
				continue;
			}

			msleep(200);
			ret = i2c_master_recv(wac_i2c->client, data, 5);
			if (ret < 0) {
				printk(KERN_DEBUG
				       "[E-PEN] i2c fail, retry, %d\n",
				       __LINE__);
				continue;
			} else if (data[0] == 0x1f)
				break;
			printk(KERN_DEBUG "[E-PEN] checksum retry\n");
		}

		if (ret >= 0) {
			printk(KERN_DEBUG
			       "[E-PEN] received checksum %x, %x, %x, %x, %x\n",
			       data[0], data[1], data[2], data[3], data[4]);
		}

		for (i = 0; i < 5; ++i) {
			if (data[i] != Firmware_checksum[i]) {
				printk(KERN_DEBUG
				       "[E-PEN] checksum fail %dth %x %x\n", i,
				       data[i], Firmware_checksum[i]);
				break;
			}
		}
		if (i == 5)
			epen_checksum_result = true;
		else
			epen_checksum_result = false;

		enable_irq(wac_i2c->client->irq);
#ifdef WACOM_PDCT_WORK_AROUND
		enable_irq(wac_i2c->irq_pdct);
#endif

		printk(KERN_DEBUG "[E-PEN] %s, result %d\n", __func__,
		       epen_checksum_result);
	}

	return count;
}

static ssize_t epen_checksum_result_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	if (epen_checksum_result) {
		printk(KERN_DEBUG "[E-PEN] checksum, PASS\n");
		return sprintf(buf, "PASS\n");
	} else {
		printk(KERN_DEBUG "[E-PEN] checksum, FAIL\n");
		return sprintf(buf, "FAIL\n");
	}
}

/* firmware update */
static DEVICE_ATTR(epen_firm_update,
		   S_IWUSR | S_IWGRP, NULL, epen_firmware_update_store);
/* return firmware update status */
static DEVICE_ATTR(epen_firm_update_status,
		   S_IRUGO, epen_firm_update_status_show, NULL);
/* return firmware version */
static DEVICE_ATTR(epen_firm_version, S_IRUGO, epen_firm_version_show, NULL);

#if defined(CONFIG_MACH_P4NOTE)
static DEVICE_ATTR(epen_sampling_rate,
		   S_IWUSR | S_IWGRP, NULL, epen_sampling_rate_store);
#else
/* screen rotation */
static DEVICE_ATTR(epen_rotation, S_IWUSR | S_IWGRP, NULL, epen_rotation_store);
/* hand type */
static DEVICE_ATTR(epen_hand, S_IWUSR | S_IWGRP, NULL, epen_hand_store);
#endif
/* For SMD Test */
static DEVICE_ATTR(epen_reset, S_IWUSR | S_IWGRP, NULL, epen_reset_store);
static DEVICE_ATTR(epen_reset_result,
		   S_IRUSR | S_IRGRP, epen_reset_result_show, NULL);

/* For SMD Test. Check checksum */
static DEVICE_ATTR(epen_checksum, S_IWUSR | S_IWGRP, NULL, epen_checksum_store);
static DEVICE_ATTR(epen_checksum_result, S_IRUSR | S_IRGRP,
		   epen_checksum_result_show, NULL);

static struct attribute *epen_attributes[] = {
	&dev_attr_epen_firm_update.attr,
	&dev_attr_epen_firm_update_status.attr,
	&dev_attr_epen_firm_version.attr,
#if defined(CONFIG_MACH_P4NOTE)
	&dev_attr_epen_sampling_rate.attr,
#else
	&dev_attr_epen_rotation.attr,
	&dev_attr_epen_hand.attr,
#endif
	&dev_attr_epen_reset.attr,
	&dev_attr_epen_reset_result.attr,
	&dev_attr_epen_checksum.attr,
	&dev_attr_epen_checksum_result.attr,
	NULL,
};

static struct attribute_group epen_attr_group = {
	.attrs = epen_attributes,
};

static int wacom_i2c_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct wacom_g5_platform_data *pdata = client->dev.platform_data;
	struct wacom_i2c *wac_i2c;
	struct input_dev *input;
	int ret = 0;

	if (pdata == NULL) {
		printk(KERN_ERR "%s: no pdata\n", __func__);
		ret = -ENODEV;
		goto err_i2c_fail;
	}

	/*Check I2C functionality */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "[E-PEN] No I2C functionality found\n");
		ret = -ENODEV;
		goto err_i2c_fail;
	}

	/*Obtain kernel memory space for wacom i2c */
	wac_i2c = kzalloc(sizeof(struct wacom_i2c), GFP_KERNEL);
	if (NULL == wac_i2c) {
		printk(KERN_ERR "[E-PEN] failed to allocate wac_i2c.\n");
		ret = -ENOMEM;
		goto err_freemem;
	}

	input = input_allocate_device();
	if (NULL == input) {
		printk(KERN_ERR "[E-PEN] failed to allocate input device.\n");
		ret = -ENOMEM;
		goto err_input_allocate_device;
	} else
		wacom_i2c_set_input_values(client, wac_i2c, input);

	wac_i2c->wac_feature = &wacom_feature_EMR;
	wac_i2c->wac_pdata = pdata;
	wac_i2c->input_dev = input;
	wac_i2c->client = client;
	wac_i2c->irq = client->irq;
#ifdef WACOM_PDCT_WORK_AROUND
	wac_i2c->irq_pdct = gpio_to_irq(pdata->gpio_pendct);
#endif

	/*Change below if irq is needed */
	wac_i2c->irq_flag = 1;

	/*Register callbacks */
	wac_i2c->callbacks.check_prox = wacom_check_emr_prox;
	if (wac_i2c->wac_pdata->register_cb)
		wac_i2c->wac_pdata->register_cb(&wac_i2c->callbacks);

	/* Firmware Feature */
	wacom_i2c_init_firm_data();

#if defined(CONFIG_MACH_Q1_BD)
	/* Change Origin offset by rev */
	if (system_rev < 6) {
		origin_offset[0] = origin_offset_48[0];
		origin_offset[1] = origin_offset_48[1];
	}

	/* Reset IC */
	wacom_i2c_reset_hw(wac_i2c->wac_pdata);
#endif
	ret = wacom_i2c_query(wac_i2c);

	if (ret < 0)
		epen_reset_result = false;
	else
		epen_reset_result = true;

	INIT_WORK(&wac_i2c->update_work, update_work_func);

#if defined(CONFIG_MACH_P4NOTE)
	if (pdata->xy_switch) {
		input_set_abs_params(input, ABS_X, WACOM_POSY_OFFSET,
				     wac_i2c->wac_feature->y_max, 4, 0);
		input_set_abs_params(input, ABS_Y, WACOM_POSX_OFFSET,
				     wac_i2c->wac_feature->x_max, 4, 0);
	} else {
		input_set_abs_params(input, ABS_X, WACOM_POSX_OFFSET,
				     wac_i2c->wac_feature->x_max, 4, 0);
		input_set_abs_params(input, ABS_Y, WACOM_POSY_OFFSET,
				     wac_i2c->wac_feature->y_max, 4, 0);
	}
	input_set_abs_params(input, ABS_PRESSURE, 0,
			     wac_i2c->wac_feature->pressure_max, 0, 0);
#else
	input_set_abs_params(wac_i2c->input_dev, ABS_X, pdata->min_x,
			     pdata->max_x, 4, 0);
	input_set_abs_params(wac_i2c->input_dev, ABS_Y, pdata->min_y,
			     pdata->max_y, 4, 0);
	input_set_abs_params(wac_i2c->input_dev, ABS_PRESSURE,
			     pdata->min_pressure, pdata->max_pressure, 0, 0);
#endif
	input_set_drvdata(input, wac_i2c);

	/*Before registering input device, data in each input_dev must be set */
	ret = input_register_device(input);
	if (ret) {
		pr_err("[E-PEN] failed to register input device.\n");
		goto err_register_device;
	}

	/*Change below if irq is needed */
	wac_i2c->irq_flag = 1;

	/*Set client data */
	i2c_set_clientdata(client, wac_i2c);

	/*Initializing for semaphor */
	mutex_init(&wac_i2c->lock);
	wake_lock_init(&wac_i2c->wakelock, WAKE_LOCK_SUSPEND, "wacom");
	INIT_DELAYED_WORK(&wac_i2c->resume_work, wacom_i2c_resume_work);
#if defined(WACOM_IRQ_WORK_AROUND)
	INIT_DELAYED_WORK(&wac_i2c->pendct_dwork, wacom_i2c_pendct_work);
#endif

	/*Request IRQ */
	if (wac_i2c->irq_flag) {
		ret =
		    request_threaded_irq(wac_i2c->irq, NULL, wacom_interrupt,
					 IRQF_DISABLED | IRQF_TRIGGER_RISING |
					 IRQF_ONESHOT, wac_i2c->name, wac_i2c);
		if (ret < 0) {
			printk(KERN_ERR
			       "[E-PEN]: failed to request irq(%d) - %d\n",
			       wac_i2c->irq, ret);
			goto err_request_irq;
		}

#if defined(WACOM_PDCT_WORK_AROUND)
		ret =
			request_threaded_irq(wac_i2c->irq_pdct, NULL,
					wacom_interrupt_pdct,
					IRQF_DISABLED | IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					wac_i2c->name, wac_i2c);
		if (ret < 0) {
			printk(KERN_ERR
				"[E-PEN]: failed to request irq(%d) - %d\n",
				wac_i2c->irq_pdct, ret);
			goto err_request_irq;
		}
#endif
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	wac_i2c->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	wac_i2c->early_suspend.suspend = wacom_i2c_early_suspend;
	wac_i2c->early_suspend.resume = wacom_i2c_late_resume;
	register_early_suspend(&wac_i2c->early_suspend);
#endif

	wac_i2c->dev = device_create(sec_class, NULL, 0, NULL, "sec_epen");
	if (IS_ERR(wac_i2c->dev))
		printk(KERN_ERR "Failed to create device(wac_i2c->dev)!\n");
	else {
		dev_set_drvdata(wac_i2c->dev, wac_i2c);
		ret = sysfs_create_group(&wac_i2c->dev->kobj, &epen_attr_group);
		if (ret) {
			printk(KERN_ERR
			       "[E-PEN]: failed to create sysfs group\n");
			goto err_sysfs_create_group;
		}
	}
#if defined(CONFIG_MACH_P4NOTE)
	if (Firmware_version_of_file > wac_i2c->wac_feature->fw_version)
		schedule_work(&wac_i2c->update_work);
#elif defined(CONFIG_MACH_Q1_BD)
	/* firmware info */
	printk(KERN_NOTICE "[E-PEN] wacom fw ver : 0x%x, new fw ver : 0x%x\n",
	       wac_i2c->wac_feature->fw_version, Firmware_version_of_file);
#endif

#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
	INIT_DELAYED_WORK(&wac_i2c->dvfs_work, free_dvfs_lock);
	if (exynos_cpufreq_get_level(500000, &wac_i2c->cpufreq_level))
		printk(KERN_ERR "[E-PEN] exynos_cpufreq_get_level Error\n");
#ifdef SEC_BUS_LOCK
	wac_i2c->dvfs_lock_status = false;
#if defined(CONFIG_MACH_P4NOTE)
	wac_i2c->bus_dev = dev_get("exynos-busfreq");
#endif	/* CONFIG_MACH_P4NOTE */
#endif	/* SEC_BUS_LOCK */
#endif	/* CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK */

	return 0;

 err_sysfs_create_group:
	free_irq(wac_i2c->irq, wac_i2c);
#ifdef WACOM_PDCT_WORK_AROUND
	free_irq(wac_i2c->irq_pdct, wac_i2c);
#endif
 err_request_irq:
 err_register_device:
	input_unregister_device(input);
	input = NULL;
 err_input_allocate_device:
	input_free_device(input);
 err_freemem:
	kfree(wac_i2c);
 err_i2c_fail:
	return ret;
}

static const struct i2c_device_id wacom_i2c_id[] = {
	{"wacom_g5sp_i2c", 0},
	{},
};

/*Create handler for wacom_i2c_driver*/
static struct i2c_driver wacom_i2c_driver = {
	.driver = {
		   .name = "wacom_g5sp_i2c",
		   },
	.probe = wacom_i2c_probe,
	.remove = wacom_i2c_remove,
	.suspend = wacom_i2c_suspend,
	.resume = wacom_i2c_resume,
	.id_table = wacom_i2c_id,
};

static int __init wacom_i2c_init(void)
{
	int ret = 0;

#if defined(WACOM_SLEEP_WITH_PEN_SLP)
	printk(KERN_ERR "[E-PEN] %s: Sleep type-PEN_SLP pin\n", __func__);
#elif defined(WACOM_SLEEP_WITH_PEN_LDO_EN)
	printk(KERN_ERR "[E-PEN] %s: Sleep type-PEN_LDO_EN pin\n", __func__);
#endif

	ret = i2c_add_driver(&wacom_i2c_driver);
	if (ret)
		printk(KERN_ERR "[E-PEN] fail to i2c_add_driver\n");
	return ret;
}

static void __exit wacom_i2c_exit(void)
{
	i2c_del_driver(&wacom_i2c_driver);
}

late_initcall(wacom_i2c_init);
module_exit(wacom_i2c_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Driver for Wacom G5SP Digitizer Controller");

MODULE_LICENSE("GPL");
