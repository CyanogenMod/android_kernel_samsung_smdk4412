/*****************************************************************************
 Copyright(c) 2011 I&C Inc. All Rights Reserved

 File name : INC_INTERFACE.c

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

#include "INC_INCLUDES.h"
#include "tdmb.h"
#include <linux/mutex.h>

/********************** Comment****************/
/* Operating Chip set : T3900                  */
/* Software version   : version 1.22           */
/* Software Update    : 2011.04.21             */
/************************************************/
static DEFINE_MUTEX(tdmb_mutex);

#define INC_MUTEX_LOCK()	mutex_lock(&tdmb_mutex)
#define INC_MUTEX_FREE()    mutex_unlock(&tdmb_mutex)
struct ST_SUBCH_INFO g_stDmbInfo;
struct ST_SUBCH_INFO g_stDabInfo;
struct ST_SUBCH_INFO g_stDataInfo;
struct ST_SUBCH_INFO g_stFIDCInfo;


void INC_DELAY(unsigned short uiDelay)
{
	msleep(uiDelay);
}

void INC_MSG_PRINTF(char nFlag, char *pFormat, ...)
{
	va_list Ap;
	unsigned short nSize;
	char acTmpBuff[1012] = {0};
	va_start(Ap, pFormat);
	nSize = vsprintf(acTmpBuff, pFormat, Ap);
	va_end(Ap);
}

unsigned short INC_I2C_READ(unsigned char ucI2CID, unsigned short uiAddr)
{
	unsigned short uiRcvData = 0;
	return uiRcvData;
}

unsigned char INC_I2C_WRITE(unsigned char ucI2CID, unsigned short uiAddr,
			unsigned short uiData)
{
	return INC_SUCCESS;
}

unsigned char INC_I2C_READ_BURST(unsigned char ucI2CID, unsigned short uiAddr,
				unsigned char *pData, unsigned short nSize)
{
	return INC_SUCCESS;
}

unsigned char INC_EBI_WRITE(unsigned char ucI2CID, unsigned short uiAddr,
				unsigned short uiData)
{
	unsigned short uiCMD = INC_REGISTER_CTRL(SPI_REGWRITE_CMD) | 1;
	unsigned short uiNewAddr =
	  (ucI2CID == TDMB_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;
	INC_MUTEX_LOCK();
	*(unsigned char *)STREAM_PARALLEL_ADDRESS = uiNewAddr >> 8;
	*(unsigned char *)STREAM_PARALLEL_ADDRESS = uiNewAddr & 0xff;
	*(unsigned char *)STREAM_PARALLEL_ADDRESS = uiCMD >> 8;
	*(unsigned char *)STREAM_PARALLEL_ADDRESS = uiCMD & 0xff;
	*(unsigned char *)STREAM_PARALLEL_ADDRESS =
		  (uiData >> 8) & 0xff;
	*(unsigned char *)STREAM_PARALLEL_ADDRESS = uiData & 0xff;
	INC_MUTEX_FREE();
	return INC_SUCCESS;
}

unsigned short INC_EBI_READ(unsigned char ucI2CID, unsigned short uiAddr)
{
	unsigned short uiRcvData = 0;
	unsigned short uiCMD = INC_REGISTER_CTRL(SPI_REGREAD_CMD) | 1;
	unsigned short uiNewAddr =
	   (ucI2CID == TDMB_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;
	INC_MUTEX_LOCK();
	*(unsigned char *)STREAM_PARALLEL_ADDRESS = uiNewAddr >> 8;
	*(unsigned char *)STREAM_PARALLEL_ADDRESS = uiNewAddr & 0xff;
	*(unsigned char *)STREAM_PARALLEL_ADDRESS = uiCMD >> 8;
	*(unsigned char *)STREAM_PARALLEL_ADDRESS = uiCMD & 0xff;
	uiRcvData =
			(*(unsigned char *)STREAM_PARALLEL_ADDRESS & 0xff) << 8;
	uiRcvData |= (*(unsigned char *)STREAM_PARALLEL_ADDRESS & 0xff);
	INC_MUTEX_FREE();
	return uiRcvData;
}

unsigned char INC_EBI_READ_BURST(unsigned char ucI2CID, unsigned short uiAddr,
				unsigned char *pData, unsigned short nSize)
{
	unsigned short uiLoop, nIndex = 0, anLength[2], uiCMD, unDataCnt;
	unsigned short uiNewAddr =
	  (ucI2CID == TDMB_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;
	if (nSize > INC_MPI_MAX_BUFF)
		return INC_ERROR;
	memset((char *) anLength, 0, sizeof(anLength));
	if (nSize > INC_TDMB_LENGTH_MASK) {
		anLength[nIndex++] = INC_TDMB_LENGTH_MASK;
		anLength[nIndex++] = nSize - INC_TDMB_LENGTH_MASK;
	} else
		anLength[nIndex++] = nSize;

	INC_MUTEX_LOCK();
	for (uiLoop = 0; uiLoop < nIndex; uiLoop++) {
		uiCMD =
			INC_REGISTER_CTRL(SPI_MEMREAD_CMD) | (anLength[uiLoop] &
						INC_TDMB_LENGTH_MASK);
		*(unsigned char *)STREAM_PARALLEL_ADDRESS =
		   uiNewAddr >> 8;
		*(unsigned char *)STREAM_PARALLEL_ADDRESS =
		   uiNewAddr & 0xff;
		*(unsigned char *)STREAM_PARALLEL_ADDRESS = uiCMD >> 8;
		*(unsigned char *)STREAM_PARALLEL_ADDRESS = uiCMD & 0xff;
		for (unDataCnt = 0; unDataCnt < anLength[uiLoop];
			unDataCnt++) {
			*pData++ =
			*(unsigned char *)STREAM_PARALLEL_ADDRESS &	0xff;
		}
	}
	INC_MUTEX_FREE();
	return INC_SUCCESS;
}

unsigned short INC_SPI_REG_READ(unsigned char ucI2CID, unsigned short uiAddr)
{
	unsigned short uiRcvData = 0;
	unsigned short uiNewAddr =
		(ucI2CID == TDMB_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;
	unsigned short uiCMD = INC_REGISTER_CTRL(SPI_REGREAD_CMD) | 1;
	unsigned char auiBuff[6];
	unsigned char cCnt = 0;
	unsigned char acRxBuff[2];
	struct spi_message msg;
	struct spi_transfer transfer[2];
	struct spi_device *_device;
	int status;

	_device = tdmb_get_spi_handle();

	INC_MUTEX_LOCK();
	auiBuff[cCnt++] = uiNewAddr >> 8;
	auiBuff[cCnt++] = uiNewAddr & 0xff;
	auiBuff[cCnt++] = uiCMD >> 8;
	auiBuff[cCnt++] = uiCMD & 0xff;

	memset(&msg, 0, sizeof(msg));
	memset(transfer, 0, sizeof(transfer));
	spi_message_init(&msg);
	msg.spi = _device;
	transfer[0].tx_buf = (u8 *) auiBuff;
	transfer[0].rx_buf = (u8 *) NULL;
	transfer[0].len = 4;
	transfer[0].bits_per_word = 8;
	transfer[0].delay_usecs = 0;
	spi_message_add_tail(&(transfer[0]), &msg);
	transfer[1].tx_buf = (u8 *) NULL;
	transfer[1].rx_buf = (u8 *) acRxBuff;
	transfer[1].len = 2;
	transfer[1].bits_per_word = 8;
	transfer[1].delay_usecs = 0;
	spi_message_add_tail(&(transfer[1]), &msg);
	status = spi_sync(_device, &msg);
	uiRcvData = (unsigned short) (acRxBuff[0] << 8) |
		(unsigned short) acRxBuff[1];

	    /* TODO SPI Read code here... */
	INC_MUTEX_FREE();
	return uiRcvData;
}

unsigned char INC_SPI_REG_WRITE(unsigned char ucI2CID, unsigned short uiAddr,
			       unsigned short uiData)
{
	unsigned short uiNewAddr =
	   (ucI2CID == TDMB_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;
	unsigned short uiCMD = INC_REGISTER_CTRL(SPI_REGWRITE_CMD) | 1;
	unsigned char auiBuff[6];
	unsigned char cCnt = 0;

	struct spi_message msg;
	struct spi_transfer transfer;
	struct spi_device *_device;
	int status;

	_device = tdmb_get_spi_handle();

	INC_MUTEX_LOCK();
	auiBuff[cCnt++] = uiNewAddr >> 8;
	auiBuff[cCnt++] = uiNewAddr & 0xff;
	auiBuff[cCnt++] = uiCMD >> 8;
	auiBuff[cCnt++] = uiCMD & 0xff;
	auiBuff[cCnt++] = uiData >> 8;
	auiBuff[cCnt++] = uiData & 0xff;
	memset(&msg, 0, sizeof(msg));
	memset(&transfer, 0, sizeof(transfer));
	spi_message_init(&msg);
	msg.spi = _device;
	transfer.tx_buf = (u8 *) auiBuff;
	transfer.rx_buf = NULL;
	transfer.len = 6;
	transfer.bits_per_word = 8;
	transfer.delay_usecs = 0;
	spi_message_add_tail(&transfer, &msg);
	status = spi_sync(_device, &msg);
	INC_MUTEX_FREE();
	return INC_SUCCESS;
}

unsigned char INC_SPI_READ_BURST(unsigned char ucI2CID, unsigned short uiAddr,
				unsigned char *pBuff, unsigned short wSize)
{
	unsigned short uiNewAddr =
	    (ucI2CID == TDMB_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;
	unsigned short uiCMD;
	unsigned char auiBuff[6];

	struct spi_message msg;
	struct spi_transfer transfer[2];
	struct spi_device *_device;
	int status;

	_device = tdmb_get_spi_handle();

	INC_MUTEX_LOCK();
	auiBuff[0] = uiNewAddr >> 8;
	auiBuff[1] = uiNewAddr & 0xff;
	uiCMD = INC_REGISTER_CTRL(SPI_MEMREAD_CMD) | (wSize & 0xFFF);
	auiBuff[2] = uiCMD >> 8;
	auiBuff[3] = uiCMD & 0xff;
	memset(&msg, 0, sizeof(msg));
	memset(transfer, 0, sizeof(transfer));
	spi_message_init(&msg);
	msg.spi = _device;
	transfer[0].tx_buf = (u8 *) auiBuff;
	transfer[0].rx_buf = (u8 *) NULL;
	transfer[0].len = 4;
	transfer[0].bits_per_word = 8;
	transfer[0].delay_usecs = 0;
	spi_message_add_tail(&(transfer[0]), &msg);
	transfer[1].tx_buf = (u8 *) NULL;
	transfer[1].rx_buf = (u8 *) pBuff;
	transfer[1].len = wSize;
	transfer[1].bits_per_word = 8;
	transfer[1].delay_usecs = 0;
	spi_message_add_tail(&(transfer[1]), &msg);
	status = spi_sync(_device, &msg);
	INC_MUTEX_FREE();
	return INC_SUCCESS;
}

unsigned char INTERFACE_DBINIT(void)
{
	memset(&g_stDmbInfo, 0, sizeof(struct ST_SUBCH_INFO));
	memset(&g_stDabInfo, 0, sizeof(struct ST_SUBCH_INFO));
	memset(&g_stDataInfo, 0, sizeof(struct ST_SUBCH_INFO));
	memset(&g_stFIDCInfo, 0, sizeof(struct ST_SUBCH_INFO));
	return INC_SUCCESS;
}

void INTERFACE_UPLOAD_MODE(unsigned char ucI2CID,
			enum UPLOAD_MODE_INFO ucUploadMode)
{
	struct INC_TDMB_MODE *stMode;
	stMode = INC_GET_TDMBMODE(ucI2CID);
	stMode->m_ucUploadMode = ucUploadMode;
	INC_SET_TDMBMODE(ucI2CID, stMode);

	INC_UPLOAD_MODE(ucI2CID);
}

unsigned char INTERFACE_PLL_MODE(unsigned char ucI2CID, enum PLL_MODE ucPllMode)
{
	struct INC_TDMB_MODE *stMode;
	stMode = INC_GET_TDMBMODE(ucI2CID);
	stMode->m_ucPLL_Mode = ucPllMode;
	INC_SET_TDMBMODE(ucI2CID, stMode);
	return INC_PLL_SET(ucI2CID);
}

unsigned char INTERFACE_INIT(unsigned char ucI2CID)
{
	return INC_INIT(ucI2CID);
}

enum INC_ERROR_INFO INTERFACE_ERROR_STATUS(unsigned char ucI2CID)
{
	struct ST_BBPINFO *pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	return pInfo->nBbpStatus;
}

/*************************************************/
/* when Single Channel Select....                */
/* pChInfo->ucServiceType, pChInfo->ucSubChID,   */
/* pChInfo->ulRFFreq                             */
/* pChInfo->nSetCnt must be defined				 */
/* When DMB Channel Select                       */
/* pChInfo->ucServiceType = 0x18                 */
/* When DAB, DATA Channel Select muse be         */
/* pChInfo->ucServiceType = 0                    */
/*************************************************/

unsigned char INTERFACE_START(unsigned char ucI2CID,
		struct ST_SUBCH_INFO *pChInfo)
{
	return INC_CHANNEL_START(ucI2CID, pChInfo);
}


/* For Factory */
unsigned char INTERFACE_START_TEST(
	unsigned char ucI2CID, struct ST_SUBCH_INFO *pChInfo)
{
	return INC_CHANNEL_START_TEST(ucI2CID, pChInfo);
}

/************************************************/
/* Multi Channel Select	..                      */
/* pChInfo->ucServiceType, pChInfo->ucSubChID,  */
/*pChInfo->ulRFFreq is muse be                  */
/* defined                                      */
/* When DMB Channel Select                      */
/* pChInfo->ucServiceType = 0x18                */
/* When DAB, DATA Channel Select  must be       */
/* pChInfo->ucServiceType = 0                   */
/* pMultiInfo->nSetCnt is count of subChannel   */
/************************************************/
unsigned char INTERFACE_SCAN(unsigned char ucI2CID, unsigned int ulFreq)
{
	INTERFACE_DBINIT();
	if (!INC_ENSEMBLE_SCAN(ucI2CID, ulFreq))
		return INC_ERROR;
	INC_DB_UPDATE(ulFreq, &g_stDmbInfo, &g_stDabInfo, &g_stDataInfo,
		 &g_stFIDCInfo);
	INC_BUBBLE_SORT(&g_stDmbInfo, INC_SUB_CHANNEL_ID);
	INC_BUBBLE_SORT(&g_stDabInfo, INC_SUB_CHANNEL_ID);
	INC_BUBBLE_SORT(&g_stDataInfo, INC_SUB_CHANNEL_ID);
	INC_BUBBLE_SORT(&g_stFIDCInfo, INC_SUB_CHANNEL_ID);
	return INC_SUCCESS;
}


/**********************************************/
/* DMB Channel count return when single       */
/* channel scan is finished                   */
/**********************************************/
unsigned short INTERFACE_GETDMB_CNT(void)
{
	return (unsigned short) g_stDmbInfo.nSetCnt;
}


/***********************************************/
/* DAB Channel count return when single channel */
/* scan is finished                             */
/************************************************/
unsigned short INTERFACE_GETDAB_CNT(void)
{
	return (unsigned short) g_stDabInfo.nSetCnt;
}


/************************************************/
/* DATA Channel count return when single        */
/* channel scan is finished                     */
/************************************************/
unsigned short INTERFACE_GETDATA_CNT(void)
{
	return (unsigned short) g_stDataInfo.nSetCnt;
}


/************************************************/
/* Ensemble Label return when single channel    */
/* scan is finished                             */
/************************************************/

unsigned char *INTERFACE_GETENSEMBLE_LABEL(unsigned char ucI2CID)
{
	struct ST_FICDB_LIST *pList;
	pList = INC_GET_FICDB_LIST();
	return pList->aucEnsembleName;
}


/************************************************/
/* Return the  DMB Channel Information          */
/************************************************/
struct INC_CHANNEL_INFO *INTERFACE_GETDB_DMB(short uiPos)
{
	if (uiPos >= MAX_SUBCH_SIZE)
		return INC_NULL;
	if (uiPos >= g_stDmbInfo.nSetCnt)
		return INC_NULL;
	return &g_stDmbInfo.astSubChInfo[uiPos];
}


/************************************************/
/* REturn the DAB Channel Information           */
/************************************************/
struct INC_CHANNEL_INFO *INTERFACE_GETDB_DAB(short uiPos)
{
	if (uiPos >= MAX_SUBCH_SIZE)
		return INC_NULL;
	if (uiPos >= g_stDabInfo.nSetCnt)
		return INC_NULL;
	return &g_stDabInfo.astSubChInfo[uiPos];
}

/************************************************/
/* REturn the  DATA Channel Information         */
/************************************************/
struct INC_CHANNEL_INFO *INTERFACE_GETDB_DATA(short uiPos)
{
	if (uiPos >= MAX_SUBCH_SIZE)
		return INC_NULL;
	if (uiPos >= g_stDataInfo.nSetCnt)
		return INC_NULL;
	return &g_stDataInfo.astSubChInfo[uiPos];
}

/*  Chech the change of FIC info when watching tv  */
unsigned char INTERFACE_RECONFIG(unsigned char ucI2CID)
{
	return INC_FIC_RECONFIGURATION_HW_CHECK(ucI2CID);
}

unsigned char INTERFACE_STATUS_CHECK(unsigned char ucI2CID)
{
	return INC_STATUS_CHECK(ucI2CID);
}

unsigned short INTERFACE_GET_CER(unsigned char ucI2CID)
{
	return INC_GET_CER(ucI2CID);
}

unsigned char INTERFACE_GET_SNR(unsigned char ucI2CID)
{
	return INC_GET_SNR(ucI2CID);
}

unsigned int INTERFACE_GET_POSTBER(unsigned char ucI2CID)
{
	return INC_GET_POSTBER(ucI2CID);
}

unsigned int INTERFACE_GET_PREBER(unsigned char ucI2CID)
{
	return INC_GET_PREBER(ucI2CID);
}

/***************************************************/
/* Be Called when Scan, start channel, force stop. */
/***************************************************/
void INTERFACE_USER_STOP(unsigned char ucI2CID)
{
	struct ST_BBPINFO *pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	pInfo->ucStop = 1;
}

void INTERFACE_USER_STOP_CLEAR(unsigned char ucI2CID)
{
	struct ST_BBPINFO *pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	pInfo->ucStop = 0;
}

/*      Interrupt Enable...    */
void INTERFACE_INT_ENABLE(unsigned char ucI2CID, unsigned short unSet)
{
	INC_INTERRUPT_SET(ucI2CID, unSet);
}

/*      Use when polling mode   */
unsigned char INTERFACE_INT_CHECK(unsigned char ucI2CID)
{
	unsigned short nValue = 0;
	nValue = INC_CMD_READ(ucI2CID, APB_INT_BASE + 0x01);
	if (!(nValue & INC_MPI_INTERRUPT_ENABLE))
		return INC_ERROR;

	return INC_SUCCESS;
}

/* Interrupt Clear    */
void INTERFACE_INT_CLEAR(unsigned char ucI2CID, unsigned short unClr)
{
	INC_INTERRUPT_CLEAR(ucI2CID, unClr);
}

/* Interrupt Service Routine... // SPI Slave Mode or MPI Slave Mode  */
unsigned char INTERFACE_ISR(unsigned char ucI2CID, unsigned char *pBuff)
{
	unsigned short unDataLength;
	unDataLength = INC_CMD_READ(ucI2CID, APB_MPI_BASE + 0x6);
	if (unDataLength < INC_INTERRUPT_SIZE)
		return INC_ERROR;

	INC_CMD_READ_BURST(ucI2CID, APB_STREAM_BASE, pBuff,
			      INC_INTERRUPT_SIZE);
	INTERFACE_INT_CLEAR(ucI2CID, INC_MPI_INTERRUPT_ENABLE);
	return INC_SUCCESS;
}
