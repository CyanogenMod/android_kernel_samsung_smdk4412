/*
 *  Copyright (C) 2011, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifndef __SSP_PRJ_H__
#define __SSP_PRJ_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/wakelock.h>
#include <linux/miscdevice.h>
#include <linux/ssp_platformdata.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/timer.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#undef CONFIG_HAS_EARLYSUSPEND
#endif

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
#include "ssp_sensorhub.h"
#endif

#define SSP_DBG		1

#define SUCCESS		1
#define FAIL		0
#define ERROR		-1

/*
 1. ACCEL : 6 Bytes
 2. GYRO : 6 Bytes
 3. MAGNETIC : 6 Bytes
 4. PRESSURE : 5 Bytes
 5. GESTURE : 4 Bytes
 6. PROXIMITY : 2 Bytes
 7. TEMPHUMI : 5 Bytes
 8. LIGHT : 8 Bytes
 SUM : 42 Bytes
 Total : 42 + 8(index) = 50 Bytes
*/
#define FACTORY_DATA_MAX	63
#if SSP_DBG
#define SSP_FUNC_DBG 1
#define SSP_DATA_DBG 0

#define ssp_dbg(dev, format, ...) do { \
	printk(KERN_INFO dev, format, ##__VA_ARGS__); \
	} while (0)
#else
#define ssp_dbg(dev, format, ...)
#endif

#if SSP_FUNC_DBG
#define func_dbg() do { \
	printk(KERN_INFO "[SSP]: %s\n", __func__); \
	} while (0)
#else
#define func_dbg()
#endif

#if SSP_DATA_DBG
#define data_dbg(dev, format, ...) do { \
	printk(KERN_INFO dev, format, ##__VA_ARGS__); \
	} while (0)
#else
#define data_dbg(dev, format, ...)
#endif

#define SSP_SW_RESET_TIME	3000
#define DEFUALT_POLLING_DELAY	(200 * NSEC_PER_MSEC)
#define PROX_AVG_READ_NUM	80
#define DEFAULT_RETRIES		3

/* SSP Binary Type */
enum {
	KERNEL_BINARY = 0,
	KERNEL_CRASHED_BINARY,
	UMS_BINARY,
};

/* Sensor Sampling Time Define */
enum {
	SENSOR_NS_DELAY_FASTEST = 10000000,	/* 10msec */
	SENSOR_NS_DELAY_GAME = 20000000,	/* 20msec */
	SENSOR_NS_DELAY_UI = 66700000,		/* 66.7msec */
	SENSOR_NS_DELAY_NORMAL = 200000000,	/* 200msec */
};

enum {
	SENSOR_MS_DELAY_FASTEST = 10,	/* 10msec */
	SENSOR_MS_DELAY_GAME = 20,	/* 20msec */
	SENSOR_MS_DELAY_UI = 66,	/* 66.7msec */
	SENSOR_MS_DELAY_NORMAL = 200,	/* 200msec */
};

enum {
	SENSOR_CMD_DELAY_FASTEST = 0,	/* 10msec */
	SENSOR_CMD_DELAY_GAME,		/* 20msec */
	SENSOR_CMD_DELAY_UI,		/* 66.7msec */
	SENSOR_CMD_DELAY_NORMAL,	/* 200msec */
};

/*
 * SENSOR_DELAY_SET_STATE
 * Check delay set to avoid sending ADD instruction twice
 */
enum {
	INITIALIZATION_STATE = 0,
	NO_SENSOR_STATE,
	ADD_SENSOR_STATE,
	RUNNING_SENSOR_STATE,
};

/* Firmware download STATE */
enum {
	FW_DL_STATE_FAIL = -1,
	FW_DL_STATE_NONE = 0,
	FW_DL_STATE_NEED_TO_SCHEDULE,
	FW_DL_STATE_SCHEDULED,
	FW_DL_STATE_DOWNLOADING,
	FW_DL_STATE_SYNC,
	FW_DL_STATE_DONE,
};

#define SSP_INVALID_REVISION		99999

/* Gyroscope DPS */
#define GYROSCOPE_DPS250		250
#define GYROSCOPE_DPS500		500
#define GYROSCOPE_DPS2000		2000

/* Gesture Sensor Current */
#define DEFUALT_IR_CURRENT    400 //0xF2

/* kernel -> ssp manager cmd*/
#define SSP_LIBRARY_SLEEP_CMD		(1 << 5)
#define SSP_LIBRARY_LARGE_DATA_CMD	(1 << 6)
#define SSP_LIBRARY_WAKEUP_CMD		(1 << 7)

/* ioctl command */
#define AKMIO				0xA1
#define ECS_IOCTL_GET_FUSEROMDATA	_IOR(AKMIO, 0x01, unsigned char[3])
#define ECS_IOCTL_GET_MAGDATA	        _IOR(AKMIO, 0x02, unsigned char[8])
#define ECS_IOCTL_GET_ACCDATA	        _IOR(AKMIO, 0x03, int[3])

/* AP -> SSP Instruction */
#define MSG2SSP_INST_BYPASS_SENSOR_ADD		0xA1
#define MSG2SSP_INST_BYPASS_SENSOR_REMOVE	0xA2
#define MSG2SSP_INST_REMOVE_ALL			0xA3
#define MSG2SSP_INST_CHANGE_DELAY		0xA4
#define MSG2SSP_INST_SENSOR_SELFTEST		0xA8
#define MSG2SSP_INST_LIBRARY_ADD		0xB1
#define MSG2SSP_INST_LIBRARY_REMOVE		0xB2
#define MSG2SSP_INST_LIB_NOTI			0xB4

#define MSG2SSP_AP_STT				0xC8
#define MSG2SSP_AP_STATUS_WAKEUP		0xD1
#define MSG2SSP_AP_STATUS_SLEEP			0xD2
#define MSG2SSP_AP_STATUS_RESUME		0xD3
#define MSG2SSP_AP_STATUS_SUSPEND		0xD4
#define MSG2SSP_AP_STATUS_RESET			0xD5
#define MSG2SSP_AP_STATUS_POW_CONNECTED		0xD6
#define MSG2SSP_AP_STATUS_POW_DISCONNECTED	0xD7
#define MSG2SSP_AP_STATUS_CALL_IDLE		0xD8
#define MSG2SSP_AP_STATUS_CALL_ACTIVE		0xD9
#define MSG2SSP_AP_TEMPHUMIDITY_CAL_DONE	0xDA

#define MSG2SSP_AP_WHOAMI			0x0F
#define MSG2SSP_AP_FIRMWARE_REV			0xF0
#define MSG2SSP_AP_SENSOR_FORMATION		0xF1
#define MSG2SSP_AP_SENSOR_PROXTHRESHOLD		0xF2
#define MSG2SSP_AP_SENSOR_BARCODE_EMUL		0xF3
#define MSG2SSP_AP_SENSOR_SCANNING		0xF4
#define MSG2SSP_AP_SET_MAGNETIC_HWOFFSET	0xF5
#define MSG2SSP_AP_GET_MAGNETIC_HWOFFSET	0xF6
#define MSG2SSP_AP_SENSOR_GESTURE_CURRENT	0xF7

#define MSG2SSP_AP_FUSEROM			0X01

/* AP -> SSP Data Protocol Frame Field */
#define MSG2SSP_SSP_SLEEP	0xC1
#define MSG2SSP_STS		0xC2	/* Start to Send */
#define MSG2SSP_RTS		0xC4	/* Ready to Send */
#define MSG2SSP_STT		0xC8
#define MSG2SSP_SRM		0xCA	/* Start to Read MSG */
#define MSG2SSP_SSM		0xCB	/* Start to Send MSG */
#define MSG2SSP_SSD		0xCE	/* Start to Send Data Type & Length */
#define MSG2SSP_NO_DATA		0xCF	/* There is no data to get from MCU */

/* SSP -> AP ACK about write CMD */
#define MSG_ACK					0x80	/* ACK from SSP to AP */
#define MSG_NAK					0x70	/* NAK from SSP to AP */

/* Accelerometer sensor*/
/* 16bits */
#define MAX_ACCEL_1G		16384
#define MAX_ACCEL_2G		32767
#define MIN_ACCEL_2G		-32768
#define MAX_ACCEL_4G		65536

#define MAX_GYRO		32767
#define MIN_GYRO		-32768

/* sensor combination, data->sns_combination */
enum {
	INVENSENSE_MPU6500_AG = 0,
	STM_K330_AG,
};

/* SSP_INSTRUCTION_CMD */
enum {
	REMOVE_SENSOR = 0,
	ADD_SENSOR,
	CHANGE_DELAY,
	GO_SLEEP,
	FACTORY_MODE,
	REMOVE_LIBRARY,
	ADD_LIBRARY,
};

/* SENSOR_TYPE */
enum {
	ACCELEROMETER_SENSOR = 0,
	GYROSCOPE_SENSOR,
	GEOMAGNETIC_SENSOR,
	PRESSURE_SENSOR,
	GESTURE_SENSOR,
	PROXIMITY_SENSOR,
	TEMPERATURE_HUMIDITY_SENSOR,
	LIGHT_SENSOR,
	PROXIMITY_RAW,
	GEOMAGNETIC_RAW,
	ORIENTATION_SENSOR,
	SENSOR_MAX,
};

#define	SSP_BYPASS_SENSORS_EN_ALL	(1 << ACCELEROMETER_SENSOR |\
	1 << GYROSCOPE_SENSOR | 1 << GEOMAGNETIC_SENSOR | 1 << PRESSURE_SENSOR |\
	1 << TEMPERATURE_HUMIDITY_SENSOR | 1 << LIGHT_SENSOR)
	/* Proximity sensor is not continuous */

/* SENSOR_FACTORY_MODE_TYPE */
enum {
	ACCELEROMETER_FACTORY = 0,
	GYROSCOPE_FACTORY,
	GEOMAGNETIC_FACTORY,
	PRESSURE_FACTORY,
	MCU_FACTORY,
	GYROSCOPE_TEMP_FACTORY,
	GYROSCOPE_DPS_FACTORY,
	MCU_SLEEP_FACTORY,
	GESTURE_FACTORY,
	TEMPHUMIDITY_CRC_FACTORY,
	SENSOR_FACTORY_MAX,
};

enum {
	SHTC1_CMD_NONE,
	SHTC1_CMD_RESET,
};

struct sensor_value {
	union {
		struct {
			s16 x;
			s16 y;
			s16 z;
		};
		struct {
			u16 r;
			u16 g;
			u16 b;
			u16 w;
		};
		u8 prox[4];
		s16 data[9];
		s32 pressure[3];
	};
};

extern struct class *sensors_event_class;

struct calibraion_data {
	s16 x;
	s16 y;
	s16 z;
};

struct hw_offset_data {
	char x;
	char y;
	char z;
};

struct ssp_data {
	struct input_dev *acc_input_dev;
	struct input_dev *gyro_input_dev;
	struct input_dev *mag_input_dev;
	struct input_dev *gesture_input_dev;
	struct input_dev *pressure_input_dev;
	struct input_dev *light_input_dev;
	struct input_dev *prox_input_dev;
	struct input_dev *temp_humi_input_dev;

	struct device *mcu_device;
	struct device *acc_device;
	struct device *gyro_device;
	struct device *mag_device;
	struct device *prs_device;
	struct device *prox_device;
	struct device *light_device;
	struct device *ges_device;
	struct device *temphumidity_device;

#ifdef CONFIG_SENSORS_SSP_STM
	struct spi_device *spi;
#endif

	struct wake_lock ssp_wake_lock;
	struct miscdevice akmd_device;
	struct timer_list debug_timer;
	struct workqueue_struct *debug_wq;
	struct delayed_work work_firmware;
	struct work_struct work_debug;
	struct calibraion_data accelcal;
	struct calibraion_data gyrocal;
	struct hw_offset_data magoffset;
	struct sensor_value buf[SENSOR_MAX];

	bool bSspShutdown;
	bool bCheckSuspend;
	bool bDebugEnabled;
	bool bMcuIRQTestSuccessed;
	bool bAccelAlert;
	bool bProximityRawEnabled;
	bool bGeomagneticRawEnabled;
	bool bBarcodeEnabled;
	bool bBinaryChashed;
	bool bProbeIsDone;

	unsigned char uProxCanc;
	unsigned char uProxHiThresh;
	unsigned char uProxLoThresh;
	unsigned char uProxHiThresh_default;
	unsigned char uProxLoThresh_default;
	unsigned int uIr_Current;
	unsigned char uFuseRomData[3];
	unsigned char uFactorydata[FACTORY_DATA_MAX];
	char *pchLibraryBuf;
	char chLcdLdi[2];
	int iIrq;
	int iLibraryLength;
	int aiCheckStatus[SENSOR_MAX];

	unsigned int uIrqFailCnt;
	unsigned int uSsdFailCnt;
	unsigned int uResetCnt;
	unsigned int uInstFailCnt;
	unsigned int uTimeOutCnt;
	unsigned int uIrqCnt;
	unsigned int uBusyCnt;
	unsigned int uMissSensorCnt;

	unsigned int uGyroDps;
	unsigned int uSensorState;
	unsigned int uCurFirmRev;
	unsigned int uFactoryProxAvg[4];
	unsigned int uFactorydataReady;
	s32 iPressureCal;
	unsigned int uTempCount;

	atomic_t aSensorEnable;
	int64_t adDelayBuf[SENSOR_MAX];

	int (*wakeup_mcu)(void);
	int (*check_mcu_ready)(void);
	int (*check_mcu_busy)(void);
	int (*set_mcu_reset)(int);
	int (*read_chg)(void);
	void (*get_sensor_data[SENSOR_MAX])(char *, int *,
		struct sensor_value *);
	void (*report_sensor_data[SENSOR_MAX])(struct ssp_data *,
		struct sensor_value *);

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	struct ssp_sensorhub_data *hub_data;
#endif
	int ap_rev;
	int ssp_changes;
	int accel_position;
	int mag_position;
	int fw_dl_state;
#ifdef CONFIG_SENSORS_SSP_SHTC1
	char *comp_engine_ver;
	struct platform_device *pdev_pam_temp;
	struct s3c_adc_client *adc_client;
	u8 cp_thm_adc_channel;
	u8 cp_thm_adc_arr_size;
	struct cp_thm_adc_table *cp_thm_adc_table;
	struct mutex cp_temp_adc_lock;
#endif
	u8 comp_engine_cmd;
	struct mutex comm_mutex;
};

void ssp_enable(struct ssp_data *, bool);
int waiting_init_mcu(struct ssp_data *);
int waiting_wakeup_mcu(struct ssp_data *);
int ssp_read_data(struct ssp_data *, char *, u16, char *, u16, int);
void toggle_mcu_reset(struct ssp_data *);
int initialize_mcu(struct ssp_data *);
int initialize_input_dev(struct ssp_data *);
int initialize_sysfs(struct ssp_data *);
void initialize_accel_factorytest(struct ssp_data *);
void initialize_prox_factorytest(struct ssp_data *);
void initialize_light_factorytest(struct ssp_data *);
void initialize_gyro_factorytest(struct ssp_data *);
void initialize_pressure_factorytest(struct ssp_data *);
void initialize_magnetic_factorytest(struct ssp_data *);
void initialize_function_pointer(struct ssp_data *);
void initialize_magnetic(struct ssp_data *);
void initialize_gesture_factorytest(struct ssp_data *data);
void initialize_temphumidity_factorytest(struct ssp_data *data);
void remove_accel_factorytest(struct ssp_data *);
void remove_gyro_factorytest(struct ssp_data *);
void remove_prox_factorytest(struct ssp_data *);
void remove_light_factorytest(struct ssp_data *);
void remove_pressure_factorytest(struct ssp_data *);
void remove_magnetic_factorytest(struct ssp_data *);
void remove_gesture_factorytest(struct ssp_data *data);
void remove_temphumidity_factorytest(struct ssp_data *data);
void remove_magnetic(struct ssp_data *data);
void sensors_remove_symlink(struct kobject *target,
		      const char *name);
void destroy_sensor_class(void);
int initialize_event_symlink(struct ssp_data *);
int sensors_create_symlink(struct kobject *target,
		      const char *name);
int accel_open_calibration(struct ssp_data *);
int gyro_open_calibration(struct ssp_data *);
int pressure_open_calibration(struct ssp_data *);
int proximity_open_calibration(struct ssp_data *);
int check_fwbl(struct ssp_data *);
void remove_input_dev(struct ssp_data *);
void remove_sysfs(struct ssp_data *);
void remove_event_symlink(struct ssp_data *);
int ssp_sleep_mode(struct ssp_data *);
int ssp_resume_mode(struct ssp_data *);
int ssp_ap_suspend(struct ssp_data *);
int ssp_ap_resume(struct ssp_data *);
int send_instruction(struct ssp_data *, u8, u8, u8 *, u8);
int ssp_send_cmd(struct ssp_data *, char);
int select_irq_msg(struct ssp_data *);
int get_chipid(struct ssp_data *);
int get_fuserom_data(struct ssp_data *);
int mag_open_hwoffset(struct ssp_data *);
int mag_store_hwoffset(struct ssp_data *);
int set_hw_offset(struct ssp_data *);
int get_hw_offset(struct ssp_data *);
int set_sensor_position(struct ssp_data *);
void sync_sensor_state(struct ssp_data *);
void set_proximity_threshold(struct ssp_data *, unsigned char, unsigned char);
void set_proximity_barcode_enable(struct ssp_data *, bool);
void set_gesture_current(struct ssp_data *, unsigned char);
unsigned int get_delay_cmd(u8);
unsigned int get_msdelay(int64_t);
unsigned int get_sensor_scanning_info(struct ssp_data *);
unsigned int get_firmware_rev(struct ssp_data *);
int forced_to_download_binary(struct ssp_data *, int);
int parse_dataframe(struct ssp_data *, char *, int);
void enable_debug_timer(struct ssp_data *);
void disable_debug_timer(struct ssp_data *);
int initialize_debug_timer(struct ssp_data *);
int proximity_open_lcd_ldi(struct ssp_data *);
void report_acc_data(struct ssp_data *, struct sensor_value *);
void report_gyro_data(struct ssp_data *, struct sensor_value *);
void report_mag_data(struct ssp_data *, struct sensor_value *);
void report_gesture_data(struct ssp_data *, struct sensor_value *);
void report_pressure_data(struct ssp_data *, struct sensor_value *);
void report_light_data(struct ssp_data *, struct sensor_value *);
void report_prox_data(struct ssp_data *, struct sensor_value *);
void report_prox_raw_data(struct ssp_data *, struct sensor_value *);
void report_geomagnetic_raw_data(struct ssp_data *, struct sensor_value *);
int print_mcu_debug(char *, int *, int);
void report_temp_humidity_data(struct ssp_data *, struct sensor_value *);
unsigned int get_module_rev(struct ssp_data *data);
void reset_mcu(struct ssp_data *);
void convert_acc_data(s16 *);
int sensors_register(struct device *, void *,
	struct device_attribute*[], char *);
void sensors_unregister(struct device *,
	struct device_attribute*[]);
ssize_t mcu_reset_show(struct device *, struct device_attribute *, char *);
ssize_t mcu_revision_show(struct device *, struct device_attribute *, char *);
ssize_t mcu_update_ums_bin_show(struct device *,
	struct device_attribute *, char *);
ssize_t mcu_update_kernel_bin_show(struct device *,
	struct device_attribute *, char *);
ssize_t mcu_update_kernel_crashed_bin_show(struct device *,
	struct device_attribute *, char *);
ssize_t mcu_factorytest_store(struct device *, struct device_attribute *,
	const char *, size_t);
ssize_t mcu_factorytest_show(struct device *,
	struct device_attribute *, char *);
ssize_t mcu_model_name_show(struct device *,
	struct device_attribute *, char *);
ssize_t mcu_sleep_factorytest_show(struct device *,
	struct device_attribute *, char *);
ssize_t mcu_sleep_factorytest_store(struct device *,
	struct device_attribute *, const char *, size_t);
ssize_t mpu6500_gyro_selftest(char *, struct ssp_data *);
ssize_t k330_gyro_selftest(char *, struct ssp_data *);
int ssp_spi_read(struct spi_device *, u8 *, size_t, const int);
int ssp_spi_write(struct spi_device *, const u8 *, const int, const int);
int ssp_spi_write_sync(struct spi_device *, const u8 *, const int);
int ssp_spi_read_sync(struct spi_device *, u8 *, size_t);
int ssp_spi_sync(struct spi_device *, u8 *, size_t,u8 *);
int ssp_spi_async(struct spi_device *, u8 *, size_t,u8 *);
#endif
