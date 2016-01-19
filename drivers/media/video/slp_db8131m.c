/*
 * linux/drivers/media/video/slp_db8131m.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/init.h>
#include <media/v4l2-device.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_exynos_camera.h>
#endif
#include <media/db8131m_platform.h>

#include "slp_db8131m.h"

#ifdef DB8131M_USLEEP
#include <linux/hrtimer.h>
#endif

#define CHECK_ERR(x)	if (unlikely((x) < 0)) { \
				cam_err("i2c failed, err %d\n", x); \
				return x; \
			}

#define NELEMS(array) (sizeof(array) / sizeof(array[0]))

static inline int db8131m_read(struct i2c_client *client,
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

/*
 * s5k6aafx sensor i2c write routine
 * <start>--<Device address><2Byte Subaddr><2Byte Value>--<stop>
 */
static inline int db8131m_write(struct i2c_client *client,
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

/* program multiple registers */
static int db8131m_write_regs(struct v4l2_subdev *sd,
		const u8 *packet, u32 num)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = -EAGAIN;
	int retry_count = 5;

	u8 buf[2] = {0,};

	struct i2c_msg msg = {
		msg.addr = client->addr,
		msg.flags = 0,
		msg.len = 2,
		msg.buf = buf,
	};

	while (num) {
		buf[0] = *packet++;
		buf[1] = *packet++;

		num -= 2;

		retry_count = 5;

		while (retry_count--) {
			ret = i2c_transfer(client->adapter, &msg, 1);
			if (likely(ret == 1))
				break;
			mdelay(10);
		}

		if (unlikely(ret < 0)) {
			cam_err("ERR - 0x%08x write failed err=%d\n",
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

static int db8131m_check_dataline(struct v4l2_subdev *sd, s32 val)
{
	return 0;
}

static int db8131m_debug_sensor_status(struct v4l2_subdev *sd)
{
	return 0;
}

static int db8131m_check_sensor_status(struct v4l2_subdev *sd)
{
	return 0;
}

static inline int db8131m_check_esd(struct v4l2_subdev *sd)
{
	return 0;
}

static int db8131m_set_preview_start(struct v4l2_subdev *sd)
{
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;

	cam_info("reset preview\n");

#ifdef CONFIG_LOAD_FILE
	err = db8131m_write_regs_from_sd(sd, "db8131m_preview");
#else
	err = db8131m_write_regs(sd, db8131m_preview,
		sizeof(db8131m_preview) / sizeof(db8131m_preview[0]));
#endif
	if (state->check_dataline)
		err = db8131m_check_dataline(sd, 1);
	if (unlikely(err)) {
		cam_err("fail to make preview\n");
		return err;
	}

	return 0;
}

static int db8131m_set_preview_stop(struct v4l2_subdev *sd)
{
	return 0;
}

static int db8131m_set_capture_start(struct v4l2_subdev *sd)
{
	return 0;
}

static int db8131m_set_sensor_mode(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	struct db8131m_state *state = to_state(sd);

	if ((ctrl->value != SENSOR_CAMERA) &&
	(ctrl->value != SENSOR_MOVIE)) {
		cam_err("ERR: Not support.(%d)\n", ctrl->value);
		return -EINVAL;
	}

	state->sensor_mode = ctrl->value;

	return 0;
}

static int db8131m_enum_framesizes(struct v4l2_subdev *sd, \
					struct v4l2_frmsizeenum *fsize)
{
	return 0;
}

static int db8131m_s_fmt(struct v4l2_subdev *sd,
	struct v4l2_mbus_framefmt *ffmt)
{
	struct db8131m_state *state = to_state(sd);
	u32 *width = NULL, *height = NULL;

	cam_dbg("E\n");
	/*
	 * Just copying the requested format as of now.
	 * We need to check here what are the formats the camera support, and
	 * set the most appropriate one according to the request from FIMC
	 */

	state->req_fmt.width = ffmt->width;
	state->req_fmt.height = ffmt->height;
	state->req_fmt.priv = ffmt->field;

	switch (state->req_fmt.priv) {
	case V4L2_PIX_FMT_MODE_PREVIEW:
		cam_dbg("V4L2_PIX_FMT_MODE_PREVIEW\n");
		width = &state->preview_frmsizes.width;
		height = &state->preview_frmsizes.height;
		break;

	case V4L2_PIX_FMT_MODE_CAPTURE:
		cam_dbg("V4L2_PIX_FMT_MODE_CAPTURE\n");
		width = &state->capture_frmsizes.width;
		height = &state->capture_frmsizes.height;
		break;

	default:
		cam_err("ERR(EINVAL)\n");
		return -EINVAL;
	}

	if ((*width != state->req_fmt.width) ||
		(*height != state->req_fmt.height)) {
		cam_err("ERR: Invalid size. width= %d, height= %d\n",
			state->req_fmt.width, state->req_fmt.height);
	}

	return 0;
}

static int db8131m_set_frame_rate(struct v4l2_subdev *sd, u32 fps)
{
	int err = 0;

	cam_info("frame rate %d\n\n", fps);

	switch (fps) {
	case 7:
		err = db8131m_write_regs(sd, db8131m_vt_7fps,
				sizeof(db8131m_vt_7fps) / \
				sizeof(db8131m_vt_7fps[0]));
		break;
	case 10:
		err = db8131m_write_regs(sd, db8131m_vt_10fps,
				sizeof(db8131m_vt_10fps) / \
				sizeof(db8131m_vt_10fps[0]));

		break;
	case 12:
		err = db8131m_write_regs(sd, db8131m_vt_12fps,
				sizeof(db8131m_vt_12fps) / \
				sizeof(db8131m_vt_12fps[0]));

		break;
	case 15:
		err = db8131m_write_regs(sd, db8131m_vt_15fps,
				sizeof(db8131m_vt_15fps) / \
				sizeof(db8131m_vt_15fps[0]));
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

static int db8131m_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	int err = 0;

	cam_dbg("E\n");

	return err;
}

static int db8131m_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	int err = 0;
	u32 fps = 0;
	struct db8131m_state *state = to_state(sd);

	if (!state->vt_mode)
		return 0;

	cam_dbg("E\n");

	fps = parms->parm.capture.timeperframe.denominator /
			parms->parm.capture.timeperframe.numerator;

	if (fps != state->set_fps) {
		if (fps < 0 && fps > 30) {
			cam_err("invalid frame rate %d\n", fps);
			fps = 30;
		}
		state->req_fps = fps;

		if (state->initialized) {
			err = db8131m_set_frame_rate(sd, state->req_fps);
			if (err >= 0)
				state->set_fps = state->req_fps;
		}

	}

	return err;
}

static int db8131m_control_stream(struct v4l2_subdev *sd, u32 cmd)
{
	int err = 0;

	switch (cmd) {
	case 0: /* STREAM_STOP */
		cam_dbg("stream stop!!!\n");
		break;

	case 1: /* STREAM_START */
		cam_warn("WARN: do nothing\n");
		break;

	default:
		cam_err("ERR: Invalid cmd\n");
		break;
	}

	if (unlikely(err))
		cam_err("failed to stream start(stop)\n");

	return err;
}

static int db8131m_init(struct v4l2_subdev *sd, u32 val)
{
	/* struct i2c_client *client = v4l2_get_subdevdata(sd); */
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;

	cam_dbg("E\n");

	/* set initial regster value */
	if (state->sensor_mode == SENSOR_CAMERA) {
		cam_info("load camera common setting\n");
		err = db8131m_write_regs(sd, db8131m_common_1,
			sizeof(db8131m_common_1) / \
			sizeof(db8131m_common_1[0]));

		msleep(150);

		err |= db8131m_write_regs(sd, db8131m_common_2,
			sizeof(db8131m_common_2) / \
			sizeof(db8131m_common_2[0]));
	} else {
		cam_info("load recording setting\n");
		err = db8131m_write_regs(sd, db8131m_common_1,
			sizeof(db8131m_common_1) / \
			sizeof(db8131m_common_1[0]));

		msleep(150);

		err = db8131m_write_regs(sd, db8131m_common_2,
			sizeof(db8131m_common_2) / \
			sizeof(db8131m_common_2[0]));
	}
	if (unlikely(err)) {
		cam_err("failed to init\n");
		return err;
	}

	/* We stop stream-output from sensor when starting camera. */
	err = db8131m_control_stream(sd, 0);
	if (unlikely(err < 0))
		return err;
	msleep(150);

	state->initialized = 1;

	return 0;
}

static int db8131m_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct db8131m_state *state = to_state(sd);
	/* struct i2c_client *client = v4l2_get_subdevdata(sd); */
	int err = 0;

	cam_info("stream mode = %d\n", enable);

	switch (enable) {
	case STREAM_MODE_CAM_OFF:
		if (state->sensor_mode == SENSOR_CAMERA) {
			if (state->check_dataline)
				err = db8131m_check_dataline(sd, 0);
			else
				err = db8131m_control_stream(sd, 0);
		}
		break;

	case STREAM_MODE_CAM_ON:
		/* The position of this code need to be adjusted later */
		if ((state->sensor_mode == SENSOR_CAMERA)
		&& (state->req_fmt.priv == V4L2_PIX_FMT_MODE_CAPTURE))
			err = db8131m_set_capture_start(sd);
		else
			err = db8131m_set_preview_start(sd);
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

static int db8131m_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct db8131m_state *state = to_state(sd);
	int err = 0;

	cam_dbg("ctrl->id : %d\n", ctrl->id - V4L2_CID_PRIVATE_BASE);

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_EXIF_TV:
		ctrl->value = state->exif.shutter_speed;
		break;
	case V4L2_CID_CAMERA_EXIF_ISO:
		ctrl->value = state->exif.iso;
		break;
	default:
		cam_err("no such control id %d\n",
				ctrl->id - V4L2_CID_PRIVATE_BASE);
		break;
	}

	return err;
}

static int db8131m_set_brightness(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;

	cam_dbg("E\n");

	if (state->check_dataline)
		return 0;

	switch (ctrl->value) {
	case EV_MINUS_4:
		err = db8131m_write_regs(sd, db8131m_bright_m4, \
			sizeof(db8131m_bright_m4) / \
			sizeof(db8131m_bright_m4[0]));
		break;
	case EV_MINUS_3:
		err = db8131m_write_regs(sd, db8131m_bright_m3, \
			sizeof(db8131m_bright_m3) / \
			sizeof(db8131m_bright_m3[0]));

		break;
	case EV_MINUS_2:
		err = db8131m_write_regs(sd, db8131m_bright_m2, \
			sizeof(db8131m_bright_m2) / \
			sizeof(db8131m_bright_m2[0]));
		break;
	case EV_MINUS_1:
		err = db8131m_write_regs(sd, db8131m_bright_m1, \
			sizeof(db8131m_bright_m1) / \
			sizeof(db8131m_bright_m1[0]));
		break;
	case EV_DEFAULT:
		err = db8131m_write_regs(sd, db8131m_bright_default, \
			sizeof(db8131m_bright_default) / \
			sizeof(db8131m_bright_default[0]));
		break;
	case EV_PLUS_1:
		err = db8131m_write_regs(sd, db8131m_bright_p1, \
			sizeof(db8131m_bright_p1) / \
			sizeof(db8131m_bright_p1[0]));
		break;
	case EV_PLUS_2:
		err = db8131m_write_regs(sd, db8131m_bright_p2, \
			sizeof(db8131m_bright_p2) / \
			sizeof(db8131m_bright_p2[0]));
		break;
	case EV_PLUS_3:
		err = db8131m_write_regs(sd, db8131m_bright_p3, \
			sizeof(db8131m_bright_p3) / \
			sizeof(db8131m_bright_p3[0]));
		break;
	case EV_PLUS_4:
		err = db8131m_write_regs(sd, db8131m_bright_p4, \
			sizeof(db8131m_bright_p4) / \
			sizeof(db8131m_bright_p4[0]));
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

static int db8131m_set_blur(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	return 0;
}

static int db8131m_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	/* struct i2c_client *client = v4l2_get_subdevdata(sd); */
	struct db8131m_state *state = to_state(sd);
	int err = 0;

	cam_info("ctrl->id : %d, value=%d\n", ctrl->id - V4L2_CID_PRIVATE_BASE,
	ctrl->value);

	if ((ctrl->id != V4L2_CID_CAMERA_CHECK_DATALINE)
	&& (ctrl->id != V4L2_CID_CAMERA_SENSOR_MODE)
	&& ((ctrl->id != V4L2_CID_CAMERA_VT_MODE))
	&& (!state->initialized)) {
		cam_warn("camera isn't initialized\n");
		return 0;
	}

	switch (ctrl->id) {
	case V4L2_CID_CAM_PREVIEW_ONOFF:
		if (ctrl->value)
			err = db8131m_set_preview_start(sd);
		else
			err = db8131m_set_preview_stop(sd);
		cam_dbg("V4L2_CID_CAM_PREVIEW_ONOFF [%d]\n", ctrl->value);
		break;

	case V4L2_CID_CAM_CAPTURE:
		err = db8131m_set_capture_start(sd);
		cam_dbg("V4L2_CID_CAM_CAPTURE\n");
		break;

	case V4L2_CID_CAMERA_BRIGHTNESS:
		err = db8131m_set_brightness(sd, ctrl);
		cam_dbg("V4L2_CID_CAMERA_BRIGHTNESS [%d]\n", ctrl->value);
		break;

	case V4L2_CID_CAMERA_VGA_BLUR:
		err = db8131m_set_blur(sd, ctrl);
		cam_dbg("V4L2_CID_CAMERA_VGA_BLUR [%d]\n", ctrl->value);
		break;

	case V4L2_CID_CAMERA_VT_MODE:
		state->vt_mode = ctrl->value;
		break;

	case V4L2_CID_CAMERA_CHECK_DATALINE:
		state->check_dataline = ctrl->value;
		cam_dbg("check_dataline = %d\n", state->check_dataline);
		err = 0;
		break;

	case V4L2_CID_CAMERA_SENSOR_MODE:
		err = db8131m_set_sensor_mode(sd, ctrl);
		cam_dbg("sensor_mode = %d\n", ctrl->value);
		break;

	case V4L2_CID_CAMERA_CHECK_DATALINE_STOP:
		cam_dbg("do nothing\n");
		/*err = db8131m_check_dataline_stop(sd);*/
		break;

	case V4L2_CID_CAMERA_CHECK_ESD:
		err = db8131m_check_esd(sd);
		break;

	case V4L2_CID_CAMERA_FRAME_RATE:
		cam_dbg("do nothing\n");
		break;

	case V4L2_CID_CAMERA_CHECK_SENSOR_STATUS:
		db8131m_debug_sensor_status(sd);
		err = db8131m_check_sensor_status(sd);
		break;

	default:
		cam_err("ERR(ENOIOCTLCMD)\n");
		/* no errors return.*/
		break;
	}

	cam_dbg("X\n");
	return err;
}

static const struct v4l2_subdev_core_ops db8131m_core_ops = {
	.init = db8131m_init,		/* initializing API */
#if 0
	.queryctrl = db8131m_queryctrl,
	.querymenu = db8131m_querymenu,
#endif
	.g_ctrl = db8131m_g_ctrl,
	.s_ctrl = db8131m_s_ctrl,
};

static const struct v4l2_subdev_video_ops db8131m_video_ops = {
	/*.s_crystal_freq = db8131m_s_crystal_freq,*/
	.s_mbus_fmt	= db8131m_s_fmt,
	.s_stream = db8131m_s_stream,
	.enum_framesizes = db8131m_enum_framesizes,
	/*.enum_frameintervals = db8131m_enum_frameintervals,*/
	/*.enum_fmt = db8131m_enum_fmt,*/
	.g_parm	= db8131m_g_parm,
	.s_parm	= db8131m_s_parm,
};

static const struct v4l2_subdev_ops db8131m_ops = {
	.core = &db8131m_core_ops,
	.video = &db8131m_video_ops,
};

/*
 * db8131m_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int db8131m_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct db8131m_state *state = NULL;
	struct v4l2_subdev *sd = NULL;
	struct db8131m_platform_data *pdata = NULL;
	cam_dbg("E\n");

	state = kzalloc(sizeof(struct db8131m_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, DB8131M_DRIVER_NAME);

	state->initialized = 0;
	state->req_fps = state->set_fps = 8;
	state->sensor_mode = SENSOR_CAMERA;

	pdata = client->dev.platform_data;

	if (!pdata) {
		cam_err("no platform data\n");
		return -ENODEV;
	}

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &db8131m_ops);

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	if (!(pdata->default_width && pdata->default_height)) {
		state->preview_frmsizes.width = DEFAULT_PREVIEW_WIDTH;
		state->preview_frmsizes.height = DEFAULT_PREVIEW_HEIGHT;
	} else {
		state->preview_frmsizes.width = pdata->default_width;
		state->preview_frmsizes.height = pdata->default_height;
	}
	state->capture_frmsizes.width = DEFAULT_CAPTURE_WIDTH;
	state->capture_frmsizes.height = DEFAULT_CAPTURE_HEIGHT;

	cam_dbg("preview_width: %d , preview_height: %d, "
	"capture_width: %d, capture_height: %d",
	state->preview_frmsizes.width, state->preview_frmsizes.height,
	state->capture_frmsizes.width, state->capture_frmsizes.height);

	state->req_fmt.width = state->preview_frmsizes.width;
	state->req_fmt.height = state->preview_frmsizes.height;

	if (!pdata->pixelformat)
		state->req_fmt.pixelformat = VT_DEFAULT_FMT;
	else
		state->req_fmt.pixelformat = pdata->pixelformat;

	cam_dbg("probed!!\n");

	return 0;
}

static int db8131m_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct db8131m_state *state = to_state(sd);

	cam_dbg("E\n");

	state->initialized = 0;

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));

	return 0;
}

static const struct i2c_device_id db8131m_id[] = {
	{ DB8131M_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, db8131m_id);

static struct i2c_driver db8131m_i2c_driver = {
	.driver = {
		.name	= DB8131M_DRIVER_NAME,
	},
	.probe		= db8131m_probe,
	.remove		= db8131m_remove,
	.id_table	= db8131m_id,
};

static int __init db8131m_mod_init(void)
{
	cam_dbg("E\n");
	return i2c_add_driver(&db8131m_i2c_driver);
}

static void __exit db8131m_mod_exit(void)
{
	cam_dbg("E\n");
	i2c_del_driver(&db8131m_i2c_driver);
}
module_init(db8131m_mod_init);
module_exit(db8131m_mod_exit);

MODULE_DESCRIPTION("DB8131M CAM driver");
MODULE_AUTHOR(" ");
MODULE_LICENSE("GPL");
