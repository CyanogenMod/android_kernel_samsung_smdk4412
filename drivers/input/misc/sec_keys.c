/*
 * drivers/input/misc/sec_keys.c
 *
 * Copyright (c) 2013 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/sec_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/irqdesc.h>
#ifdef CONFIG_FAST_BOOT
#include <linux/fake_shut_down.h>
#endif

struct sec_jog_key {
	int state_r;
	int state_l;
	int gpio_r;
	u8 error_count;
	bool chattering_check;
};

struct sec_zoom_ring {
	u32 code;
	u32 pre_code;
	int pre_state;
	bool start;
	int reverse_check;
};

struct sec_keys_drvdata {
	struct input_dev *input;
	struct sec_keys_platform_data *pdata;
	struct device *dev;
	struct device *halldev;
	struct mutex lock;
	struct list_head key_list_head;
	struct sec_jog_key buff_jkey;
	struct sec_zoom_ring buff_zkey;
	struct delayed_work zoomring_dwork;
	struct delayed_work zooming_check_dwork;
	struct timeval tv;
	struct timeval tv_speed;
	long unsigned int timediff;
	u32 n_buttons;
	u8 zoom_speed;
	u8 zoom_speed_pre;
	u8 zoom_count;
	bool zoom_speed_report;
	bool esd_check;
	bool suspend_flag;
	void *data;
};

extern struct class *sec_class;

static ssize_t key_pressed_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sec_keys_drvdata *ddata = dev_get_drvdata(dev);
	struct sec_keys_info *key_info = NULL;
	int keystate = 0;

	list_for_each_entry(key_info, &ddata->key_list_head, list) {
		if (key_info->code != SW_FLIP)
			keystate |= key_info->key_state;
	}

	if (keystate)
		sprintf(buf, "PRESS");
	else
		sprintf(buf, "RELEASE");

	return strlen(buf);
}

/* the volume keys can be the wakeup keys in special case */
static ssize_t wakeup_enable(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_keys_drvdata *ddata = dev_get_drvdata(dev);
	struct sec_keys_info *key_info = NULL;
	unsigned long *bits;
	ssize_t error;

	bits = kcalloc(BITS_TO_LONGS(KEY_CNT),
		sizeof(*bits), GFP_KERNEL);
	if (!bits)
		return -ENOMEM;

	error = bitmap_parselist(buf, bits, KEY_CNT);
	if (error)
		goto out;

	list_for_each_entry(key_info, &ddata->key_list_head, list)
		key_info->wakeup = !!(test_bit(key_info->code, bits));
out:
	kfree(bits);
	return count;
}

static ssize_t sec_qwerty_flip_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sec_keys_drvdata *ddata = dev_get_drvdata(dev);
	struct sec_keys_info *key_info = NULL;
	int keystate = 0;
	int i;

	for (i = 0; i < ddata->pdata->nkeys; i++) {
		key_info = &ddata->pdata->key_info[i];
		if ((key_info->type == EV_SW) &&
				(key_info->code == SW_FLIP)) {
			keystate = key_info->key_state;
			break;
		}
	}

	if (keystate)
		sprintf(buf, "OPEN");
	else
		sprintf(buf, "CLOSE");

	return strlen(buf);
}

static DEVICE_ATTR(sec_key_pressed, S_IRUGO, key_pressed_show, NULL);
static DEVICE_ATTR(wakeup_keys, 0664, NULL, wakeup_enable);
static DEVICE_ATTR(sec_qwerty_flip_pressed, S_IRUGO, sec_qwerty_flip_show, NULL);

static struct attribute *sec_key_attrs[] = {
	&dev_attr_sec_key_pressed.attr,
	&dev_attr_wakeup_keys.attr,
	NULL,
};

static struct attribute *sec_qwerty_flip_attrs[] = {
	&dev_attr_sec_qwerty_flip_pressed.attr,
	NULL,
};

static struct attribute_group sec_key_attr_group = {
	.attrs = sec_key_attrs,
};

static struct attribute_group sec_qwerty_flip_attr_group = {
	.attrs = sec_qwerty_flip_attrs,
};

static bool check_key_type(u8 type)
{
	bool ret = false;
	switch (type) {
	case KEY_ACT_TYPE_LOW:
	case KEY_ACT_TYPE_HIGH:
		ret = false;
		break;

	case KEY_ACT_TYPE_ONE_CH:
	case KEY_ACT_TYPE_JOG_KEY:
	case KEY_ACT_TYPE_ZOOM_RING:
		ret = true;
		break;
	}

	return ret;
}

static void zoomring_init(struct sec_keys_drvdata *ddata, bool esd)
{
	struct input_dev *input = ddata->input;

	if (ddata->buff_zkey.start) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		if (ddata->buff_zkey.code == KEY_ZOOM_RING_IN)
			printk(KERN_DEBUG "[sec_keys] ZOOM_IN release\n");
		else
			printk(KERN_DEBUG "[sec_keys] ZOOM_OUT release\n");
#endif
		input_report_key(input, ddata->buff_zkey.code, 0);
		input_sync(input);
		if (!esd)
			schedule_delayed_work(&ddata->zooming_check_dwork,
				msecs_to_jiffies(1500));
	} else {
		ddata->buff_zkey.code = 0;
		ddata->buff_zkey.pre_code = 0;
		ddata->buff_zkey.pre_state = 0;
	}

	if (esd) {
		ddata->buff_zkey.code = 0;
		ddata->buff_zkey.pre_code = 0;
		ddata->buff_zkey.pre_state = 0;
	}
	ddata->buff_zkey.start = false;
	ddata->buff_zkey.reverse_check = 0;
	ddata->esd_check = false;
	ddata->zoom_count = 0;
	ddata->zoom_speed = 0;
	ddata->zoom_speed_pre = 0;
	ddata->tv_speed.tv_sec = 0;
	ddata->tv_speed.tv_usec = 0;
	ddata->zoom_speed_report = false;
}

static void reprot_zoomring_speed(struct sec_keys_drvdata *ddata, bool down)
{
	struct input_dev *input = ddata->input;

	if (ddata->zoom_speed == 0)
		return;

	if (down) {
		if (ddata->zoom_speed == 1)
			input_report_key(input, KEY_ZOOM_RING_SPEED1, 1);
		else if (ddata->zoom_speed == 2)
			input_report_key(input, KEY_ZOOM_RING_SPEED2, 1);
		else
			input_report_key(input, KEY_ZOOM_RING_SPEED3, 1);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		printk(KERN_ERR"[sec_keys] reprot_zoomring_speed press(%d)\n",
			ddata->zoom_speed);
#endif
	} else {
		if (ddata->zoom_speed == 1)
			input_report_key(input, KEY_ZOOM_RING_SPEED1, 0);
		else if (ddata->zoom_speed == 2)
			input_report_key(input, KEY_ZOOM_RING_SPEED2, 0);
		else
			input_report_key(input, KEY_ZOOM_RING_SPEED3, 0);
		ddata->zoom_speed_report = false;
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		printk(KERN_ERR"[sec_keys] reprot_zoomring_speed release(%d)\n",
			ddata->zoom_speed);
#endif
	}
}

static void check_zoomring_reverse(struct sec_keys_info *key_info,
		u32 code, int state)
{
	struct input_dev *input = key_info->input;
	struct sec_keys_drvdata *ddata = input_get_drvdata(input);

	if (ddata->buff_zkey.reverse_check) {
		if (ddata->buff_zkey.reverse_check == 1)
			ddata->buff_zkey.reverse_check = 2;
		else
			ddata->buff_zkey.reverse_check = 1;
	} else {
		ddata->buff_zkey.reverse_check = 1;
	}
}

static void check_zoomring_direction(struct sec_keys_info *key_info,
		int state)
{
	struct input_dev *input = key_info->input;
	struct sec_keys_drvdata *ddata = input_get_drvdata(input);

	if (ddata->buff_zkey.pre_code == KEY_ZOOM_RING_IN) {
		if (ddata->buff_zkey.pre_state == 1) {
			if (state == 1) {
				ddata->buff_zkey.code = KEY_ZOOM_RING_OUT;
			} else {
				ddata->buff_zkey.code = KEY_ZOOM_RING_IN;
			}
		} else {
			if (state == 1) {
				ddata->buff_zkey.code = KEY_ZOOM_RING_IN;
			} else {
				ddata->buff_zkey.code = KEY_ZOOM_RING_OUT;
			}
		}
	} else { /* KEY_ZOOM_RING_OUT */
		if (ddata->buff_zkey.pre_state == 1) {
			if (state == 1) {
				ddata->buff_zkey.code = KEY_ZOOM_RING_IN;
			} else {
				ddata->buff_zkey.code = KEY_ZOOM_RING_OUT;
			}
		} else {
			if (state == 1) {
				ddata->buff_zkey.code = KEY_ZOOM_RING_OUT;
			} else {
				ddata->buff_zkey.code = KEY_ZOOM_RING_IN;
			}
		}
	}
}

static void report_zoomring_event(struct sec_keys_info *key_info,
	u32 code, int state)
{
	struct input_dev *input = key_info->input;
	struct sec_keys_drvdata *ddata = input_get_drvdata(input);

	cancel_delayed_work_sync(&ddata->zoomring_dwork);
	if (ddata->buff_zkey.start) {
		if (ddata->buff_zkey.pre_code == code) { /* reverse */
			check_zoomring_reverse(key_info, code, state);
		} else {
			if (ddata->buff_zkey.reverse_check) {
				if (ddata->buff_zkey.reverse_check == 1) { /* reverse */
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
					printk(KERN_DEBUG "[sec_keys] zoom reverse\n");
#endif
					input_report_key(input, ddata->buff_zkey.code, 0);
					input_sync(input);

					if (ddata->buff_zkey.code == KEY_ZOOM_RING_IN)
						ddata->buff_zkey.code = KEY_ZOOM_RING_OUT;
					else
						ddata->buff_zkey.code = KEY_ZOOM_RING_IN;

					input_report_key(input, ddata->buff_zkey.code, 1);
					input_sync(input);
					ddata->zoom_count = 1;
					ddata->zoom_speed = 0;
					ddata->zoom_speed_pre = 0;
					ddata->tv_speed.tv_sec = 0;
					ddata->tv_speed.tv_usec = 0;
					ddata->zoom_speed_report = false;
				} else {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
					printk(KERN_DEBUG "[sec_keys] skip zoom reverse\n");
#endif
				}
				ddata->buff_zkey.reverse_check = 0;
			} else {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				printk(KERN_DEBUG "[sec_keys] ZOOM_RING_MOVE event\n");
#endif
				input_report_key(input, KEY_ZOOM_RING_MOVE, 1);
				if (ddata->zoom_speed_report)
					reprot_zoomring_speed(ddata, 1);
				input_sync(input);

				input_report_key(input, KEY_ZOOM_RING_MOVE, 0);
				if (ddata->zoom_speed_report)
					reprot_zoomring_speed(ddata, 0);
				input_sync(input);
			}
		}
	} else {
		if (ddata->buff_zkey.pre_code != code) {
			check_zoomring_direction(key_info, state);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			if (ddata->buff_zkey.code == KEY_ZOOM_RING_IN)
				printk(KERN_DEBUG "[sec_keys] ZOOM IN press\n");
			else
				printk(KERN_DEBUG "[sec_keys] ZOOM OUT press\n");
#endif
			input_report_key(input, ddata->buff_zkey.code, 1);
			input_sync(input);
			ddata->buff_zkey.start = true;
		} /* if ddata->buff_zkey.pre_code == code,
			it's reverse berfore start. do nothing */
	}

	ddata->buff_zkey.pre_code = code;
	ddata->buff_zkey.pre_state = state;
	schedule_delayed_work(&ddata->zoomring_dwork,
		key_info->debounce);
}

static int zoom_time_check(struct sec_keys_drvdata *ddata)
{
	struct timeval stamp, diff;
	long unsigned int usec = 0;
	long unsigned int usec_speed = 0;

	if (!ddata->esd_check) {
		ddata->esd_check = true;
		do_gettimeofday(&ddata->tv);
		return 0;
	} else {
		do_gettimeofday(&stamp);
		diff.tv_sec = stamp.tv_sec - ddata->tv.tv_sec;
		if (diff.tv_sec > 0) {
			usec = diff.tv_sec * 1000000
				+ stamp.tv_usec - ddata->tv.tv_usec;
		} else if (diff.tv_sec == 0) {
			usec = stamp.tv_usec - ddata->tv.tv_usec;
		} else {
			usec = 1000000
				+ stamp.tv_usec - ddata->tv.tv_usec;
		}
		ddata->tv.tv_sec = stamp.tv_sec;
		ddata->tv.tv_usec = stamp.tv_usec;
	}

	ddata->timediff = usec;

	if ((ddata->zoom_count == 3) || (ddata->zoom_count == 5)) {
		if (!((ddata->tv_speed.tv_sec == 0) &&
			(ddata->tv_speed.tv_usec == 0))) {
			diff.tv_sec =
				stamp.tv_sec - ddata->tv_speed.tv_sec;
			if (diff.tv_sec > 0) {
				usec_speed = diff.tv_sec * 1000000
					+ stamp.tv_usec - ddata->tv_speed.tv_usec;
			} else if (diff.tv_sec == 0) {
				usec_speed =
					stamp.tv_usec - ddata->tv_speed.tv_usec;
			} else {
				usec_speed = 1000000
					+ stamp.tv_usec - ddata->tv_speed.tv_usec;
			}
			if (usec_speed < 75000)
				ddata->zoom_speed = 1;
			else if (usec_speed < 160000)
				ddata->zoom_speed = 2;
			else
				ddata->zoom_speed = 3;

			if (ddata->zoom_speed_pre != ddata->zoom_speed) {
				ddata->zoom_speed_report = true;
				ddata->zoom_speed_pre = ddata->zoom_speed;
			}
		}
		ddata->tv_speed.tv_sec = stamp.tv_sec;
		ddata->tv_speed.tv_usec = stamp.tv_usec;
	}

	if (usec < 2000)
		return 1;
	else
		return 0;
}

static void do_func_key(struct sec_keys_info *key_info,
	u32 _code, int state)
{
	struct input_dev *input = key_info->input;
	struct sec_keys_drvdata *ddata = input_get_drvdata(input);
	bool report_enable = false;
	u32 code = _code;

	if (ddata->pdata->lpcharge) {
		printk(KERN_DEBUG
			"[sec_keys] lpcharging. discard %s event\n",
			key_info->name);
		return;
	}

	if (ddata->suspend_flag) {
		printk(KERN_DEBUG
			"[sec_keys] suspend. discard %s event\n",
			key_info->name);
		return;
	}

	mutex_lock(&ddata->lock);
	switch (key_info->act_type) {
	case KEY_ACT_TYPE_ONE_CH:
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		printk(KERN_DEBUG "[sec_keys] %s event\n",
			key_info->name);
#endif
		report_enable = true;
		break;

	case KEY_ACT_TYPE_JOG_KEY:
		break;

	case KEY_ACT_TYPE_ZOOM_RING:
		ddata->zoom_count++;
		if (ddata->zoom_count == 7)
			ddata->zoom_count = 3;
		if (zoom_time_check(ddata)) {
			printk(KERN_ERR "[sec_keys] esd_time_check error\n");
			zoomring_init(ddata, true);
			break;
		}
		if (ddata->buff_zkey.pre_code) {
			cancel_delayed_work_sync(&ddata->zooming_check_dwork);
			report_zoomring_event(key_info, code, state);
		} else {
			ddata->buff_zkey.pre_code = code;
			ddata->buff_zkey.pre_state = state;
			schedule_delayed_work(&ddata->zoomring_dwork,
				msecs_to_jiffies(1500));
		}

		break;

	default :
		printk(KERN_ERR "[sec_keys] ignore %d\n",
			key_info->code);
		break;
	}

	if (report_enable) {
		input_report_key(input, code, 1);
		input_sync(input);
		input_report_key(input, code, 0);
		input_sync(input);
	}

	mutex_unlock(&ddata->lock);
}

static void sec_keys_report_event(struct sec_keys_info *key_info, bool resume)
{
	struct input_dev *input = key_info->input;
	struct irq_desc *desc = irq_to_desc(gpio_to_irq(key_info->gpio));
	bool wakeup = irqd_is_wakeup_set(&desc->irq_data);
	int state = gpio_get_value_cansleep(key_info->gpio);
	u32 type = key_info->type ?: EV_KEY;

	if (!key_info->open)
		return ;

	if (check_key_type(key_info->act_type))
		do_func_key(key_info, key_info->code, state);
	else {
		state ^= key_info->act_type;

		key_info->key_state = !!state;

		if (wakeup)
			state = true;
#ifdef CONFIG_FAST_BOOT
		if (fake_shut_down && (key_info->code == KEY_POWER)) {
			if (resume) {
				printk(KERN_DEBUG "[sec_keys] discard powerkey\n");
			} else {
				printk(KERN_DEBUG "[sec_keys] KEY_FAKE_PWR report\n");
				input_event(input, type, KEY_FAKE_PWR, true);
				input_sync(input);
				input_event(input, type, KEY_FAKE_PWR, false);
			}
		} else
#endif
	/*
	 *	the full shutter should be pressed
	 *	when the half shutter pressed
	*/
#if defined(CONFIG_MACH_GC2PD)
		if ((KEY_CAMERA_SHUTTER == key_info->code) &&
			!!state) {
			input_event(input, type, KEY_CAMERA_FOCUS, true);
			input_sync(input);
		}

		if ((KEY_CAMERA_FOCUS == key_info->code) &&
			!state) {
			input_event(input, type, KEY_CAMERA_SHUTTER, false);
			input_sync(input);
		}
#endif

		input_event(input, type, key_info->code, !!state);
		input_sync(input);

		if (key_info->act_type == KEY_ACT_TYPE_HIGH) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			printk(KERN_DEBUG "[sec_keys] %s %s %s\n",
				key_info->name, state ? "Release" : "Press",
				wakeup ? "wake" : "");
#else
			if (key_info->code == KEY_POWER)
				printk(KERN_DEBUG "PWR %s\n", state ? "R" : "P");
#endif
		} else {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			printk(KERN_DEBUG "[sec_keys] %s %s %s\n",
				key_info->name, state ? "Press" : "Release",
				wakeup ? "wake" : "");
#else
			if (key_info->code == KEY_POWER)
				printk(KERN_DEBUG "PWR %s\n", state ? "P" : "R");
#endif
		}
	}
}

static int sec_keys_event(struct input_dev *dev,
			unsigned int type, unsigned int code, int value)
{
#if 0
	struct sec_keys_drvdata *data = input_get_drvdata(dev);

	switch (type) {
		printk(KERN_DEBUG "[Keyboard] %s, capslock on led value=%d\n",\
			 __func__, value);
		return 0;
	}
	return -ENOEXEC;
#endif
	return 0;
}

static void sec_keys_zooming_check(struct work_struct *work)
{
	struct sec_keys_drvdata *ddata = container_of(work,
			struct sec_keys_drvdata, zooming_check_dwork.work);
	struct input_dev *input = ddata->input;

	ddata->buff_zkey.pre_code = 0;
	ddata->buff_zkey.pre_state = 0;
}

static void sec_keys_zoomring(struct work_struct *work)
{
	struct sec_keys_drvdata *ddata = container_of(work,
			struct sec_keys_drvdata, zoomring_dwork.work);
	struct input_dev *input = ddata->input;

	zoomring_init(ddata, false);
}

static void sec_keys_jogkey(struct work_struct *work)
{
	struct sec_keys_info *key_info = container_of(work,
			struct sec_keys_info, dwork.work);
	struct input_dev *input = key_info->input;
	struct sec_keys_drvdata *ddata = input_get_drvdata(input);

	int state_l;
	u32 code;

	state_l = gpio_get_value(key_info->gpio);
	if (state_l == ddata->buff_jkey.state_l) {
		if (ddata->buff_jkey.error_count < 5) {
			printk(KERN_ERR "[sec_keys] jog state_l err (pre:%d, now:%d)\n",
				ddata->buff_jkey.state_l, state_l);
			schedule_delayed_work(&key_info->dwork,
				msecs_to_jiffies(10));
			ddata->buff_jkey.error_count++;
		} else {
			printk(KERN_ERR "[sec_keys] jog state_l err count stop (%d)\n",
				ddata->buff_jkey.error_count);
			ddata->buff_jkey.error_count = 0;
		}
		return;
	}

	ddata->buff_jkey.state_l = state_l;

	if (state_l) {
		if (ddata->buff_jkey.state_r)
			code = KEY_CAMERA_JOG_L;
		else
			code = KEY_CAMERA_JOG_R;
	} else {
		if (ddata->buff_jkey.state_r)
			code = KEY_CAMERA_JOG_R;
		else
			code = KEY_CAMERA_JOG_L;
	}
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	printk(KERN_DEBUG "[sec_keys] %s event\n",
		(code == KEY_CAMERA_JOG_L) ?
		"Left" : "Right");
#endif
	input_report_key(input, code, 1);
	input_sync(input);
	input_report_key(input, code, 0);
	input_sync(input);

	ddata->buff_jkey.chattering_check = false;
	ddata->buff_jkey.error_count = 0;
}

static void sec_keys_debounce(struct work_struct *work)
{
	struct sec_keys_info *key_info = container_of(work,
			struct sec_keys_info, dwork.work);

	sec_keys_report_event(key_info, false);
}

static irqreturn_t sec_keys_isr(int irq, void *dev_id)
{
	struct sec_keys_info *key_info = dev_id;
	struct irq_desc *desc = irq_to_desc(gpio_to_irq(key_info->gpio));
	bool wakeup = irqd_is_wakeup_set(&desc->irq_data);

	if (!check_key_type(key_info->act_type) && !wakeup) {
		if ((key_info->type == EV_SW) &&
				(key_info->code == SW_FLIP))
			cancel_delayed_work_sync(&key_info->dwork);
		schedule_delayed_work(&key_info->dwork,
			key_info->debounce);
	} else {
		if ((key_info->act_type == KEY_ACT_TYPE_JOG_KEY) &&
				(key_info->code == KEY_CAMERA_JOG_L)) {
			struct input_dev *input = key_info->input;
			struct sec_keys_drvdata *ddata =
				input_get_drvdata(input);
			if (ddata->suspend_flag) {
				printk(KERN_DEBUG
					"[sec_keys] suspend. discard %s event\n",
					key_info->name);
				return IRQ_HANDLED;
			}

			ddata->buff_jkey.state_r =
				gpio_get_value(ddata->buff_jkey.gpio_r);
			if (ddata->buff_jkey.chattering_check) {
				cancel_delayed_work_sync(&key_info->dwork);
				schedule_delayed_work(&key_info->dwork,
					msecs_to_jiffies(10));
			} else {
				ddata->buff_jkey.chattering_check = true;
				schedule_delayed_work(&key_info->dwork,
					msecs_to_jiffies(10));
			}
		} else
		sec_keys_report_event(key_info, false);
	}
	return IRQ_HANDLED;
}

static int __devinit sec_keys_setup_key(struct sec_keys_drvdata *ddata,
					 struct sec_keys_info *key_info)
{
	const char *desc = key_info->name ? key_info->name : "sec_keys";
	struct input_dev *input = ddata->input;
	int irq = 0, ret = 0;
	u32 type = key_info->type ?: EV_KEY;
	u32 gpio = key_info->gpio;
	u64 irqflags = 0;

	if ((key_info->act_type == KEY_ACT_TYPE_JOG_KEY) &&
		(key_info->code == KEY_CAMERA_JOG_R)) {
		input_set_capability(input, type, key_info->code);
		ddata->buff_jkey.gpio_r = key_info->gpio;
		return 0;
	}

	if (key_info->gpio_init)
		key_info->gpio_init(gpio, desc);
	else {
		ret = gpio_request(gpio, desc);
		if (ret < 0) {
			printk(KERN_ERR "[sec_keys] failed to request GPIO %d, ret %d\n",
				gpio, ret);
			goto fail2;
		}

		ret = gpio_direction_input(gpio);
		if (ret < 0) {
			printk(KERN_ERR "[sec_keys] failed to configure"
				" direction for GPIO %d, ret %d\n",
				gpio, ret);
			goto fail3;
		}
	}

	irq = gpio_to_irq(gpio);
	if (irq < 0) {
		ret = irq;
		printk(KERN_ERR "[sec_keys] Unable to get irq number for GPIO %s, ret %d\n",
			key_info->name, ret);
		goto fail3;
	}

	if (key_info->act_type == KEY_ACT_TYPE_ONE_CH)
		irqflags = IRQF_TRIGGER_RISING;
	else
		irqflags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;

	if (key_info->wakeup)
		irqflags |= IRQF_NO_SUSPEND;

	input_set_capability(input, type, key_info->code);

	if (key_info->act_type == KEY_ACT_TYPE_ZOOM_RING)
		INIT_DELAYED_WORK(&key_info->dwork, sec_keys_zoomring);
	else if (key_info->act_type == KEY_ACT_TYPE_JOG_KEY) {
		ddata->buff_jkey.state_l = gpio_get_value(key_info->gpio);
		ddata->buff_jkey.error_count = 0;
		INIT_DELAYED_WORK(&key_info->dwork, sec_keys_jogkey);
	} else
		INIT_DELAYED_WORK(&key_info->dwork, sec_keys_debounce);

	ret = request_threaded_irq(irq, NULL, sec_keys_isr,
		irqflags, desc, key_info);
	if (ret < 0) {
		printk(KERN_ERR "[sec_keys] Unable to claim irq %d; ret %d\n",
			irq, ret);
		goto fail3;
	}

	return 0;

fail3:
	gpio_free(gpio);
fail2:
	return ret;
}

static int sec_keys_open(struct input_dev *input)
{
	struct sec_keys_drvdata *ddata = input_get_drvdata(input);
	struct sec_keys_info *key_info = NULL;

	list_for_each_entry(key_info, &ddata->key_list_head, list) {
		key_info->open = true;
		if (!check_key_type(key_info->act_type))
			sec_keys_report_event(key_info, false);
	}

	return 0;
}

static void sec_keys_close(struct input_dev *input)
{
	struct sec_keys_drvdata *ddata = input_get_drvdata(input);
	struct sec_keys_info *key_info = NULL;

	list_for_each_entry(key_info, &ddata->key_list_head, list) {
		u32 type = key_info->type ?: EV_KEY;
		key_info->open = false;
		input_event(input, type, key_info->code, 0);
	}

	input_sync(input);
}

static void sec_keys_make_sysfs_node(struct sec_keys_drvdata *ddata)
{
	int ret;
	int i;

	if (!sec_class)
		return;

	ddata->dev =
	    device_create(sec_class, NULL, 0, ddata, SEC_KEY_PATH);
	if (IS_ERR(ddata->dev))
		printk(KERN_ERR "[sec_keys] failed to create sec_key device\n");
	else {
		ret = sysfs_create_group(&ddata->dev->kobj,
			&sec_key_attr_group);
		if (ret) {
			printk(KERN_ERR
				"[sec_keys] failed to create sec_key sysfs: %d\n",
				ret);
		}
	}

	for (i = 0; i < ddata->pdata->nkeys; i++) {
		struct sec_keys_info *key_info = &ddata->pdata->key_info[i];
		if ((key_info->type == EV_SW) &&
				(key_info->code == SW_FLIP)) {
			ddata->halldev =
				device_create(sec_class, NULL,
					0, ddata, "sec_qwerty_flip");
			ret = sysfs_create_group(&ddata->halldev->kobj,
				&sec_qwerty_flip_attr_group);
			if (ret) {
				printk(KERN_ERR
					"[sec_keys] failed to create sec_qwerty_flip sysfs: %d\n",
					ret);
			}
			break;
		}
	}
}

static int __devinit sec_keys_probe(struct platform_device *pdev)
{
	struct sec_keys_platform_data *pdata = pdev->dev.platform_data;
	struct sec_keys_drvdata *ddata;
	struct device *dev = &pdev->dev;
	struct input_dev *input;
	int i = 0, ret = 0;
	bool wakeup = false;

	if (NULL == pdata) {
		printk(KERN_ERR "[sec_keys] no pdata\n");
		ret = -ENODEV;
		goto err_pdata;
	}

	ddata = kzalloc(sizeof(struct sec_keys_drvdata), GFP_KERNEL);
	if (NULL == ddata) {
		printk(KERN_ERR "[sec_keys] failed to alloc ddata.\n");
		goto err_free_ddata;
	}

	input = input_allocate_device();
	if (NULL == input) {
		printk(KERN_ERR "[sec_keys] failed to allocate input device.\n");
		ret = -ENOMEM;
		goto err_free_input_dev;
	}

	ddata->input = input;
	ddata->pdata = pdata;
	mutex_init(&ddata->lock);
	INIT_LIST_HEAD(&ddata->key_list_head);

	platform_set_drvdata(pdev, ddata);
	input_set_drvdata(input, ddata);

	input->name = pdev->name;
	input->phys = "sec_keys/input0";
	input->dev.parent = &pdev->dev;

	input->open = sec_keys_open;
	input->close = sec_keys_close;
	input->event = sec_keys_event;
	input->id.bustype = BUS_HOST;

	/* Enable auto repeat feature of Linux input subsystem */
	if (pdata->ev_rep)
		__set_bit(EV_REP, input->evbit);

	for (i = 0; i < pdata->nkeys; i++) {
		struct sec_keys_info *key_info = &pdata->key_info[i];

		key_info->input = input;

		ret = sec_keys_setup_key(ddata, key_info);
		if (ret)
			goto err_set_keys;

		if (key_info->wakeup)
			wakeup = true;

		list_add_tail(&key_info->list, &ddata->key_list_head);
	}

	for (i = 0; i < pdata->nkeys; i++) {
		struct sec_keys_info *key_info = &pdata->key_info[i];
		if (key_info->act_type == KEY_ACT_TYPE_ZOOM_RING) {
			INIT_DELAYED_WORK(&ddata->zoomring_dwork, sec_keys_zoomring);
			INIT_DELAYED_WORK(&ddata->zooming_check_dwork,
				sec_keys_zooming_check);
			ddata->buff_zkey.start = false;
			ddata->buff_zkey.reverse_check = 0;
			ddata->zoom_count = 0;
			ddata->zoom_speed = 0;
			ddata->zoom_speed_report = false;
			input_set_capability(input, EV_KEY, KEY_ZOOM_RING_MOVE);
			input_set_capability(input, EV_KEY, KEY_ZOOM_RING_SPEED1);
			input_set_capability(input, EV_KEY, KEY_ZOOM_RING_SPEED2);
			input_set_capability(input, EV_KEY, KEY_ZOOM_RING_SPEED3);
			input_set_capability(input, EV_KEY, KEY_ZOOM_RING_SPEED4);
			break;
		}
	}

#ifdef CONFIG_FAST_BOOT
	input_set_capability(input, EV_KEY, KEY_FAKE_PWR);
#endif

	sec_keys_make_sysfs_node(ddata);

	ret = input_register_device(input);
	if (ret) {
		dev_err(dev, "Unable to register input device, ret: %d\n",
			ret);
		goto err_reg_input;
	}

	device_init_wakeup(&pdev->dev, wakeup);

	return 0;

 err_reg_input:
	sysfs_remove_group(&ddata->dev->kobj, &sec_key_attr_group);
	while (--i >= 0) {
		free_irq(gpio_to_irq(pdata->key_info[i].gpio), &ddata);
		cancel_delayed_work_sync(&pdata->key_info[i].dwork);
		gpio_free(pdata->key_info[i].gpio);
	}
err_set_keys:
	platform_set_drvdata(pdev, NULL);
err_free_input_dev:
	input_free_device(input);
err_free_ddata:
	kfree(ddata);
err_pdata:
	return ret;
}

static int __devexit sec_keys_remove(struct platform_device *pdev)
{
	struct sec_keys_drvdata *ddata = platform_get_drvdata(pdev);
	struct sec_keys_info *key_info = NULL;
	struct input_dev *input = ddata->input;

	sysfs_remove_group(&pdev->dev.kobj, &sec_key_attr_group);
	device_init_wakeup(&pdev->dev, 0);

	list_for_each_entry(key_info, &ddata->key_list_head, list) {
		int irq = gpio_to_irq(key_info->gpio);
		free_irq(irq, &ddata);
		cancel_delayed_work_sync(&key_info->dwork);
		gpio_free(key_info->gpio);
	}

	input_unregister_device(input);

	return 0;
}

#ifdef CONFIG_PM
static int sec_keys_suspend(struct platform_device *pdev,
			pm_message_t state)
{
	struct sec_keys_drvdata *ddata = platform_get_drvdata(pdev);
	struct sec_keys_info *key_info = NULL;

	if (device_may_wakeup(&pdev->dev)) {
		list_for_each_entry(key_info, &ddata->key_list_head, list) {

			if (key_info->code == SW_FLIP)
				key_info->wakeup = 1;

			if ((key_info->type == KEY_ACT_TYPE_JOG_KEY) &&
				(key_info->code == KEY_CAMERA_JOG_L))
				cancel_delayed_work_sync(&key_info->dwork);

			if (key_info->wakeup) {
				int irq = gpio_to_irq(key_info->gpio);
				enable_irq_wake(irq);
			}
		}
	}
	ddata->suspend_flag = true;
	return 0;
}

static int sec_keys_resume(struct platform_device *pdev)
{
	struct sec_keys_drvdata *ddata = platform_get_drvdata(pdev);
	struct sec_keys_info *key_info = NULL;

	list_for_each_entry(key_info, &ddata->key_list_head, list) {

		if (key_info->code == SW_FLIP)
			key_info->wakeup = 1;

		if (key_info->wakeup && device_may_wakeup(&pdev->dev)) {
			int irq = gpio_to_irq(key_info->gpio);
			disable_irq_wake(irq);
		}

		if (!check_key_type(key_info->act_type))
			sec_keys_report_event(key_info, true);

		if ((key_info->act_type == KEY_ACT_TYPE_JOG_KEY) &&
			(key_info->code == KEY_CAMERA_JOG_L)) {
			ddata->buff_jkey.state_l =
				gpio_get_value(key_info->gpio);

		}
	}
	ddata->suspend_flag = false;

	return 0;
}
#endif	/* CONFIG_PM */

static struct platform_driver sec_keys_device_driver = {
	.probe		= sec_keys_probe,
	.remove		= __devexit_p(sec_keys_remove),
#ifdef CONFIG_PM
	.suspend		= sec_keys_suspend,
	.resume		= sec_keys_resume,
#endif	/* CONFIG_PM */
	.driver		= {
		.name	= "sec_keys",
		.owner	= THIS_MODULE,
	}
};

static int __init sec_keys_init(void)
{
	return platform_driver_register(&sec_keys_device_driver);
}

static void __exit sec_keys_exit(void)
{
	platform_driver_unregister(&sec_keys_device_driver);
}

module_init(sec_keys_init);
module_exit(sec_keys_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junki Min <junki671.min@samsung.com>");
MODULE_DESCRIPTION("key driver for sec product");
