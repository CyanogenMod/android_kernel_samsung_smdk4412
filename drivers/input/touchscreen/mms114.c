/*
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/i2c/mms114.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>

/* Write only registers */
#define MMS114_MODE_CONTROL		0x01
#define MMS114_OPERATION_MODE_MASK	0xE
#define MMS114_ACTIVE			(1 << 1)

#define MMS114_XY_RESOLUTION_H		0x02
#define MMS114_X_RESOLUTION		0x03
#define MMS114_Y_RESOLUTION		0x04
#define MMS114_CONTACT_THRESHOLD	0x05
#define MMS114_MOVING_THRESHOLD		0x06

/* Read only registers */
#define MMS114_PACKET_SIZE		0x0F

#define MMS114_INFOMATION		0x10
#define MMS114_ACT_OFFSET		7
#define MMS114_ACT_MASK			0x1
#define MMS114_TYPE_OFFSET		5
#define MMS114_TYPE_MASK		0x3
#define MMS114_ID_MASK			0xF

#define MMS114_TSP_REV			0xF0

/* Minimum delay time is 50us between stop and start signal of i2c */
#define MMS114_I2C_DELAY		50

/* 200ms needs after power on */
#define MMS114_POWERON_DELAY		200

/* Touchscreen absolute values */
#define MMS114_MAX_AREA			0xff

#define MMS114_MAX_TOUCH		10
#define MMS114_PACKET_NUM		6
#define MMS114_MAX_PACKET		(MMS114_MAX_TOUCH * MMS114_PACKET_NUM)

/* Touch type */
#define MMS114_TYPE_NONE		0
#define MMS114_TYPE_TOUCHSCREEN		1
#define MMS114_TYPE_TOUCHKEY		2

struct mms114_touchdata {
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int strength;
	unsigned int pressed;
	bool updated;
};

struct mms114_data {
	struct i2c_client	*client;
	struct input_dev	*input_dev;
	struct mutex		mutex;
	struct mms114_touchdata	touchdata[MMS114_MAX_TOUCH];
	struct regulator	*io_reg;
	const struct mms114_platform_data	*pdata;

	/* Use cache data for mode control register(write only) */
	u8			cache_mode_control;
};

static int __mms114_read_reg(struct mms114_data *data, unsigned int reg,
			     unsigned int len, u8 *val)
{
	struct i2c_client *client = data->client;
	struct i2c_msg xfer;
	u8 buf = reg & 0xff;
	int ret;

	if (reg == MMS114_MODE_CONTROL) {
		dev_err(&client->dev, "No support to read mode control reg\n");
		return -EINVAL;
	}

	mutex_lock(&data->mutex);

	/* Use repeated start */
	xfer.addr = client->addr;
	xfer.flags = I2C_M_TEN | I2C_M_NOSTART;
	xfer.len = 1;
	xfer.buf = &buf;

	ret = i2c_transfer(client->adapter, &xfer, 1);
	if (ret != 1) {
		dev_err(&client->dev, "%s: i2c transfer failed (%d)\n",
				__func__, ret);
		ret = -EIO;
		goto err;
	}

	ret = i2c_master_recv(client, val, len);
	udelay(MMS114_I2C_DELAY);
	if (ret != len) {
		dev_err(&client->dev, "%s, i2c recv failed (%d)\n", __func__,
				ret);
		ret = -EIO;
		goto err;
	}

	ret = 0;

err:
	mutex_unlock(&data->mutex);
	return ret;
}

static int mms114_read_reg(struct mms114_data *data, unsigned int reg)
{
	u8 val;
	int ret;

	if (reg == MMS114_MODE_CONTROL)
		return data->cache_mode_control;

	ret = __mms114_read_reg(data, reg, 1, &val);
	if (!ret)
		ret = val;

	return ret;
}

static int mms114_write_reg(struct mms114_data *data, unsigned int reg,
			    unsigned int val)
{
	struct i2c_client *client = data->client;
	u8 buf[2];
	int ret;

	buf[0] = reg & 0xff;
	buf[1] = val & 0xff;

	ret = i2c_master_send(client, buf, 2);
	udelay(MMS114_I2C_DELAY);
	if (ret != 2) {
		dev_err(&client->dev, "%s, i2c send failed (%d)\n", __func__,
				ret);
		return -EIO;
	}

	if (reg == MMS114_MODE_CONTROL)
		data->cache_mode_control = val;

	return 0;
}

static void mms114_input_st_report(struct mms114_data *data)
{
	struct mms114_touchdata *touchdata = data->touchdata;
	struct input_dev *input_dev = data->input_dev;
	int id;

	for (id = 0; id < MMS114_MAX_TOUCH; id++) {
		if (!touchdata[id].updated)
			continue;

		if (touchdata[id].pressed) {
			input_report_abs(input_dev, ABS_X, touchdata[id].x);
			input_report_abs(input_dev, ABS_Y, touchdata[id].y);
		}

		input_sync(input_dev);

		touchdata[id].updated = false;
	}
}

static void mms114_input_report(struct mms114_data *data)
{
	struct mms114_touchdata *touchdata = data->touchdata;
	struct input_dev *input_dev = data->input_dev;
	int touch_num = 0;
	int id;

	for (id = 0; id < MMS114_MAX_TOUCH; id++) {
		if (!touchdata[id].updated)
			continue;

		input_mt_slot(input_dev, id);
		input_mt_report_slot_state(input_dev, MT_TOOL_FINGER,
				touchdata[id].pressed);

		if (touchdata[id].pressed) {
			touch_num++;
			input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR,
					touchdata[id].width);
			input_report_abs(input_dev, ABS_MT_POSITION_X,
					touchdata[id].x);
			input_report_abs(input_dev, ABS_MT_POSITION_Y,
					touchdata[id].y);
			/* FIXME */
			input_sync(input_dev);
		}
	}

	input_report_key(input_dev, BTN_TOUCH, touch_num > 0);
	/* input_sync(input_dev); */

	mms114_input_st_report(data);
}

static void mms114_proc_touchdata(struct mms114_data *data, u8 *buf)
{
	const struct mms114_platform_data *pdata = data->pdata;
	struct i2c_client *client = data->client;
	struct mms114_touchdata *touchdata;
	unsigned int id = (buf[0] & MMS114_ID_MASK) - 1;
	unsigned int type = (buf[0] >> MMS114_TYPE_OFFSET) & MMS114_TYPE_MASK;
	unsigned int pressed = (buf[0] >> MMS114_ACT_OFFSET) & MMS114_ACT_MASK;
	unsigned int x = buf[2] | (buf[1] & 0xf) << 8;
	unsigned int y = buf[3] | ((buf[1] >> 4) & 0xf) << 8;

	if (id >= MMS114_MAX_TOUCH) {
		dev_dbg(&client->dev, "Wrong touch id (%d)\n", id);
		return;
	}

	if (type != MMS114_TYPE_TOUCHSCREEN) {
		dev_dbg(&client->dev, "Wrong touch type (%d)\n", type);
		return;
	}

	touchdata = &data->touchdata[id];

	if (!pressed && !touchdata->pressed) {
		dev_dbg(&client->dev, "Wrong touch release (id: %d)\n", id);
		return;
	}

	if (x > pdata->x_size || y > pdata->y_size) {
		dev_dbg(&client->dev, "Wrong touch coordinates (%d, %d)\n",
				x, y);
		return;
	}

	if (pdata->x_invert)
		x = pdata->x_size - x;
	if (pdata->y_invert)
		y = pdata->y_size - y;

	touchdata->x = x;
	touchdata->y = y;
	touchdata->width = buf[4];
	touchdata->strength = buf[5];
	touchdata->pressed = pressed;
	touchdata->updated = true;

	dev_dbg(&client->dev, "id: %d, type: %d, pressed: %d\n",
			id, type, pressed);
	dev_dbg(&client->dev, "x: %d, y: %d, width: %d, strength: %d\n",
			touchdata->x, touchdata->y,
			touchdata->width, touchdata->strength);
}

static irqreturn_t mms114_interrupt(int irq, void *dev_id)
{
	struct mms114_data *data = dev_id;
	u8 buf[MMS114_MAX_PACKET];
	int packet_size;
	int touch_size;
	int index;
	int ret;

	packet_size = mms114_read_reg(data, MMS114_PACKET_SIZE);
	if (packet_size < 0)
		goto err;

	touch_size = packet_size / MMS114_PACKET_NUM;

	ret = __mms114_read_reg(data, MMS114_INFOMATION, packet_size, buf);
	if (ret < 0)
		goto err;

	for (index = 0; index < touch_size; index++)
		mms114_proc_touchdata(data, buf + (index * MMS114_PACKET_NUM));

	mms114_input_report(data);

err:
	return IRQ_HANDLED;
}

static int mms114_set_active(struct mms114_data *data, bool active)
{
	int val;

	val = mms114_read_reg(data, MMS114_MODE_CONTROL);
	if (val < 0)
		return val;

	val &= ~MMS114_OPERATION_MODE_MASK;

	/* If active is false, sleep mode */
	if (active)
		val |= MMS114_ACTIVE;

	return mms114_write_reg(data, MMS114_MODE_CONTROL, val);
}

static int mms114_get_version(struct mms114_data *data)
{
	struct device *dev = &data->client->dev;
	u8 buf[6];
	int ret;

	ret = __mms114_read_reg(data, MMS114_TSP_REV, 6, buf);
	if (ret < 0)
		return ret;

	dev_info(dev, "TSP Rev: 0x%x, HW Rev: 0x%x, Firmware Ver: 0x%x\n",
			buf[0], buf[1], buf[3]);

	return 0;
}

static int mms114_setup_regs(struct mms114_data *data)
{
	const struct mms114_platform_data *pdata = data->pdata;
	int val;
	int ret;

	ret = mms114_set_active(data, true);
	if (ret < 0)
		return ret;

	val = (pdata->x_size >> 8) & 0xf;
	val |= ((pdata->y_size >> 8) & 0xf) << 4;
	ret = mms114_write_reg(data, MMS114_XY_RESOLUTION_H, val);
	if (ret < 0)
		return ret;

	val = pdata->x_size & 0xff;
	ret = mms114_write_reg(data, MMS114_X_RESOLUTION, val);
	if (ret < 0)
		return ret;

	val = pdata->y_size & 0xff;
	ret = mms114_write_reg(data, MMS114_Y_RESOLUTION, val);
	if (ret < 0)
		return ret;

	if (pdata->contact_threshold) {
		ret = mms114_write_reg(data, MMS114_CONTACT_THRESHOLD,
				pdata->contact_threshold);
		if (ret < 0)
			return ret;
	}

	if (pdata->moving_threshold) {
		ret = mms114_write_reg(data, MMS114_MOVING_THRESHOLD,
				pdata->moving_threshold);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int __devinit mms114_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct mms114_data *data;
	struct input_dev *input_dev;
	int ret;

	if (!client->dev.platform_data) {
		dev_err(&client->dev, "Need platform data\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_PROTOCOL_MANGLING)) {
		dev_err(&client->dev,
			"Need i2c bus that supports protocol mangling\n");
		return -ENODEV;
	}

	data = kzalloc(sizeof(struct mms114_data), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!data || !input_dev) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_free_mem;
	}

	data->client = client;
	data->input_dev = input_dev;
	data->pdata = client->dev.platform_data;

	mutex_init(&data->mutex);

	input_dev->name = "MELPAS MMS114 Touchscreen";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	input_set_abs_params(input_dev, ABS_X, 0, data->pdata->x_size, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, data->pdata->y_size, 0, 0);

	/* For multi touch */
	input_mt_init_slots(input_dev, MMS114_MAX_TOUCH);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
			     0, MMS114_MAX_AREA, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			     0, data->pdata->x_size, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
			     0, data->pdata->y_size, 0, 0);

	input_set_drvdata(input_dev, data);
	i2c_set_clientdata(client, data);

	data->io_reg = regulator_get(&client->dev, "vdd");
	if (IS_ERR(data->io_reg)) {
		ret = PTR_ERR(data->io_reg);
		dev_err(&client->dev, "Unable to get the IO regulator (%d)\n",
				ret);
		goto err_free_mem;
	}

	regulator_enable(data->io_reg);
	mdelay(MMS114_POWERON_DELAY);

	if (data->pdata->cfg_pin)
		data->pdata->cfg_pin(true);

	ret = mms114_get_version(data);
	if (ret < 0)
		goto err_io_reg;

	ret = mms114_setup_regs(data);
	if (ret < 0)
		goto err_io_reg;

	ret = request_threaded_irq(client->irq, NULL, mms114_interrupt,
			IRQF_TRIGGER_FALLING, "mms114", data);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to register interrupt\n");
		goto err_io_reg;
	}

	ret = input_register_device(data->input_dev);
	if (ret < 0)
		goto err_free_irq;

	return 0;

err_free_irq:
	free_irq(client->irq, data);
err_io_reg:
	regulator_disable(data->io_reg);
	regulator_put(data->io_reg);
err_free_mem:
	input_free_device(input_dev);
	kfree(data);
	return ret;
}

static int __devexit mms114_remove(struct i2c_client *client)
{
	struct mms114_data *data = i2c_get_clientdata(client);

	free_irq(client->irq, data);
	regulator_disable(data->io_reg);
	regulator_put(data->io_reg);
	input_unregister_device(data->input_dev);
	kfree(data);

	return 0;
}

#ifdef CONFIG_PM
static int mms114_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mms114_data *data = i2c_get_clientdata(client);
	struct mms114_touchdata *touchdata = data->touchdata;
	int id;
	int ret;

	disable_irq(client->irq);

	/* Release all touch */
	for (id = 0; id < MMS114_MAX_TOUCH; id++) {
		if (touchdata[id].pressed) {
			touchdata[id].pressed = 0;
			touchdata[id].updated = true;
		}
	}
	mms114_input_report(data);

	ret = mms114_set_active(data, false);
	if (ret < 0)
		return ret;

	if (data->pdata->cfg_pin)
		data->pdata->cfg_pin(false);

	regulator_disable(data->io_reg);

	return 0;
}

static int mms114_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mms114_data *data = i2c_get_clientdata(client);
	int ret;

	regulator_enable(data->io_reg);
	mdelay(MMS114_POWERON_DELAY);

	if (data->pdata->cfg_pin)
		data->pdata->cfg_pin(true);

	ret = mms114_setup_regs(data);
	if (ret < 0)
		return ret;

	enable_irq(client->irq);

	return 0;
}

static const struct dev_pm_ops mms114_pm_ops = {
	.suspend	= mms114_suspend,
	.resume		= mms114_resume,
};
#endif

static const struct i2c_device_id mms114_id[] = {
	{ "mms114", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mms114_id);

static struct i2c_driver mms114_driver = {
	.driver = {
		.name	= "mms114",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &mms114_pm_ops,
#endif
	},
	.probe		= mms114_probe,
	.remove		= __devexit_p(mms114_remove),
	.id_table	= mms114_id,
};

static int __init mms114_init(void)
{
	return i2c_add_driver(&mms114_driver);
}

static void __exit mms114_exit(void)
{
	i2c_del_driver(&mms114_driver);
}

module_init(mms114_init);
module_exit(mms114_exit);

/* Module information */
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_DESCRIPTION("MELFAS mms114 Touchscreen driver");
MODULE_LICENSE("GPL");
