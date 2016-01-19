/*
 * wm8994-core.c  --  Device access for Wolfson WM8994
 *
 * Copyright 2009 Wolfson Microelectronics PLC.
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mfd/core.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>

#include <linux/mfd/wm8994/core.h>
#include <linux/mfd/wm8994/pdata.h>
#include <linux/mfd/wm8994/registers.h>

static int wm8994_read(struct wm8994 *wm8994, unsigned short reg,
		       int bytes, void *dest)
{
	int ret, i;
	u16 *buf = dest;

	BUG_ON(bytes % 2);
	BUG_ON(bytes <= 0);

	ret = wm8994->read_dev(wm8994, reg, bytes, dest);
	if (ret < 0)
		return ret;

	for (i = 0; i < bytes / 2; i++) {
		dev_vdbg(wm8994->dev, "Read %04x from R%d(0x%x)\n",
			 be16_to_cpu(buf[i]), reg + i, reg + i);
	}

	return 0;
}

/**
 * wm8994_reg_read: Read a single WM8994 register.
 *
 * @wm8994: Device to read from.
 * @reg: Register to read.
 */
int wm8994_reg_read(struct wm8994 *wm8994, unsigned short reg)
{
	unsigned short val;
	int ret;

	mutex_lock(&wm8994->io_lock);

	ret = wm8994_read(wm8994, reg, 2, &val);

	mutex_unlock(&wm8994->io_lock);

	if (ret < 0)
		return ret;
	else
		return be16_to_cpu(val);
}
EXPORT_SYMBOL_GPL(wm8994_reg_read);

/**
 * wm8994_bulk_read: Read multiple WM8994 registers
 *
 * @wm8994: Device to read from
 * @reg: First register
 * @count: Number of registers
 * @buf: Buffer to fill.  The data will be returned big endian.
 */
int wm8994_bulk_read(struct wm8994 *wm8994, unsigned short reg,
		     int count, u16 *buf)
{
	int ret;

	mutex_lock(&wm8994->io_lock);

	ret = wm8994_read(wm8994, reg, count * 2, buf);

	mutex_unlock(&wm8994->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(wm8994_bulk_read);

static int wm8994_write(struct wm8994 *wm8994, unsigned short reg,
			int bytes, const void *src)
{
	const u16 *buf = src;
	int i;

	BUG_ON(bytes % 2);
	BUG_ON(bytes <= 0);

	for (i = 0; i < bytes / 2; i++) {
		dev_vdbg(wm8994->dev, "Write %04x to R%d(0x%x)\n",
			 be16_to_cpu(buf[i]), reg + i, reg + i);
	}

	return wm8994->write_dev(wm8994, reg, bytes, src);
}

/**
 * wm8994_reg_write: Write a single WM8994 register.
 *
 * @wm8994: Device to write to.
 * @reg: Register to write to.
 * @val: Value to write.
 */
int wm8994_reg_write(struct wm8994 *wm8994, unsigned short reg,
		     unsigned short val)
{
	int ret;

	val = cpu_to_be16(val);

	mutex_lock(&wm8994->io_lock);

	ret = wm8994_write(wm8994, reg, 2, &val);

	mutex_unlock(&wm8994->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(wm8994_reg_write);

/**
 * wm8994_bulk_write: Write multiple WM8994 registers
 *
 * @wm8994: Device to write to
 * @reg: First register
 * @count: Number of registers
 * @buf: Buffer to write from.  Data must be big-endian formatted.
 */
int wm8994_bulk_write(struct wm8994 *wm8994, unsigned short reg,
		      int count, const u16 *buf)
{
	int ret;

	mutex_lock(&wm8994->io_lock);

	ret = wm8994_write(wm8994, reg, count * 2, buf);

	mutex_unlock(&wm8994->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(wm8994_bulk_write);

/**
 * wm8994_set_bits: Set the value of a bitfield in a WM8994 register
 *
 * @wm8994: Device to write to.
 * @reg: Register to write to.
 * @mask: Mask of bits to set.
 * @val: Value to set (unshifted)
 */
int wm8994_set_bits(struct wm8994 *wm8994, unsigned short reg,
		    unsigned short mask, unsigned short val)
{
	int ret;
	u16 r;

	mutex_lock(&wm8994->io_lock);

	ret = wm8994_read(wm8994, reg, 2, &r);
	if (ret < 0)
		goto out;

	r = be16_to_cpu(r);

	r &= ~mask;
	r |= val;

	r = cpu_to_be16(r);

	ret = wm8994_write(wm8994, reg, 2, &r);

out:
	mutex_unlock(&wm8994->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(wm8994_set_bits);

static struct mfd_cell wm8994_regulator_devs[] = {
	{
		.name = "wm8994-ldo",
		.id = 1,
		.pm_runtime_no_callbacks = true,
	},
	{
		.name = "wm8994-ldo",
		.id = 2,
		.pm_runtime_no_callbacks = true,
	},
};

static struct resource wm8994_codec_resources[] = {
	{
		.start = WM8994_IRQ_TEMP_SHUT,
		.end   = WM8994_IRQ_TEMP_WARN,
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource wm8994_gpio_resources[] = {
	{
		.start = WM8994_IRQ_GPIO(1),
		.end   = WM8994_IRQ_GPIO(11),
		.flags = IORESOURCE_IRQ,
	},
};

static struct mfd_cell wm8994_devs[] = {
	{
		.name = "wm8994-codec",
		.num_resources = ARRAY_SIZE(wm8994_codec_resources),
		.resources = wm8994_codec_resources,
	},

	{
		.name = "wm8994-gpio",
		.num_resources = ARRAY_SIZE(wm8994_gpio_resources),
		.resources = wm8994_gpio_resources,
		.pm_runtime_no_callbacks = true,
	},
};

/*
 * Supplies for the main bulk of CODEC; the LDO supplies are ignored
 * and should be handled via the standard regulator API supply
 * management.
 */
static const char *wm1811_main_supplies[] = {
	"DBVDD1",
	"DBVDD2",
	"DBVDD3",
	"DCVDD",
	"AVDD1",
	"AVDD2",
	"CPVDD",
	"SPKVDD1",
	"SPKVDD2",
};

static const char *wm8994_main_supplies[] = {
	"DBVDD",
	"DCVDD",
	"AVDD1",
	"AVDD2",
	"CPVDD",
	"SPKVDD1",
	"SPKVDD2",
};

static const char *wm8958_main_supplies[] = {
	"DBVDD1",
	"DBVDD2",
	"DBVDD3",
	"DCVDD",
	"AVDD1",
	"AVDD2",
	"CPVDD",
	"SPKVDD1",
	"SPKVDD2",
};

#ifdef CONFIG_PM
static int wm8994_suspend(struct device *dev)
{
	struct wm8994 *wm8994 = dev_get_drvdata(dev);
	int ret;

	/* Don't actually go through with the suspend if the CODEC is
	 * still active (eg, for audio passthrough from CP. */
	ret = wm8994_reg_read(wm8994, WM8994_POWER_MANAGEMENT_1);
	if (ret < 0) {
		dev_err(dev, "Failed to read power status: %d\n", ret);
	} else if (ret & WM8994_VMID_SEL_MASK) {
		dev_dbg(dev, "CODEC still active, ignoring suspend\n");
		return 0;
	}

	ret = wm8994_reg_read(wm8994, WM8994_POWER_MANAGEMENT_4);
	if (ret < 0) {
		dev_err(dev, "Failed to read power status: %d\n", ret);
	} else if (ret & (WM8994_AIF2ADCL_ENA | WM8994_AIF2ADCR_ENA |
			  WM8994_AIF1ADC2L_ENA | WM8994_AIF1ADC2R_ENA |
			  WM8994_AIF1ADC1L_ENA | WM8994_AIF1ADC1R_ENA)) {
		dev_dbg(dev, "CODEC still active, ignoring suspend\n");
		return 0;
	}

	ret = wm8994_reg_read(wm8994, WM8994_POWER_MANAGEMENT_5);
	if (ret < 0) {
		dev_err(dev, "Failed to read power status: %d\n", ret);
	} else if (ret & (WM8994_AIF2DACL_ENA | WM8994_AIF2DACR_ENA |
			  WM8994_AIF1DAC2L_ENA | WM8994_AIF1DAC2R_ENA |
			  WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA)) {
		dev_dbg(dev, "CODEC still active, ignoring suspend\n");
		return 0;
	}

	switch (wm8994->type) {
	case WM8958:
	case WM1811:
		ret = wm8994_reg_read(wm8994, WM8958_MIC_DETECT_1);
		if (ret < 0) {
			dev_err(dev, "Failed to read power status: %d\n", ret);
		} else if (ret & WM8958_MICD_ENA) {
			dev_dbg(dev, "CODEC still active, ignoring suspend\n");
			return 0;
		}
		break;
	default:
		break;
	}

	switch (wm8994->type) {
	case WM1811:
		ret = wm8994_reg_read(wm8994, WM8994_ANTIPOP_2);
		if (ret < 0) {
			dev_err(dev, "Failed to read jackdet: %d\n", ret);
		} else if (ret & WM1811_JACKDET_MODE_MASK) {
			dev_dbg(dev, "CODEC still active, ignoring suspend\n");
			return 0;
		}
		break;
	default:
		break;
	}

	/* Disable LDO pulldowns while the device is suspended if we
	 * don't know that something will be driving them. */
	if (!wm8994->ldo_ena_always_driven)
		wm8994_set_bits(wm8994, WM8994_PULL_CONTROL_2,
				WM8994_LDO1ENA_PD | WM8994_LDO2ENA_PD,
				WM8994_LDO1ENA_PD | WM8994_LDO2ENA_PD);

	/* GPIO configuration state is saved here since we may be configuring
	 * the GPIO alternate functions even if we're not using the gpiolib
	 * driver for them.
	 */
	ret = wm8994_read(wm8994, WM8994_GPIO_1, WM8994_NUM_GPIO_REGS * 2,
			  &wm8994->gpio_regs);
	if (ret < 0)
		dev_err(dev, "Failed to save GPIO registers: %d\n", ret);

	/* For similar reasons we also stash the regulator states */
	ret = wm8994_read(wm8994, WM8994_LDO_1, WM8994_NUM_LDO_REGS * 2,
			  &wm8994->ldo_regs);
	if (ret < 0)
		dev_err(dev, "Failed to save LDO registers: %d\n", ret);

	/* Explicitly put the device into reset in case regulators
	 * don't get disabled in order to ensure consistent restart.
	 */
	wm8994_reg_write(wm8994, WM8994_SOFTWARE_RESET, 0x8994);

	wm8994->suspended = true;

	ret = regulator_bulk_disable(wm8994->num_supplies,
				     wm8994->supplies);
	if (ret != 0) {
		dev_err(dev, "Failed to disable supplies: %d\n", ret);
		return ret;
	}

	return 0;
}

static int wm8994_resume(struct device *dev)
{
	struct wm8994 *wm8994 = dev_get_drvdata(dev);
	int ret, i;

	/* We may have lied to the PM core about suspending */
	if (!wm8994->suspended)
		return 0;

	ret = regulator_bulk_enable(wm8994->num_supplies,
				    wm8994->supplies);
	if (ret != 0) {
		dev_err(dev, "Failed to enable supplies: %d\n", ret);
		return ret;
	}

	/* Write register at a time as we use the cache on the CPU so store
	 * it in native endian.
	 */
	for (i = 0; i < ARRAY_SIZE(wm8994->irq_masks_cur); i++) {
		ret = wm8994_reg_write(wm8994, WM8994_INTERRUPT_STATUS_1_MASK
				       + i, wm8994->irq_masks_cur[i]);
		if (ret < 0)
			dev_err(dev, "Failed to restore interrupt masks: %d\n",
				ret);
	}

	ret = wm8994_write(wm8994, WM8994_LDO_1, WM8994_NUM_LDO_REGS * 2,
			   &wm8994->ldo_regs);
	if (ret < 0)
		dev_err(dev, "Failed to restore LDO registers: %d\n", ret);

	ret = wm8994_write(wm8994, WM8994_GPIO_1, WM8994_NUM_GPIO_REGS * 2,
			   &wm8994->gpio_regs);
	if (ret < 0)
		dev_err(dev, "Failed to restore GPIO registers: %d\n", ret);

	/* Disable LDO pulldowns while the device is active */
	wm8994_set_bits(wm8994, WM8994_PULL_CONTROL_2,
			WM8994_LDO1ENA_PD | WM8994_LDO2ENA_PD,
			0);

	wm8994->suspended = false;

	return 0;
}
#endif

#ifdef CONFIG_REGULATOR
static int wm8994_ldo_in_use(struct wm8994_pdata *pdata, int ldo)
{
	struct wm8994_ldo_pdata *ldo_pdata;

	if (!pdata)
		return 0;

	ldo_pdata = &pdata->ldo[ldo];

	if (!ldo_pdata->init_data)
		return 0;

	return ldo_pdata->init_data->num_consumer_supplies != 0;
}
#else
static int wm8994_ldo_in_use(struct wm8994_pdata *pdata, int ldo)
{
	return 0;
}
#endif

/*
 * Instantiate the generic non-control parts of the device.
 */
static int wm8994_device_init(struct wm8994 *wm8994, int irq)
{
	struct wm8994_pdata *pdata = wm8994->dev->platform_data;
	const char *devname;
	int ret, i;
	int pulls = 0;

	mutex_init(&wm8994->io_lock);
	dev_set_drvdata(wm8994->dev, wm8994);

	/* Add the on-chip regulators first for bootstrapping */
	ret = mfd_add_devices(wm8994->dev, -1,
			      wm8994_regulator_devs,
			      ARRAY_SIZE(wm8994_regulator_devs),
			      NULL, 0);
	if (ret != 0) {
		dev_err(wm8994->dev, "Failed to add children: %d\n", ret);
		goto err;
	}

	switch (wm8994->type) {
	case WM1811:
		wm8994->num_supplies = ARRAY_SIZE(wm1811_main_supplies);
		break;
	case WM8994:
		wm8994->num_supplies = ARRAY_SIZE(wm8994_main_supplies);
		break;
	case WM8958:
		wm8994->num_supplies = ARRAY_SIZE(wm8958_main_supplies);
		break;
	default:
		BUG();
		goto err;
	}

	wm8994->supplies = kzalloc(sizeof(struct regulator_bulk_data) *
				   wm8994->num_supplies,
				   GFP_KERNEL);
	if (!wm8994->supplies) {
		ret = -ENOMEM;
		goto err;
	}

	switch (wm8994->type) {
	case WM1811:
		for (i = 0; i < ARRAY_SIZE(wm1811_main_supplies); i++)
			wm8994->supplies[i].supply = wm1811_main_supplies[i];
		break;
	case WM8994:
		for (i = 0; i < ARRAY_SIZE(wm8994_main_supplies); i++)
			wm8994->supplies[i].supply = wm8994_main_supplies[i];
		break;
	case WM8958:
		for (i = 0; i < ARRAY_SIZE(wm8958_main_supplies); i++)
			wm8994->supplies[i].supply = wm8958_main_supplies[i];
		break;
	default:
		BUG();
		goto err;
	}

	ret = regulator_bulk_get(wm8994->dev, wm8994->num_supplies,
				 wm8994->supplies);
	if (ret != 0) {
		dev_err(wm8994->dev, "Failed to get supplies: %d\n", ret);
		goto err_supplies;
	}

	ret = regulator_bulk_enable(wm8994->num_supplies,
				    wm8994->supplies);
	if (ret != 0) {
		dev_err(wm8994->dev, "Failed to enable supplies: %d\n", ret);
		goto err_get;
	}

	ret = wm8994_reg_read(wm8994, WM8994_SOFTWARE_RESET);
	if (ret < 0) {
		dev_err(wm8994->dev, "Failed to read ID register\n");
		goto err_enable;
	} else
		dev_info(wm8994->dev, "Succeeded to read ID register\n");

	switch (ret) {
	case 0x1811:
		devname = "WM1811";
		if (wm8994->type != WM1811)
			dev_warn(wm8994->dev, "Device registered as type %d\n",
				 wm8994->type);
		wm8994->type = WM1811;

		/* Samsung-specific customization of MICBIAS levels */
		wm8994_reg_write(wm8994, 0x102, 0x3);
		wm8994_reg_write(wm8994, 0xcb, 0x5151);
		wm8994_reg_write(wm8994, 0xd3, 0x3f3f);
		wm8994_reg_write(wm8994, 0xd4, 0x3f3f);
		wm8994_reg_write(wm8994, 0xd5, 0x3f3f);
		wm8994_reg_write(wm8994, 0xd6, 0x3226);
		wm8994_reg_write(wm8994, 0x4c, 0x991f);
		wm8994_reg_write(wm8994, 0x4d, 0x2513);
		wm8994_reg_write(wm8994, 0x102, 0x0);
		wm8994_reg_write(wm8994, 0xd1, 0x87);
		wm8994_reg_write(wm8994, 0x3b, 0x9);
		wm8994_reg_write(wm8994, 0x3c, 0x2);
		break;
	case 0x8994:
		devname = "WM8994";
		if (wm8994->type != WM8994)
			dev_warn(wm8994->dev, "Device registered as type %d\n",
				 wm8994->type);
		wm8994->type = WM8994;
		break;
	case 0x8958:
		devname = "WM8958";
		if (wm8994->type != WM8958)
			dev_warn(wm8994->dev, "Device registered as type %d\n",
				 wm8994->type);
		wm8994->type = WM8958;
		break;
	default:
		dev_err(wm8994->dev, "Device is not a WM8994, ID is %x\n",
			ret);
		ret = -EINVAL;
		goto err_enable;
	}

	ret = wm8994_reg_read(wm8994, WM8994_CHIP_REVISION);
	if (ret < 0) {
		dev_err(wm8994->dev, "Failed to read revision register: %d\n",
			ret);
		goto err_enable;
	}

	wm8994->revision = ret & WM8994_CHIP_REV_MASK;
	wm8994->cust_id = (ret & WM8994_CUST_ID_MASK) >> WM8994_CUST_ID_SHIFT;

	switch (wm8994->type) {
	case WM8994:
		switch (wm8994->revision) {
		case 0:
		case 1:
			dev_warn(wm8994->dev,
				 "revision %c not fully supported\n",
				 'A' + wm8994->revision);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	dev_info(wm8994->dev, "%s revision %c CUST_ID %02x\n", devname,
		 'A' + wm8994->revision, wm8994->cust_id);

	if (pdata) {
		wm8994->irq_base = pdata->irq_base;
		wm8994->gpio_base = pdata->gpio_base;

		/* GPIO configuration is only applied if it's non-zero */
		for (i = 0; i < ARRAY_SIZE(pdata->gpio_defaults); i++) {
			if (pdata->gpio_defaults[i]) {
				wm8994_set_bits(wm8994, WM8994_GPIO_1 + i,
						0xffff,
						pdata->gpio_defaults[i]);
			}
		}

		wm8994->ldo_ena_always_driven = pdata->ldo_ena_always_driven;

		if (pdata->spkmode_pu)
			pulls |= WM8994_SPKMODE_PU;
	}

	/* Disable unneeded pulls */
	wm8994_set_bits(wm8994, WM8994_PULL_CONTROL_2,
			WM8994_LDO1ENA_PD | WM8994_LDO2ENA_PD |
			WM8994_SPKMODE_PU | WM8994_CSNADDR_PD,
			pulls);

	/* In some system designs where the regulators are not in use,
	 * we can achieve a small reduction in leakage currents by
	 * floating LDO outputs.  This bit makes no difference if the
	 * LDOs are enabled, it only affects cases where the LDOs were
	 * in operation and are then disabled.
	 */
	for (i = 0; i < WM8994_NUM_LDO_REGS; i++) {
		if (wm8994_ldo_in_use(pdata, i))
			wm8994_set_bits(wm8994, WM8994_LDO_1 + i,
					WM8994_LDO1_DISCH, WM8994_LDO1_DISCH);
		else
			wm8994_set_bits(wm8994, WM8994_LDO_1 + i,
					WM8994_LDO1_DISCH, 0);
	}

	wm8994_irq_init(wm8994);

	ret = mfd_add_devices(wm8994->dev, -1,
			      wm8994_devs, ARRAY_SIZE(wm8994_devs),
			      NULL, 0);
	if (ret != 0) {
		dev_err(wm8994->dev, "Failed to add children: %d\n", ret);
		goto err_irq;
	}

#if 0 /* To do */
	pm_runtime_enable(wm8994->dev);
	pm_runtime_resume(wm8994->dev);
#endif

	return 0;

err_irq:
	wm8994_irq_exit(wm8994);
err_enable:
	regulator_bulk_disable(wm8994->num_supplies,
			       wm8994->supplies);
err_get:
	regulator_bulk_free(wm8994->num_supplies, wm8994->supplies);
err_supplies:
	kfree(wm8994->supplies);
err:
	mfd_remove_devices(wm8994->dev);
	return ret;
}

static void wm8994_device_exit(struct wm8994 *wm8994)
{
	pm_runtime_disable(wm8994->dev);
	mfd_remove_devices(wm8994->dev);
	wm8994_irq_exit(wm8994);
	regulator_bulk_disable(wm8994->num_supplies,
			       wm8994->supplies);
	regulator_bulk_free(wm8994->num_supplies, wm8994->supplies);
	kfree(wm8994->supplies);
	kfree(wm8994);
}

static int wm8994_i2c_read_device(struct wm8994 *wm8994, unsigned short reg,
				  int bytes, void *dest)
{
	struct i2c_client *i2c = wm8994->control_data;
	int ret;
	u16 r = cpu_to_be16(reg);

	ret = i2c_master_send(i2c, (unsigned char *)&r, 2);
	if (ret < 0) {
		printk(KERN_DEBUG"[%s] i2c read fail reg[%x], ret[%d]\n",
				__func__, reg, ret);
		return ret;
	}
	if (ret != 2) {
		printk(KERN_DEBUG"[%s] read fail reg[%x], ret EIO\n",
				__func__, reg);
		return -EIO;
		}

	ret = i2c_master_recv(i2c, dest, bytes);
	if (ret < 0)
		return ret;
	if (ret != bytes)
		return -EIO;
	return 0;
}

static int wm8994_i2c_gather_write_device(struct wm8994 *wm8994, unsigned short reg,
				   int bytes, const void *src)
{
	struct i2c_client *i2c = wm8994->control_data;
	struct i2c_msg xfer[2];
	int ret;

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_PROTOCOL_MANGLING)) {
		dev_vdbg(wm8994->dev,
				"%s: I2C Controller does _NOT_ support block/gather\n",
				__func__);
		return -ENOTSUPP;
	}

	dev_vdbg(wm8994->dev,
		 "%s:  gather write - Reg = 0x%04x, bytes = 0x%x\n",
		 __func__, reg, bytes);

	reg = cpu_to_be16(reg);

	xfer[0].addr = i2c->addr;
	xfer[0].flags = 0;
	xfer[0].len = 2;
	xfer[0].buf = (char *)&reg;

	xfer[1].addr = i2c->addr;
	xfer[1].flags = I2C_M_NOSTART;
	xfer[1].len = bytes;
	xfer[1].buf = (char *)src;

	ret = i2c_transfer(i2c->adapter, xfer, 2);
	if (ret < 0) {
		printk(KERN_DEBUG"[%s] write fail reg[%x], ret[%d]\n",
				__func__, reg, ret);
		return ret;
		}
	if (ret != 2) {
		printk(KERN_DEBUG"[%s] write fail reg[%x], ret EIO\n",
				__func__, reg);
		return -EIO;
		}

	return 0;
}

static int wm8994_i2c_write_device(struct wm8994 *wm8994, unsigned short reg,
				   int bytes, const void *src)
{
	struct i2c_client *i2c = wm8994->control_data;
	int ret;

	unsigned char msg[2 + 2];
	void *buf;

	/* If we're doing a single register write we can probably just
	 * send the work_buf directly, otherwise try to do a gather
	 * write.
	 */
	if (bytes == 2) {
		dev_vdbg(wm8994->dev,
			 "%s: Single register write - Reg = 0x%04x, bytes = 0x%x\n",
			 __func__, reg, bytes);

		reg = cpu_to_be16(reg);
		memcpy(&msg[0], &reg, 2);
		memcpy(&msg[2], src, bytes);

		ret = i2c_master_send(i2c, msg, bytes + 2);
		if (ret < 0) {
			printk(KERN_DEBUG"[%s] write fail reg[%x], ret[%d]\n",
					__func__, reg, ret);
			return ret;
			}
		if (ret < bytes + 2) {
			printk(KERN_DEBUG"[%s] write fail reg[%x], ret EIO\n",
					__func__, reg);
			return -EIO;
			}
		return 0;
	} else {
		ret = wm8994_i2c_gather_write_device(wm8994, reg, bytes, src);
	}

	if (ret == -ENOTSUPP) {
		dev_vdbg(wm8994->dev,
			 "%s: Manual group write - Reg = 0x%04x, bytes = 0x%x\n",
			 __func__, reg, bytes);

		buf = kmalloc(2 + bytes, GFP_KERNEL);
		if (!buf)
			return -ENOMEM;

		reg = cpu_to_be16(reg);
		memcpy(buf, &reg, 2);
		memcpy(buf + 2, src, bytes);

		ret = i2c_master_send(i2c, buf, bytes + 2);

		kfree(buf);

		if (ret < 0)
			return ret;
		if (ret < bytes + 2)
			return -EIO;

		return 0;
	}
	return ret;
}

static int wm8994_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct wm8994 *wm8994;

	wm8994 = kzalloc(sizeof(struct wm8994), GFP_KERNEL);
	if (wm8994 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, wm8994);
	wm8994->dev = &i2c->dev;
	wm8994->control_data = i2c;
	wm8994->read_dev = wm8994_i2c_read_device;
	wm8994->write_dev = wm8994_i2c_write_device;
	wm8994->irq = i2c->irq;
	wm8994->type = id->driver_data;

	if (wm8994_device_init(wm8994, i2c->irq) < 0) {
		kfree(wm8994);
		return -ENODEV;
	}

	return 0;
}

#if defined(CONFIG_MACH_BAFFIN)
static void wm8994_i2c_shutdown(struct i2c_client *i2c)
{
	struct wm8994 *wm8994 = i2c_get_clientdata(i2c);
	int ret;

	dev_vdbg(wm8994->dev, "%s: ++\n", __func__);

    /* Explicitly put the device into reset in case regulators
     * don't get disabled in order to ensure consistent restart.
     */
	wm8994_reg_write(wm8994, WM8994_SOFTWARE_RESET, 0x0000);

	wm8994->suspended = true;

	ret = regulator_bulk_disable(wm8994->num_supplies,
			       wm8994->supplies);
	if (ret != 0) {
		dev_err(wm8994->dev, "Failed to disable supplies: %d\n", ret);
		return;
	}

	dev_info(wm8994->dev, "%s: --\n", __func__);

	return;
}
#endif

static int wm8994_i2c_remove(struct i2c_client *i2c)
{
	struct wm8994 *wm8994 = i2c_get_clientdata(i2c);

	wm8994_device_exit(wm8994);

	return 0;
}

static const struct i2c_device_id wm8994_i2c_id[] = {
	{ "wm1811", WM1811 },
	{ "wm8994", WM8994 },
	{ "wm8958", WM8958 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, wm8994_i2c_id);

static UNIVERSAL_DEV_PM_OPS(wm8994_pm_ops, wm8994_suspend, wm8994_resume,
			    NULL);

static struct i2c_driver wm8994_i2c_driver = {
	.driver = {
		.name = "wm8994",
		.owner = THIS_MODULE,
		.pm = &wm8994_pm_ops,
	},
	.probe = wm8994_i2c_probe,
	.remove = wm8994_i2c_remove,
#if defined(CONFIG_MACH_BAFFIN)
	.shutdown = wm8994_i2c_shutdown,
#endif
	.id_table = wm8994_i2c_id,
};

static int __init wm8994_i2c_init(void)
{
	int ret;

	ret = i2c_add_driver(&wm8994_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register wm8994 I2C driver: %d\n", ret);

	return ret;
}
module_init(wm8994_i2c_init);

static void __exit wm8994_i2c_exit(void)
{
	i2c_del_driver(&wm8994_i2c_driver);
}
module_exit(wm8994_i2c_exit);

MODULE_DESCRIPTION("Core support for the WM8994 audio CODEC");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mark Brown <broonie@opensource.wolfsonmicro.com>");
