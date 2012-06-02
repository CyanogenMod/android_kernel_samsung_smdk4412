/*
 * max8997-muic.c - MUIC driver for the Maxim 8997
 *
 *  Copyright (C) 2010 Samsung Electronics
 *  <ms925.kim@samsung.com>
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
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/mfd/max8997.h>
#include <linux/mfd/max8997-private.h>
#include <plat/udc-hs.h>
#ifdef CONFIG_USBHUB_USB3803
#include <linux/usb3803.h>
#endif


/* MAX8997 STATUS1 register */
#define STATUS1_ADC_SHIFT		0
#define STATUS1_ADCLOW_SHIFT		5
#define STATUS1_ADCERR_SHIFT		6
#define STATUS1_ADC_MASK		(0x1f << STATUS1_ADC_SHIFT)
#define STATUS1_ADCLOW_MASK		(0x1 << STATUS1_ADCLOW_SHIFT)
#define STATUS1_ADCERR_MASK		(0x1 << STATUS1_ADCERR_SHIFT)

/* MAX8997 STATUS2 register */
#define STATUS2_CHGTYP_SHIFT		0
#define STATUS2_CHGDETRUN_SHIFT		3
#define STATUS2_VBVOLT_SHIFT		6
#define STATUS2_CHGTYP_MASK		(0x7 << STATUS2_CHGTYP_SHIFT)
#define STATUS2_CHGDETRUN_MASK		(0x1 << STATUS2_CHGDETRUN_SHIFT)
#define STATUS2_VBVOLT_MASK		(0x1 << STATUS2_VBVOLT_SHIFT)

/* MAX8997 CDETCTRL register */
#define CHGDETEN_SHIFT			0
#define CHGTYPM_SHIFT			1
#define CHGDETEN_MASK			(0x1 << CHGDETEN_SHIFT)
#define CHGTYPM_MASK			(0x1 << CHGTYPM_SHIFT)

/* MAX8997 CONTROL1 register */
#define COMN1SW_SHIFT			0
#define COMP2SW_SHIFT			3
#define MICEN_SHIFT			6
#define COMN1SW_MASK			(0x7 << COMN1SW_SHIFT)
#define COMP2SW_MASK			(0x7 << COMP2SW_SHIFT)
#define MICEN_MASK			(0x1 << MICEN_SHIFT)

/* MAX8997 CONTROL2 register */
#define CTRL2_ACCDET_SHIFT		5
#define CTRL2_ACCDET_MASK		(0x1 << CTRL2_ACCDET_SHIFT)

/* MAX8997 CONTROL3 register */
#define CTRL3_JIGSET_SHIFT		0
#define CTRL3_BOOTSET_SHIFT		2
#define CTRL3_ADCDBSET_SHIFT		4
#define CTRL3_JIGSET_MASK		(0x3 << CTRL3_JIGSET_SHIFT)
#define CTRL3_BOOTSET_MASK		(0x3 << CTRL3_BOOTSET_SHIFT)
#define CTRL3_ADCDBSET_MASK		(0x3 << CTRL3_ADCDBSET_SHIFT)

/* Interrupt 1 */
#define INT_DETACH		(0x1 << 1)
#define INT_ATTACH		(0x1 << 0)

/* MAX8997 MUIC CHG_TYP setting values */
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

enum {
	DOCK_KEY_NONE			= 0,
	DOCK_KEY_VOL_UP_PRESSED,
	DOCK_KEY_VOL_UP_RELEASED,
	DOCK_KEY_VOL_DOWN_PRESSED,
	DOCK_KEY_VOL_DOWN_RELEASED,
	DOCK_KEY_PREV_PRESSED,
	DOCK_KEY_PREV_RELEASED,
	DOCK_KEY_PLAY_PAUSE_PRESSED,
	DOCK_KEY_PLAY_PAUSE_RELEASED,
	DOCK_KEY_NEXT_PRESSED,
	DOCK_KEY_NEXT_RELEASED,
};

struct max8997_muic_info {
	struct device		*dev;
	struct max8997_dev	*max8997;
	struct i2c_client	*muic;
	struct max8997_muic_data *muic_data;
	int			irq_adc;
	int			irq_chgtype;
	int			irq_vbvolt;
	int			irq_adcerr;
	int			mansw;

	enum cable_type		cable_type;
	struct delayed_work	init_work;
	struct delayed_work	usb_work;
	struct delayed_work	mhl_work;
	struct mutex		mutex;
#if defined(CONFIG_MUIC_MAX8997_OVPUI)
	int			irq_chgins;
	int			irq_chgrm;
	bool			is_ovp_state;
#endif

	bool			is_usb_ready;
	bool			is_mhl_ready;

	struct input_dev	*input;
	int			previous_key;
};

#if 0
static void max8997_muic_dump_regs(struct max8997_muic_info *info)
{
	int i, ret;
	u8 val;

	for (i = 0; i < MAX8997_MUIC_REG_END; i++) {
		ret = max8997_read_reg(info->muic, i, &val);
		if (ret) {
			dev_err(info->dev, "%s: fail to read reg(0x%x)\n",
					__func__, i);
			continue;
		}
		dev_info(info->dev, "%s: ADDR : 0x%02x, DATA : 0x%02x\n",
				__func__, i, val);
	}
}
#endif

static ssize_t max8997_muic_show_usb_state(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);

	switch (info->cable_type) {
	case CABLE_TYPE_USB:
	case CABLE_TYPE_JIG_USB_OFF:
	case CABLE_TYPE_JIG_USB_ON:
		return sprintf(buf, "USB_STATE_CONFIGURED\n");
	default:
		break;
	}

	return sprintf(buf, "USB_STATE_NOTCONFIGURED\n");
}

static ssize_t max8997_muic_show_device(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);

	switch (info->cable_type) {
	case CABLE_TYPE_NONE:
		return sprintf(buf, "No cable\n");
	case CABLE_TYPE_USB:
		return sprintf(buf, "USB\n");
	case CABLE_TYPE_OTG:
		return sprintf(buf, "OTG\n");
	case CABLE_TYPE_TA:
		return sprintf(buf, "TA\n");
	case CABLE_TYPE_DESKDOCK:
		return sprintf(buf, "Desk Dock\n");
	case CABLE_TYPE_CARDOCK:
		return sprintf(buf, "Car Dock\n");
	case CABLE_TYPE_JIG_UART_OFF:
		return sprintf(buf, "JIG UART OFF\n");
	case CABLE_TYPE_JIG_UART_OFF_VB:
		return sprintf(buf, "JIG UART OFF/VB\n");
	case CABLE_TYPE_JIG_UART_ON:
		return sprintf(buf, "JIG UART ON\n");
	case CABLE_TYPE_JIG_USB_OFF:
		return sprintf(buf, "JIG USB OFF\n");
	case CABLE_TYPE_JIG_USB_ON:
		return sprintf(buf, "JIG USB ON\n");
	case CABLE_TYPE_MHL:
		return sprintf(buf, "mHL\n");
	case CABLE_TYPE_MHL_VB:
		return sprintf(buf, "mHL charging\n");
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

static ssize_t max8997_muic_show_manualsw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);

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

static ssize_t max8997_muic_set_manualsw(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);

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

static ssize_t max8997_muic_show_adc(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = max8997_read_reg(info->muic, MAX8997_MUIC_REG_STATUS1, &val);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}

	return sprintf(buf, "%x\n", (val & STATUS1_ADC_MASK));
}

static ssize_t max8997_muic_show_audio_path(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = max8997_read_reg(info->muic, MAX8997_MUIC_REG_CTRL1, &val);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}

	return sprintf(buf, "%x\n", val);
}

static ssize_t max8997_muic_set_audio_path(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->muic;
	u8 cntl1_val, cntl1_msk;
	u8 val;

	if (!strncmp(buf, "0", 1))
		val = 0;
	else if (!strncmp(buf, "1", 1))
		val = 2;
	else {
		dev_warn(info->dev, "%s: Wrong command\n", __func__);
		return count;
	}

	cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT) |
			    (0 << MICEN_SHIFT);
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK | MICEN_MASK;

	max8997_update_reg(client, MAX8997_MUIC_REG_CTRL1, cntl1_val,
				   cntl1_msk);

	cntl1_val = 0;
	max8997_read_reg(client, MAX8997_MUIC_REG_CTRL1, &cntl1_val);
	dev_info(info->dev, "%s: CNTL1(0x%02x)\n", __func__, cntl1_val);

	return count;
}

static ssize_t max8997_muic_show_otg_test(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = max8997_read_reg(info->muic, MAX8997_MUIC_REG_CDETCTRL, &val);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	val &= CHGDETEN_MASK;

	return sprintf(buf, "%x\n", val);
}

static ssize_t max8997_muic_set_otg_test(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->muic;
	u8 val;

	if (!strncmp(buf, "0", 1))
		val = 0;
	else if (!strncmp(buf, "1", 1))
		val = 1;
	else {
		dev_warn(info->dev, "%s: Wrong command\n", __func__);
		return count;
	}

	max8997_update_reg(client, MAX8997_MUIC_REG_CDETCTRL,
			val << CHGDETEN_SHIFT, CHGDETEN_MASK);

	val = 0;
	max8997_read_reg(client, MAX8997_MUIC_REG_CDETCTRL, &val);
	dev_info(info->dev, "%s: CDETCTRL(0x%02x)\n", __func__, val);

	return count;
}

static void max8997_muic_set_adcdbset(struct max8997_muic_info *info,
					int value)
{
	int ret;
	u8 val;

	if (value > 3) {
		dev_err(info->dev, "%s: invalid value(%d)\n", __func__, value);
		return;
	}

	if (!info->muic) {
		dev_err(info->dev, "%s: no muic i2c client\n", __func__);
		return;
	}

	val = value << CTRL3_ADCDBSET_SHIFT;
	dev_info(info->dev, "%s: ADCDBSET(0x%02x)\n", __func__, val);

	ret = max8997_update_reg(info->muic, MAX8997_MUIC_REG_CTRL3, val,
			CTRL3_ADCDBSET_MASK);
	if (ret < 0)
		dev_err(info->dev, "%s: fail to update reg\n", __func__);
}

static ssize_t max8997_muic_show_adc_debounce_time(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	if (!info->muic)
		return sprintf(buf, "No I2C client\n");

	ret = max8997_read_reg(info->muic, MAX8997_MUIC_REG_CTRL3, &val);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	val &= CTRL3_ADCDBSET_MASK;
	val = val >> CTRL3_ADCDBSET_SHIFT;

	return sprintf(buf, "%x\n", val);
}

static ssize_t max8997_muic_set_adc_debounce_time(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);

	value = (value & 0x3);

#if 0
	max8997_muic_set_adcdbset(info, value);
#else
	dev_info(info->dev, "%s: Do nothing\n", __func__);
#endif

	return count;
}

static ssize_t max8997_muic_show_status(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);
	u8 devid, int_value[3], status[3], intmask[3], ctrl[3], cdetctrl;

	max8997_read_reg(info->muic, MAX8997_MUIC_REG_ID, &devid);
	max8997_bulk_read(info->muic, MAX8997_MUIC_REG_INT1, 3, int_value);
	max8997_bulk_read(info->muic, MAX8997_MUIC_REG_STATUS1, 3, status);
	max8997_bulk_read(info->muic, MAX8997_MUIC_REG_INTMASK1, 3, intmask);
	max8997_bulk_read(info->muic, MAX8997_MUIC_REG_CTRL1, 3, ctrl);
	max8997_read_reg(info->muic, MAX8997_MUIC_REG_CDETCTRL, &cdetctrl);

	return sprintf(buf,
		       "Device ID(0x%02x)\n"
			   "INT1(0x%02x), INT2(0x%02x), INT3(0x%02x)\n"
		       "STATUS1(0x%02x), STATUS2(0x%02x), STATUS3(0x%02x)\n"
		       "INTMASK1(0x%02x), INTMASK2(0x%02x), INTMASK3(0x%02x)\n"
		       "CTRL1(0x%02x), CTRL2(0x%02x), CTRL3(0x%02x)\n"
		       "CDETCTRL(0x%02x)\n",
		       devid, int_value[0], int_value[1], int_value[2],
		       status[0], status[1], status[2],
		       intmask[0], intmask[1], intmask[2],
		       ctrl[0], ctrl[1], ctrl[2],
		       cdetctrl);
}

static ssize_t max8997_muic_set_status(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);
	u8  reg_value[3];
	char reg_name[5];

	if (sscanf(buf, "%4s:%02x,%02x,%02x" , reg_name, (u32 *)&reg_value[0],
		(u32 *)&reg_value[1], (u32 *)&reg_value[2])) {
		if (!strncmp(reg_name, "INTM", 4)) {
			dev_info(dev, "Manual Set INTMASK to 0x%02x, 0x%02x, 0x%02x\n",
				reg_value[0], reg_value[1], reg_value[2]);
			max8997_bulk_write(info->muic,
				MAX8997_MUIC_REG_INTMASK1, 3, reg_value);
		} else if (!strncmp(reg_name, "CONT", 4)) {
			dev_info(dev, "Manual Set CONTROL to 0x%02x, 0x%02x, 0x%02x\n",
				reg_value[0], reg_value[1], reg_value[2]);
			max8997_bulk_write(info->muic,
				MAX8997_MUIC_REG_CTRL1, 3, reg_value);
		} else if (!strncmp(reg_name, "CDET", 4)) {
			dev_info(dev, "Manual Set CDETCTRL to 0x%02x\n",
				reg_value[0]);
			max8997_write_reg(info->muic,
				MAX8997_MUIC_REG_CDETCTRL, reg_value[0]);
		} else {
			dev_info(dev, "Unsupported CMD format\n");
		}
	} else {
		dev_info(dev, "Read CMD fail\n");
	}

	return count;
}

static DEVICE_ATTR(usb_state, S_IRUGO, max8997_muic_show_usb_state, NULL);
static DEVICE_ATTR(device, S_IRUGO, max8997_muic_show_device, NULL);
static DEVICE_ATTR(usb_sel, 0664,
		max8997_muic_show_manualsw, max8997_muic_set_manualsw);
static DEVICE_ATTR(adc, S_IRUGO, max8997_muic_show_adc, NULL);
static DEVICE_ATTR(audio_path, 0664,
		max8997_muic_show_audio_path, max8997_muic_set_audio_path);
static DEVICE_ATTR(otg_test, 0664,
		max8997_muic_show_otg_test, max8997_muic_set_otg_test);
static DEVICE_ATTR(adc_debounce_time, 0664,
		max8997_muic_show_adc_debounce_time,
		max8997_muic_set_adc_debounce_time);
static DEVICE_ATTR(status, 0664,
			max8997_muic_show_status, max8997_muic_set_status);

static struct attribute *max8997_muic_attributes[] = {
	&dev_attr_usb_state.attr,
	&dev_attr_device.attr,
	&dev_attr_usb_sel.attr,
	&dev_attr_adc.attr,
	&dev_attr_audio_path.attr,
	&dev_attr_otg_test.attr,
	&dev_attr_adc_debounce_time.attr,
	&dev_attr_status.attr,
	NULL
};

static const struct attribute_group max8997_muic_group = {
	.attrs = max8997_muic_attributes,
};

static int max8997_muic_set_usb_path(struct max8997_muic_info *info, int path)
{
	struct i2c_client *client = info->muic;
	struct max8997_muic_data *mdata = info->muic_data;
	int ret;
	int gpio_val;
	u8 accdet, cntl1_val, cntl1_msk = 0, cntl2_val;

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
		gpio_val = 0;
		accdet = 1;

		if (info->cable_type == CABLE_TYPE_OTG) {
			accdet = 0;
			/* DN1, DP2 */
			cntl1_val = (1 << COMN1SW_SHIFT) | (1 << COMP2SW_SHIFT);
			cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
		}
		break;
	case CP_USB_MODE:
		dev_info(info->dev, "%s: CP_USB_MODE\n", __func__);
		gpio_val = 1;
		accdet = 0;
		/* UT1, UR2 */
		cntl1_val = (3 << COMN1SW_SHIFT) | (3 << COMP2SW_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
		break;
	case AUDIO_MODE:
		dev_info(info->dev, "%s: AUDIO_MODE\n", __func__);
		gpio_val = 0;
		accdet = 0;
		/* SL1, SR2 */
		cntl1_val = (2 << COMN1SW_SHIFT) | (2 << COMP2SW_SHIFT) |
			    (0 << MICEN_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK | MICEN_MASK;
		break;
	default:
		dev_warn(info->dev, "%s: invalid path(%d)\n", __func__, path);
		return -EINVAL;
	}

#if !defined(CONFIG_TARGET_LOCALE_NA) && !defined(CONFIG_MACH_U1CAMERA_BD)
	if (gpio_is_valid(info->muic_data->gpio_usb_sel))
		gpio_direction_output(mdata->gpio_usb_sel, gpio_val);
#endif /* !CONFIG_TARGET_LOCALE_NA && !CONFIG_MACH_U1CAMERA_BD */
	/* Enable/Disable Factory Accessory Detection State Machine */
	cntl2_val = accdet << CTRL2_ACCDET_SHIFT;
	max8997_update_reg(client, MAX8997_MUIC_REG_CTRL2, cntl2_val,
			CTRL2_ACCDET_MASK);

	if (!accdet) {
		dev_info(info->dev, "%s: Set manual path\n", __func__);
		max8997_update_reg(client, MAX8997_MUIC_REG_CTRL1, cntl1_val,
				   cntl1_msk);

		cntl1_val = 0;
		max8997_read_reg(client, MAX8997_MUIC_REG_CTRL1, &cntl1_val);
		dev_info(info->dev, "%s: CNTL1(0x%02x)\n", __func__, cntl1_val);
	}

	return 0;
}

static int max8997_muic_set_charging_type(struct max8997_muic_info *info,
					  bool force_disable)
{
	struct max8997_muic_data *mdata = info->muic_data;
	int ret = 0;

	if (mdata->charger_cb) {
		if (force_disable)
			ret = mdata->charger_cb(CABLE_TYPE_NONE);
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

static int max8997_muic_handle_dock_vol_key(struct max8997_muic_info *info,
					u8 status1)
{
	struct input_dev *input = info->input;
	int pre_key = info->previous_key;
	unsigned int code;
	int state;
	u8 adc;

	adc = status1 & STATUS1_ADC_MASK;

	if (info->cable_type != CABLE_TYPE_DESKDOCK)
		return 0;

	if (adc == ADC_OPEN) {
		switch (pre_key) {
		case DOCK_KEY_VOL_UP_PRESSED:
			code = KEY_VOLUMEUP;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_UP_RELEASED;
			break;
		case DOCK_KEY_VOL_DOWN_PRESSED:
			code = KEY_VOLUMEDOWN;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_DOWN_RELEASED;
			break;
		case DOCK_KEY_PREV_PRESSED:
			code = KEY_PREVIOUSSONG;
			state = 0;
			info->previous_key = DOCK_KEY_PREV_RELEASED;
			break;
		case DOCK_KEY_PLAY_PAUSE_PRESSED:
			code = KEY_PLAYPAUSE;
			state = 0;
			info->previous_key = DOCK_KEY_PLAY_PAUSE_RELEASED;
			break;
		case DOCK_KEY_NEXT_PRESSED:
			code = KEY_NEXTSONG;
			state = 0;
			info->previous_key = DOCK_KEY_NEXT_RELEASED;
			break;
		default:
			return 0;
		}
		input_event(input, EV_KEY, code, state);
		input_sync(input);
		return 0;
	}

	if (pre_key == DOCK_KEY_NONE) {
		/*
		if (adc != ADC_DOCK_VOL_UP && adc != ADC_DOCK_VOL_DN && \
		adc != ADC_DOCK_PREV_KEY && adc != ADC_DOCK_PLAY_PAUSE_KEY \
		&& adc != ADC_DOCK_NEXT_KEY)
		*/
		if ((adc < 0x03) || (adc > 0x0d))
			return 0;
	}

	dev_info(info->dev, "%s: dock vol key(%d)\n", __func__, pre_key);

	switch (adc) {
	case ADC_DOCK_VOL_UP:
		code = KEY_VOLUMEUP;
		state = 1;
		info->previous_key = DOCK_KEY_VOL_UP_PRESSED;
		break;
	case ADC_DOCK_VOL_DN:
		code = KEY_VOLUMEDOWN;
		state = 1;
		info->previous_key = DOCK_KEY_VOL_DOWN_PRESSED;
		break;
	case ADC_DOCK_PREV_KEY-1 ... ADC_DOCK_PREV_KEY+1:
		code = KEY_PREVIOUSSONG;
		state = 1;
		info->previous_key = DOCK_KEY_PREV_PRESSED;
		break;
	case ADC_DOCK_PLAY_PAUSE_KEY-1 ... ADC_DOCK_PLAY_PAUSE_KEY+1:
		code = KEY_PLAYPAUSE;
		state = 1;
		info->previous_key = DOCK_KEY_PLAY_PAUSE_PRESSED;
		break;
	case ADC_DOCK_NEXT_KEY-1 ... ADC_DOCK_NEXT_KEY+1:
		code = KEY_NEXTSONG;
		state = 1;
		info->previous_key = DOCK_KEY_NEXT_PRESSED;
		break;
	case ADC_DESKDOCK: /* key release routine */
		if (pre_key == DOCK_KEY_VOL_UP_PRESSED) {
			code = KEY_VOLUMEUP;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_UP_RELEASED;
		} else if (pre_key == DOCK_KEY_VOL_DOWN_PRESSED) {
			code = KEY_VOLUMEDOWN;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_DOWN_RELEASED;
		} else if (pre_key == DOCK_KEY_PREV_PRESSED) {
			code = KEY_PREVIOUSSONG;
			state = 0;
			info->previous_key = DOCK_KEY_PREV_RELEASED;
		} else if (pre_key == DOCK_KEY_PLAY_PAUSE_PRESSED) {
			code = KEY_PLAYPAUSE;
			state = 0;
			info->previous_key = DOCK_KEY_PLAY_PAUSE_RELEASED;
		} else if (pre_key == DOCK_KEY_NEXT_PRESSED) {
			code = KEY_NEXTSONG;
			state = 0;
			info->previous_key = DOCK_KEY_NEXT_RELEASED;
		} else {
			dev_warn(info->dev, "%s:%d should not reach here\n",
					__func__, __LINE__);
			return 0;
		}
		break;
	default:
		dev_warn(info->dev, "%s: unsupported ADC(0x%02x)\n", __func__,
				adc);
		return 0;
	}

	input_event(input, EV_KEY, code, state);
	input_sync(input);

	return 1;
}

static int max8997_muic_attach_usb_type(struct max8997_muic_info *info, int adc)
{
	struct max8997_muic_data *mdata = info->muic_data;
	int ret, path;

	if (info->cable_type == CABLE_TYPE_MHL ||
			info->cable_type == CABLE_TYPE_MHL_VB) {
		dev_warn(info->dev, "%s: mHL was attached!\n", __func__);
		return 0;
	}

	switch (adc) {
	case ADC_JIG_USB_OFF:
		if (info->cable_type == CABLE_TYPE_JIG_USB_OFF) {
			dev_info(info->dev, "%s: duplicated(JIG USB OFF)\n",
					__func__);
			return 0;
		}

		dev_info(info->dev, "%s:JIG USB BOOTOFF\n", __func__);
		info->cable_type = CABLE_TYPE_JIG_USB_OFF;
		path = AP_USB_MODE;
		break;
	case ADC_JIG_USB_ON:
		if (info->cable_type == CABLE_TYPE_JIG_USB_ON) {
			dev_info(info->dev, "%s: duplicated(JIG USB ON)\n",
					__func__);
			return 0;
		}

		dev_info(info->dev, "%s:JIG USB BOOTON\n", __func__);
		info->cable_type = CABLE_TYPE_JIG_USB_ON;
		path = AP_USB_MODE;
		break;
	case ADC_OPEN:
		if (info->cable_type == CABLE_TYPE_USB) {
			dev_info(info->dev, "%s: duplicated(USB)\n", __func__);
			return 0;
		}

		dev_info(info->dev, "%s:USB\n", __func__);
		info->cable_type = CABLE_TYPE_USB;
		path = AP_USB_MODE;
		break;
	default:
		dev_info(info->dev, "%s: Unkown cable(0x%x)\n", __func__, adc);
		return 0;
	}

	ret = max8997_muic_set_charging_type(info, false);
	if (ret) {
		info->cable_type = CABLE_TYPE_NONE;
		return ret;
	}

	if (mdata->sw_path == CP_USB_MODE) {
		info->cable_type = CABLE_TYPE_USB;
		max8997_muic_set_usb_path(info, CP_USB_MODE);
		return 0;
	}

	max8997_muic_set_usb_path(info, path);

	if ((path == AP_USB_MODE) && (adc == ADC_OPEN)) {
		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_CABLE_ATTACHED);
	}

	return 0;
}

static int max8997_muic_attach_dock_type(struct max8997_muic_info *info,
					 int adc)
{
	struct max8997_muic_data *mdata = info->muic_data;
	int path;

	switch (adc) {
	case ADC_DESKDOCK:
		/* Desk Dock */
		if (info->cable_type == CABLE_TYPE_DESKDOCK) {
			dev_info(info->dev, "%s: duplicated(DeskDock)\n",
					__func__);
			return 0;
		}
		dev_info(info->dev, "%s:DeskDock\n", __func__);
		info->cable_type = CABLE_TYPE_DESKDOCK;
		path = AUDIO_MODE;

		if (mdata->deskdock_cb)
			mdata->deskdock_cb(MAX8997_MUIC_ATTACHED);
		break;
	case ADC_CARDOCK:
		/* Car Dock */
		if (info->cable_type == CABLE_TYPE_CARDOCK) {
			dev_info(info->dev, "%s: duplicated(CarDock)\n",
					__func__);
			return 0;
		}
		dev_info(info->dev, "%s:CarDock\n", __func__);
		info->cable_type = CABLE_TYPE_CARDOCK;
		path = AUDIO_MODE;

		if (mdata->cardock_cb)
			mdata->cardock_cb(MAX8997_MUIC_ATTACHED);
		break;
	default:
		dev_info(info->dev, "%s: should not reach here(0x%x)\n",
				__func__, adc);
		return 0;
	}

	max8997_muic_set_usb_path(info, path);

	return 0;
}

static void max8997_muic_attach_mhl(struct max8997_muic_info *info, u8 chgtyp)
{
	struct max8997_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "%s\n", __func__);

	if (info->cable_type == CABLE_TYPE_USB) {
		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_CABLE_DETACHED);

		max8997_muic_set_charging_type(info, true);
	}
#if 0
	if (info->cable_type == CABLE_TYPE_MHL) {
		dev_info(info->dev, "%s: duplicated(MHL)\n", __func__);
		return;
	}
#endif
	info->cable_type = CABLE_TYPE_MHL;

	if (mdata->mhl_cb && info->is_mhl_ready)
		mdata->mhl_cb(MAX8997_MUIC_ATTACHED);

	if (chgtyp == CHGTYP_USB) {
		info->cable_type = CABLE_TYPE_MHL_VB;
		max8997_muic_set_charging_type(info, false);
	}
}

/* TODO : should be removed */
#define NOTIFY_TEST_MODE	3

static void max8997_muic_handle_jig_uart(struct max8997_muic_info *info,
					 u8 vbvolt)
{
	struct max8997_muic_data *mdata = info->muic_data;
	enum cable_type prev_ct = info->cable_type;
	bool is_otgtest = false;
	u8 cntl1_val, cntl1_msk;

	dev_info(info->dev, "%s: JIG UART/BOOTOFF(0x%x)\n", __func__, vbvolt);

#if defined(CONFIG_SEC_MODEM_M0_TD)
	gpio_set_value(GPIO_AP_CP_INT1, 1);
#endif

	/* UT1, UR2 */
	cntl1_val = (3 << COMN1SW_SHIFT) | (3 << COMP2SW_SHIFT);
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
	max8997_update_reg(info->muic, MAX8997_MUIC_REG_CTRL1, cntl1_val,
			   cntl1_msk);

	if (vbvolt & STATUS2_VBVOLT_MASK) {
		if (mdata->host_notify_cb) {
			if (mdata->host_notify_cb(1) == NOTIFY_TEST_MODE) {
				is_otgtest = true;
				dev_info(info->dev, "%s: OTG TEST\n", __func__);
			}
		}

		info->cable_type = CABLE_TYPE_JIG_UART_OFF_VB;
		max8997_muic_set_charging_type(info, is_otgtest);

	} else {
		info->cable_type = CABLE_TYPE_JIG_UART_OFF;
#if 0
		if (mdata->uart_path == UART_PATH_CP &&
				mdata->jig_uart_cb)
			mdata->jig_uart_cb(UART_PATH_CP);
#endif
		if (prev_ct == CABLE_TYPE_JIG_UART_OFF_VB) {
			max8997_muic_set_charging_type(info, false);

			if (mdata->host_notify_cb)
				mdata->host_notify_cb(0);
		}
	}
}

static int max8997_muic_handle_attach(struct max8997_muic_info *info,
		u8 status1, u8 status2)
{
	struct max8997_muic_data *mdata = info->muic_data;
	u8 adc, adclow, adcerr, chgtyp, vbvolt, chgdetrun;
	int ret = 0;
#ifdef CONFIG_USBHUB_USB3803
	/* setting usb hub in Diagnostic(hub) mode */
	usb3803_set_mode(USB_3803_MODE_HUB);
#endif /* CONFIG_USBHUB_USB3803 */

	adc = status1 & STATUS1_ADC_MASK;
	adclow = status1 & STATUS1_ADCLOW_MASK;
	adcerr = status1 & STATUS1_ADCERR_MASK;
	chgtyp = status2 & STATUS2_CHGTYP_MASK;
	vbvolt = status2 & STATUS2_VBVOLT_MASK;
	chgdetrun = status2 & STATUS2_CHGDETRUN_MASK;

	switch (info->cable_type) {
	case CABLE_TYPE_JIG_UART_OFF:
	case CABLE_TYPE_JIG_UART_OFF_VB:
		/* Workaround for Factory mode.
		 * Abandon adc interrupt of approximately +-100K range
		 * if previous cable status was JIG UART BOOT OFF.
		 */
		if (adc == (ADC_JIG_UART_OFF + 1) ||
				adc == (ADC_JIG_UART_OFF - 1)) {
			dev_warn(info->dev, "%s: abandon ADC\n", __func__);
			return 0;
		}

		if (adcerr) {
			dev_warn(info->dev, "%s: current state is jig_uart_off,"
				"just ignore\n", __func__);
			return 0;
		}

		if (adc != ADC_JIG_UART_OFF) {
			if (info->cable_type == CABLE_TYPE_JIG_UART_OFF_VB) {
				dev_info(info->dev, "%s: adc != JIG_UART_OFF, remove JIG UART/OFF/VB\n", __func__);
				info->cable_type = CABLE_TYPE_NONE;
				max8997_muic_set_charging_type(info, false);
			} else {
				dev_info(info->dev, "%s: adc != JIG_UART_OFF, remove JIG UART/BOOTOFF\n", __func__);
				info->cable_type = CABLE_TYPE_NONE;
			}
		}
		break;

	case CABLE_TYPE_DESKDOCK:
		if (adcerr || (adc != ADC_DESKDOCK)) {
			if (adcerr)
				dev_err(info->dev, "%s: ADC err occured(DESKDOCK)\n", __func__);
			else
				dev_warn(info->dev, "%s: ADC != DESKDOCK, remove DESKDOCK\n", __func__);

			info->cable_type = CABLE_TYPE_NONE;

			max8997_muic_set_charging_type(info, false);

			if (mdata->deskdock_cb)
				mdata->deskdock_cb(MAX8997_MUIC_DETACHED);

			if (adcerr)
				return 0;
		}
		break;

	case CABLE_TYPE_CARDOCK:
		if (adcerr || (adc != ADC_CARDOCK)) {
			if (adcerr)
				dev_err(info->dev, "%s: ADC err occured(CARDOCK)\n", __func__);
			else
				dev_warn(info->dev, "%s: ADC != CARDOCK, remove CARDOCK\n", __func__);

			info->cable_type = CABLE_TYPE_NONE;

			max8997_muic_set_charging_type(info, false);

			if (mdata->cardock_cb)
				mdata->cardock_cb(MAX8997_MUIC_DETACHED);

			if (adcerr)
				return 0;
		}
		break;

	default:
		break;
	}

	/* 1Kohm ID regiter detection (mHL)
	 * Old MUIC : ADC value:0x00 or 0x01, ADCLow:1
	 * New MUIC : ADC value is not set(Open), ADCLow:1, ADCError:1
	 */
	if (adclow && adcerr) {
		max8997_muic_attach_mhl(info, chgtyp);
		return 0;
	}

	switch (adc) {
	case ADC_GND:
#if defined(CONFIG_MACH_U1)
		/* This is for support old MUIC */
		if (adclow) {
			max8997_muic_attach_mhl(info, chgtyp);
			break;
		}
#endif

		if (chgtyp == CHGTYP_NO_VOLTAGE) {
			if (info->cable_type == CABLE_TYPE_OTG) {
				dev_info(info->dev,
						"%s: duplicated(OTG)\n",
						__func__);
				break;
			}

			info->cable_type = CABLE_TYPE_OTG;
			max8997_muic_set_usb_path(info, AP_USB_MODE);
			if (mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_OTGHOST_ATTACHED);
		} else if (chgtyp == CHGTYP_USB ||
				chgtyp == CHGTYP_DOWNSTREAM_PORT ||
				chgtyp == CHGTYP_DEDICATED_CHGR ||
				chgtyp == CHGTYP_500MA	||
				chgtyp == CHGTYP_1A) {
			dev_info(info->dev, "%s: OTG charging pump\n",
					__func__);
			ret = max8997_muic_set_charging_type(info, false);
		}
		break;
	case ADC_MHL:
#if defined(CONFIG_MACH_U1)
		/* This is for support old MUIC */
		max8997_muic_attach_mhl(info, chgtyp);
#endif
		break;
	case ADC_JIG_UART_OFF:
		max8997_muic_handle_jig_uart(info, vbvolt);
		break;
	case ADC_JIG_USB_OFF:
	case ADC_JIG_USB_ON:
		if (vbvolt & STATUS2_VBVOLT_MASK)
			ret = max8997_muic_attach_usb_type(info, adc);
		break;
	case ADC_DESKDOCK:
	case ADC_CARDOCK:
		max8997_muic_attach_dock_type(info, adc);
		if (chgtyp == CHGTYP_USB ||
				chgtyp == CHGTYP_DOWNSTREAM_PORT ||
				chgtyp == CHGTYP_DEDICATED_CHGR ||
				chgtyp == CHGTYP_500MA	||
				chgtyp == CHGTYP_1A)
			ret = max8997_muic_set_charging_type(info, false);
		else if (chgtyp == CHGTYP_NO_VOLTAGE && !chgdetrun)
			ret = max8997_muic_set_charging_type(info, true);
		break;
	case ADC_CEA936ATYPE1_CHG:
	case ADC_CEA936ATYPE2_CHG:
	case ADC_OPEN:
		switch (chgtyp) {
		case CHGTYP_USB:
			if (adc == ADC_CEA936ATYPE1_CHG
					|| adc == ADC_CEA936ATYPE2_CHG)
				break;
			if (mdata->is_mhl_attached
					&& mdata->is_mhl_attached() &&
					info->cable_type == CABLE_TYPE_MHL) {
				dev_info(info->dev, "%s: MHL(charging)\n",
						__func__);
				info->cable_type = CABLE_TYPE_MHL_VB;
				ret = max8997_muic_set_charging_type(info,
						false);
				return ret;
			}
			ret = max8997_muic_attach_usb_type(info, adc);
			break;
		case CHGTYP_DOWNSTREAM_PORT:
		case CHGTYP_DEDICATED_CHGR:
		case CHGTYP_500MA:
		case CHGTYP_1A:
			dev_info(info->dev, "%s:TA\n", __func__);
			info->cable_type = CABLE_TYPE_TA;
#ifdef CONFIG_USBHUB_USB3803
			/* setting usb hub in default mode (standby) */
			usb3803_set_mode(USB_3803_MODE_STANDBY);
#endif  /* CONFIG_USBHUB_USB3803 */
			ret = max8997_muic_set_charging_type(info, false);
			if (ret)
				info->cable_type = CABLE_TYPE_NONE;
			break;
		default:
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

static int max8997_muic_handle_detach(struct max8997_muic_info *info)
{
	struct i2c_client *client = info->muic;
	struct max8997_muic_data *mdata = info->muic_data;
	enum cable_type prev_ct = CABLE_TYPE_NONE;
	int ret = 0;

#if defined(CONFIG_SEC_MODEM_M0_TD)
	gpio_set_value(GPIO_AP_CP_INT1, 0);
#endif

	/*
	 * MAX8996/8997-MUIC bug:
	 *
	 * Auto-switching COMN/P is not restored automatically when detached and
	 * remains undetermined state. UART(UT1, UR2) will be short (because TA
	 * D+/D- is short) if charger(TA) insertion is followed right after the
	 * JIG off. Reset CONTROL1 is needed when detaching cable.
	 */
	max8997_write_reg(client, MAX8997_MUIC_REG_CTRL1, 0x00);

	if (info->cable_type == CABLE_TYPE_MHL) {

		/* Enable Factory Accessory Detection State Machine */
		max8997_update_reg(client, MAX8997_MUIC_REG_CTRL2,
				(1 << CTRL2_ACCDET_SHIFT), CTRL2_ACCDET_MASK);
	}

#ifdef CONFIG_USBHUB_USB3803
	/* setting usb hub in default mode (standby) */
	usb3803_set_mode(USB_3803_MODE_STANDBY);
#endif  /* CONFIG_USBHUB_USB3803 */
	info->previous_key = DOCK_KEY_NONE;

	if (info->cable_type == CABLE_TYPE_NONE) {
		dev_info(info->dev, "%s: duplicated(NONE)\n", __func__);
		return 0;
	}
#if 0
	if (mdata->jig_uart_cb)
		mdata->jig_uart_cb(UART_PATH_AP);
#endif
	if (mdata->is_mhl_attached && mdata->is_mhl_attached()
			&& info->cable_type == CABLE_TYPE_MHL) {
		dev_info(info->dev, "%s: MHL attached. Do Nothing\n",
				__func__);
		return 0;
	}

	switch (info->cable_type) {
	case CABLE_TYPE_OTG:
		dev_info(info->dev, "%s: OTG\n", __func__);
		info->cable_type = CABLE_TYPE_NONE;

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_OTGHOST_DETACHED);
		break;
	case CABLE_TYPE_USB:
	case CABLE_TYPE_JIG_USB_OFF:
	case CABLE_TYPE_JIG_USB_ON:
		dev_info(info->dev, "%s: USB(0x%x)\n", __func__,
				info->cable_type);
		prev_ct = info->cable_type;
		info->cable_type = CABLE_TYPE_NONE;

		ret = max8997_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = prev_ct;
			break;
		}

		if (mdata->sw_path == CP_USB_MODE)
			return 0;

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_CABLE_DETACHED);
		break;
	case CABLE_TYPE_DESKDOCK:
		dev_info(info->dev, "%s: DESKDOCK\n", __func__);
		info->cable_type = CABLE_TYPE_NONE;

		ret = max8997_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = CABLE_TYPE_DESKDOCK;
			break;
		}

		if (mdata->deskdock_cb)
			mdata->deskdock_cb(MAX8997_MUIC_DETACHED);
		break;
	case CABLE_TYPE_CARDOCK:
		dev_info(info->dev, "%s: CARDOCK\n", __func__);
		info->cable_type = CABLE_TYPE_NONE;

		ret = max8997_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = CABLE_TYPE_CARDOCK;
			break;
		}

		if (mdata->cardock_cb)
			mdata->cardock_cb(MAX8997_MUIC_DETACHED);
		break;
	case CABLE_TYPE_TA:
		dev_info(info->dev, "%s: TA\n", __func__);
		info->cable_type = CABLE_TYPE_NONE;
		ret = max8997_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_TA;
		break;
	case CABLE_TYPE_JIG_UART_ON:
		dev_info(info->dev, "%s: JIG UART/BOOTON\n", __func__);
		info->cable_type = CABLE_TYPE_NONE;
		break;
	case CABLE_TYPE_JIG_UART_OFF:
		dev_info(info->dev, "%s: JIG UART/BOOTOFF\n", __func__);
		info->cable_type = CABLE_TYPE_NONE;
		break;
	case CABLE_TYPE_JIG_UART_OFF_VB:
		dev_info(info->dev, "%s: JIG UART/OFF/VB\n", __func__);
		info->cable_type = CABLE_TYPE_NONE;
		ret = max8997_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_JIG_UART_OFF_VB;
		break;
	case CABLE_TYPE_MHL:
		dev_info(info->dev, "%s: MHL\n", __func__);
		info->cable_type = CABLE_TYPE_NONE;
		break;
	case CABLE_TYPE_MHL_VB:
		dev_info(info->dev, "%s: MHL VBUS\n", __func__);
		info->cable_type = CABLE_TYPE_NONE;
		max8997_muic_set_charging_type(info, false);

		if (mdata->is_mhl_attached && mdata->is_mhl_attached()) {
			if (mdata->mhl_cb && info->is_mhl_ready)
				mdata->mhl_cb(MAX8997_MUIC_DETACHED);
		}
		break;
	case CABLE_TYPE_UNKNOWN:
		dev_info(info->dev, "%s: UNKNOWN\n", __func__);
		info->cable_type = CABLE_TYPE_NONE;

		ret = max8997_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_UNKNOWN;
		break;
	default:
		dev_info(info->dev, "%s:invalid cable type %d\n",
				__func__, info->cable_type);
		break;
	}
	return ret;
}

static void max8997_muic_detect_dev(struct max8997_muic_info *info, int irq)
{
	struct i2c_client *client = info->muic;
	u8 status[2];
	u8 adc, chgtyp, adcerr;
	int intr = INT_ATTACH;
	int ret;

	ret = max8997_bulk_read(client, MAX8997_MUIC_REG_STATUS1, 2, status);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg(%d)\n", __func__,
				ret);
		return;
	}

	dev_info(info->dev, "%s: STATUS1:0x%x, 2:0x%x\n", __func__,
			status[0], status[1]);

	if ((irq == info->irq_adc) &&
			max8997_muic_handle_dock_vol_key(info, status[0]))
		return;

	adc = status[0] & STATUS1_ADC_MASK;
	adcerr = status[0] & STATUS1_ADCERR_MASK;
	chgtyp = status[1] & STATUS2_CHGTYP_MASK;

	switch (adc) {
	case ADC_MHL:
#if defined(CONFIG_MACH_U1)
		break;
#endif
	case (ADC_MHL + 1):
	case (ADC_DOCK_VOL_DN - 1):
	case (ADC_DOCK_PLAY_PAUSE_KEY + 2) ... (ADC_CEA936ATYPE1_CHG - 1):
	case (ADC_CARDOCK + 1):
		dev_warn(info->dev, "%s: unsupported ADC(0x%02x)\n", __func__, adc);
		intr = INT_DETACH;
		break;
	case ADC_OPEN:
		if (!adcerr) {
			if (chgtyp == CHGTYP_NO_VOLTAGE)
				intr = INT_DETACH;
			else if (chgtyp == CHGTYP_USB ||
					chgtyp == CHGTYP_DOWNSTREAM_PORT ||
					chgtyp == CHGTYP_DEDICATED_CHGR ||
					chgtyp == CHGTYP_500MA	||
					chgtyp == CHGTYP_1A) {
				if (info->cable_type == CABLE_TYPE_OTG ||
					info->cable_type == CABLE_TYPE_DESKDOCK ||
					info->cable_type == CABLE_TYPE_CARDOCK)
					intr = INT_DETACH;
			}
		}
		break;
	default:
		break;
	}

#if defined(CONFIG_MUIC_MAX8997_OVPUI)
	if (intr == INT_ATTACH) {
		if (irq == info->irq_chgins) {
			if (info->is_ovp_state) {
				max8997_muic_handle_attach(info, status[0],
					status[1]);
				info->is_ovp_state = false;
				dev_info(info->dev, "OVP recovered\n");
				return;
			} else {
				dev_info(info->dev, "Just inserted TA/USB\n");
				return;
			}
		} else if (irq == info->irq_chgrm) {
			max8997_muic_handle_detach(info);
			info->is_ovp_state = true;
			dev_info(info->dev, "OVP occured\n");
			return;
		}

	} else	{
		info->is_ovp_state = false;

		if (irq == info->irq_chgrm) {
			dev_info(info->dev, "Just removed TA/USB\n");
			return;
		}
	}
#endif

	if (intr == INT_ATTACH) {
		dev_info(info->dev, "%s: ATTACHED\n", __func__);
		max8997_muic_handle_attach(info, status[0], status[1]);
	} else {
		dev_info(info->dev, "%s: DETACHED\n", __func__);
		max8997_muic_handle_detach(info);
	}
	return;
}

static irqreturn_t max8997_muic_irq(int irq, void *data)
{
	struct max8997_muic_info *info = data;
	dev_info(info->dev, "%s: irq:%d\n", __func__, irq);

	mutex_lock(&info->mutex);
	max8997_muic_detect_dev(info, irq);
	mutex_unlock(&info->mutex);

	return IRQ_HANDLED;
}

#define REQUEST_IRQ(_irq, _name)					\
do {									\
	ret = request_threaded_irq(_irq, NULL, max8997_muic_irq,	\
				    0, _name, info);			\
	if (ret < 0)							\
		dev_err(info->dev, "Failed to request IRQ #%d: %d\n",	\
			_irq, ret);					\
} while (0)

static int max8997_muic_irq_init(struct max8997_muic_info *info)
{
	int ret;
#if 0
#if !defined(CONFIG_MACH_U1_REV00)
	dev_info(info->dev, "%s: system_rev=%d\n", __func__, system_rev);
#if !defined(CONFIG_MACH_P6_REV02) && !defined(CONFIG_MACH_U1_C210) \
		&& !defined(CONFIG_MACH_U1HD_C210)
	if (system_rev < 0x3) {
		dev_info(info->dev,
			"Caution !!! This system_rev does not support ALL irq\n");
		return 0;
	}
#endif
#endif
#endif

	REQUEST_IRQ(info->irq_adc, "muic-adc");
	REQUEST_IRQ(info->irq_chgtype, "muic-chgtype");
	REQUEST_IRQ(info->irq_vbvolt, "muic-vbvolt");
	REQUEST_IRQ(info->irq_adcerr, "muic-adcerr");
#if defined(CONFIG_MUIC_MAX8997_OVPUI)
	REQUEST_IRQ(info->irq_chgins, "chg-insert");
	REQUEST_IRQ(info->irq_chgrm, "chg-remove");
#endif
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

static void max8997_muic_init_detect(struct work_struct *work)
{
	struct max8997_muic_info *info = container_of(work,
			struct max8997_muic_info, init_work.work);

	dev_info(info->dev, "%s\n", __func__);
	if (!info->muic_data)
		return;

	mutex_lock(&info->mutex);
	max8997_muic_detect_dev(info, -1);
	mutex_unlock(&info->mutex);
}

static void max8997_muic_usb_detect(struct work_struct *work)
{
	struct max8997_muic_info *info = container_of(work,
			struct max8997_muic_info, usb_work.work);
	struct max8997_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "%s\n", __func__);
	if (!mdata)
		return;

	mutex_lock(&info->mutex);
	info->is_usb_ready = true;

	if (info->muic_data->sw_path != CP_USB_MODE) {
		if (mdata->usb_cb) {
			switch (info->cable_type) {
			case CABLE_TYPE_USB:
			case CABLE_TYPE_JIG_USB_OFF:
			case CABLE_TYPE_JIG_USB_ON:
				mdata->usb_cb(USB_CABLE_ATTACHED);
				break;
			case CABLE_TYPE_OTG:
				mdata->usb_cb(USB_OTGHOST_ATTACHED);
				break;
			default:
				break;
			}
		}
	}
	mutex_unlock(&info->mutex);
}

static void max8997_muic_mhl_detect(struct work_struct *work)
{
	struct max8997_muic_info *info = container_of(work,
			struct max8997_muic_info, mhl_work.work);
	struct max8997_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "%s\n", __func__);
	if (!mdata)
		return;

	mutex_lock(&info->mutex);
	info->is_mhl_ready = true;
#ifndef CONFIG_MACH_U1
	if (mdata->is_mhl_attached) {
		if (!mdata->is_mhl_attached())
			goto out;
	}
#endif
	if (info->cable_type == CABLE_TYPE_MHL || \
		info->cable_type == CABLE_TYPE_MHL_VB) {
		if (mdata->mhl_cb)
			mdata->mhl_cb(MAX8997_MUIC_ATTACHED);
	}
out:
	mutex_unlock(&info->mutex);
}
extern struct device *switch_dev;

static int __devinit max8997_muic_probe(struct platform_device *pdev)
{
	struct max8997_dev *max8997 = dev_get_drvdata(pdev->dev.parent);
	struct max8997_platform_data *pdata = dev_get_platdata(max8997->dev);
	struct max8997_muic_info *info;
	struct input_dev *input;
	int ret;

	info = kzalloc(sizeof(struct max8997_muic_info), GFP_KERNEL);
	input = input_allocate_device();
	if (!info || !input) {
		dev_err(&pdev->dev, "%s: failed to allocate state\n", __func__);
		ret = -ENOMEM;
		goto err_kfree;
	}

	info->dev = &pdev->dev;
	info->max8997 = max8997;
	info->muic = max8997->muic;
	info->input = input;
	info->irq_adc = max8997->irq_base + MAX8997_IRQ_ADC;
	info->irq_chgtype = max8997->irq_base + MAX8997_IRQ_CHGTYP;
	info->irq_vbvolt = max8997->irq_base + MAX8997_IRQ_VBVOLT;
	info->irq_adcerr = max8997->irq_base + MAX8997_IRQ_ADCERR;
#if defined(CONFIG_MUIC_MAX8997_OVPUI)
	info->irq_chgins = max8997->irq_base + MAX8997_IRQ_CHGINS;
	info->irq_chgrm = max8997->irq_base + MAX8997_IRQ_CHGRM;
#endif
	if (pdata->muic) {
		info->muic_data = pdata->muic;
		info->muic_data->sw_path = AP_USB_MODE;
	}
	info->cable_type = CABLE_TYPE_UNKNOWN;


	platform_set_drvdata(pdev, info);

	input->name = pdev->name;
	input->phys = "deskdock-key/input0";
	input->dev.parent = &pdev->dev;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0001;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	input_set_capability(input, EV_KEY, KEY_VOLUMEUP);
	input_set_capability(input, EV_KEY, KEY_VOLUMEDOWN);
	input_set_capability(input, EV_KEY, KEY_PLAYPAUSE);
	input_set_capability(input, EV_KEY, KEY_PREVIOUSSONG);
	input_set_capability(input, EV_KEY, KEY_NEXTSONG);

	ret = input_register_device(input);
	if (ret) {
		dev_err(info->dev, "%s: Unable to register input device, "
				"error: %d\n", __func__, ret);
		goto err_input;
	}

#if !defined(CONFIG_MACH_U1CAMERA_BD)
	if (info->muic_data && gpio_is_valid(info->muic_data->gpio_usb_sel)) {
		CHECK_GPIO(info->muic_data->gpio_usb_sel, "USB_SEL");

		if (info->muic_data->cfg_uart_gpio)
			info->muic_data->uart_path =
				info->muic_data->cfg_uart_gpio();

#ifndef CONFIG_TARGET_LOCALE_NA
		ret = gpio_request(info->muic_data->gpio_usb_sel, "USB_SEL");
		if (ret) {
			dev_info(info->dev, "%s: fail to request gpio(%d)\n",
					__func__, ret);
			goto err_kfree;
		}
		if (gpio_get_value(info->muic_data->gpio_usb_sel)) {
			dev_info(info->dev, "%s: CP USB\n", __func__);
			info->muic_data->sw_path = CP_USB_MODE;
		}
#endif /* CONFIG_TARGET_LOCALE_NA */
	}
#endif /* CONFIG_MACH_U1CAMERA_BD */

	/* create sysfs group*/
	ret = sysfs_create_group(&switch_dev->kobj, &max8997_muic_group);
	dev_set_drvdata(switch_dev, info);

	if (ret) {
		dev_err(&pdev->dev,
			"failed to create max8997 muic attribute group\n");
		goto fail;
	}

	if (info->muic_data && info->muic_data->init_cb)
		info->muic_data->init_cb();

	mutex_init(&info->mutex);

	/* Set ADC debounce time: 25ms */
	max8997_muic_set_adcdbset(info, 2);

	ret = max8997_muic_irq_init(info);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to initialize MUIC irq:%d\n", ret);
		goto fail;
	}

	/* initial cable detection */
	INIT_DELAYED_WORK(&info->init_work, max8997_muic_init_detect);
	schedule_delayed_work(&info->init_work, msecs_to_jiffies(3000));

	INIT_DELAYED_WORK(&info->usb_work, max8997_muic_usb_detect);
	schedule_delayed_work(&info->usb_work, msecs_to_jiffies(17000));

	INIT_DELAYED_WORK(&info->mhl_work, max8997_muic_mhl_detect);
	schedule_delayed_work(&info->mhl_work, msecs_to_jiffies(25000));

	return 0;

fail:
	if (info->irq_adc)
		free_irq(info->irq_adc, NULL);
	if (info->irq_chgtype)
		free_irq(info->irq_chgtype, NULL);
	if (info->irq_vbvolt)
		free_irq(info->irq_vbvolt, NULL);
	if (info->irq_adcerr)
		free_irq(info->irq_adcerr, NULL);
	mutex_destroy(&info->mutex);
err_input:
	platform_set_drvdata(pdev, NULL);
err_kfree:
	input_free_device(input);
	kfree(info);
	return ret;
}

static int __devexit max8997_muic_remove(struct platform_device *pdev)
{
	struct max8997_muic_info *info = platform_get_drvdata(pdev);

	sysfs_remove_group(&switch_dev->kobj, &max8997_muic_group);

	if (info) {
		input_unregister_device(info->input);
		cancel_delayed_work(&info->init_work);
		cancel_delayed_work(&info->usb_work);
		cancel_delayed_work(&info->mhl_work);
		free_irq(info->irq_adc, info);
		free_irq(info->irq_chgtype, info);
		free_irq(info->irq_vbvolt, info);
		free_irq(info->irq_adcerr, info);
#if !defined(CONFIG_TARGET_LOCALE_NA) && !defined(CONFIG_MACH_U1CAMERA_BD)
		gpio_free(info->muic_data->gpio_usb_sel);
#endif /* CONFIG_TARGET_LOCALE_NA && CONFIG_MACH_U1CAMERA_BD */
		mutex_destroy(&info->mutex);
		kfree(info);
	}

	return 0;
}

static u8 max8997_dumpaddr_muic[] = {
	MAX8997_MUIC_REG_INTMASK1,
	MAX8997_MUIC_REG_CDETCTRL,
	MAX8997_MUIC_REG_CTRL2,
};

#ifdef CONFIG_PM
static int max8997_muic_freeze(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct max8997_muic_info *info;
	int i;
	info = platform_get_drvdata(pdev);

	for (i = 0; i < ARRAY_SIZE(max8997_dumpaddr_muic); i++)
		max8997_read_reg(info->max8997->muic, max8997_dumpaddr_muic[i],
			&info->max8997->reg_dump[i + MAX8997_REG_PMIC_END]);

	cancel_delayed_work(&info->init_work);
	cancel_delayed_work(&info->usb_work);
	cancel_delayed_work(&info->mhl_work);

	/* hibernation state, disconnect usb state*/
	dev_info(info->dev, "%s: DETACHED\n", __func__);
	mutex_lock(&info->mutex);
	max8997_muic_handle_detach(info);
	mutex_unlock(&info->mutex);

	return 0;
}

static int max8997_muic_restore(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct max8997_muic_info *info;
	int i;
	info = platform_get_drvdata(pdev);

	for (i = 0; i < ARRAY_SIZE(max8997_dumpaddr_muic); i++)
		max8997_write_reg(info->max8997->muic, max8997_dumpaddr_muic[i],
			info->max8997->reg_dump[i + MAX8997_REG_PMIC_END]);

	mutex_lock(&info->mutex);
	max8997_muic_detect_dev(info, -1);
	mutex_unlock(&info->mutex);

	return 0;
}

static const struct dev_pm_ops max8997_dev_pm_ops = {
	.freeze		= max8997_muic_freeze,
	.restore	= max8997_muic_restore,
};

#define MAX8997_DEV_PM_OPS	(&max8997_dev_pm_ops)
#else
#define MAX8997_DEV_PM_OPS	NULL
#endif

void max8997_muic_shutdown(struct device *dev)
{
	struct max8997_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	if (!info->muic) {
		dev_err(info->dev, "%s: no muic i2c client\n", __func__);
		return;
	}

	dev_info(info->dev, "%s: JIGSet: auto detection\n", __func__);
	val = (0 << CTRL3_JIGSET_SHIFT) | (0 << CTRL3_BOOTSET_SHIFT);

	ret = max8997_update_reg(info->muic, MAX8997_MUIC_REG_CTRL3, val,
			CTRL3_JIGSET_MASK | CTRL3_BOOTSET_MASK);
	if (ret < 0) {
		dev_err(info->dev, "%s: fail to update reg\n", __func__);
		return;
	}
}

static struct platform_driver max8997_muic_driver = {
	.driver		= {
		.name	= "max8997-muic",
		.owner	= THIS_MODULE,
		.pm	= MAX8997_DEV_PM_OPS,
		.shutdown = max8997_muic_shutdown,
	},
	.probe		= max8997_muic_probe,
	.remove		= __devexit_p(max8997_muic_remove),
};

static int __init max8997_muic_init(void)
{
	return platform_driver_register(&max8997_muic_driver);
}
module_init(max8997_muic_init);

static void __exit max8997_muic_exit(void)
{
	platform_driver_unregister(&max8997_muic_driver);
}
module_exit(max8997_muic_exit);


MODULE_DESCRIPTION("Maxim MAX8997 MUIC driver");
MODULE_AUTHOR("<ms925.kim@samsung.com>");
MODULE_LICENSE("GPL");
