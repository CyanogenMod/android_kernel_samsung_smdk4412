#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include "Si4709_regs.h"
#include "Si4709_main.h"
#include "Si4709_dev.h"
#include "Si4709_common.h"

#if 1 /* To do */
#include "commanddefs.h"
#include "propertydefs.h"
#endif


enum {
	eTRUE,
	eFALSE,
} dev_struct_status_t;

/*dev_state*/
/*power_state*/
#define RADIO_ON		1
#define RADIO_POWERDOWN		0
/*seek_state*/
#define RADIO_SEEK_ON		1
#define RADIO_SEEK_OFF		0

#define FREQ_87500_kHz		8750
#define FREQ_76000_kHz		7600

#define RSSI_seek_th_MAX	0x7F
#define RSSI_seek_th_MIN	0x00

#define seek_SNR_th_DISB	0x00
#define seek_SNR_th_MIN		0x01	/*most stops */
#define seek_SNR_th_MAX		0x0F	/*fewest stops */

#define seek_FM_ID_th_DISB	0x00
#define seek_FM_ID_th_MAX	0x01	/*most stops */
#define seek_FM_ID_th_MIN	0x0F	/*fewest stops */

#define TUNE_RSSI_THRESHOLD	10
#define TUNE_SNR_THRESHOLD	4
#define TUNE_CNT_THRESHOLD	0x00

#define _ENABLE_RDS_

#ifdef RDS_INTERRUPT_ON_ALWAYS
#define RDS_BUFFER_LENGTH	50
static u16 *RDS_Block_Data_buffer;
static u8 *RDS_Block_Error_buffer;
static u8 RDS_Buffer_Index_read;	/* index number for last read data */
static u8 RDS_Buffer_Index_write;	/* index number for last written data */

int Si4709_RDS_flag;
int RDS_Data_Available;
int RDS_Data_Lost;
int RDS_Groups_Available_till_now;
struct workqueue_struct *Si4709_wq;
struct work_struct Si4709_work;
#endif

u8 cmd[8];
u8 rsp[13];
u8 RsqInts;
u8 Status;
u8 SMUTE;
u8 AFCRL;
u8 Valid;
u8 Pilot;
u8 Blend;
u16 Freq;
u8 RSSI;
u8 ASNR;
u8 FreqOff;
u8 STC;
u8 BLTF;
u16 AntCap;
u8  RdsInts;
u8  RdsSync;
u8  GrpLost;
u8  RdsFifoUsed;
u16 BlockA;
u16 BlockB;
u16 BlockC;
u16 BlockD;
u8  BleA;
u8  BleB;
u8  BleC;
u8  BleD;



/*static functions*/
/**********************************************/
static void wait(void);

#ifndef RDS_INTERRUPT_ON_ALWAYS
static void wait_RDS(void);
#endif

static int powerup(void);
static int powerdown(void);

static int seek(u32 *, int, int);
static int tune_freq(u32);
static u16 freq_to_channel(u32);
static void fmTuneStatus(u8 cancel, u8 intack);
static void fmRsqStatus(u8 intack);
static void si47xx_set_property(u16 propNumber, u16 propValue);
static int si47xx_command(u8 cmd_size, u8 *cmd, u8 reply_size, u8 *reply);
static void fmRdsStatus(u8 intack, u8 mtfifo);
static void fmTuneFreq(u16 frequency);
static void fmSeekStart(u8 seekUp, u8 wrap);
static u8 si47xx_readStatus(void);
static void si47xx_waitForCTS(void);
static u8 getIntStatus(void);
static int i2c_write(u8 number_bytes, u8 *data_out);
static int i2c_read(u8 number_bytes, u8 *data_in);
/**********************************************/

/*Si4709 device structure*/
static struct Si4709_device_t Si4709_dev = {
	.client = NULL,
	.valid = eFALSE,
	.valid_client_state = eFALSE,
};

/*Wait flag*/
/*WAITING or WAIT_OVER or NO_WAIT*/
int Si4709_dev_wait_flag = NO_WAIT;
#ifdef RDS_INTERRUPT_ON_ALWAYS
int Si4709_RDS_flag = NO_WAIT;
#endif

unsigned int Si4709_dev_int;
unsigned int Si4709_dev_irq;

#if defined(CONFIG_MACH_M0)
unsigned int Si4709_dev_sw;
#endif

#if defined(CONFIG_MACH_M0_CTC)
static const u16 rx_vol[] = {
0x0, 0x15, 0x18, 0x1B, 0x1E, 0x21, 0x24, 0x27,
0x2A, 0x2D, 0x30, 0x33, 0x36, 0x39, 0x3C, 0x3F};
#else
static const u16 rx_vol[] = {
0x0, 0x13, 0x16, 0x19, 0x1C, 0x1F, 0x22, 0x25,
0x28, 0x2B, 0x2E, 0x31, 0x34, 0x37, 0x3A, 0x3D};
#endif


int Si4709_dev_init(struct i2c_client *client)
{
	int ret = 0;

	debug("Si4709_dev_init called");

	mutex_lock(&(Si4709_dev.lock));

	Si4709_dev.client = client;

#if defined(CONFIG_MACH_M0)
	if (system_rev >= 15)
		Si4709_dev_int = GPIO_FM_INT_REV15;
	else
#endif
		Si4709_dev_int = GPIO_FM_INT;

	Si4709_dev_irq = gpio_to_irq(Si4709_dev_int);

#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M0_CTC)
	Si4709_dev_sw = GPIO_FM_MIC_SW;
#endif

	disable_irq(Si4709_dev_irq);

	Si4709_dev.state.power_state = RADIO_POWERDOWN;
	Si4709_dev.state.seek_state = RADIO_SEEK_OFF;
	Si4709_dev.valid_client_state = eTRUE;

#ifdef RDS_INTERRUPT_ON_ALWAYS
	/*Creating Circular Buffer */
	/*Single RDS_Block_Data buffer size is 4x16 bits */
	RDS_Block_Data_buffer = kzalloc(RDS_BUFFER_LENGTH * 8, GFP_KERNEL);
	if (!RDS_Block_Data_buffer) {
		error("Not sufficient memory for creating "
				"RDS_Block_Data_buffer");
		ret = -ENOMEM;
		goto EXIT;
	}

	/*Single RDS_Block_Error buffer size is 4x8 bits */
	RDS_Block_Error_buffer = kzalloc(RDS_BUFFER_LENGTH * 4, GFP_KERNEL);
	if (!RDS_Block_Error_buffer) {
		error("Not sufficient memory for creating "
				"RDS_Block_Error_buffer");
		ret = -ENOMEM;
		kfree(RDS_Block_Data_buffer);
		goto EXIT;
	}

	/*Initialising read and write indices */
	RDS_Buffer_Index_read = 0;
	RDS_Buffer_Index_write = 0;

	/*Creating work-queue */
	Si4709_wq = create_singlethread_workqueue("Si4709_wq");
	if (!Si4709_wq) {
		error("Not sufficient memory for Si4709_wq, work-queue");
		ret = -ENOMEM;
		kfree(RDS_Block_Error_buffer);
		kfree(RDS_Block_Data_buffer);
		goto EXIT;
	}

	/*Initialising work_queue */
	INIT_WORK(&Si4709_work, Si4709_work_func);

	RDS_Data_Available = 0;
	RDS_Data_Lost = 0;
	RDS_Groups_Available_till_now = 0;
EXIT:
#endif

	mutex_unlock(&(Si4709_dev.lock));

	debug("Si4709_dev_init call over");

	return ret;
}

int Si4709_dev_exit(void)
{
	int ret = 0;

	debug("Si4709_dev_exit called");

	mutex_lock(&(Si4709_dev.lock));

	/* Temporary blocked by abnormal function call(E-CIM 2657654) */
	/* DW Shim. 2010.03.04 */
	/*    Si4709_dev.client = NULL;	*/

	/*    Si4709_dev.valid_client_state = eFALSE;	*/
	/*    Si4709_dev.valid = eFALSE;	*/

#ifdef RDS_INTERRUPT_ON_ALWAYS
	if (Si4709_wq)
		destroy_workqueue(Si4709_wq);

	kfree(RDS_Block_Error_buffer);
	kfree(RDS_Block_Data_buffer);
#endif

	mutex_unlock(&(Si4709_dev.lock));

	debug("Si4709_dev_exit call over");

	return ret;
}

void Si4709_dev_mutex_init(void)
{
	mutex_init(&(Si4709_dev.lock));
}

int Si4709_dev_powerup(void)
{
	int ret = 0;
	u32 value = 100;

	debug("Si4709_dev_powerup called");

	if (!(RADIO_ON == Si4709_dev.state.power_state)) {
		ret = powerup();
		if (ret < 0) {
			debug("powerup failed");
		} else if (Si4709_dev.valid_client_state == eFALSE) {
			debug("Si4709_dev_powerup called "
					"when DS(state, client) is invalid");
			ret = -1;
		} else {
/* initial settings */
#ifdef _ENABLE_RDS_
			si47xx_set_property(FM_RDS_CONFIG, 1);
			si47xx_set_property(GPO_IEN, GPO_IEN_STCIEN_MASK |
				GPO_IEN_STCREP_MASK);
			si47xx_set_property(GPO_IEN, GPO_IEN_STCIEN_MASK |
				GPO_IEN_RDSIEN_MASK | GPO_IEN_STCREP_MASK);
			si47xx_set_property(FM_RDS_INTERRUPT_SOURCE,
				FM_RDS_INTERRUPT_SOURCE_RECV_MASK);
			si47xx_set_property(FM_RDS_CONFIG,
				FM_RDS_CONFIG_RDSEN_MASK |
				(3 << FM_RDS_CONFIG_BLETHA_SHFT) |
				(3 << FM_RDS_CONFIG_BLETHB_SHFT) |
				(3 << FM_RDS_CONFIG_BLETHC_SHFT) |
				(3 << FM_RDS_CONFIG_BLETHD_SHFT));
#endif
/*VNVS:18-NOV'09 : Setting DE-Time Constant as 50us(Europe,Japan,Australia)*/
			si47xx_set_property(FM_DEEMPHASIS, FM_DEEMPH_50US);
			/* SYSCONFIG2_BITSET_SEEKTH( */
			/*      &Si4709_dev.registers[SYSCONFIG2],2); */
/*VNVS:18-NOV'09 : modified for detecting more stations of good quality*/
			si47xx_set_property(FM_SEEK_TUNE_RSSI_THRESHOLD,
				TUNE_RSSI_THRESHOLD);
			si47xx_set_property(FM_SEEK_BAND_BOTTOM, 8750);
			si47xx_set_property(FM_SEEK_BAND_TOP, 10800);
			Si4709_dev.settings.band = BAND_87500_108000_kHz;
			Si4709_dev.settings.bottom_of_band = FREQ_87500_kHz;
			si47xx_set_property(FM_SEEK_FREQ_SPACING,
				CHAN_SPACING_100_kHz);
			Si4709_dev.settings.channel_spacing =
				CHAN_SPACING_100_kHz;

			/* SYSCONFIG3_BITSET_SKSNR( */
			/*      &Si4709_dev.registers[SYSCONFIG3],3); */
/*VNVS:18-NOV'09 : modified for detecting more stations of good quality*/
			si47xx_set_property(FM_SEEK_TUNE_SNR_THRESHOLD,
			TUNE_SNR_THRESHOLD);
			Si4709_dev.settings.timeout_RDS =
				msecs_to_jiffies(value);
			Si4709_dev.settings.curr_snr = TUNE_SNR_THRESHOLD;
			Si4709_dev.settings.curr_rssi_th = TUNE_RSSI_THRESHOLD;
			Si4709_dev_STEREO_SET();

/*this will write all the above registers */
			if (ret < 0)
				debug("Si4709_dev_powerup i2c_write 1 failed");
			else {
				Si4709_dev.valid = eTRUE;
#ifdef RDS_INTERRUPT_ON_ALWAYS
/*Initialising read and write indices */
				RDS_Buffer_Index_read = 0;
				RDS_Buffer_Index_write = 0;

				RDS_Data_Available = 0;
				RDS_Data_Lost = 0;
				RDS_Groups_Available_till_now = 0;
#endif
			}
		}
	} else
		debug("Device already Powered-ON");
	enable_irq(Si4709_dev_irq);
	si47xx_set_property(0xff00, 0);

	return ret;
}

int Si4709_dev_powerdown(void)
{
	int ret = 0;

	msleep(500);		/* For avoiding turned off pop noise */
	debug("Si4709_dev_powerdown called");

	mutex_lock(&(Si4709_dev.lock));

	disable_irq(Si4709_dev_irq);

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_powerdown called when DS is invalid");
		ret = -1;
	} else {
		ret = powerdown();
		if (ret < 0)
			debug("powerdown failed");
	}
	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_suspend(void)
{
	int ret = 0;

	debug("Si4709_dev_suspend called");

#ifndef _ENABLE_RDS_
	disable_irq(Si4709_dev_irq);
#endif

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid_client_state == eFALSE) {
		debug("Si4709_dev_suspend called "
				"when DS(state, client) is invalid");
		ret = -1;
	}
	mutex_unlock(&(Si4709_dev.lock));

	debug("Si4709_dev_enable call over");

	return ret;
}

int Si4709_dev_resume(void)
{
	int ret = 0;

	debug("Si4709_dev_resume called");

#ifndef _ENABLE_RDS_
	enable_irq(Si4709_dev_irq);
#endif

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid_client_state == eFALSE) {
		debug("Si4709_dev_resume called "
				"when DS(state, client) is invalid");
		ret = -1;
	}

	mutex_unlock(&(Si4709_dev.lock));
	debug("Si4709_dev_disable call over");

	return ret;
}

int Si4709_dev_band_set(int band)
{
	int ret = 0;
	u16 prev_band = 0;
	u32 prev_bottom_of_band = 0;

	debug("Si4709_dev_band_set called");

	prev_band = Si4709_dev.settings.band;
	prev_bottom_of_band = Si4709_dev.settings.bottom_of_band;
	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_band_set called when DS is invalid");
		ret = -1;
	} else {
		switch (band) {
		case BAND_87500_108000_kHz:
			si47xx_set_property(FM_SEEK_BAND_BOTTOM, 8750);
			si47xx_set_property(FM_SEEK_BAND_TOP, 10800);
			Si4709_dev.settings.band = BAND_87500_108000_kHz;
			Si4709_dev.settings.bottom_of_band = FREQ_87500_kHz;
			break;
		case BAND_76000_108000_kHz:
			si47xx_set_property(FM_SEEK_BAND_BOTTOM, 7600);
			si47xx_set_property(FM_SEEK_BAND_TOP, 10800);
			Si4709_dev.settings.band = BAND_76000_108000_kHz;
			Si4709_dev.settings.bottom_of_band = FREQ_76000_kHz;
			break;
		case BAND_76000_90000_kHz:
			si47xx_set_property(FM_SEEK_BAND_BOTTOM, 7600);
			si47xx_set_property(FM_SEEK_BAND_TOP, 9000);
			Si4709_dev.settings.band = BAND_76000_90000_kHz;
			Si4709_dev.settings.bottom_of_band = FREQ_76000_kHz;
			break;
		default:
			ret = -1;
		}
	}

	return ret;
}

int Si4709_dev_ch_spacing_set(int ch_spacing)
{
	int ret = 0;
	u16 prev_ch_spacing = 0;

	debug("Si4709_dev_ch_spacing_set called");

	mutex_lock(&(Si4709_dev.lock));
	prev_ch_spacing = Si4709_dev.settings.channel_spacing;
	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_ch_spacing_set called "
				"when DS is invalid");
		ret = -1;
	} else {
		switch (ch_spacing) {
		case CHAN_SPACING_200_kHz:
			si47xx_set_property(FM_SEEK_FREQ_SPACING, 20);
			Si4709_dev.settings.channel_spacing =
							CHAN_SPACING_200_kHz;
			break;

		case CHAN_SPACING_100_kHz:
			si47xx_set_property(FM_SEEK_FREQ_SPACING, 10);
			Si4709_dev.settings.channel_spacing =
							CHAN_SPACING_100_kHz;
			break;

		case CHAN_SPACING_50_kHz:
			si47xx_set_property(FM_SEEK_FREQ_SPACING, 5);
			Si4709_dev.settings.channel_spacing =
							CHAN_SPACING_50_kHz;
			break;

		default:
				ret = -1;
		}

		if (ret == 0) {
			if (ret < 0) {
				debug("Si4709_dev_ch_spacing_set "
						"i2c_write 1 failed");
				Si4709_dev.settings.channel_spacing =
					prev_ch_spacing;
			}
		}
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_chan_select(u32 frequency)
{
	int ret = 0;

	debug("Si4709_dev_chan_select called");

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_chan_select called when DS is invalid");
		ret = -1;
	} else {
		Si4709_dev.state.seek_state = RADIO_SEEK_ON;

		ret = tune_freq(frequency);
		debug("Si4709_dev_chan_select called1");
		Si4709_dev.state.seek_state = RADIO_SEEK_OFF;
	}
	return ret;
}

int Si4709_dev_chan_get(u32 *frequency)
{
	int ret = 0;

	debug("Si4709_dev_chan_get called");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_chan_get called when DS is invalid");
		ret = -1;
	} else {
		if (ret < 0)
			debug("Si4709_dev_chan_get i2c_read failed");
		else {
			fmTuneStatus(0, 0);
			*frequency = Freq;
		}
	}
	mutex_unlock(&(Si4709_dev.lock));
	return ret;
}

int Si4709_dev_seek_full(u32 *frequency)
{
	int ret = 0;

	debug("Si4709_dev_seek_full called\n");
	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_seek_full called when DS is invalid");
		ret = -1;
	} else {
		Si4709_dev.state.seek_state = RADIO_SEEK_ON;
		ret = seek(frequency, 1, 0);
		Si4709_dev.state.seek_state = RADIO_SEEK_OFF;
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_seek_up(u32 *frequency)
{
	int ret = 0;

	debug("Si4709_dev_seek_up called\n");
	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_seek_up called when DS is invalid");
		ret = -1;
	} else {
		Si4709_dev.state.seek_state = RADIO_SEEK_ON;
		ret = seek(frequency, 1, 1);
		Si4709_dev.state.seek_state = RADIO_SEEK_OFF;
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_seek_down(u32 *frequency)
{
	int ret = 0;

	debug("Si4709_dev_seek_down called\n");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_seek_down called when DS is invalid");
		ret = -1;
	} else {
		Si4709_dev.state.seek_state = RADIO_SEEK_ON;

		ret = seek(frequency, 0, 1);

		Si4709_dev.state.seek_state = RADIO_SEEK_OFF;
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_RSSI_seek_th_set(u8 seek_th)
{
	int ret = 0;

	debug("Si4709_dev_RSSI_seek_th_set called");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_RSSI_seek_th_set called "
				"when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_SEEK_TUNE_RSSI_THRESHOLD, seek_th);
		Si4709_dev.settings.curr_rssi_th = seek_th;
		if (ret < 0)
			debug("Si4709_dev_RSSI_seek_th_set i2c_write 1 failed");
	}
	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_seek_SNR_th_set(u8 seek_SNR)
{
	int ret = 0;

	debug("Si4709_dev_seek_SNR_th_set called");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_seek_SNR_th_set called "
				"when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_SEEK_TUNE_SNR_THRESHOLD, seek_SNR);
		Si4709_dev.settings.curr_snr = seek_SNR;

		if (ret < 0)
			debug("Si4709_dev_seek_SNR_th_set i2c_write 1 failed");
	}
	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_seek_FM_ID_th_set(u8 seek_FM_ID_th)
{
	int ret = 0;

	debug("Si4709_dev_seek_FM_ID_th_set called");

	return ret;
}

int Si4709_dev_cur_RSSI_get(struct rssi_snr_t *cur_RSSI)
{
	int ret = 0;

	debug("Si4709_dev_cur_RSSI_get called");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_cur_RSSI_get called when DS is invalid");
		ret = -1;
	} else {
		fmRsqStatus(0);
		if (ret < 0) {
			debug("Si4709_dev_cur_RSSI_get i2c_read 1 failed");
		} else {
			cur_RSSI->curr_rssi = RSSI;
			cur_RSSI->curr_rssi_th =
				Si4709_dev.settings.curr_rssi_th;
			cur_RSSI->curr_snr = Si4709_dev.settings.curr_snr;
		}
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

/* VNVS:START 13-OCT'09 :
   Functions which reads device-id,chip-id,power configuration,
   system configuration2 registers
 */
int Si4709_dev_device_id(struct device_id *dev_id)
{
	int ret = 0;

	debug("Si4709_dev_device_id called");
#if 0 /*To Do*/
	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_device_id called when DS is invalid");
		ret = -1;
	} else {
		ret = i2c_read(DEVICE_ID);
		if (ret < 0) {
			debug("Si4709_dev_device_id i2c_read failed");
		} else {
			dev_id->part_number =
				DEVICE_ID_PART_NUMBER(Si4709_dev.
						registers[DEVICE_ID]);
			dev_id->manufact_number =
				DEVICE_ID_MANUFACT_NUMBER(Si4709_dev.
						registers[DEVICE_ID]);
		}
	}

	mutex_unlock(&(Si4709_dev.lock));
#endif
	return ret;
}

int Si4709_dev_chip_id(struct chip_id *chp_id)
{
	int ret = 0;

	debug("Si4709_dev_chip_id called");
#if 0 /*To Do*/
	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_chip_id called when DS is invalid");
		ret = -1;
	} else {
		ret = i2c_read(CHIP_ID);
		if (ret < 0) {
			debug("Si4709_dev_chip_id i2c_read failed");
		} else {
			chp_id->chip_version =
				CHIP_ID_CHIP_VERSION(
						Si4709_dev.registers[CHIP_ID]);
			chp_id->device =
				CHIP_ID_DEVICE(Si4709_dev.registers[CHIP_ID]);
			chp_id->firmware_version =
				CHIP_ID_FIRMWARE_VERSION(Si4709_dev.
						registers[CHIP_ID]);
		}
	}

	mutex_unlock(&(Si4709_dev.lock));
#endif
	return ret;
}

int Si4709_dev_sys_config2(struct sys_config2 *sys_conf2)
{
	int ret = 0;

	debug("Si4709_sys_config2 called\n");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_sys_config2 called when DS is invalid");
		ret = -1;
	} else {
		if (ret < 0) {
			debug("Si4709_sys_config2 i2c_read failed");
		} else {
			sys_conf2->rssi_th = Si4709_dev.settings.curr_rssi_th;
			sys_conf2->fm_band = Si4709_dev.settings.band;
			sys_conf2->fm_chan_spac =
				Si4709_dev.settings.channel_spacing;
			sys_conf2->fm_vol = 0;
		}
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_sys_config3(struct sys_config3 *sys_conf3)
{
	int ret = 0;

	debug("Si4709_sys_config3 called\n");
	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_sys_config3 called when DS is invalid");
		mutex_unlock(&(Si4709_dev.lock));
		return  -1;
	}
	if (ret < 0) {
		debug("Si4709_sys_config3 i2c_read failed");
	} else {
		sys_conf3->smmute = 0;
		sys_conf3->smutea = 0;
		sys_conf3->volext = 0;
		sys_conf3->sksnr = Si4709_dev.settings.curr_snr;
		sys_conf3->skcnt = 0;
	}
	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_status_rssi(struct status_rssi *status)
{
	int ret = 0;

	debug("Si4709_dev_status_rssi called");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_status_rssi called when DS is invalid");
		mutex_unlock(&(Si4709_dev.lock));
		return  -1;
	}
	fmRsqStatus(0);
	if (ret < 0) {
		debug("Si4709_sys_config3 i2c_read failed");
	} else {
		debug("Si4709_dev_status_rssi %d\n", RSSI);
		status->rssi = RSSI;
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_sys_config2_set(struct sys_config2 *sys_conf2)
{
	int ret = 0;
	u16 register_bak = 0;

	debug("Si4709_dev_sys_config2_set called");

	mutex_lock(&(Si4709_dev.lock));

	register_bak = Si4709_dev.registers[SYSCONFIG2];

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_sys_config2_set called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_SEEK_TUNE_RSSI_THRESHOLD,
				sys_conf2->rssi_th);
		Si4709_dev_band_set(sys_conf2->fm_band);
		si47xx_set_property(FM_SEEK_FREQ_SPACING,
			sys_conf2->fm_chan_spac);
		Si4709_dev.settings.curr_rssi_th = sys_conf2->rssi_th;
		Si4709_dev.settings.band = sys_conf2->fm_band;
		Si4709_dev.settings.channel_spacing = sys_conf2->fm_chan_spac;

		if (ret < 0) {
			debug("Si4709_dev_sys_config2_set i2c_write 1 failed");
		} else
			debug(KERN_ERR "Si4709_dev_sys_config2_set() "
					": Write Sucess!!");
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_sys_config3_set(struct sys_config3 *sys_conf3)
{
	int ret = 0;
	u16 register_bak = 0;

	debug("Si4709_dev_sys_config3_set called");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_sys_config3_set called "
				"when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_SEEK_TUNE_SNR_THRESHOLD,
			sys_conf3->sksnr);
		Si4709_dev.settings.curr_snr = sys_conf3->sksnr;
		if (ret < 0) {
			debug("Si4709_dev_sys_config3_set i2c_write 1 failed");
			Si4709_dev.registers[SYSCONFIG3] = register_bak;
		}
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_power_config(struct power_config *pow_conf)
{
	int ret = 0;

	debug("Si4709_dev_power_config called");

	return ret;
}

/* VNVS:END */

/* VNVS:START 18-NOV'09 */
/* Reading AFCRL Bit */
int Si4709_dev_AFCRL_get(u8 *afc)
{
	int ret = 0;

	debug("Si4709_dev_AFCRL_get called");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_AFCRL_get called when DS is invalid");
		ret = -1;
	} else {
		fmRsqStatus(0);
		if (ret < 0)
			debug("Si4709_dev_AFCRL_get i2c_read failed");
		*afc = AFCRL;
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

/* Setting DE
   emphasis time constant 50us(Europe,Japan,Australia) or 75us(USA)
 */
int Si4709_dev_DE_set(u8 de_tc)
{
	u16 sysconfig1 = 0;
	int ret = 0;

	debug("Si4709_dev_DE_set called");

	mutex_lock(&(Si4709_dev.lock));

	sysconfig1 = Si4709_dev.registers[SYSCONFIG1];

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_DE_set called when DS is invalid");
		ret = -1;
	} else {
		switch (de_tc) {
		case DE_TIME_CONSTANT_50:
			si47xx_set_property(FM_DEEMPHASIS, FM_DEEMPH_50US);
		break;

		case DE_TIME_CONSTANT_75:
			si47xx_set_property(FM_DEEMPHASIS, FM_DEEMPH_75US);
		break;

		default:
				ret = -1;
		}

		if (0 == ret) {
			if (ret < 0)
				debug("Si4709_dev_DE_set i2c_write failed");
		}
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

/*Resetting the RDS Data Buffer*/
int Si4709_dev_reset_rds_data()
{
	int ret = 0;

	debug_rds("Si4709_dev_reset_rds_data called");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_reset_rds_data called when DS is invalid");
		ret = -1;
	} else {
		RDS_Buffer_Index_write = 0;
		RDS_Buffer_Index_read = 0;
		RDS_Data_Lost = 0;
		RDS_Data_Available = 0;
		memset(RDS_Block_Data_buffer, 0, RDS_BUFFER_LENGTH * 8);
		memset(RDS_Block_Error_buffer, 0, RDS_BUFFER_LENGTH * 4);
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

/*VNVS:END*/

int Si4709_dev_VOLEXT_ENB(void)
{
	int ret = 0;

	debug("Si4709_dev_VOLEXT_ENB called");

	return ret;
}

int Si4709_dev_VOLEXT_DISB(void)
{
	int ret = 0;

	debug("Si4709_dev_VOLEXT_DISB called");

	return ret;
}

int Si4709_dev_volume_set(u8 volume)
{
	int ret = 0;
	u16 sysconfig2 = 0;

	debug("Si4709_dev_volume_set called");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_volume_set called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(RX_VOLUME, rx_vol[volume] & RX_VOLUME_MASK);

		if (ret < 0) {
			debug("Si4709_dev_volume_set i2c_write failed");
			Si4709_dev.registers[SYSCONFIG2] = sysconfig2;
		}
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_volume_get(u8 *volume)
{
	int ret = 0;

	debug("Si4709_dev_volume_get called");
#if 0 /*To Do*/
	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_volume_get called when DS is invalid");
		ret = -1;
	} else
		*volume =
			SYSCONFIG2_GET_VOLUME(Si4709_dev.registers[SYSCONFIG2]);

	mutex_unlock(&(Si4709_dev.lock));
#endif
	return ret;
}

/*
VNVS:START 19-AUG'10 : Adding DSMUTE ON/OFF feature.
The Soft Mute feature is available to attenuate the audio
outputs and minimize audible noise in very weak signal conditions.
 */
int Si4709_dev_DSMUTE_ON(void)
{
	int ret = 0;
	u16 powercfg = 0;

	debug("Si4709_dev_DSMUTE_ON called");

	mutex_lock(&(Si4709_dev.lock));

	powercfg = Si4709_dev.registers[POWERCFG];

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_DSMUTE_ON called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_SOFT_MUTE_RATE, 64);
		si47xx_set_property(FM_SOFT_MUTE_MAX_ATTENUATION, 0);
		si47xx_set_property(FM_SOFT_MUTE_SNR_THRESHOLD, 4);
		if (ret < 0)
			error("Si4709_dev_DSMUTE_ON i2c_write failed");
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_DSMUTE_OFF(void)
{
	int ret = 0;
	u16 powercfg = 0;

	debug("Si4709_dev_DSMUTE_OFF called");

	mutex_lock(&(Si4709_dev.lock));

	powercfg = Si4709_dev.registers[POWERCFG];

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_DSMUTE_OFF called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_SOFT_MUTE_RATE, 64);
		si47xx_set_property(FM_SOFT_MUTE_MAX_ATTENUATION, 16);
		si47xx_set_property(FM_SOFT_MUTE_SNR_THRESHOLD, 4);
		if (ret < 0)
			error("Si4709_dev_DSMUTE_OFF i2c_write failed");
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

/*VNVS:END*/

int Si4709_dev_MUTE_ON(void)
{
	int ret = 0;
	u16 powercfg = 0;

	debug("Si4709_dev_MUTE_ON called");

	mutex_lock(&(Si4709_dev.lock));

	powercfg = Si4709_dev.registers[POWERCFG];

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_MUTE_ON called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(RX_HARD_MUTE,
			RX_HARD_MUTE_RMUTE_MASK |
			RX_HARD_MUTE_LMUTE_MASK);
		if (ret < 0)
			debug("Si4709_dev_MUTE_ON i2c_write failed");
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_MUTE_OFF(void)
{
	int ret = 0;
	u16 powercfg = 0;

	debug("Si4709_dev_MUTE_OFF called");

	mutex_lock(&(Si4709_dev.lock));

	powercfg = Si4709_dev.registers[POWERCFG];

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_MUTE_OFF called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(RX_HARD_MUTE, 0);
		if (ret < 0)
			debug("Si4709_dev_MUTE_OFF i2c_write failed");
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_MONO_SET(void)
{
	int ret = 0;
	u16 powercfg = 0;

	debug("Si4709_dev_MONO_SET called");

	mutex_lock(&(Si4709_dev.lock));

	powercfg = Si4709_dev.registers[POWERCFG];

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_MONO_SET called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_BLEND_MONO_THRESHOLD, 127);
		si47xx_set_property(FM_BLEND_STEREO_THRESHOLD, 127);
		if (ret < 0)
			debug("Si4709_dev_MONO_SET i2c_write failed");
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_STEREO_SET(void)
{
	int ret = 0;
	u16 powercfg = 0;

	debug("Si4709_dev_STEREO_SET called");

	mutex_lock(&(Si4709_dev.lock));

	powercfg = Si4709_dev.registers[POWERCFG];

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_STEREO_SET called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_BLEND_MONO_THRESHOLD, 30);
		si47xx_set_property(FM_BLEND_STEREO_THRESHOLD, 49);
		if (ret < 0)
			debug("Si4709_dev_STEREO_SET i2c_write failed");
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_RDS_ENABLE(void)
{
	int ret = 0;

	debug("Si4709_dev_RDS_ENABLE called");

	mutex_lock(&(Si4709_dev.lock));
	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_RDS_ENABLE called when DS is invalid");
		ret = -1;
	} else {
#ifdef RDS_INTERRUPT_ON_ALWAYS
		si47xx_set_property(GPO_IEN, GPO_IEN_STCIEN_MASK |
		GPO_IEN_STCREP_MASK | GPO_IEN_RDSIEN_MASK |
		GPO_IEN_RDSREP_MASK);
#endif
		si47xx_set_property(FM_RDS_INTERRUPT_SOURCE,
					FM_RDS_INTERRUPT_SOURCE_RECV_MASK);
		si47xx_set_property(FM_RDS_CONFIG, FM_RDS_CONFIG_RDSEN_MASK |
			(3 << FM_RDS_CONFIG_BLETHA_SHFT) |
			(3 << FM_RDS_CONFIG_BLETHB_SHFT) |
			(3 << FM_RDS_CONFIG_BLETHC_SHFT) |
			(3 << FM_RDS_CONFIG_BLETHD_SHFT));
		if (ret < 0)
			debug("Si4709_dev_RDS_ENABLE i2c_write failed");
#ifdef RDS_INTERRUPT_ON_ALWAYS
		else
			Si4709_RDS_flag = RDS_WAITING;
#endif
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_RDS_DISABLE(void)
{
	int ret = 0;

	debug("Si4709_dev_RDS_DISABLE called");

	mutex_lock(&(Si4709_dev.lock));
	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_RDS_DISABLE called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_RDS_CONFIG, 0);

		if (ret < 0)
			debug("Si4709_dev_RDS_DISABLE i2c_write failed");
#ifdef RDS_INTERRUPT_ON_ALWAYS
		else
			Si4709_RDS_flag = NO_WAIT;
#endif
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_rstate_get(struct dev_state_t *dev_state)
{
	int ret = 0;

	debug("Si4709_dev_rstate_get called");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_rstate_get called when DS is invalid");
		ret = -1;
	} else {
		dev_state->power_state = Si4709_dev.state.power_state;
		dev_state->seek_state = Si4709_dev.state.seek_state;
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

/* VNVS:START 7-JUNE'10 Function call for work-queue "Si4709_wq" */
#ifdef RDS_INTERRUPT_ON_ALWAYS
void Si4709_work_func(struct work_struct *work)
{
	int i = 0;
#ifdef RDS_TESTING
	u8 group_type;
#endif
	debug_rds("%s", __func__);
mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		error("Si4709_dev_RDS_data_get called when DS is invalid");
		return;
	}

	if (RDS_Data_Lost > 1)
		debug_rds("No_of_RDS_groups_Lost till now : %d",
				RDS_Data_Lost);
	fmRdsStatus(1, 0);
	/* RDSR bit and RDS Block data, so reading the RDS registers */
	do {
		/* Writing into RDS_Block_Data_buffer */
		i = 0;
		RDS_Block_Data_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			BlockA;
		RDS_Block_Data_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			BlockB;
		RDS_Block_Data_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			BlockC;
		RDS_Block_Data_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			BlockD;

		/*Writing into RDS_Block_Error_buffer */
		i = 0;

		RDS_Block_Error_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			BleA;
		RDS_Block_Error_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			BleB;
		RDS_Block_Error_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			BleC;
		RDS_Block_Error_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			BleD;
		fmRdsStatus(1, 0);
	} while (RdsFifoUsed != 0);

#ifdef RDS_TESTING
	if (RDS_Block_Error_buffer
			[0 + 4 * RDS_Buffer_Index_write] < 3) {
		debug_rds("PI Code is %d",
				RDS_Block_Data_buffer[0 + 4
				* RDS_Buffer_Index_write]);
	}
	if (RDS_Block_Error_buffer
			[1 + 4 * RDS_Buffer_Index_write] < 2) {
		group_type = RDS_Block_Data_buffer[1 + 4
			* RDS_Buffer_Index_write] >> 11;

		if (group_type & 0x01) {
			debug_rds("PI Code is %d",
					RDS_Block_Data_buffer[2 + 4
					* RDS_Buffer_Index_write]);
		}
		if (group_type == GROUP_TYPE_2A
				|| group_type == GROUP_TYPE_2B) {
			if (RDS_Block_Error_buffer
					[2 + 4 * RDS_Buffer_Index_write] < 3) {
				debug_rds("Update RT with RDSC");
			} else {
				debug_rds("RDS_Block_Error_buffer"
						" of Block C is greater than 3");
			}
		}
	}
#endif
	RDS_Buffer_Index_write++;

	if (RDS_Buffer_Index_write >= RDS_BUFFER_LENGTH)
		RDS_Buffer_Index_write = 0;

	debug_rds("RDS_Buffer_Index_write = %d", RDS_Buffer_Index_write);
	mutex_unlock(&(Si4709_dev.lock));
}
#endif
/*VNVS:END*/

int Si4709_dev_RDS_data_get(struct radio_data_t *data)
{
	int i, ret = 0;

	debug_rds("Si4709_dev_RDS_data_get called");

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		error("Si4709_dev_RDS_data_get called when DS is invalid");
		mutex_unlock(&(Si4709_dev.lock));
		return  -1;
	}
#ifdef RDS_INTERRUPT_ON_ALWAYS
	debug_rds("RDS_Buffer_Index_read = %d", RDS_Buffer_Index_read);

	/*If No New RDS Data is available return error */
	if (RDS_Buffer_Index_read == RDS_Buffer_Index_write) {
		debug_rds("No_New_RDS_Data_is_available");
		if (ret < 0)
			error("Si4709_dev_RDS_data_get i2c_read 1 failed");
		else {
			fmTuneStatus(0, 0);
			data->curr_channel = Freq;
			data->curr_rssi = RSSI;
			debug_rds("curr_channel: %u, curr_rssi:%u",
					data->curr_channel,
					(u32) data->curr_rssi);
		}
		mutex_unlock(&(Si4709_dev.lock));
		return -1;
	}
	if (ret < 0)
		error("Si4709_dev_RDS_data_get i2c_read 2 failed");
	else {
		fmTuneStatus(0, 0);
		data->curr_channel = Freq;
		data->curr_rssi = RSSI;
		debug_rds("curr_channel: %u, curr_rssi:%u",
				data->curr_channel, (u32) data->curr_rssi);

		/* Reading from RDS_Block_Data_buffer */
		i = 0;
		data->rdsa = RDS_Block_Data_buffer[i++ + 4
			* RDS_Buffer_Index_read];
		data->rdsb = RDS_Block_Data_buffer[i++ + 4
			* RDS_Buffer_Index_read];
		data->rdsc = RDS_Block_Data_buffer[i++ + 4
			* RDS_Buffer_Index_read];
		data->rdsd = RDS_Block_Data_buffer[i++ + 4
			* RDS_Buffer_Index_read];

		/* Reading from RDS_Block_Error_buffer */
		i = 0;
		data->blera = RDS_Block_Error_buffer[i++ + 4
			* RDS_Buffer_Index_read];
		data->blerb = RDS_Block_Error_buffer[i++ + 4
			* RDS_Buffer_Index_read];
		data->blerc = RDS_Block_Error_buffer[i++ + 4
			* RDS_Buffer_Index_read];
		data->blerd = RDS_Block_Error_buffer[i++ + 4
			* RDS_Buffer_Index_read];

		/*Flushing the read data */
		memset(&RDS_Block_Data_buffer[0 + 4 * RDS_Buffer_Index_read],
				0, 8);
		memset(&RDS_Block_Error_buffer[0 + 4 * RDS_Buffer_Index_read],
				0, 4);

		RDS_Buffer_Index_read++;

		if (RDS_Buffer_Index_read >= RDS_BUFFER_LENGTH)
			RDS_Buffer_Index_read = 0;
	}

	debug_rds("RDS_Buffer_Index_read = %d", RDS_Buffer_Index_read);
#else
#if 0 /*To Do*/
	SYSCONFIG1_BITSET_RDSIEN_HIGH(&Si4709_dev.registers[SYSCONFIG1]);

	ret = i2c_write(SYSCONFIG1);
	if (ret < 0) {
		debug("Si4709_dev_RDS_data_get i2c_write 1 failed");
		Si4709_dev.registers[SYSCONFIG1] = sysconfig1;
	} else {
		ret = i2c_read(SYSCONFIG1);
		if (ret < 0)
			debug("Si4709_dev_RDS_data_get i2c_read 1 failed");

		debug("sysconfig1: 0x%x", Si4709_dev.registers[SYSCONFIG1]);

		sysconfig1 = Si4709_dev.registers[SYSCONFIG1];

		Si4709_dev_wait_flag = RDS_WAITING;

		wait_RDS();

		ret = i2c_read(STATUSRSSI);
		if (ret < 0)
			debug("Si4709_dev_RDS_data_get i2c_read 2 failed");

		debug("statusrssi: 0x%x", Si4709_dev.registers[STATUSRSSI]);

		SYSCONFIG1_BITSET_RDSIEN_LOW(&Si4709_dev.registers[SYSCONFIG1]);

		ret = i2c_write(SYSCONFIG1);
		if (ret < 0) {
			debug("Si4709_dev_RDS_data_get i2c_write 2 failed");
			Si4709_dev.registers[SYSCONFIG1] = sysconfig1;
		} else if (Si4709_dev_wait_flag == WAIT_OVER) {
			Si4709_dev_wait_flag = NO_WAIT;

			ret = i2c_read(RDSD);
			if (ret < 0)
				debug("Si4709_dev_RDS_data_get "
					"i2c_read 3 failed");
			else {
				data->rdsa = Si4709_dev.registers[RDSA];
				data->rdsb = Si4709_dev.registers[RDSB];
				data->rdsc = Si4709_dev.registers[RDSC];
				data->rdsd = Si4709_dev.registers[RDSD];

				get_cur_chan_freq(&(data->curr_channel),
						Si4709_dev.registers[READCHAN]);
				debug("curr_channel: %u", data->curr_channel);
				data->curr_rssi =
					STATUSRSSI_RSSI_SIGNAL_STRENGTH
					(Si4709_dev.registers[STATUSRSSI]);
				debug("curr_rssi:%u", (u32)data->curr_rssi);
				data->blera =
					STATUSRSSI_RDS_BLOCK_A_ERRORS
					(Si4709_dev.registers[STATUSRSSI]);
				data->blerb =
					READCHAN_BLOCK_B_ERRORS(Si4709_dev.
							registers[READCHAN]);
				data->blerc =
					READCHAN_BLOCK_C_ERRORS(Si4709_dev.
							registers[READCHAN]);
				data->blerd =
					READCHAN_BLOCK_D_ERRORS(Si4709_dev.
							registers[READCHAN]);
			}
		} else {
			debug("Si4709_dev_RDS_data_get failure "
				"no interrupt or timeout");
			Si4709_dev_wait_flag = NO_WAIT;
			mutex_unlock(&(Si4709_dev.lock));
			return -1;
		}
	}
#endif
#endif

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

int Si4709_dev_RDS_timeout_set(u32 time_out)
{
	int ret = 0;
	u32 jiffy_count = 0;

	debug("Si4709_dev_RDS_timeout_set called");
	/****convert time_out(in milliseconds) into jiffies*****/

	jiffy_count = msecs_to_jiffies(time_out);

	debug("jiffy_count%d", jiffy_count);

	mutex_lock(&(Si4709_dev.lock));

	if (Si4709_dev.valid == eFALSE) {
		debug("Si4709_dev_RDS_timeout_set called when DS is invalid");
		ret = -1;
	} else {
		Si4709_dev.settings.timeout_RDS = jiffy_count;
	}

	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

/**************************************************************/
static int powerup(void)
{
	int ret = 0;
	u16 powercfg = Si4709_dev.registers[POWERCFG];

#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M0_CTC)
	gpio_set_value(Si4709_dev_sw, GPIO_LEVEL_HIGH);
#endif

	gpio_set_value(GPIO_FM_RST, GPIO_LEVEL_LOW);
	usleep_range(5, 10);
	s3c_gpio_cfgpin(Si4709_dev_int, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(Si4709_dev_int, S3C_GPIO_PULL_DOWN);
	gpio_set_value(Si4709_dev_int, GPIO_LEVEL_LOW);
	usleep_range(10, 15);
	gpio_set_value(GPIO_FM_RST, GPIO_LEVEL_HIGH);
	usleep_range(5, 10);
	s3c_gpio_cfgpin(Si4709_dev_int, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(Si4709_dev_int, S3C_GPIO_PULL_UP);
	usleep_range(10, 15);

	cmd[0] = POWER_UP;
	cmd[1] = POWER_UP_IN_GPO2OEN;
	cmd[1] |= POWER_UP_IN_FUNC_FMRX;
	cmd[2] = POWER_UP_IN_OPMODE_RX_ANALOG;
	ret = si47xx_command(3, cmd, 8, rsp);

	if (ret < 0) {
		debug("powerup->i2c_write 1 failed");
		Si4709_dev.registers[POWERCFG] = powercfg;
	} else {
		/* Si4709/09 datasheet: Table 7 */
		msleep(110);
		Si4709_dev.state.power_state = RADIO_ON;
	}
	return ret;
}

static int powerdown(void)
{
	int ret = 0;

	if (!(RADIO_POWERDOWN == Si4709_dev.state.power_state)) {
		cmd[0] = POWER_DOWN;
		ret = si47xx_command(1, cmd, 1, rsp);
		msleep(110);

		if (ret < 0)
			debug("powerdown->i2c_write failed");
		else
			Si4709_dev.state.power_state = RADIO_POWERDOWN;

		gpio_set_value(GPIO_FM_RST, GPIO_LEVEL_LOW);
	} else
		debug("Device already Powered-OFF\n");

#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M0_CTC)
	gpio_set_value(Si4709_dev_sw, GPIO_LEVEL_LOW);
#endif

	return ret;
}

static int seek(u32 *frequency, int up, int mode)
{
	int ret = 0;
	u8 get_int;

	if (ret < 0) {
		debug("seek i2c_write 1 failed");
	} else {
		Si4709_dev_wait_flag = SEEK_WAITING;
		fmSeekStart(up, mode); /* mode 0 is full scan */
		wait();
		do {
			get_int = getIntStatus();
		} while (!(get_int & STCINT));

		if (Si4709_dev_wait_flag == SEEK_CANCEL) {
			fmTuneStatus(1, 0);
			if (ret < 0) {
				debug("seek i2c_write 2 failed");
			}
			if (ret < 0)
				debug("seek i2c_read 1 failed");
			else
				*frequency = Freq;

			*frequency = 0;
		}

		Si4709_dev_wait_flag = NO_WAIT;

		fmTuneStatus(0, 1);
		if (BLTF != 1)
			*frequency = Freq;

		else {
			if (Valid)
				*frequency = Freq;
			else
				*frequency = 0;
		}
	}
	return ret;
}

static int tune_freq(u32 frequency)
{
	int ret = 0;

	u16 channel = Si4709_dev.registers[CHANNEL];
	mutex_lock(&(Si4709_dev.lock));

	channel = freq_to_channel(frequency);
	if (ret < 0) {
		debug("tune_freq i2c_write 1 failed");
		Si4709_dev.registers[CHANNEL] = channel;
	} else {
		Si4709_dev_wait_flag = TUNE_WAITING;
		fmTuneFreq(frequency);
		wait();
		Si4709_dev_wait_flag = NO_WAIT;
		debug("Si4709_dev_wait_flag = TUNE_WAITING\n");

		if (!(getIntStatus() & STCINT)) {
			printk(KERN_INFO "%s tune is failed!\n", __func__);
			fmTuneStatus(1, 1);
			mutex_unlock(&(Si4709_dev.lock));
			return -1;
		}

		fmTuneStatus(0, 1);

		debug("Si4709 tune_freq fmTuneStatus %x\n", rsp[0]);

	}
	mutex_unlock(&(Si4709_dev.lock));

	return ret;
}

static u16 freq_to_channel(u32 frequency)
{
	u16 channel;

	if (frequency < Si4709_dev.settings.bottom_of_band)
		frequency = Si4709_dev.settings.bottom_of_band;

	channel = (frequency - Si4709_dev.settings.bottom_of_band)
		/ Si4709_dev.settings.channel_spacing;

	return channel;
}

/* Only one thread will be able to call this, since this function call is
   protected by a mutex, so no race conditions can arise */
static void wait(void)
{
	wait_event_interruptible(Si4709_waitq,
			(Si4709_dev_wait_flag == WAIT_OVER) ||
			(Si4709_dev_wait_flag == SEEK_CANCEL));
}

#ifndef RDS_INTERRUPT_ON_ALWAYS
static void wait_RDS(void)
{
	wait_event_interruptible_timeout(Si4709_waitq,
			(Si4709_dev_wait_flag == WAIT_OVER),
			Si4709_dev.settings.timeout_RDS);
}
#endif

/*-----------------------------------------------------------------------------
 Helper function that sends the GET_INT_STATUS command to the part

 Returns:
   The status byte from the part.
-----------------------------------------------------------------------------*/
u8 getIntStatus(void)
{
	cmd[0] = GET_INT_STATUS;
	si47xx_command(1, cmd, 1, rsp);

	debug("Si4709 getIntStatus return %x\n", rsp[0]);
	return rsp[0];
}

/*-----------------------------------------------------------------------------
 Helper function that sends the FM_TUNE_FREQ command to the part

 Inputs:
 frequency in 10kHz steps
-----------------------------------------------------------------------------*/
static void fmTuneFreq(u16 frequency)
{
	cmd[0] = FM_TUNE_FREQ;
	cmd[1] = 0;
	cmd[2] = (u8)(frequency >> 8);
	cmd[3] = (u8)(frequency & 0x00FF);
	cmd[4] = (u8)0;
	si47xx_command(5, cmd, 1, rsp);
}

/*-----------------------------------------------------------------------------
 Helper function that sends the FM_TUNE_STATUS command to the part

 Inputs:
 cancel: If non-zero the current seek will be cancelled.
 intack: If non-zero the interrupt for STCINT will be cleared.

Outputs:  These are global variables and are set by this method
STC:    The seek/tune is complete
BLTF:   The seek reached the band limit or original start frequency
AFCRL:  The AFC is railed if this is non-zero
Valid:  The station is valid if this is non-zero
Freq:   The current frequency
RSSI:   The RSSI level read at tune.
ASNR:   The audio SNR level read at tune.
AntCap: The current level of the tuning capacitor.
-----------------------------------------------------------------------------*/

static void fmTuneStatus(u8 cancel, u8 intack)
{
	cmd[0] = FM_TUNE_STATUS;
	cmd[1] = 0;
	if (cancel)
		cmd[1] |= FM_TUNE_STATUS_IN_CANCEL;
	if (intack)
		cmd[1] |= FM_TUNE_STATUS_IN_INTACK;

	si47xx_command(2, cmd, 8, rsp);

	STC    = !!(rsp[0] & STCINT);
	BLTF   = !!(rsp[1] & FM_TUNE_STATUS_OUT_BTLF);
	AFCRL  = !!(rsp[1] & FM_TUNE_STATUS_OUT_AFCRL);
	Valid  = !!(rsp[1] & FM_TUNE_STATUS_OUT_VALID);
	Freq   = ((u16)rsp[2] << 8) | (u16)rsp[3];
	RSSI   = rsp[4];
	ASNR   = rsp[5];
	AntCap = rsp[7];
}

/*-----------------------------------------------------------------------------
 Helper function that sends the FM_RDS_STATUS command to the part

 Inputs:
  intack: If non-zero the interrupt for STCINT will be cleared.
  mtfifo: If non-zero the fifo will be cleared.

Outputs:
Status:      Contains bits about the status returned from the part.
RdsInts:     Contains bits about the interrupts that have fired
related to RDS.
RdsSync:     If non-zero the RDS is currently synced.
GrpLost:     If non-zero some RDS groups were lost.
RdsFifoUsed: The amount of groups currently remaining
in the RDS fifo.
BlockA:      Block A group data from the oldest FIFO entry.
BlockB:      Block B group data from the oldest FIFO entry.
BlockC:      Block C group data from the oldest FIFO entry.
BlockD:      Block D group data from the oldest FIFO entry.
BleA:        Block A corrected error information.
BleB:        Block B corrected error information.
BleC:        Block C corrected error information.
BleD:        Block D corrected error information.
-----------------------------------------------------------------------------*/
void fmRdsStatus(u8 intack, u8 mtfifo)
{
	cmd[0] = FM_RDS_STATUS;
	cmd[1] = 0;
	if (intack)
		cmd[1] |= FM_RDS_STATUS_IN_INTACK;
	if (mtfifo)
		cmd[1] |= FM_RDS_STATUS_IN_MTFIFO;

	si47xx_command(2, cmd, 13, rsp);

	Status      = rsp[0];
	RdsInts     = rsp[1];
	RdsSync     = !!(rsp[2] & FM_RDS_STATUS_OUT_SYNC);
	GrpLost     = !!(rsp[2] & FM_RDS_STATUS_OUT_GRPLOST);
	RdsFifoUsed = rsp[3];
	BlockA      = ((u16)rsp[4] << 8) | (u16)rsp[5];
	BlockB      = ((u16)rsp[6] << 8) | (u16)rsp[7];
	BlockC      = ((u16)rsp[8] << 8) | (u16)rsp[9];
	BlockD      = ((u16)rsp[10] << 8) | (u16)rsp[11];
	BleA        = (rsp[12] & FM_RDS_STATUS_OUT_BLEA) >>
		FM_RDS_STATUS_OUT_BLEA_SHFT;
	BleB        = (rsp[12] & FM_RDS_STATUS_OUT_BLEB) >>
		FM_RDS_STATUS_OUT_BLEB_SHFT;
	BleC        = (rsp[12] & FM_RDS_STATUS_OUT_BLEC) >>
		FM_RDS_STATUS_OUT_BLEC_SHFT;
	BleD        = (rsp[12] & FM_RDS_STATUS_OUT_BLED) >>
		FM_RDS_STATUS_OUT_BLED_SHFT;
}


/*-----------------------------------------------------------------------------
 Helper function that sends the FM_RSQ_STATUS command to the part

 Inputs:
  intack: If non-zero the interrupt for STCINT will be cleared.

Outputs:
Status:  Contains bits about the status returned from the part.
RsqInts: Contains bits about the interrupts that have fired related to RSQ.
SMUTE:   The soft mute function is currently enabled
AFCRL:   The AFC is railed if this is non-zero
Valid:   The station is valid if this is non-zero
Pilot:   A pilot tone is currently present
Blend:   Percentage of blend for stereo. (100 = full stereo)
RSSI:    The RSSI level read at tune.
ASNR:    The audio SNR level read at tune.
FreqOff: The frequency offset in kHz of the current station from the tuned
frequency.
-----------------------------------------------------------------------------*/
static void fmRsqStatus(u8 intack)
{
	cmd[0] = FM_RSQ_STATUS;
	cmd[1] = 0;
	if (intack)
		cmd[1] |= FM_RSQ_STATUS_IN_INTACK;

	si47xx_command(2, cmd, 8, rsp);

	Status  = rsp[0];
	RsqInts = rsp[1];
	SMUTE   = !!(rsp[2] & FM_RSQ_STATUS_OUT_SMUTE);
	AFCRL   = !!(rsp[2] & FM_RSQ_STATUS_OUT_AFCRL);
	Valid   = !!(rsp[2] & FM_RSQ_STATUS_OUT_VALID);
	Pilot   = !!(rsp[3] & FM_RSQ_STATUS_OUT_PILOT);
	Blend   = rsp[3] & FM_RSQ_STATUS_OUT_STBLEND;
	RSSI    = rsp[4];
	ASNR    = rsp[5];
	FreqOff = rsp[7];
}


/*-----------------------------------------------------------------------------
 Helper function that sends the FM_SEEK_START command to the part

Inputs:
seekUp: If non-zero seek will increment otherwise decrement
wrap:   If non-zero seek will wrap around band limits when hitting the end
of the band limit.
-----------------------------------------------------------------------------*/
static void fmSeekStart(u8 seekUp, u8 wrap)
{
	cmd[0] = FM_SEEK_START;
	cmd[1] = 0;
	if (seekUp)
		cmd[1] |= FM_SEEK_START_IN_SEEKUP;
	if (wrap)
		cmd[1] |= FM_SEEK_START_IN_WRAP;

	si47xx_command(2, cmd, 1, rsp);
}


/*-----------------------------------------------------------------------------
 Set the passed property number to the passed value.

 Inputs:
      propNumber:  The number identifying the property to set
      propValue:   The value of the property.
-----------------------------------------------------------------------------*/
void si47xx_set_property(u16 propNumber, u16 propValue)
{
	cmd[0] = SET_PROPERTY;
	cmd[1] = 0;
	cmd[2] = (u8)(propNumber >> 8);
	cmd[3] = (u8)(propNumber & 0x00FF);
	cmd[4] = (u8)(propValue >> 8);
	cmd[5] = (u8)(propValue & 0x00FF);

	si47xx_command(6, cmd, 0, NULL);
}

/*-----------------------------------------------------------------------------
 This command returns the status
-----------------------------------------------------------------------------*/
u8 si47xx_readStatus(void)
{
	u8 status;
	i2c_read(1, &status);

	return status;
}


/*-----------------------------------------------------------------------------
 Command that will wait for CTS before returning
-----------------------------------------------------------------------------*/
void si47xx_waitForCTS(void)
{
	u16 i = 1000;
	u8 rStatus = 0;

	do {
		rStatus = si47xx_readStatus();
		usleep_range(5, 10);
	} while (--i && !(rStatus & CTS));
}

/*-----------------------------------------------------------------------------
 Sends a command to the part and returns the reply bytes
-----------------------------------------------------------------------------*/
int si47xx_command(u8 cmd_size, u8 *cmd, u8 reply_size, u8 *reply)
{
	int ret = 0;
	ret = i2c_write(cmd_size, cmd);
	si47xx_waitForCTS();

	if (reply_size)
		i2c_read(reply_size, reply);

	if (ret < 0)
		printk(KERN_INFO "%s i2c_write failed %d\n", __func__, ret);

	return ret;
}

static int i2c_write(u8 number_bytes, u8 *data_out)
{
	int ret = 0;

	ret = i2c_master_send((struct i2c_client *)(Si4709_dev.client),
			(const char *)data_out, number_bytes);
	if (ret == number_bytes)
		ret = 0;
	else
		ret = -1;

	return ret;
}

static int i2c_read(u8 number_bytes, u8 *data_in)
{
	int ret = 0;

	ret = i2c_master_recv((struct i2c_client *)(Si4709_dev.client), data_in,
			number_bytes);
	if (ret < 0)
		printk(KERN_INFO "%s i2c_read failed %d\n", __func__, ret);

	return ret;
}
