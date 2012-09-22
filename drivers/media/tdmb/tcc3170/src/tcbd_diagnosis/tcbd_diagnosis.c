/*
 * tcbd_diagnosis.c
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
#include "tcbd_diagnosis.h"
#include "tcbd_error.h"

#define OFFSET_RF_LOOP_GAIN   4
#define OFFSET_BB_LOOP_GAIN   5
#define OFFSET_PRS_SNR		8
#define OFFSET_PCBER		 12
#define OFFSET_RS_PKT_CNT	16
#define OFFSET_RS_OVER_CNT   20
#define OFFSET_RS_ERR_CNT_LO 24
#define OFFSET_RS_ERR_CNT_HI 28

#define MAX_MAVG_ARRAY_SIZE 10

#define MIN_SNR  0
#define MAX_SNR  25

#define MIN_PCBER  0
#define MAX_PCBER  1

#define MIN_VITERBIBER  0
#define MAX_VITERBIBER  1

#define MIN_TSPER  0
#define MAX_TSPER  1

#define MIN_RSSI  (-105)
#define MAX_RSSI  3

#define SCALE_FACTOR 10000
#define ERROR_LIMIT   2000

struct tcbd_moving_avg {
	u32 array[MAX_MAVG_ARRAY_SIZE+1];
	u32 pre_sum;
	u64 total_sum;
	s32 index;
	s32 moving_cnt;
	s32 total_cnt;
};

struct tcbd_raw_status {
	u8 lock_status;
	u8 rf_loop_gain;
	u8 bb_loop_gain;

	s32 pcber;
	u32 prs_snr;

	u32 rs_over_cnt;
	u32 rs_pkt_cnt;
	u64 rs_err_cnt;
};

struct tcbd_raw_status_manager {
	u32 resynced;
	struct tcbd_raw_status old_status;
	struct tcbd_raw_status curr_status;

	struct tcbd_status_data status_data;
};

static struct tcbd_moving_avg moving_average[4];
static struct tcbd_raw_status_manager status_manager;

static inline u32 tcbd_log10(u32 _val)
{
	u32 i, snr = 0;
	u32 snr_table[28] = {
		1024, 1289, 1622,  2043, 2572, 3238, 4076, 5132,
		6461, 8133, 10240, 12891, 6229,  20431, 25721, 32381,
		40766, 51321, 64610, 81339, 102400, 128913, 162293, 204314,
		257217, 323817, 407661, 513215 };

	if (_val < snr_table[0])
		return  0;

	if (_val >= snr_table[27])
		return 27;

	for (i = 0; i < 27; i++) {
		if (_val >= snr_table[i] &&
			_val < snr_table[i + 1]) {
			snr = i;
			break;
		}
	}

	return snr;
}

static s32 tcbd_get_moving_avg(
	struct tcbd_moving_avg *_slot, u32 _value, s32 _windowSize)
{
	s32 moving_sum;
	u32 result;

	if (MAX_MAVG_ARRAY_SIZE < _windowSize) {
		tcbd_debug(0,
			"max window size is %d\n", MAX_MAVG_ARRAY_SIZE);
		return -TCERR_INVALID_ARG;
	}

	if (_slot->moving_cnt >= _windowSize)
		_slot->pre_sum += _slot->array[_slot->index];
	else
		_slot->moving_cnt++;

	_slot->array[_slot->index] = _value;
	_slot->index++;

	_slot->index %= _windowSize;
	_slot->total_cnt++;
	_slot->total_sum += _value;

	moving_sum = _slot->total_sum - _slot->pre_sum;
	result =  moving_sum / _slot->moving_cnt;

	if (_slot->index == 0) {
		_slot->total_sum = moving_sum;
		_slot->pre_sum = 0;
	}

	return result;
}


static inline void tcbd_calc_rssi(struct tcbd_status_data *_status_data)
{
	struct tcbd_raw_status *curr_status = &status_manager.curr_status;

	if (curr_status->rf_loop_gain <= 120)
		_status_data->rssi = 1500 -
			((int)curr_status->bb_loop_gain) * 37 -
			27 * ((int)curr_status->rf_loop_gain);
	else if (curr_status->rf_loop_gain <= 216)
		_status_data->rssi = 1500 -
			((int)curr_status->bb_loop_gain) * 35 -
			24 * ((int)curr_status->rf_loop_gain);
	else
		_status_data->rssi = 1500 -
			((int)curr_status->bb_loop_gain) * 33 -
			22 * ((int)curr_status->rf_loop_gain);

	_status_data->rssi /= 100;
	if (_status_data->rssi < MIN_RSSI)
		_status_data->rssi = MIN_RSSI;
	else if (_status_data->rssi > MAX_RSSI)
		_status_data->rssi = MAX_RSSI;
}

static inline void tcbd_calc_pcber(struct tcbd_status_data *_status_data)
{
	long long over;
	struct tcbd_raw_status *curr_status = &status_manager.curr_status;

	over = (u64)curr_status->pcber * SCALE_FACTOR;
	_status_data->pcber = (u32)(over >> 16);

	if (_status_data->pcber < MIN_PCBER)
		_status_data->pcber = MIN_PCBER;
	else if (_status_data->pcber > MAX_PCBER * ERROR_LIMIT)
		_status_data->pcber = MAX_PCBER * ERROR_LIMIT;

	if (_status_data->pcber <= 20)
		_status_data->pcber = 0;

	_status_data->pcber_moving_avg =
		tcbd_get_moving_avg(
			&moving_average[0],
			_status_data->pcber,
			MAX_MAVG_ARRAY_SIZE);
}

static inline void tcbd_calc_snr(struct tcbd_status_data *_status_data)
{
	struct tcbd_raw_status *curr_status = &status_manager.curr_status;

	if ((int)curr_status->prs_snr < 0)
		_status_data->snr = MAX_SNR;
	else if (curr_status->prs_snr == 0)
		_status_data->snr = MIN_SNR;
	else
	   _status_data->snr = tcbd_log10(curr_status->prs_snr);

	if (_status_data->snr < MIN_SNR)
		_status_data->snr = MIN_SNR;
	else if (_status_data->snr > MAX_SNR)
		_status_data->snr = MAX_SNR;

	_status_data->snr_moving_avg =
		tcbd_get_moving_avg(
			&moving_average[1],
			_status_data->snr,
			MAX_MAVG_ARRAY_SIZE);
}

static inline void tcbd_calc_viterbi_ber(struct tcbd_status_data *_status_data)
{
	s32 rs_err, rs_over, rs_under;
	struct tcbd_raw_status *old_status, *curr_status;

	old_status = &status_manager.old_status;
	curr_status = &status_manager.curr_status;

	if (status_manager.resynced) {
		_status_data->vber = MAX_VITERBIBER * ERROR_LIMIT;
		goto exit_calc_viterbi_ber;
	}

	if (status_manager.resynced && curr_status->rs_pkt_cnt == 0) {
		_status_data->vber = MIN_VITERBIBER;
		goto exit_calc_viterbi_ber;
	}

	rs_err = ((u32)(curr_status->rs_over_cnt -
		old_status->rs_over_cnt)) * SCALE_FACTOR;

	rs_over = (rs_err * (8 * 8) +
		((u32)(curr_status->rs_err_cnt -
			old_status->rs_err_cnt)) * SCALE_FACTOR);
	rs_under =
		(curr_status->rs_pkt_cnt -
			old_status->rs_pkt_cnt) * 204 * 8;
	if (!rs_under)
		return;

	_status_data->vber = rs_over / rs_under;

	if (_status_data->vber < MIN_VITERBIBER)
		_status_data->vber = MIN_VITERBIBER;
	else if (_status_data->vber > MAX_VITERBIBER * ERROR_LIMIT)
		_status_data->vber = MAX_VITERBIBER * ERROR_LIMIT;

exit_calc_viterbi_ber:
	_status_data->vber_moving_avg =
		tcbd_get_moving_avg(
			&moving_average[3],
			_status_data->vber,
			MAX_MAVG_ARRAY_SIZE);
}

static inline void tcbd_calc_tsper(struct tcbd_status_data *_status_data)
{
	s32 rs_over, rs_under;
	struct tcbd_raw_status *old_status, *curr_status;

	old_status = &status_manager.old_status;
	curr_status = &status_manager.curr_status;

	if (status_manager.resynced) {
		_status_data->tsper = MAX_TSPER * ERROR_LIMIT;
		goto exit_calc_tsper;
	}

	if (status_manager.resynced && curr_status->rs_pkt_cnt == 0) {
		_status_data->tsper = MIN_TSPER;
		goto exit_calc_tsper;
	}

	rs_over = (curr_status->rs_over_cnt -
			old_status->rs_over_cnt) * SCALE_FACTOR;
	rs_under = curr_status->rs_pkt_cnt - old_status->rs_pkt_cnt;
	if (!rs_under)
		return;

	_status_data->ts_error_count = rs_over;
	_status_data->tsper = rs_over / rs_under;
	if (_status_data->tsper < MIN_TSPER)
		_status_data->tsper = MIN_TSPER;
	else if (_status_data->tsper > MAX_TSPER * ERROR_LIMIT)
		_status_data->tsper = MAX_TSPER * ERROR_LIMIT;

exit_calc_tsper:
	_status_data->tsper_moving_avg = tcbd_get_moving_avg(
		&moving_average[2],
		_status_data->tsper,
		MAX_MAVG_ARRAY_SIZE);
}

void tcbd_update_status_per_frame(
	u8 *_raw_data, struct tcbd_status_data *_status_data)
{
	struct tcbd_raw_status *old_status, *curr_status;
	struct tcbd_status_data *status_data;

	if (!_raw_data && !_status_data)
		return;

	if (_raw_data == NULL) {
		memcpy(_status_data, &status_manager.status_data,
			sizeof(struct tcbd_status_data));
		return;
	}

	status_data = (_status_data) ?
		_status_data : &status_manager.status_data;
	old_status  = &status_manager.old_status;
	curr_status = &status_manager.curr_status;
	memcpy(old_status, curr_status, sizeof(struct tcbd_raw_status));

	curr_status->rf_loop_gain = _raw_data[OFFSET_RF_LOOP_GAIN];
	curr_status->bb_loop_gain = _raw_data[OFFSET_BB_LOOP_GAIN];

	curr_status->prs_snr =
		*((u32 *)(_raw_data + OFFSET_PRS_SNR));
	curr_status->pcber =
		*((u32 *)(_raw_data + OFFSET_PCBER));

	curr_status->rs_pkt_cnt  =
		*((u32 *)(_raw_data + OFFSET_RS_PKT_CNT));
	curr_status->rs_over_cnt =
		*((u32 *)(_raw_data + OFFSET_RS_OVER_CNT));
	curr_status->rs_err_cnt  =
		*((u64 *)(_raw_data + OFFSET_RS_ERR_CNT_LO));
	curr_status->rs_err_cnt |=
		(*((u64 *)(_raw_data + OFFSET_RS_ERR_CNT_HI))) << 32;

	if (curr_status->rs_pkt_cnt != old_status->rs_pkt_cnt) {
		status_manager.resynced = 0;
		if (curr_status->rs_pkt_cnt == 0 && old_status->rs_pkt_cnt)
			status_manager.resynced = 1;
		else if (curr_status->rs_pkt_cnt < old_status->rs_pkt_cnt) {
			if (old_status->rs_pkt_cnt < 0x80000000)
				status_manager.resynced = 1;
		}
	}

	tcbd_calc_snr(status_data);
	tcbd_calc_pcber(status_data);
	tcbd_calc_viterbi_ber(status_data);
	tcbd_calc_tsper(status_data);
	tcbd_calc_rssi(status_data);

	status_data->lock = *((u8 *)_raw_data);
	if (status_manager.resynced) {
		memset(moving_average, 0, sizeof(moving_average));
		memset(&status_manager.old_status, 0,
				sizeof(struct tcbd_raw_status));
	}
}

void tcbd_update_status(
	u8 *_raw_data, u32 _size, struct tcbd_status_data *_status_data)
{
	s32 i;

	if (!_raw_data)
		tcbd_update_status_per_frame(NULL, _status_data);

	for (i = 0; i < _size / TCBD_STATUS_SIZE; i++)
		tcbd_update_status_per_frame(_raw_data + i * TCBD_STATUS_SIZE,
			_status_data);
}

void tcbd_init_status_manager(void)
{
	memset(&status_manager, 0, sizeof(status_manager));
	memset(moving_average, 0, sizeof(moving_average));
}
