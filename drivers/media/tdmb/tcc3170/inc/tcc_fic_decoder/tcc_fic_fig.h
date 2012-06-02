/*
 * tcc_fic_fig.h
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

#ifndef __TCC_FIC_FIG_H__
#define __TCC_FIC_FIG_H__

#include "tcpal_os.h"
#include "tcpal_debug.h"

#define FICERR_FIG1_5_NOTREADY_SERVICE			11050
#define FICERR_FIG1_4_NOTREADY_SRVCOMP			11041
#define FICERR_FIG1_4_NOTREADY_SRVCOMP1			11040
#define FICERR_FIG1_1_NOTREADY_SERVICE			11010
#define FICERR_FIG0_13_ALREADY_USERAPPL			10130
#define FICERR_FIG0_7_BLOCK				10071
#define FICERR_FIG0_7_RETURN				10070
#define FICERR_FIG0_5_NOTREADY_SERVICE			10051
#define FICERR_FIG0_5_NOTREADY_SRVCOMP			10050
#define FICERR_FIG0_4_NOTREADY_SRVCOMP			10041
#define FICERR_FIG0_4_ALREADY_CA_FIELD			10040
#define FICERR_FIG0_3_NOTREADY_SRVCOMP			10032
#define FICERR_FIG0_3_NOTREADY_SRVCOMP1			10031
#define FICERR_FIG0_3_ALREADY_SRVCOMP			10030
#define FICERR_FIG_NODATA				2
#define FICERR_FIBD_ENDMARKER				1
#define FICERR_SUCCESS					0

#define FICERR_FIBD_FICSYNC_FAILURE			-1
#define FICERR_FIBD_CRC_FAILURE				-2
#define FICERR_FIBD_UNKNOWN_FIGTYPE			-3
#define FICERR_FIBD_INVALID_LENGTH			-4
#define FICERR_FIG0_NEXT_FIG				-1000
#define FICERR_FIG0_NEXT_FIG1				-1001
#define FICERR_FIG0_0_NO_ENSEMBLEARRAY			-10000
#define FICERR_FIG0_1_FULL_SUBCHARRAY			-10010
#define FICERR_FIG0_1_INVALID_LENGTH			-10011
#define FICERR_FIG0_1_NO_SUBCHARRAY			-10012
#define FICERR_FIG0_2_FULL_SERVICEARRAY			-10020
#define FICERR_FIG0_2_FULL_SRVCOMPARRAY			-10021
#define FICERR_FIG0_2_INVALID_LENGTH			-10022
#define FICERR_FIG0_2_NO_SRVARRAY			-10023
#define FICERR_FIG0_2_NO_SRVCOMPARRAY			-10024
#define FICERR_FIG0_3_INVALID_LENGTH			-10030
#define FICERR_FIG0_3_NO_SRVCOMPARRAY			-10031
#define FICERR_FIG0_4_INVALID_LENGTH			-10040
#define FICERR_FIG0_4_NO_SRVCOMPARRAY			-10041
#define FICERR_FIG0_5_INVALID_LENGTH			-10050
#define FICERR_FIG0_5_NO_SRVCOMPARRAY			-10051
#define FICERR_FIG0_8_INVALID_LENGTH			-10080
#define FICERR_FIG0_8_NO_SRVCOMPARRAY			-10081
#define FICERR_FIG0_13_FULL_USERAPPLARRAY		-10130
#define FICERR_FIG0_13_INVALID_LENGTH			-10131
#define FICERR_FIG0_13_NO_USERAPPLARRAY			-10132
#define FICERR_FIG0_17_OTHER_ENSEMBLE			-10170
#define FICERR_FIG0_17_NO_PROGTYPEARRAY			-10171
#define FICERR_FIG0_17_FULL_PROGTYPEARRAY		-10172
#define FICERR_FIG0_17_INVALID_LENGTH			-10073
#define FICERR_FIG1_1_NO_SERVICEARRAY			-11011
#define FICERR_FIG1_4_NO_SRVCOMPARRAY			-11041
#define FICERR_FIG1_5_NO_SERVICEARRAY			-11052
#define FICERR_FIG1_6_NO_XPADLABELARRAY			-11061

#define		FIG0			0x0
#define		FIG1			0x1

#define		EXT_00		0
#define		EXT_01		1
#define		EXT_02		2
#define		EXT_03		3
#define		EXT_04		4
#define		EXT_05		5
#define		EXT_06		6
#define		EXT_07		7
#define		EXT_08		8
#define		EXT_09		9
#define		EXT_10		10
#define		EXT_11		11
#define		EXT_12		12
#define		EXT_13		13
#define		EXT_14		14
#define		EXT_15		15
#define		EXT_16		16
#define		EXT_17		17
#define		EXT_18		18
#define		EXT_19		19
#define		EXT_20		20
#define		EXT_21		21
#define		EXT_22		22
#define		EXT_23		23
#define		EXT_24		24
#define		EXT_25		25
#define		EXT_26		26
#define		EXT_27		27
#define		EXT_28		28
#define		EXT_29		29
#define		EXT_30		30
#define		EXT_31		31

#define	INITVAL_SCIDS	0xff

#define	NUM_SVC		64 /**< max num of struct tcc_service */
#define	NUM_SUB_CH	(NUM_SVC + 0) /**< max num of struct tcc_sub_channel */
#define	NUM_SVC_COMP	NUM_SUB_CH /**< max num of struct tcc_service_comp */
#define	NUM_PRG_TYPE	NUM_SVC_COMP /**< max_num of struct tcc_program_type. */
#define	NUM_USER_APP	NUM_SVC /**< max_num of struct tcc_user_app_type. */

/**FIG 0/1 */
struct tcc_sub_channel {
	u8	subch_id; /**< 6bits Sub channel Id */
	u8	tbl_index; /**< 6bits TableIndex */
	u8	form_flag; /**< [3] : FormFlag@n
				*   [2] : Option@n
				*   [1~0] : protection */
	u16	start_cu; /**< 10bits Start Address */
	u16	size_cu; /**< 10bits Sub channel size */
};

/** FIG 0/2 and FIG1 */
struct tcc_service_comp {
	u8	order; /**< 4bits 0 : primary, 1: secondary */
	u8	tmid; /**< 2bits Transport Mechanism Id */
	u8	ascty_dscty; /**< 6bits	Audio Service Component Type */
	u8	fidc_id; /**< 6bits subch_id or FIDCId in FIG 0/4 */
	u8	ca_flag; /**< 1bit CA Flag */
	u8	dg_mf_flag; /**< 1bit DG Flag or MF flag */
	u8	lang; /**< 8bit language field of FIG 0/5 */
	u8	scids; /**< 4bit Service component Identifier
			*   within ther Service */
	u8	ca_org_flag; /**< 1bit */
	u8	charset; /**< character set */
	u8	label[16]; /**< 16bytes	Service component label */
	u16	scid; /**< 12bits Service Component Id */
	u16	pack_add; /**< 10bits Packet Address */
	u16	ca_org; /**< 16bits conditional access organization */
	u16	char_flag; /**< refer to ETSI EN 300 401 5.2.2.1 */
	u32	sid; /**< 32bit */
};


/** FIG 0/17 */
struct tcc_program_type {
	u8	sd; /**< 1bit */
	u8	ps; /**< 1bit */
	u8	nfc; /**< 2bit */
	u8	lang; /**< 8Bit */
	u8	i18n_code; /**< 5bit */
	u8	coarse_code; /**< 6bit */
	u8	fine_code; /**< 8bit */
	u16	sid; /**< 16bit */
};


/** FIG 0/2	and FIG1/1 */
struct tcc_service {
	u32	sid; /**< 32bits CountryId + serviceReference
			  *   ECC + CountryId + ServiceReference */
	u8	charset; /**< character set */
	u8	svc_label[16];	/**< 16bytes Service label
				 *   (Program service and Data service) */
	u16	char_flag; /**< refer to ETSI EN 300 401 5.2.2.1 */
	u8	ca_id; /**< 3bit */
	u8	num_svc_comp; /**< 4bits Number of Service Component */
};


/** FIG 0/0 */
struct tcc_ensemble {
	u8	al_flag; /**< 1bit Al flag */
	u8	num_subch; /**< a number of struct tcc_sub_channel */
	u8	num_svc; /**< a number of struct tcc_service */
	u8	num_program; /**< a number of ProgNumberInfo */
	u8	num_svc_comp; /**< a number of struct tcc_service_comp */
	u8	num_user_app; /**< a number of FIG0/13 */
	u8	num_ann; /**< a number of FIG0/18 */
	u8	num_prg_type; /**< a number of FIG0/17 */
	u8	num_oe_svc; /**< a number of FIG0/24 other ensemble*/
	u8	num_fi; /**< a number of FIG0/21 */
	u8	num_oe_fi; /**< a number of FIG0/21 other ensemble*/
	u8	charset; /**< character set */
	u8	label[16]; /**< 16bytes	Ensemble label */
	u16	char_flag; /**< refer to ETSI EN 300 401 5.2.2.1 */
	u16	eid; /**< 16bits country Id Ensemble reference */
};

struct tcc_user_app_type {
	u16	type; /**< User application Type */
	u8	len; /**< User Application Type length */
	u8	data[24]; /**< User Application Data */
};

/** FIG 0/13 */
struct tcc_user_app_types {
	u32	sid; /**< Service ID */
	u8	scids; /**< scids */
	u8	num_app; /**< appl */
	struct tcc_user_app_type app_type[6];
};

/** FIG 1/6 */
struct tcc_xpad_user_app {
	u8	charset; /**< character set */
	u8	label[16]; /**< label */
	u16	type; /**< X-PAD application type */
	u32	sid; /**< Service ID */
	u8	scids; /**< scids */
	u16	 char_flag;
};

#endif /* __TCC_FIC_FIG_H__ */
