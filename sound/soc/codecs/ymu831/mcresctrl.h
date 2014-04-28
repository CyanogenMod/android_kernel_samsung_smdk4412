/****************************************************************************
 *
 *	Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcresctrl.h
 *
 *	Description	: MC Driver resource control header
 *
 *	Version		: 1.0.2	2012.12.27
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

#ifndef _MCRESCTRL_H_
#define _MCRESCTRL_H_

#include "mcdevif.h"
#include "mcpacking.h"

#define	MCDRV_BURST_WRITE_ENABLE	(0x01)

/*	Slave Address	*/
#define	MCDRV_SLAVEADDR_ANA		(0x3A)
#define	MCDRV_SLAVEADDR_DIG		(0x11)

/*	DEV_ID	*/
#define	MCDRV_DEVID_ANA			(0x90)
#define	MCDRV_DEVID_DIG			(0x80)
#define	MCDRV_DEVID_MASK		(0xF8)
#define	MCDRV_VERID_MASK		(0x07)

/*	eState setting	*/
enum MCDRV_STATE {
	eMCDRV_STATE_NOTINIT,
	eMCDRV_STATE_READY
};

/*	volume setting	*/
#define	MCDRV_LOGICAL_VOL_MUTE		(-24576)	/*	-96dB	*/

#define	MCDRV_REG_MUTE			(0x00)

/*	DIO port setting	*/
enum MCDRV_DIO_PORT_NO {
	eMCDRV_DIO_0	= 0,
	eMCDRV_DIO_1,
	eMCDRV_DIO_2,
	eMCDRV_DIO_3
};

/*	Path destination setting	*/
enum MCDRV_DST_CH {
	eMCDRV_DST_CH0	= 0,
	eMCDRV_DST_CH1,
	eMCDRV_DST_CH2,
	eMCDRV_DST_CH3
};

enum MCDRV_DST_TYPE {
	eMCDRV_DST_MUSICOUT	= 0,
	eMCDRV_DST_EXTOUT,
	eMCDRV_DST_HIFIOUT,
	eMCDRV_DST_VBOXMIXIN,
	eMCDRV_DST_AE0,
	eMCDRV_DST_AE1,
	eMCDRV_DST_AE2,
	eMCDRV_DST_AE3,
	eMCDRV_DST_DAC0,
	eMCDRV_DST_DAC1,
	eMCDRV_DST_VOICEOUT,
	eMCDRV_DST_VBOXIOIN,
	eMCDRV_DST_VBOXHOSTIN,
	eMCDRV_DST_HOSTOUT,
	eMCDRV_DST_ADIF0,
	eMCDRV_DST_ADIF1,
	eMCDRV_DST_ADIF2,
	eMCDRV_DST_ADC0,
	eMCDRV_DST_ADC1,
	eMCDRV_DST_SP,
	eMCDRV_DST_HP,
	eMCDRV_DST_RCV,
	eMCDRV_DST_LOUT1,
	eMCDRV_DST_LOUT2,
	eMCDRV_DST_BIAS
};

/*	Register accsess availability	*/
enum MCDRV_REG_ACCSESS {
	eMCDRV_ACCESS_DENY	= 0,
	eMCDRV_CAN_READ		= 0x01,
	eMCDRV_CAN_WRITE	= 0x02,
	eMCDRV_READ_WRITE	= eMCDRV_CAN_READ | eMCDRV_CAN_WRITE
};

/*	UpdateReg parameter	*/
enum MCDRV_UPDATE_MODE {
	eMCDRV_UPDATE_NORMAL,
	eMCDRV_UPDATE_FORCE,
	eMCDRV_UPDATE_DUMMY
};

/*	ePacketBufAlloc setting	*/
enum MCDRV_PACKETBUF_ALLOC {
	eMCDRV_PACKETBUF_FREE,
	eMCDRV_PACKETBUF_ALLOCATED
};

/* Power management mode */
enum MCDRV_PMODE {
	eMCDRV_APM_ON,
	eMCDRV_APM_OFF
};

/*	DSP Start	*/
#define	MCDRV_DSP_START_E	(0x01)
#define	MCDRV_DSP_START_B	(0x02)
#define	MCDRV_DSP_START_M	(0x04)
#define	MCDRV_DSP_START_F	(0x08)
#define	MCDRV_DSP_START_C	(0x10)

/*	Register available num	*/
#define	MCDRV_REG_NUM_IF	(59)
#define	MCDRV_REG_NUM_A		(68)
#define	MCDRV_REG_NUM_MA	(85)
#define	MCDRV_REG_NUM_MB	(86)
#define	MCDRV_REG_NUM_B		(72)
#define	MCDRV_REG_NUM_E		(101)
#define	MCDRV_REG_NUM_C		(160)
#define	MCDRV_REG_NUM_F		(128)
#define	MCDRV_REG_NUM_ANA	(116)
#define	MCDRV_REG_NUM_CD	(53)

/* control packet for serial host interface */
#define	MCDRV_MAX_CTRL_DATA_NUM	(MCDRV_MAX_PACKETS*4)
struct MCDRV_SERIAL_CTRL_PACKET {
	UINT8	abData[MCDRV_MAX_CTRL_DATA_NUM];
	UINT16	wDataNum;
};

/*	for AEC	*/
struct MCDRV_AEC_B_DSP {
	UINT8	*pbChunkData;
	UINT32	dwSize;
};

struct MCDRV_AEC_F_DSP {
	UINT8	*pbChunkData;
	UINT32	dwSize;
};

struct MCDRV_AEC_C_DSP {
	UINT8	*pbChunkData;
	UINT32	dwSize;
};

struct MCDRV_AEC_C_DSP_DEBUG {
	UINT8	bJtagOn;
};

struct MCDRV_AEC_SYSEQ_EX {
	UINT8	bEnable;
	struct {
		UINT8	bCoef_A0[3];
		UINT8	bCoef_A1[3];
		UINT8	bCoef_A2[3];
		UINT8	bCoef_B1[3];
		UINT8	bCoef_B2[3];
	} sBand[2];
};

struct MCDRV_AEC_E2_CONFIG {
	UINT8	*pbChunkData;
	UINT32	dwSize;
};

struct MCDRV_AEC_CONFIG {
	UINT8	bFDspLocate;
};

struct MCDRV_AEC_AUDIOENGINE {
	UINT8	bEnable;
	UINT8	bAEOnOff;
	UINT8	bFDspOnOff;
	UINT8	bBDspAE0Src;
	UINT8	bBDspAE1Src;
	UINT8	bMixerIn0Src;
	UINT8	bMixerIn1Src;
	UINT8	bMixerIn2Src;
	UINT8	bMixerIn3Src;
	struct MCDRV_AEC_B_DSP	sAecBDsp;
	struct MCDRV_AEC_F_DSP	sAecFDsp;
};

struct MCDRV_AEC_V_BOX {
	UINT8	bEnable;
	UINT8	bCDspFuncAOnOff;
	UINT8	bCDspFuncBOnOff;
	UINT8	bFDspOnOff;
	UINT8	bFdsp_Po_Source;
	UINT8	bISrc2_VSource;
	UINT8	bISrc2_Ch1_VSource;
	UINT8	bISrc3_VSource;
	UINT8	bLPt2_VSource;
	UINT8	bLPt2_Mix_VolO;
	UINT8	bLPt2_Mix_VolI;
	UINT8	bSrc3_Ctrl;
	UINT8	bSrc2_Fs;
	UINT8	bSrc2_Thru;
	UINT8	bSrc3_Fs;
	UINT8	bSrc3_Thru;
	struct MCDRV_AEC_C_DSP	sAecCDspA;
	struct MCDRV_AEC_C_DSP	sAecCDspB;
	struct MCDRV_AEC_F_DSP	sAecFDsp;
	struct MCDRV_AEC_C_DSP_DEBUG	sAecCDspDbg;
};

#define	MCDRV_AEC_OUTPUT_N	(2)
struct MCDRV_AEC_OUTPUT {
	UINT8	bLpf_Pre_Thru[MCDRV_AEC_OUTPUT_N];
	UINT8	bLpf_Post_Thru[MCDRV_AEC_OUTPUT_N];
	UINT8	bDcc_Sel[MCDRV_AEC_OUTPUT_N];
	UINT8	bSig_Det_Lvl;
	UINT8	bPow_Det_Lvl[MCDRV_AEC_OUTPUT_N];
	UINT8	bOsf_Sel[MCDRV_AEC_OUTPUT_N];
	UINT8	bSys_Eq_Enb[MCDRV_AEC_OUTPUT_N];
	UINT8	bSys_Eq_Coef_A0[MCDRV_AEC_OUTPUT_N][3];
	UINT8	bSys_Eq_Coef_A1[MCDRV_AEC_OUTPUT_N][3];
	UINT8	bSys_Eq_Coef_A2[MCDRV_AEC_OUTPUT_N][3];
	UINT8	bSys_Eq_Coef_B1[MCDRV_AEC_OUTPUT_N][3];
	UINT8	bSys_Eq_Coef_B2[MCDRV_AEC_OUTPUT_N][3];
	UINT8	bClip_Md[MCDRV_AEC_OUTPUT_N];
	UINT8	bClip_Att[MCDRV_AEC_OUTPUT_N];
	UINT8	bClip_Rel[MCDRV_AEC_OUTPUT_N];
	UINT8	bClip_G[MCDRV_AEC_OUTPUT_N];
	UINT8	bOsf_Gain[MCDRV_AEC_OUTPUT_N][2];
	UINT8	bDcl_OnOff[MCDRV_AEC_OUTPUT_N];
	UINT8	bDcl_Gain[MCDRV_AEC_OUTPUT_N];
	UINT8	bDcl_Limit[MCDRV_AEC_OUTPUT_N][2];
	UINT8	bRandom_Dither_OnOff[MCDRV_AEC_OUTPUT_N];
	UINT8	bRandom_Dither_Level[MCDRV_AEC_OUTPUT_N];
	UINT8	bRandom_Dither_POS[MCDRV_AEC_OUTPUT_N];
	UINT8	bDc_Dither_OnOff[MCDRV_AEC_OUTPUT_N];
	UINT8	bDc_Dither_Level[MCDRV_AEC_OUTPUT_N];
	UINT8	bDither_Type[MCDRV_AEC_OUTPUT_N];
	UINT8	bDng_On[MCDRV_AEC_OUTPUT_N];
	UINT8	bDng_Zero[MCDRV_AEC_OUTPUT_N];
	UINT8	bDng_Time[MCDRV_AEC_OUTPUT_N];
	UINT8	bDng_Fw[MCDRV_AEC_OUTPUT_N];
	UINT8	bDng_Attack;
	UINT8	bDng_Release;
	UINT8	bDng_Target[MCDRV_AEC_OUTPUT_N];
	UINT8	bDng_Target_LineOut[MCDRV_AEC_OUTPUT_N];
	UINT8	bDng_Target_Rc;
	struct MCDRV_AEC_SYSEQ_EX	sSysEqEx[MCDRV_AEC_OUTPUT_N];
};

#define	MCDRV_AEC_INPUT_N	(3)
struct MCDRV_AEC_INPUT {
	UINT8	bDsf32_L_Type[MCDRV_AEC_INPUT_N];
	UINT8	bDsf32_R_Type[MCDRV_AEC_INPUT_N];
	UINT8	bDsf4_Sel[MCDRV_AEC_INPUT_N];
	UINT8	bDcc_Sel[MCDRV_AEC_INPUT_N];
	UINT8	bDng_On[MCDRV_AEC_INPUT_N];
	UINT8	bDng_Att[MCDRV_AEC_INPUT_N];
	UINT8	bDng_Rel[MCDRV_AEC_INPUT_N];
	UINT8	bDng_Fw[MCDRV_AEC_INPUT_N];
	UINT8	bDng_Tim[MCDRV_AEC_INPUT_N];
	UINT8	bDng_Zero[MCDRV_AEC_INPUT_N][2];
	UINT8	bDng_Tgt[MCDRV_AEC_INPUT_N][2];
	UINT8	bDepop_Att[MCDRV_AEC_INPUT_N];
	UINT8	bDepop_Wait[MCDRV_AEC_INPUT_N];
	UINT8	bRef_Sel;
};

struct MCDRV_AEC_PDM {
	UINT8	bMode;
	UINT8	bStWait;
	UINT8	bPdm0_LoadTim;
	UINT8	bPdm0_LFineDly;
	UINT8	bPdm0_RFineDly;
	UINT8	bPdm1_LoadTim;
	UINT8	bPdm1_LFineDly;
	UINT8	bPdm1_RFineDly;
	UINT8	bPdm0_Data_Delay;
	UINT8	bPdm1_Data_Delay;
};

struct MCDRV_AEC_E2 {
	UINT8	bEnable;
	UINT8	bE2_Da_Sel;
	UINT8	bE2_Ad_Sel;
	UINT8	bE2OnOff;
	struct MCDRV_AEC_E2_CONFIG	sE2Config;
};
struct MCDRV_AEC_ADJ {
	UINT8	bHold;
	UINT8	bCnt;
	UINT8	bMax[2];
};

struct MCDRV_AEC_EDSP_MISC {
	UINT8	bI2SOut_Enb;
	UINT8	bChSel;
	UINT8	bLoopBack;
};

struct MCDRV_AEC_CONTROL {
	UINT8	bCommand;
	UINT8	bParam[4];
};

struct MCDRV_AEC_INFO {
	struct MCDRV_AEC_CONFIG		sAecConfig;
	struct MCDRV_AEC_AUDIOENGINE	sAecAudioengine;
	struct MCDRV_AEC_V_BOX		sAecVBox;
	struct MCDRV_AEC_OUTPUT		sOutput;
	struct MCDRV_AEC_INPUT		sInput;
	struct MCDRV_AEC_PDM		sPdm;
	struct MCDRV_AEC_E2		sE2;
	struct MCDRV_AEC_ADJ		sAdj;
	struct MCDRV_AEC_EDSP_MISC	sEDspMisc;
	struct MCDRV_AEC_CONTROL	sControl;
};

/*	global information	*/
struct MCDRV_GLOBAL_INFO {
	UINT8				bHwId;
	enum MCDRV_PACKETBUF_ALLOC	ePacketBufAlloc;
	UINT8				abRegValIF[MCDRV_REG_NUM_IF];
	UINT8				abRegValA[MCDRV_REG_NUM_A];
	UINT8				abRegValMA[MCDRV_REG_NUM_MA];
	UINT8				abRegValMB[MCDRV_REG_NUM_MB];
	UINT8				abRegValB[MCDRV_REG_NUM_B];
	UINT8				abRegValE[MCDRV_REG_NUM_E];
	UINT8				abRegValC[MCDRV_REG_NUM_C];
	UINT8				abRegValF[MCDRV_REG_NUM_F];
	UINT8				abRegValANA[MCDRV_REG_NUM_ANA];
	UINT8				abRegValCD[MCDRV_REG_NUM_CD];

	struct MCDRV_INIT_INFO		sInitInfo;
	struct MCDRV_INIT2_INFO		sInit2Info;
	struct MCDRV_CLOCKSW_INFO	sClockSwInfo;
	struct MCDRV_PATH_INFO		sPathInfo;
	struct MCDRV_PATH_INFO		sPathInfoVirtual;
	struct MCDRV_VOL_INFO		sVolInfo;
	struct MCDRV_DIO_INFO		sDioInfo;
	struct MCDRV_DIOPATH_INFO	sDioPathInfo;
	struct MCDRV_SWAP_INFO		sSwapInfo;
	struct MCDRV_HSDET_INFO		sHSDetInfo;
	struct MCDRV_HSDET2_INFO	sHSDet2Info;
	struct MCDRV_AEC_INFO		sAecInfo;
	SINT32(*pcbfunc)(SINT32, UINT32, UINT32);
	struct MCDRV_GP_MODE		sGpMode;
	UINT8				abGpMask[3];
	UINT8				abGpPad[3];
	UINT8				bClkSel;
	UINT8				bEClkSel;
	UINT8				bCClkSel;
	UINT8				bFClkSel;
	UINT8				bPlugDetDB;

	struct MCDRV_SERIAL_CTRL_PACKET	sCtrlPacket;
	UINT16				wCurSlaveAddr;
	UINT16				wCurRegType;
	UINT16				wCurRegAddr;
	UINT16				wDataContinueCount;
	UINT16				wPrevAddrIndex;

	enum MCDRV_PMODE		eAPMode;
};



SINT32		McResCtrl_SetHwId(UINT8 bHwId_dig, UINT8 bHwId_ana);
void		McResCtrl_Init(void);
void		McResCtrl_InitABlockReg(void);
void		McResCtrl_InitMBlockReg(void);
void		McResCtrl_InitEReg(void);
void		McResCtrl_UpdateState(enum MCDRV_STATE eState);
enum MCDRV_STATE	McResCtrl_GetState(void);
UINT8		McResCtrl_GetRegVal(UINT16 wRegType, UINT16 wRegAddr);
void		McResCtrl_SetRegVal(UINT16 wRegType,
				UINT16 wRegAddr, UINT8 bRegVal);

void		McResCtrl_SetInitInfo(
			const struct MCDRV_INIT_INFO *psInitInfo,
			const struct MCDRV_INIT2_INFO *psInit2Info);
void		McResCtrl_GetInitInfo(struct MCDRV_INIT_INFO *psInitInfo,
					struct MCDRV_INIT2_INFO *psInit2Info);
void		McResCtrl_SetClockSwInfo(
			const struct MCDRV_CLOCKSW_INFO *psClockSwInfo);
void		McResCtrl_GetClockSwInfo(
				struct MCDRV_CLOCKSW_INFO *psClockSwInfo);
SINT32		McResCtrl_SetPathInfo(
				const struct MCDRV_PATH_INFO *psPathInfo);
void		McResCtrl_GetPathInfo(struct MCDRV_PATH_INFO *psPathInfo);
void		McResCtrl_GetPathInfoVirtual(
				struct MCDRV_PATH_INFO *psPathInfo);
void		McResCtrl_SetVolInfo(const struct MCDRV_VOL_INFO *psVolInfo);
void		McResCtrl_GetVolInfo(struct MCDRV_VOL_INFO *psVolInfo);
void		McResCtrl_SetDioInfo(const struct MCDRV_DIO_INFO *psDioInfo,
					UINT32 dUpdateInfo);
void		McResCtrl_GetDioInfo(struct MCDRV_DIO_INFO *psDioInfo);
void		McResCtrl_SetDioPathInfo(
				const struct MCDRV_DIOPATH_INFO *psDioPathInfo,
					UINT32 dUpdateInfo);
void		McResCtrl_GetDioPathInfo(
				struct MCDRV_DIOPATH_INFO *psDioPathInfo);
void		McResCtrl_SetSwap(
				const struct MCDRV_SWAP_INFO *psSwapInfo,
				UINT32 dUpdateInfo);
void		McResCtrl_GetSwap(struct MCDRV_SWAP_INFO *psSwapInfo);
void		McResCtrl_SetHSDet(
				const struct MCDRV_HSDET_INFO *psHSDetInfo,
				const struct MCDRV_HSDET2_INFO *psHSDet2Info,
				UINT32 dUpdateInfo);
void		McResCtrl_GetHSDet(struct MCDRV_HSDET_INFO *psHSDetInfo,
				struct MCDRV_HSDET2_INFO *psHSDet2Info);
void		McResCtrl_SetAecInfo(
				const struct MCDRV_AEC_INFO *psAecInfo);
void		McResCtrl_ReplaceAecInfo(
				const struct MCDRV_AEC_INFO *psAecInfo);
void		McResCtrl_GetAecInfo(struct MCDRV_AEC_INFO *psAecInfo);
void		McResCtrl_SetDSPCBFunc(
				SINT32 (*pcbfunc)(SINT32, UINT32, UINT32));
void		McResCtrl_GetDSPCBFunc(
				SINT32 (**pcbfunc)(SINT32, UINT32, UINT32));
void		McResCtrl_SetGPMode(const struct MCDRV_GP_MODE *psGpMode);
void		McResCtrl_GetGPMode(struct MCDRV_GP_MODE *psGpMode);
void		McResCtrl_SetGPMask(UINT8 bMask, UINT32 dPadNo);
UINT8		McResCtrl_GetGPMask(UINT32 dPadNo);
void		McResCtrl_SetGPPad(UINT8 bPad, UINT32 dPadNo);
UINT8		McResCtrl_GetGPPad(UINT32 dPadNo);

void		McResCtrl_SetClkSel(UINT8 bClkSel);
UINT8		McResCtrl_GetClkSel(void);
void		McResCtrl_SetEClkSel(UINT8 bEClkSel);
UINT8		McResCtrl_GetEClkSel(void);
void		McResCtrl_SetCClkSel(UINT8 bCClkSel);
UINT8		McResCtrl_GetCClkSel(void);
void		McResCtrl_SetFClkSel(UINT8 bFClkSel);
UINT8		McResCtrl_GetFClkSel(void);
void		McResCtrl_SetPlugDetDB(UINT8 bPlugDetDB);
UINT8		McResCtrl_GetPlugDetDB(void);

void		McResCtrl_GetVolReg(struct MCDRV_VOL_INFO *psVolInfo);
SINT16		McResCtrl_GetDigitalVolReg(SINT32 sdVol);
void		McResCtrl_GetPowerInfo(struct MCDRV_POWER_INFO *psPowerInfo);
void		McResCtrl_GetPowerInfoRegAccess(
					const struct MCDRV_REG_INFO *psRegInfo,
					struct MCDRV_POWER_INFO *psPowerInfo);
void		McResCtrl_GetCurPowerInfo(
					struct MCDRV_POWER_INFO *psPowerInfo);
UINT8		McResCtrl_IsD1SrcUsed(UINT32 dSrcOnOff);
UINT8		McResCtrl_IsD2SrcUsed(UINT32 dSrcOnOff);
UINT8		McResCtrl_IsASrcUsed(UINT32 dSrcOnOff);

UINT8		McResCtrl_HasSrc(enum MCDRV_DST_TYPE eType,
					enum MCDRV_DST_CH eCh);
UINT32		McResCtrl_GetSource(enum MCDRV_DST_TYPE eType,
					enum MCDRV_DST_CH eCh);
UINT8		McResCtrl_GetDspStart(void);

enum MCDRV_REG_ACCSESS	McResCtrl_GetRegAccess(
				const struct MCDRV_REG_INFO *psRegInfo);

enum MCDRV_PMODE	McResCtrl_GetAPMode(void);

struct MCDRV_PACKET	*McResCtrl_AllocPacketBuf(void);
void		McResCtrl_ReleasePacketBuf(void);

void		McResCtrl_InitRegUpdate(void);
void		McResCtrl_AddRegUpdate(UINT16 wRegType,
					UINT16 wAddress,
					UINT8 bData,
					enum MCDRV_UPDATE_MODE eUpdateMode);
void		McResCtrl_ExecuteRegUpdate(void);
SINT32		McResCtrl_WaitEvent(UINT32 dEvent, UINT32 dParam);

UINT8		McResCtrl_IsEnableIRQ(void);


#endif /* _MCRESCTRL_H_ */
