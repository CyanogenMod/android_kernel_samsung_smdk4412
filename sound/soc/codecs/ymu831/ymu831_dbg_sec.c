/*
 * YMU831 ASoC codec driver
 *
 * Copyright (c) 2012 Yamaha Corporation
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP

#include "mcdriver.h"
#include "ymu831_priv.h"


#define YMU831_SEC_DEBUG_LEVEL 1


static void mc_asoc_dump_init_info_sec(const void *pvPrm, UINT32 dPrm)
{
	struct MCDRV_INIT_INFO *info = (struct MCDRV_INIT_INFO *)pvPrm;
	UINT8	i;
	char str[128], *p;

	dbg_info("%13s= 0x%02x\n", "bCkSel", info->bCkSel);
	dbg_info("%13s= 0x%02x\n", "bCkInput", info->bCkInput);
	dbg_info("%13s= 0x%02x\n", "bPllModeA", info->bPllModeA);
	dbg_info("%13s= 0x%02x\n", "bPllPrevDivA", info->bPllPrevDivA);
	dbg_info("%13s= 0x%04x\n", "wPllFbDivA", info->wPllFbDivA);
	dbg_info("%13s= 0x%04x\n", "wPllFracA", info->wPllFracA);
	dbg_info("%13s= 0x%02x\n", "bPllFreqA", info->bPllFreqA);
	dbg_info("%13s= 0x%02x\n", "bPllModeB", info->bPllModeB);
	dbg_info("%13s= 0x%02x\n", "bPllPrevDivB", info->bPllPrevDivB);
	dbg_info("%13s= 0x%04x\n", "wPllFbDivB", info->wPllFbDivB);
	dbg_info("%13s= 0x%04x\n", "wPllFracB", info->wPllFracB);
	dbg_info("%13s= 0x%02x\n", "bPllFreqB", info->bPllFreqB);
	dbg_info("%13s= 0x%02x\n", "bHsdetClk", info->bHsdetClk);
	dbg_info("%13s= 0x%02x\n", "bDio1SdoHiz", info->bDio1SdoHiz);
	dbg_info("%13s= 0x%02x\n", "bDio2SdoHiz", info->bDio2SdoHiz);
	dbg_info("%13s= 0x%02x\n", "bDio0ClkHiz", info->bDio0ClkHiz);
	dbg_info("%13s= 0x%02x\n", "bDio1ClkHiz", info->bDio1ClkHiz);
	dbg_info("%13s= 0x%02x\n", "bDio2ClkHiz", info->bDio2ClkHiz);
	dbg_info("%13s= 0x%02x\n", "bDio0PcmHiz", info->bDio0PcmHiz);
	dbg_info("%13s= 0x%02x\n", "bDio1PcmHiz", info->bDio1PcmHiz);
	dbg_info("%13s= 0x%02x\n", "bDio2PcmHiz", info->bDio2PcmHiz);
	dbg_info("%13s= 0x%02x\n", "bPa0Func", info->bPa0Func);
	dbg_info("%13s= 0x%02x\n", "bPa1Func", info->bPa1Func);
	dbg_info("%13s= 0x%02x\n", "bPa2Func", info->bPa2Func);
	dbg_info("%13s= 0x%02x\n", "bPowerMode", info->bPowerMode);
	dbg_info("%13s= 0x%02x\n", "bMbSel1", info->bMbSel1);
	dbg_info("%13s= 0x%02x\n", "bMbSel2", info->bMbSel2);
	dbg_info("%13s= 0x%02x\n", "bMbSel3", info->bMbSel3);
	dbg_info("%13s= 0x%02x\n", "bMbSel4", info->bMbSel4);
	dbg_info("%13s= 0x%02x\n", "bMbsDisch", info->bMbsDisch);
	dbg_info("%13s= 0x%02x\n", "bNonClip", info->bNonClip);
	dbg_info("%13s= 0x%02x\n", "bLineIn1Dif", info->bLineIn1Dif);
	dbg_info("%13s= 0x%02x\n", "bLineOut1Dif", info->bLineOut1Dif);
	dbg_info("%13s= 0x%02x\n", "bLineOut2Dif", info->bLineOut2Dif);
	dbg_info("%13s= 0x%02x\n", "bMic1Sng", info->bMic1Sng);
	dbg_info("%13s= 0x%02x\n", "bMic2Sng", info->bMic2Sng);
	dbg_info("%13s= 0x%02x\n", "bMic3Sng", info->bMic3Sng);
	dbg_info("%13s= 0x%02x\n", "bMic4Sng", info->bMic4Sng);
	dbg_info("%13s= 0x%02x\n", "bZcLineOut1", info->bZcLineOut1);
	dbg_info("%13s= 0x%02x\n", "bZcLineOut2", info->bZcLineOut2);
	dbg_info("%13s= 0x%02x\n", "bZcRc", info->bZcRc);
	dbg_info("%13s= 0x%02x\n", "bZcSp", info->bZcSp);
	dbg_info("%13s= 0x%02x\n", "bZcHp", info->bZcHp);
	dbg_info("%13s= 0x%02x\n", "bSvolLineOut1", info->bSvolLineOut1);
	dbg_info("%13s= 0x%02x\n", "bSvolLineOut2", info->bSvolLineOut2);
	dbg_info("%13s= 0x%02x\n", "bSvolRc", info->bSvolRc);
	dbg_info("%13s= 0x%02x\n", "bSvolSp", info->bSvolSp);
	dbg_info("%13s= 0x%02x\n", "bSvolHp", info->bSvolHp);
	dbg_info("%13s= 0x%02x\n", "bRcHiz", info->bRcHiz);
	dbg_info("%13s= 0x%02x\n", "bSpHiz", info->bSpHiz);
	dbg_info("%13s= 0x%02x\n", "bHpHiz", info->bHpHiz);
	dbg_info("%13s= 0x%02x\n", "bLineOut1Hiz", info->bLineOut1Hiz);
	dbg_info("%13s= 0x%02x\n", "bLineOut2Hiz", info->bLineOut2Hiz);
	dbg_info("%13s= 0x%02x\n", "bCpMod", info->bCpMod);
	dbg_info("%13s= 0x%02x\n", "bRbSel", info->bRbSel);
	dbg_info("%13s= 0x%02x\n", "bPlugSel", info->bPlugSel);
	dbg_info("%13s= 0x%02x\n", "bGndDet", info->bGndDet);

	dbg_info("sWaitTime.dWaitTime=");
	p = str;
	for (i = 0; i < 20; i++) {
		if (i == 10) {
			dbg_info("%s\n", str);
			p = str;
		}
		p += sprintf(p, " %lu", info->sWaitTime.dWaitTime[i]);
	}
	dbg_info("%s\n", str);

	dbg_info("sWaitTime.dPollInterval=");
	p = str;
	for (i = 0; i < 20; i++) {
		if (i == 10) {
			dbg_info("%s\n", str);
			p = str;
		}
		p += sprintf(p, " %lu", info->sWaitTime.dPollInterval[i]);
	}
	dbg_info("%s\n", str);

	dbg_info("sWaitTime.dPollTimeOut=");
	p = str;
	for (i = 0; i < 20; i++) {
		if (i == 10) {
			dbg_info("%s\n", str);
			p = str;
		}
		p += sprintf(p, " %lu", info->sWaitTime.dPollTimeOut[i]);
	}
	dbg_info("%s\n", str);
}

static void mc_asoc_dump_reg_info_sec(const void *pvPrm, UINT32 dPrm)
{
	struct MCDRV_REG_INFO *info = (struct MCDRV_REG_INFO *)pvPrm;

	dbg_info("bRegType = 0x%02x\n", info->bRegType);
	dbg_info("bAddress = 0x%02x\n", info->bAddress);
	dbg_info("bData    = 0x%02x\n", info->bData);
}

#define DEF_PATH(p) {offsetof(struct MCDRV_PATH_INFO, p), #p}

static void mc_asoc_dump_path_info_sec(const void *pvPrm, UINT32 dPrm)
{
	struct MCDRV_PATH_INFO *info = (struct MCDRV_PATH_INFO *)pvPrm;
	int i;
	UINT32	mask	= (dPrm == 0) ? 0xFFFFFF : dPrm;
	size_t	offset_hostin;

	struct path_table {
		size_t offset;
		char *name;
	};

	struct path_table table[] = {
		DEF_PATH(asMusicOut[0]), DEF_PATH(asMusicOut[1]),
		DEF_PATH(asExtOut[0]), DEF_PATH(asExtOut[1]),
		DEF_PATH(asHifiOut[0]),
		DEF_PATH(asVboxMixIn[0]), DEF_PATH(asVboxMixIn[1]),
		DEF_PATH(asVboxMixIn[2]), DEF_PATH(asVboxMixIn[3]),
		DEF_PATH(asAe0[0]), DEF_PATH(asAe0[1]),
		DEF_PATH(asAe1[0]), DEF_PATH(asAe1[1]),
		DEF_PATH(asAe2[0]), DEF_PATH(asAe2[1]),
		DEF_PATH(asAe3[0]), DEF_PATH(asAe3[1]),
		DEF_PATH(asDac0[0]), DEF_PATH(asDac0[1]),
		DEF_PATH(asDac1[0]), DEF_PATH(asDac1[1]),
		DEF_PATH(asVoiceOut[0]),
		DEF_PATH(asVboxIoIn[0]),
		DEF_PATH(asVboxHostIn[0]),
		DEF_PATH(asHostOut[0]),
		DEF_PATH(asAdif0[0]), DEF_PATH(asAdif0[1]),
		DEF_PATH(asAdif1[0]), DEF_PATH(asAdif1[1]),
		DEF_PATH(asAdif2[0]), DEF_PATH(asAdif2[1]),
		DEF_PATH(asAdc0[0]), DEF_PATH(asAdc0[1]),
		DEF_PATH(asAdc1[0]),
		DEF_PATH(asSp[0]), DEF_PATH(asSp[1]),
		DEF_PATH(asHp[0]), DEF_PATH(asHp[1]),
		DEF_PATH(asRc[0]),
		DEF_PATH(asLout1[0]), DEF_PATH(asLout1[1]),
		DEF_PATH(asLout2[0]), DEF_PATH(asLout2[1]),
		DEF_PATH(asBias[0]), DEF_PATH(asBias[1]),
		DEF_PATH(asBias[2]), DEF_PATH(asBias[3])
	};

#define N_PATH_TABLE (sizeof(table) / sizeof(struct path_table))

	offset_hostin	= offsetof(struct MCDRV_PATH_INFO, asVboxHostIn);
	for (i = 0; i < N_PATH_TABLE; i++) {
		UINT32 *ch = (UINT32 *)((void *)info + table[i].offset);
		if (*ch == 0x00AAAAAA)
			continue;
		if (*ch == 0x002AAAAA)
			continue;
		if (table[i].offset == offset_hostin)
			dbg_info("%s.dSrcOnOff= 0x%08lX\n",
				table[i].name,
				(*ch) & mask);
		else
			dbg_info("%s.dSrcOnOff\t= 0x%08lX\n",
				table[i].name,
				(*ch) & mask);
	}
}

#define DEF_VOL(v) {offsetof(struct MCDRV_VOL_INFO, v), #v}

static void mc_asoc_dump_vol_info_sec(const void *pvPrm, UINT32 dPrm)
{
	struct MCDRV_VOL_INFO *info = (struct MCDRV_VOL_INFO *)pvPrm;
	int i;

	struct vol_table {
		size_t offset;
		char *name;
	};

	struct vol_table table[] = {
		DEF_VOL(aswD_MusicIn[0]), DEF_VOL(aswD_MusicIn[1]),
		DEF_VOL(aswD_ExtIn[0]), DEF_VOL(aswD_ExtIn[1]),
		DEF_VOL(aswD_VoiceIn[0]), DEF_VOL(aswD_VoiceIn[1]),
		DEF_VOL(aswD_RefIn[0]), DEF_VOL(aswD_RefIn[1]),
		DEF_VOL(aswD_Adif0In[0]), DEF_VOL(aswD_Adif0In[1]),
		DEF_VOL(aswD_Adif1In[0]), DEF_VOL(aswD_Adif1In[1]),
		DEF_VOL(aswD_Adif2In[0]), DEF_VOL(aswD_Adif2In[1]),
		DEF_VOL(aswD_MusicOut[0]), DEF_VOL(aswD_MusicOut[1]),
		DEF_VOL(aswD_ExtOut[0]), DEF_VOL(aswD_ExtOut[1]),
		DEF_VOL(aswD_VoiceOut[0]), DEF_VOL(aswD_VoiceOut[1]),
		DEF_VOL(aswD_RefOut[0]), DEF_VOL(aswD_RefOut[1]),
		DEF_VOL(aswD_Dac0Out[0]), DEF_VOL(aswD_Dac0Out[1]),
		DEF_VOL(aswD_Dac1Out[0]), DEF_VOL(aswD_Dac1Out[1]),
		DEF_VOL(aswD_DpathDa[0]), DEF_VOL(aswD_DpathDa[1]),
		DEF_VOL(aswD_DpathAd[0]), DEF_VOL(aswD_DpathAd[1]),
		DEF_VOL(aswA_LineIn1[0]), DEF_VOL(aswA_LineIn1[1]),
		DEF_VOL(aswA_Mic1[0]),
		DEF_VOL(aswA_Mic2[0]),
		DEF_VOL(aswA_Mic3[0]),
		DEF_VOL(aswA_Mic4[0]),
		DEF_VOL(aswA_Hp[0]), DEF_VOL(aswA_Hp[1]),
		DEF_VOL(aswA_Sp[0]), DEF_VOL(aswA_Sp[1]),
		DEF_VOL(aswA_Rc[0]),
		DEF_VOL(aswA_LineOut1[0]), DEF_VOL(aswA_LineOut1[1]),
		DEF_VOL(aswA_LineOut2[0]), DEF_VOL(aswA_LineOut2[1]),
		DEF_VOL(aswA_HpDet[0])
	};

#define N_VOL_TABLE (sizeof(table) / sizeof(struct vol_table))

	for (i = 0; i < N_VOL_TABLE; i++) {
		SINT16 vol = *(SINT16 *)((void *)info + table[i].offset);
		if ((vol & 0x0001) && (vol > -24575))
			dbg_info("%s = 0x%04x\n",
				table[i].name,
				(vol & 0xfffe));
	}
}

static void mc_asoc_dump_dio_info_sec(const void *pvPrm, UINT32 dPrm)
{
	struct MCDRV_DIO_INFO *info = (struct MCDRV_DIO_INFO *)pvPrm;
	struct MCDRV_DIO_PORT *port;
	UINT32 update;
	int i;

	for (i = 0; i < 4; i++) {
		dbg_info("asPortInfo[%d]:\n", i);
		port = &info->asPortInfo[i];
		update = dPrm >> (i*3);
		if (update & MCDRV_MUSIC_COM_UPDATE_FLAG) {
			dbg_info("sDioCommon.bMasterSlave = 0x%02x\n",
				 port->sDioCommon.bMasterSlave);
			dbg_info("           bAutoFs = 0x%02x\n",
				 port->sDioCommon.bAutoFs);
			dbg_info("           bFs = 0x%02x\n",
				 port->sDioCommon.bFs);
			dbg_info("           bBckFs = 0x%02x\n",
				 port->sDioCommon.bBckFs);
			dbg_info("           bInterface = 0x%02x\n",
				 port->sDioCommon.bInterface);
			dbg_info("           bBckInvert = 0x%02x\n",
				 port->sDioCommon.bBckInvert);
			dbg_info("           bSrcThru = 0x%02x\n",
				 port->sDioCommon.bSrcThru);
			dbg_info("           bPcmHizTim = 0x%02x\n",
				 port->sDioCommon.bPcmHizTim);
			dbg_info("           bPcmFrame = 0x%02x\n",
				 port->sDioCommon.bPcmFrame);
			dbg_info("           bPcmHighPeriod = 0x%02x\n",
				 port->sDioCommon.bPcmHighPeriod);
		}
		if (update & MCDRV_MUSIC_DIR_UPDATE_FLAG) {
			dbg_info(" sDir.sDaFormat.bBitSel = 0x%02x\n",
				 port->sDir.sDaFormat.bBitSel);
			dbg_info("               bMode = 0x%02x\n",
				 port->sDir.sDaFormat.bMode);
			dbg_info("     sPcmFormat.bMono = 0x%02x\n",
				 port->sDir.sPcmFormat.bMono);
			dbg_info("                bOrder = 0x%02x\n",
				 port->sDir.sPcmFormat.bOrder);
			dbg_info("                bLaw = 0x%02x\n",
				 port->sDir.sPcmFormat.bLaw);
			dbg_info("                bBitSel = 0x%02x\n",
				 port->sDir.sPcmFormat.bBitSel);
		}
		if (update & MCDRV_MUSIC_DIT_UPDATE_FLAG) {
			dbg_info(" sDit.bStMode = 0x%02x\n",
				 port->sDit.bStMode);
			dbg_info("     bEdge = 0x%02x\n",
				 port->sDit.bEdge);
			dbg_info("     sDaFormat.bBitSel = 0x%02x\n",
				 port->sDit.sDaFormat.bBitSel);
			dbg_info("               bMode = 0x%02x\n",
				 port->sDit.sDaFormat.bMode);
			dbg_info("     sPcmFormat.bMono = 0x%02x\n",
				 port->sDit.sPcmFormat.bMono);
			dbg_info("                bOrder = 0x%02x\n",
				 port->sDit.sPcmFormat.bOrder);
			dbg_info("                bLaw = 0x%02x\n",
				 port->sDit.sPcmFormat.bLaw);
			dbg_info("                bBitSel = 0x%02x\n",
				 port->sDit.sPcmFormat.bBitSel);
		}
	}
}

static void mc_asoc_dump_dio_path_info_sec(const void *pvPrm, UINT32 dPrm)
{
	struct MCDRV_DIOPATH_INFO *info = (struct MCDRV_DIOPATH_INFO *)pvPrm;

	if (dPrm & MCDRV_PHYS0_UPDATE_FLAG)
		dbg_info("abPhysPort[0] = 0x%02x\n", info->abPhysPort[0]);
	if (dPrm & MCDRV_PHYS1_UPDATE_FLAG)
		dbg_info("abPhysPort[1] = 0x%02x\n", info->abPhysPort[1]);
	if (dPrm & MCDRV_PHYS2_UPDATE_FLAG)
		dbg_info("abPhysPort[2] = 0x%02x\n", info->abPhysPort[2]);
	if (dPrm & MCDRV_PHYS3_UPDATE_FLAG)
		dbg_info("abPhysPort[3] = 0x%02x\n", info->abPhysPort[3]);

	if (dPrm & MCDRV_MUSICNUM_UPDATE_FLAG)
		dbg_info("bMusicCh = 0x%02x\n", info->bMusicCh);

	if (dPrm & MCDRV_DIR0SLOT_UPDATE_FLAG)
		dbg_info("abMusicRSlot[0] = 0x%02x\n", info->abMusicRSlot[0]);
	if (dPrm & MCDRV_DIR1SLOT_UPDATE_FLAG)
		dbg_info("abMusicRSlot[1] = 0x%02x\n", info->abMusicRSlot[1]);
	if (dPrm & MCDRV_DIR2SLOT_UPDATE_FLAG)
		dbg_info("abMusicRSlot[2] = 0x%02x\n", info->abMusicRSlot[2]);

	if (dPrm & MCDRV_DIT0SLOT_UPDATE_FLAG)
		dbg_info("abMusicTSlot[0] = 0x%02x\n", info->abMusicTSlot[0]);
	if (dPrm & MCDRV_DIT1SLOT_UPDATE_FLAG)
		dbg_info("abMusicTSlot[1] = 0x%02x\n", info->abMusicTSlot[1]);
	if (dPrm & MCDRV_DIT2SLOT_UPDATE_FLAG)
		dbg_info("abMusicTSlot[2] = 0x%02x\n", info->abMusicTSlot[2]);
}

static void mc_asoc_dump_swap_info_sec(const void *pvPrm, UINT32 dPrm)
{
	struct MCDRV_SWAP_INFO *info = (struct MCDRV_SWAP_INFO *)pvPrm;

	if (dPrm & MCDRV_SWAP_ADIF0_UPDATE_FLAG)
		dbg_info("bAdif0= 0x%02x\n", info->bAdif0);
	if (dPrm & MCDRV_SWAP_ADIF1_UPDATE_FLAG)
		dbg_info("bAdif1= 0x%02x\n", info->bAdif1);
	if (dPrm & MCDRV_SWAP_ADIF2_UPDATE_FLAG)
		dbg_info("bAdif2= 0x%02x\n", info->bAdif2);
	if (dPrm & MCDRV_SWAP_DAC0_UPDATE_FLAG)
		dbg_info("bDac0= 0x%02x\n", info->bDac0);
	if (dPrm & MCDRV_SWAP_DAC1_UPDATE_FLAG)
		dbg_info("bDac1= 0x%02x\n", info->bDac1);
	if (dPrm & MCDRV_SWAP_MUSICIN0_UPDATE_FLAG)
		dbg_info("bMusicIn0= 0x%02x\n", info->bMusicIn0);
	if (dPrm & MCDRV_SWAP_MUSICIN1_UPDATE_FLAG)
		dbg_info("bMusicIn1= 0x%02x\n", info->bMusicIn1);
	if (dPrm & MCDRV_SWAP_MUSICIN2_UPDATE_FLAG)
		dbg_info("bMusicIn2= 0x%02x\n", info->bMusicIn2);
	if (dPrm & MCDRV_SWAP_EXTIN_UPDATE_FLAG)
		dbg_info("bExtIn= 0x%02x\n", info->bExtIn);
	if (dPrm & MCDRV_SWAP_VOICEIN_UPDATE_FLAG)
		dbg_info("bVoiceIn= 0x%02x\n", info->bVoiceIn);
	if (dPrm & MCDRV_SWAP_MUSICOUT0_UPDATE_FLAG)
		dbg_info("bMusicOut0= 0x%02x\n", info->bMusicOut0);
	if (dPrm & MCDRV_SWAP_MUSICOUT1_UPDATE_FLAG)
		dbg_info("bMusicOut1= 0x%02x\n", info->bMusicOut1);
	if (dPrm & MCDRV_SWAP_MUSICOUT2_UPDATE_FLAG)
		dbg_info("bMusicOut2= 0x%02x\n", info->bMusicOut2);
	if (dPrm & MCDRV_SWAP_EXTOUT_UPDATE_FLAG)
		dbg_info("bExtOut= 0x%02x\n", info->bExtOut);
	if (dPrm & MCDRV_SWAP_VOICEOUT_UPDATE_FLAG)
		dbg_info("bVoiceOut= 0x%02x\n", info->bVoiceOut);
}

static void mc_asoc_dump_hsdet_info_sec(const void *pvPrm, UINT32 dPrm)
{
	struct MCDRV_HSDET_INFO *info = (struct MCDRV_HSDET_INFO *)pvPrm;

	if (dPrm & MCDRV_ENPLUGDET_UPDATE_FLAG)
		dbg_info("bEnPlugDet = 0x%02x\n", info->bEnPlugDet);
	if (dPrm & MCDRV_ENPLUGDETDB_UPDATE_FLAG)
		dbg_info("bEnPlugDetDb = 0x%02x\n", info->bEnPlugDetDb);
	if (dPrm & MCDRV_ENDLYKEYOFF_UPDATE_FLAG)
		dbg_info("bEnDlyKeyOff = 0x%02x\n", info->bEnDlyKeyOff);
	if (dPrm & MCDRV_ENDLYKEYON_UPDATE_FLAG)
		dbg_info("bEnDlyKeyOn = 0x%02x\n", info->bEnDlyKeyOn);
	if (dPrm & MCDRV_ENMICDET_UPDATE_FLAG)
		dbg_info("bEnMicDet = 0x%02x\n", info->bEnMicDet);
	if (dPrm & MCDRV_ENKEYOFF_UPDATE_FLAG)
		dbg_info("bEnKeyOff = 0x%02x\n", info->bEnKeyOff);
	if (dPrm & MCDRV_ENKEYON_UPDATE_FLAG)
		dbg_info("bEnKeyOn = 0x%02x\n", info->bEnKeyOn);
	if (dPrm & MCDRV_HSDETDBNC_UPDATE_FLAG)
		dbg_info("bHsDetDbnc = 0x%02x\n", info->bHsDetDbnc);
	if (dPrm & MCDRV_KEYOFFMTIM_UPDATE_FLAG)
		dbg_info("bKeyOffMtim = 0x%02x\n", info->bKeyOffMtim);
	if (dPrm & MCDRV_KEYONMTIM_UPDATE_FLAG)
		dbg_info("bKeyOnMtim = 0x%02x\n", info->bKeyOnMtim);
	if (dPrm & MCDRV_KEY0OFFDLYTIM_UPDATE_FLAG)
		dbg_info("bKey0OffDlyTim = 0x%02x\n", info->bKey0OffDlyTim);
	if (dPrm & MCDRV_KEY1OFFDLYTIM_UPDATE_FLAG)
		dbg_info("bKey1OffDlyTim = 0x%02x\n", info->bKey1OffDlyTim);
	if (dPrm & MCDRV_KEY2OFFDLYTIM_UPDATE_FLAG)
		dbg_info("bKey2OffDlyTim = 0x%02x\n", info->bKey2OffDlyTim);
	if (dPrm & MCDRV_KEY0ONDLYTIM_UPDATE_FLAG)
		dbg_info("bKey0OnDlyTim = 0x%02x\n", info->bKey0OnDlyTim);
	if (dPrm & MCDRV_KEY1ONDLYTIM_UPDATE_FLAG)
		dbg_info("bKey1OnDlyTim = 0x%02x\n", info->bKey1OnDlyTim);
	if (dPrm & MCDRV_KEY2ONDLYTIM_UPDATE_FLAG)
		dbg_info("bKey2OnDlyTim = 0x%02x\n", info->bKey2OnDlyTim);
	if (dPrm & MCDRV_KEY0ONDLYTIM2_UPDATE_FLAG)
		dbg_info("bKey0OnDlyTim2 = 0x%02x\n", info->bKey0OnDlyTim2);
	if (dPrm & MCDRV_KEY1ONDLYTIM2_UPDATE_FLAG)
		dbg_info("bKey1OnDlyTim2 = 0x%02x\n", info->bKey1OnDlyTim2);
	if (dPrm & MCDRV_KEY2ONDLYTIM2_UPDATE_FLAG)
		dbg_info("bKey2OnDlyTim2 = 0x%02x\n", info->bKey2OnDlyTim2);
	if (dPrm & MCDRV_IRQTYPE_UPDATE_FLAG)
		dbg_info("bIrqType = 0x%02x\n", info->bIrqType);
	if (dPrm & MCDRV_DETINV_UPDATE_FLAG)
		dbg_info("bDetInInv = 0x%02x\n", info->bDetInInv);
	if (dPrm & MCDRV_HSDETMODE_UPDATE_FLAG)
		dbg_info("bHsDetMode = 0x%02x\n", info->bHsDetMode);
	if (dPrm & MCDRV_SPERIOD_UPDATE_FLAG)
		dbg_info("bSperiod = 0x%02x\n", info->bSperiod);
	if (dPrm & MCDRV_LPERIOD_UPDATE_FLAG)
		dbg_info("bLperiod = 0x%02x\n", info->bLperiod);
	if (dPrm & MCDRV_DBNCNUMPLUG_UPDATE_FLAG)
		dbg_info("bDbncNumPlug = 0x%02x\n", info->bDbncNumPlug);
	if (dPrm & MCDRV_DBNCNUMMIC_UPDATE_FLAG)
		dbg_info("bDbncNumMic = 0x%02x\n", info->bDbncNumMic);
	if (dPrm & MCDRV_DBNCNUMKEY_UPDATE_FLAG)
		dbg_info("bDbncNumKey = 0x%02x\n", info->bDbncNumKey);
	if (dPrm & MCDRV_SGNL_UPDATE_FLAG) {
		dbg_info("bSgnlPeriod = 0x%02x\n", info->bSgnlPeriod);
		dbg_info("bSgnlNum = 0x%02x\n", info->bSgnlNum);
		dbg_info("bSgnlPeak = 0x%02x\n", info->bSgnlPeak);
	}
	if (dPrm & MCDRV_IMPSEL_UPDATE_FLAG)
		dbg_info("bImpSel = 0x%02x\n", info->bImpSel);

	if (dPrm & MCDRV_DLYIRQSTOP_UPDATE_FLAG)
		dbg_info("bDlyIrqStop = 0x%02x\n", info->bDlyIrqStop);

	if (dPrm & MCDRV_CBFUNC_UPDATE_FLAG)
		dbg_info("cbfunc = %8p\n", info->cbfunc);
}

struct mc_asoc_dump_func {
	bool level;
	char *name;
	void (*func)(const void *, UINT32);
};

struct mc_asoc_dump_func mc_asoc_dump_func_map_sec[] = {
	{1, "MCDRV_INIT", mc_asoc_dump_init_info_sec},
	{0, "MCDRV_TERM", NULL},
	{1, "MCDRV_READ_REG", mc_asoc_dump_reg_info_sec},
	{1, "MCDRV_WRITE_REG", mc_asoc_dump_reg_info_sec},
	{0, "MCDRV_GET_CLOCKSW", NULL},
	{0, "MCDRV_SET_CLOCKSW", NULL},
	{0, "MCDRV_GET_PATH", NULL},
	{3, "MCDRV_SET_PATH", mc_asoc_dump_path_info_sec},
	{0, "MCDRV_GET_VOLUME", NULL},
	{3, "MCDRV_SET_VOLUME", mc_asoc_dump_vol_info_sec},
	{0, "MCDRV_GET_DIGITALIO", NULL},
	{2, "MCDRV_SET_DIGITALIO", mc_asoc_dump_dio_info_sec},
	{0, "MCDRV_GET_DIGITALIO_PATH", NULL},
	{2, "MCDRV_SET_DIGITALIO_PATH", mc_asoc_dump_dio_path_info_sec},
	{0, "MCDRV_GET_SWAP", NULL},
	{1, "MCDRV_SET_SWAP", mc_asoc_dump_swap_info_sec},
	{0, "MCDRV_SET_DSP", NULL},
	{0, "MCDRV_GET_DSP", NULL},
	{0, "MCDRV_GET_DSP_DATA", NULL},
	{0, "MCDRV_SET_DSP_DATA", NULL},
	{0, "MCDRV_REGISTER_DSP_CB", NULL},
	{0, "MCDRV_GET_DSP_TRANSITION", NULL},
	{0, "MCDRV_IRQ", NULL},
	{0, "MCDRV_GET_HSDET", NULL},
	{3, "MCDRV_SET_HSDET", mc_asoc_dump_hsdet_info_sec},
	{0, "MCDRV_CONFIG_GP", NULL},
	{0, "MCDRV_MASK_GP", NULL},
	{0, "MCDRV_GETSET_GP", NULL},
};

SINT32 McDrv_Ctrl_dbg_sec(UINT32 dCmd, void *pvPrm1, void *pvPrm2, UINT32 dPrm)
{
	int err;

	if (mc_asoc_dump_func_map_sec[dCmd].level >= YMU831_SEC_DEBUG_LEVEL) {

		dbg_info("%s\n", mc_asoc_dump_func_map_sec[dCmd].name);

		if (mc_asoc_dump_func_map_sec[dCmd].func)
			mc_asoc_dump_func_map_sec[dCmd].func(pvPrm1, dPrm);
	}

	err = (int) McDrv_Ctrl(dCmd, pvPrm1, pvPrm2, dPrm);
	if (err)
		dbg_info("%s (err=%d)\n",
				mc_asoc_dump_func_map_sec[dCmd].name, err);

	return err;
}

#endif
