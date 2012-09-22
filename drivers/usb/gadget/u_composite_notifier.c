/*
 * File Name : u_composite_notifier.c
 *
 * Copyright (C) 2012 Samsung Electronics
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
static char function_lists_string[256];

static BLOCKING_NOTIFIER_HEAD(usb_composite_notifier_list);

int register_usb_composite_notifier(struct notifier_block *notifier)
{
	int retval;

	retval = blocking_notifier_chain_register(
		&usb_composite_notifier_list, notifier);

	return retval;
}
EXPORT_SYMBOL(register_usb_composite_notifier);

int unregister_usb_composite_notifier(struct notifier_block *notifier)
{
	int retval;

	retval = blocking_notifier_chain_unregister(
		&usb_composite_notifier_list, notifier);

	return retval;
}
EXPORT_SYMBOL(unregister_usb_composite_notifier);
