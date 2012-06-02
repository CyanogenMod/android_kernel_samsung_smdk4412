/*
*
* drivers/media/tdmb/tdmb_port_fc8050.c
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
#include "dmbdrv_wrap_fc8050.h"

static bool fc8050_on_air;
static bool fc8050_pwr_on;

static bool __get_ensemble_info(struct ensemble_info_type *e_info
							, unsigned long freq)
{
	int i;
	int j;
	int sub_i = 0;
	int cnt;
	const char *ensembleName = NULL;
	struct sub_channel_info_type *fci_sub_info;

	DPRINTK("%s : freq(%ld)\n", __func__, freq);

	e_info->tot_sub_ch
		= dmb_drv_get_dmb_sub_ch_cnt() + dmb_drv_get_dab_sub_ch_cnt();
	DPRINTK("total subchannel number : %d\n", e_info->tot_sub_ch);

	if (e_info->tot_sub_ch > 0) {
		ensembleName = (char *)dmb_drv_get_ensemble_label();

		if (ensembleName)
			strncpy((char *)e_info->ensem_label,
					(char *)ensembleName,
					ENSEMBLE_LABEL_MAX);

		e_info->ensem_freq = freq;

		for (i = 0; i < 2; i++) {
			cnt = (i == 0)
				? dmb_drv_get_dmb_sub_ch_cnt()
				: dmb_drv_get_dab_sub_ch_cnt();

			for (j = 0; j < cnt; j++, sub_i++) {
				fci_sub_info = (i == 0)
					? dmb_drv_get_fic_dmb(j)
					: dmb_drv_get_fic_dab(j);

				e_info->ensem_id
					= fci_sub_info->uiEnsembleID;
				e_info->sub_ch[sub_i].sub_ch_id
					= fci_sub_info->ucSubchID;
				e_info->sub_ch[sub_i].start_addr
					= fci_sub_info->uiStartAddress;
				e_info->sub_ch[sub_i].tmid
					= fci_sub_info->ucTMId;
				e_info->sub_ch[sub_i].svc_type
					= fci_sub_info->ucServiceType;
				e_info->sub_ch[sub_i].svc_id
					= fci_sub_info->ulServiceID;
				if (i == 0)
					memcpy(
						e_info->sub_ch[sub_i].svc_label,
						dmb_drv_get_sub_ch_dmb_label(j),
						SVC_LABEL_MAX);
				else
					memcpy(e_info->sub_ch[sub_i].svc_label,
						dmb_drv_get_sub_ch_dab_label(j),
						SVC_LABEL_MAX);

			}
		}
		return true;
	} else {
		return false;
	}
}

static void fc8050_power_off(void)
{
	DPRINTK("call TDMB_PowerOff !\n");

	if (fc8050_pwr_on) {
		fc8050_on_air = false;

		dmb_drv_deinit();

		tdmb_control_irq(false);
		tdmb_control_gpio(false);

		fc8050_pwr_on = false;
	}
}

static bool fc8050_power_on(void)
{
	DPRINTK("fc8050_power_on\n");

	if (fc8050_pwr_on) {
		return true;
	} else {
		tdmb_control_gpio(true);

		if (dmb_drv_init() == TDMB_FAIL) {
			tdmb_control_gpio(false);
			return false;
		} else {
			tdmb_control_irq(true);
			fc8050_pwr_on = true;
			return true;
		}
	}
}
static void fc8050_get_dm(struct tdmb_dm *info)
{
	if (fc8050_pwr_on == true && fc8050_on_air == true) {
		info->rssi = dmb_drv_get_rssi();
		info->ber = dmb_drv_get_ber();
		info->antenna = dmb_drv_get_ant();
		info->per = 0;
	} else {
		info->rssi = 100;
		info->ber = 2000;
		info->per = 0;
		info->antenna = 0;
	}
}

static bool fc8050_set_ch(unsigned long freq,
							unsigned char sub_ch_id,
							bool factory_test)
{
	unsigned long freq_temp = freq / 1000;
	unsigned char sub_ch_id_temp = sub_ch_id % 1000;
	unsigned char svc_type_temp = 0x0;

	if (sub_ch_id_temp >= 64) {
		sub_ch_id_temp -= 64;
		svc_type_temp  = 0x18;
	}

	DPRINTK("fc8050_set_ch freq:%ld, sub_ch_id:%d, svc_type:%d\n",
			freq_temp, sub_ch_id_temp, svc_type_temp);

	fc8050_on_air = false;

	if (dmb_drv_set_ch(freq_temp, sub_ch_id_temp, svc_type_temp) == 1) {
		DPRINTK("dmb_drv_set_ch Success\n");
		fc8050_on_air = true;
		return true;
	} else {
		DPRINTK("dmb_drv_set_ch Fail\n");
		return false;
	}
}

static bool fc8050_scan_ch(struct ensemble_info_type *e_info
							, unsigned long freq)
{
	if (fc8050_pwr_on == false || e_info == NULL)
		return false;
	else if (dmb_drv_scan_ch((freq / 1000)) == TDMB_SUCCESS)
		return __get_ensemble_info(e_info, freq);
	else
		return false;
}

static unsigned long fc8050_int_size(void)
{
	return 188*40;
}

static struct tdmb_drv_func fci_fc8050_drv_func = {
	.power_on = fc8050_power_on,
	.power_off = fc8050_power_off,
	.scan_ch = fc8050_scan_ch,
	.get_dm = fc8050_get_dm,
	.set_ch = fc8050_set_ch,
	.pull_data = dmb_drv_isr,
	.get_int_size = fc8050_int_size,
};

struct tdmb_drv_func *fc8050_drv_func(void)
{
	DPRINTK("tdmb_get_drv_func : fc8050\n");
	return &fci_fc8050_drv_func;
}
