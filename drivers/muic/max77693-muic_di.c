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

/* for providing API */
static struct max77693_muic_info *gInfo;

/* For restore charger interrupt states */
static u8 chg_int_state;

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
	ADC_REMOTE_SWITCH	= 0x12, /* 0x10010 64.9K ohm */
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
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	int			irq_adc1k;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
	int			mansw;

	struct wake_lock muic_wake_lock;

	enum cable_type_muic	cable_type;

	struct delayed_work	init_work;
	struct delayed_work	usb_work;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	struct delayed_work	mhl_work;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
	struct mutex		mutex;

	bool			is_usb_ready;
	bool			is_mhl_ready;

	bool			is_adc_open_prev;

	bool			is_otg_attach_blocked;
	bool			is_otg_test;

	bool			is_factory_start;
};

static int if_muic_info;
static int switch_sel;
static int if_pmic_rev;

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
	pr_info("%s %s: switch_sel: %x if_pmic_rev:%x\n",
		__FILE__, __func__, switch_sel, if_pmic_rev);
	return if_muic_info;
}
__setup("pmic_info=", get_if_pmic_inifo);

int get_switch_sel(void)
{
	return switch_sel;
}

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
			val = MAX77693_MUIC_CTRL1_BIN_3_011;
		} else if (path == UART_PATH_CP) {
			info->muic_data->uart_path = UART_PATH_CP;
			val = MAX77693_MUIC_CTRL1_BIN_5_101;
		}
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

static int max77693_muic_set_uart_path_pass2
	(struct max77693_muic_info *info, int path)
{
	int ret = 0;
	ret = max77693_muic_set_comp2_comn1_pass2
		(info, 1/*uart*/, path);
	return ret;

}

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
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_REMOTE_SWITCH)
	case CABLE_TYPE_REMOTE_SWITCH_MUIC:
		return sprintf(buf, "REMOTE SWITCH\n");
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_REMOTE_SWITCH */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	case CABLE_TYPE_MHL_MUIC:
		return sprintf(buf, "mHL\n");
	case CABLE_TYPE_MHL_VB_MUIC:
		return sprintf(buf, "mHL charging\n");
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
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
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

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
		dev_info(info->dev, "%s: CP_USB_MODE\n", __func__);
	} else
		dev_warn(info->dev, "%s: Wrong command\n", __func__);

	return count;
}

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

static ssize_t max77693_muic_set_uart_sel(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);

	if (!strncasecmp(buf, "AP", 2)) {
		int ret = max77693_muic_set_uart_path_pass2
			(info, UART_PATH_AP);
		if (ret >= 0)
			info->muic_data->uart_path = UART_PATH_AP;
		else
			dev_err(info->dev, "%s: Change(AP) fail!!"
				, __func__);
	} else if (!strncasecmp(buf, "CP", 2)) {
		int ret = max77693_muic_set_uart_path_pass2
			(info, UART_PATH_CP);
		if (ret >= 0)
			info->muic_data->uart_path = UART_PATH_CP;
		else
			dev_err(info->dev, "%s: Change(CP) fail!!"
						, __func__);
	} else {
			dev_warn(info->dev, "%s: Wrong command\n"
						, __func__);
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
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

static ssize_t max77693_muic_show_otg_block(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	const char *mode;

	/* "BLOCK": blocked(true), "ALLOW": allowed(false) */
	if (info->is_otg_attach_blocked) {
		pr_info("%s:%s otg attach=blocked\n", DEV_NAME, __func__);
		mode = "BLOCK";
	} else {
		pr_info("%s:%s otg attach=allowed\n", DEV_NAME, __func__);
		mode = "ALLOW";
	}

	return sprintf(buf, "%s\n", mode);
}

static ssize_t max77693_muic_set_otg_block(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	const char *mode;

	pr_info("%s:%s buf:%s\n", DEV_NAME, __func__, buf);

	/* "BLOCK": blocked(true), "ALLOW": allowed(false) */
	if (!strncmp(buf, "BLOCK", 5)) {
		info->is_otg_attach_blocked = true;
		mode = "BLOCK";
	} else if (!strncmp(buf, "ALLOW", 5)) {
		info->is_otg_attach_blocked = false;
		mode = "ALLOW";
	} else {
		dev_warn(info->dev, "%s: Wrong command\n", __func__);
		return count;
	}

	pr_info("%s:%s otg attach=%s\n", DEV_NAME, __func__, mode);

	return count;
}

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

static DEVICE_ATTR(uart_sel, 0664, max77693_muic_show_uart_sel,
		max77693_muic_set_uart_sel);
static DEVICE_ATTR(usb_state, S_IRUGO, max77693_muic_show_usb_state, NULL);
static DEVICE_ATTR(device, S_IRUGO, max77693_muic_show_device, NULL);
static DEVICE_ATTR(usb_sel, 0664,
		max77693_muic_show_manualsw, max77693_muic_set_manualsw);
static DEVICE_ATTR(adc, S_IRUGO, max77693_muic_show_adc, NULL);
static DEVICE_ATTR(audio_path, 0664,
		max77693_muic_show_audio_path, max77693_muic_set_audio_path);
static DEVICE_ATTR(otg_test, 0664,
		max77693_muic_show_otg_test, max77693_muic_set_otg_test);
static DEVICE_ATTR(adc_debounce_time, 0664,
		max77693_muic_show_adc_debounce_time,
		max77693_muic_set_adc_debounce_time);
static DEVICE_ATTR(otg_block, 0664,
		max77693_muic_show_otg_block,
		max77693_muic_set_otg_block);
static DEVICE_ATTR(apo_factory, 0664,
		max77693_muic_show_apo_factory,
		max77693_muic_set_apo_factory);

static struct attribute *max77693_muic_attributes[] = {
	&dev_attr_uart_sel.attr,
	&dev_attr_usb_state.attr,
	&dev_attr_device.attr,
	&dev_attr_usb_sel.attr,
	&dev_attr_adc.attr,
	&dev_attr_audio_path.attr,
	&dev_attr_otg_test.attr,
	&dev_attr_adc_debounce_time.attr,
	&dev_attr_otg_block.attr,
	&dev_attr_apo_factory.attr,
	NULL
};

static const struct attribute_group max77693_muic_group = {
	.attrs = max77693_muic_attributes,
};

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
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	if (info->cable_type == CABLE_TYPE_MHL_MUIC
	    || info->cable_type == CABLE_TYPE_MHL_VB_MUIC) {
		dev_warn(info->dev, "%s: mHL was attached!\n", __func__);
		return 0;
	}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */

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
	if (mdata->sw_path == CP_USB_MODE) {
		info->cable_type = CABLE_TYPE_USB_MUIC;
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

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_REMOTE_SWITCH)
static void max77693_muic_attach_remote_switch(struct max77693_muic_info *info)
{
	struct i2c_client *client = info->muic;
	u8 cntl1_val;

	pr_info("%s:%s\n", DEV_NAME, __func__);

	max77693_read_reg(client, MAX77693_MUIC_REG_CTRL1, &cntl1_val);
	pr_info("%s:%s CNTL1(0x%02x)\n", DEV_NAME, __func__, cntl1_val);

	cntl1_val = CTRL1_AUDIO;
	max77693_write_reg(client, MAX77693_MUIC_REG_CTRL1, cntl1_val);

	max77693_read_reg(client, MAX77693_MUIC_REG_CTRL1, &cntl1_val);
	pr_info("%s:%s CNTL1(0x%02x)\n", DEV_NAME, __func__, cntl1_val);
}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_REMOTE_SWITCH */

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
		val = MAX77693_MUIC_CTRL1_BIN_3_011;
	} else if (info->muic_data->uart_path == UART_PATH_CP) {
		val = MAX77693_MUIC_CTRL1_BIN_5_101;
	} else
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
	u8 mu_adc = max77693_muic_get_status1_adc_value();

	pr_info("%s:%s enable(%d)\n", DEV_NAME, __func__, enable);

	if (enable) {
		/* disable charger interrupt */
		max77693_read_reg(info->max77693->i2c,
			MAX77693_CHG_REG_CHG_INT_MASK, &int_mask);
		chg_int_state = int_mask;
		int_mask |= (1 << 4);	/* disable chgin intr */
		int_mask |= (1 << 6);	/* disable chg */

		/* In Factory mode using anyway Jig to switch between
		 * USB <--> UART sees a momentary 301K resistance as that of an
		 * OTG. Disabling charging INTRS now can lead to USB and MTP
		 * drivers not getting recognized in subsequent switches.
		 * Factory Mode BOOT(on) USB.
		 */
		if (mu_adc && !(info->is_otg_test)) {
			pr_info("%s:%s JIG USB CABLE adc(0x%x))\n", DEV_NAME,
					__func__, mu_adc);
			pr_info("%s:%s Enabling charging INT for the "\
				"Non-OTG case.\n", DEV_NAME, __func__);
			int_mask &= ~(1 << 6);	/* Enabling Chgin INTR.*/
		}

		int_mask &= ~(1 << 0);	/* enable byp intr */
		max77693_write_reg(info->max77693->i2c,
			MAX77693_CHG_REG_CHG_INT_MASK, int_mask);

		/* disable charger detection */
		max77693_read_reg(info->max77693->muic,
			MAX77693_MUIC_REG_CDETCTRL1, &cdetctrl1);
		cdetctrl1 &= ~(1 << 0);

		/* Factory Mode BOOT(on) USB */
		if (mu_adc && !(info->is_otg_test)) {
			pr_info("%s:%s Enabling Charging Detn. for non-OTG\n",
				DEV_NAME, __func__);
			 /*Enabling Charger Detn on Rising VB */
			cdetctrl1 |= (1 << 0);
		}

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

static int max77693_muic_handle_attach(struct max77693_muic_info *info,
				       u8 status1, u8 status2, int irq)
{
	struct max77693_muic_data *mdata = info->muic_data;
	u8 adc, adclow, vbvolt, chgtyp, dxovp;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	u8 adc1k;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
	int ret = 0;

	adc = status1 & STATUS1_ADC_MASK;
	adclow = status1 & STATUS1_ADCLOW_MASK;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	adc1k = status1 & STATUS1_ADC1K_MASK;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
	chgtyp = status2 & STATUS2_CHGTYP_MASK;
	vbvolt = status2 & STATUS2_VBVOLT_MASK;
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

			if (info->is_factory_start &&
					(adc == ADC_JIG_UART_ON)) {
				pr_info("%s:%s factory start, keep attach\n",
						DEV_NAME, __func__);
				break;
			}

			dev_warn(info->dev, "%s: abandon ADC\n", __func__);
			return 0;
		}

		if (adc != ADC_JIG_UART_OFF) {
			dev_warn(info->dev, "%s: assume jig uart off detach\n",
					__func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;
		}
		break;
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
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_REMOTE_SWITCH)
	case CABLE_TYPE_REMOTE_SWITCH_MUIC:
		if (adc != ADC_REMOTE_SWITCH) {
			dev_warn(info->dev, "%s: assume remote switch detach \n",
					__func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;
		}
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_REMOTE_SWITCH */
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

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
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
		max77693_muic_attach_mhl(info, chgtyp);
		return 0;
	}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */

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
		mdata->jig_state(true);
		break;
	case ADC_JIG_USB_ON:
		if (vbvolt & STATUS2_VBVOLT_MASK) {
			dev_info(info->dev, "%s: SKIP_JIG_USB\n", __func__);
			ret = max77693_muic_attach_usb_type(info, adc);
		}
		mdata->jig_state(true);
		break;
	/* ADC_CARDOCK == ADC_JIG_UART_ON */
	case ADC_CARDOCK:
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
		break;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_REMOTE_SWITCH)
	case ADC_REMOTE_SWITCH:
		if(info->cable_type == CABLE_TYPE_REMOTE_SWITCH_MUIC) {
			pr_info("%s:%s duplicated(REMOTE_SWITCH)\n",
					DEV_NAME, __func__);
			return 0;
		}
		pr_info("%s:%s REMOTE_SWITCH\n", DEV_NAME, __func__);
		info->cable_type = CABLE_TYPE_REMOTE_SWITCH_MUIC;
		max77693_muic_attach_remote_switch(info);
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_REMOTE_SWITCH */
	case ADC_JIG_USB_OFF:
	case ADC_CEA936ATYPE1_CHG:
	case ADC_CEA936ATYPE2_CHG:
	case ADC_OPEN:
		switch (chgtyp) {
		case CHGTYP_USB:
		case CHGTYP_DOWNSTREAM_PORT:
			if (adc == ADC_CEA936ATYPE2_CHG)
				break;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
			if (info->cable_type == CABLE_TYPE_MHL_MUIC) {
				dev_info(info->dev, "%s: MHL(charging)\n",
					 __func__);
				info->cable_type = CABLE_TYPE_MHL_VB_MUIC;
				ret = max77693_muic_set_charging_type(info,
								      false);
				return ret;
			}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
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

#ifdef CONFIG_USBHUB_USB3803
	/* setting usb hub in default mode (standby) */
	usb3803_set_mode(USB_3803_MODE_STANDBY);
#endif  /* CONFIG_USBHUB_USB3803 */

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
	case CABLE_TYPE_TA_MUIC:
		dev_info(info->dev, "%s: TA\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		ret = max77693_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_TA_MUIC;
		break;
	case CABLE_TYPE_JIG_UART_ON_MUIC:
		if (info->is_factory_start) {
			pr_info("%s:%s JIG_UART_ON\n", DEV_NAME, __func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			if (mdata->dock_cb)
				mdata->dock_cb(MAX77693_MUIC_DOCK_DETACHED);
		}
		break;
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
		dev_info(info->dev, "%s: JIG UART/BOOTOFF\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		break;
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
		dev_info(info->dev, "%s: JIG UART/OFF/VB\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		ret = max77693_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_JIG_UART_OFF_VB_MUIC;
		break;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_REMOTE_SWITCH)
	case CABLE_TYPE_REMOTE_SWITCH_MUIC:
		pr_info("%s:%s: REMOTE_SWITCH\n", DEV_NAME, __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_REMOTE_SWITCH */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
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
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
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

	if (adclow && adc1k) {
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
		pr_info("%s:%s MHL cable connected\n", DEV_NAME, __func__);
		intr = INT_ATTACH;
#else
		pr_info("%s:%s MHL dongle is not supported, just detach.\n",
				DEV_NAME, __func__);
		intr = INT_DETACH;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
		return intr;
	}

	switch (adc) {
	case ADC_GND:
		if (info->is_otg_attach_blocked) {
			pr_warn("%s:%s otg attach is blocked, ignore\n",
					DEV_NAME, __func__);
			return -1;
		}
		break;
	case ADC_MHL ... (ADC_SMARTDOCK + 1):
#if !defined(CONFIG_MUIC_MAX77693_SUPPORT_REMOTE_SWITCH)
	case ADC_REMOTE_SWITCH:
#endif /* !CONFIG_MUIC_MAX77693_SUPPORT_REMOTE_SWITCH */
	case ADC_POWER_SHARING - 1:
#if !defined(CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE)
	case ADC_POWER_SHARING:
#endif /* !CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE */
	case (ADC_POWER_SHARING + 1) ... (ADC_CEA936ATYPE1_CHG - 1):
	case ADC_DESKDOCK:
	case ADC_JIG_UART_ON ... (ADC_OPEN - 1):
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
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE)
				case CABLE_TYPE_POWER_SHARING_MUIC:
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_PS_CABLE */
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
	int ret;
	u8 val;

	dev_info(info->dev, "%s: system_rev=%x\n", __func__, system_rev);

	/* INTMASK1  3:ADC1K 2:ADCErr 1:ADCLow 0:ADC */
	/* INTMASK2  0:Chgtype */
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	max77693_write_reg(info->muic, MAX77693_MUIC_REG_INTMASK1, 0x09);
#else
	max77693_write_reg(info->muic, MAX77693_MUIC_REG_INTMASK1, 0x01);
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
	max77693_write_reg(info->muic, MAX77693_MUIC_REG_INTMASK2, 0x11);
	max77693_write_reg(info->muic, MAX77693_MUIC_REG_INTMASK3, 0x00);

	REQUEST_IRQ(info->irq_adc, "muic-adc");
	REQUEST_IRQ(info->irq_chgtype, "muic-chgtype");
	REQUEST_IRQ(info->irq_vbvolt, "muic-vbvolt");
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	REQUEST_IRQ(info->irq_adc1k, "muic-adc1k");

	dev_info(info->dev, "adc:%d chgtype:%d adc1k:%d vbvolt:%d",
		info->irq_adc, info->irq_chgtype,
		info->irq_adc1k, info->irq_vbvolt);
#else
	dev_info(info->dev, "adc:%d chgtype:%d vbvolt:%d",
		info->irq_adc, info->irq_chgtype,
		info->irq_vbvolt);
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */

	max77693_read_reg(info->muic, MAX77693_MUIC_REG_INTMASK1, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INTMASK1, val);

	max77693_read_reg(info->muic, MAX77693_MUIC_REG_INTMASK2, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INTMASK2, val);

	max77693_read_reg(info->muic, MAX77693_MUIC_REG_INTMASK3, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INTMASK3, val);

	max77693_read_reg(info->muic, MAX77693_MUIC_REG_INT1, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INT1, val);
	max77693_read_reg(info->muic, MAX77693_MUIC_REG_INT2, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INT2, val);
	max77693_read_reg(info->muic, MAX77693_MUIC_REG_INT3, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INT3, val);
	return 0;
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
		info->cable_type == CABLE_TYPE_MHL_VB_MUIC) {
		if (mdata->mhl_cb)
			mdata->mhl_cb(MAX77693_MUIC_ATTACHED);
	}
	mutex_unlock(&info->mutex);
}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */

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

static int __devinit max77693_muic_probe(struct platform_device *pdev)
{
	struct max77693_dev *max77693 = dev_get_drvdata(pdev->dev.parent);
	struct max77693_platform_data *pdata = dev_get_platdata(max77693->dev);
	struct max77693_muic_info *info;
	int switch_sel;
	u8 cntl1_val, cntl2_val, cntl3_val;
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
	info->irq_adc = max77693->irq_base + MAX77693_MUIC_IRQ_INT1_ADC;
	info->irq_chgtype = max77693->irq_base + MAX77693_MUIC_IRQ_INT2_CHGTYP;
	info->irq_vbvolt = max77693->irq_base + MAX77693_MUIC_IRQ_INT2_VBVOLT;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	info->irq_adc1k = max77693->irq_base + MAX77693_MUIC_IRQ_INT1_ADC1K;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
	info->muic_data = pdata->muic;
	info->is_adc_open_prev = true;
	info->is_otg_attach_blocked = false;
	info->is_otg_test = false;
	info->is_factory_start = false;

	wake_lock_init(&info->muic_wake_lock, WAKE_LOCK_SUSPEND,
		"muic wake lock");

	info->cable_type = CABLE_TYPE_UNKNOWN_MUIC;
	info->muic_data->sw_path = AP_USB_MODE;
	gInfo = info;

	platform_set_drvdata(pdev, info);
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	dev_info(info->dev, "adc:%d chgtype:%d, adc1k:%d\n",
		 info->irq_adc, info->irq_chgtype, info->irq_adc1k);
#else
	dev_info(info->dev, "adc:%d chgtype:%d\n",
		 info->irq_adc, info->irq_chgtype);
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */

	dev_info(info->dev, "H/W rev connected UT1 UR2 pin to AP UART");

	switch_sel = get_switch_sel();

	if (switch_sel & MAX77693_SWITCH_SEL_1st_BIT_USB)
		info->muic_data->sw_path = AP_USB_MODE;
	else
		info->muic_data->sw_path = CP_USB_MODE;

	if (switch_sel & MAX77693_SWITCH_SEL_2nd_BIT_UART)
		info->muic_data->uart_path = UART_PATH_AP;
	else
		info->muic_data->uart_path = UART_PATH_CP;

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

	/* Set ADC debounce time: 25ms */
	max77693_muic_set_adcdbset(info, 2);

	max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL1, &cntl1_val);
	max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL2, &cntl2_val);
	max77693_read_reg(info->muic, MAX77693_MUIC_REG_CTRL3, &cntl3_val);
	pr_info("%s:%s CONTROL[1:0x%x, 2:0x%x, 3:0x%x]\n", DEV_NAME, __func__,
			cntl1_val, cntl2_val, cntl3_val);

	ret = max77693_muic_irq_init(info);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to initialize MUIC irq:%d\n", ret);
		goto fail;
	}

	/* init jig state */
	max77693_update_jig_state(info);

	/* initial cable detection */
	INIT_DELAYED_WORK(&info->init_work, max77693_muic_init_detect);
	schedule_delayed_work(&info->init_work, msecs_to_jiffies(3000));

	INIT_DELAYED_WORK(&info->usb_work, max77693_muic_usb_detect);
	schedule_delayed_work(&info->usb_work, msecs_to_jiffies(17000));

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	INIT_DELAYED_WORK(&info->mhl_work, max77693_muic_mhl_detect);
	schedule_delayed_work(&info->mhl_work, msecs_to_jiffies(25000));
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */

	return 0;

 fail:
	if (info->irq_adc)
		free_irq(info->irq_adc, NULL);
	if (info->irq_chgtype)
		free_irq(info->irq_chgtype, NULL);
	if (info->irq_vbvolt)
		free_irq(info->irq_vbvolt, NULL);
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	if (info->irq_adc1k)
		free_irq(info->irq_adc1k, NULL);
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
	mutex_destroy(&info->mutex);
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
		cancel_delayed_work(&info->init_work);
		cancel_delayed_work(&info->usb_work);
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
		cancel_delayed_work(&info->mhl_work);
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
		free_irq(info->irq_adc, info);
		free_irq(info->irq_chgtype, info);
		free_irq(info->irq_vbvolt, info);
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
		free_irq(info->irq_adc1k, info);
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
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
