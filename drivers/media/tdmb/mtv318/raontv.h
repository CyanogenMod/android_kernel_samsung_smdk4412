/*
*
* File name: drivers/media/tdmb/mtv318/src/raontv.h
*
* Description : RAONTECH TV device driver API header file.
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

#ifndef __RAONTV_H__
#define __RAONTV_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "raontv_port.h"

#define RAONTV_CHIP_ID		0x8A

/*==============================================================================
 *
 * Common definitions and types.
 *
 *============================================================================*/
#ifndef NULL
	#define NULL		0
#endif

#ifndef FALSE
	#define FALSE		0
#endif

#ifndef TRUE
	#define TRUE		1
#endif

#ifndef MAX
	#define MAX(a, b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
	#define MIN(a, b)    (((a) < (b)) ? (a) : (b))
#endif

#ifndef ABS
	#define ABS(x)		 (((x) < 0) ? -(x) : (x))
#endif


#define	RTV_TS_PACKET_SIZE		188


/* Error codes. */
#define RTV_SUCCESS							0
#define RTV_INVAILD_COUNTRY_BAND			-1
#define RTV_UNSUPPORT_ADC_CLK				-2
#define RTV_INVAILD_TV_MODE				-3
#define RTV_CHANNEL_NOT_DETECTED			-4
#define RTV_INSUFFICIENT_CHANNEL_BUF		-5
#define RTV_INVAILD_FREQ					-6
#define RTV_INVAILD_SUB_CHANNEL_ID		-7 /* for T-DMB and DAB */
#define RTV_NO_MORE_SUB_CHANNEL		        -8 /* for T-DMB and DAB */
#define RTV_INVAILD_THRESHOLD_SIZE		-9
#define RTV_POWER_ON_CHECK_ERROR			-10
#define RTV_PLL_UNLOCKED					-11
#define RTV_ADC_CLK_UNLOCKED				-12





enum E_RTV_COUNTRY_BAND_TYPE {
	RTV_COUNTRY_BAND_JAPAN = 0,
	RTV_COUNTRY_BAND_KOREA,
	RTV_COUNTRY_BAND_BRAZIL,
	RTV_COUNTRY_BAND_ARGENTINA
};


/* Do not modify the order! */
enum E_RTV_ADC_CLK_FREQ_TYPE {
	RTV_ADC_CLK_FREQ_8_MHz = 0,
	RTV_ADC_CLK_FREQ_8_192_MHz,
	RTV_ADC_CLK_FREQ_9_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	MAX_NUM_RTV_ADC_CLK_FREQ_TYPE
};


/*============================================================================
 *
 * FM definitions, types and APIs.
 *
 *============================================================================*/
#define RTV_FM_PILOT_LOCK_MASK	0x1
#define RTV_FM_RDS_LOCK_MASK		0x2
#define RTV_FM_CHANNEL_LOCK_OK	(RTV_FM_PILOT_LOCK_MASK|RTV_FM_RDS_LOCK_MASK)

#define RTV_FM_RSSI_DIVIDER		10

enum E_RTV_FM_OUTPUT_MODE_TYPE {
	RTV_FM_OUTPUT_MODE_AUTO = 0,
	RTV_FM_OUTPUT_MODE_MONO = 1,
	RTV_FM_OUTPUT_MODE_STEREO = 2
};

void rtvFM_StandbyMode(int on);
void rtvFM_GetLockStatus(UINT *pLockVal, UINT *pLockCnt);
S32 rtvFM_GetRSSI(void);
void rtvFM_SetOutputMode(enum E_RTV_FM_OUTPUT_MODE_TYPE eOutputMode);
void rtvFM_DisableStreamOut(void);
void rtvFM_EnableStreamOut(void);
INT  rtvFM_SetFrequency(U32 dwChFreqKHz);
INT  rtvFM_SearchFrequency(U32 *pDetectedFreqKHz
			, U32 dwStartFreqKHz, U32 dwEndFreqKHz);
INT  rtvFM_ScanFrequency(U32 *pChBuf, UINT nNumChBuf
				, U32 dwStartFreqKHz, U32 dwEndFreqKHz);
INT  rtvFM_Initialize(enum E_RTV_ADC_CLK_FREQ_TYPE eAdcClkFreqType
					, UINT nThresholdSize);


/*============================================================================
 *
 * TDMB definitions, types and APIs.
 *
 *===========================================================================*/
#define RTV_TDMB_OFDM_LOCK_MASK		0x1
#define RTV_TDMB_FEC_LOCK_MASK		0x2
#define RTV_TDMB_CHANNEL_LOCK_OK	\
	(RTV_TDMB_OFDM_LOCK_MASK|RTV_TDMB_FEC_LOCK_MASK)

#define RTV_TDMB_BER_DIVIDER		100000
#define RTV_TDMB_CNR_DIVIDER		1000
#define RTV_TDMB_RSSI_DIVIDER		1000

enum E_RTV_TDMB_SERVICE_TYPE {
	RTV_TDMB_SERVICE_VIDEO = 0,
	RTV_TDMB_SERVICE_AUDIO,
	RTV_TDMB_SERVICE_DATA
};

struct RTV_TDMB_TII_INFO {
	int tii_combo;
	int tii_pattern;
	int tii_tower;
	int tii_strength;
};


#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#define RTV_CIF_HEADER_SIZE    4 /* bytes */
#else
	#define RTV_CIF_HEADER_SIZE    16 /* bytes */
#endif


struct RTV_CIF_DEC_INFO {
#if defined(RTV_IF_TSIF)
	UINT fic_size; /* Result size. */
	U8   *fic_buf_ptr; /* Destination buffer address. */
#endif

	/* Result size. */
	UINT msc_size[RTV_MAX_NUM_SUB_CHANNEL_USED];
	/* Result sub channel ID. */
	UINT msc_subch_id[RTV_MAX_NUM_SUB_CHANNEL_USED];
	/* Destination buffer address. */
	U8   *msc_buf_ptr[RTV_MAX_NUM_SUB_CHANNEL_USED];
};

BOOL rtvCIFDEC_Decode(struct RTV_CIF_DEC_INFO *ptDecInfo,
	const U8 *pbTsBuf, UINT nTsLen);
#if defined(RTV_IF_SPI) && defined(RTV_CIF_HEADER_INSERTED)
BOOL rtvCIFDEC_CheckCifData_SPI(const U8 *pbTsBuf, UINT nTsLen);
#endif


void rtvTDMB_StandbyMode(int on);
UINT rtvTDMB_GetLockStatus(void);
U32  rtvTDMB_GetPER(void);
S32  rtvTDMB_GetRSSI(void);
U32  rtvTDMB_GetCNR(void);
U32  rtvTDMB_GetCER(void);
U32  rtvTDMB_GetBER(void);
UINT rtvTDMB_GetAntennaLevel(U32 dwCER);
U32  rtvTDMB_GetPreviousFrequency(void);
void rtvTDMB_DisableStreamOut(void);
INT  rtvTDMB_OpenSubChannel(U32 dwChFreqKHz, UINT nSubChID,
		enum E_RTV_TDMB_SERVICE_TYPE eServiceType,
		UINT nThresholdSize);
INT  rtvTDMB_CloseSubChannel(UINT nSubChID);
void rtvTDMB_CloseAllSubChannels(void);
INT  rtvTDMB_ScanFrequency(U32 dwChFreqKHz);
UINT rtvTDMB_ReadFIC(U8 *pbBuf);
void rtvTDMB_CloseFIC(void);
void rtvTDMB_OpenFIC(void);
INT  rtvTDMB_Initialize(enum E_RTV_COUNTRY_BAND_TYPE eRtvCountryBandType);


/*============================================================================
 *
 * DAB definitions, types and APIs.
 *
 *============================================================================*/
#define RTV_DAB_OFDM_LOCK_MASK		0x1
#define RTV_DAB_FEC_LOCK_MASK		0x2
#define RTV_DAB_CHANNEL_LOCK_OK	\
	(RTV_TDMB_OFDM_LOCK_MASK|RTV_TDMB_FEC_LOCK_MASK)

#define RTV_DAB_BER_DIVIDER		100000
#define RTV_DAB_CNR_DIVIDER		1000
#define RTV_DAB_RSSI_DIVIDER		10

enum E_RTV_DAB_SERVICE_TYPE {
	RTV_DAB_SERVICE_VIDEO = 0,
	RTV_DAB_SERVICE_AUDIO,
	RTV_DAB_SERVICE_DATA
};

struct RTV_DAB_TII_INFO {
	int tii_combo;
	int tii_pattern;
	int tii_tower;
	int tii_strength;
};


enum E_RTV_DAB_TRANSMISSION_MODE {
	RTV_DAB_TRANSMISSION_MODE1 = 0,
	RTV_DAB_TRANSMISSION_MODE2,
	RTV_DAB_TRANSMISSION_MODE3,
	RTV_DAB_TRANSMISSION_MODE4,
	RTV_DAB_INVALID_TRANSMISSION_MODE
};


void rtvDAB_StandbyMode(int on);
UINT rtvDAB_GetLockStatus(void);
U32  rtvDAB_GetPER(void);
S32  rtvDAB_GetRSSI(void);
U32  rtvDAB_GetCNR(void);
U32  rtvDAB_GetCER(void);
U32  rtvDAB_GetBER(void);
U32  rtvDAB_GetPreviousFrequency(void);
void rtvDAB_DisableStreamOut(void);
INT  rtvDAB_OpenSubChannel(U32 dwChFreqKHz, UINT nSubChID
	, enum E_RTV_DAB_SERVICE_TYPE eServiceType, UINT nThresholdSize);
INT  rtvDAB_CloseSubChannel(UINT nSubChID);
void rtvDAB_CloseAllSubChannels(void);
INT  rtvDAB_ScanFrequency(U32 dwChFreqKHz);
UINT rtvDAB_ReadFIC(U8 *pbBuf, UINT nFicSize);
UINT rtvDAB_GetFicSize(void);
void rtvDAB_CloseFIC(void);
void rtvDAB_OpenFIC(void);
INT  rtvDAB_Initialize(void);


#ifdef __cplusplus
}
#endif

#endif /* __RAONTV_H__ */

