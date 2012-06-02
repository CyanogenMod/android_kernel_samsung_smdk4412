#include <sound/soc.h>
#include "mc1n2_priv.h"

#ifdef CONFIG_SND_SOC_MC1N2_DEBUG

static void mc1n2_dump_reg_info(const void *pvPrm, UINT32 dPrm)
{
	MCDRV_REG_INFO *info = (MCDRV_REG_INFO *)pvPrm;

	dbg_info("bRegType = 0x%02x\n", info->bRegType);
	dbg_info("bAddress = 0x%02x\n", info->bAddress);
}

static void mc1n2_dump_array(const char *name,
			     const unsigned char *data, size_t len)
{
	char str[2048], *p;
	int n = (len <= 256) ? len : 256;
	int i;

	p = str;
	for (i = 0; i < n; i++) {
		p += sprintf(p, "0x%02x ", data[i]);
	}

	dbg_info("%s[] = {%s}\n", name, str);
}

#define DEF_PATH(p) {offsetof(MCDRV_PATH_INFO, p), #p}

static void mc1n2_dump_path_info(const void *pvPrm, UINT32 dPrm)
{
	MCDRV_PATH_INFO *info = (MCDRV_PATH_INFO *)pvPrm;
	int i;

	struct path_table {
		size_t offset;
		char *name;
	};

	struct path_table table[] = {
		DEF_PATH(asHpOut[0]), DEF_PATH(asHpOut[1]),
		DEF_PATH(asSpOut[0]), DEF_PATH(asSpOut[1]),
		DEF_PATH(asRcOut[0]),
		DEF_PATH(asLout1[0]), DEF_PATH(asLout1[1]),
		DEF_PATH(asLout2[0]), DEF_PATH(asLout2[1]),
		DEF_PATH(asPeak[0]),
		DEF_PATH(asDit0[0]),
		DEF_PATH(asDit1[0]),
		DEF_PATH(asDit2[0]),
		DEF_PATH(asDac[0]), DEF_PATH(asDac[1]),
		DEF_PATH(asAe[0]),
		DEF_PATH(asCdsp[0]), DEF_PATH(asCdsp[1]),
		DEF_PATH(asCdsp[2]), DEF_PATH(asCdsp[3]),
		DEF_PATH(asAdc0[0]), DEF_PATH(asAdc0[1]),
		DEF_PATH(asAdc1[0]),
		DEF_PATH(asMix[0]),
		DEF_PATH(asBias[0]),
	};

#define N_PATH_TABLE (sizeof(table) / sizeof(struct path_table))

	for (i = 0; i < N_PATH_TABLE; i++) {
		MCDRV_CHANNEL *ch = (MCDRV_CHANNEL *)((void *)info + table[i].offset);
		int j;
		for (j = 0; j < SOURCE_BLOCK_NUM; j++) {
			if (ch->abSrcOnOff[j] != 0) {
				dbg_info("%s.abSrcOnOff[%d] = 0x%02x\n",
					 table[i].name, j, ch->abSrcOnOff[j]);
			}
		}
	}
}

#define DEF_VOL(v) {offsetof(MCDRV_VOL_INFO, v), #v}

static void mc1n2_dump_vol_info(const void *pvPrm, UINT32 dPrm)
{
	MCDRV_VOL_INFO *info = (MCDRV_VOL_INFO *)pvPrm;
	int i;

	struct vol_table {
		size_t offset;
		char *name;
	};

	struct vol_table table[] = {
		DEF_VOL(aswD_Ad0[0]), DEF_VOL(aswD_Ad0[1]),
		DEF_VOL(aswD_Ad1[0]),
		DEF_VOL(aswD_Aeng6[0]), DEF_VOL(aswD_Aeng6[1]),
		DEF_VOL(aswD_Pdm[0]), DEF_VOL(aswD_Pdm[1]),
		DEF_VOL(aswD_Dtmfb[0]), DEF_VOL(aswD_Dtmfb[1]),
		DEF_VOL(aswD_Dir0[0]), DEF_VOL(aswD_Dir0[1]),
		DEF_VOL(aswD_Dir1[0]), DEF_VOL(aswD_Dir1[1]),
		DEF_VOL(aswD_Dir2[0]), DEF_VOL(aswD_Dir2[1]),
		DEF_VOL(aswD_Ad0Att[0]), DEF_VOL(aswD_Ad0Att[1]),
		DEF_VOL(aswD_Ad1Att[0]),
		DEF_VOL(aswD_Dir0Att[0]), DEF_VOL(aswD_Dir0Att[1]),
		DEF_VOL(aswD_Dir1Att[0]), DEF_VOL(aswD_Dir1Att[1]),
		DEF_VOL(aswD_Dir2Att[0]), DEF_VOL(aswD_Dir2Att[1]),
		DEF_VOL(aswD_SideTone[0]), DEF_VOL(aswD_SideTone[1]),
		DEF_VOL(aswD_DtmfAtt[0]), DEF_VOL(aswD_DtmfAtt[1]),
		DEF_VOL(aswD_DacMaster[0]), DEF_VOL(aswD_DacMaster[1]),
		DEF_VOL(aswD_DacVoice[0]), DEF_VOL(aswD_DacVoice[1]),
		DEF_VOL(aswD_DacAtt[0]), DEF_VOL(aswD_DacAtt[1]),
		DEF_VOL(aswD_Dit0[0]), DEF_VOL(aswD_Dit0[1]),
		DEF_VOL(aswD_Dit1[0]), DEF_VOL(aswD_Dit1[1]),
		DEF_VOL(aswD_Dit2[0]), DEF_VOL(aswD_Dit2[1]),
		DEF_VOL(aswA_Ad0[0]), DEF_VOL(aswA_Ad0[1]),
		DEF_VOL(aswA_Ad1[0]),
		DEF_VOL(aswA_Lin1[0]), DEF_VOL(aswA_Lin1[1]),
		DEF_VOL(aswA_Lin2[0]), DEF_VOL(aswA_Lin2[1]),
		DEF_VOL(aswA_Mic1[0]),
		DEF_VOL(aswA_Mic2[0]),
		DEF_VOL(aswA_Mic3[0]),
		DEF_VOL(aswA_Hp[0]), DEF_VOL(aswA_Hp[1]),
		DEF_VOL(aswA_Sp[0]), DEF_VOL(aswA_Sp[1]),
		DEF_VOL(aswA_Rc[0]),
		DEF_VOL(aswA_Lout1[0]), DEF_VOL(aswA_Lout1[1]),
		DEF_VOL(aswA_Lout2[0]), DEF_VOL(aswA_Lout2[1]),
		DEF_VOL(aswA_Mic1Gain[0]),
		DEF_VOL(aswA_Mic2Gain[0]),
		DEF_VOL(aswA_Mic3Gain[0]),
		DEF_VOL(aswA_HpGain[0]),
	};

#define N_VOL_TABLE (sizeof(table) / sizeof(struct vol_table))

	for (i = 0; i < N_VOL_TABLE; i++) {
		SINT16 vol = *(SINT16 *)((void *)info + table[i].offset);
		if (vol & 0x0001) {
			dbg_info("%s = 0x%04x\n", table[i].name, (vol & 0xfffe));
		}
	}
}

static void mc1n2_dump_dio_info(const void *pvPrm, UINT32 dPrm)
{
	MCDRV_DIO_INFO *info = (MCDRV_DIO_INFO *)pvPrm;
	MCDRV_DIO_PORT *port;
	UINT32 update;
	int i;

	for (i = 0; i < IOPORT_NUM; i++) {
		dbg_info("asPortInfo[%d]:\n", i);
		port = &info->asPortInfo[i];
		update = dPrm >> (i*3);
		if (update & MCDRV_DIO0_COM_UPDATE_FLAG) {
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
			dbg_info("           bPcmHizTim = 0x%02x\n",
				 port->sDioCommon.bPcmHizTim);
			dbg_info("           bPcmClkDown = 0x%02x\n",
				 port->sDioCommon.bPcmClkDown);
			dbg_info("           bPcmFrame = 0x%02x\n",
				 port->sDioCommon.bPcmFrame);
			dbg_info("           bPcmHighPeriod = 0x%02x\n",
				 port->sDioCommon.bPcmHighPeriod);
		}
		if (update & MCDRV_DIO0_DIR_UPDATE_FLAG) {
			dbg_info("sDir.wSrcRate = 0x%04x\n",
				 port->sDir.wSrcRate);
			dbg_info("     sDaFormat.bBitSel = 0x%02x\n",
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
			dbg_info("     abSlot[] = {0x%02x, 0x%02x}\n",
				 port->sDir.abSlot[0], port->sDir.abSlot[1]);
		}
		if (update & MCDRV_DIO0_DIT_UPDATE_FLAG) {
			dbg_info("sDit.wSrcRate = 0x%04x\n",
				 port->sDit.wSrcRate);
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
			dbg_info("     abSlot[] = {0x%02x, 0x%02x}\n",
				 port->sDit.abSlot[0], port->sDit.abSlot[1]);
		}
	}
}

static void mc1n2_dump_dac_info(const void *pvPrm, UINT32 dPrm)
{
	MCDRV_DAC_INFO *info = (MCDRV_DAC_INFO *)pvPrm;

	if (dPrm & MCDRV_DAC_MSWP_UPDATE_FLAG) {
		dbg_info("bMasterSwap = 0x%02x\n", info->bMasterSwap);
	}
	if (dPrm & MCDRV_DAC_VSWP_UPDATE_FLAG) {
		dbg_info("bVoiceSwap = 0x%02x\n", info->bVoiceSwap);
	}
	if (dPrm & MCDRV_DAC_HPF_UPDATE_FLAG) {
		dbg_info("bDcCut = 0x%02x\n", info->bDcCut);
	}
}

static void mc1n2_dump_adc_info(const void *pvPrm, UINT32 dPrm)
{
	MCDRV_ADC_INFO *info = (MCDRV_ADC_INFO *)pvPrm;

	if (dPrm & MCDRV_ADCADJ_UPDATE_FLAG) {
		dbg_info("bAgcAdjust = 0x%02x\n", info->bAgcAdjust);
	}
	if (dPrm & MCDRV_ADCAGC_UPDATE_FLAG) {
		dbg_info("bAgcOn = 0x%02x\n", info->bAgcOn);
	}
	if (dPrm & MCDRV_ADCMONO_UPDATE_FLAG) {
		dbg_info("bMono = 0x%02x\n", info->bMono);
	}
}

static void mc1n2_dump_sp_info(const void *pvPrm, UINT32 dPrm)
{
	MCDRV_SP_INFO *info = (MCDRV_SP_INFO *)pvPrm;

	dbg_info("bSwap = 0x%02x\n", info->bSwap);
}

static void mc1n2_dump_dng_info(const void *pvPrm, UINT32 dPrm)
{
	MCDRV_DNG_INFO *info = (MCDRV_DNG_INFO *)pvPrm;

	if (dPrm & MCDRV_DNGSW_HP_UPDATE_FLAG) {
		dbg_info("HP:abOnOff = 0x%02x\n", info->abOnOff[0]);
	}
	if (dPrm & MCDRV_DNGTHRES_HP_UPDATE_FLAG) {
		dbg_info("HP:abThreshold = 0x%02x\n", info->abThreshold[0]);
	}
	if (dPrm & MCDRV_DNGHOLD_HP_UPDATE_FLAG) {
		dbg_info("HP:abHold = 0x%02x\n", info->abHold[0]);
	}
	if (dPrm & MCDRV_DNGATK_HP_UPDATE_FLAG) {
		dbg_info("HP:abAttack = 0x%02x\n", info->abAttack[0]);
	}
	if (dPrm & MCDRV_DNGREL_HP_UPDATE_FLAG) {
		dbg_info("HP:abRelease = 0x%02x\n", info->abRelease[0]);
	}
	if (dPrm & MCDRV_DNGTARGET_HP_UPDATE_FLAG) {
		dbg_info("HP:abTarget = 0x%02x\n", info->abTarget[0]);
	}

	if (dPrm & MCDRV_DNGSW_SP_UPDATE_FLAG) {
		dbg_info("SP:abOnOff = 0x%02x\n", info->abOnOff[1]);
	}
	if (dPrm & MCDRV_DNGTHRES_SP_UPDATE_FLAG) {
		dbg_info("SP:abThreshold = 0x%02x\n", info->abThreshold[1]);
	}
	if (dPrm & MCDRV_DNGHOLD_SP_UPDATE_FLAG) {
		dbg_info("SP:abHold = 0x%02x\n", info->abHold[1]);
	}
	if (dPrm & MCDRV_DNGATK_SP_UPDATE_FLAG) {
		dbg_info("SP:abAttack = 0x%02x\n", info->abAttack[1]);
	}
	if (dPrm & MCDRV_DNGREL_SP_UPDATE_FLAG) {
		dbg_info("SP:abRelease = 0x%02x\n", info->abRelease[1]);
	}
	if (dPrm & MCDRV_DNGTARGET_SP_UPDATE_FLAG) {
		dbg_info("SP:abTarget = 0x%02x\n", info->abTarget[1]);
	}

	if (dPrm & MCDRV_DNGSW_RC_UPDATE_FLAG) {
		dbg_info("RC:abOnOff = 0x%02x\n", info->abOnOff[2]);
	}
	if (dPrm & MCDRV_DNGTHRES_RC_UPDATE_FLAG) {
		dbg_info("RC:abThreshold = 0x%02x\n", info->abThreshold[2]);
	}
	if (dPrm & MCDRV_DNGHOLD_RC_UPDATE_FLAG) {
		dbg_info("RC:abHold = 0x%02x\n", info->abHold[2]);
	}
	if (dPrm & MCDRV_DNGATK_RC_UPDATE_FLAG) {
		dbg_info("RC:abAttack = 0x%02x\n", info->abAttack[2]);
	}
	if (dPrm & MCDRV_DNGREL_RC_UPDATE_FLAG) {
		dbg_info("RC:abRelease = 0x%02x\n", info->abRelease[2]);
	}
	if (dPrm & MCDRV_DNGTARGET_RC_UPDATE_FLAG) {
		dbg_info("RC:abTarget = 0x%02x\n", info->abTarget[2]);
	}
}

static void mc1n2_dump_ae_info(const void *pvPrm, UINT32 dPrm)
{
	MCDRV_AE_INFO *info = (MCDRV_AE_INFO *)pvPrm;

	dbg_info("bOnOff = 0x%02x\n", info->bOnOff);
	if (dPrm & MCDRV_AEUPDATE_FLAG_BEX) {
		mc1n2_dump_array("abBex", info->abBex, BEX_PARAM_SIZE);
	}
	if (dPrm & MCDRV_AEUPDATE_FLAG_WIDE) {
		mc1n2_dump_array("abWide", info->abWide, WIDE_PARAM_SIZE);
	}
	if (dPrm & MCDRV_AEUPDATE_FLAG_DRC) {
		mc1n2_dump_array("abDrc", info->abDrc, DRC_PARAM_SIZE);
	}
	if (dPrm & MCDRV_AEUPDATE_FLAG_EQ5) {
		mc1n2_dump_array("abEq5", info->abEq5, EQ5_PARAM_SIZE);
	}
	if (dPrm & MCDRV_AEUPDATE_FLAG_EQ3) {
		mc1n2_dump_array("abEq3", info->abEq3, EQ5_PARAM_SIZE);
	}
}

static void mc1n2_dump_pdm_info(const void *pvPrm, UINT32 dPrm)
{
	MCDRV_PDM_INFO *info = (MCDRV_PDM_INFO *)pvPrm;

	if (dPrm & MCDRV_PDMCLK_UPDATE_FLAG) {
		dbg_info("bClk = 0x%02x\n", info->bClk);
	}
	if (dPrm & MCDRV_PDMADJ_UPDATE_FLAG) {
		dbg_info("bAgcAdjust = 0x%02x\n", info->bAgcAdjust);
	}
	if (dPrm & MCDRV_PDMAGC_UPDATE_FLAG) {
		dbg_info("bAgcOn = 0x%02x\n", info->bAgcOn);
	}
	if (dPrm & MCDRV_PDMEDGE_UPDATE_FLAG) {
		dbg_info("bPdmEdge = 0x%02x\n", info->bPdmEdge);
	}
	if (dPrm & MCDRV_PDMWAIT_UPDATE_FLAG) {
		dbg_info("bPdmWait = 0x%02x\n", info->bPdmWait);
	}
	if (dPrm & MCDRV_PDMSEL_UPDATE_FLAG) {
		dbg_info("bPdmSel = 0x%02x\n", info->bPdmSel);
	}
	if (dPrm & MCDRV_PDMMONO_UPDATE_FLAG) {
		dbg_info("bMono = 0x%02x\n", info->bMono);
	}
}

static void mc1n2_dump_clksw_info(const void *pvPrm, UINT32 dPrm)
{
	MCDRV_CLKSW_INFO *info = (MCDRV_CLKSW_INFO *)pvPrm;

	dbg_info("bClkSrc = 0x%02x\n", info->bClkSrc);
}

static void mc1n2_dump_syseq_info(const void *pvPrm, UINT32 dPrm)
{
	MCDRV_SYSEQ_INFO *info = (MCDRV_SYSEQ_INFO *)pvPrm;
	int i;

	if (dPrm & MCDRV_SYSEQ_ONOFF_UPDATE_FLAG) {
		dbg_info("bOnOff = 0x%02x\n", info->bOnOff);
	}
	if (dPrm & MCDRV_SYSEQ_PARAM_UPDATE_FLAG) {
		for (i = 0; i < 15; i++){
			dbg_info("abParam[%d] = 0x%02x\n", i, info->abParam[i]);
		}
	}
}

struct mc1n2_dump_func {
	char *name;
	void (*func)(const void *, UINT32);
};

struct mc1n2_dump_func mc1n2_dump_func_map[] = {
	{"MCDRV_INIT", NULL},
	{"MCDRV_TERM", NULL},
	{"MCDRV_READ_REG", mc1n2_dump_reg_info},
	{"MCDRV_WRITE_REG", NULL},
	{"MCDRV_GET_PATH", NULL},
	{"MCDRV_SET_PATH", mc1n2_dump_path_info},
	{"MCDRV_GET_VOLUME", NULL},
	{"MCDRV_SET_VOLUME", mc1n2_dump_vol_info},
	{"MCDRV_GET_DIGITALIO", NULL},
	{"MCDRV_SET_DIGITALIO", mc1n2_dump_dio_info},
	{"MCDRV_GET_DAC", NULL},
	{"MCDRV_SET_DAC", mc1n2_dump_dac_info},
	{"MCDRV_GET_ADC", NULL},
	{"MCDRV_SET_ADC", mc1n2_dump_adc_info},
	{"MCDRV_GET_SP", NULL},
	{"MCDRV_SET_SP", mc1n2_dump_sp_info},
	{"MCDRV_GET_DNG", NULL},
	{"MCDRV_SET_DNG", mc1n2_dump_dng_info},
	{"MCDRV_SET_AUDIOENGINE", mc1n2_dump_ae_info},
	{"MCDRV_SET_AUDIOENGINE_EX", NULL},
	{"MCDRV_SET_CDSP", NULL},
	{"MCDRV_GET_CDSP_PARAM", NULL},
	{"MCDRV_SET_CDSP_PARAM", NULL},
	{"MCDRV_REGISTER_CDSP_CB", NULL},
	{"MCDRV_GET_PDM", NULL},
	{"MCDRV_SET_PDM", mc1n2_dump_pdm_info},
	{"MCDRV_SET_DTMF", NULL},
	{"MCDRV_CONFIG_GP", NULL},
	{"MCDRV_MASK_GP", NULL},
	{"MCDRV_GETSET_GP", NULL},
	{"MCDRV_GET_PEAK", NULL},
	{"MCDRV_IRQ", NULL},
	{"MCDRV_UPDATE_CLOCK", NULL},
	{"MCDRV_SWITCH_CLOCK", mc1n2_dump_clksw_info},
	{"MCDRV_GET_SYSEQ", NULL},
	{"MCDRV_SET_SYSEQ", mc1n2_dump_syseq_info},
};

SINT32 McDrv_Ctrl_dbg(UINT32 dCmd, void *pvPrm, UINT32 dPrm)
{
	SINT32 err;

	dbg_info("calling %s:\n", mc1n2_dump_func_map[dCmd].name);

	if (mc1n2_dump_func_map[dCmd].func) {
		mc1n2_dump_func_map[dCmd].func(pvPrm, dPrm);
	}

	err = McDrv_Ctrl(dCmd, pvPrm, dPrm);
	dbg_info("err = %ld\n", err);

	return err;
}

#endif /* CONFIG_SND_SOC_MC1N2_DEBUG */
