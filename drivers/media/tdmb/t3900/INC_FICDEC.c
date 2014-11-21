/*****************************************************************************
 Copyright(c) 2011 I&C Inc. All Rights Reserved

 File name : INC_FICDEC.c

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


struct ST_FICDB_LIST g_stFicDbList;
unsigned char INC_GET_BYTEDATA(struct ST_FIB_INFO *pFibInfo)
{
	return pFibInfo->aucBuff[pFibInfo->ucDataPos++] & 0xff;
}

unsigned char INC_GETAT_BYTEDATA(struct ST_FIB_INFO *pFibInfo)
{
	return pFibInfo->aucBuff[pFibInfo->ucDataPos] & 0xff;
}

unsigned short INC_GET_WORDDATA(struct ST_FIB_INFO *pFibInfo)
{
	unsigned short uiData, result;
	uiData =
	(((unsigned short) pFibInfo->aucBuff[pFibInfo->ucDataPos++] << 8) &
	     0xff00);
	uiData |=
	((unsigned short) pFibInfo->aucBuff[pFibInfo->ucDataPos++] & 0xff);
	result = (uiData & 0xffff);
	return result;
}

unsigned short INC_GETAT_WORDDATA(struct ST_FIB_INFO *pFibInfo)
{
	unsigned short uiData, result;
	uiData =
		(((unsigned short) pFibInfo->aucBuff[pFibInfo->ucDataPos] << 8)
		& 0xff00) |
		((unsigned short) pFibInfo->aucBuff[pFibInfo->ucDataPos + 1]
		& 0xff);
	result = (uiData & 0xffff);
	return result;
}

unsigned int INC_GET_LONGDATA(struct ST_FIB_INFO *pFibInfo)
{
	unsigned int ulMsb, ulLsb , result;
	ulMsb = (unsigned int) INC_GET_WORDDATA(pFibInfo);
	ulLsb = (unsigned int) INC_GET_WORDDATA(pFibInfo);
	result = (ulMsb << 16 | ulLsb);
	return result;
}

unsigned int INC_GETAT_LONGDATA(struct ST_FIB_INFO *pFibInfo)
{
	unsigned int ulMsb, ulLsb, result;
	ulMsb = (unsigned int) INC_GETAT_WORDDATA(pFibInfo);
	pFibInfo->ucDataPos += 2;
	ulLsb = (unsigned int) INC_GETAT_WORDDATA(pFibInfo);
	pFibInfo->ucDataPos -= 2;
	result = (ulMsb << 16 | ulLsb);
	return result;
}

unsigned char INC_GETAT_HEADER(struct ST_FIB_INFO *pInfo)
{
	return pInfo->aucBuff[pInfo->ucDataPos];
}

unsigned char INC_GETAT_TYPE(struct ST_FIB_INFO *pInfo)
{
	return pInfo->aucBuff[pInfo->ucDataPos + 1];
}

unsigned char INC_GET_NULL_BLOCK(union ST_FIG_HEAD *pInfo)
{
	if (pInfo->ucInfo == END_MARKER)
		return INC_ERROR;
	return INC_SUCCESS;
}

unsigned char INC_GET_FIND_LENGTH(union ST_FIG_HEAD *pInfo)
{
	if (!pInfo->ITEM.bitLength || pInfo->ITEM.bitLength > FIB_SIZE - 3)
		return INC_ERROR;
	return INC_SUCCESS;
}

unsigned short INC_CRC_CHECK(unsigned char *pBuf, unsigned char ucSize)
{
	unsigned char ucLoop;
	unsigned short nCrc = 0xFFFF, result;
	for (ucLoop = 0; ucLoop < ucSize; ucLoop++) {
		nCrc = 0xFFFF & (((nCrc << 8) | (nCrc >> 8)) ^
			      (0xFF & pBuf[ucLoop]));
		nCrc = nCrc ^ ((0xFF & nCrc) >> 4);
		nCrc = 0xFFFF & (nCrc ^ ((((((0xFF & nCrc)) << 8) |
				(((0xFF & nCrc)) >> 8))) << 4) ^
				((0xFF & nCrc)   << 5));
	}
	result = ((unsigned short) 0xFFFF & (nCrc ^ 0xFFFF));
	return result;
}

unsigned short INC_FIND_SUB_CHANNEL_SIZE(unsigned char ucTableIndex)
{
	unsigned short wSubCHSize = 0;
	switch (ucTableIndex) {
	case 0:
		wSubCHSize = 16;
		break;
	case 1:
		wSubCHSize = 21;
		break;
	case 2:
		wSubCHSize = 24;
		break;
	case 3:
		wSubCHSize = 29;
		break;
	case 4:
		wSubCHSize = 35;
		break;
	case 5:
		wSubCHSize = 24;
		break;
	case 6:
		wSubCHSize = 29;
		break;
	case 7:
		wSubCHSize = 35;
		break;
	case 8:
		wSubCHSize = 42;
		break;
	case 9:
		wSubCHSize = 52;
		break;
	case 10:
		wSubCHSize = 29;
		break;
	case 11:
		wSubCHSize = 35;
		break;
	case 12:
		wSubCHSize = 42;
		break;
	case 13:
		wSubCHSize = 52;
		break;
	case 14:
		wSubCHSize = 32;
		break;
	case 15:
		wSubCHSize = 42;
		break;
	case 16:
		wSubCHSize = 48;
		break;
	case 17:
		wSubCHSize = 58;
		break;
	case 18:
		wSubCHSize = 70;
		break;
	case 19:
		wSubCHSize = 40;
		break;
	case 20:
		wSubCHSize = 52;
		break;
	case 21:
		wSubCHSize = 58;
		break;
	case 22:
		wSubCHSize = 70;
		break;
	case 23:
		wSubCHSize = 84;
		break;
	case 24:
		wSubCHSize = 48;
		break;
	case 25:
		wSubCHSize = 58;
		break;
	case 26:
		wSubCHSize = 70;
		break;
	case 27:
		wSubCHSize = 84;
		break;
	case 28:
		wSubCHSize = 104;
		break;
	case 29:
		wSubCHSize = 58;
		break;
	case 30:
		wSubCHSize = 70;
		break;
	case 31:
		wSubCHSize = 84;
		break;
	case 32:
		wSubCHSize = 104;
		break;
	case 33:
		wSubCHSize = 64;
		break;
	case 34:
		wSubCHSize = 84;
		break;
	case 35:
		wSubCHSize = 96;
		break;
	case 36:
		wSubCHSize = 116;
		break;
	case 37:
		wSubCHSize = 140;
		break;
	case 38:
		wSubCHSize = 80;
		break;
	case 39:
		wSubCHSize = 104;
		break;
	case 40:
		wSubCHSize = 116;
		break;
	case 41:
		wSubCHSize = 140;
		break;
	case 42:
		wSubCHSize = 168;
		break;
	case 43:
		wSubCHSize = 96;
		break;
	case 44:
		wSubCHSize = 116;
		break;
	case 45:
		wSubCHSize = 140;
		break;
	case 46:
		wSubCHSize = 168;
		break;
	case 47:
		wSubCHSize = 208;
		break;
	case 48:
		wSubCHSize = 116;
		break;
	case 49:
		wSubCHSize = 140;
		break;
	case 50:
		wSubCHSize = 168;
		break;
	case 51:
		wSubCHSize = 208;
		break;
	case 52:
		wSubCHSize = 232;
		break;
	case 53:
		wSubCHSize = 128;
		break;
	case 54:
		wSubCHSize = 168;
		break;
	case 55:
		wSubCHSize = 192;
		break;
	case 56:
		wSubCHSize = 232;
		break;
	case 57:
		wSubCHSize = 280;
		break;
	case 58:
		wSubCHSize = 160;
		break;
	case 59:
		wSubCHSize = 208;
		break;
	case 60:
		wSubCHSize = 280;
		break;
	case 61:
		wSubCHSize = 192;
		break;
	case 62:
		wSubCHSize = 280;
		break;
	case 63:
		wSubCHSize = 416;
		break;
	}
	return wSubCHSize;
}

void INC_EXTENSION_000(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pstHeader;
	union ST_TYPE_0 *pstType;
	union ST_TYPE0of0_INFO *pBitStarm;
	unsigned int ulBitStram = 0;
	pstHeader =
		(union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pstType = (union ST_TYPE_0 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	ulBitStram = INC_GET_LONGDATA(pFibInfo);
	pBitStarm = (union ST_TYPE0of0_INFO *) &ulBitStram;
	INC_ADD_ENSEMBLE_ID(pBitStarm);
	if (pBitStarm->ITEM.bitChangFlag)
		pFibInfo->ucDataPos += 1;
}

void INC_EXTENSION_001(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pstHeader;
	union ST_TYPE_0 *pstType;
	union ST_TYPE0of1Short_INFO *pShortInfo = INC_NULL;
	union ST_TYPE0of1Long_INFO *pLongInfo = INC_NULL;
	struct ST_FICDB_LIST *pList;
	unsigned int ulTypeInfo;
	unsigned short uiStartAddr;
	unsigned char ucSubChId, ucIndex, ucShortLongFlag;
	pList = INC_GET_FICDB_LIST();
	pstHeader =
	(union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pstType = (union ST_TYPE_0 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	for (ucIndex = 0; ucIndex < pstHeader->ITEM.bitLength - 1;) {
		ucShortLongFlag =
		(pFibInfo->aucBuff[pFibInfo->ucDataPos + 2]
		>> 7) & 0x01;
		if (ucShortLongFlag == 0) {
			ulTypeInfo =
			INC_GET_LONGDATA(pFibInfo);
			pShortInfo =
			(union ST_TYPE0of1Short_INFO *) &ulTypeInfo;
			pFibInfo->ucDataPos -= 1;
			ucSubChId = pShortInfo->ITEM.bitSubChId;
			uiStartAddr = pShortInfo->ITEM.bitStartAddr;
		}

		else {
			ulTypeInfo = INC_GET_LONGDATA(pFibInfo);
			pLongInfo = (union ST_TYPE0of1Long_INFO *) &ulTypeInfo;
			ucSubChId = pLongInfo->ITEM.bitSubChId;
			uiStartAddr = pLongInfo->ITEM.bitStartAddr;
		}
		if (pList->ucIsSetSimple == SIMPLE_FIC_ENABLE)
			INC_FIND_SIMPLE_ORGANIZAION_SUBCHANNEL_ID(ucSubChId,
			uiStartAddr,   ulTypeInfo);
		else
			INC_FIND_ORGANIZAION_SUBCHANNEL_ID(ucSubChId,
			ulTypeInfo);
			ucIndex += (!ucShortLongFlag) ? 3 : 4;
	}
		if (pList->ucIsSimpleCnt)
			pList->ucIsSimpleFIC = 1;
}

void INC_EXTENSION_002(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pstHeader;
	union ST_TYPE_0 *pstType;
	union ST_SERVICE_COMPONENT *pService;
	unsigned int ulServiceId;
	unsigned short uiData;
	unsigned char ucService, ucLoop, ucFrame;
	pstHeader =
	    (union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pstType = (union ST_TYPE_0 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	for (ucFrame = 0; ucFrame < pstHeader->ITEM.bitLength - 1;)	{
		if (pstType->ITEM.bitPD == 0) {
			ulServiceId = (unsigned int) INC_GET_WORDDATA(pFibInfo);
			ucFrame += 2;
		}

	else {
			ulServiceId = INC_GET_LONGDATA(pFibInfo);
			ucFrame += 4;
		}
	ucService = INC_GET_BYTEDATA(pFibInfo);
	ucFrame += 1;
	pService = (union ST_SERVICE_COMPONENT *) &ucService;
	for (ucLoop = 0; ucLoop < pService->ITEM.bitNumComponent;
			ucLoop++, ucFrame += 2){
			uiData = INC_GET_WORDDATA(pFibInfo);
			INC_FIND_BASIC_SERVICE(ulServiceId, uiData);
			}
		}
}

void INC_EXTENSION_003(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pstHeader;
	union ST_TYPE_0 *pstType;
	union ST_TYPE0of3_INFO *pTypeInfo;
	union ST_TYPE0of3Id_INFO *pIdInfo;
	unsigned int uiId, uiTypeInfo;
	unsigned char ucIndex;
	pstHeader =
	    (union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pstType = (union ST_TYPE_0 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	for (ucIndex = 0; ucIndex < pstHeader->ITEM.bitLength - 1;)	{
		uiTypeInfo = INC_GET_WORDDATA(pFibInfo);
		pTypeInfo = (union ST_TYPE0of3_INFO *) &uiTypeInfo;
		uiId = INC_GET_WORDDATA(pFibInfo);
		uiId = (uiId << 16) | (INC_GET_BYTEDATA(pFibInfo) << 8);
		pIdInfo = (union ST_TYPE0of3Id_INFO *) &uiId;
		INC_FIND_PACKET_MODE(pTypeInfo, pIdInfo);
		ucIndex += 5;
		if (pTypeInfo->ITEM.bitCAOrgFlag == 1) {
			ucIndex += 2;
			pFibInfo->ucDataPos += 2;
		}
	}
}

void INC_EXTENSION_008(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pstHeader;
	union ST_TYPE_0 *pstType;
	union ST_MSC_BIT *pstMsc;
	union ST_MSC_SHORT *pstMscShort;
	union ST_MSC_LONG *pstMscLong;
	unsigned short uiMsgBit;
	unsigned int ulSerId = 0;
	unsigned char ucMscInfo, ucIndex, ucLSFlag;
	pstHeader =
	(union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pstType = (union ST_TYPE_0 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	for (ucIndex = 0; ucIndex < pstHeader->ITEM.bitLength - 1;) {
		if (!pstType->ITEM.bitPD) {
			ulSerId = (unsigned int) INC_GET_WORDDATA(pFibInfo);
			ucIndex += 2;
		}

		else {
			ulSerId = INC_GET_LONGDATA(pFibInfo);
			ucIndex += 4;
		}
		ucMscInfo = INC_GET_BYTEDATA(pFibInfo);
		pstMsc = (union ST_MSC_BIT *) &ucMscInfo;
		ucIndex += 1;
		ucLSFlag = (INC_GETAT_BYTEDATA(pFibInfo) >> 7) & 0x01;
		if (ucLSFlag) {
			uiMsgBit = INC_GET_WORDDATA(pFibInfo);
			ucIndex += 2;
			pstMscLong = (union ST_MSC_LONG *) &uiMsgBit;
			INC_FIND_GLOBAL_SERVICE_COMPONENT_LONG(
				ulSerId, pstMscLong, pstMsc);
		} else {
			uiMsgBit = (unsigned short) INC_GET_BYTEDATA(pFibInfo);
			ucIndex += 1;
			pstMscShort = (union ST_MSC_SHORT *) &uiMsgBit;
			INC_FIND_GLOBAL_SERVICE_COMPONENT_SHORT(
			ulSerId, pstMscShort, pstMsc);
		}
		if (pstMsc->ITEM.bitExtFlag) {
			ucIndex += 1;
			pFibInfo->ucDataPos += 1;
		}
	}
}

void INC_EXTENSION_010(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pstHeader;
	union ST_TYPE_0 *pstType;
	union ST_UTC_SHORT_INFO *pstUTC_Short_Info;
	union ST_UTC_LONG_INFO *pstUTC_Long_Info;
	unsigned int ulUtcInfo;
	unsigned short uiUtcLongForm;
	struct ST_FICDB_LIST *pList;
	pList = INC_GET_FICDB_LIST();
	pstHeader =
	(union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pstType = (union ST_TYPE_0 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	ulUtcInfo = INC_GET_LONGDATA(pFibInfo);
	pstUTC_Short_Info = (union ST_UTC_SHORT_INFO *) &ulUtcInfo;
	MJDtoYMD(pstUTC_Short_Info->ITEM.bitMJD, &pList->stDate);
	pList->stDate.ucHour = (pstUTC_Short_Info->ITEM.bitHours + 9) % 24;
	pList->stDate.ucMinutes = pstUTC_Short_Info->ITEM.bitMinutes;
	if (pstUTC_Short_Info->ITEM.bitUTC_Flag) {
		uiUtcLongForm = INC_GET_WORDDATA(pFibInfo);
		pstUTC_Long_Info = (union ST_UTC_LONG_INFO *) &uiUtcLongForm;
		pList->stDate.ucSeconds =
		pstUTC_Long_Info->ITEM.bitSeconds;
		pList->stDate.uiMilliseconds =
		pstUTC_Long_Info->ITEM.bitMilliseconds;
	}
#ifdef INC_FIC_DEBUG_MESSAGE
	INC_MSG_PRINTF(1, "\r\n UTC %.4d-%.2d-%.2d:	%.2d.%.2d.%.2d[%dms]",
	pList->stDate.usYear, pList->stDate.ucMonth,
	pList->stDate.ucDay, pList->stDate.ucHour,
	pList->stDate.ucMinutes, pList->stDate.ucSeconds,
	pList->stDate.uiMilliseconds);
#endif /*  */
}

#ifdef USER_APPLICATION_TYPE
void INC_EXTENSION_013(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pstHeader;
	union ST_TYPE_0 *pstType;
	union ST_USER_APP_IDnNUM *pUserAppIdNum;
	union ST_USER_APPTYPE *pUserAppType;
	union ST_USERAPP_GROUP_INFO stUserApp;
	unsigned int ulSid;
	unsigned char ucUserAppIdNum;
	unsigned char ucFrame, ucIndex, ucDataCnt;
	unsigned short uiUserAppType;
	pstHeader =
	(union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pstType = (union ST_TYPE_0 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	for (ucFrame = 0; ucFrame < pstHeader->ITEM.bitLength - 1;) {
		if (pstType->ITEM.bitPD) {
			ulSid = INC_GET_LONGDATA(pFibInfo);
			ucFrame += 4;
		} else {
			ulSid = INC_GET_WORDDATA(pFibInfo);
			ucFrame += 2;
		 }
		ucUserAppIdNum = INC_GET_BYTEDATA(pFibInfo);
		pUserAppIdNum = (union ST_USER_APP_IDnNUM *) &ucUserAppIdNum;
		ucFrame += 1;
		stUserApp.ucUAppSCId =
		pUserAppIdNum->ITEM.bitSCIdS;
		stUserApp.ucUAppCount =
		pUserAppIdNum->ITEM.bitNomUserApp;

	for (ucIndex = 0; ucIndex < stUserApp.ucUAppCount;
			ucIndex++, ucFrame += 2) {
		uiUserAppType =
		INC_GET_WORDDATA(pFibInfo);
		pUserAppType =
		(union ST_USER_APPTYPE *) &uiUserAppType;
			stUserApp.astUserApp[ucIndex].ucDataLength =
			pUserAppType->ITEM.bitUserDataLength;
			stUserApp.astUserApp[ucIndex].unUserAppType =
			pUserAppType->ITEM.bitUserAppType;
			for (ucDataCnt = 0;	ucDataCnt <
				pUserAppType->ITEM.bitUserDataLength;
				ucDataCnt++) {
				stUserApp.astUserApp[ucIndex].aucData[ucDataCnt]
				= INC_GET_BYTEDATA(pFibInfo);
				ucFrame += 1;
			}
			INC_FIND_USERAPP_TYPE(ulSid, &stUserApp);
		}
	}
}
#endif /*  */

unsigned char INC_ADD_USERAPP_TYPE(unsigned int ulSID,
			struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent,
			short nCnt, struct ST_USERAPP_GROUP_INFO *pstData)
{
	short nLoop;
		for (nLoop = 0; nLoop < nCnt; nLoop++, pSvcComponent++) {
			if (pSvcComponent->ulSid == ulSID
				&& pSvcComponent->ucIsAppData == INC_ERROR) {
				pSvcComponent->stUApp = *pstData;
				pSvcComponent->ucIsAppData = INC_SUCCESS;
#ifdef INC_FIC_DEBUG_MESSAGE
			INC_MSG_PRINTF(1,
				       "\r\n TYPE[0/13]User Application Type file SID:0x%.8X ",
				       ulSID);
#endif /*  */
			return INC_SUCCESS;
		}
	}
	return INC_ERROR;
}

void INC_FIND_USERAPP_TYPE(unsigned int ulSID,
	struct ST_USERAPP_GROUP_INFO *pstData)
{
	struct ST_FICDB_LIST *pList;
	struct ST_STREAM_INFO *pStreamInfo;
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent;
	pList = INC_GET_FICDB_LIST();
	pStreamInfo = &pList->stDMB;
	pSvcComponent = pStreamInfo->astPrimary;
	INC_ADD_USERAPP_TYPE(ulSID, pSvcComponent, pStreamInfo->nPrimaryCnt,
			      pstData);
	pSvcComponent = pStreamInfo->astSecondary;
	INC_ADD_USERAPP_TYPE(ulSID, pSvcComponent, pStreamInfo->nSecondaryCnt,
			      pstData);
	pStreamInfo = &pList->stDATA;
	pSvcComponent = pStreamInfo->astPrimary;
	if (INC_ADD_USERAPP_TYPE
		(ulSID, pSvcComponent, pStreamInfo->nPrimaryCnt,
		pstData) == INC_SUCCESS)
		pList->nUserAppCnt++;
		pSvcComponent = pStreamInfo->astSecondary;
	if (INC_ADD_USERAPP_TYPE
		(ulSID, pSvcComponent, pStreamInfo->nSecondaryCnt,
		pstData) == INC_SUCCESS)
		pList->nUserAppCnt++;
}

void INC_EXTENSION_110(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pHeader;
	union ST_TYPE_1 *pType;
	unsigned char ucLoop;
	unsigned short uiEId;
	unsigned char acBuff[MAX_LABEL_CHAR];
	pHeader =
	(union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pType = (union ST_TYPE_1 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	uiEId = INC_GET_WORDDATA(pFibInfo);
	for (ucLoop = 0; ucLoop < MAX_LABEL_CHAR; ucLoop++)
		acBuff[ucLoop] = INC_GET_BYTEDATA(pFibInfo);
	pFibInfo->ucDataPos += 2;
	INC_ADD_ENSEMBLE_NAME(uiEId, acBuff);
}

void INC_EXTENSION_111(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pHeader;
	union ST_TYPE_1 *pType;
	unsigned short uiSId, ucIndex;
	unsigned char acBuff[MAX_LABEL_CHAR];

	pHeader =
	(union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pType = (union ST_TYPE_1 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	uiSId = INC_GET_WORDDATA(pFibInfo);
	for (ucIndex = 0; ucIndex < MAX_LABEL_CHAR; ucIndex++)
		acBuff[ucIndex] = INC_GET_BYTEDATA(pFibInfo);

	pFibInfo->ucDataPos += 2;
	INC_FIND_DATA_SERVICE_COMPONENT_LABEL(uiSId, acBuff);
}

void INC_EXTENSION_112(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pHeader;
	union ST_TYPE_1 *pType;
	union ST_EXTENSION_TYPE12 *pType12;
	unsigned char ucData;
	pHeader =
	(union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pType = (union ST_TYPE_1 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	ucData = INC_GET_BYTEDATA(pFibInfo);
	pType12 = (union ST_EXTENSION_TYPE12 *) &ucData;
	if (pType12->ITEM.bitCF_flag)
		pFibInfo->ucDataPos += 1;
		pFibInfo->ucDataPos += 19;
	if (pType12->ITEM.bitCountry == 1)
		pFibInfo->ucDataPos += 2;
}

void INC_EXTENSION_113(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pHeader;
	union ST_TYPE_1 *pType;
	pHeader =
	(union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pType = (union ST_TYPE_1 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pFibInfo->ucDataPos += 19;
}

void INC_EXTENSION_114(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pHeader;
	union ST_TYPE_1 *pType;
	union ST_EXTENSION_TYPE14 *pExtenType;
	unsigned int ulSId;
	unsigned char ucData, ucIndex;
	unsigned char acBuff[MAX_LABEL_CHAR];
	pHeader =
	(union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pType = (union ST_TYPE_1 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	ucData = INC_GET_BYTEDATA(pFibInfo);
	pExtenType = (union ST_EXTENSION_TYPE14 *) &ucData;
	if (!pExtenType->ITEM.bitPD)
		ulSId = (unsigned int) INC_GET_WORDDATA(pFibInfo);

	else
		ulSId = INC_GET_LONGDATA(pFibInfo);
	for (ucIndex = 0; ucIndex < MAX_LABEL_CHAR; ucIndex++)
		acBuff[ucIndex] = INC_GET_BYTEDATA(pFibInfo);
	pFibInfo->ucDataPos += 2;
	INC_FIND_SERVICE_COMPONENT_LABEL(ulSId, acBuff);
}

void INC_EXTENSION_115(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pHeader;
	union ST_TYPE_1 *pType;
	unsigned int ulSId;
	unsigned char ucIndex, acBuff[MAX_LABEL_CHAR];
	pHeader =
	(union ST_FIG_HEAD *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	pType = (union ST_TYPE_1 *) &pFibInfo->aucBuff[pFibInfo->ucDataPos++];
	ulSId = INC_GET_LONGDATA(pFibInfo);
	for (ucIndex = 0; ucIndex < MAX_LABEL_CHAR; ucIndex++)
		acBuff[ucIndex] = INC_GET_BYTEDATA(pFibInfo);
	pFibInfo->ucDataPos += 2;
	INC_FIND_DATA_SERVICE_COMPONENT_LABEL(ulSId, acBuff);
}

void INC_SET_UPDATEFIC(struct ST_FIB_INFO *pstDestData,
	unsigned char *pSourData)
{
	unsigned char cuIndex;
	unsigned short wCRC, wCRCData;
	for (cuIndex = 0; cuIndex < FIB_SIZE; cuIndex++)
		pstDestData->aucBuff[cuIndex] = pSourData[cuIndex];
		wCRC =
		INC_CRC_CHECK((unsigned char *) pstDestData->aucBuff,
		FIB_SIZE - 2);
		wCRCData =
		((unsigned short) pstDestData->aucBuff[30] << 8) |
		pstDestData->aucBuff[31];
		pstDestData->ucDataPos = 0;
		pstDestData->uiIsCRC = wCRC == wCRCData;
}

unsigned char INC_GET_FIND_TYPE(union ST_FIG_HEAD *pInfo)
{
	if (pInfo->ITEM.bitType <= FIG_IN_HOUSE)
		return INC_SUCCESS;
	return INC_ERROR;
}


void INC_SET_FICTYPE_1(struct ST_FIB_INFO *pFibInfo)
{
	union ST_TYPE_1 *pExtern;
	union ST_FIG_HEAD *pHeader;
	unsigned char ucType, ucHeader;
	ucHeader = INC_GETAT_HEADER(pFibInfo);
	ucType = INC_GETAT_TYPE(pFibInfo);
	pHeader = (union ST_FIG_HEAD *) &ucHeader;
	pExtern = (union ST_TYPE_1 *) &ucType;
	switch (pExtern->ITEM.bitExtension) {
	case EXTENSION_0:
		INC_EXTENSION_110(pFibInfo);
		break;
	case EXTENSION_1:
		INC_EXTENSION_111(pFibInfo);
		break;
	case EXTENSION_2:
		INC_EXTENSION_112(pFibInfo);
		break;
	case EXTENSION_3:
		INC_EXTENSION_113(pFibInfo);
		break;
	case EXTENSION_4:
		INC_EXTENSION_114(pFibInfo);
		break;
	case EXTENSION_5:
		INC_EXTENSION_115(pFibInfo);
		break;
	default:
		pFibInfo->ucDataPos += (pHeader->ITEM.bitLength + 1);
		break;
	}
}

unsigned short INC_SET_FICTYPE_0(struct ST_FIB_INFO *pFibInfo)
{
	union ST_FIG_HEAD *pHeader;
	union ST_TYPE_0 *pExtern;
	unsigned char ucHeader, ucType;
	ucHeader = INC_GETAT_HEADER(pFibInfo);
	pHeader = (union ST_FIG_HEAD *) &ucHeader;
	ucType = INC_GETAT_TYPE(pFibInfo);
	pExtern = (union ST_TYPE_0 *) &ucType;
	switch (pExtern->ITEM.bitExtension)	{
	case EXTENSION_0:
		INC_EXTENSION_000(pFibInfo);
		break;
	case EXTENSION_1:
		INC_EXTENSION_001(pFibInfo);
		break;
	case EXTENSION_2:
		INC_EXTENSION_002(pFibInfo);
		break;
	case EXTENSION_3:
		INC_EXTENSION_003(pFibInfo);
		break;
	case EXTENSION_10:
		INC_EXTENSION_010(pFibInfo);
		break;

#ifdef USER_APPLICATION_TYPE
	case EXTENSION_13:
		INC_EXTENSION_013(pFibInfo);
		break;

#endif /*  */
	default:
		pFibInfo->ucDataPos += pHeader->ITEM.bitLength + 1;
		break;
	}
	return INC_SUCCESS;
}

unsigned char INC_FICPARSING(unsigned char ucI2CID, unsigned char *pucFicBuff,
			    int uFicLength, enum ST_SIMPLE_FIC bSimpleFIC)
{
	struct ST_FIC stFIC;
	struct ST_FIB_INFO *pstFib;
	union ST_FIG_HEAD *pHeader;
	struct ST_FICDB_LIST *pList;
	unsigned char ucLoop, ucHeader;
	pList = INC_GET_FICDB_LIST();
	stFIC.ucBlockNum = uFicLength / FIB_SIZE;
	pstFib = &stFIC.stBlock;
	pList->ucIsSetSimple = bSimpleFIC;
	for (ucLoop = 0; ucLoop < stFIC.ucBlockNum; ucLoop++) {
			INC_SET_UPDATEFIC(pstFib,
				&pucFicBuff[ucLoop * FIB_SIZE]);
		if (!pstFib->uiIsCRC)
			continue;
		while (pstFib->ucDataPos < FIB_SIZE - 2) {
			ucHeader = INC_GETAT_HEADER(pstFib);
			pHeader = (union ST_FIG_HEAD *) &ucHeader;
		if (!INC_GET_FIND_TYPE(pHeader)
				|| !INC_GET_NULL_BLOCK(pHeader)
				|| !INC_GET_FIND_LENGTH(pHeader))
				break;
			switch (pHeader->ITEM.bitType) {
			case FIG_MCI_SI:
				INC_SET_FICTYPE_0(pstFib);
				break;
			case FIG_LABEL:
				INC_SET_FICTYPE_1(pstFib);
				break;
			case FIG_RESERVED_0:
			case FIG_RESERVED_1:
			case FIG_RESERVED_2:
			case FIG_FICDATA_CHANNEL:
			case FIG_CONDITION_ACCESS:
			case FIG_IN_HOUSE:
			default:
				pstFib->ucDataPos +=
				pHeader->ITEM.bitLength + 1;
				break;
			}
			if (pstFib->ucDataPos == (FIB_SIZE - 1))
				break;
			}
		}
	if (bSimpleFIC == SIMPLE_FIC_ENABLE && pList->ucIsSimpleFIC)
		return INC_SUCCESS;
	if (pList->nLabelCnt &&
		pList->nSubChannelCnt &&
		pList->nEmsembleLabelFlag) {
	#ifdef USER_APPLICATION_TYPE
		if (pList->nLabelCnt == pList->nSubChannelCnt
			&& pList->nPacketCnt == pList->nPacketModeCnt
			&& pList->nUserAppCnt == pList->nPacketCnt
			&& bSimpleFIC == SIMPLE_FIC_DISABLE)
			return INC_SUCCESS;

	#else /*  */
		if (pList->nLabelCnt == pList->nSubChannelCnt
			&& pList->nPacketCnt == pList->nPacketModeCnt
			&& bSimpleFIC == SIMPLE_FIC_DISABLE)
			return INC_SUCCESS;
	#endif /*  */
	}
	return INC_ERROR;
}

void INC_LABEL_FILTER(unsigned char *pData, short nSize)
{
	short nLoop;
	for (nLoop = nSize - 1; nLoop >= 0; nLoop--) {
		if (pData[nLoop] == 0x20)
			pData[nLoop] = INC_NULL;
		else
			break;
	}
}

void INC_ADD_SERVICE_LABEL(struct ST_STREAM_INFO *pStreamInfo,
	unsigned int ulSID, unsigned char *pcLabel)
{
	struct ST_FICDB_LIST *pList;
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent;
	short nLoop;
	pList = INC_GET_FICDB_LIST();
	pSvcComponent = pStreamInfo->astPrimary;
	for (nLoop = 0; nLoop < pStreamInfo->nPrimaryCnt;
		nLoop++, pSvcComponent++) {
		if (pSvcComponent->ulSid == ulSID &&
			!pSvcComponent->ucIsLable) {
			memcpy(pSvcComponent->aucLabels, pcLabel,
			MAX_LABEL_CHAR);
			INC_LABEL_FILTER(pSvcComponent->aucLabels,
				MAX_LABEL_CHAR);
			pSvcComponent->ucIsLable = 1;
			pList->nLabelCnt++;
#ifdef INC_FIC_DEBUG_MESSAGE
			INC_MSG_PRINTF(1,
				"\r\n TYPE[1/11] SID[0x%.8X] Service label[ %s ] ",
				ulSID, pSvcComponent->aucLabels);
#endif /*  */
		}
	}

	pSvcComponent = pStreamInfo->astSecondary;
	for (nLoop = 0; nLoop < pStreamInfo->nSecondaryCnt;
		nLoop++, pSvcComponent++) {
		if (pSvcComponent->ulSid == ulSID &&
			!pSvcComponent->ucIsLable) {
			memcpy(pSvcComponent->aucLabels, pcLabel,
			MAX_LABEL_CHAR);
			INC_LABEL_FILTER(pSvcComponent->aucLabels,
					  MAX_LABEL_CHAR);
			pSvcComponent->ucIsLable = 1;
			pList->nLabelCnt++;

#ifdef INC_FIC_DEBUG_MESSAGE
			INC_MSG_PRINTF(1,
				"\r\n TYPE[1/11] SID[0x%.8X] Service label[ %s ] ",
				ulSID, pSvcComponent->aucLabels);
#endif /*  */
		}
	}
}

void INC_ADD_SERVICE_COMPONENT_LABEL(
	struct ST_STREAM_INFO *pStreamInfo, unsigned int ulSID,
	unsigned char *pcLabel)
{
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent;
	short nLoop;
	pSvcComponent = pStreamInfo->astPrimary;
	for (nLoop = 0; nLoop < pStreamInfo->nPrimaryCnt;
			nLoop++, pSvcComponent++) {
		if (pSvcComponent->ulSid == ulSID &&
			!pSvcComponent->ucIsComponentLabel) {
			memcpy(pSvcComponent->aucComponentLabels, pcLabel,
				MAX_LABEL_CHAR);
			INC_LABEL_FILTER(pSvcComponent->aucComponentLabels,
				MAX_LABEL_CHAR);
			pSvcComponent->ucIsComponentLabel = 1;

#ifdef INC_FIC_DEBUG_MESSAGE
			INC_MSG_PRINTF(1,
				"\r\nSID[0x%.8X] Service Component label[ %s ] ",
					ulSID,
					pSvcComponent->aucComponentLabels);
#endif /*  */
		}
	}
	pSvcComponent = pStreamInfo->astSecondary;
	for (nLoop = 0; nLoop < pStreamInfo->nSecondaryCnt;
			nLoop++, pSvcComponent++) {
		if (pSvcComponent->ulSid == ulSID &&
			!pSvcComponent->ucIsComponentLabel) {
			memcpy(pSvcComponent->aucComponentLabels, pcLabel,
				MAX_LABEL_CHAR);
			INC_LABEL_FILTER(pSvcComponent->aucComponentLabels,
				MAX_LABEL_CHAR);
			pSvcComponent->ucIsComponentLabel = 1;

#ifdef INC_FIC_DEBUG_MESSAGE
			INC_MSG_PRINTF(1,
				       "\r\nSID[0x%.8X] Service Component label[ %s ] ",
				ulSID,
				pSvcComponent->aucComponentLabels);

#endif /*  */
		}
	}
}

void INC_FIND_DATA_SERVICE_COMPONENT_LABEL(unsigned int ulSID,
					      unsigned char *pcLabel)
{
	struct ST_FICDB_LIST *pList;
	struct ST_STREAM_INFO *pStreamInfo;
	pList = INC_GET_FICDB_LIST();
	pStreamInfo = &pList->stDAB;
	INC_ADD_SERVICE_LABEL(pStreamInfo, ulSID, pcLabel);
	pStreamInfo = &pList->stDMB;
	INC_ADD_SERVICE_LABEL(pStreamInfo, ulSID, pcLabel);
	pStreamInfo = &pList->stDATA;
	INC_ADD_SERVICE_LABEL(pStreamInfo, ulSID, pcLabel);
}

void INC_FIND_SERVICE_COMPONENT_LABEL(unsigned int ulSID,
					    unsigned char *pcLabel)
{
	struct ST_FICDB_LIST *pList;
	struct ST_STREAM_INFO *pStreamInfo;
	pList = INC_GET_FICDB_LIST();
	pStreamInfo = &pList->stDAB;
	INC_ADD_SERVICE_COMPONENT_LABEL(pStreamInfo, ulSID, pcLabel);
	pStreamInfo = &pList->stDMB;
	INC_ADD_SERVICE_COMPONENT_LABEL(pStreamInfo, ulSID, pcLabel);

	pStreamInfo = &pList->stDATA;
	INC_ADD_SERVICE_COMPONENT_LABEL(pStreamInfo, ulSID, pcLabel);
}

void INC_ADD_GLOBAL_SERVICE_COMPONENT_SHORT(struct ST_STREAM_INFO *pStreamInfo,
						unsigned int ulSvcID,
						union ST_MSC_SHORT *pstMscShort,
						union ST_MSC_BIT *pstMsc)
{
	short nLoop;
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent;

	if (!pstMscShort->ITEM.bitMscFicFlag) {
		pSvcComponent = pStreamInfo->astPrimary;
		for (nLoop = 0; nLoop < pStreamInfo->nPrimaryCnt;
			nLoop++, pSvcComponent++) {
			if (pSvcComponent->ulSid == ulSvcID)
				pSvcComponent->ucSCidS = pstMsc->ITEM.bitScIds;
		}
		pSvcComponent = pStreamInfo->astSecondary;
		for (nLoop = 0; nLoop < pStreamInfo->nSecondaryCnt;
			nLoop++, pSvcComponent++) {
			if (pSvcComponent->ulSid == ulSvcID)
				pSvcComponent->ucSCidS = pstMsc->ITEM.bitScIds;
		}
	}
}

void INC_FIND_GLOBAL_SERVICE_COMPONENT_SHORT(unsigned int ulSvcID,
						union ST_MSC_SHORT *pstMscShort,
						union ST_MSC_BIT *pstMsc)
{
	struct ST_FICDB_LIST *pList;
	struct ST_STREAM_INFO *pStreamInfo;
	pList = INC_GET_FICDB_LIST();
	pStreamInfo = &pList->stDMB;
	INC_ADD_GLOBAL_SERVICE_COMPONENT_SHORT(pStreamInfo, ulSvcID,
						pstMscShort, pstMsc);
	pStreamInfo = &pList->stDAB;
	INC_ADD_GLOBAL_SERVICE_COMPONENT_SHORT(pStreamInfo, ulSvcID,
						pstMscShort, pstMsc);
	pStreamInfo = &pList->stDATA;
	INC_ADD_GLOBAL_SERVICE_COMPONENT_SHORT(pStreamInfo, ulSvcID,
						pstMscShort, pstMsc);
}

void INC_ADD_GLOBAL_SERVICE_COMPONENT_LONG(struct ST_STREAM_INFO *pStreamInfo,
						 unsigned int ulSvcID,
						 union ST_MSC_LONG *pstMscLong,
						 union ST_MSC_BIT *pstMsc)
{
	short nLoop;
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent;
	pSvcComponent = pStreamInfo->astPrimary;
	for (nLoop = 0; nLoop < pStreamInfo->nPrimaryCnt;
		nLoop++, pSvcComponent++) {
		if (pSvcComponent->ulSid == ulSvcID)
			pSvcComponent->ucSubChid = pstMscLong->ITEM.bitScId;
	}
	pSvcComponent = pStreamInfo->astSecondary;
	for (nLoop = 0; nLoop < pStreamInfo->nSecondaryCnt;
		nLoop++, pSvcComponent++) {
		if (pSvcComponent->ulSid == ulSvcID)
			pSvcComponent->ucSubChid = pstMscLong->ITEM.bitScId;
	}
}

void INC_FIND_GLOBAL_SERVICE_COMPONENT_LONG(unsigned int ulSvcID,
			union ST_MSC_LONG *pstMscLong,
			union ST_MSC_BIT *pstMsc)
{
	struct ST_FICDB_LIST *pList;
	struct ST_STREAM_INFO *pStreamInfo;
	pList = INC_GET_FICDB_LIST();
	pStreamInfo = &pList->stDMB;
	INC_ADD_GLOBAL_SERVICE_COMPONENT_LONG(pStreamInfo, ulSvcID, pstMscLong,
					       pstMsc);
	pStreamInfo = &pList->stDAB;
	INC_ADD_GLOBAL_SERVICE_COMPONENT_LONG(pStreamInfo, ulSvcID, pstMscLong,
					       pstMsc);
	pStreamInfo = &pList->stDATA;
	INC_ADD_GLOBAL_SERVICE_COMPONENT_LONG(pStreamInfo, ulSvcID, pstMscLong,
					       pstMsc);
}

void INC_FIND_PACKET_MODE(union ST_TYPE0of3_INFO *pTypeInfo,
				union ST_TYPE0of3Id_INFO *pIdInfo)
{
	struct ST_FICDB_LIST *pList;
	struct ST_STREAM_INFO *pStreamInfo;
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent;
	short nLoop;
	pList = INC_GET_FICDB_LIST();
	pStreamInfo = &pList->stDATA;
	pSvcComponent = pStreamInfo->astPrimary;
	for (nLoop = 0; nLoop < pStreamInfo->nPrimaryCnt;
		nLoop++, pSvcComponent++) {
		if (pSvcComponent->ucSCidS == pTypeInfo->ITEM.bitScid
			&& !pSvcComponent->IsPacketMode) {
			pSvcComponent->ucDSCType = pIdInfo->ITEM.bitDScType;
			pSvcComponent->ucSubChid = pIdInfo->ITEM.bitSubChId;
			pSvcComponent->unPacketAddr =
			pIdInfo->ITEM.bitPacketAddr;
			pSvcComponent->IsPacketMode = 1;
			pList->nPacketModeCnt++;
		}
	}
	pSvcComponent = pStreamInfo->astSecondary;
	for (nLoop = 0; nLoop < pStreamInfo->nSecondaryCnt;
			nLoop++, pSvcComponent++) {
		if (pSvcComponent->ucSCidS == pTypeInfo->ITEM.bitScid
			&& !pSvcComponent->IsPacketMode) {
			pSvcComponent->ucDSCType = pIdInfo->ITEM.bitDScType;
			pSvcComponent->ucSubChid = pIdInfo->ITEM.bitSubChId;
			pSvcComponent->unPacketAddr =
			pIdInfo->ITEM.bitPacketAddr;
			pSvcComponent->IsPacketMode = 1;
			pList->nPacketModeCnt++;
		}
	}
}

void INC_GET_SHORT_FORM(struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent,
			   union ST_TYPE0of1Short_INFO *pShort)
{
	pSvcComponent->ucShortLong = pShort->ITEM.bitShortLong;
	pSvcComponent->ucTableSW = pShort->ITEM.bitTableSw;
	pSvcComponent->ucTableIndex = pShort->ITEM.bitTableIndex;
	pSvcComponent->ucOption = 0;
	pSvcComponent->ucProtectionLevel = 0;
	pSvcComponent->uiSubChSize =
	INC_FIND_SUB_CHANNEL_SIZE(pSvcComponent->ucTableIndex);
	pSvcComponent->uiDifferentRate = 0;
	pSvcComponent->uiBitRate = INC_GET_BITRATE(pSvcComponent);
}

void INC_GET_LONG_FORM(struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent,
			     union ST_TYPE0of1Long_INFO *pLong)
{
	pSvcComponent->ucShortLong = pLong->ITEM.bitShortLong;
	pSvcComponent->ucTableSW = 0;
	pSvcComponent->ucTableIndex = 0;
	pSvcComponent->ucOption = pLong->ITEM.bitOption;
	pSvcComponent->ucProtectionLevel = pLong->ITEM.bitProtecLevel;
	pSvcComponent->uiSubChSize = pLong->ITEM.bitSubChanSize;

	if (pSvcComponent->ucOption == 0) {
		switch (pSvcComponent->ucProtectionLevel) {
		case 0:
				pSvcComponent->uiDifferentRate =
				(pSvcComponent->uiSubChSize / 12);
		break;
		case 1:
				pSvcComponent->uiDifferentRate =
				(pSvcComponent->uiSubChSize / 8);
		break;
		case 2:
				pSvcComponent->uiDifferentRate =
			    (pSvcComponent->uiSubChSize / 6);
		break;
		case 3:
				pSvcComponent->uiDifferentRate =
				(pSvcComponent->uiSubChSize / 4);
		break;
		default:
				pSvcComponent->uiDifferentRate = 0;
		break;
		}
	}

	else {
		switch (pSvcComponent->ucProtectionLevel) {
		case 0:
				pSvcComponent->uiDifferentRate =
				(pSvcComponent->uiSubChSize / 27);
		break;
		case 1:
				pSvcComponent->uiDifferentRate =
				(pSvcComponent->uiSubChSize / 21);
		break;
		case 2:
				pSvcComponent->uiDifferentRate =
				(pSvcComponent->uiSubChSize / 18);
		break;
		case 3:
				pSvcComponent->uiDifferentRate =
				(pSvcComponent->uiSubChSize / 15);
		break;
		default:
				pSvcComponent->uiDifferentRate = 0;
		break;
		}
	}
	pSvcComponent->uiBitRate = INC_GET_BITRATE(pSvcComponent);
}

unsigned short INC_GET_BITRATE(struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent)
{
	unsigned short uiBitRate = 0;
	if (!pSvcComponent->ucShortLong) {
		if (pSvcComponent->ucTableIndex <= 4)
			uiBitRate = 32;

		else if (pSvcComponent->ucTableIndex >= 5
			&& pSvcComponent->ucTableIndex <= 9)
			uiBitRate = 48;

		else if (pSvcComponent->ucTableIndex >= 10
				 && pSvcComponent->ucTableIndex <= 13)
			uiBitRate = 56;

		else if (pSvcComponent->ucTableIndex >= 14
				&& pSvcComponent->ucTableIndex <= 18)
			uiBitRate = 64;

		else if (pSvcComponent->ucTableIndex >= 19
				&& pSvcComponent->ucTableIndex <= 23)
			uiBitRate = 80;

		else if (pSvcComponent->ucTableIndex >= 24
				&& pSvcComponent->ucTableIndex <= 28)
				uiBitRate = 96;

		else if (pSvcComponent->ucTableIndex >= 29
				&& pSvcComponent->ucTableIndex <= 32)
				uiBitRate = 112;

		else if (pSvcComponent->ucTableIndex >= 33
				&& pSvcComponent->ucTableIndex <= 37)
				uiBitRate = 128;

		else if (pSvcComponent->ucTableIndex >= 38
				&& pSvcComponent->ucTableIndex <= 42)
				uiBitRate = 160;

		else if (pSvcComponent->ucTableIndex >= 43
				&& pSvcComponent->ucTableIndex <= 47)
				uiBitRate = 192;

		else if (pSvcComponent->ucTableIndex >= 48
				&& pSvcComponent->ucTableIndex <= 52)
				uiBitRate = 224;

		else if (pSvcComponent->ucTableIndex >= 53
				&& pSvcComponent->ucTableIndex <= 57)
				uiBitRate = 256;

		else if (pSvcComponent->ucTableIndex >= 58
				&& pSvcComponent->ucTableIndex <= 60)
				uiBitRate = 320;

		else if (pSvcComponent->ucTableIndex >= 61
				&& pSvcComponent->ucTableIndex <= 63)
				uiBitRate = 384;

		else
			uiBitRate = 0;
		} else	{
			if (pSvcComponent->ucOption == OPTION_INDICATE0) {
				switch (pSvcComponent->ucProtectionLevel) {
				case PROTECTION_LEVEL0:
					uiBitRate =
					(pSvcComponent->uiSubChSize / 12) * 8;
				break;
				case PROTECTION_LEVEL1:
					uiBitRate =
					(pSvcComponent->uiSubChSize / 8) * 8;
				break;
				case PROTECTION_LEVEL2:
					uiBitRate =
					(pSvcComponent->uiSubChSize / 6) * 8;
				break;
				case PROTECTION_LEVEL3:
					uiBitRate =
					(pSvcComponent->uiSubChSize / 4) * 8;
				break;
				}
			}

		else if (pSvcComponent->ucOption == OPTION_INDICATE1) {
				switch (pSvcComponent->ucProtectionLevel) {
				case PROTECTION_LEVEL0:
					uiBitRate =
					(pSvcComponent->uiSubChSize / 27) * 32;
				break;
				case PROTECTION_LEVEL1:
					uiBitRate =
					(pSvcComponent->uiSubChSize / 21) * 32;
				break;
				case PROTECTION_LEVEL2:
					uiBitRate =
					(pSvcComponent->uiSubChSize / 18) * 32;
				break;
				case PROTECTION_LEVEL3:
				uiBitRate =
					(pSvcComponent->uiSubChSize / 15) * 32;
				break;
			}
		}
	}
	return uiBitRate;
}

void INC_ADD_ORGANIZAION_SUBCHANNEL_ID(
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent,
	unsigned int ulTypeInfo)
{
	union ST_TYPE0of1Long_INFO *pLongInfo;
	union ST_TYPE0of1Short_INFO *pShortInfo;
	pShortInfo = (union ST_TYPE0of1Short_INFO *) &ulTypeInfo;
	pLongInfo = (union ST_TYPE0of1Long_INFO *) &ulTypeInfo;
	pSvcComponent->unStartAddr = pShortInfo->ITEM.bitStartAddr;
	if (pShortInfo->ITEM.bitShortLong)
		INC_GET_LONG_FORM(pSvcComponent, pLongInfo);
	else
		INC_GET_SHORT_FORM(pSvcComponent, pShortInfo);
}

void INC_SORT_ORGANIZAION_SUBCHANNEL_ID(
	struct ST_STREAM_INFO *pStreamInfo,
	unsigned char ucSubChID,
	unsigned int ulTypeInfo)
{
	short nLoop;
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent;
	pSvcComponent = pStreamInfo->astPrimary;
	for (nLoop = 0; nLoop < pStreamInfo->nPrimaryCnt;
			nLoop++, pSvcComponent++) {
		if (pSvcComponent->ucSubChid == ucSubChID
			&& !pSvcComponent->IsOrganiza) {
			INC_ADD_ORGANIZAION_SUBCHANNEL_ID(
				pSvcComponent, ulTypeInfo);
			pSvcComponent->IsOrganiza = INC_SUCCESS;
#ifdef INC_FIC_DEBUG_MESSAGE
			INC_MSG_PRINTF(1, "\r\n TYPE[0/1] SUB CH ID %x",
				ucSubChID);
#endif /*  */
		}
	}
	pSvcComponent = pStreamInfo->astSecondary;
	for (nLoop = 0; nLoop < pStreamInfo->nSecondaryCnt;
			nLoop++, pSvcComponent++) {
		if (pSvcComponent->ucSubChid == ucSubChID
			&& !pSvcComponent->IsOrganiza) {
			INC_ADD_ORGANIZAION_SUBCHANNEL_ID(
				pSvcComponent, ulTypeInfo);
			pSvcComponent->IsOrganiza = INC_SUCCESS;

#ifdef INC_FIC_DEBUG_MESSAGE
			INC_MSG_PRINTF(1, "\r\n TYPE[0/1] SUB CH ID %x",
				ucSubChID);
#endif /*  */
		}
	}
}

unsigned char INC_SORT_SIMPLE_ORGANIZAION_SUBCHANNEL_ID(
	struct ST_STREAM_INFO *pStreamInfo, unsigned char ucSubChID,
	unsigned short unStartAddr,	unsigned int ulTypeInfo)
{
	short nLoop;
	struct ST_FICDB_LIST *pList;
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent;
	pList = INC_GET_FICDB_LIST();
	pSvcComponent = pStreamInfo->astPrimary;
	for (nLoop = 0; nLoop < pStreamInfo->nPrimaryCnt;
		nLoop++, pSvcComponent++) {
		if (pSvcComponent->ucSubChid == ucSubChID
			&& pSvcComponent->unStartAddr == unStartAddr) {
			return INC_ERROR;
		}
	}
		pSvcComponent =
			&pStreamInfo->astPrimary[pStreamInfo->nPrimaryCnt];
		pSvcComponent->ucSubChid = ucSubChID;
		pSvcComponent->unStartAddr = unStartAddr;
		INC_ADD_ORGANIZAION_SUBCHANNEL_ID(pSvcComponent, ulTypeInfo);
		pStreamInfo->nPrimaryCnt++;
		pList->ucIsSimpleCnt++;
		return INC_SUCCESS;
}

void INC_FIND_SIMPLE_ORGANIZAION_SUBCHANNEL_ID(
	unsigned char ucSubChID,
	unsigned short unStartAddr,
	unsigned int ulTypeInfo)
{
	struct ST_FICDB_LIST *pList;
	struct ST_STREAM_INFO *pStreamInfo;
	pList = INC_GET_FICDB_LIST();
	pStreamInfo = &pList->stDMB;
	INC_SORT_SIMPLE_ORGANIZAION_SUBCHANNEL_ID(
		pStreamInfo, ucSubChID, unStartAddr, ulTypeInfo);
}

void INC_FIND_ORGANIZAION_SUBCHANNEL_ID(
	unsigned char ucSubChID, unsigned int ulTypeInfo)
{
	struct ST_FICDB_LIST *pList;
	struct ST_STREAM_INFO *pStreamInfo;
	pList = INC_GET_FICDB_LIST();
	pStreamInfo = &pList->stDMB;
	INC_SORT_ORGANIZAION_SUBCHANNEL_ID(pStreamInfo, ucSubChID, ulTypeInfo);
	pStreamInfo = &pList->stDAB;
	INC_SORT_ORGANIZAION_SUBCHANNEL_ID(pStreamInfo, ucSubChID, ulTypeInfo);
	pStreamInfo = &pList->stDATA;
	INC_SORT_ORGANIZAION_SUBCHANNEL_ID(pStreamInfo, ucSubChID, ulTypeInfo);
}

void INC_ADD_BASIC_SERVICE(
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent,
	unsigned int ulServiceId, unsigned short unData)
{
	union ST_TMId_MSCnFIDC *pMscnFidc;
	union ST_MSC_PACKET_INFO *pMscPacket;
	pMscnFidc = (union ST_TMId_MSCnFIDC *) &unData;
	pMscPacket = (union ST_MSC_PACKET_INFO *) &unData;
	pSvcComponent->ulSid = ulServiceId;
	pSvcComponent->ucTmID = pMscnFidc->ITEM.bitTMId;
	pSvcComponent->ucCAFlag = pMscnFidc->ITEM.bitCAflag;
	pSvcComponent->ucPS = pMscnFidc->ITEM.bitPS;
	if (pMscnFidc->ITEM.bitTMId == 0x03)
		pSvcComponent->ucSCidS = pMscPacket->ITEM.bitSCId;
	else {
		pSvcComponent->ucSubChid = pMscnFidc->ITEM.bitSubChld;
		pSvcComponent->ucDSCType = pMscnFidc->ITEM.bitAscDscTy;
	}
}

void INC_FIND_BASIC_SERVICE(unsigned int ulServiceId, unsigned short unData)
{
	short nLoop, nDataCnt;
	struct ST_FICDB_LIST *pList;
	struct ST_STREAM_INFO *pStreamInfo;
	struct ST_FICDB_SERVICE_COMPONENT *pSvcComponent;
	union ST_TMId_MSCnFIDC *pMscnFidc;
	union ST_MSC_PACKET_INFO *pMscPacket;
	pList = INC_GET_FICDB_LIST();
	if (pList->ucIsSetSimple == SIMPLE_FIC_ENABLE)
		return;
	pMscnFidc = (union ST_TMId_MSCnFIDC *) &unData;
	pMscPacket = (union ST_MSC_PACKET_INFO *) &unData;
	if (pMscnFidc->ITEM.bitTMId == 0x00)
		pStreamInfo = &pList->stDAB;

	else if (pMscnFidc->ITEM.bitTMId == 0x01)
		pStreamInfo = &pList->stDMB;

	else if (pMscnFidc->ITEM.bitTMId == 0x02)
		pStreamInfo = &pList->stFIDC;

	else
	pStreamInfo = &pList->stDATA;
	pSvcComponent = (pMscnFidc->ITEM.bitPS) ? pStreamInfo->astPrimary :
					pStreamInfo->astSecondary;
	nDataCnt = (pMscnFidc->ITEM.bitPS) ? pStreamInfo->nPrimaryCnt :
					pStreamInfo->nSecondaryCnt;

	for (nLoop = 0; nLoop < nDataCnt; nLoop++, pSvcComponent++) {
		if (pSvcComponent->ulSid == ulServiceId)
			return;
	}

	if (pMscnFidc->ITEM.bitTMId == 0x03)
		pList->nPacketCnt++;
	if (pMscnFidc->ITEM.bitTMId != 0x02) {
		pList->nSubChannelCnt++;
		INC_ADD_BASIC_SERVICE(pSvcComponent, ulServiceId, unData);
			if (pMscnFidc->ITEM.bitPS)
				pStreamInfo->nPrimaryCnt++;
			else
				pStreamInfo->nSecondaryCnt++;
		}
}

void INC_ADD_ENSEMBLE_ID(union ST_TYPE0of0_INFO *pIdInfo)
{
	struct ST_FICDB_LIST *pList;
	pList = INC_GET_FICDB_LIST();
	pList->unEnsembleID = pIdInfo->ITEM.bitEld;
	pList->ucChangeFlag = pIdInfo->ITEM.bitChangFlag;
}

unsigned char INC_ADD_ENSEMBLE_NAME(unsigned short unEnID,
	unsigned char *pcLabel)
{
	struct ST_FICDB_LIST *pList;
	pList = INC_GET_FICDB_LIST();
	if (pList->ucIsEnsembleName == INC_SUCCESS)
		return INC_ERROR;
	pList->unEnsembleID = unEnID;
	pList->nEmsembleLabelFlag = 1;
	memcpy(pList->aucEnsembleName, pcLabel, MAX_LABEL_CHAR);
	INC_LABEL_FILTER(pList->aucEnsembleName, MAX_LABEL_CHAR);
	pList->ucIsEnsembleName = INC_SUCCESS;
	return INC_SUCCESS;
}

struct ST_FICDB_LIST *INC_GET_FICDB_LIST()
{
	return  &g_stFicDbList;
}

void INC_INITDB(unsigned char ucI2CID)
{
	struct ST_FICDB_LIST *pList;
	pList = INC_GET_FICDB_LIST();
	if (pList == 0)
		return;
	memset(pList, 0, sizeof(struct ST_FICDB_LIST));
}
