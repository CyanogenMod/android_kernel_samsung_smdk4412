#ifndef _LINUX_CYPRESS_TOUCHKEY_I2C_H
#define _LINUX_CYPRESS_TOUCHKEY_I2C_H

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

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

/* Touchkey Register */
#define KEYCODE_REG			0x00

#define TK_BIT_PRESS_EV		0x08
#define TK_BIT_KEYCODE		0x07

#define TK_BIT_AUTOCAL		0x80

#if defined(CONFIG_MACH_Q1_BD)\
	|| defined(CONFIG_TARGET_LOCALE_NA)\
	|| defined(CONFIG_MACH_M0)\
	|| defined(CONFIG_MACH_C1)\
	|| defined(CONFIG_MACH_M3)\
	|| defined(CONFIG_MACH_T0)
#define TK_CMD_LED_ON		0x10
#define TK_CMD_LED_OFF		0x20
#else
#define TK_CMD_LED_ON		1
#define TK_CMD_LED_OFF		2
#endif

#define I2C_M_WR 0		/* for i2c */

#define TK_UPDATE_DOWN		1
#define TK_UPDATE_FAIL		-1
#define TK_UPDATE_PASS		0

/* Firmware Version */
#if defined(CONFIG_MACH_Q1_BD)
#define TK_FIRMWARE_VER  0x12
#define TK_MODULE_VER    0x11
#elif defined(CONFIG_MACH_C1_NA_USCC_REV05)
#define TK_FIRMWARE_VER  0x0E
#define TK_MODULE_VER    0x08
#elif defined(CONFIG_MACH_M0)\
	|| defined(CONFIG_MACH_C1)\
	|| defined(CONFIG_MACH_M3)
#define TK_FIRMWARE_VER  0x06
#define TK_MODULE_VER    0x05
#elif defined(CONFIG_MACH_T0)
#define TK_FIRMWARE_VER	 0x11
#define TK_MODULE_VER    0x08
#else
#define TK_FIRMWARE_VER	 0x04
#define TK_MODULE_VER    0x00
#endif

/* LDO Regulator */
#if defined(CONFIG_MACH_M0)\
	|| defined(CONFIG_MACH_C1)\
	|| defined(CONFIG_MACH_M3)\
	|| defined(CONFIG_MACH_T0)
#define	TK_REGULATOR_NAME	"touchkey"
#else
#define	TK_REGULATOR_NAME	"touch"
#endif

/* LED LDO Type*/
#if defined(CONFIG_MACH_M0)\
	|| defined(CONFIG_MACH_C1_KOR_SKT)\
	|| defined(CONFIG_MACH_C1_KOR_KT)\
	|| defined(CONFIG_MACH_M3)\
	|| defined(CONFIG_MACH_C1)\
	|| defined(CONFIG_MACH_T0)
#define LED_LDO_WITH_EN_PIN
#else
#define LED_LDO_WITH_REGULATOR
#endif

/* Autocalibration */
#if defined(CONFIG_TARGET_LOCALE_NAATT)\
	|| defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)\
	|| defined(CONFIG_TARGET_LOCALE_NA)\
	|| defined(CONFIG_MACH_Q1_BD)\
	|| defined(CONFIG_MACH_M0)\
	|| defined(CONFIG_MACH_C1)\
	|| defined(CONFIG_MACH_M3)\
	|| defined(CONFIG_MACH_T0)
#define TK_HAS_AUTOCAL
#endif

/* Generalized SMBus access */
#if defined(CONFIG_MACH_M0)\
	|| defined(CONFIG_MACH_M3)\
	|| defined(CONFIG_MACH_C1)\
	|| defined(CONFIG_MACH_T0)
#define TK_USE_GENERAL_SMBUS
#endif

/* Boot-up Firmware Update */
#if defined(CONFIG_MACH_Q1_BD) \
	|| defined(CONFIG_MACH_C1_NA_USCC_REV05) \
	|| defined(CONFIG_TARGET_LOCALE_NA)\
	|| defined(CONFIG_MACH_M0)\
	|| defined(CONFIG_MACH_C1)\
	|| defined(CONFIG_MACH_M3)\
	|| defined(CONFIG_MACH_T0)
#define TK_HAS_FIRMWARE_UPDATE
#endif

#if defined(CONFIG_TARGET_LOCALE_NAATT) \
	|| defined(CONFIG_TARGET_LOCALE_NAATT_TEMP)
#define TK_USE_4KEY_TYPE_ATT
#elif defined(CONFIG_MACH_LOCALE_NA) \
	|| defined(CONFIG_MACH_U1_NA_SPR) \
	|| defined(CONFIG_MACH_U1_NA_USCC)
#define TK_USE_4KEY_TYPE_NA
#elif defined(CONFIG_MACH_M0) \
	|| defined(CONFIG_MACH_C1) \
	|| defined(CONFIG_MACH_M3)\
	|| defined(CONFIG_MACH_T0)
#define TK_USE_2KEY_TYPE_M0
#else
#define TK_USE_2KEY_TYPE_U1
#endif

#if defined(TK_USE_4KEY_TYPE_ATT)\
	|| defined(TK_USE_4KEY_TYPE_NA)
#define TK_USE_4KEY
#elif defined(TK_USE_2KEY_TYPE_M0)\
	|| defined(TK_USE_2KEY_TYPE_U1)
#define TK_USE_2KEY
#endif

#if defined(CONFIG_MACH_M0_CHNOPEN)\
	|| defined(CONFIG_MACH_M0_HKTW)
#define  TOUCHKEY_FW_UPDATEABLE_HW_REV  10
#elif defined(CONFIG_MACH_M0)
#define  TOUCHKEY_FW_UPDATEABLE_HW_REV  11
#elif defined(CONFIG_MACH_C1)
#if defined(CONFIG_MACH_C1_KOR_SKT)\
	|| defined(CONFIG_MACH_C1_KOR_KT)
#define  TOUCHKEY_FW_UPDATEABLE_HW_REV  8
#elif defined(CONFIG_MACH_C1_KOR_LGT)
#define  TOUCHKEY_FW_UPDATEABLE_HW_REV  5
#else
#define  TOUCHKEY_FW_UPDATEABLE_HW_REV  7
#endif
#else
#define  TOUCHKEY_FW_UPDATEABLE_HW_REV  11
#endif

struct touchkey_platform_data {
	int gpio_sda;
	int gpio_scl;
	int gpio_int;
	void (*init_platform_hw)(void);
	int (*suspend) (void);
	int (*resume) (void);
	int (*power_on) (bool);
	int (*led_power_on) (bool);
	int (*reset_platform_hw)(void);
};

/*Parameters for i2c driver*/
struct touchkey_i2c {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct early_suspend early_suspend;
	struct mutex lock;
	struct device	*dev;
    struct work_struct	work;
    struct workqueue_struct *wq;
	int irq;
	int module_ver;
	int firmware_ver;
	struct touchkey_platform_data *pdata;
	char *name;
	int (*power)(int on);
	struct work_struct update_work;
	int update_status;
	atomic_t keypad_enable;
};

#endif /* _LINUX_CYPRESS_TOUCHKEY_I2C_H */
