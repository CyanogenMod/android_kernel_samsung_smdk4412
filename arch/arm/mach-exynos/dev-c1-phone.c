/* linux/arch/arm/mach-s5pv310/dev-herring-phone.c
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/irq.h>

#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>

#include <mach/map.h>
#include <mach/gpio.h>

#include "../../../drivers/dpram/raffaello/dpram.h"
#include "u1.h"


static int c1_is_bootmode_recovery;
static int setup_bootmode(char *str)
{
	printk(KERN_INFO "%s : %s\n", __func__, str);

	c1_is_bootmode_recovery = 0;

	if (str)
		if ((*str == '2') || (*str == '4'))
			c1_is_bootmode_recovery = 1;

	printk(KERN_INFO "c1_is_bootmode_recovery = %d\n", c1_is_bootmode_recovery);
	
	return c1_is_bootmode_recovery;
}

__setup("bootmode=", setup_bootmode);



struct platform_device sec_device_dpram = {
	.name = "dpram-device",
	.id   = -1,
};

struct platform_device sec_device_dpram_recovery = {
	.name = "dpram-recovery",
	.id   = -1,
};


static int __init c1_init_phone_interface(void)
{
#if defined(CONFIG_SAMSUNG_PHONE_TTY)
	if (c1_is_bootmode_recovery){
		printk(KERN_INFO "c1_init_phone_interface - RECOVERY");
		platform_device_register(&sec_device_dpram_recovery);
	}else{
		printk(KERN_INFO "c1_init_phone_interface");
		platform_device_register(&sec_device_dpram);
	}
#endif
	return 0;
}
device_initcall(c1_init_phone_interface);
