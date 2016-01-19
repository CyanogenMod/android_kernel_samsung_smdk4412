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
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/i2c/synaptics_rmi.h>
#include "synaptics_i2c_rmi.h"

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

#define F34_STATUS_IDLE 0x80
#define F34_STATUS_IN_PROGRESS 0xff

static ssize_t synaptics_rmi4_f34_data_read(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count);

static ssize_t synaptics_rmi4_f34_data_write(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count);

static ssize_t synaptics_rmi4_f34_bootloaderid_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f34_bootloaderid_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_f34_cmd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_f34_status_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f34_blocksize_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f34_imageblockcount_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f34_configblockcount_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f34_blocknum_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_f34_rescanPDT_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static int synaptics_rmi4_f34_read_status(
		struct synaptics_rmi4_data *rmi4_data);

struct synaptics_rmi4_f34_fn_ptr {
	int (*read)(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data,
		unsigned short length);
	int (*write)(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data,
		unsigned short length);
	int (*enable)(struct synaptics_rmi4_data *rmi4_data,
		bool enable);
};

struct synaptics_rmi4_f34_handle {
	unsigned char status;
	unsigned char cmd;
	unsigned short bootloaderid;
	unsigned short blocksize;
	unsigned short imageblockcount;
	unsigned short configblockcount;
	unsigned short blocknum;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	bool inflashprogmode;
	unsigned char intr_mask;
	struct mutex attn_mutex;
	struct synaptics_rmi4_f34_fn_ptr *fn_ptr;
};

struct bin_attribute dev_attr_data = {
	.attr = {
		.name = "data",
		.mode = 0666,
	},
	.size = 0,
	.read = synaptics_rmi4_f34_data_read,
	.write = synaptics_rmi4_f34_data_write,
};

static struct device_attribute attrs[] = {
	__ATTR(bootloaderid, (S_IRUGO | S_IWUGO),
			synaptics_rmi4_f34_bootloaderid_show,
			synaptics_rmi4_f34_bootloaderid_store),
	__ATTR(cmd, S_IWUGO,
			synaptics_rmi4_show_error,
			synaptics_rmi4_f34_cmd_store),
	__ATTR(status, S_IRUGO,
			synaptics_rmi4_f34_status_show,
			synaptics_rmi4_store_error),
	__ATTR(blocksize, S_IRUGO,
			synaptics_rmi4_f34_blocksize_show,
			synaptics_rmi4_store_error),
	__ATTR(imageblockcount, S_IRUGO,
			synaptics_rmi4_f34_imageblockcount_show,
			synaptics_rmi4_store_error),
	__ATTR(configblockcount, S_IRUGO,
			synaptics_rmi4_f34_configblockcount_show,
			synaptics_rmi4_store_error),
	__ATTR(blocknum, S_IWUGO,
			synaptics_rmi4_show_error,
			synaptics_rmi4_f34_blocknum_store),
	__ATTR(rescanPDT, S_IWUGO,
			synaptics_rmi4_show_error,
			synaptics_rmi4_f34_rescanPDT_store),
};

struct synaptics_rmi4_f34_handle *f34;

static struct completion remove_complete;

static inline void batohs(unsigned short *dest, unsigned char *src)
{
	*dest = src[1] * 0x100 + src[0];
}

static inline void hstoba(unsigned char *dest, unsigned short src)
{
	dest[0] = src % 0x100;
	dest[1] = src / 0x100;
}

static ssize_t synaptics_rmi4_f34_data_read(struct file *data_file,
	struct kobject *kobj, struct bin_attribute *attributes, char *buf,
	loff_t pos, size_t count)
{
	int retval;
	struct device *dev = container_of(kobj, struct device, kobj);
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (count != f34->blocksize) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Incorrect block size (%d), "
			"expected size = %d\n", __func__,
			count, f34->blocksize);
		return -EINVAL;
	}

	retval = f34->fn_ptr->read(rmi4_data,
			f34->data_base_addr + DATA_BLOCK_DATA_OFFSET,
			(unsigned char *)buf, count);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Could not read data from "
			"0x%04x\n", __func__,
			f34->data_base_addr + DATA_BLOCK_DATA_OFFSET);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_f34_data_write(struct file *data_file,
	struct kobject *kobj, struct bin_attribute *attributes, char *buf,
	loff_t pos, size_t count)
{
	int retval;
	struct device *dev = container_of(kobj, struct device, kobj);
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (count != f34->blocksize) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Incorrect block size (%d), "
			"expected size = %d\n", __func__,
			count, f34->blocksize);
		return -EINVAL;
	}

	if (count) {
		retval = f34->fn_ptr->write(rmi4_data,
				f34->data_base_addr + DATA_BLOCK_DATA_OFFSET,
				(unsigned char *)buf, count);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
				"%s Could not write data to "
				"0x%04x\n", __func__,
				f34->data_base_addr + DATA_BLOCK_DATA_OFFSET);
			return retval;
		}
	}

	return count;
}

static ssize_t synaptics_rmi4_f34_bootloaderid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f34->bootloaderid);
}

static ssize_t synaptics_rmi4_f34_bootloaderid_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned long id;
	unsigned char data[2];
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	retval = strict_strtoul(buf, 10, &id);
	if (retval)
		return retval;

	f34->bootloaderid = id;

	hstoba(data, (unsigned short)id);

	retval = f34->fn_ptr->write(rmi4_data,
			f34->data_base_addr + DATA_BLOCK_DATA_OFFSET, data,
			ARRAY_SIZE(data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Could not write bootloader ID "
			"to 0x%04x\n", __func__,
			f34->data_base_addr + DATA_BLOCK_DATA_OFFSET);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_f34_cmd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned long command;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	retval = strict_strtoul(buf, 10, &command);
	if (retval)
		return retval;

	if ((command != F34_CMD_ENABLE_FLASH_PROG) && !f34->inflashprogmode) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Could not send command 0x%02x "
			"to sensor - not in flash prog mode\n", __func__,
			(unsigned int)command);
		return -EINVAL;
	}

	f34->cmd = command;

	switch (f34->cmd) {
	case F34_CMD_WRITE_FW_BLOCK:
	case F34_CMD_ERASE_ALL:
	case F34_CMD_READ_CONFIG_BLOCK:
	case F34_CMD_WRITE_CONFIG_BLOCK:
	case F34_CMD_ERASE_CONFIG:
	case F34_CMD_ENABLE_FLASH_PROG:
		retval = f34->fn_ptr->write(rmi4_data,
			f34->data_base_addr + DATA_FLASH_COMMAND_OFFSET,
				&f34->cmd, sizeof(f34->cmd));
	if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
				"%s Could not write command "
				"0x%02x to 0x%04x\n", __func__, f34->cmd,
				f34->data_base_addr +
				DATA_FLASH_COMMAND_OFFSET);
			return retval;
		}

		if (f34->cmd == F34_CMD_ENABLE_FLASH_PROG)
			f34->inflashprogmode = true;

		f34->status = F34_STATUS_IN_PROGRESS;
		break;
	default:
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Unknown command 0x%02x\n", __func__, f34->cmd);
		count = -EINVAL;
	break;
	}

	return count;
}

static ssize_t synaptics_rmi4_f34_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	mutex_lock(&f34->attn_mutex);
	retval = synaptics_rmi4_f34_read_status(rmi4_data);
	mutex_unlock(&f34->attn_mutex);

	if (retval < 0)
		return retval;

	return snprintf(buf, PAGE_SIZE, "%u\n", f34->status);
}

static ssize_t synaptics_rmi4_f34_blocksize_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f34->blocksize);
}

static ssize_t synaptics_rmi4_f34_imageblockcount_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f34->imageblockcount);
}

static ssize_t synaptics_rmi4_f34_configblockcount_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f34->configblockcount);
}

static ssize_t synaptics_rmi4_f34_blocknum_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned long blocknum;
	unsigned char data[2];
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	retval = strict_strtoul(buf, 10, &blocknum);
	if (retval)
		return retval;

	f34->blocknum = blocknum;

	hstoba(data, (unsigned short)blocknum);

	retval = f34->fn_ptr->write(rmi4_data,
			f34->data_base_addr + DATA_BLOCK_NUMBER_OFFSET, data,
			ARRAY_SIZE(data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Could not write block number "
			"to 0x%04x\n", __func__,
			f34->data_base_addr + DATA_BLOCK_NUMBER_OFFSET);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_f34_rescanPDT_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned short addr;
	unsigned int rescan;
	bool f01found = false;
	bool f34found = false;
	struct synaptics_rmi4_fn_desc rmi_fd;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!f34->inflashprogmode) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Not in flash programming mode "
			"- cannot rescan PDT\n", __func__);
		return -EINVAL;
	}

	if (sscanf(buf, "%u", &rescan) != 1)
		return -EINVAL;

	if (rescan < 0 || rescan > 1)
		return -EINVAL;

	if (rescan) {
		for (addr = PDT_START;
			addr > PDT_END; addr -= PDT_ENTRY_SIZE) {
			retval = f34->fn_ptr->read(rmi4_data, addr,
					(unsigned char *)&rmi_fd,
					sizeof(rmi_fd));
			if (retval < 0)
				return retval;

			if (rmi_fd.fn_number) {
				dev_dbg(&rmi4_data->i2c_client->dev,
					"%s Found F%02x\n",
					__func__, rmi_fd.fn_number);
				switch (rmi_fd.fn_number) {
				case SYNAPTICS_RMI4_DEVICE_CONTROL_FN:
					f01found = true;
					rmi4_data->f01_query_base_addr =
						rmi_fd.query_base_addr;
					rmi4_data->f01_ctrl_base_addr =
						rmi_fd.ctrl_base_addr;
					rmi4_data->f01_data_base_addr =
						rmi_fd.data_base_addr;
					rmi4_data->f01_cmd_base_addr =
						rmi_fd.cmd_base_addr;
					break;
				case SYNAPTICS_RMI4_FLASH_FN:
					f34found = true;
					f34->query_base_addr =
						rmi_fd.query_base_addr;
					f34->control_base_addr =
						rmi_fd.ctrl_base_addr;
					f34->data_base_addr =
						rmi_fd.ctrl_base_addr;
					break;
				}
			} else {
				break;
			}
		}

		if (!f01found || !f34found) {
			dev_err(&rmi4_data->i2c_client->dev,\
				"%s Failed to find both F01 "
				"and F34 during PDT rescan" , __func__);
			return -EINVAL;
		}
	}

	return count;
}

static int synaptics_rmi4_f34_read_status(
	struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char command;
	unsigned char status;

	retval = f34->fn_ptr->read(rmi4_data,
			f34->data_base_addr + DATA_FLASH_COMMAND_OFFSET,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s Could not read command from "
				"0x%04x\n", __func__,
			f34->data_base_addr + DATA_FLASH_COMMAND_OFFSET);
	}

	retval = f34->fn_ptr->read(rmi4_data,
			f34->data_base_addr + DATA_FLASH_STATUS_OFFSET, &status,
			sizeof(status));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s Could not read status from "
				"0x%04x\n", __func__,
				f34->data_base_addr + DATA_FLASH_STATUS_OFFSET);
		status = 0xff;
	}

	f34->status = ((status & MASK_3BIT) << 4) | (command & MASK_4BIT);

	/* Program enabled bit not available - force bit to be set */
	f34->status |= 0x80;

	if ((f34->status == F34_STATUS_IDLE) &&
			(f34->cmd == F34_CMD_ENABLE_FLASH_PROG))
		f34->inflashprogmode = true;

	return retval;
}

static void synaptics_rmi4_f34_attn(struct synaptics_rmi4_data *rmi4_data,
		unsigned char intr_mask)
{
	if (f34->intr_mask & intr_mask) {
		mutex_lock(&f34->attn_mutex);
		synaptics_rmi4_f34_read_status(rmi4_data);
		mutex_unlock(&f34->attn_mutex);
	}

	return;
}

static int synaptics_rmi4_f34_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char attr_count;
	unsigned char intr_count = 0;
	unsigned char intr_offset;
	unsigned char buf[4];
	unsigned short ii;
	struct synaptics_rmi4_fn_desc rmi_fd;

	f34 = kzalloc(sizeof(*f34), GFP_KERNEL);
	if (!f34) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Failed to allocate memory for "
			"synaptics_rmi4_f34_handle\n", __func__);
		retval = -ENOMEM;
		goto exit;
	}

	f34->fn_ptr = kzalloc(sizeof(*(f34->fn_ptr)), GFP_KERNEL);

	f34->fn_ptr->read = rmi4_data->i2c_read;
	f34->fn_ptr->write = rmi4_data->i2c_write;
	f34->fn_ptr->enable = rmi4_data->irq_enable;

	mutex_init(&f34->attn_mutex);

	for (ii = PDT_START; ii > PDT_END; ii -= PDT_ENTRY_SIZE) {
		retval = f34->fn_ptr->read(rmi4_data,
				ii, (unsigned char *)&rmi_fd,
				sizeof(rmi_fd));
		if (retval < 0)
			goto exit_free_mem;

		if (rmi_fd.fn_number == SYNAPTICS_RMI4_FLASH_FN)
			break;

		intr_count += (rmi_fd.intr_src_count & 0x07);
	}

	f34->query_base_addr = rmi_fd.query_base_addr;
	f34->control_base_addr = rmi_fd.ctrl_base_addr;
	f34->data_base_addr = rmi_fd.ctrl_base_addr;

	f34->intr_mask = 0;
	intr_offset = intr_count % 8;

	for (ii = intr_offset;
			ii < ((rmi_fd.intr_src_count & 0x07) + intr_offset);
			ii++) {
		f34->intr_mask |= 1 << ii;
	}

	retval = f34->fn_ptr->read(rmi4_data,
			f34->query_base_addr + QUERY_BOOTLOADER_ID_OFFSET,
			buf, 2);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Failed to read bootloader "
			"ID\n", __func__);
		goto exit_free_mem;
	}

	batohs(&f34->bootloaderid, &(buf[0]));

	retval = f34->fn_ptr->read(rmi4_data,
			f34->query_base_addr + QUERY_BLOCK_SIZE_OFFSET,
			buf, 2);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Failed to read block size "
			"info\n", __func__);
		goto exit_free_mem;
	}

	batohs(&f34->blocksize, &(buf[0]));

	retval = f34->fn_ptr->read(rmi4_data,
			f34->query_base_addr + QUERY_BLOCK_COUNT_OFFSET,
			buf, 4);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Failed to read block count "
			"info\n", __func__);
		goto exit_free_mem;
	}

	batohs(&f34->imageblockcount, &(buf[0]));
	batohs(&f34->configblockcount, &(buf[2]));

	retval = sysfs_create_bin_file(&rmi4_data->input_dev->dev.kobj,
			&dev_attr_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Failed to create sysfs file "
			"for F34 data\n", __func__);
		goto exit_free_mem;
	}

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		retval = sysfs_create_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
				"%s Failed to create sysfs "
				"file for %s\n", __func__,
				attrs[attr_count].attr.name);
			retval = -ENODEV;
			goto exit_remove_sysfs;
		}
	}

	return 0;

exit_remove_sysfs:
	sysfs_remove_bin_file(&rmi4_data->input_dev->dev.kobj, &dev_attr_data);

	for (attr_count--; attr_count >= 0; attr_count--) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
	}

exit_free_mem:
	kfree(f34->fn_ptr);
	kfree(f34);

exit:
	return retval;
}

static void synaptics_rmi4_f34_remove(struct synaptics_rmi4_data *rmi4_data)
{
	unsigned char attr_count;

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
	}

	kfree(f34->fn_ptr);
	kfree(f34);

	complete(&remove_complete);

	return;
}

static int __init rmi4_f34_module_init(void)
{
	synaptics_rmi4_new_function(RMI_F34, true, synaptics_rmi4_f34_init,
			synaptics_rmi4_f34_remove,
			synaptics_rmi4_f34_attn);
	return 0;
}

static void __exit rmi4_f34_module_exit(void)
{
	init_completion(&remove_complete);
	synaptics_rmi4_new_function(RMI_F34, false, synaptics_rmi4_f34_init,
			synaptics_rmi4_f34_remove,
			synaptics_rmi4_f34_attn);
	wait_for_completion(&remove_complete);
	return;
}

module_init(rmi4_f34_module_init);
module_exit(rmi4_f34_module_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("RMI4 F34 Module");
MODULE_LICENSE("GPL");
