/*
*
* drivers/media/tdmb/tdmb_port_tcc3170.c
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
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include "tdmb.h"

#include "tcpal_os.h"
#include "tcpal_queue.h"

#include "tcbd_feature.h"
#include "tcbd_api_common.h"
#include "tcbd_drv_io.h"
#include "tcbd_stream_parser.h"
#include "tcc_fic_decoder.h"
#include "tcbd_drv_ip.h"
#include "tcc_fic_decoder.h"

#include "tcbd_hal.h"

/* #define TDMB_DEBUG_SCAN */

static DEFINE_MUTEX(tcc3170_mutex);

static DECLARE_WAIT_QUEUE_HEAD(wq);
static bool scan_done;

static bool tcc3170_on_air;
static bool tcc3170_pwr_on;
static struct tcbd_device tcc3170_device;
static unsigned char prev_subch = 0xFF;

#ifdef TDMB_DEBUG_SCAN
static void __print_ensemble_info(struct ensemble_info_type *e_info)
{
	int i = 0;

	DPRINTK("ensem_freq(%ld)\n", e_info->ensem_freq);
	DPRINTK("ensem_label(%s)\n", e_info->ensem_label);
	for (i = 0; i < e_info->tot_sub_ch; i++) {
		DPRINTK("[%d]\n", i);
		DPRINTK("svc_label(%s)\n", e_info->sub_ch[i].svc_label);
		DPRINTK("sub_ch_id(0x%x)\n", e_info->sub_ch[i].sub_ch_id);
		DPRINTK("start_addr(0x%x)\n", e_info->sub_ch[i].start_addr);
		DPRINTK("tmid(0x%x)\n", e_info->sub_ch[i].tmid);
		DPRINTK("svc_type(0x%x)\n", e_info->sub_ch[i].svc_type);
	}
}
#endif

static bool __get_ensemble_info(struct ensemble_info_type *e_info
							, unsigned long freq)
{
	struct tcc_ensemble_info *ensbl_info;
	struct tcc_ensemble *esbl = &ensbl_info->ensbl;
	struct tcc_service_info *svc_info;
	struct tcc_service_comp_info *svc_comp_info;

	unsigned char i;
	unsigned char cnt = 0;

	ensbl_info = tcc_fic_get_ensbl_info(1);
	esbl = &ensbl_info->ensbl;

	memset(e_info, 0, sizeof(e_info));

	e_info->ensem_freq = freq;
	e_info->ensem_id = esbl->eid;
	strncpy(e_info->ensem_label, esbl->label, \
		ENSEMBLE_LABEL_MAX);

	for (i = 0; i < esbl->num_svc; i++) {
		svc_info = &ensbl_info->svc_info[i];
		svc_comp_info = &svc_info->svc_comp_info[0];
		if ((svc_comp_info->svc_comp.tmid == 0x00 &&
			svc_comp_info->svc_comp.ascty_dscty == 0x00) ||
			(svc_comp_info->svc_comp.tmid == 0x01 &&
			svc_comp_info->svc_comp.ascty_dscty == 0x18)) {
				e_info->sub_ch[cnt].sub_ch_id = \
					svc_comp_info->sub_ch.subch_id;
			    e_info->sub_ch[cnt].start_addr = \
					svc_comp_info->sub_ch.start_cu;
			    e_info->sub_ch[cnt].tmid = \
					svc_comp_info->svc_comp.tmid;
			    e_info->sub_ch[cnt].svc_type = \
					svc_comp_info->svc_comp.ascty_dscty;
			    e_info->sub_ch[cnt].svc_id = \
					svc_comp_info->svc_comp.scid;
			    strncpy(e_info->sub_ch[cnt].svc_label, \
					svc_info->svc.svc_label, SVC_LABEL_MAX);
			cnt++;
		}
	}
	e_info->tot_sub_ch = cnt;

#ifdef TDMB_DEBUG_SCAN
	__print_ensemble_info(e_info);
#endif

	return (e_info->tot_sub_ch > 0) ? true : false;
}

static void tcc3170_power_off(void)
{
	DPRINTK("call TDMB_PowerOff !\n");

	mutex_lock(&tcc3170_mutex);

	if (tcc3170_pwr_on) {
		tcc3170_on_air = false;

		if (prev_subch != 0xFF) {
			tcbd_unregister_service(&tcc3170_device, prev_subch);
			prev_subch = 0xFF;
		}

		tcbd_io_close(&tcc3170_device);

		tdmb_control_irq(false);
		tdmb_control_gpio(false);

		tcc3170_pwr_on = false;
	}

	mutex_unlock(&tcc3170_mutex);

}

static bool tcc3170_power_on(void)
{
	unsigned char chip_id = 0;
	s32 ret;

	DPRINTK("tcc3170_power_on\n");

	if (tcc3170_pwr_on) {
		return true;
	} else {
		tdmb_control_gpio(true);

		/* command io open */
		ret = tcbd_io_open(&tcc3170_device);
		if (ret < 0) {
			DPRINTK("failed io open %d\n", (int)ret);
			tdmb_control_gpio(false);
			return false;
		}

		/* check chip id */
		tcbd_reg_read(&tcc3170_device, TCBD_CHIPID, &chip_id);
		DPRINTK("chip id : 0X%X\n", chip_id);
		if (chip_id != TCBD_CHIPID_VALUE) {
			DPRINTK("invalid chip id !!! exit!\n");
			tcbd_io_close(&tcc3170_device);
			tdmb_control_gpio(false);
			return false;
		} else {
#ifdef CONFIG_TDMB_SPI
			tcbd_select_peri(&tcc3170_device, PERI_TYPE_SPI_ONLY);
			tcbd_change_irq_mode \
				(&tcc3170_device, INTR_MODE_LEVEL_LOW);
#else
#error
#endif /* CONFIG_TDMB_SPI */
			if (tcbd_device_start \
				(&tcc3170_device, CLOCK_19200KHZ) < 0) {
				DPRINTK("could not start device!!\n");
				tcbd_io_close(&tcc3170_device);
				tdmb_control_gpio(false);
				return false;
			} else {
				tdmb_control_irq(true);
				tcc3170_pwr_on = true;
				return true;
			}
		}
	}
}


static s32 tcbd_get_ant_level(u32 pcber, s32 rssi)
{
	s32 ant_level = 0;

	s32 ant75[] = {300, 450, 600, 800, 950, 1150, };
	s32 ant80[] = {-1,  450, 600, 800, 950, 1150, };
	s32 ant85[] = {-1,  -1,  600, 800, 900, 1150, };
	s32 ant90[] = {-1,  -1,  -1,  800, 950, 1150, };
	s32 ant100[] = {-1, -1,  -1,  -1,  950, 1150, };
	s32 *ant = ant75;

	if (rssi > -75)
		ant = ant75;
	else if (rssi > -80)
		ant = ant80;
	else if (rssi > -85)
		ant = ant85;
	else if (rssi > -90)
		ant = ant90;
	else if (rssi > -100)
		ant = ant100;

	if ((s32)pcber <= ant[0])
		ant_level = 6;
	else if ((s32)pcber <= ant[1])
		ant_level = 5;
	else if ((s32)pcber <= ant[2])
		ant_level = 4;
	else if ((s32)pcber <= ant[3])
		ant_level = 3;
	else if ((s32)pcber <= ant[4])
		ant_level = 2;
	else if ((s32)pcber <= ant[5])
		ant_level = 1;
	else if ((s32)pcber > ant[5])
		ant_level = 0;

	return ant_level;
}

static void tcc3170_get_dm(struct tdmb_dm *info)
{
	struct tcbd_status_data status;

	if (tcc3170_pwr_on == true && tcc3170_on_air == true) {
		tcbd_read_signal_info(&tcc3170_device, &status);

		info->ber = status.pcber_moving_avg;
		info->rssi = status.rssi;
		info->antenna = tcbd_get_ant_level(status.pcber, status.rssi);
		info->per = status.tsper;
	} else {
		info->rssi = 100;
		info->ber = 3000;
		info->per = 100;
		info->antenna = 0;
	}
}

static s32 __set_frequency(unsigned long freqKHz, bool scan_mode)
{
	s32 ret = 0;
	u8 status;

	tcbd_disable_irq(&tcc3170_device, 0);
	tcc_fic_parser_init();
	ret = tcbd_tune_frequency(&tcc3170_device, freqKHz, 1500);
	if (ret < 0) {
		DPRINTK("tcbd_tune_frequency fail %d\n", ret);
		return -EFAULT;
	}

	if (scan_mode) {
		ret = tcbd_wait_tune(&tcc3170_device, &status);
		if (ret < 0) {
			DPRINTK("wait_tune fail status:0x%02X, ret:0x%x\n", \
				status, ret);
			ret =  -EFAULT;
		}
	}
	DPRINTK("%s : status:0x%02X, ret:0x%x\n", __func__, status, ret);

	if (ret == TCERR_SUCCESS) {
#if defined(__CSPI_ONLY__)
		tcbd_enable_irq(&tcc3170_device, TCBD_IRQ_EN_DATAINT);
#else
#error
#endif
	}
	return ret;
}

static bool tcc3170_set_ch(unsigned long freq,
							unsigned char sub_ch_id,
							bool factory_test)
{
	unsigned long freq_temp = freq / 1000;
	unsigned char sub_ch_id_temp = sub_ch_id % 1000;
	s32 ret;

	DPRINTK("tcc3170_set_ch freq:%ld, sub_ch_id:%d\n",
			freq_temp, sub_ch_id_temp);

	tcc3170_on_air = false;

	if (prev_subch != 0xFF) {
		tcbd_unregister_service(&tcc3170_device, prev_subch);
		prev_subch = 0xFF;
	}

	if (__set_frequency((freq/1000), true) == TCERR_SUCCESS) {
		if (sub_ch_id_temp >= 64) {
			sub_ch_id_temp -= 64;
			ret = tcbd_register_service(
				&tcc3170_device, sub_ch_id_temp, 1);
		} else {
			ret = tcbd_register_service(
				&tcc3170_device, sub_ch_id_temp, 0);
		}
		if (ret == TCERR_SUCCESS) {
			DPRINTK("dmb_drv_set_ch Success\n");
			prev_subch = sub_ch_id_temp - 64;
			tcc3170_on_air = true;
		}
	}

	if (tcc3170_on_air == false)
		DPRINTK("dmb_drv_set_ch Fail\n");

	return tcc3170_on_air;
}

static bool tcc3170_scan_ch(struct ensemble_info_type *e_info
							, unsigned long freq)
{
	bool ret = false;

	if (tcc3170_pwr_on == true && e_info != NULL) {
		if (prev_subch != 0xFF) {
			tcbd_unregister_service(&tcc3170_device, prev_subch);
			prev_subch = 0xFF;
		}

		tcc_fic_parser_init();
		scan_done = false;
		if (__set_frequency((freq/1000), true) == TCERR_SUCCESS) {
			msleep(1200); /* optimize this point */

			if (scan_done == true)
				ret = __get_ensemble_info(e_info, freq);
		}
	}

	return ret;
}

static unsigned long tcc3170_int_size(void)
{
	return TCBD_MAX_THRESHOLD * 2;
}

static void tcc3170_pull_data(void)
{
	s32 size, ret = 0;
	s8 irq_status;
	s8 irq_error;
	static u8 buff_read[TCBD_MAX_THRESHOLD*2];

	if (!tcc3170_pwr_on)
		return;

	mutex_lock(&tcc3170_mutex);

	ret = tcbd_read_irq_status(&tcc3170_device, &irq_status, &irq_error);
	ret |= tcbd_clear_irq(&tcc3170_device, irq_status);
	ret |= tcbd_read_stream(&tcc3170_device, buff_read, &size);
	if (ret == 0 && !irq_error) {
		tcbd_split_stream(0, buff_read, size);
	} else {
		DPRINTK("### buffer is full, skip the data "
			"(ret:%d, status=0x%02X, error=0x%02X)  ###\n",
				ret, irq_status, irq_error);

		tcbd_init_stream_data_config(&tcc3170_device,
			ENABLE_CMD_FIFO,
			tcc3170_device.selected_buff,
			tcc3170_device.intr_threshold);
		/*tcbd_reset_ip(device, TCBD_SYS_COMP_ALL, TCBD_SYS_COMP_EP);*/
		tcbd_init_parser(0, NULL);
	}
	mutex_unlock(&tcc3170_mutex);
}

static s32 __stream_callback(
	s32 _dev_idx,
	u8 *_stream,
	s32 _size,
	u8 _subch_id,
	u8 _type)
{
	u8 *stream;
	s32 size;
	s32 ret, i = 0;

#ifdef __CALLBACK_BUFFER_HEADER__
	stream = _stream + SIZE_BUFF_HEADER;
	size = _size - SIZE_BUFF_HEADER;
#else
	stream = _stream;
	size = _size;
#endif

	/* DPRINTK("%s : _type %d size %d\n", __func__, _type, size); */

	switch (_type) {
	case DATA_TYPE_FIC: /*FIC*/
		for (i = 0; i < size/TCBD_FIC_SIZE; i++) {
			ret = tcc_fic_run_decoder(stream + (i*TCBD_FIC_SIZE),
					MAX_FIC_SIZE);
			if (ret > 0)
				scan_done = true;
		}
		break;
	case DATA_TYPE_MSC: /*MSC*/
		tdmb_store_data(stream, size);
		break;
	case DATA_TYPE_STATUS: /*STATUS*/
		tcbd_update_status(stream, size, NULL);
	default:
		break;
	}
	return 0;
}
static bool tcc3170_init(void)
{
#if defined(__CSPI_ONLY__)
	tcpal_set_cspi_io_function();
#else
#error
#endif
	tcbd_init_parser(0, __stream_callback);

	return true;
}

static struct tdmb_drv_func telechips_tcc3170_drv_func = {
	.init = tcc3170_init,
	.power_on = tcc3170_power_on,
	.power_off = tcc3170_power_off,
	.scan_ch = tcc3170_scan_ch,
	.get_dm = tcc3170_get_dm,
	.set_ch = tcc3170_set_ch,
	.pull_data = tcc3170_pull_data,
	.get_int_size = tcc3170_int_size,
};

struct tdmb_drv_func *tcc3170_drv_func(void)
{
	DPRINTK("tdmb_get_drv_func : tcc3170\n");
	return &telechips_tcc3170_drv_func;
}
