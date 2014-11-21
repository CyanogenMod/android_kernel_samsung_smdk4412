/* drivers/media/video/s5k5ccgx.c
 *
 * Copyright (c) 2010, Samsung Electronics. All rights reserved
 * Author: dongseong.lim
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */
#include "s5k5ccgx.h"

static u32 dbg_level = CAMDBG_LEVEL_DEFAULT;

static const struct s5k5ccgx_fps s5k5ccgx_framerates[] = {
	{ I_FPS_0,	FRAME_RATE_AUTO },
	{ I_FPS_15,	FRAME_RATE_15 },
	{ I_FPS_25,	FRAME_RATE_25 },
	{ I_FPS_30,	FRAME_RATE_30 },
};

static const struct s5k5ccgx_framesize s5k5ccgx_preview_frmsizes[] = {
	{ PREVIEW_SZ_QCIF,	176,  144 },
	{ PREVIEW_SZ_QVGA,	320,  240 },
	{ PREVIEW_SZ_CIF,	352,  288 },
	{ PREVIEW_SZ_528x432,	528,  432 },
	{ PREVIEW_SZ_VGA,	640,  480 },
	{ PREVIEW_SZ_D1,	720,  480 },
	{ PREVIEW_SZ_SVGA,	800,  600 },
#ifdef CONFIG_MACH_P2
	{ PREVIEW_SZ_1024x552,	1024, 552 },
#else
	{ PREVIEW_SZ_1024x576,	1024, 576 },
#endif
	/*{ PREVIEW_SZ_1024x616,	1024, 616 },*/
	{ PREVIEW_SZ_XGA,	1024, 768 },
#ifdef CONFIG_MACH_PX
	{ PREVIEW_SZ_PVGA,	1280, 720 },
#endif
};

/* For video */
static const struct s5k5ccgx_framesize
s5k5ccgx_hidden_preview_frmsizes[] = {
	{ PREVIEW_SZ_PVGA,	1280, 720 },
};

static const struct s5k5ccgx_framesize s5k5ccgx_capture_frmsizes[] = {
	{ CAPTURE_SZ_VGA,	640,  480 },
#ifdef CONFIG_MACH_P2
	{ CAPTURE_SZ_W2MP,	2048, 1104 },
#else
	{ CAPTURE_SZ_W2MP,	2048, 1152 },
#endif
	{ CAPTURE_SZ_3MP,	2048, 1536 },
};

static struct s5k5ccgx_control s5k5ccgx_ctrls[] = {
	S5K5CCGX_INIT_CONTROL(V4L2_CID_CAMERA_FLASH_MODE, \
				FLASH_MODE_OFF),

	S5K5CCGX_INIT_CONTROL(V4L2_CID_CAMERA_BRIGHTNESS, \
				EV_DEFAULT),

	/* We set default value of metering to invalid value (METERING_BASE),
	 * to update sensor metering as a user set desired metering value. */
	S5K5CCGX_INIT_CONTROL(V4L2_CID_CAMERA_METERING, \
				METERING_BASE),

	S5K5CCGX_INIT_CONTROL(V4L2_CID_CAMERA_WHITE_BALANCE, \
				WHITE_BALANCE_AUTO),

	S5K5CCGX_INIT_CONTROL(V4L2_CID_CAMERA_EFFECT, \
				IMAGE_EFFECT_NONE),
};

static const struct s5k5ccgx_regs reg_datas = {
	.ev = {
		REGSET(GET_EV_INDEX(EV_MINUS_4), s5k5ccgx_brightness_m_4),
		REGSET(GET_EV_INDEX(EV_MINUS_3), s5k5ccgx_brightness_m_3),
		REGSET(GET_EV_INDEX(EV_MINUS_2), s5k5ccgx_brightness_m_2),
		REGSET(GET_EV_INDEX(EV_MINUS_1), s5k5ccgx_brightness_m_1),
		REGSET(GET_EV_INDEX(EV_DEFAULT), s5k5ccgx_brightness_0),
		REGSET(GET_EV_INDEX(EV_PLUS_1), s5k5ccgx_brightness_p_1),
		REGSET(GET_EV_INDEX(EV_PLUS_2), s5k5ccgx_brightness_p_2),
		REGSET(GET_EV_INDEX(EV_PLUS_3),s5k5ccgx_brightness_p_3),
		REGSET(GET_EV_INDEX(EV_PLUS_4), s5k5ccgx_brightness_p_4),
	},
	.metering = {
		REGSET(METERING_MATRIX, s5k5ccgx_metering_normal),
		REGSET(METERING_CENTER, s5k5ccgx_metering_center),
		REGSET(METERING_SPOT, s5k5ccgx_metering_spot),
	},
	.iso = {
		REGSET(ISO_AUTO, s5k5ccgx_iso_auto),
		REGSET(ISO_100, s5k5ccgx_iso_100),
		REGSET(ISO_200, s5k5ccgx_iso_200),
		REGSET(ISO_400, s5k5ccgx_iso_400),
	},
	.effect = {
		REGSET(IMAGE_EFFECT_NONE, s5k5ccgx_effect_off),
		REGSET(IMAGE_EFFECT_BNW, s5k5ccgx_effect_mono),
		REGSET(IMAGE_EFFECT_SEPIA, s5k5ccgx_effect_sepia),
		REGSET(IMAGE_EFFECT_NEGATIVE, s5k5ccgx_effect_negative),
	},
	.white_balance = {
		REGSET(WHITE_BALANCE_AUTO, s5k5ccgx_wb_auto),
		REGSET(WHITE_BALANCE_SUNNY, s5k5ccgx_wb_daylight),
		REGSET(WHITE_BALANCE_CLOUDY, s5k5ccgx_wb_cloudy),
		REGSET(WHITE_BALANCE_TUNGSTEN, s5k5ccgx_wb_incandescent),
		REGSET(WHITE_BALANCE_FLUORESCENT, s5k5ccgx_wb_fluorescent),
	},
	.scene_mode = {
		REGSET(SCENE_MODE_NONE, s5k5ccgx_scene_off),
		REGSET(SCENE_MODE_PORTRAIT, s5k5ccgx_scene_portrait),
		REGSET(SCENE_MODE_NIGHTSHOT, s5k5ccgx_scene_nightshot),
		REGSET(SCENE_MODE_LANDSCAPE, s5k5ccgx_scene_landscape),
		REGSET(SCENE_MODE_SPORTS, s5k5ccgx_scene_sports),
		REGSET(SCENE_MODE_PARTY_INDOOR, s5k5ccgx_scene_party),
		REGSET(SCENE_MODE_BEACH_SNOW, s5k5ccgx_scene_beach),
		REGSET(SCENE_MODE_SUNSET, s5k5ccgx_scene_sunset),
		REGSET(SCENE_MODE_DUSK_DAWN, s5k5ccgx_scene_dawn),
		REGSET(SCENE_MODE_FALL_COLOR, s5k5ccgx_scene_fall),
		REGSET(SCENE_MODE_TEXT, s5k5ccgx_scene_text),
		REGSET(SCENE_MODE_BACK_LIGHT, s5k5ccgx_scene_backlight),
		REGSET(SCENE_MODE_CANDLE_LIGHT, s5k5ccgx_scene_candle),
	},
	.saturation = {
		REGSET(SATURATION_MINUS_2, s5k5ccgx_saturation_m_2),
		REGSET(SATURATION_MINUS_1, s5k5ccgx_saturation_m_1),
		REGSET(SATURATION_DEFAULT, s5k5ccgx_saturation_0),
		REGSET(SATURATION_PLUS_1, s5k5ccgx_saturation_p_1),
		REGSET(SATURATION_PLUS_2, s5k5ccgx_saturation_p_2),
	},
	.contrast = {
		REGSET(CONTRAST_MINUS_2, s5k5ccgx_contrast_m_2),
		REGSET(CONTRAST_MINUS_1, s5k5ccgx_contrast_m_1),
		REGSET(CONTRAST_DEFAULT, s5k5ccgx_contrast_0),
		REGSET(CONTRAST_PLUS_1, s5k5ccgx_contrast_p_1),
		REGSET(CONTRAST_PLUS_2, s5k5ccgx_contrast_p_2),

	},
	.sharpness = {
		REGSET(SHARPNESS_MINUS_2, s5k5ccgx_sharpness_m_2),
		REGSET(SHARPNESS_MINUS_1, s5k5ccgx_sharpness_m_1),
		REGSET(SHARPNESS_DEFAULT, s5k5ccgx_sharpness_0),
		REGSET(SHARPNESS_PLUS_1, s5k5ccgx_sharpness_p_1),
		REGSET(SHARPNESS_PLUS_2, s5k5ccgx_sharpness_p_2),
	},
	.fps = {
		REGSET(I_FPS_0, s5k5ccgx_fps_auto),
		REGSET(I_FPS_15, s5k5ccgx_fps_15fix),
		REGSET(I_FPS_25, s5k5ccgx_fps_25fix),
		REGSET(I_FPS_30, s5k5ccgx_fps_30fix),
	},
	.preview_size = {
		REGSET(PREVIEW_SZ_QCIF, s5k5ccgx_176_144_Preview),
		REGSET(PREVIEW_SZ_QVGA, s5k5ccgx_320_240_Preview),
		REGSET(PREVIEW_SZ_CIF, s5k5ccgx_352_288_Preview),
		REGSET(PREVIEW_SZ_528x432, s5k5ccgx_528_432_Preview),
		REGSET(PREVIEW_SZ_VGA, s5k5ccgx_640_480_Preview),
		REGSET(PREVIEW_SZ_D1, s5k5ccgx_720_480_Preview),
		REGSET(PREVIEW_SZ_SVGA, s5k5ccgx_800_600_Preview),
		REGSET(PREVIEW_WIDE_SIZE, S5K5CCGX_WIDE_PREVIEW_REG),
		REGSET(PREVIEW_SZ_XGA, s5k5ccgx_1024_768_Preview),
	},

	/* Flash*/
	.flash_start = REGSET_TABLE(s5k5ccgx_mainflash_start),
	.flash_end = REGSET_TABLE(s5k5ccgx_mainflash_end),
	.af_pre_flash_start = REGSET_TABLE(s5k5ccgx_preflash_start),
	.af_pre_flash_end = REGSET_TABLE(s5k5ccgx_preflash_end),
	.flash_ae_set = REGSET_TABLE(s5k5ccgx_flash_ae_set),
	.flash_ae_clear = REGSET_TABLE(s5k5ccgx_flash_ae_clear),

	/* AWB, AE Lock */
	.ae_lock_on = REGSET_TABLE(s5k5ccgx_ae_lock),
	.ae_lock_off = REGSET_TABLE(s5k5ccgx_ae_unlock),
	.awb_lock_on = REGSET_TABLE(s5k5ccgx_awb_lock),
	.awb_lock_off = REGSET_TABLE(s5k5ccgx_awb_unlock),

	.restore_cap = REGSET_TABLE(s5k5ccgx_restore_capture_reg),
	.change_wide_cap = REGSET_TABLE(s5k5ccgx_change_wide_cap),
#ifdef CONFIG_MACH_P8
	.set_lowlight_cap = REGSET_TABLE(s5k5ccgx_set_lowlight_reg),
#endif

	/* AF */
	.af_macro_mode = REGSET_TABLE(s5k5ccgx_af_macro_on),
	.af_normal_mode = REGSET_TABLE(s5k5ccgx_af_normal_on),
#if !defined(CONFIG_MACH_P2)
	.af_night_normal_mode = REGSET_TABLE(s5k5ccgx_af_night_normal_on),
#endif
	.af_off = REGSET_TABLE(s5k5ccgx_af_off_reg),
	.hd_af_start = REGSET_TABLE(s5k5ccgx_720P_af_do),
	.hd_first_af_start = REGSET_TABLE(s5k5ccgx_1st_720P_af_do),
	.single_af_start = REGSET_TABLE(s5k5ccgx_af_do),

	.init = REGSET_TABLE(s5k5ccgx_init_reg),
	.get_esd_status = REGSET_TABLE(s5k5ccgx_get_esd_reg),
	.stream_stop = REGSET_TABLE(s5k5ccgx_stream_stop_reg),
	.get_light_level = REGSET_TABLE(s5k5ccgx_get_light_status),
	.get_iso = REGSET_TABLE(s5k5ccgx_get_iso_reg),
	.get_ae_stable = REGSET_TABLE(s5k5ccgx_get_ae_stable_reg),
	.get_shutterspeed = REGSET_TABLE(s5k5ccgx_get_shutterspeed_reg),

	.preview_mode = REGSET_TABLE(s5k5ccgx_update_preview_reg),
	.preview_hd_mode = REGSET_TABLE(s5k5ccgx_update_hd_preview_reg),
	.return_preview_mode = REGSET_TABLE(s5k5ccgx_preview_return),
	.capture_mode = {
		REGSET(CAPTURE_SZ_VGA, s5k5ccgx_snapshot_vga),
		REGSET(CAPTURE_SZ_W2MP, s5k5ccgx_snapshot),
		REGSET(CAPTURE_SZ_3MP, s5k5ccgx_snapshot),
	},
#ifdef CONFIG_MACH_P8
	.antibanding = REGSET_TABLE(S5K5CCGX_ANTIBANDING_REG),
#endif
};

static const struct v4l2_mbus_framefmt capture_fmts[] = {
	{
		.code		= V4L2_MBUS_FMT_FIXED,
		.colorspace	= V4L2_COLORSPACE_JPEG,
	},
};

/**
 * msleep_debug: wrapper function calling proper sleep()
 * @msecs: time to be sleep (in milli-seconds unit)
 * @dbg_on: whether enable log or not.
 */
static void msleep_debug(u32 msecs, bool dbg_on)
{
	u32 delta_halfrange; /* in us unit */

	if (unlikely(!msecs))
		return;

	if (dbg_on)
		cam_dbg("delay for %dms\n", msecs);

	if (msecs <= 7)
		delta_halfrange = 100;
	else
		delta_halfrange = 300;

	if (msecs <= 20)
		usleep_range((msecs * 1000 - delta_halfrange),
			(msecs * 1000 + delta_halfrange));
	else
		msleep(msecs);
}

#if CONFIG_LOAD_FILE
static int loadFile(void)
{
	struct file *fp = NULL;
	struct test *nextBuf = NULL;

	u8 *nBuf = NULL;
	size_t file_size = 0, max_size = 0, testBuf_size = 0;
	ssize_t nread = 0;
	s32 check = 0, starCheck = 0;
	s32 tmp_large_file = 0;
	s32 i = 0;
	int ret = 0;
	loff_t pos;

	mm_segment_t fs = get_fs();
	set_fs(get_ds());

	cam_info("%s: E\n", __func__);

	BUG_ON(testBuf);

	fp = filp_open(TUNING_FILE_PATH, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		cam_err("file open error\n");
		return PTR_ERR(fp);
	}

	file_size = (size_t) fp->f_path.dentry->d_inode->i_size;
	max_size = file_size;

	cam_dbg("file_size = %d\n", file_size);

	nBuf = kmalloc(file_size, GFP_KERNEL);
	if (nBuf == NULL) {
		cam_dbg("Fail to 1st get memory\n");
		nBuf = vmalloc(file_size);
		if (nBuf == NULL) {
			cam_err("ERR: nBuf Out of Memory\n");
			ret = -ENOMEM;
			goto error_out;
		}
		tmp_large_file = 1;
	}

	testBuf_size = sizeof(struct test) * file_size;
	if (tmp_large_file) {
		testBuf = (struct test *)vmalloc(testBuf_size);
		large_file = 1;
	} else {
		testBuf = kmalloc(testBuf_size, GFP_ATOMIC);
		if (testBuf == NULL) {
			cam_dbg("Fail to get mem(%d bytes)\n", testBuf_size);
			testBuf = (struct test *)vmalloc(testBuf_size);
			large_file = 1;
		}
	}
	if (testBuf == NULL) {
		cam_err("ERR: Out of Memory\n");
		ret = -ENOMEM;
		goto error_out;
	}

	pos = 0;
	memset(nBuf, 0, file_size);
	memset(testBuf, 0, file_size * sizeof(struct test));

	nread = vfs_read(fp, (char __user *)nBuf, file_size, &pos);
	if (nread != file_size) {
		cam_err("failed to read file ret = %d\n", nread);
		ret = -1;
		goto error_out;
	}

	set_fs(fs);

	i = max_size;

	printk(KERN_DEBUG "i = %d\n", i);

	while (i) {
		testBuf[max_size - i].data = *nBuf;
		if (i != 1) {
			testBuf[max_size - i].nextBuf =
				&testBuf[max_size - i + 1];
		} else {
			testBuf[max_size - i].nextBuf = NULL;
			break;
		}
		i--;
		nBuf++;
	}

	i = max_size;
	nextBuf = &testBuf[0];

#if 1
	while (i - 1) {
		if (!check && !starCheck) {
			if (testBuf[max_size - i].data == '/') {
				if (testBuf[max_size-i].nextBuf != NULL) {
					if (testBuf[max_size-i].nextBuf->data
								== '/') {
						check = 1;/* when find '//' */
						i--;
					} else if (
					    testBuf[max_size-i].nextBuf->data
					    == '*') {
						/* when find '/ *' */
						starCheck = 1;
						i--;
					}
				} else
					break;
			}
			if (!check && !starCheck) {
				/* ignore '\t' */
				if (testBuf[max_size - i].data != '\t') {
					nextBuf->nextBuf = &testBuf[max_size-i];
					nextBuf = &testBuf[max_size - i];
				}
			}
		} else if (check && !starCheck) {
			if (testBuf[max_size - i].data == '/') {
				if (testBuf[max_size-i].nextBuf != NULL) {
					if (testBuf[max_size-i].nextBuf->data
					    == '*') {
						/* when find '/ *' */
						starCheck = 1;
						check = 0;
						i--;
					}
				} else
					break;
			}

			 /* when find '\n' */
			if (testBuf[max_size - i].data == '\n' && check) {
				check = 0;
				nextBuf->nextBuf = &testBuf[max_size - i];
				nextBuf = &testBuf[max_size - i];
			}

		} else if (!check && starCheck) {
			if (testBuf[max_size - i].data == '*') {
				if (testBuf[max_size-i].nextBuf != NULL) {
					if (testBuf[max_size-i].nextBuf->data
					    == '/') {
						/* when find '* /' */
						starCheck = 0;
						i--;
					}
				} else
					break;
			}
		}

		i--;

		if (i < 2) {
			nextBuf = NULL;
			break;
		}

		if (testBuf[max_size - i].nextBuf == NULL) {
			nextBuf = NULL;
			break;
		}
	}
#endif

#if 0 /* for print */
	cam_dbg("i = %d\n", i);
	nextBuf = &testBuf[0];
	while (1) {
		if (nextBuf->nextBuf == NULL)
			break;
		cam_dbg("%c", nextBuf->data);
		nextBuf = nextBuf->nextBuf;
	}
#endif

error_out:
	tmp_large_file ? vfree(nBuf) : kfree(nBuf);

	if (fp)
		filp_close(fp, current->files);
	return ret;
}


#endif

/**
 * s5k5ccgx_i2c_read_twobyte: Read 2 bytes from sensor
 */
static int s5k5ccgx_i2c_read_twobyte(struct i2c_client *client,
				  u16 subaddr, u16 *data)
{
	int err;
	u8 buf[2];
	struct i2c_msg msg[2];

	cpu_to_be16s(&subaddr);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = (u8 *)&subaddr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = buf;

	err = i2c_transfer(client->adapter, msg, 2);
	CHECK_ERR_COND_MSG(err != 2, -EIO, "fail to read register\n");

	*data = ((buf[0] << 8) | buf[1]);

	return 0;
}

/**
 * s5k5ccgx_i2c_write_twobyte: Write (I2C) multiple bytes to the camera sensor
 * @client: pointer to i2c_client
 * @cmd: command register
 * @w_data: data to be written
 * @w_len: length of data to be written
 *
 * Returns 0 on success, <0 on error
 */
static int s5k5ccgx_i2c_write_twobyte(struct i2c_client *client,
					 u16 addr, u16 w_data)
{
	int retry_count = 5;
	int ret = 0;
	u8 buf[4] = {0,};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 4,
		.buf	= buf,
	};

	buf[0] = addr >> 8;
	buf[1] = addr;
	buf[2] = w_data >> 8;
	buf[3] = w_data & 0xff;

#if (0)
	s5k5ccgx_debug(S5K5CCGX_DEBUG_I2C, "%s : W(0x%02X%02X%02X%02X)\n",
		__func__, buf[0], buf[1], buf[2], buf[3]);
#else
	/* cam_dbg("I2C writing: 0x%02X%02X%02X%02X\n",
			buf[0], buf[1], buf[2], buf[3]); */
#endif

	do {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (likely(ret == 1))
			break;
		msleep_debug(POLL_TIME_MS, false);
		cam_err("%s: error(%d), write (%04X, %04X), retry %d.\n",
				__func__, ret, addr, w_data, retry_count);
	} while (retry_count-- > 0);

	CHECK_ERR_COND_MSG(ret != 1, -EIO, "I2C does not working.\n\n");

	return 0;
}

#if CONFIG_LOAD_FILE
static int s5k5ccgx_write_regs_from_sd(struct v4l2_subdev *sd,
						const u8 s_name[])
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct test *tempData = NULL;

	int ret = -EAGAIN;
	u32 temp;
	u32 delay = 0;
	u8 data[11];
	s32 searched = 0;
	size_t size = strlen(s_name);
	s32 i;
#if DEBUG_WRITE_REGS
	u8 regs_name[128] = {0,};

	BUG_ON(size > sizeof(regs_name));
#endif

	cam_dbg("%s: E size = %d, string = %s\n", __func__, size, s_name);
	tempData = &testBuf[0];
	while (!searched) {
		searched = 1;
		for (i = 0; i < size; i++) {
			if (tempData->data != s_name[i]) {
				searched = 0;
				break;
			}
#if DEBUG_WRITE_REGS
			regs_name[i] = tempData->data;
#endif
			tempData = tempData->nextBuf;
		}
#if DEBUG_WRITE_REGS
		if (i > 9) {
			regs_name[i] = '\0';
			cam_dbg("Searching: regs_name = %s\n", regs_name);
		}
#endif
		tempData = tempData->nextBuf;
	}
	/* structure is get..*/
#if DEBUG_WRITE_REGS
	regs_name[i] = '\0';
	cam_dbg("Searched regs_name = %s\n\n", regs_name);
#endif

	while (1) {
		if (tempData->data == '{')
			break;
		else
			tempData = tempData->nextBuf;
	}

	while (1) {
		searched = 0;
		while (1) {
			if (tempData->data == 'x') {
				/* get 10 strings.*/
				data[0] = '0';
				for (i = 1; i < 11; i++) {
					data[i] = tempData->data;
					tempData = tempData->nextBuf;
				}
				/*cam_dbg("%s\n", data);*/
				temp = simple_strtoul(data, NULL, 16);
				break;
			} else if (tempData->data == '}') {
				searched = 1;
				break;
			} else
				tempData = tempData->nextBuf;

			if (tempData->nextBuf == NULL)
				return -1;
		}

		if (searched)
			break;

		if ((temp & S5K5CCGX_DELAY) == S5K5CCGX_DELAY) {
			delay = temp & 0x0FFFF;
			msleep_debug(delay, true);
			continue;
		}

		/* cam_dbg("I2C writing: 0x%08X,\n",temp);*/
		ret = s5k5ccgx_i2c_write_twobyte(client,
			(temp >> 16), (u16)temp);

		/* In error circumstances */
		/* Give second shot */
		if (unlikely(ret)) {
			dev_info(&client->dev,
					"s5k5ccgx i2c retry one more time\n");
			ret = s5k5ccgx_i2c_write_twobyte(client,
				(temp >> 16), (u16)temp);

			/* Give it one more shot */
			if (unlikely(ret)) {
				dev_info(&client->dev,
						"s5k5ccgx i2c retry twice\n");
				ret = s5k5ccgx_i2c_write_twobyte(client,
					(temp >> 16), (u16)temp);
			}
		}
	}

	return ret;
}
#endif

/* Write register
 * If success, return value: 0
 * If fail, return value: -EIO
 */
static int s5k5ccgx_write_regs(struct v4l2_subdev *sd, const u32 regs[],
			     int size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 delay = 0;
	int i, err = 0;

	for (i = 0; i < size; i++) {
		if ((regs[i] & S5K5CCGX_DELAY) == S5K5CCGX_DELAY) {
			delay = regs[i] & 0xFFFF;
			msleep_debug(delay, true);
			continue;
		}

		err = s5k5ccgx_i2c_write_twobyte(client,
			(regs[i] >> 16), regs[i]);
		CHECK_ERR_MSG(err, "write registers\n")
	}

	return 0;
}

#define BURST_MODE_BUFFER_MAX_SIZE 2700
u8 s5k5ccgx_burstmode_buf[BURST_MODE_BUFFER_MAX_SIZE];

/* PX: */
static int s5k5ccgx_burst_write_regs(struct v4l2_subdev *sd,
			const u32 list[], u32 size, char *name)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL;
	int i = 0, idx = 0;
	u16 subaddr = 0, next_subaddr = 0, value = 0;
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 0,
		.buf	= s5k5ccgx_burstmode_buf,
	};

	cam_trace("E\n");

	for (i = 0; i < size; i++) {
		CHECK_ERR_COND_MSG((idx > (BURST_MODE_BUFFER_MAX_SIZE - 10)),
			err, "BURST MOD buffer overflow!\n")

		subaddr = (list[i] & 0xFFFF0000) >> 16;
		if (subaddr == 0x0F12)
			next_subaddr = (list[i+1] & 0xFFFF0000) >> 16;

		value = list[i] & 0x0000FFFF;

		switch (subaddr) {
		case 0x0F12:
			/* make and fill buffer for burst mode write. */
			if (idx == 0) {
				s5k5ccgx_burstmode_buf[idx++] = 0x0F;
				s5k5ccgx_burstmode_buf[idx++] = 0x12;
			}
			s5k5ccgx_burstmode_buf[idx++] = value >> 8;
			s5k5ccgx_burstmode_buf[idx++] = value & 0xFF;

			/* write in burstmode*/
			if (next_subaddr != 0x0F12) {
				msg.len = idx;
				err = i2c_transfer(client->adapter,
					&msg, 1) == 1 ? 0 : -EIO;
				CHECK_ERR_MSG(err, "i2c_transfer\n");
				/* cam_dbg("s5k5ccgx_sensor_burst_write,
						idx = %d\n", idx); */
				idx = 0;
			}
			break;

		case 0xFFFF:
			msleep_debug(value, true);
			break;

		default:
			idx = 0;
			err = s5k5ccgx_i2c_write_twobyte(client,
						subaddr, value);
			CHECK_ERR_MSG(err, "i2c_write_twobytes\n");
			break;
		}
	}

	return 0;
}

/* PX: */
static int s5k5ccgx_set_from_table(struct v4l2_subdev *sd,
				const char *setting_name,
				const struct regset_table *table,
				u32 table_size, s32 index)
{
	int err = 0;

	/* cam_dbg("%s: set %s index %d\n",
		__func__, setting_name, index); */
	CHECK_ERR_COND_MSG(((index < 0) || (index >= table_size)),
		-EINVAL, "index(%d) out of range[0:%d] for table for %s\n",
		index, table_size, setting_name);

	table += index;

#if CONFIG_LOAD_FILE
	CHECK_ERR_COND_MSG(!table->name, -EFAULT, \
		"table=%s, index=%d, reg_name = NULL\n", setting_name, index);

	cam_dbg("%s: \"%s\", reg_name=%s\n", __func__,
			setting_name, table->name);
	return s5k5ccgx_write_regs_from_sd(sd, table->name);

#else /* !CONFIG_LOAD_FILE */
	CHECK_ERR_COND_MSG(!table->reg, -EFAULT, \
		"table=%s, index=%d, reg = NULL\n", setting_name, index);

# if DEBUG_WRITE_REGS
	cam_dbg("%s: \"%s\", reg_name=%s\n", __func__,
			setting_name, table->name);
# endif /* DEBUG_WRITE_REGS */

	err = s5k5ccgx_write_regs(sd, table->reg, table->array_size);
	CHECK_ERR_MSG(err, "write regs(%s), err=%d\n", setting_name, err);

	return 0;
#endif /* CONFIG_LOAD_FILE */
}

/* PX: */
static inline int s5k5ccgx_save_ctrl(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int i, ctrl_cnt = ARRAY_SIZE(s5k5ccgx_ctrls);

	cam_trace("EX, Ctrl-ID = 0x%X (%d)", ctrl->id,
			ctrl->id - V4L2_CID_PRIVATE_BASE);

	for (i = 0; i < ctrl_cnt; i++) {
		if (ctrl->id == s5k5ccgx_ctrls[i].id) {
			s5k5ccgx_ctrls[i].value = ctrl->value;
			return 0;
		}
	}

	cam_trace("not saved, ID 0x%X (%d)\n", ctrl->id,
			ctrl->id - V4L2_CID_PRIVATE_BASE);
	return 0;
}

static inline int s5k5ccgx_power(struct v4l2_subdev *sd, int on)
{
	struct s5k5ccgx_platform_data *pdata = to_state(sd)->pdata;
	int err = 0;

	cam_trace("EX\n");

	if (pdata->common.power) {
		err = pdata->common.power(on);
		CHECK_ERR_MSG(err, "failed to power(%d)\n", on);
	}
	
	return 0;
}

static inline int __used
s5k5ccgx_transit_preview_mode(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	if (state->hd_videomode)
		return 0;

	if (state->runmode == RUNMODE_CAPTURING_STOP) {
#ifndef CONFIG_MACH_PX
		if (SCENE_MODE_NIGHTSHOT == state->scene_mode) {
			state->pdata->common.streamoff_delay = 
				S5K5CCGX_STREAMOFF_DELAY;
		}
#endif
		err = s5k5ccgx_set_from_table(sd, "return_preview_mode",
				&state->regs->return_preview_mode, 1, 0);
	} else {
		err = s5k5ccgx_set_from_table(sd, "preview_mode",
				&state->regs->preview_mode, 1, 0);
	}
	CHECK_ERR(err);

	return 0;
}

/**
 * s5k5ccgx_transit_half_mode: go to a half-release mode
 *
 * Don't forget that half mode is not used in movie mode.
 */
static inline int __used
s5k5ccgx_transit_half_mode(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);

	/* We do not go to half release mode in movie mode */
	if (state->sensor_mode == SENSOR_MOVIE)
		return 0;

	/*
	 * Write the additional codes here
	 */
	return 0;
}

static inline int __used
s5k5ccgx_transit_capture_mode(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);

	return s5k5ccgx_set_from_table(sd, "capture_mode",
			state->regs->capture_mode,
			ARRAY_SIZE(state->regs->capture_mode),
			state->capture.frmsize->index);
}

/**
 * s5k5ccgx_transit_movie_mode: switch camera mode if needed.
 *
 * Note that this fuction should be called from start_preview().
 */
static inline int __used
s5k5ccgx_transit_movie_mode(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	/* we'll go from the below modes to RUNNING or RECORDING */
	switch (state->runmode) {
	case RUNMODE_INIT:
		/* case of entering camcorder firstly */
		if (state->hd_videomode) {
			cam_dbg("switching to camcorder mode\n");
			s5k5ccgx_restore_parameter(sd);
			err = s5k5ccgx_set_from_table(sd, "preview_hd_mode",
					&state->regs->preview_hd_mode, 1, 0);
			CHECK_ERR_MSG(err, "failed to update HD preview\n");

			if (IS_AF_SUPPORTED())
				s5k5ccgx_set_from_table(sd, "hd_first_af_start",
					&state->regs->hd_first_af_start, 1, 0);
		}
		break;

	case RUNMODE_RUNNING_STOP:
		/* case of switching from camera to camcorder */
		break;

	case RUNMODE_RECORDING_STOP:
		/* case of switching from camcorder to camera */
		if (SENSOR_CAMERA == state->sensor_mode) {
			cam_dbg("switching to camera mode\n");
			cam_err("camcorder-off not implemented\n");
		}
		break;

	default:
		break;
	}

	return 0;
}

/**
 * s5k5ccgx_is_hwflash_on - check whether flash device is on
 *
 * Refer to state->flash.on to check whether flash is in use in driver.
 */
static inline int s5k5ccgx_is_hwflash_on(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	struct s5k5ccgx_platform_data *pdata = state->pdata;
	int err = 0;

	if (!IS_FLASH_SUPPORTED())
		return 0;

	if (pdata->is_flash_on)
		err = pdata->is_flash_on();
		
	return err;
}

/**
 * s5k5ccgx_flash_en - contro Flash LED
 * @mode: S5K5CCGX_FLASH_MODE_NORMAL or S5K5CCGX_FLASH_MODE_MOVIE
 * @onoff: S5K5CCGX_FLASH_ON or S5K5CCGX_FLASH_OFF
 */
static int s5k5ccgx_flash_en(struct v4l2_subdev *sd, s32 mode, s32 onoff)
{
	struct s5k5ccgx_state *state = to_state(sd);
	struct s5k5ccgx_platform_data *pdata = state->pdata;
	int err = 0;

	if (!IS_FLASH_SUPPORTED() || state->flash.ignore_flash) {
		cam_info("flash_en: ignore %d\n", state->flash.ignore_flash);
		return 0;
	}

	if (pdata->common.flash_en)
		err = pdata->common.flash_en(mode, onoff);
		
	return err;
}

/**
 * s5k5ccgx_flash_torch - turn flash on/off as torch for preflash, recording
 * @onoff: S5K5CCGX_FLASH_ON or S5K5CCGX_FLASH_OFF
 *
 * This func set state->flash.on properly.
 */
static inline int s5k5ccgx_flash_torch(struct v4l2_subdev *sd, s32 onoff)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	if (!IS_FLASH_SUPPORTED())
		return 0;

	/* On/Off flash */
	err = s5k5ccgx_flash_en(sd, S5K5CCGX_FLASH_MODE_MOVIE, onoff);
	if (unlikely(!err))
		state->flash.on = (onoff == S5K5CCGX_FLASH_ON) ? 1 : 0;

	return err;
}

/**
 * s5k5ccgx_flash_oneshot - turn main flash on for capture
 * @onoff: S5K5CCGX_FLASH_ON or S5K5CCGX_FLASH_OFF
 *
 * Main flash is turn off automatically in some milliseconds.
 */
static inline int s5k5ccgx_flash_oneshot(struct v4l2_subdev *sd, s32 onoff)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	if (!IS_FLASH_SUPPORTED())
		return 0;

	/* On/Off flash */
	err = s5k5ccgx_flash_en(sd, S5K5CCGX_FLASH_MODE_NORMAL, onoff);
	if (!err) {
		/* The flash_on here is only used for EXIF */
		state->flash.on = (onoff == S5K5CCGX_FLASH_ON) ? 1 : 0;
	}

	return err;
}

/* PX: Set scene mode */
static int s5k5ccgx_set_scene_mode(struct v4l2_subdev *sd, s32 val)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -ENODEV;

	cam_trace("E, value %d\n", val);

	if (state->scene_mode == val)
		return 0;

	/* when scene mode is switched, we frist have to reset */
	if (state->scene_mode != SCENE_MODE_NONE) {
		err = s5k5ccgx_set_from_table(sd, "scene_mode",
				state->regs->scene_mode,
				ARRAY_SIZE(state->regs->scene_mode),
				SCENE_MODE_NONE);
	}

retry:
	switch (val) {
	case SCENE_MODE_NONE:
		if (state->scene_mode != SCENE_MODE_NONE)
			break;
		/* do not break */
	case SCENE_MODE_PORTRAIT:
	case SCENE_MODE_NIGHTSHOT:
	case SCENE_MODE_BACK_LIGHT:
	case SCENE_MODE_LANDSCAPE:
	case SCENE_MODE_SPORTS:
	case SCENE_MODE_PARTY_INDOOR:
	case SCENE_MODE_BEACH_SNOW:
	case SCENE_MODE_SUNSET:
	case SCENE_MODE_DUSK_DAWN:
	case SCENE_MODE_FALL_COLOR:
	case SCENE_MODE_TEXT:
	case SCENE_MODE_CANDLE_LIGHT:
		s5k5ccgx_set_from_table(sd, "scene_mode",
			state->regs->scene_mode,
			ARRAY_SIZE(state->regs->scene_mode), val);
		break;

	default:
		cam_err("set_scene: error, not supported (%d)\n", val);
		val = SCENE_MODE_NONE;
		goto retry;
	}

	state->scene_mode = val;

	cam_trace("X\n");
	return 0;
}

/* PX: Set brightness */
static int s5k5ccgx_set_exposure(struct v4l2_subdev *sd, s32 val)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -EINVAL;

	if ((val < EV_MINUS_4) || (val > EV_PLUS_4)) {
		cam_err("%s: error, invalid value(%d)\n", __func__, val);
		return -EINVAL;
	}

	err = s5k5ccgx_set_from_table(sd, "brightness", state->regs->ev,
		ARRAY_SIZE(state->regs->ev), GET_EV_INDEX(val));

	state->exposure.val = val;

	return err;
}

/**
 * s5k5ccgx_set_whitebalance - set whilebalance
 * @val:
 */
static int s5k5ccgx_set_whitebalance(struct v4l2_subdev *sd, s32 val)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	cam_trace("E, value %d\n", val);

retry:
	switch (val) {
	case WHITE_BALANCE_AUTO:
	case WHITE_BALANCE_SUNNY:
	case WHITE_BALANCE_CLOUDY:
	case WHITE_BALANCE_TUNGSTEN:
	case WHITE_BALANCE_FLUORESCENT:
		err = s5k5ccgx_set_from_table(sd, "white balance",
				state->regs->white_balance,
				ARRAY_SIZE(state->regs->white_balance),
				val);
		break;

	default:
		cam_err("set_wb: error, not supported (%d)\n", val);
		val = WHITE_BALANCE_AUTO;
		goto retry;
	}

	state->wb.mode = val;

	cam_trace("X\n");
	return err;
}

/* PX: Check light level */
static u32 s5k5ccgx_get_light_level(struct v4l2_subdev *sd, u32 *light_level)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k5ccgx_state *state = to_state(sd);
	u16 val_lsb = 0;
	u16 val_msb = 0;
	int err = -ENODEV;

	err = s5k5ccgx_set_from_table(sd, "get_light_level",
			&state->regs->get_light_level, 1, 0);
	CHECK_ERR_MSG(err, "fail to get light level\n");

	err = s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &val_lsb);
	err = s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &val_msb);
	CHECK_ERR_MSG(err, "fail to read light level\n");

	*light_level = val_lsb | (val_msb<<16);

	/* cam_trace("X, light level = 0x%X", *light_level); */

	return 0;
}

/* PX: Set sensor mode */
static int s5k5ccgx_set_sensor_mode(struct v4l2_subdev *sd, s32 val)
{
	struct s5k5ccgx_state *state = to_state(sd);

	cam_dbg("sensor_mode: %d\n", val);

#ifdef CONFIG_MACH_PX
	state->hd_videomode = 0;
#endif

	switch (val) {
	case SENSOR_MOVIE:
		/* We does not support movie mode when in VT. */
		if (state->vt_mode) {
			state->sensor_mode = SENSOR_CAMERA;
			cam_err("%s: error, not support movie\n", __func__);
			break;
		}
		/* We do not break. */

	case SENSOR_CAMERA:
		state->sensor_mode = val;
		break;

#ifdef CONFIG_MACH_PX
	case 2:	/* 720p HD video mode */
		state->sensor_mode = SENSOR_MOVIE;
		state->hd_videomode = 1;
		break;
#endif

	default:
		cam_err("%s: error, not support.(%d)\n", __func__, val);
		state->sensor_mode = SENSOR_CAMERA;
		WARN_ON(1);
		break;
	}

	return 0;
}

/* PX: Set framerate */
static int s5k5ccgx_set_frame_rate(struct v4l2_subdev *sd, s32 fps)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -EIO;
	int i = 0, fps_index = -1;

	if (!state->initialized || (fps < 0) || state->hd_videomode
	    || (state->scene_mode != SCENE_MODE_NONE))
		return 0;

	cam_info("set frame rate %d\n", fps);

	for (i = 0; i < ARRAY_SIZE(s5k5ccgx_framerates); i++) {
		if (fps == s5k5ccgx_framerates[i].fps) {
			fps_index = s5k5ccgx_framerates[i].index;
			state->fps = fps;
			state->req_fps = -1;
			break;
		}
	}

	if (unlikely(fps_index < 0)) {
		cam_err("set_frame_rate: not supported fps(%d)\n", fps);
		return 0;
	}

	err = s5k5ccgx_set_from_table(sd, "fps", state->regs->fps,
			ARRAY_SIZE(state->regs->fps), fps_index);
	CHECK_ERR_MSG(err, "fail to set framerate\n");

	return 0;
}

static int s5k5ccgx_set_ae_lock(struct v4l2_subdev *sd, s32 val, bool force)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	switch (val) {
	case AE_LOCK:
		if (state->focus.touch)
			return 0;

		err = s5k5ccgx_set_from_table(sd, "ae_lock_on",
				&state->regs->ae_lock_on, 1, 0);
		WARN_ON(state->exposure.ae_lock);
		state->exposure.ae_lock = 1;
		break;

	case AE_UNLOCK:
		if (unlikely(!force && !state->exposure.ae_lock))
			return 0;

		err = s5k5ccgx_set_from_table(sd, "ae_lock_off",
				&state->regs->ae_lock_off, 1, 0);
		state->exposure.ae_lock = 0;
		break;

	default:
		cam_err("%s: WARNING, invalid argument(%d)\n", __func__, val);
	}

	CHECK_ERR_MSG(err, "fail to lock AE(%d), err=%d\n", val, err);

	return 0;
}

static int s5k5ccgx_set_awb_lock(struct v4l2_subdev *sd, s32 val, bool force)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	switch (val) {
	case AWB_LOCK:
		if (state->flash.on ||
		    (state->wb.mode != WHITE_BALANCE_AUTO))
			return 0;

		err = s5k5ccgx_set_from_table(sd, "awb_lock_on",
				&state->regs->awb_lock_on, 1, 0);
		WARN_ON(state->wb.awb_lock);
		state->wb.awb_lock = 1;
		break;

	case AWB_UNLOCK:
		if (unlikely(!force && !state->wb.awb_lock))
			return 0;

		err = s5k5ccgx_set_from_table(sd, "awb_lock_off",
			&state->regs->awb_lock_off, 1, 0);
		state->wb.awb_lock = 0;
		break;

	default:
		cam_err("%s: WARNING, invalid argument(%d)\n", __func__, val);
	}

	CHECK_ERR_MSG(err, "fail to lock AWB(%d), err=%d\n", val, err);

	return 0;
}

/* PX: Set AE, AWB Lock */
static int s5k5ccgx_set_lock(struct v4l2_subdev *sd, s32 lock, bool force)
{
	int err = -EIO;

	cam_trace("%s\n", lock ? "on" : "off");
	if (unlikely((u32)lock >= AEAWB_LOCK_MAX)) {
		cam_err("%s: error, invalid argument\n", __func__);
		return -EINVAL;
	}

	err = s5k5ccgx_set_ae_lock(sd, (lock == AEAWB_LOCK) ?
				AE_LOCK : AE_UNLOCK, force);
	if (unlikely(err))
		goto out_err;

	err = s5k5ccgx_set_awb_lock(sd, (lock == AEAWB_LOCK) ?
				AWB_LOCK : AWB_UNLOCK, force);
	if (unlikely(err))
		goto out_err;

	cam_trace("X\n");
	return 0;

out_err:
	cam_err("%s: error, failed to set lock\n", __func__);
	return err;
}


static int s5k5ccgx_set_af_softlanding(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -EINVAL;

	cam_trace("E\n");

	if (!state->initialized || !IS_AF_SUPPORTED())
		return 0;

	err = s5k5ccgx_set_from_table(sd, "af_off",
			&state->regs->af_off, 1, 0);
	CHECK_ERR_MSG(err, "fail to set softlanding\n");

	cam_trace("X\n");
	return 0;
}

static int s5k5ccgx_return_focus(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -EINVAL;

	if (!IS_AF_SUPPORTED())
		return 0;

	cam_trace("E\n");

	switch (state->focus.mode) {
	case FOCUS_MODE_MACRO:
		err = s5k5ccgx_set_from_table(sd, "af_macro_mode",
				&state->regs->af_macro_mode, 1, 0);
		break;

	default:
#if !defined(CONFIG_MACH_P2)
		if (state->scene_mode == SCENE_MODE_NIGHTSHOT)
			err = s5k5ccgx_set_from_table(sd,
				"af_night_normal_mode",
				&state->regs->af_night_normal_mode, 1, 0);
		else
#endif
			err = s5k5ccgx_set_from_table(sd,
				"af_norma_mode",
				&state->regs->af_normal_mode, 1, 0);
		break;
	}

	CHECK_ERR(err);
	return 0;
}

#ifdef DEBUG_FILTER_DATA
static void __used s5k5ccgx_display_AF_win_info(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k5ccgx_rect first_win = {0, 0, 0, 0};
	struct s5k5ccgx_rect second_win = {0, 0, 0, 0};

	s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
	s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x022C);
	s5k5ccgx_i2c_read_twobyte(client, 0x0F12, (u16 *)&first_win.x);
	s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
	s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x022E);
	s5k5ccgx_i2c_read_twobyte(client, 0x0F12, (u16 *)&first_win.y);
	s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
	s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x0230);
	s5k5ccgx_i2c_read_twobyte(client, 0x0F12, (u16 *)&first_win.width);
	s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
	s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x0232);
	s5k5ccgx_i2c_read_twobyte(client, 0x0F12, (u16 *)&first_win.height);

	s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
	s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x0234);
	s5k5ccgx_i2c_read_twobyte(client, 0x0F12, (u16 *)&second_win.x);
	s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
	s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x0236);
	s5k5ccgx_i2c_read_twobyte(client, 0x0F12, (u16 *)&second_win.y);
	s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
	s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x0238);
	s5k5ccgx_i2c_read_twobyte(client, 0x0F12, (u16 *)&second_win.width);
	s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
	s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x023A);
	s5k5ccgx_i2c_read_twobyte(client, 0x0F12, (u16 *)&second_win.height);

	cam_info("------- AF Window info -------\n");
	cam_info("Firtst Window: (%4d %4d %4d %4d)\n",
		first_win.x, first_win.y, first_win.width, first_win.height);
	cam_info("Second Window: (%4d %4d %4d %4d)\n",
		second_win.x, second_win.y,
		second_win.width, second_win.height);
	cam_info("------- AF Window info -------\n\n");
}
#endif /* DEBUG_FILTER_DATA */

static int s5k5ccgx_af_start_preflash(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k5ccgx_state *state = to_state(sd);
	u16 read_value = 0;
	int count = 0;
	int err = 0;

	cam_trace("E\n");

	if (state->sensor_mode == SENSOR_MOVIE)
		return 0;

	cam_dbg("Start SINGLE AF, flash mode %d\n", state->flash.mode);

	/* in case user calls auto_focus repeatedly without a cancel
	 * or a capture, we need to cancel here to allow ae_awb
	 * to work again, or else we could be locked forever while
	 * that app is running, which is not the expected behavior.
	 */
	err = s5k5ccgx_set_lock(sd, AEAWB_UNLOCK, true);
	CHECK_ERR_MSG(err, "fail to set lock\n");

	state->flash.preflash = PREFLASH_OFF;
	state->light_level = 0xFFFFFFFF;

	s5k5ccgx_get_light_level(sd, &state->light_level);

	switch (state->flash.mode) {
	case FLASH_MODE_AUTO:
		if (state->light_level >= FLASH_LOW_LIGHT_LEVEL) {
			/* flash not needed */
			break;
		}

	case FLASH_MODE_ON:
		s5k5ccgx_set_from_table(sd, "af_pre_flash_start",
			&state->regs->af_pre_flash_start, 1, 0);
		s5k5ccgx_set_from_table(sd, "flash_ae_set",
			&state->regs->flash_ae_set, 1, 0);
		s5k5ccgx_flash_torch(sd, S5K5CCGX_FLASH_ON);
		state->flash.preflash = PREFLASH_ON;
		break;

	case FLASH_MODE_OFF:
		if (state->light_level < FLASH_LOW_LIGHT_LEVEL)
			state->one_frame_delay_ms = ONE_FRAME_DELAY_MS_LOW;
		break;

	default:
		break;
	}

	/* Check AE-stable */
	if (state->flash.preflash == PREFLASH_ON) {
		/* We wait for 200ms after pre flash on.
		 * check whether AE is stable.*/
		msleep_debug(200, true);

		/* Do checking AE-stable */
		for (count = 0; count < AE_STABLE_SEARCH_COUNT; count++) {
			if (state->focus.start == AUTO_FOCUS_OFF) {
				cam_info("af_start_preflash: \
					AF is cancelled!\n");
				state->focus.status = AF_RESULT_CANCELLED;
				break;
			}

			s5k5ccgx_set_from_table(sd, "get_ae_stable",
					&state->regs->get_ae_stable, 1, 0);
			s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &read_value);

			/* af_dbg("Check AE-Stable: 0x%04X\n", read_value); */
			if (read_value == 0x0001) {
				af_dbg("AE-stable success,"
					" count=%d, delay=%dms\n", count,
					AE_STABLE_SEARCH_DELAY);
				break;
			}

			msleep_debug(AE_STABLE_SEARCH_DELAY, false);
		}

		/* restore write mode */
		s5k5ccgx_i2c_write_twobyte(client, 0x0028, 0x7000);

		if (unlikely(count >= AE_STABLE_SEARCH_COUNT)) {
			cam_err("%s: error, AE unstable."
				" count=%d, delay=%dms\n",
				__func__, count, AE_STABLE_SEARCH_DELAY);
			/* return -ENODEV; */
		}
	} else if (state->focus.start == AUTO_FOCUS_OFF) {
		cam_info("af_start_preflash: AF is cancelled!\n");
		state->focus.status = AF_RESULT_CANCELLED;
	}

	/* If AF cancel, finish pre-flash process. */
	if (state->focus.status == AF_RESULT_CANCELLED) {
		if (state->flash.preflash == PREFLASH_ON) {
			s5k5ccgx_set_from_table(sd, "af_pre_flash_end",
				&state->regs->af_pre_flash_end, 1, 0);
			s5k5ccgx_set_from_table(sd, "flash_ae_clear",
				&state->regs->flash_ae_clear, 1, 0);
			s5k5ccgx_flash_torch(sd, S5K5CCGX_FLASH_OFF);
			state->flash.preflash = PREFLASH_NONE;
		}

		if (state->focus.touch)
			state->focus.touch = 0;
	}

	cam_trace("X\n");

	return 0;
}

static int s5k5ccgx_do_af(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k5ccgx_state *state = to_state(sd);
	u16 read_value = 0;
	u32 count = 0;

	cam_trace("E\n");

	/* AE, AWB Lock */
	s5k5ccgx_set_lock(sd, AEAWB_LOCK, false);

	if (state->sensor_mode == SENSOR_MOVIE) {
		s5k5ccgx_set_from_table(sd, "hd_af_start",
			&state->regs->hd_af_start, 1, 0);

		cam_info("%s : 720P Auto Focus Operation\n\n", __func__);
	} else
		s5k5ccgx_set_from_table(sd, "single_af_start",
			&state->regs->single_af_start, 1, 0);

	/* Sleep while 2frame */
	if (state->hd_videomode)
		msleep_debug(100, false); /* 100ms */
	else if (state->scene_mode == SCENE_MODE_NIGHTSHOT)
		msleep_debug(ONE_FRAME_DELAY_MS_NIGHTMODE * 2, false); /* 330ms */
	else
		msleep_debug(ONE_FRAME_DELAY_MS_LOW * 2, false); /* 200ms */

	/* AF Searching */
	cam_dbg("AF 1st search\n");

	/*1st search*/
	for (count = 0; count < FIRST_AF_SEARCH_COUNT; count++) {
		if (state->focus.start == AUTO_FOCUS_OFF) {
			cam_dbg("do_af: AF is cancelled while doing(1st)\n");
			state->focus.status = AF_RESULT_CANCELLED;
			goto check_done;
		}

		read_value = 0x0;
		s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
		s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x2D12);
		s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &read_value);
		af_dbg("1st AF status(%02d) = 0x%04X\n",
					count, read_value);
		if (read_value != 0x01)
			break;

		msleep_debug(AF_SEARCH_DELAY, false);
	}

	if (read_value != 0x02) {
		cam_err("%s: error, 1st AF failed. count=%d, read_val=0x%X\n\n",
					__func__, count, read_value);
		state->focus.status = AF_RESULT_FAILED;
		goto check_done;
	}

	/*2nd search*/
	cam_dbg("AF 2nd search\n");
	for (count = 0; count < SECOND_AF_SEARCH_COUNT; count++) {
		if (state->focus.start == AUTO_FOCUS_OFF) {
			cam_dbg("do_af: AF is cancelled while doing(2nd)\n");
			state->focus.status = AF_RESULT_CANCELLED;
			goto check_done;
		}

		msleep_debug(AF_SEARCH_DELAY, false);

		read_value = 0x0FFFF;
		s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
		s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x1F2F);
		s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &read_value);
		af_dbg("2nd AF status(%02d) = 0x%04X\n",
						count, read_value);
		if ((read_value & 0x0ff00) == 0x0)
			break;
	}

	if (count >= SECOND_AF_SEARCH_COUNT) {
		/* 0x01XX means "not Finish". */
		cam_err("%s: error, 2nd AF failed. read_val=0x%X\n\n",
			__func__, read_value & 0x0ff00);
		state->focus.status = AF_RESULT_FAILED;
		goto check_done;
	}

	cam_info("AF Success!\n");
	state->focus.status = AF_RESULT_SUCCESS;

check_done:
	/* restore write mode */

	/* We only unlocked AE,AWB in case of being cancelled.
	 * But we now unlock it unconditionally if AF is started,
	 */
	if (state->focus.status == AF_RESULT_CANCELLED) {
		cam_dbg("do_af: Single AF cancelled\n");
		s5k5ccgx_set_lock(sd, AEAWB_UNLOCK, false);
	} else {
		state->focus.start = AUTO_FOCUS_OFF;
		cam_dbg("do_af: Single AF finished\n");
	}

	if ((state->flash.preflash == PREFLASH_ON) &&
	    (state->sensor_mode == SENSOR_CAMERA)) {
		s5k5ccgx_set_from_table(sd, "af_pre_flash_end",
				&state->regs->af_pre_flash_end, 1, 0);
		s5k5ccgx_set_from_table(sd, "flash_ae_clear",
			&state->regs->flash_ae_clear, 1, 0);
		s5k5ccgx_flash_torch(sd, S5K5CCGX_FLASH_OFF);
		if (state->focus.status == AF_RESULT_CANCELLED) {
			state->flash.preflash = PREFLASH_NONE;
		}
	}

	/* Notice: we here turn touch flag off set previously
	 * when doing Touch AF. */
	if (state->focus.touch)
		state->focus.touch = 0;

	return 0;
}

static int s5k5ccgx_set_af(struct v4l2_subdev *sd, s32 val)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	if (!IS_AF_SUPPORTED()) {
		cam_info("set_af: not supported\n");
		return 0;
	}

	cam_info("set_af: %s, focus mode %d\n", val ? "start" : "stop",
		state->focus.mode);

	if (unlikely((u32)val >= AUTO_FOCUS_MAX))
		val = AUTO_FOCUS_ON;

	if (state->focus.start == val)
		return 0;
	
	state->focus.start = val;

	if (val == AUTO_FOCUS_ON) {
		if ((state->runmode != RUNMODE_RUNNING) &&
		    (state->runmode != RUNMODE_RECORDING)) {
			cam_err("error, AF can't start, not in preview\n");
			state->focus.start = AUTO_FOCUS_OFF;
			return -ESRCH;
		}

		err = queue_work(state->workqueue, &state->af_work);
		if (unlikely(!err)) {
			cam_warn("AF is still operating!\n");
			return 0;
		}

		state->focus.status = AF_RESULT_DOING;
	} else {
		/* Cancel AF */
		cam_info("set_af: AF cancel requested!\n");
	}

	cam_trace("X\n");
	return 0;
}

static int s5k5ccgx_start_af(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	struct s5k5ccgx_stream_time *win_stable = &state->focus.win_stable;
	u32 touch_win_delay = 0;
	s32 interval = 0;

	cam_trace("E\n");

	mutex_lock(&state->af_lock);
	state->af_pid = task_pid_nr(current);
	state->focus.reset_done = 0;

	if (state->sensor_mode == SENSOR_CAMERA) {
		state->one_frame_delay_ms = ONE_FRAME_DELAY_MS_NORMAL;
		touch_win_delay = ONE_FRAME_DELAY_MS_LOW;
		if (s5k5ccgx_af_start_preflash(sd))
			goto out;

		if (state->focus.status == AF_RESULT_CANCELLED)
			goto out;
	} else
		state->one_frame_delay_ms = touch_win_delay = 50;

	/* sleep here for the time needed for af window before do_af. */
	if (state->focus.touch) {
		do_gettimeofday(&win_stable->curr_time);
		interval = GET_ELAPSED_TIME(win_stable->curr_time, \
				win_stable->before_time) / 1000;
		if (interval < touch_win_delay) {
			cam_dbg("window stable: %dms + %dms\n", interval,
				touch_win_delay - interval);
			msleep_debug(touch_win_delay - interval, true);
		} else
			cam_dbg("window stable: %dms\n", interval);
	}

	s5k5ccgx_do_af(sd);

out:
	state->af_pid = 0;
	mutex_unlock(&state->af_lock);
	cam_trace("X\n");
	return 0;
}

static int s5k5ccgx_stop_af(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	cam_trace("E\n");

	if (check_af_pid(sd)) {
		cam_err("stop_af: I should not be here\n");
		return -EPERM;
	}

	mutex_lock(&state->af_lock);

	switch (state->focus.status) {
	case AF_RESULT_FAILED:
	case AF_RESULT_SUCCESS:
		cam_dbg("Stop AF, focus mode %d, AF result %d\n",
			state->focus.mode, state->focus.status);

		err = s5k5ccgx_set_lock(sd, AEAWB_UNLOCK, false);
		if (unlikely(err)) {
			cam_err("%s: error, fail to set lock\n", __func__);
			goto err_out;
		}
		state->focus.status = AF_RESULT_CANCELLED;
		state->flash.preflash = PREFLASH_NONE;
		break;

	case AF_RESULT_CANCELLED:
		break;

	default:
		cam_warn("stop_af: warn, unnecessary calling. AF status=%d\n",
			state->focus.status);
		/* Return 0. */
		goto err_out;
		break;
	}

	if (state->focus.touch)
		state->focus.touch = 0;

	mutex_unlock(&state->af_lock);
	cam_trace("X\n");
	return 0;

err_out:
	mutex_unlock(&state->af_lock);
	return err;
}

/**
 * s5k5ccgx_check_wait_af_complete: check af and wait
 * @cancel: if true, force to cancel AF.
 *
 * check whether AF is running and then wait to be fininshed.
 */
static int s5k5ccgx_check_wait_af_complete(struct v4l2_subdev *sd, bool cancel)
{
	struct s5k5ccgx_state *state = to_state(sd);

	if (check_af_pid(sd)) {
		cam_err("check_wait_af_complete: I should not be here\n");
		return -EPERM;
	}

	if (AF_RESULT_DOING == state->focus.status) {
		if (cancel)
			s5k5ccgx_set_af(sd, AUTO_FOCUS_OFF);

		mutex_lock(&state->af_lock);
		mutex_unlock(&state->af_lock);
	}

	return 0;
}

static void s5k5ccgx_af_worker(struct work_struct *work)
{
	s5k5ccgx_start_af(&TO_STATE(work, af_work)->sd);
}

static int s5k5ccgx_set_focus_mode(struct v4l2_subdev *sd, s32 val)
{
	struct s5k5ccgx_state *state = to_state(sd);
	u32 cancel = 0;
	u8 focus_mode = (u8)val;
	int err = -EINVAL;

	cam_info("set_focus_mode %d(0x%X)\n", val, val);

	if (!IS_AF_SUPPORTED() || (state->focus.mode == val))
		return 0;

	cancel = (u32)val & FOCUS_MODE_DEFAULT;

	if (cancel) {
		/* Do nothing if cancel request occurs when af is being finished*/
		if (AF_RESULT_DOING == state->focus.status) {
			state->focus.start = AUTO_FOCUS_OFF;
			return 0;
		}
		
		s5k5ccgx_stop_af(sd);
		if (state->focus.reset_done) {
			cam_dbg("AF is already cancelled fully\n");
			goto out;
		}
		state->focus.reset_done = 1;
	}

	mutex_lock(&state->af_lock);

	switch (focus_mode) {
	case FOCUS_MODE_MACRO:
		err = s5k5ccgx_set_from_table(sd, "af_macro_mode",
				&state->regs->af_macro_mode, 1, 0);
		if (unlikely(err)) {
			cam_err("%s: error, fail to af_macro_mode (%d)\n",
				__func__, err);
			goto err_out;
		}

		state->focus.mode = focus_mode;
		break;

	case FOCUS_MODE_INFINITY:
	case FOCUS_MODE_AUTO:
	case FOCUS_MODE_FIXED:
		err = s5k5ccgx_set_from_table(sd, "af_norma_mode",
				&state->regs->af_normal_mode, 1, 0);
		if (unlikely(err)) {
			cam_err("%s: error, fail to af_norma_mode (%d)\n",
				__func__, err);
			goto err_out;
		}

		state->focus.mode = focus_mode;
		break;

	case FOCUS_MODE_FACEDETECT:
	case FOCUS_MODE_CONTINOUS:
	case FOCUS_MODE_TOUCH:
		break;

	default:
		cam_err("%s: error, invalid val(0x%X)\n:",
			__func__, val);
		goto err_out;
		break;
	}

out:
	mutex_unlock(&state->af_lock);
	return 0;

err_out:
	mutex_unlock(&state->af_lock);
	return err;
}

static int s5k5ccgx_set_af_window(struct v4l2_subdev *sd)
{
	int err = -EIO;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k5ccgx_state *state = to_state(sd);
	struct s5k5ccgx_rect inner_window = {0, 0, 0, 0};
	struct s5k5ccgx_rect outter_window = {0, 0, 0, 0};
	struct s5k5ccgx_rect first_window = {0, 0, 0, 0};
	struct s5k5ccgx_rect second_window = {0, 0, 0, 0};
	const s32 mapped_x = state->focus.pos_x;
	const s32 mapped_y = state->focus.pos_y;
	const u32 preview_width = state->preview.frmsize->width;
	const u32 preview_height = state->preview.frmsize->height;
	u32 inner_half_width = 0, inner_half_height = 0;
	u32 outter_half_width = 0, outter_half_height = 0;

	cam_trace("E\n");

	mutex_lock(&state->af_lock);
	state->af_pid = task_pid_nr(current);

	inner_window.width = SCND_WINSIZE_X * preview_width / 1024;
	inner_window.height = SCND_WINSIZE_Y * preview_height / 1024;
	outter_window.width = FIRST_WINSIZE_X * preview_width / 1024;
	outter_window.height = FIRST_WINSIZE_Y * preview_height / 1024;

	inner_half_width = inner_window.width / 2;
	inner_half_height = inner_window.height / 2;
	outter_half_width = outter_window.width / 2;
	outter_half_height = outter_window.height / 2;

	af_dbg("Preview width=%d, height=%d\n", preview_width, preview_height);
	af_dbg("inner_window_width=%d, inner_window_height=%d, " \
		"outter_window_width=%d, outter_window_height=%d\n ",
		inner_window.width, inner_window.height,
		outter_window.width, outter_window.height);

	/* Get X */
	if (mapped_x <= inner_half_width) {
		inner_window.x = outter_window.x = 0;
		af_dbg("inner & outter window over sensor left."
			"in_x=%d, out_x=%d\n", inner_window.x, outter_window.x);
	} else if (mapped_x <= outter_half_width) {
		inner_window.x = mapped_x - inner_half_width;
		outter_window.x = 0;
		af_dbg("outter window over sensor left. in_x=%d, out_x=%d\n",
					inner_window.x, outter_window.x);
	} else if (mapped_x >= ((preview_width - 1) - inner_half_width)) {
		inner_window.x = (preview_width - 1) - inner_window.width;
		outter_window.x = (preview_width - 1) - outter_window.width;
		af_dbg("inner & outter window over sensor right." \
			"in_x=%d, out_x=%d\n", inner_window.x, outter_window.x);
	} else if (mapped_x >= ((preview_width - 1) - outter_half_width)) {
		inner_window.x = mapped_x - inner_half_width;
		outter_window.x = (preview_width - 1) - outter_window.width;
		af_dbg("outter window over sensor right. in_x=%d, out_x=%d\n",
					inner_window.x, outter_window.x);
	} else {
		inner_window.x = mapped_x - inner_half_width;
		outter_window.x = mapped_x - outter_half_width;
		af_dbg("inner & outter window within sensor area." \
			"in_x=%d, out_x=%d\n", inner_window.x, outter_window.x);
	}

	/* Get Y */
	if (mapped_y <= inner_half_height) {
		inner_window.y = outter_window.y = 0;
		af_dbg("inner & outter window over sensor top." \
			"in_y=%d, out_y=%d\n", inner_window.y, outter_window.y);
	} else if (mapped_y <= outter_half_height) {
		inner_window.y = mapped_y - inner_half_height;
		outter_window.y = 0;
		af_dbg("outter window over sensor top. in_y=%d, out_y=%d\n",
					inner_window.y, outter_window.y);
	} else if (mapped_y >= ((preview_height - 1) - inner_half_height)) {
		inner_window.y = (preview_height - 1) - inner_window.height;
		outter_window.y = (preview_height - 1) - outter_window.height;
		af_dbg("inner & outter window over sensor bottom." \
			"in_y=%d, out_y=%d\n", inner_window.y, outter_window.y);
	} else if (mapped_y >= ((preview_height - 1) - outter_half_height)) {
		inner_window.y = mapped_y - inner_half_height;
		outter_window.y = (preview_height - 1) - outter_window.height;
		af_dbg("outter window over sensor bottom. in_y=%d, out_y=%d\n",
					inner_window.y, outter_window.y);
	} else {
		inner_window.y = mapped_y - inner_half_height;
		outter_window.y = mapped_y - outter_half_height;
		af_dbg("inner & outter window within sensor area." \
			"in_y=%d, out_y=%d\n", inner_window.y, outter_window.y);
	}

	af_dbg("==> inner_window top=(%d,%d), bottom=(%d, %d)\n",
		inner_window.x, inner_window.y,
		inner_window.x + inner_window.width,
		inner_window.y + inner_window.height);
	af_dbg("==> outter_window top=(%d,%d), bottom=(%d, %d)\n",
		outter_window.x, outter_window.y,
		outter_window.x + outter_window.width ,
		outter_window.y + outter_window.height);

	second_window.x = inner_window.x * 1024 / preview_width;
	second_window.y = inner_window.y * 1024 / preview_height;
	first_window.x = outter_window.x * 1024 / preview_width;
	first_window.y = outter_window.y * 1024 / preview_height;

	af_dbg("=> second_window top=(%d, %d)\n",
		second_window.x, second_window.y);
	af_dbg("=> first_window top=(%d, %d)\n",
		first_window.x, first_window.y);

	/* restore write mode */
	err = s5k5ccgx_i2c_write_twobyte(client, 0x0028, 0x7000);

	/* Set first window x, y */
	err |= s5k5ccgx_i2c_write_twobyte(client, 0x002A, 0x022C);
	err |= s5k5ccgx_i2c_write_twobyte(client, 0x0F12,
					(u16)(first_window.x));
	err |= s5k5ccgx_i2c_write_twobyte(client, 0x002A, 0x022E);
	err |= s5k5ccgx_i2c_write_twobyte(client, 0x0F12,
					(u16)(first_window.y));

	/* Set second widnow x, y */
	err |= s5k5ccgx_i2c_write_twobyte(client, 0x002A, 0x0234);
	err |= s5k5ccgx_i2c_write_twobyte(client, 0x0F12,
					(u16)(second_window.x));
	err |= s5k5ccgx_i2c_write_twobyte(client, 0x002A, 0x0236);
	err |= s5k5ccgx_i2c_write_twobyte(client, 0x0F12,
					(u16)(second_window.y));

	/* Update AF window */
	err |= s5k5ccgx_i2c_write_twobyte(client, 0x002A, 0x023C);
	err |= s5k5ccgx_i2c_write_twobyte(client, 0x0F12, 0x0001);

	do_gettimeofday(&state->focus.win_stable.before_time);
	state->focus.touch = 1;
	state->af_pid = 0;
	mutex_unlock(&state->af_lock);

	CHECK_ERR(err);
	cam_info("AF window position completed.\n");

	cam_trace("X\n");
	return 0;
}

static void s5k5ccgx_af_win_worker(struct work_struct *work)
{
	s5k5ccgx_set_af_window(&TO_STATE(work, af_win_work)->sd);
}

static int s5k5ccgx_set_touch_af(struct v4l2_subdev *sd, s32 val)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -EIO;

	if (!IS_AF_SUPPORTED())
		return 0;

	cam_trace("%s, x=%d y=%d\n", val ? "start" : "stop",
			state->focus.pos_x, state->focus.pos_y);

	if (val) {
		if (mutex_is_locked(&state->af_lock)) {
			cam_warn("%s: AF is still operating!\n", __func__);
			return 0;
		}

		err = queue_work(state->workqueue, &state->af_win_work);
		if (likely(!err))
			cam_warn("WARNING, AF window is still processing\n");
	} else
		cam_info("set_touch_af: invalid value %d\n", val);

	cam_trace("X\n");
	return 0;
}

static const struct s5k5ccgx_framesize * __used
s5k5ccgx_get_framesize_i(const struct s5k5ccgx_framesize *frmsizes,
				u32 frmsize_count, u32 index)
{
	int i = 0;

	for (i = 0; i < frmsize_count; i++) {
		if (frmsizes[i].index == index)
			return &frmsizes[i];
	}

	return NULL;
}

static const struct s5k5ccgx_framesize * __used
s5k5ccgx_get_framesize_sz(const struct s5k5ccgx_framesize *frmsizes,
				u32 frmsize_count, u32 width, u32 height)
{
	int i;

	for (i = 0; i < frmsize_count; i++) {
		if ((frmsizes[i].width == width) && (frmsizes[i].height == height))
			return &frmsizes[i];
	}

	return NULL;
}

static const struct s5k5ccgx_framesize * __used
s5k5ccgx_get_framesize_ratio(const struct s5k5ccgx_framesize *frmsizes,
				u32 frmsize_count, u32 width, u32 height)
{
	int i = 0;
	int found = -ENOENT;
	const int ratio = FRM_RATIO(width, height);
	
	for (i = 0; i < frmsize_count; i++) {
		if ((frmsizes[i].width == width) && (frmsizes[i].height == height))
			return &frmsizes[i];

		if (FRAMESIZE_RATIO(&frmsizes[i]) == ratio) {
			if ((-ENOENT == found) ||
			    (frmsizes[found].width < frmsizes[i].width))
				found = i;
		}
	}

	if (found != -ENOENT) {
		cam_dbg("get_framesize: %dx%d -> %dx%d\n", width, height,
			frmsizes[found].width, frmsizes[found].height);
		return &frmsizes[found];
	} else
		return NULL;
}

/* This function is called from the g_ctrl api
 *
 * This function should be called only after the s_fmt call,
 * which sets the required width/height value.
 *
 * It checks a list of available frame sizes and sets the
 * most appropriate frame size.
 *
 * The list is stored in an increasing order (as far as possible).
 * Hence the first entry (searching from the beginning) where both the
 * width and height is more than the required value is returned.
 * In case of no perfect match, we set the last entry (which is supposed
 * to be the largest resolution supported.)
 */
static void s5k5ccgx_set_framesize(struct v4l2_subdev *sd,
				const struct s5k5ccgx_framesize *frmsizes,
				u32 num_frmsize, bool preview)
{
	struct s5k5ccgx_state *state = to_state(sd);
	const struct s5k5ccgx_framesize **found_frmsize = NULL;
	u32 width = state->req_fmt.width;
	u32 height = state->req_fmt.height;

	cam_dbg("set_framesize: Requested Res %dx%d\n", width, height);

	found_frmsize = /*(const struct s5k5ccgx_framesize **) */
			(preview ? &state->preview.frmsize: &state->capture.frmsize);

	if (state->hd_videomode) {
		*found_frmsize = s5k5ccgx_get_framesize_sz(
				s5k5ccgx_hidden_preview_frmsizes, 
				ARRAY_SIZE(s5k5ccgx_hidden_preview_frmsizes),
				width, height);

		if (*found_frmsize == NULL)
			*found_frmsize = s5k5ccgx_get_framesize_ratio(
					frmsizes, num_frmsize, width, height);
	} else
		*found_frmsize = s5k5ccgx_get_framesize_ratio(
				frmsizes, num_frmsize, width, height);

	if (*found_frmsize == NULL) {
		cam_err("%s: error, invalid frame size %dx%d\n",
			__func__, width, height);
		*found_frmsize = preview ?
			s5k5ccgx_get_framesize_i(frmsizes, num_frmsize,
				(state->sensor_mode == SENSOR_CAMERA) ?
				PREVIEW_SZ_XGA: PREVIEW_SZ_1024x576):
			s5k5ccgx_get_framesize_i(frmsizes, num_frmsize,
					CAPTURE_SZ_3MP);
		BUG_ON(!(*found_frmsize));
	}

	if (preview) {
		cam_info("Preview Res Set: %dx%d, index %d\n",
			(*found_frmsize)->width, (*found_frmsize)->height,
			(*found_frmsize)->index);
	} else {
		cam_info("Capture Res Set: %dx%d, index %d\n",
			(*found_frmsize)->width, (*found_frmsize)->height,
			(*found_frmsize)->index);
	}
}

#if CONFIG_DEBUG_STREAMOFF && defined(CONFIG_VIDEO_IMPROVE_STREAMOFF)
static int s5k5ccgx_start_streamoff_checker(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	cam_trace("EX");
	atomic_set(&state->streamoff_check, true);

	err = queue_work(state->workqueue, &state->streamoff_work);
	if (unlikely(!err))
		cam_info("streamoff_checker is already operating!\n");

	return 0;
}

static void s5k5ccgx_stop_streamoff_checker(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);

	cam_trace("EX");

	atomic_set(&state->streamoff_check, false);

	if (flush_work(&state->streamoff_work))
		cam_dbg("wait... streamoff_checker stopped\n");
}

static void s5k5ccgx_streamoff_checker(struct work_struct *work)
{
	struct s5k5ccgx_state *state = TO_STATE(work, streamoff_work);
	struct i2c_client *client = v4l2_get_subdevdata(&state->sd);
	struct timeval start_time, end_time;
	s32 elapsed_msec = 0;
	u32 count;
	u16 val = 0;
	int err = 0;

	if (!atomic_read(&state->streamoff_check))
		return;

	do_gettimeofday(&start_time);

	for (count = 1; count != 0; count++) {
		err = s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
		err |= s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x01E6);
		err |= s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &val);
		if (unlikely(err)) {
			cam_err("streamoff_checker: error, readb\n");
			goto out;
		}

		/* complete */
		if ((val & 0x01) == 0) {
			do_gettimeofday(&end_time);
			elapsed_msec = GET_ELAPSED_TIME(end_time, 
						start_time) / 1000;
			cam_dbg("**** streamoff_checker:"
				" complete! %dms ****\n", elapsed_msec);
			goto out;
		}

		if (!atomic_read(&state->streamoff_check)) {
			do_gettimeofday(&end_time);
			elapsed_msec = GET_ELAPSED_TIME(end_time, 
						start_time) / 1000;
			cam_dbg("streamoff_checker aborted (%dms)\n",
				elapsed_msec);
			return;
		}

		/* cam_info("wait_steamoff: val = 0x%X\n", val);*/
		msleep_debug(5, false);
	}

out:
	atomic_set(&state->streamoff_check, false);
	if (!count)
		cam_info("streamoff_checker: time-out\n\n");

	return;
}
#endif /* CONFIG_DEBUG_STREAMOFF */

#if CONFIG_SUPPORT_WAIT_STREAMOFF_V2
static int s5k5ccgx_wait_steamoff(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k5ccgx_platform_data *pdata = state->pdata;
	struct s5k5ccgx_stream_time *stream_time = &state->stream_time;
	s32 elapsed_msec = 0;
	int count, err = 0;
	u16 val = 0;

	cam_trace("E\n");

	if (unlikely(!(pdata->common.is_mipi & state->need_wait_streamoff)))
		return 0;

	do_gettimeofday(&stream_time->curr_time);

	elapsed_msec = GET_ELAPSED_TIME(stream_time->curr_time, \
			stream_time->before_time) / 1000;

	for (count = 0; count < STREAMOFF_CHK_COUNT; count++) {
		err = s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
		err |= s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x01E6);
		err |= s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &val);
		CHECK_ERR_MSG(err, "wait_steamoff: error, readb\n")

		if ((val & 0x01) == 0) {
			cam_dbg("wait_steamoff: %dms + cnt(%d)\n",
				elapsed_msec, count);
			break;
		}

		/* cam_info("wait_steamoff: val = 0x%X\n", val);*/
		msleep_debug(5, false);
	}

	if (unlikely(count >= STREAMOFF_CHK_COUNT))
		cam_info("wait_steamoff: time-out!\n\n");

#if CONFIG_DEBUG_STREAMOFF && defined(CONFIG_VIDEO_IMPROVE_STREAMOFF)
	s5k5ccgx_stop_streamoff_checker(sd);
#endif

	state->need_wait_streamoff = 0;

	return 0;
}

#else
static int s5k5ccgx_wait_steamoff(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	struct s5k5ccgx_platform_data *pdata = state->pdata;
	struct s5k5ccgx_stream_time *stream_time = &state->stream_time;
	s32 elapsed_msec = 0;

	if (unlikely(!(pdata->common.is_mipi & state->need_wait_streamoff)))
		return 0;

	cam_trace("E\n");

	do_gettimeofday(&stream_time->curr_time);

	elapsed_msec = GET_ELAPSED_TIME(stream_time->curr_time, \
				stream_time->before_time) / 1000;

	if (pdata->common.streamoff_delay > elapsed_msec) {
		cam_info("stream-off: %dms + %dms\n", elapsed_msec,
			pdata->common.streamoff_delay - elapsed_msec);
		msleep_debug(pdata->common.streamoff_delay - elapsed_msec, true);
	} else
		cam_info("stream-off: %dms\n", elapsed_msec);

#if CONFIG_DEBUG_STREAMOFF && defined(CONFIG_VIDEO_IMPROVE_STREAMOFF)
	s5k5ccgx_stop_streamoff_checker(sd);
#endif

	state->need_wait_streamoff = 0;

	return 0;
}
#endif /* CONFIG_SUPPORT_WAIT_STREAMOFF_V2 */

static int s5k5ccgx_control_stream(struct v4l2_subdev *sd, u32 cmd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -EINVAL;

	if (!state->pdata->common.is_mipi)
		return 0;

	if (unlikely(cmd != STREAM_STOP))
		return 0;

	if (IS_AF_SUPPORTED())
		s5k5ccgx_check_wait_af_complete(sd, true);

	cam_info("STREAM STOP!!\n");
	err = s5k5ccgx_set_from_table(sd, "stream_stop",
			&state->regs->stream_stop, 1, 0);
	CHECK_ERR_MSG(err, "failed to stop stream\n");

#ifdef CONFIG_VIDEO_IMPROVE_STREAMOFF
	do_gettimeofday(&state->stream_time.before_time);
	state->need_wait_streamoff = 1;

#if CONFIG_DEBUG_STREAMOFF
	s5k5ccgx_start_streamoff_checker(sd);
#endif
#else
	msleep_debug(state->pdata->common.streamoff_delay, true);
#endif /* CONFIG_VIDEO_IMPROVE_STREAMOFF */

	switch (state->runmode) {
	case RUNMODE_CAPTURING:
		cam_dbg("Capture Stop!\n");
		state->runmode = RUNMODE_CAPTURING_STOP;
		state->capture.ready = 0;
		state->capture.lowlux_night = 0; /* fix me 130118 */

		/* We turn flash off if one-shot flash is still on. */
		if (s5k5ccgx_is_hwflash_on(sd))
			s5k5ccgx_flash_oneshot(sd, S5K5CCGX_FLASH_OFF);
		else
			state->flash.on = 0;

		err = s5k5ccgx_set_lock(sd, AEAWB_UNLOCK, true);
		CHECK_ERR_MSG(err, "fail to set lock\n");

#ifndef CONFIG_MACH_PX
		/* Fix me */
		if (SCENE_MODE_NIGHTSHOT == state->scene_mode) {
			state->pdata->common.streamoff_delay = 
				S5K5CCGX_STREAMOFF_NIGHT_DELAY;
		}
#endif
		break;

	case RUNMODE_RUNNING:
		cam_dbg("Preview Stop!\n");
		state->runmode = RUNMODE_RUNNING_STOP;

		/* if (state->capture.pre_req) {
			isx012_prepare_fast_capture(sd);
			state->capture.pre_req = 0;
		} */
		break;

	case RUNMODE_RECORDING:
		state->runmode = RUNMODE_RECORDING_STOP;
		break;

	default:
		break;
	}

	return 0;
}

/* PX: Set flash mode */
static int s5k5ccgx_set_flash_mode(struct v4l2_subdev *sd, s32 val)
{
	struct s5k5ccgx_state *state = to_state(sd);

	/* movie flash mode should be set when recording is started */
	/* if (state->sensor_mode == SENSOR_MOVIE && !state->recording)
		return 0;*/

	if (!IS_FLASH_SUPPORTED() || (state->flash.mode == val)) {
		cam_dbg("not support or the same flash mode=%d\n", val);
		return 0;
	}

	if (val == FLASH_MODE_TORCH)
		s5k5ccgx_flash_torch(sd, S5K5CCGX_FLASH_ON);

	if ((state->flash.mode == FLASH_MODE_TORCH)
	    && (val == FLASH_MODE_OFF))
		s5k5ccgx_flash_torch(sd, S5K5CCGX_FLASH_OFF);

	state->flash.mode = val;

	cam_dbg("Flash mode = %d\n", val);
	return 0;
}

static int s5k5ccgx_check_esd(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL;
	u16 read_value = 0;

	err = s5k5ccgx_set_from_table(sd, "get_esd_status",
		&state->regs->get_esd_status, 1, 0);
	CHECK_ERR(err);
	err = s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &read_value);
	CHECK_ERR(err);

	if (read_value != 0xAAAA)
		goto esd_out;

	cam_info("Check ESD: not detected\n\n");
	return 0;

esd_out:
	cam_err("Check ESD: error, ESD Shock detected! (val=0x%X)\n\n",
		read_value);
	return -ERESTART;
}

/* returns the real iso currently used by sensor due to lighting
 * conditions, not the requested iso we sent using s_ctrl.
 */
/* PX: */
static inline int s5k5ccgx_get_exif_iso(struct v4l2_subdev *sd, u16 *iso)
{
	struct s5k5ccgx_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 iso_gain_table[] = {10, 15, 25, 35};
	u16 iso_table[] = {0, 50, 100, 200, 400};
	int err = -EIO;
	u16 val = 0, gain = 0;
	int i = 0;

	err = s5k5ccgx_set_from_table(sd, "get_iso",
				&state->regs->get_iso, 1, 0);
	err |= s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &val);
	CHECK_ERR(err);

	gain = val * 10 / 256;
	for (i = 0; i < ARRAY_SIZE(iso_gain_table); i++) {
		if (gain < iso_gain_table[i])
			break;
	}

	*iso = iso_table[i];

	cam_dbg("gain=%d, ISO=%d\n", gain, *iso);

	/* We do not restore write mode */

	return 0;
}

/* PX: Set ISO */
static int __used s5k5ccgx_set_iso(struct v4l2_subdev *sd, s32 val)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -EINVAL;

retry:
	switch (val) {
	case ISO_AUTO:
	case ISO_50:
	case ISO_100:
	case ISO_200:
	case ISO_400:
		err = s5k5ccgx_set_from_table(sd, "iso",
			state->regs->iso, ARRAY_SIZE(state->regs->iso),
			val);
		break;

	default:
		cam_err("%s: error, invalid arguement(%d)\n", __func__, val);
		val = ISO_AUTO;
		goto retry;
		break;
	}

	cam_trace("X\n");
	return 0;
}

/* PX: Return exposure time (ms) */
static inline int s5k5ccgx_get_exif_exptime(struct v4l2_subdev *sd,
						u32 *exp_time)
{
	struct s5k5ccgx_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err;
	u16 read_value_lsb = 0;
	u16 read_value_msb = 0;

	err = s5k5ccgx_set_from_table(sd, "get_shutterspeed",
				&state->regs->get_shutterspeed, 1, 0);
	CHECK_ERR(err);

	err = s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &read_value_lsb);
	err |= s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &read_value_msb);
	CHECK_ERR(err);

	*exp_time = (((read_value_msb << 16) | (read_value_lsb & 0xFFFF))
			* 1000) / 400;

	/* We do not restore write mode */

	return 0;

}

static inline void s5k5ccgx_get_exif_flash(struct v4l2_subdev *sd,
					u16 *flash)
{
	struct s5k5ccgx_state *state = to_state(sd);

	*flash = 0;

	switch (state->flash.mode) {
	case FLASH_MODE_OFF:
		*flash |= EXIF_FLASH_MODE_SUPPRESSION;
		break;

	case FLASH_MODE_AUTO:
		*flash |= EXIF_FLASH_MODE_AUTO;
		break;

	case FLASH_MODE_ON:
	case FLASH_MODE_TORCH:
		*flash |= EXIF_FLASH_MODE_FIRING;
		break;

	default:
		break;
	}

	if (state->flash.on) {
		*flash |= EXIF_FLASH_FIRED;
		if (state->sensor_mode == SENSOR_CAMERA)
			state->flash.on = 0;
	}

}

/* PX: */
static int s5k5ccgx_get_exif(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	u32 exposure_time = 0;

	/* exposure time */
	state->exif.exp_time_den = 0;
	s5k5ccgx_get_exif_exptime(sd, &exposure_time);
	if (exposure_time)
		state->exif.exp_time_den = 1000 * 1000 / exposure_time;
	else
		cam_err("exposure time 0. Check me!\n");

	/* iso */
	state->exif.iso = 0;
	s5k5ccgx_get_exif_iso(sd, &state->exif.iso);

	/* flash */
	s5k5ccgx_get_exif_flash(sd, &state->exif.flash);

	cam_dbg("EXIF: ex_time_den=%d, iso=%d, flash=0x%02X\n",
		state->exif.exp_time_den, state->exif.iso, state->exif.flash);

	return 0;
}

static inline int s5k5ccgx_check_cap_sz_except(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -EINVAL;

	cam_err("capture_size_exception: warning, reconfiguring...\n\n");

	switch (state->wide_cmd) {
	case WIDE_REQ_CHANGE:
		cam_info("%s: Wide Capture setting\n", __func__);
		err = s5k5ccgx_set_from_table(sd, "change_wide_cap",
			&state->regs->change_wide_cap, 1, 0);
		break;

	case WIDE_REQ_RESTORE:
		cam_info("%s: Restore capture setting\n", __func__);
		err = s5k5ccgx_set_from_table(sd, "restore_capture",
				&state->regs->restore_cap, 1, 0);
		break;

	default:
		cam_err("%s: WARNING, invalid argument(%d)\n",
				__func__, state->wide_cmd);
		break;
	}

	/* Don't forget the below codes.
	 * We set here state->preview to NULL after reconfiguring
	 * capure config if capture ratio does't match with preview ratio.
	 */
	state->preview.frmsize = NULL;
	CHECK_ERR(err);

	return 0;
}

static int s5k5ccgx_set_preview_size(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -EINVAL;

	if (!state->preview.update_frmsize)
		return 0;

	if (state->hd_videomode)
		goto out;

	cam_trace("E, wide_cmd=%d\n", state->wide_cmd);

	switch (state->wide_cmd) {
	case WIDE_REQ_CHANGE:
		cam_info("%s: Wide Capture setting\n", __func__);
		err = s5k5ccgx_set_from_table(sd, "change_wide_cap",
			&state->regs->change_wide_cap, 1, 0);
		break;

	case WIDE_REQ_RESTORE:
		cam_info("%s:Restore capture setting\n", __func__);
		err = s5k5ccgx_set_from_table(sd, "restore_capture",
				&state->regs->restore_cap, 1, 0);
		/* We do not break */

	default:
		cam_dbg("set_preview_size\n");
		err = s5k5ccgx_set_from_table(sd, "preview_size",
				state->regs->preview_size,
				ARRAY_SIZE(state->regs->preview_size),
				state->preview.frmsize->index);
#ifdef CONFIG_MACH_PX
		BUG_ON(state->preview.frmsize->index == PREVIEW_SZ_PVGA);
#endif
		break;
	}
	CHECK_ERR(err);

out:
	state->preview.update_frmsize = 0;

	return 0;
}

static int s5k5ccgx_set_capture_size(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	/* do nothing about size except switching normal(wide) 
	 * to wide(normal). */
	if (unlikely(state->wide_cmd))
		err = s5k5ccgx_check_cap_sz_except(sd);

	return err;
}

static int s5k5ccgx_start_preview(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -EINVAL;

	cam_dbg("Camera Preview start, runmode = %d\n", state->runmode);

	if ((state->runmode == RUNMODE_NOTREADY) ||
	    (state->runmode == RUNMODE_CAPTURING)) {
		cam_err("%s: error - Invalid runmode\n", __func__);
		return -EPERM;
	}

	if (IS_FLASH_SUPPORTED() || IS_AF_SUPPORTED()) {
		state->flash.preflash = PREFLASH_NONE;
		state->focus.status = AF_RESULT_NONE;
		state->focus.touch = 0;
	}

	/* Set movie mode if needed. */
	err = s5k5ccgx_transit_movie_mode(sd);

	/* Set preview size */
	err = s5k5ccgx_set_preview_size(sd);
	CHECK_ERR_MSG(err, "failed to set preview size(%d)\n", err);

	/* Transit preview mode */
	err = s5k5ccgx_transit_preview_mode(sd);
	CHECK_ERR_MSG(err, "preview_mode(%d)\n", err);

	state->runmode = (state->sensor_mode == SENSOR_CAMERA) ?
			RUNMODE_RUNNING : RUNMODE_RECORDING;

	return 0;
}

/* PX: Start capture */
static int s5k5ccgx_start_capture(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -ENODEV;
#if CONFIG_SUPPORT_FLASH
	u32 light_level = 0xFFFFFFFF;
#endif

	/* Set capture size */
	err = s5k5ccgx_set_capture_size(sd);
	CHECK_ERR_MSG(err, "fail to set capture size (%d)\n", err);

#if CONFIG_SUPPORT_FLASH
	/* Set flash */
	switch (state->flash.mode) {
	case FLASH_MODE_AUTO:
		/* 3rd party App may do capturing without AF. So we check
		 * whether AF is executed  before capture and  turn on flash
		 * if needed. But we do not consider low-light capture of Market
		 * App. */
		if (state->flash.preflash == PREFLASH_NONE) {
			s5k5ccgx_get_light_level(sd, &state->light_level);
			if (light_level >= FLASH_LOW_LIGHT_LEVEL)
				break;
		} else if (state->flash.preflash == PREFLASH_OFF)
			break;
		/* We do not break. */

	case FLASH_MODE_ON:
		s5k5ccgx_flash_oneshot(sd, S5K5CCGX_FLASH_ON);
		/* We here don't need to set state->flash.on to 1 */

		err = s5k5ccgx_set_lock(sd, AEAWB_UNLOCK, true);
		CHECK_ERR_MSG(err, "fail to set lock\n");

		/* Full flash start */
		err = s5k5ccgx_set_from_table(sd, "flash_start",
			&state->regs->flash_start, 1, 0);
		break;

	case FLASH_MODE_OFF:
#ifdef CONFIG_MACH_P8
		if (state->light_level < CAPTURE_LOW_LIGHT_LEVEL)
			err = s5k5ccgx_set_from_table(sd, "set_lowlight_cap",
				&state->regs->set_lowlight_cap, 1, 0);
		break;
#endif
	default:
		break;
	}
#endif /* CONFIG_SUPPORT_FLASH */

	/* Transit to capture mode */
	s5k5ccgx_transit_capture_mode(sd);

	if (state->scene_mode == SCENE_MODE_NIGHTSHOT)
		msleep_debug(140, true);

	state->runmode = RUNMODE_CAPTURING;

	CHECK_ERR_MSG(err, "fail to capture_mode (%d)\n", err);

	s5k5ccgx_get_exif(sd);

	return 0;
}

/**
 * s5k5ccgx_switch_hd_mode: swith to/from HD mode
 * @fmt: 
 *
 */
static inline int s5k5ccgx_switch_hd_mode(
		struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	struct s5k5ccgx_state *state = to_state(sd);
	bool hd_mode = 0;
	u32 ratio = FRM_RATIO(fmt->width, fmt->height);
	int err = -EINVAL;

	cam_trace("EX, width = %d, ratio = %d", fmt->width, ratio);

	if ((SENSOR_MOVIE == state->sensor_mode) 
	    && (fmt->width == 1280) && (ratio == FRMRATIO_HD))
		hd_mode = 1;

	if (state->hd_videomode ^ hd_mode) {
		cam_info("transit HD video %d -> %d\n", !hd_mode, hd_mode);
		state->hd_videomode = hd_mode;

		cam_dbg("******* Reset start *******\n");
		err = s5k5ccgx_reset(sd, 2);
		CHECK_ERR(err);

		s5k5ccgx_init(sd, 0);
		CHECK_ERR(err);
		cam_dbg("******* Reset end *******\n\n");
	}

	return 0;
}

static void s5k5ccgx_init_parameter(struct v4l2_subdev *sd)
{
	struct s5k5ccgx_state *state = to_state(sd);

	state->runmode = RUNMODE_INIT;

	/* Default state values */
	if (IS_FLASH_SUPPORTED()) {
		state->flash.mode = FLASH_MODE_OFF;
		state->flash.on = 0;
	}

	state->scene_mode = SCENE_MODE_NONE;
	state->light_level = 0xFFFFFFFF;

	if (IS_AF_SUPPORTED()) {
		memset(&state->focus, 0, sizeof(state->focus));
		state->focus.support = IS_AF_SUPPORTED(); /* Don't fix me */
	}
}

static int s5k5ccgx_restore_parameter(struct v4l2_subdev *sd)
{
	struct v4l2_control ctrl;
	int i;

	cam_trace("EX\n");

	for (i = 0; i < ARRAY_SIZE(s5k5ccgx_ctrls); i++) {
		if (s5k5ccgx_ctrls[i].value !=
		    s5k5ccgx_ctrls[i].default_value) {
			ctrl.id = s5k5ccgx_ctrls[i].id;
			ctrl.value = s5k5ccgx_ctrls[i].value;
			s5k5ccgx_s_ctrl(sd, &ctrl);
		}
	}

	return 0;
}

static int s5k5ccgx_check_sensor(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 read_value = 0;
	int err = -ENODEV;

	/* enter read mode */
	err = s5k5ccgx_i2c_write_twobyte(client, 0x002C, 0x7000);
	CHECK_ERR_COND(err < 0, -ENODEV);

	s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x0150);
	s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &read_value);
	if (unlikely(read_value != S5K5CCGX_CHIP_ID)) {
		cam_err("Sensor ChipID: unknown ChipID %04X\n", read_value);
		return -ENODEV;
	}

	s5k5ccgx_i2c_write_twobyte(client, 0x002E, 0x0152);
	s5k5ccgx_i2c_read_twobyte(client, 0x0F12, &read_value);
	if (unlikely(read_value != S5K5CCGX_CHIP_REV)) {
		cam_err("Sensor Rev: unknown Rev 0x%04X\n", read_value);
		return -ENODEV;
	}

	cam_dbg("Sensor chip indentification: Success");

	/* restore write mode */
	err = s5k5ccgx_i2c_write_twobyte(client, 0x0028, 0x7000);
	CHECK_ERR_COND(err < 0, -ENODEV);

	return 0;
}

static int s5k5ccgx_s_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct s5k5ccgx_state *state = to_state(sd);
	s32 prev_index = 0;

	cam_dbg("%s: pixelformat = 0x%x, colorspace = 0x%x, width = %d, height = %d\n",
		__func__, fmt->code, fmt->colorspace, fmt->width, fmt->height);

	v4l2_fill_pix_format(&state->req_fmt, fmt);
#ifdef CONFIG_MACH_PX
	state->format_mode = fmt->field;
#else
	if ((IS_MODE_CAPTURE_STILL == fmt->field)
	    && (SENSOR_CAMERA == state->sensor_mode))
		state->format_mode = V4L2_PIX_FMT_MODE_CAPTURE;
	else
		state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
#endif

	state->wide_cmd = WIDE_REQ_NONE;

	if (state->format_mode != V4L2_PIX_FMT_MODE_CAPTURE) {
		prev_index = state->preview.frmsize ?
				state->preview.frmsize->index : -1;

		s5k5ccgx_set_framesize(sd, s5k5ccgx_preview_frmsizes,
			ARRAY_SIZE(s5k5ccgx_preview_frmsizes), true);

		/* preview.frmsize cannot absolutely go to null */
		if (state->preview.frmsize->index != prev_index) {
			state->preview.update_frmsize = 1;

			if ((state->preview.frmsize->index == PREVIEW_WIDE_SIZE)
			    && !state->hd_videomode) {
				cam_dbg("preview, need to change to WIDE\n");
				state->wide_cmd = WIDE_REQ_CHANGE;
			} else if ((prev_index == PREVIEW_WIDE_SIZE)
			    && !state->hd_videomode) {
				cam_dbg("preview, need to restore form WIDE\n");
				state->wide_cmd = WIDE_REQ_RESTORE;
			}
		}
	} else {
		s5k5ccgx_set_framesize(sd, s5k5ccgx_capture_frmsizes,
			ARRAY_SIZE(s5k5ccgx_capture_frmsizes), false);

		/* For maket app.
		 * Samsung camera app does not use unmatched ratio.*/
		if (unlikely(!state->preview.frmsize))
			cam_warn("warning, capture without preview\n");
		else if (unlikely(FRAMESIZE_RATIO(state->preview.frmsize)
		    != FRAMESIZE_RATIO(state->capture.frmsize))) {
			cam_warn("warning, preview, capture ratio not matched\n\n");
			if (state->capture.frmsize->index == CAPTURE_WIDE_SIZE) {
				cam_dbg("captre: need to change to WIDE\n");
				state->wide_cmd = WIDE_REQ_CHANGE;
			} else {
				cam_dbg("capture, need to restore form WIDE\n");
				state->wide_cmd = WIDE_REQ_RESTORE;
			}
		}
	}

	return 0;
}

/**
 * s5k5ccgx_pre_s_mbus_fmt: wrapper function of s_fmt() with HD-checking codes
 * @fmt: 
 *
 * We reset sensor to switch between HD and camera mode before calling the orignal 
 * s_fmt function.
 * I don't want to make this routine...
 */
static int s5k5ccgx_pre_s_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	/* check whether to be HD or normal video, and then transit */
	err = s5k5ccgx_switch_hd_mode(sd, fmt);
	CHECK_ERR(err);

	/* check whether sensor is initialized, against exception cases */
	if (unlikely(!state->initialized)) {
		cam_warn("s_fmt: warning, not initialized\n");
		err = s5k5ccgx_init(sd, 0);
		CHECK_ERR(err);
	}

	/* call original s_mbus_fmt() */
	s5k5ccgx_s_mbus_fmt(sd, fmt);

	return 0;
}

static int s5k5ccgx_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
					enum v4l2_mbus_pixelcode *code)
{
	cam_dbg("enum_mbus_fmt: index = %d\n", index);

	if (index >= ARRAY_SIZE(capture_fmts))
		return -EINVAL;

	*code = capture_fmts[index].code;

	return 0;
}

static int s5k5ccgx_try_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	int num_entries;
	int i;

	num_entries = ARRAY_SIZE(capture_fmts);

	cam_dbg("try_mbus_fmt: code = 0x%x,"
		" colorspace = 0x%x, num_entries = %d\n",
		fmt->code, fmt->colorspace, num_entries);

	for (i = 0; i < num_entries; i++) {
		if ((capture_fmts[i].code == fmt->code) &&
		    (capture_fmts[i].colorspace == fmt->colorspace)) {
			cam_dbg("%s: match found, returning 0\n", __func__);
			return 0;
		}
	}

	cam_err("%s: no match found, returning -EINVAL\n", __func__);
	return -EINVAL;
}

static int s5k5ccgx_enum_framesizes(struct v4l2_subdev *sd,
				  struct v4l2_frmsizeenum *fsize)
{
	struct s5k5ccgx_state *state = to_state(sd);

	/*
	* The camera interface should read this value, this is the resolution
	* at which the sensor would provide framedata to the camera i/f
	* In case of image capture,
	* this returns the default camera resolution (VGA)
	*/
	if (state->format_mode != V4L2_PIX_FMT_MODE_CAPTURE) {
		if (unlikely(state->preview.frmsize == NULL)) {
			cam_err("%s: error\n", __func__);
			return -EFAULT;
		}

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->preview.frmsize->width;
		fsize->discrete.height = state->preview.frmsize->height;
	} else {
		if (unlikely(state->capture.frmsize == NULL)) {
			cam_err("%s: error\n", __func__);
			return -EFAULT;
		}

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->capture.frmsize->width;
		fsize->discrete.height = state->capture.frmsize->height;
	}

	return 0;
}

static int s5k5ccgx_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	return 0;
}

static int s5k5ccgx_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	int err = 0;
	struct s5k5ccgx_state *state = to_state(sd);

	state->req_fps = param->parm.capture.timeperframe.denominator /
			param->parm.capture.timeperframe.numerator;

	cam_dbg("s_parm state->fps=%d, state->req_fps=%d\n",
		state->fps, state->req_fps);

	if ((state->req_fps < 0) || (state->req_fps > 30)) {
		cam_err("%s: error, invalid frame rate %d. we'll set to 30\n",
				__func__, state->req_fps);
		state->req_fps = 30;
	}

	err = s5k5ccgx_set_frame_rate(sd, state->req_fps);
	CHECK_ERR(err);

	return 0;
}

static int s5k5ccgx_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	if (!state->initialized) {
		cam_err("g_ctrl: warning, camera not initialized\n");
		return 0;
	}

	mutex_lock(&state->ctrl_lock);

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_EXIF_EXPTIME:
		if (state->sensor_mode == SENSOR_CAMERA)
			ctrl->value = state->exif.exp_time_den;
		else
			ctrl->value = 24;
		break;

	case V4L2_CID_CAMERA_EXIF_ISO:
		if (state->sensor_mode == SENSOR_CAMERA)
			ctrl->value = state->exif.iso;
		else
			ctrl->value = 100;
		break;

	case V4L2_CID_CAMERA_EXIF_FLASH:
		if (state->sensor_mode == SENSOR_CAMERA)
			ctrl->value = state->exif.flash;
		else
			s5k5ccgx_get_exif_flash(sd, (u16 *)ctrl->value);
		break;

	case V4L2_CID_CAMERA_AUTO_FOCUS_RESULT:
		if (IS_AF_SUPPORTED())
			ctrl->value = state->focus.status;
		else
			ctrl->value = AF_RESULT_SUCCESS;
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE:
	case V4L2_CID_CAMERA_EFFECT:
	case V4L2_CID_CAMERA_CONTRAST:
	case V4L2_CID_CAMERA_SATURATION:
	case V4L2_CID_CAMERA_SHARPNESS:
	case V4L2_CID_CAMERA_OBJ_TRACKING_STATUS:
	case V4L2_CID_CAMERA_SMART_AUTO_STATUS:
	default:
		cam_err("g_ctrl: warning, unknown Ctrl-ID %d (0x%x)\n",
			ctrl->id - V4L2_CID_PRIVATE_BASE,
			ctrl->id - V4L2_CID_PRIVATE_BASE);
		err = 0; /* we return no error. */
		break;
	}

	mutex_unlock(&state->ctrl_lock);

	return err;
}

static int s5k5ccgx_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	if (!state->initialized && (ctrl->id != V4L2_CID_CAMERA_SENSOR_MODE)) {
#ifdef CONFIG_MACH_PX
		if (state->sensor_mode == SENSOR_MOVIE)
			return 0;
#endif

		cam_warn("s_ctrl: warning, camera not initialized. ID %d(0x%X)\n",
			ctrl->id - V4L2_CID_PRIVATE_BASE,
			ctrl->id - V4L2_CID_PRIVATE_BASE);
		return 0;
	}

	cam_dbg("s_ctrl: ID =%d, val = %d\n",
		ctrl->id - V4L2_CID_PRIVATE_BASE, ctrl->value);

	mutex_lock(&state->ctrl_lock);

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_SENSOR_MODE:
		err = s5k5ccgx_set_sensor_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_X:
		state->focus.pos_x = ctrl->value;
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_Y:
		state->focus.pos_y = ctrl->value;
		break;

	case V4L2_CID_CAMERA_TOUCH_AF_START_STOP:
		err = s5k5ccgx_set_touch_af(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FOCUS_MODE:
		err = s5k5ccgx_set_focus_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SET_AUTO_FOCUS:
		err = s5k5ccgx_set_af(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FLASH_MODE:
		err = s5k5ccgx_set_flash_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_BRIGHTNESS:
		err = s5k5ccgx_set_exposure(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE:
		err = s5k5ccgx_set_whitebalance(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_EFFECT:
		err = s5k5ccgx_set_from_table(sd, "effects",
			state->regs->effect,
			ARRAY_SIZE(state->regs->effect), ctrl->value);
		break;

	case V4L2_CID_CAMERA_METERING:
		err = s5k5ccgx_set_from_table(sd, "metering",
			state->regs->metering,
			ARRAY_SIZE(state->regs->metering), ctrl->value);
		break;

	case V4L2_CID_CAMERA_CONTRAST:
		err = s5k5ccgx_set_from_table(sd, "contrast",
			state->regs->contrast,
			ARRAY_SIZE(state->regs->contrast), ctrl->value);
		break;

	case V4L2_CID_CAMERA_SATURATION:
		err = s5k5ccgx_set_from_table(sd, "saturation",
			state->regs->saturation,
			ARRAY_SIZE(state->regs->saturation), ctrl->value);
		break;

	case V4L2_CID_CAMERA_SHARPNESS:
		err = s5k5ccgx_set_from_table(sd, "sharpness",
			state->regs->sharpness,
			ARRAY_SIZE(state->regs->sharpness), ctrl->value);
		break;

	case V4L2_CID_CAMERA_SCENE_MODE:
		err = s5k5ccgx_set_scene_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_AE_LOCK_UNLOCK:
		err = s5k5ccgx_set_ae_lock(sd, ctrl->value, false);
		break;

	case V4L2_CID_CAMERA_AWB_LOCK_UNLOCK:
		err = s5k5ccgx_set_awb_lock(sd, ctrl->value, false);
		break;

	case V4L2_CID_CAMERA_CHECK_ESD:
		err = s5k5ccgx_check_esd(sd);
		break;

	case V4L2_CID_CAMERA_ANTI_BANDING:
		break;

	case V4L2_CID_CAMERA_ISO:
		/* we do not break. */
	case V4L2_CID_CAMERA_FRAME_RATE:
	default:
		cam_err("s_ctrl: warning, unknown Ctrl-ID %d (0x%x)\n",
			ctrl->id - V4L2_CID_PRIVATE_BASE,
			ctrl->id - V4L2_CID_PRIVATE_BASE);
		break;
	}

	mutex_unlock(&state->ctrl_lock);
	CHECK_ERR_MSG(err, "s_ctrl failed %d\n", err)

	return 0;
}

static int s5k5ccgx_pre_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
#ifdef CONFIG_MACH_PX
	struct s5k5ccgx_state *state = to_state(sd);

	if (SENSOR_MOVIE == state->sensor_mode)
		s5k5ccgx_save_ctrl(sd, ctrl);
#else
	s5k5ccgx_save_ctrl(sd, ctrl);
#endif

	return s5k5ccgx_s_ctrl(sd, ctrl);
}

static int s5k5ccgx_s_ext_ctrl(struct v4l2_subdev *sd,
			      struct v4l2_ext_control *ctrl)
{
	return 0;
}

static int s5k5ccgx_s_ext_ctrls(struct v4l2_subdev *sd,
				struct v4l2_ext_controls *ctrls)
{
	struct v4l2_ext_control *ctrl = ctrls->controls;
	int ret = 0;
	int i;

	for (i = 0; i < ctrls->count; i++, ctrl++) {
		ret = s5k5ccgx_s_ext_ctrl(sd, ctrl);
		if (unlikely(ret)) {
			ctrls->error_idx = i;
			break;
		}
	}

	return ret;
}

static int s5k5ccgx_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = 0;

	cam_info("stream mode = %d\n", enable);

	if (unlikely(!state->initialized)) {
		WARN(1, "s_stream, not initialized\n");
		return -EPERM;
	}

	switch (enable) {
	case STREAM_MODE_CAM_OFF:
		err = s5k5ccgx_control_stream(sd, STREAM_STOP);
		break;

	case STREAM_MODE_CAM_ON:
		if (state->format_mode == V4L2_PIX_FMT_MODE_CAPTURE)
			err = s5k5ccgx_start_capture(sd);
		else
			err = s5k5ccgx_start_preview(sd);
		break;

	case STREAM_MODE_MOVIE_ON:
		cam_info("movie on");
		state->recording = 1;
		if (state->flash.mode != FLASH_MODE_OFF)
			s5k5ccgx_flash_torch(sd, S5K5CCGX_FLASH_ON);
		break;

	case STREAM_MODE_MOVIE_OFF:
		cam_info("movie off");
		state->recording = 0;
		if (state->flash.on)
			s5k5ccgx_flash_torch(sd, S5K5CCGX_FLASH_OFF);
		break;

#ifdef CONFIG_VIDEO_IMPROVE_STREAMOFF
	case STREAM_MODE_WAIT_OFF:
		s5k5ccgx_wait_steamoff(sd);
		break;
#endif
	default:
		cam_err("%s: error - Invalid stream mode\n", __func__);
		break;
	}

	CHECK_ERR_MSG(err, "failed\n");

	return 0;
}

/**
 * s5k5ccgx_reset: reset the sensor device
 * @val: 0 - reset parameter.
 *      1 - power reset
 */
static int s5k5ccgx_reset(struct v4l2_subdev *sd, u32 val)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -EINVAL;

	cam_trace("EX\n");

	s5k5ccgx_return_focus(sd);

	state->initialized = 0;
	state->need_wait_streamoff = 0;
	state->runmode = RUNMODE_NOTREADY;

	if (val) {
		err = s5k5ccgx_power(sd, 0);
		CHECK_ERR(err);
		msleep_debug((val >= 2 ? 20 : 50), true);
		err = s5k5ccgx_power(sd, 1);
		CHECK_ERR(err);
	}

	state->reset_done = 1;

	return 0;
}

static int s5k5ccgx_pre_init(struct v4l2_subdev *sd, u32 val)
{
	int err = -EINVAL;

	cam_info("pre init: start (%s)\n", __DATE__);

	err = s5k5ccgx_check_sensor(sd);
	CHECK_ERR_MSG(err, "failed to indentify sensor chip\n");

	return 0;
}

static int s5k5ccgx_post_init(struct v4l2_subdev *sd, u32 val)
{
	struct s5k5ccgx_state *state = to_state(sd);
	int err = -EINVAL;

	if (state->initialized)
		return 0;

	if (state->hd_videomode) {
		cam_info("Post init: HD mode\n");
		err = S5K5CCGX_BURST_WRITE_REGS(sd, s5k5ccgx_hd_init_reg);
	} else {
		cam_info("Post init: Cam, Non-HD mode\n");
		err = S5K5CCGX_BURST_WRITE_REGS(sd, s5k5ccgx_init_reg);
	}
	CHECK_ERR_MSG(err, "failed to initialize camera device\n");

#ifdef CONFIG_MACH_P8
	s5k5ccgx_set_from_table(sd, "antibanding",
		&state->regs->antibanding, 1, 0);
#endif

	s5k5ccgx_init_parameter(sd);
	state->initialized = 1;

	if (state->reset_done) {
		state->reset_done = 0;
		s5k5ccgx_restore_parameter(sd);
	}

	err = s5k5ccgx_set_frame_rate(sd, state->req_fps);
	CHECK_ERR(err);

	return 0;
}

static int s5k5ccgx_init(struct v4l2_subdev *sd, u32 val)
{
	int err = -EINVAL;

	err = s5k5ccgx_pre_init(sd, val);
	CHECK_ERR(err);

	err = s5k5ccgx_post_init(sd, val);
	CHECK_ERR(err);

	return 0;
}

/*
 * s_config subdev ops
 * With camera device, we need to re-initialize
 * every single opening time therefor,
 * it is not necessary to be initialized on probe time.
 * except for version checking
 * NOTE: version checking is optional
 */
static int s5k5ccgx_s_config(struct v4l2_subdev *sd,
			int irq, void *platform_data)
{
	struct s5k5ccgx_state *state = to_state(sd);
	struct s5k5ccgx_platform_data *pdata = platform_data;
	int i;
#if CONFIG_LOAD_FILE
	int err = 0;
#endif

	if (!platform_data) {
		cam_err("%s: error, no platform data\n", __func__);
		return -ENODEV;
	}

	state->regs = &reg_datas;
	state->pdata = pdata;
	state->focus.support = IS_AF_SUPPORTED();
	state->flash.support = IS_FLASH_SUPPORTED();
	state->dbg_level = &dbg_level;

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	state->req_fmt.width = pdata->common.default_width;
	state->req_fmt.height = pdata->common.default_height;

	if (!pdata->common.pixelformat)
		state->req_fmt.pixelformat = DEFAULT_PIX_FMT;
	else
		state->req_fmt.pixelformat = pdata->common.pixelformat;

	if (!pdata->common.freq)
		state->freq = DEFAULT_MCLK;	/* 24MHz default */
	else
		state->freq = pdata->common.freq;

	state->preview.frmsize= state->capture.frmsize = NULL;
	state->sensor_mode = SENSOR_CAMERA;
	state->hd_videomode = 0;
	state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
	state->fps = 0;
	state->req_fps = -1;

	for (i = 0; i < ARRAY_SIZE(s5k5ccgx_ctrls); i++)
		s5k5ccgx_ctrls[i].value = s5k5ccgx_ctrls[i].default_value;

	if (s5k5ccgx_is_hwflash_on(sd))
		state->flash.ignore_flash = 1;

#if CONFIG_LOAD_FILE
	err = loadFile();
	if (unlikely(err < 0)) {
		cam_err("failed to load file ERR=%d\n", err);
		return err;
	}
#endif

	return 0;
}

static const struct v4l2_subdev_core_ops s5k5ccgx_core_ops = {
	.init = s5k5ccgx_init,	/* initializing API */
	.g_ctrl = s5k5ccgx_g_ctrl,
	.s_ctrl = s5k5ccgx_pre_s_ctrl,
	.s_ext_ctrls = s5k5ccgx_s_ext_ctrls,
	.reset = s5k5ccgx_reset,
};

static const struct v4l2_subdev_video_ops s5k5ccgx_video_ops = {
#ifdef CONFIG_MACH_PX
	.s_mbus_fmt = s5k5ccgx_s_mbus_fmt,
#else
	.s_mbus_fmt = s5k5ccgx_pre_s_mbus_fmt,
#endif
	.enum_framesizes = s5k5ccgx_enum_framesizes,
	.enum_mbus_fmt = s5k5ccgx_enum_mbus_fmt,
	.try_mbus_fmt = s5k5ccgx_try_mbus_fmt,
	.g_parm = s5k5ccgx_g_parm,
	.s_parm = s5k5ccgx_s_parm,
	.s_stream = s5k5ccgx_s_stream,
};

static const struct v4l2_subdev_ops s5k5ccgx_ops = {
	.core = &s5k5ccgx_core_ops,
	.video = &s5k5ccgx_video_ops,
};


/*
 * s5k5ccgx_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int s5k5ccgx_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct s5k5ccgx_state *state;
	int err = -EINVAL;

	state = kzalloc(sizeof(struct s5k5ccgx_state), GFP_KERNEL);
	if (unlikely(!state)) {
		dev_err(&client->dev, "probe, fail to get memory\n");
		return -ENOMEM;
	}

	mutex_init(&state->ctrl_lock);
	mutex_init(&state->af_lock);

	state->runmode = RUNMODE_NOTREADY;
	sd = &state->sd;
	strcpy(sd->name, driver_name);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &s5k5ccgx_ops);

	err = s5k5ccgx_s_config(sd, 0, client->dev.platform_data);
	if (unlikely(err)) {
		dev_err(&client->dev, "probe: fail to s_config\n");
		goto err_out;
	}

	state->workqueue = create_workqueue("cam_wq");
	if (unlikely(!state->workqueue)) {
		dev_err(&client->dev,
			"probe: fail to create workqueue\n");
		goto err_out;
	}

	if (IS_AF_SUPPORTED()) {
		INIT_WORK(&state->af_work, s5k5ccgx_af_worker);
		INIT_WORK(&state->af_win_work, s5k5ccgx_af_win_worker);
	}

#if CONFIG_DEBUG_STREAMOFF && defined(CONFIG_VIDEO_IMPROVE_STREAMOFF)
	INIT_WORK(&state->streamoff_work, s5k5ccgx_streamoff_checker);
#endif

	printk(KERN_DEBUG "%s %s: driver probed!!\n",
		dev_driver_string(&client->dev), dev_name(&client->dev));

	return 0;

err_out:
	kfree(state);
	return -ENOMEM;
}

static int s5k5ccgx_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct s5k5ccgx_state *state = to_state(sd);

#if CONFIG_DEBUG_STREAMOFF && defined(CONFIG_VIDEO_IMPROVE_STREAMOFF)
	if (atomic_read(&state->streamoff_check))
		s5k5ccgx_stop_streamoff_checker(sd);
#endif

	if (state->workqueue)
		destroy_workqueue(state->workqueue);

	/* for softlanding */
	s5k5ccgx_set_af_softlanding(sd);

	/* Check whether flash is on when unlolading driver,
	 * to preventing Market App from controlling improperly flash.
	 * It isn't necessary in case that you power flash down
	 * in power routine to turn camera off.*/
	if (unlikely(state->flash.on && !state->flash.ignore_flash))
		s5k5ccgx_flash_torch(sd, S5K5CCGX_FLASH_OFF);

	v4l2_device_unregister_subdev(sd);
	mutex_destroy(&state->ctrl_lock);
	mutex_destroy(&state->af_lock);
	kfree(state);

#if CONFIG_LOAD_FILE
	large_file ? vfree(testBuf) : kfree(testBuf);
	large_file = 0;
	testBuf = NULL;
#endif

	printk(KERN_DEBUG "%s %s: driver removed!!\n",
		dev_driver_string(&client->dev), dev_name(&client->dev));
	return 0;
}

#if CONFIG_SUPPORT_CAMSYSFS
static ssize_t camtype_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	/*pr_info("%s\n", __func__);*/
	return sprintf(buf, "%s_%s\n", "LSI", driver_name);
}
static DEVICE_ATTR(rear_camtype, S_IRUGO, camtype_show, NULL);

static ssize_t camfw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s %s\n", driver_name, driver_name);

}
static DEVICE_ATTR(rear_camfw, S_IRUGO, camfw_show, NULL);

static ssize_t cam_loglevel_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char temp_buf[60] = {0,};

	sprintf(buf, "Log Level: ");
	if (dbg_level & CAMDBG_LEVEL_TRACE) {
		sprintf(temp_buf, "trace ");
		strcat(buf, temp_buf);
	}

	if (dbg_level & CAMDBG_LEVEL_DEBUG) {
		sprintf(temp_buf, "debug ");
		strcat(buf, temp_buf);
	}

	if (dbg_level & CAMDBG_LEVEL_INFO) {
		sprintf(temp_buf, "info ");
		strcat(buf, temp_buf);
	}

	sprintf(temp_buf, "\n - warn and error level is always on\n\n");
	strcat(buf, temp_buf);

	return strlen(buf);
}

static ssize_t cam_loglevel_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	printk(KERN_DEBUG "CAM buf=%s, count=%d\n", buf, count);

	if (strstr(buf, "trace"))
		dbg_level |= CAMDBG_LEVEL_TRACE;
	else
		dbg_level &= ~CAMDBG_LEVEL_TRACE;

	if (strstr(buf, "debug"))
		dbg_level |= CAMDBG_LEVEL_DEBUG;
	else
		dbg_level &= ~CAMDBG_LEVEL_DEBUG;

	if (strstr(buf, "info"))
		dbg_level |= CAMDBG_LEVEL_INFO;

	return count;
}

static DEVICE_ATTR(loglevel, 0664, cam_loglevel_show, cam_loglevel_store);

int s5k5ccgx_create_sysfs(struct class *cls)
{
	struct device *dev = NULL;
	int err = -ENODEV;

	dev = device_create(cls, NULL, 0, NULL, "rear");
	if (IS_ERR(dev)) {
		pr_err("cam_init: failed to create device(rearcam_dev)\n");
		return -ENODEV;
	}

	err = device_create_file(dev, &dev_attr_rear_camtype);
	if (unlikely(err < 0)) {
		pr_err("cam_init: failed to create device file, %s\n",
			dev_attr_rear_camtype.attr.name);
	}

	err = device_create_file(dev, &dev_attr_rear_camfw);
	if (unlikely(err < 0)) {
		pr_err("cam_init: failed to create device file, %s\n",
			dev_attr_rear_camtype.attr.name);
	}

	err = device_create_file(dev, &dev_attr_loglevel);
	if (unlikely(err < 0)) {
		pr_err("cam_init: failed to create device file, %s\n",
			dev_attr_loglevel.attr.name);
	}

	return 0;
}
#endif /* CONFIG_SUPPORT_CAMSYSFS */

static const struct i2c_device_id s5k5ccgx_id[] = {
	{ S5K5CCGX_DRIVER_NAME, 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, s5k5ccgx_id);

static struct i2c_driver v4l2_i2c_driver = {
	.driver.name	= driver_name,
	.probe		= s5k5ccgx_probe,
	.remove		= s5k5ccgx_remove,
	.id_table	= s5k5ccgx_id,
};

static int __init v4l2_i2c_drv_init(void)
{
	pr_info("%s: %s init\n", __func__, driver_name);
#if CONFIG_SUPPORT_CAMSYSFS
	s5k5ccgx_create_sysfs(camera_class);
#endif
	return i2c_add_driver(&v4l2_i2c_driver);
}

static void __exit v4l2_i2c_drv_cleanup(void)
{
	pr_info("%s: %s exit\n", __func__, driver_name);
	i2c_del_driver(&v4l2_i2c_driver);
}

module_init(v4l2_i2c_drv_init);
module_exit(v4l2_i2c_drv_cleanup);

MODULE_DESCRIPTION("LSI S5K5CCGX 3MP SOC camera driver");
MODULE_AUTHOR("Dong-Seong Lim <dongseong.lim@samsung.com>");
MODULE_LICENSE("GPL");
