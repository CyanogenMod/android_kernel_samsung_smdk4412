/*
 * max14577-muic.c - MUIC driver for the Maxim 14577
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

#include <linux/mfd/max14577.h>
#include <linux/mfd/max14577-private.h>

/* MUIC header file */
#include <linux/muic/max14577-muic.h>

/* usb header file */
#include <linux/host_notify.h>
#include <plat/udc-hs.h>

extern struct sec_switch_data sec_switch_data;

/* don't access this variable directly!! except get_switch_sel_value function.
 * you must get switch_sel value by using get_switch_sel function. */
static int switch_sel;

/* func : set_switch_sel
 * switch_sel value get from bootloader comand line
 * switch_sel data consist 4 bits
 */
static int set_switch_sel(char *str)
{
	get_option(&str, &switch_sel);
	switch_sel = switch_sel & 0x0f;
	pr_info("%s:%s switch_sel: 0x%x\n", MUIC_DEV_NAME, __func__,
			switch_sel);

	return switch_sel;
}
__setup("pmic_info=", set_switch_sel);

static int get_switch_sel(void)
{
	return switch_sel;
}

static const struct max14577_muic_vps_data muic_vps_table[] = {
#if defined(CONFIG_MUIC_SUPPORT_OTG_DONGLE)
	{
		.adcerr		= 0x00,
		.adclow		= (0x0 << STATUS1_ADCLOW_SHIFT),
		.adc		= ADC_GND,
		.vbvolt		= 0x00,
		.chgdetrun	= 0x00,
		.chgtyp		= CHGTYP_NOTHING_ATTACH,
		.control1	= CTRL1_USB,
		.vps_name	= "OTG",
		.attached_dev	= ATTACHED_DEV_OTG_MUIC,
	},
#endif /* CONFIG_MUIC_SUPPORT_OTG_DONGLE */

//#if defined(CONFIG_MUIC_SUPPORT_FACTORY_BUTTON)
	{
		.adcerr		= 0x00,
		.adclow		= (0x1 << STATUS1_ADCLOW_SHIFT),
		.adc		= ADC_REMOTE_S11,
		.vbvolt		= 0x00,
		.chgdetrun	= 0x00,
		.chgtyp		= CHGTYP_NOTHING_ATTACH,
		.control1	= CTRL1_OPEN,
		.vps_name	= "Factory Button",
		.attached_dev	= ATTACHED_DEV_FACTORY_BUTTON,
	},
//#endif /* CONFIG_MUIC_SUPPORT_FACTORY_BUTTON */

//#if defined(CONFIG_MUIC_SUPPORT_POGO_TA)
	{
		.adcerr		= 0x00,
		.adclow		= (0x1 << STATUS1_ADCLOW_SHIFT),
		.adc		= ADC_SMARTDOCK,
		.vbvolt		= (0x1 << STATUS2_VBVOLT_SHIFT),
		.chgdetrun	= 0x00,
		.chgtyp		= CHGTYP_DEDICATED_CHARGER,
		.control1	= CTRL1_OPEN,
		.vps_name	= "POGO + TA",
		.attached_dev	= ATTACHED_DEV_POGO_TA_MUIC,
	},
//#endif /* CONFIG_MUIC_SUPPORT_POGO_TA */

//#if defined(CONFIG_MUIC_SUPPORT_POGO_USB)
	{
		.adcerr		= 0x00,
		.adclow		= (0x1 << STATUS1_ADCLOW_SHIFT),
		.adc		= ADC_SMARTDOCK,
		.vbvolt		= (0x1 << STATUS2_VBVOLT_SHIFT),
		.chgdetrun	= 0x00,
		.chgtyp		= CHGTYP_USB,
		.control1	= CTRL1_USB,
		.vps_name	= "POGO + USB",
		.attached_dev	= ATTACHED_DEV_POGO_USB_MUIC,
	},
//#endif /* CONFIG_MUIC_SUPPORT_POGO_USB */

	{
		.adcerr		= 0x00,
		.adclow		= (0x1 << STATUS1_ADCLOW_SHIFT),
		.adc		= ADC_JIG_USB_ON,
		.vbvolt		= (0x1 << STATUS2_VBVOLT_SHIFT),
		.chgdetrun	= 0x00,
		.chgtyp		= CHGTYP_NOTHING_ATTACH,
		.control1	= CTRL1_USB,
		.vps_name	= "Jig USB On",
		.attached_dev	= ATTACHED_DEV_JIG_USB_ON_MUIC,
	},
	{
		.adcerr		= 0x00,
		.adclow		= (0x1 << STATUS1_ADCLOW_SHIFT),
		.adc		= ADC_JIG_UART_OFF,
		.vbvolt		= 0x00,
		.chgdetrun	= 0x00,
		.chgtyp		= CHGTYP_NOTHING_ATTACH,
		.control1	= CTRL1_UART,
		.vps_name	= "Jig UART Off",
		.attached_dev	= ATTACHED_DEV_JIG_UART_OFF_MUIC,
	},
	{
		.adcerr		= 0x00,
		.adclow		= (0x1 << STATUS1_ADCLOW_SHIFT),
		.adc		= ADC_JIG_UART_OFF,
		.vbvolt		= (0x1 << STATUS2_VBVOLT_SHIFT),
		.chgdetrun	= 0x00,
		.chgtyp		= CHGTYP_NOTHING_ATTACH,
		.control1	= CTRL1_UART,
		.vps_name	= "Jig UART Off + VB",
		.attached_dev	= ATTACHED_DEV_JIG_UART_OFF_VB_MUIC,
	},
	{
		.adcerr		= 0x00,
		.adclow		= (0x1 << STATUS1_ADCLOW_SHIFT),
		.adc		= ADC_OPEN,
		.vbvolt		= (0x1 << STATUS2_VBVOLT_SHIFT),
		.chgdetrun	= 0x00,
		.chgtyp		= CHGTYP_DEDICATED_CHARGER,
		.control1	= CTRL1_OPEN,
		.vps_name	= "TA",
		.attached_dev	= ATTACHED_DEV_TA_MUIC,
	},
	{
		.adcerr		= 0x00,
		.adclow		= (0x1 << STATUS1_ADCLOW_SHIFT),
		.adc		= ADC_OPEN,
		.vbvolt		= (0x1 << STATUS2_VBVOLT_SHIFT),
		.chgdetrun	= 0x00,
		.chgtyp		= CHGTYP_USB,
		.control1	= CTRL1_USB,
		.vps_name	= "USB",
		.attached_dev	= ATTACHED_DEV_USB_MUIC,
	},
};

static int muic_lookup_vps_table(muic_attached_dev_t new_dev)
{
	const struct max14577_muic_vps_data *tmp_vps;
	int i;

	for (i = 0; i < ARRAY_SIZE(muic_vps_table); i++) {
		tmp_vps = &(muic_vps_table[i]);

		if (tmp_vps->attached_dev != new_dev)
			continue;

		pr_info("%s:%s (%d) vps table match found at i(%d), %s\n",
				MUIC_DEV_NAME, __func__, new_dev, i,
				tmp_vps->vps_name);

		return i;
	}

	pr_info("%s:%s can't find (%d) on vps table\n", MUIC_DEV_NAME,
			__func__, new_dev);

	return -1;
}

static int write_muic_ctrl_reg(struct max14577_muic_data *muic_data,
				const u8 reg, const u8 val)
{
	struct i2c_client *i2c = muic_data->i2c;
	int ret = 0;
	u8 reg_val = 0;

	ret = max14577_read_reg(i2c, reg, &reg_val);
	if (ret < 0)
		pr_err("%s:%s err read REG(0x%02x) [%d]\n", MUIC_DEV_NAME,
				__func__, reg, ret);

	if (reg_val ^ val) {
		pr_info("%s:%s overwrite REG(0x%02x) from [0x%x] to [0x%x]\n",
				MUIC_DEV_NAME, __func__, reg, reg_val, val);

		ret = max14577_write_reg(i2c, reg, val);
		if (ret < 0)
			pr_err("%s:%s err write REG(0x%02x)\n", MUIC_DEV_NAME,
					__func__, reg);
	} else {
		pr_info("%s:%s REG(0x%02x) already [0x%x], just return\n",
				MUIC_DEV_NAME, __func__, reg, reg_val);
	}

	ret = max14577_read_reg(i2c, reg, &reg_val);
	if (ret < 0)
		pr_err("%s:%s err read REG(0x%02x) [%d]\n", MUIC_DEV_NAME,
				__func__, reg, ret);
	else
		pr_info("%s:%s REG(0x%02x) after change [0x%x]\n", MUIC_DEV_NAME,
				__func__, reg, reg_val);

	return ret;
}

static int com_to_open(struct max14577_muic_data *muic_data)
{
	max14577_reg_ctrl1_t reg_val;
	int ret = 0;

	reg_val = CTRL1_OPEN;
	/* write control1 register */
	ret = write_muic_ctrl_reg(muic_data, MAX14577_MUIC_REG_CONTROL1,
			reg_val);
	if (ret)
		pr_err("%s:%s write_muic_ctrl_reg err\n", MUIC_DEV_NAME,
				__func__);

	return ret;
}

static int com_to_usb(struct max14577_muic_data *muic_data)
{

	max14577_reg_ctrl1_t reg_val;
	int ret = 0;

	reg_val = CTRL1_USB;
	/* write control1 register */
	ret = write_muic_ctrl_reg(muic_data, MAX14577_MUIC_REG_CONTROL1,
			reg_val);
	if (ret)
		pr_err("%s:%s write_muic_ctrl_reg err\n", MUIC_DEV_NAME,
				__func__);

	return ret;
}

static int com_to_uart(struct max14577_muic_data *muic_data)
{

	max14577_reg_ctrl1_t reg_val;
	int ret = 0;

	reg_val = CTRL1_UART;
	/* write control1 register */
	ret = write_muic_ctrl_reg(muic_data, MAX14577_MUIC_REG_CONTROL1,
			reg_val);
	if (ret)
		pr_err("%s:%s write_muic_ctrl_reg err\n", MUIC_DEV_NAME,
				__func__);

	return ret;
}

static int com_to_audio(struct max14577_muic_data *muic_data)
{

	max14577_reg_ctrl1_t reg_val;
	int ret = 0;

	reg_val = CTRL1_AUDIO;
	/* write control1 register */
	ret = write_muic_ctrl_reg(muic_data, MAX14577_MUIC_REG_CONTROL1,
			reg_val);
	if (ret)
		pr_err("%s:%s write_muic_ctrl_reg err\n", MUIC_DEV_NAME,
				__func__);

	return ret;
}

static int write_vps_regs(struct max14577_muic_data *muic_data,
			muic_attached_dev_t new_dev)
{
	const struct max14577_muic_vps_data *tmp_vps;
	int vps_index;
	int ret = 0;

	vps_index = muic_lookup_vps_table(new_dev);
	if (vps_index == -1)
		return -ENODEV;

	tmp_vps = &(muic_vps_table[vps_index]);

	/* write control1 register */
	ret = write_muic_ctrl_reg(muic_data, MAX14577_MUIC_REG_CONTROL1,
			tmp_vps->control1);

	return ret;
}

static u8 max14577_muic_get_adc_value(struct max14577_muic_data *muic_data)
{
	u8 status;
	u8 adc = ADC_ERROR;
	int ret = 0;

	ret = max14577_read_reg(muic_data->i2c, MAX14577_MUIC_REG_STATUS1,
			&status);
	if (ret)
		pr_err("%s:%s fail to read muic reg(%d)\n", MUIC_DEV_NAME,
				__func__, ret);
	else
		adc = status & STATUS1_ADC_MASK;

	return adc;
}

static ssize_t max14577_muic_show_uart_sel(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max14577_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;
	const char *mode = "UNKNOWN\n";

	switch (pdata->uart_path) {
	case MUIC_PATH_UART_AP:
		mode = "AP\n";
		break;
	case MUIC_PATH_UART_CP:
		mode = "CP\n";
		break;
	default:
		break;
	}

	pr_info("%s:%s %s", MUIC_DEV_NAME, __func__, mode);
	return sprintf(buf, mode);
}

/* muic uart path control function */
static int switch_to_ap_uart(struct max14577_muic_data *muic_data)
{
	int ret = 0;

	if (muic_data->attached_dev != ATTACHED_DEV_JIG_UART_OFF_MUIC)
		pr_warn("%s:%s current attached is (%d) not Jig UART Off\n",
				MUIC_DEV_NAME, __func__,
				muic_data->attached_dev);
	else
		ret = com_to_uart(muic_data);

	return ret;
}

static int switch_to_cp_uart(struct max14577_muic_data *muic_data)
{
	int ret = 0;

	if (muic_data->attached_dev != ATTACHED_DEV_JIG_UART_OFF_MUIC)
		pr_warn("%s:%s current attached is (%d) not Jig UART Off\n",
				MUIC_DEV_NAME, __func__,
				muic_data->attached_dev);
	else
		ret = com_to_uart(muic_data);

	return ret;
}

static ssize_t max14577_muic_set_uart_sel(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max14577_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;

	if (!strncasecmp(buf, "AP", 2)) {
		pdata->uart_path = MUIC_PATH_UART_AP;
		switch_to_ap_uart(muic_data);
	} else if (!strncasecmp(buf, "CP", 2)) {
		pdata->uart_path = MUIC_PATH_UART_CP;
		switch_to_cp_uart(muic_data);
	} else {
		pr_warn("%s:%s invalid value\n", MUIC_DEV_NAME, __func__);
	}

	pr_info("%s:%s uart_path(%d)\n", MUIC_DEV_NAME, __func__,
			pdata->uart_path);

	return count;
}

static ssize_t max14577_muic_show_usb_sel(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max14577_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;
	const char *mode = "UNKNOWN\n";

	switch (pdata->usb_path) {
	case MUIC_PATH_USB_AP:
		mode = "PDA\n";
		break;
	case MUIC_PATH_USB_CP:
		mode = "MODEM\n";
		break;
	default:
		break;
	}

	pr_info("%s:%s %s", MUIC_DEV_NAME, __func__, mode);
	return sprintf(buf, mode);
}

static ssize_t max14577_muic_set_usb_sel(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max14577_muic_data *muic_data = dev_get_drvdata(dev);
	struct muic_platform_data *pdata = muic_data->pdata;

	if (!strncasecmp(buf, "PDA", 3)) {
		pdata->usb_path = MUIC_PATH_USB_AP;
	} else if (!strncasecmp(buf, "MODEM", 5)) {
		pdata->usb_path = MUIC_PATH_USB_CP;
	} else {
		pr_warn("%s:%s invalid value\n", MUIC_DEV_NAME, __func__);
	}

	pr_info("%s:%s usb_path(%d)\n", MUIC_DEV_NAME, __func__,
			pdata->usb_path);

	return count;
}

static ssize_t max14577_muic_show_adc(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct max14577_muic_data *muic_data = dev_get_drvdata(dev);
	u8 adc;

	adc = max14577_muic_get_adc_value(muic_data);
	pr_info("%s:%s adc(0x%02x)\n", MUIC_DEV_NAME, __func__, adc);

	if (adc == ADC_ERROR) {
		pr_err("%s:%s fail to read adc value\n", MUIC_DEV_NAME,
				__func__);
		return sprintf(buf, "UNKNOWN\n");
	}

	return sprintf(buf, "%x\n", adc);
}

static ssize_t max14577_muic_show_usb_state(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct max14577_muic_data *muic_data = dev_get_drvdata(dev);

	pr_info("%s:%s attached_dev(%d)\n", MUIC_DEV_NAME, __func__,
			muic_data->attached_dev);

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_POGO_USB_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		return sprintf(buf, "USB_STATE_CONFIGURED\n");
	default:
		break;
	}

	return sprintf(buf, "USB_STATE_NOTCONFIGURED\n");
}

static ssize_t max14577_muic_show_attached_dev(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct max14577_muic_data *muic_data = dev_get_drvdata(dev);
	const struct max14577_muic_vps_data *tmp_vps;
	int vps_index;

	vps_index = muic_lookup_vps_table(muic_data->attached_dev);
	if (vps_index == -1)
		return sprintf(buf, "Error No Device\n");

	tmp_vps = &(muic_vps_table[vps_index]);

	return sprintf(buf, "%s\n", tmp_vps->vps_name);
}
#if 0
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
#endif
static void max14577_muic_set_adcdbset(struct max14577_muic_data *muic_data,
					int value)
{
	int ret;
	u8 val;
	u8 cntl3_before, cntl3_after;

	if (value > 3) {
		pr_err("%s:%s invalid value(%d)\n", MUIC_DEV_NAME, __func__,
				value);
		return;
	}

	if (!muic_data->i2c) {
		pr_err("%s:%s no muic i2c client\n", MUIC_DEV_NAME, __func__);
		return;
	}

	val = (value << CTRL3_ADCDBSET_SHIFT);
	max14577_read_reg(muic_data->i2c, MAX14577_MUIC_REG_CONTROL3,
			&cntl3_before);

	ret = max14577_write_reg(muic_data->i2c, MAX14577_MUIC_REG_CONTROL3,
			val);
	if (ret < 0)
		pr_err("%s: fail to write reg\n", __func__);

	max14577_read_reg(muic_data->i2c, MAX14577_MUIC_REG_CONTROL3,
			&cntl3_after);

	pr_info("%s:%s CNTL3: before(0x%02x), value(0x%02x), after(0x%02x)\n",
			MUIC_DEV_NAME, __func__, cntl3_before, val, cntl3_after);
}

static void max14577_muic_set_dchktm(struct max14577_muic_data *muic_data,
				int value)
{
	struct max14577_platform_data *mfd_pdata = muic_data->mfd_pdata;
	int ret;
	u8 old_val, val, new_val;
	const u8 mask = DCHKTM_MASK;

	if (value > 1) {
		pr_err("%s:%s invalid value(%d)\n", MUIC_DEV_NAME, __func__,
				value);
		return;
	}

	if (!muic_data->i2c) {
		pr_err("%s:%s no muic i2c client\n", MUIC_DEV_NAME, __func__);
		return;
	}

	ret = mfd_pdata->get_cdetctrl1_reg(muic_data->i2c, &old_val);
	if (ret) {
		pr_err("%s:%s fail to read reg\n", MUIC_DEV_NAME, __func__);
		return;
	}

	old_val = old_val & 0xff;

	val = (value << DCHKTM_SHIFT);
	pr_info("%s:%s DCHKTM(0x%02x)\n", MUIC_DEV_NAME, __func__, val);

	new_val = (val & mask) | (old_val & (~mask));
	ret = mfd_pdata->set_cdetctrl1_reg(muic_data->i2c, new_val);
	if (ret < 0)
		pr_err("%s: fail to update reg\n", __func__);
}

static DEVICE_ATTR(uart_sel, 0664, max14577_muic_show_uart_sel,
		max14577_muic_set_uart_sel);
static DEVICE_ATTR(usb_sel, 0664, max14577_muic_show_usb_sel,
		max14577_muic_set_usb_sel);
static DEVICE_ATTR(adc, S_IRUGO, max14577_muic_show_adc, NULL);
static DEVICE_ATTR(usb_state, S_IRUGO, max14577_muic_show_usb_state, NULL);
static DEVICE_ATTR(attached_dev, S_IRUGO, max14577_muic_show_attached_dev, NULL);
#if 0
static DEVICE_ATTR(audio_path, 0664,
		max8997_muic_show_audio_path, max8997_muic_set_audio_path);
static DEVICE_ATTR(otg_test, 0664,
		max8997_muic_show_otg_test, max8997_muic_set_otg_test);
#endif

static struct attribute *max14577_muic_attributes[] = {
	&dev_attr_uart_sel.attr,
	&dev_attr_usb_sel.attr,
	&dev_attr_adc.attr,
	&dev_attr_usb_state.attr,
	&dev_attr_attached_dev.attr,
#if 0
	&dev_attr_audio_path.attr,
	&dev_attr_otg_test.attr,
#endif
	NULL
};

static const struct attribute_group max14577_muic_group = {
	.attrs = max14577_muic_attributes,
};

static void max14577_muic_attach_callback_dock(struct max14577_muic_data *muic_data,
						int dock_type)
{
	struct sec_switch_data *switch_data = muic_data->switch_data;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	if (switch_data->dock_cb)
		switch_data->dock_cb(dock_type);
	else
		pr_info("%s:%s dock_cb pointer is NULL, just return\n",
				MUIC_DEV_NAME, __func__);
}

static void max14577_muic_detach_callback_dock(struct max14577_muic_data *muic_data)
{
	struct sec_switch_data *switch_data = muic_data->switch_data;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	if (switch_data->dock_cb)
		switch_data->dock_cb(MUIC_DOCK_DETACHED);
	else
		pr_info("%s:%s dock_cb pointer is NULL, just return\n",
				MUIC_DEV_NAME, __func__);
}

static void max14577_muic_attach_callback_usb(struct max14577_muic_data *muic_data)
{
	struct muic_platform_data *pdata = muic_data->pdata;
	struct sec_switch_data *switch_data = muic_data->switch_data;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	if (muic_data->is_usb_ready == false) {
		pr_info("%s:%s USB is not ready, just return\n", MUIC_DEV_NAME,
				__func__);
		return;
	}

	if (pdata->usb_path != MUIC_PATH_USB_AP) {
		pr_info("%s:%s USB path is not AP, just return\n",
				MUIC_DEV_NAME, __func__);
		return;
	}

	if (switch_data->usb_cb)
		switch_data->usb_cb(USB_CABLE_ATTACHED);
	else
		pr_info("%s:%s usb_cb pointer is NULL, just return\n",
				MUIC_DEV_NAME, __func__);
}

static void max14577_muic_detach_callback_usb(struct max14577_muic_data *muic_data)
{
	struct muic_platform_data *pdata = muic_data->pdata;
	struct sec_switch_data *switch_data = muic_data->switch_data;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	if (muic_data->is_usb_ready == false) {
		pr_info("%s:%s USB is not ready, just return\n", MUIC_DEV_NAME,
				__func__);
		return;
	}

	if (pdata->usb_path != MUIC_PATH_USB_AP) {
		pr_info("%s:%s USB path is not AP, just return\n",
				MUIC_DEV_NAME, __func__);
		return;
	}

	if (switch_data->usb_cb)
		switch_data->usb_cb(USB_CABLE_DETACHED);
	else
		pr_info("%s:%s usb_cb pointer is NULL, just return\n",
				MUIC_DEV_NAME, __func__);
}

static void max14577_muic_attach_callback_chg(struct max14577_muic_data *muic_data)
{
	struct sec_switch_data *switch_data = muic_data->switch_data;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	if (muic_data->is_muic_ready == false) {
		pr_info("%s:%s MUIC is not ready, just return\n", MUIC_DEV_NAME,
				__func__);
		return;
	}

	if (switch_data->charger_cb)
		switch_data->charger_cb(muic_data->attached_dev);
	else
		pr_info("%s:%s charger_cb pointer is NULL, just return\n",
				MUIC_DEV_NAME, __func__);
}

static void max14577_muic_detach_callback_chg(struct max14577_muic_data *muic_data)
{
	struct sec_switch_data *switch_data = muic_data->switch_data;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	if (muic_data->is_muic_ready == false) {
		pr_info("%s:%s MUIC is not ready, just return\n", MUIC_DEV_NAME,
				__func__);
		return;
	}

	if (switch_data->charger_cb)
		switch_data->charger_cb(muic_data->attached_dev);
	else
		pr_info("%s:%s charger_cb pointer is NULL, just return\n",
				MUIC_DEV_NAME, __func__);
}

static void max14577_muic_force_detach(struct max14577_muic_data *muic_data)
{
	muic_lookup_vps_table(muic_data->attached_dev);

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_POGO_TA_MUIC:
		muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
		max14577_muic_detach_callback_chg(muic_data);
		break;
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_POGO_USB_MUIC:
		muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
		max14577_muic_detach_callback_chg(muic_data);
		max14577_muic_detach_callback_usb(muic_data);
		break;
	case ATTACHED_DEV_FACTORY_BUTTON:
		muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
		max14577_muic_detach_callback_dock(muic_data);
		break;
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_UNKNOWN_MUIC:
		muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
	case ATTACHED_DEV_NONE_MUIC:
		break;
	default:
		pr_info("%s:%s invalid device type(%d)\n", MUIC_DEV_NAME,
				__func__, muic_data->attached_dev);
		muic_data->attached_dev = ATTACHED_DEV_NONE_MUIC;
		break;
	}

	return;
}

static int max14577_muic_logically_detach(struct max14577_muic_data *muic_data,
						muic_attached_dev_t new_dev)
{
	bool force_path_open = true;
	int ret = 0;

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_FACTORY_BUTTON:
	case ATTACHED_DEV_POGO_TA_MUIC:
	case ATTACHED_DEV_POGO_USB_MUIC:
		break;
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		if (new_dev == ATTACHED_DEV_JIG_UART_OFF_VB_MUIC)
			force_path_open = false;
		break;
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
		if (new_dev == ATTACHED_DEV_JIG_UART_OFF_MUIC)
			force_path_open = false;
		break;
	case ATTACHED_DEV_UNKNOWN_MUIC:
		if (new_dev == ATTACHED_DEV_JIG_UART_OFF_MUIC)
			force_path_open = false;
		break;
	case ATTACHED_DEV_NONE_MUIC:
		goto out;
	default:
		pr_warn("%s:%s try to attach without logically detach\n",
				MUIC_DEV_NAME, __func__);
		goto out;
	}

	pr_info("%s:%s attached(%d)!=new(%d), assume detach\n", MUIC_DEV_NAME,
			__func__, muic_data->attached_dev, new_dev);

	max14577_muic_force_detach(muic_data);

	if (force_path_open)
		ret = com_to_open(muic_data);

out:
	return ret;
}

static int max14577_muic_enable_accdet(struct max14577_muic_data *muic_data)
{
	struct max14577_platform_data *mfd_pdata = muic_data->mfd_pdata;
	struct i2c_client *i2c = muic_data->i2c;
	const u8 mask = CTRL2_ACCDET_MASK;
	u8 old_val, val, new_val;
	int ret = 0;

	if (!muic_data->i2c) {
		pr_err("%s:%s no muic i2c client\n", MUIC_DEV_NAME, __func__);
		return -ENODEV;
	}

	ret = mfd_pdata->get_control2_reg(i2c, &old_val);
	if (ret) {
		pr_err("%s:%s fail to read reg(%d)\n", MUIC_DEV_NAME, __func__,
				ret);
		return ret;
	}

	old_val = old_val & 0xff;
	val = (1 << CTRL2_ACCDET_SHIFT);
	pr_info("%s:%s ACCDET(0x%02x)\n", MUIC_DEV_NAME, __func__, val);

	new_val = (val & mask) | (old_val & (~mask));
	ret = mfd_pdata->set_control2_reg(muic_data->i2c, new_val);
	if (ret)
		pr_err("%s: fail to update reg(%d)\n", __func__, ret);

	return ret;
}

static int max14577_muic_handle_detach(struct max14577_muic_data *muic_data)
{
	int ret = 0;

	/* Enable Factory Accessory Detection State Machine */
	max14577_muic_enable_accdet(muic_data);

	max14577_muic_force_detach(muic_data);

	ret = com_to_open(muic_data);

	return ret;
}

static int max14577_muic_handle_attach(struct max14577_muic_data *muic_data,
					muic_attached_dev_t new_dev)
{
	int ret = 0;

	if (new_dev == muic_data->attached_dev) {
		pr_info("%s:%s Duplicated(%d), just ignore\n", MUIC_DEV_NAME,
				__func__, muic_data->attached_dev);
		return ret;
	}

	ret = max14577_muic_logically_detach(muic_data, new_dev);
	if (ret)
		pr_warn("%s:%s fail to logically detach(%d)\n", MUIC_DEV_NAME,
				__func__, ret);

	if (muic_data->mfd_pdata->set_gpio_pogo_cb) {
		/* VBATT <-> VBUS voltage switching, along to new_dev. */
		ret = muic_data->mfd_pdata->set_gpio_pogo_cb(new_dev);
		if (ret)
			pr_warn("%s:%s fail to VBATT <-> VBUS voltage switching(%d)\n",
					MUIC_DEV_NAME, __func__, ret);
	}

	switch (new_dev) {
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		ret = write_vps_regs(muic_data, new_dev);
		muic_data->attached_dev = new_dev;
		break;
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_POGO_TA_MUIC:
		ret = write_vps_regs(muic_data, new_dev);
		muic_data->attached_dev = new_dev;
		max14577_muic_attach_callback_chg(muic_data);
		break;
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_POGO_USB_MUIC:
		ret = write_vps_regs(muic_data, new_dev);
		muic_data->attached_dev = new_dev;
		max14577_muic_attach_callback_usb(muic_data);
		max14577_muic_attach_callback_chg(muic_data);
		break;
	case ATTACHED_DEV_FACTORY_BUTTON:
		ret = write_vps_regs(muic_data, new_dev);
		muic_data->attached_dev = new_dev;
		max14577_muic_attach_callback_dock(muic_data, MUIC_DOCK_FACTORY);
		break;
	default:
		pr_warn("%s:%s unsupported dev(%d)\n", MUIC_DEV_NAME, __func__,
				new_dev);
		break;
	}

	return ret;
}

static void max14577_muic_detect_dev(struct max14577_muic_data *muic_data, int irq)
{
	struct i2c_client *i2c = muic_data->i2c;
	const struct max14577_muic_vps_data *tmp_vps;
	muic_attached_dev_t new_dev = ATTACHED_DEV_UNKNOWN_MUIC;
	int intr = MUIC_INTR_DETACH;
	u8 status[2];
	u8 adcerr, adclow, adc, vbvolt, chgdetrun, chgtyp;
	int ret;
	int i;

	ret = max14577_bulk_read(i2c, MAX14577_MUIC_REG_STATUS1, 2, status);
	if (ret) {
		pr_err("%s:%s fail to read muic reg(%d)\n", MUIC_DEV_NAME,
				__func__, ret);
		return;
	}

	pr_info("%s:%s STATUS1:0x%02x, 2:0x%02x\n", MUIC_DEV_NAME, __func__,
			status[0], status[1]);

	adcerr = status[0] & STATUS1_ADCERR_MASK;
	adclow = status[0] & STATUS1_ADCLOW_MASK;
	adc = status[0] & STATUS1_ADC_MASK;
	vbvolt = status[1] & STATUS2_VBVOLT_MASK;
	chgdetrun = status[1] & STATUS2_CHGDETRUN_MASK;
	chgtyp = status[1] & STATUS2_CHGTYP_MASK;

	pr_info("%s:%s adcerr:0x%x adclow:0x%x adc:0x%x vb:0x%x chgdetrun:0x%x"
			" chgtyp:0x%x\n", MUIC_DEV_NAME, __func__, adcerr,
			adclow, adc, vbvolt, chgdetrun, chgtyp);

	/* Workaround for Factory mode.
	 * Abandon adc interrupt of approximately +-100K range
	 * if previous cable status was JIG UART BOOT OFF.
	 */
	if (muic_data->attached_dev == ATTACHED_DEV_JIG_UART_OFF_MUIC) {
		if ((adc == (ADC_JIG_UART_OFF + 1)) ||
				(adc == (ADC_JIG_UART_OFF - 1))) {
			pr_warn("%s:%s abandon ADC\n", MUIC_DEV_NAME, __func__);
			return;
		}
	}

	for (i = 0; i < ARRAY_SIZE(muic_vps_table); i++) {
		tmp_vps = &(muic_vps_table[i]);

		if (tmp_vps->adcerr != adcerr)
			continue;

		if (tmp_vps->adclow != adclow)
			continue;

		if (tmp_vps->adc != adc)
			continue;

		if (tmp_vps->vbvolt != vbvolt)
			continue;

		if (tmp_vps->chgdetrun != chgdetrun)
			continue;

		if (tmp_vps->chgtyp != chgtyp)
			continue;

		pr_info("%s:%s vps table match found at i(%d), %s\n",
				MUIC_DEV_NAME, __func__, i, tmp_vps->vps_name);

		new_dev = tmp_vps->attached_dev;

		intr = MUIC_INTR_ATTACH;
		break;
	}

	if (intr == MUIC_INTR_ATTACH) {
		pr_info("%s:%s ATTACHED\n", MUIC_DEV_NAME, __func__);
		max14577_muic_handle_attach(muic_data, new_dev);
	} else {
		pr_info("%s:%s DETACHED\n", MUIC_DEV_NAME, __func__);
		max14577_muic_handle_detach(muic_data);
	}
	return;
}

static irqreturn_t max14577_muic_irq(int irq, void *data)
{
	struct max14577_muic_data *muic_data = data;
	pr_info("%s:%s irq:%d\n", MUIC_DEV_NAME, __func__, irq);

	mutex_lock(&muic_data->muic_mutex);
	if (muic_data->is_muic_ready == true)
		max14577_muic_detect_dev(muic_data, irq);
	else
		pr_info("%s:%s MUIC is not ready, just return\n", MUIC_DEV_NAME,
				__func__);
	mutex_unlock(&muic_data->muic_mutex);

	return IRQ_HANDLED;
}

#define REQUEST_IRQ(_irq, _name)					\
do {									\
	ret = request_threaded_irq(_irq, NULL, max14577_muic_irq,	\
				IRQF_NO_SUSPEND, _name, muic_data);	\
	if (ret < 0)							\
		pr_err("%s:%s Failed to request IRQ #%d: %d\n",		\
				MUIC_DEV_NAME, __func__, _irq, ret);	\
} while (0)

static int max14577_muic_irq_init(struct max14577_muic_data *muic_data)
{
	int ret;

	REQUEST_IRQ(muic_data->irq_adcerr, "muic-adcerr");
	REQUEST_IRQ(muic_data->irq_adc, "muic-adc");
	REQUEST_IRQ(muic_data->irq_chgtyp, "muic-chgtyp");
	REQUEST_IRQ(muic_data->irq_vbvolt, "muic-vbvolt");

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

static void max14577_muic_init_detect(struct work_struct *work)
{
	struct max14577_muic_data *muic_data = container_of(work,
			struct max14577_muic_data, init_work.work);

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	mutex_lock(&muic_data->muic_mutex);
	muic_data->is_muic_ready = true;

	max14577_muic_detect_dev(muic_data, -1);
	mutex_unlock(&muic_data->muic_mutex);
}

static void max14577_muic_usb_detect(struct work_struct *work)
{
	struct max14577_muic_data *muic_data = container_of(work,
			struct max14577_muic_data, usb_work.work);
	struct sec_switch_data *switch_data = muic_data->switch_data;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	mutex_lock(&muic_data->muic_mutex);
	muic_data->is_usb_ready = true;

	switch (muic_data->attached_dev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_POGO_USB_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		max14577_muic_attach_callback_usb(muic_data);
		break;
	default:
		break;
	}
	mutex_unlock(&muic_data->muic_mutex);
}

static int __devinit max14577_muic_probe(struct platform_device *pdev)
{
	struct max14577_dev *max14577 = dev_get_drvdata(pdev->dev.parent);
	struct max14577_platform_data *mfd_pdata = dev_get_platdata(max14577->dev);
	struct max14577_muic_data *muic_data;
	int ret = 0;

	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);

	muic_data = kzalloc(sizeof(struct max14577_muic_data), GFP_KERNEL);
	if (!muic_data) {
		pr_err("%s: failed to allocate driver data\n", __func__);
		ret = -ENOMEM;
		goto err_return;
	}

	if (!mfd_pdata) {
		pr_err("%s: failed to get max14577 mfd platform data\n", __func__);
		ret = -ENOMEM;
		goto err_kfree;
	}

	muic_data->dev = &pdev->dev;
	mutex_init(&muic_data->muic_mutex);
	muic_data->i2c = max14577->i2c;
	muic_data->mfd_pdata = mfd_pdata;
	muic_data->irq_adcerr = mfd_pdata->irq_base + MAX14577_IRQ_INT1_ADCERR;
	muic_data->irq_adc = mfd_pdata->irq_base + MAX14577_IRQ_INT1_ADC;
	muic_data->irq_chgtyp = mfd_pdata->irq_base + MAX14577_IRQ_INT2_CHGTYP;
	muic_data->irq_vbvolt = mfd_pdata->irq_base + MAX14577_IRQ_INT2_VBVOLT;
	muic_data->switch_data = &sec_switch_data;
	muic_data->pdata = mfd_pdata->muic_pdata;
	muic_data->attached_dev = ATTACHED_DEV_UNKNOWN_MUIC;
	muic_data->is_usb_ready = false;
	muic_data->is_muic_ready = false;

	pr_info("%s:%s adcerr(%d), adc(%d), chgtyp(%d), vbvolt(%d)\n",
			MUIC_DEV_NAME, __func__, muic_data->irq_adcerr,
			muic_data->irq_adc, muic_data->irq_chgtyp,
			muic_data->irq_vbvolt);

	platform_set_drvdata(pdev, muic_data);

	if (muic_data->pdata->init_gpio_cb) {
		ret = muic_data->pdata->init_gpio_cb(get_switch_sel());
		if (ret) {
			pr_err("%s: failed to init gpio(%d)\n", __func__, ret);
			goto fail;
		}
	}

	/* Set ADC debounce time: 25ms */
	max14577_muic_set_adcdbset(muic_data, 2);

	/* Set Charger-Detection Check Time: 625ms */
	max14577_muic_set_dchktm(muic_data, 1);

	/* create sysfs group */
	ret = sysfs_create_group(&switch_device->kobj, &max14577_muic_group);
	if (ret) {
		pr_err("%s: failed to create max14577 muic attribute group\n",
				__func__);
		goto fail;
	}
	dev_set_drvdata(switch_device, muic_data);

	if (muic_data->switch_data->init_cb)
		muic_data->switch_data->init_cb();

	ret = max14577_muic_irq_init(muic_data);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to initialize MUIC irq:%d\n", ret);
		goto fail_init_irq;
	}

	/* initial cable detection */
	INIT_DELAYED_WORK(&muic_data->init_work, max14577_muic_init_detect);
	schedule_delayed_work(&muic_data->init_work,
			msecs_to_jiffies(CONFIG_MUIC_INIT_DETECT_DELAY));

	INIT_DELAYED_WORK(&muic_data->usb_work, max14577_muic_usb_detect);
	schedule_delayed_work(&muic_data->usb_work, msecs_to_jiffies(17000));

	return 0;

fail_init_irq:
	if (muic_data->irq_adcerr)
		free_irq(muic_data->irq_adcerr, NULL);
	if (muic_data->irq_adc)
		free_irq(muic_data->irq_adc, NULL);
	if (muic_data->irq_chgtyp)
		free_irq(muic_data->irq_chgtyp, NULL);
	if (muic_data->irq_vbvolt)
		free_irq(muic_data->irq_vbvolt, NULL);
fail:
	platform_set_drvdata(pdev, NULL);
	mutex_destroy(&muic_data->muic_mutex);
err_kfree:
	kfree(muic_data);
err_return:
	return ret;
}

static int __devexit max14577_muic_remove(struct platform_device *pdev)
{
	struct max14577_muic_data *muic_data = platform_get_drvdata(pdev);

	sysfs_remove_group(&switch_device->kobj, &max14577_muic_group);

	if (muic_data) {
		pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
		cancel_delayed_work(&muic_data->init_work);
		cancel_delayed_work(&muic_data->usb_work);

		free_irq(muic_data->irq_adc, muic_data);
		free_irq(muic_data->irq_chgtyp, muic_data);
		free_irq(muic_data->irq_vbvolt, muic_data);
		free_irq(muic_data->irq_adcerr, muic_data);

		mutex_destroy(&muic_data->muic_mutex);
		kfree(muic_data);
	}

	return 0;
}

void max14577_muic_shutdown(struct device *dev)
{
	struct max14577_muic_data *muic_data = dev_get_drvdata(dev);
	struct max14577_dev *max14577;
	int ret;
	u8 val;

	pr_info("%s:%s +\n", MUIC_DEV_NAME, __func__);
	if (!muic_data->i2c) {
		pr_err("%s:%s no muic i2c client\n", MUIC_DEV_NAME, __func__);
		return;
	}

	max14577 = i2c_get_clientdata(muic_data->i2c);
	pr_info("%s:%s max14577->i2c_lock.count.counter=%d\n", MUIC_DEV_NAME,
		__func__, max14577->i2c_lock.count.counter);

	pr_info("%s:%s JIGSet: auto detection\n", MUIC_DEV_NAME, __func__);
	val = (0 << CTRL3_JIGSET_SHIFT) | (0 << CTRL3_BOOTSET_SHIFT);

	ret = max14577_update_reg(muic_data->i2c, MAX14577_MUIC_REG_CONTROL3,
			val, CTRL3_JIGSET_MASK | CTRL3_BOOTSET_MASK);
	if (ret < 0) {
		pr_err("%s:%s fail to update reg\n", MUIC_DEV_NAME, __func__);
		return;
	}

	pr_info("%s:%s -\n", MUIC_DEV_NAME, __func__);
}

static struct platform_driver max14577_muic_driver = {
	.driver		= {
		.name	= MUIC_DEV_NAME,
		.owner	= THIS_MODULE,
		.shutdown = max14577_muic_shutdown,
	},
	.probe		= max14577_muic_probe,
	.remove		= __devexit_p(max14577_muic_remove),
};

static int __init max14577_muic_init(void)
{
	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
	return platform_driver_register(&max14577_muic_driver);
}
module_init(max14577_muic_init);

static void __exit max14577_muic_exit(void)
{
	pr_info("%s:%s\n", MUIC_DEV_NAME, __func__);
	platform_driver_unregister(&max14577_muic_driver);
}
module_exit(max14577_muic_exit);


MODULE_DESCRIPTION("Maxim MAX14577 MUIC driver");
MODULE_AUTHOR("<sjin.hahn@samsung.com>");
MODULE_LICENSE("GPL");
