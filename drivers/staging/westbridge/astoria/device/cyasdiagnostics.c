/*
## cyasdiagnostics.c - Linux Antioch device driver file
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
#include "cyasdiagnostics.h"

#include "../include/linux/westbridge/cyashal.h"
#include "../include/linux/westbridge/cyastoria.h"


extern uint32_t cy_as_hal_storage_test(cy_as_device_handle dev_p, uint32_t loop_max);

extern cy_as_device_handle cyasdevice_getdevhandle(void);
extern cy_as_hal_device_tag cyasdevice_gethaltag(void);
extern int cyasdevice_enter_standby(void);
extern int cyasdevice_leave_standby(void);

extern int CyAsAPIUsbInit(void);
extern int CyAsAPIUsbExit(void);
extern int cyasdevice_reload_firmware(int mtp_mode);

extern int  cyasblkdev_blk_init(int fmajor, int fsearch);
extern void  cyasblkdev_blk_exit(void);
extern int cy_as_gadget_init(int fmtp);
extern void  cy_as_gadget_cleanup(void);

#define CYASSTORAGE_MAX_XFER_SIZE	(2*32768)
#define MAX_DRQ_LOOPS_IN_ISR		128

uint8_t *actdata ;
uint8_t *expdata ;

uint32_t start_unit;

int cy_as_diagnostics(cy_as_diag_cmd_type mode, char *result)
{
	uint32_t retVal = 0;
#ifdef __CYAS_SYSFS_FOR_DIAGNOSTICS__
	cy_as_device_handle cyas_hd = cyasdevice_getdevhandle();

	switch (mode) {
	case CY_AS_DIAG_GET_VERSION:
		{
			cy_as_get_firmware_version_data ver_data = {0};
			const char *str = "" ;
			cyasdevice_leave_standby();
			retVal = cy_as_misc_get_firmware_version(cyas_hd, &ver_data, 0, 0) ;
			if (retVal != CY_AS_ERROR_SUCCESS) {
				cy_as_hal_print_message(
					"cy_as_diagnostics: cannot get firmware version. reason code: %d\n",
					retVal) ;
				sprintf(result, "fail - %d", retVal);
				return retVal;
			}

			if ((ver_data.media_type & 0x01) && (ver_data.media_type & 0x06))
				str = "nand and SD/MMC." ;
			else if ((ver_data.media_type & 0x01) && (ver_data.media_type & 0x08))
				str = "nand and CEATA." ;
			else if (ver_data.media_type & 0x01)
				str = "nand." ;
			else if (ver_data.media_type & 0x08)
				str = "CEATA." ;
			else
				str = "SD/MMC." ;

			cyasdevice_enter_standby();
			cy_as_hal_print_message("<1> cy_as_device:_firmware version: %s "
					"major=%d minor=%d build=%d,\n_media types supported:%s\n",
					((ver_data.is_debug_mode) ? "debug" : "release"),
					ver_data.major, ver_data.minor, ver_data.build, str) ;
			sprintf(result, "%d.%d.%d", ver_data.major, ver_data.minor, ver_data.build);
		}
		break;

	case CY_AS_DIAG_DISABLE_MSM_SDIO:
		break;
	case CY_AS_DIAG_ENABLE_MSM_SDIO:
		break;
	case CY_AS_DIAG_ENTER_STANDBY:
		cyasdevice_enter_standby();
		break;
	case CY_AS_DIAG_LEAVE_STANDBY:
		cyasdevice_leave_standby();
		break;
	case CY_AS_DIAG_CREATE_BLKDEV:
		retVal = cyasblkdev_blk_init(0, 0);
		break;
	case CY_AS_DIAG_DESTROY_BLKDEV:
		cyasblkdev_blk_exit();
		break;
	case CY_AS_DIAG_SD_MOUNT:
		{
			int	i;
			uint32_t count = 0 ;
			int bus = 1 ;
			cy_as_storage_query_device_data dev_data ;
			cy_as_storage_query_unit_data unit_data = {0} ;
			cyasdevice_leave_standby();
			actdata = (uint8_t *)cy_as_hal_alloc(CYASSTORAGE_MAX_XFER_SIZE);
			expdata = (uint8_t *)cy_as_hal_alloc(CYASSTORAGE_MAX_XFER_SIZE);
			cy_as_hal_mem_set(actdata, 0, CYASSTORAGE_MAX_XFER_SIZE);
			cy_as_hal_mem_set(expdata, 0, CYASSTORAGE_MAX_XFER_SIZE);
			retVal = cy_as_storage_device_control(cyas_hd, bus,
				0, cy_true, cy_false, cy_as_storage_detect_SDAT_3, 0, 0) ;
			if (retVal != CY_AS_ERROR_SUCCESS) {
				cy_as_hal_print_message(KERN_ERR
				"ERROR: Cannot set Device control - Reason code %d\n", retVal) ;
			return retVal;
		}

		/* Start the storage API and get a handle to the device we are interested in. */
		retVal = cy_as_storage_start(cyas_hd, 0, 0) ;
		if (retVal != CY_AS_ERROR_SUCCESS) {
			cy_as_hal_print_message(KERN_ERR
			"ERROR: Cannot start storage stack - Reason code %d\n", retVal) ;
			return retVal;
		}
		retVal = cy_as_storage_query_media(cyas_hd, cy_as_media_sd_flash, &count, 0, 0) ;
		if (retVal != CY_AS_ERROR_SUCCESS) {
			cy_as_hal_print_message(KERN_ERR
			"ERROR: Cannot query SD device count - Reason code %d\n", retVal) ;
			return retVal;
		}
		if (!count) {
			cy_as_hal_print_message(KERN_ERR"ERROR: SD storage media was not found\n") ;
			return retVal;
		} else {
			cy_as_hal_print_message(KERN_ERR
				"SUCCESS: %d SD device(s) found. SD_CLK, SD_CMD, and SD_D0 connected\n",
				count) ;
			dev_data.bus = 1 ;
			dev_data.device = 0 ;
			retVal = cy_as_storage_query_device(cyas_hd, &dev_data, 0, 0);
			if (retVal != CY_AS_ERROR_SUCCESS) {
				cy_as_hal_print_message(KERN_ERR
					"ERROR: Cannot query SD device count - Reason code %d\n", retVal) ;
				return retVal;
			} else {
				#if 1 /* skkm */
				retVal = cy_as_storage_change_sd_frequency(cyas_hd, bus, 0x11, 24, 0, 0);
				if (retVal != CY_AS_ERROR_SUCCESS) {
					cy_as_hal_print_message(
						"%s: Cannot control cy_as_storage_change_sd_frequency - reason [%d]\n", __func__, retVal) ;
				}
				#endif
				cy_as_hal_print_message(KERN_ERR"Checking for SD_WP Connectivity:\n");
				if (dev_data.desc_p.writeable)
					cy_as_hal_print_message(KERN_ERR" SD media is not write protected\n") ;
				else
					cy_as_hal_print_message(KERN_ERR" SD media is write protected %d\n", retVal) ;
				unit_data.device = 0 ;
				unit_data.unit   = 0 ;
				unit_data.bus    = bus;
				retVal = cy_as_storage_query_unit(cyas_hd, &unit_data, 0, 0) ;
				if (retVal != CY_AS_ERROR_SUCCESS) {
				#ifndef WESTBRIDGE_NDEBUG
					cy_as_hal_print_message(KERN_INFO
						"%s: cannot query %d device unit - reason code %d\n",
						__func__, unit_data.bus, retVal) ;
				#endif
				return retVal;
				}
			}
		}
		cy_as_hal_set_ep_dma_mode(4, false);
		cy_as_hal_set_ep_dma_mode(8, false);
		start_unit = unit_data.desc_p.unit_size - MAX_DRQ_LOOPS_IN_ISR*2;
		for (i = 0 ; i < CYASSTORAGE_MAX_XFER_SIZE ; i++)
			expdata[i] = i;
	}
	break;

	case CY_AS_DIAG_SD_READ:
		{
			int i;
			int bus = 1 ;
			struct timespec mStartTime, mEndTime;
			long second, nano;
			long mDelta;

			mStartTime = CURRENT_TIME;
			retVal = cy_as_storage_read(
					cyas_hd, bus, 0, 0, start_unit,
					actdata, MAX_DRQ_LOOPS_IN_ISR) ;
			if (retVal != CY_AS_ERROR_SUCCESS) {
				cy_as_hal_print_message(KERN_ERR
				"ERROR: cannot read from block device - code %d\n",
				retVal) ;
			break;
		}

		mEndTime = CURRENT_TIME;
		second = mEndTime.tv_sec - mStartTime.tv_sec;
		nano = mEndTime.tv_nsec - mStartTime.tv_nsec;
		mDelta = (second*1000000) + nano/1000;
		cy_as_hal_print_message("<1>%s: reading speed = %d KByte/s\n",
			__func__,
			(int)((CYASSTORAGE_MAX_XFER_SIZE*1000)/mDelta)) ;
		if (memcmp(expdata, actdata, 2048) != 0) {
			int errCnt = 0 ;
			cy_as_hal_print_message(KERN_ERR
				"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n") ;
			for (i = 0 ; i < 2048 ; i++) {
				if (expdata[i] != actdata[i]) {
					cy_as_hal_print_message(KERN_ERR "EXP[%d]: 0x%02x\n", i, expdata[i]);
					cy_as_hal_print_message(KERN_ERR "ACT[%d]: 0x%02x\n", i, actdata[i]);
					errCnt++ ;
					if (errCnt > 10)
						break;
				}
			}
			cy_as_hal_print_message(KERN_ERR
				"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n") ;
			retVal = CY_AS_ERROR_INVALID_RESPONSE;
			break;
			} else
				 cy_as_hal_print_message(KERN_ERR "success : storage test\n") ;

		}
		break;
	case CY_AS_DIAG_SD_WRITE:
		{
			int	i;
			int bus = 1 ;
			struct timespec mStartTime, mEndTime;
			long second, nano;
			long mDelta;
			for (i = 0 ; i < CYASSTORAGE_MAX_XFER_SIZE ; i++)
				expdata[i] = i;
			mStartTime = CURRENT_TIME;
			retVal = cy_as_storage_write(cyas_hd, bus, 0, 0, start_unit, expdata, MAX_DRQ_LOOPS_IN_ISR) ;
			if (retVal != CY_AS_ERROR_SUCCESS) {
				cy_as_hal_print_message(KERN_ERR "ERROR: cannot write to block device - code %d\n", retVal) ;
				break;
			}
			mEndTime = CURRENT_TIME;
			second = mEndTime.tv_sec - mStartTime.tv_sec;
			nano = mEndTime.tv_nsec - mStartTime.tv_nsec;
			mDelta = (second*1000000) + nano/1000;
			cy_as_hal_print_message("<1>%s: writing speed = %d KByte/s\n",
				__func__,
			(int)((CYASSTORAGE_MAX_XFER_SIZE*1000)/mDelta)) ;
		}
		break;
	case CY_AS_DIAG_SD_UNMOUNT:
		cy_as_hal_set_ep_dma_mode(4, true);
		cy_as_hal_set_ep_dma_mode(8, true);

		/*  Start the storage API and get a handle to the device we are interested in. */
		retVal = cy_as_storage_stop(cyas_hd, 0, 0) ;
		if (retVal != CY_AS_ERROR_SUCCESS) {
			cy_as_hal_print_message(KERN_ERR"ERROR: Cannot stop storage stack - Reason code %d\n", retVal) ;
			return retVal;
		}
		cy_as_hal_free(actdata);
		cy_as_hal_free(expdata);
		cyasdevice_enter_standby();
		break;
#if 0
	case CY_AS_DIAG_CONNECT_UMS:
		cyasdevice_leave_standby();
		/* cyasblkdev_blk_exit(); */

		cyasdevice_reload_firmware(0);
		/* CyAsHalSelUSBSwitch(1); */
		retVal = CyAsAPIUsbInit();
		if (retVal) {
			cy_as_hal_print_message("%s: USB test failed.\n", __func__) ;
			return 0;
		} else {
			msleep(1000);
			cy_as_hal_print_message("%s: USB connected.\n", __func__) ;
		}
		cy_as_hal_print_message("%s: UMS MODE init done\n", __func__) ;
		break;

	case CY_AS_DIAG_DISCONNECT_UMS:

		CyAsAPIUsbExit();
		/* CyAsHalSelUSBSwitch(0); */
		cy_as_hal_print_message("%s: UMS mode - close done\n", __func__) ;

		cyasdevice_reload_firmware(1);
		/* cyasblkdev_blk_init(0, 0); */
		cyasdevice_enter_standby();
		cy_as_hal_print_message("%s: reload F/W - close done\n", __func__) ;
		break;
	case CY_AS_DIAG_CONNECT_MTP:
		cyasdevice_leave_standby();
		/* CyAsHalSelUSBSwitch(1); */
		cyasblkdev_blk_init(0, 0);
		retVal = cy_as_gadget_init(1);
		if (retVal)
			cy_as_hal_print_message("%s: cy_as_gadget_init failed.\n", __func__) ;
		else
			cy_as_hal_print_message("%s: cy_as_gadget_init success\n", __func__) ;
		cy_as_hal_print_message("%s: Start cy_as_gadget driver -  init done\n", __func__) ;
		break;
	case CY_AS_DIAG_DISCONNECT_MTP:
		cyasblkdev_blk_exit();
		cy_as_gadget_cleanup();
		/* CyAsHalSelUSBSwitch(0); */
		cyasdevice_enter_standby();
		cy_as_hal_print_message("%s: cy_as_gadget driver - close done\n", __func__) ;
		break;
#endif
	case CY_AS_DIAG_TEST_RESET_LOW:
		/* cy_as_hal_set_reset_pin(0); */
		cy_as_hal_print_message("%s: cy_as_hal_set_reset_pin - set LOW\n", __func__) ;
		break;

	case CY_AS_DIAG_TEST_RESET_HIGH:
		/* cy_as_hal_set_reset_pin(1); */
		cy_as_hal_print_message("%s: cy_as_hal_set_reset_pin - set HIGH\n", __func__) ;
		break;
	default:
		cy_as_hal_print_message("%s: unkown mode\n", __func__) ;
		break;

	}
#endif
	return retVal;
}
EXPORT_SYMBOL(cy_as_diagnostics);


/*[]*/
