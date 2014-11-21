/* drivers/media/video/db8131a.c
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
#include "db8131a.h"

static u32 dbg_level = CAMDBG_LEVEL_DEFAULT;

static const struct db8131a_fps db8131a_framerates[] = {
	{ I_FPS_0,	FRAME_RATE_AUTO },
	{ I_FPS_7,	FRAME_RATE_7 },
	{ I_FPS_10,	10 },
	{ I_FPS_12,	12 },
	{ I_FPS_15,	FRAME_RATE_15 },
};

static const struct db8131a_framesize db8131a_preview_frmsizes[] = {
	{ PREVIEW_SZ_QVGA,	320,  240 },
	{ PREVIEW_SZ_CIF,	352,  288 },
	{ PREVIEW_SZ_VGA,	640,  480 },
};

static const struct db8131a_framesize db8131a_capture_frmsizes[] = {
	{ CAPTURE_SZ_1MP,	1280, 960 },
};

static struct db8131a_control db8131a_ctrls[] = {
	DB8131A_INIT_CONTROL(V4L2_CID_CAMERA_FLASH_MODE, \
				FLASH_MODE_OFF),

	DB8131A_INIT_CONTROL(V4L2_CID_CAMERA_BRIGHTNESS, \
				EV_DEFAULT),

	DB8131A_INIT_CONTROL(V4L2_CID_CAMERA_METERING, \
				METERING_MATRIX),

	DB8131A_INIT_CONTROL(V4L2_CID_CAMERA_WHITE_BALANCE, \
				WHITE_BALANCE_AUTO),

	DB8131A_INIT_CONTROL(V4L2_CID_CAMERA_EFFECT, \
				IMAGE_EFFECT_NONE),
};

static const struct db8131a_regs reg_datas = {
	.ev = {
		REGSET(GET_EV_INDEX(EV_MINUS_4), db8131m_bright_m4),
		REGSET(GET_EV_INDEX(EV_MINUS_3), db8131m_bright_m3),
		REGSET(GET_EV_INDEX(EV_MINUS_2), db8131m_bright_m2),
		REGSET(GET_EV_INDEX(EV_MINUS_1), db8131m_bright_m1),
		REGSET(GET_EV_INDEX(EV_DEFAULT), db8131m_bright_default),
		REGSET(GET_EV_INDEX(EV_PLUS_1), db8131m_bright_p1),
		REGSET(GET_EV_INDEX(EV_PLUS_2), db8131m_bright_p2),
		REGSET(GET_EV_INDEX(EV_PLUS_3), db8131m_bright_p3),
		REGSET(GET_EV_INDEX(EV_PLUS_4), db8131m_bright_p4),
	},
	.effect = {
		REGSET(IMAGE_EFFECT_NONE, db8131m_Effect_Normal),
		REGSET(IMAGE_EFFECT_BNW, db8131m_Effect_Mono),
		REGSET(IMAGE_EFFECT_SEPIA, db8131m_Effect_Sepia),
		REGSET(IMAGE_EFFECT_AQUA, db8131m_Effect_Aqua),
		REGSET(IMAGE_EFFECT_SKETCH, db8131m_Effect_sketch),
		REGSET(IMAGE_EFFECT_NEGATIVE, db8131m_Effect_Negative),
	},
	.white_balance = {
		REGSET(WHITE_BALANCE_AUTO, db8131m_WB_Auto),
		REGSET(WHITE_BALANCE_SUNNY, db8131m_WB_Daylight),
		REGSET(WHITE_BALANCE_CLOUDY, db8131m_WB_Cloudy),
		REGSET(WHITE_BALANCE_TUNGSTEN, db8131m_WB_Incandescent),
		REGSET(WHITE_BALANCE_FLUORESCENT, db8131m_WB_Fluorescent),
	},
	.fps = {
		REGSET(I_FPS_0, db8131m_vt_auto),
		REGSET(I_FPS_7, db8131m_vt_7fps),
		REGSET(I_FPS_10, db8131m_vt_10fps),
		REGSET(I_FPS_12, db8131m_vt_12fps),
		REGSET(I_FPS_15, db8131m_vt_15fps),
	},
	.preview_size = {
		REGSET(PREVIEW_SZ_QVGA, db8131m_size_qvga),
		REGSET(PREVIEW_SZ_CIF, db8131m_size_cif),
		REGSET(PREVIEW_SZ_VGA, db8131m_size_vga),
	},

	/* Flash */		

	/* AWB, AE Lock */

	/* AF */

	.init = REGSET_TABLE(db8131m_common),
	.init_vt = REGSET_TABLE(db8131m_vt_common),

	.stream_stop = REGSET_TABLE(db8131m_stream_stop),

	.preview_mode = REGSET_TABLE(db8131m_preview),
	.preview_hd_mode = REGSET_TABLE(db8131m_preview),
	.capture_mode = {
		REGSET(CAPTURE_SZ_VGA, db8131m_vga_capture),
		REGSET(CAPTURE_SZ_1MP, db8131m_capture),
	},
	.camcorder_on = REGSET_TABLE(db8131m_recording_60Hz_common),
#ifdef CONFIG_MACH_P8
	.antibanding = REGSET_TABLE(DB8131A_ANTIBANDING_REG),
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

static int db8131a_i2c_read_short(struct i2c_client *client,
		unsigned char subaddr, unsigned char *data)
{
	int ret;
	unsigned char buf[1];
	struct i2c_msg msg = { client->addr, 0, 1, buf };

	buf[0] = subaddr;

	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO)
		goto error;

	msg.flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO)
		goto error;

	*data = buf[0];

error:
	return ret;
}

static int db8131a_i2c_read_multi(struct i2c_client *client,
		unsigned short subaddr, unsigned long *data)
{
	unsigned char buf[4];
	struct i2c_msg msg = {client->addr, 0, 2, buf};

	int err = 0;

	if (!client->adapter)
		return -EIO;

	buf[0] = subaddr >> 8;
	buf[1] = subaddr & 0xFF;

	err = i2c_transfer(client->adapter, &msg, 1);
	if (unlikely(err < 0))
		return -EIO;

	msg.flags = I2C_M_RD;
	msg.len = 4;

	err = i2c_transfer(client->adapter, &msg, 1);
	if (unlikely(err < 0))
		return -EIO;
	/*
	 * Data comes in Little Endian in parallel mode; So there
	 * is no need for byte swapping here
	 */

	*data = *(unsigned long *)(&buf);

	return err;
}


static int db8131a_i2c_write_multi(struct i2c_client *client, u16 packet)
{
	int32_t rc = -EFAULT;
	int retry_count = 0;

	unsigned char buf[2];

	struct i2c_msg msg;
	buf[0] = (u8) (packet >> 8);
	buf[1] = (u8) (packet & 0xff);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = buf;

#if defined(CAM_I2C_DEBUG)
	printk(KERN_ERR "I2C CHIP ID=0x%x, DATA 0x%x 0x%x\n",
			client->addr, buf[0], buf[1]);
#endif
	do {
		rc = i2c_transfer(client->adapter, &msg, 1);
		if (likely(rc == 1))
			return 0;
		retry_count++;
		dev_err(&client->dev, "i2c transfer failed, retrying %x err:%d\n",
			packet, rc);
		mdelay(3);

	} while (retry_count <= 5);

	return -EIO;
}

#if CONFIG_LOAD_FILE
int db8131a_regs_table_init(void)
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

	//BUG_ON(testBuf);
	printk("CONFIG_LOAD_FILE is enable!!\n");

	fp = filp_open("/mnt/sdcard/db8131a_regs.h", O_RDONLY, 0);
	if (IS_ERR(fp)) {
		printk(KERN_ERR "failed to open /mnt/sdcard/db8131a_regs.h");
		return PTR_ERR(fp);
	}

	file_size = (size_t) fp->f_path.dentry->d_inode->i_size;
	max_size = file_size;

	//printk("file_size = %d", file_size);

	nBuf = kmalloc(file_size, GFP_KERNEL);
	if (nBuf == NULL) {
		printk(KERN_ERR "Fail to 1st get memory");
		nBuf = vmalloc(file_size);
		if (nBuf == NULL) {
			printk(KERN_ERR "ERR: nBuf Out of Memory");
			ret = -ENOMEM;
			goto error_out;
		}
		tmp_large_file = 1;
	}

	testBuf_size = sizeof(struct test) * file_size;
	if (tmp_large_file) {
		testBuf = vmalloc(testBuf_size);
		large_file = 1;
	} else {
		testBuf = kmalloc(testBuf_size, GFP_KERNEL);
		if (testBuf == NULL) {
			printk(KERN_ERR "Fail to get mem(%d bytes)", testBuf_size);
			testBuf = vmalloc(testBuf_size);
			large_file = 1;
		}
	}
	if (testBuf == NULL) {
		printk(KERN_ERR "ERR: Out of Memory");
		ret = -ENOMEM;
		goto error_out;
	}

	pos = 0;
	memset(nBuf, 0, file_size);
	memset(testBuf, 0, file_size * sizeof(struct test));

	nread = vfs_read(fp, (char __user *)nBuf, file_size, &pos);
	if (nread != file_size) {
		printk(KERN_ERR "failed to read file ret = %d", nread);
		ret = -1;
		goto error_out;
	}

	set_fs(fs);

	i = max_size;

	//printk("i = %d", i);

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

	while (i - 1) {
		if (!check && !starCheck) {
			if (testBuf[max_size - i].data == '/') {
				if (testBuf[max_size-i].nextBuf != NULL) {
					if (testBuf[max_size-i].nextBuf->data
								== '/') {
						check = 1;/* when find '//' */
						i--;
					} else if (testBuf[max_size-i].nextBuf->
								data == '*') {
						starCheck = 1;/*when'/ *' */
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
					if (testBuf[max_size-i].nextBuf->
								data == '*') {
						starCheck = 1; /*when '/ *' */
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
			if (testBuf[max_size - i].data
						== '*') {
				if (testBuf[max_size-i].nextBuf != NULL) {
					if (testBuf[max_size-i].nextBuf->
								data == '/') {
						starCheck = 0; /*when'* /' */
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

#ifdef FOR_DEBUG /* for print */
	printk("i = %d\n", i);
	nextBuf = &testBuf[0];
	while (1) {
		if (nextBuf->nextBuf == NULL)
			break;
		printk("%c", nextBuf->data);
		nextBuf = nextBuf->nextBuf;
	}
#endif

	tmp_large_file ? vfree(nBuf) : kfree(nBuf);

error_out:
	if (fp)
		filp_close(fp, current->files);
	return ret;
}

static inline int db8131a_write(struct i2c_client *client,
		u16 packet)
{
	u8 buf[2];
	int err = 0, retry_count = 5;

	struct i2c_msg msg;

	if (!client->adapter) {
		printk("ERR - can't search i2c client adapter");
		return -EIO;
	}

	buf[0] = (u8) (packet >> 8);
	buf[1] = (u8) (packet & 0xff);

	msg.addr = 0x45;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = buf;

	do {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			return 0;
		retry_count++;
		//printk("i2c transfer failed, retrying %x err:%d\n",
		       //packet, err);
		mdelay(3);

	} while (retry_count <= 5);

	return (err != 1) ? -1 : 0;
}

static int db8131a_write_regs_from_sd(struct v4l2_subdev *sd,
					const char *name)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct test *tempData = NULL;

	int ret = -EAGAIN;
	unsigned long temp;
	u16 delay = 0;
	u8 data[7];
	s32 searched = 0;
	size_t size = strlen(name);
	s32 i;

	//printk("E size = %d, string = %s", size, name);
	tempData = &testBuf[0];
	*(data + 6) = '\0';

	while (!searched) {
		searched = 1;
		for (i = 0; i < size; i++) {
			if (tempData->data != name[i]) {
				searched = 0;
				break;
			}
			tempData = tempData->nextBuf;
		}
		tempData = tempData->nextBuf;
	}

	/* structure is get..*/
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
				/* get 6 strings.*/
				data[0] = '0';
				for (i = 1; i < 6; i++) {
					data[i] = tempData->data;
					tempData = tempData->nextBuf;
				}
				ret = kstrtoul(data, 16, &temp);
				/*printk("%s\n", data);
				printk("kstrtoul data = 0x%x\n", temp);*/
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
		if ((temp & 0xFF00) == DB8131A_DELAY) {
			delay = temp & 0xFF;
			//printk("db8131 delay(%d)", delay); /*step is 10msec */
			msleep(delay);
			continue;
		}
		ret = db8131a_write(client, temp);

		/* In error circumstances */
		/* Give second shot */
		if (unlikely(ret)) {
			ret = db8131a_write(client, temp);

			/* Give it one more shot */
			if (unlikely(ret))
				ret = db8131a_write(client, temp);
			}
		}

	return 0;
}

void db8131a_regs_table_exit(void)
{
	if (testBuf == NULL)
		return;
	else {
		large_file ? vfree(testBuf) : kfree(testBuf);
		large_file = 0;
		testBuf = NULL;
	}
}

#endif

/* Write register
 * If success, return value: 0
 * If fail, return value: -EIO
 */
static int db8131a_write_regs(struct v4l2_subdev *sd,
			       const u16 *regs, int size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, err =0;
	u8 m_delay = 0;
	u16 temp_packet;

	for (i = 0; i < size; i++) {
		temp_packet = regs[i];
		if ((temp_packet & 0xFF00) == DB8131A_DELAY) {
			m_delay = temp_packet & 0xFF;
			msleep_debug(m_delay, true);
			continue;
		}
		err = db8131a_i2c_write_multi(client, temp_packet);
		if (unlikely(err < 0)) {
			cam_err("write_regs: register set failed\n");
			break;
		}
	}

	if (unlikely(err < 0))
		return -EIO;

	return 0;
}

#if 0
#define BURST_MODE_BUFFER_MAX_SIZE 2700
u8 db8131a_burstmode_buf[BURST_MODE_BUFFER_MAX_SIZE];

/* S5K5CC: */
static int db8131a_burst_write_regs(struct v4l2_subdev *sd,
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
		.buf	= db8131a_burstmode_buf,
	};

	cam_trace("E\n");

	cam_err("burst_write_regs: error, not implemented\n");
	return -ENOSYS;

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
				db8131a_burstmode_buf[idx++] = 0x0F;
				db8131a_burstmode_buf[idx++] = 0x12;
			}
			db8131a_burstmode_buf[idx++] = value >> 8;
			db8131a_burstmode_buf[idx++] = value & 0xFF;

			/* write in burstmode*/
			if (next_subaddr != 0x0F12) {
				msg.len = idx;
				err = i2c_transfer(client->adapter,
					&msg, 1) == 1 ? 0 : -EIO;
				CHECK_ERR_MSG(err, "i2c_transfer\n");
				/* cam_dbg("db8131a_sensor_burst_write,
						idx = %d\n", idx); */
				idx = 0;
			}
			break;

		case 0xFFFF:
			msleep_debug(value, true);
			break;

		default:
			idx = 0;
			/*err = db8131a_i2c_write_twobyte(client,
						subaddr, value);*/
			CHECK_ERR_MSG(err, "i2c_write_twobytes\n");
			break;
		}
	}

	return 0;
}
#endif

/* PX: */
static int db8131a_set_from_table(struct v4l2_subdev *sd,
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
	return db8131a_write_regs_from_sd(sd, table->name);
 
#else /* !CONFIG_LOAD_FILE */
	CHECK_ERR_COND_MSG(!table->reg, -EFAULT, \
		"table=%s, index=%d, reg = NULL\n", setting_name, index);

# if DEBUG_WRITE_REGS
	cam_dbg("%s: \"%s\", reg_name=%s\n", __func__,
			setting_name, table->name);
# endif /* DEBUG_WRITE_REGS */

	err = db8131a_write_regs(sd, table->reg, table->array_size);
	CHECK_ERR_MSG(err, "write regs(%s), err=%d\n", setting_name, err);

	return 0;
#endif /* CONFIG_LOAD_FILE */
}

static inline int db8131a_power(struct v4l2_subdev *sd, int on)
{
	struct db8131m_platform_data *pdata = to_state(sd)->pdata;
	int err = 0;

	cam_trace("EX\n");

	if (pdata->common.power) {
		err = pdata->common.power(on);
		CHECK_ERR_MSG(err, "failed to power(%d)\n", on);
	}
	
	return 0;
}

static inline int __used
db8131a_transit_preview_mode(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);

	return db8131a_set_from_table(sd, "preview_mode",
			&state->regs->preview_mode, 1, 0);
}

/**
 * db8131a_transit_half_mode: go to a half-release mode
 * Don't forget that half mode is not used in movie mode.
 */
static inline int __used
db8131a_transit_half_mode(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);

	/* We do not go to half release mode in movie mode */
	if (state->sensor_mode == SENSOR_MOVIE)
		return 0;

	/*
	 * Write the additional codes here
	 */
	return 0;
}

static inline int __used
db8131a_transit_capture_mode(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);

	return db8131a_set_from_table(sd, "capture_mode",
			state->regs->capture_mode,
			ARRAY_SIZE(state->regs->capture_mode),
			state->capture.frmsize->index);
}

/**
 * db8131a_transit_movie_mode: switch camera mode if needed.
 * Note that this fuction should be called from start_preview().
 */
static inline int __used
db8131a_transit_movie_mode(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);

	/* we'll go from the below modes to RUNNING or RECORDING */
	switch (state->runmode) {
	case RUNMODE_INIT:
		/* case of entering camcorder firstly */

	case RUNMODE_RUNNING_STOP:
		/* case of switching from camera to camcorder */
		if (SENSOR_MOVIE == state->sensor_mode) {
			cam_dbg("switching to camcorder mode\n");
			db8131a_set_from_table(sd, "camcorder_on",
				&state->regs->camcorder_on, 1, 0);
		}
		break;

	case RUNMODE_RECORDING_STOP:
		/* case of switching from camcorder to camera */
		if (state->sensor_mode == SENSOR_CAMERA) {
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
 * db8131a_is_hwflash_on - check whether flash device is on
 *
 * Refer to state->flash.on to check whether flash is in use in driver.
 */
static inline int db8131a_is_hwflash_on(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
	struct db8131m_platform_data *pdata = state->pdata;
	int err = 0;

	if (!IS_FLASH_SUPPORTED())
		return 0;

	if (pdata->is_flash_on)
		err = pdata->is_flash_on();
		
	return err;
}

/**
 * db8131a_flash_en - contro Flash LED
 * @mode: DB8131A_FLASH_MODE_NORMAL or DB8131A_FLASH_MODE_MOVIE
 * @onoff: DB8131A_FLASH_ON or DB8131A_FLASH_OFF
 */
static int db8131a_flash_en(struct v4l2_subdev *sd, s32 mode, s32 onoff)
{
	struct db8131a_state *state = to_state(sd);
	struct db8131m_platform_data *pdata = state->pdata;
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
 * db8131a_flash_torch - turn flash on/off as torch for preflash, recording
 * @onoff: DB8131A_FLASH_ON or DB8131A_FLASH_OFF
 *
 * This func set state->flash.on properly.
 */
static inline int db8131a_flash_torch(struct v4l2_subdev *sd, s32 onoff)
{
	struct db8131a_state *state = to_state(sd);
	int err = 0;

	if (!IS_FLASH_SUPPORTED())
		return 0;

	/* On/Off flash */
	err = db8131a_flash_en(sd, DB8131A_FLASH_MODE_MOVIE, onoff);
	if (unlikely(!err))
		state->flash.on = (onoff == DB8131A_FLASH_ON) ? 1 : 0;

	return err;
}

/**
 * db8131a_flash_oneshot - turn main flash on for capture
 * @onoff: DB8131A_FLASH_ON or DB8131A_FLASH_OFF
 *
 * Main flash is turn off automatically in some milliseconds.
 */
static inline int db8131a_flash_oneshot(struct v4l2_subdev *sd, s32 onoff)
{
	struct db8131a_state *state = to_state(sd);
	int err = 0;

	if (!IS_FLASH_SUPPORTED())
		return 0;

	/* On/Off flash */
	err = db8131a_flash_en(sd, DB8131A_FLASH_MODE_NORMAL, onoff);
	if (!err) {
		/* The flash_on here is only used for EXIF */
		state->flash.on = (onoff == DB8131A_FLASH_ON) ? 1 : 0;
	}

	return err;
}

static int db8131a_set_scene_mode(struct v4l2_subdev *sd, s32 val)
{
	struct db8131a_state *state = to_state(sd);
	int err = -ENODEV;

	cam_trace("E, value %d\n", val);

	/* if not implemented */
	cam_dbg("set_whitebalance: not implemented\n");
	return 0;

	if (state->scene_mode == val)
		return 0;

	/* when scene mode is switched, we frist have to reset */
	if (state->scene_mode != SCENE_MODE_NONE) {
		err = db8131a_set_from_table(sd, "scene_mode",
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
		db8131a_set_from_table(sd, "scene_mode",
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

static int db8131a_set_exposure(struct v4l2_subdev *sd, s32 val)
{
	struct db8131a_state *state = to_state(sd);
	int err = -EINVAL;

	if ((val < EV_MINUS_4) || (val > EV_PLUS_4)) {
		cam_err("%s: error, invalid value(%d)\n", __func__, val);
		return -EINVAL;
	}

	err = db8131a_set_from_table(sd, "brightness", state->regs->ev,
		ARRAY_SIZE(state->regs->ev), GET_EV_INDEX(val));

	state->exposure.val = val;

	return err;
}

/**
 * db8131a_set_whitebalance - set whilebalance
 * @val:
 */
static int db8131a_set_whitebalance(struct v4l2_subdev *sd, s32 val)
{
	struct db8131a_state *state = to_state(sd);
	int err = 0;

	cam_trace("E, value %d\n", val);

retry:
	switch (val) {
	case WHITE_BALANCE_AUTO:
	case WHITE_BALANCE_SUNNY:
	case WHITE_BALANCE_CLOUDY:
	case WHITE_BALANCE_TUNGSTEN:
	case WHITE_BALANCE_FLUORESCENT:
		err = db8131a_set_from_table(sd, "white balance",
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
static u32 db8131a_get_light_level(struct v4l2_subdev *sd, u32 *light_level)
{
	/*struct i2c_client *client = v4l2_get_subdevdata(sd);*/
	struct db8131a_state *state = to_state(sd);
/*	u16 val_lsb = 0;
	u16 val_msb = 0;*/
	int err = -ENODEV;

	err = db8131a_set_from_table(sd, "get_light_level",
			&state->regs->get_light_level, 1, 0);
	CHECK_ERR_MSG(err, "fail to get light level\n");

	/* cam_trace("X, light level = 0x%X", *light_level); */

	return 0;
}

/* PX: Set sensor mode */
static int db8131a_set_sensor_mode(struct v4l2_subdev *sd, s32 val)
{
	struct db8131a_state *state = to_state(sd);

	cam_dbg("sensor_mode: %d\n", val);

	state->hd_videomode = 0;

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

	case 2:	/* 720p HD video mode */
		state->sensor_mode = SENSOR_MOVIE;
		state->hd_videomode = 1;
		break;

	default:
		cam_err("%s: error, not support.(%d)\n", __func__, val);
		state->sensor_mode = SENSOR_CAMERA;
		WARN_ON(1);
		break;
	}

	return 0;
}

/* PX: Set framerate */
static int db8131a_set_frame_rate(struct v4l2_subdev *sd, s32 fps)
{
	struct db8131a_state *state = to_state(sd);
	int err = -EIO;
	int i = 0, fps_index = -1;

	if (!state->initialized || (fps < 0) || state->hd_videomode
	    || (state->scene_mode != SCENE_MODE_NONE))
		return 0;

	cam_info("set frame rate %d\n", fps);

	for (i = 0; i < ARRAY_SIZE(db8131a_framerates); i++) {
		if (fps == db8131a_framerates[i].fps) {
			fps_index = db8131a_framerates[i].index;
			state->fps = fps;
			state->req_fps = -1;
			break;
		}
	}

	if (unlikely(fps_index < 0)) {
		cam_err("set_frame_rate: not supported fps(%d)\n", fps);
		return 0;
	}

	err = db8131a_set_from_table(sd, "fps", state->regs->fps,
			ARRAY_SIZE(state->regs->fps), fps_index);
	CHECK_ERR_MSG(err, "fail to set framerate\n");

	return 0;
}

static int db8131a_set_ae_lock(struct v4l2_subdev *sd, s32 val, bool force)
{
	struct db8131a_state *state = to_state(sd);
	int err = 0;

	/* if not implemented */
	cam_dbg("set_ae_lock: not implemented\n");
	return 0;

	switch (val) {
	case AE_LOCK:
		if (state->focus.touch)
			return 0;

		err = db8131a_set_from_table(sd, "ae_lock_on",
				&state->regs->ae_lock_on, 1, 0);
		WARN_ON(state->exposure.ae_lock);
		state->exposure.ae_lock = 1;
		break;

	case AE_UNLOCK:
		if (unlikely(!force && !state->exposure.ae_lock))
			return 0;

		err = db8131a_set_from_table(sd, "ae_lock_off",
				&state->regs->ae_lock_off, 1, 0);
		state->exposure.ae_lock = 0;
		break;

	default:
		cam_err("%s: WARNING, invalid argument(%d)\n", __func__, val);
	}

	CHECK_ERR_MSG(err, "fail to lock AE(%d), err=%d\n", val, err);

	return 0;
}

static int db8131a_set_awb_lock(struct v4l2_subdev *sd, s32 val, bool force)
{
	struct db8131a_state *state = to_state(sd);
	int err = 0;

	/* if not implemented */
	cam_dbg("set_awb_lock: not implemented\n");
	return 0;

	switch (val) {
	case AWB_LOCK:
		if (state->flash.on ||
		    (state->wb.mode != WHITE_BALANCE_AUTO))
			return 0;

		err = db8131a_set_from_table(sd, "awb_lock_on",
				&state->regs->awb_lock_on, 1, 0);
		WARN_ON(state->wb.awb_lock);
		state->wb.awb_lock = 1;
		break;

	case AWB_UNLOCK:
		if (unlikely(!force && !state->wb.awb_lock))
			return 0;

		err = db8131a_set_from_table(sd, "awb_lock_off",
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
static int db8131a_set_lock(struct v4l2_subdev *sd, s32 lock, bool force)
{
	int err = -EIO;

	cam_trace("%s\n", lock ? "on" : "off");
	if (unlikely((u32)lock >= AEAWB_LOCK_MAX)) {
		cam_err("%s: error, invalid argument\n", __func__);
		return -EINVAL;
	}

	err = db8131a_set_ae_lock(sd, (lock == AEAWB_LOCK) ?
				AE_LOCK : AE_UNLOCK, force);
	if (unlikely(err))
		goto out_err;

	err = db8131a_set_awb_lock(sd, (lock == AEAWB_LOCK) ?
				AWB_LOCK : AWB_UNLOCK, force);
	if (unlikely(err))
		goto out_err;

	cam_trace("X\n");
	return 0;

out_err:
	cam_err("%s: error, failed to set lock\n", __func__);
	return err;
}


static int db8131a_set_af_softlanding(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
	int err = -EINVAL;

	cam_trace("E\n");

	if (!state->initialized || !IS_AF_SUPPORTED())
		return 0;

	err = db8131a_set_from_table(sd, "af_off",
			&state->regs->af_off, 1, 0);
	CHECK_ERR_MSG(err, "fail to set softlanding\n");

	cam_trace("X\n");
	return 0;
}

static int db8131a_return_focus(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
	int err = -EINVAL;

	if (!IS_AF_SUPPORTED())
		return 0;

	cam_trace("E\n");

	switch (state->focus.mode) {
	case FOCUS_MODE_MACRO:
		err = db8131a_set_from_table(sd, "af_macro_mode",
				&state->regs->af_macro_mode, 1, 0);
		break;

	default:
		err = db8131a_set_from_table(sd,
				"af_norma_mode",
				&state->regs->af_normal_mode, 1, 0);
		break;
	}

	CHECK_ERR(err);
	return 0;
}

static int db8131a_af_start_preflash(struct v4l2_subdev *sd)
{
	/*struct i2c_client *client = v4l2_get_subdevdata(sd);*/
	struct db8131a_state *state = to_state(sd);

	cam_trace("E\n");

	if (state->sensor_mode == SENSOR_MOVIE)
		return 0;

	cam_dbg("Start SINGLE AF, flash mode %d\n", state->flash.mode);
	cam_err("start_preflash: error, not implemented\n");
	return -ENOSYS;

	state->flash.preflash = PREFLASH_OFF;
	state->light_level = 0xFFFFFFFF;

	db8131a_get_light_level(sd, &state->light_level);

	switch (state->flash.mode) {
	case FLASH_MODE_AUTO:
		if (state->light_level >= FLASH_LOW_LIGHT_LEVEL) {
			/* flash not needed */
			break;
		}

	case FLASH_MODE_ON:
		/*
		 * Write the additional codes here
		 */
		state->flash.preflash = PREFLASH_ON;
		break;

	case FLASH_MODE_OFF:
		/*
		 * Write the additional codes here
		 */
		break;

	default:
		break;
	}

	/* Check AE-stable */
	if (state->flash.preflash == PREFLASH_ON) {
		/*
		 * Write the additional codes here
		 */
	} else if (state->focus.start == AUTO_FOCUS_OFF) {
		cam_info("af_start_preflash: AF is cancelled!\n");
		state->focus.status = AF_RESULT_CANCELLED;
	}

	/* If AF cancel, finish pre-flash process. */
	if (state->focus.status == AF_RESULT_CANCELLED) {
		if (state->flash.preflash == PREFLASH_ON) {
			/*
			 * Write the additional codes here
			 */
		}

		if (state->focus.touch)
			state->focus.touch = 0;
	}

	cam_trace("X\n");

	return 0;
}

static int db8131a_do_af(struct v4l2_subdev *sd)
{
	/*struct i2c_client *client = v4l2_get_subdevdata(sd);*/
	struct db8131a_state *state = to_state(sd);
	u16 read_value = 0;
	u32 count = 0;

	cam_trace("E\n");
	cam_err("do_af:error, not implemented\n");
	return -ENOSYS;

	/* AE, AWB Lock */
	db8131a_set_lock(sd, AEAWB_LOCK, false);

	if (state->sensor_mode == SENSOR_MOVIE) {
		db8131a_set_from_table(sd, "hd_af_start",
			&state->regs->hd_af_start, 1, 0);

		cam_info("%s : 720P Auto Focus Operation\n\n", __func__);
	} else
		db8131a_set_from_table(sd, "single_af_start",
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
	/*
	 * Write the additional codes here
	 */

	/*2nd search*/
	cam_dbg("AF 2nd search\n");
	for (count = 0; count < SECOND_AF_SEARCH_COUNT; count++) {
		if (state->focus.start == AUTO_FOCUS_OFF) {
			cam_dbg("do_af: AF is cancelled while doing(2nd)\n");
			state->focus.status = AF_RESULT_CANCELLED;
			goto check_done;
		}

		msleep_debug(AF_SEARCH_DELAY, false);

		/*
		 * Write the additional codes here
		 */
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
		/*
		 * Write the additional codes here
		 */
	} else {
		state->focus.start = AUTO_FOCUS_OFF;
		cam_dbg("do_af: Single AF finished\n");
	}

	if ((state->flash.preflash == PREFLASH_ON) &&
	    (state->sensor_mode == SENSOR_CAMERA)) {
		/*
		 * Write the additional codes here
		 */
		
		db8131a_flash_torch(sd, DB8131A_FLASH_OFF);
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

static int db8131a_set_af(struct v4l2_subdev *sd, s32 val)
{
	struct db8131a_state *state = to_state(sd);
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

static int db8131a_start_af(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
	struct db8131a_stream_time *win_stable = &state->focus.win_stable;
	u32 touch_win_delay = 0;
	s32 interval = 0;

	cam_trace("E\n");

	mutex_lock(&state->af_lock);
	state->af_pid = task_pid_nr(current);
	state->focus.reset_done = 0;

	if (state->sensor_mode == SENSOR_CAMERA) {
		state->one_frame_delay_ms = ONE_FRAME_DELAY_MS_NORMAL;
		touch_win_delay = ONE_FRAME_DELAY_MS_LOW;
		if (db8131a_af_start_preflash(sd))
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

	db8131a_do_af(sd);

out:
	state->af_pid = 0;
	mutex_unlock(&state->af_lock);
	cam_trace("X\n");
	return 0;
}

static int db8131a_stop_af(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
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

		err = db8131a_set_lock(sd, AEAWB_UNLOCK, false);
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
		cam_warn("stop_af: warning, unnecessary calling. AF status=%d\n",
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
 * db8131a_check_wait_af_complete: check af and wait
 * @cancel: if true, force to cancel AF.
 *
 * check whether AF is running and then wait to be fininshed.
 */
static int db8131a_check_wait_af_complete(struct v4l2_subdev *sd, bool cancel)
{
	struct db8131a_state *state = to_state(sd);

	if (check_af_pid(sd)) {
		cam_err("check_wait_af_complete: I should not be here\n");
		return -EPERM;
	}

	if (AF_RESULT_DOING == state->focus.status) {
		if (cancel)
			db8131a_set_af(sd, AUTO_FOCUS_OFF);

		mutex_lock(&state->af_lock);
		mutex_unlock(&state->af_lock);
	}

	return 0;
}

static void db8131a_af_worker(struct work_struct *work)
{
	db8131a_start_af(&TO_STATE(work, af_work)->sd);
}

static int db8131a_set_focus_mode(struct v4l2_subdev *sd, s32 val)
{
	struct db8131a_state *state = to_state(sd);
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

		db8131a_stop_af(sd);
		if (state->focus.reset_done) {
			cam_dbg("AF is already cancelled fully\n");
			goto out;
		}
		state->focus.reset_done = 1;
	}

	mutex_lock(&state->af_lock);

	switch (focus_mode) {
	case FOCUS_MODE_MACRO:
		err = db8131a_set_from_table(sd, "af_macro_mode",
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
		err = db8131a_set_from_table(sd, "af_norma_mode",
				&state->regs->af_normal_mode, 1, 0);
		if (unlikely(err)) {
			cam_err("set_focus_mode: error,"
				" fail to af_norma_mode (%d)\n", err);
			goto err_out;
		}

		state->focus.mode = focus_mode;
		break;

	case FOCUS_MODE_FACEDETECT:
	case FOCUS_MODE_CONTINOUS:
	case FOCUS_MODE_TOUCH:
		break;

	default:
		cam_err("set_focus_mode: error, invalid val(0x%X)\n:", val);
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

static int db8131a_set_af_window(struct v4l2_subdev *sd)
{
	int err = -EIO;
	struct db8131a_state *state = to_state(sd);

	cam_trace("E\n");

	mutex_lock(&state->af_lock);
	state->af_pid = task_pid_nr(current);

	/*
	 * Write the additional codes here
	 */

	do_gettimeofday(&state->focus.win_stable.before_time);
	state->focus.touch = 1;
	state->af_pid = 0;
	mutex_unlock(&state->af_lock);

	CHECK_ERR(err);
	cam_info("AF window position completed.\n");

	cam_trace("X\n");
	return 0;
}

static void db8131a_af_win_worker(struct work_struct *work)
{
	db8131a_set_af_window(&TO_STATE(work, af_win_work)->sd);
}

static int db8131a_set_touch_af(struct v4l2_subdev *sd, s32 val)
{
	struct db8131a_state *state = to_state(sd);
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

static const struct db8131a_framesize * __used
db8131a_get_framesize_i(const struct db8131a_framesize *frmsizes,
				u32 frmsize_count, u32 index)
{
	int i = 0;

	for (i = 0; i < frmsize_count; i++) {
		if (frmsizes[i].index == index)
			return &frmsizes[i];
	}

	return NULL;
}

static const struct db8131a_framesize * __used
db8131a_get_framesize_sz(const struct db8131a_framesize *frmsizes,
				u32 frmsize_count, u32 width, u32 height)
{
	int i;

	for (i = 0; i < frmsize_count; i++) {
		if ((frmsizes[i].width == width) && (frmsizes[i].height == height))
			return &frmsizes[i];
	}

	return NULL;
}

static const struct db8131a_framesize * __used
db8131a_get_framesize_ratio(const struct db8131a_framesize *frmsizes,
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
static void db8131a_set_framesize(struct v4l2_subdev *sd,
				const struct db8131a_framesize *frmsizes,
				u32 num_frmsize, bool preview)
{
	struct db8131a_state *state = to_state(sd);
	const struct db8131a_framesize **found_frmsize = NULL;
	u32 width = state->req_fmt.width;
	u32 height = state->req_fmt.height;

	cam_dbg("set_framesize: Requested Res %dx%d\n", width, height);

	found_frmsize = /*(const struct db8131a_framesize **) */
			(preview ? &state->preview.frmsize: &state->capture.frmsize);

	*found_frmsize = db8131a_get_framesize_ratio(
				frmsizes, num_frmsize, width, height);
	if (*found_frmsize == NULL) {
		cam_err("%s: error, invalid frame size %dx%d\n",
			__func__, width, height);
		*found_frmsize = preview ?
			db8131a_get_framesize_i(frmsizes, num_frmsize,
				PREVIEW_SZ_VGA):
			db8131a_get_framesize_i(frmsizes, num_frmsize,
					CAPTURE_SZ_1MP);
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
static int db8131a_start_streamoff_checker(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
	int err = 0;

	cam_trace("EX");
	atomic_set(&state->streamoff_check, true);

	err = queue_work(state->workqueue, &state->streamoff_work);
	if (unlikely(!err))
		cam_info("streamoff_checker is already operating!\n");

	return 0;
}

static void db8131a_stop_streamoff_checker(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);

	cam_trace("EX");

	atomic_set(&state->streamoff_check, false);

	if (flush_work(&state->streamoff_work))
		cam_dbg("wait... streamoff_checker stopped\n");
}

static void db8131a_streamoff_checker(struct work_struct *work)
{
	struct db8131a_state *state = TO_STATE(work, streamoff_work);
	struct timeval start_time, end_time;
	s32 elapsed_msec = 0;
	u32 count;
	u16 val = 0;


	if (!atomic_read(&state->streamoff_check))
		return;

	do_gettimeofday(&start_time);

	for (count = 1; count != 0; count++) {

		/* Write the the codes here that can check 
		 * whether streamoff is complete. */

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
static int db8131a_wait_steamoff(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_platform_data *pdata = state->pdata;
	struct db8131a_stream_time *stream_time = &state->stream_time;
	s32 elapsed_msec = 0;
	int count, err = 0;
	u16 val = 0;

	if (unlikely(!(pdata->common.is_mipi & state->need_wait_streamoff)))
		return 0;

	cam_trace("E\n");

	do_gettimeofday(&stream_time->curr_time);

	elapsed_msec = GET_ELAPSED_TIME(stream_time->curr_time, \
			stream_time->before_time) / 1000;

	for (count = 0; count < STREAMOFF_CHK_COUNT; count++) {
		/* Write the the codes here that can check 
		 * whether streamoff is complete. */
		cam_err("wait_steamoff v2:error, not implemented\n");
		return -ENOSYS;

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
	db8131a_stop_streamoff_checker(sd);
#endif

	state->need_wait_streamoff = 0;

	return 0;
}

#else
static int db8131a_wait_steamoff(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
	struct db8131m_platform_data *pdata = state->pdata;
	struct db8131a_stream_time *stream_time = &state->stream_time;
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
	db8131a_stop_streamoff_checker(sd);
#endif

	state->need_wait_streamoff = 0;

	return 0;
}
#endif /* CONFIG_SUPPORT_WAIT_STREAMOFF_V2 */

static int db8131a_control_stream(struct v4l2_subdev *sd, u32 cmd)
{
	struct db8131a_state *state = to_state(sd);
	int err = -EINVAL;

	if (!state->pdata->common.is_mipi)
		return 0;

	if (unlikely(cmd != STREAM_STOP))
		return 0;

	cam_info("STREAM STOP!!\n");
	err = db8131a_set_from_table(sd, "stream_stop",
			&state->regs->stream_stop, 1, 0);
	CHECK_ERR_MSG(err, "failed to stop stream\n");

#ifdef CONFIG_VIDEO_IMPROVE_STREAMOFF
	do_gettimeofday(&state->stream_time.before_time);
	state->need_wait_streamoff = 1;

#if CONFIG_DEBUG_STREAMOFF
	db8131a_start_streamoff_checker(sd);
#endif
#else
	msleep_debug(state->pdata->common.streamoff_delay, true);
#endif /* CONFIG_VIDEO_IMPROVE_STREAMOFF */

	switch (state->runmode) {
	case RUNMODE_CAPTURING:
		cam_dbg("Capture Stop!\n");
		state->runmode = RUNMODE_CAPTURING_STOP;
		state->capture.ready = 0;

		/* We turn flash off if one-shot flash is still on. */
		if (db8131a_is_hwflash_on(sd))
			db8131a_flash_oneshot(sd, DB8131A_FLASH_OFF);
		else
			state->flash.on = 0;
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
static int db8131a_set_flash_mode(struct v4l2_subdev *sd, s32 val)
{
	struct db8131a_state *state = to_state(sd);

	/* movie flash mode should be set when recording is started */
	/* if (state->sensor_mode == SENSOR_MOVIE && !state->recording)
		return 0;*/

	if (!IS_FLASH_SUPPORTED() || (state->flash.mode == val)) {
		cam_dbg("not support or the same flash mode=%d\n", val);
		return 0;
	}

	if (val == FLASH_MODE_TORCH)
		db8131a_flash_torch(sd, DB8131A_FLASH_ON);

	if ((state->flash.mode == FLASH_MODE_TORCH)
	    && (val == FLASH_MODE_OFF))
		db8131a_flash_torch(sd, DB8131A_FLASH_OFF);

	state->flash.mode = val;

	cam_dbg("Flash mode = %d\n", val);
	return 0;
}

static int db8131a_check_esd(struct v4l2_subdev *sd)
{
	/* struct db8131a_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL;*/
	u16 read_value = 0;

	cam_err("check_esd: error, not implemented\n");
	return 0;

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
static inline int db8131a_get_exif_iso(struct v4l2_subdev *sd, u16 *iso)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 lsb, msb;
	u16 coarse_time, line_length, exposureValue;

	db8131a_i2c_write_multi(client, 0xFFD0);
	db8131a_i2c_read_short(client, 0x06, &msb);
	db8131a_i2c_read_short(client, 0x07, &lsb);
	coarse_time = (msb << 8) + lsb ;

	db8131a_i2c_read_short(client, 0x0A, &msb);
	db8131a_i2c_read_short(client, 0x0B, &lsb);
	line_length = (msb << 8) + lsb ;

	exposureValue = (coarse_time * line_length) / 2400 ;

	if (exposureValue >= 1000)
		*iso = 400;
	else if (exposureValue >= 500)
		*iso = 200;
	else if (exposureValue >= 333)
		*iso = 100;
	else
		*iso = 50;

	cam_dbg("ISO=%d\n", *iso);

	return 0;
}

/* PX: Set ISO */
static int __used db8131a_set_iso(struct v4l2_subdev *sd, s32 val)
{
	struct db8131a_state *state = to_state(sd);
	int err = -EINVAL;

retry:
	switch (val) {
	case ISO_AUTO:
	case ISO_50:
	case ISO_100:
	case ISO_200:
	case ISO_400:
		err = db8131a_set_from_table(sd, "iso",
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
static inline int db8131a_get_exif_exptime(struct v4l2_subdev *sd,
						u32 *exp_time)
{
	/*struct db8131a_state *state = to_state(sd);*/
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 lsb, msb;
	u16 coarse_time, line_length;

	db8131a_i2c_write_multi(client, 0xFFD0);
	db8131a_i2c_read_short(client, 0x06, &msb);
	db8131a_i2c_read_short(client, 0x07, &lsb);
	coarse_time = (msb << 8) + lsb ;

	db8131a_i2c_read_short(client, 0x0A, &msb);
	db8131a_i2c_read_short(client, 0x0B, &lsb);
	line_length = (msb << 8) + lsb ;

	*exp_time = 24000000 / (coarse_time * line_length);

	return 0;
}

static inline void db8131a_get_exif_flash(struct v4l2_subdev *sd,
					u16 *flash)
{
	struct db8131a_state *state = to_state(sd);

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
static int db8131a_get_exif(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
	u32 exposure_time = 0;

	/* exposure time */
	state->exif.exp_time_den = 0;
	db8131a_get_exif_exptime(sd, &exposure_time);
	state->exif.exp_time_den = (u16)exposure_time;

	/* iso */
	state->exif.iso = 0;
	db8131a_get_exif_iso(sd, &state->exif.iso);

	/* flash */
	db8131a_get_exif_flash(sd, &state->exif.flash);

	cam_dbg("EXIF: ex_time_den=%d, iso=%d, flash=0x%02X\n",
		state->exif.exp_time_den, state->exif.iso, state->exif.flash);

	return 0;
}

static inline int db8131a_check_cap_sz_except(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
	int err = -EINVAL;

	cam_err("capture_size_exception: warning, reconfiguring...\n\n");

	switch (state->wide_cmd) {
	case WIDE_REQ_CHANGE:
		cam_info("%s: Wide Capture setting\n", __func__);
		err = db8131a_set_from_table(sd, "change_wide_cap",
			&state->regs->change_wide_cap, 1, 0);
		break;

	case WIDE_REQ_RESTORE:
		cam_info("%s: Restore capture setting\n", __func__);
		err = db8131a_set_from_table(sd, "restore_capture",
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

static int db8131a_set_preview_size(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
	int err = -EINVAL;

	if (!state->preview.update_frmsize)
		return 0;

	cam_trace("E\n");

	err = db8131a_set_from_table(sd, "preview_size",
			state->regs->preview_size,
			ARRAY_SIZE(state->regs->preview_size),
			state->preview.frmsize->index);
	CHECK_ERR(err);

	state->preview.update_frmsize = 0;

	return 0;
}

static int db8131a_set_capture_size(struct v4l2_subdev *sd)
{
	/*struct db8131a_state *state = to_state(sd);*/
	int err = 0;

	return err;
}

static int db8131a_start_preview(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
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
	db8131a_transit_movie_mode(sd);

	err = db8131a_set_preview_size(sd);
	CHECK_ERR_MSG(err, "failed to set preview size(%d)\n", err);

	/* Transit preview mode */
	err = db8131a_transit_preview_mode(sd);
	CHECK_ERR_MSG(err, "preview_mode (%d)\n", err);

	state->runmode = (state->sensor_mode == SENSOR_CAMERA) ?
			RUNMODE_RUNNING : RUNMODE_RECORDING;

	return 0;
}

/* PX: Start capture */
static int db8131a_start_capture(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);
	int err = -ENODEV;
#if CONFIG_SUPPORT_FLASH
	u32 light_level = 0xFFFFFFFF;
#endif

	/* Set capture size */
	err = db8131a_set_capture_size(sd);
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
			db8131a_get_light_level(sd, &state->light_level);
			if (light_level >= FLASH_LOW_LIGHT_LEVEL)
				break;
		} else if (state->flash.preflash == PREFLASH_OFF)
			break;
		/* We do not break. */

	case FLASH_MODE_ON:
		db8131a_flash_oneshot(sd, DB8131A_FLASH_ON);
		/* We here don't need to set state->flash.on to 1 */

		/*
		 * Write the additional codes here
		 */
		break;

	case FLASH_MODE_OFF:
	default:
		break;
	}
#endif /* CONFIG_SUPPORT_FLASH */

	/* Transit to capture mode */
	err = db8131a_transit_capture_mode(sd);
	CHECK_ERR_MSG(err, "fail to capture_mode (%d)\n", err);

	state->runmode = RUNMODE_CAPTURING;

	db8131a_get_exif(sd);

	return 0;
}

static void db8131a_init_parameter(struct v4l2_subdev *sd)
{
	struct db8131a_state *state = to_state(sd);

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

static int db8131a_check_sensor(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 value = 0, value_msb = 0, value_lsb = 0;
	u16 chip_id;

	/* Check chip id */
	db8131a_i2c_write_multi(client, 0xFFD0);
	db8131a_i2c_read_short(client, 0x00, &value_msb);
	db8131a_i2c_read_short(client, 0x01, &value_lsb);
	chip_id = (value_msb << 8) | value_lsb;
	if (unlikely(chip_id != DB8131A_CHIP_ID)) {
		cam_err("Sensor ChipID: unknown ChipID %04X\n", chip_id);
		return -ENODEV;
	}

	/* Check chip revision
	 * 0x06: new revision,  0x04: old revision */
	db8131a_i2c_write_multi(client, 0xFF02);
	db8131a_i2c_read_short(client, 0x09, &value);
	if (unlikely(value != DB8131A_CHIP_REV)) {
#ifdef CONFIG_MACH_ZEST
		/* We admit old sensor to be no problem temporarily */
		if (DB8131A_CHIP_REV_OLD == value) {
			cam_info("Sensor Rev: old rev %02X. check it\n", value);
			goto out;
		}
#endif
		cam_err("Sensor Rev: unkown Rev %02X\n", value);
		return -ENODEV;
	}

#ifdef CONFIG_MACH_ZEST
out:
#endif
	cam_dbg("Sensor chip indentification: Success");
	return 0;
}

static int db8131a_s_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct db8131a_state *state = to_state(sd);
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

		db8131a_set_framesize(sd, db8131a_preview_frmsizes,
			ARRAY_SIZE(db8131a_preview_frmsizes), true);

		/* preview.frmsize cannot absolutely go to null */
		if (state->preview.frmsize->index != prev_index)
			state->preview.update_frmsize = 1;
	} else {
		db8131a_set_framesize(sd, db8131a_capture_frmsizes,
			ARRAY_SIZE(db8131a_capture_frmsizes), false);

		/* For maket app.
		 * Samsung camera app does not use unmatched ratio.*/
		if (unlikely(!state->preview.frmsize))
			cam_warn("warning, capture without preview\n");
		else if (unlikely(FRAMESIZE_RATIO(state->preview.frmsize)
		    != FRAMESIZE_RATIO(state->capture.frmsize)))
			cam_warn("warning, preview, capture ratio not matched\n\n");
	}

	return 0;
}

static int db8131a_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
					enum v4l2_mbus_pixelcode *code)
{
	cam_dbg("enum_mbus_fmt: index = %d\n", index);

	if (index >= ARRAY_SIZE(capture_fmts))
		return -EINVAL;

	*code = capture_fmts[index].code;

	return 0;
}

static int db8131a_try_mbus_fmt(struct v4l2_subdev *sd,
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

static int db8131a_enum_framesizes(struct v4l2_subdev *sd,
				  struct v4l2_frmsizeenum *fsize)
{
	struct db8131a_state *state = to_state(sd);

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

static int db8131a_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	return 0;
}

static int db8131a_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	int err = 0;
	struct db8131a_state *state = to_state(sd);

	state->req_fps = param->parm.capture.timeperframe.denominator /
			param->parm.capture.timeperframe.numerator;

	cam_dbg("s_parm state->fps=%d, state->req_fps=%d\n",
		state->fps, state->req_fps);

	if ((state->req_fps < 0) || (state->req_fps > 30)) {
		cam_err("%s: error, invalid frame rate %d. we'll set to 30\n",
				__func__, state->req_fps);
		state->req_fps = 30;
	}

	err = db8131a_set_frame_rate(sd, state->req_fps);
	CHECK_ERR(err);

	return 0;
}

static int db8131a_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct db8131a_state *state = to_state(sd);
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
			db8131a_get_exif_flash(sd, (u16 *)ctrl->value);
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

static int db8131a_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct db8131a_state *state = to_state(sd);
	int err = 0;

	if (!state->initialized && (ctrl->id != V4L2_CID_CAMERA_SENSOR_MODE)) {
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
		err = db8131a_set_sensor_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_X:
		state->focus.pos_x = ctrl->value;
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_Y:
		state->focus.pos_y = ctrl->value;
		break;

	case V4L2_CID_CAMERA_TOUCH_AF_START_STOP:
		err = db8131a_set_touch_af(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FOCUS_MODE:
		err = db8131a_set_focus_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SET_AUTO_FOCUS:
		err = db8131a_set_af(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FLASH_MODE:
		err = db8131a_set_flash_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_BRIGHTNESS:
		err = db8131a_set_exposure(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE:
		err = db8131a_set_whitebalance(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_EFFECT:
		err = db8131a_set_from_table(sd, "effects",
				state->regs->effect,
				ARRAY_SIZE(state->regs->effect), 
				ctrl->value);		
		break;

	case V4L2_CID_CAMERA_METERING:
		break;

	case V4L2_CID_CAMERA_CONTRAST:
		break;

	case V4L2_CID_CAMERA_SATURATION:
		break;

	case V4L2_CID_CAMERA_SHARPNESS:
		break;

	case V4L2_CID_CAMERA_SCENE_MODE:
		err = db8131a_set_scene_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_AE_LOCK_UNLOCK:
		err = db8131a_set_ae_lock(sd, ctrl->value, false);
		break;

	case V4L2_CID_CAMERA_AWB_LOCK_UNLOCK:
		err = db8131a_set_awb_lock(sd, ctrl->value, false);
		break;

	case V4L2_CID_CAMERA_CHECK_ESD:
		err = db8131a_check_esd(sd);
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

static int db8131a_s_ext_ctrl(struct v4l2_subdev *sd,
			      struct v4l2_ext_control *ctrl)
{
	return 0;
}

static int db8131a_s_ext_ctrls(struct v4l2_subdev *sd,
				struct v4l2_ext_controls *ctrls)
{
	struct v4l2_ext_control *ctrl = ctrls->controls;
	int ret = 0;
	int i;

	for (i = 0; i < ctrls->count; i++, ctrl++) {
		ret = db8131a_s_ext_ctrl(sd, ctrl);
		if (unlikely(ret)) {
			ctrls->error_idx = i;
			break;
		}
	}

	return ret;
}

static int db8131a_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct db8131a_state *state = to_state(sd);
	int err = 0;

	cam_info("stream mode = %d\n", enable);

	if (unlikely(!state->initialized)) {
		WARN(1, "s_stream, not initialized\n");
		return -EPERM;
	}

	switch (enable) {
	case STREAM_MODE_CAM_OFF:
		err = db8131a_control_stream(sd, STREAM_STOP);
		break;

	case STREAM_MODE_CAM_ON:
		if (state->format_mode == V4L2_PIX_FMT_MODE_CAPTURE)
			err = db8131a_start_capture(sd);
		else
			err = db8131a_start_preview(sd);
		break;

	case STREAM_MODE_MOVIE_ON:
		cam_info("movie on");
		state->recording = 1;
		if (state->flash.mode != FLASH_MODE_OFF)
			db8131a_flash_torch(sd, DB8131A_FLASH_ON);
		break;

	case STREAM_MODE_MOVIE_OFF:
		cam_info("movie off");
		state->recording = 0;
		if (state->flash.on)
			db8131a_flash_torch(sd, DB8131A_FLASH_OFF);
		break;

#ifdef CONFIG_VIDEO_IMPROVE_STREAMOFF
	case STREAM_MODE_WAIT_OFF:
		db8131a_wait_steamoff(sd);
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
 * db8131a_reset: reset the sensor device
 * @val: 0 - reset parameter.
 *      1 - power reset
 */
static int db8131a_reset(struct v4l2_subdev *sd, u32 val)
{
	struct db8131a_state *state = to_state(sd);
	int err = -EINVAL;

	cam_trace("EX\n");

	db8131a_return_focus(sd);

	state->initialized = 0;
	state->need_wait_streamoff = 0;
	state->runmode = RUNMODE_NOTREADY;

	if (val) {
		err = db8131a_power(sd, 0);
		CHECK_ERR(err);
		msleep_debug(50, true);
		err = db8131a_power(sd, 1);
		CHECK_ERR(err);
	}

	return 0;
}

static int db8131a_init(struct v4l2_subdev *sd, u32 val)
{
	struct db8131a_state *state = to_state(sd);
	int err = -EINVAL;

	cam_info("init(new): start (%s)\n", __DATE__);

	err = db8131a_check_sensor(sd);
	CHECK_ERR_MSG(err, "failed to indentify sensor chip\n");

	if (state->vt_mode) {
		cam_info("init: VT mode\n");
		err = db8131a_set_from_table(sd, "init_reg",
				&state->regs->init_vt, 1, 0);
	} else {
		cam_info("init: Cam mode\n");
		err = db8131a_set_from_table(sd, "init_reg",
				&state->regs->init, 1, 0);
	}
	CHECK_ERR_MSG(err, "failed to initialize camera device\n");

#ifdef CONFIG_MACH_P8
	db8131a_set_from_table(sd, "antibanding",
		&state->regs->antibanding, 1, 0);
#endif

	db8131a_init_parameter(sd);
	state->initialized = 1;

	err = db8131a_set_frame_rate(sd, state->req_fps);
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
static int db8131a_s_config(struct v4l2_subdev *sd,
			int irq, void *platform_data)
{
	struct db8131a_state *state = to_state(sd);
	struct db8131m_platform_data *pdata = platform_data;
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

	for (i = 0; i < ARRAY_SIZE(db8131a_ctrls); i++)
		db8131a_ctrls[i].value = db8131a_ctrls[i].default_value;

	if (db8131a_is_hwflash_on(sd))
		state->flash.ignore_flash = 1;

#if CONFIG_LOAD_FILE
	err = db8131a_regs_table_init();
	CHECK_ERR(err);
#endif

	return 0;
}

static const struct v4l2_subdev_core_ops db8131a_core_ops = {
	.init = db8131a_init,	/* initializing API */
	.g_ctrl = db8131a_g_ctrl,
	.s_ctrl = db8131a_s_ctrl,
	.s_ext_ctrls = db8131a_s_ext_ctrls,
	.reset = db8131a_reset,
};

static const struct v4l2_subdev_video_ops db8131a_video_ops = {
	.s_mbus_fmt = db8131a_s_mbus_fmt,
	.enum_framesizes = db8131a_enum_framesizes,
	.enum_mbus_fmt = db8131a_enum_mbus_fmt,
	.try_mbus_fmt = db8131a_try_mbus_fmt,
	.g_parm = db8131a_g_parm,
	.s_parm = db8131a_s_parm,
	.s_stream = db8131a_s_stream,
};

static const struct v4l2_subdev_ops db8131a_ops = {
	.core = &db8131a_core_ops,
	.video = &db8131a_video_ops,
};


/*
 * db8131a_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int db8131a_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct db8131a_state *state;
	int err = -EINVAL;

	state = kzalloc(sizeof(struct db8131a_state), GFP_KERNEL);
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
	v4l2_i2c_subdev_init(sd, client, &db8131a_ops);

	err = db8131a_s_config(sd, 0, client->dev.platform_data);
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
		INIT_WORK(&state->af_work, db8131a_af_worker);
		INIT_WORK(&state->af_win_work, db8131a_af_win_worker);
	}

#if CONFIG_DEBUG_STREAMOFF && defined(CONFIG_VIDEO_IMPROVE_STREAMOFF)
	INIT_WORK(&state->streamoff_work, db8131a_streamoff_checker);
#endif

	printk(KERN_DEBUG "%s %s: driver probed!!\n",
		dev_driver_string(&client->dev), dev_name(&client->dev));

	return 0;

err_out:
	kfree(state);
	return -ENOMEM;
}

static int db8131a_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct db8131a_state *state = to_state(sd);

#if CONFIG_DEBUG_STREAMOFF && defined(CONFIG_VIDEO_IMPROVE_STREAMOFF)
	if (atomic_read(&state->streamoff_check))
		db8131a_stop_streamoff_checker(sd);
#endif

	if (state->workqueue)
		destroy_workqueue(state->workqueue);

	/* for softlanding */
	db8131a_set_af_softlanding(sd);

	/* Check whether flash is on when unlolading driver,
	 * to preventing Market App from controlling improperly flash.
	 * It isn't necessary in case that you power flash down
	 * in power routine to turn camera off.*/
	if (unlikely(state->flash.on && !state->flash.ignore_flash))
		db8131a_flash_torch(sd, DB8131A_FLASH_OFF);

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
	return sprintf(buf, "%s_%s\n", "DB", driver_name);
}
static DEVICE_ATTR(front_camtype, S_IRUGO, camtype_show, NULL);

static ssize_t camfw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s %s\n", driver_name, driver_name);

}
static DEVICE_ATTR(front_camfw, S_IRUGO, camfw_show, NULL);

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

int db8131a_create_sysfs(struct class *cls)
{
	struct device *dev = NULL;
	int err = -ENODEV;

	dev = device_create(cls, NULL, 0, NULL, "front");
	if (IS_ERR(dev)) {
		pr_err("cam_init: failed to create device(frontcam_dev)\n");
		return -ENODEV;
	}

	err = device_create_file(dev, &dev_attr_front_camtype);
	if (unlikely(err < 0)) {
		pr_err("cam_init: failed to create device file, %s\n",
			dev_attr_front_camtype.attr.name);
	}

	err = device_create_file(dev, &dev_attr_front_camfw);
	if (unlikely(err < 0)) {
		pr_err("cam_init: failed to create device file, %s\n",
			dev_attr_front_camtype.attr.name);
	}

	err = device_create_file(dev, &dev_attr_loglevel);
	if (unlikely(err < 0)) {
		pr_err("cam_init: failed to create device file, %s\n",
			dev_attr_loglevel.attr.name);
	}

	return 0;
}
#endif /* CONFIG_SUPPORT_CAMSYSFS */

static const struct i2c_device_id db8131a_id[] = {
	{ DB8131A_DRIVER_NAME, 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, db8131a_id);

static struct i2c_driver v4l2_i2c_driver = {
	.driver.name	= driver_name,
	.probe		= db8131a_probe,
	.remove		= db8131a_remove,
	.id_table	= db8131a_id,
};

static int __init v4l2_i2c_drv_init(void)
{
	pr_info("%s: %s init\n", __func__, driver_name);
#if CONFIG_SUPPORT_CAMSYSFS
	db8131a_create_sysfs(camera_class);
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

MODULE_DESCRIPTION("LSI DB8131A 3MP SOC camera driver");
MODULE_AUTHOR("Dong-Seong Lim <dongseong.lim@samsung.com>");
MODULE_LICENSE("GPL");
