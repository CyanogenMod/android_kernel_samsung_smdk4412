/*
 * tcbd_api_common.c
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

#include "tcpal_os.h"
#include "tcpal_debug.h"

#include "tcbd_feature.h"
#include "tcbd_api_common.h"
#include "tcbd_drv_ip.h"
#include "tcbd_drv_io.h"
#include "tcbd_drv_rf.h"

#include "tcc317x_boot_tdmb.h"

s32 tcbd_device_start(struct tcbd_device *_device,
			enum tcbd_clock_type _clock_type)
{
	s32 ret = 0, ver;
	u8 *dsp_rom = TCC317X_BOOT_DATA_TDMB;
	s32 size = TCC317X_BOOT_SIZE_TDMB;
#if defined(__DEBUG_DSP_ROM__)
	static u8 dsp_rom_buff[MAX_SIZE_DSP_ROM];
	if (tcbd_debug_rom_from_fs() == 1) {
		ret = tcbd_read_file(_device, tcbd_debug_rom_path(),
					dsp_rom_buff, MAX_SIZE_DSP_ROM);
		if (ret > 0) {
			dsp_rom = dsp_rom_buff;
			size = ret;
			tcbd_debug(DEBUG_ERROR, "rom:%s, size:%d\n",
						tcbd_debug_rom_path(), size);
		}
	}
#endif /*__DEBUG_DSP_ROM__*/

	/* Initialize PLL */
	tcbd_init_pll(_device, _clock_type);
	tcbd_init_buffer_region(_device);
	tcbd_init_div_io(_device, DIV_IO_TYPE_SINGLE);
	tcbd_enable_buffer(_device);
	tcbd_enable_peri(_device);
	ret = tcbd_init_dsp(_device, dsp_rom, size);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "failed to initialize dsp!! "
						"error:%d\n", ret);
		return -TCERR_FATAL_ERROR;
	} else {
		tcbd_get_rom_version(_device, &ver);
		tcbd_debug(DEBUG_API_COMMON, "device start success!! "
						"version:0x%X\n", ver);
	}
	return 0;
}

s32 tcbd_enable_irq(struct tcbd_device *_device, u8 _en_bit)
{
	s32 ret = 0;

	if (!_device->is_pal_irq_en) {
		tcpal_irq_enable();
		_device->is_pal_irq_en = 1;
	}
	ret |= tcbd_reg_write(_device, TCBD_IRQ_STAT_CLR, TCBD_IRQ_STATCLR_ALL);
	ret |= tcbd_reg_write(_device, TCBD_IRQ_EN, _en_bit);
	_device->enabled_irq = _en_bit;
	return ret;
}

s32 tcbd_disable_irq(struct tcbd_device *_device, u8 _mask)
{
	s32 ret = 0;

	if (_device->is_pal_irq_en) {
		tcpal_irq_disable();
		_device->is_pal_irq_en = 0;
	}
	ret |= tcbd_reg_write(_device, TCBD_IRQ_STAT_CLR, TCBD_IRQ_STATCLR_ALL);
	ret |= tcbd_reg_write(_device, TCBD_IRQ_EN, _mask);
	return ret;
}

s32 tcbd_read_irq_status(
	struct tcbd_device *_device,
	u8 *_irq_status,
	u8 *_err_status)
{
	s32 ret = 0;
	short status;

	ret = tcbd_reg_write(_device, TCBD_IRQ_LATCH, 0x5E);
	ret |= tcbd_reg_read_burst_cont(
		_device, TCBD_IRQ_STAT_CLR, (u8 *)&status, 2);
	*_irq_status = status & 0xFF;
	*_err_status = (status >> 8) & 0xFF;
	return ret;
}

s32 tcbd_clear_irq(struct tcbd_device *_device, u8 _status)
{
	return tcbd_reg_write(_device, TCBD_IRQ_STAT_CLR, _status);
}

s32 tcbd_change_irq_mode(
	struct tcbd_device *_device, enum tcbd_intr_mode _mode)
{
	u8 mode = TCBD_IRQ_MODE_PAD_ENABLE;
	switch (_mode) {
	case INTR_MODE_LEVEL_HIGH:
		mode |= TCBD_IRQ_MODE_LEVEL | TCBD_IRQ_MODE_RISING;
		break;
	case INTR_MODE_LEVEL_LOW:
		mode |= TCBD_IRQ_MODE_LEVEL | TCBD_IRQ_MODE_FALLING;
		break;
	case INTR_MODE_EDGE_RISING:
		mode |= TCBD_IRQ_MODE_TRIGER | TCBD_IRQ_MODE_RISING;
		break;
	case INTR_MODE_EDGE_FALLING:
		mode |= TCBD_IRQ_MODE_TRIGER | TCBD_IRQ_MODE_FALLING;
		break;
	default:
		mode |= TCBD_IRQ_MODE_LEVEL | TCBD_IRQ_MODE_FALLING;
		break;
	}
	return tcbd_reg_write(_device, TCBD_IRQ_MODE, mode);
}

s32 tcbd_init_stream_data_config(
	struct tcbd_device *_device,
	u8 _use_cmd_fifo,
	u8 _buffer_mask,
	u32 _threshold)
{
	s32 ret = 0;
	u16 threshold = SWAP16((_threshold>>2));

	/* Disable internal buffer */
	ret |= tcbd_reg_write(_device, TCBD_STREAM_CFG0, 0);
	/* Change interrupt threshold */
	ret |= tcbd_reg_write_burst_cont(
		_device, TCBD_STREAM_CFG1, (u8 *)&threshold, 2);

	if (_use_cmd_fifo == ENABLE_CMD_FIFO)
		ret |= tcbd_reg_write(_device, TCBD_STREAM_CFG3, 0x12);
	else
		ret |= tcbd_reg_write(_device, TCBD_STREAM_CFG3, 0x10);

	tcbd_reset_ip(_device, TCBD_SYS_COMP_ALL, TCBD_SYS_COMP_EP);

	/* Enable internal buffer */
	ret |= tcbd_reg_write(_device, TCBD_STREAM_CFG0, _buffer_mask);

	_device->intr_threshold = _threshold;
	_device->selected_buff = _buffer_mask;
	_device->en_cmd_fifo = _use_cmd_fifo;

#if defined(__READ_VARIABLE_LENGTH__)
	_device->size_more_read = 0;
#endif /*__READ_VARIABLE_LENGTH__*/
	return ret;
}

#if defined(__CSPI_ONLY__)
#if defined(__READ_FIXED_LENGTH__)
s32 tcbd_read_stream(struct tcbd_device *_device, u8 *_buff, s32 *_size)
{
	u32 bytes_read = _device->intr_threshold;

	if (bytes_read <= 0 || _buff == NULL)
		return -TCERR_INVALID_ARG;
/*
	tcbd_reg_write(_device, TCBD_STREAM_CFG3, 0x22);
	tcbd_reg_read_burst_cont(
		_device, TCBD_STREAM_CFG1, (u8 *)&bytes_remain, 2);
	bytes_remain = SWAP16(bytes_remain) << 2;

	tcbd_debug(DEBUG_STREAM_READ, "%d bytes read, %d bytes remain\n",
		bytes_read, bytes_remain);
*/
	*_size = bytes_read;

	return tcbd_reg_read_burst_fix(
		_device, TCBD_STREAM_CFG4, _buff, bytes_read);
}
#elif defined(__READ_VARIABLE_LENGTH__)
s32 tcbd_read_stream(struct tcbd_device *_device, u8 *_buff, s32 *_size)
{
	u32 bytes_remain = 0;
	u32 bytes_read = _device->intr_threshold;

	if (bytes_read <= 0 || _buff == NULL)
		return -TCERR_INVALID_ARG;

	tcbd_reg_write(_device, TCBD_STREAM_CFG3, 0x22);
	tcbd_reg_read_burst_cont(
		_device, TCBD_STREAM_CFG1, (u8 *)&bytes_remain, 2);

	/* bytes_remain is word count. x4 needed */
	bytes_remain = SWAP16(bytes_remain) << 2;
	bytes_read = bytes_read + bytes_remain - _device->size_more_read;
	tcbd_debug(DEBUG_STREAM_READ, "%d bytes remain, real data size:%d\n",
		bytes_remain, bytes_read);

	if ((_device->intr_threshold << 1) < bytes_read) {
		tcbd_debug(DEBUG_ERROR, "Could not read data over "
					"TCBD_MAX_THRESHOLD(%d)\n",
					bytes_read);
		return -TCERR_FATAL_ERROR;
	}
	_device->size_more_read = bytes_remain;
	*_size = bytes_read;
	return tcbd_reg_read_burst_fix(
		_device, TCBD_STREAM_CFG4, _buff, bytes_read);
}
#else
#error "you must define __READ_VARIABLE_LENGTH__ or __READ_FIXED_LENGTH__"
#endif /*!__READ_FIXED_LENGTH__ && !__READ_VARIABLE_LENGTH__*/
#endif /*__CSPI_ONLY__*/

s32 tcbd_tune_frequency(
	struct tcbd_device *_device, u32 _freq_khz, s32 _bw_khz)
{
	s32 ret = 0;
	u64 tick;

	tcbd_debug(DEBUG_API_COMMON, "freq:%d, bw:%d\n", _freq_khz, _bw_khz);

	memset(&_device->mult_service, 0, sizeof(_device->mult_service));
	tick = tcpal_get_time();
	if (_freq_khz < 1000000)
		_device->curr_band = BAND_TYPE_BAND3;
	else
		_device->curr_band = BAND_TYPE_LBAND;
	ret |= tcbd_reset_ip(_device, TCBD_SYS_COMP_ALL, TCBD_SYS_COMP_ALL);

	ret |= tcbd_rf_tune_frequency(_device, _freq_khz, _bw_khz);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "failed to tune frequency to RF!! "
					"ret:%d\n", ret);
		return ret;
	}

	switch (_device->peri_type) {
	case PERI_TYPE_SPI_ONLY:
		ret |= tcbd_init_stream_data_config(_device, ENABLE_CMD_FIFO,
				STREAM_DATA_ENABLE | STREAM_HEADER_ON |
				STREAM_MASK_BUFFERA, TCBD_THRESHOLD_FIC);
		break;
	case PERI_TYPE_STS:
		ret |= tcbd_init_stream_data_config(_device, DISABLE_CMD_FIFO,
									0, 0);
		break;
	default:
		tcbd_debug(DEBUG_ERROR, "%d not implemented!!\n",
						_device->peri_type);
		return -TCERR_FATAL_ERROR;
	}
	tcbd_init_status_manager();

	ret |= tcbd_demod_tune_frequency(_device, _freq_khz, _bw_khz);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "failed to tune frequency "
			"to demodulator!! ret:%d\n", ret);
		return ret;
	}
	_device->prev_band = _device->curr_band;
	_device->frequency = _freq_khz;

#if defined(__READ_VARIABLE_LENGTH__)
	_device->size_more_read = 0;
#endif /*__READ_VARIABLE_LENGTH__*/
	tcbd_debug(DEBUG_API_COMMON, " # Frequency set time :%lld\n",
						tcpal_diff_time(tick));

	return ret;
}

s32 tcbd_wait_tune(struct tcbd_device *_device, u8 *_status)
{
	s32 ret = 0;
	u8 cto, cfo, ofdm;
	u64 time_tick, time_tune_wait;

	time_tune_wait = TDMB_OFDMDETECT_LOCK * TDMB_OFDMDETECT_RETRY;
	time_tick = tcpal_get_time();
	do {
		ret = tcbd_reg_read(_device, TCBD_PROGRAMID, _status);
		if (ret < 0)
			goto exit_wait_tune;

		cto = ((*_status) >> 1) & 0x01;
		cfo = ((*_status) >> 2) & 0x01;
		if (cto && cfo)
			goto exit_wait_tune;

		ofdm = ((*_status) >> 5) & 0x01;
		if (ofdm)
			break;
	} while (tcpal_diff_time(time_tick) < time_tune_wait);

	if (ofdm == 0) {
		ret = -TCERR_TUNE_FAILED;
		goto exit_wait_tune;
	}

	time_tune_wait = (TDMB_CTO_LOCK * TDMB_CTO_RETRY) +
				   (TDMB_CTO_LOCK + TDMB_CFO_LOCK) *
				   TDMB_CFO_RETRY;
	time_tick = tcpal_get_time();
	do {
		ret = tcbd_reg_read(_device, TCBD_PROGRAMID, _status);
		if (ret < 0)
			goto exit_wait_tune;

		cto = ((*_status) >> 1) & 0x01;
		cfo = ((*_status) >> 2) & 0x01;
		if (cto && cfo)
			break;
	} while (tcpal_diff_time(time_tick) < time_tune_wait);

	if (cto && cfo)
		tcbd_debug(DEBUG_API_COMMON, "lock status : 0x%02X\n",
								*_status);
	else
		ret = -TCERR_TUNE_FAILED;

exit_wait_tune:
	return ret;
}

static inline s32 tcbd_calc_threshold(struct tcbd_service *_service)
{
	s32 threshold = 0;
	s32 uep_bitrate[] = {
		32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384};

	switch (_service->ptype) {
	case PROTECTION_TYPE_UEP:
		threshold = (uep_bitrate[_service->bitrate] * 6) << 2;
		break;
	case PROTECTION_TYPE_EEP:
		threshold = (_service->bitrate * 6) << 5;
		break;
	}

	if (TCBD_MAX_THRESHOLD < threshold)
		threshold  = TCBD_MAX_THRESHOLD;

	tcbd_debug(DEBUG_API_COMMON, "ptype:%s, bitrate :%d, interrupt "
			"threshold:%d\n", (_service->ptype) ? "EEP" : "UEP",
			 _service->bitrate, threshold);

	return threshold>>1;
}

static inline s32 tcbd_find_empty_slot(
	struct tcbd_multi_service *_multi_svc)
{
	s32 i;

	for (i = 0; i < TCBD_MAX_NUM_SERVICE; i++) {
		if (_multi_svc->on_air[i] == 0)
			return i;
	}
	return -1;
}

static inline s32 tcbd_find_used_slot(
	struct tcbd_multi_service *_multi_svc, u8 _subch_id)
{
	s32 i;
	u32 *service_info = _multi_svc->service_info;

	if (_multi_svc->service_count == 0)
		return -1;

	for (i = 0; i < TCBD_MAX_NUM_SERVICE; i++) {
		if (((service_info[i * 2] >> 20) & 0x3F) == _subch_id)
			return i;
	}
	return -1;
}

#define FLAG_SHORT_PARAM 0
#define FLAG_LONG_PARAM 1
static inline s32 tcbd_set_service(struct tcbd_device *_device,
				struct tcbd_service *_service, s32 _flag)
{
	s32 ret = 0;
	u32 threshold = 0, sel_buff = 0, sel_stream = 0;
	u8 en_cmd_fifo = 0;

	sel_buff = STREAM_DATA_ENABLE | STREAM_HEADER_ON |
				STREAM_MASK_BUFFERA;
	switch (_device->peri_type) {
	case PERI_TYPE_SPI_ONLY:
		if (_flag == FLAG_LONG_PARAM) {
			threshold = tcbd_calc_threshold(_service);
			sel_stream = STREAM_SET_GARBAGE(threshold) |
				STREAM_TYPE_ALL;
		} else {
		threshold = TCBD_MAX_THRESHOLD;
		sel_stream = STREAM_SET_GARBAGE(TCBD_MAX_THRESHOLD) |
				STREAM_TYPE_ALL;
		}
		tcbd_debug(DEBUG_API_COMMON, "threshold : %d\n", threshold);
		en_cmd_fifo = ENABLE_CMD_FIFO;
		break;
	case PERI_TYPE_STS:
		en_cmd_fifo = DISABLE_CMD_FIFO;
		sel_stream = STREAM_TYPE_ALL;
#if !defined(__ALWAYS_FIC_ON__)
		sel_buff &= ~(STREAM_HEADER_ON);
#endif /*__ALWAYS_FIC_ON__*/
		break;
	default:
		tcbd_debug(DEBUG_ERROR, "not implemented!\n");
		return -1;
	}

#if !defined(__ALWAYS_FIC_ON__)
	sel_stream &= ~(STREAM_TYPE_FIC);
#endif /*__ALWAYS_FIC_ON__*/

#if defined(__STATUS_IN_REGISTER__)
	sel_stream &= ~(STREAM_TYPE_STATUS);
#endif /* __STATUS_IN_REGISTER__ */

#if defined(__READ_VARIABLE_LENGTH__)
	_device->size_more_read = 0;
#endif /*__READ_VARIABLE_LENGTH__*/
	ret |= tcbd_disable_irq(_device, 0);

	ret |= tcbd_change_stream_type(_device, sel_stream);
	ret |= tcbd_init_stream_data_config(
		_device, en_cmd_fifo, sel_buff, threshold);

	ret |= tcbd_send_service_info(_device);
	if (_device->peri_type == PERI_TYPE_SPI_ONLY)
		ret |= tcbd_enable_irq(_device, _device->enabled_irq);
	return ret;
}

s32 tcbd_register_service(struct tcbd_device *_device, u8 _subch_id,
							u8 _data_mode)
{
	s32 empty_slot, empty_slot2x;
	struct tcbd_service service = {0, };
	struct tcbd_multi_service *mult_service = &_device->mult_service;
	u32 *service_info = mult_service->service_info;

	if (tcbd_find_used_slot(mult_service, _subch_id) >= 0) {
		tcbd_debug(DEBUG_ERROR, "aready registerd service! "
			"subch_id:%d\n", _subch_id);
		return -TCERR_AREADY_REGISTERED;
	}

	empty_slot = tcbd_find_empty_slot(mult_service);
	if (empty_slot < 0) {
		tcbd_debug(DEBUG_ERROR, "Exceed maxinum number of service!!\n");
		return -TCERR_MAX_NUM_SERVICE;
	}
	service.subch_id = _subch_id;
	empty_slot2x = empty_slot << 1;
	mult_service->on_air[empty_slot] = 1;
	mult_service->service_count++;

	tcbd_debug(DEBUG_API_COMMON, "sub channel:%d, data mode:%d\n",
		_subch_id, _data_mode);
	service_info[empty_slot2x] =
		(_subch_id << 20) |
		(2 << 26);
	service_info[empty_slot2x + 1] =
		(empty_slot << 20) |
		(_data_mode << 16);

	return tcbd_set_service(_device, &service, FLAG_SHORT_PARAM);
}

s32 tcbd_register_service_long(struct tcbd_device *_device,
				struct tcbd_service *_service)
{
	s32 empty_slot, empty_slot2x;
	u32 data_mode = 0;
	struct tcbd_multi_service *mult_service = &_device->mult_service;
	u32 *service_info = mult_service->service_info;

	if (tcbd_find_used_slot(mult_service, _service->subch_id) >= 0) {
		tcbd_debug(DEBUG_ERROR, "aready registerd service! "
					"subch_id:%d\n", _service->subch_id);
		return -TCERR_AREADY_REGISTERED;
	}

	empty_slot = tcbd_find_empty_slot(mult_service);
	if (empty_slot < 0) {
		tcbd_debug(DEBUG_ERROR, "Exceed maxinum number of service!!\n");
		return -TCERR_MAX_NUM_SERVICE;
	}

	switch (_service->type) {
	case SERVICE_TYPE_DAB:
	case SERVICE_TYPE_DABPLUS:
	case SERVICE_TYPE_DATA:
		data_mode = 0; break;
	case SERVICE_TYPE_DMB:
		data_mode = 1; break;
	default:
		data_mode = 2; break;
	}

	empty_slot2x = empty_slot << 1;
	mult_service->on_air[empty_slot] = 1;
	mult_service->service_count++;

	service_info[empty_slot2x]  =
		(_service->reconfig << 26) |
		(_service->subch_id << 20) |
		(_service->size_cu << 10) |
		_service->start_cu;
	service_info[empty_slot2x + 1] =
		(empty_slot << 20) |
		/*(0 << 18) |*/
		(data_mode << 16) |
		(_service->ptype << 11) |
		(_service->plevel << 8) |
		_service->bitrate;

	return tcbd_set_service(_device, _service, FLAG_LONG_PARAM);
}

s32 tcbd_unregister_service(struct tcbd_device *_device, u8 _subch_id)
{
	s32 ret = 0;
	s32 service_idx;
	struct tcbd_multi_service *mult_service = &_device->mult_service;
	u32 *service_info = mult_service->service_info;

	tcbd_disable_irq(_device, 0);
	service_idx = tcbd_find_used_slot(mult_service, _subch_id);
	if (service_idx < 0) {
		tcbd_debug(DEBUG_ERROR, "not registered service!\n");
		return -TCERR_SERVICE_NOT_FOUND;
	}
	/*tcbd_disable_peri(_device);*/

	service_info[service_idx * 2] = 0x00;
	service_info[service_idx * 2 + 1] = 0x00;
	mult_service->on_air[service_idx] = 0;
	mult_service->service_count--;

	ret |= tcbd_send_service_info(_device);
	return ret;
}

s32 tcbd_read_fic_data(struct tcbd_device *_device, u8 *_buff,	s32 _size)
{
	s32 ret = 0;
	u32 addr_fic_buff;
	u8 status, err_status;

	tcbd_read_irq_status(_device, &status, &err_status);
	if (_size != TCBD_FIC_SIZE) {
		tcbd_debug(DEBUG_ERROR, "wrong fic size! %d\n", _size);
		ret = -TCERR_INVALID_ARG;
		goto exit_read_fic;
	}
	if (status & TCBD_IRQ_STAT_FIFOAINIT) {
		addr_fic_buff = PHY_MEM_ADDR_A_START;
		tcbd_debug(DEBUG_INTRRUPT, "status:0x%02X, err:0x%02X\n",
						status, err_status);

		ret = tcbd_mem_read(_device, addr_fic_buff, _buff, _size);
	} else
		ret = -TCERR_NO_FIC_DATA;
exit_read_fic:
	tcbd_clear_irq(_device, status);
	return ret;
	}

static s32 tcbd_disp_dsp_debug(struct tcbd_device *_device)
{
	s32 pos_buf = 0, ret, i;
	u32 len = 0, *data = NULL;
	u16 cmd;
	s8 debug_buff[256];

	tcbd_debug_mbox_rx(&cmd, &len, &data);
	if (data == NULL)
		return 0;

	ret = tcbd_read_mail_box(_device, cmd, len, data);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "failed to read mail box, "
					"err:%d\n", ret);
	return ret;
}

	for (i = 0; i < 6; i++)
		pos_buf += sprintf(debug_buff + pos_buf, "[%d:%08X]",
							i, data[i]);
	tcbd_debug(DEBUG_INFO, "%s\n", debug_buff);

	return 0;
}

s32 tcbd_read_signal_info(
	struct tcbd_device *_device,
	struct tcbd_status_data *_status_data)
{
	s32 ret = 0;
#if defined(__STATUS_IN_REGISTER__)
	u8 status[32] = {0, };
#endif /*__STATUS_IN_REGISTER__*/

	if (!_status_data)
		return -TCERR_INVALID_ARG;

	if (tcbd_debug_spur_dbg() == 1)
		tcbd_disp_dsp_debug(_device);

#if defined(__STATUS_IN_REGISTER__)
	ret = tcbd_reg_write(_device, TCBD_OP_STATUS0, 0x1);
	ret |= tcbd_reg_read_burst_fix(_device,
		TCBD_OP_STATUS1, status, TCBD_STATUS_SIZE);
	tcbd_update_status(status, TCBD_STATUS_SIZE, _status_data);
#elif defined(__STATUS_IN_STREAM__)

	tcbd_update_status(NULL, 0, _status_data);
#endif /*__STATUS_IN_STREAM__*/
	return ret;
}
