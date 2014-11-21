#ifndef LINUX_INPUT_SEC_INPUT_BRIDGE_H
#define LINUX_INPUT_SEC_INPUT_BRIDGE_H

#include <linux/kobject.h>

enum mkey_check_option {
	MKEY_CHECK_AUTO,
	MKEY_CHECK_AWAYS
};

struct sec_input_bridge_mkey {
	unsigned int type;
	unsigned int code;
	enum mkey_check_option option;
};

struct sec_input_bridge_mmap {
	const struct sec_input_bridge_mkey *mkey_map;
	unsigned int num_mkey;
	const char *uevent_env_str;
	const char *uevent_env_value;
	const unsigned char enable_uevent;
	void (*pre_event_func)(void *event_data);
	enum kobject_action uevent_action;
};

struct sec_input_bridge_platform_data {
	void *event_data;

	const struct sec_input_bridge_mmap *mmap;
	unsigned int num_map;
	void (*lcd_warning_func)(void);
};

#endif /* LINUX_INPUT_SEC_INPUT_BRIDGE_H */

