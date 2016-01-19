/*
 * Copyright (c) 2010-2011 Yamaha Corporation
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef __YAS_H__
#define __YAS_H__

#include "yas_cfg.h"

#define YAS_VERSION                        "6.0.2"

/* ------------------ */
/*  Typedef definition    */
/* ------------------- */

#include <linux/types.h>


/* -------------------------- */
/*  Macro definition                        */
/* ------------------------- */

/* Debugging */
#define DEBUG                               (0)

#if DEBUG
#include <linux/kernel.h>
#define YLOGD(args) (printk args)
#define YLOGI(args) (printk args)
#define YLOGE(args) (printk args)
#define YLOGW(args) (printk args)
#else /* DEBUG */
#define YLOGD(args)
#define YLOGI(args)
#define YLOGW(args)
#define YLOGE(args)
#endif /* DEBUG */

#define YAS_REPORT_DATA                     (0x01)
#define YAS_REPORT_CALIB                    (0x02)
#define YAS_REPORT_OVERFLOW_OCCURED         (0x04)
#define YAS_REPORT_HARD_OFFSET_CHANGED      (0x08)
#define YAS_REPORT_CALIB_OFFSET_CHANGED     (0x10)

#define YAS_HARD_OFFSET_UNKNOWN             (0x7f)
#define YAS_CALIB_OFFSET_UNKNOWN            (0x7fffffff)

#define YAS_NO_ERROR                        (0)
#define YAS_ERROR_ARG                       (-1)
#define YAS_ERROR_NOT_INITIALIZED           (-2)
#define YAS_ERROR_BUSY                      (-3)
#define YAS_ERROR_DEVICE_COMMUNICATION      (-4)
#define YAS_ERROR_CHIP_ID                   (-5)
#define YAS_ERROR_NOT_ACTIVE                (-6)
#define YAS_ERROR_RESTARTSYS                (-7)
#define YAS_ERROR_HARDOFFSET_NOT_WRITTEN    (-8)
#define YAS_ERROR_INTERRUPT                 (-9)
#define YAS_ERROR_ERROR                     (-128)

enum cable_type_t {
	CABLE_TYPE_NONE = 0,
	CABLE_TYPE_USB,
	CABLE_TYPE_AC,
};

#ifndef NULL
#define NULL ((void *)(0))
#endif
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (!(0))
#endif
#ifndef NELEMS
#define NELEMS(a) ((int)(sizeof(a)/sizeof(a[0])))
#endif
#ifndef ABS
#define ABS(a) ((a) > 0 ? (a) : -(a))
#endif
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

enum {
	YAS532_POSITION_0 = 1,
	YAS532_POSITION_1,
	YAS532_POSITION_2,
	YAS532_POSITION_3,
	YAS532_POSITION_4,
	YAS532_POSITION_5,
	YAS532_POSITION_6,
	YAS532_POSITION_7,
};

#define YAS532_POSITION_OFFSET	1

/* -------------------------------------------------------------------------- */
/*  Structure definition                                                      */
/* -------------------------------------------------------------------------- */

struct acc_platform_data {
	const char *cal_path;
	void (*ldo_on) (bool);
	int orientation;
};
struct accel_cal {
	s16 v[3];
};
struct yas_mag_filter {
	int len;
	int noise[3];
	int threshold;		/* nT */
};
struct yas_vector {
	int32_t v[3];
};

struct mag_platform_data {
	void (*power_on) (bool);
	int offset_enable;
	int chg_status;
	struct yas_vector ta_offset;
	struct yas_vector usb_offset;
	struct yas_vector full_offset;
	int orientation;
};

struct yas_matrix {
	int32_t matrix[9];
};
struct yas_acc_data {
	struct yas_vector xyz;
	struct yas_vector raw;
};
struct yas_gyro_data {
	struct yas_vector xyz;
	struct yas_vector raw;
	int overrun;
	int num;
};
struct yas_mag_data {
	struct yas_vector xyz;	/* without offset, filtered */
	struct yas_vector raw;	/* with offset, not filtered */
	struct yas_vector xy1y2;
	int16_t temperature;
};

struct yas_mag_offset {
	int8_t hard_offset[3];
	struct yas_vector calib_offset;
};
struct yas_mag_status {
	struct yas_mag_offset offset;
	int accuracy;
	struct yas_matrix dynamic_matrix;
};
struct yas_offset {
	struct yas_mag_status mag[YAS_MAGCALIB_SHAPE_NUM];
};

struct yas_mag_driver_callback {
	int (*lock) (void);
	int (*unlock) (void);
	int (*device_open) (void);
	int (*device_close) (void);
#if YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS529
	int (*device_write) (const uint8_t *buf, int len);
	int (*device_read) (uint8_t *buf, int len);
#else
	int (*device_write) (uint8_t addr, const uint8_t *buf, int len);
	int (*device_read) (uint8_t addr, uint8_t *buf, int len);
#endif
	void (*msleep) (int msec);
	void (*current_time) (int32_t *sec, int32_t *msec);
};

struct yas_mag_driver {
	int (*init) (void);
	int (*term) (void);
	int (*get_delay) (void);
	int (*set_delay) (int msec);
	int (*get_offset) (struct yas_mag_offset *offset);
	int (*set_offset) (struct yas_mag_offset *offset);
#ifdef YAS_MAG_MANUAL_OFFSET
	int (*get_manual_offset) (struct yas_vector *offset);
	int (*set_manual_offset) (struct yas_vector *offset);
#endif
	int (*get_static_matrix) (struct yas_matrix *static_matrix);
	int (*set_static_matrix) (struct yas_matrix *static_matrix);
	int (*get_dynamic_matrix) (struct yas_matrix *dynamic_matrix);
	int (*set_dynamic_matrix) (struct yas_matrix *dynamic_matrix);
	int (*get_enable) (void);
	int (*set_enable) (int enable);
	int (*get_filter) (struct yas_mag_filter *filter);
	int (*set_filter) (struct yas_mag_filter *filter);
	int (*get_filter_enable) (void);
	int (*set_filter_enable) (int enable);
	int (*get_position) (void);
	int (*set_position) (int position);
#if YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS529
	int (*read_reg) (uint8_t *buf, int len);
	int (*write_reg) (const uint8_t *buf, int len);
#else
	int (*read_reg) (uint8_t addr, uint8_t *buf, int len);
	int (*write_reg) (uint8_t addr, const uint8_t *buf, int len);
#endif
	int (*measure) (struct yas_mag_data *data, int *time_delay_ms);
	struct yas_mag_driver_callback callback;
};

struct yas_mag_calibration_result {
	int32_t spread;
	int32_t variation;
	int32_t radius;
	int8_t axis;
	int8_t level;
	int8_t accuracy;
	struct yas_matrix dynamic_matrix;
};

struct yas_mag_calibration_threshold {
	int32_t spread;
	int32_t variation[3];
};

struct yas_mag_calibration_callback {
	int (*lock) (void);
	int (*unlock) (void);
};

#define YAS_MAGCALIB_MODE_SPHERE        (0)
#define YAS_MAGCALIB_MODE_ELLIPSOID     (1)

struct yas_mag_calibration {
	int (*init) (void);
	int (*term) (void);
	int (*update) (struct yas_vector *mag,
		       struct yas_mag_calibration_result *result);
	int (*get_accuracy) (void);
	int (*set_accuracy) (int accuracy);
	int (*get_offset) (struct yas_vector *offset);
	int (*set_offset) (struct yas_vector *offset);
	int (*get_shape) (void);
	int (*set_shape) (int shape);
	int (*get_threshold) (struct yas_mag_calibration_threshold *threshold);
	int (*set_threshold) (struct yas_mag_calibration_threshold *threshold);
	int (*get_mode) (void);
	int (*set_mode) (int mode);
	int (*get_max_sample) (void);
	int (*set_max_sample) (int num_samples);
	int (*get_dynamic_matrix) (struct yas_matrix *dynamic_matrix);
	struct yas_mag_calibration_callback callback;
};

#if YAS_SUPPORT_FUSION_DRIVER

struct yas_quaternion {
	int32_t q[4];
};

struct yas_fusion_callback {
	int (*lock) (void);
	int (*unlock) (void);
	void (*current_time) (int32_t *sec, int32_t *msec);
};

struct yas_fusion {
	int (*init) (void);
	int (*term) (void);
	int (*update) (struct yas_vector *acc, struct yas_vector *mag,
		       struct yas_vector *gyro);
	int (*get_quaternion) (struct yas_quaternion *quaternion);
	int (*get_fusion) (struct yas_quaternion *quaternion,
			   struct yas_vector *acc, struct yas_vector *gravity,
			   struct yas_vector *linear_acceleration,
			   struct yas_vector *rotation_vector);
	struct yas_fusion_callback callback;
};
#endif

#if YAS_SUPPORT_SOFTWARE_GYROSCOPE

struct yas_swgyro_callback {
	int (*lock) (void);
	int (*unlock) (void);
};

struct yas_swgyro {
	int (*init) (void);
	int (*term) (void);
	int (*get_delay) (void);
	int (*set_delay) (int msec);
	int (*update) (struct yas_vector *acc, struct yas_vector *mag,
		       struct yas_vector *gyro);
	struct yas_swgyro_callback callback;
};

#endif

struct yas_acc_filter {
	int threshold;		/* um/s^2 */
};

struct yas_acc_driver_callback {
	int (*lock) (void);
	int (*unlock) (void);
	int (*device_open) (void);
	int (*device_close) (void);
	int (*device_write) (uint8_t adr, const uint8_t *buf, int len);
	int (*device_read) (uint8_t adr, uint8_t *buf, int len);
	void (*msleep) (int msec);
};

struct yas_acc_driver {
	int (*init) (void);
	int (*term) (void);
	int (*get_delay) (void);
	int (*set_delay) (int delay);
	int (*get_offset) (struct yas_vector *offset);
	int (*set_offset) (struct yas_vector *offset);
	int (*get_enable) (void);
	int (*set_enable) (int enable);
	int (*get_filter) (struct yas_acc_filter *filter);
	int (*set_filter) (struct yas_acc_filter *filter);
	int (*get_filter_enable) (void);
	int (*set_filter_enable) (int enable);
	int (*get_position) (void);
	int (*set_position) (int position);
	int (*measure) (struct yas_acc_data *data);
	void (*set_motion_interrupt)(bool enable, bool factorytest);
	int (*get_motion_interrupt)(void);
#if DEBUG
	int (*get_register) (uint8_t adr, uint8_t *val);
#endif
	struct yas_acc_driver_callback callback;
};

struct yas_acc_calibration_threshold {
	int32_t variation;
};

struct yas_acc_calibration_callback {
	int (*lock) (void);
	int (*unlock) (void);
};

struct yas_acc_calibration {
	int (*init) (void);
	int (*term) (void);
	int (*update) (struct yas_vector *acc);
	int (*get_offset) (struct yas_vector *offset);
	int (*get_threshold) (struct yas_acc_calibration_threshold *threshold);
	int (*set_threshold) (struct yas_acc_calibration_threshold *threshold);
	struct yas_acc_calibration_callback callback;
};

struct yas_gyro_filter {
	int threshold;		/*mdegree/s */
};

struct yas_gyro_driver_callback {
	int (*lock) (void);
	int (*unlock) (void);
	int (*device_open) (void);
	int (*device_close) (void);
	int (*device_write) (uint8_t adr, const uint8_t *buf, int len);
	int (*device_read) (uint8_t adr, uint8_t *buf, int len);
	int (*interrupt_enable) (void);
	int (*interrupt_disable) (void);
	void (*interrupt_notify) (int num);
	void (*msleep) (int msec);
};

struct yas_gyro_driver {
	int (*init) (void);
	int (*term) (void);
	int (*get_delay) (void);
	int (*set_delay) (int delay);
	int (*get_offset) (struct yas_vector *offset);
	int (*set_offset) (struct yas_vector *offset);
	int (*get_enable) (void);
	int (*set_enable) (int enable);
	int (*get_filter) (struct yas_gyro_filter *filter);
	int (*set_filter) (struct yas_gyro_filter *filter);
	int (*get_filter_enable) (void);
	int (*set_filter_enable) (int enable);
	int (*get_position) (void);
	int (*set_position) (int position);
	int (*get_interrupt) (void);
	int (*set_interrupt) (int interrupt);
	int (*measure) (struct yas_gyro_data *data, int num);
	void (*interrupt_handler) (void);
#if DEBUG
	int (*get_register) (uint8_t adr, uint8_t *val);
	int (*set_register) (uint8_t adr, uint8_t val);
#endif
	struct yas_gyro_driver_callback callback;
};

struct yas_gyro_calibration_threshold {
	int32_t variation;
};

struct yas_gyro_calibration_callback {
	int (*lock) (void);
	int (*unlock) (void);
};

struct yas_gyro_calibration {
	int (*init) (void);
	int (*term) (void);
	int (*update) (struct yas_vector *gyro);
	int (*get_offset) (struct yas_vector *offset);
	int (*get_threshold) (struct yas_gyro_calibration_threshold *
			      threshold);
	int (*set_threshold) (struct yas_gyro_calibration_threshold *
			      threshold);
	struct yas_gyro_calibration_callback callback;
};

struct yas_utility {
	int (*get_rotation_matrix) (struct yas_vector *acc,
				    struct yas_vector *mag,
				    struct yas_matrix *rotation_matrix);
	int (*get_euler) (struct yas_matrix *rotation_matrix,
			  struct yas_vector *euler);
};

/*-------------------------------------------------------------------------- */
/* Global function definition                                                */
/*-------------------------------------------------------------------------- */

int yas_mag_driver_init(struct yas_mag_driver *f);
int yas_mag_calibration_init(struct yas_mag_calibration *f);
int yas_acc_driver_init(struct yas_acc_driver *f);
int yas_acc_calibration_init(struct yas_acc_calibration *f);
int yas_gyro_driver_init(struct yas_gyro_driver *f, int interrupt);
int yas_gyro_calibration_init(struct yas_gyro_calibration *f);
int yas_utility_init(struct yas_utility *f);
#if YAS_SUPPORT_FUSION_DRIVER
int yas_fusion_init(struct yas_fusion *f);
#endif
#if YAS_SUPPORT_SOFTWARE_GYROSCOPE
int yas_swgyro_init(struct yas_swgyro *f);
#endif

#endif /*__YAS_H__ */
