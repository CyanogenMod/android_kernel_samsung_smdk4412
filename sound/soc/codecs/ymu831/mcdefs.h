/****************************************************************************
 *
 *	Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcdefs.h
 *
 *	Description	: MC Device Definitions
 *
 *	Version		: 1.0.1	2012.12.18
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.	In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *	claim that you wrote the original software. If you use this software
 *	in a product, an acknowledgment in the product documentation would be
 *	appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *	misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ****************************************************************************/

#ifndef _MCDEFS_H_
#define	_MCDEFS_H_

/*	Register Definition

	[Naming Rules]

	  MCI_xxx	: Registers
	  MCI_xxx_DEF	: Default setting of registers
	  MCB_xxx	: Miscelleneous bit definition
*/

/*	Registers	*/
/*	IF_REG	*/
#define	MCI_A_REG_A			(0)
#define	MCB_A_REG_AINC			(0x80)

#define	MCI_A_REG_D			(1)

#define	MCI_RST_A			(2)
#define	MCB_RST_A			(0x01)
#define	MCI_RST_A_DEF			(MCB_RST_A)

#define	MCI_RST				(3)
#define	MCB_PSW_S			(0x80)
#define	MCB_PSW_M			(0x40)
#define	MCB_PSW_F			(0x20)
#define	MCB_PSW_C			(0x10)
#define	MCB_RST_S			(0x08)
#define	MCB_RST_M			(0x04)
#define	MCB_RST_F			(0x02)
#define	MCB_RST_C			(0x01)
#define	MCI_RST_DEF			(MCB_PSW_S|MCB_RST_S	\
					|MCB_PSW_M|MCB_RST_M	\
					|MCB_PSW_F|MCB_RST_F	\
					|MCB_PSW_C|MCB_RST_C)

#define	MCI_IRQ				(4)
#define	MCB_IRQ				(0x02)
#define	MCB_EIRQ			(0x01)

#define	MCI_ANA_REG_A			(6)
#define	MCB_ANA_REG_AINC		(0x80)

#define	MCI_ANA_REG_D			(7)

#define	MCI_CD_REG_A			(8)
#define	MCB_CD_REG_AINC			(0x80)

#define	MCI_CD_REG_D			(9)

#define	MCI_IRQR			(10)
#define	MCB_IRQR			(0x02)
#define	MCB_EIRQR			(0x01)

#define	MCI_MA_REG_A			(12)
#define	MCB_MA_REG_AINC			(0x80)

#define	MCI_MA_REG_D			(13)

#define	MCI_MB_REG_A			(14)
#define	MCB_MB_REG_AINC			(0x80)

#define	MCI_MB_REG_D			(15)

/*	IF_REG = #16: B_REG_A

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| B_REG |                        B_REG_A                        |
	| _AINC |                                                       |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_B_REG_A			(16)
#define MCI_B_REG_A_DEF			(0x00)
#define	MCB_B_REG_AINC			(0x80)
#define MCB_B_REG_A			(0x7F)

/*	IF_REG = #17: B_REG_D

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          B_REG_D[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_B_REG_D			(17)
#define MCI_B_REG_D_DEF			(0x00)
#define MCB_B_REG_D			(0xFF)

/*	IF_REG = #18: BMAA0

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  | "0"   |  "0"  |  "0"  |  "0"  |  "0"  | BMAA  |
	|       |       |       |       |       |       |       |  [16] |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define	MCI_BMAA0			(18)
#define MCI_BMAA0_DEF			(0x00)
#define MCB_BMAA0			(0x01)

/*	IF_REG = #19: BMAA1

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          BMAA[15:8]                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_BMAA1			(19)
#define MCI_BMAA1_DEF			(0x00)
#define MCB_BMAA1			(0xFF)

/*	IF_REG = #20: BMAA2

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          BMAA[7:0]                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_BMAA2			(20)
#define MCI_BMAA2_DEF			(0x00)
#define MCB_BMAA2			(0xFF)

/*	IF_REG = #21: BMACtl

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| BDSPT |  "0"  |  "0"  |  "0"  | BMAMOD| BMADIR|  BMABUS[1:0]  |
	|   INI |       |       |       |       |       |               |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_BMACTL			(21)
#define MCI_BMACTL_DEF			(0x00)
#define MCB_BDSPTINI			(0x80)
#define MCB_BMAMOD			(0x08)
#define MCB_BMAMOD_DMA			(0x00)
#define MCB_BMAMOD_DSP			(0x08)
#define MCB_BMADIR			(0x04)
#define MCB_BMADIR_DL			(0x00)
#define MCB_BMADIR_UL			(0x04)
#define MCB_BMABUS			(0x03)
#define MCB_BMABUS_I			(0x02)
#define MCB_BMABUS_Y			(0x01)
#define MCB_BMABUS_X			(0x00)

/*	IF_REG = #22: BMADat

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          BMAD[7:0]                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_BMAD			(22)
#define MCI_BMAD_DEF			(0x00)
#define MCB_BMAD			(0xFF)

/*	IF_REG = #23: BDSPTReq

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  BDSP |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |
	|  TREQ |       |       |       |       |       |       |       |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_BDSPTREQ			(23)
#define MCI_BDSPTREQ_DEF		(0x00)
#define MCB_BDSPTREQ			(0x80)

/*	IF_REG = #24: BDSPTCNT

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|  "0"  |  "0"  |  "0"  |  "0"  |         BDSPTCNT[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_BDSP_TCNT			(24)
#define MCI_BDSP_TCNT_DEF		(0x00)
#define MCB_BDSP_TCNT			(0x0F)


/*	IF_REG = #32: E_REG_A

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| E_REG |                     E_REG_A[6:0]                      |
	| _AINC |                                                       |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E_REG_A			(32)
#define MCI_E_REG_A_DEF			(0x00)
#define	MCB_E_REG_AINC			(0x80)
#define MCB_E_REG_A			(0x7F)

/*	IF_REG = #33: E_REG_D

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          E_REG_D[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E_REG_D			(33)
#define MCI_E_REG_D_DEF			(0x00)
#define MCB_E_REG_D			(0xFF)

/*	IF_REG = #34: EDSP_FW_PAGE

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|       EDSP_FW_PAGE[2:0]       |  "0"  |  "0"  |  "0"  |EDSP_FW|
        |                               |       |       |       | _A[8] |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_EDSP_FW_PAGE		(34)
#define MCI_EDSP_FW_PAGE_DEF		(0x00)
#define MCB_EDSP_FW_PAGE		(0x70)
#define MCB_EDSP_FW_PAGE_E2IMEM		(0x10)
#define MCB_EDSP_FW_PAGE_SYSEQ0		(0x20)
#define MCB_EDSP_FW_PAGE_SYSEQ1		(0x30)
#define MCB_EDSP_FW_PAGE_E2YMEM		(0x40)
#define MCB_EDSP_FW_PAGE_E2XMEM		(0x60)
#define MCB_EDSP_FW_PAGE_SYSEQ0_B0	(0x20)
#define MCB_EDSP_FW_PAGE_SYSEQ1_B0	(0x30)
#define MCB_EDSP_FW_PAGE_SYSEQ0_B1	(0x80)
#define MCB_EDSP_FW_PAGE_SYSEQ1_B1	(0x90)
#define MCB_EDSP_FW_PAGE_SYSEQ0_B2	(0xa0)
#define MCB_EDSP_FW_PAGE_SYSEQ1_B2	(0xb0)
#define MCB_EDSP_FW_A_8			(0x01)

/*	IF_REG = #35: EDSP_FW_A

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                        EDSP_FW_A[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_EDSP_FW_A			(35)
#define MCI_EDSP_FW_A_DEF		(0x00)
#define MCB_EDSP_FW_A			(0xFF)

/*	IF_REG = #36: EDSP_FW_D

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                        EDSP_FW_D[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_EDSP_FW_D			(36)
#define MCI_EDSP_FW_D_DEF		(0x00)
#define MCB_EDSP_FW_D			(0xFF)

/*	IF_REG = #37: EDSP_IRQ_CTRL

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  | EE2DSP| EE2DSP|  "0"  |  "0"  | EE1DSP| EE1DSP|
        |       |       |  _OV  |  _STA |       |       |   _OV |  _STA |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_EEDSP			(37)
#define	MCI_EEDSP_DEF			(0x00)
#define	MCB_EE2DSP_OV			(0x20)
#define	MCB_EE2DSP			(0x10)
#define	MCB_EE1DSP_OV			(0x02)
#define	MCB_EE1DSP			(0x01)

/*	IF_REG = #38: EDSP_IRQ_FLAG

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  | E2DSP | E2DSP |  "0"  |  "0"  | E1DSP | E1DSP |
        |       |       |  _OV  |  _STA |       |       |   _OV |  _STA |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_EDSP			(38)
#define	MCI_EDSP_DEF			(0x00)
#define	MCB_E2DSP_OV			(0x20)
#define	MCB_E2DSP_STA			(0x10)
#define	MCB_E1DSP_OV			(0x02)
#define	MCB_E1DSP_STA			(0x01)

#define	MCI_C_REG_A			(40)

#define	MCI_C_REG_D			(41)

#define	MCI_DEC_FIFO			(42)

/*	IF_ADR = #43: decoder IRQ control register

	7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| ECDSP | EFFIFO| ERFIFO| EEFIFO| EOFIFO| EDFIFO| EENC  | EDEC  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_ECDSP			(43)
#define	MCB_ECDSP			(0x80)
#define	MCB_EFFIFO			(0x40)
#define	MCB_ERFIFO			(0x20)
#define	MCB_EEFIFO			(0x10)
#define	MCB_EOFIFO			(0x08)
#define	MCB_EDFIFO			(0x04)
#define	MCB_EENC			(0x02)
#define	MCB_EDEC			(0x01)

/*	IF_ADR = #44: decoder IRQ Flag register

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| CDSP  | FFIFO | RFIFO | EFIFO | OFIFO | DFIFO | ENC   | DEC   |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_CDSP			(44)
#define MCB_IRQFLAG_CDSP		(0x80)
#define MCB_IRQFLAG_FFIFO		(0x40)
#define MCB_IRQFLAG_RFIFO		(0x20)
#define MCB_IRQFLAG_EFIFO		(0x10)
#define MCB_IRQFLAG_OFIFO		(0x08)
#define MCB_IRQFLAG_DFIFO		(0x04)
#define MCB_IRQFLAG_ENC			(0x02)
#define MCB_IRQFLAG_DEC			(0x01)
#define MCB_IRQFLAG_CDSP_ALL		(0xFF)

/*	IF_ADR = #45: FFIFO register

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|                         FSQ_FIFO[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FSQ_FFIFO			(45)

/*	IF_ADR = #48: F_REG_A register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| F_REG |                      F_REG_A[6:0]                     |
	|  AINC |                                                       |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_F_REG_A			(48)
#define MCI_F_REG_A_DEF			(0x00)
#define	MCB_F_REG_AINC			(0x80)
#define MCB_F_REG_A			(0x7F)

/*	IF_ADR = #49: F_REG_D register

	7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           F_REG_D[7:0]                        |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_F_REG_D			(49)
#define MCI_F_REG_D_DEF			(0x00)
#define MCB_F_REG_D			(0xFF)

/*	IF_ADR = #50: FMAA

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  |        FMAA[19:16]            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FMAA_19_16			(50)
#define MCI_FMAA_19_16_DEF		(0x00)
#define MCB_FMAA_19_16			(0x0F)

/*	IF_ADR = #51: FMAA

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          FMAA[15:8]                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FMAA_15_8			(51)
#define MCI_FMAA_15_8_DEF		(0x00)
#define MCB_FMAA_15_8			(0xFF)

/*	IF_ADR = #52: FMAA

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          FMAA[7:0]                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FMAA_7_0			(52)
#define MCI_FMAA_7_0_DEF		(0x00)
#define MCB_FMAA_7_0			(0xFF)

/*	IF_ADR = #53: FDSPTINI

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|DSPTINI|  "0"  | "0"   |  "0"  | FMAMOD| FMADIR|  FMABUS[1:0]  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FDSPTINI			(53)
#define MCI_FDSPTINI_DEF		(0x00)
#define MCB_DSPTINI			(0x80)
#define MCB_FMAMOD			(0x08)
#define MCB_FMAMOD_DMA			(0x00)
#define MCB_FMAMOD_DSP			(0x08)
#define MCB_FMADIR			(0x04)
#define MCB_FMADIR_DL			(0x00)
#define MCB_FMADIR_UL			(0x04)
#define MCB_FMABUS			(0x03)
#define MCB_FMABUS_I			(0x02)
#define MCB_FMABUS_Y			(0x01)
#define MCB_FMABUS_X			(0x00)

/*	IF_ADR = #54: FMAD

	7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          FMAD[7:0]                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FMAD			(54)
#define MCI_FMAD_DEF			(0x00)
#define MCB_FMAD			(0xFF)

/*	IF_ADR = #55: DSPTReq

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|DSPTREQ|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FDSPTREQ			(55)
#define MCI_FDSPTREQ_DEF		(0x00)
#define MCB_FDSPTREQ			(0x80)

/*	IF_ADR = #57: IENB

       	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| IESERR|  "0"  | IEAMT | IEAMT |           IEFW[3:0]           |
	|       |       |  BEG  |  END  |                               |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_IESERR			(57)
#define MCI_IESERR_DEF			(0x00)
#define	MCB_IESERR			(0x80)
#define	MCB_IEAMTBEG			(0x20)
#define	MCB_IEAMTEND			(0x10)
#define	MCB_IEFW			(0x0F)
#define MCB_IEFW_STRT			(0x08)
#define MCB_IEFW_TOP			(0x04)
#define MCB_IEFW_APP			(0x02)
#define MCB_IEFW_DSP			(0x01)

/*	A_ADR = #58: IReq

      	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| IRSERR|  "0"  | IRAMT | IRAMT |           IRFW[3:0]           |
	|       |       |  BEG  |  END  |                               |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_IRSERR			(58)
#define MCI_IRSERR_DEF			(0x00)
#define MCB_IRSERR			(0x80)
#define MCB_IRMTBEG			(0x20)
#define MCB_IRMTEND			(0x10)
#define MCB_IRFW			(0x0F)
#define MCB_IRFW_STRT			(0x08)
#define MCB_IRFW_TOP			(0x04)
#define MCB_IRFW_APP			(0x02)
#define MCB_IRFW_DSP			(0x01)


/*	A_REG	*/
#define	MCI_A_DEV_ID			(0)

#define	MCI_CLK_MSK			(1)
#define	MCB_RTCI_MSK			(0x04)
#define	MCB_CLKI1_MSK			(0x02)
#define	MCB_CLKI0_MSK			(0x01)
#define	MCI_CLK_MSK_DEF			(MCB_RTCI_MSK	\
					|MCB_CLKI1_MSK	\
					|MCB_CLKI0_MSK)

#define	MCI_PD				(2)
#define	MCB_PLL_PD			(0x80)
#define	MCB_ANACLK_PD			(0x40)
#define	MCB_PE_CLK_PD			(0x20)
#define	MCB_PB_CLK_PD			(0x10)
#define	MCB_PM_CLK_PD			(0x08)
#define	MCB_PF_CLK_PD			(0x04)
#define	MCB_PC_CLK_PD			(0x02)
#define	MCB_VCOOUT_PD			(0x01)
#define	MCI_PD_DEF			(MCB_PLL_PD \
					|MCB_ANACLK_PD \
					|MCB_PE_CLK_PD \
					|MCB_PB_CLK_PD \
					|MCB_PM_CLK_PD \
					|MCB_PF_CLK_PD \
					|MCB_PC_CLK_PD \
					|MCB_VCOOUT_PD)

#define	MCI_JTAGSEL			(3)
#define	MCB_JTAGSEL			(0x01)

#define	MCI_DO0_DRV			(4)
#define	MCB_DO0_DRV			(0x80)
#define	MCB_BCLK0_INV			(0x40)
#define	MCB_BCLK0_MSK			(0x20)
#define	MCB_BCLK0_DDR			(0x10)
#define	MCB_LRCK0_MSK			(0x08)
#define	MCB_LRCK0_DDR			(0x04)
#define	MCB_SDIN0_MSK			(0x02)
#define	MCB_SDO0_DDR			(0x01)
#define	MCI_DO0_DRV_DEF			(MCB_BCLK0_MSK	\
					|MCB_LRCK0_MSK	\
					|MCB_SDIN0_MSK)

#define	MCI_DO1_DRV			(5)
#define	MCB_DO1_DRV			(0x80)
#define	MCB_BCLK1_INV			(0x40)
#define	MCB_BCLK1_MSK			(0x20)
#define	MCB_BCLK1_DDR			(0x10)
#define	MCB_LRCK1_MSK			(0x08)
#define	MCB_LRCK1_DDR			(0x04)
#define	MCB_SDIN1_MSK			(0x02)
#define	MCB_SDO1_DDR			(0x01)
#define	MCI_DO1_DRV_DEF			(MCB_BCLK1_MSK	\
					|MCB_LRCK1_MSK	\
					|MCB_SDIN1_MSK)

#define	MCI_DO2_DRV			(6)
#define	MCB_DO2_DRV			(0x80)
#define	MCB_BCLK2_INV			(0x40)
#define	MCB_BCLK2_MSK			(0x20)
#define	MCB_BCLK2_DDR			(0x10)
#define	MCB_LRCK2_MSK			(0x08)
#define	MCB_LRCK2_DDR			(0x04)
#define	MCB_SDIN2_MSK			(0x02)
#define	MCB_SDO2_DDR			(0x01)
#define	MCI_DO2_DRV_DEF			(MCB_BCLK2_MSK	\
					|MCB_LRCK2_MSK	\
					|MCB_SDIN2_MSK)

#define	MCI_PCMOUT_HIZ			(8)
#define	MCB_PDM2_DATA_DELAY		(0x40)
#define	MCB_PDM1_DATA_DELAY		(0x20)
#define	MCB_PDM0_DATA_DELAY		(0x10)
#define	MCB_PCMOUT2_HIZ			(0x04)
#define	MCB_PCMOUT1_HIZ			(0x02)
#define	MCB_PCMOUT0_HIZ			(0x01)

#define	MCI_PA0				(9)
#define	MCB_PA0_OUT			(0x80)
#define	MCB_PA0_DDR			(0x20)
#define	MCB_PA0_DATA			(0x10)
#define	MCB_PA0_OUTSEL			(0x04)
#define	MCB_PA0_MSK			(0x02)
#define	MCB_PA0_INV			(0x01)
#define	MCI_PA0_DEF			(MCB_PA0_MSK)

#define	MCI_PA1				(10)
#define	MCB_PA1_DDR			(0x20)
#define	MCB_PA1_DATA			(0x10)
#define	MCB_PA1_OUTSEL			(0x04)
#define	MCB_PA1_MSK			(0x02)
#define	MCB_PA1_INV			(0x01)
#define	MCI_PA1_DEF			(MCB_PA1_MSK)

#define	MCI_PA2				(11)
#define	MCB_PA2_DDR			(0x20)
#define	MCB_PA2_DATA			(0x10)
#define	MCB_PA2_OUTSEL			(0x04)
#define	MCB_PA2_MSK			(0x02)
#define	MCB_PA2_INV			(0x01)
#define	MCI_PA2_DEF			(MCB_PA2_MSK)

#define	MCI_DOA_DRV			(12)
#define	MCI_DOA_DRV_DEF			(0x80)

#define	MCI_LP0_FP			(13)
#define	MCI_LP0_FP_DEF			(0x00)

#define	MCI_LP1_FP			(14)
#define	MCI_LP1_FP_DEF			(0x01)

#define	MCI_LP2_FP			(15)
#define	MCI_LP2_FP_DEF			(0x02)

#define	MCI_LP3_FP			(16)
#define	MCI_LP3_FP_DEF			(0x03)

#define	MCI_CK_TCX0			(17)

#define	MCI_CLKSRC			(18)
#define	MCB_CLKBUSY			(0x80)
#define	MCB_CLKSRC			(0x20)
#define	MCB_CLK_INPUT			(0x10)
#define	MCI_CLKSRC_DEF			(0x04)

#define	MCI_FREQ73M			(19)
#define	MCI_FREQ73M_DEF			(0x00)

#define	MCI_PLL_MODE_A			(24)
#define	MCI_PLL_MODE_A_DEF		(0x04)

#define	MCI_PLL_PREDIV_A		(25)

#define	MCI_PLL_FBDIV_A_12_8		(26)
#define	MCI_PLL_FBDIV_A_7_0		(27)

#define	MCI_PLL_FRAC_A_15_8		(28)
#define	MCI_PLL_FRAC_A_7_0		(29)

#define	MCI_PLL_FOUT_A			(30)
#define	MCB_PLL_FOUT_A			(0x02)

#define	MCI_PLL_MODE_B			(40)
#define	MCI_PLL_MODE_B_DEF		(0x04)

#define	MCI_PLL_PREDIV_B		(41)

#define	MCI_PLL_FBDIV_B_12_8		(42)
#define	MCI_PLL_FBDIV_B_7_0		(43)

#define	MCI_PLL_FRAC_B_15_8		(44)
#define	MCI_PLL_FRAC_B_7_0		(45)

#define	MCI_PLL_FOUT_B			(46)
#define	MCB_PLL_FOUT_B			(0x02)

#define	MCI_SCKMSKON_B			(67)


/*	MA_REG	*/
#define	MCI_DIFI_VFLAG			(0)
#define	MCB_DIFI3_VFLAG1		(0x80)
#define	MCB_DIFI3_VFLAG0		(0x40)
#define	MCB_DIFI2_VFLAG1		(0x20)
#define	MCB_DIFI2_VFLAG0		(0x10)
#define	MCB_DIFI1_VFLAG1		(0x08)
#define	MCB_DIFI1_VFLAG0		(0x04)
#define	MCB_DIFI0_VFLAG1		(0x02)
#define	MCB_DIFI0_VFLAG0		(0x01)

#define	MCI_ADI_VFLAG			(1)
#define	MCB_ADI2_VFLAG1			(0x20)
#define	MCB_ADI2_VFLAG0			(0x10)
#define	MCB_ADI1_VFLAG1			(0x08)
#define	MCB_ADI1_VFLAG0			(0x04)
#define	MCB_ADI0_VFLAG1			(0x02)
#define	MCB_ADI0_VFLAG0			(0x01)

#define	MCI_DIFO_VFLAG			(2)
#define	MCB_DIFO3_VFLAG1		(0x80)
#define	MCB_DIFO3_VFLAG0		(0x40)
#define	MCB_DIFO2_VFLAG1		(0x20)
#define	MCB_DIFO2_VFLAG0		(0x10)
#define	MCB_DIFO1_VFLAG1		(0x08)
#define	MCB_DIFO1_VFLAG0		(0x04)
#define	MCB_DIFO0_VFLAG1		(0x02)
#define	MCB_DIFO0_VFLAG0		(0x01)

#define	MCI_DAO_VFLAG			(3)
#define	MCB_DAO1_VFLAG1			(0x08)
#define	MCB_DAO1_VFLAG0			(0x04)
#define	MCB_DAO0_VFLAG1			(0x02)
#define	MCB_DAO0_VFLAG0			(0x01)

#define	MCI_I_VINTP			(4)
#define	MCB_ADI2_VINTP			(0x40)
#define	MCB_ADI1_VINTP			(0x20)
#define	MCB_ADI0_VINTP			(0x10)
#define	MCB_DIFI3_VINTP			(0x08)
#define	MCB_DIFI2_VINTP			(0x04)
#define	MCB_DIFI1_VINTP			(0x02)
#define	MCB_DIFI0_VINTP			(0x01)
#define	MCI_I_VINTP_DEF			(MCB_ADI2_VINTP \
					|MCB_ADI1_VINTP \
					|MCB_ADI0_VINTP \
					|MCB_DIFI3_VINTP \
					|MCB_DIFI2_VINTP \
					|MCB_DIFI1_VINTP \
					|MCB_DIFI0_VINTP)

#define	MCI_O_VINTP			(5)
#define	MCB_DAO1_VINTP			(0x20)
#define	MCB_DAOO_VINTP			(0x10)
#define	MCB_DIFO3_VINTP			(0x08)
#define	MCB_DIFO2_VINTP			(0x04)
#define	MCB_DIFO1_VINTP			(0x02)
#define	MCB_DIFO0_VINTP			(0x01)
#define	MCI_O_VINTP_DEF			(MCB_DAO1_VINTP \
					|MCB_DAOO_VINTP \
					|MCB_DIFO3_VINTP \
					|MCB_DIFO2_VINTP \
					|MCB_DIFO1_VINTP \
					|MCB_DIFO0_VINTP)

#define	MCI_DIFI0_VOL0			(6)
#define	MCB_DIFI0_VSEP			(0x80)

#define	MCI_DIFI0_VOL1			(7)

#define	MCI_DIFI1_VOL0			(8)
#define	MCB_DIFI1_VSEP			(0x80)

#define	MCI_DIFI1_VOL1			(9)

#define	MCI_DIFI2_VOL0			(10)
#define	MCB_DIFI2_VSEP			(0x80)

#define	MCI_DIFI2_VOL1			(11)

#define	MCI_DIFI3_VOL0			(12)
#define	MCB_DIFI3_VSEP			(0x80)

#define	MCI_DIFI3_VOL1			(13)

#define	MCI_ADI0_VOL0			(14)
#define	MCB_ADI0_VSEP			(0x80)

#define	MCI_ADI0_VOL1			(15)

#define	MCI_ADI1_VOL0			(16)
#define	MCB_ADI1_VSEP			(0x80)

#define	MCI_ADI1_VOL1			(17)

#define	MCI_ADI2_VOL0			(18)
#define	MCB_ADI2_VSEP			(0x80)

#define	MCI_ADI2_VOL1			(19)

#define	MCI_DIFO0_VOL0			(20)
#define	MCB_DIFO0_VSEP			(0x80)

#define	MCI_DIFO0_VOL1			(21)

#define	MCI_DIFO1_VOL0			(22)
#define	MCB_DIFO1_VSEP			(0x80)

#define	MCI_DIFO1_VOL1			(23)

#define	MCI_DIFO2_VOL0			(24)
#define	MCB_DIFO2_VSEP			(0x80)

#define	MCI_DIFO2_VOL1			(25)

#define	MCI_DIFO3_VOL0			(26)
#define	MCB_DIFO3_VSEP			(0x80)

#define	MCI_DIFO3_VOL1			(27)

#define	MCI_DAO0_VOL0			(28)
#define	MCB_DAO0_VSEP			(0x80)

#define	MCI_DAO0_VOL1			(29)

#define	MCI_DAO1_VOL0			(30)
#define	MCB_DAO1_VSEP			(0x80)

#define	MCI_DAO1_VOL1			(31)

#define	MCI_ADI_SWAP			(32)

#define	MCI_ADI2_SWAP			(33)

#define	MCI_DAO_SWAP			(34)

#define	MCI_IN0_MIX0			(35)
#define	MCB_IN0_MSEP			(0x80)

#define	MCI_IN0_MIX1			(36)

#define	MCI_IN1_MIX0			(37)
#define	MCB_IN1_MSEP			(0x80)

#define	MCI_IN1_MIX1			(38)

#define	MCI_IN2_MIX0			(39)
#define	MCB_IN2_MSEP			(0x80)

#define	MCI_IN2_MIX1			(40)

#define	MCI_IN3_MIX0			(41)
#define	MCB_IN3_MSEP			(0x80)

#define	MCI_IN3_MIX1			(42)

#define	MCI_OUT0_MIX0_10_8		(43)
#define	MCB_OUT0_MSEP			(0x80)

#define	MCI_OUT0_MIX0_7_0		(44)

#define	MCI_OUT0_MIX1_10_8		(45)

#define	MCI_OUT0_MIX1_7_0		(46)

#define	MCI_OUT1_MIX0_10_8		(47)
#define	MCB_OUT1_MSEP			(0x80)

#define	MCI_OUT1_MIX0_7_0		(48)

#define	MCI_OUT1_MIX1_10_8		(49)

#define	MCI_OUT1_MIX1_7_0		(50)

#define	MCI_OUT2_MIX0_10_8		(51)
#define	MCB_OUT2_MSEP			(0x80)

#define	MCI_OUT2_MIX0_7_0		(52)

#define	MCI_OUT2_MIX1_10_8		(53)

#define	MCI_OUT2_MIX1_7_0		(54)

#define	MCI_OUT3_MIX0_10_8		(55)
#define	MCB_OUT3_MSEP			(0x80)

#define	MCI_OUT3_MIX0_7_0		(56)

#define	MCI_OUT3_MIX1_10_8		(57)

#define	MCI_OUT3_MIX1_7_0		(58)

#define	MCI_OUT4_MIX0_10_8		(59)
#define	MCB_OUT4_MSEP			(0x80)

#define	MCI_OUT4_MIX0_7_0		(60)

#define	MCI_OUT4_MIX1_10_8		(61)

#define	MCI_OUT4_MIX1_7_0		(62)

#define	MCI_OUT5_MIX0_10_8		(63)
#define	MCB_OUT5_MSEP			(0x80)

#define	MCI_OUT5_MIX0_7_0		(64)

#define	MCI_OUT5_MIX1_10_8		(65)

#define	MCI_OUT5_MIX1_7_0		(66)

#define	MCI_DSP_START			(67)
#define	MCB_DSP_START			(0x01)

#define	MCI_SPATH_ON			(68)
#define	MCB_SPATH_ON			(0x80)
#define	MCB_IN_MIX_ON			(0x40)
#define	MCB_OUT_MIX_ON_5		(1<<5)
#define	MCB_OUT_MIX_ON_4		(1<<4)
#define	MCB_OUT_MIX_ON_3		(1<<3)
#define	MCB_OUT_MIX_ON_2		(1<<2)
#define	MCB_OUT_MIX_ON_1		(1<<1)
#define	MCB_OUT_MIX_ON_0		(1<<0)

#define	MCI_CLK_SEL			(70)

#define	MCI_LINK_LOCK			(71)
#define	MCB_LINK_LOCK			(0x80)

#define	MCI_FDSP_PI_SOURCE		(80)
#define	MCB_FDSP_PI_SOURCE_AE		(0x00)
#define	MCB_FDSP_PI_SOURCE_VDSP		(0x07)
#define	MCB_FDSP_PI_SOURCE_VDSP_Ex	(0xF)
#define	MCB_FDSP_EX_SYNC		(0x80)

#define	MCI_FDSP_PO_SOURCE		(81)
#define	MCB_FDSP_PO_SOURCE_AE		(0x00)
#define	MCB_FDSP_PO_SOURCE_VDSP		(0x07)
#define	MCB_FDSP_PO_SOURCE_VDSP_Ex	(0x3F)
#define	MCB_FDSP_PO_SOURCE_MASK		(0x3F)

#define	MCI_BDSP_SOURCE			(82)

#define	MCI_SRC_VSOURCE			(83)

#define	MCI_LPT2_MIX_VOL		(84)


/*	MB_REG	*/
#define	MCI_LP0_MODE			(0)
#define	MCB_LP0_STMODE			(0x80)
#define	MCB_LP0_AUTO_FS			(0x40)
#define	MCB_LP0_SRC_THRU		(0x20)
#define	MCB_LPT0_EDGE			(0x10)
#define	MCB_LP0_MODE			(0x01)
#define	MCI_LP0_MODE_DEF		(MCB_LP0_AUTO_FS)

#define	MCI_LP0_BCK			(1)

#define	MCI_LPR0_SLOT			(2)
#define	MCI_LPR0_SLOT_DEF		(0x24)

#define	MCI_LPR0_SWAP			(3)

#define	MCI_LPT0_SLOT			(4)
#define	MCI_LPT0_SLOT_DEF		(0x24)

#define	MCI_LPT0_SWAP			(5)

#define	MCI_LP0_FMT			(6)

#define	MCI_LP0_PCM			(7)

#define	MCI_LPR0_PCM			(8)
#define	MCB_LPR0_PCM_MONO		(0x80)
#define	MCI_LPR0_PCM_DEF		(MCB_LPR0_PCM_MONO)

#define	MCI_LPT0_PCM			(9)
#define	MCB_LPT0_PCM_MONO		(0x80)
#define	MCI_LPT0_PCM_DEF		(MCB_LPT0_PCM_MONO)

#define	MCI_LP0_START			(10)
#define	MCB_LP0_TIM_START		(0x80)
#define	MCB_LPR0_START_SRC		(0x08)
#define	MCB_LPR0_START			(0x04)
#define	MCB_LPT0_START_SRC		(0x02)
#define	MCB_LPT0_START			(0x01)

#define	MCI_LP1_MODE			(16)
#define	MCB_LP1_STMODE			(0x80)
#define	MCB_LP1_AUTO_FS			(0x40)
#define	MCB_LP1_SRC_THRU		(0x20)
#define	MCB_LP1_MODE			(0x01)
#define	MCI_LP1_MODE_DEF		(MCB_LP1_AUTO_FS)

#define	MCI_LP1_BCK			(17)

#define	MCI_LPR1_SWAP			(19)

#define	MCI_LPT1_SWAP			(21)

#define	MCI_LP1_FMT			(22)

#define	MCI_LP1_PCM			(23)

#define	MCI_LPR1_PCM			(24)
#define	MCB_LPR1_PCM_MONO		(0x80)
#define	MCI_LPR1_PCM_DEF		(MCB_LPR1_PCM_MONO)

#define	MCI_LPT1_PCM			(25)
#define	MCB_LPT1_PCM_MONO		(0x80)
#define	MCI_LPT1_PCM_DEF		(MCB_LPT1_PCM_MONO)

#define	MCI_LP1_START			(26)
#define	MCB_LP1_TIM_START		(0x80)
#define	MCB_LPR1_START_SRC		(0x08)
#define	MCB_LPR1_START			(0x04)
#define	MCB_LPT1_START_SRC		(0x02)
#define	MCB_LPT1_START			(0x01)

#define	MCI_LP2_MODE			(32)
#define	MCB_LP2_STMODE			(0x80)
#define	MCB_LP2_AUTO_FS			(0x40)
#define	MCB_LP2_SRC_THRU		(0x20)
#define	MCB_LP2_MODE			(0x01)
#define	MCI_LP2_MODE_DEF		(MCB_LP2_AUTO_FS)

#define	MCI_LP2_BCK			(33)

#define	MCI_LPR2_SWAP			(35)

#define	MCI_LPT2_SWAP			(37)

#define	MCI_LP2_FMT			(38)

#define	MCI_LP2_PCM			(39)

#define	MCI_LPR2_PCM			(40)
#define	MCB_LPR2_PCM_MONO		(0x80)
#define	MCI_LPR2_PCM_DEF		(MCB_LPR2_PCM_MONO)

#define	MCI_LPT2_PCM			(41)
#define	MCB_LPT2_PCM_MONO		(0x80)
#define	MCI_LPT2_PCM_DEF		(MCB_LPT2_PCM_MONO)

#define	MCI_LP2_START			(42)
#define	MCB_LP2_TIM_START		(0x80)
#define	MCB_LPR2_START_SRC		(0x08)
#define	MCB_LPR2_START			(0x04)
#define	MCB_LPT2_START_SRC		(0x02)
#define	MCB_LPT2_START			(0x01)

#define	MCI_SRC3			(43)

#define	MCI_SRC3_START			(44)
#define	MCB_SRC3_TIM_START		(0x80)
#define	MCB_ISRC3_START			(0x08)
#define	MCB_OSRC3_START			(0x02)

#define	MCI_LPT3_STMODE			(48)

#define	MCI_LP3_BCK			(49)

#define	MCI_LPR3_SWAP			(51)

#define	MCI_LPT3_SWAP			(53)

#define	MCI_LP3_FMT			(54)

#define	MCI_LP3_START			(58)
#define	MCB_LP3_TIM_START		(0x80)
#define	MCB_LPR3_START			(0x04)
#define	MCB_LPT3_START			(0x01)

#define	MCI_T_DPLL_FAST			(85)
#define	MCB_T_DPLL_FAST			(0x80)
#define	MCB_VOLREL_TIME			(0x20)

/*	B_REG	*/
/*	B_REG = #0: DSPCtl

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  DSP  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  DSP  |
	| BYPASS|       |       |       |       |       |       | START |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_BDSPCTRL			(0x00)
#define MCI_BDSPCTRL_DEF		(0x00)
#define MCB_BDSPBYPASS			(0x80)
#define MCB_BDSPSTART			(0x01)

/*	B_REG = #1: State0

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  OVER |  "0"  |  "0"  |  "0"  |  "0"  |      LOAD[10:8]       |
	|  LOAD |       |       |       |       |                       |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_STATE0			(0x01)
#define MCI_STATE0_DEF			(0x00)
#define MCB_OVERLOAD			(0x80)
#define MCB_LOAD0			(0x07)

/*	B_REG = #2: State1

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           LOAD[7:0]                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_STATE1			(0x02)
#define MCI_STATE1_DEF			(0x00)
#define MCB_LOAD1			(0xFF)

/*	B_REG = #64: FWCTRL0 (AEExec0)

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  GEN  |  "0"  | EQ3_0B| DRC_0 | DRC3  | EQ3_0A|  HEX  |  WIDE |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FWCTL0			(64)
#define MCI_AEEXEC0			MCI_FWCTL0
#define MCI_AEEXEC0_DEF			(0x00)
#define MCB_AEEXEC0_GEN			(0x80)
#define MCB_AEEXEC0_EQ3_0B		(0x20)
#define MCB_AEEXEC0_DRC_0		(0x10)
#define MCB_AEEXEC0_DRC3		(0x08)
#define MCB_AEEXEC0_EQ3_0A		(0x04)
#define MCB_AEEXEC0_HEX			(0x02)
#define MCB_AEEXEC0_WIDE		(0x01)

/*	B_REG = #65: FWCTRL1 (AEExec1)

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  | EQ3_1B| DRC_1 |  AGC  | EQ3_1A|  "0"  |  "0"  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FWCTL1			(65)
#define MCI_AEEXEC1			MCI_FWCTL1
#define MCI_AEEXEC1_DEF			(0x00)
#define MCB_AEEXEC1_EQ3_1B		(0x20)
#define MCB_AEEXEC1_DRC_1		(0x10)
#define MCB_AEEXEC1_AGC			(0x08)
#define MCB_AEEXEC1_EQ3_1A		(0x04)


/*	B_REG = #66: FWCTRL2 (AEBypass)

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  AE1  |  AE0  |
	|       |       |       |       |       |       | BYPASS| BYPASS|
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FWCTL2			(66)
#define MCI_AEBYPASS			MCI_FWCTL2
#define MCI_AEBYPASS_DEF		(0x00)
#define MCB_AEBYPASS_AE1		(0x02)
#define MCB_AEBYPASS_AE0		(0x01)

/*	B_REG = #67: FWCTRL3 (AEFade)

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
  R	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  AE1  |  AE0  |
	|       |       |       |       |       |       |  FADE |  FADE |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FWCTL3			(67)
#define MCI_AEFADE			MCI_FWCTL3
#define MCI_AEFADE_DEF			(0x00)
#define MCB_AEFADE_AE1			(0x02)
#define MCB_AEFADE_AE0			(0x01)

/*	B_REG = #68: FWCTRL4 (F0/1SEL)

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |   F1SEL[1:0]  |  "0"  |  "0"  |   F0SEL[1:0]  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FWCTL4			(68)
#define MCI_F01SEL			MCI_FWCTL4
#define MCI_F01SEL_DEF			(0x00)
#define MCB_F1SEL			(0x30)
#define MCB_F0SEL			(0x03)

/*	B_REG = #69: FWCTRL5 (SinOut)

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  SIN  |
	|       |       |       |       |       |       |       |  OUT  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FWCTL5			(69)
#define MCI_SINOUT			MCI_FWCTL5
#define MCI_SINOUT_DEF			(0x00)
#define MCB_SINOUT			(0x01)

/*	B_REG = #70: FWCTRL6 (SinOutFlg)

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  SIN  |
	|       |       |       |       |       |       |       |  OFLG |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FWCTL6			(70)
#define MCI_SINOUTFLG			MCI_FWCTL6
#define MCI_SINOUTFLG_DEF		(0x00)
#define MCB_SINOFLG			(0x01)

/*	B_REG = #71: FWCTRL7 (No Used)

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_FWCTL7			(71)

/*	E_REG	*/
/*	E_REG = #0: E1DSP_CTRL

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| E1DSP |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  | E1DSP |
	| _HALT |       |       |       |       |       |       |  _RST |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E1DSP_CTRL			(0)
#define	MCB_E1DSP_HALT			(0x80)
#define	MCB_E1DSP_RST			(0x01)
#define	MCI_E1DSP_CTRL_DEF		(MCB_E1DSP_RST)

/*	E_REG = #1: E1COMMAND/E1STATUS

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|                           E1COMMAND                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                            E1STATUS                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E1COMMAND			(1)
#define MCI_E1STATUS			(1)
#define MCI_E1COMMAND_DEF		(0x00)
#define MCI_E1STATUS_DEF		(0x00)
#define MCB_E1COMMAND			(0xFF)
#define MCB_E1STATUS			(0xFF)
#define E1COMMAND_RUN_MODE		(0x40)
#define E1COMMAND_OFFSET_CANCEL		(0x02)
#define E1COMMAND_IMP_SENSE		(0x04)
#define E1COMMAND_WAIT			(0x08)
#define E1COMMAND_PD			(0x10)
#define E1COMMAND_GND_DET		(0x20)
#define E1COMMAND_ADDITIONAL		(0x80)
#define E1COMMAND_SLEEP_MODE		(0x00)

#define	MCI_LPF_THR			(2)
#define	MCB_LPF1_PST_THR		(0x80)
#define	MCB_LPF0_PST_THR		(0x40)
#define	MCB_LPF1_PRE_THR		(0x20)
#define	MCB_LPF0_PRE_THR		(0x10)
#define	MCB_OSF1_MN			(0x08)
#define	MCB_OSF0_MN			(0x04)
#define	MCB_OSF1_ENB			(0x02)
#define	MCB_OSF0_ENB			(0x01)
#define	MCI_LPF_THR_DEF			(MCB_LPF0_PST_THR|MCB_LPF0_PRE_THR)

#define	MCI_DAC_DCC_SEL			(3)
#define	MCI_DAC_DCC_SEL_DEF		(0x0A)

#define	MCI_DET_LVL			(4)

#define	MCI_ECLK_SEL			(6)

#define	MCI_OSF_SEL			(8)
#define	MCI_OSF_SEL_DEF			(0x0F)

#define	MCI_SYSEQ			(9)
#define	MCB_SYSEQ_SYSEQ1_ENB		(0x02)
#define	MCB_SYSEQ_SYSEQ0_ENB		(0x01)
#define	MCB_SYSEQ_SYSEQ1_B2_ENB		(0x40)
#define	MCB_SYSEQ_SYSEQ1_B1_ENB		(0x20)
#define	MCB_SYSEQ_SYSEQ1_B0_ENB		(0x10)
#define	MCB_SYSEQ_SYSEQ0_B2_ENB		(0x04)
#define	MCB_SYSEQ_SYSEQ0_B1_ENB		(0x02)
#define	MCB_SYSEQ_SYSEQ0_B0_ENB		(0x01)
#define	MCI_SYSEQ_DEF			(0x00)

#define	MCI_CLIP_MD			(10)

#define	MCI_CLIP_ATT			(11)

#define	MCI_CLIP_REL			(12)

#define	MCI_CLIP_G			(13)

#define	MCI_OSF_GAIN0_15_8		(14)
#define	MCI_OSF_GAIN0_15_8_DEF		(0x40)
#define	MCI_OSF_GAIN0_7_0		(15)
#define	MCI_OSF_GAIN0_7_0_DEF		(0x00)

#define	MCI_OSF_GAIN1_15_8		(16)
#define	MCI_OSF_GAIN1_15_8_DEF		(0x40)
#define	MCI_OSF_GAIN1_7_0		(17)
#define	MCI_OSF_GAIN1_7_0_DEF		(0x00)

#define	MCI_DCL_GAIN			(18)
#define	MCB_DCL1_OFF			(0x80)
#define	MCB_DCL0_OFF			(0x08)

#define	MCI_DCL0_LMT_14_8		(19)
#define	MCI_DCL0_LMT_14_8_DEF		(0x7F)
#define	MCI_DCL0_LMT_7_0		(20)
#define	MCI_DCL0_LMT_7_0_DEF		(0xFF)

#define	MCI_DCL1_LMT_14_8		(21)
#define	MCI_DCL1_LMT_14_8_DEF		(0x7F)
#define	MCI_DCL1_LMT_7_0		(22)
#define	MCI_DCL1_LMT_7_0_DEF		(0xFF)

#define	MCI_DITHER0			(23)
#define	MCI_DITHER0_DEF			(0x50)

#define	MCI_DITHER1			(24)
#define	MCI_DITHER1_DEF			(0x50)

#define	MCI_DNG0_ES1			(25)
#define	MCI_DNG0_DEF_ES1		(0xC9)

#define	MCI_DNG1_ES1			(26)
#define	MCI_DNG1_DEF_ES1		(0xC9)

#define	MCI_DNG_ON_ES1			(27)

#define	MCI_DIRECTPATH_ENB		(28)
#define	MCB_DIRECTPATH_ENB		(0x01)
#define	MCB_DIRECT_ENB_ADC		(0x04)
#define	MCB_DIRECT_ENB_DAC1		(0x02)
#define	MCB_DIRECT_ENB_DAC0		(0x01)

#define	MCI_DPATH_DA_V			(29)
#define	MCB_DPATH_DA_VFLAG		(0x02)
#define	MCB_DPATH_DA_VINTP		(0x01)
#define	MCI_DPATH_DA_V_DEF		(MCB_DPATH_DA_VFLAG|MCB_DPATH_DA_VINTP)

#define	MCI_DPATH_DA_VOL_L		(30)
#define	MCB_DPATH_DA_VSEP		(0x80)

#define	MCI_DPATH_DA_VOL_R		(31)

#define	MCI_DPATH_AD_V			(32)
#define	MCB_DPATH_AD_VFLAG		(0x02)
#define	MCB_DPATH_AD_VINTP		(0x01)
#define	MCI_DPATH_AD_V_DEF		(MCB_DPATH_AD_VFLAG|MCB_DPATH_AD_VINTP)

#define	MCI_DPATH_AD_VOL_L		(33)
#define	MCB_DPATH_AD_VSEP		(0x80)

#define	MCI_DPATH_AD_VOL_R		(34)

#define	MCI_ADJ_HOLD			(35)

#define	MCI_ADJ_CNT			(36)

#define	MCI_ADJ_MAX_15_8		(37)
#define	MCI_ADJ_MAX_7_0			(38)

#define	MCI_DSF0_FLT_TYPE		(41)
#define	MCB_DSF0R_PRE_FLT_TYPE		(0x20)
#define	MCB_DSF0L_PRE_FLT_TYPE		(0x10)
#define	MCB_DSF0_MN			(0x02)
#define	MCB_DSF0ENB			(0x01)

#define	MCI_DSF0_PRE_INPUT		(42)
#define	MCI_DSF0_PRE_INPUT_DEF		(0x10)

#define	MCI_DSF1_FLT_TYPE		(43)
#define	MCB_DSF1R_PRE_FLT_TYPE		(0x20)
#define	MCB_DSF1L_PRE_FLT_TYPE		(0x10)
#define	MCB_DSF1_MN			(0x02)
#define	MCB_DSF1ENB			(0x01)
#define	MCI_DSF1_FLT_TYPE_DEF		(MCB_DSF1_MN)

#define	MCI_DSF1_PRE_INPUT		(44)
#define	MCI_DSF1_PRE_INPUT_DEF		(0x22)

#define	MCI_DSF2_FLT_TYPE		(45)
#define	MCB_DSF2R_PRE_FLT_TYPE		(0x40)
#define	MCB_DSF2L_PRE_FLT_TYPE		(0x10)
#define	MCB_DSF2REFSEL			(0x08)
#define	MCB_DSF2REFBACK			(0x04)
#define	MCB_DSF2_MN			(0x02)
#define	MCB_DSF2ENB			(0x01)

#define	MCI_DSF2_PRE_INPUT		(46)
#define	MCI_DSF2_PRE_INPUT_DEF		(0x43)

#define	MCI_DSF_SEL			(47)

#define	MCI_ADC_DCC_SEL			(48)
#define	MCI_ADC_DCC_SEL_DEF_ES1		(0x2A)
#define	MCI_ADC_DCC_SEL_DEF		(0x15)

#define	MCI_ADC_DNG_ON			(49)

#define	MCI_ADC_DNG0_FW			(50)
#define	MCI_ADC_DNG0_FW_DEF		(0x0B)

#define	MCI_ADC_DNG0_TIM		(51)

#define	MCI_ADC_DNG0_ZERO_15_8		(52)
#define	MCI_ADC_DNG0_ZERO_7_0		(53)

#define	MCI_ADC_DNG0_TGT_15_8		(54)
#define	MCI_ADC_DNG0_TGT_7_0		(55)

#define	MCI_ADC_DNG1_FW			(56)
#define	MCI_ADC_DNG1_FW_DEF		(0x0B)

#define	MCI_ADC_DNG1_TIM		(57)

#define	MCI_ADC_DNG1_ZERO_15_8		(58)
#define	MCI_ADC_DNG1_ZERO_7_0		(59)

#define	MCI_ADC_DNG1_TGT_15_8		(60)
#define	MCI_ADC_DNG1_TGT_7_0		(61)

#define	MCI_ADC_DNG2_FW			(62)
#define	MCI_ADC_DNG2_FW_DEF		(0x0B)

#define	MCI_ADC_DNG2_TIM		(63)

#define	MCI_ADC_DNG2_ZERO_15_8		(64)
#define	MCI_ADC_DNG2_ZERO_7_0		(65)

#define	MCI_ADC_DNG2_TGT_15_8		(66)
#define	MCI_ADC_DNG2_TGT_7_0		(67)

#define	MCI_DEPOP0			(68)
#define	MCI_DEPOP0_DEF			(0x0A)

#define	MCI_DEPOP1			(69)
#define	MCI_DEPOP1_DEF			(0x0A)

#define	MCI_DEPOP2			(70)
#define	MCI_DEPOP2_DEF			(0x0A)

#define	MCI_PDM_MODE			(71)
#define	MCI_PDM_MODE_DEF		(0x08)

#define	MCI_PDM_LOAD_TIM		(72)
#define	MCB_PDM1_START			(0x80)
#define	MCB_PDM0_START			(0x08)

#define	MCI_PDM0L_FINE_DLY		(73)
#define	MCI_PDM0R_FINE_DLY		(74)
#define	MCI_PDM1L_FINE_DLY		(75)
#define	MCI_PDM1R_FINE_DLY		(76)

/*	E_REG = #77: E2DSP_CTRL

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| E2DSP |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  | E2DSP |
	| _HALT |       |       |       |       |       |       |  _RST |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E2DSP			(77)
#define	MCB_E2DSP_HALT			(0x80)
#define	MCB_E2DSP_RST			(0x01)
#define	MCI_E2DSP_DEF			(MCB_E2DSP_RST)

/*	E_REG = #78: E2REQ_0/E2RES_0

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|                            E2REQ_0                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                            E2RES_0                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E2REQ_0			(78)
#define MCI_E2RES_0			MCI_E2REQ_0
#define MCI_E2REQ_0_DEF			(0x00)
#define MCI_E2RES_0_DEF			MCI_E2REQ_0_DEF
#define MCB_E2REQ_0			(0xFF)
#define MCB_E2RES_0			MCB_E2REQ_0

/*	E_REG = #79: E2REQ_1/E2RES_1

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|                            E2REQ_1                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                            E2RES_1                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E2REQ_1			(79)
#define MCI_E2RES_1			MCI_E2REQ_1
#define MCI_E2REQ_1_DEF			(0x00)
#define MCI_E2RES_1_DEF			MCI_E2REQ_1_DEF
#define MCB_E2REQ_1			(0xFF)
#define MCB_E2RES_1			MCB_E2REQ_1

/*	E_REG = #80: E2REQ_2/E2RES_2

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|                            E2REQ_2                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                            E2RES_2                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E2REQ_2			(80)
#define MCI_E2RES_2			MCI_E2REQ_2
#define MCI_E2REQ_2_DEF			(0x00)
#define MCI_E2RES_2_DEF			MCI_E2REQ_2_DEF
#define MCB_E2REQ_2			(0xFF)
#define MCB_E2RES_2			MCB_E2REQ_2

/*	E_REG = #81: E2REQ_3/E2RES_3

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|                            E2REQ_3                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                            E2RES_3                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E2REQ_3			(81)
#define MCI_E2RES_3			MCI_E2REQ_3
#define MCI_E2REQ_3_DEF			(0x00)
#define MCI_E2RES_3_DEF			MCI_E2REQ_3_DEF
#define MCB_E2REQ_3			(0xFF)
#define MCB_E2RES_3			MCB_E2REQ_3

/*	E_REG = #82: E2REQ_4/E2RES_4

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|                            E2REQ_4                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                            E2RES_4                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E2REQ_4			(82)
#define MCI_E2RES_4			MCI_E2REQ_4
#define MCI_E2REQ_4_DEF			(0x00)
#define MCI_E2RES_4_DEF			MCI_E2REQ_4_DEF
#define MCB_E2REQ_4			(0xFF)
#define MCB_E2RES_4			MCB_E2REQ_4

/*	E_REG = #83: E2REQ_5/E2RES_5

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|                            E2REQ_5                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                            E2RES_5                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E2REQ_5			(83)
#define MCI_E2RES_5			MCI_E2REQ_5
#define MCI_E2REQ_5_DEF			(0x00)
#define MCI_E2RES_5_DEF			MCI_E2REQ_5_DEF
#define MCB_E2REQ_5			(0xFF)
#define MCB_E2RES_5			MCB_E2REQ_5

/*	E_REG = #84: E2REQ_6/E2RES_6

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|                            E2REQ_6                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                            E2RES_6                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E2REQ_6			(84)
#define MCI_E2RES_6			MCI_E2REQ_6
#define MCI_E2REQ_6_DEF			(0x00)
#define MCI_E2RES_6_DEF			MCI_E2REQ_6_DEF
#define MCB_E2REQ_6			(0xFF)
#define MCB_E2RES_6			MCB_E2REQ_6

/*	E_REG = #85: E2REQ_7/E2RES_7

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|                            E2REQ_7                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                            E2RES_7                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E2REQ_7			(85)
#define MCI_E2RES_7			MCI_E2REQ_7
#define MCI_E2REQ_7_DEF			(0x00)
#define MCI_E2RES_7_DEF			MCI_E2REQ_7_DEF
#define MCB_E2REQ_7			(0xFF)
#define MCB_E2RES_7			MCB_E2REQ_7

/*	E_REG = #86: E2COMMAND/E2STATUS

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|                           E2COMMAND                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                            E2STATUS                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define	MCI_E2COMMAND			(86)
#define MCI_E2STATUS			MCI_E2COMMAND
#define MCI_E2COMMAND_DEF		(0x00)
#define MCI_E2STATUS_DEF		MCI_E2COMMAND_DEF
#define MCB_E2COMMAND			(0xFF)
#define MCB_E2STATUS			MCB_E2COMMAND

#define	MCI_CH_SEL			(87)
#define	MCB_I2SOUT_ENB			(0x20)

#define	MCI_IMPSEL			(90)

#define	MCI_HPIMP_15_8			(91)
#define	MCI_HPIMP_7_0			(92)

#define	MCI_PLUG_REV			(93)

#define	MCI_E2_SEL			(94)

#define	MCI_DNG0			(96)
#define	MCI_DNG0_DEF			(0xC9)

#define	MCI_DNG1			(97)
#define	MCI_DNG1_DEF			(0xC9)

#define	MCI_DNG_ON			(98)


/*	C_REG	*/
/*	C_REG = #0: power management digital (CDSP) register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| CDSP_ |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |
	|SAVEOFF|       |       |       |       |       |       |       |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_PWM_CDSP_SAVEOFF		(0x80)
#define MCI_PWM_DIGITAL_CDSP		(0)

/*	C_REG = #1: CDSP input/output FIFO level (1) register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|          OFIFO_LVL            |         DFIFO_LVL             |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_OFIFO_LVL		(0xF0)
#define MCB_DEC_DFIFO_LVL		(0x0F)
#define MCI_DEC_FIFOLEVEL_1		(1)

/*	C_REG = #2: CDSP input/output FIFO level (2) register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|          RFIFO_LVL            |         EFIFO_LVL             |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_RFIFO_LVL		(0xF0)
#define MCB_DEC_EFIFO_LVL		(0x0F)
#define MCI_DEC_FIFOLEVEL_2		(2)

/*	C_REG = #3: CDSP input/output FIFO level (3) register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|  "0"  |  "0"  |  "0"  |  "0"  |           FFIFO_LVL           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_FFIFO_LVL		(0x0F)
#define MCI_DEC_FIFOLEVEL_3		(3)

/*	C_REG = #4: decoder play position1 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                         DEC_POS[31:24]                        |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_POS1			(0xFF)
#define MCI_DEC_POS1			(4)

/*	C_REG = #5: decoder play position2 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                         DEC_POS[23:16]                        |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_POS2			(0xFF)
#define MCI_DEC_POS2			(5)

/*	C_REG = #6: decoder play position3 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                         DEC_POS[15:8]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_POS3			(0xFF)
#define MCI_DEC_POS3			(6)

/*	C_REG = #7: decoder play position4 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          DEC_POS[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_POS4			(0xFF)
#define MCI_DEC_POS4			(7)

/*	C_REG = #8: encoder play position1 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                         ENC_POS[31:24]                        |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_POS1			(0xFF)
#define MCI_ENC_POS1			(8)

/*	C_REG = #9: encoder play position2 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                         ENC_POS[23:16]                        |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_POS2			(0xFF)
#define MCI_ENC_POS2			(9)

/*	C_REG = #10: encoder play position3 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                         ENC_POS[15:8]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_POS3			(0xFF)
#define MCI_ENC_POS3			(10)

/*	C_REG = #11: encoder play position4 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          ENC_POS[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_POS4			(0xFF)
#define MCI_ENC_POS4			(11)

/*	C_REG = #12: decoder error flag register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                            DEC_ERR                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_ERROR			(0xFF)
#define MCI_DEC_ERROR			(12)

/*	C_REG = #13: encoder error flag register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                            ENC_ERR                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_ERROR			(0xFF)
#define MCI_ENC_ERROR			(13)

/*	C_REG = #14: decoder FIFO reset register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  | FFIFO | EFIFO | RFIFO | OFIFO | DFIFO |
	|       |       |       | _RST  | _RST  | _RST  | _RST  | _RST  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_FFIFO_RST		(0x10)
#define MCB_DEC_EFIFO_RST		(0x08)
#define MCB_DEC_RFIFO_RST		(0x04)
#define MCB_DEC_OFIFO_RST		(0x02)
#define MCB_DEC_DFIFO_RST		(0x01)
#define MCI_DEC_FIFO_RST		(14)

/*	C_REG = #15: decoder start register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|OUT_STA|  "0"  |  "0"  | FSQ_  | ENC_  |CDSP_OU| OUT_  | DEC_  |
	| RT_SEL|       |       | START | START |T_START| START | START |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_OUT_START_SEL		(0x80)
#define MCB_DEC_FSQ_START		(0x10)
#define MCB_DEC_ENC_START		(0x08)
#define MCB_DEC_CDSP_OUT_START		(0x04)
#define MCB_DEC_OUT_START		(0x02)
#define MCB_DEC_DEC_START		(0x01)
#define MCI_DEC_START			(15)

/*	C_REG = #16: decoder FIFO ch setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| EFIFO | EFIFO |    EFIFO_CH   | CDSP  |  "0"  |    OFIFO_CH   |
	| _START| _START|               | _EFIFO|       |               |
	|  _SEL |       |               | _START|       |               |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_EFIFO_START_SEL		(0x80)
#define MCB_DEC_EFIFO_START		(0x40)
#define MCB_DEC_EFIFO_CH		(0x30)
#define MCB_DEC_EFIFO_CH_2_16		(0x00)
#define MCB_DEC_EFIFO_CH_4_16		(0x10)
#define MCB_DEC_EFIFO_CH_2_32		(0x20)
#define MCB_DEC_EFIFO_CH_4_32		(0x30)
#define MCB_DEC_CDSP_EFIFO_START	(0x08)
#define MCB_DEC_OFIFO_CH		(0x03)
#define MCB_DEC_OFIFO_CH_2_16		(0x00)
#define MCB_DEC_OFIFO_CH_4_16		(0x01)
#define MCB_DEC_OFIFO_CH_2_32		(0x02)
#define MCB_DEC_OFIFO_CH_4_32		(0x03)
#define MCI_DEC_FIFO_CH			(16)

/*	C_REG = #17: decoder start register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|RFIFO  |  "0"  |  "0"  |  "0"  |  "0"  |CDSP   |RFIFO  |  "0"  |
	| _START|       |       |       |       | _RFIFO| _START|       |
	|  _SEL |       |       |       |       | _START|       |       |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_RFIFO_START_SEL		(0X80)
#define MCB_DEC_CDSP_RFIFO_START	(0X04)
#define MCB_RFIFO_START			(0X02)
#define MCI_DEC_START2			(17)

/*	C_REG = #19: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL15                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL15			(0xFF)
#define MCI_DEC_CTL15			(19)

/*	C_REG = #20: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL14                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL14			(0xFF)
#define MCI_DEC_CTL14			(20)

/*	C_REG = #21: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL13                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL13			(0xFF)
#define MCI_DEC_CTL13			(21)

/*	C_REG = #22: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL12                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL12			(0xFF)
#define MCI_DEC_CTL12			(22)

/*	C_REG = #23: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL11                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL11			(0xFF)
#define MCI_DEC_CTL11			(23)

/*	C_REG = #24: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL10                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL10			(0xFF)
#define MCI_DEC_CTL10			(24)

/*	C_REG = #25: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL9                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL9			(0xFF)
#define MCI_DEC_CTL9			(25)

/*	C_REG = #26: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL8                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL8			(0xFF)
#define MCI_DEC_CTL8			(26)

/*	C_REG = #27: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL7                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL7			(0xFF)
#define MCI_DEC_CTL7			(27)

/*	C_REG = #28: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL6                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL6			(0xFF)
#define MCI_DEC_CTL6			(28)

/*	C_REG = #29: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL5                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL5			(0xFF)
#define MCI_DEC_CTL5			(29)

/*	C_REG = #30: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL4                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL4			(0xFF)
#define MCI_DEC_CTL4			(30)

/*	C_REG = #31: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL3                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL3			(0xFF)
#define MCI_DEC_CTL3			(31)

/*	C_REG = #32: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL2                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL2			(0xFF)
#define MCI_DEC_CTL2			(32)

/*	C_REG = #33: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL1                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL1			(0xFF)
#define MCI_DEC_CTL1			(33)

/*	C_REG = #34: decoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_CTL0                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_CTL0			(0xFF)
#define MCI_DEC_CTL0			(34)

/*	C_REG = #35: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR15                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR15			(0xFF)
#define MCI_DEC_GPR15			(35)

/*	C_REG = #36: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR14                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR14			(0xFF)
#define MCI_DEC_GPR14			(36)

/*	C_REG = #37: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR13                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR13			(0xFF)
#define MCI_DEC_GPR13			(37)

/*	C_REG = #38: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR12                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR12			(0xFF)
#define MCI_DEC_GPR12			(38)

/*	C_REG = #39: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR11                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR11		(0xFF)
#define MCI_DEC_GPR11		(39)

/*	C_REG = #40: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR10                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR10			(0xFF)
#define MCI_DEC_GPR10			(40)

/*	C_REG = #41: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR9                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR9			(0xFF)
#define MCI_DEC_GPR9			(41)

/*	C_REG = #42: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR8                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR8			(0xFF)
#define MCI_DEC_GPR8			(42)

/*	C_REG = #43: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR7                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR7			(0xFF)
#define MCI_DEC_GPR7			(43)

/*	C_REG = #44: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR6                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR6			(0xFF)
#define MCI_DEC_GPR6			(44)

/*	C_REG = #45: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR5                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR5			(0xFF)
#define MCI_DEC_GPR5			(45)

/*	C_REG = #46: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR4                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR4			(0xFF)
#define MCI_DEC_GPR4			(46)

/*	C_REG = #47: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR3                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR3			(0xFF)
#define MCI_DEC_GPR3			(47)

/*	C_REG = #48: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR2                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR2			(0xFF)
#define MCI_DEC_GPR2			(48)

/*	C_REG = #49: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR1                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR1			(0xFF)
#define MCI_DEC_GPR1			(49)

/*	C_REG = #50: decoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           DEC_GPR0                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR0			(0xFF)
#define MCI_DEC_GPR0			(50)

/*	C_REG = #51: decoder special function register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_SFR1                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_SFR1			(0xFF)
#define MCI_DEC_SFR1			(51)

/*	C_REG = #52: decoder special function register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           DEC_SFR0                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_SFR0			(0xFF)
#define MCI_DEC_SFR0			(52)

/*	C_REG = #53: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL15                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL15			(0xFF)
#define MCI_ENC_CTL15			(53)

/*	C_REG = #54: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL14                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL14			(0xFF)
#define MCI_ENC_CTL14			(54)

/*	C_REG = #55: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL13                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL13			(0xFF)
#define MCI_ENC_CTL13			(55)

/*	C_REG = #56: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL12                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL12			(0xFF)
#define MCI_ENC_CTL12			(56)

/*	C_REG = #57: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL11                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL11			(0xFF)
#define MCI_ENC_CTL11			(57)

/*	C_REG = #58: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL10                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL10			(0xFF)
#define MCI_ENC_CTL10			(58)

/*	C_REG = #59: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL9                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL9			(0xFF)
#define MCI_ENC_CTL9			(59)

/*	C_REG = #60: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL8                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL8			(0xFF)
#define MCI_ENC_CTL8			(60)

/*	C_REG = #61: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL7                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL7			(0xFF)
#define MCI_ENC_CTL7			(61)

/*	C_REG = #62: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL6                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL6			(0xFF)
#define MCI_ENC_CTL6			(62)

/*	C_REG = #63: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL5                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL5			(0xFF)
#define MCI_ENC_CTL5			(63)

/*	C_REG = #64: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL4                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL4			(0xFF)
#define MCI_ENC_CTL4			(64)

/*	C_REG = #65: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL3                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL3			(0xFF)
#define MCI_ENC_CTL3			(65)

/*	C_REG = #66: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL2                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL2			(0xFF)
#define MCI_ENC_CTL2			(66)

/*	C_REG = #67: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL1                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL1			(0xFF)
#define MCI_ENC_CTL1			(67)

/*	C_REG = #68: encoder control setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_CTL0                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_CTL0			(0xFF)
#define MCI_ENC_CTL0			(68)

/*	C_REG = #69: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR15                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR15			(0xFF)
#define MCI_ENC_GPR15			(69)

/*	C_REG = #70: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR14                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR14			(0xFF)
#define MCI_ENC_GPR14			(70)

/*	C_REG = #71: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR13                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR13			(0xFF)
#define MCI_ENC_GPR13			(71)

/*	C_REG = #72: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR12                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR12			(0xFF)
#define MCI_ENC_GPR12			(72)

/*	C_REG = #73: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR11                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR11			(0xFF)
#define MCI_ENC_GPR11			(73)

/*	C_REG = #74: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR10                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR10			(0xFF)
#define MCI_ENC_GPR10			(74)

/*	C_REG = #75: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR9                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR9			(0xFF)
#define MCI_ENC_GPR9			(75)

/*	C_REG = #76: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR8                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR8			(0xFF)
#define MCI_ENC_GPR8			(76)

/*	C_REG = #77: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR7                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR7			(0xFF)
#define MCI_ENC_GPR7			(77)

/*	C_REG = #78: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR6                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR6			(0xFF)
#define MCI_ENC_GPR6			(78)

/*	C_REG = #79: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR5                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR5			(0xFF)
#define MCI_ENC_GPR5			(79)

/*	C_REG = #80: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR4                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR4			(0xFF)
#define MCI_ENC_GPR4			(80)

/*	C_REG = #81: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR3                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR3			(0xFF)
#define MCI_ENC_GPR3			(81)

/*	C_REG = #82: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR2                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR2			(0xFF)
#define MCI_ENC_GPR2			(82)

/*	C_REG = #83: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR1                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR1			(0xFF)
#define MCI_ENC_GPR1			(83)

/*	C_REG = #84: encoder general purpose register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           ENC_GPR0                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR0			(0xFF)
#define MCI_ENC_GPR0			(84)

/*	C_REG = #85: encoder special function register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_SFR1                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_SFR1			(0xFF)
#define MCI_ENC_SFR1			(85)

/*	C_REG = #86: encoder special function register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           ENC_SFR0                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_SFR0			(0xFF)
#define MCI_ENC_SFR0			(86)

/*	C_REG = #87: CDSP DFIFO pointer	(H) register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|  "0"  |  "0"  |  "0"  |          DFIFO_POINTER[12:8]          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DFIFO_POINTER_H		(0x1F)
#define MCI_DFIFO_POINTER_H		(87)

/*	C_REG = #88: CDSP DFIFO pointer	(L) register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                       DFIFO_POINTER[7:0]                      |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DFIFO_POINTER_L		(0xFF)
#define MCI_DFIFO_POINTER_L		(88)

/*	C_REG = #89: CDSP FFIFO pointer	(H) register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  FFIFO_POINTER[10:8]  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_FFIFO_POINTER_H		(0x07)
#define MCI_FFIFO_POINTER_H		(89)

/*	C_REG = #90: CDSP FFIFO pointer	(L) register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                       FFIFO_POINTER[7:0]                      |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_FFIFO_POINTER_L		(0xFF)
#define MCI_FFIFO_POINTER_L		(90)

/*	C_REG = #91: CDSP DFIFO flag register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| JDEMP | JDPNT |  "0"  |  "0"  | DOVF  | DUDF  | DEMP  | DPNT  |
	|       |       |       |       | _FLG  | _FLG  | _FLG  | _FLG  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DFIFO_FLG_JDEMP		(0x80)
#define MCB_DFIFO_FLG_JDPNT		(0x40)
#define MCB_DFIFO_FLG_DOVF		(0x08)
#define MCB_DFIFO_FLG_DUDF		(0x04)
#define MCB_DFIFO_FLG_DEMP		(0x02)
#define MCB_DFIFO_FLG_DPNT		(0x01)
#define MCB_DFIFO_FLG_ALL		(0x0F)
#define MCI_DFIFO_FLG			(91)

/*	C_REG = #92: CDSP OFIFO flag register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| JOEMP | JOPNT |  "0"  |  "0"  | OOVF  | OUDF  | OEMP  | OPNT  |
	|       |       |       |       | _FLG  | _FLG  | _FLG  | _FLG  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_OFIFO_FLG_JOEMP		(0x80)
#define MCB_OFIFO_FLG_JOPNT		(0x40)
#define MCB_OFIFO_FLG_OOVF		(0x08)
#define MCB_OFIFO_FLG_OUDF		(0x04)
#define MCB_OFIFO_FLG_OEMP		(0x02)
#define MCB_OFIFO_FLG_OPNT		(0x01)
#define MCB_OFIFO_FLG_ALL		(0x0F)
#define MCI_OFIFO_FLG			(92)

/*	C_REG = #93: CDSP EFIFO flag register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| JEEMP | JEPNT |  "0"  |  "0"  | EOVF  | EUDF  | EEMP  | EPNT  |
	|       |       |       |       | _FLG  | _FLG  | _FLG  | _FLG  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_EFIFO_FLG_JEEMP		(0x80)
#define MCB_EFIFO_FLG_JEPNT		(0x40)
#define MCB_EFIFO_FLG_EOVF		(0x08)
#define MCB_EFIFO_FLG_EUDF		(0x04)
#define MCB_EFIFO_FLG_EEMP		(0x02)
#define MCB_EFIFO_FLG_EPNT		(0x01)
#define MCB_EFIFO_FLG_ALL		(0x0F)
#define MCI_EFIFO_FLG			(93)

/*	C_REG = #94: CDSP RFIFO flag register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| JREMP | JRPNT |  "0"  |  "0"  | ROVF  | RUDF  | REMP  | RPNT  |
	|       |       |       |       | _FLG  | _FLG  | _FLG  | _FLG  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_RFIFO_FLG_JREMP		(0x80)
#define MCB_RFIFO_FLG_JRPNT		(0x40)
#define MCB_RFIFO_FLG_ROVF		(0x08)
#define MCB_RFIFO_FLG_RUDF		(0x04)
#define MCB_RFIFO_FLG_REMP		(0x02)
#define MCB_RFIFO_FLG_RPNT		(0x01)
#define MCB_RFIFO_FLG_ALL		(0x0F)
#define MCI_RFIFO_FLG			(94)

/*	C_REG = #95: CDSP FFIFO flag register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| JFEMP | JFPNT |FSQ_END|  "0"  | FOVF  |  "0"  | FEMP  | FPNT  |
	|       |       | _FLG  |       | _FLG  |       | _FLG  | _FLG  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_FFIFO_FLG_JFEMP		(0x80)
#define MCB_FFIFO_FLG_JFPNT		(0x40)
#define MCB_FFIFO_FLG_FSQ_END		(0x20)
#define MCB_FFIFO_FLG_FOVF		(0x08)
#define MCB_FFIFO_FLG_FEMP		(0x02)
#define MCB_FFIFO_FLG_FPNT		(0x01)
#define MCB_FFIFO_FLG_ALL		(0x2B)
#define MCI_FFIFO_FLG			(95)

/*	C_REG = #96: CDSP decoder flag register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|DEC_EPARAM_FLG |  "0"  |DEC_EVT|  "0"  |DEC_END|DEC_ERR|DEC_SFR|
	|               |       | _FLG  |       | _FLG  | _FLG  | _FLG  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_EPARAM_FLG		(0xC0)
#define MCB_DEC_EVT_FLG			(0x10)
#define MCB_DEC_FLG_END			(0x04)
#define MCB_DEC_FLG_ERR			(0x02)
#define MCB_DEC_FLG_SFR			(0x01)
#define MCB_DEC_FLG_ALL			(0xD7)
#define MCI_DEC_FLG			(96)

/*	C_REG = #97: CDSP encoder flag register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|ENC_EPARAM_FLG |  "0"  |ENC_EVT|  "0"  |ENC_END|ENC_ERR|ENC_SFR|
	|               |       | _FLG  |       | _FLG  | _FLG  | _FLG  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_EPARAM_FLG		(0xC0)
#define MCB_ENC_EVT_FLG			(0x10)
#define MCB_ENC_FLG_END			(0x04)
#define MCB_ENC_FLG_ERR			(0x02)
#define MCB_ENC_FLG_SFR			(0x01)
#define MCB_ENC_FLG_ALL			(0xD7)
#define MCI_ENC_FLG			(97)

/*	C_REG = #98: CDSP decoder general purpose flag register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          DEC_GPR_FLG                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_GPR_FLG			(0xFF)
#define MCI_DEC_GPR_FLG			(98)

/*	C_REG = #99: CDSP encoder general purpose flag register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          ENC_GPR_FLG                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_GPR_FLG			(0xFF)
#define MCI_ENC_GPR_FLG			(99)

/*	C_REG = #100: CDSP DFIFO enable register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  | EDOVF | EDUDF | EDEMP | EDPNT |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DFIFO_EDOVF			(0x08)
#define MCB_DFIFO_EDUDF			(0x04)
#define MCB_DFIFO_EDEMP			(0x02)
#define MCB_DFIFO_EDPNT			(0x01)
#define MCI_DFIFO_ENABLE		(100)

/*	C_REG = #101: CDSP OFIFO enable register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  | EOOVF | EOUDF | EOEMP | EOPNT |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_OFIFO_EOOVF			(0x08)
#define MCB_OFIFO_EOUDF			(0x04)
#define MCB_OFIFO_EOEMP			(0x02)
#define MCB_OFIFO_EOPNT			(0x01)
#define MCI_OFIFO_ENABLE		(101)

/*	C_REG = #102: CDSP EFIFO enable register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  | EEOVF | EEUDF | EEEMP | EEPNT |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_EFIFO_EEOVF			(0x08)
#define MCB_EFIFO_EEUDF			(0x04)
#define MCB_EFIFO_EEEMP			(0x02)
#define MCB_EFIFO_EEPNT			(0x01)
#define MCI_EFIFO_ENABLE		(102)

/*	C_REG = #103: CDSP RFIFO enable register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  | EROVF | ERUDF | EREMP | ERPNT |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_RFIFO_EROVF			(0x08)
#define MCB_RFIFO_ERUDF			(0x04)
#define MCB_RFIFO_EREMP			(0x02)
#define MCB_RFIFO_ERPNT			(0x01)
#define MCI_RFIFO_ENABLE		(103)

/*	C_REG = #104: CDSP FFIFO enable register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  | EFSQ  |  "0"  | EFOVF |  "0"  | EFEMP | EFPNT |
	|       |       | _END  |       |       |       |       |       |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_FFIFO_EFSQ_END		(0x20)
#define MCB_FFIFO_EFOVF			(0x08)
#define MCB_FFIFO_EFEMP			(0x02)
#define MCB_FFIFO_EFPNT			(0x01)
#define MCI_FFIFO_ENABLE		(104)

/*	C_REG = #105: CDSP decoder enable register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  EDEC_EPARAM  |  "0"  | EDEC  |  "0"  | EDEC  | EDEC  | EDEC  |
	|               |       | _EVT  |       | _END  | _ERR  | _SFR  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_EDEC_EPARAM			(0xC0)
#define MCB_EDEC_EVT			(0x10)
#define MCB_EDEC_END			(0x04)
#define MCB_EDEC_ERR			(0x02)
#define MCB_EDEC_SFR			(0x01)
#define MCI_DEC_ENABLE			(105)

/*	C_REG = #106: CDSP encoder enable register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  EENC_EPARAM  |  "0"  | EENC  |  "0"  | EENC  | EENC  | EENC  |
	|               |       | _EVT  |       | _END  | _ERR  | _SFR  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_EENC_EPARAM			(0xC0)
#define MCB_EENC_EVT			(0x10)
#define MCB_EENC_END			(0x04)
#define MCB_EENC_ERR			(0x02)
#define MCB_EENC_SFR			(0x01)
#define MCI_ENC_ENABLE			(106)

/*	C_REG = #107: CDSP decoder general purpose enable register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           EDEC_GPR                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_EDEC_GPR			(0xFF)
#define MCI_DEC_GPR_ENABLE		(107)

/*	C_REG = #108: CDSP encoder general purpose enable register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                           EENC_GPR                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_EENC_GPR			(0xFF)
#define MCI_ENC_GPR_ENABLE		(108)

/*	C_REG = #110: CDSP reset register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|   CDSP_MSEL   | CDSP_ | CDSP_ |  "0"  | FSQ_  |  "0"  | CDSP_ |
	|               | DMODE | FMODE |       | SRST  |       | SRST  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_CDSP_MSEL			(0xC0)
#define MCB_CDSP_MSEL_PROG		(0x00)
#define MCB_CDSP_MSEL_DATA		(0x40)
#define MCB_CDSP_DMODE			(0x20)
#define MCB_CDSP_FMODE			(0x10)
#define MCB_CDSP_FSQ_SRST		(0x04)
#define MCB_CDSP_SRST			(0x01)
#define MCI_CDSP_RESET			(110)

/*	C_REG = #112: CDSP power mode register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	| SLEEP |  "0"  | CDSP_HLT_MODE |  "0"  |  "0"  |  "0"  |  "0"  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_CDSP_SLEEP			(0x80)
#define MCB_CDSP_HLT_MODE		(0x30)
#define MCB_CDSP_HLT_MODE_IDLE		(0x10)
#define MCB_CDSP_HLT_MODE_SLEEP_HALT	(0x20)
#define MCI_CDSP_POWER_MODE		(112)

/*	C_REG = #113: CDSP error register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                         CDSP_ERR[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_CDSP_ERR			(0xFF)
#define MCI_CDSP_ERR			(113)

/*	C_REG = #114: CDSP memory address	(H) setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                         CDSP_MAR[15:8]                        |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_CDSP_MAR_H			(0xFF)
#define MCI_CDSP_MAR_H			(114)

/*	C_REG = #115: CDSP memory address	(L) setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                         CDSP_MAR[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_CDSP_MAR_L			(0xFF)
#define MCI_CDSP_MAR_L			(115)

/*	C_REG = #116: OFIFO irq point	(H) setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  |      OFIFO_IRQ_PNT[11:8]      |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_OFIFO_IRQ_PNT_H		(0x1F)
#define MCI_OFIFO_IRQ_PNT_H		(116)

/*	C_REG = #117: OFIFO irq point	(L) setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                       OFIFO_IRQ_PNT[7:0]                      |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_OFIFO_IRQ_PNT_L		(0xFF)
#define MCI_OFIFO_IRQ_PNT_L		(117)

/*	C_REG = #118: DFIFO irq point	(H) setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  |      DFIFO_IRQ_PNT[14:7]      |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DFIFO_IRQ_PNT_H		(0x0F)
#define MCI_DFIFO_IRQ_PNT_H		(118)

/*	C_REG = #119: dfifo irq point	(L) setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                       DFIFO_IRQ_PNT[7:0]                      |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DFIFO_IRQ_PNT_L		(0xFF)
#define MCI_DFIFO_IRQ_PNT_L		(119)

/*	C_REG = #120: RFIFO irq point	(H) setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |          RFIFO_IRQ_PNT[12:8]          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_RFIFO_IRQ_PNT_H		(0x1F)
#define MCI_RFIFO_IRQ_PNT_H		(120)

/*	C_REG = #121: RFIFO irq point (L) setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                       RFIFO_IRQ_PNT[7:0]                      |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_RFIFO_IRQ_PNT_L		(0xFF)
#define MCI_RFIFO_IRQ_PNT_L		(121)

/*	C_REG = #122: EFIFO irq point	(H) setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  |      EFIFO_IRQ_PNT[11:8]      |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_EFIFO_IRQ_PNT_H		(0x0F)
#define MCI_EFIFO_IRQ_PNT_H		(122)

/*	C_REG = #123: EFIFO irq point	(L) setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                       EFIFO_IRQ_PNT[7:0]                      |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_EFIFO_IRQ_PNT_L		(0xFF)
#define MCI_EFIFO_IRQ_PNT_L		(123)

/*	C_REG = #124: FFIFO irq point	(H) setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  | FFIFO_IRQ_PNT |
	|       |       |       |       |       |       |     [9:8]     |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_FFIFO_IRQ_PNT_H		(0x03)
#define MCI_FFIFO_IRQ_PNT_H		(124)

/*	C_REG = #125: FFIFO irq point	(L) setting register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                       FFIFO_IRQ_PNT[7:0]                      |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_FFIFO_IRQ_PNT_L		(0xFF)
#define MCI_FFIFO_IRQ_PNT_L		(125)

/*	C_REG = #128: CDSP flag register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |CDSP_  |  "0"  |CDSP_  |
	|       |       |       |       |       |ERR_FLG|       |WDT_FLG|
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_CDSP_FLG_ERR		(0x04)
#define MCB_CDSP_FLG_WDT		(0x01)
#define MCB_CDSP_FLG_ALL		(0x05)
#define MCI_CDSP_FLG			(128)

/*	C_REG = #129: CDSP enable register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  | ECDSP |  "0"  | ECDSP |
	|       |       |       |       |       | _ERR  |       | _WDT  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_CDSP_EERR			(0x04)
#define MCB_CDSP_EWDT			(0x01)
#define MCB_CDSP_ENABLE_ALL		(0x05)
#define MCI_CDSP_ENABLE			(129)

/*	C_REG = #130: CDSP RFIFO pointer	(H) register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|  "0"  |  "0"  |  "0"  |          RFIFO_POINTER[12:8]          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_RFIFO_POINTER_H		(0x1F)
#define MCI_RFIFO_POINTER_H		(130)

/*	C_REG = #131: CDSP RFIFO pointer	(L) register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                       RFIFO_POINTER[7:0]                      |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_RFIFO_POINTER_L		(0xFF)
#define MCI_RFIFO_POINTER_L		(131)

/*	C_REG = #144: Decoder event register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                            DEC_EVT                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_EVT			(0xFF)
#define MCI_DEC_EVT			(144)

/*	C_REG = #145: Decoder event parameter3 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          DEC_EPARAM3                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_EPARAM3			(0xFF)
#define MCI_DEC_EPARAM3			(145)

/*	C_REG = #146: Decoder event parameter2 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          DEC_EPARAM2                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_EPARAM2			(0xFF)
#define MCI_DEC_EPARAM2			(146)

/*	C_REG = #147: Decoder event parameter1 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          DEC_EPARAM1                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_EPARAM1			(0xFF)
#define MCI_DEC_EPARAM1			(147)

/*	C_REG = #148: Decoder event parameter0 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          DEC_EPARAM0                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_DEC_EPARAM0			(0xFF)
#define MCI_DEC_EPARAM0			(148)

/*	C_REG = #149: OUT0LR_SEL

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |    OUT0R_SEL[2:0]     |  "0"  |     OUT0L_SEL[2:0]    |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_OUT0_SEL_DEF		(0x10)
#define MCB_OUT0R_SEL			(0x70)
#define MCB_OUT0L_SEL			(0x07)
#define MCI_OUT0_SEL			(149)

/*	C_REG = #150: OUT1LR_SEL

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |    OUT1R_SEL[2:0]     |  "0"  |     OUT1L_SEL[2:0]    |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_OUT1_SEL_DEF		(0x10)
#define MCB_OUT1R_SEL			(0x70)
#define MCB_OUT1L_SEL			(0x07)
#define MCI_OUT1_SEL			(150)

/*	C_REG = #151: OUT2LR_SEL

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |    OUT2R_SEL[2:0]     |  "0"  |     OUT2L_SEL[2:0]    |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_OUT2_SEL_DEF		(0x32)
#define MCB_OUT2R_SEL			(0x70)
#define MCB_OUT2L_SEL			(0x07)
#define MCI_OUT2_SEL			(151)


/*	C_REG = #152: Encoder event register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                            ENC_EVT                            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_EVT			(0xFF)
#define MCI_ENC_EVT			(152)

/*	C_REG = #153: Encoder event parameter3 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          ENC_EPARAM3                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_EPARAM3			(0xFF)
#define MCI_ENC_EPARAM3			(153)

/*	C_REG = #154: Encoder event parameter2 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          ENC_EPARAM2                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_EPARAM2			(0xFF)
#define MCI_ENC_EPARAM2			(154)

/*	C_REG = #155: Encoder event parameter1 register
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          ENC_EPARAM1                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_EPARAM1			(0xFF)
#define MCI_ENC_EPARAM1			(155)

/*	C_REG = #156: Encoder event parameter0 register

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          ENC_EPARAM0                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCB_ENC_EPARAM0			(0xFF)
#define MCI_ENC_EPARAM0			(156)


/*	C_REG = #157: RDFIFO_BIT_SEL

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| DFIFO | DFIFO | CDSP  |  "0"  | RFIFO_| RFIFO_| DFIFO_| DFIFO_|
	| _START| _START| _DFIFO|       |  BIT  |  SEL  |  BIT  |  SEL  |
	|  _SEL |       | _START|       |       |       |       |       |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_RDFIFO_BIT_SEL_DEF		(0x00)
#define MCB_RDFIFO_DFIFO_START_SEL	(0x80)
#define MCB_RDFIFO_DFIFO_START		(0x40)
#define MCB_RDFIFO_CDSP_DFIFO_START	(0x20)
#define MCB_RFIFO_BIT			(0x08)
#define MCB_RFIFO_BIT_16		(0x00)
#define MCB_RFIFO_BIT_32		(0x08)
#define MCB_RFIFO_SEL			(0x04)
#define MCB_RFIFO_SEL_SRC		(0x00)
#define MCB_RFIFO_SEL_HOST		(0x04)
#define MCB_DFIFO_BIT			(0x02)
#define MCB_DFIFO_BIT_16		(0x00)
#define MCB_DFIFO_BIT_32		(0x02)
#define MCB_DFIFO_SEL			(0x01)
#define MCB_DFIFO_SEL_SRC		(0x00)
#define MCB_DFIFO_SEL_HOST		(0x01)
#define MCI_RDFIFO_BIT_SEL		(157)


/*	C_REG = #158: EFIFO01_SEL

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |    EFIFO01_SEL[2:0]   |  "0"  |    EFIFO00_SEL[2:0]   |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_EFIFO01_SEL_DEF		(0x32)
#define MCB_EFIFO1_SEL			(0x70)
#define MCB_EFIFO0_SEL			(0x07)
#define MCI_EFIFO01_SEL			(158)

/*	C_REG = #159: EFIFO23_SEL

	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |    EFIFO03_SEL[2:0]   |  "0"  |    EFIFO02_SEL[2:0]   |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_EFIFO23_SEL_DEF		(0x54)
#define MCB_EFIFO3_SEL			(0x70)
#define MCB_EFIFO2_SEL			(0x07)
#define MCI_EFIFO23_SEL			(159)


/*	F_REG	*/

/*	FDSP_ADR = #3: ADIDFmt0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| ADI3DFMT[1:0] | ADI2DFMT[1:0] | ADI1DFMT[1:0] | ADI0DFMT[1:0] |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADIDFMT0			(0x03)
#define MCI_ADIDFMT0_DEF		(0x00)
#define MCB_ADI3DFMT			(0xC0)
#define MCB_ADI2DFMT			(0x30)
#define MCB_ADI1DFMT			(0x0C)
#define MCB_ADI0DFMT			(0x03)

/*	FDSP_ADR = #4: ADIDFmt1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| ADI7DFMT[1:0] | ADI6DFMT[1:0] | ADI5DFMT[1:0] | ADI4DFMT[1:0] |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADIDFMT1			(0x04)
#define MCI_ADIDFMT1_DEF		(0x00)
#define MCB_ADI7DFMT			(0xC0)
#define MCB_ADI6DFMT			(0x30)
#define MCB_ADI5DFMT			(0x0C)
#define MCB_ADI4DFMT			(0x03)

/*	FDSP_ADR = #5: ADIMute0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| ADI07 | ADI06 | ADI05 | ADI04 | ADI03 | ADI02 | ADI01 | ADI00 |
	|  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADIMUTE0			(0x05)
#define MCI_ADIMUTE0_DEF		(0x00)
#define MCB_ADI07MTN			(0x80)
#define MCB_ADI06MTN			(0x40)
#define MCB_ADI05MTN			(0x20)
#define MCB_ADI04MTN			(0x10)
#define MCB_ADI03MTN			(0x08)
#define MCB_ADI02MTN			(0x04)
#define MCB_ADI01MTN			(0x02)
#define MCB_ADI00MTN			(0x01)

/*	FDSP_ADR = #6: ADIMute1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| ADI15 | ADI14 | ADI13 | ADI12 | ADI11 | ADI10 | ADI09 | ADI08 |
	|  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADIMUTE1			(0x06)
#define MCI_ADIMUTE1_DEF		(0x00)
#define MCB_ADI15MTN			(0x80)
#define MCB_ADI14MTN			(0x40)
#define MCB_ADI13MTN			(0x20)
#define MCB_ADI12MTN			(0x10)
#define MCB_ADI11MTN			(0x08)
#define MCB_ADI10MTN			(0x04)
#define MCB_ADI09MTN			(0x02)
#define MCB_ADI08MTN			(0x01)

/*	FDSP_ADR = #7: ADIMute2
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  | ADIMT |
	|       |       |       |       |       |       |       |  SET  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADIMUTE2			(0x07)
#define MCI_ADIMUTE2_DEF		(0x00)
#define MCB_ADIMTSET			(0x01)

/*	FDSP_ADR = #8: ADICSel0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADI01CSEL[3:0]         |        ADI00CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADICSEL0			(0x08)
#define MCI_ADICSEL0_DEF		(0x10)
#define MCB_ADI01CSEL			(0xF0)
#define MCB_ADI00CSEL			(0x0F)

/*	FDSP_ADR = #9: ADICSel1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADI03CSEL[3:0]         |        ADI02CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_ADICSEL1			(0x09)
#define MCI_ADICSEL1_DEF		(0x32)
#define MCB_ADI03CSEL			(0xF0)
#define MCB_ADI02CSEL			(0x0F)

/*	FDSP_ADR = #10: ADICSel2
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADI05CSEL[3:0]         |        ADI04CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADICSEL2			(0x0A)
#define MCI_ADICSEL2_DEF		(0x54)
#define MCB_ADI05CSEL			(0xF0)
#define MCB_ADI04CSEL			(0x0F)

/*	FDSP_ADR = #11: ADICSel3
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADI07CSEL[3:0]         |        ADI06CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADICSEL3			(0x0B)
#define MCI_ADICSEL3_DEF		(0x76)
#define MCB_ADI07CSEL			(0xF0)
#define MCB_ADI06CSEL			(0x0F)

/*	FDSP_ADR = #12: ADICSel4
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADI09CSEL[3:0]         |        ADI08CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADICSEL4			(0x0C)
#define MCI_ADICSEL4_DEF		(0x98)
#define MCB_ADI09CSEL			(0xF0)
#define MCB_ADI08CSEL			(0x0F)

/*	FDSP_ADR = #13: ADICSel5
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADI11CSEL[3:0]         |        ADI10CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADICSEL5			(0x0D)
#define MCI_ADICSEL5_DEF		(0xBA)
#define MCB_ADI11CSEL			(0xF0)
#define MCB_ADI10CSEL			(0x0F)

/*	FDSP_ADR = #14: ADICSel6
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADI13CSEL[3:0]         |        ADI12CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADICSEL6			(0x0E)
#define MCI_ADICSEL6_DEF		(0xDC)
#define MCB_ADI13CSEL			(0xF0)
#define MCB_ADI12CSEL			(0x0F)

/*	FDSP_ADR = #15: ADICSel7
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADI15CSEL[3:0]         |        ADI14CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADICSEL7			(0x0F)
#define MCI_ADICSEL7_DEF		(0xFE)
#define MCB_ADI15CSEL			(0xF0)
#define MCB_ADI14CSEL			(0x0F)

/*	FDSP_ADR = #19: ADODFmt0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| ADO3DFMT[1:0] | ADO2DFMT[1:0] | ADO1DFMT[1:0] | ADO0DFMT[1:0] |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADODFMT0			(0x13)
#define MCI_ADODFMT0_DEF		(0x00)
#define MCB_ADO3DFMT			(0xC0)
#define MCB_ADO2DFMT			(0x30)
#define MCB_ADO1DFMT			(0x0C)
#define MCB_ADO0DFMT			(0x03)

/*	FDSP_ADR = #20: ADODFmt1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| ADO7DFMT[1:0] | ADO6DFMT[1:0] | ADO5DFMT[1:0] | ADO4DFMT[1:0] |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADODFMT1			(0x14)
#define MCI_ADODFMT1_DEF		(0x00)
#define MCB_ADO7DFMT			(0xC0)
#define MCB_ADO6DFMT			(0x30)
#define MCB_ADO5DFMT			(0x0C)
#define MCB_ADO4DFMT			(0x03)

/*	FDSP_ADR = #21: ADOMute0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| ADO07 | ADO06 | ADO05 | ADO04 | ADO03 | ADO02 | ADO01 | ADO00 |
	|  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADOMUTE0			(0x15)
#define MCI_ADOMUTE0_DEF		(0x00)
#define MCB_ADO07MTN			(0x80)
#define MCB_ADO06MTN			(0x40)
#define MCB_ADO05MTN			(0x20)
#define MCB_ADO04MTN			(0x10)
#define MCB_ADO03MTN			(0x08)
#define MCB_ADO02MTN			(0x04)
#define MCB_ADO01MTN			(0x02)
#define MCB_ADO00MTN			(0x01)

/*	FDSP_ADR = #22: ADOMute1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	| ADO15 | ADO14 | ADO13 | ADO12 | ADO11 | ADO10 | ADO09 | ADO08 |
	|  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |  MTN  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADOMUTE1			(0x16)
#define MCI_ADOMUTE1_DEF		(0x00)
#define MCB_ADO15MTN			(0x80)
#define MCB_ADO14MTN			(0x40)
#define MCB_ADO13MTN			(0x20)
#define MCB_ADO12MTN			(0x10)
#define MCB_ADO11MTN			(0x08)
#define MCB_ADO10MTN			(0x04)
#define MCB_ADO09MTN			(0x02)
#define MCB_ADO08MTN			(0x01)

/*	FDSP_ADR = #23: ADOMute2
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  | ADOMT |
	|       |       |       |       |       |       |       |  SET  |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADOMUTE2			(0x17)
#define MCI_ADOMUTE2_DEF		(0x00)
#define MCB_ADOMTSET			(0x01)

/*	FDSP_ADR = #24: ADOCSel0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADO01CSEL[3:0]         |        ADO00CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADOCSEL0			(0x18)
#define MCI_ADOCSEL0_DEF		(0x10)
#define MCB_ADO01CSEL			(0xF0)
#define MCB_ADO00CSEL			(0x0F)

/*	FDSP_ADR = #25: ADOCSel1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADO03CSEL[3:0]         |        ADO02CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADOCSEL1			(0x19)
#define MCI_ADOCSEL1_DEF		(0x32)
#define MCB_ADO03CSEL			(0xF0)
#define MCB_ADO02CSEL			(0x0F)

/*	FDSP_ADR = #26: ADOCSel2
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADO05CSEL[3:0]         |        ADO04CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADOCSEL2			(0x1A)
#define MCI_ADOCSEL2_DEF		(0x54)
#define MCB_ADO05CSEL			(0xF0)
#define MCB_ADO04CSEL			(0x0F)

/*	FDSP_ADR = #27: ADOCSel3
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADO07CSEL[3:0]         |        ADO06CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADOCSEL3			(0x1B)
#define MCI_ADOCSEL3_DEF		(0x76)
#define MCB_ADO07CSEL			(0xF0)
#define MCB_ADO06CSEL			(0x0F)

/*	FDSP_ADR = #28: ADOCSel4
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADO09CSEL[3:0]         |        ADO08CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADOCSEL4			(0x1C)
#define MCI_ADOCSEL4_DEF		(0x98)
#define MCB_ADO09CSEL			(0xF0)
#define MCB_ADO08CSEL			(0x0F)

/*	FDSP_ADR = #29: ADOCSel5
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADO11CSEL[3:0]         |        ADO10CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADOCSEL5			(0x1D)
#define MCI_ADOCSEL5_DEF		(0xBA)
#define MCB_ADO11CSEL			(0xF0)
#define MCB_ADO10CSEL			(0x0F)

/*	FDSP_ADR = #30: ADOCSel6
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADO13CSEL[3:0]         |        ADO12CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADOCSEL6			(0x1E)
#define MCI_ADOCSEL6_DEF		(0xDC)
#define MCB_ADO13CSEL			(0xF0)
#define MCB_ADO12CSEL			(0x0F)

/*	FDSP_ADR = #31: ADOCSel7
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|        ADO15CSEL[3:0]         |        ADO14CSEL[3:0]         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADOCSEL7			(0x1F)
#define MCI_ADOCSEL7_DEF		(0xFE)
#define MCB_ADO15CSEL			(0xF0)
#define MCB_ADO14CSEL			(0x0F)

/*	FDSP_ADR = #32: DSPCtl
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  DSP  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  DSP  |
	| BYPASS|       |       |       |       |       |       | START |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_DSPCTRL			(0x20)
#define MCI_DSPCTRL_DEF			(0x00)
#define MCB_DSPBYPASS			(0x80)
#define MCB_DSPSTART			(0x01)

/*	FDSP_ADR = #33: DSPState
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|AUTOMTN|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  | DSPACT|
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_DSPSTATE			(0x21)
#define MCI_DSPSTATE_DEF		(0x00)
#define MCB_AUTOMTN			(0x80)
#define MCB_DSPACT			(0x01)

/*	FDSP_ADR = #34: ADIFs0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          ADIFS[15:8]                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADIFS0			(0x22)
#define MCI_ADIFS0_DEF			(0x00)
#define MCB_ADIFS0			(0xFF)

/*	FDSP_ADR = #35: ADIFs1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          ADIFS[7:0]                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADIFS1			(0x23)
#define MCI_ADIFS1_DEF			(0x00)
#define MCB_ADIFS1			(0xFF)

/*	FDSP_ADR = #36: ADOFs0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          ADOFS[15:8]                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_ADOFS0			(0x24)
#define MCI_ADOFS0_DEF			(0x00)
#define MCB_ADOFS0			(0xFF)

/*	FDSP_ADR = #37: ADOFs1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
R	|                          ADOFS[7:0]                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_ADOFS1			(0x25)
#define MCI_ADOFS1_DEF			(0x00)
#define MCB_ADOFS1			(0xFF)

/*	FDSP_ADR = #38: IReqApp0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          IRAPP[23:16]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_IREQAPP0			(0x26)
#define MCI_IREQAPP0_DEF		(0x00)
#define MCB_IREQAPP0			(0xFF)
#define MCB_IRAPP23			(0x80)
#define MCB_IRAPP22			(0x40)
#define MCB_IRAPP21			(0x20)
#define MCB_IRAPP20			(0x10)
#define MCB_IRAPP19			(0x08)
#define MCB_IRAPP18			(0x04)
#define MCB_IRAPP17			(0x02)
#define MCB_IRAPP16			(0x01)

/*	FDSP_ADR = #39: IReqApp1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          IRAPP[15:8]                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_IREQAPP1			(0x27)
#define MCI_IREQAPP1_DEF		(0x00)
#define MCB_IREQAPP1			(0xFF)
#define MCB_IRAPP15			(0x80)
#define MCB_IRAPP14			(0x40)
#define MCB_IRAPP13			(0x20)
#define MCB_IRAPP12			(0x10)
#define MCB_IRAPP11			(0x08)
#define MCB_IRAPP10			(0x04)
#define MCB_IRAPP09			(0x02)
#define MCB_IRAPP08			(0x01)

/*	FDSP_ADR = #40: IReqApp2
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          IRAPP[7:0]                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_IREQAPP2			(0x28)
#define MCI_IREQAPP2_DEF		(0x00)
#define MCB_IREQAPP2			(0xFF)
#define MCB_IRAPP07			(0x80)
#define MCB_IRAPP06			(0x40)
#define MCB_IRAPP05			(0x20)
#define MCB_IRAPP04			(0x10)
#define MCB_IRAPP03			(0x08)
#define MCB_IRAPP02			(0x04)
#define MCB_IRAPP01			(0x02)
#define MCB_IRAPP00			(0x01)

/*	FDSP_ADR = #41: IReqTop
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          IRTOP[7:0]                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_IREQTOP			(0x29)
#define MCI_IREQTOP_DEF			(0x00)
#define MCB_IREQTOP			(0x0F)
#define MCB_IREQTOP3			(0x08)
#define MCB_IREQTOP2			(0x04)
#define MCB_IREQTOP1			(0x02)
#define MCB_IREQTOP0			(0x01)

/*	FDSP_ADR = #64: FWMod
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|          FWMOD[3:0]           |            FS[3:0]            |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_FWMOD			(0x40)
#define MCI_FWMOD_DEF			(0x00)
#define MCB_FWMOD			(0xF0)
#define MCB_FS				(0x0F)

/*	FDSP_ADR = #65: AppExec0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                         APPEXEC[23:16]                        |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_APPEXEC0			(0x41)
#define MCI_APPEXEC0_DEF		(0x00)
#define MCB_APPEXEC0			(0xFF)
#define MCB_APPEXEC23			(0x80)
#define MCB_APPEXEC22			(0x40)
#define MCB_APPEXEC21			(0x20)
#define MCB_APPEXEC20			(0x10)
#define MCB_APPEXEC19			(0x08)
#define MCB_APPEXEC18			(0x04)
#define MCB_APPEXEC17			(0x02)
#define MCB_APPEXEC16			(0x01)

/*	FDSP_ADR = #66: AppExec1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                         APPEXEC[15:8]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_APPEXEC1			(0x42)
#define MCI_APPEXEC1_DEF		(0x00)
#define MCB_APPEXEC1			(0xFF)
#define MCB_APPEXEC15			(0x80)
#define MCB_APPEXEC14			(0x40)
#define MCB_APPEXEC13			(0x20)
#define MCB_APPEXEC12			(0x10)
#define MCB_APPEXEC11			(0x08)
#define MCB_APPEXEC10			(0x04)
#define MCB_APPEXEC09			(0x02)
#define MCB_APPEXEC08			(0x01)

/*	FDSP_ADR = #67: AppExec2
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          APPEXEC[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_APPEXEC2			(0x43)
#define MCI_APPEXEC2_DEF		(0x00)
#define MCB_APPEXEC2			(0xFF)
#define MCB_APPEXEC07			(0x80)
#define MCB_APPEXEC06			(0x40)
#define MCB_APPEXEC05			(0x20)
#define MCB_APPEXEC04			(0x10)
#define MCB_APPEXEC03			(0x08)
#define MCB_APPEXEC02			(0x04)
#define MCB_APPEXEC01			(0x02)
#define MCB_APPEXEC00			(0x01)

/*	FDSP_ADR = #68: AppAct0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                         APPACT[23:16]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_APPACT0			(0x44)
#define MCI_APPACT0_DEF			(0x00)
#define MCB_APPACT23			(0x80)
#define MCB_APPACT22			(0x40)
#define MCB_APPACT21			(0x20)
#define MCB_APPACT20			(0x10)
#define MCB_APPACT19			(0x08)
#define MCB_APPACT18			(0x04)
#define MCB_APPACT17			(0x02)
#define MCB_APPACT16			(0x01)

/*	FDSP_ADR = #69: AppAct1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          APPACT[15:8]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_APPACT1			(0x45)
#define MCI_APPACT1_DEF			(0x00)
#define MCB_APPACT15			(0x80)
#define MCB_APPACT14			(0x40)
#define MCB_APPACT13			(0x20)
#define MCB_APPACT12			(0x10)
#define MCB_APPACT11			(0x08)
#define MCB_APPACT10			(0x04)
#define MCB_APPACT09			(0x02)
#define MCB_APPACT08			(0x01)

/*	FDSP_ADR = #70: AppAct2
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           APPACT[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_APPACT2			(0x46)
#define MCI_APPACT2_DEF			(0x00)
#define MCB_APPACT07			(0x80)
#define MCB_APPACT06			(0x40)
#define MCB_APPACT05			(0x20)
#define MCB_APPACT04			(0x10)
#define MCB_APPACT03			(0x08)
#define MCB_APPACT02			(0x04)
#define MCB_APPACT01			(0x02)
#define MCB_APPACT00			(0x01)

/*	FDSP_ADR = #71: AppIEnb0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          APPIE[23:16]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_APPIENB0			(0x47)
#define MCI_APPIENB0_DEF		(0x00)
#define MCB_APPIE23			(0x80)
#define MCB_APPIE22			(0x40)
#define MCB_APPIE21			(0x20)
#define MCB_APPIE20			(0x10)
#define MCB_APPIE19			(0x08)
#define MCB_APPIE18			(0x04)
#define MCB_APPIE17			(0x02)
#define MCB_APPIE16			(0x01)

/*	FDSP_ADR = #72: AppIEnb1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           APPIE[15:8]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_APPIENB1			(0x48)
#define MCI_APPIENB1_DEF		(0x00)
#define MCB_APPIE15			(0x80)
#define MCB_APPIE14			(0x40)
#define MCB_APPIE13			(0x20)
#define MCB_APPIE12			(0x10)
#define MCB_APPIE11			(0x08)
#define MCB_APPIE10			(0x04)
#define MCB_APPIE09			(0x02)
#define MCB_APPIE08			(0x01)

/*	FDSP_ADR = #73: AppIEnb2
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           APPIE[7:0]                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_APPIENB2			(0x49)
#define MCI_APPIENB2_DEF		(0x00)
#define MCB_APPIE07			(0x80)
#define MCB_APPIE06			(0x40)
#define MCB_APPIE05			(0x20)
#define MCB_APPIE04			(0x10)
#define MCB_APPIE03			(0x08)
#define MCB_APPIE02			(0x04)
#define MCB_APPIE01			(0x02)
#define MCB_APPIE00			(0x01)

/*	FDSP_ADR = #74: AppFade0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                         APPFADE[23:16]                        |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/

#define MCI_APPFADE0			(0x4A)
#define MCI_APPFADE0_DEF		(0x00)
#define MCB_APPFADE23			(0x80)
#define MCB_APPFADE22			(0x40)
#define MCB_APPFADE21			(0x20)
#define MCB_APPFADE20			(0x10)
#define MCB_APPFADE19			(0x08)
#define MCB_APPFADE18			(0x04)
#define MCB_APPFADE17			(0x02)
#define MCB_APPFADE16			(0x01)

/*	FDSP_ADR = #75: AppFade1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          APPFADE[15:8]                        |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_APPFADE1			(0x4B)
#define MCI_APPFADE1_DEF		(0x00)
#define MCB_APPFADE15			(0x80)
#define MCB_APPFADE14			(0x40)
#define MCB_APPFADE13			(0x20)
#define MCB_APPFADE12			(0x10)
#define MCB_APPFADE11			(0x08)
#define MCB_APPFADE10			(0x04)
#define MCB_APPFADE09			(0x02)
#define MCB_APPFADE08			(0x01)

/*	FDSP_ADR = #76: AppFade2
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          APPFADE[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_APPFADE2			(0x4C)
#define MCI_APPFADE2_DEF		(0x00)
#define MCB_APPFADE07			(0x80)
#define MCB_APPFADE06			(0x40)
#define MCB_APPFADE05			(0x20)
#define MCB_APPFADE04			(0x10)
#define MCB_APPFADE03			(0x08)
#define MCB_APPFADE02			(0x04)
#define MCB_APPFADE01			(0x02)
#define MCB_APPFADE00			(0x01)

/*	FDSP_ADR = #77: FadeCode
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |  FADECODE[1:0]|
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_FADECODE			(0x4D)
#define MCI_FADECODE_DEF		(0x00)
#define MCB_FADECODE			(0x03)

/*	FDSP_ADR = #78: TopIEnb
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           TOPIE[7:0]                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_TOPIENB			(0x4E)
#define MCI_TOPIENB_DEF			(0x00)
#define MCB_TOPIENB			(0xFF)
#define MCB_TOPIE7			(0x80)
#define MCB_TOPIE6			(0x40)
#define MCB_TOPIE5			(0x20)
#define MCB_TOPIE4			(0x10)
#define MCB_TOPIE3			(0x08)
#define MCB_TOPIE2			(0x04)
#define MCB_TOPIE1			(0x02)
#define MCB_TOPIE0			(0x01)

/*	FDSP_ADR = #119: FWSel
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |                  FWSEL[5:0]                   |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_FWSEL			(0x77)
#define MCI_FWSEL_DEF			(0x00)
#define MCB_FWSEL			(0x3F)

/*	FDSP_ADR = #120: FWID
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           FWID[7:0]                           |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_FWID			(0x78)
#define MCI_FWID_DEF			(0x00)
#define MCB_FWID			(0xFF)

/*	FDSP_ADR = #121: FWVer0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          FWVER[23:16]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_FWVER0			(0x79)
#define MCI_FWVER0_DEF			(0x00)
#define MCB_FWVER0			(0xFF)

/*	FDSP_ADR = #122: FWVer1
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          FWVER[15:8]                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_FWVER1			(0x7A)
#define MCI_FWVER1_DEF			(0x00)
#define MCB_FWVER1			(0xFF)

/*	FDSP_ADR = #123: FWVer2
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           FWVER[7:0]                          |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_FWVER2			(0x7B)
#define MCI_FWVER2_DEF			(0x00)
#define MCB_FWVER2			(0xFF)

/*	FDSP_ADR = #124: FWState
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|  "0"  |  "0"  |  "0"  |  "0"  |  "0"  |      FWSTATE[2:0]     |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_FWSTATE			(0x7C)
#define MCI_FWSTATE_DEF			(0x00)
#define MCB_FWSTATE			(0x07)

/*	FDSP_ADR = #125: FWRetVal
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                         FWRETVAL[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_FWRETVAL			(0x7D)
#define MCI_FWRETVAL_DEF		(0x00)
#define MCB_FWRETVAL			(0xFF)

/*	FDSP_ADR = #126: FWLoad0
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                          FWLOAD[15:8]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_FWLOAD0			(0x7E)
#define MCI_FWLOAD0_DEF			(0x00)
#define MCB_FWLOAD0			(0xFF)

/*	FDSP_ADR = #127: FWLoad1
	   7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
W/R	|                           FWLOAD[7:0]                         |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
#define MCI_FWLOAD1			(0x7F)
#define MCI_FWLOAD1_DEF			(0x00)
#define MCI_FWLOAD1			(0x7F)


/*	ANA_REG	*/
#define	MCI_ANA_ID			(0)
#define	MCI_ANA_ID_DEF			(0x90)

#define	MCI_ANA_RST			(1)
#define	MCB_ANA_RST			(0x01)
#define	MCI_ANA_RST_DEF			(MCB_ANA_RST)

#define	MCI_AP				(2)
#define	MCB_AP_HPDET			(0x20)
#define	MCB_AP_LDOA			(0x10)
#define	MCB_AP_LDOD			(0x08)
#define	MCB_AP_BGR			(0x04)
#define	MCB_AP_CP			(0x02)
#define	MCB_AP_VR			(0x01)
#define	MCI_AP_DEF			(MCB_AP_VR	\
					|MCB_AP_CP	\
					|MCB_AP_BGR	\
					|MCB_AP_LDOA	\
					|MCB_AP_LDOD	\
					|MCB_AP_HPDET)

#define	MCI_AP_DA0			(3)
#define	MCB_AP_RC			(0x40)
#define	MCB_AP_HPR			(0x20)
#define	MCB_AP_HPL			(0x10)
#define	MCB_AP_LO1R			(0x08)
#define	MCB_AP_LO1L			(0x04)
#define	MCB_AP_DA0R			(0x02)
#define	MCB_AP_DA0L			(0x01)
#define	MCI_AP_DA0_DEF			(MCB_AP_DA0L	\
					|MCB_AP_DA0R	\
					|MCB_AP_LO1L	\
					|MCB_AP_LO1R	\
					|MCB_AP_HPL	\
					|MCB_AP_HPR	\
					|MCB_AP_RC)

#define	MCI_AP_DA1			(4)
#define	MCB_AP_SPR2			(0x80)
#define	MCB_AP_SPR1			(0x40)
#define	MCB_AP_SPL2			(0x20)
#define	MCB_AP_SPL1			(0x10)
#define	MCB_AP_LO2R			(0x08)
#define	MCB_AP_LO2L			(0x04)
#define	MCB_AP_DA1R			(0x02)
#define	MCB_AP_DA1L			(0x01)
#define	MCI_AP_DA1_DEF			(MCB_AP_DA1L	\
					|MCB_AP_DA1R	\
					|MCB_AP_LO2L	\
					|MCB_AP_LO2R	\
					|MCB_AP_SPL1	\
					|MCB_AP_SPL2	\
					|MCB_AP_SPR1	\
					|MCB_AP_SPR2)

#define	MCI_PN_DA			(5)
#define	MCB_PN_DA1			(0x02)
#define	MCB_PN_DA0			(0x01)

#define	MCI_AP_MIC			(6)
#define	MCB_MC4				(0x80)
#define	MCB_MC3				(0x40)
#define	MCB_MC2				(0x20)
#define	MCB_MC1				(0x10)
#define	MCB_MB4				(0x08)
#define	MCB_MB3				(0x04)
#define	MCB_MB2				(0x02)
#define	MCB_MB1				(0x01)
#define	MCI_AP_MIC_DEF			(MCB_MB1	\
					|MCB_MB2	\
					|MCB_MB3	\
					|MCB_MB4	\
					|MCB_MC1	\
					|MCB_MC2	\
					|MCB_MC3	\
					|MCB_MC4)

#define	MCI_AP_AD			(7)
#define	MCB_AP_LI			(0x80)
#define	MCB_AP_ADM			(0x04)
#define	MCB_AP_ADR			(0x02)
#define	MCB_AP_ADL			(0x01)
#define	MCI_AP_AD_DEF			(MCB_AP_LI	\
					|MCB_AP_ADM	\
					|MCB_AP_ADR	\
					|MCB_AP_ADL)

#define	MCI_PPD				(8)
#define	MCB_PPD_RC			(0x10)
#define	MCB_PPD_HP			(0x08)
#define	MCB_PPD_SP			(0x04)
#define	MCB_PPD_LO2			(0x02)
#define	MCB_PPD_LO1			(0x01)

#define	MCI_APM				(9)
#define	MCB_APMOFF			(0x01)

#define	MCI_BUSY1			(11)
#define	MCB_SPR_BUSY			(0x20)
#define	MCB_SPL_BUSY			(0x10)
#define	MCB_HPR_BUSY			(0x08)
#define	MCB_HPL_BUSY			(0x04)

#define	MCI_BUSY2			(12)
#define	MCB_LO2R_BUSY			(0x80)
#define	MCB_LO2L_BUSY			(0x40)
#define	MCB_LO1R_BUSY			(0x20)
#define	MCB_LO1L_BUSY			(0x10)
#define	MCB_RC_BUSY			(0x08)

#define	MCI_MBSEL			(13)

#define	MCI_KDSET			(14)
#define	MCB_MBS4_DISCH			(0x80)
#define	MCB_MBS3_DISCH			(0x40)
#define	MCB_MBS2_DISCH			(0x20)
#define	MCB_MBS1_DISCH			(0x10)
#define	MCB_KDSET2			(0x02)
#define	MCB_KDSET1			(0x01)

#define	MCI_NONCLIP			(21)
#define	MCI_NONCLIP_DEF			(0x03)

#define	MCI_DIF				(24)

#define	MCI_LIVOL_L			(25)
#define	MCB_ALAT_LI			(0x80)

#define	MCI_LIVOL_R			(26)

#define	MCI_MC1VOL			(27)
#define	MCI_MC2VOL			(28)
#define	MCI_MC3VOL			(29)
#define	MCI_MC4VOL			(30)

#define	MCI_HPVOL_L			(31)
#define	MCB_ALAT_HP			(0x80)

#define	MCI_HPVOL_R			(32)

#define	MCI_SPVOL_L			(33)
#define	MCB_ALAT_SP			(0x80)

#define	MCI_SPVOL_R			(34)

#define	MCI_RCVOL			(35)

#define	MCI_LO1VOL_L			(36)
#define	MCB_ALAT_LO1			(0x80)

#define	MCI_LO1VOL_R			(37)

#define	MCI_LO2VOL_L			(38)
#define	MCB_ALAT_LO2			(0x80)

#define	MCI_LO2VOL_R			(39)

#define	MCI_HPDETVOL			(40)

#define	MCI_SVOL			(41)
#define	MCB_SVOL_HP			(0x80)
#define	MCB_SVOL_SP			(0x40)
#define	MCB_SVOL_RC			(0x20)
#define	MCB_SVOL_LO2			(0x10)
#define	MCB_SVOL_LO1			(0x08)
#define	MCB_SVOL_HPDET			(0x04)
#define	MCI_SVOL_DEF			(MCB_SVOL_HP	\
					|MCB_SVOL_SP	\
					|MCB_SVOL_RC	\
					|MCB_SVOL_LO2	\
					|MCB_SVOL_LO1	\
					|MCB_SVOL_HPDET)

#define	MCI_HIZ				(42)
#define	MCB_HPR_HIZ			(0x80)
#define	MCB_HPL_HIZ			(0x40)
#define	MCB_SPR_HIZ			(0x20)
#define	MCB_SPL_HIZ			(0x10)
#define	MCB_RC_HIZ			(0x08)
#define	MCI_HIZ_DEF			(0xF8)

#define	MCI_LO_HIZ			(43)
#define	MCB_LO2R_HIZ			(0x80)
#define	MCB_LO2L_HIZ			(0x40)
#define	MCB_LO1R_HIZ			(0x20)
#define	MCB_LO1L_HIZ			(0x10)
#define	MCI_LO_HIZ_DEF			(0xF0)

#define	MCI_MCSNG			(44)
#define	MCB_MC4SNG			(0x80)
#define	MCB_MC3SNG			(0x40)
#define	MCB_MC2SNG			(0x20)
#define	MCB_MC1SNG			(0x10)

#define	MCI_RDY1			(45)
#define	MCB_VREF_RDY			(0x40)
#define	MCB_SPDY_R			(0x20)
#define	MCB_SPDY_L			(0x10)
#define	MCB_HPDY_R			(0x08)
#define	MCB_HPDY_L			(0x04)
#define	MCB_CPPDRDY			(0x02)
#define	MCB_HPDET_RDY			(0x01)

#define	MCI_RDY2			(46)
#define	MCB_LO2RDY_R			(0x80)
#define	MCB_LO2RDY_L			(0x40)
#define	MCB_LO1RDY_R			(0x20)
#define	MCB_LO1RDY_L			(0x10)
#define	MCB_RCRDY			(0x08)

#define	MCI_ZCOFF			(47)
#define	MCB_ZCOFF_HP			(0x80)
#define	MCB_ZCOFF_SP			(0x40)
#define	MCB_ZCOFF_RC			(0x20)
#define	MCB_ZCOFF_LO2			(0x10)
#define	MCB_ZCOFF_LO1			(0x08)
#define	MCI_ZCOFF_DEF_ES1		(MCB_ZCOFF_HP	\
					|MCB_ZCOFF_LO2	\
					|MCB_ZCOFF_LO1)

#define	MCI_ADL_MIX			(48)
#define	MCI_ADR_MIX			(49)
#define	MCI_ADM_MIX			(50)

#define	MCB_AD_M4MIX			(0x80)
#define	MCB_AD_M3MIX			(0x40)
#define	MCB_AD_M2MIX			(0x20)
#define	MCB_AD_M1MIX			(0x10)
#define	MCB_MONO_AD_LI			(0x02)
#define	MCB_AD_LIMIX			(0x01)

#define	MCI_DCERR			(51)
#define	MCB_DCERR			(0x10)
#define	MCB_L_OCPR			(0x04)
#define	MCB_L_OCPL			(0x02)
#define	MCB_OTP				(0x01)

#define	MCI_CPMOD			(53)
#define	MCB_HIP_DISABLE			(0x08)
#define	MCB_CPMOD			(0x01)
#define	MCI_CPMOD_DEF_ES1		(MCB_CPMOD)
#define	MCI_CPMOD_DEF			(MCB_HIP_DISABLE|MCB_CPMOD)

#define	MCI_DNG_ES1			(55)
#define	MCI_DNG_DEF_ES1			(0x14)

#define	MCI_DNG_HP_ES1			(56)
#define	MCI_DNG_HP_DEF_ES1		(0x05)

#define	MCI_DNG_SP_ES1			(57)
#define	MCI_DNG_SP_DEF_ES1		(0x05)

#define	MCI_DNG_RC_ES1			(58)
#define	MCI_DNG_RC_DEF_ES1		(0x05)

#define	MCI_DNG_LO1_ES1			(59)
#define	MCI_DNG_LO1_DEF_ES1		(0x05)

#define	MCI_DNG_LO2_ES1			(60)
#define	MCI_DNG_LO2_DEF_ES1		(0x05)

#define	MCI_RBSEL			(61)
#define	MCB_RBSEL			(0x80)
#define	MCB_OFC_MAN			(0x01)

#define	MCI_DNG				(108)
#define	MCI_DNG_DEF			(0x31)

#define	MCI_DNG_HP			(109)
#define	MCI_DNG_HP_DEF			(0x0F)

#define	MCI_DNG_SP			(110)
#define	MCI_DNG_SP_DEF			(0x0F)

#define	MCI_DNG_RC			(111)
#define	MCI_DNG_RC_DEF			(0x0F)

#define	MCI_DNG_LO1			(112)
#define	MCI_DNG_LO1_DEF			(0x0F)

#define	MCI_DNG_LO2			(113)
#define	MCI_DNG_LO2_DEF			(0x0F)


/*	CD_REG	*/
#define	MCI_HW_ID			(0)
#define	MCI_HW_ID_DEF			(0x90)

#define	MCI_CD_RST			(1)
#define	MCI_CD_RST_DEF			(0x01)

#define	MCI_DP				(2)
#define	MCB_DP_ADC			(0x40)
#define	MCB_DP_DAC1			(0x20)
#define	MCB_DP_DAC0			(0x10)
#define	MCB_DP_PDMCK			(0x04)
#define	MCB_DP_PDMADC			(0x02)
#define	MCB_DP_PDMDAC			(0x01)
#define	MCI_DP_DEF			(MCB_DP_ADC	\
					|MCB_DP_DAC1	\
					|MCB_DP_DAC0	\
					|MCB_DP_PDMCK	\
					|MCB_DP_PDMADC	\
					|MCB_DP_PDMDAC)

#define	MCI_DP_OSC			(3)
#define	MCB_DP_OSC			(0x01)
#define	MCI_DP_OSC_DEF			(MCB_DP_OSC)

#define	MCI_CKSEL			(4)
#define	MCB_CKSEL0			(0x10)
#define	MCB_CRTC			(0x02)
#define	MCB_HSDET			(0x01)
#define	MCI_CKSEL_DEF_ES1		(MCB_HSDET)
#define	MCI_CKSEL_DEF			(MCB_CRTC|MCB_HSDET)

#define	MCI_SCKMSKON_R			(5)

#define	MCI_IRQHS			(8)
#define	MCB_EIRQHS			(0x80)
#define	MCB_IRQHS			(0x40)

#define	MCI_EPLUGDET			(9)
#define	MCB_EPLUGDET			(0x80)
#define	MCB_EPLUGUNDET_DB		(0x02)
#define	MCB_EPLUGDET_DB			(0x01)

#define	MCI_PLUGDET			(10)
#define	MCB_PLUGDET			(0x80)
#define	MCB_SPLUGUNDET_DB		(0x02)
#define	MCB_SPLUGDET_DB			(0x01)

#define	MCI_PLUGDET_DB			(11)
#define	MCB_PLUGUNDET_DB		(0x02)
#define	MCB_PLUGDET_DB			(0x01)

#define	MCI_RPLUGDET			(12)
#define	MCB_RPLUGDET			(0x80)
#define	MCB_RPLUGUNDET_DB		(0x02)
#define	MCB_RPLUGDET_DB			(0x01)

#define	MCI_EDLYKEY			(13)

#define	MCI_SDLYKEY			(14)

#define	MCI_EMICDET			(15)

#define	MCI_SMICDET			(16)
#define	MCB_SMICDET			(0x40)

#define	MCI_MICDET			(17)
#define	MCB_MICDET			(0x40)
#define	MCI_MICDET_DEF			(0x38)

#define	MCI_RMICDET			(18)

#define	MCI_KEYCNTCLR0			(19)
#define	MCB_KEYCNTCLR0			(0x80)

#define	MCI_KEYCNTCLR1			(20)
#define	MCB_KEYCNTCLR1			(0x80)

#define	MCI_KEYCNTCLR2			(21)
#define	MCB_KEYCNTCLR2			(0x80)

#define	MCI_HSDETEN			(22)
#define	MCB_HSDETEN			(0x80)
#define	MCB_MKDETEN			(0x40)
#define	MCI_HSDETEN_DEF			(0x05)

#define	MCI_IRQTYPE			(23)
#define	MCI_IRQTYPE_DEF			(0xC3)

#define	MCI_DLYIRQSTOP			(24)

#define	MCI_DETIN_INV			(25)
#define	MCI_DETIN_INV_DEF_ES1		(0x01)
#define	MCI_DETIN_INV_DEF		(0xE3)

#define	MCI_IRQM_KEYOFF			(26)

#define	MCI_HSDETMODE			(28)
#define	MCI_HSDETMODE_DEF		(0x04)

#define	MCI_DBNC_PERIOD			(29)
#define	MCB_DBNC_LPERIOD		(0x02)
#define	MCI_DBNC_PERIOD_DEF		(0x22)

#define	MCI_DBNC_NUM			(30)
#define	MCI_DBNC_NUM_DEF		(0xBF)
#define	MCI_DBNC_NUM_DEF_ES1		(0x3F)

#define	MCI_KEY_MTIM			(31)
#define	MCI_KEY_MTIM_DEF		(0x80)

#define	MCI_KEYONDLYTIM_K0		(32)
#define	MCI_KEYOFFDLYTIM_K0		(33)

#define	MCI_KEYONDLYTIM_K1		(34)
#define	MCI_KEYOFFDLYTIM_K1		(35)

#define	MCI_KEYONDLYTIM_K2		(36)
#define	MCI_KEYOFFDLYTIM_K2		(37)

#define	MCI_EIRQSENSE			(48)
#define	MCB_EIRQSENSE			(0x80)
#define	MCB_EIRQOFC			(0x08)

#define	MCI_SSENSEFIN			(49)
#define	MCB_SSENSEFIN			(0x80)
#define	MCB_SENSE_BSY			(0x40)
#define	MCB_SOFFCANFIN			(0x08)
#define	MCB_OFFCAN_BSY			(0x04)

#define	MCI_STDPLUGSEL			(52)
#define	MCB_STDPLUGSEL			(0x80)

#endif /* __MCDEFS_H__ */
