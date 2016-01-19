/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : ficdecoder.h

	Description : fic parser

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA


	History :
	----------------------------------------------------------------------
*******************************************************************************/
#ifndef __ficdecodera_h__
#define __ficdecodera_h__

#include "fci_types.h"

#define MAX_ESB_NUM	1
#define MAX_SVC_NUM	128
#define MAX_SC_NUM	128
#define MAX_SUBCH_NUM	64
#define MAX_DIDP_NUM	8

#define MAX_USER_APPL_NUM       15
#define MAX_USER_APPL_DATA_SIZE 24

struct fig {
	u8	head;
	u8	data[29];
};

struct fib {
	u8	data[30];
	u16	crc;
};

struct fic {
	/* Fib		fib[12]; */
	struct fib	fib[32];
};

struct figdata {
	u8	head;
	u8	data[28];
};

struct esbinfo_t {
	u8	flag;
	u16	eid;
	u8	label[32];
	u8	ecc;
};

struct service_info_t {
	u8	flag;
	u32	sid;
	u16	scid;
	u8	ascty;
	u8	dscty;
	u8	fidc_id;
	u8	addrType;		/* PD */
	u8	tmid;
	u8	sub_channel_id;
	u8	nscps;
	u8	label[32];

	u8  scids;
	u8  num_of_user_appl;
	u16 user_appl_type[MAX_USER_APPL_NUM];
	u8  user_appl_length[MAX_USER_APPL_NUM];
	u8  user_appl_data[MAX_USER_APPL_NUM][MAX_USER_APPL_DATA_SIZE];
};

struct scInfo_t {
	u8	flag;
	u16	scid;
	u8	dscty;
	u8	sub_channel_id;
	u8	scca_flag;
	u8	dg_flag;
	u16	packet_address;
	u16	scca;
	u8	label[32];
};

struct subch_info_t {
	u8	flag;
	u8	subchannel_id;
	u16	start_address;
	u8	form_type;
	u8	table_index;
	u8	table_switch;
	u8	option;
	u8	protect_level;
	u16	subch_size;
	u32	sid;
	u8	service_channel_id;
	u8	re_config;

	/* T-MMB */
	u8	mode;			/* 0 T-DMB, 1 T-MMB */
	u8	mod_type;
	u8	enc_type;
	u8	intv_depth;
	u8	pl;
	/* T-MMB */

	 /* FEC */
	u8  fec_schem;

};

struct didp_info_t {
	u8	flag;
	u8	reconfig_offset;
	u8	subchannel_id;
	u16	start_address;
	u8	form_type;
	u16	subch_size;
	u16	speed;			/* kbsp */
	u16	l1;
	u8	p1;
	u16	l2;
	u8	p2;
	u16	l3;
	u8	p3;
	u16	l4;
	u8	p4;
	u8	pad;
	/* T-MMB */
	u8	mode;			/* 0 T-DMB, 1 T-MMB */
	u8	mod_type;
	u8	enc_type;
	u8	intv_depth;
	u8	pl;
	u16	mi;			/* kies use */
	/* T-MMB */
};

#ifdef __cplusplus
	extern "C" {
#endif

extern struct esbinfo_t		ensemble_info[MAX_ESB_NUM];
extern struct service_info_t	service_info[MAX_SVC_NUM];
extern struct subch_info_t	subchannel_info[MAX_SUBCH_NUM];
extern struct didp_info_t	didpInfo[MAX_DIDP_NUM];


extern int fic_decoder(struct fic *pfic, u16 length);
extern int fib_decoder(struct fib *pfib);
extern struct esbinfo_t *get_emsemble_info(void);
extern struct subch_info_t *get_subchannel_info(u8 subchannel_id);
extern struct service_info_t *get_service_info(u32 sid);
extern struct scInfo_t *get_sc_info(u16 scid);
extern struct service_info_t *get_service_info_list(u8 service_index);
extern void subchannel_org_clean(void);
extern void subchannel_org_prn(int subchannel_id);
extern int found_all_labels(void);
extern void didp_prn(struct didp_info_t *pdidp);
extern int set_didp_reg(int service_channel_id, struct didp_info_t *pdidp);
extern int  subchannel_org_to_didp(
	struct subch_info_t *sub_ch_info, struct didp_info_t *pdidp);
extern void subchannel_org_prn(int subchannel_id);
extern int dummy_decoder(u8 *fibBuffer, int figLength);


#ifdef __cplusplus
	} /* extern "C" {*/
#endif

#endif /* __ficdecoder_h__ */
