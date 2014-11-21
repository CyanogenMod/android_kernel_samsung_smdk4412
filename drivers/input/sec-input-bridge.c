/*
 *  sec-input-bridge.c - Specific control input event bridge
 *  for Samsung Electronics
 *
 *  Copyright (C) 2010 Samsung Electronics
 *  Yongsul Oh <yongsul96.oh@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/init.h>

#include <linux/workqueue.h>
#include <linux/mutex.h>

#include <linux/input/sec-input-bridge.h>

struct sec_input_bridge {
	struct sec_input_bridge_platform_data *pdata;
	struct work_struct work;
	struct mutex lock;
	struct platform_device *dev;

	/*
	 * Because this flag size is 32 byte, Max map table number is 32.
	 */
	u32 send_uevent_flag;
	u8 *check_index;
};

static void input_bridge_set_ids(struct input_device_id *ids, unsigned int type,
				 unsigned int code)
{
	switch (type) {
	case EV_KEY:
		ids->flags = INPUT_DEVICE_ID_MATCH_KEYBIT;
		__set_bit(code, ids->keybit);
		break;

	case EV_REL:
		ids->flags = INPUT_DEVICE_ID_MATCH_RELBIT;
		__set_bit(code, ids->relbit);
		break;

	case EV_ABS:
		ids->flags = INPUT_DEVICE_ID_MATCH_ABSBIT;
		__set_bit(code, ids->absbit);
		break;

	case EV_MSC:
		ids->flags = INPUT_DEVICE_ID_MATCH_MSCIT;
		__set_bit(code, ids->mscbit);
		break;

	case EV_SW:
		ids->flags = INPUT_DEVICE_ID_MATCH_SWBIT;
		__set_bit(code, ids->swbit);
		break;

	case EV_LED:
		ids->flags = INPUT_DEVICE_ID_MATCH_LEDBIT;
		__set_bit(code, ids->ledbit);
		break;

	case EV_SND:
		ids->flags = INPUT_DEVICE_ID_MATCH_SNDBIT;
		__set_bit(code, ids->sndbit);
		break;

	case EV_FF:
		ids->flags = INPUT_DEVICE_ID_MATCH_FFBIT;
		__set_bit(code, ids->ffbit);
		break;

	case EV_PWR:
		/* do nothing */
		break;

	default:
		printk(KERN_ERR
		       "input_bridge_set_ids: unknown type %u (code %u)\n",
		       type, code);
		return;
	}

	ids->flags |= INPUT_DEVICE_ID_MATCH_EVBIT;
	__set_bit(type, ids->evbit);
}

static void input_bridge_work(struct work_struct *work)
{
	struct sec_input_bridge *bridge = container_of(work,
						       struct sec_input_bridge,
						       work);
	int state, i;
	char env_str[16];
	char *envp[] = { env_str, NULL };

	mutex_lock(&bridge->lock);

	for (i = 0; i < bridge->pdata->num_map; i++) {
		if (bridge->send_uevent_flag & (1 << i)) {
			if (bridge->pdata->mmap[i].enable_uevent) {
				printk(KERN_ERR
				     "!!!!sec-input-bridge: OK!!, KEY input matched , now send uevent!!!!\n");
				sprintf(env_str, "%s=%s",
					bridge->pdata->mmap[i].uevent_env_str,
					bridge->pdata->mmap[i].
					uevent_env_value);
				printk(KERN_ERR
				    "<kobject_uevent_env for sec-input-bridge>, event: %s\n",
				    env_str);
				state =
				    kobject_uevent_env(&bridge->dev->dev.kobj,
						       bridge->pdata->mmap[i].
						       uevent_action, envp);
				if (state != 0)
					printk(KERN_ERR
					       "<error, kobject_uevent_env fail> with action : %d\n",
					       bridge->pdata->mmap[i].
					       uevent_action);
			}
			if (bridge->pdata->mmap[i].pre_event_func) {
				bridge->pdata->mmap[i].
				pre_event_func(bridge->pdata->event_data);
			}

			bridge->send_uevent_flag &= ~(1 << i);
		}
	}

	if (bridge->pdata->lcd_warning_func)
		bridge->pdata->lcd_warning_func();

	mutex_unlock(&bridge->lock);

	printk(KERN_INFO "<sec-input-bridge> all process done !!!!\n");
}

static void input_bridge_event(struct input_handle *handle, unsigned int type,
			       unsigned int code, int value)
{
	int rep_check;
	int i;

	struct input_handler *sec_bridge_handler = handle->handler;
	struct sec_input_bridge *sec_bridge = sec_bridge_handler->private;

	rep_check = test_bit(EV_REP, sec_bridge_handler->id_table->evbit);
	rep_check = (rep_check << 1) | 1;

	switch (type) {
	case EV_KEY:
		if (value & rep_check) {
			printk(KERN_INFO
			       "sec-input-bridge: KEY input intercepted, type : %d , code : %d , value %d ",
			       type, code, value);

			for (i = 0; i < sec_bridge->pdata->num_map; i++) {
				if (sec_bridge->pdata->mmap[i].
				    mkey_map[(sec_bridge->check_index[i])].
				    code == code) {
					sec_bridge->check_index[i]++;
					if ((sec_bridge->check_index[i]) >=
						(sec_bridge->pdata->mmap[i].
						num_mkey)) {
						sec_bridge->send_uevent_flag |=
							1 << i;
						schedule_work(&sec_bridge->
							work);
						sec_bridge->check_index[i] = 0;
					}
				} else if (sec_bridge->pdata->mmap[i].
						mkey_map[0].code == code) {
					sec_bridge->check_index[i] = 1;
				} else {
					sec_bridge->check_index[i] = 0;
				}
			}
		}
		break;

	default:
		break;
	}

}

static int input_bridge_connect(struct input_handler *handler,
				struct input_dev *dev,
				const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "sec-input-bridge";

	error = input_register_handle(handle);
	if (error) {
		printk(KERN_ERR
		       "sec-input-bridge: Failed to register input bridge handler, "
		       "error %d\n", error);
		kfree(handle);
		return error;
	}

	error = input_open_device(handle);
	if (error) {
		printk(KERN_ERR
		       "sec-input-bridge: Failed to open input bridge device, "
		       "error %d\n", error);
		input_unregister_handle(handle);
		kfree(handle);
		return error;
	}

	return 0;
}

static void input_bridge_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static struct input_handler input_bridge_handler = {
	.event = input_bridge_event,
	.connect = input_bridge_connect,
	.disconnect = input_bridge_disconnect,
	.name = "sec-input-bridge",
};

static int __devinit sec_input_bridge_probe(struct platform_device *dev)
{
	struct sec_input_bridge_platform_data *pdata;
	struct sec_input_bridge *bridge;
	struct input_device_id *input_bridge_ids;

	int state;
	int i, j;
	int k = 0;
	int total_num_key = 0;

	pdata = dev->dev.platform_data;
	if (!pdata) {
		dev_err(&dev->dev, "No samsung input bridge platform data.\n");
		return -EINVAL;
	}

	if (pdata->num_map == 0) {
		dev_err(&dev->dev,
			"No samsung input bridge platform data. num_mkey == 0\n");
		return -EINVAL;
	}

	bridge = kzalloc(sizeof(struct sec_input_bridge), GFP_KERNEL);
	if (!bridge)
		return -ENOMEM;

	bridge->check_index = kzalloc(sizeof(u8) * pdata->num_map, GFP_KERNEL);
	if (!bridge->check_index) {
		kfree(bridge);
		return -ENOMEM;
	}

	bridge->send_uevent_flag = 0;

	for (i = 0; i < pdata->num_map; i++)
		total_num_key += pdata->mmap[i].num_mkey;

	input_bridge_ids =
	    kzalloc(sizeof(struct input_device_id[(total_num_key + 1)]),
		    GFP_KERNEL);
	if (!input_bridge_ids) {
		kfree(bridge->check_index);
		kfree(bridge);
		return -ENOMEM;
	}
	memset(input_bridge_ids, 0x00, sizeof(input_bridge_ids));

	for (i = 0; i < pdata->num_map; i++) {
		for (j = 0; j < pdata->mmap[i].num_mkey; j++) {
			input_bridge_set_ids(&input_bridge_ids[k++],
					     pdata->mmap[i].mkey_map[j].type,
					     pdata->mmap[i].mkey_map[j].code);
		}
	}

	input_bridge_handler.private = bridge;
	input_bridge_handler.id_table = input_bridge_ids;

	state = input_register_handler(&input_bridge_handler);
	if (state)
		goto input_register_fail;

	bridge->dev = dev;
	bridge->pdata = pdata;

	INIT_WORK(&bridge->work, input_bridge_work);
	mutex_init(&bridge->lock);

	platform_set_drvdata(dev, bridge);

	return 0;

 input_register_fail:
	kfree(bridge->check_index);
	kfree(bridge);
	kfree(input_bridge_ids);

	return state;

}

static int __devexit sec_input_bridge_remove(struct platform_device *dev)
{
	struct sec_input_bridge *bridge = platform_get_drvdata(dev);

	cancel_work_sync(&bridge->work);
	mutex_destroy(&bridge->lock);
	kfree(input_bridge_handler.id_table);
	input_unregister_handler(&input_bridge_handler);
	kfree(bridge->check_index);
	kfree(bridge);
	platform_set_drvdata(dev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int sec_input_bridge_suspend(struct platform_device *dev,
				    pm_message_t state)
{
	return 0;
}

static int sec_input_bridge_resume(struct platform_device *dev)
{
	return 0;
}
#else
#define sec_input_bridge_suspend		NULL
#define sec_input_bridge_resume		NULL
#endif

static struct platform_driver sec_input_bridge_driver = {
	.probe = sec_input_bridge_probe,
	.remove = __devexit_p(sec_input_bridge_remove),
	.suspend = sec_input_bridge_suspend,
	.resume = sec_input_bridge_resume,
	.driver = {
		   .name = "samsung_input_bridge",
		   },
};

static int __init sec_input_bridge_init(void)
{
	return platform_driver_register(&sec_input_bridge_driver);
}

static void __exit sec_input_bridge_exit(void)
{
	platform_driver_unregister(&sec_input_bridge_driver);
}

module_init(sec_input_bridge_init);
module_exit(sec_input_bridge_exit);

MODULE_AUTHOR("Yongsul Oh <yongsul96.oh@samsung.com>");
MODULE_DESCRIPTION("Input Event -> Specific Control Bridge");
MODULE_LICENSE("GPL");
