/*
 *
 * File name: drivers/media/tdmb/mtv318/src/raontv_internal.h
 *
 * Description : RAONTECH TV internal header file.
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

#ifndef __RAONTV_INTERNAL_H__
#define __RAONTV_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "raontv.h"

/* Do not modify the order! */
enum E_RTV_TV_MODE_TYPE {
	RTV_TV_MODE_TDMB = 0, /* Band III  Korea */
	RTV_TV_MODE_DAB_B3 = 1, /* Band III*/
	RTV_TV_MODE_DAB_L = 2, /* L-Band*/
	RTV_TV_MODE_1SEG = 3, /* UHF*/
	RTV_TV_MODE_FM = 4, /* FM*/
	MAX_NUM_RTV_MODE
};

struct RTV_REG_INIT_INFO {
	U8 bReg;
	U8 bVal;
};

struct RTV_REG_MASK_INFO {
	U8 bReg;
	U8 bMask;
	U8 bVal;
};

#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
#if defined(RTV_TSIF_CLK_SPEED_DIV_2) /* Host Clk/2*/
#define RTV_COMM_CON47_CLK_SEL	(0<<6)
#elif defined(RTV_TSIF_CLK_SPEED_DIV_4) /* Host Clk/4*/
#define RTV_COMM_CON47_CLK_SEL	(1<<6)
#elif defined(RTV_TSIF_CLK_SPEED_DIV_6) /* Host Clk/6*/
#define RTV_COMM_CON47_CLK_SEL	(2<<6)
#elif defined(RTV_TSIF_CLK_SPEED_DIV_8) /* Host Clk/8*/
#define RTV_COMM_CON47_CLK_SEL	(3<<6)
#else
#error "Code not present"
#endif
#endif

#define MSC1_E_OVER_FLOW       0x40
#define MSC1_E_UNDER_FLOW      0x20
#define MSC1_E_INT             0x10
#define MSC0_E_OVER_FLOW       0x08
#define MSC0_E_UNDER_FLOW      0x04
#define MSC0_E_INT             0x02
#define FIC_E_INT              0x01
#define RE_CONFIG_E_INT        0x04

#define MSC1_INTR_BITS	(MSC1_E_INT|MSC1_E_UNDER_FLOW|MSC1_E_OVER_FLOW)
#define MSC0_INTR_BITS	(MSC0_E_INT|MSC0_E_UNDER_FLOW|MSC0_E_OVER_FLOW)

/* [2]MSC1 int clear [1]MSC0 int clear [0]FIC int clear */
#define INT_E_UCLRL         (0x35)

/* [6]OFDM TII done clear */
#define INT_E_UCLRH         (0x36)

/* [7]OFDM Lock status [6]MSC1 overrun [5]MSC1 underrun
 * [4]MSC1 int [3]MSC0 overrun [2]MSC0 underrun [1]MSC0 int [0]FIC int */
#define INT_E_STATL         (0x33)

/* [7]OFDM NIS [6]OFDM TII [5]OFDM scan [4]OFDM window position
 * [3]OFDM unlock [2]FEC re-configuration [1]FEC CIF end [0]FEC soft reset*/
#define INT_E_STATH         (0x34)

/* [7]Buf_en [6]PKT_CRC_MODE:enable (stored) [5]MPEG_HEAD [4]MPEG-2TS_EN
 * [3]EPKT_MODE [2]CAS_MODE [1]FIDC_MODE [0]FIC_INIT_EN == 1:enable*/
#define DD_E_TOPCON         (0x45)

/* [4]FIC_CRC stored- 1:enable [3]FIC uclear-1
 * [2]FIC_Update - 1:dependent by user
 * [1]FIC_EN [0]FIG_EN - 1:only FIG 6 dump*/
#define FIC_E_DDCON         (0x46)

/* [3]MSC0 uclear-1
 * [2]MSC0_en
 * [1]MSC0 read length -1:user length, 0:interrupt length
 * [0]MSC0 interrupt sel-1:user th, 0:CIF end*/
#define MSC0_E_CON          (0x47)

/* [5]MSC1_length-0:subch+length [4]MSC1 header-0:disable [3]MSC1 uclear-1
 * [2]MSC1_en [1]MSC1 read length -1:user length, 0:interrupt length
 * [0]MSC1 interrupt sel-1:user th, 0:CIF end*/
#define MSC1_E_CON          (0x48)

/* [3]NIS uclear-1 [2]NIS_Update - 1:dependent by user
 * [1]TII status clear -1:user set only, 0:user set & internal event
 * [0]TII_Update - 1:dependent by user*/
#define OFDM_E_DDCON        (0x49)

/* [11:8] setting in MSC0 read length for interrupt clear*/
#define MSC0_E_RSIZE_H      (0x4E)

/* [7:0]*/
#define MSC0_E_RSIZE_L      (0x4F)

/* [11:8] MSC0 interrupt threshold*/
#define MSC0_E_INTTH_H      (0x50)

/* [7:0]*/
#define MSC0_E_INTTH_L      (0x51)

/* [11:8]*/
#define MSC0_E_TSIZE_L      (0x52)

/* [7:0] MSC0 total size which you can read*/
#define MSC0_E_TSIZE_H      (0x53)

/* [11:8] setting in MSC1 read length for interrupt clear*/
#define MSC1_E_RSIZE_H      (0x54)
#define MSC1_E_RSIZE_L      (0x55)  /* [7:0]*/
#define MSC1_E_INTTH_H      (0x56)  /* [11:8] MSC1 interrupt threshold*/
#define MSC1_E_INTTH_L      (0x57)  /* [7:0]*/
#define MSC1_E_TSIZE_L      (0x58)  /* [11:8]*/
#define MSC1_E_TSIZE_H      (0x59)  /* [7:0] MSC1 read length*/

#define MODE1 2
#define MODE2 1
#define MODE3 0

#define MAP_SEL_REG	0x03

#define OFDM_PAGE	0x02 /* for 1seg */
#define FEC_PAGE	0x03 /* for 1seg */
#define COMM_PAGE	0x04
#define FM_PAGE		0x06 /* T-DMB OFDM/FM */
#define HOST_PAGE	0x07
#define CAS_PAGE	0x08
#define DD_PAGE		0x09 /* FEC for TDMB, DAB, FM */
#define FIC_PAGE	0x0A
#define MSC0_PAGE	0x0B
#define MSC1_PAGE	0x0C
#define RF_PAGE		0x0F

#define DEMOD_0SC_DIV2_ON  0x80
#define DEMOD_0SC_DIV2_OFF 0x00

#if (RTV_SRC_CLK_FREQ_KHz >= 32000)
#define DEMOD_OSC_DIV2	DEMOD_0SC_DIV2_ON
#else
#define DEMOD_OSC_DIV2	DEMOD_0SC_DIV2_OFF
#endif

#define MAP_SEL_VAL(page)		(DEMOD_OSC_DIV2|page)
#define RTV_REG_MAP_SEL(page)	RTV_REG_SET(MAP_SEL_REG, MAP_SEL_VAL(page))

#define RTV_TS_STREAM_DISABLE_DELAY		20 /* ms */

/* ISDB-T Channel */
#define ISDBT_CH_NUM_START__JAPAN			13
#define ISDBT_CH_NUM_END__JAPAN				62
#define ISDBT_CH_FREQ_START__JAPAN			473143
#define ISDBT_CH_FREQ_STEP__JAPAN			6000

#define ISDBT_CH_NUM_START__BRAZIL			14
#define ISDBT_CH_NUM_END__BRAZIL			69
#define ISDBT_CH_FREQ_START__BRAZIL			473143
#define ISDBT_CH_FREQ_STEP__BRAZIL			6000

#define ISDBT_CH_NUM_START__ARGENTINA		14
#define ISDBT_CH_NUM_END__ARGENTINA			69
#define ISDBT_CH_FREQ_START__ARGENTINA		473143
#define ISDBT_CH_FREQ_STEP__ARGENTINA		6000

/* T-DMB Channel */
#define TDMB_CH_FREQ_START__KOREA		175280
#define TDMB_CH_FREQ_STEP__KOREA		1728 /* about... */
/* DAB Channel */
#define DAB_CH_BAND3_START_FREQ_KHz		174928
#define DAB_CH_BAND3_STEP_FREQ_KHz		1712 /* in KHz */
#define DAB_CH_LBAND_START_FREQ_KHz		1452960
#define DAB_CH_LBAND_STEP_STEP_FREQ_KHz	1712 /*in KHz */

extern enum E_RTV_ADC_CLK_FREQ_TYPE g_eRtvAdcClkFreqType;
extern enum E_RTV_COUNTRY_BAND_TYPE g_eRtvCountryBandType;

#ifdef RTV_DAB_ENABLE
extern enum E_RTV_TV_MODE_TYPE g_curDabSetType;
#endif

/* Use SPI/EBI2 interrupt handler to prevent the changing of register map. */
extern BOOL g_fRtvChannelChange;
extern BOOL g_fRtvStreamEnabled;

extern UINT g_nRtvMscThresholdSize;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
extern U8 g_bRtvIntrMaskRegL;

#else
#if !defined(RTV_CIF_MODE_ENABLED) || !defined(RTV_FIC_POLLING_MODE)
/* T-DMB: Single sub channel and Serial TSIF */
extern U8 g_bRtvIntrMaskRegL;
#endif
#endif

/*See PLL table in raontv_rf_pll_data_fm.h */
#define FM_MIN_FREQ_KHz			75950
#define FM_MAX_FREQ_KHz			108050
#define FM_SCAN_STEP_FREQ_KHz		(RTV_FM_CH_STEP_FREQ_KHz/2)

#if (RTV_FM_CH_MIN_FREQ_KHz < FM_MIN_FREQ_KHz) \
	|| (RTV_FM_CH_MAX_FREQ_KHz > FM_MAX_FREQ_KHz)
#error "Invalid FM freq range"
#endif

/*============================================================================
 *
 * Common inline functions.
 *
 *===========================================================================*/

/* Forward prototype. */
static INLINE void rtv_SetupMemory_MSC1(U16 wThresholdSize);

static INLINE enum E_RTV_TV_MODE_TYPE rtv_GetDabTvMode(U32 dwChFreqKHz)
{
	if (dwChFreqKHz >= DAB_CH_LBAND_START_FREQ_KHz) {
		/*RTV_DBGMSG0("return RTV_TV_MODE_DAB_L;\n");*/
		return RTV_TV_MODE_DAB_L; /* L-Band */
	} else {
		switch (dwChFreqKHz) {
		case 175280/*7A*/: case 177008/*7B*/: case 178736/*7C*/:
		case 181280/*8A*/: case 183008/*8B*/: case 184736/*8C*/:
		case 187280/*9A*/: case 189008/*9B*/: case 190736/*9C*/:
		case 193280/*10A*/: case 195008/*10B*/: case 196736/*10C*/:
		case 199280/*11A*/: case 201008/*11B*/: case 202736/*11C*/:
		case 205280/*12A*/: case 207008/*12B*/: case 208736/*12C*/:
		case 211280/*13A*/: case 213008/*13B*/: case 214736/*13C*/:
			/*RTV_DBGMSG0("return RTV_TV_MODE_TDMB;\n");*/
			return RTV_TV_MODE_TDMB; /*Korea TDMB.*/

		default:
			/*RTV_DBGMSG0("return RTV_TV_MODE_DAB_B3;\n"); */
			return RTV_TV_MODE_DAB_B3; /* DAB Band-III */
		}
	}
}

/* Pause straem */
static INLINE void rtv_StreamDisable(enum E_RTV_TV_MODE_TYPE eTvMode)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	if (g_fRtvStreamEnabled == FALSE)
		return;

	switch (eTvMode) {
#ifdef RTV_ISDBT_ENABLE
	case RTV_TV_MODE_1SEG:
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		RTV_REG_MAP_SEL(HOST_PAGE);
		/* FIC, MSC0, MSC1 */
		RTV_REG_SET(0x62,
			g_bRtvIntrMaskRegL|MSC0_INTR_BITS|MSC1_INTR_BITS);

#elif defined(RTV_IF_MPEG2_SERIAL_TSIF) \
	|| defined(RTV_IF_SPI_SLAVE)\
	|| defined(RTV_IF_QUALCOMM_TSIF)

		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x29, 0x08);
		RTV_DELAY_MS(RTV_TS_STREAM_DISABLE_DELAY);
#endif
		break;
#endif /* RTV_ISDBT_ENABLE */

#ifdef RTV_FM_ENABLE
	case RTV_TV_MODE_FM:
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		RTV_REG_MAP_SEL(HOST_PAGE);
		/* FIC, MSC0, MSC1 */
		RTV_REG_SET(0x62,
			g_bRtvIntrMaskRegL|MSC0_INTR_BITS|MSC1_INTR_BITS);

#elif defined(RTV_IF_MPEG2_SERIAL_TSIF) \
	|| defined(RTV_IF_SPI_SLAVE) \
	|| defined(RTV_IF_QUALCOMM_TSIF)

		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x29, 0x08);
		RTV_DELAY_MS(RTV_TS_STREAM_DISABLE_DELAY);
#endif
		break;
#endif/* RTV_FM_ENABLE */

#ifdef RTV_TDMB_ENABLE
	case RTV_TV_MODE_TDMB:
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		RTV_REG_MAP_SEL(HOST_PAGE);

		/* FIC, MSC0, MSC1 */
		RTV_REG_SET(0x62,
		g_bRtvIntrMaskRegL|MSC0_INTR_BITS|MSC1_INTR_BITS|FIC_E_INT);
#elif defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x29, 0x08);
		RTV_DELAY_MS(RTV_TS_STREAM_DISABLE_DELAY);

#ifndef RTV_CIF_MODE_ENABLED /* Individual Mode */
#ifndef RTV_FIC_POLLING_MODE
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x62, g_bRtvIntrMaskRegL|FIC_E_INT);
#endif
#else /* CIF Mode. */

#endif
#endif
		break;
#endif	/* RTV_TDMB_ENABLE */

#ifdef RTV_DAB_ENABLE
	case RTV_TV_MODE_DAB_B3:
	case RTV_TV_MODE_DAB_L:
	case RTV_TV_MODE_TDMB:
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x62,
		g_bRtvIntrMaskRegL|MSC0_INTR_BITS|MSC1_INTR_BITS|FIC_E_INT);

#elif defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x29, 0x08);
		RTV_DELAY_MS(RTV_TS_STREAM_DISABLE_DELAY);

#ifndef RTV_CIF_MODE_ENABLED /* Individual Mode */
#ifndef RTV_FIC_POLLING_MODE
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x62, g_bRtvIntrMaskRegL|FIC_E_INT);
#endif
#else /* Multi Sub Channel Mode. */

#endif
#endif
		break;
#endif	/* RTV_DAB_ENABLE */

	default:
		break;
	}

	g_fRtvStreamEnabled = FALSE;
#endif
}


static INLINE void rtv_StreamRestore(enum E_RTV_TV_MODE_TYPE eTvMode)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	if (g_fRtvStreamEnabled == TRUE)
		return;

	switch (eTvMode) {
#ifdef RTV_ISDBT_ENABLE
	case RTV_TV_MODE_1SEG:
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		RTV_REG_MAP_SEL(DD_PAGE);
		RTV_REG_SET(0x35, 0x04); /* MSC1 Interrupt status clear. */

		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x62, g_bRtvIntrMaskRegL);
#endif
		break;
#endif

#ifdef RTV_FM_ENABLE
	case RTV_TV_MODE_FM:
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		RTV_REG_MAP_SEL(DD_PAGE);
		RTV_REG_SET(0x35, 0x04);

		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x62, g_bRtvIntrMaskRegL);
#elif defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x29, 0x00);
#endif
		break;
#endif

#ifdef RTV_TDMB_ENABLE
	case RTV_TV_MODE_TDMB:
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		RTV_REG_MAP_SEL(DD_PAGE);
		RTV_REG_SET(0x35, 0x07);

		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x62, g_bRtvIntrMaskRegL);

#elif defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x29, 0x00);

#ifndef RTV_CIF_MODE_ENABLED /* Individual Mode */
#ifndef RTV_FIC_POLLING_MODE
		RTV_REG_MAP_SEL(DD_PAGE);
		RTV_REG_SET(0x35, 0x01);

		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x62, g_bRtvIntrMaskRegL);
#endif
#else /* Multi Sub Channel Mode. */
#endif
#endif
		break;
#endif

#ifdef RTV_DAB_ENABLE
	case RTV_TV_MODE_DAB_B3:
	case RTV_TV_MODE_DAB_L:
	case RTV_TV_MODE_TDMB:
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		RTV_REG_MAP_SEL(DD_PAGE);
		RTV_REG_SET(0x35, 0x07);

		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x62, g_bRtvIntrMaskRegL);/* FIC, MSC0, MSC1 */

#elif defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x29, 0x00);

#ifndef RTV_CIF_MODE_ENABLED /* Individual Mode */
#ifndef RTV_FIC_POLLING_MODE
		RTV_REG_MAP_SEL(DD_PAGE);
		RTV_REG_SET(0x35, 0x01); /* FIC Interrupt status clear. */

		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x62, g_bRtvIntrMaskRegL);
#endif
#else /* Multi Sub Channel Mode. */
#endif
#endif
		break;
#endif	/* RTV_DAB_ENABLE */
	default:
		/* RTV_DBGMSG0("[rtv_StreamRestore] Invalid TV mode.\n"); */
		break;
	}

	g_fRtvStreamEnabled = TRUE;
#endif
}

/* Enable the stream path forcely for ISDB-T and FM only! */
static INLINE void rtv_StreamEnable(void)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	if (g_fRtvStreamEnabled == TRUE)
		return;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x35, 0x04); /* MSC1 Interrupt status clear. */

	RTV_REG_MAP_SEL(HOST_PAGE);

	/* [6] MSC1 over-run [5] MSC1 under-run [4] MSC1 interrupt */
	g_bRtvIntrMaskRegL = 0x8F;
	RTV_REG_SET(0x62, g_bRtvIntrMaskRegL);

#elif defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x29, 0x00);
#endif

	/* Restore MSC1 memory. */
	rtv_SetupMemory_MSC1(g_nRtvMscThresholdSize);

	g_fRtvStreamEnabled = TRUE;
#endif /* #ifndef RTV_IF_MPEG2_PARALLEL_TSIF */
}

static INLINE void rtv_ConfigureTsifFormat(void)
{
	RTV_REG_MAP_SEL(COMM_PAGE);

#if defined(RTV_IF_MPEG2_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
#if defined(RTV_TSIF_FORMAT_1)
	RTV_REG_SET(0x45, 0x00);
#elif defined(RTV_TSIF_FORMAT_2)
	RTV_REG_SET(0x45, 0x02);
#elif defined(RTV_TSIF_FORMAT_3)
	RTV_REG_SET(0x45, 0x21);
#elif defined(RTV_TSIF_FORMAT_4)
	RTV_REG_SET(0x45, 0x23);
#else
#error "Code not present"
#endif

#elif defined(RTV_IF_QUALCOMM_TSIF)
#if defined(RTV_TSIF_FORMAT_1)
	RTV_REG_SET(0x45, 0x00);
#elif defined(RTV_TSIF_FORMAT_2)
	RTV_REG_SET(0x45, 0xE9);

#elif defined(RTV_TSIF_FORMAT_3)
	RTV_REG_SET(0x45, 0xE1);
#elif defined(RTV_TSIF_FORMAT_4)
	RTV_REG_SET(0x45, 0x40);
#elif defined(RTV_TSIF_FORMAT_5)
	RTV_REG_SET(0x45, 0x21);
#else
#error "Code not present"
#endif
#endif

#if defined(RTV_IF_MPEG2_SERIAL_TSIF)
	RTV_REG_SET(0x46, 0x80);
#elif defined(RTV_IF_QUALCOMM_TSIF)
	RTV_REG_SET(0x46, 0xA0);
#elif defined(RTV_IF_SPI_SLAVE)
	RTV_REG_SET(0x46, 0x82);
#endif
}

static INLINE void rtv_ResetMemory_MSC0(void)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x47, 0x00); /* MSC0 memory control register clear. */
}

static INLINE void rtv_ResetMemory_MSC1(void)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x48, 0x00); /* MSC1 memory control register clear. */
#endif
}

/* Only for T-DMB and DAB. */
static INLINE void rtv_ClearAndSetupMemory_MSC0(void)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
#ifdef RTV_MSC_INTR_ISTAUS_ACC_CLR_MODE
	U8 msc0_int_usel = 0; /* 0: auto&uclr, 1: user set only */
#else
	U8 msc0_int_usel = 1;
#endif
#else
	U8 msc0_int_usel = 0;
#endif

#ifndef RTV_CIF_MODE_ENABLED /* Individual Mode */
	U8 int_type = 1; /* 0: CIF end,  1: Threshold */
#else
	U8 int_type = 0;
#endif

	RTV_REG_SET(0x47, 0x00);
	RTV_REG_SET(0x47, (msc0_int_usel<<3) | (1/*msc0_en*/<<2) | int_type);
#endif /* #ifndef RTV_IF_MPEG2_PARALLEL_TSIF */
}

/* Only for T-DMB and DAB. */
static INLINE void rtv_SetupMemory_MSC0(U16 wThresholdSize)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	RTV_REG_MAP_SEL(DD_PAGE);

	RTV_REG_SET(0x50, ((wThresholdSize>>8) & 0x0F));
	RTV_REG_SET(0x51, (wThresholdSize & 0xFF));

	rtv_ClearAndSetupMemory_MSC0();

	RTV_REG_SET(0x35, 0x02); /* MSC0 Interrupt clear. */

#endif /* #ifndef RTV_IF_MPEG2_PARALLEL_TSIF */
}

/* For SPI interface ISR. */
static INLINE void rtv_ClearAndSetupMemory_MSC1(void)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
#ifdef RTV_MSC_INTR_ISTAUS_ACC_CLR_MODE
	U8 msc1_int_usel = 0; /* 0: auto&uclr, 1: user set only */
#else
	U8 msc1_int_usel = 1;
#endif
#else
	/* TSIF */
	U8 msc1_int_usel = 0;
#endif

	U8 int_type = 1; /* 0: CIF end, 1: Threshold (for SPI, Header OFF) */

	RTV_REG_SET(0x48, 0x00);
	RTV_REG_SET(0x48,
	(0<<5) | (0<<4) | (msc1_int_usel<<3) | (1/*msc1_en*/<<2) | int_type);

#endif /* #ifndef RTV_IF_MPEG2_PARALLEL_TSIF */
}

static INLINE void rtv_SetupMemory_MSC1(U16 wThresholdSize)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	RTV_REG_MAP_SEL(DD_PAGE);

	/* The below settings are not applied to CIF of TDMB/DAB. */
	RTV_REG_SET(0x56, ((wThresholdSize>>8) & 0x0F));
	RTV_REG_SET(0x57, (wThresholdSize & 0xFF));

	rtv_ClearAndSetupMemory_MSC1();
#endif /* #ifndef RTV_IF_MPEG2_PARALLEL_TSIF */
}

/* Only this sub channel contains the RS decoder. */
static INLINE void rtv_Set_MSC1_SUBCH0(UINT nSubChID,
					BOOL fSubChEnable, BOOL fRsEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);

#if !defined(RTV_IF_MPEG2_PARALLEL_TSIF)
	RTV_REG_SET(0x3A, (fSubChEnable << 7) | (fRsEnable << 6) | nSubChID);
#else
	if (fRsEnable == TRUE) {
		RTV_REG_SET(0xFF, 0x00);
		RTV_REG_SET(0x3A,
			(fSubChEnable << 7) | (fRsEnable << 6) | nSubChID);

	} else {
		RTV_REG_SET(0x3A, 0x00);
		RTV_REG_SET(0xFF, (fSubChEnable << 7) | nSubChID);
	}
#endif
}

static INLINE void rtv_Set_MSC1_SUBCH1(UINT nSubChID, BOOL fSubChEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x3B, (fSubChEnable << 7) | nSubChID);
}

static INLINE void rtv_Set_MSC1_SUBCH2(UINT nSubChID, BOOL fSubChEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x3C, (fSubChEnable << 7) | nSubChID);
}

static INLINE void rtv_Set_MSC0_SUBCH3(UINT nSubChID, BOOL fSubChEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x3D, (fSubChEnable << 7) | nSubChID);
}

static INLINE void rtv_Set_MSC0_SUBCH4(UINT nSubChID, BOOL fSubChEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x3E, (fSubChEnable << 7) | nSubChID);
}

static INLINE void rtv_Set_MSC0_SUBCH5(UINT nSubChID, BOOL fSubChEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x3F, (fSubChEnable << 7) | nSubChID);
}

static INLINE void rtv_Set_MSC0_SUBCH6(UINT nSubChID, BOOL fSubChEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x42, (fSubChEnable << 7) | nSubChID);
}

/*=============================================================================
 *
 * T-DMB inline functions.
 *
 *===========================================================================*/
static INLINE void rtv_SetupMemory_FIC(void)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	/* auto user clr, get fic  CRC 2byte including[4] */
	RTV_REG_SET(0x46, 0x10);
	RTV_REG_SET(0x46, 0x16); /* FIC enable */
}

static INLINE void rtv_ResetMemory_FIC(void)
{

	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x46, 0x00);
}

/*==============================================================================
 *
 * Parallel TSIF inline functions.
 *
 *============================================================================*/
#ifdef RTV_IF_MPEG2_PARALLEL_TSIF
#if defined(RTV_ISDBT_ENABLE)
static INLINE void rtv_SetParallelTsif_1SEG_Only(void)
{
	RTV_REG_MAP_SEL(FEC_PAGE);

#if defined(RTV_TSIF_FORMAT_1)
	RTV_REG_SET(0x6E, 0x11);
#elif defined(RTV_TSIF_FORMAT_2)
	RTV_REG_SET(0x6E, 0x13);
#elif defined(RTV_TSIF_FORMAT_3)
	RTV_REG_SET(0x6E, 0x10);
#elif defined(RTV_TSIF_FORMAT_4)
	RTV_REG_SET(0x6E, 0x12);
#else
#error "Code not present"
#endif
	RTV_REG_SET(0x6F, 0x02);
	RTV_REG_SET(0x70, 0x88);
}

#elif defined(RTV_TDMB_ENABLE)
static INLINE void rtv_SetParallelTsif_TDMB_Only(void)
{
	RTV_REG_MAP_SEL(0x09);

#if defined(RTV_TSIF_FORMAT_1)
	RTV_REG_SET(0xDD, 0xD0);
#elif defined(RTV_TSIF_FORMAT_2)
	RTV_REG_SET(0xDD, 0xD2);
#elif defined(RTV_TSIF_FORMAT_3)
	RTV_REG_SET(0xDD, 0xD1);
#elif defined(RTV_TSIF_FORMAT_4)
	RTV_REG_SET(0xDD, 0xD3);
#else
#error "Code not present"
#endif
	RTV_REG_SET(0xDE, 0x12);
	RTV_REG_SET(0x45, 0xB0);
}
#else
#error "Code not present"
#endif
#endif

/*==============================================================================
 * External functions for RAONTV driver core.
 *============================================================================*/
void rtv_ConfigureHostIF(void);
INT rtv_InitSystem(enum E_RTV_TV_MODE_TYPE eTvMode,
		enum E_RTV_ADC_CLK_FREQ_TYPE eAdcClkFreqType);

#ifdef RTV_CIF_HEADER_INSERTED
void rtvCIFDEC_Init(void);
void rtvCIFDEC_Deinit(void);
void rtvCIFDEC_DeleteSubChannelID(UINT nSubChID);
void rtvCIFDEC_AddSubChannelID(UINT nSubChID);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RAONTV_INTERNAL_H__ */

/*
 WINDOWS
 #define INLINE __inline
 #define FORCE_INLINE __forceinline
 #define NAKED __declspec(naked)

 __GNUC__
 #define INLINE __inline__
 #define FORCE_INLINE __attribute__((always_inline)) __inline__
 #define NAKED __attribute__((naked))

 ARM RVDS compiler
 #define INLINE __inline
 #define FORCE_INLINE __forceinline
 #define NAKED __asm
 */

