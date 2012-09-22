/* drivers/input/touchscreen/synaptics_s7301.c
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#include <linux/synaptics_s7301.h>

#define REPORT_MT(x, y, z, w_max, w_min) \
do {     \
	input_report_abs(data->input, ABS_MT_POSITION_X, x);	\
	input_report_abs(data->input, ABS_MT_POSITION_Y,	y);	\
	input_report_abs(data->input, ABS_MT_PRESSURE, z);	\
	input_report_abs(data->input, ABS_MT_TOUCH_MAJOR, w_max);	\
	input_report_abs(data->input, ABS_MT_TOUCH_MINOR, w_min);	\
} while (0)

#define SET_FUNC_ADDR(num, page)	\
do {		\
	data->f##num.query_base_addr = buffer[0] + page;	\
	data->f##num.command_base_addr = buffer[1] + page;	\
	data->f##num.control_base_addr = buffer[2] + page;	\
	data->f##num.data_base_addr = buffer[3] + page;	\
	if (!test_bit(buffer[5], data->func_bit)) {	\
		__set_bit(buffer[5], data->func_bit);	\
		cnt++;	\
	}	\
} while (0)

#define CHECK_PAGE(addr)	((addr >> 8) & 0xff)

static int synaptics_ts_set_page(struct synaptics_drv_data *data,
	u16 addr)
{
	u8 page = CHECK_PAGE(addr);
	if (data->suspend)
		return -EAGAIN;

	if (page != data->page) {
		u8 buf[2] = {0xff, page};
		 i2c_master_send(data->client, buf, sizeof(buf));
		 data->page = page;
	}
	return 0;
}

int synaptics_ts_write_data(struct synaptics_drv_data *data,
	u16 addr, u8 cmd)
{
	struct i2c_msg msg;
	u8 buf[2];

	if (synaptics_ts_set_page(data, addr))
		return -EAGAIN;

	buf[0] = addr & 0xff;
	buf[1] = cmd;

	msg.addr = data->client->addr;
	msg.flags = data->client->flags & I2C_M_TEN;
	msg.len = 2;
	msg.buf = buf;

	return i2c_transfer(data->client->adapter, &msg, 1);
}

int synaptics_ts_read_data(struct synaptics_drv_data *data,
	u16 addr, u8 *buf)
{
	struct i2c_msg msg[2];

	if (synaptics_ts_set_page(data, addr))
		return -EAGAIN;

	msg[0].addr = data->client->addr;
	msg[0].flags = 0x00;
	msg[0].len = 1;
	msg[0].buf = (u8 *) &addr;

	msg[1].addr = data->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	return i2c_transfer(data->client->adapter, msg, 2);
}

int synaptics_ts_write_block(struct synaptics_drv_data *data,
	u16 addr, u8 *cmd, u16 count)
{
	struct i2c_msg msg;
	int ret = 0, i = 0;
	u8 buf[256];

	if (synaptics_ts_set_page(data, addr))
		return -EAGAIN;

	buf[0] = addr & 0xff;

	for (i  = 1; i <= count; i++)
		buf[i] = *cmd++;

	msg.addr = data->client->addr;
	msg.flags = data->client->flags & I2C_M_TEN;
	msg.len = count + 1;
	msg.buf = buf;

	ret = i2c_transfer(data->client->adapter, &msg, 1);

	return (ret == 1) ? count : ret;
}

int synaptics_ts_read_block(struct synaptics_drv_data *data,
	u16 addr, u8 *buf, u16 count)
{
	struct i2c_msg msg[2];
	int ret = 0;

	if (synaptics_ts_set_page(data, addr))
		return -EAGAIN;

	msg[0].addr = data->client->addr;
	msg[0].flags = 0x00;
	msg[0].len = 1;
	msg[0].buf = (u8 *) &addr;

	msg[1].addr = data->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = count;
	msg[1].buf = buf;

	ret = i2c_transfer(data->client->adapter, msg, 2);

	return (ret == 1) ? count : ret;
}

#if defined(CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK)
static void free_dvfs_lock(struct work_struct *work)
{
	struct synaptics_drv_data *data =
		container_of(work, struct synaptics_drv_data,
			dvfs_dwork.work);

	if (data->dvfs_lock_status)
		dev_lock(data->bus_dev,
			data->dev, SEC_BUS_LOCK_FREQ);
	else {
		dev_unlock(data->bus_dev, data->dev);
		exynos_cpufreq_lock_free(DVFS_LOCK_ID_TSP);
	}
}
void set_dvfs_lock(struct synaptics_drv_data *data, bool en)
{
	if (0 == data->cpufreq_level)
		exynos_cpufreq_get_level(SEC_DVFS_LOCK_FREQ,
			&data->cpufreq_level);

	if (en) {
		if (!data->dvfs_lock_status) {
			cancel_delayed_work(&data->dvfs_dwork);
			dev_lock(data->bus_dev,
				data->dev, SEC_BUS_LOCK_FREQ2);
			exynos_cpufreq_lock(DVFS_LOCK_ID_TSP,
				data->cpufreq_level);
			data->dvfs_lock_status = true;
			schedule_delayed_work(&data->dvfs_dwork,
				 msecs_to_jiffies(SEC_DVFS_LOCK_TIMEOUT));
		}
	} else {
		if (data->dvfs_lock_status) {
			schedule_delayed_work(&data->dvfs_dwork,
				msecs_to_jiffies(SEC_DVFS_LOCK_TIMEOUT));
			data->dvfs_lock_status = false;
		}
	}
}
#endif	/* CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK */

static void forced_release_fingers(struct synaptics_drv_data *data)
{
	int i;
#if 0
	printk(KERN_DEBUG "[TSP] %s\n", __func__);
#endif
	for (i = 0; i < MAX_TOUCH_NUM; ++i) {
		input_mt_slot(data->input, i);
		input_mt_report_slot_state(data->input,
			MT_TOOL_FINGER, 0);

		data->finger[i].status = MT_STATUS_INACTIVE;
		data->finger[i].z = 0;
	}
	input_sync(data->input);
	set_dvfs_lock(data, false);
	return ;
}

static int synaptics_ts_set_func_info(struct synaptics_drv_data *data)
{
	int i = 0, cnt = 0;
	u8 buffer[6];
	u16 addr = 0;
	u16 base_addr = FUNC_ADDR_FIRST;
	u16 last_addr = FUNC_ADDR_LAST;

	for (i = 0; i <= PAGE_MAX; i += PAGE_MAX) {
		base_addr += i;
		last_addr += i;
		for (addr = base_addr; addr >= last_addr;
			addr -= FUNC_ADDR_SIZE) {
			synaptics_ts_read_block(data,
				addr, buffer, 6);
			if (data->debug) {
				printk(KERN_DEBUG
					"[TSP] function : 0x%x\n",
					buffer[5]);
				printk(KERN_DEBUG
					"[TSP] query_base_addr : 0x%x\n",
					buffer[0]);
				printk(KERN_DEBUG
					"[TSP] command_base_addr : 0x%x\n",
					buffer[1]);
				printk(KERN_DEBUG
					"[TSP] control_base_addr : 0x%x\n",
					buffer[2]);
				printk(KERN_DEBUG
					"[TSP] data_base_addr : 0x%x\n",
					buffer[3]);
			}
			switch (buffer[5]) {
			case 0x01:
				SET_FUNC_ADDR(01, i);
				break;

			case 0x11:
				SET_FUNC_ADDR(11, i);
				break;

			case 0x34:
				SET_FUNC_ADDR(34, i);
				break;

			case 0x54:
				SET_FUNC_ADDR(54, i);
				break;

			default:
				break;
			}
		}
	}
	return cnt;
}

static int synaptics_ts_read_dummy(struct synaptics_drv_data *data)
{
	u8 buf;
	int ret = 0, cnt = 0;
	while (cnt < 5) {
		ret = synaptics_ts_read_data(data,
				data->f01.data_base_addr, &buf);
		if (ret < 0) {
			pr_err("[TSP] read reg_data failed(%d) ret : %d\n",
				cnt++, ret);
			msleep(20);
		} else
			return 0;
	}
	return -EIO;
}

static void set_charger_connection_bit(struct synaptics_drv_data *data)
{
	u8 buf = 0;
	u8 delta_threshold = 0;

	if (data->suspend) {
		schedule_delayed_work(&data->noti_dwork, HZ / 2);
		return ;
	}

	synaptics_ts_read_data(data,
		data->f01.control_base_addr, &buf);

	if (data->charger_connection) {
		buf |= CHARGER_CONNECT_BIT;
		delta_threshold = 3;
	} else {
		buf &= ~(CHARGER_CONNECT_BIT);
		delta_threshold = 1;
	}

	synaptics_ts_write_data(data,
		data->f01.control_base_addr, buf);

	synaptics_ts_write_data(data,
		data->f11.control_base_addr + 2, delta_threshold);
	synaptics_ts_write_data(data,
		data->f11.control_base_addr + 3, delta_threshold);
}

static void inform_charger_connection(struct charger_callbacks *cb, int mode)
{
	struct synaptics_drv_data *data = container_of(cb,
			struct synaptics_drv_data, callbacks);

	data->charger_connection = !!mode;
	if (data->ready)
		set_charger_connection_bit(data);
}

static int synaptics_ts_set_func(struct synaptics_drv_data *data)
{
	int i = 0;

	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	if (synaptics_ts_set_func_info(data) != 4) {
		pr_err("[TSP] failed to get function info.\n");
		forced_fw_update(data);
		synaptics_ts_set_func_info(data);
	} else
		synaptics_fw_updater(data, NULL);

	printk(KERN_DEBUG "[TSP] firmware version %s\n",
		data->firm_version);

	for (i = 0; i < MAX_TOUCH_NUM; ++i)
		data->finger[i].status = MT_STATUS_INACTIVE;

	return synaptics_ts_read_dummy(data);
}

static int check_interrupt_status(struct synaptics_drv_data *data,
	u32 *finger_status)
{
	int ret = 0;
	u8 buf[3];
	u16 addr = 0;

	/* read the interrupt status */
	addr = data->f01.data_base_addr + 1;
	ret = synaptics_ts_read_data(data,
		addr, buf);
	if (ret < 0) {
		pr_err("[TSP] failed to read i2c data(%d)\n", __LINE__);
		return -EIO;
	}

	/* read the finger states */
	addr = data->f11.data_base_addr;
	ret = synaptics_ts_read_block(data,
		addr, buf, 3);
	if (ret < 0) {
		pr_err("[TSP] failed to read i2c data(%d)\n", __LINE__);
		return -EIO;
	}

	*finger_status = (u32) (buf[0] | (buf[1] << 8) |
		((buf[2] & 0xf) << 16));

	if (data->debug)
		printk(KERN_DEBUG
			"[TSP] finger_status : 0x%x\n",
			*finger_status);

	return 0;
}

static void synaptics_ts_read_points(struct synaptics_drv_data *data,
	u32 finger_status)
{
	struct finger_data buf;
	bool finger_pressed = false;
	int ret = 0;
	int id = 0;
	u16 addr = data->f11.data_base_addr + 3;

	for (id = 0; id < MAX_TOUCH_NUM; id++,
		addr += sizeof(struct finger_data)) {
		if (finger_status & (0x3 << (id * 2))) {
			ret = synaptics_ts_read_block(data,
					addr, (u8 *) &buf, 5);
			if (ret < 0) {
				pr_err("[TSP] failed to read finger[%u]\n", id);
				return ;
			}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			if (data->debug)
				printk(KERN_DEBUG
					"[TSP] ID: %d, x_msb: %d, y_msb: %d, z: %d\n",
					id,
					buf.x_msb,
					buf.y_msb,
					buf.z);
#endif

			data->finger[id].x =
				(buf.x_msb << 4) +
				(buf.xy_lsb & 0x0F);
			data->finger[id].y =
				(buf.y_msb << 4) +
				(buf.xy_lsb >> 4);

			if ((buf.w >> 4) >
				(buf.w & 0x0F)) {
				data->finger[id].w_max =
					((buf.w & 0xF0) >> 4);
				data->finger[id].w_min =
					(buf.w & 0x0F);
			} else {
				data->finger[id].w_min =
					((buf.w & 0xF0) >> 4);
				data->finger[id].w_max =
					(buf.w & 0x0F);
			}

			data->finger[id].z = buf.z;
			if (data->finger[id].z) {
				if (MT_STATUS_INACTIVE ==
					data->finger[id].status) {
					data->finger[id].status =
						MT_STATUS_PRESS;
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
						printk(KERN_DEBUG
							"[TSP] ID: %d, x: %d, y: %d, z: %d\n",
							id,
							data->finger[id].x,
							data->finger[id].y,
							data->finger[id].z);
#else
						printk(KERN_DEBUG
							"s7301 %d P\n", id);
#endif
				}
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				else if (data->debug)
					printk(KERN_DEBUG
						"[TSP] ID: %d, x: %d, y: %d, z: %d\n",
						id,
						data->finger[id].x,
						data->finger[id].y,
						data->finger[id].z);
#endif
			}
		} else if (MT_STATUS_PRESS == data->finger[id].status) {
			data->finger[id].status = MT_STATUS_RELEASE;
			data->finger[id].z = 0;
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				printk(KERN_DEBUG "[TSP] ID: %d\n", id);
#else
				printk(KERN_DEBUG "s7301 %d R\n", id);
#endif
		}
	}

	for (id = 0; id < MAX_TOUCH_NUM; ++id) {
		if (MT_STATUS_INACTIVE == data->finger[id].status)
			continue;

		input_mt_slot(data->input, id);
		input_mt_report_slot_state(data->input,
			MT_TOOL_FINGER,
			!!data->finger[id].z);

		switch (data->finger[id].status) {
		case MT_STATUS_PRESS:
		case MT_STATUS_MOVE:
			finger_pressed = true;
			REPORT_MT(
				data->finger[id].x,
				data->finger[id].y,
				data->finger[id].z,
				data->finger[id].w_max,
				data->finger[id].w_min);
			break;

		case MT_STATUS_RELEASE:
			data->finger[id].status = MT_STATUS_INACTIVE;
			break;
		default:
			break;
		}
	}
	input_sync(data->input);
	set_dvfs_lock(data, finger_pressed);
}

static irqreturn_t synaptics_ts_irq_handler(int irq, void *_data)
{
	struct synaptics_drv_data *data = (struct synaptics_drv_data *)_data;
	u32 finger_status = 0;
	if (!check_interrupt_status(data, &finger_status))
		synaptics_ts_read_points(data, finger_status);
	return IRQ_HANDLED;
}

#if defined(CONFIG_HAS_EARLYSUSPEND)
static void synaptics_ts_early_suspend(struct early_suspend *h)
{
	struct synaptics_drv_data *data =
		container_of(h, struct synaptics_drv_data, early_suspend);
	printk(KERN_DEBUG "[TSP] %s\n", __func__);
	cancel_delayed_work_sync(&data->resume_dwork);
	mutex_lock(&data->mutex);
	if (!data->suspend) {
		disable_irq(data->client->irq);
		forced_release_fingers(data);
		if (!wake_lock_active(&data->wakelock)) {
			data->pdata->set_power(0);
			data->suspend = true;
		}
	}
	mutex_unlock(&data->mutex);
}

static void synaptics_ts_late_resume(struct early_suspend *h)
{
	struct synaptics_drv_data *data =
		container_of(h, struct synaptics_drv_data, early_suspend);

	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	if (data->suspend) {
		if (data->pdata->set_power(1))
			data->pdata->hw_reset();
	}
	schedule_delayed_work(&data->resume_dwork, HZ / 10);
}
#endif

static void init_function_data_dwork(struct work_struct *work)
{
	struct synaptics_drv_data *data =
		container_of(work, struct synaptics_drv_data, init_dwork.work);
	int ret = 0;

	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	ret = synaptics_ts_set_func(data);
	if (ret) {
		pr_err("[TSP] failed to initialize\n");
		return ;
	}

#if defined(CONFIG_HAS_EARLYSUSPEND)
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = synaptics_ts_early_suspend;
	data->early_suspend.resume = synaptics_ts_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

#if defined(CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK)
	INIT_DELAYED_WORK(&data->dvfs_dwork,
		free_dvfs_lock);
	data->bus_dev = dev_get("exynos-busfreq");
	data->dvfs_lock_status = false;
	ret = exynos_cpufreq_get_level(SEC_DVFS_LOCK_FREQ,
		&data->cpufreq_level);
	if (ret < 0)
		data->cpufreq_level = 0;
#endif

	data->ready = true;
	set_charger_connection_bit(data);

	if (data->client->irq) {
		ret = request_threaded_irq(data->client->irq, NULL,
				 synaptics_ts_irq_handler,
				 IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				 data->client->name, data);
		if (ret < 0) {
			pr_err("[TSP] failed to request threaded irq %d, ret %d\n",
				data->client->irq, ret);
			return ;
		}
	}

	if (!data->input_open) {
		disable_irq(data->client->irq);
		data->pdata->set_power(0);
		data->suspend = true;
	}
}

static void synaptics_ts_resume_dwork(struct work_struct *work)
{
	struct synaptics_drv_data *data =
		container_of(work, struct synaptics_drv_data,
		resume_dwork.work);

	mutex_lock(&data->mutex);
	if (data->suspend) {
		data->suspend = false;
		set_charger_connection_bit(data);
		synaptics_ts_drawing_mode(data);
		synaptics_ts_read_dummy(data);
		enable_irq(data->client->irq);
	}
	mutex_unlock(&data->mutex);
}

static void synaptics_ts_noti_dwork(struct work_struct *work)
{
	struct synaptics_drv_data *data =
		container_of(work, struct synaptics_drv_data,
		noti_dwork.work);

	set_charger_connection_bit(data);
}

static int synaptics_ts_open(struct input_dev *dev)
{
	struct synaptics_drv_data *data =
		input_get_drvdata(dev);

	data->input_open = true;

	if (data->suspend) {
		data->pdata->set_power(1);
		schedule_delayed_work(&data->resume_dwork, HZ / 10);
	}

	return 0;
}

static void synaptics_ts_close(struct input_dev *dev)
{
	/* TBD */
}

static int __init synaptics_ts_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct synaptics_drv_data *ddata;
	struct synaptics_platform_data *pdata;
	struct input_dev *input;
	int ret = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[TSP] failed to check i2c functionality!\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ddata = kzalloc(sizeof(struct synaptics_drv_data), GFP_KERNEL);
	if (unlikely(ddata == NULL)) {
		pr_err("[TSP] failed to allocate the synaptics_drv_data.\n");
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	pdata = client->dev.platform_data;
	if (pdata == NULL) {
		pr_err("[TSP] failed to get platform data\n");
		goto err_pdata;
	}

	ddata->client = client;
	ddata->pdata = pdata;
	ddata->gpio = pdata->gpio_attn;
	ddata->x_line = pdata->x_line;
	ddata->y_line = pdata->y_line;

	/* Register callbacks */
	/* To inform tsp , charger connection status*/
	ddata->callbacks.inform_charger = inform_charger_connection;
	if (pdata->register_cb)
		pdata->register_cb(&ddata->callbacks);

	i2c_set_clientdata(client, ddata);

	input = input_allocate_device();
	if (!input) {
		pr_err("[TSP] failed to allocate input device\n");
		ret = -ENOMEM;
		goto err_input_dev_alloc_failed;
	}

	ddata->input = input;
	input_set_drvdata(input, ddata);

#if 0
	input->name = client->driver->driver.name;
#else
	input->name = "sec_touchscreen";
#endif
	input->open = synaptics_ts_open;
	input->close = synaptics_ts_close;

	__set_bit(EV_ABS, input->evbit);
	__set_bit(EV_KEY, input->evbit);
	__set_bit(MT_TOOL_FINGER, input->keybit);
	__set_bit(INPUT_PROP_DIRECT, input->propbit);

	input_mt_init_slots(input, MAX_TOUCH_NUM);
	input_set_abs_params(input, ABS_MT_POSITION_X, 0,
			     pdata->max_x, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0,
			     pdata->max_y, 0, 0);
	input_set_abs_params(input, ABS_MT_PRESSURE, 0,
			     pdata->max_pressure, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0,
			     pdata->max_width, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MINOR, 0,
			     pdata->max_width, 0, 0);

	ret = input_register_device(input);
	if (ret) {
		pr_err("[TSP] failed to register input device\n");
		ret = -ENOMEM;
		goto err_input_register_device_failed;
	}

	mutex_init(&ddata->mutex);
	wake_lock_init(&ddata->wakelock, WAKE_LOCK_SUSPEND, "touch");

	INIT_DELAYED_WORK(&ddata->init_dwork, init_function_data_dwork);
	INIT_DELAYED_WORK(&ddata->resume_dwork, synaptics_ts_resume_dwork);
	INIT_DELAYED_WORK(&ddata->noti_dwork, synaptics_ts_noti_dwork);
	schedule_delayed_work(&ddata->init_dwork, HZ);

	ret = set_tsp_sysfs(ddata);
	if (ret) {
		pr_err("[TSP] failed to register input device\n");
		ret = -ENODEV;
		goto err_make_sysfs_failed;
	}

	return 0;

err_make_sysfs_failed:
	input_unregister_device(input);
err_input_register_device_failed:
	input_free_device(input);
err_input_dev_alloc_failed:
err_pdata:
	kfree(ddata);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;
}

static int synaptics_ts_remove(struct i2c_client *client)
{
	struct synaptics_drv_data *data = i2c_get_clientdata(client);

	unregister_early_suspend(&data->early_suspend);
	free_irq(client->irq, data);
	remove_tsp_sysfs(data);
	input_unregister_device(data->input);
	kfree(data);
	return 0;
}

static const struct i2c_device_id synaptics_ts_id[] = {
	{SYNAPTICS_TS_NAME, 0},
	{}
};

static struct i2c_driver synaptics_ts_driver = {
	.driver = {
		   .name = SYNAPTICS_TS_NAME,
	},
	.id_table = synaptics_ts_id,
	.probe = synaptics_ts_probe,
	.remove = __devexit_p(synaptics_ts_remove),
};

static int __devinit synaptics_ts_init(void)
{
	return i2c_add_driver(&synaptics_ts_driver);
}

static void __exit synaptics_ts_exit(void)
{
	i2c_del_driver(&synaptics_ts_driver);
}

module_init(synaptics_ts_init);
module_exit(synaptics_ts_exit);

MODULE_AUTHOR("junki671.min@samsung.com");
MODULE_DESCRIPTION("Driver for Synaptics S7301 Touchscreen Controller");
MODULE_LICENSE("GPL");

