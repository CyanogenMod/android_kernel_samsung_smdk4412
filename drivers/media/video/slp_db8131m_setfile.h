/*
 * linux/drivers/media/video/slp_db8131m_setfile.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __DB8131M_SETFILE_H
#define __DB8131M_SETFILE_H

#include <linux/types.h>

/* 1.3M mipi setting-common from PARTRON */
/*******************************************************************/
/* Name		: DB8131M Initial Setfile*/
/* PLL mode	: MCLK=24MHz / PCLK=48MHz*/
/* FPS		: Preview 7.5~15fps / Capture 7.5fps / recording 25fps*/
/* Date		: 2011.12.06*/
/*******************************************************************/


static const u8 db8131m_common_1[] = {
/***************************************************/
/* Command Preview 7.5~15fps*/
/***************************************************/
0xFF, 0xC0,  /*Page mode*/
0x10, 0x01,
};
/*Wait  150*/

static const u8 db8131m_common_2[] = {
/***************************************************/
/* Format*/
/***************************************************/
0xFF, 0xA1, /*Page mode*/
0x70, 0x01,
0x71, 0x0D,

/***************************************************/
/* SensorCon*/
/***************************************************/
0xFF, 0xD0, /*Page mode*/
0x0F, 0x0B,	/*ABLK_Ctrl_1_Addr*/
0x13, 0x00,	/*Gain_Addr*/
0x15, 0x01,	/*IVREFT_REFB_Addr*/
0x20, 0x0E,	/*ABLK_Rsrv_Addr*/
0x23, 0x01,	/*IVREFT2_REFB2_Addr*/
0x24, 0x01,	/*IPPG_IVCM2_Addr*/
0x39, 0x70,	/*RiseSx_CDS_1_L_Addr*/
0x51, 0x19,	/*Fallcreset_1_L_Addr*/
0x83, 0x2D,	/*RiseTran_Sig_Even_L_Addr*/
0x85, 0x2F,	/*FallTran_Sig_Even_L_Addr*/
0x87, 0x2D,	/*RiseTran_Sig_Odd_L_Addr*/
0x89, 0x2F,	/*FallTran_Sig_Odd_L_Addr*/
0x8B, 0x27,	/*RiseCNT_EN_1_L_Addr*/
0x8D, 0x6c,	/*FallCNT_EN_1_L_Addr*/
0xD7, 0x80,	/*ABLK_Ctrl_12_Addr*/
0xDB, 0xA2,	/*FallScanTx15_1_L_Addr*/
0xED, 0x01,	/*PLL_P_Addr*/
0xEE, 0x0F,	/*PLL_M_Addr*/
0xEF, 0x00,	/*PLL_S_Addr*/
0xF9, 0x00,	/*ABLK_Ctrl_8*/
0xF8, 0x00, /*Vblank Sleep Mode enable*/
0xFB, 0x90,	/*PostADC_Gain_Addr*/

/***************************************************/
/* Analog ADF*/
/***************************************************/
0xFF, 0x85, /*Page mode*/
0x89, 0x93,	/*gAdf_u8APThreshold*/
0x8A, 0x0C, /*u8APClmpThreshold*/
0x8C, 0x07,	/*gAdf_u8APMinVal_ThrClampH*/
0x8D, 0x40,	/*gAdf_u8APMinVal_ThrClampL*/
0x8E, 0x03,	/*gAdf_u8APMinVal_DOFFSET*/
0x8F, 0x14,	/*gAdf_u8APMinVal_AMP2_1_SDM*/
0x91, 0x1A,	/*gAdf_u8APMinVal_AMP4_3_SDM*/
0x92, 0x0F,	/*gAdf_u8APMinVal_FallIntTx15*/
0x93, 0x47,	/*gAdf_u8APMinVal_CDSxRange_CtrlPre*/
0x95, 0x18,	/*gAdf_u8APMinVal_REFB_IVCM*/
0x96, 0x38,	/*gAdf_u8APMinVal_ref_os_PB*/
0x97, 0x0D,	/*gAdf_u8APMinVal_NTx_Range*/
0x98, 0x0D,	/*gAdf_u8APMaxVal_Clmp_rst*/
0x99, 0x06,	/*gAdf_u8APMaxVal_ThrClampH*/
0x9A, 0x9F,	/*gAdf_u8APMaxVal_ThrClampL*/
0x9B, 0x02,	/*gAdf_u8APMaxVal_DOFFSET*/
0x9C, 0x1C,	/*gAdf_u8APMaxVal_AMP2_1_SDM*/
0x9E, 0x11,	/*gAdf_u8APMaxVal_AMP4_3_SDM*/
0x9F, 0x5D,	/*gAdf_u8APMaxVal_FallIntTx15*/
0xA0, 0x78,	/*gAdf_u8APMaxVal_CDSxRange_CtrlPre*/
0xA2, 0x18,	/*gAdf_u8APMaxVal_REFB_IVCM*/
0xA3, 0x40,	/*gAdf_u8APMaxVal_ref_os_PB*/
0xA4, 0x0B,	/*gAdf_u8APMaxVal_NTx_Range*/

0xFF, 0x86, /*Page mode*/
0x15, 0x00,	/*gPT_u8Adf_APThrHys*/
0x16, 0xF7,	/*gPT_u8Adf_APFallIntTxThrLevel*/
0x17, 0x13,	/*gPT_u8Adf_APMinVal_BP2_1_SDM*/
0x18, 0x13,	/*gPT_u8Adf_APMidVal_BP2_1_SDM*/
0x19, 0x1C,	/*gPT_u8Adf_APMaxVal_BP2_1_SDM*/
0x1A, 0x06,	/*gPT_u8Adf_APMidVal_ThrClampH*/
0x1B, 0xF0,	/*gPT_u8Adf_APMidVal_ThrClampL*/
0x1C, 0x01,	/*gPT_u8Adf_APMidVal_DOFFSET*/
0x1D, 0x14,	/*gPT_u8Adf_APMidVal_AMP2_1_SDM*/
0x1F, 0x31,	/*gPT_u8Adf_APMidVal_AMP4_3_SDM*/
0x20, 0x68,	/*gPT_u8Adf_APMidVal_CDSxRange_CtrlPre*/
0x22, 0x18,	/*gPT_u8Adf_APMidVal_REFB_IVCM*/
0x23, 0x38,	/*gPT_u8Adf_APMidVal_ref_os_PB*/
0x24, 0x0F,	/*gPT_u8Adf_APMidVal_NTx_Range*/
0x25, 0x77,	/*gPT_u8Adf_APVal_EnSiSoSht_EnSm*/

0xFF, 0x87,	 /*Page mode*/
0xEA, 0x41,

0xFF, 0xD0, /*Page mode*/
0x20, 0x0D,	/*ABLK_Rsrv_Addr*/

0xFF, 0x83, /*Page mode*/
0x63, 0x28,    /*Again Table*/
0x64, 0x10,    /*Again Table*/
0x65, 0xA8,    /*Again Table*/
0x66, 0x50,    /*Again Table*/
0x67, 0x28,    /*Again Table*/
0x68, 0x14,    /*Again Table*/


/***************************************************/
/* AE*/
/***************************************************/
0xFF, 0x82, /*Page mode*/
0x95, 0x88,     /* AE weight*/
0x96, 0x88,
0x97, 0xF8,
0x98, 0x8F,
0x99, 0xF8,
0x9A, 0x8F,
0x9B, 0x88,
0x9C, 0x88,
0xA9, 0x40,	/* OTarget*/
0xAA, 0x40,	/* ITarget*/
0x9D, 0x66,	/* AE Speed*/
0x9F, 0x06,	/* AE HoldBnd*/
0xA8, 0x40,	/* STarget*/
0xB9, 0x04,	/* RGain*/
0xBB, 0x04,	/* GGain*/
0xBD, 0x04,	/* BGain*/
0xC5, 0x02,	/* PeakMvStep*/
0xC6, 0x38,	/* PeakTgMin*/
0xC7, 0x24,	/* PeakRatioTh1*/
0xC8, 0x10,	/* PeakRatioTh0*/
0xC9, 0x05,	/* PeakLuTh*/
0xD5, 0x60,	/* LuxGainTB_2*/
0xFF, 0x83, /*Page mode  */
0x2F, 0x04,     /* TimeNum0*/
0x30, 0x05,     /* TimeNum1*/
0x4F, 0x05,     /* FrameOffset*/
0xFF, 0x82, /*Page mode  */
0xA1, 0x0A,     /* AnalogGainMax*/
0xF3, 0x09,     /* SCLK*/
0xF4, 0x60,
0xF9, 0x00,     /* GainMax*/
0xFA, 0xC8,     /* GainMax*/
0xFB, 0x62,     /* Gain3Lut*/
0xFC, 0x39,     /* Gain2Lut*/
0xFD, 0x28,     /* Gain1Lut*/
0xFE, 0x12,     /* GainMin*/
0xFF, 0x83, /*Page mode    */
0x03, 0x0F,     /* TimeMax60Hz	 : 8fps*/
0x04, 0x0A,     /* Time3Lux60Hz : 12fps*/
0x05, 0x04,     /* Time2Lut60Hz : 24fps*/
0x06, 0x04,     /* Time1Lut60Hz : 24fps*/
0xFF, 0x82, /*Page mode  */
0xD3, 0x12,     /* LuxTBGainStep0  */
0xD4, 0x36,     /* LuxTBGainStep1  */
0xD5, 0x60,     /* LuxTBGainStep2*/
0xD6, 0x01,     /* LuxTBTimeStep0H*/
0xD7, 0x00,     /* LuxTBTimeStep0L*/
0xD8, 0x01,     /* LuxTBTimeStep1H*/
0xD9, 0xC0,     /* LuxTBTimeStep1L*/
0xDA, 0x06,     /* LuxTBTimeStep2H*/
0xDB, 0x00,     /* LuxTBTimeStep2L*/
0xFF, 0x83, /*Page mode  */
0x0B, 0x04,
0x0C, 0x4C,	/* Frame Rate*/
0xFF, 0x82, /*Page mode  */
0x92, 0x5D,

/***************************************************/
/* AWB*/
/***************************************************/
0xFF, 0x83, /*Page mode  */
0x79, 0x83,     /* AWB SKIN ON*/
0x86, 0x07,     /* gAWB_u16MinGrayCnt_rw_0*/
0x87, 0x00,     /* gAWB_u16MinGrayCnt_rw_1*/
0x90, 0x05,     /* gAWB_u16FinalRGain_ro_0*/
0x94, 0x05,     /* gAWB_u16FinalBGain_ro_0*/
0x98, 0xD4,     /* SkinWinCntTh*/
0xA2, 0x28,     /* SkinYTh*/
0xA3, 0x00,     /* SkinHoldHitCnt*/
0xA4, 0x0F,     /* SkinHoldHitCnt*/
0xAD, 0x65,     /* u8SkinTop2*/
0xAE, 0x80,     /* gAwb_u8SkinTop2LS1Ratio_rw  5zone*/
0xAF, 0x20,	 /* gAwb_u8SkinTop2LS2Ratio_rw  6zone */
0xB4, 0x10,     /* u8SkinTop2LSHys_rw*/
0xB5, 0x54,     /* gAwb_u8SkinLTx*/
0xB6, 0xbd,     /* gAwb_u8SkinLTy*/
0xB7, 0x74,     /* gAwb_u8SkinRBx*/
0xB8, 0x9d,     /* gAwb_u8SkinRBy*/
0xBA, 0x4F,     /* UniCThrY_rw*/
0xBF, 0x0C,     /* u16UniCGrayCntThr_rw_0*/
0xC0, 0x80,     /* u16UniCGrayCntThr_rw_1*/
0xFF, 0x87, /*Page mode  */
0xC9, 0x22,     /* gUR_u8AWBTrim_Addr*/
0xFF, 0x84, /*Page mode  */
0x49, 0x02,     /* Threshold_indoor*/
0x4A, 0x00,
0x4B, 0x03,     /* Threshold_outdoor*/
0x4C, 0x80,
0xFF, 0x83, /*Page mode  */
0xCB, 0x03,     /* R MinGain [Default 0X20]  A Spec Pass       */
0xCC, 0xC0,     /* R MinGain [Default 0X20]  A Spec Pass       */
0x82, 0x00,	/* lockratio*/
0xFF, 0x84,	/*Page mode  */
0x3D, 0x00,	/* gAwb_u32LuxConst1_rw_0*/
0x3E, 0x00,	/* gAwb_u32LuxConst1_rw_1*/
0x3F, 0x06,	/* gAwb_u32LuxConst1_rw_2*/
0x40, 0x20,    /* gAwb_u32LuxConst1_rw_3*/
0x41, 0x07,    /* gAwb_u32LuxConst2_rw_0*/
0x42, 0x53,    /* gAwb_u32LuxConst2_rw_1*/
0x43, 0x00,    /* gAwb_u32LuxConst2_rw_2*/
0x44, 0x00,    /* gAwb_u32LuxConst2_rw_3*/
0x55, 0x03,	/* gAwb_u8Weight_Gen_rw_0	*/
0x56, 0x10,    /* gAwb_u8Weight_Gen_rw_1	*/
0x57, 0x14,    /* gAwb_u8Weight_Gen_rw_2	*/
0x58, 0x07,    /* gAwb_u8Weight_Gen_rw_3	*/
0x59, 0x04,    /* gAwb_u8Weight_Gen_rw_4	*/
0x5A, 0x03,    /* gAwb_u8Weight_Gen_rw_5	*/
0x5B, 0x03,    /* gAwb_u8Weight_Gen_rw_6	*/
0x5C, 0x15,    /* gAwb_u8Weight_Gen_rw_7	*/
0x5D, 0x01,    /* gAwb_u8Weight_Ind_rw_0	*/
0x5E, 0x0F,    /* gAwb_u8Weight_Ind_rw_1	*/
0x5F, 0x07,    /* gAwb_u8Weight_Ind_rw_2	*/
0x60, 0x14,    /* gAwb_u8Weight_Ind_rw_3	*/
0x61, 0x14,    /* gAwb_u8Weight_Ind_rw_4	*/
0x62, 0x12,    /* gAwb_u8Weight_Ind_rw_5	*/
0x63, 0x11,    /* gAwb_u8Weight_Ind_rw_6	*/
0x64, 0x14,    /* gAwb_u8Weight_Ind_rw_7	*/
0x65, 0x03,    /* gAwb_u8Weight_Outd_rw_0*/
0x66, 0x05,    /* gAwb_u8Weight_Outd_rw_1*/
0x67, 0x15,    /* gAwb_u8Weight_Outd_rw_2*/
0x68, 0x04,    /* gAwb_u8Weight_Outd_rw_3*/
0x69, 0x01,    /* gAwb_u8Weight_Outd_rw_4*/
0x6A, 0x02,    /* gAwb_u8Weight_Outd_rw_5*/
0x6B, 0x03,    /* gAwb_u8Weight_Outd_rw_6*/
0x6C, 0x15,    /* gAwb_u8Weight_Outd_rw_6*/
0xFF, 0x85, /*Page mode  */
0xE2, 0x0C,     /* gPT_u8Awb_UnicolorZone_rw */
0xFF, 0x83, /*Page mode  */
0xCD, 0x06,		 /*Max Rgain*/
0xCE, 0x80,
0xD1, 0x06,		 /*Max BGain*/
0xd2,	0x80,

/***************************************************/
/* AWB STE*/
/***************************************************/
0xFF, 0xA1, /*Page mode  */
/*Flash*/
0xA0, 0x5c,	/*AWBZone0LTx*/
0xA1, 0x7a,	/*AWBZone0LTy*/
0xA2, 0x69,	/*AWBZone0RBx*/
0xA3, 0x6f,	/*AWBZone0RBy*/
/*cloudy*/
0xA4, 0x73,	/*AWBZone1LTx*/
0xA5, 0x55,	/*AWBZone1LTy*/
0xA6, 0x8C,	/*AWBZone1RBx*/
0xA7, 0x30,	/*AWBZone1RBy */
/*Daylight      */
0xA8, 0x69,	/*AWBZone2LTx*/
0xA9, 0x69,	/*AWBZone2LTy*/
0xAA, 0x83,	/*AWBZone2RBx*/
0xAB, 0x52,	/*AWBZone2RBy */
/*Fluorescent   */
0xAC, 0x57,	/*AWBZone3LTx*/
0xAD, 0x6e,	/*AWBZone3LTy*/
0xAE, 0x6f,	/*AWBZone3RBx*/
0xAF, 0x59,	/*AWBZone3RBy*/

/*CWF           */
0xB0, 0x50,	/*AWBZone4LTx*/
0xB1, 0x74,	/*AWBZone4LTy*/
0xB2, 0x65,	/*AWBZone4RBx*/
0xB3, 0x5d,	/*AWBZone4RBy*/
/*TL84          */
0xB4, 0x53,	/*AWBZone5LTx*/
0xB5, 0x7f,	/*AWBZone5LTy*/
0xB6, 0x62,	/*AWBZone5RBx*/
0xB7, 0x75,	/*AWBZone5RBy */
/*A             */
0xB8, 0x4a,	/*AWBZone6LTx*/
0xB9, 0x87,	/*AWBZone6LTy*/
0xBA, 0x59,	/*AWBZone6RBx*/
0xBB, 0x78,	/*AWBZone6RBy*/
/*Horizon       */
0xBC, 0x41,	/*AWBZone7LTx*/
0xBD, 0x91,	/*AWBZone7LTy*/
0xBE, 0x4b,	/*AWBZone7RBx*/
0xBF, 0x89,	/*AWBZone7RBy*/
/*Skin          */
0xC0, 0x5b,	/*AWBZone8LTx*/
0xC1, 0x85,	/*AWBZone8LTy*/
0xC2, 0x60,	/*AWBZone8RBx*/
0xC3, 0x7b,	/*AWBZone8RBy*/

/***************************************************/
/* UR*/
/***************************************************/
0xFF, 0x85, /*Page mode  */
0x06,	0x05,
0xFF, 0x86, /*Page mode  */
0x14, 0x1E,	/* CCM sum 1*/
0xFF, 0x85, /*Page mode  */
0x86, 0x42,	/* 42 saturation level*/
0x07, 0x00,	  /* sup hysteresis*/

/*DAY light     */
0xFF, 0x83, /*Page mode  */
0xEA, 0x00,	/*gAwb_s16AdapCCMTbl_0*/
0xEB, 0x53,	/*gAwb_s16AdapCCMTbl_1*/
0xEC, 0xFF,	/*gAwb_s16AdapCCMTbl_2*/
0xED, 0xE1,	/*gAwb_s16AdapCCMTbl_3*/
0xEE, 0x00,	/*gAwb_s16AdapCCMTbl_4*/
0xEF, 0x05,	/*gAwb_s16AdapCCMTbl_5*/
0xF0, 0xFF,	/*gAwb_s16AdapCCMTbl_6*/
0xF1, 0xF3,	/*gAwb_s16AdapCCMTbl_7*/
0xF2, 0x00,	/*gAwb_s16AdapCCMTbl_8*/
0xF3, 0x4B,	/*gAwb_s16AdapCCMTbl_9*/
0xF4, 0xFF,	/*gAwb_s16AdapCCMTbl_10*/
0xF5, 0xFA,	/*gAwb_s16AdapCCMTbl_11*/
0xF6, 0xFF,	/*gAwb_s16AdapCCMTbl_12*/
0xF7, 0xFa, /*gAwb_s16AdapCCMTbl_13*/
0xF8, 0xFF,	/*gAwb_s16AdapCCMTbl_14*/
0xF9, 0xC3,	/*gAwb_s16AdapCCMTbl_15*/
0xFA, 0x00,	/*gAwb_s16AdapCCMTbl_16*/
0xFB, 0x80,	/*gAwb_s16AdapCCMTbl_17*/

/*CWF lgiht    */
0xFF, 0x83, /*Page mode*/
0xFC, 0x00,     /* gAwb_s16AdapCCMTbl_18   */
0xFD, 0x68,     /* gAwb_s16AdapCCMTbl_19   */
0xFF, 0x85, /*Page mode  */
0xE0, 0xFF,	 /* gAwb_s16AdapCCMTbl_20   */
0xE1, 0xde,	 /* gAwb_s16AdapCCMTbl_21   */
0xFF, 0x84, /*Page mode  */
0x00, 0xff,     /* gAwb_s16AdapCCMTbl_22   */
0x01, 0xfa,     /* gAwb_s16AdapCCMTbl_23   */
0x02, 0xFF,     /* gAwb_s16AdapCCMTbl_24   */
0x03, 0xf0,     /* gAwb_s16AdapCCMTbl_25   */
0x04, 0x00,     /* gAwb_s16AdapCCMTbl_26   */
0x05, 0x52,     /* gAwb_s16AdapCCMTbl_27   */
0x06, 0xFF,     /* gAwb_s16AdapCCMTbl_28   */
0x07, 0xFa,     /* gAwb_s16AdapCCMTbl_29   */
0x08, 0x00,     /* gAwb_s16AdapCCMTbl_30   */
0x09, 0x00,     /* gAwb_s16AdapCCMTbl_31   */
0x0A, 0xFF,     /* gAwb_s16AdapCCMTbl_32   */
0x0B, 0xdb,     /* gAwb_s16AdapCCMTbl_33   */
0x0C, 0x00,     /* gAwb_s16AdapCCMTbl_34   */
0x0D, 0x68,     /* gAwb_s16AdapCCMTbl_35 */

/*A light */

0x0E, 0x00,     /* gAwb_s16AdapCCMTbl_36 */
0x0F, 0x6d,	 /* gAwb_s16AdapCCMTbl_37 */
0x10, 0xFF,     /* gAwb_s16AdapCCMTbl_38 */
0x11, 0xd5,	/* gAwb_s16AdapCCMTbl_39 */
0x12, 0xff,	/* gAwb_s16AdapCCMTbl_40 */
0x13, 0xfe,	/* gAwb_s16AdapCCMTbl_41 */
0x14, 0xFF,	/* gAwb_s16AdapCCMTbl_42 */
0x15, 0xf4,	/* gAwb_s16AdapCCMTbl_43 */
0x16, 0x00,	/* gAwb_s16AdapCCMTbl_44 */
0x17, 0x5a,	/* gAwb_s16AdapCCMTbl_45 */
0x18, 0xff,	/* gAwb_s16AdapCCMTbl_46 */
0x19, 0xef,	/* gAwb_s16AdapCCMTbl_47 */
0x1A, 0xff,	/* gAwb_s16AdapCCMTbl_48 */
0x1B, 0xfa,	/* gAwb_s16AdapCCMTbl_49 */
0x1C, 0xFF,	/* gAwb_s16AdapCCMTbl_50 */
0x1D, 0xbe,	/* gAwb_s16AdapCCMTbl_51 */
0x1E, 0x00,	/* gAwb_s16AdapCCMTbl_52 */
0x1F, 0x86,	/* gAwb_s16AdapCCMTbl_53 */


/***************************************************/
/* ADF*/
/***************************************************/

/* ISP setting*/
0xFF, 0xA0, /*Page mode  */
0x10, 0x80,	/* BLC: ABLC db*/
0x60, 0x73, /* CDC: Dark CDC ON  */
0x61, 0x1F, /* Six CDC_Edge En, Slash EN*/
0x69, 0x0C, /* Slash direction Line threshold*/
0x6A, 0x60, /* Slash direction Pixel threshold*/
0xC2, 0x04,	/* NSF: directional smoothing*/
0xD0, 0x51,	/* DEM: pattern detection*/
0xFF, 0xA1, /*Page mode  */
0x30, 0x01,	/* EDE: Luminane adaptation on*/
0x32, 0x50,	/* EDE: Adaptation slope */
0x34, 0x00,	/* EDE: x1 point */
0x35, 0x0B,	/* EDE: x1 point */
0x36, 0x01,	/* EDE: x2 point */
0x37, 0x80,	/* EDE: x2 point */
0x3A, 0x00,	/* EDE: Adaptation left margin*/
0x3B, 0x30,	/* EDE: Adaptation right margin*/
0x3C, 0x08,	/* EDE: rgb edge threshol*/

/* Adaptive Setting*/
0xFF, 0x85, /*Page mode  */
/* LSC*/
0x0F, 0x43,	/* LVLSC lux level threshold*/
0x10, 0x43,	/* LSLSC Light source , threshold lux*/

/* BGT*/
0x17, 0x30,	/* BGT lux level threshold*/
0x26, 0x20,	/* MinVal */
0x3c, 0x00,	/* MaxVal */

/* CNT*/
0x18, 0x43,	/* CNT lux level threshold*/
0x27, 0x00,	/* MinVal */
0x3d, 0x00,	/* MaxVal */

/* NSF */
0x12, 0xA5,	/* NSF lux level threshold */
0x22, 0x38,	/* u8MinVal_NSF1*/
0x23, 0x70,	/* u8MinVal_NSF2*/
0xFF, 0x86, /*Page mode  */
0x12, 0x00,	/* u8MinVal_NSF3*/
0xFF, 0x85, /*Page mode  */
0x38, 0x12,	/* u8MaxVal_NSF1*/
0x39, 0x30,	/* u8MaxVal_NSF2*/
0xFF, 0x86,	/*Page mode  */
0x13, 0x08,	/* u8MaxVal_NSF3*/

/* GDC*/
0xFF, 0x85, /*Page mode  */
0x15, 0xF4,	/* GDC lux level threshold */
0x2D, 0x20,	/* u8MinVal_GDC1*/
0x2E, 0x30,	/* u8MinVal_GDC2*/
0x43, 0x40,	/* u8MaxVal_GDC1*/
0x44, 0x80,	/* u8MaxVal_GDC2*/

/* ISP  Edge*/
0x04, 0xFB,	/* EnEDE*/
0x14, 0x54,	/* u8ThrLevel_EDE*/
0x28, 0x00,	/* u8MinVal_EDE_CP*/
0x29, 0x03,	/* u8MinVal_EDE1*/
0x2A, 0x20,	/* u8MinVal_EDE2*/
0x2B, 0x00,	/* u8MinVal_EDE_OFS*/
0x2C, 0x22,	/* u8MinVal_SG*/
0x3E, 0x00,	/* u8MaxVal_EDE_CP*/
0x3F, 0x09,	/* u8MaxVal_EDE1*/
0x40, 0x22,	/* u8MaxVal_EDE2*/
0x41, 0x02,	/* u8MaxVal_EDE_OFS*/
0x42, 0x33,	/* u8MaxVal_SG*/

/* Gamma Adaptive*/

0x16, 0xA0,	/* Gamma Threshold*/
0x47, 0x00,	/* Min_Gamma_0*/
0x48, 0x03,	/* Min_Gamma_1*/
0x49, 0x10,	/* Min_Gamma_2*/
0x4A, 0x25,	/* Min_Gamma_3*/
0x4B, 0x3B,	/* Min_Gamma_4*/
0x4C, 0x4F,	/* Min_Gamma_5*/
0x4D, 0x6D,	/* Min_Gamma_6*/
0x4E, 0x86,	/* Min_Gamma_7*/
0x4F, 0x9B,	/* Min_Gamma_8*/
0x50, 0xAD,	/* Min_Gamma_9*/
0x51, 0xC2,	/* Min_Gamma_10*/
0x52, 0xD3,	/* Min_Gamma_11*/
0x53, 0xE1,	/* Min_Gamma_12*/
0x54, 0xE9,	/* Min_Gamma_13*/
0x55, 0xF2,	/* Min_Gamma_14*/
0x56, 0xFA,	/* Min_Gamma_15*/
0x57, 0xFF,	/* Min_Gamma_16*/
0x58, 0x00,	/* Max_Gamma_0  */
0x59, 0x06,	/* Max_Gamma_1  */
0x5a, 0x14,	/* Max_Gamma_2  */
0x5b, 0x30,	/* Max_Gamma_3  */
0x5c, 0x4A,	/* Max_Gamma_4  */
0x5d, 0x5D,	/* Max_Gamma_5  */
0x5e, 0x75,	/* Max_Gamma_6  */
0x5f, 0x89,	/* Max_Gamma_7  */
0x60, 0x9A,	/* Max_Gamma_8  */
0x61, 0xA7,	/* Max_Gamma_9  */
0x62, 0xBC,	/* Max_Gamma_10 */
0x63, 0xD0,	/* Max_Gamma_11 */
0x64, 0xE0,	/* Max_Gamma_12 */
0x65, 0xE7,	/* Max_Gamma_13 */
0x66, 0xEE,	/* Max_Gamma_14 */
0x67, 0xF7,	/* Max_Gamma_15 */
0x68, 0xFF,	/* Max_Gamma_16 */

/* Initial edge, noise filter setting*/
0xFF, 0xA0,	/*Page mode  */
0xC0, 0x12,	/*NSFTh1_Addr*/
0xC1, 0x30,	/*NSFTh2_Addr*/
0xC2, 0x08,	/*NSFTh3_Addr*/
0xFF, 0xA1,	/*Page mode */
0x30, 0x01,	/*EDEOption_Addr*/
0x31, 0x33,	/*EDESlopeGain_Addr*/
0x32, 0x50,	/*EDELuAdpCtrl_Addr*/
0x33, 0x00,	/*EDEPreCoringPt_Addr*/
0x34, 0x00,	/*EDETransFuncXp1_Addr1*/
0x35, 0x0B,	/*EDETransFuncXp1_Addr0*/
0x36, 0x01,	/*EDETransFuncXp2_Addr1*/
0x37, 0x80,	/*EDETransFuncXp2_Addr0*/
0x38, 0x09,	/*EDETransFuncSl1_Addr*/
0x39, 0x22,	/*EDETransFuncSl2_Addr*/
0x3A, 0x00,	/*EDELuAdpLOffset_Addr*/
0x3B, 0x30,	/*EDELuAdpROffset_Addr*/
0x3C, 0x08,	/*EDERBThrd_Addr*/
0x3D, 0x02,	/*EDESmallOffset_Addr*/


/* CCM Saturation Level*/
0xFF, 0x85,	/*Page mode */
0x1A, 0x64,	/* SUP Threshold	*/
0x30, 0x40,	/* MinSUP	 */

0xFF, 0xA0, /*Page mode */
/* Lens Shading*/
0x43, 0x80,	/* RH7 rrhrhh*/
0x44, 0x80,	/* RV*/
0x45, 0x80,	/* RH	*/
0x46, 0x80,	/* RV*/
0x47, 0x80,	/* GH*/
0x48, 0x80,	/* GV*/
0x49, 0x80,	/* GH*/
0x4A, 0x80,	/* GV	*/
0x4B, 0x80,	/* BH*/
0x4C, 0x80,	/* BV*/
0x4D, 0x80,	/* BH*/
0x4E, 0x80,	/* BV  */
0x52, 0x90,	/* GGain*/
0x53, 0x20,	/* GGain*/
0x54, 0x00,	/* GGain*/

/*Max Shading*/
0xFF, 0x85, /*Page mode */
0x32, 0xC0,
0x33, 0x30,
0x34, 0x00,
0x35, 0x90,
0x36, 0x20,
0x37, 0x00,

/*Min Shading*/
0x1c, 0xC0,
0x1d, 0x30,
0x1e, 0x00,
0x1f, 0x90,
0x20, 0x18,
0x21, 0x00,

/* LineLength     */
0xFF, 0x87, /*Page mode */
0xDC, 0x05,	/* by Yong In Han 091511*/
0xDD, 0xB0,	/* by Yong In Han 091511*/
0xd5, 0x00,	/* Flip*/
/***************************************************/
/* SensorCon */
/***************************************************/
0xFF, 0xD0, /*Page mode */
0x20, 0x0E,	/* ABLK_Rsrv_Addr*/
0x20, 0x0D,	/* ABLK_Rsrv_Addr*/

/***************************************************/
/* MIPI */
/***************************************************/
0xFF, 0xB0, /*Page mode */
0x54, 0x02,	/* MIPI PHY_HS_TX_CTRL*/
0x38, 0x05,	/* MIPI DPHY_CTRL_SET*/


/* SXGA PR*/
0xFF, 0x85, /*Page mode */
0xB8, 0x0A,	/* gPT_u8PR_Active_SXGA_WORD_COUNT_Addr0*/
0xB9, 0x00,	/* gPT_u8PR_Active_SXGA_WORD_COUNT_Addr1*/
0xBC, 0x04,	/* gPT_u8PR_Active_SXGA_DPHY_CLK_TIME_Addr3*/
0xFF, 0x87, /*Page mode */
0x0C, 0x00,	/* start Y*/
0x0D, 0x20,	/* start Y   */
0x10, 0x03,	/* end Y*/
0x11, 0xE0,	/* end Y   */

/* Recoding */
0xFF, 0x86, /*Page mode */
0x38, 0x05,	/* gPT_u8PR_Active_720P_WORD_COUNT_Addr0*/
0x39, 0x00,	/* gPT_u8PR_Active_720P_WORD_COUNT_Addr1*/
0x3C, 0x04,	/* gPT_u8PR_Active_720P_DPHY_CLK_TIME_Addr3*/

0xFF, 0x87,
0x23, 0x02,	/*gPR_Active_720P_u8SensorCtrl_Addr           */
0x25, 0x01,	/*gPR_Active_720P_u8PLL_P_Addr                */
0x26, 0x0F,	/*gPR_Active_720P_u8PLL_M_Addr                */
0x27, 0x00,	/*gPR_Active_720P_u8PLL_S_Addr                */
0x28, 0x00,	/*gPR_Active_720P_u8PLL_Ctrl_Addr             */
0x29, 0x01,	/*gPR_Active_720P_u8src_clk_sel_Addr          */
0x2A, 0x00,	/*gPR_Active_720P_u8output_pad_status_Addr    */
0x2B, 0x3F,	/*gPR_Active_720P_u8ablk_ctrl_10_Addr         */
0x2C, 0xFF,	/*gPR_Active_720P_u8BayerFunc_Addr            */
0x2D, 0xFF,	/*gPR_Active_720P_u8RgbYcFunc_Addr            */
0x2E, 0x00,	/*gPR_Active_720P_u8ISPMode_Addr              */
0x2F, 0x02,	/*gPR_Active_720P_u8SCLCtrl_Addr              */
0x30, 0x01,	/*gPR_Active_720P_u8SCLHorScale_Addr0         */
0x31, 0xFF,	/*gPR_Active_720P_u8SCLHorScale_Addr1         */
0x32, 0x03,	/*gPR_Active_720P_u8SCLVerScale_Addr0         */
0x33, 0xFF,	/*gPR_Active_720P_u8SCLVerScale_Addr1         */
0x34, 0x00,	/*gPR_Active_720P_u8SCLCropStartX_Addr0       */
0x35, 0x00,	/*gPR_Active_720P_u8SCLCropStartX_Addr1       */
0x36, 0x00,	/*gPR_Active_720P_u8SCLCropStartY_Addr0       */
0x37, 0x10,	/*gPR_Active_720P_u8SCLCropStartY_Addr1       */
0x38, 0x02,	/*gPR_Active_720P_u8SCLCropEndX_Addr0         */
0x39, 0x80,	/*gPR_Active_720P_u8SCLCropEndX_Addr1         */
0x3A, 0x01,	/*gPR_Active_720P_u8SCLCropEndY_Addr0         */
0x3B, 0xF0,	/*gPR_Active_720P_u8SCLCropEndY_Addr1         */
0x3C, 0x01,	/*gPR_Active_720P_u8OutForm_Addr              */
0x3D, 0x0C,	/*gPR_Active_720P_u8OutCtrl_Addr              */
0x3E, 0x04,	/*gPR_Active_720P_u8AEWinStartX_Addr          */
0x3F, 0x04,	/*gPR_Active_720P_u8AEWinStartY_Addr          */
0x40, 0x66,	/*gPR_Active_720P_u8MergedWinWidth_Addr       */
0x41, 0x5E,	/*gPR_Active_720P_u8MergedWinHeight_Addr      */
0x42, 0x04,	/*gPR_Active_720P_u8AEHistWinAx_Addr          */
0x43, 0x04,	/*gPR_Active_720P_u8AEHistWinAy_Addr          */
0x44, 0x98,	/*gPR_Active_720P_u8AEHistWinBx_Addr          */
0x45, 0x78,	/*gPR_Active_720P_u8AEHistWinBy_Addr          */
0x46, 0x22,	/*gPR_Active_720P_u8AWBTrim_Addr              */
0x47, 0x28,	/*gPR_Active_720P_u8AWBCTWinAx_Addr           */
0x48, 0x20,	/*gPR_Active_720P_u8AWBCTWinAy_Addr           */
0x49, 0x78,	/*gPR_Active_720P_u8AWBCTWinBx_Addr           */
0x4A, 0x60,	/*gPR_Active_720P_u8AWBCTWinBy_Addr           */
0x4B, 0x03,	/*gPR_Active_720P_u16AFCFrameLength_0         */
0x4C, 0x00,	/*gPR_Active_720P_u16AFCFrameLength_1         */

/* VGA PR     */
0xFF, 0x86, /*Page mode */
0x2F, 0x05,	/* gPT_u8PR_Active_VGA_WORD_COUNT_Addr0*/
0x30, 0x00,	/* gPT_u8PR_Active_VGA_WORD_COUNT_Addr1*/
0x33, 0x04,	/* gPT_u8PR_Active_VGA_DPHY_CLK_TIME_Addr3*/

0xFF, 0x87, /*Page mode */
0x4D, 0x00,	/*gPR_Active_VGA_u8SensorCtrl_Addr*/
0x4E, 0x72,	/*gPR_Active_VGA_u8SensorMode_Addr*/
0x4F, 0x01,	/*gPR_Active_VGA_u8PLL_P_Addr*/
0x50, 0x0F,	/*gPR_Active_VGA_u8PLL_M_Addr*/
0x51, 0x00,	/*gPR_Active_VGA_u8PLL_S_Addr*/
0x52, 0x00,	/*gPR_Active_VGA_u8PLL_Ctrl_Addr*/
0x53, 0x01,	/*gPR_Active_VGA_u8src_clk_sel_Addr*/
0x54, 0x00,	/*gPR_Active_VGA_u8output_pad_status_Addr*/
0x55, 0x3F,	/*gPR_Active_VGA_u8ablk_ctrl_10_Addr*/
0x56, 0xFF,	/*gPR_Active_VGA_u8BayerFunc_Addr*/
0x57, 0xFF,	/*gPR_Active_VGA_u8RgbYcFunc_Addr*/
0x58, 0x00,	/*gPR_Active_VGA_u8ISPMode_Addr*/
0x59, 0x02,	/*gPR_Active_VGA_u8SCLCtrl_Addr*/
0x5A, 0x01,	/*gPR_Active_VGA_u8SCLHorScale_Addr0*/
0x5B, 0xFF,	/*gPR_Active_VGA_u8SCLHorScale_Addr1*/
0x5C, 0x01,	/*gPR_Active_VGA_u8SCLVerScale_Addr0*/
0x5D, 0xFF,	/*gPR_Active_VGA_u8SCLVerScale_Addr1*/
0x5E, 0x00,	/*gPR_Active_VGA_u8SCLCropStartX_Addr0*/
0x5F, 0x00,	/*gPR_Active_VGA_u8SCLCropStartX_Addr1*/
0x60, 0x00,	/*gPR_Active_VGA_u8SCLCropStartY_Addr0*/
0x61, 0x20,	/*gPR_Active_VGA_u8SCLCropStartY_Addr1*/
0x62, 0x05,	/*gPR_Active_VGA_u8SCLCropEndX_Addr0*/
0x63, 0x00,	/*gPR_Active_VGA_u8SCLCropEndX_Addr1*/
0x64, 0x03,	/*gPR_Active_VGA_u8SCLCropEndY_Addr0*/
0x65, 0xE0,	/*gPR_Active_VGA_u8SCLCropEndY_Addr1*/

0xFF, 0xd1, /*Page mode */
0x07, 0x00, /* power off mask clear*/
0x0b, 0x00, /* clock off mask clear*/
0xFF, 0xC0, /*Page mode */
0x10, 0x41,

/* Self-Cam END of Initial */
};

/* Set-data based on SKT VT standard ,when using 3G network*/
/* VGA 8fps
*/
static const u8 db8131m_vt_common_1[] = {
/***************************************************/
/* Device : DB8131M fixed 8Fps */
/* MIPI Interface for Noncontious Clock */
/***************************************************/
/***************************************************/
/* Command */
/***************************************************/
0xFF, 0xC0,  /*Page mode*/
0x10, 0x01,
};
/*wait	150*/

static const u8 db8131m_vt_common_2[] = {
/***************************************************/
/* Format */
/***************************************************/
0xFF, 0xA1, /*Page mode    */
0x70, 0x01,
0x71, 0x0D,

/***************************************************/
/* SensorCon */
/***************************************************/
0xFF, 0xD0, /*Page mode    */
0x0F, 0x0B,	/*ABLK_Ctrl_1_Addr*/
0x13, 0x00,	/*Gain_Addr*/
0x15, 0x01,	/*IVREFT_REFB_Addr*/
0x20, 0x0E,	/*ABLK_Rsrv_Addr*/
0x23, 0x01,	/*IVREFT2_REFB2_Addr*/
0x24, 0x01,	/*IPPG_IVCM2_Addr*/
0x39, 0x70,	/*RiseSx_CDS_1_L_Addr*/
0x51, 0x19,	/*Fallcreset_1_L_Addr*/
0x83, 0x2D,	/*RiseTran_Sig_Even_L_Addr*/
0x85, 0x2F,	/*FallTran_Sig_Even_L_Addr*/
0x87, 0x2D,	/*RiseTran_Sig_Odd_L_Addr*/
0x89, 0x2F,	/*FallTran_Sig_Odd_L_Addr*/
0x8B, 0x27,	/*RiseCNT_EN_1_L_Addr*/
0x8D, 0x6c,	/*FallCNT_EN_1_L_Addr*/
0xD7, 0x80,	/*ABLK_Ctrl_12_Addr*/
0xDB, 0xA2,	/*FallScanTx15_1_L_Addr*/
0xED, 0x01,	/*PLL_P_Addr*/
0xEE, 0x0F,	/*PLL_M_Addr*/
0xEF, 0x00,	/*PLL_S_Addr*/
0xF9, 0x00,	/*ABLK_Ctrl_8*/
0xF8, 0x00, /*Vblank Sleep Mode enable*/
0xFB, 0x90,	/*PostADC_Gain_Addr*/

/***************************************************/
/* Analog ADF */
/***************************************************/
0xFF, 0x85, /*Page mode    */
0x89, 0x93,	/*gAdf_u8APThreshold*/
0x8A, 0x0C, /*u8APClmpThreshold*/
0x8C, 0x07,	/*gAdf_u8APMinVal_ThrClampH*/
0x8D, 0x40,	/*gAdf_u8APMinVal_ThrClampL*/
0x8E, 0x03,	/*gAdf_u8APMinVal_DOFFSET*/
0x8F, 0x14,	/*gAdf_u8APMinVal_AMP2_1_SDM*/
0x91, 0x1A,	/*gAdf_u8APMinVal_AMP4_3_SDM*/
0x92, 0x0F,	/*gAdf_u8APMinVal_FallIntTx15*/
0x93, 0x47,	/*gAdf_u8APMinVal_CDSxRange_CtrlPre*/
0x95, 0x18,	/*gAdf_u8APMinVal_REFB_IVCM*/
0x96, 0x38,	/*gAdf_u8APMinVal_ref_os_PB*/
0x97, 0x0D,	/*gAdf_u8APMinVal_NTx_Range*/
0x98, 0x0D,	/*gAdf_u8APMaxVal_Clmp_rst*/
0x99, 0x06,	/*gAdf_u8APMaxVal_ThrClampH*/
0x9A, 0x9F,	/*gAdf_u8APMaxVal_ThrClampL*/
0x9B, 0x02,	/*gAdf_u8APMaxVal_DOFFSET*/
0x9C, 0x1C,	/*gAdf_u8APMaxVal_AMP2_1_SDM*/
0x9E, 0x11,	/*gAdf_u8APMaxVal_AMP4_3_SDM*/
0x9F, 0x5D,	/*gAdf_u8APMaxVal_FallIntTx15*/
0xA0, 0x78,	/*gAdf_u8APMaxVal_CDSxRange_CtrlPre*/
0xA2, 0x18,	/*gAdf_u8APMaxVal_REFB_IVCM*/
0xA3, 0x40,	/*gAdf_u8APMaxVal_ref_os_PB*/
0xA4, 0x0B,	/*gAdf_u8APMaxVal_NTx_Range*/

0xFF, 0x86, /*Page mode    */
0x15, 0x00,	/*gPT_u8Adf_APThrHys*/
0x16, 0xF7,	/*gPT_u8Adf_APFallIntTxThrLevel*/
0x17, 0x13,	/*gPT_u8Adf_APMinVal_BP2_1_SDM*/
0x18, 0x13,	/*gPT_u8Adf_APMidVal_BP2_1_SDM*/
0x19, 0x1C,	/*gPT_u8Adf_APMaxVal_BP2_1_SDM*/
0x1A, 0x06,	/*gPT_u8Adf_APMidVal_ThrClampH*/
0x1B, 0xF0,	/*gPT_u8Adf_APMidVal_ThrClampL*/
0x1C, 0x01,	/*gPT_u8Adf_APMidVal_DOFFSET*/
0x1D, 0x14,	/*gPT_u8Adf_APMidVal_AMP2_1_SDM*/
0x1F, 0x31,	/*gPT_u8Adf_APMidVal_AMP4_3_SDM*/
0x20, 0x68,	/*gPT_u8Adf_APMidVal_CDSxRange_CtrlPre*/
0x22, 0x18,	/*gPT_u8Adf_APMidVal_REFB_IVCM*/
0x23, 0x38,	/*gPT_u8Adf_APMidVal_ref_os_PB*/
0x24, 0x0F,	/*gPT_u8Adf_APMidVal_NTx_Range*/
0x25, 0x77,	/*gPT_u8Adf_APVal_EnSiSoSht_EnSm*/

0xFF, 0x87, /*Page mode    */
0xEA, 0x41,

0xFF, 0xD0, /*Page mode    */
0x20, 0x0D,	/*ABLK_Rsrv_Addr*/

0xFF, 0x83, /*Page mode  */
0x63, 0x28,	/*Again Table*/
0x64, 0x10,	/*Again Table*/
0x65, 0xA8,	/*Again Table*/
0x66, 0x50,	/*Again Table*/
0x67, 0x28,	/*Again Table*/
0x68, 0x14,	/*Again Table*/


/***************************************************/
/* AE */
/***************************************************/
0xFF, 0x82, /*Page mode */
0x91, 0x02,	/*	AeMode*/
0x95, 0x88,	/* AE weight*/
0x96, 0x88,
0x97, 0xF8,
0x98, 0x8F,
0x99, 0xF8,
0x9A, 0x8F,
0x9B, 0x88,
0x9C, 0x88,
0xA9, 0x40,	/* OTarget*/
0xAA, 0x40,	/* ITarget*/
0x9D, 0x66,	/* AE Speed*/
0x9F, 0x06,	/* AE HoldBnd*/
0xA8, 0x40,	/* STarget*/
0xB9, 0x04,	/* RGain*/
0xBB, 0x04,	/* GGain*/
0xBD, 0x04,	/* BGain*/
0xC5, 0x02,	/* PeakMvStep*/
0xC6, 0x38,	/* PeakTgMin*/
0xC7, 0x24,	/* PeakRatioTh1*/
0xC8, 0x10,	/* PeakRatioTh0*/
0xC9, 0x05,	/* PeakLuTh*/
0xD5, 0x60,	/* LuxGainTB_2*/
0xFF, 0x83, /*Page mode  */
0x2F, 0x04,	/* TimeNum0*/
0x30, 0x05,	/* TimeNum1*/
0x4F, 0x05,	/* FrameOffset*/
0xFF, 0x82, /*Page mode  */
0xA1, 0x0A,	/* AnalogGainMax*/
0xF3, 0x09,	/* SCLK*/
0xF4, 0x60,
0xF9, 0x00,	/* GainMax*/
0xFA, 0xC8,	/* GainMax*/
0xFB, 0x62,	/* Gain3Lut*/
0xFC, 0x39,	/* Gain2Lut*/
0xFD, 0x28,	/* Gain1Lut*/
0xFE, 0x12,	/* GainMin*/
0xFF, 0x83, /*Page mode    */
0x03, 0x0F,	/* TimeMax60Hz	 : 8fps*/
0x04, 0x0A,	/* Time3Lux60Hz : 12fps*/
0x05, 0x04,	/* Time2Lut60Hz : 24fps*/
0x06, 0x04,	/* Time1Lut60Hz : 24fps*/
0xFF, 0x82, /*Page mode  */
0xD3, 0x12,	/* LuxTBGainStep0  */
0xD4, 0x36,	/* LuxTBGainStep1  */
0xD5, 0x60,	/* LuxTBGainStep2*/
0xD6, 0x01,	/* LuxTBTimeStep0H*/
0xD7, 0x00,	/* LuxTBTimeStep0L*/
0xD8, 0x01,	/* LuxTBTimeStep1H*/
0xD9, 0xC0,	/* LuxTBTimeStep1L*/
0xDA, 0x06,	/* LuxTBTimeStep2H*/
0xDB, 0x00,	/* LuxTBTimeStep2L*/
0xFF, 0x83, /*Page mode  */
0x0B, 0x08,
0x0C, 0x0C,	/* Frame Rate*/
0xFF, 0x82, /*Page mode  */
0x92, 0x5D,

/***************************************************/
/* AWB */
/***************************************************/
0xFF, 0x83, /*Page mode  */
0x79, 0x83,	/* AWB SKIN ON*/
0x86, 0x07,	/* gAWB_u16MinGrayCnt_rw_0*/
0x87, 0x00,	/* gAWB_u16MinGrayCnt_rw_1*/
0x90, 0x05,	/* gAWB_u16FinalRGain_ro_0*/
0x94, 0x05,	/* gAWB_u16FinalBGain_ro_0*/
0x98, 0xD4,	/* SkinWinCntTh*/
0xA2, 0x28,	/* SkinYTh*/
0xA3, 0x00,	/* SkinHoldHitCnt*/
0xA4, 0x0F,	/* SkinHoldHitCnt*/
0xAD, 0x65,	/* u8SkinTop2*/
0xAE, 0x80,	/* gAwb_u8SkinTop2LS1Ratio_rw  5zone */
0xAF, 0x20,	/* gAwb_u8SkinTop2LS2Ratio_rw  6zone */
0xB4, 0x10,	/* u8SkinTop2LSHys_rw     */
0xB5, 0x54,	/* gAwb_u8SkinLTx */
0xB6, 0xbd,	/* gAwb_u8SkinLTy */
0xB7, 0x74,	/* gAwb_u8SkinRBx */
0xB8, 0x9d,	/* gAwb_u8SkinRBy */
0xBA, 0x4F,	/* UniCThrY_rw    */
0xBF, 0x0C,	/* u16UniCGrayCntThr_rw_0 */
0xC0, 0x80,	/* u16UniCGrayCntThr_rw_1 */
0xFF, 0x87, /*Page mode  */
0xC9, 0x22,	/* gUR_u8AWBTrim_Addr */
0xFF, 0x84, /*Page mode  */
0x49, 0x02,	/* Threshold_indoor */
0x4A, 0x00,
0x4B, 0x03,	/* Threshold_outdoor */
0x4C, 0x80,
0xFF, 0x83, /*Page mode  */
0xCB, 0x03,	/* R MinGain [Default 0X20]  A Spec Pass       */
0xCC, 0xC0,	/* R MinGain [Default 0X20]  A Spec Pass       */
0x82, 0x00,	/* lockratio*/
0xFF, 0x84, /*Page mode  */
0x3D, 0x00,	/* gAwb_u32LuxConst1_rw_0*/
0x3E, 0x00,	/* gAwb_u32LuxConst1_rw_1*/
0x3F, 0x06,	/* gAwb_u32LuxConst1_rw_2*/
0x40, 0x20,	/* gAwb_u32LuxConst1_rw_3*/
0x41, 0x07,	/* gAwb_u32LuxConst2_rw_0*/
0x42, 0x53,	/* gAwb_u32LuxConst2_rw_1*/
0x43, 0x00,	/* gAwb_u32LuxConst2_rw_2*/
0x44, 0x00,	/* gAwb_u32LuxConst2_rw_3*/
0x55, 0x03,	/* gAwb_u8Weight_Gen_rw_0	*/
0x56, 0x10,	/* gAwb_u8Weight_Gen_rw_1	*/
0x57, 0x14,	/* gAwb_u8Weight_Gen_rw_2	*/
0x58, 0x07,	/* gAwb_u8Weight_Gen_rw_3	*/
0x59, 0x04,	/* gAwb_u8Weight_Gen_rw_4	*/
0x5A, 0x03,	/* gAwb_u8Weight_Gen_rw_5	*/
0x5B, 0x03,	/* gAwb_u8Weight_Gen_rw_6	*/
0x5C, 0x15,	/* gAwb_u8Weight_Gen_rw_7	*/
0x5D, 0x01,	/* gAwb_u8Weight_Ind_rw_0	*/
0x5E, 0x0F,	/* gAwb_u8Weight_Ind_rw_1	*/
0x5F, 0x07,	/* gAwb_u8Weight_Ind_rw_2	*/
0x60, 0x14,	/* gAwb_u8Weight_Ind_rw_3	*/
0x61, 0x14,	/* gAwb_u8Weight_Ind_rw_4	*/
0x62, 0x12,	/* gAwb_u8Weight_Ind_rw_5	*/
0x63, 0x11,	/* gAwb_u8Weight_Ind_rw_6	*/
0x64, 0x14,	/* gAwb_u8Weight_Ind_rw_7	*/
0x65, 0x03,	/* gAwb_u8Weight_Outd_rw_0*/
0x66, 0x05,	/* gAwb_u8Weight_Outd_rw_1*/
0x67, 0x15,	/* gAwb_u8Weight_Outd_rw_2*/
0x68, 0x04,	/* gAwb_u8Weight_Outd_rw_3*/
0x69, 0x01,	/* gAwb_u8Weight_Outd_rw_4*/
0x6A, 0x02,	/* gAwb_u8Weight_Outd_rw_5*/
0x6B, 0x03,	/* gAwb_u8Weight_Outd_rw_6*/
0x6C, 0x15,	/* gAwb_u8Weight_Outd_rw_6*/
0xFF, 0x85, /*Page mode  */
0xE2, 0x0C,	/* gPT_u8Awb_UnicolorZone_rw */
0xFF, 0x83, /*Page mode  */
0xCD, 0x06,	/*Max Rgain*/
0xCE, 0x80,
0xD1, 0x06,	/*Max BGain*/
0xd2, 0x80,

/***************************************************/
/* AWB STE */
/***************************************************/
0xFF, 0xA1, /*Page mode  */
/* Flash */
0xA0, 0x5c,	/*AWBZone0LTx*/
0xA1, 0x7a,	/*AWBZone0LTy*/
0xA2, 0x69,	/*AWBZone0RBx*/
0xA3, 0x6f,	/*AWBZone0RBy*/
/* cloudy */
0xA4, 0x73,	/*AWBZone1LTx*/
0xA5, 0x55,	/*AWBZone1LTy*/
0xA6, 0x8C,	/*AWBZone1RBx*/
0xA7, 0x30,	/*AWBZone1RBy */
/* Daylight */
0xA8, 0x69,	/*AWBZone2LTx*/
0xA9, 0x69,	/*AWBZone2LTy*/
0xAA, 0x83,	/*AWBZone2RBx*/
0xAB, 0x52,	/*AWBZone2RBy */
/* Fluorescent */
0xAC, 0x57,	/*AWBZone3LTx*/
0xAD, 0x6e,	/*AWBZone3LTy*/
0xAE, 0x6f,	/*AWBZone3RBx*/
0xAF, 0x59,	/*AWBZone3RBy*/

/*CWF           */
0xB0, 0x50,	/*AWBZone4LTx*/
0xB1, 0x74,	/*AWBZone4LTy*/
0xB2, 0x65,	/*AWBZone4RBx*/
0xB3, 0x5d,	/*AWBZone4RBy*/
/*TL84          */
0xB4, 0x53,	/*AWBZone5LTx*/
0xB5, 0x7f,	/*AWBZone5LTy*/
0xB6, 0x62,	/*AWBZone5RBx*/
0xB7, 0x75,	/*AWBZone5RBy */
/*A             */
0xB8, 0x4a,	/*AWBZone6LTx*/
0xB9, 0x87,	/*AWBZone6LTy*/
0xBA, 0x59,	/*AWBZone6RBx*/
0xBB, 0x78,	/*AWBZone6RBy*/
/*Horizon       */
0xBC, 0x41,	/*AWBZone7LTx*/
0xBD, 0x91,	/*AWBZone7LTy*/
0xBE, 0x4b,	/*AWBZone7RBx*/
0xBF, 0x89,	/*AWBZone7RBy*/
/*Skin          */
0xC0, 0x5b,	/*AWBZone8LTx*/
0xC1, 0x85,	/*AWBZone8LTy*/
0xC2, 0x60,	/*AWBZone8RBx*/
0xC3, 0x7b,	/*AWBZone8RBy*/

/***************************************************/
/* UR */
/***************************************************/
0xFF, 0x85, /*Page mode  */
0x06, 0x05,
0xFF, 0x86, /*Page mode  */
0x14, 0x1E,	/* CCM sum 1*/
0xFF, 0x85, /*Page mode  */
0x86, 0x42,	/* 42 saturation level*/
0x07, 0x00,	/* sup hysteresis*/

/*DAY light     */
0xFF, 0x83, /*Page mode  */
0xEA, 0x00,	/*gAwb_s16AdapCCMTbl_0*/
0xEB, 0x53,	/*gAwb_s16AdapCCMTbl_1*/
0xEC, 0xFF,	/*gAwb_s16AdapCCMTbl_2*/
0xED, 0xE1,	/*gAwb_s16AdapCCMTbl_3*/
0xEE, 0x00,	/*gAwb_s16AdapCCMTbl_4*/
0xEF, 0x05,	/*gAwb_s16AdapCCMTbl_5*/
0xF0, 0xFF,	/*gAwb_s16AdapCCMTbl_6*/
0xF1, 0xF3,	/*gAwb_s16AdapCCMTbl_7*/
0xF2, 0x00,	/*gAwb_s16AdapCCMTbl_8*/
0xF3, 0x4B,	/*gAwb_s16AdapCCMTbl_9*/
0xF4, 0xFF,	/*gAwb_s16AdapCCMTbl_10*/
0xF5, 0xFA,	/*gAwb_s16AdapCCMTbl_11*/
0xF6, 0xFF,	/*gAwb_s16AdapCCMTbl_12*/
0xF7, 0xFa, /*gAwb_s16AdapCCMTbl_13*/
0xF8, 0xFF,	/*gAwb_s16AdapCCMTbl_14*/
0xF9, 0xC3,	/*gAwb_s16AdapCCMTbl_15*/
0xFA, 0x00,	/*gAwb_s16AdapCCMTbl_16*/
0xFB, 0x80,	/*gAwb_s16AdapCCMTbl_17*/

/*CWF lgiht    */
0xFF, 0x83, /*Page mode */
0xFC, 0x00,	/* gAwb_s16AdapCCMTbl_18   */
0xFD, 0x68,	/* gAwb_s16AdapCCMTbl_19   */
0xFF, 0x85, /*Page mode  */
0xE0, 0xFF,	/* gAwb_s16AdapCCMTbl_20   */
0xE1, 0xde,	/* gAwb_s16AdapCCMTbl_21   */
0xFF, 0x84, /*Page mode  */
0x00, 0xff,	/* gAwb_s16AdapCCMTbl_22   */
0x01, 0xfa,	/* gAwb_s16AdapCCMTbl_23   */
0x02, 0xFF,	/* gAwb_s16AdapCCMTbl_24   */
0x03, 0xf0,	/* gAwb_s16AdapCCMTbl_25   */
0x04, 0x00,	/* gAwb_s16AdapCCMTbl_26   */
0x05, 0x52,	/* gAwb_s16AdapCCMTbl_27   */
0x06, 0xFF,	/* gAwb_s16AdapCCMTbl_28   */
0x07, 0xFa,	/* gAwb_s16AdapCCMTbl_29   */
0x08, 0x00,	/* gAwb_s16AdapCCMTbl_30   */
0x09, 0x00,	/* gAwb_s16AdapCCMTbl_31   */
0x0A, 0xFF,	/* gAwb_s16AdapCCMTbl_32   */
0x0B, 0xdb,	/* gAwb_s16AdapCCMTbl_33   */
0x0C, 0x00,	/* gAwb_s16AdapCCMTbl_34   */
0x0D, 0x68,	/* gAwb_s16AdapCCMTbl_35 */

/*A light                                       */

0x0E, 0x00,	/* gAwb_s16AdapCCMTbl_36 */
0x0F, 0x6d,	/* gAwb_s16AdapCCMTbl_37 */
0x10, 0xFF,	/* gAwb_s16AdapCCMTbl_38 */
0x11, 0xd5,	/* gAwb_s16AdapCCMTbl_39 */
0x12, 0xff,	/* gAwb_s16AdapCCMTbl_40 */
0x13, 0xfe,	/* gAwb_s16AdapCCMTbl_41 */
0x14, 0xFF,	/* gAwb_s16AdapCCMTbl_42 */
0x15, 0xf4,	/* gAwb_s16AdapCCMTbl_43 */
0x16, 0x00,	/* gAwb_s16AdapCCMTbl_44 */
0x17, 0x5a,	/* gAwb_s16AdapCCMTbl_45 */
0x18, 0xff,	/* gAwb_s16AdapCCMTbl_46 */
0x19, 0xef,	/* gAwb_s16AdapCCMTbl_47 */
0x1A, 0xff,	/* gAwb_s16AdapCCMTbl_48 */
0x1B, 0xfa,	/* gAwb_s16AdapCCMTbl_49 */
0x1C, 0xFF,	/* gAwb_s16AdapCCMTbl_50 */
0x1D, 0xbe,	/* gAwb_s16AdapCCMTbl_51 */
0x1E, 0x00,	/* gAwb_s16AdapCCMTbl_52 */
0x1F, 0x86,	/* gAwb_s16AdapCCMTbl_53      */


/***************************************************/
/* ADF */
/***************************************************/

/* ISP setting*/
0xFF, 0xA0, /*Page mode  */
0x10, 0x80,	/* BLC: ABLC db*/
0x60, 0x73, /* CDC: Dark CDC ON  */
0x61, 0x1F, /* Six CDC_Edge En, Slash EN*/
0x69, 0x0C, /* Slash direction Line threshold*/
0x6A, 0x60, /* Slash direction Pixel threshold*/
0xC2, 0x04,	/* NSF: directional smoothing*/
0xD0, 0x51,	/* DEM: pattern detection*/
0xFF, 0xA1, /*Page mode  */
0x30, 0x01,	/* EDE: Luminane adaptation on*/
0x32, 0x50,	/* EDE: Adaptation slope */
0x34, 0x00,	/* EDE: x1 point */
0x35, 0x0B,	/* EDE: x1 point */
0x36, 0x01,	/* EDE: x2 point */
0x37, 0x80,	/* EDE: x2 point */
0x3A, 0x00,	/* EDE: Adaptation left margin*/
0x3B, 0x30,	/* EDE: Adaptation right margin*/
0x3C, 0x08,	/* EDE: rgb edge threshol*/

/* Adaptive Setting*/
0xFF, 0x85, /*Page mode  */
/* LSC*/
0x0F, 0x43,	/* LVLSC lux level threshold*/
0x10, 0x43,	/* LSLSC Light source , threshold lux*/

/* BGT*/
0x17, 0x30,	/* BGT lux level threshold*/
0x26, 0x20,	/* MinVal */
0x3c, 0x00,	/* MaxVal */

/* CNT*/
0x18, 0x43,	/* CNT lux level threshold*/
0x27, 0x00,	/* MinVal */
0x3d, 0x00,	/* MaxVal */

/* NSF */
0x12, 0xA5,	/* NSF lux level threshold */
0x22, 0x38,	/* u8MinVal_NSF1*/
0x23, 0x70,	/* u8MinVal_NSF2*/
0xFF, 0x86, /*Page mode  */
0x12, 0x00,	/* u8MinVal_NSF3*/
0xFF, 0x85, /*Page mode  */
0x38, 0x12,	/* u8MaxVal_NSF1*/
0x39, 0x30,	/* u8MaxVal_NSF2*/
0xFF, 0x86, /*Page mode  */
0x13, 0x08,	/* u8MaxVal_NSF3*/

/* GDC*/
0xFF, 0x85, /*Page mode  */
0x15, 0xF4,	/* GDC lux level threshold */
0x2D, 0x20,	/* u8MinVal_GDC1*/
0x2E, 0x30,	/* u8MinVal_GDC2*/
0x43, 0x40,	/* u8MaxVal_GDC1*/
0x44, 0x80,	/* u8MaxVal_GDC2*/

/* ISP  Edge*/
0x04, 0xFB,	/* EnEDE*/
0x14, 0x54,	/* u8ThrLevel_EDE*/
0x28, 0x00,	/* u8MinVal_EDE_CP*/
0x29, 0x03,	/* u8MinVal_EDE1*/
0x2A, 0x20,	/* u8MinVal_EDE2*/
0x2B, 0x00,	/* u8MinVal_EDE_OFS*/
0x2C, 0x22,	/* u8MinVal_SG*/
0x3E, 0x00,	/* u8MaxVal_EDE_CP*/
0x3F, 0x09,	/* u8MaxVal_EDE1*/
0x40, 0x22,	/* u8MaxVal_EDE2*/
0x41, 0x02,	/* u8MaxVal_EDE_OFS*/
0x42, 0x33,	/* u8MaxVal_SG*/

/* Gamma Adaptive*/

0x16, 0xA0,	/* Gamma Threshold*/
0x47, 0x00,	/* Min_Gamma_0*/
0x48, 0x03,	/* Min_Gamma_1*/
0x49, 0x10,	/* Min_Gamma_2*/
0x4A, 0x25,	/* Min_Gamma_3*/
0x4B, 0x3B,	/* Min_Gamma_4*/
0x4C, 0x4F,	/* Min_Gamma_5*/
0x4D, 0x6D,	/* Min_Gamma_6*/
0x4E, 0x86,	/* Min_Gamma_7*/
0x4F, 0x9B,	/* Min_Gamma_8*/
0x50, 0xAD,	/* Min_Gamma_9*/
0x51, 0xC2,	/* Min_Gamma_10*/
0x52, 0xD3,	/* Min_Gamma_11*/
0x53, 0xE1,	/* Min_Gamma_12*/
0x54, 0xE9,	/* Min_Gamma_13*/
0x55, 0xF2,	/* Min_Gamma_14*/
0x56, 0xFA,	/* Min_Gamma_15*/
0x57, 0xFF,	/* Min_Gamma_16*/
0x58, 0x00,	/* Max_Gamma_0  */
0x59, 0x06,	/* Max_Gamma_1  */
0x5a, 0x14,	/* Max_Gamma_2  */
0x5b, 0x30,	/* Max_Gamma_3  */
0x5c, 0x4A,	/* Max_Gamma_4  */
0x5d, 0x5D,	/* Max_Gamma_5  */
0x5e, 0x75,	/* Max_Gamma_6  */
0x5f, 0x89,	/* Max_Gamma_7  */
0x60, 0x9A,	/* Max_Gamma_8  */
0x61, 0xA7,	/* Max_Gamma_9  */
0x62, 0xBC,	/* Max_Gamma_10 */
0x63, 0xD0,	/* Max_Gamma_11 */
0x64, 0xE0,	/* Max_Gamma_12 */
0x65, 0xE7,	/* Max_Gamma_13 */
0x66, 0xEE,	/* Max_Gamma_14 */
0x67, 0xF7,	/* Max_Gamma_15 */
0x68, 0xFF,	/* Max_Gamma_16 */

/* Initial edge, noise filter setting*/
0xFF, 0xA0, /*Page mode  */
0xC0, 0x12,	/*NSFTh1_Addr*/
0xC1, 0x30,	/*NSFTh2_Addr*/
0xC2, 0x08,	/*NSFTh3_Addr*/
0xFF, 0xA1, /*Page mode */
0x30, 0x01,	/*EDEOption_Addr*/
0x31, 0x33,	/*EDESlopeGain_Addr*/
0x32, 0x50,	/*EDELuAdpCtrl_Addr*/
0x33, 0x00,	/*EDEPreCoringPt_Addr*/
0x34, 0x00,	/*EDETransFuncXp1_Addr1*/
0x35, 0x0B,	/*EDETransFuncXp1_Addr0*/
0x36, 0x01,	/*EDETransFuncXp2_Addr1*/
0x37, 0x80,	/*EDETransFuncXp2_Addr0*/
0x38, 0x09,	/*EDETransFuncSl1_Addr*/
0x39, 0x22,	/*EDETransFuncSl2_Addr*/
0x3A, 0x00,	/*EDELuAdpLOffset_Addr*/
0x3B, 0x30,	/*EDELuAdpROffset_Addr*/
0x3C, 0x08,	/*EDERBThrd_Addr*/
0x3D, 0x02,	/*EDESmallOffset_Addr*/

/* CCM Saturation Level*/
0xFF, 0x85, /*Page mode */
0x1A, 0x64,	/* SUP Threshold	*/
0x30, 0x40,	/* MinSUP	 */

0xFF, 0xA0, /*Page mode */
/* Lens Shading*/
0x43, 0x80,	/* RH7 rrhrhh*/
0x44, 0x80,	/* RV*/
0x45, 0x80,	/* RH	*/
0x46, 0x80,	/* RV*/
0x47, 0x80,	/* GH*/
0x48, 0x80,	/* GV*/
0x49, 0x80,	/* GH*/
0x4A, 0x80,	/* GV	*/
0x4B, 0x80,	/* BH*/
0x4C, 0x80,	/* BV*/
0x4D, 0x80,	/* BH*/
0x4E, 0x80,	/* BV  */
0x52, 0x90,	/* GGain*/
0x53, 0x20,	/* GGain*/
0x54, 0x00,	/* GGain*/

/*Max Shading*/
0xFF, 0x85, /*Page mode */
0x32, 0xC0,
0x33, 0x30,
0x34, 0x00,
0x35, 0x90,
0x36, 0x20,
0x37, 0x00,

/*Min Shading*/
0x1c,	0xC0,
0x1d,	0x30,
0x1e,	0x00,
0x1f,	0x90,
0x20,	0x18,
0x21,	0x00,

/* LineLength     */
0xFF, 0x87, /*Page mode */
0xDC, 0x05,	/* by Yong In Han 091511*/
0xDD, 0xB0,	/* by Yong In Han 091511*/
0xd5, 0x00,	/* Flip*/
/***************************************************/
/* SensorCon */
/***************************************************/
0xFF, 0xD0, /*Page mode */
0x20, 0x0E,	/* ABLK_Rsrv_Addr*/
0x20, 0x0D,	/* ABLK_Rsrv_Addr*/

/***************************************************/
/* MIPI */
/***************************************************/
0xFF, 0xB0, /*Page mode */
0x54, 0x02,	/* MIPI PHY_HS_TX_CTRL*/
0x38, 0x05,	/* MIPI DPHY_CTRL_SET*/


/* SXGA PR*/
0xFF, 0x85, /*Page mode */
0xB8, 0x0A,	/* gPT_u8PR_Active_SXGA_WORD_COUNT_Addr0*/
0xB9, 0x00,	/* gPT_u8PR_Active_SXGA_WORD_COUNT_Addr1*/
0xBC, 0x04,	/* gPT_u8PR_Active_SXGA_DPHY_CLK_TIME_Addr3*/
0xFF, 0x87, /*Page mode */
0x0C, 0x00,	/* start Y*/
0x0D, 0x20,	/* start Y   */
0x10, 0x03,	/* end Y*/
0x11, 0xE0,	/* end Y   */

/* Recoding */
0xFF, 0x86, /*Page mode */
0x38, 0x05,	/* gPT_u8PR_Active_720P_WORD_COUNT_Addr0*/
0x39, 0x00,	/* gPT_u8PR_Active_720P_WORD_COUNT_Addr1*/
0x3C, 0x04,	/* gPT_u8PR_Active_720P_DPHY_CLK_TIME_Addr3*/

0xFF, 0x87,
0x23, 0x02,	/*gPR_Active_720P_u8SensorCtrl_Addr           */
0x25, 0x01,	/*gPR_Active_720P_u8PLL_P_Addr                */
0x26, 0x0F,	/*gPR_Active_720P_u8PLL_M_Addr                */
0x27, 0x00,	/*gPR_Active_720P_u8PLL_S_Addr                */
0x28, 0x00,	/*gPR_Active_720P_u8PLL_Ctrl_Addr             */
0x29, 0x01,	/*gPR_Active_720P_u8src_clk_sel_Addr          */
0x2A, 0x00,	/*gPR_Active_720P_u8output_pad_status_Addr    */
0x2B, 0x3F,	/*gPR_Active_720P_u8ablk_ctrl_10_Addr         */
0x2C, 0xFF,	/*gPR_Active_720P_u8BayerFunc_Addr            */
0x2D, 0xFF,	/*gPR_Active_720P_u8RgbYcFunc_Addr            */
0x2E, 0x00,	/*gPR_Active_720P_u8ISPMode_Addr              */
0x2F, 0x02,	/*gPR_Active_720P_u8SCLCtrl_Addr              */
0x30, 0x01,	/*gPR_Active_720P_u8SCLHorScale_Addr0         */
0x31, 0xFF,	/*gPR_Active_720P_u8SCLHorScale_Addr1         */
0x32, 0x03,	/*gPR_Active_720P_u8SCLVerScale_Addr0         */
0x33, 0xFF,	/*gPR_Active_720P_u8SCLVerScale_Addr1         */
0x34, 0x00,	/*gPR_Active_720P_u8SCLCropStartX_Addr0       */
0x35, 0x00,	/*gPR_Active_720P_u8SCLCropStartX_Addr1       */
0x36, 0x00,	/*gPR_Active_720P_u8SCLCropStartY_Addr0       */
0x37, 0x10,	/*gPR_Active_720P_u8SCLCropStartY_Addr1       */
0x38, 0x02,	/*gPR_Active_720P_u8SCLCropEndX_Addr0         */
0x39, 0x80,	/*gPR_Active_720P_u8SCLCropEndX_Addr1         */
0x3A, 0x01,	/*gPR_Active_720P_u8SCLCropEndY_Addr0         */
0x3B, 0xF0,	/*gPR_Active_720P_u8SCLCropEndY_Addr1         */
0x3C, 0x01,	/*gPR_Active_720P_u8OutForm_Addr              */
0x3D, 0x0C,	/*gPR_Active_720P_u8OutCtrl_Addr              */
0x3E, 0x04,	/*gPR_Active_720P_u8AEWinStartX_Addr          */
0x3F, 0x04,	/*gPR_Active_720P_u8AEWinStartY_Addr          */
0x40, 0x66,	/*gPR_Active_720P_u8MergedWinWidth_Addr       */
0x41, 0x5E,	/*gPR_Active_720P_u8MergedWinHeight_Addr      */
0x42, 0x04,	/*gPR_Active_720P_u8AEHistWinAx_Addr          */
0x43, 0x04,	/*gPR_Active_720P_u8AEHistWinAy_Addr          */
0x44, 0x98,	/*gPR_Active_720P_u8AEHistWinBx_Addr          */
0x45, 0x78,	/*gPR_Active_720P_u8AEHistWinBy_Addr          */
0x46, 0x22,	/*gPR_Active_720P_u8AWBTrim_Addr              */
0x47, 0x28,	/*gPR_Active_720P_u8AWBCTWinAx_Addr           */
0x48, 0x20,	/*gPR_Active_720P_u8AWBCTWinAy_Addr           */
0x49, 0x78,	/*gPR_Active_720P_u8AWBCTWinBx_Addr           */
0x4A, 0x60,	/*gPR_Active_720P_u8AWBCTWinBy_Addr           */
0x4B, 0x03,	/*gPR_Active_720P_u16AFCFrameLength_0         */
0x4C, 0x00,	/*gPR_Active_720P_u16AFCFrameLength_1         */


/* VGA PR     */
0xFF, 0x86, /*Page mode */
0x2F, 0x05,	/* gPT_u8PR_Active_VGA_WORD_COUNT_Addr0*/
0x30, 0x00,	/* gPT_u8PR_Active_VGA_WORD_COUNT_Addr1*/
0x33, 0x04,	/* gPT_u8PR_Active_VGA_DPHY_CLK_TIME_Addr3  */

0xFF, 0x87, /*Page mode */
0x4D, 0x00,	/*gPR_Active_VGA_u8SensorCtrl_Addr*/
0x4E, 0x72,	/*gPR_Active_VGA_u8SensorMode_Addr*/
0x4F, 0x01,	/*gPR_Active_VGA_u8PLL_P_Addr*/
0x50, 0x0F,	/*gPR_Active_VGA_u8PLL_M_Addr*/
0x51, 0x00,	/*gPR_Active_VGA_u8PLL_S_Addr*/
0x52, 0x00,	/*gPR_Active_VGA_u8PLL_Ctrl_Addr*/
0x53, 0x01,	/*gPR_Active_VGA_u8src_clk_sel_Addr*/
0x54, 0x00,	/*gPR_Active_VGA_u8output_pad_status_Addr*/
0x55, 0x3F,	/*gPR_Active_VGA_u8ablk_ctrl_10_Addr*/
0x56, 0xFF,	/*gPR_Active_VGA_u8BayerFunc_Addr*/
0x57, 0xFF,	/*gPR_Active_VGA_u8RgbYcFunc_Addr*/
0x58, 0x00,	/*gPR_Active_VGA_u8ISPMode_Addr*/
0x59, 0x02,	/*gPR_Active_VGA_u8SCLCtrl_Addr*/
0x5A, 0x01,	/*gPR_Active_VGA_u8SCLHorScale_Addr0*/
0x5B, 0xFF,	/*gPR_Active_VGA_u8SCLHorScale_Addr1*/
0x5C, 0x01,	/*gPR_Active_VGA_u8SCLVerScale_Addr0*/
0x5D, 0xFF,	/*gPR_Active_VGA_u8SCLVerScale_Addr1*/
0x5E, 0x00,	/*gPR_Active_VGA_u8SCLCropStartX_Addr0*/
0x5F, 0x00,	/*gPR_Active_VGA_u8SCLCropStartX_Addr1*/
0x60, 0x00,	/*gPR_Active_VGA_u8SCLCropStartY_Addr0*/
0x61, 0x20,	/*gPR_Active_VGA_u8SCLCropStartY_Addr1*/
0x62, 0x05,	/*gPR_Active_VGA_u8SCLCropEndX_Addr0*/
0x63, 0x00,	/*gPR_Active_VGA_u8SCLCropEndX_Addr1*/
0x64, 0x03,	/*gPR_Active_VGA_u8SCLCropEndY_Addr0*/
0x65, 0xE0,	/*gPR_Active_VGA_u8SCLCropEndY_Addr1*/

0xFF, 0xd1, /*Page mode */
0x07, 0x00, /* power off mask clear*/
0x0b, 0x00, /* clock off mask clear*/
0xFF, 0xC0, /*Page mode */
0x10, 0x41,


/* VT-Call END of Initial*/
};


/* Set-data based on Samsung Reliabilty Group standard
* ,when using WIFI. 15fps
*/
static const u8 db8131m_vt_wifi_common_1[] = {
/***************************************************/
/* Device : DB8131M Fixed 8fps */
/* MIPI Interface for Noncontious Clock */
/***************************************************/
/***************************************************/
/* Command */
/***************************************************/
0xFF, 0xC0,  /*Page mode*/
0x10, 0x01,
};
/*wait	150*/

static const u8 db8131m_vt_wifi_common_2[] = {
/***************************************************/
/* Format */
/***************************************************/
0xFF, 0xA1, /*Page mode    */
0x70, 0x01,
0x71, 0x0D,

/***************************************************/
/* SensorCon */
/***************************************************/
0xFF, 0xD0, /*Page mode    */
0x0F, 0x0B,	/*ABLK_Ctrl_1_Addr*/
0x13, 0x00,	/*Gain_Addr*/
0x15, 0x01,	/*IVREFT_REFB_Addr*/
0x20, 0x0E,	/*ABLK_Rsrv_Addr*/
0x23, 0x01,	/*IVREFT2_REFB2_Addr*/
0x24, 0x01,	/*IPPG_IVCM2_Addr*/
0x39, 0x70,	/*RiseSx_CDS_1_L_Addr*/
0x51, 0x19,	/*Fallcreset_1_L_Addr*/
0x83, 0x2D,	/*RiseTran_Sig_Even_L_Addr*/
0x85, 0x2F,	/*FallTran_Sig_Even_L_Addr*/
0x87, 0x2D,	/*RiseTran_Sig_Odd_L_Addr*/
0x89, 0x2F,	/*FallTran_Sig_Odd_L_Addr*/
0x8B, 0x27,	/*RiseCNT_EN_1_L_Addr*/
0x8D, 0x6c,	/*FallCNT_EN_1_L_Addr*/
0xD7, 0x80,	/*ABLK_Ctrl_12_Addr*/
0xDB, 0xA2,	/*FallScanTx15_1_L_Addr*/
0xED, 0x01,	/*PLL_P_Addr*/
0xEE, 0x0F,	/*PLL_M_Addr*/
0xEF, 0x00,	/*PLL_S_Addr*/
0xF9, 0x00,	/*ABLK_Ctrl_8*/
0xF8, 0x00, /*Vblank Sleep Mode enable*/
0xFB, 0x90,	/*PostADC_Gain_Addr*/

/***************************************************/
/* Analog ADF */
/***************************************************/
0xFF, 0x85, /*Page mode    */
0x89, 0x93,	/*gAdf_u8APThreshold*/
0x8A, 0x0C, /*u8APClmpThreshold*/
0x8C, 0x07,	/*gAdf_u8APMinVal_ThrClampH*/
0x8D, 0x40,	/*gAdf_u8APMinVal_ThrClampL*/
0x8E, 0x03,	/*gAdf_u8APMinVal_DOFFSET*/
0x8F, 0x14,	/*gAdf_u8APMinVal_AMP2_1_SDM*/
0x91, 0x1A,	/*gAdf_u8APMinVal_AMP4_3_SDM*/
0x92, 0x0F,	/*gAdf_u8APMinVal_FallIntTx15*/
0x93, 0x47,	/*gAdf_u8APMinVal_CDSxRange_CtrlPre*/
0x95, 0x18,	/*gAdf_u8APMinVal_REFB_IVCM*/
0x96, 0x38,	/*gAdf_u8APMinVal_ref_os_PB*/
0x97, 0x0D,	/*gAdf_u8APMinVal_NTx_Range*/
0x98, 0x0D,	/*gAdf_u8APMaxVal_Clmp_rst*/
0x99, 0x06,	/*gAdf_u8APMaxVal_ThrClampH*/
0x9A, 0x9F,	/*gAdf_u8APMaxVal_ThrClampL*/
0x9B, 0x02,	/*gAdf_u8APMaxVal_DOFFSET*/
0x9C, 0x1C,	/*gAdf_u8APMaxVal_AMP2_1_SDM*/
0x9E, 0x11,	/*gAdf_u8APMaxVal_AMP4_3_SDM*/
0x9F, 0x5D,	/*gAdf_u8APMaxVal_FallIntTx15*/
0xA0, 0x78,	/*gAdf_u8APMaxVal_CDSxRange_CtrlPre*/
0xA2, 0x18,	/*gAdf_u8APMaxVal_REFB_IVCM*/
0xA3, 0x40,	/*gAdf_u8APMaxVal_ref_os_PB*/
0xA4, 0x0B,	/*gAdf_u8APMaxVal_NTx_Range*/

0xFF, 0x86, /*Page mode    */
0x15, 0x00,	/*gPT_u8Adf_APThrHys*/
0x16, 0xF7,	/*gPT_u8Adf_APFallIntTxThrLevel*/
0x17, 0x13,	/*gPT_u8Adf_APMinVal_BP2_1_SDM*/
0x18, 0x13,	/*gPT_u8Adf_APMidVal_BP2_1_SDM*/
0x19, 0x1C,	/*gPT_u8Adf_APMaxVal_BP2_1_SDM*/
0x1A, 0x06,	/*gPT_u8Adf_APMidVal_ThrClampH*/
0x1B, 0xF0,	/*gPT_u8Adf_APMidVal_ThrClampL*/
0x1C, 0x01,	/*gPT_u8Adf_APMidVal_DOFFSET*/
0x1D, 0x14,	/*gPT_u8Adf_APMidVal_AMP2_1_SDM*/
0x1F, 0x31,	/*gPT_u8Adf_APMidVal_AMP4_3_SDM*/
0x20, 0x68,	/*gPT_u8Adf_APMidVal_CDSxRange_CtrlPre*/
0x22, 0x18,	/*gPT_u8Adf_APMidVal_REFB_IVCM*/
0x23, 0x38,	/*gPT_u8Adf_APMidVal_ref_os_PB*/
0x24, 0x0F,	/*gPT_u8Adf_APMidVal_NTx_Range*/
0x25, 0x77,	/*gPT_u8Adf_APVal_EnSiSoSht_EnSm*/

0xFF, 0x87, /*Page mode    */
0xEA, 0x41,

0xFF, 0xD0, /*Page mode    */
0x20, 0x0D,	/*ABLK_Rsrv_Addr*/

0xFF, 0x83, /*Page mode  */
0x63, 0x28,    /*Again Table*/
0x64, 0x10,    /*Again Table*/
0x65, 0xA8,    /*Again Table*/
0x66, 0x50,    /*Again Table*/
0x67, 0x28,    /*Again Table*/
0x68, 0x14,    /*Again Table*/


/***************************************************/
/* AE */
/***************************************************/
0xFF, 0x82, /*Page mode  */
0x91, 0x02,	/*	AeMode*/
0x95, 0x88,	/* AE weight*/
0x96, 0x88,
0x97, 0xF8,
0x98, 0x8F,
0x99, 0xF8,
0x9A, 0x8F,
0x9B, 0x88,
0x9C, 0x88,
0xA9, 0x40,	/* OTarget*/
0xAA, 0x40,	/* ITarget*/
0x9D, 0x66,	/* AE Speed*/
0x9F, 0x06,	/* AE HoldBnd*/
0xA8, 0x40,	/* STarget*/
0xB9, 0x04,	/* RGain*/
0xBB, 0x04,	/* GGain*/
0xBD, 0x04,	/* BGain*/
0xC5, 0x02,	/* PeakMvStep*/
0xC6, 0x38,	/* PeakTgMin*/
0xC7, 0x24,	/* PeakRatioTh1*/
0xC8, 0x10,	/* PeakRatioTh0*/
0xC9, 0x05,	/* PeakLuTh*/
0xD5, 0x60,	/* LuxGainTB_2*/
0xFF, 0x83, /*Page mode  */
0x2F, 0x04,	/* TimeNum0*/
0x30, 0x05,	/* TimeNum1*/
0x4F, 0x05,	/* FrameOffset*/
0xFF, 0x82, /*Page mode  */
0xA1, 0x0A,	/* AnalogGainMax*/
0xF3, 0x09,	/* SCLK*/
0xF4, 0x60,
0xF9, 0x00,	/* GainMax*/
0xFA, 0xC8,	/* GainMax*/
0xFB, 0x62,	/* Gain3Lut*/
0xFC, 0x39,	/* Gain2Lut*/
0xFD, 0x28,	/* Gain1Lut*/
0xFE, 0x12,	/* GainMin*/
0xFF, 0x83, /*Page mode    */
0x03, 0x0F,	/* TimeMax60Hz	 : 8fps*/
0x04, 0x0A,	/* Time3Lux60Hz : 12fps*/
0x05, 0x04,	/* Time2Lut60Hz : 24fps*/
0x06, 0x04,	/* Time1Lut60Hz : 24fps*/
0xFF, 0x82, /*Page mode  */
0xD3, 0x12,	/* LuxTBGainStep0  */
0xD4, 0x36,	/* LuxTBGainStep1  */
0xD5, 0x60,	/* LuxTBGainStep2*/
0xD6, 0x01,	/* LuxTBTimeStep0H*/
0xD7, 0x00,	/* LuxTBTimeStep0L*/
0xD8, 0x01,	/* LuxTBTimeStep1H*/
0xD9, 0xC0,	/* LuxTBTimeStep1L*/
0xDA, 0x06,	/* LuxTBTimeStep2H*/
0xDB, 0x00,	/* LuxTBTimeStep2L*/
0xFF, 0x83, /*Page mode  */
0x0B, 0x08,
0x0C, 0x0C,	/* Frame Rate*/
0xFF, 0x82, /*Page mode  */
0x92, 0x5D,

/***************************************************/
/* AWB */
/***************************************************/
0xFF, 0x83, /*Page mode  */
0x79, 0x83,	/* AWB SKIN ON*/
0x86, 0x07,	/* gAWB_u16MinGrayCnt_rw_0*/
0x87, 0x00,	/* gAWB_u16MinGrayCnt_rw_1*/
0x90, 0x05,	/* gAWB_u16FinalRGain_ro_0*/
0x94, 0x05,	/* gAWB_u16FinalBGain_ro_0*/
0x98, 0xD4,	/* SkinWinCntTh*/
0xA2, 0x28,	/* SkinYTh*/
0xA3, 0x00,	/* SkinHoldHitCnt*/
0xA4, 0x0F,	/* SkinHoldHitCnt*/
0xAD, 0x65,	/* u8SkinTop2*/
0xAE, 0x80,	/* gAwb_u8SkinTop2LS1Ratio_rw  5zone                  */
0xAF, 0x20,	/* gAwb_u8SkinTop2LS2Ratio_rw  6zone */
0xB4, 0x10,	/* u8SkinTop2LSHys_rw     */
0xB5, 0x54,	/* gAwb_u8SkinLTx                              */
0xB6, 0xbd,	/* gAwb_u8SkinLTy                              */
0xB7, 0x74,	/* gAwb_u8SkinRBx                              */
0xB8, 0x9d,	/* gAwb_u8SkinRBy   */
0xBA, 0x4F,	/* UniCThrY_rw    */
0xBF, 0x0C,	/* u16UniCGrayCntThr_rw_0*/
0xC0, 0x80,	/* u16UniCGrayCntThr_rw_1               */
0xFF, 0x87, /*Page mode  */
0xC9, 0x22,	/* gUR_u8AWBTrim_Addr                          */
0xFF, 0x84, /*Page mode  */
0x49, 0x02,	/* Threshold_indoor                            */
0x4A, 0x00,
0x4B, 0x03,	/* Threshold_outdoor                           */
0x4C, 0x80,
0xFF, 0x83, /*Page mode  */
0xCB, 0x03,	/* R MinGain [Default 0X20]  A Spec Pass       */
0xCC, 0xC0,	/* R MinGain [Default 0X20]  A Spec Pass       */
0x82, 0x00,	/* lockratio*/
0xFF, 0x84, /*Page mode  */
0x3D, 0x00,	/* gAwb_u32LuxConst1_rw_0*/
0x3E, 0x00,	/* gAwb_u32LuxConst1_rw_1*/
0x3F, 0x06,	/* gAwb_u32LuxConst1_rw_2*/
0x40, 0x20,	/* gAwb_u32LuxConst1_rw_3*/
0x41, 0x07,	/* gAwb_u32LuxConst2_rw_0*/
0x42, 0x53,	/* gAwb_u32LuxConst2_rw_1*/
0x43, 0x00,	/* gAwb_u32LuxConst2_rw_2*/
0x44, 0x00,	/* gAwb_u32LuxConst2_rw_3*/
0x55, 0x03,	/* gAwb_u8Weight_Gen_rw_0	*/
0x56, 0x10,	/* gAwb_u8Weight_Gen_rw_1	*/
0x57, 0x14,	/* gAwb_u8Weight_Gen_rw_2	*/
0x58, 0x07,	/* gAwb_u8Weight_Gen_rw_3	*/
0x59, 0x04,	/* gAwb_u8Weight_Gen_rw_4	*/
0x5A, 0x03,	/* gAwb_u8Weight_Gen_rw_5	*/
0x5B, 0x03,	/* gAwb_u8Weight_Gen_rw_6	*/
0x5C, 0x15,	/* gAwb_u8Weight_Gen_rw_7	*/
0x5D, 0x01,	/* gAwb_u8Weight_Ind_rw_0	*/
0x5E, 0x0F,	/* gAwb_u8Weight_Ind_rw_1	*/
0x5F, 0x07,	/* gAwb_u8Weight_Ind_rw_2	*/
0x60, 0x14,	/* gAwb_u8Weight_Ind_rw_3	*/
0x61, 0x14,	/* gAwb_u8Weight_Ind_rw_4	*/
0x62, 0x12,	/* gAwb_u8Weight_Ind_rw_5	*/
0x63, 0x11,	/* gAwb_u8Weight_Ind_rw_6	*/
0x64, 0x14,	/* gAwb_u8Weight_Ind_rw_7	*/
0x65, 0x03,	/* gAwb_u8Weight_Outd_rw_0*/
0x66, 0x05,	/* gAwb_u8Weight_Outd_rw_1*/
0x67, 0x15,	/* gAwb_u8Weight_Outd_rw_2*/
0x68, 0x04,	/* gAwb_u8Weight_Outd_rw_3*/
0x69, 0x01,	/* gAwb_u8Weight_Outd_rw_4*/
0x6A, 0x02,	/* gAwb_u8Weight_Outd_rw_5*/
0x6B, 0x03,	/* gAwb_u8Weight_Outd_rw_6*/
0x6C, 0x15,	/* gAwb_u8Weight_Outd_rw_6*/
0xFF, 0x85, /*Page mode  */
0xE2, 0x0C,	/* gPT_u8Awb_UnicolorZone_rw */
0xFF, 0x83, /*Page mode  */
0xCD, 0x06,	/*Max Rgain*/
0xCE, 0x80,
0xD1, 0x06,	/*Max BGain*/
0xd2, 0x80,

/***************************************************/
/* AWB STE */
/***************************************************/
0xFF, 0xA1, /*Page mode  */
/*Flash*/
0xA0, 0x5c,	/*AWBZone0LTx*/
0xA1, 0x7a,	/*AWBZone0LTy*/
0xA2, 0x69,	/*AWBZone0RBx*/
0xA3, 0x6f,	/*AWBZone0RBy*/
/*cloudy*/
0xA4, 0x73,	/*AWBZone1LTx*/
0xA5, 0x55,	/*AWBZone1LTy*/
0xA6, 0x8C,	/*AWBZone1RBx*/
0xA7, 0x30,	/*AWBZone1RBy */
/*Daylight      */
0xA8, 0x69,	/*AWBZone2LTx*/
0xA9, 0x69,	/*AWBZone2LTy*/
0xAA, 0x83,	/*AWBZone2RBx*/
0xAB, 0x52,	/*AWBZone2RBy */
/*Fluorescent   */
0xAC, 0x57,	/*AWBZone3LTx*/
0xAD, 0x6e,	/*AWBZone3LTy*/
0xAE, 0x6f,	/*AWBZone3RBx*/
0xAF, 0x59,	/*AWBZone3RBy*/

/*CWF           */
0xB0, 0x50,	/*AWBZone4LTx*/
0xB1, 0x74,	/*AWBZone4LTy*/
0xB2, 0x65,	/*AWBZone4RBx*/
0xB3, 0x5d,	/*AWBZone4RBy*/
/*TL84          */
0xB4, 0x53,	/*AWBZone5LTx*/
0xB5, 0x7f,	/*AWBZone5LTy*/
0xB6, 0x62,	/*AWBZone5RBx*/
0xB7, 0x75,	/*AWBZone5RBy */
/*A             */
0xB8, 0x4a,	/*AWBZone6LTx*/
0xB9, 0x87,	/*AWBZone6LTy*/
0xBA, 0x59,	/*AWBZone6RBx*/
0xBB, 0x78,	/*AWBZone6RBy*/
/*Horizon       */
0xBC, 0x41,	/*AWBZone7LTx*/
0xBD, 0x91,	/*AWBZone7LTy*/
0xBE, 0x4b,	/*AWBZone7RBx*/
0xBF, 0x89,	/*AWBZone7RBy*/
/*Skin          */
0xC0, 0x5b,	/*AWBZone8LTx*/
0xC1, 0x85,	/*AWBZone8LTy*/
0xC2, 0x60,	/*AWBZone8RBx*/
0xC3, 0x7b,	/*AWBZone8RBy*/

/***************************************************/
/* UR */
/***************************************************/
0xFF, 0x85, /*Page mode  */
0x06, 0x05,
0xFF, 0x86, /*Page mode  */
0x14, 0x1E,	/* CCM sum 1*/
0xFF, 0x85, /*Page mode  */
0x86, 0x42,	/* 42 saturation level*/
0x07, 0x00,	/* sup hysteresis*/

/*DAY light     */
0xFF, 0x83, /*Page mode  */
0xEA, 0x00,	/*gAwb_s16AdapCCMTbl_0*/
0xEB, 0x53,	/*gAwb_s16AdapCCMTbl_1*/
0xEC, 0xFF,	/*gAwb_s16AdapCCMTbl_2*/
0xED, 0xE1,	/*gAwb_s16AdapCCMTbl_3*/
0xEE, 0x00,	/*gAwb_s16AdapCCMTbl_4*/
0xEF, 0x05,	/*gAwb_s16AdapCCMTbl_5*/
0xF0, 0xFF,	/*gAwb_s16AdapCCMTbl_6*/
0xF1, 0xF3,	/*gAwb_s16AdapCCMTbl_7*/
0xF2, 0x00,	/*gAwb_s16AdapCCMTbl_8*/
0xF3, 0x4B,	/*gAwb_s16AdapCCMTbl_9*/
0xF4, 0xFF,	/*gAwb_s16AdapCCMTbl_10*/
0xF5, 0xFA,	/*gAwb_s16AdapCCMTbl_11*/
0xF6, 0xFF,	/*gAwb_s16AdapCCMTbl_12*/
0xF7, 0xFa,	/*gAwb_s16AdapCCMTbl_13*/
0xF8, 0xFF,	/*gAwb_s16AdapCCMTbl_14*/
0xF9, 0xC3,	/*gAwb_s16AdapCCMTbl_15*/
0xFA, 0x00,	/*gAwb_s16AdapCCMTbl_16*/
0xFB, 0x80,	/*gAwb_s16AdapCCMTbl_17*/

/*CWF lgiht    */
0xFF, 0x83, /*Page mode                                      */
0xFC, 0x00,	/* gAwb_s16AdapCCMTbl_18   */
0xFD, 0x68,	/* gAwb_s16AdapCCMTbl_19   */
0xFF, 0x85, /*Page mode  */
0xE0, 0xFF,	/* gAwb_s16AdapCCMTbl_20   */
0xE1, 0xde,	/* gAwb_s16AdapCCMTbl_21   */
0xFF, 0x84, /*Page mode  */
0x00, 0xff,	/* gAwb_s16AdapCCMTbl_22   */
0x01, 0xfa,	/* gAwb_s16AdapCCMTbl_23   */
0x02, 0xFF,	/* gAwb_s16AdapCCMTbl_24   */
0x03, 0xf0,	/* gAwb_s16AdapCCMTbl_25   */
0x04, 0x00,	/* gAwb_s16AdapCCMTbl_26   */
0x05, 0x52,	/* gAwb_s16AdapCCMTbl_27   */
0x06, 0xFF,	/* gAwb_s16AdapCCMTbl_28   */
0x07, 0xFa,	/* gAwb_s16AdapCCMTbl_29   */
0x08, 0x00,	/* gAwb_s16AdapCCMTbl_30   */
0x09, 0x00,	/* gAwb_s16AdapCCMTbl_31   */
0x0A, 0xFF,	/* gAwb_s16AdapCCMTbl_32   */
0x0B, 0xdb,	/* gAwb_s16AdapCCMTbl_33   */
0x0C, 0x00,	/* gAwb_s16AdapCCMTbl_34   */
0x0D, 0x68,	/* gAwb_s16AdapCCMTbl_35 */

/*A light */

0x0E, 0x00,	/* gAwb_s16AdapCCMTbl_36 */
0x0F, 0x6d,	/* gAwb_s16AdapCCMTbl_37 */
0x10, 0xFF,	/* gAwb_s16AdapCCMTbl_38 */
0x11, 0xd5,	/* gAwb_s16AdapCCMTbl_39 */
0x12, 0xff,	/* gAwb_s16AdapCCMTbl_40 */
0x13, 0xfe,	/* gAwb_s16AdapCCMTbl_41 */
0x14, 0xFF,	/* gAwb_s16AdapCCMTbl_42 */
0x15, 0xf4,	/* gAwb_s16AdapCCMTbl_43 */
0x16, 0x00,	/* gAwb_s16AdapCCMTbl_44 */
0x17, 0x5a,	/* gAwb_s16AdapCCMTbl_45 */
0x18, 0xff,	/* gAwb_s16AdapCCMTbl_46 */
0x19, 0xef,	/* gAwb_s16AdapCCMTbl_47 */
0x1A, 0xff,	/* gAwb_s16AdapCCMTbl_48 */
0x1B, 0xfa,	/* gAwb_s16AdapCCMTbl_49 */
0x1C, 0xFF,	/* gAwb_s16AdapCCMTbl_50 */
0x1D, 0xbe,	/* gAwb_s16AdapCCMTbl_51 */
0x1E, 0x00,	/* gAwb_s16AdapCCMTbl_52 */
0x1F, 0x86,	/* gAwb_s16AdapCCMTbl_53      */


/***************************************************/
/* ADF */
/***************************************************/

/* ISP setting*/
0xFF, 0xA0, /*Page mode  */
0x10, 0x80,	/* BLC: ABLC db*/
0x60, 0x73, /* CDC: Dark CDC ON  */
0x61, 0x1F, /* Six CDC_Edge En, Slash EN*/
0x69, 0x0C, /* Slash direction Line threshold*/
0x6A, 0x60, /* Slash direction Pixel threshold*/
0xC2, 0x04,	/* NSF: directional smoothing*/
0xD0, 0x51,	/* DEM: pattern detection*/
0xFF, 0xA1, /*Page mode  */
0x30, 0x01,	/* EDE: Luminane adaptation on*/
0x32, 0x50,	/* EDE: Adaptation slope */
0x34, 0x00,	/* EDE: x1 point */
0x35, 0x0B,	/* EDE: x1 point */
0x36, 0x01,	/* EDE: x2 point */
0x37, 0x80,	/* EDE: x2 point */
0x3A, 0x00,	/* EDE: Adaptation left margin*/
0x3B, 0x30,	/* EDE: Adaptation right margin*/
0x3C, 0x08,	/* EDE: rgb edge threshol*/

/* Adaptive Setting*/
0xFF, 0x85, /*Page mode  */
/* LSC*/
0x0F, 0x43,	/* LVLSC lux level threshold*/
0x10, 0x43,	/* LSLSC Light source , threshold lux*/

/* BGT*/
0x17, 0x30,  /* BGT lux level threshold*/
0x26, 0x20,  /* MinVal */
0x3c, 0x00,  /* MaxVal */

/* CNT*/
0x18, 0x43,	/* CNT lux level threshold*/
0x27, 0x00,	/* MinVal */
0x3d, 0x00,	/* MaxVal */

/* NSF */
0x12, 0xA5,	/* NSF lux level threshold */
0x22, 0x38,	/* u8MinVal_NSF1*/
0x23, 0x70,	/* u8MinVal_NSF2*/
0xFF, 0x86, /*Page mode  */
0x12, 0x00,	/* u8MinVal_NSF3*/
0xFF, 0x85, /*Page mode  */
0x38, 0x12,	/* u8MaxVal_NSF1*/
0x39, 0x30,	/* u8MaxVal_NSF2*/
0xFF, 0x86, /*Page mode  */
0x13, 0x08,	/* u8MaxVal_NSF3*/

/* GDC*/
0xFF, 0x85, /*Page mode  */
0x15, 0xF4,	/* GDC lux level threshold */
0x2D, 0x20,	/* u8MinVal_GDC1*/
0x2E, 0x30,	/* u8MinVal_GDC2*/
0x43, 0x40,	/* u8MaxVal_GDC1*/
0x44, 0x80,	/* u8MaxVal_GDC2*/

/* ISP  Edge*/
0x04, 0xFB,	/* EnEDE*/
0x14, 0x54,	/* u8ThrLevel_EDE*/
0x28, 0x00,	/* u8MinVal_EDE_CP*/
0x29, 0x03,	/* u8MinVal_EDE1*/
0x2A, 0x20,	/* u8MinVal_EDE2*/
0x2B, 0x00,	/* u8MinVal_EDE_OFS*/
0x2C, 0x22,	/* u8MinVal_SG*/
0x3E, 0x00,	/* u8MaxVal_EDE_CP*/
0x3F, 0x09,	/* u8MaxVal_EDE1*/
0x40, 0x22,	/* u8MaxVal_EDE2*/
0x41, 0x02,	/* u8MaxVal_EDE_OFS*/
0x42, 0x33,	/* u8MaxVal_SG*/

/* Gamma Adaptive*/

0x16, 0xA0,	/* Gamma Threshold*/
0x47, 0x00,	/* Min_Gamma_0*/
0x48, 0x03,	/* Min_Gamma_1*/
0x49, 0x10,	/* Min_Gamma_2*/
0x4A, 0x25,	/* Min_Gamma_3*/
0x4B, 0x3B,	/* Min_Gamma_4*/
0x4C, 0x4F,	/* Min_Gamma_5*/
0x4D, 0x6D,	/* Min_Gamma_6*/
0x4E, 0x86,	/* Min_Gamma_7*/
0x4F, 0x9B,	/* Min_Gamma_8*/
0x50, 0xAD,	/* Min_Gamma_9*/
0x51, 0xC2,	/* Min_Gamma_10*/
0x52, 0xD3,	/* Min_Gamma_11*/
0x53, 0xE1,	/* Min_Gamma_12*/
0x54, 0xE9,	/* Min_Gamma_13*/
0x55, 0xF2,	/* Min_Gamma_14*/
0x56, 0xFA,	/* Min_Gamma_15*/
0x57, 0xFF,	/* Min_Gamma_16*/
0x58, 0x00,	/* Max_Gamma_0  */
0x59, 0x06,	/* Max_Gamma_1  */
0x5a, 0x14,	/* Max_Gamma_2  */
0x5b, 0x30,	/* Max_Gamma_3  */
0x5c, 0x4A,	/* Max_Gamma_4  */
0x5d, 0x5D,	/* Max_Gamma_5  */
0x5e, 0x75,	/* Max_Gamma_6  */
0x5f, 0x89,	/* Max_Gamma_7  */
0x60, 0x9A,	/* Max_Gamma_8  */
0x61, 0xA7,	/* Max_Gamma_9  */
0x62, 0xBC,	/* Max_Gamma_10 */
0x63, 0xD0,	/* Max_Gamma_11 */
0x64, 0xE0,	/* Max_Gamma_12 */
0x65, 0xE7,	/* Max_Gamma_13 */
0x66, 0xEE,	/* Max_Gamma_14 */
0x67, 0xF7,	/* Max_Gamma_15 */
0x68, 0xFF,	/* Max_Gamma_16 */

/* Initial edge, noise filter setting*/
0xFF, 0xA0, /*Page mode  */
0xC0, 0x12,	/*NSFTh1_Addr*/
0xC1, 0x30,	/*NSFTh2_Addr*/
0xC2, 0x08,	/*NSFTh3_Addr*/
0xFF, 0xA1, /*Page mode */
0x30, 0x01,	/*EDEOption_Addr*/
0x31, 0x33,	/*EDESlopeGain_Addr*/
0x32, 0x50,	/*EDELuAdpCtrl_Addr*/
0x33, 0x00,	/*EDEPreCoringPt_Addr*/
0x34, 0x00,	/*EDETransFuncXp1_Addr1*/
0x35, 0x0B,	/*EDETransFuncXp1_Addr0*/
0x36, 0x01,	/*EDETransFuncXp2_Addr1*/
0x37, 0x80,	/*EDETransFuncXp2_Addr0*/
0x38, 0x09,	/*EDETransFuncSl1_Addr*/
0x39, 0x22,	/*EDETransFuncSl2_Addr*/
0x3A, 0x00,	/*EDELuAdpLOffset_Addr*/
0x3B, 0x30,	/*EDELuAdpROffset_Addr*/
0x3C, 0x08,	/*EDERBThrd_Addr*/
0x3D, 0x02,	/*EDESmallOffset_Addr*/


/* CCM Saturation Level*/
0xFF, 0x85, /*Page mode */
0x1A, 0x64,	/* SUP Threshold	*/
0x30, 0x40,	/* MinSUP	 */

0xFF, 0xA0, /*Page mode */
/* Lens Shading*/
0x43, 0x80,	/* RH7 rrhrhh*/
0x44, 0x80,	/* RV*/
0x45, 0x80,	/* RH	*/
0x46, 0x80,	/* RV*/
0x47, 0x80,	/* GH*/
0x48, 0x80,	/* GV*/
0x49, 0x80,	/* GH*/
0x4A, 0x80,	/* GV	*/
0x4B, 0x80,	/* BH*/
0x4C, 0x80,	/* BV*/
0x4D, 0x80,	/* BH*/
0x4E, 0x80,	/* BV  */
0x52, 0x90,	/* GGain*/
0x53, 0x20,	/* GGain*/
0x54, 0x00,	/* GGain*/

/*Max Shading*/
0xFF, 0x85, /*Page mode */
0x32, 0xC0,
0x33, 0x30,
0x34, 0x00,
0x35, 0x90,
0x36, 0x20,
0x37, 0x00,

/*Min Shading*/
0x1c, 0xC0,
0x1d, 0x30,
0x1e, 0x00,
0x1f, 0x90,
0x20, 0x18,
0x21, 0x00,

/* LineLength     */
0xFF, 0x87, /*Page mode */
0xDC, 0x05,	/* by Yong In Han 091511*/
0xDD, 0xB0,	/* by Yong In Han 091511*/
0xd5, 0x00,	/* Flip*/
/***************************************************/
/* SensorCon */
/***************************************************/
0xFF, 0xD0, /*Page mode */
0x20, 0x0E,	/* ABLK_Rsrv_Addr*/
0x20, 0x0D,	/* ABLK_Rsrv_Addr*/

/***************************************************/
/* MIPI */
/***************************************************/
0xFF, 0xB0, /*Page mode */
0x54, 0x02,	/* MIPI PHY_HS_TX_CTRL*/
0x38, 0x05,	/* MIPI DPHY_CTRL_SET*/


/* SXGA PR*/
0xFF, 0x85, /*Page mode */
0xB8, 0x0A,  /* gPT_u8PR_Active_SXGA_WORD_COUNT_Addr0*/
0xB9, 0x00,  /* gPT_u8PR_Active_SXGA_WORD_COUNT_Addr1*/
0xBC, 0x04,  /* gPT_u8PR_Active_SXGA_DPHY_CLK_TIME_Addr3*/
0xFF, 0x87, /*Page mode */
0x0C, 0x00,  /* start Y*/
0x0D, 0x20,  /* start Y   */
0x10, 0x03,  /* end Y*/
0x11, 0xE0,  /* end Y   */

/* Recoding */
0xFF, 0x86, /*Page mode */
0x38, 0x05,  /* gPT_u8PR_Active_720P_WORD_COUNT_Addr0*/
0x39, 0x00,  /* gPT_u8PR_Active_720P_WORD_COUNT_Addr1*/
0x3C, 0x04,  /* gPT_u8PR_Active_720P_DPHY_CLK_TIME_Addr3*/

0xFF, 0x87,
0x23, 0x02,	/*gPR_Active_720P_u8SensorCtrl_Addr           */
0x25, 0x01,	/*gPR_Active_720P_u8PLL_P_Addr                */
0x26, 0x0F,	/*gPR_Active_720P_u8PLL_M_Addr                */
0x27, 0x00,	/*gPR_Active_720P_u8PLL_S_Addr                */
0x28, 0x00,	/*gPR_Active_720P_u8PLL_Ctrl_Addr             */
0x29, 0x01,	/*gPR_Active_720P_u8src_clk_sel_Addr          */
0x2A, 0x00,	/*gPR_Active_720P_u8output_pad_status_Addr    */
0x2B, 0x3F,	/*gPR_Active_720P_u8ablk_ctrl_10_Addr         */
0x2C, 0xFF,	/*gPR_Active_720P_u8BayerFunc_Addr            */
0x2D, 0xFF,	/*gPR_Active_720P_u8RgbYcFunc_Addr            */
0x2E, 0x00,	/*gPR_Active_720P_u8ISPMode_Addr              */
0x2F, 0x02,	/*gPR_Active_720P_u8SCLCtrl_Addr              */
0x30, 0x01,	/*gPR_Active_720P_u8SCLHorScale_Addr0         */
0x31, 0xFF,	/*gPR_Active_720P_u8SCLHorScale_Addr1         */
0x32, 0x03,	/*gPR_Active_720P_u8SCLVerScale_Addr0         */
0x33, 0xFF,	/*gPR_Active_720P_u8SCLVerScale_Addr1         */
0x34, 0x00,	/*gPR_Active_720P_u8SCLCropStartX_Addr0       */
0x35, 0x00,	/*gPR_Active_720P_u8SCLCropStartX_Addr1       */
0x36, 0x00,	/*gPR_Active_720P_u8SCLCropStartY_Addr0       */
0x37, 0x10,	/*gPR_Active_720P_u8SCLCropStartY_Addr1       */
0x38, 0x02,	/*gPR_Active_720P_u8SCLCropEndX_Addr0         */
0x39, 0x80,	/*gPR_Active_720P_u8SCLCropEndX_Addr1         */
0x3A, 0x01,	/*gPR_Active_720P_u8SCLCropEndY_Addr0         */
0x3B, 0xF0,	/*gPR_Active_720P_u8SCLCropEndY_Addr1         */
0x3C, 0x01,	/*gPR_Active_720P_u8OutForm_Addr              */
0x3D, 0x0C,	/*gPR_Active_720P_u8OutCtrl_Addr              */
0x3E, 0x04,	/*gPR_Active_720P_u8AEWinStartX_Addr          */
0x3F, 0x04,	/*gPR_Active_720P_u8AEWinStartY_Addr          */
0x40, 0x66,	/*gPR_Active_720P_u8MergedWinWidth_Addr       */
0x41, 0x5E,	/*gPR_Active_720P_u8MergedWinHeight_Addr      */
0x42, 0x04,	/*gPR_Active_720P_u8AEHistWinAx_Addr          */
0x43, 0x04,	/*gPR_Active_720P_u8AEHistWinAy_Addr          */
0x44, 0x98,	/*gPR_Active_720P_u8AEHistWinBx_Addr          */
0x45, 0x78,	/*gPR_Active_720P_u8AEHistWinBy_Addr          */
0x46, 0x22,	/*gPR_Active_720P_u8AWBTrim_Addr              */
0x47, 0x28,	/*gPR_Active_720P_u8AWBCTWinAx_Addr           */
0x48, 0x20,	/*gPR_Active_720P_u8AWBCTWinAy_Addr           */
0x49, 0x78,	/*gPR_Active_720P_u8AWBCTWinBx_Addr           */
0x4A, 0x60,	/*gPR_Active_720P_u8AWBCTWinBy_Addr           */
0x4B, 0x03,	/*gPR_Active_720P_u16AFCFrameLength_0         */
0x4C, 0x00,	/*gPR_Active_720P_u16AFCFrameLength_1         */


/* VGA PR     */
0xFF, 0x86, /*Page mode */
0x2F, 0x05,  /* gPT_u8PR_Active_VGA_WORD_COUNT_Addr0*/
0x30, 0x00,  /* gPT_u8PR_Active_VGA_WORD_COUNT_Addr1*/
0x33, 0x04,  /* gPT_u8PR_Active_VGA_DPHY_CLK_TIME_Addr3*/

0xFF, 0x87, /*Page mode */
0x4D, 0x00,	/*gPR_Active_VGA_u8SensorCtrl_Addr*/
0x4E, 0x72,	/*gPR_Active_VGA_u8SensorMode_Addr*/
0x4F, 0x01,	/*gPR_Active_VGA_u8PLL_P_Addr*/
0x50, 0x0F,	/*gPR_Active_VGA_u8PLL_M_Addr*/
0x51, 0x00,	/*gPR_Active_VGA_u8PLL_S_Addr*/
0x52, 0x00,	/*gPR_Active_VGA_u8PLL_Ctrl_Addr*/
0x53, 0x01,	/*gPR_Active_VGA_u8src_clk_sel_Addr*/
0x54, 0x00,	/*gPR_Active_VGA_u8output_pad_status_Addr*/
0x55, 0x3F,	/*gPR_Active_VGA_u8ablk_ctrl_10_Addr*/
0x56, 0xFF,	/*gPR_Active_VGA_u8BayerFunc_Addr*/
0x57, 0xFF,	/*gPR_Active_VGA_u8RgbYcFunc_Addr*/
0x58, 0x00,	/*gPR_Active_VGA_u8ISPMode_Addr*/
0x59, 0x02,	/*gPR_Active_VGA_u8SCLCtrl_Addr*/
0x5A, 0x01,	/*gPR_Active_VGA_u8SCLHorScale_Addr0*/
0x5B, 0xFF,	/*gPR_Active_VGA_u8SCLHorScale_Addr1*/
0x5C, 0x01,	/*gPR_Active_VGA_u8SCLVerScale_Addr0*/
0x5D, 0xFF,	/*gPR_Active_VGA_u8SCLVerScale_Addr1*/
0x5E, 0x00,	/*gPR_Active_VGA_u8SCLCropStartX_Addr0*/
0x5F, 0x00,	/*gPR_Active_VGA_u8SCLCropStartX_Addr1*/
0x60, 0x00,	/*gPR_Active_VGA_u8SCLCropStartY_Addr0*/
0x61, 0x20,	/*gPR_Active_VGA_u8SCLCropStartY_Addr1*/
0x62, 0x05,	/*gPR_Active_VGA_u8SCLCropEndX_Addr0*/
0x63, 0x00,	/*gPR_Active_VGA_u8SCLCropEndX_Addr1*/
0x64, 0x03,	/*gPR_Active_VGA_u8SCLCropEndY_Addr0*/
0x65, 0xE0,	/*gPR_Active_VGA_u8SCLCropEndY_Addr1*/

0xFF, 0xd1, /*Page mode */
0x07, 0x00, /* power off mask clear*/
0x0b, 0x00, /* clock off mask clear*/
0xFF, 0xC0, /*Page mode */
0x10, 0x41,

/* Wifi VT-Call END of Initial*/
};

/***************************************************/
/* CAMERA_PREVIEW - ÃÔ¿µ ÈÄ ÇÁ¸®ºä º¹±Í½Ã ¼ÂÆÃ */
/***************************************************/

static const u8 db8131m_preview[] = {
0xff, 0x82,
0x7F, 0x35,
};

/***************************************************/
/* CAMERA_SNAPSHOT - ÃÔ¿µ */
/***************************************************/
static const u8 db8131m_capture[] = {
0xff, 0x82,
0x7F, 0x34,
0xff, 0xC0,
0x10, 0x03,
};
/*Wait 150ms*/
/* capture ½ÇÇà*/

/***************************************************/
/*	CAMERA_RECORDING WITH 25fps  */
/***************************************************/

static const u8 db8131m_recording_60Hz_common[] = {
/***************************************************/
/* Device : DB8131M */
/* MIPI Interface for Noncontious Clock */
/***************************************************/

/* Recording Anti-Flicker 60Hz END of Initial */
0xFF, 0x87,
0xDE, 0x7A,	/*gPR_Active_720P_u8SensorMode_Addr   */
0xFF, 0xC0, /* Page mode*/
0x10, 0x42,	/*	Preview Command*/

/* Fixed 25fps Mode*/
0xFF, 0x82, /* Page mode*/
0x91, 0x02,	/*	AeMode*/
0xFF, 0x83, /* Page mode*/
0x0B, 0x02,	/*	Frame Rate*/
0x0C, 0x94,	/*	Frame Rate*/
0x03, 0x04,	/*	TimeMax60Hz*/
0x04, 0x03,	/*	Time3Lux60Hz*/
0x05, 0x02,	/*	Time2Lut60Hz*/
0x06, 0x01,	/*	Time1Lut60Hz*/
0xFF, 0x82, /* Page mode*/
0x92, 0x5D,
};


static const u8 db8131m_stream_stop[] = {

};


/***************************************************/
/*	CAMERA_BRIGHTNESS_1 (1/9) M4 */
/***************************************************/
static const u8 db8131m_bright_m4[] = {
/* Brightness -4 */
0xFF, 0x87, /* Page mode*/
0xAE, 0xE0,	/* Brightness*/
};

/***************************************************/
/*	CAMERA_BRIGHTNESS_2 (2/9) M3 */
/***************************************************/

static const u8 db8131m_bright_m3[] = {
/* Brightness -3 */
0xFF, 0x87, /* Page mode*/
0xAE, 0xE8,	/* Brightness*/
};

/***************************************************/
/*	CAMERA_BRIGHTNESS_3 (3/9) M2 */
/***************************************************/
static const u8 db8131m_bright_m2[] = {
/* Brightness -2 */
0xFF, 0x87, /* Page mode*/
0xAE, 0xF0,	/* Brightness*/
};

/***************************************************/
/*	CAMERA_BRIGHTNESS_4 (4/9) M1 */
/***************************************************/

static const u8 db8131m_bright_m1[] = {
/* Brightness -1 */
0xFF, 0x87, /* Page mode*/
0xAE, 0xF8,	/* Brightness*/
};

/***************************************************/
/*	CAMERA_BRIGHTNESS_5 (5/9) Default */
/***************************************************/
static const u8 db8131m_bright_default[] = {
/* Brightness 0 */
0xFF, 0x87, /* Page mode*/
0xAE, 0x00,	/* Brightness*/
};

/***************************************************/
/*	CAMERA_BRIGHTNESS_6 (6/9) P1 */
/***************************************************/
static const u8 db8131m_bright_p1[] = {
/* Brightness +1 */
0xFF, 0x87, /* Page mode*/
0xAE, 0x08,	/* Brightness*/
};

/***************************************************/
/*	CAMERA_BRIGHTNESS_7 (7/9) P2 */
/***************************************************/
static const u8 db8131m_bright_p2[] = {
/* Brightness +2 */
0xFF, 0x87, /* Page mode*/
0xAE, 0x10,	/* Brightness*/
};

/***************************************************/
/*	CAMERA_BRIGHTNESS_8 (8/9) P3 */
/***************************************************/
static const u8 db8131m_bright_p3[] = {
/* Brightness +3 */
0xFF, 0x87, /* Page mode*/
0xAE, 0x18,	/* Brightness*/
};

/***************************************************/
/*	CAMERA_BRIGHTNESS_9 (9/9) P4 */
/***************************************************/
static const u8 db8131m_bright_p4[] = {
/* Brightness +4 */
0xFF, 0x87, /* Page mode*/
0xAE, 0x20,	/* Brightness*/
};


static const u8 db8131m_vt_7fps[] = {
/* Fixed 7fps Mode*/
0xFF, 0x82, /* Page mode*/
0x91, 0x02,	/*	AeMode*/
0xFF, 0x83, /* Page mode*/
0x0B, 0x09,	/*	Frame Rate*/
0x0C, 0x33,	/*	Frame Rate*/
0x03, 0x0F,	/*	TimeMax60Hz*/
0x04, 0x0A,	/*	Time3Lux60Hz*/
0x05, 0x06,	/*	Time2Lut60Hz*/
0x06, 0x04,	/*	Time1Lut60Hz*/
0xFF, 0x82, /* Page mode*/
0x92, 0x5D,
};

static const u8 db8131m_vt_10fps[] = {
/* Fixed 10fps Mode */
0xFF, 0x82,	/* Page mode*/
0x91, 0x02, /*	AeMode*/
0xFF, 0x83,	/* Page mode*/
0x0B, 0x06, /*	Frame Rate*/
0x0C, 0x70, /*	Frame Rate*/
0x03, 0x0A, /*	TimeMax60Hz*/
0x04, 0x08, /*	Time3Lux60Hz*/
0x05, 0x06, /*	Time2Lut60Hz*/
0x06, 0x04, /*	Time1Lut60Hz*/
0xFF, 0x82,	/* Page mode*/
0x92, 0x5D,

};

static const u8 db8131m_vt_12fps[] = {
/* Fixed 12fps Mode */
0xFF, 0x82,	/* Page mode*/
0x91, 0x02, /*	AeMode*/
0xFF, 0x83,	/* Page mode*/
0x0B, 0x05, /*	Frame Rate*/
0x0C, 0x5E, /*	Frame Rate*/
0x03, 0x0C, /*	TimeMax60Hz*/
0x04, 0x0A, /*	Time3Lux60Hz*/
0x05, 0x06, /*	Time2Lut60Hz*/
0x06, 0x04, /*	Time1Lut60Hz*/
0xFF, 0x82,	/* Page mode*/
0x92, 0x5D,
};

static const u8 db8131m_vt_15fps[] = {
/* Fixed 15fps Mode */
0xFF, 0x82,	/* Page mode*/
0x91, 0x02, /*	AeMode*/
0xFF, 0x83,	/* Page mode*/
0x0B, 0x04, /*	Frame Rate*/
0x0C, 0x4C, /*	Frame Rate*/
0x03, 0x08, /*	TimeMax60Hz*/
0x04, 0x06, /*	Time3Lux60Hz*/
0x05, 0x04, /*	Time2Lut60Hz*/
0x06, 0x04, /*	Time1Lut60Hz*/
0xFF, 0x82,	/* Page mode*/
0x92, 0x5D,

};

/***************************************************/
/*	CAMERA_DTP_ON */
/***************************************************/
static const u8 db8131m_pattern_on[] = {
0xFF, 0x87, /* Page mode*/
0xAB, 0x00, /* BayerFunc*/
0xAC, 0x28, /* RGBYcFunc*/
0xFF, 0xA0, /* Page mode*/
0x02, 0x05, /* TPG ? Gamma*/

};

/***************************************************/
/*	CAMERA_DTP_OFF */
/***************************************************/
static const u8 db8131m_pattern_off[] = {
0xFF, 0x87, /* Page mode*/
0xAB, 0xFF, /* BayerFunc*/
0xAC, 0xFF, /* RGBYcFunc*/
0xFF, 0xA0, /* Page mode*/
0x02, 0x00, /* TPG Disable*/

};
#endif /* __DB8131M_SETFILE_H */
