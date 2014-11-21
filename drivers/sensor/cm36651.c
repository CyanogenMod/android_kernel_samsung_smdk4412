/* driver/sensor/cm36651.c
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
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sensor/cm36651.h>
#include <linux/sensor/sensors_core.h>

/* For debugging */
#undef	CM36651_DEBUG

#define	VENDOR		"CAPELLA"
#define	CHIP_ID		"CM36651"

#define I2C_M_WR	0 /* for i2c Write */
#define I2c_M_RD	1 /* for i2c Read */

#define REL_RED		REL_X
#define REL_GREEN	REL_Y
#define REL_BLUE	REL_Z
#define REL_WHITE	REL_MISC

/* slave addresses */
#define CM36651_ALS	0x30 /* 7bits : 0x18 */
#define CM36651_PS	0x32 /* 7bits : 0x19 */

/* register addresses */
/* Ambient light sensor */
#define CS_CONF1	0x00
#define CS_CONF2	0x01
#define ALS_WH_M	0x02
#define ALS_WH_L	0x03
#define ALS_WL_M	0x04
#define ALS_WL_L	0x05
#define CS_CONF3	0x06

#define RED		0x00
#define GREEN		0x01
#define BLUE		0x02
#define WHITE		0x03

/* Proximity sensor */
#define PS_CONF1	0x00
#define PS_THD		0x01
#define PS_CANC		0x02
#define PS_CONF2	0x03

#define ALS_REG_NUM	3
#define PS_REG_NUM	4

/* Intelligent Cancelation*/
#define CM36651_CANCELATION
#ifdef CM36651_CANCELATION
#ifdef CONFIG_SLP
#define CANCELATION_FILE_PATH	"/csa/sensor/prox_cal_data"
#else
#define CANCELATION_FILE_PATH	"/efs/prox_cal"
#endif
#define CANCELATION_THRESHOLD	9
#endif

#define PROX_READ_NUM	40
 /*lightsnesor log time 6SEC 200mec X 30*/
#define LIGHT_LOG_TIME	30
#define LIGHT_ADD_STARTTIME 300000000
enum {
	LIGHT_ENABLED = BIT(0),
	PROXIMITY_ENABLED = BIT(1),
};

/* register settings */
static u8 als_reg_setting[ALS_REG_NUM][2] = {
	{0x00, 0x04},	/* CS_CONF1 */
	{0x01, 0x08},	/* CS_CONF2 */
/* Don't care.
 *	{0x02, 0x00}, ALS_WH_M
 *	{0x03, 0x00}, ALS_WH_L
 *	{0x04, 0x00}, ALS_WL_M
 *	{0x05, 0x00}, ALS_WL_L
 */
	{0x06, 0x00}	/* CS_CONF3 */
};

/* Change threshold value on the midas-sensor.c */
static u8 ps_reg_setting[PS_REG_NUM][2] = {
	{0x00, 0x3C},	/* PS_CONF1 */
	{0x01, 0x09},	/* PS_THD */
	{0x02, 0x00},	/* PS_CANC */
	{0x03, 0x13},	/* PS_CONF2 */
};

 /* driver data */
struct cm36651_data {
	struct i2c_client *i2c_client;
	struct wake_lock prx_wake_lock;
	struct input_dev *proximity_input_dev;
	struct input_dev *light_input_dev;
	struct cm36651_platform_data *pdata;
	struct mutex power_lock;
	struct mutex read_lock;
	struct hrtimer light_timer;
	struct hrtimer prox_timer;
	struct workqueue_struct *light_wq;
	struct workqueue_struct *prox_wq;
	struct work_struct work_light;
	struct work_struct work_prox;
	struct device *proximity_dev;
	struct device *light_dev;
	ktime_t light_poll_delay;
	ktime_t prox_poll_delay;
	int irq;
	u8 power_state;
	int avg[3];
	u16 color[4];
	int count_log_time;
#ifdef CM36651_CANCELATION
	u8 default_threshold;
#endif
};

int cm36651_i2c_read_byte(struct cm36651_data *cm36651, u8 addr, u8 * val)
{
	int err = 0;
	int retry = 3;
	struct i2c_msg msg[1];
	struct i2c_client *client = cm36651->i2c_client;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	/* send slave address & command */
	msg->addr = addr >> 1;
	msg->flags = I2C_M_RD;
	msg->len = 1;
	msg->buf = val;

	while (retry--) {
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0)
			return err;
	}
	pr_err("%s: i2c read failed at addr 0x%x: %d\n", __func__, addr, err);
	return err;
}

int cm36651_i2c_read_word(struct cm36651_data *cm36651, u8 addr, u8 command,
			  u16 *val)
{
	int err = 0;
	int retry = 3;
	struct i2c_client *client = cm36651->i2c_client;
	struct i2c_msg msg[2];
	unsigned char data[2] = {0,};
	u16 value = 0;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		/* send slave address & command */
		msg[0].addr = addr>>1;
		msg[0].flags = I2C_M_WR;
		msg[0].len = 1;
		msg[0].buf = &command;

		/* read word data */
		msg[1].addr = addr>>1;
		msg[1].flags = I2C_M_RD;
		msg[1].len = 2;
		msg[1].buf = data;

		err = i2c_transfer(client->adapter, msg, 2);

		if (err >= 0) {
			value = (u16)data[1];
			*val = (value << 8) | (u16)data[0];
			return err;
		}
	}
	printk(KERN_ERR "%s, i2c transfer error ret=%d\n", __func__, err);
	return err;
}

int cm36651_i2c_write_byte(struct cm36651_data *cm36651, u8 addr, u8 command,
			   u8 val)
{
	int err = 0;
	struct i2c_client *client = cm36651->i2c_client;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int retry = 3;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		data[0] = command;
		data[1] = val;

		/* send slave address & command */
		msg->addr = addr>>1;
		msg->flags = I2C_M_WR;
		msg->len = 2;
		msg->buf = data;

		err = i2c_transfer(client->adapter, msg, 1);

		if (err >= 0)
			return 0;
	}
	pr_err("%s, i2c transfer error(%d)\n", __func__, err);
	return err;
}

static void cm36651_light_enable(struct cm36651_data *cm36651)
{
	/* enable setting */
	int64_t start_add_time = 0;
	start_add_time = ktime_to_ns(cm36651->light_poll_delay)\
	+ LIGHT_ADD_STARTTIME;
	cm36651_i2c_write_byte(cm36651, CM36651_ALS, CS_CONF1,
		als_reg_setting[0][1]);
	cm36651_i2c_write_byte(cm36651, CM36651_ALS, CS_CONF2,
		als_reg_setting[1][1]);
#if defined(CONFIG_MACH_C1_KOR_SKT) || defined(CONFIG_MACH_C1_KOR_KT)\
|| defined(CONFIG_MACH_C1_KOR_LGT)
	hrtimer_start(&cm36651->light_timer, ns_to_ktime(start_add_time),
					HRTIMER_MODE_REL);
#else
	hrtimer_start(&cm36651->light_timer, cm36651->light_poll_delay,
	      HRTIMER_MODE_REL);
#endif
}

static void cm36651_light_disable(struct cm36651_data *cm36651)
{
	/* disable setting */
	cm36651_i2c_write_byte(cm36651, CM36651_ALS, CS_CONF1,
			       0x01);
	hrtimer_cancel(&cm36651->light_timer);
	cancel_work_sync(&cm36651->work_light);
}

/* sysfs */
static ssize_t cm36651_poll_delay_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(cm36651->light_poll_delay));
}

static ssize_t cm36651_poll_delay_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	mutex_lock(&cm36651->power_lock);
	if (new_delay != ktime_to_ns(cm36651->light_poll_delay)) {
		cm36651->light_poll_delay = ns_to_ktime(new_delay);
		if (cm36651->power_state & LIGHT_ENABLED) {
			cm36651_light_disable(cm36651);
			cm36651_light_enable(cm36651);
		}
		pr_info("%s, poll_delay = %lld\n", __func__, new_delay);
	}
	mutex_unlock(&cm36651->power_lock);

	return size;
}

static ssize_t light_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&cm36651->power_lock);
	pr_info("%s,new_value=%d\n", __func__, new_value);
	if (new_value && !(cm36651->power_state & LIGHT_ENABLED)) {
		cm36651->power_state |= LIGHT_ENABLED;
		cm36651_light_enable(cm36651);
	} else if (!new_value && (cm36651->power_state & LIGHT_ENABLED)) {
		cm36651_light_disable(cm36651);
		cm36651->power_state &= ~LIGHT_ENABLED;
	}
	mutex_unlock(&cm36651->power_lock);
	return size;
}

static ssize_t light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (cm36651->power_state & LIGHT_ENABLED) ? 1 : 0);
}

#ifdef CM36651_CANCELATION
static int proximity_open_cancelation(struct cm36651_data *data)
{
	struct file *cancel_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cancel_filp = filp_open(CANCELATION_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cancel_filp)) {
		err = PTR_ERR(cancel_filp);
		if (err != -ENOENT)
			pr_err("%s: Can't open cancelation file\n", __func__);
		set_fs(old_fs);
		return err;
	}

	err = cancel_filp->f_op->read(cancel_filp,
		(char *)&ps_reg_setting[2][1], sizeof(u8), &cancel_filp->f_pos);
	if (err != sizeof(u8)) {
		pr_err("%s: Can't read the cancel data from file\n", __func__);
		err = -EIO;
	}

	if (ps_reg_setting[2][1] != 0) /*If there is an offset cal data. */
		ps_reg_setting[1][1] = CANCELATION_THRESHOLD;
	pr_info("%s: proximity ps_data = %d, ps_thresh = %d\n",
		__func__, ps_reg_setting[2][1], ps_reg_setting[1][1]);

	filp_close(cancel_filp, current->files);
	set_fs(old_fs);

	return err;
}

static int proximity_store_cancelation(struct device *dev, bool do_calib)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);
	struct file *cancel_filp = NULL;
	mm_segment_t old_fs;
	int err = 0;

	if (do_calib) {
		mutex_lock(&cm36651->read_lock);
		cm36651_i2c_read_byte(cm36651, CM36651_PS,
			&ps_reg_setting[2][1]);
		mutex_unlock(&cm36651->read_lock);
		ps_reg_setting[1][1] = CANCELATION_THRESHOLD;
	} else { /* reset */
		ps_reg_setting[2][1] = 0;
		ps_reg_setting[1][1] = cm36651->default_threshold;
	}

	cm36651_i2c_write_byte(cm36651, CM36651_PS, PS_THD,
		ps_reg_setting[1][1]);
	cm36651_i2c_write_byte(cm36651, CM36651_PS, PS_CANC,
		ps_reg_setting[2][1]);
	pr_info("%s: prox_cal = 0x%x, prox_thresh = %d\n",
		__func__, ps_reg_setting[2][1], ps_reg_setting[1][1]);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cancel_filp = filp_open(CANCELATION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0666);
	if (IS_ERR(cancel_filp)) {
		pr_err("%s: Can't open cancelation file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cancel_filp);
		return err;
	}

	err = cancel_filp->f_op->write(cancel_filp,
		(char *)&ps_reg_setting[2][1], sizeof(u8), &cancel_filp->f_pos);
	if (err != sizeof(u8)) {
		pr_err("%s: Can't write the cancel data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cancel_filp, current->files);
	set_fs(old_fs);

	if (!do_calib) /* delay for clearing */
		msleep(150);

	return err;
}

static ssize_t proximity_cancel_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	bool do_calib;
	int err;

	if (sysfs_streq(buf, "1")) /* calibrate cancelation value */
		do_calib = true;
	else if (sysfs_streq(buf, "0")) /* reset cancelation value */
		do_calib = false;
	else {
		pr_debug("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	err = proximity_store_cancelation(dev, do_calib);
	if (err < 0) {
		pr_err("%s: proximity_store_cancelation() failed\n", __func__);
		return err;
	}

	return size;
}

static ssize_t proximity_cancel_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d,%d\n", ps_reg_setting[2][1],
		ps_reg_setting[1][1]);
}
#endif

static ssize_t proximity_enable_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);
	bool new_value;
	int err = 0;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&cm36651->power_lock);
	pr_info("%s, new_value = %d, threshold = %d\n", __func__, new_value,
		ps_reg_setting[1][1]);
	if (new_value && !(cm36651->power_state & PROXIMITY_ENABLED)) {
		u8 val = 1;
		int i;
		if (cm36651->pdata->cm36651_led_on) {
			cm36651->pdata->cm36651_led_on(true);
			msleep(20);
		}
#ifdef CM36651_CANCELATION
		/* open cancelation data */
		err = proximity_open_cancelation(cm36651);
		if (err < 0 && err != -ENOENT)
			pr_err("%s: proximity_open_cancelation() failed\n",
				__func__);
#endif
		cm36651->power_state |= PROXIMITY_ENABLED;
		/* enable settings */
		for (i = 0; i < 4; i++) {
			cm36651_i2c_write_byte(cm36651, CM36651_PS,
				ps_reg_setting[i][0], ps_reg_setting[i][1]);
		}

		val = gpio_get_value(cm36651->pdata->irq);
		/* 0 is close, 1 is far */
		input_report_abs(cm36651->proximity_input_dev,
			ABS_DISTANCE, val);
		input_sync(cm36651->proximity_input_dev);

		enable_irq(cm36651->irq);
		enable_irq_wake(cm36651->irq);
	} else if (!new_value && (cm36651->power_state & PROXIMITY_ENABLED)) {
		cm36651->power_state &= ~PROXIMITY_ENABLED;
		disable_irq_wake(cm36651->irq);
		disable_irq(cm36651->irq);
		/* disable settings */
		cm36651_i2c_write_byte(cm36651, CM36651_PS, PS_CONF1,
				       0x01);
		if (cm36651->pdata->cm36651_led_on)
			cm36651->pdata->cm36651_led_on(false);
	}
	mutex_unlock(&cm36651->power_lock);
	return size;
}

static ssize_t proximity_enable_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n",
		       (cm36651->power_state & PROXIMITY_ENABLED) ? 1 : 0);
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   cm36651_poll_delay_show, cm36651_poll_delay_store);

static struct device_attribute dev_attr_light_enable =
__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	light_enable_show, light_enable_store);

static struct device_attribute dev_attr_proximity_enable =
__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_enable_show, proximity_enable_store);

static struct attribute *light_sysfs_attrs[] = {
	&dev_attr_light_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group light_attribute_group = {
	.attrs = light_sysfs_attrs,
};

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_proximity_enable.attr,
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};

/* proximity sysfs */
static ssize_t proximity_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);

	return sprintf(buf, "%d,%d,%d\n", cm36651->avg[0],
		cm36651->avg[1], cm36651->avg[2]);
}

static ssize_t proximity_avg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);
	bool new_value = false;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s, invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	pr_info("%s, average enable = %d\n", __func__, new_value);
	mutex_lock(&cm36651->power_lock);
	if (new_value) {
		if (!(cm36651->power_state & PROXIMITY_ENABLED)) {
			if (cm36651->pdata->cm36651_led_on) {
				cm36651->pdata->cm36651_led_on(true);
				msleep(20);
			}
			cm36651_i2c_write_byte(cm36651, CM36651_PS, PS_CONF1,
			ps_reg_setting[0][1]);
		}
		hrtimer_start(&cm36651->prox_timer, cm36651->prox_poll_delay,
			HRTIMER_MODE_REL);
	} else if (!new_value) {
		hrtimer_cancel(&cm36651->prox_timer);
		cancel_work_sync(&cm36651->work_prox);
		if (!(cm36651->power_state & PROXIMITY_ENABLED)) {
			cm36651_i2c_write_byte(cm36651, CM36651_PS, PS_CONF1,
				0x01);
			if (cm36651->pdata->cm36651_led_on)
				cm36651->pdata->cm36651_led_on(false);
		}
	}
	mutex_unlock(&cm36651->power_lock);

	return size;
}

static ssize_t proximity_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);
	u8 proximity_value = 0;

	mutex_lock(&cm36651->power_lock);
	if (!(cm36651->power_state & PROXIMITY_ENABLED)) {
		if (cm36651->pdata->cm36651_led_on) {
			cm36651->pdata->cm36651_led_on(true);
			msleep(20);
		}
		cm36651_i2c_write_byte(cm36651, CM36651_PS, PS_CONF1,
			ps_reg_setting[0][1]);
	}

	mutex_lock(&cm36651->read_lock);
	cm36651_i2c_read_byte(cm36651, CM36651_PS, &proximity_value);
	mutex_unlock(&cm36651->read_lock);

	if (!(cm36651->power_state & PROXIMITY_ENABLED)) {
		cm36651_i2c_write_byte(cm36651, CM36651_PS, PS_CONF1, 0x01);
		if (cm36651->pdata->cm36651_led_on)
			cm36651->pdata->cm36651_led_on(false);
	}
	mutex_unlock(&cm36651->power_lock);

	return sprintf(buf, "%d\n", proximity_value);
}

static ssize_t proximity_thresh_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "prox_threshold = %d\n", ps_reg_setting[1][1]);
}

static ssize_t proximity_thresh_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);
	u8 thresh_value = 0x09;
	int err = 0;

	err = kstrtou8(buf, 10, &thresh_value);
	if (err < 0)
		pr_err("%s, kstrtoint failed.", __func__);

	ps_reg_setting[1][1] = thresh_value;
	err = cm36651_i2c_write_byte(cm36651, CM36651_PS,
			PS_THD, ps_reg_setting[1][1]);
	if (err < 0) {
		pr_err("%s: cm36651_ps_reg is failed. %d\n", __func__,
		       err);
		return err;
	}
	pr_info("%s, new threshold = 0x%x\n",
		__func__, ps_reg_setting[1][1]);
	msleep(150);

	return size;
}

#ifdef CM36651_CANCELATION
static DEVICE_ATTR(prox_cal, 0644, proximity_cancel_show,
	proximity_cancel_store);
#endif
static DEVICE_ATTR(prox_avg, 0644, proximity_avg_show,
	proximity_avg_store);
static DEVICE_ATTR(state, 0644, proximity_state_show, NULL);
static struct device_attribute attr_prox_raw = __ATTR(raw_data, 0644,
	proximity_state_show, NULL);
static DEVICE_ATTR(prox_thresh, 0644, proximity_thresh_show,
	proximity_thresh_store);

/* light sysfs */
static ssize_t light_lux_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);

	return sprintf(buf, "%u,%u,%u,%u\n",
		cm36651->color[0]+1, cm36651->color[1]+1,
		cm36651->color[2]+1, cm36651->color[3]+1);
}

static ssize_t light_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);

	return sprintf(buf, "%u,%u,%u,%u\n",
		cm36651->color[0]+1, cm36651->color[1]+1,
		cm36651->color[2]+1, cm36651->color[3]+1);
}

static DEVICE_ATTR(lux, 0644, light_lux_show, NULL);
static DEVICE_ATTR(raw_data, 0644, light_data_show, NULL);

/* sysfs for vendor & name */
static ssize_t cm36651_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t cm36651_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}
static DEVICE_ATTR(vendor, 0644, cm36651_vendor_show, NULL);
static DEVICE_ATTR(name, 0644, cm36651_name_show, NULL);

/* interrupt happened due to transition/change of near/far proximity state */
irqreturn_t cm36651_irq_thread_fn(int irq, void *data)
{
	struct cm36651_data *cm36651 = data;
	u8 val = 1;
#ifdef CM36651_DEBUG
	static int count;
#endif
	u8 ps_data = 0;

	val = gpio_get_value(cm36651->pdata->irq);
	cm36651_i2c_read_byte(cm36651, CM36651_PS, &ps_data);
#ifdef CM36651_DEBUG
	pr_info("%s: count = %d\n", __func__, count++);
#endif
	/* 0 is close, 1 is far */
	input_report_abs(cm36651->proximity_input_dev, ABS_DISTANCE, val);
	input_sync(cm36651->proximity_input_dev);
	wake_lock_timeout(&cm36651->prx_wake_lock, 3 * HZ);
#ifdef CONFIG_SLP
	pm_wakeup_event(cm36651->proximity_dev, 0);
#endif
	pr_info("%s: val = %d, ps_data = %d (close:0, far:1)\n",
		__func__, val, ps_data);

	return IRQ_HANDLED;
}

static int cm36651_setup_reg(struct cm36651_data *cm36651)
{
	int err = 0, i = 0;
	u8 tmp = 0;

	/* ALS initialization */
	for (i = 0; i < ALS_REG_NUM; i++) {
		err = cm36651_i2c_write_byte(cm36651,
					     CM36651_ALS, als_reg_setting[i][0],
					     als_reg_setting[i][1]);
		if (err < 0) {
			pr_err("%s: cm36651_als_reg is failed. %d\n", __func__,
			       err);
			return err;
		}
	}

	/* PS initialization */
	for (i = 0; i < PS_REG_NUM; i++) {
		err = cm36651_i2c_write_byte(cm36651, CM36651_PS,
			ps_reg_setting[i][0], ps_reg_setting[i][1]);
		if (err < 0) {
			pr_err("%s: cm36651_ps_reg is failed. %d\n", __func__,
			       err);
			return err;
		}
	}

	/* printing the inital proximity value with no contact */
	msleep(50);
	mutex_lock(&cm36651->read_lock);
	err = cm36651_i2c_read_byte(cm36651, CM36651_PS, &tmp);
	mutex_unlock(&cm36651->read_lock);
	if (err < 0) {
		pr_err("%s: read ps_data failed\n", __func__);
		err = -EIO;
	}
	pr_err("%s: initial proximity value = %d\n", __func__, tmp);

	/* turn off */
	cm36651_i2c_write_byte(cm36651,   CM36651_ALS, CS_CONF1, 0x01);
	cm36651_i2c_write_byte(cm36651,   CM36651_PS, PS_CONF1, 0x01);

	pr_info("%s is success.", __func__);
	return err;
}

static int cm36651_setup_irq(struct cm36651_data *cm36651)
{
	int rc = -EIO;
	struct cm36651_platform_data *pdata = cm36651->pdata;

	rc = gpio_request(pdata->irq, "gpio_proximity_out");
	if (rc < 0) {
		pr_err("%s: gpio %d request failed (%d)\n",
		       __func__, pdata->irq, rc);
		return rc;
	}

	rc = gpio_direction_input(pdata->irq);
	if (rc < 0) {
		pr_err("%s: failed to set gpio %d as input (%d)\n",
		       __func__, pdata->irq, rc);
		goto err_gpio_direction_input;
	}

	cm36651->irq = gpio_to_irq(pdata->irq);
	rc = request_threaded_irq(cm36651->irq, NULL,
				  cm36651_irq_thread_fn,
				  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				  "proximity_int", cm36651);
	if (rc < 0) {
		pr_err("%s: request_irq(%d) failed for gpio %d (%d)\n",
		       __func__, cm36651->irq, pdata->irq, rc);
		goto err_request_irq;
	}

	/* start with interrupts disabled */
	disable_irq(cm36651->irq);

	pr_err("%s, success\n", __func__);

	goto done;

err_request_irq:
err_gpio_direction_input:
	gpio_free(pdata->irq);
done:
	return rc;
}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart cm36651_light_timer_func(struct hrtimer *timer)
{
	struct cm36651_data *cm36651
	    = container_of(timer, struct cm36651_data, light_timer);
	queue_work(cm36651->light_wq, &cm36651->work_light);
	hrtimer_forward_now(&cm36651->light_timer, cm36651->light_poll_delay);
	return HRTIMER_RESTART;
}

static void cm36651_work_func_light(struct work_struct *work)
{
	struct cm36651_data *cm36651 = container_of(work, struct cm36651_data,
						    work_light);

	mutex_lock(&cm36651->read_lock);
	cm36651_i2c_read_word(cm36651, CM36651_ALS, RED, &cm36651->color[0]);
	cm36651_i2c_read_word(cm36651, CM36651_ALS, GREEN, &cm36651->color[1]);
	cm36651_i2c_read_word(cm36651, CM36651_ALS, BLUE, &cm36651->color[2]);
	cm36651_i2c_read_word(cm36651, CM36651_ALS, WHITE, &cm36651->color[3]);
	mutex_unlock(&cm36651->read_lock);

	input_report_rel(cm36651->light_input_dev, REL_RED,
		cm36651->color[0]+1);
	input_report_rel(cm36651->light_input_dev, REL_GREEN,
		cm36651->color[1]+1);
	input_report_rel(cm36651->light_input_dev, REL_BLUE,
		cm36651->color[2]+1);
	input_report_rel(cm36651->light_input_dev, REL_WHITE,
		cm36651->color[3]+1);
	input_sync(cm36651->light_input_dev);

	if (cm36651->count_log_time >= LIGHT_LOG_TIME) {
		pr_info("%s, red = %u green = %u blue = %u white = %u\n",
			__func__, cm36651->color[0]+1, cm36651->color[1]+1,
			cm36651->color[2]+1, cm36651->color[3]+1);
		cm36651->count_log_time = 0;
	} else
		cm36651->count_log_time++;

#ifdef CM36651_DEBUG
	pr_info("%s, red = %u green = %u blue = %u white = %u\n",
		__func__, cm36651->color[0]+1, cm36651->color[1]+1,
		cm36651->color[2]+1, val_whitecm36651->color[3]1);
#endif
}

static void proxsensor_get_avg_val(struct cm36651_data *cm36651)
{
	int min = 0, max = 0, avg = 0;
	int i;
	u8 ps_data = 0;

	for (i = 0; i < PROX_READ_NUM; i++) {
		msleep(40);
		cm36651_i2c_read_byte(cm36651, CM36651_PS, &ps_data);
		avg += ps_data;

		if (!i)
			min = ps_data;
		else if (ps_data < min)
			min = ps_data;

		if (ps_data > max)
			max = ps_data;
	}
	avg /= PROX_READ_NUM;

	cm36651->avg[0] = min;
	cm36651->avg[1] = avg;
	cm36651->avg[2] = max;
}

static void cm36651_work_func_prox(struct work_struct *work)
{
	struct cm36651_data *cm36651 = container_of(work, struct cm36651_data,
						  work_prox);
	proxsensor_get_avg_val(cm36651);
}

static enum hrtimer_restart cm36651_prox_timer_func(struct hrtimer *timer)
{
	struct cm36651_data *cm36651
			= container_of(timer, struct cm36651_data, prox_timer);
	queue_work(cm36651->prox_wq, &cm36651->work_prox);
	hrtimer_forward_now(&cm36651->prox_timer, cm36651->prox_poll_delay);
	return HRTIMER_RESTART;
}

static int cm36651_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct cm36651_data *cm36651 = NULL;

	pr_info("%s is called.\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		return ret;
	}

	cm36651 = kzalloc(sizeof(struct cm36651_data), GFP_KERNEL);
	if (!cm36651) {
		pr_err
		    ("%s: failed to alloc memory for RGB sensor module data\n",
		     __func__);
		return -ENOMEM;
	}

	cm36651->pdata = client->dev.platform_data;
	cm36651->i2c_client = client;
	i2c_set_clientdata(client, cm36651);
	mutex_init(&cm36651->power_lock);
	mutex_init(&cm36651->read_lock);

	/* wake lock init for proximity sensor */
	wake_lock_init(&cm36651->prx_wake_lock, WAKE_LOCK_SUSPEND,
		       "prx_wake_lock");
	if (cm36651->pdata->cm36651_led_on) {
		cm36651->pdata->cm36651_led_on(true);
		msleep(20);
	}
	/* Check if the device is there or not. */
	ret = cm36651_i2c_write_byte(cm36651, CM36651_PS, CS_CONF1, 0x01);
	if (ret < 0) {
		pr_err("%s: cm36651 is not connected.(%d)\n", __func__, ret);
		goto err_setup_reg;
	}
	/* setup initial registers */
	if (cm36651->pdata->cm36651_get_threshold)
		ps_reg_setting[1][1] = cm36651->pdata->cm36651_get_threshold();
#ifdef CM36651_CANCELATION
	cm36651->default_threshold = ps_reg_setting[1][1];
#endif
	ret = cm36651_setup_reg(cm36651);
	if (ret < 0) {
		pr_err("%s: could not setup regs\n", __func__);
		goto err_setup_reg;
	}
	if (cm36651->pdata->cm36651_led_on)
		cm36651->pdata->cm36651_led_on(false);

	/* allocate proximity input_device */
	cm36651->proximity_input_dev = input_allocate_device();
	if (!cm36651->proximity_input_dev) {
		pr_err("%s: could not allocate proximity input device\n",
		       __func__);
		goto err_input_allocate_device_proximity;
	}

	input_set_drvdata(cm36651->proximity_input_dev, cm36651);
	cm36651->proximity_input_dev->name = "proximity_sensor";
	input_set_capability(cm36651->proximity_input_dev, EV_ABS,
			     ABS_DISTANCE);
	input_set_abs_params(cm36651->proximity_input_dev, ABS_DISTANCE, 0, 1,
			     0, 0);

	ret = input_register_device(cm36651->proximity_input_dev);
	if (ret < 0) {
		input_free_device(cm36651->proximity_input_dev);
		pr_err("%s: could not register input device\n", __func__);
		goto err_input_register_device_proximity;
	}

	ret = sysfs_create_group(&cm36651->proximity_input_dev->dev.kobj,
				 &proximity_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_proximity;
	}

	/* setup irq */
	ret = cm36651_setup_irq(cm36651);
	if (ret) {
		pr_err("%s: could not setup irq\n", __func__);
		goto err_setup_irq;
	}

	/* For factory test mode, we use timer to get average proximity data. */
	/* prox_timer settings. we poll for light values using a timer. */
	hrtimer_init(&cm36651->prox_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cm36651->prox_poll_delay = ns_to_ktime(2000 * NSEC_PER_MSEC);/*2 sec*/
	cm36651->prox_timer.function = cm36651_prox_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	cm36651->prox_wq = create_singlethread_workqueue("cm36651_prox_wq");
	if (!cm36651->prox_wq) {
		ret = -ENOMEM;
		pr_err("%s: could not create prox workqueue\n", __func__);
		goto err_create_prox_workqueue;
	}
	/* this is the thread function we run on the work queue */
	INIT_WORK(&cm36651->work_prox, cm36651_work_func_prox);

	/* allocate lightsensor input_device */
	cm36651->light_input_dev = input_allocate_device();
	if (!cm36651->light_input_dev) {
		pr_err("%s: could not allocate light input device\n", __func__);
		goto err_input_allocate_device_light;
	}

	input_set_drvdata(cm36651->light_input_dev, cm36651);
	cm36651->light_input_dev->name = "light_sensor";
	input_set_capability(cm36651->light_input_dev, EV_REL, REL_RED);
	input_set_capability(cm36651->light_input_dev, EV_REL, REL_GREEN);
	input_set_capability(cm36651->light_input_dev, EV_REL, REL_BLUE);
	input_set_capability(cm36651->light_input_dev, EV_REL, REL_WHITE);

	ret = input_register_device(cm36651->light_input_dev);
	if (ret < 0) {
		input_free_device(cm36651->light_input_dev);
		pr_err("%s: could not register input device\n", __func__);
		goto err_input_register_device_light;
	}

	ret = sysfs_create_group(&cm36651->light_input_dev->dev.kobj,
				 &light_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}

	/* light_timer settings. we poll for light values using a timer. */
	hrtimer_init(&cm36651->light_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cm36651->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	cm36651->light_timer.function = cm36651_light_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	cm36651->light_wq = create_singlethread_workqueue("cm36651_light_wq");
	if (!cm36651->light_wq) {
		ret = -ENOMEM;
		pr_err("%s: could not create light workqueue\n", __func__);
		goto err_create_light_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&cm36651->work_light, cm36651_work_func_light);

	/* set sysfs for proximity sensor */
	cm36651->proximity_dev = sensors_classdev_register("proximity_sensor");
	if (IS_ERR(cm36651->proximity_dev)) {
		pr_err("%s: could not create proximity_dev\n", __func__);
		goto err_proximity_device_create;
	}

	if (device_create_file(cm36651->proximity_dev, &dev_attr_state) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_state.attr.name);
		goto err_proximity_device_create_file1;
	}

	if (device_create_file(cm36651->proximity_dev, &attr_prox_raw) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       attr_prox_raw.attr.name);
		goto err_proximity_device_create_file7;
	}

#ifdef CM36651_CANCELATION
	if (device_create_file(cm36651->proximity_dev,
		&dev_attr_prox_cal) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_prox_cal.attr.name);
		goto err_proximity_device_create_file2;
	}
#endif
	if (device_create_file(cm36651->proximity_dev,
		&dev_attr_prox_avg) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_prox_avg.attr.name);
		goto err_proximity_device_create_file3;
	}

	if (device_create_file(cm36651->proximity_dev,
		&dev_attr_prox_thresh) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
			   dev_attr_prox_thresh.attr.name);
		goto err_proximity_device_create_file4;
	}

	if (device_create_file(cm36651->proximity_dev,
		&dev_attr_vendor) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
			   dev_attr_vendor.attr.name);
		goto err_proximity_device_create_file5;
	}

	if (device_create_file(cm36651->proximity_dev,
		&dev_attr_name) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
			   dev_attr_name.attr.name);
		goto err_proximity_device_create_file6;
	}

	dev_set_drvdata(cm36651->proximity_dev, cm36651);

	/* set sysfs for light sensor */
	cm36651->light_dev = sensors_classdev_register("light_sensor");
	if (IS_ERR(cm36651->light_dev)) {
		pr_err("%s: could not create light_dev\n", __func__);
		goto err_light_device_create;
	}

	if (device_create_file(cm36651->light_dev, &dev_attr_lux) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_lux.attr.name);
		goto err_light_device_create_file1;
	}

	if (device_create_file(cm36651->light_dev, &dev_attr_raw_data) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_raw_data.attr.name);
		goto err_light_device_create_file2;
	}

	if (device_create_file(cm36651->light_dev, &dev_attr_vendor) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_vendor.attr.name);
		goto err_light_device_create_file3;
	}

	if (device_create_file(cm36651->light_dev, &dev_attr_name) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		       dev_attr_name.attr.name);
		goto err_light_device_create_file4;
	}

#ifdef CONFIG_SLP
	device_init_wakeup(cm36651->proximity_dev, true);
#endif

	dev_set_drvdata(cm36651->light_dev, cm36651);

	pr_info("%s is success.\n", __func__);
	goto done;

/* error, unwind it all */
err_light_device_create_file4:
	device_remove_file(cm36651->light_dev, &dev_attr_vendor);
err_light_device_create_file3:
	device_remove_file(cm36651->light_dev, &dev_attr_raw_data);
err_light_device_create_file2:
	device_remove_file(cm36651->light_dev, &dev_attr_lux);
err_light_device_create_file1:
	sensors_classdev_unregister(cm36651->light_dev);
err_light_device_create:
	device_remove_file(cm36651->proximity_dev, &dev_attr_name);
err_proximity_device_create_file5:
	device_remove_file(cm36651->proximity_dev, &dev_attr_vendor);
err_proximity_device_create_file6:
	device_remove_file(cm36651->proximity_dev, &dev_attr_prox_thresh);
err_proximity_device_create_file4:
	device_remove_file(cm36651->proximity_dev, &dev_attr_prox_avg);
err_proximity_device_create_file3:
#ifdef CM36651_CANCELATION
	device_remove_file(cm36651->proximity_dev, &dev_attr_prox_cal);
err_proximity_device_create_file2:
#endif
	device_remove_file(cm36651->proximity_dev, &attr_prox_raw);
err_proximity_device_create_file7:
	device_remove_file(cm36651->proximity_dev, &dev_attr_state);
err_proximity_device_create_file1:
	sensors_classdev_unregister(cm36651->proximity_dev);
err_proximity_device_create:
	destroy_workqueue(cm36651->light_wq);
err_create_light_workqueue:
	sysfs_remove_group(&cm36651->light_input_dev->dev.kobj,
			   &light_attribute_group);
err_sysfs_create_group_light:
	input_unregister_device(cm36651->light_input_dev);
err_input_register_device_light:
err_input_allocate_device_light:
	destroy_workqueue(cm36651->prox_wq);
err_create_prox_workqueue:
	free_irq(cm36651->irq, cm36651);
	gpio_free(cm36651->pdata->irq);
err_setup_irq:
	sysfs_remove_group(&cm36651->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
err_sysfs_create_group_proximity:
	input_unregister_device(cm36651->proximity_input_dev);
err_input_register_device_proximity:
err_input_allocate_device_proximity:
err_setup_reg:
	wake_lock_destroy(&cm36651->prx_wake_lock);
	mutex_destroy(&cm36651->read_lock);
	mutex_destroy(&cm36651->power_lock);
	kfree(cm36651);
done:
	return ret;
}

static int cm36651_i2c_remove(struct i2c_client *client)
{
	struct cm36651_data *cm36651 = i2c_get_clientdata(client);

	/* free irq */
	if (cm36651->power_state & PROXIMITY_ENABLED) {
		disable_irq_wake(cm36651->irq);
		disable_irq(cm36651->irq);
	}
	free_irq(cm36651->irq, cm36651);
	gpio_free(cm36651->pdata->irq);

	/* device off */
	if (cm36651->power_state & LIGHT_ENABLED)
		cm36651_light_disable(cm36651);
	if (cm36651->power_state & PROXIMITY_ENABLED) {
		cm36651_i2c_write_byte(cm36651, CM36651_PS, PS_CONF1,
					   0x01);
		if (cm36651->pdata->cm36651_led_on)
			cm36651->pdata->cm36651_led_on(false);
	}

	/* destroy workqueue */
	destroy_workqueue(cm36651->light_wq);
	destroy_workqueue(cm36651->prox_wq);

#ifdef CONFIG_SLP
	device_init_wakeup(cm36651->proximity_dev, false);
#endif

	/* sysfs destroy */
	device_remove_file(cm36651->light_dev, &dev_attr_name);
	device_remove_file(cm36651->light_dev, &dev_attr_vendor);
	device_remove_file(cm36651->light_dev, &dev_attr_raw_data);
	device_remove_file(cm36651->light_dev, &dev_attr_lux);
	sensors_classdev_unregister(cm36651->light_dev);

	device_remove_file(cm36651->proximity_dev, &dev_attr_name);
	device_remove_file(cm36651->proximity_dev, &dev_attr_vendor);
	device_remove_file(cm36651->proximity_dev, &dev_attr_prox_thresh);
	device_remove_file(cm36651->proximity_dev, &dev_attr_prox_avg);
#ifdef CM36651_CANCELATION
	device_remove_file(cm36651->proximity_dev, &dev_attr_prox_cal);
#endif
	device_remove_file(cm36651->proximity_dev, &attr_prox_raw);
	device_remove_file(cm36651->proximity_dev, &dev_attr_state);
	sensors_classdev_unregister(cm36651->proximity_dev);

	/* input device destroy */
	sysfs_remove_group(&cm36651->light_input_dev->dev.kobj,
			   &light_attribute_group);
	input_unregister_device(cm36651->light_input_dev);
	sysfs_remove_group(&cm36651->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
	input_unregister_device(cm36651->proximity_input_dev);

	/* lock destroy */
	mutex_destroy(&cm36651->read_lock);
	mutex_destroy(&cm36651->power_lock);
	wake_lock_destroy(&cm36651->prx_wake_lock);

	kfree(cm36651);

	return 0;
}

static int cm36651_suspend(struct device *dev)
{
	/* We disable power only if proximity is disabled.  If proximity
	   is enabled, we leave power on because proximity is allowed
	   to wake up device.  We remove power without changing
	   cm36651->power_state because we use that state in resume.
	 */
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);

	if (cm36651->power_state & LIGHT_ENABLED)
		cm36651_light_disable(cm36651);

	return 0;
}

static int cm36651_resume(struct device *dev)
{
	struct cm36651_data *cm36651 = dev_get_drvdata(dev);

	if (cm36651->power_state & LIGHT_ENABLED)
		cm36651_light_enable(cm36651);

	return 0;
}

static const struct i2c_device_id cm36651_device_id[] = {
	{"cm36651", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, cm36651_device_id);

static const struct dev_pm_ops cm36651_pm_ops = {
	.suspend = cm36651_suspend,
	.resume = cm36651_resume
};

static struct i2c_driver cm36651_i2c_driver = {
	.driver = {
		   .name = "cm36651",
		   .owner = THIS_MODULE,
		   .pm = &cm36651_pm_ops},
	.probe = cm36651_i2c_probe,
	.remove = cm36651_i2c_remove,
	.id_table = cm36651_device_id,
};

static int __init cm36651_init(void)
{
	return i2c_add_driver(&cm36651_i2c_driver);
}

static void __exit cm36651_exit(void)
{
	i2c_del_driver(&cm36651_i2c_driver);
}

module_init(cm36651_init);
module_exit(cm36651_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("RGB Sensor device driver for cm36651");
MODULE_LICENSE("GPL");
