/* Synaptics Register Mapped Interface (RMI4) I2C Physical Layer Driver.
 * Copyright (c) 2007-2012, Synaptics Incorporated
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
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/syscalls.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/firmware.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/i2c/synaptics_rmi.h>
#include "synaptics_i2c_rmi.h"

#define FW_IMAGE_NAME "synaptics_fw.img"

#define CHECKSUM_OFFSET 0x00
#define BOOTLOADER_VERSION_OFFSET 0x07
#define IMAGE_SIZE_OFFSET 0x08
#define CONFIG_SIZE_OFFSET 0x0C
#define PRODUCT_ID_OFFSET 0x10
#define PRODUCT_INFO_OFFSET 0x1E
#define FW_IMAGE_OFFSET 0x100
#define PRODUCT_ID_SIZE 10

#define QUERY_BOOTLOADER_ID_OFFSET 0
#define QUERY_BLOCK_SIZE_OFFSET 2
#define QUERY_BLOCK_COUNT_OFFSET 3
#define DATA_BLOCK_NUMBER_OFFSET 0
#define DATA_BLOCK_DATA_OFFSET 1
#define DATA_FLASH_COMMAND_OFFSET 2
#define DATA_FLASH_STATUS_OFFSET 3

#define F34_CMD_WRITE_FW_BLOCK 0x2
#define F34_CMD_ERASE_ALL 0x3
#define F34_CMD_READ_CONFIG_BLOCK 0x5
#define F34_CMD_WRITE_CONFIG_BLOCK 0x6
#define F34_CMD_ERASE_CONFIG 0x7
#define F34_CMD_ENABLE_FLASH_PROG 0xf

#define SLEEP_MODE_NORMAL (0x00)
#define SLEEP_MODE_SENSOR_SLEEP (0x01)
#define SLEEP_MODE_RESERVED0 (0x02)
#define SLEEP_MODE_RESERVED1 (0x03)

#define ENABLE_WAIT_MS (1 * 1000)
#define WRITE_WAIT_MS (3 * 1000)
#define ERASE_WAIT_MS (5 * 1000)

#define MIN_SLEEP_TIME_US 50
#define MAX_SLEEP_TIME_US 100

struct synaptics_rmi4_fwu_fn_ptr {
	int (*read) (struct synaptics_rmi4_data *rmi4_data,
		     unsigned short addr, unsigned char *data,
		     unsigned short length);
	int (*write) (struct synaptics_rmi4_data *rmi4_data,
		      unsigned short addr, unsigned char *data,
		      unsigned short length);
	int (*enable) (struct synaptics_rmi4_data *rmi4_data, bool enable);
};

struct image_header {
	unsigned int checksum;
	unsigned int image_size;
	unsigned int config_size;
	unsigned char options;
	unsigned char bootloader_version;
	unsigned char product_id[SYNAPTICS_RMI4_PRODUCT_ID_LENGTH + 1];
	unsigned char product_info[SYNAPTICS_RMI4_PRODUCT_INFO_SIZE];
};

union pdt_properties {
	struct {
		unsigned char reserved_1:6;
		unsigned char has_bsr:1;
		unsigned char reserved_2:1;
	} __attribute__ ((__packed__));
	unsigned char data[1];
};

union f01_device_status {
	struct {
		unsigned char status_code:4;
		unsigned char reserved:2;
		unsigned char flash_prog:1;
		unsigned char unconfigured:1;
	} __attribute__ ((__packed__));
	unsigned char data[1];
};

union f01_device_control {
	struct {
		unsigned char sleep_mode:2;
		unsigned char nosleep:1;
		unsigned char reserved:2;
		unsigned char charger_connected:1;
		unsigned char report_rate:1;
		unsigned char configured:1;
	} __attribute__ ((__packed__));
	unsigned char data[1];
};

union f34_flash_status {
	struct {
		unsigned char status:6;
		unsigned char reserved:1;
		unsigned char program_enabled:1;
	} __attribute__ ((__packed__));
	unsigned char data[1];
};

struct synaptics_rmi4_fwu_handle {
	unsigned char *ext_fw_source;
	unsigned char intr_mask;
	unsigned char command;
	unsigned char bootloader_id[2];
	unsigned char productinfo1;
	unsigned char productinfo2;
	unsigned short block_size;
	unsigned short fw_block_count;
	unsigned short config_block_count;
	char product_id[SYNAPTICS_RMI4_PRODUCT_ID_LENGTH + 1];
	const unsigned char *firmware_data;
	const unsigned char *config_data;
	union f34_flash_status flash_status;
	struct synaptics_rmi4_fn_desc f01_fd;
	struct synaptics_rmi4_fn_desc f34_fd;
	struct synaptics_rmi4_fwu_fn_ptr *fn_ptr;
	struct synaptics_rmi4_data *rmi4_data;
};

struct synaptics_rmi4_fwu_handle *fwu;

static inline void batohs(unsigned short *dest, unsigned char *src)
{
	*dest = src[1] * 0x100 + src[0];
}

static unsigned int extract_uint(const unsigned char *ptr)
{
	return (unsigned int)ptr[0] +
	    (unsigned int)ptr[1] * 0x100 +
	    (unsigned int)ptr[2] * 0x10000 + (unsigned int)ptr[3] * 0x1000000;
}

static int fwu_read_f01_device_status(union f01_device_status
				      *f01_device_status)
{
	int retval;

	retval = fwu->fn_ptr->read(fwu->rmi4_data, fwu->f01_fd.data_base_addr,
				   f01_device_status->data,
				   sizeof(f01_device_status->data));
	if (retval < 0) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Failed to read F01 " "device status\n", __func__);
		return retval;
	}

	return 0;
}

static int fwu_read_f34_queries(void)
{
	int retval;
	unsigned char buf[4];

	retval = fwu->fn_ptr->read(fwu->rmi4_data,
				   fwu->f34_fd.query_base_addr +
				   QUERY_BOOTLOADER_ID_OFFSET,
				   fwu->bootloader_id, 2);
	if (retval < 0) {
		dev_err(&fwu->rmi4_data->i2c_client->dev, "%s Failed to read "
			"bootloader ID\n", __func__);
		return retval;
	}

	retval = fwu->fn_ptr->read(fwu->rmi4_data,
				   fwu->f34_fd.query_base_addr +
				   QUERY_BLOCK_SIZE_OFFSET, buf, 2);
	if (retval < 0) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Failed to read block " "size info\n", __func__);
		return retval;
	}

	batohs(&fwu->block_size, &(buf[0]));

	retval = fwu->fn_ptr->read(fwu->rmi4_data,
				   fwu->f34_fd.query_base_addr +
				   QUERY_BLOCK_COUNT_OFFSET, buf, 4);
	if (retval < 0) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Failed to read block " "count info\n", __func__);
		return retval;
	}

	batohs(&fwu->fw_block_count, &(buf[0]));
	batohs(&fwu->config_block_count, &(buf[2]));

	return 0;
}

static int fwu_read_f34_flash_status(void)
{
	int retval;
	unsigned char status;
	unsigned char command;

	retval = fwu->fn_ptr->read(fwu->rmi4_data,
				   fwu->f34_fd.data_base_addr +
				   DATA_FLASH_STATUS_OFFSET, &status,
				   sizeof(status));
	if (retval < 0) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Failed to read flash " "status\n", __func__);
		return retval;
	}

	/* Program enabled bit not available - force bit to be set */
	fwu->flash_status.program_enabled = 1;
	fwu->flash_status.status = status & MASK_3BIT;

	retval = fwu->fn_ptr->read(fwu->rmi4_data,
				   fwu->f34_fd.data_base_addr +
				   DATA_FLASH_COMMAND_OFFSET, &command,
				   sizeof(command));
	if (retval < 0) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Failed to read flash " "command\n", __func__);
		return retval;
	}

	fwu->command = command & MASK_4BIT;

	return 0;
}

static int fwu_write_f34_command(unsigned char cmd)
{
	int retval;
	unsigned char command = cmd;

	retval = fwu->fn_ptr->write(fwu->rmi4_data,
				    fwu->f34_fd.data_base_addr +
				    DATA_FLASH_COMMAND_OFFSET, &command,
				    sizeof(command));
	if (retval < 0) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Failed to write command " "0x%02x\n", __func__,
			command);
		return retval;
	}

	fwu->command = cmd;

	return 0;
}

static int fwu_wait_for_idle(int timeout_ms)
{
	int retval;
	int count = 0;
	int timeout_count = ((timeout_ms * 1000) / MAX_SLEEP_TIME_US) + 1;

	do {
		usleep_range(MIN_SLEEP_TIME_US, MAX_SLEEP_TIME_US);

		count++;
		if (count == timeout_count)
			fwu_read_f34_flash_status();

		if ((fwu->command == 0x00)
		    && (fwu->flash_status.status == 0x00))
			return 0;
	} while (count < timeout_count);

	dev_err(&fwu->rmi4_data->i2c_client->dev,
		"%s Timed out waiting for idle " "status\n", __func__);

	return -ETIMEDOUT;
}

static int fwu_scan_pdt(void)
{
	int retval;
	unsigned char ii;
	unsigned char intr_count = 0;
	unsigned char intr_offset;
	unsigned short addr;
	bool f01found = false;
	bool f34found = false;
	struct synaptics_rmi4_fn_desc rmi_fd;

	for (addr = PDT_START; addr > PDT_END; addr -= PDT_ENTRY_SIZE) {
		retval = fwu->fn_ptr->read(fwu->rmi4_data, addr,
					   (unsigned char *)&rmi_fd,
					   sizeof(rmi_fd));
		if (retval < 0)
			return retval;

		if (rmi_fd.fn_number) {
			dev_dbg(&fwu->rmi4_data->i2c_client->dev,
				"%s Found F%02x\n", __func__, rmi_fd.fn_number);
			switch (rmi_fd.fn_number) {
			case SYNAPTICS_RMI4_DEVICE_CONTROL_FN:
				f01found = true;
				fwu->f01_fd.query_base_addr =
				    rmi_fd.query_base_addr;
				fwu->f01_fd.ctrl_base_addr =
				    rmi_fd.ctrl_base_addr;
				fwu->f01_fd.data_base_addr =
				    rmi_fd.data_base_addr;
				fwu->f01_fd.cmd_base_addr =
				    rmi_fd.cmd_base_addr;
				break;
			case SYNAPTICS_RMI4_FLASH_FN:
				f34found = true;
				fwu->f34_fd.query_base_addr =
				    rmi_fd.query_base_addr;
				fwu->f34_fd.ctrl_base_addr =
				    rmi_fd.ctrl_base_addr;
				fwu->f34_fd.data_base_addr =
				    rmi_fd.data_base_addr;

				fwu->intr_mask = 0;
				intr_offset = intr_count % 8;
				for (ii = intr_offset;
				     ii <
				     ((rmi_fd.intr_src_count & 0x07) +
				      intr_offset); ii++) {
					fwu->intr_mask |= 1 << ii;
				}
				break;
			}
		} else {
			break;
		}

		intr_count += (rmi_fd.intr_src_count & 0x07);
	}

	if (!f01found || !f34found) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Failed to find both F01 " "and F34 during PDT scan",
			__func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_write_blocks(unsigned char *block_ptr, unsigned short block_cnt,
			    unsigned char command)
{
	int retval;
	unsigned char zeros[] = { 0, 0 };
	unsigned short block_num;

	retval = fwu->fn_ptr->write(fwu->rmi4_data,
				    fwu->f34_fd.data_base_addr +
				    DATA_BLOCK_NUMBER_OFFSET, zeros,
				    ARRAY_SIZE(zeros));
	if (retval < 0) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Failed to write initial "
			"zeros to block number register\n", __func__);
		return retval;
	}

	for (block_num = 0; block_num < block_cnt; block_num++) {
		retval = fwu->fn_ptr->write(fwu->rmi4_data,
					    fwu->f34_fd.data_base_addr +
					    DATA_BLOCK_DATA_OFFSET, block_ptr,
					    fwu->block_size);
		if (retval < 0) {
			dev_err(&fwu->rmi4_data->i2c_client->dev,
				"%s Failed to write " "block data (block %d)\n",
				__func__, block_num);
			return retval;
		}

		retval = fwu_write_f34_command(command);
		if (retval < 0) {
			dev_err(&fwu->rmi4_data->i2c_client->dev,
				"%s Failed to write " "command for block %d\n",
				__func__, block_num);
			return retval;
		}

		retval = fwu_wait_for_idle(WRITE_WAIT_MS);
		if (retval < 0) {
			dev_err(&fwu->rmi4_data->i2c_client->dev,
				"%s Failed to wait for "
				"idle status after writing block %d\n",
				__func__, block_num);
			return retval;
		}

		block_ptr += fwu->block_size;
	}

	return 0;
}

static int fwu_write_firmware(void)
{
	return fwu_write_blocks((unsigned char *)fwu->firmware_data,
				fwu->fw_block_count, F34_CMD_WRITE_FW_BLOCK);
}

static int fwu_write_configuration(void)
{
	return fwu_write_blocks((unsigned char *)fwu->config_data,
				fwu->config_block_count,
				F34_CMD_WRITE_CONFIG_BLOCK);
}

static int fwu_write_bootloader_id(void)
{
	int retval;

	retval = fwu->fn_ptr->write(fwu->rmi4_data,
				    fwu->f34_fd.data_base_addr +
				    DATA_BLOCK_DATA_OFFSET, fwu->bootloader_id,
				    ARRAY_SIZE(fwu->bootloader_id));
	if (retval < 0) {
		dev_err(&fwu->rmi4_data->i2c_client->dev, "%s Failed to write "
			"bootloader ID\n", __func__);
		return retval;
	}

	return 0;
}

static int fwu_enter_flash_prog(void)
{
	int retval;
	union f01_device_status f01_device_status;
	union f01_device_control f01_device_control;

	retval = fwu_write_bootloader_id();
	if (retval < 0)
		return retval;

	retval = fwu_write_f34_command(F34_CMD_ENABLE_FLASH_PROG);
	if (retval < 0)
		return retval;

	retval = fwu_wait_for_idle(ENABLE_WAIT_MS);
	if (retval)
		return retval;

	if (!fwu->flash_status.program_enabled) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Reached idle status but "
			"program enabled bit not set\n", __func__);
		return -EINVAL;
	}

	retval = fwu_scan_pdt();
	if (retval < 0)
		return retval;

	retval = fwu_read_f01_device_status(&f01_device_status);
	if (retval < 0)
		return retval;

	if (!f01_device_status.flash_prog) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Device not in flash " "programming mode.\n",
			__func__);
		return -EINVAL;
	}

	retval = fwu_read_f34_queries();
	if (retval)
		return retval;

	retval = fwu->fn_ptr->read(fwu->rmi4_data, fwu->f01_fd.ctrl_base_addr,
				   f01_device_control.data,
				   sizeof(f01_device_control.data));
	if (retval < 0) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Failed to read F01 " "device control\n", __func__);
		return retval;
	}

	f01_device_control.nosleep = true;
	f01_device_control.sleep_mode = SLEEP_MODE_NORMAL;

	retval = fwu->fn_ptr->write(fwu->rmi4_data, fwu->f01_fd.ctrl_base_addr,
				    f01_device_control.data,
				    sizeof(f01_device_control.data));
	if (retval < 0) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Failed to write F01 " "device control\n", __func__);
		return retval;
	}

	return retval;
}

static void fwu_reset_device(void)
{
	int retval;
	unsigned char command = 0x01;

	retval = fwu->fn_ptr->write(fwu->rmi4_data, fwu->f01_fd.cmd_base_addr,
				    &command, sizeof(command));
	if (retval < 0) {
		dev_err(&fwu->rmi4_data->i2c_client->dev,
			"%s Failed to reset device " "after reflash\n",
			__func__);
	}

	msleep(100);

	return;
}

static int fwu_do_reflash(void)
{
	int retval;

	retval = fwu_enter_flash_prog();
	if (retval < 0)
		return retval;

	dev_dbg(&fwu->rmi4_data->i2c_client->dev,
		"%s Entered reflash programming " "mode\n", __func__);

	retval = fwu_write_bootloader_id();
	if (retval < 0)
		return retval;

	dev_dbg(&fwu->rmi4_data->i2c_client->dev, "%s Bootloader ID written\n",
		__func__);

	retval = fwu_write_f34_command(F34_CMD_ERASE_ALL);
	if (retval < 0)
		return retval;

	dev_dbg(&fwu->rmi4_data->i2c_client->dev,
		"%s Erase all command written\n", __func__);

	retval = fwu_wait_for_idle(ERASE_WAIT_MS);
	if (retval < 0)
		return retval;

	dev_dbg(&fwu->rmi4_data->i2c_client->dev, "%s Idle status detected\n",
		__func__);

	if (fwu->firmware_data) {
		retval = fwu_write_firmware();
		if (retval < 0)
			return retval;
	}

	dev_dbg(&fwu->rmi4_data->i2c_client->dev, "%s Firmware programmed\n",
		__func__);

	if (fwu->config_data) {
		retval = fwu_write_configuration();
		if (retval < 0)
			return retval;
	}

	dev_dbg(&fwu->rmi4_data->i2c_client->dev,
		"%s Configuration programmed\n", __func__);

	return retval;
}

static int fwu_start_reflash(void)
{
	int retval;
	struct image_header header;
	const unsigned char *fw_image;
	const struct firmware *fw_entry = NULL;

	pr_notice("%s Start of reflash process\n", __func__);

	if (fwu->ext_fw_source)
		fw_image = fwu->ext_fw_source;
	else {
		dev_dbg(&fwu->rmi4_data->i2c_client->dev,
			"%s Requesting firmware " "image %s\n", __func__,
			FW_IMAGE_NAME);

		retval = request_firmware(&fw_entry, FW_IMAGE_NAME,
					  &fwu->rmi4_data->i2c_client->dev);
		if (retval != 0) {
			dev_err(&fwu->rmi4_data->i2c_client->dev,
				"%s Firmware image %s " "not available\n",
				__func__, FW_IMAGE_NAME);
			return -EINVAL;
		}

		dev_dbg(&fwu->rmi4_data->i2c_client->dev,
			"%s Firmware size = %d\n", __func__, fw_entry->size);

		fw_image = fw_entry->data;
	}

	header.checksum = extract_uint(&fw_image[CHECKSUM_OFFSET]);
	header.bootloader_version = fw_image[BOOTLOADER_VERSION_OFFSET];
	header.image_size = extract_uint(&fw_image[IMAGE_SIZE_OFFSET]);
	header.config_size = extract_uint(&fw_image[CONFIG_SIZE_OFFSET]);
	memcpy(header.product_id, &fw_image[PRODUCT_ID_OFFSET],
	       SYNAPTICS_RMI4_PRODUCT_ID_LENGTH);
	header.product_id[SYNAPTICS_RMI4_PRODUCT_ID_LENGTH] = 0;
	memcpy(header.product_info, &fw_image[PRODUCT_INFO_OFFSET],
	       SYNAPTICS_RMI4_PRODUCT_INFO_SIZE);

	if (header.image_size)
		fwu->firmware_data = fw_image + FW_IMAGE_OFFSET;
	if (header.config_size)
		fwu->config_data =
		    fw_image + FW_IMAGE_OFFSET + header.image_size;

	retval = fwu_do_reflash();

	fwu_reset_device();

	if (fw_entry)
		release_firmware(fw_entry);

	fwu->ext_fw_source = NULL;

	pr_notice("%s End of reflash process\n", __func__);

	return retval;
}

int synaptics_fw_updater(unsigned char *fw_data)
{
	if (!fwu)
		return -ENODEV;

	fwu->ext_fw_source = fw_data;

	return fwu_start_reflash();
}
EXPORT_SYMBOL(synaptics_fw_updater);

static void synaptics_rmi4_fwu_attn(struct synaptics_rmi4_data *rmi4_data,
				    unsigned char intr_mask)
{
	if (fwu->intr_mask & intr_mask)
		fwu_read_f34_flash_status();

	return;
}

static int synaptics_rmi4_fwu_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	union pdt_properties pdt_props;

	fwu = kzalloc(sizeof(*fwu), GFP_KERNEL);
	if (!fwu) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Failed to allocate memory "
			"for synaptics_rmi4_fwu_handle\n", __func__);
		goto exit;
	}

	fwu->fn_ptr = kzalloc(sizeof(*(fwu->fn_ptr)), GFP_KERNEL);

	fwu->rmi4_data = rmi4_data;
	fwu->fn_ptr->read = rmi4_data->i2c_read;
	fwu->fn_ptr->write = rmi4_data->i2c_write;
	fwu->fn_ptr->enable = rmi4_data->irq_enable;

	retval = fwu->fn_ptr->read(rmi4_data, PDT_PROPS, pdt_props.data,
				   sizeof(pdt_props.data));
	if (retval < 0) {
		dev_dbg(&rmi4_data->i2c_client->dev,
			"%s Failed to read PDT properties "
			"entry, assuming 0x00\n", __func__);
	} else if (pdt_props.has_bsr) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Firmware update for LTS not "
			"currently supported\n", __func__);
		goto exit_free_mem;
	}

	retval = fwu_scan_pdt();
	if (retval < 0)
		goto exit_free_mem;

	fwu->productinfo1 = rmi4_data->rmi4_mod_info.product_info[0];
	fwu->productinfo2 = rmi4_data->rmi4_mod_info.product_info[1];
	memcpy(fwu->product_id, rmi4_data->rmi4_mod_info.product_id_string,
	       SYNAPTICS_RMI4_PRODUCT_ID_LENGTH);
	fwu->product_id[SYNAPTICS_RMI4_PRODUCT_ID_LENGTH] = 0;

	dev_dbg(&rmi4_data->i2c_client->dev,
		"%s F01 product info: 0x%04x 0x%04x\n", __func__,
		fwu->productinfo1, fwu->productinfo2);
	dev_dbg(&rmi4_data->i2c_client->dev, "%s F01 product ID: %s\n",
		__func__, fwu->product_id);

	retval = fwu_read_f34_queries();
	if (retval)
		goto exit_free_mem;

	return 0;

 exit_free_mem:
	kfree(fwu->fn_ptr);
	kfree(fwu);

 exit:
	return 0;
}

static void synaptics_rmi4_fwu_remove(struct synaptics_rmi4_data *rmi4_data)
{
	kfree(fwu->fn_ptr);
	kfree(fwu);
	return;
}

static int __init rmi4_fw_update_module_init(void)
{
	synaptics_rmi4_new_function(RMI_FW_UPDATER, true,
				    synaptics_rmi4_fwu_init,
				    synaptics_rmi4_fwu_remove,
				    synaptics_rmi4_fwu_attn);
	return 0;
}

static void __exit rmi4_fw_update_module_exit(void)
{
	synaptics_rmi4_new_function(RMI_FW_UPDATER, false,
				    synaptics_rmi4_fwu_init,
				    synaptics_rmi4_fwu_remove,
				    synaptics_rmi4_fwu_attn);
	return;
}

module_init(rmi4_fw_update_module_init);
module_exit(rmi4_fw_update_module_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("RMI4 FW Update Module");
MODULE_LICENSE("GPL");
