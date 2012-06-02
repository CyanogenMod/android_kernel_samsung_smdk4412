/*
 * drivers/media/radio/si4705/radio-si4705-i2c.c
 *
 * I2C driver for radios with Silicon Labs Si4705 FM Radio Receivers
 *
 * Copyright (c) 2012 Samsung Electronics Co.Ltd
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


/* kernel includes */
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/si4705_pdata.h>
#include "radio-si4705.h"


/* I2C Device ID List */
static const struct i2c_device_id si4705_i2c_id[] = {
	/* Generic Entry */
	{ "si4705", 0 },
	/* Terminating entry */
	{ }
};
MODULE_DEVICE_TABLE(i2c, si4705_i2c_id);


#define msb(x)                  ((u8)((u16) x >> 8))
#define lsb(x)                  ((u8)((u16) x &  0x00FF))
#define compose_u16(msb, lsb)	(((u16)msb << 8) | lsb)

#define STATE_POWER_UP		1
#define STATE_POWER_DOWN	0
/**************************************************************************
 * Module Parameters
 **************************************************************************/

/* Radio Nr */
static int radio_nr = -1;
module_param(radio_nr, int, 0444);
MODULE_PARM_DESC(radio_nr, "Radio Nr");

/* RDS buffer blocks */
static unsigned int rds_buf = 100;
module_param(rds_buf, uint, 0444);
MODULE_PARM_DESC(rds_buf, "RDS buffer entries: *100*");

/* RDS maximum block errors */
static unsigned short max_rds_errors = 1;
/* 0 means   0  errors requiring correction */
/* 1 means 1-2  errors requiring correction (used by original USBRadio.exe) */
/* 2 means 3-5  errors requiring correction */
/* 3 means   6+ errors or errors in checkword, correction not possible */
module_param(max_rds_errors, ushort, 0644);
MODULE_PARM_DESC(max_rds_errors, "RDS maximum block errors: *1*");

#define CTS_MAX_COUNT	1000

/**************************************************************************
 * General Driver Functions - REGISTERs
 **************************************************************************/

static int si4705_get_register(struct si4705_device *radio,
				u8 cmd_size, u8 *cmd)
{
	struct i2c_client *i2c = radio->client;
	int ret = 0;

	ret = i2c_master_recv(i2c, (char *)cmd, cmd_size);
	if (ret != cmd_size)
		return -EIO;

	return 0;
}

static int si4705_set_register(struct si4705_device *radio,
				u8 cmd_size, u8 *cmd)
{
	struct i2c_client *i2c = radio->client;
	int ret = 0;

	ret = i2c_master_send(i2c, (const char *)cmd, cmd_size);
	if (ret != cmd_size)
		return -EIO;

	return 0;
}

static void si4705_wait_for_cts(struct si4705_device *radio)
{
	u16 count = CTS_MAX_COUNT;
	u8 status = 0;
	do {
		si4705_get_register(radio, 1, &status);
		usleep_range(5, 10);
	} while (--count && !(status & CTS));

	radio->int_status = status;

	if (status & ERR)
		pr_err("%s: status = 0x%02x", __func__, status);
}

static int si4705_send_command(struct si4705_device *radio, u8 cmd_size,
				u8 *cmd, u8 reply_size, u8 *reply)
{
	int ret = 0;
	ret = si4705_set_register(radio, cmd_size, cmd);
	if (ret != 0) {
		pr_err("%s: si4705 set register error %d", __func__, ret);
		return ret;
	}

	si4705_wait_for_cts(radio);

	if (reply_size == 1)
		reply[0] = radio->int_status;
	else if (reply_size > 1)
		ret = si4705_get_register(radio, reply_size, reply);

	if (ret != 0) {
		pr_err("%s: si4705 get register error %d", __func__, ret);
		return ret;
	}

	return 0;

}

/*
 * si4705_power_up : power the radio up
 */
int si4705_power_up(struct si4705_device *radio)
{
	u8 cmd[POWER_UP_NARGS] = {
		POWER_UP,
		POWER_UP_IN_GPO2OEN,
		POWER_UP_IN_OPMODE_RX_ANALOG
	};
	u8 reply[POWER_UP_NRESP];
	int retval = 0;

	/* enable radio device gpio reset pin */
	if (radio->pdata->reset)
		radio->pdata->reset(true);

	if (STATE_POWER_DOWN == radio->power_state) {
		/* the power up command is accepted only in power down mode */
		if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
			    ARRAY_SIZE(reply), &reply[0]) < 0) {
			pr_err("%s: send power up command failed", __func__);
			retval = -EIO;
		} else
			radio->power_state = STATE_POWER_UP;
	}

	msleep(110);

	return retval;
}

/*
 * si4705_power_down : power the radio down
 */
int si4705_power_down(struct si4705_device *radio)
{
	u8 cmd[POWER_DOWN_NARGS] = {POWER_DOWN};
	u8 reply[POWER_DOWN_NRESP];
	int retval = 0;

	if (STATE_POWER_UP == radio->power_state) {
		/* the power down command is accepted only in power up mode */
		if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
			    ARRAY_SIZE(reply), &reply[0]) < 0) {
			retval = -EIO;
		} else
			radio->power_state = STATE_POWER_DOWN;
	}

	/* disable radio device gpio reset pin */
	if (radio->pdata->reset)
		radio->pdata->reset(false);

	return retval;
}

/*
 * si4705_get_revision : get the revision information
 */
static int si4705_get_revision(struct si4705_device *radio)
{
	u8 cmd[GET_REV_NARGS] = {GET_REV};
	u8 reply[GET_REV_NRESP];
	int retval = 0;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
		    ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;

	dev_info(&radio->client->dev, "Part Number:Si47%02d\n", reply[1]);
	dev_info(&radio->client->dev, "FirmWare Major:%c, Minor:%c\n",
		reply[2], reply[3]);
	dev_info(&radio->client->dev, "Patch ID :0x%x%x\n", reply[4], reply[5]);
	dev_info(&radio->client->dev, "Component Major:%c, Minor:%c\n",
		reply[6], reply[7]);
	dev_info(&radio->client->dev, "Chip Revision:rev%c\n", reply[8]);

	return retval;
}

/*
 * si4705_set_property : set the property
 */
int si4705_set_property(struct si4705_device *radio, u16 prop, u16 val)
{
	u8 cmd[SET_PROPERTY_NARGS] = {
		SET_PROPERTY,
		0x00,
		msb(prop),
		lsb(prop),
		msb(val),
		lsb(val)
	};
	u8 reply[SET_PROPERTY_NRESP];
	int retval = 0;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
		    ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;

	return retval;
}

/*
 * si4705_get_property : get the property
 */
int si4705_get_property(struct si4705_device *radio, u16 prop, u16 *pVal)
{
	u8 cmd[GET_PROPERTY_NARGS] = {
		GET_PROPERTY,
		0x00,
		msb(prop),
		lsb(prop)
	};
	u8 reply[GET_PROPERTY_NRESP];
	int retval = 0;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
		    ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;

	*pVal = compose_u16(reply[2], reply[3]);

	return retval;
}

/*
 * si4705_get_int_status : Updates bits 6:0 of the status byte
 */
int si4705_get_int_status(struct si4705_device *radio, u8 *cts, u8 *err,
		u8 *rsqint, u8 *rdsint, u8 *stcint)
{
	u8 cmd[GET_INT_STATUS_NARGS] = {GET_INT_STATUS};
	u8 reply[GET_INT_STATUS_NRESP];
	int retval = 0;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
		    ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;
	else {
		pr_info("%s: int_status = 0x%02x", __func__, reply[0]);
		if (cts != NULL)
			*cts = (reply[0] & 0x80) >> 7;
		if (err != NULL)
			*err = (reply[0] & 0x40) >> 6;
		if (rsqint != NULL)
			*rsqint = (reply[0] & 0x08) >> 3;
		if (rdsint != NULL)
			*rdsint = (reply[0] & 0x04) >> 2;
		if (stcint != NULL)
			*stcint = (reply[0] & 0x01);
	}

	return retval;
}

/*
 * si4705_fm_tune_freq : tune frequency
 */
int si4705_fm_tune_freq(struct si4705_device *radio, u16 freq, u8 antcap)
{
	u8 cmd[FM_TUNE_FREQ_NARGS] = {
		FM_TUNE_FREQ,
		0x00,
		msb(freq),
		lsb(freq),
		antcap
	};
	u8 reply[FM_TUNE_FREQ_NRESP];
	int retval = 0;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
		    ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;

	return retval;
}

/*
 * si4705_fm_seek_start : begins searching for a valid frequency
 */
int si4705_fm_seek_start(struct si4705_device *radio, u8 seekUp, u8 wrap)
{
	u8 cmd[FM_SEEK_START_NARGS] = {
		FM_SEEK_START,
		0x00
	};
	u8 reply[FM_SEEK_START_NRESP];
	int retval = 0;

	if (seekUp)
		cmd[1] = FM_SEEK_START_IN_SEEKUP;
	if (wrap)
		cmd[1] |= FM_SEEK_START_IN_WRAP;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
		    ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;

	return retval;
}

/*
 * si4705_fm_tune_status : returns the status of tune or seek commands
 */
int si4705_fm_tune_status(struct si4705_device *radio, u8 cancel,
		u8 intack, u8 *bltf, u16 *freq, u8 *rssi, u8 *snr, u8 *antcap)
{
	u8 cmd[FM_TUNE_STATUS_NARGS] = {
		FM_TUNE_STATUS,
		0x00
	};
	u8 reply[FM_TUNE_STATUS_NRESP];
	int retval = 0;

	if (cancel)
		cmd[1] = FM_TUNE_STATUS_IN_CANCEL;
	if (intack)
		cmd[1] |= FM_TUNE_STATUS_IN_INTACK;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
		    ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;
	else {
		pr_info("%s: tune_status = 0x%02x", __func__, reply[0]);
		if (bltf != NULL)
			*bltf = (reply[1] & 0x80) >> 7;
		if (freq != NULL)
			*freq = compose_u16(reply[2], reply[3]);
		if (rssi != NULL)
			*rssi = reply[4];
		if (snr != NULL)
			*snr = reply[5];
		if (antcap != NULL)
			*antcap = reply[7];
	}

	return retval;
}

/*
 * si4705_fm_rsq_status : returns status information
 * about the received signal quality
 */
int si4705_fm_rsq_status(struct si4705_device *radio, u8 intack,
		u8 *intStatus, u8 *indStatus, u8 *stBlend, u8 *rssi,
		u8 *snr, u8 *freqOff)
{
	u8 cmd[FM_RSQ_STATUS_NARGS] = {
		FM_RSQ_STATUS,
		0x00
	};
	u8 reply[FM_RSQ_STATUS_NRESP];
	int retval = 0;

	if (intack)
		cmd[1] = FM_RSQ_STATUS_IN_INTACK;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
		    ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;
	else {
		if (intStatus != NULL)
			*intStatus = reply[1];
		if (indStatus != NULL)
			*indStatus = reply[2];
		if (stBlend != NULL)
			*stBlend = reply[3];
		if (rssi != NULL)
			*rssi = reply[4];
		if (snr != NULL)
			*snr = reply[5];
		if (freqOff != NULL)
			*freqOff = reply[7];
	}

	return retval;
}

/*
 * si4705_fm_rds_status : returns RDS information for current channel
 * and reads an entry from the RDS FIFO
 */
int si4705_fm_rds_status(struct si4705_device *radio, u8 mtfifo,
		u8 intack, u8 *rdsInd, u8 *sync, u8 *fifoUsed, u16 *rdsFifo,
		u8 *ble)
{
	u8 cmd[FM_RDS_STATUS_NARGS] = {
		FM_RDS_STATUS,
		0x00
	};
	u8 reply[FM_RDS_STATUS_NRESP];
	int retval = 0;
	int i;

	if (mtfifo)
		cmd[1] = FM_RDS_STATUS_IN_MTFIFO;
	if (intack)
		cmd[1] |= FM_RDS_STATUS_IN_INTACK;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
		    ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;
	else {
		if (rdsInd != NULL)
			*rdsInd = reply[1];
		if (sync != NULL)
			*sync = reply[2];
		if (fifoUsed != NULL)
			*fifoUsed = reply[3];
		if (rdsFifo != NULL) {
			for (i = 0; i < 4; i++)
				rdsFifo[i] = compose_u16(reply[(i*2)+4],
							reply[(i*2)+5]);
		}
		if (ble != NULL)
			*ble = reply[12];
	}

	return retval;
}

/*
 * si4705_fm_agc_status : Returns the AGC setting of the device
 */
int si4705_fm_agc_status(struct si4705_device *radio, u8 *rfAgcDis,
		u8 *lnaGainIndex)
{
	u8 cmd[FM_AGC_STATUS_NARGS] = {
		FM_AGC_STATUS,
	};
	u8 reply[FM_AGC_STATUS_NRESP];
	int retval = 0;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
			ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;
	else {
		if (rfAgcDis != NULL)
			*rfAgcDis = (reply[1] & FM_AGC_STATUS_RFAGCDIS);
		if (lnaGainIndex != NULL)
			*lnaGainIndex = (reply[2] & FM_AGC_STATUS_LNAGAINIDX);
	}

	return retval;

}

/*
 * si4705_fm_agc_override : Overrides AGC setting by disabling the AGC
 * and forcing the LNA to have a certain gain that ranges between 0
 * (minimum attenuation) and 26 (maximum attenuation).
 */
int si4705_fm_agc_override(struct si4705_device *radio, u8 *rfAgcDis,
		u8 *lnaGainIndex)
{
	u8 cmd[FM_AGC_OVERRIDE_NARGS] = {
		FM_AGC_OVERRIDE,
	};
	u8 reply[FM_AGC_OVERRIDE_NRESP];
	int retval = 0;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
			ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;
	else {
		if (rfAgcDis != NULL)
			*rfAgcDis = (reply[1] & FM_AGC_OVERRIDE_RFAGCDIS);
		if (lnaGainIndex != NULL)
			*lnaGainIndex = (reply[2] & FM_AGC_OVERRIDE_LNAGAINIDX);
	}

	return retval;

}

/*
 * si4705_gpio_ctl : Enables output for GPO1, 2, and 3. GPO1, 2, and 3
 * can be configured for output (Hi-Z or active drive) by setting
 * the GPO1OEN, GPO2OEN, and GPO3OEN bit.
 */
int si4705_gpio_ctl(struct si4705_device *radio, u8 gpio1,
		u8 gpio2, u8 gpio3)
{
	u8 cmd[GPIO_CTL_NARGS] = {
		GPIO_CTL,
	};
	u8 reply[GPIO_CTL_NRESP];
	int retval = 0;

	if (gpio1)
		cmd[1] = GPIO_CTL_GPO1OEN;
	if (gpio2)
		cmd[1] |= GPIO_CTL_GPO2OEN;
	if (gpio3)
		cmd[1] |= GPIO_CTL_GPO3OEN;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
			ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;

	return retval;

}

/*
 * si4705_gpio_set : Sets the output level (high or low) for GPO1, 2, and 3.
 */
int si4705_gpio_set(struct si4705_device *radio, u8 gpio1,
		u8 gpio2, u8 gpio3)
{
	u8 cmd[GPIO_SET_NARGS] = {
		GPIO_SET,
	};
	u8 reply[GPIO_SET_NRESP];
	int retval = 0;

	if (gpio1)
		cmd[1] = GPIO_SET_GPO1LEVEL;
	if (gpio2)
		cmd[1] |= GPIO_SET_GPO2LEVEL;
	if (gpio3)
		cmd[1] |= GPIO_SET_GPO3LEVEL;

	if (si4705_send_command(radio, ARRAY_SIZE(cmd), &cmd[0],
			ARRAY_SIZE(reply), &reply[0]) < 0)
		retval = -EIO;

	return retval;

}

/*
 * si4705_vol_conv_index_to_value - convert from volume index to real value
 */
u16 si4705_vol_conv_index_to_value(struct si4705_device *radio, s32 index)
{
	if (index < 0) {
		pr_warning("%s: out of range %d", __func__, index);
		return 0;
	}

	if (!((radio->pdata->pdata_values & SI4705_PDATA_BIT_VOL_STEPS)
		&& (radio->pdata->pdata_values & SI4705_PDATA_BIT_VOL_TABLE))) {
		pr_err("%s: it seems si4705_pdata isn't filled", __func__);
		return RX_VOLUME_MAX;
	}

	if (index > radio->pdata->rx_vol_steps-1) {
		pr_warning("%s: out of range %d", __func__, index);
		return RX_VOLUME_MAX;
	}

	return radio->pdata->rx_vol_table[index];
}

/*
 * si4705_vol_conv_valueto_index
 *		- convert from real volume value to volume index
 */
s32 si4705_vol_conv_value_to_index(struct si4705_device *radio, u16 value)
{
	s32 i;

	if (!((radio->pdata->pdata_values & SI4705_PDATA_BIT_VOL_STEPS)
		&& (radio->pdata->pdata_values & SI4705_PDATA_BIT_VOL_TABLE))) {
		pr_err("%s: it seems si4705_pdata isn't filled", __func__);
		return RX_VOLUME_MAX;
	}

	for (i = 0; i < radio->pdata->rx_vol_steps; i++) {
		if (value == radio->pdata->rx_vol_table[i])
			return i;
	}
	pr_warning("%s: can not find matching volume %u", __func__, value);
	return 0;
}

/*
 * si4705_get_seek_tune_rssi_threshold_value
 *		- read FM_SEEK_TUNE_RSSI_THRESHOLD value from pdata
 */
u16 si4705_get_seek_tune_rssi_threshold_value(struct si4705_device *radio)
{
	if (!(radio->pdata->pdata_values & SI4705_PDATA_BIT_RSSI_THRESHOLD)) {
		pr_err("%s: it seems si4705_pdata isn't filled", __func__);
		return FM_SEEK_TUNE_RSSI_THRESHOLD_DEFAULT;
	} else
		return radio->pdata->rx_seek_tune_rssi_threshold;
}

/*
 * si4705_get_seek_tune_snr_threshold_value
 *		- read FM_SEEK_TUNE_SNR_THRESHOLD value from pdata
 */
u16 si4705_get_seek_tune_snr_threshold_value(struct si4705_device *radio)
{
	if (!(radio->pdata->pdata_values & SI4705_PDATA_BIT_SNR_THRESHOLD)) {
		pr_err("%s: it seems si4705_pdata isn't filled", __func__);
		return FM_SEEK_TUNE_SNR_THRESHOLD_DEFAULT;
	} else
		return radio->pdata->rx_seek_tune_snr_threshold;
}

/**************************************************************************
 * General Driver Functions - DISCONNECT_CHECK
 **************************************************************************/

/*
 * si4705_disconnect_check - check whether radio disconnects
 */
int si4705_disconnect_check(struct si4705_device *radio)
{
	return 0;
}


/**************************************************************************
 * I2C Interface
 **************************************************************************/

/*
 * si4705_resp_workqueue - interrupt workqueue
 */
void si4705_resp_work(struct work_struct *work)
{
	struct si4705_device *radio =
		container_of(work, struct si4705_device, resp_work);
	unsigned char blocknum;
	unsigned short bler; /* rds block errors */
	unsigned short rds;
	unsigned char tmpbuf[3];
	int retval = 0;
	u16 rdsConfig = 0;
	u8 resInd = 0;
	u16 rdsFifo[4] = {0,};
	u8 ble = 0;

	/* safety checks */
	retval = si4705_get_property(radio, RDS_CONFIG, &rdsConfig);
	if (retval < 0)
		goto end;

	if ((rdsConfig & RDS_CONFIG_RDSEN_MASK) == 0)
		goto end;

	retval = si4705_fm_rds_status(radio, 0x00, 0x01, &resInd, NULL, NULL,
		rdsFifo, &ble);
	if (retval < 0)
		goto end;

	/* get rds blocks */
	if ((resInd & FM_RDS_STATUS_OUT_RECV) == 0)
		/* No RDS group ready, better luck next time */
		goto end;

	for (blocknum = 0; blocknum < 4; blocknum++) {
		switch (blocknum) {
		default:
			bler = (ble & FM_RDS_STATUS_OUT_BLEA)
				>> FM_RDS_STATUS_OUT_BLEA_SHFT;
			rds = rdsFifo[0];
			break;
		case 1:
			bler = (ble & FM_RDS_STATUS_OUT_BLEB)
				>> FM_RDS_STATUS_OUT_BLEB_SHFT;
			rds = rdsFifo[1];
			break;
		case 2:
			bler = (ble & FM_RDS_STATUS_OUT_BLEC)
				>> FM_RDS_STATUS_OUT_BLEC_SHFT;
			rds = rdsFifo[2];
			break;
		case 3:
			bler = (ble & FM_RDS_STATUS_OUT_BLED)
				>> FM_RDS_STATUS_OUT_BLED_SHFT;
			rds = rdsFifo[3];
			break;
		};

		/* Fill the V4L2 RDS buffer */
		put_unaligned_le16(rds, &tmpbuf);
		tmpbuf[2] = blocknum;		/* offset name */
		tmpbuf[2] |= blocknum << 3;	/* received offset */
		if (bler > max_rds_errors)
			tmpbuf[2] |= 0x80;	/* uncorrectable errors */
		else if (bler > 0)
			tmpbuf[2] |= 0x40;	/* corrected error(s) */

		/* copy RDS block to internal buffer */
		memcpy(&radio->buffer[radio->wr_index], &tmpbuf, 3);
		radio->wr_index += 3;

		/* wrap write pointer */
		if (radio->wr_index >= radio->buf_size)
			radio->wr_index = 0;

		/* check for overflow */
		if (radio->wr_index == radio->rd_index) {
			/* increment and wrap read pointer */
			radio->rd_index += 3;
			if (radio->rd_index >= radio->buf_size)
				radio->rd_index = 0;
		}
	}

	if (radio->wr_index != radio->rd_index)
		wake_up_interruptible(&radio->read_queue);
end:
	return;
}

/*
 * si4705_i2c_interrupt - interrupt handler
 */
static irqreturn_t si4705_i2c_interrupt(int irq, void *dev_id)
{
	struct si4705_device *radio = dev_id;
	int retval = 0;
	u8 stcint = 0;

	/* check Seek/Tune Complete */
	retval = si4705_get_int_status(radio, NULL, NULL, NULL, NULL, &stcint);
	if (retval < 0)
		goto end;

	if (stcint)
		complete(&radio->completion);

	queue_work(radio->queue, &radio->resp_work);

end:
	return IRQ_HANDLED;
}

/*
 * si4705_i2c_probe - probe for the device
 */
static int __devinit si4705_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct si4705_device *radio;
	int retval = 0;

	/* private data allocation and initialization */
	radio = kzalloc(sizeof(struct si4705_device), GFP_KERNEL);
	if (!radio) {
		retval = -ENOMEM;
		goto err_initial;
	}
	radio->pdata = client->dev.platform_data;

	radio->users = 0;
	radio->client = client;
	mutex_init(&radio->lock);

	/* video device allocation and initialization */
	radio->videodev = video_device_alloc();
	if (!radio->videodev) {
		retval = -ENOMEM;
		goto err_radio;
	}
	memcpy(radio->videodev, &si4705_viddev_template,
			sizeof(si4705_viddev_template));
	video_set_drvdata(radio->videodev, radio);

	/* set init. operational mode */
	radio->op_mode = RADIO_OP_DEFAULT;

	/* set init. power state */
	/*
	TODO: Need to make sure radio chip is really
	      in power down state to support hibernation feature
	*/
	radio->power_state = STATE_POWER_DOWN;

	/* power up */
	if (si4705_power_up(radio) != 0) {
		retval = -EIO;
		goto err_radio;
	}

	/* read chip revision */
	if (si4705_get_revision(radio) != 0) {
		retval = -EIO;
		goto err_radio;
	}

	/* rds buffer allocation */
	radio->buf_size = rds_buf * 3;
	radio->buffer = kmalloc(radio->buf_size, GFP_KERNEL);
	if (!radio->buffer) {
		retval = -EIO;
		goto err_video;
	}

	/* rds buffer configuration */
	radio->wr_index = 0;
	radio->rd_index = 0;
	init_waitqueue_head(&radio->read_queue);

	/* mark Seek/Tune Complete Interrupt enabled */
	radio->stci_enabled = true;
	init_completion(&radio->completion);

	INIT_WORK(&radio->resp_work, si4705_resp_work);
	radio->queue = create_freezable_workqueue("si4705_wq");
	if (radio->queue == NULL) {
		retval = -ENOMEM;
		pr_err("%s: Failed to create workqueue", __func__);
		goto err_rds;
	}

	retval = request_threaded_irq(client->irq, NULL, si4705_i2c_interrupt,
				IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				DRIVER_NAME, radio);
	if (retval) {
		dev_err(&client->dev, "Failed to register interrupt\n");
		goto err_rds;
	}

	/* register video device */
	retval = video_register_device(radio->videodev, VFL_TYPE_RADIO,
			radio_nr);
	if (retval) {
		dev_warn(&client->dev, "Could not register video device\n");
		goto err_all;
	}
	i2c_set_clientdata(client, radio);

	/* power down */
	if (si4705_power_down(radio) != 0) {
		retval = -EIO;
		goto err_radio;
	}

	return 0;
err_all:
	free_irq(client->irq, radio);
err_rds:
	kfree(radio->buffer);
err_video:
	video_device_release(radio->videodev);
err_radio:
	kfree(radio);
err_initial:
	return retval;
}

/*
 * si4705_i2c_remove - remove the device
 */
static __devexit int si4705_i2c_remove(struct i2c_client *client)
{
	struct si4705_device *radio = i2c_get_clientdata(client);

	free_irq(client->irq, radio);
	destroy_workqueue(radio->queue);
	video_unregister_device(radio->videodev);
	kfree(radio);

	return 0;
}

#ifdef CONFIG_PM
/*
 * si4705_i2c_suspend - suspend the device
 */
static int si4705_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct si4705_device *radio = i2c_get_clientdata(client);

	if (RADIO_OP_RICH_MODE == radio->op_mode) {
		radio->power_state_at_suspend = radio->power_state;
		/* power down */
		if (si4705_power_down(radio) < 0)
			return -EIO;
	}

	return 0;
}

/*
 * si4705_i2c_resume - resume the device
 */
static int si4705_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct si4705_device *radio = i2c_get_clientdata(client);

	if (RADIO_OP_RICH_MODE == radio->op_mode) {
		/* power up if it was powered up at suspend: need 110ms */
		if (radio->power_state_at_suspend == STATE_POWER_UP &&
		    si4705_power_up(radio) < 0)
			return -EIO;
	}

	return 0;
}

static SIMPLE_DEV_PM_OPS(si4705_i2c_pm, si4705_i2c_suspend, si4705_i2c_resume);
#endif

/*
 * si4705_i2c_driver - i2c driver interface
 */
static struct i2c_driver si4705_i2c_driver = {
	.driver = {
		.name		= "si470x",
		.owner		= THIS_MODULE,
#ifdef CONFIG_PM
		.pm		= &si4705_i2c_pm,
#endif
	},
	.probe			= si4705_i2c_probe,
	.remove			= __devexit_p(si4705_i2c_remove),
	.id_table		= si4705_i2c_id,
};


/**************************************************************************
 * Module Interface
 **************************************************************************/

/*
 * si4705_i2c_init - module init
 */
static int __init si4705_i2c_init(void)
{
	printk(KERN_INFO DRIVER_DESC ", Version " DRIVER_VERSION "\n");
	return i2c_add_driver(&si4705_i2c_driver);
}

/*
 * si4705_i2c_exit - module exit
 */
static void __exit si4705_i2c_exit(void)
{
	i2c_del_driver(&si4705_i2c_driver);
}

module_init(si4705_i2c_init);
module_exit(si4705_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
