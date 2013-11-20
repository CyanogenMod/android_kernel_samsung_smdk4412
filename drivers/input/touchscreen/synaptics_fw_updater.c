/* drivers/input/touchscreen/synaptics_fw_updater.c
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/synaptics_s7301.h>
#include "synaptics_fw.h"

static void synaptics_setup(struct synaptics_drv_data *data)
{
	struct synaptics_ts_fw_block *fw = data->fw;

	fw->f34_database = data->f34.data_base_addr;
	fw->f34_querybase = data->f34.query_base_addr;
	fw->f01_database = data->f01.data_base_addr;
	fw->f01_commandbase = data->f01.command_base_addr;
	fw->f01_controlbase = data->f01.control_base_addr;

	fw->f34_reflash_blocknum
		= fw->f34_database;
	fw->f34_reflash_blockdata
		= fw->f34_database + 2;
	fw->f34_reflashquery_boot_id
		= fw->f34_querybase;
	fw->f34_reflashquery_flashpropertyquery
		= fw->f34_querybase + 2;
	fw->f34_reflashquery_fw_blocksize
		= fw->f34_querybase + 3;
	fw->f34_reflashquery_fw_blockcount
		= fw->f34_querybase + 5;
	fw->f34_reflashquery_config_blocksize
		= fw->f34_querybase + 3;
	fw->f34_reflashquery_config_blockcount
		= fw->f34_querybase + 7;

	fw->fw_imgdata = (u8 *)((&fw->fw_data[0]) + 0x100);
	fw->config_imgdata = (u8 *)(fw->fw_imgdata + fw->imagesize);
	fw->fw_version = (u32)(fw->fw_data[7]);

	switch (fw->fw_version) {
	case 2:
		fw->lock_imgdata = (u8 *)((&fw->fw_data[0]) + 0xD0);
		break;
	case 3:
	case 4:
		fw->lock_imgdata = (u8 *)((&fw->fw_data[0]) + 0xC0);
		break;
	case 5:
		fw->lock_imgdata = (u8 *)((&fw->fw_data[0]) + 0xB0);
		break;
	default:
		break;
	}
}

/* synaptics_fw_initialize sets up the reflahs process
 */
static void synaptics_fw_initialize(struct synaptics_drv_data *data)
{
	struct synaptics_ts_fw_block *fw = data->fw;
	u8 buf[2];

	buf[0] = 0x00;
	synaptics_ts_write_data(data, 0xff, buf[0]);

	synaptics_setup(data);

	buf[0] = 0x0f;
	synaptics_ts_write_data(data,
		fw->f01_controlbase + 1, buf[0]);

	synaptics_ts_read_block(data,
		fw->f34_reflashquery_fw_blocksize, buf, 2);

	fw->fw_blocksize = buf[0] | (buf[1] << 8);
	printk(KERN_DEBUG "[TSP] %s - fw_blocksize : %u\n",
		__func__, fw->fw_blocksize);
}

/* synaptics_read_fw_info reads the F34 query registers and retrieves the block
 * size and count of the firmware section of the image to be reflashed
 */
static void synaptics_read_fw_info(struct synaptics_drv_data *data)
{
	struct synaptics_ts_fw_block *fw = data->fw;
	u8 buf[2];

	synaptics_ts_read_block(data,
		fw->f34_reflashquery_fw_blockcount,
		buf, 2);
	fw->fw_blockcount = buf[0] | (buf[1] << 8);
	fw->imagesize = (u32)(fw->fw_blockcount * fw->fw_blocksize);
	printk(KERN_DEBUG "[TSP] %s - fw_blockcount : %u\n",
		__func__, fw->fw_blockcount);
	printk(KERN_DEBUG "[TSP] %s - imagesize : %u\n",
		__func__, fw->imagesize);
}

/* synaptics_read_config_info reads the F34 query registers
 * and retrieves the block size and count of the configuration section
 * of the image to be reflashed
 */
static void synaptics_read_config_info(struct synaptics_drv_data *data)
{
	struct synaptics_ts_fw_block *fw = data->fw;
	u8 buf[2];

	synaptics_ts_read_block(data,
		fw->f34_reflashquery_config_blocksize,
		buf, 2);
	fw->config_blocksize = (u16)(buf[0] | (buf[1] << 8));

	printk(KERN_DEBUG "[TSP] config_blocksize : %u\n",
		fw->config_blocksize);

	synaptics_ts_read_block(data,
		fw->f34_reflashquery_config_blockcount,
		buf, 2);
	fw->config_blockcount = (u16)(buf[0] | (buf[1] << 8));
	fw->config_imagesize =
		(u32)(fw->config_blockcount * fw->config_blocksize);
	printk(KERN_DEBUG "[TSP] config_blockcount : %u\n",
		fw->config_blockcount);
	printk(KERN_DEBUG "[TSP] config_imagesize : %u\n",
		fw->config_imagesize);
}

/* synaptics_read_bootload_id reads the F34 query registers
 * and retrieves the bootloader ID of the firmware
 */
static void synaptics_read_bootload_id(struct synaptics_drv_data *data)
{
	struct synaptics_ts_fw_block *fw = data->fw;
	u8 buf[2];
	synaptics_ts_read_block(data,
		fw->f34_reflashquery_boot_id, buf, 2);
	fw->boot_id = (u16)(buf[0] | buf[1] << 8);
	printk(KERN_DEBUG "[TSP] read BootloadID : 0x%x\n", fw->boot_id);
}

/* synaptics_write_bootload_id writes the bootloader ID
 * to the F34 data register to unlock the reflash process
 */
static void synaptics_write_bootload_id(struct synaptics_drv_data *data)
{
	struct synaptics_ts_fw_block *fw = data->fw;
	u8 buf[2];

	buf[0] = fw->boot_id & 0xff;
	buf[1] = (fw->boot_id >> 8) & 0xff;

	synaptics_ts_write_block(data,
		fw->f34_reflash_blockdata, buf, 2);
}

/* synaptics_enable_flashing kicks off the reflash process
 */
static void synaptics_enable_flashing(struct synaptics_drv_data *data)
{
	struct synaptics_ts_fw_block *fw = data->fw;
	u8 buf;
	u8 status;
	int cnt = 0;

	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	/* Reflash is enabled by first reading the bootloader ID from
	   the firmware and write it back */
	synaptics_read_bootload_id(data);
	synaptics_write_bootload_id(data);

	/* Make sure Reflash is not already enabled */
	synaptics_ts_read_data(data,
		fw->f34_flashcontrol, &buf);
	while (((buf & 0x0f) != 0x00) && (cnt++ < 300)) {
		usleep_range(500, 1000);
		synaptics_ts_read_data(data,
			fw->f34_flashcontrol, &buf);
	}

	synaptics_ts_read_data(data,
		fw->f01_database, &status);

	if ((status & 0x40) == 0) {
		/* Write the "Enable Flash Programming command to
		F34 Control register Wait for ATTN and then clear the ATTN. */
		buf = 0x0f;
		synaptics_ts_write_data(data,
			fw->f34_flashcontrol, buf);
		mdelay(300);
		synaptics_ts_read_data(data,
			(fw->f01_database + 1), &status);

		/* Scan the PDT again to ensure all register offsets are
		correct */
		synaptics_setup(data);

		/* Read the "Program Enabled" bit of the F34 Control register,
		and proceed only if the bit is set.*/
		synaptics_ts_read_data(data,
		fw->f34_flashcontrol, &buf);

		cnt = 0;
		while (((buf & 0x0f) != 0x00) && (cnt++ < 300)) {
			/* In practice, if buf!=0x80 happens for multiple
			counts, it indicates reflash is failed to be enabled,
			and program should quit */
			usleep_range(500, 1000);
			synaptics_ts_read_data(data,
				fw->f34_flashcontrol, &buf);
		}
	}
}

/* synaptics_wait_attn waits for ATTN to be asserted
 * within a certain time threshold.
 * The function also checks for the F34 "Program Enabled" bit and clear ATTN
 * accordingly.
 */
static void synaptics_wait_attn(struct synaptics_drv_data *data)
{
	struct synaptics_ts_fw_block *fw = data->fw;
	u8 buf;
	u8 status;
	int cnt = 0;

	while (gpio_get_value(data->gpio) && cnt++ < 300)
		usleep_range(500, 1000);

	synaptics_ts_read_data(data,
		fw->f34_flashcontrol, &buf);
	while ((buf != 0x80) && (cnt++ < 300)) {
		usleep_range(500, 1000);
		synaptics_ts_read_data(data,
			fw->f34_flashcontrol, &buf);
	}
	synaptics_ts_read_data(data,
		(fw->f01_database + 1), &status);
}

/* synaptics_program_config writes the configuration section of the image block
 * by block
 */
static void synaptics_program_config(struct synaptics_drv_data *data)
{
	struct synaptics_ts_fw_block *fw = data->fw;
	u8 buf[2];
	u8 *pbuf;
	unsigned short block;

	printk(KERN_DEBUG "[TSP] Program Configuration Section...\n");

	pbuf = (u8 *) &fw->fw_data[0xb100];

	for (block = 0; block < fw->config_blockcount; block++) {
		buf[0] = block & 0xff;
		buf[1] = (block & 0xff00) >> 8;

		/* Block by blcok, write the block number and data to
		the corresponding F34 data registers */
		synaptics_ts_write_block(data,
			fw->f34_reflash_blocknum, buf, 2);
		synaptics_ts_write_block(data,
			fw->f34_reflash_blockdata,
			pbuf, fw->config_blocksize);
		pbuf += fw->config_blocksize;

		/* Issue the "Write Configuration Block" command */
		buf[0] = 0x06;
		synaptics_ts_write_data(data,
			fw->f34_flashcontrol, buf[0]);
		synaptics_wait_attn(data);
		printk(KERN_DEBUG ".");
	}
}

/* synaptics_finalize_reflash finalizes the reflash process
*/
static void synaptics_finalize_reflash(struct synaptics_drv_data *data)
{
	struct synaptics_ts_fw_block *fw = data->fw;
	u8 buf;
	u8 status;
	int cnt = 0;

	printk(KERN_DEBUG "[TSP] Finalizing Reflash..\n");

	/* Issue the "Reset" command to F01 command register to reset the chip
	 This command will also test the new firmware image and check if its is
	 valid */
	buf = 1;
	synaptics_ts_write_data(data,
		fw->f01_commandbase, buf);

	mdelay(160);
	synaptics_ts_read_data(data,
		fw->f01_database, &buf);

	/* Sanity check that the reflash process is still enabled */
	synaptics_ts_read_data(data,
		fw->f34_flashcontrol, &status);
	while (((status & 0x0f) != 0x00) && (cnt++ < 300)) {
		usleep_range(500, 1000);
		synaptics_ts_read_data(data,
			fw->f34_flashcontrol, &status);
	}
	synaptics_ts_read_data(data,
		(fw->f01_database + 1), &status);

	synaptics_setup(data);

	buf = 0;
	cnt = 0;

	/* Check if the "Program Enabled" bit in F01 data register is cleared
	 Reflash is completed, and the image passes testing when the bit is
	 cleared */
	synaptics_ts_read_data(data, fw->f01_database, &buf);
	while (((buf & 0x40) != 0) && (cnt++ < 300)) {
		usleep_range(500, 1000);
		synaptics_ts_read_data(data, fw->f01_database, &buf);
	}

	/* Rescan PDT the update any changed register offsets */
	synaptics_setup(data);

	printk(KERN_DEBUG "[TSP] Reflash Completed. Please reboot.\n");
}

/* synaptics_fw_write writes the firmware section of the image block by
 * block
 */
static void synaptics_fw_write(struct synaptics_drv_data *data)
{
	struct synaptics_ts_fw_block *fw = data->fw;
	u8 *fw_data;
	u8 buf[2];
	unsigned short block;

	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	fw_data = (u8 *) &fw->fw_data[0x100];

	for (block = 0; block < fw->fw_blockcount; ++block) {
		static unsigned long swtich_slot_time;
		if (printk_timed_ratelimit(&swtich_slot_time, 5000))
			printk(KERN_DEBUG "[TSP] block : %u\n", block);
		/* Block by blcok, write the block number and data to
		the corresponding F34 data registers */
		buf[0] = block & 0xff;
		buf[1] = (block & 0xff00) >> 8;
		synaptics_ts_write_block(data,
			fw->f34_reflash_blocknum, buf, 2);

		synaptics_ts_write_block(data,
			fw->f34_reflash_blockdata, fw_data,
			fw->fw_blocksize);
		fw_data += fw->fw_blocksize;

		/* Issue the "Write Firmware Block" command */
		buf[0] = 2;
		synaptics_ts_write_data(data,
			fw->f34_flashcontrol, buf[0]);

		synaptics_wait_attn(data);
	}
}

/* synaptics_program_fw prepares the firmware writing process
*/
static void synaptics_program_fw(struct synaptics_drv_data *data)
{
	struct synaptics_ts_fw_block *fw = data->fw;

	printk(KERN_DEBUG "[TSP] %s\n", __func__);

#if 0
	synaptics_read_bootload_id(data);
#endif
	synaptics_write_bootload_id(data);

	synaptics_ts_write_data(data, fw->f34_flashcontrol, 0x3);

	synaptics_wait_attn(data);
	synaptics_fw_write(data);
}

int synaptics_fw_updater(struct synaptics_drv_data *data, u8 *fw_data)
{
	struct synaptics_ts_fw_block *fw;
	int irq = gpio_to_irq(data->gpio);
	bool update = true;

	fw = kzalloc(sizeof(struct synaptics_ts_fw_block), GFP_KERNEL);
	data->fw = fw;

	if (NULL == fw_data) {
		u8 *buf, *fw_version;
		buf = kzalloc(4, GFP_KERNEL);
		fw_version = kzalloc(4, GFP_KERNEL);
		fw->fw_data = (u8 *)rmi_fw;
		strncpy(fw_version, &rmi_fw[0xb100],
			sizeof(fw_version));
		strncpy(data->firm_version, fw_version,
			sizeof(data->firm_version));
		strncpy(data->firm_config, rmi_config_ver,
			sizeof(data->firm_config));
		synaptics_ts_read_block(data,
			data->f34.control_base_addr,
			buf, 4);

		printk(KERN_DEBUG "[TSP] IC FW. : [%s], new FW. : [%s]\n",
			buf, fw_version);

		if (strncmp((char *)fw_version, (char *)buf, 4) == 0)
			update = false;

		kfree(buf);
		kfree(fw_version);

	} else
		fw->fw_data = fw_data;

	if (update) {
		disable_irq(irq);
		wake_lock(&data->wakelock);
		synaptics_fw_initialize(data);
		synaptics_read_config_info(data);
		synaptics_read_fw_info(data);
		fw->f34_flashcontrol = fw->f34_database
			+ fw->fw_blocksize + 2;
		printk(KERN_DEBUG
			"[TSP] F34_FlashControl : %u\n",
			fw->f34_flashcontrol);

		synaptics_enable_flashing(data);
		synaptics_program_fw(data);
		synaptics_program_config(data);
		synaptics_finalize_reflash(data);
		if (data->pdata->set_power(false))
			data->pdata->hw_reset();
		else {
			msleep(100);
			data->pdata->set_power(true);
			msleep(100);
		}

		wake_unlock(&data->wakelock);
		enable_irq(irq);
	}
	return 0;
}

void forced_fw_update(struct synaptics_drv_data *data)
{
	synaptics_fw_updater(data, (u8 *)rmi_fw);
}

