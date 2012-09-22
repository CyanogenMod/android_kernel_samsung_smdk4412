/*
 * File Name : multi_config.c
 *
 * Virtual multi configuration utilities for composite USB gadgets.
 * This utilitie can support variable interface for variable Host PC.
 *
 * Copyright (C) 2011 Samsung Electronics
 * Author: SoonYong, Cho <soonyong.cho@samsung.com>
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

#include "multi_config.h"

static int multi; /* current configuration */
static int is_multi; /* Is multi configuration available ? */
static int stringMode = OTHER_REQUEST;
static int interfaceCount;

/* Description  : Set configuration number
 * Parameter    : unsigned num (host request)
 * Return value : always return 0 (It's virtual multiconfiguration)
 */
unsigned set_config_number(unsigned num)
{
	if (num != 0)
		USB_DBG_ESS("multi config_num=%d(zero base)\n", num);
	else
		USB_DBG_ESS("single config\n");

	multi = num;	/* save config number from Host request */
	return 0;	/* always return 0 config */
}

/* Description  : Get configuration number
 * Return value : virtual multiconfiguration number (zero base)
 */
int get_config_number(void)
{
	USB_DBG("multi=%d\n", multi);

	return multi;
}

/* Description  : Check configuration number
 * Parameter    : unsigned num (host request)
 * Return value : 1 (true  : virtual multi configuraiton)
		  0 (false : normal configuration)
 */
int check_config(unsigned num)
{
	USB_DBG("num=%d, multi=%d\n", num, multi);
	/* multi is zero base, but num is 1 ase */
	if (num && num == multi + 1) {
		USB_DBG("Use virtual multi configuration\n");
		return 1;
	} else {
		USB_DBG("normal configuration\n");
		return 0;
	}
}

/* Description  : Search number of configuration including virtual configuration
 * Parameter    : usb_configuration *c (referenced configuration)
		  unsigned count (real number of configuration)
 * Return value : virtual or real number of configuration
 */
unsigned count_multi_config(struct usb_configuration *c, unsigned count)
{
	int				f_first = 0;
	int				f_second = 0;
	int				f_exception = 0;
	struct usb_function		*f;
	is_multi = 0;

	if (!c) {
		USB_DBG("usb_configuration is not valid\n");
		return 0;
	}

	list_for_each_entry(f, &c->functions, list) {
		if (!strcmp(f->name, MULTI_FUNCTION_1)) {
			USB_DBG("%s +\n", MULTI_FUNCTION_1);
			f_first = 1;
		} else if (!strcmp(f->name, MULTI_FUNCTION_2)) {
			USB_DBG("%s +\n", MULTI_FUNCTION_2);
			f_second = 1;
		} else if (!strcmp(f->name, MULTI_EXCEPTION_FUNCTION)) {
			USB_DBG("exception %s +\n", MULTI_EXCEPTION_FUNCTION);
			f_exception = 1;
		}
	}

	if (f_first && f_second && !f_exception) {
		USB_DBG_ESS("ready multi\n");
		is_multi = 1;
		return 2;
	}
	return count;
}

/* Description  : Is multi configuration available ?
 * Return value : 1 (true), 0 (false)
 */
int is_multi_configuration(void)
{
	USB_DBG("= %d\n", is_multi);
	return is_multi;
}

/* Description  : Check function to skip for multi configuration
 * Parameter    : char* name (function name)
 * Return value : 0 (not available), 1 (available)
 */
int is_available_function(const char *name)
{
	if (is_multi_configuration()) {
		USB_DBG("multi case\n");
		if (!multi) {
			if (!strcmp(name, MAIN_FUNCTION)) {
				USB_DBG("%s is available.\n",
						MAIN_FUNCTION);
				return 1;
			}
			return 0; /* anothor function is not available */
		} else {
			USB_DBG("multi=%d all available\n", multi);
		}
	}
	return 1; /* if single configuration, every function is available */
}

/* Description  : Change configuration using virtual multi configuration.
 * Parameter    : struct usb_funciton f (to be changed function interface)
		  void *next (next means usb req->buf)
		  int len (length for to fill buffer)
		  struct usb_configuration *config
		  (To reference interface array of current config)
		  enum usb_device_speed speed (usb speed)
 * Return value : "ret < 0" means fillbuffer function is failed.
 */
int change_conf(struct usb_function *f,
		void *next, int len,
		struct usb_configuration *config,
		enum usb_device_speed speed)
{
	u8 *dest;
	int status = 0;
	struct usb_descriptor_header *descriptor;
	struct usb_interface_descriptor *intf;
	int index_intf = 0;
	int change_intf = 0;
	struct usb_descriptor_header **descriptors;

	USB_DBG("f->%s process multi\n", f->name);

	if (!f || !config || !next) {
		USB_DBG_ESS("one of f, config, next is not valid\n");
		return -EFAULT;
	}
	if (speed == USB_SPEED_HIGH)
		descriptors = f->hs_descriptors;
	else
		descriptors = f->descriptors;
	if (!descriptors) {
		USB_DBG_ESS("descriptor is not available\n");
		return -EFAULT;
	}

	if (f->set_config_desc)
		f->set_config_desc(stringMode);

	/* set interface numbers dynamically */
	dest = next;

	while ((descriptor = *descriptors++) != NULL) {
		intf = (struct usb_interface_descriptor *)dest;
		if (intf->bDescriptorType == USB_DT_INTERFACE) {
			if (intf->bAlternateSetting == 0) {
				intf->bInterfaceNumber = interfaceCount++;
				USB_DBG("a=0 intf->bInterfaceNumber=%d\n",
						intf->bInterfaceNumber);
			} else {
				intf->bInterfaceNumber = interfaceCount - 1;
				USB_DBG("a!=0 intf->bInterfaceNumber=%d\n",
						intf->bInterfaceNumber);
			}
			config->interface
				[intf->bInterfaceNumber]
				= f;
			if (f->set_intf_num) {
				change_intf = 1;
				f->set_intf_num(f,
						intf->bInterfaceNumber,
						index_intf++);
			}
		}
		dest += intf->bLength;
	}

	if (change_intf) {
		if (speed == USB_SPEED_HIGH)
			descriptors = f->hs_descriptors;
		else
			descriptors = f->descriptors;
		status = usb_descriptor_fillbuf(
				next, len,
				(const struct usb_descriptor_header **)
				descriptors);
		if (status < 0) {
			USB_DBG_ESS("usb_descriptor_fillbuf failed\n");
			return status;
		}
	}

	return status;
}

/* Description  : Set interface count
 * Parameter    : struct usb_configuration *config
 *		  (To reference interface array of current config)
 *		  struct usb_config_descriptor *c
 *		  (number of interfaces)
 * Return value : void
 */
void set_interface_count(struct usb_configuration *config,
	struct usb_config_descriptor *c)
{
	USB_DBG_ESS("next_interface_id=%d\n", interfaceCount);
	config->next_interface_id = interfaceCount;
	config->interface[interfaceCount] = 0;
	c->bNumInterfaces = interfaceCount;
	interfaceCount = 0;
	return ;
}

/* Description  : Set string mode
 *		  This mode will be used for deciding other interface.
 * Parameter    : u16 w_length
 *		- 2 means MAC request.
 *		- Windows and Linux PC always request 255 size.
 */
void set_string_mode(u16 w_length)
{
	if (w_length == 2) {
		USB_DBG("mac request\n");
		stringMode = MAC_REQUEST;
	} else if (w_length == 0) {
		USB_DBG("initialize string mode\n");
		stringMode = OTHER_REQUEST;
	}
}

/* Description  : Get Host OS type
 * Return value : type - u16
 *		- 0 : MAC PC
 *		- 1 : Windows and Linux PC
 */
u16 get_host_os_type(void)
{
	return stringMode;
}
