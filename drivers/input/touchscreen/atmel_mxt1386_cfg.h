/*
 *  drivers/input/touchscreen/atmel_mxt1386_cfg.h
 *
 *  Copyright (c) 2010 Samsung Electronics Co., LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __ATMEL_MXT1386_CFG_H
#define __ATMEL_MXT1386_CFG_H


extern u8 firmware_latest[];

extern int mxt_i2c_master_recv(struct i2c_client *client, u8 *buf, u32 count);
extern int mxt_i2c_master_send(struct i2c_client *client, u8 *buf, u32 count);
extern int mxt_write_block(struct i2c_client *client,
				u16 addr, u16 length, u8 *value);
extern u16 get_object_address(uint8_t object_type,
					uint8_t instance,
					struct mxt_object *object_table,
					int max_objs);
extern u16 get_object_size(uint8_t object_type,
					struct mxt_object *object_table,
					int max_objs);
extern int backup_to_nv(struct mxt_data *mxt);
extern int reset_chip(struct mxt_data *mxt, u8 mode);
extern int mxt_config_settings(struct mxt_data *mxt);
extern int mxt_get_object_values(struct mxt_data *mxt, u8 *buf, int obj_type);
extern int mxt_check_firmware(struct device *dev, int *ver);
extern int mxt_load_firmware(struct device *dev, const char *fn);
extern int mxt_power_config(struct mxt_data *mxt);
extern int mxt_multitouch_config(struct mxt_data *mxt);

#endif  /* __ATMEL_MXT1386_CFG_H */
