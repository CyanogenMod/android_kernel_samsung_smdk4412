/*  Date: 2011/8/8 11:00:00
 *  Revision: 1.6
 */

/*
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 */


/* file BMA254.c
   brief This file contains all function implementations for the BMA254 in linux

*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/earlysuspend.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#define SENSOR_NAME			"bma254"
#define GRAVITY_EARTH                   9806550
#define ABSMIN_2G                       (-GRAVITY_EARTH * 2)
#define ABSMAX_2G                       (GRAVITY_EARTH * 2)
#define SLOPE_THRESHOLD_VALUE		32
#define SLOPE_DURATION_VALUE		1
#define INTERRUPT_LATCH_MODE		13
#define INTERRUPT_ENABLE		1
#define INTERRUPT_DISABLE		0
#define MAP_SLOPE_INTERRUPT		2
#define SLOPE_X_INDEX			5
#define SLOPE_Y_INDEX			6
#define SLOPE_Z_INDEX			7

#define BMA254_MIN_DELAY		1
#define BMA254_DEFAULT_DELAY	200

#define BMA254_CHIP_ID			0xFA
#define BMA254_RANGE_SET		0
#define BMA254_BW_SET			4


/*
 *
 *      register definitions
 *
 */

#define BMA254_CHIP_ID_REG                      0x00
#define BMA254_VERSION_REG                      0x01
#define BMA254_X_AXIS_LSB_REG                   0x02
#define BMA254_X_AXIS_MSB_REG                   0x03
#define BMA254_Y_AXIS_LSB_REG                   0x04
#define BMA254_Y_AXIS_MSB_REG                   0x05
#define BMA254_Z_AXIS_LSB_REG                   0x06
#define BMA254_Z_AXIS_MSB_REG                   0x07
#define BMA254_TEMP_RD_REG                      0x08
#define BMA254_STATUS1_REG                      0x09
#define BMA254_STATUS2_REG                      0x0A
#define BMA254_STATUS_TAP_SLOPE_REG             0x0B
#define BMA254_STATUS_ORIENT_HIGH_REG           0x0C
#define BMA254_RANGE_SEL_REG                    0x0F
#define BMA254_BW_SEL_REG                       0x10
#define BMA254_MODE_CTRL_REG                    0x11
#define BMA254_LOW_NOISE_CTRL_REG               0x12
#define BMA254_DATA_CTRL_REG                    0x13
#define BMA254_RESET_REG                        0x14
#define BMA254_INT_ENABLE1_REG                  0x16
#define BMA254_INT_ENABLE2_REG                  0x17
#define BMA254_INT1_PAD_SEL_REG                 0x19
#define BMA254_INT_DATA_SEL_REG                 0x1A
#define BMA254_INT2_PAD_SEL_REG                 0x1B
#define BMA254_INT_SRC_REG                      0x1E
#define BMA254_INT_SET_REG                      0x20
#define BMA254_INT_CTRL_REG                     0x21
#define BMA254_LOW_DURN_REG                     0x22
#define BMA254_LOW_THRES_REG                    0x23
#define BMA254_LOW_HIGH_HYST_REG                0x24
#define BMA254_HIGH_DURN_REG                    0x25
#define BMA254_HIGH_THRES_REG                   0x26
#define BMA254_SLOPE_DURN_REG                   0x27
#define BMA254_SLOPE_THRES_REG                  0x28
#define BMA254_TAP_PARAM_REG                    0x2A
#define BMA254_TAP_THRES_REG                    0x2B
#define BMA254_ORIENT_PARAM_REG                 0x2C
#define BMA254_THETA_BLOCK_REG                  0x2D
#define BMA254_THETA_FLAT_REG                   0x2E
#define BMA254_FLAT_HOLD_TIME_REG               0x2F
#define BMA254_STATUS_LOW_POWER_REG             0x31
#define BMA254_SELF_TEST_REG                    0x32
#define BMA254_EEPROM_CTRL_REG                  0x33
#define BMA254_SERIAL_CTRL_REG                  0x34
#define BMA254_CTRL_UNLOCK_REG                  0x35
#define BMA254_OFFSET_CTRL_REG                  0x36
#define BMA254_OFFSET_PARAMS_REG                0x37
#define BMA254_OFFSET_FILT_X_REG                0x38
#define BMA254_OFFSET_FILT_Y_REG                0x39
#define BMA254_OFFSET_FILT_Z_REG                0x3A
#define BMA254_OFFSET_UNFILT_X_REG              0x3B
#define BMA254_OFFSET_UNFILT_Y_REG              0x3C
#define BMA254_OFFSET_UNFILT_Z_REG              0x3D
#define BMA254_SPARE_0_REG                      0x3E
#define BMA254_SPARE_1_REG                      0x3F




#define BMA254_ACC_X_LSB__POS           4
#define BMA254_ACC_X_LSB__LEN           4
#define BMA254_ACC_X_LSB__MSK           0xF0
#define BMA254_ACC_X_LSB__REG           BMA254_X_AXIS_LSB_REG

#define BMA254_ACC_X_MSB__POS           0
#define BMA254_ACC_X_MSB__LEN           8
#define BMA254_ACC_X_MSB__MSK           0xFF
#define BMA254_ACC_X_MSB__REG           BMA254_X_AXIS_MSB_REG

#define BMA254_ACC_Y_LSB__POS           4
#define BMA254_ACC_Y_LSB__LEN           4
#define BMA254_ACC_Y_LSB__MSK           0xF0
#define BMA254_ACC_Y_LSB__REG           BMA254_Y_AXIS_LSB_REG

#define BMA254_ACC_Y_MSB__POS           0
#define BMA254_ACC_Y_MSB__LEN           8
#define BMA254_ACC_Y_MSB__MSK           0xFF
#define BMA254_ACC_Y_MSB__REG           BMA254_Y_AXIS_MSB_REG

#define BMA254_ACC_Z_LSB__POS           4
#define BMA254_ACC_Z_LSB__LEN           4
#define BMA254_ACC_Z_LSB__MSK           0xF0
#define BMA254_ACC_Z_LSB__REG           BMA254_Z_AXIS_LSB_REG

#define BMA254_ACC_Z_MSB__POS           0
#define BMA254_ACC_Z_MSB__LEN           8
#define BMA254_ACC_Z_MSB__MSK           0xFF
#define BMA254_ACC_Z_MSB__REG           BMA254_Z_AXIS_MSB_REG

#define BMA254_RANGE_SEL__POS             0
#define BMA254_RANGE_SEL__LEN             4
#define BMA254_RANGE_SEL__MSK             0x0F
#define BMA254_RANGE_SEL__REG             BMA254_RANGE_SEL_REG

#define BMA254_BANDWIDTH__POS             0
#define BMA254_BANDWIDTH__LEN             5
#define BMA254_BANDWIDTH__MSK             0x1F
#define BMA254_BANDWIDTH__REG             BMA254_BW_SEL_REG

#define BMA254_EN_LOW_POWER__POS          6
#define BMA254_EN_LOW_POWER__LEN          1
#define BMA254_EN_LOW_POWER__MSK          0x40
#define BMA254_EN_LOW_POWER__REG          BMA254_MODE_CTRL_REG

#define BMA254_EN_SUSPEND__POS            7
#define BMA254_EN_SUSPEND__LEN            1
#define BMA254_EN_SUSPEND__MSK            0x80
#define BMA254_EN_SUSPEND__REG            BMA254_MODE_CTRL_REG

#define BMA254_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)


#define BMA254_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))


/* range and bandwidth */

#define BMA254_RANGE_2G                 3
#define BMA254_RANGE_4G                 5
#define BMA254_RANGE_8G                 8
#define BMA254_RANGE_16G                12


#define BMA254_BW_7_81HZ        0x08
#define BMA254_BW_15_63HZ       0x09
#define BMA254_BW_31_25HZ       0x0A
#define BMA254_BW_62_50HZ       0x0B
#define BMA254_BW_125HZ         0x0C
#define BMA254_BW_250HZ         0x0D
#define BMA254_BW_500HZ         0x0E
#define BMA254_BW_1000HZ        0x0F

/* mode settings */

#define BMA254_MODE_NORMAL      0
#define BMA254_MODE_LOWPOWER    1
#define BMA254_MODE_SUSPEND     2


#define BMA254_EN_SELF_TEST__POS                0
#define BMA254_EN_SELF_TEST__LEN                2
#define BMA254_EN_SELF_TEST__MSK                0x03
#define BMA254_EN_SELF_TEST__REG                BMA254_SELF_TEST_REG



#define BMA254_NEG_SELF_TEST__POS               2
#define BMA254_NEG_SELF_TEST__LEN               1
#define BMA254_NEG_SELF_TEST__MSK               0x04
#define BMA254_NEG_SELF_TEST__REG               BMA254_SELF_TEST_REG

#define BMA254_EN_FAST_COMP__POS                5
#define BMA254_EN_FAST_COMP__LEN                2
#define BMA254_EN_FAST_COMP__MSK                0x60
#define BMA254_EN_FAST_COMP__REG                BMA254_OFFSET_CTRL_REG

#define BMA254_FAST_COMP_RDY_S__POS             4
#define BMA254_FAST_COMP_RDY_S__LEN             1
#define BMA254_FAST_COMP_RDY_S__MSK             0x10
#define BMA254_FAST_COMP_RDY_S__REG             BMA254_OFFSET_CTRL_REG

#define BMA254_COMP_TARGET_OFFSET_X__POS        1
#define BMA254_COMP_TARGET_OFFSET_X__LEN        2
#define BMA254_COMP_TARGET_OFFSET_X__MSK        0x06
#define BMA254_COMP_TARGET_OFFSET_X__REG        BMA254_OFFSET_PARAMS_REG

#define BMA254_COMP_TARGET_OFFSET_Y__POS        3
#define BMA254_COMP_TARGET_OFFSET_Y__LEN        2
#define BMA254_COMP_TARGET_OFFSET_Y__MSK        0x18
#define BMA254_COMP_TARGET_OFFSET_Y__REG        BMA254_OFFSET_PARAMS_REG

#define BMA254_COMP_TARGET_OFFSET_Z__POS        5
#define BMA254_COMP_TARGET_OFFSET_Z__LEN        2
#define BMA254_COMP_TARGET_OFFSET_Z__MSK        0x60
#define BMA254_COMP_TARGET_OFFSET_Z__REG        BMA254_OFFSET_PARAMS_REG


static const u8 bma254_valid_range[] = {
	BMA254_RANGE_2G,
	BMA254_RANGE_4G,
	BMA254_RANGE_8G,
	BMA254_RANGE_16G,
};

static const u8 bma254_valid_bw[] = {
	BMA254_BW_7_81HZ,
	BMA254_BW_15_63HZ,
	BMA254_BW_31_25HZ,
	BMA254_BW_62_50HZ,
	BMA254_BW_125HZ,
	BMA254_BW_250HZ,
	BMA254_BW_500HZ,
	BMA254_BW_1000HZ,
};

struct bma254acc {
	s16	x,
		y,
		z;
};

struct bma254_data {
	struct i2c_client *bma254_client;
	atomic_t delay;
	atomic_t enable;
	unsigned char mode;
	struct input_dev *input;
	struct bma254acc value;
	struct mutex value_mutex;
	struct mutex enable_mutex;
	struct mutex mode_mutex;
	struct delayed_work work;
	struct work_struct irq_work;
	struct early_suspend early_suspend;

	atomic_t selftest_result;
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma254_early_suspend(struct early_suspend *h);
static void bma254_late_resume(struct early_suspend *h);
#endif

static int bma254_smbus_read_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_read_byte_data(client, reg_addr);
	if (dummy < 0)
		return -1;
	*data = dummy & 0x000000ff;

	return 0;
}

static int bma254_smbus_write_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
	if (dummy < 0)
		return -1;
	return 0;
}

static int bma254_smbus_read_byte_block(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data, unsigned char len)
{
	s32 dummy;
	dummy = i2c_smbus_read_i2c_block_data(client, reg_addr, len, data);
	if (dummy < 0)
		return -1;
	return 0;
}

static int bma254_set_mode(struct i2c_client *client, unsigned char Mode)
{
	int comres = 0;
	unsigned char data1;

	if (client == NULL) {
		comres = -1;
	} else{
		if (Mode < 3) {
			comres = bma254_smbus_read_byte(client,
					BMA254_EN_LOW_POWER__REG, &data1);
			switch (Mode) {
			case BMA254_MODE_NORMAL:
				data1  = BMA254_SET_BITSLICE(data1,
						BMA254_EN_LOW_POWER, 0);
				data1  = BMA254_SET_BITSLICE(data1,
						BMA254_EN_SUSPEND, 0);
				break;
			case BMA254_MODE_LOWPOWER:
				data1  = BMA254_SET_BITSLICE(data1,
						BMA254_EN_LOW_POWER, 1);
				data1  = BMA254_SET_BITSLICE(data1,
						BMA254_EN_SUSPEND, 0);
				break;
			case BMA254_MODE_SUSPEND:
				data1  = BMA254_SET_BITSLICE(data1,
						BMA254_EN_LOW_POWER, 0);
				data1  = BMA254_SET_BITSLICE(data1,
						BMA254_EN_SUSPEND, 1);
				break;
			default:
				break;
			}

			comres += bma254_smbus_write_byte(client,
					BMA254_EN_LOW_POWER__REG, &data1);
		} else{
			comres = -1;
		}
	}

	return comres;
}

static int bma254_get_mode(struct i2c_client *client, unsigned char *Mode)
{
	int comres = 0;

	if (client == NULL) {
		comres = -1;
	} else{
		comres = bma254_smbus_read_byte(client,
				BMA254_EN_LOW_POWER__REG, Mode);
		*Mode  = (*Mode) >> 6;
	}

	return comres;
}

static int bma254_set_range(struct i2c_client *client, unsigned char Range)
{
	int comres = 0;
	unsigned char data1;
	int i;

	if (client == NULL) {
		comres = -1;
	} else{
		for (i = 0; i < ARRAY_SIZE(bma254_valid_range); i++) {
			if (bma254_valid_range[i] == Range)
				break;
		}

		if (ARRAY_SIZE(bma254_valid_range) > i) {
			comres = bma254_smbus_read_byte(client,
					BMA254_RANGE_SEL_REG, &data1);

			data1  = BMA254_SET_BITSLICE(data1,
					BMA254_RANGE_SEL, Range);

			comres += bma254_smbus_write_byte(client,
					BMA254_RANGE_SEL_REG, &data1);
		} else {
			comres = -EINVAL;
		}
	}

	return comres;
}

static int bma254_get_range(struct i2c_client *client, unsigned char *Range)
{
	int comres = 0;
	unsigned char data;

	if (client == NULL) {
		comres = -1;
	} else{
		comres = bma254_smbus_read_byte(client, BMA254_RANGE_SEL__REG,
				&data);
		data = BMA254_GET_BITSLICE(data, BMA254_RANGE_SEL);
		*Range = data;
	}

	return comres;
}


static int bma254_set_bandwidth(struct i2c_client *client, unsigned char BW)
{
	int comres = 0;
	unsigned char data;
	int i = 0;

	if (client == NULL) {
		comres = -1;
	} else {

		for (i = 0; i < ARRAY_SIZE(bma254_valid_bw); i++) {
			if (bma254_valid_bw[i] == BW)
				break;
		}

		if (ARRAY_SIZE(bma254_valid_bw) > i) {
			comres = bma254_smbus_read_byte(client,
					BMA254_BANDWIDTH__REG, &data);
			data = BMA254_SET_BITSLICE(data, BMA254_BANDWIDTH, BW);
			comres += bma254_smbus_write_byte(client,
					BMA254_BANDWIDTH__REG, &data);
		} else {
			comres = -EINVAL;
		}
	}

	return comres;
}

static int bma254_get_bandwidth(struct i2c_client *client, unsigned char *BW)
{
	int comres = 0;
	unsigned char data;

	if (client == NULL) {
		comres = -1;
	} else{
		comres = bma254_smbus_read_byte(client, BMA254_BANDWIDTH__REG,
				&data);
		data = BMA254_GET_BITSLICE(data, BMA254_BANDWIDTH);
		if (data < BMA254_BW_7_81HZ)
			*BW = BMA254_BW_7_81HZ;
		else if (data > BMA254_BW_1000HZ)
			*BW = BMA254_BW_1000HZ;
		else
			*BW = data;
	}

	return comres;
}

static int bma254_read_accel_xyz(struct i2c_client *client,
		struct bma254acc *acc)
{
	int comres;
	unsigned char data[6];
	if (client == NULL) {
		comres = -1;
	} else{
		comres = bma254_smbus_read_byte_block(client,
				BMA254_ACC_X_LSB__REG, data, 6);

		acc->x = BMA254_GET_BITSLICE(data[0], BMA254_ACC_X_LSB)
			|(BMA254_GET_BITSLICE(data[1], BMA254_ACC_X_MSB)
					<<BMA254_ACC_X_LSB__LEN);
		acc->x = acc->x << (sizeof(short)*8-(BMA254_ACC_X_LSB__LEN
					+ BMA254_ACC_X_MSB__LEN));
		acc->x = acc->x >> (sizeof(short)*8-(BMA254_ACC_X_LSB__LEN
					+ BMA254_ACC_X_MSB__LEN));
		acc->y = BMA254_GET_BITSLICE(data[2], BMA254_ACC_Y_LSB)
			| (BMA254_GET_BITSLICE(data[3], BMA254_ACC_Y_MSB)
					<<BMA254_ACC_Y_LSB__LEN);
		acc->y = acc->y << (sizeof(short)*8-(BMA254_ACC_Y_LSB__LEN
					+ BMA254_ACC_Y_MSB__LEN));
		acc->y = acc->y >> (sizeof(short)*8-(BMA254_ACC_Y_LSB__LEN
					+ BMA254_ACC_Y_MSB__LEN));

		acc->z = BMA254_GET_BITSLICE(data[4], BMA254_ACC_Z_LSB)
			| (BMA254_GET_BITSLICE(data[5], BMA254_ACC_Z_MSB)
					<<BMA254_ACC_Z_LSB__LEN);
		acc->z = acc->z << (sizeof(short)*8-(BMA254_ACC_Z_LSB__LEN
					+ BMA254_ACC_Z_MSB__LEN));
		acc->z = acc->z >> (sizeof(short)*8-(BMA254_ACC_Z_LSB__LEN
					+ BMA254_ACC_Z_MSB__LEN));
	}

	return comres;
}

static void bma254_work_func(struct work_struct *work)
{
	struct bma254_data *bma254 = container_of((struct delayed_work *)work,
			struct bma254_data, work);
	static struct bma254acc acc;
	unsigned long delay = msecs_to_jiffies(atomic_read(&bma254->delay));

	bma254_read_accel_xyz(bma254->bma254_client, &acc);

	input_report_abs(bma254->input, ABS_X, acc.x);
	input_report_abs(bma254->input, ABS_Y, acc.y);
	input_report_abs(bma254->input, ABS_Z, acc.z);
	input_sync(bma254->input);
	mutex_lock(&bma254->value_mutex);
	bma254->value = acc;
	mutex_unlock(&bma254->value_mutex);
	schedule_delayed_work(&bma254->work, delay);
}

static ssize_t bma254_range_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	if (bma254_get_range(bma254->bma254_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);
}

static ssize_t bma254_range_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma254_set_range(bma254->bma254_client, (unsigned char) data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma254_bandwidth_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	if (bma254_get_bandwidth(bma254->bma254_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}

static ssize_t bma254_bandwidth_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma254_set_bandwidth(bma254->bma254_client,
				(unsigned char) data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma254_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	if (bma254_get_mode(bma254->bma254_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);
}

static ssize_t bma254_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma254_set_mode(bma254->bma254_client, (unsigned char) data) < 0)
		return -EINVAL;

	return count;
}


static ssize_t bma254_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct bma254_data *bma254 = input_get_drvdata(input);
	int err;

	mutex_lock(&bma254->value_mutex);
	err = sprintf(buf, "%d %d %d\n", bma254->value.x, bma254->value.y,
			bma254->value.z);
	mutex_unlock(&bma254->value_mutex);

	return err;
}

static ssize_t bma254_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", atomic_read(&bma254->delay));

}

static ssize_t bma254_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (data <= 0)
		return -EINVAL;

	if (data < BMA254_MIN_DELAY)
		data = BMA254_MIN_DELAY;

	atomic_set(&bma254->delay, (unsigned int) data);

	return count;
}


static ssize_t bma254_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", atomic_read(&bma254->enable));

}

static void bma254_set_enable(struct device *dev, int enable)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);
	int pre_enable = atomic_read(&bma254->enable);

	mutex_lock(&bma254->enable_mutex);
	if (enable) {
		if (pre_enable == 0) {
			schedule_delayed_work(&bma254->work,
					msecs_to_jiffies(
						atomic_read(&bma254->delay)));
			atomic_set(&bma254->enable, 1);
		}

	} else {
		if (pre_enable == 1) {
			cancel_delayed_work_sync(&bma254->work);
			atomic_set(&bma254->enable, 0);
		}
	}
	mutex_unlock(&bma254->enable_mutex);

}

static ssize_t bma254_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if ((data == 0) || (data == 1))
		bma254_set_enable(dev, data);

	return count;
}

static ssize_t bma254_update_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	mutex_lock(&bma254->value_mutex);
	bma254_read_accel_xyz(bma254->bma254_client, &bma254->value);
	mutex_unlock(&bma254->value_mutex);
	return count;
}

static int bma254_set_selftest_st(struct i2c_client *client,
		unsigned char selftest)
{
	int comres = 0;
	unsigned char data;

	comres = bma254_smbus_read_byte(client,
			BMA254_EN_SELF_TEST__REG, &data);
	data = BMA254_SET_BITSLICE(data,
			BMA254_EN_SELF_TEST, selftest);
	comres = bma254_smbus_write_byte(client,
			BMA254_EN_SELF_TEST__REG, &data);

	return comres;
}

static int bma254_set_selftest_stn(struct i2c_client *client, unsigned char stn)
{
	int comres = 0;
	unsigned char data;

	comres = bma254_smbus_read_byte(client,
			BMA254_NEG_SELF_TEST__REG, &data);
	data = BMA254_SET_BITSLICE(data, BMA254_NEG_SELF_TEST, stn);
	comres = bma254_smbus_write_byte(client,
			BMA254_NEG_SELF_TEST__REG, &data);

	return comres;
}

static int bma254_read_accel_x(struct i2c_client *client, short *a_x)
{
	int comres;
	unsigned char data[2];

	comres = bma254_smbus_read_byte_block(client,
			BMA254_ACC_X_LSB__REG, data, 2);
	*a_x = BMA254_GET_BITSLICE(data[0], BMA254_ACC_X_LSB)
		| (BMA254_GET_BITSLICE(data[1], BMA254_ACC_X_MSB)
				<<BMA254_ACC_X_LSB__LEN);
	*a_x = *a_x << (sizeof(short)*8
			-(BMA254_ACC_X_LSB__LEN+BMA254_ACC_X_MSB__LEN));
	*a_x = *a_x >> (sizeof(short)*8
			-(BMA254_ACC_X_LSB__LEN+BMA254_ACC_X_MSB__LEN));

	return comres;
}
static int bma254_read_accel_y(struct i2c_client *client, short *a_y)
{
	int comres;
	unsigned char data[2];

	comres = bma254_smbus_read_byte_block(client,
			BMA254_ACC_Y_LSB__REG, data, 2);
	*a_y = BMA254_GET_BITSLICE(data[0], BMA254_ACC_Y_LSB)
		| (BMA254_GET_BITSLICE(data[1], BMA254_ACC_Y_MSB)
				<<BMA254_ACC_Y_LSB__LEN);
	*a_y = *a_y << (sizeof(short)*8
			-(BMA254_ACC_Y_LSB__LEN+BMA254_ACC_Y_MSB__LEN));
	*a_y = *a_y >> (sizeof(short)*8
			-(BMA254_ACC_Y_LSB__LEN+BMA254_ACC_Y_MSB__LEN));

	return comres;
}

static int bma254_read_accel_z(struct i2c_client *client, short *a_z)
{
	int comres;
	unsigned char data[2];

	comres = bma254_smbus_read_byte_block(client,
			BMA254_ACC_Z_LSB__REG, data, 2);
	*a_z = BMA254_GET_BITSLICE(data[0], BMA254_ACC_Z_LSB)
		| BMA254_GET_BITSLICE(data[1], BMA254_ACC_Z_MSB)
		<<BMA254_ACC_Z_LSB__LEN;
	*a_z = *a_z << (sizeof(short)*8
			-(BMA254_ACC_Z_LSB__LEN+BMA254_ACC_Z_MSB__LEN));
	*a_z = *a_z >> (sizeof(short)*8
			-(BMA254_ACC_Z_LSB__LEN+BMA254_ACC_Z_MSB__LEN));

	return comres;
}

static ssize_t bma254_selftest_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", atomic_read(&bma254->selftest_result));
}

static ssize_t bma254_selftest_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	unsigned char clear_value = 0;
	int error;
	short value1 = 0;
	short value2 = 0;
	short diff = 0;
	unsigned long result = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (data != 1)
		return -EINVAL;
	/* set to 2 G range */
	if (bma254_set_range(bma254->bma254_client, 0) < 0)
		return -EINVAL;

	bma254_smbus_write_byte(bma254->bma254_client, 0x32, &clear_value);
	/* 1 for x-axis*/
	bma254_set_selftest_st(bma254->bma254_client, 1);
	/* positive direction*/
	bma254_set_selftest_stn(bma254->bma254_client, 0);
	usleep_range(10000, 20000);
	bma254_read_accel_x(bma254->bma254_client, &value1);
	/* negative direction*/
	bma254_set_selftest_stn(bma254->bma254_client, 1);
	usleep_range(10000, 20000);
	bma254_read_accel_x(bma254->bma254_client, &value2);
	diff = value1-value2;

	pr_info("diff x is %d, value1 is %d, value2 is %d\n",
			diff, value1, value2);

	if (abs(diff) < 204)
		result |= 1;

	/* 2 for y-axis*/
	bma254_set_selftest_st(bma254->bma254_client, 2);
	/* positive direction*/
	bma254_set_selftest_stn(bma254->bma254_client, 0);
	usleep_range(10000, 20000);
	bma254_read_accel_y(bma254->bma254_client, &value1);
	/* negative direction*/
	bma254_set_selftest_stn(bma254->bma254_client, 1);
	usleep_range(10000, 20000);
	bma254_read_accel_y(bma254->bma254_client, &value2);
	diff = value1-value2;
	pr_info("diff y is %d, value1 is %d, value2 is %d\n",
			diff, value1, value2);
	if (abs(diff) < 204)
		result |= 2;

	/* 3 for z-axis*/
	bma254_set_selftest_st(bma254->bma254_client, 3);
	/* positive direction*/
	bma254_set_selftest_stn(bma254->bma254_client, 0);
	usleep_range(10000, 20000);
	bma254_read_accel_z(bma254->bma254_client, &value1);
	/* negative direction*/
	bma254_set_selftest_stn(bma254->bma254_client, 1);
	usleep_range(10000, 20000);
	bma254_read_accel_z(bma254->bma254_client, &value2);
	diff = value1 - value2;

	pr_info("diff z is %d, value1 is %d, value2 is %d\n",
			diff, value1, value2);
	if (abs(diff) < 102)
		result |= 4;

	atomic_set(&bma254->selftest_result, (unsigned int) result);

	pr_info("self test finished\n");


	return count;
}

static int bma254_set_offset_target_x(struct i2c_client *client,
		unsigned char offsettarget)
{
	int comres = 0;
	unsigned char data;

	comres = bma254_smbus_read_byte(client,
			BMA254_COMP_TARGET_OFFSET_X__REG, &data);
	data = BMA254_SET_BITSLICE(data,
			BMA254_COMP_TARGET_OFFSET_X, offsettarget);
	comres = bma254_smbus_write_byte(client,
			BMA254_COMP_TARGET_OFFSET_X__REG, &data);

	return comres;
}

static int bma254_get_offset_target_x(struct i2c_client *client,
		unsigned char *offsettarget)
{
	int comres = 0 ;
	unsigned char data;

	comres = bma254_smbus_read_byte(client,
			BMA254_OFFSET_PARAMS_REG, &data);
	data = BMA254_GET_BITSLICE(data, BMA254_COMP_TARGET_OFFSET_X);
	*offsettarget = data;

	return comres;
}

static int bma254_set_offset_target_y(struct i2c_client *client,
		unsigned char offsettarget)
{
	int comres = 0;
	unsigned char data;

	comres = bma254_smbus_read_byte(client,
			BMA254_COMP_TARGET_OFFSET_Y__REG, &data);
	data = BMA254_SET_BITSLICE(data,
			BMA254_COMP_TARGET_OFFSET_Y, offsettarget);
	comres = bma254_smbus_write_byte(client,
			BMA254_COMP_TARGET_OFFSET_Y__REG, &data);

	return comres;
}

static int bma254_get_offset_target_y(struct i2c_client *client,
		unsigned char *offsettarget)
{
	int comres = 0 ;
	unsigned char data;

	comres = bma254_smbus_read_byte(client,
			BMA254_OFFSET_PARAMS_REG, &data);
	data = BMA254_GET_BITSLICE(data, BMA254_COMP_TARGET_OFFSET_Y);
	*offsettarget = data;

	return comres;
}

static int bma254_set_offset_target_z(struct i2c_client *client,
		unsigned char offsettarget)
{
	int comres = 0;
	unsigned char data;

	comres = bma254_smbus_read_byte(client,
			BMA254_COMP_TARGET_OFFSET_Z__REG, &data);
	data = BMA254_SET_BITSLICE(data,
			BMA254_COMP_TARGET_OFFSET_Z, offsettarget);
	comres = bma254_smbus_write_byte(client,
			BMA254_COMP_TARGET_OFFSET_Z__REG, &data);

	return comres;
}

static int bma254_get_offset_target_z(struct i2c_client *client,
		unsigned char *offsettarget)
{
	int comres = 0 ;
	unsigned char data;

	comres = bma254_smbus_read_byte(client,
			BMA254_OFFSET_PARAMS_REG, &data);
	data = BMA254_GET_BITSLICE(data, BMA254_COMP_TARGET_OFFSET_Z);
	*offsettarget = data;

	return comres;
}

static int bma254_get_cal_ready(struct i2c_client *client,
		unsigned char *calrdy)
{
	int comres = 0 ;
	unsigned char data;

	comres = bma254_smbus_read_byte(client,
			BMA254_OFFSET_CTRL_REG, &data);
	data = BMA254_GET_BITSLICE(data, BMA254_FAST_COMP_RDY_S);
	*calrdy = data;

	return comres;
}

static int bma254_set_cal_trigger(struct i2c_client *client,
		unsigned char caltrigger)
{
	int comres = 0;
	unsigned char data;

	comres = bma254_smbus_read_byte(client,
			BMA254_EN_FAST_COMP__REG, &data);
	data = BMA254_SET_BITSLICE(data,
			BMA254_EN_FAST_COMP, caltrigger);
	comres = bma254_smbus_write_byte(client,
			BMA254_EN_FAST_COMP__REG, &data);

	return comres;
}


static ssize_t bma254_fast_calibration_x_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	if (bma254_get_offset_target_x(bma254->bma254_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);
}

static ssize_t bma254_fast_calibration_x_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	signed char tmp;
	unsigned char timeout = 0;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma254_set_offset_target_x(bma254->bma254_client,
				(unsigned char)data) < 0)
		return -EINVAL;

	if (bma254_set_cal_trigger(bma254->bma254_client, 1) < 0)
		return -EINVAL;

	do {
		usleep_range(2000, 3000);
		bma254_get_cal_ready(bma254->bma254_client, &tmp);

		pr_info("wait 2ms and got cal ready flag is %d\n",
				tmp);
		timeout++;
		if (timeout == 50) {
			pr_err("get fast calibration ready error\n");
			return -EINVAL;
		};

	} while (tmp == 0);

	pr_info("x axis fast calibration finished\n");
	return count;
}

static ssize_t bma254_fast_calibration_y_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{


	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	if (bma254_get_offset_target_y(bma254->bma254_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}

static ssize_t bma254_fast_calibration_y_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	signed char tmp;
	unsigned char timeout = 0;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma254_set_offset_target_y(bma254->bma254_client,
				(unsigned char)data) < 0)
		return -EINVAL;

	if (bma254_set_cal_trigger(bma254->bma254_client, 2) < 0)
		return -EINVAL;

	do {
		usleep_range(2000, 3000);
		bma254_get_cal_ready(bma254->bma254_client, &tmp);

		pr_info("wait 2ms and got cal ready flag is %d\n",
				tmp);
		timeout++;
		if (timeout == 50) {
			pr_err("get fast calibration ready error\n");
			return -EINVAL;
		};

	} while (tmp == 0);

	pr_info("y axis fast calibration finished\n");
	return count;
}

static ssize_t bma254_fast_calibration_z_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{


	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	if (bma254_get_offset_target_z(bma254->bma254_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}

static ssize_t bma254_fast_calibration_z_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	signed char tmp;
	unsigned char timeout = 0;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma254_set_offset_target_z(bma254->bma254_client,
				(unsigned char)data) < 0)
		return -EINVAL;

	if (bma254_set_cal_trigger(bma254->bma254_client, 3) < 0)
		return -EINVAL;

	do {
		usleep_range(2000, 3000);
		bma254_get_cal_ready(bma254->bma254_client, &tmp);

		pr_info("wait 2ms and got cal ready flag is %d\n",
				tmp);
		timeout++;
		if (timeout == 50) {
			pr_err("get fast calibration ready error\n");
			return -EINVAL;
		}
	} while (tmp == 0);

	pr_info("z axis fast calibration finished\n");
	return count;
}



static DEVICE_ATTR(range, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma254_range_show, bma254_range_store);
static DEVICE_ATTR(bandwidth, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma254_bandwidth_show, bma254_bandwidth_store);
static DEVICE_ATTR(mode, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma254_mode_show, bma254_mode_store);
static DEVICE_ATTR(value, S_IRUGO,
		bma254_value_show, NULL);
static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma254_delay_show, bma254_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma254_enable_show, bma254_enable_store);
static DEVICE_ATTR(update, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		NULL, bma254_update_store);
static DEVICE_ATTR(selftest, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma254_selftest_show, bma254_selftest_store);
static DEVICE_ATTR(fast_calibration_x, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma254_fast_calibration_x_show,
		bma254_fast_calibration_x_store);
static DEVICE_ATTR(fast_calibration_y, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma254_fast_calibration_y_show,
		bma254_fast_calibration_y_store);
static DEVICE_ATTR(fast_calibration_z, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma254_fast_calibration_z_show,
		bma254_fast_calibration_z_store);

static struct attribute *bma254_attributes[] = {
	&dev_attr_range.attr,
	&dev_attr_bandwidth.attr,
	&dev_attr_mode.attr,
	&dev_attr_value.attr,
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	&dev_attr_update.attr,
	&dev_attr_selftest.attr,
	&dev_attr_fast_calibration_x.attr,
	&dev_attr_fast_calibration_y.attr,
	&dev_attr_fast_calibration_z.attr,
	NULL
};

static struct attribute_group bma254_attribute_group = {
	.attrs = bma254_attributes
};

static int bma254_input_init(struct bma254_data *bma254)
{
	struct input_dev *dev;
	int err;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;
	dev->name = SENSOR_NAME;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_ABS, ABS_MISC);
	input_set_abs_params(dev, ABS_X, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_abs_params(dev, ABS_Y, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_abs_params(dev, ABS_Z, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_drvdata(dev, bma254);

	err = input_register_device(dev);
	if (err < 0) {
		pr_err("bma254_input_init input_register_device=%d\n", err);
		input_free_device(dev);
		return err;
	}
	bma254->input = dev;

	return 0;
}

static void bma254_input_delete(struct bma254_data *bma254)
{
	struct input_dev *dev = bma254->input;

	input_unregister_device(dev);
	input_free_device(dev);
}

static int bma254_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int err = 0;
	int tempvalue;
	struct bma254_data *data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("i2c_check_functionality error\n");
		goto exit;
	}
	data = kzalloc(sizeof(struct bma254_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	/* read chip id */
	tempvalue = 0;
	tempvalue = i2c_smbus_read_word_data(client, BMA254_CHIP_ID_REG);

	if ((tempvalue&0x00FF) == BMA254_CHIP_ID) {
		pr_info("Bosch Sensortec Device detected!\n" \
				"BMA254 registered I2C driver!\n");
	} else{
		pr_err("Bosch Sensortec Device not found, "\
				"i2c error %d\n", tempvalue);
		err = -1;
		goto kfree_exit;
	}
	i2c_set_clientdata(client, data);
	data->bma254_client = client;
	mutex_init(&data->value_mutex);
	mutex_init(&data->mode_mutex);
	mutex_init(&data->enable_mutex);
	bma254_set_bandwidth(client, BMA254_BW_SET);
	bma254_set_range(client, BMA254_RANGE_SET);

	INIT_DELAYED_WORK(&data->work, bma254_work_func);
	atomic_set(&data->delay, BMA254_DEFAULT_DELAY);
	atomic_set(&data->enable, 0);
	err = bma254_input_init(data);
	if (err < 0)
		goto kfree_exit;

	err = sysfs_create_group(&data->input->dev.kobj,
			&bma254_attribute_group);
	if (err < 0)
		goto error_sysfs;

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = bma254_early_suspend;
	data->early_suspend.resume = bma254_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

	return 0;

error_sysfs:
	bma254_input_delete(data);

kfree_exit:
	kfree(data);
exit:
	return err;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma254_early_suspend(struct early_suspend *h)
{
	struct bma254_data *data =
		container_of(h, struct bma254_data, early_suspend);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable) == 1) {
		bma254_set_mode(data->bma254_client, BMA254_MODE_SUSPEND);
		cancel_delayed_work_sync(&data->work);
	}
	mutex_unlock(&data->enable_mutex);
}


static void bma254_late_resume(struct early_suspend *h)
{
	struct bma254_data *data =
		container_of(h, struct bma254_data, early_suspend);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable) == 1) {
		bma254_set_mode(data->bma254_client, BMA254_MODE_NORMAL);
		schedule_delayed_work(&data->work,
				msecs_to_jiffies(atomic_read(&data->delay)));
	}
	mutex_unlock(&data->enable_mutex);
}
#endif

static int bma254_remove(struct i2c_client *client)
{
	struct bma254_data *data = i2c_get_clientdata(client);

	bma254_set_enable(&client->dev, 0);
	unregister_early_suspend(&data->early_suspend);
	sysfs_remove_group(&data->input->dev.kobj, &bma254_attribute_group);
	bma254_input_delete(data);
	kfree(data);
	return 0;
}


static const struct i2c_device_id bma254_id[] = {
	{ SENSOR_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, bma254_id);

static struct i2c_driver bma254_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= SENSOR_NAME,
	},
	.id_table	= bma254_id,
	.probe		= bma254_probe,
	.remove		= bma254_remove,

};

static int __init BMA254_init(void)
{
	return i2c_add_driver(&bma254_driver);
}

static void __exit BMA254_exit(void)
{
	i2c_del_driver(&bma254_driver);
}

MODULE_AUTHOR("Albert Zhang <xu.zhang@bosch-sensortec.com>");
MODULE_DESCRIPTION("BMA254 driver");
MODULE_LICENSE("GPL");

module_init(BMA254_init);
module_exit(BMA254_exit);
