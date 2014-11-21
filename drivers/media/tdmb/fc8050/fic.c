/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : FIC.c

	Description : FIC Wrapper

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
#include "fic.h"

int fic_decoder_put(struct fic *pfic, u16 length)
{
	return fic_decoder(pfic, length);
}

struct esbinfo_t *fic_decoder_get_ensemble_info(u32 freq)
{
	return get_emsemble_info();
}

struct subch_info_t *fic_decoder_get_subchannel_info(u8 subchannel_id)
{
	return get_subchannel_info(subchannel_id);
}

struct service_info_t *fic_decoder_get_service_info(u32 sid)
{
	return get_service_info(sid);
}

struct scInfo_t *fic_decoder_get_sc_info(u16 scid)
{
	return get_sc_info(scid);
}

struct service_info_t *fic_decoder_get_service_info_list(u8 service_index)
{
	return get_service_info_list(service_index);
}

void fic_decoder_subchannel_org_prn(int subchannel_id)
{
	subchannel_org_prn(subchannel_id);
}

int fic_decoder_subchannel_org_to_didp(
	struct subch_info_t *sub_ch_info, struct didp_info_t *pdidp)
{
	return subchannel_org_to_didp(sub_ch_info, pdidp);
}

int fic_decoder_found_all_labels(void)
{
	if (found_all_labels())
		return 1;
	return 0;
}

int fic_decoder_subchannel_info_clean(void)
{
	subchannel_org_clean();
	return 0;
}
