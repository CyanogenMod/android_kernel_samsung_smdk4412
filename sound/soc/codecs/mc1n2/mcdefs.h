/****************************************************************************
 *
 *		Copyright(c) 2010 Yamaha Corporation. All rights reserved.
 *
 *		Module		: mcdefs.h
 *
 *		Description	: MC Device Definitions
 *
 *		Version		: 1.0.0 	2010.07.05
 *
 ****************************************************************************/

#ifndef _MCDEFS_H_
#define	_MCDEFS_H_

/*	Register Definition

	[Naming Rules]

	  MCI_xxx		: Registers
	  MCI_xxx_DEF	: Default setting of registers
	  MCB_xxx		: Miscelleneous bit definition
*/

/*	Registers	*/
/*	A_ADR	*/
#define	MCI_RST						(0)
#define	MCB_RST						(0x01)
#define	MCI_RST_DEF					(MCB_RST)

#define	MCI_BASE_ADR				(1)
#define	MCI_BASE_WINDOW				(2)

#define	MCI_HW_ID					(8)
#define	MCI_HW_ID_DEF				(0x79)

#define	MCI_ANA_ADR					(12)
#define	MCI_ANA_WINDOW				(13)

#define	MCI_CD_ADR					(14)
#define	MCI_CD_WINDOW				(15)

#define	MCI_MIX_ADR					(16)
#define	MCI_MIX_WINDOW				(17)

#define	MCI_AE_ADR					(18)
#define	MCI_AE_WINDOW				(19)

#define	MCI_BDSP_ST					(20)
#define	MCB_EQ5ON					(0x80)
#define	MCB_DRCON					(0x40)
#define	MCB_EQ3ON					(0x20)
#define	MCB_DBEXON					(0x08)
#define	MCB_BDSP_ST					(0x01)

#define	MCI_BDSP_RST				(21)
#define	MCB_TRAM_RST				(0x02)
#define	MCB_BDSP_RST				(0x01)

#define	MCI_BDSP_ADR				(22)
#define	MCI_BDSP_WINDOW				(23)

#define	MCI_CDSP_ADR				(24)
#define	MCI_CDSP_WINDOW				(25)

/*	B_ADR(BASE)	*/
#define	MCI_RSTB					(0)
#define	MCB_RSTB					(0x10)
#define	MCI_RSTB_DEF				(MCB_RSTB)

#define	MCI_PWM_DIGITAL				(1)
#define	MCB_PWM_DP2					(0x04)
#define	MCB_PWM_DP1					(0x02)
#define	MCI_PWM_DIGITAL_DEF			(MCB_PWM_DP2 | MCB_PWM_DP1)

#define	MCI_PWM_DIGITAL_1			(3)
#define	MCB_PWM_DPPDM				(0x10)
#define	MCB_PWM_DPDI2				(0x08)
#define	MCB_PWM_DPDI1				(0x04)
#define	MCB_PWM_DPDI0				(0x02)
#define	MCB_PWM_DPB					(0x01)
#define	MCI_PWM_DIGITAL_1_DEF		(MCB_PWM_DPPDM | MCB_PWM_DPDI2 | MCB_PWM_DPDI1 | MCB_PWM_DPDI0 | MCB_PWM_DPB)

#define	MCI_PWM_DIGITAL_CDSP		(4)
#define	MCB_PWM_DPCDSP				(0x00)
#define	MCI_PWM_DIGITAL_CDSP_DEF	(MCB_PWM_DPCDSP)

#define	MCI_PWM_DIGITAL_BDSP		(6)
#define	MCI_PWM_DIGITAL_BDSP_DEF	(MCB_PWM_DPBDSP)
#define	MCB_PWM_DPBDSP				(0x01)

#define	MCI_SD_MSK					(9)
#define	MCB_SDIN_MSK2				(0x80)
#define	MCB_SDO_DDR2				(0x10)
#define	MCB_SDIN_MSK1				(0x08)
#define	MCB_SDO_DDR1				(0x01)
#define	MCI_SD_MSK_DEF				(MCB_SDIN_MSK2 | MCB_SDIN_MSK1)

#define	MCI_SD_MSK_1				(10)
#define	MCB_SDIN_MSK0				(0x80)
#define	MCB_SDO_DDR0				(0x10)
#define	MCI_SD_MSK_1_DEF			(MCB_SDIN_MSK0)

#define	MCI_BCLK_MSK				(11)
#define	MCB_BCLK_MSK2				(0x80)
#define	MCB_BCLK_DDR2				(0x40)
#define	MCB_LRCK_MSK2				(0x20)
#define	MCB_LRCK_DDR2				(0x10)
#define	MCB_BCLK_MSK1				(0x08)
#define	MCB_BCLK_DDR1				(0x04)
#define	MCB_LRCK_MSK1				(0x02)
#define	MCB_LRCK_DDR1				(0x01)
#define	MCI_BCLK_MSK_DEF			(MCB_BCLK_MSK2 | MCB_LRCK_MSK2 | MCB_BCLK_MSK1 | MCB_LRCK_MSK1)

#define	MCI_BCLK_MSK_1				(12)
#define	MCB_BCLK_MSK0				(0x80)
#define	MCB_BCLK_DDR0				(0x40)
#define	MCB_LRCK_MSK0				(0x20)
#define	MCB_LRCK_DDR0				(0x10)
#define	MCB_PCMOUT_HIZ2				(0x08)
#define	MCB_PCMOUT_HIZ1				(0x04)
#define	MCB_PCMOUT_HIZ0				(0x02)
#define	MCB_ROUTER_MS				(0x01)
#define	MCI_BCLK_MSK_1_DEF			(MCB_BCLK_MSK0 | MCB_LRCK_MSK0)

#define	MCI_BCKP					(13)
#define	MCB_DI2_BCKP				(0x04)
#define	MCB_DI1_BCKP				(0x02)
#define	MCB_DI0_BCKP				(0x01)
#define	MCI_BCKP_DEF				(0)

#define	MCI_BYPASS					(21)
#define	MCB_LOCK1					(0x80)
#define	MCB_LOCK0					(0x40)
#define	MCB_BYPASS1					(0x02)
#define	MCB_BYPASS0					(0x01)

#define	MCI_EPA_IRQ					(22)
#define	MCB_EPA2_IRQ				(0x04)
#define	MCB_EPA1_IRQ				(0x02)
#define	MCB_EPA0_IRQ				(0x01)

#define	MCI_PA_FLG					(23)
#define	MCB_PA2_FLAG				(0x04)
#define	MCB_PA1_FLAG				(0x02)
#define	MCB_PA0_FLAG				(0x01)

#define	MCI_PA_MSK					(26)
#define	MCB_PA1_MSK					(0x80)
#define	MCB_PA1_DDR					(0x40)
#define	MCB_PA0_MSK					(0x08)
#define	MCB_PA0_DDR					(0x04)
#define	MCI_PA_MSK_DEF				(MCB_PA1_MSK | MCB_PA0_MSK)

#define	MCI_PA_HOST					(28)
#define	MCI_PA_HOST_1				(29)

#define	MCI_PA_OUT					(30)
#define	MCB_PA_OUT					(0x01)

#define	MCI_PA_SCU_PA				(31)
#define	MCB_PA_SCU_PA0				(0x01)
#define	MCB_PA_SCU_PA1				(0x02)

/*	B_ADR(MIX)	*/
#define	MCI_DIT_INVFLAGL			(0)
#define	MCB_DIT0_INVFLAGL			(0x20)
#define	MCB_DIT1_INVFLAGL			(0x10)
#define	MCB_DIT2_INVFLAGL			(0x08)

#define	MCI_DIT_INVFLAGR			(1)
#define	MCB_DIT0_INVFLAGR			(0x20)
#define	MCB_DIT1_INVFLAGR			(0x10)
#define	MCB_DIT2_INVFLAGR			(0x08)

#define	MCI_DIR_VFLAGL				(2)
#define	MCB_PDM0_VFLAGL				(0x80)
#define	MCB_DIR0_VFLAGL				(0x20)
#define	MCB_DIR1_VFLAGL				(0x10)
#define	MCB_DIR2_VFLAGL				(0x08)

#define	MCI_DIR_VFLAGR				(3)
#define	MCB_PDM0_VFLAGR				(0x80)
#define	MCB_DIR0_VFLAGR				(0x20)
#define	MCB_DIR1_VFLAGR				(0x10)
#define	MCB_DIR2_VFLAGR				(0x08)

#define	MCI_AD_VFLAGL				(4)
#define	MCB_ADC_VFLAGL				(0x80)
#define	MCB_AENG6_VFLAGL			(0x20)

#define	MCI_AD_VFLAGR				(5)
#define	MCB_ADC_VFLAGR				(0x80)
#define	MCB_AENG6_VFLAGR			(0x20)

#define	MCI_AFLAGL					(6)
#define	MCB_ADC_AFLAGL				(0x40)
#define	MCB_DIR0_AFLAGL				(0x20)
#define	MCB_DIR1_AFLAGL				(0x10)
#define	MCB_DIR2_AFLAGL				(0x04)

#define	MCI_AFLAGR					(7)
#define	MCB_ADC_AFLAGR				(0x40)
#define	MCB_DIR0_AFLAGR				(0x20)
#define	MCB_DIR1_AFLAGR				(0x10)
#define	MCB_DIR2_AFLAGR				(0x04)

#define	MCI_DAC_INS_FLAG			(8)
#define	MCB_DAC_INS_FLAG			(0x80)

#define	MCI_INS_FLAG				(9)
#define	MCB_ADC_INS_FLAG			(0x40)
#define	MCB_DIR0_INS_FLAG			(0x20)
#define	MCB_DIR1_INS_FLAG			(0x10)
#define	MCB_DIR2_INS_FLAG			(0x04)

#define	MCI_DAC_FLAGL				(10)
#define	MCB_ST_FLAGL				(0x80)
#define	MCB_MASTER_OFLAGL			(0x40)
#define	MCB_VOICE_FLAGL				(0x10)
#define	MCB_DAC_FLAGL				(0x02)

#define	MCI_DAC_FLAGR				(11)
#define	MCB_ST_FLAGR				(0x80)
#define	MCB_MASTER_OFLAGR			(0x40)
#define	MCB_VOICE_FLAGR				(0x10)
#define	MCB_DAC_FLAGR				(0x02)

#define	MCI_DIT0_INVOLL				(12)
#define	MCB_DIT0_INLAT				(0x80)
#define	MCB_DIT0_INVOLL				(0x7F)

#define	MCI_DIT0_INVOLR				(13)
#define	MCB_DIT0_INVOLR				(0x7F)

#define	MCI_DIT1_INVOLL				(14)
#define	MCB_DIT1_INLAT				(0x80)
#define	MCB_DIT1_INVOLL				(0x7F)

#define	MCI_DIT1_INVOLR				(15)
#define	MCB_DIT1_INVOLR				(0x7F)

#define	MCI_DIT2_INVOLL				(16)
#define	MCB_DIT2_INLAT				(0x80)
#define	MCB_DIT2_INVOLL				(0x7F)

#define	MCI_DIT2_INVOLR				(17)
#define	MCB_DIT2_INVOLR				(0x7F)

#define	MCI_ESRC0_INVOLL			(16)
#define	MCI_ESRC0_INVOLR			(17)

#define	MCI_PDM0_VOLL				(24)
#define	MCB_PDM0_LAT				(0x80)
#define	MCB_PDM0_VOLL				(0x7F)

#define	MCI_PDM0_VOLR				(25)
#define	MCB_PDM0_VOLR				(0x7F)

#define	MCI_DIR0_VOLL				(28)
#define	MCB_DIR0_LAT				(0x80)
#define	MCB_DIR0_VOLL				(0x7F)

#define	MCI_DIR0_VOLR				(29)
#define	MCB_DIR0_VOLR				(0x7F)

#define	MCI_DIR1_VOLL				(30)
#define	MCB_DIR1_LAT				(0x80)
#define	MCB_DIR1_VOLL				(0x7F)

#define	MCI_DIR1_VOLR				(31)
#define	MCB_DIR1_VOLR				(0x7F)

#define	MCI_DIR2_VOLL				(32)
#define	MCB_DIR2_LAT				(0x80)
#define	MCB_DIR2_VOLL				(0x7F)

#define	MCI_DIR2_VOLR				(33)
#define	MCB_DIR2_VOLR				(0x7F)
/*
#define	MCI_ADC1_VOLL				(38?)
#define	MCB_ADC1_LAT				(0x80)
#define	MCB_ADC1_VOLL				(0x7F)

#define	MCI_ADC1_VOLR				(39?)
#define	MCB_ADC1_VOLR				(0x7F)
*/
#define	MCI_ADC_VOLL				(40)
#define	MCB_ADC_LAT					(0x80)
#define	MCB_ADC_VOLL				(0x7F)

#define	MCI_ADC_VOLR				(41)
#define	MCB_ADC_VOLR				(0x7F)
/*
#define	MCI_DTMFB_VOLL				(42)
#define	MCI_DTMFB_VOLR				(43)
*/
#define	MCI_AENG6_VOLL				(44)
#define	MCB_AENG6_LAT				(0x80)
#define	MCB_AENG6_VOLL				(0x7F)

#define	MCI_AENG6_VOLR				(45)
#define	MCB_AENG6_VOLR				(0x7F)

#define	MCI_DIT_ININTP				(50)
#define	MCB_DIT0_ININTP				(0x20)
#define	MCB_DIT1_ININTP				(0x10)
#define	MCB_DIT2_ININTP				(0x08)
#define	MCI_DIT_ININTP_DEF			(MCB_DIT0_ININTP | MCB_DIT1_ININTP | MCB_DIT2_ININTP)

#define	MCI_DIR_INTP				(51)
#define	MCB_PDM0_INTP				(0x80)
#define	MCB_DIR0_INTP				(0x20)
#define	MCB_DIR1_INTP				(0x10)
#define	MCB_DIR2_INTP				(0x08)
#define	MCB_ADC2_INTP				(0x01)
#define	MCI_DIR_INTP_DEF			(MCB_PDM0_INTP | MCB_DIR0_INTP | MCB_DIR1_INTP | MCB_DIR2_INTP)

#define	MCI_ADC_INTP				(52)
#define	MCB_ADC_INTP				(0x80)
#define	MCB_AENG6_INTP				(0x20)
#define	MCI_ADC_INTP_DEF			(MCB_ADC_INTP | MCB_AENG6_INTP)

#define	MCI_ADC_ATTL				(54)
#define	MCB_ADC_ALAT				(0x80)
#define	MCB_ADC_ATTL				(0x7F)

#define	MCI_ADC_ATTR				(55)
#define	MCB_ADC_ATTR				(0x7F)

#define	MCI_DIR0_ATTL				(56)
#define	MCB_DIR0_ALAT				(0x80)
#define	MCB_DIR0_ATTL				(0x7F)

#define	MCI_DIR0_ATTR				(57)
#define	MCB_DIR0_ATTR				(0x7F)

#define	MCI_DIR1_ATTL				(58)
#define	MCB_DIR1_ALAT				(0x80)
#define	MCB_DIR1_ATTL				(0x7F)

#define	MCI_DIR1_ATTR				(59)
#define	MCB_DIR1_ATTR				(0x7F)
/*
#define	MCI_ADC2_ATTL				(60)
#define	MCI_ADC2_ATTR				(61)
*/
#define	MCI_DIR2_ATTL				(62)
#define	MCB_DIR2_ALAT				(0x80)
#define	MCB_DIR2_ATTL				(0x7F)

#define	MCI_DIR2_ATTR				(63)
#define	MCB_DIR2_ATTR				(0x7F)

#define	MCI_AINTP					(72)
#define	MCB_ADC_AINTP				(0x40)
#define	MCB_DIR0_AINTP				(0x20)
#define	MCB_DIR1_AINTP				(0x10)
#define	MCB_DIR2_AINTP				(0x04)
#define	MCI_AINTP_DEF				(MCB_ADC_AINTP | MCB_DIR0_AINTP | MCB_DIR1_AINTP | MCB_DIR2_AINTP)

#define	MCI_DAC_INS					(74)
#define	MCB_DAC_INS					(0x80)

#define	MCI_INS						(75)
#define	MCB_ADC_INS					(0x40)
#define	MCB_DIR0_INS				(0x20)
#define	MCB_DIR1_INS				(0x10)
#define	MCB_DIR2_INS				(0x04)

#define	MCI_IINTP					(76)
#define	MCB_DAC_IINTP				(0x80)
#define	MCB_ADC_IINTP				(0x40)
#define	MCB_DIR0_IINTP				(0x20)
#define	MCB_DIR1_IINTP				(0x10)
#define	MCB_DIR2_IINTP				(0x04)
#define	MCI_IINTP_DEF				(MCB_DAC_IINTP | MCB_ADC_IINTP | MCB_DIR0_IINTP | MCB_DIR1_IINTP | MCB_DIR2_IINTP)

#define	MCI_ST_VOLL					(77)
#define	MCB_ST_LAT					(0x80)
#define	MCB_ST_VOLL					(0x7F)

#define	MCI_ST_VOLR					(78)
#define	MCB_ST_VOLR					(0x7F)

#define	MCI_MASTER_OUTL				(79)
#define	MCB_MASTER_OLAT				(0x80)
#define	MCB_MASTER_OUTL				(0x7F)

#define	MCI_MASTER_OUTR				(80)
#define	MCB_MASTER_OUTR				(0x7F)

#define	MCI_VOICE_ATTL				(83)
#define	MCB_VOICE_LAT				(0x80)
#define	MCB_VOICE_ATTL				(0x7F)

#define	MCI_VOICE_ATTR				(84)
#define	MCB_VOICE_ATTR				(0x7F)
/*
#define	MCI_DTMF_ATTL				(85)
#define	MCI_DTMF_ATTR				(86)
*/
#define	MCI_DAC_ATTL				(89)
#define	MCB_DAC_LAT					(0x80)
#define	MCB_DAC_ATTL				(0x7F)

#define	MCI_DAC_ATTR				(90)
#define	MCB_DAC_ATTR				(0x7F)

#define	MCI_DAC_INTP				(93)
#define	MCB_ST_INTP					(0x80)
#define	MCB_MASTER_OINTP			(0x40)
#define	MCB_VOICE_INTP				(0x10)
/*#define	MCB_DTMF_INTP				(0x08)*/
#define	MCB_DAC_INTP				(0x02)
#define	MCI_DAC_INTP_DEF			(MCB_ST_INTP | MCB_MASTER_OINTP | MCB_VOICE_INTP | MCB_DAC_INTP)

#define	MCI_SOURCE					(110)
#define	MCB_DAC_SOURCE_AD			(0x10)
#define	MCB_DAC_SOURCE_DIR2			(0x20)
#define	MCB_DAC_SOURCE_DIR0			(0x30)
#define	MCB_DAC_SOURCE_DIR1			(0x40)
#define	MCB_DAC_SOURCE_MIX			(0x70)
#define	MCB_VOICE_SOURCE_AD			(0x01)
#define	MCB_VOICE_SOURCE_DIR2		(0x02)
#define	MCB_VOICE_SOURCE_DIR0		(0x03)
#define	MCB_VOICE_SOURCE_DIR1		(0x04)
#define	MCB_VOICE_SOURCE_MIX		(0x07)

#define	MCI_SWP						(111)

#define	MCI_SRC_SOURCE				(112)
#define	MCB_DIT0_SOURCE_AD			(0x10)
#define	MCB_DIT0_SOURCE_DIR2		(0x20)
#define	MCB_DIT0_SOURCE_DIR0		(0x30)
#define	MCB_DIT0_SOURCE_DIR1		(0x40)
#define	MCB_DIT0_SOURCE_MIX			(0x70)
#define	MCB_DIT1_SOURCE_AD			(0x01)
#define	MCB_DIT1_SOURCE_DIR2		(0x02)
#define	MCB_DIT1_SOURCE_DIR0		(0x03)
#define	MCB_DIT1_SOURCE_DIR1		(0x04)
#define	MCB_DIT1_SOURCE_MIX			(0x07)

#define	MCI_SRC_SOURCE_1			(113)
#define	MCB_AE_SOURCE_AD			(0x10)
#define	MCB_AE_SOURCE_DIR2			(0x20)
#define	MCB_AE_SOURCE_DIR0			(0x30)
#define	MCB_AE_SOURCE_DIR1			(0x40)
#define	MCB_AE_SOURCE_MIX			(0x70)
#define	MCB_DIT2_SOURCE_AD			(0x01)
#define	MCB_DIT2_SOURCE_DIR2		(0x02)
#define	MCB_DIT2_SOURCE_DIR0		(0x03)
#define	MCB_DIT2_SOURCE_DIR1		(0x04)
#define	MCB_DIT2_SOURCE_MIX			(0x07)

#define	MCI_ESRC_SOURCE				(114)

#define	MCI_AENG6_SOURCE			(115)
#define	MCB_AENG6_ADC0				(0x00)
#define	MCB_AENG6_PDM				(0x01)

#define	MCI_EFIFO_SOURCE			(116)

#define	MCI_SRC_SOURCE_2			(117)

#define	MCI_PEAK_METER				(121)

#define	MCI_OVFL					(122)
#define	MCI_OVFR					(123)

#define	MCI_DIMODE0					(130)

#define	MCI_DIRSRC_RATE0_MSB		(131)

#define	MCI_DIRSRC_RATE0_LSB		(132)

#define	MCI_DITSRC_RATE0_MSB		(133)

#define	MCI_DITSRC_RATE0_LSB		(134)

#define	MCI_DI_FS0					(135)

/*	DI Common Parameter	*/
#define	MCB_DICOMMON_SRC_RATE_SET	(0x01)

#define	MCI_DI0_SRC					(136)

#define	MCI_DIX0_START				(137)
#define	MCB_DITIM0_START			(0x40)
#define	MCB_DIR0_SRC_START			(0x08)
#define	MCB_DIR0_START				(0x04)
#define	MCB_DIT0_SRC_START			(0x02)
#define	MCB_DIT0_START				(0x01)

#define	MCI_DIX0_FMT				(142)

#define	MCI_DIR0_CH					(143)
#define	MCI_DIR0_CH_DEF				(0x10)

#define	MCI_DIT0_SLOT				(144)
#define	MCI_DIT0_SLOT_DEF			(0x10)

#define	MCI_HIZ_REDGE0				(145)

#define	MCI_PCM_RX0					(146)
#define	MCB_PCM_MONO_RX0			(0x80)
#define	MCI_PCM_RX0_DEF				(MCB_PCM_MONO_RX0)

#define	MCI_PCM_SLOT_RX0			(147)

#define	MCI_PCM_TX0					(148)
#define	MCB_PCM_MONO_TX0			(0x80)
#define	MCI_PCM_TX0_DEF				(MCB_PCM_MONO_TX0)

#define	MCI_PCM_SLOT_TX0			(149)
#define	MCI_PCM_SLOT_TX0_DEF		(0x10)

#define	MCI_DIMODE1					(150)

#define	MCI_DIRSRC_RATE1_MSB		(151)
#define	MCI_DIRSRC_RATE1_LSB		(152)

#define	MCI_DITSRC_RATE1_MSB		(153)
#define	MCI_DITSRC_RATE1_LSB		(154)

#define	MCI_DI_FS1					(155)

#define	MCI_DI1_SRC					(156)

#define	MCI_DIX1_START				(157)
#define	MCB_DITIM1_START			(0x40)
#define	MCB_DIR1_SRC_START			(0x08)
#define	MCB_DIR1_START				(0x04)
#define	MCB_DIT1_SRC_START			(0x02)
#define	MCB_DIT1_START				(0x01)

#define	MCI_DIX1_FMT				(162)

#define	MCI_DIR1_CH					(163)
#define	MCB_DIR1_CH1				(0x10)
#define	MCI_DIR1_CH_DEF				(MCB_DIR1_CH1)

#define	MCI_DIT1_SLOT				(164)
#define	MCB_DIT1_SLOT1				(0x10)
#define	MCI_DIT1_SLOT_DEF			(MCB_DIT1_SLOT1)

#define	MCI_HIZ_REDGE1				(165)

#define	MCI_PCM_RX1					(166)
#define	MCB_PCM_MONO_RX1			(0x80)
#define	MCI_PCM_RX1_DEF				(MCB_PCM_MONO_RX1)

#define	MCI_PCM_SLOT_RX1			(167)

#define	MCI_PCM_TX1					(168)
#define	MCB_PCM_MONO_TX1			(0x80)
#define	MCI_PCM_TX1_DEF				(MCB_PCM_MONO_TX1)

#define	MCI_PCM_SLOT_TX1			(169)
#define	MCI_PCM_SLOT_TX1_DEF		(0x10)

#define	MCI_DIMODE2					(170)

#define	MCI_DIRSRC_RATE2_MSB		(171)
#define	MCI_DIRSRC_RATE2_LSB		(172)

#define	MCI_DITSRC_RATE2_MSB		(173)
#define	MCI_DITSRC_RATE2_LSB		(174)

#define	MCI_DI_FS2					(175)

#define	MCI_DI2_SRC					(176)

#define	MCI_DIX2_START				(177)
#define	MCB_DITIM2_START			(0x40)
#define	MCB_DIR2_SRC_START			(0x08)
#define	MCB_DIR2_START				(0x04)
#define	MCB_DIT2_SRC_START			(0x02)
#define	MCB_DIT2_START				(0x01)

#define	MCI_DIX2_FMT				(182)

#define	MCI_DIR2_CH					(183)
#define	MCB_DIR2_CH1				(0x10)
#define	MCB_DIR2_CH0				(0x01)
#define	MCI_DIR2_CH_DEF				(MCB_DIR2_CH1)

#define	MCI_DIT2_SLOT				(184)
#define	MCB_DIT2_SLOT1				(0x10)
#define	MCB_DIT2_SLOT0				(0x01)
#define	MCI_DIT2_SLOT_DEF			(MCB_DIT2_SLOT1)

#define	MCI_HIZ_REDGE2				(185)

#define	MCI_PCM_RX2					(186)
#define	MCB_PCM_MONO_RX2			(0x80)
#define	MCI_PCM_RX2_DEF				(MCB_PCM_MONO_RX2)

#define	MCI_PCM_SLOT_RX2			(187)

#define	MCI_PCM_TX2					(188)
#define	MCB_PCM_MONO_TX2			(0x80)
#define	MCI_PCM_TX2_DEF				(MCB_PCM_MONO_TX2)

#define	MCI_PCM_SLOT_TX2			(189)
#define	MCI_PCM_SLOT_TX2_DEF		(0x10)

#define	MCI_CD_START				(192)

#define	MCI_CDI_CH					(193)
#define	MCI_CDI_CH_DEF				(0xE4)

#define	MCI_CDO_SLOT				(194)
#define	MCI_CDO_SLOT_DEF			(0xE4)

#define	MCI_PDM_AGC					(200)
#define	MCI_PDM_AGC_DEF				(0x03)

#define	MCI_PDM_START				(202)
#define	MCB_PDM_MN					(0x02)
#define	MCB_PDM_START				(0x01)

#define	MCI_PDM_STWAIT				(205)
#define	MCI_PDM_STWAIT_DEF			(0x40)

#define	MCI_HP_ID					(206)

#define	MCI_CHP_H					(207)
#define	MCI_CHP_H_DEF				(0x3F)

#define	MCI_CHP_M					(208)
#define	MCI_CHP_M_DEF				(0xEA)

#define	MCI_CHP_L					(209)
#define	MCI_CHP_L_DEF				(0x94)

#define	MCI_SINGEN0_VOL				(210)
#define	MCI_SINGEN1_VOL				(211)

#define	MCI_SINGEN_FREQ0_MSB		(212)
#define	MCI_SINGEN_FREQ0_LSB		(213)

#define	MCI_SINGEN_FREQ1_MSB		(214)
#define	MCI_SINGEN_FREQ1_LSB		(215)

#define	MCI_SINGEN_GATETIME			(216)

#define	MCI_SINGEN_FLAG				(217)

/*	BADR(AE)	*/
#define	MCI_BAND0_CEQ0				(0)
#define	MCI_BAND0_CEQ0_H_DEF		(0x10)

#define	MCI_BAND1_CEQ0				(15)
#define	MCI_BAND1_CEQ0_H_DEF		(0x10)

#define	MCI_BAND2_CEQ0				(30)
#define	MCI_BAND2_CEQ0_H_DEF		(0x10)

#define	MCI_BAND3H_CEQ0				(45)
#define	MCI_BAND3H_CEQ0_H_DEF		(0x10)

#define	MCI_BAND4H_CEQ0				(75)
#define	MCI_BAND4H_CEQ0_H_DEF		(0x10)

#define	MCI_BAND5_CEQ0				(105)
#define	MCI_BAND5_CEQ0_H_DEF		(0x10)

#define	MCI_BAND6H_CEQ0				(120)
#define	MCI_BAND6H_CEQ0_H_DEF		(0x10)

#define	MCI_BAND7H_CEQ0				(150)
#define	MCI_BAND7H_CEQ0_H_DEF		(0x10)

#define	MCI_PDM_CHP0_H				(240)
#define	MCI_PDM_CHP0_H_DEF			(0x3F)
#define	MCI_PDM_CHP0_M				(241)
#define	MCI_PDM_CHP0_M_DEF			(0xEA)
#define	MCI_PDM_CHP0_L				(242)
#define	MCI_PDM_CHP0_L_DEF			(0x94)

#define	MCI_PDM_CHP1_H				(243)
#define	MCI_PDM_CHP1_H_DEF			(0xC0)
#define	MCI_PDM_CHP1_M				(244)
#define	MCI_PDM_CHP1_M_DEF			(0x15)
#define	MCI_PDM_CHP1_L				(245)
#define	MCI_PDM_CHP1_L_DEF			(0x6C)

#define	MCI_PDM_CHP2_H				(246)
#define	MCI_PDM_CHP2_H_DEF			(0x00)
#define	MCI_PDM_CHP2_M				(247)
#define	MCI_PDM_CHP2_M_DEF			(0x00)
#define	MCI_PDM_CHP2_L				(248)
#define	MCI_PDM_CHP2_L_DEF			(0x00)

#define	MCI_PDM_CHP3_H				(249)
#define	MCI_PDM_CHP3_H_DEF			(0x3F)
#define	MCI_PDM_CHP3_M				(250)
#define	MCI_PDM_CHP3_M_DEF			(0xD5)
#define	MCI_PDM_CHP3_L				(251)
#define	MCI_PDM_CHP3_L_DEF			(0x29)

#define	MCI_PDM_CHP4_H				(252)
#define	MCI_PDM_CHP4_H_DEF			(0x00)
#define	MCI_PDM_CHP4_M				(253)
#define	MCI_PDM_CHP4_M_DEF			(0x00)
#define	MCI_PDM_CHP4_L				(254)
#define	MCI_PDM_CHP4_L_DEF			(0x00)

/*	B_ADR(CDSP)	*/
#define	MCI_CDSP_SAVEOFF			(0)

#define	MCI_OFIFO_LVL				(1)

#define	MCI_EFIFO_LVL				(2)

#define	MCI_DEC_POS_24				(4)
#define	MCI_DEC_POS_16				(5)
#define	MCI_DEC_POS_8				(6)
#define	MCI_DEC_POS_0				(7)

#define	MCI_ENC_POS_24				(8)
#define	MCI_ENC_POS_16				(9)
#define	MCI_ENC_POS_8				(10)
#define	MCI_ENC_POS_0				(11)

#define	MCI_DEC_ERR					(12)
#define	MCI_ENC_ERR					(13)

#define	MCI_FIFO_RST				(14)

#define	MCI_DEC_ENC_START			(15)

#define	MCI_FIFO4CH					(16)

#define	MCI_DEC_CTL15				(19)

#define	MCI_DEC_GPR15				(35)

#define	MCI_DEC_SFR1				(51)
#define	MCI_DEC_SFR0				(52)

#define	MCI_ENC_CTL15				(53)

#define	MCI_ENC_GPR15				(69)

#define	MCI_ENC_SFR1				(85)
#define	MCI_ENC_SFR0				(86)

#define	MCI_JOEMP					(92)
#define	MCB_JOEMP					(0x80)
#define	MCB_JOPNT					(0x40)
#define	MCB_OOVF_FLG				(0x08)
#define	MCB_OUDF_FLG				(0x04)
#define	MCB_OEMP_FLG				(0x02)
#define	MCB_OPNT_FLG				(0x01)
#define	MCI_JOEMP_DEF				(MCB_JOEMP | MCB_OEMP_FLG)

#define	MCI_JEEMP					(93)
#define	MCB_JEEMP					(0x80)
#define	MCB_JEPNT					(0x40)
#define	MCB_EOVF_FLG				(0x08)
#define	MCB_EUDF_FLG				(0x04)
#define	MCB_EEMP_FLG				(0x02)
#define	MCB_EPNT_FLG				(0x01)
#define	MCI_JEEMP_DEF				(MCB_JEEMP | MCB_EEMP_FLG)

#define	MCI_DEC_FLG					(96)
#define	MCI_ENC_FLG					(97)

#define	MCI_DEC_GPR_FLG				(98)
#define	MCI_ENC_GPR_FLG				(99)

#define	MCI_EOPNT					(101)

#define	MCI_EDEC					(105)
#define	MCI_EENC					(106)

#define	MCI_EDEC_GPR				(107)
#define	MCI_EENC_GPR				(108)

#define	MCI_CDSP_SRST				(110)
#define	MCB_CDSP_FMODE				(0x10)
#define	MCB_CDSP_MSAVE				(0x08)
#define	MCB_CDSP_SRST				(0x01)
#define	MCI_CDSP_SRST_DEF			(MCB_CDSP_SRST)

#define	MCI_CDSP_SLEEP				(112)

#define	MCI_CDSP_ERR				(113)

#define	MCI_CDSP_MAR_MSB			(114)
#define	MCI_CDSP_MAR_LSB			(115)

#define	MCI_OFIFO_IRQ_PNT			(116)

#define	MCI_EFIFO_IRQ_PNT			(122)

#define	MCI_CDSP_FLG				(128)

#define	MCI_ECDSP_ERR				(129)

/*	B_ADR(CD)	*/
#define	MCI_DPADIF					(1)
#define	MCB_CLKSRC					(0x80)
#define	MCB_CLKBUSY					(0x40)
#define	MCB_CLKINPUT				(0x20)
#define	MCB_DPADIF					(0x10)
#define	MCB_DP0_CLKI1				(0x08)
#define	MCB_DP0_CLKI0				(0x01)
#define	MCI_DPADIF_DEF				(MCB_DPADIF|MCB_DP0_CLKI1|MCB_DP0_CLKI0)

#define	MCI_CKSEL					(4)
#define	MCB_CK1SEL					(0x80)
#define	MCB_CK0SEL					(0x40)

#define	MCI_CD_HW_ID				(8)
#define	MCI_CD_HW_ID_DEF			(0x79)

#define	MCI_PLL_RST					(15)
#define	MCB_PLLRST0					(0x01)
#define	MCI_PLL_RST_DEF				(MCB_PLLRST0)

#define	MCI_DIVR0					(16)
#define	MCI_DIVR0_DEF				(0x0D)

#define	MCI_DIVF0					(17)
#define	MCI_DIVF0_DEF				(0x55)

#define	MCI_DIVR1					(18)
#define	MCI_DIVR1_DEF				(0x02)

#define	MCI_DIVF1					(19)
#define	MCI_DIVF1_DEF				(0x48)

#define	MCI_AD_AGC					(70)
#define	MCI_AD_AGC_DEF				(0x03)

#define	MCI_AD_START				(72)
#define	MCI_AD_START_DEF			(0x00)
#define	MCB_AD_START				(0x01)

#define	MCI_DCCUTOFF				(77)
#define	MCI_DCCUTOFF_DEF			(0x00)

#define	MCI_DAC_CONFIG				(78)
#define	MCI_DAC_CONFIG_DEF			(0x02)
#define	MCB_NSMUTE					(0x02)
#define	MCB_DACON					(0x01)

#define	MCI_DCL						(79)
#define	MCI_DCL_DEF					(0x00)

#define	MCI_SYS_CEQ0_19_12			(80)
#define	MCI_SYS_CEQ0_19_12_DEF		(0x10)

#define	MCI_SYS_CEQ0_11_4			(81)
#define	MCI_SYS_CEQ0_11_4_DEF		(0xC4)

#define	MCI_SYS_CEQ0_3_0			(82)
#define	MCI_SYS_CEQ0_3_0_DEF		(0x50)

#define	MCI_SYS_CEQ1_19_12			(83)
#define	MCI_SYS_CEQ1_19_12_DEF		(0x12)

#define	MCI_SYS_CEQ1_11_4			(84)
#define	MCI_SYS_CEQ1_11_4_DEF		(0xC4)

#define	MCI_SYS_CEQ1_3_0			(85)
#define	MCI_SYS_CEQ1_3_0_DEF		(0x40)

#define	MCI_SYS_CEQ2_19_12			(86)
#define	MCI_SYS_CEQ2_19_12_DEF		(0x02)

#define	MCI_SYS_CEQ2_11_4			(87)
#define	MCI_SYS_CEQ2_11_4_DEF		(0xA9)

#define	MCI_SYS_CEQ2_3_0			(88)
#define	MCI_SYS_CEQ2_3_0_DEF		(0x60)

#define	MCI_SYS_CEQ3_19_12			(89)
#define	MCI_SYS_CEQ3_19_12_DEF		(0xED)

#define	MCI_SYS_CEQ3_11_4			(90)
#define	MCI_SYS_CEQ3_11_4_DEF		(0x3B)

#define	MCI_SYS_CEQ3_3_0			(91)
#define	MCI_SYS_CEQ3_3_0_DEF		(0xC0)

#define	MCI_SYS_CEQ4_19_12			(92)
#define	MCI_SYS_CEQ4_19_12_DEF		(0xFC)

#define	MCI_SYS_CEQ4_11_4			(93)
#define	MCI_SYS_CEQ4_11_4_DEF		(0x92)

#define	MCI_SYS_CEQ4_3_0			(94)
#define	MCI_SYS_CEQ4_3_0_DEF		(0x40)

#define	MCI_SYSTEM_EQON				(95)
#define	MCB_SYSEQ_INTP				(0x20)
#define	MCB_SYSEQ_FLAG				(0x10)
#define	MCB_SYSTEM_EQON				(0x01)
#define	MCI_SYSTEM_EQON_DEF			(MCB_SYSEQ_INTP|MCB_SYSTEM_EQON)

/*	B_ADR(ANA)	*/
#define	MCI_ANA_RST					(0)
#define	MCI_ANA_RST_DEF				(0x01)

#define	MCI_PWM_ANALOG_0			(2)
#define	MCB_PWM_VR					(0x01)
#define	MCB_PWM_CP					(0x02)
#define	MCB_PWM_REFA				(0x04)
#define	MCB_PWM_LDOA				(0x08)
#define	MCI_PWM_ANALOG_0_DEF		(MCB_PWM_VR|MCB_PWM_CP|MCB_PWM_REFA|MCB_PWM_LDOA)

#define	MCI_PWM_ANALOG_1			(3)
#define	MCB_PWM_SPL1				(0x01)
#define	MCB_PWM_SPL2				(0x02)
#define	MCB_PWM_SPR1				(0x04)
#define	MCB_PWM_SPR2				(0x08)
#define	MCB_PWM_HPL					(0x10)
#define	MCB_PWM_HPR					(0x20)
#define	MCB_PWM_ADL					(0x40)
#define	MCB_PWM_ADR					(0x80)
#define	MCI_PWM_ANALOG_1_DEF		(MCB_PWM_SPL1|MCB_PWM_SPL2|MCB_PWM_SPR1|MCB_PWM_SPR2|MCB_PWM_HPL|MCB_PWM_HPR|MCB_PWM_ADL|MCB_PWM_ADR)

#define	MCI_PWM_ANALOG_2			(4)
#define	MCB_PWM_LO1L				(0x01)
#define	MCB_PWM_LO1R				(0x02)
#define	MCB_PWM_LO2L				(0x04)
#define	MCB_PWM_LO2R				(0x08)
#define	MCB_PWM_RC1					(0x10)
#define	MCB_PWM_RC2					(0x20)
#define	MCI_PWM_ANALOG_2_DEF		(MCB_PWM_LO1L|MCB_PWM_LO1R|MCB_PWM_LO2L|MCB_PWM_LO2R|MCB_PWM_RC1|MCB_PWM_RC2)

#define	MCI_PWM_ANALOG_3			(5)
#define	MCB_PWM_MB1					(0x01)
#define	MCB_PWM_MB2					(0x02)
#define	MCB_PWM_MB3					(0x04)
#define	MCB_PWM_DAL					(0x08)
#define	MCB_PWM_DAR					(0x10)
#define	MCI_PWM_ANALOG_3_DEF		(MCB_PWM_MB1|MCB_PWM_MB2|MCB_PWM_MB3|MCB_PWM_DAL|MCB_PWM_DAR)

#define	MCI_PWM_ANALOG_4			(6)
#define	MCB_PWM_MC1					(0x10)
#define	MCB_PWM_MC2					(0x20)
#define	MCB_PWM_MC3					(0x40)
#define	MCB_PWM_LI					(0x80)
#define	MCI_PWM_ANALOG_4_DEF		(MCB_PWM_MC1|MCB_PWM_MC2|MCB_PWM_MC3|MCB_PWM_LI)

#define	MCI_BUSY1					(12)
#define	MCB_RC_BUSY					(0x20)
#define	MCB_HPL_BUSY				(0x10)
#define	MCB_SPL_BUSY				(0x08)

#define	MCI_BUSY2					(13)
#define	MCB_HPR_BUSY				(0x10)
#define	MCB_SPR_BUSY				(0x08)

#define	MCI_APMOFF					(16)
#define	MCB_APMOFF_SP				(0x01)
#define	MCB_APMOFF_HP				(0x02)
#define	MCB_APMOFF_RC				(0x04)

#define	MCI_DIF_LINE				(24)
#define	MCI_DIF_LINE_DEF			(0x00)

#define	MCI_LI1VOL_L				(25)
#define	MCI_LI1VOL_L_DEF			(0x00)
#define	MCB_ALAT_LI1				(0x40)
#define	MCB_LI1VOL_L				(0x1F)

#define	MCI_LI1VOL_R				(26)
#define	MCI_LI1VOL_R_DEF			(0x00)
#define	MCB_LI1VOL_R				(0x1F)

#define	MCI_LI2VOL_L				(27)
#define	MCI_LI2VOL_L_DEF			(0x00)
#define	MCB_ALAT_LI2				(0x40)
#define	MCB_LI2VOL_L				(0x1F)

#define	MCI_LI2VOL_R				(28)
#define	MCI_LI2VOL_R_DEF			(0x00)
#define	MCB_LI2VOL_R				(0x1F)

#define	MCI_MC1VOL					(29)
#define	MCI_MC1VOL_DEF				(0x00)
#define	MCB_MC1VOL					(0x1F)

#define	MCI_MC2VOL					(30)
#define	MCI_MC2VOL_DEF				(0x00)
#define	MCB_MC2VOL					(0x1F)

#define	MCI_MC3VOL					(31)
#define	MCI_MC3VOL_DEF				(0x00)
#define	MCB_MC3VOL					(0x1F)

#define	MCI_ADVOL_L					(32)
#define	MCI_ADVOL_L_DEF				(0x00)
#define	MCB_ALAT_AD					(0x40)
#define	MCB_ADVOL_L					(0x1F)

#define	MCI_ADVOL_R					(33)
#define	MCB_ADVOL_R					(0x1F)

#define	MCI_HPVOL_L					(35)
#define	MCB_ALAT_HP					(0x40)
#define	MCB_SVOL_HP					(0x20)
#define	MCB_HPVOL_L					(0x1F)
#define	MCI_HPVOL_L_DEF				(MCB_SVOL_HP)

#define	MCI_HPVOL_R					(36)
#define	MCI_HPVOL_R_DEF				(0x00)
#define	MCB_HPVOL_R					(0x1F)

#define	MCI_SPVOL_L					(37)
#define	MCB_ALAT_SP					(0x40)
#define	MCB_SVOL_SP					(0x20)
#define	MCB_SPVOL_L					(0x1F)
#define	MCI_SPVOL_L_DEF				(MCB_SVOL_SP)

#define	MCI_SPVOL_R					(38)
#define	MCI_SPVOL_R_DEF				(0x00)
#define	MCB_SPVOL_R					(0x1F)

#define	MCI_RCVOL					(39)
#define	MCB_SVOL_RC					(0x20)
#define	MCB_RCVOL					(0x1F)
#define	MCI_RCVOL_DEF				(MCB_SVOL_RC)

#define	MCI_LO1VOL_L				(40)
#define	MCI_LO1VOL_L_DEF			(0x20)
#define	MCB_ALAT_LO1				(0x40)
#define	MCB_LO1VOL_L				(0x1F)

#define	MCI_LO1VOL_R				(41)
#define	MCI_LO1VOL_R_DEF			(0x00)
#define	MCB_LO1VOL_R				(0x1F)

#define	MCI_LO2VOL_L				(42)
#define	MCI_LO2VOL_L_DEF			(0x20)
#define	MCB_ALAT_LO2				(0x40)
#define	MCB_LO2VOL_L				(0x1F)

#define	MCI_LO2VOL_R				(43)
#define	MCI_LO2VOL_R_DEF			(0x00)
#define	MCB_LO2VOL_R				(0x1F)

#define	MCI_SP_MODE					(44)
#define	MCB_SPR_HIZ					(0x20)
#define	MCB_SPL_HIZ					(0x10)
#define	MCB_SPMN					(0x02)
#define	MCB_SP_SWAP					(0x01)

#define	MCI_MC_GAIN					(45)
#define	MCI_MC_GAIN_DEF				(0x00)
#define	MCB_MC2SNG					(0x40)
#define	MCB_MC2GAIN					(0x30)
#define	MCB_MC1SNG					(0x04)
#define	MCB_MC1GAIN					(0x03)

#define	MCI_MC3_GAIN				(46)
#define	MCI_MC3_GAIN_DEF			(0x00)
#define	MCB_MC3SNG					(0x04)
#define	MCB_MC3GAIN					(0x03)

#define	MCI_RDY_FLAG				(47)
#define	MCB_LDO_RDY					(0x80)
#define	MCB_VREF_RDY				(0x40)
#define	MCB_SPRDY_R					(0x20)
#define	MCB_SPRDY_L					(0x10)
#define	MCB_HPRDY_R					(0x08)
#define	MCB_HPRDY_L					(0x04)
#define	MCB_CPPDRDY					(0x02)

/*	analog mixer common */
#define	MCB_LI1MIX					(0x01)
#define	MCB_M1MIX					(0x08)
#define	MCB_M2MIX					(0x10)
#define	MCB_M3MIX					(0x20)
#define	MCB_DAMIX					(0x40)
#define	MCB_DARMIX					(0x40)
#define	MCB_DALMIX					(0x80)

#define	MCB_MONO_DA					(0x40)
#define	MCB_MONO_LI1				(0x01)

#define	MCI_ADL_MIX					(50)
#define	MCI_ADL_MONO				(51)
#define	MCI_ADR_MIX					(52)
#define	MCI_ADR_MONO				(53)

#define	MCI_LO1L_MIX				(55)
#define	MCI_LO1L_MONO				(56)
#define	MCI_LO1R_MIX				(57)

#define	MCI_LO2L_MIX				(58)
#define	MCI_LO2L_MONO				(59)
#define	MCI_LO2R_MIX				(60)

#define	MCI_HPL_MIX					(61)
#define	MCI_HPL_MONO				(62)
#define	MCI_HPR_MIX					(63)

#define	MCI_SPL_MIX					(64)
#define	MCI_SPL_MONO				(65)
#define	MCI_SPR_MIX					(66)
#define	MCI_SPR_MONO				(67)

#define	MCI_RC_MIX					(69)

#define	MCI_CPMOD					(72)

#define	MCI_HP_GAIN					(77)

#define	MCI_LEV						(79)
#define	MCB_AVDDLEV					(0x07)
#define	MCI_LEV_DEF					(0x24)

#define	MCI_DNGATRT_HP				(82)
#define	MCI_DNGATRT_HP_DEF			(0x23)

#define	MCI_DNGTARGET_HP			(83)
#define	MCI_DNGTARGET_HP_DEF		(0x50)

#define	MCI_DNGON_HP				(84)
#define	MCI_DNGON_HP_DEF			(0x54)

#define	MCI_DNGATRT_SP				(85)
#define	MCI_DNGATRT_SP_DEF			(0x23)

#define	MCI_DNGTARGET_SP			(86)
#define	MCI_DNGTARGET_SP_DEF		(0x50)

#define	MCI_DNGON_SP				(87)
#define	MCI_DNGON_SP_DEF			(0x54)

#define	MCI_DNGATRT_RC				(88)
#define	MCI_DNGATRT_RC_DEF			(0x23)

#define	MCI_DNGTARGET_RC			(89)
#define	MCI_DNGTARGET_RC_DEF		(0x50)

#define	MCI_DNGON_RC				(90)
#define	MCI_DNGON_RC_DEF			(0x54)

#define	MCI_AP_A1					(123)
#define	MCB_AP_CP_A					(0x10)
#define	MCB_AP_HPL_A				(0x02)
#define	MCB_AP_HPR_A				(0x01)

#define	MCI_AP_A2					(124)
#define	MCB_AP_RC1_A				(0x20)
#define	MCB_AP_RC2_A				(0x10)
#define	MCB_AP_SPL1_A				(0x08)
#define	MCB_AP_SPR1_A				(0x04)
#define	MCB_AP_SPL2_A				(0x02)
#define	MCB_AP_SPR2_A				(0x01)

#endif /* __MCDEFS_H__ */
