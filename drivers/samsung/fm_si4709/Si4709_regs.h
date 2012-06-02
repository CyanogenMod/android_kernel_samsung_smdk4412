#ifndef _Si4709_REGS_H
#define _Si4709_REGS_H

#define NUM_OF_REGISTERS		0x10

/*Si4709 registers*/
#define    DEVICE_ID			0x00
#define    CHIP_ID			0x01
#define    POWERCFG			0x02
#define    CHANNEL			0x03
#define    SYSCONFIG1			0x04
#define    SYSCONFIG2			0x05
#define    SYSCONFIG3			0x06
#define    TEST1			0x07
#define    TEST2			0x08
#define    BOOTCONFIG			0x09
#define    STATUSRSSI			0x0A
#define    READCHAN			0x0B
#define    RDSA				0x0C
#define    RDSB				0x0D
#define    RDSC				0x0E
#define    RDSD				0x0F

/***********POWERCFG************/
#define POWERCFG_DSMUTE			0x8000
#define POWERCFG_DMUTE			0x4000
#define POWERCFG_MONO			0x2000
#define POWERCFG_RDSM			0x0800
#define POWERCFG_SKMODE			0x0400
#define POWERCFG_SEEKUP			0x0200
#define POWERCFG_SEEK			0x0100
#define POWERCFG_DISABLE		0x0040
#define POWERCFG_ENABLE			0x0001
/************************************/

/***********CHANNEL************/
#define CHANNEL_TUNE			0x8000
/************************************/

/***********SYSCONFIG1************/
#define SYSCONFIG1_RDSIEN		0x8000
#define SYSCONFIG1_STCIEN		0x4000
#define SYSCONFIG1_RDS			0x1000
#define SYSCONFIG1_DE			0x0800
#define SYSCONFIG1_AGCD			0x0400
#define SYSCONFIG1_BLNDADJ1		0x0080
#define SYSCONFIG1_BLNDADJ0		0x0040
#define SYSCONFIG1_GPO1			0x0008
#define SYSCONFIG1_GPO0			0x0004
/************************************/

/***********SYSCONFIG2************/
#define SYSCONFIG2_BAND1		0x0080
#define SYSCONFIG2_BAND0		0x0040
#define SYSCONFIG2_SPACE1		0x0020
#define SYSCONFIG2_SPACE0		0x0010
#define SYSCONFIG2_VOLUME3		0x0008
#define SYSCONFIG2_VOLUME2		0x0004
#define SYSCONFIG2_VOLUME1		0x0002
#define SYSCONFIG2_VOLUME0		0x0001
/************************************/

/***********SYSCONFIG3************/
#define SYSCONFIG3_SMUTER1		0x8000
#define SYSCONFIG3_SMUTER0		0x4000
#define SYSCONFIG3_SMUTEA1		0x2000
#define SYSCONFIG3_SMUTEA0		0x1000
#define SYSCONFIG3_VOLEXT		0x0100
#define SYSCONFIG3_SKSNR3		0x0080
#define SYSCONFIG3_SKSNR2		0x0040
#define SYSCONFIG3_SKSNR1		0x0020
#define SYSCONFIG3_SKSNR0		0x0010
#define SYSCONFIG3_SKCNT3		0x0008
#define SYSCONFIG3_SKCNT2		0x0004
#define SYSCONFIG3_SKCNT1		0x0002
#define SYSCONFIG3_SKCNT0		0x0001
/************************************/

/***********TEST1************/
#define TEST1_AHIZEN			0x4000
/************************************/

/***********STATUSRSSI************/
#define STATUSRSSI_RDSR			0x8000
#define STATUSRSSI_STC			0x4000
#define STATUSRSSI_SF_BL		0x2000
#define STATUSRSSI_AFCRL		0x1000
#define STATUSRSSI_RDSS			0x0800
#define STATUSRSSI_BLERA1		0x0400
#define STATUSRSSI_BLERA0		0x0200
#define STATUSRSSI_ST			0x0100
/************************************/

/***********READCHAN************/
#define READCHAN_BLERB1			0x8000
#define READCHAN_BLERB0			0x4000
#define READCHAN_BLERC1			0x2000
#define READCHAN_BLERC0			0x1000
#define READCHAN_BLERD1			0x0800
#define READCHAN_BLERD0			0x0400

#define READCHAN_CHAN_MASK		0x03FF
/************************************/

/*************************************************************/
static inline void switch_on_bits(u16 *data, u16 bits_on)
{
	*data |= bits_on;
}

static inline void switch_off_bits(u16 *data, u16 bits_off)
{
	u16 aux = 0xFFFF;
	aux ^= bits_off;
	*data &= aux;
}

#define BIT_ON				1
#define BIT_OFF				0

static inline int check_bit(u16 data, u16 bit)
{
	return data & bit ? BIT_ON : BIT_OFF;
}

/**************************************************************/

/********************************************************************/
static inline void POWERCFG_BITSET_ENABLE_HIGH(u16 *data)
{
	switch_on_bits(data, (POWERCFG_ENABLE));
}

static inline void POWERCFG_BITSET_DISABLE_HIGH(u16 *data)
{
	switch_on_bits(data, (POWERCFG_DISABLE));
}

static inline void POWERCFG_BITSET_DISABLE_LOW(u16 *data)
{
	switch_off_bits(data, (POWERCFG_DISABLE));
}

static inline void POWERCFG_BITSET_DMUTE_HIGH(u16 *data)
{
	switch_on_bits(data, (POWERCFG_DMUTE));
}

static inline void POWERCFG_BITSET_DMUTE_LOW(u16 *data)
{
	switch_off_bits(data, (POWERCFG_DMUTE));
}

static inline void POWERCFG_BITSET_DSMUTE_HIGH(u16 *data)
{
	switch_on_bits(data, (POWERCFG_DSMUTE));
}

static inline void POWERCFG_BITSET_DSMUTE_LOW(u16 *data)
{
	switch_off_bits(data, (POWERCFG_DSMUTE));
}

static inline void POWERCFG_BITSET_MONO_HIGH(u16 *data)
{
	switch_on_bits(data, (POWERCFG_MONO));
}

static inline void POWERCFG_BITSET_MONO_LOW(u16 *data)
{
	switch_off_bits(data, (POWERCFG_MONO));
}

static inline void POWERCFG_BITSET_RDSM_HIGH(u16 *data)
{
	switch_on_bits(data, (POWERCFG_RDSM));
}

static inline void POWERCFG_BITSET_RDSM_LOW(u16 *data)
{
	switch_off_bits(data, (POWERCFG_RDSM));
}

static inline void POWERCFG_BITSET_SKMODE_HIGH(u16 *data)
{
	switch_on_bits(data, (POWERCFG_SKMODE));
}

static inline void POWERCFG_BITSET_SKMODE_LOW(u16 *data)
{
	switch_off_bits(data, (POWERCFG_SKMODE));
}

static inline void POWERCFG_BITSET_SEEKUP_HIGH(u16 *data)
{
	switch_on_bits(data, (POWERCFG_SEEKUP));
}

static inline void POWERCFG_BITSET_SEEKUP_LOW(u16 *data)
{
	switch_off_bits(data, (POWERCFG_SEEKUP));
}

static inline void POWERCFG_BITSET_SEEK_HIGH(u16 *data)
{
	switch_on_bits(data, (POWERCFG_SEEK));
}

static inline void POWERCFG_BITSET_SEEK_LOW(u16 *data)
{
	switch_off_bits(data, (POWERCFG_SEEK));
}

static inline void POWERCFG_BITSET_RESERVED(u16 *data)
{
	*data &= 0xEF41;
}

/********************************************************************/

static inline void CHANNEL_BITSET_TUNE_HIGH(u16 *data)
{
	switch_on_bits(data, (CHANNEL_TUNE));
}

static inline void CHANNEL_BITSET_TUNE_LOW(u16 *data)
{
	switch_off_bits(data, (CHANNEL_TUNE));
}

static inline void CHANNEL_BITSET_RESERVED(u16 *data)
{
	*data &= 0x83FF;
}

/********************************************************************/

static inline void SYSCONFIG1_BITSET_RDSIEN_HIGH(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG1_RDSIEN));
}

static inline void SYSCONFIG1_BITSET_RDSIEN_LOW(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG1_RDSIEN));
}

static inline void SYSCONFIG1_BITSET_STCIEN_HIGH(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG1_STCIEN));
}

static inline void SYSCONFIG1_BITSET_STCIEN_LOW(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG1_STCIEN));
}

static inline void SYSCONFIG1_BITSET_RDS_HIGH(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG1_RDS));
}

static inline void SYSCONFIG1_BITSET_RDS_LOW(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG1_RDS));
}

static inline void SYSCONFIG1_BITSET_DE_50(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG1_DE));
}

static inline void SYSCONFIG1_BITSET_DE_75(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG1_DE));
}

static inline void SYSCONFIG1_BITSET_AGCD_HIGH(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG1_AGCD));
}

static inline void SYSCONFIG1_BITSET_AGCD_LOW(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG1_AGCD));
}

static inline void SYSCONFIG1_BITSET_RSSI_DEF_31_49(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG1_BLNDADJ1 | SYSCONFIG1_BLNDADJ0));
}

static inline void SYSCONFIG1_BITSET_RSSI_37_55(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG1_BLNDADJ0));
	switch_off_bits(data, (SYSCONFIG1_BLNDADJ1));
}

static inline void SYSCONFIG1_BITSET_RSSI_19_37(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG1_BLNDADJ1));
	switch_off_bits(data, (SYSCONFIG1_BLNDADJ0));
}

static inline void SYSCONFIG1_BITSET_RSSI_25_43(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG1_BLNDADJ1 | SYSCONFIG1_BLNDADJ0));
}

static inline void SYSCONFIG1_BITSET_GPIO_HIGH_IMP(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG1_GPO1 | SYSCONFIG1_GPO0));
}

static inline void SYSCONFIG1_BITSET_GPIO_STC_RDS_INT(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG1_GPO0));
	switch_off_bits(data, (SYSCONFIG1_GPO1));
}

static inline void SYSCONFIG1_BITSET_GPIO_LOW(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG1_GPO1));
	switch_off_bits(data, (SYSCONFIG1_GPO0));
}

static inline void SYSCONFIG1_BITSET_GPIO_HIGH(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG1_GPO1 | SYSCONFIG1_GPO0));
}

static inline void SYSCONFIG1_BITSET_RESERVED(u16 *data)
{
	*data &= 0xDCCC;
	*data |= 0x22;
}

/********************************************************************/

/*US/EUROPE (Default)*/
static inline void SYSCONFIG2_BITSET_BAND_87p5_108_MHz(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG2_BAND1 | SYSCONFIG2_BAND0));
}

/*Japan wide band*/
static inline void SYSCONFIG2_BITSET_BAND_76_108_MHz(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG2_BAND0));
	switch_off_bits(data, (SYSCONFIG2_BAND1));
}

/*Japan*/
static inline void SYSCONFIG2_BITSET_BAND_76_90_MHz(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG2_BAND1));
	switch_off_bits(data, (SYSCONFIG2_BAND0));
}

/*US, Australia (Default)*/
static inline void SYSCONFIG2_BITSET_SPACE_200_KHz(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG2_SPACE1 | SYSCONFIG2_SPACE0));
}

/*Europe, Japan*/
static inline void SYSCONFIG2_BITSET_SPACE_100_KHz(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG2_SPACE0));
	switch_off_bits(data, (SYSCONFIG2_SPACE1));
}

static inline void SYSCONFIG2_BITSET_SPACE_50_KHz(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG2_SPACE1));
	switch_off_bits(data, (SYSCONFIG2_SPACE0));
}

static inline void SYSCONFIG2_BITSET_VOLUME(u16 *data, u8 volume)
{
	*data &= 0xFFF0;
	*data |= (volume & 0x0F);
}

static inline void SYSCONFIG2_BITSET_SEEKTH(u16 *data, u8 seek_th)
{
	*data &= 0x00FF;
	*data |= ((seek_th << 8) & 0xFF00);
}

static inline u8 SYSCONFIG2_GET_VOLUME(u16 data)
{
	return (u8) (data & 0x000F);
}

/********************************************************************/

static inline void SYSCONFIG3_BITSET_SMUTER_FASTEST(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG3_SMUTER1 | SYSCONFIG3_SMUTER0));
}

static inline void SYSCONFIG3_BITSET_SMUTER_FAST(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG3_SMUTER1));
	switch_on_bits(data, (SYSCONFIG3_SMUTER0));
}

static inline void SYSCONFIG3_BITSET_SMUTER_SLOW(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG3_SMUTER0));
	switch_on_bits(data, (SYSCONFIG3_SMUTER1));
}

static inline void SYSCONFIG3_BITSET_SMUTER_SLOWEST(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG3_SMUTER1 | SYSCONFIG3_SMUTER0));
}

static inline void SYSCONFIG3_BITSET_SMUTEA_16_dB(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG3_SMUTEA1 | SYSCONFIG3_SMUTEA0));
}

static inline void SYSCONFIG3_BITSET_SMUTEA_14dB(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG3_SMUTEA1));
	switch_on_bits(data, (SYSCONFIG3_SMUTEA0));
}

static inline void SYSCONFIG3_BITSET_SMUTEA_12dB(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG3_SMUTEA0));
	switch_on_bits(data, (SYSCONFIG3_SMUTEA1));
}

static inline void SYSCONFIG3_BITSET_SMUTEA_10dB(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG3_SMUTEA1 | SYSCONFIG3_SMUTEA0));
}

static inline void SYSCONFIG3_BITSET_VOLEXT_DISB(u16 *data)
{
	switch_off_bits(data, (SYSCONFIG3_VOLEXT));
}

static inline void SYSCONFIG3_BITSET_VOLEXT_ENB(u16 *data)
{
	switch_on_bits(data, (SYSCONFIG3_VOLEXT));
}

static inline void SYSCONFIG3_BITSET_SKSNR(u16 *data, u8 seeksnr)
{
	*data &= 0xFF0F;
	*data |= ((seeksnr << 4) & 0xF0);
}

static inline void SYSCONFIG3_BITSET_SKCNT(u16 *data, u8 seekcnt)
{
	*data &= 0xFFF0;
	*data |= ((seekcnt) & 0x0F);
}

static inline void SYSCONFIG3_BITSET_RESERVED(u16 *data)
{
	*data &= 0xF1FF;
}

/********************************************************************/

static inline void TEST1_BITSET_AHIZEN_HIGH(u16 *data)
{
	switch_on_bits(data, (TEST1_AHIZEN));
}

static inline void TEST1_BITSET_AHIZEN_LOW(u16 *data)
{
	switch_off_bits(data, (TEST1_AHIZEN));
}

static inline void TEST1_BITSET_RESERVED(u16 *data)
{
	*data &= 0x7FFF;
}

/********************************************************************/

#define NEW_RDS_GROUP_READY		1
#define NO_RDS_GROUP_READY		0

#define COMPLETE			1
#define CLEAR				0

#define SEEK_SUCCESSFUL			1
#define SEEK_FAILURE_BAND_LMT_RCHD	0

#define AFC_RAILED			1
#define AFC_NOT_RAILED			0

#define RDS_DECODER_SYNCHRONIZED	1
#define RDS_DECODER_NOT_SYNCHRONIZED	0

#define STEREO				1
#define MONO				0

#define ERRORS_0			0
#define ERRORS_1_2			1
#define ERRORS_3_5			2
#define ERRORS_NO_CORREC_POSSIBLE_6_p	3

static inline int STATUSRSSI_RDS_READY_STATUS(u16 data)
{
	if (check_bit(data, STATUSRSSI_RDSR) == BIT_ON)
		return NEW_RDS_GROUP_READY;
	else
		return NO_RDS_GROUP_READY;
}

static inline int STATUSRSSI_SEEK_TUNE_STATUS(u16 data)
{
	if (check_bit(data, STATUSRSSI_STC) == BIT_ON)
		return COMPLETE;
	else
		return CLEAR;
}

static inline int STATUSRSSI_SF_BL_STATUS(u16 data)
{
	if (check_bit(data, STATUSRSSI_SF_BL) == BIT_ON)
		return SEEK_FAILURE_BAND_LMT_RCHD;
	else
		return SEEK_SUCCESSFUL;
}

static inline int STATUSRSSI_AFC_RAIL_STATUS(u16 data)
{
	if (check_bit(data, STATUSRSSI_AFCRL) == BIT_ON)
		return AFC_RAILED;
	else
		return AFC_NOT_RAILED;
}

static inline int STATUSRSSI_RDS_SYNC_STATUS(u16 data)
{
	if (check_bit(data, STATUSRSSI_RDSS) == BIT_ON)
		return RDS_DECODER_SYNCHRONIZED;
	else
		return RDS_DECODER_NOT_SYNCHRONIZED;
}

static inline int STATUSRSSI_RDS_BLOCK_A_ERRORS(u16 data)
{
	int ret = 0;
	int bits_status = 0;

	ret = check_bit(data, STATUSRSSI_BLERA1);

	if (ret == BIT_ON)
		bits_status = 0x02;
	else
		bits_status = 0x00;

	ret = check_bit(data, STATUSRSSI_BLERA0);

	if (ret == BIT_ON)
		bits_status |= 0x01;

	return bits_status;
}

static inline int STATUSRSSI_STEREO_STATUS(u16 data)
{
	if (check_bit(data, STATUSRSSI_RDSS) == BIT_ON)
		return STEREO;
	else
		return MONO;
}

static inline u8 STATUSRSSI_RSSI_SIGNAL_STRENGTH(u16 data)
{
	return (u8) (0x00FF & data);
}

static inline u8 DEVICE_ID_PART_NUMBER(u16 data)
{
	data = data >> 12;
	return (u8) (0x000F & data);
}

static inline u16 DEVICE_ID_MANUFACT_NUMBER(u16 data)
{
	return (u16) (0x0FFF & data);
}

static inline u8 CHIP_ID_CHIP_VERSION(u16 data)
{
	data = data >> 10;
	return (u8) (0x003F & data);
}

static inline u8 CHIP_ID_DEVICE(u16 data)
{
	data = data >> 6;
	return (u8) (0x000F & data);
}

static inline u8 CHIP_ID_FIRMWARE_VERSION(u16 data)
{
	return (u8) (0x003F & data);
}

static inline u8 SYS_CONFIG2_RSSI_TH(u16 data)
{
	data = data >> 8;
	return (u8) (0x00FF & data);
}

static inline u8 SYS_CONFIG2_FM_BAND(u16 data)
{
	data = data >> 6;
	return (u8) (0x0003 & data);
}

static inline u8 SYS_CONFIG2_FM_CHAN_SPAC(u16 data)
{
	data = data >> 4;
	return (u8) (0x0003 & data);
}

static inline u8 SYS_CONFIG2_FM_VOL(u16 data)
{
	return (u8) (0x000F & data);
}

/*POWER_CONFIG_STATUS*/

#define SOFTMUTE_ENABLE			0
#define SOFTMUTE_DISABLE		1

#define MUTE_ENABLE			0
#define MUTE_DISABLE			1

#define STEREO_SELECT			0
#define MONO_SELECT			1

#define RDS_MODE_STANDARD		0
#define RDS_MODE_VERBOSE		1

#define SEEK_MODE_CONT_SEEK		0
#define SEEK_MODE_STOP_SEEK		1

#define SEEK_DOWN			0
#define SEEK_UP				1

#define SEEK_DISABLE			0
#define SEEK_ABLE			1

static inline int POWER_CONFIG_SOFTMUTE_STATUS(u16 data)
{
	if (check_bit(data, POWERCFG_DSMUTE) == BIT_ON)
		return SOFTMUTE_DISABLE;
	else
		return SOFTMUTE_ENABLE;
}

static inline int POWER_CONFIG_MUTE_STATUS(u16 data)
{
	if (check_bit(data, POWERCFG_DMUTE) == BIT_ON)
		return MUTE_DISABLE;
	else
		return MUTE_ENABLE;
}

static inline int POWER_CONFIG_MONO_STATUS(u16 data)
{
	if (check_bit(data, POWERCFG_MONO) == BIT_ON)
		return MONO_SELECT;
	else
		return STEREO_SELECT;
}

static inline int POWER_CONFIG_RDS_MODE_STATUS(u16 data)
{
	if (check_bit(data, POWERCFG_RDSM) == BIT_ON)
		return RDS_MODE_VERBOSE;
	else
		return RDS_MODE_STANDARD;
}

static inline int POWER_CONFIG_SKMODE_STATUS(u16 data)
{
	if (check_bit(data, POWERCFG_SKMODE) == BIT_ON)
		return SEEK_MODE_STOP_SEEK;
	else
		return SEEK_MODE_CONT_SEEK;
}

static inline int POWER_CONFIG_SEEKUP_STATUS(u16 data)
{
	if (check_bit(data, POWERCFG_SEEKUP) == BIT_ON)
		return SEEK_UP;
	else
		return SEEK_DOWN;
}

static inline int POWER_CONFIG_SEEK_STATUS(u16 data)
{
	if (check_bit(data, POWERCFG_SEEK) == BIT_ON)
		return SEEK_ABLE;
	else
		return SEEK_DISABLE;
}

static inline int POWER_CONFIG_DISABLE_STATUS(u16 data)
{
	if (check_bit(data, POWERCFG_DISABLE) == BIT_ON)
		return 1;
	else
		return 0;
}

static inline int POWER_CONFIG_ENABLE_STATUS(u16 data)
{
	if (check_bit(data, POWERCFG_ENABLE) == BIT_ON)
		return 1;
	else
		return 0;
}

/********************************************************************/

static inline int READCHAN_BLOCK_B_ERRORS(u16 data)
{
	int ret = 0;
	int bits_status = 0;

	ret = check_bit(data, READCHAN_BLERB1);

	if (ret == BIT_ON)
		bits_status = 0x02;
	else
		bits_status = 0x00;

	ret = check_bit(data, READCHAN_BLERB0);

	if (ret == BIT_ON)
		bits_status |= 0x01;

	return bits_status;
}

static inline int READCHAN_BLOCK_C_ERRORS(u16 data)
{
	int ret = 0;
	int bits_status = 0;

	ret = check_bit(data, READCHAN_BLERC1);

	if (ret == BIT_ON)
		bits_status = 0x02;
	else
		bits_status = 0x00;

	ret = check_bit(data, READCHAN_BLERC0);

	if (ret == BIT_ON)
		bits_status |= 0x01;

	return bits_status;
}

static inline int READCHAN_BLOCK_D_ERRORS(u16 data)
{
	int ret = 0;
	int bits_status = 0;

	ret = check_bit(data, READCHAN_BLERD1);

	if (ret == BIT_ON)
		bits_status = 0x02;
	else
		bits_status = 0x00;

	ret = check_bit(data, READCHAN_BLERD0);

	if (ret == BIT_ON)
		bits_status |= 0x01;

	return bits_status;
}

static inline u16 READCHAN_GET_CHAN(u16 data)
{
	return data & READCHAN_CHAN_MASK;
}

/********************************************************************/

#endif
