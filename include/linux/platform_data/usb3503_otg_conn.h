#ifndef USB3503_H
#define USB3503_H

#define USB3503_I2C_NAME "usb3503"
int usb3503_set_mode(int mode);

enum {
	USB_3503_MODE_HUB = 0,
	USB_3503_MODE_STANDBY = 1,
};

struct usb3503_platform_data {
	bool init_needed;
	bool es_ver;
	char inital_mode;
	int (*hw_config)(void);
	int (*reset_n)(int);
	int (*port_enable)(int, int);
};

#endif

