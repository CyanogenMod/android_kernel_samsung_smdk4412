/*
 * tcc_fic_fig0.c
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

void fig0_ext00_update_reconf_mode(struct fic_parser_matadata *parser)
{
/*
	if (parser->cif_count == 0xff00)
		gpFicParserInfo->reconf_mode = 2;

	if (cif_cnt >= parser->cif_count &&
		(cif_cnt - parser->cif_count) > 0x100)
		gpFicParserInfo->reconf_mode = 3;
	else if (cif_cnt + 0x1400 - parser->cif_count > 0x100)
		gpFicParserInfo->reconf_mode = 3;

	parser->cif_count = cif_cnt;
	if (fig_len == 5 && change_flag) {
		parser->occur_change = *fig_buff;

		if (parser->reconf_stage == 0)
			parser->reconf_stage = 1;

		if (parser->cif_count_hi == 0xff) {
			parser->cif_count_hi =
				(parser->cif_count & 0x1f00) >> 8;
			parser->cif_count_lo =
				(parser->cif_count & 0xff);
		}
	} else if (parser->reconf_stage == 1 &&
		parser->cif_count_hi != ((parser->cif_count & 0x1f00) >> 8) &&
		parser->occur_change <= ((parser->cif_count & 0xff) + 4)) {
			parser->reconf_stage = 2;
			parser->cif_count_hi = 0xff;
			gpFicParserInfo->reconf_mode = 1;
		}
	}
*/
}

/********************************************************************
*   Function: Parsing FIG 0/0
*   Ensemble Information Field ETSI EN 300401 V010401p Figure 29
*********************************************************************/
s32 fig0_ext00(struct fic_parser_matadata *parser,
	u8 *fig_buff, s32 fig_len)
{
	s32 ret = FICERR_SUCCESS;
	u8 change_flag;
	u16 cif_cnt;
	struct tcc_ensemble *esb_start;

	esb_start = &parser->esmbl_start;
	if (esb_start == NULL)
		ret = FICERR_FIG0_0_NO_ENSEMBLEARRAY;
	else {
		esb_start->eid = (u16)(*fig_buff++) << 8;
		/* eid = Country Id(b15-b12), Ensemble reference(b11-b0) */
		esb_start->eid |= *fig_buff++;

		/*b15-b14: Change flag about service Organization */
		change_flag  = (*fig_buff & 0xc0) >> 6;
		/*b13: Alarm flag, 1: alarm message accessible */
		esb_start->al_flag = (*fig_buff & 0x20) >> 5;
		cif_cnt	= (u16)(*fig_buff++ & 0x1f) << 8;
		/*b12-b0: CIF Count, b12-b8->modulo 20, b7-b0->modulo250 */
		cif_cnt |= *fig_buff++;

		/*fig0_ext00_update_reconf_mode(parser);*/
	}
	return ret;
}

/********************************************************************
*   Function: Parsing FIG 0/1
*   Sub-channel Organization Field ETSI EN 300401 V010401p Figure 22
*********************************************************************/
s32 fig0_ext01(struct fic_parser_matadata *parser,
	u8 *fig_buff, s32 fig_len)
{
	s32 ret = FICERR_SUCCESS;
	u8 subchid, i, flag = 0;
	u8 tbl_switch = 0xff;

	struct tcc_ensemble *esmbl;
	struct tcc_sub_channel *subch_start, *subch_prev;

	esmbl = &parser->esmbl_start;
	subch_start = subch_prev = parser->subch_start;

	if (subch_start == 0)
		return FICERR_FIG0_1_NO_SUBCHARRAY;
	while (fig_len > 0) {
		if (esmbl->num_subch == NUM_SUB_CH) {
			ret = FICERR_FIG0_1_FULL_SUBCHARRAY;
			return ret;
		}

		subchid = (*fig_buff & 0xfc) >> 2; /*b15-b10: subch_id */

		for (i = 0; i < esmbl->num_subch; i++) {
			subch_start = subch_prev + i;
			if (subchid == subch_start->subch_id) {
				flag = 1;
				break;
			}
		}

		subch_start = subch_prev + i;
		subch_start->subch_id = subchid;
		subch_start->start_cu = (u16)(*fig_buff++ & 0x03) << 8;
		/*b9-b0: Start Address */
		subch_start->start_cu |= *fig_buff++;
		/*b7(b15): Short or Long form flag, 0:Short form 1:Long form*/
		subch_start->form_flag = ((*fig_buff & 0x80) ? 1 :  0) << 3;
		fig_len -= 2;


		/* In Long form case */
		if (subch_start->form_flag & 0x08) {
			/*b14-b12: Option  000: 1,2,3,4-A  001: 1,2,3,4-B*/
			subch_start->form_flag |=
				((*fig_buff & 0x10) ? 1 : 0) << 2;
			/*b11-b10: Protection level */
			subch_start->form_flag |= (*fig_buff & 0x0c) >> 2;
			subch_start->size_cu = (u16)(*fig_buff++ & 0x03) << 8;
			/*b9-b0: Sub-channel size */
			subch_start->size_cu |= *fig_buff++;
			fig_len -= 2;
		} else {
			/* In Short form case */
			/* b6: Table switch */
			tbl_switch = (*fig_buff & 0x40)>>6;
			if (!tbl_switch)
				/*b5-b0: Table index */
				subch_start->tbl_index = *fig_buff++ & 0x3f;
			else
				/* Invalid Table index */
				subch_start->tbl_index = *fig_buff++ & 0xff;

			fig_len -= 1;
		}

		if (!flag)
			esmbl->num_subch++;
		flag = 0;
	}
	if (fig_len < 0)
		ret = FICERR_FIG0_1_INVALID_LENGTH;

	return ret;
}

u8 *fig0_ext02_sid(struct fic_parser_matadata *parser,
	u8 *fig_buff, s32 *fig_len, u32 *sid)
{
	/* PD from FIG0 Header PD=1: 32bit sid */
	if (parser->fig_pd == 1) {
		/*b31-b0: sid(ECC,Country Id,Service reference) */
		*sid  = (u32)(*fig_buff++ << 24);
		*sid |= (u32)(*fig_buff++ << 16);
		*sid |= (u32)(*fig_buff++ << 8);
		*sid |= (u32)(*fig_buff++);
		*fig_len -= 4;
	} else {
		/* PD from FIG0 Header PD=0: 16bit sid */
		/*b15-b0: sid(Country Id,Service reference */
		*sid = (u32)(*fig_buff++ << 8);
		*sid |= (u32)(*fig_buff++);
		*fig_len -= 2;
	}
	return fig_buff;
}

u8 *fig0_ext02_check_num_comp(struct fic_parser_matadata *parser,
	u8 *fig_buff, u8 *num_comp)
{
	/* b3-b0: Number of Service Componentsin current FIG0_2,
	 *		max 12 or 11 */
	*num_comp = (*fig_buff & 0x0f);
	if (parser->fig_pd == 1) {
		if (*num_comp > 11)
			return NULL;
	} else {
		if (*num_comp > 12)
			return NULL;
	}
	return fig_buff;
}

u8 *fig0_ext02_update_svc(struct fic_parser_matadata *parser,
	u8 *fig_buff, u32 sid, struct tcc_service **svc_start)
{
	u8 i, al;
	struct tcc_ensemble *esmbl;
	/*struct tcc_service *svc_start;*/

	esmbl = &parser->esmbl_start;
	al = 0;
	for (i = 0; i < esmbl->num_svc; i++) {
		*svc_start = parser->svc_start + i;
		if (sid == (*svc_start)->sid) {
			al = 1;
			break;
		}
	}

	if (al == 0) {
		*svc_start = parser->svc_start + i;
		(*svc_start)->sid = sid;
		/*b6-b4: Conditional Access Identifier */
		(*svc_start)->ca_id = (*fig_buff++ & 0x70) >> 4;
		esmbl->num_svc++;
	} else {
		fig_buff++;
	}
	return fig_buff;
}

s32 fig0_ext02_check_new_svc_comp(struct fic_parser_matadata *parser,
	u32 sid, u16 scid, u8 tmid, u8 fidc_id)
{
	s32 j;
	struct tcc_service_comp *svc_comp_start;

	for (j = 0; j < parser->esmbl_start.num_svc_comp; j++) {
		svc_comp_start = parser->svc_comp_start + j;
		if ((tmid == 0) || (tmid == 1) || (tmid == 2)) {
			if ((fidc_id == svc_comp_start->fidc_id) &&
				(tmid == svc_comp_start->tmid) &&
				(sid == svc_comp_start->sid))
				return 1;
		} else if (tmid == 3) {
			if ((scid == svc_comp_start->scid) &&
				(sid == svc_comp_start->sid))
				return 1;
		}
	}
	return 0;
}

u8 *fig0_ext02_update_svc_comp(struct fic_parser_matadata *parser,
	u8 *fig_buff, u32 sid, u8 *p_flag, struct tcc_service *svc_start)
{
	u8 is_updated, tmid, ascty_dscty = 0xff, fidc_id = 0xff;
	u16 scid = 0xffff;

	struct tcc_ensemble *esmbl;
	/*struct tcc_service *svc_start;*/
	struct tcc_service_comp *svc_comp_start;

	esmbl = &parser->esmbl_start;
	svc_comp_start = parser->svc_comp_start;

	/*b15-b14: Transport Mechanism Identifier */
	tmid = (*fig_buff & 0xc0) >> 6;
	switch (tmid) {
	case 0: /* MSC-Stream mode - audio */
	case 1: /* MSC-Stream mode - data */
	case 2:
		/*b13-b8: Data Service Component type */
		ascty_dscty = (*fig_buff++ & 0x3f);
		/*b7-b2: FIDCId */
		fidc_id = (*fig_buff & 0xfc) >> 2;
		/* Invalid scid */
		scid = 0xffff;
		break;
	case 3: /* MSC-Packet mode - data */
		scid = (*fig_buff++ & 0x3f) << 6;
		/*b13-b2: Service Component Identifier */
		scid |= (*fig_buff & 0xfc) >> 2;
		/* Invalid subch_id */
		fidc_id = 0xff;
		ascty_dscty = 0xff;
		break;
	default:
		tcbd_debug(DEBUG_ERROR, "unknown tmid!\n");
		break;
	}

	is_updated = fig0_ext02_check_new_svc_comp(parser,
		sid, scid, tmid, fidc_id);
	if (is_updated == 0) {
		/*b1: PS flag, 1: Primary service component  0: Secondary */
		if ((*fig_buff & 0x02) && *p_flag == 0) {
			svc_comp_start = parser->svc_comp_start +
						esmbl->num_svc_comp;
			svc_comp_start->order = 0;
			*p_flag = 1;
		} else {
			if (*p_flag) {
				svc_comp_start = parser->svc_comp_start +
							esmbl->num_svc_comp;
				svc_comp_start->order =
					svc_start->num_svc_comp;
			} else {
				svc_comp_start = parser->svc_comp_start +
							esmbl->num_svc_comp + 1;
				svc_comp_start->order =
					svc_start->num_svc_comp + 1;
			}
		}
		svc_comp_start->sid = sid;
		svc_comp_start->tmid = tmid;
		svc_comp_start->ascty_dscty = ascty_dscty;
		svc_comp_start->fidc_id = fidc_id;
		svc_comp_start->scid = scid;
		svc_comp_start->ca_flag = (*fig_buff++ & 0x01);
		svc_comp_start->ca_org = 0xffff;
		svc_comp_start->scids = INITVAL_SCIDS;

		esmbl->num_svc_comp++;
		svc_start->num_svc_comp++;
	} else {
		fig_buff++;   /* pointer update */
	}
	return fig_buff;
}


/********************************************************************
*   Function: Parsing FIG 0/2
*   Service Organization Field EN 300401 v010401p Figure 24
*********************************************************************/
s32 fig0_ext02(struct fic_parser_matadata *parser,
	u8 *fig_buff, s32 fig_len)
{
	s32 ret = FICERR_SUCCESS;
	u8 k, num_comp, p_flag;
	u32 sid;
	struct tcc_ensemble *esmbl;
	struct tcc_service *svc;

	esmbl = &parser->esmbl_start;
	while (fig_len > 0) {
		if (esmbl->num_svc == NUM_SVC)
			return FICERR_FIG0_2_FULL_SERVICEARRAY;

		if (!fig_buff)
			return FICERR_FIG0_2_INVALID_LENGTH;

		fig_buff = fig0_ext02_sid(parser, fig_buff, &fig_len, &sid);
		fig_buff = fig0_ext02_check_num_comp(
				parser, fig_buff, &num_comp);
		if (fig_buff == NULL)
			return FICERR_FIG0_2_INVALID_LENGTH;

		fig_buff = fig0_ext02_update_svc(parser, fig_buff, sid, &svc);

		if (num_comp == 0)
			goto cont_while;

		p_flag = 0;
		for (k = 0; k < num_comp; k++) {
			if (esmbl->num_svc_comp == NUM_SVC_COMP)
				return FICERR_FIG0_2_FULL_SRVCOMPARRAY;

			fig_buff = fig0_ext02_update_svc_comp(
					parser, fig_buff, sid, &p_flag, svc);
			fig_len -= 2;
		}
cont_while:
		fig_len -= 1;
	}

	if (fig_len < 0)
		return FICERR_FIG0_2_INVALID_LENGTH;

	return ret;
}

/********************************************************************
*   Function: Parsing FIG 0/3
*   Service Component Field in Packet Mode
*	ETSI EN 300401 V010401p Figure 26
*********************************************************************/
s32 fig0_ext03(struct fic_parser_matadata *parser,
	u8 *fig_buff, s32 fig_len)
{
	u8 i;
	u16 scid = 0xffff;
	struct tcc_ensemble *esmbl;
	struct tcc_service_comp *svc_comp_start;

	u8 ca_org_flag = 0xff, dg_mf_flag = 0xff;
	u8 ascty_dscty = 0xff, fidc_id = 0xff;
	u16 pack_add = 0xffff, ca_org = 0xffff;

	esmbl = &parser->esmbl_start;
	svc_comp_start = parser->svc_comp_start;

	if (svc_comp_start == 0)
		return FICERR_FIG0_3_NO_SRVCOMPARRAY;

	if (esmbl->num_svc_comp == 0)
		return FICERR_FIG0_3_NOTREADY_SRVCOMP1;

	while (fig_len > 0) {
		scid = (u16)(*fig_buff++) << 4;
		/* Service Component Identifier (b15~b4) */
		scid |= (*fig_buff & 0xf0) >> 4;
		/* b0, 0: ca_org field absent, 1: ca_org field present */
		ca_org_flag = (*fig_buff++ & 0x01);
		/* b7, 1: data groups are not used to transport
		 *		the service component */
		dg_mf_flag = (*fig_buff & 0x80) >> 7;
		/* b5-b0, Data Service Component Type */
		ascty_dscty = (*fig_buff++ & 0x3f);
		/* b15-b10, Sub-channel Identifier */
		fidc_id = (*fig_buff & 0xfc) >> 2;
		pack_add = (u16)(*fig_buff++ & 0x03) << 8;
		/* b9-b0, Packet address */
		pack_add |= (*fig_buff++);
		fig_len -= 5;
		if (ca_org_flag) {
			/* b15-b0, Conditional Access Organization */
			ca_org = (u16)(*fig_buff++) << 8;
			ca_org |= (u16)(*fig_buff++);
			fig_len -= 2;
		}
		for (i = 0; i < esmbl->num_svc_comp; i++) {
			svc_comp_start = parser->svc_comp_start + i;
			if (scid == svc_comp_start->scid) {
				svc_comp_start->ca_org_flag = ca_org_flag;
				svc_comp_start->dg_mf_flag = dg_mf_flag;
				svc_comp_start->ascty_dscty = ascty_dscty;
				svc_comp_start->fidc_id = fidc_id;
				svc_comp_start->pack_add = pack_add;
				if (ca_org_flag)
					svc_comp_start->ca_org = ca_org;
			}
		}
	}
	if (fig_len < 0)
		return FICERR_FIG0_3_INVALID_LENGTH;
	return FICERR_SUCCESS;
}

/********************************************************************
*   Function: Parsing FIG 0/4
*   Service Component Field in Stream Mode or
*	FIC ETSI EN 300401 v010401p Figure 27
*********************************************************************/
s32 fig0_ext04(struct fic_parser_matadata *parser,
	u8 *fig_buff, s32 fig_len)
{
	u8 i = 0;
	u8 flag, id;
	struct tcc_ensemble *esmbl;
	struct tcc_service_comp *svc_comp_start;

	esmbl = &parser->esmbl_start;
	svc_comp_start = parser->svc_comp_start;

	if (svc_comp_start == 0)
		return FICERR_FIG0_4_NO_SRVCOMPARRAY;

	while (fig_len > 0) {
		/* b6, 0: MSC and subch_id, 1: FIC and FIDCId */
		flag = (*fig_buff & 0x40) ? 1 : 0;
		/* b5-b0, subch_id or FIDCId */
		id = (*fig_buff++ & 0x3f);

		for (i = 0; i < esmbl->num_svc_comp; i++) {
			svc_comp_start = parser->svc_comp_start + i;
			if (id == svc_comp_start->fidc_id) {
				if (svc_comp_start->ca_org != 0xffff)
					return FICERR_FIG0_4_ALREADY_CA_FIELD;
				else
					break;
			}
		}

		if (i >= esmbl->num_svc_comp)
			return FICERR_FIG0_4_NOTREADY_SRVCOMP;

		svc_comp_start = parser->svc_comp_start + i;
		svc_comp_start->dg_mf_flag = flag;
		svc_comp_start->fidc_id = id;
		svc_comp_start->ca_org = (u16)(*fig_buff++) << 8;
		/* b15-b0: Conditional Access Organization */
		svc_comp_start->ca_org |= (u16)(*fig_buff++);
		fig_len -= 3;
	}

	if (fig_len < 0)
		return FICERR_FIG0_4_INVALID_LENGTH;

	return FICERR_SUCCESS;
}

/********************************************************************
*   Function: Parsing FIG 0/5
*   Service Component Language Field ETSI EN 300401 v010401p Figure 45
*   Short form(LS=0) b7: LS flag, b6: MSC_FIC flag, b5-b0: subch_id_FIDCId,
*					b7-b0: Language
*   Long form(LS=1) b15: LS flag, b14-b12: Rfa, b11-b0: scid, b7-b0: Language
*********************************************************************/
s32 fig0_ext05(struct fic_parser_matadata *parser, u8 *fig_buff, s32 fig_len)
{
	u16 id;
	u8 i;
	struct tcc_ensemble *esmbl;
	struct tcc_service_comp *svc_comp_start;

	esmbl = &parser->esmbl_start;
	svc_comp_start = parser->svc_comp_start;

	if (svc_comp_start == 0)
		return FICERR_FIG0_5_NO_SRVCOMPARRAY;

	if (esmbl->num_svc == 0)
		return FICERR_FIG0_5_NOTREADY_SERVICE;

	while (fig_len > 0) {
		if (*fig_buff & 0x80) {
			/* b7(LS) = 1: Long form Case */
			id = (*fig_buff++ & 0x0f) << 8;
			/* b11-b0: scid Service Component Id */
			id |= (*fig_buff++);
			fig_len -= 2;
			for (i = 0; i < esmbl->num_svc_comp; i++) {
				svc_comp_start = parser->svc_comp_start + i;
				if (id == svc_comp_start->scid)
					break;
			}
		} else {
			/* b7(LS) = 0: Short form Case */
			/* b5-b0: subch_id/FIDCId is mixed*/
			id = (*fig_buff++ & 0x3f);
			fig_len -= 1;
			for (i = 0; i < esmbl->num_svc_comp; i++) {
				svc_comp_start = parser->svc_comp_start + i;
				if (id == svc_comp_start->fidc_id)
					break;
			}
		}

		if (i == esmbl->num_svc_comp)
			return FICERR_FIG0_5_NOTREADY_SRVCOMP;

		svc_comp_start = parser->svc_comp_start + i;
		/* b7-b0: Language of the audio or data service component,
		 * TS 101 756 Tables 9 and 10 */
		svc_comp_start->lang = *fig_buff++;
		fig_len -= 1;
	}
	if (fig_len < 0)
		return FICERR_FIG0_5_INVALID_LENGTH;

	return FICERR_SUCCESS;
}

/****************************************************************************
*   Function: Parsing FIG 0/7
*   Data Service Component Type extension
*   There is not FIG 0/7 in ETSI EN 300401 not yet.
*****************************************************************************/
s32 fig0_ext07(struct fic_parser_matadata *parser,
	u8 *fig_buff, s32 fig_len)
{
	u8 tmid, num, i, k;
	u16 id, extdscty;
	struct tcc_ensemble *esmbl;
	struct tcc_service_comp *svc_comp_start;

	esmbl = &parser->esmbl_start;
	svc_comp_start = parser->svc_comp_start;

	tmid = (*fig_buff & 0x30) >> 4;
	num = (*fig_buff++ & 0x0f);
	for (k = 0; k < num; k++) {
		switch (tmid) {
		case 0:
			return FICERR_FIG0_7_RETURN;
		case 1:
		case 2:
			/* MSC stream/FIDC */
			id = (*fig_buff & 0xfc) >> 2;
			extdscty = (u16)(*fig_buff & 0x03) << 8;
			extdscty |= (*fig_buff++);
			fig_len -= 2;
			for (i = 0; i < esmbl->num_svc_comp; i++) {
				svc_comp_start = parser->svc_comp_start + i;
				if (tmid == svc_comp_start->tmid &&
					id == svc_comp_start->fidc_id)
					break;
			}
			break;

		case 3:
			id = (u16)(*fig_buff & 0x3f) << 6;
			id |= (*fig_buff++ & 0xfc) >> 2;
			extdscty = (u16)(*fig_buff & 0x03) << 8;
			extdscty |= (*fig_buff++);
			fig_len -= 3;
			for (i = 0; i < esmbl->num_svc_comp; i++) {
				svc_comp_start = parser->svc_comp_start + i;
				if (tmid == svc_comp_start->tmid &&
					id == svc_comp_start->scid)
					break;
			}
			break;
		default:
			break;
		}
	}
	return FICERR_SUCCESS;
}

/********************************************************************
*   Function: Parsing FIG 0/8
*   Service Component Global Definition Field EN 300401 V010401p Figure 28
*********************************************************************/
s32 fig0_ext08(struct fic_parser_matadata *parser,
	u8 *fig_buff, s32 fig_len)
{
	s32 ret = FICERR_SUCCESS;
	u8 i;
	u8 ls_flag = 0xff, ext_flag = 0xff;
	u8 scids = 0xff, subch_id = 0xff;
	u32 sid = 0xffffffff;
	u16 scid = 0xffff;
	s32 is_fic_db = 0;
	s32 is_fic = 0;
	struct tcc_ensemble *esmbl;
	struct tcc_service_comp *svc_comp_start;

	esmbl = &parser->esmbl_start;
	svc_comp_start = parser->svc_comp_start;

	if (svc_comp_start == 0)
		return FICERR_FIG0_8_NO_SRVCOMPARRAY;

	while (fig_len > 0) {
		/* PD from FIG0 Header PD=1: 32bit sid */
		if (parser->fig_pd == 1) {
			/* b32-b0: sid(Service Identifier) */
			sid  = (u32)(*fig_buff++ << 24);
			sid |= (u32)(*fig_buff++ << 16);
			sid |= (u32)(*fig_buff++ << 8);
			sid |= (u32)(*fig_buff++);
			fig_len -= 4;
		} else {
			/* PD from FIG0 Header PD=0: 16bit sid  */
			sid  = (u32)(*fig_buff++ << 8);
			/* b15-b0: sid(Service Identifier) */
			sid |= (u32)(*fig_buff++);
			fig_len -= 2;
		}

		/* b7: Ext.flag(extension Flag)
		 * 0: Rfa field absent, 1: Rfa field present */
		ext_flag = (*fig_buff & 0x80);
		/* b3-b0:
		 * scids(Service Component Identifier within the Service) */
		scids = (*fig_buff++ & 0x0f);
		/* b7: LS flag(Long form or Short form),
		 * 1: Long form 0: Short form */
		ls_flag = (*fig_buff & 0x80);

		if (ls_flag) {
			/* Long form case */
			scid = (*fig_buff++ & 0x0f) << 8;
			/* b11-b0: scid(Service Component Identifier) */
			scid |= (*fig_buff++);
			fig_len -= 3;
		} else {
			/* Short form case */
			/* b6: MSC_FIC flag 0: MSC_subch_id 1: FIC_FIDCId */
			is_fic = ((*fig_buff) >> 6) & 1;
			/* b5-b0: subch_id or FIDCId */
			subch_id = (*fig_buff++ & 0x3f);
			fig_len -= 2;
		}


		if (ext_flag) {
			fig_buff++;
			fig_len -= 1;
		}

		for (i = 0; i < esmbl->num_svc_comp; i++) {
			svc_comp_start = parser->svc_comp_start + i;
			if (sid != svc_comp_start->sid)
				continue;

			if (ls_flag && scid == svc_comp_start->scid) {
				svc_comp_start->scids = scids;
				break;
			} else {
				is_fic_db = (svc_comp_start->tmid == 2) ? 1 : 0;
				if ((is_fic_db == is_fic) &&
					(subch_id == svc_comp_start->fidc_id)) {
					svc_comp_start->scids = scids;
					break;
				}
			}
		}
	}
	if (fig_len < 0)
		ret = FICERR_FIG0_8_INVALID_LENGTH;
	return ret;
}

/********************************************************************
*   Function: Parsing FIG 0/13
*   User Application Field EN 300401 v010401p Figure68
*********************************************************************/
s32 fig0_ext13(struct fic_parser_matadata *parser,
	u8 *fig_buff, s32 fig_len)
{
	u32 i, k;
	u32 sid;
	u8 scids;
	struct tcc_ensemble *esmbl;
	struct tcc_user_app_types *usrapp_start;

	esmbl = &parser->esmbl_start;
	usrapp_start = parser->user_app_start;

	if (usrapp_start == 0)
		return FICERR_FIG0_13_NO_USERAPPLARRAY;

	while (fig_len > 0) {
		if (esmbl->num_user_app == NUM_USER_APP)
			return FICERR_FIG0_13_FULL_USERAPPLARRAY;

		/* PD from FIG0 Header PD=1: 32bit sid */
		if (parser->fig_pd == 1) {
			sid  = (u32)(*fig_buff++ << 24);
			sid |= (u32)(*fig_buff++ << 16);
			sid |= (u32)(*fig_buff++ << 8);
			sid |= (u32)(*fig_buff++);
			fig_len -= 4;
		} else {
			/* PD from FIG0 Header PD=0: 16bit sid */
			sid = (u32)(*fig_buff++ << 8);
			/*b15-b0: sid(16bit) */
			sid |= (u32)(*fig_buff++);
			fig_len -= 2;
		}

		/* b7-b4:
		 * scids(Service Component Identifier within the Service) */
		scids = (*fig_buff & 0xf0) >> 4;

		for (i = 0; i < esmbl->num_user_app; i++) {
			usrapp_start = parser->user_app_start + i;
			if (sid == usrapp_start->sid &&
				scids == usrapp_start->scids) {
				return FICERR_FIG0_13_ALREADY_USERAPPL;
			}
		}
		usrapp_start = parser->user_app_start + i;

		/* b3-b0: No. of User Applications */
		usrapp_start->num_app = (*fig_buff++ & 0x0f);
		if (usrapp_start->num_app > 6) {
			usrapp_start->num_app = 0;
			return FICERR_FIG0_13_INVALID_LENGTH;
		}
		fig_len -= 1;

		for (i = 0; i < usrapp_start->num_app; i++) {
			struct tcc_user_app_type *ua =
				&usrapp_start->app_type[i];
			ua->type = (*fig_buff++) << 3;
			/*b15-b5: User Application Type */
			ua->type += ((*fig_buff & 0xe0) >> 5);
			/*b4-b0: User Application data length in bytes */
			ua->len = (*fig_buff++ & 0x1f);
			if (ua->len > 23) {
				ua->len = 0;
				return FICERR_FIG0_13_INVALID_LENGTH;
			}

			for (k = 0; k < ua->len; k++)
				ua->data[k] = *fig_buff++;
			fig_len -= (2 + ua->len);
		}

		usrapp_start->sid = sid;
		usrapp_start->scids = scids;

		esmbl->num_user_app++;
	}

	if (fig_len < 0)
		return FICERR_FIG0_13_INVALID_LENGTH;

	return FICERR_SUCCESS;
}

/********************************************************************
*   Function: Parsing FIG 0/17
*   Programme Type Field EN 300401 V010401p Figure49
*   Input   : ucOffset is the point of buffer being processed and length
*   Return  : return the start point of next FIG
*********************************************************************/
s32 fig0_ext17(struct fic_parser_matadata *parser,
	u8 *fig_buff, s32 fig_len)
{
	s32 ret = FICERR_SUCCESS;
	u8 i, k;
	u8 lang_flag, cc_flag, end_flag;
	u16 sid;
	u8 sd, ps;
	struct tcc_ensemble *esmbl;
	struct tcc_program_type *prg_start;

	esmbl = &parser->esmbl_start;
	prg_start = parser->prg_start;

	if (prg_start == 0x00) {
		ret = FICERR_FIG0_17_NO_PROGTYPEARRAY;
		return ret;
	}

	if (parser->fig_oe0) {
		ret = FICERR_FIG0_17_OTHER_ENSEMBLE;
		return ret;
	}

	while (fig_len > 0) {
		if (esmbl->num_prg_type == NUM_PRG_TYPE) {
			ret = FICERR_FIG0_17_FULL_PROGTYPEARRAY;
			return ret;
		}
		sid  = (*fig_buff++) << 8;
		/*b15-b0: sid */
		sid |= *fig_buff++;
		/*b7:SD(Static or Dynamic) 1:
		 * represent the current programme contents */
		sd = (*fig_buff & 0x80) ? 1 : 0;
		/*b6: PS(Primary or Secondary) 0:
		 * Primary Service Component 1: Secondary  */
		ps = (*fig_buff & 0x40) ? 1 : 0;
		/* for making End condition */
		end_flag = 0;
		for (k = 0; k < esmbl->num_prg_type; k++) {
			prg_start = parser->prg_start + k;
			if (sid == prg_start->sid &&
				sd == prg_start->sd &&
				ps == prg_start->ps) {
				end_flag = 1;
				break;
			}
		}
		fig_len -= 2;
		if (end_flag)
			prg_start = parser->prg_start + k;
		else
			prg_start = parser->prg_start +
					esmbl->num_prg_type;

		prg_start->sid = sid;
		prg_start->sd = sd;
		prg_start->ps = ps;
		/*b5: Language flag, 0: language field absent */
		lang_flag = (*fig_buff & 0x20) ? 1 : 0;
		/*b4: Complementary Code flag, 1:
		 * CC, preceding Rfa and Rfu fields present */
		cc_flag = (*fig_buff & 0x10) ? 1 : 0;
		/* This field is strange things  */
		prg_start->nfc = (*fig_buff++ & 0x0f);
		fig_len -= 1;
		if (lang_flag) {
			fig_len -= 1;
			prg_start->lang = *fig_buff++;
		}

		/*b4-b0: International Code */
		prg_start->i18n_code = (*fig_buff++ & 0x1f);
		fig_len -= 1;
		if (cc_flag) {
			prg_start->coarse_code = (*fig_buff++);
			fig_len -= 1;
		}

		for (i = 0; i < prg_start->nfc; i++) {
			prg_start->fine_code = *fig_buff++;
			fig_len -= 1;
		}
		if (!end_flag)
			esmbl->num_prg_type++;
	}
	if (fig_len < 0)
		ret = FICERR_FIG0_17_INVALID_LENGTH;
	return ret;
}
