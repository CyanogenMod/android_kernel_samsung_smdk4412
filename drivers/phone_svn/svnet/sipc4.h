/**
 * SAMSUNG MODEM IPC header version 4
 *
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __SAMSUNG_IPC_V4_H__
#define __SAMSUNG_IPC_V4_H__

/* IPC4.1 NEW PARTITION MAP
 * This map is seen by AP side

	0x00_0000       ===========================================
			MAGIC(4)| ACCESS(4)     |       RESERVED(8)
	0x00_0010       -------------------------------------------
			FMT_OUT_PTR             |       FMT_IN_PTR
			HEAD(4) | TAIL(4)       | HEAD(4) | TAIL(4)
	0x00_0020       -------------------------------------------
			RAW_OUT_PTR             |       RAW_IN_PTR
			HEAD(4) | TAIL(4)       | HEAD(4) | TAIL(4)
	0x00_0030       -------------------------------------------
			RFS_OUT_PTR             |       RFS_IN_PTR
			HEAD(4) | TAIL(4)       | HEAD(4) | TAIL(4)
	0x00_0040       -------------------------------------------
			RESERVED                        (4KB - 64B)
	0x00_1000       -------------------------------------------
			CP Fatal Display                (160B)
	0x00_10A0       -------------------------------------------
			RESERVED        (1MB - 4kb-4kb-4kb - 160B)
	0x0F_E000       -------------------------------------------
			Formatted Out                   (64KB)
	0x10_E000       -------------------------------------------
			Formatted In                    (64KB)
	0x11_E000       ===========================================
			Raw Out                         (1MB)
	0x21_E000       ===========================================
			Raw In                          (1MB)
	0x31_E000       ===========================================
			RemoteFS Out                    (1MB)
	0x41_E000       ===========================================
			RemoteFS In                     (1MB)
	0x51_E000       ===========================================

	0xFF_FFFF       ===========================================
*/

#if defined(CONFIG_PHONE_IPC_SPI)
#define FMT_OUT	0x0FE000
#define FMT_IN		0x10E000
#define FMT_SZ		0x10000   /* 65536 bytes */

#define RAW_OUT	0x11E000
#define RAW_IN		0x21E000
#define RAW_SZ		0x100000 /* 1 MB */

#define RFS_OUT	0x31E000
#define RFS_IN		0x41E000
#define RFS_SZ		0x100000 /* 1 MB */
#else
#if defined(CONFIG_PHONE_IPC_HSI)
#define FMT_OUT	0x0FE000
#define FMT_IN		0x10E000
#define FMT_SZ		0x10000   /* 65536 bytes */

#define RAW_OUT	0x11E000
#define RAW_IN		0x21E000
#define RAW_SZ		0x100000 /* 1 MB */

#define RFS_OUT	0x31E000
#define RFS_IN		0x41E000
#define RFS_SZ		0x100000 /* 1 MB */
#endif
#endif

#define FATAL_DISP     0x001000
#define FATAL_DISP_SZ  0xA0  /* 160 bytes */

#define SIPC_MAP_SIZE (RFS_IN + RFS_SZ)
#define SIPC_NAME "IPCv4.1"

enum {
	IPCIDX_FMT = 0,
	IPCIDX_RAW,
	IPCIDX_RFS,
	IPCIDX_MAX
};

struct ringbuf_cont {
	u32 out_head;
	u32 out_tail;
	u32 in_head;
	u32 in_tail;
};

struct sipc_mapped { /* map to the onedram start addr */
	u32 magic;
	u32 access;
	u32 reserved[2];

	struct ringbuf_cont rbcont[IPCIDX_MAX];
};


#define PN_CMD 0x00
#define PN_FMT 0x01
#define PN_RFS 0x41
#define PN_RAW(chid) (0x20 | (chid))
#define CHID(x) ((x) & 0x1F)

#define res_to_ridx(x) ((x) >> 5)

/*
 * IPC Frame Format
 */
#define HDLC_START	0x7F
#define HDLC_END	0x7E

/* Formatted IPC Frame */
struct fmt_hdr {
	u16 len;
	u8 control;
} __packed;

#define FMT_ID_MASK 0x7F /* Information ID mask */
#define FMT_ID_SIZE 0x80 /* = 128 ( 0 ~ 127 ) */
#define FMT_MB_MASK 0x80 /* More bit mask */

#define FMT_TX_MIN 5 /* ??? */

#define is_fmt_last(x) (!((x) & FMT_MB_MASK))

/* RAW IPC Frame */
struct raw_hdr {
	u32 len;
	u8 channel;
	u8 control;
} __packed;


/* RFS IPC Frame */
struct rfs_hdr {
	u32 len;
	u8 cmd;
	u8 id;
} __packed;

/*
 * RAW frame channel ID
 */
enum {
	CHID_0 = 0,
	CHID_CSD_VT_DATA,
	CHID_PDS_PVT_CONTROL,
	CHID_PDS_VT_AUDIO,
	CHID_PDS_VT_VIDEO,
	CHID_5,                /* 5 */
	CHID_6,
	CHID_CDMA_DATA,
	CHID_PCM_DATA,
	CHID_TRANSFER_SCREEN,
	CHID_PSD_DATA1,        /* 10 */
	CHID_PSD_DATA2,
	CHID_PSD_DATA3,
	CHID_PSD_DATA4,
	CHID_PSD_DATA5,
	CHID_PSD_DATA6,        /* 15 */
	CHID_PSD_DATA7,
	CHID_PSD_DATA8,
	CHID_PSD_DATA9,
	CHID_PSD_DATA10,
	CHID_PSD_DATA11,       /* 20 */
	CHID_PSD_DATA12,
	CHID_PSD_DATA13,
	CHID_PSD_DATA14,
	CHID_PSD_DATA15,
	CHID_BT_DUN,           /* 25 */
	CHID_CIQ_BRIDGE_DATA,
	CHID_27,
	CHID_CP_LOG1,
	CHID_CP_LOG2,
	CHID_30,               /* 30 */
	CHID_31,
	CHID_MAX
};

#define PDP_MAX 15
#define PN_PDP_START PN_RAW(CHID_PSD_DATA1)
#define PN_PDP_END PN_RAW(CHID_PSD_DATA15)

#define PN_PDP(chid) (0x20 | ((chid) + CHID_PSD_DATA1 - 1))
#define PDP_ID(res) ((res) - PN_PDP_START)


/*
 * IPC 4.0 Mailbox message definition
 */
#define MB_VALID          0x0080
#define MB_COMMAND        0x0040

#define MB_CMD(x)         (MB_VALID | MB_COMMAND | x)
#define MB_DATA(x)        (MB_VALID | x)

/*
 * If not command
 */
#define MBD_SEND_FMT      0x0002
#define MBD_SEND_RAW      0x0001
#define MBD_SEND_RFS      0x0100
#define MBD_REQ_ACK_FMT   0x0020
#define MBD_REQ_ACK_RAW   0x0010
#define MBD_REQ_ACK_RFS   0x0400
#define MBD_RES_ACK_FMT   0x0008
#define MBD_RES_ACK_RAW   0x0004
#define MBD_RES_ACK_RFS   0x0200

/*
 * If command
 */
enum {
	MBC_NONE = 0,
	MBC_INIT_START,
	MBC_INIT_END,
	MBC_REQ_ACTIVE,
	MBC_RES_ACTIVE,
	MBC_TIME_SYNC,
	MBC_POWER_OFF,
	MBC_RESET,
	MBC_PHONE_START,
	MBC_ERR_DISPLAY,
	MBC_POWER_SAVE,
	MBC_NV_REBUILD,
	MBC_EMER_DOWN,
	MBC_REQ_SEM,
	MBC_RES_SEM,
	MBC_MAX
};
#define MBC_MASK 0xFF

/* CMD_INIT_END extended bit */
#define CP_BOOT_ONLINE     0x0000
#define CP_BOOT_AIRPLANE   0x1000
#define AP_OS_ANDROID      0x0100
#define AP_OS_WINMOBILE    0x0200
#define AP_OS_LINUX        0x0300
#define AP_OS_SYMBIAN      0x0400

/* CMD_PHONE_START extended bit */
#define CP_QUALCOMM        0x0100
#define CP_INFINEON        0x0200
#define CP_BROADCOM        0x0300


#endif /* __SAMSUNG_IPC_V4_H__ */
