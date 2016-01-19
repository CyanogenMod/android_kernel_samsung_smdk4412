/* drivers/motor/isa1200_vibrator.c
 *
 * Copyright (C) 2011 Samsung Electronics Co. Ltd. All Rights Reserved.
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

#include <linux/hrtimer.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/pwm.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/timed_output.h>
#include <linux/isa1200_vibrator.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#if 0
#define MOTOR_DEBUG
#endif

struct isa1200_vibrator_drvdata {
	struct timed_output_dev dev;
	struct hrtimer timer;
	struct work_struct work;
	struct clk *vib_clk;
	struct pwm_device	*pwm;
	struct i2c_client *client;
	int (*gpio_en) (bool) ;
	spinlock_t lock;
	bool running;
	int timeout;
	int max_timeout;
	u8 ctrl0;
	u8 ctrl1;
	u8 ctrl2;
	u8 ctrl4;
	u8 pll;
	u8 duty;
	u8 period;
	int pwm_id;
	u16 pwm_duty;
	u16 pwm_period;
};


static struct isa1200_vibrator_drvdata	*g_drvdata;

static int isa1200_vibrator_i2c_write(struct i2c_client *client,
					u8 addr, u8 val)
{
	int error = 0;
	error = i2c_smbus_write_byte_data(client, addr, val);
	if (error)
		printk(KERN_ERR "[VIB] Failed to write addr=[0x%x], val=[0x%x]\n",
				addr, val);

	return error;
}

int vibtonz_i2c_write(u8 addr, int length, u8 *data)
{
	if (NULL == g_drvdata) {
		printk(KERN_ERR "[VIB] driver is not ready\n");
		return -EFAULT;
	}

	if (2 != length)
		printk(KERN_ERR "[VIB] length should be 2(len:%d)\n",
			length);

#ifdef MOTOR_DEBUG
	printk(KERN_DEBUG "[VIB] data : %x, %x\n",
		data[0], data[1]);
#endif

	return isa1200_vibrator_i2c_write(g_drvdata->client,
		data[0], data[1]);
}

void vibtonz_clk_enable(bool en)
{
	if (NULL == g_drvdata) {
		printk(KERN_ERR "[VIB] driver is not ready\n");
		return ;
	}

	if (en) {
		if (g_drvdata->vib_clk)
			clk_enable(g_drvdata->vib_clk);
		else
			pwm_enable(g_drvdata->pwm);
	} else {
		if (g_drvdata->vib_clk)
			clk_disable(g_drvdata->vib_clk);
		else
			pwm_disable(g_drvdata->pwm);
	}
}

void vibtonz_clk_config(int duty)
{
	u32 duty_ns = 0;

	duty_ns = (u32)(g_drvdata->pwm_period * duty)
		/ 1000;

	pwm_config(g_drvdata->pwm,
		duty_ns,
		g_drvdata->pwm_period);

}

void vibtonz_chip_enable(bool en)
{
	if (NULL == g_drvdata) {
		printk(KERN_ERR "[VIB] driver is not ready\n");
		return ;
	}
	g_drvdata->gpio_en(en ? true : false);
}

static void isa1200_vibrator_hw_init(struct isa1200_vibrator_drvdata *data)
{
	data->gpio_en(true);
	msleep(20);
	isa1200_vibrator_i2c_write(data->client,
		HAPTIC_CONTROL_REG0, data->ctrl0);
	isa1200_vibrator_i2c_write(data->client,
		HAPTIC_CONTROL_REG1, data->ctrl1);
	isa1200_vibrator_i2c_write(data->client,
		HAPTIC_CONTROL_REG2, data->ctrl2);
	isa1200_vibrator_i2c_write(data->client,
		HAPTIC_PLL_REG, data->pll);
	isa1200_vibrator_i2c_write(data->client,
		HAPTIC_CONTROL_REG4, data->ctrl4);
	isa1200_vibrator_i2c_write(data->client,
		HAPTIC_PWM_DUTY_REG, data->period/2);
	isa1200_vibrator_i2c_write(data->client,
		HAPTIC_PWM_PERIOD_REG, data->period);

#ifdef MOTOR_DEBUG
	printk(KERN_DEBUG "[VIB] ctrl0 = 0x%x\n", data->ctrl0);
	printk(KERN_DEBUG "[VIB] ctrl1 = 0x%x\n", data->ctrl1);
	printk(KERN_DEBUG "[VIB] ctrl2 = 0x%x\n", data->ctrl2);
	printk(KERN_DEBUG "[VIB] pll = 0x%x\n", data->pll);
	printk(KERN_DEBUG "[VIB] ctrl4 = 0x%x\n", data->ctrl4);
	printk(KERN_DEBUG "[VIB] duty = 0x%x\n", data->period/2);
	printk(KERN_DEBUG "[VIB] period = 0x%x\n", data->period);
#endif

}

static void isa1200_vibrator_on(struct isa1200_vibrator_drvdata *data)
{
	isa1200_vibrator_i2c_write(data->client,
		HAPTIC_CONTROL_REG0, data->ctrl0 | CTL0_NORMAL_OP);
	isa1200_vibrator_i2c_write(data->client,
		HAPTIC_PWM_DUTY_REG, data->duty - 3);
#ifdef MOTOR_DEBUG
	printk(KERN_DEBUG "[VIB] ctrl0 = 0x%x\n", data->ctrl0 | CTL0_NORMAL_OP);
	printk(KERN_DEBUG "[VIB] duty = 0x%x\n", data->duty);
#endif
}

static void isa1200_vibrator_off(struct isa1200_vibrator_drvdata *data)
{
	isa1200_vibrator_i2c_write(data->client,
		HAPTIC_PWM_DUTY_REG, data->period / 2);
	isa1200_vibrator_i2c_write(data->client,
		HAPTIC_CONTROL_REG0, data->ctrl0);
}

static enum hrtimer_restart isa1200_vibrator_timer_func(struct hrtimer *_timer)
{
	struct isa1200_vibrator_drvdata *data =
		container_of(_timer, struct isa1200_vibrator_drvdata, timer);

	data->timeout = 0;

	schedule_work(&data->work);
	return HRTIMER_NORESTART;
}

static void isa1200_vibrator_work(struct work_struct *_work)
{
	struct isa1200_vibrator_drvdata *data =
		container_of(_work, struct isa1200_vibrator_drvdata, work);

	if (0 == data->timeout) {
		if (!data->running)
			return ;

		data->running = false;
		isa1200_vibrator_off(data);
		vibtonz_clk_config(500);
		vibtonz_clk_enable(false);
	} else {
		if (data->running)
			return ;

		data->running = true;
		vibtonz_clk_config(999);
		vibtonz_clk_enable(true);
		mdelay(1);
		isa1200_vibrator_on(data);
	}
}

static int isa1200_vibrator_get_time(struct timed_output_dev *_dev)
{
	struct isa1200_vibrator_drvdata	*data =
		container_of(_dev, struct isa1200_vibrator_drvdata, dev);

	if (hrtimer_active(&data->timer)) {
		ktime_t r = hrtimer_get_remaining(&data->timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	} else
		return 0;
}

static void isa1200_vibrator_enable(struct timed_output_dev *_dev, int value)
{
	struct isa1200_vibrator_drvdata	*data =
		container_of(_dev, struct isa1200_vibrator_drvdata, dev);
	unsigned long	flags;

#ifdef MOTOR_DEBUG
	printk(KERN_DEBUG "[VIB] time = %dms\n", value);
#endif
	cancel_work_sync(&data->work);
	hrtimer_cancel(&data->timer);
	data->timeout = value;
	schedule_work(&data->work);
	spin_lock_irqsave(&data->lock, flags);
	if (value > 0) {
		if (value > data->max_timeout)
			value = data->max_timeout;

		hrtimer_start(&data->timer,
			ns_to_ktime((u64)value * NSEC_PER_MSEC),
			HRTIMER_MODE_REL);
	}
	spin_unlock_irqrestore(&data->lock, flags);
}

static int __devinit isa1200_vibrator_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct isa1200_vibrator_platform_data *pdata =
		client->dev.platform_data;
	struct isa1200_vibrator_drvdata *ddata;
	int ret = 0;

	ddata = kzalloc(sizeof(struct isa1200_vibrator_drvdata), GFP_KERNEL);
	if (NULL == ddata) {
		printk(KERN_ERR "[VIB] Failed to alloc memory\n");
		ret = -ENOMEM;
		goto err_free_mem;
	}

	ddata->client = client;
	ddata->ctrl0 = pdata->ctrl0;
	ddata->ctrl1 = pdata->ctrl1;
	ddata->ctrl2 = pdata->ctrl2;
	ddata->ctrl4 = pdata->ctrl4;
	ddata->gpio_en = pdata->gpio_en;
	if (pdata->get_clk) {
		ddata->vib_clk = pdata->get_clk();
		if (IS_ERR(ddata->vib_clk)) {
			pr_err("[VIB] Failed to request vib_clk.\n");
			ret = -EFAULT;
			goto err_request_clk;
		}
	} else {
		ddata->vib_clk = NULL;
		ddata->pwm = pwm_request(pdata->pwm_id, "vibrator");
		if (IS_ERR(ddata->pwm)) {
			pr_err("[VIB] Failed to request pwm.\n");
			ret = -EFAULT;
			goto err_request_clk;
		}
		ddata->pwm_id = pdata->pwm_id;
		ddata->pwm_duty = pdata->pwm_duty;
		ddata->pwm_period = pdata->pwm_period;
	}
	ddata->max_timeout = pdata->max_timeout;
	ddata->pll = pdata->pll;
	ddata->duty = pdata->duty;
	ddata->period = pdata->period;

	ddata->dev.name = "vibrator";
	ddata->dev.get_time = isa1200_vibrator_get_time;
	ddata->dev.enable = isa1200_vibrator_enable;

	hrtimer_init(&ddata->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ddata->timer.function = isa1200_vibrator_timer_func;
	INIT_WORK(&ddata->work, isa1200_vibrator_work);
	spin_lock_init(&ddata->lock);

	i2c_set_clientdata(client, ddata);
	isa1200_vibrator_hw_init(ddata);

	ret = timed_output_dev_register(&ddata->dev);
	if (ret < 0) {
		printk(KERN_ERR "[VIB] Failed to register timed_output : -%d\n", ret);
		goto err_to_dev_reg;
	}

	g_drvdata = ddata;

	return 0;

err_to_dev_reg:
err_request_clk:
	kfree(ddata);
err_free_mem:
	return ret;

}

static int isa1200_vibrator_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct isa1200_vibrator_drvdata *ddata  = i2c_get_clientdata(client);
	ddata->gpio_en(false);
	return 0;
}

static int isa1200_vibrator_resume(struct i2c_client *client)
{
	struct isa1200_vibrator_drvdata *ddata  = i2c_get_clientdata(client);
	isa1200_vibrator_hw_init(ddata);
	return 0;
}

static const struct i2c_device_id isa1200_vibrator_device_id[] = {
	{"isa1200_vibrator", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, isa1200_vibrator_device_id);

static struct i2c_driver isa1200_vibrator_i2c_driver = {
	.driver = {
		.name = "isa1200_vibrator",
		.owner = THIS_MODULE,
	},
	.probe     = isa1200_vibrator_i2c_probe,
	.id_table  = isa1200_vibrator_device_id,
	.suspend	= isa1200_vibrator_suspend,
	.resume	= isa1200_vibrator_resume,
};

static int __init isa1200_vibrator_init(void)
{
	return i2c_add_driver(&isa1200_vibrator_i2c_driver);
}

static void __exit isa1200_vibrator_exit(void)
{
	i2c_del_driver(&isa1200_vibrator_i2c_driver);
}

module_init(isa1200_vibrator_init);
module_exit(isa1200_vibrator_exit);
