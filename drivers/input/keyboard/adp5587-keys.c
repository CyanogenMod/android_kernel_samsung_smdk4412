/*
 * File: drivers/input/keyboard/adp5587_keys.c
 * Description:  keypad driver for ADP5587 with slide HALL IC
 *		 I2C QWERTY Keypad and IO Expander
 * Bugs: Enter bugs at http://blackfin.uclinux.org/
 *
 * Copyright (C) 2008-2010 Analog Devices Inc.
 * Licensed under the GPL-2 or later.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/errno.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/i2c/adp5587.h>

/* Key Event Register xy */
#define KEY_EV_PRESSED		(1 << 7)
#define KEY_EV_MASK		(0x7F)

#define KP_SEL(x)		(0xFFFF >> (16 - x))	/* 2^x-1 */

#define KEYP_MAX_EVENT		10

/*
 * Early pre 4.0 Silicon required to delay readout by at least 25ms,
 * since the Event Counter Register updated 25ms after the interrupt
 * asserted.
 */
#define WA_DELAYED_READOUT_REVID(rev)		((rev) < 4)

struct adp5587_kpad {
	struct i2c_client *client;
	struct input_dev *input;
	struct delayed_work work;
	unsigned long delay;
	unsigned short keycode[ADP5587_KEYMAPSIZE];
	const struct adp5587_gpi_map *gpimap;
	unsigned short gpimapsize;
#ifdef CONFIG_GPIOLIB
	unsigned char gpiomap[ADP5587_MAXGPIO];
	bool export_gpio;
	struct gpio_chip gc;
	struct mutex gpio_lock;	/* Protect cached dir, dat_out */
	u8 dat_out[3];
	u8 dir[3];
#endif
	int hall_irq;
	bool led_cmd;
};

extern struct class *sec_class;
struct device *sec_keypad;

static int adp5587_read(struct i2c_client *client, u8 reg)
{
	int ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "Read Error\n");

	return ret;
}

static int adp5587_write(struct i2c_client *client, u8 reg, u8 val)
{
	return i2c_smbus_write_byte_data(client, reg, val);
}

#ifdef CONFIG_GPIOLIB
static int adp5587_gpio_get_value(struct gpio_chip *chip, unsigned off)
{
	struct adp5587_kpad *kpad = container_of(chip, struct adp5587_kpad, gc);
	unsigned int bank = ADP5587_BANK(kpad->gpiomap[off]);
	unsigned int bit = ADP5587_BIT(kpad->gpiomap[off]);

	return !!(adp5587_read(kpad->client, GPIO_DAT_STAT1 + bank) & bit);
}

static void adp5587_gpio_set_value(struct gpio_chip *chip,
				   unsigned off, int val)
{
	struct adp5587_kpad *kpad = container_of(chip, struct adp5587_kpad, gc);
	unsigned int bank = ADP5587_BANK(kpad->gpiomap[off]);
	unsigned int bit = ADP5587_BIT(kpad->gpiomap[off]);

	mutex_lock(&kpad->gpio_lock);

	if (val)
		kpad->dat_out[bank] |= bit;
	else
		kpad->dat_out[bank] &= ~bit;

	adp5587_write(kpad->client, GPIO_DAT_OUT1 + bank,
			   kpad->dat_out[bank]);

	mutex_unlock(&kpad->gpio_lock);
}

static int adp5587_gpio_direction_input(struct gpio_chip *chip, unsigned off)
{
	struct adp5587_kpad *kpad = container_of(chip, struct adp5587_kpad, gc);
	unsigned int bank = ADP5587_BANK(kpad->gpiomap[off]);
	unsigned int bit = ADP5587_BIT(kpad->gpiomap[off]);
	int ret;

	mutex_lock(&kpad->gpio_lock);

	kpad->dir[bank] &= ~bit;
	ret = adp5587_write(kpad->client, GPIO_DIR1 + bank, kpad->dir[bank]);

	mutex_unlock(&kpad->gpio_lock);

	return ret;
}

static int adp5587_gpio_direction_output(struct gpio_chip *chip,
					 unsigned off, int val)
{
	struct adp5587_kpad *kpad = container_of(chip, struct adp5587_kpad, gc);
	unsigned int bank = ADP5587_BANK(kpad->gpiomap[off]);
	unsigned int bit = ADP5587_BIT(kpad->gpiomap[off]);
	int ret;

	mutex_lock(&kpad->gpio_lock);

	kpad->dir[bank] |= bit;

	if (val)
		kpad->dat_out[bank] |= bit;
	else
		kpad->dat_out[bank] &= ~bit;

	ret = adp5587_write(kpad->client, GPIO_DAT_OUT1 + bank,
				 kpad->dat_out[bank]);
	ret |= adp5587_write(kpad->client, GPIO_DIR1 + bank,
				 kpad->dir[bank]);

	mutex_unlock(&kpad->gpio_lock);

	return ret;
}

static int __devinit adp5587_build_gpiomap(struct adp5587_kpad *kpad,
				const struct adp5587_kpad_platform_data *pdata)
{
	bool pin_used[ADP5587_MAXGPIO];
	int n_unused = 0;
	int i;

	memset(pin_used, 0, sizeof(pin_used));

	for (i = 0; i < pdata->rows; i++)
		pin_used[i] = true;

	for (i = 0; i < pdata->cols; i++)
		pin_used[i + GPI_PIN_COL_BASE - GPI_PIN_BASE] = true;

	for (i = 0; i < kpad->gpimapsize; i++)
		pin_used[kpad->gpimap[i].pin - GPI_PIN_BASE] = true;

	for (i = 0; i < ADP5587_MAXGPIO; i++)
		if (!pin_used[i])
			kpad->gpiomap[n_unused++] = i;

	return n_unused;
}

static int __devinit adp5587_gpio_add(struct adp5587_kpad *kpad)
{
	struct device *dev = &kpad->client->dev;
	const struct adp5587_kpad_platform_data *pdata = dev->platform_data;
	const struct adp5587_gpio_platform_data *gpio_data = pdata->gpio_data;
	int i, error;

	if (!gpio_data)
		return 0;

	kpad->gc.ngpio = adp5587_build_gpiomap(kpad, pdata);
	if (kpad->gc.ngpio == 0) {
		dev_info(dev, "No unused gpios left to export\n");
		return 0;
	}

	kpad->export_gpio = true;

	kpad->gc.direction_input = adp5587_gpio_direction_input;
	kpad->gc.direction_output = adp5587_gpio_direction_output;
	kpad->gc.get = adp5587_gpio_get_value;
	kpad->gc.set = adp5587_gpio_set_value;
	kpad->gc.can_sleep = 1;

	kpad->gc.base = gpio_data->gpio_start;
	kpad->gc.label = kpad->client->name;
	kpad->gc.owner = THIS_MODULE;

	mutex_init(&kpad->gpio_lock);

	error = gpiochip_add(&kpad->gc);
	if (error) {
		dev_err(dev, "gpiochip_add failed, err: %d\n", error);
		return error;
	}

	for (i = 0; i <= ADP5587_BANK(ADP5587_MAXGPIO); i++) {
		kpad->dat_out[i] = adp5587_read(kpad->client,
						GPIO_DAT_OUT1 + i);
		kpad->dir[i] = adp5587_read(kpad->client, GPIO_DIR1 + i);
	}

	if (gpio_data->setup) {
		error = gpio_data->setup(kpad->client,
					 kpad->gc.base, kpad->gc.ngpio,
					 gpio_data->context);
		if (error)
			dev_warn(dev, "setup failed, %d\n", error);
	}

	return 0;
}

static void __devexit adp5587_gpio_remove(struct adp5587_kpad *kpad)
{
	struct device *dev = &kpad->client->dev;
	const struct adp5587_kpad_platform_data *pdata = dev->platform_data;
	const struct adp5587_gpio_platform_data *gpio_data = pdata->gpio_data;
	int error;

	if (!kpad->export_gpio)
		return;

	if (gpio_data->teardown) {
		error = gpio_data->teardown(kpad->client,
					    kpad->gc.base, kpad->gc.ngpio,
					    gpio_data->context);
		if (error)
			dev_warn(dev, "teardown failed %d\n", error);
	}

	error = gpiochip_remove(&kpad->gc);
	if (error)
		dev_warn(dev, "gpiochip_remove failed %d\n", error);
}
#else
static inline int adp5587_gpio_add(struct adp5587_kpad *kpad)
{
	return 0;
}

static inline void adp5587_gpio_remove(struct adp5587_kpad *kpad)
{
}
#endif

static void adp5587_report_events(struct adp5587_kpad *kpad, int ev_cnt)
{
	struct i2c_client *client = kpad->client;

	int i, j;

	for (i = 0; i < ev_cnt; i++) {
		int key = adp5587_read(kpad->client, Key_EVENTA + i);
		int key_val = key & KEY_EV_MASK;

		if (key_val >= GPI_PIN_BASE && key_val <= GPI_PIN_END) {
			for (j = 0; j < kpad->gpimapsize; j++) {
				if (key_val == kpad->gpimap[j].pin) {
					input_report_switch(kpad->input,
							kpad->gpimap[j].sw_evt,
							key & KEY_EV_PRESSED);
					break;
				}
			}
		} else {
			input_report_key(kpad->input,
					 kpad->keycode[key_val - 1],
					 key & KEY_EV_PRESSED);
		}
		dev_err(&client->dev, "key_val : %d, keycode : %d (%d)\n",
			key_val, kpad->keycode[key_val - 1], key & KEY_EV_PRESSED);
	}
}

static void adp5587_work(struct work_struct *work)
{
	struct adp5587_kpad *kpad = container_of(work,
						struct adp5587_kpad, work.work);
	struct i2c_client *client = kpad->client;
	int status, ev_cnt;

	status = adp5587_read(client, INT_STAT);

	if (status & ADP5587_OVR_FLOW_INT)	/* Unlikely and should never happen */
		dev_err(&client->dev, "Event Overflow Error\n");

	if (status & ADP5587_KE_INT) {
		ev_cnt = adp5587_read(client, KEY_LCK_EC_STAT) & ADP5587_KEC;
		if (ev_cnt) {
			adp5587_report_events(kpad, ev_cnt);
			input_sync(kpad->input);
		}
	}
	adp5587_write(client, INT_STAT, status); /* Status is W1C */
}

static irqreturn_t adp5587_irq(int irq, void *handle)
{
	struct adp5587_kpad *kpad = handle;

	/*
	 * use keventd context to read the event fifo registers
	 * Schedule readout at least 25ms after notification for
	 * REVID < 4
	 */

	schedule_delayed_work(&kpad->work, kpad->delay);

	return IRQ_HANDLED;
}

static irqreturn_t slide_hall_irq_handler(int irq, void *handle)
{
	struct adp5587_kpad *kpad = (struct adp5587_kpad *)handle;
	struct device *kdev = &kpad->client->dev;
	struct adp5587_kpad_platform_data *pkpad = kdev->platform_data;

	int lid_status = gpio_get_value(pkpad->hall_gpio);

	if (lid_status) {
		kpad->input->sw[SW_LID] = 1;
		dev_err(kdev, "slide open\n");
	} else {
		kpad->input->sw[SW_LID] = 0;
		dev_err(kdev, "slide close\n");
	}

	input_report_switch(kpad->input, SW_LID, (lid_status ? 0 : 1));
	input_sync(kpad->input);

	return IRQ_HANDLED;
}

static int __devinit adp5587_setup(struct i2c_client *client)
{
	const struct adp5587_kpad_platform_data *pdata = client->dev.platform_data;
	const struct adp5587_gpio_platform_data *gpio_data = pdata->gpio_data;
	int i, ret;
	unsigned char evt_mode1 = 0, evt_mode2 = 0, evt_mode3 = 0;

	ret = adp5587_write(client, KP_GPIO1, KP_SEL(pdata->rows));
	ret |= adp5587_write(client, KP_GPIO2, KP_SEL(pdata->cols) & 0xFF);
	ret |= adp5587_write(client, KP_GPIO3, KP_SEL(pdata->cols) >> 8);

	if (pdata->en_keylock) {
		ret |= adp5587_write(client, UNLOCK1, pdata->unlock_key1);
		ret |= adp5587_write(client, UNLOCK2, pdata->unlock_key2);
		ret |= adp5587_write(client, KEY_LCK_EC_STAT, ADP5587_K_LCK_EN);
	}

	for (i = 0; i < KEYP_MAX_EVENT; i++)
		ret |= adp5587_read(client, Key_EVENTA);

	for (i = 0; i < pdata->gpimapsize; i++) {
		unsigned short pin = pdata->gpimap[i].pin;

		if (pin <= GPI_PIN_ROW_END) {
			evt_mode1 |= (1 << (pin - GPI_PIN_ROW_BASE));
		} else {
			evt_mode2 |= ((1 << (pin - GPI_PIN_COL_BASE)) & 0xFF);
			evt_mode3 |= ((1 << (pin - GPI_PIN_COL_BASE)) >> 8);
		}
	}

	if (pdata->gpimapsize) {
		ret |= adp5587_write(client, GPI_EM1, evt_mode1);
		ret |= adp5587_write(client, GPI_EM2, evt_mode2);
		ret |= adp5587_write(client, GPI_EM3, evt_mode3);
	}

	if (gpio_data) {
		for (i = 0; i <= ADP5587_BANK(ADP5587_MAXGPIO); i++) {
			int pull_mask = gpio_data->pullup_dis_mask;

			ret |= adp5587_write(client, GPIO_PULL1 + i,
				(pull_mask >> (8 * i)) & 0xFF);
		}
	}

	ret |= adp5587_write(client, INT_STAT,
				ADP5587_CMP2_INT | ADP5587_CMP1_INT |
				ADP5587_OVR_FLOW_INT | ADP5587_K_LCK_INT |
				ADP5587_GPI_INT | ADP5587_KE_INT); /* Status is W1C */

	ret |= adp5587_write(client, CFG, ADP5587_INT_CFG |
					  ADP5587_OVR_FLOW_IEN |
					  ADP5587_KE_IEN);

	if (ret < 0) {
		dev_err(&client->dev, "Write Error\n");
		return ret;
	}

	return 0;
}

static ssize_t keypad_led_control(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct adp5587_kpad *kpad = dev_get_drvdata(dev);
	struct i2c_client *client = kpad->client;
	struct device *kdev = &kpad->client->dev;
	struct adp5587_kpad_platform_data *pkpad = kdev->platform_data;
	int data;
	int ret;

	ret = sscanf(buf, "%d", &data);
	if (ret != 1) {
		dev_err(&client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(data == 0 || data == 1)) {
		dev_err(&client->dev, "%s: wrong command(%d)\n",
			__func__, data);
		return count;
	}

	if (data == 1) {
		dev_notice(&client->dev, "led on\n");
		gpio_direction_output(pkpad->led_gpio, 1);
		kpad->led_cmd = true;
	} else {
		dev_notice(&client->dev, "led off\n");
		gpio_direction_output(pkpad->led_gpio, 0);
		kpad->led_cmd = false;
	}
	return count;
}

static ssize_t keypad_brightness(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct adp5587_kpad *kpad = dev_get_drvdata(dev);
	struct i2c_client *client = kpad->client;
	struct device *kdev = &kpad->client->dev;
	struct adp5587_kpad_platform_data *pkpad = kdev->platform_data;
	int data;
	int ret;

	ret = sscanf(buf, "%d", &data);
	if (ret != 1) {
		dev_err(&client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(data == 0 || data == 1)) {
		dev_err(&client->dev, "%s: wrong command(%d)\n",
			__func__, data);
		return count;
	}

	if (data == 1) {
		dev_notice(&client->dev, "led on\n");
		gpio_direction_output(pkpad->led_gpio, 1);
		kpad->led_cmd = true;
	} else {
		dev_notice(&client->dev, "led off\n");
		gpio_direction_output(pkpad->led_gpio, 0);
		kpad->led_cmd = false;
	}
	return count;
}


static DEVICE_ATTR(enabled, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
			keypad_led_control);
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
			keypad_brightness);

static void __devinit adp5587_report_switch_state(struct adp5587_kpad *kpad)
{
	int gpi_stat1 = adp5587_read(kpad->client, GPIO_DAT_STAT1);
	int gpi_stat2 = adp5587_read(kpad->client, GPIO_DAT_STAT2);
	int gpi_stat3 = adp5587_read(kpad->client, GPIO_DAT_STAT3);
	int gpi_stat_tmp, pin_loc;
	int i;

	for (i = 0; i < kpad->gpimapsize; i++) {
		unsigned short pin = kpad->gpimap[i].pin;

		if (pin <= GPI_PIN_ROW_END) {
			gpi_stat_tmp = gpi_stat1;
			pin_loc = pin - GPI_PIN_ROW_BASE;
		} else if ((pin - GPI_PIN_COL_BASE) < 8) {
			gpi_stat_tmp = gpi_stat2;
			pin_loc = pin - GPI_PIN_COL_BASE;
		} else {
			gpi_stat_tmp = gpi_stat3;
			pin_loc = pin - GPI_PIN_COL_BASE - 8;
		}

		if (gpi_stat_tmp < 0) {
			dev_err(&kpad->client->dev,
				"Can't read GPIO_DAT_STAT switch %d default to OFF\n",
				pin);
			gpi_stat_tmp = 0;
		}

		input_report_switch(kpad->input,
				    kpad->gpimap[i].sw_evt,
				    !(gpi_stat_tmp & (1 << pin_loc)));
	}

	input_sync(kpad->input);
}


static int __devinit adp5587_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct adp5587_kpad *kpad;
	struct adp5587_kpad_platform_data *pdata = client->dev.platform_data;
	struct input_dev *input;
	unsigned int revid;
	int ret, i;
	int error;

	if (!i2c_check_functionality(client->adapter,
					I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "SMBUS Byte Data not Supported\n");
		return -EIO;
	}

	if (!pdata) {
		dev_err(&client->dev, "no platform data?\n");
		return -EINVAL;
	}

	if (!pdata->rows || !pdata->cols || !pdata->keymap) {
		dev_err(&client->dev, "no rows, cols or keymap from pdata\n");
		return -EINVAL;
	}

	if (pdata->keymapsize != ADP5587_KEYMAPSIZE) {
		dev_err(&client->dev, "invalid keymapsize\n");
		return -EINVAL;
	}

	if (!pdata->gpimap && pdata->gpimapsize) {
		dev_err(&client->dev, "invalid gpimap from pdata\n");
		return -EINVAL;
	}

	if (pdata->gpimapsize > ADP5587_GPIMAPSIZE_MAX) {
		dev_err(&client->dev, "invalid gpimapsize\n");
		return -EINVAL;
	}

	for (i = 0; i < pdata->gpimapsize; i++) {
		unsigned short pin = pdata->gpimap[i].pin;

		if (pin < GPI_PIN_BASE || pin > GPI_PIN_END) {
			dev_err(&client->dev, "invalid gpi pin data\n");
			return -EINVAL;
		}

		if (pin <= GPI_PIN_ROW_END) {
			if (pin - GPI_PIN_ROW_BASE + 1 <= pdata->rows) {
				dev_err(&client->dev, "invalid gpi row data\n");
				return -EINVAL;
			}
		} else {
			if (pin - GPI_PIN_COL_BASE + 1 <= pdata->cols) {
				dev_err(&client->dev, "invalid gpi col data\n");
				return -EINVAL;
			}
		}
	}

	if (!client->irq) {
		dev_err(&client->dev, "no IRQ?\n");
		return -EINVAL;
	}

	if (!pdata->hall_gpio) {
		dev_err(&client->dev, "no hall gpio\n");
		return -EINVAL;
	}

	kpad = kzalloc(sizeof(*kpad), GFP_KERNEL);
	if (!kpad)
		return -ENOMEM;

	input = input_allocate_device();
	if (!input) {
		error = -ENOMEM;
		goto err_free_mem;
	}

	kpad->client = client;
	kpad->input = input;
	INIT_DELAYED_WORK(&kpad->work, adp5587_work);

	input->name = client->name;
	input->phys = "adp5587-keys/input0";
	input->dev.parent = &client->dev;

	input->id.bustype = BUS_I2C;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	/* input->id.version = revid; */

	input->keycodesize = sizeof(kpad->keycode[0]);
	input->keycodemax = pdata->keymapsize;
	input->keycode = kpad->keycode;

	memcpy(kpad->keycode, pdata->keymap,
		pdata->keymapsize * input->keycodesize);

	kpad->gpimap = pdata->gpimap;
	kpad->gpimapsize = pdata->gpimapsize;

	kpad->hall_irq = gpio_to_irq(pdata->hall_gpio);
	if (gpio_get_value(pdata->hall_gpio))
		kpad->input->sw[SW_LID] = 0;
	else
		kpad->input->sw[SW_LID] = 1;

	/* setup input device */
	__set_bit(EV_KEY, input->evbit);

	if (pdata->repeat)
		__set_bit(EV_REP, input->evbit);

	for (i = 0; i < input->keycodemax; i++)
		__set_bit(kpad->keycode[i] & KEY_MAX, input->keybit);
	__clear_bit(KEY_RESERVED, input->keybit);

	if (kpad->gpimapsize)
		__set_bit(EV_SW, input->evbit);
	for (i = 0; i < kpad->gpimapsize; i++)
		__set_bit(kpad->gpimap[i].sw_evt, input->swbit);

	__set_bit(SW_LID, input->swbit);
	__set_bit(EV_SW, input->evbit);
	__set_bit(EV_LED, input->evbit);
	__set_bit(LED_MISC, input->ledbit);

	input_set_drvdata(input, kpad);
	i2c_set_clientdata(client, kpad);

	ret = adp5587_read(client, DEV_ID);
	if (ret < 0) {
		error = ret;
		goto err_free_device;
	}

	revid = (u8) ret & ADP5587_DEVICE_ID_MASK;
	if (WA_DELAYED_READOUT_REVID(revid))
		kpad->delay = msecs_to_jiffies(30);
	dev_info(&client->dev, "revid : %d\n", revid);

	error = input_register_device(input);
	if (error) {
		dev_err(&client->dev, "unable to register input device\n");
		goto err_free_device;
	}

	error = adp5587_setup(client);
	if (error)
		goto err_free_register;

	if (kpad->gpimapsize)
		adp5587_report_switch_state(kpad);

	error = adp5587_gpio_add(kpad);
	if (error)
		goto err_free_register;

	if (device_create_file(&input->dev, &dev_attr_enabled) < 0)
		pr_err("[TSP] Failed to create device file(%s)!\n",
		       dev_attr_enabled.attr.name);

	sec_keypad = device_create(sec_class,
			NULL, 0, kpad, "sec_keypad");
	if (IS_ERR(sec_keypad)) {
		dev_err(&client->dev,
			"Failed to create device for the sysfs\n");
		error = -ENODEV;
		goto err_free_register;
	}

	if (device_create_file(sec_keypad, &dev_attr_brightness) < 0 )
		pr_err("[TSP] Failed to create device file(%s)!\n",
			   dev_attr_brightness.attr.name);

	error = request_irq(client->irq, adp5587_irq,
			IRQF_TRIGGER_FALLING | IRQF_DISABLED,
			client->dev.driver->name, kpad);
	if (error) {
		dev_err(&client->dev, "irq %d busy?\n", client->irq);
		goto err_free_mem;
	}

	error = request_threaded_irq(kpad->hall_irq,
		NULL, slide_hall_irq_handler,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_DISABLED,
		"slide_irq", kpad);
	if (error) {
		dev_err(&client->dev, "slide irq failed...\n");
		goto err_free_irq;
	}

	dev_info(&client->dev, "Rev.%d keypad, irq %d\n", revid, client->irq);

	device_init_wakeup(&client->dev, 1);

	return 0;

err_free_irq:
	free_irq(client->irq, kpad);
err_free_register:
	input_unregister_device(kpad->input);
	input = NULL;
err_free_device:
	input_free_device(input);
err_free_mem:
	kfree(kpad);

	return error;
}

static int __devexit adp5587_remove(struct i2c_client *client)
{
	struct adp5587_kpad *kpad = i2c_get_clientdata(client);

	adp5587_write(client, CFG, 0);
	free_irq(client->irq, kpad);
	free_irq(kpad->hall_irq, kpad);
	cancel_delayed_work_sync(&kpad->work);
	input_unregister_device(kpad->input);
	adp5587_gpio_remove(kpad);
	kfree(kpad);

	return 0;
}

#ifdef CONFIG_PM
static int adp5587_suspend(struct device *dev)
{
	struct adp5587_kpad *kpad = dev_get_drvdata(dev);
	struct i2c_client *client = kpad->client;
	struct device *kdev = &kpad->client->dev;
	struct adp5587_kpad_platform_data *pkpad = kdev->platform_data;

	disable_irq(client->irq);
	disable_irq(kpad->hall_irq);
	cancel_delayed_work_sync(&kpad->work);

	if (kpad->led_cmd == true) {
		dev_notice(&client->dev, "led force off\n");
		gpio_direction_output(pkpad->led_gpio, 0);
		kpad->led_cmd = false;
	}

	if (device_may_wakeup(&client->dev)) {
		enable_irq_wake(client->irq);
		enable_irq_wake(kpad->hall_irq);
	}

	return 0;
}

static int adp5587_resume(struct device *dev)
{
	struct adp5587_kpad *kpad = dev_get_drvdata(dev);
	struct i2c_client *client = kpad->client;

	if (device_may_wakeup(&client->dev)) {
		disable_irq_wake(client->irq);
		disable_irq_wake(kpad->hall_irq);
	}
	enable_irq(client->irq);
	enable_irq(kpad->hall_irq);

	return 0;
}

static const struct dev_pm_ops adp5587_dev_pm_ops = {
	.suspend = adp5587_suspend,
	.resume  = adp5587_resume,
};
#endif

static const struct i2c_device_id adp5587_id[] = {
	{ "sec_keypad", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, adp5587_id);

static struct i2c_driver adp5587_driver = {
	.driver = {
		.name = KBUILD_MODNAME,
#ifdef CONFIG_PM
		.pm   = &adp5587_dev_pm_ops,
#endif
	},
	.probe    = adp5587_probe,
	.remove   = __devexit_p(adp5587_remove),
	.id_table = adp5587_id,
};

static int __init adp5587_init(void)
{
	return i2c_add_driver(&adp5587_driver);
}
module_init(adp5587_init);

static void __exit adp5587_exit(void)
{
	i2c_del_driver(&adp5587_driver);
}
module_exit(adp5587_exit);

MODULE_DESCRIPTION("ADP5587 Keypad driver");
MODULE_LICENSE("GPL");
