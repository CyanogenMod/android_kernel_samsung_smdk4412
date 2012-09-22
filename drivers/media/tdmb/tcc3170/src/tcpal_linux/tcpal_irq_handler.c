/*
 * tcpal_irq_handler.c
 *
 * Author:  <linux@telechips.com>
 * Description: Telechips broadcast driver
 *
 * Copyright (c) Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <asm/mach-types.h>

#include "tcpal_os.h"
#include "tcpal_debug.h"

#include "tcbd_feature.h"
#include "tcbd_api_common.h"
#include "tcbd_drv_ip.h"

#include "tcbd_stream_parser.h"
#include "tcbd_diagnosis.h"
#include "tcc_fic_decoder.h"
#include "tcbd_hal.h"

struct tcbd_irq_data {
	struct work_struct work;
	struct workqueue_struct *work_queue;
	struct tcbd_device *device;
	u64 start_tick;
	s32 tcbd_irq;
	s32 is_irq_en;
};

static struct tcbd_irq_data tcbd_irq_handler_data;

#if defined(__CSPI_ONLY__)
static inline void tcpal_split_stream(struct tcbd_irq_data *irq_data)
{
	s32 size, ret = 0;
	s8 irq_status;
	s8 irq_error;
	static u8 buff_read[TCBD_MAX_THRESHOLD*2];
	struct tcbd_device *device = irq_data->device;

	ret = tcbd_read_irq_status(device, &irq_status, &irq_error);
	ret |= tcbd_clear_irq(device, irq_status);

	ret |= tcbd_read_stream(device, buff_read, &size);
	if (ret == 0 && !irq_error)
		tcbd_split_stream(0, buff_read, size);
	else {
		tcbd_debug(DEBUG_ERROR, "### buffer is full, skip the data "
			"(ret:%d, status=0x%02X, error=0x%02X, %d)  ###\n",
			ret, irq_status, irq_error,
			(s32)tcpal_diff_time(irq_data->start_tick));

		tcbd_init_stream_data_config(device,
			ENABLE_CMD_FIFO,
			device->selected_buff,
			device->intr_threshold);
		/*tcbd_reset_ip(device, TCBD_SYS_COMP_ALL, TCBD_SYS_COMP_EP);*/
		tcbd_init_parser(0, NULL);
	}
}

static void tcpal_work_parse_stream(struct work_struct *_param)
{
	u64 diff = tcpal_diff_time(tcbd_irq_handler_data.start_tick);
	struct tcbd_irq_data *irq_data =
		container_of(_param, struct tcbd_irq_data, work);

	/* for checking delay of workqueue */
	if (diff > 10)
		tcbd_debug(DEBUG_INTRRUPT, "diff work start and process :%d\n",
						(s32)diff);
	tcpal_split_stream(irq_data);
	enable_irq(irq_data->tcbd_irq);
}


s32 start_tune;
static s32 tcpal_irq_stream_callback(
	s32 _dev_idx,
	u8 *_stream,
	s32 _size,
	u8 _subch_id,
	u8 _type)
{
	/*static u64 time = 0;*/
	/*struct tcbd_status_data status;*/
	s32 ret, i = 0;

	switch (_type) {
	case 0: /*MSC*/
		/* write your own code!!*/
	case 1: /*FIC*/
		/* write your own code!!*/
		if (!start_tune) /* set by tune_frequency*/
			goto skip_fic_parse;

		for (i = 0; i < _size / TCBD_FIC_SIZE; i++) {
			ret = tcc_fic_run_decoder(_stream + (i * TCBD_FIC_SIZE),
					MAX_FIC_SIZE);
			if (ret > 0) {
				tcc_fic_get_ensbl_info(1);
				start_tune = 0;
				tcc_fic_parser_init();
			}
		}
		/*tcbd_read_signal_info(tcbd_irq_handler_data.device, &status);
		tcbd_debug(DEBUG_ERROR,
			"PCBER:%d, SNR:%d, RSSI:%d, VBER:%d, TSPER:%d\n",
				status.pcber, status.snr, status.rssi,
				status.vber, status.tsper);*/
		/*if (tcpal_diff_time(time) > 1000) {
			tcbd_check_dsp_status(tcbd_irq_handler_data.device);
			time = tcpal_get_time();
		}*/
skip_fic_parse:
		/*tcbd_enqueue_data(_stream, _size, _subch_id, _type);*/
		break;
	case 2: /*STATUS*/
		tcbd_debug(DEBUG_STATUS, "status size:%d\n", _size);
		tcbd_update_status(_stream, _size, NULL);
		break;
	default:
		break;
	}
	return 0;
}
#endif /*__CSPI_ONLY__*/

#if defined(__I2C_STS__)
static void tcpal_work_read_fic(struct work_struct *_param)
{
	s32 size = TCBD_FIC_SIZE, ret;
	u8 buff[TCBD_FIC_SIZE];
	u64 diff;
	struct tcbd_irq_data *irq_data = container_of(_param,
						struct tcbd_irq_data, work);
	struct tcbd_device *device = irq_data->device;

	diff = tcpal_diff_time(irq_data->start_tick);
	tcbd_debug(DEBUG_INTRRUPT, "work delay :%d\n", (u32)diff);

	ret = tcbd_read_fic_data(device, buff, size);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "failed to read fic! %d\n", ret);
		goto exit_work;
	}

	tcbd_enqueue_data(buff, size, 0, 1);
	if (!start_tune) /* set by tune_frequency*/
		goto exit_work;

	ret = tcc_fic_run_decoder(buff, MAX_FIC_SIZE);
	if (ret > 0) {
		tcc_fic_get_ensbl_info(1);
		start_tune = 0;
		tcc_fic_parser_init();
	}
exit_work:
	enable_irq(irq_data->tcbd_irq);
}
#endif /*__I2C_STS__*/

static irqreturn_t tcpal_irq_handler(s32 _irq, void *_param)
{
	struct tcbd_irq_data *irq_data = (struct tcbd_irq_data *)_param;
	struct tcbd_device *device = irq_data->device;

	disable_irq_nosync(irq_data->tcbd_irq);
	if (device->is_pal_irq_en) {
		irq_data->start_tick = tcpal_get_time();
		queue_work(irq_data->work_queue, &irq_data->work);
		tcbd_debug(DEBUG_INTRRUPT, "\n");
	}
	return IRQ_HANDLED;
}

s32 tcpal_irq_register_handler(void *_device)
{
	s32 ret;

	tcbd_irq_handler_data.work_queue =
		create_singlethread_workqueue("tdmb_work");
	tcbd_irq_handler_data.device = (struct tcbd_device *)_device;
#if defined(__USE_TC_CPU__)
	tcbd_irq_handler_data.tcbd_irq = IRQ_TC317X;
#endif /*__USE_TC_CPU__*/

#if defined(__I2C_STS__)
	INIT_WORK(&tcbd_irq_handler_data.work, tcpal_work_read_fic);
#elif defined(__CSPI_ONLY__)
	INIT_WORK(&tcbd_irq_handler_data.work, tcpal_work_parse_stream);
	tcbd_init_parser(0, tcpal_irq_stream_callback);
#endif /*__CSPI_ONLY__*/

	ret = request_irq(tcbd_irq_handler_data.tcbd_irq, tcpal_irq_handler,
			IRQF_TRIGGER_FALLING | IRQF_DISABLED,
				"tdmb_irq", &tcbd_irq_handler_data);
	tcbd_debug(DEBUG_INTRRUPT, "request_irq : %d\n", (int)ret);
	return ret;
}

s32 tcpal_irq_unregister_handler(void)
{
	disable_irq(tcbd_irq_handler_data.tcbd_irq);
	free_irq(tcbd_irq_handler_data.tcbd_irq, NULL);
	flush_workqueue(tcbd_irq_handler_data.work_queue);
	destroy_workqueue(tcbd_irq_handler_data.work_queue);
	return 0;
}

s32 tcpal_irq_enable(void)
{
#if defined(__CSPI_ONLY__)
	tcbd_init_parser(0, NULL);
#endif /*__CSPI_ONLY__*/
	tcbd_debug(DEBUG_INTRRUPT, "\n");
	/* enable_irq(tcbd_irq_handler_data.tcbd_irq); */
	return 0;
}

s32 tcpal_irq_disable(void)
{
	tcbd_debug(DEBUG_INTRRUPT, "\n");
	/* disable_irq(tcbd_irq_handler_data.tcbd_irq); */
	return 0;
}
