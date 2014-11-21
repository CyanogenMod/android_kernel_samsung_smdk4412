/*
*
* drivers/media/tdmb/tdmb_port_t3900.c
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

#include "tdmb.h"
#include "INC_INCLUDES.h"

static struct ST_SUBCH_INFO *st_ch_info;
static bool t3900_on_air;
static bool t3900_pwr_on;
static unsigned char stream_buff[INC_INTERRUPT_SIZE+188];

static bool __get_ensemble_info(struct ensemble_info_type *e_info
						, unsigned long freq)
{
	int i;
	int j;
	int sub_i = 0;
	int cnt;
	const char *e_name = NULL;
	struct INC_CHANNEL_INFO *inc_sub_info;

	DPRINTK("t3900_get_ensemble_info - freq(%ld)\n", freq);

	e_info->tot_sub_ch =
		INTERFACE_GETDMB_CNT() + INTERFACE_GETDAB_CNT();
	DPRINTK("total subchannel number : %d\n"
		, e_info->tot_sub_ch);

	if (e_info->tot_sub_ch > 0) {
		e_name =
			(char *)INTERFACE_GETENSEMBLE_LABEL(TDMB_I2C_ID80);

		if (e_name)
			strncpy((char *)e_info->ensem_label,
					(char *)e_name,
					ENSEMBLE_LABEL_MAX);

		e_info->ensem_freq = freq;

		for (i = 0; i < 2; i++) {
			cnt = (i == 0) ? INTERFACE_GETDMB_CNT()
				: INTERFACE_GETDAB_CNT();

			for (j = 0; j < cnt; j++, sub_i++) {
				inc_sub_info = (i == 0)
					? INTERFACE_GETDB_DMB(j)
					: INTERFACE_GETDB_DAB(j);
				e_info->sub_ch[sub_i].sub_ch_id
					= inc_sub_info->ucSubChID;
				e_info->sub_ch[sub_i].start_addr
					= inc_sub_info->uiStarAddr;
				e_info->sub_ch[sub_i].tmid
					= inc_sub_info->uiTmID;
				e_info->sub_ch[sub_i].svc_type
					= inc_sub_info->ucServiceType;
				e_info->sub_ch[sub_i].svc_id
					= inc_sub_info->ulServiceID;
				memcpy(e_info->sub_ch[sub_i].svc_label,
						inc_sub_info->aucLabel,
						SVC_LABEL_MAX);

			}
		}
		return true;
	} else {
		return false;
	}
}

static void t3900_power_off(void)
{
	DPRINTK("t3900_power_off\n");

	if (t3900_pwr_on) {
		t3900_on_air = false;

		INC_STOP(TDMB_I2C_ID80);

		tdmb_control_irq(false);
		tdmb_control_gpio(false);

		vfree(st_ch_info);
		st_ch_info = NULL;

		t3900_pwr_on = false;
	}
}

static bool t3900_power_on(void)
{
	DPRINTK("t3900_power_on\n");

	if (t3900_pwr_on) {
		return true;
	} else {
		st_ch_info = vmalloc(sizeof(struct ST_SUBCH_INFO));
		if (st_ch_info == NULL) {
			return false;
		} else {
			tdmb_control_gpio(true);

			if (INTERFACE_INIT(TDMB_I2C_ID80) != INC_SUCCESS) {
				tdmb_control_gpio(false);

				vfree(st_ch_info);
				st_ch_info = NULL;

				return false;
			} else {
				tdmb_control_irq(true);
				t3900_pwr_on = true;
				return true;
			}
		}
	}
}

static void t3900_get_dm(struct tdmb_dm *info)
{
	if (t3900_pwr_on == true && t3900_on_air == true) {
		INC_STATUS_CHECK(TDMB_I2C_ID80);
		info->rssi = INC_GET_RSSI(TDMB_I2C_ID80);
		info->ber = INC_GET_SAMSUNG_BER(TDMB_I2C_ID80);
		info->antenna = INC_GET_SAMSUNG_ANT_LEVEL(TDMB_I2C_ID80);
		info->per = 0;
	} else {
		info->rssi = 100;
		info->ber = 2000;
		info->per = 0;
		info->antenna = 0;
	}
}

static bool t3900_set_ch(unsigned long freq
						, unsigned char subchid
						, bool factory_test)
{
	unsigned char ret_err;
	bool ret = false;

	if (t3900_pwr_on) {
		st_ch_info->nSetCnt = 1;
		st_ch_info->astSubChInfo[0].ulRFFreq = freq / 1000;
		st_ch_info->astSubChInfo[0].ucSubChID = subchid;
		st_ch_info->astSubChInfo[0].ucServiceType = 0x0;
		if (st_ch_info->astSubChInfo[0].ucSubChID >= 64) {
			st_ch_info->astSubChInfo[0].ucSubChID -= 64;
			st_ch_info->astSubChInfo[0].ucServiceType = 0x18;
		}

		t3900_on_air = false;

		if (factory_test == false)
			ret_err = INTERFACE_START(
				TDMB_I2C_ID80, st_ch_info);
		else
			ret_err = INTERFACE_START_TEST(
				TDMB_I2C_ID80, st_ch_info);

		if (ret_err == INC_SUCCESS) {
			/* TODO Ensemble  good code .... */
			t3900_on_air = true;
			ret = true;
		} else if (ret_err == INC_RETRY) {
			DPRINTK("IOCTL_TDMB_ASSIGN_CH retry\n");

			t3900_power_off();
			t3900_power_on();

			if (factory_test == false)
				ret_err = INTERFACE_START(
					TDMB_I2C_ID80, st_ch_info);
			else
				ret_err = INTERFACE_START_TEST(
					TDMB_I2C_ID80, st_ch_info);

			if (ret_err == INC_SUCCESS) {
				/* TODO Ensemble  good code .... */
				t3900_on_air = true;
				ret = true;
			}
		}
	}

	return ret;
}

static bool t3900_scan_ch(struct ensemble_info_type *e_info
							, unsigned long freq)
{
	if (t3900_pwr_on == false || e_info == NULL)
		return false;
	else if (INTERFACE_SCAN(TDMB_I2C_ID80, (freq / 1000)) == INC_SUCCESS)
		return __get_ensemble_info(e_info, freq);
	else
		return false;
}

static void t3900_pull_data(void)
{
	unsigned short remain_len = INC_INTERRUPT_SIZE;
#if !(INC_INTERRUPT_SIZE <= 0xFFF)
	unsigned short idx = 0;
	unsigned short spi_size = 0xFFF;
#endif

	memset(stream_buff, 0, sizeof(stream_buff));

#if (INC_INTERRUPT_SIZE <= 0xFFF)
	INC_CMD_READ_BURST(
		TDMB_I2C_ID80, APB_STREAM_BASE
		, stream_buff
		, remain_len);
#else
	while (remain_len) {
		if (remain_len >= spi_size) {
			INC_CMD_READ_BURST(
				TDMB_I2C_ID80, APB_STREAM_BASE
				, &stream_buff[idx*spi_size]
				, spi_size);
			idx++;
			remain_len -= spi_size;
		} else {
			INC_CMD_READ_BURST(
				TDMB_I2C_ID80, APB_STREAM_BASE
				, &stream_buff[idx*spi_size]
				, remain_len);
			remain_len = 0;
		}
	}
#endif

	tdmb_store_data(stream_buff, INC_INTERRUPT_SIZE);
}

static unsigned long t3900_int_size(void)
{
	return INC_INTERRUPT_SIZE;
}

static struct tdmb_drv_func inc_t3900_drv_func = {
	.power_on = t3900_power_on,
	.power_off = t3900_power_off,
	.scan_ch = t3900_scan_ch,
	.get_dm = t3900_get_dm,
	.set_ch = t3900_set_ch,
	.pull_data = t3900_pull_data,
	.get_int_size = t3900_int_size,
};

struct tdmb_drv_func *t3900_drv_func(void)
{
	DPRINTK("tdmb_get_drv_func : t3900\n");
	return &inc_t3900_drv_func;
}
