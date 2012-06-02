/*****************************************************************************
 Copyright(c) 2011 I&C Inc. All Rights Reserved

 File name : INC_INCLUDES.h.c

 Description :

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*******************************************************************************/

#ifndef _INC_T3700_INCLUDES_H_
#define _INC_T3700_INCLUDES_H_

#include <linux/kernel.h>
#include <linux/string.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <linux/hwmon.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/stat.h>
#include <asm/system.h>
#include <asm/mach-types.h>
#include <linux/uaccess.h>
#include <asm/irq.h>
#include <asm/mach/map.h>
#include <linux/timer.h>
#include <asm/mach/time.h>
#include <linux/dma-mapping.h>

enum {
	FALSE = 0,
	TRUE = 1
};
#define INC_DEBUG_LEVEL					0

#ifdef	INC_MULTI_CHANNEL_ENABLE
#define INC_MULTI_CHANNEL_FIC_UPLOAD
#define INC_MULTI_HEADER_ENABLE
#endif

#define INC_MULTI_MAX_CHANNEL			3
/*************************************************/
/* FIFO Source 사용시                            */
/*************************************************/
#define INC_FIFO_SOURCE_ENABLE

/******************************************/
/* USer Application type 사용시          */
/******************************************/

#define INC_MPI_INTERRUPT_ENABLE		0x0001
#define INC_I2C_INTERRUPT_ENABLE		0x0002
#define INC_EWS_INTERRUPT_ENABLE		0x0040
#define INC_REC_INTERRUPT_ENABLE		0x0080
#define INC_TII_INTERRUPT_ENABLE		0x0100
#define INC_FIC_INTERRUPT_ENABLE		0x0200
#define INC_CIFS_INTERRUPT_ENABLE		0x0400
#define INC_FS_INTERRUPT_ENABLE			0x0800
#define INC_EXT_INTERRUPT_ENABLE		0x1000
#define INC_MS_INTERRUPT_ENABLE			0x2000
#define INC_DPLLU_INTERRUPT_ENABLE	0x4000
#define INC_SSI_INTERRUPT_ENABLE		0x8000
#define INC_DISABLE_INTERRUPT			0x0000

#define INC_INTERRUPT_POLARITY_HIGH		0x0000
#define INC_INTERRUPT_POLARITY_LOW		0x8000
#define INC_INTERRUPT_PULSE				0x0000
#define INC_INTERRUPT_LEVEL				0x4000
#define INC_INTERRUPT_AUTOCLEAR_DISABLE	0x0000
#define INC_INTERRUPT_AUTOCLEAR_ENABLE	0x2000
#define INC_EXT_INTERRUPT_POLARITY_HIGH	0x0000
#define INC_EXT_INTERRUPT_POLARITY_LOW	0x1000
#define INC_INTERRUPT_PULSE_COUNT		0x00FF
#define INC_INTERRUPT_PULSE_COUNT_MASK	0x03FF

#define T3900_SCAN_IF_DELAY				1500
#define T3900_SCAN_RF_DELAY				1500
#define T3900_PLAY_IF_DELAY				1000
#define T3900_PLAY_RF_DELAY				1100

#define WORD_SWAP(X)	(((X)>>8&0xff)|(((X)<<8)&0xff00))
#define DWORD_SWAP(X)	(((X)>>24&0xff)|(((X)>>8)&0xff00)|\
(((X)<<8)&0xff0000)|(((X)<<24)&0xff000000))

#define INC_REGISTER_CTRL(X)			((X)*0x1000)
#define STREAM_PARALLEL_ADDRESS			(0x40000000)
#define STREAM_PARALLEL_ADDRESS_CS		(0x50000000)

#define FIC_REF_TIME_OUT				2500
#define INC_SUCCESS						1
#define INC_ERROR						0
#define INC_NULL						0
#define INC_RETRY						0xff

#define INC_CER_PERIOD_TIME				1000
#define INC_CER_PERIOD	(3000 / INC_CER_PERIOD_TIME)
#define INC_BIT_PERIOD	(2000 / INC_CER_PERIOD_TIME)

#define INC_TDMB_LENGTH_MASK			0xFFF
#define TDMB_I2C_ID80					0x80
#define TDMB_I2C_ID82					0x82

#define MPI_CS_SIZE						(188*8)
#define INC_MPI_MAX_BUFF				(1024*8)
#define INC_INTERRUPT_SIZE				(188*20)
#define MAX_SUBCH_SIZE					32
#define MAX_SUBCHANNEL					64
#define MAX_LABEL_CHAR					16
#define SPI_INTERRUPT_SIZE				(188*8)

#define MAX_KOREABAND_FULL_CHANNEL		21
#define MAX_KOREABAND_NORMAL_CHANNEL	6
#define MAX_BAND_III_CHANNEL			41
#define MAX_L_BAND_CHANNEL				23
#define MAX_CHINA_CHANNEL				31
#define MAX_ROAMING_CHANNEL				12

#define RF500_REG_CTRL					200

#define APB_INT_BASE					0x0100
#define APB_GEN_BASE					0x0200
#define APB_PHY_BASE					0x0500
#define APB_DEINT_BASE					0x0600
#define APB_VTB_BASE					0x0700
#define APB_I2S_BASE					0x0800
#define APB_RDI_BASE					0x0900
#define APB_MPI_BASE					0x0A00
#define APB_RS_BASE						0x0B00
#define APB_SPI_BASE					0x0C00
#define APB_I2C_BASE					0x0D00
#define APB_RF_BASE						0x0E00

#define APB_FIC_BASE					0x1000
#define APB_STREAM_BASE					0x2000

#define TS_ERR_THRESHOLD				0x014C

#define END_MARKER						0xff
#define FIB_SIZE						32
#define FIB_WORD_SIZE					(FIB_SIZE/2)
#define MAX_FIB_NUM						12
#define MAX_FIC_SIZE					(MAX_FIB_NUM*FIB_SIZE)
#define MAX_FRAME_DURATION				96
#define MAX_USER_APP_DATA				32

enum ST_TRANSMISSION {
	TRANSMISSION_MODE1 = 1,
	TRANSMISSION_MODE2,
	TRANSMISSION_MODE3,
	TRANSMISSION_MODE4,
	TRANSMISSION_AUTO,
	TRANSMISSION_AUTOFAST,
} ;

enum INC_DPD_MODE {
	INC_DPD_OFF = 0,
	INC_DPD_ON,
} ;

enum PLL_MODE {
	INPUT_CLOCK_24576KHZ = 0,
	INPUT_CLOCK_12000KHZ,
	INPUT_CLOCK_19200KHZ,
	INPUT_CLOCK_27000KHZ,
	INPUT_CLOCK_27120KHZ,
} ;


enum ST_EXTENSION_TYPE {
	EXTENSION_0	= 0,
	EXTENSION_1,
	EXTENSION_2,
	EXTENSION_3,
	EXTENSION_4,
	EXTENSION_5,
	EXTENSION_6,
	EXTENSION_7,
	EXTENSION_8,
	EXTENSION_9,
	EXTENSION_10,
	EXTENSION_11,
	EXTENSION_12,
	EXTENSION_13,
	EXTENSION_14,
	EXTENSION_15,
	EXTENSION_16,
	EXTENSION_17,
	EXTENSION_18,
	EXTENSION_19,
	EXTENSION_20,
	EXTENSION_21,
	EXTENSION_22,
	EXTENSION_23,
	EXTENSION_24,
} ;

enum ST_SPI_CONTROL {
	SPI_REGREAD_CMD	= 0,
	SPI_REGWRITE_CMD,
	SPI_MEMREAD_CMD,
	SPI_MEMWRITE_CMD,
} ;

enum ST_TMID {
	TMID_0 = 0,
	TMID_1,
	TMID_2,
	TMID_3,
} ;

enum ST_PROTECTION_LEVEL {
	PROTECTION_LEVEL0 = 0,
	PROTECTION_LEVEL1,
	PROTECTION_LEVEL2,
	PROTECTION_LEVEL3,
} ;

enum ST_INDICATE {
	OPTION_INDICATE0 = 0,
	OPTION_INDICATE1,
} ;


enum ST_FICHEADER_TYPE {
	FIG_MCI_SI = 0,
	FIG_LABEL,
	FIG_RESERVED_0,
	FIG_RESERVED_1,
	FIG_RESERVED_2,
	FIG_FICDATA_CHANNEL,
	FIG_CONDITION_ACCESS,
	FIG_IN_HOUSE,
} ;

enum ST_SIMPLE_FIC {
	SIMPLE_FIC_ENABLE = 1,
	SIMPLE_FIC_DISABLE,
} ;

enum INC_CTRL {
	INC_DMB = 1,
	INC_DAB,
	INC_DATA,
	INC_MULTI,
	INC_SINGLE,

	FREQ_FREE = 0,
	FREQ_LOCK,
	FIC_OK = 1,
} ;

enum INC_ERROR_INFO {
	ERROR_NON					= 0x0000,
	ERROR_PLL					= 0xE000,
	ERROR_STOP					= 0xF000,
	ERROR_READY					= 0xFF00,
	ERROR_SYNC_NO_SIGNAL		= 0xFC01,
	ERROR_SYNC_LOW_SIGNAL		= 0xFD01,
	ERROR_SYNC_NULL				= 0xFE01,
	ERROR_SYNC_TIMEOUT			= 0xFF01,
	ERROR_FICDECODER			= 0xFF02,
	ERROR_START_MODEM_CLEAR		= 0xFF05,
	ERROR_USER_STOP				= 0xFA00,
	ERROR_MULTI_CHANNEL_COUNT_OVER	= 0x8000,
	ERROR_MULTI_CHANNEL_COUNT_NON	= 0x8001,
	ERROR_MULTI_CHANNEL_NULL	= 0x8002,
	ERROR_MULTI_CHANNEL_FREQ	= 0x8003,
	ERROR_MULTI_CHANNEL_DMB_MAX	= 0x8004,
} ;


#define INC_ENSEMBLE_LABLE_MAX				17
#define INC_SERVICE_MAX						32
#define INC_USER_APPLICATION_TYPE_LENGTH	32

struct ST_USER_APPLICATION_TYPE {
	unsigned char	ucDataLength;
	unsigned short	unUserAppType;
	unsigned char	aucData[MAX_USER_APP_DATA];
} ;

struct ST_USERAPP_GROUP_INFO {
	unsigned char	ucUAppSCId;
	unsigned char	ucUAppCount;
	struct ST_USER_APPLICATION_TYPE
		astUserApp[INC_USER_APPLICATION_TYPE_LENGTH];
} ;

struct INC_CHANNEL_INFO {
	unsigned int	ulRFFreq;
	unsigned short	uiEnsembleID;
	unsigned short	uiBitRate;
	unsigned char	uiTmID;
	char	aucLabel[MAX_LABEL_CHAR+1];
	char	aucEnsembleLabel[MAX_LABEL_CHAR+1];

	unsigned char	ucSubChID;
	unsigned char	ucServiceType;
	unsigned short	uiStarAddr;
	unsigned char	ucSlFlag;
	unsigned char	ucTableIndex;
	unsigned char	ucOption;
	unsigned char	ucProtectionLevel;
	unsigned short	uiDifferentRate;
	unsigned short	uiSchSize;

	unsigned int	ulServiceID;
	unsigned short	uiPacketAddr;

#ifdef USER_APPLICATION_TYPE
	ST_USERAPP_GROUP_INFO stUsrApp;
#endif
} ;

struct ST_SUBCH_INFO {
	short			nSetCnt;
	struct INC_CHANNEL_INFO astSubChInfo[MAX_SUBCH_SIZE];
} ;

enum UPLOAD_MODE_INFO {
	STREAM_UPLOAD_MASTER_SERIAL = 0,
	STREAM_UPLOAD_MASTER_PARALLEL,
	STREAM_UPLOAD_SLAVE_SERIAL,
	STREAM_UPLOAD_SLAVE_PARALLEL,
	STREAM_UPLOAD_SPI,
	STREAM_UPLOAD_TS,

} ;

enum CLOCK_SPEED {
	INC_OUTPUT_CLOCK_4096 = 1,
	INC_OUTPUT_CLOCK_2048,
	INC_OUTPUT_CLOCK_1024,

} ;

enum ENSEMBLE_BAND {
	KOREA_BAND_ENABLE = 0,
	BANDIII_ENABLE,
	LBAND_ENABLE,
	CHINA_ENABLE,
	ROAMING_ENABLE,
	EXTERNAL_ENABLE,

} ;

enum FREQ_LOCKINFO {
	INC_FREQUENCY_UNLOCK = 0,
	INC_FREQUENCY_LOCK,
} ;

enum CTRL_MODE {
	INC_I2C_CTRL = 0,
	INC_SPI_CTRL,
	INC_EBI_CTRL,
} ;

enum INC_ACTIVE_MODE {
	INC_ACTIVE_LOW = 0,
	INC_ACTIVE_HIGH,
} ;

struct INC_TDMB_MODE {
	enum ENSEMBLE_BAND m_ucRfBand;
	enum UPLOAD_MODE_INFO m_ucUploadMode;
	enum CLOCK_SPEED m_ucClockSpeed;
	enum INC_ACTIVE_MODE m_ucMPI_CS_Active;
	enum INC_ACTIVE_MODE m_ucMPI_CLK_Active;
	enum CTRL_MODE m_ucCommandMode;
	enum ST_TRANSMISSION m_ucTransMode;
	enum PLL_MODE m_ucPLL_Mode;
	enum INC_DPD_MODE m_ucDPD_Mode;
	unsigned short m_unIntCtrl;
} ;

#define BER_BUFFER_MAX		3
#define BER_REF_VALUE		35

struct ST_BBPINFO {
	unsigned int		ulFreq;
	enum INC_ERROR_INFO		nBbpStatus;
	unsigned char		ucStop;
	enum ST_TRANSMISSION	ucTransMode;

	unsigned char		ucAntLevel;
	unsigned char		ucSnr;
	unsigned char		ucVber;
	unsigned short		uiCER;
	unsigned short		wRssi;
	unsigned int		dPreBER;
	unsigned int		uiPostBER;

	unsigned int		ulReConfigTime;

	unsigned short		uiInCAntTick;
	unsigned short		uiInCERAvg;
	unsigned short		uiIncPostBER;
	unsigned short		auiANTBuff[BER_BUFFER_MAX];

	unsigned char		ucProtectionLevel;
	unsigned short		uiInCBERTick;
	unsigned short		uiBerSum;
	unsigned short		auiBERBuff[BER_BUFFER_MAX];

	unsigned char		ucCERCnt;
} ;

struct ST_FIB_INFO {
	unsigned short uiIsCRC;
	unsigned char ucDataPos;
	unsigned char aucBuff[FIB_SIZE];
} ;

struct ST_FIC {
	unsigned char	ucBlockNum;
	struct ST_FIB_INFO  stBlock;
} ;

union ST_FIG_HEAD {
	unsigned char ucInfo;
	struct {
		unsigned char bitLength:5;
		unsigned char bitType:3;
	} ITEM;
} ;

union ST_TYPE_0 {
	unsigned char ucInfo;
	struct  {
		unsigned char bitExtension:5;
		unsigned char bitPD:1;
		unsigned char bitOE:1;
		unsigned char bitCN:1;
	} ITEM;
} ;

union ST_TYPE_1 {
	unsigned char ucInfo;
	struct  {
		unsigned char bitExtension:3;
		unsigned char bitOE:1;
		unsigned char bitCharset:4;
	} ITEM;
} ;

union ST_USER_APPSERID_16 {
	unsigned short uiInfo;
	struct  {
		unsigned short bitServiceID:16;
	} ITEM;

} ;

union ST_USER_APPSERID_32 {
	unsigned int ulInfo;
	struct  {
		unsigned int bitServiceID:32;
	} ITEM;

} ;

union ST_USER_APP_IDnNUM  {
	unsigned char ucInfo;
	struct {
		unsigned char bitNomUserApp:4;
		unsigned char bitSCIdS:4;
	} ITEM;
} ;

union ST_USER_APPTYPE {
	unsigned short uiInfo;
	struct {
		unsigned short bitUserDataLength:5;
		unsigned short bitUserAppType:11;
	} ITEM;
} ;


union ST_TYPE0of0_INFO {
	unsigned int ulBuff;
	struct  {
		unsigned int bitLow_CIFCnt:8;
		unsigned int bitHigh_CIFCnt:5;
		unsigned int bitAlFlag:1;
		unsigned int bitChangFlag:2;
		unsigned int bitEld:16;
	} ITEM;
} ;

union ST_TYPE0of1Short_INFO {
	unsigned int nBuff;
	struct  {
		unsigned int bitReserved:8;
		unsigned int bitTableIndex:6;
		unsigned int bitTableSw:1;
		unsigned int bitShortLong:1;
		unsigned int bitStartAddr:10;
		unsigned int bitSubChId:6;
	} ITEM;
} ;

union ST_TYPE0of1Long_INFO {
	unsigned int nBuff;
	struct  {
		unsigned int bitSubChanSize:10;
		unsigned int bitProtecLevel:2;
		unsigned int bitOption:3;
		unsigned int bitShortLong:1;
		unsigned int bitStartAddr:10;
		unsigned int bitSubChId:6;
	} ITEM;
} ;

union ST_TYPE0of3Id_INFO {
	unsigned int ulData;
	struct {
		unsigned int bitReserved:8;
		unsigned int bitPacketAddr:10;
		unsigned int bitSubChId:6;
		unsigned int bitDScType:6;
		unsigned int bitRfu:1;
		unsigned int bitFlag:1;
	} ITEM;
} ;

union ST_TYPE0of3_INFO {
	unsigned short nData;
	struct {
		unsigned short bitCAOrgFlag:1;
		unsigned short bitReserved:3;
		unsigned short bitScid:12;
	} ITEM;
} ;

union ST_SERVICE_COMPONENT {
	unsigned char ucData;
	struct  {
		unsigned char bitNumComponent:4;
		unsigned char bitCAId:3;
		unsigned char bitLocalFlag:1;
	} ITEM;
} ;

union ST_TMId_MSCnFIDC {
	unsigned short uiData;
	struct  {
		unsigned short bitCAflag:1;
		unsigned short bitPS:1;
		unsigned short bitSubChld:6;
		unsigned short bitAscDscTy:6;
		unsigned short bitTMId:2;
	} ITEM;
} ;

union ST_MSC_PACKET_INFO {
	unsigned short usData;
	struct  {
		unsigned short bitCAflag:1;
		unsigned short bitPS:1;
		unsigned short bitSCId:12;
		unsigned short bitTMId:2;
	} ITEM;
} ;

union ST_MSC_BIT {
	unsigned char ucData;
	struct {
		unsigned char bitScIds:4;
		unsigned char bitRfa:3;
		unsigned char bitExtFlag:1;
	} ITEM;
} ;

union ST_MSC_LONG {
	unsigned short usData;
	struct  {
		unsigned short bitScId:12;
		unsigned short bitDummy:3;
		unsigned short bitLsFlag:1;
	} ITEM;
} ;

union ST_MSC_SHORT {
	unsigned char ucData;
	struct   {
		unsigned char bitSUBnFIDCId:6;
		unsigned char bitMscFicFlag:1;
		unsigned char bitLsFlag:1;
	} ITEM;
} ;

union ST_EXTENSION_TYPE14 {
	unsigned char ucData;
	struct  {
		unsigned char bitSCidS:4;
		unsigned char bitRfa:3;
		unsigned char bitPD:1;
	} ITEM;
} ;

union ST_EXTENSION_TYPE12 {
	unsigned char ucData;
	struct  {
		unsigned char bitReserved1:6;
		unsigned char bitCF_flag:1;
		unsigned char bitCountry:1;
	} ITEM;
} ;

union ST_UTC_SHORT_INFO {
	unsigned int ulBuff;
	struct  {
		unsigned int bitMinutes:6;
		unsigned int bitHours:5;
		unsigned int bitUTC_Flag:1;
		unsigned int bitConf_Ind:1;
		unsigned int bitLSI:1;
		unsigned int bitMJD:17;
		unsigned int bitRfu:1;
	} ITEM;
} ;

union ST_UTC_LONG_INFO {
	unsigned short unBuff;
	struct  {
		unsigned short bitMilliseconds:10;
		unsigned short bitSeconds:6;
	} ITEM;
} ;

struct ST_DATE_T {
	unsigned short	usYear;
	unsigned char	ucMonth;
	unsigned char	ucDay;
	unsigned char	ucHour;
	unsigned char	ucMinutes;
	unsigned char   ucSeconds;
	unsigned short  uiMilliseconds;

} ;


struct ST_FICDB_SERVICE_COMPONENT {
	unsigned int	ulSid;
	unsigned char	ucSCidS;
	unsigned char	ucSubChid;
	unsigned char	ucTmID;
	unsigned char	ucCAFlag;
	unsigned char	ucPS;
	unsigned char	ucDSCType;
	unsigned char	aucComponentLabels[INC_ENSEMBLE_LABLE_MAX];
	unsigned char	ucIsComponentLabel;
	unsigned char	aucLabels[INC_ENSEMBLE_LABLE_MAX];
	unsigned short	unEnsembleFlag;
	unsigned char	ucIsLable;
	unsigned short	unPacketAddr;
	unsigned char	ucCAOrgFlag;
	unsigned char	ucDGFlag;
	unsigned short	unCAOrg;
	unsigned short	unStartAddr;
	unsigned char   ucShortLong;
	unsigned char   ucTableSW;
	unsigned char   ucTableIndex;
	unsigned char   ucOption;
	unsigned char	ucProtectionLevel;
	unsigned short  uiSubChSize;
	unsigned short  uiBitRate;
	unsigned short  uiDifferentRate;
	unsigned char	IsOrganiza;
	unsigned char	IsPacketMode;
	unsigned char	ucIsAppData;
	struct ST_USERAPP_GROUP_INFO stUApp;

} ;

struct ST_STREAM_INFO {
	short nPrimaryCnt;
	short nSecondaryCnt;
	struct ST_FICDB_SERVICE_COMPONENT	astPrimary[INC_SERVICE_MAX];
	struct ST_FICDB_SERVICE_COMPONENT	astSecondary[INC_SERVICE_MAX];

} ;


struct ST_FICDB_LIST {
	short		nLabelCnt;
	short		nEmsembleLabelFlag;
	short		nSubChannelCnt;
	short		nPacketCnt;
	short		nPacketModeCnt;
	unsigned char		ucIsSimpleFIC;
	unsigned char		ucIsSimpleCnt;
	enum ST_SIMPLE_FIC	ucIsSetSimple;
	unsigned short		unEnsembleID;
	unsigned char		ucChangeFlag;
	unsigned short		unCIFCount;
	unsigned char		ucOccurrence;

	unsigned char		aucEnsembleName[INC_ENSEMBLE_LABLE_MAX];
	unsigned short		unEnsembleFlag;
	unsigned char		ucIsEnsembleName;

	struct ST_STREAM_INFO	stDMB;
	struct ST_STREAM_INFO	stDAB;
	struct ST_STREAM_INFO	stFIDC;
	struct ST_STREAM_INFO	stDATA;

	struct ST_DATE_T		stDate;
	short		nUserAppCnt;
} ;

#ifdef	INC_FIFO_SOURCE_ENABLE
#define	INC_CIF_MAX_SIZE		(188*8)
#define	INC_FIFO_DEPTH		(1024*5)
#define	MAX_CHANNEL_FIFO		5
#define	MAX_HEADER_SIZE			16
#define	HEADER_SERACH_SIZE		(INC_CIF_MAX_SIZE + MAX_HEADER_SIZE)
#define	HEADER_ID_0x33			0x33
#define	HEADER_ID_0x00			0x00
#define	HEADER_SIZE_BITMASK		0x3FF
#define	INC_HEADER_CHECK_BUFF	(INC_CIF_MAX_SIZE*4)

enum MULTI_CHANNEL_INFO {
	MAIN_INPUT_DATA = 0,
	FIC_STREAM_DATA,
	CHANNEL1_STREAM_DATA,
	CHANNEL2_STREAM_DATA,
	CHANNEL3_STREAM_DATA,
} ;

enum ST_HEADER_INFO {
	INC_HEADER_SIZE_ERROR = 0,
	INC_HEADER_NOT_SEARACH,
	INC_HEADER_GOOD,
} ;

#define INC_SUB_CHANNEL_ID_MASK			0xFFFF
struct ST_FIFO {
	unsigned int	ulDepth;
	unsigned int	ulFront;
	unsigned int	ulRear;
	unsigned short	unSubChID;
	unsigned char	acBuff[INC_FIFO_DEPTH+1];
} ;

void		INC_MULTI_SORT_INIT(void);
unsigned char	INC_QFIFO_INIT(struct ST_FIFO *pFF,  unsigned int ulDepth);
unsigned char	INC_QFIFO_ADD(struct ST_FIFO *pFF,
				unsigned char *pData, unsigned int ulSize);
unsigned char	INC_QFIFO_AT(struct ST_FIFO *pFF,
				unsigned char *pData, unsigned int ulSize);
unsigned char	INC_QFIFO_BRING(struct ST_FIFO *pFF,
				unsigned char *pData, unsigned int ulSize);
unsigned char	INC_GET_ID_BRINGUP(unsigned short unID,
				unsigned char *pData, unsigned int ulSize);
unsigned char	INC_MULTI_FIFO_PROCESS(
	unsigned char *pData, unsigned int ulSize);

unsigned int	INC_QFIFO_FREE_SIZE(struct ST_FIFO *pFF);
unsigned int	INC_QFIFO_GET_SIZE(struct ST_FIFO *pFF);
unsigned int	INC_GET_IDS_SIZE(unsigned short unID);

struct ST_FIFO *INC_GET_CHANNEL_FIFO(enum MULTI_CHANNEL_INFO ucIndex);
enum ST_HEADER_INFO INC_HEADER_CHECK(struct ST_FIFO *pMainFifo);
#endif


enum INC_SORT_OPTION {
	INC_SUB_CHANNEL_ID = 0,
	INC_START_ADDRESS,
	INC_BIRRATE,
	INC_FREQUENCY,
} ;

void INC_MSG_PRINTF(char nFlag, char *pFormat, ...);
void INC_SCAN_SETTING(unsigned char ucI2CID);
void INC_AIRPLAY_SETTING(unsigned char ucI2CID);
void INC_DELAY(unsigned short uiDelay);
void INC_MPICLOCK_SET(unsigned char ucI2CID);
void INC_UPLOAD_MODE(unsigned char ucI2CID);
void INC_INTERRUPT_CLEAR(unsigned char ucI2CID, unsigned short uiClrInt);
void INC_INTERRUPT_CTRL(unsigned char ucI2CID);
void INC_INTERRUPT_SET(unsigned char ucI2CID, unsigned short uiSetInt);
void INC_SET_CHANNEL(unsigned char ucI2CID, struct ST_SUBCH_INFO *pChInfo);
void INC_BUBBLE_SORT(struct ST_SUBCH_INFO *pMainInfo, enum INC_SORT_OPTION Opt);
void INC_SWAP(struct ST_SUBCH_INFO *pMainInfo,
	unsigned short nNo1, unsigned short nNo2);
void INTERFACE_USER_STOP(unsigned char ucI2CID);
void INTERFACE_USER_STOP_CLEAR(unsigned char ucI2CID);
void INTERFACE_INT_ENABLE(unsigned char ucI2CID, unsigned short unSet);
void INTERFACE_INT_CLEAR(unsigned char ucI2CID, unsigned short unClr);
void INC_INITDB(unsigned char ucI2CID);
void INC_EXTENSION_000(struct ST_FIB_INFO *pFibInfo);
void INC_EXTENSION_001(struct ST_FIB_INFO *pFibInfo);
void INC_EXTENSION_002(struct ST_FIB_INFO *pFibInfo);
void INC_EXTENSION_003(struct ST_FIB_INFO *pFibInfo);
void INC_EXTENSION_008(struct ST_FIB_INFO *pFibInfo);
void INC_INIT_MPI(unsigned char ucI2CID);
#ifdef USER_APPLICATION_TYPE
void INC_EXTENSION_013(struct ST_FIB_INFO *pFibInfo);
#endif
void INC_EXTENSION_110(struct ST_FIB_INFO *pFibInfo);
void INC_EXTENSION_111(struct ST_FIB_INFO *pFibInfo);
void INC_EXTENSION_112(struct ST_FIB_INFO *pFibInfo);
void INC_EXTENSION_113(struct ST_FIB_INFO *pFibInfo);
void INC_EXTENSION_114(struct ST_FIB_INFO *pFibInfo);
void INC_EXTENSION_115(struct ST_FIB_INFO *pFibInfo);
void INC_SET_FICTYPE_1(struct ST_FIB_INFO *pFibInfo);

void INC_SET_FICTYPE_5(struct ST_FIB_INFO *pFibInfo);
void INC_SET_UPDATEFIC(struct ST_FIB_INFO *pstDestData,
		unsigned char *pSourData);
void INTERFACE_UPLOAD_MODE(unsigned char ucI2CID,
		enum UPLOAD_MODE_INFO ucUploadMode);
void INC_UPDATE_LIST(struct INC_CHANNEL_INFO *pUpDateCh,
		struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent);

unsigned char INC_GET_FIND_TYPE(union ST_FIG_HEAD *pInfo);
unsigned char INTERFACE_PLL_MODE(unsigned char ucI2CID,
			enum PLL_MODE ucPllMode);
unsigned char INTERFACE_DBINIT(void);
unsigned char INTERFACE_INIT(unsigned char ucI2CID);
unsigned char INTERFACE_RECONFIG(unsigned char ucI2CID);
unsigned char INTERFACE_STATUS_CHECK(unsigned char ucI2CID);
unsigned char INTERFACE_START(unsigned char ucI2CID,
			struct ST_SUBCH_INFO *pChInfo);
unsigned char INTERFACE_START_TEST(
		unsigned char ucI2CID, struct ST_SUBCH_INFO *pChInfo);
unsigned char INTERFACE_INT_CHECK(unsigned char ucI2CID);
unsigned char INC_BUBBLE_SORT_by_TYPE(struct ST_SUBCH_INFO *pMainInfo);
unsigned char INTERFACE_SCAN(unsigned char ucI2CID, unsigned int ulFreq);
unsigned char INTERFACE_GET_SNR(unsigned char ucI2CID);
unsigned char INTERFACE_ISR(unsigned char ucI2CID, unsigned char *pBuff);
unsigned char INC_CHECK_SERVICE_DB16(unsigned short *ptr, unsigned short wVal,
				 unsigned short wNum);
unsigned char INC_CHECK_SERVICE_DB8(unsigned char *ptr, unsigned char cVal,
				unsigned char cNum);

unsigned char INC_GET_BYTEDATA(struct ST_FIB_INFO *pFibInfo);
unsigned char INC_GETAT_BYTEDATA(struct ST_FIB_INFO *pFibInfo);
unsigned char INC_GET_HEADER(struct ST_FIB_INFO *pInfo);

unsigned char INC_GET_TYPE(struct ST_FIB_INFO *pInfo);
unsigned char INC_GETAT_HEADER(struct ST_FIB_INFO *pInfo);
unsigned char INC_GETAT_TYPE(struct ST_FIB_INFO *pInfo);
unsigned char INC_INIT(unsigned char ucI2CID);
unsigned char INC_READY(unsigned char ucI2CID, unsigned int ulFreq);
unsigned char INC_SYNCDETECTOR(unsigned char ucI2CID, unsigned int ulFreq,
			   unsigned char ucScanMode);
unsigned char INC_FICDECODER(unsigned char ucI2CID,
			enum ST_SIMPLE_FIC bSimpleFIC);
unsigned char INC_FIC_UPDATE(unsigned char ucI2CID,
			struct ST_SUBCH_INFO *pChInfo,
			 enum ST_SIMPLE_FIC bSimpleFIC);
unsigned char INC_START(unsigned char ucI2CID, struct ST_SUBCH_INFO *pChInfo,
		    unsigned short IsEnsembleSame);
unsigned char INC_STOP(unsigned char ucI2CID);
unsigned char INC_CHANNEL_START(unsigned char ucI2CID,
				struct ST_SUBCH_INFO *pChInfo);
unsigned char INC_CHANNEL_START_TEST(
		unsigned char ucI2CID, struct ST_SUBCH_INFO *pChInfo);
unsigned char INC_ENSEMBLE_SCAN(unsigned char ucI2CID, unsigned int ulFreq);
unsigned char INC_FIC_RECONFIGURATION_HW_CHECK(unsigned char ucI2CID);
unsigned char INC_STATUS_CHECK(unsigned char ucI2CID);
unsigned char INC_GET_ANT_LEVEL(unsigned char ucI2CID);
unsigned char INC_GET_SNR(unsigned char ucI2CID);
unsigned char INC_GET_VBER(unsigned char ucI2CID);
unsigned char INC_GET_NULLBLOCK(union ST_FIG_HEAD *pInfo);
unsigned char INC_GET_FINDLENGTH(union ST_FIG_HEAD *pInfo);
unsigned char INC_SET_TRANSMIT_MODE(unsigned char ucMode);
unsigned char INC_GET_FINDTYPE(union ST_FIG_HEAD *pInfo);
unsigned char INC_GET_NULL_BLOCK(union ST_FIG_HEAD *pInfo);
unsigned char INC_GET_FIND_LENGTH(union ST_FIG_HEAD *pInfo);
unsigned char INC_FICPARSING(unsigned char ucI2CID, unsigned char *pucFicBuff,
			 int uFicLength, enum ST_SIMPLE_FIC bSimpleFIC);
unsigned char INC_CMD_WRITE(unsigned char ucI2CID, unsigned short uiAddr,
			unsigned short uiData);
unsigned char INC_I2C_WRITE(unsigned char ucI2CID, unsigned short uiAddr,
			unsigned short uiData);
unsigned char INC_EBI_WRITE(unsigned char ucI2CID, unsigned short uiAddr,
			unsigned short uiData);
unsigned char INC_RE_SYNC(unsigned char ucI2CID);
unsigned char INC_PLL_SET(unsigned char ucI2CID);
unsigned char SAVE_CHANNEL_INFO(char *pStr);
unsigned char LOAD_CHANNEL_INFO(char *pStr);
unsigned char INC_I2C_READ_BURST(unsigned char ucI2CID, unsigned short uiAddr,
			     unsigned char *pData, unsigned short nSize);
unsigned char INC_SPI_REG_WRITE(unsigned char ucI2CID, unsigned short uiAddr,
			    unsigned short uiData);
unsigned char INC_CMD_READ_BURST(unsigned char ucI2CID, unsigned short uiAddr,
			     unsigned char *pData, unsigned short nSize);
unsigned char INC_SPI_READ_BURST(unsigned char ucI2CID, unsigned short uiAddr,
			     unsigned char *pBuff, unsigned short wSize);
unsigned char INC_CHIP_STATUS(unsigned char ucI2CID);
unsigned char INC_RF500_START(unsigned char ucI2CID, unsigned int ulRFChannel,
			  enum ENSEMBLE_BAND ucBand, enum PLL_MODE ucPLL);
unsigned char INC_RF500_I2C_WRITE(unsigned char ucI2CID, unsigned char *pucData,
			      unsigned int uLength);
unsigned char INC_GET_FIB_CNT(enum ST_TRANSMISSION ucMode);
unsigned char INC_EBI_READ_BURST(unsigned char ucI2CID, unsigned short uiAddr,
			     unsigned char *pData, unsigned short nSize);
unsigned char *INTERFACE_GETENSEMBLE_LABEL(unsigned char ucI2CID);
struct INC_TDMB_MODE *INC_GET_TDMBMODE(unsigned char ucI2CID);
void INC_SET_TDMBMODE(unsigned char ucI2CID, struct INC_TDMB_MODE *stMode);

short INC_CHECK_SERVICE_CNT16(unsigned short *ptr, unsigned short wVal,
				  unsigned char cNum, unsigned short wMask);
short INC_CHECK_SERVICE_CNT8(unsigned char *ptr, unsigned char cVal,
				 unsigned char cNum, unsigned char cMask);
short INC_CHECK_SERVICE_CNT32(unsigned int *ptr, unsigned int wVal,
				  unsigned char cNum);
short INC_GET_RSSI(unsigned char ucI2CID);
unsigned short INC_EBI_READ(unsigned char ucI2CID, unsigned short uiAddr);
unsigned short INC_FIND_KOR_FREQ(unsigned int ulFreq);
unsigned short INC_CRC_CHECK(unsigned char *pBuf, unsigned char ucSize);
unsigned short INC_SET_FICTYPE_0(struct ST_FIB_INFO *pFibInfo);
unsigned short INC_GET_WORDDATA(struct ST_FIB_INFO *pFibInfo);
unsigned short INC_GETAT_WORDDATA(struct ST_FIB_INFO *pFibInfo);
unsigned short INC_FIND_SUBCH_SIZE(unsigned char ucTableIndex);
unsigned short INTERFACE_GET_CER(unsigned char ucI2CID);
unsigned short INTERFACE_GETDMB_CNT(void);
unsigned short INTERFACE_GETDAB_CNT(void);
unsigned short INTERFACE_GETDATA_CNT(void);
unsigned short INC_CMD_READ(unsigned char ucI2CID, unsigned short uiAddr);
unsigned short	INC_I2C_READ(unsigned char ucI2CID, unsigned short ulAddr);
unsigned short	INC_SPI_REG_READ(unsigned char ucI2CID, unsigned short uiAddr);
unsigned short INC_GET_FRAME_DURATION(enum ST_TRANSMISSION cTrnsMode);
unsigned short INC_GET_CER(unsigned char ucI2CID);
unsigned short INC_RESYNC_CER(unsigned char ucI2CID);

unsigned int INC_GET_KOREABAND_FULL_TABLE(unsigned short uiIndex);
unsigned int INC_GET_KOREABAND_NORMAL_TABLE(unsigned short uiIndex);
unsigned int INC_GET_LONGDATA(struct ST_FIB_INFO *pFibInfo);
unsigned int INC_GETAT_LONGDATA(struct ST_FIB_INFO *pFibInfo);
unsigned int YMDtoMJD(struct ST_DATE_T stDate);
void MJDtoYMD(unsigned short wMJD, struct ST_DATE_T *pstDate);

unsigned int INTERFACE_GET_POSTBER(unsigned char ucI2CID);
unsigned int INTERFACE_GET_PREBER(unsigned char ucI2CID);
unsigned int INC_GET_PREBER(unsigned char ucI2CID);
unsigned int INC_GET_POSTBER(unsigned char ucI2CID);

struct ST_BBPINFO *INC_GET_STRINFO(unsigned char ucI2CID);
enum INC_ERROR_INFO INTERFACE_ERROR_STATUS(unsigned char ucI2CID);
struct INC_CHANNEL_INFO *INTERFACE_GETDB_DMB(short uiPos);
struct INC_CHANNEL_INFO *INTERFACE_GETDB_DAB(short uiPos);
struct INC_CHANNEL_INFO *INTERFACE_GETDB_DATA(short uiPos);

unsigned char INC_ADD_ENSEMBLE_NAME(
		unsigned short unEnID, unsigned char *pcLabel);
struct ST_FICDB_LIST *INC_GET_FICDB_LIST(void);
void INC_FIND_BASIC_SERVICE(unsigned int ulServiceId, unsigned short unData);
void INC_ADD_BASIC_SERVICE(
		struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent,
		unsigned int ulServiceId, unsigned short unData);

void INC_ADD_ORGANIZAION_SUBCHANNEL_ID(
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent,
	unsigned int ulTypeInfo);
void INC_SORT_ORGANIZAION_SUBCHANNEL_ID(
	struct ST_STREAM_INFO *pStreamInfo,
	unsigned char ucSubChID, unsigned int ulTypeInfo);
unsigned char INC_SORT_SIMPLE_ORGANIZAION_SUBCHANNEL_ID(
	struct ST_STREAM_INFO *pStreamInfo, unsigned char ucSubChID,
	unsigned short unStartAddr, unsigned int ulTypeInfo);
void INC_FIND_ORGANIZAION_SUBCHANNEL_ID(unsigned char ucSubChID,
	unsigned int ulTypeInfo);

void INC_FIND_SIMPLE_ORGANIZAION_SUBCHANNEL_ID(
	unsigned char ucSubChID,
	unsigned short unStartAddr,
	unsigned int ulTypeInfo);

unsigned short INC_GET_BITRATE(
		struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent);
void INC_GET_LONG_FORM(
		struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent,
		union ST_TYPE0of1Long_INFO *pLong);
void INC_GET_SHORT_FORM(
		struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent,
		union ST_TYPE0of1Short_INFO *pShort);

void INC_ADD_ENSEMBLE_ID(union ST_TYPE0of0_INFO *pIdInfo);
void INC_FIND_PACKET_MODE(
		union ST_TYPE0of3_INFO *pTypeInfo,
		union ST_TYPE0of3Id_INFO *pIdInfo);

void INC_FIND_GLOBAL_SERVICE_COMPONENT_LONG(
	unsigned int ulSvcID,
	union ST_MSC_LONG *pstMscLong, union ST_MSC_BIT *pstMsc);
void INC_ADD_GLOBAL_SERVICE_COMPONENT_LONG(
		struct ST_STREAM_INFO *pStreamInfo,
		unsigned int ulSvcID,
		union ST_MSC_LONG *pstMscLong, union ST_MSC_BIT *pstMsc);
void INC_FIND_GLOBAL_SERVICE_COMPONENT_SHORT(unsigned int ulSvcID,
		union ST_MSC_SHORT *pstMscShort, union ST_MSC_BIT *pstMsc);
void INC_ADD_GLOBAL_SERVICE_COMPONENT_SHORT(
		struct ST_STREAM_INFO *pStreamInfo, unsigned int ulSvcID,
		union ST_MSC_SHORT *pstMscShort, union ST_MSC_BIT *pstMsc);
void INC_FIND_SERVICE_COMPONENT_LABEL(
	unsigned int ulSID, unsigned char *pcLabel);
void INC_FIND_DATA_SERVICE_COMPONENT_LABEL(
		unsigned int ulSID, unsigned char *pcLabel);
unsigned short INC_FIND_SUB_CHANNEL_SIZE(unsigned char ucTableIndex);
void	INC_LABEL_FILTER(unsigned char *pData, short nSize);
void	INC_ADD_SERVICE_LABEL(struct ST_STREAM_INFO *pStreamInfo,
				unsigned int ulSID, unsigned char *pcLabel);
void INC_FIND_USERAPP_TYPE(unsigned int ulSID,
			struct ST_USERAPP_GROUP_INFO *pstData);
unsigned char INC_ADD_USERAPP_TYPE(unsigned int ulSID,
		 struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent,
		 short nCnt, struct ST_USERAPP_GROUP_INFO *pstData);

unsigned char INC_DB_UPDATE(unsigned int ulFreq, struct ST_SUBCH_INFO *pDMB,
			struct ST_SUBCH_INFO *pDAB, struct ST_SUBCH_INFO *pDATA,
			struct ST_SUBCH_INFO *pFIDC);
void INC_ADD_SERVICE_COMPONENT_LABEL(
	struct ST_STREAM_INFO *pStreamInfo,
	unsigned int ulSID,
	unsigned char *pcLabel);
void INC_DB_COPY(unsigned int ulFreq, short nCnt,
	struct INC_CHANNEL_INFO *pChInfo,
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent);

unsigned short INC_GET_SAMSUNG_BER(unsigned char ucI2CID);
unsigned char INC_GET_SAMSUNG_ANT_LEVEL(unsigned char ucI2CID);
unsigned short INC_GET_SAMSUNG_BER_FOR_FACTORY_MODE(unsigned char ucI2CID);

extern struct spi_device *spi_dmb;

#endif
