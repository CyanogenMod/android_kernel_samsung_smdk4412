/*
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SIPC_DEF_H__
#define __SIPC_DEF_H__

/* definition of SIPC 4.x *****************************************************/
struct fmt_hdr {
	u16 len;
	u8 control;
} __packed;

struct raw_hdr {
	u32 len;
	u8 channel;
	u8 control;
} __packed;

struct rfs_hdr {
	u32 len;
	u8 cmd;
	u8 id;
} __packed;

struct sipc4_hdlc_start {
	u8 hdlc_start;
} __packed;

struct sipc4_hdlc_end {
	u8 hdlc_end;
} __packed;

union sipc4_hdr {
	struct fmt_hdr fmt;
	struct raw_hdr raw;
	struct rfs_hdr rfs;
} __packed;

#define HDLC_START		0x7F
#define HDLC_END		0x7E
#define SIZE_OF_HDLC_START	sizeof(struct sipc4_hdlc_start)
#define SIZE_OF_HDLC_END	sizeof(struct sipc4_hdlc_end)
#define SIPC4_HDR_LEN_MAX	sizeof(union sipc4_hdr)
/* If iod->id is 0, do not need to store to `iodevs_tree_fmt' in SIPC4 */
#define sipc4_is_not_reserved_channel(ch) ((ch) != 0)

/* definition of SIPC 5.0 *****************************************************/
struct sipc5_normal_hdr {
	u8 cfg;
	u8 ch;
	u16 len;
} __packed;

struct sipc5_ctl_hdr {
	u8 cfg;
	u8 ch;
	u16 len;
	u8 ctl;
} __packed;

struct sipc5_ext_hdr {
	u8 cfg;
	u8 ch;
	u32 len;
} __packed;

union sipc5_hdr {
	struct sipc5_normal_hdr nor;
	struct sipc5_ctl_hdr ctl;
	struct sipc5_ext_hdr ext;
} __packed;

struct sipc5_link_hdr {
	u8 cfg;
	u8 ch;
	u16 len;
	union {
		u8 ctl;
		u16 ext_len;
	} ext;
} __packed;

#define SIPC5_HDR_LEN		sizeof(struct sipc5_normal_hdr)
#define SIPC5_HDR_LEN_CTRL	sizeof(struct sipc5_ctl_hdr)
#define SIPC5_HDR_LEN_EXT	sizeof(struct sipc5_ext_hdr)
#define SIPC5_HDR_LEN_MAX	sizeof(union sipc5_hdr)

#define SIPC5_HDR_PAD		(0b100)
#define SIPC5_HDR_EXT		(0b010)
#define SIPC5_HDR_CTRL		(0b001)
#define SIPC5_HDR_CONTROL	(SIPC5_HDR_EXT | SIPC5_HDR_CTRL)

#define SIPC5_HDR_CFG_START	0xF8
#define SIPC5_HDR_CFG_MASK	(SIPC5_HDR_PAD | SIPC5_HDR_EXT | SIPC5_HDR_CTRL)
#define SIPC5_HDR_EXT_MASK	(SIPC5_HDR_EXT | SIPC5_HDR_CTRL)
/* Channel 0, 5, 6, 27, 255 are reserved in SIPC5.
 * see SIPC5 spec: 2.2.2 Channel Identification (Ch ID) Field.
 * They do not need to store in `iodevs_tree_fmt' */
#define sipc5_is_not_reserved_channel(ch) \
	((ch) != 0 && (ch) != 5 && (ch) != 6 && (ch) != 27 && (ch) != 255)

/* SIPC common definitions ****************************************************/
union sipc_all_hdr {
	struct fmt_hdr fmt;
	struct raw_hdr raw;
	struct rfs_hdr rfs;
	struct sipc5_normal_hdr nor;
	struct sipc5_ctl_hdr ctl;
	struct sipc5_ext_hdr ext;
} __packed;

enum sipc_ver {
	SIPC_VER_NONE = 0,
	SIPC_VER_40 = 40,
	SIPC_VER_41,
	SIPC_VER_42,
	SIPC_VER_50 = 50,
	MAX_SIPC_VER = SIPC_VER_50,
};
#define SIPC_VER_4X_MASK	(SIPC_VER_40)
#define SIPC_VER_5X_MASK	(SIPC_VER_50)

enum sipc_ch_id {
	SIPC_CH_ID_RAW_0 = 0,	/*reserved*/
	SIPC_CH_ID_CS_VT_DATA,
	SIPC_CH_ID_CS_VT_CONTROL,
	SIPC_CH_ID_CS_VT_AUDIO,
	SIPC_CH_ID_CS_VT_VIDEO,
	SIPC_CH_ID_RAW_5,	/*reserved*/
	SIPC_CH_ID_RAW_6,	/*reserved*/
	SIPC_CH_ID_CDMA_DATA,
	SIPC_CH_ID_PCM_DATA,
	SIPC_CH_ID_TRANSFER_SCREEN,

	SIPC_CH_ID_PDP_0,	/*ID:10*/
	SIPC_CH_ID_PDP_1,
	SIPC_CH_ID_PDP_2,
	SIPC_CH_ID_PDP_3,
	SIPC_CH_ID_PDP_4,
	SIPC_CH_ID_PDP_5,
	SIPC_CH_ID_PDP_6,
	SIPC_CH_ID_PDP_7,
	SIPC_CH_ID_PDP_8,
	SIPC_CH_ID_PDP_9,
	SIPC_CH_ID_PDP_10,
	SIPC_CH_ID_PDP_11,
	SIPC_CH_ID_PDP_12,
	SIPC_CH_ID_PDP_13,
	SIPC_CH_ID_PDP_14,
	SIPC_CH_ID_BT_DUN,	/*ID:25*/
	SIPC_CH_ID_CIQ_DATA,
	SIPC_CH_ID_PDP_18,	/*reserved*/
	SIPC_CH_ID_CPLOG1,	/*ID:28*/
	SIPC_CH_ID_CPLOG2,	/*ID:29*/
	SIPC_CH_ID_LOOPBACK1,
	SIPC_CH_ID_LOOPBACK2,
				/*32~234 was reserved*/
	SIPC5_CH_ID_FMT_0 = 235,
	SIPC5_CH_ID_FMT_1,
	SIPC5_CH_ID_FMT_2,
	SIPC5_CH_ID_FMT_3,
	SIPC5_CH_ID_FMT_4,
	SIPC5_CH_ID_FMT_5,
	SIPC5_CH_ID_FMT_6,
	SIPC5_CH_ID_FMT_7,
	SIPC5_CH_ID_FMT_8,
	SIPC5_CH_ID_FMT_9,

	SIPC5_CH_ID_RFS_0,
	SIPC5_CH_ID_RFS_1,
	SIPC5_CH_ID_RFS_2,
	SIPC5_CH_ID_RFS_3,
	SIPC5_CH_ID_RFS_4,
	SIPC5_CH_ID_RFS_5,
	SIPC5_CH_ID_RFS_6,
	SIPC5_CH_ID_RFS_7,
	SIPC5_CH_ID_RFS_8,
	SIPC5_CH_ID_RFS_9,
	SIPC5_CH_ID_MAX	= SIPC5_CH_ID_RFS_9,
};

struct sipc_main_hdr {
	u16 len;
	u8  msg_seq;
	u8  ack_seq;
	u8  main_cmd;
	u8  sub_cmd;
	u8  cmd_type;
} __packed;

#define SIPC_HDR_LEN_MAX	sizeof(union sipc_all_hdr)
#define SIPC_MULTIFMT_LEN	2048
#define SIPC_MULTIFMT_ID_MAX	0x7F
#define SIPC_MULTIFMT_MOREBIT	0x80

#define SIPC_ALIGN_UNIT		sizeof(unsigned)
#define SIPC_ALIGN_PAD_MAX	(SIPC_ALIGN_UNIT - 1)
#define SIPC_ALIGN(x) ALIGN(x, SIPC_ALIGN_UNIT)
#define SIPC_PADLEN(x) (ALIGN(x, SIPC_ALIGN_UNIT) - x)

/* Compatibility with legacy driver codes *************************************/
#define SIPC4_CHID_FMT		(0x0 << 0x5)
#define SIPC4_CHID_RAW		(0x1 << 0x5)
#define SIPC5_CHID_RFS		(0x2 << 0x5)

#define PSD_DATA_CHID_BEGIN	(SIPC4_CHID_RAW | SIPC_CH_ID_PDP_0)
#define PSD_DATA_CHID_END	(SIPC4_CHID_RAW | SIPC_CH_ID_PDP_18)
#define PS_DATA_CH_0		(SIPC_CH_ID_PDP_0)
#define PS_DATA_CH_LAST		(SIPC_CH_ID_PDP_18)
#define CP2AP_LOOPBACK_CHANNEL	(SIPC_CH_ID_LOOPBACK1) /* CP->AP*/
#define DATA_LOOPBACK_CHANNEL	(SIPC_CH_ID_LOOPBACK2)
#define RMNET0_CH_ID		(SIPC_CH_ID_PDP_0)
#define MAX_LINK_PADDING_SIZE	(SIPC_ALIGN_PAD_MAX)
#define NO_SIPC_VER		(SIPC_VER_NONE)

#endif
