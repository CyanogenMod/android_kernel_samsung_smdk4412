/*
 * Copyright (c) 2011 SAMSUNG
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <mach/hardware.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/sensor/gp2a.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <mach/gpio-midas.h>
#include <linux/sensor/sensors_core.h>
#include <linux/printk.h>

/*********** for debug *******************************/
#undef DEBUG

#define	VENDOR		"SHARP"
#define	CHIP_ID		"GP2AP"

#define OFFSET_FILE_PATH "/efs/prox_cal"

#if 1
#define gprintk(fmt, x...) printk(KERN_INFO "%s(%d): "\
fmt, __func__ , __LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/**************************************************/
#if defined(CONFIG_MACH_BAFFIN)
#define PROX_READ_NUM	20
#else
#define PROX_READ_NUM	40
#endif

#define PS_LOW_THD_L		0x08
#define PS_LOW_THD_H		0x09
#define PS_HIGH_THD_L		0x0A
#define PS_HIGH_THD_H		0x0B

/* global var */
static struct i2c_driver opt_i2c_driver;
static struct i2c_client *opt_i2c_client;

int proximity_enable;
char proximity_sensor_detection;
static char proximity_avg_on;

struct gp2a_data {
	struct input_dev *input_dev;
	struct work_struct work;	/* for proximity sensor */
	struct mutex data_mutex;
	struct device *proximity_dev;
	struct gp2a_platform_data *pdata;
	struct wake_lock prx_wake_lock;
	struct hrtimer prox_timer;
	struct workqueue_struct *prox_wq;
	struct work_struct work_prox;
	int enabled;
	int proximity_data;
	int irq;
	int average[3];	/*for proximity adc average */
	ktime_t prox_poll_delay;

	/* Auto Calibration */
	int offset_value;
	int cal_result;
	uint16_t threshold_high;
	bool offset_cal_high;
};

/* initial value for sensor register */
#define COL 8
static u8 gp2a_original_image_030a[COL][2] = {
	/*	{Regster, Value} */
	/*PRST :01(4 cycle at Detection/Non-detection),
	   ALSresolution :16bit, range *128   //0x1F -> 5F by sharp */
	{0x01, 0x63},
	/*ALC : 0, INTTYPE : 1, PS mode resolution : 12bit, range*1 */
	{0x02, 0x1A},
	/*LED drive current 110mA, Detection/Non-detection judgment output */
	{0x03, 0x3C},
	/*	{0x04 , 0x00}, */
	/*	{0x05 , 0x00}, */
	/*	{0x06 , 0xFF}, */
	/*	{0x07 , 0xFF}, */
#if defined(CONFIG_MACH_BAFFIN)
	{0x08, 0x08},
#else
	{0x08, 0x09},		/*PS mode LTH(Loff):  (??mm) */
#endif
	{0x09, 0x00},		/*PS mode LTH(Loff) : */
	{0x0A, 0x0A},		/*PS mode HTH(Lon) : (??mm) */
	{0x0B, 0x00},		/* PS mode HTH(Lon) : */
	/* {0x13 , 0x08}, by sharp for internal calculation (type:0) */
	/*alternating mode (PS+ALS), TYPE=1
	   (0:externel 1:auto calculated mode) //umfa.cal */
	{0x00, 0xC0}
};

static u8 gp2a_original_image[COL][2] = {
	/*  {Regster, Value} */
	/*PRST :01(4 cycle at Detection/Non-detection),
	   ALSresolution :16bit, range *128   //0x1F -> 5F by sharp */
	{0x01, 0x63},
	/*ALC : 0, INTTYPE : 1, PS mode resolution : 12bit, range*1 */
	{0x02, 0x72},
	/*LED drive current 110mA, Detection/Non-detection judgment output */
	{0x03, 0x3C},
	/*	{0x04 , 0x00}, */
	/*	{0x05 , 0x00}, */
	/*	{0x06 , 0xFF}, */
	/*	{0x07 , 0xFF}, */
	{0x08, 0x07},		/*PS mode LTH(Loff):  (??mm) */
	{0x09, 0x00},		/*PS mode LTH(Loff) : */
	{0x0A, 0x08},		/*PS mode HTH(Lon) : (??mm) */
	{0x0B, 0x00},		/* PS mode HTH(Lon) : */
	/* {0x13 , 0x08}, by sharp for internal calculation (type:0) */
	/*alternating mode (PS+ALS), TYPE=1
	   (0:externel 1:auto calculated mode) //umfa.cal */
	{0x00, 0xC0}
};

#define THR_REG_LSB(data, reg) \
	{ \
		reg = (u8)data & 0xff; \
	}
#define THR_REG_MSB(data, reg) \
	{ \
		reg = (u8)data >> 8; \
	}

static int proximity_onoff(u8 onoff);

int is_gp2a030a(void)
{
#if defined(CONFIG_MACH_C1) || \
	defined(CONFIG_MACH_M0) || \
	defined(CONFIG_MACH_GRANDE) || \
	defined(CONFIG_MACH_IRON)
	return (system_rev != 0 && system_rev != 3);
#endif
#if defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_BAFFIN)
	return 1;
#endif
#if defined(CONFIG_MACH_REDWOOD)
	return (system_rev == 0x04);
#endif

	return 0;
}

static int gp2a_update_threshold(u8 (*selected_image)[2],
				unsigned long new_threshold, bool update_reg)
{
	int i, err = 0;
	u8 set_value;

	for (i = 0; i < COL; i++) {
		switch (selected_image[i][0]) {
		case PS_LOW_THD_L:
			/*PS mode LTH(Loff) for low 8bit*/
			set_value = new_threshold & 0x00FF;
			break;

		case PS_LOW_THD_H:
			/*PS mode LTH(Loff) for high 8bit*/
			set_value = (new_threshold & 0xFF00) >> 8;
			break;

		case PS_HIGH_THD_L:
			/*PS mode HTH(Lon) for low 8bit*/
			set_value = (new_threshold+1) & 0x00FF;
			break;

		case PS_HIGH_THD_H:
			/* PS mode HTH(Lon) for high 8bit*/
			set_value = ((new_threshold+1) & 0xFF00) >> 8;
			break;

		default:
			continue;
		}

		if (update_reg)
			err = opt_i2c_write(selected_image[i][0], &set_value);

		if (err) {
			pr_err("%s : setting error i = %d, err=%d\n",
			 __func__, i, err);
			return err;
		} else
			selected_image[i][1] = set_value;
	}

	return err;
}

static int proximity_adc_read(struct gp2a_data *data)
{
	int sum[OFFSET_ARRAY_LENGTH];
	int i = OFFSET_ARRAY_LENGTH-1;
	int avg;
	int min;
	int max;
	int total;
	int D2_data;
	unsigned char get_D2_data[2];

	mutex_lock(&data->data_mutex);
	do {
		msleep(50);
		opt_i2c_read(DATA2_LSB, get_D2_data, sizeof(get_D2_data));
		D2_data = (get_D2_data[1] << 8) | get_D2_data[0];
		sum[i] = D2_data;
		if (i == 0) {
			min = sum[i];
			max = sum[i];
		} else {
			if (sum[i] < min)
				min = sum[i];
			else if (sum[i] > max)
				max = sum[i];
		}
		total += sum[i];
	} while (i--);
	mutex_unlock(&data->data_mutex);

	total -= (min + max);
	avg = (int)(total / (OFFSET_ARRAY_LENGTH - 2));
	pr_info("%s: offset = %d\n", __func__, avg);

	return avg;
}


static int proximity_open_calibration(struct gp2a_data  *data)
{
	struct file *cal_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(OFFSET_FILE_PATH,
		O_RDONLY, S_IRUGO | S_IWUSR | S_IWGRP);

	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->read(cal_filp,
		(char *)&data->offset_value,
			sizeof(int), &cal_filp->f_pos);
	if (err != sizeof(int)) {
		pr_err("%s: Can't read the cal data from file\n", __func__);
		err = -EIO;
	}

	pr_info("%s: (%d)\n", __func__,
		data->offset_value);

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}

static int proximity_do_calibrate(struct gp2a_data  *data,
			bool do_calib, bool thresh_set)
{
	struct file *cal_filp;
	int err;
	int xtalk_avg = 0;
	int offset_change = 0;
	uint16_t thrd = 0;
	u8 reg;
	mm_segment_t old_fs;

	u8 (*cal_image)[2] = (is_gp2a030a() ?
			gp2a_original_image_030a : gp2a_original_image);

	if (do_calib) {
		if (thresh_set) {
			/* for proximity_thresh_store */
			data->offset_value =
				data->threshold_high -
				(cal_image[6][1] << 8 | cal_image[5][1]);
		} else {
			/* tap offset button */
			/* get offset value */
			xtalk_avg = proximity_adc_read(data);
			offset_change =
				(cal_image[6][1] << 8 | cal_image[5][1])
				- DEFAULT_HI_THR;
			if (xtalk_avg < offset_change) {
				/* do not need calibration */
				data->cal_result = 0;
				err = 0;
				goto no_cal;
			}
			data->offset_value = xtalk_avg - offset_change;
		}
		/* update threshold */
		thrd = (cal_image[4][1] << 8 | cal_image[3][1])
			+ (data->offset_value);
		THR_REG_LSB(thrd, reg);
		opt_i2c_write(cal_image[3][0], &reg);
		THR_REG_MSB(thrd, reg);
		opt_i2c_write(cal_image[4][0], &reg);

		thrd = (cal_image[4][1] << 8 | cal_image[5][1])
			+(data->offset_value);
		THR_REG_LSB(thrd, reg);
		opt_i2c_write(cal_image[5][0], &reg);
		THR_REG_MSB(thrd, reg);
		opt_i2c_write(cal_image[6][0], &reg);

		/* calibration result */
		if (!thresh_set)
			data->cal_result = 1;
	} else {
		/* tap reset button */
		data->offset_value = 0;
		/* update threshold */
		opt_i2c_write(cal_image[3][0], &cal_image[3][1]);
		opt_i2c_write(cal_image[4][0], &cal_image[4][1]);
		opt_i2c_write(cal_image[5][0], &cal_image[5][1]);
		opt_i2c_write(cal_image[6][0], &cal_image[6][1]);
		/* calibration result */
		data->cal_result = 2;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(OFFSET_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY,
			S_IRUGO | S_IWUSR | S_IWGRP);

	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&data->offset_value, sizeof(int),
			&cal_filp->f_pos);
	if (err != sizeof(int)) {
		pr_err("%s: Can't write the cal data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
no_cal:
	return err;
}

static int proximity_manual_offset(struct gp2a_data  *data, u8 change_on)
{
	struct file *cal_filp;
	int err;
	int16_t thrd;
	u8 reg;
	mm_segment_t old_fs;

	u8 (*manual_image)[2] = (is_gp2a030a() ?
			gp2a_original_image_030a : gp2a_original_image);

	data->offset_value = change_on;
	/* update threshold */
	thrd = manual_image[3][1]+(data->offset_value);
	THR_REG_LSB(thrd, reg);
	opt_i2c_write(manual_image[3][0], &reg);
	THR_REG_MSB(thrd, reg);
	opt_i2c_write(manual_image[4][0], &reg);

	thrd = manual_image[5][1]+(data->offset_value);
	THR_REG_LSB(thrd, reg);
	opt_i2c_write(manual_image[5][0], &reg);
	THR_REG_MSB(thrd, reg);
	opt_i2c_write(manual_image[6][0], &reg);

	/* calibration result */
	data->cal_result = 1;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(OFFSET_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY,
			S_IRUGO | S_IWUSR | S_IWGRP);

	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&data->offset_value, sizeof(int),
			&cal_filp->f_pos);
	if (err != sizeof(int)) {
		pr_err("%s: Can't write the cal data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}


/* Proximity Sysfs interface */
static ssize_t
proximity_enable_show(struct device *dev,
		      struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int enabled;

	enabled = data->enabled;

	return sprintf(buf, "%d\n", enabled);
}

static ssize_t
proximity_enable_store(struct device *dev,
		       struct device_attribute *attr,
		       const char *buf, size_t count)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int value = 0;
	char input;
	int err = 0;

	err = kstrtoint(buf, 10, &value);

	if (err)
		printk(KERN_ERR "%s, kstrtoint failed.", __func__);

	if (value != 0 && value != 1)
		return count;

	gprintk("value = %d\n", value);

	if (data->enabled && !value) {	/* Proximity power off */
		disable_irq(data->irq);

		proximity_enable = value;
		proximity_onoff(0);
		disable_irq_wake(data->irq);
		data->pdata->gp2a_led_on(false);
	} else if (!data->enabled && value) {	/* proximity power on */
		data->pdata->gp2a_led_on(true);
		/*msleep(1); */

		proximity_enable = value;
		proximity_onoff(1);
		enable_irq_wake(data->irq);
		msleep(160);

		input = gpio_get_value(data->pdata->p_out);
		input_report_abs(data->input_dev, ABS_DISTANCE, input);
		input_sync(data->input_dev);

		enable_irq(data->irq);
	}
	data->enabled = value;

	return count;
}

static ssize_t proximity_state_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	static int count;		/*count for proximity average */

	int D2_data = 0;
	unsigned char get_D2_data[2] = { 0, };

	mutex_lock(&data->data_mutex);
	opt_i2c_read(0x10, get_D2_data, sizeof(get_D2_data));
	mutex_unlock(&data->data_mutex);
	D2_data = (get_D2_data[1] << 8) | get_D2_data[0];

	data->average[count] = D2_data;
	count++;
	if (count == PROX_READ_NUM)
		count = 0;
	pr_debug("%s: D2_data = %d\n", __func__, D2_data);
	return snprintf(buf, PAGE_SIZE, "%d\n", D2_data);
}

static ssize_t proximity_avg_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d, %d, %d\n"\
		, data->average[0], data->average[1], data->average[2]);
}

static ssize_t proximity_avg_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if (new_value && !proximity_avg_on) {
		if (!(proximity_enable)) {
			/*data->pdata->gp2a_led_on(true);*/
			proximity_onoff(1);
		}
		hrtimer_start(&data->prox_timer, data->prox_poll_delay,
							HRTIMER_MODE_REL);
		proximity_avg_on = 1;
	} else if (!new_value && proximity_avg_on) {
		int i;

		cancel_work_sync(&data->work_prox);
		hrtimer_cancel(&data->prox_timer);
		proximity_avg_on = 0;

		if (!(proximity_enable)) {
			proximity_onoff(0);
			/*data->pdata->gp2a_led_on(false);*/
		}

		for (i = 0 ; i < 3 ; i++)
			data->average[i] = 0;
	}

	return size;
}

static ssize_t proximity_thresh_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int i;
	int threshold = 0;
	u8 (*selected_image)[2] = (is_gp2a030a() ?
			gp2a_original_image_030a : gp2a_original_image);

	for (i = 0; i < COL; i++) {
		if (selected_image[i][0] == 0x08)
			/*PS mode LTH(Loff) */
			threshold = selected_image[i][1];
		else if (selected_image[i][0] == 0x09)
			/*PS mode LTH(Loff) */
			threshold |= selected_image[i][1]<<8;
	}

	return sprintf(buf, "prox_threshold = %d\n", threshold);
}

static ssize_t proximity_thresh_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	unsigned long threshold;
	int err = 0;

	err = strict_strtoul(buf, 10, &threshold);

	if (err) {
		pr_err("%s, conversion %s to number.\n",
			__func__, buf);
		return err;
	}

	err = gp2a_update_threshold(is_gp2a030a() ?
			gp2a_original_image_030a : gp2a_original_image,
			threshold, true);

	if (err) {
		pr_err("gp2a threshold(with register) update fail.\n");
		return err;
	}

	return size;
}

static ssize_t proximity_cal_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int thresh_hi;
	unsigned char get_D2_data[2];

	msleep(20);
	opt_i2c_read(PS_HT_LSB, get_D2_data, sizeof(get_D2_data));
	thresh_hi = (get_D2_data[1] << 8) | get_D2_data[0];
	data->threshold_high = thresh_hi;
	return sprintf(buf, "%d,%d\n",
			data->offset_value, data->threshold_high);
}

static ssize_t proximity_cal_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	bool do_calib;
	int err;

	if (sysfs_streq(buf, "1")) { /* calibrate cancelation value */
		do_calib = true;
	} else if (sysfs_streq(buf, "0")) { /* reset cancelation value */
		do_calib = false;
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		err = -EINVAL;
		goto done;
	}
	err = proximity_do_calibrate(data, do_calib, false);
	if (err < 0) {
		pr_err("%s: proximity_store_offset() failed\n", __func__);
		goto done;
	}
done:
	return err;
}

static DEVICE_ATTR(enable, 0664, proximity_enable_show, proximity_enable_store);
static DEVICE_ATTR(prox_avg, 0664, proximity_avg_show, proximity_avg_store);
static DEVICE_ATTR(state, 0664, proximity_state_show, NULL);
static DEVICE_ATTR(prox_thresh, S_IRUGO | S_IWUSR,
			proximity_thresh_show, proximity_thresh_store);
static DEVICE_ATTR(prox_cal, 0664, proximity_cal_show, proximity_cal_store);

static struct attribute *proximity_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_state.attr,
#ifdef CONFIG_SLP
	&dev_attr_prox_thresh.attr,
#endif
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_attributes
};

/* sysfs for vendor & name */
static ssize_t proximity_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t proximity_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{

	return is_gp2a030a() ? sprintf(buf, "%s030\n", CHIP_ID)
		: sprintf(buf, "%s020\n", CHIP_ID);
}

static ssize_t proximity_raw_data_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int D2_data = 0;
	unsigned char get_D2_data[2] = { 0, };
	struct gp2a_data *data = dev_get_drvdata(dev);

	msleep(20);
	mutex_lock(&data->data_mutex);
	opt_i2c_read(0x10, get_D2_data, sizeof(get_D2_data));
	mutex_unlock(&data->data_mutex);
	D2_data = (get_D2_data[1] << 8) | get_D2_data[0];

	return sprintf(buf, "%d\n", D2_data);
}

static DEVICE_ATTR(vendor, 0644, proximity_vendor_show, NULL);
static DEVICE_ATTR(name, 0644, proximity_name_show, NULL);
static DEVICE_ATTR(raw_data, 0644, proximity_raw_data_show, NULL);

static void proxsensor_get_avgvalue(struct gp2a_data *data)
{
	int min = 0, max = 0, avg = 0;
	int i;
	u8 proximity_value = 0;
	unsigned char get_D2_data[2] = { 0, };

	mutex_lock(&data->data_mutex);

	for (i = 0; i < PROX_READ_NUM; i++) {
		msleep(20);
		opt_i2c_read(0x10, get_D2_data, sizeof(get_D2_data));
		proximity_value = (get_D2_data[1] << 8) | get_D2_data[0];
		avg += proximity_value;

		if (!i)
			min = proximity_value;
		else if (proximity_value < min)
			min = proximity_value;

		if (proximity_value > max)
			max = proximity_value;
	}
	mutex_unlock(&data->data_mutex);
	avg /= PROX_READ_NUM;

	data->average[0] = min;
	data->average[1] = avg;
	data->average[2] = max;
}

static void gp2a_work_func_prox_avg(struct work_struct *work)
{
	struct gp2a_data *data = container_of(work, struct gp2a_data,
						  work_prox);
	proxsensor_get_avgvalue(data);
}

static enum hrtimer_restart gp2a_prox_timer_func(struct hrtimer *timer)
{
	struct gp2a_data *data
			= container_of(timer, struct gp2a_data, prox_timer);
	queue_work(data->prox_wq, &data->work_prox);
	hrtimer_forward_now(&data->prox_timer, data->prox_poll_delay);
	return HRTIMER_RESTART;
}

irqreturn_t gp2a_irq_handler(int irq, void *gp2a_data_p)
{
	struct gp2a_data *data = gp2a_data_p;

	wake_lock_timeout(&data->prx_wake_lock, 3 * HZ);
#ifdef CONFIG_SLP
	pm_wakeup_event(data->proximity_dev, 0);
#endif

	schedule_work(&data->work);

	gprintk("[PROXIMITY] IRQ_HANDLED.\n");
	return IRQ_HANDLED;
}

/***************************************************
 *
 *	function	: gp2a_setup_irq
 *	description : This function sets up the interrupt.
 *
 */
static int gp2a_setup_irq(struct gp2a_data *gp2a)
{
	int rc = -EIO;
	struct gp2a_platform_data *pdata = gp2a->pdata;
	int irq;

	pr_err("%s, start\n", __func__);

	rc = gpio_request(pdata->p_out, "gpio_proximity_out");
	if (rc < 0) {
		pr_err("%s: gpio %d request failed (%d)\n",
		       __func__, pdata->p_out, rc);
		return rc;
	}

	rc = gpio_direction_input(pdata->p_out);
	if (rc < 0) {
		pr_err("%s: failed to set gpio %d as input (%d)\n",
		       __func__, pdata->p_out, rc);
		goto err_gpio_direction_input;
	}

	irq = gpio_to_irq(pdata->p_out);
	rc = request_threaded_irq(irq, NULL,
				  gp2a_irq_handler,
				  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				  "proximity_int", gp2a);
	if (rc < 0) {
		pr_err("%s: request_irq(%d) failed for gpio %d (%d)\n",
		       __func__, irq, pdata->p_out, rc);
		goto err_request_irq;
	}

	/* start with interrupts disabled */
	disable_irq(irq);
	gp2a->irq = irq;

	pr_err("%s, success\n", __func__);

	goto done;

err_request_irq:
err_gpio_direction_input:
	gpio_free(pdata->p_out);
done:
	return rc;
}

/***************************************************
 *
 *  function    : gp2a_work_func_prox
 *  description : This function is for proximity sensor work function.
 *         when INT signal is occured , it gets value the interrupt pin.
 */
static void gp2a_work_func_prox(struct work_struct *work)
{
	struct gp2a_data *gp2a = container_of((struct work_struct *)work,
					      struct gp2a_data, work);

	unsigned char value;
	char result;
	int ret;

	/* 0 : proximity, 1 : away */
	result = gpio_get_value(gp2a->pdata->p_out);
	proximity_sensor_detection = !result;

	input_report_abs(gp2a->input_dev, ABS_DISTANCE, result);
	input_sync(gp2a->input_dev);
	gprintk("Proximity value = %d\n", result);

	disable_irq(gp2a->irq);

	value = 0x0C;
	ret = opt_i2c_write(COMMAND1, &value);	/*Software reset */

	if (result == 0) {	/* detection = Falling Edge */
		if (lightsensor_mode == 0)	/* Low mode */
			value = 0x23;
		else		/* High mode */
			value = 0x27;
		ret = opt_i2c_write(COMMAND2, &value);
	} else {		/* none Detection */
		if (lightsensor_mode == 0)	/* Low mode */
			value = 0x63;
		else		/* High mode */
			value = 0x67;
		ret = opt_i2c_write(COMMAND2, &value);
	}

	enable_irq(gp2a->irq);

	value = 0xCC;
	ret = opt_i2c_write(COMMAND1, &value);

	gp2a->proximity_data = result;
#ifdef DEBUG
	gprintk("proximity = %d, lightsensor_mode=%d\n",
		result, lightsensor_mode);
#endif
}

static int opt_i2c_init(void)
{
	if (i2c_add_driver(&opt_i2c_driver)) {
		printk(KERN_ERR "i2c_add_driver failed\n");
		return -ENODEV;
	}
	return 0;
}

int opt_i2c_read(u8 reg, unsigned char *rbuf, int len)
{
	int ret = -1;
	struct i2c_msg msg;
	/*int i;*/

	if ((opt_i2c_client == NULL) || (!opt_i2c_client->adapter)) {
		printk(KERN_ERR "%s %d (opt_i2c_client == NULL)\n",
		       __func__, __LINE__);
		return -ENODEV;
	}

	/*gprintk("register num : 0x%x\n", reg); */

	msg.addr = opt_i2c_client->addr;
	msg.flags = I2C_M_WR;
	msg.len = 1;
	msg.buf = &reg;

	ret = i2c_transfer(opt_i2c_client->adapter, &msg, 1);

	if (ret >= 0) {
		msg.flags = I2c_M_RD;
		msg.len = len;
		msg.buf = rbuf;
		ret = i2c_transfer(opt_i2c_client->adapter, &msg, 1);
	}

	if (ret < 0)
		printk(KERN_ERR "%s, i2c transfer error ret=%d\n"\
		, __func__, ret);

	/* for (i=0;i<len;i++)
		gprintk("0x%x, 0x%x\n", reg++,rbuf[i]); */

	return ret;
}

int opt_i2c_write(u8 reg, u8 *val)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int retry = 3;

	if ((opt_i2c_client == NULL) || (!opt_i2c_client->adapter))
		return -ENODEV;

	while (retry--) {
		data[0] = reg;
		data[1] = *val;

		msg->addr = opt_i2c_client->addr;
		msg->flags = I2C_M_WR;
		msg->len = 2;
		msg->buf = data;

		err = i2c_transfer(opt_i2c_client->adapter, msg, 1);
		/*gprintk("0x%x, 0x%x\n", reg, *val); */

		if (err >= 0)
			return 0;
	}
	printk(KERN_ERR "%s, i2c transfer error(%d)\n", __func__, err);
	return err;
}

static int proximity_input_init(struct gp2a_data *data)
{
	int err = 0;

	gprintk("start\n");

	data->input_dev = input_allocate_device();
	if (!data->input_dev) {
		printk(KERN_ERR "%s, error\n", __func__);
		return -ENOMEM;
	}

	input_set_capability(data->input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(data->input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	data->input_dev->name = "proximity_sensor";
	input_set_drvdata(data->input_dev, data);

	err = input_register_device(data->input_dev);
	if (err < 0) {
		input_free_device(data->input_dev);
		return err;
	}

	gprintk("success\n");
	return 0;
}

static int gp2a_opt_probe(struct platform_device *pdev)
{
	struct gp2a_data *gp2a;
	struct gp2a_platform_data *pdata = pdev->dev.platform_data;
	u8 value = 0;
	int err = 0;

	gprintk("probe start!\n");

	if (!pdata) {
		pr_err("%s: missing pdata!\n", __func__);
		return err;
	}

	if (!pdata->gp2a_led_on) {
		pr_err("%s: incomplete pdata!\n", __func__);
		return err;
	}
	/* gp2a power on */
	pdata->gp2a_led_on(true);

	if (pdata->gp2a_get_threshold) {
		gp2a_update_threshold(is_gp2a030a() ?
			gp2a_original_image_030a : gp2a_original_image,
			pdata->gp2a_get_threshold(), false);
	}

	/* allocate driver_data */
	gp2a = kzalloc(sizeof(struct gp2a_data), GFP_KERNEL);
	if (!gp2a) {
		pr_err("kzalloc error\n");
		return -ENOMEM;
	}

	proximity_enable = 0;
	proximity_sensor_detection = 0;
	proximity_avg_on = 0;
	gp2a->enabled = 0;
	gp2a->pdata = pdata;

	/* prox_timer settings. we poll for prox_avg values using a timer. */
	hrtimer_init(&gp2a->prox_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	gp2a->prox_poll_delay = ns_to_ktime(2000 * NSEC_PER_MSEC);
	gp2a->prox_timer.function = gp2a_prox_timer_func;

	gp2a->prox_wq = create_singlethread_workqueue("gp2a_prox_wq");
	if (!gp2a->prox_wq) {
		err = -ENOMEM;
		pr_err("%s: could not create prox workqueue\n", __func__);
		goto err_create_prox_workqueue;
	}

	INIT_WORK(&gp2a->work_prox, gp2a_work_func_prox_avg);
	INIT_WORK(&gp2a->work, gp2a_work_func_prox);

	err = proximity_input_init(gp2a);
	if (err < 0)
		goto error_setup_reg;

	err = sysfs_create_group(&gp2a->input_dev->dev.kobj,
				 &proximity_attribute_group);
	if (err < 0)
		goto err_sysfs_create_group_proximity;

	/* set platdata */
	platform_set_drvdata(pdev, gp2a);

	mutex_init(&gp2a->data_mutex);

	/* wake lock init */
	wake_lock_init(&gp2a->prx_wake_lock, WAKE_LOCK_SUSPEND,
		       "prx_wake_lock");

	/* init i2c */
	opt_i2c_init();

	if (opt_i2c_client == NULL) {
		pr_err("opt_probe failed : i2c_client is NULL\n");
		goto err_no_device;
	} else
		printk(KERN_INFO "opt_i2c_client : (0x%p), address = %x\n",
		       opt_i2c_client, opt_i2c_client->addr);

	/* GP2A Regs INIT SETTINGS  and Check I2C communication */
	value = 0x00;
	/* shutdown mode op[3]=0 */
	err = opt_i2c_write((u8) (COMMAND1), &value);

	if (err < 0) {
		pr_err("%s failed : threre is no such device.\n", __func__);
		goto err_no_device;
	}

	/* Setup irq */
	err = gp2a_setup_irq(gp2a);
	if (err) {
		pr_err("%s: could not setup irq\n", __func__);
		goto err_setup_irq;
	}

	/* set sysfs for proximity sensor */
	gp2a->proximity_dev = sensors_classdev_register("proximity_sensor");
	if (IS_ERR(gp2a->proximity_dev)) {
		pr_err("%s: could not create proximity_dev\n", __func__);
		goto err_proximity_device_create;
	}

	if (device_create_file(gp2a->proximity_dev, &dev_attr_state) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_state.attr.name);
		goto err_proximity_device_create_file1;
	}

	if (device_create_file(gp2a->proximity_dev, &dev_attr_prox_avg) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_prox_avg.attr.name);
		goto err_proximity_device_create_file2;
	}

	if (device_create_file(gp2a->proximity_dev,
						&dev_attr_prox_thresh) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_prox_thresh.attr.name);
		goto err_proximity_device_create_file3;
	}

	if (device_create_file(gp2a->proximity_dev,
						&dev_attr_vendor) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_vendor.attr.name);
		goto err_proximity_device_create_file4;
	}

	if (device_create_file(gp2a->proximity_dev,
						&dev_attr_name) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_name.attr.name);
		goto err_proximity_device_create_file5;
	}

	if (device_create_file(gp2a->proximity_dev, &dev_attr_raw_data) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_raw_data.attr.name);
		goto err_proximity_device_create_file6;
	}

	if (device_create_file(gp2a->proximity_dev, &dev_attr_prox_cal) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_prox_cal.attr.name);
		goto err_proximity_device_create_file7;
	}

#ifdef CONFIG_SLP
	device_init_wakeup(gp2a->proximity_dev, true);
#endif
	dev_set_drvdata(gp2a->proximity_dev, gp2a);

	device_init_wakeup(&pdev->dev, 1);

	gprintk("probe success!\n");

	return 0;

err_proximity_device_create_file7:
	device_remove_file(gp2a->proximity_dev, &dev_attr_prox_cal);
err_proximity_device_create_file6:
	device_remove_file(gp2a->proximity_dev, &dev_attr_raw_data);
err_proximity_device_create_file5:
	device_remove_file(gp2a->proximity_dev, &dev_attr_name);
err_proximity_device_create_file4:
	device_remove_file(gp2a->proximity_dev, &dev_attr_vendor);
err_proximity_device_create_file3:
	device_remove_file(gp2a->proximity_dev, &dev_attr_prox_avg);
err_proximity_device_create_file2:
	device_remove_file(gp2a->proximity_dev, &dev_attr_state);
err_proximity_device_create_file1:
	sensors_classdev_unregister(gp2a->proximity_dev);
err_proximity_device_create:
	gpio_free(pdata->p_out);
err_setup_irq:
err_no_device:
	sysfs_remove_group(&gp2a->input_dev->dev.kobj,
			   &proximity_attribute_group);
	wake_lock_destroy(&gp2a->prx_wake_lock);
	mutex_destroy(&gp2a->data_mutex);
err_sysfs_create_group_proximity:
	input_unregister_device(gp2a->input_dev);
error_setup_reg:
	destroy_workqueue(gp2a->prox_wq);
err_create_prox_workqueue:
	kfree(gp2a);
	return err;
}

static int gp2a_opt_remove(struct platform_device *pdev)
{
	struct gp2a_data *gp2a = platform_get_drvdata(pdev);

	if (gp2a == NULL) {
		printk(KERN_ERR "%s, gp2a_data is NULL!!!!!\n", __func__);
		return -1;
	}

	if (gp2a->enabled) {
		disable_irq(gp2a->irq);
		proximity_enable = 0;
		proximity_onoff(0);
		disable_irq_wake(gp2a->irq);
#ifndef CONFIG_MACH_MIDAS_02_BD
		gp2a->pdata->gp2a_led_on(false);
#endif
		gp2a->enabled = 0;
	}

	hrtimer_cancel(&gp2a->prox_timer);
	cancel_work_sync(&gp2a->work_prox);
	destroy_workqueue(gp2a->prox_wq);
#ifdef CONFIG_SLP
	device_init_wakeup(gp2a->proximity_dev, false);
#endif
	device_remove_file(gp2a->proximity_dev, &dev_attr_prox_thresh);
	device_remove_file(gp2a->proximity_dev, &dev_attr_prox_avg);
	device_remove_file(gp2a->proximity_dev, &dev_attr_state);
	device_remove_file(gp2a->proximity_dev, &dev_attr_vendor);
	device_remove_file(gp2a->proximity_dev, &dev_attr_name);
	device_remove_file(gp2a->proximity_dev, &dev_attr_raw_data);
	sensors_classdev_unregister(gp2a->proximity_dev);

	if (gp2a->input_dev != NULL) {
		sysfs_remove_group(&gp2a->input_dev->dev.kobj,
				   &proximity_attribute_group);
		input_unregister_device(gp2a->input_dev);
		if (gp2a->input_dev != NULL)
			kfree(gp2a->input_dev);
	}

	wake_lock_destroy(&gp2a->prx_wake_lock);
	device_init_wakeup(&pdev->dev, 0);
	free_irq(gp2a->irq, gp2a);
	gpio_free(gp2a->pdata->p_out);
	mutex_destroy(&gp2a->data_mutex);
	kfree(gp2a);

	return 0;
}

static int gp2a_opt_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct gp2a_data *gp2a = platform_get_drvdata(pdev);

	gprintk("\n");

	if (gp2a->enabled) {
		if (device_may_wakeup(&pdev->dev))
			enable_irq_wake(gp2a->irq);
	}

	if (proximity_avg_on) {
		cancel_work_sync(&gp2a->work_prox);
		hrtimer_cancel(&gp2a->prox_timer);
	}

	return 0;
}

static int gp2a_opt_resume(struct platform_device *pdev)
{
	struct gp2a_data *gp2a = platform_get_drvdata(pdev);

	gprintk("\n");

	if (gp2a->enabled) {
		if (device_may_wakeup(&pdev->dev))
			enable_irq_wake(gp2a->irq);
	}

	if (proximity_avg_on)
		hrtimer_start(&gp2a->prox_timer, gp2a->prox_poll_delay,
							HRTIMER_MODE_REL);

	return 0;
}

static int proximity_onoff(u8 onoff)
{
	u8 value;
	int i;
	int err = 0;

#ifdef DEBUG
	gprintk("proximity turn on/off = %d\n", onoff);
#endif

	/* already on light sensor, so must simultaneously
	   turn on light sensor and proximity sensor */
	if (onoff) {
		for (i = 0; i < COL; i++) {
			if (is_gp2a030a())
				err =
				    opt_i2c_write(gp2a_original_image_030a[i][0]
					, &gp2a_original_image_030a[i][1]);
			else
				err =
				    opt_i2c_write(gp2a_original_image[i][0],
						  &gp2a_original_image[i][1]);
			if (err < 0)
				printk(KERN_ERR
				      "%s : turnning on error i = %d, err=%d\n",
				       __func__, i, err);
			lightsensor_mode = 0;
		}
	} else { /* light sensor turn on and proximity turn off */
		if (lightsensor_mode)
			value = 0x67; /*resolution :16bit, range: *8(HIGH) */
		else
			value = 0x63; /* resolution :16bit, range: *128(LOW) */
		opt_i2c_write(COMMAND2, &value);
		/* OP3 : 1(operating mode)
		   OP2 :1(coutinuous operating mode) OP1 : 01(ALS mode) */
		value = 0xD0;
		opt_i2c_write(COMMAND1, &value);
	}

	return 0;
}

static int opt_i2c_remove(struct i2c_client *client)
{
	kfree(client);
	opt_i2c_client = NULL;

	return 0;
}

static int opt_i2c_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	gprintk("start!!!\n");

	if (client == NULL)
		printk(KERN_ERR "GP2A i2c client is NULL !!!\n");

	opt_i2c_client = client;
	gprintk("end!!!\n");

	return 0;
}

static const struct i2c_device_id opt_device_id[] = {
	{"gp2a", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, opt_device_id);

static struct i2c_driver opt_i2c_driver = {
	.driver = {
		   .name = "gp2a",
		   .owner = THIS_MODULE,
		   },
	.probe = opt_i2c_probe,
	.remove = opt_i2c_remove,
	.id_table = opt_device_id,
};

static struct platform_driver gp2a_opt_driver = {
	.probe = gp2a_opt_probe,
	.remove = gp2a_opt_remove,
	.suspend = gp2a_opt_suspend,
	.resume = gp2a_opt_resume,
	.driver = {
		   .name = "gp2a-opt",
		   .owner = THIS_MODULE,
		   },
};

static int __init gp2a_opt_init(void)
{
	int ret;

	ret = platform_driver_register(&gp2a_opt_driver);
	return ret;
}

static void __exit gp2a_opt_exit(void)
{
	platform_driver_unregister(&gp2a_opt_driver);
}

module_init(gp2a_opt_init);
module_exit(gp2a_opt_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("Optical Sensor driver for GP2AP020A00F");
MODULE_LICENSE("GPL");
