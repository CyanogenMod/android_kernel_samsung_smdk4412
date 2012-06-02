/****************************************************************************
 *
 *		Copyright(c) 2010-2011 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcdriver.c
 *
 *		Description	: MC Driver
 *
 *		Version		: 1.0.2		2011.04.18
 *
 ****************************************************************************/


#include "mcdriver.h"
#include "mcdriver_AA.h"
#include "mcservice.h"



/*******************************************/

/*	Register Definition

	[Naming Rules]

	  MCI_AA_xxx		: Registers
	  MCI_AA_xxx_DEF	: Default setting of registers
	  MCB_AA_xxx		: Miscelleneous bit definition
*/

/*	Registers	*/
/*	A_ADR	*/
#define	MCI_AA_RST						(0)
#define	MCB_AA_RST						(0x01)
#define	MCI_AA_RST_DEF					(MCB_AA_RST)

#define	MCI_AA_BASE_ADR				(1)
#define	MCI_AA_BASE_WINDOW				(2)

#define	MCI_AA_HW_ID					(8)
#define	MCI_AA_HW_ID_DEF				(0x44)

#define	MCI_AA_ANA_ADR					(12)
#define	MCI_AA_ANA_WINDOW				(13)

#define	MCI_AA_CD_ADR					(14)
#define	MCI_AA_CD_WINDOW				(15)

#define	MCI_AA_MIX_ADR					(16)
#define	MCI_AA_MIX_WINDOW				(17)

#define	MCI_AA_AE_ADR					(18)
#define	MCI_AA_AE_WINDOW				(19)

#define	MCI_AA_BDSP_ST					(20)
#define	MCB_AA_EQ5ON					(0x80)
#define	MCB_AA_DRCON					(0x40)
#define	MCB_AA_EQ3ON					(0x20)
#define	MCB_AA_DBEXON					(0x08)
#define	MCB_AA_BDSP_ST					(0x01)

#define	MCI_AA_BDSP_RST				(21)
#define	MCB_AA_TRAM_RST				(0x02)
#define	MCB_AA_BDSP_RST				(0x01)

#define	MCI_AA_BDSP_ADR				(22)
#define	MCI_AA_BDSP_WINDOW				(23)

#define	MCI_AA_CDSP_ADR				(24)
#define	MCI_AA_CDSP_WINDOW				(25)

/*	B_ADR(BASE)	*/
#define	MCI_AA_RSTB					(0)
#define	MCB_AA_RSTB					(0x10)
#define	MCI_AA_RSTB_DEF				(MCB_AA_RSTB)

#define	MCI_AA_PWM_DIGITAL				(1)
#define	MCB_AA_PWM_DP2					(0x04)
#define	MCB_AA_PWM_DP1					(0x02)
#define	MCB_AA_PWM_DP0					(0x01)
#define	MCI_AA_PWM_DIGITAL_DEF			(MCB_AA_PWM_DP2 | MCB_AA_PWM_DP1 | MCB_AA_PWM_DP0)

#define	MCI_AA_PWM_DIGITAL_1			(3)
#define	MCB_AA_PWM_DPPDM				(0x10)
#define	MCB_AA_PWM_DPDI2				(0x08)
#define	MCB_AA_PWM_DPDI1				(0x04)
#define	MCB_AA_PWM_DPDI0				(0x02)
#define	MCB_AA_PWM_DPB					(0x01)
#define	MCI_AA_PWM_DIGITAL_1_DEF		(MCB_AA_PWM_DPPDM | MCB_AA_PWM_DPDI2 | MCB_AA_PWM_DPDI1 | MCB_AA_PWM_DPDI0 | MCB_AA_PWM_DPB)

#define	MCI_AA_PWM_DIGITAL_CDSP		(4)
#define	MCB_AA_PWM_DPCDSP				(0x00)
#define	MCI_AA_PWM_DIGITAL_CDSP_DEF	(MCB_AA_PWM_DPCDSP)

#define	MCI_AA_PWM_DIGITAL_BDSP		(6)
#define	MCI_AA_PWM_DIGITAL_BDSP_DEF	(MCB_AA_PWM_DPBDSP)
#define	MCB_AA_PWM_DPBDSP				(0x01)

#define	MCI_AA_SD_MSK					(9)
#define	MCB_AA_SDIN_MSK2				(0x80)
#define	MCB_AA_SDO_DDR2				(0x10)
#define	MCB_AA_SDIN_MSK1				(0x08)
#define	MCB_AA_SDO_DDR1				(0x01)
#define	MCI_AA_SD_MSK_DEF				(MCB_AA_SDIN_MSK2 | MCB_AA_SDIN_MSK1)

#define	MCI_AA_SD_MSK_1				(10)
#define	MCB_AA_SDIN_MSK0				(0x80)
#define	MCB_AA_SDO_DDR0				(0x10)
#define	MCI_AA_SD_MSK_1_DEF			(MCB_AA_SDIN_MSK0)

#define	MCI_AA_BCLK_MSK				(11)
#define	MCB_AA_BCLK_MSK2				(0x80)
#define	MCB_AA_BCLK_DDR2				(0x40)
#define	MCB_AA_LRCK_MSK2				(0x20)
#define	MCB_AA_LRCK_DDR2				(0x10)
#define	MCB_AA_BCLK_MSK1				(0x08)
#define	MCB_AA_BCLK_DDR1				(0x04)
#define	MCB_AA_LRCK_MSK1				(0x02)
#define	MCB_AA_LRCK_DDR1				(0x01)
#define	MCI_AA_BCLK_MSK_DEF			(MCB_AA_BCLK_MSK2 | MCB_AA_LRCK_MSK2 | MCB_AA_BCLK_MSK1 | MCB_AA_LRCK_MSK1)

#define	MCI_AA_BCLK_MSK_1				(12)
#define	MCB_AA_BCLK_MSK0				(0x80)
#define	MCB_AA_BCLK_DDR0				(0x40)
#define	MCB_AA_LRCK_MSK0				(0x20)
#define	MCB_AA_LRCK_DDR0				(0x10)
#define	MCB_AA_PCMOUT_HIZ2				(0x08)
#define	MCB_AA_PCMOUT_HIZ1				(0x04)
#define	MCB_AA_PCMOUT_HIZ0				(0x02)
#define	MCB_AA_ROUTER_MS				(0x01)
#define	MCI_AA_BCLK_MSK_1_DEF			(MCB_AA_BCLK_MSK0 | MCB_AA_LRCK_MSK0)

#define	MCI_AA_BCKP					(13)
#define	MCB_AA_DI2_BCKP				(0x04)
#define	MCB_AA_DI1_BCKP				(0x02)
#define	MCB_AA_DI0_BCKP				(0x01)
#define	MCI_AA_BCKP_DEF				(0)

#define	MCI_AA_PLL_RST					(14)
#define	MCB_AA_PLLRST1					(0x00)
#define	MCB_AA_PLLRST0					(0x01)
#define	MCI_AA_PLL_RST_DEF				(MCB_AA_PLLRST1 | MCB_AA_PLLRST0)

#define	MCI_AA_DIVR0					(15)
#define	MCI_AA_DIVR0_DEF				(0x0A)

#define	MCI_AA_DIVF0					(16)
#define	MCI_AA_DIVF0_DEF				(0x48)

#define	MCI_AA_DIVR1					(17)
#define	MCI_AA_DIVF1					(18)

#define	MCI_AA_CKSEL					(20)
#define	MCB_AA_CKSEL					(0x80)

#define	MCI_AA_BYPASS					(21)
#define	MCB_AA_LOCK1					(0x80)
#define	MCB_AA_LOCK0					(0x40)
#define	MCB_AA_BYPASS1					(0x02)
#define	MCB_AA_BYPASS0					(0x01)

#define	MCI_AA_EPA_IRQ					(22)
#define	MCB_AA_EPA2_IRQ				(0x04)
#define	MCB_AA_EPA1_IRQ				(0x02)
#define	MCB_AA_EPA0_IRQ				(0x01)

#define	MCI_AA_PA_FLG					(23)
#define	MCB_AA_PA2_FLAG				(0x04)
#define	MCB_AA_PA1_FLAG				(0x02)
#define	MCB_AA_PA0_FLAG				(0x01)

#define	MCI_AA_PA_MSK					(25)
#define	MCB_AA_PA2_MSK					(0x08)
#define	MCB_AA_PA2_DDR					(0x04)
#define	MCI_AA_PA_MSK_DEF				(MCB_AA_PA2_MSK)

#define	MCI_AA_PA_MSK_1				(26)
#define	MCB_AA_PA1_MSK					(0x80)
#define	MCB_AA_PA1_DDR					(0x40)
#define	MCB_AA_PA0_MSK					(0x08)
#define	MCB_AA_PA0_DDR					(0x04)
#define	MCI_AA_PA_MSK_1_DEF			(MCB_AA_PA1_MSK | MCB_AA_PA0_MSK)

#define	MCI_AA_PA_HOST					(28)
#define	MCI_AA_PA_HOST_1				(29)

#define	MCI_AA_PA_OUT					(30)
#define	MCB_AA_PA_OUT					(0x01)

#define	MCI_AA_PA_SCU_PA				(31)
#define	MCB_AA_PA_SCU_PA0				(0x01)
#define	MCB_AA_PA_SCU_PA1				(0x02)
#define	MCB_AA_PA_SCU_PA2				(0x04)

/*	B_ADR(MIX)	*/
#define	MCI_AA_DIT_INVFLAGL			(0)
#define	MCB_AA_DIT0_INVFLAGL			(0x20)
#define	MCB_AA_DIT1_INVFLAGL			(0x10)
#define	MCB_AA_DIT2_INVFLAGL			(0x08)

#define	MCI_AA_DIT_INVFLAGR			(1)
#define	MCB_AA_DIT0_INVFLAGR			(0x20)
#define	MCB_AA_DIT1_INVFLAGR			(0x10)
#define	MCB_AA_DIT2_INVFLAGR			(0x08)

#define	MCI_AA_DIR_VFLAGL				(2)
#define	MCB_AA_PDM0_VFLAGL				(0x80)
#define	MCB_AA_DIR0_VFLAGL				(0x20)
#define	MCB_AA_DIR1_VFLAGL				(0x10)
#define	MCB_AA_DIR2_VFLAGL				(0x08)

#define	MCI_AA_DIR_VFLAGR				(3)
#define	MCB_AA_PDM0_VFLAGR				(0x80)
#define	MCB_AA_DIR0_VFLAGR				(0x20)
#define	MCB_AA_DIR1_VFLAGR				(0x10)
#define	MCB_AA_DIR2_VFLAGR				(0x08)

#define	MCI_AA_AD_VFLAGL				(4)
#define	MCB_AA_ADC_VFLAGL				(0x80)
#define	MCB_AA_AENG6_VFLAGL			(0x20)

#define	MCI_AA_AD_VFLAGR				(5)
#define	MCB_AA_ADC_VFLAGR				(0x80)
#define	MCB_AA_AENG6_VFLAGR			(0x20)

#define	MCI_AA_AFLAGL					(6)
#define	MCB_AA_ADC_AFLAGL				(0x40)
#define	MCB_AA_DIR0_AFLAGL				(0x20)
#define	MCB_AA_DIR1_AFLAGL				(0x10)
#define	MCB_AA_DIR2_AFLAGL				(0x04)

#define	MCI_AA_AFLAGR					(7)
#define	MCB_AA_ADC_AFLAGR				(0x40)
#define	MCB_AA_DIR0_AFLAGR				(0x20)
#define	MCB_AA_DIR1_AFLAGR				(0x10)
#define	MCB_AA_DIR2_AFLAGR				(0x04)

#define	MCI_AA_DAC_INS_FLAG			(8)
#define	MCB_AA_DAC_INS_FLAG			(0x80)

#define	MCI_AA_INS_FLAG				(9)
#define	MCB_AA_ADC_INS_FLAG			(0x40)
#define	MCB_AA_DIR0_INS_FLAG			(0x20)
#define	MCB_AA_DIR1_INS_FLAG			(0x10)
#define	MCB_AA_DIR2_INS_FLAG			(0x04)

#define	MCI_AA_DAC_FLAGL				(10)
#define	MCB_AA_ST_FLAGL				(0x80)
#define	MCB_AA_MASTER_OFLAGL			(0x40)
#define	MCB_AA_VOICE_FLAGL				(0x10)
#define	MCB_AA_DAC_FLAGL				(0x02)

#define	MCI_AA_DAC_FLAGR				(11)
#define	MCB_AA_ST_FLAGR				(0x80)
#define	MCB_AA_MASTER_OFLAGR			(0x40)
#define	MCB_AA_VOICE_FLAGR				(0x10)
#define	MCB_AA_DAC_FLAGR				(0x02)

#define	MCI_AA_DIT0_INVOLL				(12)
#define	MCB_AA_DIT0_INLAT				(0x80)
#define	MCB_AA_DIT0_INVOLL				(0x7F)

#define	MCI_AA_DIT0_INVOLR				(13)
#define	MCB_AA_DIT0_INVOLR				(0x7F)

#define	MCI_AA_DIT1_INVOLL				(14)
#define	MCB_AA_DIT1_INLAT				(0x80)
#define	MCB_AA_DIT1_INVOLL				(0x7F)

#define	MCI_AA_DIT1_INVOLR				(15)
#define	MCB_AA_DIT1_INVOLR				(0x7F)

#define	MCI_AA_DIT2_INVOLL				(16)
#define	MCB_AA_DIT2_INLAT				(0x80)
#define	MCB_AA_DIT2_INVOLL				(0x7F)

#define	MCI_AA_DIT2_INVOLR				(17)
#define	MCB_AA_DIT2_INVOLR				(0x7F)

#define	MCI_AA_ESRC0_INVOLL			(16)
#define	MCI_AA_ESRC0_INVOLR			(17)

#define	MCI_AA_PDM0_VOLL				(24)
#define	MCB_AA_PDM0_INLAT				(0x80)
#define	MCB_AA_PDM0_VOLL				(0x7F)

#define	MCI_AA_PDM0_VOLR				(25)
#define	MCB_AA_PDM0_VOLR				(0x7F)

#define	MCI_AA_DIR0_VOLL				(28)
#define	MCB_AA_DIR0_LAT				(0x80)
#define	MCB_AA_DIR0_VOLL				(0x7F)

#define	MCI_AA_DIR0_VOLR				(29)
#define	MCB_AA_DIR0_VOLR				(0x7F)

#define	MCI_AA_DIR1_VOLL				(30)
#define	MCB_AA_DIR1_LAT				(0x80)
#define	MCB_AA_DIR1_VOLL				(0x7F)

#define	MCI_AA_DIR1_VOLR				(31)
#define	MCB_AA_DIR1_VOLR				(0x7F)

#define	MCI_AA_DIR2_VOLL				(32)
#define	MCB_AA_DIR2_LAT				(0x80)
#define	MCB_AA_DIR2_VOLL				(0x7F)

#define	MCI_AA_DIR2_VOLR				(33)
#define	MCB_AA_DIR2_VOLR				(0x7F)
/*
#define	MCI_AA_ADC1_VOLL				(38?)
#define	MCB_AA_ADC1_LAT				(0x80)
#define	MCB_AA_ADC1_VOLL				(0x7F)

#define	MCI_AA_ADC1_VOLR				(39?)
#define	MCB_AA_ADC1_VOLR				(0x7F)
*/
#define	MCI_AA_ADC_VOLL				(40)
#define	MCB_AA_ADC_LAT					(0x80)
#define	MCB_AA_ADC_VOLL				(0x7F)

#define	MCI_AA_ADC_VOLR				(41)
#define	MCB_AA_ADC_VOLR				(0x7F)
/*
#define	MCI_AA_DTMFB_VOLL				(42)
#define	MCI_AA_DTMFB_VOLR				(43)
*/
#define	MCI_AA_AENG6_VOLL				(44)
#define	MCB_AA_AENG6_LAT				(0x80)
#define	MCB_AA_AENG6_VOLL				(0x7F)

#define	MCI_AA_AENG6_VOLR				(45)
#define	MCB_AA_AENG6_VOLR				(0x7F)

#define	MCI_AA_DIT_ININTP				(50)
#define	MCB_AA_DIT0_ININTP				(0x20)
#define	MCB_AA_DIT1_ININTP				(0x10)
#define	MCB_AA_DIT2_ININTP				(0x08)
#define	MCI_AA_DIT_ININTP_DEF			(MCB_AA_DIT0_ININTP | MCB_AA_DIT1_ININTP | MCB_AA_DIT2_ININTP)

#define	MCI_AA_DIR_INTP				(51)
#define	MCB_AA_PDM0_INTP				(0x80)
#define	MCB_AA_DIR0_INTP				(0x20)
#define	MCB_AA_DIR1_INTP				(0x10)
#define	MCB_AA_DIR2_INTP				(0x08)
#define	MCB_AA_ADC2_INTP				(0x01)
#define	MCI_AA_DIR_INTP_DEF			(MCB_AA_PDM0_INTP | MCB_AA_DIR0_INTP | MCB_AA_DIR1_INTP | MCB_AA_DIR2_INTP)

#define	MCI_AA_ADC_INTP				(52)
#define	MCB_AA_ADC_INTP				(0x80)
#define	MCB_AA_AENG6_INTP				(0x20)
#define	MCI_AA_ADC_INTP_DEF			(MCB_AA_ADC_INTP | MCB_AA_AENG6_INTP)

#define	MCI_AA_ADC_ATTL				(54)
#define	MCB_AA_ADC_ALAT				(0x80)
#define	MCB_AA_ADC_ATTL				(0x7F)

#define	MCI_AA_ADC_ATTR				(55)
#define	MCB_AA_ADC_ATTR				(0x7F)

#define	MCI_AA_DIR0_ATTL				(56)
#define	MCB_AA_DIR0_ALAT				(0x80)
#define	MCB_AA_DIR0_ATTL				(0x7F)

#define	MCI_AA_DIR0_ATTR				(57)
#define	MCB_AA_DIR0_ATTR				(0x7F)

#define	MCI_AA_DIR1_ATTL				(58)
#define	MCB_AA_DIR1_ALAT				(0x80)
#define	MCB_AA_DIR1_ATTL				(0x7F)

#define	MCI_AA_DIR1_ATTR				(59)
#define	MCB_AA_DIR1_ATTR				(0x7F)
/*
#define	MCI_AA_ADC2_ATTL				(60)
#define	MCI_AA_ADC2_ATTR				(61)
*/
#define	MCI_AA_DIR2_ATTL				(62)
#define	MCB_AA_DIR2_ALAT				(0x80)
#define	MCB_AA_DIR2_ATTL				(0x7F)

#define	MCI_AA_DIR2_ATTR				(63)
#define	MCB_AA_DIR2_ATTR				(0x7F)

#define	MCI_AA_AINTP					(72)
#define	MCB_AA_ADC_AINTP				(0x40)
#define	MCB_AA_DIR0_AINTP				(0x20)
#define	MCB_AA_DIR1_AINTP				(0x10)
#define	MCB_AA_DIR2_AINTP				(0x04)
#define	MCI_AA_AINTP_DEF				(MCB_AA_ADC_AINTP | MCB_AA_DIR0_AINTP | MCB_AA_DIR1_AINTP | MCB_AA_DIR2_AINTP)

#define	MCI_AA_DAC_INS					(74)
#define	MCB_AA_DAC_INS					(0x80)

#define	MCI_AA_INS						(75)
#define	MCB_AA_ADC_INS					(0x40)
#define	MCB_AA_DIR0_INS				(0x20)
#define	MCB_AA_DIR1_INS				(0x10)
#define	MCB_AA_DIR2_INS				(0x04)

#define	MCI_AA_IINTP					(76)
#define	MCB_AA_DAC_IINTP				(0x80)
#define	MCB_AA_ADC_IINTP				(0x40)
#define	MCB_AA_DIR0_IINTP				(0x20)
#define	MCB_AA_DIR1_IINTP				(0x10)
#define	MCB_AA_DIR2_IINTP				(0x04)
#define	MCI_AA_IINTP_DEF				(MCB_AA_DAC_IINTP | MCB_AA_ADC_IINTP | MCB_AA_DIR0_IINTP | MCB_AA_DIR1_IINTP | MCB_AA_DIR2_IINTP)

#define	MCI_AA_ST_VOLL					(77)
#define	MCB_AA_ST_LAT					(0x80)
#define	MCB_AA_ST_VOLL					(0x7F)

#define	MCI_AA_ST_VOLR					(78)
#define	MCB_AA_ST_VOLR					(0x7F)

#define	MCI_AA_MASTER_OUTL				(79)
#define	MCB_AA_MASTER_OLAT				(0x80)
#define	MCB_AA_MASTER_OUTL				(0x7F)

#define	MCI_AA_MASTER_OUTR				(80)
#define	MCB_AA_MASTER_OUTR				(0x7F)

#define	MCI_AA_VOICE_ATTL				(83)
#define	MCB_AA_VOICE_LAT				(0x80)
#define	MCB_AA_VOICE_ATTL				(0x7F)

#define	MCI_AA_VOICE_ATTR				(84)
#define	MCB_AA_VOICE_ATTR				(0x7F)
/*
#define	MCI_AA_DTMF_ATTL				(85)
#define	MCI_AA_DTMF_ATTR				(86)
*/
#define	MCI_AA_DAC_ATTL				(89)
#define	MCB_AA_DAC_LAT					(0x80)
#define	MCB_AA_DAC_ATTL				(0x7F)

#define	MCI_AA_DAC_ATTR				(90)
#define	MCB_AA_DAC_ATTR				(0x7F)

#define	MCI_AA_DAC_INTP				(93)
#define	MCB_AA_ST_INTP					(0x80)
#define	MCB_AA_MASTER_OINTP			(0x40)
#define	MCB_AA_VOICE_INTP				(0x10)
/*#define	MCB_AA_DTMF_INTP				(0x08)*/
#define	MCB_AA_DAC_INTP				(0x02)
#define	MCI_AA_DAC_INTP_DEF			(MCB_AA_ST_INTP | MCB_AA_MASTER_OINTP | MCB_AA_VOICE_INTP | MCB_AA_DAC_INTP)

#define	MCI_AA_SOURCE					(110)
#define	MCB_AA_DAC_SOURCE_AD			(0x10)
#define	MCB_AA_DAC_SOURCE_DIR2			(0x20)
#define	MCB_AA_DAC_SOURCE_DIR0			(0x30)
#define	MCB_AA_DAC_SOURCE_DIR1			(0x40)
#define	MCB_AA_DAC_SOURCE_MIX			(0x70)
#define	MCB_AA_VOICE_SOURCE_AD			(0x01)
#define	MCB_AA_VOICE_SOURCE_DIR2		(0x02)
#define	MCB_AA_VOICE_SOURCE_DIR0		(0x03)
#define	MCB_AA_VOICE_SOURCE_DIR1		(0x04)
#define	MCB_AA_VOICE_SOURCE_MIX		(0x07)

#define	MCI_AA_SWP						(111)

#define	MCI_AA_SRC_SOURCE				(112)
#define	MCB_AA_DIT0_SOURCE_AD			(0x10)
#define	MCB_AA_DIT0_SOURCE_DIR2		(0x20)
#define	MCB_AA_DIT0_SOURCE_DIR0		(0x30)
#define	MCB_AA_DIT0_SOURCE_DIR1		(0x40)
#define	MCB_AA_DIT0_SOURCE_MIX			(0x70)
#define	MCB_AA_DIT1_SOURCE_AD			(0x01)
#define	MCB_AA_DIT1_SOURCE_DIR2		(0x02)
#define	MCB_AA_DIT1_SOURCE_DIR0		(0x03)
#define	MCB_AA_DIT1_SOURCE_DIR1		(0x04)
#define	MCB_AA_DIT1_SOURCE_MIX			(0x07)

#define	MCI_AA_SRC_SOURCE_1			(113)
#define	MCB_AA_AE_SOURCE_AD			(0x10)
#define	MCB_AA_AE_SOURCE_DIR2			(0x20)
#define	MCB_AA_AE_SOURCE_DIR0			(0x30)
#define	MCB_AA_AE_SOURCE_DIR1			(0x40)
#define	MCB_AA_AE_SOURCE_MIX			(0x70)
#define	MCB_AA_DIT2_SOURCE_AD			(0x01)
#define	MCB_AA_DIT2_SOURCE_DIR2		(0x02)
#define	MCB_AA_DIT2_SOURCE_DIR0		(0x03)
#define	MCB_AA_DIT2_SOURCE_DIR1		(0x04)
#define	MCB_AA_DIT2_SOURCE_MIX			(0x07)

#define	MCI_AA_ESRC_SOURCE				(114)

#define	MCI_AA_AENG6_SOURCE			(115)
#define	MCB_AA_AENG6_ADC0				(0x00)
#define	MCB_AA_AENG6_PDM				(0x01)

#define	MCI_AA_EFIFO_SOURCE			(116)

#define	MCI_AA_SRC_SOURCE_2			(117)

#define	MCI_AA_PEAK_METER				(121)

#define	MCI_AA_OVFL					(122)
#define	MCI_AA_OVFR					(123)

#define	MCI_AA_DIMODE0					(130)

#define	MCI_AA_DIRSRC_RATE0_MSB		(131)

#define	MCI_AA_DIRSRC_RATE0_LSB		(132)

#define	MCI_AA_DITSRC_RATE0_MSB		(133)

#define	MCI_AA_DITSRC_RATE0_LSB		(134)

#define	MCI_AA_DI_FS0					(135)

/*	DI Common Parameter	*/
#define	MCB_AA_DICOMMON_SRC_RATE_SET	(0x01)

#define	MCI_AA_DI0_SRC					(136)

#define	MCI_AA_DIX0_START				(137)
#define	MCB_AA_DITIM0_START			(0x40)
#define	MCB_AA_DIR0_SRC_START			(0x08)
#define	MCB_AA_DIR0_START				(0x04)
#define	MCB_AA_DIT0_SRC_START			(0x02)
#define	MCB_AA_DIT0_START				(0x01)

#define	MCI_AA_DIX0_FMT				(142)

#define	MCI_AA_DIR0_CH					(143)
#define	MCI_AA_DIR0_CH_DEF				(0x10)

#define	MCI_AA_DIT0_SLOT				(144)
#define	MCI_AA_DIT0_SLOT_DEF			(0x10)

#define	MCI_AA_HIZ_REDGE0				(145)

#define	MCI_AA_PCM_RX0					(146)
#define	MCB_AA_PCM_MONO_RX0			(0x80)
#define	MCI_AA_PCM_RX0_DEF				(MCB_AA_PCM_MONO_RX0)

#define	MCI_AA_PCM_SLOT_RX0			(147)

#define	MCI_AA_PCM_TX0					(148)
#define	MCB_AA_PCM_MONO_TX0			(0x80)
#define	MCI_AA_PCM_TX0_DEF				(MCB_AA_PCM_MONO_TX0)

#define	MCI_AA_PCM_SLOT_TX0			(149)
#define	MCI_AA_PCM_SLOT_TX0_DEF		(0x10)

#define	MCI_AA_DIMODE1					(150)

#define	MCI_AA_DIRSRC_RATE1_MSB		(151)
#define	MCI_AA_DIRSRC_RATE1_LSB		(152)

#define	MCI_AA_DITSRC_RATE1_MSB		(153)
#define	MCI_AA_DITSRC_RATE1_LSB		(154)

#define	MCI_AA_DI_FS1					(155)

#define	MCI_AA_DI1_SRC					(156)

#define	MCI_AA_DIX1_START				(157)
#define	MCB_AA_DITIM1_START			(0x40)
#define	MCB_AA_DIR1_SRC_START			(0x08)
#define	MCB_AA_DIR1_START				(0x04)
#define	MCB_AA_DIT1_SRC_START			(0x02)
#define	MCB_AA_DIT1_START				(0x01)

#define	MCI_AA_DIX1_FMT				(162)

#define	MCI_AA_DIR1_CH					(163)
#define	MCB_AA_DIR1_CH1				(0x10)
#define	MCI_AA_DIR1_CH_DEF				(MCB_AA_DIR1_CH1)

#define	MCI_AA_DIT1_SLOT				(164)
#define	MCB_AA_DIT1_SLOT1				(0x10)
#define	MCI_AA_DIT1_SLOT_DEF			(MCB_AA_DIT1_SLOT1)

#define	MCI_AA_HIZ_REDGE1				(165)

#define	MCI_AA_PCM_RX1					(166)
#define	MCB_AA_PCM_MONO_RX1			(0x80)
#define	MCI_AA_PCM_RX1_DEF				(MCB_AA_PCM_MONO_RX1)

#define	MCI_AA_PCM_SLOT_RX1			(167)

#define	MCI_AA_PCM_TX1					(168)
#define	MCB_AA_PCM_MONO_TX1			(0x80)
#define	MCI_AA_PCM_TX1_DEF				(MCB_AA_PCM_MONO_TX1)

#define	MCI_AA_PCM_SLOT_TX1			(169)
#define	MCI_AA_PCM_SLOT_TX1_DEF		(0x10)

#define	MCI_AA_DIMODE2					(170)

#define	MCI_AA_DIRSRC_RATE2_MSB		(171)
#define	MCI_AA_DIRSRC_RATE2_LSB		(172)

#define	MCI_AA_DITSRC_RATE2_MSB		(173)
#define	MCI_AA_DITSRC_RATE2_LSB		(174)

#define	MCI_AA_DI_FS2					(175)

#define	MCI_AA_DI2_SRC					(176)

#define	MCI_AA_DIX2_START				(177)
#define	MCB_AA_DITIM2_START			(0x40)
#define	MCB_AA_DIR2_SRC_START			(0x08)
#define	MCB_AA_DIR2_START				(0x04)
#define	MCB_AA_DIT2_SRC_START			(0x02)
#define	MCB_AA_DIT2_START				(0x01)

#define	MCI_AA_DIX2_FMT				(182)

#define	MCI_AA_DIR2_CH					(183)
#define	MCB_AA_DIR2_CH1				(0x10)
#define	MCB_AA_DIR2_CH0				(0x01)
#define	MCI_AA_DIR2_CH_DEF				(MCB_AA_DIR2_CH1)

#define	MCI_AA_DIT2_SLOT				(184)
#define	MCB_AA_DIT2_SLOT1				(0x10)
#define	MCB_AA_DIT2_SLOT0				(0x01)
#define	MCI_AA_DIT2_SLOT_DEF			(MCB_AA_DIT2_SLOT1)

#define	MCI_AA_HIZ_REDGE2				(185)

#define	MCI_AA_PCM_RX2					(186)
#define	MCB_AA_PCM_MONO_RX2			(0x80)
#define	MCI_AA_PCM_RX2_DEF				(MCB_AA_PCM_MONO_RX2)

#define	MCI_AA_PCM_SLOT_RX2			(187)

#define	MCI_AA_PCM_TX2					(188)
#define	MCB_AA_PCM_MONO_TX2			(0x80)
#define	MCI_AA_PCM_TX2_DEF				(MCB_AA_PCM_MONO_TX2)

#define	MCI_AA_PCM_SLOT_TX2			(189)
#define	MCI_AA_PCM_SLOT_TX2_DEF		(0x10)

#define	MCI_AA_CD_START				(192)

#define	MCI_AA_CDI_CH					(193)
#define	MCI_AA_CDI_CH_DEF				(0xE4)

#define	MCI_AA_CDO_SLOT				(194)
#define	MCI_AA_CDO_SLOT_DEF			(0xE4)

#define	MCI_AA_PDM_AGC					(200)
#define	MCI_AA_PDM_AGC_DEF				(0x03)

#define	MCI_AA_PDM_MUTE				(201)
#define	MCI_AA_PDM_MUTE_DEF			(0x00)
#define	MCB_AA_PDM_MUTE				(0x01)

#define	MCI_AA_PDM_START				(202)
#define	MCB_AA_PDM_MN					(0x02)
#define	MCB_AA_PDM_START				(0x01)

#define	MCI_AA_PDM_STWAIT				(205)
#define	MCI_AA_PDM_STWAIT_DEF			(0x40)

#define	MCI_AA_HP_ID					(206)

#define	MCI_AA_CHP_H					(207)
#define	MCI_AA_CHP_H_DEF				(0x3F)

#define	MCI_AA_CHP_M					(208)
#define	MCI_AA_CHP_M_DEF				(0xEA)

#define	MCI_AA_CHP_L					(209)
#define	MCI_AA_CHP_L_DEF				(0x94)

#define	MCI_AA_SINGEN0_VOL				(210)
#define	MCI_AA_SINGEN1_VOL				(211)

#define	MCI_AA_SINGEN_FREQ0_MSB		(212)
#define	MCI_AA_SINGEN_FREQ0_LSB		(213)

#define	MCI_AA_SINGEN_FREQ1_MSB		(214)
#define	MCI_AA_SINGEN_FREQ1_LSB		(215)

#define	MCI_AA_SINGEN_GATETIME			(216)

#define	MCI_AA_SINGEN_FLAG				(217)

/*	BADR(AE)	*/
#define	MCI_AA_BAND0_CEQ0				(0)
#define	MCI_AA_BAND0_CEQ0_H_DEF		(0x10)

#define	MCI_AA_BAND1_CEQ0				(15)
#define	MCI_AA_BAND1_CEQ0_H_DEF		(0x10)

#define	MCI_AA_BAND2_CEQ0				(30)
#define	MCI_AA_BAND2_CEQ0_H_DEF		(0x10)

#define	MCI_AA_BAND3H_CEQ0				(45)
#define	MCI_AA_BAND3H_CEQ0_H_DEF		(0x10)

#define	MCI_AA_BAND4H_CEQ0				(75)
#define	MCI_AA_BAND4H_CEQ0_H_DEF		(0x10)

#define	MCI_AA_BAND5_CEQ0				(105)
#define	MCI_AA_BAND5_CEQ0_H_DEF		(0x10)

#define	MCI_AA_BAND6H_CEQ0				(120)
#define	MCI_AA_BAND6H_CEQ0_H_DEF		(0x10)

#define	MCI_AA_BAND7H_CEQ0				(150)
#define	MCI_AA_BAND7H_CEQ0_H_DEF		(0x10)

#define	MCI_AA_PDM_CHP0_H				(240)
#define	MCI_AA_PDM_CHP0_H_DEF			(0x3F)
#define	MCI_AA_PDM_CHP0_M				(241)
#define	MCI_AA_PDM_CHP0_M_DEF			(0xEA)
#define	MCI_AA_PDM_CHP0_L				(242)
#define	MCI_AA_PDM_CHP0_L_DEF			(0x94)

#define	MCI_AA_PDM_CHP1_H				(243)
#define	MCI_AA_PDM_CHP1_H_DEF			(0xC0)
#define	MCI_AA_PDM_CHP1_M				(244)
#define	MCI_AA_PDM_CHP1_M_DEF			(0x15)
#define	MCI_AA_PDM_CHP1_L				(245)
#define	MCI_AA_PDM_CHP1_L_DEF			(0x6C)

#define	MCI_AA_PDM_CHP2_H				(246)
#define	MCI_AA_PDM_CHP2_H_DEF			(0x00)
#define	MCI_AA_PDM_CHP2_M				(247)
#define	MCI_AA_PDM_CHP2_M_DEF			(0x00)
#define	MCI_AA_PDM_CHP2_L				(248)
#define	MCI_AA_PDM_CHP2_L_DEF			(0x00)

#define	MCI_AA_PDM_CHP3_H				(249)
#define	MCI_AA_PDM_CHP3_H_DEF			(0x3F)
#define	MCI_AA_PDM_CHP3_M				(250)
#define	MCI_AA_PDM_CHP3_M_DEF			(0xD5)
#define	MCI_AA_PDM_CHP3_L				(251)
#define	MCI_AA_PDM_CHP3_L_DEF			(0x29)

#define	MCI_AA_PDM_CHP4_H				(252)
#define	MCI_AA_PDM_CHP4_H_DEF			(0x00)
#define	MCI_AA_PDM_CHP4_M				(253)
#define	MCI_AA_PDM_CHP4_M_DEF			(0x00)
#define	MCI_AA_PDM_CHP4_L				(254)
#define	MCI_AA_PDM_CHP4_L_DEF			(0x00)

/*	B_ADR(CDSP)	*/
#define	MCI_AA_CDSP_SAVEOFF			(0)

#define	MCI_AA_OFIFO_LVL				(1)

#define	MCI_AA_EFIFO_LVL				(2)

#define	MCI_AA_DEC_POS_24				(4)
#define	MCI_AA_DEC_POS_16				(5)
#define	MCI_AA_DEC_POS_8				(6)
#define	MCI_AA_DEC_POS_0				(7)

#define	MCI_AA_ENC_POS_24				(8)
#define	MCI_AA_ENC_POS_16				(9)
#define	MCI_AA_ENC_POS_8				(10)
#define	MCI_AA_ENC_POS_0				(11)

#define	MCI_AA_DEC_ERR					(12)
#define	MCI_AA_ENC_ERR					(13)

#define	MCI_AA_FIFO_RST				(14)

#define	MCI_AA_DEC_ENC_START			(15)

#define	MCI_AA_FIFO4CH					(16)

#define	MCI_AA_DEC_CTL15				(19)

#define	MCI_AA_DEC_GPR15				(35)

#define	MCI_AA_DEC_SFR1				(51)
#define	MCI_AA_DEC_SFR0				(52)

#define	MCI_AA_ENC_CTL15				(53)

#define	MCI_AA_ENC_GPR15				(69)

#define	MCI_AA_ENC_SFR1				(85)
#define	MCI_AA_ENC_SFR0				(86)

#define	MCI_AA_JOEMP					(92)
#define	MCB_AA_JOEMP					(0x80)
#define	MCB_AA_JOPNT					(0x40)
#define	MCB_AA_OOVF_FLG				(0x08)
#define	MCB_AA_OUDF_FLG				(0x04)
#define	MCB_AA_OEMP_FLG				(0x02)
#define	MCB_AA_OPNT_FLG				(0x01)
#define	MCI_AA_JOEMP_DEF				(MCB_AA_JOEMP | MCB_AA_OEMP_FLG)

#define	MCI_AA_JEEMP					(93)
#define	MCB_AA_JEEMP					(0x80)
#define	MCB_AA_JEPNT					(0x40)
#define	MCB_AA_EOVF_FLG				(0x08)
#define	MCB_AA_EUDF_FLG				(0x04)
#define	MCB_AA_EEMP_FLG				(0x02)
#define	MCB_AA_EPNT_FLG				(0x01)
#define	MCI_AA_JEEMP_DEF				(MCB_AA_JEEMP | MCB_AA_EEMP_FLG)

#define	MCI_AA_DEC_FLG					(96)
#define	MCI_AA_ENC_FLG					(97)

#define	MCI_AA_DEC_GPR_FLG				(98)
#define	MCI_AA_ENC_GPR_FLG				(99)

#define	MCI_AA_EOPNT					(101)

#define	MCI_AA_EDEC					(105)
#define	MCI_AA_EENC					(106)

#define	MCI_AA_EDEC_GPR				(107)
#define	MCI_AA_EENC_GPR				(108)

#define	MCI_AA_CDSP_SRST				(110)
#define	MCB_AA_CDSP_FMODE				(0x10)
#define	MCB_AA_CDSP_MSAVE				(0x08)
#define	MCB_AA_CDSP_SRST				(0x01)
#define	MCI_AA_CDSP_SRST_DEF			(MCB_AA_CDSP_SRST)

#define	MCI_AA_CDSP_SLEEP				(112)

#define	MCI_AA_CDSP_ERR				(113)

#define	MCI_AA_CDSP_MAR_MSB			(114)
#define	MCI_AA_CDSP_MAR_LSB			(115)

#define	MCI_AA_OFIFO_IRQ_PNT			(116)

#define	MCI_AA_EFIFO_IRQ_PNT			(122)

#define	MCI_AA_CDSP_FLG				(128)

#define	MCI_AA_ECDSP_ERR				(129)

/*	B_ADR(CD)	*/
#define	MCI_AA_DPADIF					(1)
#define	MCB_AA_DPADIF					(0x10)
#define	MCI_AA_DPADIF_DEF				(MCB_AA_DPADIF)

#define	MCI_AA_CD_HW_ID				(8)
#define	MCI_AA_CD_HW_ID_DEF			(0x78)

#define	MCI_AA_AD_AGC					(70)
#define	MCI_AA_AD_AGC_DEF				(0x03)

#define	MCI_AA_AD_MUTE					(71)
#define	MCI_AA_AD_MUTE_DEF				(0x00)
#define	MCB_AA_AD_MUTE					(0x01)

#define	MCI_AA_AD_START				(72)
#define	MCI_AA_AD_START_DEF			(0x00)
#define	MCB_AA_AD_START				(0x01)

#define	MCI_AA_DCCUTOFF				(77)
#define	MCI_AA_DCCUTOFF_DEF			(0x00)

#define	MCI_AA_DAC_CONFIG				(78)
#define	MCI_AA_DAC_CONFIG_DEF			(0x02)
#define	MCB_AA_NSMUTE					(0x02)
#define	MCB_AA_DACON					(0x01)

#define	MCI_AA_DCL						(79)
#define	MCI_AA_DCL_DEF					(0x00)


/*	B_ADR(ANA)	*/
#define	MCI_AA_ANA_RST					(0)
#define	MCI_AA_ANA_RST_DEF				(0x01)

#define	MCI_AA_PWM_ANALOG_0			(2)
#define	MCB_AA_PWM_VR					(0x01)
#define	MCB_AA_PWM_CP					(0x02)
#define	MCB_AA_PWM_REFA				(0x04)
#define	MCB_AA_PWM_LDOA				(0x08)
#define	MCI_AA_PWM_ANALOG_0_DEF		(MCB_AA_PWM_VR|MCB_AA_PWM_CP|MCB_AA_PWM_REFA|MCB_AA_PWM_LDOA)

#define	MCI_AA_PWM_ANALOG_1			(3)
#define	MCB_AA_PWM_SPL1				(0x01)
#define	MCB_AA_PWM_SPL2				(0x02)
#define	MCB_AA_PWM_SPR1				(0x04)
#define	MCB_AA_PWM_SPR2				(0x08)
#define	MCB_AA_PWM_HPL					(0x10)
#define	MCB_AA_PWM_HPR					(0x20)
#define	MCB_AA_PWM_ADL					(0x40)
#define	MCB_AA_PWM_ADR					(0x80)
#define	MCI_AA_PWM_ANALOG_1_DEF		(MCB_AA_PWM_SPL1|MCB_AA_PWM_SPL2|MCB_AA_PWM_SPR1|MCB_AA_PWM_SPR2|MCB_AA_PWM_HPL|MCB_AA_PWM_HPR|MCB_AA_PWM_ADL|MCB_AA_PWM_ADR)

#define	MCI_AA_PWM_ANALOG_2			(4)
#define	MCB_AA_PWM_LO1L				(0x01)
#define	MCB_AA_PWM_LO1R				(0x02)
#define	MCB_AA_PWM_LO2L				(0x04)
#define	MCB_AA_PWM_LO2R				(0x08)
#define	MCB_AA_PWM_RC1					(0x10)
#define	MCB_AA_PWM_RC2					(0x20)
#define	MCI_AA_PWM_ANALOG_2_DEF		(MCB_AA_PWM_LO1L|MCB_AA_PWM_LO1R|MCB_AA_PWM_LO2L|MCB_AA_PWM_LO2R|MCB_AA_PWM_RC1|MCB_AA_PWM_RC2)

#define	MCI_AA_PWM_ANALOG_3			(5)
#define	MCB_AA_PWM_MB1					(0x01)
#define	MCB_AA_PWM_MB2					(0x02)
#define	MCB_AA_PWM_MB3					(0x04)
#define	MCB_AA_PWM_DA					(0x08)
#define	MCI_AA_PWM_ANALOG_3_DEF		(MCB_AA_PWM_MB1|MCB_AA_PWM_MB2|MCB_AA_PWM_MB3|MCB_AA_PWM_DA)

#define	MCI_AA_PWM_ANALOG_4			(6)
#define	MCB_AA_PWM_MC1					(0x10)
#define	MCB_AA_PWM_MC2					(0x20)
#define	MCB_AA_PWM_MC3					(0x40)
#define	MCB_AA_PWM_LI					(0x80)
#define	MCI_AA_PWM_ANALOG_4_DEF		(MCB_AA_PWM_MC1|MCB_AA_PWM_MC2|MCB_AA_PWM_MC3|MCB_AA_PWM_LI)

#define	MCI_AA_BUSY1					(12)
#define	MCB_AA_RC_BUSY					(0x20)
#define	MCB_AA_HPL_BUSY				(0x10)
#define	MCB_AA_SPL_BUSY				(0x08)

#define	MCI_AA_BUSY2					(13)
#define	MCB_AA_HPR_BUSY				(0x10)
#define	MCB_AA_SPR_BUSY				(0x08)

#define	MCI_AA_AMP						(16)
#define	MCB_AA_AMPOFF_SP				(0x01)
#define	MCB_AA_AMPOFF_HP				(0x02)
#define	MCB_AA_AMPOFF_RC				(0x04)

#define	MCI_AA_DNGATRT					(20)
#define	MCI_AA_DNGATRT_DEF				(0x22)

#define	MCI_AA_DNGON					(21)
#define	MCI_AA_DNGON_DEF				(0x34)

#define	MCI_AA_DIF_LINE				(24)
#define	MCI_AA_DIF_LINE_DEF			(0x00)

#define	MCI_AA_LI1VOL_L				(25)
#define	MCI_AA_LI1VOL_L_DEF			(0x00)
#define	MCB_AA_ALAT_LI1				(0x40)
#define	MCB_AA_LI1VOL_L				(0x1F)

#define	MCI_AA_LI1VOL_R				(26)
#define	MCI_AA_LI1VOL_R_DEF			(0x00)
#define	MCB_AA_LI1VOL_R				(0x1F)

#define	MCI_AA_LI2VOL_L				(27)
#define	MCI_AA_LI2VOL_L_DEF			(0x00)
#define	MCB_AA_ALAT_LI2				(0x40)
#define	MCB_AA_LI2VOL_L				(0x1F)

#define	MCI_AA_LI2VOL_R				(28)
#define	MCI_AA_LI2VOL_R_DEF			(0x00)
#define	MCB_AA_LI2VOL_R				(0x1F)

#define	MCI_AA_MC1VOL					(29)
#define	MCI_AA_MC1VOL_DEF				(0x00)
#define	MCB_AA_MC1VOL					(0x1F)

#define	MCI_AA_MC2VOL					(30)
#define	MCI_AA_MC2VOL_DEF				(0x00)
#define	MCB_AA_MC2VOL					(0x1F)

#define	MCI_AA_MC3VOL					(31)
#define	MCI_AA_MC3VOL_DEF				(0x00)
#define	MCB_AA_MC3VOL					(0x1F)

#define	MCI_AA_ADVOL_L					(32)
#define	MCI_AA_ADVOL_L_DEF				(0x00)
#define	MCB_AA_ALAT_AD					(0x40)
#define	MCB_AA_ADVOL_L					(0x1F)

#define	MCI_AA_ADVOL_R					(33)
#define	MCB_AA_ADVOL_R					(0x1F)

#define	MCI_AA_HPVOL_L					(35)
#define	MCB_AA_ALAT_HP					(0x40)
#define	MCB_AA_SVOL_HP					(0x20)
#define	MCB_AA_HPVOL_L					(0x1F)
#define	MCI_AA_HPVOL_L_DEF				(MCB_AA_SVOL_HP)

#define	MCI_AA_HPVOL_R					(36)
#define	MCI_AA_HPVOL_R_DEF				(0x00)
#define	MCB_AA_HPVOL_R					(0x1F)

#define	MCI_AA_SPVOL_L					(37)
#define	MCB_AA_ALAT_SP					(0x40)
#define	MCB_AA_SVOL_SP					(0x20)
#define	MCB_AA_SPVOL_L					(0x1F)
#define	MCI_AA_SPVOL_L_DEF				(MCB_AA_SVOL_SP)

#define	MCI_AA_SPVOL_R					(38)
#define	MCI_AA_SPVOL_R_DEF				(0x00)
#define	MCB_AA_SPVOL_R					(0x1F)

#define	MCI_AA_RCVOL					(39)
#define	MCB_AA_SVOL_RC					(0x20)
#define	MCB_AA_RCVOL					(0x1F)
#define	MCI_AA_RCVOL_DEF				(MCB_AA_SVOL_RC)

#define	MCI_AA_LO1VOL_L				(40)
#define	MCI_AA_LO1VOL_L_DEF			(0x20)
#define	MCB_AA_ALAT_LO1				(0x40)
#define	MCB_AA_LO1VOL_L				(0x1F)

#define	MCI_AA_LO1VOL_R				(41)
#define	MCI_AA_LO1VOL_R_DEF			(0x00)
#define	MCB_AA_LO1VOL_R				(0x1F)

#define	MCI_AA_LO2VOL_L				(42)
#define	MCI_AA_LO2VOL_L_DEF			(0x20)
#define	MCB_AA_ALAT_LO2				(0x40)
#define	MCB_AA_LO2VOL_L				(0x1F)

#define	MCI_AA_LO2VOL_R				(43)
#define	MCI_AA_LO2VOL_R_DEF			(0x00)
#define	MCB_AA_LO2VOL_R				(0x1F)

#define	MCI_AA_SP_MODE					(44)
#define	MCB_AA_SPR_HIZ					(0x20)
#define	MCB_AA_SPL_HIZ					(0x10)
#define	MCB_AA_SPMN					(0x02)
#define	MCB_AA_SP_SWAP					(0x01)

#define	MCI_AA_MC_GAIN					(45)
#define	MCI_AA_MC_GAIN_DEF				(0x00)
#define	MCB_AA_MC2SNG					(0x40)
#define	MCB_AA_MC2GAIN					(0x30)
#define	MCB_AA_MC1SNG					(0x04)
#define	MCB_AA_MC1GAIN					(0x03)

#define	MCI_AA_MC3_GAIN				(46)
#define	MCI_AA_MC3_GAIN_DEF			(0x00)
#define	MCB_AA_MC3SNG					(0x04)
#define	MCB_AA_MC3GAIN					(0x03)

#define	MCI_AA_RDY_FLAG				(47)
#define	MCB_AA_LDO_RDY					(0x80)
#define	MCB_AA_VREF_RDY				(0x40)
#define	MCB_AA_SPRDY_R					(0x20)
#define	MCB_AA_SPRDY_L					(0x10)
#define	MCB_AA_HPRDY_R					(0x08)
#define	MCB_AA_HPRDY_L					(0x04)
#define	MCB_AA_CPPDRDY					(0x02)

/*	analog mixer common */
#define	MCB_AA_LI1MIX					(0x01)
#define	MCB_AA_M1MIX					(0x08)
#define	MCB_AA_M2MIX					(0x10)
#define	MCB_AA_M3MIX					(0x20)
#define	MCB_AA_DAMIX					(0x40)
#define	MCB_AA_DARMIX					(0x40)
#define	MCB_AA_DALMIX					(0x80)

#define	MCB_AA_MONO_DA					(0x40)
#define	MCB_AA_MONO_LI1				(0x01)

#define	MCI_AA_ADL_MIX					(50)
#define	MCI_AA_ADL_MONO				(51)
#define	MCI_AA_ADR_MIX					(52)
#define	MCI_AA_ADR_MONO				(53)

#define	MCI_AA_LO1L_MIX				(55)
#define	MCI_AA_LO1L_MONO				(56)
#define	MCI_AA_LO1R_MIX				(57)

#define	MCI_AA_LO2L_MIX				(58)
#define	MCI_AA_LO2L_MONO				(59)
#define	MCI_AA_LO2R_MIX				(60)

#define	MCI_AA_HPL_MIX					(61)
#define	MCI_AA_HPL_MONO				(62)
#define	MCI_AA_HPR_MIX					(63)

#define	MCI_AA_SPL_MIX					(64)
#define	MCI_AA_SPL_MONO				(65)
#define	MCI_AA_SPR_MIX					(66)
#define	MCI_AA_SPR_MONO				(67)

#define	MCI_AA_RC_MIX					(69)

#define	MCI_AA_HP_GAIN					(77)

#define	MCI_AA_LEV						(79)
#define	MCB_AA_AVDDLEV					(0x07)
#define	MCI_AA_LEV_DEF					(0x24)

#define	MCI_AA_AP_A1					(123)
#define	MCB_AA_AP_CP_A					(0x10)
#define	MCB_AA_AP_HPL_A				(0x02)
#define	MCB_AA_AP_HPR_A				(0x01)

#define	MCI_AA_AP_A2					(124)
#define	MCB_AA_AP_RC1_A				(0x20)
#define	MCB_AA_AP_RC2_A				(0x10)
#define	MCB_AA_AP_SPL1_A				(0x08)
#define	MCB_AA_AP_SPR1_A				(0x04)
#define	MCB_AA_AP_SPL2_A				(0x02)
#define	MCB_AA_AP_SPR2_A				(0x01)

/************************************/

typedef enum
{
	eMCDRV_DEV_ID_1N2	= 0,
	eMCDRV_DEV_ID_1N2B,
	eMCDRV_DEV_ID_2N,
	eMCDRV_DEV_ID_3N
} MCDRV_DEV_ID;

typedef enum
{
	eMCDRV_FUNC_LI2	= 0,
	eMCDRV_FUNC_DIVR1,
	eMCDRV_FUNC_DIVF1,
	eMCDRV_FUNC_RANGE,
	eMCDRV_FUNC_BYPASS,
	eMCDRV_FUNC_ADC1,
	eMCDRV_FUNC_PAD2,
	eMCDRV_FUNC_DBEX,
	eMCDRV_FUNC_GPMODE,
	eMCDRV_FUNC_DTMF,
	eMCDRV_FUNC_IRQ,
	eMCDRV_FUNC_HWADJ
} MCDRV_FUNC_KIND;

typedef enum
{
	eMCDRV_SLAVE_ADDR_DIG	= 0,
	eMCDRV_SLAVE_ADDR_ANA
} MCDRV_SLAVE_ADDR_KIND;

static	MCDRV_DEV_ID	geDevID	= eMCDRV_DEV_ID_1N2;

static	UINT8	gabValid[][4]	=
{
/*	MC-1N2	MC-1N2B	MC-2N	MC-3N	*/
	{0,		0,		0,		1},		/*	DI2		*/
	{0,		0,		0,		1},		/*	DIVR1	*/
	{0,		0,		0,		1},		/*	DIVF1	*/
	{0,		0,		0,		1},		/*	RANGE	*/
	{0,		0,		0,		1},		/*	BYPASS	*/
	{0,		0,		0,		1},		/*	ADC1	*/
	{0,		0,		0,		1},		/*	PAD2	*/
	{0,		0,		1,		1},		/*	DBEX	*/
	{0,		0,		0,		1},		/*	GPMODE	*/
	{0,		0,		0,		1},		/*	DTMF	*/
	{0,		0,		0,		1},		/*	IRQ		*/
	{0,		0,		0,		0},		/*	HWADJ	*/
};

static UINT8	gabSlaveAddr[3][2]	=
{
/*	Digital Analog	*/
	{0x3A,	0x3A},	/*	MC1N2	*/
	{0x00,	0x00},	/*	MC2N	*/
	{0x00,	0x00}	/*	MC3N	*/
};

/****************************************************************************
 *	McDevProf_SetDevId
 *
 *	Description:
 *			Set device ID.
 *	Arguments:
 *			eDevId	device ID
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	McDevProf_SetDevId(MCDRV_DEV_ID eDevId)
{
	geDevID	= eDevId;
}

/****************************************************************************
 *	McDevProf_IsValid
 *
 *	Description:
 *			Validity function.
 *	Arguments:
 *			function kind
 *	Return:
 *			0:Invalid/1:Valid
 *
 ****************************************************************************/
static UINT8	McDevProf_IsValid
(
	MCDRV_FUNC_KIND eFuncKind
)
{
	return gabValid[eFuncKind][geDevID];
}

/****************************************************************************
 *	McDevProf_GetSlaveAddr
 *
 *	Description:
 *			get slave address.
 *	Arguments:
 *			eSlaveAddrKind	slave address kind
 *	Return:
 *			slave address
 *
 ****************************************************************************/
static UINT8	McDevProf_GetSlaveAddr
(
	MCDRV_SLAVE_ADDR_KIND eSlaveAddrKind
)
{
	return gabSlaveAddr[geDevID][eSlaveAddrKind];
}

/**************************************/

/*	UpdateReg parameter	*/
typedef enum
{
	eMCDRV_UPDATE_NORMAL_AA,
	eMCDRV_UPDATE_FORCE_AA,
	eMCDRV_UPDATE_DUMMY_AA
} MCDRV_UPDATE_MODE_AA;

/*	ePacketBufAlloc setting	*/
typedef enum
{
	eMCDRV_PACKETBUF_FREE_AA,
	eMCDRV_PACKETBUF_ALLOCATED_AA
} MCDRV_PACKETBUF_ALLOC_AA;

/* packet */
typedef struct
{
	UINT32	dDesc;
	UINT8	bData;
} MCDRV_PACKET_AA;

#define	MCDRV_MAX_PACKETS_AA					(256UL)

/*	packet dDesc	*/
/*	packet type	*/
#define	MCDRV_PACKET_TYPE_WRITE_AA				(0x10000000UL)
#define	MCDRV_PACKET_TYPE_FORCE_WRITE_AA		(0x20000000UL)
#define	MCDRV_PACKET_TYPE_TIMWAIT_AA			(0x30000000UL)
#define	MCDRV_PACKET_TYPE_EVTWAIT_AA			(0x40000000UL)
#define	MCDRV_PACKET_TYPE_TERMINATE_AA			(0xF0000000UL)

#define	MCDRV_PACKET_TYPE_MASK_AA				(0xF0000000UL)

/*	reg type	*/
#define	MCDRV_PACKET_REGTYPE_A_AA				(0x00000000UL)
#define	MCDRV_PACKET_REGTYPE_B_BASE_AA			(0x00001000UL)
#define	MCDRV_PACKET_REGTYPE_B_MIXER_AA		(0x00002000UL)
#define	MCDRV_PACKET_REGTYPE_B_AE_AA			(0x00003000UL)
#define	MCDRV_PACKET_REGTYPE_B_CDSP_AA			(0x00004000UL)
#define	MCDRV_PACKET_REGTYPE_B_CODEC_AA		(0x00005000UL)
#define	MCDRV_PACKET_REGTYPE_B_ANA_AA			(0x00006000UL)

#define	MCDRV_PACKET_REGTYPE_MASK_AA			(0x0000F000UL)
#define	MCDRV_PACKET_ADR_MASK_AA				(0x00000FFFUL)

/*	event	*/
#define	MCDRV_EVT_INSFLG_AA					(0x00010000UL)
#define	MCDRV_EVT_ALLMUTE_AA					(0x00020000UL)
#define	MCDRV_EVT_DACMUTE_AA					(0x00030000UL)
#define	MCDRV_EVT_DITMUTE_AA					(0x00040000UL)
#define	MCDRV_EVT_SVOL_DONE_AA					(0x00050000UL)
#define	MCDRV_EVT_APM_DONE_AA					(0x00060000UL)
#define	MCDRV_EVT_ANA_RDY_AA					(0x00070000UL)
#define	MCDRV_EVT_SYSEQ_FLAG_RESET_AA			(0x00080000UL)
#define	MCDRV_EVT_CLKBUSY_RESET_AA				(0x00090000UL)
#define	MCDRV_EVT_CLKSRC_SET_AA				(0x000A0000UL)
#define	MCDRV_EVT_CLKSRC_RESET_AA				(0x000B0000UL)

#define	MCDRV_PACKET_EVT_MASK_AA				(0x0FFF0000UL)
#define	MCDRV_PACKET_EVTPRM_MASK_AA			(0x0000FFFFUL)

/*	timer	*/
#define	MCDRV_PACKET_TIME_MASK_AA				(0x0FFFFFFFUL)

static SINT32	McDevIf_AllocPacketBuf_AA	(void);
static void		McDevIf_ReleasePacketBuf_AA	(void);
static void		McDevIf_ClearPacket_AA		(void);
static void		McDevIf_AddPacket_AA		(UINT32 dDesc, UINT8 bData);
static void		McDevIf_AddPacketRepeat_AA	(UINT32 dDesc, const UINT8* pbData, UINT16 wDataCount);
static SINT32	McDevIf_ExecutePacket_AA	(void);

static MCDRV_PACKET_AA*	gpsPacket	= NULL;


static MCDRV_PACKET_AA*	McResCtrl_AllocPacketBuf_AA			(void);
static void				McResCtrl_ReleasePacketBuf_AA		(void);
static void				McResCtrl_InitRegUpdate_AA			(void);
static void				McResCtrl_AddRegUpdate_AA			(UINT16 wRegType, UINT16 wAddress, UINT8 bData, MCDRV_UPDATE_MODE_AA eUpdateMode);
static void				McResCtrl_ExecuteRegUpdate_AA		(void);
static SINT32			McResCtrl_WaitEvent_AA				(UINT32 dEvent, UINT32 dParam);


/****************************************************************************
 *	McDevIf_AllocPacketBuf_AA
 *
 *	機能:
 *			レジスタ設定パケット用バッファの確保.
 *	引数:
 *			なし
 *	戻り値:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR
 *
 ****************************************************************************/
SINT32	McDevIf_AllocPacketBuf_AA
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

	gpsPacket	= McResCtrl_AllocPacketBuf_AA();
	if(gpsPacket == NULL)
	{
		sdRet	= MCDRV_ERROR;
	}
	else
	{
		McDevIf_ClearPacket_AA();
	}

	return sdRet;
}

/****************************************************************************
 *	McDevIf_ReleasePacketBuf_AA
 *
 *	機能:
 *			レジスタ設定パケット用バッファの開放.
 *	引数:
 *			なし
 *	戻り値:
 *			なし
 *
 ****************************************************************************/
void	McDevIf_ReleasePacketBuf_AA
(
	void
)
{
	McResCtrl_ReleasePacketBuf_AA();
}

/****************************************************************************
 *	McDevIf_ClearPacket_AA
 *
 *	機能:
 *			パケットのクリア.
 *	引数:
 *			なし
 *	戻り値:
 *			なし
 *
 ****************************************************************************/
void	McDevIf_ClearPacket_AA
(
	void
)
{
	if(gpsPacket == NULL)
	{
		return;
	}

	gpsPacket[0].dDesc = MCDRV_PACKET_TYPE_TERMINATE_AA;
}

/****************************************************************************
 *	McDevIf_AddPacket_AA
 *
 *	機能:
 *			パケット追加
 *	引数:
 *			dDesc			パケット情報
 *			bData			パケットデータ
 *	戻り値:
 *			なし
 *
 ****************************************************************************/
void	McDevIf_AddPacket_AA
(
	UINT32			dDesc,
	UINT8			bData
)
{
	UINT32	i;

	if(gpsPacket == NULL)
	{
	}
	else
	{
		for(i = 0; i < MCDRV_MAX_PACKETS_AA; i++)
		{
			if(gpsPacket[i].dDesc == MCDRV_PACKET_TYPE_TERMINATE_AA)
			{
				break;
			}
		}
		if(i >= MCDRV_MAX_PACKETS_AA)
		{
			McDevIf_ExecutePacket_AA();
			i	= 0;
		}

		gpsPacket[i].dDesc		= dDesc;
		gpsPacket[i].bData		= bData;
		gpsPacket[i+1UL].dDesc	= MCDRV_PACKET_TYPE_TERMINATE_AA;
	}
}

/****************************************************************************
 *	McDevIf_AddPacketRepeat_AA
 *
 *	機能:
 *			同一レジスタに繰り返しデータセットするパケットを追加
 *	引数:
 *			dDesc			パケット情報
 *			pbData			パケットデータバッファのポインタ
 *			wDataCount		パケットデータ数
 *	戻り値:
 *			なし
 *
 ****************************************************************************/
void	McDevIf_AddPacketRepeat_AA
(
	UINT32			dDesc,
	const UINT8*	pbData,
	UINT16			wDataCount
)
{
	UINT16	wCount;

	for(wCount = 0; wCount < wDataCount; wCount++)
	{
		McDevIf_AddPacket_AA(dDesc, pbData[wCount]);
	}
}

/****************************************************************************
 *	McDevIf_ExecutePacket_AA
 *
 *	機能:
 *			レジスタ設定シーケンスの実行.
 *	引数:
 *			なし
 *	戻り値:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
SINT32	McDevIf_ExecutePacket_AA
(
	void
)
{
	SINT32	sdRet;
	SINT16	swPacketIndex;
	UINT32	dPacketType;
	UINT32	dParam1;
	UINT32	dParam2;
	UINT16	wAddress;
	UINT16	wRegType;
	MCDRV_UPDATE_MODE_AA	eUpdateMode;

	if(gpsPacket == NULL)
	{
		sdRet	= MCDRV_ERROR_RESOURCEOVER;
	}
	else
	{
		sdRet	= MCDRV_SUCCESS;

		McResCtrl_InitRegUpdate_AA();
		swPacketIndex = 0;
		while ((MCDRV_PACKET_TYPE_TERMINATE_AA != (gpsPacket[swPacketIndex].dDesc & MCDRV_PACKET_TYPE_MASK_AA)) && (sdRet == MCDRV_SUCCESS))
		{
			dPacketType = gpsPacket[swPacketIndex].dDesc & MCDRV_PACKET_TYPE_MASK_AA;
			switch (dPacketType)
			{
			case MCDRV_PACKET_TYPE_WRITE_AA:
			case MCDRV_PACKET_TYPE_FORCE_WRITE_AA:
				wRegType = (UINT16)(gpsPacket[swPacketIndex].dDesc & MCDRV_PACKET_REGTYPE_MASK_AA);
				wAddress = (UINT16)(gpsPacket[swPacketIndex].dDesc & MCDRV_PACKET_ADR_MASK_AA);
				if (MCDRV_PACKET_TYPE_WRITE_AA == dPacketType)
				{
					eUpdateMode = eMCDRV_UPDATE_NORMAL_AA;
				}
				else if (MCDRV_PACKET_TYPE_FORCE_WRITE_AA == dPacketType)
				{
					eUpdateMode = eMCDRV_UPDATE_FORCE_AA;
				}
				else
				{
					eUpdateMode = eMCDRV_UPDATE_DUMMY_AA;
				}
				McResCtrl_AddRegUpdate_AA(wRegType, wAddress, gpsPacket[swPacketIndex].bData, eUpdateMode);
				break;

			case MCDRV_PACKET_TYPE_TIMWAIT_AA:
				McResCtrl_ExecuteRegUpdate_AA();
				McResCtrl_InitRegUpdate_AA();
				dParam1 = gpsPacket[swPacketIndex].dDesc & MCDRV_PACKET_TIME_MASK_AA;
				McSrv_Sleep(dParam1);
				break;

			case MCDRV_PACKET_TYPE_EVTWAIT_AA:
				McResCtrl_ExecuteRegUpdate_AA();
				McResCtrl_InitRegUpdate_AA();
				dParam1 = gpsPacket[swPacketIndex].dDesc & MCDRV_PACKET_EVT_MASK_AA;
				dParam2 = gpsPacket[swPacketIndex].dDesc & MCDRV_PACKET_EVTPRM_MASK_AA;
				sdRet = McResCtrl_WaitEvent_AA(dParam1, dParam2);
				break;

			default:
				sdRet	= MCDRV_ERROR;
				break;
			}

			swPacketIndex++;
		}
		if(sdRet == MCDRV_SUCCESS)
		{
			McResCtrl_ExecuteRegUpdate_AA();
		}
		McDevIf_ClearPacket_AA();
	}

	return sdRet;
}

/*****************************************/

/*	power setting	*/
#define	MCDRV_POWINFO_DIGITAL_DP0_AA		((UINT32)0x0001)
#define	MCDRV_POWINFO_DIGITAL_DP1_AA		((UINT32)0x0002)
#define	MCDRV_POWINFO_DIGITAL_DP2_AA		((UINT32)0x0004)
#define	MCDRV_POWINFO_DIGITAL_DPB_AA		((UINT32)0x0008)
#define	MCDRV_POWINFO_DIGITAL_DPDI0_AA		((UINT32)0x0010)
#define	MCDRV_POWINFO_DIGITAL_DPDI1_AA		((UINT32)0x0020)
#define	MCDRV_POWINFO_DIGITAL_DPDI2_AA		((UINT32)0x0040)
#define	MCDRV_POWINFO_DIGITAL_DPPDM_AA		((UINT32)0x0080)
#define	MCDRV_POWINFO_DIGITAL_DPBDSP_AA		((UINT32)0x0100)
#define	MCDRV_POWINFO_DIGITAL_DPADIF_AA		((UINT32)0x0200)
#define	MCDRV_POWINFO_DIGITAL_PLLRST0_AA	((UINT32)0x0400)
typedef struct
{
	UINT32	dDigital;
	UINT8	abAnalog[5];
} MCDRV_POWER_INFO_AA;


#define	MCDRV_BURST_WRITE_ENABLE	(0x01)

/*	eState setting	*/
typedef enum
{
	eMCDRV_STATE_NOTINIT_AA,
	eMCDRV_STATE_READY_AA
} MCDRV_STATE_AA;

/*	volume setting	*/
#define	MCDRV_LOGICAL_VOL_MUTE		(-24576)	/*	-96dB	*/
#define	MCDRV_LOGICAL_MICGAIN_DEF	(3840)		/*	15dB	*/
#define	MCDRV_LOGICAL_HPGAIN_DEF	(0)			/*	0dB		*/

#define	MCDRV_VOLUPDATE_ALL_AA			(0xFFFFFFFFUL)
#define	MCDRV_VOLUPDATE_ANAOUT_ALL_AA	(0x00000001UL)

#define	MCDRV_REG_MUTE	(0x00)

/*	DAC source setting	*/
typedef enum
{
	eMCDRV_DAC_MASTER_AA	= 0,
	eMCDRV_DAC_VOICE_AA
} MCDRV_DAC_CH_AA;

/*	DIO port setting	*/
typedef enum
{
	eMCDRV_DIO_0_AA	= 0,
	eMCDRV_DIO_1_AA,
	eMCDRV_DIO_2_AA
} MCDRV_DIO_PORT_NO_AA;

/*	Path source setting	*/
typedef enum
{
	eMCDRV_SRC_NONE_AA			= (0),
	eMCDRV_SRC_MIC1_AA			= (1<<0),
	eMCDRV_SRC_MIC2_AA			= (1<<1),
	eMCDRV_SRC_MIC3_AA			= (1<<2),
	eMCDRV_SRC_LINE1_L_AA		= (1<<3),
	eMCDRV_SRC_LINE1_R_AA		= (1<<4),
	eMCDRV_SRC_LINE1_M_AA		= (1<<5),
	eMCDRV_SRC_LINE2_L_AA		= (1<<6),
	eMCDRV_SRC_LINE2_R_AA		= (1<<7),
	eMCDRV_SRC_LINE2_M_AA		= (1<<8),
	eMCDRV_SRC_DIR0_AA			= (1<<9),
	eMCDRV_SRC_DIR1_AA			= (1<<10),
	eMCDRV_SRC_DIR2_AA			= (1<<11),
	eMCDRV_SRC_DTMF_AA			= (1<<12),
	eMCDRV_SRC_PDM_AA			= (1<<13),
	eMCDRV_SRC_ADC0_AA			= (1<<14),
	eMCDRV_SRC_ADC1_AA			= (1<<15),
	eMCDRV_SRC_DAC_L_AA			= (1<<16),
	eMCDRV_SRC_DAC_R_AA			= (1<<17),
	eMCDRV_SRC_DAC_M_AA			= (1<<18),
	eMCDRV_SRC_AE_AA			= (1<<19),
	eMCDRV_SRC_CDSP_AA			= (1<<20),
	eMCDRV_SRC_MIX_AA			= (1<<21),
	eMCDRV_SRC_DIR2_DIRECT_AA	= (1<<22),
	eMCDRV_SRC_CDSP_DIRECT_AA	= (1<<23)
} MCDRV_SRC_TYPE_AA;

/*	Path destination setting	*/
typedef enum
{
	eMCDRV_DST_CH0_AA	= 0,
	eMCDRV_DST_CH1_AA
} MCDRV_DST_CH;
typedef enum
{
	eMCDRV_DST_HP_AA	= 0,
	eMCDRV_DST_SP_AA,
	eMCDRV_DST_RCV_AA,
	eMCDRV_DST_LOUT1_AA,
	eMCDRV_DST_LOUT2_AA,
	eMCDRV_DST_PEAK_AA,
	eMCDRV_DST_DIT0_AA,
	eMCDRV_DST_DIT1_AA,
	eMCDRV_DST_DIT2_AA,
	eMCDRV_DST_DAC_AA,
	eMCDRV_DST_AE_AA,
	eMCDRV_DST_CDSP_AA,
	eMCDRV_DST_ADC0_AA,
	eMCDRV_DST_ADC1_AA,
	eMCDRV_DST_MIX_AA,
	eMCDRV_DST_BIAS_AA
} MCDRV_DST_TYPE_AA;

/*	Register accsess availability	*/
typedef enum
{
	eMCDRV_ACCESS_DENY_AA	= 0,
	eMCDRV_READ_ONLY_AA	= 0x01,
	eMCDRV_WRITE_ONLY_AA	= 0x02,
	eMCDRV_READ_WRITE_AA	= eMCDRV_READ_ONLY_AA | eMCDRV_WRITE_ONLY_AA
} MCDRV_REG_ACCSESS_AA;


/* power management sequence mode */
typedef enum
{
	eMCDRV_APM_ON_AA,
	eMCDRV_APM_OFF_AA
} MCDRV_PMODE_AA;

#define	MCDRV_A_REG_NUM_AA			(64)
#define	MCDRV_B_BASE_REG_NUM_AA		(32)
#define	MCDRV_B_MIXER_REG_NUM_AA	(218)
#define	MCDRV_B_AE_REG_NUM_AA		(255)
#define	MCDRV_B_CDSP_REG_NUM_AA		(130)
#define	MCDRV_B_CODEC_REG_NUM_AA	(128)
#define	MCDRV_B_ANA_REG_NUM_AA		(128)

/* control packet for serial host interface */
#define	MCDRV_MAX_CTRL_DATA_NUM	(1024)
typedef struct
{
	UINT8	abData[MCDRV_MAX_CTRL_DATA_NUM];
	UINT16	wDataNum;
} MCDRV_SERIAL_CTRL_PACKET_AA;

/*	HWADJ setting	*/
typedef enum _MCDRV_HWADJ
{
	eMCDRV_HWADJ_NOCHANGE	= 0,
	eMCDRV_HWADJ_THRU,
	eMCDRV_HWADJ_REC8,
	eMCDRV_HWADJ_REC44,
	eMCDRV_HWADJ_REC48,
	eMCDRV_HWADJ_PLAY8,
	eMCDRV_HWADJ_PLAY44,
	eMCDRV_HWADJ_PLAY48
} MCDRV_HWADJ;

/*	global information	*/
typedef struct
{
	UINT8						bHwId;
	MCDRV_PACKETBUF_ALLOC_AA		ePacketBufAlloc;
	UINT8						abRegValA[MCDRV_A_REG_NUM_AA];
	UINT8						abRegValB_BASE[MCDRV_B_BASE_REG_NUM_AA];
	UINT8						abRegValB_MIXER[MCDRV_B_MIXER_REG_NUM_AA];
	UINT8						abRegValB_AE[MCDRV_B_AE_REG_NUM_AA];
	UINT8						abRegValB_CDSP[MCDRV_B_CDSP_REG_NUM_AA];
	UINT8						abRegValB_CODEC[MCDRV_B_CODEC_REG_NUM_AA];
	UINT8						abRegValB_ANA[MCDRV_B_ANA_REG_NUM_AA];

	MCDRV_INIT_INFO				sInitInfo;
	MCDRV_PATH_INFO				sPathInfo;
	MCDRV_PATH_INFO				sPathInfoVirtual;
	MCDRV_VOL_INFO				sVolInfo;
	MCDRV_DIO_INFO				sDioInfo;
	MCDRV_DAC_INFO				sDacInfo;
	MCDRV_ADC_INFO				sAdcInfo;
	MCDRV_SP_INFO				sSpInfo;
	MCDRV_DNG_INFO				sDngInfo;
	MCDRV_AE_INFO				sAeInfo;
	MCDRV_PDM_INFO				sPdmInfo;
	MCDRV_GP_MODE				sGpMode;
	UINT8						abGpMask[GPIO_PAD_NUM];

	MCDRV_SERIAL_CTRL_PACKET_AA	sCtrlPacket;
	UINT16						wCurSlaveAddress;
	UINT16						wCurRegType;
	UINT16						wCurRegAddress;
	UINT16						wDataContinueCount;
	UINT16						wPrevAddressIndex;

	MCDRV_PMODE_AA					eAPMode;

	MCDRV_HWADJ					eHwAdj;
} MCDRV_GLOBAL_INFO_AA;

static SINT32			McResCtrl_SetHwId_AA				(UINT8 bHwId);
static void				McResCtrl_Init_AA					(const MCDRV_INIT_INFO* psInitInfo);
static void				McResCtrl_UpdateState_AA			(MCDRV_STATE_AA eState);
static MCDRV_STATE_AA	McResCtrl_GetState_AA				(void);
static UINT8			McResCtrl_GetRegVal_AA				(UINT16 wRegType, UINT16 wRegAddr);
static void				McResCtrl_SetRegVal_AA				(UINT16 wRegType, UINT16 wRegAddr, UINT8 bRegVal);

static void				McResCtrl_GetInitInfo_AA			(MCDRV_INIT_INFO* psInitInfo);
static void				McResCtrl_SetClockInfo_AA			(const MCDRV_CLOCK_INFO* psClockInfo);
static void				McResCtrl_SetPathInfo_AA			(const MCDRV_PATH_INFO* psPathInfo);
static void				McResCtrl_GetPathInfo_AA			(MCDRV_PATH_INFO* psPathInfo);
static void				McResCtrl_GetPathInfoVirtual_AA		(MCDRV_PATH_INFO* psPathInfo);
static void				McResCtrl_SetDioInfo_AA				(const MCDRV_DIO_INFO* psDioInfo, UINT32 dUpdateInfo);
static void				McResCtrl_GetDioInfo_AA				(MCDRV_DIO_INFO* psDioInfo);
static void				McResCtrl_SetVolInfo_AA				(const MCDRV_VOL_INFO* psVolInfo);
static void				McResCtrl_GetVolInfo_AA				(MCDRV_VOL_INFO* psVolInfo);
static void				McResCtrl_SetDacInfo_AA				(const MCDRV_DAC_INFO* psDacInfo, UINT32 dUpdateInfo);
static void				McResCtrl_GetDacInfo_AA				(MCDRV_DAC_INFO* psDacInfo);
static void				McResCtrl_SetAdcInfo_AA				(const MCDRV_ADC_INFO* psAdcInfo, UINT32 dUpdateInfo);
static void				McResCtrl_GetAdcInfo_AA				(MCDRV_ADC_INFO* psAdcInfo);
static void				McResCtrl_SetSpInfo_AA				(const MCDRV_SP_INFO* psSpInfo);
static void				McResCtrl_GetSpInfo_AA				(MCDRV_SP_INFO* psSpInfo);
static void				McResCtrl_SetDngInfo_AA				(const MCDRV_DNG_INFO* psDngInfo, UINT32 dUpdateInfo);
static void				McResCtrl_GetDngInfo_AA				(MCDRV_DNG_INFO* psDngInfo);
static void				McResCtrl_SetAeInfo_AA				(const MCDRV_AE_INFO* psAeInfo, UINT32 dUpdateInfo);
static void				McResCtrl_GetAeInfo_AA				(MCDRV_AE_INFO* psAeInfo);
static void				McResCtrl_SetPdmInfo_AA				(const MCDRV_PDM_INFO* psPdmInfo, UINT32 dUpdateInfo);
static void				McResCtrl_GetPdmInfo_AA				(MCDRV_PDM_INFO* psPdmInfo);
static void				McResCtrl_SetGPMode_AA				(const MCDRV_GP_MODE* psGpMode);
static void				McResCtrl_GetGPMode_AA				(MCDRV_GP_MODE* psGpMode);
static void				McResCtrl_SetGPMask_AA				(UINT8 bMask, UINT32 dPadNo);
static void				McResCtrl_GetGPMask_AA				(UINT8* pabMask);

static void				McResCtrl_GetVolReg_AA				(MCDRV_VOL_INFO* psVolInfo);
static void				McResCtrl_GetPowerInfo_AA			(MCDRV_POWER_INFO_AA* psPowerInfo);
static void				McResCtrl_GetPowerInfoRegAccess_AA	(const MCDRV_REG_INFO* psRegInfo, MCDRV_POWER_INFO_AA* psPowerInfo);
static void				McResCtrl_GetCurPowerInfo_AA		(MCDRV_POWER_INFO_AA* psPowerInfo);

static MCDRV_SRC_TYPE_AA	McResCtrl_GetDACSource_AA		(MCDRV_DAC_CH_AA eCh);
static MCDRV_SRC_TYPE_AA	McResCtrl_GetDITSource_AA		(MCDRV_DIO_PORT_NO_AA ePort);
static MCDRV_SRC_TYPE_AA	McResCtrl_GetAESource_AA		(void);
static UINT8				McResCtrl_IsSrcUsed_AA			(MCDRV_SRC_TYPE_AA ePathSrc);
static UINT8				McResCtrl_IsDstUsed_AA			(MCDRV_DST_TYPE_AA eType, MCDRV_DST_CH eCh);
static MCDRV_REG_ACCSESS_AA	McResCtrl_GetRegAccess_AA		(const MCDRV_REG_INFO* psRegInfo);

static MCDRV_PMODE_AA		McResCtrl_GetAPMode_AA			(void);



static MCDRV_HWADJ		McResCtrl_ConfigHwAdj_AA			(void);

/*************************************/

/* HW_ID */
#define	MCDRV_HWID_YMU821_AA	(0x78)

/* wait time */
#define	MCDRV_INTERVAL_MUTE_AA	(1000)
#define	MCDRV_TIME_OUT_MUTE_AA	(1000)


static MCDRV_STATE_AA	geState	= eMCDRV_STATE_NOTINIT_AA;

static MCDRV_GLOBAL_INFO_AA	gsGlobalInfo_AA;
static MCDRV_PACKET_AA		gasPacket[MCDRV_MAX_PACKETS_AA+1];

/* register next address */
static const UINT16	gawNextAddressA[MCDRV_A_REG_NUM_AA] =
{
	0,		1,		2,		0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	8,		0xFFFF,	0xFFFF,	0xFFFF,	12,		13,		14,		15,
	16,		17,		18,		19,		20,		21,		22,		23,		0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF
};

static const UINT16	gawNextAddressB_BASE[MCDRV_B_BASE_REG_NUM_AA] =
{
	1,		2,		3,		4,		5,		6,		7,		0xFFFF,	9,		10,		11,		12,		13,		14,		15,		16,
	17,		18,		19,		20,		0xFFFF,	22,		23,		24,		25,		26,		27,		28,		29,		30,		31,		0xFFFF
};

static const UINT16	gawNextAddressB_MIXER[MCDRV_B_MIXER_REG_NUM_AA] =
{
	1,		2,		3,		4,		5,		6,		7,		8,		9,		10,		11,		12,		13,		14,		15,		16,
	17,		18,		19,		20,		21,		22,		23,		24,		25,		26,		27,		28,		29,		30,		31,		32,
	33,		34,		35,		36,		37,		38,		39,		40,		41,		42,		43,		44,		45,		46,		47,		48,
	49,		50,		51,		52,		53,		54,		55,		56,		57,		58,		59,		60,		61,		62,		63,		64,
	65,		66,		67,		68,		69,		70,		71,		72,		73,		74,		75,		76,		77,		78,		79,		80,
	81,		82,		83,		84,		85,		86,		87,		88,		89,		90,		91,		92,		93,		94,		95,		96,
	97,		98,		99,		100,	101,	102,	103,	104,	105,	106,	107,	108,	109,	110,	111,	112,
	113,	114,	115,	116,	117,	118,	119,	120,	121,	122,	123,	124,	125,	126,	127,	128,
	129,	130,	131,	132,	133,	134,	135,	136,	137,	138,	139,	140,	141,	142,	143,	144,
	145,	146,	147,	148,	149,	150,	151,	152,	153,	154,	155,	156,	157,	158,	159,	160,
	161,	162,	163,	164,	165,	166,	167,	168,	169,	170,	171,	172,	173,	174,	175,	176,
	177,	178,	179,	180,	181,	182,	183,	184,	185,	186,	187,	188,	189,	190,	191,	192,
	193,	194,	195,	196,	197,	198,	199,	200,	201,	202,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF
};

static const UINT16	gawNextAddressB_AE[MCDRV_B_AE_REG_NUM_AA] =
{
	1,		2,		3,		4,		5,		6,		7,		8,		9,		10,		11,		12,		13,		14,		15,		16,
	17,		18,		19,		20,		21,		22,		23,		24,		25,		26,		27,		28,		29,		30,		31,		32,
	33,		34,		35,		36,		37,		38,		39,		40,		41,		42,		43,		44,		45,		46,		47,		48,
	49,		50,		51,		52,		53,		54,		55,		56,		57,		58,		59,		60,		61,		62,		63,		64,
	65,		66,		67,		68,		69,		70,		71,		72,		73,		74,		75,		76,		77,		78,		79,		80,
	81,		82,		83,		84,		85,		86,		87,		88,		89,		90,		91,		92,		93,		94,		95,		96,
	97,		98,		99,		100,	101,	102,	103,	104,	105,	106,	107,	108,	109,	110,	111,	112,
	113,	114,	115,	116,	117,	118,	119,	120,	121,	122,	123,	124,	125,	126,	127,	128,
	129,	130,	131,	132,	133,	134,	135,	136,	137,	138,	139,	140,	141,	142,	143,	144,
	145,	146,	147,	148,	149,	150,	151,	152,	153,	154,	155,	156,	157,	158,	159,	160,
	161,	162,	163,	164,	165,	166,	167,	168,	169,	170,	171,	172,	173,	174,	175,	176,
	177,	178,	179,	180,	181,	182,	183,	184,	185,	186,	187,	188,	189,	190,	191,	192,
	193,	194,	195,	196,	197,	198,	199,	200,	201,	202,	203,	204,	205,	206,	207,	208,
	209,	210,	211,	212,	213,	214,	215,	216,	217,	218,	219,	220,	221,	222,	223,	224,
	225,	226,	227,	228,	229,	230,	231,	232,	233,	234,	235,	236,	237,	238,	239,	240,
	241,	242,	243,	244,	245,	246,	247,	248,	249,	250,	251,	252,	253,	254,	0xFFFF
};

static const UINT16	gawNextAddressB_CDSP[MCDRV_B_CDSP_REG_NUM_AA] =
{
	1,		2,		3,		4,		5,		6,		7,		8,		9,		10,		11,		12,		13,		14,		15,		16,
	17,		18,		19,		20,		21,		22,		23,		24,		25,		26,		27,		28,		29,		30,		31,		32,
	33,		34,		35,		36,		37,		38,		39,		40,		41,		42,		43,		44,		45,		46,		47,		48,
	49,		50,		51,		52,		53,		54,		55,		56,		57,		58,		59,		60,		61,		62,		63,		64,
	65,		66,		67,		68,		69,		70,		71,		72,		73,		74,		75,		76,		77,		78,		79,		80,
	81,		82,		83,		84,		85,		86,		87,		88,		89,		90,		91,		92,		93,		94,		95,		96,
	97,		98,		99,		100,	101,	102,	103,	104,	105,	106,	107,	108,	109,	110,	111,	112,
	113,	114,	115,	116,	117,	118,	119,	120,	121,	122,	123,	124,	125,	126,	127,	128,
	129,	0xFFFF
};

static const UINT16	gawNextAddressB_CODEC[MCDRV_B_CODEC_REG_NUM_AA] =
{
	1,		2,		3,		4,		5,		6,		7,		8,		9,		10,		11,		12,		13,		14,		15,		16,
	17,		18,		19,		20,		21,		22,		23,		24,		25,		26,		27,		28,		29,		30,		31,		32,
	33,		34,		35,		36,		37,		38,		39,		40,		41,		42,		43,		44,		45,		46,		47,		48,
	49,		50,		51,		52,		53,		54,		55,		56,		57,		58,		59,		60,		61,		62,		63,		64,
	65,		66,		67,		68,		69,		70,		71,		72,		73,		74,		75,		76,		77,		78,		79,		80,
	81,		82,		83,		84,		85,		86,		87,		88,		89,		90,		91,		92,		93,		94,		95,		96,
	97,		98,		99,		100,	101,	102,	103,	104,	105,	106,	107,	108,	109,	110,	111,	112,
	113,	114,	115,	116,	117,	118,	119,	120,	121,	122,	123,	124,	125,	126,	127,	0xFFFF
};

static const UINT16	gawNextAddressB_Ana[MCDRV_B_ANA_REG_NUM_AA] =
{
	1,		2,		3,		4,		5,		6,		7,		8,		9,		10,		11,		12,		13,		14,		15,		16,
	17,		18,		19,		20,		21,		22,		23,		24,		25,		26,		27,		28,		29,		30,		31,		32,
	33,		34,		35,		36,		37,		38,		39,		40,		41,		42,		43,		44,		45,		46,		47,		48,
	49,		50,		51,		52,		53,		54,		55,		56,		57,		58,		59,		60,		61,		62,		63,		64,
	65,		66,		67,		68,		69,		70,		71,		72,		73,		74,		75,		76,		77,		0xFFFF,	79,		80,
	81,		82,		83,		84,		85,		86,		87,		88,		89,		90,		91,		92,		93,		94,		95,		96,
	97,		98,		99,		100,	101,	102,	103,	104,	105,	106,	107,	108,	109,	110,	111,	112,
	113,	114,	115,	116,	117,	118,	119,	120,	121,	122,	123,	124,	125,	126,	127,	0xFFFF
};

/*	register access available	*/
static const MCDRV_REG_ACCSESS_AA	gawRegAccessAvailableA[256] =
{
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_READ_ONLY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
};
static const MCDRV_REG_ACCSESS_AA	gawRegAccessAvailableB_BASE[256] =
{
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
};
static const MCDRV_REG_ACCSESS_AA	gawRegAccessAvailableB_ANA[256] =
{
	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_ONLY_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
};
static const MCDRV_REG_ACCSESS_AA	gawRegAccessAvailableB_CODEC[256] =
{
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_READ_ONLY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
};
static const MCDRV_REG_ACCSESS_AA	gawRegAccessAvailableB_MIX[256] =
{
	eMCDRV_READ_ONLY_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_READ_ONLY_AA,
	eMCDRV_READ_ONLY_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_READ_ONLY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,
	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_READ_WRITE_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
};
static const MCDRV_REG_ACCSESS_AA	gawRegAccessAvailableB_AE[256] =
{
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,
	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_WRITE_ONLY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,	eMCDRV_ACCESS_DENY_AA,
};


static void		SetRegDefault(void);
static void		InitPathInfo(void);
static void		InitVolInfo(void);
static void		InitDioInfo(void);
static void		InitDacInfo(void);
static void		InitAdcInfo(void);
static void		InitSpInfo(void);
static void		InitDngInfo(void);
static void		InitAeInfo(void);
static void		InitPdmInfo(void);
static void		InitGpMode(void);
static void		InitGpMask(void);

static void		SetHPSourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetSPSourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetRCVSourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetLO1SourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetLO2SourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetPMSourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetDIT0SourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetDIT1SourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetDIT2SourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetDACSourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetAESourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetCDSPSourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetADC0SourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetADC1SourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetMixSourceOnOff(const MCDRV_PATH_INFO* psPathInfo);
static void		SetBiasSourceOnOff(const MCDRV_PATH_INFO* psPathInfo);

static void		SetDIOCommon(const MCDRV_DIO_INFO* psDioInfo, UINT8 bPort);
static void		SetDIODIR(const MCDRV_DIO_INFO* psDioInfo, UINT8 bPort);
static void		SetDIODIT(const MCDRV_DIO_INFO* psDioInfo, UINT8 bPort);

static SINT16	GetDigitalVolReg(SINT16 swVol);
static SINT16	GetADVolReg(SINT16 swVol);
static SINT16	GetLIVolReg(SINT16 swVol);
static SINT16	GetMcVolReg(SINT16 swVol);
static SINT16	GetMcGainReg(SINT16 swVol);
static SINT16	GetHpVolReg(SINT16 swVol);
static SINT16	GetHpGainReg(SINT16 swVol);
static SINT16	GetSpVolReg(SINT16 swVol);
static SINT16	GetRcVolReg(SINT16 swVol);
static SINT16	GetLoVolReg(SINT16 swVol);

static SINT32	WaitBitSet(UINT8 bSlaveAddr, UINT16 wRegAddr, UINT8 bBit, UINT32 dCycleTime, UINT32 dTimeOut);
static SINT32	WaitBitRelease(UINT8 bSlaveAddr, UINT16 wRegAddr, UINT8 bBit, UINT32 dCycleTime, UINT32 dTimeOut);

/****************************************************************************
 *	McResCtrl_SetHwId_AA
 *
 *	Description:
 *			Set hardware ID.
 *	Arguments:
 *			bHwId	hardware ID
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_INIT
 *
 ****************************************************************************/
SINT32	McResCtrl_SetHwId_AA
(
	UINT8	bHwId
)
{
	gsGlobalInfo_AA.bHwId	= bHwId;

	switch (bHwId)
	{
	case MCDRV_HWID_YMU821_AA:
		McDevProf_SetDevId(eMCDRV_DEV_ID_1N2);
		break;
	default:
		return MCDRV_ERROR_INIT;
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McResCtrl_Init_AA
 *
 *	Description:
 *			initialize the resource controller.
 *	Arguments:
 *			psInitInfo	pointer to the initialize information struct
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_Init_AA
(
	const MCDRV_INIT_INFO*	psInitInfo
)
{
	gsGlobalInfo_AA.ePacketBufAlloc	= eMCDRV_PACKETBUF_FREE_AA;
	SetRegDefault();

	gsGlobalInfo_AA.sInitInfo.bCkSel	= psInitInfo->bCkSel;
	gsGlobalInfo_AA.sInitInfo.bDivR0	= psInitInfo->bDivR0;
	gsGlobalInfo_AA.sInitInfo.bDivF0	= psInitInfo->bDivF0;
	if(McDevProf_IsValid(eMCDRV_FUNC_DIVR1) == 1)
	{
		gsGlobalInfo_AA.sInitInfo.bDivR1	= psInitInfo->bDivR1;
	}
	else
	{
		gsGlobalInfo_AA.sInitInfo.bDivR1	= 0;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_DIVF1) == 1)
	{
		gsGlobalInfo_AA.sInitInfo.bDivF1	= psInitInfo->bDivF1;
	}
	else
	{
		gsGlobalInfo_AA.sInitInfo.bDivF1	= 0;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_RANGE) == 1)
	{
		gsGlobalInfo_AA.sInitInfo.bRange0	= psInitInfo->bRange0;
		gsGlobalInfo_AA.sInitInfo.bRange1	= psInitInfo->bRange1;
	}
	else
	{
		gsGlobalInfo_AA.sInitInfo.bRange0	= 0;
		gsGlobalInfo_AA.sInitInfo.bRange1	= 0;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_BYPASS) == 1)
	{
		gsGlobalInfo_AA.sInitInfo.bBypass	= psInitInfo->bBypass;
	}
	else
	{
		gsGlobalInfo_AA.sInitInfo.bBypass	= 0;
	}
	gsGlobalInfo_AA.sInitInfo.bDioSdo0Hiz	= psInitInfo->bDioSdo0Hiz;
	gsGlobalInfo_AA.sInitInfo.bDioSdo1Hiz	= psInitInfo->bDioSdo1Hiz;
	gsGlobalInfo_AA.sInitInfo.bDioSdo2Hiz	= psInitInfo->bDioSdo2Hiz;
	gsGlobalInfo_AA.sInitInfo.bDioClk0Hiz	= psInitInfo->bDioClk0Hiz;
	gsGlobalInfo_AA.sInitInfo.bDioClk1Hiz	= psInitInfo->bDioClk1Hiz;
	gsGlobalInfo_AA.sInitInfo.bDioClk2Hiz	= psInitInfo->bDioClk2Hiz;
	gsGlobalInfo_AA.sInitInfo.bPcmHiz		= psInitInfo->bPcmHiz;
	gsGlobalInfo_AA.sInitInfo.bLineIn1Dif	= psInitInfo->bLineIn1Dif;
	if(McDevProf_IsValid(eMCDRV_FUNC_LI2) == 1)
	{
		gsGlobalInfo_AA.sInitInfo.bLineIn2Dif	= psInitInfo->bLineIn2Dif;
	}
	else
	{
		gsGlobalInfo_AA.sInitInfo.bLineIn2Dif	= MCDRV_LINE_STEREO;
	}
	gsGlobalInfo_AA.sInitInfo.bLineOut1Dif	= psInitInfo->bLineOut1Dif;
	gsGlobalInfo_AA.sInitInfo.bLineOut2Dif	= psInitInfo->bLineOut2Dif;
	gsGlobalInfo_AA.sInitInfo.bSpmn		= psInitInfo->bSpmn;
	gsGlobalInfo_AA.sInitInfo.bMic1Sng		= psInitInfo->bMic1Sng;
	gsGlobalInfo_AA.sInitInfo.bMic2Sng		= psInitInfo->bMic2Sng;
	gsGlobalInfo_AA.sInitInfo.bMic3Sng		= psInitInfo->bMic3Sng;
	gsGlobalInfo_AA.sInitInfo.bPowerMode	= psInitInfo->bPowerMode;
	gsGlobalInfo_AA.sInitInfo.bSpHiz		= psInitInfo->bSpHiz;
	gsGlobalInfo_AA.sInitInfo.bLdo			= psInitInfo->bLdo;
	gsGlobalInfo_AA.sInitInfo.bPad0Func	= psInitInfo->bPad0Func;
	gsGlobalInfo_AA.sInitInfo.bPad1Func	= psInitInfo->bPad1Func;
	if(McDevProf_IsValid(eMCDRV_FUNC_PAD2) == 1)
	{
		gsGlobalInfo_AA.sInitInfo.bPad2Func	= psInitInfo->bPad2Func;
	}
	else
	{
		gsGlobalInfo_AA.sInitInfo.bPad2Func	= MCDRV_PAD_GPIO;
	}
	gsGlobalInfo_AA.sInitInfo.bAvddLev		= psInitInfo->bAvddLev;
	gsGlobalInfo_AA.sInitInfo.bVrefLev		= psInitInfo->bVrefLev;
	gsGlobalInfo_AA.sInitInfo.bDclGain		= psInitInfo->bDclGain;
	gsGlobalInfo_AA.sInitInfo.bDclLimit	= psInitInfo->bDclLimit;
	gsGlobalInfo_AA.sInitInfo.bReserved1	= psInitInfo->bReserved1;
	gsGlobalInfo_AA.sInitInfo.bReserved2	= psInitInfo->bReserved2;
	gsGlobalInfo_AA.sInitInfo.bReserved3	= psInitInfo->bReserved3;
	gsGlobalInfo_AA.sInitInfo.bReserved4	= psInitInfo->bReserved4;
	gsGlobalInfo_AA.sInitInfo.bReserved5	= psInitInfo->bReserved5;
	gsGlobalInfo_AA.sInitInfo.sWaitTime	= psInitInfo->sWaitTime;

	InitPathInfo();
	InitVolInfo();
	InitDioInfo();
	InitDacInfo();
	InitAdcInfo();
	InitSpInfo();
	InitDngInfo();
	InitAeInfo();
	InitPdmInfo();
	InitGpMode();
	InitGpMask();

	McResCtrl_InitRegUpdate_AA();

	gsGlobalInfo_AA.eAPMode = eMCDRV_APM_OFF_AA;

	gsGlobalInfo_AA.eHwAdj = eMCDRV_HWADJ_THRU;
}

/****************************************************************************
 *	SetRegDefault
 *
 *	Description:
 *			Initialize the virtual registers.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetRegDefault
(
	void
)
{
	UINT16	i;
	for(i = 0; i < MCDRV_A_REG_NUM_AA; i++)
	{
		gsGlobalInfo_AA.abRegValA[i]	= 0;
	}
	gsGlobalInfo_AA.abRegValA[MCI_AA_RST]		= MCI_AA_RST_DEF;
	gsGlobalInfo_AA.abRegValA[MCI_AA_HW_ID]	= MCI_AA_HW_ID_DEF;

	for(i = 0; i < MCDRV_B_BASE_REG_NUM_AA; i++)
	{
		gsGlobalInfo_AA.abRegValB_BASE[i]	= 0;
	}
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_RSTB]				= MCI_AA_RSTB_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_PWM_DIGITAL]		= MCI_AA_PWM_DIGITAL_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_PWM_DIGITAL_1]		= MCI_AA_PWM_DIGITAL_1_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_PWM_DIGITAL_CDSP]	= MCI_AA_PWM_DIGITAL_CDSP_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_PWM_DIGITAL_BDSP]	= MCI_AA_PWM_DIGITAL_BDSP_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_SD_MSK]				= MCI_AA_SD_MSK_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_SD_MSK_1]			= MCI_AA_SD_MSK_1_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_BCLK_MSK]			= MCI_AA_BCLK_MSK_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_BCLK_MSK_1]			= MCI_AA_BCLK_MSK_1_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_BCKP]				= MCI_AA_BCKP_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_PLL_RST]				= MCI_AA_PLL_RST_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_DIVR0]				= MCI_AA_DIVR0_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_DIVF0]				= MCI_AA_DIVF0_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_PA_MSK]				= MCI_AA_PA_MSK_DEF;
	gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_PA_MSK_1]			= MCI_AA_PA_MSK_1_DEF;

	for(i = 0; i < MCDRV_B_MIXER_REG_NUM_AA; i++)
	{
		gsGlobalInfo_AA.abRegValB_MIXER[i]	= 0;
	}
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_DIT_ININTP]	= MCI_AA_DIT_ININTP_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_DIR_INTP]		= MCI_AA_DIR_INTP_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_ADC_INTP]		= MCI_AA_ADC_INTP_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_AINTP]			= MCI_AA_AINTP_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_IINTP]			= MCI_AA_IINTP_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_DAC_INTP]		= MCI_AA_DAC_INTP_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_DIR0_CH]		= MCI_AA_DIR0_CH_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_DIT0_SLOT]		= MCI_AA_DIT0_SLOT_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_PCM_RX0]		= MCI_AA_PCM_RX0_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_PCM_TX0]		= MCI_AA_PCM_TX0_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_PCM_SLOT_TX0]	= MCI_AA_PCM_SLOT_TX0_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_DIR1_CH]		= MCI_AA_DIR1_CH_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_DIT1_SLOT]		= MCI_AA_DIT1_SLOT_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_PCM_RX1]		= MCI_AA_PCM_RX1_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_PCM_TX1]		= MCI_AA_PCM_TX1_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_PCM_SLOT_TX1]	= MCI_AA_PCM_SLOT_TX1_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_DIR2_CH]		= MCI_AA_DIR2_CH_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_DIT2_SLOT]		= MCI_AA_DIT2_SLOT_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_PCM_RX2]		= MCI_AA_PCM_RX2_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_PCM_TX2]		= MCI_AA_PCM_TX2_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_PCM_SLOT_TX2]	= MCI_AA_PCM_SLOT_TX2_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_CDI_CH]		= MCI_AA_CDI_CH_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_CDO_SLOT]		= MCI_AA_CDO_SLOT_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_PDM_AGC]		= MCI_AA_PDM_AGC_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_PDM_MUTE]		= MCI_AA_PDM_MUTE_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_PDM_STWAIT]	= MCI_AA_PDM_STWAIT_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_CHP_H]			= MCI_AA_CHP_H_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_CHP_M]			= MCI_AA_CHP_M_DEF;
	gsGlobalInfo_AA.abRegValB_MIXER[MCI_AA_CHP_L]			= MCI_AA_CHP_L_DEF;

	for(i = 0; i < MCDRV_B_AE_REG_NUM_AA; i++)
	{
		gsGlobalInfo_AA.abRegValB_AE[i]	= 0;
	}
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_BAND0_CEQ0]	= MCI_AA_BAND0_CEQ0_H_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_BAND1_CEQ0]	= MCI_AA_BAND1_CEQ0_H_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_BAND2_CEQ0]	= MCI_AA_BAND2_CEQ0_H_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_BAND3H_CEQ0]	= MCI_AA_BAND3H_CEQ0_H_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_BAND4H_CEQ0]	= MCI_AA_BAND4H_CEQ0_H_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_BAND5_CEQ0]	= MCI_AA_BAND5_CEQ0_H_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_BAND6H_CEQ0]	= MCI_AA_BAND6H_CEQ0_H_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_BAND7H_CEQ0]	= MCI_AA_BAND7H_CEQ0_H_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_PDM_CHP0_H]	= MCI_AA_PDM_CHP0_H_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_PDM_CHP0_M]	= MCI_AA_PDM_CHP0_M_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_PDM_CHP0_L]	= MCI_AA_PDM_CHP0_L_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_PDM_CHP1_H]	= MCI_AA_PDM_CHP1_H_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_PDM_CHP1_M]	= MCI_AA_PDM_CHP1_M_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_PDM_CHP1_L]	= MCI_AA_PDM_CHP1_L_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_PDM_CHP3_H]	= MCI_AA_PDM_CHP3_H_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_PDM_CHP3_M]	= MCI_AA_PDM_CHP3_M_DEF;
	gsGlobalInfo_AA.abRegValB_AE[MCI_AA_PDM_CHP3_L]	= MCI_AA_PDM_CHP3_L_DEF;

	for(i = 0; i < MCDRV_B_CDSP_REG_NUM_AA; i++)
	{
		gsGlobalInfo_AA.abRegValB_CDSP[i]	= 0;
	}
	gsGlobalInfo_AA.abRegValB_CDSP[MCI_AA_JOEMP]		= MCI_AA_JOEMP_DEF;
	gsGlobalInfo_AA.abRegValB_CDSP[MCI_AA_JEEMP]		= MCI_AA_JEEMP_DEF;
	gsGlobalInfo_AA.abRegValB_CDSP[MCI_AA_CDSP_SRST]	= MCI_AA_CDSP_SRST_DEF;

	for(i = 0; i < MCDRV_B_ANA_REG_NUM_AA; i++)
	{
		gsGlobalInfo_AA.abRegValB_ANA[i]	= 0;
	}
	gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_ANA_RST]			= MCI_AA_ANA_RST_DEF;
	gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_PWM_ANALOG_0]	= MCI_AA_PWM_ANALOG_0_DEF;
	gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_PWM_ANALOG_1]	= MCI_AA_PWM_ANALOG_1_DEF;
	gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_PWM_ANALOG_2]	= MCI_AA_PWM_ANALOG_2_DEF;
	gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_PWM_ANALOG_3]	= MCI_AA_PWM_ANALOG_3_DEF;
	gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_PWM_ANALOG_4]	= MCI_AA_PWM_ANALOG_4_DEF;
	gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_DNGATRT]			= MCI_AA_DNGATRT_DEF;
	gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_DNGON]			= MCI_AA_DNGON_DEF;
	gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_HPVOL_L]			= MCI_AA_HPVOL_L_DEF;
	gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_SPVOL_L]			= MCI_AA_SPVOL_L_DEF;
	gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_RCVOL]			= MCI_AA_RCVOL_DEF;
	gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_LEV]				= MCI_AA_LEV_DEF;

	for(i = 0; i < MCDRV_B_CODEC_REG_NUM_AA; i++)
	{
		gsGlobalInfo_AA.abRegValB_CODEC[i]	= 0;
	}
	gsGlobalInfo_AA.abRegValB_CODEC[MCI_AA_DPADIF]		= MCI_AA_DPADIF_DEF;
	gsGlobalInfo_AA.abRegValB_CODEC[MCI_AA_AD_AGC]		= MCI_AA_AD_AGC_DEF;
	gsGlobalInfo_AA.abRegValB_CODEC[MCI_AA_DAC_CONFIG]	= MCI_AA_DAC_CONFIG_DEF;
}

/****************************************************************************
 *	InitPathInfo
 *
 *	Description:
 *			Initialize path info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitPathInfo
(
	void
)
{
	UINT8	bCh, bBlockIdx;
	UINT8	abOnOff[SOURCE_BLOCK_NUM];

	abOnOff[0]	= (MCDRV_SRC0_MIC1_OFF|MCDRV_SRC0_MIC2_OFF|MCDRV_SRC0_MIC3_OFF);
	abOnOff[1]	= (MCDRV_SRC1_LINE1_L_OFF|MCDRV_SRC1_LINE1_R_OFF|MCDRV_SRC1_LINE1_M_OFF);
	abOnOff[2]	= (MCDRV_SRC2_LINE2_L_OFF|MCDRV_SRC2_LINE2_R_OFF|MCDRV_SRC2_LINE2_M_OFF);
	abOnOff[3]	= (MCDRV_SRC3_DIR0_OFF|MCDRV_SRC3_DIR1_OFF|MCDRV_SRC3_DIR2_OFF|MCDRV_SRC3_DIR2_DIRECT_OFF);
	abOnOff[4]	= (MCDRV_SRC4_DTMF_OFF|MCDRV_SRC4_PDM_OFF|MCDRV_SRC4_ADC0_OFF|MCDRV_SRC4_ADC1_OFF);
	abOnOff[5]	= (MCDRV_SRC5_DAC_L_OFF|MCDRV_SRC5_DAC_R_OFF|MCDRV_SRC5_DAC_M_OFF);
	abOnOff[6]	= (MCDRV_SRC6_MIX_OFF|MCDRV_SRC6_AE_OFF|MCDRV_SRC6_CDSP_OFF|MCDRV_SRC6_CDSP_DIRECT_OFF);

	for(bCh = 0; bCh < HP_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asHpOut[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < SP_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asSpOut[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < RC_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asRcOut[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < LOUT1_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asLout1[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < LOUT2_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asLout2[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < PEAK_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asPeak[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < DIT0_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asDit0[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < DIT1_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asDit1[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < DIT2_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asDit2[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < DAC_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < AE_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asAe[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < CDSP_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asCdsp[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < ADC0_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asAdc0[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < ADC1_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asAdc1[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < MIX_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asMix[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	for(bCh = 0; bCh < BIAS_PATH_CHANNELS; bCh++)
	{
		for(bBlockIdx = 0; bBlockIdx < SOURCE_BLOCK_NUM; bBlockIdx++)
		{
			gsGlobalInfo_AA.sPathInfo.asBias[bCh].abSrcOnOff[bBlockIdx]	= abOnOff[bBlockIdx];
		}
	}
	gsGlobalInfo_AA.sPathInfoVirtual	= gsGlobalInfo_AA.sPathInfo;
}

/****************************************************************************
 *	InitVolInfo
 *
 *	Description:
 *			Initialize volume info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitVolInfo
(
	void
)
{
	UINT8	bCh;
	for(bCh = 0; bCh < AD0_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Ad0[bCh]			= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < AD1_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Ad1[bCh]			= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < AENG6_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Aeng6[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < PDM_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Pdm[bCh]			= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DTMF_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Dtmfb[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DIO0_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Dir0[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DIO1_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Dir1[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DIO2_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Dir2[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < AD0_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Ad0Att[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < AD1_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Ad1Att[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DIO0_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Dir0Att[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DIO1_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Dir1Att[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DIO2_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Dir2Att[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < PDM_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_SideTone[bCh]	= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DTFM_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_DtmfAtt[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DAC_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_DacMaster[bCh]	= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DAC_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_DacVoice[bCh]	= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DAC_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_DacAtt[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DIO0_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Dit0[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DIO1_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Dit1[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < DIO2_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswD_Dit2[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < AD0_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Ad0[bCh]			= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < AD1_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Ad1[bCh]			= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < LIN1_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Lin1[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < LIN2_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Lin2[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < MIC1_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Mic1[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < MIC2_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Mic2[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < MIC3_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Mic3[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < HP_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Hp[bCh]			= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < SP_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Sp[bCh]			= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < RC_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Rc[bCh]			= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < LOUT1_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Lout1[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < LOUT2_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Lout2[bCh]		= MCDRV_LOGICAL_VOL_MUTE;
	}
	for(bCh = 0; bCh < MIC1_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Mic1Gain[bCh]	= MCDRV_LOGICAL_MICGAIN_DEF;
	}
	for(bCh = 0; bCh < MIC2_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Mic2Gain[bCh]	= MCDRV_LOGICAL_MICGAIN_DEF;
	}
	for(bCh = 0; bCh < MIC3_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_Mic3Gain[bCh]	= MCDRV_LOGICAL_MICGAIN_DEF;
	}
	for(bCh = 0; bCh < HPGAIN_VOL_CHANNELS; bCh++)
	{
		gsGlobalInfo_AA.sVolInfo.aswA_HpGain[bCh]		= MCDRV_LOGICAL_HPGAIN_DEF;
	}
}

/****************************************************************************
 *	InitDioInfo
 *
 *	Description:
 *			Initialize Digital I/O info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitDioInfo
(
	void
)
{
	UINT8	bDioIdx, bDioCh;
	for(bDioIdx = 0; bDioIdx < 3; bDioIdx++)
	{
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDioCommon.bMasterSlave	= MCDRV_DIO_SLAVE;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDioCommon.bAutoFs		= MCDRV_AUTOFS_ON;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDioCommon.bFs			= MCDRV_FS_48000;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDioCommon.bBckFs			= MCDRV_BCKFS_64;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDioCommon.bInterface		= MCDRV_DIO_DA;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDioCommon.bBckInvert		= MCDRV_BCLK_NORMAL;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDioCommon.bPcmHizTim		= MCDRV_PCMHIZTIM_FALLING;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDioCommon.bPcmClkDown	= MCDRV_PCM_CLKDOWN_OFF;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDioCommon.bPcmFrame		= MCDRV_PCM_SHORTFRAME;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDioCommon.bPcmHighPeriod	= 0;

		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDir.wSrcRate				= 0x0000;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDir.sDaFormat.bBitSel	= MCDRV_BITSEL_16;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDir.sDaFormat.bMode		= MCDRV_DAMODE_HEADALIGN;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDir.sPcmFormat.bMono		= MCDRV_PCM_MONO;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDir.sPcmFormat.bOrder	= MCDRV_PCM_MSB_FIRST;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDir.sPcmFormat.bLaw		= MCDRV_PCM_LINEAR;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDir.sPcmFormat.bBitSel	= MCDRV_PCM_BITSEL_8;
		for(bDioCh = 0; bDioCh < DIO_CHANNELS; bDioCh++)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDir.abSlot[bDioCh]	= bDioCh;
		}

		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDit.wSrcRate				= 0x0000;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDit.sDaFormat.bBitSel	= MCDRV_BITSEL_16;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDit.sDaFormat.bMode		= MCDRV_DAMODE_HEADALIGN;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDit.sPcmFormat.bMono		= MCDRV_PCM_MONO;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDit.sPcmFormat.bOrder	= MCDRV_PCM_MSB_FIRST;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDit.sPcmFormat.bLaw		= MCDRV_PCM_LINEAR;
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDit.sPcmFormat.bBitSel	= MCDRV_PCM_BITSEL_8;
		for(bDioCh = 0; bDioCh < DIO_CHANNELS; bDioCh++)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bDioIdx].sDit.abSlot[bDioCh]	= bDioCh;
		}
	}
}

/****************************************************************************
 *	InitDacInfo
 *
 *	Description:
 *			Initialize Dac info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitDacInfo
(
	void
)
{
	gsGlobalInfo_AA.sDacInfo.bMasterSwap	= MCDRV_DSWAP_OFF;
	gsGlobalInfo_AA.sDacInfo.bVoiceSwap	= MCDRV_DSWAP_OFF;
	gsGlobalInfo_AA.sDacInfo.bDcCut		= MCDRV_DCCUT_ON;
}

/****************************************************************************
 *	InitAdcInfo
 *
 *	Description:
 *			Initialize Adc info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitAdcInfo
(
	void
)
{
	gsGlobalInfo_AA.sAdcInfo.bAgcAdjust	= MCDRV_AGCADJ_0;
	gsGlobalInfo_AA.sAdcInfo.bAgcOn		= MCDRV_AGC_OFF;
	gsGlobalInfo_AA.sAdcInfo.bMono			= MCDRV_ADC_STEREO;
}

/****************************************************************************
 *	InitSpInfo
 *
 *	Description:
 *			Initialize SP info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitSpInfo
(
	void
)
{
	gsGlobalInfo_AA.sSpInfo.bSwap	= MCDRV_SPSWAP_OFF;
}

/****************************************************************************
 *	InitDngInfo
 *
 *	Description:
 *			Initialize DNG info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitDngInfo
(
	void
)
{
	gsGlobalInfo_AA.sDngInfo.abOnOff[0]		= MCDRV_DNG_OFF;
	gsGlobalInfo_AA.sDngInfo.abThreshold[0]	= MCDRV_DNG_THRES_48;
	gsGlobalInfo_AA.sDngInfo.abHold[0]			= MCDRV_DNG_HOLD_500;
	gsGlobalInfo_AA.sDngInfo.abAttack[0]		= 2/*MCDRV_DNG_ATTACK_1100*/;
	gsGlobalInfo_AA.sDngInfo.abRelease[0]		= MCDRV_DNG_RELEASE_940;
}

/****************************************************************************
 *	InitAeInfo
 *
 *	Description:
 *			Initialize Audio Engine info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitAeInfo
(
	void
)
{
	if(McDevProf_IsValid(eMCDRV_FUNC_HWADJ) == 1)
	{
		gsGlobalInfo_AA.sAeInfo.bOnOff		= MCDRV_EQ3_ON;
	}
	else
	{
		gsGlobalInfo_AA.sAeInfo.bOnOff		= 0;
	}
}

/****************************************************************************
 *	InitPdmInfo
 *
 *	Description:
 *			Initialize Pdm info.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitPdmInfo
(
	void
)
{
	gsGlobalInfo_AA.sPdmInfo.bClk			= MCDRV_PDM_CLK_64;
	gsGlobalInfo_AA.sPdmInfo.bAgcAdjust	= MCDRV_AGCADJ_0;
	gsGlobalInfo_AA.sPdmInfo.bAgcOn		= MCDRV_AGC_OFF;
	gsGlobalInfo_AA.sPdmInfo.bPdmEdge		= MCDRV_PDMEDGE_LH;
	gsGlobalInfo_AA.sPdmInfo.bPdmWait		= MCDRV_PDMWAIT_10;
	gsGlobalInfo_AA.sPdmInfo.bPdmSel		= MCDRV_PDMSEL_L1R2;
	gsGlobalInfo_AA.sPdmInfo.bMono			= MCDRV_PDM_STEREO;
}

/****************************************************************************
 *	InitGpMode
 *
 *	Description:
 *			Initialize Gp mode.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitGpMode
(
	void
)
{
	UINT8	bGpioIdx;
	for(bGpioIdx = 0; bGpioIdx < GPIO_PAD_NUM; bGpioIdx++)
	{
		gsGlobalInfo_AA.sGpMode.abGpDdr[bGpioIdx]		= MCDRV_GPDDR_IN;
		gsGlobalInfo_AA.sGpMode.abGpMode[bGpioIdx]		= MCDRV_GPMODE_RISING;
		gsGlobalInfo_AA.sGpMode.abGpHost[bGpioIdx]		= MCDRV_GPHOST_SCU;
		gsGlobalInfo_AA.sGpMode.abGpInvert[bGpioIdx]	= MCDRV_GPINV_NORMAL;
	}
}

/****************************************************************************
 *	InitGpMask
 *
 *	Description:
 *			Initialize Gp mask.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	InitGpMask
(
	void
)
{
	UINT8	bGpioIdx;
	for(bGpioIdx = 0; bGpioIdx < GPIO_PAD_NUM; bGpioIdx++)
	{
		gsGlobalInfo_AA.abGpMask[bGpioIdx]		= MCDRV_GPMASK_ON;
	}
}

/****************************************************************************
 *	McResCtrl_UpdateState_AA
 *
 *	Description:
 *			update state.
 *	Arguments:
 *			eState	state
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_UpdateState_AA
(
	MCDRV_STATE_AA	eState
)
{
	geState = eState;
}

/****************************************************************************
 *	McResCtrl_GetState_AA
 *
 *	Description:
 *			Get state.
 *	Arguments:
 *			none
 *	Return:
 *			current state
 *
 ****************************************************************************/
MCDRV_STATE_AA	McResCtrl_GetState_AA
(
	void
)
{
	return geState;
}

/****************************************************************************
 *	McResCtrl_GetRegVal_AA
 *
 *	Description:
 *			Get register value.
 *	Arguments:
 *			wRegType	register type
 *			wRegAddr	address
 *	Return:
 *			register value
 *
 ****************************************************************************/
UINT8	McResCtrl_GetRegVal_AA
(
	UINT16	wRegType,
	UINT16	wRegAddr
)
{
	switch(wRegType)
	{
	case	MCDRV_PACKET_REGTYPE_A_AA:
		return gsGlobalInfo_AA.abRegValA[wRegAddr];
	case	MCDRV_PACKET_REGTYPE_B_BASE_AA:
		return gsGlobalInfo_AA.abRegValB_BASE[wRegAddr];
	case	MCDRV_PACKET_REGTYPE_B_MIXER_AA:
		return gsGlobalInfo_AA.abRegValB_MIXER[wRegAddr];
	case	MCDRV_PACKET_REGTYPE_B_AE_AA:
		return gsGlobalInfo_AA.abRegValB_AE[wRegAddr];
	case	MCDRV_PACKET_REGTYPE_B_CDSP_AA:
		return gsGlobalInfo_AA.abRegValB_CDSP[wRegAddr];
	case	MCDRV_PACKET_REGTYPE_B_CODEC_AA:
		return gsGlobalInfo_AA.abRegValB_CODEC[wRegAddr];
	case	MCDRV_PACKET_REGTYPE_B_ANA_AA:
		return gsGlobalInfo_AA.abRegValB_ANA[wRegAddr];
	default:
		break;
	}

	return 0;
}

/****************************************************************************
 *	McResCtrl_SetRegVal_AA
 *
 *	Description:
 *			Set register value.
 *	Arguments:
 *			wRegType	register type
 *			wRegAddr	address
 *			bRegVal		register value
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetRegVal_AA
(
	UINT16	wRegType,
	UINT16	wRegAddr,
	UINT8	bRegVal
)
{
	switch(wRegType)
	{
	case	MCDRV_PACKET_REGTYPE_A_AA:
		gsGlobalInfo_AA.abRegValA[wRegAddr]		= bRegVal;
		break;
	case	MCDRV_PACKET_REGTYPE_B_BASE_AA:
		gsGlobalInfo_AA.abRegValB_BASE[wRegAddr]	= bRegVal;
		break;
	case	MCDRV_PACKET_REGTYPE_B_MIXER_AA:
		gsGlobalInfo_AA.abRegValB_MIXER[wRegAddr]	= bRegVal;
		break;
	case	MCDRV_PACKET_REGTYPE_B_AE_AA:
		gsGlobalInfo_AA.abRegValB_AE[wRegAddr]		= bRegVal;
		break;
	case	MCDRV_PACKET_REGTYPE_B_CDSP_AA:
		gsGlobalInfo_AA.abRegValB_CDSP[wRegAddr]	= bRegVal;
		break;
	case	MCDRV_PACKET_REGTYPE_B_CODEC_AA:
		gsGlobalInfo_AA.abRegValB_CODEC[wRegAddr]	= bRegVal;
		break;
	case	MCDRV_PACKET_REGTYPE_B_ANA_AA:
		gsGlobalInfo_AA.abRegValB_ANA[wRegAddr]	= bRegVal;
		break;
	default:
		break;
	}
}

/****************************************************************************
 *	McResCtrl_GetInitInfo_AA
 *
 *	Description:
 *			Get Initialize information.
 *	Arguments:
 *			psInitInfo	Initialize information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetInitInfo_AA
(
	MCDRV_INIT_INFO*	psInitInfo
)
{
	*psInitInfo	= gsGlobalInfo_AA.sInitInfo;
}

/****************************************************************************
 *	McResCtrl_SetClockInfo_AA
 *
 *	Description:
 *			Set clock information.
 *	Arguments:
 *			psClockInfo	clock information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetClockInfo_AA
(
	const MCDRV_CLOCK_INFO* psClockInfo
)
{

	gsGlobalInfo_AA.sInitInfo.bCkSel	= psClockInfo->bCkSel;
	gsGlobalInfo_AA.sInitInfo.bDivR0	= psClockInfo->bDivR0;
	gsGlobalInfo_AA.sInitInfo.bDivF0	= psClockInfo->bDivF0;
	if(McDevProf_IsValid(eMCDRV_FUNC_DIVR1) == 1)
	{
		gsGlobalInfo_AA.sInitInfo.bDivR1	= psClockInfo->bDivR1;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_DIVF1) == 1)
	{
		gsGlobalInfo_AA.sInitInfo.bDivF1	= psClockInfo->bDivF1;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_RANGE) == 1)
	{
		gsGlobalInfo_AA.sInitInfo.bRange0	= psClockInfo->bRange0;
		gsGlobalInfo_AA.sInitInfo.bRange1	= psClockInfo->bRange1;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_BYPASS) == 1)
	{
		gsGlobalInfo_AA.sInitInfo.bBypass	= psClockInfo->bBypass;
	}
}

/****************************************************************************
 *	McResCtrl_SetPathInfo_AA
 *
 *	Description:
 *			Set path information.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetPathInfo_AA
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{

	gsGlobalInfo_AA.sPathInfo	= gsGlobalInfo_AA.sPathInfoVirtual;

	/*	HP source on/off	*/
	SetHPSourceOnOff(psPathInfo);

	/*	SP source on/off	*/
	SetSPSourceOnOff(psPathInfo);

	/*	RCV source on/off	*/
	SetRCVSourceOnOff(psPathInfo);

	/*	LOut1 source on/off	*/
	SetLO1SourceOnOff(psPathInfo);

	/*	LOut2 source on/off	*/
	SetLO2SourceOnOff(psPathInfo);

	/*	Peak source on/off	*/
	SetPMSourceOnOff(psPathInfo);

	/*	DIT0 source on/off	*/
	SetDIT0SourceOnOff(psPathInfo);

	/*	DIT1 source on/off	*/
	SetDIT1SourceOnOff(psPathInfo);

	/*	DIT2 source on/off	*/
	SetDIT2SourceOnOff(psPathInfo);

	/*	DAC source on/off	*/
	SetDACSourceOnOff(psPathInfo);

	/*	AE source on/off	*/
	SetAESourceOnOff(psPathInfo);

	/*	CDSP source on/off	*/
	SetCDSPSourceOnOff(psPathInfo);

	/*	ADC0 source on/off	*/
	SetADC0SourceOnOff(psPathInfo);

	/*	ADC1 source on/off	*/
	SetADC1SourceOnOff(psPathInfo);

	/*	Mix source on/off	*/
	SetMixSourceOnOff(psPathInfo);

	/*	Bias source on/off	*/
	SetBiasSourceOnOff(psPathInfo);

	gsGlobalInfo_AA.sPathInfoVirtual	= gsGlobalInfo_AA.sPathInfo;

	if((McResCtrl_IsDstUsed_AA(eMCDRV_DST_ADC0_AA, eMCDRV_DST_CH0_AA) == 0)
	&& (McResCtrl_IsDstUsed_AA(eMCDRV_DST_ADC0_AA, eMCDRV_DST_CH1_AA) == 0))
	{/*	ADC0 source all off	*/
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]		&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]		|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
	}

	if((McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DAC_L_AA) == 0)
	&& (McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DAC_M_AA) == 0)
	&& (McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DAC_R_AA) == 0))
	{/*	DAC is unused	*/
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]		&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]		&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]		&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]		&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]		|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]		|= MCDRV_SRC6_MIX_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]		|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]		|= MCDRV_SRC6_MIX_OFF;
	}

	if(McResCtrl_GetAESource_AA() == eMCDRV_SRC_NONE_AA)
	{/*	AE source all off	*/
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
	}
	else if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_AE_AA) == 0)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}

	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_MIX_AA, eMCDRV_DST_CH0_AA) == 0)
	{/*	MIX source all off	*/
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]		&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]		&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]		&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]		|= MCDRV_SRC6_MIX_OFF;
		gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]		|= MCDRV_SRC6_MIX_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]		|= MCDRV_SRC6_MIX_OFF;
	}
	else if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_MIX_AA) == 0)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]		&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]		|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
	}

	if((McResCtrl_GetDACSource_AA(eMCDRV_DAC_MASTER_AA) == eMCDRV_SRC_NONE_AA)
	&& (McResCtrl_GetDACSource_AA(eMCDRV_DAC_VOICE_AA) == eMCDRV_SRC_NONE_AA))
	{/*	DAC source all off	*/
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_ON;
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_ON;
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_ON;
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_ON;
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_ON;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_ON;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_OFF;
	}

	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_ADC0_AA) == 0)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]		&= (UINT8)~MCDRV_SRC0_MIC1_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]		&= (UINT8)~MCDRV_SRC0_MIC2_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]		&= (UINT8)~MCDRV_SRC0_MIC3_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_L_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]		&= (UINT8)~MCDRV_SRC0_MIC1_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]		&= (UINT8)~MCDRV_SRC0_MIC2_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]		&= (UINT8)~MCDRV_SRC0_MIC3_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_R_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_ON;

		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]		|= MCDRV_SRC0_MIC1_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]		|= MCDRV_SRC0_MIC2_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]		|= MCDRV_SRC0_MIC3_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	|= MCDRV_SRC1_LINE1_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]		|= MCDRV_SRC0_MIC1_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]		|= MCDRV_SRC0_MIC2_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]		|= MCDRV_SRC0_MIC3_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	|= MCDRV_SRC1_LINE1_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_OFF;
	}
}

/****************************************************************************
 *	SetHPSourceOnOff
 *
 *	Description:
 *			Set HP source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetHPSourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_ON;
	}
	else if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_OFF;
	}
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_ON;
	}
	else if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_OFF;
	}
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_ON;
	}
	else if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_OFF;
	}

	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	|= MCDRV_SRC1_LINE1_L_ON;
	}
	else if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_L_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	|= MCDRV_SRC1_LINE1_L_OFF;
	}
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_ON;
	}
	else if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_OFF;
	}

	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_ON;
	}
	else if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_OFF;
	}
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_ON;
	}
	else if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_OFF;
	}

	if((psPathInfo->asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_ON;
	}
	else if((psPathInfo->asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_OFF;
	}
	if((psPathInfo->asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_ON;
	}
	else if((psPathInfo->asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_OFF;
	}
	if((psPathInfo->asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_ON;
	}
	else if((psPathInfo->asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_OFF;
	}

	if((psPathInfo->asHpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	|= MCDRV_SRC1_LINE1_R_ON;
	}
	else if((psPathInfo->asHpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_R_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	|= MCDRV_SRC1_LINE1_R_OFF;
	}

	if((psPathInfo->asHpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_ON;
	}
	else if((psPathInfo->asHpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_ON;
		gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_OFF;
	}
}

/****************************************************************************
 *	SetSPSourceOnOff
 *
 *	Description:
 *			Set SP source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetSPSourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	|= MCDRV_SRC1_LINE1_L_ON;
	}
	else if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_L_ON;
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	|= MCDRV_SRC1_LINE1_L_OFF;
	}
	if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_ON;
	}
	else if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_ON;
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_OFF;
	}

	if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_ON;
	}
	else if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_ON;
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_OFF;
	}
	if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_ON;
	}
	else if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_ON;
		gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_OFF;
	}

	if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	|= MCDRV_SRC1_LINE1_R_ON;
	}
	else if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_R_ON;
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	|= MCDRV_SRC1_LINE1_R_OFF;
	}
	if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_ON;
	}
	else if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_ON;
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_OFF;
	}

	if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_ON;
	}
	else if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_ON;
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_OFF;
	}
	if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_ON;
	}
	else if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_ON;
		gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_OFF;
	}
}

/****************************************************************************
 *	SetRCVSourceOnOff
 *
 *	Description:
 *			Set RCV source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetRCVSourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	if((psPathInfo->asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_OFF;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_ON;
	}
	else if((psPathInfo->asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_ON;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_OFF;
	}
	if((psPathInfo->asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_OFF;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_ON;
	}
	else if((psPathInfo->asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_ON;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_OFF;
	}
	if((psPathInfo->asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_OFF;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_ON;
	}
	else if((psPathInfo->asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_ON;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_OFF;
	}

	if((psPathInfo->asRcOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_ON;
	}
	else if((psPathInfo->asRcOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_ON;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_OFF;
	}

	if((psPathInfo->asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_ON;
	}
	else if((psPathInfo->asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_ON;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_OFF;
	}

	if((psPathInfo->asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_ON;
	}
	else if((psPathInfo->asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_ON;
		gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_OFF;
	}
}

/****************************************************************************
 *	SetLO1SourceOnOff
 *
 *	Description:
 *			Set LOut1 source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetLO1SourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_ON;
	}
	else if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_OFF;
	}
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_ON;
	}
	else if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_OFF;
	}
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_ON;
	}
	else if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_OFF;
	}

	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	|= MCDRV_SRC1_LINE1_L_ON;
	}
	else if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_L_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	|= MCDRV_SRC1_LINE1_L_OFF;
	}
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_ON;
	}
	else if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_OFF;
	}

	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_ON;
	}
	else if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_OFF;
	}
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_ON;
	}
	else if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_OFF;
	}

	if((psPathInfo->asLout1[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_ON;
	}
	else if((psPathInfo->asLout1[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_OFF;
	}
	if((psPathInfo->asLout1[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_ON;
	}
	else if((psPathInfo->asLout1[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_OFF;
	}
	if((psPathInfo->asLout1[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_ON;
	}
	else if((psPathInfo->asLout1[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_OFF;
	}

	if((psPathInfo->asLout1[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	|= MCDRV_SRC1_LINE1_R_ON;
	}
	else if((psPathInfo->asLout1[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_R_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	|= MCDRV_SRC1_LINE1_R_OFF;
	}

	if((psPathInfo->asLout1[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_ON;
	}
	else if((psPathInfo->asLout1[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_ON;
		gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_OFF;
	}
}

/****************************************************************************
 *	SetLO2SourceOnOff
 *
 *	Description:
 *			Set LOut2 source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetLO2SourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_ON;
	}
	else if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_OFF;
	}
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_ON;
	}
	else if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_OFF;
	}
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_ON;
	}
	else if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_OFF;
	}

	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	|= MCDRV_SRC1_LINE1_L_ON;
	}
	else if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_L_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	|= MCDRV_SRC1_LINE1_L_OFF;
	}
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_ON;
	}
	else if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_OFF;
	}

	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_ON;
	}
	else if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_L_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK]	|= MCDRV_SRC5_DAC_L_OFF;
	}
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_ON;
	}
	else if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_M_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK]	|= MCDRV_SRC5_DAC_M_OFF;
	}

	if((psPathInfo->asLout2[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_ON;
	}
	else if((psPathInfo->asLout2[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_OFF;
	}
	if((psPathInfo->asLout2[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_ON;
	}
	else if((psPathInfo->asLout2[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_OFF;
	}
	if((psPathInfo->asLout2[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_ON;
	}
	else if((psPathInfo->asLout2[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_OFF;
	}

	if((psPathInfo->asLout2[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	|= MCDRV_SRC1_LINE1_R_ON;
	}
	else if((psPathInfo->asLout2[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_R_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	|= MCDRV_SRC1_LINE1_R_OFF;
	}

	if((psPathInfo->asLout2[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_ON;
	}
	else if((psPathInfo->asLout2[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	&= (UINT8)~MCDRV_SRC5_DAC_R_ON;
		gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK]	|= MCDRV_SRC5_DAC_R_OFF;
	}
}

/****************************************************************************
 *	SetPMSourceOnOff
 *
 *	Description:
 *			Set PeakMeter source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetPMSourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	(void)psPathInfo;
}

/****************************************************************************
 *	SetDIT0SourceOnOff
 *
 *	Description:
 *			Set DIT0 source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetDIT0SourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	&& gsGlobalInfo_AA.sInitInfo.bPad0Func == MCDRV_PAD_PDMCK)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
	}

	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
	}

	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
	}

	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
	}

	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_ON;
	}
	else if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
}

/****************************************************************************
 *	SetDIT1SourceOnOff
 *
 *	Description:
 *			Set DIT1 source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetDIT1SourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	&& gsGlobalInfo_AA.sInitInfo.bPad0Func == MCDRV_PAD_PDMCK)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
	}

	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
	}

	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
	}

	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
	}

	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_ON;
	}
	else if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
}

/****************************************************************************
 *	SetDIT2SourceOnOff
 *
 *	Description:
 *			Set DIT2 source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetDIT2SourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	&& gsGlobalInfo_AA.sInitInfo.bPad0Func == MCDRV_PAD_PDMCK)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
	}

	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
	}

	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
	}

	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
	}

	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_OFF;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_ON;
	}
	else if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
}

/****************************************************************************
 *	SetDACSourceOnOff
 *
 *	Description:
 *			Set DAC source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetDACSourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	UINT8	bCh;
	for(bCh = 0; bCh < DAC_PATH_CHANNELS; bCh++)
	{
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		&& gsGlobalInfo_AA.sInitInfo.bPad0Func == MCDRV_PAD_PDMCK)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
		}
		else if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_OFF)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		}

		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
		}
		else if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_OFF)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		}

		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
		}
		else if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_OFF)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
		}
		else if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_OFF)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
		}
		else if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_OFF)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		}

		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
		}
		else if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_OFF)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
		}

		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_OFF;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_ON;
		}
		else if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_OFF)
		{
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
			gsGlobalInfo_AA.sPathInfo.asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
		}
	}
}

/****************************************************************************
 *	SetAESourceOnOff
 *
 *	Description:
 *			Set AE source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetAESourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	&& gsGlobalInfo_AA.sInitInfo.bPad0Func == MCDRV_PAD_PDMCK)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
	}

	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
	}

	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
	else if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
	}

	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_OFF;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_ON;
	}
	else if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
		gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
	}
}

/****************************************************************************
 *	SetCDSPSourceOnOff
 *
 *	Description:
 *			Set CDSP source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetCDSPSourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	(void)psPathInfo;
}

/****************************************************************************
 *	SetADC0SourceOnOff
 *
 *	Description:
 *			Set ADC0 source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetADC0SourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_ON;
	}
	else if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_OFF;
	}
	if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_ON;
	}
	else if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_OFF;
	}
	if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_ON;
	}
	else if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_OFF;
	}

	if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_L_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	|= MCDRV_SRC1_LINE1_L_ON;
	}
	else if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_L_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK]	|= MCDRV_SRC1_LINE1_L_OFF;
	}
	if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_ON;
	}
	else if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_OFF;
	}

	if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_ON;
	}
	else if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_OFF;
	}
	if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_ON;
	}
	else if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_OFF;
	}
	if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_ON;
	}
	else if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_OFF;
	}

	if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_R_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	|= MCDRV_SRC1_LINE1_R_ON;
	}
	else if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_R_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK]	|= MCDRV_SRC1_LINE1_R_OFF;
	}
	if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_OFF;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_ON;
	}
	else if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	&= (UINT8)~MCDRV_SRC1_LINE1_M_ON;
		gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK]	|= MCDRV_SRC1_LINE1_M_OFF;
	}
}

/****************************************************************************
 *	SetADC1SourceOnOff
 *
 *	Description:
 *			Set ADC1 source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetADC1SourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	(void)psPathInfo;
}

/****************************************************************************
 *	SetMixSourceOnOff
 *
 *	Description:
 *			Set Mix source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetMixSourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	&& gsGlobalInfo_AA.sInitInfo.bPad0Func == MCDRV_PAD_PDMCK)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_ON;
	}
	else if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
	}

	if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_ON;
	}
	else if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
	}

	if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_ON;
	}
	else if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
	}
	if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_ON;
	}
	else if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
	}
	if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_ON;
	}
	else if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
	}

	if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_OFF;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_ON;
	}
	else if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_ON;
		gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_OFF;
	}
}

/****************************************************************************
 *	SetExtBiasOnOff
 *
 *	Description:
 *			Set Bias source On/Off.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetBiasSourceOnOff
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	if((psPathInfo->asBias[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_OFF;
		gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_ON;
	}
	else if((psPathInfo->asBias[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & (MCDRV_SRC0_MIC1_ON|MCDRV_SRC0_MIC1_OFF)) == MCDRV_SRC0_MIC1_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC1_ON;
		gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK]	|= MCDRV_SRC0_MIC1_OFF;
	}
	if((psPathInfo->asBias[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_OFF;
		gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_ON;
	}
	else if((psPathInfo->asBias[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & (MCDRV_SRC0_MIC2_ON|MCDRV_SRC0_MIC2_OFF)) == MCDRV_SRC0_MIC2_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC2_ON;
		gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK]	|= MCDRV_SRC0_MIC2_OFF;
	}
	if((psPathInfo->asBias[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_ON)
	{
		gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_OFF;
		gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_ON;
	}
	else if((psPathInfo->asBias[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & (MCDRV_SRC0_MIC3_ON|MCDRV_SRC0_MIC3_OFF)) == MCDRV_SRC0_MIC3_OFF)
	{
		gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	&= (UINT8)~MCDRV_SRC0_MIC3_ON;
		gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK]	|= MCDRV_SRC0_MIC3_OFF;
	}
}

/****************************************************************************
 *	McResCtrl_GetPathInfo_AA
 *
 *	Description:
 *			Get path information.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetPathInfo_AA
(
	MCDRV_PATH_INFO*	psPathInfo
)
{
	*psPathInfo	= gsGlobalInfo_AA.sPathInfo;
}

/****************************************************************************
 *	McResCtrl_GetPathInfoVirtual_AA
 *
 *	Description:
 *			Get virtaul path information.
 *	Arguments:
 *			psPathInfo	virtaul path information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetPathInfoVirtual_AA
(
	MCDRV_PATH_INFO*	psPathInfo
)
{
	*psPathInfo	= gsGlobalInfo_AA.sPathInfoVirtual;
}

/****************************************************************************
 *	McResCtrl_SetDioInfo_AA
 *
 *	Description:
 *			Set digital io information.
 *	Arguments:
 *			psDioInfo	digital io information
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetDioInfo_AA
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT32	dUpdateInfo
)
{

	if((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != 0)
	{
		SetDIOCommon(psDioInfo, 0);
	}
	if((dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != 0)
	{
		SetDIOCommon(psDioInfo, 1);
	}
	if((dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != 0)
	{
		SetDIOCommon(psDioInfo, 2);
	}

	if((dUpdateInfo & MCDRV_DIO0_DIR_UPDATE_FLAG) != 0)
	{
		SetDIODIR(psDioInfo, 0);
	}
	if((dUpdateInfo & MCDRV_DIO1_DIR_UPDATE_FLAG) != 0)
	{
		SetDIODIR(psDioInfo, 1);
	}
	if((dUpdateInfo & MCDRV_DIO2_DIR_UPDATE_FLAG) != 0)
	{
		SetDIODIR(psDioInfo, 2);
	}

	if((dUpdateInfo & MCDRV_DIO0_DIT_UPDATE_FLAG) != 0)
	{
		SetDIODIT(psDioInfo, 0);
	}
	if((dUpdateInfo & MCDRV_DIO1_DIT_UPDATE_FLAG) != 0)
	{
		SetDIODIT(psDioInfo, 1);
	}
	if((dUpdateInfo & MCDRV_DIO2_DIT_UPDATE_FLAG) != 0)
	{
		SetDIODIT(psDioInfo, 2);
	}
}

/****************************************************************************
 *	SetDIOCommon
 *
 *	Description:
 *			Set digital io common information.
 *	Arguments:
 *			psDioInfo	digital io information
 *			bPort		port number
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetDIOCommon
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT8	bPort
)
{
	if(psDioInfo->asPortInfo[bPort].sDioCommon.bMasterSlave == MCDRV_DIO_SLAVE
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
	{
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bMasterSlave	= psDioInfo->asPortInfo[bPort].sDioCommon.bMasterSlave;
	}
	if(psDioInfo->asPortInfo[bPort].sDioCommon.bAutoFs == MCDRV_AUTOFS_OFF
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bAutoFs == MCDRV_AUTOFS_ON)
	{
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bAutoFs	= psDioInfo->asPortInfo[bPort].sDioCommon.bAutoFs;
	}
	if(psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_48000
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_44100
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_32000
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_24000
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_22050
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_16000
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_12000
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_11025
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_8000)
	{
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bFs	= psDioInfo->asPortInfo[bPort].sDioCommon.bFs;
	}
	if(psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_64
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_48
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_32
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_512
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_256
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_128
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_16)
	{
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bBckFs	= psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs;
	}
	if(psDioInfo->asPortInfo[bPort].sDioCommon.bInterface == MCDRV_DIO_DA
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bInterface == MCDRV_DIO_PCM)
	{
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bInterface	= psDioInfo->asPortInfo[bPort].sDioCommon.bInterface;
	}
	if(psDioInfo->asPortInfo[bPort].sDioCommon.bBckInvert == MCDRV_BCLK_NORMAL
	|| psDioInfo->asPortInfo[bPort].sDioCommon.bBckInvert == MCDRV_BCLK_INVERT)
	{
		gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bBckInvert	= psDioInfo->asPortInfo[bPort].sDioCommon.bBckInvert;
	}
	if(psDioInfo->asPortInfo[bPort].sDioCommon.bInterface == MCDRV_DIO_PCM)
	{
		if(psDioInfo->asPortInfo[bPort].sDioCommon.bPcmHizTim == MCDRV_PCMHIZTIM_FALLING
		|| psDioInfo->asPortInfo[bPort].sDioCommon.bPcmHizTim == MCDRV_PCMHIZTIM_RISING)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bPcmHizTim	= psDioInfo->asPortInfo[bPort].sDioCommon.bPcmHizTim;
		}
		if(psDioInfo->asPortInfo[bPort].sDioCommon.bPcmClkDown == MCDRV_PCM_CLKDOWN_OFF
		|| psDioInfo->asPortInfo[bPort].sDioCommon.bPcmClkDown == MCDRV_PCM_CLKDOWN_HALF)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bPcmClkDown	= psDioInfo->asPortInfo[bPort].sDioCommon.bPcmClkDown;
		}
		if(psDioInfo->asPortInfo[bPort].sDioCommon.bPcmFrame == MCDRV_PCM_SHORTFRAME
		|| psDioInfo->asPortInfo[bPort].sDioCommon.bPcmFrame == MCDRV_PCM_LONGFRAME)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bPcmFrame	= psDioInfo->asPortInfo[bPort].sDioCommon.bPcmFrame;
		}
		if(psDioInfo->asPortInfo[bPort].sDioCommon.bPcmHighPeriod <= 31)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bPcmHighPeriod	= psDioInfo->asPortInfo[bPort].sDioCommon.bPcmHighPeriod;
		}
	}
}

/****************************************************************************
 *	SetDIODIR
 *
 *	Description:
 *			Set digital io dir information.
 *	Arguments:
 *			psDioInfo	digital io information
 *			bPort		port number
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetDIODIR
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT8	bPort
)
{
	UINT8	bDIOCh;

	gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDir.wSrcRate	= psDioInfo->asPortInfo[bPort].sDir.wSrcRate;
	if(gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bInterface == MCDRV_DIO_DA)
	{
		if(psDioInfo->asPortInfo[bPort].sDir.sDaFormat.bBitSel == MCDRV_BITSEL_16
		|| psDioInfo->asPortInfo[bPort].sDir.sDaFormat.bBitSel == MCDRV_BITSEL_20
		|| psDioInfo->asPortInfo[bPort].sDir.sDaFormat.bBitSel == MCDRV_BITSEL_24)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDir.sDaFormat.bBitSel	= psDioInfo->asPortInfo[bPort].sDir.sDaFormat.bBitSel;
		}
		if(psDioInfo->asPortInfo[bPort].sDir.sDaFormat.bMode == MCDRV_DAMODE_HEADALIGN
		|| psDioInfo->asPortInfo[bPort].sDir.sDaFormat.bMode == MCDRV_DAMODE_I2S
		|| psDioInfo->asPortInfo[bPort].sDir.sDaFormat.bMode == MCDRV_DAMODE_TAILALIGN)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDir.sDaFormat.bMode	= psDioInfo->asPortInfo[bPort].sDir.sDaFormat.bMode;
		}
	}
	else if(gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bInterface == MCDRV_DIO_PCM)
	{
		if(psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bMono == MCDRV_PCM_STEREO
		|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bMono == MCDRV_PCM_MONO)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDir.sPcmFormat.bMono	= psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bMono;
		}
		if(psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bOrder == MCDRV_PCM_MSB_FIRST
		|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bOrder == MCDRV_PCM_LSB_FIRST
		|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bOrder == MCDRV_PCM_MSB_FIRST_SIGN
		|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bOrder == MCDRV_PCM_LSB_FIRST_SIGN
		|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bOrder == MCDRV_PCM_MSB_FIRST_ZERO
		|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bOrder == MCDRV_PCM_LSB_FIRST_ZERO)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDir.sPcmFormat.bOrder	= psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bOrder;
		}
		if(psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bLaw == MCDRV_PCM_LINEAR
		|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bLaw == MCDRV_PCM_ALAW
		|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bLaw == MCDRV_PCM_MULAW)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDir.sPcmFormat.bLaw	= psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bLaw;
		}
		if(psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_8
		|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_13
		|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_14
		|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_16)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDir.sPcmFormat.bBitSel	= psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bBitSel;
		}
	}
	for(bDIOCh = 0; bDIOCh < DIO_CHANNELS; bDIOCh++)
	{
		if(psDioInfo->asPortInfo[bPort].sDir.abSlot[bDIOCh] < 2)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDir.abSlot[bDIOCh]	= psDioInfo->asPortInfo[bPort].sDir.abSlot[bDIOCh];
		}
	}
}

/****************************************************************************
 *	SetDIODIT
 *
 *	Description:
 *			Set digital io dit information.
 *	Arguments:
 *			psDioInfo	digital io information
 *			bPort		port number
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	SetDIODIT
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT8	bPort
)
{
	UINT8	bDIOCh;

	gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDit.wSrcRate	= psDioInfo->asPortInfo[bPort].sDit.wSrcRate;
	if(gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bInterface == MCDRV_DIO_DA)
	{
		if(psDioInfo->asPortInfo[bPort].sDit.sDaFormat.bBitSel == MCDRV_BITSEL_16
		|| psDioInfo->asPortInfo[bPort].sDit.sDaFormat.bBitSel == MCDRV_BITSEL_20
		|| psDioInfo->asPortInfo[bPort].sDit.sDaFormat.bBitSel == MCDRV_BITSEL_24)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDit.sDaFormat.bBitSel	= psDioInfo->asPortInfo[bPort].sDit.sDaFormat.bBitSel;
		}
		if(psDioInfo->asPortInfo[bPort].sDit.sDaFormat.bMode == MCDRV_DAMODE_HEADALIGN
		|| psDioInfo->asPortInfo[bPort].sDit.sDaFormat.bMode == MCDRV_DAMODE_I2S
		|| psDioInfo->asPortInfo[bPort].sDit.sDaFormat.bMode == MCDRV_DAMODE_TAILALIGN)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDit.sDaFormat.bMode	= psDioInfo->asPortInfo[bPort].sDit.sDaFormat.bMode;
		}
	}
	else if(gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDioCommon.bInterface == MCDRV_DIO_PCM)
	{
		if(psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bMono == MCDRV_PCM_STEREO
		|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bMono == MCDRV_PCM_MONO)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDit.sPcmFormat.bMono	= psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bMono;
		}
		if(psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bOrder == MCDRV_PCM_MSB_FIRST
		|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bOrder == MCDRV_PCM_LSB_FIRST
		|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bOrder == MCDRV_PCM_MSB_FIRST_SIGN
		|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bOrder == MCDRV_PCM_LSB_FIRST_SIGN
		|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bOrder == MCDRV_PCM_MSB_FIRST_ZERO
		|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bOrder == MCDRV_PCM_LSB_FIRST_ZERO)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDit.sPcmFormat.bOrder	= psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bOrder;
		}
		if(psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bLaw == MCDRV_PCM_LINEAR
		|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bLaw == MCDRV_PCM_ALAW
		|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bLaw == MCDRV_PCM_MULAW)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDit.sPcmFormat.bLaw	= psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bLaw;
		}
		if(psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_8
		|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_13
		|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_14
		|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_16)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDit.sPcmFormat.bBitSel	= psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bBitSel;
		}
	}
	for(bDIOCh = 0; bDIOCh < DIO_CHANNELS; bDIOCh++)
	{
		if(psDioInfo->asPortInfo[bPort].sDit.abSlot[bDIOCh] < 2)
		{
			gsGlobalInfo_AA.sDioInfo.asPortInfo[bPort].sDit.abSlot[bDIOCh]	= psDioInfo->asPortInfo[bPort].sDit.abSlot[bDIOCh];
		}
	}
}

/****************************************************************************
 *	McResCtrl_GetDioInfo_AA
 *
 *	Description:
 *			Get digital io information.
 *	Arguments:
 *			psDioInfo	digital io information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetDioInfo_AA
(
	MCDRV_DIO_INFO*	psDioInfo
)
{
	*psDioInfo	= gsGlobalInfo_AA.sDioInfo;
}

/****************************************************************************
 *	McResCtrl_SetVolInfo_AA
 *
 *	Description:
 *			Update volume.
 *	Arguments:
 *			psVolInfo		volume setting
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetVolInfo_AA
(
	const MCDRV_VOL_INFO*	psVolInfo
)
{
	UINT8	bCh;


	for(bCh = 0; bCh < AD0_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswD_Ad0[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Ad0[bCh]	= (psVolInfo->aswD_Ad0[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswD_Ad0Att[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Ad0Att[bCh]	= (psVolInfo->aswD_Ad0Att[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswA_Ad0[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Ad0[bCh]	= (psVolInfo->aswA_Ad0[bCh] & 0xFFFE);
		}
	}
	for(bCh = 0; bCh < AD1_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswD_Ad1[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Ad1[bCh]	= (psVolInfo->aswD_Ad1[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswD_Ad1Att[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Ad1Att[bCh]	= (psVolInfo->aswD_Ad1Att[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswA_Ad1[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Ad1[bCh]	= (psVolInfo->aswA_Ad1[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < AENG6_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswD_Aeng6[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Aeng6[bCh]	= (psVolInfo->aswD_Aeng6[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < PDM_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswD_Pdm[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Pdm[bCh]	= (psVolInfo->aswD_Pdm[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswD_SideTone[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_SideTone[bCh]	= (psVolInfo->aswD_SideTone[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < DTMF_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswD_Dtmfb[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Dtmfb[bCh]	= (psVolInfo->aswD_Dtmfb[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswD_DtmfAtt[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_DtmfAtt[bCh]	= (psVolInfo->aswD_DtmfAtt[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < DIO0_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswD_Dir0[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Dir0[bCh]	= (psVolInfo->aswD_Dir0[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswD_Dir0Att[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Dir0Att[bCh]	= (psVolInfo->aswD_Dir0Att[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswD_Dit0[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Dit0[bCh]	= (psVolInfo->aswD_Dit0[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < DIO1_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswD_Dir1[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Dir1[bCh]	= (psVolInfo->aswD_Dir1[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswD_Dir1Att[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Dir1Att[bCh]	= (psVolInfo->aswD_Dir1Att[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswD_Dit1[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Dit1[bCh]	= (psVolInfo->aswD_Dit1[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < DIO2_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswD_Dir2[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Dir2[bCh]	= (psVolInfo->aswD_Dir2[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswD_Dir2Att[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Dir2Att[bCh]	= (psVolInfo->aswD_Dir2Att[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswD_Dit2[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_Dit2[bCh]	= (psVolInfo->aswD_Dit2[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < DAC_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswD_DacMaster[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_DacMaster[bCh]	= (psVolInfo->aswD_DacMaster[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswD_DacVoice[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_DacVoice[bCh]	= (psVolInfo->aswD_DacVoice[bCh] & 0xFFFE);
		}
		if((psVolInfo->aswD_DacAtt[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswD_DacAtt[bCh]	= (psVolInfo->aswD_DacAtt[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < LIN1_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Lin1[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Lin1[bCh]	= (psVolInfo->aswA_Lin1[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < LIN2_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Lin2[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Lin2[bCh]	= (psVolInfo->aswA_Lin2[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < MIC1_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Mic1[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Mic1[bCh]	= (psVolInfo->aswA_Mic1[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < MIC2_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Mic2[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Mic2[bCh]	= (psVolInfo->aswA_Mic2[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < MIC3_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Mic3[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Mic3[bCh]	= (psVolInfo->aswA_Mic3[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < HP_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Hp[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Hp[bCh]	= (psVolInfo->aswA_Hp[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < SP_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Sp[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Sp[bCh]	= (psVolInfo->aswA_Sp[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < RC_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Rc[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Rc[bCh]	= (psVolInfo->aswA_Rc[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < LOUT1_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Lout1[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Lout1[bCh]	= (psVolInfo->aswA_Lout1[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < LOUT2_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Lout2[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Lout2[bCh]	= (psVolInfo->aswA_Lout2[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < MIC1_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Mic1Gain[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Mic1Gain[bCh]	= (psVolInfo->aswA_Mic1Gain[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < MIC2_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Mic2Gain[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Mic2Gain[bCh]	= (psVolInfo->aswA_Mic2Gain[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < MIC3_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_Mic3Gain[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_Mic3Gain[bCh]	= (psVolInfo->aswA_Mic3Gain[bCh] & 0xFFFE);
		}
	}

	for(bCh = 0; bCh < HPGAIN_VOL_CHANNELS; bCh++)
	{
		if((psVolInfo->aswA_HpGain[bCh] & 0x01) != 0)
		{
			gsGlobalInfo_AA.sVolInfo.aswA_HpGain[bCh]	= (psVolInfo->aswA_HpGain[bCh] & 0xFFFE);
		}
	}
}

/****************************************************************************
 *	McResCtrl_GetVolInfo_AA
 *
 *	Description:
 *			Get volume setting.
 *	Arguments:
 *			psVolInfo		volume setting
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetVolInfo_AA
(
	MCDRV_VOL_INFO*	psVolInfo
)
{
	*psVolInfo	= gsGlobalInfo_AA.sVolInfo;
}

/****************************************************************************
 *	McResCtrl_SetDacInfo_AA
 *
 *	Description:
 *			Set DAC information.
 *	Arguments:
 *			psDacInfo	DAC information
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetDacInfo_AA
(
	const MCDRV_DAC_INFO*	psDacInfo,
	UINT32	dUpdateInfo
)
{

	if((dUpdateInfo & MCDRV_DAC_MSWP_UPDATE_FLAG) != 0)
	{
		switch(psDacInfo->bMasterSwap)
		{
		case	MCDRV_DSWAP_OFF:
		case	MCDRV_DSWAP_SWAP:
		case	MCDRV_DSWAP_MUTE:
		case	MCDRV_DSWAP_RMVCENTER:
		case	MCDRV_DSWAP_MONO:
		case	MCDRV_DSWAP_MONOHALF:
		case	MCDRV_DSWAP_BOTHL:
		case	MCDRV_DSWAP_BOTHR:
			gsGlobalInfo_AA.sDacInfo.bMasterSwap	= psDacInfo->bMasterSwap;
			break;
		default:
			break;
		}
	}
	if((dUpdateInfo & MCDRV_DAC_VSWP_UPDATE_FLAG) != 0)
	{
		switch(psDacInfo->bVoiceSwap)
		{
		case	MCDRV_DSWAP_OFF:
		case	MCDRV_DSWAP_SWAP:
		case	MCDRV_DSWAP_MUTE:
		case	MCDRV_DSWAP_RMVCENTER:
		case	MCDRV_DSWAP_MONO:
		case	MCDRV_DSWAP_MONOHALF:
		case	MCDRV_DSWAP_BOTHL:
		case	MCDRV_DSWAP_BOTHR:
			gsGlobalInfo_AA.sDacInfo.bVoiceSwap	= psDacInfo->bVoiceSwap;
			break;
		default:
			break;
		}
	}
	if((dUpdateInfo & MCDRV_DAC_HPF_UPDATE_FLAG) != 0)
	{
		if(psDacInfo->bDcCut == MCDRV_DCCUT_ON || psDacInfo->bDcCut == MCDRV_DCCUT_OFF)
		{
			gsGlobalInfo_AA.sDacInfo.bDcCut	= psDacInfo->bDcCut;
		}
	}
}

/****************************************************************************
 *	McResCtrl_GetDacInfo_AA
 *
 *	Description:
 *			Get DAC information.
 *	Arguments:
 *			psDacInfo	DAC information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetDacInfo_AA
(
	MCDRV_DAC_INFO*	psDacInfo
)
{
	*psDacInfo	= gsGlobalInfo_AA.sDacInfo;
}

/****************************************************************************
 *	McResCtrl_SetAdcInfo_AA
 *
 *	Description:
 *			Set ADC information.
 *	Arguments:
 *			psAdcInfo	ADC information
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetAdcInfo_AA
(
	const MCDRV_ADC_INFO*	psAdcInfo,
	UINT32	dUpdateInfo
)
{

	if((dUpdateInfo & MCDRV_ADCADJ_UPDATE_FLAG) != 0)
	{
		switch(psAdcInfo->bAgcAdjust)
		{
		case	MCDRV_AGCADJ_24:
		case	MCDRV_AGCADJ_18:
		case	MCDRV_AGCADJ_12:
		case	MCDRV_AGCADJ_0:
			gsGlobalInfo_AA.sAdcInfo.bAgcAdjust	= psAdcInfo->bAgcAdjust;
			break;
		default:
			break;
		}
	}
	if((dUpdateInfo & MCDRV_ADCAGC_UPDATE_FLAG) != 0)
	{
		if(psAdcInfo->bAgcOn == MCDRV_AGC_OFF || psAdcInfo->bAgcOn == MCDRV_AGC_ON)
		{
			gsGlobalInfo_AA.sAdcInfo.bAgcOn	= psAdcInfo->bAgcOn;
		}
	}
	if((dUpdateInfo & MCDRV_ADCMONO_UPDATE_FLAG) != 0)
	{
		if(psAdcInfo->bMono == MCDRV_ADC_STEREO || psAdcInfo->bMono == MCDRV_ADC_MONO)
		{
			gsGlobalInfo_AA.sAdcInfo.bMono	= psAdcInfo->bMono;
		}
	}
}

/****************************************************************************
 *	McResCtrl_GetAdcInfo_AA
 *
 *	Description:
 *			Get ADC information.
 *	Arguments:
 *			psAdcInfo	ADC information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetAdcInfo_AA
(
	MCDRV_ADC_INFO*	psAdcInfo
)
{
	*psAdcInfo	= gsGlobalInfo_AA.sAdcInfo;
}

/****************************************************************************
 *	McResCtrl_SetSpInfo_AA
 *
 *	Description:
 *			Set SP information.
 *	Arguments:
 *			psSpInfo	SP information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetSpInfo_AA
(
	const MCDRV_SP_INFO*	psSpInfo
)
{

	if(psSpInfo->bSwap == MCDRV_SPSWAP_OFF || psSpInfo->bSwap == MCDRV_SPSWAP_SWAP)
	{
		gsGlobalInfo_AA.sSpInfo.bSwap	= psSpInfo->bSwap;
	}
}

/****************************************************************************
 *	McResCtrl_GetSpInfo_AA
 *
 *	Description:
 *			Get SP information.
 *	Arguments:
 *			psSpInfo	SP information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetSpInfo_AA
(
	MCDRV_SP_INFO*	psSpInfo
)
{
	*psSpInfo	= gsGlobalInfo_AA.sSpInfo;
}

/****************************************************************************
 *	McResCtrl_SetDngInfo_AA
 *
 *	Description:
 *			Set Digital Noise Gate information.
 *	Arguments:
 *			psDngInfo	DNG information
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetDngInfo_AA
(
	const MCDRV_DNG_INFO*	psDngInfo,
	UINT32	dUpdateInfo
)
{

	if((dUpdateInfo & MCDRV_DNGSW_HP_UPDATE_FLAG) != 0)
	{
		if(psDngInfo->abOnOff[0] == MCDRV_DNG_OFF || psDngInfo->abOnOff[0] == MCDRV_DNG_ON)
		{
			gsGlobalInfo_AA.sDngInfo.abOnOff[0]	= psDngInfo->abOnOff[0];
		}
	}
	if((dUpdateInfo & MCDRV_DNGTHRES_HP_UPDATE_FLAG) != 0)
	{
		if(psDngInfo->abThreshold[0] <= MCDRV_DNG_THRES_72)
		{
			gsGlobalInfo_AA.sDngInfo.abThreshold[0]	= psDngInfo->abThreshold[0];
		}
	}
	if((dUpdateInfo & MCDRV_DNGHOLD_HP_UPDATE_FLAG) != 0)
	{
		switch(psDngInfo->abHold[0])
		{
		case	MCDRV_DNG_HOLD_30:
		case	MCDRV_DNG_HOLD_120:
		case	MCDRV_DNG_HOLD_500:
			gsGlobalInfo_AA.sDngInfo.abHold[0]	= psDngInfo->abHold[0];
			break;
		default:
			break;
		}
	}
	if((dUpdateInfo & MCDRV_DNGATK_HP_UPDATE_FLAG) != 0)
	{
		if(psDngInfo->abAttack[0] == MCDRV_DNG_ATTACK_25)
		{
			gsGlobalInfo_AA.sDngInfo.abAttack[0]	= psDngInfo->abAttack[0];
		}
		else if(psDngInfo->abAttack[0] == MCDRV_DNG_ATTACK_800)
		{
			gsGlobalInfo_AA.sDngInfo.abAttack[0]	= 1/*MCDRV_DNG_ATTACK_800*/;
		}
	}
	if((dUpdateInfo & MCDRV_DNGREL_HP_UPDATE_FLAG) != 0)
	{
		switch(psDngInfo->abRelease[0])
		{
		case	MCDRV_DNG_RELEASE_7950:
		case	MCDRV_DNG_RELEASE_470:
		case	MCDRV_DNG_RELEASE_940:
			gsGlobalInfo_AA.sDngInfo.abRelease[0]	= psDngInfo->abRelease[0];
			break;
		default:
			break;
		}
	}
}

/****************************************************************************
 *	McResCtrl_GetDngInfo_AA
 *
 *	Description:
 *			Get Digital Noise Gate information.
 *	Arguments:
 *			psDngInfo	DNG information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetDngInfo_AA
(
	MCDRV_DNG_INFO*	psDngInfo
)
{
	*psDngInfo	= gsGlobalInfo_AA.sDngInfo;
}

/****************************************************************************
 *	McResCtrl_SetAeInfo_AA
 *
 *	Description:
 *			Set Audio Engine information.
 *	Arguments:
 *			psAeInfo	AE information
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetAeInfo_AA
(
	const MCDRV_AE_INFO*	psAeInfo,
	UINT32	dUpdateInfo
)
{

	if(McDevProf_IsValid(eMCDRV_FUNC_DBEX) == 1
	&& (dUpdateInfo & MCDRV_AEUPDATE_FLAG_BEXWIDE_ONOFF) != 0)
	{
		if((psAeInfo->bOnOff & MCDRV_BEXWIDE_ON) != 0)
		{
			gsGlobalInfo_AA.sAeInfo.bOnOff	|= MCDRV_BEXWIDE_ON;
		}
		else
		{
			gsGlobalInfo_AA.sAeInfo.bOnOff &= (UINT8)~MCDRV_BEXWIDE_ON;
		}
	}
	if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_DRC_ONOFF) != 0)
	{
		if((psAeInfo->bOnOff & MCDRV_DRC_ON) != 0)
		{
			gsGlobalInfo_AA.sAeInfo.bOnOff	|= MCDRV_DRC_ON;
		}
		else
		{
			gsGlobalInfo_AA.sAeInfo.bOnOff &= (UINT8)~MCDRV_DRC_ON;
		}
	}
	if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ5_ONOFF) != 0)
	{
		if((psAeInfo->bOnOff & MCDRV_EQ5_ON) != 0)
		{
			gsGlobalInfo_AA.sAeInfo.bOnOff	|= MCDRV_EQ5_ON;
		}
		else
		{
			gsGlobalInfo_AA.sAeInfo.bOnOff &= (UINT8)~MCDRV_EQ5_ON;
		}
	}
	if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ3_ONOFF) != 0)
	{
		if((psAeInfo->bOnOff & MCDRV_EQ3_ON) != 0)
		{
			gsGlobalInfo_AA.sAeInfo.bOnOff	|= MCDRV_EQ3_ON;
		}
		else
		{
			gsGlobalInfo_AA.sAeInfo.bOnOff &= (UINT8)~MCDRV_EQ3_ON;
		}
	}

	if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_BEX) != 0)
	{
		McSrv_MemCopy(psAeInfo->abBex, gsGlobalInfo_AA.sAeInfo.abBex, BEX_PARAM_SIZE);
	}

	if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_WIDE) != 0)
	{
		McSrv_MemCopy(psAeInfo->abWide, gsGlobalInfo_AA.sAeInfo.abWide, WIDE_PARAM_SIZE);
	}

	if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_DRC) != 0)
	{
		McSrv_MemCopy(psAeInfo->abDrc, gsGlobalInfo_AA.sAeInfo.abDrc, DRC_PARAM_SIZE);
	}

	if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ5) != 0)
	{
		McSrv_MemCopy(psAeInfo->abEq5, gsGlobalInfo_AA.sAeInfo.abEq5, EQ5_PARAM_SIZE);
	}

	if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ3) != 0)
	{
		McSrv_MemCopy(psAeInfo->abEq3, gsGlobalInfo_AA.sAeInfo.abEq3, EQ3_PARAM_SIZE);
	}

	if(McDevProf_IsValid(eMCDRV_FUNC_HWADJ) == 1)
	{
		gsGlobalInfo_AA.sAeInfo.bOnOff |= MCDRV_EQ3_ON;
	}
}

/****************************************************************************
 *	McResCtrl_GetAeInfo_AA
 *
 *	Description:
 *			Get Audio Engine information.
 *	Arguments:
 *			psAeInfo	AE information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetAeInfo_AA
(
	MCDRV_AE_INFO*	psAeInfo
)
{
	*psAeInfo	= gsGlobalInfo_AA.sAeInfo;
}

/****************************************************************************
 *	McResCtrl_SetPdmInfo_AA
 *
 *	Description:
 *			Set PDM information.
 *	Arguments:
 *			psPdmInfo	PDM information
 *			dUpdateInfo	update information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetPdmInfo_AA
(
	const MCDRV_PDM_INFO*	psPdmInfo,
	UINT32	dUpdateInfo
)
{

	if((dUpdateInfo & MCDRV_PDMCLK_UPDATE_FLAG) != 0)
	{
		switch(psPdmInfo->bClk)
		{
		case	MCDRV_PDM_CLK_128:
		case	MCDRV_PDM_CLK_64:
		case	MCDRV_PDM_CLK_32:
			gsGlobalInfo_AA.sPdmInfo.bClk	= psPdmInfo->bClk;
			break;
		default:
			break;
		}
	}
	if((dUpdateInfo & MCDRV_PDMADJ_UPDATE_FLAG) != 0)
	{
		switch(psPdmInfo->bAgcAdjust)
		{
		case	MCDRV_AGCADJ_24:
		case	MCDRV_AGCADJ_18:
		case	MCDRV_AGCADJ_12:
		case	MCDRV_AGCADJ_0:
			gsGlobalInfo_AA.sPdmInfo.bAgcAdjust	= psPdmInfo->bAgcAdjust;
			break;
		default:
			break;
		}
	}
	if((dUpdateInfo & MCDRV_PDMAGC_UPDATE_FLAG) != 0)
	{
		switch(psPdmInfo->bAgcOn)
		{
		case	MCDRV_AGC_OFF:
		case	MCDRV_AGC_ON:
			gsGlobalInfo_AA.sPdmInfo.bAgcOn	= psPdmInfo->bAgcOn;
			break;
		default:
			break;
		}
	}
	if((dUpdateInfo & MCDRV_PDMEDGE_UPDATE_FLAG) != 0)
	{
		switch(psPdmInfo->bPdmEdge)
		{
		case	MCDRV_PDMEDGE_LH:
		case	MCDRV_PDMEDGE_HL:
			gsGlobalInfo_AA.sPdmInfo.bPdmEdge	= psPdmInfo->bPdmEdge;
			break;
		default:
			break;
		}
	}
	if((dUpdateInfo & MCDRV_PDMWAIT_UPDATE_FLAG) != 0)
	{
		switch(psPdmInfo->bPdmWait)
		{
		case	MCDRV_PDMWAIT_0:
		case	MCDRV_PDMWAIT_1:
		case	MCDRV_PDMWAIT_10:
		case	MCDRV_PDMWAIT_20:
			gsGlobalInfo_AA.sPdmInfo.bPdmWait	= psPdmInfo->bPdmWait;
			break;
		default:
			break;
		}
	}
	if((dUpdateInfo & MCDRV_PDMSEL_UPDATE_FLAG) != 0)
	{
		switch(psPdmInfo->bPdmSel)
		{
		case	MCDRV_PDMSEL_L1R2:
		case	MCDRV_PDMSEL_L2R1:
		case	MCDRV_PDMSEL_L1R1:
		case	MCDRV_PDMSEL_L2R2:
			gsGlobalInfo_AA.sPdmInfo.bPdmSel	= psPdmInfo->bPdmSel;
			break;
		default:
			break;
		}
	}
	if((dUpdateInfo & MCDRV_PDMMONO_UPDATE_FLAG) != 0)
	{
		switch(psPdmInfo->bMono)
		{
		case	MCDRV_PDM_STEREO:
		case	MCDRV_PDM_MONO:
			gsGlobalInfo_AA.sPdmInfo.bMono	= psPdmInfo->bMono;
			break;
		default:
			break;
		}
	}
}

/****************************************************************************
 *	McResCtrl_GetPdmInfo_AA
 *
 *	Description:
 *			Get PDM information.
 *	Arguments:
 *			psPdmInfo	PDM information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetPdmInfo_AA
(
	MCDRV_PDM_INFO*	psPdmInfo
)
{
	*psPdmInfo	= gsGlobalInfo_AA.sPdmInfo;
}

/****************************************************************************
 *	McResCtrl_SetGPMode_AA
 *
 *	Description:
 *			Set GP mode.
 *	Arguments:
 *			psGpMode	GP mode
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetGPMode_AA
(
	const MCDRV_GP_MODE*	psGpMode
)
{
	UINT8	bPad;


	for(bPad = 0; bPad < GPIO_PAD_NUM; bPad++)
	{
		if(psGpMode->abGpDdr[bPad] == MCDRV_GPDDR_IN
		|| psGpMode->abGpDdr[bPad] == MCDRV_GPDDR_OUT)
		{
			gsGlobalInfo_AA.sGpMode.abGpDdr[bPad]	= psGpMode->abGpDdr[bPad];
		}
		if(McDevProf_IsValid(eMCDRV_FUNC_GPMODE) == 1)
		{
			if(psGpMode->abGpMode[bPad] == MCDRV_GPMODE_RISING
			|| psGpMode->abGpMode[bPad] == MCDRV_GPMODE_FALLING
			|| psGpMode->abGpMode[bPad] == MCDRV_GPMODE_BOTH)
			{
				gsGlobalInfo_AA.sGpMode.abGpMode[bPad]	= psGpMode->abGpMode[bPad];
			}
			if(psGpMode->abGpHost[bPad] == MCDRV_GPHOST_SCU
			|| psGpMode->abGpHost[bPad] == MCDRV_GPHOST_CDSP)
			{
				gsGlobalInfo_AA.sGpMode.abGpHost[bPad]	= psGpMode->abGpHost[bPad];
			}
			if(psGpMode->abGpInvert[bPad] == MCDRV_GPINV_NORMAL
			|| psGpMode->abGpInvert[bPad] == MCDRV_GPINV_INVERT)
			{
				gsGlobalInfo_AA.sGpMode.abGpInvert[bPad]	= psGpMode->abGpInvert[bPad];
			}
		}
	}
}

/****************************************************************************
 *	McResCtrl_GetGPMode_AA
 *
 *	Description:
 *			Get GP mode.
 *	Arguments:
 *			psGpMode	GP mode
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetGPMode_AA
(
	MCDRV_GP_MODE*	psGpMode
)
{
	*psGpMode	= gsGlobalInfo_AA.sGpMode;
}

/****************************************************************************
 *	McResCtrl_SetGPMask_AA
 *
 *	Description:
 *			Set GP mask.
 *	Arguments:
 *			bMask	GP mask
 *			dPadNo	PAD Number
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_SetGPMask_AA
(
	UINT8	bMask,
	UINT32	dPadNo
)
{
	if(dPadNo == MCDRV_GP_PAD0)
	{
		if(gsGlobalInfo_AA.sInitInfo.bPad0Func == MCDRV_PAD_GPIO
		&& gsGlobalInfo_AA.sGpMode.abGpDdr[dPadNo] == MCDRV_GPDDR_IN)
		{
			if(bMask == MCDRV_GPMASK_ON || bMask == MCDRV_GPMASK_OFF)
			{
				gsGlobalInfo_AA.abGpMask[dPadNo]	= bMask;
			}
		}
	}
	else if(dPadNo == MCDRV_GP_PAD1)
	{
		if(gsGlobalInfo_AA.sInitInfo.bPad1Func == MCDRV_PAD_GPIO
		&& gsGlobalInfo_AA.sGpMode.abGpDdr[dPadNo] == MCDRV_GPDDR_IN)
		{
			if(bMask == MCDRV_GPMASK_ON || bMask == MCDRV_GPMASK_OFF)
			{
				gsGlobalInfo_AA.abGpMask[dPadNo]	= bMask;
			}
		}
	}
/*
	else if(dPadNo == MCDRV_GP_PAD2)
	{
		if(gsGlobalInfo_AA.sInitInfo.bPad2Func == MCDRV_PAD_GPIO
		&& gsGlobalInfo_AA.sGpMode.abGpDdr[dPadNo] == MCDRV_GPDDR_IN)
		{
			if(bMask == MCDRV_GPMASK_ON || bMask == MCDRV_GPMASK_OFF)
			{
				gsGlobalInfo_AA.abGpMask[dPadNo]	= bMask;
			}
		}
	}
*/
}

/****************************************************************************
 *	McResCtrl_GetGPMask_AA
 *
 *	Description:
 *			Get GP mask.
 *	Arguments:
 *			pabMask	GP mask
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetGPMask_AA
(
	UINT8*	pabMask
)
{
	UINT8	bPadNo;
	for(bPadNo = 0; bPadNo < GPIO_PAD_NUM; bPadNo++)
	{
		pabMask[bPadNo]	= gsGlobalInfo_AA.abGpMask[bPadNo];
	}
}


/****************************************************************************
 *	McResCtrl_GetVolReg_AA
 *
 *	Description:
 *			Get value of volume registers.
 *	Arguments:
 *			psVolInfo	volume information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetVolReg_AA
(
	MCDRV_VOL_INFO*	psVolInfo
)
{
	UINT8	bCh;
	MCDRV_DST_CH	abDSTCh[]	= {eMCDRV_DST_CH0_AA, eMCDRV_DST_CH1_AA};
	SINT16	swGainUp;


	*psVolInfo	= gsGlobalInfo_AA.sVolInfo;

	if(gsGlobalInfo_AA.sInitInfo.bDclGain == MCDRV_DCLGAIN_6)
	{
		swGainUp	= 6 * 256;
	}
	else if(gsGlobalInfo_AA.sInitInfo.bDclGain == MCDRV_DCLGAIN_12)
	{
		swGainUp	= 12 * 256;
	}
	else if(gsGlobalInfo_AA.sInitInfo.bDclGain == MCDRV_DCLGAIN_18)
	{
		swGainUp	= 18 * 256;
	}
	else
	{
		swGainUp	= 0;
	}

	psVolInfo->aswA_HpGain[0]	= MCDRV_REG_MUTE;
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_HP_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		psVolInfo->aswA_Hp[0]	= MCDRV_REG_MUTE;
	}
	else
	{
		psVolInfo->aswA_Hp[0]		= GetHpVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Hp[0]);
		psVolInfo->aswA_HpGain[0]	= GetHpGainReg(gsGlobalInfo_AA.sVolInfo.aswA_HpGain[0]);
	}
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_HP_AA, eMCDRV_DST_CH1_AA) == 0)
	{
		psVolInfo->aswA_Hp[1]	= MCDRV_REG_MUTE;
	}
	else
	{
		psVolInfo->aswA_Hp[1]	= GetHpVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Hp[1]);
		psVolInfo->aswA_HpGain[0]	= GetHpGainReg(gsGlobalInfo_AA.sVolInfo.aswA_HpGain[0]);
	}

	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_SP_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		psVolInfo->aswA_Sp[0]	= MCDRV_REG_MUTE;
	}
	else
	{
		psVolInfo->aswA_Sp[0]	= GetSpVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Sp[0]);
	}
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_SP_AA, eMCDRV_DST_CH1_AA) == 0)
	{
		psVolInfo->aswA_Sp[1]	= MCDRV_REG_MUTE;
	}
	else
	{
		psVolInfo->aswA_Sp[1]	= GetSpVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Sp[1]);
	}

	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_RCV_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		psVolInfo->aswA_Rc[0]	= MCDRV_REG_MUTE;
	}
	else
	{
		psVolInfo->aswA_Rc[0]	= GetRcVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Rc[0]);
	}

	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_LOUT1_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		psVolInfo->aswA_Lout1[0]	= MCDRV_REG_MUTE;
	}
	else
	{
		psVolInfo->aswA_Lout1[0]	= GetLoVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Lout1[0]);
	}
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_LOUT1_AA, eMCDRV_DST_CH1_AA) == 0)
	{
		psVolInfo->aswA_Lout1[1]	= MCDRV_REG_MUTE;
	}
	else
	{
		psVolInfo->aswA_Lout1[1]	= GetLoVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Lout1[1]);
	}

	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_LOUT2_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		psVolInfo->aswA_Lout2[0]	= MCDRV_REG_MUTE;
	}
	else
	{
		psVolInfo->aswA_Lout2[0]	= GetLoVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Lout2[0]);
	}
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_LOUT2_AA, eMCDRV_DST_CH1_AA) == 0)
	{
		psVolInfo->aswA_Lout2[1]	= MCDRV_REG_MUTE;
	}
	else
	{
		psVolInfo->aswA_Lout2[1]	= GetLoVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Lout2[1]);
	}

	for(bCh = 0; bCh < AD0_VOL_CHANNELS; bCh++)
	{
		if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_ADC0_AA, abDSTCh[bCh]) == 0)
		{/*	ADC0 source all off	*/
			psVolInfo->aswA_Ad0[bCh]	= MCDRV_REG_MUTE;
			psVolInfo->aswD_Ad0[bCh]	= MCDRV_REG_MUTE;
		}
		else
		{
			psVolInfo->aswA_Ad0[bCh]	= GetADVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Ad0[bCh]);
			psVolInfo->aswD_Ad0[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Ad0[bCh] - swGainUp);
		}
	}
	if((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
	|| (gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
	|| ((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON
		&& (McResCtrl_GetAESource_AA() == eMCDRV_SRC_PDM_AA || McResCtrl_GetAESource_AA() == eMCDRV_SRC_ADC0_AA)))
	{
		for(bCh = 0; bCh < AD0_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Ad0Att[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Ad0Att[bCh]);
		}
	}
	else
	{
		for(bCh = 0; bCh < AD0_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Ad0Att[bCh]	= MCDRV_REG_MUTE;
		}
	}

	if(McDevProf_IsValid(eMCDRV_FUNC_ADC1) == 1)
	{
		if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_ADC1_AA, eMCDRV_DST_CH0_AA) == 0)
		{/*	ADC1 source all off	*/
			for(bCh = 0; bCh < AD1_VOL_CHANNELS; bCh++)
			{
				psVolInfo->aswA_Ad1[bCh]	= MCDRV_REG_MUTE;
				psVolInfo->aswD_Ad1[bCh]	= MCDRV_REG_MUTE;
			}
		}
		else
		{
			for(bCh = 0; bCh < AD1_VOL_CHANNELS; bCh++)
			{
				psVolInfo->aswA_Ad1[bCh]	= GetADVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Ad1[bCh]);
				psVolInfo->aswD_Ad1[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Ad1[bCh] - swGainUp);
			}
		}
		if((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC1_BLOCK] & MCDRV_SRC4_ADC1_ON) == MCDRV_SRC4_ADC1_ON)
		{
			for(bCh = 0; bCh < AD1_VOL_CHANNELS; bCh++)
			{
				psVolInfo->aswD_Ad1Att[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Ad1Att[bCh]);
			}
		}
		else
		{
			for(bCh = 0; bCh < AD1_VOL_CHANNELS; bCh++)
			{
				psVolInfo->aswD_Ad1Att[bCh]	= MCDRV_REG_MUTE;
			}
		}
	}

	if(((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON))
	{
		psVolInfo->aswA_Lin1[0]	= GetLIVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Lin1[0]);
	}
	else
	{
		psVolInfo->aswA_Lin1[0]	= MCDRV_REG_MUTE;
	}

	if(((gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON)
	|| ((gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON))
	{
		psVolInfo->aswA_Lin1[1]	= GetLIVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Lin1[1]);
	}
	else
	{
		psVolInfo->aswA_Lin1[1]	= MCDRV_REG_MUTE;
	}

	if(McDevProf_IsValid(eMCDRV_FUNC_LI2) == 1)
	{
		psVolInfo->aswA_Lin2[0]	= MCDRV_REG_MUTE;
		psVolInfo->aswA_Lin2[1]	= MCDRV_REG_MUTE;
	}

	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_MIC1_AA) == 0)
	{/*	MIC1 is unused	*/
		for(bCh = 0; bCh < MIC1_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswA_Mic1[bCh]		= MCDRV_REG_MUTE;
			psVolInfo->aswA_Mic1Gain[bCh]	= (SINT16)(McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_MC_GAIN) & MCB_AA_MC1GAIN);
		}
	}
	else
	{
		for(bCh = 0; bCh < MIC1_VOL_CHANNELS; bCh++)
		{
			if(((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) != MCDRV_SRC0_MIC1_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) != MCDRV_SRC0_MIC1_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) != MCDRV_SRC0_MIC1_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) != MCDRV_SRC0_MIC1_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) != MCDRV_SRC0_MIC1_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) != MCDRV_SRC0_MIC1_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) != MCDRV_SRC0_MIC1_ON))
			{
				psVolInfo->aswA_Mic1[bCh]	= MCDRV_REG_MUTE;
			}
			else
			{
				psVolInfo->aswA_Mic1[bCh]	= GetMcVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Mic1[bCh]);
			}
			psVolInfo->aswA_Mic1Gain[bCh]	= GetMcGainReg(gsGlobalInfo_AA.sVolInfo.aswA_Mic1Gain[bCh]);
		}
	}
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_MIC2_AA) == 0)
	{/*	MIC2 is unused	*/
		for(bCh = 0; bCh < MIC2_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswA_Mic2[bCh]		= MCDRV_REG_MUTE;
			psVolInfo->aswA_Mic2Gain[bCh]	= (SINT16)((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_MC_GAIN) & MCB_AA_MC2GAIN) >> 4);
		}
	}
	else
	{
		for(bCh = 0; bCh < MIC2_VOL_CHANNELS; bCh++)
		{
			if(((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) != MCDRV_SRC0_MIC2_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) != MCDRV_SRC0_MIC2_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) != MCDRV_SRC0_MIC2_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) != MCDRV_SRC0_MIC2_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) != MCDRV_SRC0_MIC2_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) != MCDRV_SRC0_MIC2_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) != MCDRV_SRC0_MIC2_ON))
			{
				psVolInfo->aswA_Mic2[bCh]	= MCDRV_REG_MUTE;
			}
			else
			{
				psVolInfo->aswA_Mic2[bCh]	= GetMcVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Mic2[bCh]);
			}
			psVolInfo->aswA_Mic2Gain[bCh]	= GetMcGainReg(gsGlobalInfo_AA.sVolInfo.aswA_Mic2Gain[bCh]);
		}
	}
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_MIC3_AA) == 0)
	{/*	MIC3 is unused	*/
		for(bCh = 0; bCh < MIC3_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswA_Mic3[bCh]		= MCDRV_REG_MUTE;
			psVolInfo->aswA_Mic3Gain[bCh]	= (SINT16)(McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_MC3_GAIN) & MCB_AA_MC3GAIN);
		}
	}
	else
	{
		for(bCh = 0; bCh < MIC3_VOL_CHANNELS; bCh++)
		{
			if(((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) != MCDRV_SRC0_MIC3_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) != MCDRV_SRC0_MIC3_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) != MCDRV_SRC0_MIC3_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) != MCDRV_SRC0_MIC3_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) != MCDRV_SRC0_MIC3_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) != MCDRV_SRC0_MIC3_ON)
			&& ((gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) != MCDRV_SRC0_MIC3_ON))
			{
				psVolInfo->aswA_Mic3[bCh]	= MCDRV_REG_MUTE;
			}
			else
			{
				psVolInfo->aswA_Mic3[bCh]	= GetMcVolReg(gsGlobalInfo_AA.sVolInfo.aswA_Mic3[bCh]);
			}
			psVolInfo->aswA_Mic3Gain[bCh]	= GetMcGainReg(gsGlobalInfo_AA.sVolInfo.aswA_Mic3Gain[bCh]);
		}
	}

	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR0_AA) == 0)
	{/*	DIR0 is unused	*/
		for(bCh = 0; bCh < DIO0_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dir0[bCh]		= MCDRV_REG_MUTE;
		}
	}
	else
	{
		for(bCh = 0; bCh < DIO0_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dir0[bCh]		= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Dir0[bCh] - swGainUp);
		}
	}
	if((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == MCDRV_SRC3_DIR0_ON
	|| ((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON
		&& McResCtrl_GetAESource_AA() == eMCDRV_SRC_DIR0_AA))
	{
		for(bCh = 0; bCh < DIO0_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dir0Att[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Dir0Att[bCh]);
		}
	}
	else
	{
		for(bCh = 0; bCh < DIO0_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dir0Att[bCh]	= MCDRV_REG_MUTE;
		}
	}
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR1_AA) == 0)
	{/*	DIR1 is unused	*/
		for(bCh = 0; bCh < DIO1_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dir1[bCh]		= MCDRV_REG_MUTE;
		}
	}
	else
	{
		for(bCh = 0; bCh < DIO1_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dir1[bCh]		= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Dir1[bCh] - swGainUp);
		}
	}
	if((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == MCDRV_SRC3_DIR1_ON
	|| ((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON
		&& McResCtrl_GetAESource_AA() == eMCDRV_SRC_DIR1_AA))
	{
		for(bCh = 0; bCh < DIO1_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dir1Att[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Dir1Att[bCh]);
		}
	}
	else
	{
		for(bCh = 0; bCh < DIO1_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dir1Att[bCh]	= MCDRV_REG_MUTE;
		}
	}
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR2_AA) == 0)
	{/*	DIR2 is unused	*/
		for(bCh = 0; bCh < DIO2_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dir2[bCh]		= MCDRV_REG_MUTE;
		}
	}
	else
	{
		for(bCh = 0; bCh < DIO2_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dir2[bCh]		= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Dir2[bCh] - swGainUp);
		}
	}
	if((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == MCDRV_SRC3_DIR2_ON
	|| ((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON
		&& McResCtrl_GetAESource_AA() == eMCDRV_SRC_DIR2_AA))
	{
		for(bCh = 0; bCh < DIO2_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dir2Att[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Dir2Att[bCh]);
		}
	}
	else
	{
		for(bCh = 0; bCh < DIO2_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dir2Att[bCh]	= MCDRV_REG_MUTE;
		}
	}

	if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_0_AA) == eMCDRV_SRC_NONE_AA)
	{/*	DIT0 source all off	*/
		for(bCh = 0; bCh < DIO0_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dit0[bCh]	= MCDRV_REG_MUTE;
		}
	}
	else
	{
		for(bCh = 0; bCh < DIO0_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dit0[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Dit0[bCh] + swGainUp);
		}
	}
	if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_1_AA) == eMCDRV_SRC_NONE_AA)
	{/*	DIT1 source all off	*/
		for(bCh = 0; bCh < DIO1_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dit1[bCh]	= MCDRV_REG_MUTE;
		}
	}
	else
	{
		for(bCh = 0; bCh < DIO1_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dit1[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Dit1[bCh] + swGainUp);
		}
	}
	if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_2_AA) == eMCDRV_SRC_NONE_AA)
	{/*	DIT2 source all off	*/
		for(bCh = 0; bCh < DIO2_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dit2[bCh]	= MCDRV_REG_MUTE;
		}
	}
	else
	{
		for(bCh = 0; bCh < DIO2_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dit2[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Dit2[bCh] + swGainUp);
		}
	}

	if(McResCtrl_GetDACSource_AA(eMCDRV_DAC_MASTER_AA) == eMCDRV_SRC_NONE_AA
	&& McResCtrl_GetDACSource_AA(eMCDRV_DAC_VOICE_AA) == eMCDRV_SRC_NONE_AA)
	{
		for(bCh = 0; bCh < DAC_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_DacAtt[bCh]		= MCDRV_REG_MUTE;
			psVolInfo->aswD_DacMaster[bCh]	= MCDRV_REG_MUTE;
			psVolInfo->aswD_DacVoice[bCh]	= MCDRV_REG_MUTE;
		}
	}
	else
	{
		if(McResCtrl_GetDACSource_AA(eMCDRV_DAC_MASTER_AA) == eMCDRV_SRC_NONE_AA)
		{
			for(bCh = 0; bCh < DAC_VOL_CHANNELS; bCh++)
			{
				psVolInfo->aswD_DacMaster[bCh]	= MCDRV_REG_MUTE;
			}
		}
		else
		{
			for(bCh = 0; bCh < DAC_VOL_CHANNELS; bCh++)
			{
				psVolInfo->aswD_DacMaster[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_DacMaster[bCh]);
			}
		}
		if(McResCtrl_GetDACSource_AA(eMCDRV_DAC_VOICE_AA) == eMCDRV_SRC_NONE_AA)
		{
			for(bCh = 0; bCh < DAC_VOL_CHANNELS; bCh++)
			{
				psVolInfo->aswD_DacVoice[bCh]	= MCDRV_REG_MUTE;
			}
		}
		else
		{
			for(bCh = 0; bCh < DAC_VOL_CHANNELS; bCh++)
			{
				psVolInfo->aswD_DacVoice[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_DacVoice[bCh]);
			}
		}
		for(bCh = 0; bCh < DAC_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_DacAtt[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_DacAtt[bCh]);
		}
	}

	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_PDM_AA) == 0
	&& McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_ADC0_AA) == 0)
	{/*	PDM&ADC0 is unused	*/
		for(bCh = 0; bCh < AENG6_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Aeng6[bCh]	= MCDRV_REG_MUTE;
		}
	}
	else
	{
		for(bCh = 0; bCh < AENG6_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Aeng6[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Aeng6[bCh]);
		}
	}

	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_PDM_AA) == 0)
	{/*	PDM is unused	*/
		for(bCh = 0; bCh < PDM_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Pdm[bCh]		= MCDRV_REG_MUTE;
			psVolInfo->aswD_SideTone[bCh]	= MCDRV_REG_MUTE;
		}
	}
	else
	{
		for(bCh = 0; bCh < PDM_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Pdm[bCh]		= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Pdm[bCh] - swGainUp);
			psVolInfo->aswD_SideTone[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_SideTone[bCh] - swGainUp);
		}
	}

	if(McDevProf_IsValid(eMCDRV_FUNC_DTMF) == 1)
	{
		/*	DTMF	*/
		for(bCh = 0; bCh < DTMF_VOL_CHANNELS; bCh++)
		{
			psVolInfo->aswD_Dtmfb[bCh]		= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_Dtmfb[bCh] - swGainUp);
			psVolInfo->aswD_DtmfAtt[bCh]	= GetDigitalVolReg(gsGlobalInfo_AA.sVolInfo.aswD_DtmfAtt[bCh] - swGainUp);
		}
	}
}

/****************************************************************************
 *	GetDigitalVolReg
 *
 *	Description:
 *			Get value of digital volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetDigitalVolReg
(
	SINT16	swVol
)
{
	SINT16	swRet;

	if(swVol < (-74*256))
	{
		swRet	= 0;
	}
	else if(swVol < 0)
	{
		swRet	= 96 + (swVol-128)/256;
	}
	else
	{
		swRet	= 96 + (swVol+128)/256;
	}

	if(swRet < 22)
	{
		swRet	= 0;
	}

	if(swRet > 114)
	{
		swRet	= 114;
	}

	return swRet;
}

/****************************************************************************
 *	GetADVolReg
 *
 *	Description:
 *			Get update value of analog AD volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetADVolReg
(
	SINT16	swVol
)
{
	SINT16	swRet;

	if(swVol < (-27*256))
	{
		swRet	= 0;
	}
	else if(swVol < 0)
	{
		swRet	= 19 + (swVol-192) * 2 / (256*3);
	}
	else
	{
		swRet	= 19 + (swVol+192) * 2 / (256*3);
	}

	if(swRet < 0)
	{
		swRet	= 0;
	}

	if(swRet > 31)
	{
		swRet	= 31;
	}

	return swRet;
}

/****************************************************************************
 *	GetLIVolReg
 *
 *	Description:
 *			Get update value of analog LIN volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetLIVolReg
(
	SINT16	swVol
)
{
	SINT16	swRet;

	if(swVol < (-30*256))
	{
		swRet	= 0;
	}
	else if(swVol < 0)
	{
		swRet	= 21 + (swVol-192) * 2 / (256*3);
	}
	else
	{
		swRet	= 21 + (swVol+192) * 2 / (256*3);
	}

	if(swRet < 0)
	{
		swRet	= 0;
	}
	if(swRet > 31)
	{
		swRet	= 31;
	}
	return swRet;
}

/****************************************************************************
 *	GetMcVolReg
 *
 *	Description:
 *			Get update value of analog Mic volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetMcVolReg
(
	SINT16	swVol
)
{
	SINT16	swRet;

	if(swVol < (-30*256))
	{
		swRet	= 0;
	}
	else if(swVol < 0)
	{
		swRet	= 21 + (swVol-192) * 2 / (256*3);
	}
	else
	{
		swRet	= 21 + (swVol+192) * 2 / (256*3);
	}

	if(swRet < 0)
	{
		swRet	= 0;
	}
	if(swRet > 31)
	{
		swRet	= 31;
	}
	return swRet;
}

/****************************************************************************
 *	GetMcGainReg
 *
 *	Description:
 *			Get update value of analog Mic gain registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetMcGainReg
(
	SINT16	swVol
)
{
	SINT16	swGain	= (swVol+128)/256;

	if(swGain < 18)
	{
		return 0;
	}
	if(swGain < 23)
	{
		return 1;
	}
	if(swGain < 28)
	{
		return 2;
	}

	return 3;
}

/****************************************************************************
 *	GetHpVolReg
 *
 *	Description:
 *			Get update value of analog Hp volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetHpVolReg
(
	SINT16	swVol
)
{
	SINT16	swDB	= swVol/256;

	if(swVol >= 0)
	{
		return 31;
	}

	if(swDB <= -8)
	{
		if(swVol < (-36*256))
		{
			return 0;
		}
		if(swDB <= -32)
		{
			return 1;
		}
		if(swDB <= -26)
		{
			return 2;
		}
		if(swDB <= -23)
		{
			return 3;
		}
		if(swDB <= -21)
		{
			return 4;
		}
		if(swDB <= -19)
		{
			return 5;
		}
		if(swDB <= -17)
		{
			return 6;
		}
		return 23+(swVol-128)/256;
	}

	return 31 + (swVol-64)*2/256;
}

/****************************************************************************
 *	GetHpGainReg
 *
 *	Description:
 *			Get update value of analog Hp gain registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetHpGainReg
(
	SINT16	swVol
)
{
	SINT16	swDB	= swVol/(256/4);

	if(swDB < 3)
	{
		return 0;
	}
	if(swDB < 9)
	{
		return 1;
	}
	if(swDB < 18)
	{
		return 2;
	}

	return 3;
}

/****************************************************************************
 *	GetSpVolReg
 *
 *	Description:
 *			Get update value of analog Sp volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetSpVolReg
(
	SINT16	swVol
)
{
	SINT16	swDB	= swVol/256;

	if(swVol >= 0)
	{
		return 31;
	}

	if(swDB <= -8)
	{
		if(swVol < (-36*256))
		{
			return 0;
		}
		if(swDB <= -32)
		{
			return 1;
		}
		if(swDB <= -26)
		{
			return 2;
		}
		if(swDB <= -23)
		{
			return 3;
		}
		if(swDB <= -21)
		{
			return 4;
		}
		if(swDB <= -19)
		{
			return 5;
		}
		if(swDB <= -17)
		{
			return 6;
		}
		return 23+(swVol-128)/256;
	}

	return 31 + (swVol-64)*2/256;
}

/****************************************************************************
 *	GetRcVolReg
 *
 *	Description:
 *			Get update value of analog Rcv volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetRcVolReg
(
	SINT16	swVol
)
{
	SINT16	swRet	= 31 + (swVol-128)/256;
	if(swVol < (-30*256))
	{
		swRet	= 0;
	}
	if(swRet < 0)
	{
		swRet	= 0;
	}
	if(swRet > 31)
	{
		swRet	= 31;
	}
	return swRet;
}

/****************************************************************************
 *	GetLoVolReg
 *
 *	Description:
 *			Get update value of analog Lout volume registers.
 *	Arguments:
 *			swVol	volume(dB value*256)
 *	Return:
 *			value of registers
 *
 ****************************************************************************/
static SINT16	GetLoVolReg
(
	SINT16	swVol
)
{
	SINT16	swRet	= 31 + (swVol-128)/256;
	if(swVol < (-30*256))
	{
		swRet	= 0;
	}
	if(swRet < 0)
	{
		swRet	= 0;
	}
	if(swRet > 31)
	{
		swRet	= 31;
	}
	return swRet;
}

/****************************************************************************
 *	McResCtrl_GetPowerInfo_AA
 *
 *	Description:
 *			Get power information.
 *	Arguments:
 *			psPowerInfo	power information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetPowerInfo_AA
(
	MCDRV_POWER_INFO_AA*	psPowerInfo
)
{
	UINT8	i;
	UINT8	bAnalogOn	= 0;
	UINT8	bPowerMode	= gsGlobalInfo_AA.sInitInfo.bPowerMode;

	/*	Digital power	*/
	psPowerInfo->dDigital	= 0;
	if((bPowerMode & MCDRV_POWMODE_CLKON) == 0)
	{
		psPowerInfo->dDigital |= (MCDRV_POWINFO_DIGITAL_DP0_AA | MCDRV_POWINFO_DIGITAL_DP1_AA | MCDRV_POWINFO_DIGITAL_DP2_AA | MCDRV_POWINFO_DIGITAL_PLLRST0_AA);
	}
	if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_0_AA)		!= eMCDRV_SRC_NONE_AA
	|| McResCtrl_GetDITSource_AA(eMCDRV_DIO_1_AA)		!= eMCDRV_SRC_NONE_AA
	|| McResCtrl_GetDITSource_AA(eMCDRV_DIO_2_AA)		!= eMCDRV_SRC_NONE_AA
	|| McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DAC_L_AA)	!= 0
	|| McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DAC_M_AA)	!= 0
	|| McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DAC_R_AA)	!= 0
	|| McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_ADC0_AA)		!= 0
	|| McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_AE_AA)		!= 0
	|| McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_MIX_AA)		!= 0)
	{
		/*	DP0-2, PLLRST0 on	*/
		psPowerInfo->dDigital &= ~(MCDRV_POWINFO_DIGITAL_DP0_AA | MCDRV_POWINFO_DIGITAL_DP1_AA | MCDRV_POWINFO_DIGITAL_DP2_AA | MCDRV_POWINFO_DIGITAL_PLLRST0_AA);
	}
	else
	{
		psPowerInfo->dDigital |= MCDRV_POWINFO_DIGITAL_DPB_AA;
	}

	/*	DPBDSP	*/
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_AE_AA) == 0
	|| (gsGlobalInfo_AA.sAeInfo.bOnOff&(MCDRV_BEXWIDE_ON|MCDRV_DRC_ON)) == 0)
	{
		psPowerInfo->dDigital |= MCDRV_POWINFO_DIGITAL_DPBDSP_AA;
	}

	/*	DPADIF	*/
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_ADC0_AA) == 0)
	{
		psPowerInfo->dDigital |= MCDRV_POWINFO_DIGITAL_DPADIF_AA;
	}

	/*	DPPDM	*/
	if(gsGlobalInfo_AA.sInitInfo.bPad0Func != MCDRV_PAD_PDMCK || McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_PDM_AA) == 0)
	{
		psPowerInfo->dDigital |= MCDRV_POWINFO_DIGITAL_DPPDM_AA;
	}

	/*	DPDI*	*/
	if(gsGlobalInfo_AA.sDioInfo.asPortInfo[0].sDioCommon.bMasterSlave == MCDRV_DIO_SLAVE
	|| (McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR0_AA) == 0 && McResCtrl_GetDITSource_AA(eMCDRV_DIO_0_AA) == eMCDRV_SRC_NONE_AA))
	{
		psPowerInfo->dDigital |= MCDRV_POWINFO_DIGITAL_DPDI0_AA;
	}
	if(gsGlobalInfo_AA.sDioInfo.asPortInfo[1].sDioCommon.bMasterSlave == MCDRV_DIO_SLAVE
	|| (McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR1_AA) == 0 && McResCtrl_GetDITSource_AA(eMCDRV_DIO_1_AA) == eMCDRV_SRC_NONE_AA))
	{
		psPowerInfo->dDigital |= MCDRV_POWINFO_DIGITAL_DPDI1_AA;
	}
	if(gsGlobalInfo_AA.sDioInfo.asPortInfo[2].sDioCommon.bMasterSlave == MCDRV_DIO_SLAVE
	|| (McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR2_AA) == 0 && McResCtrl_GetDITSource_AA(eMCDRV_DIO_2_AA) == eMCDRV_SRC_NONE_AA))
	{
		psPowerInfo->dDigital |= MCDRV_POWINFO_DIGITAL_DPDI2_AA;
	}

	/*	Analog power	*/
	for(i = 0; i < 5; i++)
	{
		psPowerInfo->abAnalog[i]	= 0;
	}

	/*	SPL*	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_SP_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		psPowerInfo->abAnalog[1] |= MCB_AA_PWM_SPL1;
		psPowerInfo->abAnalog[1] |= MCB_AA_PWM_SPL2;
	}
	else
	{
		bAnalogOn	= 1;
	}
	/*	SPR*	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_SP_AA, eMCDRV_DST_CH1_AA) == 0)
	{
		psPowerInfo->abAnalog[1] |= MCB_AA_PWM_SPR1;
		psPowerInfo->abAnalog[1] |= MCB_AA_PWM_SPR2;
	}
	else
	{
		bAnalogOn	= 1;
	}

	/*	HPL	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_HP_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		psPowerInfo->abAnalog[1] |= MCB_AA_PWM_HPL;
	}
	else
	{
		bAnalogOn	= 1;
	}
	/*	HPR	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_HP_AA, eMCDRV_DST_CH1_AA) == 0)
	{
		psPowerInfo->abAnalog[1] |= MCB_AA_PWM_HPR;
	}
	else
	{
		bAnalogOn	= 1;
	}
	/*	CP	*/
	if((psPowerInfo->abAnalog[1] & MCB_AA_PWM_HPL) != 0 && (psPowerInfo->abAnalog[1] & MCB_AA_PWM_HPR) != 0)
	{
		psPowerInfo->abAnalog[0] |= MCB_AA_PWM_CP;
	}

	/*	LOUT1L	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_LOUT1_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		psPowerInfo->abAnalog[2] |= MCB_AA_PWM_LO1L;
	}
	else
	{
		bAnalogOn	= 1;
	}
	/*	LOUT1R	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_LOUT1_AA, eMCDRV_DST_CH1_AA) == 0)
	{
		psPowerInfo->abAnalog[2] |= MCB_AA_PWM_LO1R;
	}
	else
	{
		bAnalogOn	= 1;
	}
	/*	LOUT2L	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_LOUT2_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		psPowerInfo->abAnalog[2] |= MCB_AA_PWM_LO2L;
	}
	else
	{
		bAnalogOn	= 1;
	}
	/*	LOUT2R	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_LOUT2_AA, eMCDRV_DST_CH1_AA) == 0)
	{
		psPowerInfo->abAnalog[2] |= MCB_AA_PWM_LO2R;
	}
	else
	{
		bAnalogOn	= 1;
	}
	/*	RCV	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_RCV_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		psPowerInfo->abAnalog[2] |= MCB_AA_PWM_RC1;
		psPowerInfo->abAnalog[2] |= MCB_AA_PWM_RC2;
	}
	else
	{
		bAnalogOn	= 1;
	}
	/*	DA	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_DAC_AA, eMCDRV_DST_CH0_AA) == 0
	&& McResCtrl_IsDstUsed_AA(eMCDRV_DST_DAC_AA, eMCDRV_DST_CH1_AA) == 0)
	{
		psPowerInfo->abAnalog[3] |= MCB_AA_PWM_DA;
	}
	else
	{
		bAnalogOn	= 1;
	}
	/*	ADC0L	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_ADC0_AA, eMCDRV_DST_CH0_AA) == 1)
	{
		bAnalogOn	= 1;
	}
	else
	{
		if(McDevProf_IsValid(eMCDRV_FUNC_LI2) == 1
		&& ((gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE2_L_BLOCK] & MCDRV_SRC2_LINE2_L_ON) == MCDRV_SRC2_LINE2_L_ON
			|| (gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE2_M_BLOCK] & MCDRV_SRC2_LINE2_M_ON) == MCDRV_SRC2_LINE2_M_ON))
		{
			bAnalogOn	= 1;
		}
		else
		{
			psPowerInfo->abAnalog[1] |= MCB_AA_PWM_ADL;
		}
	}
	/*	ADC0R	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_ADC0_AA, eMCDRV_DST_CH1_AA) == 1)
	{
		bAnalogOn	= 1;
	}
	else
	{
		if(McDevProf_IsValid(eMCDRV_FUNC_LI2) == 1
		&& ((gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE2_R_BLOCK] & MCDRV_SRC2_LINE2_R_ON) == MCDRV_SRC2_LINE2_R_ON
			|| (gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE2_M_BLOCK] & MCDRV_SRC2_LINE2_M_ON) == MCDRV_SRC2_LINE2_M_ON))
		{
			bAnalogOn	= 1;
		}
		else
		{
			psPowerInfo->abAnalog[1] |= MCB_AA_PWM_ADR;
		}
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_ADC1) == 1)
	{
	}
	/*	LI	*/
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_LINE1_L_AA) == 0
	&& McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_LINE1_M_AA) == 0
	&& McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_LINE1_R_AA) == 0)
	{
		psPowerInfo->abAnalog[4] |= MCB_AA_PWM_LI;
	}
	else
	{
		bAnalogOn	= 1;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_LI2) == 1)
	{
		if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_LINE2_L_AA) == 0
		&& McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_LINE2_M_AA) == 0
		&& McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_LINE2_R_AA) == 0)
		{
			/*psPowerInfo->abAnalog[4] |= MCB_AA_PWM_LI2;*/
		}
		else
		{
			bAnalogOn	= 1;
		}
	}
	/*	MC1	*/
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_MIC1_AA) == 0)
	{
		psPowerInfo->abAnalog[4] |= MCB_AA_PWM_MC1;
	}
	else
	{
		bAnalogOn	= 1;
	}
	/*	MC2	*/
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_MIC2_AA) == 0)
	{
		psPowerInfo->abAnalog[4] |= MCB_AA_PWM_MC2;
	}
	else
	{
		bAnalogOn	= 1;
	}
	/*	MC3	*/
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_MIC3_AA) == 0)
	{
		psPowerInfo->abAnalog[4] |= MCB_AA_PWM_MC3;
	}
	else
	{
		bAnalogOn	= 1;
	}
	if((gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) != MCDRV_SRC0_MIC1_ON)
	{
		psPowerInfo->abAnalog[3] |= MCB_AA_PWM_MB1;
	}
	else
	{
		bAnalogOn	= 1;
	}
	if((gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) != MCDRV_SRC0_MIC2_ON)
	{
		psPowerInfo->abAnalog[3] |= MCB_AA_PWM_MB2;
	}
	else
	{
		bAnalogOn	= 1;
	}
	if((gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) != MCDRV_SRC0_MIC3_ON)
	{
		psPowerInfo->abAnalog[3] |= MCB_AA_PWM_MB3;
	}
	else
	{
		bAnalogOn	= 1;
	}

	/*	VR/LDOA/REFA	*/
	if ((0 == bAnalogOn) && ((bPowerMode & MCDRV_POWMODE_VREFON) == 0))
	{
		psPowerInfo->abAnalog[0] |= MCB_AA_PWM_VR;
		psPowerInfo->abAnalog[0] |= MCB_AA_PWM_REFA;
		psPowerInfo->abAnalog[0] |= MCB_AA_PWM_LDOA;
	}
	else
	{
		if (MCDRV_LDO_OFF == gsGlobalInfo_AA.sInitInfo.bLdo)
		{
			psPowerInfo->abAnalog[0] |= MCB_AA_PWM_LDOA;
		}
		else
		{
			psPowerInfo->abAnalog[0] |= MCB_AA_PWM_REFA;
		}
	}
}

/****************************************************************************
 *	McResCtrl_GetPowerInfoRegAccess_AA
 *
 *	Description:
 *			Get power information to access register.
 *	Arguments:
 *			psRegInfo	register information
 *			psPowerInfo	power information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetPowerInfoRegAccess_AA
(
	const MCDRV_REG_INFO*	psRegInfo,
	MCDRV_POWER_INFO_AA*		psPowerInfo
)
{

	McResCtrl_GetPowerInfo_AA(psPowerInfo);

	switch(psRegInfo->bRegType)
	{
	default:
	case	MCDRV_REGTYPE_A:
	case	MCDRV_REGTYPE_B_BASE:
	case	MCDRV_REGTYPE_B_AE:
		break;
	case	MCDRV_REGTYPE_B_ANALOG:
		switch(psRegInfo->bAddress)
		{
		case	MCI_AA_AMP:
		case	MCI_AA_DNGATRT:
		case	MCI_AA_HPVOL_L:
		case	MCI_AA_HPVOL_R:
		case	MCI_AA_SPVOL_L:
		case	MCI_AA_SPVOL_R:
		case	MCI_AA_RCVOL:
		case	MCI_AA_ADL_MIX:
		case	MCI_AA_ADL_MONO:
		case	MCI_AA_ADR_MIX:
		case	MCI_AA_ADR_MONO:
		case	MCI_AA_LO1L_MIX:
		case	MCI_AA_LO1L_MONO:
		case	MCI_AA_LO1R_MIX:
		case	MCI_AA_LO2L_MIX:
		case	MCI_AA_LO2L_MONO:
		case	MCI_AA_LO2R_MIX:
		case	MCI_AA_HPL_MIX:
		case	MCI_AA_HPL_MONO:
		case	MCI_AA_HPR_MIX:
		case	MCI_AA_SPL_MIX:
		case	MCI_AA_SPL_MONO:
		case	MCI_AA_SPR_MIX:
		case	MCI_AA_SPR_MONO:
		case	MCI_AA_RC_MIX:
			psPowerInfo->abAnalog[0]	&= (UINT8)~MCB_AA_PWM_VR;
			break;
		case	MCI_AA_DNGON:
			psPowerInfo->dDigital	&= ~(MCDRV_POWINFO_DIGITAL_DP0_AA | MCDRV_POWINFO_DIGITAL_DP1_AA
												| MCDRV_POWINFO_DIGITAL_DP2_AA | MCDRV_POWINFO_DIGITAL_PLLRST0_AA);
			break;
		default:
			break;
		}
		break;

	case	MCDRV_REGTYPE_B_CODEC:
		if(psRegInfo->bAddress == MCI_AA_DPADIF || psRegInfo->bAddress == MCI_AA_CD_HW_ID)
		{
			break;
		}
		psPowerInfo->dDigital	&= ~(MCDRV_POWINFO_DIGITAL_DP0_AA | MCDRV_POWINFO_DIGITAL_DP1_AA
											| MCDRV_POWINFO_DIGITAL_DP2_AA | MCDRV_POWINFO_DIGITAL_PLLRST0_AA | MCDRV_POWINFO_DIGITAL_DPB_AA);
		break;

	case	MCDRV_REGTYPE_B_MIXER:
		psPowerInfo->dDigital	&= ~(MCDRV_POWINFO_DIGITAL_DP0_AA | MCDRV_POWINFO_DIGITAL_DP1_AA
											| MCDRV_POWINFO_DIGITAL_DP2_AA | MCDRV_POWINFO_DIGITAL_PLLRST0_AA | MCDRV_POWINFO_DIGITAL_DPB_AA);
		break;

	case	MCDRV_REGTYPE_B_CDSP:
		break;
	}
}

/****************************************************************************
 *	McResCtrl_GetCurPowerInfo_AA
 *
 *	Description:
 *			Get current power setting.
 *	Arguments:
 *			psPowerInfo	power information
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_GetCurPowerInfo_AA
(
	MCDRV_POWER_INFO_AA*	psPowerInfo
)
{
	UINT8	bReg;


	psPowerInfo->abAnalog[0]	= gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_PWM_ANALOG_0];
	psPowerInfo->abAnalog[1]	= gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_PWM_ANALOG_1];
	psPowerInfo->abAnalog[2]	= gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_PWM_ANALOG_2];
	psPowerInfo->abAnalog[3]	= gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_PWM_ANALOG_3];
	psPowerInfo->abAnalog[4]	= gsGlobalInfo_AA.abRegValB_ANA[MCI_AA_PWM_ANALOG_4];

	psPowerInfo->dDigital	= 0;
	bReg	= gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_PWM_DIGITAL];
	if((bReg & MCB_AA_PWM_DP0) == MCB_AA_PWM_DP0)
	{
		psPowerInfo->dDigital	|= MCDRV_POWINFO_DIGITAL_DP0_AA;
	}
	if((bReg & MCB_AA_PWM_DP1) == MCB_AA_PWM_DP1)
	{
		psPowerInfo->dDigital	|= MCDRV_POWINFO_DIGITAL_DP1_AA;
	}
	if((bReg & MCB_AA_PWM_DP2) == MCB_AA_PWM_DP2)
	{
		psPowerInfo->dDigital	|= MCDRV_POWINFO_DIGITAL_DP2_AA;
	}

	bReg	= gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_PWM_DIGITAL_1];
	if((bReg & MCB_AA_PWM_DPB) == MCB_AA_PWM_DPB)
	{
		psPowerInfo->dDigital	|= MCDRV_POWINFO_DIGITAL_DPB_AA;
	}
	if((bReg & MCB_AA_PWM_DPDI0) == MCB_AA_PWM_DPDI0)
	{
		psPowerInfo->dDigital	|= MCDRV_POWINFO_DIGITAL_DPDI0_AA;
	}
	if((bReg & MCB_AA_PWM_DPDI1) == MCB_AA_PWM_DPDI1)
	{
		psPowerInfo->dDigital	|= MCDRV_POWINFO_DIGITAL_DPDI1_AA;
	}
	if((bReg & MCB_AA_PWM_DPDI2) == MCB_AA_PWM_DPDI2)
	{
		psPowerInfo->dDigital	|= MCDRV_POWINFO_DIGITAL_DPDI2_AA;
	}
	if((bReg & MCB_AA_PWM_DPPDM) == MCB_AA_PWM_DPPDM)
	{
		psPowerInfo->dDigital	|= MCDRV_POWINFO_DIGITAL_DPPDM_AA;
	}

	bReg	= gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_PWM_DIGITAL_BDSP];
	if((bReg & MCB_AA_PWM_DPBDSP) == MCB_AA_PWM_DPBDSP)
	{
		psPowerInfo->dDigital	|= MCDRV_POWINFO_DIGITAL_DPBDSP_AA;
	}

	bReg	= gsGlobalInfo_AA.abRegValB_BASE[MCI_AA_PLL_RST];
	if((bReg & MCB_AA_PLLRST0) == MCB_AA_PLLRST0)
	{
		psPowerInfo->dDigital	|= MCDRV_POWINFO_DIGITAL_PLLRST0_AA;
	}

	bReg	= gsGlobalInfo_AA.abRegValB_CODEC[MCI_AA_DPADIF];
	if((bReg & MCB_AA_DPADIF) == MCB_AA_DPADIF)
	{
		psPowerInfo->dDigital	|= MCDRV_POWINFO_DIGITAL_DPADIF_AA;
	}
}

/****************************************************************************
 *	McResCtrl_GetDACSource_AA
 *
 *	Description:
 *			Get DAC source information.
 *	Arguments:
 *			eCh	0:Master/1:Voice
 *	Return:
 *			path source(MCDRV_SRC_TYPE_AA)
 *
 ****************************************************************************/
MCDRV_SRC_TYPE_AA	McResCtrl_GetDACSource_AA
(
	MCDRV_DAC_CH_AA	eCh
)
{

	if((gsGlobalInfo_AA.sPathInfo.asDac[eCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON)
	{
		return eMCDRV_SRC_PDM_AA;
	}
	else if((gsGlobalInfo_AA.sPathInfo.asDac[eCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON)
	{
		return eMCDRV_SRC_ADC0_AA;
	}
	else if((gsGlobalInfo_AA.sPathInfo.asDac[eCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == MCDRV_SRC3_DIR0_ON)
	{
		return eMCDRV_SRC_DIR0_AA;
	}
	else if((gsGlobalInfo_AA.sPathInfo.asDac[eCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == MCDRV_SRC3_DIR1_ON)
	{
		return eMCDRV_SRC_DIR1_AA;
	}
	else if((gsGlobalInfo_AA.sPathInfo.asDac[eCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == MCDRV_SRC3_DIR2_ON)
	{
		return eMCDRV_SRC_DIR2_AA;
	}
	else if((gsGlobalInfo_AA.sPathInfo.asDac[eCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == MCDRV_SRC6_MIX_ON)
	{
		return eMCDRV_SRC_MIX_AA;
	}
	else if((gsGlobalInfo_AA.sPathInfo.asDac[eCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON)
	{
		return McResCtrl_GetAESource_AA();
	}
	return eMCDRV_SRC_NONE_AA;
}

/****************************************************************************
 *	McResCtrl_GetDITSource_AA
 *
 *	Description:
 *			Get DIT source information.
 *	Arguments:
 *			ePort	port number
 *	Return:
 *			path source(MCDRV_SRC_TYPE_AA)
 *
 ****************************************************************************/
MCDRV_SRC_TYPE_AA	McResCtrl_GetDITSource_AA
(
	MCDRV_DIO_PORT_NO_AA	ePort
)
{
	MCDRV_CHANNEL*	pasDit;

	if(ePort == 0)
	{
		pasDit	= &gsGlobalInfo_AA.sPathInfo.asDit0[0];
	}
	else if(ePort == 1)
	{
		pasDit	= &gsGlobalInfo_AA.sPathInfo.asDit1[0];
	}
	else if(ePort == 2)
	{
		pasDit	= &gsGlobalInfo_AA.sPathInfo.asDit2[0];
	}
	else
	{
		return eMCDRV_SRC_NONE_AA;
	}

	if((pasDit->abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON)
	{
		return eMCDRV_SRC_PDM_AA;
	}
	else if((pasDit->abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON)
	{
		return eMCDRV_SRC_ADC0_AA;
	}
	else if((pasDit->abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == MCDRV_SRC3_DIR0_ON)
	{
		return eMCDRV_SRC_DIR0_AA;
	}
	else if((pasDit->abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == MCDRV_SRC3_DIR1_ON)
	{
		return eMCDRV_SRC_DIR1_AA;
	}
	else if((pasDit->abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == MCDRV_SRC3_DIR2_ON)
	{
		return eMCDRV_SRC_DIR2_AA;
	}
	else if((pasDit->abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == MCDRV_SRC6_MIX_ON)
	{
		return eMCDRV_SRC_MIX_AA;
	}
	else if((pasDit->abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON)
	{
		return McResCtrl_GetAESource_AA();
	}
	return eMCDRV_SRC_NONE_AA;
}

/****************************************************************************
 *	McResCtrl_GetAESource_AA
 *
 *	Description:
 *			Get AE source information.
 *	Arguments:
 *			none
 *	Return:
 *			path source(MCDRV_SRC_TYPE_AA)
 *
 ****************************************************************************/
MCDRV_SRC_TYPE_AA	McResCtrl_GetAESource_AA
(
void
)
{
	if((gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON)
	{
		return eMCDRV_SRC_PDM_AA;
	}
	else if((gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON)
	{
		return eMCDRV_SRC_ADC0_AA;
	}
	else if((gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == MCDRV_SRC3_DIR0_ON)
	{
		return eMCDRV_SRC_DIR0_AA;
	}
	else if((gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == MCDRV_SRC3_DIR1_ON)
	{
		return eMCDRV_SRC_DIR1_AA;
	}
	else if((gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == MCDRV_SRC3_DIR2_ON)
	{
		return eMCDRV_SRC_DIR2_AA;
	}
	else if((gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == MCDRV_SRC6_MIX_ON)
	{
		return eMCDRV_SRC_MIX_AA;
	}
	return eMCDRV_SRC_NONE_AA;
}

/****************************************************************************
 *	McResCtrl_IsSrcUsed_AA
 *
 *	Description:
 *			Is Src used
 *	Arguments:
 *			ePathSrc	path src type
 *	Return:
 *			0:unused/1:used
 *
 ****************************************************************************/
UINT8	McResCtrl_IsSrcUsed_AA
(
	MCDRV_SRC_TYPE_AA	ePathSrc
)
{
	UINT8	bUsed	= 0;

	switch(ePathSrc)
	{
	case	eMCDRV_SRC_MIC1_AA:
		if((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_MIC2_AA:
		if((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_MIC3_AA:
		if((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
		|| (gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
		|| (gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_LINE1_L_AA:
		if((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_LINE1_R_AA:
		if((gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON
		|| (gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_LINE1_M_AA:
		if((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_LINE2_L_AA:
		if((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE2_L_BLOCK] & MCDRV_SRC2_LINE2_L_ON) == MCDRV_SRC2_LINE2_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE2_L_BLOCK] & MCDRV_SRC2_LINE2_L_ON) == MCDRV_SRC2_LINE2_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE2_L_BLOCK] & MCDRV_SRC2_LINE2_L_ON) == MCDRV_SRC2_LINE2_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE2_L_BLOCK] & MCDRV_SRC2_LINE2_L_ON) == MCDRV_SRC2_LINE2_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE2_L_BLOCK] & MCDRV_SRC2_LINE2_L_ON) == MCDRV_SRC2_LINE2_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE2_L_BLOCK] & MCDRV_SRC2_LINE2_L_ON) == MCDRV_SRC2_LINE2_L_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_LINE2_R_AA:
		if((gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_LINE2_R_BLOCK] & MCDRV_SRC2_LINE2_R_ON) == MCDRV_SRC2_LINE2_R_ON
		|| (gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE2_R_BLOCK] & MCDRV_SRC2_LINE2_R_ON) == MCDRV_SRC2_LINE2_R_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_LINE2_R_BLOCK] & MCDRV_SRC2_LINE2_R_ON) == MCDRV_SRC2_LINE2_R_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_LINE2_R_BLOCK] & MCDRV_SRC2_LINE2_R_ON) == MCDRV_SRC2_LINE2_R_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE2_R_BLOCK] & MCDRV_SRC2_LINE2_R_ON) == MCDRV_SRC2_LINE2_R_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_LINE2_M_AA:
		if((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE2_M_BLOCK] & MCDRV_SRC2_LINE2_M_ON) == MCDRV_SRC2_LINE2_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE2_M_BLOCK] & MCDRV_SRC2_LINE2_M_ON) == MCDRV_SRC2_LINE2_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE2_M_BLOCK] & MCDRV_SRC2_LINE2_M_ON) == MCDRV_SRC2_LINE2_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_LINE2_M_BLOCK] & MCDRV_SRC2_LINE2_M_ON) == MCDRV_SRC2_LINE2_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE2_M_BLOCK] & MCDRV_SRC2_LINE2_M_ON) == MCDRV_SRC2_LINE2_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE2_M_BLOCK] & MCDRV_SRC2_LINE2_M_ON) == MCDRV_SRC2_LINE2_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE2_M_BLOCK] & MCDRV_SRC2_LINE2_M_ON) == MCDRV_SRC2_LINE2_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE2_M_BLOCK] & MCDRV_SRC2_LINE2_M_ON) == MCDRV_SRC2_LINE2_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE2_M_BLOCK] & MCDRV_SRC2_LINE2_M_ON) == MCDRV_SRC2_LINE2_M_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_DIR0_AA:
		if((gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == MCDRV_SRC3_DIR0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == MCDRV_SRC3_DIR0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == MCDRV_SRC3_DIR0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == MCDRV_SRC3_DIR0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == MCDRV_SRC3_DIR0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == MCDRV_SRC3_DIR0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == MCDRV_SRC3_DIR0_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_DIR1_AA:
		if((gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == MCDRV_SRC3_DIR1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == MCDRV_SRC3_DIR1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == MCDRV_SRC3_DIR1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == MCDRV_SRC3_DIR1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == MCDRV_SRC3_DIR1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == MCDRV_SRC3_DIR1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == MCDRV_SRC3_DIR1_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_DIR2_AA:
		if((gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == MCDRV_SRC3_DIR2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == MCDRV_SRC3_DIR2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == MCDRV_SRC3_DIR2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == MCDRV_SRC3_DIR2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == MCDRV_SRC3_DIR2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == MCDRV_SRC3_DIR2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == MCDRV_SRC3_DIR2_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_DTMF_AA:
		break;

	case	eMCDRV_SRC_PDM_AA:
		if((gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
		|| (gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_ADC0_AA:
		if((gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_ADC1_AA:
		break;

	case	eMCDRV_SRC_DAC_L_AA:
		if((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_DAC_R_AA:
		if((gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON
		|| (gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON
		|| (gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_DAC_M_AA:
		if((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON
		|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_AE_AA:
		if((gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON
		|| (gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_CDSP_AA:
		break;

	case	eMCDRV_SRC_MIX_AA:
		if((gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == MCDRV_SRC6_MIX_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == MCDRV_SRC6_MIX_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == MCDRV_SRC6_MIX_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == MCDRV_SRC6_MIX_ON
		|| (gsGlobalInfo_AA.sPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == MCDRV_SRC6_MIX_ON
		|| (gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == MCDRV_SRC6_MIX_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_SRC_DIR2_DIRECT_AA:
		break;

	case	eMCDRV_SRC_CDSP_DIRECT_AA:
		break;

	default:
		break;
	}

	return bUsed;
}

/****************************************************************************
 *	McResCtrl_IsDstUsed_AA
 *
 *	Description:
 *			Is Destination used
 *	Arguments:
 *			eType	path destination
 *			eCh		channel
 *	Return:
 *			0:unused/1:used
 *
 ****************************************************************************/
UINT8	McResCtrl_IsDstUsed_AA
(
	MCDRV_DST_TYPE_AA	eType,
	MCDRV_DST_CH	eCh
)
{
	UINT8	bUsed	= 0;

	switch(eType)
	{
	case	eMCDRV_DST_HP_AA:
		if(eCh == eMCDRV_DST_CH0_AA)
		{
			if((gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
			|| (gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
			|| (gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
			|| (gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
			|| (gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
			|| (gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
			|| (gsGlobalInfo_AA.sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
			{
				bUsed	= 1;
			}
		}
		else if(eCh == eMCDRV_DST_CH1_AA)
		{
			if((gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
			|| (gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
			|| (gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
			|| (gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON
			|| (gsGlobalInfo_AA.sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
			{
				bUsed	= 1;
			}
		}
		else
		{
		}
		break;

	case	eMCDRV_DST_SP_AA:
		if(eCh == eMCDRV_DST_CH0_AA)
		{
			if((gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
			|| (gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
			|| (gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
			|| (gsGlobalInfo_AA.sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
			{
				bUsed	= 1;
			}
		}
		else if(eCh == eMCDRV_DST_CH1_AA)
		{
			if((gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON
			|| (gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
			|| (gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON
			|| (gsGlobalInfo_AA.sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
			{
				bUsed	= 1;
			}
		}
		else
		{
		}
		break;

	case	eMCDRV_DST_RCV_AA:
		if(eCh == eMCDRV_DST_CH0_AA)
		{
			if((gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
			|| (gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
			|| (gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
			|| (gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
			|| (gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
			|| (gsGlobalInfo_AA.sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) ==MCDRV_SRC5_DAC_R_ON)
			{
				bUsed	= 1;
			}
		}
		else
		{
		}
		break;

	case	eMCDRV_DST_LOUT1_AA:
		if(eCh == eMCDRV_DST_CH0_AA)
		{
			if((gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
			{
				bUsed	= 1;
			}
		}
		else if(eCh == eMCDRV_DST_CH1_AA)
		{
			if((gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
			{
				bUsed	= 1;
			}
		}
		else
		{
		}
		break;

	case	eMCDRV_DST_LOUT2_AA:
		if(eCh == eMCDRV_DST_CH0_AA)
		{
			if((gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
			{
				bUsed	= 1;
			}
		}
		else if(eCh == eMCDRV_DST_CH1_AA)
		{
			if((gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON
			|| (gsGlobalInfo_AA.sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
			{
				bUsed	= 1;
			}
		}
		else
		{
		}
		break;

	case	eMCDRV_DST_PEAK_AA:
		break;

	case	eMCDRV_DST_DIT0_AA:
		if(eCh == eMCDRV_DST_CH0_AA)
		{
			if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_0_AA) != eMCDRV_SRC_NONE_AA)
			{
				bUsed	= 1;
			}
		}
		else
		{
		}
		break;

	case	eMCDRV_DST_DIT1_AA:
		if(eCh == eMCDRV_DST_CH0_AA)
		{
			if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_1_AA) != eMCDRV_SRC_NONE_AA)
			{
				bUsed	= 1;
			}
		}
		else
		{
		}
		break;

	case	eMCDRV_DST_DIT2_AA:
		if(eCh == eMCDRV_DST_CH0_AA)
		{
			if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_2_AA) != eMCDRV_SRC_NONE_AA)
			{
				bUsed	= 1;
			}
		}
		else
		{
		}
		break;

	case	eMCDRV_DST_DAC_AA:
		if(eCh == eMCDRV_DST_CH0_AA)
		{
			if(McResCtrl_GetDACSource_AA(eMCDRV_DAC_MASTER_AA) != eMCDRV_SRC_NONE_AA)
			{
				bUsed	= 1;
			}
		}
		else if(eCh == eMCDRV_DST_CH1_AA)
		{
			if(McResCtrl_GetDACSource_AA(eMCDRV_DAC_VOICE_AA) != eMCDRV_SRC_NONE_AA)
			{
				bUsed	= 1;
			}
		}
		else
		{
		}
		break;

	case	eMCDRV_DST_AE_AA:
		if(eCh == eMCDRV_DST_CH0_AA)
		{
			if(McResCtrl_GetAESource_AA() != eMCDRV_SRC_NONE_AA)
			{
				bUsed	= 1;
			}
		}
		else
		{
		}
		break;

	case	eMCDRV_DST_CDSP_AA:
		break;

	case	eMCDRV_DST_ADC0_AA:
		if(eCh == eMCDRV_DST_CH0_AA)
		{
			if((gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
			|| (gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
			|| (gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
			|| (gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
			|| (gsGlobalInfo_AA.sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
			{
				bUsed	= 1;
			}
		}
		else if(eCh == eMCDRV_DST_CH1_AA)
		{
			if((gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
			|| (gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
			|| (gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
			|| (gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON
			|| (gsGlobalInfo_AA.sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
			{
				bUsed	= 1;
			}
		}
		else
		{
		}
		break;

	case	eMCDRV_DST_ADC1_AA:
		if(McDevProf_IsValid(eMCDRV_FUNC_ADC1) == 1)
		{
			if(eCh == eMCDRV_DST_CH0_AA)
			{
				if((gsGlobalInfo_AA.sPathInfo.asAdc1[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
				|| (gsGlobalInfo_AA.sPathInfo.asAdc1[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
				|| (gsGlobalInfo_AA.sPathInfo.asAdc1[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON
				|| (gsGlobalInfo_AA.sPathInfo.asAdc1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
				|| (gsGlobalInfo_AA.sPathInfo.asAdc1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
				{
					bUsed	= 1;
				}
			}
			else
			{
			}
		}
		break;

	case	eMCDRV_DST_MIX_AA:
		if(eCh != 0)
		{
			break;
		}
		if((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
		|| (gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == MCDRV_SRC3_DIR0_ON
		|| (gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == MCDRV_SRC3_DIR1_ON
		|| (gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == MCDRV_SRC3_DIR2_ON
		|| (gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON)
		{
			bUsed	= 1;
		}
		break;

	case	eMCDRV_DST_BIAS_AA:
		if(eCh == eMCDRV_DST_CH0_AA)
		{
			if((gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON
			|| (gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON
			|| (gsGlobalInfo_AA.sPathInfo.asBias[0].abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON)
			{
				bUsed	= 1;
			}
		}
		else
		{
		}
		break;

	default:
		break;
	}

	return bUsed;
}

/****************************************************************************
 *	McResCtrl_GetRegAccess_AA
 *
 *	Description:
 *			Get register access availability
 *	Arguments:
 *			psRegInfo	register information
 *	Return:
 *			MCDRV_REG_ACCSESS_AA
 *
 ****************************************************************************/
MCDRV_REG_ACCSESS_AA	McResCtrl_GetRegAccess_AA
(
	const MCDRV_REG_INFO*	psRegInfo
)
{
	switch(psRegInfo->bRegType)
	{
	case	MCDRV_REGTYPE_A:
		return gawRegAccessAvailableA[psRegInfo->bAddress];

	case	MCDRV_REGTYPE_B_BASE:
		return gawRegAccessAvailableB_BASE[psRegInfo->bAddress];

	case	MCDRV_REGTYPE_B_ANALOG:
		return gawRegAccessAvailableB_ANA[psRegInfo->bAddress];

	case	MCDRV_REGTYPE_B_CODEC:
		return gawRegAccessAvailableB_CODEC[psRegInfo->bAddress];

	case	MCDRV_REGTYPE_B_MIXER:
		return gawRegAccessAvailableB_MIX[psRegInfo->bAddress];

	case	MCDRV_REGTYPE_B_AE:
		return gawRegAccessAvailableB_AE[psRegInfo->bAddress];

	default:
		break;
	}
	return eMCDRV_ACCESS_DENY_AA;
}

/****************************************************************************
 *	McResCtrl_GetAPMode_AA
 *
 *	Description:
 *			get auto power management mode.
 *	Arguments:
 *			none
 *	Return:
 *			eMCDRV_APM_ON_AA
 *			eMCDRV_APM_OFF
 *
 ****************************************************************************/
MCDRV_PMODE_AA	McResCtrl_GetAPMode_AA
(
	void
)
{
	return gsGlobalInfo_AA.eAPMode;
}


/****************************************************************************
 *	McResCtrl_AllocPacketBuf_AA
 *
 *	Description:
 *			allocate the buffer for register setting packets.
 *	Arguments:
 *			none
 *	Return:
 *			pointer to the area to store packets
 *
 ****************************************************************************/
MCDRV_PACKET_AA*	McResCtrl_AllocPacketBuf_AA
(
	void
)
{
	if(eMCDRV_PACKETBUF_ALLOCATED_AA == gsGlobalInfo_AA.ePacketBufAlloc)
	{
		return NULL;
	}

	gsGlobalInfo_AA.ePacketBufAlloc = eMCDRV_PACKETBUF_ALLOCATED_AA;
	return gasPacket;
}

/****************************************************************************
 *	McResCtrl_ReleasePacketBuf_AA
 *
 *	Description:
 *			Release the buffer for register setting packets.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_ReleasePacketBuf_AA
(
	void
)
{
	gsGlobalInfo_AA.ePacketBufAlloc = eMCDRV_PACKETBUF_FREE_AA;
}

/****************************************************************************
 *	McResCtrl_InitRegUpdate_AA
 *
 *	Description:
 *			Initialize the process of register update.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_InitRegUpdate_AA
(
	void
)
{
	gsGlobalInfo_AA.sCtrlPacket.wDataNum	= 0;
	gsGlobalInfo_AA.wCurSlaveAddress		= 0xFFFF;
	gsGlobalInfo_AA.wCurRegType			= 0xFFFF;
	gsGlobalInfo_AA.wCurRegAddress			= 0xFFFF;
	gsGlobalInfo_AA.wDataContinueCount		= 0;
	gsGlobalInfo_AA.wPrevAddressIndex		= 0;
}

/****************************************************************************
 *	McResCtrl_AddRegUpdate_AA
 *
 *	Description:
 *			Add register update packet and save register value.
 *	Arguments:
 *			wRegType	register type
 *			wAddress	address
 *			bData		write data
 *			eUpdateMode	updete mode
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_AddRegUpdate_AA
(
	UINT16	wRegType,
	UINT16	wAddress,
	UINT8	bData,
	MCDRV_UPDATE_MODE_AA	eUpdateMode
)
{
	UINT8			*pbRegVal;
	UINT8			bAddressADR;
	UINT8			bAddressWINDOW;
	UINT8			*pbCtrlData;
	UINT16			*pwCtrlDataNum;
	const UINT16	*pwNextAddress;
	UINT16			wNextAddress;
	UINT16			wSlaveAddress;

	switch (wRegType)
	{
	case MCDRV_PACKET_REGTYPE_A_AA:
		wSlaveAddress	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal		= gsGlobalInfo_AA.abRegValA;
		pwNextAddress	= gawNextAddressA;
		bAddressADR		= (UINT8)wAddress;
		bAddressWINDOW	= bAddressADR;
		if(MCDRV_A_REG_NUM_AA <= wAddress)
		{
			return;
		}
		break;

	case MCDRV_PACKET_REGTYPE_B_BASE_AA:
		wSlaveAddress	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal		= gsGlobalInfo_AA.abRegValB_BASE;
		pwNextAddress	= gawNextAddressB_BASE;
		bAddressADR		= MCI_AA_BASE_ADR;
		bAddressWINDOW	= MCI_AA_BASE_WINDOW;
		if(MCDRV_B_BASE_REG_NUM_AA <= wAddress)
		{
			return;
		}
		break;

	case MCDRV_PACKET_REGTYPE_B_CODEC_AA:
		wSlaveAddress	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal		= gsGlobalInfo_AA.abRegValB_CODEC;
		pwNextAddress	= gawNextAddressB_CODEC;
		bAddressADR		= MCI_AA_CD_ADR;
		bAddressWINDOW	= MCI_AA_CD_WINDOW;
		if(MCDRV_B_CODEC_REG_NUM_AA <= wAddress)
		{
			return;
		}
		break;

	case MCDRV_PACKET_REGTYPE_B_ANA_AA:
		wSlaveAddress	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
		pbRegVal		= gsGlobalInfo_AA.abRegValB_ANA;
		pwNextAddress	= gawNextAddressB_Ana;
		bAddressADR		= MCI_AA_ANA_ADR;
		bAddressWINDOW	= MCI_AA_ANA_WINDOW;
		if(MCDRV_B_ANA_REG_NUM_AA <= wAddress)
		{
			return;
		}
		break;

	case MCDRV_PACKET_REGTYPE_B_MIXER_AA:
		wSlaveAddress	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal		= gsGlobalInfo_AA.abRegValB_MIXER;
		pwNextAddress	= gawNextAddressB_MIXER;
		bAddressADR		= MCI_AA_MIX_ADR;
		bAddressWINDOW	= MCI_AA_MIX_WINDOW;
		if(MCDRV_B_MIXER_REG_NUM_AA <= wAddress)
		{
			return;
		}
		break;

	case MCDRV_PACKET_REGTYPE_B_AE_AA:
		wSlaveAddress	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal		= gsGlobalInfo_AA.abRegValB_AE;
		pwNextAddress	= gawNextAddressB_AE;
		bAddressADR		= MCI_AA_AE_ADR;
		bAddressWINDOW	= MCI_AA_AE_WINDOW;
		if(MCDRV_B_AE_REG_NUM_AA <= wAddress)
		{
			return;
		}
		break;

	case MCDRV_PACKET_REGTYPE_B_CDSP_AA:
		wSlaveAddress	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		pbRegVal		= gsGlobalInfo_AA.abRegValB_CDSP;
		pwNextAddress	= gawNextAddressB_CDSP;
		bAddressADR		= MCI_AA_CDSP_ADR;
		bAddressWINDOW	= MCI_AA_CDSP_WINDOW;
		if(MCDRV_B_CDSP_REG_NUM_AA <= wAddress)
		{
			return;
		}
		break;

	default:
		return;
	}

	if(gsGlobalInfo_AA.wCurSlaveAddress != 0xFFFF && gsGlobalInfo_AA.wCurSlaveAddress != wSlaveAddress)
	{
		McResCtrl_ExecuteRegUpdate_AA();
		McResCtrl_InitRegUpdate_AA();
	}

	if(gsGlobalInfo_AA.wCurRegType != 0xFFFF && gsGlobalInfo_AA.wCurRegType != wRegType)
	{
		McResCtrl_ExecuteRegUpdate_AA();
		McResCtrl_InitRegUpdate_AA();
	}

	if ((eMCDRV_UPDATE_FORCE_AA != eUpdateMode) && (bData == pbRegVal[wAddress]))
	{
		return;
	}

	if(gsGlobalInfo_AA.wCurRegAddress == 0xFFFF)
	{
		gsGlobalInfo_AA.wCurRegAddress	= wAddress;
	}

	if (eMCDRV_UPDATE_DUMMY_AA != eUpdateMode)
	{
		pbCtrlData		= gsGlobalInfo_AA.sCtrlPacket.abData;
		pwCtrlDataNum	= &(gsGlobalInfo_AA.sCtrlPacket.wDataNum);
		wNextAddress	= pwNextAddress[gsGlobalInfo_AA.wCurRegAddress];

		if ((wSlaveAddress == gsGlobalInfo_AA.wCurSlaveAddress)
		&& (wRegType == gsGlobalInfo_AA.wCurRegType)
		&& (0xFFFF != wNextAddress)
		&& (wAddress != wNextAddress))
		{
			if (pwNextAddress[wNextAddress] == wAddress)
			{
				if (0 == gsGlobalInfo_AA.wDataContinueCount)
				{
					pbCtrlData[gsGlobalInfo_AA.wPrevAddressIndex] |= MCDRV_BURST_WRITE_ENABLE;
				}
				pbCtrlData[*pwCtrlDataNum]	= pbRegVal[wNextAddress];
				(*pwCtrlDataNum)++;
				gsGlobalInfo_AA.wDataContinueCount++;
				wNextAddress				= pwNextAddress[wNextAddress];
			}
			else if ((0xFFFF != pwNextAddress[wNextAddress])
					&& pwNextAddress[pwNextAddress[wNextAddress]] == wAddress)
			{
				if (0 == gsGlobalInfo_AA.wDataContinueCount)
				{
					pbCtrlData[gsGlobalInfo_AA.wPrevAddressIndex] |= MCDRV_BURST_WRITE_ENABLE;
				}
				pbCtrlData[*pwCtrlDataNum]	= pbRegVal[wNextAddress];
				(*pwCtrlDataNum)++;
				pbCtrlData[*pwCtrlDataNum]	= pbRegVal[pwNextAddress[wNextAddress]];
				(*pwCtrlDataNum)++;
				gsGlobalInfo_AA.wDataContinueCount += 2;
				wNextAddress				= pwNextAddress[pwNextAddress[wNextAddress]];
			}
		}

		if ((0 == *pwCtrlDataNum) || (wAddress != wNextAddress))
		{
			if (0 != gsGlobalInfo_AA.wDataContinueCount)
			{
				McResCtrl_ExecuteRegUpdate_AA();
				McResCtrl_InitRegUpdate_AA();
			}

			if (MCDRV_PACKET_REGTYPE_A_AA == wRegType)
			{
				pbCtrlData[*pwCtrlDataNum]		= (bAddressADR << 1);
				gsGlobalInfo_AA.wPrevAddressIndex	= *pwCtrlDataNum;
				(*pwCtrlDataNum)++;
			}
			else
			{
				pbCtrlData[(*pwCtrlDataNum)++]	= (bAddressADR << 1);
				pbCtrlData[(*pwCtrlDataNum)++]	= (UINT8)wAddress;
				pbCtrlData[*pwCtrlDataNum]		= (bAddressWINDOW << 1);
				gsGlobalInfo_AA.wPrevAddressIndex	= (*pwCtrlDataNum)++;
			}
		}
		else
		{
			if (0 == gsGlobalInfo_AA.wDataContinueCount)
			{
				pbCtrlData[gsGlobalInfo_AA.wPrevAddressIndex] |= MCDRV_BURST_WRITE_ENABLE;
			}
			gsGlobalInfo_AA.wDataContinueCount++;
		}

		pbCtrlData[(*pwCtrlDataNum)++]	= bData;
	}

	gsGlobalInfo_AA.wCurSlaveAddress	= wSlaveAddress;
	gsGlobalInfo_AA.wCurRegType		= wRegType;
	gsGlobalInfo_AA.wCurRegAddress		= wAddress;

	/* save register value */
	pbRegVal[wAddress] = bData;
}

/****************************************************************************
 *	McResCtrl_ExecuteRegUpdate_AA
 *
 *	Description:
 *			Add register update packet and save register value.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
void	McResCtrl_ExecuteRegUpdate_AA
(
	void
)
{
	if (0 != gsGlobalInfo_AA.sCtrlPacket.wDataNum)
	{
		McSrv_WriteI2C((UINT8)gsGlobalInfo_AA.wCurSlaveAddress, gsGlobalInfo_AA.sCtrlPacket.abData, gsGlobalInfo_AA.sCtrlPacket.wDataNum);
	}
}

/****************************************************************************
 *	McResCtrl_WaitEvent_AA
 *
 *	Description:
 *			Wait event.
 *	Arguments:
 *			dEvent	event to wait
 *			dParam	event parameter
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McResCtrl_WaitEvent_AA
(
	UINT32	dEvent,
	UINT32	dParam
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT32	dInterval;
	UINT32	dTimeOut;
	UINT8	bSlaveAddr;
	UINT8	abWriteData[2];


	switch(dEvent)
	{
	case	MCDRV_EVT_INSFLG_AA:
		dInterval	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dSvolInterval;
		dTimeOut	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dSvolTimeOut;
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		if((dParam>>8) != (UINT32)0)
		{
			abWriteData[0]	= MCI_AA_MIX_ADR<<1;
			abWriteData[1]	= MCI_AA_DAC_INS_FLAG;
			McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
			sdRet	= WaitBitRelease(bSlaveAddr, MCI_AA_MIX_WINDOW, (UINT8)(dParam>>8), dInterval, dTimeOut);
			if(MCDRV_SUCCESS != sdRet)
			{
				break;
			}
		}
		else if((dParam&(UINT32)0xFF) != (UINT32)0)
		{
			abWriteData[0]	= MCI_AA_MIX_ADR<<1;
			abWriteData[1]	= MCI_AA_INS_FLAG;
			McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
			sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (UINT8)(dParam&(UINT32)0xFF), dInterval, dTimeOut);
		}
		break;

	case	MCDRV_EVT_ALLMUTE_AA:
		dInterval	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dSvolInterval;
		dTimeOut	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dSvolTimeOut;
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		abWriteData[0]	= MCI_AA_MIX_ADR<<1;
		abWriteData[1]	= MCI_AA_DIT_INVFLAGL;
		McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (MCB_AA_DIT0_INVFLAGL|MCB_AA_DIT1_INVFLAGL|MCB_AA_DIT2_INVFLAGL), dInterval, dTimeOut);
		if(MCDRV_SUCCESS != sdRet)
		{
			break;
		}
		abWriteData[0]	= MCI_AA_MIX_ADR<<1;
		abWriteData[1]	= MCI_AA_DIT_INVFLAGR;
		McSrv_WriteI2C(McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG), abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (MCB_AA_DIT0_INVFLAGR|MCB_AA_DIT1_INVFLAGR|MCB_AA_DIT2_INVFLAGR), dInterval, dTimeOut);
		if(MCDRV_SUCCESS != sdRet)
		{
			break;
		}
		abWriteData[0]	= MCI_AA_MIX_ADR<<1;
		abWriteData[1]	= MCI_AA_DIR_VFLAGL;
		McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (MCB_AA_PDM0_VFLAGL|MCB_AA_DIR0_VFLAGL|MCB_AA_DIR1_VFLAGL|MCB_AA_DIR2_VFLAGL), dInterval, dTimeOut);
		if(MCDRV_SUCCESS != sdRet)
		{
			break;
		}
		abWriteData[0]	= MCI_AA_MIX_ADR<<1;
		abWriteData[1]	= MCI_AA_DIR_VFLAGR;
		McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (MCB_AA_PDM0_VFLAGR|MCB_AA_DIR0_VFLAGR|MCB_AA_DIR1_VFLAGR|MCB_AA_DIR2_VFLAGR), dInterval, dTimeOut);
		if(MCDRV_SUCCESS != sdRet)
		{
			break;
		}
		abWriteData[0]	= MCI_AA_MIX_ADR<<1;
		abWriteData[1]	= MCI_AA_AD_VFLAGL;
		McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (MCB_AA_ADC_VFLAGL|MCB_AA_AENG6_VFLAGL), dInterval, dTimeOut);
		if(MCDRV_SUCCESS != sdRet)
		{
			break;
		}
		abWriteData[0]	= MCI_AA_MIX_ADR<<1;
		abWriteData[1]	= MCI_AA_AD_VFLAGR;
		McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (MCB_AA_ADC_VFLAGR|MCB_AA_AENG6_VFLAGR), dInterval, dTimeOut);
		if(MCDRV_SUCCESS != sdRet)
		{
			break;
		}
		abWriteData[0]	= MCI_AA_MIX_ADR<<1;
		abWriteData[1]	= MCI_AA_AFLAGL;
		McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (MCB_AA_ADC_AFLAGL|MCB_AA_DIR0_AFLAGL|MCB_AA_DIR1_AFLAGL|MCB_AA_DIR2_AFLAGL), dInterval, dTimeOut);
		if(MCDRV_SUCCESS != sdRet)
		{
			break;
		}
		abWriteData[0]	= MCI_AA_MIX_ADR<<1;
		abWriteData[1]	= MCI_AA_AFLAGR;
		McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (MCB_AA_ADC_AFLAGR|MCB_AA_DIR0_AFLAGR|MCB_AA_DIR1_AFLAGR|MCB_AA_DIR2_AFLAGR), dInterval, dTimeOut);
		if(MCDRV_SUCCESS != sdRet)
		{
			break;
		}
		abWriteData[0]	= MCI_AA_MIX_ADR<<1;
		abWriteData[1]	= MCI_AA_DAC_FLAGL;
		McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (MCB_AA_ST_FLAGL|MCB_AA_MASTER_OFLAGL|MCB_AA_VOICE_FLAGL|MCB_AA_DAC_FLAGL), dInterval, dTimeOut);
		if(MCDRV_SUCCESS != sdRet)
		{
			break;
		}
		abWriteData[0]	= MCI_AA_MIX_ADR<<1;
		abWriteData[1]	= MCI_AA_DAC_FLAGR;
		McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (MCB_AA_ST_FLAGR|MCB_AA_MASTER_OFLAGR|MCB_AA_VOICE_FLAGR|MCB_AA_DAC_FLAGR), dInterval, dTimeOut);
		break;

	case	MCDRV_EVT_DITMUTE_AA:
		dInterval	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dSvolInterval;
		dTimeOut	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dSvolTimeOut;
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		if((dParam>>8) != (UINT32)0)
		{
			abWriteData[0]	= MCI_AA_MIX_ADR<<1;
			abWriteData[1]	= MCI_AA_DIT_INVFLAGL;
			McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
			sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (UINT8)(dParam>>8), dInterval, dTimeOut);
			if(MCDRV_SUCCESS != sdRet)
			{
				break;
			}
		}
		if((dParam&(UINT32)0xFF) != (UINT32)0)
		{
			abWriteData[0]	= MCI_AA_MIX_ADR<<1;
			abWriteData[1]	= MCI_AA_DIT_INVFLAGR;
			McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
			sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (UINT8)(dParam&(UINT32)0xFF), dInterval, dTimeOut);
		}
		break;

	case	MCDRV_EVT_DACMUTE_AA:
		dInterval	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dSvolInterval;
		dTimeOut	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dSvolTimeOut;
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
		if((dParam>>8) != (UINT32)0)
		{
			abWriteData[0]	= MCI_AA_MIX_ADR<<1;
			abWriteData[1]	= MCI_AA_DAC_FLAGL;
			McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
			sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (UINT8)(dParam>>8), dInterval, dTimeOut);
			if(MCDRV_SUCCESS != sdRet)
			{
				break;
			}
		}
		if((dParam&(UINT32)0xFF) != (UINT32)0)
		{
			abWriteData[0]	= MCI_AA_MIX_ADR<<1;
			abWriteData[1]	= MCI_AA_DAC_FLAGR;
			McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
			sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_MIX_WINDOW, (UINT8)(dParam&(UINT32)0xFF), dInterval, dTimeOut);
		}
		break;

	case	MCDRV_EVT_SVOL_DONE_AA:
		dInterval	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dSvolInterval;
		dTimeOut	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dSvolTimeOut;
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
		if((dParam>>8) != (UINT32)0)
		{
			abWriteData[0]	= MCI_AA_ANA_ADR<<1;
			abWriteData[1]	= MCI_AA_BUSY1;
			McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
			sdRet	= WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_ANA_WINDOW, (UINT8)(dParam>>8), dInterval, dTimeOut);
			if(MCDRV_SUCCESS != sdRet)
			{
				break;
			}
		}
		if((dParam&(UINT32)0xFF) != (UINT32)0)
		{
			abWriteData[0]	= MCI_AA_ANA_ADR<<1;
			abWriteData[1]	= MCI_AA_BUSY2;
			McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
			return WaitBitRelease(bSlaveAddr, (UINT16)MCI_AA_ANA_WINDOW, (UINT8)(dParam&(UINT8)0xFF), dInterval, dTimeOut);
		}
		break;

	case	MCDRV_EVT_APM_DONE_AA:
		dInterval	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dSvolInterval;
		dTimeOut	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dSvolTimeOut;
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
		abWriteData[0]	= MCI_AA_ANA_ADR<<1;
		abWriteData[1]	= MCI_AA_AP_A1;
		McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitSet(bSlaveAddr, (UINT16)MCI_AA_ANA_WINDOW, (UINT8)(dParam>>8), dInterval, dTimeOut);
		if(MCDRV_SUCCESS != sdRet)
		{
			break;
		}
		abWriteData[0]	= MCI_AA_ANA_ADR<<1;
		abWriteData[1]	= MCI_AA_AP_A2;
		McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitSet(bSlaveAddr, (UINT16)MCI_AA_ANA_WINDOW, (UINT8)(dParam&(UINT8)0xFF), dInterval, dTimeOut);
		break;

	case	MCDRV_EVT_ANA_RDY_AA:
		dInterval	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dAnaRdyInterval;
		dTimeOut	= gsGlobalInfo_AA.sInitInfo.sWaitTime.dAnaRdyTimeOut;
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
		abWriteData[0]	= MCI_AA_ANA_ADR<<1;
		abWriteData[1]	= MCI_AA_RDY_FLAG;
		McSrv_WriteI2C(bSlaveAddr, abWriteData, 2);
		sdRet	= WaitBitSet(bSlaveAddr, (UINT16)MCI_AA_ANA_WINDOW, (UINT8)dParam, dInterval, dTimeOut);
		break;

	default:
		sdRet	= MCDRV_ERROR_ARGUMENT;
	}

	return sdRet;
}

/****************************************************************************
 *	WaitBitSet
 *
 *	Description:
 *			Wait register bit to set.
 *	Arguments:
 *			bSlaveAddr	slave address
 *			wRegAddr	register address
 *			bBit		bit
 *			dCycleTime	cycle time to poll [us]
 *			dTimeOut	number of read cycles for time out
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	WaitBitSet
(
	UINT8	bSlaveAddr,
	UINT16	wRegAddr,
	UINT8	bBit,
	UINT32	dCycleTime,
	UINT32	dTimeOut
)
{
	UINT8	bData;
	UINT32	dCycles;
	SINT32	sdRet;


	dCycles	= 0;
	sdRet	= MCDRV_ERROR_TIMEOUT;

	while(dCycles < dTimeOut)
	{
		bData	= McSrv_ReadI2C(bSlaveAddr, wRegAddr);
		if((bData & bBit) == bBit)
		{
			sdRet	= MCDRV_SUCCESS;
			break;
		}

		McSrv_Sleep(dCycleTime);
		dCycles++;
	}

	return sdRet;
}

/****************************************************************************
 *	WaitBitRelease
 *
 *	Description:
 *			Wait register bit to release.
 *	Arguments:
 *			bSlaveAddr	slave address
 *			wRegAddr	register address
 *			bBit		bit
 *			dCycleTime	cycle time to poll [us]
 *			dTimeOut	number of read cycles for time out
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	WaitBitRelease
(
	UINT8	bSlaveAddr,
	UINT16	wRegAddr,
	UINT8	bBit,
	UINT32	dCycleTime,
	UINT32	dTimeOut
)
{
	UINT8	bData;
	UINT32	dCycles;
	SINT32	sdRet;


	dCycles	= 0;
	sdRet	= MCDRV_ERROR_TIMEOUT;

	while(dCycles < dTimeOut)
	{
		bData	= McSrv_ReadI2C(bSlaveAddr, wRegAddr);
		if(0 == (bData & bBit))
		{
			sdRet	= MCDRV_SUCCESS;
			break;
		}

		McSrv_Sleep(dCycleTime);
		dCycles++;
	}

	return sdRet;
}

/****************************************************************************
 *	McResCtrl_ConfigHwAdj_AA
 *
 *	Description:
 *			HWADJ configuration.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_HWADJ
 *
 ****************************************************************************/
MCDRV_HWADJ	McResCtrl_ConfigHwAdj_AA
(
	void
)
{
	UINT8			bFs;
	UINT8			bSrc1;
	UINT8			bSrc2;
	UINT8			bSrc3;
	MCDRV_DST_TYPE_AA	eDstType;
	MCDRV_SRC_TYPE_AA	eSrcType;
	MCDRV_HWADJ		eHwAdj;

	eDstType = eMCDRV_DST_DIT0_AA;
	eSrcType = eMCDRV_SRC_DIR0_AA;
	eHwAdj = eMCDRV_HWADJ_THRU;
	bFs = 0xFF;

	/* DIR0->AE or DIR0->MIX */
	bSrc1 = gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON;
	bSrc2 = gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON;
	if((bSrc1 != 0) || (bSrc2 != 0))
	{
		bFs = gsGlobalInfo_AA.sDioInfo.asPortInfo[0].sDioCommon.bFs;
		eSrcType = eMCDRV_SRC_DIR0_AA;
	}

	/* DIR1->AE or DIR1->MIX */
	bSrc1 = gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON;
	bSrc2 = gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON;
	if((bSrc1 != 0) || (bSrc2 != 0))
	{
		if(bFs > gsGlobalInfo_AA.sDioInfo.asPortInfo[1].sDioCommon.bFs)
		{
			bFs = gsGlobalInfo_AA.sDioInfo.asPortInfo[1].sDioCommon.bFs;
			eSrcType = eMCDRV_SRC_DIR1_AA;
		}
	}

	/* DIR2->AE or DIR2->MIX */
	bSrc1 = gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON;
	bSrc2 = gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON;
	if((bSrc1 != 0) || (bSrc2 != 0))
	{
		if(bFs > gsGlobalInfo_AA.sDioInfo.asPortInfo[2].sDioCommon.bFs)
		{
			bFs = gsGlobalInfo_AA.sDioInfo.asPortInfo[2].sDioCommon.bFs;
			eSrcType = eMCDRV_SRC_DIR2_AA;
		}
	}

	if(bFs != 0xFF)
	{
		if((bFs != MCDRV_FS_48000) && (bFs != MCDRV_FS_44100) && (bFs != MCDRV_FS_8000))
		{
			bFs = 0xFF;
		}

		if(bFs != 0xFF)
		{
			switch(eSrcType)
			{
			case	eMCDRV_SRC_DIR0_AA:
				if((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) != 0)
				{
					if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_AE_AA, eMCDRV_DST_CH0_AA) == 1)
					{
						if(((gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == 0) &&
						   ((gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & MCDRV_SRC3_DIR0_ON) == 0))
						{
							bFs = 0xFF;
						}
					}
					else
					{
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;

						gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
						gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
						gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_OFF;
						gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_ON;
					}
				}
				break;
			case	eMCDRV_SRC_DIR1_AA:
				if((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) != 0)
				{
					if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_AE_AA, eMCDRV_DST_CH0_AA) == 1)
					{
						if(((gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == 0) &&
						   ((gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & MCDRV_SRC3_DIR1_ON) == 0))
						{
							bFs = 0xFF;
						}
					}
					else
					{
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;

						gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
						gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
						gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_OFF;
						gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_ON;
					}
				}
				break;
			case	eMCDRV_SRC_DIR2_AA:
				if((gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) != 0)
				{
					if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_AE_AA, eMCDRV_DST_CH0_AA) == 1)
					{
						if(((gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == 0) &&
						   ((gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & MCDRV_SRC3_DIR2_ON) == 0))
						{
							bFs = 0xFF;
						}
					}
					else
					{
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_OFF;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
						gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;

						gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
						gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
						gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	&= (UINT8)~MCDRV_SRC6_AE_OFF;
						gsGlobalInfo_AA.sPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]	|= MCDRV_SRC6_AE_ON;
					}
				}
				break;
			default:
				bFs = 0xFF;
				break;
			}

			switch(bFs)
			{
			case	MCDRV_FS_48000:
				eHwAdj = eMCDRV_HWADJ_PLAY48;
				break;
			case	MCDRV_FS_44100:
				eHwAdj = eMCDRV_HWADJ_PLAY44;
				break;
			case	MCDRV_FS_8000:
				eHwAdj = eMCDRV_HWADJ_PLAY8;
				break;
			default:
				break;
			}
		}
	}
	else
	{
		/* AD->DIT0 or AD->AE->DIT0 */
		bSrc1 = gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON;
		bSrc2 = gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON;
		bSrc3 = gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON;
		if((bSrc1 != 0) || ((bSrc2 != 0) && (bSrc3 != 0)))
		{
			bFs = gsGlobalInfo_AA.sDioInfo.asPortInfo[0].sDioCommon.bFs;
			eDstType = eMCDRV_DST_DIT0_AA;
		}

		/* AD->DIT1 or AD->AE->DIT1 */
		bSrc1 = gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON;
		bSrc2 = gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON;
		bSrc3 = gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON;
		if((bSrc1 != 0) || ((bSrc2 != 0) && (bSrc3 != 0)))
		{
			if(bFs > gsGlobalInfo_AA.sDioInfo.asPortInfo[1].sDioCommon.bFs)
			{
				bFs = gsGlobalInfo_AA.sDioInfo.asPortInfo[1].sDioCommon.bFs;
				eDstType = eMCDRV_DST_DIT1_AA;
			}
		}

		/* AD->DIT2 or AD->AE->DIT2 */
		bSrc1 = gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON;
		bSrc2 = gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON;
		bSrc3 = gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON;
		if((bSrc1 != 0) || ((bSrc2 != 0) && (bSrc3 != 0)))
		{
			if(bFs > gsGlobalInfo_AA.sDioInfo.asPortInfo[2].sDioCommon.bFs)
			{
				bFs = gsGlobalInfo_AA.sDioInfo.asPortInfo[2].sDioCommon.bFs;
				eDstType = eMCDRV_DST_DIT2_AA;
			}
		}

		if((bFs != MCDRV_FS_48000) && (bFs != MCDRV_FS_44100) && (bFs != MCDRV_FS_8000))
		{
			bFs = 0xFF;
		}

		if(bFs != 0xFF)
		{
			switch(eDstType)
			{
			case	eMCDRV_DST_DIT0_AA:
				if(((gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) != 0) ||
				   ((gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) != 0))
				{
					bFs = 0xFF;
				}
				else
				{
					if((gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) != 0)
					{
						if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_AE_AA, eMCDRV_DST_CH0_AA) == 1)
						{
							bFs = 0xFF;
						}
						else
						{
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_ON;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
							gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
						}
					}
				}
				break;
			case	eMCDRV_DST_DIT1_AA:
				if(((gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) != 0) ||
				   ((gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) != 0))
				{
					bFs = 0xFF;
				}
				else
				{
					if((gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) != 0)
					{
						if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_AE_AA, eMCDRV_DST_CH0_AA) == 1)
						{
							bFs = 0xFF;
						}
						else
						{
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_ON;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
							gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
						}
					}
				}
				break;
			case	eMCDRV_DST_DIT2_AA:
				if(((gsGlobalInfo_AA.sPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) != 0) ||
				   ((gsGlobalInfo_AA.sPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) != 0))
				{
					bFs = 0xFF;
				}
				else
				{
					if((gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) != 0)
					{
						if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_AE_AA, eMCDRV_DST_CH0_AA) == 1)
						{
							bFs = 0xFF;
						}
						else
						{
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_ON;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		&= (UINT8)~MCDRV_SRC6_AE_OFF;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK]		|= MCDRV_SRC6_AE_ON;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
							gsGlobalInfo_AA.sPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
						}
					}
				}
				break;
			default:
				bFs = 0xFF;
				break;
			}


			if((bFs != 0xFF) && (McResCtrl_IsDstUsed_AA(eMCDRV_DST_AE_AA, eMCDRV_DST_CH0_AA) == 0))
			{
				gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	&= (UINT8)~MCDRV_SRC4_PDM_ON;
				gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK]	|= MCDRV_SRC4_PDM_OFF;
				gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	&= (UINT8)~MCDRV_SRC4_ADC0_OFF;
				gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK]	|= MCDRV_SRC4_ADC0_ON;
				gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR0_ON;
				gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK]	|= MCDRV_SRC3_DIR0_OFF;
				gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR1_ON;
				gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK]	|= MCDRV_SRC3_DIR1_OFF;
				gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	&= (UINT8)~MCDRV_SRC3_DIR2_ON;
				gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK]	|= MCDRV_SRC3_DIR2_OFF;
				gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	&= (UINT8)~MCDRV_SRC6_MIX_ON;
				gsGlobalInfo_AA.sPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK]	|= MCDRV_SRC6_MIX_OFF;
			}

			switch(bFs)
			{
			case	MCDRV_FS_48000:
				eHwAdj = eMCDRV_HWADJ_REC48;
				break;
			case	MCDRV_FS_44100:
				eHwAdj = eMCDRV_HWADJ_REC44;
				break;
			case	MCDRV_FS_8000:
				eHwAdj = eMCDRV_HWADJ_REC8;
				break;
			default:
				break;
			}
		}
	}

	if(gsGlobalInfo_AA.eHwAdj == eHwAdj)
	{
		eHwAdj = eMCDRV_HWADJ_NOCHANGE;
	}
	else
	{
		gsGlobalInfo_AA.eHwAdj = eHwAdj;
	}

	return eHwAdj;
}

/**************************************/

/* volume update */
typedef enum
{
	eMCDRV_VOLUPDATE_MUTE_AA,
	eMCDRV_VOLUPDATE_ALL_AA
} MCDRV_VOLUPDATE_MODE_AA;

/* power update */
typedef struct
{
	UINT32	dDigital;
	UINT8	abAnalog[5];
} MCDRV_POWER_UPDATE_AA;

#define	MCDRV_POWUPDATE_DIGITAL_ALL_AA		(0xFFFFFFFFUL)
#define	MCDRV_POWUPDATE_ANALOG0_ALL_AA		(0x0F)
#define	MCDRV_POWUPDATE_ANALOG1_ALL_AA		(0xFF)
#define	MCDRV_POWUPDATE_ANALOG2_ALL_AA		(0x3F)
#define	MCDRV_POWUPDATE_ANALOG3_ALL_AA		(0x0F)
#define	MCDRV_POWUPDATE_ANALOG4_ALL_AA		(0xF0)
#define	MCDRV_POWUPDATE_ANALOG0_IN_AA		(0x0D)
#define	MCDRV_POWUPDATE_ANALOG1_IN_AA		(0xC0)
#define	MCDRV_POWUPDATE_ANALOG2_IN_AA		(0x00)
#define	MCDRV_POWUPDATE_ANALOG3_IN_AA		(0x0F)
#define	MCDRV_POWUPDATE_ANALOG4_IN_AA		(0xF0)
#define	MCDRV_POWUPDATE_ANALOG0_OUT_AA		(0x02)
#define	MCDRV_POWUPDATE_ANALOG1_OUT_AA		(0x3F)
#define	MCDRV_POWUPDATE_ANALOG2_OUT_AA		(0x3F)
#define	MCDRV_POWUPDATE_ANALOG3_OUT_AA		(0x00)
#define	MCDRV_POWUPDATE_ANALOG4_OUT_AA		(0x00)


#define	MCDRV_TCXO_WAIT_TIME_AA		((UINT32)2000)
#define	MCDRV_PLRST_WAIT_TIME_AA	((UINT32)2000)
#define	MCDRV_LDOA_WAIT_TIME_AA		((UINT32)1000)
#define	MCDRV_SP_WAIT_TIME_AA		((UINT32)150)
#define	MCDRV_HP_WAIT_TIME_AA		((UINT32)300)
#define	MCDRV_RC_WAIT_TIME_AA		((UINT32)150)

/* SrcRate default */
#define	MCDRV_DIR_SRCRATE_48000_AA	(32768)
#define	MCDRV_DIR_SRCRATE_44100_AA	(30106)
#define	MCDRV_DIR_SRCRATE_32000_AA	(21845)
#define	MCDRV_DIR_SRCRATE_24000_AA	(16384)
#define	MCDRV_DIR_SRCRATE_22050_AA	(15053)
#define	MCDRV_DIR_SRCRATE_16000_AA	(10923)
#define	MCDRV_DIR_SRCRATE_12000_AA	(8192)
#define	MCDRV_DIR_SRCRATE_11025_AA	(7526)
#define	MCDRV_DIR_SRCRATE_8000_AA	(5461)

#define	MCDRV_DIT_SRCRATE_48000_AA	(4096)
#define	MCDRV_DIT_SRCRATE_44100_AA	(4458)
#define	MCDRV_DIT_SRCRATE_32000_AA	(6144)
#define	MCDRV_DIT_SRCRATE_24000_AA	(8192)
#define	MCDRV_DIT_SRCRATE_22050_AA	(8916)
#define	MCDRV_DIT_SRCRATE_16000_AA	(12288)
#define	MCDRV_DIT_SRCRATE_12000_AA	(16384)
#define	MCDRV_DIT_SRCRATE_11025_AA	(17833)
#define	MCDRV_DIT_SRCRATE_8000_AA	(24576)

static SINT32	AddAnalogPowerUpAuto(const MCDRV_POWER_INFO_AA* psPowerInfo, const MCDRV_POWER_UPDATE_AA* psPowerUpdate);

static void		AddDIPad(void);
static void		AddPAD(void);
static SINT32	AddSource(void);
static void		AddDIStart(void);
static UINT8	GetMicMixBit(const MCDRV_CHANNEL* psChannel);
static void		AddStopADC(void);
static void		AddStopPDM(void);
static UINT8	IsModifiedDIO(UINT32 dUpdateInfo);
static UINT8	IsModifiedDIOCommon(MCDRV_DIO_PORT_NO_AA ePort);
static UINT8	IsModifiedDIODIR(MCDRV_DIO_PORT_NO_AA ePort);
static UINT8	IsModifiedDIODIT(MCDRV_DIO_PORT_NO_AA ePort);
static void		AddDIOCommon(MCDRV_DIO_PORT_NO_AA ePort);
static void		AddDIODIR(MCDRV_DIO_PORT_NO_AA ePort);
static void		AddDIODIT(MCDRV_DIO_PORT_NO_AA ePort);

#define	MCDRV_DPB_KEEP_AA	0
#define	MCDRV_DPB_UP_AA	1
static SINT32	PowerUpDig(UINT8 bDPBUp);

static UINT32	GetMaxWait(UINT8 bRegChange);

static SINT32		McPacket_AddInit_AA			(const MCDRV_INIT_INFO* psInitInfo);
static SINT32		McPacket_AddVol_AA			(UINT32 dUpdate, MCDRV_VOLUPDATE_MODE_AA eMode);
static SINT32		McPacket_AddPowerUp_AA		(const MCDRV_POWER_INFO_AA* psPowerInfo, const MCDRV_POWER_UPDATE_AA* psPowerUpdate);
static SINT32		McPacket_AddPowerDown_AA	(const MCDRV_POWER_INFO_AA* psPowerInfo, const MCDRV_POWER_UPDATE_AA* psPowerUpdate);
static SINT32		McPacket_AddPathSet_AA		(void);
static SINT32		McPacket_AddMixSet_AA		(void);
static SINT32		McPacket_AddStart_AA		(void);
static SINT32		McPacket_AddStop_AA			(void);
static SINT32		McPacket_AddDigitalIO_AA	(UINT32 dUpdateInfo);
static SINT32		McPacket_AddDAC_AA			(UINT32 dUpdateInfo);
static SINT32		McPacket_AddADC_AA			(UINT32 dUpdateInfo);
static SINT32		McPacket_AddSP_AA			(void);
static SINT32		McPacket_AddDNG_AA			(UINT32 dUpdateInfo);
static SINT32		McPacket_AddAE_AA			(UINT32 dUpdateInfo);
static SINT32		McPacket_AddPDM_AA			(UINT32 dUpdateInfo);
static SINT32		McPacket_AddGPMode_AA		(void);
static SINT32		McPacket_AddGPMask_AA		(UINT32 dPadNo);
static SINT32		McPacket_AddGPSet_AA		(UINT8 bGpio, UINT32 dPadNo);

/****************************************************************************
 *	McPacket_AddInit_AA
 *
 *	Description:
 *			Add initialize packet.
 *	Arguments:
 *			psInitInfo		information for initialization
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
SINT32	McPacket_AddInit_AA
(
	const MCDRV_INIT_INFO*	psInitInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	MCDRV_POWER_INFO_AA	sPowerInfo;
	MCDRV_POWER_UPDATE_AA	sPowerUpdate;


	/*	RSTA	*/
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | MCI_AA_RST, MCB_AA_RST);
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | MCI_AA_RST, 0);

	/*	ANA_RST	*/
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_ANA_RST, MCI_AA_ANA_RST_DEF);
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_ANA_RST, 0);

	/*	SDIN_MSK*, SDO_DDR*	*/
	bReg	= MCB_AA_SDIN_MSK2;
	if(psInitInfo->bDioSdo2Hiz == MCDRV_DAHIZ_LOW)
	{
		bReg |= MCB_AA_SDO_DDR2;
	}
	bReg |= MCB_AA_SDIN_MSK1;
	if(psInitInfo->bDioSdo1Hiz == MCDRV_DAHIZ_LOW)
	{
		bReg |= MCB_AA_SDO_DDR1;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_SD_MSK, bReg);

	bReg	= MCB_AA_SDIN_MSK0;
	if(psInitInfo->bDioSdo0Hiz == MCDRV_DAHIZ_LOW)
	{
		bReg |= MCB_AA_SDO_DDR0;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_SD_MSK_1, bReg);

	/*	BCLK_MSK*, BCLD_DDR*, LRCK_MSK*, LRCK_DDR*, PCM_HIZ*	*/
	bReg	= 0;
	bReg |= MCB_AA_BCLK_MSK2;
	bReg |= MCB_AA_LRCK_MSK2;
	if(psInitInfo->bDioClk2Hiz == MCDRV_DAHIZ_LOW)
	{
		bReg |= MCB_AA_BCLK_DDR2;
		bReg |= MCB_AA_LRCK_DDR2;
	}
	bReg |= MCB_AA_BCLK_MSK1;
	bReg |= MCB_AA_LRCK_MSK1;
	if(psInitInfo->bDioClk1Hiz == MCDRV_DAHIZ_LOW)
	{
		bReg |= MCB_AA_BCLK_DDR1;
		bReg |= MCB_AA_LRCK_DDR1;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_BCLK_MSK, bReg);

	bReg	= 0;
	bReg |= MCB_AA_BCLK_MSK0;
	bReg |= MCB_AA_LRCK_MSK0;
	if(psInitInfo->bDioClk0Hiz == MCDRV_DAHIZ_LOW)
	{
		bReg |= MCB_AA_BCLK_DDR0;
		bReg |= MCB_AA_LRCK_DDR0;
	}
	if(psInitInfo->bPcmHiz == MCDRV_PCMHIZ_HIZ)
	{
		bReg |= (MCB_AA_PCMOUT_HIZ2 | MCB_AA_PCMOUT_HIZ1 | MCB_AA_PCMOUT_HIZ0);
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_BCLK_MSK_1, bReg);

	/*	DI*_BCKP	*/

	/*	PA*_MSK, PA*_DDR	*/
	bReg	= MCI_AA_PA_MSK_1_DEF;
	if(psInitInfo->bPad0Func == MCDRV_PAD_PDMCK)
	{
		bReg	|= MCB_AA_PA0_DDR;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PA_MSK_1, bReg);

	/*	PA0_OUT	*/
	if(psInitInfo->bPad0Func == MCDRV_PAD_PDMCK)
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PA_OUT, MCB_AA_PA_OUT);
	}

	/*	SCU_PA*	*/

	/*	CKSEL	*/
	if(psInitInfo->bCkSel != MCDRV_CKSEL_CMOS)
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_CKSEL, MCB_AA_CKSEL);
	}

	/*	DIVR0, DIVF0	*/
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_DIVR0, psInitInfo->bDivR0);
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_DIVF0, psInitInfo->bDivF0);

	/*	Clock Start	*/
	sdRet	= McDevIf_ExecutePacket_AA();
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}
	McSrv_ClockStart();

	/*	DP0	*/
	bReg	= MCB_AA_PWM_DP2|MCB_AA_PWM_DP1;
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL, bReg);
	if(psInitInfo->bCkSel != MCDRV_CKSEL_CMOS)
	{
		/*	2ms wait	*/
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | MCDRV_TCXO_WAIT_TIME_AA, 0);
	}

	/*	PLLRST0	*/
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PLL_RST, 0);
	/*	2ms wait	*/
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | MCDRV_PLRST_WAIT_TIME_AA, 0);
	/*	DP1/DP2 up	*/
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL, 0);
	/*	RSTB	*/
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_RSTB, 0);
	/*	DPB	*/
	bReg	= MCB_AA_PWM_DPPDM|MCB_AA_PWM_DPDI2|MCB_AA_PWM_DPDI1|MCB_AA_PWM_DPDI0;
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL_1, bReg);

	/*	DCL_GAIN, DCL_LMT	*/
	bReg	= (psInitInfo->bDclGain<<4) | psInitInfo->bDclLimit;
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_DCL, bReg);

	/*	DIF_LI, DIF_LO*	*/
	bReg	= (psInitInfo->bLineOut2Dif<<5) | (psInitInfo->bLineOut1Dif<<4) | psInitInfo->bLineIn1Dif;
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_DIF_LINE, bReg);

	/*	SP*_HIZ, SPMN	*/
	bReg	= (psInitInfo->bSpmn << 1);
	if(MCDRV_SPHIZ_HIZ == psInitInfo->bSpHiz)
	{
		bReg	|= (MCB_AA_SPL_HIZ|MCB_AA_SPR_HIZ);
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_SP_MODE, bReg);

	/*	MC*SNG	*/
	bReg	= (psInitInfo->bMic2Sng<<6) | (psInitInfo->bMic1Sng<<2);
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_MC_GAIN, bReg);
	bReg	= (psInitInfo->bMic3Sng<<2);
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_MC3_GAIN, bReg);

	/*	AVDDLEV, VREFLEV	*/
	bReg	= 0x20 | psInitInfo->bAvddLev;
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LEV, bReg);

	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | (UINT32)MCI_AA_ANA_ADR, MCI_AA_PWM_ANALOG_0);
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | (UINT32)60, 0xD9);
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | (UINT32)MCI_AA_ANA_WINDOW, MCI_AA_PWM_ANALOG_0_DEF);

	if((psInitInfo->bPowerMode & MCDRV_POWMODE_VREFON) != 0 || McResCtrl_GetAPMode_AA() == eMCDRV_APM_OFF_AA)
	{
		bReg	= MCI_AA_PWM_ANALOG_0_DEF;
		if(psInitInfo->bLdo == MCDRV_LDO_ON)
		{
			/*	AP_LDOA	*/
			bReg	&= (UINT8)~MCB_AA_PWM_LDOA;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_0, bReg);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | MCDRV_LDOA_WAIT_TIME_AA, 0);
		}
		else
		{
			bReg	&= (UINT8)~MCB_AA_PWM_REFA;
		}
		/*	AP_VR up	*/
		bReg	&= (UINT8)~MCB_AA_PWM_VR;
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_0, bReg);
		sdRet	= McDevIf_ExecutePacket_AA();
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | psInitInfo->sWaitTime.dVrefRdy2, 0);
	}

	if(McResCtrl_GetAPMode_AA() == eMCDRV_APM_OFF_AA)
	{
		bReg	= MCB_AA_AMPOFF_SP|MCB_AA_AMPOFF_HP|MCB_AA_AMPOFF_RC;
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_AMP, bReg);
	}

	sdRet	= McDevIf_ExecutePacket_AA();
	if (MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	unused path power down	*/
	McResCtrl_GetPowerInfo_AA(&sPowerInfo);
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL_AA;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_ANALOG1_ALL_AA;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_ANALOG2_ALL_AA;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_ANALOG3_ALL_AA;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_ANALOG4_ALL_AA;
	return McPacket_AddPowerDown_AA(&sPowerInfo, &sPowerUpdate);
}

/****************************************************************************
 *	McPacket_AddPowerUp_AA
 *
 *	Description:
 *			Add powerup packet.
 *	Arguments:
 *			psPowerInfo		power information
 *			psPowerUpdate	power update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddPowerUp_AA
(
	const MCDRV_POWER_INFO_AA*		psPowerInfo,
	const MCDRV_POWER_UPDATE_AA*	psPowerUpdate
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	UINT8	bRegCur;
	UINT8	bRegAna1;
	UINT8	bRegAna2;
	UINT32	dUpdate;
	UINT8	bRegChange;
	UINT8	bSpHizReg;
	UINT32	dWaitTime;
	UINT32	dWaitTimeDone	= 0;
	UINT8	bWaitVREFRDY	= 0;
	UINT8	bWaitHPVolUp	= 0;
	UINT8	bWaitSPVolUp	= 0;
	MCDRV_INIT_INFO	sInitInfo;


	McResCtrl_GetInitInfo_AA(&sInitInfo);

	/*	Digital Power	*/
	dUpdate	= ~psPowerInfo->dDigital & psPowerUpdate->dDigital;
	if((dUpdate & MCDRV_POWINFO_DIGITAL_DP0_AA) != 0)
	{
		if((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PWM_DIGITAL) & MCB_AA_PWM_DP0) != 0)
		{/*	DP0 changed	*/
			/*	CKSEL	*/
			if(sInitInfo.bCkSel != MCDRV_CKSEL_CMOS)
			{
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | (UINT32)MCI_AA_CKSEL, MCB_AA_CKSEL);
			}
			else
			{
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | (UINT32)MCI_AA_CKSEL, 0);
			}
			/*	DIVR0, DIVF0	*/
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | (UINT32)MCI_AA_DIVR0, sInitInfo.bDivR0);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | (UINT32)MCI_AA_DIVF0, sInitInfo.bDivF0);
			sdRet	= McDevIf_ExecutePacket_AA();
			if(sdRet != MCDRV_SUCCESS)
			{
				return sdRet;
			}
			/*	Clock Start	*/
			McSrv_ClockStart();
			/*	DP0 up	*/
			bReg	= MCB_AA_PWM_DP2|MCB_AA_PWM_DP1;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL, bReg);
			if(sInitInfo.bCkSel != MCDRV_CKSEL_CMOS)
			{
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | MCDRV_TCXO_WAIT_TIME_AA, 0);
			}
			/*	PLLRST0 up	*/
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PLL_RST, 0);
			/*	2ms wait	*/
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | MCDRV_PLRST_WAIT_TIME_AA, 0);
			/*	DP1/DP2 up	*/
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL, 0);
			/*	DPB/DPDI* up	*/
			bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PWM_DIGITAL_1);
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI0_AA) != 0)
			{
				bReg	&= (UINT8)~MCB_AA_PWM_DPDI0;
			}
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI1_AA) != 0)
			{
				bReg	&= (UINT8)~MCB_AA_PWM_DPDI1;
			}
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI2_AA) != 0)
			{
				bReg	&= (UINT8)~MCB_AA_PWM_DPDI2;
			}
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPPDM_AA) != 0)
			{
				bReg	&= (UINT8)~MCB_AA_PWM_DPPDM;
			}
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPB_AA) != 0)
			{
				bReg	&= (UINT8)~MCB_AA_PWM_DPB;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL_1, bReg);
		}
		else if((dUpdate & MCDRV_POWINFO_DIGITAL_DP2_AA) != 0)
		{
			/*	DP1/DP2 up	*/
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL, 0);
			/*	DPB/DPDI* up	*/
			bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PWM_DIGITAL_1);
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI0_AA) != 0)
			{
				bReg	&= (UINT8)~MCB_AA_PWM_DPDI0;
			}
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI1_AA) != 0)
			{
				bReg	&= (UINT8)~MCB_AA_PWM_DPDI1;
			}
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI2_AA) != 0)
			{
				bReg	&= (UINT8)~MCB_AA_PWM_DPDI2;
			}
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPPDM_AA) != 0)
			{
				bReg	&= (UINT8)~MCB_AA_PWM_DPPDM;
			}
			if((dUpdate & MCDRV_POWINFO_DIGITAL_DPB_AA) != 0)
			{
				bReg	&= (UINT8)~MCB_AA_PWM_DPB;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL_1, bReg);
		}

		/*	DPBDSP	*/
		if((dUpdate & MCDRV_POWINFO_DIGITAL_DPBDSP_AA) != 0)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL_BDSP, 0);
		}
		/*	DPADIF	*/
		if((dUpdate & MCDRV_POWINFO_DIGITAL_DPADIF_AA) != 0)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_DPADIF, 0);
		}
	}

	if(McResCtrl_GetAPMode_AA() == eMCDRV_APM_ON_AA)
	{
		return AddAnalogPowerUpAuto(psPowerInfo, psPowerUpdate);
	}

	/*	Analog Power	*/
	dUpdate	= ~psPowerInfo->abAnalog[0] & psPowerUpdate->abAnalog[0];
	if((dUpdate & MCB_AA_PWM_VR) != 0)
	{
		if((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_0) & MCB_AA_PWM_VR) != 0)
		{/*	AP_VR changed	*/
			bReg	= MCI_AA_PWM_ANALOG_0_DEF;
			if(sInitInfo.bLdo == MCDRV_LDO_ON)
			{
				/*	AP_LDOA	*/
				bReg	&= (UINT8)~MCB_AA_PWM_LDOA;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_PWM_ANALOG_0, bReg);
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | MCDRV_LDOA_WAIT_TIME_AA, 0);
			}
			else
			{
				bReg	&= (UINT8)~MCB_AA_PWM_REFA;
			}
			/*	AP_VR up	*/
			bReg	&= (UINT8)~MCB_AA_PWM_VR;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_PWM_ANALOG_0, bReg);
			dWaitTimeDone	= sInitInfo.sWaitTime.dVrefRdy1;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | dWaitTimeDone, 0);
			bWaitVREFRDY	= 1;
		}

		bReg	= (UINT8)~((UINT8)~psPowerInfo->abAnalog[1] & psPowerUpdate->abAnalog[1]) & McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_1);
		/*	SP_HIZ control	*/
		if(MCDRV_SPHIZ_HIZ == sInitInfo.bSpHiz)
		{
			bSpHizReg	= 0;
			if((bReg & (MCB_AA_PWM_SPL1 | MCB_AA_PWM_SPL2)) != 0)
			{
				bSpHizReg |= MCB_AA_SPL_HIZ;
			}

			if((bReg & (MCB_AA_PWM_SPR1 | MCB_AA_PWM_SPR2)) != 0)
			{
				bSpHizReg |= MCB_AA_SPR_HIZ;
			}

			bSpHizReg |= (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_SP_MODE) & (MCB_AA_SPMN | MCB_AA_SP_SWAP));
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_SP_MODE, bSpHizReg);
		}

		bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_3);
		bReg	= (UINT8)~((UINT8)~psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3]) & bRegCur;
		bRegChange	= bReg ^ bRegCur;
		/*	set DACON and NSMUTE before setting 0 to AP_DA	*/
		if((bRegChange & MCB_AA_PWM_DA) != 0 && (psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3] & MCB_AA_PWM_DA) == 0)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | (UINT32)MCI_AA_DAC_CONFIG, MCB_AA_NSMUTE);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | (UINT32)MCI_AA_DAC_CONFIG, (MCB_AA_DACON | MCB_AA_NSMUTE));
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | (UINT32)MCI_AA_DAC_CONFIG, MCB_AA_DACON);
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_3, bReg);

		bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_4);
		bReg	= (UINT8)~((UINT8)~psPowerInfo->abAnalog[4] & psPowerUpdate->abAnalog[4]) & bRegCur;
		bRegChange	|= (bReg ^ bRegCur);
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_4, bReg);

		if(bWaitVREFRDY != 0)
		{
			/*	wait VREF_RDY	*/
			dWaitTimeDone	= sInitInfo.sWaitTime.dVrefRdy2 - dWaitTimeDone;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | dWaitTimeDone, 0);
		}

		sdRet	= McDevIf_ExecutePacket_AA();
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
		bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_0);
		bReg	= (UINT8)~dUpdate & bRegCur;
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_0, bReg);
		if((bRegCur & MCB_AA_PWM_CP) != 0 && (bReg & MCB_AA_PWM_CP) == 0)
		{/*	AP_CP up	*/
			dWaitTime		= MCDRV_HP_WAIT_TIME_AA;
		}
		else
		{
			dWaitTime	= 0;
		}

		bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_1);
		bRegAna1	= (UINT8)~(~psPowerInfo->abAnalog[1] & psPowerUpdate->abAnalog[1]) & bRegCur;
		if(((bRegCur & MCB_AA_PWM_SPL1) != 0 && (bRegAna1 & MCB_AA_PWM_SPL1) == 0)
		|| ((bRegCur & MCB_AA_PWM_SPR1) != 0 && (bRegAna1 & MCB_AA_PWM_SPR1) == 0))
		{/*	AP_SP* up	*/
			bReg	= bRegAna1|(bRegCur&(UINT8)~(MCB_AA_PWM_SPL1|MCB_AA_PWM_SPR1));
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_1, bReg);
			if(dWaitTime == (UINT32)0)
			{
				dWaitTime		= MCDRV_SP_WAIT_TIME_AA;
				bWaitSPVolUp	= 1;
			}
		}
		if(((bRegCur & MCB_AA_PWM_HPL) != 0 && (bRegAna1 & MCB_AA_PWM_HPL) == 0)
		|| ((bRegCur & MCB_AA_PWM_HPR) != 0 && (bRegAna1 & MCB_AA_PWM_HPR) == 0))
		{
			bWaitHPVolUp	= 1;
		}

		bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_2);
		bRegAna2	= (UINT8)~(~psPowerInfo->abAnalog[2] & psPowerUpdate->abAnalog[2]) & bRegCur;
		if((bRegCur & MCB_AA_PWM_RC1) != 0 && (bRegAna2 & MCB_AA_PWM_RC1) == 0)
		{/*	AP_RC up	*/
			bReg	= bRegAna2|(bRegCur&~MCB_AA_PWM_RC1);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_2, bReg);
			if(dWaitTime == (UINT32)0)
			{
				dWaitTime	= MCDRV_RC_WAIT_TIME_AA;
			}
		}
		if(dWaitTime > (UINT32)0)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | dWaitTime, 0);
			dWaitTimeDone	+= dWaitTime;
		}

		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_1, bRegAna1);
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_2, bRegAna2);

		/*	time wait	*/
		dWaitTime	= GetMaxWait(bRegChange);
		if(dWaitTime > dWaitTimeDone)
		{
			dWaitTime	= dWaitTime - dWaitTimeDone;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | dWaitTime, 0);
			dWaitTimeDone	+= dWaitTime;
		}

		if(bWaitSPVolUp != 0 && sInitInfo.sWaitTime.dSpRdy > dWaitTimeDone)
		{
			dWaitTime	= sInitInfo.sWaitTime.dSpRdy - dWaitTimeDone;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | dWaitTime, 0);
			dWaitTimeDone	+= dWaitTime;
		}
		if(bWaitHPVolUp != 0 && sInitInfo.sWaitTime.dHpRdy > dWaitTimeDone)
		{
			dWaitTime	= sInitInfo.sWaitTime.dHpRdy - dWaitTimeDone;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | dWaitTime, 0);
			dWaitTimeDone	+= dWaitTime;
		}
	}
	return sdRet;
}

/****************************************************************************
 *	AddAnalogPowerUpAuto
 *
 *	Description:
 *			Add analog auto powerup packet.
 *	Arguments:
 *			psPowerInfo		power information
 *			psPowerUpdate	power update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	AddAnalogPowerUpAuto
(
	const MCDRV_POWER_INFO_AA*		psPowerInfo,
	const MCDRV_POWER_UPDATE_AA*	psPowerUpdate
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	UINT8	bRegCur;
	UINT32	dUpdate;
	UINT8	bRegChange;
	UINT8	bSpHizReg;
	MCDRV_INIT_INFO	sInitInfo;
	UINT32	dWaitTime	= 0;


	McResCtrl_GetInitInfo_AA(&sInitInfo);

	/*	Analog Power	*/
	dUpdate	= (UINT32)~psPowerInfo->abAnalog[0] & psPowerUpdate->abAnalog[0];
	if((dUpdate & MCB_AA_PWM_VR) != 0)
	{
		if((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_0) & MCB_AA_PWM_VR) != 0)
		{/*	AP_VR changed	*/
			/*	AP_VR up	*/
			bReg	= MCI_AA_PWM_ANALOG_0_DEF;
			if(sInitInfo.bLdo == MCDRV_LDO_ON)
			{
				/*	AP_LDOA	*/
				bReg	&= (UINT8)~MCB_AA_PWM_LDOA;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_PWM_ANALOG_0, bReg);
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | MCDRV_LDOA_WAIT_TIME_AA, 0);
			}
			else
			{
				bReg	&= (UINT8)~MCB_AA_PWM_REFA;
			}
			bReg	&= (UINT8)~MCB_AA_PWM_VR;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_PWM_ANALOG_0, bReg);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | sInitInfo.sWaitTime.dVrefRdy1, 0);
			if(sInitInfo.sWaitTime.dVrefRdy2 > sInitInfo.sWaitTime.dVrefRdy1)
			{
				dWaitTime	= sInitInfo.sWaitTime.dVrefRdy2 - sInitInfo.sWaitTime.dVrefRdy1;
			}
		}

		bReg	= (UINT8)~((UINT8)~psPowerInfo->abAnalog[1] & psPowerUpdate->abAnalog[1]) & McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_1);
		/*	SP_HIZ control	*/
		if(MCDRV_SPHIZ_HIZ == sInitInfo.bSpHiz)
		{
			bSpHizReg	= 0;
			if((bReg & (MCB_AA_PWM_SPL1 | MCB_AA_PWM_SPL2)) != 0)
			{
				bSpHizReg |= MCB_AA_SPL_HIZ;
			}

			if((bReg & (MCB_AA_PWM_SPR1 | MCB_AA_PWM_SPR2)) != 0)
			{
				bSpHizReg |= MCB_AA_SPR_HIZ;
			}

			bSpHizReg |= (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_SP_MODE) & (MCB_AA_SPMN | MCB_AA_SP_SWAP));
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_SP_MODE, bSpHizReg);
		}

		bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_3);
		bReg	= (UINT8)~((UINT8)~psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3]) & bRegCur;
		bRegChange	= bReg ^ bRegCur;
		/*	set DACON and NSMUTE before setting 0 to AP_DA	*/
		if((bRegChange & MCB_AA_PWM_DA) != 0 && (psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3] & MCB_AA_PWM_DA) == 0)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | (UINT32)MCI_AA_DAC_CONFIG, MCB_AA_NSMUTE);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | (UINT32)MCI_AA_DAC_CONFIG, (MCB_AA_DACON | MCB_AA_NSMUTE));
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | (UINT32)MCI_AA_DAC_CONFIG, MCB_AA_DACON);
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_3, bReg);

		bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_4);
		bReg	= (UINT8)~((UINT8)~psPowerInfo->abAnalog[4] & psPowerUpdate->abAnalog[4]) & bRegCur;
		bRegChange	|= (bReg ^ bRegCur);
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_4, bReg);

		if(dWaitTime > (UINT32)0)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | dWaitTime, 0);
		}

		sdRet	= McDevIf_ExecutePacket_AA();
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
		bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_0);
		bReg	= (UINT8)~dUpdate & bRegCur;
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_0, bReg);

		bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_1);
		bReg	= (UINT8)~((UINT8)~psPowerInfo->abAnalog[1] & psPowerUpdate->abAnalog[1]) & bRegCur;
		if((bRegCur & (MCB_AA_PWM_ADL|MCB_AA_PWM_ADR)) != (bReg & (MCB_AA_PWM_ADL|MCB_AA_PWM_ADR)))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_1, bReg);
		}
		else
		{
			sdRet	= McDevIf_ExecutePacket_AA();
			if(sdRet != MCDRV_SUCCESS)
			{
				return sdRet;
			}
			McResCtrl_SetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_1, bReg);
		}

		bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_2);
		bReg	= (UINT8)~((UINT8)~psPowerInfo->abAnalog[2] & psPowerUpdate->abAnalog[2]) & bRegCur;
		if((bRegCur & (MCB_AA_PWM_LO1L|MCB_AA_PWM_LO1R|MCB_AA_PWM_LO2L|MCB_AA_PWM_LO2R)) != (bReg & (MCB_AA_PWM_LO1L|MCB_AA_PWM_LO1R|MCB_AA_PWM_LO2L|MCB_AA_PWM_LO2R)))
		{
			bReg	= bReg|(bRegCur&(MCB_AA_PWM_RC1|MCB_AA_PWM_RC2));
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_2, bReg);
		}
		else
		{
			sdRet	= McDevIf_ExecutePacket_AA();
			if(sdRet != MCDRV_SUCCESS)
			{
				return sdRet;
			}
			McResCtrl_SetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_2, bReg);
		}

		/*	time wait	*/
		if(dWaitTime < GetMaxWait(bRegChange))
		{
			dWaitTime	= GetMaxWait(bRegChange) - dWaitTime;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | dWaitTime, 0);
		}
	}
	return sdRet;
}

/****************************************************************************
 *	McPacket_AddPowerDown_AA
 *
 *	Description:
 *			Add powerdown packet.
 *	Arguments:
 *			psPowerInfo		power information
 *			psPowerUpdate	power update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddPowerDown_AA
(
	const MCDRV_POWER_INFO_AA*		psPowerInfo,
	const MCDRV_POWER_UPDATE_AA*	psPowerUpdate
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	UINT8	bRegCur;
	UINT32	dUpdate	= psPowerInfo->dDigital & psPowerUpdate->dDigital;
	UINT32	dAPMDoneParam;
	UINT32	dAnaRdyParam;
	UINT8	bSpHizReg;
	MCDRV_INIT_INFO	sInitInfo;


	McResCtrl_GetInitInfo_AA(&sInitInfo);

	if(McResCtrl_GetAPMode_AA() == eMCDRV_APM_ON_AA)
	{
		if(((psPowerInfo->abAnalog[0] & psPowerUpdate->abAnalog[0] & MCB_AA_PWM_VR) != 0
			&& (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_0) & MCB_AA_PWM_VR) == 0))
		{
			/*	wait AP_XX_A	*/
			dAPMDoneParam	= ((MCB_AA_AP_CP_A|MCB_AA_AP_HPL_A|MCB_AA_AP_HPR_A)<<8)
							| (MCB_AA_AP_RC1_A|MCB_AA_AP_RC2_A|MCB_AA_AP_SPL1_A|MCB_AA_AP_SPR1_A|MCB_AA_AP_SPL2_A|MCB_AA_AP_SPR2_A);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_EVTWAIT_AA | MCDRV_EVT_APM_DONE_AA | dAPMDoneParam, 0);
		}
	}

	if((dUpdate & MCDRV_POWINFO_DIGITAL_DP0_AA) != 0
	&& (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PWM_DIGITAL) & MCB_AA_PWM_DP0) == 0)
	{
		/*	wait mute complete	*/
		sdRet	= McDevIf_ExecutePacket_AA();
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_EVTWAIT_AA | MCDRV_EVT_ALLMUTE_AA, 0);
	}

	/*	Analog Power	*/
	bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_1);
	bReg	= (psPowerInfo->abAnalog[1] & psPowerUpdate->abAnalog[1]) | bRegCur;
	if((psPowerUpdate->abAnalog[1] & MCDRV_POWUPDATE_ANALOG1_OUT_AA) != 0 && (MCDRV_SPHIZ_HIZ == sInitInfo.bSpHiz))
	{
		/*	SP_HIZ control	*/
		bSpHizReg	= 0;
		if((bReg & (MCB_AA_PWM_SPL1 | MCB_AA_PWM_SPL2)) != 0)
		{
			bSpHizReg |= MCB_AA_SPL_HIZ;
		}

		if((bReg & (MCB_AA_PWM_SPR1 | MCB_AA_PWM_SPR2)) != 0)
		{
			bSpHizReg |= MCB_AA_SPR_HIZ;
		}

		bSpHizReg |= (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_SP_MODE) & (MCB_AA_SPMN | MCB_AA_SP_SWAP));
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_SP_MODE, bSpHizReg);
	}

	if(McResCtrl_GetAPMode_AA() == eMCDRV_APM_ON_AA)
	{
		if((bRegCur & (MCB_AA_PWM_ADL|MCB_AA_PWM_ADR)) != (bReg & (MCB_AA_PWM_ADL|MCB_AA_PWM_ADR)))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_1, bReg);
		}
		else
		{
			sdRet	= McDevIf_ExecutePacket_AA();
			if(sdRet != MCDRV_SUCCESS)
			{
				return sdRet;
			}
			McResCtrl_SetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_1, bReg);
		}
	}
	else
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_PWM_ANALOG_1, bReg);
	}

	bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_2);
	bReg	= (psPowerInfo->abAnalog[2] & psPowerUpdate->abAnalog[2]) | bRegCur;
	if(McResCtrl_GetAPMode_AA() == eMCDRV_APM_ON_AA)
	{
		if((bRegCur & (MCB_AA_PWM_LO1L|MCB_AA_PWM_LO1R|MCB_AA_PWM_LO2L|MCB_AA_PWM_LO2R)) != (bReg & (MCB_AA_PWM_LO1L|MCB_AA_PWM_LO1R|MCB_AA_PWM_LO2L|MCB_AA_PWM_LO2R)))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_2, bReg);
		}
		else
		{
			sdRet	= McDevIf_ExecutePacket_AA();
			if(sdRet != MCDRV_SUCCESS)
			{
				return sdRet;
			}
			McResCtrl_SetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_2, bReg);
		}
	}
	else
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_PWM_ANALOG_2, bReg);
	}

	bReg	= (UINT8)(psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3]) | McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_3);
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_3, bReg);
	bReg	= (UINT8)(psPowerInfo->abAnalog[4] & psPowerUpdate->abAnalog[4]) | McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_4);
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_PWM_ANALOG_4, bReg);

	/*	set DACON and NSMUTE after setting 1 to AP_DA	*/
	if((psPowerInfo->abAnalog[3] & psPowerUpdate->abAnalog[3] & MCB_AA_PWM_DA) != 0)
	{
		if((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_CODEC_AA, MCI_AA_DAC_CONFIG) & MCB_AA_DACON) == MCB_AA_DACON)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_EVTWAIT_AA | MCDRV_EVT_DACMUTE_AA | (UINT32)((MCB_AA_DAC_FLAGL<<8)|MCB_AA_DAC_FLAGR), 0);
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | (UINT32)MCI_AA_DAC_CONFIG, MCB_AA_NSMUTE);
	}

	bRegCur	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_PWM_ANALOG_0);
	bReg	= psPowerInfo->abAnalog[0] & psPowerUpdate->abAnalog[0];
	if(McResCtrl_GetAPMode_AA() == eMCDRV_APM_OFF_AA)
	{
		/*	wait CPPDRDY	*/
		dAnaRdyParam	= 0;
		if((bRegCur & MCB_AA_PWM_CP) == 0 && (bReg & MCB_AA_PWM_CP) == MCB_AA_PWM_CP)
		{
			dAnaRdyParam	= MCB_AA_CPPDRDY;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_EVTWAIT_AA | MCDRV_EVT_ANA_RDY_AA | dAnaRdyParam, 0);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_PWM_ANALOG_0, (bRegCur|MCB_AA_PWM_CP));
		}
	}

	if((bReg & MCB_AA_PWM_VR) != 0 && (bRegCur & MCB_AA_PWM_VR) == 0)
	{/*	AP_VR changed	*/
		/*	AP_xx down	*/
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_PWM_ANALOG_0, MCI_AA_PWM_ANALOG_0_DEF);
	}
	else
	{
		bReg	|= bRegCur;
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_PWM_ANALOG_0, bReg);
	}


	/*	Digital Power	*/
	if((dUpdate & MCDRV_POWINFO_DIGITAL_DPADIF_AA) != 0
	&& McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_CODEC_AA, MCI_AA_DPADIF) != MCB_AA_DPADIF)
	{
		/*	AD_MUTE	*/
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_AD_MUTE, MCB_AA_AD_MUTE);
		/*	DPADIF	*/
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_DPADIF, MCB_AA_DPADIF);
	}

	if((dUpdate & MCDRV_POWINFO_DIGITAL_DPPDM_AA) != 0
	&& (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PWM_DIGITAL_1) & MCB_AA_PWM_DPPDM) == 0)
	{
		/*	PDM_MUTE	*/
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM_MUTE, MCB_AA_PDM_MUTE);
	}

	if((dUpdate & MCDRV_POWINFO_DIGITAL_DPBDSP_AA) != 0)
	{
		/*	DPBDSP	*/
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL_BDSP, MCB_AA_PWM_DPBDSP);
	}

	/*	DPDI*, DPPDM	*/
	bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PWM_DIGITAL_1);
	if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI0_AA) != 0 || (dUpdate & MCDRV_POWINFO_DIGITAL_DP2_AA) != 0)
	{
		bReg |= MCB_AA_PWM_DPDI0;
	}
	if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI1_AA) != 0 || (dUpdate & MCDRV_POWINFO_DIGITAL_DP2_AA) != 0)
	{
		bReg |= MCB_AA_PWM_DPDI1;
	}
	if((dUpdate & MCDRV_POWINFO_DIGITAL_DPDI2_AA) != 0 || (dUpdate & MCDRV_POWINFO_DIGITAL_DP2_AA) != 0)
	{
		bReg |= MCB_AA_PWM_DPDI2;
	}
	if((dUpdate & MCDRV_POWINFO_DIGITAL_DPPDM_AA) != 0)
	{
		bReg |= MCB_AA_PWM_DPPDM;
	}
	if(bReg != 0)
	{
		/*	cannot set DP* & DPB at the same time	*/
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL_1, bReg);
	}
	/*	DPB	*/
	if((dUpdate & MCDRV_POWINFO_DIGITAL_DPB_AA) != 0)
	{
		bReg |= MCB_AA_PWM_DPB;
	}
	if(bReg != 0)
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL_1, bReg);
	}

	if((dUpdate & MCDRV_POWINFO_DIGITAL_DP2_AA) != 0
	&& (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PWM_DIGITAL) & MCB_AA_PWM_DP2) == 0)
	{
		if((dUpdate & MCDRV_POWINFO_DIGITAL_DP0_AA) != 0)
		{
			/*	DP2, DP1	*/
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL, (MCB_AA_PWM_DP2 | MCB_AA_PWM_DP1));
			if((dUpdate & MCDRV_POWINFO_DIGITAL_PLLRST0_AA) != 0)
			{
				/*	PLLRST0	*/
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PLL_RST, MCB_AA_PLLRST0);
			}
			/*	DP0	*/
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL, (MCB_AA_PWM_DP2 | MCB_AA_PWM_DP1 | MCB_AA_PWM_DP0));
			sdRet	= McDevIf_ExecutePacket_AA();
			if(sdRet != MCDRV_SUCCESS)
			{
				return sdRet;
			}
			McSrv_ClockStop();
		}
		else
		{
			/*	DP2	*/
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PWM_DIGITAL, MCB_AA_PWM_DP2);
		}
	}
	return sdRet;
}

/****************************************************************************
 *	McPacket_AddPathSet_AA
 *
 *	Description:
 *			Add path update packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddPathSet_AA
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;

	/*	DI Pad	*/
	AddDIPad();

	/*	PAD(PDM)	*/
	AddPAD();

	/*	Digital Mixer Source	*/
	sdRet	= AddSource();
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}

	/*	DIR*_START, DIT*_START	*/
	AddDIStart();

	return sdRet;
}

/****************************************************************************
 *	AddDIPad
 *
 *	Description:
 *			Add DI Pad setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDIPad
(
	void
)
{
	UINT8	bReg;
	UINT8	bIsUsedDIR[3];
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_PATH_INFO	sPathInfo;
	MCDRV_DIO_INFO	sDioInfo;

	McResCtrl_GetInitInfo_AA(&sInitInfo);
	McResCtrl_GetPathInfo_AA(&sPathInfo);
	McResCtrl_GetDioInfo_AA(&sDioInfo);

	/*	SDIN_MSK2/1	*/
	bReg	= 0;
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR2_AA) == 0)
	{
		bReg |= MCB_AA_SDIN_MSK2;
		bIsUsedDIR[2]	= 0;
	}
	else
	{
		bIsUsedDIR[2]	= 1;
	}
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR1_AA) == 0)
	{
		bReg |= MCB_AA_SDIN_MSK1;
		bIsUsedDIR[1]	= 0;
	}
	else
	{
		bIsUsedDIR[1]	= 1;
	}
	/*	SDO_DDR2/1	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_DIT2_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		if(sInitInfo.bDioSdo2Hiz == MCDRV_DAHIZ_LOW)
		{
			bReg |= MCB_AA_SDO_DDR2;
		}
	}
	else
	{
		bReg |= MCB_AA_SDO_DDR2;
	}
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_DIT1_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		if(sInitInfo.bDioSdo1Hiz == MCDRV_DAHIZ_LOW)
		{
			bReg |= MCB_AA_SDO_DDR1;
		}
	}
	else
	{
		bReg |= MCB_AA_SDO_DDR1;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | (UINT32)MCI_AA_SD_MSK, bReg);

	/*	SDIN_MSK0	*/
	bReg	= 0;
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR0_AA) == 0)
	{
		bReg |= MCB_AA_SDIN_MSK0;
		bIsUsedDIR[0]	= 0;
	}
	else
	{
		bIsUsedDIR[0]	= 1;
	}
	/*	SDO_DDR0	*/
	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_DIT0_AA, eMCDRV_DST_CH0_AA) == 0)
	{
		if(sInitInfo.bDioSdo0Hiz == MCDRV_DAHIZ_LOW)
		{
			bReg |= MCB_AA_SDO_DDR0;
		}
	}
	else
	{
		bReg |= MCB_AA_SDO_DDR0;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | (UINT32)MCI_AA_SD_MSK_1, bReg);

	/*	BCLK_MSK*, BCLD_DDR*, LRCK_MSK*, LRCK_DDR*	*/
	bReg	= 0;
	if(bIsUsedDIR[2] == 0 && McResCtrl_GetDITSource_AA(eMCDRV_DIO_2_AA) == eMCDRV_SRC_NONE_AA)
	{
		bReg |= MCB_AA_BCLK_MSK2;
		bReg |= MCB_AA_LRCK_MSK2;
		if(sInitInfo.bDioClk2Hiz == MCDRV_DAHIZ_LOW)
		{
			bReg |= MCB_AA_BCLK_DDR2;
			bReg |= MCB_AA_LRCK_DDR2;
		}
	}
	else
	{
		if(sDioInfo.asPortInfo[2].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_AA_BCLK_DDR2;
			bReg |= MCB_AA_LRCK_DDR2;
		}
	}
	if(bIsUsedDIR[1] == 0 && McResCtrl_GetDITSource_AA(eMCDRV_DIO_1_AA) == eMCDRV_SRC_NONE_AA)
	{
		bReg |= MCB_AA_BCLK_MSK1;
		bReg |= MCB_AA_LRCK_MSK1;
		if(sInitInfo.bDioClk1Hiz == MCDRV_DAHIZ_LOW)
		{
			bReg |= MCB_AA_BCLK_DDR1;
			bReg |= MCB_AA_LRCK_DDR1;
		}
	}
	else
	{
		if(sDioInfo.asPortInfo[1].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_AA_BCLK_DDR1;
			bReg |= MCB_AA_LRCK_DDR1;
		}
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_BCLK_MSK, bReg);

	/*	BCLK_MSK*, BCLD_DDR*, LRCK_MSK*, LRCK_DDR*, PCM_HIZ*	*/
	bReg	= 0;
	if(bIsUsedDIR[0] == 0 && McResCtrl_GetDITSource_AA(eMCDRV_DIO_0_AA) == eMCDRV_SRC_NONE_AA)
	{
		bReg |= MCB_AA_BCLK_MSK0;
		bReg |= MCB_AA_LRCK_MSK0;
		if(sInitInfo.bDioClk0Hiz == MCDRV_DAHIZ_LOW)
		{
			bReg |= MCB_AA_BCLK_DDR0;
			bReg |= MCB_AA_LRCK_DDR0;
		}
	}
	else
	{
		if(sDioInfo.asPortInfo[0].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_AA_BCLK_DDR0;
			bReg |= MCB_AA_LRCK_DDR0;
		}
	}
	if(sInitInfo.bPcmHiz == MCDRV_PCMHIZ_HIZ)
	{
		bReg |= (MCB_AA_PCMOUT_HIZ2 | MCB_AA_PCMOUT_HIZ1 | MCB_AA_PCMOUT_HIZ0);
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_BCLK_MSK_1, bReg);
}

/****************************************************************************
 *	AddPAD
 *
 *	Description:
 *			Add PAD setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddPAD
(
	void
)
{
	UINT8	bReg;
	MCDRV_INIT_INFO	sInitInfo;

	McResCtrl_GetInitInfo_AA(&sInitInfo);

	/*	PA*_MSK	*/
	bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PA_MSK_1);
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_PDM_AA) == 0)
	{
		bReg	|= MCB_AA_PA0_MSK;
		if(sInitInfo.bPad1Func == MCDRV_PAD_PDMDI)
		{
			bReg	|= MCB_AA_PA1_MSK;
		}
	}
	else
	{
		bReg	&= (UINT8)~MCB_AA_PA0_MSK;
		if(sInitInfo.bPad1Func == MCDRV_PAD_PDMDI)
		{
			bReg	&= (UINT8)~MCB_AA_PA1_MSK;
		}
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PA_MSK_1, bReg);
}

/****************************************************************************
 *	AddSource
 *
 *	Description:
 *			Add source setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	AddSource
(
	void
)
{
	SINT32	sdRet			= MCDRV_SUCCESS;
	UINT8	bReg;
	UINT8	bAEng6			= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_AENG6_SOURCE);
	UINT8	bRegAESource	= 0;
	UINT8	bAESourceChange	= 0;
	UINT32	dXFadeParam		= 0;
	MCDRV_SRC_TYPE_AA	eAESource	= McResCtrl_GetAESource_AA();
	MCDRV_PATH_INFO	sPathInfo;
	MCDRV_DAC_INFO	sDacInfo;
	MCDRV_AE_INFO	sAeInfo;

	McResCtrl_GetPathInfo_AA(&sPathInfo);
	McResCtrl_GetAeInfo_AA(&sAeInfo);

	switch(eAESource)
	{
	case	eMCDRV_SRC_PDM_AA:		bRegAESource	= MCB_AA_AE_SOURCE_AD;		bAEng6	= MCB_AA_AENG6_PDM;	break;
	case	eMCDRV_SRC_ADC0_AA:	bRegAESource	= MCB_AA_AE_SOURCE_AD;		bAEng6	= MCB_AA_AENG6_ADC0;	break;
	case	eMCDRV_SRC_DIR0_AA:	bRegAESource	= MCB_AA_AE_SOURCE_DIR0;	break;
	case	eMCDRV_SRC_DIR1_AA:	bRegAESource	= MCB_AA_AE_SOURCE_DIR1;	break;
	case	eMCDRV_SRC_DIR2_AA:	bRegAESource	= MCB_AA_AE_SOURCE_DIR2;	break;
	case	eMCDRV_SRC_MIX_AA:		bRegAESource	= MCB_AA_AE_SOURCE_MIX;	break;
	default:					bRegAESource	= 0;
	}
	if(bRegAESource != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_SRC_SOURCE_1)&0xF0))
	{
		/*	xxx_INS	*/
		dXFadeParam	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DAC_INS);
		dXFadeParam <<= 8;
		dXFadeParam	|= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_INS);
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)MCI_AA_DAC_INS, 0);
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)MCI_AA_INS, 0);
		bAESourceChange	= 1;
		sdRet	= McDevIf_ExecutePacket_AA();
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
	}

	McResCtrl_GetDacInfo_AA(&sDacInfo);

	/*	DAC_SOURCE/VOICE_SOURCE	*/
	bReg	= 0;
	switch(McResCtrl_GetDACSource_AA(eMCDRV_DAC_MASTER_AA))
	{
	case	eMCDRV_SRC_PDM_AA:
		bReg |= MCB_AA_DAC_SOURCE_AD;
		bAEng6	= MCB_AA_AENG6_PDM;
		break;
	case	eMCDRV_SRC_ADC0_AA:
		bReg |= MCB_AA_DAC_SOURCE_AD;
		bAEng6	= MCB_AA_AENG6_ADC0;
		break;
	case	eMCDRV_SRC_DIR0_AA:
		bReg |= MCB_AA_DAC_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1_AA:
		bReg |= MCB_AA_DAC_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2_AA:
		bReg |= MCB_AA_DAC_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX_AA:
		bReg |= MCB_AA_DAC_SOURCE_MIX;
		break;
	default:
		break;
	}
	switch(McResCtrl_GetDACSource_AA(eMCDRV_DAC_VOICE_AA))
	{
	case	eMCDRV_SRC_PDM_AA:
		bReg |= MCB_AA_VOICE_SOURCE_AD;
		bAEng6	= MCB_AA_AENG6_PDM;
		break;
	case	eMCDRV_SRC_ADC0_AA:
		bReg |= MCB_AA_VOICE_SOURCE_AD;
		bAEng6	= MCB_AA_AENG6_ADC0;
		break;
	case	eMCDRV_SRC_DIR0_AA:
		bReg |= MCB_AA_VOICE_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1_AA:
		bReg |= MCB_AA_VOICE_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2_AA:
		bReg |= MCB_AA_VOICE_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX_AA:
		bReg |= MCB_AA_VOICE_SOURCE_MIX;
		break;
	default:
		break;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_SOURCE, bReg);

	/*	SWP/VOICE_SWP	*/
	bReg	= (sDacInfo.bMasterSwap << 4) | sDacInfo.bVoiceSwap;
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_SWP, bReg);

	/*	DIT0SRC_SOURCE/DIT1SRC_SOURCE	*/
	bReg	= 0;
	switch(McResCtrl_GetDITSource_AA(eMCDRV_DIO_0_AA))
	{
	case	eMCDRV_SRC_PDM_AA:
		bReg |= MCB_AA_DIT0_SOURCE_AD;
			bAEng6	= MCB_AA_AENG6_PDM;
		break;
	case	eMCDRV_SRC_ADC0_AA:
		bReg |= MCB_AA_DIT0_SOURCE_AD;
			bAEng6	= MCB_AA_AENG6_ADC0;
		break;
	case	eMCDRV_SRC_DIR0_AA:
		bReg |= MCB_AA_DIT0_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1_AA:
		bReg |= MCB_AA_DIT0_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2_AA:
		bReg |= MCB_AA_DIT0_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX_AA:
		bReg |= MCB_AA_DIT0_SOURCE_MIX;
		break;
	default:
		break;
	}
	switch(McResCtrl_GetDITSource_AA(eMCDRV_DIO_1_AA))
	{
	case	eMCDRV_SRC_PDM_AA:
		bReg |= MCB_AA_DIT1_SOURCE_AD;
			bAEng6	= MCB_AA_AENG6_PDM;
		break;
	case	eMCDRV_SRC_ADC0_AA:
		bReg |= MCB_AA_DIT1_SOURCE_AD;
			bAEng6	= MCB_AA_AENG6_ADC0;
		break;
	case	eMCDRV_SRC_DIR0_AA:
		bReg |= MCB_AA_DIT1_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1_AA:
		bReg |= MCB_AA_DIT1_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2_AA:
		bReg |= MCB_AA_DIT1_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX_AA:
		bReg |= MCB_AA_DIT1_SOURCE_MIX;
		break;
	default:
		break;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_SRC_SOURCE, bReg);

	/*	AE_SOURCE/DIT2SRC_SOURCE	*/
	bReg	= bRegAESource;
	switch(McResCtrl_GetDITSource_AA(eMCDRV_DIO_2_AA))
	{
	case	eMCDRV_SRC_PDM_AA:
		bReg |= MCB_AA_DIT2_SOURCE_AD;
			bAEng6	= MCB_AA_AENG6_PDM;
		break;
	case	eMCDRV_SRC_ADC0_AA:
		bReg |= MCB_AA_DIT2_SOURCE_AD;
			bAEng6	= MCB_AA_AENG6_ADC0;
		break;
	case	eMCDRV_SRC_DIR0_AA:
		bReg |= MCB_AA_DIT2_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1_AA:
		bReg |= MCB_AA_DIT2_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2_AA:
		bReg |= MCB_AA_DIT2_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX_AA:
		bReg |= MCB_AA_DIT2_SOURCE_MIX;
		break;
	default:
		break;
	}
	if(bAESourceChange != 0)
	{
		/*	wait xfade complete	*/
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_EVTWAIT_AA | MCDRV_EVT_INSFLG_AA | dXFadeParam, 0);
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_SRC_SOURCE_1, bReg);

	/*	BDSP_ST	*/
	if(McResCtrl_GetAESource_AA() == eMCDRV_SRC_NONE_AA)
	{/*	AE is unused	*/
		/*	BDSP stop & reset	*/
		if((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_A_AA, MCI_AA_BDSP_ST)&MCB_AA_BDSP_ST) != 0)
		{
			bReg	= 0;
			if((sAeInfo.bOnOff & MCDRV_EQ5_ON) != 0)
			{
				bReg |= MCB_AA_EQ5ON;
			}
			if((sAeInfo.bOnOff & MCDRV_DRC_ON) != 0)
			{
				bReg |= MCB_AA_DRCON;
			}
			if((sAeInfo.bOnOff & MCDRV_EQ3_ON) != 0)
			{
				bReg |= MCB_AA_EQ3ON;
			}
			if(McDevProf_IsValid(eMCDRV_FUNC_DBEX) == 1)
			{
				if((sAeInfo.bOnOff & MCDRV_BEXWIDE_ON) != 0)
				{
					bReg |= MCB_AA_DBEXON;
				}
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | MCI_AA_BDSP_ST, bReg);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | MCI_AA_BDSP_RST, MCB_AA_TRAM_RST);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | MCI_AA_BDSP_RST, 0);
		}
	}
	else
	{/*	AE is used	*/
		bReg	= 0;
		if((sAeInfo.bOnOff & MCDRV_EQ5_ON) != 0)
		{
			bReg |= MCB_AA_EQ5ON;
		}
		if((sAeInfo.bOnOff & MCDRV_DRC_ON) != 0)
		{
			bReg |= MCB_AA_DRCON;
			bReg |= MCB_AA_BDSP_ST;
		}
		if((sAeInfo.bOnOff & MCDRV_EQ3_ON) != 0)
		{
			bReg |= MCB_AA_EQ3ON;
		}
		if(McDevProf_IsValid(eMCDRV_FUNC_DBEX) == 1)
		{
			if((sAeInfo.bOnOff & MCDRV_BEXWIDE_ON) != 0)
			{
				bReg |= MCB_AA_DBEXON;
				bReg |= MCB_AA_BDSP_ST;
			}
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | MCI_AA_BDSP_ST, bReg);
	}

	/*	check MIX SOURCE for AENG6_SOURCE	*/
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_PDM_AA) != 0)
	{
		bAEng6	= MCB_AA_AENG6_PDM;
	}
	else if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_ADC0_AA) != 0)
	{
		bAEng6	= MCB_AA_AENG6_ADC0;
	}

	/*	AENG6_SOURCE	*/
	if(bAEng6 != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_AENG6_SOURCE))
	{
		bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_CODEC_AA, MCI_AA_AD_START);
		if((bReg & MCB_AA_AD_START) != 0)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_AD_MUTE, MCB_AA_AD_MUTE);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_AD_START, bReg&(UINT8)~MCB_AA_AD_START);
		}
		bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PDM_START);
		if((bReg & MCB_AA_PDM_START) != 0)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM_MUTE, MCB_AA_PDM_MUTE);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM_START, bReg&(UINT8)~MCB_AA_PDM_START);
		}
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_AENG6_SOURCE, bAEng6);

	/*	xxx_INS	*/
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_AE_AA) != 0)
	{
		switch(eAESource)
		{
		case	eMCDRV_SRC_PDM_AA:
		case	eMCDRV_SRC_ADC0_AA:
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)MCI_AA_INS, MCB_AA_ADC_INS);
			break;
		case	eMCDRV_SRC_DIR0_AA:
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)MCI_AA_INS, MCB_AA_DIR0_INS);
			break;
		case	eMCDRV_SRC_DIR1_AA:
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)MCI_AA_INS, MCB_AA_DIR1_INS);
			break;
		case	eMCDRV_SRC_DIR2_AA:
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)MCI_AA_INS, MCB_AA_DIR2_INS);
			break;
		case	eMCDRV_SRC_MIX_AA:
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)MCI_AA_DAC_INS, MCB_AA_DAC_INS);
			break;
		default:
			break;
		}
	}

	return sdRet;
}

/****************************************************************************
 *	AddDIStart
 *
 *	Description:
 *			Add DIStart setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDIStart
(
	void
)
{
	UINT8	bReg;
	MCDRV_PATH_INFO	sPathInfo;
	MCDRV_DIO_INFO	sDioInfo;

	McResCtrl_GetPathInfo_AA(&sPathInfo);
	McResCtrl_GetDioInfo_AA(&sDioInfo);

	/*	DIR*_START, DIT*_START	*/
	bReg	= 0;
	if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_0_AA) != eMCDRV_SRC_NONE_AA)
	{
		if(sDioInfo.asPortInfo[0].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_AA_DITIM0_START;
		}
		bReg |= MCB_AA_DIT0_SRC_START;
		bReg |= MCB_AA_DIT0_START;
	}
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR0_AA) != 0)
	{
		if(sDioInfo.asPortInfo[0].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_AA_DITIM0_START;
		}
		bReg |= MCB_AA_DIR0_SRC_START;
		bReg |= MCB_AA_DIR0_START;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIX0_START, bReg);

	bReg	= 0;
	if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_1_AA) != eMCDRV_SRC_NONE_AA)
	{
		if(sDioInfo.asPortInfo[1].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_AA_DITIM1_START;
		}
		bReg |= MCB_AA_DIT1_SRC_START;
		bReg |= MCB_AA_DIT1_START;
	}
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR1_AA) != 0)
	{
		if(sDioInfo.asPortInfo[1].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_AA_DITIM1_START;
		}
		bReg |= MCB_AA_DIR1_SRC_START;
		bReg |= MCB_AA_DIR1_START;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIX1_START, bReg);

	bReg	= 0;
	if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_2_AA) != eMCDRV_SRC_NONE_AA)
	{
		if(sDioInfo.asPortInfo[2].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_AA_DITIM2_START;
		}
		bReg |= MCB_AA_DIT2_SRC_START;
		bReg |= MCB_AA_DIT2_START;
	}
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR2_AA) != 0)
	{
		if(sDioInfo.asPortInfo[2].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER)
		{
			bReg |= MCB_AA_DITIM2_START;
		}
		bReg |= MCB_AA_DIR2_SRC_START;
		bReg |= MCB_AA_DIR2_START;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIX2_START, bReg);
}

/****************************************************************************
 *	McPacket_AddMixSet_AA
 *
 *	Description:
 *			Add analog mixer set packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
SINT32	McPacket_AddMixSet_AA
(
	void
)
{
	UINT8	bReg;
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_PATH_INFO	sPathInfo;

	McResCtrl_GetInitInfo_AA(&sInitInfo);
	McResCtrl_GetPathInfo_AA(&sPathInfo);

	/*	ADL_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asAdc0[0]);
	if((sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
	|| (sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_LI1MIX;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_ADL_MIX, bReg);
	/*	ADL_MONO	*/
	bReg	= 0;
	if((sPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_MONO_LI1;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_ADL_MONO, bReg);

	/*	ADR_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asAdc0[1]);
	if((sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON
	|| (sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_LI1MIX;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_ADR_MIX, bReg);
	/*	ADR_MONO	*/
	bReg	= 0;
	if((sPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_MONO_LI1;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_ADR_MONO, bReg);

	/*	L1L_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asLout1[0]);
	if((sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
	|| (sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_LI1MIX;
	}
	if((sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
	|| (sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_AA_DAMIX;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LO1L_MIX, bReg);
	/*	L1L_MONO	*/
	bReg	= 0;
	if((sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_MONO_LI1;
	}
	if((sPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_AA_MONO_DA;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LO1L_MONO, bReg);

	/*	L1R_MIX	*/
	if(sInitInfo.bLineOut1Dif != MCDRV_LINE_DIF)
	{
		bReg	= GetMicMixBit(&sPathInfo.asLout1[1]);
		if((sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON)
		{
			bReg |= MCB_AA_LI1MIX;
		}
		if((sPathInfo.asLout1[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
		{
			bReg |= MCB_AA_DAMIX;
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_LO1R_MIX, bReg);
	}

	/*	L2L_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asLout2[0]);
	if((sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
	|| (sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_LI1MIX;
	}
	if((sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
	|| (sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_AA_DAMIX;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LO2L_MIX, bReg);
	/*	L2L_MONO	*/
	bReg	= 0;
	if((sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_MONO_LI1;
	}
	if((sPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_AA_MONO_DA;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LO2L_MONO, bReg);

	/*	L2R_MIX	*/
	if(sInitInfo.bLineOut2Dif != MCDRV_LINE_DIF)
	{
		bReg	= GetMicMixBit(&sPathInfo.asLout2[1]);
		if((sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON)
		{
			bReg |= MCB_AA_LI1MIX;
		}
		if((sPathInfo.asLout2[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
		{
			bReg |= MCB_AA_DAMIX;
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_LO2R_MIX, bReg);
	}

	/*	HPL_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asHpOut[0]);
	if((sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
	|| (sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_LI1MIX;
	}
	if((sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
	|| (sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_AA_DAMIX;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_HPL_MIX, bReg);
	/*	HPL_MONO	*/
	bReg	= 0;
	if((sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_MONO_LI1;
	}
	if((sPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_AA_MONO_DA;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_HPL_MONO, bReg);

	/*	HPR_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asHpOut[1]);
	if((sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON)
	{
		bReg |= MCB_AA_LI1MIX;
	}
	if((sPathInfo.asHpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
	{
		bReg |= MCB_AA_DAMIX;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_HPR_MIX, bReg);

	/*	SPL_MIX	*/
	bReg	= 0;
	if((sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & MCDRV_SRC1_LINE1_L_ON) == MCDRV_SRC1_LINE1_L_ON
	|| (sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_LI1MIX;
	}
	if((sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON
	|| (sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_AA_DAMIX;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_SPL_MIX, bReg);
	/*	SPL_MONO	*/
	bReg	= 0;
	if((sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_MONO_LI1;
	}
	if((sPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_AA_MONO_DA;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_SPL_MONO, bReg);

	/*	SPR_MIX	*/
	bReg	= 0;
	if((sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & MCDRV_SRC1_LINE1_R_ON) == MCDRV_SRC1_LINE1_R_ON
	|| (sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_LI1MIX;
	}
	if((sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON
	|| (sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_AA_DAMIX;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_SPR_MIX, bReg);
	/*	SPR_MONO	*/
	bReg	= 0;
	if((sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_MONO_LI1;
	}
	if((sPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & MCDRV_SRC5_DAC_M_ON) == MCDRV_SRC5_DAC_M_ON)
	{
		bReg |= MCB_AA_MONO_DA;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_SPR_MONO, bReg);

	/*	RCV_MIX	*/
	bReg	= GetMicMixBit(&sPathInfo.asRcOut[0]);
	if((sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & MCDRV_SRC1_LINE1_M_ON) == MCDRV_SRC1_LINE1_M_ON)
	{
		bReg |= MCB_AA_LI1MIX;
	}
	if((sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & MCDRV_SRC5_DAC_L_ON) == MCDRV_SRC5_DAC_L_ON)
	{
		bReg |= MCB_AA_DALMIX;
	}
	if((sPathInfo.asRcOut[0].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & MCDRV_SRC5_DAC_R_ON) == MCDRV_SRC5_DAC_R_ON)
	{
		bReg |= MCB_AA_DARMIX;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_RC_MIX, bReg);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	GetMicMixBit
 *
 *	Description:
 *			Get mic mixer bit.
 *	Arguments:
 *			source info
 *	Return:
 *			mic mixer bit
 *
 ****************************************************************************/
static UINT8	GetMicMixBit
(
	const MCDRV_CHANNEL* psChannel
)
{
	UINT8	bMicMix	= 0;

	if((psChannel->abSrcOnOff[MCDRV_SRC_MIC1_BLOCK] & MCDRV_SRC0_MIC1_ON) == MCDRV_SRC0_MIC1_ON)
	{
		bMicMix |= MCB_AA_M1MIX;
	}
	if((psChannel->abSrcOnOff[MCDRV_SRC_MIC2_BLOCK] & MCDRV_SRC0_MIC2_ON) == MCDRV_SRC0_MIC2_ON)
	{
		bMicMix |= MCB_AA_M2MIX;
	}
	if((psChannel->abSrcOnOff[MCDRV_SRC_MIC3_BLOCK] & MCDRV_SRC0_MIC3_ON) == MCDRV_SRC0_MIC3_ON)
	{
		bMicMix |= MCB_AA_M3MIX;
	}
	return bMicMix;
}

/****************************************************************************
 *	McPacket_AddStart_AA
 *
 *	Description:
 *			Add start packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddStart_AA
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	UINT8	bADStart	= 0;
	UINT8	bPDMStart	= 0;
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_PATH_INFO	sPathInfo;
	MCDRV_ADC_INFO	sAdcInfo;
	MCDRV_PDM_INFO	sPdmInfo;

	McResCtrl_GetInitInfo_AA(&sInitInfo);
	McResCtrl_GetPathInfo_AA(&sPathInfo);
	McResCtrl_GetAdcInfo_AA(&sAdcInfo);
	McResCtrl_GetPdmInfo_AA(&sPdmInfo);

	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_ADC0_AA, eMCDRV_DST_CH0_AA) == 1
	|| McResCtrl_IsDstUsed_AA(eMCDRV_DST_ADC0_AA, eMCDRV_DST_CH1_AA) == 1)
	{/*	ADC0 source is used	*/
		if((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_CODEC_AA, MCI_AA_AD_START) & MCB_AA_AD_START) == 0)
		{
			bReg	= (sAdcInfo.bAgcOn << 2) | sAdcInfo.bAgcAdjust;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_AD_AGC, bReg);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_AD_MUTE, MCB_AA_AD_MUTE);
			bReg	= (sAdcInfo.bMono << 1) | MCB_AA_AD_START;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_AD_START, bReg);
			bADStart	= 1;
		}
	}
	else if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_PDM_AA) != 0)
	{/*	PDM is used	*/
		if((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PDM_START) & MCB_AA_PDM_START) == 0)
		{
			bReg	= (sPdmInfo.bAgcOn << 2) | sPdmInfo.bAgcAdjust;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM_AGC, bReg);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM_MUTE, MCB_AA_PDM_MUTE);
			bReg	= (sPdmInfo.bMono << 1) | MCB_AA_PDM_START;
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM_START, bReg);
			bPDMStart	= 1;
		}
	}

	if(bADStart == 1 || bPDMStart == 1)
	{
		sdRet	= McDevIf_ExecutePacket_AA();
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_TIMWAIT_AA | sInitInfo.sWaitTime.dAdHpf, 0);
		if(bADStart == 1)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_AD_MUTE, 0);
		}
		else
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM_MUTE, 0);
		}
	}
	return sdRet;
}

/****************************************************************************
 *	McPacket_AddStop_AA
 *
 *	Description:
 *			Add stop packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
SINT32	McPacket_AddStop_AA
(
	void
)
{
	UINT8	bReg;

	bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIX0_START);
	if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_0_AA) == eMCDRV_SRC_NONE_AA)
	{/*	DIT is unused	*/
		bReg &= (UINT8)~MCB_AA_DIT0_SRC_START;
		bReg &= (UINT8)~MCB_AA_DIT0_START;
	}
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR0_AA) == 0)
	{/*	DIR is unused	*/
		bReg &= (UINT8)~MCB_AA_DIR0_SRC_START;
		bReg &= (UINT8)~MCB_AA_DIR0_START;
	}
	if((bReg & 0x0F) == 0)
	{
		bReg &= (UINT8)~MCB_AA_DITIM0_START;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIX0_START, bReg);

	bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIX1_START);
	if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_1_AA) == eMCDRV_SRC_NONE_AA)
	{/*	DIT is unused	*/
		bReg &= (UINT8)~MCB_AA_DIT1_SRC_START;
		bReg &= (UINT8)~MCB_AA_DIT1_START;
	}
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR1_AA) == 0)
	{/*	DIR is unused	*/
		bReg &= (UINT8)~MCB_AA_DIR1_SRC_START;
		bReg &= (UINT8)~MCB_AA_DIR1_START;
	}
	if((bReg & 0x0F) == 0)
	{
		bReg &= (UINT8)~MCB_AA_DITIM1_START;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIX1_START, bReg);

	bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIX2_START);
	if(McResCtrl_GetDITSource_AA(eMCDRV_DIO_2_AA) == eMCDRV_SRC_NONE_AA)
	{/*	DIT is unused	*/
		bReg &= (UINT8)~MCB_AA_DIT2_SRC_START;
		bReg &= (UINT8)~MCB_AA_DIT2_START;
	}
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_DIR2_AA) == 0)
	{/*	DIR is unused	*/
		bReg &= (UINT8)~MCB_AA_DIR2_SRC_START;
		bReg &= (UINT8)~MCB_AA_DIR2_START;
	}
	if((bReg & 0x0F) == 0)
	{
		bReg &= (UINT8)~MCB_AA_DITIM2_START;
	}
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIX2_START, bReg);

	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_ADC0_AA, eMCDRV_DST_CH0_AA) == 0
	&& McResCtrl_IsDstUsed_AA(eMCDRV_DST_ADC0_AA, eMCDRV_DST_CH1_AA) == 0)
	{/*	ADC0 source is unused	*/
		AddStopADC();
	}
	if(McResCtrl_IsSrcUsed_AA(eMCDRV_SRC_PDM_AA) == 0)
	{/*	PDM is unused	*/
		AddStopPDM();
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McPacket_AddVol_AA
 *
 *	Description:
 *			Add volume mute packet.
 *	Arguments:
 *			dUpdate		target volume items
 *			eMode		update mode
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
SINT32	McPacket_AddVol_AA
(
	UINT32					dUpdate,
	MCDRV_VOLUPDATE_MODE_AA	eMode
)
{
	UINT8			bVolL;
	UINT8			bVolR;
	UINT8			bLAT;
	UINT8			bReg;
	UINT32			dSVolDoneParam	= 0;
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_VOL_INFO	sVolInfo;

	McResCtrl_GetInitInfo_AA(&sInitInfo);
	McResCtrl_GetVolReg_AA(&sVolInfo);

	if((dUpdate & MCDRV_VOLUPDATE_ANAOUT_ALL_AA) != (UINT32)0)
	{
		bVolL	= (UINT8)sVolInfo.aswA_Hp[0]&MCB_AA_HPVOL_L;
		bVolR	= (UINT8)sVolInfo.aswA_Hp[1]&MCB_AA_HPVOL_R;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_HPVOL_L) & MCB_AA_HPVOL_L))
		{
			if((eMode != eMCDRV_VOLUPDATE_MUTE_AA) || (bVolL == MCDRV_REG_MUTE))
			{
				if(((eMode != eMCDRV_VOLUPDATE_MUTE_AA) || (bVolR == MCDRV_REG_MUTE))
				&& (bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_HPVOL_R)))
				{
					bLAT	= MCB_AA_ALAT_HP;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|MCB_AA_SVOL_HP|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_HPVOL_L, bReg);
				if(bVolL == MCDRV_REG_MUTE)
				{
					dSVolDoneParam	|= (MCB_AA_HPL_BUSY<<8);
				}
			}
		}
		if((eMode != eMCDRV_VOLUPDATE_MUTE_AA) || (bVolR == MCDRV_REG_MUTE))
		{
			if((bVolR == MCDRV_REG_MUTE) && (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_HPVOL_R) != 0))
			{
				dSVolDoneParam	|= (UINT8)MCB_AA_HPR_BUSY;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_HPVOL_R, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Sp[0]&MCB_AA_SPVOL_L;
		bVolR	= (UINT8)sVolInfo.aswA_Sp[1]&MCB_AA_SPVOL_R;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_SPVOL_L) & MCB_AA_SPVOL_L))
		{
			if((eMode != eMCDRV_VOLUPDATE_MUTE_AA) || (bVolL == MCDRV_REG_MUTE))
			{
				if(((eMode != eMCDRV_VOLUPDATE_MUTE_AA) || (bVolR == MCDRV_REG_MUTE))
				&& (bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_SPVOL_R)))
				{
					bLAT	= MCB_AA_ALAT_SP;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|MCB_AA_SVOL_SP|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_SPVOL_L, bReg);
				if(bVolL == MCDRV_REG_MUTE)
				{
					dSVolDoneParam	|= (MCB_AA_SPL_BUSY<<8);
				}
			}
		}
		if((eMode != eMCDRV_VOLUPDATE_MUTE_AA) || (bVolR == MCDRV_REG_MUTE))
		{
			if((bVolR == MCDRV_REG_MUTE) && (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_SPVOL_R) != 0))
			{
				dSVolDoneParam	|= (UINT8)MCB_AA_SPR_BUSY;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_SPVOL_R, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Rc[0]&MCB_AA_RCVOL;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_RCVOL) & MCB_AA_RCVOL))
		{
			if((eMode != eMCDRV_VOLUPDATE_MUTE_AA) || (bVolL == MCDRV_REG_MUTE))
			{
				bReg	= MCB_AA_SVOL_RC|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | (UINT32)MCI_AA_RCVOL, bReg);
				if(bVolL == MCDRV_REG_MUTE)
				{
					dSVolDoneParam	|= (MCB_AA_RC_BUSY<<8);
				}
			}
		}

		bVolL	= (UINT8)sVolInfo.aswA_Lout1[0]&MCB_AA_LO1VOL_L;
		bVolR	= (UINT8)sVolInfo.aswA_Lout1[1]&MCB_AA_LO1VOL_R;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_LO1VOL_L) & MCB_AA_LO1VOL_L))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_LO1VOL_R))
				{
					bLAT	= MCB_AA_ALAT_LO1;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LO1VOL_L, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LO1VOL_R, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Lout2[0]&MCB_AA_LO2VOL_L;
		bVolR	= (UINT8)sVolInfo.aswA_Lout2[1]&MCB_AA_LO2VOL_R;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_LO2VOL_L) & MCB_AA_LO2VOL_L))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_LO2VOL_R))
				{
					bLAT	= MCB_AA_ALAT_LO2;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LO2VOL_L, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LO2VOL_R, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswA_HpGain[0];
		if(bVolL != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_HP_GAIN))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_HP_GAIN, bVolL);
			}
		}
		/*	wait XX_BUSY	*/
		if(dSVolDoneParam != (UINT32)0)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_EVTWAIT_AA | MCDRV_EVT_SVOL_DONE_AA | dSVolDoneParam, 0);
		}
	}
	if((dUpdate & (UINT32)~MCDRV_VOLUPDATE_ANAOUT_ALL_AA) != (UINT32)0)
	{
		bVolL	= (UINT8)sVolInfo.aswA_Lin1[0]&MCB_AA_LI1VOL_L;
		bVolR	= (UINT8)sVolInfo.aswA_Lin1[1]&MCB_AA_LI1VOL_R;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_LI1VOL_L) & MCB_AA_LI1VOL_L))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_LI1VOL_R))
				{
					bLAT	= MCB_AA_ALAT_LI1;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LI1VOL_L, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LI1VOL_R, bVolR);
		}

		if(McDevProf_IsValid(eMCDRV_FUNC_LI2) == 1)
		{
			bVolL	= (UINT8)sVolInfo.aswA_Lin2[0]&MCB_AA_LI2VOL_L;
			bVolR	= (UINT8)sVolInfo.aswA_Lin2[1]&MCB_AA_LI2VOL_R;
			if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_LI2VOL_L) & MCB_AA_LI2VOL_L))
			{
				if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
				{
					if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
					&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_LI2VOL_R))
					{
						bLAT	= MCB_AA_ALAT_LI2;
					}
					else
					{
						bLAT	= 0;
					}
					bReg	= bLAT|bVolL;
					McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LI2VOL_L, bReg);
				}
			}
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
			{
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_LI2VOL_R, bVolR);
			}
		}

		bVolL	= (UINT8)sVolInfo.aswA_Mic1[0]&MCB_AA_MC1VOL;
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_MC1VOL, bVolL);
		}
		bVolL	= (UINT8)sVolInfo.aswA_Mic2[0]&MCB_AA_MC2VOL;
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_MC2VOL, bVolL);
		}
		bVolL	= (UINT8)sVolInfo.aswA_Mic3[0]&MCB_AA_MC3VOL;
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_MC3VOL, bVolL);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Ad0[0]&MCB_AA_ADVOL_L;
		bVolR	= (UINT8)sVolInfo.aswA_Ad0[1]&MCB_AA_ADVOL_R;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_ADVOL_L) & MCB_AA_ADVOL_L))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_ADVOL_R))
				{
					bLAT	= MCB_AA_ALAT_AD;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_ADVOL_L, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_ADVOL_R, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswA_Mic2Gain[0]&0x03;
		bVolL	= (UINT8)((bVolL << 4) & MCB_AA_MC2GAIN) | (UINT8)(sVolInfo.aswA_Mic1Gain[0]&MCB_AA_MC1GAIN);
		bVolL |= ((sInitInfo.bMic2Sng << 6) & MCB_AA_MC2SNG);
		bVolL |= ((sInitInfo.bMic1Sng << 2) & MCB_AA_MC1SNG);
		if(eMode == eMCDRV_VOLUPDATE_MUTE_AA)
		{
			bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_MC_GAIN);
			if(((bReg & MCB_AA_MC2GAIN) == 0) && ((bReg & MCB_AA_MC1GAIN) == 0))
			{
				;
			}
			else
			{
				if((bReg & MCB_AA_MC2GAIN) == 0)
				{
					bVolL &= (UINT8)~MCB_AA_MC2GAIN;
				}
				else if((bReg & MCB_AA_MC1GAIN) == 0)
				{
					bVolL &= (UINT8)~MCB_AA_MC1GAIN;
				}
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_MC_GAIN, bVolL);
			}
		}
		else
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_MC_GAIN, bVolL);
		}

		bVolL	= (UINT8)(sVolInfo.aswA_Mic3Gain[0]&MCB_AA_MC3GAIN) | ((sInitInfo.bMic3Sng << 2) & MCB_AA_MC3SNG);
		if(eMode == eMCDRV_VOLUPDATE_MUTE_AA)
		{
			bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_MC3_GAIN);
			if((bReg & MCB_AA_MC3GAIN) == 0)
			{
				;
			}
			else
			{
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_MC3_GAIN, bVolL);
			}
		}
		else
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_MC3_GAIN, bVolL);
		}

		bVolL	= (UINT8)sVolInfo.aswD_Dit0[0]&MCB_AA_DIT0_INVOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Dit0[1]&MCB_AA_DIT0_INVOLR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT0_INVOLL) & MCB_AA_DIT0_INVOLL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT0_INVOLR))
				{
					bLAT	= MCB_AA_DIT0_INLAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIT0_INVOLL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIT0_INVOLR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_Dit1[0]&MCB_AA_DIT1_INVOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Dit1[1]&MCB_AA_DIT1_INVOLR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT1_INVOLL) & MCB_AA_DIT1_INVOLL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT1_INVOLR))
				{
					bLAT	= MCB_AA_DIT1_INLAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIT1_INVOLL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIT1_INVOLR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_Dit2[0]&MCB_AA_DIT2_INVOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Dit2[1]&MCB_AA_DIT2_INVOLR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT2_INVOLL) & MCB_AA_DIT2_INVOLL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT2_INVOLR))
				{
					bLAT	= MCB_AA_DIT2_INLAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIT2_INVOLL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIT2_INVOLR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_Pdm[0]&MCB_AA_PDM0_VOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Pdm[1]&MCB_AA_PDM0_VOLR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PDM0_VOLL) & MCB_AA_PDM0_VOLL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PDM0_VOLR))
				{
					bLAT	= MCB_AA_PDM0_INLAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM0_VOLL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM0_VOLR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_Dir0[0]&MCB_AA_DIR0_VOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Dir0[1]&MCB_AA_DIR0_VOLR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR0_VOLL) & MCB_AA_DIR0_VOLL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR0_VOLR))
				{
					bLAT	= MCB_AA_DIR0_LAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIR0_VOLL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIR0_VOLR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_Dir1[0]&MCB_AA_DIR1_VOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Dir1[1]&MCB_AA_DIR1_VOLR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR1_VOLL) & MCB_AA_DIR1_VOLL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR1_VOLR))
				{
					bLAT	= MCB_AA_DIR1_LAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIR1_VOLL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIR1_VOLR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_Dir2[0]&MCB_AA_DIR2_VOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Dir2[1]&MCB_AA_DIR2_VOLR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR2_VOLL) & MCB_AA_DIR2_VOLL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR2_VOLR))
				{
					bLAT	= MCB_AA_DIR2_LAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIR2_VOLL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIR2_VOLR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_Ad0[0]&MCB_AA_ADC_VOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Ad0[1]&MCB_AA_ADC_VOLR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_ADC_VOLL) & MCB_AA_ADC_VOLL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_ADC_VOLR))
				{
					bLAT	= MCB_AA_ADC_LAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_ADC_VOLL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_ADC_VOLR, bVolR);
		}

		if(McDevProf_IsValid(eMCDRV_FUNC_ADC1) == 1)
		{
#if 0
			bVolL	= (UINT8)sVolInfo.aswD_Ad1[0]&MCB_AA_ADC1_VOLL;
			bVolR	= (UINT8)sVolInfo.aswD_Ad1[1]&MCB_AA_ADC1_VOLR;
			if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_ADC1_VOLL) & MCB_AA_ADC1_VOLL))
			{
				if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
				{
					if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
					&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_ADC1_VOLR))
					{
						bLAT	= MCB_AA_ADC_LAT;
					}
					else
					{
						bLAT	= 0;
					}
					bReg	= bLAT|bVolL;
					McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_ADC1_VOLL, bReg);
				}
			}
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
			{
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_ADC1_VOLR, bVolR);
			}
#endif
		}

		bVolL	= (UINT8)sVolInfo.aswD_Aeng6[0]&MCB_AA_AENG6_VOLL;
		bVolR	= (UINT8)sVolInfo.aswD_Aeng6[1]&MCB_AA_AENG6_VOLR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_AENG6_VOLL) & MCB_AA_AENG6_VOLL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_AENG6_VOLR))
				{
					bLAT	= MCB_AA_AENG6_LAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_AENG6_VOLL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_AENG6_VOLR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_Ad0Att[0]&MCB_AA_ADC_ATTL;
		bVolR	= (UINT8)sVolInfo.aswD_Ad0Att[1]&MCB_AA_ADC_ATTR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_ADC_ATTL) & MCB_AA_ADC_ATTL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_ADC_ATTR))
				{
					bLAT	= MCB_AA_ADC_ALAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_ADC_ATTL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_ADC_ATTR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_Dir0Att[0]&MCB_AA_DIR0_ATTL;
		bVolR	= (UINT8)sVolInfo.aswD_Dir0Att[1]&MCB_AA_DIR0_ATTR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR0_ATTL) & MCB_AA_DIR0_ATTL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR0_ATTR))
				{
					bLAT	= MCB_AA_DIR0_ALAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIR0_ATTL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIR0_ATTR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_Dir1Att[0]&MCB_AA_DIR1_ATTL;
		bVolR	= (UINT8)sVolInfo.aswD_Dir1Att[1]&MCB_AA_DIR1_ATTR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR1_ATTL) & MCB_AA_DIR1_ATTL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR1_ATTR))
				{
					bLAT	= MCB_AA_DIR1_ALAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIR1_ATTL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIR1_ATTR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_Dir2Att[0]&MCB_AA_DIR2_ATTL;
		bVolR	= (UINT8)sVolInfo.aswD_Dir2Att[1]&MCB_AA_DIR2_ATTR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR2_ATTL) & MCB_AA_DIR2_ATTL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR2_ATTR))
				{
					bLAT	= MCB_AA_DIR2_ALAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIR2_ATTL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIR2_ATTR, bVolR);
		}

		if(McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_AENG6_SOURCE) == MCB_AA_AENG6_PDM)
		{
			bVolL	= (UINT8)sVolInfo.aswD_SideTone[0]&MCB_AA_ST_VOLL;
			bVolR	= (UINT8)sVolInfo.aswD_SideTone[1]&MCB_AA_ST_VOLR;
			if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_ST_VOLL) & MCB_AA_ST_VOLL))
			{
				if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
				{
					if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
					&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_ST_VOLR))
					{
						bLAT	= MCB_AA_ST_LAT;
					}
					else
					{
						bLAT	= 0;
					}
					bReg	= bLAT|bVolL;
					McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_ST_VOLL, bReg);
				}
			}
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
			{
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_ST_VOLR, bVolR);
			}
		}

		bVolL	= (UINT8)sVolInfo.aswD_DacMaster[0]&MCB_AA_MASTER_OUTL;
		bVolR	= (UINT8)sVolInfo.aswD_DacMaster[1]&MCB_AA_MASTER_OUTR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_MASTER_OUTL) & MCB_AA_MASTER_OUTL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_MASTER_OUTR))
				{
					bLAT	= MCB_AA_MASTER_OLAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_MASTER_OUTL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_MASTER_OUTR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_DacVoice[0]&MCB_AA_VOICE_ATTL;
		bVolR	= (UINT8)sVolInfo.aswD_DacVoice[1]&MCB_AA_VOICE_ATTR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_VOICE_ATTL) & MCB_AA_VOICE_ATTL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_VOICE_ATTR))
				{
					bLAT	= MCB_AA_VOICE_LAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_VOICE_ATTL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_VOICE_ATTR, bVolR);
		}

		bVolL	= (UINT8)sVolInfo.aswD_DacAtt[0]&MCB_AA_DAC_ATTL;
		bVolR	= (UINT8)sVolInfo.aswD_DacAtt[1]&MCB_AA_DAC_ATTR;
		if(bVolL != (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DAC_ATTL) & MCB_AA_DAC_ATTL))
		{
			if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolL == MCDRV_REG_MUTE)
			{
				if((eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
				&& bVolR != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DAC_ATTR))
				{
					bLAT	= MCB_AA_DAC_LAT;
				}
				else
				{
					bLAT	= 0;
				}
				bReg	= bLAT|bVolL;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DAC_ATTL, bReg);
			}
		}
		if(eMode != eMCDRV_VOLUPDATE_MUTE_AA || bVolR == MCDRV_REG_MUTE)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DAC_ATTR, bVolR);
		}
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	AddStopADC
 *
 *	Description:
 *			Add stop ADC packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddStopADC
(
	void
)
{
	UINT8	bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_CODEC_AA, MCI_AA_AD_START);
	if((bReg & MCB_AA_AD_START) != 0)
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_AD_MUTE, MCB_AA_AD_MUTE);
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_AD_START, bReg&(UINT8)~MCB_AA_AD_START);
	}
}

/****************************************************************************
 *	AddStopPDM
 *
 *	Description:
 *			Add stop PDM packet.
 *	Arguments:
 *			none
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddStopPDM
(
	void
)
{
	UINT8	bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PDM_START);
	if((bReg & MCB_AA_PDM_START) != 0)
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM_MUTE, MCB_AA_PDM_MUTE);
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM_START, bReg&(UINT8)~MCB_AA_PDM_START);
	}
}

/****************************************************************************
 *	McPacket_AddDigitalIO_AA
 *
 *	Description:
 *			Add DigitalI0 setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddDigitalIO_AA
(
	UINT32	dUpdateInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	MCDRV_POWER_INFO_AA	sPowerInfo;
	MCDRV_POWER_UPDATE_AA	sPowerUpdate;
	MCDRV_DIO_INFO		sDioInfo;

	if(IsModifiedDIO(dUpdateInfo) == 0)
	{
		return sdRet;
	}

	McResCtrl_GetCurPowerInfo_AA(&sPowerInfo);
	sdRet	= PowerUpDig(MCDRV_DPB_UP_AA);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}

	if((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != (UINT32)0)
	{
		AddDIOCommon(eMCDRV_DIO_0_AA);
	}
	if((dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != (UINT32)0)
	{
		AddDIOCommon(eMCDRV_DIO_1_AA);
	}
	if((dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != (UINT32)0)
	{
		AddDIOCommon(eMCDRV_DIO_2_AA);
	}

	/*	DI*_BCKP	*/
	if((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != (UINT32)0
	|| (dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != (UINT32)0
	|| (dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != (UINT32)0)
	{
		McResCtrl_GetDioInfo_AA(&sDioInfo);
		bReg	= 0;
		if(sDioInfo.asPortInfo[0].sDioCommon.bBckInvert == MCDRV_BCLK_INVERT)
		{
			bReg |= MCB_AA_DI0_BCKP;
		}
		if(sDioInfo.asPortInfo[1].sDioCommon.bBckInvert == MCDRV_BCLK_INVERT)
		{
			bReg |= MCB_AA_DI1_BCKP;
		}
		if(sDioInfo.asPortInfo[2].sDioCommon.bBckInvert == MCDRV_BCLK_INVERT)
		{
			bReg |= MCB_AA_DI2_BCKP;
		}
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_BCKP))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | (UINT32)MCI_AA_BCKP, bReg);
		}
	}

	if((dUpdateInfo & MCDRV_DIO0_DIR_UPDATE_FLAG) != (UINT32)0)
	{
		AddDIODIR(eMCDRV_DIO_0_AA);
	}
	if((dUpdateInfo & MCDRV_DIO1_DIR_UPDATE_FLAG) != (UINT32)0)
	{
		AddDIODIR(eMCDRV_DIO_1_AA);
	}
	if((dUpdateInfo & MCDRV_DIO2_DIR_UPDATE_FLAG) != (UINT32)0)
	{
		AddDIODIR(eMCDRV_DIO_2_AA);
	}

	if((dUpdateInfo & MCDRV_DIO0_DIT_UPDATE_FLAG) != (UINT32)0)
	{
		AddDIODIT(eMCDRV_DIO_0_AA);
	}
	if((dUpdateInfo & MCDRV_DIO1_DIT_UPDATE_FLAG) != (UINT32)0)
	{
		AddDIODIT(eMCDRV_DIO_1_AA);
	}
	if((dUpdateInfo & MCDRV_DIO2_DIT_UPDATE_FLAG) != (UINT32)0)
	{
		AddDIODIT(eMCDRV_DIO_2_AA);
	}

	/*	unused path power down	*/
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
	sPowerUpdate.abAnalog[0]	=
	sPowerUpdate.abAnalog[1]	=
	sPowerUpdate.abAnalog[2]	=
	sPowerUpdate.abAnalog[3]	=
	sPowerUpdate.abAnalog[4]	= 0;
	return McPacket_AddPowerDown_AA(&sPowerInfo, &sPowerUpdate);
}

/****************************************************************************
 *	IsModifiedDIO
 *
 *	Description:
 *			Is modified DigitalIO.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			0:not modified/1:modified
 *
 ****************************************************************************/
static UINT8	IsModifiedDIO
(
	UINT32		dUpdateInfo
)
{
	if((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != (UINT32)0 && IsModifiedDIOCommon(eMCDRV_DIO_0_AA) == 1)
	{
		return 1;
	}
	if((dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != (UINT32)0 && IsModifiedDIOCommon(eMCDRV_DIO_1_AA) == 1)
	{
		return 1;
	}
	if((dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != (UINT32)0 && IsModifiedDIOCommon(eMCDRV_DIO_2_AA) == 1)
	{
		return 1;
	}

	if((dUpdateInfo & MCDRV_DIO0_DIR_UPDATE_FLAG) != (UINT32)0 && IsModifiedDIODIR(eMCDRV_DIO_0_AA) == 1)
	{
		return 1;
	}
	if((dUpdateInfo & MCDRV_DIO1_DIR_UPDATE_FLAG) != (UINT32)0 && IsModifiedDIODIR(eMCDRV_DIO_1_AA) == 1)
	{
		return 1;
	}
	if((dUpdateInfo & MCDRV_DIO2_DIR_UPDATE_FLAG) != (UINT32)0 && IsModifiedDIODIR(eMCDRV_DIO_2_AA) == 1)
	{
		return 1;
	}

	if((dUpdateInfo & MCDRV_DIO0_DIT_UPDATE_FLAG) != (UINT32)0 && IsModifiedDIODIT(eMCDRV_DIO_0_AA) == 1)
	{
		return 1;
	}
	if((dUpdateInfo & MCDRV_DIO1_DIT_UPDATE_FLAG) != (UINT32)0 && IsModifiedDIODIT(eMCDRV_DIO_1_AA) == 1)
	{
		return 1;
	}
	if((dUpdateInfo & MCDRV_DIO2_DIT_UPDATE_FLAG) != (UINT32)0 && IsModifiedDIODIT(eMCDRV_DIO_2_AA) == 1)
	{
		return 1;
	}
	return 0;
}

/****************************************************************************
 *	IsModifiedDIOCommon
 *
 *	Description:
 *			Is modified DigitalIO Common.
 *	Arguments:
 *			ePort		port number
 *	Return:
 *			0:not modified/1:modified
 *
 ****************************************************************************/
static UINT8	IsModifiedDIOCommon
(
	MCDRV_DIO_PORT_NO_AA	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	MCDRV_DIO_INFO	sDioInfo;

	if(ePort == eMCDRV_DIO_0_AA)
	{
		bRegOffset	= 0;
	}
	else if(ePort == eMCDRV_DIO_1_AA)
	{
		bRegOffset	= MCI_AA_DIMODE1 - MCI_AA_DIMODE0;
	}
	else if(ePort == eMCDRV_DIO_2_AA)
	{
		bRegOffset	= MCI_AA_DIMODE2 - MCI_AA_DIMODE0;
	}
	else
	{
		return 0;
	}

	McResCtrl_GetDioInfo_AA(&sDioInfo);

	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIMODE0+bRegOffset))
	{
		return 1;
	}

	bReg	= (sDioInfo.asPortInfo[ePort].sDioCommon.bAutoFs << 7)
			| (sDioInfo.asPortInfo[ePort].sDioCommon.bBckFs << 4)
			| sDioInfo.asPortInfo[ePort].sDioCommon.bFs;
	if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DI_FS0+bRegOffset))
	{
		return 1;
	}

	bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DI0_SRC+bRegOffset);
	if(sDioInfo.asPortInfo[ePort].sDioCommon.bAutoFs == 0
	&& sDioInfo.asPortInfo[ePort].sDioCommon.bMasterSlave == MCDRV_DIO_SLAVE)
	{
		bReg |= MCB_AA_DICOMMON_SRC_RATE_SET;
	}
	if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DI0_SRC+bRegOffset))
	{
		return 1;
	}

	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_PCM)
	{
		bReg	= (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmHizTim << 7)
				| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmClkDown << 6)
				| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmFrame << 5)
				| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmHighPeriod);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_HIZ_REDGE0+bRegOffset))
		{
			return 1;
		}
	}
	return 0;
}

/****************************************************************************
 *	IsModifiedDIODIR
 *
 *	Description:
 *			Is modified DigitalIO DIR.
 *	Arguments:
 *			ePort		port number
 *	Return:
 *			0:not modified/1:modified
 *
 ****************************************************************************/
static UINT8	IsModifiedDIODIR
(
	MCDRV_DIO_PORT_NO_AA	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	MCDRV_DIO_INFO	sDioInfo;

	if(ePort == eMCDRV_DIO_0_AA)
	{
		bRegOffset	= 0;
	}
	else if(ePort == eMCDRV_DIO_1_AA)
	{
		bRegOffset	= MCI_AA_DIMODE1 - MCI_AA_DIMODE0;
	}
	else if(ePort == eMCDRV_DIO_2_AA)
	{
		bRegOffset	= MCI_AA_DIMODE2 - MCI_AA_DIMODE0;
	}
	else
	{
		return 0;
	}

	McResCtrl_GetDioInfo_AA(&sDioInfo);

	bReg	= (UINT8)(sDioInfo.asPortInfo[ePort].sDir.wSrcRate>>8);
	if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIRSRC_RATE0_MSB+bRegOffset))
	{
		return 1;
	}

	bReg	= (UINT8)sDioInfo.asPortInfo[ePort].sDir.wSrcRate;
	if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIRSRC_RATE0_LSB+bRegOffset))
	{
		return 1;
	}

	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_DA)
	{
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bMode << 6)
				| (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bBitSel << 4)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bMode << 2)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIX0_FMT+bRegOffset))
		{
			return 1;
		}
		/*	DIR*_CH	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDir.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDir.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR0_CH+bRegOffset))
		{
			return 1;
		}
	}
	else
	{
		/*	PCM_MONO_RX*, PCM_EXTEND_RX*, PCM_LSBON_RX*, PCM_LAW_RX*, PCM_BIT_RX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bMono << 7)
				| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bOrder << 4)
				| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bLaw << 2)
				| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PCM_RX0+bRegOffset))
		{
			return 1;
		}
		/*	PCM_CH1_RX*, PCM_CH0_RX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDir.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDir.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PCM_SLOT_RX0+bRegOffset))
		{
			return 1;
		}
	}
	return 0;
}

/****************************************************************************
 *	IsModifiedDIODIT
 *
 *	Description:
 *			Is modified DigitalIO DIT.
 *	Arguments:
 *			ePort		port number
 *	Return:
 *			0:not modified/1:modified
 *
 ****************************************************************************/
static UINT8	IsModifiedDIODIT
(
	MCDRV_DIO_PORT_NO_AA	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	MCDRV_DIO_INFO	sDioInfo;

	if(ePort == eMCDRV_DIO_0_AA)
	{
		bRegOffset	= 0;
	}
	else if(ePort == eMCDRV_DIO_1_AA)
	{
		bRegOffset	= MCI_AA_DIMODE1 - MCI_AA_DIMODE0;
	}
	else if(ePort == eMCDRV_DIO_2_AA)
	{
		bRegOffset	= MCI_AA_DIMODE2 - MCI_AA_DIMODE0;
	}
	else
	{
		return 0;
	}

	McResCtrl_GetDioInfo_AA(&sDioInfo);

	bReg	= (UINT8)(sDioInfo.asPortInfo[ePort].sDit.wSrcRate>>8);
	if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DITSRC_RATE0_MSB+bRegOffset))
	{
		return 1;
	}
	bReg	= (UINT8)sDioInfo.asPortInfo[ePort].sDit.wSrcRate;
	if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DITSRC_RATE0_LSB+bRegOffset))
	{
		return 1;
	}

	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_DA)
	{
		/*	DIT*_FMT, DIT*_BIT, DIR*_FMT, DIR*_BIT	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bMode << 6)
				| (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bBitSel << 4)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bMode << 2)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIX0_FMT+bRegOffset))
		{
			return 1;
		}

		/*	DIT*_SLOT	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDit.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT0_SLOT+bRegOffset))
		{
			return 1;
		}
	}
	else
	{
		/*	PCM_MONO_TX*, PCM_EXTEND_TX*, PCM_LSBON_TX*, PCM_LAW_TX*, PCM_BIT_TX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bMono << 7)
				| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bOrder << 4)
				| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bLaw << 2)
				| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PCM_TX0+bRegOffset))
		{
			return 1;
		}

		/*	PCM_CH1_TX*, PCM_CH0_TX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDit.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PCM_SLOT_TX0+bRegOffset))
		{
			return 1;
		}
	}
	return 0;
}

/****************************************************************************
 *	AddDIOCommon
 *
 *	Description:
 *			Add DigitalI0 Common setup packet.
 *	Arguments:
 *			ePort		port number
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDIOCommon
(
	MCDRV_DIO_PORT_NO_AA	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	MCDRV_DIO_INFO	sDioInfo;

	if(ePort == eMCDRV_DIO_0_AA)
	{
		bRegOffset	= 0;
	}
	else if(ePort == eMCDRV_DIO_1_AA)
	{
		bRegOffset	= MCI_AA_DIMODE1 - MCI_AA_DIMODE0;
	}
	else if(ePort == eMCDRV_DIO_2_AA)
	{
		bRegOffset	= MCI_AA_DIMODE2 - MCI_AA_DIMODE0;
	}
	else
	{
		return;
	}

	McResCtrl_GetDioInfo_AA(&sDioInfo);

	/*	DIMODE*	*/
	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIMODE0+bRegOffset))
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_DIMODE0+bRegOffset),
							sDioInfo.asPortInfo[ePort].sDioCommon.bInterface);
	}

	/*	DIAUTO_FS*, DIBCK*, DIFS*	*/
	bReg	= (sDioInfo.asPortInfo[ePort].sDioCommon.bAutoFs << 7)
			| (sDioInfo.asPortInfo[ePort].sDioCommon.bBckFs << 4)
			| sDioInfo.asPortInfo[ePort].sDioCommon.bFs;
	if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DI_FS0+bRegOffset))
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_DI_FS0+bRegOffset), bReg);
	}

	/*	DI*_SRCRATE_SET	*/
	bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DI0_SRC+bRegOffset);
	if(sDioInfo.asPortInfo[ePort].sDioCommon.bAutoFs == 0
	&& sDioInfo.asPortInfo[ePort].sDioCommon.bMasterSlave == MCDRV_DIO_SLAVE)
	{
		bReg |= MCB_AA_DICOMMON_SRC_RATE_SET;
	}
	else
	{
		bReg &= (UINT8)~MCB_AA_DICOMMON_SRC_RATE_SET;
	}
	if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DI0_SRC+bRegOffset))
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_DI0_SRC+bRegOffset), bReg);
	}

	/*	HIZ_REDGE*, PCM_CLKDOWN*, PCM_FRAME*, PCM_HPERIOD*	*/
	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_PCM)
	{
		bReg	= (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmHizTim << 7)
				| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmClkDown << 6)
				| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmFrame << 5)
				| (sDioInfo.asPortInfo[ePort].sDioCommon.bPcmHighPeriod);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_HIZ_REDGE0+bRegOffset))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_HIZ_REDGE0+bRegOffset), bReg);
		}
	}
}

/****************************************************************************
 *	AddDIODIR
 *
 *	Description:
 *			Add DigitalI0 DIR setup packet.
 *	Arguments:
 *			ePort		port number
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDIODIR
(
	MCDRV_DIO_PORT_NO_AA	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	UINT16	wSrcRate;
	MCDRV_DIO_INFO	sDioInfo;

	if(ePort == eMCDRV_DIO_0_AA)
	{
		bRegOffset	= 0;
	}
	else if(ePort == eMCDRV_DIO_1_AA)
	{
		bRegOffset	= MCI_AA_DIMODE1 - MCI_AA_DIMODE0;
	}
	else if(ePort == eMCDRV_DIO_2_AA)
	{
		bRegOffset	= MCI_AA_DIMODE2 - MCI_AA_DIMODE0;
	}
	else
	{
		return;
	}

	McResCtrl_GetDioInfo_AA(&sDioInfo);

	/*	DIRSRC_RATE*	*/
	wSrcRate	= sDioInfo.asPortInfo[ePort].sDir.wSrcRate;
	if(wSrcRate == 0)
	{
		switch(sDioInfo.asPortInfo[ePort].sDioCommon.bFs)
		{
		case MCDRV_FS_48000:
			wSrcRate	= MCDRV_DIR_SRCRATE_48000_AA;
			break;
		case MCDRV_FS_44100:
			wSrcRate	= MCDRV_DIR_SRCRATE_44100_AA;
			break;
		case MCDRV_FS_32000:
			wSrcRate	= MCDRV_DIR_SRCRATE_32000_AA;
			break;
		case MCDRV_FS_24000:
			wSrcRate	= MCDRV_DIR_SRCRATE_24000_AA;
			break;
		case MCDRV_FS_22050:
			wSrcRate	= MCDRV_DIR_SRCRATE_22050_AA;
			break;
		case MCDRV_FS_16000:
			wSrcRate	= MCDRV_DIR_SRCRATE_16000_AA;
			break;
		case MCDRV_FS_12000:
			wSrcRate	= MCDRV_DIR_SRCRATE_12000_AA;
			break;
		case MCDRV_FS_11025:
			wSrcRate	= MCDRV_DIR_SRCRATE_11025_AA;
			break;
		case MCDRV_FS_8000:
			wSrcRate	= MCDRV_DIR_SRCRATE_8000_AA;
			break;
		default:
			/* unreachable */
			wSrcRate = 0;
			break;
		}
	}
	bReg	= (UINT8)(wSrcRate>>8);
	if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIRSRC_RATE0_MSB+bRegOffset))
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_DIRSRC_RATE0_MSB+bRegOffset), bReg);
	}
	bReg	= (UINT8)wSrcRate;
	if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIRSRC_RATE0_LSB+bRegOffset))
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_DIRSRC_RATE0_LSB+bRegOffset), bReg);
	}

	/*	DIT*_FMT, DIT*_BIT, DIR*_FMT, DIR*_BIT	*/
	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_DA)
	{
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bMode << 6)
				| (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bBitSel << 4)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bMode << 2)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIX0_FMT+bRegOffset))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_DIX0_FMT+bRegOffset), bReg);
		}
		/*	DIR*_CH	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDir.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDir.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIR0_CH+bRegOffset))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_DIR0_CH+bRegOffset), bReg);
		}
	}
	else
	{
		/*	PCM_MONO_RX*, PCM_EXTEND_RX*, PCM_LSBON_RX*, PCM_LAW_RX*, PCM_BIT_RX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bMono << 7)
				| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bOrder << 4)
				| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bLaw << 2)
				| (sDioInfo.asPortInfo[ePort].sDir.sPcmFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PCM_RX0+bRegOffset))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_PCM_RX0+bRegOffset), bReg);
		}
		/*	PCM_CH1_RX*, PCM_CH0_RX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDir.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDir.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PCM_SLOT_RX0+bRegOffset))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_PCM_SLOT_RX0+bRegOffset), bReg);
		}
	}
}

/****************************************************************************
 *	AddDIODIT
 *
 *	Description:
 *			Add DigitalI0 DIT setup packet.
 *	Arguments:
 *			ePort		port number
 *	Return:
 *			none
 *
 ****************************************************************************/
static void	AddDIODIT
(
	MCDRV_DIO_PORT_NO_AA	ePort
)
{
	UINT8	bReg;
	UINT8	bRegOffset;
	UINT16	wSrcRate;
	MCDRV_DIO_INFO	sDioInfo;

	if(ePort == eMCDRV_DIO_0_AA)
	{
		bRegOffset	= 0;
	}
	else if(ePort == eMCDRV_DIO_1_AA)
	{
		bRegOffset	= MCI_AA_DIMODE1 - MCI_AA_DIMODE0;
	}
	else if(ePort == eMCDRV_DIO_2_AA)
	{
		bRegOffset	= MCI_AA_DIMODE2 - MCI_AA_DIMODE0;
	}
	else
	{
		return;
	}

	McResCtrl_GetDioInfo_AA(&sDioInfo);

	wSrcRate	= sDioInfo.asPortInfo[ePort].sDit.wSrcRate;
	if(wSrcRate == 0)
	{
		switch(sDioInfo.asPortInfo[ePort].sDioCommon.bFs)
		{
		case MCDRV_FS_48000:
			wSrcRate	= MCDRV_DIT_SRCRATE_48000_AA;
			break;
		case MCDRV_FS_44100:
			wSrcRate	= MCDRV_DIT_SRCRATE_44100_AA;
			break;
		case MCDRV_FS_32000:
			wSrcRate	= MCDRV_DIT_SRCRATE_32000_AA;
			break;
		case MCDRV_FS_24000:
			wSrcRate	= MCDRV_DIT_SRCRATE_24000_AA;
			break;
		case MCDRV_FS_22050:
			wSrcRate	= MCDRV_DIT_SRCRATE_22050_AA;
			break;
		case MCDRV_FS_16000:
			wSrcRate	= MCDRV_DIT_SRCRATE_16000_AA;
			break;
		case MCDRV_FS_12000:
			wSrcRate	= MCDRV_DIT_SRCRATE_12000_AA;
			break;
		case MCDRV_FS_11025:
			wSrcRate	= MCDRV_DIT_SRCRATE_11025_AA;
			break;
		case MCDRV_FS_8000:
			wSrcRate	= MCDRV_DIT_SRCRATE_8000_AA;
			break;
		default:
			/* unreachable */
			wSrcRate = 0;
			break;
		}
	}
	/*	DITSRC_RATE*	*/
	bReg	= (UINT8)(wSrcRate>>8);
	if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DITSRC_RATE0_MSB+bRegOffset))
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_DITSRC_RATE0_MSB+bRegOffset), bReg);
	}
	bReg	= (UINT8)wSrcRate;
	if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DITSRC_RATE0_LSB+bRegOffset))
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_DITSRC_RATE0_LSB+bRegOffset), bReg);
	}

	if(sDioInfo.asPortInfo[ePort].sDioCommon.bInterface == MCDRV_DIO_DA)
	{
		/*	DIT*_FMT, DIT*_BIT, DIR*_FMT, DIR*_BIT	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bMode << 6)
				| (sDioInfo.asPortInfo[ePort].sDit.sDaFormat.bBitSel << 4)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bMode << 2)
				| (sDioInfo.asPortInfo[ePort].sDir.sDaFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIX0_FMT+bRegOffset))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_DIX0_FMT+bRegOffset), bReg);
		}

		/*	DIT*_SLOT	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDit.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT0_SLOT+bRegOffset))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_DIT0_SLOT+bRegOffset), bReg);
		}
	}
	else
	{
		/*	PCM_MONO_TX*, PCM_EXTEND_TX*, PCM_LSBON_TX*, PCM_LAW_TX*, PCM_BIT_TX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bMono << 7)
				| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bOrder << 4)
				| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bLaw << 2)
				| (sDioInfo.asPortInfo[ePort].sDit.sPcmFormat.bBitSel);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PCM_TX0+bRegOffset))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_PCM_TX0+bRegOffset), bReg);
		}

		/*	PCM_CH1_TX*, PCM_CH0_TX*	*/
		bReg	= (sDioInfo.asPortInfo[ePort].sDit.abSlot[1] << 4) | (sDioInfo.asPortInfo[ePort].sDit.abSlot[0]);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PCM_SLOT_TX0+bRegOffset))
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)(MCI_AA_PCM_SLOT_TX0+bRegOffset), bReg);
		}
	}
}

/****************************************************************************
 *	McPacket_AddDAC_AA
 *
 *	Description:
 *			Add DAC setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddDAC_AA
(
	UINT32			dUpdateInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	MCDRV_POWER_INFO_AA	sPowerInfo;
	MCDRV_POWER_UPDATE_AA	sPowerUpdate;
	MCDRV_DAC_INFO		sDacInfo;
	UINT8	bReg;

	McResCtrl_GetDacInfo_AA(&sDacInfo);

	if((dUpdateInfo & MCDRV_DAC_MSWP_UPDATE_FLAG) != (UINT32)0 || (dUpdateInfo & MCDRV_DAC_VSWP_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (sDacInfo.bMasterSwap<<4)|sDacInfo.bVoiceSwap;
		if(bReg == McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_SWP))
		{
			dUpdateInfo	&= ~(MCDRV_DAC_MSWP_UPDATE_FLAG|MCDRV_DAC_VSWP_UPDATE_FLAG);
		}
	}
	if((dUpdateInfo & MCDRV_DAC_HPF_UPDATE_FLAG) != (UINT32)0)
	{
		if(sDacInfo.bDcCut == McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_CODEC_AA, MCI_AA_DCCUTOFF))
		{
			dUpdateInfo	&= ~(MCDRV_DAC_HPF_UPDATE_FLAG);
		}
	}
	if(dUpdateInfo == (UINT32)0)
	{
		return sdRet;
	}

	McResCtrl_GetCurPowerInfo_AA(&sPowerInfo);
	sdRet	= PowerUpDig(MCDRV_DPB_UP_AA);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}

	if((dUpdateInfo & MCDRV_DAC_MSWP_UPDATE_FLAG) != (UINT32)0 || (dUpdateInfo & MCDRV_DAC_VSWP_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (sDacInfo.bMasterSwap<<4)|sDacInfo.bVoiceSwap;
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_SWP, bReg);
	}
	if((dUpdateInfo & MCDRV_DAC_HPF_UPDATE_FLAG) != (UINT32)0)
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_DCCUTOFF, sDacInfo.bDcCut);
	}

	/*	unused path power down	*/
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
	sPowerUpdate.abAnalog[0]	=
	sPowerUpdate.abAnalog[1]	=
	sPowerUpdate.abAnalog[2]	=
	sPowerUpdate.abAnalog[3]	=
	sPowerUpdate.abAnalog[4]	= 0;
	return McPacket_AddPowerDown_AA(&sPowerInfo, &sPowerUpdate);
}

/****************************************************************************
 *	McPacket_AddADC_AA
 *
 *	Description:
 *			Add ADC setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddADC_AA
(
	UINT32			dUpdateInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	MCDRV_POWER_INFO_AA	sPowerInfo;
	MCDRV_POWER_UPDATE_AA	sPowerUpdate;
	MCDRV_ADC_INFO		sAdcInfo;

	McResCtrl_GetAdcInfo_AA(&sAdcInfo);

	if((dUpdateInfo & MCDRV_ADCADJ_UPDATE_FLAG) != (UINT32)0 || (dUpdateInfo & MCDRV_ADCAGC_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (sAdcInfo.bAgcOn<<2)|sAdcInfo.bAgcAdjust;
		if(bReg == McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_CODEC_AA, MCI_AA_AD_AGC))
		{
			dUpdateInfo	&= ~(MCDRV_ADCADJ_UPDATE_FLAG|MCDRV_ADCAGC_UPDATE_FLAG);
		}
	}
	if((dUpdateInfo & MCDRV_ADCMONO_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_CODEC_AA, MCI_AA_AD_START) & MCB_AA_AD_START) | (sAdcInfo.bMono << 1);
		if(bReg == McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_CODEC_AA, MCI_AA_AD_START))
		{
			dUpdateInfo	&= ~(MCDRV_ADCMONO_UPDATE_FLAG);
		}
	}

	if(dUpdateInfo == (UINT32)0)
	{
		return sdRet;
	}

	McResCtrl_GetCurPowerInfo_AA(&sPowerInfo);
	sdRet	= PowerUpDig(MCDRV_DPB_UP_AA);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}

	if((dUpdateInfo & MCDRV_ADCADJ_UPDATE_FLAG) != (UINT32)0 || (dUpdateInfo & MCDRV_ADCAGC_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (sAdcInfo.bAgcOn<<2)|sAdcInfo.bAgcAdjust;
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_CODEC_AA, MCI_AA_AD_AGC))
		{
			AddStopADC();
			sdRet	= McDevIf_ExecutePacket_AA();
			if(MCDRV_SUCCESS != sdRet)
			{
				return sdRet;
			}
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_AD_AGC, bReg);
	}
	if((dUpdateInfo & MCDRV_ADCMONO_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_CODEC_AA, MCI_AA_AD_START) & MCB_AA_AD_START) | (sAdcInfo.bMono << 1);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_CODEC_AA, MCI_AA_AD_START))
		{
			AddStopADC();
			sdRet	= McDevIf_ExecutePacket_AA();
			if(MCDRV_SUCCESS != sdRet)
			{
				return sdRet;
			}
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | MCI_AA_AD_START, (sAdcInfo.bMono << 1));
	}

	/*	unused path power down	*/
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
	sPowerUpdate.abAnalog[0]	=
	sPowerUpdate.abAnalog[1]	=
	sPowerUpdate.abAnalog[2]	=
	sPowerUpdate.abAnalog[3]	=
	sPowerUpdate.abAnalog[4]	= 0;
	sdRet	= McPacket_AddPowerDown_AA(&sPowerInfo, &sPowerUpdate);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}
	return McPacket_AddStart_AA();
}

/****************************************************************************
 *	McPacket_AddSP_AA
 *
 *	Description:
 *			Add SP setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
SINT32	McPacket_AddSP_AA
(
	void
)
{
	MCDRV_SP_INFO	sSpInfo;
	UINT8	bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_SP_MODE) & (UINT8)~MCB_AA_SP_SWAP;

	McResCtrl_GetSpInfo_AA(&sSpInfo);

	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_SP_MODE, bReg|sSpInfo.bSwap);
	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McPacket_AddDNG_AA
 *
 *	Description:
 *			Add Digital Noise Gate setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddDNG_AA
(
	UINT32			dUpdateInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	MCDRV_POWER_INFO_AA	sCurPowerInfo;
	MCDRV_POWER_INFO_AA	sPowerInfo;
	MCDRV_POWER_UPDATE_AA	sPowerUpdate;
	MCDRV_DNG_INFO		sDngInfo;
	UINT8	bReg;

	McResCtrl_GetDngInfo_AA(&sDngInfo);

	if((dUpdateInfo & MCDRV_DNGREL_HP_UPDATE_FLAG) != (UINT32)0 || (dUpdateInfo & MCDRV_DNGATK_HP_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (sDngInfo.abRelease[0]<<4)|sDngInfo.abAttack[0];
		if(bReg == McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_DNGATRT))
		{
			dUpdateInfo	&= ~(MCDRV_DNGREL_HP_UPDATE_FLAG|MCDRV_DNGATK_HP_UPDATE_FLAG);
		}
	}
	if((dUpdateInfo & MCDRV_DNGSW_HP_UPDATE_FLAG) != (UINT32)0
	|| (dUpdateInfo & MCDRV_DNGTHRES_HP_UPDATE_FLAG) != (UINT32)0
	|| (dUpdateInfo & MCDRV_DNGHOLD_HP_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (sDngInfo.abThreshold[0]<<4)|(sDngInfo.abHold[0]<<1)|sDngInfo.abOnOff[0];
		if(bReg == McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_ANA_AA, MCI_AA_DNGON))
		{
			dUpdateInfo	&= ~(MCDRV_DNGSW_HP_UPDATE_FLAG|MCDRV_DNGTHRES_HP_UPDATE_FLAG|MCDRV_DNGHOLD_HP_UPDATE_FLAG);
		}
	}
	if(dUpdateInfo == (UINT32)0)
	{
		return sdRet;
	}

	McResCtrl_GetCurPowerInfo_AA(&sCurPowerInfo);
	sPowerInfo	= sCurPowerInfo;
	sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP0_AA;
	sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP1_AA;
	sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP2_AA;
	sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_PLLRST0_AA;
	sPowerUpdate.dDigital	= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
	if((dUpdateInfo & MCDRV_DNGREL_HP_UPDATE_FLAG) != (UINT32)0 || (dUpdateInfo & MCDRV_DNGATK_HP_UPDATE_FLAG) != (UINT32)0)
	{
		sPowerInfo.abAnalog[0]	&= (UINT8)~MCB_AA_PWM_VR;
		sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL_AA;
		sPowerUpdate.abAnalog[1]	=
		sPowerUpdate.abAnalog[2]	=
		sPowerUpdate.abAnalog[3]	=
		sPowerUpdate.abAnalog[4]	= 0;
	}
	sdRet	= McPacket_AddPowerUp_AA(&sPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	if((dUpdateInfo & MCDRV_DNGREL_HP_UPDATE_FLAG) != (UINT32)0 || (dUpdateInfo & MCDRV_DNGATK_HP_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (sDngInfo.abRelease[0]<<4)|sDngInfo.abAttack[0];
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_DNGATRT, bReg);
	}
	if((dUpdateInfo & MCDRV_DNGSW_HP_UPDATE_FLAG) != (UINT32)0
	|| (dUpdateInfo & MCDRV_DNGTHRES_HP_UPDATE_FLAG) != (UINT32)0
	|| (dUpdateInfo & MCDRV_DNGHOLD_HP_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (sDngInfo.abThreshold[0]<<4)|(sDngInfo.abHold[0]<<1)|sDngInfo.abOnOff[0];
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | MCI_AA_DNGON, bReg);
	}

	/*	restore power	*/
	return McPacket_AddPowerDown_AA(&sCurPowerInfo, &sPowerUpdate);
}

/****************************************************************************
 *	McPacket_AddAE_AA
 *
 *	Description:
 *			Add Audio Engine setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddAE_AA
(
	UINT32	dUpdateInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg;
	UINT8	i;
	UINT32	dXFadeParam	= 0;
	MCDRV_AE_INFO	sAeInfo;
	MCDRV_PATH_INFO	sPathInfo;
	MCDRV_POWER_INFO_AA	sPowerInfo;
	MCDRV_POWER_UPDATE_AA	sPowerUpdate;

	McResCtrl_GetPathInfo_AA(&sPathInfo);
	McResCtrl_GetAeInfo_AA(&sAeInfo);

	if(McResCtrl_IsDstUsed_AA(eMCDRV_DST_AE_AA, eMCDRV_DST_CH0_AA) == 1)
	{/*	AE is used	*/
		bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_A_AA, MCI_AA_BDSP_ST);
		if(McDevProf_IsValid(eMCDRV_FUNC_DBEX) == 1)
		{
			if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_BEXWIDE_ONOFF) != (UINT32)0)
			{
				if(((sAeInfo.bOnOff & MCDRV_BEXWIDE_ON) != 0 && (bReg & MCB_AA_DBEXON) != 0)
				|| ((sAeInfo.bOnOff & MCDRV_BEXWIDE_ON) == 0 && (bReg & MCB_AA_DBEXON) == 0))
				{
					dUpdateInfo	&= ~MCDRV_AEUPDATE_FLAG_BEXWIDE_ONOFF;
				}
			}
		}
		if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_DRC_ONOFF) != (UINT32)0)
		{
			if(((sAeInfo.bOnOff & MCDRV_DRC_ON) != 0 && (bReg & MCB_AA_DRCON) != 0)
			|| ((sAeInfo.bOnOff & MCDRV_DRC_ON) == 0 && (bReg & MCB_AA_DRCON) == 0))
			{
				dUpdateInfo	&= ~MCDRV_AEUPDATE_FLAG_DRC_ONOFF;
			}
		}
		if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ5_ONOFF) != (UINT32)0)
		{
			if(((sAeInfo.bOnOff & MCDRV_EQ5_ON) != 0 && (bReg & MCB_AA_EQ5ON) != 0)
			|| ((sAeInfo.bOnOff & MCDRV_EQ5_ON) == 0 && (bReg & MCB_AA_EQ5ON) == 0))
			{
				dUpdateInfo	&= ~MCDRV_AEUPDATE_FLAG_EQ5_ONOFF;
			}
		}
		if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ3_ONOFF) != (UINT32)0)
		{
			if(((sAeInfo.bOnOff & MCDRV_EQ3_ON) != 0 && (bReg & MCB_AA_EQ3ON) != 0)
			|| ((sAeInfo.bOnOff & MCDRV_EQ3_ON) == 0 && (bReg & MCB_AA_EQ3ON) == 0))
			{
				dUpdateInfo	&= ~MCDRV_AEUPDATE_FLAG_EQ3_ONOFF;
			}
		}
		if(dUpdateInfo == (UINT32)0)
		{
			return sdRet;
		}

		/*	on/off setting or param changed	*/
		if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ5) != (UINT32)0
		|| (dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ3) != (UINT32)0
		|| (dUpdateInfo & MCDRV_AEUPDATE_FLAG_BEX) != (UINT32)0
		|| (dUpdateInfo & MCDRV_AEUPDATE_FLAG_WIDE) != (UINT32)0
		|| (dUpdateInfo & MCDRV_AEUPDATE_FLAG_DRC) != (UINT32)0
		|| (dUpdateInfo & MCDRV_AEUPDATE_FLAG_BEXWIDE_ONOFF) != (UINT32)0
		|| (dUpdateInfo & MCDRV_AEUPDATE_FLAG_DRC_ONOFF) != (UINT32)0
		|| (dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ5_ONOFF) != (UINT32)0
		|| (dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ3_ONOFF) != (UINT32)0)
		{
			dXFadeParam	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DAC_INS);
			dXFadeParam	<<= 8;
			dXFadeParam	|= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_INS);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)MCI_AA_DAC_INS, 0);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)MCI_AA_INS, 0);
		}
		sdRet	= McDevIf_ExecutePacket_AA();
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}

		/*	wait xfade complete	*/
		if(dXFadeParam != (UINT32)0)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_EVTWAIT_AA | MCDRV_EVT_INSFLG_AA | dXFadeParam, 0);
			bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_A_AA, MCI_AA_BDSP_ST);
			if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ5) != (UINT32)0)
			{
				bReg	&= (UINT8)~MCB_AA_EQ5ON;
			}
			if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ3) != (UINT32)0)
			{
				bReg	&= (UINT8)~MCB_AA_EQ3ON;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | (UINT32)MCI_AA_BDSP_ST, bReg);
		}

		bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_A_AA, MCI_AA_BDSP_ST);
		if((bReg & MCB_AA_BDSP_ST) == MCB_AA_BDSP_ST)
		{
			/*	Stop BDSP	*/
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | (UINT32)MCI_AA_BDSP_ST, 0);
			/*	Reset TRAM	*/
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | (UINT32)MCI_AA_BDSP_RST, MCB_AA_TRAM_RST);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | (UINT32)MCI_AA_BDSP_RST, 0);
		}
	}

	if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_DRC) != (UINT32)0)
	{
		McResCtrl_GetPowerInfo_AA(&sPowerInfo);
		sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP0_AA;
		sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP1_AA;
		sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP2_AA;
		sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DPBDSP_AA;
		sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_PLLRST0_AA;
		sPowerUpdate.dDigital	= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
		sPowerUpdate.abAnalog[0]	=
		sPowerUpdate.abAnalog[1]	=
		sPowerUpdate.abAnalog[2]	=
		sPowerUpdate.abAnalog[3]	=
		sPowerUpdate.abAnalog[4]	= 0;
		sdRet	= McPacket_AddPowerUp_AA(&sPowerInfo, &sPowerUpdate);
		if(MCDRV_SUCCESS != sdRet)
		{
			return sdRet;
		}
		sdRet	= McDevIf_ExecutePacket_AA();
		if(MCDRV_SUCCESS != sdRet)
		{
			return sdRet;
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | (UINT32)MCI_AA_BDSP_ADR, 0);
		sdRet	= McDevIf_ExecutePacket_AA();
		if(MCDRV_SUCCESS != sdRet)
		{
			return sdRet;
		}
		McDevIf_AddPacketRepeat_AA(MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | (UINT32)MCI_AA_BDSP_WINDOW, sAeInfo.abDrc, DRC_PARAM_SIZE);
	}

	if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ5) != (UINT32)0)
	{
		for(i = 0; i < EQ5_PARAM_SIZE; i++)
		{
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_AE_AA | (UINT32)(MCI_AA_BAND0_CEQ0+i), sAeInfo.abEq5[i]);
		}
	}
	if((dUpdateInfo & MCDRV_AEUPDATE_FLAG_EQ3) != (UINT32)0)
	{
		if(McDevProf_IsValid(eMCDRV_FUNC_HWADJ) == 1)
		{
			for(i = 0; i < 15; i++)
			{
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_AE_AA | (UINT32)(MCI_AA_BAND5_CEQ0+i), sAeInfo.abEq3[i]);
			}
		}
		else
		{
			for(i = 0; i < EQ3_PARAM_SIZE; i++)
			{
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_AE_AA | (UINT32)(MCI_AA_BAND5_CEQ0+i), sAeInfo.abEq3[i]);
			}
		}
	}

	return sdRet;
}

/****************************************************************************
 *	McPacket_AddPDM_AA
 *
 *	Description:
 *			Add PDM setup packet.
 *	Arguments:
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
SINT32	McPacket_AddPDM_AA
(
	UINT32			dUpdateInfo
)
{
	SINT32				sdRet		= MCDRV_SUCCESS;
	UINT8				bReg;
	MCDRV_POWER_INFO_AA	sPowerInfo;
	MCDRV_POWER_UPDATE_AA	sPowerUpdate;
	MCDRV_PDM_INFO		sPdmInfo;

	McResCtrl_GetPdmInfo_AA(&sPdmInfo);
	if((dUpdateInfo & MCDRV_PDMADJ_UPDATE_FLAG) != (UINT32)0 || (dUpdateInfo & MCDRV_PDMAGC_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (sPdmInfo.bAgcOn<<2)|sPdmInfo.bAgcAdjust;
		if(bReg == McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PDM_AGC))
		{
			dUpdateInfo &= ~(MCDRV_PDMADJ_UPDATE_FLAG|MCDRV_PDMAGC_UPDATE_FLAG);
		}
	}
	if((dUpdateInfo & MCDRV_PDMMONO_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PDM_START) & (UINT8)MCB_AA_PDM_MN;
		if((sPdmInfo.bMono<<1) == bReg)
		{
			dUpdateInfo &= ~(MCDRV_PDMMONO_UPDATE_FLAG);
		}
	}
	if((dUpdateInfo & MCDRV_PDMCLK_UPDATE_FLAG) != (UINT32)0
	|| (dUpdateInfo & MCDRV_PDMEDGE_UPDATE_FLAG) != (UINT32)0
	|| (dUpdateInfo & MCDRV_PDMWAIT_UPDATE_FLAG) != (UINT32)0
	|| (dUpdateInfo & MCDRV_PDMSEL_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (sPdmInfo.bPdmWait<<5) | (sPdmInfo.bPdmEdge<<4) | (sPdmInfo.bPdmSel<<2) | sPdmInfo.bClk;
		if(bReg == McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PDM_STWAIT))
		{
			dUpdateInfo &= ~(MCDRV_PDMCLK_UPDATE_FLAG|MCDRV_PDMEDGE_UPDATE_FLAG|MCDRV_PDMWAIT_UPDATE_FLAG|MCDRV_PDMSEL_UPDATE_FLAG);
		}
	}
	if(dUpdateInfo == (UINT32)0)
	{
		return MCDRV_SUCCESS;
	}

	McResCtrl_GetCurPowerInfo_AA(&sPowerInfo);
	sdRet	= PowerUpDig(MCDRV_DPB_UP_AA);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}

	if((dUpdateInfo & MCDRV_PDMADJ_UPDATE_FLAG) != (UINT32)0 || (dUpdateInfo & MCDRV_PDMAGC_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (sPdmInfo.bAgcOn<<2)|sPdmInfo.bAgcAdjust;
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PDM_AGC))
		{
			AddStopPDM();
			sdRet	= McDevIf_ExecutePacket_AA();
			if(MCDRV_SUCCESS != sdRet)
			{
				return sdRet;
			}
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM_AGC, bReg);
	}
	if((dUpdateInfo & MCDRV_PDMMONO_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PDM_START) & (UINT8)~MCB_AA_PDM_MN) | (sPdmInfo.bMono<<1);
		if(bReg != McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_PDM_START))
		{
			AddStopPDM();
			sdRet	= McDevIf_ExecutePacket_AA();
			if(MCDRV_SUCCESS != sdRet)
			{
				return sdRet;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM_START, (sPdmInfo.bMono<<1));
		}
	}
	if((dUpdateInfo & MCDRV_PDMCLK_UPDATE_FLAG) != (UINT32)0
	|| (dUpdateInfo & MCDRV_PDMEDGE_UPDATE_FLAG) != (UINT32)0
	|| (dUpdateInfo & MCDRV_PDMWAIT_UPDATE_FLAG) != (UINT32)0
	|| (dUpdateInfo & MCDRV_PDMSEL_UPDATE_FLAG) != (UINT32)0)
	{
		bReg	= (sPdmInfo.bPdmWait<<5) | (sPdmInfo.bPdmEdge<<4) | (sPdmInfo.bPdmSel<<2) | sPdmInfo.bClk;
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_PDM_STWAIT, bReg);
	}

	/*	unused path power down	*/
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
	sPowerUpdate.abAnalog[0]	=
	sPowerUpdate.abAnalog[1]	=
	sPowerUpdate.abAnalog[2]	=
	sPowerUpdate.abAnalog[3]	=
	sPowerUpdate.abAnalog[4]	= 0;
	sdRet	= McPacket_AddPowerDown_AA(&sPowerInfo, &sPowerUpdate);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}
	return McPacket_AddStart_AA();
}

/****************************************************************************
 *	McPacket_AddGPMode_AA
 *
 *	Description:
 *			Add GP mode setup packet.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
SINT32	McPacket_AddGPMode_AA
(
	void
)
{
	UINT8	abReg[2];
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_GP_MODE	sGPMode;

	McResCtrl_GetInitInfo_AA(&sInitInfo);
	McResCtrl_GetGPMode_AA(&sGPMode);

	abReg[0]	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PA_MSK);
	abReg[1]	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PA_MSK_1);

	if(sInitInfo.bPad0Func == MCDRV_PAD_GPIO)
	{
		if(sGPMode.abGpDdr[0] == MCDRV_GPDDR_IN)
		{
			abReg[1]	&= (UINT8)~MCB_AA_PA0_DDR;
		}
		else
		{
			abReg[1]	|= MCB_AA_PA0_DDR;
		}
	}
	if(sInitInfo.bPad1Func == MCDRV_PAD_GPIO)
	{
		if(sGPMode.abGpDdr[1] == MCDRV_GPDDR_IN)
		{
			abReg[1]	&= (UINT8)~MCB_AA_PA1_DDR;
		}
		else
		{
			abReg[1]	|= MCB_AA_PA1_DDR;
		}
	}
/*
	if(sInitInfo.bPad2Func == MCDRV_PAD_GPIO)
	{
		if(sGPMode.abGpDdr[2] == MCDRV_GPDDR_IN)
		{
			abReg[0]	&= (UINT8)~MCB_AA_PA2_DDR;
		}
		else
		{
			abReg[0]	|= MCB_AA_PA2_DDR;
		}
	}
*/
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PA_MSK, abReg[0]);
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PA_MSK_1, abReg[1]);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McPacket_AddGPMask_AA
 *
 *	Description:
 *			Add GP mask setup packet.
 *	Arguments:
 *			dPadNo		PAD number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
SINT32	McPacket_AddGPMask_AA
(
	UINT32	dPadNo
)
{
	UINT8	bReg;
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_GP_MODE	sGPMode;
	UINT8	abMask[GPIO_PAD_NUM];

	McResCtrl_GetInitInfo_AA(&sInitInfo);
	McResCtrl_GetGPMode_AA(&sGPMode);
	McResCtrl_GetGPMask_AA(abMask);

	if(dPadNo == MCDRV_GP_PAD0)
	{
		if(sInitInfo.bPad0Func == MCDRV_PAD_GPIO && sGPMode.abGpDdr[0] == MCDRV_GPDDR_IN)
		{
			bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PA_MSK_1);
			if(abMask[0] == MCDRV_GPMASK_OFF)
			{
				bReg	&= (UINT8)~MCB_AA_PA0_MSK;
			}
			else
			{
				bReg	|= MCB_AA_PA0_MSK;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PA_MSK_1, bReg);
		}
	}
	else if(dPadNo == MCDRV_GP_PAD1)
	{
		if(sInitInfo.bPad1Func == MCDRV_PAD_GPIO && sGPMode.abGpDdr[1] == MCDRV_GPDDR_IN)
		{
			bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PA_MSK_1);
			if(abMask[1] == MCDRV_GPMASK_OFF)
			{
				bReg	&= (UINT8)~MCB_AA_PA1_MSK;
			}
			else
			{
				bReg	|= MCB_AA_PA1_MSK;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PA_MSK_1, bReg);
		}
	}
/*
	else if(dPadNo == MCDRV_GP_PAD2)
	{
		if(sInitInfo.bPad2Func == MCDRV_PAD_GPIO && sGPMode.abGpDdr[2] == MCDRV_GPDDR_IN)
		{
			bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PA_MSK);
			if(abMask[2] == MCDRV_GPMASK_OFF)
			{
				bReg	&= (UINT8)~MCB_AA_PA2_MSK;
			}
			else
			{
				bReg	|= MCB_AA_PA2_MSK;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PA_MSK, bReg);
		}
	}
*/
	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McPacket_AddGPSet_AA
 *
 *	Description:
 *			Add GPIO output packet.
 *	Arguments:
 *			bGpio		pin state setting
 *			dPadNo		PAD number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
SINT32	McPacket_AddGPSet_AA
(
	UINT8	bGpio,
	UINT32	dPadNo
)
{
	UINT8	bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_BASE_AA, MCI_AA_PA_SCU_PA);

	if(dPadNo == MCDRV_GP_PAD0)
	{
		if(bGpio == MCDRV_GP_LOW)
		{
			bReg	&= (UINT8)~MCB_AA_PA_SCU_PA0;
		}
		else if(bGpio == MCDRV_GP_HIGH)
		{
			bReg	|= MCB_AA_PA_SCU_PA0;
		}
	}
	else if(dPadNo == MCDRV_GP_PAD1)
	{
		if(bGpio == MCDRV_GP_LOW)
		{
			bReg	&= (UINT8)~MCB_AA_PA_SCU_PA1;
		}
		else if(bGpio == MCDRV_GP_HIGH)
		{
			bReg	|= MCB_AA_PA_SCU_PA1;
		}
	}
/*
	else if(dPadNo == MCDRV_GP_PAD2)
	{
		if(bGpio == MCDRV_GP_LOW)
		{
			bReg	&= (UINT8)~MCB_AA_PA_SCU_PA2;
		}
		else if(bGpio == MCDRV_GP_HIGH)
		{
			bReg	|= MCB_AA_PA_SCU_PA2;
		}
	}
*/
	McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | MCI_AA_PA_SCU_PA, bReg);

	return MCDRV_SUCCESS;
}


/****************************************************************************
 *	PowerUpDig
 *
 *	Description:
 *			Digital power up.
 *	Arguments:
 *			bDPBUp		1:DPB power up
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_TIMEOUT
 *
 ****************************************************************************/
static SINT32	PowerUpDig
(
	UINT8	bDPBUp
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	MCDRV_POWER_INFO_AA	sPowerInfo;
	MCDRV_POWER_UPDATE_AA	sPowerUpdate;

	McResCtrl_GetCurPowerInfo_AA(&sPowerInfo);
	sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP0_AA;
	sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP1_AA;
	sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DP2_AA;
	sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_PLLRST0_AA;
	if(bDPBUp == MCDRV_DPB_UP_AA)
	{
		sPowerInfo.dDigital	&= ~MCDRV_POWINFO_DIGITAL_DPB_AA;
	}
	sPowerUpdate.dDigital	= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
	sdRet	= McPacket_AddPowerUp_AA(&sPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	return McDevIf_ExecutePacket_AA();
}

/****************************************************************************
 *	GetMaxWait
 *
 *	Description:
 *			Get maximum wait time.
 *	Arguments:
 *			bRegChange	analog power management register update information
 *	Return:
 *			wait time
 *
 ****************************************************************************/
static UINT32	GetMaxWait
(
	UINT8	bRegChange
)
{
	UINT32	dWaitTimeMax	= 0;
	MCDRV_INIT_INFO	sInitInfo;

	McResCtrl_GetInitInfo_AA(&sInitInfo);

	if((bRegChange & MCB_AA_PWM_LI) != 0)
	{
		if(sInitInfo.sWaitTime.dLine1Cin > dWaitTimeMax)
		{
			dWaitTimeMax	= sInitInfo.sWaitTime.dLine1Cin;
		}
	}
	if((bRegChange & MCB_AA_PWM_MB1) != 0 || (bRegChange & MCB_AA_PWM_MC1) != 0)
	{
		if(sInitInfo.sWaitTime.dMic1Cin > dWaitTimeMax)
		{
			dWaitTimeMax	= sInitInfo.sWaitTime.dMic1Cin;
		}
	}
	if((bRegChange & MCB_AA_PWM_MB2) != 0 || (bRegChange & MCB_AA_PWM_MC2) != 0)
	{
		if(sInitInfo.sWaitTime.dMic2Cin > dWaitTimeMax)
		{
			dWaitTimeMax	= sInitInfo.sWaitTime.dMic2Cin;
		}
	}
	if((bRegChange & MCB_AA_PWM_MB3) != 0 || (bRegChange & MCB_AA_PWM_MC3) != 0)
	{
		if(sInitInfo.sWaitTime.dMic3Cin > dWaitTimeMax)
		{
			dWaitTimeMax	= sInitInfo.sWaitTime.dMic3Cin;
		}
	}

	return dWaitTimeMax;
}

/*******************************/

#define	MCDRV_MAX_WAIT_TIME_AA	(0x0FFFFFFFUL)

static SINT32	init					(const MCDRV_INIT_INFO* psInitInfo);
static SINT32	term					(void);

static SINT32	read_reg				(MCDRV_REG_INFO* psRegInfo);
static SINT32	write_reg				(const MCDRV_REG_INFO* psRegInfo);

static SINT32	update_clock			(const MCDRV_CLOCK_INFO* psClockInfo);

static SINT32	get_path				(MCDRV_PATH_INFO* psPathInfo);
static SINT32	set_path				(const MCDRV_PATH_INFO* psPathInfo);

static SINT32	get_volume				(MCDRV_VOL_INFO* psVolInfo);
static SINT32	set_volume				(const MCDRV_VOL_INFO *psVolInfo);

static SINT32	get_digitalio			(MCDRV_DIO_INFO* psDioInfo);
static SINT32	set_digitalio			(const MCDRV_DIO_INFO* psDioInfo, UINT32 dUpdateInfo);

static SINT32	get_dac					(MCDRV_DAC_INFO* psDacInfo);
static SINT32	set_dac					(const MCDRV_DAC_INFO* psDacInfo, UINT32 dUpdateInfo);

static SINT32	get_adc					(MCDRV_ADC_INFO* psAdcInfo);
static SINT32	set_adc					(const MCDRV_ADC_INFO* psAdcInfo, UINT32 dUpdateInfo);

static SINT32	get_sp					(MCDRV_SP_INFO* psSpInfo);
static SINT32	set_sp					(const MCDRV_SP_INFO* psSpInfo);

static SINT32	get_dng					(MCDRV_DNG_INFO* psDngInfo);
static SINT32	set_dng					(const MCDRV_DNG_INFO* psDngInfo, UINT32 dUpdateInfo);

static SINT32	set_ae					(const MCDRV_AE_INFO* psAeInfo, UINT32 dUpdateInfo);

static SINT32	get_pdm					(MCDRV_PDM_INFO* psPdmInfo);
static SINT32	set_pdm					(const MCDRV_PDM_INFO* psPdmInfo, UINT32 dUpdateInfo);

static SINT32	config_gp				(const MCDRV_GP_MODE* psGpMode);
static SINT32	mask_gp					(UINT8* pbMask, UINT32 dPadNo);
static SINT32	getset_gp				(UINT8* pbGpio, UINT32 dPadNo);

static SINT32	ValidateInitParam		(const MCDRV_INIT_INFO* psInitInfo);
static SINT32	ValidateClockParam		(const MCDRV_CLOCK_INFO* psClockInfo);
static SINT32	ValidateReadRegParam	(const MCDRV_REG_INFO* psRegInfo);
static SINT32	ValidateWriteRegParam	(const MCDRV_REG_INFO* psRegInfo);
static SINT32	ValidatePathParam		(const MCDRV_PATH_INFO* psPathInfo);
static SINT32	ValidateDioParam		(const MCDRV_DIO_INFO* psDioInfo, UINT32 dUpdateInfo);
static SINT32	CheckDIOCommon			(const MCDRV_DIO_INFO*	psDioInfo, UINT8 bPort);
static SINT32	CheckDIODIR				(const MCDRV_DIO_INFO*	psDioInfo, UINT8 bPort, UINT8 bInterface);
static SINT32	CheckDIODIT				(const MCDRV_DIO_INFO*	psDioInfo, UINT8 bPort, UINT8 bInterface);

static SINT32	SetVol					(UINT32 dUpdate, MCDRV_VOLUPDATE_MODE_AA eMode);
static	SINT32	PreUpdatePath			(void);

static UINT8	abHwAdjParam_Thru[60] =
{
	0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
static UINT8	abHwAdjParam_Rec8[60] =
{
	0x10,0x07,0x79,0xEA,0xEB,0xAA,0xE3,0xBE,0xD3,0x9D,
	0x80,0xF4,0x0F,0x46,0x5C,0xEE,0x26,0x04,0x1C,0x41,
	0x2C,0x62,0x7F,0x0E,0xF0,0xB2,0x29,0x26,0xEE,0x56,
	0x10,0x00,0xCD,0x1A,0xC6,0x9C,0xE2,0x35,0xC9,0x28,
	0x8C,0x62,0x0F,0x74,0xD1,0x4A,0x2F,0x16,0x1D,0xCA,
	0x36,0xD7,0x73,0xA0,0xF0,0x8A,0x61,0x9B,0x0A,0x50
};
static UINT8	abHwAdjParam_Rec44[60] =
{
	0x10,0x83,0x6A,0x3F,0x68,0x46,0x19,0x89,0xC7,0xB0,
	0x55,0x42,0x0C,0xFF,0x7A,0xAB,0x42,0x3A,0xE6,0x76,
	0x38,0x4F,0xAA,0xC0,0xF2,0x7D,0x1B,0x15,0x55,0x82,
	0x10,0x69,0xF3,0x99,0x9D,0x9E,0x08,0x63,0x05,0xAA,
	0x11,0xB8,0xFB,0x72,0x65,0x70,0x71,0x56,0xF7,0x7F,
	0x4C,0x6D,0x22,0xCA,0x04,0x41,0x54,0xDE,0xBC,0x90
};
static UINT8	abHwAdjParam_Rec48[60] =
{
	0x10,0x58,0xEB,0x76,0xA5,0x22,0x19,0x40,0x9A,0xF9,
	0x6A,0x26,0x0C,0xCF,0xC3,0x76,0x0C,0x38,0xE6,0xBF,
	0x65,0x06,0x95,0xDC,0xF2,0xD7,0x51,0x13,0x4E,0xA6,
	0x10,0x46,0x7F,0x35,0x83,0x86,0x08,0x50,0xE8,0x01,
	0xBD,0x14,0xFB,0x7C,0x3B,0x31,0x2A,0xD4,0xF7,0x9B,
	0x3F,0xDC,0xC7,0xC8,0x04,0x51,0x1D,0xBA,0xCC,0xCC
};
static UINT8	abHwAdjParam_Play8[60] =
{
	0x10,0x17,0xAE,0x9C,0x5E,0x76,0xE4,0x96,0x5C,0x5C,
	0xE7,0x7E,0x0F,0x53,0x7E,0x85,0xBC,0xD0,0x1B,0x69,
	0xA3,0xA3,0x18,0x84,0xF0,0x94,0xD2,0xDD,0xE4,0xBC,
	0x10,0x11,0x1E,0x9A,0xB6,0xCE,0xE4,0x2A,0x1A,0xE5,
	0x81,0x80,0x0F,0x62,0xDA,0x03,0x5A,0xB4,0x1B,0xD5,
	0xE5,0x1A,0x7E,0x82,0xF0,0x8C,0x07,0x61,0xEE,0x80
};
static UINT8	abHwAdjParam_Play44[60] =
{
	0x10,0x6D,0x19,0x4D,0xE6,0xFA,0x19,0x7A,0xF7,0x6F,
	0x4E,0x18,0x0D,0x04,0xAD,0x61,0x04,0xD2,0xE6,0x85,
	0x08,0x90,0xB1,0xEA,0xF2,0x8E,0x39,0x51,0x14,0x36,
	0x10,0xA7,0x40,0x2A,0x76,0x8E,0x0B,0xEA,0x7D,0x17,
	0x6A,0x44,0xFE,0x5D,0xCE,0x5E,0x8C,0x5A,0xF3,0xE6,
	0x41,0x04,0x5A,0x44,0x01,0x2A,0x33,0x5B,0x38,0x94
};
static UINT8	abHwAdjParam_Play48[60] =
{
	0x0F,0x90,0xBD,0x36,0x47,0x08,0x15,0x62,0x68,0x36,
	0xFC,0x2A,0x09,0x20,0x8F,0xCB,0xCD,0x52,0xEA,0x9D,
	0x97,0xC9,0x03,0xD8,0xF7,0x4E,0xB2,0xFD,0xEB,0xA8,
	0x11,0xB9,0x89,0xB8,0x89,0x34,0x10,0x0F,0xDD,0x08,
	0xCB,0x38,0x03,0xA3,0x87,0xD1,0x51,0x14,0xEF,0x0F,
	0x12,0xEC,0x91,0xA2,0xFB,0x83,0xFE,0x80,0xC8,0xE2
};

/****************************************************************************
 *	init
 *
 *	Description:
 *			Initialize.
 *	Arguments:
 *			psInitInfo	initialize information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	init
(
	const MCDRV_INIT_INFO	*psInitInfo
)
{
	SINT32	sdRet;
	UINT8	bReg;

	if(NULL == psInitInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA != McResCtrl_GetState_AA())
	{
		return MCDRV_ERROR_STATE;
	}

	McSrv_SystemInit();
	McSrv_Lock();

	bReg	= McSrv_ReadI2C(McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG), (UINT32)MCI_AA_HW_ID);
	sdRet	= McResCtrl_SetHwId_AA(bReg);
	if(sdRet == MCDRV_SUCCESS)
	{
		sdRet	= ValidateInitParam(psInitInfo);
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		McResCtrl_Init_AA(psInitInfo);
		sdRet	= McDevIf_AllocPacketBuf_AA();
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		sdRet	= McPacket_AddInit_AA(psInitInfo);
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		sdRet	= McDevIf_ExecutePacket_AA();
	}

	if(sdRet == MCDRV_SUCCESS)
	{
		McResCtrl_UpdateState_AA(eMCDRV_STATE_READY_AA);
	}
	else
	{
		McDevIf_ReleasePacketBuf_AA();
	}

	McSrv_Unlock();

	if(sdRet != MCDRV_SUCCESS)
	{
		McSrv_SystemTerm();
	}

	return sdRet;
}

/****************************************************************************
 *	term
 *
 *	Description:
 *			Terminate.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	term
(
	void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bCh, bSrcIdx;
	UINT8	abOnOff[SOURCE_BLOCK_NUM];
	MCDRV_PATH_INFO		sPathInfo;
	MCDRV_POWER_INFO_AA	sPowerInfo;
	MCDRV_POWER_UPDATE_AA	sPowerUpdate;

	if(eMCDRV_STATE_READY_AA != McResCtrl_GetState_AA())
	{
		return MCDRV_ERROR_STATE;
	}

	McSrv_Lock();

	abOnOff[0]	= (MCDRV_SRC0_MIC1_OFF|MCDRV_SRC0_MIC2_OFF|MCDRV_SRC0_MIC3_OFF);
	abOnOff[1]	= (MCDRV_SRC1_LINE1_L_OFF|MCDRV_SRC1_LINE1_R_OFF|MCDRV_SRC1_LINE1_M_OFF);
	abOnOff[2]	= (MCDRV_SRC2_LINE2_L_OFF|MCDRV_SRC2_LINE2_R_OFF|MCDRV_SRC2_LINE2_M_OFF);
	abOnOff[3]	= (MCDRV_SRC3_DIR0_OFF|MCDRV_SRC3_DIR1_OFF|MCDRV_SRC3_DIR2_OFF|MCDRV_SRC3_DIR2_DIRECT_OFF);
	abOnOff[4]	= (MCDRV_SRC4_DTMF_OFF|MCDRV_SRC4_PDM_OFF|MCDRV_SRC4_ADC0_OFF|MCDRV_SRC4_ADC1_OFF);
	abOnOff[5]	= (MCDRV_SRC5_DAC_L_OFF|MCDRV_SRC5_DAC_R_OFF|MCDRV_SRC5_DAC_M_OFF);
	abOnOff[6]	= (MCDRV_SRC6_MIX_OFF|MCDRV_SRC6_AE_OFF|MCDRV_SRC6_CDSP_OFF|MCDRV_SRC6_CDSP_DIRECT_OFF);

	for(bSrcIdx = 0; bSrcIdx < SOURCE_BLOCK_NUM; bSrcIdx++)
	{
		for(bCh = 0; bCh < HP_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asHpOut[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < SP_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asSpOut[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < RC_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asRcOut[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < LOUT1_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asLout1[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < LOUT2_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asLout2[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < PEAK_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asPeak[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < DIT0_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asDit0[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < DIT1_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asDit1[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < DIT2_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asDit2[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < DAC_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asDac[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < AE_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asAe[bCh].abSrcOnOff[bSrcIdx]		= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < CDSP_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asCdsp[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < ADC0_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asAdc0[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < ADC1_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asAdc1[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < MIX_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asMix[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
		for(bCh = 0; bCh < BIAS_PATH_CHANNELS; bCh++)
		{
			sPathInfo.asBias[bCh].abSrcOnOff[bSrcIdx]	= abOnOff[bSrcIdx];
		}
	}
	sdRet	= set_path(&sPathInfo);
	if(sdRet == MCDRV_SUCCESS)
	{
		sPowerInfo.dDigital			= (MCDRV_POWINFO_DIGITAL_DP0_AA
									  |MCDRV_POWINFO_DIGITAL_DP1_AA
									  |MCDRV_POWINFO_DIGITAL_DP2_AA
									  |MCDRV_POWINFO_DIGITAL_DPB_AA
									  |MCDRV_POWINFO_DIGITAL_DPDI0_AA
									  |MCDRV_POWINFO_DIGITAL_DPDI1_AA
									  |MCDRV_POWINFO_DIGITAL_DPDI2_AA
									  |MCDRV_POWINFO_DIGITAL_DPPDM_AA
									  |MCDRV_POWINFO_DIGITAL_DPBDSP_AA
									  |MCDRV_POWINFO_DIGITAL_DPADIF_AA
									  |MCDRV_POWINFO_DIGITAL_PLLRST0_AA);
		sPowerInfo.abAnalog[0]		=
		sPowerInfo.abAnalog[1]		=
		sPowerInfo.abAnalog[2]		=
		sPowerInfo.abAnalog[3]		=
		sPowerInfo.abAnalog[4]		= (UINT8)0xFF;
		sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
		sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL_AA;
		sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_ANALOG1_ALL_AA;
		sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_ANALOG2_ALL_AA;
		sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_ANALOG3_ALL_AA;
		sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_ANALOG4_ALL_AA;
		sdRet	= McPacket_AddPowerDown_AA(&sPowerInfo, &sPowerUpdate);
	}
	if(sdRet == MCDRV_SUCCESS)
	{
		sdRet	= McDevIf_ExecutePacket_AA();
	}

	McDevIf_ReleasePacketBuf_AA();

	McResCtrl_UpdateState_AA(eMCDRV_STATE_NOTINIT_AA);

	McSrv_Unlock();

	McSrv_SystemTerm();

	return sdRet;
}

/****************************************************************************
 *	read_reg
 *
 *	Description:
 *			read register.
 *	Arguments:
 *			psRegInfo	register information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	read_reg
(
	MCDRV_REG_INFO*	psRegInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bSlaveAddr;
	UINT8	bAddr;
	UINT8	abData[2];
	MCDRV_POWER_INFO_AA	sPowerInfo;
	MCDRV_POWER_INFO_AA	sCurPowerInfo;
	MCDRV_POWER_UPDATE_AA	sPowerUpdate;

	if(NULL == psRegInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == McResCtrl_GetState_AA())
	{
		return MCDRV_ERROR_STATE;
	}

	sdRet	= ValidateReadRegParam(psRegInfo);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}

	/*	get current power info	*/
	McResCtrl_GetCurPowerInfo_AA(&sCurPowerInfo);

	/*	power up	*/
	McResCtrl_GetPowerInfoRegAccess_AA(psRegInfo, &sPowerInfo);
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL_AA;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_ANALOG1_ALL_AA;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_ANALOG2_ALL_AA;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_ANALOG3_ALL_AA;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_ANALOG4_ALL_AA;
	sdRet	= McPacket_AddPowerUp_AA(&sPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	bAddr	= psRegInfo->bAddress;

	if(psRegInfo->bRegType == MCDRV_REGTYPE_A)
	{
		bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
	}
	else
	{
		switch(psRegInfo->bRegType)
		{
		case	MCDRV_REGTYPE_B_BASE:
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_AA_BASE_ADR<<1;
			abData[1]	= bAddr;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bAddr	= MCI_AA_BASE_WINDOW;
			break;

		case	MCDRV_REGTYPE_B_MIXER:
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_AA_MIX_ADR<<1;
			abData[1]	= bAddr;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bAddr	= MCI_AA_MIX_WINDOW;
			break;

		case	MCDRV_REGTYPE_B_AE:
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_AA_AE_ADR<<1;
			abData[1]	= bAddr;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bAddr	= MCI_AA_AE_WINDOW;
			break;

		case	MCDRV_REGTYPE_B_CDSP:
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_AA_CDSP_ADR<<1;
			abData[1]	= bAddr;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bAddr	= MCI_AA_CDSP_WINDOW;
			break;

		case	MCDRV_REGTYPE_B_CODEC:
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_AA_CD_ADR<<1;
			abData[1]	= bAddr;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bAddr	= MCI_AA_CD_WINDOW;
			break;

		case	MCDRV_REGTYPE_B_ANALOG:
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_ANA);
			abData[0]	= MCI_AA_ANA_ADR<<1;
			abData[1]	= bAddr;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bAddr	= MCI_AA_ANA_WINDOW;
			break;

		default:
			return MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	read register	*/
	psRegInfo->bData	= McSrv_ReadI2C(bSlaveAddr, bAddr);

	/*	restore power	*/
	sdRet	= McPacket_AddPowerDown_AA(&sCurPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket_AA();
}

/****************************************************************************
 *	write_reg
 *
 *	Description:
 *			Write register.
 *	Arguments:
 *			psWR	register information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
static	SINT32	write_reg
(
	const MCDRV_REG_INFO*	psRegInfo
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	MCDRV_POWER_INFO_AA	sPowerInfo;
	MCDRV_POWER_INFO_AA	sCurPowerInfo;
	MCDRV_POWER_UPDATE_AA	sPowerUpdate;

	if(NULL == psRegInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == McResCtrl_GetState_AA())
	{
		return MCDRV_ERROR_STATE;
	}

	sdRet	= ValidateWriteRegParam(psRegInfo);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}

	/*	get current power info	*/
	McResCtrl_GetCurPowerInfo_AA(&sCurPowerInfo);

	/*	power up	*/
	McResCtrl_GetPowerInfoRegAccess_AA(psRegInfo, &sPowerInfo);
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL_AA;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_ANALOG1_ALL_AA;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_ANALOG2_ALL_AA;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_ANALOG3_ALL_AA;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_ANALOG4_ALL_AA;
	sdRet	= McPacket_AddPowerUp_AA(&sPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	switch(psRegInfo->bRegType)
	{
	case	MCDRV_REGTYPE_A:
		McDevIf_AddPacket_AA((MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | psRegInfo->bAddress), psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B_BASE:
		McDevIf_AddPacket_AA((MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_BASE_AA | psRegInfo->bAddress), psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B_MIXER:
		McDevIf_AddPacket_AA((MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | psRegInfo->bAddress), psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B_AE:
		McDevIf_AddPacket_AA((MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_AE_AA | psRegInfo->bAddress), psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B_CDSP:
		McDevIf_AddPacket_AA((MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CDSP_AA | psRegInfo->bAddress), psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B_CODEC:
		McDevIf_AddPacket_AA((MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_CODEC_AA | psRegInfo->bAddress), psRegInfo->bData);
		break;

	case	MCDRV_REGTYPE_B_ANALOG:
		McDevIf_AddPacket_AA((MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_ANA_AA | psRegInfo->bAddress), psRegInfo->bData);
		break;

	default:
		return MCDRV_ERROR_ARGUMENT;
	}

	sdRet	= McDevIf_ExecutePacket_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	restore power	*/
	if(psRegInfo->bRegType == MCDRV_REGTYPE_B_BASE)
	{
		if(psRegInfo->bAddress == MCI_AA_PWM_DIGITAL)
		{
			if((psRegInfo->bData & MCB_AA_PWM_DP0) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DP0_AA;
			}
			if((psRegInfo->bData & MCB_AA_PWM_DP1) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DP1_AA;
			}
			if((psRegInfo->bData & MCB_AA_PWM_DP2) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DP2_AA;
			}
		}
		else if(psRegInfo->bAddress == MCI_AA_PWM_DIGITAL_1)
		{
			if((psRegInfo->bData & MCB_AA_PWM_DPB) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPB_AA;
			}
			if((psRegInfo->bData & MCB_AA_PWM_DPDI0) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPDI0_AA;
			}
			if((psRegInfo->bData & MCB_AA_PWM_DPDI1) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPDI1_AA;
			}
			if((psRegInfo->bData & MCB_AA_PWM_DPDI2) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPDI2_AA;
			}
			if((psRegInfo->bData & MCB_AA_PWM_DPPDM) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPPDM_AA;
			}
		}
		else if(psRegInfo->bAddress == MCI_AA_PWM_DIGITAL_BDSP)
		{
			if((psRegInfo->bData & MCB_AA_PWM_DPBDSP) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPBDSP_AA;
			}
		}
		else if(psRegInfo->bAddress == MCI_AA_PLL_RST)
		{
			if((psRegInfo->bData & MCB_AA_PLLRST0) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_PLLRST0_AA;
			}
		}
	}
	else if(psRegInfo->bRegType == MCDRV_REGTYPE_B_CODEC)
	{
		if(psRegInfo->bAddress == MCI_AA_DPADIF)
		{
			if((psRegInfo->bData & MCB_AA_DPADIF) == 0)
			{
				sCurPowerInfo.dDigital &= ~MCDRV_POWINFO_DIGITAL_DPADIF_AA;
			}
		}
	}
	else if(psRegInfo->bRegType == MCDRV_REGTYPE_B_ANALOG)
	{
		if(psRegInfo->bAddress == MCI_AA_PWM_ANALOG_0)
		{
			sCurPowerInfo.abAnalog[0]	= psRegInfo->bData;
		}
		else if(psRegInfo->bAddress == MCI_AA_PWM_ANALOG_1)
		{
			sCurPowerInfo.abAnalog[1]	= psRegInfo->bData;
		}
		else if(psRegInfo->bAddress == MCI_AA_PWM_ANALOG_2)
		{
			sCurPowerInfo.abAnalog[2]	= psRegInfo->bData;
		}
		else if(psRegInfo->bAddress == MCI_AA_PWM_ANALOG_3)
		{
			sCurPowerInfo.abAnalog[3]	= psRegInfo->bData;
		}
		else if(psRegInfo->bAddress == MCI_AA_PWM_ANALOG_4)
		{
			sCurPowerInfo.abAnalog[4]	= psRegInfo->bData;
		}
	}

	sdRet	= McPacket_AddPowerDown_AA(&sCurPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket_AA();
}

/****************************************************************************
 *	update_clock
 *
 *	Description:
 *			Update clock info.
 *	Arguments:
 *			psClockInfo	clock info
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	update_clock
(
	const MCDRV_CLOCK_INFO*	psClockInfo
)
{
	SINT32			sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE_AA		eState	= McResCtrl_GetState_AA();
	MCDRV_INIT_INFO	sInitInfo;

	if(NULL == psClockInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetInitInfo_AA(&sInitInfo);
	if((sInitInfo.bPowerMode & MCDRV_POWMODE_CLKON) != 0)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	sdRet	= ValidateClockParam(psClockInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	McResCtrl_SetClockInfo_AA(psClockInfo);
	return sdRet;
}

/****************************************************************************
 *	get_path
 *
 *	Description:
 *			Get current path setting.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_path
(
	MCDRV_PATH_INFO*	psPathInfo
)
{
	if(NULL == psPathInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == McResCtrl_GetState_AA())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetPathInfoVirtual_AA(psPathInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_path
 *
 *	Description:
 *			Set path.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_path
(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	SINT32			sdRet	= MCDRV_SUCCESS;
	UINT32			dXFadeParam	= 0;
	MCDRV_STATE_AA		eState	= McResCtrl_GetState_AA();
	MCDRV_POWER_INFO_AA	sPowerInfo;
	MCDRV_POWER_UPDATE_AA	sPowerUpdate;
	MCDRV_HWADJ			eHwAdj;
	UINT8				*pbHwAdjParam;
	UINT8				bReg;
	UINT8				i;

	if(NULL == psPathInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	sdRet	= ValidatePathParam(psPathInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	McResCtrl_SetPathInfo_AA(psPathInfo);

	/*	unused analog out volume mute	*/
	sdRet	= SetVol(MCDRV_VOLUPDATE_ANAOUT_ALL_AA, eMCDRV_VOLUPDATE_MUTE_AA);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	DAC/DIT* mute	*/
	sdRet	= PreUpdatePath();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	unused volume mute	*/
	sdRet	= SetVol(MCDRV_VOLUPDATE_ALL_AA&~MCDRV_VOLUPDATE_ANAOUT_ALL_AA, eMCDRV_VOLUPDATE_MUTE_AA);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	stop unused path	*/
	sdRet	= McPacket_AddStop_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	if(McDevProf_IsValid(eMCDRV_FUNC_HWADJ) == 1)
	{
		eHwAdj = McResCtrl_ConfigHwAdj_AA();

		if(eHwAdj != eMCDRV_HWADJ_NOCHANGE)
		{
			switch(eHwAdj)
			{
			case	eMCDRV_HWADJ_REC8:
				pbHwAdjParam = abHwAdjParam_Rec8;
				break;
			case	eMCDRV_HWADJ_REC44:
				pbHwAdjParam = abHwAdjParam_Rec44;
				break;
			case	eMCDRV_HWADJ_REC48:
				pbHwAdjParam = abHwAdjParam_Rec48;
				break;
			case	eMCDRV_HWADJ_PLAY8:
				pbHwAdjParam = abHwAdjParam_Play8;
				break;
			case	eMCDRV_HWADJ_PLAY44:
				pbHwAdjParam = abHwAdjParam_Play44;
				break;
			case	eMCDRV_HWADJ_PLAY48:
				pbHwAdjParam = abHwAdjParam_Play48;
				break;
			default:
				pbHwAdjParam = abHwAdjParam_Thru;
				break;
			}

			dXFadeParam	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DAC_INS);
			dXFadeParam	<<= 8;
			dXFadeParam	|= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_INS);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)MCI_AA_DAC_INS, 0);
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | (UINT32)MCI_AA_INS, 0);
			sdRet	= McDevIf_ExecutePacket_AA();
			if(sdRet != MCDRV_SUCCESS)
			{
				return sdRet;
			}

			/*	wait xfade complete	*/
			if(dXFadeParam != (UINT32)0)
			{
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_EVTWAIT_AA | MCDRV_EVT_INSFLG_AA | dXFadeParam, 0);
				bReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_A_AA, MCI_AA_BDSP_ST);
				bReg	&= (UINT8)~MCB_AA_EQ3ON;
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_A_AA | (UINT32)MCI_AA_BDSP_ST, bReg);
			}

			for(i = 0; i < 60; i++)
			{
				McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_FORCE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_AE_AA | (UINT32)(MCI_AA_BAND6H_CEQ0+i), pbHwAdjParam[i]);
			}

			sdRet	= McDevIf_ExecutePacket_AA();
			if(MCDRV_SUCCESS != sdRet)
			{
				return sdRet;
			}
		}
	}

	McResCtrl_GetPowerInfo_AA(&sPowerInfo);

	/*	used path power up	*/
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
	sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL_AA;
	sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_ANALOG1_ALL_AA;
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_ANALOG2_ALL_AA;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_ANALOG3_ALL_AA;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_ANALOG4_ALL_AA;
	sdRet	= McPacket_AddPowerUp_AA(&sPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	set digital mixer	*/
	sdRet	= McPacket_AddPathSet_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	set analog mixer	*/
	sdRet	= McPacket_AddMixSet_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	unused path power down	*/
	sPowerUpdate.dDigital		= MCDRV_POWUPDATE_DIGITAL_ALL_AA;
	if(McDevProf_IsValid(eMCDRV_FUNC_HWADJ) == 1)
	{
		sPowerUpdate.abAnalog[0]	= 0;
		sPowerUpdate.abAnalog[1]	= 0xCF;
	}
	else
	{
		sPowerUpdate.abAnalog[0]	= (UINT8)MCDRV_POWUPDATE_ANALOG0_ALL_AA;
		sPowerUpdate.abAnalog[1]	= (UINT8)MCDRV_POWUPDATE_ANALOG1_ALL_AA;
	}
	sPowerUpdate.abAnalog[2]	= (UINT8)MCDRV_POWUPDATE_ANALOG2_ALL_AA;
	sPowerUpdate.abAnalog[3]	= (UINT8)MCDRV_POWUPDATE_ANALOG3_ALL_AA;
	sPowerUpdate.abAnalog[4]	= (UINT8)MCDRV_POWUPDATE_ANALOG4_ALL_AA;
	sdRet	= McPacket_AddPowerDown_AA(&sPowerInfo, &sPowerUpdate);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	start	*/
	sdRet	= McPacket_AddStart_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	/*	set volume	*/
	sdRet	= SetVol(MCDRV_VOLUPDATE_ALL_AA&~MCDRV_VOLUPDATE_ANAOUT_ALL_AA, eMCDRV_VOLUPDATE_ALL_AA);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return SetVol(MCDRV_VOLUPDATE_ANAOUT_ALL_AA, eMCDRV_VOLUPDATE_ALL_AA);
}

/****************************************************************************
 *	get_volume
 *
 *	Description:
 *			Get current volume setting.
 *	Arguments:
 *			psVolInfo	volume information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	get_volume
(
	MCDRV_VOL_INFO*	psVolInfo
)
{
	if(NULL == psVolInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == McResCtrl_GetState_AA())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetVolInfo_AA(psVolInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_volume
 *
 *	Description:
 *			Set volume.
 *	Arguments:
 *			psVolInfo	volume update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	set_volume
(
	const MCDRV_VOL_INFO*	psVolInfo
)
{
	MCDRV_STATE_AA	eState	= McResCtrl_GetState_AA();

	if(NULL == psVolInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetVolInfo_AA(psVolInfo);

	return SetVol(MCDRV_VOLUPDATE_ALL_AA, eMCDRV_VOLUPDATE_ALL_AA);
}

/****************************************************************************
 *	get_digitalio
 *
 *	Description:
 *			Get current digital IO setting.
 *	Arguments:
 *			psDioInfo	digital IO information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_digitalio
(
	MCDRV_DIO_INFO*	psDioInfo
)
{
	if(NULL == psDioInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == McResCtrl_GetState_AA())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetDioInfo_AA(psDioInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_digitalio
 *
 *	Description:
 *			Update digital IO configuration.
 *	Arguments:
 *			psDioInfo	digital IO configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_digitalio
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE_AA	eState	= McResCtrl_GetState_AA();

	if(NULL == psDioInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	sdRet	= ValidateDioParam(psDioInfo, dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	McResCtrl_SetDioInfo_AA(psDioInfo, dUpdateInfo);

	sdRet	= McPacket_AddDigitalIO_AA(dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket_AA();
}

/****************************************************************************
 *	get_dac
 *
 *	Description:
 *			Get current DAC setting.
 *	Arguments:
 *			psDacInfo	DAC information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_dac
(
	MCDRV_DAC_INFO*	psDacInfo
)
{
	if(NULL == psDacInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == McResCtrl_GetState_AA())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetDacInfo_AA(psDacInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_dac
 *
 *	Description:
 *			Update DAC configuration.
 *	Arguments:
 *			psDacInfo	DAC configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_dac
(
	const MCDRV_DAC_INFO*	psDacInfo,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE_AA	eState	= McResCtrl_GetState_AA();

	if(NULL == psDacInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetDacInfo_AA(psDacInfo, dUpdateInfo);

	sdRet	= McPacket_AddDAC_AA(dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket_AA();
}

/****************************************************************************
 *	get_adc
 *
 *	Description:
 *			Get current ADC setting.
 *	Arguments:
 *			psAdcInfo	ADC information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_adc
(
	MCDRV_ADC_INFO*	psAdcInfo
)
{
	if(NULL == psAdcInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == McResCtrl_GetState_AA())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetAdcInfo_AA(psAdcInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_adc
 *
 *	Description:
 *			Update ADC configuration.
 *	Arguments:
 *			psAdcInfo	ADC configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_adc
(
	const MCDRV_ADC_INFO*	psAdcInfo,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE_AA	eState	= McResCtrl_GetState_AA();

	if(NULL == psAdcInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetAdcInfo_AA(psAdcInfo, dUpdateInfo);

	sdRet	= McPacket_AddADC_AA(dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket_AA();
}

/****************************************************************************
 *	get_sp
 *
 *	Description:
 *			Get current SP setting.
 *	Arguments:
 *			psSpInfo	SP information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_sp
(
	MCDRV_SP_INFO*	psSpInfo
)
{
	if(NULL == psSpInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == McResCtrl_GetState_AA())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetSpInfo_AA(psSpInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_sp
 *
 *	Description:
 *			Update SP configuration.
 *	Arguments:
 *			psSpInfo	SP configuration
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_sp
(
	const MCDRV_SP_INFO*	psSpInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE_AA	eState	= McResCtrl_GetState_AA();

	if(NULL == psSpInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetSpInfo_AA(psSpInfo);

	sdRet	= McPacket_AddSP_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket_AA();
}

/****************************************************************************
 *	get_dng
 *
 *	Description:
 *			Get current Digital Noise Gate setting.
 *	Arguments:
 *			psDngInfo	DNG information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_dng
(
	MCDRV_DNG_INFO*	psDngInfo
)
{
	if(NULL == psDngInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == McResCtrl_GetState_AA())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetDngInfo_AA(psDngInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_dng
 *
 *	Description:
 *			Update Digital Noise Gate configuration.
 *	Arguments:
 *			psDngInfo	DNG configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_dng
(
	const MCDRV_DNG_INFO*	psDngInfo,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE_AA	eState	= McResCtrl_GetState_AA();

	if(NULL == psDngInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetDngInfo_AA(psDngInfo, dUpdateInfo);

	sdRet	= McPacket_AddDNG_AA(dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket_AA();
}

/****************************************************************************
 *	set_ae
 *
 *	Description:
 *			Update Audio Engine configuration.
 *	Arguments:
 *			psAeInfo	AE configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_ae
(
	const MCDRV_AE_INFO*	psAeInfo,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE_AA	eState	= McResCtrl_GetState_AA();
	MCDRV_PATH_INFO	sPathInfo;

	if(NULL == psAeInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetAeInfo_AA(psAeInfo, dUpdateInfo);

	sdRet	= McPacket_AddAE_AA(dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	sdRet	= McDevIf_ExecutePacket_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}

	McResCtrl_GetPathInfoVirtual_AA(&sPathInfo);
	return	set_path(&sPathInfo);
}

/****************************************************************************
 *	get_pdm
 *
 *	Description:
 *			Get current PDM setting.
 *	Arguments:
 *			psPdmInfo	PDM information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	get_pdm
(
	MCDRV_PDM_INFO*	psPdmInfo
)
{
	if(NULL == psPdmInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == McResCtrl_GetState_AA())
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetPdmInfo_AA(psPdmInfo);

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	set_pdm
 *
 *	Description:
 *			Update PDM configuration.
 *	Arguments:
 *			psPdmInfo	PDM configuration
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
static	SINT32	set_pdm
(
	const MCDRV_PDM_INFO*	psPdmInfo,
	UINT32					dUpdateInfo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE_AA	eState	= McResCtrl_GetState_AA();

	if(NULL == psPdmInfo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetPdmInfo_AA(psPdmInfo, dUpdateInfo);

	sdRet	= McPacket_AddPDM_AA(dUpdateInfo);
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket_AA();
}

/****************************************************************************
 *	config_gp
 *
 *	Description:
 *			Set GPIO mode.
 *	Arguments:
 *			psGpMode	GPIO mode information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
*
 ****************************************************************************/
static	SINT32	config_gp
(
	const MCDRV_GP_MODE*	psGpMode
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE_AA	eState	= McResCtrl_GetState_AA();

	if(NULL == psGpMode)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_SetGPMode_AA(psGpMode);

	sdRet	= McPacket_AddGPMode_AA();
	if(MCDRV_SUCCESS != sdRet)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket_AA();
}

/****************************************************************************
 *	mask_gp
 *
 *	Description:
 *			Set GPIO input mask.
 *	Arguments:
 *			pbMask	mask setting
 *			dPadNo	PAD number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_STATE
 *			MCDRV_ERROR
*
 ****************************************************************************/
static	SINT32	mask_gp
(
	UINT8*	pbMask,
	UINT32	dPadNo
)
{
	SINT32		sdRet	= MCDRV_SUCCESS;
	MCDRV_STATE_AA	eState	= McResCtrl_GetState_AA();
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_GP_MODE	sGPMode;

	if(NULL == pbMask)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetInitInfo_AA(&sInitInfo);
	McResCtrl_GetGPMode_AA(&sGPMode);

	if(dPadNo == MCDRV_GP_PAD0)
	{
		if(sInitInfo.bPad0Func == MCDRV_PAD_GPIO && sGPMode.abGpDdr[dPadNo] == MCDRV_GPDDR_OUT)
		{
			return MCDRV_ERROR;
		}
	}
	else if(dPadNo == MCDRV_GP_PAD1)
	{
		if(sInitInfo.bPad1Func == MCDRV_PAD_GPIO && sGPMode.abGpDdr[dPadNo] == MCDRV_GPDDR_OUT)
		{
			return MCDRV_ERROR;
		}
	}
/*
	else if(dPadNo == MCDRV_GP_PAD2)
	{
		if(sInitInfo.bPad2Func == MCDRV_PAD_GPIO && sGPMode.abGpDdr[dPadNo] == MCDRV_GPDDR_OUT)
		{
			return MCDRV_ERROR;
		}
	}
*/
	McResCtrl_SetGPMask_AA(*pbMask, dPadNo);

	sdRet	= McPacket_AddGPMask_AA(dPadNo);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket_AA();
}

/****************************************************************************
 *	getset_gp
 *
 *	Description:
 *			Set or get state of GPIO pin.
 *	Arguments:
 *			pbGpio	pin state
 *			dPadNo	PAD number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
*
 ****************************************************************************/
static	SINT32	getset_gp
(
	UINT8*	pbGpio,
	UINT32	dPadNo
)
{
	SINT32			sdRet	= MCDRV_SUCCESS;
	UINT8			bSlaveAddr;
	UINT8			abData[2];
	UINT8			bRegData;
	MCDRV_STATE_AA		eState	= McResCtrl_GetState_AA();
	MCDRV_INIT_INFO	sInitInfo;
	MCDRV_GP_MODE	sGPMode;

	if(NULL == pbGpio)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(eMCDRV_STATE_NOTINIT_AA == eState)
	{
		return MCDRV_ERROR_STATE;
	}

	McResCtrl_GetInitInfo_AA(&sInitInfo);
	McResCtrl_GetGPMode_AA(&sGPMode);

	if((dPadNo == MCDRV_GP_PAD0 && sInitInfo.bPad0Func != MCDRV_PAD_GPIO)
	|| (dPadNo == MCDRV_GP_PAD1 && sInitInfo.bPad1Func != MCDRV_PAD_GPIO)
	/*|| (dPadNo == MCDRV_GP_PAD2 && sInitInfo.bPad2Func != MCDRV_PAD_GPIO)*/)
	{
		return MCDRV_ERROR;
	}

	if(dPadNo < GPIO_PAD_NUM)
	{
		if(sGPMode.abGpDdr[dPadNo] == MCDRV_GPDDR_IN)
		{
			bSlaveAddr	= McDevProf_GetSlaveAddr(eMCDRV_SLAVE_ADDR_DIG);
			abData[0]	= MCI_AA_BASE_ADR<<1;
			abData[1]	= MCI_AA_PA_SCU_PA;
			McSrv_WriteI2C(bSlaveAddr, abData, 2);
			bRegData	= McSrv_ReadI2C(bSlaveAddr, MCI_AA_BASE_WINDOW);
			if(dPadNo == MCDRV_GP_PAD0)
			{
				*pbGpio	= bRegData & MCB_AA_PA_SCU_PA0;
			}
			else if(dPadNo == MCDRV_GP_PAD1)
			{
				*pbGpio	= (bRegData & MCB_AA_PA_SCU_PA1) >> 1;
			}
	/*
			else if(dPadNo == MCDRV_GP_PAD2)
			{
				*pbGpio	= (bRegData & MCB_AA_PA_SCU_PA2) >> 2;
			}
	*/
		}
		else
		{
			sdRet	= McPacket_AddGPSet_AA(*pbGpio, dPadNo);
			if(sdRet != MCDRV_SUCCESS)
			{
				return sdRet;
			}
			return McDevIf_ExecutePacket_AA();
		}
	}

	return MCDRV_SUCCESS;
}


/****************************************************************************
 *	ValidateInitParam
 *
 *	Description:
 *			validate init parameters.
 *	Arguments:
 *			psInitInfo	initialize information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	ValidateInitParam
(
	const MCDRV_INIT_INFO*	psInitInfo
)
{

	if(MCDRV_CKSEL_CMOS != psInitInfo->bCkSel && 1/*MCDRV_CKSEL_TCXO*/ != psInitInfo->bCkSel)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(psInitInfo->bDivR0 == 0x00 || psInitInfo->bDivR0 > 0x7F)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_DIVR1) == 1
	&& (psInitInfo->bDivR1 == 0x00 || psInitInfo->bDivR1 > 0x7F))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(psInitInfo->bDivF0 == 0x00)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_DIVF1) == 1 && psInitInfo->bDivF1 == 0x00)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_RANGE) == 1
	&& (psInitInfo->bRange0 > 0x07 || psInitInfo->bRange1 > 0x07))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_BYPASS) == 1 && psInitInfo->bBypass > 0x03)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_DAHIZ_LOW != psInitInfo->bDioSdo0Hiz && MCDRV_DAHIZ_HIZ != psInitInfo->bDioSdo0Hiz)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_DAHIZ_LOW != psInitInfo->bDioSdo1Hiz && MCDRV_DAHIZ_HIZ != psInitInfo->bDioSdo1Hiz)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_DAHIZ_LOW != psInitInfo->bDioSdo2Hiz && MCDRV_DAHIZ_HIZ != psInitInfo->bDioSdo2Hiz)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_PCMHIZ_HIZ != psInitInfo->bPcmHiz && MCDRV_PCMHIZ_LOW != psInitInfo->bPcmHiz)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_DAHIZ_LOW != psInitInfo->bDioClk0Hiz && MCDRV_DAHIZ_HIZ != psInitInfo->bDioClk0Hiz)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_DAHIZ_LOW != psInitInfo->bDioClk1Hiz && MCDRV_DAHIZ_HIZ != psInitInfo->bDioClk1Hiz)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_DAHIZ_LOW != psInitInfo->bDioClk2Hiz && MCDRV_DAHIZ_HIZ != psInitInfo->bDioClk2Hiz)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_PCMHIZ_HIZ != psInitInfo->bPcmHiz && MCDRV_PCMHIZ_LOW != psInitInfo->bPcmHiz)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_LINE_STEREO != psInitInfo->bLineIn1Dif && MCDRV_LINE_DIF != psInitInfo->bLineIn1Dif)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_LI2) == 1
	&& MCDRV_LINE_STEREO != psInitInfo->bLineIn2Dif && MCDRV_LINE_DIF != psInitInfo->bLineIn2Dif)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_LINE_STEREO != psInitInfo->bLineOut1Dif && MCDRV_LINE_DIF != psInitInfo->bLineOut1Dif)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_LINE_STEREO != psInitInfo->bLineOut2Dif && MCDRV_LINE_DIF != psInitInfo->bLineOut2Dif)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_SPMN_ON != psInitInfo->bSpmn && MCDRV_SPMN_OFF != psInitInfo->bSpmn)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_MIC_DIF != psInitInfo->bMic1Sng && MCDRV_MIC_SINGLE != psInitInfo->bMic1Sng)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_MIC_DIF != psInitInfo->bMic2Sng && MCDRV_MIC_SINGLE != psInitInfo->bMic2Sng)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_MIC_DIF != psInitInfo->bMic3Sng && MCDRV_MIC_SINGLE != psInitInfo->bMic3Sng)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_POWMODE_NORMAL != psInitInfo->bPowerMode
	&& MCDRV_POWMODE_CLKON != psInitInfo->bPowerMode
	&& MCDRV_POWMODE_VREFON != psInitInfo->bPowerMode
	&& MCDRV_POWMODE_CLKVREFON != psInitInfo->bPowerMode
	&& MCDRV_POWMODE_FULL != psInitInfo->bPowerMode)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_SPHIZ_PULLDOWN != psInitInfo->bSpHiz && MCDRV_SPHIZ_HIZ != psInitInfo->bSpHiz)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_LDO_OFF != psInitInfo->bLdo && MCDRV_LDO_ON != psInitInfo->bLdo)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(MCDRV_PAD_GPIO != psInitInfo->bPad0Func
	&& MCDRV_PAD_PDMCK != psInitInfo->bPad0Func && MCDRV_PAD_IRQ != psInitInfo->bPad0Func)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_IRQ) != 1 && MCDRV_PAD_IRQ == psInitInfo->bPad0Func)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(MCDRV_PAD_GPIO != psInitInfo->bPad1Func
	&& MCDRV_PAD_PDMDI != psInitInfo->bPad1Func && MCDRV_PAD_IRQ != psInitInfo->bPad1Func)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(McDevProf_IsValid(eMCDRV_FUNC_IRQ) != 1 && MCDRV_PAD_IRQ == psInitInfo->bPad1Func)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(McDevProf_IsValid(eMCDRV_FUNC_PAD2) == 1
	&& MCDRV_PAD_GPIO != psInitInfo->bPad2Func
	&& MCDRV_PAD_PDMDI != psInitInfo->bPad2Func
	&& MCDRV_PAD_IRQ != psInitInfo->bPad2Func)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(MCDRV_OUTLEV_0 != psInitInfo->bAvddLev && MCDRV_OUTLEV_1 != psInitInfo->bAvddLev
	&& MCDRV_OUTLEV_2 != psInitInfo->bAvddLev && MCDRV_OUTLEV_3 != psInitInfo->bAvddLev
	&& MCDRV_OUTLEV_4 != psInitInfo->bAvddLev && MCDRV_OUTLEV_5 != psInitInfo->bAvddLev
	&& MCDRV_OUTLEV_6 != psInitInfo->bAvddLev && MCDRV_OUTLEV_7 != psInitInfo->bAvddLev)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(MCDRV_OUTLEV_0 != psInitInfo->bVrefLev && MCDRV_OUTLEV_1 != psInitInfo->bVrefLev
	&& MCDRV_OUTLEV_2 != psInitInfo->bVrefLev && MCDRV_OUTLEV_3 != psInitInfo->bVrefLev
	&& MCDRV_OUTLEV_4 != psInitInfo->bVrefLev && MCDRV_OUTLEV_5 != psInitInfo->bVrefLev
	&& MCDRV_OUTLEV_6 != psInitInfo->bVrefLev && MCDRV_OUTLEV_7 != psInitInfo->bVrefLev)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(MCDRV_DCLGAIN_0 != psInitInfo->bDclGain && MCDRV_DCLGAIN_6 != psInitInfo->bDclGain
	&& MCDRV_DCLGAIN_12!= psInitInfo->bDclGain && MCDRV_DCLGAIN_18!= psInitInfo->bDclGain)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if(MCDRV_DCLLIMIT_0 != psInitInfo->bDclLimit && MCDRV_DCLLIMIT_116 != psInitInfo->bDclLimit
	&& MCDRV_DCLLIMIT_250 != psInitInfo->bDclLimit && MCDRV_DCLLIMIT_602 != psInitInfo->bDclLimit)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if((MCDRV_MAX_WAIT_TIME_AA < psInitInfo->sWaitTime.dAdHpf)
	|| (MCDRV_MAX_WAIT_TIME_AA < psInitInfo->sWaitTime.dMic1Cin)
	|| (MCDRV_MAX_WAIT_TIME_AA < psInitInfo->sWaitTime.dMic2Cin)
	|| (MCDRV_MAX_WAIT_TIME_AA < psInitInfo->sWaitTime.dMic3Cin)
	|| (MCDRV_MAX_WAIT_TIME_AA < psInitInfo->sWaitTime.dLine1Cin)
	|| (MCDRV_MAX_WAIT_TIME_AA < psInitInfo->sWaitTime.dLine2Cin)
	|| (MCDRV_MAX_WAIT_TIME_AA < psInitInfo->sWaitTime.dVrefRdy1)
	|| (MCDRV_MAX_WAIT_TIME_AA < psInitInfo->sWaitTime.dVrefRdy2)
	|| (MCDRV_MAX_WAIT_TIME_AA < psInitInfo->sWaitTime.dHpRdy)
	|| (MCDRV_MAX_WAIT_TIME_AA < psInitInfo->sWaitTime.dSpRdy)
	|| (MCDRV_MAX_WAIT_TIME_AA < psInitInfo->sWaitTime.dPdm))
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	ValidateClockParam
 *
 *	Description:
 *			validate clock parameters.
 *	Arguments:
 *			psClockInfo	clock information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	ValidateClockParam
(
	const MCDRV_CLOCK_INFO*	psClockInfo
)
{

	if((MCDRV_CKSEL_CMOS != psClockInfo->bCkSel) && (1/*MCDRV_CKSEL_TCXO*/ != psClockInfo->bCkSel))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if((psClockInfo->bDivR0 == 0x00) || (psClockInfo->bDivR0 > 0x7F))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if((McDevProf_IsValid(eMCDRV_FUNC_DIVR1) == 1)
	&& ((psClockInfo->bDivR1 == 0x00) || (psClockInfo->bDivR1 > 0x7F)))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(psClockInfo->bDivF0 == 0x00)
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if((McDevProf_IsValid(eMCDRV_FUNC_DIVF1) == 1) && (psClockInfo->bDivF1 == 0x00))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if((McDevProf_IsValid(eMCDRV_FUNC_RANGE) == 1)
	&& ((psClockInfo->bRange0 > 0x07) || (psClockInfo->bRange1 > 0x07)))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if((McDevProf_IsValid(eMCDRV_FUNC_BYPASS) == 1) && (psClockInfo->bBypass > 0x03))
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	ValidateReadRegParam
 *
 *	Description:
 *			validate read reg parameter.
 *	Arguments:
 *			psRegInfo	register information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	ValidateReadRegParam
(
	const MCDRV_REG_INFO*	psRegInfo
)
{

	if((McResCtrl_GetRegAccess_AA(psRegInfo) & eMCDRV_READ_ONLY_AA) == 0)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	ValidateWriteRegParam
 *
 *	Description:
 *			validate write reg parameter.
 *	Arguments:
 *			psRegInfo	register information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static	SINT32	ValidateWriteRegParam
(
	const MCDRV_REG_INFO*	psRegInfo
)
{

	if((McResCtrl_GetRegAccess_AA(psRegInfo) & eMCDRV_WRITE_ONLY_AA) == 0)
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	ValidatePathParam
 *
 *	Description:
 *			validate path parameters.
 *	Arguments:
 *			psPathInfo	path information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	ValidatePathParam(
	const MCDRV_PATH_INFO*	psPathInfo
)
{
	UINT8	bBlock;
	UINT8	bCh;
	UINT8	bPDMIsSrc	= 0;
	UINT8	bADC0IsSrc	= 0;
	MCDRV_PATH_INFO		sCurPathInfo;


	McResCtrl_GetPathInfoVirtual_AA(&sCurPathInfo);
	/*	set off to current path info	*/
	for(bBlock = 0; bBlock < SOURCE_BLOCK_NUM; bBlock++)
	{
		for(bCh = 0; bCh < HP_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asHpOut[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asHpOut[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asHpOut[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asHpOut[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asHpOut[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < SP_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asSpOut[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x02;
			}
			if((psPathInfo->asSpOut[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x08;
			}
			if((psPathInfo->asSpOut[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x20;
			}
			if((psPathInfo->asSpOut[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asSpOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x80;
			}
		}
		for(bCh = 0; bCh < RC_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asRcOut[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x02;
			}
			if((psPathInfo->asRcOut[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x08;
			}
			if((psPathInfo->asRcOut[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x20;
			}
			if((psPathInfo->asRcOut[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asRcOut[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x80;
			}
		}
		for(bCh = 0; bCh < LOUT1_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asLout1[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x02;
			}
			if((psPathInfo->asLout1[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x08;
			}
			if((psPathInfo->asLout1[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x20;
			}
			if((psPathInfo->asLout1[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asLout1[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x80;
			}
		}
		for(bCh = 0; bCh < LOUT2_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asLout2[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asLout2[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asLout2[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asLout2[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asLout2[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < DIT0_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asDit0[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asDit0[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asDit0[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asDit0[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asDit0[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < DIT1_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asDit1[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asDit1[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asDit1[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asDit1[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asDit1[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < DIT2_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asDit2[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asDit2[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asDit2[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asDit2[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asDit2[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < DAC_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asDac[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asDac[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asDac[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asDac[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asDac[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < AE_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asAe[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asAe[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asAe[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asAe[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asAe[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
		for(bCh = 0; bCh < ADC0_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asAdc0[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x02;
			}
			if((psPathInfo->asAdc0[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x08;
			}
			if((psPathInfo->asAdc0[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x20;
			}
			if((psPathInfo->asAdc0[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asAdc0[bCh].abSrcOnOff[bBlock]	|= (UINT8)0x80;
			}
		}
		for(bCh = 0; bCh < MIX_PATH_CHANNELS; bCh++)
		{
			if((psPathInfo->asMix[bCh].abSrcOnOff[bBlock] & (0x03)) == 0x02)
			{
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x01;
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	|= 0x02;
			}
			if((psPathInfo->asMix[bCh].abSrcOnOff[bBlock] & (0x0C)) == 0x08)
			{
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x04;
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	|= 0x08;
			}
			if((psPathInfo->asMix[bCh].abSrcOnOff[bBlock] & (0x30)) == 0x20)
			{
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x10;
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	|= 0x20;
			}
			if((psPathInfo->asMix[bCh].abSrcOnOff[bBlock] & (0xC0)) == 0x80)
			{
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	&= (UINT8)~0x40;
				sCurPathInfo.asMix[bCh].abSrcOnOff[bBlock]	|= 0x80;
			}
		}
	}

	if((sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
	|| (sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
	|| (sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
	|| (sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
	|| (sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
	|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON
	|| (sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & MCDRV_SRC4_PDM_ON) == MCDRV_SRC4_PDM_ON)
	{
		bPDMIsSrc	= 1;
	}
	if((sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
	|| (sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
	|| (sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
	|| (sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
	|| (sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
	|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON
	|| (sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & MCDRV_SRC4_ADC0_ON) == MCDRV_SRC4_ADC0_ON)
	{
		bADC0IsSrc	= 1;
	}

	/*	HP	*/
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		if(((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		if(((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
		|| ((sCurPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON))
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		if((sCurPathInfo.asHpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	SP	*/
	if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		if(((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		if(((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
		|| ((sCurPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON))
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		if((sCurPathInfo.asSpOut[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
	{
		if(((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_ON)
	{
		if(((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
		|| ((sCurPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON))
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		if((sCurPathInfo.asSpOut[1].abSrcOnOff[MCDRV_SRC_DAC_R_BLOCK] & (MCDRV_SRC5_DAC_R_ON|MCDRV_SRC5_DAC_R_OFF)) == MCDRV_SRC5_DAC_R_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	RCV	*/

	/*	LOUT1	*/
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		if(((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		if(((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
		|| ((sCurPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON))
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		if((sCurPathInfo.asLout1[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	LOUT2	*/
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		if(((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
	{
		if(((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
		|| ((sCurPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON))
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_M_BLOCK] & (MCDRV_SRC5_DAC_M_ON|MCDRV_SRC5_DAC_M_OFF)) == MCDRV_SRC5_DAC_M_ON)
	{
		if((sCurPathInfo.asLout2[0].abSrcOnOff[MCDRV_SRC_DAC_L_BLOCK] & (MCDRV_SRC5_DAC_L_ON|MCDRV_SRC5_DAC_L_OFF)) == MCDRV_SRC5_DAC_L_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	PeakMeter	*/

	/*	DIT0	*/
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
	{
		if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	{
		if((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}

	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(bADC0IsSrc == 1)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bPDMIsSrc	= 1;
		}
	}
	if((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(bPDMIsSrc == 1)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bADC0IsSrc	= 1;
		}
	}

	/*	DIT1	*/
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
	{
		if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	{
		if((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}

	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(bADC0IsSrc == 1)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bPDMIsSrc	= 1;
		}
	}
	if((psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(bPDMIsSrc == 1)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bADC0IsSrc	= 1;
		}
	}

	/*	DIT2	*/
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
	{
		if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
	{
		if((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}

	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(bADC0IsSrc == 1)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bPDMIsSrc	= 1;
		}
	}
	if((psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(bPDMIsSrc == 1)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bADC0IsSrc	= 1;
		}
	}

	/*	DAC	*/
	for(bCh = 0; bCh < DAC_PATH_CHANNELS; bCh++)
	{
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
		{
			if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
			|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
			{
				return MCDRV_ERROR_ARGUMENT;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
		{
			if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
			|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
			{
				return MCDRV_ERROR_ARGUMENT;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
		{
			if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
			|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
			{
				return MCDRV_ERROR_ARGUMENT;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		{
			if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
			|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
			{
				return MCDRV_ERROR_ARGUMENT;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		{
			if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
			|| (psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
			|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
			{
				return MCDRV_ERROR_ARGUMENT;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
		{
			if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
			{
				return MCDRV_ERROR_ARGUMENT;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			if((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
			{
				return MCDRV_ERROR_ARGUMENT;
			}
		}

		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
		{
			if(bADC0IsSrc == 1)
			{
				return MCDRV_ERROR_ARGUMENT;
			}
			else
			{
				bPDMIsSrc	= 1;
			}
		}
		if((psPathInfo->asDac[bCh].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
		{
			if(bPDMIsSrc == 1)
			{
				return MCDRV_ERROR_ARGUMENT;
			}
			else
			{
				bADC0IsSrc	= 1;
			}
		}
	}

	/*	AE	*/
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	{
		if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
		|| (psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
		|| (psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		|| (psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	{
		if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
		|| (psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		|| (psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	{
		if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
		|| (psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
		|| (psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}

	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(bADC0IsSrc == 1)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bPDMIsSrc	= 1;
		}
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(bPDMIsSrc == 1)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bADC0IsSrc	= 1;
		}
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
	&& ((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON
		|| (sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & MCDRV_SRC6_AE_ON) == MCDRV_SRC6_AE_ON))
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	/*	CDSP	*/
	for(bCh = 0; bCh < 4; bCh++)
	{
	}

	/*	ADC0	*/
	if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
	{
		if(((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asAdc0[0].abSrcOnOff[MCDRV_SRC_LINE1_L_BLOCK] & (MCDRV_SRC1_LINE1_L_ON|MCDRV_SRC1_LINE1_L_OFF)) == MCDRV_SRC1_LINE1_L_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
	{
		if(((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
		|| ((sCurPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON))
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if((psPathInfo->asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_M_BLOCK] & (MCDRV_SRC1_LINE1_M_ON|MCDRV_SRC1_LINE1_M_OFF)) == MCDRV_SRC1_LINE1_M_ON)
	{
		if((sCurPathInfo.asAdc0[1].abSrcOnOff[MCDRV_SRC_LINE1_R_BLOCK] & (MCDRV_SRC1_LINE1_R_ON|MCDRV_SRC1_LINE1_R_OFF)) == MCDRV_SRC1_LINE1_R_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}

	/*	ADC1	*/

	/*	MIX	*/
	if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	{
		if(bADC0IsSrc == 1)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bPDMIsSrc	= 1;
		}
	}
	if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	{
		if(bPDMIsSrc == 1)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
		else
		{
			bADC0IsSrc	= 1;
		}
	}
	if((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_AE_BLOCK] & (MCDRV_SRC6_AE_ON|MCDRV_SRC6_AE_OFF)) == MCDRV_SRC6_AE_ON)
	{
		if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
		|| (sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & MCDRV_SRC6_MIX_ON) == MCDRV_SRC6_MIX_ON)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	if(((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON)
	&& ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON)
	&& ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON)
	&& ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON)
	&& ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if(((psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON)
	&& ((sCurPathInfo.asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON))
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	&& ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	 || (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	 || (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	 || (psPathInfo->asDac[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	 || (psPathInfo->asDac[1].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	 || (psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	 || (sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	 || (sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	 || (sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	 || (sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	 || (sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	 || (sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR0_BLOCK] & (MCDRV_SRC3_DIR0_ON|MCDRV_SRC3_DIR0_OFF)) == MCDRV_SRC3_DIR0_ON
	 ))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	&& ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	 || (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	 || (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	 || (psPathInfo->asDac[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	 || (psPathInfo->asDac[1].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	 || (psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	 || (sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	 || (sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	 || (sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	 || (sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	 || (sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	 || (sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR1_BLOCK] & (MCDRV_SRC3_DIR1_ON|MCDRV_SRC3_DIR1_OFF)) == MCDRV_SRC3_DIR1_ON
	 ))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	&& ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	 || (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	 || (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	 || (psPathInfo->asDac[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	 || (psPathInfo->asDac[1].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	 || (psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	 || (sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	 || (sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	 || (sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	 || (sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	 || (sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	 || (sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_DIR2_BLOCK] & (MCDRV_SRC3_DIR2_ON|MCDRV_SRC3_DIR2_OFF)) == MCDRV_SRC3_DIR2_ON
	 ))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	&& ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	 || (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	 || (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	 || (psPathInfo->asDac[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	 || (psPathInfo->asDac[1].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	 || (psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	 || (sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	 || (sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	 || (sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	 || (sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	 || (sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON
	 || (sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_PDM_BLOCK] & (MCDRV_SRC4_PDM_ON|MCDRV_SRC4_PDM_OFF)) == MCDRV_SRC4_PDM_ON))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
	&& ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
	 || (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
	 || (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
	 || (psPathInfo->asDac[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
	 || (psPathInfo->asDac[1].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
	 || (psPathInfo->asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
	 || (sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
	 || (sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
	 || (sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
	 || (sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
	 || (sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON
	 || (sCurPathInfo.asMix[0].abSrcOnOff[MCDRV_SRC_ADC0_BLOCK] & (MCDRV_SRC4_ADC0_ON|MCDRV_SRC4_ADC0_OFF)) == MCDRV_SRC4_ADC0_ON))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if((psPathInfo->asAe[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
	&& ((psPathInfo->asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
	 || (psPathInfo->asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
	 || (psPathInfo->asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
	 || (psPathInfo->asDac[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
	 || (psPathInfo->asDac[1].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
	 || (sCurPathInfo.asDit0[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
	 || (sCurPathInfo.asDit1[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
	 || (sCurPathInfo.asDit2[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
	 || (sCurPathInfo.asDac[0].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
	 || (sCurPathInfo.asDac[1].abSrcOnOff[MCDRV_SRC_MIX_BLOCK] & (MCDRV_SRC6_MIX_ON|MCDRV_SRC6_MIX_OFF)) == MCDRV_SRC6_MIX_ON
	 ))
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	ValidateDioParam
 *
 *	Description:
 *			validate digital IO parameters.
 *	Arguments:
 *			psDioInfo	digital IO information
 *			dUpdateInfo	update information
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	ValidateDioParam
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT32					dUpdateInfo
)
{
	SINT32				sdRet	= MCDRV_SUCCESS;
	UINT8				bPort;
	MCDRV_SRC_TYPE_AA		aeDITSource[IOPORT_NUM];
	MCDRV_SRC_TYPE_AA		eAESource	= McResCtrl_GetAESource_AA();
	MCDRV_SRC_TYPE_AA		eDAC0Source	= McResCtrl_GetDACSource_AA(eMCDRV_DAC_MASTER_AA);
	MCDRV_SRC_TYPE_AA		eDAC1Source	= McResCtrl_GetDACSource_AA(eMCDRV_DAC_VOICE_AA);
	UINT8				bDIRUsed[IOPORT_NUM];
	MCDRV_DIO_INFO		sDioInfo;
	MCDRV_DIO_PORT_NO_AA	aePort[IOPORT_NUM]	= {eMCDRV_DIO_0_AA, eMCDRV_DIO_1_AA, eMCDRV_DIO_2_AA};


	McResCtrl_GetDioInfo_AA(&sDioInfo);

	for(bPort = 0; bPort < IOPORT_NUM; bPort++)
	{
		aeDITSource[bPort]	= McResCtrl_GetDITSource_AA(aePort[bPort]);
		bDIRUsed[bPort]		= 0;
	}

	if(eAESource == eMCDRV_SRC_DIR0_AA || eDAC0Source == eMCDRV_SRC_DIR0_AA || eDAC1Source == eMCDRV_SRC_DIR0_AA
	|| aeDITSource[0] == eMCDRV_SRC_DIR0_AA || aeDITSource[1] == eMCDRV_SRC_DIR0_AA || aeDITSource[2] == eMCDRV_SRC_DIR0_AA)
	{
		bDIRUsed[0]	= 1;
	}
	if(eAESource == eMCDRV_SRC_DIR1_AA || eDAC0Source == eMCDRV_SRC_DIR1_AA || eDAC1Source == eMCDRV_SRC_DIR1_AA
	|| aeDITSource[0] == eMCDRV_SRC_DIR1_AA || aeDITSource[1] == eMCDRV_SRC_DIR1_AA || aeDITSource[2] == eMCDRV_SRC_DIR1_AA)
	{
		bDIRUsed[1]	= 1;
	}
	if(eAESource == eMCDRV_SRC_DIR2_AA || eDAC0Source == eMCDRV_SRC_DIR2_AA || eDAC1Source == eMCDRV_SRC_DIR2_AA
	|| aeDITSource[0] == eMCDRV_SRC_DIR2_AA || aeDITSource[1] == eMCDRV_SRC_DIR2_AA || aeDITSource[2] == eMCDRV_SRC_DIR2_AA)
	{
		bDIRUsed[2]	= 1;
	}

	if((bDIRUsed[0] == 1 && ((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != 0 || (dUpdateInfo & MCDRV_DIO0_DIR_UPDATE_FLAG) != 0))
	|| (aeDITSource[0] != eMCDRV_SRC_NONE_AA && ((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != 0 || (dUpdateInfo & MCDRV_DIO0_DIT_UPDATE_FLAG) != 0)))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if((bDIRUsed[1] == 1 && ((dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != 0 || (dUpdateInfo & MCDRV_DIO1_DIR_UPDATE_FLAG) != 0))
	|| (aeDITSource[1] != eMCDRV_SRC_NONE_AA && ((dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != 0 || (dUpdateInfo & MCDRV_DIO1_DIT_UPDATE_FLAG) != 0)))
	{
		return MCDRV_ERROR_ARGUMENT;
	}
	if((bDIRUsed[2] == 1 && ((dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != 0 || (dUpdateInfo & MCDRV_DIO2_DIR_UPDATE_FLAG) != 0))
	|| (aeDITSource[2] != eMCDRV_SRC_NONE_AA && ((dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != 0 || (dUpdateInfo & MCDRV_DIO2_DIT_UPDATE_FLAG) != 0)))
	{
		return MCDRV_ERROR_ARGUMENT;
	}

	if((dUpdateInfo & MCDRV_DIO0_COM_UPDATE_FLAG) != 0)
	{
		sdRet	= CheckDIOCommon(psDioInfo, 0);
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
		sDioInfo.asPortInfo[0].sDioCommon.bInterface	= psDioInfo->asPortInfo[0].sDioCommon.bInterface;
	}
	if((dUpdateInfo & MCDRV_DIO1_COM_UPDATE_FLAG) != 0)
	{
		sdRet	= CheckDIOCommon(psDioInfo, 1);
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
		sDioInfo.asPortInfo[1].sDioCommon.bInterface	= psDioInfo->asPortInfo[1].sDioCommon.bInterface;
	}
	if((dUpdateInfo & MCDRV_DIO2_COM_UPDATE_FLAG) != 0)
	{
		sdRet	= CheckDIOCommon(psDioInfo, 2);
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
		sDioInfo.asPortInfo[2].sDioCommon.bInterface	= psDioInfo->asPortInfo[2].sDioCommon.bInterface;
	}

	if((dUpdateInfo & MCDRV_DIO0_DIR_UPDATE_FLAG) != 0)
	{
		sdRet	= CheckDIODIR(psDioInfo, 0, sDioInfo.asPortInfo[0].sDioCommon.bInterface);
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
	}
	if((dUpdateInfo & MCDRV_DIO1_DIR_UPDATE_FLAG) != 0)
	{
		sdRet	= CheckDIODIR(psDioInfo, 1, sDioInfo.asPortInfo[1].sDioCommon.bInterface);
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
	}
	if((dUpdateInfo & MCDRV_DIO2_DIR_UPDATE_FLAG) != 0)
	{
		sdRet	= CheckDIODIR(psDioInfo, 2, sDioInfo.asPortInfo[2].sDioCommon.bInterface);
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
	}

	if((dUpdateInfo & MCDRV_DIO0_DIT_UPDATE_FLAG) != 0)
	{
		sdRet	= CheckDIODIT(psDioInfo, 0, sDioInfo.asPortInfo[0].sDioCommon.bInterface);
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
	}
	if((dUpdateInfo & MCDRV_DIO1_DIT_UPDATE_FLAG) != 0)
	{
		sdRet	= CheckDIODIT(psDioInfo, 1, sDioInfo.asPortInfo[1].sDioCommon.bInterface);
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
	}
	if((dUpdateInfo & MCDRV_DIO2_DIT_UPDATE_FLAG) != 0)
	{
		sdRet	= CheckDIODIT(psDioInfo, 2, sDioInfo.asPortInfo[2].sDioCommon.bInterface);
		if(sdRet != MCDRV_SUCCESS)
		{
			return sdRet;
		}
	}

	return sdRet;
}


/****************************************************************************
 *	CheckDIOCommon
 *
 *	Description:
 *			validate Digital IO Common parameters.
 *	Arguments:
 *			psDioInfo	digital IO information
 *			bPort		port number
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	CheckDIOCommon
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT8	bPort
)
{

	if(psDioInfo->asPortInfo[bPort].sDioCommon.bInterface == MCDRV_DIO_PCM)
	{
		if(psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_48000
		|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_44100
		|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_32000
		|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_24000
		|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_22050
		|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_12000
		|| psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_11025)
		{
			return MCDRV_ERROR_ARGUMENT;
		}

		if(psDioInfo->asPortInfo[bPort].sDioCommon.bMasterSlave == MCDRV_DIO_MASTER
		&& psDioInfo->asPortInfo[bPort].sDioCommon.bFs == MCDRV_FS_16000)
		{
			if(psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_512)
			{
				return MCDRV_ERROR_ARGUMENT;
			}
		}
	}
	else
	{
		if(psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_512
		|| psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_256
		|| psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_128
		|| psDioInfo->asPortInfo[bPort].sDioCommon.bBckFs == MCDRV_BCKFS_16)
		{
			return MCDRV_ERROR_ARGUMENT;
		}
	}
	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	CheckDIODIR
 *
 *	Description:
 *			validate Digital IO DIR parameters.
 *	Arguments:
 *			psDioInfo	digital IO information
 *			bPort		port number
 *			bInterface	Interface(DA/PCM)
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	CheckDIODIR
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT8	bPort,
	UINT8	bInterface
)
{

	if(bInterface == MCDRV_DIO_PCM)
	{
		if(psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bLaw == MCDRV_PCM_ALAW
		|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bLaw == MCDRV_PCM_MULAW)
		{
			if(psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_13
			|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_14
			|| psDioInfo->asPortInfo[bPort].sDir.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_16)
			{
				return MCDRV_ERROR_ARGUMENT;
			}
		}
	}
	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	CheckDIDIT
 *
 *	Description:
 *			validate Digital IO DIT parameters.
 *	Arguments:
 *			psDioInfo	digital IO information
 *			bPort		port number
 *			bInterface	Interface(DA/PCM)
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *
 ****************************************************************************/
static SINT32	CheckDIODIT
(
	const MCDRV_DIO_INFO*	psDioInfo,
	UINT8	bPort,
	UINT8	bInterface
)
{

	if(bInterface == MCDRV_DIO_PCM)
	{
		if(psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bLaw == MCDRV_PCM_ALAW
		|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bLaw == MCDRV_PCM_MULAW)
		{
			if(psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_13
			|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_14
			|| psDioInfo->asPortInfo[bPort].sDit.sPcmFormat.bBitSel == MCDRV_PCM_BITSEL_16)
			{
				return MCDRV_ERROR_ARGUMENT;
			}
		}
	}
	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	SetVol
 *
 *	Description:
 *			set volume.
 *	Arguments:
 *			dUpdate		target volume items
 *			eMode		update mode
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
static SINT32	SetVol
(
	UINT32					dUpdate,
	MCDRV_VOLUPDATE_MODE_AA	eMode
)
{
	SINT32	sdRet	= McPacket_AddVol_AA(dUpdate, eMode);
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}
	return McDevIf_ExecutePacket_AA();
}

/****************************************************************************
 *	PreUpdatePath
 *
 *	Description:
 *			Preprocess update path.
 *	Arguments:
 *			none
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_RESOURCEOVER
 *
 ****************************************************************************/
static	SINT32	PreUpdatePath
(
void
)
{
	SINT32	sdRet	= MCDRV_SUCCESS;
	UINT8	bReg	= 0;
	UINT8	bReadReg= 0;
	UINT16	wDACMuteParam	= 0;
	UINT16	wDITMuteParam	= 0;
	UINT8	bLAT	= 0;

	switch(McResCtrl_GetDACSource_AA(eMCDRV_DAC_MASTER_AA))
	{
	case	eMCDRV_SRC_PDM_AA:
		bReg	= MCB_AA_DAC_SOURCE_AD;
		break;
	case	eMCDRV_SRC_ADC0_AA:
		bReg	= MCB_AA_DAC_SOURCE_AD;
		break;
	case	eMCDRV_SRC_DIR0_AA:
		bReg	= MCB_AA_DAC_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1_AA:
		bReg	= MCB_AA_DAC_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2_AA:
		bReg	= MCB_AA_DAC_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX_AA:
		bReg	= MCB_AA_DAC_SOURCE_MIX;
		break;
	default:
		bReg	= 0;
	}
	bReadReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_SOURCE);
	if((bReadReg & 0xF0) != 0 && bReg != (bReadReg & 0xF0))
	{/*	DAC source changed	*/
		if((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_MASTER_OUTL)&MCB_AA_MASTER_OUTL) != 0)
		{
			if(McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_MASTER_OUTR) != 0)
			{
				bLAT	= MCB_AA_MASTER_OLAT;
			}
			else
			{
				bLAT	= 0;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_MASTER_OUTL, bLAT);
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_MASTER_OUTR, 0);
		wDACMuteParam	|= (UINT16)(MCB_AA_MASTER_OFLAGL<<8);
		wDACMuteParam	|= MCB_AA_MASTER_OFLAGR;
	}

	switch(McResCtrl_GetDACSource_AA(eMCDRV_DAC_VOICE_AA))
	{
	case	eMCDRV_SRC_PDM_AA:
		bReg	= MCB_AA_VOICE_SOURCE_AD;
		break;
	case	eMCDRV_SRC_ADC0_AA:
		bReg	= MCB_AA_VOICE_SOURCE_AD;
		break;
	case	eMCDRV_SRC_DIR0_AA:
		bReg	= MCB_AA_VOICE_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1_AA:
		bReg	= MCB_AA_VOICE_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2_AA:
		bReg	= MCB_AA_VOICE_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX_AA:
		bReg	= MCB_AA_VOICE_SOURCE_MIX;
		break;
	default:
		bReg	= 0;
	}
	if((bReadReg & 0x0F) != 0 && bReg != (bReadReg & 0x0F))
	{/*	VOICE source changed	*/
		if((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_VOICE_ATTL)&MCB_AA_VOICE_ATTL) != 0)
		{
			if(McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_VOICE_ATTR) != 0)
			{
				bLAT	= MCB_AA_VOICE_LAT;
			}
			else
			{
				bLAT	= 0;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_VOICE_ATTL, bLAT);
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_VOICE_ATTR, 0);
		wDACMuteParam	|= (UINT16)(MCB_AA_VOICE_FLAGL<<8);
		wDACMuteParam	|= MCB_AA_VOICE_FLAGR;
	}

	switch(McResCtrl_GetDITSource_AA(eMCDRV_DIO_0_AA))
	{
	case	eMCDRV_SRC_PDM_AA:
		bReg	= MCB_AA_DIT0_SOURCE_AD;
		break;
	case	eMCDRV_SRC_ADC0_AA:
		bReg	= MCB_AA_DIT0_SOURCE_AD;
		break;
	case	eMCDRV_SRC_DIR0_AA:
		bReg	= MCB_AA_DIT0_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1_AA:
		bReg	= MCB_AA_DIT0_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2_AA:
		bReg	= MCB_AA_DIT0_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX_AA:
		bReg	= MCB_AA_DIT0_SOURCE_MIX;
		break;
	default:
		bReg	= 0;
	}
	bReadReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_SRC_SOURCE);
	if((bReadReg & 0xF0) != 0 && bReg != (bReadReg & 0xF0))
	{/*	DIT source changed	*/
		if((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT0_INVOLL)&MCB_AA_DIT0_INVOLL) != 0)
		{
			if(McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT0_INVOLR) != 0)
			{
				bLAT	= MCB_AA_DIT0_INLAT;
			}
			else
			{
				bLAT	= 0;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIT0_INVOLL, bLAT);
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIT0_INVOLR, 0);
		wDITMuteParam	|= (UINT16)(MCB_AA_DIT0_INVFLAGL<<8);
		wDITMuteParam	|= MCB_AA_DIT0_INVFLAGR;
	}

	switch(McResCtrl_GetDITSource_AA(eMCDRV_DIO_1_AA))
	{
	case	eMCDRV_SRC_PDM_AA:
		bReg	= MCB_AA_DIT1_SOURCE_AD;
		break;
	case	eMCDRV_SRC_ADC0_AA:
		bReg	= MCB_AA_DIT1_SOURCE_AD;
		break;
	case	eMCDRV_SRC_DIR0_AA:
		bReg	= MCB_AA_DIT1_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1_AA:
		bReg	= MCB_AA_DIT1_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2_AA:
		bReg	= MCB_AA_DIT1_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX_AA:
		bReg	= MCB_AA_DIT1_SOURCE_MIX;
		break;
	default:
		bReg	= 0;
	}
	if((bReadReg & 0x0F) != 0 && bReg != (bReadReg & 0x0F))
	{/*	DIT source changed	*/
		if((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT1_INVOLL)&MCB_AA_DIT1_INVOLL) != 0)
		{
			if(McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT1_INVOLR) != 0)
			{
				bLAT	= MCB_AA_DIT1_INLAT;
			}
			else
			{
				bLAT	= 0;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIT1_INVOLL, bLAT);
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIT1_INVOLR, 0);
		wDITMuteParam	|= (UINT16)(MCB_AA_DIT1_INVFLAGL<<8);
		wDITMuteParam	|= MCB_AA_DIT1_INVFLAGR;
	}

	switch(McResCtrl_GetDITSource_AA(eMCDRV_DIO_2_AA))
	{
	case	eMCDRV_SRC_PDM_AA:
		bReg	= MCB_AA_DIT2_SOURCE_AD;
		break;
	case	eMCDRV_SRC_ADC0_AA:
		bReg	= MCB_AA_DIT2_SOURCE_AD;
		break;
	case	eMCDRV_SRC_DIR0_AA:
		bReg	= MCB_AA_DIT2_SOURCE_DIR0;
		break;
	case	eMCDRV_SRC_DIR1_AA:
		bReg	= MCB_AA_DIT2_SOURCE_DIR1;
		break;
	case	eMCDRV_SRC_DIR2_AA:
		bReg	= MCB_AA_DIT2_SOURCE_DIR2;
		break;
	case	eMCDRV_SRC_MIX_AA:
		bReg	= MCB_AA_DIT2_SOURCE_MIX;
		break;
	default:
		bReg	= 0;
	}
	bReadReg	= McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_SRC_SOURCE_1);
	if((bReadReg & 0x0F) != 0 && bReg != (bReadReg & 0x0F))
	{/*	DIT source changed	*/
		if((McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT2_INVOLL)&MCB_AA_DIT2_INVOLL) != 0)
		{
			if(McResCtrl_GetRegVal_AA(MCDRV_PACKET_REGTYPE_B_MIXER_AA, MCI_AA_DIT2_INVOLR) != 0)
			{
				bLAT	= MCB_AA_DIT2_INLAT;
			}
			else
			{
				bLAT	= 0;
			}
			McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIT2_INVOLL, bLAT);
		}
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_WRITE_AA | MCDRV_PACKET_REGTYPE_B_MIXER_AA | MCI_AA_DIT2_INVOLR, 0);
		wDITMuteParam	|= (UINT16)(MCB_AA_DIT2_INVFLAGL<<8);
		wDITMuteParam	|= MCB_AA_DIT2_INVFLAGR;
	}

	sdRet	= McDevIf_ExecutePacket_AA();
	if(sdRet != MCDRV_SUCCESS)
	{
		return sdRet;
	}

	if(wDACMuteParam != 0)
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_EVTWAIT_AA | MCDRV_EVT_DACMUTE_AA | wDACMuteParam, 0);
	}
	if(wDITMuteParam != 0)
	{
		McDevIf_AddPacket_AA(MCDRV_PACKET_TYPE_EVTWAIT_AA | MCDRV_EVT_DITMUTE_AA | wDITMuteParam, 0);
	}
	/*	do not Execute & Clear	*/
	return MCDRV_SUCCESS;
}

/****************************************************************************
 *	McDrv_Ctrl
 *
 *	Description:
 *			MC Driver I/F function.
 *	Arguments:
 *			dCmd		command #
 *			pvPrm		parameter
 *			dPrm		update info
 *	Return:
 *			MCDRV_SUCCESS
 *			MCDRV_ERROR_ARGUMENT
 *			MCDRV_ERROR_RESOURCEOVER
 *			MCDRV_ERROR_STATE
 *
 ****************************************************************************/
SINT32	McDrv_Ctrl_AA
(
	UINT32	dCmd,
	void*	pvPrm,
	UINT32	dPrm
)
{
	SINT32	sdRet	= MCDRV_ERROR;

	switch(dCmd)
	{
	case	MCDRV_INIT:	return init((MCDRV_INIT_INFO*)pvPrm);
	case	MCDRV_TERM:	return term();
	default:			break;
	}

	McSrv_Lock();

	switch (dCmd)
	{
	case	MCDRV_UPDATE_CLOCK:
		sdRet	= update_clock((MCDRV_CLOCK_INFO*)pvPrm);
		break;
	case	MCDRV_GET_PATH:
		sdRet	= get_path((MCDRV_PATH_INFO*)pvPrm);
		break;
	case	MCDRV_SET_PATH:
		sdRet	= set_path((MCDRV_PATH_INFO*)pvPrm);
		break;
	case	MCDRV_GET_VOLUME:
		sdRet	= get_volume((MCDRV_VOL_INFO*)pvPrm);
		break;
	case	MCDRV_SET_VOLUME:
		sdRet	= set_volume((MCDRV_VOL_INFO*)pvPrm);
		break;
	case	MCDRV_GET_DIGITALIO:
		sdRet	= get_digitalio((MCDRV_DIO_INFO*)pvPrm);
		break;
	case	MCDRV_SET_DIGITALIO:
		sdRet	= set_digitalio((MCDRV_DIO_INFO*)pvPrm, dPrm);
		break;
	case	MCDRV_GET_DAC:
		sdRet	= get_dac((MCDRV_DAC_INFO*)pvPrm);
		break;
	case	MCDRV_SET_DAC:
		sdRet	= set_dac((MCDRV_DAC_INFO*)pvPrm, dPrm);
		break;
	case	MCDRV_GET_ADC:
		sdRet	= get_adc((MCDRV_ADC_INFO*)pvPrm);
		break;
	case	MCDRV_SET_ADC:
		sdRet	= set_adc((MCDRV_ADC_INFO*)pvPrm, dPrm);
		break;
	case	MCDRV_GET_SP:
		sdRet	= get_sp((MCDRV_SP_INFO*)pvPrm);
		break;
	case	MCDRV_SET_SP:
		sdRet	= set_sp((MCDRV_SP_INFO*)pvPrm);
		break;
	case	MCDRV_GET_DNG:
		sdRet	= get_dng((MCDRV_DNG_INFO*)pvPrm);
		break;
	case	MCDRV_SET_DNG:
		sdRet	= set_dng((MCDRV_DNG_INFO*)pvPrm, dPrm);
		break;
	case	MCDRV_SET_AUDIOENGINE:
		sdRet	= set_ae((MCDRV_AE_INFO*)pvPrm, dPrm);
		break;
	case	MCDRV_GET_PDM:
		sdRet	= get_pdm((MCDRV_PDM_INFO*)pvPrm);
		break;
	case	MCDRV_SET_PDM:
		sdRet	= set_pdm((MCDRV_PDM_INFO*)pvPrm, dPrm);
		break;
	case MCDRV_CONFIG_GP:
		sdRet	= config_gp((MCDRV_GP_MODE*)pvPrm);
		break;
	case MCDRV_MASK_GP:
		sdRet	= mask_gp((UINT8*)pvPrm, dPrm);
		break;
	case MCDRV_GETSET_GP:
		sdRet	= getset_gp((UINT8*)pvPrm, dPrm);
		break;
	case MCDRV_READ_REG :
		sdRet	= read_reg((MCDRV_REG_INFO*)pvPrm);
		break;
	case MCDRV_WRITE_REG :
		sdRet	= write_reg((MCDRV_REG_INFO*)pvPrm);
		break;
	case MCDRV_GET_SYSEQ :
	case MCDRV_SET_SYSEQ :
		sdRet	= MCDRV_SUCCESS;
		break;

	case MCDRV_IRQ:
		break;

	default:
		break;
	}

	McSrv_Unlock();

	return sdRet;
}
