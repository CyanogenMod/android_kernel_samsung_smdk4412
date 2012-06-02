/*
 *  drivers/media/radio/si4705/radio-si4705-common.c
 *
 *  Driver for radios with Silicon Labs si4705 FM Radio Receivers
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
#include <linux/si4705_pdata.h>
#include "radio-si4705.h"


/**************************************************************************
 * Module Parameters
 **************************************************************************/

/* Spacing (kHz) */
/* 0: 200 kHz (USA, Australia) */
/* 1: 100 kHz (Europe, Japan) */
/* 2:  50 kHz */
static unsigned short space = 2;
module_param(space, ushort, 0444);
MODULE_PARM_DESC(space, "Spacing: 0=200kHz 1=100kHz *2=50kHz*");

/* Bottom of Band (MHz) */
/* 0: 87.5 - 108 MHz (USA, Europe)*/
/* 1: 76   - 108 MHz (Japan wide band) */
/* 2: 76   -  90 MHz (Japan) */
static unsigned short band = 1;
module_param(band, ushort, 0444);
MODULE_PARM_DESC(band, "Band: 0=87.5..108MHz *1=76..108MHz* 2=76..90MHz");

/* De-emphasis */
/* 0: V4L2_DEEMPHASIS_DISABLED */
/* 1: V4L2_DEEMPHASIS_50_uS (Europe, Australia, Japan) */
/* 2: V4L2_DEEMPHASIS_75_uS (South Korea, USA) */
static unsigned short de = 1;
module_param(de, ushort, 0444);
MODULE_PARM_DESC(de, "De-emphasis: 0=Disabled *1=50us* 2=75us");

/* Tune timeout */
static unsigned int tune_timeout = 3000;
module_param(tune_timeout, uint, 0644);
MODULE_PARM_DESC(tune_timeout, "Tune timeout: *3000*");

/* Seek timeout */
static unsigned int seek_timeout = 5000;
module_param(seek_timeout, uint, 0644);
MODULE_PARM_DESC(seek_timeout, "Seek timeout: *5000*");

/**************************************************************************
 * Defines
 **************************************************************************/

/* si4705 uses 10kHz unit */
#define FREQ_DEV_TO_V4L2(x)	(x * FREQ_MUL / 100)
#define FREQ_V4L2_TO_DEV(x)	(x * 100 / FREQ_MUL)

/**************************************************************************
 * Generic Functions
 **************************************************************************/
/*
 * si4705_set_deemphasis
 */
static int si4705_set_deemphasis(struct si4705_device *radio,
					unsigned int deemphasis)
{
	int retval;
	unsigned short chip_de;

	switch (deemphasis) {
	case V4L2_DEEMPHASIS_DISABLED:
		return 0;
	case V4L2_DEEMPHASIS_50_uS: /* Europe, Australia, Japan */
		chip_de = 1;
		break;
	case V4L2_DEEMPHASIS_75_uS: /* South Korea, USA */
		chip_de = 0;
		break;
	default:
		retval = -EINVAL;
		goto done;
	}

	retval = si4705_set_property(radio, FM_DEEMPHASIS, chip_de);

done:
	if (retval < 0)
		pr_err("%s: set de-emphasis failed with %d", __func__, retval);

	return retval;
}

/*
 * si4705_set_chan - set the channel
 */
static int si4705_set_chan(struct si4705_device *radio, unsigned short chan)
{
	int retval;
	unsigned long timeout;
	bool timed_out = 0;
	u8 stcint = 0;

	pr_info("%s: chan = %d", __func__, chan);

	/* start tuning */
	retval = si4705_fm_tune_freq(radio, chan, 0x00);
	if (retval < 0)
		goto done;

	/* currently I2C driver only uses interrupt way to tune */
	if (radio->stci_enabled) {
		INIT_COMPLETION(radio->completion);

		/* wait till tune operation has completed */
		retval = wait_for_completion_timeout(&radio->completion,
				msecs_to_jiffies(tune_timeout));
		if (!retval)
			timed_out = true;

		stcint = (radio->int_status & GET_INT_STATUS_STCINT);
		pr_err("%s: stcint = 0x%02x", __func__, stcint);
	} else {
		/* wait till tune operation has completed */
		timeout = jiffies + msecs_to_jiffies(tune_timeout);
		do {
			retval = si4705_get_int_status(radio, NULL, NULL, NULL,
				NULL, &stcint);
			if (retval < 0)
				goto stop;
			timed_out = time_after(jiffies, timeout);
		} while ((stcint == 0) && (!timed_out));
	}

	if (stcint == 0)
		pr_err("%s: tune does not complete. stcint = %d",
						__func__, stcint);
	if (timed_out)
		pr_err("%s: tune timed out after %u ms",
					__func__, tune_timeout);

stop:
	/* stop tuning */
	retval = si4705_fm_tune_status(radio, 0x01, 0x01,
		NULL, NULL, NULL, NULL, NULL);

done:
	return retval;
}

/*
 * si4705_get_freq - get the frequency
 */
static int si4705_get_freq(struct si4705_device *radio, unsigned int *freq)
{
	int retval;
	u16 chan = 0;

	retval = si4705_fm_tune_status(radio, 0x00, 0x00,
		NULL, &chan, NULL, NULL, NULL);

	*freq = FREQ_DEV_TO_V4L2(chan);

	pr_info("%s: frequency = %d", __func__, chan);

	return retval;
}

/*
 * si4705_set_freq - set the frequency
 */
int si4705_set_freq(struct si4705_device *radio, unsigned int freq)
{
	u16 chan = 0;

	chan = FREQ_V4L2_TO_DEV(freq);

	return si4705_set_chan(radio, chan);
}

/*
 * si4705_set_seek - set seek
 */
static int si4705_set_seek(struct si4705_device *radio,
		unsigned int wrap_around, unsigned int seek_upward)
{
	int retval = 0;
	unsigned long timeout;
	bool timed_out = 0;
	u8 stcint = 0;
	u8 bltf = 0;

	/* start seeking */
	retval = si4705_fm_seek_start(radio, seek_upward, wrap_around);
	if (retval < 0)
		goto done;

	/* currently I2C driver only uses interrupt way to seek */
	if (radio->stci_enabled) {
		INIT_COMPLETION(radio->completion);

		/* wait till seek operation has completed */
		retval = wait_for_completion_timeout(&radio->completion,
				msecs_to_jiffies(seek_timeout));
		if (!retval)
			timed_out = true;

		stcint = (radio->int_status & GET_INT_STATUS_STCINT);
	} else {
		/* wait till seek operation has completed */
		timeout = jiffies + msecs_to_jiffies(seek_timeout);
		do {
			retval = si4705_get_int_status(radio, NULL, NULL, NULL,
				NULL, &stcint);
			if (retval < 0)
				goto stop;
			timed_out = time_after(jiffies, timeout);
		} while ((stcint == 0) && (!timed_out));
	}

	if (stcint == 0)
		pr_err("%s: seek does not complete", __func__);
	if (bltf)
		pr_err("%s: seek failed / band limit reached", __func__);
	if (timed_out)
		pr_err("%s: seek timed out after %u ms",
					__func__, seek_timeout);

stop:
	/* stop seeking */
	retval = si4705_fm_tune_status(radio, 0x01, 0x01,
		NULL, NULL, NULL, NULL, NULL);

done:
	/* try again, if timed out */
	if ((retval == 0) && timed_out)
		retval = -EAGAIN;

	return retval;
}

/*
 * si4705_start - switch on radio
 */
int si4705_start(struct si4705_device *radio)
{
	int retval;
	u16 threshold;

	retval = si4705_power_up(radio);
	if (retval < 0)
		goto done;

	retval = si4705_set_property(radio, GPO_IEN, GPO_IEN_STCIEN_MASK
		 | GPO_IEN_STCREP_MASK);
	if (retval < 0)
		goto done;

	retval = si4705_set_deemphasis(radio, de);
	if (retval < 0)
		goto done;

	retval = si4705_set_property(radio, FM_SEEK_BAND_BOTTOM, 7600);
	if (retval < 0)
		goto done;

	retval = si4705_set_property(radio, FM_SEEK_BAND_TOP, 10800);
	if (retval < 0)
		goto done;

	retval = si4705_set_property(radio, FM_SEEK_FREQ_SPACING, 5);
	if (retval < 0)
		goto done;

	threshold = si4705_get_seek_tune_snr_threshold_value(radio);

	retval = si4705_set_property(radio,
				     FM_SEEK_TUNE_SNR_THRESHOLD, threshold);
	if (retval < 0)
		goto done;

	threshold = si4705_get_seek_tune_rssi_threshold_value(radio);

	retval = si4705_set_property(radio,
				     FM_SEEK_TUNE_RSSI_THRESHOLD, threshold);
	if (retval < 0)
		goto done;

done:
	return retval;
}

/*
 * si4705_stop - switch off radio
 */
int si4705_stop(struct si4705_device *radio)
{
	int retval;

	retval = si4705_set_property(radio, RDS_CONFIG, 0x00);
	if (retval < 0)
		goto done;

	retval = si4705_set_property(radio, RX_HARD_MUTE, 0x03);
	if (retval < 0)
		goto done;

	retval = si4705_power_down(radio);

done:
	return retval;
}

/*
 * si4705_rds_on - switch on rds reception
 */
static int si4705_rds_on(struct si4705_device *radio)
{
	int retval;

	retval = si4705_set_property(radio, GPO_IEN, GPO_IEN_STCIEN_MASK
		| GPO_IEN_RDSIEN_MASK | GPO_IEN_STCREP_MASK);
	if (retval < 0)
		goto done;

	retval = si4705_set_property(radio, RDS_INT_SOURCE,
				RDS_INT_SOURCE_RECV_MASK);
	if (retval < 0)
		goto done;

	retval = si4705_set_property(radio, RDS_CONFIG, RDS_CONFIG_RDSEN_MASK |
				(3 << RDS_CONFIG_BLETHA_SHFT) |
				(3 << RDS_CONFIG_BLETHB_SHFT) |
				(3 << RDS_CONFIG_BLETHC_SHFT) |
				(3 << RDS_CONFIG_BLETHD_SHFT));

done:
	return retval;
}


/**************************************************************************
 * File Operations Interface
 **************************************************************************/

/*
 * si4705_fops_open - file open
 */
int si4705_fops_open(struct file *file)
{
	struct si4705_device *radio = video_drvdata(file);
	int retval = 0;

	mutex_lock(&radio->lock);
	radio->users++;

	if (radio->users == 1) {
		/* start radio */
		retval = si4705_start(radio);
		if (retval < 0)
			goto done;
	}

done:
	mutex_unlock(&radio->lock);
	return retval;
}

/*
 * si4705_fops_release - file release
 */
int si4705_fops_release(struct file *file)
{
	struct si4705_device *radio = video_drvdata(file);
	int retval = 0;

	/* safety check */
	if (!radio)
		return -ENODEV;

	mutex_lock(&radio->lock);
	radio->users--;
	if (radio->users == 0)
		/* stop radio */
		retval = si4705_stop(radio);

	mutex_unlock(&radio->lock);

	return retval;
}

/*
 * si4705_fops_read - read RDS data
 */
static ssize_t si4705_fops_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	struct si4705_device *radio = video_drvdata(file);
	int retval = 0;
	unsigned int block_count = 0;
	u16 rdsen = 0;

	/* switch on rds reception */
	mutex_lock(&radio->lock);

	retval = si4705_get_property(radio, RDS_CONFIG, &rdsen);
	if (retval < 0)
		goto done;

	rdsen &= RDS_CONFIG_RDSEN_MASK;

	if (rdsen == 0)
		si4705_rds_on(radio);

	/* block if no new data available */
	while (radio->wr_index == radio->rd_index) {
		if (file->f_flags & O_NONBLOCK) {
			retval = -EWOULDBLOCK;
			goto done;
		}
		if (wait_event_interruptible(radio->read_queue,
			radio->wr_index != radio->rd_index) < 0) {
			retval = -EINTR;
			goto done;
		}
	}

	/* calculate block count from byte count */
	count /= 3;

	/* copy RDS block out of internal buffer and to user buffer */
	while (block_count < count) {
		if (radio->rd_index == radio->wr_index)
			break;

		/* always transfer rds complete blocks */
		if (copy_to_user(buf, &radio->buffer[radio->rd_index], 3))
			/* retval = -EFAULT; */
			break;

		/* increment and wrap read pointer */
		radio->rd_index += 3;
		if (radio->rd_index >= radio->buf_size)
			radio->rd_index = 0;

		/* increment counters */
		block_count++;
		buf += 3;
		retval += 3;
	}

done:
	mutex_unlock(&radio->lock);
	return retval;
}

/*
 * si4705_fops_poll - poll RDS data
 */
static unsigned int si4705_fops_poll(struct file *file,
		struct poll_table_struct *pts)
{
	struct si4705_device *radio = video_drvdata(file);
	int retval = 0;
	u16 rdsen = 0;

	/* switch on rds reception */

	mutex_lock(&radio->lock);

	retval = si4705_get_property(radio, RDS_CONFIG, &rdsen);
	if (retval < 0)
		goto done;

	rdsen &= RDS_CONFIG_RDSEN_MASK;

	if (rdsen == 0)
		si4705_rds_on(radio);
	mutex_unlock(&radio->lock);

	poll_wait(file, &radio->read_queue, pts);

	if (radio->rd_index != radio->wr_index)
		retval = POLLIN | POLLRDNORM;

	return retval;

done:
	mutex_unlock(&radio->lock);
	return retval;
}

/*
 * si4705_fops - file operations interface
 */
static const struct v4l2_file_operations si4705_fops = {
	.owner			= THIS_MODULE,
	.read			= si4705_fops_read,
	.poll			= si4705_fops_poll,
	.unlocked_ioctl		= video_ioctl2,
	.open			= si4705_fops_open,
	.release		= si4705_fops_release,
};


/**************************************************************************
 * Video4Linux Interface
 **************************************************************************/

/*
 * si4705_vidioc_querycap - query device capabilities
 */
int si4705_vidioc_querycap(struct file *file, void *priv,
		struct v4l2_capability *capability)
{
	strlcpy(capability->driver, DRIVER_NAME, sizeof(capability->driver));
	strlcpy(capability->card, DRIVER_CARD, sizeof(capability->card));
	capability->version = DRIVER_KERNEL_VERSION;
	capability->capabilities = V4L2_CAP_HW_FREQ_SEEK |
		V4L2_CAP_TUNER | V4L2_CAP_RADIO;

	return 0;
}

/*
 * si4705_vidioc_queryctrl - enumerate control items
 */
static int si4705_vidioc_queryctrl(struct file *file, void *priv,
		struct v4l2_queryctrl *qc)
{
	struct si4705_device *radio = video_drvdata(file);
	int retval = -EINVAL;

	/* abort if qc->id is below V4L2_CID_BASE */
	if (qc->id < V4L2_CID_BASE)
		goto done;

	/* search video control */
	switch (qc->id) {
	case V4L2_CID_AUDIO_VOLUME:
		return v4l2_ctrl_query_fill(qc, 0, 15, 1, 15);
	case V4L2_CID_AUDIO_MUTE:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	}

	/* disable unsupported base controls */
	/* to satisfy kradio and such apps */
	if ((retval == -EINVAL) && (qc->id < V4L2_CID_LASTP1)) {
		qc->flags = V4L2_CTRL_FLAG_DISABLED;
		retval = 0;
	}

done:
	if (retval < 0)
		dev_warn(&radio->videodev->dev,
			"query controls failed with %d\n", retval);
	return retval;
}

/*
 * si4705_vidioc_g_ctrl - get the value of a control
 */
static int si4705_vidioc_g_ctrl(struct file *file, void *priv,
		struct v4l2_control *ctrl)
{
	struct si4705_device *radio = video_drvdata(file);
	int retval = 0;
	u16 vol_mute = 0;

	mutex_lock(&radio->lock);
	/* safety checks */
	retval = si4705_disconnect_check(radio);
	if (retval)
		goto done;

	switch (ctrl->id) {
	case V4L2_CID_AUDIO_VOLUME:
		retval = si4705_get_property(radio, RX_VOLUME, &vol_mute);
		if (retval)
			goto done;

		vol_mute &= RX_VOLUME_MASK;
		ctrl->value = si4705_vol_conv_value_to_index(radio, vol_mute);
		break;
	case V4L2_CID_AUDIO_MUTE:

		retval = si4705_get_property(radio, RX_HARD_MUTE, &vol_mute);
		if (retval)
			goto done;

		ctrl->value = (vol_mute == 0) ? 1 : 0;
		break;
	/*
	 * TODO : need to support op_mode change lp_mode <-> rich_mode
	 */
	default:
		retval = -EINVAL;
	}

done:
	if (retval < 0)
		dev_warn(&radio->videodev->dev,
			"get control failed with %d\n", retval);

	mutex_unlock(&radio->lock);
	return retval;
}

/*
 * si4705_vidioc_s_ctrl - set the value of a control
 */
static int si4705_vidioc_s_ctrl(struct file *file, void *priv,
		struct v4l2_control *ctrl)
{
	struct si4705_device *radio = video_drvdata(file);
	int retval = 0;
	u16 vol;

	mutex_lock(&radio->lock);
	/* safety checks */
	retval = si4705_disconnect_check(radio);
	if (retval)
		goto done;

	switch (ctrl->id) {
	case V4L2_CID_AUDIO_VOLUME:
		vol = si4705_vol_conv_index_to_value(radio, ctrl->value);
		retval = si4705_set_property(radio, RX_VOLUME, vol);
		break;
	case V4L2_CID_AUDIO_MUTE:
		if (ctrl->value == 1)
			retval = si4705_set_property(radio, RX_HARD_MUTE, 0x03);
		else
			retval = si4705_set_property(radio, RX_HARD_MUTE, 0x00);

		break;
	case V4L2_CID_TUNE_DEEMPHASIS:
		retval = si4705_set_deemphasis(radio, ctrl->value);
		break;

	/*
	 * TODO : need to support op_mode change lp_mode <-> rich_mode
	 */
	default:
		retval = -EINVAL;
	}

done:
	if (retval < 0)
		dev_warn(&radio->videodev->dev,
			"set control failed with %d\n", retval);
	mutex_unlock(&radio->lock);
	return retval;
}

/*
 * si4705_vidioc_g_audio - get audio attributes
 */
static int si4705_vidioc_g_audio(struct file *file, void *priv,
		struct v4l2_audio *audio)
{
	/* driver constants */
	audio->index = 0;
	strcpy(audio->name, "Radio");
	audio->capability = V4L2_AUDCAP_STEREO;
	audio->mode = 0;

	return 0;
}

/*
 * si4705_vidioc_g_tuner - get tuner attributes
 */
static int si4705_vidioc_g_tuner(struct file *file, void *priv,
		struct v4l2_tuner *tuner)
{
	struct si4705_device *radio = video_drvdata(file);
	int retval = 0;
	u8 afcrl = 0;
	u8 stblend = 0;
	u8 rssi = 0;
	u16 bandLow = 0;
	u16 bandHigh = 0;
	u16 stereo_th = 0;
	u16 mono_th = 0;

	mutex_lock(&radio->lock);
	/* safety checks */
	retval = si4705_disconnect_check(radio);
	if (retval)
		goto done;

	if (tuner->index != 0) {
		retval = -EINVAL;
		goto done;
	}

	retval = si4705_fm_rsq_status(radio, 0x00, NULL, &afcrl,
		&stblend, &rssi, NULL, NULL);
	if (retval < 0)
		goto done;

	pr_err("%s: afcrl = 0x%02x, stblend = 0x%02x, rssi = 0x%02x", __func__,
		afcrl, stblend, rssi);

	/* driver constants */
	strcpy(tuner->name, "FM");
	tuner->type = V4L2_TUNER_RADIO;
	tuner->capability = V4L2_TUNER_CAP_LOW | V4L2_TUNER_CAP_STEREO |
			    V4L2_TUNER_CAP_RDS | V4L2_TUNER_CAP_RDS_BLOCK_IO;

	/* range limits */
	retval = si4705_get_property(radio, FM_SEEK_BAND_BOTTOM, &bandLow);
	if (retval)
		goto done;

	retval = si4705_get_property(radio, FM_SEEK_BAND_TOP, &bandHigh);
	if (retval)
		goto done;

	tuner->rangelow  = FREQ_DEV_TO_V4L2(bandLow);
	tuner->rangehigh = FREQ_DEV_TO_V4L2(bandHigh);

	/* stereo indicator == stereo (instead of mono) */
	if ((stblend & FM_RSQ_STATUS_OUT_STBLEND) == 0)
		tuner->rxsubchans = V4L2_TUNER_SUB_MONO;
	else
		tuner->rxsubchans = V4L2_TUNER_SUB_MONO | V4L2_TUNER_SUB_STEREO;
	/* If there is a reliable method of detecting an RDS channel,
	   then this code should check for that before setting this
	   RDS subchannel. */
	tuner->rxsubchans |= V4L2_TUNER_SUB_RDS;

	retval = si4705_get_property(radio, FM_BLEND_STEREO_THRESHOLD,
		&stereo_th);
	if (retval < 0)
		goto done;

	retval = si4705_get_property(radio, FM_BLEND_MONO_THRESHOLD, &mono_th);
	if (retval < 0)
		goto done;

	/* mono/stereo selector */
	if (stereo_th == 127 || mono_th == 127)
		tuner->audmode = V4L2_TUNER_MODE_MONO;
	else
		tuner->audmode = V4L2_TUNER_MODE_STEREO;

	/* V4L2 does not specify how to use signal strength field. */
	/* it just descripbes higher is a better signal */
	/* update rssi value as real dbµV unit */
	tuner->signal = rssi;

	/* automatic frequency control: -1: freq to low, 1 freq to high */
	/* AFCRL does only indicate that freq. differs, not if too low/high */

	tuner->afc = (afcrl & FM_RSQ_STATUS_OUT_AFCRL) ? 1 : 0;

done:
	if (retval < 0)
		pr_err("%s: get tuner failed with %d", __func__, retval);
	mutex_unlock(&radio->lock);
	return retval;
}

/*
 * si4705_vidioc_s_tuner - set tuner attributes
 */
static int si4705_vidioc_s_tuner(struct file *file, void *priv,
		struct v4l2_tuner *tuner)
{
	struct si4705_device *radio = video_drvdata(file);
	int retval = 0;
	u16 stereo_th = 0;
	u16 mono_th = 0;
	u16 bandLow = 0;
	u16 bandHigh = 0;

	mutex_lock(&radio->lock);
	/* safety checks */
	retval = si4705_disconnect_check(radio);
	if (retval)
		goto done;

	if (tuner->index != 0)
		goto done;

	/* mono/stereo selector */
	switch (tuner->audmode) {
	case V4L2_TUNER_MODE_MONO:
		stereo_th = 127;
		mono_th = 127;  /* force mono */
		break;
	case V4L2_TUNER_MODE_STEREO:
		stereo_th = 0x0031;
		mono_th = 0x001E; /* try stereo */
		break;
	default:
		goto done;
	}

	retval = si4705_set_property(radio, FM_BLEND_STEREO_THRESHOLD,
		stereo_th);
	if (retval < 0)
		goto done;

	retval = si4705_set_property(radio, FM_BLEND_MONO_THRESHOLD, mono_th);
	if (retval < 0)
		goto done;

	/* range limit */
	if (tuner->rangelow) {
		bandLow = FREQ_V4L2_TO_DEV(tuner->rangelow);
		retval = si4705_set_property(radio,
						FM_SEEK_BAND_BOTTOM, bandLow);
		if (retval < 0)
			goto done;
	}

	if (tuner->rangehigh) {
		bandHigh = FREQ_V4L2_TO_DEV(tuner->rangehigh);
		retval = si4705_set_property(radio, FM_SEEK_BAND_TOP, bandHigh);
		if (retval < 0)
			goto done;
	}

done:
	if (retval < 0)
		pr_err("%s: set tuner failed with %d", __func__, retval);
	mutex_unlock(&radio->lock);
	return retval;
}

/*
 * si4705_vidioc_g_frequency - get tuner or modulator radio frequency
 */
static int si4705_vidioc_g_frequency(struct file *file, void *priv,
		struct v4l2_frequency *freq)
{
	struct si4705_device *radio = video_drvdata(file);
	int retval = 0;

	/* safety checks */
	mutex_lock(&radio->lock);
	retval = si4705_disconnect_check(radio);
	if (retval)
		goto done;

	if (freq->tuner != 0) {
		retval = -EINVAL;
		goto done;
	}

	freq->type = V4L2_TUNER_RADIO;
	retval = si4705_get_freq(radio, &freq->frequency);

done:
	if (retval < 0)
		pr_err("%s: set frequency failed with %d", __func__, retval);
	mutex_unlock(&radio->lock);
	return retval;
}

/*
 * si4705_vidioc_s_frequency - set tuner or modulator radio frequency
 */
static int si4705_vidioc_s_frequency(struct file *file, void *priv,
		struct v4l2_frequency *freq)
{
	struct si4705_device *radio = video_drvdata(file);
	int retval = 0;

	mutex_lock(&radio->lock);
	/* safety checks */
	retval = si4705_disconnect_check(radio);
	if (retval)
		goto done;

	if (freq->tuner != 0) {
		retval = -EINVAL;
		goto done;
	}

	retval = si4705_set_freq(radio, freq->frequency);

done:
	if (retval < 0)
		pr_err("%s: set frequency failed with %d", __func__, retval);
	mutex_unlock(&radio->lock);
	return retval;
}

/*
 * si4705_vidioc_s_hw_freq_seek - set hardware frequency seek
 */
static int si4705_vidioc_s_hw_freq_seek(struct file *file, void *priv,
		struct v4l2_hw_freq_seek *seek)
{
	struct si4705_device *radio = video_drvdata(file);
	int retval = 0;

	mutex_lock(&radio->lock);
	/* safety checks */
	retval = si4705_disconnect_check(radio);
	if (retval)
		goto done;

	if (seek->tuner != 0) {
		retval = -EINVAL;
		goto done;
	}

	retval = si4705_set_seek(radio, seek->wrap_around, seek->seek_upward);

done:
	if (retval < 0)
		pr_err("%s : set hardware frequency seek failed with %d",
							__func__, retval);
	mutex_unlock(&radio->lock);
	return retval;
}

/*
 * si4705_ioctl_ops - video device ioctl operations
 */
static const struct v4l2_ioctl_ops si4705_ioctl_ops = {
	.vidioc_querycap	= si4705_vidioc_querycap,
	.vidioc_queryctrl	= si4705_vidioc_queryctrl,
	.vidioc_g_ctrl		= si4705_vidioc_g_ctrl,
	.vidioc_s_ctrl		= si4705_vidioc_s_ctrl,
	.vidioc_g_audio		= si4705_vidioc_g_audio,
	.vidioc_g_tuner		= si4705_vidioc_g_tuner,
	.vidioc_s_tuner		= si4705_vidioc_s_tuner,
	.vidioc_g_frequency	= si4705_vidioc_g_frequency,
	.vidioc_s_frequency	= si4705_vidioc_s_frequency,
	.vidioc_s_hw_freq_seek	= si4705_vidioc_s_hw_freq_seek,
};

/*
 * si4705_viddev_template - video device interface
 */
struct video_device si4705_viddev_template = {
	.fops			= &si4705_fops,
	.name			= DRIVER_NAME,
	.release		= video_device_release,
	.ioctl_ops		= &si4705_ioctl_ops,
};
