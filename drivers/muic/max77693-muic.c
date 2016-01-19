/*
 * max77693-muic.c - MUIC driver for the Maxim 77693
 *
 *  Copyright (C) 2012 Samsung Electronics
 *  <sukdong.kim@samsung.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>
#include <linux/host_notify.h>
#include <linux/power_supply.h>
#include <plat/udc-hs.h>
#include <linux/wakelock.h>
#ifdef CONFIG_USBHUB_USB3803
#include <linux/usb3803.h>
#endif
#include <linux/delay.h>

#define DEV_NAME	"max77693-muic"

#if defined(CONFIG_MACH_IRON) || \
	defined(CONFIG_MACH_GRANDE) || \
	defined(CONFIG_MACH_T0_CHN_CTC) || \
	defined(CONFIG_MACH_M0_DUOSCTC)
#define REGARD_442K_AS_523K
#endif

/* for providing API */
static struct max77693_muic_info *gInfo;

/* For restore charger interrupt states */
static u8 chg_int_state;

#ifdef CONFIG_LTE_VIA_SWITCH
/* For saving uart path during CP booting */
static int cpbooting;
#endif

/* MAX77693 MUIC CHG_TYP setting values */
enum {
	/* No Valid voltage at VB (Vvb < Vvbdet) */
	CHGTYP_NO_VOLTAGE	= 0x00,
	/* Unknown (D+/D- does not present a valid USB charger signature) */
	CHGTYP_USB		= 0x01,
	/* Charging Downstream Port */
	CHGTYP_DOWNSTREAM_PORT	= 0x02,
	/* Dedicated Charger (D+/D- shorted) */
	CHGTYP_DEDICATED_CHGR	= 0x03,
	/* Special 500mA charger, max current 500mA */
	CHGTYP_500MA		= 0x04,
	/* Special 1A charger, max current 1A */
	CHGTYP_1A		= 0x05,
	/* Reserved for Future Use */
	CHGTYP_RFU		= 0x06,
	/* Dead Battery Charging, max current 100mA */
	CHGTYP_DB_100MA		= 0x07,
	CHGTYP_MAX,

	CHGTYP_INIT,
	CHGTYP_MIN = CHGTYP_NO_VOLTAGE
};

enum {
	ADC_GND			= 0x00,
	ADC_MHL			= 0x01,
	ADC_DOCK_PREV_KEY	= 0x04,
	ADC_DOCK_NEXT_KEY	= 0x07,
	ADC_DOCK_VOL_DN		= 0x0a, /* 0x01010 14.46K ohm */
	ADC_DOCK_VOL_UP		= 0x0b, /* 0x01011 17.26K ohm */
	ADC_DOCK_PLAY_PAUSE_KEY = 0x0d,
	ADC_SMARTDOCK		= 0x10, /* 0x10000 40.2K ohm */
	ADC_AUDIODOCK		= 0x12, /* 0x10010 64.9K ohm */
	ADC_POWER_SHARING	= 0x14, /* 0x10100 102K ohm */
	ADC_CEA936ATYPE1_CHG	= 0x17,	/* 0x10111 200K ohm */
	ADC_JIG_USB_OFF		= 0x18, /* 0x11000 255K ohm */
	ADC_JIG_USB_ON		= 0x19, /* 0x11001 301K ohm */
	ADC_DESKDOCK		= 0x1a, /* 0x11010 365K ohm */
	ADC_CEA936ATYPE2_CHG	= 0x1b, /* 0x11011 442K ohm */
	ADC_JIG_UART_OFF	= 0x1c, /* 0x11100 523K ohm */
	ADC_JIG_UART_ON		= 0x1d, /* 0x11101 619K ohm */
	ADC_CARDOCK		= 0x1d, /* 0x11101 619K ohm */
	ADC_OPEN		= 0x1f
};

struct max77693_muic_info {
	struct device		*dev;
	struct max77693_dev	*max77693;
	struct i2c_client	*muic;
	struct max77693_muic_data *muic_data;
	int			irq_adc;
	int			irq_chgtype;
	int			irq_vbvolt;
	int			irq_adc1k;
	int			mansw;
	bool			is_default_uart_path_cp;

	struct wake_lock muic_wake_lock;

	enum cable_type_muic	cable_type;

#if defined(CONFIG_MUIC_MAX77693_RESET_WA)
	struct work_struct	reg_init_work;
#endif /* CONFIG_MUIC_MAX77693_RESET_WA */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK) ||\
	defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
	struct delayed_work	dock_work;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK ||
	CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */
	struct delayed_work	init_work;
	struct delayed_work	usb_work;
	struct delayed_work	mhl_work;
	struct mutex		mutex;

	bool			is_usb_ready;
	bool			is_mhl_ready;

	bool			is_adc_open_prev;

	bool			is_otg_test;

#if !defined(CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK)
	bool			is_factory_start;
#endif /* !CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK */
};

static int if_muic_info;
static int switch_sel;
static int if_pmic_rev;
#if defined(REGARD_442K_AS_523K)
static int is_factory_mode = -1;
#endif

/* func : get_if_pmic_inifo
 * switch_sel value get from bootloader comand line
 * switch_sel data consist 8 bits (xxxxzzzz)
 * first 4bits(zzzz) mean path infomation.
 * next 4bits(xxxx) mean if pmic version info
 */
static int get_if_pmic_inifo(char *str)
{
	get_option(&str, &if_muic_info);
	switch_sel = if_muic_info & 0x0f;
	if_pmic_rev = (if_muic_info & 0xf0) >> 4;
	pr_info("%s: switch_sel: %x if_pmic_rev:%x\n",
		__func__, switch_sel, if_pmic_rev);
	return if_muic_info;
}
__setup("pmic_info=", get_if_pmic_inifo);

int get_switch_sel(void)
{
	return switch_sel;
}

#if 0
static int max77693_muic_get_comp2_comn1_pass2
	(struct max77693_muic_info *info)
{
	int ret;
	u8 val;

	ret = max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL1, &val);
	val = val & CLEAR_IDBEN_MICEN_MASK;
	dev_info(info->dev, "func:%s ret:%d val:%x\n", __func__, ret, val);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return -EINVAL;
	}
	return val;
}
#endif

static int max77693_muic_set_comp2_comn1_pass2
	(struct max77693_muic_info *info, int type, int path)
{
	/* type 0 == usb, type 1 == uart */
	u8 cntl1_val, cntl1_msk;
	int ret = 0;
	int val;

	dev_info(info->dev, "func: %s type: %d path: %d\n",
		__func__, type, path);
	if (type == 0) {
		if (path == AP_USB_MODE) {
			info->muic_data->sw_path = AP_USB_MODE;
			val = MAX77693_MUIC_CTRL1_BIN_1_001;
		} else if (path == CP_USB_MODE) {
			info->muic_data->sw_path = CP_USB_MODE;
			val = MAX77693_MUIC_CTRL1_BIN_4_100;
		} else {
			dev_err(info->dev, "func: %s invalid usb path\n"
				, __func__);
			return -EINVAL;
		}
	} else if (type == 1) {
		if (path == UART_PATH_AP) {
			info->muic_data->uart_path = UART_PATH_AP;
			if (info->is_default_uart_path_cp)
				val = MAX77693_MUIC_CTRL1_BIN_5_101;
			else
				val = MAX77693_MUIC_CTRL1_BIN_3_011;
		} else if (path == UART_PATH_CP) {
			info->muic_data->uart_path = UART_PATH_CP;
			if (info->is_default_uart_path_cp)
				val = MAX77693_MUIC_CTRL1_BIN_3_011;
			else
				val = MAX77693_MUIC_CTRL1_BIN_5_101;
#ifdef CONFIG_LTE_VIA_SWITCH
			dev_info(info->dev, "%s: cpbooting is %s\n",
				__func__,
				cpbooting ?
				"started. skip path set" : "done. set path");
			if (!cpbooting) {
				if (gpio_is_valid(GPIO_LTE_VIA_UART_SEL)) {
					gpio_set_value(GPIO_LTE_VIA_UART_SEL,
						GPIO_LEVEL_HIGH);
					dev_info(info->dev,
						"%s: LTE_GPIO_LEVEL_HIGH"
						, __func__);
				} else {
					dev_err(info->dev,
						"%s: ERR_LTE_GPIO_SET_HIGH\n"
						, __func__);
					return -EINVAL;
				}
			}
#endif
		}
#ifdef CONFIG_LTE_VIA_SWITCH
		else if (path == UART_PATH_LTE) {
			info->muic_data->uart_path = UART_PATH_LTE;
			val = MAX77693_MUIC_CTRL1_BIN_5_101;
			if (gpio_is_valid(GPIO_LTE_VIA_UART_SEL)) {
				gpio_set_value(GPIO_LTE_VIA_UART_SEL,
					GPIO_LEVEL_LOW);
				dev_info(info->dev, "%s: LTE_GPIO_LEVEL_LOW\n"
								, __func__);
			} else {
				dev_err(info->dev, "%s: ERR_LTE_GPIO_SET_LOW\n"
								, __func__);
				return -EINVAL;
			}
		}
#endif
		else {
			dev_err(info->dev, "func: %s invalid uart path\n"
				, __func__);
			return -EINVAL;
		}
	}
	else {
		dev_err(info->dev, "func: %s invalid path type(%d)\n"
			, __func__, type);
		return -EINVAL;
	}

	cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;

	ret = max77693_update_reg(info->muic, MAX77693_MUIC_REG_CTRL1,
			cntl1_val, cntl1_msk);

	dev_info(info->dev, "%s: CNTL1(0x%02x) ret: %d\n",
			__func__, cntl1_val, ret);

	return ret;
}

#if 0
static int max77693_muic_set_usb_path_pass2
	(struct max77693_muic_info *info, int path)
{
	int ret = 0;
	ret = max77693_muic_set_comp2_comn1_pass2
		(info, 0/*usb*/, path);
	sysfs_notify(&switch_dev->kobj, NULL, "usb_sel");
	return ret;
}

static int max77693_muic_get_usb_path_pass2
	(struct max77693_muic_info *info)
{
	u8 val;

	val = max77693_muic_get_comp2_comn1_pass2(info);
	if (val == CTRL1_AP_USB)
		return AP_USB_MODE;
	else if (val == CTRL1_CP_USB)
		return CP_USB_MODE;
	else if (val == CTRL1_AUDIO)
		return AUDIO_MODE;
	else
		return -EINVAL;
}
#endif

static int max77693_muic_set_uart_path_pass2
	(struct max77693_muic_info *info, int path)
{
	int ret = 0;

	switch (info->cable_type) {
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
		ret = max77693_muic_set_comp2_comn1_pass2
				(info, 1/*uart*/, path);
		break;
	default:
		pr_info("%s:%s JIG UART OFF isn't connected,"
				"don't change MUIC path\n", DEV_NAME, __func__);
		break;
	}

	return ret;

}

#if 0
static int max77693_muic_get_uart_path_pass2
	(struct max77693_muic_info *info)
{
	u8 val;

	val = max77693_muic_get_comp2_comn1_pass2(info);

	if (val == CTRL1_AP_UART) {
		if (info->is_default_uart_path_cp)
			return UART_PATH_CP;
		else
			return UART_PATH_AP;
	} else if (val == CTRL1_CP_UART) {
#ifdef CONFIG_LTE_VIA_SWITCH
		if (gpio_is_valid(GPIO_LTE_VIA_UART_SEL)) {
			if (gpio_get_value(GPIO_LTE_VIA_UART_SEL))
				return UART_PATH_CP;
			else
				return UART_PATH_LTE;
		} else {
			dev_info(info->dev, "%s: ERR_UART_PATH_LTE\n"
								, __func__);
			return -EINVAL;
		}
#endif
#ifndef CONFIG_LTE_VIA_SWITCH
		if (info->is_default_uart_path_cp)
			return UART_PATH_AP;
		else
			return UART_PATH_CP;
#endif
	} else {
		return -EINVAL;
	}
}
#endif

#if defined(REGARD_442K_AS_523K)
static void max77693_muic_force_uart_switch(int uart_path)
{
	u8 ctrl1_mask, ctrl1_val;
	u8 ctrl2_val;
	u8 gpio_uart_sel = 0;

	switch (uart_path)	{
	case UART_PATH_CP:
		/* Switch UART path to MASTER (PMB9811C, infinion) */
		pr_info("[%s] Force UART path switch to CP (infi)\n",
				__func__);
		ctrl1_val =
			(MAX77693_MUIC_CTRL1_BIN_5_101<<COMN1SW_SHIFT) |
			(MAX77693_MUIC_CTRL1_BIN_5_101<<COMP2SW_SHIFT);
		ctrl1_mask = COMN1SW_MASK | COMP2SW_MASK;
		gpio_uart_sel = GPIO_LEVEL_LOW;
		break;
	case UART_PATH_CP_ESC:
		/* Switch UART path to SLAVE (ESC6270, qualcomm) */
		pr_info("[%s] Force UART path switch to CP (esc)\n",
				__func__);
		ctrl1_val =
			(MAX77693_MUIC_CTRL1_BIN_5_101<<COMN1SW_SHIFT) |
			(MAX77693_MUIC_CTRL1_BIN_5_101<<COMP2SW_SHIFT);
		ctrl1_mask = COMN1SW_MASK | COMP2SW_MASK;
		gpio_uart_sel = GPIO_LEVEL_HIGH;
		break;
	case UART_PATH_AP:
		/* Switch UART path to AP */
		pr_info("[%s] Force UART path switch to AP\n",
				__func__);
		ctrl1_val =
			(MAX77693_MUIC_CTRL1_BIN_3_011<<COMN1SW_SHIFT) |
			(MAX77693_MUIC_CTRL1_BIN_3_011<<COMP2SW_SHIFT);
		ctrl1_mask = COMN1SW_MASK | COMP2SW_MASK;
		break;
	default:
		pr_info("[%s] wrong uart_path, return\n", __func__);
		return;
		break;
	}

	max77693_update_reg(gInfo->muic, MAX77693_MUIC_REG_CTRL1,
						ctrl1_val, ctrl1_mask);
	max77693_update_reg(gInfo->muic,
					MAX77693_MUIC_REG_CTRL2,
					0 << CTRL2_ACCDET_SHIFT,
					CTRL2_ACCDET_MASK);
	max77693_read_reg(gInfo->muic, MAX77693_MUIC_REG_CTRL1, &ctrl1_val);
	max77693_read_reg(gInfo->muic, MAX77693_MUIC_REG_CTRL2, &ctrl2_val);
	pr_info("[%s] REG_CTRL1=0x%x, REG_CTRL2=0x%x\n",
			__func__, ctrl1_val, ctrl2_val);
	if (uart_path != UART_PATH_AP)
		gpio_set_value(GPIO_UART_SEL, gpio_uart_sel);
	pr_info("[%s] GPIO_UART_SEL(%d)\n",
			__func__, gpio_get_value(GPIO_UART_SEL));

}
#endif

static ssize_t max77693_muic_show_usb_state(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	pr_info("%s:%s info->cable_type:%d\n", DEV_NAME, __func__,
			info->cable_type);

	switch (info->cable_type) {
	case CABLE_TYPE_USB_MUIC:
	case CABLE_TYPE_JIG_USB_OFF_MUIC:
	case CABLE_TYPE_JIG_USB_ON_MUIC:
		return sprintf(buf, "USB_STATE_CONFIGURED\n");
	default:
		break;
	}

	return sprintf(buf, "USB_STATE_NOTCONFIGURED\n");
}

static ssize_t max77693_muic_show_device(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	pr_info("%s:%s info->cable_type:%d\n", DEV_NAME, __func__,
			info->cable_type);

	switch (info->cable_type) {
	case CABLE_TYPE_NONE_MUIC:
		return sprintf(buf, "No cable\n");
	case CABLE_TYPE_USB_MUIC:
		return sprintf(buf, "USB\n");
	case CABLE_TYPE_OTG_MUIC:
		return sprintf(buf, "OTG\n");
	case CABLE_TYPE_TA_MUIC:
		return sprintf(buf, "TA\n");
	case CABLE_TYPE_DESKDOCK_MUIC:
		return sprintf(buf, "Desk Dock\n");
	case CABLE_TYPE_CARDOCK_MUIC:
		return sprintf(buf, "Car Dock\n");
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
		return sprintf(buf, "JIG UART OFF\n");
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
		return sprintf(buf, "JIG UART OFF/VB\n");
	case CABLE_TYPE_JIG_UART_ON_MUIC:
		return sprintf(buf, "JIG UART ON\n");
	case CABLE_TYPE_JIG_USB_OFF_MUIC:
		return sprintf(buf, "JIG USB OFF\n");
	case CABLE_TYPE_JIG_USB_ON_MUIC:
		return sprintf(buf, "JIG USB ON\n");
	case CABLE_TYPE_MHL_MUIC:
		return sprintf(buf, "mHL\n");
	case CABLE_TYPE_MHL_VB_MUIC:
		return sprintf(buf, "mHL charging\n");
	case CABLE_TYPE_SMARTDOCK_MUIC:
		return sprintf(buf, "Smart Dock\n");
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
		return sprintf(buf, "Smart Dock+TA\n");
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		return sprintf(buf, "Smart Dock+USB\n");
	case CABLE_TYPE_AUDIODOCK_MUIC:
		return sprintf(buf, "Audio Dock\n");
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE)
	case CABLE_TYPE_POWER_SHARING_MUIC:
		return sprintf(buf, "Power Sharing cable\n");
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE */
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

static ssize_t max77693_muic_show_manualsw(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);

	pr_info("%s:%s sw_path:%d\n", DEV_NAME, __func__,
			info->muic_data->sw_path);/*For debuging*/

	switch (info->muic_data->sw_path) {
	case AP_USB_MODE:
		return sprintf(buf, "PDA\n");
	case CP_USB_MODE:
		return sprintf(buf, "MODEM\n");
	case AUDIO_MODE:
		return sprintf(buf, "Audio\n");
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	case CP_ESC_USB_MODE:
		return sprintf(buf, "ESC\n");
#endif
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

#if defined(CONFIG_SWITCH_USB_PATH_AUTO)
/*
When execute the 'DUN' function. the USB path is switched automatically
without reconnecting the USB
*/
static int max77693_muic_set_usb_path(struct max77693_muic_info *info, int path);
static ssize_t max77693_muic_set_manualsw(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	struct max77693_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "func:%s buf:%s,count:%d\n", __func__, buf, count);

	if (!strncasecmp(buf, "PDA", 3)) {
		info->muic_data->sw_path = AP_USB_MODE;

		if (mdata->usb_cb)
				mdata->usb_cb(USB_CABLE_ATTACHED);
#if defined(CONFIG_MACH_M0_CTC) || defined(CONFIG_MACH_T0_CHN_CTC)
		if (system_rev < 11) {
			gpio_direction_output(GPIO_USB_BOOT_EN, 0);
		} else if (system_rev == 11) {
			gpio_direction_output(GPIO_USB_BOOT_EN, 0);
			gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 0);
		} else {
			gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 0);
		}
#endif
		max77693_muic_set_usb_path(info, AP_USB_MODE);
		dev_info(info->dev, "%s: AP_USB_MODE\n", __func__);
	} else if (!strncasecmp(buf, "MODEM", 5)) {
		info->muic_data->sw_path = CP_USB_MODE;

		if (mdata->usb_cb)
				mdata->usb_cb(USB_CABLE_DETACHED);
#if defined(CONFIG_MACH_M0_CTC) || defined(CONFIG_MACH_T0_CHN_CTC)
		if (system_rev < 11) {
			gpio_direction_output(GPIO_USB_BOOT_EN, 1);
		} else if (system_rev == 11) {
			gpio_direction_output(GPIO_USB_BOOT_EN, 1);
			gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 1);
		} else {
			gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 1);
		}
#endif
		max77693_muic_set_usb_path(info, CP_USB_MODE);

#if defined(CONFIG_SWITCH_DUAL_MODEM)
		gpio_set_value(GPIO_USB_SEL, GPIO_LEVEL_LOW);
		dev_info(info->dev, "%s: MODEM %d\n", __func__,
			gpio_get_value(GPIO_USB_SEL));
#endif
		dev_info(info->dev, "%s: CP_USB_MODE\n", __func__);
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	} else if (!strncasecmp(buf, "ESC", 3)) {
		info->muic_data->sw_path = CP_ESC_USB_MODE;
		gpio_set_value(GPIO_USB_SEL, GPIO_LEVEL_HIGH);
		dev_info(info->dev, "%s: ESC %d\n", __func__,
			gpio_get_value(GPIO_USB_SEL));
#endif
	} else
		dev_warn(info->dev, "%s: Wrong command\n", __func__);

	return count;
}
#else
static ssize_t max77693_muic_set_manualsw(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);

	dev_info(info->dev, "func:%s buf:%s,count:%d\n", __func__, buf, count);

	if (!strncasecmp(buf, "PDA", 3)) {
		info->muic_data->sw_path = AP_USB_MODE;
		dev_info(info->dev, "%s: AP_USB_MODE\n", __func__);
	} else if (!strncasecmp(buf, "MODEM", 5)) {
		info->muic_data->sw_path = CP_USB_MODE;
#if defined(CONFIG_SWITCH_DUAL_MODEM)
		gpio_set_value(GPIO_USB_SEL, GPIO_LEVEL_LOW);
		dev_info(info->dev, "%s: MODEM %d\n", __func__,
			gpio_get_value(GPIO_USB_SEL));
#endif
		dev_info(info->dev, "%s: CP_USB_MODE\n", __func__);
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	} else if (!strncasecmp(buf, "ESC", 3)) {
		info->muic_data->sw_path = CP_ESC_USB_MODE;
		gpio_set_value(GPIO_USB_SEL, GPIO_LEVEL_HIGH);
		dev_info(info->dev, "%s: ESC %d\n", __func__,
			gpio_get_value(GPIO_USB_SEL));
#endif
	} else
		dev_warn(info->dev, "%s: Wrong command\n", __func__);

	return count;
}
#endif /*CONFIG_SWITCH_USB_PATH_AUTO*/

static ssize_t max77693_muic_show_adc(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = max77693_read_reg(info->muic, MAX77693_MUIC_REG_STATUS1, &val);
	pr_info("%s:%s ret:%d val:%x\n", DEV_NAME, __func__, ret, val);

	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}

	return sprintf(buf, "%x\n", (val & STATUS1_ADC_MASK));
}

static ssize_t max77693_muic_show_jig_on(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);

	const char *mode;

	if (info->cable_type ==	CABLE_TYPE_JIG_UART_OFF_MUIC ||
		info->cable_type == CABLE_TYPE_JIG_UART_ON_MUIC ||
		info->cable_type ==	CABLE_TYPE_JIG_USB_OFF_MUIC ||
		info->cable_type == CABLE_TYPE_JIG_USB_ON_MUIC)
		mode = "1";
	else
		mode = "0";

	pr_info("%s:%s mode(%s)\n", DEV_NAME, __func__, mode);

	return sprintf(buf, "%s\n", mode);
}

static ssize_t max77693_muic_show_audio_path(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL1, &val);
	pr_info("%s:%s ret:%d val:%x\n", DEV_NAME, __func__, ret, val);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}

	return sprintf(buf, "%x\n", val);
}

static ssize_t max77693_muic_set_audio_path(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t count)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->muic;
	u8 cntl1_val, cntl1_msk;
	u8 val;
	dev_info(info->dev, "func:%s buf:%s\n", __func__, buf);
	if (!strncmp(buf, "0", 1))
		val = MAX77693_MUIC_CTRL1_BIN_0_000;
	else if (!strncmp(buf, "1", 1))
		val = MAX77693_MUIC_CTRL1_BIN_2_010;
	else {
		dev_warn(info->dev, "%s: Wrong command\n", __func__);
		return count;
	}

	cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT) |
	    (0 << MICEN_SHIFT);
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK | MICEN_MASK;

	max77693_update_reg(client, MAX77693_MUIC_REG_CTRL1, cntl1_val,
			    cntl1_msk);
	dev_info(info->dev, "MUIC cntl1_val:%x, cntl1_msk:%x\n", cntl1_val,
		 cntl1_msk);

	cntl1_val = MAX77693_MUIC_CTRL1_BIN_0_000;
	max77693_read_reg(client, MAX77693_MUIC_REG_CTRL1, &cntl1_val);
	dev_info(info->dev, "%s: CNTL1(0x%02x)\n", __func__, cntl1_val);

	return count;
}

static ssize_t max77693_muic_show_otg_test(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = max77693_read_reg(info->muic, MAX77693_MUIC_REG_CDETCTRL1, &val);
	pr_info("%s:%s ret:%d val:%x buf:%s\n", DEV_NAME, __func__, ret, val,
			buf);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	val &= CHGDETEN_MASK;

	return sprintf(buf, "%x\n", val);
}

static void max77693_muic_enable_accdet(struct max77693_muic_info *info)
{
	u8 cntl2;

	max77693_update_reg(info->muic, MAX77693_MUIC_REG_CTRL2,
			1 << CTRL2_ACCDET_SHIFT, CTRL2_ACCDET_MASK);

	max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL2, &cntl2);
	pr_info("%s:%s CTRL2:0x%02x\n", DEV_NAME, __func__, cntl2);

}

static void max77693_muic_disable_accdet(struct max77693_muic_info *info)
{
	u8 cntl2;

	max77693_update_reg(info->muic, MAX77693_MUIC_REG_CTRL2,
			0 << CTRL2_ACCDET_SHIFT, CTRL2_ACCDET_MASK);

	max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL2, &cntl2);
	pr_info("%s:%s CTRL2:0x%02x\n", DEV_NAME, __func__, cntl2);

}

static ssize_t max77693_muic_set_otg_test(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->muic;
	u8 val;

	pr_info("%s:%s buf:%s\n", DEV_NAME, __func__, buf);
	if (!strncmp(buf, "0", 1)) {
		val = 0;
		info->is_otg_test = true;
		max77693_muic_enable_accdet(info);
	} else if (!strncmp(buf, "1", 1)) {
		val = 1;
		info->is_otg_test = false;
	} else {
		pr_warn("%s:%s Wrong command\n", DEV_NAME, __func__);
		return count;
	}

	max77693_update_reg(client, MAX77693_MUIC_REG_CDETCTRL1,
			    val << CHGDETEN_SHIFT, CHGDETEN_MASK);

	val = 0;
	max77693_read_reg(client, MAX77693_MUIC_REG_CDETCTRL1, &val);
	dev_info(info->dev, "%s: CDETCTRL(0x%02x)\n", __func__, val);

	return count;
}

static void max77693_muic_set_adcdbset(struct max77693_muic_info *info,
				       int value)
{
	int ret;
	u8 val;
	u8 cntl3_before, cntl3_after;

	dev_info(info->dev, "func:%s value:%x\n", __func__, value);
	if (value > 3) {
		dev_err(info->dev, "%s: invalid value(%x)\n", __func__, value);
		return;
	}

	if (!info->muic) {
		dev_err(info->dev, "%s: no muic i2c client\n", __func__);
		return;
	}

	val = value << CTRL3_ADCDBSET_SHIFT;
	max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL3,
				&cntl3_before);
	max77693_write_reg(info->muic, MAX77693_MUIC_REG_CTRL3, val);

	ret = max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL3,
				&cntl3_after);

	pr_info("%s: CNTL3: before:0x%x value:0x%x after:0x%x\n",
		__func__, cntl3_before, val, cntl3_after);

	if (ret < 0)
		dev_err(info->dev, "%s: fail to update reg\n", __func__);
}

static ssize_t max77693_muic_show_adc_debounce_time(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;
	pr_info("%s:%s buf:%s\n", DEV_NAME, __func__, buf);

	if (!info->muic)
		return sprintf(buf, "No I2C client\n");

	ret = max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL3, &val);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	val &= CTRL3_ADCDBSET_MASK;
	val = val >> CTRL3_ADCDBSET_SHIFT;
	dev_info(info->dev, "func:%s val:%x\n", __func__, val);
	return sprintf(buf, "%x\n", val);
}

static ssize_t max77693_muic_set_adc_debounce_time(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);
	value = (value & 0x3);

	dev_info(info->dev, "%s: Do nothing\n", __func__);

	return count;
}

#if defined(REGARD_442K_AS_523K)
static ssize_t max77693_muic_show_is_factory_mode(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;
	pr_info("[%s][buf=%s]", __func__, buf);

	if (!info->muic)
		return sprintf(buf, "No I2C client\n");

	return sprintf(buf, "%d\n", is_factory_mode);
}

static ssize_t max77693_muic_set_is_factory_mode(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	pr_info("[%s][buf=%s][cable_type=%d]", __func__, buf, info->cable_type);

	if (!strncasecmp(buf, "0", 1)) {
		is_factory_mode = 0;
		if (info->cable_type ==
				CABLE_TYPE_JIG_UART_OFF_MUIC)
			max77693_muic_force_uart_switch(
				info->muic_data->uart_path);
	} else if ((!strncasecmp(buf, "1", 1))) {
		is_factory_mode = 1;
		if (info->cable_type ==
				CABLE_TYPE_JIG_UART_OFF_MUIC ||
			info->cable_type ==
				CABLE_TYPE_CEA936ATYPE2_CHG)
			max77693_muic_force_uart_switch(UART_PATH_CP);
	} else {
		pr_info("[%s] wrong value", __func__);
		return -1;
	}

	return count;
}
#endif

static ssize_t max77693_muic_set_uart_sel(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);

	if (!strncasecmp(buf, "AP", 2)) {
		int ret = max77693_muic_set_uart_path_pass2(info, UART_PATH_AP);
		if (ret >= 0)
			info->muic_data->uart_path = UART_PATH_AP;
		else
			dev_err(info->dev, "%s: Change(AP) fail!!", __func__);
	} else if (!strncasecmp(buf, "CP", 2)) {
		int ret = max77693_muic_set_uart_path_pass2(info, UART_PATH_CP);
		if (ret >= 0)
			info->muic_data->uart_path = UART_PATH_CP;
		else
			dev_err(info->dev, "%s: Change(CP) fail!!", __func__);
#if defined(CONFIG_SWITCH_DUAL_MODEM)
		if (gpio_is_valid(GPIO_UART_SEL))	{
			gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);
			dev_info(info->dev,
					"%s: CP %d\n",
					__func__,
					gpio_get_value(GPIO_UART_SEL));
		} else	{
			dev_err(info->dev,
					"%s: Change GPIO failed",
					__func__);
		}
#endif
	}
#ifdef CONFIG_LTE_VIA_SWITCH
	else if (!strncasecmp(buf, "LTE", 3)) {
		int ret = max77693_muic_set_uart_path_pass2
			(info, UART_PATH_LTE);
		if (ret >= 0)
			info->muic_data->uart_path = UART_PATH_LTE;
		else
			dev_err(info->dev, "%s: Change(LTE) fail!!"
						, __func__);
	}
#endif
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	else if (!strncasecmp(buf, "ESC", 3)) {
		int ret = max77693_muic_set_uart_path_pass2
			(info, UART_PATH_CP);
		if (ret >= 0)
			info->muic_data->uart_path = UART_PATH_CP;
		else
			dev_err(info->dev, "%s: Change(CP_ESC) fail!!"
						, __func__);
		if (gpio_is_valid(GPIO_UART_SEL))	{
			gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);
			info->muic_data->uart_path = UART_PATH_CP_ESC;
			dev_info(info->dev, "%s: ESC %d\n",
					__func__,
					gpio_get_value(GPIO_UART_SEL));
		} else	{
			dev_err(info->dev,
					"%s: Change GPIO failed",
					__func__);
		}
	}
#endif
	else {
		dev_warn(info->dev, "%s: Wrong command\n", __func__);
	}

	return count;
}

static ssize_t max77693_muic_show_uart_sel(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);

	pr_info("%s:%s muic_data->uart_path(%d)\n", DEV_NAME, __func__,
			info->muic_data->uart_path);

	switch (info->muic_data->uart_path) {
	case UART_PATH_AP:
		return sprintf(buf, "AP\n");
		break;
	case UART_PATH_CP:
		return sprintf(buf, "CP\n");
		break;
#ifdef CONFIG_LTE_VIA_SWITCH
	case UART_PATH_LTE:
		return sprintf(buf, "LTE\n");
		break;
#endif
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	case UART_PATH_CP_ESC:
		return sprintf(buf, "ESC\n");
		break;
#endif
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

#if !defined(CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK)
static ssize_t max77693_muic_show_apo_factory(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	const char *mode;

	/* true: Factory mode, false: not Factory mode */
	if (info->is_factory_start)
		mode = "FACTORY_MODE";
	else
		mode = "NOT_FACTORY_MODE";

	pr_info("%s:%s apo factory=%s\n", DEV_NAME, __func__, mode);

	return sprintf(buf, "%s\n", mode);
}

static ssize_t max77693_muic_set_apo_factory(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	const char *mode;

	pr_info("%s:%s buf:%s\n", DEV_NAME, __func__, buf);

	/* "FACTORY_START": factory mode */
	if (!strncmp(buf, "FACTORY_START", 13)) {
		info->is_factory_start = true;
		mode = "FACTORY_MODE";
	} else {
		pr_warn("%s:%s Wrong command\n", DEV_NAME, __func__);
		return count;
	}

	pr_info("%s:%s apo factory=%s\n", DEV_NAME, __func__, mode);

	return count;
}
#endif /* !CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK */

#ifdef CONFIG_LTE_VIA_SWITCH
static ssize_t max77693_muic_show_check_cpboot(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	if (cpbooting)
		return sprintf(buf, "start\n");
	else
		return sprintf(buf, "done\n");
}

static ssize_t max77693_muic_set_check_cpboot(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);

	if (!strncasecmp(buf, "start", 5)) {
		if (gpio_is_valid(GPIO_LTE_VIA_UART_SEL)) {
			dev_info(info->dev, "%s: Fix CP-UART path to LTE during CP Booting",
					__func__);
			gpio_set_value(GPIO_LTE_VIA_UART_SEL, GPIO_LEVEL_LOW);
		} else {
			dev_err(info->dev, "%s: ERR_LTE_GPIO_SET_LOW\n",
					__func__);
			return -EINVAL;
		}
		cpbooting = 1;
	} else if (!strncasecmp(buf, "done", 4)) {
		if (info->muic_data->uart_path == UART_PATH_CP) {
			if (gpio_is_valid(GPIO_LTE_VIA_UART_SEL)) {
				dev_info(info->dev, "%s: Reroute CP-UART path to VIA after CP Booting",
						__func__);
				gpio_set_value(GPIO_LTE_VIA_UART_SEL,
						GPIO_LEVEL_HIGH);
			} else {
				dev_err(info->dev, "%s: ERR_LTE_GPIO_SET_HIGH\n",
						__func__);
				return -EINVAL;
			}
		}
		cpbooting = 0;
	} else {
		dev_warn(info->dev, "%s: Wrong command : %s\n", __func__, buf);
	}

	return count;
}
#endif

#if defined(CONFIG_MUIC_MAX77693_DEBUG)
static ssize_t max77693_muic_show_regdump(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	const enum max77693_muic_reg regfile[] = {
			MAX77693_MUIC_REG_STATUS1,
			MAX77693_MUIC_REG_STATUS2,
			MAX77693_MUIC_REG_STATUS3,
			MAX77693_MUIC_REG_INTMASK1,
			MAX77693_MUIC_REG_INTMASK2,
			MAX77693_MUIC_REG_INTMASK3,
			MAX77693_MUIC_REG_CDETCTRL1,
			MAX77693_MUIC_REG_CDETCTRL2,
			MAX77693_MUIC_REG_CTRL1,
			MAX77693_MUIC_REG_CTRL2,
			MAX77693_MUIC_REG_CTRL3,
		};
	u8 val;
	int i;
	int ret = 0;

	for (i = 0; i < ARRAY_SIZE(regfile); i++) {
		ret = max77693_read_reg(info->muic, regfile[i], &val);
		if (ret) {
			pr_err("%s:%s fail to read muic reg(%d), ret=%d\n",
					DEV_NAME, __func__, regfile[i], ret);
			continue;
		}

		pr_info("%s:%s reg(%d)=[0x%02x]\n", DEV_NAME, __func__,
				regfile[i], val);
	}
	return sprintf(buf, "done\n");
}
#endif /* CONFIG_MUIC_MAX77693_DEBUG */

#if defined(CONFIG_MUIC_MAX77693_RESET_WA)
static ssize_t max77693_muic_show_reset(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	u8 muic_irq_mask[3] = {};
	u8 reset_val = 0x0;

	/* MUIC INT1 ~ INT3 MASK register check */
	max77693_bulk_read(info->muic, MAX77693_MUIC_REG_INTMASK1,
			MAX77693_NUM_IRQ_MUIC_REGS, muic_irq_mask);

	if ((reset_val == muic_irq_mask[0])
			&& (reset_val == muic_irq_mask[1])
			&& (reset_val == muic_irq_mask[2])) {
		return sprintf(buf, "RESETED\n");
	}

	return sprintf(buf, "NORMAL\n");
}
#endif /* CONFIG_MUIC_MAX77693_RESET_WA */

static DEVICE_ATTR(uart_sel, 0664, max77693_muic_show_uart_sel,
		max77693_muic_set_uart_sel);
static DEVICE_ATTR(usb_state, S_IRUGO, max77693_muic_show_usb_state, NULL);
static DEVICE_ATTR(device, S_IRUGO, max77693_muic_show_device, NULL);
static DEVICE_ATTR(usb_sel, 0664,
		max77693_muic_show_manualsw, max77693_muic_set_manualsw);
static DEVICE_ATTR(adc, S_IRUGO, max77693_muic_show_adc, NULL);
static DEVICE_ATTR(jig_on, S_IRUGO, max77693_muic_show_jig_on, NULL);
static DEVICE_ATTR(audio_path, 0664,
		max77693_muic_show_audio_path, max77693_muic_set_audio_path);
static DEVICE_ATTR(otg_test, 0664,
		max77693_muic_show_otg_test, max77693_muic_set_otg_test);
static DEVICE_ATTR(adc_debounce_time, 0664,
		max77693_muic_show_adc_debounce_time,
		max77693_muic_set_adc_debounce_time);
#if !defined(CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK)
static DEVICE_ATTR(apo_factory, 0664,
		max77693_muic_show_apo_factory,
		max77693_muic_set_apo_factory);
#endif /* !CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK */
#ifdef CONFIG_LTE_VIA_SWITCH
static DEVICE_ATTR(check_cpboot, 0664,
		max77693_muic_show_check_cpboot,
		max77693_muic_set_check_cpboot);
#endif

#if defined(REGARD_442K_AS_523K)
static DEVICE_ATTR(is_factory_mode, 0664,
		max77693_muic_show_is_factory_mode,
		max77693_muic_set_is_factory_mode);
#endif

#if defined(CONFIG_MUIC_MAX77693_DEBUG)
static DEVICE_ATTR(regdump, S_IRUGO, max77693_muic_show_regdump, NULL);
#endif /* CONFIG_MUIC_MAX77693_DEBUG */

#if defined(CONFIG_MUIC_MAX77693_RESET_WA)
static DEVICE_ATTR(reset, S_IRUGO, max77693_muic_show_reset, NULL);
#endif /* CONFIG_MUIC_MAX77693_RESET_WA */

static struct attribute *max77693_muic_attributes[] = {
	&dev_attr_uart_sel.attr,
	&dev_attr_usb_state.attr,
	&dev_attr_device.attr,
	&dev_attr_usb_sel.attr,
	&dev_attr_adc.attr,
	&dev_attr_jig_on.attr,
	&dev_attr_audio_path.attr,
	&dev_attr_otg_test.attr,
	&dev_attr_adc_debounce_time.attr,
#if !defined(CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK)
	&dev_attr_apo_factory.attr,
#endif /* !CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK */
#ifdef CONFIG_LTE_VIA_SWITCH
	&dev_attr_check_cpboot.attr,
#endif
#if defined(REGARD_442K_AS_523K)
	&dev_attr_is_factory_mode.attr,
#endif
#if defined(CONFIG_MUIC_MAX77693_DEBUG)
	&dev_attr_regdump.attr,
#endif /* CONFIG_MUIC_MAX77693_DEBUG */
#if defined(CONFIG_MUIC_MAX77693_RESET_WA)
	&dev_attr_reset.attr,
#endif /* CONFIG_MUIC_MAX77693_RESET_WA */
	NULL
};

static const struct attribute_group max77693_muic_group = {
	.attrs = max77693_muic_attributes,
};

#if defined(CONFIG_MACH_M0_CTC) || defined(CONFIG_MACH_T0_CHN_CTC)
/*0 : usb is not conneted to CP	1 : usb is connected to CP */
int cp_usb_state;

int max7693_muic_cp_usb_state(void)
{
	return cp_usb_state;
}
EXPORT_SYMBOL(max7693_muic_cp_usb_state);
#endif

static int max77693_muic_set_chgdeten(struct max77693_muic_info *info,
				bool enable)
{
	int ret = 0, val;
	const u8 reg = MAX77693_MUIC_REG_CDETCTRL1;
	const u8 mask = CHGDETEN_MASK;
	const u8 shift = CHGDETEN_SHIFT;
	u8 cdetctrl1;

	val = (enable ? 1 : 0);

	ret = max77693_update_reg(info->muic, reg, (val << shift), mask);
	if (ret)
		pr_err("%s fail to read reg[0x%02x], ret(%d)\n",
				__func__, reg, ret);

	max77693_read_reg(info->muic, reg, &cdetctrl1);
	pr_info("%s:%s CDETCTRL1:0x%02x\n", DEV_NAME, __func__, cdetctrl1);

	return ret;

}

static int max77693_muic_enable_chgdet(struct max77693_muic_info *info)
{
	pr_info("%s:%s\n", DEV_NAME, __func__);
	return max77693_muic_set_chgdeten(info, true);
}

static int max77693_muic_disable_chgdet(struct max77693_muic_info *info)
{
	pr_info("%s:%s\n", DEV_NAME, __func__);
	return max77693_muic_set_chgdeten(info, false);
}

static int max77693_muic_set_usb_path(struct max77693_muic_info *info, int path)
{
	struct i2c_client *client = info->muic;
	struct max77693_muic_data *mdata = info->muic_data;
	int ret;
	u8 cntl1_val, cntl1_msk;
	int val;

	dev_info(info->dev, "func:%s path:%d\n", __func__, path);
	if (mdata->set_safeout) {
		ret = mdata->set_safeout(path);
		if (ret) {
			dev_err(info->dev, "%s: fail to set safout!\n",
				__func__);
			return ret;
		}
	}

	switch (path) {
	case AP_USB_MODE:
		dev_info(info->dev, "%s: AP_USB_MODE\n", __func__);
		val = MAX77693_MUIC_CTRL1_BIN_1_001;
		cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
		break;
	case CP_USB_MODE:
		dev_info(info->dev, "%s: CP_USB_MODE\n", __func__);
		val = MAX77693_MUIC_CTRL1_BIN_4_100;
		cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
		break;
	case AUDIO_MODE:
		dev_info(info->dev, "%s: AUDIO_MODE\n", __func__);
		/* SL1, SR2 */
		cntl1_val = (MAX77693_MUIC_CTRL1_BIN_2_010 << COMN1SW_SHIFT)
			| (MAX77693_MUIC_CTRL1_BIN_2_010 << COMP2SW_SHIFT) |
			(0 << MICEN_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK | MICEN_MASK;
		break;
	case OPEN_USB_MODE:
		dev_info(info->dev, "%s: OPEN_USB_MODE\n", __func__);
		val = MAX77693_MUIC_CTRL1_BIN_0_000;
		cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
		break;
	default:
		dev_warn(info->dev, "%s: invalid path(%d)\n", __func__, path);
		return -EINVAL;
	}

	dev_info(info->dev, "%s: Set manual path\n", __func__);
#if defined(CONFIG_SND_USE_MUIC_SWITCH)
	if (info->cable_type != CABLE_TYPE_CARDOCK_MUIC
		&& info->cable_type != CABLE_TYPE_DESKDOCK_MUIC)
#endif
		max77693_update_reg(client, MAX77693_MUIC_REG_CTRL1, cntl1_val,
				cntl1_msk);

	max77693_update_reg(client, MAX77693_MUIC_REG_CTRL2,
				CTRL2_CPEn1_LOWPWD0,
				CTRL2_CPEn_MASK | CTRL2_LOWPWD_MASK);

	cntl1_val = MAX77693_MUIC_CTRL1_BIN_0_000;
	max77693_read_reg(client, MAX77693_MUIC_REG_CTRL1, &cntl1_val);
	dev_info(info->dev, "%s: CNTL1(0x%02x)\n", __func__, cntl1_val);

	cntl1_val = MAX77693_MUIC_CTRL1_BIN_0_000;
	max77693_read_reg(client, MAX77693_MUIC_REG_CTRL2, &cntl1_val);
	dev_info(info->dev, "%s: CNTL2(0x%02x)\n", __func__, cntl1_val);

	sysfs_notify(&switch_dev->kobj, NULL, "usb_sel");
	return 0;
}

int max77693_muic_get_charging_type(void)
{
	if(!gInfo) {
		pr_err("%s:%s called before MUIC probe!\n", DEV_NAME, __func__);
		return CABLE_TYPE_UNKNOWN_MUIC;
	}

	return gInfo->cable_type;
}

static int max77693_muic_set_charging_type(struct max77693_muic_info *info,
					   bool force_disable)
{
	struct max77693_muic_data *mdata = info->muic_data;
	int ret = 0;
	dev_info(info->dev, "func:%s force_disable:%d\n",
		 __func__, force_disable);
	if (mdata->charger_cb) {
		if (force_disable)
			ret = mdata->charger_cb(CABLE_TYPE_NONE_MUIC);
		else
			ret = mdata->charger_cb(info->cable_type);
	}

	if (ret) {
		dev_err(info->dev, "%s: error from charger_cb(%d)\n", __func__,
			ret);
		return ret;
	}
	return 0;
}

static int max77693_muic_attach_usb_type(struct max77693_muic_info *info,
					 int adc)
{
	struct max77693_muic_data *mdata = info->muic_data;
	int ret, path;
	dev_info(info->dev, "func:%s adc:%x cable_type:%d\n",
		 __func__, adc, info->cable_type);
	if (info->cable_type == CABLE_TYPE_MHL_MUIC
	    || info->cable_type == CABLE_TYPE_MHL_VB_MUIC) {
		dev_warn(info->dev, "%s: mHL was attached!\n", __func__);
		return 0;
	}

	switch (adc) {
	case ADC_JIG_USB_OFF:
		if (info->cable_type == CABLE_TYPE_JIG_USB_OFF_MUIC) {
			dev_info(info->dev, "%s: duplicated(JIG USB OFF)\n",
				 __func__);
			return 0;
		}

		dev_info(info->dev, "%s:JIG USB BOOTOFF\n", __func__);
		info->cable_type = CABLE_TYPE_JIG_USB_OFF_MUIC;
		path = AP_USB_MODE;
		break;
	case ADC_JIG_USB_ON:
		if (info->cable_type == CABLE_TYPE_JIG_USB_ON_MUIC) {
			dev_info(info->dev, "%s: duplicated(JIG USB ON)\n",
				 __func__);
			return 0;
		}

		dev_info(info->dev, "%s:JIG USB BOOTON\n", __func__);
		info->cable_type = CABLE_TYPE_JIG_USB_ON_MUIC;
		path = AP_USB_MODE;
		break;
	case ADC_CEA936ATYPE1_CHG:
	case ADC_OPEN:
		if (info->cable_type == CABLE_TYPE_USB_MUIC) {
			dev_info(info->dev, "%s: duplicated(USB)\n", __func__);
			return 0;
		}

		dev_info(info->dev, "%s:USB\n", __func__);
		info->cable_type = CABLE_TYPE_USB_MUIC;
		path = AP_USB_MODE;
		break;
	default:
		dev_info(info->dev, "%s: Unkown cable(0x%x)\n", __func__, adc);
		return 0;
	}

	ret = max77693_muic_set_charging_type(info, false);
	if (ret) {
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		return ret;
	}
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	if (mdata->sw_path == CP_USB_MODE ||
		mdata->sw_path == CP_ESC_USB_MODE) {
#else
	if (mdata->sw_path == CP_USB_MODE) {
#endif
		info->cable_type = CABLE_TYPE_USB_MUIC;
#if defined(CONFIG_MACH_M0_CTC) || defined(CONFIG_MACH_T0_CHN_CTC)
		if (system_rev < 11) {
			gpio_direction_output(GPIO_USB_BOOT_EN, 1);
		} else if (system_rev == 11) {
			gpio_direction_output(GPIO_USB_BOOT_EN, 1);
			gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 1);
		} else {
			gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 1);
		}
		cp_usb_state = 1;
#endif
#if defined(CONFIG_SWITCH_DUAL_MODEM)
		if (mdata->sw_path == CP_USB_MODE) {
			gpio_set_value(GPIO_USB_SEL, GPIO_LEVEL_LOW);
#if defined(CONFIG_MACH_T0_CHN_CMCC)
			gpio_direction_output(GPIO_CP_USB_ON, 1);
#endif
		} else {
			gpio_set_value(GPIO_USB_SEL, GPIO_LEVEL_HIGH);
		}
#endif
		max77693_muic_set_usb_path(info, CP_USB_MODE);
		return 0;
	}

	max77693_muic_set_usb_path(info, path);

	if (path == AP_USB_MODE) {
		if (mdata->usb_cb && info->is_usb_ready)
#ifdef CONFIG_USBHUB_USB3803
			/* setting usb hub in Diagnostic(hub) mode */
			usb3803_set_mode(USB_3803_MODE_HUB);
#endif				/* CONFIG_USBHUB_USB3803 */
		mdata->usb_cb(USB_CABLE_ATTACHED);
	}

	return 0;
}

static int max77693_muic_attach_dock_type(struct max77693_muic_info *info,
					  int adc)
{
	struct max77693_muic_data *mdata = info->muic_data;
	int path;
	dev_info(info->dev, "func:%s adc:%x, open(%d)\n",
			__func__, adc, info->is_adc_open_prev);
	/*Workaround for unstable adc*/
	if (info->is_adc_open_prev == false) {
		return 0;
	}
	switch (adc) {
	case ADC_DESKDOCK:
		/* Desk Dock */
		if (info->cable_type == CABLE_TYPE_DESKDOCK_MUIC) {
			dev_info(info->dev, "%s: duplicated(DeskDock)\n",
				 __func__);
			return 0;
		}
		dev_info(info->dev, "%s:DeskDock\n", __func__);
		info->cable_type = CABLE_TYPE_DESKDOCK_MUIC;
		path = AUDIO_MODE;

		if (mdata->dock_cb)
			mdata->dock_cb(MAX77693_MUIC_DOCK_DESKDOCK);
		break;
	case ADC_CARDOCK:
		/* Car Dock */
		if (info->cable_type == CABLE_TYPE_CARDOCK_MUIC) {
			dev_info(info->dev, "%s: duplicated(CarDock)\n",
				 __func__);
			return 0;
		}
		dev_info(info->dev, "%s:CarDock\n", __func__);
		info->cable_type = CABLE_TYPE_CARDOCK_MUIC;
		path = AUDIO_MODE;

		if (mdata->dock_cb)
			mdata->dock_cb(MAX77693_MUIC_DOCK_CARDOCK);
		break;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK)
	case ADC_SMARTDOCK:
		if (info->cable_type == CABLE_TYPE_SMARTDOCK_MUIC) {
			dev_info(info->dev, "%s: duplicated(SmartDock)\n",
				__func__);
			return 0;
		}
		dev_info(info->dev, "%s:SmartDock\n", __func__);
		info->cable_type = CABLE_TYPE_SMARTDOCK_MUIC;

		if (info->is_usb_ready) {
			pr_info("%s:%s usb is ready, D+,D- line(AP_USB)\n",
				DEV_NAME, __func__);
			path = AP_USB_MODE;
		} else {
			pr_info("%s:%s usb not ready yet, D+,D- line(Open)\n",
				DEV_NAME, __func__);
			path = OPEN_USB_MODE;
		}

		max77693_muic_set_charging_type(info, false);
		msleep(40);

		if (mdata->mhl_cb && info->is_mhl_ready)
			mdata->mhl_cb(MAX77693_MUIC_ATTACHED);

		if (mdata->dock_cb)
			mdata->dock_cb(MAX77693_MUIC_DOCK_SMARTDOCK);
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
	case ADC_AUDIODOCK:
		if (info->cable_type == CABLE_TYPE_AUDIODOCK_MUIC) {
			pr_info("%s:%s duplicated(AudioDock)\n", DEV_NAME,
					__func__);
			return 0;
		}
		pr_info("%s:%s AudioDock\n", DEV_NAME, __func__);
		info->cable_type = CABLE_TYPE_AUDIODOCK_MUIC;

		if (info->is_usb_ready) {
			pr_info("%s:%s usb is ready, D+,D- line(AP_USB)\n",
				DEV_NAME, __func__);
			path = AP_USB_MODE;
		} else {
			pr_info("%s:%s usb not ready yet, D+,D- line(Open)\n",
				DEV_NAME, __func__);
			path = OPEN_USB_MODE;
		}

		max77693_muic_set_charging_type(info, false);

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_POWERED_HOST_ATTACHED);

		if (mdata->dock_cb)
			mdata->dock_cb(MAX77693_MUIC_DOCK_AUDIODOCK);
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */
	default:
		dev_info(info->dev, "%s: should not reach here(0x%x)\n",
			 __func__, adc);
		return 0;
	}

	max77693_muic_set_usb_path(info, path);

	return 0;
}

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
static void max77693_muic_attach_mhl(struct max77693_muic_info *info, u8 chgtyp)
{
	struct max77693_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "func:%s chgtyp:%x\n", __func__, chgtyp);

	if (info->cable_type == CABLE_TYPE_USB_MUIC) {
		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_CABLE_DETACHED);

		max77693_muic_set_charging_type(info, true);
	}
#if 0
	if (info->cable_type == CABLE_TYPE_MHL) {
		dev_info(info->dev, "%s: duplicated(MHL)\n", __func__);
		return;
	}
#endif
	info->cable_type = CABLE_TYPE_MHL_MUIC;

	if (mdata->mhl_cb && info->is_mhl_ready)
		mdata->mhl_cb(MAX77693_MUIC_ATTACHED);

	if (chgtyp == CHGTYP_USB) {
		info->cable_type = CABLE_TYPE_MHL_VB_MUIC;
		max77693_muic_set_charging_type(info, false);
	}
}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */

static void max77693_muic_handle_jig_uart(struct max77693_muic_info *info,
					  u8 vbvolt)
{
	struct max77693_muic_data *mdata = info->muic_data;
	enum cable_type_muic prev_ct = info->cable_type;
	u8 cntl1_val, cntl1_msk;
	u8 val = MAX77693_MUIC_CTRL1_BIN_3_011;
	int ret = 0;

	dev_info(info->dev, "func:%s vbvolt:%x cable_type:%d\n",
		 __func__, vbvolt, info->cable_type);
	dev_info(info->dev, "%s: JIG UART/BOOTOFF(0x%x)\n", __func__, vbvolt);

	if (info->muic_data->uart_path == UART_PATH_AP) {
		if (info->is_default_uart_path_cp)
			val = MAX77693_MUIC_CTRL1_BIN_5_101;
		else
			val = MAX77693_MUIC_CTRL1_BIN_3_011;
	} else if (info->muic_data->uart_path == UART_PATH_CP) {
		if (info->is_default_uart_path_cp)
			val = MAX77693_MUIC_CTRL1_BIN_3_011;
		else
			val = MAX77693_MUIC_CTRL1_BIN_5_101;
#ifdef CONFIG_LTE_VIA_SWITCH
		dev_info(info->dev, "%s: cpbooting is %s\n",
			__func__,
			cpbooting ?
			"started. skip path set" : "done. set path");
		if (!cpbooting) {
			if (gpio_is_valid(GPIO_LTE_VIA_UART_SEL)) {
				gpio_set_value(GPIO_LTE_VIA_UART_SEL,
					GPIO_LEVEL_HIGH);
				dev_info(info->dev,
					"%s: LTE_GPIO_LEVEL_HIGH"
					, __func__);
			} else {
				dev_err(info->dev,
					"%s: ERR_LTE_GPIO_SET_HIGH\n"
					, __func__);
			}
		}
#endif
#if defined(CONFIG_SWITCH_DUAL_MODEM)
		if (gpio_is_valid(GPIO_UART_SEL)) {
			gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);
			dev_info(info->dev,
					"%s: UART_SEL_GPIO_LEVEL_LOW",
					__func__);
		} else
			dev_err(info->dev,
					"%s: ERR_UART_SEL_GPIO_SET_HIGH\n"
					, __func__);
#endif
	}
#ifdef CONFIG_LTE_VIA_SWITCH
	else if (info->muic_data->uart_path == UART_PATH_LTE) {
		val = MAX77693_MUIC_CTRL1_BIN_5_101;
		/*TODO must modify H/W rev.5*/
		if (gpio_is_valid(GPIO_LTE_VIA_UART_SEL)) {
			gpio_set_value(GPIO_LTE_VIA_UART_SEL,
				GPIO_LEVEL_LOW);
			dev_info(info->dev, "%s: LTE_GPIO_LEVEL_LOW\n"
							, __func__);
		} else
			dev_err(info->dev, "%s: ERR_LTE_GPIO_SET_LOW\n"
							, __func__);
	}
#endif
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	else if (info->muic_data->uart_path == UART_PATH_CP_ESC) {
		val = MAX77693_MUIC_CTRL1_BIN_5_101;
		if (gpio_is_valid(GPIO_UART_SEL)) {
			gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);
			dev_info(info->dev,
					"%s: UART_SEL_GPIO_LEVEL_HIGH",
					__func__);
		} else
			dev_err(info->dev,
					"%s: ERR_UART_SEL_GPIO_SET_LOW\n"
					, __func__);
	}
#endif
	else
		val = MAX77693_MUIC_CTRL1_BIN_3_011;

	cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
	ret = max77693_update_reg(info->muic, MAX77693_MUIC_REG_CTRL1,
			cntl1_val, cntl1_msk);

	dev_info(info->dev, "%s: CNTL1(0x%02x) ret: %d\n",
			__func__, cntl1_val, ret);

	max77693_update_reg(info->muic, MAX77693_MUIC_REG_CTRL2,
				CTRL2_CPEn1_LOWPWD0,
				CTRL2_CPEn_MASK | CTRL2_LOWPWD_MASK);

	if (vbvolt & STATUS2_VBVOLT_MASK) {
		if (mdata->host_notify_cb)
			mdata->host_notify_cb(1);

		if (info->is_otg_test == false)
			max77693_muic_disable_accdet(info);

		info->cable_type = CABLE_TYPE_JIG_UART_OFF_VB_MUIC;
		max77693_muic_set_charging_type(info, info->is_otg_test);

	} else {
		info->cable_type = CABLE_TYPE_JIG_UART_OFF_MUIC;

		if (prev_ct == CABLE_TYPE_JIG_UART_OFF_VB_MUIC) {
			max77693_muic_set_charging_type(info, false);

			if (mdata->host_notify_cb)
				mdata->host_notify_cb(0);

			max77693_muic_enable_accdet(info);
		}
	}
}

static void max77693_otg_control(struct max77693_muic_info *info, int enable)
{
	u8 int_mask, cdetctrl1, chg_cnfg_00;
	pr_info("%s:%s enable(%d)\n", DEV_NAME, __func__, enable);

	if (enable) {
		/* disable charger interrupt */
		max77693_read_reg(info->max77693->i2c,
			MAX77693_CHG_REG_CHG_INT_MASK, &int_mask);
		chg_int_state = int_mask;
		int_mask |= (1 << 4);	/* disable chgin intr */
		int_mask |= (1 << 6);	/* disable chg */

		int_mask &= ~(1 << 0);	/* enable byp intr */
		max77693_write_reg(info->max77693->i2c,
			MAX77693_CHG_REG_CHG_INT_MASK, int_mask);

		/* disable charger detection */
		max77693_read_reg(info->max77693->muic,
			MAX77693_MUIC_REG_CDETCTRL1, &cdetctrl1);
		cdetctrl1 &= ~(1 << 0);
		max77693_write_reg(info->max77693->muic,
			MAX77693_MUIC_REG_CDETCTRL1, cdetctrl1);

		/* OTG on, boost on, DIS_MUIC_CTRL=1 */
		max77693_read_reg(info->max77693->i2c,
			MAX77693_CHG_REG_CHG_CNFG_00, &chg_cnfg_00);
		chg_cnfg_00 &= ~(CHG_CNFG_00_CHG_MASK
				| CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BUCK_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		chg_cnfg_00 |= (CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		max77693_write_reg(info->max77693->i2c,
			MAX77693_CHG_REG_CHG_CNFG_00, chg_cnfg_00);
	} else {
		/* OTG off, boost off, (buck on),
		   DIS_MUIC_CTRL = 0 unless CHG_ENA = 1 */
		max77693_read_reg(info->max77693->i2c,
			MAX77693_CHG_REG_CHG_CNFG_00, &chg_cnfg_00);
		chg_cnfg_00 &= ~(CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		chg_cnfg_00 |= CHG_CNFG_00_BUCK_MASK;
		max77693_write_reg(info->max77693->i2c,
			MAX77693_CHG_REG_CHG_CNFG_00, chg_cnfg_00);

		mdelay(50);

		/* enable charger detection */
		max77693_read_reg(info->max77693->muic,
			MAX77693_MUIC_REG_CDETCTRL1, &cdetctrl1);
		cdetctrl1 |= (1 << 0);
		max77693_write_reg(info->max77693->muic,
			MAX77693_MUIC_REG_CDETCTRL1, cdetctrl1);

		/* enable charger interrupt */
		max77693_write_reg(info->max77693->i2c,
			MAX77693_CHG_REG_CHG_INT_MASK, chg_int_state);
		max77693_read_reg(info->max77693->i2c,
			MAX77693_CHG_REG_CHG_INT_MASK, &int_mask);
	}

	pr_info("%s:%s INT_MASK(0x%x), CDETCTRL1(0x%x), CHG_CNFG_00(0x%x)\n",
			DEV_NAME, __func__, int_mask, cdetctrl1, chg_cnfg_00);
}

static void max77693_powered_otg_control(struct max77693_muic_info *info,
						int enable)
{
	u8 chg_cnfg_00;
	pr_info("%s: powered otg(%d)\n", __func__, enable);

	/*
	 * if powered otg state, disable charger's otg and boost.
	 * don't care about buck, charger state
	 */

	max77693_read_reg(info->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_00, &chg_cnfg_00);
	pr_info("%s: CHG_CNFG_00(0x%x)\n", __func__, chg_cnfg_00);

	chg_cnfg_00 &= ~(CHG_CNFG_00_OTG_MASK
			| CHG_CNFG_00_BOOST_MASK
			| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);

	max77693_write_reg(info->max77693->i2c,
		MAX77693_CHG_REG_CHG_CNFG_00, chg_cnfg_00);
}

/* use in mach for otg */
void otg_control(int enable)
{
	if(!gInfo) {
		pr_err("%s:%s called before MUIC probe!\n", DEV_NAME, __func__);
		return;
	}

	pr_debug("%s: enable(%d)\n", __func__, enable);

	max77693_otg_control(gInfo, enable);
}

/* use in mach for powered-otg */
void powered_otg_control(int enable)
{
	if(!gInfo) {
		pr_err("%s:%s called before MUIC probe!\n", DEV_NAME, __func__);
		return;
	}

	pr_debug("%s: enable(%d)\n", __func__, enable);

	max77693_powered_otg_control(gInfo, enable);
}

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK)
static void max77693_muic_set_cddelay(struct max77693_muic_info *info)
{
	u8 cdetctrl1;
	int ret = 0;

	ret = max77693_read_reg(info->max77693->muic,
		MAX77693_MUIC_REG_CDETCTRL1, &cdetctrl1);

	pr_info("%s:%s read CDETCTRL1=0x%x, ret=%d\n", DEV_NAME, __func__,
			cdetctrl1, ret);

	if ((cdetctrl1 & 0x10) == 0x10) {
		pr_info("%s:%s CDDelay already setted, return\n", DEV_NAME,
				__func__);
		return;
	}

	cdetctrl1 |= 0x10;

	ret = max77693_write_reg(info->max77693->muic,
		MAX77693_MUIC_REG_CDETCTRL1, cdetctrl1);

	pr_info("%s:%s write CDETCTRL1=0x%x, ret=%d\n", DEV_NAME,
			__func__, cdetctrl1, ret);
}

static void max77693_muic_clear_cddelay(struct max77693_muic_info *info)
{
	u8 cdetctrl1;
	int ret = 0;

	ret = max77693_read_reg(info->max77693->muic,
		MAX77693_MUIC_REG_CDETCTRL1, &cdetctrl1);

	pr_info("%s:%s read CDETCTRL1=0x%x, ret=%d\n", DEV_NAME, __func__,
			cdetctrl1, ret);

	if ((cdetctrl1 & 0x10) == 0x0) {
		pr_info("%s:%s CDDelay already cleared, return\n", DEV_NAME,
				__func__);
		return;
	}

	cdetctrl1 &= ~(0x10);

	ret = max77693_write_reg(info->max77693->muic,
		MAX77693_MUIC_REG_CDETCTRL1, cdetctrl1);

	pr_info("%s:%s write CDETCTRL1=0x%x, ret=%d\n", DEV_NAME,
			__func__, cdetctrl1, ret);
}

static void max77693_muic_detach_smart_dock(struct max77693_muic_info *info)
{
	struct max77693_muic_data *mdata = info->muic_data;
	enum cable_type_muic tmp_cable_type = info->cable_type;

	pr_info("%s:%s\n", DEV_NAME, __func__);

	if (info->cable_type != CABLE_TYPE_SMARTDOCK_MUIC &&
			info->cable_type != CABLE_TYPE_SMARTDOCK_TA_MUIC &&
			info->cable_type != CABLE_TYPE_SMARTDOCK_USB_MUIC) {
		pr_info("%s:%s cable_type is not SMARTDOCK\n", DEV_NAME,
				__func__);
		return;
	}

	if (mdata->dock_cb)
		mdata->dock_cb(MAX77693_MUIC_DOCK_DETACHED);

	info->cable_type = CABLE_TYPE_NONE_MUIC;

	max77693_muic_set_charging_type(info, false);

	if (mdata->mhl_cb && info->is_mhl_ready)
		mdata->mhl_cb(MAX77693_MUIC_DETACHED);

	switch (tmp_cable_type) {
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
		pr_info("%s:%s SMARTDOCK+TA\n", DEV_NAME, __func__);

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_POWERED_HOST_DETACHED);
		break;
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		pr_info("%s:%s SMARTDOCK+USB\n", DEV_NAME, __func__);

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_CABLE_DETACHED);
		break;
	case CABLE_TYPE_SMARTDOCK_MUIC:
		/* clear CDDelay 500ms */
		max77693_muic_clear_cddelay(info);
		pr_info("%s:%s SMARTDOCK\n", DEV_NAME, __func__);
		break;
	default:
		pr_warn("%s:%s should not reach here!\n", DEV_NAME,
				__func__);
		return;
		break;
	}
}

static void max77693_muic_attach_smart_dock(struct max77693_muic_info *info,
						u8 adc, u8 vbvolt, u8 chgtyp)
{
	struct max77693_muic_data *mdata = info->muic_data;

	switch (info->cable_type) {
	case CABLE_TYPE_SMARTDOCK_MUIC:
		if (chgtyp == CHGTYP_DEDICATED_CHGR) {
			/* clear CDDelay 500ms */
			max77693_muic_clear_cddelay(info);
			pr_info("%s:%s SMART_DOCK+TA=OTG Enable\n", DEV_NAME,
					__func__);

			if (mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_POWERED_HOST_ATTACHED);

			info->cable_type = CABLE_TYPE_SMARTDOCK_TA_MUIC;
		} else if (chgtyp == CHGTYP_USB) {
			/* clear CDDelay 500ms */
			max77693_muic_clear_cddelay(info);
			pr_info("%s:%s SMART_DOCK+USB=USB Enable\n", DEV_NAME,
					__func__);

			if (mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_CABLE_ATTACHED);

			info->cable_type = CABLE_TYPE_SMARTDOCK_USB_MUIC;
		} else
			pr_info("%s:%s SMART_DOCK + [%d] = ?\n", DEV_NAME,
					__func__, chgtyp);
		break;
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
		if (chgtyp == CHGTYP_DEDICATED_CHGR)
			pr_info("%s:%s Duplicated(SMARTDOCK+TA)\n", DEV_NAME,
					__func__);
		else if (vbvolt)
			pr_info("%s:%s SMART_DOCK + TA -> chgtyp:%x\n",
					DEV_NAME, __func__, chgtyp);
		else
			max77693_muic_detach_smart_dock(info);
		break;
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		if (chgtyp == CHGTYP_USB)
			pr_info("%s:%s Duplicated(SMARTDOCK+USB)\n", DEV_NAME,
					__func__);
		else if (vbvolt)
			pr_info("%s:%s SMART_DOCK + USB -> chgtyp:%x\n",
					DEV_NAME, __func__, chgtyp);
		else
			max77693_muic_detach_smart_dock(info);
		break;
	default:
		if (vbvolt) {
			pr_info("%s:%s SMART_DOCK+vbvolt=chgdetrun\n",
					DEV_NAME, __func__);
			max77693_muic_attach_dock_type(info, adc);
			if (chgtyp == CHGTYP_DEDICATED_CHGR ||
					chgtyp == CHGTYP_USB) {
				pr_info("%s:%s SMART_DOCK+(TA or USB)="\
					"Recursive call to attach oneshot\n",
					DEV_NAME, __func__);
				max77693_muic_attach_smart_dock(info, adc,
						vbvolt, chgtyp);
			}
		} else {
			/* set CDDelay 500ms */
			max77693_muic_set_cddelay(info);
			dev_warn(info->dev, "no vbus in SAMRTDOCK\n");
		}
		break;
	}
}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK */

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
static void max77693_muic_detach_audio_dock(struct max77693_muic_info *info)
{
	struct max77693_muic_data *mdata = info->muic_data;

	pr_info("%s:%s AUDIODOCK\n", DEV_NAME, __func__);

	info->cable_type = CABLE_TYPE_NONE_MUIC;

	max77693_muic_set_charging_type(info, false);

	if (mdata->dock_cb)
		mdata->dock_cb(MAX77693_MUIC_DOCK_DETACHED);

	if (mdata->usb_cb && info->is_usb_ready)
		mdata->usb_cb(USB_POWERED_HOST_DETACHED);
}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */

static int max77693_muic_handle_attach(struct max77693_muic_info *info,
				       u8 status1, u8 status2, int irq)
{
	struct max77693_muic_data *mdata = info->muic_data;
	u8 adc, adclow, vbvolt, chgtyp, chgdetrun, adc1k, dxovp;
	int ret = 0;

	adc = status1 & STATUS1_ADC_MASK;
	adclow = status1 & STATUS1_ADCLOW_MASK;
	adc1k = status1 & STATUS1_ADC1K_MASK;
	chgtyp = status2 & STATUS2_CHGTYP_MASK;
	vbvolt = status2 & STATUS2_VBVOLT_MASK;
	chgdetrun = status2 & STATUS2_CHGDETRUN_MASK;
	dxovp = status2 & STATUS2_DXOVP_MASK;

	dev_info(info->dev, "func:%s st1:%x st2:%x cable_type:%d\n",
		 __func__, status1, status2, info->cable_type);

	switch (info->cable_type) {
	case CABLE_TYPE_OTG_MUIC:
		if (!!adclow) {
			pr_warn("%s:%s assume OTG detach\n", DEV_NAME,
					__func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			max77693_muic_set_charging_type(info, false);

			if (mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_OTGHOST_DETACHED);
		}
		break;
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
		/* Workaround for Factory mode.
		 * Abandon adc interrupt of approximately +-100K range
		 * if previous cable status was JIG UART BOOT OFF.
		 */
		if (adc == (ADC_JIG_UART_OFF + 1) ||
				adc == (ADC_JIG_UART_OFF - 1)) {
			/* Workaround for factory mode in MUIC PASS2
			* In uart path cp, adc is unstable state
			* MUIC PASS2 turn to AP_UART mode automatically
			* So, in this state set correct path manually.
			* !! NEEDED ONLY IF PMIC PASS2 !!
			*/
			if (info->muic_data->uart_path == UART_PATH_CP)
				max77693_muic_handle_jig_uart(info, vbvolt);

#if !defined(CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK)
			if (info->is_factory_start &&
					(adc == ADC_JIG_UART_ON)) {
				pr_info("%s:%s factory start, keep attach\n",
						DEV_NAME, __func__);
				break;
			}
#endif /* !CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK */
			dev_warn(info->dev, "%s: abandon ADC\n", __func__);
			return 0;
		}

		if (adc != ADC_JIG_UART_OFF) {
			dev_warn(info->dev, "%s: assume jig uart off detach\n",
					__func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;
		}
		break;
	case CABLE_TYPE_DESKDOCK_MUIC:
		if (adc != ADC_DESKDOCK) {
			dev_warn(info->dev, "%s: assume deskdock detach\n",
					__func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			max77693_muic_set_charging_type(info, false);
			info->is_adc_open_prev = false;
			if (mdata->dock_cb)
				mdata->dock_cb(MAX77693_MUIC_DOCK_DETACHED);
		}
		break;
	case CABLE_TYPE_CARDOCK_MUIC:
		if (adc != ADC_CARDOCK) {
			dev_warn(info->dev, "%s: assume cardock detach\n",
					__func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			max77693_muic_set_charging_type(info, false);
			info->is_adc_open_prev = false;
			if (mdata->dock_cb)
				mdata->dock_cb(MAX77693_MUIC_DOCK_DETACHED);
		}
		break;
#if !defined(CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK)
	case CABLE_TYPE_JIG_UART_ON_MUIC:
		if ((adc != ADC_JIG_UART_ON) &&
			info->is_factory_start) {
			pr_warn("%s:%s assume jig uart on detach\n",
					DEV_NAME, __func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			if (mdata->dock_cb)
				mdata->dock_cb(MAX77693_MUIC_DOCK_DETACHED);
		}
		break;
#endif /* !CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK)
	case CABLE_TYPE_SMARTDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		if (adc != ADC_SMARTDOCK) {
			dev_warn(info->dev, "%s: assume smartdock detach\n",
					__func__);

			max77693_muic_detach_smart_dock(info);

			info->is_adc_open_prev = false;
		}
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
	case CABLE_TYPE_AUDIODOCK_MUIC:
		if ((adc != ADC_AUDIODOCK) || (!vbvolt)) {
			dev_warn(info->dev, "%s: assume audiodock detach\n",
					__func__);

			max77693_muic_detach_audio_dock(info);
		}
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE)
	case CABLE_TYPE_POWER_SHARING_MUIC:
		if (adc != ADC_POWER_SHARING) {
			pr_warn("%s:%s assume Power Sharing detach\n", DEV_NAME,
				__func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;
			max77693_muic_set_charging_type(info, false);
			max77693_muic_enable_chgdet(info);
		}
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE */
	default:
		break;
	}

	/* 1Kohm ID regiter detection (mHL)
	 * Old MUIC : ADC value:0x00 or 0x01, ADCLow:1
	 * New MUIC : ADC value is not set(Open), ADCLow:1, ADCError:1
	 */
	if (adc1k) {
		if (irq == info->irq_adc
			|| irq == info->irq_chgtype
			|| irq == info->irq_vbvolt) {
			dev_warn(info->dev,
				 "%s: Ignore irq:%d at MHL detection\n",
				 __func__, irq);
			if (vbvolt) {
				if (info->cable_type == CABLE_TYPE_MHL_MUIC
						&& chgtyp == CHGTYP_USB)
					info->cable_type = CABLE_TYPE_MHL_VB_MUIC;

				dev_info(info->dev, "%s: call charger_cb(%d)"
					, __func__, vbvolt);
				max77693_muic_set_charging_type(info, false);
			} else {
				dev_info(info->dev, "%s: call charger_cb(%d)"
					, __func__, vbvolt);
				max77693_muic_set_charging_type(info, true);
			}
			return 0;
		}
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
		max77693_muic_attach_mhl(info, chgtyp);
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
		return 0;
	}

	switch (adc) {
	case ADC_GND:
		if (chgtyp == CHGTYP_NO_VOLTAGE) {
			if (info->cable_type == CABLE_TYPE_OTG_MUIC) {
				dev_info(info->dev,
					 "%s: duplicated(OTG)\n", __func__);
				break;
			}

			info->cable_type = CABLE_TYPE_OTG_MUIC;
			max77693_muic_set_usb_path(info, AP_USB_MODE);
			msleep(40);
			if (mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_OTGHOST_ATTACHED);
		} else if (chgtyp == CHGTYP_USB ||
			   chgtyp == CHGTYP_DOWNSTREAM_PORT ||
			   chgtyp == CHGTYP_DEDICATED_CHGR ||
			   chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A) {
			dev_info(info->dev, "%s: OTG charging pump\n",
				 __func__);
			ret = max77693_muic_set_charging_type(info, false);
		}
		break;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK)
	case ADC_SMARTDOCK:
		max77693_muic_attach_smart_dock(info, adc, vbvolt, chgtyp);
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
	case ADC_AUDIODOCK:
		if (!!vbvolt)
			max77693_muic_attach_dock_type(info, adc);
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE)
	case ADC_POWER_SHARING:
		pr_info("%s:%s Power sharing\n", DEV_NAME, __func__);

		if (chgtyp == CHGTYP_NO_VOLTAGE) {
			if (info->cable_type == CABLE_TYPE_POWER_SHARING_MUIC) {
				pr_info("%s:%s: duplicated(PS)\n", DEV_NAME, __func__);
				break;
			}
			info->cable_type = CABLE_TYPE_POWER_SHARING_MUIC;
			ret = max77693_muic_disable_chgdet(info);
			if (ret)
				pr_err("%s:%s cannot enable chgdet(%d)\n", DEV_NAME,
					__func__, ret);

			ret = max77693_muic_set_charging_type(info, false);
			if (ret)
				pr_err("%s:%s cannot set charging type(%d)\n", DEV_NAME,
					__func__, ret);
		} else if (chgtyp == CHGTYP_USB ||
			   chgtyp == CHGTYP_DOWNSTREAM_PORT ||
			   chgtyp == CHGTYP_DEDICATED_CHGR ||
			   chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A) {
			dev_info(info->dev, "%s: PS charging pump\n",
				 __func__);
			ret = max77693_muic_set_charging_type(info, false);
		}
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE */
	case ADC_JIG_UART_OFF:
		max77693_muic_handle_jig_uart(info, vbvolt);
#if defined(CONFIG_MACH_T0_CHN_CMCC)
		gpio_set_value(GPIO_AP_TD_INT1, GPIO_LEVEL_HIGH);
#endif
		mdata->jig_state(true);
		break;
	case ADC_JIG_USB_ON:
		if (vbvolt & STATUS2_VBVOLT_MASK) {
			dev_info(info->dev, "%s: SKIP_JIG_USB\n", __func__);
			ret = max77693_muic_attach_usb_type(info, adc);
		}
#if defined(CONFIG_MACH_T0_CHN_CMCC)
		gpio_set_value(GPIO_AP_TD_INT1, GPIO_LEVEL_HIGH);
#endif
		mdata->jig_state(true);
		break;
	case ADC_DESKDOCK:
		max77693_muic_attach_dock_type(info, adc);
		if (chgtyp == CHGTYP_USB ||
			chgtyp == CHGTYP_DOWNSTREAM_PORT ||
			chgtyp == CHGTYP_DEDICATED_CHGR ||
			chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A)
			ret = max77693_muic_set_charging_type(info, false);
		else if (chgtyp == CHGTYP_NO_VOLTAGE && !chgdetrun)
			ret = max77693_muic_set_charging_type(info, !vbvolt);
			/* For MAX77693 IC doesn`t occur chgtyp IRQ
			* because of audio noise prevention.
			* So, If below condition is set,
			* we do charging at CARDOCK.
			*/
		break;
	/* ADC_CARDOCK == ADC_JIG_UART_ON */
	case ADC_CARDOCK:
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK)
		max77693_muic_attach_dock_type(info, adc);
		if (chgtyp == CHGTYP_USB ||
			chgtyp == CHGTYP_DOWNSTREAM_PORT ||
			chgtyp == CHGTYP_DEDICATED_CHGR ||
			chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A)
			ret = max77693_muic_set_charging_type(info, false);
		else if (chgtyp == CHGTYP_NO_VOLTAGE && !chgdetrun)
			ret = max77693_muic_set_charging_type(info, !vbvolt);
			/* For MAX77693 IC doesn`t occur chgtyp IRQ
			* because of audio noise prevention.
			* So, If below condition is set,
			* we do charging at CARDOCK.
			*/
#else
		/* because of change FACTORY CPOriented to APOriented,
		 * at manufacture need AP wake-up method. write apo_factory
		 * FACTORY_START is set is_factory_start true.
		 */
		if (info->is_factory_start) {
			if (info->cable_type == CABLE_TYPE_JIG_UART_ON_MUIC) {
				pr_info("%s:%s duplicated(JIG_UART_ON)\n",
					DEV_NAME, __func__);
				return 0;
			}
			pr_info("%s:%s JIG_UART_ON\n", DEV_NAME, __func__);
			info->cable_type = CABLE_TYPE_JIG_UART_ON_MUIC;

			if (mdata->dock_cb)
				mdata->dock_cb(MAX77693_MUIC_DOCK_DESKDOCK);

			return 0;
		}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK */
		break;
	case ADC_JIG_USB_OFF:
	case ADC_CEA936ATYPE1_CHG:
	case ADC_CEA936ATYPE2_CHG:
	case ADC_OPEN:
		switch (chgtyp) {
		case CHGTYP_USB:
		case CHGTYP_DOWNSTREAM_PORT:
			if (adc == ADC_CEA936ATYPE2_CHG)
				break;
			if (info->cable_type == CABLE_TYPE_MHL_MUIC) {
				dev_info(info->dev, "%s: MHL(charging)\n",
					 __func__);
				info->cable_type = CABLE_TYPE_MHL_VB_MUIC;
				ret = max77693_muic_set_charging_type(info,
								      false);
				return ret;
			}

			ret = max77693_muic_attach_usb_type(info, adc);
			break;
		case CHGTYP_DEDICATED_CHGR:
		case CHGTYP_500MA:
		case CHGTYP_1A:
			dev_info(info->dev, "%s:TA\n", __func__);
			info->cable_type = CABLE_TYPE_TA_MUIC;
#ifdef CONFIG_USBHUB_USB3803
			/* setting usb hub in default mode (standby) */
			usb3803_set_mode(USB_3803_MODE_STANDBY);
#endif				/* CONFIG_USBHUB_USB3803 */
			ret = max77693_muic_set_charging_type(info, false);
			if (ret)
				info->cable_type = CABLE_TYPE_NONE_MUIC;
			break;
		default:
			if (dxovp) {
				dev_info(info->dev, "%s:TA(DXOVP)\n", __func__);
				info->cable_type = CABLE_TYPE_TA_MUIC;
				ret = max77693_muic_set_charging_type(info,
							false);
				if (ret)
					info->cable_type = CABLE_TYPE_NONE_MUIC;
			}
			if (adc == ADC_JIG_USB_OFF) {
				pr_info("%s:%s attach JIG_USB_OFF with vbus\n",
						DEV_NAME, __func__);
				max77693_muic_disable_accdet(info);
			}
			break;
		}
		break;
	default:
		dev_warn(info->dev, "%s: unsupported adc=0x%x\n", __func__,
			 adc);
		break;
	}

	return ret;
}

static int max77693_muic_handle_detach(struct max77693_muic_info *info, int irq)
{
	struct i2c_client *client = info->muic;
	struct max77693_muic_data *mdata = info->muic_data;
	enum cable_type_muic prev_ct = CABLE_TYPE_NONE_MUIC;
	u8 cntl2_val;
	int ret = 0;
	dev_info(info->dev, "func:%s\n", __func__);

	info->is_adc_open_prev = true;
	/* Workaround: irq doesn't occur after detaching mHL cable */
	max77693_write_reg(client, MAX77693_MUIC_REG_CTRL1,
				MAX77693_MUIC_CTRL1_BIN_0_000);

	/* Enable Factory Accessory Detection State Machine */
	max77693_muic_enable_accdet(info);

	max77693_update_reg(client, MAX77693_MUIC_REG_CTRL2,
				CTRL2_CPEn0_LOWPWD1,
				CTRL2_CPEn_MASK | CTRL2_LOWPWD_MASK);

	max77693_read_reg(client, MAX77693_MUIC_REG_CTRL2, &cntl2_val);
	dev_info(info->dev, "%s: CNTL2(0x%02x)\n", __func__, cntl2_val);

#if defined(CONFIG_MACH_M0_CTC) || defined(CONFIG_MACH_T0_CHN_CTC)
	if (cp_usb_state != 0) {
		if (system_rev < 11) {
			gpio_direction_output(GPIO_USB_BOOT_EN, 0);
		} else if (system_rev == 11) {
			gpio_direction_output(GPIO_USB_BOOT_EN, 0);
			gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 0);
		} else {
			gpio_direction_output(GPIO_USB_BOOT_EN_REV06, 0);
		}
	cp_usb_state = 0;
	}
#endif
#if defined(CONFIG_SWITCH_DUAL_MODEM) && defined(CONFIG_MACH_T0_CHN_CMCC)
	gpio_direction_output(GPIO_CP_USB_ON, 0);
#endif

#ifdef CONFIG_USBHUB_USB3803
	/* setting usb hub in default mode (standby) */
	usb3803_set_mode(USB_3803_MODE_STANDBY);
#endif  /* CONFIG_USBHUB_USB3803 */

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK)
	/* clear CDDelay 500ms */
	max77693_muic_clear_cddelay(info);
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK */

	mdata->jig_state(false);
	if (info->cable_type == CABLE_TYPE_NONE_MUIC) {
		dev_info(info->dev, "%s: duplicated(NONE)\n", __func__);
		return 0;
	}

	switch (info->cable_type) {
	case CABLE_TYPE_OTG_MUIC:
		dev_info(info->dev, "%s: OTG\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_OTGHOST_DETACHED);
		break;
	case CABLE_TYPE_USB_MUIC:
	case CABLE_TYPE_JIG_USB_OFF_MUIC:
	case CABLE_TYPE_JIG_USB_ON_MUIC:
		dev_info(info->dev, "%s: USB(0x%x)\n", __func__,
			 info->cable_type);
		prev_ct = info->cable_type;
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77693_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = prev_ct;
			break;
		}

		if (mdata->sw_path == CP_USB_MODE)
			return 0;

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_CABLE_DETACHED);
		break;
	case CABLE_TYPE_DESKDOCK_MUIC:
		dev_info(info->dev, "%s: DESKDOCK\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77693_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = CABLE_TYPE_DESKDOCK_MUIC;
			break;
		}
		if (mdata->dock_cb)
			mdata->dock_cb(MAX77693_MUIC_DOCK_DETACHED);
		break;
	case CABLE_TYPE_CARDOCK_MUIC:
		dev_info(info->dev, "%s: CARDOCK\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77693_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = CABLE_TYPE_CARDOCK_MUIC;
			break;
		}
		if (mdata->dock_cb)
			mdata->dock_cb(MAX77693_MUIC_DOCK_DETACHED);
		break;
	case CABLE_TYPE_TA_MUIC:
		dev_info(info->dev, "%s: TA\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		ret = max77693_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_TA_MUIC;
		break;
	case CABLE_TYPE_JIG_UART_ON_MUIC:
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK)
		dev_info(info->dev, "%s: JIG UART/BOOTON\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
#else
		if (info->is_factory_start) {
			pr_info("%s:%s JIG_UART_ON\n", DEV_NAME, __func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			if (mdata->dock_cb)
				mdata->dock_cb(MAX77693_MUIC_DOCK_DETACHED);
		}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK */
		break;
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
		dev_info(info->dev, "%s: JIG UART/BOOTOFF\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		break;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK)
	case CABLE_TYPE_SMARTDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		max77693_muic_detach_smart_dock(info);
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
	case CABLE_TYPE_AUDIODOCK_MUIC:
		max77693_muic_detach_audio_dock(info);
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
		dev_info(info->dev, "%s: JIG UART/OFF/VB\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		ret = max77693_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_JIG_UART_OFF_VB_MUIC;
		break;
	case CABLE_TYPE_MHL_MUIC:
		if (irq == info->irq_adc || irq == info->irq_chgtype) {
			dev_warn(info->dev, "Detech mhl: Ignore irq:%d\n", irq);
			break;
		}
		dev_info(info->dev, "%s: MHL\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		max77693_muic_set_charging_type(info, false);

		if (mdata->mhl_cb && info->is_mhl_ready)
			mdata->mhl_cb(MAX77693_MUIC_DETACHED);
		break;
	case CABLE_TYPE_MHL_VB_MUIC:
		if (irq == info->irq_adc || irq == info->irq_chgtype) {
			dev_warn(info->dev,
				 "Detech vbMhl: Ignore irq:%d\n", irq);
			break;
		}
		dev_info(info->dev, "%s: MHL VBUS\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		max77693_muic_set_charging_type(info, false);

		if (mdata->mhl_cb && info->is_mhl_ready)
			mdata->mhl_cb(MAX77693_MUIC_DETACHED);
		break;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE)
	case CABLE_TYPE_POWER_SHARING_MUIC:
		dev_info(info->dev, "%s: Power Sharing\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		ret = max77693_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_POWER_SHARING_MUIC;
		ret = max77693_muic_enable_chgdet(info);
		if (ret)
			pr_err("%s:%s cannot enable chgdet(%d)\n", DEV_NAME,
				__func__, ret);
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE */
	case CABLE_TYPE_UNKNOWN_MUIC:
		dev_info(info->dev, "%s: UNKNOWN\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77693_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_UNKNOWN_MUIC;
		break;
	default:
		dev_info(info->dev, "%s:invalid cable type %d\n",
			 __func__, info->cable_type);
		break;
	}

	/* jig state clear */
	mdata->jig_state(false);
#if defined(CONFIG_MACH_T0_CHN_CMCC)
	gpio_set_value(GPIO_AP_TD_INT1, GPIO_LEVEL_LOW);
#endif
	return ret;
}

static int max77693_muic_filter_dev(struct max77693_muic_info *info,
					u8 status1, u8 status2)
{
	u8 adc, adclow, adcerr, adc1k, chgtyp, vbvolt, dxovp;
	int intr = INT_ATTACH;

	adc = status1 & STATUS1_ADC_MASK;
	adclow = status1 & STATUS1_ADCLOW_MASK;
	adcerr = status1 & STATUS1_ADCERR_MASK;
	adc1k = status1 & STATUS1_ADC1K_MASK;
	chgtyp = status2 & STATUS2_CHGTYP_MASK;
	vbvolt = status2 & STATUS2_VBVOLT_MASK;
	dxovp = status2 & STATUS2_DXOVP_MASK;

	dev_info(info->dev, "adc:%x adcerr:%x chgtyp:%x vb:%x dxovp:%x cable_type:%d\n",
		adc, adcerr, chgtyp, vbvolt, dxovp, info->cable_type);

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	if (adclow && adc1k) {
		pr_info("%s:%s MHL cable connected\n", DEV_NAME, __func__);
		return INT_ATTACH;
	}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */

	switch (adc) {
	case ADC_GND:
		if (!adclow) {
			pr_info("%s:%s ADC_GND & !adclow = OTG\n", DEV_NAME,
					__func__);
			break;
		}
		pr_info("%s:%s ADC_GND & adclow != OTG\n", DEV_NAME,
				__func__);
	case ADC_MHL ... (ADC_SMARTDOCK - 1):
	case (ADC_OPEN - 1):
	case (ADC_SMARTDOCK + 1):
#if !defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
	case ADC_AUDIODOCK:
#endif /* !CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */
	case ADC_AUDIODOCK + 1:
#if !defined(CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE)
	case ADC_POWER_SHARING:
#endif /* !CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE */
	case (ADC_POWER_SHARING + 1) ... (ADC_CEA936ATYPE1_CHG - 1):
		dev_warn(info->dev, "%s: unsupported ADC(0x%02x)\n",
				__func__, adc);
		intr = INT_DETACH;
		break;
	case ADC_OPEN:
		if (!adcerr) {
			if (chgtyp == CHGTYP_NO_VOLTAGE) {
				if (dxovp)
					break;
				else
					intr = INT_DETACH;
			} else if (chgtyp == CHGTYP_USB ||
				 chgtyp == CHGTYP_DOWNSTREAM_PORT ||
				 chgtyp == CHGTYP_DEDICATED_CHGR ||
				 chgtyp == CHGTYP_500MA ||
				 chgtyp == CHGTYP_1A) {
				switch (info->cable_type) {
				case CABLE_TYPE_OTG_MUIC:
				case CABLE_TYPE_DESKDOCK_MUIC:
				case CABLE_TYPE_CARDOCK_MUIC:
				case CABLE_TYPE_SMARTDOCK_MUIC:
				case CABLE_TYPE_SMARTDOCK_TA_MUIC:
				case CABLE_TYPE_SMARTDOCK_USB_MUIC:
				case CABLE_TYPE_AUDIODOCK_MUIC:
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE)
				case CABLE_TYPE_POWER_SHARING_MUIC:
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE) */
					intr = INT_DETACH;
					break;
				default:
					break;
				}
			}
		}
		break;
	default:
		break;
	}

	return intr;
}

static void max77693_muic_detect_dev(struct max77693_muic_info *info, int irq)
{
	struct i2c_client *client = info->muic;
	u8 status[2];
	int ret;
	u8 cntl1_val, cntl2_val, cntl3_val;
	int intr = INT_ATTACH;

	max77693_read_reg(client, MAX77693_MUIC_REG_CTRL1, &cntl1_val);
	max77693_read_reg(client, MAX77693_MUIC_REG_CTRL2, &cntl2_val);
	max77693_read_reg(client, MAX77693_MUIC_REG_CTRL3, &cntl3_val);
	pr_info("%s:%s CONTROL[1:0x%02x, 2:0x%02x, 3:0x%02x]\n",
		DEV_NAME, __func__, cntl1_val, cntl2_val, cntl3_val);

	ret = max77693_bulk_read(client, MAX77693_MUIC_REG_STATUS1, 2, status);
	dev_info(info->dev, "func:%s irq:%d ret:%d\n", __func__, irq, ret);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg(%d)\n", __func__,
			ret);
		return;
	}

	dev_info(info->dev, "%s: STATUS1:0x%x, 2:0x%x\n", __func__,
		 status[0], status[1]);

	wake_lock_timeout(&info->muic_wake_lock, HZ * 2);

	intr = max77693_muic_filter_dev(info, status[0], status[1]);

	if (intr == INT_ATTACH) {
		dev_info(info->dev, "%s: ATTACHED\n", __func__);
		max77693_muic_handle_attach(info, status[0], status[1], irq);
	} else if (intr == INT_DETACH) {
		dev_info(info->dev, "%s: DETACHED\n", __func__);
		max77693_muic_handle_detach(info, irq);
	} else {
		pr_info("%s:%s device filtered, nothing affect.\n", DEV_NAME,
				__func__);
	}
	return;
}

static irqreturn_t max77693_muic_irq(int irq, void *data)
{
	struct max77693_muic_info *info = data;
	dev_info(info->dev, "%s: irq:%d\n", __func__, irq);

	mutex_lock(&info->mutex);
	max77693_muic_detect_dev(info, irq);
	mutex_unlock(&info->mutex);

	return IRQ_HANDLED;
}

#define REQUEST_IRQ(_irq, _name)					\
do {									\
	ret = request_threaded_irq(_irq, NULL, max77693_muic_irq,	\
				    IRQF_NO_SUSPEND, _name, info);	\
	if (ret < 0)							\
		dev_err(info->dev, "Failed to request IRQ #%d: %d\n",	\
			_irq, ret);					\
} while (0)

static int max77693_muic_irq_init(struct max77693_muic_info *info)
{
	u8 val;
	int ret = 0;

	dev_info(info->dev, "%s: system_rev=%x\n", __func__, system_rev);

	max77693_read_reg(info->muic, MAX77693_MUIC_REG_INT1, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INT1, val);
	max77693_read_reg(info->muic, MAX77693_MUIC_REG_INT2, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INT2, val);
	max77693_read_reg(info->muic, MAX77693_MUIC_REG_INT3, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INT3, val);

	if (info->max77693 && (info->max77693->irq_base > 0)) {
		int irq_base = info->max77693->irq_base;

		info->irq_adc = irq_base + MAX77693_MUIC_IRQ_INT1_ADC;
		REQUEST_IRQ(info->irq_adc, "muic-adc");
		if (ret < 0)
			info->irq_adc = 0;

		info->irq_chgtype = irq_base + MAX77693_MUIC_IRQ_INT2_CHGTYP;
		REQUEST_IRQ(info->irq_chgtype, "muic-chgtype");
		if (ret < 0)
			info->irq_chgtype = 0;

		info->irq_vbvolt = irq_base + MAX77693_MUIC_IRQ_INT2_VBVOLT;
		REQUEST_IRQ(info->irq_vbvolt, "muic-vbvolt");
		if (ret < 0)
			info->irq_vbvolt = 0;

		info->irq_adc1k = irq_base + MAX77693_MUIC_IRQ_INT1_ADC1K;
		REQUEST_IRQ(info->irq_adc1k, "muic-adc1k");
		if (ret < 0)
			info->irq_adc1k = 0;
	}

	dev_info(info->dev, "adc:%d chgtype:%d adc1k:%d vbvolt:%d",
		info->irq_adc, info->irq_chgtype,
		info->irq_adc1k, info->irq_vbvolt);

	max77693_read_reg(info->muic, MAX77693_MUIC_REG_INTMASK1, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INTMASK1, val);

	max77693_read_reg(info->muic, MAX77693_MUIC_REG_INTMASK2, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INTMASK2, val);

	max77693_read_reg(info->muic, MAX77693_MUIC_REG_INTMASK3, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INTMASK3, val);

	return ret;
}

#define CHECK_GPIO(_gpio, _name)					\
do {									\
	if (!_gpio) {							\
		dev_err(&pdev->dev, _name " GPIO defined as 0 !\n");	\
		WARN_ON(!_gpio);					\
		ret = -EIO;						\
		goto err_kfree;						\
	}								\
} while (0)

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK) ||\
	defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
static void max77693_muic_dock_detect(struct work_struct *work)
{
	struct max77693_muic_info *info =
		container_of(work, struct max77693_muic_info, dock_work.work);
	struct i2c_client *client = info->muic;
	u8 status[2];
	int ret;
	u8 cntl1_val;
	u8 adc, adclow, adcerr, adc1k, chgtyp, vbvolt, dxovp;

	mutex_lock(&info->mutex);
	ret = max77693_read_reg(client, MAX77693_MUIC_REG_CTRL1, &cntl1_val);
	pr_info("%s:%s CONTROL1:%x\n", DEV_NAME, __func__, cntl1_val);

	ret = max77693_bulk_read(client, MAX77693_MUIC_REG_STATUS1, 2, status);
	if (ret) {
		pr_err("%s:%s fail to read muic reg(%d)\n", DEV_NAME, __func__,
			ret);
		goto end;
	}

	pr_info("%s:%s STATUS1:0x%x, 2:0x%x\n", DEV_NAME, __func__, status[0],
		status[1]);

	adc = status[0] & STATUS1_ADC_MASK;
	adclow = status[0] & STATUS1_ADCLOW_MASK;
	adcerr = status[0] & STATUS1_ADCERR_MASK;
	adc1k = status[0] & STATUS1_ADC1K_MASK;
	chgtyp = status[1] & STATUS2_CHGTYP_MASK;
	vbvolt = status[1] & STATUS2_VBVOLT_MASK;
	dxovp = status[1] & STATUS2_DXOVP_MASK;

	pr_info("%s:%s adc:%x adcerr:%x chgtyp:%x vb:%x dxovp:%x"\
		" cable_type:%d\n", DEV_NAME, __func__, adc, adcerr, chgtyp,
		vbvolt, dxovp, info->cable_type);

	if (adc1k) {
		pr_info("%s:%s MHL attached, goto end\n", DEV_NAME, __func__);
		goto end;
	}

	if (adcerr) {
		pr_info("%s:%s ADC error, goto end\n", DEV_NAME, __func__);
		goto end;
	}

	switch (adc) {
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK)
	case ADC_SMARTDOCK:
		pr_info("%s:%s Smart Dock\n", DEV_NAME, __func__);

		if (vbvolt && !info->is_usb_ready) {
			pr_info("%s:%s usb not ready yet, D+,D- line(Open)\n",
				DEV_NAME, __func__);
			max77693_muic_set_usb_path(info, OPEN_USB_MODE);
		}
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
	case ADC_AUDIODOCK:
		pr_info("%s:%s Audio Dock\n", DEV_NAME, __func__);

		if (vbvolt && !info->is_usb_ready) {
			pr_info("%s:%s usb not ready yet, D+,D- line(Open)\n",
				DEV_NAME, __func__);
			max77693_muic_set_usb_path(info, OPEN_USB_MODE);
		}
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */
	default:
		break;
	}

end:
	mutex_unlock(&info->mutex);
}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK ||
	CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */

static void max77693_muic_init_detect(struct work_struct *work)
{
	struct max77693_muic_info *info =
	    container_of(work, struct max77693_muic_info, init_work.work);
	dev_info(info->dev, "func:%s\n", __func__);
	mutex_lock(&info->mutex);
	max77693_muic_detect_dev(info, -1);
	mutex_unlock(&info->mutex);
}

static void max77693_muic_usb_detect(struct work_struct *work)
{
	struct max77693_muic_info *info =
	    container_of(work, struct max77693_muic_info, usb_work.work);
	struct max77693_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "func:%s info->muic_data->sw_path:%d\n",
		 __func__, info->muic_data->sw_path);

	mutex_lock(&info->mutex);
	info->is_usb_ready = true;

	if (info->muic_data->sw_path != CP_USB_MODE) {
		if (mdata->usb_cb) {
			switch (info->cable_type) {
			case CABLE_TYPE_USB_MUIC:
			case CABLE_TYPE_JIG_USB_OFF_MUIC:
			case CABLE_TYPE_JIG_USB_ON_MUIC:
#ifdef CONFIG_USBHUB_USB3803
				/* setting usb hub in Diagnostic(hub) mode */
				usb3803_set_mode(USB_3803_MODE_HUB);
#endif				/* CONFIG_USBHUB_USB3803 */
				mdata->usb_cb(USB_CABLE_ATTACHED);
				break;
			case CABLE_TYPE_OTG_MUIC:
				mdata->usb_cb(USB_OTGHOST_ATTACHED);
				break;
			case CABLE_TYPE_SMARTDOCK_MUIC:
				pr_info("%s:%s now usb ready, turn "\
					"D+,D- line to AP_USB\n", DEV_NAME,
					__func__);
				max77693_muic_set_usb_path(info, AP_USB_MODE);
				break;
			case CABLE_TYPE_SMARTDOCK_TA_MUIC:
				pr_info("%s:%s now usb ready, turn "\
					"D+,D- line to AP_USB\n", DEV_NAME,
					__func__);
				max77693_muic_set_usb_path(info, AP_USB_MODE);

				mdata->usb_cb(USB_POWERED_HOST_ATTACHED);
				break;
			case CABLE_TYPE_SMARTDOCK_USB_MUIC:
				pr_info("%s:%s now usb ready, turn "\
					"D+,D- line to AP_USB\n", DEV_NAME,
					__func__);
				max77693_muic_set_usb_path(info, AP_USB_MODE);

				mdata->usb_cb(USB_CABLE_ATTACHED);
				break;
			case CABLE_TYPE_AUDIODOCK_MUIC:
				pr_info("%s:%s now usb ready, turn "\
					"D+,D- line to AP_USB\n", DEV_NAME,
					__func__);
				max77693_muic_set_usb_path(info, AP_USB_MODE);

				mdata->usb_cb(USB_POWERED_HOST_ATTACHED);
				break;
			default:
				break;
			}
		}
	}
	mutex_unlock(&info->mutex);
}

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
static void max77693_muic_mhl_detect(struct work_struct *work)
{
	struct max77693_muic_info *info =
	    container_of(work, struct max77693_muic_info, mhl_work.work);
	struct max77693_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "func:%s cable_type:%d\n", __func__,
		 info->cable_type);
	mutex_lock(&info->mutex);
	info->is_mhl_ready = true;

	if (info->cable_type == CABLE_TYPE_MHL_MUIC ||
		info->cable_type == CABLE_TYPE_MHL_VB_MUIC ||
		info->cable_type == CABLE_TYPE_SMARTDOCK_MUIC ||
		info->cable_type == CABLE_TYPE_SMARTDOCK_TA_MUIC ||
		info->cable_type == CABLE_TYPE_SMARTDOCK_USB_MUIC) {
#ifdef CONFIG_EXTCON
		if (info->edev)
			extcon_set_cable_state(info->edev, "MHL", true);
#else
		if (mdata->mhl_cb)
			mdata->mhl_cb(MAX77693_MUIC_ATTACHED);
#endif
	}
	mutex_unlock(&info->mutex);
}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */

static int uart_switch_init(struct max77693_muic_info *info)
{
	int ret, val;

#ifdef CONFIG_LTE_VIA_SWITCH
	ret = gpio_request(GPIO_LTE_VIA_UART_SEL, "LTE_VIA_SEL");
	if (ret < 0) {
		pr_err("Failed to request GPIO_LTE_VIA_UART_SEL!\n");
		return -ENODEV;
	}
	s3c_gpio_setpull(GPIO_LTE_VIA_UART_SEL, S3C_GPIO_PULL_NONE);
	val = gpio_get_value(GPIO_LTE_VIA_UART_SEL);
	gpio_direction_output(GPIO_LTE_VIA_UART_SEL, val);
	pr_info("func: %s lte_gpio val: %d\n", __func__, val);
#endif
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	ret = gpio_request(GPIO_UART_SEL, "UART_SEL");
	if (ret < 0) {
		pr_err("Failed to request GPIO_UART_SEL!\n");
		return -ENODEV;
	}
	val = gpio_get_value(GPIO_UART_SEL);
	gpio_direction_output(GPIO_UART_SEL, val);

	ret = gpio_request(GPIO_USB_SEL, "USB_SEL");
	if (ret < 0) {
		pr_err("Failed to request GPIO_USB_SEL!\n");
		return -ENODEV;
	}
	val = gpio_get_value(GPIO_USB_SEL);
	gpio_direction_output(GPIO_USB_SEL, val);

#if defined(CONFIG_MACH_T0_CHN_CMCC)
	ret = gpio_request(GPIO_CP_USB_ON, "CP_USB_ON");
	if (ret < 0) {
		pr_err("Failed to request GPIO_CP_USB_ON!\n");
		return -ENODEV;
	}
	gpio_direction_output(GPIO_CP_USB_ON, 0);
#endif
#endif
	return 0;
}

int max77693_muic_get_status1_adc1k_value(void)
{
	u8 adc1k;
	int ret;

	if(!gInfo) {
		pr_err("%s:%s called before MUIC probe!\n", DEV_NAME, __func__);
		return -ENODEV;
	}

	ret = max77693_read_reg(gInfo->muic,
					MAX77693_MUIC_REG_STATUS1, &adc1k);
	if (ret) {
		dev_err(gInfo->dev, "%s: fail to read muic reg(%d)\n",
					__func__, ret);
		return -EINVAL;
	}
	adc1k = adc1k & STATUS1_ADC1K_MASK ? 1 : 0;

	pr_info("func:%s, adc1k: %d\n", __func__, adc1k);
	/* -1:err, 0:adc1k not detected, 1:adc1k detected */
	return adc1k;
}

int max77693_muic_get_status1_adc_value(void)
{
	u8 adc;
	int ret;

	if(!gInfo) {
		pr_err("%s:%s called before MUIC probe!\n", DEV_NAME, __func__);
		return -ENODEV;
	}

	ret = max77693_read_reg(gInfo->muic,
		MAX77693_MUIC_REG_STATUS1, &adc);
	if (ret) {
		dev_err(gInfo->dev, "%s: fail to read muic reg(%d)\n",
			__func__, ret);
		return -EINVAL;
	}

	return adc & STATUS1_ADC_MASK;
}

/*
* func: max77693_muic_set_audio_switch
* arg: bool enable(true:set vps path, false:set path open)
* return: only 0 success
*/
int max77693_muic_set_audio_switch(bool enable)
{
	struct i2c_client *client;
	u8 cntl1_val, cntl1_msk;
	int ret;

	if(!gInfo) {
		pr_err("%s:%s called before MUIC probe!\n", DEV_NAME, __func__);
		return -ENODEV;
	}

	client = gInfo->muic;

	pr_info("func:%s enable(%d)", __func__, enable);

	if (enable) {
		cntl1_val = (MAX77693_MUIC_CTRL1_BIN_2_010 << COMN1SW_SHIFT)
		| (MAX77693_MUIC_CTRL1_BIN_2_010 << COMP2SW_SHIFT) |
		(0 << MICEN_SHIFT);
	} else {
		cntl1_val = 0x3f;
	}
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK | MICEN_MASK;

	ret = max77693_update_reg(client, MAX77693_MUIC_REG_CTRL1, cntl1_val,
			    cntl1_msk);
	cntl1_val = MAX77693_MUIC_CTRL1_BIN_0_000;
	max77693_read_reg(client, MAX77693_MUIC_REG_CTRL1, &cntl1_val);
	dev_info(gInfo->dev, "%s: CNTL1(0x%02x)\n", __func__, cntl1_val);
	return ret;
}

void max77693_update_jig_state(struct max77693_muic_info *info)
{
	struct i2c_client *client = info->muic;
	struct max77693_muic_data *mdata = info->muic_data;
	u8 reg_data, adc;
	int ret, jig_state;

	ret = max77693_read_reg(client, MAX77693_MUIC_REG_STATUS1, &reg_data);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg(%d)\n",
					__func__, ret);
		return;
	}
	adc = reg_data & STATUS1_ADC_MASK;

	switch (adc) {
	case ADC_JIG_UART_OFF:
	case ADC_JIG_USB_OFF:
	case ADC_JIG_USB_ON:
		jig_state = true;
		break;
	default:
		jig_state = false;
		break;
	}

	mdata->jig_state(jig_state);
}

static void max77693_muic_free_irqs(struct max77693_muic_info *info)
{

	pr_info("%s:%s\n", DEV_NAME, __func__);

	/* free MUIC IRQ */
	if (info->irq_adc) {
		free_irq(info->irq_adc, info);
		pr_info("%s:%s irq(%d) free done\n", DEV_NAME, __func__,
				info->irq_adc);
	}
	if (info->irq_chgtype) {
		free_irq(info->irq_chgtype, info);
		pr_info("%s:%s irq(%d) free done\n", DEV_NAME, __func__,
				info->irq_chgtype);
	}
	if (info->irq_vbvolt) {
		free_irq(info->irq_vbvolt, info);
		pr_info("%s:%s irq(%d) free done\n", DEV_NAME, __func__,
				info->irq_vbvolt);
	}
	if (info->irq_adc1k) {
		free_irq(info->irq_adc1k, info);
		pr_info("%s:%s irq(%d) free done\n", DEV_NAME, __func__,
				info->irq_adc1k);
	}
}

static void max77693_muic_init_regs(struct max77693_muic_info *info)
{
	u8 cntl1_val, cntl2_val, cntl3_val;
	int ret;

	/* Set ADC debounce time: 25ms */
	max77693_muic_set_adcdbset(info, 2);

	max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL1, &cntl1_val);
	max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL2, &cntl2_val);
	max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL3, &cntl3_val);
	pr_info("%s:%s CONTROL[1:0x%x, 2:0x%x, 3:0x%x]\n", DEV_NAME, __func__,
			cntl1_val, cntl2_val, cntl3_val);

	ret = max77693_muic_irq_init(info);
	if (ret < 0) {
		pr_err("%s:%s Failed to initialize MUIC irq:%d\n", DEV_NAME,
				__func__, ret);
		max77693_muic_free_irqs(info);
	}
}

#if defined(CONFIG_MUIC_MAX77693_RESET_WA)
static void max77693_muic_rewrite_regs(struct work_struct *work)
{
	struct max77693_muic_info *info =
	    container_of(work, struct max77693_muic_info, reg_init_work);

	mutex_lock(&info->mutex);
	pr_info("%s:%s\n", DEV_NAME, __func__);

	max77693_muic_free_irqs(info);
	max77693_muic_init_regs(info);
	mutex_unlock(&info->mutex);
}
#endif /* CONFIG_MUIC_MAX77693_RESET_WA */

static int __devinit max77693_muic_probe(struct platform_device *pdev)
{
	struct max77693_dev *max77693 = dev_get_drvdata(pdev->dev.parent);
	struct max77693_platform_data *pdata = dev_get_platdata(max77693->dev);
	struct max77693_muic_info *info;

	int switch_sel;
	int ret;

	pr_info("func:%s\n", __func__);
	info = kzalloc(sizeof(struct max77693_muic_info), GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "%s: failed to allocate info\n", __func__);
		ret = -ENOMEM;
		goto err_return;
	}

	info->dev = &pdev->dev;
	info->max77693 = max77693;
	info->muic = max77693->muic;
	info->muic_data = pdata->muic;
	info->is_adc_open_prev = true;
	info->is_otg_test = false;
#if !defined(CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK)
	info->is_factory_start = false;
#endif /* !CONFIG_MUIC_MAX77693_SUPPORT_CAR_DOCK */

	wake_lock_init(&info->muic_wake_lock, WAKE_LOCK_SUSPEND,
		"muic wake lock");

	if (pdata->is_default_uart_path_cp)
		info->is_default_uart_path_cp =
			pdata->is_default_uart_path_cp();
	else
		info->is_default_uart_path_cp = false;
	info->cable_type = CABLE_TYPE_UNKNOWN_MUIC;
	info->muic_data->sw_path = AP_USB_MODE;
	gInfo = info;

	platform_set_drvdata(pdev, info);
	dev_info(info->dev, "adc:%d chgtype:%d, adc1k:%d\n",
		 info->irq_adc, info->irq_chgtype, info->irq_adc1k);

	ret = uart_switch_init(info);
	if (ret) {
		pr_err("Failed to initialize uart\n");
		goto err_input;
	}

	if (info->is_default_uart_path_cp)
		dev_info(info->dev, "H/W rev connected UT1 UR2 pin to CP UART");
	else
		dev_info(info->dev, "H/W rev connected UT1 UR2 pin to AP UART");

/*PASS2 */
	switch_sel = get_switch_sel();
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	switch_sel &= 0xf;
	if ((switch_sel & MAX77693_SWITCH_SEL_1st_BIT_USB) == 0x1)
		info->muic_data->sw_path = AP_USB_MODE;
	else if ((switch_sel & MAX77693_SWITCH_SEL_1st_BIT_USB) == 0x2)
		info->muic_data->sw_path = CP_ESC_USB_MODE;
	else
		info->muic_data->sw_path = CP_USB_MODE;

	if ((switch_sel & MAX77693_SWITCH_SEL_2nd_BIT_UART)
		== 0x1 << 2)
		info->muic_data->uart_path = UART_PATH_AP;
	else if ((switch_sel & MAX77693_SWITCH_SEL_2nd_BIT_UART)
		== 0x2 << 2)
		info->muic_data->uart_path = UART_PATH_CP_ESC;
	else
		info->muic_data->uart_path = UART_PATH_CP;
#else
	/* set USB path follow SWITCH_SEL */
	if (switch_sel & MAX77693_SWITCH_SEL_1st_BIT_USB)
		info->muic_data->sw_path = AP_USB_MODE;
	else
		info->muic_data->sw_path = CP_USB_MODE;

	/* set UART path follow SWITCH_SEL */
	if (switch_sel & MAX77693_SWITCH_SEL_2nd_BIT_UART)
		info->muic_data->uart_path = UART_PATH_AP;
	else {
		info->muic_data->uart_path = UART_PATH_CP;
#ifdef CONFIG_LTE_VIA_SWITCH
		if (switch_sel & MAX77693_SWITCH_SEL_3rd_BIT_LTE_UART)
			info->muic_data->uart_path = UART_PATH_LTE;
#endif
	}
#endif
	pr_info("%s: switch_sel: %x\n", __func__, switch_sel);

	/* create sysfs group */
	ret = sysfs_create_group(&switch_dev->kobj, &max77693_muic_group);
	dev_set_drvdata(switch_dev, info);
	if (ret) {
		dev_err(&pdev->dev,
			"failed to create max77693 muic attribute group\n");
		goto fail;
	}

	if (info->muic_data->init_cb)
		info->muic_data->init_cb();

	mutex_init(&info->mutex);

	mutex_lock(&info->mutex);
	max77693_muic_init_regs(info);
	mutex_unlock(&info->mutex);

	/* init jig state */
	max77693_update_jig_state(info);

#if defined(CONFIG_MUIC_MAX77693_RESET_WA)
	INIT_WORK(&info->reg_init_work, max77693_muic_rewrite_regs);
	max77693->muic_init_work = &info->reg_init_work;
#endif /* CONFIG_MUIC_MAX77693_RESET_WA */

	/* initial cable detection */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK) ||\
	defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
	INIT_DELAYED_WORK(&info->dock_work, max77693_muic_dock_detect);
	schedule_delayed_work(&info->dock_work, msecs_to_jiffies(50));
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK ||
	CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */

	INIT_DELAYED_WORK(&info->init_work, max77693_muic_init_detect);
	schedule_delayed_work(&info->init_work,
			msecs_to_jiffies(CONFIG_MUIC_INIT_DETECT_DELAY));

	INIT_DELAYED_WORK(&info->usb_work, max77693_muic_usb_detect);
	schedule_delayed_work(&info->usb_work,
			msecs_to_jiffies(CONFIG_MUIC_USB_DETECT_DELAY));

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	INIT_DELAYED_WORK(&info->mhl_work, max77693_muic_mhl_detect);
	schedule_delayed_work(&info->mhl_work, msecs_to_jiffies(25000));
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */

	return 0;

 fail:
	mutex_destroy(&info->mutex);
 err_input:
	platform_set_drvdata(pdev, NULL);
	wake_lock_destroy(&info->muic_wake_lock);
 err_kfree:
	kfree(info);
 err_return:
	return ret;
}

static int __devexit max77693_muic_remove(struct platform_device *pdev)
{
	struct max77693_muic_info *info = platform_get_drvdata(pdev);
	sysfs_remove_group(&switch_dev->kobj, &max77693_muic_group);

	if (info) {
		dev_info(info->dev, "func:%s\n", __func__);

#if defined(CONFIG_MUIC_MAX77693_RESET_WA)
		cancel_work_sync(&info->reg_init_work);
#endif /* CONFIG_MUIC_MAX77693_RESET_WA*/
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK) ||\
	defined(CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK)
		cancel_delayed_work(&info->dock_work);
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_SMART_DOCK ||
	CONFIG_MUIC_MAX77693_SUPPORT_OTG_AUDIO_DOCK */
		cancel_delayed_work(&info->init_work);
		cancel_delayed_work(&info->usb_work);
		cancel_delayed_work(&info->mhl_work);

		max77693_muic_free_irqs(info);

		wake_lock_destroy(&info->muic_wake_lock);
		mutex_destroy(&info->mutex);
		kfree(info);
	}
	return 0;
}

void max77693_muic_shutdown(struct device *dev)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	struct max77693_dev *max77693 = i2c_get_clientdata(info->muic);
	int ret;
	u8 val;

	pr_info("%s:%s +\n", DEV_NAME, __func__);
	if (!info->muic) {
		dev_err(info->dev, "%s: no muic i2c client\n", __func__);
		return;
	}

	pr_info("%s:%s max77693->iolock.count.counter=%d\n", DEV_NAME,
		__func__, max77693->iolock.count.counter);

	pr_info("%s:%s JIGSet: auto detection\n", DEV_NAME, __func__);
	val = (0 << CTRL3_JIGSET_SHIFT) | (0 << CTRL3_BOOTSET_SHIFT);

	ret = max77693_update_reg(info->muic, MAX77693_MUIC_REG_CTRL3, val,
			CTRL3_JIGSET_MASK | CTRL3_BOOTSET_MASK);
	if (ret < 0) {
		dev_err(info->dev, "%s: fail to update reg\n", __func__);
		return;
	}

	pr_info("%s:%s -\n", DEV_NAME, __func__);
}

static struct platform_driver max77693_muic_driver = {
	.driver		= {
		.name	= DEV_NAME,
		.owner	= THIS_MODULE,
		.shutdown = max77693_muic_shutdown,
	},
	.probe		= max77693_muic_probe,
	.remove		= __devexit_p(max77693_muic_remove),
};

static int __init max77693_muic_init(void)
{
	pr_info("func:%s\n", __func__);
	return platform_driver_register(&max77693_muic_driver);
}
module_init(max77693_muic_init);

static void __exit max77693_muic_exit(void)
{
	pr_info("func:%s\n", __func__);
	platform_driver_unregister(&max77693_muic_driver);
}
module_exit(max77693_muic_exit);


MODULE_DESCRIPTION("Maxim MAX77693 MUIC driver");
MODULE_AUTHOR("<sukdong.kim@samsung.com>");
MODULE_LICENSE("GPL");
