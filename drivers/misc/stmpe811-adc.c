/*
 * stmpe811-adc.c
 *
 * Copyright (C) 2011 Samsung Electronics
 * SangYoung Son <hello.son@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <mach/midas-thermistor.h>
#include <linux/stmpe811-adc.h>
#include <plat/gpio-cfg.h>

#define STMPE811_CHIP_ID	0x00
#define STMPE811_ID_VER		0x02
#define STMPE811_SYS_CTRL1	0x03
#define STMPE811_SYS_CTRL2	0x04
#define STMPE811_INT_CTRL	0x09
#define STMPE811_INT_EN		0x0A
#define STMPE811_INT_STA	0x0B
#define STMPE811_ADC_INT_EN	0x0E
#define STMPE811_ADC_INT_STA	0x0F
#define STMPE811_ADC_CTRL1	0x20
#define STMPE811_ADC_CTRL2	0x21
#define STMPE811_ADC_CAPT	0x22
#define STMPE811_ADC_DATA_CH0	0x30
#define STMPE811_ADC_DATA_CH1	0x32
#define STMPE811_ADC_DATA_CH2	0x34
#define STMPE811_ADC_DATA_CH3	0x36
#define STMPE811_ADC_DATA_CH4	0x38
#define STMPE811_ADC_DATA_CH5	0x3A
#define STMPE811_ADC_DATA_CH6	0x3C
#define STMPE811_ADC_DATA_CH7	0x3E
#define STMPE811_GPIO_AF	0x17
#define STMPE811_TSC_CTRL	0x40

static struct device *adc_dev;
static struct i2c_client *stmpe811_adc_i2c_client;

struct stmpe811_adc_data {
	struct i2c_client	*client;
	struct stmpe811_platform_data	*pdata;

	struct mutex		adc_lock;
};

static int stmpe811_i2c_read(struct i2c_client *client,
							u8 reg,
							u8 *data,
							u8 length)
{
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client, (u8)reg, length, data);
	if (ret < 0) {
		pr_err("%s: err %d, reg: 0x%02x\n", __func__, ret, reg);
		return ret;
	}

	return 0;
}

int stmpe811_write_register(struct i2c_client *client,
							u8 reg,
							u16 w_data)
{
	int ret;

	ret = i2c_smbus_write_word_data(client, (u8)reg, w_data);
	if (ret < 0) {
		pr_err("%s: err %d, reg: 0x%02x\n", __func__, ret, reg);
		return ret;
	}

	return 0;
}

u16 stmpe811_get_adc_data(u8 ch)
{
	struct i2c_client *client = stmpe811_adc_i2c_client;
	u8 data[2];
	u16 w_data;
	int prog, retry_cnt;
	int data_channel_addr;

	stmpe811_write_register(client, STMPE811_ADC_CAPT, (1 << ch));

	prog = 0;
	retry_cnt = 10;
	do {
		stmpe811_i2c_read(client, STMPE811_ADC_CAPT, data, (u8)1);
		pr_debug("%s: ADC_CAPT(0x%x)\n", __func__, data[0]);

		prog = (data[0] & (1 << ch));

		if (prog) {
			pr_debug("%s: ch%d conversion completed\n",
							__func__, ch);
			break;
		} else {
			pr_info("%s: ch%d conversion progressing\n",
							__func__, ch);
			msleep(20);
		}
	} while ((!prog) && (--retry_cnt > 0));

	if (retry_cnt == 0) {
		pr_err("%s: ch%d conversion fail\n", __func__, ch);
		return -EBUSY;
	}

	data_channel_addr = STMPE811_ADC_DATA_CH0 + (ch * 2);
	stmpe811_i2c_read(client, data_channel_addr, data, (u8)2);
	w_data = ((data[0] << 8) | data[1]) & 0x0FFF;
	pr_debug("%s: ADC_DATA_CH%d(0x%x, %d)\n", __func__, ch, w_data, w_data);

	return w_data;
}
EXPORT_SYMBOL(stmpe811_get_adc_data);

int stmpe811_get_adc_value(u8 channel)
{
	struct stmpe811_platform_data *pdata =
		stmpe811_adc_i2c_client->dev.platform_data;
	int adc_data;
	int adc_value;
	int low, mid, high;
	struct adc_table_data *temper_table = NULL;
	pr_debug("%s\n", __func__);

	adc_data = stmpe811_get_adc_data(channel);

	low = mid = high = 0;
	switch (channel) {
	case 0:
		if ((!pdata->adc_table_ch0) || (!pdata->table_size_ch0))
			goto table_err;
		temper_table = pdata->adc_table_ch0;
		high = pdata->table_size_ch0 - 1;
		break;
	case 1:
		if ((!pdata->adc_table_ch1) || (!pdata->table_size_ch1))
			goto table_err;
		temper_table = pdata->adc_table_ch1;
		high = pdata->table_size_ch1 - 1;
		break;
	case 2:
		if ((!pdata->adc_table_ch2) || (!pdata->table_size_ch2))
			goto table_err;
		temper_table = pdata->adc_table_ch2;
		high = pdata->table_size_ch2 - 1;
		break;
	case 3:
		if ((!pdata->adc_table_ch3) || (!pdata->table_size_ch3))
			goto table_err;
		temper_table = pdata->adc_table_ch3;
		high = pdata->table_size_ch3 - 1;
		break;
	case 4:
		if ((!pdata->adc_table_ch4) || (!pdata->table_size_ch4))
			goto table_err;
		temper_table = pdata->adc_table_ch4;
		high = pdata->table_size_ch4 - 1;
		break;
	case 5:
		if ((!pdata->adc_table_ch5) || (!pdata->table_size_ch5))
			goto table_err;
		temper_table = pdata->adc_table_ch5;
		high = pdata->table_size_ch5 - 1;
		break;
	case 6:
		if ((!pdata->adc_table_ch6) || (!pdata->table_size_ch6))
			goto table_err;
		temper_table = pdata->adc_table_ch6;
		high = pdata->table_size_ch6 - 1;
		break;
	case 7:
		if ((!pdata->adc_table_ch7) || (!pdata->table_size_ch7))
			goto table_err;
		temper_table = pdata->adc_table_ch7;
		high = pdata->table_size_ch7 - 1;
		break;
	default:
		pr_info("%s: not exist temper table for ch(%d)\n", __func__,
							channel);
		return -EINVAL;
		break;
	}

	/* Out of table range */
	if (adc_data >= temper_table[low].adc) {
		adc_value = temper_table[low].value * 10;
		return adc_value;
	} else if (adc_data <= temper_table[high].adc) {
		adc_value = temper_table[high].value * 10;
		return adc_value;
	}

	while (low <= high) {
		mid = (low + high) / 2;
		if (temper_table[mid].adc > adc_data)
			low = mid + 1;
		else if (temper_table[mid].adc < adc_data)
			high = mid - 1;
		else
			break;
	}
	adc_value = temper_table[mid].value * 10;

	pr_debug("%s: adc data(%d), adc value(%d)\n", __func__,
					adc_data, adc_value);
	return adc_value;

table_err:
	pr_err("%s: table for ch%d does not exist\n", __func__, channel);
	return -EINVAL;
}
EXPORT_SYMBOL(stmpe811_get_adc_value);

static ssize_t adc_data_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%s\n", "adc_test_show");
}

static ssize_t adc_data_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	int mode;
	s16 val;
	sscanf(buf, "%d", &mode);

	if (mode < 0 || mode > 7) {
		pr_err("invalid channel: %d", mode);
		return -EINVAL;
	}

	val = stmpe811_get_adc_data((u8)mode);
	pr_info("adc data from ch%d: %d\n", mode, val);

	return count;
}
static DEVICE_ATTR(adc_data, S_IRUGO | S_IWUSR | S_IWGRP,
		adc_data_show, adc_data_store);

static ssize_t adc_value_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%s\n", "adc_test_show");
}

static ssize_t adc_value_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	int mode;
	s16 val;
	sscanf(buf, "%d", &mode);

	if (mode < 0 || mode > 7) {
		pr_err("invalid channel: %d", mode);
		return -EINVAL;
	}

	val = stmpe811_get_adc_value((u8)mode);
	pr_info("adc value from ch%d: %d\n", mode, val);

	return count;
}
static DEVICE_ATTR(adc_value, S_IRUGO | S_IWUSR | S_IWGRP,
		adc_value_show, adc_value_store);

static int stmpe811_reg_init(struct stmpe811_adc_data *adc_data)
{
	struct i2c_client *client = adc_data->client;
	int ret;
	u8 data[2];
	u16 w_data;
	pr_debug("%s\n", __func__);

	/* read device identification */
	ret = stmpe811_i2c_read(client, STMPE811_CHIP_ID, data, (u8)2);
	pr_info("%s: ret: %d\n", __func__, ret);
	if (ret < 0) {
		pr_err("%s: reg init error: %d\n", __func__, ret);
		goto reg_init_error;
	}

	w_data = ((data[0]<<8) | data[1]) & 0x0FFF;
	pr_info("%s: STMPE811_CHIP_ID(0x%x)\n", __func__, w_data);

	/* read revision number, 0x01 for es, 0x03 for final silicon */
	stmpe811_i2c_read(client, STMPE811_ID_VER, data, (u8)1);
	pr_info("%s: STMPE811_ID_VER(0x%x)\n", __func__, data[0]);

	/* clock control: only adc on */
	w_data = 0x0E;
	stmpe811_write_register(client, STMPE811_SYS_CTRL2, w_data);
	stmpe811_i2c_read(client, STMPE811_SYS_CTRL2, data, (u8)1);
	pr_info("%s: STMPE811_SYS_CTRL2(0x%x)\n", __func__, data[0]);

	/* interrupt enable: disable interrupt */
	w_data = 0x00;
	stmpe811_write_register(client, STMPE811_INT_EN, w_data);
	stmpe811_i2c_read(client, STMPE811_INT_EN, data, (u8)1);
	pr_info("%s: STMPE811_INT_EN(0x%x)\n", __func__, data[0]);

	/* adc control: 64 sample time, 12bit adc, internel referance*/
	w_data = 0x38;
	stmpe811_write_register(client, STMPE811_ADC_CTRL1, w_data);
	stmpe811_i2c_read(client, STMPE811_ADC_CTRL1, data, (u8)1);
	pr_info("%s: STMPE811_ADC_CTRL1(0x%x)\n", __func__, data[0]);

	/* adc control: 1.625MHz typ */
	w_data = 0x03;
	stmpe811_write_register(client, STMPE811_ADC_CTRL2, w_data);
	stmpe811_i2c_read(client, STMPE811_ADC_CTRL2, data, (u8)1);
	pr_info("%s: STMPE811_ADC_CTRL2(0x%x)\n", __func__, data[0]);

	/* alt func: use for adc */
	w_data = 0x00;
	stmpe811_write_register(client, STMPE811_GPIO_AF, w_data);
	stmpe811_i2c_read(client, STMPE811_GPIO_AF, data, (u8)1);
	pr_info("%s: STMPE811_GPIO_AF(0x%x)\n", __func__, data[0]);

	/* ts control: tsc disable */
	w_data = 0x00;
	stmpe811_write_register(client, STMPE811_TSC_CTRL, w_data);
	stmpe811_i2c_read(client, STMPE811_TSC_CTRL, data, (u8)1);
	pr_info("%s: STMPE811_TSC_CTRL(0x%x)\n", __func__, data[0]);

reg_init_error:
	return ret;
}

static const struct file_operations stmpe811_fops = {
	.owner = THIS_MODULE,
};

static struct miscdevice stmpe811_adc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sec_adc",
	.fops = &stmpe811_fops,
};

static int stmpe811_adc_i2c_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct stmpe811_adc_data *adc_data;
	int ret;
	u8 i2c_data[2];
	pr_info("%s: stmpe811 adc driver loading\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	adc_data = kzalloc(sizeof(struct stmpe811_adc_data), GFP_KERNEL);
	if (!adc_data)
		return -ENOMEM;

	adc_data->client = client;
	adc_data->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, adc_data);

	stmpe811_adc_i2c_client = client;

	ret = misc_register(&stmpe811_adc_device);
	if (ret)
		goto misc_register_fail;

	mutex_init(&adc_data->adc_lock);

	/* initialize adc registers */
	ret = stmpe811_reg_init(adc_data);
	if (ret < 0)
		goto reg_init_error;

	/* TODO: ADC_INT setting */

	/* create sysfs for debugging and factory mode*/
	adc_dev = device_create(sec_class, NULL, 0, NULL, "adc");
	if (IS_ERR(adc_dev)) {
		ret = -ENOMEM;
		goto deregister_misc;
	}

	ret = device_create_file(adc_dev, &dev_attr_adc_data);
	if (ret < 0)
		goto destroy_device;

	ret = device_create_file(adc_dev, &dev_attr_adc_value);
	if (ret < 0)
		goto dev_attr_adc_value_err;

	stmpe811_i2c_read(client, STMPE811_CHIP_ID, i2c_data, (u8)2);
	pr_info("stmpe811 adc(id 0x%x) initialized\n",
				((i2c_data[0]<<8) | i2c_data[1]));

	return 0;

dev_attr_adc_value_err:
	device_remove_file(adc_dev, &dev_attr_adc_data);
destroy_device:
	device_destroy(sec_class, 0);
deregister_misc:
reg_init_error:
	misc_deregister(&stmpe811_adc_device);
misc_register_fail:
	pr_info("stmpe811 probe fail: %d\n", ret);
	kfree(adc_data);
	return ret;
}

static int stmpe811_adc_i2c_remove(struct i2c_client *client)
{
	struct stmpe811_adc_data *adc = i2c_get_clientdata(client);
	pr_info("%s\n", __func__);

	misc_deregister(&stmpe811_adc_device);

	device_remove_file(adc_dev, &dev_attr_adc_value);
	device_remove_file(adc_dev, &dev_attr_adc_data);
	device_destroy(sec_class, 0);

	mutex_destroy(&adc->adc_lock);
	kfree(adc);

	return 0;
}

static int stmpe811_adc_suspend(struct device *dev)
{

	return 0;
}

static int stmpe811_adc_resume(struct device *dev)
{
	return 0;
}

static const struct i2c_device_id stmpe811_adc_device_id[] = {
	{"stmpe811-adc", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, stmpe811_adc_device_id);

static const struct dev_pm_ops stmpe811_adc_pm_ops = {
	.suspend = stmpe811_adc_suspend,
	.resume = stmpe811_adc_resume,
};

static struct i2c_driver stmpe811_adc_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "stmpe811-adc",
		.pm = &stmpe811_adc_pm_ops,
	},
	.probe		= stmpe811_adc_i2c_probe,
	.remove		= stmpe811_adc_i2c_remove,
	.id_table	= stmpe811_adc_device_id,
};

static int __init stmpe811_adc_init(void)
{
	return i2c_add_driver(&stmpe811_adc_i2c_driver);
}

static void __exit stmpe811_adc_exit(void)
{
	i2c_del_driver(&stmpe811_adc_i2c_driver);
}

module_init(stmpe811_adc_init);
module_exit(stmpe811_adc_exit);

MODULE_AUTHOR("SangYoung Son <hello.son@samsung.com>");
MODULE_DESCRIPTION("stmpe811 adc driver");
MODULE_LICENSE("GPL");
