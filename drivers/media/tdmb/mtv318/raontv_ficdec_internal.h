/*
*
* File name: drivers/media/tdmb/mtv318/src/raontv_ficdec_internal.h
*
* Description : RAONTECH TV  FIC Decoder internal header file.
*
* Copyright (C) (2011, RAONTECH)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef __RAONTV_FICDEC_INTERNAL_H__
#define __RAONTV_FICDEC_INTERNAL_H__

/********************/
/*   Header Files   */
/********************/
#include <linux/string.h>

#include "raontv_ficdec.h"


#define RTV_FAIL       (-1)
#define RTV_UNLOCK     (-10)
#define RTV_OK         0
#define RTV_SCAN_OK    1
#define RTV_LOCK_CHECK 10
#define RTV_NOTHING    2


#define FIC_GOING       0
#define FIC_LABEL       0x01
#define FIC_APP_TYPE    0x10
#define FIC_DONE        0x11
#define FIC_CRC_ERR       0x44


/**************************/
/*     FIC Definition     */
/**************************/
#define MSC_STREAM_AUDIO   0x00
#define MSC_STREAM_DATA    0x01
#define FIDC               0x02
#define MSC_PACKET_DATA    0x03

/* Ensemble, service, service component label number */
#define LABEL_NUM          17

/* maximum service component number in Ensemble channel */
#define MAX_SERV_COMP      64

/* maximum service component number in one service */
#define MAX_SUB_CH_NUM     12

/* maximum service number in one ensemble */
#define MAX_SERVICE_NUM    20

/*maximum number of user applications */
#define USER_APP_NUM       6

/* get FIC Time information complete */
#define ALL_FLAG_MATCH	   0x11

/* get FIC type0 extension10 complete */
#define TIME_FLAG          0x01

/* get FIC type0 extension9 complete */
#define LTO_FLAG           0x10

#define Unspec_DATA        0
#define TMC_Type           1
#define EWS_Type           2
#define ITTS_Type          3
#define Paging_Type        4
#define TDC_Type           5
#define KDMB_Type          24
#define	Em_IP_Type         59
#define	MOT_Type           60
#define	PS_noDSCTy         61

/****************************/
/*    FIC Data structure    */
/****************************/
/*! Date and Time information for Application user */
struct DATE_TIME_INFO {
	U32 MJD;
	U8  LSI;
	U8  conf_ind;
	U8  utc_flag;
	U8  apm_flag;
	U8  time_flag;
	U8  get_flag;
	U16 years;
	U8  months_dec;
	char months_ste[4];
	char weeks[4];
	U8  days;
	U8  hours;
	U8  minutes;
	U8  seconds;
	U16 milliseconds;
	U8  LTO;          /*Local Time Offset */
};

/*! Service Component Descriptor for Application user */
struct SVR_COM_DESP {
	S32 BIT_RATE;
	S32 P_L;
	S32 START_Addr;
	S32 SUB_CH_Size;
	U8  TMID;
	U8  ASCTy;
	U8  SubChid;
	U8  P_S;
	U8  CA_flag;
	U8  DSCTy;
	U8  FIDCid;
	U8  TCid;
	U8  Ext;
	U16 SCid;
	U8  DG_flag;
	U16 Packet_add;
	U16 CA_Org;
	U8  FEC_scheme;
	U8	language;
	U8  charset;
	char Label[LABEL_NUM];
	U32 Sid;
	U8  SCidS;
	U8  Num_User_App;
	U16 User_APP_Type[USER_APP_NUM];
	U8  User_APP_data_length[USER_APP_NUM];
	U8  User_APP_data[24];
};

/*! Service Descriptor for Application user */
struct SERVICE_DESC {
	/*MCI Information*/
	U32 Sid;
	U8  Country_id;
	U32 Service_ref;
	U8  ECC;
	U8  Local_flag;
	U8  CAID;
	U8  Num_ser_comp;
	U8  P_D;
	/*SI Information */
	U8  label_flag;
	U8  charset;
	char  Label[LABEL_NUM];
	/*Program type */
	U8 int_code;
	U8  ser_comp_num[MAX_SUB_CH_NUM];
};

/*! FIDC Descriptor for Application user */
struct FIDC_EWS_Region {
	U8  Sub_Region[11];
};

struct FIDC_DESC {
	U8  EWS_current_segmemt;
	U8  EWS_total_segmemt;
	U8  EWS_Message_ID;
	S8  EWS_category[4];
	U8  EWS_priority;
	U32 EWS_time_MJD;
	U8  EWS_time_Hours;
	U8  EWS_time_Minutes;
	U8  EWS_region_form;
	U8  EWS_region_num;
	U8  EWS_Rev;
	struct FIDC_EWS_Region  EWS_Region[15];
	U8  EWS_short_sentence[409];
};

/*! Ensemble Descriptor for Application user */
struct ENSEMBLE_DESC {
	/*COMMON Information*/
	U32 svr_num;
	U32 svr_comp_num;
	U32 label_num;
	/*MCI Information*/
	U16 id;
	U8  change_flag;
	U8  Alarm_flag;
	/*SI Information*/
	U8  charset;
	char  Label[LABEL_NUM];
	U32 freq;
	U8  label_flag;

	struct DATE_TIME_INFO date_time_info;
	struct SERVICE_DESC svr_desc[MAX_SERVICE_NUM];
	struct SVR_COM_DESP svr_comp[MAX_SERV_COMP];
	struct FIDC_DESC fidc_desc;

};


/*****************/
/*    EXTERNS    */
/*****************/
extern char *PROGRAM_TYPE_CODE16[32];
extern char *USER_APP_TYPE_CODE[11];
extern char *FIDC_EXT_CODE[3];
extern char *ASCTy[3];
extern char *DSCTy[11];
extern char *ANNOUNCEMENT_TYPE_CODE[12];
extern S32 SUBCH_UEP_TABLE[64][3];
extern char *WEEK_TABLE[8];
extern char *MONTH_TABLE[12];
extern struct ENSEMBLE_DESC ENS_DESC, NEXT_ENS_DESC;
extern struct ENSEMBLE_DESC ENS_DESC1, NEXT_ENS_DESC1;
extern struct ENSEMBLE_DESC ENS_DESC2, NEXT_ENS_DESC2;
extern char *EWS_CATEGORY[67][3];
extern char *EWS_PRIORITY_TABLE[4];
extern char *EWS_REGION_FORM_TABLE[4];
extern char *EWS_OFFICIAL_ORGANIZATION_TABLE[4];



/********************/
/*  FIC Definition  */
/********************/
#define PN_FIB_END_MARKER (0xFF)

/****************************/
/*   FIC Parser function    */
/****************************/
S32  FIB_Init_Dec(U8 *);
S32  MCI_SI_DEC(U8);
S32  SI_LABEL_DEC1(U8);
S32  SI_LABEL_DEC2(U8);
S32  FIDC_DEC(U8);
S32  CA_DEC(U8);
S32  RESERVED1(U8);
S32  RESERVED2(U8);
S32  RESERVED3(U8);

/*****************************/
/*  FIG Data Type structure  */
/*****************************/
struct FIG_DATA {
	U8 type;
	U8 length;
	U8 *data;
	U8 byte_cnt;
	U8 bit_cnt;
};

/****************************/
/*  FIG type 0 data field   */
/****************************/
struct FIG_TYPE0 {
	U8 C_N;
	U8 OE;
	U8 P_D;
	U8 Ext;
};

/* Ensemble Information */
struct FIG_TYPE0_Ext0 {
	U16 Eid;
	U8  Country_ID;
	U32 Ensemble_Ref;
	U8  Change_flag;
	U8  AI_flag;
	U8  CIF_Count0;
	U8  CIF_Count1;
	U8  Occurence_Change;
};

/* Structure of the sub-channel organization field */
struct FIG_TYPE0_Ext1 {
	U8  SubChid;
	U32 StartAdd;
	U8  S_L_form;
	U32 Size_Protection;
	U8  Table_sw;
	U8  Table_index;
	U8  Option;
	U8  Protection_Level;
	U32 Sub_ch_size;
};

/* Structure of the service organization field */
struct FIG_TYPE0_Ext2_ser_comp_des {
	U8  TMID;
	U8  ASCTy;
	U8  SubChid;
	U8  P_S;
	U8  CA_flag;
	U8  DSCTy;
	U8  FIDCid;
	U8  TCid;
	U8  Ext;
	U16 SCid;
};

/* Basic service and service component definition structure  */
struct FIG_TYPE0_Ext2 {
	U32 Sid;
	U8  Country_id;
	U32 Service_ref;
	U8  ECC;
	U8  Local_flag;
	U8  CAID;
	U8  Num_ser_comp;
	struct FIG_TYPE0_Ext2_ser_comp_des svr_comp_des[MAX_SERV_COMP];
};

/* Structure of the service component in packet mode */
struct FIG_TYPE0_Ext3 {
	U16 SCid;
	U8  CA_Org_flag;
	U8  DG_flag;
	U8  DSCTy;
	U8  SubChid;
	U16 Packet_add;
	U16 CA_Org;
};

/* Structure of the service component field in Stream mode or FIC */
struct FIG_TYPE0_Ext4 {
	U8  M_F;
	U8  SubChid;
	U8  FIDCid;
	U16 CA_Org;
};

/* Structure of the service component language field */
struct FIG_TYPE0_Ext5 {
	U8  L_S_flag;
	U8  MSC_FIC_flag;
	U8  SubChid;
	U8  FIDCid;
	U16 SCid;
	U8  Language;
};

/* Service Linking Information */
struct FIG_TYPE0_Ext6 {
	U8  id_list_flag;
	U8  LA;
	U8  S_H;
	U8  ILS;
	U32 LSN;
	U8  id_list_usage;
	U8  idLQ;
	U8  Shd;
	U8  Num_ids;
	U16 id[12];
	U8  ECC[12];
	U32 Sid[12];
};

/* Structure of the service component global definition field */
struct FIG_TYPE0_Ext8 {
	U32 Sid;
	U8  Ext_flag;
	U8  SCidS;
	U8  L_S_flag;
	U8  MSC_FIC_flag;
	U8  SubChid;
	U8  FIDCid;
	U32 SCid;
};

/* Structure of Country, LTO International field */
struct FIG_TYPE0_Ext9 {
	U8  Ext_flag;
	U8  LTO_unique;
	U8  Ensemble_LTO;
	U8  Ensemble_ECC;
	U8  Inter_Table_ID;
	U8  Num_Ser;
	U8  LTO;
	U8  ECC;
	U32 Sid[11];
};

/* Structure of the data and time field */
struct FIG_TYPE0_Ext10 {
	U32 MJD;
	U8  LSI;
	U8  Conf_ind;
	U8  UTC_flag;
	U32 UTC;
	U8  Hours;
	U8  Minutes;
	U8  Seconds;
	U16 Milliseconds;
};

/* Structure of the Region Definition */
struct FIG_TYPE0_Ext11 {
	U8  GATy;
	U8  G_E_flag;
	U8  Upper_part;
	U8  Lower_part;
	U8  length_TII_list;
	U8  Mainid[12];
	U8  Length_Subid_list;
	U8  Subid[36];
	U32 Latitude_Coarse;
	U32 Longitude_coarse;
	U32 Extent_Latitude;
	U32 Extent_Longitude;
};

/* Structure of the User Application Information */
struct FIG_TYPE0_Ext13 {
	U32 Sid;
	U8  SCidS;
	U8  Num_User_App;
	U16 User_APP_Type[6];
	U8  User_APP_data_length[6];
	U8  CA_flag;
	U8  CA_Org_flag;
	U8  X_PAD_App_Ty;
	U8  DG_flag;
	U8  DSCTy;
	U16 CA_Org;
	U8  User_APP_data[24];
};

/* FEC sub-channel organization */
struct FIG_TYPE0_Ext14 {
	U8  SubChid;
	U8  FEC_scheme;
};

/* Program Number structure  */
struct FIG_TYPE0_Ext16 {
	U16 Sid;
	U16 PNum;
	U8  Continuation_flag;
	U8  Update_flag;
	U16 New_Sid;
	U16 New_PNum;
};

/*	Program Type structure	*/
struct FIG_TYPE0_Ext17 {
	U16 Sid;
	U8  S_D;
	U8  P_S;
	U8  L_flag;
	U8  CC_flag;
	U8  Language;
	U8  Int_code;
	U8  Comp_code;
};

/* Announcement support */
struct FIG_TYPE0_Ext18 {
	U16 Sid;
	U16 ASU_flags;
	U8  Num_clusters;
	U8  Cluster_ID[23];
};

/* Announcement switching */
struct FIG_TYPE0_Ext19 {
	U8  Cluster_ID;
	U16 ASW_flags;
	U8  New_flag;
	U8  Region_flag;
	U8  SubChid;
	U8  Regionid_Lower_Part;
};

/* Frequency Information */
struct FIG_TYPE0_Ext21 {
	U16 ResionID;
	U8  Length_of_FI_list;
	U16 id_field;
	U8  R_M;
	U8  Continuity_flag;
	U8  Length_Freq_list;
	U8  Control_field[5];
	U8  id_field2[4];
	U32 Freq_a[5];
	U8  Freq_b[17];
	U16 Freq_c[8];
	U16 Freq_d[7];
};

/* Transmitter Identification Information (TII) database */
struct FIG_TYPE0_Ext22 {
	U8  M_S;
	U8  Mainid;
	U32 Latitude_coarse;
	U32 Longitude_coarse;
	U8  Latitude_fine;
	U8  Longitude_fine;
	U8  Num_Subid_fields;
	U8  Subid[4];
	U16 TD[4];
	U16 Latitude_offset[4];
	U16 Longitude_offset[4];
};

/* Other Ensemble Service */
struct FIG_TYPE0_Ext24 {
	U32 Sid;
	U8  CAid;
	U8  Number_Eids;
	U16 Eid[12];
};

/* Other Ensemble Announcement support */
struct FIG_TYPE0_Ext25 {
	U32 Sid;
	U32 ASU_flag;
	U8  Number_Eids;
	U8  Eid[12];
};

/* Other Ensemble Announcement switching */
struct FIG_TYPE0_Ext26 {
	U8  Cluster_id_Current_Ensemble;
	U32 Asw_flags;
	U8  New_flag;
	U8  Region_flag;
	U8  Region_id_current_Ensemble;
	U32 Eid_Other_Ensemble;
	U8  Cluster_id_other_Ensemble;
	U8  Region_id_Oter_Ensemble;
};

/* FM Announcement support */
struct FIG_TYPE0_Ext27 {
	U32 Sid;
	U8  Number_PI_Code;
	U32 PI[12];
};

/* FM Announcement switching */
struct FIG_TYPE0_Ext28 {
	U8  Cluster_id_Current_Ensemble;
	U8  New_flag;
	U8  Region_id_Current_Ensemble;
	U32 PI;
};

/* FIC re-direction */
struct FIG_TYPE0_Ext31 {
	U32 FIG0_flag_field;
	U8  FIG1_flag_field;
	U8  FIG2_flag_field;
};

/****************************/
/*  FIG type 5 data field   */
/****************************/
struct FIG_TYPE5 {
	U8 D1;
	U8 D2;
	U8 TCid;
	U8 Ext;
};

/* Paging */
struct FIG_TPE5_Ext0 {
	U8  SubChid;
	U16 Packet_add;
	U8  F1;
	U8  F2;
	U16 LFN;
	U8  F3;
	U16 Time;
	U8  CAid;
	U16 CA_Org;
	U32 Paging_user_group;
};

/* TMC */
struct FIG_TYPE5_Ext1 {
	U32 TMC_User_Message[30];
	U16 TMC_System_Message[30];
};

/* EWS */
struct FIG_EWS_Region {
	U8  Sub_Region[11];
};

struct FIG_TYPE5_Ext2 {
	U8  current_segmemt;
	U8  total_segmemt;
	U8  Message_ID;
	S8  category[4];
	U8  priority;
	U32 time_MJD;
	U8  time_Hours;
	U8  time_Minutes;
	U8  region_form;
	U8  region_num;
	U8  Rev;
	struct FIG_EWS_Region Region[15];
	U8  data[409];
};

/****************************/
/*  FIG type 6 data field   */
/****************************/
struct FIG_TYPE6 {
	U8  C_N;
	U8  OE;
	U8  P_D;
	U8  LEF;
	U8  ShortCASysId;
	U32 Sid;
	U16 CASysId;
	U16 CAIntChar;
};

/***************************/
/*  Function declarations  */
/***************************/

/* FIG TYPE 0 function */
S32 Get_FIG0_EXT0(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT1(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT2(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT3(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT4(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT5(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT6(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT7(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT8(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT9(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT10(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT11(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT12(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT13(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT14(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT15(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT16(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT17(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT18(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT19(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT20(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT21(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT22(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT23(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT24(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT25(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT26(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT27(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT28(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT29(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT30(U8 fic_cmd, U8 P_D, U8 C_N);
S32 Get_FIG0_EXT31(U8 fic_cmd, U8 P_D, U8 C_N);

/* FIG TYPE 1 function */
S32 Get_FIG1_EXT0(U8 fic_cmd, U8 Char_Set);
S32 Get_FIG1_EXT1(U8 fic_cmd, U8 Char_Set);
S32 Get_FIG1_EXT2(U8 fic_cmd, U8 Char_Set);
S32 Get_FIG1_EXT3(U8 fic_cmd, U8 Char_Set);
S32 Get_FIG1_EXT4(U8 fic_cmd, U8 Char_Set);
S32 Get_FIG1_EXT5(U8 fic_cmd, U8 Char_Set);
S32 Get_FIG1_EXT6(U8 fic_cmd, U8 Char_Set);
S32 Get_FIG1_EXT7(U8 fic_cmd, U8 Char_Set);

/* FIG TYPE 2 function */
S32 Get_FIG2_EXT0(U8 fic_cmd, U8 Seg_Index);
S32 Get_FIG2_EXT1(U8 fic_cmd, U8 Seg_Index);
S32 Get_FIG2_EXT2(U8 fic_cmd, U8 Seg_Index);
S32 Get_FIG2_EXT3(U8 fic_cmd, U8 Seg_Index);
S32 Get_FIG2_EXT4(U8 fic_cmd, U8 Seg_Index);
S32 Get_FIG2_EXT5(U8 fic_cmd, U8 Seg_Index);
S32 Get_FIG2_EXT6(U8 fic_cmd, U8 Seg_Index);
S32 Get_FIG2_EXT7(U8 fic_cmd, U8 Seg_Index);

/* FIG TYPE 5 function */
S32 Get_FIG5_EXT0(U8 D1, U8 D2, U8 fic_cmd, U8 TCid);
S32 Get_FIG5_EXT1(U8 D1, U8 D2, U8 fic_cmd, U8 TCid);
S32 Get_FIG5_EXT2(U8 D1, U8 D2, U8 fic_cmd, U8 TCid);



U8 GET_SUBCH_INFO(struct FIG_TYPE0_Ext1 *type0_ext1
		, S32 *BIT_RATE, S32 *SUB_CH_Size, S32 *P_L);
U8 GET_DATE_TIME(struct DATE_TIME_INFO *time_desc);
U8 GET_EWS_TIME(struct DATE_TIME_INFO *time_desc);



#endif /* __RAONTV_FICDEC_INTERNAL_H__ */
