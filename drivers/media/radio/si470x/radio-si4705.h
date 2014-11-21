/*
 *  drivers/media/radio/si4705/radio-si4705.h
 *
 *  Driver for radios with Silicon Labs Si4705 FM Radio Receivers
 *
 *  Copyright (c) 2009 Tobias Lorenz <tobias.lorenz@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


/* driver definitions */
#define DRIVER_NAME "radio-si4705"
#define DRIVER_AUTHOR "NULL";
#define DRIVER_KERNEL_VERSION KERNEL_VERSION(1, 0, 1)
#define DRIVER_CARD "Silicon Labs Si4705 FM Radio Receiver"
#define DRIVER_DESC "I2C radio driver for Si4705 FM Radio Receivers"
#define DRIVER_VERSION "1.0.1"

/* kernel includes */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <linux/mutex.h>
#include <linux/si4705_pdata.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <asm/unaligned.h>

/**************************************************************************
 * Register Definitions
 **************************************************************************/

/* Power up device and mode selection. */
#define POWER_UP		0x01
#define POWER_UP_NARGS		3
#define POWER_UP_NRESP		1
#define POWER_UP_IN_FUNC_FMRX         0x00
#define POWER_UP_IN_FUNC_AMRX         0x01
#define POWER_UP_IN_FUNC_FMTX         0x02
#define POWER_UP_IN_FUNC_WBRX         0x03
#define POWER_UP_IN_FUNC_QUERY        0x0F
#define POWER_UP_IN_PATCH             0x20
#define POWER_UP_IN_GPO2OEN           0x40
#define POWER_UP_IN_CTSIEN            0x80
#define POWER_UP_IN_OPMODE_RX_ANALOG  0x05
#define POWER_UP_IN_OPMODE_RX_DIGITAL 0xB0

/* Returns revision information on the device. */
#define GET_REV			0x10
#define GET_REV_NARGS		1
#define GET_REV_NRESP		9

/* Power down device. */
#define POWER_DOWN		0x11
#define POWER_DOWN_NARGS		1
#define POWER_DOWN_NRESP		1

/* Sets the value of a property. */
#define SET_PROPERTY		0x12
#define SET_PROPERTY_NARGS	6
#define SET_PROPERTY_NRESP	1

/* Retrieves a property¡¯s value. */
#define GET_PROPERTY		0x13
#define GET_PROPERTY_NARGS	4
#define GET_PROPERTY_NRESP	4

/* Reads interrupt status bits. */
#define GET_INT_STATUS		0x14
#define GET_INT_STATUS_NARGS	1
#define GET_INT_STATUS_NRESP	1
#define GET_INT_STATUS_CTS		0x80
#define GET_INT_STATUS_ERR		0x40
#define GET_INT_STATUS_RSQINT	0x08
#define GET_INT_STATUS_RDSINT	0x04
#define GET_INT_STATUS_STCINT	0x01
#define GET_INT_STATUS_CTS_SHFT		7
#define GET_INT_STATUS_ERR_SHFT		6
#define GET_INT_STATUS_RSQINT_SHFT	3
#define GET_INT_STATUS_RDSINT_SHFT	2
#define GET_INT_STATUS_STCINT_SHFT	0

/* Reserved command used for patch file downloads. */
#define PATCH_ARGS		0x15

/* Reserved command used for patch file downloads. */
#define PATCH_DATA		0x16

/* Selects the FM tuning frequency. */
#define FM_TUNE_FREQ		0x20
#define FM_TUNE_FREQ_NARGS	5
#define FM_TUNE_FREQ_NRESP	1

/* Begins searching for a valid frequency. */
#define FM_SEEK_START		0x21
#define FM_SEEK_START_NARGS	2
#define FM_SEEK_START_NRESP	1
#define FM_SEEK_START_IN_WRAP   0x04
#define FM_SEEK_START_IN_SEEKUP 0x08

/* Queries the status of previous FM_TUNE_FREQ or FM_SEEK_START command. */
#define FM_TUNE_STATUS			0x22
#define FM_TUNE_STATUS_NARGS		2
#define FM_TUNE_STATUS_NRESP		8
#define FM_TUNE_STATUS_IN_INTACK	0x01
#define FM_TUNE_STATUS_IN_CANCEL	0x02
#define FM_TUNE_STATUS_OUT_VALID	0x01
#define FM_TUNE_STATUS_OUT_AFCRL	0x02
#define FM_TUNE_STATUS_OUT_BTLF		0x80

/* Queries the status of the Received Signal Quality (RSQ) */
/* of the current channel. */
#define FM_RSQ_STATUS			0x23
#define FM_RSQ_STATUS_NARGS		2
#define FM_RSQ_STATUS_NRESP		8
#define FM_RSQ_STATUS_IN_INTACK		0x01
#define FM_RSQ_STATUS_OUT_RSSILINT	0x01
#define FM_RSQ_STATUS_OUT_RSSIHINT	0x02
#define FM_RSQ_STATUS_OUT_ASNRLINT	0x04
#define FM_RSQ_STATUS_OUT_ASNRHINT	0x08
#define FM_RSQ_STATUS_OUT_BLENDINT	0x80
#define FM_RSQ_STATUS_OUT_VALID		0x01
#define FM_RSQ_STATUS_OUT_AFCRL		0x02
#define FM_RSQ_STATUS_OUT_SMUTE		0x08
#define FM_RSQ_STATUS_OUT_PILOT		0x80
#define FM_RSQ_STATUS_OUT_STBLEND	0x7F

/* Returns RDS information for current channel */
/* and reads an entry from RDS FIFO. */
#define FM_RDS_STATUS			0x24
#define FM_RDS_STATUS_NARGS		2
#define FM_RDS_STATUS_NRESP		13
#define FM_RDS_STATUS_IN_INTACK		0x01
#define FM_RDS_STATUS_IN_MTFIFO		0x02
#define FM_RDS_STATUS_OUT_RECV		0x01
#define FM_RDS_STATUS_OUT_SYNCLOST	0x02
#define FM_RDS_STATUS_OUT_SYNCFOUND	0x04
#define FM_RDS_STATUS_OUT_SYNC		0x01
#define FM_RDS_STATUS_OUT_GRPLOST	0x04
#define FM_RDS_STATUS_OUT_BLED		0x03
#define FM_RDS_STATUS_OUT_BLEC		0x0C
#define FM_RDS_STATUS_OUT_BLEB		0x30
#define FM_RDS_STATUS_OUT_BLEA		0xC0
#define FM_RDS_STATUS_OUT_BLED_SHFT	0
#define FM_RDS_STATUS_OUT_BLEC_SHFT	2
#define FM_RDS_STATUS_OUT_BLEB_SHFT	4
#define FM_RDS_STATUS_OUT_BLEA_SHFT	6

/* Queries the current AGC settings */
#define FM_AGC_STATUS			0x27
#define FM_AGC_STATUS_NARGS		1
#define FM_AGC_STATUS_NRESP		3
#define FM_AGC_STATUS_RFAGCDIS		0x01
#define FM_AGC_STATUS_LNAGAINIDX	0x1F

/* Override AGC setting by disabling and forcing it to a fixed value */
#define FM_AGC_OVERRIDE			0x28
#define FM_AGC_OVERRIDE_NARGS		3
#define FM_AGC_OVERRIDE_NRESP		1
#define FM_AGC_OVERRIDE_RFAGCDIS	0x01
#define FM_AGC_OVERRIDE_LNAGAINIDX	0x1F

/* Configures GPO1, 2, and 3 as output or Hi-Z. */
#define GPIO_CTL		0x80
#define GPIO_CTL_NARGS		2
#define GPIO_CTL_NRESP		1
#define GPIO_CTL_GPO3OEN	0x08
#define GPIO_CTL_GPO2OEN	0x04
#define GPIO_CTL_GPO1OEN	0x02

/* Sets GPO1, 2, and 3 output level (low or high) */
#define GPIO_SET		0x81
#define GPIO_SET_NARGS		2
#define GPIO_SET_NRESP		1
#define GPIO_SET_GPO3LEVEL	0x08
#define GPIO_SET_GPO2LEVEL	0x04
#define GPIO_SET_GPO1LEVEL	0x02

/* STATUS bits - Used by all methods */
#define STCINT  0x01
#define ASQINT  0x02
#define RDSINT  0x04
#define RSQINT  0x08
#define ERR     0x40
#define CTS     0x80

/*==================================================================
 General Properties
==================================================================*/

/* GPO_IEN */
#define GPO_IEN	0x0001
#define GPO_IEN_STCIEN_MASK 0x0001
#define GPO_IEN_ASQIEN_MASK 0x0002
#define GPO_IEN_RDSIEN_MASK 0x0004
#define GPO_IEN_RSQIEN_MASK 0x0008
#define GPO_IEN_ERRIEN_MASK 0x0040
#define GPO_IEN_CTSIEN_MASK 0x0080
#define GPO_IEN_STCREP_MASK 0x0100
#define GPO_IEN_ASQREP_MASK 0x0200
#define GPO_IEN_RDSREP_MASK 0x0400
#define GPO_IEN_RSQREP_MASK 0x0800
#define GPO_IEN_STCIEN_SHFT 0
#define GPO_IEN_ASQIEN_SHFT 1
#define GPO_IEN_RDSIEN_SHFT 2
#define GPO_IEN_RSQIEN_SHFT 3
#define GPO_IEN_ERRIEN_SHFT 6
#define GPO_IEN_CTSIEN_SHFT 7
#define GPO_IEN_STCREP_SHFT 8
#define GPO_IEN_ASQREP_SHFT 9
#define GPO_IEN_RDSREP_SHFT 10
#define GPO_IEN_RSQREP_SHFT 11

/* DIGITAL_INPUT_FORMAT */
#define DIGITAL_INPUT_FORMAT            0x0101
#define DIGITAL_INPUT_FORMAT_ISIZE_MASK 0x0003
#define DIGITAL_INPUT_FORMAT_IMONO_MASK 0x0004
#define DIGITAL_INPUT_FORMAT_IMODE_MASK 0x0078
#define DIGITAL_INPUT_FORMAT_IFALL_MASK 0x0080
#define DIGITAL_INPUT_FORMAT_ISIZE_SHFT 0
#define DIGITAL_INPUT_FORMAT_IMONO_SHFT 2
#define DIGITAL_INPUT_FORMAT_IMODE_SHFT 3
#define DIGITAL_INPUT_FORMAT_IFALL_SHFT 7

/* DIGITAL_INPUT_SAMPLE_RATE */
#define DIGITAL_INPUT_SAMPLE_RATE 0x0103

/* DIGITAL_OUTPUT_FORMAT */
#define DIGITAL_OUTPUT_FORMAT            0x0102
#define DIGITAL_OUTPUT_FORMAT_OSIZE_MASK 0x0003
#define DIGITAL_OUTPUT_FORMAT_OMONO_MASK 0x0004
#define DIGITAL_OUTPUT_FORMAT_OMODE_MASK 0x0078
#define DIGITAL_OUTPUT_FORMAT_OFALL_MASK 0x0080
#define DIGITAL_OUTPUT_FORMAT_OSIZE_SHFT 0
#define DIGITAL_OUTPUT_FORMAT_OMONO_SHFT 2
#define DIGITAL_OUTPUT_FORMAT_OMODE_SHFT 3
#define DIGITAL_OUTPUT_FORMAT_OFALL_SHFT 7

/* DIGITAL_OUTPUT_SAMPLE_RATE */
#define DIGITAL_OUTPUT_SAMPLE_RATE 0x0104

/* REFCLK_FREQ */
#define REFCLK_FREQ 0x0201

/* REFCLK_PRESCALE */
#define REFCLK_PRESCALE      0x0202
#define REFCLK_PRESCALE_MASK 0x0FFF
#define REFCLK_PRESCALE_SHFT 0

/*==================================================================
 FM Receive Properties
==================================================================*/

/* FM_DEEMPHASIS */
#define FM_DEEMPHASIS      0x1100
#define FM_DEEMPHASIS_MASK 0x0003
#define FM_DEEMPHASIS_SHFT 0

/* FM_BLEND_STEREO_THRESHOLD */
#define FM_BLEND_STEREO_THRESHOLD      0x1105
#define FM_BLEND_STEREO_THRESHOLD_MASK 0x007F
#define FM_BLEND_STEREO_THRESHOLD_SHFT 0

/* FM_BLEND_MONO_THRESHOLD */
#define FM_BLEND_MONO_THRESHOLD      0x1106
#define FM_BLEND_MONO_THRESHOLD_MASK 0x007F
#define FM_BLEND_MONO_THRESHOLD_SHFT 0

/* FM_ANTENNA_INPUT */
#define FM_ANTENNA_INPUT      0x1107
#define FM_ANTENNA_INPUT_MASK 0x0001
#define FM_ANTENNA_INPUT_SHFT 0

/* FM_MAX_TUNE_ERROR */
#define FM_MAX_TUNE_ERROR      0x1108
#define FM_MAX_TUNE_ERROR_MASK 0x007F
#define FM_MAX_TUNE_ERROR_SHFT 0

/* FM_RSQ_INT_SOURCE */
#define FM_RSQ_INT_SOURCE               0x1200
#define FM_RSQ_INT_SOURCE_RSSILIEN_MASK 0x0001
#define FM_RSQ_INT_SOURCE_RSSIHIEN_MASK 0x0002
#define FM_RSQ_INT_SOURCE_ASNRLIEN_MASK 0x0004
#define FM_RSQ_INT_SOURCE_ASNRHIEN_MASK 0x0008
#define FM_RSQ_INT_SOURCE_BLENDIEN_MASK 0x0080
#define FM_RSQ_INT_SOURCE_RSSILIEN_SHFT 0
#define FM_RSQ_INT_SOURCE_RSSIHIEN_SHFT 1
#define FM_RSQ_INT_SOURCE_ASNRLIEN_SHFT 2
#define FM_RSQ_INT_SOURCE_ASNRHIEN_SHFT 3
#define FM_RSQ_INT_SOURCE_BLENDIEN_SHFT 7

/* FM_RSQ_SNR_HI_THRESHOLD */
#define FM_RSQ_SNR_HI_THRESHOLD      0x1201
#define FM_RSQ_SNR_HI_THRESHOLD_MASK 0x007F
#define FM_RSQ_SNR_HI_THRESHOLD_SHFT 0

/* FM_RSQ_SNR_LO_THRESHOLD */
#define FM_RSQ_SNR_LO_THRESHOLD      0x1202
#define FM_RSQ_SNR_LO_THRESHOLD_MASK 0x007F
#define FM_RSQ_SNR_LO_THRESHOLD_SHFT 0

/* FM_RSQ_RSSI_HI_THRESHOLD */
#define FM_RSQ_RSSI_HI_THRESHOLD      0x1203
#define FM_RSQ_RSSI_HI_THRESHOLD_MASK 0x007F
#define FM_RSQ_RSSI_HI_THRESHOLD_SHFT 0

/* FM_RSQ_RSSI_LO_THRESHOLD */
#define FM_RSQ_RSSI_LO_THRESHOLD      0x1204
#define FM_RSQ_RSSI_LO_THRESHOLD_MASK 0x007F
#define FM_RSQ_RSSI_LO_THRESHOLD_SHFT 0

/* FM_RSQ_BLEND_THRESHOLD */
#define FM_RSQ_BLEND_THRESHOLD            0x1207
#define FM_RSQ_BLEND_THRESHOLD_BLEND_MASK 0x007F
#define FM_RSQ_BLEND_THRESHOLD_PILOT_MASK 0x0080
#define FM_RSQ_BLEND_THRESHOLD_BLEND_SHFT 0
#define FM_RSQ_BLEND_THRESHOLD_PILOT_SHFT 7

/* FM_SOFT_MUTE_RATE */
#define FM_SOFT_MUTE_RATE      0x1300
#define FM_SOFT_MUTE_RATE_MASK 0x00FF
#define FM_SOFT_MUTE_RATE_SHFT 0

/* FM_SOFT_MUTE_MAX_ATTENUATION */
#define FM_SOFT_MUTE_MAX_ATTENUATION      0x1302
#define FM_SOFT_MUTE_MAX_ATTENUATION_MASK 0x001F
#define FM_SOFT_MUTE_MAX_ATTENUATION_SHFT 0

/* FM_SOFT_MUTE_SNR_THRESHOLD */
#define FM_SOFT_MUTE_SNR_THRESHOLD      0x1303
#define FM_SOFT_MUTE_SNR_THRESHOLD_MASK 0x000F
#define FM_SOFT_MUTE_SNR_THRESHOLD_SHFT 0

/* FM_SEEK_BAND_BOTTOM */
#define FM_SEEK_BAND_BOTTOM 0x1400

/* FM_SEEK_BAND_TOP */
#define FM_SEEK_BAND_TOP 0x1401

/* FM_SEEK_FREQ_SPACING */
#define FM_SEEK_FREQ_SPACING      0x1402
#define FM_SEEK_FREQ_SPACING_MASK 0x001F
#define FM_SEEK_FREQ_SPACING_SHFT 0

/* FM_SEEK_TUNE_SNR_THRESHOLD */
#define FM_SEEK_TUNE_SNR_THRESHOLD      0x1403
#define FM_SEEK_TUNE_SNR_THRESHOLD_MASK 0x007F
#define FM_SEEK_TUNE_SNR_THRESHOLD_SHFT 0
#define FM_SEEK_TUNE_SNR_THRESHOLD_DEFAULT 0xa

/* FM_SEEK_TUNE_RSSI_THRESHOLD */
#define FM_SEEK_TUNE_RSSI_THRESHOLD      0x1404
#define FM_SEEK_TUNE_RSSI_THRESHOLD_MASK 0x007F
#define FM_SEEK_TUNE_RSSI_THRESHOLD_SHFT 0
#define FM_SEEK_TUNE_RSSI_THRESHOLD_DEFAULT 0x14

/* RDS_INT_SOURCE */
#define RDS_INT_SOURCE                0x1500
#define RDS_INT_SOURCE_RECV_MASK      0x0001
#define RDS_INT_SOURCE_SYNCLOST_MASK  0x0002
#define RDS_INT_SOURCE_SYNCFOUND_MASK 0x0004
#define RDS_INT_SOURCE_RECV_SHFT      0
#define RDS_INT_SOURCE_SYNCLOST_SHFT  1
#define RDS_INT_SOURCE_SYNCFOUND_SHFT 2

/* RDS_INT_FIFO_COUNT */
#define RDS_INT_FIFO_COUNT      0x1501
#define RDS_INT_FIFO_COUNT_MASK 0x00FF
#define RDS_INT_FIFO_COUNT_SHFT 0

/* RDS_CONFIG */
#define RDS_CONFIG             0x1502
#define RDS_CONFIG_RDSEN_MASK  0x0001
#define RDS_CONFIG_BLETHD_MASK 0x0300
#define RDS_CONFIG_BLETHC_MASK 0x0C00
#define RDS_CONFIG_BLETHB_MASK 0x3000
#define RDS_CONFIG_BLETHA_MASK 0xC000
#define RDS_CONFIG_RDSEN_SHFT  0
#define RDS_CONFIG_BLETHD_SHFT 8
#define RDS_CONFIG_BLETHC_SHFT 10
#define RDS_CONFIG_BLETHB_SHFT 12
#define RDS_CONFIG_BLETHA_SHFT 14

/*==================================================================
 General Receive Properties
==================================================================*/

/* RX_VOLUME */
#define RX_VOLUME      0x4000
#define RX_VOLUME_MASK 0x003F
#define RX_VOLUME_MAX  0x003F
#define RX_VOLUME_SHFT 0

/* RX_HARD_MUTE */
#define RX_HARD_MUTE 0x4001
#define RX_HARD_MUTE_RMUTE_MASK 0x0001
#define RX_HARD_MUTE_LMUTE_MASK 0x0002
#define RX_HARD_MUTE_RMUTE_SHFT 0
#define RX_HARD_MUTE_LMUTE_SHFT 1

/*==================================================================
 Bit Definitions for Properties
==================================================================*/

/* DIGITAL_MODE - used for input or output */
#define DIGITAL_MODE_I2S    0x0
#define DIGITAL_MODE_LEFT   0x6
#define DIGITAL_MODE_MSB1ST 0xC
#define DIGITAL_MODE_MSB2ND 0x8

/* DIGITAL_SIZE - used for input or output */
#define DIGITAL_SIZE_16 0x0
#define DIGITAL_SIZE_20 0x1
#define DIGITAL_SIZE_24 0x2
#define DIGITAL_SIZE_8  0x3

/* FM_DEEMPH */
#define FM_DEEMPH_75US 0x2
#define FM_DEEMPH_50US 0x1

/* FM_RDS_BLETH - used for all block error thresholds */
#define FM_RDS_BLETH_NO_ERRORS     0x0
#define FM_RDS_BLETH_1OR2_ERRORS   0x1
#define FM_RDS_BLETH_3TO5_ERRORS   0x2
#define FM_RDS_BLETH_UNCORRECTABLE 0x3

/**************************************************************************
 * General Driver Definitions
 **************************************************************************/
enum {
	RADIO_OP_LP_MODE	= 0,
	/* radio playback routed directly codec. default */
	RADIO_OP_RICH_MODE,
	/* radio playback routed via AP */
	RADIO_OP_DEFAULT = RADIO_OP_LP_MODE
};

/*
 * si4705_device - private data
 */
struct si4705_device {
	struct video_device *videodev;

	/* driver management */
	unsigned int users;
	unsigned char op_mode;
	unsigned char power_state;
	unsigned char power_state_at_suspend;

	/* si4705 int_status (0..7) */
	unsigned char int_status;

	/* RDS receive buffer */
	wait_queue_head_t read_queue;
	struct mutex lock;		/* buffer locking */
	struct work_struct resp_work;
	struct workqueue_struct *queue;
	unsigned char *buffer;		/* size is always multiple of three */
	unsigned int buf_size;
	unsigned int rd_index;
	unsigned int wr_index;

	struct completion completion;
	bool stci_enabled;		/* Seek/Tune Complete Interrupt */

	struct i2c_client *client;
	struct si4705_pdata *pdata;
};



/**************************************************************************
 * Firmware Versions
 **************************************************************************/

#define RADIO_FW_VERSION	15



/**************************************************************************
 * Frequency Multiplicator
 **************************************************************************/

/*
 * The frequency is set in units of 62.5 Hz when using V4L2_TUNER_CAP_LOW,
 * 62.5 kHz otherwise.
 * The tuner is able to have a channel spacing of 50, 100 or 200 kHz.
 * tuner->capability is therefore set to V4L2_TUNER_CAP_LOW
 * The FREQ_MUL is then: 1 MHz / 62.5 Hz = 16000
 */
#define FREQ_MUL (10000000 / 625)



/**************************************************************************
 * Common Functions
 **************************************************************************/
extern struct video_device si4705_viddev_template;
int si4705_power_up(struct si4705_device *radio);
int si4705_power_down(struct si4705_device *radio);
int si4705_set_property(struct si4705_device *radio, u16 prop, u16 val);
int si4705_get_property(struct si4705_device *radio, u16 prop, u16 *pVal);
int si4705_get_int_status(struct si4705_device *radio, u8 *cts, u8 *err,
		u8 *rsqint, u8 *rdsint, u8 *stcint);
int si4705_fm_tune_freq(struct si4705_device *radio, u16 freq, u8 antcap);
int si4705_fm_seek_start(struct si4705_device *radio, u8 seekUp, u8 wrap);
int si4705_fm_tune_status(struct si4705_device *radio, u8 cancel,
		u8 intack, u8 *bltf, u16 *freq, u8 *rssi, u8 *snr, u8 *antcap);
int si4705_fm_rsq_status(struct si4705_device *radio, u8 intack,
		u8 *intStatus, u8 *indStatus, u8 *stBlend, u8 *rssi,
		u8 *snr, u8 *freqOff);
int si4705_fm_rds_status(struct si4705_device *radio, u8 mtfifo,
		u8 intack, u8 *rdsInd, u8 *sync, u8 *fifoUsed, u16 *rdsFifo,
		u8 *ble);
int si4705_fm_agc_status(struct si4705_device *radio, u8 *rfAgcDis,
		u8 *lnaGainIndex);
int si4705_fm_agc_override(struct si4705_device *radio, u8 *rfAgcDis,
		u8 *lnaGainIndex);
int si4705_gpio_ctl(struct si4705_device *radio, u8 gpio1,
		u8 gpio2, u8 gpio3);
int si4705_gpio_set(struct si4705_device *radio, u8 gpio1,
		u8 gpio2, u8 gpio3);
int si4705_disconnect_check(struct si4705_device *radio);
u16 si4705_vol_conv_index_to_value(struct si4705_device *radio, s32 index);
s32 si4705_vol_conv_value_to_index(struct si4705_device *radio, u16 value);
u16 si4705_get_seek_tune_rssi_threshold_value(struct si4705_device *radio);
u16 si4705_get_seek_tune_snr_threshold_value(struct si4705_device *radio);
