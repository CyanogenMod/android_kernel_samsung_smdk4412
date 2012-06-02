
#ifndef _LINUX_WACOM_I2C_H
#define _LINUX_WACOM_I2C_H

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/wakelock.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define NAMEBUF 12
#define WACNAME "WAC_I2C_EMR"
#define WACFLASH "WAC_I2C_FLASH"
#define WACOM_FW_SIZE 32768

/*Wacom Command*/
#define COM_COORD_NUM      7
#define COM_QUERY_NUM      9

#define COM_SAMPLERATE_40  0x33
#define COM_SAMPLERATE_80  0x32
#define COM_SAMPLERATE_133 0x31
#define COM_SURVEYSCAN     0x2B
#define COM_QUERY          0x2A
#define COM_FLASH          0xff
#define COM_CHECKSUM       0x63

/*I2C address for digitizer and its boot loader*/
#define WACOM_I2C_ADDR 0x56
#define WACOM_I2C_BOOT 0x57

/*Information for input_dev*/
#define EMR 0
#define WACOM_PKGLEN_I2C_EMR 0

/*Enable/disable irq*/
#define ENABLE_IRQ 1
#define DISABLE_IRQ 0

/*Special keys*/
#define EPEN_TOOL_PEN		0x220
#define EPEN_TOOL_RUBBER	0x221
#define EPEN_STYLUS			0x22b
#define EPEN_STYLUS2		0x22c

#define WACOM_DELAY_FOR_RST_RISING 200
/* #define INIT_FIRMWARE_FLASH */

/*PDCT Signal*/
#define PDCT_NOSIGNAL 1
#define PDCT_DETECT_PEN 0
#define WACOM_PDCT_WORK_AROUND

#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_P4)
#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
#define SEC_BUS_LOCK
#endif
#define WACOM_HAVE_RESET_CONTROL 0
#define WACOM_POSX_MAX 21866
#define WACOM_POSY_MAX 13730
#define WACOM_POSX_OFFSET 170
#define WACOM_POSY_OFFSET 170
#define WACOM_IRQ_WORK_AROUND
#elif defined(CONFIG_MACH_Q1_BD)
#define BOARD_Q1C210
#define COOR_WORK_AROUND
#define WACOM_IMPORT_FW_ALGO
#define WACOM_SLEEP_WITH_PEN_SLP
#define WACOM_HAVE_RESET_CONTROL 1
#define CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK

#if defined(BOARD_P4ADOBE) && defined(COOR_WORK_AROUND)
	#define COOR_WORK_AROUND_X_MAX		0x54C0
	#define COOR_WORK_AROUND_Y_MAX		0x34F8
	#define COOR_WORK_AROUND_PRESSURE_MAX	0xFF
#elif (defined(BOARD_Q1OMAP4430) || defined(BOARD_Q1C210))\
	&& defined(COOR_WORK_AROUND)
	#define COOR_WORK_AROUND_X_MAX		0x2C80
	#define COOR_WORK_AROUND_Y_MAX		0x1BD0
	#define COOR_WORK_AROUND_PRESSURE_MAX	0xFF
#endif

#define WACOM_I2C_TRANSFER_STYLE
#if !defined(WACOM_I2C_TRANSFER_STYLE)
#define WACOM_I2C_RECV_SEND_STYLE
#endif

#ifdef CONFIG_MACH_Q1_BD
/* For Android origin */
#define WACOM_POSX_MAX 7120
#define WACOM_POSY_MAX 11392
#define WACOM_PRESSURE_MAX 255

#define MAX_ROTATION	4
#define MAX_HAND		2
#endif	/* CONFIG_MACH_Q1_BD */
#endif	/* !defined(WACOM_P4) */

#if !defined(WACOM_SLEEP_WITH_PEN_SLP)
#define WACOM_SLEEP_WITH_PEN_LDO_EN
#endif

/*Parameters for wacom own features*/
struct wacom_features {
	int x_max;
	int y_max;
	int pressure_max;
	char comstat;
#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_P4)
	u8 data[COM_QUERY_NUM];
#else
	u8 data[COM_COORD_NUM];
#endif
	unsigned int fw_version;
	int firm_update_status;
};

/*sec_class sysfs*/
extern struct class *sec_class;

static struct wacom_features wacom_feature_EMR = {
#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_P4)
	.x_max = 0x54C0,
	.y_max = 0x34F8,
	.pressure_max = 0xFF,
#else
	.x_max = 16128,
	.y_max = 8448,
	.pressure_max = 256,
#endif
	.comstat = COM_QUERY,
	.data = {0, 0, 0, 0, 0, 0, 0},
	.fw_version = 0x0,
	.firm_update_status = 0,
};

struct wacom_g5_callbacks {
	int (*check_prox)(struct wacom_g5_callbacks *);
};

struct wacom_g5_platform_data {
	char *name;
	int x_invert;
	int y_invert;
	int xy_switch;
	int min_x;
	int max_x;
	int min_y;
	int max_y;
	int max_pressure;
	int min_pressure;
	int gpio_pendct;
	int (*init_platform_hw)(void);
	int (*exit_platform_hw)(void);
	int (*suspend_platform_hw)(void);
	int (*resume_platform_hw)(void);
	int (*early_suspend_platform_hw)(void);
	int (*late_resume_platform_hw)(void);
	int (*reset_platform_hw)(void);
	void (*register_cb)(struct wacom_g5_callbacks *);
};

/*Parameters for i2c driver*/
struct wacom_i2c {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct early_suspend early_suspend;
	struct mutex lock;
	struct wake_lock wakelock;
	struct device	*dev;
	int irq;
#ifdef WACOM_PDCT_WORK_AROUND
	int irq_pdct;
#endif
	int pen_pdct;
	int gpio;
	int irq_flag;
	int pen_prox;
	int pen_pressed;
	int side_pressed;
	int tool;
	const char name[NAMEBUF];
	struct wacom_features *wac_feature;
	struct wacom_g5_platform_data *wac_pdata;
	struct wacom_g5_callbacks callbacks;
	int (*power)(int on);
	struct work_struct update_work;
	struct delayed_work resume_work;
#ifdef WACOM_IRQ_WORK_AROUND
	struct delayed_work pendct_dwork;
#endif
#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
	unsigned int cpufreq_level;
	bool dvfs_lock_status;
	struct delayed_work dvfs_work;
#if defined(CONFIG_MACH_P4NOTE)
	struct device *bus_dev;
#endif
#endif
};

#endif /* _LINUX_WACOM_I2C_H */
