/*
 * tcc_fic_fig1.c
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

static u8 *fig_put_in_label(u8 *label, u8 *fig_buff)
{
	u8 m;

	for (m = 0; m < 16; m++)
		*label++ = *fig_buff++;
	label--;
	for (; *label == 0x20 || *label == 0; label--)
		*label = 0;

	return fig_buff;
}

s32 fig1_ext00(struct fic_parser_matadata *fic_info,
	u8 *fig_buff,
	s32 length,
	u8 charset)
{
	s32 ret = FICERR_SUCCESS;
	u16 tmp;
	struct tcc_ensemble *ensmbl;

	ensmbl = &fic_info->esmbl_start;

	tmp = (*fig_buff++) << 8;
	tmp |= (*fig_buff++);

	ensmbl->charset = charset;
	fig_buff = fig_put_in_label(ensmbl->label, fig_buff);
	ensmbl->char_flag = (*fig_buff++)<<8;
	ensmbl->char_flag |= *fig_buff;

	return ret;
}

/*
 * Program sService label
 */
s32 fig1_ext01(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 length, u8 charset)
{
	s32 ret = FICERR_SUCCESS;
	u16 sid;
	u8 i;
	struct tcc_ensemble *ensmbl;
	struct tcc_service *svc_start;

	ensmbl = &fic_info->esmbl_start;
	svc_start = fic_info->svc_start;

	if (svc_start == 0)
		return FICERR_FIG1_1_NO_SERVICEARRAY;

	sid = (*fig_buff++) << 8;
	sid |= (*fig_buff++);
	for (i = 0; i < ensmbl->num_svc; i++) {
		svc_start = fic_info->svc_start + i;
		if (sid == svc_start->sid)
			break;
	}

	if (i == ensmbl->num_svc) {
		ret = FICERR_FIG1_1_NOTREADY_SERVICE;
		return ret;
	}

	svc_start->charset = charset;
	fig_buff = fig_put_in_label(svc_start->svc_label, fig_buff);
	svc_start->char_flag = (*fig_buff++)<<8;
	svc_start->char_flag |= *fig_buff;

	return ret;
}

/*
 * sService Component label
 */
s32 fig1_ext04(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 length, u8 charset)
{
	s32 ret = FICERR_SUCCESS;
	u32 sid;
	u8 i, scids, pd;
	struct tcc_service_comp *svc_comp_start;
	struct tcc_ensemble *ensmbl;

	ensmbl = &fic_info->esmbl_start;
	svc_comp_start = fic_info->svc_comp_start;

	if (svc_comp_start == 0)
		return FICERR_FIG1_4_NO_SRVCOMPARRAY;

	/* If no service informaion, return */
	if (ensmbl->num_svc_comp == 0) {
		ret = FICERR_FIG1_4_NOTREADY_SRVCOMP;
		return ret;
	}

	pd = (*fig_buff & 0x80) ? 1 : 0;
	scids = *fig_buff++ & 0x0f;
	if (pd) {
		sid = (u32)(*fig_buff++ << 24);
		sid |= (u32)(*fig_buff++ << 16);
		sid |= (u32)(*fig_buff++ << 8);
		sid |= (u32)(*fig_buff++);
	} else {
		sid = (u32)(*fig_buff++ << 8);
		sid |= (u32)(*fig_buff++);
	}

		for (i = 0; i < ensmbl->num_svc_comp; i++) {
			svc_comp_start = fic_info->svc_comp_start + i;
			if (scids == svc_comp_start->scids &&
				sid == svc_comp_start->sid)
				break;
		}
	if (i == ensmbl->num_svc_comp) {
		ret = FICERR_FIG1_4_NOTREADY_SRVCOMP1;
		return ret;
	}

	svc_comp_start->charset = charset;
	fig_buff = fig_put_in_label(svc_comp_start->label, fig_buff);

	svc_comp_start->char_flag = (*fig_buff++) << 8;
	svc_comp_start->char_flag |= *fig_buff;

	return ret;
}

/*
 * Data sService label
 */
s32 fig1_ext05(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 length, u8 charset)
{
	s32 ret = FICERR_SUCCESS;
	u32 sid;
	u32 j;
	struct tcc_ensemble *ensmbl;
	struct tcc_service *svc_start;

	ensmbl = &fic_info->esmbl_start;
	svc_start = fic_info->svc_start;

	if (svc_start == 0)
		return FICERR_FIG1_5_NO_SERVICEARRAY;

	sid = (*fig_buff++) << 24;
	sid |= (*fig_buff++) << 16;
	sid |= (*fig_buff++) << 8;
	sid |= (*fig_buff++);
	for (j = 0; j < ensmbl->num_svc; j++) {
		svc_start = fic_info->svc_start + j;
		if (sid == svc_start->sid)
			break;
	}

	if (j == ensmbl->num_svc) {
		ret = FICERR_FIG1_5_NOTREADY_SERVICE;
		return ret;
	}

	svc_start->charset = charset;
	fig_buff = fig_put_in_label(svc_start->svc_label, fig_buff);
	svc_start->char_flag = (*fig_buff++) << 8;
	svc_start->char_flag |= *fig_buff;

	return ret;
}

s32 fig1_ext06(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 length, u8 charset)
{
	s32 ret = FICERR_SUCCESS;
	s32 p_d;
	u32 sid;
	u8 scids;
	u8 xpad_appl_type;
	s32 i, empty_idx;

	struct tcc_ensemble *ensmbl;
	struct tcc_xpad_user_app *user_app_start;

	ensmbl = &fic_info->esmbl_start;
	user_app_start = fic_info->fig1_6_start;

	if (user_app_start == 0)
		return FICERR_FIG1_6_NO_XPADLABELARRAY;

	p_d = fig_buff[0] >> 7;
	scids = fig_buff[0] & 0x0f;
	fig_buff++;
	if (p_d) {
		sid = (fig_buff[0] << 24) | (fig_buff[1] << 16) |
			(fig_buff[2] << 8) | fig_buff[3];
		fig_buff += 4;
	} else {
		sid = (fig_buff[0] << 8) | fig_buff[1];
		fig_buff += 2;
	}
	xpad_appl_type = fig_buff[0] & 0x1f;
	fig_buff++;

	empty_idx = -1;

	for (i = 0; i < NUM_USER_APP; i++) {
		if (user_app_start[i].sid == sid &&
			user_app_start[i].scids == scids &&
			user_app_start[i].type == xpad_appl_type) {
			empty_idx = i;
			break;
		}
		if (empty_idx == -1 && user_app_start[i].sid == 0)
			empty_idx = i;
	}

	if (empty_idx != -1) {
		user_app_start[empty_idx].sid = sid;
		user_app_start[empty_idx].scids = scids;
		user_app_start[empty_idx].charset = charset;
		user_app_start[empty_idx].type = xpad_appl_type;
		fig_buff = fig_put_in_label(
				user_app_start[empty_idx].label, fig_buff);
		user_app_start[empty_idx].char_flag = (*fig_buff++) << 8;
		user_app_start[empty_idx].char_flag |= *fig_buff;
	}

	return ret;
}
