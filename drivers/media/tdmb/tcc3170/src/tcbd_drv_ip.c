/*
 * tcbd_drv_ip.c
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
#include "tcbd_feature.h"
#include "tcpal_debug.h"

#include "tcbd_api_common.h"
#include "tcbd_drv_ip.h"
#include "tcbd_drv_io.h"
#include "tcbd_drv_rf.h"

struct tcbd_spur_data {
	s32 freq_khz;
	s32 num_data;
	s32 data[7];
};

struct tcbd_agc_data {
	s32 cmd;
	s32 num_data;
	u32 data[7];
};

#define MAX_BOOT_TABLE 5
struct tcbd_boot_bin {
	u32 size;
	u32 crc;
	u32 addr;
	u8 *data;
};

static u32 clock_config_table[3][5] = {
	{ 0x60, 0x00, 0x0F, 0x03, 19200},
	{ 0x60, 0x00, 0x0C, 0x03, 24576},
	{ 0x60, 0x01, 0x0F, 0x03, 38400}
};

static struct tcbd_spur_data spur_data_table_default[] = {
	{ 181936, 2, {(0x035C<<16)|(0x0342), (0x0359<<16)|(0x033e)} },
	{ 183008, 2, {(0x00CA<<16)|(0x036B), (0x00CE<<16)|(0x0368)} },
	{ 192352, 2, {(0x0081<<16)|(0x0328), (0x0084<<16)|(0x0324)} },
	{ 201008, 2, {(0x033A<<16)|(0x0366), (0x0336<<16)|(0x0363)} },
	{ 201072, 2, {(0x034B<<16)|(0x0352), (0x0347<<16)|(0x034F)} },
	{ 211280, 2, {(0x001E<<16)|(0x0307), (0x001F<<16)|(0x0302)} },
	{ 211648, 2, {(0x009F<<16)|(0x033E), (0x00A2<<16)|(0x033A)} },
	{ 220352, 2, {(0x0361<<16)|(0x033E), (0x035E<<16)|(0x033A)} },
	{ 230784, 2, {(0x008B<<16)|(0x032F), (0x008E<<16)|(0x032B)} },
	{1459808, 2, {(0x00CA<<16)|(0x036B), (0x00CE<<16)|(0x0368)} },
	{1468368, 2, {(0x0366<<16)|(0x033A), (0x0363<<16)|(0x0336)} },
	{1478640, 2, {(0x005A<<16)|(0x0316), (0x005C<<16)|(0x0311)} }
};

static struct tcbd_spur_data spur_data_table_24576[] = {
	{ 184736, 2, {(0x0092<<16)|(0x033A), (0x0098<<16)|(0x0332)} },
	{ 196736, 2, {(0x0030<<16)|(0x030F), (0x0032<<16)|(0x0305)} },
	{ 199280, 2, {(0x0373<<16)|(0x0337), (0x036D<<16)|(0x032F)} },
	{ 205280, 2, {(0x00A5<<16)|(0x0349), (0x00AC<<16)|(0x0342)} },
	{ 213008, 2, {(0x0006<<16)|(0x030A), (0x0006<<16)|(0x0300)} },
};


#if defined(__AGC_TABLE_IN_DSP__)
static struct tcbd_agc_data agc_data_table_lband[] = {
	{MBCMD_AGC_DAB_JAM, 3,
		{0x0, (195 << 16) + (23 << 8) + 24, 0x001C0012} },
};

static struct tcbd_agc_data agc_table_data_vhf[] = {
	{MBCMD_AGC_DAB_JAM, 3,
		{0x0, (195 << 16) + (16 << 8) + 17, 0x005a0043} },
};
#else /*__AGC_TABLE_IN_DSP__*/
static struct tcbd_agc_data agc_data_table_lband[] = {
	{ MBCMD_AGC_DAB_CFG, 3, {0xC0ACFF44, 0x031EFF53, 0x011EFF07} },
	{ MBCMD_AGC_DAB_JAM, 3,
		{0x00091223, (195 << 16) + (23 << 8) + 24, 0x001C0012} },
	{ MBCMD_AGC_DAB_3,   7,
		{0x52120223, 0x58120823, 0x5c121223,
		 0x60121c23, 0x62122023, 0x68122423, 0x6a122a23} },
	{ MBCMD_AGC_DAB_4,   7,
		{0x70123223, 0x30093a23, 0x32094023, 0x3a094823,
		 0x3c095223, 0x40095823, 0x42095c23} },
	{ MBCMD_AGC_DAB_5,   7,
		{0x48096023, 0x4a096223, 0x52096423,
		 0x4e096823, 0x58096a23, 0x5a097023, 0x5e090812} },
	{ MBCMD_AGC_DAB_6,   7,
		{0x62091212, 0x68090e12, 0x70091c12,
		 0x72092012, 0x78092412, 0x7a093012, 0x7c093812} },
	{ MBCMD_AGC_DAB_7,   4,
		{0x7e093a12, 0x6f094012, 0x73094212, 0x7f094a12} },
	{ MBCMD_AGC_DAB_8,   5,
		{0x30093a23, 0x32094023, 0x3a094823, 0x3c095223, 0x40095823} },
	{ MBCMD_AGC_DAB_9,   5,
		{0x72123a23, 0x78124023, 0x7a124823, 0x7e125223, 0x73125823} },
	{ MBCMD_AGC_DAB_A,   5,
		{0x5e090812, 0x62091212, 0x68090e12, 0x70091c12, 0x72092012} },
	{ MBCMD_AGC_DAB_B,   5,
		{0x5e097223, 0x62097823, 0x68097a23, 0x70097e23, 0x72097323} },
	{ MBCMD_AGC_DAB_C,   5, {0x0, 0x0, 0x0, 0x0, 0x0} },
	{ MBCMD_AGC_DAB_D,   5, {0x0, 0x0, 0x0, 0x0, 0x0} },
	{ MBCMD_AGC_DAB_E,   5, {0x0, 0x0, 0x0, 0x0, 0x0} },
	{ MBCMD_AGC_DAB_F,   5, {0x0, 0x0, 0x0, 0x0, 0x0} },
};

static struct tcbd_agc_data agc_table_data_vhf[] = {
	{ MBCMD_AGC_DAB_CFG, 3,
		{0xC0ACFF44, 0x031EFF53, 0x011EFF07} },
	{ MBCMD_AGC_DAB_JAM, 3,
		{0x16112243, (195 << 16) + (16 << 8) + 17, 0x005a0043} },
	{ MBCMD_AGC_DAB_3,   7,
		{0x5a220243, 0x5e220843, 0x60221043,
		 0x62221443, 0x64221c43, 0x68222043, 0x6c222243} },
	{ MBCMD_AGC_DAB_4,   7,
		{0x70222843, 0x2a112c43, 0x30113443, 0x34113a43,
		 0x38113e43, 0x3c114243, 0x3e114643} },
	{ MBCMD_AGC_DAB_5,   7,
		{0x40114c43, 0x44115443, 0x48115a43, 0x4a115e43,
		 0x50116043, 0x54116443, 0x5a112422} },
	{ MBCMD_AGC_DAB_6,   7,
		{0x5e112822, 0x60113222, 0x62113622, 0x66113a22,
		 0x6a113e22, 0x6e114222, 0x72114622} },
	{ MBCMD_AGC_DAB_7,   4,
		{0x76114822, 0x7a115022, 0x7e115422, 0x7f115622} },
	{ MBCMD_AGC_DAB_8,   5,
		{0x2a112c43, 0x30113443, 0x34113a43, 0x38113e43, 0x3c114243} },
	{ MBCMD_AGC_DAB_9,   5,
		{0x74222c43, 0x78223443, 0x7a223a43, 0x7e223e43, 0x7f224243} },
	{ MBCMD_AGC_DAB_A,   5,
		{0x5a112422, 0x5e112822, 0x60113222, 0x62113622, 0x66113a22} },
	{ MBCMD_AGC_DAB_B,   5,
		{0x5a116843, 0x5e116c43, 0x60117243, 0x62117643, 0x66117a43} },
	{ MBCMD_AGC_DAB_C,   5, {0x0, 0x0, 0x0, 0x0, 0x0} },
	{ MBCMD_AGC_DAB_D,   5, {0x0, 0x0, 0x0, 0x0, 0x0} },
	{ MBCMD_AGC_DAB_E,   5, {0x0, 0x0, 0x0, 0x0, 0x0} },
	{ MBCMD_AGC_DAB_F,   5, {0x0, 0x0, 0x0, 0x0, 0x0} },
};
#endif /*!__AGC_TABLE_IN_DSP__*/

s32 tcbd_send_spur_data(struct tcbd_device *_device, s32 _freq_khz)
{
	s32 ret = 0, i;
	s32 size_table;
	struct tcbd_mail_data mail = {0, };
	struct tcbd_spur_data *spur_table;

	switch (_device->clock_type) {
	case CLOCK_24576KHZ:
		spur_table = spur_data_table_24576;
		size_table = ARRAY_SIZE(spur_data_table_24576);
		break;
	case CLOCK_38400KHZ:
	case CLOCK_19200KHZ:
	default:
		spur_table = spur_data_table_default;
		size_table = ARRAY_SIZE(spur_data_table_default);
		break;
	}
	mail.flag = MB_CMD_WRITE;
	mail.cmd = MBCMD_FP_DAB_IIR;
	for (i = 0; i < size_table; i++) {
		if (spur_table[i].freq_khz == _freq_khz) {
			mail.count = spur_table[i].num_data;
			memcpy(mail.data, spur_table[i].data,
				mail.count * sizeof(u32));
			tcbd_debug(DEBUG_DRV_COMP,
				"freq:%d, num mail data:%d\n",
					_freq_khz, mail.count);
			ret = tcbd_send_mail(_device, &mail);
			break;
		}
	}

	return ret;
}

s32 tcbd_send_agc_data(
	struct tcbd_device *_device, enum tcbd_band_type _band_type)
{
	s32 ret = 0, i;
	s32 size_table;
	struct tcbd_agc_data *agc_table;
	struct tcbd_mail_data mail = {0, };

	switch (_band_type) {
	case BAND_TYPE_LBAND:
		size_table = ARRAY_SIZE(agc_data_table_lband);
		agc_table = agc_data_table_lband;
		break;
	case BAND_TYPE_BAND3:
		size_table = ARRAY_SIZE(agc_table_data_vhf);
		agc_table = agc_table_data_vhf;
		break;
	default:
		tcbd_debug(DEBUG_ERROR,
			"Unknown band type:%d\n", _band_type);
		return -1;
	}
	tcbd_debug(DEBUG_DRV_COMP, "agc table size:%d, band:%s\n",
		size_table,
		(_band_type == BAND_TYPE_LBAND) ? "Lband" : "Band3");

	mail.flag = MB_CMD_WRITE;
	for (i = 0; i < size_table; i++) {
		mail.cmd = agc_table[i].cmd;
		mail.count = agc_table[i].num_data;
		memcpy(mail.data, agc_table[i].data, mail.count * sizeof(u32));
		ret = tcbd_send_mail(_device, &mail);
	}

	return ret;
}

s32 tcbd_send_frequency(struct tcbd_device *_device, s32 _freq_khz)
{
	s32 ret = 0;
	struct tcbd_mail_data mail;

	tcbd_debug(DEBUG_DRV_COMP, "freq:%d\n", _freq_khz);

	mail.flag = MB_CMD_WRITE;
	mail.cmd = MBPARA_SYS_NUM_FREQ;
	mail.count = 1;
	mail.data[0] = 1;
	ret |= tcbd_send_mail(_device, &mail);

	mail.cmd = MBPARA_SYS_FREQ_0_6;
	mail.data[0] = _freq_khz;
	ret |= tcbd_send_mail(_device, &mail);

	return ret;
}

static inline void tcbd_sort_start_cu(
	struct tcbd_multi_service *_multi_svc, u32 *_svc_info)
{
	s32 i, j;
	s32 start_cu[TCBD_MAX_NUM_SERVICE];
	u32 tmp_cu, tmp_info[2];

	memcpy(_svc_info, _multi_svc->service_info,
		sizeof(u32) * TCBD_MAX_NUM_SERVICE << 1);
	for (i = 0; i < TCBD_MAX_NUM_SERVICE; i++) {
		if (_svc_info[i << 1] != 0)
			start_cu[i] = _svc_info[i << 1] & 0x3FF;
		else
			start_cu[i] = 0x3FF;
	}

	for (i = 0; i < TCBD_MAX_NUM_SERVICE; i++) {
		for (j = i + 1; j < TCBD_MAX_NUM_SERVICE; j++) {
			if (start_cu[i] > start_cu[j]) {
				tmp_cu = start_cu[i];
				tmp_info[0] = _svc_info[i * 2];
				tmp_info[1] = _svc_info[i * 2 + 1];

				start_cu[i] = start_cu[j];
				_svc_info[i * 2] = _svc_info[j * 2];
				_svc_info[i * 2 + 1] = _svc_info[j * 2 + 1];

				start_cu[j] = tmp_cu;
				_svc_info[j * 2] = tmp_info[0];
				_svc_info[j * 2 + 1] = tmp_info[1];
			}
		}
	}
	for (i = 0; i < TCBD_MAX_NUM_SERVICE * 2; i++)
		tcbd_debug(DEBUG_DRV_COMP,
			"%02d: 0x%08X\n", i, _svc_info[i]);
}

s32 tcbd_send_service_info(struct tcbd_device *_device)
{
	s32 ret = 0;
	struct tcbd_mail_data mail = {0, } ;
	struct tcbd_multi_service *mult_service = &_device->mult_service;
	u32 sorted_svc_info[TCBD_MAX_NUM_SERVICE * 2];
	u8 num_svc = TCBD_MAX_NUM_SERVICE;

	tcbd_sort_start_cu(mult_service, sorted_svc_info);

	mail.cmd = MBPARA_SEL_CH_INFO_PRAM;
	mail.count = num_svc;
	memcpy(mail.data, sorted_svc_info,
		num_svc * sizeof(u32));
	ret = tcbd_send_mail(_device, &mail);

	mail.cmd = MBPARA_SEL_CH_INFO_PRAM + 1;
	mail.count = num_svc;
	memcpy(mail.data, sorted_svc_info + num_svc,
		num_svc * sizeof(u32));
	ret = tcbd_send_mail(_device, &mail);

	mail.cmd = MBPARA_SYS_DAB_MCI_UPDATE;
	mail.count = 1;
	mail.data[0] = mult_service->service_count;
	ret |= tcbd_send_mail(_device, &mail);
	tcbd_debug(DEBUG_DRV_COMP, "service count:%d\n",
		mult_service->service_count);
	return ret;
}

s32 tcbd_disable_buffer(struct tcbd_device *_device)
{
	s32 ret = 0;
	ret |= tcbd_reg_write(_device, TCBD_OBUFF_CONFIG, 0);
	return ret;
}

s32 tcbd_enable_buffer(struct tcbd_device *_device)
{
	s32 i, ret = 0;
	u8 buff_en = 0, buff_init = 0;

	u32 list_size_buf[] = {
		TCBD_BUFFER_A_SIZE, TCBD_BUFFER_B_SIZE,
		TCBD_BUFFER_C_SIZE, TCBD_BUFFER_D_SIZE };
	u8 list_buff_init[] = {
		TCBD_OBUFF_BUFF_A_INIT, TCBD_OBUFF_BUFF_B_INIT,
		TCBD_OBUFF_BUFF_C_INIT, TCBD_OBUFF_BUFF_D_INIT };
	u32 list_buff_en[] = {
		TCBD_OBUFF_CONFIG_BUFF_A_EN | TCBD_OBUFF_CONFIG_BUFF_A_CIRCULAR,
		TCBD_OBUFF_CONFIG_BUFF_B_EN | TCBD_OBUFF_CONFIG_BUFF_B_CIRCULAR,
		TCBD_OBUFF_CONFIG_BUFF_C_EN | TCBD_OBUFF_CONFIG_BUFF_C_CIRCULAR,
		TCBD_OBUFF_CONFIG_BUFF_D_EN | TCBD_OBUFF_CONFIG_BUFF_D_CIRCULAR,
	};

	for (i = 0; i < ARRAY_SIZE(list_size_buf); i++) {
		if (list_size_buf[i]) {
			buff_init |= list_buff_init[i];
			buff_en |= list_buff_en[i];
		}
	}
	tcbd_debug(DEBUG_DRV_COMP, "buffer init : 0x%02X, buffer en:0x%02X\n",
		buff_init, buff_en);
	ret |= tcbd_reg_write(_device, TCBD_OBUFF_INIT, buff_init);
	ret |= tcbd_reg_write(_device, TCBD_OBUFF_CONFIG, buff_en);
	return ret;
}

s32 tcbd_init_buffer_region(struct tcbd_device *_device)
{
	s32 i, ret = 0;
	u32 addr_start, addr_end;
#if defined(__DEBUG_TCBD__)
	u16 reg_val[2];
#endif  /*__DEBUG_TCBD__*/
	u32 list_size_buf[] = {
		TCBD_BUFFER_A_SIZE, TCBD_BUFFER_B_SIZE,
		TCBD_BUFFER_C_SIZE, TCBD_BUFFER_D_SIZE };
	u8 reg_addr_buff[] = {
		TCBD_OBUFF_A_SADDR, TCBD_OBUFF_B_SADDR,
		TCBD_OBUFF_C_SADDR, TCBD_OBUFF_D_SADDR };
	u32 addr_set_buff[] = {
		(PHY_MEM_ADDR_A_START>>2), (PHY_MEM_ADDR_A_END>>2),
		(PHY_MEM_ADDR_B_START>>2), (PHY_MEM_ADDR_B_END>>2),
		(PHY_MEM_ADDR_C_START>>2), (PHY_MEM_ADDR_C_END>>2),
		(PHY_MEM_ADDR_D_START>>2), (PHY_MEM_ADDR_D_END>>2),
	};

	for (i = 0; i < ARRAY_SIZE(list_size_buf); i++) {
		if (!list_size_buf[i])
			continue;

		addr_start = SWAP16(addr_set_buff[i * 2 + 0]);
		addr_end = SWAP16(addr_set_buff[i * 2 + 1]);
		tcbd_debug(DEBUG_DRV_COMP, "reg:%02x, %c buffer size: %d\n",
			reg_addr_buff[i], 'A' + i,
			(addr_set_buff[i * 2 + 1] << 2) -
				(addr_set_buff[i * 2 + 0] << 2) + 4);
		ret = tcbd_reg_write_burst_cont(
			_device, reg_addr_buff[i], (u8 *)&addr_start, 2);
		ret |= tcbd_reg_write_burst_cont(
			_device, reg_addr_buff[i] + 2, (u8 *)&addr_end, 2);

		if (ret < 0)
			return ret;
	}
#if defined(__DEBUG_TCBD__)
	for (i = 0; i < 4 && list_size_buf[i]; i++) {
		tcbd_reg_read_burst_cont(
			_device, reg_addr_buff[i], (u8 *)&reg_val[0], 2);
		tcbd_reg_read_burst_cont(
			_device, reg_addr_buff[i] + 2, (u8 *)&reg_val[1], 2);
		tcbd_debug(DEBUG_DRV_COMP, "%c buffer start:%X, end:%X\n",
				'A' + i,
				SWAP16(reg_val[0]) << 2,
				SWAP16(reg_val[1]) << 2);
	}
#endif /*__DEBUG_TCBD__*/
	/*tcbd_enable_buffer(_device);*/
	return ret;
}

s32 tcbd_change_memory_view(
	struct tcbd_device *_device,
	enum tcbd_remap_type _remap)
{
	return tcbd_reg_write(_device, TCBD_INIT_REMAP, (u8)_remap);
}

s32 tcbd_init_bias_key(struct tcbd_device *_device)
{
	s32 ret = 0;

	ret |= tcbd_reg_write(_device, TCBD_OP_XTAL_BIAS, 0x05);
	ret = tcbd_reg_write(_device, TCBD_OP_XTAL_BIAS_KEY, 0x5E);

	return ret;
}

u32 tcbd_get_osc_clock(struct tcbd_device *_device)
{
	return clock_config_table[(int)_device->clock_type][4];
}

s32 tcbd_init_pll(
	struct tcbd_device *_device, enum tcbd_clock_type _ctype)
{
   /*
	* _pll_data[0] = PLL_WAIT_TIME
	* _pll_data[1] = PLL_P
	* _pll_data[2] = PLL_M
	* _pll_data[3] = PLL_S
	* _pll_data[4] = OSC_LOCK
	* */
	s32 ret = 0;
	u32 out, vco;
	u8 pll_data[2];
	u8 pll_r, pll_f, pll_od, pll_m;
	u32 *clock_config = NULL;

	if (!_device || _ctype >= CLOCK_MAX)
		return -TCERR_INVALID_ARG;

	_device->clock_type = _ctype;

	clock_config = clock_config_table[(s32)_ctype];
	tcbd_debug(DEBUG_DRV_COMP, "osc clock:%d, P:%X, M:%X, S:%d\n",
		clock_config[1], clock_config[2],
		clock_config[3], clock_config[4]);
	pll_r = clock_config[1] + 1;
	pll_f = clock_config[2] + 1;
	pll_od = clock_config[3] & 0x3;
	pll_m = (clock_config[3] & 0x4) >> 2;

	vco = clock_config[4] * (pll_f / pll_r);

	if (pll_od)
		out = vco >> pll_od;
	else
		out = vco;

	if (pll_m)
		out = out>>pll_m;

	_device->main_clock = out;
	pll_data[0] = clock_config[2] | (pll_m << 6) | 0x80;
	pll_data[1] = (clock_config[1] << 3) | (pll_od << 1);

	ret = tcbd_reg_write(_device, TCBD_PLL_7, pll_data[1]);
	ret = tcbd_reg_write(_device, TCBD_PLL_6, pll_data[0]);
	tcpal_msleep(1);
	tcbd_debug(DEBUG_DRV_COMP, "clock out:%d, pll[0]=%02X, pll[1]=%02X\n",
		out, pll_data[0], pll_data[1]);

	ret |= tcbd_init_bias_key(_device);

	return ret;
}


static s32 tcbd_check_dsp(struct tcbd_device *_device)
{
	s32 ret = 0;
	u32 status;

	ret = tcbd_reg_read_burst_fix(
		_device, TCBD_MAIL_FIFO_WIND, (u8 *)&status, 4);

	if (status != 0x1ACCE551) {
		tcbd_debug(DEBUG_ERROR,
			" # Error access mail [0x%X]\n", status);
		return -TCERR_CANNOT_ACCESS_MAIL;
	}

	return ret;
}

s32 tcbd_reset_ip(
	struct tcbd_device *_device,
	u8 _comp_en,
	u8 _comp_rst)
{
	s32 ret = 0;
	_device->processor = _comp_en;

	ret |= tcbd_reg_write(_device, TCBD_SYS_EN, _comp_en);
	ret |= tcbd_reg_write(_device, TCBD_SYS_RESET, _comp_rst);

	return ret;
}

s32 tcbd_change_stream_type(struct tcbd_device *_device, u32 _format)
{
	struct tcbd_mail_data mail = {0, };

	mail.flag = MB_CMD_WRITE;
	mail.cmd = MBPARA_SYS_DAB_STREAM_SET;
	mail.count = 1;
	mail.data[0] = _format;
	_device->selected_stream = _format;
	return tcbd_send_mail(_device, &mail);
}

s32 tcbd_demod_tune_frequency(
	struct tcbd_device *_device, u32 _freq_khz, s32 _bw_khz)
{
	s32 ret = 0;
	u32 sel_stream = 0;

	switch (_device->peri_type) {
	case PERI_TYPE_SPI_ONLY:
	case PERI_TYPE_STS:
		sel_stream = STREAM_TYPE_ALL;
		break;
	default:
		tcbd_debug(DEBUG_ERROR, "not implemented!\n");
		return -1;
	}
#if defined(__STATUS_IN_REGISTER__)
	sel_stream &= ~(STREAM_TYPE_STATUS);
#endif /* __STATUS_IN_REGISTER__ */

	ret = tcbd_change_stream_type(_device, sel_stream);
	ret |= tcbd_send_spur_data(_device, _freq_khz);
	ret |= tcbd_send_frequency(_device, _freq_khz);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR,
			"Failed to send spur and freq to op, err:%d\n", ret);
		return ret;
	}

	if (_device->curr_band != _device->prev_band)
		tcbd_send_agc_data(_device, _device->curr_band);

	ret |= tcbd_dsp_warm_start(_device);
	return ret;
}


s32 tcbd_dsp_cold_start(struct tcbd_device *_device)
{
	s32 ret = 0;
	u16 new_pc = SWAP16(START_PC_OFFSET);

	ret = tcbd_change_memory_view(_device, OP_RAM0_EP_RAM1_PC2);
	ret |= tcbd_reg_write_burst_cont(
		_device, TCBD_INIT_PC, (u8 *)&new_pc, 2);
	ret |= tcbd_reg_write(_device, TCBD_SYS_RESET, TCBD_SYS_COMP_DSP);
	ret |= tcbd_reg_write(
		_device, TCBD_SYS_EN, TCBD_SYS_COMP_DSP | TCBD_SYS_COMP_EP);

	if (ret < 0)
		return ret;

	return tcbd_check_dsp(_device);
}

static inline s32 tcbd_start_demod(struct tcbd_device *_device)
{
	s32 ret = 0;
	struct tcbd_mail_data mail = {0, };

	mail.flag = MB_CMD_READ;
	mail.cmd = MBPARA_SYS_START;
	mail.count = 1;
	mail.data[0] = 0x19;
	switch (_device->clock_type) {
	case CLOCK_24576KHZ:
		mail.data[0] |= 0x1 << 28;
		break;
	case CLOCK_38400KHZ:
		mail.data[0] |= 0x2 << 28;
		break;
	case CLOCK_19200KHZ:
		mail.data[0] |= 0x0 << 28;
		break;
	default:
		break;
	}
	ret |= tcbd_send_mail(_device, &mail);
	ret |= tcbd_recv_mail(_device, &mail);

	return ret;
}

s32 tcbd_dsp_warm_start(struct tcbd_device *_device)
{
	s32 ret = 0;
	struct tcbd_mail_data mail = {0, };

	mail.flag = MB_CMD_WRITE;
	mail.cmd = MBPARA_SYS_WARMBOOT;
	mail.count = 0;

	ret |= tcbd_send_mail(_device, &mail);
	ret |= tcbd_recv_mail(_device, &mail);

	if (ret < 0 || mail.data[0] != 0x1ACCE551) {
		tcbd_debug(DEBUG_ERROR,
			" # Could not warm boot! [%08X]\n", mail.data[0]);
		return -TCERR_WARMBOOT_FAIL;
	}
	ret = tcbd_start_demod(_device);

	if (ret >= 0)
		tcbd_debug(DEBUG_DRV_COMP,
			"Warm boot succeed! [0x%X] ret:%d\n",
				mail.data[0], ret);
	return ret;
}

static s32 tcbd_parse_dsp_rom(
	u8 *_data, u32 _size, struct tcbd_boot_bin *_boot_bin)
{

	u32 idx = 0;
	u32 length;
	s32 table_idx = 0;

	memset(_boot_bin, 0, sizeof(struct tcbd_boot_bin) * MAX_BOOT_TABLE);

	/* cold boot */
	if (_data[idx + 3] != 0x01) {
		tcbd_debug(DEBUG_ERROR, "# Coldboot_preset_Error\n");
		return 0;
	}
	idx += 4;
	length = (_data[idx] << 24) + (_data[idx + 1] << 16) +
			(_data[idx + 2] << 8) + (_data[idx + 3]);
	idx += 4;
	_boot_bin[table_idx].addr = CODE_MEM_BASE;
	_boot_bin[table_idx].data = _data + idx;
	_boot_bin[table_idx].crc = *((u32 *)(_data + idx + length - 4));
	_boot_bin[table_idx++].size = length - 4;
	idx += length;
	_size -= (length + 8);

	/* dagu */
	if (_data[idx + 3] != 0x02) {
		tcbd_debug(DEBUG_ERROR, "# Coldboot_preset_Error\n");
		return 0;
	}
	idx += 4;
	length = (_data[idx] << 24) + (_data[idx + 1] << 16) +
			(_data[idx + 2] << 8) + (_data[idx + 3]);
	idx += 4;
	_boot_bin[table_idx].addr = CODE_TABLEBASE_DAGU;
	_boot_bin[table_idx].data = _data + idx;
	_boot_bin[table_idx++].size = length;
	idx += length;
	_size -= (length + 8);

	/* dint */
	if (_data[idx + 3] != 0x03) {
		tcbd_debug(DEBUG_ERROR, "# Coldboot_preset_Error\n");
		return 0;
	}
	idx += 4;
	length = (_data[idx] << 24) + (_data[idx + 1] << 16) +
			(_data[idx + 2] << 8) + (_data[idx + 3]);
	idx += 4;
	_boot_bin[table_idx].addr = CODE_TABLEBASE_DINT;
	_boot_bin[table_idx].data = _data + idx;
	_boot_bin[table_idx++].size = length;
	idx += length;
	_size -= (length + 8);

	/* rand */
	if (_data[idx + 3] != 0x04) {
		tcbd_debug(DEBUG_ERROR, "# Coldboot_preset_Error\n");
		return 0;
	}

	idx += 4;
	length = (_data[idx] << 24) + (_data[idx + 1] << 16) +
			(_data[idx + 2] << 8) + (_data[idx + 3]);
	idx += 4;
	_boot_bin[table_idx].addr = CODE_TABLEBASE_RAND;
	_boot_bin[table_idx].data = _data + idx;
	_boot_bin[table_idx++].size = length;
	idx += length;
	_size -= (length + 8);

	if (_size >= 8) {
		/* col_order */
		if (_data[idx + 3] != 0x05) {
			tcbd_debug(DEBUG_ERROR, "# Coldboot_preset_Error\n");
			return 0;
		}
		idx += 4;
		length = (_data[idx] << 24) + (_data[idx + 1] << 16) +
				 (_data[idx + 2] << 8) + (_data[idx + 3]);
		idx += 4;

		_boot_bin[table_idx].addr = CODE_TABLEBASE_COL_ORDER;
		_boot_bin[table_idx].data = _data + idx;
		_boot_bin[table_idx++].size = length;
		idx += length;
		_size -= (length + 8);
	}

	/* tcbd_debug(DEBUG_DRV_COMP, " # remain size :%d\n", _size); */
	return table_idx;
}

s32 tcbd_init_dsp(struct tcbd_device *_device, u8 *_boot_code, s32 _size)
{
	s32 i, ret = 0;
	s32 num_table_entry = 0;
	u32 dma_crc = 0;
	u64 tick;
	struct tcbd_boot_bin boot_bin[MAX_BOOT_TABLE];

	tick = tcpal_get_time();
	num_table_entry = tcbd_parse_dsp_rom(_boot_code, _size, boot_bin);

	ret |= tcbd_change_memory_view(_device, EP_RAM0_RAM1);

	for (i = 0; i < num_table_entry && boot_bin[i].size; i++) {
		tcbd_debug(DEBUG_API_COMMON,
			"# download boot to 0x%X, size %d\n",
				boot_bin[i].addr, boot_bin[i].size);
		ret |= tcbd_mem_write(
				_device,
				boot_bin[i].addr,
				boot_bin[i].data,
				boot_bin[i].size);
		ret |= tcbd_reg_read_burst_cont(_device,
				TCBD_CMDDMA_CRC32,
				(u8 *)&dma_crc,
				4);
		if (boot_bin[i].crc &&
				(SWAP32(dma_crc) != boot_bin[i].crc)) {
			tcbd_debug(DEBUG_ERROR,
				"# CRC Error DMA[0x%08X] != BIN[0x%08X]\n",
					dma_crc, boot_bin[i].crc);
			return -TCERR_CRC_FAIL;
		}
	}

	ret = tcbd_dsp_cold_start(_device);
	if (ret < 0)
		tcbd_debug(DEBUG_ERROR, "Failed to cold boot! ret:%d\n", ret);
	else
		tcbd_debug(DEBUG_API_COMMON, "# %llums elapsed to download!\n",
			tcpal_diff_time(tick));
	return ret;
}

s32 tcbd_get_rom_version(struct tcbd_device *_device, u32 *_boot_version)
{
	s32 ret = 0;
	struct tcbd_mail_data mail = {0, };

	mail.flag = MB_CMD_READ;
	mail.cmd = MBPARA_SYS_VERSION;
	mail.count = 0;
	ret = tcbd_send_mail(_device, &mail);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "failed to send mail! %d\n", ret);
		return ret;
	}

	ret = tcbd_recv_mail(_device, &mail);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "failed to recv mail! %d\n", ret);
		return ret;
	}

	memcpy(_boot_version, &mail.data[0], sizeof(u32));

	return ret;
}

s32 tcbd_check_dsp_status(struct tcbd_device *_device)
{
	s32 i, j, len;
	s8 *item_name[] = {"   CTO", "   CFO", "   SFE", "RESYNC" };
	s8 dbg_buff[255];
	static struct tcbd_mail_data check_item[] = {
		{MB_CMD_READ, 2, MBCMD_CTO_DAB_RESULT, },
		{MB_CMD_READ, 3, MBCMD_CFO_DAB_RESULT, },
		{MB_CMD_READ, 3, MBCMD_SFE_DAB_RESULT, },
		{MB_CMD_READ, 7, MBPARA_SYS_DAB_RESYNC_RESULT, }
	};

	for (i = 0; i < ARRAY_SIZE(check_item); i++) {
		tcbd_read_mail_box(_device,
			check_item[i].cmd,
			check_item[i].count,
			check_item[i].data);
		len = sprintf(dbg_buff, " %s ", item_name[i]);
		for (j = 0; j < check_item[i].count; j++) {
			len += sprintf(dbg_buff + len, "[%d] = 0x%08X, ",
				j, check_item[i].data[j]);
		}
		tcbd_debug(DEBUG_INFO, "%s\n", dbg_buff);
	}
	return 0;
}
