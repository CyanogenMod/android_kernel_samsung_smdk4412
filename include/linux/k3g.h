/*
 *  k3g.h -  ST Microelectronics three-axis gyroscope sensor
 *
 *  Copyright (c) 2010 Samsung Eletronics
 *  Donggeun Kim <dg77.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _K3G_H_
#define _K3G_H_

#define K3G_WHO_AM_I			0x0f
#define K3G_CTRL_REG1			0x20
#define K3G_CTRL_REG2			0x21
#define K3G_CTRL_REG3			0x22
#define K3G_CTRL_REG4			0x23
#define K3G_CTRL_REG5			0x24
#define K3G_REFERENCE			0x25
#define K3G_OUT_TEMP			0x26
#define K3G_STATUS_REG			0x27
#define K3G_OUT_X_L			0x28
#define K3G_OUT_X_H			0x29
#define K3G_OUT_Y_L			0x2a
#define K3G_OUT_Y_H			0x2b
#define K3G_OUT_Z_L			0x2c
#define K3G_OUT_Z_H			0x2d
#define K3G_FIFO_CTRL_REG		0x2e
#define K3G_FIFO_SRC_REG		0x2f
#define K3G_INT1_CFG_REG		0x30
#define K3G_INT1_SRC_REG		0x31
#define K3G_INT1_THS_XH			0x32
#define K3G_INT1_THS_XL			0x33
#define K3G_INT1_THS_YH			0x34
#define K3G_INT1_THS_YL			0x35
#define K3G_INT1_THS_ZH			0x36
#define K3G_INT1_THS_ZL			0x37
#define K3G_INT1_DURATION_REG		0x38

#define K3G_MULTI_OUT_X_L		0xa8

#define K3G_DATA_RATE_SHIFT		6
#define K3G_DATA_RATE_105HZ		((0x0) << K3G_DATA_RATE_SHIFT)
#define K3G_DATA_RATE_210HZ		((0x1) << K3G_DATA_RATE_SHIFT)
#define K3G_DATA_RATE_420HZ		((0x2) << K3G_DATA_RATE_SHIFT)
#define K3G_DATA_RATE_840HZ		((0x3) << K3G_DATA_RATE_SHIFT)
#define K3G_BANDWIDTH_SHIFT		4
#define K3G_BANDWIDTH_MASK		((0x3) << K3G_BANDWIDTH_SHIFT)
#define K3G_POWERDOWN_SHIFT		3
#define K3G_POWERDOWN_POWER_DOWN	((0x0) << K3G_POWERDOWN_SHIFT)
#define K3G_POWERDOWN_NORMAL		((0x1) << K3G_POWERDOWN_SHIFT)
#define K3G_POWERDOWN_MASK		((0x1) << K3G_POWERDOWN_SHIFT)
#define K3G_Z_EN_SHIFT			2
#define K3G_Z_EN			((0x1) << K3G_Z_EN_SHIFT)
#define K3G_Y_EN_SHIFT			1
#define K3G_Y_EN			((0x1) << K3G_Y_EN_SHIFT)
#define K3G_X_EN_SHIFT			0
#define K3G_X_EN			((0x1) << K3G_X_EN_SHIFT)

#define K3G_HIGH_PASS_FILTER_SHIFT				4
#define K3G_HIGH_PASS_FILTER_NORMAL_MODE_RESET_READING	\
			((0x0) << K3G_HIGH_PASS_FILTER_SHIFT)
#define K3G_HIGH_PASS_FILTER_REFERENCE_SIGNAL	\
			((0x1) << K3G_HIGH_PASS_FILTER_SHIFT)
#define K3G_HIGH_PASS_FILTER_NORMAL_MODE	\
			((0x2) << K3G_HIGH_PASS_FILTER_SHIFT)
#define K3G_HIGH_PASS_FILTER_AUTORESET_ON_INTERRUPT	\
			((0x3) << K3G_HIGH_PASS_FILTER_SHIFT)
#define K3G_HIGH_PASS_FILTER_CUTOFF_FREQ_SHIFT			0
#define K3G_HIGH_PASS_FILTER_CUTOFF_FREQ_MASK	\
			((0xf) << K3G_HIGH_PASS_FILTER_CUTOFF_FREQ_SHIFT)

#define K3G_INT1_EN_SHIFT			7
#define K3G_INT1_EN				((0x1) << K3G_INT1_EN_SHIFT)
#define K3G_INT1_BOOT_SHIFT			6
#define K3G_INT1_BOOT				((0x1) << K3G_INT1_BOOT_SHIFT)
#define K3G_INT1_ACTIVE_SHIFT			5
#define K3G_INT1_ACTIVE_HIGH			((0x0) << K3G_INT1_ACTIVE_SHIFT)
#define K3G_INT1_ACTIVE_LOW			((0x1) << K3G_INT1_ACTIVE_SHIFT)
#define K3G_INT_PUSH_PULL_OPEN_DRAIN_SHIFT	4
#define K3G_INT_PUSH_PULL	\
			((0x0) << K3G_INT_PUSH_PULL_OPEN_DRAIN_SHIFT)
#define K3G_INT_OPEN_DRAIN	\
			((0x1) << K3G_INT_PUSH_PULL_OPEN_DRAIN_SHIFT)
#define K3G_INT2_SRC_SHIFT			0
#define K3G_INT2_SRC_MASK			((0x0f) << K3G_INT2_SRC_SHIFT)
#define K3G_INT2_DATA_READY_SHIFT		3
#define K3G_INT2_DATA_READY		((0x1) << K3G_INT2_DATA_READY_SHIFT)
#define K3G_INT2_WATERMARK_SHIFT		2
#define K3G_INT2_WATERMARK		((0x1) << K3G_INT2_WATERMARK_SHIFT)
#define K3G_INT2_OVERRUN_SHIFT			1
#define K3G_INT2_OVERRUN		((0x1) << K3G_INT2_OVERRUN_SHIFT)
#define K3G_INT2_EMPTY_SHIFT			0
#define K3G_INT2_EMPTY			((0x1) << K3G_INT2_EMPTY_SHIFT)

#define K3G_BLOCK_DATA_UPDATE_SHIFT		7
#define K3G_BLOCK_DATA_UPDATE	\
			((0x1) << K3G_BLOCK_DATA_UPDATE_SHIFT)
#define K3G_BIG_LITTLE_ENDIAN_SHIFT		6
#define K3G_BIG_ENDIAN			((0x0) << K3G_BIG_LITTLE_ENDIAN_SHIFT)
#define K3G_LITTLE_ENDIAN		((0x1) << K3G_BIG_LITTLE_ENDIAN_SHIFT)
#define K3G_FULL_SCALE_SHIFT			4
#define K3G_FULL_SCALE_250DPS			((0x0) << K3G_FULL_SCALE_SHIFT)
#define K3G_FULL_SCALE_500DPS			((0x1) << K3G_FULL_SCALE_SHIFT)
#define K3G_FULL_SCALE_2000DPS			((0x2) << K3G_FULL_SCALE_SHIFT)
#define K3G_SELF_TEST_SHIFT			1
#define K3G_SELF_TESET_NORMAL			((0x0) << K3G_SELF_TEST_SHIFT)
#define K3G_SELF_TESET_0			((0x1) << K3G_SELF_TEST_SHIFT)
#define K3G_SELF_TESET_1			((0x3) << K3G_SELF_TEST_SHIFT)
#define K3G_SPI_MODE_SHIFT			0
#define K3G_SPI_FOUR_WIRE			((0x0) << K3G_SPI_MODE_SHIFT)
#define K3G_SPI_THREE_WIRE			((0x1) << K3G_SPI_MODE_SHIFT)

#define K3G_REBOOT_SHIFT			7
#define K3G_REBOOT				((0x1) << K3G_REBOOT_SHIFT)
#define K3G_FIFO_EN_SHIFT			6
#define K3G_FIFO_EN				((0x1) << K3G_FIFO_EN_SHIFT)
#define K3G_HIGH_PASS_FILTER_EN_SHIFT		4
#define K3G_HIGH_PASS_FILTER_EN	\
			((0x1) << K3G_HIGH_PASS_FILTER_EN_SHIFT)
#define K3G_INT1_SELECTION_SHIFT		2
#define K3G_INT1_SELECTION		((0x3) << K3G_INT1_SELECTION_SHIFT)
#define K3G_OUT_SELECTION_SHIFT			0
#define K3G_OUT_SELECTION		((0x3) << K3G_OUT_SELECTION_SHIFT)

#define K3G_ZYX_OVERRUN_SHIFT			7
#define K3G_ZYX_OVERRUN				((0x1) << K3G_ZYX_OVERRUN_SHIFT)
#define K3G_Z_OVERRUN_SHIFT			6
#define K3G_Z_OVERRUN				((0x1) << K3G_Z_OVERRUN_SHIFT)
#define K3G_Y_OVERRUN_SHIFT			5
#define K3G_Y_OVERRUN				((0x1) << K3G_Y_OVERRUN_SHIFT)
#define K3G_X_OVERRUN_SHIFT			4
#define K3G_X_OVERRUN				((0x1) << K3G_X_OVERRUN_SHIFT)
#define K3G_ZYX_DATA_AVAILABLE_SHIFT		3
#define K3G_ZYX_DATA_AVAILABEL	\
			((0x1) << K3G_ZYX_DATA_AVAILABLE_SHIFT)
#define K3G_Z_DATA_AVAILABLE_SHIFT		2
#define K3G_Z_DATA_AVAILABLE		((0x1) << K3G_Z_DATA_AVAILABLE_SHIFT)
#define K3G_Y_DATA_AVAILABLE_SHIFT		1
#define K3G_Y_DATA_AVAILABLE		((0x1) << K3G_Y_DATA_AVAILABLE_SHIFT)
#define K3G_X_DATA_AVAILABLE_SHIFT		0
#define K3G_X_DATA_AVAILABLE		((0x1) << K3G_X_DATA_AVAILABLE_SHIFT)

#define K3G_FIFO_MODE_SHIFT			5
#define K3G_FIFO_BYPASS_MODE			((0x0) << K3G_FIFO_MODE_SHIFT)
#define K3G_FIFO_FIFO_MODE			((0x1) << K3G_FIFO_MODE_SHIFT)
#define K3G_FIFO_STREAM_MODE			((0x2) << K3G_FIFO_MODE_SHIFT)
#define K3G_FIFO_STREAM_TO_FIFO_MODE		((0x3) << K3G_FIFO_MODE_SHIFT)
#define K3G_FIFO_BYPASS_TO_STREAM_MODE		((0x4) << K3G_FIFO_MODE_SHIFT)
#define K3G_FIFO_MODE_MASK			((0x7) << K3G_FIFO_MODE_SHIFT)
#define K3G_WATERMARK_THRES_SHIFT		0
#define K3G_WATERMARK_THRES_MASK	((0x1f) << K3G_WATERMARK_THRES_SHIFT)

#define K3G_WATERMARK_SHIFT			7
#define K3G_WATERMARK				((0x1) << K3G_WATERMARK_SHIFT)
#define K3G_OVERRUN_SHIFT			6
#define K3G_OVERRUN				((0x1) << K3G_OVERRUN_SHIFT)
#define K3G_EMPTY_SHIFT				5
#define K3G_EMPTY				((0x1) << K3G_EMPTY_SHIFT)
#define K3G_FIFO_STORED_SHIFT			0
#define K3G_FIFO_STORED_MASK		((0x1f) << K3G_FIFO_STORED_SHIFT)

#define K3G_AND_OR_COMBINATION_SHIFT		7
#define K3G_OR_COMBINATION		((0x0) << K3G_AND_OR_COMBINATION_SHIFT)
#define K3G_AND_COMBINATION		((0x1) << K3G_AND_OR_COMBINATION_SHIFT)
#define K3G_LATCH_INTERRUPT_SHIFT		6
#define K3G_INTERRUPT_NO_LATCHED	((0x0) << K3G_LATCH_INTERRUPT_SHIFT)
#define K3G_INTERRUPT_LATCHED		((0x1) << K3G_LATCH_INTERRUPT_SHIFT)
#define K3G_Z_HIGH_INT_EN_SHIFT			5
#define K3G_Z_HIGH_INT_EN		((0x1) << K3G_Z_HIGH_INT_EN_SHIFT)
#define K3G_Z_LOW_INT_EN_SHIFT			4
#define K3G_Z_LOW_INT_EN		((0x1) << K3G_Z_LOW_INT_EN_SHIFT)
#define K3G_Y_HIGH_INT_EN_SHIFT			3
#define K3G_Y_HIGH_INT_EN		((0x1) << K3G_Y_HIGH_INT_EN_SHIFT)
#define K3G_Y_LOW_INT_EN_SHIFT			2
#define K3G_Y_LOW_INT_EN		((0x1) << K3G_Y_LOW_INT_EN_SHIFT)
#define K3G_X_HIGH_INT_EN_SHIFT			1
#define K3G_X_HIGH_INT_EN		((0x1) << K3G_X_HIGH_INT_EN_SHIFT)
#define K3G_X_LOW_INT_EN_SHIFT			0
#define K3G_X_LOW_INT_EN		((0x1) << K3G_X_LOW_INT_EN_SHIFT)

#define K3G_INTERRUPT_ACTIVE_SHIFT		6
#define K3G_INTERRUPT_ACTIVE		((0x1) << K3G_INTERRUPT_ACTIVE_SHIFT)
#define K3G_Z_HIGH_SHIFT			5
#define K3G_Z_HIGH				((0x1) << K3G_Z_HIGH_SHIFT)
#define K3G_Z_LOW_SHIFT				4
#define K3G_Z_LOW				((0x1) << K3G_Z_LOW_SHIFT)
#define K3G_Y_HIGH_SHIFT			3
#define K3G_Y_HIGH				((0x1) << K3G_Y_HIGH_SHIFT)
#define K3G_Y_LOW_SHIFT				2
#define K3G_Y_LOW				((0x1) << K3G_Y_LOW_SHIFT)
#define K3G_X_HIGH_SHIFT			1
#define K3G_X_HIGH				((0x1) << K3G_X_HIGH_SHIFT)
#define K3G_X_LOW_SHIFT				0
#define K3G_X_LOW				((0x1) << K3G_X_LOW_SHIFT)

#define K3G_INT1_WAIT_EN_SHIFT			7
#define K3G_INT1_WAIT_EN		((0x1) << K3G_INT1_WAIT_EN_SHIFT)
#define K3G_INT1_DURATION_SHIFT			0
#define K3G_INT1_DURATION_MASK		((0x7f) << K3G_INT1_DURATION_SHIFT)

#define K3G_TEMP_SHIFT				0
#define K3G_TEMP_MASK				((0xff) << K3G_TEMP_SHIFT)

#define K3G_DEV_ID				0xd3

struct k3g_platform_data {
	int irq2;
	u8 data_rate;
	u8 bandwidth;
	u8 powerdown;
	u8 zen;
	u8 yen;
	u8 xen;
	u8 hpmode;
	u8 hpcf;
	u8 int1_enable;
	u8 int1_boot;
	u8 int1_hl_active;
	u8 int_pp_od;
	u8 int2_src;
	u8 block_data_update;
	u8 endian;
	u8 fullscale;
	u8 selftest;
	u8 spi_mode;
	u8 reboot;
	u8 fifo_enable;
	u8 hp_enable;
	u8 int1_sel;
	u8 out_sel;
	u8 fifo_mode;
	u8 fifo_threshold;
	u8 int1_combination;
	u8 int1_latch;
	u8 int1_z_high_enable;
	u8 int1_z_low_enable;
	u8 int1_y_high_enable;
	u8 int1_y_low_enable;
	u8 int1_x_high_enable;
	u8 int1_x_low_enable;
	u8 int1_wait_enable;
	u8 int1_wait_duration;
	u16 int1_z_threshold;
	u16 int1_y_threshold;
	u16 int1_x_threshold;
};

struct k3g_chip {
	struct i2c_client		*client;
	struct iio_dev			*indio_dev;
	struct work_struct		work_thresh;
	struct work_struct		work_fifo;
	struct iio_trigger		*trig;
	s64				last_timestamp;
	struct mutex			lock;
	struct mutex			ring_lock;
	struct k3g_platform_data	*pdata;
};

#endif
