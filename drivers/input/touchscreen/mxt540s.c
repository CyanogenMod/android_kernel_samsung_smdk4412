/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c/mxt540s.h>
#include <asm/unaligned.h>
#include <linux/firmware.h>
#include <linux/string.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "mxt540s_dev.h"

int mxt_read_mem(struct mxt_data *data, u16 reg, u8 len, u8 *buf)
{
	int ret = 0, i = 0;
	u16 le_reg = cpu_to_le16(reg);
	struct i2c_msg msg[2] = {
		{
			.addr = data->client->addr,
			.flags = 0,
			.len = 2,
			.buf = (u8 *)&le_reg,
		},
		{
			.addr = data->client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf,
		},
	};

#if TSP_ITDEV
	if (data->command_off)
		return 0;
#endif

	for (i = 0; i < 3 ; i++) {
		ret = i2c_transfer(data->client->adapter, msg, 2);
		if (ret < 0)
			dev_err(&data->client->dev, "%s fail[%d] address[0x%x]\n",
				__func__, ret, le_reg);
		else
			break;
	}
	return ret == 2 ? 0 : -EIO;
}

int mxt_write_mem(struct mxt_data *data,
		u16 reg, u8 len, const u8 *buf)
{
	int ret = 0, i = 0;
	u8 tmp[len + 2];

#if TSP_ITDEV
	if (data->command_off)
		return 0;
#endif

	put_unaligned_le16(cpu_to_le16(reg), tmp);
	memcpy(tmp + 2, buf, len);

	for (i = 0; i < 3 ; i++) {
		ret = i2c_master_send(data->client, tmp, sizeof(tmp));
		if (ret < 0)
			dev_err(&data->client->dev,
				"%s %d times write error on address[0x%x,0x%x]\n",
				__func__, i, tmp[1], tmp[0]);
		else
			break;
	}

	return ret == sizeof(tmp) ? 0 : -EIO;
}

struct mxt_object *
	mxt_get_object(struct mxt_data *data, u8 object_type)
{
	struct mxt_object *object;
	int i;

	if (!data->objects)
		return NULL;

	for (i = 0; i < data->info.object_num; i++) {
		object = data->objects + i;
		if (object->object_type == object_type)
			return object;
	}

	dev_err(&data->client->dev, "Invalid object type T%d\n",
		object_type);

	return NULL;
}

int mxt_read_object(struct mxt_data *data,
				u8 type, u8 offset, u8 *val)
{
	struct mxt_object *object;
	u16 reg;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	reg = object->start_address;

	return mxt_read_mem(data, reg + offset, 1, val);
}

int mxt_write_object(struct mxt_data *data,
				 u8 type, u8 offset, u8 val)
{
	struct mxt_object *object;
	u16 reg;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	if (offset >= object->size * object->instances) {
		dev_err(&data->client->dev,
			"Tried to write outside object T%d offset:%d, size:%d\n",
			type, offset, object->size);
		return -EINVAL;
	}
	reg = object->start_address;
	return mxt_write_mem(data, reg + offset, 1, &val);
}

static int mxt_reset(struct mxt_data *data)
{
	u8 buf = 1u;
	return mxt_write_mem(data, data->cmd_proc + CMD_RESET_OFFSET, 1, &buf);
}

static int mxt_backup(struct mxt_data *data)
{
	u8 buf = 0x55u;
	return mxt_write_mem(data, data->cmd_proc + CMD_BACKUP_OFFSET, 1, &buf);
}

static int mxt_start(struct mxt_data *data)
{
	int error;

	/* Touch report enable */
	error = mxt_write_object(data, TOUCH_MULTITOUCHSCREEN_T9,
		MXT_T9_CTRL, data->tsp_ctrl);

	if (error)
		dev_err(&data->client->dev, "Fail to start touch\n");

	return error;
}

static void mxt_stop(struct mxt_data *data)
{
	/* Touch report disable */
	mxt_write_object(data, TOUCH_MULTITOUCHSCREEN_T9, MXT_T9_CTRL, 0);
}

static int mxt_check_instance(struct mxt_data *data, u8 object_type)
{
	int i;

	for (i = 0; i < data->info.object_num; i++) {
		if (data->objects[i].object_type == object_type)
			return data->objects[i].instances;
	}
	return 0;
}

static u32 crc24(u32 crc, u8 byte1, u8 byte2)
{
	static const u32 crcpoly = 0x80001B;
	u32 res;
	u16 data_word;

	data_word = (((u16)byte2) << 8) | byte1;
	res = (crc << 1) ^ (u32)data_word;

	if (res & 0x1000000)
		res ^= crcpoly;

	return res;
}

static int mxt_calculate_infoblock_crc(struct mxt_data *data,
		u32 *crc_pointer)
{
	u32 crc = 0;
	u8 mem[7 + data->info.object_num * 6];
	int ret;
	int i;

	ret = mxt_read_mem(data, 0, sizeof(mem), mem);

	if (ret)
		return ret;

	for (i = 0; i < sizeof(mem) - 1; i += 2)
		crc = crc24(crc, mem[i], mem[i + 1]);

	*crc_pointer = crc24(crc, mem[i], 0) & 0x00FFFFFF;

	return 0;
}

static int mxt_read_info_crc(struct mxt_data *data, u32 *crc_pointer)
{
	u16 crc_address;
	u8 msg[3];
	int ret;

	/* Read Info block CRC address */
	crc_address = OBJECT_TABLE_START_ADDRESS +
			data->info.object_num * OBJECT_TABLE_ELEMENT_SIZE;

	ret = mxt_read_mem(data, crc_address, 3, msg);
	if (ret)
		return ret;

	*crc_pointer = msg[0] | (msg[1] << 8) | (msg[2] << 16);

	return 0;
}

static int mxt_reportid_to_type(struct mxt_data *data,
			u8 report_id, u8 *instance)
{
	if (report_id <= data->max_report_id) {
		*instance = data->rid_map[report_id].instance;
		return data->rid_map[report_id].object_type;
	} else
		return 0;
}

static int mxt_read_config_crc(struct mxt_data *data, u32 *crc_pointer)
{
	struct device *dev = &data->client->dev;
	struct mxt_object *object;
	u8 msg[data->msg_object_size];
	int error, try = 0;
	int fail_count = data->max_report_id * 2;
	u8 object_type, instance;

	object = mxt_get_object(data, GEN_COMMANDPROCESSOR_T6);
	if (!object)
		return -EIO;

	/* Try to read the config checksum of the existing cfg */
	mxt_write_object(data, GEN_COMMANDPROCESSOR_T6,
		CMD_REPORTATLL_OFFSET, 1);

	/* Read message from command processor, which only has one report ID */
	while (++try < fail_count) {
		error = mxt_read_mem(data, data->msg_proc, sizeof(msg), msg);
		if (error)
			return error;

		object_type = mxt_reportid_to_type(data, msg[0] , &instance);
		if (object_type == RESERVED_T0) {
			dev_err(&data->client->dev,
				"Untreated Object type[%d]\n", object_type);
			continue;
		}

		if (object_type == GEN_COMMANDPROCESSOR_T6)
			break;
	}

	if (error) {
		dev_err(dev, "Failed to retrieve CRC\n");
		return error;
	}

	/* Bytes 1-3 are the checksum. */
	*crc_pointer = msg[2] | (msg[3] << 8) | (msg[4] << 16);

	return 0;
}

static int mxt_get_id_info(struct mxt_data *data)
{
	int ret = 0;
	u8 id[ID_BLOCK_SIZE];

	/* Read IC information */
	ret = mxt_read_mem(data, 0, sizeof(id), id);
	if (ret) {
		dev_err(&data->client->dev, "Read fail. IC information\n");
		goto out;
	} else {
		dev_info(&data->client->dev,
			"family: 0x%x variant: 0x%x version: 0x%x"
			" build: 0x%x matrix X,Y size:  %d,%d"
			" number of obect: %d\n"
			, id[0], id[1], id[2], id[3], id[4], id[5], id[6]);
		data->info.family_id = id[0];
		data->info.variant_id = id[1];
		data->info.version = id[2];
		data->info.build = id[3];
		data->info.matrix_xsize = id[4];
		data->info.matrix_ysize = id[5];
		data->info.object_num = id[6];
	}

out:
	return ret;
}

static int mxt_get_object_table(struct mxt_data *data)
{
	int ret = 0;
	int i;
	u8 type_count = 0;

	ret = mxt_read_mem(data, OBJECT_TABLE_START_ADDRESS,
		data->info.object_num * sizeof(*data->objects),
		(u8 *)data->objects);

	if (ret)
		goto out;

	data->max_report_id = 0;

	for (i = 0; i < data->info.object_num; i++) {
		data->objects[i].start_address =
			le16_to_cpu(data->objects[i].start_address);
		/* size and instance values are smaller than atual value */
		data->objects[i].size += 1;
		data->objects[i].instances += 1;
		data->max_report_id += data->objects[i].num_report_ids *
						(data->objects[i].instances);

		switch (data->objects[i].object_type) {
		case GEN_MESSAGEPROCESSOR_T5:
			data->msg_object_size = data->objects[i].size;
			data->msg_proc = data->objects[i].start_address;
			dev_dbg(&data->client->dev,
				"mesage object size: %d message address: 0x%x\n",
				data->msg_object_size, data->msg_proc);
			break;
		case GEN_COMMANDPROCESSOR_T6:
			data->cmd_proc = data->objects[i].start_address;
			break;
		case TOUCH_MULTITOUCHSCREEN_T9:
			data->finger_report_id = type_count + 1;
			dev_dbg(&data->client->dev, "Finger report id: %d\n",
						data->finger_report_id);
			break;
		}

		if (data->objects[i].num_report_ids) {
			type_count += data->objects[i].num_report_ids *
						(data->objects[i].instances);
		}
	}

	dev_info(&data->client->dev, "maXTouch: %d Objects\n",
			data->info.object_num);
#ifdef DEBUG
	for (i = 0; i < data->info.object_num; i++) {
		dev_dbg(&data->client->dev,
			"Object:T%d\t\t\t Address:0x%x\tSize:%d\tInstance:%d\tReport Id's:%d\n",
			data->objects[i].object_type,
			data->objects[i].start_address,
			data->objects[i].size,
			data->objects[i].instances,
			data->objects[i].num_report_ids);
	}
#endif

out:
	return ret;
}

static void __devinit mxt_make_reportid_table(struct mxt_data *data)
{
	struct mxt_object *objects = data->objects;
	int i, j;
	int cur_id, sta_id;

	data->rid_map[0].instance = 0;
	data->rid_map[0].object_type = 0;
	cur_id = 1;

	for (i = 0; i < data->info.object_num; i++) {
		if (objects[i].num_report_ids == 0)
			continue;
		for (j = 1; j <= objects[i].instances; j++) {
			for (sta_id = cur_id;
				cur_id < (sta_id + objects[i].num_report_ids);
				cur_id++) {

				data->rid_map[cur_id].instance = j;
				data->rid_map[cur_id].object_type =
					objects[i].object_type;
			}
		}
	}

	dev_info(&data->client->dev, "maXTouch: %d report ID\n",
			data->max_report_id);

#ifdef DEBUG
	for (i = 0; i < data->max_report_id; i++) {
		dev_dbg(&data->client->dev, "Report_id[%d]:\tT%d\n",
			i, data->rid_map[i].object_type);
	}
#endif
}

static int mxt_init_write_config(struct mxt_data *data,
		u8 type, const u8 *cfg)
{
	struct mxt_object *object;
	u8 *temp;
	int ret;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	if ((object->size == 0) || (object->start_address == 0)) {
		dev_err(&data->client->dev,
			"%s error object_type T%d\n", __func__, type);
		return -ENODEV;
	}

	ret = mxt_write_mem(data, object->start_address,
			object->size, cfg);
	if (ret) {
		dev_err(&data->client->dev,
			"%s write error T%d address[0x%x]\n",
			__func__, type, object->start_address);
		return ret;
	}

	if (mxt_check_instance(data, type)) {
		temp = kzalloc(object->size, GFP_KERNEL);

		if (temp == NULL)
			return -ENOMEM;

		ret |= mxt_write_mem(data, object->start_address + object->size,
			object->size, temp);
		kfree(temp);
	}

	return ret;
}

static int mxt_write_config_from_pdata(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	u8 **tsp_config = (u8 **)data->pdata->config;
	u8 i;
	int ret;

	if (!tsp_config) {
		dev_info(dev, "No cfg data in pdata\n");
		return 0;
	}

	for (i = 0; tsp_config[i][0] != RESERVED_T255; i++) {
		ret = mxt_init_write_config(data, tsp_config[i][0],
							tsp_config[i] + 1);
		if (ret)
			return ret;

		if (tsp_config[i][0] == TOUCH_MULTITOUCHSCREEN_T9) {
			/* Are x and y inverted? */
			if (tsp_config[i][10] & 0x1) {
				data->x_dropbits =
					(!(tsp_config[i][22] & 0xC)) << 1;
				data->y_dropbits =
					(!(tsp_config[i][20] & 0xC)) << 1;
			} else {
				data->x_dropbits =
					(!(tsp_config[i][20] & 0xC)) << 1;
				data->y_dropbits =
					(!(tsp_config[i][22] & 0xC)) << 1;
			}
		}
	}
	return ret;
}

#if DUAL_CFG
static int mxt_write_config(struct mxt_fw_info *fw_info)
{
	struct mxt_data *data = fw_info->data;
	struct device *dev = &data->client->dev;
	struct mxt_object *object;
	struct mxt_cfg_data *cfg_data;
	u32 current_crc;
	u8 i, val = 0;
	u16 reg, index;
	int ret;
	u32 cfg_length = data->cfg_len = fw_info->cfg_len / 2 ;

	if (!fw_info->ta_cfg_raw_data && !fw_info->batt_cfg_raw_data) {
		dev_info(dev, "No cfg data in file\n");
		ret = mxt_write_config_from_pdata(data);
		return ret;
	}

	/* Get config CRC from device */
	ret = mxt_read_config_crc(data, &current_crc);
	if (ret)
		return ret;

	/* Check Version information */
	if (fw_info->fw_ver != data->info.version) {
		dev_err(dev, "Warning: version mismatch! %s\n", __func__);
		return 0;
	}
	if (fw_info->build_ver != data->info.build) {
		dev_err(dev, "Warning: build num mismatch! %s\n", __func__);
		return 0;
	}

	/* Check config CRC */
	if (current_crc == fw_info->cfg_crc) {
		dev_info(dev, "Skip writing Config:[CRC 0x%06X]\n",
			current_crc);
		return 0;
	}

	dev_info(dev, "Writing Config:[CRC 0x%06X!=0x%06X]\n",
		current_crc, fw_info->cfg_crc);

	/* Get the address of configuration data */
	data->batt_cfg_raw_data = fw_info->batt_cfg_raw_data;
	data->ta_cfg_raw_data = fw_info->ta_cfg_raw_data =
		fw_info->batt_cfg_raw_data + cfg_length;

	/* Write config info */
	for (index = 0; index < cfg_length;) {
		if (index + sizeof(struct mxt_cfg_data) >= cfg_length) {
			dev_err(dev, "index(%d) of cfg_data exceeded total size(%d)!!\n",
				index + sizeof(struct mxt_cfg_data),
				cfg_length);
			return -EINVAL;
		}

		/* Get the info about each object */
		if (data->charging_mode)
			cfg_data = (struct mxt_cfg_data *)
					(&fw_info->ta_cfg_raw_data[index]);
		else
			cfg_data = (struct mxt_cfg_data *)
					(&fw_info->batt_cfg_raw_data[index]);

		index += sizeof(struct mxt_cfg_data) + cfg_data->size;
		if (index > cfg_length) {
			dev_err(dev, "index(%d) of cfg_data exceeded total size(%d) in T%d object!!\n",
				index, cfg_length, cfg_data->type);
			return -EINVAL;
		}

		object = mxt_get_object(data, cfg_data->type);
		if (!object) {
			dev_err(dev, "T%d is Invalid object type\n",
				cfg_data->type);
			return -EINVAL;
		}

		/* Check and compare the size, instance of each object */
		if (cfg_data->size > object->size) {
			dev_err(dev, "T%d Object length exceeded!\n",
				cfg_data->type);
			return -EINVAL;
		}
		if (cfg_data->instance >= object->instances) {
			dev_err(dev, "T%d Object instances exceeded!\n",
				cfg_data->type);
			return -EINVAL;
		}

		dev_dbg(dev, "Writing config for obj %d len %d instance %d (%d/%d)\n",
			cfg_data->type, object->size,
			cfg_data->instance, index, cfg_length);

		reg = object->start_address + object->size * cfg_data->instance;

		/* Write register values of each object */
		ret = mxt_write_mem(data, reg, cfg_data->size,
					 cfg_data->register_val);
		if (ret) {
			dev_err(dev, "Write T%d Object failed\n",
				object->object_type);
			return ret;
		}

		/*
		 * If firmware is upgraded, new bytes may be added to end of
		 * objects. It is generally forward compatible to zero these
		 * bytes - previous behaviour will be retained. However
		 * this does invalidate the CRC and will force a config
		 * download every time until the configuration is updated.
		 */
		if (cfg_data->size < object->size) {
			dev_err(dev, "Warning: zeroing %d byte(s) in T%d\n",
				 object->size - cfg_data->size, cfg_data->type);

			for (i = cfg_data->size + 1; i < object->size; i++) {
				ret = mxt_write_mem(data, reg + i, 1, &val);
				if (ret)
					return ret;
			}
		}
	}
	dev_info(dev, "Updated configuration\n");

	return ret;
}
#else
static int mxt_write_config(struct mxt_fw_info *fw_info)
{
	struct mxt_data *data = fw_info->data;
	struct device *dev = &data->client->dev;
	struct mxt_object *object;
	struct mxt_cfg_data *cfg_data;
	u32 current_crc;
	u8 i, val = 0;
	u16 reg, index;
	int ret;

	if (!fw_info->cfg_raw_data) {
		dev_info(dev, "No cfg data in file\n");
		ret = mxt_write_config_from_pdata(data);
		return ret;
	}

	/* Get config CRC from device */
	ret = mxt_read_config_crc(data, &current_crc);
	if (ret)
		return ret;

	/* Check Version information */
	if (fw_info->fw_ver != data->info.version) {
		dev_err(dev, "Warning: version mismatch! %s\n", __func__);
		return 0;
	}
	if (fw_info->build_ver != data->info.build) {
		dev_err(dev, "Warning: build num mismatch! %s\n", __func__);
		return 0;
	}

	/* Check config CRC */
	if (current_crc == fw_info->cfg_crc) {
		dev_info(dev, "Skip writing Config:[CRC 0x%06X]\n",
			current_crc);
		return 0;
	}

	dev_info(dev, "Writing Config:[CRC 0x%06X!=0x%06X]\n",
		current_crc, fw_info->cfg_crc);

	/* Write config info */
	for (index = 0; index < fw_info->cfg_len;) {

		if (index + sizeof(struct mxt_cfg_data) >= fw_info->cfg_len) {
			dev_err(dev, "index(%d) of cfg_data exceeded total size(%d)!!\n",
				index + sizeof(struct mxt_cfg_data),
				fw_info->cfg_len);
			return -EINVAL;
		}

		/* Get the info about each object */
		cfg_data = (struct mxt_cfg_data *)
					(&fw_info->cfg_raw_data[index]);

		index += sizeof(struct mxt_cfg_data) + cfg_data->size;
		if (index > fw_info->cfg_len) {
			dev_err(dev, "index(%d) of cfg_data exceeded total size(%d) in T%d object!!\n",
				index, fw_info->cfg_len, cfg_data->type);
			return -EINVAL;
		}

		object = mxt_get_object(data, cfg_data->type);
		if (!object) {
			dev_err(dev, "T%d is Invalid object type\n",
				cfg_data->type);
			return -EINVAL;
		}

		/* Check and compare the size, instance of each object */
		if (cfg_data->size > object->size) {
			dev_err(dev, "T%d Object length exceeded!\n",
				cfg_data->type);
			return -EINVAL;
		}
		if (cfg_data->instance >= object->instances) {
			dev_err(dev, "T%d Object instances exceeded!\n",
				cfg_data->type);
			return -EINVAL;
		}

		dev_dbg(dev, "Writing config for obj %d len %d instance %d (%d/%d)\n",
			cfg_data->type, object->size,
			cfg_data->instance, index, fw_info->cfg_len);

		reg = object->start_address + object->size * cfg_data->instance;

		/* Write register values of each object */
		ret = mxt_write_mem(data, reg, cfg_data->size,
					 cfg_data->register_val);
		if (ret) {
			dev_err(dev, "Write T%d Object failed\n",
				object->object_type);
			return ret;
		}

		/*
		 * If firmware is upgraded, new bytes may be added to end of
		 * objects. It is generally forward compatible to zero these
		 * bytes - previous behaviour will be retained. However
		 * this does invalidate the CRC and will force a config
		 * download every time until the configuration is updated.
		 */
		if (cfg_data->size < object->size) {
			dev_err(dev, "Warning: zeroing %d byte(s) in T%d\n",
				 object->size - cfg_data->size, cfg_data->type);

			for (i = cfg_data->size + 1; i < object->size; i++) {
				ret = mxt_write_mem(data, reg + i, 1, &val);
				if (ret)
					return ret;
			}
		}
	}
	dev_info(dev, "Updated configuration\n");

	return ret;
}
#endif


static int mxt_calibrate_chip(struct mxt_data *data)
{
	u8 cal_data = 1;
	int ret = 0;
	/* send calibration command to the chip */
	ret = mxt_write_mem(data,
		data->cmd_proc + CMD_CALIBRATE_OFFSET,
		1, &cal_data);
	if (!ret)
		dev_info(&data->client->dev, "success sending calibration cmd!!!\n");
	return ret;
}

#if TSP_INFORM_CHARGER
static int set_charger_config(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	struct mxt_object *object;
	struct mxt_cfg_data *cfg_data;
	u8 i, val = 0;
	u16 reg, index;
	int ret;

	dev_dbg(dev, "set_charger_config data->cfg_len = %d\n", data->cfg_len);

	for (index = 0; index < data->cfg_len;) {
		if (index + sizeof(struct mxt_cfg_data) >= data->cfg_len) {
			dev_err(dev, "index(%d) of cfg_data exceeded total size(%d)!!\n",
				index + sizeof(struct mxt_cfg_data),
				data->cfg_len);
			return -EINVAL;
		}

		/* Get the info about each object */
		if (data->charging_mode)
			cfg_data = (struct mxt_cfg_data *)
					(&data->ta_cfg_raw_data[index]);
		else
			cfg_data = (struct mxt_cfg_data *)
					(&data->batt_cfg_raw_data[index]);

		index += sizeof(struct mxt_cfg_data) + cfg_data->size;
		if (index > data->cfg_len) {
			dev_err(dev, "index(%d) of cfg_data exceeded total size(%d) in T%d object!!\n",
				index, data->cfg_len, cfg_data->type);
			return -EINVAL;
		}

		object = mxt_get_object(data, cfg_data->type);
		if (!object) {
			dev_err(dev, "T%d is Invalid object type\n",
				cfg_data->type);
			return -EINVAL;
		}

		/* Check and compare the size, instance of each object */
		if (cfg_data->size > object->size) {
			dev_err(dev, "T%d Object length exceeded!\n",
				cfg_data->type);
			return -EINVAL;
		}
		if (cfg_data->instance >= object->instances) {
			dev_err(dev, "T%d Object instances exceeded!\n",
				cfg_data->type);
			return -EINVAL;
		}

		dev_dbg(dev, "Writing config for obj %d len %d instance %d (%d/%d)\n",
			cfg_data->type, object->size,
			cfg_data->instance, index, data->cfg_len);

		reg = object->start_address + object->size * cfg_data->instance;

		/* Write register values of each object */
		ret = mxt_write_mem(data, reg, cfg_data->size,
					 cfg_data->register_val);
		if (ret) {
			dev_err(dev, "Write T%d Object failed\n",
				object->object_type);
			return ret;
		}

		/*
		 * If firmware is upgraded, new bytes may be added to end of
		 * objects. It is generally forward compatible to zero these
		 * bytes - previous behaviour will be retained. However
		 * this does invalidate the CRC and will force a config
		 * download every time until the configuration is updated.
		 */
		if (cfg_data->size < object->size) {
			dev_err(dev, "Warning: zeroing %d byte(s) in T%d\n",
				 object->size - cfg_data->size, cfg_data->type);

			for (i = cfg_data->size + 1; i < object->size; i++) {
				ret = mxt_write_mem(data, reg + i, 1, &val);
				if (ret)
					return ret;
			}
		}
	}

	return ret;
}

static void inform_charger(struct mxt_callbacks *cb,
	bool en)
{
	struct mxt_data *data = container_of(cb,
			struct mxt_data, callbacks);

	cancel_delayed_work_sync(&data->noti_dwork);
	data->charging_mode = en;
	schedule_delayed_work(&data->noti_dwork, HZ / 5);
}

static void charger_noti_dwork(struct work_struct *work)
{
	struct mxt_data *data =
		container_of(work, struct mxt_data,
		noti_dwork.work);

	if (!data->mxt_enabled) {
		schedule_delayed_work(&data->noti_dwork, HZ / 5);
		return ;
	}

	dev_info(&data->client->dev,
		"%s mode\n",
		data->charging_mode ? "charging" : "battery");

	set_charger_config(data);
}

static void inform_charger_init(struct mxt_data *data)
{
	INIT_DELAYED_WORK(&data->noti_dwork, charger_noti_dwork);
}
#endif

#if CHECK_ANTITOUCH
void mxt_t61_timer_set(struct mxt_data *data, u8 mode, u8 cmd, u16 ms_period)
{
	struct mxt_object *object;
	int ret = 0;
	u8 buf[5] = {3, 0, 0, 0, 0};

	object = mxt_get_object(data, SPT_TIMER_T61);

	buf[1] = cmd;
	buf[2] = mode;
	buf[3] = ms_period & 0xFF;
	buf[4] = (ms_period >> 8) & 0xFF;

	ret = mxt_write_mem(data, object->start_address, 5, buf);
	if (ret)
		dev_err(&data->client->dev,
			"%s write error T%d address[0x%x]\n",
			__func__, SPT_TIMER_T61, object->start_address);

	if (ms_period)
		dev_info(&data->client->dev,
			"T61 Timer Enabled %d\n", ms_period);
}

void mxt_t8_autocal_set(struct mxt_data *data, u8 mstime)
{
	struct mxt_object *object;
	int ret = 0;

	if (mstime) {
		data->check_autocal = 1;
#if TSP_DEBUG_INFO
		dev_info(&data->client->dev,
			"T8 Autocal Enabled %d\n", mstime);
#endif
	} else {
		data->check_autocal = 0;
#if TSP_DEBUG_INFO
		dev_info(&data->client->dev,
			"T8 Autocal Disabled %d\n", mstime);
#endif
	}

	object =	mxt_get_object(data,
		GEN_ACQUISITIONCONFIG_T8);

	ret = mxt_write_mem(data,
			object->start_address + 4,
			1, &mstime);
	if (ret)
		dev_err(&data->client->dev,
			"%s write error T%d address[0x%x]\n",
			__func__, SPT_TIMER_T61, object->start_address);
}

static void mxt_check_coordinate(struct mxt_data *data,
	bool detect, u8 id, u16 *px, u16 *py)
{
	static int tcount[] = {0, };
	static u16 pre_x[][4] = {{0}, };
	static u16 pre_y[][4] = {{0}, };
	int distance = 0;

	if (detect)
		tcount[id] = 0;

	if (tcount[id] > 1) {
		distance = abs(pre_x[id][0] - *px) + abs(pre_y[id][0] - *py);
#if TSP_DEBUG_INFO
		dev_info(&data->client->dev,
			"Check Distance ID:%d, %d\n",
			id, distance);
#endif
		/* AutoCal Disable */
		if (distance > 3)
			mxt_t8_autocal_set(data, 0);
	}

	pre_x[id][0] = *px;
	pre_y[id][0] = *py;

	tcount[id]++;
}
#endif	/* CHECK_ANTITOUCH */

static void mxt_report_input_data(struct mxt_data *data)
{
	int i;
	int count = 0;
	int report_count = 0;

	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].state == MXT_STATE_INACTIVE)
			continue;

		if (data->fingers[i].state == MXT_STATE_RELEASE) {
			input_mt_slot(data->input_dev, i);
			input_mt_report_slot_state(data->input_dev,
					MT_TOOL_FINGER, false);
		} else {
			input_mt_slot(data->input_dev, i);
			input_mt_report_slot_state(data->input_dev,
					MT_TOOL_FINGER, true);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X,
					data->fingers[i].x);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
					data->fingers[i].y);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
					data->fingers[i].w);
			input_report_abs(data->input_dev, ABS_MT_PRESSURE,
					 data->fingers[i].z);
#if TSP_USE_SHAPETOUCH
			input_report_abs(data->input_dev, ABS_MT_COMPONENT,
					 data->fingers[i].component);
			input_report_abs(data->input_dev, ABS_MT_SUMSIZE,
					 data->sumsize);
#endif
		}
		report_count++;

#if TSP_DEBUG_INFO
		if (data->fingers[i].state == MXT_STATE_PRESS)
			dev_info(&data->client->dev, "P: id[%d] X[%d],Y[%d]\n",
				i, data->fingers[i].x, data->fingers[i].y);
#else
		if (data->fingers[i].state == MXT_STATE_PRESS)
			dev_info(&data->client->dev, "P: id[%d]\n", i);
#endif
		else if (data->fingers[i].state == MXT_STATE_RELEASE)
			dev_info(&data->client->dev, "R: id[%d] M[%d]\n",
				i, data->fingers[i].mcount);

		if (data->fingers[i].state == MXT_STATE_RELEASE) {
			data->fingers[i].state = MXT_STATE_INACTIVE;
			data->fingers[i].mcount = 0;
		} else {
			data->fingers[i].state = MXT_STATE_MOVE;
			count++;
		}
	}

	if (report_count > 0) {
#if TSP_ITDEV
		if (!data->driver_paused)
#endif
			input_sync(data->input_dev);
	}

#if (TSP_USE_SHAPETOUCH || TSP_BOOSTER)
	/* all fingers are released */
	if (count == 0) {
#if TSP_USE_SHAPETOUCH
		data->sumsize = 0;
#endif
#if TSP_BOOSTER
		mxt_set_dvfs_on(data, false);
#endif
	}
#endif

	data->finger_mask = 0;
}

static void mxt_treat_T6_object(struct mxt_data *data, u8 *msg)
{
	/* normal mode */
	if (msg[1] == 0x00)
		dev_info(&data->client->dev, "normal mode\n");
	/* I2C checksum error */
	if (msg[1] & 0x04)
		dev_err(&data->client->dev, "I2C checksum error\n");
	/* config error */
	if (msg[1] & 0x08)
		dev_err(&data->client->dev, "config error\n");
	/* calibration */
	if (msg[1] & 0x10) {
		dev_info(&data->client->dev, "calibration is on going !!\n");
#if CHECK_ANTITOUCH
		/* After Calibration */
		data->check_antitouch = 1;
		mxt_t61_timer_set(data,
			MXT_T61_TIMER_ONESHOT,
			MXT_T61_TIMER_CMD_STOP, 0);
		data->check_timer = 0;
		data->check_calgood = 0;
#endif
	}
	/* signal error */
	if (msg[1] & 0x20)
		dev_err(&data->client->dev, "signal error\n");
	/* overflow */
	if (msg[1] & 0x40)
		dev_err(&data->client->dev, "overflow detected\n");
	/* reset */
	if (msg[1] & 0x80) {
		dev_info(&data->client->dev, "reset is ongoing\n");
#if TSP_INFORM_CHARGER
		if (data->charging_mode)
			set_charger_config(data);
#endif
#if TSP_SEC_SYSFS
		data->sysfs_data->current_crc = msg[2]
				| (msg[3] << 8) | (msg[4] << 16);
		dev_dbg(&data->client->dev, "CRC [0x%06X]\n",
			 data->sysfs_data->current_crc);
#endif
	}
}

static void mxt_treat_T9_object(struct mxt_data *data, u8 *msg)
{
	int id;

	id = msg[0] - data->finger_report_id;

	/* If not a touch event, return */
	if (id < 0 || id >= data->num_fingers)
		return;

	if (msg[1] & RELEASE_MSG_MASK) {
		data->fingers[id].z = 0;
		data->fingers[id].w = msg[5];
		data->fingers[id].state = MXT_STATE_RELEASE;
		mxt_report_input_data(data);
	} else if ((msg[1] & DETECT_MSG_MASK)
		&& (msg[1] &	(PRESS_MSG_MASK | MOVE_MSG_MASK))) {
		data->fingers[id].z = msg[6];
		data->fingers[id].w = msg[5];
		data->fingers[id].x = (((msg[2] << 4) | (msg[4] >> 4))
			>> data->x_dropbits);
		data->fingers[id].y = (((msg[3] << 4) | (msg[4] & 0xF))
			>> data->y_dropbits);

#if TSP_USE_SHAPETOUCH
		data->fingers[id].component = msg[7];
#endif
		data->finger_mask |= 1U << id;

		if (msg[1] & PRESS_MSG_MASK) {
			data->fingers[id].state = MXT_STATE_PRESS;
			data->fingers[id].mcount = 0;
#if CHECK_ANTITOUCH
			if (data->check_autocal)
				mxt_check_coordinate(data, 1, id,
					&data->fingers[id].x,
					&data->fingers[id].y);
#endif
		} else if (msg[1] & MOVE_MSG_MASK) {
			data->fingers[id].mcount += 1;
#if CHECK_ANTITOUCH
			if (data->check_autocal)
				mxt_check_coordinate(data, 0, id,
					&data->fingers[id].x,
					&data->fingers[id].y);
#endif
		}

#if TSP_BOOSTER
		mxt_set_dvfs_on(data, true);
#endif
	} else if ((msg[1] & SUPPRESS_MSG_MASK)
		&& (data->fingers[id].state != MXT_STATE_INACTIVE)) {
		data->fingers[id].z = 0;
		data->fingers[id].w = msg[5];
		data->fingers[id].state = MXT_STATE_RELEASE;
		data->finger_mask |= 1U << id;
	} else {
		/* ignore changed amplitude and vector messsage */
		if (!((msg[1] & DETECT_MSG_MASK)
				&& (msg[1] & AMPLITUDE_MSG_MASK
				 || msg[1] & VECTOR_MSG_MASK)))
			dev_err(&data->client->dev, "Unknown state %#02x %#02x\n",
				msg[0], msg[1]);
	}
}

static void mxt_treat_T42_object(struct mxt_data *data, u8 *msg)
{
	if (msg[1] & 0x01)
		/* Palm Press */
		dev_info(&data->client->dev, "palm touch detected\n");
	else
		/* Palm release */
		dev_info(&data->client->dev, "palm touch released\n");
}

static void mxt_treat_T57_object(struct mxt_data *data, u8 *msg)
{
#if CHECK_ANTITOUCH
	u16 tch_area = 0, atch_area = 0;

	tch_area = msg[3] | (msg[4] << 8);
	atch_area = msg[5] | (msg[6] << 8);
	if (data->check_antitouch) {
		if (tch_area) {
#if TSP_DEBUG_INFO
			dev_info(&data->client->dev,
				"TCHAREA=%d\n", tch_area);
#endif
			/* First Touch After Calibration */
			if (data->check_timer == 0) {
				mxt_t61_timer_set(data,
					MXT_T61_TIMER_ONESHOT,
					MXT_T61_TIMER_CMD_START,
					3000);
				data->check_timer = 1;
			}

			if (tch_area <= 2)
				mxt_t8_autocal_set(data, 5);

			if (atch_area > 5) {
				if (atch_area - tch_area > 0) {
#if TSP_DEBUG_INFO
					dev_info(&data->client->dev,
						"Case:1 TCHAREA=%d ATCHAREA=%d\n",
						tch_area, atch_area);
#endif
					mxt_calibrate_chip(data);
				}
			}

		} else {
			if (atch_area) {
				/* Only Anti-touch */
#if TSP_DEBUG_INFO
				dev_info(&data->client->dev,
					"Case:2 TCHAREA=%d ATCHAREA=%d\n",
					tch_area, atch_area);
#endif
				mxt_calibrate_chip(data);
			}
		}
	}

	if (data->check_calgood == 1) {
		if (tch_area) {
			if ((atch_area - tch_area) > 8) {
				if (tch_area < 35) {
#if TSP_DEBUG_INFO
					dev_info(&data->client->dev,
						"Cal Not Good 1 - %d %d\n",
						atch_area, tch_area);
#endif
					mxt_calibrate_chip(data);
				}
			}
			if (((tch_area - atch_area) >= 40) &&
				(atch_area > 4)) {
#if TSP_DEBUG_INFO
				dev_info(&data->client->dev,
					"Cal Not Good 2 - %d %d\n",
					atch_area, tch_area);
#endif
				mxt_calibrate_chip(data);
			}
		} else {
			if (atch_area) {
				/* Only Anti-touch */
#if TSP_DEBUG_INFO
				dev_info(&data->client->dev,
					"Cal Not Good 3 - %d %d\n",
					atch_area, tch_area);
#endif
				mxt_calibrate_chip(data);
			}
		}
	}
#endif	/* CHECK_ANTITOUCH */

#if TSP_USE_SHAPETOUCH
	data->sumsize = msg[1] + (msg[2] << 8);
#endif	/* TSP_USE_SHAPETOUCH */

}

#if CHECK_ANTITOUCH
static void mxt_treat_T61_object(struct mxt_data *data, u8 *msg)
{
	if ((msg[1] & 0xa0) == 0xa0) {

		if (data->check_calgood == 1)
			data->check_calgood = 0;

		if (data->check_antitouch) {
#if TSP_DEBUG_INFO
			dev_info(&data->client->dev,
				"SPT_TIMER_T61 Stop\n");
#endif
			data->check_antitouch = 0;
			data->check_timer = 0;
			mxt_t8_autocal_set(data, 0);
			data->check_calgood = 1;
			mxt_t61_timer_set(data,
				MXT_T61_TIMER_ONESHOT,
				MXT_T61_TIMER_CMD_START, 10000);
		}
	}
}
#endif	/* CHECK_ANTITOUCH */

static irqreturn_t mxt_irq_thread(int irq, void *ptr)
{
	struct mxt_data *data = ptr;
	u8 msg[data->msg_object_size];
	u8 object_type, instance;

	do {
		if (mxt_read_mem(data, data->msg_proc, sizeof(msg), msg))
			return IRQ_HANDLED;

#if TSP_ITDEV
		if (data->debug_enabled)
			print_hex_dump(KERN_INFO, "MXT MSG:",
				DUMP_PREFIX_NONE, 16, 1,
				msg, sizeof(msg), false);
#endif	/* TSP_ITDEV */
		object_type = mxt_reportid_to_type(data, msg[0] , &instance);

		if (object_type == RESERVED_T0)
			continue;

		switch (object_type) {
		case GEN_COMMANDPROCESSOR_T6:
			mxt_treat_T6_object(data, msg);
			break;
		case TOUCH_MULTITOUCHSCREEN_T9:
			mxt_treat_T9_object(data, msg);
			break;
		case PROCI_TOUCHSUPPRESSION_T42:
			mxt_treat_T42_object(data, msg);
			break;

		case PROCI_EXTRATOUCHSCREENDATA_T57:
			mxt_treat_T57_object(data, msg);
			break;
#if CHECK_ANTITOUCH
		case SPT_TIMER_T61:
			mxt_treat_T61_object(data, msg);
			break;
#endif	/* CHECK_ANTITOUCH */
		case PROCG_NOISESUPPRESSION_T62:
			break;
		default:
			dev_err(&data->client->dev,
			"Untreated Object type[%d]\tmessage[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]\n",
				object_type, msg[0], msg[1], msg[2],
				msg[3], msg[4], msg[5], msg[6], msg[7], msg[8]);
			break;
		}
	} while (!gpio_get_value(data->pdata->gpio_read_done));

	if (data->finger_mask)
		mxt_report_input_data(data);

	return IRQ_HANDLED;
}

/* This API should be deleted later */
/* dummy API for cypress touchkey */
int get_tsp_status(void)
{
	return 0;
}
EXPORT_SYMBOL(get_tsp_status);

static int mxt_internal_suspend(struct mxt_data *data)
{
	int i;
	int count = 0;

	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].state == MXT_STATE_INACTIVE)
			continue;
		data->fingers[i].z = 0;
		data->fingers[i].state = MXT_STATE_RELEASE;
		count++;
	}
	if (count)
		mxt_report_input_data(data);

#if TSP_BOOSTER
	mxt_set_dvfs_on(data, false);
#endif

	data->pdata->power_off();

	return 0;
}

static int mxt_internal_resume(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int ret;

	data->pdata->power_on();

	/* Soft reset */
	ret = mxt_reset(data);
	if (ret) {
		dev_err(dev, "Failed Reset IC\n");
		goto out;
	}
	msleep(MXT_540S_SW_RESET_TIME);

out:
	return ret;
}

static int mxt_get_bootloader_version(struct i2c_client *client, u8 val)
{
	u8 buf[3];

	if (val & MXT_BOOT_EXTENDED_ID) {
		if (i2c_master_recv(client, buf, sizeof(buf)) != sizeof(buf)) {
			dev_err(&client->dev, "%s: i2c recv failed\n",
				 __func__);
			return -EIO;
		}
		dev_info(&client->dev, "Bootloader ID:%d Version:%d",
			 buf[1], buf[2]);
	} else {
		dev_info(&client->dev, "Bootloader ID:%d",
			 val & MXT_BOOT_ID_MASK);
	}
	return 0;
}

static int mxt_check_bootloader(struct i2c_client *client,
				     unsigned int state)
{
	u8 val;

recheck:
	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
		return -EIO;
	}

	switch (state) {
	case MXT_WAITING_BOOTLOAD_CMD:
		if (mxt_get_bootloader_version(client, val))
			return -EIO;
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_WAITING_FRAME_DATA:
	case MXT_APP_CRC_FAIL:
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_FRAME_CRC_PASS:
		if (val == MXT_FRAME_CRC_CHECK)
			goto recheck;
		if (val == MXT_FRAME_CRC_FAIL) {
			dev_err(&client->dev, "Bootloader CRC fail\n");
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	if (val != state) {
		dev_err(&client->dev,
			 "Invalid bootloader mode state 0x%X\n", val);
		return -EINVAL;
	}

	return 0;
}

static int mxt_unlock_bootloader(struct i2c_client *client)
{
	u8 buf[2] = {MXT_UNLOCK_CMD_LSB, MXT_UNLOCK_CMD_MSB};

	if (i2c_master_send(client, buf, 2) != 2) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);

		return -EIO;
	}

	return 0;
}

static int mxt_probe_bootloader(struct i2c_client *client)
{
	u8 val;

	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
		return -EIO;
	}

	if (val & (~MXT_BOOT_STATUS_MASK)) {
		if (val & MXT_APP_CRC_FAIL)
			dev_err(&client->dev, "Application CRC failure\n");
		else
			dev_err(&client->dev, "Device in bootloader mode\n");
	} else {
		dev_err(&client->dev, "%s: Unknow status\n", __func__);
		return -EIO;
	}
	return 0;
}

static int mxt_fw_write(struct i2c_client *client,
				const u8 *frame_data, unsigned int frame_size)
{
	if (i2c_master_send(client, frame_data, frame_size) != frame_size) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

#if DUAL_CFG
int mxt_verify_fw(struct mxt_fw_info *fw_info, const struct firmware *fw)
{
	struct mxt_data *data = fw_info->data;
	struct device *dev = &data->client->dev;
	struct mxt_fw_image *fw_img;

	if (!fw) {
		dev_err(dev, "could not find firmware file\n");
		return -ENOENT;
	}

	fw_img = (struct mxt_fw_image *)fw->data;

	if (le32_to_cpu(fw_img->magic_code) != MXT_FW_MAGIC) {
		/* In case, firmware file only consist of firmware */
		dev_info(dev, "Firmware file only consist of raw firmware\n");
		fw_info->fw_len = fw->size;
		fw_info->fw_raw_data = fw->data;
	} else {
		/*
		 * In case, firmware file consist of header,
		 * configuration, firmware.
		 */
		dev_info(dev, "Firmware file consist of header, configuration, firmware\n");
		fw_info->fw_ver = fw_img->fw_ver;
		fw_info->build_ver = fw_img->build_ver;
		fw_info->hdr_len = le32_to_cpu(fw_img->hdr_len);
		fw_info->cfg_len = le32_to_cpu(fw_img->cfg_len);
		fw_info->fw_len = le32_to_cpu(fw_img->fw_len);
		fw_info->cfg_crc = le32_to_cpu(fw_img->cfg_crc);

		/* Check the firmware file with header */
		if (fw_info->hdr_len != sizeof(struct mxt_fw_image)
			|| fw_info->hdr_len + fw_info->cfg_len
				+ fw_info->fw_len != fw->size) {
			dev_err(dev, "Firmware file is invaild !!hdr size[%d] cfg,fw size[%d,%d] filesize[%d]\n",
				fw_info->hdr_len, fw_info->cfg_len,
				fw_info->fw_len, fw->size);
			return -EINVAL;
		}

		if (!fw_info->cfg_len) {
			dev_err(dev, "Firmware file dose not include configuration data\n");
			return -EINVAL;
		}
		if (!fw_info->fw_len) {
			dev_err(dev, "Firmware file dose not include raw firmware data\n");
			return -EINVAL;
		}

		/* Get the address of configuration data */
		data->cfg_len = fw_info->cfg_len / 2;
		data->batt_cfg_raw_data = fw_info->batt_cfg_raw_data
			= fw_img->data;
		data->ta_cfg_raw_data = fw_info->ta_cfg_raw_data
			= fw_img->data +  (fw_info->cfg_len / 2) ;

		/* Get the address of firmware data */
		fw_info->fw_raw_data = fw_img->data + fw_info->cfg_len;

#if TSP_SEC_SYSFS
		data->sysfs_data->fw_ver = fw_info->fw_ver;
		data->sysfs_data->build_ver = fw_info->build_ver;
#endif
	}

	return 0;
}
#else
int mxt_verify_fw(struct mxt_fw_info *fw_info, const struct firmware *fw)
{
	struct mxt_data *data = fw_info->data;
	struct device *dev = &data->client->dev;
	struct mxt_fw_image *fw_img;

	if (!fw) {
		dev_err(dev, "could not find firmware file\n");
		return -ENOENT;
	}

	fw_img = (struct mxt_fw_image *)fw->data;

	if (le32_to_cpu(fw_img->magic_code) != MXT_FW_MAGIC) {
		/* In case, firmware file only consist of firmware */
		dev_dbg(dev, "Firmware file only consist of raw firmware\n");
		fw_info->fw_len = fw->size;
		fw_info->fw_raw_data = fw->data;
	} else {
		/*
		 * In case, firmware file consist of header,
		 * configuration, firmware.
		 */
		dev_dbg(dev, "Firmware file consist of header, configuration, firmware\n");
		fw_info->fw_ver = fw_img->fw_ver;
		fw_info->build_ver = fw_img->build_ver;
		fw_info->hdr_len = le32_to_cpu(fw_img->hdr_len);
		fw_info->cfg_len = le32_to_cpu(fw_img->cfg_len);
		fw_info->fw_len = le32_to_cpu(fw_img->fw_len);
		fw_info->cfg_crc = le32_to_cpu(fw_img->cfg_crc);

		/* Check the firmware file with header */
		if (fw_info->hdr_len != sizeof(struct mxt_fw_image)
			|| fw_info->hdr_len + fw_info->cfg_len
				+ fw_info->fw_len != fw->size) {
			dev_err(dev, "Firmware file is invaild !!hdr size[%d] cfg,fw size[%d,%d] filesize[%d]\n",
				fw_info->hdr_len, fw_info->cfg_len,
				fw_info->fw_len, fw->size);
			return -EINVAL;
		}

		if (!fw_info->cfg_len) {
			dev_err(dev, "Firmware file dose not include configuration data\n");
			return -EINVAL;
		}
		if (!fw_info->fw_len) {
			dev_err(dev, "Firmware file dose not include raw firmware data\n");
			return -EINVAL;
		}

		/* Get the address of configuration data */
		fw_info->cfg_raw_data = fw_img->data;

		/* Get the address of firmware data */
		fw_info->fw_raw_data = fw_img->data + fw_info->cfg_len;

#if TSP_SEC_SYSFS
		data->sysfs_data->fw_ver = fw_info->fw_ver;
		data->sysfs_data->build_ver = fw_info->build_ver;
#endif
	}

	return 0;
}
#endif

static int mxt_flash_fw(struct mxt_fw_info *fw_info)
{
	struct mxt_data *data = fw_info->data;
	struct i2c_client *client = data->client_boot;
	struct device *dev = &data->client->dev;
	const u8 *fw_data = fw_info->fw_raw_data;
	size_t fw_size = fw_info->fw_len;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;

	if (!fw_data) {
		dev_err(dev, "firmware data is Null\n");
		return -ENOMEM;
	}

	ret = mxt_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);
	if (ret) {
		/*may still be unlocked from previous update attempt */
		ret = mxt_check_bootloader(client, MXT_WAITING_FRAME_DATA);
		if (ret)
			goto out;
	} else {
		dev_info(dev, "Unlocking bootloader\n");
		/* Unlock bootloader */
		mxt_unlock_bootloader(client);
	}
	while (pos < fw_size) {
		ret = mxt_check_bootloader(client,
					MXT_WAITING_FRAME_DATA);
		if (ret) {
			dev_err(dev,	"Fail updating firmware. wating_frame_data err\n");
			goto out;
		}

		frame_size = ((*(fw_data + pos) << 8) | *(fw_data + pos + 1));

		/*
		* We should add 2 at frame size as the the firmware data is not
		* included the CRC bytes.
		*/

		frame_size += 2;

		/* Write one frame to device */
		mxt_fw_write(client, fw_data + pos, frame_size);

		ret = mxt_check_bootloader(client,
						MXT_FRAME_CRC_PASS);
		if (ret) {
			dev_err(dev, "Fail updating firmware. frame_crc err\n");
			goto out;
		}

		pos += frame_size;

		dev_dbg(dev, "Updated %d bytes / %zd bytes\n",
				pos, fw_size);

		msleep(20);
	}
	dev_info(dev, "success updating firmware\n");

	msleep(MXT_540S_FW_RESET_TIME);
out:
	return ret;
}

int  mxt_initialize(struct mxt_data *data)
{
	struct i2c_client *client = data->client;

	u32 read_info_crc, calc_info_crc;
	int ret;

	ret = mxt_get_id_info(data);
	if (ret)
		return ret;

	data->objects = kcalloc(data->info.object_num,
				sizeof(*data->objects),
				GFP_KERNEL);
	if (!data->objects) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto out;
	}

	/* Get object table */
	ret = mxt_get_object_table(data);
	if (ret)
		goto out;

	data->rid_map = kcalloc(data->max_report_id + 1,
			sizeof(*data->rid_map),
			GFP_KERNEL);

	if (!data->rid_map) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto out;
	}

	/* Make report id table */
	mxt_make_reportid_table(data);

	/* Verify the info CRC */
	ret = mxt_read_info_crc(data, &read_info_crc);
	if (ret)
		goto out;

	ret = mxt_calculate_infoblock_crc(data, &calc_info_crc);
	if (ret)
		goto out;

	if (read_info_crc != calc_info_crc) {
		dev_err(&data->client->dev, "CRC error :[CRC 0x%06X!=0x%06X]\n",
				read_info_crc, calc_info_crc);
		ret = -EFAULT;
		goto out;
	}
	return 0;

out:
	return ret;
}

static void mxt_handle_init_data(struct mxt_data *data)
{
	const struct mxt_platform_data *pdata = data->pdata;
	int ret;
	u8 val;

	/* Set touchscreen resolution */
	mxt_write_object(data, TOUCH_MULTITOUCHSCREEN_T9,
			MXT_T9_XRANGE_LSB, (pdata->max_x) & 0xff);
	mxt_write_object(data, TOUCH_MULTITOUCHSCREEN_T9,
			MXT_T9_XRANGE_MSB, (pdata->max_x) >> 8);
	mxt_write_object(data, TOUCH_MULTITOUCHSCREEN_T9,
			MXT_T9_YRANGE_LSB, (pdata->max_y) & 0xff);
	mxt_write_object(data, TOUCH_MULTITOUCHSCREEN_T9,
			MXT_T9_YRANGE_MSB, (pdata->max_y) >> 8);

	/* Get acquistion time */
	ret = mxt_read_object(data, TOUCH_MULTITOUCHSCREEN_T9,
		MXT_T9_CTRL, &val);

	if (ret)
		data->tsp_ctrl = 0x83;
	else
		data->tsp_ctrl = (val > 0) ? val : 0x83;

	mxt_write_object(data, TOUCH_MULTITOUCHSCREEN_T9,
			MXT_T9_CTRL, data->tsp_ctrl);
	dev_info(&data->client->dev, "T9 CTRL : %d\n", data->tsp_ctrl);

	/* Get num of x,y lines that object occupies */
	ret = mxt_read_object(data, TOUCH_MULTITOUCHSCREEN_T9,
		MXT_T9_X_SIZE, &val);
	data->x_num = val;

	ret = mxt_read_object(data, TOUCH_MULTITOUCHSCREEN_T9,
		MXT_T9_Y_SIZE, &val);
	data->y_num = val;
	dev_info(&data->client->dev, "occupied matrix X,Y size:  %d,%d\n",
		data->x_num, data->y_num);
}

int  mxt_rest_initialize(struct mxt_fw_info *fw_info)
{
	struct mxt_data *data = fw_info->data;
	struct device *dev = &data->client->dev;
	int ret;

	/* Write config */
	ret = mxt_write_config(fw_info);
	if (ret) {
		dev_err(dev, "Failed to write config from file\n");
		goto out;
	}

	/* Handle data for init */
	mxt_handle_init_data(data);

	/* Backup to memory */
	ret = mxt_backup(data);
	if (ret) {
		dev_err(dev, "Failed backup NV data\n");
		goto out;
	}

	/* Soft reset */
	ret = mxt_reset(data);
	if (ret) {
		dev_err(dev, "Failed Reset IC\n");
		goto out;
	}
	msleep(MXT_540S_SW_RESET_TIME);

out:
	return ret;
}

static int mxt_enter_bootloader(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int error;

	data->objects = kcalloc(data->info.object_num,
				     sizeof(struct mxt_object),
				     GFP_KERNEL);
	if (!data->objects) {
		dev_err(dev, "%s Failed to allocate memory\n",
			__func__);
		error = -ENOMEM;
		goto out;
	}

	/* Get object table information*/
	error = mxt_get_object_table(data);
	if (error)
		goto err_free_mem;

	/* Change to the bootloader mode */
	mxt_write_object(data, GEN_COMMANDPROCESSOR_T6,
			CMD_RESET_OFFSET, MXT_BOOT_VALUE);

	msleep(MXT_540S_SW_RESET_TIME);

err_free_mem:
	kfree(data->objects);
	data->objects = NULL;

out:
	return error;
}

static int mxt_flash_fw_on_probe(struct mxt_fw_info *fw_info)
{
	struct mxt_data *data = fw_info->data;
	struct device *dev = &data->client->dev;
	int error;

	error = mxt_get_id_info(data);
	if (error) {
		/* need to check IC is in boot mode */
		error = mxt_probe_bootloader(data->client_boot);
		if (error) {
			dev_err(dev, "Failed to verify bootloader's status\n");
			goto out;
		}

		dev_info(dev, "Updating firmware from boot-mode\n");
		goto load_fw;
	}

	/* compare the version to verify necessity of firmware updating */
	if (data->info.version == fw_info->fw_ver
			&& data->info.build == fw_info->build_ver) {
		dev_dbg(dev, "Firmware version is same with in IC\n");
		goto out;
	}

	dev_info(dev, "Updating firmware from app-mode : IC:0x%x,0x%x =! FW:0x%x,0x%x\n",
			data->info.version, data->info.build,
			fw_info->fw_ver, fw_info->build_ver);

	error = mxt_enter_bootloader(data);
	if (error) {
		dev_err(dev, "Failed updating firmware\n");
		goto out;
	}

load_fw:
	error = mxt_flash_fw(fw_info);
	if (error)
		dev_err(dev, "Failed updating firmware\n");
	else
		dev_info(dev, "succeeded updating firmware\n");
out:
	return error;
}

static int mxt_touch_finish_init(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int error;

	error = request_threaded_irq(client->irq, NULL, mxt_irq_thread,
		IRQF_TRIGGER_LOW | IRQF_ONESHOT, "mxt_ts", data);

	if (error) {
		dev_err(&client->dev, "Failed to register interrupt\n");
		goto err_req_irq;
	}

#if TSP_BOOSTER
	error = mxt_init_dvfs(data);
	if (error < 0) {
		dev_err(&client->dev, "Fail get dvfs level for touch booster\n");
		goto err_req_irq;
	}
#endif

	/*
	* to prevent unnecessary report of touch event
	* it will be enabled in open function
	*/
	mxt_stop(data);

	dev_info(&client->dev,  "Mxt touch controller initialized\n");

	/* for blocking to be excuted open function untile finishing ts init */
	complete_all(&data->init_done);
	return 0;

err_req_irq:
	return error;
}

static int mxt_touch_rest_init(struct mxt_fw_info *fw_info)
{
	struct mxt_data *data = fw_info->data;
	struct device *dev = &data->client->dev;
	int error;

	error = mxt_initialize(data);
	if (error) {
		dev_err(dev, "Failed to initialize\n");
		goto err_free_mem;
	}

	error = mxt_rest_initialize(fw_info);
	if (error) {
		dev_err(dev, "Failed to rest initialize\n");
		goto err_free_mem;
	}

	error = mxt_touch_finish_init(data);
	if (error)
		goto err_free_mem;

	return 0;

err_free_mem:
	kfree(data->objects);
	data->objects = NULL;
	kfree(data->rid_map);
	data->rid_map = NULL;
	return error;
}


static void mxt_request_firmware_work(const struct firmware *fw,
		void *context)
{
	struct mxt_data *data = context;
	struct mxt_fw_info fw_info;
	int error;

	memset(&fw_info, 0, sizeof(struct mxt_fw_info));
	fw_info.data = data;

	error = mxt_verify_fw(&fw_info, fw);
	if (error)
		goto ts_rest_init;

	/* Skip update on boot up if firmware file does not have a header */
	if (!fw_info.hdr_len)
		goto ts_rest_init;

	error = mxt_flash_fw_on_probe(&fw_info);
	if (error)
		goto out;

ts_rest_init:
	error = mxt_touch_rest_init(&fw_info);
out:
	if (error)
		/* complete anyway, so open() doesn't get blocked */
		complete_all(&data->init_done);

	release_firmware(fw);
}

static int __devinit mxt_touch_init(struct mxt_data *data, bool nowait)
{
	struct i2c_client *client = data->client;
	const char *firmware_name = data->pdata->firmware_name ?: MXT_FW_NAME;
	int ret = 0;

	data->pdata->power_on();

	if (nowait) {
		const struct firmware *fw;
		ret = request_firmware(&fw, firmware_name, &client->dev);
		if (ret) {
			dev_err(&client->dev,
				"error requesting built-in firmware\n");
			goto out;
		}
		mxt_request_firmware_work(fw, data);
	} else {
		ret = request_firmware_nowait(THIS_MODULE, true, firmware_name,
				      &client->dev, GFP_KERNEL,
				      data, mxt_request_firmware_work);
		if (ret)
			dev_err(&client->dev,
				"cannot schedule firmware update (%d)\n", ret);
	}

out:
	return ret;
}

static int mxt_input_open(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);
	int ret;

	ret = wait_for_completion_interruptible_timeout(&data->init_done,
			msecs_to_jiffies(90 * MSEC_PER_SEC));

	if (ret < 0) {
		dev_err(&dev->dev,
			"error while waiting for device to init (%d)\n", ret);
		ret = -ENXIO;
		goto err_open;
	}
	if (ret == 0) {
		dev_err(&dev->dev,
			"timedout while waiting for device to init\n");
		ret = -ENXIO;
		goto err_open;
	}
	data->pdata->power_reset();

	ret = mxt_start(data);
	if (ret)
		goto err_open;

	return 0;

err_open:
	return ret;
}

static void mxt_input_close(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);

	mxt_stop(data);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
#define mxt_suspend	NULL
#define mxt_resume	NULL

static void mxt_early_suspend(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data,
								early_suspend);
#if TSP_INFORM_CHARGER
	cancel_delayed_work_sync(&data->noti_dwork);
#endif

	mutex_lock(&data->lock);

	if (data->mxt_enabled) {
		disable_irq(data->client->irq);
		data->mxt_enabled = false;
		mxt_internal_suspend(data);
	} else {
		dev_err(&data->client->dev,
			"%s. but touch already off\n", __func__);
	}

	mutex_unlock(&data->lock);
}

static void mxt_late_resume(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data,
								early_suspend);
	mutex_lock(&data->lock);

	if (data->mxt_enabled) {
		dev_err(&data->client->dev,
			"%s. but touch already on\n", __func__);
	} else {
		mxt_internal_resume(data);
		data->mxt_enabled = true;
		enable_irq(data->client->irq);
	}

	mutex_unlock(&data->lock);
}
#else
static int mxt_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);

	disable_irq(data->client->irq);
	mxt_internal_suspend(data);
	data->mxt_enabled = false;

	mutex_unlock(&data->lock);
	return 0;
}

static int mxt_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);

	mxt_internal_resume(data);
	data->mxt_enabled = true;
	enable_irq(data->client->irq);

	mutex_unlock(&data->lock);
	return 0;
}
#endif

/* Added for samsung dependency codes such as Factory test,
 * Touch booster, Related debug sysfs.
 */
#include "mxt540s_sec.c"

static int __devinit mxt_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	const struct mxt_platform_data *pdata = client->dev.platform_data;
	struct mxt_data *data;
	struct input_dev *input_dev;
	u16 boot_address;
	int i, ret = 0;

	if (!pdata) {
		dev_err(&client->dev, "missing platform data\n");
		return -ENODEV;
	}

	if (pdata->max_finger_touches <= 0)
		return -EINVAL;

	data = kzalloc(sizeof(*data) + pdata->max_finger_touches *
					sizeof(*data->fingers), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!data || !input_dev) {
		ret = -ENOMEM;
		dev_err(&client->dev, "input device allocation failed\n");
		goto err_alloc_dev;
	}

	data->client = client;
	data->input_dev = input_dev;
	data->pdata = pdata;
	data->num_fingers = pdata->max_finger_touches;
	data->mxt_enabled = true;
	mutex_init(&data->lock);
	init_completion(&data->init_done);

	input_dev->name = "sec_touchscreen";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->open = mxt_input_open;
	input_dev->close = mxt_input_close;

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(MT_TOOL_FINGER, input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_mt_init_slots(input_dev, data->num_fingers);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, pdata->min_x,
			     pdata->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, pdata->min_y,
			     pdata->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, pdata->min_w,
			     pdata->max_w, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, pdata->min_z,
			     pdata->max_z, 0, 0);

#if TSP_USE_SHAPETOUCH
	input_set_abs_params(input_dev, ABS_MT_COMPONENT, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_SUMSIZE, 0, 16 * 26, 0, 0);
#endif
	for (i = 0; i < data->num_fingers; i++)
		data->fingers[i].state = MXT_STATE_INACTIVE;

	input_set_drvdata(input_dev, data);
	i2c_set_clientdata(client, data);

	/* regist dummy device for boot_address */
	if (data->pdata->boot_address) {
		boot_address = data->pdata->boot_address;
	} else {
		if (client->addr == MXT_APP_LOW)
			boot_address = MXT_BOOT_LOW;
		else
			boot_address = MXT_BOOT_HIGH;
	}
	data->client_boot = i2c_new_dummy(client->adapter, boot_address);
	if (!data->client_boot) {
		dev_err(&client->dev, "Fail to register sub client[0x%x]\n",
			 boot_address);
		goto err_alloc_dev;
	}

	/* regist input device */
	ret = input_register_device(input_dev);
	if (ret)
		goto err_reg_dev;

#if TSP_INFORM_CHARGER
	/* Register callbacks */
	/* To inform tsp , charger connection status*/
	data->callbacks.inform_charger = inform_charger;
	if (pdata->register_cb) {
		pdata->register_cb(&data->callbacks);
		inform_charger_init(data);
	}
#endif

	ret = mxt_sysfs_init(client);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to creat sysfs\n");
		goto err_init_drv;
	}

	ret = mxt_touch_init(data, false);
	if (ret) {
		dev_err(&client->dev, "Failed to init driver\n");
		goto err_init_drv;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = mxt_early_suspend;
	data->early_suspend.resume = mxt_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

	return 0;

err_init_drv:
	input_unregister_device(input_dev);
	input_dev = NULL;
	gpio_free(data->pdata->gpio_read_done);
	data->pdata->power_off();
err_reg_dev:
	input_free_device(input_dev);
	i2c_unregister_device(data->client_boot);
err_alloc_dev:
	kfree(data);

	return ret;
}

static int __devexit mxt_remove(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif
	free_irq(client->irq, data);
	kfree(data->objects);
	kfree(data->rid_map);
	gpio_free(data->pdata->gpio_read_done);
	data->pdata->power_off();
	input_unregister_device(data->input_dev);
	i2c_unregister_device(data->client_boot);
	kfree(data);

	return 0;
}

static struct i2c_device_id mxt_idtable[] = {
	{MXT_DEV_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, mxt_idtable);

static const struct dev_pm_ops mxt_pm_ops = {
	.suspend = mxt_suspend,
	.resume = mxt_resume,
};

static struct i2c_driver mxt_i2c_driver = {
	.id_table = mxt_idtable,
	.probe = mxt_probe,
	.remove = __devexit_p(mxt_remove),
	.driver = {
		.owner	= THIS_MODULE,
		.name	= MXT_DEV_NAME,
#ifdef CONFIG_PM
		.pm	= &mxt_pm_ops,
#endif
	},
};

static int __init mxt_init(void)
{
	return i2c_add_driver(&mxt_i2c_driver);
}

static void __exit mxt_exit(void)
{
	i2c_del_driver(&mxt_i2c_driver);
}
module_init(mxt_init);
module_exit(mxt_exit);

MODULE_DESCRIPTION("Atmel MaXTouch driver");
MODULE_AUTHOR("bumwoo.lee<bw365.lee@samsung.com>");
MODULE_LICENSE("GPL");
