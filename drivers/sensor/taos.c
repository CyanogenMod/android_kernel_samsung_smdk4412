/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* file taos.c
   brief Optical Sensor(proximity sensor) driver for TAOSP002S00F
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/uaccess.h>
#include <linux/unistd.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <mach/gpio.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/wakelock.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/sensor/taos.h>

#define TAOS_DEBUG 1
#define IRQ_WAKE 1

/* Triton register offsets */
#define CNTRL				0x00
#define ALS_TIME			0X01
#define PRX_TIME			0x02
#define WAIT_TIME			0x03
#define ALS_MINTHRESHLO			0X04
#define ALS_MINTHRESHHI			0X05
#define ALS_MAXTHRESHLO			0X06
#define ALS_MAXTHRESHHI			0X07
#define PRX_MINTHRESHLO			0X08
#define PRX_MINTHRESHHI			0X09
#define PRX_MAXTHRESHLO			0X0A
#define PRX_MAXTHRESHHI			0X0B
#define INTERRUPT			0x0C
#define PRX_CFG				0x0D
#define PRX_COUNT			0x0E
#define GAIN				0x0F
#define REVID				0x11
#define CHIPID				0x12
#define STATUS				0x13
#define ALS_CHAN0LO			0x14
#define ALS_CHAN0HI			0x15
#define ALS_CHAN1LO			0x16
#define ALS_CHAN1HI			0x17
#define PRX_LO				0x18
#define PRX_HI				0x19
#define TEST_STATUS			0x1F

/* Triton cmd reg masks */
#define CMD_REG				0X80
#define CMD_BYTE_RW			0x00
#define CMD_WORD_BLK_RW			0x20
#define CMD_SPL_FN			0x60
#define CMD_PROX_INTCLR			0X05
#define CMD_ALS_INTCLR			0X06
#define CMD_PROXALS_INTCLR		0X07
#define CMD_TST_REG			0X08
#define CMD_USER_REG			0X09

/* Triton cntrl reg masks */
#define CNTL_REG_CLEAR			0x00
#define CNTL_PROX_INT_ENBL		0X20
#define CNTL_ALS_INT_ENBL		0X10
#define CNTL_WAIT_TMR_ENBL		0X08
#define CNTL_PROX_DET_ENBL		0X04
#define CNTL_ADC_ENBL			0x02
#define CNTL_PWRON			0x01
#define CNTL_ALSPON_ENBL		0x03
#define CNTL_INTALSPON_ENBL		0x13
#define CNTL_PROXPON_ENBL		0x0F
#define CNTL_INTPROXPON_ENBL		0x2F

/* Triton status reg masks */
#define STA_ADCVALID			0x01
#define STA_PRXVALID			0x02
#define STA_ADC_PRX_VALID		0x03
#define STA_ADCINTR			0x10
#define STA_PRXINTR			0x20

/* Lux constants */
#define SCALE_MILLILUX			3
#define FILTER_DEPTH			3
#define	GAINFACTOR			9

/* Thresholds */
#define ALS_THRESHOLD_LO_LIMIT		0x0000
#define ALS_THRESHOLD_HI_LIMIT		0xFFFF
#define PROX_THRESHOLD_LO_LIMIT		0x0000
#define PROX_THRESHOLD_HI_LIMIT		0xFFFF

/* Device default configuration */
#define CALIB_TGT_PARAM			300000
#define SCALE_FACTOR_PARAM		1
#define GAIN_TRIM_PARAM			512
#define GAIN_PARAM			1
#define ALS_THRSH_HI_PARAM		0xFFFF
#define ALS_THRSH_LO_PARAM		0
#define PRX_THRSH_HI_PARAM		0x28A /* 700 -> 650 */
#define PRX_THRSH_LO_PARAM		0x208 /* 550 -> 520 */
#define ALS_TIME_PARAM			0xF6	/* 50ms -> 27ms */
#define PRX_ADC_TIME_PARAM		0xFF /* [HSS] Original value : 0XEE */
#define PRX_WAIT_TIME_PARAM		0xFF /* 2.73ms */
#define INTR_FILTER_PARAM		0x63	/* 33 -> 63 */
#define PRX_CONFIG_PARAM		0x00
#define PRX_PULSE_CNT_PARAM		0x0A
#define PRX_GAIN_PARAM			0x28	/* 21 */
#define LIGHT_BUFFER_NUM 5

#define SENSOR_DEFAULT_DELAY		(200)
#define SENSOR_MAX_DELAY			(2000)
#define ABS_STATUS                      (ABS_BRAKE)
#define ABS_WAKE                        (ABS_MISC)
#define ABS_CONTROL_REPORT              (ABS_THROTTLE)

static enum taos_als_fops_status taos_als_status = TAOS_ALS_CLOSED;
static enum taos_prx_fops_status taos_prx_status = TAOS_PRX_CLOSED;
static enum taos_chip_working_status taos_chip_status = TAOS_CHIP_UNKNOWN;

static const int adc_table[4] = {
	15,
	150,
	1450,
	13000,
};

/*  i2c write routine for taos */
static int opt_i2c_write(struct i2c_client *client, u8 reg, u8 *val)
{
	int err;
	struct i2c_msg msg[1];
	unsigned char data[2];

	if (client == NULL)
		return -ENODEV;

	data[0] = reg;
	data[1] = *val;

	msg->addr = client->addr;
	msg->flags = I2C_M_WR;
	msg->len = 2;
	msg->buf = data;

	err = i2c_transfer(client->adapter, msg, 1);

	if (err >= 0)
		return 0;

	pr_err("[TAOS] %s %d i2c_transfer error : reg[%X], err[%d]\n",\
		__func__, __LINE__, reg, err);
	return err;
}

static int taos_get_lux(struct taos_data *taos)
{
	int i = 0;
	int integration_time = 27; /* 49->27 */
	int als_gain = 1;
	int irfactor = 0, irfactor2 = 0;
	int calculated_lux = 0;
	u8 prox_int_thresh[4];

	if (taos_als_status == TAOS_ALS_CLOSED) {
		pr_info("!!! Light sensor had been off return 0\n");
		return 0;
	}

	taos->cleardata = i2c_smbus_read_word_data(taos->client,
				CMD_REG | ALS_CHAN0LO);
	taos->irdata = i2c_smbus_read_word_data(taos->client,
				CMD_REG | ALS_CHAN1LO);

	irfactor = 100 * taos->cleardata - 175 * taos->irdata;
	irfactor2 = 63 * taos->cleardata - 100 * taos->irdata;

	if (irfactor2 > irfactor)
		irfactor = irfactor2;

	if (0 > irfactor)
		irfactor = 0;

	calculated_lux = 20*irfactor / (integration_time * als_gain);

	if (calculated_lux > MAX_LUX)
		calculated_lux = MAX_LUX;

	/* QUICK FIX : Code for saturation case */
	/* Saturation value seems to be affected by setting ALS_TIME,
		xEE (=238) which sets max count to 20480. */
	/* change ATIME value from 50ms to 27ms, so condition 18000 -> 9000 */
	if (taos->cleardata >= 9000 || taos->irdata >= 9000) {
		if (taos->proximity_value == 1) {
			taos->proximity_value = 0;
			prox_int_thresh[0] = (0x0000) & 0xFF;
			prox_int_thresh[1] = (0x0000 >> 8) & 0xFF;
			prox_int_thresh[2] = (PRX_THRSH_HI_PARAM) & 0xFF;
			prox_int_thresh[3] = (PRX_THRSH_HI_PARAM >> 8) & 0xFF;
			for (i = 0; i < 4; i++)
				opt_i2c_write(taos->client,
					(CMD_REG|(PRX_MINTHRESHLO + i)),
					&prox_int_thresh[i]);

			if (USE_INPUT_DEVICE) {
				input_report_abs(taos->proximity_input_dev,
					ABS_DISTANCE, !taos->proximity_value);
				pr_info("[PROXIMITY] [%s] saturation. proximity_value is %d (1: close, 0: far)\n",
					__func__, taos->proximity_value);
				input_sync(taos->proximity_input_dev);
				usleep_range(1000, 2000);
			}
		}
	}

	return calculated_lux;
}

static int taos_chip_on(struct taos_data *taos)
{
	u8 value;
	u8 prox_int_thresh[4];
	int err = 0;
	int i;
	int fail_num = 0;

	pr_info("[TAOS] %s\n", __func__);
	value = CNTL_REG_CLEAR;
	err = opt_i2c_write(taos->client, (CMD_REG|CNTRL), &value);
	if (err < 0) {
		pr_info("[diony] i2c_smbus_write_byte_data to clr ctrl reg failed in ioctl prox_on\n");
		fail_num++;
	}
	value = ALS_TIME_PARAM;
	err = opt_i2c_write(taos->client, (CMD_REG|ALS_TIME), &value);
	if (err < 0) {
		pr_info("[diony] i2c_smbus_write_byte_data to als time reg failed in ioctl prox_on\n");
		fail_num++;
	}
	value = PRX_ADC_TIME_PARAM;
	err = opt_i2c_write(taos->client, (CMD_REG|PRX_TIME), &value);
	if (err < 0) {
		pr_info("[diony] i2c_smbus_write_byte_data to prox time reg failed in ioctl prox_on\n");
		fail_num++;
	}
	value = PRX_WAIT_TIME_PARAM;
	err = opt_i2c_write(taos->client, (CMD_REG|WAIT_TIME), &value);
	if (err < 0) {
		pr_info("[diony] i2c_smbus_write_byte_data to wait time reg failed in ioctl prox_on\n");
		fail_num++;
	}
	value = INTR_FILTER_PARAM;
	err = opt_i2c_write(taos->client, (CMD_REG|INTERRUPT), &value);
	if (err < 0) {
		pr_info("[diony] i2c_smbus_write_byte_data to interrupt reg failed in ioctl prox_on\n");
		fail_num++;
	}
	value = PRX_CONFIG_PARAM;
	err = opt_i2c_write(taos->client, (CMD_REG|PRX_CFG), &value);
	if (err < 0) {
		pr_info("[diony] i2c_smbus_write_byte_data to prox cfg reg failed in ioctl prox_on\n");
		fail_num++;
	}

	if (taos->chipID == 0x39)
		value = 0x08;
	else if (taos->chipID == 0x29)
		value = 0x0A;
	else
		value = 0x08;

	err = opt_i2c_write(taos->client, (CMD_REG|PRX_COUNT), &value);
	if (err < 0) {
		pr_info("[diony] i2c_smbus_write_byte_data to prox cnt reg failed in ioctl prox_on\n");
		fail_num++;
	}
	if (taos->chipID == 0x39)
		value = PRX_GAIN_PARAM; /* 100mA, ch1, pgain 4x, again 1x */
	else if (taos->chipID == 0x29)
		value = 0x20; /* 100mA, ch1, pgain 1x, again 1x */
	else
		value = PRX_GAIN_PARAM;

	err = opt_i2c_write(taos->client, (CMD_REG|GAIN), &value);
	if (err < 0) {
		pr_info("[diony] i2c_smbus_write_byte_data to prox gain reg failed in ioctl prox_on\n");
		fail_num++;
	}
	prox_int_thresh[0] = (0x0000) & 0xFF;
	prox_int_thresh[1] = (0x0000 >> 8) & 0xFF;
	prox_int_thresh[2] = (PRX_THRSH_HI_PARAM) & 0xFF;
	prox_int_thresh[3] = (PRX_THRSH_HI_PARAM >> 8) & 0xFF;
	for (i = 0; i < 4; i++) {
		err = opt_i2c_write(taos->client,
			(CMD_REG|(PRX_MINTHRESHLO+i)), &prox_int_thresh[i]);
		if (err < 0) {
			pr_info("[diony]i2c_smbus_write_byte_data to prox int thrsh regs failed in ioctl prox_on\n");
			fail_num++;
		}
	}
	value = CNTL_INTPROXPON_ENBL;
	err = opt_i2c_write(taos->client, (CMD_REG|CNTRL), &value);
	if (err < 0) {
		pr_info("[diony]i2c_smbus_write_byte_data to ctrl reg "
			"failed in ioctl prox_on\n");
		fail_num++;
	}

	usleep_range(12000, 20000);

	if (fail_num == 0) {
#if IRQ_WAKE
		err = irq_set_irq_wake(taos->irq, 1); /* enable:1, disable:0 */
		pr_info("[TAOS] register wakeup source = %d\n", err);
		if (err)
			pr_err("[TAOS] register wakeup source failed\n");
#endif
		taos_chip_status = TAOS_CHIP_WORKING;
	} else
		pr_err("I2C failed in taos_chip_on, # of fail I2C=[%d]\n",
			fail_num);

	return err;
}

static int taos_chip_off(struct taos_data *taos)
{
	int ret = 0;
	u8 reg_cntrl;
	int err = 0;

	pr_info("[TAOS] %s\n", __func__);

#if IRQ_WAKE
	err = irq_set_irq_wake(taos->irq, 0); /* enable : 1, disable : 0 */
	if (err)
		pr_err("[TAOS] register wakeup source failed\n");
#endif

	reg_cntrl = CNTL_REG_CLEAR;

	ret = opt_i2c_write(taos->client, (CMD_REG | CNTRL), &reg_cntrl);
	if (ret < 0) {
		pr_err("opt_i2c_write failed in taos_chip_off\n");
		return ret;
	}

	taos_chip_status = TAOS_CHIP_SLEEP;

	return ret;
}

/************************************************************
 *
 *  function    : taos_on
 *  description : This function is power-on function for optical sensor.
 *
 *  int type    : Sensor type. Two values is available (PROXIMITY,LIGHT).
 *                it support power-on function separately.
 *
 *
 */

static void taos_on(struct taos_data *taos, int type)
{
	int err = 0;

	pr_info("%s : type=%d (0:light, 1:prox, 2:all)\n",
		__func__, type);

	taos_chip_on(taos);

#if IRQ_WAKE
	err = irq_set_irq_wake(taos->irq, 1);  /* enable : 1, disable : 0 */
	if (err)
		pr_err("[TAOS] register wakeup source failed\n");
#endif
	if (type == TAOS_PROXIMITY || type == TAOS_ALL) {
		pr_info("enable irq for proximity\n");
		enable_irq(taos->irq);
		taos->proximity_enable = 1;
		taos_prx_status = TAOS_PRX_OPENED;
		taos->proximity_value = 0;

		pr_info("[TAOS_PROXIMITY] %s: timer start for prox sensor\n",
			__func__);
		hrtimer_start(&taos->ptimer, taos->prox_polling_time,
			HRTIMER_MODE_REL);
	}
	if (type == TAOS_LIGHT || type == TAOS_ALL) {
		pr_info("[TAOS_LIGHT] %s: timer start for light sensor\n",
			__func__);
		taos->light_polling_time = ktime_set(0, 0);
		taos->light_polling_time =
			ktime_add_us(taos->light_polling_time, 200000);

		hrtimer_start(&taos->timer, taos->light_polling_time,
			HRTIMER_MODE_REL);
		taos->light_enable = 1;
		msleep(50);  /* for sure validation of first polling value */
		taos_als_status = TAOS_ALS_OPENED;
	}
}

/******************************************************************
 *
 *  function    : taos_off
 *  description : This function is power-off function for optical sensor.
 *
 *  int type    : Sensor type. Three values is available (PROXIMITY,LIGHT,ALL).
 *                it support power-on function separately.
 *
 *
 */

static void taos_off(struct taos_data *taos, int type)
{
#ifdef TAOS_DEBUG
	pr_info("%s : type=%d (0:light, 1:prox, 2:all)\n",
		__func__, type);
#endif

	if (type == TAOS_PROXIMITY || type == TAOS_ALL) {
		pr_info("[TAOS] %s: disable irq for proximity\n",
			__func__);
		disable_irq(taos->irq);
		hrtimer_cancel(&taos->ptimer);
		taos->proximity_enable = 0;
		taos_prx_status = TAOS_PRX_CLOSED;
		taos->proximity_value = 0;  /* initialize proximity_value */
	}

	if (type == TAOS_LIGHT || type == TAOS_ALL) {
		pr_info("[TAOS] %s: timer cancel for light sensor\n",
			__func__);
		hrtimer_cancel(&taos->timer);
		taos->light_enable = 0;
		taos_als_status = TAOS_ALS_CLOSED;
	}

	if (taos_prx_status == TAOS_PRX_CLOSED &&
		taos_als_status == TAOS_ALS_CLOSED &&
		taos_chip_status == TAOS_CHIP_WORKING)
		taos_chip_off(taos);
}

/*************************************************************************/
/*		TAOS sysfs					         */
/*************************************************************************/

static void taos_light_enable(struct taos_data *taos)
{
	taos->light_enable = 1;
	taos->light_count = 0;
	taos->light_buffer = 0;
	hrtimer_start(&taos->timer, taos->light_polling_time, HRTIMER_MODE_REL);
}

static void taos_light_disable(struct taos_data *taos)
{
	hrtimer_cancel(&taos->timer);
	cancel_work_sync(&taos->work_light);
	taos->light_enable = 0;
}
static void taos_ptime_disable(struct taos_data *taos)
{
	hrtimer_cancel(&taos->ptimer);
	cancel_work_sync(&taos->work_ptime);
	taos->proximity_enable = 0;
}
static ssize_t poll_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n", ktime_to_ns(taos->light_polling_time));
}

static ssize_t poll_delay_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;
#if TAOS_DEBUG
	pr_info("[TAOS] %s: new delay = %lldns, old delay = %lldns\n",\
		__func__, new_delay, ktime_to_ns(taos->light_polling_time));
#endif
	mutex_lock(&taos->power_lock);
	if (new_delay != ktime_to_ns(taos->light_polling_time)) {
		taos->light_polling_time = ns_to_ktime(new_delay);
		if (taos->light_enable) {
			taos_light_disable(taos);
			taos_light_enable(taos);
		}
	}
	mutex_unlock(&taos->power_lock);

	return size;
}

static ssize_t light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", (taos->light_enable) ? 1 : 0);
}

static ssize_t proximity_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", (taos->proximity_enable) ? 1 : 0);
}

static ssize_t light_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);
#if TAOS_DEBUG
	pr_info("[TAOS] %s: input value = %d\n", __func__, value);
#endif

	if (value == 1 && taos->light_enable == OFF) {
		taos_on(taos, TAOS_LIGHT);
		value = ON;
		pr_info("[TAOS] *#0589# test start : [%d]\n", value);
	} else if (value == 0 && taos->light_enable == ON) {
		taos_off(taos, TAOS_LIGHT);

		value = OFF;
		pr_info("[TAOS] *#0589# test end : [%d]\n", value);
	}

	return size;
}

static ssize_t proximity_enable_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);
#if TAOS_DEBUG
	pr_info("[TAOS] %s: input value = %d\n", __func__, value);
#endif
	if (value == 1 && taos->proximity_enable == OFF) {
		/* reset Interrupt pin */
		/* to active Interrupt, TMD2771x IR. pin shoud be reset. */
		i2c_smbus_write_byte(taos->client,
			(CMD_REG|CMD_SPL_FN|CMD_PROXALS_INTCLR));

		taos_on(taos, TAOS_PROXIMITY);
		input_report_abs(taos->proximity_input_dev,
			ABS_DISTANCE, !taos->proximity_value);
		input_sync(taos->proximity_input_dev);
		pr_info("[TAOS] Temp : Power ON, chipID=%X\n", taos->chipID);
	} else if (value == 0 && taos->proximity_enable == ON) {
		taos_off(taos, TAOS_PROXIMITY);

		pr_info("[TAOS_PROXIMITY] Temporary : Power OFF\n");
	}

	return size;
}

static ssize_t lightsensor_file_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	int adc = 0;

	if (taos->light_enable) {
		adc = taos_get_lux(taos);
		return sprintf(buf, "%d\n", adc);
	} else {
		return sprintf(buf, "%d\n", adc);
	}
}

static ssize_t proximity_delay_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	int delay;

	delay = taos->delay;
	return sprintf(buf, "%d\n", delay);
}

static ssize_t proximity_delay_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct taos_data *taos = input_get_drvdata(input_data);
	int delay = 0; /*simple_strtoul(buf, NULL, 10);*/
	int err = 0;

	err = kstrtouint(buf, 10, &delay);
	if (err)
		pr_err("%s, kstrtouint failed.(err:%d)",
			__func__, err);

	pr_info("delay = %d\n", delay);
	if (delay < 0)
		return size;

	if (SENSOR_MAX_DELAY < delay)
		delay = SENSOR_MAX_DELAY;

	taos->delay = delay;

	input_report_abs(input_data, ABS_CONTROL_REPORT,
		taos->proximity_enable<<16 | delay);

	return size;
}

static ssize_t proximity_wake_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	static int cnt = 1;

	input_report_abs(input_data, ABS_WAKE, cnt++);

	return count;
}

static ssize_t proximity_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", taos->proximity_value);
}

static ssize_t proximity_adc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	u16 proximity_value = 0;

	proximity_value = i2c_smbus_read_word_data(taos->client,
						CMD_REG | PRX_LO);
	if (proximity_value > TAOS_PROX_MAX)
		proximity_value = TAOS_PROX_MAX;
	return sprintf(buf, "%d\n", proximity_value);
}

static ssize_t proximity_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);

	return sprintf(buf, "%d,%d,%d\n",
		taos->avg[0], taos->avg[1], taos->avg[2]);
}

static ssize_t proximity_avg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	return proximity_enable_store(dev, attr, buf, size);
}

static struct device_attribute dev_attr_light_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	       light_enable_show, light_enable_store);
static struct device_attribute dev_attr_poll_delay =
	__ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
	       poll_delay_show, poll_delay_store);
static struct device_attribute dev_attr_light_adc =
	__ATTR(light_adc, S_IRUGO | S_IWUSR | S_IWGRP,
	       lightsensor_file_state_show, NULL);

static struct device_attribute dev_attr_proximity_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	       proximity_enable_show, proximity_enable_store);
static struct device_attribute dev_attr_proximity_delay =
	__ATTR(delay, S_IRUGO | S_IWUSR | S_IWGRP,
	       proximity_delay_show, proximity_delay_store);
static struct device_attribute dev_attr_proximity_wake =
	__ATTR(wake, S_IRUGO | S_IWUSR | S_IWGRP,
	       NULL, proximity_wake_store);
static struct device_attribute dev_attr_proximity_data =
	__ATTR(data, S_IRUGO | S_IWUSR | S_IWGRP,
	       proximity_data_show, NULL);
static struct device_attribute dev_attr_proximity_state =
	__ATTR(proximity_state, S_IRUGO | S_IWUSR | S_IWGRP,
	       proximity_adc_show, NULL);
static struct device_attribute dev_attr_proximity_avg =
	__ATTR(proximity_avg, S_IRUGO | S_IWUSR | S_IWGRP,
	       proximity_avg_show, proximity_avg_store);

static struct attribute *light_sysfs_attrs[] = {
	&dev_attr_light_enable.attr,
	&dev_attr_poll_delay.attr,
	&dev_attr_light_adc.attr,
	NULL
};

static struct attribute_group light_attribute_group = {
	.attrs = light_sysfs_attrs,
};

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_proximity_enable.attr,
	&dev_attr_proximity_delay.attr,
	&dev_attr_proximity_wake.attr,
	&dev_attr_proximity_data.attr,
	&dev_attr_proximity_state.attr,
	&dev_attr_proximity_avg.attr,
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};


/****************************************************************
 *
 *  function    : taos_work_func_prox
 *  description : This function is for proximity sensor (using B-1 Mode).
 *                when INT signal is occured , it gets value from VO register.
 *
 *
 */
#if USE_INTERRUPT
static void taos_work_func_prox(struct work_struct *work)
{
	struct taos_data *taos
		= container_of(work, struct taos_data, work_prox);
	u16 adc_data;
	u16 threshold_high;
	u16 threshold_low;
	u8 prox_int_thresh[4];
	int i;

	/* change Threshold */
	adc_data = i2c_smbus_read_word_data(taos->client, CMD_REG | 0x18);
	threshold_high = i2c_smbus_read_word_data(taos->client,
		(CMD_REG | PRX_MAXTHRESHLO));
	threshold_low = i2c_smbus_read_word_data(taos->client,
		(CMD_REG | PRX_MINTHRESHLO));
	pr_info("[TAOS] %s %d, high(%x), low(%x)\n",
		__func__, adc_data, threshold_high, threshold_low);

	/* this is protection code for saturation */
	if (taos_get_lux(taos) >= 1500) {

#if TAOS_DEBUG
		pr_info("[PROXIMITY] [%s] saturation. adc_data=[%X], threshold_high=[%X],  threshold_min=[%X]\n",
			__func__, adc_data, threshold_high, threshold_low);
#endif
		taos->proximity_value = 0;
		prox_int_thresh[0] = (0x0000) & 0xFF;
		prox_int_thresh[1] = (0x0000 >> 8) & 0xFF;
		prox_int_thresh[2] = (PRX_THRSH_HI_PARAM) & 0xFF;
		prox_int_thresh[3] = (PRX_THRSH_HI_PARAM >> 8) & 0xFF;
		for (i = 0; i < 4; i++)
			opt_i2c_write(taos->client,
			(CMD_REG|(PRX_MINTHRESHLO + i)),
			&prox_int_thresh[i]);
	} /* end of protection code for saturation */
	else if ((threshold_high == (PRX_THRSH_HI_PARAM))
		&& (adc_data >= (PRX_THRSH_HI_PARAM))) {
#if TAOS_DEBUG
		pr_info("[PROXIMITY] [%s] +++ adc_data=[%X], threshold_high=[%X],  threshold_min=[%X]\n",
			__func__, adc_data, threshold_high, threshold_low);
#endif
		taos->proximity_value = 1;
		prox_int_thresh[0] = (PRX_THRSH_LO_PARAM) & 0xFF;
		prox_int_thresh[1] = (PRX_THRSH_LO_PARAM >> 8) & 0xFF;
		prox_int_thresh[2] = (0xFFFF) & 0xFF;
		prox_int_thresh[3] = (0xFFFF >> 8) & 0xFF;
		for (i = 0; i < 4; i++)
			opt_i2c_write(taos->client,
				(CMD_REG|(PRX_MINTHRESHLO + i)),
				&prox_int_thresh[i]);
	} else if ((threshold_high ==  (0xFFFF))
		&& (adc_data <= (PRX_THRSH_LO_PARAM))) {
#if TAOS_DEBUG
		pr_info("[PROXIMITY] [%s] --- adc_data=[%X], threshold_high=[%X],  threshold_min=[%X]\n",
			__func__, adc_data, threshold_high, threshold_low);
#endif
		taos->proximity_value = 0;
		prox_int_thresh[0] = (0x0000) & 0xFF;
		prox_int_thresh[1] = (0x0000 >> 8) & 0xFF;
		prox_int_thresh[2] = (PRX_THRSH_HI_PARAM) & 0xFF;
		prox_int_thresh[3] = (PRX_THRSH_HI_PARAM >> 8) & 0xFF;
		for (i = 0; i < 4; i++)
			opt_i2c_write(taos->client,
				(CMD_REG|(PRX_MINTHRESHLO + i)),
				&prox_int_thresh[i]);
	} else {
		pr_err("[PROXIMITY] [%s] Not Common!adc_data=[%X], threshold_high=[%X],  threshold_min=[%X]\n",
			__func__, adc_data, threshold_high, threshold_low);
	}

	if (USE_INPUT_DEVICE) {
		input_report_abs(taos->proximity_input_dev,
			ABS_DISTANCE, !taos->proximity_value);
		pr_info("[PROXIMITY] [%s] proximity_value is %d (1: close, 0: far)\n",
			__func__, taos->proximity_value);
		input_sync(taos->proximity_input_dev);
		usleep_range(1000, 2000);
	}

	/* reset Interrupt pin */
	/* to active Interrupt, TMD2771x Interuupt pin shoud be reset. */
	i2c_smbus_write_byte(taos->client,
		(CMD_REG|CMD_SPL_FN|CMD_PROXALS_INTCLR));

	/* enable INT */
	enable_irq(taos->irq);
}


static irqreturn_t taos_irq_handler(int irq, void *dev_id)
{
	struct taos_data *taos = dev_id;

#ifdef TAOS_DEBUG
	pr_info("[PROXIMITY] taos->irq = %d\n", taos->irq);
#endif

	if (taos->irq != -1) {
		wake_lock_timeout(&taos->prx_wake_lock, 3*HZ);
		disable_irq_nosync(taos->irq);
		queue_work(taos->taos_wq, &taos->work_prox);
	}

	return IRQ_HANDLED;
}

static int taos_setup_irq(struct taos_data *taos)
{
	int rc = -EIO;
	struct taos_platform_data *pdata = taos->client->dev.platform_data;
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
				taos_irq_handler,
				IRQF_DISABLED|IRQ_TYPE_EDGE_FALLING,
				"taos_int", taos);
	if (rc < 0) {
		pr_err("%s: request_irq(%d) failed for gpio %d (%d)\n",
		       __func__, irq, pdata->p_out, rc);
		goto err_request_irq;
	}

	rc = irq_set_irq_wake(irq, 1);
	if (rc < 0)
		pr_err("[TAOS] irq_set_irq_wake failed (%d)\n", rc);

	/* start with interrupts disabled */
	taos->irq = irq;

	pr_err("%s, success\n", __func__);

	goto done;

err_request_irq:
err_gpio_direction_input:
	gpio_free(pdata->p_out);
done:
	return rc;
}

static void taos_work_func_light(struct work_struct *work)
{
	struct taos_data *taos
		= container_of(work, struct taos_data, work_light);
	int i = 0;
	int lux = 0;
/*	state_type level_state = LIGHT_INIT; */

	/* read value */
	lux =  taos_get_lux(taos);

	for (i = 0 ; ARRAY_SIZE(adc_table) ; i++)
		if (lux <= adc_table[i])
			break;

	if (taos->light_buffer == i) {
		if (taos->light_count++ == LIGHT_BUFFER_NUM) {
#if TAOS_DEBUG
			pr_info("[TAOS_LIGHT] taos_work_func_light called lux=[%d], ch[0]=[%d], ch[1]=[%d]\n",
				lux, taos->cleardata, taos->irdata);
#endif
			input_report_abs(taos->light_input_dev, ABS_MISC, lux);
			input_sync(taos->light_input_dev);
			taos->light_count = 0;
		}
	} else {
		taos->light_buffer = i;
		taos->light_count = 0;
	}
}

static enum hrtimer_restart taos_timer_func(struct hrtimer *timer)
{
	struct taos_data *taos = container_of(timer, struct taos_data, timer);

	queue_work(taos->taos_wq, &taos->work_light);
	taos->light_polling_time = ktime_set(0, 0);
	taos->light_polling_time
		= ktime_add_us(taos->light_polling_time, 200000);
	hrtimer_start(&taos->timer, taos->light_polling_time, HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;

}


static void taos_work_func_ptime(struct work_struct *work)
{
	struct taos_data *taos
		= container_of(work, struct taos_data, work_ptime);
	u16 proximity_value = 0;
	int min = 0, max = 0, avg = 0;
	int i = 0;

	for (i = 0 ; i < PROX_READ_NUM ; i++) {
		proximity_value = i2c_smbus_read_word_data(taos->client,
						CMD_REG | PRX_LO);
		if (proximity_value > TAOS_PROX_MIN) {
			if (proximity_value > TAOS_PROX_MAX)
				proximity_value = TAOS_PROX_MAX;
			avg += proximity_value;
			if (!i)
				min = proximity_value;
			if (proximity_value < min)
				min = proximity_value;
			if (proximity_value > max)
				max = proximity_value;
		} else {
			proximity_value = TAOS_PROX_MIN;
			break;
		}
		msleep(40);
	}

	if (i == 0) {
		avg = proximity_value;
		min = max = TAOS_PROX_MIN;
	} else
		avg /= i;

	taos->avg[0] = min;
	taos->avg[1] = avg;
	taos->avg[2] = max;
}

static enum hrtimer_restart taos_ptimer_func(struct hrtimer *timer)
{
	struct taos_data *taos = container_of(timer, struct taos_data, ptimer);

	queue_work(taos->taos_test_wq, &taos->work_ptime);
	hrtimer_forward_now(&taos->ptimer, taos->prox_polling_time);

	return HRTIMER_RESTART;
}

#endif

/*************************************************************************/
/*		TAOS file operations				         */
/*************************************************************************/

static const struct file_operations light_fops = {
	.owner = THIS_MODULE,
};

static struct miscdevice light_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "light",
	.fops = &light_fops,
};

static int taos_opt_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int err = 0;
	struct input_dev *input_dev;
	struct taos_data *taos;

	pr_info("[TAOS] taos_opt_probe\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[TAOS] i2c_check_functionality error\n");
		err = -ENODEV;
		goto exit;
	}
	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA)) {
		pr_err("[TAOS] byte op is not permited.\n");
		goto exit;
	}

	/* OK. For now, we presume we have a valid client. We now create the
	client structure, even though we cannot fill it completely yet. */
	taos = kzalloc(sizeof(struct taos_data), GFP_KERNEL);
	if (!taos) {
		err = -ENOMEM;
		goto exit;
	}
	memset(taos, 0, sizeof(struct taos_data));
	taos->client = client;
	i2c_set_clientdata(client, taos);

	taos->irdata = 0;		/*Ch[1] */
	taos->cleardata = 0;	/*Ch[0] */
	taos->chipID = 0;

	usleep_range(12000, 20000);

	/* wake lock init */
	wake_lock_init(&taos->prx_wake_lock,
		WAKE_LOCK_SUSPEND, "prx_wake_lock");
	mutex_init(&taos->power_lock);

	/* allocate proximity input_device */
	if (USE_INPUT_DEVICE) {
		input_dev = input_allocate_device();
		if (input_dev == NULL) {
			pr_err("Failed to allocate input device\n");
			err = -ENOMEM;
			goto err_input_allocate_device_proximity;
		}
		taos->proximity_input_dev = input_dev;
		input_set_drvdata(input_dev, taos);
		input_dev->name = "proximity_sensor";
		input_set_capability(input_dev, EV_ABS, ABS_DISTANCE);
		input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);

		err = input_register_device(input_dev);
		if (err) {
			pr_err("Unable to register %s input device\n",
				input_dev->name);
			goto err_input_register_device_proximity;
		}

		err = sysfs_create_group(&input_dev->dev.kobj,
				 &proximity_attribute_group);
		if (err) {
			pr_err("%s: could not create sysfs group\n", __func__);
			goto err_sysfs_create_group_proximity;
		}
	}

#if USE_INTERRUPT
	/* WORK QUEUE Settings */
	taos->taos_wq = create_singlethread_workqueue("taos_wq");
	if (!taos->taos_wq) {
		err = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}
	INIT_WORK(&taos->work_prox, taos_work_func_prox);
	INIT_WORK(&taos->work_light, taos_work_func_light);

	taos->taos_test_wq = create_singlethread_workqueue("taos_test_wq");
	if (!taos->taos_test_wq) {
		err = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}
	INIT_WORK(&taos->work_ptime, taos_work_func_ptime);
#endif

	/* hrtimer settings.  we poll for light values using a timer. */
	hrtimer_init(&taos->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	taos->timer.function = taos_timer_func;

	hrtimer_init(&taos->ptimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	taos->prox_polling_time = ns_to_ktime(2000 * NSEC_PER_MSEC);
	taos->ptimer.function = taos_ptimer_func;

	/* allocate lightsensor-level input_device */
	if (USE_INPUT_DEVICE) {
		input_dev = input_allocate_device();
		if (!input_dev) {
			pr_err("%s: could not allocate input device\n",
				__func__);
			err = -ENOMEM;
			goto err_input_allocate_device_light;
		}
		input_set_drvdata(input_dev, taos);
		input_dev->name = "light_sensor";
		input_set_capability(input_dev, EV_ABS, ABS_MISC);
		input_set_abs_params(input_dev, ABS_MISC, 0, 1, 0, 0);

		err = input_register_device(input_dev);
		if (err) {
			pr_err("Unable to register %s input device\n",
				input_dev->name);
			goto err_input_register_device_light;
		}
		taos->light_input_dev = input_dev;
		err = sysfs_create_group(&input_dev->dev.kobj,
				 &light_attribute_group);
		if (err) {
			pr_err("%s: could not create sysfs group\n", __func__);
			goto err_sysfs_create_group_light;
		}
	}

	/* misc device Settings */
	err = misc_register(&light_device);
	if (err)
		pr_err(KERN_ERR "[TAOS] %s: misc_register failed - light\n",
			__func__);

	/* set sysfs for proximity sensor */
	taos->proximity_class = class_create(THIS_MODULE, "proximity");
	if (IS_ERR(taos->proximity_class))
		pr_err("%s: could not create proximity_class\n", __func__);

	taos->proximity_dev = device_create(taos->proximity_class,
		NULL, 0, NULL, "proximity");
	if (IS_ERR(taos->proximity_dev))
		pr_err("%s: could not create proximity_dev\n", __func__);

	if (device_create_file(taos->proximity_dev,
		&dev_attr_proximity_state) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		dev_attr_proximity_state.attr.name);
	}

	if (device_create_file(taos->proximity_dev,
		&dev_attr_proximity_avg) < 0) {
		pr_err("%s: could not create device file(%s)!\n", __func__,
		dev_attr_proximity_avg.attr.name);
	}
	dev_set_drvdata(taos->proximity_dev, taos);

	/* set sysfs for light sensor */
	taos->lightsensor_class = class_create(THIS_MODULE, "lightsensor");
	if (IS_ERR(taos->lightsensor_class))
		pr_err("Failed to create class(lightsensor)!\n");

	taos->switch_cmd_dev = device_create(taos->lightsensor_class,
		NULL, 0, NULL, "switch_cmd");
	if (IS_ERR(taos->switch_cmd_dev))
		pr_err("Failed to create device(switch_cmd_dev)!\n");

	if (device_create_file(taos->switch_cmd_dev,
		&dev_attr_light_adc) < 0)
		pr_err("Failed to create device file(%s)!\n",
		dev_attr_light_adc.attr.name);

	dev_set_drvdata(taos->switch_cmd_dev, taos);
	usleep_range(2000, 3000);

	taos_setup_irq(taos);

	taos->chipID = i2c_smbus_read_byte_data(client, CMD_REG | CHIPID);
	pr_info("[TAOS] %s: chipID[%X]\n", __func__, taos->chipID);

	/* maintain power-down mode before using sensor */
	taos_off(taos, TAOS_ALL);
	pr_info("%s: done!!\n", __func__);

	return 0;

err_sysfs_create_group_light:
	input_unregister_device(taos->light_input_dev);
err_input_register_device_light:
err_input_allocate_device_light:
	destroy_workqueue(taos->taos_wq);
	destroy_workqueue(taos->taos_test_wq);
err_create_workqueue:
	sysfs_remove_group(&taos->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
err_sysfs_create_group_proximity:
	input_unregister_device(taos->proximity_input_dev);
err_input_register_device_proximity:
err_input_allocate_device_proximity:
	mutex_destroy(&taos->power_lock);
	wake_lock_destroy(&taos->prx_wake_lock);
	kfree(taos);
	taos = NULL;
exit:
	taos = NULL;
	return err;
}


static int taos_opt_remove(struct i2c_client *client)
{
	struct taos_data *taos = i2c_get_clientdata(client);
#ifdef TAOS_DEBUG
	pr_info("%s\n", __func__);
#endif
	if (USE_INPUT_DEVICE) {
		sysfs_remove_group(&taos->light_input_dev->dev.kobj,
			&light_attribute_group);
		input_unregister_device(taos->light_input_dev);
		sysfs_remove_group(&taos->proximity_input_dev->dev.kobj,
			&proximity_attribute_group);
		input_unregister_device(taos->proximity_input_dev);
	}
	if (taos->light_enable)
		taos_light_disable(taos);
	if (taos->proximity_enable) {
		taos_ptime_disable(taos);
		disable_irq(taos->irq);
	}
	if (taos->taos_wq)
		destroy_workqueue(taos->taos_wq);
	if (taos->taos_test_wq)
		destroy_workqueue(taos->taos_test_wq);
	mutex_destroy(&taos->power_lock);
	wake_lock_destroy(&taos->prx_wake_lock);
	kfree(taos);
	return 0;
}

static void taos_opt_shutdown(struct i2c_client *client)
{
	struct taos_data *taos = i2c_get_clientdata(client);
#ifdef TAOS_DEBUG
	pr_info("%s\n", __func__);
#endif
	if (USE_INPUT_DEVICE) {
		sysfs_remove_group(&taos->light_input_dev->dev.kobj,
			&light_attribute_group);
		input_unregister_device(taos->light_input_dev);
		sysfs_remove_group(&taos->proximity_input_dev->dev.kobj,
			&proximity_attribute_group);
		input_unregister_device(taos->proximity_input_dev);
	}
	if (taos->light_enable)
		taos_light_disable(taos);
	if (taos->proximity_enable) {
		taos_ptime_disable(taos);
		disable_irq(taos->irq);
	}
	if (taos->taos_wq)
		destroy_workqueue(taos->taos_wq);
	if (taos->taos_test_wq)
		destroy_workqueue(taos->taos_test_wq);
	mutex_destroy(&taos->power_lock);
	wake_lock_destroy(&taos->prx_wake_lock);
	kfree(taos);
}
#ifdef CONFIG_PM
static int taos_opt_suspend(struct device *dev)
{
	/* We disable power only if proximity is disabled.  If proximity
	 * is enabled, we leave power on because proximity is allowed
	 * to wake up device.  We remove power without changing
	 * gp2a->power_state because we use that state in resume
	*/
#ifdef TAOS_DEBUG
	pr_info("[%s] TAOS !!suspend mode!!\n", __func__);
#endif

	return 0;
}

static int taos_opt_resume(struct device *dev)
{
#ifdef TAOS_DEBUG
	pr_info("[%s] TAOS !!resume mode!!\n", __func__);
#endif

	return 0;
}
#else
#define taos_opt_suspend NULL
#define taos_opt_resume NULL
#endif

static const struct i2c_device_id taos_id[] = {
	{ "taos", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, taos_id);

static const struct dev_pm_ops taos_pm_ops = {
	.suspend	= taos_opt_suspend,
	.resume		= taos_opt_resume,
};

static struct i2c_driver taos_opt_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "taos",
		.pm = &taos_pm_ops
	},
	.probe		= taos_opt_probe,
	.remove		= taos_opt_remove,
	.shutdown	= taos_opt_shutdown,
	.id_table	= taos_id,
};

static int __init taos_opt_init(void)
{

#ifdef TAOS_DEBUG
	pr_info("%s\n", __func__);
#endif
	return i2c_add_driver(&taos_opt_driver);
}

static void __exit taos_opt_exit(void)
{
	i2c_del_driver(&taos_opt_driver);
#ifdef TAOS_DEBUG
	pr_info("%s\n", __func__);
#endif
}

module_init(taos_opt_init);
module_exit(taos_opt_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("Optical Sensor driver for taosp002s00f");
MODULE_LICENSE("GPL");
