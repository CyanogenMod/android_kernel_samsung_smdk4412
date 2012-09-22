/* Lte modem bootloader support for Samsung Tuna Board.
 *
 * Copyright (C) 2011 Google, Inc.
 * Copyright (C) 2011 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LTE_MODEM_BOOTLOADER_H
#define __LTE_MODEM_BOOTLOADER_H

#define LTE_MODEM_BOOTLOADER_DRIVER_NAME	"lte_modem_bootloader"

#define IOCTL_LTE_MODEM_XMIT_BOOT		_IOW('o', 0x23, unsigned int)
#define IOCTL_LTE_MODEM_LTE2AP_STATUS	_IOR('o', 0x24, unsigned int)

#define IOCTL_LTE_MODEM_FACTORY_MODE_ON		_IOWR('o', 0x25, unsigned int)
#define IOCTL_LTE_MODEM_FACTORY_MODE_OFF	_IOWR('o', 0x26, unsigned int)

struct lte_modem_bootloader_param {
	char __user *buf;
	int len;
};

struct lte_modem_bootloader_platform_data {
	const char *name;
	unsigned int gpio_lte2ap_status;
};
#endif/* LTE_MODEM_BOOTLOADER_H */
