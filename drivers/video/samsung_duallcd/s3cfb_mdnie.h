
/* linux/drivers/video/samsung/s3cfb_mdnie.h
 *
 * Header file for Samsung (MDNIE) driver
 *
 * Copyright (c) 2009 Samsung Electronics
 *	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S3CFB_MDNIE_H
#define _S3CFB_MDNIE_H

#define S3C_MDNIE_PHY_BASE                              0x11CA0000
#define S3C_MDNIE_MAP_SIZE                              0x00001000

#define S3C_MDNIE_rR34                                  0x0088
#define S3C_MDNIE_rR35                                  0x008C

#define S3C_MDNIE_rR0                                   0x0000
#define S3C_MDNIE_rR1                                   0x0004
#define S3C_MDNIE_rR3                                   0x000C
#define S3C_MDNIE_rR4                                   0x0010
#define S3C_MDNIE_rR5                                   0x0014
#define S3C_MDNIE_rR6                                   0x0018
#define S3C_MDNIE_rR8                                   0x0020
#define S3C_MDNIE_rR9                                   0x0024
#define S3C_MDNIE_rR28                                  0x00A0

#define S3C_MDNIE_rR36                                  0x0090
#define S3C_MDNIE_rR37                                  0x0094
#define S3C_MDNIE_rR38                                  0x0098
#define S3C_MDNIE_rR39                                  0x009C
#define S3C_MDNIE_rR40                                  0x00A0
#define S3C_MDNIE_rR41                                  0x00A4
#define S3C_MDNIE_rR42                                  0x00A8
#define S3C_MDNIE_rR43                                  0x00AC
#define S3C_MDNIE_rR44                                  0x00B0
#define S3C_MDNIE_rR45                                  0x00B4
#define S3C_MDNIE_rR46                                  0x00B8

#define S3C_MDNIE_rR48                                  0x00C0

#define S3C_MDNIE_rR49                                  0x00C4
#define S3C_MDNIE_rR50                                  0x00C8
#define S3C_MDNIE_rR51                                  0x00CC
#define S3C_MDNIE_rR52					0x00D0
#define S3C_MDNIE_rR53					0x00D4
#define S3C_MDNIE_rR54					0x00D8
#define S3C_MDNIE_rR55					0x00DC
#define S3C_MDNIE_rR56					0x00E0


#define S3C_MDNIE_rR64					0x0100
#define S3C_MDNIE_rR65					0x0104
#define S3C_MDNIE_rR66					0x0108
#define S3C_MDNIE_rR67					0x010C
#define S3C_MDNIE_rR68					0x0110
#define S3C_MDNIE_rR69					0x0114


#define S3C_MDNIE_rR72					0x0120
#define S3C_MDNIE_rR73					0x0124
#define S3C_MDNIE_rR74					0x0128
#define S3C_MDNIE_rR75					0x012C
#define S3C_MDNIE_rR76					0x0130
#define S3C_MDNIE_rR77					0x0134
#define S3C_MDNIE_rR78					0x0138
#define S3C_MDNIE_rR79					0x013C
#define S3C_MDNIE_rR80					0x0140
#define S3C_MDNIE_rR81					0x0144
#define S3C_MDNIE_rR82					0x0148
#define S3C_MDNIE_rR83					0x014C
#define S3C_MDNIE_rR84					0x0150
#define S3C_MDNIE_rR85					0x0154
#define S3C_MDNIE_rR86					0x0158
#define S3C_MDNIE_rR87					0x015C


#define S3C_MDNIE_rR88					0x0160
#define S3C_MDNIE_rR89					0x0164
#define S3C_MDNIE_rR90					0x0168
#define S3C_MDNIE_rR91					0x016C
#define S3C_MDNIE_rR92					0x0170
#define S3C_MDNIE_rR93					0x0174
#define S3C_MDNIE_rR94					0x0178
#define S3C_MDNIE_rR95					0x017C
#define S3C_MDNIE_rR96					0x0180
#define S3C_MDNIE_rR97					0x0184
#define S3C_MDNIE_rR98					0x0188
#define S3C_MDNIE_rR99					0x018C
#define S3C_MDNIE_rR100					0x0190
#define S3C_MDNIE_rR101					0x0194

#define S3C_MDNIE_rR102					0x0198

#define S3C_MDNIE_rR103					0x019C

/*R1*/
#define S3C_MDNIE_HDTR					(0<<6)
#define S3C_MDNIE_HDTR_CP				(1<<6)	/* Color Preference */
#define S3C_MDNIE_HDTR_VE				(2<<6)	/* Visibility Enhancement */
#define S3C_MDNIE_MCM_OFF				(0<<5)
#define S3C_MDNIE_MCM_ON				(1<<5)
#define S3C_MDNIE_LPA_OFF				(0<<4)
#define S3C_MDNIE_LPA_ON				(1<<4)
#define	S3C_MDNIE_ALGORITHM_MASK			(0xF<<4)
#define S3C_MDNIE_REG_UNMASK				(0<<0)
#define S3C_MDNIE_REG_MASK				(1<<0)
/*R3*/
#define S3C_MDNIE_HSIZE(n)				(((n)&0x7FF)<<0)
/*R4*/
#define S3C_MDNIE_VSIZE(n)				(((n)&0x7FF)<<0)
#define	S3C_MDNIE_SIZE_MASK				(0x7FF<<0)

/*R36*/
#define S3C_MDNIE_DECONT_TH(n)				(((n)&0xFFF)<<0)
/*R37*/
#define S3C_MDNIE_DIRECT_TH(n)				(((n)&0xFFF)<<0)
/*R38*/
#define S3C_MDNIE_SIMPLE_TH(n)				(((n)&0x7F)<<0)
/*R39*/
#define S3C_MDNIE_DE_CONT(n)				(((n)&0x7F)<<5)
#define S3C_MDNIE_CE_ON					(0<<4)
#define S3C_MDNIE_CE_OFF				(1<<4)
#define S3C_MDNIE_CE_CURVE(n)				(((n)&0x7)<<1)
#define S3C_MDNIE_AMOLED_NOTSELECT			(0<<0)
#define S3C_MDNIE_AMOLED_SELECT				(1<<0)
/*R40*/
#define S3C_MDNIE_IPF_ALPHA(n)				(((n)&0xFF)<<7)
#define S3C_MDNIE_IPF_BETA(n)				(((n)&0xFF)<<0)
/*R41*/
#define S3C_MDNIE_IPF_THETA(n)				(((n)&0xFF)<<0)
/*R42*/
#define S3C_MDNIE_IPF_CNTRST(n)				(((n)&0x3)<<12)
#define S3C_MDNIE_IPF_G(n)				(((n)&0xF)<<8)
#define S3C_MDNIE_IPF_T(n)				(((n)&0xFF)<<0)

/*R43*/
#define S3C_MDNIE_CS_SKIN_OFF				(0<<10)
#define S3C_MDNIE_CS_SKIN_ON				(1<<10)
#define S3C_MDNIE_CS_CONT(n)				(((n)&0x3FF)<<0)

/*R45*/
#define S3C_MDNIE_DE_TH(n)				(((n)&0x3FF)<<0)

/*R48*/
#define S3C_MDNIE_SN_LVL_0				(0<<10)
#define S3C_MDNIE_SN_LVL_1				(1<<10)
#define S3C_MDNIE_SN_LVL_2				(2<<10)
#define S3C_MDNIE_SN_LVL_3				(3<<10)
#define S3C_MDNIE_SY_LVL_0				(0<<8)
#define S3C_MDNIE_SY_LVL_1				(1<<8)
#define S3C_MDNIE_SY_LVL_2				(2<<8)
#define S3C_MDNIE_SY_LVL_3				(3<<8)
#define S3C_MDNIE_GR_LVL_0				(0<<6)
#define S3C_MDNIE_GR_LVL_1				(1<<6)
#define S3C_MDNIE_GR_LVL_2				(2<<6)
#define S3C_MDNIE_GR_LVL_3				(3<<6)
#define S3C_MDNIE_RD_LVL_0				(0<<4)
#define S3C_MDNIE_RD_LVL_1				(1<<4)
#define S3C_MDNIE_RD_LVL_2				(2<<4)
#define S3C_MDNIE_RD_LVL_3				(3<<4)
#define S3C_MDNIE_YE_LVL_0				(0<<2)
#define S3C_MDNIE_YE_LVL_1				(1<<2)
#define S3C_MDNIE_YE_LVL_2				(2<<2)
#define S3C_MDNIE_YE_LVL_3				(3<<2)
#define S3C_MDNIE_PU_LVL_0				(0<<0)
#define S3C_MDNIE_PU_LVL_1				(1<<0)
#define S3C_MDNIE_PU_LVL_2				(2<<0)
#define S3C_MDNIE_PU_LVL_3				(3<<0)

/*R49*/
#define S3C_MDNIE_SN_L1_TCB(n)				(((n)&0xFF)<<8)
#define S3C_MDNIE_SN_L2_TCB(n)				(((n)&0xFF)<<0)
/*R50*/
#define S3C_MDNIE_SN_L3_TCB(n)				(((n)&0xFF)<<8)
#define S3C_MDNIE_SN_L1_TCR(n)				(((n)&0xFF)<<0)
/*R51*/
#define S3C_MDNIE_SN_L2_TCR(n)				(((n)&0xFF)<<8)
#define S3C_MDNIE_SN_L3_TCR(n)				(((n)&0xFF)<<0)
/*R52*/
#define S3C_MDNIE_SN_L1_S(n)				(((n)&0x7)<<6)
#define S3C_MDNIE_SN_L2_S(n)				(((n)&0x7)<<3)
#define S3C_MDNIE_SN_L3_S(n)				(((n)&0x7)<<0)
/*R53*/
#define S3C_MDNIE_RD_L1_TCB(n)				(((n)&0xFF)<<8)
#define S3C_MDNIE_RD_L2_TCB(n)				(((n)&0xFF)<<0)
/*R54*/
#define S3C_MDNIE_RD_L3_TCB(n)				(((n)&0xFF)<<8)
#define S3C_MDNIE_RD_L1_TCR(n)				(((n)&0xFF)<<0)
/*R55*/
#define S3C_MDNIE_RD_L2_TCR(n)				(((n)&0xFF)<<8)
#define S3C_MDNIE_RD_L3_TCR(n)				(((n)&0xFF)<<0)
/*R56*/
#define S3C_MDNIE_RD_L1_S(n)				(((n)&0x7)<<6)
#define S3C_MDNIE_RD_L2_S(n)				(((n)&0x7)<<3)
#define S3C_MDNIE_RD_L3_S(n)				(((n)&0x7)<<0)

/*R64*/
#define S3C_MDNIE_LIGHT_P(n)				(((n)&0xFF)<<8)
#define S3C_MDNIE_CHROMA_P(n)				(((n)&0xFF)<<0)
/*R65*/
#define S3C_MDNIE_TRANSFUNL_1(n)			(((n)&0x7F)<<8)
#define S3C_MDNIE_TRANSFUNL_2(n)			(((n)&0x7F)<<0)
/*R66*/
#define S3C_MDNIE_TRANSFUNL_3(n)			(((n)&0x7F)<<8)
#define S3C_MDNIE_TRANSFUNL_4(n)			(((n)&0x7F)<<0)
/*R67*/
#define S3C_MDNIE_TRANSFUNL_5(n)			(((n)&0x7F)<<8)
#define S3C_MDNIE_TRANSFUNL_6(n)			(((n)&0x7F)<<0)
/*R68*/
#define S3C_MDNIE_TRANSFUNL_7(n)			(((n)&0x7F)<<8)
#define S3C_MDNIE_TRANSFUNL_8(n)			(((n)&0x7F)<<0)
/*R69*/
#define S3C_MDNIE_TRANSFUNL_9(n)			(((n)&0x7F)<<8)

/*R72*/
#define S3C_MDNIE_QUADRANT_OFF				(0<<8)
#define S3C_MDNIE_QUADRAND_ON				(1<<8)
#define S3C_MDNIE_COLOR_TEMP_DEST(n)			((((n)&0xFF)<<0)
/*R73*/
#define S3C_MDNIE_COLOR_TEMP1(n)			(((n)&0xFF)<<8)
#define S3C_MDNIE_COLOR_TEMP2(n)			(((n)&0xFF)<<0)
/*R74*/
#define S3C_MDNIE_COLOR_TEMP3(n)			(((n)&0xFF)<<8)
#define S3C_MDNIE_COLOR_TEMP4(n)			(((n)&0xFF)<<0)
/*R75*/
#define S3C_MDNIE_COLOR_TEMP5(n)			(((n)&0xFF)<<8)
#define S3C_MDNIE_LSF1(n)				(((n)&0xFF)<<0)
/*R76*/
#define S3C_MDNIE_LSF2(n)				(((n)&0xFF)<<8)
#define S3C_MDNIE_LSF3(n)				(((n)&0xFF)<<0)
/*R76*/
#define S3C_MDNIE_LSF4(n)				(((n)&0xFF)<<8)
#define S3C_MDNIE_LSF5(n)				(((n)&0xFF)<<0)
/*R78*/
#define S3C_MDNIE_CT1_HIGH_CB(n)			(((n)&0xFF)<<8)
#define S3C_MDNIE_CT2_HIGH_CB(n)			(((n)&0xFF)<<0)
/*R79*/
#define S3C_MDNIE_CT3_HIGH_CB(n)			(((n)&0xFF)<<8)
#define S3C_MDNIE_CT4_HIGH_CB(n)			(((n)&0xFF)<<0)
/*R80*/
#define S3C_MDNIE_CT5_HIGH_CB(n)			(((n)&0xFF)<<8)
#define S3C_MDNIE_CT1_HIGH_CR(n)			(((n)&0xFF)<<0)
/*R81*/
#define S3C_MDNIE_CT2_HIGH_CR(n)			(((n)&0xFF)<<8)
#define S3C_MDNIE_CT3_HIGH_CR(n)			(((n)&0xFF)<<0)
/*R82*/
#define S3C_MDNIE_CT4_HIGH_CR(n)			(((n)&0xFF)<<8)
#define S3C_MDNIE_CT5_HIGH_CR(n)			(((n)&0xFF)<<0)
/*R83*/
#define S3C_MDNIE_GRAY_AXIS_LONG(n)			(((n)&0xFF)<<8)
#define S3C_MDNIE_GRAY_AXIS_SHORT(n)			(((n)&0xFF)<<0)
/*R84*/
#define S3C_MDNIE_GRAY_ANGLE_COS(n)			(((n)&0x3ff)<<0)
/*R85*/
#define S3C_MDNIE_GRAY_ANGLE_SIN(n)			(((n)&0x3ff)<<0)
/*R86*/
#define S3C_MDNIE_QUADRANT_TMP1(n)			(((n)&0xFF)<<8)
#define S3C_MDNIE_QUADRANT_TMP2(n)			(((n)&0xFF)<<0)
/*R87*/
#define S3C_MDNIE_QUADRANT_TMP3(n)			(((n)&0xFF)<<8)
#define S3C_MDNIE_QUADRANT_TMP4(n)			(((n)&0xFF)<<0)

/*R88*/
#define S3C_MDNIE_FGain_LUT0(n)				(((n)&0x7F)<<8)
#define S3C_MDNIE_FGain_LUT1(n)				(((n)&0x7F)<<0)
/*R89*/
#define S3C_MDNIE_FGain_LUT2(n)				(((n)&0x7F)<<8)
#define S3C_MDNIE_FGain_LUT3(n)				(((n)&0x7F)<<0)
/*R90*/
#define S3C_MDNIE_FGain_LUT4(n)				(((n)&0x7F)<<8)
#define S3C_MDNIE_FGain_LUT5(n)				(((n)&0x7F)<<0)
/*R91*/
#define S3C_MDNIE_FGain_LUT6(n)				(((n)&0x7F)<<8)
#define S3C_MDNIE_FGain_LUT7(n)				(((n)&0x7F)<<0)
/*R92*/
#define S3C_MDNIE_DGain_LUT0(n)				(((n)&0x7F)<<8)
#define S3C_MDNIE_DGain_LUT1(n)				(((n)&0x7F)<<0)
/*R93*/
#define S3C_MDNIE_DGain_LUT2(n)				(((n)&0x7F)<<8)
#define S3C_MDNIE_DGain_LUT3(n)				(((n)&0x7F)<<0)
/*R94*/
#define S3C_MDNIE_DGain_LUT4(n)				(((n)&0x7F)<<8)
#define S3C_MDNIE_DGain_LUT5(n)				(((n)&0x7F)<<0)
/*R95*/
#define S3C_MDNIE_DGain_LUT6(n)				(((n)&0x7F)<<8)
#define S3C_MDNIE_DGain_LUT7(n)				(((n)&0x7F)<<0)
/*R96*/
#define S3C_MDNIE_Power_LUT0(n)				(((n)&0x7F)<<8)
#define S3C_MDNIE_Power_LUT1(n)				(((n)&0x7F)<<0)
/*R97*/
#define S3C_MDNIE_Power_LUT2(n)				(((n)&0x7F)<<8)
#define S3C_MDNIE_Power_LUT3(n)				(((n)&0x7F)<<0)
/*R98*/
#define S3C_MDNIE_Power_LUT4(n)				(((n)&0x7F)<<8)
#define S3C_MDNIE_Power_LUT5(n)				(((n)&0x7F)<<0)
/*R99*/
#define S3C_MDNIE_Power_LUT6(n)				(((n)&0x7F)<<8)
#define S3C_MDNIE_Power_LUT7(n)				(((n)&0x7F)<<0)
/*R100*/
#define S3C_MDNIE_Power_LUT8(n)				(((n)&0x7F)<<8)

/*R101*/
#define S3C_MDNIE_ZONAL_PARA0(n)			(((n)&0x3)<<4)
#define S3C_MDNIE_ZONAL_PARA1(n)			(((n)&0x3)<<0)
/*R102*/
#define S3C_MDNIE_BLU_DISABLE				(0<<1)
#define S3C_MDNIE_BLU_ENABLE				(1<<1)
#define S3C_MDNIE_DISPLAY_LCD				(0<<0)
#define S3C_MDNIE_DISPLAY_OLED				(1<<0)

/*R103*/
#define S3C_MDNIE_PWM_PHIGH				(1<<15)
#define S3C_MDNIE_PWM_PLOW				(0<<15)
#define S3C_MDNIE_PWM_CT_LPA				(0<<14) /* using LPA counter */
#define S3C_MDNIE_PWM_CT_PWM				(1<<14) /* using PWM counter */
#define S3C_MDNIE_PWM_CT(n)				(((n)&0x3F)<<0)

#define TRUE				1
#define FALSE				0

int s3c_mdnie_setup(void);
int s3c_mdnie_init_global(struct s3cfb_global *s3cfb_ctrl);
int s3c_mdnie_display_on(struct s3cfb_global *ctrl);
int s3c_mdnie_display_off(void);
int s3c_mdnie_off(void);

int mdnie_write(unsigned int addr, unsigned int val);
int s3c_mdnie_mask(void);
int s3c_mdnie_unmask(void);

#endif
