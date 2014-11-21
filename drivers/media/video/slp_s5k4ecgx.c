/*
 * Driver for S5K4ECGX from Samsung Electronics
 *
 * 5Mp CMOS Image Sensor SoC with an Embedded Image Processor
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/i2c.h>
#include <linux/init.h>
#include <media/v4l2-device.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <linux/workqueue.h>
#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_exynos_media.h>
#include <linux/videodev2_exynos_camera.h>
#endif
#include <media/s5k4ecgx_platform.h>

#include "slp_s5k4ecgx.h"

#ifdef S5K4ECGX_USLEEP
#include <linux/hrtimer.h>
#endif

#define S5K4ECGX_BURST_MODE
#ifdef S5K4ECGX_BURST_MODE
	static u16 addr, value;

	static int len;
	static u8 buf[SZ_4K] = {0,};
#else
	static u8 buf[4] = {0,};
#endif

/* 5M mem size */
#define S5K4ECGX_INTLV_DATA_MAXSIZE	(5 << 20)

static const struct s5k4ecgx_framesize preview_frmsizes[] = {
	{ S5K4ECGX_PREVIEW_640, 640, 480 },
	{ S5K4ECGX_PREVIEW_176, 640, 480 },
	{ S5K4ECGX_PREVIEW_320, 320, 240 },
	{ S5K4ECGX_PREVIEW_720, 720, 480 },
	{ S5K4ECGX_PREVIEW_800, 800, 480 },
	{ S5K4ECGX_PREVIEW_1280, 1280, 720 },
};

static const struct s5k4ecgx_framesize capture_frmsizes[] = {
	{ S5K4ECGX_CAPTURE_5MP, 2560, 1920 },
	{ S5K4ECGX_CAPTURE_3MP, 2048, 1536 },
	{ S5K4ECGX_CAPTURE_2MP, 1600, 1200 },
	{ S5K4ECGX_CAPTURE_1MP, 1280, 960 },
	{ S5K4ECGX_CAPTURE_XGA, 1024, 768 },
	{ S5K4ECGX_CAPTURE_VGA, 640, 480 },
};

#define CHECK_ERR(x)	if (unlikely((x) < 0)) { \
				cam_err("i2c failed, err %d\n", x); \
				return x; \
			}

#define NELEMS(array) (sizeof(array) / sizeof(array[0]))

#ifdef S5K4ECGX_USLEEP
/*
 * Use msleep() if the sleep time is over 1000 us.
*/
static void s5k4ecgx_usleep(u32 usecs)
{
	ktime_t expires;
	u64 add_time = (u64)usecs * 1000;

	if (unlikely(!usecs))
		return;

	expires = ktime_add_ns(ktime_get(), add_time);
	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_hrtimeout(&expires, HRTIMER_MODE_ABS);
}
#endif

static void s5k4ecgx_cam_delay(struct v4l2_subdev *sd)
{
	struct s5k4ecgx_state *state = to_state(sd);

	if (state->scene_mode == SCENE_MODE_NIGHTSHOT ||
		state->scene_mode == SCENE_MODE_FIREWORKS)
		msleep(250);
	else
		msleep(200);
}

static inline int s5k4ecgx_read(struct i2c_client *client,
	u16 subaddr, u16 *data)
{
	u8 buf[2];
	int err = 0;
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.len = 2,
		.buf = buf,
	};

	*(u16 *)buf = cpu_to_be16(subaddr);

	err = i2c_transfer(client->adapter, &msg, 1);
	if (unlikely(err < 0))
		cam_err("ERR: %d register read fail\n", __LINE__);

	msg.flags = I2C_M_RD;

	err = i2c_transfer(client->adapter, &msg, 1);
	if (unlikely(err < 0))
		cam_err("ERR: %d register read fail\n", __LINE__);

	*data = ((buf[0] << 8) | buf[1]);

	return err;
}

static inline int s5k4ecgx_write(struct i2c_client *client,
		u32 packet)
{
	u8 buf[4];
	int err = 0, retry_count = 5;

	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.buf	= buf,
		.len	= 4,
	};

	if (!client->adapter) {
		cam_err("ERR - can't search i2c client adapter\n");
		return -EIO;
	}

	while (retry_count--) {
		*(u32 *)buf = cpu_to_be32(packet);
		err = i2c_transfer(client->adapter, &msg, 1);
		if (likely(err == 1))
			break;
		mdelay(10);
	}

	if (unlikely(err < 0)) {
		cam_err("ERR - 0x%08x write failed err=%d\n",
				(u32)packet, err);
		return err;
	}

	return (err != 1) ? -1 : 0;
}

/*
* Read a register.
*/
static int s5k4ecgx_read_reg(struct v4l2_subdev *sd,
		u16 page, u16 addr, u16 *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u32 page_cmd = (0x002C << 16) | page;
	u32 addr_cmd = (0x002E << 16) | addr;
	int err = 0;

	cam_dbg("page_cmd=0x%X, addr_cmd=0x%X\n", page_cmd, addr_cmd);

	err = s5k4ecgx_write(client, page_cmd);
	CHECK_ERR(err);
	err = s5k4ecgx_write(client, addr_cmd);
	CHECK_ERR(err);
	err = s5k4ecgx_read(client, 0x0F12, val);
	CHECK_ERR(err);

	return 0;
}

/* program multiple registers */
static int s5k4ecgx_write_regs(struct v4l2_subdev *sd,
		const u32 *packet, u32 num)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = -EAGAIN;
	u32 temp = 0;
	u16 delay = 0;
	int retry_count = 5;

	struct i2c_msg msg = {
		msg.addr = client->addr,
		msg.flags = 0,
		msg.len = 4,
		msg.buf = buf,
	};

	while (num--) {
		temp = *packet++;

		if ((temp & S5K4ECGX_DELAY) == S5K4ECGX_DELAY) {
			delay = temp & 0xFFFF;
			cam_dbg("line(%d):delay(0x%x):delay(%d)\n",
						__LINE__, delay, delay);
			msleep(delay);
			continue;
		}

#ifdef S5K4ECGX_BURST_MODE
		addr = temp >> 16;
		value = temp & 0xFFFF;

		switch (addr) {
		case 0x0F12:
			if (len == 0) {
				buf[len++] = addr >> 8;
				buf[len++] = addr & 0xFF;
			}
			buf[len++] = value >> 8;
			buf[len++] = value & 0xFF;

			if ((*packet >> 16) != addr) {
				msg.len = len;
				goto s5k4ecgx_burst_write;
			}
			break;

		case 0xFFFF:
			break;

		default:
			msg.len = 4;
			*(u32 *)buf = cpu_to_be32(temp);
			goto s5k4ecgx_burst_write;
		}

		continue;
#else
		*(u32 *)buf = cpu_to_be32(temp);
#endif

#ifdef S5K4ECGX_BURST_MODE
s5k4ecgx_burst_write:
		len = 0;
#endif
		retry_count = 5;

		while (retry_count--) {
			ret = i2c_transfer(client->adapter, &msg, 1);
			if (likely(ret == 1))
				break;
			mdelay(10);
		}

		if (unlikely(ret < 0)) {
			cam_err("ERR - 0x%08x write failed err=%d\n", \
				(u32)packet, ret);
			break;
		}
	}

	if (unlikely(ret < 0)) {
		cam_err("fail to write registers!!\n");
		return -EIO;
	}

	return 0;
}

static int camera_flash_manual_ctrl(struct v4l2_subdev *sd, int mode)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int ret = 0;
	cam_dbg(" E\n");
	if (mode == CAM_FLASH_ON) {
		/* FLASH mode */
		ret = state->pdata->flash_ctrl(CAM_FLASH_ON);
		state->preflash = PREFLASH_ON;
	} else if (mode == CAM_FLASH_TORCH) {
		/* TORCH mode */
		ret = state->pdata->flash_ctrl(CAM_FLASH_TORCH);
		state->preflash = PREFLASH_ON;
	} else {
		ret = state->pdata->flash_ctrl(CAM_FLASH_OFF);
		state->preflash = PREFLASH_OFF;
	}
	return ret;
}

static int s5k4ecgx_get_exif(struct v4l2_subdev *sd)
{
	return 0;
}

static int s5k4ecgx_get_lux(struct v4l2_subdev *sd)
{
	struct s5k4ecgx_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int msb = 0;
	int lsb = 0;
	int cur_lux = 0;
	bool lowlight = false;
	int err = 0;
	cam_dbg(" E\n");
	err = s5k4ecgx_write(client, 0x002C7000);
	CHECK_ERR(err);
	err = s5k4ecgx_write(client, 0x002E2C18);
	CHECK_ERR(err);
	err = s5k4ecgx_read(client, 0x0F12, (unsigned short *)&lsb);
	CHECK_ERR(err);
	err = s5k4ecgx_read(client, 0x0F12, (unsigned short *)&msb);
	CHECK_ERR(err);

	cur_lux = (msb << 16) | lsb;

	if (cur_lux >= 0x32)
		lowlight = false;
	else
		lowlight = true;

	state->lowlight = lowlight;

	cam_info("%s, s5k4ecgx_status.lowlight is %d\n", __func__,
		     state->lowlight);
	/*this value is under 0x0032 in low light condition */
	return err;
}

static int s5k4ecgx_ae_stable(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k4ecgx_state *state = to_state(sd);
	int val = 0;
	int err = 0;
	int cnt;
	do {
		if (state->focus.start == AUTO_FOCUS_OFF) {
			cam_info("af_start_preflash: AF is cancelled!\n");
			state->focus.status = CAMERA_AF_STATUS_MAX;
			break;
		}

		err = s5k4ecgx_write(client, 0x002C7000);
		CHECK_ERR(err);
		err = s5k4ecgx_write(client, 0x002E2C74);
		CHECK_ERR(err);
		err = s5k4ecgx_read(client, 0x0F12, (unsigned short *)&val);
		CHECK_ERR(err);

		if (val == 0x1)
			break;

		s5k4ecgx_usleep(10*1000);

	} while (cnt < 40);
	cam_info("%s, ret value is %d\n", __func__,
		     val);

	return err;
}

static int s5k4ecgx_set_awb_lock(struct v4l2_subdev *sd, int val)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = 0;
	cam_info(" E\n");

	if (val == state->awb_lock)
		return 0;

	if (val)
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_awb_lock_EVT1,\
		sizeof(s5k4ecgx_awb_lock_EVT1) / \
		sizeof(s5k4ecgx_awb_lock_EVT1[0]));
	else
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_awb_unlock_EVT1,\
		sizeof(s5k4ecgx_awb_unlock_EVT1) / \
		sizeof(s5k4ecgx_awb_unlock_EVT1[0]));

	CHECK_ERR(err);
	state->awb_lock = val;
	cam_info("awb %s\n", val ? "lock" : "unlock");
	return err;
}

static int s5k4ecgx_set_ae_lock(struct v4l2_subdev *sd, int val)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = 0;
	cam_info(" E\n");

	if (val == state->ae_lock)
		return 0;

	if (val)
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_ae_lock_EVT1,\
		sizeof(s5k4ecgx_ae_lock_EVT1) / \
		sizeof(s5k4ecgx_ae_lock_EVT1[0]));
	else
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_ae_unlock_EVT1,\
		sizeof(s5k4ecgx_ae_unlock_EVT1) / \
		sizeof(s5k4ecgx_ae_unlock_EVT1[0]));

	CHECK_ERR(err);
	state->ae_lock = val;
	cam_info("ae %s\n", val ? "lock" : "unlock");
	return err;
}

static int s5k4ecgx_check_dataline(struct v4l2_subdev *sd, s32 val)
{
	cam_info("DTP %s\n", val ? "ON" : "OFF");
	return 0;
}

static int s5k4ecgx_debug_sensor_status(struct v4l2_subdev *sd)
{
	return 0;
}

static int s5k4ecgx_check_sensor_status(struct v4l2_subdev *sd)
{
	return 0;
}

static inline int s5k4ecgx_check_esd(struct v4l2_subdev *sd)
{
	return 0;
}

static int s5k4ecgx_set_preview_start(struct v4l2_subdev *sd)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = -EINVAL;

	cam_info(" E\n");

	if ((state->runmode == RUNMODE_NOTREADY) ||
	    (state->runmode == RUNMODE_CAPTURING)) {
		cam_err("%s: ERROR - Invalid runmode\n", __func__);
		return -EPERM;
	}

	state->focus.status = CAMERA_AF_STATUS_MAX;

	if (state->runmode == RUNMODE_CAPTURE_STOP) {
		if (state->preflash != PREFLASH_OFF) {
			err = state->pdata->flash_ctrl(CAM_FLASH_OFF);
			state->preflash = PREFLASH_OFF;
		}
		/* AE/AWB unlock */
		err = s5k4ecgx_set_ae_lock(sd, false);
		err = s5k4ecgx_set_awb_lock(sd, false);

		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Preview_Return_EVT1,\
			sizeof(s5k4ecgx_Preview_Return_EVT1) / \
			sizeof(s5k4ecgx_Preview_Return_EVT1[0]));
	} else {
		switch (state->preview_frmsizes->index) {
		case S5K4ECGX_PREVIEW_1280:
			err = s5k4ecgx_write_regs(sd, \
				s5k4ecgx_1280_Preview_EVT1,\
				sizeof(s5k4ecgx_1280_Preview_EVT1) / \
				sizeof(s5k4ecgx_1280_Preview_EVT1[0]));
			cam_info("S5K4ECGX_PREVIEW_1280\n");
			break;
		case S5K4ECGX_PREVIEW_800:
			err = s5k4ecgx_write_regs(sd, \
				s5k4ecgx_800_Preview_EVT1,\
				sizeof(s5k4ecgx_800_Preview_EVT1) / \
				sizeof(s5k4ecgx_800_Preview_EVT1[0]));
			cam_info("S5K4ECGX_PREVIEW_800\n");
			break;
		case S5K4ECGX_PREVIEW_720:
			err = s5k4ecgx_write_regs(sd, \
				s5k4ecgx_720_Preview_EVT1,\
				sizeof(s5k4ecgx_720_Preview_EVT1) / \
				sizeof(s5k4ecgx_720_Preview_EVT1[0]));
			cam_info("S5K4ECGX_PREVIEW_720\n");
			break;
		case S5K4ECGX_PREVIEW_320:
			err = s5k4ecgx_write_regs(sd, \
				s5k4ecgx_320_Preview_EVT1,\
				sizeof(s5k4ecgx_320_Preview_EVT1) / \
				sizeof(s5k4ecgx_320_Preview_EVT1[0]));
			cam_info("S5K4ECGX_PREVIEW_320\n");
			break;
		case S5K4ECGX_PREVIEW_176:
			err = s5k4ecgx_write_regs(sd, \
				s5k4ecgx_176_Preview_EVT1,\
				sizeof(s5k4ecgx_176_Preview_EVT1) / \
				sizeof(s5k4ecgx_176_Preview_EVT1[0]));
			cam_info("S5K4ECGX_PREVIEW_640\n");
			break;
		case S5K4ECGX_PREVIEW_640:
		default:
			err = s5k4ecgx_write_regs(sd, \
				s5k4ecgx_640_Preview_EVT1,\
				sizeof(s5k4ecgx_640_Preview_EVT1) / \
				sizeof(s5k4ecgx_640_Preview_EVT1[0]));
			cam_info("S5K4ECGX_PREVIEW_640\n");
			break;
		}

		if (state->check_dataline)
			err = s5k4ecgx_check_dataline(sd, 1);
	}

	if (unlikely(err)) {
		cam_err("fail to make preview\n");
		return err;
	}

	state->runmode = RUNMODE_RUNNING;

	return 0;
}

static int s5k4ecgx_set_preview_stop(struct v4l2_subdev *sd)
{
	int err = 0;
	cam_info("do nothing.\n");

	return err;
}

static int s5k4ecgx_check_capture_status(struct v4l2_subdev *sd)
{
	int cnt = 0;
	int status = 0;
	int err = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	msleep(20);
	while (cnt < 150) {
		err = s5k4ecgx_write(client, 0x002C7000);
		CHECK_ERR(err);
		err = s5k4ecgx_write(client, 0x002E0244);
		CHECK_ERR(err);
		err = s5k4ecgx_read(client, 0x0F12, (unsigned short *)
				     &status);
		CHECK_ERR(err);

		if (!status)
			break;
		msleep(20);
		cnt++;
	}
	if (cnt)
		pr_info("[s5k4ecgx] wait time for capture frame : %dms\n",
			cnt * 10);
	if (status)
		pr_info("[s5k4ecgx] take picture failed.\n");

	return 0;
}

static int s5k4ecgx_set_capture_start(struct v4l2_subdev *sd)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = -EINVAL;
	cam_info(" E\n");
	/* check lowlight */
	if (state->lowlight) {
		/* check scene mode */
		if (state->scene_mode != SCENE_MODE_NIGHTSHOT &&
			state->scene_mode != SCENE_MODE_FIREWORKS) {
			/*low light capture start */
			err = s5k4ecgx_write_regs(sd,
				s5k4ecgx_Low_Cap_On_EVT1, \
				sizeof(s5k4ecgx_Low_Cap_On_EVT1) / \
				sizeof(s5k4ecgx_Low_Cap_On_EVT1[0]));
			cam_info("s5k4ecgx_set_capture_start  : s5k4ecgx_Low_Cap_On_EVT1\n");

			/* check flash on */
			if (state->flash_mode == FLASH_MODE_AUTO ||
				state->flash_mode == FLASH_MODE_ON) {
				/* AE/AWB unlock */
				err = s5k4ecgx_set_ae_lock(sd, false);
				CHECK_ERR(err);
				err = s5k4ecgx_set_awb_lock(sd, false);
				CHECK_ERR(err);
				/* Main flash on */
				state->pdata->flash_ctrl(CAM_FLASH_ON);
				state->preflash = PREFLASH_ON;
				/* 200ms AE delay */
				msleep(200);
			} else
				s5k4ecgx_cam_delay(sd);
		}
	} else {
		/* check flash on */
		if (state->flash_mode == FLASH_MODE_ON) {
			/* AE/AWB unlock */
			err = s5k4ecgx_set_ae_lock(sd, false);
			CHECK_ERR(err);
			err = s5k4ecgx_set_awb_lock(sd, false);
			CHECK_ERR(err);
			/* Main flash on */
			state->pdata->flash_ctrl(CAM_FLASH_ON);
			state->preflash = PREFLASH_ON;
			/* 200ms AE delay */
			msleep(200);
		}
	}

	/*capture start*/
	err = s5k4ecgx_write_regs(sd, s5k4ecgx_Capture_Start_EVT1, \
		sizeof(s5k4ecgx_Capture_Start_EVT1) / \
		sizeof(s5k4ecgx_Capture_Start_EVT1[0]));
	cam_info("s5k4ecgx_set_capture_start  : s5k4ecgx_Capture_Start_EVT1\n");

	state->runmode = RUNMODE_CAPTURING;

	/*capture delay*/
	err = s5k4ecgx_check_capture_status(sd);

	/* FIX later - temp code*/
	state->pdata->flash_ctrl(CAM_FLASH_OFF);
	state->preflash = PREFLASH_OFF;

	if (unlikely(err)) {
		cam_err("failed to make capture\n");
		return err;
	}

	s5k4ecgx_get_exif(sd);

	return err;
}

static int s5k4ecgx_set_sensor_mode(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	struct s5k4ecgx_state *state = to_state(sd);

	if ((ctrl->value != SENSOR_CAMERA) &&
	(ctrl->value != SENSOR_MOVIE)) {
		cam_err("ERR: Not support.(%d)\n", ctrl->value);
		return -EINVAL;
	}

	state->sensor_mode = ctrl->value;

	return 0;
}

static int s5k4ecgx_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	cam_dbg("E\n");
	return 0;
}

static int s5k4ecgx_enum_framesizes(struct v4l2_subdev *sd, \
					struct v4l2_frmsizeenum *fsize)
{
	struct s5k4ecgx_state *state = to_state(sd);

	cam_dbg("s5k4ecgx_enum_framesizes E\n");

	/*
	* Return the actual output settings programmed to the camera
	*
	* The camera interface should read this value, this is the resolution
	* at which the sensor would provide framedata to the camera i/f
	* In case of image capture,
	* this returns the default camera resolution (VGA)
	*/
	if (state->req_fmt.priv == V4L2_PIX_FMT_MODE_CAPTURE) {
		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->capture_frmsizes->width;
		fsize->discrete.height = state->capture_frmsizes->height;
	} else {
		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->preview_frmsizes->width;
		fsize->discrete.height = state->preview_frmsizes->height;
	}

	cam_info("width - %d , height - %d\n",
		fsize->discrete.width, fsize->discrete.height);

	return 0;
}

static int s5k4ecgx_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	cam_dbg("E\n");

	return err;
}

static int s5k4ecgx_set_fmt_size(struct v4l2_subdev *sd)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = 0;
	cam_info("s5k4ecgx_set_fmt_size E\n");

	if (state->req_fmt.priv == V4L2_PIX_FMT_MODE_CAPTURE) {
		switch (state->capture_frmsizes->index) {
		case S5K4ECGX_CAPTURE_5MP:
			err = s5k4ecgx_write_regs(sd,
					s5k4ecgx_5M_Capture_EVT1,\
					sizeof(s5k4ecgx_5M_Capture_EVT1) /
					sizeof(s5k4ecgx_5M_Capture_EVT1[0]));
			cam_info("s5k4ecgx_set_fmt_size  : S5K4ECGX_CAPTURE_5MP\n");
			break;
		case S5K4ECGX_CAPTURE_3MP:
			err = s5k4ecgx_write_regs(sd,
					s5k4ecgx_3M_Capture_EVT1,
					sizeof(s5k4ecgx_3M_Capture_EVT1) /
					sizeof(s5k4ecgx_3M_Capture_EVT1[0]));
			cam_info("s5k4ecgx_set_fmt_size  : S5K4ECGX_CAPTURE_3MP\n");
			break;
		case S5K4ECGX_CAPTURE_2MP:
			err = s5k4ecgx_write_regs(sd,
					s5k4ecgx_2M_Capture_EVT1,
					sizeof(s5k4ecgx_2M_Capture_EVT1) /
					sizeof(s5k4ecgx_2M_Capture_EVT1[0]));
			cam_info("s5k4ecgx_set_fmt_size  : S5K4ECGX_CAPTURE_2MP\n");
			break;
		case S5K4ECGX_CAPTURE_1MP:
			err = s5k4ecgx_write_regs(sd,
					s5k4ecgx_1M_Capture_EVT1,
					sizeof(s5k4ecgx_1M_Capture_EVT1) /
					sizeof(s5k4ecgx_1M_Capture_EVT1[0]));
			cam_info("s5k4ecgx_set_fmt_size  : S5K4ECGX_CAPTURE_1MP\n");
			break;
		case S5K4ECGX_CAPTURE_XGA:
			err = s5k4ecgx_write_regs(sd,
					s5k4ecgx_XGA_Capture_EVT1,
					sizeof(s5k4ecgx_XGA_Capture_EVT1) /
					sizeof(s5k4ecgx_XGA_Capture_EVT1[0]));
			cam_info("s5k4ecgx_set_fmt_size  : S5K4ECGX_CAPTURE_XGA\n");
			break;
		case S5K4ECGX_CAPTURE_VGA:
			err = s5k4ecgx_write_regs(sd,
					s5k4ecgx_VGA_Capture_EVT1,
					sizeof(s5k4ecgx_VGA_Capture_EVT1) /
					sizeof(s5k4ecgx_VGA_Capture_EVT1[0]));
			cam_info("s5k4ecgx_set_fmt_size  : S5K4ECGX_CAPTURE_VGA\n");
			break;
		default:
			cam_err("ERR: Invalid capture size\n");
			break;
		}
	}

	if (unlikely(err < 0)) {
		cam_err("i2c_write for set framerate\n");
		return -EIO;
	}

	return err;
}

static int s5k4ecgx_s_fmt(struct v4l2_subdev *sd, \
	struct v4l2_mbus_framefmt *ffmt)
{
	struct s5k4ecgx_state *state = to_state(sd);
	u32 num_entries = 0;
	u32 i;
	u32 err = 0;
	u32 width, height = 0;

	cam_info("s5k4ecgx_s_fmt E\n");
	/*
	 * Just copying the requested format as of now.
	 * We need to check here what are the formats the camera support, and
	 * set the most appropriate one according to the request from FIMC
	 */

	width = state->req_fmt.width = ffmt->width;
	height = state->req_fmt.height = ffmt->height;

	if (ffmt->colorspace == V4L2_COLORSPACE_JPEG)
		state->req_fmt.priv = V4L2_PIX_FMT_MODE_CAPTURE;
	else
		state->req_fmt.priv = V4L2_PIX_FMT_MODE_PREVIEW;

	switch (state->req_fmt.priv) {
	case V4L2_PIX_FMT_MODE_PREVIEW:
		cam_info("V4L2_PIX_FMT_MODE_PREVIEW\n");
		num_entries = ARRAY_SIZE(preview_frmsizes);
		for (i = 0; i < num_entries; i++) {
			if (width ==	preview_frmsizes[i].width &&
				height == preview_frmsizes[i].height) {
				state->preview_frmsizes = &preview_frmsizes[i];
				break;
			}
			if (i == (num_entries - 1))
				state->preview_frmsizes = &preview_frmsizes[0];
		}
		break;

	case V4L2_PIX_FMT_MODE_CAPTURE:
		cam_info("V4L2_PIX_FMT_MODE_CAPTURE\n");
		num_entries = ARRAY_SIZE(capture_frmsizes);
		for (i = 0; i < num_entries; i++) {
			if (width == capture_frmsizes[i].width && \
				height == capture_frmsizes[i].height) {
				state->capture_frmsizes = &capture_frmsizes[i];
				break;
			}
		}
		break;

	default:
		cam_err
		("ERR(EINVAL) : Invalid capture size width(%d), height(%d)\n",\
		ffmt->width, ffmt->height);
		return -EINVAL;
	}

	/*set frame size of preview or capture*/
	err = s5k4ecgx_set_fmt_size(sd);

	return err;
}

static int s5k4ecgx_set_frame_rate(struct v4l2_subdev *sd, u32 fps)
{
	int err = 0;

	cam_info("frame rate %d\n\n", fps);

	switch (fps) {
	case 7:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_FPS_7_EVT1,
				sizeof(s5k4ecgx_FPS_7_EVT1) / \
				sizeof(s5k4ecgx_FPS_7_EVT1[0]));
		break;
	case 15:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_FPS_15_EVT1,
				sizeof(s5k4ecgx_FPS_15_EVT1) / \
				sizeof(s5k4ecgx_FPS_15_EVT1[0]));

		break;
	case 20:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_FPS_20_EVT1,
				sizeof(s5k4ecgx_FPS_20_EVT1) / \
				sizeof(s5k4ecgx_FPS_20_EVT1[0]));

		break;
	case 24:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_FPS_24_EVT1,
				sizeof(s5k4ecgx_FPS_24_EVT1) / \
				sizeof(s5k4ecgx_FPS_24_EVT1[0]));
		break;
	case 30:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_FPS_30_EVT1,
				sizeof(s5k4ecgx_FPS_30_EVT1) / \
				sizeof(s5k4ecgx_FPS_30_EVT1[0]));
		break;
	default:
		cam_err("ERR: Invalid framerate\n");
		break;
	}

	if (unlikely(err < 0)) {
		cam_err("i2c_write for set framerate\n");
		return -EIO;
	}

	return err;
}

static int s5k4ecgx_g_parm(struct v4l2_subdev *sd,\
	struct v4l2_streamparm *parms)
{
	int err = 0;

	cam_dbg("E\n");

	return err;
}

static int s5k4ecgx_s_parm(struct v4l2_subdev *sd, \
	struct v4l2_streamparm *parms)
{
	int err = 0;
	u32 fps = 0;
	u32 denom = parms->parm.capture.timeperframe.denominator;
	u32 numer = parms->parm.capture.timeperframe.numerator;
	struct s5k4ecgx_state *state = to_state(sd);

	cam_dbg("E\n");

	if (denom >= 1 && numer == 1)
		fps = denom / numer;
	else if (denom == 1 && numer == 0)
		fps = 0;

	if (fps != state->set_fps) {
		if (fps < 0 && fps > 30) {
			cam_err("invalid frame rate %d\n", fps);
			fps = 30;
		}
		state->req_fps = fps;

		if (state->initialized) {
			err = s5k4ecgx_set_frame_rate(sd, state->req_fps);
			if (err >= 0)
				state->set_fps = state->req_fps;
		}

	}

	return err;
}

static int s5k4ecgx_control_stream(struct v4l2_subdev *sd, int cmd)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = 0;

	switch (cmd) {
	case STREAM_START:
		cam_warn("WARN: do nothing\n");
		break;

	case STREAM_STOP:
		cam_dbg("stream stop!!!\n");
		if (state->runmode == RUNMODE_CAPTURING)
			state->runmode = RUNMODE_CAPTURE_STOP;

		break;

	default:
		cam_err("ERR: Invalid cmd\n");
		break;
	}

	if (unlikely(err))
		cam_err("failed to stream start(stop)\n");

	return err;
}

static int s5k4ecgx_init(struct v4l2_subdev *sd, u32 val)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = -EINVAL;

	cam_dbg("E\n");

	/* set initial regster value */
	if (state->sensor_mode == SENSOR_CAMERA) {
		cam_info("load camera common setting\n");
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_init_reg1_EVT1,
			sizeof(s5k4ecgx_init_reg1_EVT1) / \
			sizeof(s5k4ecgx_init_reg1_EVT1[0]));

		msleep(20);

		err |= s5k4ecgx_write_regs(sd, s5k4ecgx_init_reg2_EVT1,
			sizeof(s5k4ecgx_init_reg2_EVT1) / \
			sizeof(s5k4ecgx_init_reg2_EVT1[0]));
	} else {
		cam_info("load recording setting\n");
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_init_reg1_EVT1,
			sizeof(s5k4ecgx_init_reg1_EVT1) / \
			sizeof(s5k4ecgx_init_reg1_EVT1[0]));

		msleep(20);

		err = s5k4ecgx_write_regs(sd, s5k4ecgx_init_reg2_EVT1,
			sizeof(s5k4ecgx_init_reg2_EVT1) / \
			sizeof(s5k4ecgx_init_reg2_EVT1[0]));
	}

	if (unlikely(err)) {
		cam_err("failed to init\n");
		return err;
	}

	state->runmode = RUNMODE_INIT;

	/* We stop stream-output from sensor when starting camera. */
	err = s5k4ecgx_control_stream(sd, STREAM_STOP);
	if (unlikely(err < 0))
		return err;
	msleep(150);

	state->initialized = 1;

	return 0;
}

static int s5k4ecgx_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = 0;

	cam_info("stream mode = %d\n", enable);

	switch (enable) {
	case STREAM_MODE_CAM_OFF:
		if (state->sensor_mode == SENSOR_CAMERA) {
			if (state->check_dataline)
				err = s5k4ecgx_check_dataline(sd, 0);
			else
				err = s5k4ecgx_control_stream(sd, STREAM_STOP);
		}
		break;

	case STREAM_MODE_CAM_ON:
		/* The position of this code need to be adjusted later */
		if ((state->sensor_mode == SENSOR_CAMERA)
		&& (state->req_fmt.priv == V4L2_PIX_FMT_MODE_CAPTURE))
			err = s5k4ecgx_set_capture_start(sd);
		else
			err = s5k4ecgx_set_preview_start(sd);
		break;

	case STREAM_MODE_MOVIE_ON:
		cam_dbg("do nothing(movie on)!!\n");
		break;

	case STREAM_MODE_MOVIE_OFF:
		cam_dbg("do nothing(movie off)!!\n");
		break;

	default:
		cam_err("ERR: Invalid stream mode\n");
		break;
	}

	if (unlikely(err < 0)) {
		cam_err("ERR: faild\n");
		return err;
	}

	return 0;
}

static int s5k4ecgx_get_af_1stsearch(struct v4l2_subdev *sd,
		struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int af_status = -1;
	int err = 0;
	int ret = 0;
	cam_info("- Start\n");
	err = s5k4ecgx_write(client, 0x002C7000);
	CHECK_ERR(err);
	err = s5k4ecgx_write(client, 0x002E2EEE);
	CHECK_ERR(err);
	err = s5k4ecgx_read(client, 0x0F12, (unsigned short *)&af_status);
	CHECK_ERR(err);

	switch (af_status & 0xff) {
	case AF_PROGRESS_1ST:
		ret = CAMERA_AF_STATUS_IN_PROGRESS;
		break;

	case AF_SUCCESS_1ST:
		ret = CAMERA_AF_STATUS_SUCCESS;
		break;

	case AF_FAIL_1ST:
	default:
		ret = CAMERA_AF_STATUS_FAIL;
		break;
	}

	ctrl->value = ret;
	cam_info("- END%d\n", ret);
	return ret;
}

static int s5k4ecgx_get_af_2ndsearch(struct v4l2_subdev *sd,
			struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int af_status = -1;
	int err = 0;
	int ret = 0;
	cam_info(" E\n");
	err = s5k4ecgx_write(client, 0x002C7000);
	CHECK_ERR(err);
	err = s5k4ecgx_write(client, 0x002E2207);
	CHECK_ERR(err);
	err = s5k4ecgx_read(client, 0x0F12, (unsigned short *)&af_status);
	CHECK_ERR(err);

	switch (af_status & 0xff) {
	case AF_SUCCESS_2ND:
		ret = CAMERA_AF_STATUS_SUCCESS;
		break;

	case AF_PROGRESS_2ND:
	default:
		ret = CAMERA_AF_STATUS_IN_PROGRESS;
		break;
	}

	ctrl->value = ret;
	cam_info(" - END%d\n", ret);
	return ret;
}

static int s5k4ecgx_get_af_result(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int status = 0;
	cam_info(" - START\n");
	if (state->focus.first == true) {
		state->focus.first = false;
		s5k4ecgx_cam_delay(sd);
	}

	/* 1st search af */
	s5k4ecgx_cam_delay(sd);

	status = s5k4ecgx_get_af_1stsearch(sd, ctrl);

	if (status != CAMERA_AF_STATUS_SUCCESS) {
		state->focus.status = status;
		if (status == CAMERA_AF_STATUS_FAIL)
			state->pdata->flash_ctrl(CAM_FLASH_OFF);
			state->preflash = PREFLASH_OFF;
		return status;
	}

	/* 2nd search af */
	s5k4ecgx_cam_delay(sd);

	status = s5k4ecgx_get_af_2ndsearch(sd, ctrl);

	state->focus.status = status;
	state->pdata->flash_ctrl(CAM_FLASH_OFF);
	state->preflash = PREFLASH_OFF;

	cam_info("af_status = %d\n", status);
	cam_info(" - END\n");
	return 0;
}


static int s5k4ecgx_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = 0;

	/* V4L2_CID_PRIVATE_BASE==0x08000000 */
	/* V4L2_CID_CAMERA_CLASS_BASE==0x009a0900 */
	/* V4L2_CID_BASE==0x00980900 */

	if (ctrl->id > V4L2_CID_PRIVATE_BASE)
		cam_dbg("ctrl->id:%d, value=%d V4L2_CID_PRIVATE_BASE\n",\
		ctrl->id - V4L2_CID_PRIVATE_BASE, ctrl->value);
	else if (ctrl->id > V4L2_CID_CAMERA_CLASS_BASE)
		cam_dbg("ctrl->id:%d, value=%d V4L2_CID_CAMERA_CLASS_BASE\n",\
		ctrl->id - V4L2_CID_CAMERA_CLASS_BASE, ctrl->value);
	else
		cam_dbg("ctrl->id:%d, value=%d V4L2_CID_BASE\n", \
		ctrl->id - V4L2_CID_BASE,	ctrl->value);

	mutex_lock(&state->ctrl_lock);

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_AUTO_FOCUS_RESULT:
		ctrl->value = state->focus.status;
		break;

	case V4L2_CID_CAM_JPEG_MEMSIZE:
		ctrl->value = S5K4ECGX_INTLV_DATA_MAXSIZE;
		break;

	case V4L2_CID_CAM_JPEG_MAIN_SIZE:
		ctrl->value = S5K4ECGX_INTLV_DATA_MAXSIZE;
		break;

	case V4L2_CID_CAM_JPEG_MAIN_OFFSET:
		ctrl->value = 0;
		break;

	case V4L2_CID_CAM_JPEG_THUMB_SIZE:
		ctrl->value = 0;
		break;

	case V4L2_CID_CAM_JPEG_THUMB_OFFSET:
		ctrl->value = 0;
		break;

	case V4L2_CID_CAM_JPEG_POSTVIEW_OFFSET:
		ctrl->value = 0;
		break;

	case V4L2_CID_CAMERA_EXIF_FLASH:
		ctrl->value = state->exif.flash;
		break;

	case V4L2_CID_CAMERA_ISO:
		ctrl->value = state->exif.iso;
		break;

	case V4L2_CID_CAMERA_EXIF_ISO:
		ctrl->value = state->exif.iso;
		break;

	case V4L2_CID_CAMERA_EXIF_TV:
		ctrl->value = state->exif.tv;
		break;

	case V4L2_CID_CAMERA_EXIF_BV:
		ctrl->value = state->exif.bv;
		break;

	case V4L2_CID_CAMERA_EXIF_EBV:
		ctrl->value = state->exif.ebv;
		break;

	case V4L2_CID_WHITE_BALANCE_PRESET:
		ctrl->value = state->wb_mode;
		break;

	default:
		cam_err("no such control id %d\n",
				ctrl->id - V4L2_CID_PRIVATE_BASE);
		break;
	}

	mutex_unlock(&state->ctrl_lock);

	return err;
}

static int s5k4ecgx_set_iso(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err;
	struct s5k4ecgx_state *state = to_state(sd);
	cam_dbg("E, value %d\n", ctrl->value);

retry:
	switch (ctrl->value) {
	case ISO_AUTO:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_ISO_Auto_EVT1, \
			sizeof(s5k4ecgx_ISO_Auto_EVT1) / \
			sizeof(s5k4ecgx_ISO_Auto_EVT1[0]));
		break;

	case ISO_50:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_ISO_50_EVT1, \
			sizeof(s5k4ecgx_ISO_50_EVT1) / \
			sizeof(s5k4ecgx_ISO_50_EVT1[0]));
		break;

	case ISO_100:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_ISO_100_EVT1, \
			sizeof(s5k4ecgx_ISO_100_EVT1) / \
			sizeof(s5k4ecgx_ISO_100_EVT1[0]));
		break;

	case ISO_200:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_ISO_200_EVT1, \
			sizeof(s5k4ecgx_ISO_200_EVT1) / \
			sizeof(s5k4ecgx_ISO_200_EVT1[0]));
		break;

	case ISO_400:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_ISO_400_EVT1, \
			sizeof(s5k4ecgx_ISO_400_EVT1) / \
			sizeof(s5k4ecgx_ISO_400_EVT1[0]));
		break;

	default:
		cam_warn("invalid value, %d\n", ctrl->value);
		ctrl->value = ISO_AUTO;
		goto retry;
	}

	state->exif.iso = ctrl->value;

	cam_dbg("X\n");
	return 0;
}

static int s5k4ecgx_set_metering(struct v4l2_subdev *sd, int val)
{
	int err;
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case METERING_CENTER:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Metering_Center_EVT1, \
			sizeof(s5k4ecgx_Metering_Center_EVT1) / \
			sizeof(s5k4ecgx_Metering_Center_EVT1[0]));
		break;

	case METERING_SPOT:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Metering_Spot_EVT1, \
			sizeof(s5k4ecgx_Metering_Spot_EVT1) / \
			sizeof(s5k4ecgx_Metering_Spot_EVT1[0]));
		break;

	case METERING_MATRIX:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Metering_Matrix_EVT1, \
			sizeof(s5k4ecgx_Metering_Matrix_EVT1) / \
			sizeof(s5k4ecgx_Metering_Matrix_EVT1[0]));
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = METERING_CENTER;
		goto retry;
	}

	cam_dbg("X\n");
	return 0;
}

static int s5k4ecgx_set_exposure(struct v4l2_subdev *sd, \
	struct v4l2_control *ctrl)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = -EINVAL;

	cam_dbg("E\n");

	if (state->check_dataline)
		return 0;

	switch (ctrl->value) {
	case 0:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_EV_Minus_4_EVT1, \
			sizeof(s5k4ecgx_EV_Minus_4_EVT1) / \
			sizeof(s5k4ecgx_EV_Minus_4_EVT1[0]));
		break;
	case 1:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_EV_Minus_3_EVT1, \
			sizeof(s5k4ecgx_EV_Minus_3_EVT1) / \
			sizeof(s5k4ecgx_EV_Minus_3_EVT1[0]));

		break;
	case 2:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_EV_Minus_2_EVT1, \
			sizeof(s5k4ecgx_EV_Minus_2_EVT1) / \
			sizeof(s5k4ecgx_EV_Minus_2_EVT1[0]));
		break;
	case 3:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_EV_Minus_1_EVT1, \
			sizeof(s5k4ecgx_EV_Minus_1_EVT1) / \
			sizeof(s5k4ecgx_EV_Minus_1_EVT1[0]));
		break;
	case 4:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_EV_Default_EVT1, \
			sizeof(s5k4ecgx_EV_Default_EVT1) / \
			sizeof(s5k4ecgx_EV_Default_EVT1[0]));
		break;
	case 5:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_EV_Plus_1_EVT1, \
			sizeof(s5k4ecgx_EV_Plus_1_EVT1) / \
			sizeof(s5k4ecgx_EV_Plus_1_EVT1[0]));
		break;
	case 6:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_EV_Plus_2_EVT1, \
			sizeof(s5k4ecgx_EV_Plus_2_EVT1) / \
			sizeof(s5k4ecgx_EV_Plus_2_EVT1[0]));
		break;
	case 7:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_EV_Plus_3_EVT1, \
			sizeof(s5k4ecgx_EV_Plus_3_EVT1) / \
			sizeof(s5k4ecgx_EV_Plus_3_EVT1[0]));
		break;
	case 8:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_EV_Plus_4_EVT1, \
			sizeof(s5k4ecgx_EV_Plus_4_EVT1) / \
			sizeof(s5k4ecgx_EV_Plus_4_EVT1[0]));
		break;
	default:
		cam_err("ERR: invalid brightness(%d)\n", ctrl->value);
		return err;
		break;
	}

	if (unlikely(err < 0)) {
		cam_err("ERR: i2c_write for set brightness\n");
		return -EIO;
	}

	return 0;
}

static int s5k4ecgx_set_whitebalance(struct v4l2_subdev *sd, int val)
{
	int err;
	struct s5k4ecgx_state *state = to_state(sd);
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case WHITE_BALANCE_AUTO:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_WB_Auto_EVT1, \
			sizeof(s5k4ecgx_WB_Auto_EVT1) / \
			sizeof(s5k4ecgx_WB_Auto_EVT1[0]));
		break;

	case WHITE_BALANCE_SUNNY:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_WB_Sunny_EVT1, \
			sizeof(s5k4ecgx_WB_Sunny_EVT1) / \
			sizeof(s5k4ecgx_WB_Sunny_EVT1[0]));
		break;

	case WHITE_BALANCE_CLOUDY:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_WB_Cloudy_EVT1, \
			sizeof(s5k4ecgx_WB_Cloudy_EVT1) / \
			sizeof(s5k4ecgx_WB_Cloudy_EVT1[0]));
		break;

	case WHITE_BALANCE_TUNGSTEN:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_WB_Tungsten_EVT1, \
			sizeof(s5k4ecgx_WB_Tungsten_EVT1) / \
			sizeof(s5k4ecgx_WB_Tungsten_EVT1[0]));
		break;

	case WHITE_BALANCE_FLUORESCENT:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_WB_Fluorescent_EVT1, \
			sizeof(s5k4ecgx_WB_Fluorescent_EVT1) / \
			sizeof(s5k4ecgx_WB_Fluorescent_EVT1[0]));
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = WHITE_BALANCE_AUTO;
		goto retry;
	}

	state->wb_mode = val;
	cam_dbg("X\n");
	return 0;
}

static int s5k4ecgx_set_scene_mode(struct v4l2_subdev *sd, int val)
{
	int err;
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case SCENE_MODE_NONE:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Default_EVT1, \
			sizeof(s5k4ecgx_Scene_Default_EVT1) / \
			sizeof(s5k4ecgx_Scene_Default_EVT1[0]));
		break;

	case SCENE_MODE_PORTRAIT:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Portrait_EVT1, \
			sizeof(s5k4ecgx_Scene_Portrait_EVT1) / \
			sizeof(s5k4ecgx_Scene_Portrait_EVT1[0]));
		break;

	case SCENE_MODE_LANDSCAPE:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Landscape_EVT1, \
			sizeof(s5k4ecgx_Scene_Landscape_EVT1) / \
			sizeof(s5k4ecgx_Scene_Landscape_EVT1[0]));
		break;

	case SCENE_MODE_SPORTS:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Sports_EVT1, \
			sizeof(s5k4ecgx_Scene_Sports_EVT1) / \
			sizeof(s5k4ecgx_Scene_Sports_EVT1[0]));
		break;

	case SCENE_MODE_PARTY_INDOOR:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Party_Indoor_EVT1,\
			sizeof(s5k4ecgx_Scene_Party_Indoor_EVT1) / \
			sizeof(s5k4ecgx_Scene_Party_Indoor_EVT1[0]));
		break;

	case SCENE_MODE_BEACH_SNOW:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Beach_Snow_EVT1, \
			sizeof(s5k4ecgx_Scene_Beach_Snow_EVT1) / \
			sizeof(s5k4ecgx_Scene_Beach_Snow_EVT1[0]));
		break;

	case SCENE_MODE_SUNSET:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Sunset_EVT1, \
			sizeof(s5k4ecgx_Scene_Sunset_EVT1) / \
			sizeof(s5k4ecgx_Scene_Sunset_EVT1[0]));
		break;

	case SCENE_MODE_DUSK_DAWN:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Duskdawn_EVT1, \
			sizeof(s5k4ecgx_Scene_Duskdawn_EVT1) / \
			sizeof(s5k4ecgx_Scene_Duskdawn_EVT1[0]));
		break;

	case SCENE_MODE_FALL_COLOR:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Fall_Color_EVT1, \
			sizeof(s5k4ecgx_Scene_Fall_Color_EVT1) / \
			sizeof(s5k4ecgx_Scene_Fall_Color_EVT1[0]));
		break;

	case SCENE_MODE_NIGHTSHOT:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Nightshot_EVT1, \
			sizeof(s5k4ecgx_Scene_Nightshot_EVT1) / \
			sizeof(s5k4ecgx_Scene_Nightshot_EVT1[0]));
		break;

	case SCENE_MODE_BACK_LIGHT:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Backlight_EVT1, \
			sizeof(s5k4ecgx_Scene_Backlight_EVT1) / \
			sizeof(s5k4ecgx_Scene_Backlight_EVT1[0]));
		break;

	case SCENE_MODE_FIREWORKS:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Fireworks_EVT1, \
			sizeof(s5k4ecgx_Scene_Fireworks_EVT1) / \
			sizeof(s5k4ecgx_Scene_Fireworks_EVT1[0]));
		break;

	case SCENE_MODE_TEXT:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Scene_Text_EVT1, \
			sizeof(s5k4ecgx_Scene_Text_EVT1) / \
			sizeof(s5k4ecgx_Scene_Text_EVT1[0]));
		break;

	case SCENE_MODE_CANDLE_LIGHT:
		err = s5k4ecgx_write_regs(sd, \
			s5k4ecgx_Scene_Candle_Light_EVT1, \
			sizeof(s5k4ecgx_Scene_Candle_Light_EVT1) / \
			sizeof(s5k4ecgx_Scene_Candle_Light_EVT1[0]));
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = SCENE_MODE_NONE;
		goto retry;
	}

	cam_dbg("X\n");
	return 0;
}

static int s5k4ecgx_set_effect(struct v4l2_subdev *sd, int val)
{
	int err;
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case IMAGE_EFFECT_NONE:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Effect_Normal_EVT1, \
			sizeof(s5k4ecgx_Effect_Normal_EVT1) / \
			sizeof(s5k4ecgx_Effect_Normal_EVT1[0]));
		break;

	case IMAGE_EFFECT_SEPIA:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Effect_Sepia_EVT1, \
			sizeof(s5k4ecgx_Effect_Sepia_EVT1) / \
			sizeof(s5k4ecgx_Effect_Sepia_EVT1[0]));
		break;

	case IMAGE_EFFECT_BNW:
		err = s5k4ecgx_write_regs(sd, \
			s5k4ecgx_Effect_Black_White_EVT1, \
			sizeof(s5k4ecgx_Effect_Black_White_EVT1) / \
			sizeof(s5k4ecgx_Effect_Black_White_EVT1[0]));
		break;

	case IMAGE_EFFECT_NEGATIVE:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Effect_Negative_EVT1, \
			sizeof(s5k4ecgx_Effect_Negative_EVT1) / \
			sizeof(s5k4ecgx_Effect_Negative_EVT1[0]));
		break;

	case IMAGE_EFFECT_AQUA:
		err = s5k4ecgx_write_regs(sd, \
			s5k4ecgx_Effect_Solarization_EVT1, \
			sizeof(s5k4ecgx_Effect_Solarization_EVT1) / \
			sizeof(s5k4ecgx_Effect_Solarization_EVT1[0]));
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = IMAGE_EFFECT_NONE;
		goto retry;
	}

	cam_dbg("X\n");
	return 0;
}

static int s5k4ecgx_set_wdr(struct v4l2_subdev *sd, int val)
{
	int err;
	cam_dbg("E, value %d\n", val);

retry:
	switch (val) {
	case WDR_OFF:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_WDR_off_EVT1, \
			sizeof(s5k4ecgx_WDR_off_EVT1) / \
			sizeof(s5k4ecgx_WDR_off_EVT1[0]));
		break;

	case WDR_ON:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_WDR_on_EVT1, \
			sizeof(s5k4ecgx_WDR_on_EVT1) / \
			sizeof(s5k4ecgx_WDR_on_EVT1[0]));
		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = WDR_OFF;
		goto retry;
	}

	cam_dbg("X\n");
	return 0;
}

static int s5k4ecgx_set_jpeg_quality(struct v4l2_subdev *sd, int val)
{
	int err = -1;
	cam_dbg("E, value %d\n", val);

	if (val <= 65)		/* Normal */
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Jpeg_Quality_Low_EVT1,\
			sizeof(s5k4ecgx_Jpeg_Quality_Low_EVT1) / \
			sizeof(s5k4ecgx_Jpeg_Quality_Low_EVT1[0]));
	else if (val <= 75)	/* Fine */
		err = s5k4ecgx_write_regs
			(sd, s5k4ecgx_Jpeg_Quality_Normal_EVT1,\
			sizeof(s5k4ecgx_Jpeg_Quality_Normal_EVT1) / \
			sizeof(s5k4ecgx_Jpeg_Quality_Normal_EVT1[0]));
	else		/* Superfine */
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_Jpeg_Quality_High_EVT1,\
			sizeof(s5k4ecgx_Jpeg_Quality_High_EVT1) / \
			sizeof(s5k4ecgx_Jpeg_Quality_High_EVT1[0]));

	CHECK_ERR(err);

	cam_dbg("X\n");
	return 0;
}

static int s5k4ecgx_return_focus(struct v4l2_subdev *sd)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = -EINVAL;

	cam_info("E\n");

	switch (state->focus.mode) {
	case FOCUS_MODE_MACRO:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_AF_Macro_mode_1_EVT1,\
			sizeof(s5k4ecgx_AF_Macro_mode_1_EVT1) / \
			sizeof(s5k4ecgx_AF_Macro_mode_1_EVT1[0]));

		s5k4ecgx_cam_delay(sd);

		err = s5k4ecgx_write_regs(sd, s5k4ecgx_AF_Macro_mode_2_EVT1,\
			sizeof(s5k4ecgx_AF_Macro_mode_2_EVT1) / \
			sizeof(s5k4ecgx_AF_Macro_mode_2_EVT1[0]));

		s5k4ecgx_cam_delay(sd);

		err = s5k4ecgx_write_regs(sd, s5k4ecgx_AF_Macro_mode_3_EVT1,\
			sizeof(s5k4ecgx_AF_Macro_mode_3_EVT1) / \
			sizeof(s5k4ecgx_AF_Macro_mode_3_EVT1[0]));

		break;

	default:
		err = s5k4ecgx_write_regs(sd, s5k4ecgx_AF_Normal_mode_1_EVT1,\
			sizeof(s5k4ecgx_AF_Normal_mode_1_EVT1) / \
			sizeof(s5k4ecgx_AF_Normal_mode_1_EVT1[0]));

		s5k4ecgx_cam_delay(sd);

		err = s5k4ecgx_write_regs(sd, s5k4ecgx_AF_Normal_mode_2_EVT1,\
			sizeof(s5k4ecgx_AF_Normal_mode_2_EVT1) / \
			sizeof(s5k4ecgx_AF_Normal_mode_2_EVT1[0]));

		s5k4ecgx_cam_delay(sd);

		err = s5k4ecgx_write_regs(sd, s5k4ecgx_AF_Normal_mode_3_EVT1,\
			sizeof(s5k4ecgx_AF_Normal_mode_3_EVT1) / \
			sizeof(s5k4ecgx_AF_Normal_mode_3_EVT1[0]));

		break;
	}

	CHECK_ERR(err);
	return 0;
}

/* PX: Stop AF */
static int s5k4ecgx_stop_af(struct v4l2_subdev *sd, s32 touch)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = 0;

	cam_info("E\n");
	mutex_lock(&state->af_lock);

	switch (state->focus.status) {
	case CAMERA_AF_STATUS_FAIL:
	case CAMERA_AF_STATUS_SUCCESS:
		cam_dbg("Stop AF, focus mode %d, AF result %d\n",
			state->focus.mode, state->focus.status);

		err = s5k4ecgx_set_ae_lock(sd, false);
		err = s5k4ecgx_set_awb_lock(sd, false);
		if (unlikely(err)) {
			cam_err("%s: ERROR, fail to set lock\n", __func__);
			goto err_out;
		}
		state->focus.status = CAMERA_AF_STATUS_MAX;
		state->preflash = PREFLASH_NONE;
		break;

	case CAMERA_AF_STATUS_MAX:
		break;

	default:
		cam_err("%s: WARNING, unnecessary calling. AF status=%d\n",
			__func__, state->focus.status);
		/* Return 0. */
		goto err_out;
		break;
	}

	if (!touch) {
		/* We move lens to default position if af is cancelled.*/
		err = s5k4ecgx_return_focus(sd);
		if (unlikely(err)) {
			cam_err("%s: ERROR, fail to af_norma_mode (%d)\n",
				__func__, err);
			goto err_out;
		}
	}

	mutex_unlock(&state->af_lock);
	cam_info("X\n");
	return 0;

err_out:
	mutex_unlock(&state->af_lock);
	return err;
}

static int s5k4ecgx_set_af_mode(struct v4l2_subdev *sd, int val)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = -1;
	u32 af_cancel = 0;
	cam_info(" - value %d\n", val);

	mutex_lock(&state->af_lock);

retry:
	af_cancel = (u32)val & FOCUS_MODE_DEFAULT;
	switch (val) {
	case FOCUS_MODE_AUTO:
	case FOCUS_MODE_INFINITY:
		state->focus.mode = val;

		err = s5k4ecgx_write_regs(sd, s5k4ecgx_AF_Normal_mode_1_EVT1,\
			sizeof(s5k4ecgx_AF_Normal_mode_1_EVT1) / \
			sizeof(s5k4ecgx_AF_Normal_mode_1_EVT1[0]));

		s5k4ecgx_cam_delay(sd);

		err = s5k4ecgx_write_regs(sd, s5k4ecgx_AF_Normal_mode_2_EVT1,\
			sizeof(s5k4ecgx_AF_Normal_mode_2_EVT1) / \
			sizeof(s5k4ecgx_AF_Normal_mode_2_EVT1[0]));

		s5k4ecgx_cam_delay(sd);

		err = s5k4ecgx_write_regs(sd, s5k4ecgx_AF_Normal_mode_3_EVT1,\
			sizeof(s5k4ecgx_AF_Normal_mode_3_EVT1) / \
			sizeof(s5k4ecgx_AF_Normal_mode_3_EVT1[0]));

		break;

	case FOCUS_MODE_MACRO:
		state->focus.mode = val;

		err = s5k4ecgx_write_regs(sd, s5k4ecgx_AF_Macro_mode_1_EVT1,\
			sizeof(s5k4ecgx_AF_Macro_mode_1_EVT1) / \
			sizeof(s5k4ecgx_AF_Macro_mode_1_EVT1[0]));

		s5k4ecgx_cam_delay(sd);

		err = s5k4ecgx_write_regs(sd, s5k4ecgx_AF_Macro_mode_2_EVT1,\
			sizeof(s5k4ecgx_AF_Macro_mode_2_EVT1) / \
			sizeof(s5k4ecgx_AF_Macro_mode_2_EVT1[0]));

		s5k4ecgx_cam_delay(sd);

		err = s5k4ecgx_write_regs(sd, s5k4ecgx_AF_Macro_mode_3_EVT1,\
			sizeof(s5k4ecgx_AF_Macro_mode_3_EVT1) / \
			sizeof(s5k4ecgx_AF_Macro_mode_3_EVT1[0]));

		break;

	default:
		cam_warn("invalid value, %d\n", val);
		val = FOCUS_MODE_AUTO;
		goto retry;
	}

	state->focus.mode = val;
	mutex_unlock(&state->af_lock);

	if (af_cancel)
		s5k4ecgx_stop_af(sd, 0);

	return 0;

	CHECK_ERR(err);
	return 0;
}

/* PX: Do AF */
static int s5k4ecgx_do_af(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k4ecgx_state *state = to_state(sd);
	u16 read_value = 0;
	u32 count = 0;
	int err = 0;

	cam_info("E\n");

	/* AE, AWB Lock */
	err = s5k4ecgx_set_ae_lock(sd, true);
	CHECK_ERR(err);
	err = s5k4ecgx_set_awb_lock(sd, true);
	CHECK_ERR(err);

	/* AF start */
	err = s5k4ecgx_write_regs(sd, s5k4ecgx_Single_AF_Start_EVT1,\
			sizeof(s5k4ecgx_Single_AF_Start_EVT1) / \
			sizeof(s5k4ecgx_Single_AF_Start_EVT1[0]));
	CHECK_ERR(err);

	/* 1 frame delay */
	s5k4ecgx_cam_delay(sd);

	/* AF Searching */
	cam_dbg("AF 1st search\n");

	/*1st search*/
	for (count = 0; count < FIRST_AF_SEARCH_COUNT; count++) {
		if (state->focus.start == AUTO_FOCUS_OFF) {
			cam_dbg("do_af: AF is cancelled while doing(1st)\n");
			state->focus.status = CAMERA_AF_STATUS_MAX;
			goto check_done;
		}

		/* 1 frame delay */
		s5k4ecgx_cam_delay(sd);

		read_value = 0x0;
		err = s5k4ecgx_write(client, 0x002C7000);
		CHECK_ERR(err);
		err = s5k4ecgx_write(client, 0x002E2EEE);
		CHECK_ERR(err);
		err = s5k4ecgx_read(client, 0x0F12, &read_value);
		CHECK_ERR(err);
		cam_info("1st AF status(%02d) = 0x%04X\n",
					count, read_value);

		if ((read_value & 0xff) != AF_PROGRESS_1ST)
			break;
	}

	if ((read_value & 0xff) != AF_SUCCESS_1ST) {
		cam_err("%s: ERROR, 1st AF failed. count=%d, read_val=0x%X\n\n",
					__func__, count, read_value);
		state->focus.status = CAMERA_AF_STATUS_FAIL;
		goto check_done;
	}

	/*2nd search*/
	cam_dbg("AF 2nd search\n");
	for (count = 0; count < SECOND_AF_SEARCH_COUNT; count++) {
		if (state->focus.start == AUTO_FOCUS_OFF) {
			cam_dbg("do_af: AF is cancelled while doing(2nd)\n");
			state->focus.status = CAMERA_AF_STATUS_MAX;
			goto check_done;
		}

		read_value = 0x0;
		err = s5k4ecgx_write(client, 0x002C7000);
		CHECK_ERR(err);
		err = s5k4ecgx_write(client, 0x002E2207);
		CHECK_ERR(err);
		err = s5k4ecgx_read(client, 0x0F12, &read_value);
		CHECK_ERR(err);
		cam_info("2nd AF status(%02d) = 0x%04X\n",
						count, read_value);
		if ((read_value & 0xff) == AF_SUCCESS_2ND)
			break;
	}

	if (count >= SECOND_AF_SEARCH_COUNT) {
		/* 0x01XX means "Not Finish". */
		cam_err("%s: ERROR, 2nd AF failed. read_val=0x%X\n\n",
			__func__, read_value & 0xff);
		state->focus.status = CAMERA_AF_STATUS_FAIL;
		goto check_done;
	}

	cam_info("AF Success!\n");
	state->focus.status = CAMERA_AF_STATUS_SUCCESS;

check_done:
	/* restore write mode */

	/* We only unlocked AE,AWB in case of being cancelled.
	 * But we now unlock it unconditionally if AF is started,
	 */
	if (state->focus.status == CAMERA_AF_STATUS_MAX) {
		cam_dbg("%s: Single AF cancelled.\n", __func__);
		/* AE, AWB Lock */
		err = s5k4ecgx_set_ae_lock(sd, false);
		CHECK_ERR(err);
		err = s5k4ecgx_set_awb_lock(sd, false);
		CHECK_ERR(err);
	} else {
		state->focus.start = AUTO_FOCUS_OFF;
		cam_dbg("%s: Single AF finished\n", __func__);
	}

	if ((state->preflash == PREFLASH_ON) &&
	    (state->sensor_mode == SENSOR_CAMERA)) {
		state->pdata->flash_ctrl(CAM_FLASH_OFF);
		state->preflash = PREFLASH_OFF;
		if (state->focus.status == CAMERA_AF_STATUS_MAX)
			state->preflash = PREFLASH_NONE;
	}

	/* Notice: we here turn touch flag off set previously
	 * when doing Touch AF. */

	return 0;
}

static void s5k4ecgx_af_worker(struct work_struct *work)
{
	struct s5k4ecgx_state *state = container_of(work, \
				struct s5k4ecgx_state, af_work);
	struct v4l2_subdev *sd = &state->sd;
	int err = -EINVAL;

	cam_info("E\n");

	mutex_lock(&state->af_lock);

	/* 1. Check Low Light */
	err = s5k4ecgx_get_lux(sd);

	/* 2. Turn on Pre Flash */
	switch (state->flash_mode) {
	case FLASH_MODE_AUTO:
		if (!state->lowlight) {
			/* flash not needed */
			break;
		}

	case FLASH_MODE_ON:
		state->pdata->flash_ctrl(CAM_FLASH_TORCH);
		state->preflash = PREFLASH_ON;
		break;

	case FLASH_MODE_OFF:
	default:
		break;
	}

	if (state->preflash == PREFLASH_ON) {
		/* 3. Waiting until AE Stable */
		err = s5k4ecgx_ae_stable(sd);
	} else if (state->focus.start == AUTO_FOCUS_OFF) {
		cam_info("af_start_preflash: AF is cancelled!\n");
		state->focus.status = CAMERA_AF_STATUS_MAX;
	}

	if (state->focus.status == CAMERA_AF_STATUS_MAX) {
		if (state->preflash == PREFLASH_ON) {
			state->pdata->flash_ctrl(CAM_FLASH_OFF);
			state->preflash = PREFLASH_OFF;
		}
		goto out;
	}

	s5k4ecgx_do_af(sd);

out:
	mutex_unlock(&state->af_lock);
	cam_info("X\n");
	return;
}

static int s5k4ecgx_set_af(struct v4l2_subdev *sd, s32 val)
{
	struct s5k4ecgx_state *state = to_state(sd);
	int err = 0;

	cam_info("%s: %s, focus mode %d\n", __func__,
			val ? "start" : "stop", state->focus.mode);

	if (unlikely((u32)val >= AUTO_FOCUS_MAX)) {
		cam_err("%s: ERROR, invalid value(%d)\n", __func__, val);
		return -EINVAL;
	}

/* need to check the AF scenario with SLP team - temp code */
/*
	if (state->focus.start == val)
		return 0;
*/
	state->focus.start = val;

	if (val == AUTO_FOCUS_ON) {
		err = queue_work(state->workqueue, &state->af_work);
		if (likely(err))
			state->focus.status = CAMERA_AF_STATUS_IN_PROGRESS;
		else
			cam_warn("WARNING, AF is still processing. So new AF cannot start\n");
	} else {
		/* Cancel AF */
		/* 1. AE/AWB UnLock */
		err = s5k4ecgx_set_ae_lock(sd, false);
		CHECK_ERR(err);
		err = s5k4ecgx_set_awb_lock(sd, false);
		CHECK_ERR(err);
		/* 2. Turn off on pre flash */
		state->pdata->flash_ctrl(CAM_FLASH_OFF);
		state->preflash = PREFLASH_OFF;
		/* 3. Cancel AF mode */
		err = s5k4ecgx_set_af_mode(sd, state->focus.mode);
		cam_info("set_af: AF cancel requested!\n");
	}

	cam_info("X\n");
	return 0;
}

static int s5k4ecgx_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	/* struct i2c_client *client = v4l2_get_subdevdata(sd); */
	struct s5k4ecgx_state *state = to_state(sd);
	int err = 0;

	/* V4L2_CID_PRIVATE_BASE == 0x08000000 */
	/* V4L2_CID_CAMERA_CLASS_BASE == 0x009a0900 */
	/* V4L2_CID_BASE == 0x00980900 */

	if (ctrl->id > V4L2_CID_PRIVATE_BASE)
		cam_dbg("ctrl->id:%d, value=%d V4L2_CID_PRIVATE_BASE\n", \
		ctrl->id - V4L2_CID_PRIVATE_BASE, ctrl->value);
	else if (ctrl->id > V4L2_CID_CAMERA_CLASS_BASE)
		cam_dbg("ctrl->id:%d, value=%d V4L2_CID_CAMERA_CLASS_BASE\n",\
		ctrl->id - V4L2_CID_CAMERA_CLASS_BASE, ctrl->value);
	else
		cam_dbg("ctrl->id:%d, value=%d V4L2_CID_BASE\n", \
		ctrl->id - V4L2_CID_BASE,	ctrl->value);

	if ((ctrl->id != V4L2_CID_CAMERA_CHECK_DATALINE)
	&& (ctrl->id != V4L2_CID_CAMERA_SENSOR_MODE)
	&& ((ctrl->id != V4L2_CID_CAMERA_VT_MODE))
	&& (!state->initialized)) {
		cam_warn("camera isn't initialized\n");
		return 0;
	}

	if (ctrl->id != V4L2_CID_CAMERA_SET_AUTO_FOCUS)
		mutex_lock(&state->ctrl_lock);

	switch (ctrl->id) {
	case V4L2_CID_CAM_JPEG_QUALITY:
		err = s5k4ecgx_set_jpeg_quality(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_ISO:
		err = s5k4ecgx_set_iso(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_METERING:
		if (state->sensor_mode == SENSOR_CAMERA)
			err = s5k4ecgx_set_metering(sd, ctrl->value);
		break;

	case V4L2_CID_EXPOSURE:
		err = s5k4ecgx_set_exposure(sd, ctrl);
		cam_dbg("V4L2_CID_EXPOSURE [%d]\n", ctrl->value);
		break;

	case V4L2_CID_WHITE_BALANCE_PRESET:
	case V4L2_CID_CAMERA_WHITE_BALANCE:
		err = s5k4ecgx_set_whitebalance(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SCENE_MODE:
		state->scene_mode = ctrl->value;
		err = s5k4ecgx_set_scene_mode(sd, ctrl->value);
		break;

	case V4L2_CID_COLORFX:
		err = s5k4ecgx_set_effect(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_WDR:
		err = s5k4ecgx_set_wdr(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FLASH_MODE:
		state->flash_mode = ctrl->value;
		break;

	case V4L2_CID_FOCUS_AUTO_MODE:
	case V4L2_CID_CAMERA_FOCUS_MODE:
		err = s5k4ecgx_set_af_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SET_AUTO_FOCUS:
		err = s5k4ecgx_set_af(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_CHECK_DATALINE:
		state->check_dataline = ctrl->value;
		cam_dbg("check_dataline = %d\n", state->check_dataline);
		err = 0;
		break;

	case V4L2_CID_CAMERA_SENSOR_MODE:
		err = s5k4ecgx_set_sensor_mode(sd, ctrl);
		cam_dbg("sensor_mode = %d\n", ctrl->value);
		break;

	case V4L2_CID_CAMERA_CHECK_DATALINE_STOP:
		cam_dbg("do nothing\n");
		break;

	case V4L2_CID_CAMERA_CHECK_ESD:
		err = s5k4ecgx_check_esd(sd);
		break;

	case V4L2_CID_CAMERA_FRAME_RATE:
		err = s5k4ecgx_set_frame_rate(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_CHECK_SENSOR_STATUS:
		s5k4ecgx_debug_sensor_status(sd);
		err = s5k4ecgx_check_sensor_status(sd);
		break;

	default:
		cam_err("ERR(ENOIOCTLCMD)\n");
		/* no errors return.*/
		break;
	}

	if (ctrl->id != V4L2_CID_CAMERA_SET_AUTO_FOCUS)
		mutex_unlock(&state->ctrl_lock);

	cam_dbg("X\n");
	return err;
}

static const struct v4l2_subdev_core_ops s5k4ecgx_core_ops = {
	.init = s5k4ecgx_init,		/* initializing API */
	.g_ctrl = s5k4ecgx_g_ctrl,
	.s_ctrl = s5k4ecgx_s_ctrl,
};

static const struct v4l2_subdev_video_ops s5k4ecgx_video_ops = {
	.s_mbus_fmt	= s5k4ecgx_s_fmt,
	.s_stream = s5k4ecgx_s_stream,
	.enum_framesizes = s5k4ecgx_enum_framesizes,
	.g_parm	= s5k4ecgx_g_parm,
	.s_parm	= s5k4ecgx_s_parm,
};

static const struct v4l2_subdev_ops s5k4ecgx_ops = {
	.core = &s5k4ecgx_core_ops,
	.video = &s5k4ecgx_video_ops,
};

/*
 * s5k4ecgx_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int s5k4ecgx_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct s5k4ecgx_state *state = NULL;
	struct v4l2_subdev *sd = NULL;
	struct s5k4ecgx_platform_data *pdata = NULL;
	cam_dbg("E\n");

	state = kzalloc(sizeof(struct s5k4ecgx_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, S5K4ECGX_DRIVER_NAME);

	state->runmode = RUNMODE_NOTREADY;
	state->initialized = 0;
	state->req_fps = state->set_fps = 0;
	state->sensor_mode = SENSOR_CAMERA;
	state->zoom = 0;
	state->flash_mode = FLASH_MODE_BASE;
	state->lowlight = 0;
	state->awb_lock = 0;
	state->ae_lock = 0;
	state->preflash = PREFLASH_NONE;

	pdata = client->dev.platform_data;

	if (!pdata) {
		cam_err("no platform data\n");
		return -ENODEV;
	}

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &s5k4ecgx_ops);

	mutex_init(&state->ctrl_lock);
	mutex_init(&state->af_lock);

	state->workqueue = create_workqueue("cam_workqueue");
	if (unlikely(!state->workqueue)) {
		dev_err(&client->dev, "probe, fail to create workqueue\n");
		goto err_out;
	}
	INIT_WORK(&state->af_work, s5k4ecgx_af_worker);

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */

	 /*S5K4ECGX_PREVIEW_VGA*/
	state->preview_frmsizes = &preview_frmsizes[0];
	 /*S5K4ECGX_CAPTURE_5MP */
	state->capture_frmsizes = &capture_frmsizes[0];
	cam_dbg("preview_width: %d , preview_height: %d, "
	"capture_width: %d, capture_height: %d",
	state->preview_frmsizes->width, state->preview_frmsizes->height,
	state->capture_frmsizes->width, state->capture_frmsizes->height);

	state->req_fmt.width = state->preview_frmsizes->width;
	state->req_fmt.height = state->preview_frmsizes->height;

	state->pdata = pdata;

	if (!pdata->pixelformat)
		state->req_fmt.pixelformat = DEFAULT_FMT;
	else
		state->req_fmt.pixelformat = pdata->pixelformat;

	cam_dbg("probed!!\n");

	return 0;

err_out:
	kfree(state);
	return -ENOMEM;
}

static int s5k4ecgx_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct s5k4ecgx_state *state = to_state(sd);

	cam_dbg("E\n");

	destroy_workqueue(state->workqueue);

	state->initialized = 0;

	v4l2_device_unregister_subdev(sd);

	mutex_destroy(&state->ctrl_lock);
	mutex_destroy(&state->af_lock);

	kfree(to_state(sd));

	return 0;
}

static const struct i2c_device_id s5k4ecgx_id[] = {
	{ S5K4ECGX_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, s5k4ecgx_id);

static struct i2c_driver s5k4ecgx_i2c_driver = {
	.driver = {
		.name	= S5K4ECGX_DRIVER_NAME,
	},
	.probe		= s5k4ecgx_probe,
	.remove		= s5k4ecgx_remove,
	.id_table	= s5k4ecgx_id,
};

static int __init s5k4ecgx_mod_init(void)
{
	cam_dbg("E\n");
	return i2c_add_driver(&s5k4ecgx_i2c_driver);
}

static void __exit s5k4ecgx_mod_exit(void)
{
	cam_dbg("E\n");
	i2c_del_driver(&s5k4ecgx_i2c_driver);
}
module_init(s5k4ecgx_mod_init);
module_exit(s5k4ecgx_mod_exit);

MODULE_DESCRIPTION("S5K4ECGX MIPI sensor driver");
MODULE_AUTHOR("seungwoolee<samuell.lee@samsung.com>");
MODULE_AUTHOR("jinsookim<js1002.kim@samsung.com>");
MODULE_LICENSE("GPL");
