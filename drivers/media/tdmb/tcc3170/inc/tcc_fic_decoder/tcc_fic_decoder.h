/*
 * tcc_fic_decoder.h
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

#ifndef	__TCC_FIC_DECODER_H__
#define	__TCC_FIC_DECODER_H__
#include "tcc_fic_fig.h"

#define CH_UPDATE_NO_DATA		0
#define CH_UPDATE_ESSENTIAL_DATA	1
#define CH_UPDATE_FULL_DATA		2

#define MAX_FIC_SIZE	384
#define TCC_FIB_SIZE	32

struct fic_parser_matadata {
	struct tcc_ensemble esmbl_start;
	struct tcc_service svc_start[NUM_SVC];
	struct tcc_service_comp svc_comp_start[NUM_SVC_COMP];
	struct tcc_sub_channel subch_start[NUM_SUB_CH];
	struct tcc_user_app_types user_app_start[NUM_USER_APP];
	struct tcc_program_type	prg_start[NUM_SVC_COMP];
	struct tcc_xpad_user_app fig1_6_start[NUM_USER_APP];

	u8 fig_cn;
	u8 fig_oe0;
	u8 fig_pd;
	u8 fig_oe1;

	u8 reconf_stage;
	u8 cif_count_hi;
	u8 cif_count_lo;
	u16 cif_count;
	u8 occur_change;

	u32 fib_cnt; /* max 12 */
};

#define MAX_SVC_COMP_NUM 2
#define MAX_SVC_NUM 10

struct tcc_service_comp_info {
	struct tcc_service_comp svc_comp;
	struct tcc_sub_channel sub_ch;
};

struct tcc_service_info {
	struct tcc_service svc;
	struct tcc_service_comp_info svc_comp_info[MAX_SVC_COMP_NUM];
};

struct tcc_ensemble_info {
	struct tcc_ensemble ensbl;
	struct tcc_service_info svc_info[MAX_SVC_NUM];
};

s32 fig0_ext00(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen);
s32 fig0_ext01(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen);
s32 fig0_ext02(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen);
s32 fig0_ext03(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen);
s32 fig0_ext04(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen);
s32 fig0_ext05(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen);
s32 fig0_ext07(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen);
s32 fig0_ext08(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen);
s32 fig0_ext13(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen);
s32 fig0_ext17(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen);

s32 fig1_ext00(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen, u8 charset);
s32 fig1_ext01(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen, u8 charset);
s32 fig1_ext04(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen, u8 charset);
s32 fig1_ext05(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen, u8 charset);
s32 fig1_ext06(struct fic_parser_matadata *fic_info,
	u8 *fig_buff, s32 iLen, u8 charset);

void tcc_fic_parser_init(void);
s32 tcc_fic_run_decoder(u8 *buff, s32 size);

void tcc_fic_disp_ensbl_info(struct tcc_ensemble_info *ensbl_info);
struct tcc_ensemble_info *tcc_fic_get_ensbl_info(s32 _disp);

struct tcc_service_comp_info *tcc_fic_get_svc_comp_info(s32 _subch_id);
u8 tcc_fic_get_ptype(struct tcc_service_comp_info *svc_comp_info);
u8 tcc_fic_get_plevel(struct tcc_service_comp_info *svc_comp_info);
u16 tcc_fic_get_cu_start(struct tcc_service_comp_info *svc_comp_info);
u16 tcc_fic_get_cu_size(struct tcc_service_comp_info *svc_comp_info);
u8 tcc_fic_get_subch_id(struct tcc_service_comp_info *svc_comp_info);
u8 tcc_fic_get_bitrate(struct tcc_service_comp_info *svc_comp_info);
u8 tcc_fic_get_rs(struct tcc_service_comp_info *svc_comp_info);


#endif /* __TCC_FIC_DECODER_H__ */
