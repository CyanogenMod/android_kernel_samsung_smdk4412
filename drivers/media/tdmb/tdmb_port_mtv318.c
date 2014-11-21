/*
*
* drivers/media/tdmb/tdmb_port_mtv318.c
*
* tdmb driver
*
* Copyright (C) (2011, Samsung Electronics)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fcntl.h>

#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#include <linux/fs.h>
#include <linux/uaccess.h>

#include <linux/time.h>
#include <linux/timer.h>

#include <linux/vmalloc.h>

#include "raontv.h"
#include "raontv_internal.h"
#include "raontv_ficdec_internal.h"
#include "raontv_ficdec.h"

#include "tdmb.h"

#define MTV318_INTERRUPT_SIZE (188*10)
/* #define TDMB_DEBUG_SCAN */

static bool mtv318_on_air;
static bool mtv318_pwr_on;
static unsigned char stream_buff[MTV318_INTERRUPT_SIZE + 1];

static void __print_ensemble_info(struct ensemble_info_type *e_info)
{
	int i = 0;

	DPRINTK("ensem_freq(%ld)\n", e_info->ensem_freq);
	DPRINTK("ensem_label(%s)\n", e_info->ensem_label);
	for (i = 0; i < e_info->tot_sub_ch; i++) {
		DPRINTK("sub_ch_id(0x%x)\n", e_info->sub_ch[i].sub_ch_id);
		DPRINTK("start_addr(0x%x)\n", e_info->sub_ch[i].start_addr);
		DPRINTK("tmid(0x%x)\n", e_info->sub_ch[i].tmid);
		DPRINTK("svc_type(0x%x)\n", e_info->sub_ch[i].svc_type);
		DPRINTK("svc_label(%s)\n", e_info->sub_ch[i].svc_label);
	}
}

static bool __get_ensemble_info(struct ensemble_info_type *e_info
						, unsigned long freq)
{
	DPRINTK("%s\n", __func__);
	memcpy(e_info,
		rtvFICDEC_GetEnsembleInfo(freq),
		sizeof(struct ensemble_info_type));

	if (e_info->tot_sub_ch) {
#ifdef TDMB_DEBUG_SCAN
		__print_ensemble_info(e_info);
#endif
		return true;
	} else {
		return false;
	}
}

static void mtv318_power_off(void)
{
	DPRINTK("mtv318_power_off\n");

	if (mtv318_pwr_on) {
		mtv318_on_air = false;
		tdmb_control_irq(false);
		tdmb_control_gpio(false);

		mtv318_pwr_on = false;

		RTV_GUARD_DEINIT;
	}
}

static bool mtv318_power_on(void)
{
	DPRINTK("mtv318_power_on\n");

	if (mtv318_pwr_on) {
		return true;
	} else {
		tdmb_control_gpio(true);

		if (rtvTDMB_Initialize(RTV_COUNTRY_BAND_KOREA) != RTV_SUCCESS) {
			tdmb_control_gpio(false);
			return false;
		} else {
			tdmb_control_irq(true);
			mtv318_pwr_on = true;
			return true;
		}
	}
}

static void mtv318_get_dm(struct tdmb_dm *info)
{
	if (mtv318_pwr_on == true && mtv318_on_air == true) {
		info->rssi = (rtvTDMB_GetRSSI() / RTV_TDMB_RSSI_DIVIDER);
		info->per = rtvTDMB_GetPER();
		info->ber = rtvTDMB_GetCER();
		info->antenna = rtvTDMB_GetAntennaLevel(info->ber);
	} else {
		info->rssi = 100;
		info->ber = 2000;
		info->per = 0;
		info->antenna = 0;
	}
}

static bool mtv318_set_ch(unsigned long freq
						, unsigned char subchid
						, bool factory_test)
{
	bool ret = false;
	enum E_RTV_TDMB_SERVICE_TYPE eServiceType;

	DPRINTK("%s : %ld %d\n", __func__, freq, subchid);

	if (mtv318_pwr_on) {
		if (subchid >= 64) {
			subchid -= 64;
			eServiceType = RTV_TDMB_SERVICE_VIDEO;
		} else {
			eServiceType = RTV_TDMB_SERVICE_AUDIO;
		}

		ret = rtvTDMB_OpenSubChannel((freq/1000),
							subchid,
							eServiceType,
							MTV318_INTERRUPT_SIZE);
		if (ret == RTV_SUCCESS || ret == RTV_NO_MORE_SUB_CHANNEL) {
			mtv318_on_air = true;
			ret = TRUE;
			DPRINTK("mtv318_set_ch Success\n");
		} else {
			DPRINTK("mtv318_set_ch Fail\n");
		}
	}

	return ret;
}

static bool mtv318_scan_ch(struct ensemble_info_type *e_info
							, unsigned long freq)
{
	bool ret = false;

	if (mtv318_pwr_on == true && e_info != NULL) {
		if (rtvTDMB_ScanFrequency(freq/1000) == RTV_SUCCESS) {
			rtvTDMB_OpenFIC();
			ret = rtvFICDEC_Decode(freq/1000);
			rtvTDMB_CloseFIC();
			if (ret == true)
				__get_ensemble_info(e_info, (freq));
		}
	}

	return ret;
}

static void mtv318_pull_data(void)
{
	unsigned char int_type_val1;
	unsigned char int_type_val2;

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(DD_PAGE);
	int_type_val1 = RTV_REG_GET(INT_E_STATL);
	int_type_val2 = RTV_REG_GET(INT_E_STATH);

	/*===========================================
	 * Processing MSC1 interrupt.
	 * T-DMB/DAB: 1 VIDEO data or 1 AUDIO data
	 * 1 seg: 1 VIDEO data
	 * FM: PCM data
	 *===========================================*/
	 /* MSC1 memory overflow or under run */
	if (int_type_val1&(MSC1_E_INT|MSC1_E_OVER_FLOW|MSC1_E_UNDER_FLOW)) {
		if (int_type_val1 & (MSC1_E_OVER_FLOW|MSC1_E_UNDER_FLOW)) {
			rtv_ClearAndSetupMemory_MSC1();
			/* MSC1 Interrupt status clear */
			RTV_REG_SET(INT_E_UCLRL, 0x04);
			DPRINTK("%s : int_type 0x%x\n",
					 __func__, int_type_val1);
		} else {
			/* Get the frame data using CPU or DMA.
			RTV_REG_BURST_GET() macro should implemented by user. */
			RTV_REG_MAP_SEL(MSC1_PAGE);
			RTV_REG_BURST_GET(0x10, stream_buff,
				MTV318_INTERRUPT_SIZE+1);

			RTV_REG_MAP_SEL(DD_PAGE);
			/* MSC1 Interrupt status clear */
			RTV_REG_SET(INT_E_UCLRL, 0x04);

			tdmb_store_data(&stream_buff[1], MTV318_INTERRUPT_SIZE);
		}
	}

	RTV_GUARD_FREE;
}

static unsigned long mtv318_int_size(void)
{
	return MTV318_INTERRUPT_SIZE;
}

static bool mtv318_init(void)
{
	RTV_GUARD_INIT;
	return true;
}

static struct tdmb_drv_func raontech_mtv318_drv_func = {
	.init = mtv318_init,
	.power_on = mtv318_power_on,
	.power_off = mtv318_power_off,
	.scan_ch = mtv318_scan_ch,
	.get_dm = mtv318_get_dm,
	.set_ch = mtv318_set_ch,
	.pull_data = mtv318_pull_data,
	.get_int_size = mtv318_int_size,
};

struct tdmb_drv_func *mtv318_drv_func(void)
{
	DPRINTK("tdmb_get_drv_func : mtv318\n");
	return &raontech_mtv318_drv_func;
}
