/*
*
* File name: drivers/media/tdmb/mtv318/src/raontv_ficdec.c
*
* Description : RAONTECH FIC Decoder driver.
*
* Copyright (C) (2011, RAONTECH)
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

#include <linux/string.h>
#include "raontv_ficdec_internal.h"
#include "raontv_internal.h"

struct ensemble_info_type ensble;

/*****************************/
/* FIC Information Variable  */
/*****************************/
static struct FIG_DATA fig_data;
static const U8 FIC_BIT_MASK[8] = {0x0, 0x80, 0xC0, 0xE0, 0xFF,
	0xF8, 0xFC, 0xFE};


/**********************************/
/*  FIC information get function  */
/**********************************/
static S32 Get_Bytes(U8 cnt, void *Res)
{
	S32 i;

	for (i = 0; i < cnt; i++) {
		*((U8 *) Res+i) = *(fig_data.data+fig_data.byte_cnt);
		fig_data.byte_cnt++;
	}

	return RTV_OK;
}

static S32 Get_Bits(U8 cnt, void *Res)
{
	*(U8 *)Res = (U8) (*(fig_data.data+fig_data.byte_cnt) &
			(FIC_BIT_MASK[cnt] >> fig_data.bit_cnt));

	*(U8 *)Res = (*(U8 *)Res) >> (8-cnt-fig_data.bit_cnt);
	fig_data.bit_cnt += cnt;
	if (fig_data.bit_cnt == 8) {
		fig_data.byte_cnt++;
		fig_data.bit_cnt = 0;
	}
	return RTV_OK;
}

void *Get_FIG0_EXT[] = {
	(void *) Get_FIG0_EXT0,  (void *) Get_FIG0_EXT1,
	(void *) Get_FIG0_EXT2,  (void *) Get_FIG0_EXT3,
	(void *) Get_FIG0_EXT4,  (void *) Get_FIG0_EXT5,
	(void *) Get_FIG0_EXT6,  (void *) Get_FIG0_EXT7,
	(void *) Get_FIG0_EXT8,  (void *) Get_FIG0_EXT9,
	(void *) Get_FIG0_EXT10, (void *) Get_FIG0_EXT11,
	(void *) Get_FIG0_EXT12, (void *) Get_FIG0_EXT13,
	(void *) Get_FIG0_EXT14, (void *) Get_FIG0_EXT15,
	(void *) Get_FIG0_EXT16, (void *) Get_FIG0_EXT17,
	(void *) Get_FIG0_EXT18, (void *) Get_FIG0_EXT19,
	(void *) Get_FIG0_EXT20, (void *) Get_FIG0_EXT21,
	(void *) Get_FIG0_EXT22, (void *) Get_FIG0_EXT23,
	(void *) Get_FIG0_EXT24, (void *) Get_FIG0_EXT25,
	(void *) Get_FIG0_EXT26, (void *) Get_FIG0_EXT27,
	(void *) Get_FIG0_EXT28, (void *) Get_FIG0_EXT29,
	(void *) Get_FIG0_EXT30, (void *) Get_FIG0_EXT31,
	0
};

void *Get_FIG1_EXT[] = {
	(void *) Get_FIG1_EXT0, (void *) Get_FIG1_EXT1,
	(void *) Get_FIG1_EXT2, (void *) Get_FIG1_EXT3,
	(void *) Get_FIG1_EXT4, (void *) Get_FIG1_EXT5,
	(void *) Get_FIG1_EXT6, (void *) Get_FIG1_EXT7,
	0
};

void *Get_FIG2_EXT[] = {
	(void *) Get_FIG2_EXT0, (void *) Get_FIG2_EXT1,
	(void *) Get_FIG2_EXT2, (void *) Get_FIG2_EXT3,
	(void *) Get_FIG2_EXT4, (void *) Get_FIG2_EXT5,
	(void *) Get_FIG2_EXT6, (void *) Get_FIG2_EXT7,
	0
};

void *Get_FIG5_EXT[] = {
	(void *) Get_FIG5_EXT0, (void *) Get_FIG5_EXT1, (void *) Get_FIG5_EXT2,
	0
};

void *FIG_PARSER[] = {
	(void *) MCI_SI_DEC,
	(void *) SI_LABEL_DEC1,
	(void *) SI_LABEL_DEC2,
	(void *) RESERVED1,
	(void *) RESERVED2,
	(void *) FIDC_DEC,
	(void *) CA_DEC,
	(void *) RESERVED3,
	0
};

S32 Get_FIG_Init(U8 *data)
{
	fig_data.data = data;
	fig_data.byte_cnt = 0;
	fig_data.bit_cnt = 0;
	return RTV_OK;
}

S32 Get_FIG_Header(struct FIG_DATA *fig_data)
{

	Get_Bytes(1, &(fig_data->length));
	if (fig_data->length == PN_FIB_END_MARKER)
		return PN_FIB_END_MARKER;

	fig_data->type = (fig_data->length) >> 5;
	fig_data->length = (fig_data->length) & 0x1f;
	return RTV_OK;
}

S32 FIB_INIT_DEC(U8 *fib_ptr)
{
	U8 fib_cnt = 0;
	U8 fic_cmd = 1;

	while (fib_cnt < 30) {
		Get_FIG_Init(fib_ptr+fib_cnt);

		if (Get_FIG_Header(&fig_data) == PN_FIB_END_MARKER)
			return RTV_OK;
		if (fig_data.length == 0)
			return RTV_FAIL;

		((S32 (*) (U8)) FIG_PARSER[fig_data.type]) (fic_cmd);

		fib_cnt += (fig_data.length+1);
	}

	return RTV_OK;
}

S32 MCI_SI_DEC(U8 fic_cmd)
{
	U8 C_N, OE, P_D, EXT;

	Get_Bits(1, &C_N);
	Get_Bits(1, &OE);
	Get_Bits(1, &P_D);
	Get_Bits(5, &EXT);

	((S32 (*) (U8, U8, U8)) Get_FIG0_EXT[EXT]) (fic_cmd, P_D, C_N);
	return RTV_OK;
}

S32 SI_LABEL_DEC1(U8 fic_cmd)
{
	U8 Charset, OE, EXT;

	Get_Bits(4, &Charset);
	Get_Bits(1, &OE);
	Get_Bits(3, &EXT);

	((S32 (*) (U8, U8)) Get_FIG1_EXT[EXT]) (fic_cmd, Charset);
	return RTV_OK;
}

S32 SI_LABEL_DEC2(U8 fic_cmd)
{
	U8 Toggle_Flag, Seg_Index, OE, EXT;

	Get_Bits(1, &Toggle_Flag);
	Get_Bits(3, &Seg_Index);
	Get_Bits(1, &OE);
	Get_Bits(3, &EXT);

	((S32 (*) (U8, U8)) Get_FIG2_EXT[EXT]) (fic_cmd, Seg_Index);
	return RTV_OK;
}

S32 RESERVED1(U8 fic_cmd)
{
	return RTV_OK;
}

S32 RESERVED2(U8 fic_cmd)
{
	return RTV_OK;
}

S32 FIDC_DEC(U8 fic_cmd)
{
	U8 D1, D2, TCid, EXT;

	Get_Bits(1, &D1);
	Get_Bits(1, &D2);
	Get_Bits(3, &TCid);
	Get_Bits(3, &EXT);

	((S32 (*) (U8, U8, U8, U8)) Get_FIG5_EXT[EXT]) (D1, D2, fic_cmd, TCid);
	return RTV_OK;
}

S32 CA_DEC(U8 fic_cmd)
{
	U8 F_L, EXT;

	Get_Bits(2, &F_L);
	Get_Bits(6, &EXT);
	return RTV_OK;
}

S32 RESERVED3(U8 fic_cmd)
{
	return RTV_OK;
}

/*
* FIG TYPE 0 Extension Function
* Ensemble Information */
S32 Get_FIG0_EXT0(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2;

	struct FIG_TYPE0_Ext0 type0_ext0;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bytes(1, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext0.Eid = (temp1 << 8) | temp2;

		Get_Bits(2, &type0_ext0.Change_flag);
		Get_Bits(1, &type0_ext0.AI_flag);

		Get_Bits(5, &type0_ext0.CIF_Count0);
		Get_Bytes(1, &type0_ext0.CIF_Count1);

		if (type0_ext0.Change_flag != 0)
			Get_Bytes(1, &type0_ext0.Occurence_Change);

		if (fic_cmd) {
			ENS_DESC.id = type0_ext0.Eid;
			ENS_DESC.change_flag = type0_ext0.Change_flag;
			ENS_DESC.Alarm_flag = type0_ext0.AI_flag;
		}
	}

	return RTV_OK;
}

/* Basic sub-channel organization */
S32 Get_FIG0_EXT1(U8 fic_cmd, U8 P_D, U8 C_N)
{
U8 temp1, temp2, cnt;
S32 bit_rate, sub_ch_size, p_l;

struct FIG_TYPE0_Ext1 type0_ext1;

while (fig_data.byte_cnt < fig_data.length) {
	Get_Bits(6, &type0_ext1.SubChid);
	Get_Bits(2, &temp1);
	Get_Bytes(1, &temp2);
	type0_ext1.StartAdd = (temp1 << 8) | temp2;

	Get_Bits(1, &type0_ext1.S_L_form);
	if (type0_ext1.S_L_form) {
		Get_Bits(3, &type0_ext1.Option);
		Get_Bits(2, &type0_ext1.Protection_Level);
		Get_Bits(2, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext1.Sub_ch_size = (temp1 << 8) | temp2;
		type0_ext1.Size_Protection = (type0_ext1.Option << 12)
			| (type0_ext1.Protection_Level << 10)
			| type0_ext1.Sub_ch_size;
	} else {
		Get_Bits(1, &type0_ext1.Table_sw);
		Get_Bits(6, &type0_ext1.Table_index);
		type0_ext1.Size_Protection = (type0_ext1.Table_sw << 6)
			| type0_ext1.Table_index;
	}

	if (fic_cmd) {
		if (C_N) {
			GET_SUBCH_INFO(&type0_ext1, &bit_rate,
					&sub_ch_size, &p_l);
			/*Old SId & New SId matching */
			for (cnt = 0
				; cnt < NEXT_ENS_DESC.svr_comp_num
				; cnt++){

				if (NEXT_ENS_DESC.svr_comp[cnt].SubChid
					== type0_ext1.SubChid) {

					NEXT_ENS_DESC.svr_comp[cnt].START_Addr
							= type0_ext1.StartAdd;
					NEXT_ENS_DESC.svr_comp[cnt].SUB_CH_Size
							= sub_ch_size;
					NEXT_ENS_DESC.svr_comp[cnt].P_L
							= p_l;
					NEXT_ENS_DESC.svr_comp[cnt].BIT_RATE
							= bit_rate;
				}
			}
		} else {
			GET_SUBCH_INFO(&type0_ext1,
					&bit_rate,
					&sub_ch_size, &p_l);

			for (cnt = 0
				; cnt < ENS_DESC.svr_comp_num
				; cnt++) {
				/*Old SId & New SId matching */
			if (ENS_DESC.svr_comp[cnt].SubChid
				== type0_ext1.SubChid) {
				ENS_DESC.svr_comp[cnt].START_Addr
					= type0_ext1.StartAdd;
				ENS_DESC.svr_comp[cnt].SUB_CH_Size
					= sub_ch_size;
				ENS_DESC.svr_comp[cnt].P_L
					= p_l;
				ENS_DESC.svr_comp[cnt].BIT_RATE
					= bit_rate;
				}
			}
		}
	}
}

return RTV_OK;
}

static INLINE void update_next_ens_desc_type0_ext2(
				struct ENSEMBLE_DESC *next,
				struct FIG_TYPE0_Ext2 *type0_ext2,
				U8 P_D)
{
	UINT l = 0;

	if (P_D) {
		next->svr_desc[next->svr_num].P_D = 1;
		next->svr_desc[next->svr_num].ECC
			= type0_ext2->ECC;
	} else
		next->svr_desc[next->svr_num].P_D = 0;

	next->svr_desc[next->svr_num].Country_id   = type0_ext2->Country_id;
	next->svr_desc[next->svr_num].Service_ref  = type0_ext2->Service_ref;
	next->svr_desc[next->svr_num].Sid	   = type0_ext2->Sid;

	next->svr_desc[next->svr_num].Local_flag   = type0_ext2->Local_flag;
	next->svr_desc[next->svr_num].CAID		   = type0_ext2->CAID;
	next->svr_desc[next->svr_num].Num_ser_comp = type0_ext2->Num_ser_comp;

	for (l = 0; l < type0_ext2->Num_ser_comp; l++) {

		next->svr_desc[next->svr_num].ser_comp_num[l]
			= next->svr_comp_num;
		next->svr_comp[next->svr_comp_num].TMID
			= type0_ext2->svr_comp_des[l].TMID;
		/* Transport Mechanism Identifier. */
		switch (type0_ext2->svr_comp_des[l].TMID) {
		/* MSC stream Audio mode. */
		case MSC_STREAM_AUDIO:
			next->svr_comp[next->svr_comp_num].Sid
				= type0_ext2->Sid;
			next->svr_comp[next->svr_comp_num].ASCTy
				= type0_ext2->svr_comp_des[l].ASCTy;
			next->svr_comp[next->svr_comp_num].SubChid
				= type0_ext2->svr_comp_des[l].SubChid;
			next->svr_comp[next->svr_comp_num].P_S
				= type0_ext2->svr_comp_des[l].P_S;
			next->svr_comp[next->svr_comp_num].CA_flag
				= type0_ext2->svr_comp_des[l].CA_flag;
			break;
		/* MSC stream data mode. */
		case MSC_STREAM_DATA:
			next->svr_comp[next->svr_comp_num].Sid
				= type0_ext2->Sid;
			next->svr_comp[next->svr_comp_num].DSCTy
				= type0_ext2->svr_comp_des[l].DSCTy;
			next->svr_comp[next->svr_comp_num].SubChid
				= type0_ext2->svr_comp_des[l].SubChid;
			next->svr_comp[next->svr_comp_num].P_S
				= type0_ext2->svr_comp_des[l].P_S;
			next->svr_comp[next->svr_comp_num].CA_flag
				= type0_ext2->svr_comp_des[l].CA_flag;
			break;
		/* FIDC mode. */
		case FIDC:
			next->svr_comp[next->svr_comp_num].Sid
				= type0_ext2->Sid;
			next->svr_comp[next->svr_comp_num].DSCTy
				= type0_ext2->svr_comp_des[l].DSCTy;
			next->svr_comp[next->svr_comp_num].FIDCid
				= type0_ext2->svr_comp_des[l].FIDCid;
			next->svr_comp[next->svr_comp_num].P_S
				= type0_ext2->svr_comp_des[l].P_S;
			next->svr_comp[next->svr_comp_num].CA_flag
				= type0_ext2->svr_comp_des[l].CA_flag;
			break;
		/* MSC Packet data mode. */
		case MSC_PACKET_DATA:
			next->svr_comp[next->svr_comp_num].Sid
				= type0_ext2->Sid;
			next->svr_comp[next->svr_comp_num].SCid
				= type0_ext2->svr_comp_des[l].SCid;
			next->svr_comp[next->svr_comp_num].P_S
				= type0_ext2->svr_comp_des[l].P_S;
			next->svr_comp[next->svr_comp_num].CA_flag
				= type0_ext2->svr_comp_des[l].CA_flag;
			break;
		}
		next->svr_comp_num++;
	}
	next->svr_num++;

}


static INLINE void update_current_ens_desc_type0_ext2(
					struct ENSEMBLE_DESC *cur,
					struct FIG_TYPE0_Ext2 *type0_ext2,
					U8 P_D)
{
	UINT l = 0;

	if (P_D) {
		cur->svr_desc[cur->svr_num].P_D = 1;
		cur->svr_desc[cur->svr_num].ECC = type0_ext2->ECC;
	} else
		cur->svr_desc[cur->svr_num].P_D = 0;

	cur->svr_desc[cur->svr_num].Country_id	 = type0_ext2->Country_id;
	cur->svr_desc[cur->svr_num].Service_ref  = type0_ext2->Service_ref;
	cur->svr_desc[cur->svr_num].Sid  = type0_ext2->Sid;

	cur->svr_desc[cur->svr_num].Local_flag	 = type0_ext2->Local_flag;
	cur->svr_desc[cur->svr_num].CAID		 = type0_ext2->CAID;
	cur->svr_desc[cur->svr_num].Num_ser_comp = type0_ext2->Num_ser_comp;

	for (l = 0; l < type0_ext2->Num_ser_comp; l++) {
		cur->svr_desc[cur->svr_num].ser_comp_num[l] = cur->svr_comp_num;
		cur->svr_comp[cur->svr_comp_num].TMID
			= type0_ext2->svr_comp_des[l].TMID;

		switch (type0_ext2->svr_comp_des[l].TMID) {
		case MSC_STREAM_AUDIO:
			/* MSC stream Audio mode. */
			cur->svr_comp[cur->svr_comp_num].Sid
			= type0_ext2->Sid;
			cur->svr_comp[cur->svr_comp_num].ASCTy
				= type0_ext2->svr_comp_des[l].ASCTy;
			cur->svr_comp[cur->svr_comp_num].SubChid
				= type0_ext2->svr_comp_des[l].SubChid;
			cur->svr_comp[cur->svr_comp_num].P_S
				= type0_ext2->svr_comp_des[l].P_S ;
			cur->svr_comp[cur->svr_comp_num].CA_flag
				= type0_ext2->svr_comp_des[l].CA_flag;
			break;
		case MSC_STREAM_DATA:
			/* MSC stream data mode.*/
			cur->svr_comp[cur->svr_comp_num].Sid
			= type0_ext2->Sid;
			cur->svr_comp[cur->svr_comp_num].DSCTy
				= type0_ext2->svr_comp_des[l].DSCTy;
			cur->svr_comp[cur->svr_comp_num].SubChid
				= type0_ext2->svr_comp_des[l].SubChid;
			cur->svr_comp[cur->svr_comp_num].P_S
				= type0_ext2->svr_comp_des[l].P_S;
			cur->svr_comp[cur->svr_comp_num].CA_flag
				= type0_ext2->svr_comp_des[l].CA_flag;
			break;
		case FIDC:
			/* FIDC mode. */
	#if 0
			/* EWS Data have no Label in Korea */
			if (type0_ext2->svr_comp_des[l].DSCTy == 2) {
				cur->svr_comp_num--;
				cur->svr_num--;
			}
	#endif
			cur->svr_comp[cur->svr_comp_num].Sid
			= type0_ext2->Sid;
			cur->svr_comp[cur->svr_comp_num].DSCTy
				= type0_ext2->svr_comp_des[l].DSCTy;
			cur->svr_comp[cur->svr_comp_num].FIDCid
				= type0_ext2->svr_comp_des[l].FIDCid;
			cur->svr_comp[cur->svr_comp_num].P_S
				= type0_ext2->svr_comp_des[l].P_S;
			cur->svr_comp[cur->svr_comp_num].CA_flag
				= type0_ext2->svr_comp_des[l].CA_flag;
			break;
		case MSC_PACKET_DATA:
			/* MSC Packet data mode.*/
			cur->svr_comp[cur->svr_comp_num].Sid
			= type0_ext2->Sid;
			cur->svr_comp[cur->svr_comp_num].SCid
				= type0_ext2->svr_comp_des[l].SCid;
			cur->svr_comp[cur->svr_comp_num].P_S
				= type0_ext2->svr_comp_des[l].P_S;
			cur->svr_comp[cur->svr_comp_num].CA_flag
				= type0_ext2->svr_comp_des[l].CA_flag;
			break;
		}
		cur->svr_comp_num++;
	}
	cur->svr_num++;
}





/* Basic service and service component definition */
S32 Get_FIG0_EXT2(U8 fic_cmd, U8 P_D, U8 C_N)
{
U8 temp1, temp2, temp3;
UINT cnt, update_flag = 0;

struct FIG_TYPE0_Ext2 type0_ext2;

while (fig_data.byte_cnt < fig_data.length) {
	if (P_D) {
		Get_Bytes(1, &type0_ext2.ECC);
		Get_Bits(4, &type0_ext2.Country_id);
		Get_Bits(4, &temp1);
		Get_Bytes(1, &temp2);
		Get_Bytes(1, &temp3);
		type0_ext2.Service_ref = (temp1 << 16)
			| (temp2 << 8) | temp3;
		type0_ext2.Sid = (type0_ext2.ECC << 24)
			| (type0_ext2.Country_id << 20)
			| type0_ext2.Service_ref;
	} else {
		Get_Bits(4, &type0_ext2.Country_id);
		Get_Bits(4, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext2.Service_ref = (temp1 << 8) | temp2;
		type0_ext2.Sid = (type0_ext2.Country_id << 12)
			| type0_ext2.Service_ref;
	}

	Get_Bits(1, &type0_ext2.Local_flag);
	Get_Bits(3, &type0_ext2.CAID);
	Get_Bits(4, &type0_ext2.Num_ser_comp);

	for (cnt = 0; cnt < type0_ext2.Num_ser_comp; cnt++) {
		Get_Bits(2, &type0_ext2.svr_comp_des[cnt].TMID);
		switch (type0_ext2.svr_comp_des[cnt].TMID) {
		case MSC_STREAM_AUDIO:
			Get_Bits(6,
				&type0_ext2.svr_comp_des[cnt].ASCTy);
			Get_Bits(6,
				&type0_ext2.svr_comp_des[cnt].SubChid);
			Get_Bits(1,
				&type0_ext2.svr_comp_des[cnt].P_S);
			Get_Bits(1,
				&type0_ext2.svr_comp_des[cnt].CA_flag);
			break;
		case MSC_STREAM_DATA:
			Get_Bits(6,
				&type0_ext2.svr_comp_des[cnt].DSCTy);
			Get_Bits(6,
				&type0_ext2.svr_comp_des[cnt].SubChid);
			Get_Bits(1,
				&type0_ext2.svr_comp_des[cnt].P_S);
			Get_Bits(1,
				&type0_ext2.svr_comp_des[cnt].CA_flag);
			break;
		case FIDC:
			Get_Bits(6,
				&type0_ext2.svr_comp_des[cnt].DSCTy);
			Get_Bits(6,
				&type0_ext2.svr_comp_des[cnt].FIDCid);
			Get_Bits(1,
				&type0_ext2.svr_comp_des[cnt].P_S);
			Get_Bits(1,
				&type0_ext2.svr_comp_des[cnt].CA_flag);
			break;
		case MSC_PACKET_DATA:
			Get_Bits(6, &temp1);
			Get_Bits(6, &temp2);
			type0_ext2.svr_comp_des[cnt].SCid = (temp1 << 6)
				|temp2;
			Get_Bits(1, &type0_ext2.svr_comp_des[cnt].P_S);
			Get_Bits(1,
				&type0_ext2.svr_comp_des[cnt].CA_flag);
			break;
		}
	}

	if (fic_cmd) {
		if (C_N) {
			update_flag = 1;
			for (cnt = 0
				; cnt < NEXT_ENS_DESC.svr_num
				; cnt++) {
				/*Old SId & New SId matching */
				if (NEXT_ENS_DESC.svr_desc[cnt].Sid
						== type0_ext2.Sid) {
					update_flag = 0;
					break;
				}
			}

			if (update_flag) {
				update_next_ens_desc_type0_ext2(&NEXT_ENS_DESC,
					&type0_ext2, P_D);
			}
		} else {
			update_flag = 1;
			for (cnt = 0; cnt < ENS_DESC.svr_num; cnt++) {
				/*Old SId & New SId matching */
				if (ENS_DESC.svr_desc[cnt].Sid
					== type0_ext2.Sid) {
					update_flag = 0;
					break;
				}
			}

			if (update_flag) {
				update_current_ens_desc_type0_ext2(&ENS_DESC,
					&type0_ext2, P_D);
			}
		}
	}
}

return RTV_OK;
}


static INLINE void update_ens_desc_type0_ext3(
				struct ENSEMBLE_DESC *next,
				struct ENSEMBLE_DESC *cur,
				struct FIG_TYPE0_Ext3 *type0_ext3,
				U8 C_N)
{
	UINT k = 0;

	if (C_N) {
		for (k = 0; k < next->svr_comp_num; k++) {
			if ((next->svr_comp[k].TMID == MSC_PACKET_DATA) &&
				(next->svr_comp[k].SCid == type0_ext3->SCid)) {
				next->svr_comp[k].SubChid
					= type0_ext3->SubChid;
				next->svr_comp[k].Packet_add
					= type0_ext3->Packet_add;
				next->svr_comp[k].DSCTy
					= type0_ext3->DSCTy;
				next->svr_comp[k].DG_flag
					= type0_ext3->DG_flag;

				if (type0_ext3->CA_Org_flag)
					next->svr_comp[k].CA_Org
						= type0_ext3->CA_Org;
			}
		}
	} else {
		for (k = 0; k < cur->svr_comp_num; k++) {
			if ((cur->svr_comp[k].TMID == MSC_PACKET_DATA) &&
				(cur->svr_comp[k].SCid == type0_ext3->SCid)) {
				cur->svr_comp[k].SubChid
					= type0_ext3->SubChid;
				cur->svr_comp[k].Packet_add
					= type0_ext3->Packet_add;

				cur->svr_comp[k].DSCTy
					= type0_ext3->DSCTy;
				cur->svr_comp[k].DG_flag
					= type0_ext3->DG_flag;

				if (type0_ext3->CA_Org_flag)
					cur->svr_comp[k].CA_Org
					= type0_ext3->CA_Org;
			}
		}
	}
}

/* Service component in packet mode with or without Conditional Access */
S32 Get_FIG0_EXT3(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2;

	struct FIG_TYPE0_Ext3 type0_ext3;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bytes(1, &temp1);
		Get_Bits(4, &temp2);
		type0_ext3.SCid = (temp1 << 4) | temp2;

		Get_Bits(3, &temp1);
		Get_Bits(1, &type0_ext3.CA_Org_flag);
		Get_Bits(1, &type0_ext3.DG_flag);
		Get_Bits(1, &temp2);
		Get_Bits(6, &type0_ext3.DSCTy);
		Get_Bits(6, &type0_ext3.SubChid);

		Get_Bits(2, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext3.Packet_add = (temp1 << 8) | temp2;

		if (type0_ext3.CA_Org_flag) {
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			type0_ext3.CA_Org = (temp1 << 8) | temp2;
		}

		if (fic_cmd) {
			update_ens_desc_type0_ext3(&NEXT_ENS_DESC,
							&ENS_DESC, &type0_ext3,
							C_N);
		}
	}

	return RTV_OK;
}


static INLINE void update_ens_desc_type0_ext4(
				struct ENSEMBLE_DESC *next,
				struct ENSEMBLE_DESC *cur,
				struct FIG_TYPE0_Ext4 *type0_ext4,
				U8 C_N)
{
	UINT k = 0;

	if (C_N) {
		for (k = 0; k < next->svr_comp_num; k++) {
			if (type0_ext4->M_F) {
				if ((next->svr_comp[k].FIDCid
					== type0_ext4->FIDCid))
					next->svr_comp[k].CA_Org
						= type0_ext4->CA_Org;
			} else {
				if ((next->svr_comp[k].SubChid
					== type0_ext4->SubChid))
					next->svr_comp[k].CA_Org
						= type0_ext4->CA_Org;
			}
		}
	} else {
		for (k = 0; k < cur->svr_comp_num; k++) {
			if (type0_ext4->M_F) {
				if ((cur->svr_comp[k].FIDCid
					== type0_ext4->FIDCid))
					cur->svr_comp[k].CA_Org
						= type0_ext4->CA_Org;
			} else {
				if ((cur->svr_comp[k].SubChid
					== type0_ext4->SubChid))
					cur->svr_comp[k].CA_Org
						= type0_ext4->CA_Org;
			}
		}
	}

}


/* Service component with Conditional Access in stream mode or FIC */
S32 Get_FIG0_EXT4(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2;

	struct FIG_TYPE0_Ext4 type0_ext4;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bits(1, &temp1);
		Get_Bits(1, &type0_ext4.M_F);

		if (type0_ext4.M_F)
			Get_Bits(6, &type0_ext4.FIDCid);
		else
			Get_Bits(6, &type0_ext4.SubChid);

		Get_Bytes(1, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext4.CA_Org = (temp1 << 8) | temp2;

		if (fic_cmd) {
			update_ens_desc_type0_ext4(&NEXT_ENS_DESC,
						&ENS_DESC,
						&type0_ext4,
						C_N);
		}
	}

	return RTV_OK;
}

static INLINE void update_current_ens_desc_type0_ext5(
				struct ENSEMBLE_DESC *cur,
				struct FIG_TYPE0_Ext5 *type0_ext5)
{
	UINT k;

	for (k = 0; k < cur->svr_comp_num; k++) {
		if (type0_ext5->L_S_flag) {
			if (cur->svr_comp[k].SCid == type0_ext5->SCid)
				cur->svr_comp[k].language
					= type0_ext5->Language;
		} else {
			if (type0_ext5->MSC_FIC_flag) {
				if (cur->svr_comp[k].FIDCid
					== type0_ext5->FIDCid)
					cur->svr_comp[k].language
						= type0_ext5->Language;
			} else {
				if (cur->svr_comp[k].SubChid
					== type0_ext5->SubChid)
					cur->svr_comp[k].language
						= type0_ext5->Language;
			}
		}
	}
}

/* Service Component Language */
S32 Get_FIG0_EXT5(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2, temp3;

	struct FIG_TYPE0_Ext5 type0_ext5;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bits(1, &type0_ext5.L_S_flag);
		if (type0_ext5.L_S_flag) {
			Get_Bits(3, &temp1);
			Get_Bits(4, &temp2);
			Get_Bytes(1, &temp3);
			type0_ext5.SCid = (temp2 << 8) | temp3;
		} else {
			Get_Bits(1, &type0_ext5.MSC_FIC_flag);
			if (type0_ext5.MSC_FIC_flag)
				Get_Bits(6, &type0_ext5.FIDCid);
			else
				Get_Bits(6, &type0_ext5.SubChid);

		}
		Get_Bytes(1, &type0_ext5.Language);

		if (fic_cmd) {
			update_current_ens_desc_type0_ext5(&ENS_DESC,
							&type0_ext5);
		}
	}

	return RTV_OK;
}

/* Service Linking Information */
S32 Get_FIG0_EXT6(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2, temp3, temp4;
	U8 k;

	struct FIG_TYPE0_Ext6 type0_ext6;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bits(1, &type0_ext6.id_list_flag);
		Get_Bits(1, &type0_ext6.LA);
		Get_Bits(1, &type0_ext6.S_H);
		Get_Bits(1, &type0_ext6.ILS);
		Get_Bits(4, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext6.LSN = (temp1 << 8) | temp2;

		if (type0_ext6.id_list_flag) {
			if (P_D) {
				Get_Bits(4, &temp1);
				Get_Bits(4, &type0_ext6.Num_ids);

				for (k = 0; k < type0_ext6.Num_ids; k++) {
					Get_Bytes(1, &temp1);
					Get_Bytes(1, &temp2);
					Get_Bytes(1, &temp3);
					Get_Bytes(1, &temp4);
					type0_ext6.Sid[k] = (temp1 << 24)
						| (temp2 << 16) | (temp3 << 8)
						| temp4;
				}
			} else {
				Get_Bits(1, &temp1);
				Get_Bits(2, &type0_ext6.idLQ);
				Get_Bits(1, &type0_ext6.Shd);
				Get_Bits(4, &type0_ext6.Num_ids);

				for (k = 0; k < type0_ext6.Num_ids; k++) {
					if (type0_ext6.ILS)
						Get_Bytes(1,
							&type0_ext6.ECC[k]);

					Get_Bytes(1, &temp1);
					Get_Bytes(1, &temp2);
					type0_ext6.id[k] = (temp1 << 8) | temp2;
				}
			}
		}

		if (fic_cmd) {
			/* Not yet implementation
			 *
			 * */
		}
	}

	return RTV_OK;
}

S32 Get_FIG0_EXT7(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}


static INLINE void update_ens_desc_type0_ext8(
				struct ENSEMBLE_DESC *next,
				struct ENSEMBLE_DESC *cur,
				struct FIG_TYPE0_Ext8 *type0_ext8,
				U8 C_N)
{
UINT k;

if (C_N) {
	for (k = 0; k < next->svr_comp_num; k++) {
		if (type0_ext8->L_S_flag) {
			if (next->svr_comp[k].SCid
					== type0_ext8->SCid) {
				if (next->svr_comp[k].Sid
					== type0_ext8->Sid)
					next->svr_comp[k].SCidS
						= type0_ext8->SCidS;
			}
		} else {
			if (type0_ext8->MSC_FIC_flag) {
				if (next->svr_comp[k].FIDCid
					== type0_ext8->FIDCid) {
					if (next->svr_comp[k].Sid
						== type0_ext8->Sid)
						next->svr_comp[k].SCidS
							= type0_ext8->SCidS;
				}
			} else {
				if (next->svr_comp[k].SubChid
					== type0_ext8->SubChid) {
					if (next->svr_comp[k].Sid
						== type0_ext8->Sid)
						next->svr_comp[k].SCidS
						= type0_ext8->SCidS;
				}
			}
		}
	}
} else {
	for (k = 0; k < cur->svr_comp_num; k++) {
		if (type0_ext8->L_S_flag) {
			if (cur->svr_comp[k].SCid
				== type0_ext8->SCid) {
				if (cur->svr_comp[k].Sid
					== type0_ext8->Sid)
					cur->svr_comp[k].SCidS
						= type0_ext8->SCidS;
			}
		} else {
			if (type0_ext8->MSC_FIC_flag) {
				if (cur->svr_comp[k].FIDCid
					== type0_ext8->FIDCid) {
					if (cur->svr_comp[k].Sid
						== type0_ext8->Sid)
						cur->svr_comp[k].SCidS
							= type0_ext8->SCidS;
				}
			} else {
				if (cur->svr_comp[k].SubChid
					== type0_ext8->SubChid) {
					if (cur->svr_comp[k].Sid
						== type0_ext8->Sid)
						cur->svr_comp[k].SCidS
							= type0_ext8->SCidS;
				}
			}
		}
	}
}

}



/* Service component global definition*/
S32 Get_FIG0_EXT8(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2, temp3, temp4;

	struct FIG_TYPE0_Ext8 type0_ext8;

	while (fig_data.byte_cnt < fig_data.length) {
		if (P_D) {
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			Get_Bytes(1, &temp3);
			Get_Bytes(1, &temp4);
			type0_ext8.Sid = (temp1 << 24)
				| (temp2 << 16)
				| (temp3 << 8)
				| temp4;
		} else {
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			type0_ext8.Sid = (temp1 << 8) | temp2;
		}

		Get_Bits(1, &type0_ext8.Ext_flag);
		Get_Bits(3, &temp1);
		Get_Bits(4, &type0_ext8.SCidS);
		Get_Bits(1, &type0_ext8.L_S_flag);

		if (type0_ext8.L_S_flag) {
			Get_Bits(3, &temp1);
			Get_Bits(4, &temp2);
			Get_Bytes(1, &temp3);
			type0_ext8.SCid = (temp2 << 8) | temp3;
		} else {
			Get_Bits(1, &type0_ext8.MSC_FIC_flag);
			if (type0_ext8.MSC_FIC_flag)
				Get_Bits(6, &type0_ext8.FIDCid);
			else
				Get_Bits(6, &type0_ext8.SubChid);
		}

		if (type0_ext8.Ext_flag)
			Get_Bytes(1, &temp1);

		if (fic_cmd) {
			update_ens_desc_type0_ext8(&NEXT_ENS_DESC,
						&ENS_DESC,
						&type0_ext8,
						C_N);
		}
	}

	return RTV_OK;
}

/* Country, LTO and International Table */
S32 Get_FIG0_EXT9(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2, temp3, temp4;
	U8 k = 0;

	struct FIG_TYPE0_Ext9 type0_ext9;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bits(1, &type0_ext9.Ext_flag);
		Get_Bits(1, &type0_ext9.LTO_unique);
		Get_Bits(6, &type0_ext9.Ensemble_LTO);

		Get_Bytes(1, &type0_ext9.Ensemble_ECC);
		Get_Bytes(1, &type0_ext9.Inter_Table_ID);

		if (type0_ext9.Ext_flag) {
			Get_Bits(2, &type0_ext9.Num_Ser);
			Get_Bits(6, &type0_ext9.LTO);

			if (P_D) {
				for (k = 0; k < type0_ext9.Num_Ser; k++) {
					Get_Bytes(1, &temp1);
					Get_Bytes(1, &temp2);
					Get_Bytes(1, &temp3);
					Get_Bytes(1, &temp4);
					type0_ext9.Sid[k] = (temp1 << 24)
						| (temp2 << 16)
						| (temp3 << 8)
						| temp4;
				}
			} else {
				if (type0_ext9.Num_Ser != 0)
					Get_Bytes(1, &type0_ext9.ECC);

				for (k = 0; k < type0_ext9.Num_Ser; k++) {
					Get_Bytes(1, &temp1);
					Get_Bytes(1, &temp2);
					type0_ext9.Sid[k]
						= (temp1 << 8) | temp2;
				}
			}
		}

		if (fic_cmd) {
			ENS_DESC.date_time_info.LTO = type0_ext9.Ensemble_LTO;
			ENS_DESC.date_time_info.get_flag |= LTO_FLAG;
		}
	}

	return RTV_OK;
}

/* Date and Time */
S32 Get_FIG0_EXT10(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2, temp3;

	struct FIG_TYPE0_Ext10 type0_ext10;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bits(1, &temp1);
		Get_Bits(7, &temp1);
		Get_Bytes(1, &temp2);
		Get_Bits(2, &temp3);

		type0_ext10.MJD = (temp1 << 10) | (temp2 << 2) | temp3;

		Get_Bits(1, &type0_ext10.LSI);
		Get_Bits(1, &type0_ext10.Conf_ind);
		Get_Bits(1, &type0_ext10.UTC_flag);
		Get_Bits(3, &temp1);
		Get_Bits(2, &temp2);
		type0_ext10.Hours = (temp1 << 2) | temp2;
		Get_Bits(6, &type0_ext10.Minutes);

		if (type0_ext10.UTC_flag) {
			Get_Bits(6, &type0_ext10.Seconds);
			Get_Bits(2, &temp1);
			Get_Bytes(1, &temp2);
			type0_ext10.Milliseconds = (temp1 << 8) | temp2;
		}

		if (fic_cmd) {
			ENS_DESC.date_time_info.MJD      = type0_ext10.MJD;
			ENS_DESC.date_time_info.LSI      = type0_ext10.LSI;
			ENS_DESC.date_time_info.conf_ind = type0_ext10.Conf_ind;
			ENS_DESC.date_time_info.utc_flag = type0_ext10.UTC_flag;
			ENS_DESC.date_time_info.hours    = type0_ext10.Hours;
			ENS_DESC.date_time_info.minutes  = type0_ext10.Minutes;
			if (type0_ext10.UTC_flag) {
				ENS_DESC.date_time_info.seconds
					= type0_ext10.Seconds;
				ENS_DESC.date_time_info.milliseconds
					= type0_ext10.Milliseconds;
			}

			ENS_DESC.date_time_info.get_flag |= TIME_FLAG;
		}
	}

	return RTV_OK;
}

/* Region Definition */
S32 Get_FIG0_EXT11(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}
S32 Get_FIG0_EXT12(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}


static INLINE void update_ens_desc_type0_ext13(
				struct FIG_TYPE0_Ext13 *type0_ext13)
{
UINT i, k, j, p, cnt;

for (i = 0; i < ENS_DESC.svr_num; i++) {
	if (type0_ext13->Sid == ENS_DESC.svr_desc[i].Sid) {
		for (cnt = 0; cnt < ENS_DESC.svr_desc[i].Num_ser_comp; cnt++) {
			j = ENS_DESC.svr_desc[i].ser_comp_num[cnt];
			ENS_DESC.svr_comp[j].Num_User_App
				= type0_ext13->Num_User_App;
			for (k = 0; k < type0_ext13->Num_User_App; k++) {
				ENS_DESC.svr_comp[j].User_APP_Type[k]
					= type0_ext13->User_APP_Type[k];
				ENS_DESC.svr_comp[j].User_APP_data_length[k]
					= type0_ext13->User_APP_data_length[k];
				for (p = 0;
				p < type0_ext13->User_APP_data_length[k]; p++)
					ENS_DESC.svr_comp[j].User_APP_data[p]
					= type0_ext13->User_APP_data[p];
			}
		}
	}
}

}

/* User Application Information */
S32 Get_FIG0_EXT13(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2, temp3, temp4;
	UINT k, p;

	struct FIG_TYPE0_Ext13 type0_ext13;

	while (fig_data.byte_cnt < fig_data.length) {
		if (P_D) {
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			Get_Bytes(1, &temp3);
			Get_Bytes(1, &temp4);
			type0_ext13.Sid = (temp1 << 24) | (temp2 << 16)
				| (temp3 << 8) | temp4;
		} else {
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			type0_ext13.Sid = (temp1 << 8) | temp2;
		}

		Get_Bits(4, &type0_ext13.SCidS);
		Get_Bits(4, &type0_ext13.Num_User_App);

		for (k = 0; k < type0_ext13.Num_User_App; k++) {
			Get_Bytes(1, &temp1);
			Get_Bits(3, &temp2);
			type0_ext13.User_APP_Type[k] = (temp1 << 3) | temp2;

			Get_Bits(5, &type0_ext13.User_APP_data_length[k]);
/*
			if (type0_ext13.User_APP_data_length[k]> 2) {
				Get_Bits(1, &type0_ext13.CA_flag);
				Get_Bits(1, &type0_ext13.CA_Org_flag);
				Get_Bits(1, &temp1);
				Get_Bits(5, &type0_ext13.X_PAD_App_Ty);
				Get_Bits(1, &type0_ext13.DG_flag);
				Get_Bits(1, &temp2);
				Get_Bits(6, &type0_ext13.DSCTy);

				if (type0_ext13.CA_Org_flag) {
					Get_Bytes(1, &temp1);
					Get_Bytes(1, &temp2);
					type0_ext13.CA_Org
						= (temp1 << 8) | temp2;
				}
			}
*/
			for (p = 0; p < type0_ext13.User_APP_data_length[k]
				; p++)
				Get_Bytes(1, &type0_ext13.User_APP_data[p]);

		}

		if (fic_cmd)
			update_ens_desc_type0_ext13(&type0_ext13);
	}

	return RTV_OK;
}

/* FEC sub-channel organization */
S32 Get_FIG0_EXT14(U8 fic_cmd, U8 P_D, U8 C_N)
{
UINT cnt;

struct FIG_TYPE0_Ext14 type0_ext14;

while (fig_data.byte_cnt < fig_data.length) {
	Get_Bits(6, &type0_ext14.SubChid);
	Get_Bits(2, &type0_ext14.FEC_scheme);

	if (fic_cmd) {
		if (C_N) {
			for (cnt = 0;
			cnt < NEXT_ENS_DESC.svr_comp_num; cnt++) {
				if (type0_ext14.SubChid
					== NEXT_ENS_DESC.svr_comp[cnt].SubChid)
					NEXT_ENS_DESC.svr_comp[cnt].FEC_scheme
						= type0_ext14.FEC_scheme;
			}
		} else {
			for (cnt = 0; cnt < ENS_DESC.svr_comp_num; cnt++) {
				if (type0_ext14.SubChid
					== ENS_DESC.svr_comp[cnt].SubChid)
					ENS_DESC.svr_comp[cnt].FEC_scheme
					= type0_ext14.FEC_scheme;
			}
		}
	}
}

return RTV_OK;
}

S32 Get_FIG0_EXT15(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}

/* Program Number */
S32 Get_FIG0_EXT16(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2;
	/*U8 cnt;*/

	struct FIG_TYPE0_Ext16 type0_ext16;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bytes(1, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext16.Sid = (temp1 << 8) | temp2;
		Get_Bytes(1, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext16.PNum = (temp1 << 8) | temp2;
		Get_Bits(2, &temp1);
		Get_Bits(4, &temp2);
		Get_Bits(1, &type0_ext16.Continuation_flag);
		Get_Bits(1, &type0_ext16.Update_flag);
		Get_Bytes(1, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext16.New_Sid = (temp1 << 8) | temp2;
		Get_Bytes(1, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext16.New_PNum = (temp1 << 8) | temp2;
#if 0
		if (fic_cmd) {
			for (cnt = 0; cnt < ENS_DESC.svr_num; cnt++) {
				if (type0_ext16.Sid
					== ENS_DESC.svr_desc[cnt].Sid)
					/* Not yet implementation */

			}
		}
#endif
	}

	return RTV_OK;
}

/* Program Type */
S32 Get_FIG0_EXT17(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2;
	U8 cnt;

	struct FIG_TYPE0_Ext17 type0_ext17;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bytes(1, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext17.Sid = (temp1 << 8) | temp2;

		Get_Bits(1, &type0_ext17.S_D);
		Get_Bits(1, &type0_ext17.P_S);
		Get_Bits(1, &type0_ext17.L_flag);
		Get_Bits(1, &type0_ext17.CC_flag);
		Get_Bits(4, &temp1);

		if (type0_ext17.L_flag)
			Get_Bytes(1, &type0_ext17.Language);

		Get_Bits(3, &temp2);
		Get_Bits(5, &type0_ext17.Int_code);

		if (type0_ext17.CC_flag) {
			Get_Bits(3, &temp1);
			Get_Bits(5, &type0_ext17.Comp_code);
		}

		if (fic_cmd) {
			if (ENS_DESC.svr_num != 0) {
				for (cnt = 0; cnt < ENS_DESC.svr_num; cnt++) {
					if (type0_ext17.Sid
						== ENS_DESC.svr_desc[cnt].Sid)
						ENS_DESC.svr_desc[cnt].int_code
							= type0_ext17.Int_code;

				}
			}
		}
	}

	return RTV_OK;
}

/* Announcement support */
S32 Get_FIG0_EXT18(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2;
	U8 i;
	/*U8 cnt; */

	struct FIG_TYPE0_Ext18 type0_ext18;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bytes(1, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext18.Sid = (temp1 << 8) | temp2;

		Get_Bytes(1, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext18.ASU_flags = (temp1 << 8) | temp2;

		Get_Bits(3, &temp1);
		Get_Bits(5, &type0_ext18.Num_clusters);


		for (i = 0; i < type0_ext18.Num_clusters; i++)
			Get_Bytes(1, &type0_ext18.Cluster_ID[i]);

#if 0
		if (fic_cmd) {
			for (cnt = 0; cnt < ENS_DESC.svr_num; cnt++) {
				if (type0_ext18.Sid
					== ENS_DESC.svr_desc[cnt].Sid)
					/* Not yet implementation */
			}
		}
#endif
	}

	return RTV_OK;
}

/* Announcement switching */
S32 Get_FIG0_EXT19(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2;

	struct FIG_TYPE0_Ext19 type0_ext19;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bytes(1, &type0_ext19.Cluster_ID);

		Get_Bytes(1, &temp1);
		Get_Bytes(1, &temp2);
		type0_ext19.ASW_flags = (temp1 << 8) | temp2;

		Get_Bits(1, &type0_ext19.New_flag);
		Get_Bits(1, &type0_ext19.Region_flag);
		Get_Bits(6, &type0_ext19.SubChid);

		if (type0_ext19.Region_flag) {
			Get_Bits(2, &temp1);
			Get_Bits(6, &type0_ext19.Regionid_Lower_Part);
		}
#if 0
		if (fic_cmd)
			/* Not yet implementation */
#endif
	}

	return RTV_OK;
}

S32 Get_FIG0_EXT20(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}
/* Frequency Information */
S32 Get_FIG0_EXT21(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}
/* Transmitter Identification Information (TII) database */
S32 Get_FIG0_EXT22(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2;
	U8 i;

	struct FIG_TYPE0_Ext22 type0_ext22;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bits(1, &type0_ext22.M_S);
		if (type0_ext22.M_S) {
			Get_Bits(7, &type0_ext22.Mainid);
			Get_Bits(5, &temp1);
			Get_Bits(3, &type0_ext22.Num_Subid_fields);

			for (i = 0; i < type0_ext22.Num_Subid_fields; i++) {
				Get_Bits(5, &type0_ext22.Subid[i]);
				Get_Bits(3, &temp1);
				Get_Bytes(1, &temp2);
				type0_ext22.TD[i] = (temp1 << 8) | temp2;
				Get_Bytes(1, &temp1);
				Get_Bytes(1, &temp2);
				type0_ext22.Latitude_offset[i]
					= (temp1 << 8) | temp2;
				Get_Bytes(1, &temp1);
				Get_Bytes(1, &temp2);
				type0_ext22.Longitude_offset[i]
					= (temp1 << 8) | temp2;
			}
		} else {
			Get_Bits(7, &type0_ext22.Mainid);
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			type0_ext22.Latitude_coarse = (temp1 << 8) | temp2;
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			type0_ext22.Longitude_coarse = (temp1 << 8) | temp2;
			Get_Bits(4, &type0_ext22.Latitude_fine);
			Get_Bits(4, &type0_ext22.Longitude_fine);
		}
#if 0
		if (fic_cmd)
			/* Not yet implementation */
#endif
	}

	return RTV_OK;
}
S32 Get_FIG0_EXT23(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}

/* Other Ensemble Service */
S32 Get_FIG0_EXT24(U8 fic_cmd, U8 P_D, U8 C_N)
{
	U8 temp1, temp2, temp3, temp4;
	U8 i;
	/* U8 cnt;*/

	struct FIG_TYPE0_Ext24 type0_ext24;

	while (fig_data.byte_cnt < fig_data.length) {
		if (P_D) {
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			Get_Bytes(1, &temp3);
			Get_Bytes(1, &temp4);
			type0_ext24.Sid
				= (temp1 << 24)
				| (temp2 << 16) | (temp3 << 8) | temp4;
		} else {
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			type0_ext24.Sid = (temp1 << 8) | temp2;
		}

		Get_Bits(1, &temp1);
		Get_Bits(3, &type0_ext24.CAid);
		Get_Bits(4, &type0_ext24.Number_Eids);

		for (i = 0; i < type0_ext24.Number_Eids; i++) {
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			type0_ext24.Eid[i] = (temp1 << 8) | temp2;
		}
#if 0
		if (fic_cmd) {
			for (cnt = 0; cnt < ENS_DESC.svr_num; cnt++) {
				if (type0_ext24.Sid
					== ENS_DESC.svr_desc[cnt].Sid)
					/* Not yet implementation*/

			}
		}
#endif
	}

	return RTV_OK;
}

/* Other Ensemble Announcement support */
S32 Get_FIG0_EXT25(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}
/* Other Ensemble Announcement switching */
S32 Get_FIG0_EXT26(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}
/* FM Announcement support */
S32 Get_FIG0_EXT27(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}
/* FM Announcement switching */
S32 Get_FIG0_EXT28(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}
S32 Get_FIG0_EXT29(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}
S32 Get_FIG0_EXT30(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}
/* FIC re-direction */
S32 Get_FIG0_EXT31(U8 fic_cmd, U8 P_D, U8 C_N)
{
	return RTV_OK;
}


/* FIG TYPE 1 Extension Function                       */
/* Ensemble Label */
S32 Get_FIG1_EXT0(U8 fic_cmd, U8 Char_Set)
{
	U16 Eid;
	U8 label[17];
	S8 i = 0;
	U8 temp1, temp2;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bytes(1, &temp1);
		Get_Bytes(1, &temp2);
		Eid = (temp1 << 8) | temp2;

		if (fic_cmd) {
			Get_Bytes(16, label);

			if (!ENS_DESC.label_flag) {
				for (i = 15; i >= 0; i--) {
					if (label[i] != 0x20) {
						label[i+1] = '\0';
						break;
					}
				}
				label[16] = '\0';

				ENS_DESC.charset = Char_Set;
				memcpy(ENS_DESC.Label, label, 17);
				ENS_DESC.label_flag = 1;
			}
		}
	}

	return RTV_OK;
}

/* Program Service Label */
S32 Get_FIG1_EXT1(U8 fic_cmd, U8 Char_Set)
{
U32 sid;
U8 label[17];
U8 cnt;
S8 i;
U8 temp1, temp2;

while (fig_data.byte_cnt < fig_data.length) {
	Get_Bytes(1, &temp1);
	Get_Bytes(1, &temp2);
	sid = (temp1 << 8) | temp2;

	if (fic_cmd) {
		Get_Bytes(16, label);

		for (cnt = 0; cnt < ENS_DESC.svr_num; cnt++) {
			if (sid == ENS_DESC.svr_desc[cnt].Sid) {
				if (!ENS_DESC.svr_desc[cnt].label_flag) {
					for (i = 15; i >= 0; i--) {
						if (label[i] != 0x20) {
							label[i+1] = '\0';
							break;
						}
					}
					label[16] = '\0';

					ENS_DESC.svr_desc[cnt].charset
						= Char_Set;
					memcpy(ENS_DESC.svr_desc[cnt].Label,
						label, 17);
					ENS_DESC.svr_desc[cnt].label_flag = 1;
					ENS_DESC.label_num++;
				}
			}
		}
	}
}

return RTV_OK;
}

S32 Get_FIG1_EXT2(U8 fic_cmd, U8 Char_Set)
{
	return RTV_OK;
}

/* Region Label */
S32 Get_FIG1_EXT3(U8 fic_cmd, U8 Char_Set)
{
	U8 temp1;
	U8 RegionId_Lower_part;
	U8 label[17];

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bits(2, &temp1);
		Get_Bits(6, &RegionId_Lower_part);

		if (fic_cmd)
			Get_Bytes(16, label);
	}

	return RTV_OK;
}

/* Service Component Label */
S32 Get_FIG1_EXT4(U8 fic_cmd, U8 Char_Set)
{
	U8 P_D;
	U8 SCidS;
	U32 sid;
	U8 label[17];
	S8 i = 0, k = 0;
	U8 temp1, temp2, temp3, temp4;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bits(1, &P_D);
		Get_Bits(3, &temp1);
		Get_Bits(4, &SCidS);

		if (P_D) {
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			Get_Bytes(1, &temp3);
			Get_Bytes(1, &temp4);
			sid = (temp1 << 24)
				| (temp2 << 16) | (temp3 << 8) | temp4;
		} else {
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			sid = (temp1 << 8) | temp2;
		}

		if (fic_cmd) {
			Get_Bytes(16, label);

			for (k = 0; k < ENS_DESC.svr_comp_num; k++) {
				if ((ENS_DESC.svr_comp[k].Sid == sid)
					&& (ENS_DESC.svr_comp[k].SCidS
						== SCidS)) {
					for (i = 15; i >= 0; i--) {
						if (label[i] != 0x20) {
							label[i+1] = '\0';
							break;
						}
					}
					label[16] = '\0';

					ENS_DESC.svr_comp[k].charset
						= Char_Set;
					memcpy(ENS_DESC.svr_comp[k].Label,
						label, 17);
					break;
				}
			}
		}
	}

	return RTV_OK;
}

/* Data Service Label */
S32 Get_FIG1_EXT5(U8 fic_cmd, U8 Char_Set)
{
U32 sid;
U8 label[17];
U8 cnt;
S8 i;
U8 temp1, temp2, temp3, temp4;

while (fig_data.byte_cnt < fig_data.length) {
	Get_Bytes(1, &temp1);
	Get_Bytes(1, &temp2);
	Get_Bytes(1, &temp3);
	Get_Bytes(1, &temp4);
	sid = (temp1 << 24) | (temp2 << 16) | (temp3 << 8) | temp4;

	if (fic_cmd) {
		Get_Bytes(16, label);

		for (cnt = 0; cnt < ENS_DESC.svr_num; cnt++) {
			if (sid == ENS_DESC.svr_desc[cnt].Sid) {
				if (!ENS_DESC.svr_desc[cnt].label_flag) {
					for (i = 15; i >= 0; i--) {
						if (label[i] != 0x20) {
							label[i+1] = '\0';
							break;
						}
					}
					label[16] = '\0';

					ENS_DESC.svr_desc[cnt].charset
						= Char_Set;
					memcpy(ENS_DESC.svr_desc[cnt].Label,
						label, 17);
					ENS_DESC.svr_desc[cnt].label_flag = 1;
					ENS_DESC.label_num++;
				}
			}
		}
	}
}

return RTV_OK;
}

/* X-PAD user application label */
S32 Get_FIG1_EXT6(U8 fic_cmd, U8 Char_Set)
{
	U8 P_D;
	U8 SCidS;
	U32 sid;
	U8 X_PAD_app_type;
	U8 label[17];
	U8 temp1, temp2, temp3, temp4;

	while (fig_data.byte_cnt < fig_data.length) {
		Get_Bits(1, &P_D);
		Get_Bits(3, &temp1);
		Get_Bits(4, &SCidS);

		if (P_D) {
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			Get_Bytes(1, &temp3);
			Get_Bytes(1, &temp4);
			sid = (temp1 << 24)
				| (temp2 << 16) | (temp3 << 8) | temp4;
		} else {
			Get_Bytes(1, &temp1);
			Get_Bytes(1, &temp2);
			sid = (temp1 << 8) | temp2;
		}

		Get_Bits(2, &temp1);
		Get_Bits(1, &temp2);
		Get_Bits(5, &X_PAD_app_type);

		if (fic_cmd)
			Get_Bytes(16, label);
	}

	return RTV_OK;
}

S32 Get_FIG1_EXT7(U8 fic_cmd, U8 Char_Set)
{
	return RTV_OK;
}


/* FIG TYPE 2 Extension Function                       */
/* Ensemble Label */
S32 Get_FIG2_EXT0(U8 fic_cmd, U8 Seg_Index)
{
	return RTV_OK;
}

/* Program Service Label */
S32 Get_FIG2_EXT1(U8 fic_cmd, U8 Seg_Index)
{
	return RTV_OK;
}

S32 Get_FIG2_EXT2(U8 fic_cmd, U8 Seg_Index)
{
	return RTV_OK;
}

/* Region Label */
S32 Get_FIG2_EXT3(U8 fic_cmd, U8 Seg_Index)
{
	return RTV_OK;
}

/* Service Component Label */
S32 Get_FIG2_EXT4(U8 fic_cmd, U8 Seg_Index)
{
	return RTV_OK;
}

/* Data Service Label */
S32 Get_FIG2_EXT5(U8 fic_cmd, U8 Seg_Index)
{
	return RTV_OK;
}

/* X-PAD user application label */
S32 Get_FIG2_EXT6(U8 fic_cmd, U8 Seg_Index)
{
	return RTV_OK;
}

/* Character Definition */
S32 Get_FIG2_EXT7(U8 fic_cmd, U8 Seg_Index)
{
	return RTV_OK;
}


/* FIG TYPE 5 Extension Function                       */
/* Paging */
S32 Get_FIG5_EXT0(U8 D1, U8 D2, U8 fic_cmd, U8 TCid)
{
	U8 cnt = 0;
	U8 FIDC_ID;

	FIDC_ID = (TCid << 3) | 0x00;

	if (fic_cmd) {
		for (cnt = 0; cnt < ENS_DESC.svr_comp_num; cnt++) {
			if (FIDC_ID == ENS_DESC.svr_comp[cnt].FIDCid) {
				ENS_DESC.svr_comp[cnt].TCid = TCid;
				ENS_DESC.svr_comp[cnt].Ext = 0;
			}
		}
	}

	return RTV_OK;
}

/* Traffic Message Channel (TMC) */
S32 Get_FIG5_EXT1(U8 D1, U8 D2, U8 fic_cmd, U8 TCid)
{
	U8 cnt = 0;
	U8 FIDC_ID;

	FIDC_ID = (TCid << 3) | 0x01;

	if (fic_cmd) {
		for (cnt = 0; cnt < ENS_DESC.svr_comp_num; cnt++) {
			if (FIDC_ID == ENS_DESC.svr_comp[cnt].FIDCid) {
				ENS_DESC.svr_comp[cnt].TCid = TCid;
				ENS_DESC.svr_comp[cnt].Ext = 1;
			}
		}
	}

	return RTV_OK;
}

/* Emergency Warning System (EWS) */
S32 Get_FIG5_EXT2(U8 D1, U8 D2, U8 fic_cmd, U8 TCid)
{
	U8 cnt = 0;
	U8 FIDC_ID;

	FIDC_ID = (TCid << 3) | 0x02;

	if (fic_cmd) {
		for (cnt = 0; cnt < ENS_DESC.svr_comp_num; cnt++) {
			if (FIDC_ID == ENS_DESC.svr_comp[cnt].FIDCid) {
				ENS_DESC.svr_comp[cnt].TCid = TCid;
				ENS_DESC.svr_comp[cnt].Ext = 2;
			}
		}
	}

	return RTV_OK;
}


U8 GET_SUBCH_INFO(struct FIG_TYPE0_Ext1 *type0_ext1,
	S32 *BIT_RATE, S32 *SUB_CH_Size, S32 *P_L)
{
/*Indicate the option used for the long form coding(Equal Error Protection)*/
	if (type0_ext1->S_L_form) {
		*SUB_CH_Size = type0_ext1->Sub_ch_size;
		*P_L = ((type0_ext1->S_L_form<<7)
			| (type0_ext1->Option<<6)
			| type0_ext1->Protection_Level);

		if (type0_ext1->Option == 0x1) {
			switch (type0_ext1->Protection_Level) {
			case 0:
				*BIT_RATE = (type0_ext1->Sub_ch_size)*32/27;
				break;
			case 1:
				*BIT_RATE = (type0_ext1->Sub_ch_size)*32/21;
				break;
			case 2:
				*BIT_RATE = (type0_ext1->Sub_ch_size)*32/18;
				break;
			case 3:
				*BIT_RATE = (type0_ext1->Sub_ch_size)*32/15;
				break;
			}
		} else if (type0_ext1->Option == 0x0) {
			switch (type0_ext1->Protection_Level) {
			case 0:
				*BIT_RATE = (type0_ext1->Sub_ch_size)*2/3;
				break;
			case 1:
				*BIT_RATE = (type0_ext1->Sub_ch_size);
				break;
			case 2:
				*BIT_RATE = (type0_ext1->Sub_ch_size)*4/3;
				break;
			case 3:
				*BIT_RATE = (type0_ext1->Sub_ch_size)*2;
				break;
			}
		}
	} else {
		*SUB_CH_Size = SUBCH_UEP_TABLE[type0_ext1->Table_index][0];
		*P_L         = SUBCH_UEP_TABLE[type0_ext1->Table_index][1];
		*BIT_RATE    = SUBCH_UEP_TABLE[type0_ext1->Table_index][2];
	}

	return RTV_OK;
}

U8 GET_DATE_TIME(struct DATE_TIME_INFO *time_desc)
{
	U16 MJD_Ref[2] = {2000, 51544};   /*2000.01.01 reference day*/
	U8 month0_table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	U8 month1_table[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	U16 UTC_hours, MJD_temp, day_temp;
	U8 increase_year, leap_year_num;
	U8 normal_year_num, offset_flag, offset_value;
	U16 MJDRef_year;
	U16 MJDRef_month;
	U16 MJDRef_day = 0;
	U8 week;
	U8 k = 0, i = 0;

/* sense of the Local Time Offset (0: positive offset 1: negative offset)*/
	offset_flag = (ENS_DESC.date_time_info.LTO & 0x20) >> 5;

	/* Local time offset value (0 ~ 23)*/
	offset_value = (ENS_DESC.date_time_info.LTO & 0x1f) / 2;

	/* UTC hours */
	UTC_hours = ENS_DESC.date_time_info.hours;

	if (offset_flag) {
		if (UTC_hours < offset_value)
			time_desc->time_flag = 0;
		else
			time_desc->time_flag = 1;
	} else {
		if (((23-offset_value) < UTC_hours) && (UTC_hours < 24))
			time_desc->time_flag = 0;
		else
			time_desc->time_flag = 1;
	}

	/* current MJD - ref MJD */
	MJD_temp = ENS_DESC.date_time_info.MJD - MJD_Ref[1];
	increase_year = MJD_temp / 365; /* 2000 + x year */

	/* detection 2000 + x year */
	time_desc->years = MJD_Ref[0] + increase_year;
	leap_year_num = (increase_year - 1) / 4; /* 366 year number */
	normal_year_num = (increase_year - 1) % 4;/* 365 year number */

	/* first MJD for current year */
	MJDRef_year
		= MJD_Ref[1] + 366 * (leap_year_num + 1)
		+ 365 * ((3 * leap_year_num) + normal_year_num);


	if (time_desc->time_flag)
		MJDRef_month = ENS_DESC.date_time_info.MJD - MJDRef_year;
	else {
		if (offset_flag)
			MJDRef_month
				= ENS_DESC.date_time_info.MJD
					- MJDRef_year - 1;
		else
			MJDRef_month = ENS_DESC.date_time_info.MJD
					- MJDRef_year + 1;
	}

	for (k = 0; k < 12; k++) {
		day_temp = MJDRef_month - MJDRef_day;

		if (normal_year_num == 3) {
			if (day_temp >= month1_table[k])
				MJDRef_day += month1_table[k];
			else {
				/* detection month */
				time_desc->months_dec = k + 1;
				strcpy((char *)time_desc->months_ste,
					MONTH_TABLE[k]);

				/* detection day */
				time_desc->days = day_temp + 1;
				break;
			}
		} else {
			if (day_temp >= month0_table[k])
				MJDRef_day += month0_table[k];
			else {
				/* detection month */
				time_desc->months_dec = k + 1;
				strcpy((char *)time_desc->months_ste,
					MONTH_TABLE[k]);
				/* detection day */
				time_desc->days = day_temp + 1;
				break;
			}
		}
	}

	week = MJD_temp % 7;
	for (i = 0; i < 7; i++) {
		if (i == week) {
			if (time_desc->time_flag) {
				strcpy((char *)time_desc->weeks,
					WEEK_TABLE[i]);
			} else {
				if (offset_flag)
					strcpy((char *)time_desc->weeks,
					WEEK_TABLE[i-1]);
				else
					strcpy((char *)time_desc->weeks,
					WEEK_TABLE[i+1]);
			}
			break;
		}
	}

	if (ENS_DESC.date_time_info.utc_flag) {
		if (time_desc->time_flag) {
			time_desc->hours
				= ENS_DESC.date_time_info.hours + offset_value;
			if (time_desc->hours < 12)
				time_desc->apm_flag = 0;
			else if (time_desc->hours == 12)
				time_desc->apm_flag = 1;
			else {
				time_desc->hours = time_desc->hours - 12;
				time_desc->apm_flag = 1;
			}
		} else {
			if (offset_flag) {
				time_desc->hours
					= (ENS_DESC.date_time_info.hours + 12)
						- offset_value;
				time_desc->apm_flag = 1;
			} else {
				time_desc->hours
					= (ENS_DESC.date_time_info.hours
						+ offset_value) - 24;
				time_desc->apm_flag = 0;
			}
		}

		time_desc->minutes = ENS_DESC.date_time_info.minutes;
		time_desc->seconds = ENS_DESC.date_time_info.seconds;
		time_desc->milliseconds = ENS_DESC.date_time_info.milliseconds;
	} else {
		if (time_desc->time_flag) {
			time_desc->hours
				= ENS_DESC.date_time_info.hours + offset_value;
			if (time_desc->hours < 12)
				time_desc->apm_flag = 0;
			else if (time_desc->hours == 12)
				time_desc->apm_flag = 1;
			else {
				time_desc->hours = time_desc->hours - 12;
				time_desc->apm_flag = 1;
			}
		} else {
			if (offset_flag) {
				time_desc->hours
					= (ENS_DESC.date_time_info.hours + 12)
					- offset_value;
				time_desc->apm_flag = 1;
			} else {
				time_desc->hours
					= (ENS_DESC.date_time_info.hours
					+ offset_value) - 24;
				time_desc->apm_flag = 0;
			}
		}

		time_desc->minutes = ENS_DESC.date_time_info.minutes;
	}

	return RTV_OK;
}


char *PROGRAM_TYPE_CODE16[32] = {
	"None", "News", "Current_Affairs", "Information", "Sport",
	"Education", "Drama", "Arts", "Science", "Talk",
	"Pop_Music", "Rock_Music", "Easy_Listening",
	"Light_Classical", "Classical_Music",
	"Other_Music", "Weather", "Finance", "Children's", "Factual",
	"Religion", "Phone_In", "Travel", "Leisure", "Jazz_and_Blues",
	"Country_Music", "National_Music",
	"Oldies_Music", "Folk_Music", "Documentary"
};

char *PROGRAM_TYPE_CODE8[32] = {
	"None", "News", "Affairs", "Info", "Sport",
	"Educate", "Drama", "Arts", "Science", "Talk",
	"Pop", "Rock", "Easy", "Classics", "Classics",
	"Other_M", "Weather", "Finance", "Children", "Factual",
	"Religion", "Phone_In", "Travel", "Leisure", "Jazz",
	"Country", "Nation_M", "Oldies", "Folk", "Document"
};

char *USER_APP_TYPE_CODE[11] = {
	"Reserved", "Not used", "MOT Slideshow", "MOT BWS", "TPEG",
	"DGPS", "TMC", "EPG", "DAB Java", "DMB", "Reserved"
};

char *FIDC_EXT_CODE[3] = {
	"Paging", "Traffic Message(TMC)", "Emergency Warning(EWS)"
};

char *ASCTy[3] = {
	"Foreground Sound", "Background Sound", "Multi-CH Audio"
};

char *DSCTy[11] = {
	"Unspecified Data", "Traffic Message(TMC)", "Emergency Warning(EWS)",
		"ITTS", "Paging", "TDC",
	"KDMB", "Embedded IP", "MOT", "Proprietary Service", "Reserved"
};

char *ANNOUNCEMENT_TYPE_CODE[12] = {
	"Alarm", "Road Traffic flash", "Transport flash", "Warning/Service",
	"News flash", "Area weather flash",
	"Event announcement", "Special event", "Programme information",
	"Sport report", "Financial report",
	"Reserved for future definition"
};

int SUBCH_SIZE_TABLE[64] = { 32, 32, 32, 32, 32, 48, 48, 48, 48, 48,
			   56, 56, 56, 56, 64, 64, 64, 64, 64, 80,
			   80, 80, 80, 80, 96, 96, 96, 96, 96, 112,
			   112, 112, 112, 128, 128, 128, 128, 128, 160, 160,
			   160, 160, 160, 192, 192, 192, 192, 192, 224, 224,
			   224, 224, 224, 256, 256, 256, 256, 256, 320, 320,
			  320, 384, 384, 384};

int SUBCH_UEP_TABLE[64][3] = {
	/* 0 {Sub-channel size, Protection level, Bit rate} */
	{16, 5, 32},	{21, 4, 32},	{24, 3, 32},	{29, 2, 32},
	{35, 1, 32},	{24, 5, 48},	{29, 4, 48},	{35, 3, 48}, /*4 */
	{42, 4, 48},	{52, 1, 48},	{29, 5, 56},	{35, 4, 56}, /* 8 */
	{42, 3, 56},	{52, 2, 56},	{32, 5, 64},	{42, 4, 64}, /* 12 */
	{48, 3, 64},	{58, 2, 64},	{70, 1, 64},	{40, 5, 80}, /* 16 */
	{52, 4, 80},	{58, 3, 80},	{70, 2, 80},	{84, 1, 80}, /* 20 */
	{48, 5, 96},	{58, 4, 96},	{70, 3, 96},	{84, 2, 96}, /* 24 */
	{104, 1, 96},	{58, 5, 112},	{70, 4, 112},	{84, 3, 112}, /* 28 */
	{104, 2, 112},	{64, 5, 128},	{84, 4, 128},	{96, 3, 128}, /* 32 */
	{116, 2, 128},	{140, 1, 128},	{80, 5, 160},	{104, 4, 160},/* 36 */
	{116, 3, 160},	{140, 2, 160},	{168, 1, 160},	{96, 5, 192}, /* 40 */
	{116, 4, 192},	{140, 3, 192},	{168, 2, 192},	{208, 1, 192}, /* 44 */
	{116, 5, 224},	{140, 4, 224},	{168, 3, 224},	{208, 2, 224}, /* 48 */
	{232, 1, 224},	{128, 5, 256},	{168, 4, 256},	{192, 3, 256}, /* 52 */
	{232, 2, 256},	{280, 1, 256},	{160, 5, 320},	{208, 4, 320}, /* 56 */
	{280, 2, 320},	{192, 5, 384},	{280, 3, 384},	{416, 1, 384}, /* 60 */
};

char *MONTH_TABLE[12] = {
	"Jan",	"Feb",	"Mar",	"Apr",	"May",	"Jun",
	"Jul",	"Aug",	"Sep",	"Oct",	"Nov",	"Dec"
};

char *WEEK_TABLE[8] = {
	"SAT",	"SUN",	"MON",	"TUE",	"WED",	"THU",	"FRI", "SAT"
};
/*
int MJD_TABLE[][] ={
	{53370, 1, 2005},	{53735, 1, 2006},
	{54100, 1, 2007},	{54465, 1, 2008},
	{53401, 2, 2005},	{53766, 2, 2006},
	{54131, 2, 2007},	{54496, 2, 2008},
	{53429, 3, 2005},	{53794, 3, 2006},
	{54159, 3, 2007},	{54525, 3, 2008},
	{53460, 4, 2005},	{53825, 4, 2006},
	{54190, 4, 2007},	{54556, 4, 2008},
	{53490, 5, 2005},	{53855, 5, 2006},
	{54220, 5, 2007},	{54586, 5, 2008},
	{53521, 6, 2005},	{53866, 6, 2006},
	{54251, 6, 2007},	{54617, 6, 2008},
	{53551, 7, 2005},	{53916, 7, 2006},
	{54281, 7, 2007},	{54647, 7, 2008},
	{53582, 8, 2005},	{53947, 8, 2006},
	{54312, 8, 2007},	{54678, 8, 2008},
	{53613, 9, 2005},	{53978, 9, 2006},
	{54343, 9, 2007},	{54709, 9, 2008},
	{53643, 10, 2005},	{54008, 10, 2006},
	{54373, 10, 2007},	{54739, 10, 2008},
	{53674, 11, 2005},	{54039, 11, 2006},
	{54404, 11, 2007},	{54770, 11, 2008},
	{53704, 12, 2005},	{54069, 12, 2006},
	{54434, 12, 2007},	{54800, 12, 2008},
};
*/

char *EWS_PRIORITY_TABLE[4] = {
	"Unknown", "보통", "긴급", "매우긴급"
};

char *EWS_REGION_FORM_TABLE[4] = {
	"대한민국 전국",
	"대한민국 정부 지정",
	"행자부 행정동 표기",
	"Rfa"
};

char *EWS_OFFICIAL_ORGANIZATION_TABLE[4] = {
	"소방방재청", "시,도", "군,도", "Rfa"
};

char *EWS_CATEGORY[67][3] = {
	{"호우 주의보",              "HRA", "Heavy Rain Watch"},
	{"호우 경보",                "HRW", "Heavy Rain Warning"},
	{"대설 주의보",              "HSW", "Heavy Snow Watch"},
	{"대설 경보",                "HAS", "Heavy Snow Warning"},
	{"폭풍해일주의보",           "SSA", "Storm Surge Watch"},
	{"폭풍해일 경보",            "SSW", "Storm Surge Warning"},
	{"황사 경보",                "YSW", "Yellow Sand Warning"},
	{"한파 주의보",              "CWA", "Cold Wave Watch"},
	{"한파 경보",                "CWW", "Cold Wave Warning"},
	{"풍랑 경보",                "WWW", "Wind and Waves Warning"},
	{"건조 경보",                "HAW", "Heavy Arid Warning"},
	{"산불 경보",                "MFW", "Mountain Fire Warning"},
	{"교통 통제",                "RTW", "Regulate Traffic Warning"},
	{"국가 비상 상황 발생",
	"EAN", "Emergency Action Notification(National only)"},
	{"국가 비상 상황 종료",
	"EAT", "Emergency Action Termination(National only)"},
	{"중앙 재난 안전 대책 본부",
	"NIC", "National Information Center"},
	{"전국적 주기 테스트",       "NPT", "National Periodic Test"},
	{"전국적 월별 의무 테스트",  "RMT", "Required Monthly Test"},
	{"전국적 주간별 의무 테스트", "RWT", "Required Weekly Test"},
	{"특수 수신기 테스트",       "STT", "Special Terminal Test"},
	{"행정 메시지",              "ADR", "Administrative Message"},
	{"산사태 경보",              "AVW", "Avalanche Warning"},
	{"산사태 주의보",            "AVA", "Avalanche Watch"},
	{"폭풍설경보",               "BZW", "Blizzard Warning"},
	{"어린이 유괴 긴급 상황",
	"CAE", "Child Abduction Emergency"},
	{"시민 위험 상황 경보",      "CDW", "Civil Danger Warning"},
	{"시민 응급 상황 메시지",    "CEM", "Civil Emergency Message"},
	{"해안 침수 경보",           "CFW", "Coastal Flood Warning"},
	{"해안 침수 주의보",         "CFA", "Coastal Flood Watch"},
	{"모래 폭풍 경보",           "DSW", "Dust Storm Warning"},
	{"지진 경보",                "EQW", "Earthquake Warning"},
	{"즉시 대피",                "EVI", "Evacuation Immediate"},
	{"화재 경보",                "FRW", "Fire Warning"},
	{"긴급 홍수 경보",           "FFW", "Flash Flood Warning"},
	{"긴급 홍수 주의보",         "FFA", "Flash Flood Watch"},
	{"긴급 홍수 상황",           "FFS", "Flash Flood Statement"},
	{"홍수 경보",                "FLW", "Flood Warning"},
	{"홍수 주의보",              "FLA", "Flood Watch"},
	{"홍수 상황",                "FLS", "Flood Statement"},
	{"위험 물질 경보",
	"HMW", "Hazardous Materials Warning"},
	{"강풍 경보",                "HWW", "High Wind Warning"},
	{"강풍 주의보",              "HWA", "High Wind Watch"},
	{"태풍 경보",                "HUW", "Hurricane Warning"},
	{"태풍 주의보",              "HUA", "Hurricane Watch"},
	{"태풍정보",                 "HLS", "Hurricane Statement"},
	{"법집행 경고",              "LEW", "Law Enforcement Warning"},
	{"지역 긴급 상황",           "LAE", "Local Area Emergency"},
	{"통신 메지시 알림",
	"NMN", "Network Message Notification"},
	{"119 전화 불통 응급 상황",
	"TOE", "119 Telephone Outage Emergency"},
	{"핵발전소 관련 경보",
	"NUW", "Nuclear Power Plant Warning"},
	{"실제/연습 경보",           "DMO", "Practice/Demo Warning"},
	{"방사능 위험 경보",
	"RHW", "Radiological Hazard Warning"},
	{"뇌우 경보",                "SVR", "Severe Thunderstorm Warning"},
	{"뇌우 주의보",              "SVA", "Severe Thunderstorm Watch"},
	{"악기상정보",               "SVS", "Severe Weather Statement"},
	{"안전한 장소로 피난 경보",
	"SPW", "Shelter in Place Warning"},
	{"특수 해양 경보",           "SMW", "Special Marine Warning"},
	{"특이 기상 정보",           "SPS", "Special Weather Statement"},
	{"토네이도 경보",            "TOR", "Tornado Warning"},
	{"토네이도 주의보",          "TOA", "Tornado Watch"},
	{"열대 폭풍(태풍) 경보",     "TRW", "Tropical Storm Warning"},
	{"열대 폭풍(태풍) 주의보",   "TRA", "Tropical Storm Watch"},
	{"지진해일 경보",            "TSW", "Tsunami Warning"},
	{"지진해일 주의보",          "TSA", "Tsunami Watch"},
	{"화산 경보",                "VOW", "Volcano Warning"},
	{"눈폭풍 경보",              "WSW", "Winter Storm Warning"},
	{"눈폭풍 주의보",            "WSA", "Winter Storm Watch"}
};

static const U16 crc_ccitt_tab[] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

static S32 CRC_CHECK(U8 *data, U16 data_len)
{

	U16 crc = 0xffff;
	U16 crc2 = 0xffff;
	U16 crc_val, i;
	U8  crc_cal_data;

	for (i = 0; i < (data_len - 2); i++) {
		crc_cal_data = *(data+i);
		crc = (crc<<8)^crc_ccitt_tab[(crc>>8)^(crc_cal_data)++];
	}

	crc_val = *(data+i)<<8;
	crc_val = crc_val | *(data+i+1);

	crc2 = (crc_val^crc2);

	if (crc == crc2)
		return RTV_OK;
	else
		return RTV_FAIL;
}


/*****************************/
/* FIC Information Variable  */
/*****************************/
struct ENSEMBLE_DESC  ENS_DESC, NEXT_ENS_DESC;
U32 FIC_CONUT;


static UINT fib_crc_err_cnt;
S32 FIC_Init_Dec(U8 *fic, U8 fib_num, U8 CN)
{
	U32 i;
	U32 ret;
	unsigned int fib_crc_pos = 0;
	FIC_CONUT++;

	for (i = 0; i < fib_num; i++) {
		if (CRC_CHECK(fic+fib_crc_pos, 32) != RTV_OK) {
			fib_crc_pos += 32;
			fib_crc_err_cnt++;
			return FIC_CRC_ERR;
		}

		ret = FIB_INIT_DEC(fic+fib_crc_pos);

		if (ENS_DESC.svr_num) {
			if ((ENS_DESC.svr_num == ENS_DESC.label_num)
				&&/* (FIC_CONUT > 5)
				&&*/ (ENS_DESC.label_flag == 1)) {
				FIC_CONUT = 0;
				return FIC_DONE;
			}
		}
	    fib_crc_pos += 32;
	}
	return FIC_GOING;
}

#ifdef USER_APPLICATION_TYPE
static UINT rtvTDMB_SetVisualRadioInfo(SubChInfoType *subChInfo)
{
	UINT i = 0, j = 0;

	subChInfo->bVisualRadio = 0;

	/* UA Type - TV, Visual Radio : 0x009 */
	for (i = 0; i < subChInfo->NumberofUserAppl; i++) {
		if (subChInfo->UserApplType[i] == 0x009) {
			for (j = 0; j < subChInfo->UserApplLength[i]; j++) {
				if ((subChInfo->UserApplData[j][0] == 0x01)
					&& (subChInfo->UserApplData[j][1]
						== 0x02)) {
					subChInfo->bVisualRadio = 1;
					return 1;
				}
			}
		}
	}
	return 0;
}
#endif


static BOOL fic_decode_run;


BOOL rtvFICDEC_Decode(U32 ch_freq_khz)
{
	UINT ret;
	UINT cnt = 0;
	U8 int_type_val1;
	U8 fic_buf[384+1];
	/*UINT crc_err_cnt = 0; */
	UINT read_cnt = 0;
	unsigned long diff_jiffies = get_jiffies_64();
	unsigned long diff_jiffies_1;

	fic_decode_run = TRUE;
	fib_crc_err_cnt = 0;

	FIC_CONUT = 0;
	memset(&ENS_DESC, 0, sizeof(struct ENSEMBLE_DESC));

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(INT_E_UCLRL, 0x01);

	while (++cnt <= 408) {
		if (fic_decode_run == FALSE)
			break;

		int_type_val1 = RTV_REG_GET(INT_E_STATL);

	/* RTV_DBGMSG0("FIC_Check rootine!\n");*/
		if (int_type_val1 & FIC_E_INT) {
			read_cnt++;

			RTV_REG_MAP_SEL(FIC_PAGE);
			RTV_REG_BURST_GET(0x10, fic_buf, 384+1);

			/* FIC interrupt status clear */
			RTV_REG_MAP_SEL(DD_PAGE);
			RTV_REG_SET(INT_E_UCLRL, 0x01);

			ret = FIC_Init_Dec(&fic_buf[1], 12, 0);
			if (ret == FIC_DONE) {
				diff_jiffies_1 = get_jiffies_64();
				RTV_DBGMSG2\
				("[mtv] CNT: %u, FICread time ms (%u)\n",
				read_cnt,
				jiffies_to_msecs(diff_jiffies_1-diff_jiffies));

			/*RTV_DBGMSG1\
			("[mtv] FIC_Parseing DONE time ms (%u)\n\n",
			jiffies_to_msecs(get_jiffies_64() - diff_jiffies_1));*/

				RTV_GUARD_FREE;

			   return TRUE;
			}
			/* added 2011/12/27*/
			else if (ret == FIC_CRC_ERR) {
				RTV_GUARD_FREE;
				return FALSE;
			}
		}

		RTV_DELAY_MS(12);
	}

	RTV_GUARD_FREE;

	diff_jiffies_1 = get_jiffies_64();
	RTV_DBGMSG2("[mtv] CNT: %u, FIC_read time ms (%u)\n\n",
		read_cnt, jiffies_to_msecs(diff_jiffies_1 - diff_jiffies));


	return FALSE;
}


void rtvFICDEC_Stop(void)
{
	rtvTDMB_CloseFIC();

	/* Set the flag.*/
	fic_decode_run = FALSE;
}

struct ensemble_info_type *rtvFICDEC_GetEnsembleInfo(unsigned long freq)
{
	UINT i, j;
	UINT Comp_Index = 0;
	UINT nSubChIdx = 0;

	struct ENSEMBLE_DESC *desc = &ENS_DESC;

	ensble.ensem_freq = freq;
	ensble.ensem_id = desc->id;

	strncpy(ensble.ensem_label, desc->Label, ENSEMBLE_LABEL_MAX);

	for (i = 0; i < desc->svr_num; i++) {
		for (j = 0; j < desc->svr_desc[i].Num_ser_comp; j++) {
			Comp_Index = desc->svr_desc[i].ser_comp_num[j];
			switch (desc->svr_comp[Comp_Index].TMID) {
			case MSC_STREAM_AUDIO:
				ensble.sub_ch[nSubChIdx].sub_ch_id
					= desc->svr_comp[Comp_Index].SubChid;
				ensble.sub_ch[nSubChIdx].start_addr
					= desc->svr_comp[Comp_Index].START_Addr;
				ensble.sub_ch[nSubChIdx].tmid
					= desc->svr_comp[Comp_Index].TMID;
				ensble.sub_ch[nSubChIdx].svc_type
					= desc->svr_comp[Comp_Index].ASCTy;
				ensble.sub_ch[nSubChIdx].svc_id
					= desc->svr_desc[i].Sid;
				memcpy(ensble.sub_ch[nSubChIdx].svc_label,
					desc->svr_desc[i].Label, SVC_LABEL_MAX);
				nSubChIdx++;
				break;

			case MSC_STREAM_DATA:
				ensble.sub_ch[nSubChIdx].sub_ch_id
					= desc->svr_comp[Comp_Index].SubChid;
				ensble.sub_ch[nSubChIdx].start_addr
					= desc->svr_comp[Comp_Index].START_Addr;
				ensble.sub_ch[nSubChIdx].tmid
					= desc->svr_comp[Comp_Index].TMID;
				ensble.sub_ch[nSubChIdx].svc_type
					= desc->svr_comp[Comp_Index].DSCTy;
				ensble.sub_ch[nSubChIdx].svc_id
					= desc->svr_desc[i].Sid;
				memcpy(ensble.sub_ch[nSubChIdx].svc_label,
					desc->svr_desc[i].Label, SVC_LABEL_MAX);
				nSubChIdx++;
				break;

			case FIDC: /* No service */
/*
				ensble.sub_ch[nSubChIdx].tmid
					= desc->svr_comp[Comp_Index].TMID;
				ensble.sub_ch[nSubChIdx].svc_id
					= desc->svr_desc[i].Sid;
				memcpy(ensble.sub_ch[nSubChIdx].svc_label,
					desc->svr_desc[i].Label, SVC_LABEL_MAX);
*/
				break;

			case MSC_PACKET_DATA:
/*
				ensble.sub_ch[nSubChIdx].sub_ch_id
				= desc->svr_comp[Comp_Index].SubChid;
				ensble.sub_ch[nSubChIdx].start_addr
					= desc->svr_comp[Comp_Index].START_Addr;
				ensble.sub_ch[nSubChIdx].tmid
					= desc->svr_comp[Comp_Index].TMID;
				ensble.sub_ch[nSubChIdx].svc_type
					= desc->svr_comp[Comp_Index].DSCTy;
				ensble.sub_ch[nSubChIdx].svc_id
					= desc->svr_desc[i].Sid;
				memcpy(ensble.sub_ch[nSubChIdx].svc_label,
					desc->svr_desc[i].Label, SVC_LABEL_MAX);
*/
				break;
			default:
				RTV_DBGMSG0("NO TMID\n");
				break;
			}

/*
			RTV_DBGMSG2("ensble->sub_ch[%d].sub_ch_id: %d\n",
			nSubChIdx, ensble->sub_ch[nSubChIdx].sub_ch_id);
			RTV_DBGMSG2("ensble->sub_ch[%d].start_addr: %d\n",
			nSubChIdx, ensble->sub_ch[nSubChIdx].start_addr);
			RTV_DBGMSG2("ensble->sub_ch[%d].tmid: %d\n",
				nSubChIdx, ensble->sub_ch[nSubChIdx].tmid);
			RTV_DBGMSG2("ensble->sub_ch[%d].svc_type: %d\n",
				nSubChIdx, ensble->sub_ch[nSubChIdx].svc_type);
			RTV_DBGMSG2("ensble->sub_ch[%d].svc_id: 0x%lX\n",
				nSubChIdx, ensble->sub_ch[nSubChIdx].svc_id);

			desc->svr_desc[i].Label[SVC_LABEL_MAX] = '\0';
			RTV_DBGMSG2("ensble->sub_ch[%d].ServiceLabel: %s\n\n",
				nSubChIdx, desc->svr_desc[i].Label);
*/
		}
	}

	ensble.tot_sub_ch = nSubChIdx;
/*
	RTV_DBGMSG1("ensble->TotalSubChNumber: %d\n\n", ensble->tot_sub_ch);
*/

	return &ensble;
}

