/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fc8050_regs.h

	Description : baseband header file

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


	History :
	----------------------------------------------------------------------
	2009/09/14	jason		initial
*******************************************************************************/

#ifndef __FC8050_REGS_H__
#define __FC8050_REGS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* X-TAL Frequency Configuration */
/* #define FC8050_FREQ_XTAL  16384 */
/* #define FC8050_FREQ_XTAL    19200 */
#define FC8050_FREQ_XTAL  24576
/* #define FC8050_FREQ_XTAL  27000 */
/* #define FC8050_FREQ_XTAL  38400 */
#define FEATURE_FC8050_DEBUG
/* #define FEATURE_INTERFACE_TEST_MODE */

/* INTERRUPT SOURCE */
#define BBM_MF_INT                  0x0001
#define BBM_WAGC_INT                0x0002
#define BBM_RECFG_INT               0x0004
#define BBM_TII_INT                 0x0008
#define BBM_SYNC_INT                0x0010
#define BBM_I2C_INT                 0x0020
#define	BBM_SCI_INT                 0x0040

/* Host Access Common Register */
#define BBM_COMMAND_REG				0x0000
#define BBM_ADDRESS_REG				0x0001
#define BBM_DATA_REG				0x0002

/* COMMON */
#define BBM_COM_AP2APB_LT           0x0000
#define BBM_COM_RESET				0x0001
#define BBM_COM_INT_STATUS			0x0002
#define BBM_COM_INT_ENABLE          0x0003
#define BBM_COM_STATUS_ENABLE       0x0006
#define BBM_COM_FIC_DATA			0x0007
#define BBM_COM_CH0_DATA			0x0008
#define BBM_COM_CH1_DATA			0x0009
#define BBM_COM_CH2_DATA			0x000a
#define BBM_COM_CH3_DATA			0x000b
#define BBM_COM_CH4_DATA			0x000c
#define BBM_COM_CH5_DATA			0x000d
#define BBM_COM_CH6_DATA			0x000e
#define BBM_COM_CH7_DATA			0x000f

/* QDD */
#define BBM_QDD_SYNC_RST_EN		0x0010
#define BBM_QDD_CHIP_IDL			0x0012
#define BBM_QDD_CHIP_IDH			0x0013
#define BBM_QDD_COMMAN				0x0014
#define BBM_QDD_TUN_COMMA			0x0015
#define BBM_QDD_TRAGET_RMS		0x0016
#define BBM_QDD_TUN_GAIN			0x0017
#define BBM_QDD_TUN_GAIN_LOC		0x0018
#define BBM_QDD_QLEVEL				0x0019
#define BBM_QDD_GAIN_MIN			0x001a
#define BBM_QDD_GAIN_MAX			0x001b
#define BBM_QDD_AGC_GAIN			0x001c
#define BBM_QDD_IF_RMS				0x001d
#define BBM_QDD_AGC_CTRL			0x001e
#define BBM_QDD_AGC_PERIOD		0x001f
#define BBM_QDD_AGC_STEP			0x0020
#define BBM_QDD_DC_CTRL			0x0021
#define BBM_QDD_I_COMP				0x0022
#define BBM_QDD_Q_COMP				0x0023
#define BBM_QDD_I_DC				0x0024
#define BBM_QDD_Q_DC				0x0025
#define BBM_QDD_PWM_VALL			0x0026
#define BBM_QDD_PWM_VALH			0x0027
#define BBM_QDD_PWM_CTRL			0x0028
#define BBM_QDD_UPDN_PERIOD		0x0029
#define BBM_QDD_M_FREQ_OFFSET		0x002a
#define BBM_QDD_COEF_BANK_SEL		0x0030
#define BBM_QDD_COEF				0x0031
#define BBM_QDD_AGC530_EN         0x0032
#define BBM_QDD_BLOCK_AVG_SIZE	0x0033
#define BBM_QDD_BLOCK_AVG_SIZE_LOCK 0x0034
#define BBM_QDD_GAIN_UPDATE_SPEED   0x0035
#define BBM_QDD_REF_AMPL            0x0036
#define BBM_QDD_BW_CTRL_LOCK        0x0037
#define BBM_QDD_GAIN_CONSTANT       0x0038
#define BBM_QDD_GAIN_CONSTANT_LOCK  0x0039
#define BBM_QDD_BLOCK_AVG           0x003a
#define BBM_QDD_DET_CNT_BOUND       0x003c
#define BBM_QDD_AGC_LOCK            0x003e
#define BBM_QDD_UPDOWN_CLK_PERIOD   0x003f

/* SYNC */
#define BBM_SYNC_WIN_SIZE			0x0050
#define BBM_SYNC_FRSYNC_PERIOD	0x0052
#define BBM_SYNC_NF_ON				0x0053
#define BBM_SYNC_RMS_CLIP			0x0054
#define BBM_SYNC_GSG_CF			0x0056
#define BBM_SYNC_MIN_CF			0x0057
#define BBM_SYNC_GCMD_CF			0x0058
#define BBM_SYNC_FTERR_THRESH		0x005A
#define BBM_SYNC_MA_GAIN			0x005B
#define BBM_SYNC_FTSYNC_CTRL		0x005C
#define BBM_SYNC_LPF_POWER		0x005e
#define BBM_SYNC_HPF_POWER		0x005f
#define BBM_SYNC_MODE				0x0060
#define BBM_SYNC_FFT_SHIFT		0x0061
#define BBM_SYNC_FF_ERROR			0x0062
#define BBM_SYNC_CIR_THRESH		0x0064
#define BBM_SYNC_FF_AVG_LEN		0x0065
#define BBM_SYNC_D4GIBSHIFT		0x0066
#define BBM_SYNC_CF_CBW			0x0067
#define BBM_SYNC_CF_ERROR			0x0068
#define BBM_SYNC_CFOFFSET_RNGP		0x006a
#define BBM_SYNC_CFOFFSET_RNGM		0x006b
#define BBM_SYNC_CFOFFSET_TGT			0x006c
#define BBM_SYNC_FIC_CNTRL			0x006d
#define BBM_SYNC_DET_CNTRL			0x0070
#define BBM_SYNC_DET_ACC_PERIOD		0x0071
#define BBM_SYNC_DET_MAX_THRL			0x0072
#define BBM_SYNC_DET_MAX_THRH			0x0073
#define BBM_SYNC_DET_MAX_MAGL			0x0074
#define BBM_SYNC_DET_MAX_MAGH			0x0075
#define BBM_SYNC_DET_MEAN_MAGL		0x0076
#define BBM_SYNC_DET_MEAN_MAGH		0x0077
#define BBM_SYNC_DET_STATUS			0x0078
#define BBM_SYNC_DET_DONECNT			0x0079
#define BBM_SYNC_DET_OKCNT			0x007A
#define BBM_SYNC_DET_MODE_ENABLE		0x007B

#define BBM_SYNC_NSBLK				0x0090
#define BBM_SYNC_FTOFFSET			0x0092
#define BBM_SYNC_FT_RANGE			0x0094
#define BBM_SYNC_CNTRL				0x0096
#define BBM_SYNC_STATUS			0x0098
#define BBM_SYNC_AMD_RANGE		0x009a
#define BBM_SYNC_ERR_THRESH		0x009e
#define BBM_SYNC_LOOP				0x009f
#define BBM_SYNC_WINDOW			0x00a0
#define BBM_SYNC_NONZERO			0x00a1
#define BBM_SYNC_LOOP_OUT			0x00a2
#define BBM_SYNC_TUNER				0x00a4
#define BBM_SYNC_TII_CTRL			0x00a5
#define BBM_SYNC_MIN_THRESH		0x00a6
#define BBM_SYNC_MAINID1			0x00a7
#define BBM_SYNC_SUBID1			0x00a8
#define BBM_SYNC_MAINID2			0x00a9
#define BBM_SYNC_SUBID2			0x00aa
#define BBM_SYNC_ID1_POWER			0x00ab
#define BBM_SYNC_ID2_POWER			0x00ac
#define BBM_SYNC_TII_THRESH			0x00ad
#define BBM_SYNC_MODE_TARGET			0x00ae
#define BBM_SYNC_SYNC_TEST_SEL		0x00af

/* AGC */
#define BBM_AGC_REFGAIN			0x0110
#define BBM_AGC_PERIOD				0x0111
#define BBM_AGC_GAIN_EXP			0x0113
#define BBM_AGC_GAIN_FRP			0x0114
#define BBM_AGC_RMS				0x0116
#define BBM_AGC_MTH				0x0117
#define BBM_AGC_Q_LEVEL			0x0118
#define BBM_AGC_UPDATE_VAL		0x0119
#define BBM_AGC_FIXED_ON			0x011b
#define BBM_AGC_EXP_FIXED			0x011c
#define BBM_AGC_FR_FIXED			0x011d
#define BBM_AGC_CTRL				0x011f

/* FFT */
#define BBM_FFT_SCALEV_FFT			0x0120
#define BBM_FFT_SCALEV_IFFT			0x0122
#define BBM_FFT_ADC_CONTROL			0x0128
#define BBM_FFT_MODEM_STSL			0x0129
#define BBM_FFT_MODEM_STSH			0x012A
#define BBM_FFT_PAD_DRIVING_SEL			0x012B

/* TII */
#define BBM_TII_IF_EN				0x0130
#define BBM_TII_DATA				0x0131
#define BBM_TII_DATA_LEN			0x0132

/* DIDP */
#define BBM_DIDP_CH_EN				0x0150
#define BBM_DIDP_MODE				0x0151
#define BBM_DIDP_CH0_SUBCH			0x0152
#define BBM_DIDP_CH1_SUBCH			0x0153
#define BBM_DIDP_CH2_SUBCH			0x0154
#define BBM_DIDP_CH3_SUBCH			0x0155
#define BBM_DIDP_CH4_SUBCH			0x0156
#define BBM_DIDP_CH5_SUBCH			0x0157
#define BBM_DIDP_CH6_SUBCH			0x0158
#define BBM_DIDP_CH7_SUBCH			0x0159
#define BBM_DIDP_POWER_OPT0			0x015a
#define BBM_DIDP_ADD_N_SHIFT0			0x015b
#define BBM_DIDP_POWER_OPT1			0x015c
#define BBM_DIDP_ADD_N_SHIFT1			0x015d
#define BBM_DIDP_POWER_OPT2			0x015e
#define BBM_DIDP_ADD_N_SHIFT2			0x015f

/* VT */
#define BBM_VT_BER_PERIOD			0x0210
#define BBM_VT_ERROR_SUM			0x0214
#define BBM_VT_RT_BER_PERIOD		0x0218
#define BBM_VT_RT_ERROR_SUM		0x021c
#define BBM_VT_CONTROL				0x0220

/* FIC */
#define BBM_FIC_CRC_CONTROL		0x0222
#define BBM_FIC_ERR_SUM			0x0223

/* CDI */
#define BBM_CDI0_SUBCH_EN			0x0224
#define BBM_CDI1_SUBCH_EN			0x0225
#define BBM_CDI0_COUNT				0x0226
#define BBM_CDI1_COUNT				0x0227
#define BBM_CDI0_ERROR				0x0228
#define BBM_CDI1_ERROR				0x0229
#define BBM_CDI_SYNC_PATTERN		0x022a
#define BBM_CDI_CONTROL			0x022b

/* RS */
#define BBM_RS_BER_PERIOD			0x022c
#define BBM_RS_FAIL_COUNT			0x022e
#define BBM_RS_ERR_SUM				0x0230
#define BBM_RS_RT_BER_PER			0x0234
#define BBM_RS_RT_FAIL_CNT			0x0236
#define BBM_RS_RT_ERR_SUM			0x0238
#define BBM_RS_CONTROL				0x023e

/* BUF */
#define BBM_BUF_STATUS				0x0250
#define BBM_BUF_OVERRUN			0x0252
#define BBM_BUF_ENABLE				0x0254
#define BBM_BUF_INT				0x0256
#define BBM_BUF_STS_CTRL			0x0258
#define BBM_BUF_STS_CLK_DIV		0x0259
#define BBM_BUF_STS_CHID			0x025a
#define BBM_BUF_CLOCK_EN			0x025b
#define BBM_BUF_MISC_CTRL			0x025c
#define BBM_BUF_TEST_MODE			0x025d
#define BBM_BUF_TEST_SIGNAL		0x025e
#define BBM_BUF_CH0_SUBCH			0x0260
#define BBM_BUF_CH1_SUBCH			0x0261
#define BBM_BUF_CH2_SUBCH			0x0262
#define BBM_BUF_CH3_SUBCH			0x0263
#define BBM_BUF_CH4_SUBCH			0x0264
#define BBM_BUF_CH5_SUBCH			0x0265
#define BBM_BUF_CH6_SUBCH			0x0266
#define BBM_BUF_CH7_SUBCH			0x0267
#define BBM_BUF_CH0_START			0x0268
#define BBM_BUF_CH1_START			0x026a
#define BBM_BUF_CH2_START			0x026c
#define BBM_BUF_CH3_START			0x026e
#define BBM_BUF_CH4_START			0x0270
#define BBM_BUF_CH5_START			0x0272
#define BBM_BUF_CH6_START			0x0274
#define BBM_BUF_CH7_START			0x0276
#define BBM_BUF_FIC_START			0x0278
#define BBM_BUF_CH0_END			0x0290
#define BBM_BUF_CH1_END			0x0292
#define BBM_BUF_CH2_END			0x0294
#define BBM_BUF_CH3_END			0x0296
#define BBM_BUF_CH4_END			0x0298
#define BBM_BUF_CH5_END			0x029a
#define BBM_BUF_CH6_END			0x029c
#define BBM_BUF_CH7_END			0x029e
#define BBM_BUF_FIC_END			0x02a0
#define BBM_BUF_CH0_THR			0x02a2
#define BBM_BUF_CH1_THR			0x02a4
#define BBM_BUF_CH2_THR			0x02a6
#define BBM_BUF_CH3_THR			0x02a8
#define BBM_BUF_CH4_THR			0x02aa
#define BBM_BUF_CH5_THR			0x02ac
#define BBM_BUF_CH6_THR			0x02ae
#define BBM_BUF_CH7_THR			0x02b0
#define BBM_BUF_FIC_THR			0x02b2

/* I2C */
#define BBM_I2C_PR				0x0310
#define BBM_I2C_CTR			0x0312
#define BBM_I2C_RXR			0x0313
#define BBM_I2C_SR				0x0314
#define BBM_I2C_TXR			0x0315
#define BBM_I2C_CR				0x0316

#define BBM_TS_PAUSE			0x0378
#define BBM_TS_SELECT			0x037A

/* SCI (PL131) */
/* 0x000  SCIDATA       ,SCI Data register */
#define	BBM_SCI_DATA				0x0390
/* 0x004  SCICR0        ,SCI Control register 0 */
#define	BBM_SCI_CR0					0x0392
/* 0x008  SCICR1        ,SCI Control register 1 */
#define	BBM_SCI_CR1					0x0394
/* 0x00C  SCICR2        ,SCI Control register 2 */
#define	BBM_SCI_CR2					0x0396
/* 0x010  SCICLKICC     ,SCI Smart card clock frequency */
#define	BBM_SCI_CLKICC				0x0398
/* 0x014  SCIVALUE      ,SCI Baud cycles time register */
#define	BBM_SCI_VALUE				0x039a
/* 0x018  SCIBAUD       ,SCI Baud rate clock time  */
#define	BBM_SCI_BAUD				0x039c
/* 0x01C  SCITIDE       ,SCI Tx and Rx Tide mark  */
#define	BBM_SCI_TIDE				0x039e
/* 0x020  SCIDMACR      ,SCI Direct Memory Access control register */
#define	BBM_SCI_DMACR				0x03a0
/* 0x024  SCISTABLE     ,SCI Debounce time register  */
#define	BBM_SCI_STABLE				0x03a2
/* 0x028  SCIATIME      ,SCI card activation event time register */
#define	BBM_SCI_ATIME				0x03a4
/* 0x02C  SCIDTIME      ,SCI card deactivation event time register */
#define	BBM_SCI_DTIME				0x03a6
/* 0x030  SCIATRSTIME   ,SCI ATR start time register */
#define	BBM_SCI_ATRSTIME			0x03a8
/* 0x034  SCIATRDTIME   ,SCI ATR duration time register */
#define	BBM_SCI_ATRDTIME			0x03aa
/* 0x038  SCISTOPTIME   ,SCI Duration before Card Clk can be stopped */
#define	BBM_SCI_STOPTIME			0x03ac
/* 0x03C  SCISTARTTIME  ,SCI Duration before Card Clk can be re-started */
#define	BBM_SCI_STARTTIME			0x03ae
/* 0x040  SCIRETRY      ,SCI Tx and Rx Retry register */
#define	BBM_SCI_RETRY				0x03b0
/* 0x044  SCICHTIMELS   ,SCI Char to char timeout timeout least sig. */
#define	BBM_SCI_CHTIMELS			0x03b2
/* 0x048  SCICHTIMEMS   ,SCI Char to char timeout timeout most  sig. */
#define	BBM_SCI_CHTIMEMS			0x03b4
/* 0x04C  SCIBLKTIMELS  ,SCI Receive timeout between blocks least sig. */
#define	BBM_SCI_BLKTIMELS			0x03b6
/* 0x050  SCIBLKTIMEMS  ,SCI Receive timeout between blocks most  sig. */
#define	BBM_SCI_BLKTIMEMS			0x03b8
/* 0x054  SCICHGUARD    ,SCI Character guard time register */
#define	BBM_SCI_CHGUARD				0x03ba
/* 0x058  SCIBLKGUARD   ,SCI Block guard time register */
#define	BBM_SCI_BLKGUARD			0x03bc
/* 0x05C  SCIRXTIME     ,SCI RX read timeout register  */
#define	BBM_SCI_RXTIME				0x03be
/* 0x060  SCIFIFOSTATUS ,SCI TX and RX FIFO Status */
#define	BBM_SCI_FIFOSTATUS			0x03d0
/* 0X064  SCITXCOUNT    ,SCI TX FIFO fill level  */
#define	BBM_SCI_TXCOUNT				0x03d2
/* 0x068  SCIRXCOUNT    ,SCI RX FIFO fill level */
#define	BBM_SCI_RXCOUNT				0x03d4
/* 0x06C  SCIIMSC       ,SCI Interrupt mask set or clear register  */
#define	BBM_SCI_IMSC				0x03d6
/* 0x070  SCIRIS        ,SCI Raw interrupt status register */
#define	BBM_SCI_RIS					0x03d8
/* 0x074  SCIMIS        ,SCI Masked interrupt status register */
#define	BBM_SCI_MIS					0x03da
/* 0x078  SCIICR        ,SCI Interrupt clear register */
#define	BBM_SCI_ICR					0x03dc
/* 0x07C  SCISYNCACT    ,SCI Synchronous mode Activation register */
#define	BBM_SCI_SYNCACT				0x03de
/* 0x080  SCISYNCTX     ,SCI Synchronous mode transmit register */
#define	BBM_SCI_SYNCTX				0x03e0
/* 0x084  SCISYNCRX     ,SCI Synchronous mode receive register */
#define	BBM_SCI_SYNCRX				0x03e2

/* ----------------------------------------------------
//	BUFFER MANAGEMENT
//---------------------------------------------------- */
#define FIC_BUF_START	0x0000
#define FIC_BUF_LENGTH	(32*24)
#define FIC_BUF_END	(FIC_BUF_START + FIC_BUF_LENGTH - 1)
#define FIC_BUF_THR	(FIC_BUF_LENGTH/2-1)

#define CH0_BUF_START	(FIC_BUF_START + FIC_BUF_LENGTH)
#define CH0_BUF_LENGTH	(188*40*2)
#define CH0_BUF_END	(CH0_BUF_START + CH0_BUF_LENGTH - 1)
#define CH0_BUF_THR	(CH0_BUF_LENGTH/2-1)

#define CH1_BUF_START	(FIC_BUF_START + FIC_BUF_LENGTH)
#define CH1_BUF_LENGTH	(0)
#define CH1_BUF_END	(CH1_BUF_START + CH1_BUF_LENGTH - 1)
#define CH1_BUF_THR	(0)

#define CH2_BUF_START	(FIC_BUF_START + FIC_BUF_LENGTH)
#define CH2_BUF_LENGTH	(128*6)
#define CH2_BUF_END	(CH2_BUF_START + CH2_BUF_LENGTH - 1)
#define CH2_BUF_THR	(CH2_BUF_LENGTH/2-1)

#define CH3_BUF_START	(FIC_BUF_START + FIC_BUF_LENGTH)
#define CH3_BUF_LENGTH	(188*20*2)
#define CH3_BUF_END	(CH3_BUF_START + CH3_BUF_LENGTH - 1)
#define CH3_BUF_THR	(CH3_BUF_LENGTH/2-1)

#ifdef __cplusplus
}
#endif

#endif	/* __FC8050_REGS_H__ */
