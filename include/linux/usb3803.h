#ifndef USB3803_H
#define USB3803_H

#define USB3803_I2C_NAME "usb3803"
int usb3803_set_mode(int mode);

enum {
	USB_3803_MODE_HUB = 0,
	USB_3803_MODE_BYPASS = 1,
	USB_3803_MODE_STANDBY = 2,
};

struct usb3803_platform_data {
	bool init_needed;
	bool es_ver;
	char inital_mode;
	int (*hw_config)(void);
	int (*reset_n)(int);
	int (*bypass_n)(int);
	int (*clock_en)(int);
};

#endif
