/*
 * Copyright (c) 2011, 2012 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/rmi4.h>
#include <linux/slab.h>
#include <linux/version.h>
#include "rmi_driver.h"
#include "rmi_f34.h"

#define BLK_SZ_OFF	3
#define IMG_BLK_CNT_OFF	5
#define CFG_BLK_CNT_OFF	7

#define BLK_NUM_OFF 2
#define FUNCTION_NUMBER 0x34

/* data specific to fn $34 that needs to be kept around */
struct rmi_fn_34_data {
	u8 status;
	u8 cmd;
	u16 bootloaderid;
	u16 blocksize;
	u16 imageblockcount;
	u16 configblockcount;
	u16 blocknum;
	bool inflashprogmode;
	struct mutex attn_mutex;
};

static int f34_read_status(struct rmi_function_container *fc)
{
	struct rmi_fn_34_data *instance_data = fc->data;
	u16 data_base_addr = fc->fd.data_base_addr;
	u8 status;
	int retval;

	if (instance_data->status == ECONNRESET)
		return instance_data->status;

	/* Read the Fn $34 status from F34_Flash_Data3 to see the previous
	 * commands status. F34_Flash_Data3 will be the address after the
	 * 2 block number registers plus blocksize Data registers.
	 *  inform user space - through a sysfs param. */
	retval = rmi_read(fc->rmi_dev,
			  data_base_addr + instance_data->blocksize +
			  BLK_NUM_OFF, &status);

	if (retval < 0) {
		dev_err(&fc->dev, "Could not read status from 0x%x\n",
		       data_base_addr + instance_data->blocksize + BLK_NUM_OFF);
		status = 0xff;	/* failure */
	}

	/* set a sysfs value that the user mode can read - only
	 * upper 4 bits are the status. successful is $80, anything
	 * else is failure */
	instance_data->status = status & 0xf0;

	/* put mode into Flash Prog Mode when we successfully do
	 * an Enable Flash Prog cmd. */
	if ((instance_data->status == F34_STATUS_IDLE) &&
		(instance_data->cmd == F34_ENABLE_FLASH_PROG))
		instance_data->inflashprogmode = true;

	return retval;
}

static ssize_t rmi_fn_34_bootloaderid_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%u\n", instance_data->bootloaderid);
}

static ssize_t rmi_fn_34_bootloaderid_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	int error;
	unsigned long val;
	u8 data[2];
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;
	u16 data_base_addr;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;

	/* need to convert the string data to an actual value */
	error = strict_strtoul(buf, 10, &val);

	if (error)
		return error;

	instance_data->bootloaderid = val;

	/* Write the Bootloader ID key data back to the first two Block
	 * Data registers (F34_Flash_Data2.0 and F34_Flash_Data2.1). */
	hstoba(data, (u16)val);
	data_base_addr = fc->fd.data_base_addr;

	error = rmi_write_block(fc->rmi_dev,
				data_base_addr + BLK_NUM_OFF,
				data,
				ARRAY_SIZE(data));

	if (error < 0) {
		dev_err(dev, "%s : Could not write bootloader id to 0x%x\n",
		       __func__, data_base_addr + BLK_NUM_OFF);
		return error;
	}

	return count;
}

static ssize_t rmi_fn_34_blocksize_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%u\n", instance_data->blocksize);
}

static ssize_t rmi_fn_34_imageblockcount_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%u\n",
			instance_data->imageblockcount);
}

static ssize_t rmi_fn_34_configblockcount_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%u\n",
			instance_data->configblockcount);
}

static ssize_t rmi_fn_34_status_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;
	int retval;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;

	mutex_lock(&instance_data->attn_mutex);
	retval = f34_read_status(fc);
	mutex_unlock(&instance_data->attn_mutex);

	if (retval < 0)
		return retval;

	return snprintf(buf, PAGE_SIZE, "%u\n", instance_data->status);
}


static ssize_t rmi_fn_34_status_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;

	instance_data->status = 0;

	return 0;
}


static ssize_t rmi_fn_34_cmd_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%u\n", instance_data->cmd);
}

static ssize_t rmi_fn_34_cmd_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;
	unsigned long val;
	u16 data_base_addr;
	int error;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;
	data_base_addr = fc->fd.data_base_addr;

	/* need to convert the string data to an actual value */
	error = strict_strtoul(buf, 10, &val);
	if (error)
		return error;

	/* make sure we are in Flash Prog mode for all cmds except the
	 * Enable Flash Programming cmd - otherwise we are in error */
	if ((val != F34_ENABLE_FLASH_PROG) && !instance_data->inflashprogmode) {
		dev_err(dev, "Cannot send command to sensor - not in flash programming mode.\n");
		return -EINVAL;
	}

	instance_data->cmd = val;

	/* Validate command value and (if necessary) write it to the command
	 * register.
	 */
	switch (instance_data->cmd) {
	case F34_ENABLE_FLASH_PROG:
	case F34_ERASE_ALL:
	case F34_ERASE_CONFIG:
	case F34_WRITE_FW_BLOCK:
	case F34_READ_CONFIG_BLOCK:
	case F34_WRITE_CONFIG_BLOCK:
		/* Reset the status to indicate we are in progress on a cmd. */
		/* The status will change when the ATTN interrupt happens
		   and the status of the cmd that was issued is read from
		   the F34_Flash_Data3 register - result should be 0x80 for
		   success - any other value indicates an error */

		/* Issue the command to the device. */
		error = rmi_write(fc->rmi_dev,
				data_base_addr + instance_data->blocksize +
				BLK_NUM_OFF, instance_data->cmd);

		if (error < 0) {
			dev_err(dev, "Could not write command %#04x to %#06x\n",
				instance_data->cmd,
				data_base_addr + instance_data->blocksize +
				BLK_NUM_OFF);
			return error;
		}

		if (instance_data->cmd == F34_ENABLE_FLASH_PROG)
			instance_data->inflashprogmode = true;

		/* set status to indicate we are in progress */
		instance_data->status = F34_STATUS_IN_PROGRESS;
		break;
	default:
		dev_dbg(dev, "RMI4 F34 - unknown command 0x%02lx.\n", val);
		count = -EINVAL;
		break;
	}

	return count;
}

static ssize_t rmi_fn_34_blocknum_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%u\n", instance_data->blocknum);
}

static ssize_t rmi_fn_34_blocknum_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	int error;
	unsigned long val;
	u8 data[2];
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;
	u16 data_base_addr;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;
	data_base_addr = fc->fd.data_base_addr;

	/* need to convert the string data to an actual value */
	error = strict_strtoul(buf, 10, &val);

	if (error)
		return error;

	instance_data->blocknum = val;

	/* Write the Block Number data back to the first two Block
	 * Data registers (F34_Flash_Data_0 and F34_Flash_Data_1). */
	hstoba(data, (u16)val);

	error = rmi_write_block(fc->rmi_dev, data_base_addr,
				data, ARRAY_SIZE(data));

	if (error < 0) {
		dev_err(dev, "Could not write block number %u to %#06x.\n",
		       instance_data->blocknum, data_base_addr);
		return error;
	}

	return count;
}

static ssize_t rmi_fn_34_rescanPDT_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;
	struct rmi_device *rmi_dev;
	struct rmi_driver_data *driver_data;
	struct pdt_entry pdt_entry;
	bool fn01found = false;
	bool fn34found = false;
	unsigned int rescan;
	int irq_count = 0;
	int retval = 0;
	int i;

	/* Rescan of the PDT is needed since issuing the Flash Enable cmd
	 * the device registers for Fn$01 and Fn$34 moving around because
	 * of the change from Bootloader mode to Flash Programming mode
	 * may change to a different PDT with only Fn$01 and Fn$34 that
	 * could have addresses for query, control, data, command registers
	 * that differ from the PDT scan done at device initialization. */

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;
	rmi_dev = fc->rmi_dev;
	driver_data = dev_get_drvdata(&rmi_dev->dev);

	/* Make sure we are only in Flash Programming mode  - DON'T
	 * ALLOW THIS IN UI MODE. */
	if (instance_data->cmd != F34_ENABLE_FLASH_PROG) {
		dev_err(dev, "Not in flash programming mode, cannot rescan PDT.");
		return -EINVAL;
	}

	/* The only good value to write to this is 1, we allow 0, but with
	 * no effect (this is consistent with the way the command bit works. */
	if (sscanf(buf, "%u", &rescan) != 1)
		return -EINVAL;
	if (rescan < 0 || rescan > 1)
		return -EINVAL;

	/* 0 has no effect, so we skip it entirely. */
	if (rescan) {
		/* rescan the PDT - filling in Fn01 and Fn34 addresses -
		 * this is only temporary - the device will need to be reset
		 * to return the PDT to the normal values. */

		/* mini-parse the PDT - we only have to get Fn$01 and Fn$34 and
		   since we are Flash Programming mode we only have page 0. */
		for (i = PDT_START_SCAN_LOCATION; i >= PDT_END_SCAN_LOCATION;
			i -= sizeof(pdt_entry)) {
			retval = rmi_read_block(rmi_dev, i, (u8 *)&pdt_entry,
					       sizeof(pdt_entry));
			if (retval != sizeof(pdt_entry)) {
				dev_err(dev, "Failed to read PDT entry at %#06x, error = %d.",
						retval, i);
				return retval;
			}

			if ((pdt_entry.function_number == 0x00) ||
				(pdt_entry.function_number == 0xff))
				break;

			dev_dbg(dev, "%s: Found F%.2X\n",
				__func__, pdt_entry.function_number);

			/* f01 found - just fill in the new addresses in
			 * the existing fc. */
			if (pdt_entry.function_number == 0x01) {
				struct rmi_function_container *f01_fc =
					driver_data->f01_container;
				fn01found = true;
				f01_fc->fd.query_base_addr =
					pdt_entry.query_base_addr;
				f01_fc->fd.command_base_addr =
				  pdt_entry.command_base_addr;
				f01_fc->fd.control_base_addr =
				  pdt_entry.control_base_addr;
				f01_fc->fd.data_base_addr =
				  pdt_entry.data_base_addr;
				f01_fc->fd.function_number =
				  pdt_entry.function_number;
				f01_fc->fd.interrupt_source_count =
				  pdt_entry.interrupt_source_count;
				f01_fc->num_of_irqs =
				  pdt_entry.interrupt_source_count;
				f01_fc->irq_pos = irq_count;

				irq_count += f01_fc->num_of_irqs;

				if (fn34found)
					break;
			}

			/* f34 found - just fill in the new addresses in
			 * the existing fc. */
			if (pdt_entry.function_number == 0x34) {
				fn34found = true;
				fc->fd.query_base_addr =
				  pdt_entry.query_base_addr;
				fc->fd.command_base_addr =
				  pdt_entry.command_base_addr;
				fc->fd.control_base_addr =
				  pdt_entry.control_base_addr;
				fc->fd.data_base_addr =
				  pdt_entry.data_base_addr;
				fc->fd.function_number =
				  pdt_entry.function_number;
				fc->fd.interrupt_source_count =
				  pdt_entry.interrupt_source_count;
				fc->num_of_irqs =
				  pdt_entry.interrupt_source_count;
				fc->irq_pos = irq_count;

				irq_count += fc->num_of_irqs;

				if (fn01found)
					break;
			}

		}

		if (!fn01found || !fn34found) {
			dev_err(dev, "F01 or F34 not found in PDT rescan.\n");
			return -EINVAL;
		}
	}

	return count;
}

static struct device_attribute attrs[] = {
	__ATTR(status, RMI_RW_ATTR,
	       rmi_fn_34_status_show, rmi_fn_34_status_store),

	/* Also, sysfs will need to have a file set up to distinguish
	 * between commands - like Config write/read, Image write/verify. */
	__ATTR(cmd, RMI_RW_ATTR,
	       rmi_fn_34_cmd_show, rmi_fn_34_cmd_store),
	__ATTR(bootloaderid, RMI_RW_ATTR,
	       rmi_fn_34_bootloaderid_show, rmi_fn_34_bootloaderid_store),
	__ATTR(blocksize, RMI_RO_ATTR,
	       rmi_fn_34_blocksize_show, NULL),
	__ATTR(imageblockcount, RMI_RO_ATTR,
	       rmi_fn_34_imageblockcount_show, NULL),
	__ATTR(configblockcount, RMI_RO_ATTR,
	       rmi_fn_34_configblockcount_show, NULL),
	__ATTR(blocknum, RMI_RW_ATTR,
	       rmi_fn_34_blocknum_show, rmi_fn_34_blocknum_store),
	__ATTR(rescanPDT, RMI_WO_ATTR,
	       NULL, rmi_fn_34_rescanPDT_store)
};

static ssize_t rmi_fn_34_data_read(struct file *data_file,
				struct kobject *kobj,
				struct bin_attribute *attributes,
				char *buf,
				loff_t pos,
				size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;
	u16 data_base_addr;
	int error;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;

	data_base_addr = fc->fd.data_base_addr;

	if (count != instance_data->blocksize) {
		dev_err(dev, "Incorrect F34 block size %d. Expected size %d.\n",
				count, instance_data->blocksize);
		return -EINVAL;
	}

	/* Read the data from flash into buf.  The app layer will be blocked
	 * at reading from the sysfs file.  When we return the count (or
	 * error if we fail) the app will resume. */
	error = rmi_read_block(fc->rmi_dev, data_base_addr + BLK_NUM_OFF,
			(u8 *)buf, count);

	if (error < 0) {
		dev_err(dev, "Could not read data from %#06x\n",
				data_base_addr + BLK_NUM_OFF);
		return error;
	}

	return count;
}

static ssize_t rmi_fn_34_data_write(struct file *data_file,
				struct kobject *kobj,
				struct bin_attribute *attributes,
				char *buf,
				loff_t pos,
				size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct rmi_function_container *fc;
	struct rmi_fn_34_data *instance_data;
	u16 data_base_addr;
	int error;

	fc = to_rmi_function_container(dev);
	instance_data = fc->data;

	data_base_addr = fc->fd.data_base_addr;

	/* Write the data from buf to flash. The app layer will be
	 * blocked at writing to the sysfs file.  When we return the
	 * count (or error if we fail) the app will resume. */

	if (count != instance_data->blocksize) {
		dev_err(dev, "Incorrect F34 block size %d. Expected size %d.\n",
			count, instance_data->blocksize);
		return -EINVAL;
	}

	/* Write the data block - only if the count is non-zero  */
	if (count) {
		error = rmi_write_block(fc->rmi_dev,
				data_base_addr + BLK_NUM_OFF,
				(u8 *)buf, count);

		if (error < 0) {
			dev_err(dev, "%s : Could not write block data to 0x%x\n",
				__func__, data_base_addr + BLK_NUM_OFF);
			return error;
		}
	}

	return count;
}


struct bin_attribute dev_attr_data = {
	.attr = {
		 .name = "data",
		 .mode = 0666},
	.size = 0,
	.read = rmi_fn_34_data_read,
	.write = rmi_fn_34_data_write,
};

static int rmi_f34_alloc_memory(struct rmi_function_container *fc)
{
	struct rmi_fn_34_data *f34;

	f34 = devm_kzalloc(&fc->dev, sizeof(struct rmi_fn_34_data), GFP_KERNEL);
	if (!f34) {
		dev_err(&fc->dev, "Failed to allocate rmi_fn_34_data.\n");
		return -ENOMEM;
	}
	fc->data = f34;

	return 0;
}

static int rmi_f34_initialize(struct rmi_function_container *fc)
{
	struct rmi_device *rmi_dev = fc->rmi_dev;
	struct rmi_device_platform_data *pdata;
	int retval = 0;
	struct rmi_fn_34_data *f34 = fc->data;
	u16 query_base_addr;
	u16 control_base_addr;
	u8 buf[2];

	pdata = to_rmi_platform_data(rmi_dev);
	dev_dbg(&fc->dev, "Initializing F34 values for %s.\n",
		pdata->sensor_name);

	mutex_init(&f34->attn_mutex);

	/* get the Bootloader ID and Block Size. */
	query_base_addr = fc->fd.query_base_addr;
	control_base_addr = fc->fd.control_base_addr;

	retval = rmi_read_block(fc->rmi_dev, query_base_addr, buf,
			ARRAY_SIZE(buf));

	if (retval < 0) {
		dev_err(&fc->dev, "Could not read bootloaderid from 0x%04x.\n",
			query_base_addr);
		return retval;
	}

	f34->bootloaderid = batohs(buf);

	retval = rmi_read_block(fc->rmi_dev, query_base_addr + BLK_SZ_OFF, buf,
			ARRAY_SIZE(buf));

	if (retval < 0) {
		dev_err(&fc->dev, "Could not read block size from 0x%04x, error=%d.\n",
			query_base_addr + BLK_SZ_OFF, retval);
		return retval;
	}
	f34->blocksize = batohs(buf);

	/* Get firmware image block count and store it in the instance data */
	retval = rmi_read_block(fc->rmi_dev, query_base_addr + IMG_BLK_CNT_OFF,
			buf, ARRAY_SIZE(buf));

	if (retval < 0) {
		dev_err(&fc->dev, "Couldn't read image block count from 0x%x, error=%d.\n",
			query_base_addr + IMG_BLK_CNT_OFF, retval);
		return retval;
	}
	f34->imageblockcount = batohs(buf);

	/* Get config block count and store it in the instance data */
	retval = rmi_read_block(fc->rmi_dev, query_base_addr + 7, buf,
			ARRAY_SIZE(buf));

	if (retval < 0) {
		dev_err(&fc->dev, "Couldn't read config block count from 0x%x, error=%d.\n",
			query_base_addr + CFG_BLK_CNT_OFF, retval);
		return retval;
	}
	f34->configblockcount = batohs(buf);

	return 0;
}

static int rmi_f34_create_sysfs(struct rmi_function_container *fc)
{
	int attr_count = 0;
	int rc;

	dev_dbg(&fc->dev, "Creating sysfs files.");

	/* We need a sysfs file for the image/config block to write or read.
	 * Set up sysfs bin file for binary data block. Since the image is
	 * already in our format there is no need to convert the data for
	 * endianess. */
	rc = sysfs_create_bin_file(&fc->dev.kobj,
				&dev_attr_data);
	if (rc < 0) {
		dev_err(&fc->dev, "Failed to create sysfs file for F34 data, error=%d.\n",
			rc);
		return -ENODEV;
	}

	/* Set up sysfs device attributes. */
	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		if (sysfs_create_file
		    (&fc->dev.kobj, &attrs[attr_count].attr) < 0) {
			dev_err(&fc->dev, "Failed to create sysfs file for %s.",
			     attrs[attr_count].attr.name);
			rc = -ENODEV;
			goto err_remove_sysfs;
		}
	}

	return 0;

err_remove_sysfs:
	sysfs_remove_bin_file(&fc->dev.kobj, &dev_attr_data);

	for (attr_count--; attr_count >= 0; attr_count--)
		sysfs_remove_file(&fc->dev.kobj,
				  &attrs[attr_count].attr);
	return rc;
}

static int f34_device_init(struct rmi_function_container *fc)
{
	int retval;

	dev_info(&fc->dev, "Intializing f34 values.");

	/* init instance data, fill in values and create any sysfs files */
	retval = rmi_f34_alloc_memory(fc);
	if (retval < 0)
		return retval;

	retval = rmi_f34_initialize(fc);
	if (retval < 0)
		return retval;

	retval = rmi_f34_create_sysfs(fc);
	if (retval < 0)
		return retval;

	return 0;
}

static int rmi_f34_config(struct rmi_function_container *fc)
{
	/* for this function we should do nothing here */
	return 0;
}


static int rmi_f34_reset(struct rmi_function_container *fc)
{
	struct  rmi_fn_34_data  *instance_data = fc->data;

	instance_data->status = ECONNRESET;

	return 0;
}

static int f34_remove_device(struct device *dev)
{
	int attr_count;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	sysfs_remove_bin_file(&fc->dev.kobj, &dev_attr_data);

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++)
		sysfs_remove_file(&fc->dev.kobj, &attrs[attr_count].attr);

	return 0;
}

int rmi_f34_attention(struct rmi_function_container *fc,
					unsigned long *irq_bits)
{
	int retval;
	struct rmi_fn_34_data *data = fc->data;

	mutex_lock(&data->attn_mutex);
	retval = f34_read_status(fc);
	mutex_unlock(&data->attn_mutex);
	return retval;
}

static __devinit int f34_probe(struct device *dev)
{
	struct rmi_function_container *fc;

	if (dev->type != &rmi_function_type)
		return -ENODEV;

	fc = to_rmi_function_container(dev);
	if (fc->fd.function_number != FUNCTION_NUMBER)
		return -ENODEV;

	return f34_device_init(fc);
}

static struct rmi_function_handler function_handler = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "rmi_f34",
		.bus = &rmi_bus_type,
		.probe = f34_probe,
		.remove = f34_remove_device,
	},
	.func = FUNCTION_NUMBER,
	.config = rmi_f34_config,
	.reset = rmi_f34_reset,
	.attention = rmi_f34_attention,
};

static int __init rmi_f34_module_init(void)
{
	int error;

	error = driver_register(&function_handler.driver);
	if (error < 0) {
		pr_err("%s : register failed !\n", __func__);
		return error;
	}

	return 0;
}

static void rmi_f34_module_exit(void)
{
	driver_unregister(&function_handler.driver);
}

module_init(rmi_f34_module_init);
module_exit(rmi_f34_module_exit);

MODULE_AUTHOR("William Manson <wmanson@synaptics.com");
MODULE_DESCRIPTION("RMI F34 module");
MODULE_LICENSE("GPL");
MODULE_VERSION(RMI_DRIVER_VERSION);
