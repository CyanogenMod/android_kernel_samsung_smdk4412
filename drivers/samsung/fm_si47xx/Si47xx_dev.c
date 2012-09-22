/* drivers/samsung/fm_si47xx/Si47xx_dev.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include "Si47xx_dev.h"
#include <linux/i2c/si47xx_common.h>

#include "commanddefs.h"
#include "propertydefs.h"


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

int Si47xx_RDS_flag;
int RDS_Data_Available;
int RDS_Data_Lost;
int RDS_Groups_Available_till_now;
struct workqueue_struct *Si47xx_wq;
struct work_struct Si47xx_work;
#endif

/*Si47xx device structure*/
static struct Si47xx_device_t *Si47xx_dev;
static struct si47xx_platform_data *pSi47xxdata;

static int si47xx_irq;
wait_queue_head_t Si47xx_waitq;

/*Wait flag*/
/*WAITING or WAIT_OVER or NO_WAIT*/
int Si47xx_dev_wait_flag = NO_WAIT;
#ifdef RDS_INTERRUPT_ON_ALWAYS
int Si47xx_RDS_flag = NO_WAIT;
#endif

static irqreturn_t Si47xx_isr(int irq, void *unused)
{
	debug("Si47xx_isr: FM device called IRQ: %d\n", irq);
#ifdef RDS_INTERRUPT_ON_ALWAYS
	if ((Si47xx_dev_wait_flag == SEEK_WAITING) ||
	    (Si47xx_dev_wait_flag == TUNE_WAITING)) {
		debug("Si47xx_isr: FM Seek/Tune Interrupt "
			"called IRQ %d\n", irq);
		Si47xx_dev_wait_flag = WAIT_OVER;
		wake_up_interruptible(&Si47xx_waitq);
	} else if (Si47xx_RDS_flag == RDS_WAITING) {	/* RDS Interrupt */
		debug_rds("Si47xx_isr: FM RDS Interrupt "
			"called IRQ %d", irq);

		debug_rds("RDS_Groups_Available_till_now b/w "
			"Power ON/OFF : %d",
			  RDS_Groups_Available_till_now);

		if (!work_pending(&Si47xx_work))
			queue_work(Si47xx_wq, &Si47xx_work);
	}
#else
	if ((Si47xx_dev_wait_flag == SEEK_WAITING) ||
	    (Si47xx_dev_wait_flag == TUNE_WAITING) ||
	    (Si47xx_dev_wait_flag == RDS_WAITING)) {
		Si47xx_dev_wait_flag = WAIT_OVER;
		wake_up_interruptible(&Si47xx_waitq);
	}
#endif
	return IRQ_HANDLED;
}


/*-----------------------------------------------------------------------------
 This command returns the status
-----------------------------------------------------------------------------*/
static u8 si47xx_readStatus(void)
{
	u8 status;
	int ret = 0;
	ret = i2c_master_recv((struct i2c_client *)(Si47xx_dev->client),
		&status, 1);

	if (ret < 0) {
		dev_err(Si47xx_dev->dev, "%s si47xx_readStatus failed %d\n",
		__func__, ret);

		return ret;
	}

	return status;
}


/*-----------------------------------------------------------------------------
 Command that will wait for CTS before returning
-----------------------------------------------------------------------------*/
static void si47xx_waitForCTS(void)
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
static int si47xx_command(u8 cmd_size, u8 *cmd, u8 reply_size, u8 *reply)
{
	int ret = 0;
	ret = i2c_master_send((struct i2c_client *)(Si47xx_dev->client),
		cmd, cmd_size);
	if (ret < 0) {
		dev_err(Si47xx_dev->dev, "%s si47xx_command failed %d\n",
			__func__, ret);

		return ret;
	}
	si47xx_waitForCTS();

	if (reply_size) {
		ret = i2c_master_recv((struct i2c_client *)(Si47xx_dev->client),
		reply, reply_size);

		if (ret < 0)
			dev_err(Si47xx_dev->dev,
			"%s si47xx_command failed %d\n", __func__, ret);

		return ret;
	}

	return ret;
}

/*-----------------------------------------------------------------------------
 Set the passed property number to the passed value.

 Inputs:
      propNumber:  The number identifying the property to set
      propValue:   The value of the property.
-----------------------------------------------------------------------------*/
static void si47xx_set_property(u16 propNumber, u16 propValue)
{
	u8 cmd[8];
	int ret = 0;

	cmd[0] = SET_PROPERTY;
	cmd[1] = 0;
	cmd[2] = (u8)(propNumber >> 8);
	cmd[3] = (u8)(propNumber & 0x00FF);
	cmd[4] = (u8)(propValue >> 8);
	cmd[5] = (u8)(propValue & 0x00FF);

	ret = si47xx_command(6, cmd, 0, NULL);

	if (ret < 0)
		dev_err(Si47xx_dev->dev,
		"%s si47xx_set_property failed %d\n", __func__, ret);
}

static int powerup(void)
{
	int ret = 0;
	u8 cmd[8];
	u8 rsp[13];

	pSi47xxdata->power(1);

	cmd[0] = POWER_UP;
	cmd[1] = POWER_UP_IN_GPO2OEN | POWER_UP_IN_FUNC_FMRX;
	cmd[2] = POWER_UP_IN_OPMODE_RX_ANALOG;
	ret = si47xx_command(3, cmd, 8, rsp);

	if (ret < 0) {
		dev_err(Si47xx_dev->dev, "%s failed %d\n", __func__, ret);
	} else {
		/* Si4709/09 datasheet: Table 7 */
		msleep(110);
		Si47xx_dev->state.power_state = RADIO_ON;
	}
	return ret;
}

static int powerdown(void)
{
	int ret = 0;
	u8 cmd[8];
	u8 rsp[13];

	if (!(RADIO_POWERDOWN == Si47xx_dev->state.power_state)) {
		cmd[0] = POWER_DOWN;
		ret = si47xx_command(1, cmd, 1, rsp);

		if (ret < 0)
			dev_err(Si47xx_dev->dev, "%s failed %d\n",
			__func__, ret);
		else
			Si47xx_dev->state.power_state = RADIO_POWERDOWN;

		msleep(110);
		pSi47xxdata->power(0);
	} else
		debug("Device already Powered-OFF\n");

	return ret;
}

/*-----------------------------------------------------------------------------
 Helper function that sends the GET_INT_STATUS command to the part

 Returns:
   The status byte from the part.
-----------------------------------------------------------------------------*/
static s8 getIntStatus(void)
{
	u8 cmd[8];
	u8 rsp[13];
	int ret = 0;
	cmd[0] = GET_INT_STATUS;
	ret = si47xx_command(1, cmd, 1, rsp);

	if (ret < 0) {
		dev_err(Si47xx_dev->dev, "%s getIntStatus failed %d\n",
				__func__, ret);
		return ret;
	}
	return rsp[0];
}

/*-----------------------------------------------------------------------------
 Helper function that sends the FM_SEEK_START command to the part

Inputs:
seekUp: If non-zero seek will increment otherwise decrement
wrap:   If non-zero seek will wrap around band limits when hitting the end
of the band limit.
-----------------------------------------------------------------------------*/
static int fmSeekStart(u8 seekUp, u8 wrap)
{
	u8 cmd[8];
	u8 rsp[13];
	int ret;

	cmd[0] = FM_SEEK_START;
	cmd[1] = 0;
	if (seekUp)
		cmd[1] |= FM_SEEK_START_IN_SEEKUP;
	if (wrap)
		cmd[1] |= FM_SEEK_START_IN_WRAP;

	ret = si47xx_command(2, cmd, 1, rsp);
	return ret;

}

static u16 freq_to_channel(u32 frequency)
{
	u16 channel;

	if (frequency < Si47xx_dev->settings.bottom_of_band)
		frequency = Si47xx_dev->settings.bottom_of_band;

	channel = (frequency - Si47xx_dev->settings.bottom_of_band)
		/ Si47xx_dev->settings.channel_spacing;

	return channel;
}

/* Only one thread will be able to call this, since this function call is
   protected by a mutex, so no race conditions can arise */
static void wait(void)
{
	wait_event_interruptible(Si47xx_waitq,
			(Si47xx_dev_wait_flag == WAIT_OVER) ||
			(Si47xx_dev_wait_flag == SEEK_CANCEL));
}

#ifndef RDS_INTERRUPT_ON_ALWAYS
static void wait_RDS(void)
{
	wait_event_interruptible_timeout(Si47xx_waitq,
			(Si47xx_dev_wait_flag == WAIT_OVER),
			Si47xx_dev->settings.timeout_RDS);
}
#endif

/*-----------------------------------------------------------------------------
 Helper function that sends the FM_TUNE_FREQ command to the part

 Inputs:
 frequency in 10kHz steps
-----------------------------------------------------------------------------*/
static int fmTuneFreq(u16 frequency)
{
	u8 cmd[8];
	u8 rsp[13];
	int ret;

	cmd[0] = FM_TUNE_FREQ;
	cmd[1] = 0;
	cmd[2] = (u8)(frequency >> 8);
	cmd[3] = (u8)(frequency & 0x00FF);
	cmd[4] = (u8)0;
	ret  = si47xx_command(5, cmd, 1, rsp);

	return ret;
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

static int fmTuneStatus(u8 cancel, u8 intack, struct tune_data_t *tune_data)
{
	u8 cmd[8];
	u8 rsp[13];
	int ret;

	cmd[0] = FM_TUNE_STATUS;
	cmd[1] = 0;
	if (cancel)
		cmd[1] |= FM_TUNE_STATUS_IN_CANCEL;
	if (intack)
		cmd[1] |= FM_TUNE_STATUS_IN_INTACK;

	ret = si47xx_command(2, cmd, 8, rsp);

	tune_data->stc    = !!(rsp[0] & STCINT);
	tune_data->bltf   = !!(rsp[1] & FM_TUNE_STATUS_OUT_BTLF);
	tune_data->afcrl  = !!(rsp[1] & FM_TUNE_STATUS_OUT_AFCRL);
	tune_data->valid  = !!(rsp[1] & FM_TUNE_STATUS_OUT_VALID);
	tune_data->freq   = ((u16)rsp[2] << 8) | (u16)rsp[3];
	tune_data->rssi   = rsp[4];
	tune_data->asnr   = rsp[5];
	tune_data->antcap = rsp[7];

	return ret;
}

/* -----------------------------------------------------------------------------
 Helper function that sends the FM_RSQ_STATUS command to the part

 Inputs:
  intack: If non-zero the interrupt for STCINT will be cleared.

  Outputs:
  Si47xx_status.Status:  Contains bits about the status returned from the part.
  Si47xx_status.RsqInts: Contains bits about the interrupts
  that have fired related to RSQ.
  SMUTE:   The soft mute function is currently enabled
  AFCRL:   The AFC is railed if this is non-zero
  Valid:   The station is valid if this is non-zero
  Pilot:   A pilot tone is currently present
  Blend:   Percentage of blend for stereo. (100 = full stereo)
  RSSI:    The RSSI level read at tune.
  ASNR:    The audio SNR level read at tune.
  FreqOff: The frequency offset in kHz of the current station
  from the tuned frequency.
-----------------------------------------------------------------------------*/
static void fmRsqStatus(u8 intack, struct rsq_data_t *rsq_data)
{
	u8 cmd[8];
	u8 rsp[13];

	cmd[0] = FM_RSQ_STATUS;
	cmd[1] = 0;

	if (intack)
		cmd[1] |= FM_RSQ_STATUS_IN_INTACK;

	si47xx_command(2, cmd, 8, rsp);

	rsq_data->rsqints = rsp[1];
	rsq_data->smute   = !!(rsp[2] & FM_RSQ_STATUS_OUT_SMUTE);
	rsq_data->afcrl   = !!(rsp[2] & FM_RSQ_STATUS_OUT_AFCRL);
	rsq_data->valid   = !!(rsp[2] & FM_RSQ_STATUS_OUT_VALID);
	rsq_data->pilot   = !!(rsp[3] & FM_RSQ_STATUS_OUT_PILOT);
	rsq_data->blend   = rsp[3] & FM_RSQ_STATUS_OUT_STBLEND;
	rsq_data->rssi    = rsp[4];
	rsq_data->snr    = rsp[5];
	rsq_data->freqoff = rsp[7];
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
static void fmRdsStatus(u8 intack, u8 mtfifo, struct radio_data_t *rds_data,
				u8 *RdsFifoUsed)
{
	u8 cmd[8];
	u8 rsp[13];
	int ret = 0;

	cmd[0] = FM_RDS_STATUS;
	cmd[1] = 0;
	if (intack)
		cmd[1] |= FM_RDS_STATUS_IN_INTACK;
	if (mtfifo)
		cmd[1] |= FM_RDS_STATUS_IN_MTFIFO;

	ret = si47xx_command(2, cmd, 13, rsp);
	if (ret < 0) {
		dev_err(Si47xx_dev->dev, "%s fmRdsStatusfailed %d\n",
				__func__, ret);
		return;
	}

	*RdsFifoUsed = rsp[3];
	rds_data->rdsa      = ((u16)rsp[4] << 8) | (u16)rsp[5];
	rds_data->rdsb      = ((u16)rsp[6] << 8) | (u16)rsp[7];
	rds_data->rdsc      = ((u16)rsp[8] << 8) | (u16)rsp[9];
	rds_data->rdsd      = ((u16)rsp[10] << 8) | (u16)rsp[11];
	rds_data->blera = (rsp[12] & FM_RDS_STATUS_OUT_BLEA) >>
		FM_RDS_STATUS_OUT_BLEA_SHFT;
	rds_data->blerb        = (rsp[12] & FM_RDS_STATUS_OUT_BLEB) >>
		FM_RDS_STATUS_OUT_BLEB_SHFT;
	rds_data->blerc        = (rsp[12] & FM_RDS_STATUS_OUT_BLEC) >>
		FM_RDS_STATUS_OUT_BLEC_SHFT;
	rds_data->blerd        = (rsp[12] & FM_RDS_STATUS_OUT_BLED) >>
		FM_RDS_STATUS_OUT_BLED_SHFT;
}

static int seek(u32 *frequency, int up, int mode)
{
	int ret = 0;
	struct tune_data_t tune_data;

	Si47xx_dev_wait_flag = SEEK_WAITING;
	ret = fmSeekStart(up, mode); /* mode 0 is full scan */
	wait();

	if (Si47xx_dev_wait_flag == SEEK_CANCEL) {
		ret = fmTuneStatus(1, 1, &tune_data);

		*frequency = 0;

		Si47xx_dev_wait_flag = NO_WAIT;

		return ret;
	}

	Si47xx_dev_wait_flag = NO_WAIT;

	if (!(getIntStatus() & STCINT)) {
		dev_err(Si47xx_dev->dev, "%s seek is failed!\n", __func__);
		fmTuneStatus(1, 1, &tune_data);
		return -1;
	}

	ret = fmTuneStatus(0, 1, &tune_data);
	if (tune_data.bltf != 1)
		*frequency = tune_data.freq;

	else {
		if (tune_data.valid)
			*frequency = tune_data.freq;
		else
			*frequency = 0;
	}
	return ret;
}

static int tune_freq(u32 frequency)
{
	int ret = 0;

	u16 channel = 0;
	struct tune_data_t tune_data;
	mutex_lock(&(Si47xx_dev->lock));

	channel = freq_to_channel(frequency);

	Si47xx_dev_wait_flag = TUNE_WAITING;
	ret = fmTuneFreq(frequency);
	wait();
	Si47xx_dev_wait_flag = NO_WAIT;
	debug("Si47xx_dev_wait_flag = TUNE_WAITING\n");

	if (!(getIntStatus() & STCINT)) {
		dev_err(Si47xx_dev->dev, "%s tune is failed!\n", __func__);
		fmTuneStatus(1, 1, &tune_data);
		mutex_unlock(&(Si47xx_dev->lock));
		return -1;
	}

	ret = fmTuneStatus(0, 1, &tune_data);

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}


int Si47xx_dev_init(struct Si47xx_device_t *si47xx_dev)
{
	int ret = 0;
	Si47xx_dev = si47xx_dev;
	Si47xx_dev->client = si47xx_dev->client;
	pSi47xxdata = si47xx_dev->pdata;
	si47xx_irq = Si47xx_dev->client->irq;

	debug("Si47xx_dev_init called");

	mutex_lock(&Si47xx_dev->lock);

	Si47xx_dev->state.power_state = RADIO_POWERDOWN;
	Si47xx_dev->state.seek_state = RADIO_SEEK_OFF;
	Si47xx_dev->valid_client_state = eTRUE;
	Si47xx_dev->valid = eFALSE;

#ifdef RDS_INTERRUPT_ON_ALWAYS
	/*Creating Circular Buffer */
	/*Single RDS_Block_Data buffer size is 4x16 bits */
	RDS_Block_Data_buffer = kzalloc(RDS_BUFFER_LENGTH * 8, GFP_KERNEL);
	if (!RDS_Block_Data_buffer) {
		dev_err(Si47xx_dev->dev, "Not sufficient memory for creating "
				"RDS_Block_Data_buffer");
		ret = -ENOMEM;
		goto EXIT;
	}

	/*Single RDS_Block_Error buffer size is 4x8 bits */
	RDS_Block_Error_buffer = kzalloc(RDS_BUFFER_LENGTH * 4, GFP_KERNEL);
	if (!RDS_Block_Error_buffer) {
		dev_err(Si47xx_dev->dev, "Not sufficient memory for creating "
				"RDS_Block_Error_buffer");
		ret = -ENOMEM;
		kfree(RDS_Block_Data_buffer);
		goto EXIT;
	}

	/*Initialising read and write indices */
	RDS_Buffer_Index_read = 0;
	RDS_Buffer_Index_write = 0;

	/*Creating work-queue */
	Si47xx_wq = create_singlethread_workqueue("Si47xx_wq");
	if (!Si47xx_wq) {
		dev_err(Si47xx_dev->dev, "Not sufficient memory for Si47xx_wq, work-queue");
		ret = -ENOMEM;
		kfree(RDS_Block_Error_buffer);
		kfree(RDS_Block_Data_buffer);
		goto EXIT;
	}

	/*Initialising work_queue */
	INIT_WORK(&Si47xx_work, Si47xx_work_func);

	RDS_Data_Available = 0;
	RDS_Data_Lost = 0;
	RDS_Groups_Available_till_now = 0;
EXIT:
#endif

	mutex_unlock(&(Si47xx_dev->lock));

	debug("Si47xx_dev_init call over");

	return ret;
}

int Si47xx_dev_exit(void)
{
	int ret = 0;

	debug("Si47xx_dev_exit called");

	mutex_lock(&(Si47xx_dev->lock));
#ifdef RDS_INTERRUPT_ON_ALWAYS
	if (Si47xx_wq)
		destroy_workqueue(Si47xx_wq);

	kfree(RDS_Block_Error_buffer);
	kfree(RDS_Block_Data_buffer);
#endif

	mutex_unlock(&(Si47xx_dev->lock));

	debug("Si47xx_dev_exit call over");

	return ret;
}

int Si47xx_dev_powerup(void)
{
	int ret = 0;
	u32 value = 100;

	debug("Si47xx_dev_powerup called");

	if (!(RADIO_ON == Si47xx_dev->state.power_state)) {
		ret = powerup();
		if (ret < 0) {
			dev_err(Si47xx_dev->dev, "%s failed %d\n",
				__func__, ret);
		} else if (Si47xx_dev->valid_client_state == eFALSE) {
			dev_err(Si47xx_dev->dev, "Si47xx_dev_powerup called "
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
			/*      &Si47xx_dev->registers[SYSCONFIG2],2); */
/*VNVS:18-NOV'09 : modified for detecting more stations of good quality*/
			si47xx_set_property(FM_SEEK_TUNE_RSSI_THRESHOLD,
				TUNE_RSSI_THRESHOLD);
			si47xx_set_property(FM_SEEK_BAND_BOTTOM, 8750);
			si47xx_set_property(FM_SEEK_BAND_TOP, 10800);
			Si47xx_dev->settings.band = BAND_87500_108000_kHz;
			Si47xx_dev->settings.bottom_of_band = FREQ_87500_kHz;
			si47xx_set_property(FM_SEEK_FREQ_SPACING,
				CHAN_SPACING_100_kHz);
			Si47xx_dev->settings.channel_spacing =
				CHAN_SPACING_100_kHz;

			/* SYSCONFIG3_BITSET_SKSNR( */
			/*      &Si47xx_dev->registers[SYSCONFIG3],3); */
/*VNVS:18-NOV'09 : modified for detecting more stations of good quality*/
			si47xx_set_property(FM_SEEK_TUNE_SNR_THRESHOLD,
			TUNE_SNR_THRESHOLD);
			Si47xx_dev->settings.timeout_RDS =
				msecs_to_jiffies(value);
			Si47xx_dev->settings.curr_snr = TUNE_SNR_THRESHOLD;
			Si47xx_dev->settings.curr_rssi_th = TUNE_RSSI_THRESHOLD;
			Si47xx_dev->valid = eTRUE;

			Si47xx_dev_STEREO_SET();
#ifdef RDS_INTERRUPT_ON_ALWAYS
/*Initialising read and write indices */
			RDS_Buffer_Index_read = 0;
			RDS_Buffer_Index_write = 0;

			RDS_Data_Available = 0;
			RDS_Data_Lost = 0;
			RDS_Groups_Available_till_now = 0;
#endif

		}
	} else
		debug("Device already Powered-ON");

	ret = request_irq(si47xx_irq, Si47xx_isr,
		IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "Si47xx", NULL);
	si47xx_set_property(0xff00, 0);

	/* tune initial frequency to remove tunestatus func err
	 * sometimes occur tunestatus func err when execute tunestatus function
	 * before to complete tune_freq.
	 * so run tune_freq just after to complete booting sequence*/
	ret = tune_freq(Si47xx_dev->settings.bottom_of_band);

	return ret;
}

int Si47xx_dev_powerdown(void)
{
	int ret = 0;

	msleep(500);		/* For avoiding turned off pop noise */
	debug("Si47xx_dev_powerdown called");

	mutex_lock(&(Si47xx_dev->lock));

	free_irq(si47xx_irq, NULL);

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_powerdown called when DS is invalid");
		ret = -1;
	} else {
		ret = powerdown();
		if (ret < 0)
			dev_err(Si47xx_dev->dev, "%s failed %d\n",
			__func__, ret);
	}
	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_suspend(void)
{
	int ret = 0;

	debug("Si47xx_dev_suspend called");

#ifndef _ENABLE_RDS_
	disable_irq(si47xx_irq);
#endif

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid_client_state == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_suspend called "
				"when DS(state, client) is invalid");
		ret = -1;
	}
	mutex_unlock(&(Si47xx_dev->lock));

	debug("Si47xx_dev_enable call over");

	return ret;
}

int Si47xx_dev_resume(void)
{
	int ret = 0;

	debug("Si47xx_dev_resume called");

#ifndef _ENABLE_RDS_
	enable_irq(si47xx_irq);
#endif

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid_client_state == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_resume called "
				"when DS(state, client) is invalid");
		ret = -1;
	}

	mutex_unlock(&(Si47xx_dev->lock));
	debug("Si47xx_dev_disable call over");

	return ret;
}

int Si47xx_dev_band_set(int band)
{
	int ret = 0;
	u16 prev_band = 0;
	u32 prev_bottom_of_band = 0;

	debug("Si47xx_dev_band_set called");

	prev_band = Si47xx_dev->settings.band;
	prev_bottom_of_band = Si47xx_dev->settings.bottom_of_band;
	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_band_set called when DS is invalid");
		ret = -1;
	} else {
		switch (band) {
		case BAND_87500_108000_kHz:
			si47xx_set_property(FM_SEEK_BAND_BOTTOM, 8750);
			si47xx_set_property(FM_SEEK_BAND_TOP, 10800);
			Si47xx_dev->settings.band = BAND_87500_108000_kHz;
			Si47xx_dev->settings.bottom_of_band = FREQ_87500_kHz;
			break;
		case BAND_76000_108000_kHz:
			si47xx_set_property(FM_SEEK_BAND_BOTTOM, 7600);
			si47xx_set_property(FM_SEEK_BAND_TOP, 10800);
			Si47xx_dev->settings.band = BAND_76000_108000_kHz;
			Si47xx_dev->settings.bottom_of_band = FREQ_76000_kHz;
			break;
		case BAND_76000_90000_kHz:
			si47xx_set_property(FM_SEEK_BAND_BOTTOM, 7600);
			si47xx_set_property(FM_SEEK_BAND_TOP, 9000);
			Si47xx_dev->settings.band = BAND_76000_90000_kHz;
			Si47xx_dev->settings.bottom_of_band = FREQ_76000_kHz;
			break;
		default:
			ret = -1;
		}
	}

	return ret;
}

int Si47xx_dev_ch_spacing_set(int ch_spacing)
{
	int ret = 0;
	u16 prev_ch_spacing = 0;

	debug("Si47xx_dev_ch_spacing_set called");

	mutex_lock(&(Si47xx_dev->lock));
	prev_ch_spacing = Si47xx_dev->settings.channel_spacing;
	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_ch_spacing_set called "
				"when DS is invalid");
		ret = -1;
	} else {
		switch (ch_spacing) {
		case CHAN_SPACING_200_kHz:
			si47xx_set_property(FM_SEEK_FREQ_SPACING, 20);
			Si47xx_dev->settings.channel_spacing =
							CHAN_SPACING_200_kHz;
			break;

		case CHAN_SPACING_100_kHz:
			si47xx_set_property(FM_SEEK_FREQ_SPACING, 10);
			Si47xx_dev->settings.channel_spacing =
							CHAN_SPACING_100_kHz;
			break;

		case CHAN_SPACING_50_kHz:
			si47xx_set_property(FM_SEEK_FREQ_SPACING, 5);
			Si47xx_dev->settings.channel_spacing =
							CHAN_SPACING_50_kHz;
			break;

		default:
				ret = -1;
		}

		if (ret == 0) {
			if (ret < 0) {
				dev_err(Si47xx_dev->dev, "Si47xx_dev_ch_spacing_set "
						"i2c_write 1 failed");
				Si47xx_dev->settings.channel_spacing =
					prev_ch_spacing;
			}
		}
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_chan_select(u32 frequency)
{
	int ret = 0;

	debug("Si47xx_dev_chan_select called");

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_chan_select called when DS is invalid");
		ret = -1;
	} else {
		Si47xx_dev->state.seek_state = RADIO_SEEK_ON;

		ret = tune_freq(frequency);
		debug("Si47xx_dev_chan_select called1");
		Si47xx_dev->state.seek_state = RADIO_SEEK_OFF;
	}
	return ret;
}

int Si47xx_dev_chan_get(u32 *frequency)
{
	int ret = 0;
	struct tune_data_t tune_data;

	debug("Si47xx_dev_chan_get called\n");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_chan_get called when DS is invalid");
		ret = -1;
	} else {
		if (ret < 0)
			debug("Si47xx_dev_chan_get i2c_read failed");
		else {
			ret = fmTuneStatus(0, 1, &tune_data);
			*frequency = tune_data.freq;
		}
	}
	mutex_unlock(&(Si47xx_dev->lock));
	return ret;
}

int Si47xx_dev_seek_full(u32 *frequency)
{
	int ret = 0;

	debug("Si47xx_dev_seek_full called\n");
	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_seek_full called when DS is invalid");
		ret = -1;
	} else {
		Si47xx_dev->state.seek_state = RADIO_SEEK_ON;
		ret = seek(frequency, 1, 0);
		Si47xx_dev->state.seek_state = RADIO_SEEK_OFF;
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_seek_up(u32 *frequency)
{
	int ret = 0;

	debug("Si47xx_dev_seek_up called\n");
	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_seek_up called when DS is invalid");
		ret = -1;
	} else {
		Si47xx_dev->state.seek_state = RADIO_SEEK_ON;
		ret = seek(frequency, 1, 1);
		Si47xx_dev->state.seek_state = RADIO_SEEK_OFF;
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_seek_down(u32 *frequency)
{
	int ret = 0;

	debug("Si47xx_dev_seek_down called\n");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_seek_down called when DS is invalid");
		ret = -1;
	} else {
		Si47xx_dev->state.seek_state = RADIO_SEEK_ON;

		ret = seek(frequency, 0, 1);

		Si47xx_dev->state.seek_state = RADIO_SEEK_OFF;
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_RSSI_seek_th_set(u8 seek_th)
{
	int ret = 0;

	debug("Si47xx_dev_RSSI_seek_th_set called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_RSSI_seek_th_set called "
				"when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_SEEK_TUNE_RSSI_THRESHOLD, seek_th);
		Si47xx_dev->settings.curr_rssi_th = seek_th;
		if (ret < 0)
			debug("Si47xx_dev_RSSI_seek_th_set i2c_write 1 failed");
	}
	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_seek_SNR_th_set(u8 seek_SNR)
{
	int ret = 0;

	debug("Si47xx_dev_seek_SNR_th_set called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_seek_SNR_th_set called "
				"when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_SEEK_TUNE_SNR_THRESHOLD, seek_SNR);
		Si47xx_dev->settings.curr_snr = seek_SNR;

		if (ret < 0)
			debug("Si47xx_dev_seek_SNR_th_set i2c_write 1 failed");
	}
	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

/*ToDo Don't use anymore*/
int Si47xx_dev_seek_FM_ID_th_set(u8 seek_FM_ID_th)
{
	int ret = 0;

	debug("Si47xx_dev_seek_FM_ID_th_set called");

	return ret;
}

int Si47xx_dev_cur_RSSI_get(struct rssi_snr_t *cur_RSSI)
{
	int ret = 0;

	struct rsq_data_t rsq_data;
	debug("Si47xx_dev_cur_RSSI_get called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_cur_RSSI_get called when DS is invalid");
		ret = -1;
	} else {
		fmRsqStatus(0, &rsq_data);
		cur_RSSI->curr_rssi = rsq_data.rssi;
		cur_RSSI->curr_rssi_th =
			Si47xx_dev->settings.curr_rssi_th;
		cur_RSSI->curr_snr = rsq_data.snr;

	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

/*ToDo Don't use anymore*/
int Si47xx_dev_device_id(struct device_id *dev_id)
{
	int ret = 0;

	debug("Si47xx_dev_device_id called");

	return ret;
}

/*ToDo Don't use anymore*/
int Si47xx_dev_chip_id(struct chip_id *chp_id)
{
	int ret = 0;

	debug("Si47xx_dev_chip_id called");

	return ret;
}

int Si47xx_dev_sys_config2(struct sys_config2 *sys_conf2)
{
	int ret = 0;

	debug("Si4709_sys_config2 called\n");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si4709_sys_config2 called when DS is invalid");
		ret = -1;
	} else {
		sys_conf2->rssi_th = Si47xx_dev->settings.curr_rssi_th;
		sys_conf2->fm_band = Si47xx_dev->settings.band;
		sys_conf2->fm_chan_spac =
			Si47xx_dev->settings.channel_spacing;
		sys_conf2->fm_vol = 0;
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_sys_config3(struct sys_config3 *sys_conf3)
{
	int ret = 0;

	debug("Si4709_sys_config3 called\n");
	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si4709_sys_config3 called when DS is invalid");
		ret = -1;
	} else {
		sys_conf3->smmute = 0;
		sys_conf3->smutea = 0;
		sys_conf3->volext = 0;
		sys_conf3->sksnr = Si47xx_dev->settings.curr_snr;
		sys_conf3->skcnt = 0;
	}
	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_status_rssi(struct status_rssi *status)
{
	int ret = 0;
	struct rsq_data_t rsq_data;

	debug("Si47xx_dev_status_rssi called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_status_rssi called when DS is invalid");
		mutex_unlock(&(Si47xx_dev->lock));
		return  -1;
	}
	fmRsqStatus(0, &rsq_data);

	pr_debug("%s: Si47xx_dev_status_rssi %d\n",
			__func__, rsq_data.rssi);

	Si47xx_dev->settings.curr_rssi = rsq_data.rssi;
	status->rssi = rsq_data.rssi;

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_sys_config2_set(struct sys_config2 *sys_conf2)
{
	int ret = 0;

	debug("Si47xx_dev_sys_config2_set called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_sys_config2_set called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_SEEK_TUNE_RSSI_THRESHOLD,
				sys_conf2->rssi_th);
		Si47xx_dev_band_set(sys_conf2->fm_band);
		si47xx_set_property(FM_SEEK_FREQ_SPACING,
			sys_conf2->fm_chan_spac);
		Si47xx_dev->settings.curr_rssi_th = sys_conf2->rssi_th;
		Si47xx_dev->settings.band = sys_conf2->fm_band;
		Si47xx_dev->settings.channel_spacing = sys_conf2->fm_chan_spac;
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_sys_config3_set(struct sys_config3 *sys_conf3)
{
	int ret = 0;

	debug("Si47xx_dev_sys_config3_set called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_sys_config3_set called "
				"when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_SEEK_TUNE_SNR_THRESHOLD,
			sys_conf3->sksnr);
		Si47xx_dev->settings.curr_snr = sys_conf3->sksnr;
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

/* VNVS:END */

/* VNVS:START 18-NOV'09 */
/* Reading AFCRL Bit */
int Si47xx_dev_AFCRL_get(u8 *afc)
{
	int ret = 0;
	struct rsq_data_t rsq_data;

	debug("Si47xx_dev_AFCRL_get called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_AFCRL_get called when DS is invalid");
		ret = -1;
	} else {
		fmRsqStatus(0, &rsq_data);
		*afc = rsq_data.afcrl;
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

/* Setting DE
   emphasis time constant 50us(Europe,Japan,Australia) or 75us(USA)
 */
int Si47xx_dev_DE_set(u8 de_tc)
{
	int ret = 0;

	debug("Si47xx_dev_DE_set called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_DE_set called when DS is invalid");
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
				dev_err(Si47xx_dev->dev, "%s failed %d\n",
				__func__, ret);
		}
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

/*Resetting the RDS Data Buffer*/
int Si47xx_dev_reset_rds_data()
{
	int ret = 0;

	debug_rds("Si47xx_dev_reset_rds_data called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_reset_rds_data called when DS is invalid");
		ret = -1;
	} else {
		RDS_Buffer_Index_write = 0;
		RDS_Buffer_Index_read = 0;
		RDS_Data_Lost = 0;
		RDS_Data_Available = 0;
		memset(RDS_Block_Data_buffer, 0, RDS_BUFFER_LENGTH * 8);
		memset(RDS_Block_Error_buffer, 0, RDS_BUFFER_LENGTH * 4);
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_volume_set(u8 volume)
{
	int ret = 0;

	debug("Si47xx_dev_volume_set called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_volume_set called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(RX_VOLUME, pSi47xxdata->rx_vol[volume] &
			RX_VOLUME_MASK);
		Si47xx_dev->vol_idx = volume;

		if (ret < 0)
			dev_err(Si47xx_dev->dev, "%s failed %d\n",
			__func__, ret);
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_volume_get(u8 *volume)
{
	int ret = 0;

	debug("Si4709_dev_volume_get called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si4709_dev_volume_get called when DS is invalid");
		ret = -1;
	} else
		*volume = Si47xx_dev->vol_idx;

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

/*
VNVS:START 19-AUG'10 : Adding DSMUTE ON/OFF feature.
The Soft Mute feature is available to attenuate the audio
outputs and minimize audible noise in very weak signal conditions.
 */
int Si47xx_dev_DSMUTE_ON(void)
{
	int ret = 0;

	debug("Si47xx_dev_DSMUTE_ON called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_DSMUTE_ON called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_SOFT_MUTE_RATE, 64);
		si47xx_set_property(FM_SOFT_MUTE_MAX_ATTENUATION, 0);
		si47xx_set_property(FM_SOFT_MUTE_SNR_THRESHOLD, 4);
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_DSMUTE_OFF(void)
{
	int ret = 0;

	debug("Si47xx_dev_DSMUTE_OFF called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_DSMUTE_OFF called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_SOFT_MUTE_RATE, 64);
		si47xx_set_property(FM_SOFT_MUTE_MAX_ATTENUATION, 16);
		si47xx_set_property(FM_SOFT_MUTE_SNR_THRESHOLD, 4);
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

/*VNVS:END*/

int Si47xx_dev_MUTE_ON(void)
{
	int ret = 0;

	debug("Si47xx_dev_MUTE_ON called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_MUTE_ON called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(RX_HARD_MUTE,
			RX_HARD_MUTE_RMUTE_MASK |
			RX_HARD_MUTE_LMUTE_MASK);
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_MUTE_OFF(void)
{
	int ret = 0;

	debug("Si47xx_dev_MUTE_OFF called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_MUTE_OFF called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(RX_HARD_MUTE, 0);
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_MONO_SET(void)
{
	int ret = 0;

	debug("Si47xx_dev_MONO_SET called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_MONO_SET called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_BLEND_MONO_THRESHOLD, 127);
		si47xx_set_property(FM_BLEND_STEREO_THRESHOLD, 127);
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_STEREO_SET(void)
{
	int ret = 0;

	debug("Si47xx_dev_STEREO_SET called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_STEREO_SET called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_BLEND_MONO_THRESHOLD, 30);
		si47xx_set_property(FM_BLEND_STEREO_THRESHOLD, 49);
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_RDS_ENABLE(void)
{
	int ret = 0;

	debug("Si47xx_dev_RDS_ENABLE called");

	mutex_lock(&(Si47xx_dev->lock));
	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_RDS_ENABLE called when DS is invalid");
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
#ifdef RDS_INTERRUPT_ON_ALWAYS
		Si47xx_RDS_flag = RDS_WAITING;
#endif
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_RDS_DISABLE(void)
{
	int ret = 0;

	debug("Si47xx_dev_RDS_DISABLE called");

	mutex_lock(&(Si47xx_dev->lock));
	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_RDS_DISABLE called when DS is invalid");
		ret = -1;
	} else {
		si47xx_set_property(FM_RDS_CONFIG, 0);
#ifdef RDS_INTERRUPT_ON_ALWAYS
		Si47xx_RDS_flag = NO_WAIT;
#endif
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_rstate_get(struct dev_state_t *dev_state)
{
	int ret = 0;

	debug("Si47xx_dev_rstate_get called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_rstate_get called when DS is invalid");
		ret = -1;
	} else {
		dev_state->power_state = Si47xx_dev->state.power_state;
		dev_state->seek_state = Si47xx_dev->state.seek_state;
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

/* VNVS:START 7-JUNE'10 Function call for work-queue "Si47xx_wq" */
#ifdef RDS_INTERRUPT_ON_ALWAYS
void Si47xx_work_func(struct work_struct *work)
{
	struct radio_data_t rds_data;
	int i = 0;
	u8 RdsFifoUsed;
#ifdef RDS_TESTING
	u8 group_type;
#endif
	debug_rds("%s", __func__);
mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_RDS_data_get called when DS is invalid");
		return;
	}

	if (RDS_Data_Lost > 1)
		debug_rds("No_of_RDS_groups_Lost till now : %d",
				RDS_Data_Lost);
	fmRdsStatus(1, 0, &rds_data, &RdsFifoUsed);
	/* RDSR bit and RDS Block data, so reading the RDS registers */
	do {
		/* Writing into RDS_Block_Data_buffer */
		i = 0;
		RDS_Block_Data_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			rds_data.rdsa;
		RDS_Block_Data_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			rds_data.rdsb;
		RDS_Block_Data_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			rds_data.rdsc;
		RDS_Block_Data_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			rds_data.rdsd;

		/*Writing into RDS_Block_Error_buffer */
		i = 0;

		RDS_Block_Error_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			rds_data.blera;
		RDS_Block_Error_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			rds_data.blerb;
		RDS_Block_Error_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			rds_data.blerc;
		RDS_Block_Error_buffer[i++ + 4 * RDS_Buffer_Index_write] =
			rds_data.blerd;
		fmRdsStatus(1, 0, &rds_data, &RdsFifoUsed);
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
	mutex_unlock(&(Si47xx_dev->lock));
}
#endif
/*VNVS:END*/

int Si47xx_dev_RDS_data_get(struct radio_data_t *data)
{
	int i, ret = 0;
	struct tune_data_t tune_data;
	struct rsq_data_t rsq_data;

	debug_rds("Si47xx_dev_RDS_data_get called");

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		dev_err(Si47xx_dev->dev, "Si47xx_dev_RDS_data_get called when DS is invalid");
		mutex_unlock(&(Si47xx_dev->lock));
		return  -1;
	}
#ifdef RDS_INTERRUPT_ON_ALWAYS
	debug_rds("RDS_Buffer_Index_read = %d", RDS_Buffer_Index_read);

	/*If No New RDS Data is available return error */
	if (RDS_Buffer_Index_read == RDS_Buffer_Index_write) {
		debug_rds("No_New_RDS_Data_is_available");
		ret = fmTuneStatus(0, 1, &tune_data);
		data->curr_channel = tune_data.freq;
		fmRsqStatus(0, &rsq_data);
		data->curr_rssi = rsq_data.rssi;
		debug_rds("curr_channel: %u, curr_rssi:%u",
				data->curr_channel,
				(u32) data->curr_rssi);
		mutex_unlock(&(Si47xx_dev->lock));
		return -1;
	}

	ret = fmTuneStatus(0, 1, &tune_data);
	data->curr_channel = tune_data.freq;
	fmRsqStatus(0, &rsq_data);
	data->curr_rssi = rsq_data.rssi;
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

	debug_rds("RDS_Buffer_Index_read = %d", RDS_Buffer_Index_read);
#endif

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}

int Si47xx_dev_RDS_timeout_set(u32 time_out)
{
	int ret = 0;
	u32 jiffy_count = 0;

	debug("Si47xx_dev_RDS_timeout_set called");
	/****convert time_out(in milliseconds) into jiffies*****/

	jiffy_count = msecs_to_jiffies(time_out);

	debug("jiffy_count%d", jiffy_count);

	mutex_lock(&(Si47xx_dev->lock));

	if (Si47xx_dev->valid == eFALSE) {
		debug("Si47xx_dev_RDS_timeout_set called when DS is invalid");
		ret = -1;
	} else {
		Si47xx_dev->settings.timeout_RDS = jiffy_count;
	}

	mutex_unlock(&(Si47xx_dev->lock));

	return ret;
}
