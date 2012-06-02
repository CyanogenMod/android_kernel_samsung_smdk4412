/****************************************************************************
*  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
*
*  [File Name]   : s3c-otg-common-regdef.h
*  [Description] :
*
*  [Author]      : Kyu Hyeok Jang { kyuhyeok.jang@samsung.com }
*  [Department]  : System LSI Division/Embedded Software Center
*  [Created Date]: 2007/12/15
*  [Revision History]
*      (1) 2007/12/15   by Kyu Hyeok Jang { kyuhyeok.jang@samsung.com }
*          - Created
*
****************************************************************************/
/****************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ****************************************************************************/

#ifndef _OTG_REG_DEF_H
#define _OTG_REG_DEF_H


#define GOTGCTL		0x000 /* OTG Control & Status */
#define GOTGINT		0x004 /* OTG Interrupt */
#define GAHBCFG		0x008 /* Core AHB Configuration */
#define GUSBCFG		0x00C /* Core USB Configuration */
#define GRSTCTL		0x010 /* Core Reset */
#define GINTSTS		0x014 /* Core Interrupt */
#define GINTMSK		0x018 /* Core Interrupt Mask */
#define GRXSTSR		0x01C /* Receive Status Debug Read/Status Read */
#define GRXSTSP		0x020 /* Receive Status Debug Pop/Status Pop */
#define GRXFSIZ		0x024 /* Receive FIFO Size */
#define GNPTXFSIZ	0x028 /* Non-Periodic Transmit FIFO Size */
#define GNPTXSTS	0x02C /* Non-Periodic Transmit FIFO/Queue Status */
#define GPVNDCTL	0x034 /* PHY Vendor Control */
#define GGPIO		0x038 /* General Purpose I/O */
#define GUID		0x03C /* User ID */
#define GSNPSID		0x040 /* Synopsys ID */
#define GHWCFG1		0x044 /* User HW Config1 */
#define GHWCFG2		0x048 /* User HW Config2 */
#define GHWCFG3		0x04C /* User HW Config3 */
#define GHWCFG4		0x050 /* User HW Config4 */

#define HPTXFSIZ	0x100 /* Host Periodic Transmit FIFO Size */
#define DPTXFSIZ1	0x104 /* Device Periodic Transmit FIFO-1 Size */
#define DPTXFSIZ2	0x108 /* Device Periodic Transmit FIFO-2 Size */
#define DPTXFSIZ3	0x10C /* Device Periodic Transmit FIFO-3 Size */
#define DPTXFSIZ4	0x110 /* Device Periodic Transmit FIFO-4 Size */
#define DPTXFSIZ5	0x114 /* Device Periodic Transmit FIFO-5 Size */
#define DPTXFSIZ6	0x118 /* Device Periodic Transmit FIFO-6 Size */
#define DPTXFSIZ7	0x11C /* Device Periodic Transmit FIFO-7 Size */
#define DPTXFSIZ8	0x120 /* Device Periodic Transmit FIFO-8 Size */
#define DPTXFSIZ9	0x124 /* Device Periodic Transmit FIFO-9 Size */
#define DPTXFSIZ10	0x128 /* Device Periodic Transmit FIFO-10 Size */
#define DPTXFSIZ11	0x12C /* Device Periodic Transmit FIFO-11 Size */
#define DPTXFSIZ12	0x130 /* Device Periodic Transmit FIFO-12 Size */
#define DPTXFSIZ13	0x134 /* Device Periodic Transmit FIFO-13 Size */
#define DPTXFSIZ14	0x138 /* Device Periodic Transmit FIFO-14 Size */
#define DPTXFSIZ15	0x13C /* Device Periodic Transmit FIFO-15 Size */

/*********************************************************************
 * Host Mode Registers
 *********************************************************************/
/* Host Global Registers */

/* Channel specific registers */
#define HCCHAR_ADDR			(0x500)
#define HCCHAR(n)			(0x500 + ((n)*0x20))
#define HCSPLT(n)			(0x504 + ((n)*0x20))
#define HCINT(n)			(0x508 + ((n)*0x20))
#define HCINTMSK(n)			(0x50C + ((n)*0x20))
#define HCTSIZ(n)			(0x510 + ((n)*0x20))
#define HCDMA(n)			(0x514 + ((n)*0x20))

#define HCFG		0x400 /* Host Configuration */
#define HFIR		0x404 /* Host Frame Interval */
#define HFNUM		0x408 /* Host Frame Number/Frame Time Remaining */
#define HPTXSTS		0x410 /* Host Periodic Transmit FIFO/Queue Status */
#define HAINT		0x414 /* Host All Channels Interrupt */
#define HAINTMSK	0x418 /* Host All Channels Interrupt Mask */

/* Host Port Control & Status Registers */
#define HPRT 0x440 /* Host Port Control & Status */

#define EP_FIFO(n)	(0x1000 + ((n)*0x1000))

#define PCGCCTL		0x0E00

#define BASE_REGISTER_OFFSET	0x0
#define REGISTER_SET_SIZE		0x200

/* Power Reg Bits */
#define USB_RESET					0x8
#define MCU_RESUME					0x4
#define SUSPEND_MODE				0x2
#define SUSPEND_MODE_ENABLE_CTRL	0x1

/* EP0 CSR */
#define EP0_OUT_PACKET_RDY		0x1
#define EP0_IN_PACKET_RDY		0x2
#define EP0_SENT_STALL			0x4
#define DATA_END				0x8
#define SETUP_END				0x10
#define EP0_SEND_STALL			0x20
#define SERVICED_OUT_PKY_RDY	0x40
#define SERVICED_SETUP_END		0x80

/* IN_CSR1_REG Bit definitions */
#define IN_PACKET_READY			0x1
#define UNDER_RUN				0x4   /* Iso Mode Only */
#define FLUSH_IN_FIFO			0x8
#define IN_SEND_STALL			0x10
#define IN_SENT_STALL			0x20
#define IN_CLR_DATA_TOGGLE		0x40

/* OUT_CSR1_REG Bit definitions */
#define OUT_PACKET_READY		0x1
#define FLUSH_OUT_FIFO			0x10
#define OUT_SEND_STALL			0x20
#define OUT_SENT_STALL			0x40
#define OUT_CLR_DATA_TOGGLE		0x80

/* IN_CSR2_REG Bit definitions */
#define IN_DMA_INT_DISABLE		0x10
#define SET_MODE_IN				0x20

#define EPTYPE					(0x3<<18)
#define SET_TYPE_CONTROL		(0x0<<18)
#define SET_TYPE_ISO			(0x1<<18)
#define SET_TYPE_BULK			(0x2<<18)
#define SET_TYPE_INTERRUPT		(0x3<<18)

#define AUTO_MODE				0x80

/* OUT_CSR2_REG Bit definitions */
#define AUTO_CLR				0x40
#define OUT_DMA_INT_DISABLE		0x20

/* Can be used for Interrupt and Interrupt Enable Reg - common bit def */
#define EP0_IN_INT				(0x1<<0)
#define EP1_IN_INT				(0x1<<1)
#define EP2_IN_INT				(0x1<<2)
#define EP3_IN_INT				(0x1<<3)
#define EP4_IN_INT				(0x1<<4)
#define EP5_IN_INT				(0x1<<5)
#define EP6_IN_INT				(0x1<<6)
#define EP7_IN_INT				(0x1<<7)
#define EP8_IN_INT				(0x1<<8)
#define EP9_IN_INT				(0x1<<9)
#define EP10_IN_INT				(0x1<<10)
#define EP11_IN_INT				(0x1<<11)
#define EP12_IN_INT				(0x1<<12)
#define EP13_IN_INT				(0x1<<13)
#define EP14_IN_INT				(0x1<<14)
#define EP15_IN_INT				(0x1<<15)
#define EP0_OUT_INT				(0x1<<16)
#define EP1_OUT_INT				(0x1<<17)
#define EP2_OUT_INT				(0x1<<18)
#define EP3_OUT_INT				(0x1<<19)
#define EP4_OUT_INT				(0x1<<20)
#define EP5_OUT_INT				(0x1<<21)
#define EP6_OUT_INT				(0x1<<22)
#define EP7_OUT_INT				(0x1<<23)
#define EP8_OUT_INT				(0x1<<24)
#define EP9_OUT_INT				(0x1<<25)
#define EP10_OUT_INT			(0x1<<26)
#define EP11_OUT_INT			(0x1<<27)
#define EP12_OUT_INT			(0x1<<28)
#define EP13_OUT_INT			(0x1<<29)
#define EP14_OUT_INT			(0x1<<30)
#define EP15_OUT_INT			(0x1<<31)

/* GOTGINT */
#define SesEndDet				(0x1<<2)

/* GRSTCTL */
#define TxFFlsh					(0x1<<5)
#define RxFFlsh					(0x1<<4)
#define INTknQFlsh				(0x1<<3)
#define FrmCntrRst				(0x1<<2)
#define HSftRst					(0x1<<1)
#define CSftRst					(0x1<<0)

#define	CLEAR_ALL_EP_INTRS			0xffffffff

/* Bits to write to EP_INT_EN_REG - Use CLEAR */
#define	EP_INTERRUPT_DISABLE_ALL	0x0

/* DMA control register bit definitions */
#define RUN_OB						0x80
#define STATE						0x70
#define DEMAND_MODE					0x8
#define OUT_DMA_RUN					0x4
#define IN_DMA_RUN					0x2
#define DMA_MODE_EN					0x1


#define REAL_PHYSICAL_ADDR_EP0_FIFO	(0x520001c0) /*Endpoint 0 FIFO */
#define REAL_PHYSICAL_ADDR_EP1_FIFO	(0x520001c4) /*Endpoint 1 FIFO */
#define REAL_PHYSICAL_ADDR_EP2_FIFO	(0x520001c8) /*Endpoint 2 FIFO */
#define REAL_PHYSICAL_ADDR_EP3_FIFO	(0x520001cc) /*Endpoint 3 FIFO */
#define REAL_PHYSICAL_ADDR_EP4_FIFO	(0x520001d0) /*Endpoint 4 FIFO */

/* GAHBCFG */
#define MODE_DMA				(1<<5)
#define MODE_SLAVE				(0<<5)
#define BURST_SINGLE			(0<<1)
#define BURST_INCR				(1<<1)
#define BURST_INCR4				(3<<1)
#define BURST_INCR8				(5<<1)
#define BURST_INCR16			(7<<1)
#define GBL_INT_MASK			(0<<0)
#define GBL_INT_UNMASK			(1<<0)


/*********************************************************************
 * Global CSR
 *********************************************************************/

/* GAHBCFG
 * OTG AHB Configuration Register
 *  p1359
 *  d201
 */
typedef union _gahbcfg_t {
	/** raw register data */
	u32 d32;
	/** register bits */
	struct {
		unsigned glblintrmsk:1;
#define GAHBCFG_GLBINT_ENABLE				1
		unsigned hburstlen:4;
#define INT_DMA_MODE_SINGLE					0
#define INT_DMA_MODE_INCR					1
#define INT_DMA_MODE_INCR4					3
#define INT_DMA_MODE_INCR8					5
#define INT_DMA_MODE_INCR16					7
		unsigned dmaenable:1;
#define GAHBCFG_DMAENABLE					1
		unsigned reserved:1;
		unsigned nptxfemplvl:1;
		unsigned ptxfemplvl:1;
#define GAHBCFG_TXFEMPTYLVL_EMPTY			1
#define GAHBCFG_TXFEMPTYLVL_HALFEMPTY		0
		unsigned reserved9_31:22;
	} b;
} gahbcfg_t;


/* GUSBCFG
 * OTG USB Configuration Register
 * p1360
 * d202
 */
typedef union _gusbcfg_t {
	/** raw register data */
	u32 d32;
	/** register bits */
	struct {
		unsigned toutcal:3;
		unsigned phyif:1;
		unsigned ulpi_utmi_sel:1;
		unsigned fsintf:1;
		unsigned physel:1;
		unsigned ddrsel:1;
		unsigned srpcap:1;
		unsigned hnpcap:1;
		unsigned usbtrdtim:4;
		unsigned nptxfrwnden:1;
		unsigned phylpwrclksel:1;
		unsigned reserved:13;
		unsigned forcehstmode:1;
		unsigned reserved2:2;
	} b;
} gusbcfg_t;


/* GRSTCTL
 * Core Reset Register
 * p1362
 * d208
 */
typedef union grstctl_t {
	/** raw register data */
	u32 d32;
	/** register bits */
	struct {
		unsigned csftrst:1;
		unsigned hsftrst:1;
		unsigned hstfrm:1;
		unsigned intknqflsh:1;
		unsigned rxfflsh:1;
		unsigned txfflsh:1;
		unsigned txfnum:5;
		unsigned reserved11_29:19;
		unsigned dmareq:1;
		unsigned ahbidle:1;
	} b;
} grstctl_t;


/* GINTSTS
 * Core Interrupt Register
 * p1364
 * d212
 */
typedef union _gintsts_t {
	/** raw register data */
	u32 d32;
#define SOF_INTR_MASK 0x0008
	/** register bits */
	struct {
#define HOST_MODE			1
#define DEVICE_MODE		0
		unsigned curmode:1;
#define	OTG_HOST_MODE		1
#define	OTG_DEVICE_MODE	0

		unsigned modemismatch:1;
		unsigned otgintr:1;
		unsigned sofintr:1;
		unsigned rxstsqlvl:1;
		unsigned nptxfempty:1;
		unsigned ginnakeff:1;
		unsigned goutnakeff:1;
		unsigned reserved8:1;
		unsigned i2cintr:1;
		unsigned erlysuspend:1;
		unsigned usbsuspend:1;
		unsigned usbreset:1;
		unsigned enumdone:1;
		unsigned isooutdrop:1;
		unsigned eopframe:1;
		unsigned intokenrx:1;
		unsigned epmismatch:1;
		unsigned inepint:1;
		unsigned outepintr:1;
		unsigned incompisoin:1;
		unsigned incompisoout:1;
		unsigned reserved22_23:2;
		unsigned portintr:1;
		unsigned hcintr:1;
		unsigned ptxfempty:1;
		unsigned reserved27:1;
		unsigned conidstschng:1;
		unsigned disconnect:1;
		unsigned sessreqintr:1;
		unsigned wkupintr:1;
	} b;
} gintsts_t;


/* GINTMSK
 * Core Interrupt Mask Register
 * p1369
 * d217
 */
typedef union _gintmsk_t {
	/** raw register data */
	u32 d32;
	/** register bits */
	struct {
		unsigned reserved0:1;
		unsigned modemismatch:1;
		unsigned otgintr:1;
		unsigned sofintr:1;
		unsigned rxstsqlvl:1;
		unsigned nptxfempty:1;
		unsigned ginnakeff:1;
		unsigned goutnakeff:1;
		unsigned reserved8:1;
		unsigned i2cintr:1;
		unsigned erlysuspend:1;
		unsigned usbsuspend:1;
		unsigned usbreset:1;
		unsigned enumdone:1;
		unsigned isooutdrop:1;
		unsigned eopframe:1;
		unsigned reserved16:1;
		unsigned epmismatch:1;
		unsigned inepintr:1;
		unsigned outepintr:1;
		unsigned incompisoin:1;
		unsigned incompisoout:1;
		unsigned reserved22_23:2;
		unsigned portintr:1;
		unsigned hcintr:1;
		unsigned ptxfempty:1;
		unsigned reserved27:1;
		unsigned conidstschng:1;
		unsigned disconnect:1;
		unsigned sessreqintr:1;
		unsigned wkupintr:1;
	} b;
} gintmsk_t;


/* GRXSTSR/GRXSTSP
 * Host Mode Receive Status Debug Read/Status Read and Pop Registers
 * p1370
 * d219
 */
typedef union _grxstsr_t {
	/* raw register data */
	u32 d32;

	/* register bits */
	struct {
		unsigned chnum:4;
		unsigned bcnt:11;
		unsigned dpid:2;
		unsigned pktsts:4;
		unsigned Reserved:11;
	} b;
} grxstsr_t;


/* User HW Config2 Register
 * d230
 */
typedef union _ghwcfg2_t {
	/** raw register data */
	u32 d32;
	/** register bits */
	struct {
		/* GHWCFG2 */
		unsigned op_mode:3;
#define MODE_HNP_SRP_CAPABLE		0
#define MODE_SRP_ONLY_CAPABLE		1
#define MODE_NO_HNP_SRP_CAPABLE		2
#define MODE_SRP_CAPABLE_DEVICE		3
#define MODE_NO_SRP_CAPABLE_DEVICE	4
#define MODE_SRP_CAPABLE_HOST		5
#define MODE_NO_SRP_CAPABLE_HOST	6

		unsigned architecture:2;
#define HWCFG2_ARCH_SLAVE_ONLY		0x00
#define HWCFG2_ARCH_EXT_DMA			0x01
#define HWCFG2_ARCH_INT_DMA			0x02

		unsigned point2point:1;
		unsigned hs_phy_type:2;
		unsigned fs_phy_type:2;
		unsigned num_dev_ep:4;
		unsigned num_host_chan:4;
		unsigned perio_ep_supported:1;
		unsigned dynamic_fifo:1;
		unsigned rx_status_q_depth:2;
		unsigned nonperio_tx_q_depth:2;
		unsigned host_perio_tx_q_depth:2;
		unsigned dev_token_q_depth:5;
		unsigned reserved31:1;
	} b;
} ghwcfg2_t;


/*********************************************************************
 * Host Mode Registers
 *********************************************************************/

/* Host Configuration Register
 * d247
 */
typedef union _hcfg_t {
	/** raw register data */
	u32 d32;

	/** register bits */
	struct {
		/** FS/LS Phy Clock Select */
		unsigned fslspclksel:2;
#define HCFG_30_60_MHZ	0
#define HCFG_48_MHZ	    1
#define HCFG_6_MHZ		2

		/** FS/LS Only Support */
		unsigned fslssupp:1;
		unsigned reserved3_31:29;
	} b;
} hcfg_t;


/* Host Frame Interval Register
 * d247
 */
typedef union _hfir_t {
	/* raw register data */
	u32 d32;

	/* register bits */
	struct {
		unsigned frint:16;
		unsigned Reserved:16;
	} b;
} hfir_t;

/* Host Frame Number/Frame Time Remaining Register
 * d248
 */
typedef union _hfnum_t {
	/* raw register data */
	u32 d32;

	/* register bits */
	struct {
		unsigned frnum:16;
#define HFNUM_MAX_FRNUM 0x3FFF
		unsigned frrem:16;
	} b;
} hfnum_t;


/* Host Channel Interrupt Mask Register
   d257
 */
typedef union _hcintmsk_t {
	/* raw register data */
	u32 d32;

	/* register bits */
	struct {
		unsigned xfercompl:1;
		unsigned chhltd:1;
		unsigned ahberr:1;
		unsigned stall:1;
		unsigned nak:1;
		unsigned ack:1;
		unsigned nyet:1;
		unsigned xacterr:1;
		unsigned bblerr:1;
		unsigned frmovrun:1;
		unsigned datatglerr:1;
		unsigned reserved:21;
	} b;
} hcintmsk_t;


/* Host Channel Interrupt Register
   d254
 */
typedef union _hcintn_t {
	u32	d32;
	struct {
		u32	xfercompl:1;
		u32	chhltd:1;
		u32	abherr:1;
		u32	stall:1;
		u32	nak:1;
		u32	ack:1;
		u32	nyet:1;
		u32	xacterr:1;
		u32	bblerr:1;
		u32	frmovrun:1;
		u32	datatglerr:1;
		u32	reserved:21;
	} b;
} hcintn_t;


/* Host All Channels Interrupt Register
   d250
 */

#define MAX_COUNT 10000
#define INT_ALL	0xffffffff

typedef union _haint_t {
	u32	d32;
	struct {
		u32	channel_intr_0:1;
		u32	channel_intr_1:1;
		u32	channel_intr_2:1;
		u32	channel_intr_3:1;
		u32	channel_intr_4:1;
		u32	channel_intr_5:1;
		u32	channel_intr_6:1;
		u32	channel_intr_7:1;
		u32	channel_intr_8:1;
		u32	channel_intr_9:1;
		u32	channel_intr_10:1;
		u32	channel_intr_11:1;
		u32	channel_intr_12:1;
		u32	channel_intr_13:1;
		u32	channel_intr_14:1;
		u32	channel_intr_15:1;
		u32	reserved1:16;
	} b;
} haint_t;


/* Host Port Control and Status Register
 * d250
 */
typedef union _hprt_t {
	/** raw register data */
	u32 d32;
	/** register bits */
	struct {
		unsigned prtconnsts:1;
		unsigned prtconndet:1;
		unsigned prtena:1;
		unsigned prtenchng:1;
		unsigned prtovrcurract:1;
		unsigned prtovrcurrchng:1;
		unsigned prtres:1;
		unsigned prtsusp:1;
		unsigned prtrst:1;
		unsigned reserved9:1;
		unsigned prtlnsts:2;
		unsigned prtpwr:1;
		unsigned prttstctl:4;
		unsigned prtspd:2;
#define HPRT0_PRTSPD_HIGH_SPEED 0
#define HPRT0_PRTSPD_FULL_SPEED 1
#define HPRT0_PRTSPD_LOW_SPEED  2
		unsigned reserved19_31:13;
	} b;
} hprt_t;


/* Port status for the HC */
#define	HCD_DRIVE_RESET			0x0001
#define	HCD_SEND_SETUP			0x0002

#define	HC_MAX_PKT_COUNT		511
#define	HC_MAX_TRANSFER_SIZE	65535
#define	MAXP_SIZE_64BYTE		64
#define	MAXP_SIZE_512BYTE		512
#define	MAXP_SIZE_1024BYTE		1024

/* Host Channel-n Charracteristics Register
 * d253
 */
typedef union _hcchar_t {
	/* raw register data */
	u32 d32;

	/* register bits */
	struct {
		/* Maximum packet size in bytes */
		unsigned mps:11;

		/* Endpoint number */
		unsigned epnum:4;

		/* 0: OUT, 1: IN */
		unsigned epdir:1;
#define HCDIR_OUT				0
#define HCDIR_IN				1

		unsigned reserved:1;

		/* 0: Full/high speed device, 1: Low speed device */
		unsigned lspddev:1;

		/* 0: Control, 1: Isoc, 2: Bulk, 3: Intr */
		unsigned eptype:2;
#define OTG_EP_TYPE_CONTROL	0
#define OTG_EP_TYPE_ISOC		1
#define OTG_EP_TYPE_BULK		2
#define OTG_EP_TYPE_INTR		3

		/* Packets per frame for periodic transfers. 0 is reserved. */
		unsigned multicnt:2;

		/* Device address */
		unsigned devaddr:7;

		/* Frame to transmit periodic transaction. */
		/* 0: even, 1: odd */
		unsigned oddfrm:1;

		/* Channel disable */
		unsigned chdis:1;

		/* Channel enable */
		unsigned chen:1;
	} b;
} hcchar_t;

/* Host Channel-n Transfer Size Register
 * d257
 */
typedef union _hctsiz_t {
	/* raw register data */
	u32 d32;

	/* register bits */
	struct {
		/* Total transfer size in bytes */
		unsigned xfersize:19;

		/* Data packets to transfer */
		unsigned pktcnt:10;

		/* Packet ID for next data packet */
		/* 0: DATA0 */
		/* 1: DATA2 */
		/* 2: DATA1 */
		/* 3: MDATA (non-Control), SETUP (Control) */
		unsigned pid:2;
#define HCTSIZ_DATA0		0
#define HCTSIZ_DATA1		2
#define HCTSIZ_DATA2		1
#define HCTSIZ_MDATA		3
#define HCTSIZ_SETUP		3

		/* Do PING protocol when 1 */
		unsigned dopng:1;
	} b;
} hctsiz_t;


/* The Power and Clock Gating Control Register
   d292
 */
typedef	union _pcgcctl_t {

	/** raw register data */
	u32 d32;
	/** register bits */
	struct {
		unsigned	stoppclk:1;
		unsigned	gatehclk:1;
		unsigned	pwrclmp:1;
		unsigned	rstpdwnmodule:1;
		unsigned	physuspended:1;
		unsigned	Reserved5_31:27;
	} b;
} pcgcctl_t;


#endif


