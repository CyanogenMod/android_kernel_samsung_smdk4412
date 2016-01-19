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
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/i2c/synaptics_rmi.h>
#include "synaptics_i2c_rmi.h"

#define DRIVER_NAME "synaptics_rmi4_i2c"
#define INPUT_PHYS_NAME "synaptics_rmi4_i2c/input0"

#define PROXIMITY 1

#define RPT_OBJ_TYPE (1 << 0)
#define RPT_X_LSB (1 << 1)
#define RPT_X_MSB (1 << 2)
#define RPT_Y_LSB (1 << 3)
#define RPT_Y_MSB (1 << 4)
#define RPT_Z (1 << 5)
#define RPT_WX (1 << 6)
#define RPT_WY (1 << 7)
#define F12_RPT_DEFAULT (RPT_OBJ_TYPE | RPT_X_LSB | RPT_Y_LSB | RPT_Y_MSB)
#define F12_RPT (F12_RPT_DEFAULT | RPT_WX | RPT_WY)

#if PROXIMITY
#define HOVER_Z_MAX 255
#define PROXIMITY_FINGER_HOVER (1 << 0)
#define PROXIMITY_AIR_SWIPE (1 << 1)
#define PROXIMITY_LARGE_OBJ (1 << 2)
#define PROXIMITY_HOVER_PINCH (1 << 4)
#define PROXIMITY_ENABLE PROXIMITY_FINGER_HOVER
#endif

#define EXP_FN_DET_INTERVAL 1000	/* ms */
#define SENSOR_POLLING_PERIOD 1	/* ms */
#define SYN_I2C_RETRY_TIMES 10
#define MAX_NUMBER_OF_FINGERS 8
#define MAX_NUMBER_OF_BUTTONS 4
#define MAX_INTR_REGISTERS 4
#define MAX_TOUCH_MAJOR 15
#define PAGE_SELECT_LEN 2
#define F01_STD_QUERY_LEN 21
#define F11_STD_QUERY_LEN 9
#define F11_STD_CTRL_LEN 10
#define F11_STD_DATA_LEN 12

#define NORMAL_OPERATION (0 << 0)
#define SENSOR_SLEEP (1 << 0)
#define NO_SLEEP_OFF (0 << 3)
#define NO_SLEEP_ON (1 << 3)

static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
				   unsigned short addr, unsigned char *data,
				   unsigned short length);

static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
				    unsigned short addr, unsigned char *data,
				    unsigned short length);

#ifdef CONFIG_HAS_EARLYSUSPEND
static ssize_t synaptics_rmi4_full_pm_cycle_show(struct device *dev,
						 struct device_attribute *attr,
						 char *buf);

static ssize_t synaptics_rmi4_full_pm_cycle_store(struct device *dev,
						  struct device_attribute *attr,
						  const char *buf,
						  size_t count);

static void synaptics_rmi4_early_suspend(struct early_suspend *h);

static void synaptics_rmi4_late_resume(struct early_suspend *h);

static int synaptics_rmi4_suspend(struct device *dev);

static int synaptics_rmi4_resume(struct device *dev);
#endif

static ssize_t synaptics_rmi4_f01_reset_store(struct device *dev,
					      struct device_attribute *attr,
					      const char *buf, size_t count);

static ssize_t synaptics_rmi4_f01_productinfo_show(
	struct device *dev, struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f01_flashprog_show(struct device *dev,
						 struct device_attribute *attr,
						 char *buf);

static ssize_t synaptics_rmi4_0dbutton_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf);

static ssize_t synaptics_rmi4_0dbutton_store(struct device *dev,
					     struct device_attribute *attr,
					     const char *buf, size_t count);

struct synaptics_rmi4_f01_device_status {
	union {
		struct {
			unsigned char status_code:4;
			unsigned char reserved:2;
			unsigned char flash_prog:1;
			unsigned char unconfigured:1;
		};
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f12_query_5 {
	union {
		struct {
			unsigned char size_of_query6;
			struct {
				unsigned char ctrl0_is_present:1;
				unsigned char ctrl1_is_present:1;
				unsigned char ctrl2_is_present:1;
				unsigned char ctrl3_is_present:1;
				unsigned char ctrl4_is_present:1;
				unsigned char ctrl5_is_present:1;
				unsigned char ctrl6_is_present:1;
				unsigned char ctrl7_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl8_is_present:1;
				unsigned char ctrl9_is_present:1;
				unsigned char ctrl10_is_present:1;
				unsigned char ctrl11_is_present:1;
				unsigned char ctrl12_is_present:1;
				unsigned char ctrl13_is_present:1;
				unsigned char ctrl14_is_present:1;
				unsigned char ctrl15_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl16_is_present:1;
				unsigned char ctrl17_is_present:1;
				unsigned char ctrl18_is_present:1;
				unsigned char ctrl19_is_present:1;
				unsigned char ctrl20_is_present:1;
				unsigned char ctrl21_is_present:1;
				unsigned char ctrl22_is_present:1;
				unsigned char ctrl23_is_present:1;
			} __packed;
		};
		unsigned char data[4];
	};
};

struct synaptics_rmi4_f12_query_8 {
	union {
		struct {
			unsigned char size_of_query9;
			struct {
				unsigned char data0_is_present:1;
				unsigned char data1_is_present:1;
				unsigned char data2_is_present:1;
				unsigned char data3_is_present:1;
				unsigned char data4_is_present:1;
				unsigned char data5_is_present:1;
				unsigned char data6_is_present:1;
				unsigned char data7_is_present:1;
			} __packed;
		};
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f12_query_10 {
	union {
		struct {
			unsigned char maximum_number_of_sensed_objects;
			unsigned char finger;
			unsigned char pen;
			unsigned char proximate_fingers;
			unsigned char gapless_touches;
		};
		unsigned char data[5];
	};
};

struct synaptics_rmi4_f12_ctrl_8 {
	union {
		struct {
			unsigned char max_x_coord_lsb;
			unsigned char max_x_coord_msb;
			unsigned char max_y_coord_lsb;
			unsigned char max_y_coord_msb;
		};
		unsigned char data[4];
	};
};

struct synaptics_rmi4_f12_ctrl_23 {
	union {
		struct {
			unsigned char obj_type_enable;
			unsigned char max_reported_objects;
		};
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f12_finger_data {
	union {
		struct {
			unsigned char object_type_and_status;
			unsigned char x_lsb;
			unsigned char x_msb;
			unsigned char y_lsb;
			unsigned char y_msb;
			unsigned char z;
			unsigned char wx;
			unsigned char wy;
		};
		unsigned char data[8];
	};
};

union synaptics_rmi4_f1a_query {
	struct {
		unsigned char max_button_count:3;
		unsigned char reserved:5;
		unsigned char has_general_control:1;
		unsigned char has_interrupt_enable:1;
		unsigned char has_multibutton_select:1;
		unsigned char has_tx_rx_map:1;
		unsigned char has_perbutton_threshold:1;
		unsigned char has_release_threshold:1;
		unsigned char has_strongestbtn_hysteresis:1;
		unsigned char has_filter_strength:1;
	};
	unsigned char regs[2];
};

union synaptics_rmi4_f1a_control_0 {
	struct {
		unsigned char multibutton_report:2;
		unsigned char filter_mode:2;
		unsigned char reserved:4;
	};
	unsigned char regs[1];
};

struct synaptics_rmi4_f1a_control_3_4 {
	unsigned char transmitterbutton;
	unsigned char receiverbutton;
};

struct synaptics_rmi4_f1a_control {
	union synaptics_rmi4_f1a_control_0 general_control;
	unsigned char *button_int_enable;
	unsigned char *multi_button;
	struct synaptics_rmi4_f1a_control_3_4 *electrode_map;
	unsigned char *button_threshold;
	unsigned char button_release_threshold;
	unsigned char strongest_button_hysteresis;
	unsigned char filter_strength;
};

struct synaptics_rmi4_f1a_handle {
	struct synaptics_rmi4_f1a_control button_control;
	union synaptics_rmi4_f1a_query button_query;
	unsigned char button_count;
	unsigned char valid_button_count;
	int button_bitmask_size;
	unsigned char *button_data_buffer;
	unsigned char *button_map;
};

#if PROXIMITY
struct synaptics_rmi4_f51_query {
	union {
		struct {
			unsigned char query_register_count;
			unsigned char data_register_count;
			unsigned char control_register_count;
			unsigned char command_register_count;
		};
		unsigned char data[4];
	};
};

struct synaptics_rmi4_f51_data {
	union {
		struct {
			unsigned char proximity_detection;
			unsigned char hover_finger_x_4__11;
			unsigned char hover_finger_y_4__11;
			unsigned char hover_finger_xy_0__3;
			unsigned char hover_finger_z;
			unsigned char hover_pinch;
			unsigned char large_obj_air_swipe;
		};
		unsigned char data[7];
	};
};
#endif

struct synaptics_rmi4_exp_fn {
	enum exp_fn fn_type;
	bool inserted;
	int (*func_init) (struct synaptics_rmi4_data *rmi4_data);
	void (*func_remove) (struct synaptics_rmi4_data *rmi4_data);
	void (*func_attn) (struct synaptics_rmi4_data *rmi4_data,
			   unsigned char intr_mask);
	struct list_head link;
};

static struct device_attribute attrs[] = {
#ifdef CONFIG_HAS_EARLYSUSPEND
	__ATTR(full_pm_cycle, (S_IRUGO | S_IWUGO),
	       synaptics_rmi4_full_pm_cycle_show,
	       synaptics_rmi4_full_pm_cycle_store),
#endif
	__ATTR(reset, S_IWUGO,
	       synaptics_rmi4_show_error,
	       synaptics_rmi4_f01_reset_store),
	__ATTR(productinfo, S_IRUGO,
	       synaptics_rmi4_f01_productinfo_show,
	       synaptics_rmi4_store_error),
	__ATTR(flashprog, S_IRUGO,
	       synaptics_rmi4_f01_flashprog_show,
	       synaptics_rmi4_store_error),
	__ATTR(0 dbutton, (S_IRUGO | S_IWUGO),
	       synaptics_rmi4_0dbutton_show,
	       synaptics_rmi4_0dbutton_store),
};

static bool exp_fn_inited;
static struct mutex synaptics_rmi4_exp_fn_list_mutex;
static struct list_head synaptics_rmi4_exp_fn_list;

#ifdef CONFIG_HAS_EARLYSUSPEND
static ssize_t synaptics_rmi4_full_pm_cycle_show(struct device *dev,
						 struct device_attribute *attr,
						 char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", rmi4_data->full_pm_cycle);
}

static ssize_t synaptics_rmi4_full_pm_cycle_store(struct device *dev,
						  struct device_attribute *attr,
						  const char *buf, size_t count)
{
	int input;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (sscanf(buf, "%d", &input) != 1)
		return -EINVAL;

	rmi4_data->full_pm_cycle = input > 0 ? 1 : 0;

	return count;
}
#endif

static ssize_t synaptics_rmi4_f01_reset_store(struct device *dev,
					      struct device_attribute *attr,
					      const char *buf, size_t count)
{
	int retval;
	int reset;
	unsigned char command;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (sscanf(buf, "%d", &reset) != 1)
		return -EINVAL;

	if ((reset < 0) || (reset > 1))
		return -EINVAL;

	if (reset) {
		command = 0x01;

		retval = synaptics_rmi4_i2c_write(rmi4_data,
						  rmi4_data->f01_cmd_base_addr,
						  &command, sizeof(command));
		if (retval < 0) {
			dev_err(dev,
				"%s Failed to issue reset command, error = %d\n",
				__func__, retval);
			return retval;
		}
	}

	return count;
}

static ssize_t synaptics_rmi4_f01_productinfo_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "0x%02x 0x%02x\n",
			(rmi4_data->rmi4_mod_info.product_info[0]),
			(rmi4_data->rmi4_mod_info.product_info[1]));
}

static ssize_t synaptics_rmi4_f01_flashprog_show(struct device *dev,
						 struct device_attribute *attr,
						 char *buf)
{
	int retval;
	struct synaptics_rmi4_f01_device_status device_status;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 rmi4_data->f01_data_base_addr,
					 device_status.data,
					 sizeof(device_status.data));
	if (retval < 0) {
		dev_err(dev, "%s Failed to read device status, error = %d\n",
			__func__, retval);
		return retval;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", device_status.flash_prog);
}

static ssize_t synaptics_rmi4_0dbutton_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", rmi4_data->button_0d_enabled);
}

static ssize_t synaptics_rmi4_0dbutton_store(struct device *dev,
					     struct device_attribute *attr,
					     const char *buf, size_t count)
{
	int retval;
	int input;
	unsigned char ii;
	unsigned char intr_enable;
	struct synaptics_rmi4_fn *function_handler;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (sscanf(buf, "%d", &input) != 1)
		return -EINVAL;

	input = input > 0 ? 1 : 0;

	if (rmi4_data->button_0d_enabled != input) {
		list_for_each_entry(function_handler,
				    &rmi4_data->rmi4_mod_info.support_fn_list,
				    link) {
			if (function_handler->fn_number ==
			    SYNAPTICS_RMI4_0D_BUTTON_FN) {
				ii = function_handler->index_to_intr_reg;

				retval = synaptics_rmi4_i2c_read(rmi4_data,
					rmi4_data->
					f01_ctrl_base_addr
					+ 1 + ii,
					&intr_enable,
					sizeof
					(intr_enable));
				if (retval < 0)
					return retval;

				if (input == 1)
					intr_enable |=
					    function_handler->intr_mask;
				else
					intr_enable &=
					    ~function_handler->intr_mask;

				retval = synaptics_rmi4_i2c_write(rmi4_data,
					rmi4_data->
					f01_ctrl_base_addr
					+ 1 + ii,
					&intr_enable,
					sizeof
					(intr_enable));
				if (retval < 0)
					return retval;
			}
		}
	}

	rmi4_data->button_0d_enabled = input;

	return count;
}

 /**
 * synaptics_rmi4_set_page()
 *
 * Called by synaptics_rmi4_i2c_read() and synaptics_rmi4_i2c_write().
 *
 * This function writes to the page select register to switch to the
 * assigned page.
 */
static int synaptics_rmi4_set_page(struct synaptics_rmi4_data *rmi4_data,
				   unsigned int address)
{
	int retval = 0;
	unsigned char retry;
	unsigned char buf[PAGE_SELECT_LEN];
	unsigned char page;
	struct i2c_client *i2c = rmi4_data->i2c_client;

	page = ((address >> 8) & MASK_8BIT);
	if (page != rmi4_data->current_page) {
		buf[0] = MASK_8BIT;
		buf[1] = page;
		for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
			retval = i2c_master_send(i2c, buf, PAGE_SELECT_LEN);
			if (retval != PAGE_SELECT_LEN) {
				dev_err(&i2c->dev,
					"%s I2C retry %d\n", __func__,
					retry + 1);
				usleep_range(1000, 1000);
			} else {
				rmi4_data->current_page = page;
				break;
			}
		}
	} else {
		retval = PAGE_SELECT_LEN;
	}

	return retval;
}

 /**
 * synaptics_rmi4_i2c_read()
 *
 * Called by various functions in this driver, and also exported to other
 * expansion Function modules such as rmi_dev.
 *
 * This function reads data of an arbitrary length from the sensor, starting
 * from an assigned register address of the sensor, via I2C with a retry
 * mechanism.
 */
static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
				   unsigned short addr, unsigned char *data,
				   unsigned short length)
{
	int retval;
	unsigned char retry;
	unsigned char buf;
	struct i2c_msg msg[] = {
		{
		 .addr = rmi4_data->i2c_client->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = &buf,
		 },
		{
		 .addr = rmi4_data->i2c_client->addr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = data,
		 },
	};

	buf = addr & MASK_8BIT;

	mutex_lock(&(rmi4_data->rmi4_io_ctrl_mutex));

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN)
		goto exit;

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(rmi4_data->i2c_client->adapter, msg, 2) == 2) {
			retval = length;
			break;
		}
		dev_err(&rmi4_data->i2c_client->dev, "%s I2C retry %d\n",
			__func__, retry + 1);
		usleep_range(1000, 1000);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s I2C read over retry limit\n", __func__);
		retval = -EIO;
	}

 exit:
	mutex_unlock(&(rmi4_data->rmi4_io_ctrl_mutex));

	return retval;
}

 /**
 * synaptics_rmi4_i2c_write()
 *
 * Called by various functions in this driver, and also exported to other
 * expansion Function modules such as rmi_dev.
 *
 * This function writes data of an arbitrary length to the sensor, starting
 * from an assigned register address of the sensor, via I2C with a retry
 * mechanism.
 */
static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
				    unsigned short addr, unsigned char *data,
				    unsigned short length)
{
	int retval;
	unsigned char retry;
	unsigned char buf[length + 1];
	struct i2c_msg msg[] = {
		{
		 .addr = rmi4_data->i2c_client->addr,
		 .flags = 0,
		 .len = length + 1,
		 .buf = buf,
		 }
	};

	mutex_lock(&(rmi4_data->rmi4_io_ctrl_mutex));

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN)
		goto exit;

	buf[0] = addr & MASK_8BIT;
	memcpy(&buf[1], &data[0], length);

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(rmi4_data->i2c_client->adapter, msg, 1) == 1) {
			retval = length;
			break;
		}
		dev_err(&rmi4_data->i2c_client->dev,
			"%s I2C retry %d\n", __func__, retry + 1);
		mdelay(10);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s I2C read over retry limit\n", __func__);
		retval = -EIO;
	}

 exit:
	mutex_unlock(&(rmi4_data->rmi4_io_ctrl_mutex));

	return retval;
}

 /**
 * synaptics_rmi4_f11_abs_report()
 *
 * Called by synaptics_rmi4_report_touch() when valid Function $11
 * finger data has been detected.
 *
 * This function reads the Function $11 data registers, determines the
 * status of each finger supported by the Function, processes any
 * necessary coordinate manipulation, reports the finger data to
 * the input subsystem, and returns the number of fingers detected.
 */
static int synaptics_rmi4_f11_abs_report(
			struct synaptics_rmi4_data *rmi4_data,
			struct synaptics_rmi4_fn *function_handler)
{
	int retval;
	unsigned char touch_count = 0;	/* number of touch points */
	unsigned char reg_index;
	unsigned char finger;
	unsigned char fingers_supported;
	unsigned char num_of_finger_status_regs;
	unsigned char finger_shift;
	unsigned char finger_status;
	unsigned char data_reg_blk_size;
	unsigned char finger_status_reg[3];
	unsigned char data[F11_STD_DATA_LEN];
	unsigned short data_base_addr;
	unsigned short data_offset;
	int x[MAX_NUMBER_OF_FINGERS];
	int y[MAX_NUMBER_OF_FINGERS];
	int wx[MAX_NUMBER_OF_FINGERS];
	int wy[MAX_NUMBER_OF_FINGERS];

	/*
	 * The number of finger status registers is determined by the maximum
	 * number of fingers supported - 2 bits per finger. So the number of
	 * finger status registers to read is:
	 * register_count = ceil(max_num_of_fingers / 4)
	 */
	fingers_supported = function_handler->num_of_data_points;
	num_of_finger_status_regs = (fingers_supported + 3) / 4;
	data_base_addr = function_handler->full_addr.data_base;
	data_reg_blk_size = function_handler->size_of_data_register_block;

	retval = synaptics_rmi4_i2c_read(rmi4_data, data_base_addr,
					 finger_status_reg,
					 num_of_finger_status_regs);
	if (retval < 0)
		return 0;

	for (finger = 0; finger < fingers_supported; finger++) {
		reg_index = finger / 4;

		/* bit shift to get finger status */
		finger_shift = (finger % 4) * 2;
		finger_status = (finger_status_reg[reg_index] >> finger_shift)
		    & MASK_2BIT;

		/*
		 * Each 2-bit finger status field represents the following:
		 * 00 = finger not present
		 * 01 = finger present and data accurate
		 * 10 = finger present but data may be inaccurate
		 * 11 = reserved
		 */
		if ((finger_status == 0x1) || (finger_status == 0x02)) {
			data_offset =
			    data_base_addr + num_of_finger_status_regs +
			    (finger * data_reg_blk_size);
			retval =
			    synaptics_rmi4_i2c_read(rmi4_data, data_offset,
						    data, data_reg_blk_size);
			if (retval < 0)
				return 0;
			else {
				x[touch_count] = (data[0] << 4) |
				    (data[2] & MASK_4BIT);
				y[touch_count] = (data[1] << 4) |
				    ((data[2] >> 4) & MASK_4BIT);
				wx[touch_count] = (data[3] & MASK_4BIT);
				wy[touch_count] = (data[3] >> 4) & MASK_4BIT;

				if (rmi4_data->board->x_flip)
					x[touch_count] =
					    rmi4_data->sensor_max_x -
					    x[touch_count];
				if (rmi4_data->board->y_flip)
					y[touch_count] =
					    rmi4_data->sensor_max_y -
					    y[touch_count];

				dev_info(&rmi4_data->i2c_client->dev,
					 "%s Finger %d: "
					 "status = 0x%02x "
					 "x = %d y = %d "
					 "wx = %d wy = %d\n",
					 __func__, finger,
					 finger_status, x[touch_count],
					 y[touch_count], wx[touch_count],
					 wy[touch_count]);
			}
			touch_count++;
		}
	}

	if (touch_count) {
		for (finger = 0; finger < touch_count; finger++) {
			input_report_abs(rmi4_data->input_dev,
					 ABS_MT_POSITION_X, x[finger]);
			input_report_abs(rmi4_data->input_dev,
					 ABS_MT_POSITION_Y, y[finger]);
			input_report_abs(rmi4_data->input_dev,
					 ABS_MT_TOUCH_MAJOR, max(wx[finger],
								 wy[finger]));
			input_report_abs(rmi4_data->input_dev,
					 ABS_MT_TOUCH_MINOR, min(wx[finger],
								 wy[finger]));
			input_mt_sync(rmi4_data->input_dev);
		}
	} else {
		input_mt_sync(rmi4_data->input_dev);
	}

	input_sync(rmi4_data->input_dev);

	return touch_count;
}

 /**
 * synaptics_rmi4_f12_abs_report()
 *
 * Called by synaptics_rmi4_report_touch() when valid Function $12
 * finger data has been detected.
 *
 * This function reads the Function $12 data registers, determines the
 * status of each finger supported by the Function, processes any
 * necessary coordinate manipulation, reports the finger data to
 * the input subsystem, and returns the number of fingers detected.
 */
static int synaptics_rmi4_f12_abs_report(
			struct synaptics_rmi4_data *rmi4_data,
			struct synaptics_rmi4_fn *function_handler)
{
	int retval;
	unsigned char touch_count = 0;	/* number of touch points */
	unsigned char finger;
	unsigned char fingers_supported;
	unsigned char finger_status;
	unsigned short data_base_addr;
	int x;
	int y;
	int wx;
	int wy;
	struct synaptics_rmi4_f12_finger_data *data_base;
	struct synaptics_rmi4_f12_finger_data *finger_data;

	fingers_supported = function_handler->num_of_data_points;
	data_base_addr = function_handler->full_addr.data_base;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 data_base_addr +
					 function_handler->data1_offset,
					 (unsigned char *)
					 function_handler->data,
					 function_handler->data_size);
	if (retval < 0)
		return 0;

	data_base =
	    (struct synaptics_rmi4_f12_finger_data *)function_handler->data;

	for (finger = 0; finger < fingers_supported; finger++) {
		finger_data = data_base + finger;
		finger_status = finger_data->object_type_and_status & MASK_2BIT;

		/*
		 * Each 2-bit finger status field represents the following:
		 * 00 = finger not present
		 * 01 = finger present and data accurate
		 * 10 = finger present but data may be inaccurate
		 * 11 = reserved
		 */
		if ((finger_status == 0x01) || (finger_status == 0x02)) {
			x = (finger_data->x_msb << 8) | (finger_data->x_lsb);
			y = (finger_data->y_msb << 8) | (finger_data->y_lsb);
			wx = finger_data->wx;
			wy = finger_data->wy;

			if (rmi4_data->board->x_flip)
				x = rmi4_data->sensor_max_x - x;
			if (rmi4_data->board->y_flip)
				y = rmi4_data->sensor_max_y - y;

			dev_info(&rmi4_data->i2c_client->dev, "%s Finger %d: "
				 "status = 0x%02x x = %d y = %d "
				 "wx = %d wy = %d\n", __func__, finger,
				 finger_status, x, y, wx, wy);

			input_report_abs(rmi4_data->input_dev,
					 ABS_MT_POSITION_X, x);
			input_report_abs(rmi4_data->input_dev,
					 ABS_MT_POSITION_Y, y);
			input_report_abs(rmi4_data->input_dev,
					 ABS_MT_TOUCH_MAJOR, max(wx, wy));
			input_report_abs(rmi4_data->input_dev,
					 ABS_MT_TOUCH_MINOR, min(wx, wy));
			input_report_key(rmi4_data->input_dev, BTN_TOUCH, 1);
			input_mt_sync(rmi4_data->input_dev);

			touch_count++;
		}
	}

	if (!touch_count) {
		input_report_key(rmi4_data->input_dev, BTN_TOUCH, 0);
		input_mt_sync(rmi4_data->input_dev);
	}

	input_sync(rmi4_data->input_dev);

	return touch_count;
}

static void synaptics_rmi4_0d_report(struct synaptics_rmi4_data *rmi4_data,
				     struct synaptics_rmi4_fn *function_handler)
{
	int retval;
	unsigned char button;
	unsigned char button_reg;
	unsigned char button_shift;
	unsigned char button_status;
	unsigned char *data_base;
	unsigned short data_base_addr = function_handler->full_addr.data_base;
	struct synaptics_rmi4_f1a_handle *f1a = function_handler->data;
	static unsigned char do_once = 1;
	static bool current_button_status[MAX_NUMBER_OF_BUTTONS];
#if NO_0D_WHILE_2D
	static bool before_2d_button_status[MAX_NUMBER_OF_BUTTONS];
	static bool while_2d_button_status[MAX_NUMBER_OF_BUTTONS];
#endif

	if (do_once) {
		memset(current_button_status, 0, sizeof(current_button_status));
#if NO_0D_WHILE_2D
		memset(before_2d_button_status, 0,
		       sizeof(before_2d_button_status));
		memset(while_2d_button_status, 0,
		       sizeof(while_2d_button_status));
#endif
		do_once = 0;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data, data_base_addr,
					 f1a->button_data_buffer,
					 f1a->button_bitmask_size);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Failed to read button data "
			"registers\n", __func__);
		return;
	}

	data_base = f1a->button_data_buffer;

	for (button = 0; button < f1a->valid_button_count; button++) {
		button_reg = button / 8;
		button_shift = button % 8;
		button_status =
		    ((data_base[button_reg] >> button_shift) & MASK_1BIT);

		if (current_button_status[button] == button_status)
			continue;
		else
			current_button_status[button] = button_status;

		dev_info(&rmi4_data->i2c_client->dev,
			 "%s Button %d (code %d) -> %d\n",
			 __func__, button, f1a->button_map[button],
			 button_status);

#if NO_0D_WHILE_2D
		if (rmi4_data->fingers_on_2d == false) {
			if (button_status == 1)
				before_2d_button_status[button] = 1;
			else {
				if (while_2d_button_status[button] == 1) {
					while_2d_button_status[button] = 0;
					continue;
				} else {
					before_2d_button_status[button] = 0;
				}
			}
			input_report_key(rmi4_data->input_dev,
					 f1a->button_map[button],
					 button_status);
		} else {
			if (before_2d_button_status[button] == 1) {
				before_2d_button_status[button] = 0;
				input_report_key(rmi4_data->input_dev,
						 f1a->button_map[button],
						 button_status);
			} else {
				if (button_status == 1)
					while_2d_button_status[button] = 1;
				else
					while_2d_button_status[button] = 0;
			}
		}
#else
		input_report_key(rmi4_data->input_dev, f1a->button_map[button],
				 button_status);
#endif
	}

	input_sync(rmi4_data->input_dev);

	return;
}

#if PROXIMITY
static int synaptics_rmi4_proximity_report(struct synaptics_rmi4_data
					   *rmi4_data, struct synaptics_rmi4_fn
					   *function_handler)
{
	int retval;
	unsigned char touch_count = 0;	/* number of touch points */
	unsigned short data_base_addr;
	int x;
	int y;
	int z;
	struct synaptics_rmi4_f51_data *data_register;

	data_base_addr = function_handler->full_addr.data_base;
	data_register =
	    (struct synaptics_rmi4_f51_data *)function_handler->data;

	retval = synaptics_rmi4_i2c_read(rmi4_data, data_base_addr,
					 (unsigned char *)data_register,
					 sizeof(*data_register));
	if (retval < 0)
		return retval;

	if (data_register->proximity_detection & PROXIMITY_FINGER_HOVER) {
		if (data_register->hover_finger_z > 0) {
			x = (data_register->hover_finger_x_4__11 << 4) |
			    (data_register->hover_finger_xy_0__3 & 0x0f);
			y = (data_register->hover_finger_y_4__11 << 4) |
			    (data_register->hover_finger_xy_0__3 >> 4);
			z = HOVER_Z_MAX - data_register->hover_finger_z;

			if (0 == z) {
				dev_dbg(&rmi4_data->i2c_client->dev,
				"%s Hover finger 1 release\n", __func__);

				input_report_abs(rmi4_data->input_dev,
						ABS_MT_POSITION_X, x);
				input_report_abs(rmi4_data->input_dev,
						ABS_MT_POSITION_Y, y);
				input_report_abs(rmi4_data->input_dev,
						ABS_MT_DISTANCE, z);
				input_report_key(rmi4_data->input_dev,
						BTN_TOOL_FINGER, 0);
				input_mt_sync(rmi4_data->input_dev);

			} else {
				dev_dbg(&rmi4_data->i2c_client->dev,
				"%s Hover finger 1:\n" "x = %d\n" "y = %d\n"
				"z = %d\n", __func__, x, y, z);

				input_report_abs(rmi4_data->input_dev,
						ABS_MT_POSITION_X, x);
				input_report_abs(rmi4_data->input_dev,
						ABS_MT_POSITION_Y, y);
				input_report_abs(rmi4_data->input_dev,
						ABS_MT_DISTANCE, z);
				input_report_key(rmi4_data->input_dev,
						BTN_TOOL_FINGER, 1);
				input_mt_sync(rmi4_data->input_dev);
			}
			touch_count++;
		}
	}

	if (data_register->proximity_detection & PROXIMITY_AIR_SWIPE) {
		dev_dbg(&rmi4_data->i2c_client->dev,
			"%s Swipe direction = 0x%01x\n", __func__,
			data_register->large_obj_air_swipe & 0x03);
	}

	if (data_register->proximity_detection & PROXIMITY_LARGE_OBJ) {
		dev_dbg(&rmi4_data->i2c_client->dev,
			"%s Large object activity = " "0x%01x\n", __func__,
			data_register->large_obj_air_swipe >> 6);
	}

	if (data_register->proximity_detection & PROXIMITY_HOVER_PINCH) {
		dev_dbg(&rmi4_data->i2c_client->dev,
			"%s Hover pinch direction =" "0x%01x\n", __func__,
			data_register->hover_pinch >> 7);
		dev_dbg(&rmi4_data->i2c_client->dev,
			"%s Hover pinch motion =" "0x%02x\n", __func__,
			data_register->hover_pinch & 0x7f);
	}

	if (!touch_count)
		input_mt_sync(rmi4_data->input_dev);

	input_sync(rmi4_data->input_dev);

	return touch_count;
}
#endif

 /**
 * synaptics_rmi4_report_touch()
 *
 * Called by synaptics_rmi4_sensor_report().
 *
 * This function calls the appropriate finger data reporting function
 * based on the function handler it receives and returns the number of
 * fingers detected.
 */
static void synaptics_rmi4_report_touch(
				struct synaptics_rmi4_data *rmi4_data,
				struct synaptics_rmi4_fn *function_handler,
				unsigned char *touch_count)
{
	unsigned char touch_count_2d;

	switch (function_handler->fn_number) {
	case SYNAPTICS_RMI4_F11_2D_SENSOR_FN:
		touch_count_2d =
			synaptics_rmi4_f11_abs_report(rmi4_data,
						function_handler);

		*touch_count += touch_count_2d;

		if (touch_count_2d)
			rmi4_data->fingers_on_2d = true;
		else
			rmi4_data->fingers_on_2d = false;
		break;
	case SYNAPTICS_RMI4_F12_2D_SENSOR_FN:
		touch_count_2d =
			synaptics_rmi4_f12_abs_report(rmi4_data,
						function_handler);

		*touch_count += touch_count_2d;

		if (touch_count_2d)
			rmi4_data->fingers_on_2d = true;
		else
			rmi4_data->fingers_on_2d = false;
		break;
	case SYNAPTICS_RMI4_0D_BUTTON_FN:
		synaptics_rmi4_0d_report(rmi4_data, function_handler);
		break;
#if PROXIMITY
	case SYNAPTICS_RMI4_PROXIMITY_FN:
		*touch_count +=
			synaptics_rmi4_proximity_report(rmi4_data,
						function_handler);
		break;
#endif
	default:
		break;
	}

	return;
}

 /**
 * synaptics_rmi4_sensor_report()
 *
 * Called by synaptics_rmi4_irq().
 *
 * This function determines the interrupt source(s) from the sensor and
 * calls synaptics_rmi4_report_touch() with the appropriate function
 * handler for each function with valid data inputs.
 */
static int synaptics_rmi4_sensor_report(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char touch_count = 0;
	unsigned char intr_status[MAX_INTR_REGISTERS];
	struct synaptics_rmi4_fn *function_handler;
	struct synaptics_rmi4_exp_fn *exp_fn_handler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	/*
	 * Get interrupt status information from F01 Data1 register to
	 * determine the source(s) that are flagging the interrupt.
	 */
	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 rmi4_data->f01_data_base_addr + 1,
					 intr_status,
					 rmi4_data->
					 number_of_interrupt_registers);
	if (retval < 0)
		return retval;

	/*
	 * Traverse the function handler list and service the source(s) of
	 * the interrupt accordingly.
	 */
	list_for_each_entry(function_handler, &rmi->support_fn_list, link) {
		if (function_handler->num_of_data_sources) {
			if (function_handler->intr_mask &
			    intr_status[function_handler->index_to_intr_reg]) {
				synaptics_rmi4_report_touch(rmi4_data,
							    function_handler,
							    &touch_count);
			}
		}
	}

	mutex_lock(&synaptics_rmi4_exp_fn_list_mutex);
	if (!list_empty(&synaptics_rmi4_exp_fn_list)) {
		list_for_each_entry(exp_fn_handler,
				    &synaptics_rmi4_exp_fn_list, link) {
			if (exp_fn_handler &&
			    (exp_fn_handler->func_attn != NULL))
				exp_fn_handler->func_attn(rmi4_data,
							  intr_status[0]);
		}
	}
	mutex_unlock(&synaptics_rmi4_exp_fn_list_mutex);

	return touch_count;
}

 /**
 * synaptics_rmi4_irq()
 *
 * Called by the kernel when an interrupt occurs (when the sensor asserts
 * the attention irq.
 *
 * This function is the ISR thread and handles the acquisition and
 * the reporting of finger data when the presence of fingers is detected.
 */
static irqreturn_t synaptics_rmi4_irq(int irq, void *data)
{
	unsigned char touch_count;
	struct synaptics_rmi4_data *rmi4_data = data;

	do {
		touch_count = synaptics_rmi4_sensor_report(rmi4_data);

		if (touch_count > 0) {
			wait_event_timeout(rmi4_data->wait,
					   rmi4_data->touch_stopped,
					   msecs_to_jiffies
					   (SENSOR_POLLING_PERIOD));
		} else {
			break;
		}
	} while (!rmi4_data->touch_stopped);

	return IRQ_HANDLED;
}

 /**
 * synaptics_rmi4_irq_enable()
 *
 * Called by synaptics_rmi4_probe() and the power management functions in
 * this driver and also exported to other expansion Function modules such
 * as rmi_dev.
 *
 * This function handles the enabling and disabling of the attention irq
 * including the setting up of the ISR thread.
 */
static int synaptics_rmi4_irq_enable(struct synaptics_rmi4_data *rmi4_data,
				     bool enable)
{
	int retval = 0;
	unsigned char intr_status;
	const struct synaptics_rmi4_platform_data *platform_data =
	    rmi4_data->i2c_client->dev.platform_data;

	if (enable) {
		if (rmi4_data->irq_enabled)
			return retval;

		/* Clear interrupts first */
		retval = synaptics_rmi4_i2c_read(rmi4_data,
						 rmi4_data->f01_data_base_addr +
						 1, &intr_status,
						 rmi4_data->
						 number_of_interrupt_registers);
		if (retval < 0)
			return retval;

		retval = request_threaded_irq(rmi4_data->irq,
					      NULL, synaptics_rmi4_irq,
					      platform_data->irq_type,
					      DRIVER_NAME, rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
				"%s Failed to create irq "
				"thread\n", __func__);
			return retval;
		}

		rmi4_data->irq_enabled = true;
	} else {
		if (rmi4_data->irq_enabled) {
			disable_irq(rmi4_data->irq);
			free_irq(rmi4_data->irq, rmi4_data);
			rmi4_data->irq_enabled = false;
		}
	}

	return retval;
}

 /**
 * synaptics_rmi4_f11_2d_init()
 *
 * Called by synaptics_rmi4_query_device().
 *
 * This funtion parses information
 * from the Function 11 registers and determines
 * the number of fingers supported, x and y data ranges, offset to the
 * associated interrupt status register, interrupt bit mask, and gathers finger
 * data acquisition capabilities from the query registers.
 */
static int synaptics_rmi4_f11_2d_init(
		struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *function_handler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned char intr_offset;
	unsigned char abs_data_size;
	unsigned char abs_data_blk_size;
	unsigned char query[F11_STD_QUERY_LEN];
	unsigned char control[F11_STD_CTRL_LEN];

	function_handler->fn_number = fd->fn_number;
	function_handler->num_of_data_sources = fd->intr_src_count;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 function_handler->full_addr.query_base,
					 query, sizeof(query));
	if (retval < 0)
		return retval;

	/* Maximum number of fingers supported */
	if ((query[1] & MASK_3BIT) <= 4)
		function_handler->num_of_data_points =
		    (query[1] & MASK_3BIT) + 1;
	else {
		if ((query[1] & MASK_3BIT) == 5)
			function_handler->num_of_data_points = 10;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 function_handler->full_addr.ctrl_base,
					 control, sizeof(control));
	if (retval < 0)
		return retval;

	/* Maximum x and y */
	rmi4_data->sensor_max_x = ((control[6] & MASK_8BIT) << 0) |
	    ((control[7] & MASK_4BIT) << 8);
	rmi4_data->sensor_max_y = ((control[8] & MASK_8BIT) << 0) |
	    ((control[9] & MASK_4BIT) << 8);
	dev_info(&rmi4_data->i2c_client->dev,
		 "%s Function %02x max x = %d max y = "
		 "%d\n", __func__, function_handler->fn_number,
		 rmi4_data->sensor_max_x, rmi4_data->sensor_max_y);

	function_handler->index_to_intr_reg = (intr_count + 7) / 8;
	if (function_handler->index_to_intr_reg != 0)
		function_handler->index_to_intr_reg -= 1;

	/* Set an enable bit in the interrupt enble mask for each data source */
	intr_offset = intr_count % 8;
	function_handler->intr_mask = 0;
	for (ii = intr_offset;
	     ii < ((fd->intr_src_count & MASK_3BIT) + intr_offset); ii++)
		function_handler->intr_mask |= 1 << ii;

	abs_data_size = query[5] & MASK_2BIT;
	abs_data_blk_size = 3 + (2 * (abs_data_size == 0 ? 1 : 0));
	function_handler->size_of_data_register_block = abs_data_blk_size;

	return retval;
}

 /**
 * synaptics_rmi4_f12_2d_init()
 *
 * Called by synaptics_rmi4_query_device().
 *
 * This funtion parses information from the Function 12 registers and
 * determines the number of fingers supported, offset to the data1 register,
 * x and y data ranges, offset to the associated interrupt status register,
 * interrupt bit mask, and allocates memory resources for finger data
 * acquisition.
 */
static int synaptics_rmi4_f12_2d_init(
		struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *function_handler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned char intr_offset;
	unsigned char ctrl_8_offset;
	unsigned char ctrl_23_offset;
	struct synaptics_rmi4_f12_query_5 query_5;
	struct synaptics_rmi4_f12_query_8 query_8;
	struct synaptics_rmi4_f12_ctrl_8 ctrl_8;
	struct synaptics_rmi4_f12_ctrl_23 ctrl_23;
	struct synaptics_rmi4_f12_finger_data *finger_data_list;

	function_handler->fn_number = fd->fn_number;
	function_handler->num_of_data_sources = fd->intr_src_count;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 function_handler->full_addr.
					 query_base + 5, query_5.data,
					 sizeof(query_5.data));
	if (retval < 0)
		return retval;

	ctrl_8_offset = query_5.ctrl0_is_present +
	    query_5.ctrl1_is_present +
	    query_5.ctrl2_is_present +
	    query_5.ctrl3_is_present +
	    query_5.ctrl4_is_present +
	    query_5.ctrl5_is_present +
	    query_5.ctrl6_is_present + query_5.ctrl7_is_present;

	ctrl_23_offset = ctrl_8_offset +
	    query_5.ctrl8_is_present +
	    query_5.ctrl9_is_present +
	    query_5.ctrl10_is_present +
	    query_5.ctrl11_is_present +
	    query_5.ctrl12_is_present +
	    query_5.ctrl13_is_present +
	    query_5.ctrl14_is_present +
	    query_5.ctrl15_is_present +
	    query_5.ctrl16_is_present +
	    query_5.ctrl17_is_present +
	    query_5.ctrl18_is_present +
	    query_5.ctrl19_is_present +
	    query_5.ctrl20_is_present +
	    query_5.ctrl21_is_present + query_5.ctrl22_is_present;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 function_handler->full_addr.ctrl_base +
					 ctrl_23_offset, ctrl_23.data,
					 sizeof(ctrl_23.data));
	if (retval < 0)
		return retval;

	/* Maximum number of fingers supported */
	function_handler->num_of_data_points = ctrl_23.max_reported_objects;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 function_handler->full_addr.
					 query_base + 8, query_8.data,
					 sizeof(query_8.data));
	if (retval < 0)
		return retval;

	/* Determine the presence of the Data0 register */
	function_handler->data1_offset = query_8.data0_is_present;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 function_handler->full_addr.ctrl_base +
					 ctrl_8_offset, ctrl_8.data,
					 sizeof(ctrl_8.data));
	if (retval < 0)
		return retval;

	/* Maximum x and y */
	rmi4_data->sensor_max_x =
	    ((unsigned short)ctrl_8.max_x_coord_lsb << 0) |
	    ((unsigned short)ctrl_8.max_x_coord_msb << 8);
	rmi4_data->sensor_max_y =
	    ((unsigned short)ctrl_8.max_y_coord_lsb << 0) |
	    ((unsigned short)ctrl_8.max_y_coord_msb << 8);
	dev_info(&rmi4_data->i2c_client->dev,
		 "%s Function %02x max x = %d max y = "
		 "%d\n", __func__, function_handler->fn_number,
		 rmi4_data->sensor_max_x, rmi4_data->sensor_max_y);

	function_handler->index_to_intr_reg = (intr_count + 7) / 8;
	if (function_handler->index_to_intr_reg != 0)
		function_handler->index_to_intr_reg -= 1;

	/* Set an enable bit in the interrupt enble mask for each data source */
	intr_offset = intr_count % 8;
	function_handler->intr_mask = 0;
	for (ii = intr_offset;
	     ii < ((fd->intr_src_count & MASK_3BIT) + intr_offset); ii++)
		function_handler->intr_mask |= 1 << ii;

	/* Allocate memory for finger data storage space */
	function_handler->data_size = function_handler->num_of_data_points *
	    sizeof(struct synaptics_rmi4_f12_finger_data);
	finger_data_list = kmalloc(function_handler->data_size, GFP_KERNEL);
	function_handler->data = (void *)finger_data_list;

	return retval;
}

static int synaptics_rmi4_0d_alloc_memory(
		struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *function_handler)
{
	int retval;
	struct synaptics_rmi4_f1a_handle *f1a;

	f1a = kzalloc(sizeof(*f1a), GFP_KERNEL);
	if (!f1a) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Failed to allocate memory "
			"for function handle\n", __func__);
		return -ENOMEM;
	}

	function_handler->data = (void *)f1a;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 function_handler->full_addr.query_base,
					 (unsigned char *)&f1a->button_query,
					 sizeof(union
						synaptics_rmi4_f1a_query));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Failed to read query " "registers\n", __func__);
		return retval;
	}

	f1a->button_count = f1a->button_query.max_button_count + 1;
	f1a->button_bitmask_size = (f1a->button_count + 7) / 8;

	f1a->button_data_buffer = kcalloc(f1a->button_bitmask_size,
					  sizeof(*(f1a->button_data_buffer)),
					  GFP_KERNEL);
	if (!f1a->button_data_buffer) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Failed to allocate memory "
			"for button data buffer\n", __func__);
		return -ENOMEM;
	}

	f1a->button_map =
	    kcalloc(f1a->button_count, sizeof(*(f1a->button_map)), GFP_KERNEL);
	if (!f1a->button_map) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Failed to allocate memory "
			"for button map\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

#if NO_0D_WHILE_2D
static int synaptics_rmi4_0d_button_map(
		struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *function_handler)
{
	unsigned char ii;
	struct synaptics_rmi4_f1a_handle *f1a = function_handler->data;

	if (!rmi4_data->board->f1a_button_map) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s f1a_button_map is NULL in "
			"board file\n", __func__);
		return -ENODEV;
	} else if (!rmi4_data->board->f1a_button_map->map) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Button map is missing in "
			"board file\n", __func__);
		return -ENODEV;
	} else {
		if (rmi4_data->board->f1a_button_map->nbuttons !=
		    f1a->button_count) {
			f1a->valid_button_count =
			    min(f1a->button_count,
				rmi4_data->board->f1a_button_map->nbuttons);
		} else {
			f1a->valid_button_count = f1a->button_count;
		}

		for (ii = 0; ii < f1a->valid_button_count; ii++)
			f1a->button_map[ii] =
			    rmi4_data->board->f1a_button_map->map[ii];
	}

	return 0;
}
#endif

static void synaptics_rmi4_0d_kfree(struct synaptics_rmi4_fn *function_handler)
{
	struct synaptics_rmi4_f1a_handle *f1a = function_handler->data;

	if (f1a) {
		kfree(f1a->button_data_buffer);
		kfree(f1a->button_map);
		kfree(f1a);
		function_handler->data = NULL;
	}

	return;
}

static int synaptics_rmi4_0d_button_init(
				struct synaptics_rmi4_data *rmi4_data,
				struct synaptics_rmi4_fn *function_handler,
				struct synaptics_rmi4_fn_desc *fd,
				unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned short intr_offset;

	function_handler->fn_number = fd->fn_number;
	function_handler->num_of_data_sources = fd->intr_src_count;

	function_handler->index_to_intr_reg = (intr_count + 7) / 8;
	if (function_handler->index_to_intr_reg != 0)
		function_handler->index_to_intr_reg -= 1;

	/* Set an enable bit in the interrupt enble mask for each data source */
	intr_offset = intr_count % 8;
	function_handler->intr_mask = 0;
	for (ii = intr_offset;
	     ii < ((fd->intr_src_count & MASK_3BIT) + intr_offset); ii++)
		function_handler->intr_mask |= 1 << ii;

	retval = synaptics_rmi4_0d_alloc_memory(rmi4_data, function_handler);
	if (retval < 0)
		goto error_exit;

#if NO_0D_WHILE_2D
	retval = synaptics_rmi4_0d_button_map(rmi4_data, function_handler);
	if (retval < 0)
		goto error_exit;
#endif
	rmi4_data->button_0d_enabled = 1;

	return 0;

 error_exit:
	synaptics_rmi4_0d_kfree(function_handler);

	return retval;
}

#if PROXIMITY
static int synaptics_rmi4_proximity_init(
				struct synaptics_rmi4_data *rmi4_data,
				struct synaptics_rmi4_fn *function_handler,
				struct synaptics_rmi4_fn_desc *fd,
				unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned short intr_offset;
	unsigned char proximity_enable_mask = PROXIMITY_ENABLE;
	struct synaptics_rmi4_f51_query query_register;
	struct synaptics_rmi4_f51_data *data_register;

	function_handler->fn_number = fd->fn_number;
	function_handler->num_of_data_sources = fd->intr_src_count;

	function_handler->index_to_intr_reg = (intr_count + 7) / 8;
	if (function_handler->index_to_intr_reg != 0)
		function_handler->index_to_intr_reg -= 1;

	/* Set an enable bit in the interrupt enble mask for each data source */
	intr_offset = intr_count % 8;
	function_handler->intr_mask = 0;
	for (ii = intr_offset;
	     ii < ((fd->intr_src_count & MASK_3BIT) + intr_offset); ii++)
		function_handler->intr_mask |= 1 << ii;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 function_handler->full_addr.query_base,
					 (unsigned char *)&query_register,
					 sizeof(query_register));
	if (retval < 0)
		return retval;

	function_handler->data_size = sizeof(data_register->data);
	data_register = kmalloc(function_handler->data_size, GFP_KERNEL);
	function_handler->data = (void *)data_register;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
					  function_handler->full_addr.
					  ctrl_base +
					  query_register.
					  control_register_count - 1,
					  &proximity_enable_mask,
					  sizeof(proximity_enable_mask));
	if (retval < 0)
		return retval;

	return 0;
}
#endif

static int synaptics_rmi4_alloc_fh(struct synaptics_rmi4_fn **function_handler,
				   struct synaptics_rmi4_fn_desc *rmi_fd,
				   int page_number)
{
	*function_handler = kmalloc(sizeof(**function_handler), GFP_KERNEL);
	if (!(*function_handler))
		return -ENOMEM;

	(*function_handler)->full_addr.data_base =
	    (rmi_fd->data_base_addr | (page_number << 8));
	(*function_handler)->full_addr.ctrl_base =
	    (rmi_fd->ctrl_base_addr | (page_number << 8));
	(*function_handler)->full_addr.cmd_base =
	    (rmi_fd->cmd_base_addr | (page_number << 8));
	(*function_handler)->full_addr.query_base =
	    (rmi_fd->query_base_addr | (page_number << 8));

	return 0;
}

 /**
 * synaptics_rmi4_query_device()
 *
 * Called by synaptics_rmi4_probe().
 *
 * This funtion scans the page description table, records the offsets to the
 * register types of Function $01, sets up the function handlers for Function
 * $11 and Function $12, determines the number of interrupt sources from the
 * sensor, adds valid Functions with data inputs to the Function linked list,
 * parses information from the query registers of Function $01, and enables
 * the interrupt sources from the valid Functions with data inputs.
 */
static int synaptics_rmi4_query_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii;
	unsigned char page_number;
	unsigned char intr_count = 0;
	unsigned char data_sources = 0;
	unsigned char f01_query[F01_STD_QUERY_LEN];
	unsigned char interrupt_mask[MAX_INTR_REGISTERS];
	unsigned short pdt_entry_addr;
	unsigned short f01_ctrl_offset;
	struct synaptics_rmi4_f01_device_status device_status;
	struct synaptics_rmi4_fn_desc rmi_fd;
	struct synaptics_rmi4_fn *function_handler;
	struct synaptics_rmi4_device_info *rmi;

	INIT_LIST_HEAD(&rmi4_data->rmi4_mod_info.support_fn_list);

	/* Scann the page description tables of the pages to service */
	for (page_number = 0; page_number < PAGES_TO_SERVICE; page_number++) {
		for (pdt_entry_addr = PDT_START; pdt_entry_addr > PDT_END;
		     pdt_entry_addr -= PDT_ENTRY_SIZE) {
			pdt_entry_addr |= (page_number << 8);

			retval =
			    synaptics_rmi4_i2c_read(rmi4_data, pdt_entry_addr,
						    (unsigned char *)&rmi_fd,
						    sizeof(rmi_fd));
			if (retval < 0)
				return retval;

			function_handler = NULL;

			if (rmi_fd.fn_number) {
				dev_info(&rmi4_data->i2c_client->dev,
					 "%s Function %02x found"
					 " on page %d\n", __func__,
					 rmi_fd.fn_number, page_number);
				switch (rmi_fd.fn_number & MASK_8BIT) {
				case SYNAPTICS_RMI4_DEVICE_CONTROL_FN:
					rmi4_data->f01_query_base_addr =
					    rmi_fd.query_base_addr;
					rmi4_data->f01_ctrl_base_addr =
					    rmi_fd.ctrl_base_addr;
					rmi4_data->f01_data_base_addr =
					    rmi_fd.data_base_addr;
					rmi4_data->f01_cmd_base_addr =
					    rmi_fd.cmd_base_addr;

					retval =
					synaptics_rmi4_i2c_read(rmi4_data,
					rmi4_data->f01_data_base_addr,
					device_status.
					data,
					sizeof(device_status.data));
					if (retval < 0)
						return retval;

					if (device_status.flash_prog == 1) {
						pr_notice
						    ("%s Device in flash "
						    "programming mode, "
						     "status = 0x%02x\n",
						     __func__,
						     device_status.status_code);
						goto flash_prog_mode;
					}
					break;
				case SYNAPTICS_RMI4_F11_2D_SENSOR_FN:
					if (rmi_fd.intr_src_count) {
						retval =
						    synaptics_rmi4_alloc_fh
						    (&function_handler, &rmi_fd,
						     page_number);
						if (retval < 0) {
							dev_dbg
							    (&rmi4_data->
							     i2c_client->dev,
							     "%s Failed "
							     "to allocate memory for function handler "
							     "for Function %d\n",
							     __func__,
							     rmi_fd.fn_number);
							return retval;
						}

						retval =
						    synaptics_rmi4_f11_2d_init
						    (rmi4_data,
						     function_handler, &rmi_fd,
						     intr_count);
						if (retval < 0)
							return retval;
					}
					break;
				case SYNAPTICS_RMI4_F12_2D_SENSOR_FN:
					if (rmi_fd.intr_src_count) {
						retval =
						    synaptics_rmi4_alloc_fh
						    (&function_handler, &rmi_fd,
						     page_number);
						if (retval < 0) {
							dev_dbg
							    (&rmi4_data->
							     i2c_client->dev,
							     "%s Failed "
							     "to allocate memory for function handler "
							     "for Function %d\n",
							     __func__,
							     rmi_fd.fn_number);
							return retval;
						}

						retval =
						    synaptics_rmi4_f12_2d_init
						    (rmi4_data,
						     function_handler, &rmi_fd,
						     intr_count);
						if (retval < 0)
							return retval;
					}
					break;
				case SYNAPTICS_RMI4_0D_BUTTON_FN:
					if (rmi_fd.intr_src_count) {
						retval =
						    synaptics_rmi4_alloc_fh
						    (&function_handler, &rmi_fd,
						     page_number);
						if (retval < 0) {
							dev_dbg
							    (&rmi4_data->
							     i2c_client->dev,
							     "%s Failed "
							     "to allocate memory for function handler "
							     "for Function %d\n",
							     __func__,
							     rmi_fd.fn_number);
							return retval;
						}

						retval =
						synaptics_rmi4_0d_button_init
						    (rmi4_data,
						     function_handler, &rmi_fd,
						     intr_count);
						if (retval < 0)
							return retval;
					}
					break;
#if PROXIMITY
				case SYNAPTICS_RMI4_PROXIMITY_FN:
					if (rmi_fd.intr_src_count) {
						retval =
						    synaptics_rmi4_alloc_fh
						    (&function_handler, &rmi_fd,
						     page_number);
						if (retval < 0) {
							dev_dbg
							    (&rmi4_data->
							     i2c_client->dev,
							     "%s Failed "
							     "to allocate memory for function handler "
							     "for Function %d\n",
							     __func__,
							     rmi_fd.fn_number);
							return retval;
						}

						retval =
						synaptics_rmi4_proximity_init
						    (rmi4_data,
						     function_handler, &rmi_fd,
						     intr_count);
						if (retval < 0)
							return retval;
					}
					break;
#endif
				}

		/* Accumulate the interrupt count for the next iteration */
				intr_count +=
				    (rmi_fd.intr_src_count & MASK_3BIT);

				if (function_handler && rmi_fd.intr_src_count) {
					dev_dbg(&rmi4_data->i2c_client->dev,
						"%s Adding Function "
						"%02x to the function handler list\n",
						__func__,
						function_handler->fn_number);
					list_add_tail(&function_handler->link,
						      &rmi4_data->rmi4_mod_info.
						      support_fn_list);
				}
			} else {
				dev_dbg(&rmi4_data->i2c_client->dev,
					"%s Reached end of PDT\n", __func__);
				break;
			}
		}
	}

 flash_prog_mode:
	rmi4_data->number_of_interrupt_registers = (intr_count + 7) / 8;
	dev_dbg(&rmi4_data->i2c_client->dev,
		"%s Number of interrupt registers = " "%d\n", __func__,
		rmi4_data->number_of_interrupt_registers);

	retval =
	    synaptics_rmi4_i2c_read(rmi4_data, rmi4_data->f01_query_base_addr,
				    f01_query, sizeof(f01_query));
	if (retval < 0)
		return retval;

	/* RMI Version 4.0 currently supported */
	rmi4_data->rmi4_mod_info.version_major = 4;
	rmi4_data->rmi4_mod_info.version_minor = 0;

	rmi4_data->rmi4_mod_info.manufacturer_id = f01_query[0];
	rmi4_data->rmi4_mod_info.product_props = f01_query[1];
	rmi4_data->rmi4_mod_info.product_info[0] = f01_query[2] & MASK_7BIT;
	rmi4_data->rmi4_mod_info.product_info[1] = f01_query[3] & MASK_7BIT;
	rmi4_data->rmi4_mod_info.date_code[0] = f01_query[4] & MASK_5BIT;
	rmi4_data->rmi4_mod_info.date_code[1] = f01_query[5] & MASK_4BIT;
	rmi4_data->rmi4_mod_info.date_code[2] = f01_query[6] & MASK_5BIT;
	rmi4_data->rmi4_mod_info.tester_id = ((f01_query[7] & MASK_7BIT) << 8) |
	    (f01_query[8] & MASK_7BIT);
	rmi4_data->rmi4_mod_info.serial_number =
	    ((f01_query[9] & MASK_7BIT) << 8) | (f01_query[10] & MASK_7BIT);
	memcpy(rmi4_data->rmi4_mod_info.product_id_string, &f01_query[11], 10);

	if (rmi4_data->rmi4_mod_info.manufacturer_id != 1) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s Non-Synaptics device found,"
			" manufacturer ID = %d\n", __func__,
			rmi4_data->rmi4_mod_info.manufacturer_id);
	}

	memset(interrupt_mask, 0x00, sizeof(interrupt_mask));

	/*
	 * Map out the interrupt bit masks for the interrupt sources from the
	 * registered function handlers.
	 */
	list_for_each_entry(function_handler,
			    &rmi4_data->rmi4_mod_info.support_fn_list, link)
	    data_sources += function_handler->num_of_data_sources;
	if (data_sources) {
		rmi = &(rmi4_data->rmi4_mod_info);
		list_for_each_entry(function_handler, &rmi->support_fn_list,
				    link) {
			if (function_handler->num_of_data_sources) {
				interrupt_mask
				    [function_handler->index_to_intr_reg] |=
				    function_handler->intr_mask;
			}
		}
	}

	/* Enable the interrupt sources of the registered function handlers */
	for (ii = 0; ii < MAX_INTR_REGISTERS; ii++) {
		if (interrupt_mask[ii] != 0x00) {
			dev_dbg(&rmi4_data->i2c_client->dev,
				"%s Interrupt enable mask %d " "= 0x%02x\n",
				__func__, ii, interrupt_mask[ii]);
			f01_ctrl_offset =
			    rmi4_data->f01_ctrl_base_addr + 1 + ii;
			retval =
			    synaptics_rmi4_i2c_write(rmi4_data, f01_ctrl_offset,
						     &(interrupt_mask[ii]),
						     sizeof(interrupt_mask
							    [ii]));
			if (retval < 0)
				return retval;
		}
	}

	return 0;
}

/**
* synaptics_rmi4_detection_work()
*
* Called by the kernel at the scheduled time.
*
* This function is a self-rearming work thread that checks for the insertion
* and removal of other expansion Function modules such as rmi_dev and calls
* their initialization and removal callback functions accordingly.
*/
static void synaptics_rmi4_detection_work(struct work_struct *work)
{
	struct synaptics_rmi4_exp_fn *exp_fn_handler, *next_list_entry;
	struct synaptics_rmi4_data *rmi4_data = container_of(work,
					struct synaptics_rmi4_data,
					detection_work.work);

	queue_delayed_work(rmi4_data->detection_workqueue,
			   &rmi4_data->detection_work,
			   msecs_to_jiffies(EXP_FN_DET_INTERVAL));

	mutex_lock(&synaptics_rmi4_exp_fn_list_mutex);
	if (!list_empty(&synaptics_rmi4_exp_fn_list)) {
		list_for_each_entry_safe(exp_fn_handler, next_list_entry,
					 &synaptics_rmi4_exp_fn_list, link) {
			if (exp_fn_handler) {
				if ((exp_fn_handler->func_init != NULL)
				    && (exp_fn_handler->inserted == false)) {
					exp_fn_handler->func_init(rmi4_data);
					exp_fn_handler->inserted = true;
				} else if ((exp_fn_handler->func_init == NULL)
					   && (exp_fn_handler->inserted ==
					       true)) {
					exp_fn_handler->func_remove(rmi4_data);
					list_del(&exp_fn_handler->link);
					kfree(exp_fn_handler);
				}
			}
		}
	}
	mutex_unlock(&synaptics_rmi4_exp_fn_list_mutex);

	return;
}

/**
* synaptics_rmi4_new_function()
*
* Called by other expansion Function modules in their module init and
* module exit functions.
*
* This function is used by other expansion Function modules such as rmi_dev
* to register themselves with the driver by providing their initialization and
* removal callback function pointers so that they can be inserted or removed
* dynamically at module init and exit times, respectively.
*/
void synaptics_rmi4_new_function(enum exp_fn fn_type, bool insert,
				 int (*func_init) (struct synaptics_rmi4_data *
						   rmi4_data),
				 void (*func_remove) (struct synaptics_rmi4_data
						      *rmi4_data),
				 void (*func_attn) (struct synaptics_rmi4_data *
						    rmi4_data,
						    unsigned char intr_mask))
{
	struct synaptics_rmi4_exp_fn *exp_fn_handler;

	if (!exp_fn_inited) {
		mutex_init(&synaptics_rmi4_exp_fn_list_mutex);
		INIT_LIST_HEAD(&synaptics_rmi4_exp_fn_list);
		exp_fn_inited = 1;
	}

	mutex_lock(&synaptics_rmi4_exp_fn_list_mutex);
	if (insert) {
		exp_fn_handler = kzalloc(sizeof(*exp_fn_handler), GFP_KERNEL);
		if (!exp_fn_handler) {
			pr_err("%s Failed to allocate memory"
			       "for expansion function\n", __func__);
			goto exit;
		}
		exp_fn_handler->fn_type = fn_type;
		exp_fn_handler->func_init = func_init;
		exp_fn_handler->func_remove = func_remove;
		exp_fn_handler->func_attn = func_attn;
		exp_fn_handler->inserted = false;
		list_add_tail(&exp_fn_handler->link,
			      &synaptics_rmi4_exp_fn_list);
	} else {
		list_for_each_entry(exp_fn_handler,
				    &synaptics_rmi4_exp_fn_list, link) {
			if (exp_fn_handler &&
			    (exp_fn_handler->func_init == func_init)) {
				exp_fn_handler->func_init = NULL;
				goto exit;
			}
		}
	}

 exit:
	mutex_unlock(&synaptics_rmi4_exp_fn_list_mutex);

	return;
}

 /* dummy API for cypress touchkey */
int get_tsp_status(void)
{
	return 0;
}

EXPORT_SYMBOL(get_tsp_status);

 /**
 * synaptics_rmi4_probe()
 *
 * Called by the kernel when an association with an I2C device of the
 * same name is made (after doing i2c_add_driver).
 *
 * This funtion allocates and initializes the resources for the driver
 * as an input driver, turns on the power to the sensor, queries the
 * sensor for its supported Functions and characteristics, registers
 * the driver to the input subsystem, sets up the interrupt, handles
 * the registration of the early_suspend and late_resume functions, and
 * creates a work queue for detection of other expansion Function modules.
 */
static int __devinit synaptics_rmi4_probe(struct i2c_client *client,
					  const struct i2c_device_id *dev_id)
{
	int retval;
	unsigned char i;
	unsigned char attr_count;
	struct synaptics_rmi4_f1a_handle *f1a;
	struct synaptics_rmi4_fn *function_handler;
	struct synaptics_rmi4_data *rmi4_data;
	const struct synaptics_rmi4_platform_data *platform_data =
	    client->dev.platform_data;

	if (!i2c_check_functionality(client->adapter,
		I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev,
			"%s SMBus byte data not supported\n", __func__);
		return -EIO;
	}

	if (!platform_data) {
		dev_err(&client->dev, "%s No platform data found\n", __func__);
		return -EINVAL;
	}

	rmi4_data = kzalloc(sizeof(*rmi4_data) * 2, GFP_KERNEL);
	if (!rmi4_data) {
		dev_err(&client->dev,
			"%s Failed to allocate memory for rmi4_data "
			"instance\n", __func__);
		return -ENOMEM;
	}

	rmi4_data->input_dev = input_allocate_device();
	if (rmi4_data->input_dev == NULL) {
		dev_err(&client->dev,
			"%s Failed to allocate input device\n", __func__);
		retval = -ENOMEM;
		goto err_input_device;
	}
/*
	if (platform_data->regulator_en) {
		rmi4_data->regulator = regulator_get(&client->dev, "vdd");
		if (IS_ERR(rmi4_data->regulator)) {
	dev_err(&client->dev,
	"%s Failed to get regulator for device\n",
					__func__);
			retval = PTR_ERR(rmi4_data->regulator);
			goto err_regulator;
		}
		regulator_enable(rmi4_data->regulator);
	}
*/
	platform_data->power(true);

	rmi4_data->i2c_client = client;
	rmi4_data->current_page = MASK_8BIT;
	rmi4_data->board = platform_data;
	rmi4_data->touch_stopped = false;
	rmi4_data->sensor_sleep = false;
	rmi4_data->irq_enabled = false;

	rmi4_data->i2c_read = synaptics_rmi4_i2c_read;
	rmi4_data->i2c_write = synaptics_rmi4_i2c_write;
	rmi4_data->irq_enable = synaptics_rmi4_irq_enable;

	init_waitqueue_head(&rmi4_data->wait);
	mutex_init(&(rmi4_data->rmi4_io_ctrl_mutex));

	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval) {
		dev_err(&client->dev, "%s Failed to query device\n", __func__);
		goto err_query_device;
	}

	i2c_set_clientdata(client, rmi4_data);

	rmi4_data->input_dev->name = DRIVER_NAME;
	rmi4_data->input_dev->phys = INPUT_PHYS_NAME;
	rmi4_data->input_dev->id.bustype = BUS_I2C;
	rmi4_data->input_dev->dev.parent = &client->dev;
	input_set_drvdata(rmi4_data->input_dev, rmi4_data);

	set_bit(EV_SYN, rmi4_data->input_dev->evbit);
	set_bit(EV_KEY, rmi4_data->input_dev->evbit);
	set_bit(EV_ABS, rmi4_data->input_dev->evbit);
#ifdef INPUT_PROP_DIRECT
	set_bit(BTN_TOUCH, rmi4_data->input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, rmi4_data->input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, rmi4_data->input_dev->propbit);
#endif

	input_set_abs_params(rmi4_data->input_dev, ABS_MT_POSITION_X, 0,
			     rmi4_data->sensor_max_x, 0, 0);
	input_set_abs_params(rmi4_data->input_dev, ABS_MT_POSITION_Y, 0,
			     rmi4_data->sensor_max_y, 0, 0);
	input_set_abs_params(rmi4_data->input_dev, ABS_MT_TOUCH_MAJOR, 0,
			     MAX_TOUCH_MAJOR, 0, 0);
#if PROXIMITY
	input_set_abs_params(rmi4_data->input_dev, ABS_MT_DISTANCE, 0,
			     HOVER_Z_MAX, 0, 0);
#endif

	if (!list_empty(&rmi4_data->rmi4_mod_info.support_fn_list)) {
		list_for_each_entry(function_handler,
				    &rmi4_data->rmi4_mod_info.support_fn_list,
				    link) {
			if (function_handler
			    &&(function_handler->fn_number ==
				SYNAPTICS_RMI4_0D_BUTTON_FN)) {
				f1a = function_handler->data;
				for (i = 0; i < f1a->valid_button_count; i++) {
					set_bit(f1a->button_map[i],
						rmi4_data->input_dev->keybit);
					input_set_capability
					    (rmi4_data->input_dev, EV_KEY,
					     f1a->button_map[i]);
				}
			}
		}
	}

	retval = input_register_device(rmi4_data->input_dev);
	if (retval) {
		dev_err(&client->dev,
			"%s Failed to register input device\n", __func__);
		goto err_query_device;
	}
	rmi4_data->irq = gpio_to_irq(platform_data->gpio);

	retval = synaptics_rmi4_irq_enable(rmi4_data, true);
	if (retval) {
		dev_err(&client->dev,
			"%s Failed to enable attention interrupt\n", __func__);
		goto err_enable_irq;
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	rmi4_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	rmi4_data->early_suspend.suspend = synaptics_rmi4_early_suspend;
	rmi4_data->early_suspend.resume = synaptics_rmi4_late_resume;
	register_early_suspend(&rmi4_data->early_suspend);
#endif

	if (!exp_fn_inited) {
		mutex_init(&synaptics_rmi4_exp_fn_list_mutex);
		INIT_LIST_HEAD(&synaptics_rmi4_exp_fn_list);
		exp_fn_inited = 1;
	}

	rmi4_data->detection_workqueue =
	    create_singlethread_workqueue("rmi_detection_workqueue");
	INIT_DELAYED_WORK(&rmi4_data->detection_work,
			  synaptics_rmi4_detection_work);
	queue_delayed_work(rmi4_data->detection_workqueue,
			   &rmi4_data->detection_work,
			   msecs_to_jiffies(EXP_FN_DET_INTERVAL));

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		retval = sysfs_create_file(&rmi4_data->input_dev->dev.kobj,
					   &attrs[attr_count].attr);
		if (retval < 0) {
			dev_err(&client->dev,
				"%s Failed to create sysfs attributes\n",
				__func__);
			goto err_sysfs;
		}
	}

	return retval;

 err_sysfs:
	for (attr_count--; attr_count >= 0; attr_count--) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				  &attrs[attr_count].attr);
	}

 err_enable_irq:
	input_unregister_device(rmi4_data->input_dev);

 err_query_device:
	platform_data->power(false);
/*	if (platform_data->regulator_en) {
		regulator_disable(rmi4_data->regulator);
		regulator_put(rmi4_data->regulator);
	}
*/
	if (!list_empty(&rmi4_data->rmi4_mod_info.support_fn_list)) {
		list_for_each_entry(function_handler,
				    &rmi4_data->rmi4_mod_info.support_fn_list,
				    link) {
			if (function_handler) {
				if (function_handler->fn_number ==
				    SYNAPTICS_RMI4_0D_BUTTON_FN)
					synaptics_rmi4_0d_kfree
					    (function_handler);
				else
					kfree(function_handler->data);
				kfree(function_handler);
			}
		}
	}

 err_regulator:
	input_free_device(rmi4_data->input_dev);
	rmi4_data->input_dev = NULL;

 err_input_device:
	kfree(rmi4_data);

	return retval;
}

 /**
 * synaptics_rmi4_remove()
 *
 * Called by the kernel when the association with an I2C device of the
 * same name is broken (when the driver is unloaded).
 *
 * This funtion terminates the work queue, stops sensor data acquisition,
 * frees the interrupt, unregisters the driver from the input subsystem,
 * turns off the power to the sensor, and frees other allocated resources.
 */
static int __devexit synaptics_rmi4_remove(struct i2c_client *client)
{
	unsigned char attr_count;
	struct synaptics_rmi4_fn *function_handler;
	struct synaptics_rmi4_data *rmi4_data = i2c_get_clientdata(client);

	cancel_delayed_work(&rmi4_data->detection_work);
/*	cancel_rearming_delayed_work(&rmi4_data->detection_work);*/
	flush_workqueue(rmi4_data->detection_workqueue);
	destroy_workqueue(rmi4_data->detection_workqueue);

	rmi4_data->touch_stopped = true;
	wake_up(&rmi4_data->wait);

	synaptics_rmi4_irq_enable(rmi4_data, false);

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				  &attrs[attr_count].attr);
	}

	input_unregister_device(rmi4_data->input_dev);
	rmi4_data->board->power(false);
/*
	if (platform_data->regulator_en) {
		regulator_disable(rmi4_data->regulator);
		regulator_put(rmi4_data->regulator);
	}
*/
	if (!list_empty(&rmi4_data->rmi4_mod_info.support_fn_list)) {
		list_for_each_entry(function_handler,
				    &rmi4_data->rmi4_mod_info.support_fn_list,
				    link) {
			if (function_handler) {
				if (function_handler->fn_number ==
				    SYNAPTICS_RMI4_0D_BUTTON_FN)
					synaptics_rmi4_0d_kfree
					    (function_handler);
				else
					kfree(function_handler->data);
				kfree(function_handler);
			}
		}
	}

	input_free_device(rmi4_data->input_dev);

	kfree(rmi4_data);

	return 0;
}

#ifdef CONFIG_PM
 /**
 * synaptics_rmi4_sensor_sleep()
 *
 * Called by synaptics_rmi4_early_suspend() and synaptics_rmi4_suspend().
 *
 * This function stops finger data acquisition and puts the sensor to sleep.
 */
static void synaptics_rmi4_sensor_sleep(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char device_ctrl;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 rmi4_data->f01_ctrl_base_addr,
					 &device_ctrl, sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->input_dev->dev),
			"%s Failed to enter sleep mode\n", __func__);
		rmi4_data->sensor_sleep = false;
		return;
	}

	device_ctrl = (device_ctrl & ~MASK_3BIT);
	device_ctrl = (device_ctrl | NO_SLEEP_OFF | SENSOR_SLEEP);

	retval = synaptics_rmi4_i2c_write(rmi4_data,
					  rmi4_data->f01_ctrl_base_addr,
					  &device_ctrl, sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->input_dev->dev),
			"%s Failed to enter sleep mode\n", __func__);
		rmi4_data->sensor_sleep = false;
		return;
	} else {
		rmi4_data->sensor_sleep = true;
	}

	return;
}

 /**
 * synaptics_rmi4_sensor_wake()
 *
 * Called by synaptics_rmi4_resume() and synaptics_rmi4_late_resume().
 *
 * This function wakes the sensor from sleep.
 */
static void synaptics_rmi4_sensor_wake(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char device_ctrl;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
					 rmi4_data->f01_ctrl_base_addr,
					 &device_ctrl, sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->input_dev->dev),
			"%s Failed to wake from sleep mode\n", __func__);
		rmi4_data->sensor_sleep = true;
		return;
	}

	device_ctrl = (device_ctrl & ~MASK_3BIT);
	device_ctrl = (device_ctrl | NO_SLEEP_OFF | NORMAL_OPERATION);

	retval = synaptics_rmi4_i2c_write(rmi4_data,
					  rmi4_data->f01_ctrl_base_addr,
					  &device_ctrl, sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->input_dev->dev),
			"%s Failed to wake from sleep mode\n", __func__);
		rmi4_data->sensor_sleep = true;
		return;
	} else {
		rmi4_data->sensor_sleep = false;
	}

	return;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
 /**
 * synaptics_rmi4_early_suspend()
 *
 * Called by the kernel during the early suspend phase when the system
 * enters suspend.
 *
 * This function calls synaptics_rmi4_sensor_sleep() to stop finger data
 * acquisition and put the sensor to sleep.
 */
static void synaptics_rmi4_early_suspend(struct early_suspend *h)
{
	struct synaptics_rmi4_data *rmi4_data =
	    container_of(h, struct synaptics_rmi4_data, early_suspend);

	rmi4_data->touch_stopped = true;
	wake_up(&rmi4_data->wait);
	synaptics_rmi4_irq_enable(rmi4_data, false);
	synaptics_rmi4_sensor_sleep(rmi4_data);

	if (rmi4_data->full_pm_cycle)
		synaptics_rmi4_suspend(&(rmi4_data->input_dev->dev));

	return;
}

 /**
 * synaptics_rmi4_late_resume()
 *
 * Called by the kernel during the late resume phase when the system
 * wakes up from suspend.
 *
 * This function goes through the sensor wake process if the system wakes up
 * from early suspend (without going into suspend).
 */
static void synaptics_rmi4_late_resume(struct early_suspend *h)
{
	struct synaptics_rmi4_data *rmi4_data =
	    container_of(h, struct synaptics_rmi4_data, early_suspend);

	if (rmi4_data->full_pm_cycle)
		synaptics_rmi4_resume(&(rmi4_data->input_dev->dev));

	if (rmi4_data->sensor_sleep == true) {
		synaptics_rmi4_sensor_wake(rmi4_data);
		rmi4_data->touch_stopped = false;
		synaptics_rmi4_irq_enable(rmi4_data, true);
	}

	return;
}
#endif

 /**
 * synaptics_rmi4_suspend()
 *
 * Called by the kernel during the suspend phase when the system
 * enters suspend.
 *
 * This function stops finger data acquisition and puts the sensor to
 * sleep (if not already done so during the early suspend phase),
 * disables the interrupt, and turns off the power to the sensor.
 */
static int synaptics_rmi4_suspend(struct device *dev)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	const struct synaptics_rmi4_platform_data *platform_data =
	    rmi4_data->board;

	if (!rmi4_data->sensor_sleep) {
		rmi4_data->touch_stopped = true;
		wake_up(&rmi4_data->wait);
		synaptics_rmi4_irq_enable(rmi4_data, false);
		synaptics_rmi4_sensor_sleep(rmi4_data);
	}

	platform_data->power(false);
/*
	if (platform_data->regulator_en)
		regulator_disable(rmi4_data->regulator);
*/
	return 0;
}

 /**
 * synaptics_rmi4_resume()
 *
 * Called by the kernel during the resume phase when the system
 * wakes up from suspend.
 *
 * This function turns on the power to the sensor, wakes the sensor
 * from sleep, enables the interrupt, and starts finger data
 * acquisition.
 */
static int synaptics_rmi4_resume(struct device *dev)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	const struct synaptics_rmi4_platform_data *platform_data =
	    rmi4_data->board;

	platform_data->power(true);

/*	if (platform_data->regulator_en)
		regulator_enable(rmi4_data->regulator);
*/
	synaptics_rmi4_sensor_wake(rmi4_data);
	rmi4_data->touch_stopped = false;
	synaptics_rmi4_irq_enable(rmi4_data, true);

	return 0;
}

static const struct dev_pm_ops synaptics_rmi4_dev_pm_ops = {
	.suspend = synaptics_rmi4_suspend,
	.resume = synaptics_rmi4_resume,
};
#endif

static const struct i2c_device_id synaptics_rmi4_id_table[] = {
	{DRIVER_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, synaptics_rmi4_id_table);

static struct i2c_driver synaptics_rmi4_driver = {
	.driver = {
		   .name = DRIVER_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_PM
		   .pm = &synaptics_rmi4_dev_pm_ops,
#endif
		   },
	.probe = synaptics_rmi4_probe,
	.remove = __devexit_p(synaptics_rmi4_remove),
	.id_table = synaptics_rmi4_id_table,
};

 /**
 * synaptics_rmi4_init()
 *
 * Called by the kernel during do_initcalls (if built-in)
 * or when the driver is loaded (if a module).
 *
 * This function registers the driver to the I2C subsystem.
 *
 */
static int __init synaptics_rmi4_init(void)
{
	return i2c_add_driver(&synaptics_rmi4_driver);
}

 /**
 * synaptics_rmi4_exit()
 *
 * Called by the kernel when the driver is unloaded.
 *
 * This funtion unregisters the driver from the I2C subsystem.
 *
 */
static void __exit synaptics_rmi4_exit(void)
{
	i2c_del_driver(&synaptics_rmi4_driver);
}

module_init(synaptics_rmi4_init);
module_exit(synaptics_rmi4_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("synaptics rmi4 i2c touch Driver");
MODULE_ALIAS("i2c:synaptics_rmi4_ts");
