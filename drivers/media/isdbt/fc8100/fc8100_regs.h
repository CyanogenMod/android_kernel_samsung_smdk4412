/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fc8100_regs.h

 Description : RF & baseband registermap  header file

 History :
 ----------------------------------------------------------------------
 2009/09/29	bruce		initial
*******************************************************************************/

#ifndef __FC8100_REGS_H__
#define __FC8100_REGS_H__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------
	FC8100 COMMON DEFINTION
----------------------------------------------------*/
#define BER_PACKET_SIZE 1  /* // 256 PACKET PERIOD */


/*----------------------------------------------------
	FC8100 RF REGISTER MAP
----------------------------------------------------*/

/* // -- CONTROL -- */
#define RF_MODULE_RESET			0x00
#define RF_TUNER_CHIP_ID                   0x01
#define RF_MODE_CTL			              0x02
#define RF_MANUAL_CTL                        0x03
#define RF_STBY_ACTIVE1                     0x04
#define RF_STBY_ACTIVE2                     0x05
#define RF_STBY_ACTIVE3                     0x06
#define RF_STBY_ACTIVE4                     0x07

/* // -- VCO -- */
#define RF_VCO_MODE                            0x08
#define RF_LO_BIAS_MODE1			0x09
#define RF_LO_BIAS_MODE2                  0x0A

/* // -- PLL,  LNA&Mixer,  CSF, Output Buffer, Bias Top --  */
#define RF_BIAS_TOP_CTRL			0x0B

/* //-- PGA, ADC, AGC, Revision Number, LNA, Common Bias, VGA -- */
#define RF_LNA_ELNA_Control			0x0C
#define RF_PLL_MODE1			       0x0D
#define RF_PLL_CTRL1				0x0E
#define RF_ELNA_CONTROL_OFF		0x0F
#define RF_PLL_CTRL3			0x10
#define RF_ELNA_CONTROL_ON		0x11
#define RF_PLL_CTRL4			       0x12
#define RF_PLL_CTRL5			       0x13
#define RF_PLL_CTRL6			       0x14
#define RF_PLL_CTRL7			       0x15
#define RF_PLL_K2				0x16
#define RF_NTARGET_VALUE	       0x17
#define RF_PLL_K1			              0x18
#define RF_PLL_K0			              0x19
#define RF_PLL_N			                     0x1A
#define RF_ELNA_CONTROL1		       0x1B
#define RF_ELNA_CONTROL2		       0x1C
#define RF_PLL_CTRL10			       0x1D
#define RF_PLL_CTRL11			       0x1E
#define RF_DMB_BANK_MONITOR		0x1F
#define RF_LOCK_CTRL0				0x20
#define RF_LOCK_CTRL1				0x21
#define RF_BS_CTRL1				0x22
#define RF_BS_CTRL2				0x23
#define RF_BS_CTRL3				0x24
#define RF_BS_CTRL4				0x25
#define RF_BS_MON1				0x26
#define RF_BS_MON2				0x27
#define RF_LNAMIX_CTL				0x28
#define RF_LNA_ICTRL				0x29
#define RF_LNAMIX_ICTRL1		0x2A
#define RF_AGC_ELNA				0x2B
#define RF_LNAMIX_ICTRL2			0x2C
#define RF_LNAMIX_ICTRL3		0x2D
#define RF_LNAMIX_ICTRL4			0x2E
#define RF_LNAMIX_ICTRL5			0x2F
#define RF_LNAMIX_ICTRL_MON			0x30
#define RF_ELNA_GAIN				0x31
#define RF_CSF_MODE				0x32
#define RF_CSF_CAPTUNE_MON1		0x33
#define RF_CSF_CAPTUNE_MON2		0x34
#define RF_CSFA_I2C_CRNT			0x35
#define RF_CSF_I2C_CORE_CRNT_AH	0x36
#define RF_CSF_I2C_OUT_CRNT_AH	0x37
#define RF_CSF_RX_CRNT				0x38
#define RF_CSF_PRE_CUR				0x39
#define RF_CSF_STATE				0x3A
#define RF_ELNA_CONTROL3			0x3B
#define RF_CSF_CAL_CRNT			0x3C
#define RF_CSF_CF_CLK_DIV_LSB		0x3D
#define RF_CSF_CLK_DIV_LSB			0x3E
#define RF_CSF_AUTO_RECAL_PERIOD	0x3F
#define RF_CSF_CAL_MAN1			0x40
#define RF_CSF_CAL_MAN2			0x41
#define RF_CSF_AGC_CRNT_LOW		0x42
#define RF_BG_CTRL0				0x43
#define RF_PGA_1ST_STATE_CUR		0x44
#define RF_PGA_2ND_STATE_CUR	0x45
#define RF_ELNA_CONTROL4	              0x46
#define RF_PGA_MODE			       0x47
#define RF_AGC_COMP_BIAS			0x48
#define RF_W_FACTOR1			       0x49
#define RF_ADC_BIAS1			       0x4A
#define RF_ADC1_BOUT				0x4B
#define RF_W_FACTOR2				0x4C
#define RF_ADC_BIAS2			       0x4D
#define RF_ADC2_BOUT			       0x4E
#define RF_ADC_BIAS3                           0x4F
#define RF_ADC3_BOUT			       0x50
#define RF_ADC4_BOUT			       0x51
#define RF_RFAGC_MODE			       0x52
#define RF_RFAGC_MODE2				0x53
#define RF_IFAGC_MODE1			       0x54
#define RF_RFAGC_TEST_MODE1		0x55
#define RF_RFAGC_TEST_MODE2	       0x56
#define RF_IFAGC_TEST_MODE1		0x57
#define RF_IFAGC_TEST_MODE2		0x58
#define RF_PD1_MAX				0x59
#define RF_PD1_MIN			       0x5A
#define RF_PD2_MAX			       0x5B
#define RF_PD2_MIN				0x5C
#define RF_AGC_CLK_MODE		       0x5D
#define RF_RFAGC_WAIT_CLK_LNA1      0x5F
#define RF_RFAGC_WAIT_CLK_LNA2	0x60
#define RF_RFAGC_WAIT_CLK_RFVGA    0x61
#define RF_RFAGC_WAIT_CLK_PD_CHANGE       0x62
#define RF_RFAGC_WAIT_CLK_STDBY		     0x63
#define RF_RFAGC_WAIT_CLK_HOLDAGC           0x64
#define RF_IFAGC_WAIT_CLK_FILTER                0x65
#define RF_IFAGC_WAIT_CLK_STBY                   0x66
#define RF_IFAGC_WAIT_CLK_HOLD_OFF          0x67
#define RF_PGA_AGC_STEP                    0x68
#define RF_RFVGA_L0_MAX                    0x69
#define RF_RFVGA_L0_MIN				0x6A
#define RF_RFVGA_L1_MAX				0x6B
#define RF_RFVGA_L1_MIN				0x6C
#define RF_RFVGA_L2_MAX				0x6D
#define RF_RFVGA_L2_MIN				0x6E
#define RF_RFVGA_L3_MAX				0x6F
#define RF_RFVGA_L3_MIN				0x70
#define RF_RFVGA_L4_MAX				0x71
#define RF_RFVGA_L4_MIN				0x72
#define RF_RFVGA_L5_MAX				0x73
#define RF_RFVGA_L5_MIN				0x74
#define RF_RFVGA_L6_MAX				0x75
#define RF_RFVGA_L6_MIN				0x76
#define RF_IFAGC_PGA_MAX		       0x77
#define RF_IFAGC_PGA_MIN		       0x78
#define RF_STATE_MONITOR1		       0x79
#define RF_STATE_MONITOR2			0x7A
#define RF_STATE_MONITOR3			0x7B
#define RF_STATE_MONITOR4			0x7C
#define RF_AGC_PD_OFFSET			0x7D
#define RF_AGC_SH_SL				0x7E
#define RF_REVISION_NUMBER	       0x7F


/* ----------------------------------------------------
	FC8100 BB REGISTER MAP
----------------------------------------------------*/

/* // 1. System Control */
#define BBM_VERSION				       0x00
#define BBM_SYSRST			              0x01

/* // 2. Serial Bus Control */
#define BBM_SBCTRL			              0x02
#define BBM_SBADDR				       0x03
#define BBM_INCTRL		                     0x05
#define BBM_IOMODE			              0x07
#define BBM_OUTCTRL0			       0x08
#define BBM_OUTCTRL1				0x09
#define BBM_PLL0			              0x0C
#define BBM_PLL1			              0x0D
#define BBM_PLL2			              0x0E

/* // 3. INTR */
#define BBM_INTRPT0				       0x11
#define BBM_INTRPT1				       0x12
#define BBM_INTSEL0			       0x14
#define BBM_INTSEL1			              0x15
#define BBM_INTRST0			       0x17
#define BBM_INTRST1			              0x18
#define BBM_RSINTR			              0x19
#define BBM_INTFORM			       0x1C

/* // 4. LOCK */
#define BBM_LOCKF			              0x1D
#define BBM_GIVEUP			              0x1E

/* // 5. TMCC */
#define BBM_TMCRNT0                            0x20
#define BBM_TMCRNT1                            0x21
#define BBM_TMCRNT2                            0x22
#define BBM_TMNEXT0                            0x26
#define BBM_TMNEXT1                            0x27
#define BBM_TMMAND0                           0x2B
#define BBM_TMMAND1                              0x2C
#define BBM_TMMNSW                            0x30
#define BBM_TMFECF                              0x31

/* // 7. MONITOR */
#define BBM_STATEF                              0x32
#define BBM_DCDF                                  0x33
#define BBM_INPWR0                              0x34
#define BBM_INPWR1                              0x35
#define BBM_AGADAT                             0x36
#define BBM_EQIDAT0                            0x38
#define BBM_EQIDAT1                            0x39
#define BBM_EQQDAT0                           0x3A
#define BBM_EQQDAT1                           0x3B

/* // 8. AGC Setup */
#define BBM_AGTIME                             0x40
#define BBM_AGAVCNT                           0x41
#define BBM_AGTG                                 0x42
#define BBM_AGSG                                 0x43
#define BBM_AGAINI                              0x44
#define BBM_AGREF0                             0x46
#define BBM_AGREF1                             0x47
#define BBM_AGCTRL                             0x4D

/* // 9. GPIO */
#define BBM_GPIOSEL0                          0x59
#define BBM_GPIOSEL1                          0x5A
#define BBM_GPI                                    0x5B
#define BBM_GPO                                   0x5C
#define BBM_PWMSET                            0x5D
#define BBM_PWMLV0                            0x5E
#define BBM_PWMLV1                            0x5F

/* // 10. Read-Solomon decoder/error counter */
#define BBM_RSERST                             0x60
#define BBM_RSECNT0                           0x61
#define BBM_RSECNT1                           0x62
#define BBM_RSECNT2                           0x63
#define BBM_RSTCNT0                           0x64
#define BBM_RSTCNT1                           0x65
#define BBM_RSEPER                             0x70

/* // 11. Block through */
#define BBM_THRUFD                            0x73
#define BBM_THRUBD                            0x74
#define BBM_PREVTO                            0x76
#define BBM_THRURS                            0x77
#define BBM_THRUDR                            0x78

/* // 12. Symbo synchronization */
#define BBM_SYTIME                             0xA0
#define BBM_SYATTH                             0xA1
#define BBM_SYMNTH0                          0xA2
#define BBM_SYMNTH1                          0xA3
#define BBM_SYSW1                              0xA4
#define BBM_SYSW2                              0xA5
#define BBM_SYCTRL1                           0xA6
#define BBM_SYMODE1                          0xA7
#define BBM_SYMODE2                          0xA8
#define BBM_SYMODE3                          0xA9

/* // 13. Digital AGC */
#define BBM_DAM2REF0                        0xAA
#define BBM_DAM2REF1                        0xAB
#define BBM_DAM3REF0                        0xAC
#define BBM_DAM3REF1                        0xAD
#define BBM_DAAVCNT                          0xAE
#define BBM_DALPG                               0xAF

/* // 14. FFT */
#define BBM_FTCTRL                             0xB0

/* // 15. TMCC Decoder */
#define BBM_TMCTRL0                           0xBC
#define BBM_TMCTRL1                           0xBD
#define BBM_TMCTRL2                           0xBE
#define BBM_EQCTRL                             0xC0
#define BBM_EQCONF                            0xC1
#define BBM_PRBSEL                             0xC3
#define BBM_GNFCNT1                          0xC4

/* // 17. IFFT */
#define BBM_GNFSNTH                          0xC5
#define BBM_GNFCNT2                          0xC7
#define BBM_WINPTMN                         0xC8
#define BBM_IFCTRL                              0xC9
#define BBM_WINCTRL                          0xCA
#define BBM_WINCTRL1                        0xCB
#define BBM_WINCTRL2_1                    0xCC
#define BBM_WINCTRL2_2                    0xCD
#define BBM_WINCTRL2_3                    0xCE

/* // 18. TIME RANGE UNLOCK DETECTION */
#define BBM_IFUN                                0xCF

/* // 19. Status readout */
#define BBM_FRLKCNT0                        0xD0
#define BBM_FRLKCNT1                        0xD1
#define BBM_SYLKCNT0                        0xD2
#define BBM_SYLKCNT1                        0xD3
#define BBM_MRLKCNT0                        0xD4
#define BBM_MRLKCNT1                        0xD5
#define BBM_ULSTAT                            0xD6
#define BBM_SELGNF                            0xD7
#define BBM_ERRPWR1                         0xD8
#define BBM_ERRPWR2                         0xD9

/* // 20. GIC */
#define BBM_GICCNT1                           0xE7
#define BBM_GICCNT2                           0xE8
#define BBM_WINTHTR                          0xE9
#define BBM_GICOFF                             0xEA

/* // 21. Antenna switching */
#define BBM_AWCTRL                           0xF0
#define BBM_AWSWTH                          0xF1
#define BBM_AWONTH                          0xF2
#define BBM_AWOFFTH                        0xF3
#define BBM_AWAGCTH                        0xF4
#define BBM_AWAVCE                           0xF5

/* // 22. SI */
#define BBM_EQSI1                               0xF6
#define BBM_EQSI2                               0xF7


#ifdef __cplusplus
}
#endif

#endif
