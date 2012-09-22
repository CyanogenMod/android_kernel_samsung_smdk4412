/* cyasswitch.c - West Bridge test module source file
## ===========================
## Copyright (C) 2010  Cypress Semiconductor
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor
## Boston, MA  02110-1301, USA.
## ===========================
*/
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/scatterlist.h>
#include <linux/mm.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <asm/io.h>
#include <linux/clk.h>

#include <linux/device.h>
#include <linux/sched.h>

#include "../include/linux/westbridge/cyashal.h"
#include "../include/linux/westbridge/cyastoria.h"
#include "../device/cyasdiagnostics.h"

#define	CY_AS_DRIVER_DESC		"cypress west bridge usb switch"
#define	CY_AS_DRIVER_VERSION		"REV 0.1"

extern int cy_as_diagnostics(cy_as_diag_cmd_type mode, char *result);

static int mode;
module_param(mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "firmware number, change switch=0, UMS=1");

char *cyasswitch_buf;

static int __init cy_as_switch_init(void)
{
	uint32_t retVal = 0;

	cyasswitch_buf = (uint8_t *) cy_as_hal_alloc(4096);
	cy_as_hal_mem_set(cyasswitch_buf, 0, 4096);

	switch (mode) {
	case 1:
		retVal =
		    cy_as_diagnostics(CY_AS_DIAG_GET_VERSION,
				      cyasswitch_buf);
		if (retVal)
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_GET_VERSION failed.\n");
		else
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_GET_VERSION - %s\n",
			     cyasswitch_buf);
		break;

	case 10:
		{
			int i;
			retVal =
			    cy_as_diagnostics(CY_AS_DIAG_SD_MOUNT,
					      cyasswitch_buf);
			if (retVal)
				cy_as_hal_print_message
				    ("<1>CyAsSwitch: CY_AS_DIAG_SD_MOUNT failed.\n");
			else
				cy_as_hal_print_message
				    ("<1>CyAsSwitch: CY_AS_DIAG_SD_MOUNT - %s\n",
				     cyasswitch_buf);

			for (i = 0; i < 4; i++) {
				retVal =
				    cy_as_diagnostics(CY_AS_DIAG_SD_WRITE,
						      cyasswitch_buf);
				if (retVal)
					cy_as_hal_print_message
					    ("<1>CyAsSwitch: CY_AS_DIAG_SD_MOUNT failed.\n");
				else
					cy_as_hal_print_message
					    ("<1>CyAsSwitch: CY_AS_DIAG_SD_MOUNT - %s KBytes/s\n",
					     cyasswitch_buf);

			}

			for (i = 0; i < 32; i++) {
				retVal =
				    cy_as_diagnostics(CY_AS_DIAG_SD_READ,
						      cyasswitch_buf);
				if (retVal)
					cy_as_hal_print_message
					    ("<1>CyAsSwitch: CY_AS_DIAG_SD_MOUNT failed.\n");
				else
					cy_as_hal_print_message
					    ("<1>CyAsSwitch: CY_AS_DIAG_SD_MOUNT - %s KBytes/s\n",
					     cyasswitch_buf);

			}

		}
		break;

	case 20:
		retVal =
		    cy_as_diagnostics(CY_AS_DIAG_CONNECT_UMS,
				      cyasswitch_buf);
		if (retVal)
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_CONNECT_UMS failed.\n");
		else
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_CONNECT_UMS - %s\n",
			     cyasswitch_buf);
		break;

	case 30:
		retVal =
		    cy_as_diagnostics(CY_AS_DIAG_CONNECT_MTP,
				      cyasswitch_buf);
		if (retVal)
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_CONNECT_MTP failed.\n");
		else
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_CONNECT_MTP - %s\n",
			     cyasswitch_buf);
		break;

	case 40:
		cy_as_diagnostics(CY_AS_DIAG_DISABLE_MSM_SDIO,
				  cyasswitch_buf);

		cy_as_diagnostics(CY_AS_DIAG_LEAVE_STANDBY,
				  cyasswitch_buf);

		retVal =
		    cy_as_diagnostics(CY_AS_DIAG_CREATE_BLKDEV,
				      cyasswitch_buf);
		if (retVal)
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_CREATE_BLKDEV failed.\n");
		else
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_CREATE_BLKDEV - %s\n",
			     cyasswitch_buf);

		break;

	default:
		cy_as_hal_print_message
		    ("<1>CyAsSwitch: unkown mode, please run 'rmmod cyasswitch.ko' \n");
		break;

	}
	return 0;
}

module_init(cy_as_switch_init);

static void __exit cy_as_switch_cleanup(void)
{
	uint32_t retVal = 0;

	switch (mode) {
	case 10:
		retVal =
		    cy_as_diagnostics(CY_AS_DIAG_SD_UNMOUNT,
				      cyasswitch_buf);
		if (retVal)
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_SD_UNMOUNT failed.\n");
		else
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_SD_UNMOUNT - %s\n",
			     cyasswitch_buf);
		break;

	case 20:

		retVal =
		    cy_as_diagnostics(CY_AS_DIAG_DISCONNECT_UMS,
				      cyasswitch_buf);
		if (retVal)
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_DISCONNECT_UMS failed.\n");
		else
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_DISCONNECT_UMS - %s\n",
			     cyasswitch_buf);
		break;

	case 30:
		retVal =
		    cy_as_diagnostics(CY_AS_DIAG_DISCONNECT_MTP,
				      cyasswitch_buf);
		if (retVal)
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_DISCONNECT_MTP failed.\n");
		else
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_DISCONNECT_MTP - %s\n",
			     cyasswitch_buf);
		break;

	case 40:
		retVal =
		    cy_as_diagnostics(CY_AS_DIAG_DESTROY_BLKDEV,
				      cyasswitch_buf);
		if (retVal)
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_DESTROY_BLKDEV failed.\n");
		else
			cy_as_hal_print_message
			    ("<1>CyAsSwitch: CY_AS_DIAG_DESTROY_BLKDEV - %s\n",
			     cyasswitch_buf);

		cy_as_diagnostics(CY_AS_DIAG_ENTER_STANDBY,
				  cyasswitch_buf);

		cy_as_diagnostics(CY_AS_DIAG_ENABLE_MSM_SDIO,
				  cyasswitch_buf);
		break;

	default:
		break;
	}

	cy_as_hal_print_message("<1>CyAsSwitch: exit \n");

	cy_as_hal_free(cyasswitch_buf);

	return;
}

module_exit(cy_as_switch_cleanup);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(CY_AS_DRIVER_DESC);
MODULE_AUTHOR("cypress semiconductor");
/*[]*/
