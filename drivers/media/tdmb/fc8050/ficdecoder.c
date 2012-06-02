/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : ficdecoder.c

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
#include <linux/string.h>
#include <linux/delay.h>

#include "ficdecoder.h"
#include "fci_oal.h"

#define MSB(X)				(((X) >> 8) & 0Xff)
#define LSB(X)				((X) & 0Xff)
#define BYTESWAP(X)			((LSB(X)<<8) | MSB(X))

struct esbinfo_t ensemble_info[MAX_ESB_NUM];
struct service_info_t service_info[MAX_SVC_NUM];
struct scInfo_t sc_info[MAX_SC_NUM];
struct subch_info_t subchannel_info[MAX_SUBCH_NUM];
struct didp_info_t didpInfo[MAX_DIDP_NUM];

static int fig0_decoder(struct fig *pFig);
static int fig1_decoder(struct fig *pFig);

static int fig0_ext1_decoder(u8 cn, u8 *fibBuffer, int figLength);
static int fig0_ext2_decoder(u8 *fibBuffer, int figLength, int pd);
static int fig0_ext3_decoder(u8 *fibBuffer, int figLength);
/* static int fig0_ext4_decoder(u8 *fibBuffer, int figLength); */
static int fig0_ext10_decoder(u8 *fibBuffer, int figLength);
static int fig0_ext13_decoder(u8 *fibBuffer, int figLength, int pd);
static int fig0_ext14_decoder(u8 *fibBuffer, int figLength);
static int fig0_ext15_decoder(u8 *fibBuffer, int figLength, int pd);
static int fig0_ext18_decoder(u8 *fibBuffer, int figLength);
static int fig1_ext0_decoder(u8 *fibBuffer, int figLength);
static int fig1_ext5_decoder(u8 *fibBuffer, int figLength);
static int fig1_ext1_decoder(u8 *fibBuffer, int figLength);
static int fig1_ext4_decoder(u8 *fibBuffer, int figLength);

const u16 bitrate_profile[64][3] = {  /* CU  PL  Bit Rates */
	 { 16, 5,  32},  { 21, 4,  32},  { 24, 3,  32},
	 { 29, 2,  32},  { 35, 1,  32},  { 24, 5,  48},
	 { 29, 4,  48},  { 35, 3,  48},  { 42, 2,  48},
	 { 52, 1,  48},  { 29, 5,  56},  { 35, 4,  56},
	 { 42, 3,  56},  { 52, 2,  56},  { 32, 5,  64},
	 { 42, 4,  64},  { 48, 3,  64},  { 58, 2,  64},
	 { 70, 1,  64},  { 40, 5,  80}, { 52, 4,  80},
	 { 58, 3,  80},  { 70, 2,  80},  { 84, 1,  80},
	 { 48, 5,  96},  { 58, 4,  96},  { 70, 3,  96},
	 { 84, 2,  96},  {104, 1,  96},  { 58, 5, 112},
	 { 70, 4, 112},  { 84, 3, 112},  {104, 2, 112},
	 { 64, 5, 128},  { 84, 4, 128}, { 96, 3, 128},
	 {116, 2, 128},  {140, 1, 128},  { 80, 5, 160},
	 {104, 4, 160}, {116, 3, 160},  {140, 2, 160},
	 {168, 1, 160},  { 96, 5, 192},  {116, 4, 192},
	 {140, 3, 192},  {168, 2, 192},  {208, 1, 192},
	 {116, 5, 224},  {140, 4, 224}, {168, 3, 224},
	 {208, 2, 224},  {232, 1, 224},  {128, 5, 256},
	 {168, 4, 256}, {192, 3, 256},  {232, 2, 256},
	 {280, 1, 256},  {160, 5, 320},  {208, 4, 320},
	 {280, 2, 320},  {192, 5, 384},  {280, 3, 384},
	 {416, 1, 384}
};

const u16 uep_profile[14][5][9] = {	/* L1  L2  L3  L4 PI1 PI2 PI3 PI4 pad */
	/* 32kbps */
	{
		{  3,  5, 13,  3, 24, 17, 12, 17, 4},
		{  3,  4, 14,  3, 22, 13,  8, 13, 0},
		{  3,  4, 14,  3, 15,  9,  6,  8, 0},
		{  3,  3, 18,  0, 11,  6,  5,  0, 0},
		{  3,  4, 17,  0,  5,  3,  2,  0, 0}
	},

	/* 48kbps */
	{
		{  3,  5, 25,  3, 24, 18, 13, 18, 0},
		{  3,  4, 26,  3, 24, 14,  8, 15, 0},
		{  3,  4, 26,  3, 15, 10,  6,  9, 4},
		{  3,  4, 26,  3,  9,  6,  4,  6, 0},
		{  4,  3, 26,  3,  5,  4,  2,  3, 0}
	},

	/* 56kbps */
	{
		{  0,  0,  0,  0,  0,  0,  0,  0, 0},	/* not use */
		{  6, 10, 23,  3, 23, 13,  8, 13, 8},
		{  6, 12, 21,  3, 16,  7,  6,  9, 0},
		{  6, 10, 23,  3,  9,  6,  4,  5, 0},
		{  6, 10, 23,  3,  5,  4,  2,  3, 0}
	},

	/* 64kbps */
	{
		{  6, 11, 28,  3, 24, 18, 12, 18, 4},
		{  6, 10, 29,  3, 23, 13,  8, 13, 8},
		{  6, 12, 27,  3, 16,  8,  6,  9, 0},
		{  6,  9, 33,  0, 11,  6,  5,  0, 0},
		{  6,  9, 31,  2,  5,  3,  2,  3, 0}
	},

	/* 80kbps */
	{
		{  6, 10, 41,  3, 24, 17, 12, 18, 4},
		{  6, 10, 41,  3, 23, 13,  8, 13, 8},
		{  6, 11, 40,  3, 16,  8,  6,  7, 0},
		{  6, 10, 41,  3, 11,  6,  5,  6, 0},
		{  6, 10, 41,  3,  6,  3,  2,  3, 0}
	},

	/* 96kbps */
	{
		{  6, 13, 50,  3, 24, 18, 13, 19, 0},
		{  6, 10, 53,  3, 22, 12,  9, 12, 0},
		{  6, 12, 51,  3, 16,  9,  6, 10, 4},
		{  7, 10, 52,  3,  9,  6,  4,  6, 0},
		{  7,  9, 53,  3,  5,  4,  2,  4, 0}
	},

	/* 112kbps */
	{
		{  0,  0,  0,  0,  0,  0,  0,  0, 0},	/* not use */
		{ 11, 21, 49,  3, 23, 12,  9, 14, 4},
		{ 11, 23, 47,  3, 16,  8,  6,  9, 0},
		{ 11, 21, 49,  3,  9,  6,  4,  8, 0},
		{ 14, 17, 50,  3,  5,  4,  2,  5, 0}
	},

	/* 128kbps */
	{
		{ 11, 20, 62,  3, 24, 17, 13, 19, 8},
		{ 11, 21, 61,  3, 22, 12,  9, 14, 0},
		{ 11, 22, 60,  3, 16,  9,  6, 10, 4},
		{ 11, 21, 61,  3, 11,  6,  5,  7, 0},
		{ 12, 19, 62,  3,  5,  3,  2,  4, 0}
	},

	/* 160kbps */
	{
		{ 11, 22, 84,  3, 24, 18, 12, 19, 0},
		{ 11, 21, 85,  3, 22, 11,  9, 13, 0},
		{ 11, 24, 82,  3, 16,  8,  6, 11, 0},
		{ 11, 23, 83,  3, 11,  6,  5,  9, 0},
		{ 11, 19, 87,  3,  5,  4,  2,  4, 0}
	},

	/* 192kbps */
	{
		{ 11, 21, 109,  3, 24, 20, 13, 24, 0},
		{ 11, 20, 110,  3, 22, 13,  9, 13, 8},
		{ 11, 24, 106,  3, 16, 10,  6, 11, 0},
		{ 11, 22, 108,  3, 10,  6,  4,  9, 0},
		{ 11, 20, 110,  3,  6,  4,  2,  5, 0}
	},

	/* 224kbps */
	{
		{ 11, 24, 130,  3, 24, 20, 12, 20, 4},
		{ 11, 22, 132,  3, 24, 16, 10, 15, 0},
		{ 11, 20, 134,  3, 16, 10,  7,  9, 0},
		{ 12, 26, 127,  3, 12,  8,  4, 11, 0},
		{ 12, 22, 131,  3,  8,  6,  2,  6, 4}
	},

	/* 256kbps */
	{
		{ 11, 26, 152,  3, 24, 19, 14, 18, 4},
		{ 11, 22, 156,  3, 24, 14, 10, 13, 8},
		{ 11, 27, 151,  3, 16, 10,  7, 10, 0},
		{ 11, 24, 154,  3, 12,  9,  5, 10, 4},
		{ 11, 24, 154,  3,  6,  5,  2,  5, 0}
	},

	/* 320kbps */
	{
		{  0,  0,  0,  0,  0,  0,  0,  0, 0},	/* not use */
		{ 11, 26, 200,  3, 24, 17,  9, 17, 0},
		{  0,  0,  0,  0,  0,  0,  0,  0, 0},	/* not use */
		{ 11, 25, 201,  3, 13,  9,  5, 10, 8},
		{ 11, 26, 200,  3,  8,  5,  2,  6, 4}
	},

	/* 384kbps */
	{
		{ 12, 28, 245,  3, 24, 20, 14, 23, 8},
		{  0,  0,  0,  0,  0,  0,  0,  0, 0},	/* not use */
		{ 11, 24, 250,  3, 16,  9,  7, 10, 4},
		{  0,  0,  0,  0,  0,  0,  0,  0, 0},	/* not use */
		{ 11, 27, 247,  3,  8,  6,  2,  7, 0}
	}
};

int crc_good_cnt;
int crc_bad_cnt;
int fic_nice_cnt;

int announcement;
int bbm_recfg_flag;

struct esbinfo_t *get_emsemble_info(void)
{
	return &ensemble_info[0];
}

struct subch_info_t *get_subchannel_info(u8 subchannel_id)
{
	struct subch_info_t *sub_ch_info;
	int i;

	for (i = 0; i < MAX_SUBCH_NUM; i++) {
		sub_ch_info = &subchannel_info[i];
		if ((sub_ch_info->flag != 0)
			&& (sub_ch_info->subchannel_id == subchannel_id))
			break;
	}

	if (i == MAX_SUBCH_NUM) {
		for (i = 0; i < MAX_SUBCH_NUM; i++) {
			sub_ch_info = &subchannel_info[i];
			if (sub_ch_info->flag == 0)
				break;
		}
		if (i == MAX_SUBCH_NUM)
			return NULL;
	}

	return sub_ch_info;
}

struct service_info_t *get_service_info_list(u8 service_index)
{
	return &service_info[service_index];
}

struct service_info_t *get_service_info(u32 sid)
{
	struct service_info_t *svc_info;
	int i;

	for (i = 0; i < MAX_SVC_NUM; i++) {
		svc_info = &service_info[i];
		if ((svc_info->flag != 0) && (sid == svc_info->sid))
			break;
	}

	if (i == MAX_SVC_NUM) {
		for (i = 0; i < MAX_SVC_NUM; i++) {
			svc_info = &service_info[i];
			if (svc_info->flag == 0) {
				svc_info->sid = sid;
				break;
			}
		}
		if (i == MAX_SVC_NUM)
			return NULL;
	}

	return svc_info;
}

struct scInfo_t *get_sc_info(u16	scid)
{
	struct scInfo_t  *pScInfo;
	int i;

	for (i = 0; i < MAX_SC_NUM; i++) {
		pScInfo = &sc_info[i];
		if ((pScInfo->flag == 99) && (pScInfo->scid == scid)) {
			/* pScInfo->scid = 0xffff; */
			break;
		}
	}
	if (i == MAX_SVC_NUM) {
		for (i = 0; i < MAX_SVC_NUM; i++) {
			pScInfo = &sc_info[i];
			if (pScInfo->flag == 0)
				break;
		}
		if (i == MAX_SC_NUM)
			return NULL;
	}

	return pScInfo;
}

static unsigned short crc16(unsigned char *fibBuffer, int len)
{
	int i, j, k;
	unsigned int sta, din;
	unsigned int crc_tmp = 0x0;
	int crc_buf[16];
	int crc_coff[16] = {	/* CRC16 CCITT REVERSED */
		0, 0, 0, 0,		/* 0x0 */
		1, 0, 0, 0,		/* 0x8 */
		0, 0, 0, 1,		/* 0x1 */
		0, 0, 0, 1		/* 0x1 */
	};

	for (j = 0; j < 16; j++)
		crc_buf[j] = 0x1;

	for (i = 0; i < len; i++) {
		sta = fibBuffer[i] & 0xff;

		for (k = 7; k >= 0; k--) {
			din = ((sta >> k) & 0x1) ^ (crc_buf[15] & 0x1);

			for (j = 15; j > 0; j--)
				crc_buf[j] =
				(crc_buf[j-1] & 0x1)
				^ ((crc_coff[j-1] * din) & 0x1);

			crc_buf[0] = din;
		}
	}

	crc_tmp = 0;
	for (j = 15; j >= 0; j--)
		crc_tmp = (crc_tmp << 1) | (crc_buf[j] & 0x1);

	return ~crc_tmp & 0xffff;
}

int fic_crc_ctrl = 1;		/* fic crc check enable */

int fic_decoder(struct fic *pfic, u16 length)
{
	struct fib	*pfib;
	int	result = 0;
	int	i;
	u16	bufferCnt;

	bufferCnt = length;

	if (bufferCnt % 32) {
		/* print_log(NULL
			, "FIC BUFFER LENGTH ERROR %d\n", bufferCnt); */
		return 1;
	}

	for (i = 0; i < bufferCnt/32; i++) {
		pfib = &pfic->fib[i];
		if (fic_crc_ctrl) {
			if (crc16(pfib->data, 30) == BYTESWAP(pfib->crc)) {
				crc_good_cnt++;
				result = fib_decoder(pfib);
			} else {
				crc_bad_cnt++;
				/* print_log(NULL, "CRC ERROR: FIB %d\n", i); */
			}
		} else {
			result = fib_decoder(pfib);
			crc_good_cnt++;
		}
	}

	return result;
}

int fib_decoder(struct fib *pfib)
{
	struct fig  *pFig;
	int  type, length;
	int  fib_ptr = 0;
	int  result = 0;

	while (fib_ptr < 30) {
		pFig = (struct fig *)&pfib->data[fib_ptr];

		type = (pFig->head >> 5) & 0x7;
		length = pFig->head & 0x1f;

		if (pFig->head == 0xff || !length) {	 /* end mark */
			break;
		}

		fic_nice_cnt++;

		switch (type) {
		case 0:
			result = fig0_decoder(pFig);		/* MCI & SI */
			break;
		case 1:
			result = fig1_decoder(pFig);		/* SI */
			/*
			if (result)
				print_log(NULL, "SI Error [%x]\n", result);
			*/
			break;

		default:
			/*
			print_log(NULL, "FIG 0x%X Length : 0x%X 0x%X\n"
			, type, length, fib_ptr);
			*/
			result = 1;
			break;
		}

		fib_ptr += length + 1;
	}

	return result;
}

/*
 * MCI & SI
 */
static int fig0_decoder(struct fig *pFig)
{
	int result = 0;
	int extension, length, pd;
	u8  cn;

	length = pFig->head & 0x1f;
	cn = (pFig->data[0] & 0x80) >> 7;
	if ((bbm_recfg_flag == 1) && (cn == 0))
			return 0;
	/* if (cn)
		print_log(NULL, "N");
	 */
	extension = pFig->data[0] & 0x1F;
	pd = (pFig->data[0] & 0x20) >> 5;

	switch (extension) {
	case 1:
		result = fig0_ext1_decoder(cn, &pFig->data[1], length);
		break;
	case 2:
		result = fig0_ext2_decoder(&pFig->data[1], length, pd);
		break;
	case 3:	/* Service component in packet mode or without CA */
		result = fig0_ext3_decoder(&pFig->data[1], length);
		break;
	case 4:		/* Service component with CA */
		/*
		result = fig0_ext4_decoder(&pFig->data[1], length);
		*/
		break;
	case 10:	/* Date & Time */
		result = fig0_ext10_decoder(&pFig->data[1], length-1);
		break;
	case 13:
		result = fig0_ext13_decoder(&pFig->data[1], length, pd);
		break;
	case 14:    /* FEC */
		result = fig0_ext14_decoder(&pFig->data[1], length);
		break;
	case 15:
		result = fig0_ext15_decoder(&pFig->data[1], length, pd);
		break;
	case 0:		/* Ensembel Information */
	case 5:		/* Language */
	case 8:		/* Service component global definition */
	case 9:		/* Country LTO and International table */
	case 17:		/* Programme Type */
		result = dummy_decoder(&pFig->data[1], length);
		break;
	case 18:	/* Announcements */
		if (announcement)
			result =
			fig0_ext18_decoder(&pFig->data[1], length);
		break;
	case 19:	/* Announcements switching */
		/*
		print_log(NULL, "FIG 0x%X/0x%X Length : 0x%X\n"
		, 0, extension, length);
		*/
		break;
	default:
		/*
		print_log(NULL, "FIG 0x%X/0x%X Length : 0x%X\n"
		, 0, extension, length);
		*/
		result = 1;
		break;
	}

	return result;
}

static int fig1_decoder(struct fig *pFig)
{
	int result = 0;
	int length;
	int /*charset, oe,*/ extension;

	length = pFig->head & 0x1f;
	/* charset = (pFig->data[0] >> 4) & 0xF; */
	/* oe = (pFig->data[0]) >> 3 & 0x1; */
	extension = pFig->data[0] & 0x7;

	switch (extension) {
	case 0:	/* Ensembel Label */
		result = fig1_ext0_decoder(&pFig->data[1], length);
		break;
	case 1:	/* Programme service Label */
		result = fig1_ext1_decoder(&pFig->data[1], length);
		break;
	case 5:	/* Data service Label */
		result = fig1_ext5_decoder(&pFig->data[1], length);
		break;
	case 4:	/* Service component Label */
		result = fig1_ext4_decoder(&pFig->data[1], length);
		break;
	default:
		/*
		print_log(NULL, "FIG 0x%X/0x%X Length : 0x%X\n"
		, 1, extension, length);
		*/
		result = 1;
		break;
	}

	return result;
}

int dummy_decoder(u8 *fibBuffer, int figLength)
{
	return 0;
}

/*
 *  FIG 0/1 MCI, Sub Channel Organization
 */
int fig0_ext1_decoder(u8 cn, u8 *fibBuffer, int figLength)
{
	u8	sta;
	int	result = 0;
	int	readcnt = 0;

	u8	subchannel_id;
	struct subch_info_t	*sub_ch_info;

	while (figLength-1 > readcnt) {
		sta = fibBuffer[readcnt++];
		if (sta == 0xFF)
			break;
		subchannel_id = (sta >> 2) & 0x3F;
		sub_ch_info = get_subchannel_info(subchannel_id);
		if (sub_ch_info == NULL) {
			/*print_log(NULL, "subchannel_info error ..\n"); */
			return 1;
		}

		sub_ch_info->flag = 99;
		sub_ch_info->mode = 0;		/* T-DMB */
		sub_ch_info->subchannel_id = subchannel_id;

		sub_ch_info->start_address = (sta & 0x3) << 8;
		sta = fibBuffer[readcnt++];
		sub_ch_info->start_address |= sta;
		sta = fibBuffer[readcnt++];
		sub_ch_info->form_type = (sta & 0x80) >> 7;

		switch (sub_ch_info->form_type) {
		case	0:	/* short form */
			sub_ch_info->table_switch = (sta & 0x40) >> 6;
			sub_ch_info->table_index = sta & 0x3f;
			break;
		case	1:	/* long form */
			sub_ch_info->option = (sta & 0x70) >> 4;
			sub_ch_info->protect_level = (sta & 0x0c) >> 2;
			sub_ch_info->subch_size = (sta & 0x03) << 8;
			sta = fibBuffer[readcnt++];
			sub_ch_info->subch_size |= sta;
			break;
		default:
			/*
			print_log(NULL, "Unknown Form Type %d\n"
			, sub_ch_info->form_type);
			*/
			result = 1;
			break;
		}
		if (cn) {
			if (sub_ch_info->re_config == 0)
				sub_ch_info->re_config = 1;
			/* ReConfig Info Updated */
		}
	}

	return result;
}

/*
 *  FIG 0/2 MCI, Sub Channel Organization
 */
static int fig0_ext2_decoder(u8 *fibBuffer, int figLength, int pd)
{
	struct service_info_t *svc_info;
	struct subch_info_t	*sub_ch_info;
	u8	sta;
	int	result = 0;
	int	readcnt = 0;
	u32	sid = 0xffffffff;
	int	nscps;
	u32	temp;
	int	tmid;
	int	i;

	while (figLength-1 > readcnt) {
		temp = 0;

		temp = fibBuffer[readcnt++];
		temp = (temp << 8) | fibBuffer[readcnt++];


		switch (pd) {
		case 0:		/* 16-bit sid, used for programme services */
			{
				temp = temp;
				/*sid = temp & 0xFFF; */
				sid = temp;
			}
			break;
		case 1:		/*32bit sid, used for data service */
			{
				temp = (temp << 8) | fibBuffer[readcnt++];
				temp = (temp << 8) | fibBuffer[readcnt++];

				/*sid = temp & 0xFFFFF; */
				sid = temp;
			}
			break;
		default:
			break;
		}

		svc_info = get_service_info(sid);
		if (svc_info == NULL) {
			/*print_log(NULL, "get_service_info Error ...\n"); */
			break;
		}

		svc_info->addrType = pd;
		svc_info->sid = sid;
		svc_info->flag |= 0x02;

		sta = fibBuffer[readcnt++];    /* flag, CAId, nscps  */

		nscps = sta & 0xF;

		svc_info->nscps = nscps;

		for (i = 0; i < nscps; i++) {
			sta = fibBuffer[readcnt++];
			tmid = (sta >> 6) & 0x3;
			/* svc_info->tmid = tmid; */

			switch (tmid) {
			case 0:		/* MSC stream audio */
				svc_info->ascty = sta & 0x3f;
				sta = fibBuffer[readcnt++];
				if ((sta & 0x02) == 0x02) {	/* Primary */
					svc_info->sub_channel_id
						= (sta >> 2) & 0x3F;
					svc_info->tmid = tmid;
				}
				sub_ch_info =
				get_subchannel_info(svc_info->sub_channel_id);
				if (sub_ch_info == NULL) {
					/*
					print_log(NULL
					, "get_subchannel_info Error ...\n");
					*/
					return 1;
				}
				sub_ch_info->sid = svc_info->sid;
				svc_info->flag |= 0x04;
				break;
			case 1:		/* MSC stream data */
				svc_info->dscty = sta & 0x3f;
				sta = fibBuffer[readcnt++];
				if ((sta & 0x02) == 0x02) {	/* Primary */
					svc_info->sub_channel_id
						= (sta >> 2) & 0x3F;
					svc_info->tmid = tmid;
				}
				sub_ch_info =
				get_subchannel_info(svc_info->sub_channel_id);
				if (sub_ch_info == NULL) {
					/*
					print_log(NULL
					, "get_subchannel_info Error ...\n");
					*/
					return 1;
				}
				sub_ch_info->sid = svc_info->sid;
				svc_info->flag |= 0x04;
				break;
			case 2:		/* FIDC */
				svc_info->dscty = sta & 0x3f;
				sta = fibBuffer[readcnt++];
				if ((sta & 0x02) == 0x02) {	/*  Primary */
					svc_info->fidc_id = (sta & 0xFC) >> 2;
					svc_info->tmid = tmid;
				}
				svc_info->flag |= 0x04;
				break;
			case 3:		/* MSC packet data */
				svc_info->scid = (sta & 0x3F) << 6;
				sta = fibBuffer[readcnt++];
				if ((sta & 0x02) == 0x02) {	/*  Primary */
					svc_info->scid |= (sta & 0xFC) >> 2;
					svc_info->tmid = tmid;
				}
				/*  by iproda */
				svc_info->flag |= 0x04;
				break;
			default:
				/* print_log(NULL
					, "Unkown tmid [%X]\n", tmid); */
				result = 1;
				break;
			}
		}
	}

	return result;
}

int fig0_ext3_decoder(u8 *fibBuffer, int figLength)
{
	u8	sta;
	int	result = 0;
	int	readcnt = 0;
	u16	scid;
	int i;

	struct scInfo_t	*pScInfo;
	struct service_info_t	*svc_info;
	struct subch_info_t	*sub_ch_info;

	while (figLength-1 > readcnt) {
		scid = 0;
		sta = fibBuffer[readcnt++];
		scid = sta;
		scid = scid << 4;
		sta = fibBuffer[readcnt++];
		scid |= (sta & 0xf0) >> 4;

		pScInfo = get_sc_info(scid);
		if (pScInfo == NULL) {
			/* print_log(NULL, "get_sc_info Error ...\n"); */
			return 1;
		}

		pScInfo->flag = 99;
		pScInfo->scid = scid;
		pScInfo->scca_flag = sta & 0x1;
		sta = fibBuffer[readcnt++];
		pScInfo->dg_flag = (sta & 0x80) >> 7;
		pScInfo->dscty = (sta & 0x3f);
		sta = fibBuffer[readcnt++];
		pScInfo->sub_channel_id = (sta & 0xfc) >> 2;
		pScInfo->packet_address = sta & 0x3;
		pScInfo->packet_address = pScInfo->packet_address << 8;
		sta = fibBuffer[readcnt++];
		pScInfo->packet_address |= sta;
		if (pScInfo->scca_flag) {
			sta = fibBuffer[readcnt++];
			pScInfo->scca = sta;
			pScInfo->scca = pScInfo->scca << 8;
			sta = fibBuffer[readcnt++];
			pScInfo->scca |= sta;
		}

		for (i = 0; i < MAX_SVC_NUM; i++) {
			svc_info = &service_info[i];
			if (svc_info->scid == pScInfo->scid
				&& svc_info->tmid == 3) {
				sub_ch_info =
				get_subchannel_info(pScInfo->sub_channel_id);
				if (sub_ch_info == NULL) {
					/*
					print_log(NULL
					, "get_subchannel_info Error ...\n");
					*/
					return 1;
				}

				sub_ch_info->sid = svc_info->sid;
				svc_info->sub_channel_id
					= sub_ch_info->subchannel_id;
			}
		}
	}

	return result;
}

/*int fig0_ext4_decoder(u8 *fibBuffer, int figLength) {
	int result = 0;
	int readcnt = 0;
	int Mf, sub_channel_id, CAOrg;

	while (figLength - 1 > readcnt) {
		Mf      = (fibBuffer[readcnt] & 0x40) >> 6;
		sub_channel_id = (fibBuffer[readcnt] & 0x3f);
		CAOrg   =
		(fibBuffer[readcnt + 1] << 8) + fibBuffer[readcnt + 2];
		readcnt += 3;
		//print_log(NULL, "CA MF: %d, SubChiD: %d, CAOrg: %d\n"
		, Mf, sub_channel_id, CAOrg);
	}

	return result;
}*/

/*
 *  FIG 0/10 Date & Time
 */
int fig0_ext10_decoder(u8 *fibBuffer, int figLength)
{
	int result = 0;

	u8 MJD,  /*ConfInd,*/ UTCflag;
	/* u16 LSI; */
	u8 hour = 0; /*minutes = 0, seconds = 0*/
	u16 milliseconds = 0;

	MJD = (fibBuffer[0] & 0x7f) << 10;
	MJD |= (fibBuffer[1] << 2);
	MJD |= (fibBuffer[2] & 0xc0) >> 6;
	/*LSI = (fibBuffer[2] & 0x20) >> 5; */
	/*ConfInd = (fibBuffer[2] & 0x10) >> 4; */
	UTCflag = (fibBuffer[2] & 0x08) >> 3;

	hour = (fibBuffer[2] & 0x07) << 2;
	hour |= (fibBuffer[3] & 0xc0) >> 6;

	/* minutes = fibBuffer[3] & 0x3f; */

	if (UTCflag) {
		/* seconds = (fibBuffer[4] & 0xfc) >> 2; */
		milliseconds = (fibBuffer[4] & 0x03) << 8;
		milliseconds |= fibBuffer[5];
	}

	/*
	print_log(NULL, " %d:%d:%d.%d\n"
	, hour+9, minutes, seconds, milliseconds);
	*/

	return result;
}

/*
 *  FIG 0/13 Announcement
 */
int fig0_ext13_decoder(u8 *fibBuffer, int figLength, int pd)
{
	u8	sta;
	int	result = 0;
	int	readcnt = 0;
	u32	sid = 0xffffffff;
	/* u8	SCIdS; */
	u8	NumOfUAs;
	u16	UAtype;
	u8	UAlen;
	int	i, j;

	struct service_info_t	*svc_info;


	while (figLength-1 > readcnt) {
		switch (pd) {
		case 0:		/* 16-bit sid, used for programme services */
			{
				u32 temp;

				temp = 0;

				temp = fibBuffer[readcnt++];
				temp = (temp << 8) | fibBuffer[readcnt++];

				sid = temp;
			}
			break;
		case 1:		/* 32bit sid, used for data service */
			{
				u32 temp;

				temp = 0;

				temp = fibBuffer[readcnt++];
				temp = (temp << 8) | fibBuffer[readcnt++];
				temp = (temp << 8) | fibBuffer[readcnt++];
				temp = (temp << 8) | fibBuffer[readcnt++];

				sid = temp;
			}
			break;
		default:
			break;
		}

		svc_info = get_service_info(sid);
		if (svc_info == NULL) {
			/* print_log(NULL, "get_service_info Error ...\n"); */
			break;
		}
		svc_info->sid = sid;

		svc_info->flag |= 0x04;

		sta = fibBuffer[readcnt++];

		/* SCIdS = (sta & 0xff00) >> 4; */
		NumOfUAs = sta & 0xff;

		/* Because of Visual Radio */
		svc_info->num_of_user_appl = NumOfUAs;

		for (i = 0; i < NumOfUAs; i++) {
			UAtype = 0;
			sta = fibBuffer[readcnt++];
			UAtype = sta;
			sta = fibBuffer[readcnt++];
			UAtype = (UAtype << 3) | ((sta >> 5) & 0x07);

			/* Because of Visual Radio */
			UAlen = sta & 0x1f;

			svc_info->user_appl_type[i] = UAtype;
			svc_info->user_appl_length[i] = UAlen;

			for (j = 0; j < UAlen; j++) {
				sta = fibBuffer[readcnt++];
				svc_info->user_appl_data[i][j] = sta;
			}
		}
	}

	return result;
}

int fig0_ext14_decoder(u8 *fibBuffer, int figLength)
{
	int result = 0;
	int	readcnt = 0;
	unsigned char subch, fec_scheme;
	struct subch_info_t *sub_ch_info;

	while (figLength-1 > readcnt) {
		subch = (fibBuffer[readcnt] & 0xfc) >> 2;
		fec_scheme = (fibBuffer[readcnt] & 0x03);
		readcnt++;
		/*
		print_log(NULL, "SubChID: %d, FEC Scheme: %d\n"
		, subch, fec_scheme);
		*/
		sub_ch_info = get_subchannel_info(subch);
		if (sub_ch_info)
			sub_ch_info->fec_schem = fec_scheme;
	}

	return result;
}

 /*
 * TMMB kjju TODO
 */
int fig0_ext15_decoder(u8 *fibBuffer, int figLength, int pd)
{
	u8	sta;
	int	result = 0;
	int	readcnt = 0;
	u8 subchannel_id;
	struct subch_info_t		*sub_ch_info;

	while (figLength-1 > readcnt) {
		sta = fibBuffer[readcnt++];
		if (sta == 0xFF)
			break;

		subchannel_id = (sta & 0xfc) >> 2;
		sub_ch_info = get_subchannel_info(subchannel_id);
		if (sub_ch_info == NULL) {
			/* print_log(NULL, "subchannel_info error ..\n"); */
			return 1;
		}

		sub_ch_info->flag = 99;
		sub_ch_info->mode = 1;		/* T-MMB */
		sub_ch_info->subchannel_id = subchannel_id;
		sub_ch_info->start_address = (sta & 0x3) << 8;

		sta = fibBuffer[readcnt++];
		sub_ch_info->start_address |= sta;

		sub_ch_info = get_subchannel_info(sub_ch_info->subchannel_id);
		if (sub_ch_info == NULL) {
			/* print_log(NULL, "subchannel_info error ..\n"); */
			return 1;
		}

		sta = fibBuffer[readcnt++];

		sub_ch_info->mod_type = (sta & 0xc0) >> 6;
		sub_ch_info->enc_type = (sta & 0x20) >> 5;
		sub_ch_info->intv_depth = (sta & 0x18) >> 3;
		sub_ch_info->pl = (sta & 0x04) >> 2;
		sub_ch_info->subch_size = (sta & 0x03) << 8;

		sta = fibBuffer[readcnt++];
		sub_ch_info->subch_size |= sta;
	}

	return result;
}

/*
 *  FIG 0/18 Announcement
 */
int fig0_ext18_decoder(u8 *fibBuffer, int figLength)
{
	u8	sta;
	int	result = 0;
	int	readcnt = 0;
	u16	sid;
	/* u8	CId; */
	u16	AsuFlag;
	int	nocs;
	int	i;

	while (figLength-1 > readcnt) {
		sta = fibBuffer[readcnt++];
		sid = sta << 8;
		sta = fibBuffer[readcnt++];
		sid |= sta;
		/* print_log(NULL, "sid = 0x%X, ", sid); */

		sta = fibBuffer[readcnt++];
		AsuFlag = sta << 8;
		sta = fibBuffer[readcnt++];
		AsuFlag |= sta;
		/* print_log(NULL, "AsuFlag = 0x%X, ", AsuFlag); */

		sta = fibBuffer[readcnt++];
		nocs = sta & 0x1F;
		/* print_log(NULL, "nocs = 0x%X, ", nocs); */

		for (i = 0; i < nocs; i++) {
			sta = fibBuffer[readcnt++];
			/* CId = sta; */
			/* print_log(NULL, "CId = %d, ", CId); */
		}
		/* print_log(NULL, "\n"); */
	}

	return result;
}

static int fig1_ext0_decoder(u8 *fibBuffer, int figLength)
{
	int	result = 0;
	int	readcnt = 0;
	int	i;

	u16 eid;
	u16 flag;

	eid = 0;
	eid = fibBuffer[readcnt++];
	eid = eid << 8 | fibBuffer[readcnt++];

	for (i = 0; i < 16; i++)
		ensemble_info[0].label[i] = fibBuffer[readcnt++];

	flag = 0;
	flag = fibBuffer[readcnt++];
	flag = flag << 8 | fibBuffer[readcnt++];

	ensemble_info[0].label[16] = '\0';
	ensemble_info[0].flag = 99;
	ensemble_info[0].eid  = eid;
	/*
	print_log(DMB_FIC_INFO"FIG 1/0 label [%x][%s]\n"
	, eid, ensemble_info[0].label);
	*/

	for (i = 16-1; i >= 0; i--) {
		if (ensemble_info[0].label[i] == 0x20)
			ensemble_info[0].label[i] = 0;
		else {
			if (i == 16-1)
				ensemble_info[0].label[i] = 0;
			break;
		}
	}

	return result;
}

static int fig1_ext1_decoder(u8 *fibBuffer, int figLength)
{
	struct service_info_t *svc_info;
	u32	temp;
	int	result = 0;
	int	readcnt = 0;
	int	i;

	u16 sid;

	temp = 0;
	temp = fibBuffer[readcnt++];
	temp = temp << 8 | fibBuffer[readcnt++];

	sid = temp;

	svc_info = get_service_info(sid);
	if (svc_info == NULL) {
		/* print_log(NULL, "get_service_info Error ...\n"); */
		return 1;
	}

	svc_info->sid = sid;

	svc_info->flag |= 0x01;

	for (i = 0; i < 16; i++)
		svc_info->label[i] = fibBuffer[readcnt++];

	svc_info->label[16] = '\0';
	/* print_log(NULL, "FIG 1/1 label [%x][%s]\n", sid, svc_info->label); */

	for (i = 16-1; i >= 0; i--) {
		if (svc_info->label[i] == 0x20)
			svc_info->label[i] = 0;
		else {
			if (i == 16-1)
				svc_info->label[i] = 0;
			break;
		}
	}

	/* print_log(NULL, "FIG 1/5 label [%x][%s]\n", sid, svc_info->label); */

	return result;
}

static int fig1_ext4_decoder(u8 *fibBuffer, int figLength)
{
	struct scInfo_t  *pScInfo;
	u8	sta;
	u8	pd;
	u32	temp;
	int		result = 0;
	int		readcnt = 0;
	int		i;

	u16		scid;
	/* u32		sid; */
	u16		flag;

	sta = fibBuffer[readcnt++];

	pd = (sta & 0x80) >> 7;
	scid = (sta & 0x0f);

	temp = 0;
	temp = fibBuffer[readcnt++];
	temp = temp << 8 | fibBuffer[readcnt++];

	if (pd) {
		temp = temp << 8 | fibBuffer[readcnt++];
		temp = temp << 8 | fibBuffer[readcnt++];
		/* sid = temp; */
	} else {
		/* sid = temp; */
	}

	pScInfo = get_sc_info(scid);
	if (pScInfo == NULL) {
		/* print_log(NULL, "get_service_info Error ...\n"); */
		return 1;
	}

	pScInfo->flag = 99;
	pScInfo->scid = scid;

	for (i = 0; i < 16; i++)
		pScInfo->label[i] = fibBuffer[readcnt++];

	flag = 0;
	flag = fibBuffer[readcnt++];
	flag = flag << 8 | fibBuffer[readcnt++];

	pScInfo->label[16] = '\0';
	/* print_log(NULL, "FIG 1/4 label [%x][%s]\n", sid, pScInfo->label); */

	for (i = 16-1; i >= 0; i--) {
		if (pScInfo->label[i] == 0x20)
			pScInfo->label[i] = 0;
		else {
			if (i == 16-1)
				pScInfo->label[i] = 0;
			break;
		}
	}

	/*print_log(NULL, "FIG 1/5 label [%x][%s]\n", sid, svc_info->label); */

	return result;
}

static int fig1_ext5_decoder(u8 *fibBuffer, int figLength)
{
	struct service_info_t *svc_info;
	u32	temp;
	int	result = 0;
	int	readcnt = 0;
	int	i;

	u32 sid;
	u16 flag;

	temp = 0;
	temp = fibBuffer[readcnt++];
	temp = temp << 8 | fibBuffer[readcnt++];
	temp = temp << 8 | fibBuffer[readcnt++];
	temp = temp << 8 | fibBuffer[readcnt++];

	sid = temp;

	svc_info = get_service_info(sid);
	if (svc_info == NULL) {
		/* print_log(NULL, "get_service_info Error ...\n"); */
		return 1;
	}

	svc_info->sid = sid;

	svc_info->flag |= 0x01;

	for (i = 0; i < 16; i++)
		svc_info->label[i] = fibBuffer[readcnt++];

	flag = 0;
	flag = fibBuffer[readcnt++];
	flag = flag << 8 | fibBuffer[readcnt++];

	svc_info->label[16] = '\0';

	for (i = 16-1; i >= 0; i--) {
		if (svc_info->label[i] == 0x20)
			svc_info->label[i] = 0;
		else {
			if (i == 16-1)
				svc_info->label[i] = 0;
			break;
		}
	}

	/* print_log(NULL, "FIG 1/5 label [%x][%s]\n", sid, svc_info->label); */

	return result;
}

void subchannel_org_prn(int subchannel_id)
{
	struct didp_info_t  didp;
	struct subch_info_t *sub_ch_info;

	memset(&didp, 0, sizeof(didp));

	sub_ch_info = get_subchannel_info(subchannel_id);
	if (sub_ch_info == NULL)
		return;

	if (sub_ch_info->flag == 99) {
		subchannel_org_to_didp(sub_ch_info, &didp);
		/*
		if (sub_ch_info->service_channel_id & 0x40)
			print_log(NULL, "service_channel_id = 0x%X, "
			, sub_ch_info->service_channel_id & 0x3F);
		else
			print_log(NULL, "service_channel_id = NOTUSE, ");
		*/
		switch (sub_ch_info->re_config)  {
		case 0:
			/* print_log(NULL, "re_config = INIT\n"); */
			break;
		case 1:
			/* print_log(NULL, "re_config = UPDATED\n"); */
			break;
		case 2:
			/* print_log(NULL, "re_config = DONE\n"); */
			break;
		}

		/* print_log(NULL, "sid = 0x%X\n", sub_ch_info->sid); */
		/*  didp_prn(&didp); */
	}
}

void subchannel_org_clean(void)
{
	int i;

	memset(ensemble_info, 0, sizeof(struct esbinfo_t) * MAX_ESB_NUM);
	memset(service_info, 0, sizeof(struct service_info_t) * MAX_SVC_NUM);
	memset(sc_info, 0, sizeof(struct scInfo_t) * MAX_SC_NUM);
	memset(subchannel_info, 0, sizeof(struct subch_info_t) * MAX_SUBCH_NUM);

	for (i = 0; i < MAX_SUBCH_NUM; i++)
		subchannel_info[i].flag = 0;

	for (i = 0; i < MAX_SVC_NUM; i++) {
		service_info[i].flag = 0;
		service_info[i].scid = 0xffff;
	}

	for (i = 0; i < MAX_SC_NUM; i++)
		sc_info[i].scid = 0xffff;

	return;
}

int bitrate_to_index(u16 bitrate)
{
	int index;

	switch (bitrate) {
	case 32:
		index =  0; break;
	case 48:
		index =  1; break;
	case 56:
		index =  2; break;
	case 64:
		index =  3; break;
	case 80:
		index =  4; break;
	case 96:
		index =  5; break;
	case 112:
		index =  6; break;
	case 128:
		index =  7; break;
	case 160:
		index =  8; break;
	case 192:
		index =  9; break;
	case 224:
		index =  10; break;
	case 256:
		index =  11; break;
	case 320:
		index =  12; break;
	case 384:
		index =  13; break;
	default:
		index =  -1; break;
	}

	return index;
}

int GetN(struct subch_info_t *sub_ch_info, int *n)
{
	int result = 0;

	switch (sub_ch_info->option) {
	case 0:
		switch (sub_ch_info->protect_level) {
		case 0:
			*n = sub_ch_info->subch_size / 12;
			break;
		case 1:
			*n = sub_ch_info->subch_size / 8;
			break;
		case 2:
			*n = sub_ch_info->subch_size / 6;
			break;
		case 3:
			*n = sub_ch_info->subch_size / 4;
			break;
		default:
			/*
			print_log(NULL, "Unknown Protection Level %d\n"
			, sub_ch_info->protect_level);
			*/
			result = 1;
			break;
		}
		break;
	case 1:
		switch (sub_ch_info->protect_level) {
		case 0:
			*n = sub_ch_info->subch_size / 27;
			break;
		case 1:
			*n = sub_ch_info->subch_size / 21;
			break;
		case 2:
			*n = sub_ch_info->subch_size / 18;
			break;
		case 3:
			*n = sub_ch_info->subch_size / 15;
			break;
		default:
			/*
			print_log(NULL, "Unknown Protection Level %d\n"
			, sub_ch_info->protect_level);
			*/
			result = 1;
			break;
		}
		break;
	default:
		/* print_log(NULL, "Unknown Option %d\n"
					, sub_ch_info->option); */
		result = 1;
		break;
	}

	return result;
}

int subchannel_org_to_didp(
	struct subch_info_t *sub_ch_info, struct didp_info_t *pdidp)
{
	int index, bitrate, level;
	int result = 0, n = 0;
	u16	subch_size = 0;
	u16	dataRate;
	u8  intv_depth = 0;

	if (sub_ch_info->flag != 99)
		return 1;

	switch (sub_ch_info->mode) {
	case 0:		/* T-DMB */
		pdidp->mode = sub_ch_info->mode;
		switch (sub_ch_info->form_type) {
		case 0:		/* short form  UEP */
			pdidp->subchannel_id = sub_ch_info->subchannel_id;
			pdidp->start_address = sub_ch_info->start_address;
			pdidp->form_type = sub_ch_info->form_type;
			subch_size =
				bitrate_profile[sub_ch_info->table_index][0];
			pdidp->speed =
				bitrate_profile[sub_ch_info->table_index][2];

			level = bitrate_profile[sub_ch_info->table_index][1];
			bitrate = bitrate_profile[sub_ch_info->table_index][2];
			index = bitrate_to_index(bitrate);

			if (index < 0) {
				result = 1;
				break;
			}

			pdidp->l1  = uep_profile[index][level-1][0];
			pdidp->l2  = uep_profile[index][level-1][1];
			pdidp->l3  = uep_profile[index][level-1][2];
			pdidp->l4  = uep_profile[index][level-1][3];
			pdidp->p1  = (u8)uep_profile[index][level-1][4];
			pdidp->p2  = (u8)uep_profile[index][level-1][5];
			pdidp->p3  = (u8)uep_profile[index][level-1][6];
			pdidp->p4  = (u8)uep_profile[index][level-1][7];
			pdidp->pad = (u8)uep_profile[index][level-1][8];
			break;
		case 1:		/* long form EEP */
			pdidp->subchannel_id = sub_ch_info->subchannel_id;
			pdidp->start_address = sub_ch_info->start_address;
			pdidp->form_type = sub_ch_info->form_type;
			subch_size = sub_ch_info->subch_size;
			pdidp->l3 = 0;
			pdidp->p3 = 0;
			pdidp->l4 = 0;
			pdidp->p4 = 0;
			pdidp->pad = 0;

			if (GetN(sub_ch_info, &n)) {
				result = 1;
				break;
			}

			switch (sub_ch_info->option) {
			case 0:
				switch (sub_ch_info->protect_level) {
				case 0:
					pdidp->l1 = 6*n - 3;
					pdidp->l2 = 3;
					pdidp->p1 = 24;
					pdidp->p2 = 23;
					break;
				case 1:
					if (n > 1) {
						pdidp->l1 = 2*n - 3;
						pdidp->l2 = 4*n + 3;
						pdidp->p1 = 14;
						pdidp->p2 = 13;
					} else {
						pdidp->l1 = 5;
						pdidp->l2 = 1;
						pdidp->p1 = 13;
						pdidp->p2 = 12;
					}
					break;
				case 2:
					pdidp->l1 = 6*n - 3;
					pdidp->l2 = 3;
					pdidp->p1 = 8;
					pdidp->p2 = 7;
					break;
				case 3:
					pdidp->l1 = 4*n - 3;
					pdidp->l2 = 2*n + 3;
					pdidp->p1 = 3;
					pdidp->p2 = 2;
					break;
				default:
					result = 1;
					break;
				}
				pdidp->speed = 8*n;
				break;
			case 1:
				switch (sub_ch_info->protect_level) {
				case 0:
					pdidp->l1 = 24*n - 3;
					pdidp->l2 = 3;
					pdidp->p1 = 10;
					pdidp->p2 = 9;
					break;
				case 1:
					pdidp->l1 = 24*n - 3;
					pdidp->l2 = 3;
					pdidp->p1 = 6;
					pdidp->p2 = 5;
					break;
				case 2:
					pdidp->l1 = 24*n - 3;
					pdidp->l2 = 3;
					pdidp->p1 = 4;
					pdidp->p2 = 3;
					break;
				case 3:
					pdidp->l1 = 24*n - 3;
					pdidp->l2 = 3;
					pdidp->p1 = 2;
					pdidp->p2 = 1;
					break;
				default:
					break;
				}
				pdidp->speed = 32*n;
				break;
			default:
				result = 1;
				break;
			}
			break;
		default:
			result = 1;
			break;
		}

		if (subch_size <= pdidp->subch_size)
			pdidp->reconfig_offset = 0;
		else
			pdidp->reconfig_offset = 1;

		pdidp->subch_size = subch_size;
		break;
	case 1:		/* T-MMB */
		pdidp->mode = sub_ch_info->mode;
		pdidp->start_address = sub_ch_info->start_address;
		pdidp->subchannel_id = sub_ch_info->subchannel_id;
		pdidp->subch_size = sub_ch_info->subch_size;
		pdidp->mod_type = sub_ch_info->mod_type;
		pdidp->enc_type = sub_ch_info->enc_type;
		pdidp->intv_depth = sub_ch_info->intv_depth;
		pdidp->pl = sub_ch_info->pl;

		switch (pdidp->mod_type) {
		case 0:
			n =  pdidp->subch_size / 18;
			break;
		case 1:
			n =  pdidp->subch_size / 12;
			break;
		case 2:
			n =  pdidp->subch_size / 9;
			break;
		default:
			result = 1;
			break;
		}

		switch (pdidp->intv_depth) {
		case 0:
			intv_depth = 16;
			break;
		case 1:
			intv_depth = 32;
			break;
		case 2:
			intv_depth = 64;
			break;
		default:
			result = 1;
			break;
		}

		if (result == 1)
			break;

		if (pdidp->pl) {
			dataRate = n * 32;
			pdidp->mi =
				(((((dataRate * 3) / 2) * 24) / intv_depth) * 3)
				/ 4;
		} else {
			dataRate = n * 24;
			pdidp->mi =
				((((dataRate * 2) * 24) / intv_depth) * 3) / 4;
		}
		break;
	default:
		break;
	}

	return result;
}

int found_all_labels(void)
{
	ms_wait(1200);
	return 1;
}
