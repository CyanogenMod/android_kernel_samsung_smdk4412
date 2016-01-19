/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __db8131m_REG_H
#define __db8131m_REG_H

/* =================================================================*/
/* Name     : db8131m Module                                */
/* Version  :                                               */
/* PLL mode : MCLK - 24MHz                                  */
/* fPS      :                                               */
/* PRVIEW   : 640*`480                                       */
/* Made by  : Dongbu Hitek                                  */
/* date     : 13/03/29                                      */
/* Model    : SS1                                           */
/* 주의사항 : 0xE796 셋팅값을 만나면 0xE796을 I2C write 하지 말고 */
/*            150ms delay 후 다음 셋팅값을 I2C write해 주면 됨    */
/* =================================================================*/

static const u16 db8131m_common[] = {
/*==================================*/
/* Preview Command (SXGA)           */
/*==================================*/
0xFFC0, /*Page mode*/
0x1001,
0xE764, /*Wait  100*/

/*==================================*/
/* Format                           */
/*==================================*/
0xFFA1, /*Page mode*/
0x7001,
0x710D,

/*==================================*/
/* SensorCon                        */
/*==================================*/

0xFFD0, /*Page mode */
0x0E08, /*SnsrCon.ABLK_Ctrl_0      */
0x0F0D, /*ABLK_Ctrl_1_Addr         */
0x1300, /*Gain_Addr                */
0x1501, /*IVREFT_REFB_Addr  */
0x1834, /*ABLK_Ctrl_3              */
0x1921, /*ABLK_Ctrl_4              */
0x1A07, /*Rx_Tx_Range              */
0x200E, /*ABLK_Rsrv_Addr           */
0x2300, /*SnsrCon.IVREFT2_REFB2    */
0x2400, /*IPPG_IVCM2               */
0x39C7, /*RiseSx_CDS_1_L_Addr      */
0x511F, /*Fallcreset_1_L           */
0x6119, /*SnsrCon.RiseQBLPDN_L     */
0x8365, /*RiseTran_Sig_Even_L_Addr */
0x8567, /*FallTran_Sig_Even_L_Addr */
0x8765, /*RiseTran_Sig_Odd_L_Addr  */
0x8967, /*FallTran_Sig_Odd_L_Addr  */
0x8B27, /*RiseCNT_EN_1_L_Addr      */
0x8D6c, /*FallCNT_EN_1_L_Addr      */
0x9115, /*FallCNT_EN_2_L           */
0xC509, /*FallScanRx_L             */
0xD1CC, /*Pixel_RiseSx15_2_L       */
0xD400, /*DoutrClampVal_H          */
0xD744, /*SnsrCon.ABLK_Ctrl_12     */
0xDBCA, /*Pixel_FallScanTx15_1_L   */
0xED01, /*PLL_P_Addr               */
0xEE0F, /*PLL_M_Addr               */
0xEF00, /*PLL_S_Addr               */
0xF540, /*SnsrCon.PLL_Ctrl         */
0xF840, /*TestMode                 */
0xF900, /*ABLK_Ctrl_8              */
0xFB50, /*SnsrCon.PostADC_Gain     */

/*Dgain*/
0xFF82, 
0xB909,  /*pre R gain*/
0xBA80,  /*pre R gain*/
0xBB09,  /*pre G gain*/
0xBC80,  /*pre G gain*/
0xBD09,  /*pre B gain*/
0xBE80,  /*pre B gain*/

0xFF83,
0x6328, /*Again	Table*/
0x6410, /*Again	Table*/
0x65A8, /*Again	Table*/
0x6650, /*Again	Table*/
0x6728, /*Again	Table*/
0x6814, /*Again	Table*/
 
/*==================================*/ 
/*	Analog	ADF                     */
/*==================================*/ 

0xFF85,
0x8993, /*ADF.u8APThreshold          */
0x8A08, /*u8APClmpThreshold<=====1.0 */
      
0x8C07, /*gAdf_u8APMinVal_ThrClampH  */
0x8D40, /*gAdf_u8APMinVal_ThrClampL  */

0x8E00, /*gAdf_u8APMinVal_DOFFSET   */       
         
0x8F0C, /*gAdf_u8APMinVal_AMP2_1_SDM */
0x9111, /*gAdf_u8APMinVal_AMP4_3_SDM */
         
0x921F, /*gAdf_u8APMinVal_FallIntTx15 */
0x9377, /*ADF.u8APMinVal_CDSxRange_CtrlPre */
 
0x951D, /*ADF.u8APMinVal_REFB_IVCM  */
0x961A, /*ADF.u8APMinVal_ref_os_PB  */
0x970E, /*gAdf_u8APMinVal_NTx_Range */
0x980f, /*gAdf_u8APMaxVal_Clmp_rst  */
0x9907, /*gAdf_u8APMaxVal_ThrClampH */
0x9A00, /*gAdf_u8APMaxVal_ThrClampL */

0x9B00,

0x9C0C, /*gAdf_u8APMaxVal_AMP2_1_SDM 1.0 */
0x9D7E, /*ADF_Max_FallintRx                  */
0x9E29, /*gAdf_u8APMaxVal_AMP4_3_SDM         */
0x9F3F, /*gAdf_u8APMaxVal_FallIntTx15        */
0xA077, /*ADF.u8APMaxVal_CDSxRange_CtrlPre   */
0xA175, /*ADF_Max_FallintLatch               */
0xA218, /*gAdf_u8APMaxVal_REFB_IVCM          */
0xA33A, /*ADF.u8APMaxVal_ref_os_PB           */
0xA40F, /*gAdf_u8APMaxVal_NTx_Range          */

0xFF86,
0x1500, /*gPT_u8Adf_APThrHys            */
0x16C2, /*gPT_u8Adf_APFallIntTxThrLevel */
0x1709, /*gPT_u8Adf_APMinVal_BP2_1_SDM  */
0x1809, /*gPT_u8Adf_APMidVal_BP2_1_SDM  */
0x1909, /*gPT_u8Adf_APMaxVal_BP2_1_SDM  */

0x1BF0, 
0x1C00, /*DOFFSET   */

0x1D14, /*gPT_u8Adf_APMidVal_AMP2_1_SDM<====1.0 */
 
0x1F01, /*gPT_u8Adf_APMidVal_AMP4_3_SDM<=====1.0*/
0x2077, /*ADF.u8APMidVal_CDSxRange_CtrlPre      */
 
0x2218, /*gPT_u8Adf_APMidVal_REFB_IVCM  */
0x231A, /*ADF.u8APMidVal_ref_os_PB      */
0x240E, /*gPT_u8Adf_APMidVal_NTx_Range  */
0x2577, /*gPT_u8Adf_APVal_EnSiSoSht_EnSm*/

0x2DEB,

0xFF87, 
0xDDB0, /*UR.u8LinLength_Addr1              */
0xE1EB, /*UR.u8Clmp_sig_rst_Addr <=====v2.0 */
0xEA41, /*                                  */

0xF139, /*	count icnt_S에만적용 */

 
/*Analog UR */
0xFFD1,
0x0700, /*	power off mask clear  */
0x0B00, /*	clock off mask clear  */
0x0301, /*	parallel port disable */

/*==================================*/ 						
/* AE						                    */ 
/*==================================*/ 						

0xFF82,						
0x9101,			/* AE Mode	       */
0x9555,			/* AE weight 2_1	 */
0x9655,			/* AE weight 4_3	 */
0x97f5,			/* AE weight 6_5	 */
0x985f,			/* AE weight 8_7	 */
0x99f5,			/* AE weight 10_9	 */
0x9a5f,			/* AE weight 12_11	*/
0x9b55,			/* AE weight 14_13	*/
0x9c55,			/* AE weight 16_15 */
	
0x9D88,			/* AE Speed	   */
0x9E01,			/* AE LockBnd	 */
0x9F06,			/* AE HoldBnd	 */
0xA10A,			/* Analog Max	 */

0xA93B,			/*OTarget */	
0xAA3D,			/*ITarget */	
0xA840,			/*STarget */	

0xC502,			/* PeakMvStep	   */
0xC638,			/* PeakTgMin	   */
0xC724,			/* PeakRatioTh1	 */
0xC810,			/* PeakRatioTh0	 */
0xC905,			/* PeakLuTh	     */

0xD310,			/* LuxTBGainStep0	 */
0xD450,			/* LuxTBGainStep1 46	*/
0xD555,			/* LuxTBGainStep2	 */
0xD601,			/* LuxTBTimeStep0H	*/
0xD700,			/* LuxTBTimeStep0L	*/
0xD801,			/* LuxTBTimeStep1H	*/
0xD940,			/* LuxTBTimeStep1L	*/
0xDA04,			/* LuxTBTimeStep2H	*/
0xDB90,			/* LuxTBTimeStep2L	*/

0xf309,			/* SCLK	*/
0xf460,				

0xF900,			/* GainMax	 */
0xFAf0,			/* GainMax	 */
0xFB58,			/* Gain3Lut	 */
0xFC3A,			/* Gain2Lut	 */
0xFD28,			/* Gain1Lut	 */
0xFE12,			/* GainMin	 */

0xFF83,										
0x030F,			/* TimeMax60Hz	: 8fps   */
0x0408,			/* Time3Lux60Hz	: 12fps  */
0x0504,			/* Time2Lut60Hz	: 24fps  */
0x0604,			/* Time1Lut60Hz	: 24fps  */

0x070c,			/* TimeMax50Hz	:          */
0x080b,			/* Time3Lux50Hz	: 8.5fps   */
0x090a,			/* Time2Lut50Hz	: 10fps    */
0x0A06,			/* Time1Lut50Hz	: 15fps    */

0x0b04,			 	
0x0c4C,			/* 60Hz Frame Rate	*/

0x1104,			/* 50Hz Frame Rate	*/
0x1276,				

0x1730,			/* LuxLevel_Y3	*/
0x1810,			/* LuxLevel_Y2	*/
0x1902,			/* LuxLevel_Y1	*/
0x1a00,			/* LuxLevel_Yx	*/

0x2F04,			/* TimeNum0	   */
0x3005,			/* TimeNum1	   */
0x4f05,			/* FrameOffset	*/

0xFF85,												
0xC803,			/* Min 8fps	*/
0xC921,			

0xCC31,			/* B[0] Lux Hys Enable */	
0xCA03,			/* Lux Hys Boundary	   */

0xFF86,					
0xd402,		/* AeAccCtrl  */
0xd501,		/* FlickLine  */
0xd64d,		/* UpperPTh   */
0xd758,		/* MiddlePTh  */
0xd862,		/* LowerPTh   */
0xd9e0,		/* UpperYTh   */
0xdad2,		/* MiddleTTh  */
0xdbbe,		/* LowerYTh		*/			

0xe902,		/* MinimumExTh	*/				
0xea01,		/* MinFineEn		*/	
0xeb7f,		/* FineTimeMin	*/			

0xFF83,
0x5b00,		/*UpperFine-H */
0x5c00,		/*UpperFine-L */
0x5d00,		/*LowerFine-H */
0x5e01,		/*LowerFine-L */
					
0xFF82,
0x925D,			 /* AE Renew */	

						
/*==================================*/						
/* AWB						                  */
/*==================================*/						

0xFF83,						
0x7983,			/* AWBCtrl	       */
0x8200,			/* LockRatio	     */
0x8601,			/* MinGrayCnt	     */
0x8780,			/* MinGrayCnt	     */
0x9005,			/* RGain	         */
0x9405,			/* BGain	         */
0x98D4,			/* SkinWinCntTh	   */
0xA228,			/* SkinYTh	       */
0xA300,			/* SkinHoldHitCnt	 */
0xA40F,			/* SkinHoldHitCnt	 */
0xAD65,			/* SkinTop2	       */
0xAE80,			/* SkinTop2LS1Ratio*/
0xAF20,			/* SkinTop2LS2Ratio*/
0xB410,			/* SkinTop2LSHys	 */
0xB554,			 /* SkinLTx */	
0xB6BD,			 /* SkinLTy */
0xB774,			 /* SkinLBx */
0xB89D,			 /* SkinLBy */
0xBA4F,			/* UniCThrY	        */
0xBF0C,			/* UniCGrayCntThr_0	*/
0xC080,			/* UniCGrayCntThr_1	*/
0xFF84,
0x3D00,			/* AWB_LuxConst1_0 */ 
0x3E00,			/* AWB_LuxConst1_1 */ 
0x3F06,			/* AWB_LuxConst1_2 */ 
0x4020,			/* AWB_LuxConst1_3 */ 
0x4107,			/* AWB_LuxConst2_0 */ 
0x4253,			/* AWB_LuxConst2_1 */ 
0x4300,			/* AWB_LuxConst2_2 */ 
0x4400,			/* AWB_LuxConst2_3 */ 

0x4900,			/* Threshold_indoor	*/
0x4A07,				
0x4B01,			/* Threshold_outdoor */	
0x4C00,				

0x5D03,			/* AWB_Weight_Indoor_0	  */                
0x5E03,			/* AWB_Weight_Indoor_1	  */                
0x5F03,			/* AWB_Weight_Indoor_2	  */                      
0x6005,			/* AWB_Weight_Indoor_3	  */                
0x6110,			/* AWB_Weight_Indoor_4	  */                
0x6200,			/* AWB_Weight_Indoor_5	  */                
0x630C,			/* AWB_Weight_Indoor_6	  */
0x6408,			/* AWB_Weight_Indoor_7	  */
0x5501,			/* AWB_Weight_Genernal_0  */
0x5607,			/* AWB_Weight_Genernal_1  */
0x5714,			/* AWB_Weight_Genernal_2  */
0x5814,			/* AWB_Weight_Genernal_3  */
0x5920,			/* AWB_Weight_Genernal_4  */
0x5A00,			/* AWB_Weight_Genernal_5  */
0x5B16,			/* AWB_Weight_Genernal_6  */
0x5C10,			/* AWB_Weight_Genernal_7  */
0x6500,			/* AWB_Weight_Outdoor_0	  */
0x6600,			/* AWB_Weight_Outdoor_1	  */
0x6700,			/* AWB_Weight_Outdoor_2	  */
0x6840,			/* AWB_Weight_Outdoor_3	  */
0x6900,			/* AWB_Weight_Outdoor_4	  */
0x6A00,			/* AWB_Weight_Outdoor_5	  */
0x6B00,			/* AWB_Weight_Outdoor_6	  */
0x6C00,			/* AWB_Weight_Outdoor_7	  */

0xFF85,
0xE20C,			/* AWB_unicolorzone	*/	

0xFF83,
0xCB03,	 /* Min Rgain	*/
0xCCCB,
0xCD07,	 /* Max Rgain */
0xCE40,
0xD106,	 /* Max BGain */
0xD2C9,
0xCF02,	 /* Min Bgain */
0xD080,         

/*=================================*/						
/*AWB STE						               */
/*=================================*/
0xFFA1,
0x9C00,			/* AWBLuThL                 */    
0x9DF0,			/* AWBLuThH						      */
0xA063,			/* AWBZone0LTx - Flash	    */
0xA17A,			/* AWBZone0LTy	            */
0xA269,			/* AWBZone0RBx	            */
0xA36F,			/* AWBZone0RBy	            */
0xA48C,			/* AWBZone1LTx - Cloudy	    */
0xA540,			/* AWBZone1LTy	            */
0xA6A6,			/* AWBZone1RBx	            */
0xA731,			/* AWBZone1RBy	            */
0xA86E,			/* AWBZone2LTx - D65	      */
0xA95b,			/* AWBZone2LTy	            */
0xAA8b,			/* AWBZone2RBx	            */
0xAB41,			/* AWBZone2RBy	            */
0xAC64,			/* AWBZone3LTx - Fluorecent */
0xAD68,			/* AWBZone3LTy	            */
0xAE7D,			/* AWBZone3RBx	            */
0xAF47,			/* AWBZone3RBy	            */
0xB046,			/* AWBZone4LTx - CWF	      */
0xB168,			/* AWBZone4LTy	            */
0xB264,			/* AWBZone4RBx	            */
0xB34D,			/* AWBZone4RBy	            */
0xB400,			/* AWBZone5LTx - TL84	      */
0xB500,			/* AWBZone5LTy	            */
0xB600,			/* AWBZone5RBx	            */
0xB700,			/* AWBZone5RBy	            */    
0xB842,			/* AWBZone6LTx - A	        */
0xB985,			/* AWBZone6LTy	            */
0xBA64,			/* AWBZone6RBx	            */
0xBB69,			/* AWBZone6RBy	            */
0xBC39,			/* AWBZone7LTx - Horizon	  */
0xBDA0,			/* AWBZone7LTy	            */
0xBE59,			/* AWBZone7RBx	            */
0xBF7f,			/* AWBZone7RBy	            */
0xC05B,			/* AWBZone8LTx - Skin	      */
0xC185,			/* AWBZone8LTy	            */
0xC260,			/* AWBZone8RBx	            */
0xC37F,			/* AWBZone8RBy   	          */

/*==================================*/
/* UR                               */
/*==================================*/
0xFF87,
0xC922, 	 /* AWBTrim */
0xFF86,
0x14DE,	   /* CCM sum 1 */

0xFF85,
0x0605,	  /* Saturation CCM 1    */
0x8640, 	/* 42 saturation level */
0x0700,	  /* sup hysteresis      */

/*==================================*/						
/* CCM D65						              */
/*==================================*/
0xFF83,		
0xEA00,	/*gAwb_s16AdapCCMTbl_0  */
0xEB72,	/*gAwb_s16AdapCCMTbl_1  */
0xECFF,	/*gAwb_s16AdapCCMTbl_2  */
0xEDC9,	/*gAwb_s16AdapCCMTbl_3  */
0xEEff,	/*gAwb_s16AdapCCMTbl_4  */
0xEFF8,	/*gAwb_s16AdapCCMTbl_5  */
0xF0FF,	/*gAwb_s16AdapCCMTbl_6  */
0xF1ED,	/*gAwb_s16AdapCCMTbl_7  */
0xF200,	/*gAwb_s16AdapCCMTbl_8  */
0xF356,	/*gAwb_s16AdapCCMTbl_9  54  */
0xF4FF,	/*gAwb_s16AdapCCMTbl_10 */
0xF5F1,	/*gAwb_s16AdapCCMTbl_11 */
0xF6FF,	/*gAwb_s16AdapCCMTbl_12 */
0xF7FF,	/*gAwb_s16AdapCCMTbl_13 */
0xF8FF,	/*gAwb_s16AdapCCMTbl_14 */
0xF9C8,	/*gAwb_s16AdapCCMTbl_15 */
0xFA00,	/*gAwb_s16AdapCCMTbl_16 */
0xFB6E,	/*gAwb_s16AdapCCMTbl_17 */
					  
/* CWF lgiht */         
0xFC00,    /* gAwb_s16AdapCCMTbl_18 */ 
0xFD65,    /* gAwb_s16AdapCCMTbl_19 */ 
0xFF85,
0xE0FF,	 /* gAwb_s16AdapCCMTbl_20  */ 
0xE1e0,	 /* gAwb_s16AdapCCMTbl_21  */
0xFF84, 
0x00ff,     /* gAwb_s16AdapCCMTbl_22 */
0x01f4,     /* gAwb_s16AdapCCMTbl_23 */
0x02FF,     /* gAwb_s16AdapCCMTbl_24 */
0x03F3,     /* gAwb_s16AdapCCMTbl_25 */
0x0400,     /* gAwb_s16AdapCCMTbl_26 */
0x054B,     /* gAwb_s16AdapCCMTbl_27 */
0x06FF,     /* gAwb_s16AdapCCMTbl_28 */
0x07FA,     /* gAwb_s16AdapCCMTbl_29 */
0x08FF,     /* gAwb_s16AdapCCMTbl_30 */
0x09FC,     /* gAwb_s16AdapCCMTbl_31 */
0x0AFF,     /* gAwb_s16AdapCCMTbl_32 */
0x0BCC,     /* gAwb_s16AdapCCMTbl_33 */
0x0C00,     /* gAwb_s16AdapCCMTbl_34 */
0x0D6E,     /* gAwb_s16AdapCCMTbl_35 */

/* A light Green */
0x0E00,     /* gAwb_s16AdapCCMTbl_36 */
0x0F67,	    /* gAwb_s16AdapCCMTbl_37 */  
0x10FF,     /* gAwb_s16AdapCCMTbl_38 */
0x11d8,  	  /* gAwb_s16AdapCCMTbl_39 */
0x12ff,     /* gAwb_s16AdapCCMTbl_40 */
0x13ff,     /* gAwb_s16AdapCCMTbl_41 */
0x14FF,     /* gAwb_s16AdapCCMTbl_42 */
0x15ED,     /* gAwb_s16AdapCCMTbl_43 */
0x1600,     /* gAwb_s16AdapCCMTbl_44 */
0x1759,     /* gAwb_s16AdapCCMTbl_45 */
0x18ff,     /* gAwb_s16AdapCCMTbl_46 */
0x19f2,     /* gAwb_s16AdapCCMTbl_47 */
0x1Aff,     /* gAwb_s16AdapCCMTbl_48 */
0x1BEA,     /* gAwb_s16AdapCCMTbl_49 */
0x1CFF,     /* gAwb_s16AdapCCMTbl_50 */
0x1DbC,     /* gAwb_s16AdapCCMTbl_51 */
0x1E00,     /* gAwb_s16AdapCCMTbl_52 */
0x1F8d,     /* gAwb_s16AdapCCMTbl_53 */

/* Out door CCM */
0xFF86,
0x4501,    /* CCM LuxThreshold */
0x4600,    /* CCM LuxThreshold */
0xFF85,
0xFEB1,	   /* Outdoor CCM On   */

0xFF85,
0xEC00,     /* gAwb_s16AdapCCMTbl_0  */
0xED77,     /* gAwb_s16AdapCCMTbl_1  */
0xEEFF,     /* gAwb_s16AdapCCMTbl_2  */
0xEFB9,     /* gAwb_s16AdapCCMTbl_3  */
0xF000,     /* gAwb_s16AdapCCMTbl_4  */
0xF110,     /* gAwb_s16AdapCCMTbl_5  */
0xF2FF,     /* gAwb_s16AdapCCMTbl_6  */
0xF3F6,     /* gAwb_s16AdapCCMTbl_7  */
0xF400,     /* gAwb_s16AdapCCMTbl_8  */
0xF54B,     /* gAwb_s16AdapCCMTbl_9  */
0xF6FF,     /* gAwb_s16AdapCCMTbl_10 */
0xF7FF,     /* gAwb_s16AdapCCMTbl_11 */
0xF8FF,     /* gAwb_s16AdapCCMTbl_12 */
0xF9F5,     /* gAwb_s16AdapCCMTbl_13 */
0xFAFF,     /* gAwb_s16AdapCCMTbl_14 */
0xFBCE,     /* gAwb_s16AdapCCMTbl_15 */
0xFC00,     /* gAwb_s16AdapCCMTbl_16 */
0xFD7D,     /* gAwb_s16AdapCCMTbl_17 */
						
/*==================================*/						
/* ISP Global Control #1						*/
/*==================================*/						

0xFFA0,
0x1040,			// BLC Th	
0x1109,			// BLC Separator	
0x6073,			// CDC : Dark CDC ON  	
0x611F,			// Six CDC_Edge En, Slash EN	
0x62FE,	
0x6391,	    // EdgThresCtrl1 
0x64FD,	    // EdgThresCtrl2 
0x65A0,	    // ClipMode 
0x66A0,	    // DivRatio 
0x6720,	    // DivLoBD 
0x68B0,	    // DivUpBD 			
0x6920,			// Slash direction Line threshold	
0x6A40,			// Slash direction Pixel threshold	
0x6B10,	    // LineTH_SL 
0x6C40,	    // PixeTH_SL 
0x6D60,	    // TBase_SDC 
0x6E60,	    // TBase_DDC 
0x6F00,	    // Slope_Sign 
0x7000,				
0x7116,				
0x7200,				
0x7304,	
0x740F,	    // YOff0_DDC 
0x7520,	    // YOff1_DDC 			
0xC012,			// NSF: Th1	
0xC130,			// NSF: Th2	
0xC208,			// NSF: Th3	
0xD071,			// DEM: pattern detection	
0xD100,			// False color threshold	

/* LSC */
0xFF85,
0x0F43,	  /* LVLSC lux level threshold          */
0x10E3,	  /* LSLSC Light source , threshold lux */
0x1b00,

/* Lens Shading */
0xFFA0,
0x4380, 	  /* RH7 rrhrhh */
0x4480, 	  /* RV         */
0x4580, 	  /* RH	        */
0x4680, 	  /* RV         */
0x4780, 	  /* GH         */
0x4880, 	  /* GV         */
0x4980, 	  /* GH         */
0x4A80, 	  /* GV	        */
0x4B80, 	  /* BH         */
0x4C80,	    /* BV         */
0x4D80, 	  /* BH         */
0x4E80, 	  /* BV         */

0x5290, 	  /* GGain      */
0x5320, 	  /* GGain      */
0x5400, 	  /* GGain      */

/* Max Shading */
0xFF86,
0x51A0,	  /*Rgain1*/
0x5230, 	/*Rgain2*/
0x5300,	  /*Rgain3*/

0x5480, 	/*Bgain1*/
0x5520, 	/*Bgain2*/
0x5600,	  /*Bgain3*/

0x5790, 	/*GGain1*/
0x5820, 	/*GGain2*/
0x5900, 	/*GGain3*/

/* Min Shading */
0x48B0,	  /*Rgain1*/
0x4920,	  /*Rgain2*/
0x4A00,	  /*Rgain3*/

0x4B80, 	/*Bgain1*/
0x4C18, 	/*Bgain2*/
0x4D00, 	/*Bgain3*/

0x4E90, 	/*GGain1*/
0x4F20, 	/*GGain2*/
0x5000, 	/*GGain3*/
						
/*==================================*/						
/* ISP Global Control #2						*/
/*==================================*/						
						
0xFFA1,												
0x3030,			/* EDE: Luminane adaptation off	*/
0x3122,			/* EDE: SlopeGain	              */
0x3250,			/* EDE: Adaptation slope 	      */
0x3300,			/* EDE : PreCoringPt	          */
0x3400,			/* EDE: x1 point	              */
0x3516,			/* EDE: x1 point 	              */
0x3600,			/* EDE: x2 point	              */
0x3730,			/* EDE: x2 point 	              */
0x3809,			/* EDE: TransFuncSl1	          */
0x3922,			/* EDE: TransFuncSl2	          */
0x3A08,			/* EDE: Adaptation left margin	*/
0x3B08,			/* EDE: Adaptation right margin	*/
0x3C08,			/* EDE: RGB edge threshol	      */
0x3D00,			/* EDE: SmallOffset						  */
						                               

/*==================================*/						
/* ADF						                  */
/*==================================*/						

0xFF85,						
0x04FB,			/* FB EnEDE (v1.6)	   */
0x0605,			/* Flag	           */
0x0700,			/* sup hysteresis	 */

/*--------------------*/
/* ADF BGT	          */
/*--------------------*/
0x1721,			/* Th_BGT	 */
0x260A,			/* Min_BGT	*/
0x3C00,			/* Max_BGT	*/              
              
/*--------------------*/
/* ADF CNT            */
/*--------------------*/
0x18CB,			/* Th_CNT	  */
0x2700,			/* Min_CON	*/
0x3D02,			/* Max_CON	*/

/*----------------*/
/* ADF on         */
/*----------------*/
0xFF85,
0x04FB,	/* [7]CNF [3]NSF ADF on */
0xFF86,
0x14DE,	/* [7]ADF on for EDE new features */

/*----------------*/
/* ADF - CNFTh1   */
/*----------------*/
0xFF86,
0x7C03,	/* lux threshold for CNFTh1	*/
0xFF85,
0x2493,	/* CNFTh1 @ current_lux <  lux_thrd(0x867C)*/
0x3AC3,	/* CNFTh1 @ current_lux >= lux_thrd(0x867C)*/
/*----------------*/
/* ADF - CNFTh2   */
/*----------------*/
0xFF85,
0x13ED,	/* lux threshold for CNFTh2([7:4] : max_lux, [3:0] min_lux)*/ 
0x2509,	/* CNFTh2 @ min lux level                                  */
0x3B03,	/* CNFTh2 @ max lux level                                  */
/*----------------*/
/* ADF - CNFTh3, 4*/ 
/*----------------*/
0xFF86,
0x7D04,	/* CNFTh3 @ #0 light source [0x83D3:0x00] - cloudy, daylight, flourscent*/
0x8019,	/* CNFTh4 @ #0 light source [0x83D3:0x00] - cloudy, daylight, flourscent*/
0x7E00,	/* CNFTh3 @ #1 light source [0x83D3:0x01] - CWF, TL84                   */
0x8100,	/* CNFTh4 @ #1 light source [0x83D3:0x01] - CWF, TL84                   */
0x7F00,	/* CNFTh3 @ #2 light source [0x83D3:0x02] - A, Horizon                  */
0x8200,	/* CNFTh4 @ #2 light source [0x83D3:0x02] - A, Horizon                  */
/*----------------                     */
/* HW address (if CNF ADF turned off)  */
/*----------------                     */
0xFFA1,
0x4607,    /* CNFTh5 HW only */

/*----------------*/
/* ADF - NSF1/2/3 */
/*----------------*/  
0xFF85,
0x12A4,	/* Th_NSF	      */
0x221E,	/* Min_NSF_Th1	 */
0x2350,	/* Min_NSF_Th2	 */
0x3810,	/* Max_NSF_Th1	 */
0x3928,	/* Max_NSF_Th2   */
0xFF86,	                                             
0x1206,	/* Min_NSF_Th3	  */
0x1306,	/* Max_NSF_Th3   */
0xFF85,	    
0xE85C,	/* u8ADF_STVal_NSF_TH1 //patch initial value @ firmware 2.0  */
0xE962,	/* u8ADF_STVal_NSF_TH2 //patch initial value @ firmware 2.0  */
0xEA0E,	/* u8ADF_EndVal_NSF_TH1 //patch initial value @ firmware 2.0 */
0xEB18,	/* u8ADF_EndVal_NSF_TH2 //patch initial value @ firmware 2.0 */

/*----------------*/
/* ADF - NSF4     */
/*----------------*/
0xFF86,
0xA285,	/* Lux. level threshold for NSF4 */
0xA500,	/* Min_NSF_Th4                   */
0xA808,	/* Max_NSF_Th4                   */
/*----------------*/
/* ADF - NSF5/6   */
/*----------------*/
0xA30B,	/* Lux. level threshold for NSF5/6 */
0xA624,	/* Min_NSF_Th5                     */
0xA944,	/* Max_NSF_Th5                     */
0xA702,	/* Min_NSF_Th6                     */
0xAA00,	/* Max_NSF_Th6                     */

/*----------------*/
/* ADF - EDE new  */
/*----------------*/
0xC00E,	/* Lux level threshold for EDEOption            */
0xC130,	/* EDEOption value over lux level threshold     */
0xC230,	/* EDEOption value under lux level threshol     */
0xC330,	/* EDESmEdgThrd value over lux level threshold  */
0xC416,	/* EDESmEdgThrd value under lux level threshold */
0xC5A5,	/* Lux level threshold for EDESmEdgGain         */
0xC602,	/* EDESmEdgGain value @ min lux level           */
0xC703,	/* EDESmEdgGain value @ max lux level           */

/*--------------------*/                          
/* ADF - GDC          */
/*--------------------*/ 
0xFF85, 
0x1543,	/* GDC lux level threshold */  
0x2D40,	/* Min_GDC_Th1             */
0x2E80,	/* Min_GDC_Th2             */
0x4308,	/* Max_GDC_Th1             */
0x4430,	/* Max_GDC_Th2             */

/*--------------------*/
/* ADF Edge           */
/*--------------------*/
0x14B3,			/* Th_Edge	            */
0x2806,			/* Min_Coring	          */
0x2906,			/* Min_Edge_Slope1	    */
0x2A07,			/* Min_Edge_Slope2	    */
0x2B06,			/* Min_Edge_SmallOffset */
0x2C22,			/* Min_Edge_Slope	      */
0x3E02,			/* Max_Coring	          */
0x3F08,			/* Max_Edge_Slope1	    */
0x4009,			/* Max_Edge_Slope2	    */
0x4106,			/* Max_Edge_SmallOffset */
0x4222,			/* Max_Edge_Slope	      */

/*--------------------*/
/* ADF Gamma          */
/*--------------------*/
0xFF86,
0xBF01,   /* DarkGMA Threshold */
0xAE00,		/* DarkGMA_0         */
0xAF04,		/* DarkGMA_0         */
0xB008,		/* DarkGMA_0         */
0xB110,		/* DarkGMA_0         */
0xB218,		/* DarkGMA_0         */
0xB320,		/* DarkGMA_0         */
0xB430,		/* DarkGMA_0         */
0xB540,		/* DarkGMA_0         */
0xB650,		/* DarkGMA_0         */
0xB760,		/* DarkGMA_0         */
0xB880,		/* DarkGMA_0         */
0xB9A0,		/* DarkGMA_0         */
0xBAC0,		/* DarkGMA_0         */
0xBBD0,		/* DarkGMA_0         */
0xBCE0,		/* DarkGMA_0         */
0xBDF0,		/* DarkGMA_0         */
0xBEFF,		/* DarkGMA_0         */

0xFF85,
0x1670,   /* Gamma Threshold */
0x4700,	  /* Min_Gamma_0     */
0x4816,	  /* Min_Gamma_1     */
0x4925,	  /* Min_Gamma_2     */
0x4A37,	  /* Min_Gamma_3     */
0x4B45,	  /* Min_Gamma_4     */
0x4C51,	  /* Min_Gamma_5     */
0x4D65,	  /* Min_Gamma_6     */
0x4E77,	  /* Min_Gamma_7     */
0x4F87,	  /* Min_Gamma_8     */
0x5095,	  /* Min_Gamma_9     */
0x51AF,	  /* Min_Gamma_10    */
0x52C6,	  /* Min_Gamma_11    */
0x53DB,	  /* Min_Gamma_12    */
0x54E5,	  /* Min_Gamma_13    */
0x55EE,	  /* Min_Gamma_14    */
0x56F7,	  /* Min_Gamma_15    */
0x57FF,	  /* Min_Gamma_16    */

0x5800,	  /* Max_Gamma_0  */ 
0x5902,	  /* Max_Gamma_1  */
0x5a14,	  /* Max_Gamma_2  */
0x5b30,	  /* Max_Gamma_3  */
0x5c4A,	  /* Max_Gamma_4  */
0x5d5D,	  /* Max_Gamma_5  */
0x5e78,	  /* Max_Gamma_6  */
0x5f8E,	  /* Max_Gamma_7  */
0x609F,	  /* Max_Gamma_8  */
0x61AC,	  /* Max_Gamma_9  */
0x62C2,	  /* Max_Gamma_10 */
0x63D7,	  /* Max_Gamma_11 */
0x64E7,	  /* Max_Gamma_12 */
0x65EE,	  /* Max_Gamma_13 */
0x66F3,	  /* Max_Gamma_14 */
0x67F9,	  /* Max_Gamma_15 */
0x68FF,	  /* Max_Gamma_16 */

/*--------------------*/ 
/* Suppression	      */
/*--------------------*/
0x1A21,			/* Th_SUP	          */
0x3060,			/* Min_suppression	*/
0x4680,			/* Max_suppression  */

/*--------------------*/ 
/* CDC	              */
/*--------------------*/
0x1144,			// Th_CDC	
0xFF86,
0xE102,	    // u8CDCDarkTh 
0xFF85,
0x6960,			// CDCMin_TBaseDDC	
0x6A00,			// CDCMin_SlopeSign	
0x6B15,			// CDCMin_SlopeDDC	
0x6C0F,			// CDCMin_YOff0	
0x6D20,			// CDCMin_YOff1	
0x6E48,			// CDCMax_TBaseDDC	
0x6F00,			// CDCMax_SlopeSign	
0x70F4,			// CDCMax_SlopeDDC	
0x713C,			// CDCMax_YOff0	
0x7244,			// CDCMax_YOff1	
0xFF86,						
0xE001,	    // u8CDCEn - forcibly make dark cdc enable
0xE27F,	    // u8DarkCdcCtrlA
0xE320,	    // u8DarkTBase_DDC
0xE405,	    // u8DarkSlope_DDC
0xE500,	    // u8DarkYOff0_DDC
0xE60F,	    // u8DarkYOff1_DDC			
						
/*==================================*/						
/* ADF Update						            */
/*==================================*/	
0xFF85,					
0x0910,			/* CurLux	*/					
						
/*==================================*/						
/* Saturation						            */
/*==================================*/						
0x8640,			/* Color Saturation	*/

/* BGT2 */
0xFF86,
0x6800,	  /* BGTLux1 */
0x6900,	  /* BGTLux2 */
0x6a00,	  /* BGTLux3 */
0x6b00,	  /* Value1  */
0x6c00,	  /* Value2  */
0x6d00,	  /* Value3  */

/* SAT */ 
0x6F09,	  /*SATLux1 Gamut             */
0x7000,	  /*SATLux2                   */
0x7102,	  /*SATZone                   */
0x7200,	  /*SATZone                   */
0x7301,	  /*SATValue1    D65 Gamut    */
0x7400,	  /*SATValue2                 */


/* LineLength*/
0xFF87, /*Page mode */
0xDC05,
0xDDB0,
0xd500, /* Flip*/


/*==================================*/
/* MIPI                             */
/*==================================*/
0xFFB0, /*Page mode*/
0x5402, /* MIPI PHY_HS_TX_CTRL*/
0x3805, /* MIPI DPHY_CTRL_SET*/
0x3C81,
0x5012, /*MIPI.PHY_HL_TX_SEL */
0x5884, /* MIPI setting  추가 setting */
0x5921, /* MIPI setting  추가 setting */

/* SXGA PR*/
0xFF85, /*Page mode */
0xB71E,
0xB80A, /* gPT_u8PR_Active_SXGA_WORD_COUNT_Addr0*/
0xB900, /* gPT_u8PR_Active_SXGA_WORD_COUNT_Addr1*/
0xBC04, /* gPT_u8PR_Active_SXGA_DPHY_CLK_TIME_Addr3*/
0xBF05, /* gPT_u8PR_Active_SXGA_DPHY_DATA_TIME_Addr3*/
0xFF87, /* Page mode*/
0x0C00, /* start Y*/
0x0D20, /* start Y*/
0x1003, /* end Y*/
0x11E0, /* end Y*/

0xFF86, /*u8PLL_Ctrl_Addr*/
0xFE40,

/* VGA Capture */
0xFF86, /*Page mode*/
0x371E,
0x3805, /*gPT_u8PR_Active_720P_WORD_COUNT_Addr0*/
0x3900, /*gPT_u8PR_Active_720P_WORD_COUNT_Addr1*/
0x3C04, /*gPT_u8PR_Active_720P_DPHY_CLK_TIME_Addr3*/
0x3F05, /* gPT_u8PR_Active_720P_DPHY_DATA_TIME_Addr3*/

0xFF87,
0x2300, /*gPR_Active_720P_u8SensorCtrl_Addr*/
0x2472, /*gPR_Active_720P_u8SensorMode_Addr*/
0x2501, /*gPR_Active_720P_u8PLL_P_Addr*/
0x260F, /*gPR_Active_720P_u8PLL_M_Addr*/
0x2700, /*gPR_Active_720P_u8PLL_S_Addr*/
0x2840, /*gPR_Active_720P_u8PLL_Ctrl_Addr*/
0x2901, /*gPR_Active_720P_u8src_clk_sel_Addr*/
0x2A00, /*gPR_Active_720P_u8output_pad_status_Addr*/
0x2B3F, /*gPR_Active_720P_u8ablk_ctrl_10_Addr*/
0x2CFF, /*gPR_Active_720P_u8BayerFunc_Addr*/
0x2DFF, /*gPR_Active_720P_u8RgbYcFunc_Addr*/
0x2E00, /*gPR_Active_720P_u8ISPMode_Addr*/
0x2F02, /*gPR_Active_720P_u8SCLCtrl_Addr*/
0x3001, /*gPR_Active_720P_u8SCLHorScale_Addr0*/
0x31FF, /*gPR_Active_720P_u8SCLHorScale_Addr1*/
0x3201, /*gPR_Active_720P_u8SCLVerScale_Addr0*/
0x33FF, /*gPR_Active_720P_u8SCLVerScale_Addr1*/
0x3400, /*gPR_Active_720P_u8SCLCropStartX_Addr0*/
0x3500, /*gPR_Active_720P_u8SCLCropStartX_Addr1*/
0x3600, /*gPR_Active_720P_u8SCLCropStartY_Addr0*/
0x3710, /*gPR_Active_720P_u8SCLCropStartY_Addr1*/
0x3802, /*gPR_Active_720P_u8SCLCropEndX_Addr0*/
0x3980, /*gPR_Active_720P_u8SCLCropEndX_Addr1*/
0x3A01, /*gPR_Active_720P_u8SCLCropEndY_Addr0*/
0x3BF0, /*gPR_Active_720P_u8SCLCropEndY_Addr1*/
0x3C01, /*gPR_Active_720P_u8OutForm_Addr*/
0x3D0C, /*gPR_Active_720P_u8OutCtrl_Addr*/
0x3E04, /*gPR_Active_720P_u8AEWinStartX_Addr*/
0x3F04, /*gPR_Active_720P_u8AEWinStartY_Addr*/
0x4066, /*gPR_Active_720P_u8MergedWinWidth_Addr*/
0x415E, /*gPR_Active_720P_u8MergedWinHeight_Addr*/
0x4204, /*gPR_Active_720P_u8AEHistWinAx_Addr*/
0x4304, /*gPR_Active_720P_u8AEHistWinAy_Addr*/
0x4498, /*gPR_Active_720P_u8AEHistWinBx_Addr*/
0x4578, /*gPR_Active_720P_u8AEHistWinBy_Addr*/
0x4622, /*gPR_Active_720P_u8AWBTrim_Addr*/
0x4728, /*gPR_Active_720P_u8AWBCTWinAx_Addvr*/
0x4820, /*gPR_Active_720P_u8AWBCTWinAy_Addr*/
0x4978, /*gPR_Active_720P_u8AWBCTWinBx_Addr*/
0x4A60, /*gPR_Active_720P_u8AWBCTWinBy_Addr*/
0x4B03, /*gPR_Active_720P_u16AFCFrameLength_0*/
0x4C08, /*gPR_Active_720P_u16AFCFrameLength_1*/

/*VGA PR*/
0xFF86, /*Page mode*/
0x2E1E,
0x2F05, /* gPT_u8PR_Active_VGA_WORD_COUNT_Addr0*/
0x3000, /* gPT_u8PR_Active_VGA_WORD_COUNT_Addr1*/
0x3304, /* gPT_u8PR_Active_VGA_DPHY_CLK_TIME_Addr3*/
0x3605, /* gPT_u8PR_Active_VGA_DPHY_DATA_TIME_Addr3*/

0xFF87, /*Page mode*/
0x4D00, /*gPR_Active_VGA_u8SensorCtrl_Addr*/
0x4E72, /*gPR_Active_VGA_u8SensorMode_Addr*/
0x4F01, /*gPR_Active_VGA_u8PLL_P_Addr*/
0x500F, /*gPR_Active_VGA_u8PLL_M_Addr*/
0x5100, /*gPR_Active_VGA_u8PLL_S_Addr*/
0x5240, /*gPR_Active_VGA_u8PLL_Ctrl_Addr*/
0x5301, /*gPR_Active_VGA_u8src_clk_sel_Addr*/
0x5400, /*gPR_Active_VGA_u8output_pad_status_Addr*/
0x553F, /*gPR_Active_VGA_u8ablk_ctrl_10_Addr*/
0x56FF, /*gPR_Active_VGA_u8BayerFunc_Addr*/
0x57FF, /*gPR_Active_VGA_u8RgbYcFunc_Addr*/
0x5800, /*gPR_Active_VGA_u8ISPMode_Addr*/
0x5902, /*gPR_Active_VGA_u8SCLCtrl_Addr*/
0x5A01, /*gPR_Active_VGA_u8SCLHorScale_Addr0*/
0x5BFF, /*gPR_Active_VGA_u8SCLHorScale_Addr1*/
0x5C01, /*gPR_Active_VGA_u8SCLVerScale_Addr0*/
0x5DFF, /*gPR_Active_VGA_u8SCLVerScale_Addr1*/
0x5E00, /*gPR_Active_VGA_u8SCLCropStartX_Addr0*/
0x5F00, /*gPR_Active_VGA_u8SCLCropStartX_Addr1*/
0x6000, /*gPR_Active_VGA_u8SCLCropStartY_Addr0*/
0x6110, /*gPR_Active_VGA_u8SCLCropStartY_Addr1*/
0x6202, /*gPR_Active_VGA_u8SCLCropEndX_Addr0*/
0x6380, /*gPR_Active_VGA_u8SCLCropEndX_Addr1*/
0x6401, /*gPR_Active_VGA_u8SCLCropEndY_Addr0*/
0x65F0, /*gPR_Active_VGA_u8SCLCropEndY_Addr1*/


/* Self-Cam END of Initial*/
};

/* Set-data based on SKT VT standard ,when using 3G network */
/* 8fps */
static const u16 db8131m_vt_common[] = {
	
/*
 Command Preview Fixed 8fps
*/
/*==================================*/
/* Preview Command (SXGA)           */
/*==================================*/
0xFFC0, /*Page mode*/
0x1001,
0xE764, /*Wait  100*/

/*==================================*/
/* Format                           */
/*==================================*/
0xFFA1, /*Page mode*/
0x7001,
0x710D,

/*==================================*/
/* SensorCon                        */
/*==================================*/

0xFFD0, /*Page mode */
0x0E08, /*SnsrCon.ABLK_Ctrl_0      */
0x0F0D, /*ABLK_Ctrl_1_Addr         */
0x1300, /*Gain_Addr                */
0x1501, /*IVREFT_REFB_Addr  */
0x1814, /*ABLK_Ctrl_3              */
0x1921, /*ABLK_Ctrl_4              */
0x1A07, /*Rx_Tx_Range              */
0x200E, /*ABLK_Rsrv_Addr           */
0x2300, /*SnsrCon.IVREFT2_REFB2    */
0x2400, /*IPPG_IVCM2               */
0x39C7, /*RiseSx_CDS_1_L_Addr      */
0x511C, /*Fallcreset_1_L           */
0x6119, /*SnsrCon.RiseQBLPDN_L     */
0x8365, /*RiseTran_Sig_Even_L_Addr */
0x8567, /*FallTran_Sig_Even_L_Addr */
0x8765, /*RiseTran_Sig_Odd_L_Addr  */
0x8967, /*FallTran_Sig_Odd_L_Addr  */
0x8B27, /*RiseCNT_EN_1_L_Addr      */
0x8D6c, /*FallCNT_EN_1_L_Addr      */
0x9115, /*FallCNT_EN_2_L           */
0xC509, /*FallScanRx_L             */
0xD1CC, /*Pixel_RiseSx15_2_L       */
0xD400, /*DoutrClampVal_H          */
0xD744, /*SnsrCon.ABLK_Ctrl_12     */
0xDBCA, /*Pixel_FallScanTx15_1_L   */
0xED01, /*PLL_P_Addr               */
0xEE0F, /*PLL_M_Addr               */
0xEF00, /*PLL_S_Addr               */
0xF540, /*SnsrCon.PLL_Ctrl         */
0xF840, /*TestMode                 */
0xF900, /*ABLK_Ctrl_8              */
0xFB50, /*SnsrCon.PostADC_Gain     */

/*Dgain*/
0xFF82, 
0xB909,  /*pre R gain*/
0xBA80,  /*pre R gain*/
0xBB09,  /*pre G gain*/
0xBC80,  /*pre G gain*/
0xBD09,  /*pre B gain*/
0xBE80,  /*pre B gain*/

0xFF83,
0x6328, /*Again	Table*/
0x6410, /*Again	Table*/
0x65A8, /*Again	Table*/
0x6650, /*Again	Table*/
0x6728, /*Again	Table*/
0x6814, /*Again	Table*/
 
/*==================================*/ 
/*	Analog	ADF                     */
/*==================================*/ 

0xFF85,
0x8993, /*ADF.u8APThreshold          */
0x8A08, /*u8APClmpThreshold<=====1.0 */
      
0x8C07, /*gAdf_u8APMinVal_ThrClampH  */
0x8D40, /*gAdf_u8APMinVal_ThrClampL  */

0x8E00, /*gAdf_u8APMinVal_DOFFSET   */       
         
0x8F0C, /*gAdf_u8APMinVal_AMP2_1_SDM */
0x9101, /*gAdf_u8APMinVal_AMP4_3_SDM */
         
0x921F, /*gAdf_u8APMinVal_FallIntTx15 */
0x9377, /*ADF.u8APMinVal_CDSxRange_CtrlPre */
 
0x951D, /*ADF.u8APMinVal_REFB_IVCM  */
0x961A, /*ADF.u8APMinVal_ref_os_PB  */
0x970E, /*gAdf_u8APMinVal_NTx_Range */
0x980f, /*gAdf_u8APMaxVal_Clmp_rst  */
0x9907, /*gAdf_u8APMaxVal_ThrClampH */
0x9A00, /*gAdf_u8APMaxVal_ThrClampL */

0x9B00,

0x9C0C, /*gAdf_u8APMaxVal_AMP2_1_SDM 1.0 */
0x9D7E, /*ADF_Max_FallintRx                  */
0x9E29, /*gAdf_u8APMaxVal_AMP4_3_SDM         */
0x9F3F, /*gAdf_u8APMaxVal_FallIntTx15        */
0xA077, /*ADF.u8APMaxVal_CDSxRange_CtrlPre   */
0xA175, /*ADF_Max_FallintLatch               */
0xA218, /*gAdf_u8APMaxVal_REFB_IVCM          */
0xA33A, /*ADF.u8APMaxVal_ref_os_PB           */
0xA40F, /*gAdf_u8APMaxVal_NTx_Range          */

0xFF86,
0x1500, /*gPT_u8Adf_APThrHys            */
0x16C2, /*gPT_u8Adf_APFallIntTxThrLevel */
0x1709, /*gPT_u8Adf_APMinVal_BP2_1_SDM  */
0x1809, /*gPT_u8Adf_APMidVal_BP2_1_SDM  */
0x1909, /*gPT_u8Adf_APMaxVal_BP2_1_SDM  */

0x1BF0, 
0x1C00, /*DOFFSET   */

0x1D14, /*gPT_u8Adf_APMidVal_AMP2_1_SDM<====1.0 */
 
0x1F01, /*gPT_u8Adf_APMidVal_AMP4_3_SDM<=====1.0*/
0x2077, /*ADF.u8APMidVal_CDSxRange_CtrlPre      */
 
0x2218, /*gPT_u8Adf_APMidVal_REFB_IVCM  */
0x231A, /*ADF.u8APMidVal_ref_os_PB      */
0x240E, /*gPT_u8Adf_APMidVal_NTx_Range  */
0x2577, /*gPT_u8Adf_APVal_EnSiSoSht_EnSm*/

0x2DEB,

0xFF87, 
0xDDB0, /*UR.u8LinLength_Addr1              */
0xE1EB, /*UR.u8Clmp_sig_rst_Addr <=====v2.0 */
0xEA41, /*                                  */

0xF139, /*	count icnt_S에만적용 */

 
/*Analog UR */
0xFFD1,
0x0700, /*	power off mask clear  */
0x0B00, /*	clock off mask clear  */
0x0301, /*	parallel port disable */

/*==================================*/ 						
/* AE						                    */ 
/*==================================*/ 						

0xFF82,						
0x9102,			/* AE Mode	       */
0x9555,			/* AE weight 2_1	 */
0x9655,			/* AE weight 4_3	 */
0x97f5,			/* AE weight 6_5	 */
0x985f,			/* AE weight 8_7	 */
0x99f5,			/* AE weight 10_9	 */
0x9a5f,			/* AE weight 12_11	*/
0x9b55,			/* AE weight 14_13	*/
0x9c55,			/* AE weight 16_15 */
	
0x9D88,			/* AE Speed	   */
0x9E02,			/* AE LockBnd	 */
0x9F06,			/* AE HoldBnd	 */
0xA10A,			/* Analog Max	 */

0xA93B,			/*OTarget */	
0xAA3E,			/*ITarget */	
0xA840,			/*STarget */	

0xC502,			/* PeakMvStep	   */
0xC638,			/* PeakTgMin	   */
0xC724,			/* PeakRatioTh1	 */
0xC810,			/* PeakRatioTh0	 */
0xC905,			/* PeakLuTh	     */

0xD310,			/* LuxTBGainStep0	 */
0xD450,			/* LuxTBGainStep1 46	*/
0xD555,			/* LuxTBGainStep2	 */
0xD601,			/* LuxTBTimeStep0H	*/
0xD700,			/* LuxTBTimeStep0L	*/
0xD801,			/* LuxTBTimeStep1H	*/
0xD940,			/* LuxTBTimeStep1L	*/
0xDA04,			/* LuxTBTimeStep2H	*/
0xDB90,			/* LuxTBTimeStep2L	*/

0xf309,			/* SCLK	*/
0xf460,				

0xF900,			/* GainMax	 */
0xFAf0,			/* GainMax	 */
0xFB58,			/* Gain3Lut	 */
0xFC3A,			/* Gain2Lut	 */
0xFD28,			/* Gain1Lut	 */
0xFE12,			/* GainMin	 */

0xFF83,										
0x030F,			/* TimeMax60Hz	: 8fps   */
0x040E,			/* Time3Lux60Hz	: 12fps  */
0x050C,			/* Time2Lut60Hz	: 24fps  */
0x0608,			/* Time1Lut60Hz	: 24fps  */

0x070c,			/* TimeMax50Hz	:          */
0x080b,			/* Time3Lux50Hz	: 8.5fps   */
0x090a,			/* Time2Lut50Hz	: 10fps    */
0x0A06,			/* Time1Lut50Hz	: 15fps    */

0x0b08,			 	
0x0c11,			/* 60Hz Frame Rate	*/

0x1107,			/* 50Hz Frame Rate	*/
0x12bb,				

0x1730,			/* LuxLevel_Y3	*/
0x1810,			/* LuxLevel_Y2	*/
0x1902,			/* LuxLevel_Y1	*/
0x1a00,			/* LuxLevel_Yx	*/

0x2F04,			/* TimeNum0	   */
0x3005,			/* TimeNum1	   */
0x4f05,			/* FrameOffset	*/

0xFF85,												
0xC803,			/* Min 8fps	*/
0xC921,			

0xCC31,			/* B[0] Lux Hys Enable */	
0xCA03,			/* Lux Hys Boundary	   */

0xFF86,					
0xd402,		/* AeAccCtrl  */
0xd501,		/* FlickLine  */
0xd64d,		/* UpperPTh   */
0xd758,		/* MiddlePTh  */
0xd862,		/* LowerPTh   */
0xd9e0,		/* UpperYTh   */
0xdad2,		/* MiddleTTh  */
0xdbbe,		/* LowerYTh		*/			

0xe902,		/* MinimumExTh	*/				
0xea01,		/* MinFineEn		*/	
0xeb7f,		/* FineTimeMin	*/			

0xFF83,
0x5b00,		/*UpperFine-H */
0x5c00,		/*UpperFine-L */
0x5d00,		/*LowerFine-H */
0x5e01,		/*LowerFine-L */
					
0xFF82,
0x925D,			 /* AE Renew */	

						
/*==================================*/						
/* AWB						                  */
/*==================================*/						

0xFF83,						
0x7983,			/* AWBCtrl	       */
0x8200,			/* LockRatio	     */
0x8601,			/* MinGrayCnt	     */
0x8780,			/* MinGrayCnt	     */
0x9005,			/* RGain	         */
0x9405,			/* BGain	         */
0x98D4,			/* SkinWinCntTh	   */
0xA228,			/* SkinYTh	       */
0xA300,			/* SkinHoldHitCnt	 */
0xA40F,			/* SkinHoldHitCnt	 */
0xAD65,			/* SkinTop2	       */
0xAE80,			/* SkinTop2LS1Ratio*/
0xAF20,			/* SkinTop2LS2Ratio*/
0xB410,			/* SkinTop2LSHys	 */
0xB554,			 /* SkinLTx */	
0xB6BD,			 /* SkinLTy */
0xB774,			 /* SkinLBx */
0xB89D,			 /* SkinLBy */
0xBA4F,			/* UniCThrY	        */
0xBF0C,			/* UniCGrayCntThr_0	*/
0xC080,			/* UniCGrayCntThr_1	*/
0xFF84,
0x3D00,			/* AWB_LuxConst1_0 */ 
0x3E00,			/* AWB_LuxConst1_1 */ 
0x3F06,			/* AWB_LuxConst1_2 */ 
0x4020,			/* AWB_LuxConst1_3 */ 
0x4107,			/* AWB_LuxConst2_0 */ 
0x4253,			/* AWB_LuxConst2_1 */ 
0x4300,			/* AWB_LuxConst2_2 */ 
0x4400,			/* AWB_LuxConst2_3 */ 

0x4900,			/* Threshold_indoor	*/
0x4A07,				
0x4B01,			/* Threshold_outdoor */	
0x4C00,				

0x5D03,			/* AWB_Weight_Indoor_0	  */                
0x5E03,			/* AWB_Weight_Indoor_1	  */                
0x5F03,			/* AWB_Weight_Indoor_2	  */                      
0x6005,			/* AWB_Weight_Indoor_3	  */                
0x6110,			/* AWB_Weight_Indoor_4	  */                
0x6200,			/* AWB_Weight_Indoor_5	  */                
0x6308,			/* AWB_Weight_Indoor_6	  */
0x6408,			/* AWB_Weight_Indoor_7	  */
0x5501,			/* AWB_Weight_Genernal_0  */
0x5607,			/* AWB_Weight_Genernal_1  */
0x5714,			/* AWB_Weight_Genernal_2  */
0x5814,			/* AWB_Weight_Genernal_3  */
0x5920,			/* AWB_Weight_Genernal_4  */
0x5A00,			/* AWB_Weight_Genernal_5  */
0x5B16,			/* AWB_Weight_Genernal_6  */
0x5C10,			/* AWB_Weight_Genernal_7  */
0x6500,			/* AWB_Weight_Outdoor_0	  */
0x6600,			/* AWB_Weight_Outdoor_1	  */
0x6700,			/* AWB_Weight_Outdoor_2	  */
0x6840,			/* AWB_Weight_Outdoor_3	  */
0x6900,			/* AWB_Weight_Outdoor_4	  */
0x6A00,			/* AWB_Weight_Outdoor_5	  */
0x6B00,			/* AWB_Weight_Outdoor_6	  */
0x6C00,			/* AWB_Weight_Outdoor_7	  */

0xFF85,
0xE20C,			/* AWB_unicolorzone	*/	

0xFF83,
0xCB04,	 /* Min Rgain	*/
0xCC28,
0xCD07,	 /* Max Rgain */
0xCE40,
0xD102,	 /* Max BGain */
0xD280,
0xCF06,	 /* Min Bgain */
0xD0A0,         

/*=================================*/						
/*AWB STE						               */
/*=================================*/
0xFFA1,
0x9C00,			/* AWBLuThL                 */    
0x9DF0,			/* AWBLuThH						      */
0xA063,			/* AWBZone0LTx - Flash	    */
0xA17A,			/* AWBZone0LTy	            */
0xA269,			/* AWBZone0RBx	            */
0xA36F,			/* AWBZone0RBy	            */
0xA48C,			/* AWBZone1LTx - Cloudy	    */
0xA540,			/* AWBZone1LTy	            */
0xA6A6,			/* AWBZone1RBx	            */
0xA731,			/* AWBZone1RBy	            */
0xA86E,			/* AWBZone2LTx - D65	      */
0xA95b,			/* AWBZone2LTy	            */
0xAA8b,			/* AWBZone2RBx	            */
0xAB41,			/* AWBZone2RBy	            */
0xAC64,			/* AWBZone3LTx - Fluorecent */
0xAD68,			/* AWBZone3LTy	            */
0xAE7D,			/* AWBZone3RBx	            */
0xAF47,			/* AWBZone3RBy	            */
0xB046,			/* AWBZone4LTx - CWF	      */
0xB168,			/* AWBZone4LTy	            */
0xB264,			/* AWBZone4RBx	            */
0xB34D,			/* AWBZone4RBy	            */
0xB400,			/* AWBZone5LTx - TL84	      */
0xB500,			/* AWBZone5LTy	            */
0xB600,			/* AWBZone5RBx	            */
0xB700,			/* AWBZone5RBy	            */    
0xB842,			/* AWBZone6LTx - A	        */
0xB985,			/* AWBZone6LTy	            */
0xBA64,			/* AWBZone6RBx	            */
0xBB69,			/* AWBZone6RBy	            */
0xBC39,			/* AWBZone7LTx - Horizon	  */
0xBDA0,			/* AWBZone7LTy	            */
0xBE59,			/* AWBZone7RBx	            */
0xBF7f,			/* AWBZone7RBy	            */
0xC05B,			/* AWBZone8LTx - Skin	      */
0xC185,			/* AWBZone8LTy	            */
0xC260,			/* AWBZone8RBx	            */
0xC37F,			/* AWBZone8RBy   	          */

/*==================================*/
/* UR                               */
/*==================================*/
0xFF87,
0xC922, 	 /* AWBTrim */
0xFF86,
0x14DE,	   /* CCM sum 1 */

0xFF85,
0x0605,	  /* Saturation CCM 1    */
0x8640, 	/* 42 saturation level */
0x0700,	  /* sup hysteresis      */

/*==================================*/						
/* CCM D65						              */
/*==================================*/
0xFF83,		
0xEA00,	/*gAwb_s16AdapCCMTbl_0  */
0xEB72,	/*gAwb_s16AdapCCMTbl_1  */
0xECFF,	/*gAwb_s16AdapCCMTbl_2  */
0xEDC9,	/*gAwb_s16AdapCCMTbl_3  */
0xEEff,	/*gAwb_s16AdapCCMTbl_4  */
0xEFF8,	/*gAwb_s16AdapCCMTbl_5  */
0xF0FF,	/*gAwb_s16AdapCCMTbl_6  */
0xF1ED,	/*gAwb_s16AdapCCMTbl_7  */
0xF200,	/*gAwb_s16AdapCCMTbl_8  */
0xF356,	/*gAwb_s16AdapCCMTbl_9  54  */
0xF4FF,	/*gAwb_s16AdapCCMTbl_10 */
0xF5F1,	/*gAwb_s16AdapCCMTbl_11 */
0xF6FF,	/*gAwb_s16AdapCCMTbl_12 */
0xF7FF,	/*gAwb_s16AdapCCMTbl_13 */
0xF8FF,	/*gAwb_s16AdapCCMTbl_14 */
0xF9C8,	/*gAwb_s16AdapCCMTbl_15 */
0xFA00,	/*gAwb_s16AdapCCMTbl_16 */
0xFB6E,	/*gAwb_s16AdapCCMTbl_17 */
					  
/* CWF lgiht */         
0xFC00,    /* gAwb_s16AdapCCMTbl_18 */ 
0xFD78,    /* gAwb_s16AdapCCMTbl_19 */ 
0xFF85,
0xE0FF,	 /* gAwb_s16AdapCCMTbl_20  */ 
0xE1D2,	 /* gAwb_s16AdapCCMTbl_21  */
0xFF84, 
0x00FF,     /* gAwb_s16AdapCCMTbl_22 */
0x01EF,     /* gAwb_s16AdapCCMTbl_23 */
0x02FF,     /* gAwb_s16AdapCCMTbl_24 */
0x03EA,     /* gAwb_s16AdapCCMTbl_25 */
0x0400,     /* gAwb_s16AdapCCMTbl_26 */
0x055B,     /* gAwb_s16AdapCCMTbl_27 */
0x06FF,     /* gAwb_s16AdapCCMTbl_28 */
0x07F5,     /* gAwb_s16AdapCCMTbl_29 */
0x08FF,     /* gAwb_s16AdapCCMTbl_30 */
0x09F5,     /* gAwb_s16AdapCCMTbl_31 */
0x0AFF,     /* gAwb_s16AdapCCMTbl_32 */
0x0BC0,     /* gAwb_s16AdapCCMTbl_33 */
0x0C00,     /* gAwb_s16AdapCCMTbl_34 */
0x0D84,     /* gAwb_s16AdapCCMTbl_35 */

/* A light Green */
0x0E00,     /* gAwb_s16AdapCCMTbl_36 */
0x0F67,	    /* gAwb_s16AdapCCMTbl_37 */  
0x10FF,     /* gAwb_s16AdapCCMTbl_38 */
0x11d8,  	  /* gAwb_s16AdapCCMTbl_39 */
0x12ff,     /* gAwb_s16AdapCCMTbl_40 */
0x13ff,     /* gAwb_s16AdapCCMTbl_41 */
0x14FF,     /* gAwb_s16AdapCCMTbl_42 */
0x15ED,     /* gAwb_s16AdapCCMTbl_43 */
0x1600,     /* gAwb_s16AdapCCMTbl_44 */
0x1759,     /* gAwb_s16AdapCCMTbl_45 */
0x18ff,     /* gAwb_s16AdapCCMTbl_46 */
0x19f2,     /* gAwb_s16AdapCCMTbl_47 */
0x1Aff,     /* gAwb_s16AdapCCMTbl_48 */
0x1BEA,     /* gAwb_s16AdapCCMTbl_49 */
0x1CFF,     /* gAwb_s16AdapCCMTbl_50 */
0x1DbC,     /* gAwb_s16AdapCCMTbl_51 */
0x1E00,     /* gAwb_s16AdapCCMTbl_52 */
0x1F8d,     /* gAwb_s16AdapCCMTbl_53 */

/* Out door CCM */
0xFF86,
0x4501,    /* CCM LuxThreshold */
0x4600,    /* CCM LuxThreshold */
0xFF85,
0xFEB1,	   /* Outdoor CCM On   */

0xFF85,
0xEC00,     /* gAwb_s16AdapCCMTbl_0  */
0xED75,     /* gAwb_s16AdapCCMTbl_1  */
0xEEFF,     /* gAwb_s16AdapCCMTbl_2  */
0xEFB8,     /* gAwb_s16AdapCCMTbl_3  */
0xF000,     /* gAwb_s16AdapCCMTbl_4  */
0xF113,     /* gAwb_s16AdapCCMTbl_5  */
0xF2FF,     /* gAwb_s16AdapCCMTbl_6  */
0xF3F7,     /* gAwb_s16AdapCCMTbl_7  */
0xF400,     /* gAwb_s16AdapCCMTbl_8  */
0xF554,     /* gAwb_s16AdapCCMTbl_9  */
0xF6FF,     /* gAwb_s16AdapCCMTbl_10 */
0xF7F5,     /* gAwb_s16AdapCCMTbl_11 */
0xF8FF,     /* gAwb_s16AdapCCMTbl_12 */
0xF9F1,     /* gAwb_s16AdapCCMTbl_13 */
0xFAFF,     /* gAwb_s16AdapCCMTbl_14 */
0xFBC7,     /* gAwb_s16AdapCCMTbl_15 */
0xFC00,     /* gAwb_s16AdapCCMTbl_16 */
0xFD88,     /* gAwb_s16AdapCCMTbl_17 */
						
/*==================================*/						
/* ISP Global Control #1						*/
/*==================================*/						

0xFFA0,
0x1040,			// BLC Th	
0x1109,			// BLC Separator	
0x6073,			// CDC : Dark CDC ON  	
0x611F,			// Six CDC_Edge En, Slash EN	
0x62FE,	
0x6391,	    // EdgThresCtrl1 
0x64FD,	    // EdgThresCtrl2 
0x65A0,	    // ClipMode 
0x66A0,	    // DivRatio 
0x6720,	    // DivLoBD 
0x68B0,	    // DivUpBD 			
0x6920,			// Slash direction Line threshold	
0x6A40,			// Slash direction Pixel threshold	
0x6B10,	    // LineTH_SL 
0x6C40,	    // PixeTH_SL 
0x6D60,	    // TBase_SDC 
0x6E60,	    // TBase_DDC 
0x6F00,	    // Slope_Sign 
0x7000,				
0x7116,				
0x7200,				
0x7304,	
0x740F,	    // YOff0_DDC 
0x7520,	    // YOff1_DDC 			
0xC012,			// NSF: Th1	
0xC130,			// NSF: Th2	
0xC208,			// NSF: Th3	
0xD071,			// DEM: pattern detection	
0xD100,			// False color threshold	

/* LSC */
0xFF85,
0x0F43,	  /* LVLSC lux level threshold          */
0x10E3,	  /* LSLSC Light source , threshold lux */
0x1BE0,

/* Lens Shading */
0xFFA0,
0x4380, 	  /* RH7 rrhrhh */
0x4480, 	  /* RV         */
0x4580, 	  /* RH	        */
0x4680, 	  /* RV         */
0x4780, 	  /* GH         */
0x4880, 	  /* GV         */
0x4980, 	  /* GH         */
0x4A80, 	  /* GV	        */
0x4B80, 	  /* BH         */
0x4C80,	    /* BV         */
0x4D80, 	  /* BH         */
0x4E80, 	  /* BV         */

0x5290, 	  /* GGain      */
0x5320, 	  /* GGain      */
0x5400, 	  /* GGain      */

/* Max Shading */
0xFF86,
0x51A0,	  /*Rgain1*/
0x5230, 	/*Rgain2*/
0x5300,	  /*Rgain3*/

0x5480, 	/*Bgain1*/
0x5520, 	/*Bgain2*/
0x5600,	  /*Bgain3*/

0x5790, 	/* GGain1 */
0x5820, 	/* GGain2 */
0x5900, 	/* GGain3 */

/* Min Shading */
0x48B0,	  /*Rgain1*/
0x4920,	  /*Rgain2*/
0x4A00,	  /*Rgain3*/

0x4B80, 	  /*Bgain1*/
0x4C18, 	  /*Bgain2*/	
0x4D00, 	  /*Bgain3*/

0x4E90, 	  /* GGain1 */
0x4F20, 	  /* GGain2 */
0x5000, 	  /* GGain3 */
						
/*==================================*/						
/* ISP Global Control #2						*/
/*==================================*/						
						
0xFFA1,												
0x3030,			/* EDE: Luminane adaptation off	*/
0x3122,			/* EDE: SlopeGain	              */
0x3250,			/* EDE: Adaptation slope 	      */
0x3300,			/* EDE : PreCoringPt	          */
0x3400,			/* EDE: x1 point	              */
0x3516,			/* EDE: x1 point 	              */
0x3600,			/* EDE: x2 point	              */
0x3730,			/* EDE: x2 point 	              */
0x3809,			/* EDE: TransFuncSl1	          */
0x3922,			/* EDE: TransFuncSl2	          */
0x3A08,			/* EDE: Adaptation left margin	*/
0x3B08,			/* EDE: Adaptation right margin	*/
0x3C08,			/* EDE: RGB edge threshol	      */
0x3D00,			/* EDE: SmallOffset						  */
						                               

/*==================================*/						
/* ADF						                  */
/*==================================*/						

0xFF85,						
0x04FB,			/* FB EnEDE (v1.6)	   */
0x0605,			/* Flag	           */
0x0700,			/* sup hysteresis	 */

/*--------------------*/
/* ADF BGT	          */
/*--------------------*/
0x1732,			/* Th_BGT	 */
0x2620,			/* Min_BGT	*/
0x3C00,			/* Max_BGT	*/              
              
/*--------------------*/
/* ADF CNT            */
/*--------------------*/
0x18CB,			/* Th_CNT	  */
0x2700,			/* Min_CON	*/
0x3D02,			/* Max_CON	*/

/*----------------*/
/* ADF on         */
/*----------------*/
0xFF85,
0x04FB,	/* [7]CNF [3]NSF ADF on */
0xFF86,
0x14DE,	/* [7]ADF on for EDE new features */

/*----------------*/
/* ADF - CNFTh1   */
/*----------------*/
0xFF86,
0x7C03,	/* lux threshold for CNFTh1	*/
0xFF85,
0x24B3,	/* CNFTh1 @ current_lux <  lux_thrd(0x867C)*/
0x3AC3,	/* CNFTh1 @ current_lux >= lux_thrd(0x867C)*/
/*----------------*/
/* ADF - CNFTh2   */
/*----------------*/
0xFF85,
0x13ED,	/* lux threshold for CNFTh2([7:4] : max_lux, [3:0] min_lux)*/ 
0x2509,	/* CNFTh2 @ min lux level                                  */
0x3B03,	/* CNFTh2 @ max lux level                                  */
/*----------------*/
/* ADF - CNFTh3, 4*/ 
/*----------------*/
0xFF86,
0x7D04,	/* CNFTh3 @ #0 light source [0x83D3:0x00] - cloudy, daylight, flourscent*/
0x8019,	/* CNFTh4 @ #0 light source [0x83D3:0x00] - cloudy, daylight, flourscent*/
0x7E00,	/* CNFTh3 @ #1 light source [0x83D3:0x01] - CWF, TL84                   */
0x8100,	/* CNFTh4 @ #1 light source [0x83D3:0x01] - CWF, TL84                   */
0x7F00,	/* CNFTh3 @ #2 light source [0x83D3:0x02] - A, Horizon                  */
0x8200,	/* CNFTh4 @ #2 light source [0x83D3:0x02] - A, Horizon                  */
/*----------------                     */
/* HW address (if CNF ADF turned off)  */
/*----------------                     */
0xFFA1,
0x4607,    /* CNFTh5 HW only */

/*----------------*/
/* ADF - NSF1/2/3 */
/*----------------*/  
0xFF85,
0x12A4,	/* Th_NSF	      */
0x221E,	/* Min_NSF_Th1	 */
0x2350,	/* Min_NSF_Th2	 */
0x3810,	/* Max_NSF_Th1	 */
0x3928,	/* Max_NSF_Th2   */
0xFF86,	                                             
0x1206,	/* Min_NSF_Th3	  */
0x1306,	/* Max_NSF_Th3   */
0xFF85,	    
0xE85C,	/* u8ADF_STVal_NSF_TH1 //patch initial value @ firmware 2.0  */
0xE962,	/* u8ADF_STVal_NSF_TH2 //patch initial value @ firmware 2.0  */
0xEA0E,	/* u8ADF_EndVal_NSF_TH1 //patch initial value @ firmware 2.0 */
0xEB18,	/* u8ADF_EndVal_NSF_TH2 //patch initial value @ firmware 2.0 */

/*----------------*/
/* ADF - NSF4     */
/*----------------*/
0xFF86,
0xA285,	/* Lux. level threshold for NSF4 */
0xA500,	/* Min_NSF_Th4                   */
0xA808,	/* Max_NSF_Th4                   */
/*----------------*/
/* ADF - NSF5/6   */
/*----------------*/
0xA30B,	/* Lux. level threshold for NSF5/6 */
0xA624,	/* Min_NSF_Th5                     */
0xA944,	/* Max_NSF_Th5                     */
0xA702,	/* Min_NSF_Th6                     */
0xAA00,	/* Max_NSF_Th6                     */

/*----------------*/
/* ADF - EDE new  */
/*----------------*/
0xC00E,	/* Lux level threshold for EDEOption            */
0xC130,	/* EDEOption value over lux level threshold     */
0xC230,	/* EDEOption value under lux level threshol     */
0xC330,	/* EDESmEdgThrd value over lux level threshold  */
0xC416,	/* EDESmEdgThrd value under lux level threshold */
0xC5A5,	/* Lux level threshold for EDESmEdgGain         */
0xC602,	/* EDESmEdgGain value @ min lux level           */
0xC703,	/* EDESmEdgGain value @ max lux level           */

/*--------------------*/                          
/* ADF - GDC          */
/*--------------------*/ 
0xFF85, 
0x1543,	/* GDC lux level threshold */  
0x2D40,	/* Min_GDC_Th1             */
0x2E80,	/* Min_GDC_Th2             */
0x4308,	/* Max_GDC_Th1             */
0x4430,	/* Max_GDC_Th2             */

/*--------------------*/
/* ADF Edge           */
/*--------------------*/
0x14B3,			/* Th_Edge	            */
0x2806,			/* Min_Coring	          */
0x2906,			/* Min_Edge_Slope1	    */
0x2A07,			/* Min_Edge_Slope2	    */
0x2B06,			/* Min_Edge_SmallOffset */
0x2C22,			/* Min_Edge_Slope	      */
0x3E02,			/* Max_Coring	          */
0x3F08,			/* Max_Edge_Slope1	    */
0x4009,			/* Max_Edge_Slope2	    */
0x4106,			/* Max_Edge_SmallOffset */
0x4222,			/* Max_Edge_Slope	      */

/*--------------------*/
/* ADF Gamma          */
/*--------------------*/
0xFF86,
0xBF01,   /* DarkGMA Threshold */
0xAE00,		/* DarkGMA_0         */
0xAF04,		/* DarkGMA_0         */
0xB008,		/* DarkGMA_0         */
0xB110,		/* DarkGMA_0         */
0xB218,		/* DarkGMA_0         */
0xB320,		/* DarkGMA_0         */
0xB430,		/* DarkGMA_0         */
0xB540,		/* DarkGMA_0         */
0xB650,		/* DarkGMA_0         */
0xB760,		/* DarkGMA_0         */
0xB880,		/* DarkGMA_0         */
0xB9A0,		/* DarkGMA_0         */
0xBAC0,		/* DarkGMA_0         */
0xBBD0,		/* DarkGMA_0         */
0xBCE0,		/* DarkGMA_0         */
0xBDF0,		/* DarkGMA_0         */
0xBEFF,		/* DarkGMA_0         */

0xFF85,
0x1670,   /* Gamma Threshold */
0x4700,	  /* Min_Gamma_0     */
0x4816,	  /* Min_Gamma_1     */
0x4925,	  /* Min_Gamma_2     */
0x4A37,	  /* Min_Gamma_3     */
0x4B45,	  /* Min_Gamma_4     */
0x4C51,	  /* Min_Gamma_5     */
0x4D65,	  /* Min_Gamma_6     */
0x4E77,	  /* Min_Gamma_7     */
0x4F87,	  /* Min_Gamma_8     */
0x5095,	  /* Min_Gamma_9     */
0x51AF,	  /* Min_Gamma_10    */
0x52C6,	  /* Min_Gamma_11    */
0x53DB,	  /* Min_Gamma_12    */
0x54E5,	  /* Min_Gamma_13    */
0x55EE,	  /* Min_Gamma_14    */
0x56F7,	  /* Min_Gamma_15    */
0x57FF,	  /* Min_Gamma_16    */

0x5800,	  /* Max_Gamma_0  */ 
0x5902,	  /* Max_Gamma_1  */
0x5a14,	  /* Max_Gamma_2  */
0x5b35,	  /* Max_Gamma_3  */
0x5c53,	  /* Max_Gamma_4  */
0x5d66,	  /* Max_Gamma_5  */
0x5e7E,	  /* Max_Gamma_6  */
0x5f90,	  /* Max_Gamma_7  */
0x609F,	  /* Max_Gamma_8  */
0x61AC,	  /* Max_Gamma_9  */
0x62C2,	  /* Max_Gamma_10 */
0x63D7,	  /* Max_Gamma_11 */
0x64E7,	  /* Max_Gamma_12 */
0x65EE,	  /* Max_Gamma_13 */
0x66F3,	  /* Max_Gamma_14 */
0x67F9,	  /* Max_Gamma_15 */
0x68FF,	  /* Max_Gamma_16 */

/*--------------------*/ 
/* Suppression	      */
/*--------------------*/
0x1A21,			/* Th_SUP	          */
0x3080,			/* Min_suppression	*/
0x4680,			/* Max_suppression  */

/*--------------------*/ 
/* CDC	              */
/*--------------------*/
0x1144,			// Th_CDC	
0xFF86,
0xE102,	    // u8CDCDarkTh 
0xFF85,
0x6960,			// CDCMin_TBaseDDC	
0x6A00,			// CDCMin_SlopeSign	
0x6B15,			// CDCMin_SlopeDDC	
0x6C0F,			// CDCMin_YOff0	
0x6D20,			// CDCMin_YOff1	
0x6E48,			// CDCMax_TBaseDDC	
0x6F00,			// CDCMax_SlopeSign	
0x70F4,			// CDCMax_SlopeDDC	
0x713C,			// CDCMax_YOff0	
0x7244,			// CDCMax_YOff1	
0xFF86,						
0xE001,	    // u8CDCEn - forcibly make dark cdc enable
0xE27F,	    // u8DarkCdcCtrlA
0xE320,	    // u8DarkTBase_DDC
0xE405,	    // u8DarkSlope_DDC
0xE500,	    // u8DarkYOff0_DDC
0xE60F,	    // u8DarkYOff1_DDC			
						
/*==================================*/						
/* ADF Update						            */
/*==================================*/	
0xFF85,					
0x0910,			/* CurLux	*/					
						
/*==================================*/						
/* Saturation						            */
/*==================================*/						
0x8640,			/* Color Saturation	*/

/* BGT2 */
0xFF86,
0x6800,	  /* BGTLux1 */
0x6900,	  /* BGTLux2 */
0x6a00,	  /* BGTLux3 */
0x6b00,	  /* Value1  */
0x6c00,	  /* Value2  */
0x6d00,	  /* Value3  */

/* SAT */ 
0x6F00,	  /*SATLux1 Gamut             */
0x7000,	  /*SATLux2                   */
0x7100,	  /*SATZone                   */
0x7200,	  /*SATZone                   */
0x7300,	  /*SATValue1    D65 Gamut    */
0x7400,	  /*SATValue2                 */


/* LineLength*/
0xFF87, /*Page mode */
0xDC05,
0xDDB0,
0xd500, /* Flip*/


/*==================================*/
/* MIPI                             */
/*==================================*/
0xFFB0, /*Page mode*/
0x5402, /* MIPI PHY_HS_TX_CTRL*/
0x3805, /* MIPI DPHY_CTRL_SET*/
0x3C81,
0x5012, /*MIPI.PHY_HL_TX_SEL */
0x5884, /* MIPI setting  추가 setting */
0x5921, /* MIPI setting  추가 setting */

/* SXGA PR*/
0xFF85, /*Page mode */
0xB71E,
0xB80A, /* gPT_u8PR_Active_SXGA_WORD_COUNT_Addr0*/
0xB900, /* gPT_u8PR_Active_SXGA_WORD_COUNT_Addr1*/
0xBC04, /* gPT_u8PR_Active_SXGA_DPHY_CLK_TIME_Addr3*/
0xBF05, /* gPT_u8PR_Active_SXGA_DPHY_DATA_TIME_Addr3*/
0xFF87, /* Page mode*/
0x0C00, /* start Y*/
0x0D20, /* start Y*/
0x1003, /* end Y*/
0x11E0, /* end Y*/

0xFF86, /*u8PLL_Ctrl_Addr*/
0xFE40,

/* Recoding*/
0xFF86, /*Page mode*/
0x371E,
0x3805, /*gPT_u8PR_Active_720P_WORD_COUNT_Addr0*/
0x3900, /*gPT_u8PR_Active_720P_WORD_COUNT_Addr1*/
0x3C04, /*gPT_u8PR_Active_720P_DPHY_CLK_TIME_Addr3*/
0x3F05, /* gPT_u8PR_Active_720P_DPHY_DATA_TIME_Addr3*/

0xFF87,
0x2302, /*gPR_Active_720P_u8SensorCtrl_Addr*/
0x2472, /*gPR_Active_720P_u8SensorMode_Addr*/
0x2501, /*gPR_Active_720P_u8PLL_P_Addr*/
0x260F, /*gPR_Active_720P_u8PLL_M_Addr*/
0x2700, /*gPR_Active_720P_u8PLL_S_Addr*/
0x2840, /*gPR_Active_720P_u8PLL_Ctrl_Addr*/
0x2901, /*gPR_Active_720P_u8src_clk_sel_Addr*/
0x2A00, /*gPR_Active_720P_u8output_pad_status_Addr*/
0x2B3F, /*gPR_Active_720P_u8ablk_ctrl_10_Addr*/
0x2CFF, /*gPR_Active_720P_u8BayerFunc_Addr*/
0x2DFF, /*gPR_Active_720P_u8RgbYcFunc_Addr*/
0x2E00, /*gPR_Active_720P_u8ISPMode_Addr*/
0x2F02, /*gPR_Active_720P_u8SCLCtrl_Addr*/
0x3001, /*gPR_Active_720P_u8SCLHorScale_Addr0*/
0x31FF, /*gPR_Active_720P_u8SCLHorScale_Addr1*/
0x3203, /*gPR_Active_720P_u8SCLVerScale_Addr0*/
0x33FF, /*gPR_Active_720P_u8SCLVerScale_Addr1*/
0x3400, /*gPR_Active_720P_u8SCLCropStartX_Addr0*/
0x3500, /*gPR_Active_720P_u8SCLCropStartX_Addr1*/
0x3600, /*gPR_Active_720P_u8SCLCropStartY_Addr0*/
0x3710, /*gPR_Active_720P_u8SCLCropStartY_Addr1*/
0x3802, /*gPR_Active_720P_u8SCLCropEndX_Addr0*/
0x3980, /*gPR_Active_720P_u8SCLCropEndX_Addr1*/
0x3A01, /*gPR_Active_720P_u8SCLCropEndY_Addr0*/
0x3BF0, /*gPR_Active_720P_u8SCLCropEndY_Addr1*/
0x3C01, /*gPR_Active_720P_u8OutForm_Addr*/
0x3D0C, /*gPR_Active_720P_u8OutCtrl_Addr*/
0x3E04, /*gPR_Active_720P_u8AEWinStartX_Addr*/
0x3F04, /*gPR_Active_720P_u8AEWinStartY_Addr*/
0x4066, /*gPR_Active_720P_u8MergedWinWidth_Addr*/
0x415E, /*gPR_Active_720P_u8MergedWinHeight_Addr*/
0x4204, /*gPR_Active_720P_u8AEHistWinAx_Addr*/
0x4304, /*gPR_Active_720P_u8AEHistWinAy_Addr*/
0x4498, /*gPR_Active_720P_u8AEHistWinBx_Addr*/
0x4578, /*gPR_Active_720P_u8AEHistWinBy_Addr*/
0x4622, /*gPR_Active_720P_u8AWBTrim_Addr*/
0x4728, /*gPR_Active_720P_u8AWBCTWinAx_Addvr*/
0x4820, /*gPR_Active_720P_u8AWBCTWinAy_Addr*/
0x4978, /*gPR_Active_720P_u8AWBCTWinBx_Addr*/
0x4A60, /*gPR_Active_720P_u8AWBCTWinBy_Addr*/
0x4B03, /*gPR_Active_720P_u16AFCFrameLength_0*/
0x4C00, /*gPR_Active_720P_u16AFCFrameLength_1*/

/*VGA PR*/
0xFF86, /*Page mode*/
0x2E1E,
0x2F05, /* gPT_u8PR_Active_VGA_WORD_COUNT_Addr0*/
0x3000, /* gPT_u8PR_Active_VGA_WORD_COUNT_Addr1*/
0x3304, /* gPT_u8PR_Active_VGA_DPHY_CLK_TIME_Addr3*/
0x3605, /* gPT_u8PR_Active_VGA_DPHY_DATA_TIME_Addr3*/

0xFF87, /*Page mode*/
0x4D00, /*gPR_Active_VGA_u8SensorCtrl_Addr*/
0x4E72, /*gPR_Active_VGA_u8SensorMode_Addr*/
0x4F01, /*gPR_Active_VGA_u8PLL_P_Addr*/
0x500F, /*gPR_Active_VGA_u8PLL_M_Addr*/
0x5100, /*gPR_Active_VGA_u8PLL_S_Addr*/
0x5240, /*gPR_Active_VGA_u8PLL_Ctrl_Addr*/
0x5301, /*gPR_Active_VGA_u8src_clk_sel_Addr*/
0x5400, /*gPR_Active_VGA_u8output_pad_status_Addr*/
0x553F, /*gPR_Active_VGA_u8ablk_ctrl_10_Addr*/
0x56FF, /*gPR_Active_VGA_u8BayerFunc_Addr*/
0x57FF, /*gPR_Active_VGA_u8RgbYcFunc_Addr*/
0x5800, /*gPR_Active_VGA_u8ISPMode_Addr*/
0x5902, /*gPR_Active_VGA_u8SCLCtrl_Addr*/
0x5A01, /*gPR_Active_VGA_u8SCLHorScale_Addr0*/
0x5BFF, /*gPR_Active_VGA_u8SCLHorScale_Addr1*/
0x5C01, /*gPR_Active_VGA_u8SCLVerScale_Addr0*/
0x5DFF, /*gPR_Active_VGA_u8SCLVerScale_Addr1*/
0x5E00, /*gPR_Active_VGA_u8SCLCropStartX_Addr0*/
0x5F00, /*gPR_Active_VGA_u8SCLCropStartX_Addr1*/
0x6000, /*gPR_Active_VGA_u8SCLCropStartY_Addr0*/
0x6110, /*gPR_Active_VGA_u8SCLCropStartY_Addr1*/
0x6202, /*gPR_Active_VGA_u8SCLCropEndX_Addr0*/
0x6380, /*gPR_Active_VGA_u8SCLCropEndX_Addr1*/
0x6401, /*gPR_Active_VGA_u8SCLCropEndY_Addr0*/
0x65F0, /*gPR_Active_VGA_u8SCLCropEndY_Addr1*/
	
/*
 Command Preview Fixed 8fps
*/
};

/* Set-data based on Samsung Reliabilty Group standard */
/* ,when using WIFI. 15fps*/
static const u16 db8131m_vt_wifi_common[] = {
/*
 Command Preview Fixed 15fps
*/
};

/*===========================================*/
/* CAMERA_PREVIEW - 촬영 후 프리뷰 복귀시 셋팅 */
/*============================================*/
static const u16 db8131m_vga_capture[] = {
0xffC0,
0x1042,
0xE796,		/*Wait  150*/
};

static const u16 db8131m_preview[] = {
0xFFC0, 
0x1041,
0xE764,
0xFFB0,
0x40BB,
};

static const u16 db8131m_stream_stop[] = {
0xFFB0,
0x4088,
};

/*===========================================
	CAMERA_SNAPSHOT
============================================*/
static const u16 db8131m_capture[] = {
0xffC0,
0x1043,
0xE796,	/*Wait  100*/
0xFFB0,
0x40BB,
};

static const u16 db8131m_720p_common[] = {
};

static const u16 db8131m_vga_common[] = {
};


/*------------------------------------------*/
// Size - VGA 640x480
static const u16 db8131m_size_vga[] = {
0xFFB0,	// Page Mode
0x1405,		// MIPI_VGA_WORD_COUNT_Addr0
0x1500,		// MIPI_VGA_WORD_COUNT_Addr1

0xFFD0, //Page Mode
0x7B96,		// HValidOffset

0xFF87,	  // Page Mode
0xB002,
0xB101,
0xB2FF,
0xB301,
0xB4FF,
0xB500,		// UR_VGA_u8SCLCropStartX_Addr0
0xB600,		// UR_VGA_u8SCLCropStartX_Addr1
0xB700,		// UR_VGA_u8SCLCropStartY_Addr0
0xB810,		// UR_VGA_u8SCLCropStartY_Addr1
0xB902,		// UR_VGA_u8SCLCropEndX_Addr0
0xBA80,		// UR_VGA_u8SCLCropEndX_Addr1
0xBB01,		// UR_VGA_u8SCLCropEndY_Addr0
0xBCF0,		// UR_VGA_u8SCLCropEndY_Addr1

0xFF86,	// PR
0x2F05,		// gPT_u8PR_Active_VGA_WORD_COUNT_Addr0
0x3000,		// gPT_u8PR_Active_VGA_WORD_COUNT_Addr1

0xFF85, // Page Mode
0xB396,		// u8PR_Active_VGA_HvalidOffset_Addr

0xFF87,	// Page Mode
0x5902,		// gPR_Active_VGA_u8SCLCtrl_Addr
0x5A01,		// gPR_Active_VGA_u8SCLHorScale_Addr0
0x5BFF,		// gPR_Active_VGA_u8SCLHorScale_Addr1
0x5C01,		// gPR_Active_VGA_u8SCLVerScale_Addr0
0x5DFF,		// gPR_Active_VGA_u8SCLVerScale_Addr1
0x5E00,		// gPR_Active_VGA_u8SCLCropStartX_Addr0
0x5F00,		// gPR_Active_VGA_u8SCLCropStartX_Addr1
0x6000,		// gPR_Active_VGA_u8SCLCropStartY_Addr0
0x6110,		// gPR_Active_VGA_u8SCLCropStartY_Addr1
0x6202,		// gPR_Active_VGA_u8SCLCropEndX_Addr0
0x6380,		// gPR_Active_VGA_u8SCLCropEndX_Addr1
0x6401,		// gPR_Active_VGA_u8SCLCropEndY_Addr0
0x65F0,		// gPR_Active_VGA_u8SCLCropEndY_Addr1
};

/*------------------------------------------*/
// Size - QVGA 320x240
static const u16 db8131m_size_qvga[] = {
0xFFB0,	// Page Mode
0x1402,		// MIPI_VGA_WORD_COUNT_Addr0
0x1580,		// MIPI_VGA_WORD_COUNT_Addr1

0xFFD0, //Page Mode
0x7B96,		// HValidOffset

0xFF87,	// Page Mode
0xB002,   // UR_VGA_u8SCLCtrl_Addr
0xB100,   // UR_VGA_u8SCLHorScale_Addr0 
0xB2FF,   // UR_VGA_u8SCLHorScale_Addr1 
0xB300,   // UR_VGA_u8SCLVerScale_Addr0 
0xB4FF,   // UR_VGA_u8SCLVerScale_Addr1 
0xB500,		// UR_VGA_u8SCLCropStartX_Addr0
0xB600,		// UR_VGA_u8SCLCropStartX_Addr1
0xB700,		// UR_VGA_u8SCLCropStartY_Addr0
0xB800,		// UR_VGA_u8SCLCropStartY_Addr1
0xB901,		// UR_VGA_u8SCLCropEndX_Addr0
0xBA40,		// UR_VGA_u8SCLCropEndX_Addr1
0xBB00,		// UR_VGA_u8SCLCropEndY_Addr0
0xBCF0,		// UR_VGA_u8SCLCropEndY_Addr1

0xFF86,	// PR
0x2F02,		// gPT_u8PR_Active_VGA_WORD_COUNT_Addr0
0x3080,		// gPT_u8PR_Active_VGA_WORD_COUNT_Addr1

0xFF85, // Page Mode
0xB396,		// u8PR_Active_VGA_HvalidOffset_Addr

0xFF87,	// Page Mode
0x5902,		// gPR_Active_VGA_u8SCLCtrl_Addr
0x5A00,		// gPR_Active_VGA_u8SCLHorScale_Addr0
0x5BFF,		// gPR_Active_VGA_u8SCLHorScale_Addr1
0x5C00,		// gPR_Active_VGA_u8SCLVerScale_Addr0
0x5DFF,		// gPR_Active_VGA_u8SCLVerScale_Addr1
0x5E00,		// gPR_Active_VGA_u8SCLCropStartX_Addr0
0x5F00,		// gPR_Active_VGA_u8SCLCropStartX_Addr1
0x6000,		// gPR_Active_VGA_u8SCLCropStartY_Addr0
0x6100,		// gPR_Active_VGA_u8SCLCropStartY_Addr1
0x6201,		// gPR_Active_VGA_u8SCLCropEndX_Addr0
0x6340,		// gPR_Active_VGA_u8SCLCropEndX_Addr1
0x6400,		// gPR_Active_VGA_u8SCLCropEndY_Addr0
0x65F0,		// gPR_Active_VGA_u8SCLCropEndY_Addr1
};

/*------------------------------------------*/
// Size - CIF 352x288
static const u16 db8131m_size_cif[] = {
0xFFB0,	// Page Mode
0x1402,		// MIPI_VGA_WORD_COUNT_Addr0
0x15C0,		// MIPI_VGA_WORD_COUNT_Addr1

0xFFD0, //Page Mode
0x7B96,		// HValidOffset

0xFF87,	// Page Mode
0xB002,   // UR_VGA_u8SCLCtrl_Addr        
0xB101,   // UR_VGA_u8SCLHorScale_Addr0   
0xB233,   // UR_VGA_u8SCLHorScale_Addr1   
0xB301,   // UR_VGA_u8SCLVerScale_Addr0   
0xB433,   // UR_VGA_u8SCLVerScale_Addr1   
0xB500,		// UR_VGA_u8SCLCropStartX_Addr0
0xB610,		// UR_VGA_u8SCLCropStartX_Addr1
0xB700,		// UR_VGA_u8SCLCropStartY_Addr0
0xB800,		// UR_VGA_u8SCLCropStartY_Addr1
0xB901,		// UR_VGA_u8SCLCropEndX_Addr0
0xBA70,		// UR_VGA_u8SCLCropEndX_Addr1
0xBB01,		// UR_VGA_u8SCLCropEndY_Addr0
0xBC20,		// UR_VGA_u8SCLCropEndY_Addr1

0xFF86,	// PR
0x2F02,		// gPT_u8PR_Active_VGA_WORD_COUNT_Addr0
0x30C0,		// gPT_u8PR_Active_VGA_WORD_COUNT_Addr1

0xFF85, // Page Mode
0xB396,		// u8PR_Active_VGA_HvalidOffset_Addr

0xFF87,	// Page Mode
0x5902,		// gPR_Active_VGA_u8SCLCtrl_Addr
0x5A01,		// gPR_Active_VGA_u8SCLHorScale_Addr0
0x5B33,		// gPR_Active_VGA_u8SCLHorScale_Addr1
0x5C01,		// gPR_Active_VGA_u8SCLVerScale_Addr0
0x5D33,		// gPR_Active_VGA_u8SCLVerScale_Addr1
0x5E00,		// gPR_Active_VGA_u8SCLCropStartX_Addr0
0x5F10,		// gPR_Active_VGA_u8SCLCropStartX_Addr1
0x6000,		// gPR_Active_VGA_u8SCLCropStartY_Addr0
0x6100,		// gPR_Active_VGA_u8SCLCropStartY_Addr1
0x6201,		// gPR_Active_VGA_u8SCLCropEndX_Addr0
0x6370,		// gPR_Active_VGA_u8SCLCropEndX_Addr1
0x6401,		// gPR_Active_VGA_u8SCLCropEndY_Addr0
0x6520,		// gPR_Active_VGA_u8SCLCropEndY_Addr1
};

/*------------------------------------------*/
// Size - QCIF 176x144
static const u16 db8131m_size_qcif[] = {
0xFFB0,	// Page Mode
0x1401,		// MIPI_VGA_WORD_COUNT_Addr0
0x1560,		// MIPI_VGA_WORD_COUNT_Addr1

0xFFD0, //Page Mode
0x7B96,		// HValidOffset

0xFF87,	// Page Mode
0xB002,   // UR_VGA_u8SCLCtrl_Addr        
0xB100,   // UR_VGA_u8SCLHorScale_Addr0   
0xB299,   // UR_VGA_u8SCLHorScale_Addr1   
0xB300,   // UR_VGA_u8SCLVerScale_Addr0   
0xB499,   // UR_VGA_u8SCLVerScale_Addr1   
0xB500,		// UR_VGA_u8SCLCropStartX_Addr0
0xB608,		// UR_VGA_u8SCLCropStartX_Addr1
0xB700,		// UR_VGA_u8SCLCropStartY_Addr0
0xB800,		// UR_VGA_u8SCLCropStartY_Addr1
0xB900,		// UR_VGA_u8SCLCropEndX_Addr0
0xBAB8,		// UR_VGA_u8SCLCropEndX_Addr1
0xBB00,		// UR_VGA_u8SCLCropEndY_Addr0
0xBC90,		// UR_VGA_u8SCLCropEndY_Addr1

0xFF86,	// PR
0x2F01,		// gPT_u8PR_Active_VGA_WORD_COUNT_Addr0
0x3060,		// gPT_u8PR_Active_VGA_WORD_COUNT_Addr1

0xFF85, // Page Mode
0xB396,		// u8PR_Active_VGA_HvalidOffset_Addr

0xFF87,	// Page Mode
0x5902,		// gPR_Active_VGA_u8SCLCtrl_Addr
0x5A00,		// gPR_Active_VGA_u8SCLHorScale_Addr0
0x5B99,		// gPR_Active_VGA_u8SCLHorScale_Addr1
0x5C00,		// gPR_Active_VGA_u8SCLVerScale_Addr0
0x5D99,		// gPR_Active_VGA_u8SCLVerScale_Addr1
0x5E00,		// gPR_Active_VGA_u8SCLCropStartX_Addr0
0x5F08,		// gPR_Active_VGA_u8SCLCropStartX_Addr1
0x6000,		// gPR_Active_VGA_u8SCLCropStartY_Addr0
0x6100,		// gPR_Active_VGA_u8SCLCropStartY_Addr1
0x6200,		// gPR_Active_VGA_u8SCLCropEndX_Addr0
0x63B8,		// gPR_Active_VGA_u8SCLCropEndX_Addr1
0x6400,		// gPR_Active_VGA_u8SCLCropEndY_Addr0
0x6590,		// gPR_Active_VGA_u8SCLCropEndY_Addr1
};

/*===========================================*/
/*	CAMERA_RECORDING WITH 15fps  */
/*============================================*/

static const u16 db8131m_recording_60Hz_common[] = {
/*================================================================
Device : DB8131M
MIPI Interface for Noncontious Clock
================================================================*/

/*Recording Anti-Flicker 60Hz END of Initial*/
0xE796, /*Wait  150*/

0xFF82,
0x9102,

0xFF83,
0x0B04,
0x0C4C,
0x1104,
0x1276,

0x0308,
0x0406,
0x0504,
0x0604,
0x070C,
0x0807,
0x0903,
0x0A03,

0xFF82,
0x925D,
};

static const u16 db8131m_recording_50Hz_common[] = {
0xE796, /*Wait  150*/

0xFF82,
0x9102,

0xFF83,
0x0B04,
0x0C4C,
0x1104,
0x1276,

0x0308,
0x0406,
0x0504,
0x0604,
0x070C,
0x0807,
0x0903,
0x0A03,

0xFF82,
0x9259,
};


/*=================================
*	CAMERA_BRIGHTNESS_1 (1/9) M4   *
==================================*/
static const u16 db8131m_bright_m4[] = {
/* Brightness -4 */
0xFF87,		/* Page mode */
0xAEB0,		/* Brightness*/
};

/*=================================
*	CAMERA_BRIGHTNESS_2 (2/9) M3  *
==================================*/
static const u16 db8131m_bright_m3[] = {
/* Brightness -3 */
0xFF87,		/* Page mode */
0xAED0,		/* Brightness*/
};

/*=================================
	CAMERA_BRIGHTNESS_3 (3/9) M2
==================================*/
static const u16 db8131m_bright_m2[] = {
/* Brightness -2 */
0xFF87,		/* Page mode */
0xAEE0,		/* Brightness*/
};

/*=================================
	CAMERA_BRIGHTNESS_4 (4/9) M1
==================================*/
static const u16 db8131m_bright_m1[] = {

/* Brightness -1 */
0xFF87,		/* Page mode */
0xAEF0,		/* Brightness */
};

/*=================================
	CAMERA_BRIGHTNESS_5 (5/9) Default
==================================*/
static const u16 db8131m_bright_default[] = {
/* Brightness 0 */
0xFF87,		/* Page mode*/
0xAE00,		/* Brightness */
};

/*=================================
	CAMERA_BRIGHTNESS_6 (6/9) P1
==================================*/
static const u16 db8131m_bright_p1[] = {
/* Brightness +1 */
0xFF87,		/* Page mode */
0xAE10,		/* Brightness */
};

/*=================================
	CAMERA_BRIGHTNESS_7 (7/9) P2
==================================*/
static const u16 db8131m_bright_p2[] = {
/* Brightness +2 */
0xFF87,		/* Page mode */
0xAE20,		/* Brightness */
};

/*=================================
	CAMERA_BRIGHTNESS_8 (8/9) P3
==================================*/
static const u16 db8131m_bright_p3[] = {
/* Brightness +3 */
0xFF87,		/* Page mode */
0xAE30,		/* Brightness*/
};

/*=================================
	CAMERA_BRIGHTNESS_9 (9/9) P4
==================================*/
static const u16 db8131m_bright_p4[] = {
/* Brightness +4 */
0xFF87,		/* Page mode */
0xAE50,		/* Brightness */
};


/*******************************************************
*	CAMERA_Auto White Blance
*******************************************************/
/*------------------------------------------*/
// AWB Auto
static const u16 db8131m_WB_Auto[] = {
0xFF86,
0x14DE,
0xFF83,	// Page Mode
0xCB03,		// Min RGain
0xCCCB,
0xCD07,		// Max RGain
0xCE40,
0xCF02,		// Min BGain
0xD080,
0xD106,		// Max BGain
0xD2C9,

0xFF84,
0x4900,		// Threshold_indoor	
0x4A07,			
0x4B01,		// Threshold_outdoor	
0x4C00,	
0x6700,		// AWB_Weight_2	
0x6840,		// AWB_Weight_3	
0x6900,		// AWB_Weight_4	
0x6B00,		// AWB_Weight_6	
};

/*------------------------------------------*/
/* MWB - Daylight */
static const u16 db8131m_WB_Daylight[] = {
0xFF86,
0x14FE,
0xFF83,	// Page Mode
0xCB00,		// Min RGain
0xCC00,
0xCD05,		// Max RGain
0xCE9A,
0xCF05,		// Min BGain
0xD004,
0xD107,		// Max BGain
0xD200,

0xFF84,
0x4900,		// Threshold_indoor	
0x4A00,			
0x4B00,		// Threshold_outdoor	
0x4C00,	
0x6740,		// AWB_Weight_2	
0x6800,		// AWB_Weight_3	
0x6900,		// AWB_Weight_4	
0x6B00,		// AWB_Weight_6	
};

/*------------------------------------------*/
/* MWB - Cloudy */
static const u16 db8131m_WB_Cloudy[] = {
0xFF86,
0x14DE,
0xFF83,	// Page Mode
0xCB06,		// Min RGain
0xCC00,
0xCD06,		// Max RGain
0xCEB0,
0xCF04,		// Min BGain
0xD007,
0xD104,		// Max BGain
0xD247,

0xFF84,
0x4900,		// Threshold_indoor	
0x4A00,			
0x4B00,		// Threshold_outdoor	
0x4C00,	
0x6740,		// AWB_Weight_2	
0x6800,		// AWB_Weight_3	
0x6900,		// AWB_Weight_4	
0x6B00,		// AWB_Weight_6	
};

/*------------------------------------------*/
/* MWB - Incandescent */
static const u16 db8131m_WB_Incandescent[] = {
0xFF86,
0x14FE,
0xFF83,	// Page Mode
0xCB04,		// Min RGain
0xCC00,
0xCD04,		// Max RGain
0xCE20,
0xCF06,		// Min BGain
0xD030,
0xD106,		// Max BGain
0xD260,

0xFF84,
0x4900,		// Threshold_indoor	
0x4A00,			
0x4B00,		// Threshold_outdoor	
0x4C00,	
0x6700,		// AWB_Weight_2	
0x6800,		// AWB_Weight_3	
0x6900,		// AWB_Weight_4	
0x6B40,		// AWB_Weight_6	
};

/*------------------------------------------*/
/* MWB - Fluorescent */
static const u16 db8131m_WB_Fluorescent[] = {
0xFF86,
0x14DE,
0xFF83,	// Page Mode
0xCB05,		// Min RGain
0xCC70,
0xCD05,		// Max RGain
0xCE90,
0xCF05,		// Min BGain
0xD0F0,
0xD106,		// Max BGain
0xD210,

0xFF84,
0x4900,		// Threshold_indoor	
0x4A00,			
0x4B00,		// Threshold_outdoor	
0x4C00,	
0x6700,		// AWB_Weight_2	
0x6800,		// AWB_Weight_3	
0x6940,		// AWB_Weight_4	
0x6B00,		// AWB_Weight_6	
};


/*******************************************************
*	CAMERA_Effect
*******************************************************/
/*------------------------------------------*/
/* Effect Normal */
static const u16 db8131m_Effect_Normal[] = {
0xFF87,
0xBD00,
0xBE00,
0xBF00,
0xC000,
};

/*------------------------------------------*/
/* Effect Mono */
static const u16 db8131m_Effect_Mono[] = {
0xFF87,
0xBD08,
0xBE00,
0xBF00,
0xC000,
};

/*------------------------------------------*/
/* Effect Sepia */
static const u16 db8131m_Effect_Sepia[] = {
0xFF87,
0xBD18,
0xBE00,
0xBF61,
0xC09A,
};

/*------------------------------------------*/
/* Effect Aqua */
static const u16 db8131m_Effect_Aqua[] = {
0xFF87,
0xBD18,
0xBE00,
0xBFAB,
0xC000,
};

/*------------------------------------------*/
/* Effect Aqua */
static const u16 db8131m_Effect_sketch[] = {
0xFF87,
0xBD10,
0xBE80,
0xBF00,
0xC000,
};

/*------------------------------------------*/
/* Effect Negative */
static const u16 db8131m_Effect_Negative[] = {
0xFF87,
0xBD20,
0xBE00,
0xBF00,
0xC000,
};


/*******************************************************
* CAMERA_VT_PRETTY_0 Default
* 200s self cam pretty
*******************************************************/
static const u16 db8131m_vt_pretty_default[] = {
};

/*******************************************************
*	CAMERA_VT_PRETTY_1
*******************************************************/
static const u16 db8131m_vt_pretty_1[] = {
};


/*******************************************************
*	CAMERA_VT_PRETTY_2				*
*******************************************************/
static const u16 db8131m_vt_pretty_2[] = {
};


/*******************************************************
*	CAMERA_VT_PRETTY_3
*******************************************************/
static const u16 db8131m_vt_pretty_3[] = {
};

static const u16 db8131m_vt_7fps[] = {
/* Fixed 7fps Mode */
0xFF82, /*Page mode*/
0x9102,	/*AeMode*/
0xFF83, /*Page mode*/
0x0B09,	/*Frame Rate*/
0x0C24,	/*Frame Rate*/
0x0311,	/*TimeMax60Hz*/
0x0410,	/*Time3Lux60Hz*/
0x050C,	/*Time2Lut60Hz*/
0x0608,	/*Time1Lut60Hz*/
0xFF82, /*Page mode*/
0x925D,
};

static const u16 db8131m_vt_10fps[] = {
/* Fixed 10fps Mode */
0xFF82, /*Page mode*/
0x9102, /*AeMode*/
0xFF83, /*Page mode*/
0x0B06, /*Frame Rate*/
0x0C75, /*Frame Rate*/
0x030C, /*TimeMax60Hz*/
0x040B, /*Time3Lux60Hz*/
0x050A, /*Time2Lut60Hz*/
0x0608, /*Time1Lut60Hz*/
0xFF82, /*Page mode*/
0x925D,
};

static const u16 db8131m_vt_12fps[] = {
/* Fixed 12fps Mode */
0xFF82, /*Page mode*/
0x9102, /*AeMode*/
0xFF83, /*Page mode*/
0x0B05, /*Frame Rate*/
0x0C62, /*Frame Rate*/
0x030A, /*TimeMax60Hz*/
0x0409, /*Time3Lux60Hz*/
0x0508, /*Time2Lut60Hz*/
0x0606, /*Time1Lut60Hz*/
0xFF82, /*Page mode*/
0x925D,
};

static const u16 db8131m_vt_15fps[] = {
/* Fixed 15fps Mode */
0xFF82, /*Page mode*/
0x9102, /*AeMode*/
0xFF83, /*Page mode*/
0x0B04, /*Frame Rate*/
0x0C4C, /*Frame Rate*/
0x0308, /*TimeMax60Hz*/
0x0406, /*Time3Lux60Hz*/
0x0504, /*Time2Lut60Hz*/
0x0604, /*Time1Lut60Hz*/
0xFF82, /*Page mode*/
0x925D,
};

static const u16 db8131m_vt_auto[] = {
0xFF82, /*Page mode*/
};

/*******************************************************
*	CAMERA_DTP_ON
*******************************************************/
static const u16 db8131m_pattern_on[] = {
0xFF87,		/*Page mode*/
0xAB00,		/*BayerFunc*/
0xAC28,		/*RGBYcFunc*/
0xFFA0,		/*Page mode*/
0x0205,		/*TPG Gamma*/

0xE796, /*Wait  150*/
};

/*******************************************************
*	CAMERA_DTP_OFF
*******************************************************/
static const u16 db8131m_pattern_off[] = {
0xFF87,		/*Page mode*/
0xABFF,		/*BayerFunc*/
0xACFF,		/*RGBYcFunc*/
0xFFA0,		/*Page mode*/
0x0200,		/*TPG Disable*/

0xE796, /*Wait  150*/
};

static const u16 db8131m_flip_off[] = {
0xFF87,
0xd500,
};

static const u16 db8131m_vflip[] = {
0xFF87,
0xd508,
};

static const u16 db8131m_hflip[] = {
0xFF87,
0xd504,
};

static const u16 db8131m_flip_off_No15fps[] = {
//0xFF87,
//0xd502,
};

static const u16 db8131m_vflip_No15fps[] = {
//0xFF87,
//0xd50A,
};

static const u16 db8131m_hflip_No15fps[] = {
//0xFF87,
//0xd506,
};
#endif /* __DB8131M_REG_H */
