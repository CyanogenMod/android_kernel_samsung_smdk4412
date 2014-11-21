/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "db8131m.h"

static u32 dbg_level = CAMDBG_LEVEL_DEFAULT;

struct db8131m_enum_framesize db8131m_framesize_list[] = {
	{ DB8131M_PREVIEW_QCIF, 176, 144 },
	{ DB8131M_PREVIEW_QVGA, 320, 240 },
	{ DB8131M_PREVIEW_CIF, 352, 288 },
	{ DB8131M_PREVIEW_VGA, 640, 480 }
};

struct db8131m_enum_framesize db8131m_capture_framesize_list[] = {
	{ DB8131M_CAPTURE_VGA, 640, 480 },
	{ DB8131M_CAPTURE_1MP, 1280, 960 }
};

#if CONFIG_LOAD_FILE
struct db8131m_regset_table {
	const u16 *  regset;
	int			array_size;
	char		*name;
};

#define DB8131M_REGSET_TABLE_ELEMENT(x, y)		\
	[(x)] = {					\
		.regset		= (y),			\
		.array_size	= ARRAY_SIZE((y)),	\
		.name		= #y,			\
	}
#else
struct db8131m_regset_table {
	const u16 *  regset;
	int			array_size;
};

#define DB8131M_REGSET_TABLE_ELEMENT(x, y)		\
	[(x)] = {					\
		.regset		= (y),			\
		.array_size	= ARRAY_SIZE((y)),	\
	}
#endif

/*static struct db8131m_regset_table brightness_vt[] = {
	DB8131M_REGSET_TABLE_ELEMENT(GET_EV_INDEX(EV_MINUS_4), db8131m_ev_vt_m4),
	DB8131M_REGSET_TABLE_ELEMENT(GET_EV_INDEX(EV_MINUS_3), db8131m_ev_vt_m3),
	DB8131M_REGSET_TABLE_ELEMENT(GET_EV_INDEX(EV_MINUS_2), db8131m_ev_vt_m2),
	DB8131M_REGSET_TABLE_ELEMENT(GET_EV_INDEX(EV_MINUS_1), db8131m_ev_vt_m1),
	DB8131M_REGSET_TABLE_ELEMENT(GET_EV_INDEX(EV_DEFAULT), db8131m_ev_vt_default),
	DB8131M_REGSET_TABLE_ELEMENT(GET_EV_INDEX(EV_PLUS_1), db8131m_ev_vt_p1),
	DB8131M_REGSET_TABLE_ELEMENT(GET_EV_INDEX(EV_PLUS_2), db8131m_ev_vt_p2),
	DB8131M_REGSET_TABLE_ELEMENT(GET_EV_INDEX(EV_PLUS_3), db8131m_ev_vt_p3),
	DB8131M_REGSET_TABLE_ELEMENT(GET_EV_INDEX(EV_PLUS_4), db8131m_ev_vt_p4),
};*/

static struct db8131m_regset_table brightness[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_bright_m4),
	DB8131M_REGSET_TABLE_ELEMENT(1, db8131m_bright_m3),
	DB8131M_REGSET_TABLE_ELEMENT(2, db8131m_bright_m2),
	DB8131M_REGSET_TABLE_ELEMENT(3, db8131m_bright_m1),
	DB8131M_REGSET_TABLE_ELEMENT(4, db8131m_bright_default),
	DB8131M_REGSET_TABLE_ELEMENT(5, db8131m_bright_p1),
	DB8131M_REGSET_TABLE_ELEMENT(6, db8131m_bright_p2),
	DB8131M_REGSET_TABLE_ELEMENT(7, db8131m_bright_p3),
	DB8131M_REGSET_TABLE_ELEMENT(8, db8131m_bright_p4),
};

/*static struct db8131m_regset_table white_balance[] = {
	DB8131M_REGSET_TABLE_ELEMENT(WHITE_BALANCE_AUTO,
						db8131m_wb_auto),
	DB8131M_REGSET_TABLE_ELEMENT(WHITE_BALANCE_SUNNY,
						db8131m_wb_sunny),
	DB8131M_REGSET_TABLE_ELEMENT(WHITE_BALANCE_CLOUDY,
						db8131m_wb_cloudy),
	DB8131M_REGSET_TABLE_ELEMENT(WHITE_BALANCE_TUNGSTEN,
						db8131m_wb_tungsten),
	DB8131M_REGSET_TABLE_ELEMENT(WHITE_BALANCE_FLUORESCENT,
						db8131m_wb_fluorescent),
};*/

/*static struct db8131m_regset_table effects[] = {
	DB8131M_REGSET_TABLE_ELEMENT(IMAGE_EFFECT_NONE,
						db8131m_effect_none),
	DB8131M_REGSET_TABLE_ELEMENT(IMAGE_EFFECT_BNW,
						db8131m_effect_gray),
	DB8131M_REGSET_TABLE_ELEMENT(IMAGE_EFFECT_SEPIA,
						db8131m_effect_sepia),
	DB8131M_REGSET_TABLE_ELEMENT(IMAGE_EFFECT_NEGATIVE,
						db8131m_effect_negative),
};*/

static struct db8131m_regset_table fps_vt_table[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_vt_7fps),
	DB8131M_REGSET_TABLE_ELEMENT(1, db8131m_vt_10fps),
	DB8131M_REGSET_TABLE_ELEMENT(2, db8131m_vt_15fps),
};

/*static struct db8131m_regset_table fps_table[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_fps_7),
	DB8131M_REGSET_TABLE_ELEMENT(1, db8131m_fps_10),
	DB8131M_REGSET_TABLE_ELEMENT(2, db8131m_fps_15),
	DB8131M_REGSET_TABLE_ELEMENT(3, db8131m_fps_auto),
};*/

/*static struct db8131m_regset_table blur_vt[] = {
	DB8131M_REGSET_TABLE_ELEMENT(BLUR_LEVEL_0, db8131m_blur_vt_none),
	DB8131M_REGSET_TABLE_ELEMENT(BLUR_LEVEL_1, db8131m_blur_vt_p1),
	DB8131M_REGSET_TABLE_ELEMENT(BLUR_LEVEL_2, db8131m_blur_vt_p2),
	DB8131M_REGSET_TABLE_ELEMENT(BLUR_LEVEL_3, db8131m_blur_vt_p3),
};

static struct db8131m_regset_table blur[] = {
	DB8131M_REGSET_TABLE_ELEMENT(BLUR_LEVEL_0, db8131m_blur_none),
	DB8131M_REGSET_TABLE_ELEMENT(BLUR_LEVEL_1, db8131m_blur_p1),
	DB8131M_REGSET_TABLE_ELEMENT(BLUR_LEVEL_2, db8131m_blur_p2),
	DB8131M_REGSET_TABLE_ELEMENT(BLUR_LEVEL_3, db8131m_blur_p3),
};*/

static struct db8131m_regset_table dataline[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_pattern_on),
};

static struct db8131m_regset_table dataline_stop[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_pattern_off),
};

static struct db8131m_regset_table init_reg[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_common),
};

static struct db8131m_regset_table init_vt_reg[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_vt_common),
};

static struct db8131m_regset_table preview[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_preview),
};

static struct db8131m_regset_table stream_stop[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_stream_stop),
};

static struct db8131m_regset_table vga_capture[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_vga_capture),
};

static struct db8131m_regset_table max_capture[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_capture),
};

static struct db8131m_regset_table record[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_recording_60Hz_common),
};

static struct db8131m_regset_table record_vga[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_size_vga),
};

static struct db8131m_regset_table record_qvga[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_size_qvga),
};

static struct db8131m_regset_table record_qcif[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_size_qcif),
};

static struct db8131m_regset_table record_cif[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_size_cif),
};

/*static struct db8131m_regset_table frame_size[] = {
	DB8131M_REGSET_TABLE_ELEMENT(0, db8131m_QCIF),
	DB8131M_REGSET_TABLE_ELEMENT(1, db8131m_QVGA),
	DB8131M_REGSET_TABLE_ELEMENT(2, db8131m_Return_VGA),
};*/

static int db8131m_reset(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_platform_data *pdata;

	pdata = client->dev.platform_data;

	if (pdata->common.power) {
		pdata->common.power(0);
		msleep(5);
		pdata->common.power(1);
		msleep(5);
		db8131m_init(sd, 0);
	}

	return 0;
}

#if CONFIG_LOAD_FILE
int db8131m_regs_table_init(void)
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

	fp = filp_open("/mnt/sdcard/db8131m_reg.h", O_RDONLY, 0);
	if (IS_ERR(fp)) {
		printk("failed to open /mnt/sdcard/db8131m_reg.h");
		return PTR_ERR(fp);
	}

	file_size = (size_t) fp->f_path.dentry->d_inode->i_size;
	max_size = file_size;

	//printk("file_size = %d", file_size);

	nBuf = kmalloc(file_size, GFP_ATOMIC);
	if (nBuf == NULL) {
		printk("Fail to 1st get memory");
		nBuf = vmalloc(file_size);
		if (nBuf == NULL) {
			printk("ERR: nBuf Out of Memory");
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
		testBuf = kmalloc(testBuf_size, GFP_ATOMIC);
		if (testBuf == NULL) {
			printk("Fail to get mem(%d bytes)", testBuf_size);
			testBuf = vmalloc(testBuf_size);
			large_file = 1;
		}
	}
	if (testBuf == NULL) {
		printk("ERR: Out of Memory");
		ret = -ENOMEM;
		goto error_out;
	}

	pos = 0;
	memset(nBuf, 0, file_size);
	memset(testBuf, 0, file_size * sizeof(struct test));

	nread = vfs_read(fp, (char __user *)nBuf, file_size, &pos);
	if (nread != file_size) {
		printk("failed to read file ret = %d", nread);
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

static inline int db8131m_write(struct i2c_client *client,
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

static int db8131m_write_regs_from_sd(char *name, struct i2c_client *client)
{
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
		if ((temp & 0xFF00) == DB8131M_DELAY) {
			delay = temp & 0xFF;
			//printk("db8131 delay(%d)", delay); /*step is 10msec */
			msleep(delay);
			continue;
		}
		ret = db8131m_write(client, temp);

		/* In error circumstances */
		/* Give second shot */
		if (unlikely(ret)) {
			ret = db8131m_write(client, temp);

			/* Give it one more shot */
			if (unlikely(ret))
				ret = db8131m_write(client, temp);
			}
		}

	return 0;
}

void db8131m_regs_table_exit(void)
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

static int db8131m_i2c_read_short(struct i2c_client *client, unsigned char subaddr,
	unsigned char *data)
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

static int db8131m_i2c_read_multi(struct i2c_client *client, unsigned short subaddr,
	unsigned long *data)
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

static int db8131m_i2c_write_multi(struct i2c_client *client, u16 packet)
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
static int db8131m_write_regs(struct v4l2_subdev *sd,
			       const u16 *regs, int size, char* name)
#else
static int db8131m_write_regs(struct v4l2_subdev *sd,
			       const u16 *regs, int size)
#endif
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, err, ret = 0;
	u8 m_delay = 0;
	u16 temp_packet;

#if CONFIG_LOAD_FILE
	ret = db8131m_write_regs_from_sd(name, client);
#else
	for (i = 0; i < size; i++) {
		temp_packet = regs[i];
		if ((temp_packet & 0xFF00) == DB8131M_DELAY) {
			m_delay = temp_packet & 0xFF;
			cam_dbg("delay: %d ms\n", m_delay);
			msleep(m_delay);
			continue;
		}
		err = db8131m_i2c_write_multi(client, temp_packet);
		if (unlikely(err < 0)) {
			cam_err("write_regs: register set failed\n");
			break;
		}
	}

	if (unlikely(err < 0))
		return -EIO;
#endif
	return 0;
}

static int db8131m_write_regset_table(struct v4l2_subdev *sd,
				const struct db8131m_regset_table *regset_table)
{
	int err;

	if (regset_table->regset)
#if CONFIG_LOAD_FILE
		err = db8131m_write_regs(sd, regset_table->regset,
				regset_table->array_size,
				regset_table->name);
#else
		err = db8131m_write_regs(sd, regset_table->regset,
				regset_table->array_size);
#endif
	else
		err = -EINVAL;

	return err;
}

static int db8131m_set_from_table(struct v4l2_subdev *sd,
				const char *setting_name,
				const struct db8131m_regset_table *table,
				int table_size, int index)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_dbg(&client->dev, "%s: set %s index %d\n",
		__func__, setting_name, index);

	if ((index < 0) || (index >= table_size)) {
		dev_err(&client->dev,
			"%s: index(%d) out of range[0:%d] for table for %s\n",
			__func__, index, table_size, setting_name);
		return -EINVAL;
	}
	table += index;
	if (table == NULL)
		return -EINVAL;
	return db8131m_write_regset_table(sd, table);
}

static int db8131m_set_parameter(struct v4l2_subdev *sd,
				int *current_value_ptr,
				int new_value,
				const char *setting_name,
				const struct db8131m_regset_table *table,
				int table_size)
{
	int err;

	if (*current_value_ptr == new_value)
		return 0;

	err = db8131m_set_from_table(sd, setting_name, table,
				table_size, new_value);

	if (!err)
		*current_value_ptr = new_value;

	return err;
}

static int db8131m_set_preview_start(struct v4l2_subdev *sd)
{
	/*int err;
	struct db8131m_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!state->pix.width || !state->pix.height)
		return -EINVAL;

	err = db8131m_set_from_table(sd, "frame_size", frame_size,
			ARRAY_SIZE(frame_size), state->framesize_index);
	if (err < 0) {
		dev_err(&client->dev,
				"%s: failed: Could not set preview size\n",
				__func__);
		return -EIO;
	}*/

	return 0;
}

static struct v4l2_queryctrl db8131m_controls[] = {
	/* Add here if needed */
};

/*const char **db8131m_ctrl_get_menu(u32 id)
{
	pr_debug("%s is called... id : %d\n", __func__, id);

	switch (id) {
	default:
		return v4l2_ctrl_get_menu(id);
	}
}*/

static inline struct v4l2_queryctrl const *db8131m_find_qctrl(int id)
{
	int i;

	pr_debug("%s is called... id : %d\n", __func__, id);

	for (i = 0; i < ARRAY_SIZE(db8131m_controls); i++)
		if (db8131m_controls[i].id == id)
			return &db8131m_controls[i];

	return NULL;
}

static int db8131m_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;

	pr_debug("%s is called...\n", __func__);

	for (i = 0; i < ARRAY_SIZE(db8131m_controls); i++) {
		if (db8131m_controls[i].id == qc->id) {
			memcpy(qc, &db8131m_controls[i],
			       sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int db8131m_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;

	pr_debug("%s is called...\n", __func__);

	qctrl.id = qm->id;
	db8131m_queryctrl(sd, &qctrl);

	//return v4l2_ctrl_query_menu(qm, &qctrl, db8131m_ctrl_get_menu(qm->id));
	return 0;
}

/*
 * Clock configuration
 * Configure expected MCLK from host and return EINVAL if not supported clock
 * frequency is expected
 * freq : in Hz
 * flag : not supported for now
 */
static int db8131m_s_crystal_freq(struct v4l2_subdev *sd, u32 freq, u32 flags)
{
	int err = -EINVAL;

	pr_debug("%s is called...\n", __func__);

	return err;
}

static int db8131m_g_mbus_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	int err = 0;

	pr_debug("%s is called...\n", __func__);

	return err;
}

static void db8131m_set_framesize(struct v4l2_subdev *sd,
				const struct db8131m_enum_framesize *frmsize,
				int frmsize_count, bool preview)
{
	struct db8131m_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	const struct db8131m_enum_framesize *last_frmsize =
		&frmsize[frmsize_count - 1];

	cam_dbg("set_framesize: Requested Res %dx%d\n",
		state->pix.width, state->pix.height);

	do {
		if (frmsize->width == state->pix.width &&
		    frmsize->height == state->pix.height) {
			break;
		}

		frmsize++;
	} while (frmsize <= last_frmsize);

	if (frmsize > last_frmsize)
		frmsize = last_frmsize;

	state->pix.width = frmsize->width;
	state->pix.height = frmsize->height;
	state->framesize_index = frmsize->index;

	cam_info("%s Res Set: %dx%d, index %d\n", (preview) ? "Preview" : "Capture",
		state->pix.width, state->pix.height, state->framesize_index);
}

static int db8131m_s_mbus_fmt(struct v4l2_subdev *sd,
			       struct v4l2_mbus_framefmt *fmt)
{
	struct db8131m_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;

	if (fmt->code == V4L2_MBUS_FMT_FIXED &&
			fmt->colorspace != V4L2_COLORSPACE_JPEG) {
		dev_dbg(&client->dev,
				"%s: mismatch in pixelformat and colorspace\n",
				__func__);
		return -EINVAL;
	}

	if (fmt->field < IS_MODE_CAPTURE_STILL)
		state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
	else
		state->format_mode = V4L2_PIX_FMT_MODE_CAPTURE;

	state->pix.width = fmt->width;
	state->pix.height = fmt->height;
	if (fmt->colorspace == V4L2_COLORSPACE_JPEG)
		state->pix.pixelformat = V4L2_PIX_FMT_JPEG;
	else
		state->pix.pixelformat = 0; /* is this used anywhere? */

	if (state->format_mode == V4L2_PIX_FMT_MODE_PREVIEW) {
		db8131m_set_framesize(sd, db8131m_framesize_list,
			ARRAY_SIZE(db8131m_framesize_list), true);
	} else if (state->format_mode == V4L2_PIX_FMT_MODE_CAPTURE) {
		db8131m_set_framesize(sd, db8131m_capture_framesize_list,
			ARRAY_SIZE(db8131m_capture_framesize_list), false);
	}


	return err;
}

static int db8131m_enum_framesizes(struct v4l2_subdev *sd,
				    struct v4l2_frmsizeenum *fsize)
{
	struct db8131m_state *state = to_state(sd);
	int num_entries = ARRAY_SIZE(db8131m_framesize_list);
	struct db8131m_enum_framesize *elem;
	int index = 0;
	int i = 0;

	/* The camera interface should read this value, this is the resolution
	 * at which the sensor would provide framedata to the camera i/f
	 *
	 * In case of image capture,
	 * this returns the default camera resolution (WVGA)
	 */
	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;

	index = state->framesize_index;

	if (state->format_mode == V4L2_PIX_FMT_MODE_PREVIEW) {
		for (i = 0; i < num_entries; i++) {
			elem = &db8131m_framesize_list[i];
			if (elem->index == index) {
				fsize->discrete.width =
				    db8131m_framesize_list[index].width;
				fsize->discrete.height =
				    db8131m_framesize_list[index].height;
				cam_dbg("enum_framesizes: index %d\n", index);
				return 0;
			}
		}
	} else if (state->format_mode == V4L2_PIX_FMT_MODE_CAPTURE) {
		for (i = 0; i < num_entries; i++) {
			elem = &db8131m_capture_framesize_list[i];
			if (elem->index == index) {
				fsize->discrete.width =
				    db8131m_capture_framesize_list[index].width;
				fsize->discrete.height =
				    db8131m_capture_framesize_list[index].height;
				cam_dbg("enum_framesizes: index %d\n", index);
				return 0;
			}
		}
	}

	cam_err("enum_framesizes:%dx%d not found\n",
		fsize->discrete.width, fsize->discrete.height);
	return -EINVAL;
}

static int db8131m_enum_frameintervals(struct v4l2_subdev *sd,
					struct v4l2_frmivalenum *fival)
{
	int err = 0;


	return err;
}

static int db8131m_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
				  enum v4l2_mbus_pixelcode *code)
{
	int err = 0;


	return err;
}

static int db8131m_try_mbus_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	int err = 0;


	return err;
}

static int db8131m_g_parm(struct v4l2_subdev *sd,
			   struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_state *state = to_state(sd);

	dev_dbg(&client->dev, "%s\n", __func__);
	state->strm.parm.capture.timeperframe.numerator = 1;
	state->strm.parm.capture.timeperframe.denominator = state->fps;
	memcpy(param, &state->strm, sizeof(param));

	return 0;
}

static int db8131m_s_parm(struct v4l2_subdev *sd,
			   struct v4l2_streamparm *param)
{
	int err = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_state *state = to_state(sd);
	/*struct sec_cam_parm *new_parms =
		(struct sec_cam_parm *)&param->parm.raw_data;
	struct sec_cam_parm *parms =
		(struct sec_cam_parm *)&state->strm.parm.raw_data;

	dev_dbg(&client->dev, "%s: start\n", __func__);*/

	/* we return an error if one happened but don't stop trying to
	 * set all parameters passed
	 */

	/*err = db8131m_set_parameter(sd, &parms->brightness,
				new_parms->brightness, "brightness",
				brightness, ARRAY_SIZE(brightness));
	err |= db8131m_set_parameter(sd, &parms->effects, new_parms->effects,
				"effects", effects,
				ARRAY_SIZE(effects));
	err |= db8131m_set_parameter(sd, &parms->white_balance,
				new_parms->white_balance,
				"white_balance", white_balance,
				ARRAY_SIZE(white_balance));

	dev_dbg(&client->dev, "%s: returning %d\n", __func__, err);*/

	return err;
}

/* set sensor register values for adjusting brightness */
static int db8131m_set_brightness(struct v4l2_subdev *sd,
				   struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_state *state = to_state(sd);
	struct db8131m_regset_table *ev;
	int err = -EINVAL;
	int ev_index = 0;
	int array_size = 0;

	dev_dbg(&client->dev, "%s: value : %d state->vt_mode %d\n",
			__func__, ctrl->value, state->vt_mode);

	pr_debug("state->vt_mode : %d\n", state->vt_mode);

	if ((ctrl->value < EV_MINUS_4) || (ctrl->value >= EV_MAX))
		ev_index = EV_DEFAULT;
	else
		ev_index = ctrl->value + 4;

	if (state->vt_mode == 1) {
		//ev = brightness_vt;
		//array_size = ARRAY_SIZE(brightness_vt);
	} else {
		ev = brightness;
		array_size = ARRAY_SIZE(brightness);
	}

	if (ev_index >= array_size)
		ev_index = EV_DEFAULT + 4;

	ev += ev_index;
	err = db8131m_write_regset_table(sd, ev);

	if (err)
		dev_err(&client->dev, "%s: register set failed\n", __func__);

	return err;
}

/*
 * set sensor register values for
 * adjusting whitebalance, both auto and manual
 */
static int db8131m_set_wb(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	/*struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_regset_table *wb = white_balance;
	int err = -EINVAL;

	dev_dbg(&client->dev, "%s: value : %d\n", __func__, ctrl->value);

	if ((ctrl->value < WHITE_BALANCE_BASE) ||
		(ctrl->value > WHITE_BALANCE_MAX) ||
		(ctrl->value >= ARRAY_SIZE(white_balance))) {
		dev_dbg(&client->dev, "%s: Value(%d) out of range([%d:%d])\n",
			__func__, ctrl->value,
			WHITE_BALANCE_BASE, WHITE_BALANCE_MAX);
		dev_dbg(&client->dev, "%s: Value out of range\n", __func__);
		goto out;
	}

	wb += ctrl->value;

	err = db8131m_write_regset_table(sd, wb);

	if (err)
		dev_dbg(&client->dev, "%s: register set failed\n", __func__);
out:
	return err;*/
	return 0;
}

/* set sensor register values for adjusting color effect */
static int db8131m_set_effect(struct v4l2_subdev *sd,
			       struct v4l2_control *ctrl)
{
	/*struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_regset_table *effect = effects;
	int err = -EINVAL;

	dev_dbg(&client->dev, "%s: value : %d\n", __func__, ctrl->value);

	if ((ctrl->value < IMAGE_EFFECT_BASE) ||
		(ctrl->value > IMAGE_EFFECT_MAX) ||
		(ctrl->value >= ARRAY_SIZE(effects))) {
		dev_dbg(&client->dev, "%s: Value(%d) out of range([%d:%d])\n",
			__func__, ctrl->value,
			IMAGE_EFFECT_BASE, IMAGE_EFFECT_MAX);
		goto out;
	}

	effect += ctrl->value;

	err = db8131m_write_regset_table(sd, effect);

	if (err)
		dev_dbg(&client->dev, "%s: register set failed\n", __func__);
out:
	return err;*/
	return 0;
}

/* set sensor register values for frame rate(fps) setting */
static int db8131m_set_frame_rate(struct v4l2_subdev *sd,
				   struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_state *state = to_state(sd);
	struct db8131m_regset_table *fps;
	int err = -EINVAL;
	int fps_index;

	cam_dbg("set_frame_rate: fps %d (vt %d)\n", ctrl->value, state->vt_mode);

	switch (ctrl->value) {
	case 0:
		fps_index = 3;
		break;

	case 7:
		fps_index = 0;
		break;

	case 10:
		fps_index = 1;
		break;

	case 15:
		fps_index = 2;
		break;

	default:
		cam_err("%s: Value(%d) is not supported\n",
			__func__, ctrl->value);
		goto out;
	}

	if (state->vt_mode == 1)
		fps = fps_vt_table;
	//else
		//fps = fps_table;

	fps += fps_index;
	state->fps = fps_index;

	err = db8131m_write_regset_table(sd, fps);
out:
	return err;
}

/* set sensor register values for adjusting blur effect */
static int db8131m_set_blur(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	/*struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_state *state = to_state(sd);
	struct db8131m_regset_table *bl;
	int err = -EINVAL;
	int array_size;

	dev_dbg(&client->dev, "%s: value : %d\n", __func__, ctrl->value);

	pr_debug("state->vt_mode : %d\n", state->vt_mode);

	if ((ctrl->value < BLUR_LEVEL_0) || (ctrl->value > BLUR_LEVEL_MAX)) {
		dev_dbg(&client->dev, "%s: Value(%d) out of range([%d:%d])\n",
			__func__, ctrl->value,
			BLUR_LEVEL_0, BLUR_LEVEL_MAX);
		goto out;
	}

	if (state->vt_mode == 1) {
		bl = blur_vt;
		array_size = ARRAY_SIZE(blur_vt);
	} else {
		bl = blur;
		array_size = ARRAY_SIZE(blur);
	}

	if (ctrl->value >= array_size) {
		dev_dbg(&client->dev, "%s: Value(%d) out of range([%d:%d))\n",
			__func__, ctrl->value,
			BLUR_LEVEL_0, array_size);
		goto out;
	}

	bl += ctrl->value;

	err = db8131m_write_regset_table(sd, bl);
out:
	return err;*/
	return 0;
}

static int db8131m_check_dataline_stop(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;

	dev_dbg(&client->dev, "%s\n", __func__);

	err = db8131m_write_regset_table(sd, dataline_stop);
	if (err < 0) {
		v4l_info(client, "%s: register set failed\n", __func__);
		return -EIO;
	}

	state->check_dataline = 0;
	err = db8131m_reset(sd);
	if (err < 0) {
		v4l_info(client, "%s: register set failed\n", __func__);
		return -EIO;
	}
	return err;
}

/* returns the real iso currently used by sensor due to lighting
 * conditions, not the requested iso we sent using s_ctrl.
 */
static int db8131m_get_iso(struct i2c_client *client, struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	unsigned char lsb, msb;
	unsigned short coarse_time, line_length, shutter_speed, exposureValue, iso;

	db8131m_i2c_write_multi(client, 0xFFD0);
	db8131m_i2c_read_short(client, 0x06, &msb);
	db8131m_i2c_read_short(client, 0x07, &lsb);
	coarse_time = (msb << 8) + lsb ;

	db8131m_i2c_read_short(client, 0x0A, &msb);
	db8131m_i2c_read_short(client, 0x0B, &lsb);
	line_length = (msb << 8) + lsb ;

	shutter_speed = 24000000 / (coarse_time * line_length);
	exposureValue = (coarse_time * line_length) / 2400 ;

	if (exposureValue >= 1000)
		iso = 400;
	else if (exposureValue >= 500)
		iso = 200;
	else if (exposureValue >= 333)
		iso = 100;
	else
		iso = 50;

	return iso;
}

static int db8131m_get_shutterspeed(struct i2c_client *client, struct v4l2_subdev *sd,
		struct v4l2_control *ctrl)
{
	unsigned char lsb, msb;
	unsigned short coarse_time, line_length, shutter_speed, exposureValue;

	db8131m_i2c_write_multi(client, 0xFFD0);
	db8131m_i2c_read_short(client, 0x06, &msb);
	db8131m_i2c_read_short(client, 0x07, &lsb);
	coarse_time = (msb << 8) + lsb ;

	db8131m_i2c_read_short(client, 0x0A, &msb);
	db8131m_i2c_read_short(client, 0x0B, &lsb);
	line_length = (msb << 8) + lsb ;

	shutter_speed = 24000000 / (coarse_time * line_length);

	return shutter_speed;
}

static int db8131m_set_sensor_mode(struct v4l2_subdev *sd, s32 val)
{
	struct db8131m_state *state = to_state(sd);

	cam_dbg("sensor_mode: %d\n", val);

	switch (val) {
	case SENSOR_MOVIE:
		/* We does not support movie mode when in VT. */
		if (state->vt_mode) {
			state->sensor_mode = SENSOR_CAMERA;
			cam_err("%s: error, not support movie\n", __func__);
			break;
		}
		state->sensor_mode = val;
		/* We do not break. */

	case SENSOR_CAMERA:
		state->sensor_mode = val;
		break;

	case 2:	/* 720p HD video mode */
		state->sensor_mode = SENSOR_MOVIE;
		break;

	default:
		cam_err("%s: error, not support.(%d)\n", __func__, val);
		state->sensor_mode = SENSOR_CAMERA;
		break;
	}

	return 0;
}

static int db8131m_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_state *state = to_state(sd);
	struct db8131m_userset userset = state->userset;
	int err = 0;

	dev_dbg(&client->dev, "%s: id : 0x%08x\n", __func__, ctrl->id);

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		ctrl->value = userset.exposure_bias;
		break;

	case V4L2_CID_AUTO_WHITE_BALANCE:
		ctrl->value = userset.auto_wb;
		break;

	case V4L2_CID_WHITE_BALANCE_PRESET:
		ctrl->value = userset.manual_wb;
		break;

	case V4L2_CID_COLORFX:
		ctrl->value = userset.effect;
		break;

	case V4L2_CID_CONTRAST:
		ctrl->value = userset.contrast;
		break;

	case V4L2_CID_SATURATION:
		ctrl->value = userset.saturation;
		break;

	case V4L2_CID_SHARPNESS:
		ctrl->value = userset.saturation;
		break;

	case V4L2_CID_CAMERA_EXIF_ISO:
		ctrl->value = db8131m_get_iso(client, sd, ctrl);
		break;

	case V4L2_CID_CAMERA_EXIF_EXPTIME:
		ctrl->value = db8131m_get_shutterspeed(client, sd, ctrl);
		break;

	/*case V4L2_CID_ESD_INT:
		ctrl->value = 0;
		break;*/

	default:
		dev_dbg(&client->dev, "%s: no such ctrl\n", __func__);
		err = -EINVAL;
		break;
	}

	return err;
}

static int db8131m_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;

	cam_dbg("s_ctrl: ID =%d, val = %d\n",
		ctrl->id - V4L2_CID_PRIVATE_BASE, ctrl->value);

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_SENSOR_MODE:
		err = db8131m_set_sensor_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_BRIGHTNESS:
		cam_trace("V4L2_CID_CAMERA_BRIGHTNESS\n");
		err = db8131m_set_brightness(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE:
		cam_trace("V4L2_CID_AUTO_WHITE_BALANCE\n");
		err = db8131m_set_wb(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_EFFECT:
		cam_trace("V4L2_CID_CAMERA_EFFECT\n");
		err = db8131m_set_effect(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_FRAME_RATE:
		cam_trace("V4L2_CID_CAMERA_FRAME_RATE\n");
		err = db8131m_set_frame_rate(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_VGA_BLUR:
		cam_trace("V4L2_CID_CAMERA_VGA_BLUR\n");
		err = db8131m_set_blur(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_VT_MODE:
		state->vt_mode = ctrl->value;
		cam_trace("vt_mode %d\n", state->vt_mode);
		err = 0;
		break;

	case V4L2_CID_CAMERA_CHECK_DATALINE:
		state->check_dataline = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_CHECK_DATALINE_STOP:
		err = db8131m_check_dataline_stop(sd);
		break;

	case V4L2_CID_CAM_PREVIEW_ONOFF:
		if (state->check_previewdata == 0)
			err = db8131m_set_preview_start(sd);
		else
			err = -EIO;
		break;

	case V4L2_CID_CAMERA_RESET:
		cam_trace("V4L2_CID_CAMERA_RESET\n");
		err = db8131m_reset(sd);
		break;

	default:
		cam_err("s_ctrl: unknown Ctrl-ID 0x%x\n", ctrl->id);
		err = 0;
		break;
	}

	if (err < 0)
		goto out;
	else
		return 0;

 out:
	cam_err("s_ctrl: failed\n");
	return err;
}

static int db8131m_check_sensor(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 value = 0, value_msb = 0, value_lsb = 0;
	u16 chip_id;

	/* Check chip id */
	db8131m_i2c_write_multi(client, 0xFFD0);
	db8131m_i2c_read_short(client, 0x00, &value_msb);
	db8131m_i2c_read_short(client, 0x01, &value_lsb);
	chip_id = (value_msb << 8) | value_lsb;
	if (unlikely(chip_id != DB8131M_CHIP_ID)) {
		cam_err("Sensor ChipID: unknown ChipID %04X\n", chip_id);
		return -ENODEV;
	}

	/* Check chip revision
	 * 0x06: new revision,  0x04: old revision */
	db8131m_i2c_write_multi(client, 0xFF02);
	db8131m_i2c_read_short(client, 0x09, &value);
	if (unlikely(value != DB8131M_CHIP_REV)) {
#ifdef CONFIG_MACH_ZEST
		/* We admit old sensor to be no problem temporarily */
		if (DB8131M_CHIP_REV_OLD == value) {
			cam_info("Sensor Rev: old rev %02X. check it\n", value);
			goto out;
		}
#endif
		cam_err("Sensor Rev: unkown Rev %02X\n", value);
		return -ENODEV;
	}

out:
	cam_dbg("Sensor chip indentification: Success");
	return 0;
}

static void db8131m_init_parameters(struct v4l2_subdev *sd)
{
	struct db8131m_state *state = to_state(sd);
	/*struct sec_cam_parm *parms =
		(struct sec_cam_parm *)&state->strm.parm.raw_data;

	parms->effects = IMAGE_EFFECT_NONE;
	parms->brightness = EV_DEFAULT;
	parms->white_balance = WHITE_BALANCE_AUTO;*/
	state->sensor_mode = SENSOR_CAMERA;
}

static int db8131m_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;

	cam_info("init: start (%s), vt %d\n", __DATE__, state->vt_mode);

	err = db8131m_check_sensor(sd);
	CHECK_ERR_MSG(err, "failed to indentify sensor chip\n");

	db8131m_init_parameters(sd);
	if (state->vt_mode == 0) {
		err = db8131m_write_regset_table(sd, init_reg);
	} else
		err = db8131m_write_regset_table(sd, init_vt_reg);

	if (err < 0) {
		/* This is preview fail */
		printk("[YUNCAM] %s, init reg fail!!!!", __func__);
		state->check_previewdata = 100;
		return -EIO;
	}

	/* This is preview success */
	state->check_previewdata = 0;
	return 0;
}

static int db8131m_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct db8131m_state *state = to_state(sd);
	int err = 0;

	cam_info("stream mode = %d, sensor_mode = %d\n", enable, state->sensor_mode);

	switch (enable) {
	case STREAM_MODE_CAM_OFF:
		cam_info("STREAM STOP!!\n");
		err = db8131m_write_regset_table(sd, stream_stop);//yuncam
		if (err < 0) {
			printk(KERN_ERR "%s, stop stream fail!!!!", __func__);
			state->check_previewdata = 100;
			return -EIO;
		}
		break;

	case STREAM_MODE_CAM_ON:
		switch (state->sensor_mode) {
		case SENSOR_CAMERA:
			if (state->format_mode == V4L2_PIX_FMT_MODE_CAPTURE) {
				cam_info("Camera Capture start\n");

				//err = db8131m_write_regset_table(sd, preview);//yuncam
				if (state->pix.width == 1280 && state->pix.height == 960) {
					err = db8131m_write_regset_table(sd, max_capture);//yuncam
				} else if (state->pix.width == 640 && state->pix.height == 480) {
					err = db8131m_write_regset_table(sd, vga_capture);//yuncam
				}
				if (err < 0) {
					printk(KERN_ERR "%s, start capture fail!!!!", __func__);
					state->check_previewdata = 100;
					return -EIO;
				}
			} else if (state->format_mode == V4L2_PIX_FMT_MODE_PREVIEW) {
				cam_info("Camera Preview start\n");

				if (state->pix.width == 640 && state->pix.height == 480) {
					err = db8131m_write_regset_table(sd, record_vga);
				} else if (state->pix.width == 352 && state->pix.height == 288) {
					err = db8131m_write_regset_table(sd, record_cif);
				} else if (state->pix.width == 320 && state->pix.height == 240) {
					err = db8131m_write_regset_table(sd, record_qvga);
				}
				err |= db8131m_write_regset_table(sd, preview);//yuncam
				if (err < 0) {
					printk(KERN_ERR "%s, start preview fail!!!!", __func__);
					state->check_previewdata = 100;
					return -EIO;
				}
			} else
				BUG_ON(1);

			break;

		case SENSOR_MOVIE:
			cam_info("Video Preview start\n");

			err = db8131m_write_regset_table(sd, record);
			if (state->pix.width == 640 && state->pix.height == 480) {
				err |= db8131m_write_regset_table(sd, record_vga);
			} else if (state->pix.width == 352 && state->pix.height == 288) {
				err |= db8131m_write_regset_table(sd, record_cif);
			} else if (state->pix.width == 320 && state->pix.height == 240) {
				err |= db8131m_write_regset_table(sd, record_qvga);
			} else if (state->pix.width == 176 && state->pix.height == 144) {
				err |= db8131m_write_regset_table(sd, record_qcif);
			}
			err |= db8131m_write_regset_table(sd, preview);//yuncam
			if (err < 0) {
				printk(KERN_ERR "%s, start movie fail!!!!", __func__);
				state->check_previewdata = 100;
				return -EIO;
			}
			break;

		default:
			break;
		}
		break;

	case STREAM_MODE_MOVIE_ON:
		/*cam_info("movie on");
		state->recording = 1;
		if (state->flash.mode != FLASH_MODE_OFF)
			isx012_flash_torch(sd, ISX012_FLASH_ON);*/
		break;

	case STREAM_MODE_MOVIE_OFF:
		/*cam_info("movie off");
		state->recording = 0;
		if (state->flash.on)
			isx012_flash_torch(sd, ISX012_FLASH_OFF);*/
		break;

	default:
		printk("%s: error - Invalid stream mode\n", __func__);
		break;
	}

	return 0;
}

static const struct v4l2_subdev_core_ops db8131m_core_ops = {
	.init = db8131m_init,		/* initializing API */
	.queryctrl = db8131m_queryctrl,
	.querymenu = db8131m_querymenu,
	.g_ctrl = db8131m_g_ctrl,
	.s_ctrl = db8131m_s_ctrl,
};

static const struct v4l2_subdev_video_ops db8131m_video_ops = {
	.s_crystal_freq = db8131m_s_crystal_freq,
	.g_mbus_fmt = db8131m_g_mbus_fmt,
	.s_mbus_fmt = db8131m_s_mbus_fmt,
	.enum_framesizes = db8131m_enum_framesizes,
	.enum_frameintervals = db8131m_enum_frameintervals,
	.enum_mbus_fmt = db8131m_enum_mbus_fmt,
	.try_mbus_fmt = db8131m_try_mbus_fmt,
	.g_parm = db8131m_g_parm,
	.s_parm = db8131m_s_parm,
	.s_stream = db8131m_s_stream,
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
	struct db8131m_state *state;
	struct v4l2_subdev *sd;
	struct db8131m_platform_data *pdata = client->dev.platform_data;
#if CONFIG_LOAD_FILE
	int err = -EINVAL;
#endif
	state = kzalloc(sizeof(struct db8131m_state), GFP_KERNEL);
	if (unlikely(!state)) {
		dev_err(&client->dev, "probe, fail to get memory\n");
		return -ENOMEM;
	}

	sd = &state->sd;
	strcpy(sd->name, driver_name);

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	if (!(pdata->common.default_width && pdata->common.default_height)) {
		/* TODO: assign driver default resolution */
	} else {
		state->pix.width = pdata->common.default_width;
		state->pix.height = pdata->common.default_height;
	}

	if (!pdata->common.pixelformat)
		state->pix.pixelformat = DEFAULT_FMT;
	else
		state->pix.pixelformat = pdata->common.pixelformat;

	if (pdata->common.freq == 0)
		state->freq = DEFUALT_MCLK;
	else
		state->freq = pdata->common.freq;

	if (!pdata->common.is_mipi) {
		state->is_mipi = 0;
		dev_dbg(&client->dev, "parallel mode\n");
	} else
		state->is_mipi = pdata->common.is_mipi;

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &db8131m_ops);

#if CONFIG_LOAD_FILE
	err = db8131m_regs_table_init();
	CHECK_ERR_MSG(err, "failed to load the config file\n");
#endif
	printk(KERN_DEBUG "%s %s: driver probed!!\n",
		dev_driver_string(&client->dev), dev_name(&client->dev));
	return 0;
}

static int db8131m_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct db8131m_state *state = to_state(sd);

	v4l2_device_unregister_subdev(sd);
	kfree(state);

	printk(KERN_DEBUG "%s %s: driver removed!!\n",
		dev_driver_string(&client->dev), dev_name(&client->dev));
	return 0;
}

static ssize_t camtype_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	/*pr_info("%s\n", __func__);*/
	return sprintf(buf, "%s_%s\n", "DB", "DB8131A");
}
static DEVICE_ATTR(front_camtype, S_IRUGO, camtype_show, NULL);

static ssize_t camfw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s %s\n", "DB8131A", "DB8131A");

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

int db8131m_create_sysfs(struct class *cls)
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

static const struct i2c_device_id db8131m_id[] = {
	{ DB8131M_DRIVER_NAME, 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, db8131m_id);

static struct i2c_driver v4l2_i2c_driver = {
	.driver.name = driver_name,
	.probe = db8131m_probe,
	.remove = db8131m_remove,
	.id_table = db8131m_id,
};

static int __init v4l2_i2c_drv_init(void)
{
	pr_info("%s: %s init\n", __func__, driver_name);
	db8131m_create_sysfs(camera_class);
	return i2c_add_driver(&v4l2_i2c_driver);
}

static void __exit v4l2_i2c_drv_cleanup(void)
{
	pr_info("%s: %s exit\n", __func__, driver_name);
	i2c_del_driver(&v4l2_i2c_driver);
}

module_init(v4l2_i2c_drv_init);
module_exit(v4l2_i2c_drv_cleanup);

MODULE_DESCRIPTION("Samsung Electronics DB8131M UXGA camera driver");
MODULE_AUTHOR("Kyoungho Yun <kyoungho.yun@samsung.com>");
MODULE_LICENSE("GPL");
