#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/hid.h>
#include <linux/mutex.h>
#include <linux/input.h>
#include <linux/input/mt.h>

#include "hid-ids.h"

#define MAX_NUM_OF_FINGERS 		5
#define FINGER_DATA_SIZE		8
#define DEFAULT_MAX_ABS_MT_PRESSURE	255

static int debug = 0;
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Activate debugging output");

struct syntp_finger {
	int x;
	int y;
	int z;
	int w;
	int dribble_count;
};

struct hid_synaptics_data {
	char phys[64];
	struct input_dev *input;	/* input dev */
	struct syntp_finger fingers[MAX_NUM_OF_FINGERS];
	int button_down;
};

#define samsung_kbd_mouse_map_key_clear(c) \
	hid_map_usage_clear(hi, usage, bit, max, EV_KEY, (c))

static int hid_synaptics_raw_event(struct hid_device *hdev,
			struct hid_report *report, u8 *data, int size)
{
	struct hid_synaptics_data *syntp_data = hid_get_drvdata(hdev);
	struct input_dev *input = syntp_data->input;
	int raw_z, raw_w, raw_x, raw_y, i = 0;
	int inverted_y;
	int send_packet;

	/* make sure this report has the right report id */
	if (data[0] != 9 || ((size - 1) < (MAX_NUM_OF_FINGERS * FINGER_DATA_SIZE)))
		return 0;

	++data;
	for (i = 0; i < MAX_NUM_OF_FINGERS; ++i, data += FINGER_DATA_SIZE) {
		raw_w = data[0] & 0x0f;
		raw_x = be16_to_cpup((__be16 *)&data[2]);
		raw_y = be16_to_cpup((__be16 *)&data[4]);
		raw_z = data[6];

		send_packet = 0;

		if (raw_z == 0 && raw_w == 0) {
			/* dribble packet */
			if (syntp_data->fingers[i].dribble_count == 0) {
				raw_x = syntp_data->fingers[i].x;
				raw_y = syntp_data->fingers[i].y;
				raw_z = 0;
				raw_w = 0;
				++syntp_data->fingers[i].dribble_count;
				send_packet = 1;
			}
		} else {
			/* real packet */
			syntp_data->fingers[i].dribble_count = 0;
			send_packet = 1;

			/* update with last real packet */
			syntp_data->fingers[i].x = raw_x;
			syntp_data->fingers[i].y = raw_y;
			syntp_data->fingers[i].z = raw_z;
			syntp_data->fingers[i].w = raw_w;
		}

		if (send_packet) {
			inverted_y = (5888 - 1024) - raw_y;

			dev_dbg(&hdev->dev, "Finger Count = %d\n", data[7] >> 4);
			dev_dbg(&hdev->dev, "ID=%d X=%d Y=%d W=%d Z=%d "
				"button=%d\n", i, raw_x, raw_y,
				raw_w, raw_z, data[1]);

			input_mt_slot(input, i);
			input_mt_report_slot_state(input, MT_TOOL_FINGER, 1);
			input_report_abs(input, ABS_MT_PRESSURE, raw_z);
			input_report_abs(input, ABS_MT_TOUCH_MAJOR, raw_w);
			input_report_abs(input, ABS_MT_TOUCH_MINOR, raw_w);
			input_report_abs(input, ABS_MT_ORIENTATION, 0);
			input_report_abs(input, ABS_MT_POSITION_X, raw_x);
			input_report_abs(input, ABS_MT_POSITION_Y, inverted_y);

			input_mt_report_pointer_emulation(input, true);
		}

		if (data[1] & 0x02) {
			if (!syntp_data->button_down) {
				dev_dbg(&hdev->dev, "Button Down!\n");
				syntp_data->button_down = 1;
				input_report_key(input, BTN_LEFT, 1);
			}
		} else if (syntp_data->button_down) {
			dev_dbg(&hdev->dev, "Button Up!\n");
			syntp_data->button_down = 0;
			input_report_key(input, BTN_LEFT, 0);
		}
	}

	input_sync(input);

	return 0;
}

static int hid_synaptics_event(struct hid_device *hdev, struct hid_field *field,
		 		struct hid_usage *usage, __s32 value)
{
	if (field->report->id == 9)
		return 1;

	return 0;
}

static int hid_synaptics_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	struct hid_synaptics_data *syntp_data;
	struct input_dev * input_dev;
	unsigned int connect_mask = HID_CONNECT_DEFAULT;
	int ret;

	dev_dbg(&hdev->dev, "%s\n", __func__);

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		goto err_free;
	}

	ret = hid_hw_start(hdev, connect_mask);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		goto err_free;
	}

	syntp_data = devm_kzalloc(&hdev->dev, sizeof(struct hid_synaptics_data), GFP_KERNEL);
	if (!syntp_data) {
		ret =-ENOMEM;
		goto hid_stop;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		goto hid_stop;
	}

	input_dev->name = "Synaptics HID TouchPad";
	snprintf(syntp_data->phys, 64, "%s/input0", dev_name(&hdev->dev));
	input_dev->phys = syntp_data->phys;

	/* Setup Events to Report */
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_LEFT, input_dev->keybit);

	set_bit(INPUT_PROP_POINTER, input_dev->propbit);

	input_set_capability(input_dev, EV_KEY, BTN_TOUCH);

	set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, input_dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, input_dev->keybit);
	set_bit(BTN_TOOL_QUADTAP, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_X, 1024, 5888, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 1024, 5888, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, DEFAULT_MAX_ABS_MT_PRESSURE, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
					0, 15, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR,
					0, 15, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_ORIENTATION,
					0, 1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID,
					1, 5, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
					1024, 5888, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
					1024, 5888, 0, 0);

	input_mt_init_slots(input_dev, 5);

	syntp_data->input = input_dev;
	ret = input_register_device(syntp_data->input);
	if (ret)
		goto hid_init_failed;

	hid_set_drvdata(hdev, syntp_data);

	dev_dbg(&hdev->dev, "Opening low level driver\n");
	hdev->ll_driver->open(hdev);

	dev_info(&hdev->dev, "registered rmi hid driver for %s\n", hdev->phys);
	return 0;

hid_init_failed:
        hdev->ll_driver->close(hdev);
hid_stop:
        hid_hw_stop(hdev);

err_free:
        return ret;
}

static void hid_synaptics_remove(struct hid_device *hdev)
{
	hdev->ll_driver->close(hdev);
	hid_hw_stop(hdev);
}

static int samsung_bookcover_input_mapping(struct hid_device *hdev,
	struct hid_input *hi, struct hid_field *field, struct hid_usage *usage,
	unsigned long **bit, int *max)
{
	if (!(HID_UP_CONSUMER == (usage->hid & HID_USAGE_PAGE) ||
			HID_UP_KEYBOARD == (usage->hid & HID_USAGE_PAGE)))
		return 0;

	dbg_hid("samsung wireless keyboard input mapping event [0x%x]\n",
		usage->hid & HID_USAGE);

	if (HID_UP_KEYBOARD == (usage->hid & HID_USAGE_PAGE)) {
		switch (usage->hid & HID_USAGE) {
		set_bit(EV_REP, hi->input->evbit);
		/* Only for UK keyboard */
		/* key found */
#ifdef CONFIG_HID_KK_UPGRADE
		case 0x32: samsung_kbd_mouse_map_key_clear(KEY_KBDILLUMTOGGLE); break;
		case 0x64: samsung_kbd_mouse_map_key_clear(KEY_BACKSLASH); break;
#else
		case 0x32: samsung_kbd_mouse_map_key_clear(KEY_BACKSLASH); break;
		case 0x64: samsung_kbd_mouse_map_key_clear(KEY_102ND); break;
#endif
		/* Only for BR keyboard */
		case 0x87: samsung_kbd_mouse_map_key_clear(KEY_RO); break;
		default:
			return 0;
		}
	}

	if (HID_UP_CONSUMER == (usage->hid & HID_USAGE_PAGE)) {
		switch (usage->hid & HID_USAGE) {
		/* report 2 */
		/* MENU */
		case 0x040: samsung_kbd_mouse_map_key_clear(KEY_MENU); break;
		case 0x18a: samsung_kbd_mouse_map_key_clear(KEY_MAIL); break;
		case 0x196: samsung_kbd_mouse_map_key_clear(KEY_WWW); break;
		case 0x19e: samsung_kbd_mouse_map_key_clear(KEY_SCREENLOCK); break;
		case 0x221: samsung_kbd_mouse_map_key_clear(KEY_SEARCH); break;
		case 0x223: samsung_kbd_mouse_map_key_clear(KEY_HOMEPAGE); break;
		/* RECENTAPPS */
		case 0x301: samsung_kbd_mouse_map_key_clear(BTN_TRIGGER_HAPPY1); break;
		/* APPLICATION */
		case 0x302: samsung_kbd_mouse_map_key_clear(BTN_TRIGGER_HAPPY2); break;
		/* Voice search */
		case 0x305: samsung_kbd_mouse_map_key_clear(BTN_TRIGGER_HAPPY4); break;
		/* QPANEL on/off */
		case 0x306: samsung_kbd_mouse_map_key_clear(BTN_TRIGGER_HAPPY5); break;
		/* SIP on/off */
		case 0x307: samsung_kbd_mouse_map_key_clear(BTN_TRIGGER_HAPPY3); break;
		/* LANG */
		case 0x308: samsung_kbd_mouse_map_key_clear(KEY_LANGUAGE); break;
		case 0x30a: samsung_kbd_mouse_map_key_clear(KEY_BRIGHTNESSDOWN); break;
		case 0x070: samsung_kbd_mouse_map_key_clear(KEY_BRIGHTNESSDOWN); break;
		case 0x30b: samsung_kbd_mouse_map_key_clear(KEY_BRIGHTNESSUP); break;
		case 0x06f: samsung_kbd_mouse_map_key_clear(KEY_BRIGHTNESSUP); break;
		/* S-Finder */
		case 0x304: samsung_kbd_mouse_map_key_clear(BTN_TRIGGER_HAPPY7); break;
		/* Screen Capture */
		case 0x303: samsung_kbd_mouse_map_key_clear(KEY_SYSRQ); break;
		/* Multi Window */
		case 0x309: samsung_kbd_mouse_map_key_clear(BTN_TRIGGER_HAPPY9); break;
		default:
			return 0;
		}
	}

	return 1;
}

static int hid_synaptics_mapping(struct hid_device *hdev, struct hid_input *hi,
	struct hid_field *field, struct hid_usage *usage,
	unsigned long **bit, int *max)
{
	int ret = 0;

	if (USB_DEVICE_ID_SAMSUNG_WIRELESS_BOOKCOVER == hdev->product)
		ret = samsung_bookcover_input_mapping(hdev,
			hi, field, usage, bit, max);

	return ret;
}

static const struct hid_device_id hid_synaptics_id[] = {
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_SAMSUNG_ELECTRONICS, USB_DEVICE_ID_SAMSUNG_WIRELESS_BOOKCOVER),
		.driver_data = 0 },
	{ HID_USB_DEVICE(0x06cb, 0x5555),
		.driver_data = 0 },
	{}
};
MODULE_DEVICE_TABLE(hid, hid_synaptics_id);

static struct hid_driver hid_synaptics_driver = {
	.name = "hid-synaptics-bt",
	.driver = {
		.owner  = THIS_MODULE,
		.name 	= "hid-synaptics-bt",
	},
	.id_table	= hid_synaptics_id,
	.probe		= hid_synaptics_probe,
	.remove		= hid_synaptics_remove,
	.raw_event	= hid_synaptics_raw_event,
	.event		= hid_synaptics_event,
	.input_mapping = hid_synaptics_mapping,
};

static int __init hid_synaptics_init(void)
{
	int ret;

	ret = hid_register_driver(&hid_synaptics_driver);
	if (ret)
		pr_err("Failed to register hid_synaptics_driver (%d)\n", ret);
	else
		pr_info("Successfully registered hid_synaptics_driver\n");

	return ret;
}

static void __exit hid_synaptics_exit(void)
{
	hid_unregister_driver(&hid_synaptics_driver);
}

module_init(hid_synaptics_init);
module_exit(hid_synaptics_exit);

MODULE_AUTHOR("Andrew Duggan");
MODULE_DESCRIPTION("Synaptics HID Bluetooth Dock Driver");
MODULE_LICENSE("GPL");
