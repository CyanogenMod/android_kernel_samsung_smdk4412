/*
 * tcc_fic_decoder.c
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

#include "tcc_fic_decoder.h"

/******************************************************************************
*						UEP Table
*	+---------------+-----------+-----------------------------------+
*	|Bit rate(4bits)|Protect Lv |        SubCh Size(9bits)          |
*	|---------------|---+---+---|---+---+---+---+---+---+---+---+---|
*	| x | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x |
*	+---------------+-----------+-----------------------------------+
*
*******************************************************************************/
static const unsigned short uep_table[64] = {
	0x0810, 0x0615, 0x0418, 0x021d, 0x0023, 0x1818, 0x161d, 0x1423,
	0x122a, 0x1034, 0x281d, 0x2623, 0x242a, 0x2234, 0x3820, 0x362a,
	0x3430, 0x323a, 0x3046, 0x4828, 0x4634, 0x443a, 0x4246, 0x4054,
	0x5830, 0x563a, 0x5446, 0x5254, 0x5068, 0x683a, 0x6646, 0x6454,
	0x6268, 0x7840, 0x7654, 0x7460, 0x7274, 0x708c, 0x8850, 0x8668,
	0x8474, 0x828c, 0x80a8, 0x9860, 0x9674, 0x948c, 0x92a8, 0x90d0,
	0xa874, 0xa68c, 0xa4a8, 0xa2d0, 0xa0e8, 0xb880, 0xb6a8, 0xb4c0,
	0xb2e8, 0xb118, 0xc8a0, 0xc6d0, 0xc318, 0xd8c0, 0xd518, 0xd1a0 };

static struct fic_parser_matadata parser_metadata;
static struct tcc_ensemble_info user_ensbl_info;

static s32 parsing_fig0(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 length)
{
	s32 ret = FICERR_SUCCESS;
	u8 extension;

	fic_info->fig_cn = (*fig_buff & 0x80) >> 7;
	fic_info->fig_oe0 = (*fig_buff & 0x40) >> 6;
	fic_info->fig_pd = (*fig_buff & 0x20) >> 5;
	extension = (*fig_buff++ & 0x1f);

	length--;
	/* service link FIG */
	if ((extension != 6) && (extension != 21) && (extension != 24)) {
		if (fic_info->fig_cn) {
			if (fic_info->reconf_stage) {
				if (extension > 0)
					return FICERR_FIG0_NEXT_FIG;
			} else {
				return FICERR_FIG0_NEXT_FIG1;
			}
		}
	}

	switch (extension) {
	case EXT_00:
		ret = fig0_ext00(fic_info, fig_buff, length);
		break;
	case EXT_01:
		ret = fig0_ext01(fic_info, fig_buff, length);
		break;
	case EXT_02:
		ret = fig0_ext02(fic_info, fig_buff, length);
		break;
	case EXT_03:
		ret = fig0_ext03(fic_info, fig_buff, length);
		break;
	case EXT_04:
		ret = fig0_ext04(fic_info, fig_buff, length);
		break;
	case EXT_05:
		ret = fig0_ext05(fic_info, fig_buff, length);
		break;
	case EXT_08:
		ret = fig0_ext08(fic_info, fig_buff, length);
		break;
	case EXT_13:
		ret = fig0_ext13(fic_info, fig_buff, length);
		break;
	case EXT_17:
		ret = fig0_ext17(fic_info, fig_buff, length);
		break;
	default:
		break;
	}
	return ret;
}


static s32 parsing_fig1(
	struct fic_parser_matadata *fic_info,
	u8 *fig_buff,
	s32 length)
{
	s32 ret;
	u8 extension, charset;

	fic_info->fig_oe1 = (*fig_buff & 0x08) >> 3;

	if (fic_info->fig_oe1 != 0)
		return FICERR_FIG_NODATA;

	charset = (*fig_buff & 0xf0) >> 4;
	extension = (*fig_buff++ & 0x07);
	length--;
	switch (extension) {
	case EXT_00:
		ret = fig1_ext00(fic_info, fig_buff, length, charset);
		break;
	case EXT_01:
		ret = fig1_ext01(fic_info, fig_buff, length, charset);
		break;
	case EXT_04:
		ret = fig1_ext04(fic_info, fig_buff, length, charset);
		break;
	case EXT_05:
		ret = fig1_ext05(fic_info, fig_buff, length, charset);
		break;
	case EXT_06:
		ret = fig1_ext06(fic_info, fig_buff, length, charset);
	default:
		break;
	}

	return FICERR_SUCCESS;
}

static u32 fic_channel_updated(void)
{
	u32 i, j;
	u32 num_svc, num_svc_comp, num_subch;

	struct fic_parser_matadata *metadata = &parser_metadata;
	struct tcc_ensemble *esmbl = &metadata->esmbl_start;
	struct tcc_service *svc = metadata->svc_start;
	struct tcc_service_comp *svc_comp = metadata->svc_comp_start;
	struct tcc_user_app_types *user_app = metadata->user_app_start;

	num_svc = esmbl->num_svc;
	num_svc_comp = esmbl->num_svc_comp;
	num_subch = esmbl->num_svc_comp;

	if (!num_svc_comp)
		return CH_UPDATE_NO_DATA;

	if (!num_subch)
		return CH_UPDATE_NO_DATA;

	if (esmbl->label[0] == 0x00)
		return CH_UPDATE_NO_DATA;

	if (num_svc) {
		for (i = 0; i < num_svc; i++) {
			if ((svc + i)->svc_label[0] == 0)
				return CH_UPDATE_NO_DATA;
		}
	} else {
		return CH_UPDATE_NO_DATA;
	}

	for (i = 0; i < num_svc_comp; i++) {
		if ((svc_comp + i)->fidc_id == 0xff)
			return CH_UPDATE_NO_DATA;
	}

	for (i = 0; i < num_svc_comp; i++) {
		if (svc_comp[i].tmid == 3 || svc_comp[i].ascty_dscty == 5) {
			if (svc_comp[i].ascty_dscty == 0)
				return CH_UPDATE_ESSENTIAL_DATA;

			for (j = 0; j < num_svc_comp; j++) {
				if (svc_comp[i].sid == user_app[j].sid &&
					svc_comp[i].scids == user_app[j].scids)
					break;
			}
			if (j == num_svc_comp)
				return CH_UPDATE_ESSENTIAL_DATA;

			if (svc_comp[i].scids == 0x0f)
				return CH_UPDATE_ESSENTIAL_DATA;
		}

		if (svc_comp[i].order) {
			if ((svc_comp + i)->label[0] == 0)
				return CH_UPDATE_ESSENTIAL_DATA;
		}
	}

	return CH_UPDATE_FULL_DATA;
}

void tcc_fic_parser_init(void)
{
	s32 i;
	struct fic_parser_matadata *metadata;

	metadata = &parser_metadata;
	memset(metadata, 0, sizeof(parser_metadata));

	metadata->reconf_stage = 0;
	metadata->fig_cn = 0;
	metadata->fig_oe0 = 0;
	metadata->fig_pd = 0;
	metadata->fig_oe1 = 0;
	metadata->fib_cnt = 0;

	for (i = 0; i < NUM_SVC_COMP; i++)
		metadata->svc_comp_start[i].scids = 0x0f;

	for (i = 0; i < NUM_USER_APP; i++)
		metadata->user_app_start[i].scids = 0x0f;
}

static s32 fib_decode(u8 *fig_buff)
{
	s32 ret = FICERR_SUCCESS;
	struct fic_parser_matadata *fic_info;

	s32 fig_len;
	u32 fig_type;
	u32 buff_point = 0;
	s32 buff_len = 30;

	fic_info = &parser_metadata;

	if (ret != FICERR_SUCCESS)
		return ret;

	/* the size of FIG is bigger than 2bytes */
	while (buff_len > 0) {
		/* find a FIG header */
		if (*(fig_buff + buff_point) == 0xff ||
			*(fig_buff + buff_point) == 0x00) {
			ret = FICERR_FIBD_ENDMARKER;
			break;
		}

		fig_type = (*(fig_buff + buff_point) & 0xe0) >> 5;
		fig_len = (*(fig_buff + buff_point) & 0x1f);
		if (fig_len > 29 || fig_len == 0)
			return FICERR_FIBD_INVALID_LENGTH;

		buff_point++;
		if (fig_len <= 0x01) {
			ret = FICERR_FIG_NODATA;
			return ret;
		}

		switch (fig_type) {
		case FIG0:
			ret = parsing_fig0(fic_info,
				fig_buff + buff_point, fig_len);
			break;
		case FIG1:
			ret = parsing_fig1(fic_info,
				fig_buff+buff_point, fig_len);
			break;
		default:
			ret = FICERR_FIBD_UNKNOWN_FIGTYPE;
			return ret;
		}

		buff_point += fig_len;
		buff_len -= (fig_len + 1);
	}

	if (buff_len < 0)
		ret = FICERR_FIBD_INVALID_LENGTH;
	return ret;
}

static u16 fib_crc16(u8 *buf)
{
	u32 b, len;
	u8 crcl, crcm;

	crcl = 0xff;
	crcm = 0xff;


	for (len = 0; len < 30; len++) {
		b = *(buf + len) ^ crcm;
		b = b ^ (b >> 4);
		crcm = crcl ^ (b >> 3) ^ (b << 4);
		crcl = b ^ (b << 5);
	}

	crcl = crcl ^ 0xff;
	crcm = crcm ^ 0xff;
	return (u16)crcl | ((u16)crcm << 8);
}


static void fic_copy_sub_ch(struct fic_parser_matadata *fic_info,
	struct tcc_service_comp_info *svc_comp_info)
{
	s32 i;
	struct tcc_ensemble *esbl = &fic_info->esmbl_start;
	struct tcc_sub_channel *subch_start = fic_info->subch_start;

	for (i = 0; i < esbl->num_svc_comp; i++) {
		if (svc_comp_info->svc_comp.fidc_id != subch_start[i].subch_id)
			continue;
		memcpy(&svc_comp_info->sub_ch, &subch_start[i],
			sizeof(struct tcc_sub_channel));
		break;
	}
}

static void fic_copy_svc_comp(struct fic_parser_matadata *fic_info,
	struct tcc_service_info *svc_info)
{
	s32 i, j = 0;
	struct tcc_ensemble *esbl = &fic_info->esmbl_start;
	struct tcc_service_comp *src_svc_comp = fic_info->svc_comp_start;
	struct tcc_service_comp_info *svc_comp_info;

	for (i = 0; i < esbl->num_svc_comp; i++) {
		if (svc_info->svc.sid != src_svc_comp[i].sid)
			continue;
		svc_comp_info = &svc_info->svc_comp_info[j];
		memcpy(&svc_comp_info->svc_comp, &src_svc_comp[i],
			sizeof(struct tcc_service_comp));
		fic_copy_sub_ch(fic_info, svc_comp_info);
		j++;
	}
}

struct tcc_service_comp_info *tcc_fic_get_svc_comp_info(s32 _subch_id)
{
	s32 i;
	struct tcc_service_comp_info *svc_comp_info;
	struct tcc_ensemble *ensbl = &user_ensbl_info.ensbl;
	struct tcc_service_info *svc_info = user_ensbl_info.svc_info;

	for (i = 0; i < ensbl->num_svc_comp; i++) {
		svc_comp_info = &svc_info[i].svc_comp_info[0];
		if (svc_comp_info->svc_comp.fidc_id == _subch_id)
			return svc_comp_info;
	}
	return NULL;
}

u8 tcc_fic_get_ptype(struct tcc_service_comp_info *svc_comp_info)
{
	struct tcc_sub_channel *sub_ch = &svc_comp_info->sub_ch;
	if (sub_ch->form_flag & 0x08)
		return 1;
	else
		return 0;
}

u8 tcc_fic_get_plevel(struct tcc_service_comp_info *svc_comp_info)
{
	u8 eep8[4] = {0, 1, 2, 3};
	u8 eep32[4] = {4, 5, 6, 7};
	u8 opt, protect;
	struct tcc_sub_channel *sub_ch = &svc_comp_info->sub_ch;

	if (sub_ch->form_flag & 0x08) {
		opt = (sub_ch->form_flag & 0x04) ? 1 : 0;
		protect = sub_ch->form_flag & 0x03;
		if (opt)
			return eep32[protect];
		else
			return eep8[protect];
	} else {
		return (uep_table[sub_ch->tbl_index] & 0x0E00) >> 9;
	}
}

u16 tcc_fic_get_cu_size(struct tcc_service_comp_info *svc_comp_info)
{
	struct tcc_sub_channel *sub_ch = &svc_comp_info->sub_ch;

	if (sub_ch->form_flag & 0x08)
		return sub_ch->size_cu;
	else
		return uep_table[sub_ch->tbl_index] & 0x1FF;
}

u16 tcc_fic_get_cu_start(struct tcc_service_comp_info *svc_comp_info)
{
	struct tcc_sub_channel *sub_ch = &svc_comp_info->sub_ch;

	return sub_ch->size_cu;
}

u8 tcc_fic_get_subch_id(struct tcc_service_comp_info *svc_comp_info)
{
	struct tcc_sub_channel *sub_ch = &svc_comp_info->sub_ch;

	return sub_ch->subch_id;
}


u8 tcc_fic_get_bitrate(struct tcc_service_comp_info *svc_comp_info)
{
	u8 opt, protect;
	struct tcc_sub_channel *sub_ch = &svc_comp_info->sub_ch;

	if ((sub_ch->form_flag & 0x08) == 0)
		return (uep_table[sub_ch->tbl_index] & 0xf000) >> 12;

	opt = (sub_ch->form_flag & 0x04) ? 1 : 0;
	protect = sub_ch->form_flag & 0x03;
	if (opt) {
		switch (protect) {
		case 0:
			return sub_ch->size_cu / 27;
		case 1:
			return sub_ch->size_cu / 21;
		case 2:
			return sub_ch->size_cu / 18;
		case 3:
			return sub_ch->size_cu / 15;
		default:
			return 0;
		}
		/*human readable bitrate is x32*/
	} else {
		switch (protect) {
		case 0:
			return sub_ch->size_cu / 12;
		case 1:
			return sub_ch->size_cu / 8;
		case 2:
			return sub_ch->size_cu / 6;
		case 3:
			return sub_ch->size_cu / 4;
		default:
			return 0;
		}
		/*human readable bitrate is x8*/
	}
}

u8 tcc_fic_get_rs(struct tcc_service_comp_info *svc_comp_info)
{
	struct tcc_service_comp *svc_comp = &svc_comp_info->svc_comp;
	if (svc_comp->ascty_dscty == 0x18)
		return 1;
	else
		return 0;
}

/**
* @param buff buffer for fic
* @param size fic buffer size. it must be 384 bytes.
* @return -1 : invalid argument, 0 : in progress, > 0 : parsing done
*/
s32 tcc_fic_run_decoder(u8 *buff, s32 size)
{
	s32 cnt = 0, ret = 0;
	s32 num_broken_fib = 0;
	u16 fib_crc;

	if (size != MAX_FIC_SIZE) {
		tcbd_debug(DEBUG_ERROR, "invalid fic size!\n");
		return -1;
	}
	while (size > cnt * TCC_FIB_SIZE) {
		fib_crc = (u16)(buff[(cnt * TCC_FIB_SIZE) + 30] << 8) |
				(u16)buff[(cnt * TCC_FIB_SIZE) + 31];
		if (fib_crc != fib_crc16(buff + (cnt * TCC_FIB_SIZE))) {
			num_broken_fib++;
			tcbd_debug(DEBUG_ERROR, "broken fib cnt:%d\n", cnt);
			goto cont_loop;
		}
		fib_decode(buff + (cnt * TCC_FIB_SIZE));
		ret = fic_channel_updated();
		if (ret == CH_UPDATE_ESSENTIAL_DATA ||
			ret == CH_UPDATE_FULL_DATA) {
			break;
		}
cont_loop:
		cnt++;
	}
	return ret;
}

void tcc_fic_disp_ensbl_info(struct tcc_ensemble_info *ensbl_info)
{
	s32 i, j;
	struct tcc_ensemble *esbl = &ensbl_info->ensbl;
	struct tcc_service_info *svc_info;
	struct tcc_service_comp_info *svc_comp_info;

	tcbd_debug(DEBUG_INFO, "[ %s ] %d services available!\n",
				esbl->label, esbl->num_svc);
	for (i = 0; i < esbl->num_svc; i++) {
		svc_info = &ensbl_info->svc_info[i];
		tcbd_debug(DEBUG_INFO, " => [ %s ] %d service component\n",
					svc_info->svc.svc_label,
					svc_info->svc.num_svc_comp);
		for (j = 0; j < svc_info->svc.num_svc_comp; j++) {
			svc_comp_info = &svc_info->svc_comp_info[j];
			tcbd_debug(DEBUG_INFO, "  -> sub channel id [%d]\n",
					svc_comp_info[j].svc_comp.fidc_id);
		}
	}
}

struct tcc_ensemble_info *tcc_fic_get_ensbl_info(s32 _disp)
{
	s32 i;
	struct fic_parser_matadata *fic_info = &parser_metadata;
	struct tcc_ensemble *esbl = &fic_info->esmbl_start;
	struct tcc_service *svc = fic_info->svc_start;
	struct tcc_service_info *svc_info;

	memcpy(&user_ensbl_info.ensbl, esbl, sizeof(struct tcc_ensemble));
	for (i = 0; i < esbl->num_svc; i++) {
		svc_info = &user_ensbl_info.svc_info[i];
		memcpy(&svc_info->svc, &svc[i], sizeof(struct tcc_service));
		fic_copy_svc_comp(fic_info, svc_info);
	}

	if (_disp)
		tcc_fic_disp_ensbl_info(&user_ensbl_info);

	return &user_ensbl_info;
}

