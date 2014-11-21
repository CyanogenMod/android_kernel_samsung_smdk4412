/*
 * File Name : multi_config.h
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

#ifndef __MULTI_CONFIG_H
#define __MULTI_CONFIG_H

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/usb/composite.h>

/*
 * Debugging macro and defines
 */
/*#define USB_DEBUG
 *#define USB_MORE_DEBUG
 */
#define USB_DEBUG_ESS

#ifdef USB_DEBUG
#  ifdef USB_MORE_DEBUG
#    define USB_DBG(fmt, args...) \
	printk(KERN_INFO "usb: %s "fmt, __func__, ##args)
#  else
#    define USB_DBG(fmt, args...) \
	printk(KERN_DEBUG "usb: %s "fmt, __func__, ##args)
#  endif
#else /* DO NOT PRINT LOG */
#  define USB_DBG(fmt, args...) do { } while (0)
#endif /* USB_DEBUG */

#ifdef USB_DEBUG_ESS
#  ifdef USB_MORE_DEBUG
#    define USB_DBG_ESS(fmt, args...) \
	printk(KERN_INFO "usb: %s "fmt, __func__, ##args)
#  else
#    define USB_DBG_ESS(fmt, args...) \
	printk(KERN_DEBUG "usb: %s "fmt, __func__, ##args)
#  endif
#else /* DO NOT PRINT LOG */
#  define USB_DBG_ESS(fmt, args...) do { } while (0)
#endif /* USB_DEBUG_ESS */

#define MAIN_FUNCTION "mtp"
#define MULTI_FUNCTION_1 "mtp"
#define MULTI_FUNCTION_2 "acm0"
#define MULTI_EXCEPTION_FUNCTION "adb"

#define MAC_REQUEST	0
#define OTHER_REQUEST	1

#define MAX_MULTI_CONFIG_NUM 2

/* Description  : Set configuration number
 * Parameter    : unsigned num (host request)
 * Return value : always return 0 (It's virtual multiconfiguration)
 */
unsigned set_config_number(unsigned num);

/* Description  : Get configuration number
 * Return value : virtual multiconfiguration number (zero base)
 */
int get_config_number(void);

/* Description  : Check configuration number
 * Parameter    : unsigned num (host request)
 * Return value : 1 (true  : virtual multi configuraiton)
		  0 (false : normal configuration)
 */
int check_config(unsigned num);

/* Description  : Search number of configuration including virtual configuration
 * Parameter    : usb_configuration *c (referenced configuration)
		  unsigned count (real number of configuration)
 * Return value : virtual or real number of configuration
 */
unsigned count_multi_config(struct usb_configuration *c, unsigned count);

/* Description  : Is multi configuration available ?
 * Return value : 1 (true), 0 (false)
 */
int is_multi_configuration(void);

/* Description  : Check function to skip for multi configuration
 * Parameter    : char* name (function name)
 * Return value : 0 (not available), 1 (available)
 */
int is_available_function(const char *name);

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
		enum usb_device_speed speed);

/* Description  : Set interface count
 * Parameter    : struct usb_configuration *config
		  (To reference interface array of current config)
		  struct usb_config_descriptor *c
		  (number of interfaces)
 * Return value : void
 */
void set_interface_count(struct usb_configuration *config,
	struct usb_config_descriptor *c);

/* Description  : Set string mode
 *		  This mode will be used for deciding other interface.
 * Parameter    : u16 w_length
 *		  - 2 means MAC request.
 *		  - Windows and Linux PC always request 255 size.
 */
void set_string_mode(u16 w_length);

/* Description  : Get Host OS type
 * Return value : type - u16
 *		- 0 : MAC PC
 *		- 1 : Windows and Linux PC
 */
u16 get_host_os_type(void);
#endif /* __MULTI_CONFIG_H */
