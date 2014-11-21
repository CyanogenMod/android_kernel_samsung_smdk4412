/*
 * Copyright (c) 2011-2012 Synaptics Incorporated
 * Copyright (c) 2011 Unixphere
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
#include <linux/debugfs.h>
#include <linux/kconfig.h>
#include <linux/rmi4.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "rmi_driver.h"
#include "rmi_f01.h"

#define FUNCTION_NUMBER 0x01

/**
 * @reset - set this bit to force a firmware reset of the sensor.
 */
struct f01_device_commands {
	u8 reset:1;
	u8 reserved:7;
};

/**
 * @ctrl0 - see documentation in rmi_f01.h.
 * @interrupt_enable - A mask of per-function interrupts on the touch sensor.
 * @doze_interval - controls the interval between checks for finger presence
 * when the touch sensor is in doze mode, in units of 10ms.
 * @wakeup_threshold - controls the capacitance threshold at which the touch
 * sensor will decide to wake up from that low power state.
 * @doze_holdoff - controls how long the touch sensor waits after the last
 * finger lifts before entering the doze state, in units of 100ms.
 */
struct f01_device_control {
	struct f01_device_control_0 ctrl0;
	u8 *interrupt_enable;
	u8 doze_interval;
	u8 wakeup_threshold;
	u8 doze_holdoff;
};

/**
 * @has_ds4_queries - if true, the query registers relating to Design Studio 4
 * features are present.
 * @has_multi_phy - if true, multiple physical communications interfaces are
 * supported.
 * @has_guest - if true, a "guest" device is supported.
 */
struct f01_query_42 {
		u8 has_ds4_queries:1;
		u8 has_multi_phy:1;
		u8 has_guest:1;
		u8 reserved:5;
} __attribute__((__packed__));

/**
 * @length - the length of the remaining Query43.* register block, not
 * including the first register.
 * @has_package_id_query -  the package ID query data will be accessible from
 * inside the ProductID query registers.
 * @has_packrat_query -  the packrat query data will be accessible from inside
 * the ProductID query registers.
 * @has_reset_query - the reset pin related registers are valid.
 * @has_maskrev_query - the silicon mask revision number will be reported.
 * @has_i2c_control - the register F01_RMI_Ctrl6 will exist.
 * @has_spi_control - the register F01_RMI_Ctrl7 will exist.
 * @has_attn_control - the register F01_RMI_Ctrl8 will exist.
 * @reset_enabled - the hardware reset pin functionality has been enabled
 * for this device.
 * @reset_polarity - If this bit reports as ‘0’, it means that the reset state
 * is active low. A ‘1’ means that the reset state is active high.
 * @pullup_enabled - If set, it indicates that a built-in weak pull up has
 * been enabled on the Reset pin; clear means that no pull-up is present.
 * @reset_pin_number - This field represents which GPIO pin number has been
 * assigned the reset functionality.
 */
struct f01_ds4_queries {
	u8 length:4;
	u8 reserved_1:4;

	u8 has_package_id_query:1;
	u8 has_packrat_query:1;
	u8 has_reset_query:1;
	u8 has_maskrev_query:1;
	u8 reserved_2:4;

	u8 has_i2c_control:1;
	u8 has_spi_control:1;
	u8 has_attn_control:1;
	u8 reserved_3:5;

	u8 reset_enabled:1;
	u8 reset_polarity:1;
	u8 pullup_enabled:1;
	u8 reserved_4:1;
	u8 reset_pin_number:4;
} __attribute__((__packed__));

struct f01_data {
	struct f01_device_control device_control;
	struct f01_basic_queries basic_queries;
	struct f01_device_status device_status;
	u8 product_id[RMI_PRODUCT_ID_LENGTH+1];

	u16 interrupt_enable_addr;
	u16 doze_interval_addr;
	u16 wakeup_threshold_addr;
	u16 doze_holdoff_addr;

	int irq_count;
	int num_of_irq_regs;

#ifdef	CONFIG_PM
	bool suspended;
	bool old_nosleep;
#endif

#ifdef	CONFIG_RMI4_DEBUG
	struct dentry *debugfs_interrupt_enable;
#endif
};

#ifdef	CONFIG_RMI4_DEBUG
struct f01_debugfs_data {
	bool done;
	struct rmi_function_container *fc;
};

static int f01_debug_open(struct inode *inodep, struct file *filp)
{
	struct f01_debugfs_data *data;
	struct rmi_function_container *fc = inodep->i_private;

	data = kzalloc(sizeof(struct f01_debugfs_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->fc = fc;
	filp->private_data = data;
	return 0;
}

static int f01_debug_release(struct inode *inodep, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

static ssize_t interrupt_enable_read(struct file *filp, char __user *buffer,
				     size_t size, loff_t *offset) {
	int i;
	int len;
	int total_len = 0;
	char local_buf[size];
	char *current_buf = local_buf;
	struct f01_debugfs_data *data = filp->private_data;
	struct f01_data *f01 = data->fc->data;

	if (data->done)
		return 0;

	data->done = 1;

	/* loop through each irq value and copy its
	 * string representation into buf */
	for (i = 0; i < f01->irq_count; i++) {
		int irq_reg;
		int irq_shift;
		int interrupt_enable;

		irq_reg = i / 8;
		irq_shift = i % 8;
		interrupt_enable =
		    ((f01->device_control.interrupt_enable[irq_reg]
			>> irq_shift) & 0x01);

		/* get next irq value and write it to buf */
		len = snprintf(current_buf, size - total_len,
			"%u ", interrupt_enable);
		/* bump up ptr to next location in buf if the
		 * snprintf was valid.  Otherwise issue an error
		 * and return. */
		if (len > 0) {
			current_buf += len;
			total_len += len;
		} else {
			dev_err(&data->fc->dev, "Failed to build interrupt_enable buffer, code = %d.\n",
						len);
			return snprintf(local_buf, size, "unknown\n");
		}
	}
	len = snprintf(current_buf, size - total_len, "\n");
	if (len > 0)
		total_len += len;
	else
		dev_warn(&data->fc->dev, "%s: Failed to append carriage return.\n",
			 __func__);

	if (copy_to_user(buffer, local_buf, total_len))
		return -EFAULT;

	return total_len;
}

static ssize_t interrupt_enable_write(struct file *filp,
		const char __user *buffer, size_t size, loff_t *offset) {
	int retval;
	char buf[size];
	char *local_buf = buf;
	int i;
	int irq_count = 0;
	int irq_reg = 0;
	struct f01_debugfs_data *data = filp->private_data;
	struct f01_data *f01 = data->fc->data;

	retval = copy_from_user(buf, buffer, size);
	if (retval)
		return -EFAULT;

	for (i = 0; i < f01->irq_count && *local_buf != 0;
	     i++, local_buf += 2) {
		int irq_shift;
		int interrupt_enable;
		int result;

		irq_reg = i / 8;
		irq_shift = i % 8;

		/* get next interrupt mapping value and store and bump up to
		 * point to next item in local_buf */
		result = sscanf(local_buf, "%u", &interrupt_enable);
		if ((result != 1) ||
			(interrupt_enable != 0 && interrupt_enable != 1)) {
			dev_err(&data->fc->dev, "Interrupt enable[%d] is not a valid value 0x%x.\n",
				i, interrupt_enable);
			return -EINVAL;
		}
		if (interrupt_enable == 0) {
			f01->device_control.interrupt_enable[irq_reg] &=
				(1 << irq_shift) ^ 0xFF;
		} else
			f01->device_control.interrupt_enable[irq_reg] |=
				(1 << irq_shift);
		irq_count++;
	}

	/* Make sure the irq count matches */
	if (irq_count != f01->irq_count) {
		dev_err(&data->fc->dev, "Interrupt enable count of %d doesn't match device count of %d.\n",
			 irq_count, f01->irq_count);
		return -EINVAL;
	}

	/* write back to the control register */
	retval = rmi_write_block(data->fc->rmi_dev, f01->interrupt_enable_addr,
			f01->device_control.interrupt_enable,
			f01->num_of_irq_regs);
	if (retval < 0) {
		dev_err(&data->fc->dev, "Could not write interrupt_enable mask to %#06x\n",
			f01->interrupt_enable_addr);
		return retval;
	}

	return size;
}

static const struct file_operations interrupt_enable_fops = {
	.owner = THIS_MODULE,
	.open = f01_debug_open,
	.release = f01_debug_release,
	.read = interrupt_enable_read,
	.write = interrupt_enable_write,
};

static int setup_debugfs(struct rmi_function_container *fc)
{
	struct f01_data *data = fc->data;

	if (!fc->debugfs_root)
		return -ENODEV;

	data->debugfs_interrupt_enable = debugfs_create_file("interrupt_enable",
		RMI_RW_ATTR, fc->debugfs_root, fc, &interrupt_enable_fops);
	if (!data->debugfs_interrupt_enable)
		dev_warn(&fc->dev,
			 "Failed to create debugfs interrupt_enable.\n");

	return 0;
}

static void teardown_debugfs(struct f01_data *f01)
{
	if (f01->debugfs_interrupt_enable)
		debugfs_remove(f01->debugfs_interrupt_enable);
}
#endif

/* Utility routine to set the value of a bit field in a register. */
int rmi_mask_and_set(struct rmi_device *rmi_dev,
		      u16 address,
		      u8 mask,
		      u8 set)
{
	u8 reg_contents;
	int retval;

	retval = rmi_read(rmi_dev, address, &reg_contents);
	if (retval < 0)
		return retval;
	reg_contents = (reg_contents & ~mask) | set;
	retval = rmi_write(rmi_dev, address, reg_contents);
	if (retval == 1)
		return 0;
	else if (retval == 0)
		return -EIO;
	return retval;
}

static ssize_t rmi_fn_01_productinfo_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "0x%02x 0x%02x\n",
			data->basic_queries.productinfo_1,
			data->basic_queries.productinfo_2);
}

static ssize_t rmi_fn_01_productid_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%s\n", data->product_id);
}

static ssize_t rmi_fn_01_manufacturer_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "0x%02x\n",
			data->basic_queries.manufacturer_id);
}

static ssize_t rmi_fn_01_datecode_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "20%02u-%02u-%02u\n",
			data->basic_queries.year,
			data->basic_queries.month,
			data->basic_queries.day);
}

static ssize_t rmi_fn_01_reset_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct rmi_function_container *fc = NULL;
	unsigned int reset;
	int retval = 0;

	fc = to_rmi_function_container(dev);

	if (sscanf(buf, "%u", &reset) != 1)
		return -EINVAL;
	if (reset < 0 || reset > 1)
		return -EINVAL;

	/* Per spec, 0 has no effect, so we skip it entirely. */
	if (reset) {
		/* Command register always reads as 0, so just use a local. */
		struct f01_device_commands commands = {
			.reset = 1
		};
		retval = rmi_write_block(fc->rmi_dev, fc->fd.command_base_addr,
				&commands, sizeof(commands));
		if (retval < 0) {
			dev_err(dev, "Failed to issue reset command, code = %d.",
						retval);
			return retval;
		}
	}

	return count;
}

static ssize_t rmi_fn_01_sleepmode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE,
			"%d\n", data->device_control.ctrl0.sleep_mode);
}

static ssize_t rmi_fn_01_sleepmode_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct f01_data *data = NULL;
	unsigned long new_value;
	int retval;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	retval = strict_strtoul(buf, 10, &new_value);
	if (retval < 0 || !RMI_IS_VALID_SLEEPMODE(new_value)) {
		dev_err(dev, "%s: Invalid sleep mode %s.", __func__, buf);
		return -EINVAL;
	}

	dev_dbg(dev, "Setting sleep mode to %ld.", new_value);
	data->device_control.ctrl0.sleep_mode = new_value;
	retval = rmi_write_block(fc->rmi_dev, fc->fd.control_base_addr,
			&data->device_control.ctrl0,
			sizeof(data->device_control.ctrl0));
	if (retval >= 0)
		retval = count;
	else
		dev_err(dev, "Failed to write sleep mode, code %d.\n", retval);
	return retval;
}

static ssize_t rmi_fn_01_nosleep_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%d\n",
		data->device_control.ctrl0.nosleep);
}

static ssize_t rmi_fn_01_nosleep_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct f01_data *data = NULL;
	unsigned long new_value;
	int retval;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	retval = strict_strtoul(buf, 10, &new_value);
	if (retval < 0 || new_value > 1) {
		dev_err(dev, "%s: Invalid nosleep bit %s.", __func__, buf);
		return -EINVAL;
	}

	data->device_control.ctrl0.nosleep = new_value;
	retval = rmi_write_block(fc->rmi_dev, fc->fd.control_base_addr,
			&data->device_control.ctrl0,
			sizeof(data->device_control.ctrl0));
	if (retval >= 0)
		retval = count;
	else
		dev_err(dev, "Failed to write nosleep bit.\n");
	return retval;
}

static ssize_t rmi_fn_01_chargerinput_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%d\n",
			data->device_control.ctrl0.charger_input);
}

static ssize_t rmi_fn_01_chargerinput_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct f01_data *data = NULL;
	unsigned long new_value;
	int retval;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	retval = strict_strtoul(buf, 10, &new_value);
	if (retval < 0 || new_value > 1) {
		dev_err(dev, "%s: Invalid chargerinput bit %s.", __func__, buf);
		return -EINVAL;
	}

	data->device_control.ctrl0.charger_input = new_value;
	retval = rmi_write_block(fc->rmi_dev, fc->fd.control_base_addr,
			&data->device_control.ctrl0,
			sizeof(data->device_control.ctrl0));
	if (retval >= 0)
		retval = count;
	else
		dev_err(dev, "Failed to write chargerinput bit.\n");
	return retval;
}

static ssize_t rmi_fn_01_reportrate_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%d\n",
			data->device_control.ctrl0.report_rate);
}

static ssize_t rmi_fn_01_reportrate_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct f01_data *data = NULL;
	unsigned long new_value;
	int retval;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	retval = strict_strtoul(buf, 10, &new_value);
	if (retval < 0 || new_value > 1) {
		dev_err(dev, "%s: Invalid reportrate bit %s.", __func__, buf);
		return -EINVAL;
	}

	data->device_control.ctrl0.report_rate = new_value;
	retval = rmi_write_block(fc->rmi_dev, fc->fd.control_base_addr,
			&data->device_control.ctrl0,
			sizeof(data->device_control.ctrl0));
	if (retval >= 0)
		retval = count;
	else
		dev_err(dev, "Failed to write reportrate bit.\n");
	return retval;
}

static ssize_t rmi_fn_01_interrupt_enable_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct rmi_function_container *fc;
	struct f01_data *data;
	int i, len, total_len = 0;
	char *current_buf = buf;

	fc = to_rmi_function_container(dev);
	data = fc->data;
	/* loop through each irq value and copy its
	 * string representation into buf */
	for (i = 0; i < data->irq_count; i++) {
		int irq_reg;
		int irq_shift;
		int interrupt_enable;

		irq_reg = i / 8;
		irq_shift = i % 8;
		interrupt_enable =
		    ((data->device_control.interrupt_enable[irq_reg]
			>> irq_shift) & 0x01);

		/* get next irq value and write it to buf */
		len = snprintf(current_buf, PAGE_SIZE - total_len,
			"%u ", interrupt_enable);
		/* bump up ptr to next location in buf if the
		 * snprintf was valid.  Otherwise issue an error
		 * and return. */
		if (len > 0) {
			current_buf += len;
			total_len += len;
		} else {
			dev_err(dev, "Failed to build interrupt_enable buffer, code = %d.\n",
						len);
			return snprintf(buf, PAGE_SIZE, "unknown\n");
		}
	}
	len = snprintf(current_buf, PAGE_SIZE - total_len, "\n");
	if (len > 0)
		total_len += len;
	else
		dev_warn(dev, "%s: Failed to append carriage return.\n",
			 __func__);
	return total_len;

}

static ssize_t rmi_fn_01_doze_interval_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%d\n",
			data->device_control.doze_interval);

}

static ssize_t rmi_fn_01_doze_interval_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct f01_data *data = NULL;
	unsigned long new_value;
	int retval;
	u16 ctrl_base_addr;

	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	retval = strict_strtoul(buf, 10, &new_value);
	if (retval < 0 || new_value > 255) {
		dev_err(dev, "%s: Invalid doze interval %s.", __func__, buf);
		return -EINVAL;
	}

	data->device_control.doze_interval = new_value;
	ctrl_base_addr = fc->fd.control_base_addr + sizeof(u8) +
			(sizeof(u8)*(data->num_of_irq_regs));
	dev_dbg(dev, "doze_interval store address %x, value %d",
		ctrl_base_addr, data->device_control.doze_interval);

	retval = rmi_write_block(fc->rmi_dev, data->doze_interval_addr,
			&data->device_control.doze_interval,
			sizeof(u8));
	if (retval >= 0)
		retval = count;
	else
		dev_err(dev, "Failed to write doze interval.\n");
	return retval;

}

static ssize_t rmi_fn_01_wakeup_threshold_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%d\n",
			data->device_control.wakeup_threshold);
}

static ssize_t rmi_fn_01_wakeup_threshold_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct f01_data *data = NULL;
	unsigned long new_value;
	int retval;

	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	retval = strict_strtoul(buf, 10, &new_value);
	if (retval < 0 || new_value > 255) {
		dev_err(dev, "%s: Invalid wakeup threshold %s.", __func__, buf);
		return -EINVAL;
	}

	data->device_control.doze_interval = new_value;
	retval = rmi_write_block(fc->rmi_dev, data->wakeup_threshold_addr,
			&data->device_control.wakeup_threshold,
			sizeof(u8));
	if (retval >= 0)
		retval = count;
	else
		dev_err(dev, "Failed to write wakeup threshold.\n");
	return retval;

}

static ssize_t rmi_fn_01_doze_holdoff_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%d\n",
			data->device_control.doze_holdoff);

}


static ssize_t rmi_fn_01_doze_holdoff_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct f01_data *data = NULL;
	unsigned long new_value;
	int retval;

	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	retval = strict_strtoul(buf, 10, &new_value);
	if (retval < 0 || new_value > 255) {
		dev_err(dev, "%s: Invalid doze holdoff %s.", __func__, buf);
		return -EINVAL;
	}

	data->device_control.doze_interval = new_value;
	retval = rmi_write_block(fc->rmi_dev, data->doze_holdoff_addr,
			&data->device_control.doze_holdoff,
			sizeof(u8));
	if (retval >= 0)
		retval = count;
	else
		dev_err(dev, "Failed to write doze holdoff.\n");
	return retval;

}

static ssize_t rmi_fn_01_configured_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%d\n",
			data->device_control.ctrl0.configured);
}

static ssize_t rmi_fn_01_unconfigured_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%d\n",
			data->device_status.unconfigured);
}

static ssize_t rmi_fn_01_flashprog_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "%d\n",
			data->device_status.flash_prog);
}

static ssize_t rmi_fn_01_statuscode_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct f01_data *data = NULL;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	data = fc->data;

	return snprintf(buf, PAGE_SIZE, "0x%02x\n",
			data->device_status.status_code);
}

static struct device_attribute fn_01_attrs[] = {
	__ATTR(productinfo, RMI_RO_ATTR,
	       rmi_fn_01_productinfo_show, NULL),
	__ATTR(productid, RMI_RO_ATTR,
	       rmi_fn_01_productid_show, NULL),
	__ATTR(manufacturer, RMI_RO_ATTR,
	       rmi_fn_01_manufacturer_show, NULL),
	__ATTR(datecode, RMI_RO_ATTR,
	       rmi_fn_01_datecode_show, NULL),

	/* control register access */
	__ATTR(sleepmode, RMI_RW_ATTR,
	       rmi_fn_01_sleepmode_show, rmi_fn_01_sleepmode_store),
	__ATTR(nosleep, RMI_RW_ATTR,
	       rmi_fn_01_nosleep_show, rmi_fn_01_nosleep_store),
	__ATTR(chargerinput, RMI_RW_ATTR,
	       rmi_fn_01_chargerinput_show, rmi_fn_01_chargerinput_store),
	__ATTR(reportrate, RMI_RW_ATTR,
	       rmi_fn_01_reportrate_show, rmi_fn_01_reportrate_store),
	/* We don't want arbitrary callers changing the interrupt enable mask,
	 * so it's read only.
	 */
	__ATTR(interrupt_enable, RMI_RO_ATTR,
	       rmi_fn_01_interrupt_enable_show, NULL),
	__ATTR(doze_interval, RMI_RW_ATTR,
	       rmi_fn_01_doze_interval_show, rmi_fn_01_doze_interval_store),
	__ATTR(wakeup_threshold, RMI_RW_ATTR,
	       rmi_fn_01_wakeup_threshold_show,
		rmi_fn_01_wakeup_threshold_store),
	__ATTR(doze_holdoff, RMI_RW_ATTR,
	       rmi_fn_01_doze_holdoff_show, rmi_fn_01_doze_holdoff_store),

	/* We make report rate RO, since the driver uses that to look for
	 * resets.  We don't want someone faking us out by changing that
	 * bit.
	 */
	__ATTR(configured, RMI_RO_ATTR,
	       rmi_fn_01_configured_show, NULL),

	/* Command register access. */
	__ATTR(reset, RMI_WO_ATTR,
	       NULL, rmi_fn_01_reset_store),

	/* STatus register access. */
	__ATTR(unconfigured, RMI_RO_ATTR,
	       rmi_fn_01_unconfigured_show, NULL),
	__ATTR(flashprog, RMI_RO_ATTR,
	       rmi_fn_01_flashprog_show, NULL),
	__ATTR(statuscode, RMI_RO_ATTR,
	       rmi_fn_01_statuscode_show, NULL),
};

static int rmi_f01_alloc_memory(struct rmi_function_container *fc,
	int num_of_irq_regs)
{
	struct f01_data *f01;

	f01 = devm_kzalloc(&fc->dev, sizeof(struct f01_data), GFP_KERNEL);
	if (!f01) {
		dev_err(&fc->dev, "Failed to allocate fn_01_data.\n");
		return -ENOMEM;
	}

	f01->device_control.interrupt_enable = devm_kzalloc(&fc->dev,
			sizeof(u8)*(num_of_irq_regs),
			GFP_KERNEL);
	if (!f01->device_control.interrupt_enable) {
		dev_err(&fc->dev, "Failed to allocate interrupt enable.\n");
		return -ENOMEM;
	}
	fc->data = f01;

	return 0;
}

static int rmi_f01_initialize(struct rmi_function_container *fc)
{
	u8 temp;
	int retval;
	u16 ctrl_base_addr;
	struct rmi_device *rmi_dev = fc->rmi_dev;
	struct rmi_driver_data *driver_data = dev_get_drvdata(&rmi_dev->dev);
	struct f01_data *data = fc->data;
	struct rmi_device_platform_data *pdata = to_rmi_platform_data(rmi_dev);

	/* Set the configured bit and (optionally) other important stuff
	 * in the device control register. */
	ctrl_base_addr = fc->fd.control_base_addr;
	retval = rmi_read_block(rmi_dev, fc->fd.control_base_addr,
			&data->device_control.ctrl0,
			sizeof(data->device_control.ctrl0));
	if (retval < 0) {
		dev_err(&fc->dev, "Failed to read F01 control.\n");
		return retval;
	}
	switch (pdata->power_management.nosleep) {
	case RMI_F01_NOSLEEP_DEFAULT:
		break;
	case RMI_F01_NOSLEEP_OFF:
		data->device_control.ctrl0.nosleep = 0;
		break;
	case RMI_F01_NOSLEEP_ON:
		data->device_control.ctrl0.nosleep = 1;
		break;
	}
	/* Sleep mode might be set as a hangover from a system crash or
	 * reboot without power cycle.  If so, clear it so the sensor
	 * is certain to function.
	 */
	if (data->device_control.ctrl0.sleep_mode != RMI_SLEEP_MODE_NORMAL) {
		dev_warn(&fc->dev,
			 "WARNING: Non-zero sleep mode found. Clearing...\n");
		data->device_control.ctrl0.sleep_mode = RMI_SLEEP_MODE_NORMAL;
	}

	data->device_control.ctrl0.configured = 1;
	retval = rmi_write_block(rmi_dev, fc->fd.control_base_addr,
			&data->device_control.ctrl0,
			sizeof(data->device_control.ctrl0));
	if (retval < 0) {
		dev_err(&fc->dev, "Failed to write F01 control.\n");
		return retval;
	}

	data->irq_count = driver_data->irq_count;
	data->num_of_irq_regs = driver_data->num_of_irq_regs;
	ctrl_base_addr += sizeof(struct f01_device_control_0);

	data->interrupt_enable_addr = ctrl_base_addr;
	retval = rmi_read_block(rmi_dev, ctrl_base_addr,
			data->device_control.interrupt_enable,
			sizeof(u8)*(data->num_of_irq_regs));
	if (retval < 0) {
		dev_err(&fc->dev, "Failed to read F01 control interrupt enable register.\n");
		goto error_exit;
	}
	ctrl_base_addr += data->num_of_irq_regs;

	/* dummy read in order to clear irqs */
	retval = rmi_read(rmi_dev, fc->fd.data_base_addr + 1, &temp);
	if (retval < 0) {
		dev_err(&fc->dev, "Failed to read Interrupt Status.\n");
		return retval;
	}

	retval = rmi_read_block(rmi_dev, fc->fd.query_base_addr,
				&data->basic_queries,
				sizeof(data->basic_queries));
	if (retval < 0) {
		dev_err(&fc->dev, "Failed to read device query registers.\n");
		return retval;
	}

	retval = rmi_read_block(rmi_dev,
		fc->fd.query_base_addr + sizeof(data->basic_queries),
		data->product_id, RMI_PRODUCT_ID_LENGTH);
	if (retval < 0) {
		dev_err(&fc->dev, "Failed to read product ID.\n");
		return retval;
	}
	data->product_id[RMI_PRODUCT_ID_LENGTH] = '\0';
	dev_info(&fc->dev, "found RMI device, manufacturer: %s, product: %s\n",
		 data->basic_queries.manufacturer_id == 1 ?
							"synaptics" : "unknown",
		 data->product_id);

	/* read control register */
	if (data->basic_queries.has_adjustable_doze) {
		data->doze_interval_addr = ctrl_base_addr;
		ctrl_base_addr++;

		if (pdata->power_management.doze_interval) {
			data->device_control.doze_interval =
				pdata->power_management.doze_interval;
			retval = rmi_write(rmi_dev, data->doze_interval_addr,
					data->device_control.doze_interval);
			if (retval < 0) {
				dev_err(&fc->dev, "Failed to configure F01 doze interval register.\n");
				goto error_exit;
			}
		} else {
			retval = rmi_read(rmi_dev, data->doze_interval_addr,
					&data->device_control.doze_interval);
			if (retval < 0) {
				dev_err(&fc->dev, "Failed to read F01 doze interval register.\n");
				goto error_exit;
			}
		}

		data->wakeup_threshold_addr = ctrl_base_addr;
		ctrl_base_addr++;

		if (pdata->power_management.wakeup_threshold) {
			data->device_control.wakeup_threshold =
				pdata->power_management.wakeup_threshold;
			retval = rmi_write(rmi_dev, data->wakeup_threshold_addr,
					data->device_control.wakeup_threshold);
			if (retval < 0) {
				dev_err(&fc->dev, "Failed to configure F01 wakeup threshold register.\n");
				goto error_exit;
			}
		} else {
			retval = rmi_read(rmi_dev, data->wakeup_threshold_addr,
					&data->device_control.wakeup_threshold);
			if (retval < 0) {
				dev_err(&fc->dev, "Failed to read F01 wakeup threshold register.\n");
				goto error_exit;
			}
		}
	}

	if (data->basic_queries.has_adjustable_doze_holdoff) {
		data->doze_holdoff_addr = ctrl_base_addr;
		ctrl_base_addr++;

		if (pdata->power_management.doze_holdoff) {
			data->device_control.doze_holdoff =
				pdata->power_management.doze_holdoff;
			retval = rmi_write(rmi_dev, data->doze_holdoff_addr,
					data->device_control.doze_holdoff);
			if (retval < 0) {
				dev_err(&fc->dev, "Failed to configure F01 doze holdoff register.\n");
				goto error_exit;
			}
		} else {
			retval = rmi_read(rmi_dev, data->doze_holdoff_addr,
					&data->device_control.doze_holdoff);
			if (retval < 0) {
				dev_err(&fc->dev, "Failed to read F01 doze holdoff register.\n");
				goto error_exit;
			}
		}
	}

	retval = rmi_read_block(rmi_dev, fc->fd.data_base_addr,
		&data->device_status, sizeof(data->device_status));
	if (retval < 0) {
		dev_err(&fc->dev, "Failed to read device status.\n");
		goto error_exit;
	}

	if (data->device_status.unconfigured) {
		dev_err(&fc->dev, "Device reset during configuration process, status: %#02x!\n",
				data->device_status.status_code);
		retval = -EINVAL;
		goto error_exit;
	}

	if (IS_ENABLED(CONFIG_RMI4_DEBUG)) {
		retval = setup_debugfs(fc);
		if (retval < 0)
			dev_warn(&fc->dev, "Failed to setup debugfs. Code: %d.\n",
				retval);
	}

	return retval;

 error_exit:
	kfree(data);
	return retval;
}

static int rmi_f01_create_sysfs(struct rmi_function_container *fc)
{
	int attr_count = 0;
	int retval = 0;
	struct f01_data *data = fc->data;

	dev_dbg(&fc->dev, "Creating sysfs files.");
	for (attr_count = 0; attr_count < ARRAY_SIZE(fn_01_attrs);
			attr_count++) {
		if (!strcmp("doze_interval", fn_01_attrs[attr_count].attr.name)
			&& !data->basic_queries.has_lts) {
			continue;
		}
		if (!strcmp("wakeup_threshold",
			fn_01_attrs[attr_count].attr.name)
			&& !data->basic_queries.has_adjustable_doze) {
			continue;
		}
		if (!strcmp("doze_holdoff", fn_01_attrs[attr_count].attr.name)
			&& !data->basic_queries.has_adjustable_doze_holdoff) {
			continue;
		}
		retval = sysfs_create_file(&fc->dev.kobj,
				      &fn_01_attrs[attr_count].attr);
		if (retval < 0) {
			dev_err(&fc->dev, "Failed to create sysfs file for %s.",
			       fn_01_attrs[attr_count].attr.name);
			goto err_remove_sysfs;
		}
	}

	return 0;

err_remove_sysfs:
	for (attr_count--; attr_count >= 0; attr_count--)
		sysfs_remove_file(&fc->dev.kobj,
				  &fn_01_attrs[attr_count].attr);

	return retval;
}

static int rmi_f01_config(struct rmi_function_container *fc)
{
	struct f01_data *data = fc->data;
	int retval;

	retval = rmi_write_block(fc->rmi_dev, fc->fd.control_base_addr,
			&data->device_control.ctrl0,
			sizeof(data->device_control.ctrl0));
	if (retval < 0) {
		dev_err(&fc->dev, "Failed to write device_control.reg.\n");
		return retval;
	}

	retval = rmi_write_block(fc->rmi_dev, data->interrupt_enable_addr,
			data->device_control.interrupt_enable,
			sizeof(u8)*(data->num_of_irq_regs));

	if (retval < 0) {
		dev_err(&fc->dev, "Failed to write interrupt enable.\n");
		return retval;
	}
	if (data->basic_queries.has_lts) {
		retval = rmi_write_block(fc->rmi_dev, data->doze_interval_addr,
				&data->device_control.doze_interval,
				sizeof(u8));
		if (retval < 0) {
			dev_err(&fc->dev, "Failed to write doze interval.\n");
			return retval;
		}
	}

	if (data->basic_queries.has_adjustable_doze) {
		retval = rmi_write_block(
				fc->rmi_dev, data->wakeup_threshold_addr,
				&data->device_control.wakeup_threshold,
				sizeof(u8));
		if (retval < 0) {
			dev_err(&fc->dev, "Failed to write wakeup threshold.\n");
			return retval;
		}
	}

	if (data->basic_queries.has_adjustable_doze_holdoff) {
		retval = rmi_write_block(fc->rmi_dev, data->doze_holdoff_addr,
				&data->device_control.doze_holdoff,
				sizeof(u8));
		if (retval < 0) {
			dev_err(&fc->dev, "Failed to write doze holdoff.\n");
			return retval;
		}
	}
	return 0;
}

static int f01_device_init(struct rmi_function_container *fc)
{
	struct rmi_driver_data *driver_data =
			dev_get_drvdata(&fc->rmi_dev->dev);
	int error;

	error = rmi_f01_alloc_memory(fc, driver_data->num_of_irq_regs);
	if (error < 0)
		return error;

	error = rmi_f01_initialize(fc);
	if (error < 0)
		return error;

	error = rmi_f01_create_sysfs(fc);
	if (error < 0)
		return error;

	return 0;
}

#ifdef CONFIG_PM
static int rmi_f01_suspend(struct rmi_function_container *fc)
{
	struct rmi_device *rmi_dev = fc->rmi_dev;
	struct f01_data *data = fc->data;
	int retval = 0;

	if (data->suspended)
		return 0;

	data->old_nosleep = data->device_control.ctrl0.nosleep;
	data->device_control.ctrl0.nosleep = 0;
	data->device_control.ctrl0.sleep_mode = RMI_SLEEP_MODE_SENSOR_SLEEP;

	retval = rmi_write_block(rmi_dev,
			fc->fd.control_base_addr,
			&data->device_control.ctrl0,
			sizeof(data->device_control.ctrl0));
	if (retval < 0) {
		dev_err(&fc->dev, "Failed to write sleep mode. Code: %d.\n",
			retval);
		data->device_control.ctrl0.nosleep = data->old_nosleep;
		data->device_control.ctrl0.sleep_mode = RMI_SLEEP_MODE_NORMAL;
	} else {
		data->suspended = true;
		retval = 0;
	}

	return retval;
}

static int rmi_f01_resume(struct rmi_function_container *fc)
{
	struct rmi_device *rmi_dev = fc->rmi_dev;
	struct f01_data *data = fc->data;
	int retval = 0;

	if (!data->suspended)
		return 0;

	data->device_control.ctrl0.nosleep = data->old_nosleep;
	data->device_control.ctrl0.sleep_mode = RMI_SLEEP_MODE_NORMAL;

	retval = rmi_write_block(rmi_dev, fc->fd.control_base_addr,
			&data->device_control.ctrl0,
			sizeof(data->device_control.ctrl0));
	if (retval < 0)
		dev_err(&fc->dev,
			"Failed to restore normal operation. Code: %d.\n",
			retval);
	else {
		data->suspended = false;
		retval = 0;
	}

	return retval;
}
#endif /* CONFIG_PM */

static int f01_remove_device(struct device *dev)
{
	int attr_count;
	struct rmi_function_container *fc = to_rmi_function_container(dev);

	if (IS_ENABLED(CONFIG_RMI4_DEBUG))
		teardown_debugfs(fc->data);

	for (attr_count = 0; attr_count < ARRAY_SIZE(fn_01_attrs);
			attr_count++) {
		sysfs_remove_file(&fc->dev.kobj, &fn_01_attrs[attr_count].attr);
	}
	return 0;
}

static int rmi_f01_attention(struct rmi_function_container *fc,
						unsigned long *irq_bits)
{
	struct rmi_device *rmi_dev = fc->rmi_dev;
	struct f01_data *data = fc->data;
	int retval;

	retval = rmi_read_block(rmi_dev, fc->fd.data_base_addr,
		&data->device_status, sizeof(data->device_status));
	if (retval < 0) {
		dev_err(&fc->dev, "Failed to read device status, code: %d.\n",
			retval);
		return retval;
	}
	if (data->device_status.unconfigured) {
		dev_warn(&fc->dev, "Device reset detected.\n");
		retval = rmi_dev->driver->reset_handler(rmi_dev);
		if (retval < 0)
			return retval;
	}
	return 0;
}

static __devinit int f01_probe(struct device *dev)
{
	struct rmi_function_container *fc;

	if (dev->type != &rmi_function_type)
		return -ENODEV;

	fc = to_rmi_function_container(dev);
	if (fc->fd.function_number != FUNCTION_NUMBER)
		return -ENODEV;

	return f01_device_init(fc);
}

static struct rmi_function_handler function_handler = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "rmi_f01",
		.bus = &rmi_bus_type,
		.probe = f01_probe,
		.remove = f01_remove_device,
	},
	.func = FUNCTION_NUMBER,
	.config = rmi_f01_config,
	.attention = rmi_f01_attention,

#ifdef	CONFIG_PM
#if defined(CONFIG_HAS_EARLYSUSPEND)
	.early_suspend = rmi_f01_suspend,
	.late_resume = rmi_f01_resume,
#else
	.suspend = rmi_f01_suspend,
	.resume = rmi_f01_resume,
#endif  /* defined(CONFIG_HAS_EARLYSUSPEND) && !def... */
#endif  /* CONFIG_PM */
};


static int __init rmi_f01_module_init(void)
{
	int error;

	error = driver_register(&function_handler.driver);
	if (error < 0) {
		pr_err("%s: register failed!\n", __func__);
		return error;
	}

	return 0;
}

static void __exit rmi_f01_module_exit(void)
{
	driver_unregister(&function_handler.driver);
}

module_init(rmi_f01_module_init);
module_exit(rmi_f01_module_exit);

MODULE_AUTHOR("Christopher Heiny <cheiny@synaptics.com>");
MODULE_DESCRIPTION("RMI F01 module");
MODULE_LICENSE("GPL");
MODULE_VERSION(RMI_DRIVER_VERSION);
