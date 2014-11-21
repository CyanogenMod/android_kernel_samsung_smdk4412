/*
 * Interrupt controller support for MAX8997
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 *
 * based on max8998-irq.c
 *
 *  <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mfd/max8997-private.h>
#include <linux/delay.h>

extern struct class *sec_class;

#define FLED1_FLT_BIT	(1<<0)
#define FLED2_FLT_BIT	(1<<1)
#define FLED_FLT_MASK	(FLED1_FLT_BIT | FLED2_FLT_BIT)

static u8 fled_status;

struct max8997_irq_data {
	int reg;
	int mask;
};

static struct max8997_irq_data max8997_irqs[] = {
	[MAX8997_IRQ_PWRONR] = {
		.reg = 1,
		.mask = MAX8997_IRQ_PWRONR_MASK,
	},
	[MAX8997_IRQ_PWRONF] = {
		.reg = 1,
		.mask = MAX8997_IRQ_PWRONF_MASK,
	},
	[MAX8997_IRQ_PWRON1SEC] = {
		.reg = 1,
		.mask = MAX8997_IRQ_PWRON1SEC_MASK,
	},
	[MAX8997_IRQ_JIGONR] = {
		.reg = 1,
		.mask = MAX8997_IRQ_JIGONR_MASK,
	},
	[MAX8997_IRQ_JIGONF] = {
		.reg = 1,
		.mask = MAX8997_IRQ_JIGONF_MASK,
	},
	[MAX8997_IRQ_LOWBAT2] = {
		.reg = 1,
		.mask = MAX8997_IRQ_LOWBAT2_MASK,
	},
	[MAX8997_IRQ_LOWBAT1] = {
		.reg = 1,
		.mask = MAX8997_IRQ_LOWBAT1_MASK,
	},
	[MAX8997_IRQ_JIGR] = {
		.reg = 2,
		.mask = MAX8997_IRQ_JIGR_MASK,
	},
	[MAX8997_IRQ_JIGF] = {
		.reg = 2,
		.mask = MAX8997_IRQ_JIGF_MASK,
	},
	[MAX8997_IRQ_MR] = {
		.reg = 2,
		.mask = MAX8997_IRQ_MR_MASK,
	},
	[MAX8997_IRQ_DVS1OK] = {
		.reg = 2,
		.mask = MAX8997_IRQ_DVS1OK_MASK,
	},
	[MAX8997_IRQ_DVS2OK] = {
		.reg = 2,
		.mask = MAX8997_IRQ_DVS2OK_MASK,
	},
	[MAX8997_IRQ_DVS4OK] = {
		.reg = 2,
		.mask = MAX8997_IRQ_DVS4OK_MASK,
	},
	[MAX8997_IRQ_DVS5OK] = {
		.reg = 2,
		.mask = MAX8997_IRQ_DVS5OK_MASK,
	},
	[MAX8997_IRQ_CHGINS] = {
		.reg = 3,
		.mask = MAX8997_IRQ_CHGINS_MASK,
	},
	[MAX8997_IRQ_CHGRM] = {
		.reg = 3,
		.mask = MAX8997_IRQ_CHGRM_MASK,
	},
	[MAX8997_IRQ_DCINOVP] = {
		.reg = 3,
		.mask = MAX8997_IRQ_DCINOVP_MASK,
	},
	[MAX8997_IRQ_TOPOFF] = {
		.reg = 3,
		.mask = MAX8997_IRQ_TOPOFF_MASK,
	},
	[MAX8997_IRQ_CHGRSTF] = {
		.reg = 3,
		.mask = MAX8997_IRQ_CHGRSTF_MASK,
	},
	[MAX8997_IRQ_MBCHGTMEXPD] = {
		.reg = 3,
		.mask = MAX8997_IRQ_MBCHGTMEXPD_MASK,
	},
	[MAX8997_IRQ_RTC60S] = {
		.reg = 4,
		.mask = MAX8997_IRQ_RTC60S_MASK,
	},
	[MAX8997_IRQ_RTCA1] = {
		.reg = 4,
		.mask = MAX8997_IRQ_RTCA1_MASK,
	},
	[MAX8997_IRQ_RTCA2] = {
		.reg = 4,
		.mask = MAX8997_IRQ_RTCA2_MASK,
	},
	[MAX8997_IRQ_SMPL_INT] = {
		.reg = 4,
		.mask = MAX8997_IRQ_SMPL_INT_MASK,
	},
	[MAX8997_IRQ_RTC1S] = {
		.reg = 4,
		.mask = MAX8997_IRQ_RTC1S_MASK,
	},
	[MAX8997_IRQ_WTSR] = {
		.reg = 4,
		.mask = MAX8997_IRQ_WTSR_MASK,
	},
	[MAX8997_IRQ_ADC] = {
		.reg = 5,
		.mask = MAX8997_IRQ_ADC_MASK,
	},
	[MAX8997_IRQ_ADCLOW] = {
		.reg = 5,
		.mask = MAX8997_IRQ_ADCLOW_MASK,
	},
	[MAX8997_IRQ_ADCERR] = {
		.reg = 5,
		.mask = MAX8997_IRQ_ADCERR_MASK,
	},
	[MAX8997_IRQ_CHGTYP] = {
		.reg = 6,
		.mask = MAX8997_IRQ_CHGTYP_MASK,
	},
	[MAX8997_IRQ_CHGDETRUN] = {
		.reg = 6,
		.mask = MAX8997_IRQ_CHGDETRUN_MASK,
	},
	[MAX8997_IRQ_DCDTMR] = {
		.reg = 6,
		.mask = MAX8997_IRQ_DCDTMR_MASK,
	},
	[MAX8997_IRQ_DBCHG] = {
		.reg = 6,
		.mask = MAX8997_IRQ_DCDTMR_MASK,
	},
	[MAX8997_IRQ_VBVOLT] = {
		.reg = 6,
		.mask = MAX8997_IRQ_VBVOLT_MASK,
	},
	[MAX8997_IRQ_OVP] = {
		.reg = 7,
		.mask = MAX8997_IRQ_OVP_MASK,
	},
};

static inline struct max8997_irq_data *
irq_to_max8997_irq(struct max8997_dev *max8997, int irq)
{
	return &max8997_irqs[irq - max8997->irq_base];
}

static void max8997_irq_lock(struct irq_data *data)
{
	struct max8997_dev *max8997 = irq_get_chip_data(data->irq);

	mutex_lock(&max8997->irqlock);
}

static void max8997_irq_sync_unlock(struct irq_data *data)
{
	struct max8997_dev *max8997 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < ARRAY_SIZE(max8997->irq_masks_cur); i++) {
		/*
		 * If there's been a change in the mask write it back
		 * to the hardware.
		 */
		if (max8997->irq_masks_cur[i] != max8997->irq_masks_cache[i]) {
			max8997->irq_masks_cache[i] = max8997->irq_masks_cur[i];
			if (i < MAX8997_NUM_IRQ_PMIC_REGS)
				max8997_write_reg(max8997->i2c,
						MAX8997_REG_IRQM1 + i,
						max8997->irq_masks_cur[i]);
			else
				max8997_write_reg(max8997->muic,
						MAX8997_MUIC_REG_INTMASK1 + i
						- MAX8997_NUM_IRQ_PMIC_REGS,
						~max8997->irq_masks_cur[i]);
		}
	}

	mutex_unlock(&max8997->irqlock);
}

static void max8997_irq_unmask(struct irq_data *data)
{
	struct max8997_dev *max8997 = irq_get_chip_data(data->irq);
	struct max8997_irq_data *irq_data
		= irq_to_max8997_irq(max8997, data->irq);

	max8997->irq_masks_cur[irq_data->reg - 1] &= ~irq_data->mask;
}

static void max8997_irq_mask(struct irq_data *data)
{
	struct max8997_dev *max8997 = irq_get_chip_data(data->irq);
	struct max8997_irq_data *irq_data
		= irq_to_max8997_irq(max8997, data->irq);

	max8997->irq_masks_cur[irq_data->reg - 1] |= irq_data->mask;
}

static struct irq_chip max8997_irq_chip = {
	.name = "max8997",
	.irq_bus_lock = max8997_irq_lock,
	.irq_bus_sync_unlock = max8997_irq_sync_unlock,
	.irq_mask = max8997_irq_mask,
	.irq_unmask = max8997_irq_unmask,
};

static irqreturn_t max8997_irq_thread(int irq, void *data)
{
	struct max8997_dev *max8997 = data;
	u8 irq_src;
	u8 irq_flash;
	u8 irq_reg[MAX8997_NUM_IRQ_REGS];
	int ret;
	int i;

	ret = max8997_read_reg(max8997->i2c, MAX8997_REG_IRQ_SOURCE, &irq_src);

	if (ret < 0) {
		dev_err(max8997->dev, "Failed to read interrupt src"
				" register: %d\n", ret);
		return IRQ_NONE;
	}

	dev_info(max8997->dev, "%s: irq:%d, irq_src:0x%x\n", __func__,
			irq, irq_src);


	for (i = 0; i < MAX8997_NUM_IRQ_REGS; i++)
		irq_reg[i] = 0x0;

	if (irq_src & MAX8997_INTR_PMIC_MASK)
		ret = max8997_bulk_read(max8997->i2c, MAX8997_REG_IRQ1,
				MAX8997_NUM_IRQ_PMIC_REGS, irq_reg);
	else if (irq_src & MAX8997_INTR_MUIC_MASK)
		ret = max8997_bulk_read(max8997->muic, MAX8997_MUIC_REG_INT1,
				MAX8997_NUM_IRQ_MUIC_REGS,
				&irq_reg[MAX8997_NUM_IRQ_PMIC_REGS]);
	else if (irq_src & MAX8997_INTR_FLASH_MASK) {
		ret = max8997_read_reg(max8997->i2c, MAX8997_REG_FLASH_STATUS,
				&irq_flash);
		if (ret < 0)
			dev_err(max8997->dev, "%s: fail to read flash"
					" status: %d\n", __func__, ret);
		else {
			dev_info(max8997->dev, "%s: FLASH Interrupt: 0x%02x\n",
					__func__, irq_flash);
			fled_status = irq_flash & FLED_FLT_MASK;
		}
	} else if (irq_src & MAX8997_INTR_FUELGAUGE_MASK) {
		dev_warn(max8997->dev, "%s: Fuel Gauge interrupt\n", __func__);
		msleep(20);
	} else {
		dev_warn(max8997->dev, "Unused interrupt source: 0x%x\n",
				irq_src);
		return IRQ_NONE;
	}

	if (ret < 0) {
		dev_err(max8997->dev, "Failed to read interrupt register: %d\n",
				ret);
		return IRQ_NONE;
	}

	/* Apply masking */
	for (i = 0; i < MAX8997_NUM_IRQ_REGS; i++)
		irq_reg[i] &= ~max8997->irq_masks_cur[i];

	/* Report */
	for (i = 0; i < MAX8997_IRQ_NR; i++) {
		if (irq_reg[max8997_irqs[i].reg - 1] & max8997_irqs[i].mask)
			handle_nested_irq(max8997->irq_base + i);
	}

	return IRQ_HANDLED;
}

static ssize_t max8997_show_fled_state(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	if (fled_status & FLED1_FLT_BIT)
		sprintf(buf, "LED1 FAULT\n");
	else if (fled_status & FLED2_FLT_BIT)
		sprintf(buf, "LED2 FAULT\n");
	else
		sprintf(buf, "LED OK\n");

	pr_info("%s: %s\n", __func__, buf);

	return strlen(buf);
}

static DEVICE_ATTR(fled_state, 0444, max8997_show_fled_state, NULL);

int max8997_irq_init(struct max8997_dev *max8997)
{
	int i;
	int cur_irq;
	int ret;
	u8 pmic_intm[MAX8997_NUM_IRQ_PMIC_REGS];
	u8 muic_intm[MAX8997_NUM_IRQ_MUIC_REGS];
	u8 pmic_id;
	struct device *fled_dev;

	if (!max8997->irq) {
		dev_warn(max8997->dev,
			 "No interrupt specified, no interrupts\n");
		max8997->irq_base = 0;
		return 0;
	}

	if (!max8997->irq_base) {
		dev_err(max8997->dev,
			"No interrupt base specified, no interrupts\n");
		return 0;
	}

	for (i = 0; i < MAX8997_NUM_IRQ_PMIC_REGS; i++)
		pmic_intm[i] = 0xff;

	for (i = 0; i < MAX8997_NUM_IRQ_MUIC_REGS; i++)
		muic_intm[i] = 0x00;

	mutex_init(&max8997->irqlock);

	ret = max8997_read_reg(max8997->i2c, MAX8997_REG_ID, &pmic_id);
	if (ret)
		dev_err(max8997->dev, "%s: fail to read PMIC ID(0x%X)\n",
				__func__, ret);
	else
		dev_info(max8997->dev, "%s: PMIC ID(0x%X)\n",
				__func__, pmic_id);

	/* Mask the individual interrupt sources */
	for (i = 0; i < MAX8997_NUM_IRQ_REGS; i++) {
		max8997->irq_masks_cur[i] = 0xff;
		max8997->irq_masks_cache[i] = 0xff;
	}

	ret = max8997_bulk_write(max8997->i2c, MAX8997_REG_IRQM1,
			MAX8997_NUM_IRQ_PMIC_REGS, pmic_intm);
	if (ret)
		dev_err(max8997->dev, "%s: fail to write PMIC IRQM(%d)\n",
				__func__, ret);

	ret = max8997_bulk_write(max8997->muic, MAX8997_MUIC_REG_INTMASK1,
			MAX8997_NUM_IRQ_MUIC_REGS, muic_intm);
	if (ret)
		dev_err(max8997->dev, "%s: fail to write MUIC IRQM(%d)\n",
				__func__, ret);

	/* Enable flash interrupts to debug flash over current issues
	 * refer MAX8997 I2C Registers: FLASH STATUS control register(0x6D)
	 * 0x01: FLED1_FLT : FLED1 status read back
	 * 0x02: FLED2_FLT : FLED2 status read back
	 */
	ret = max8997_write_reg(max8997->i2c, MAX8997_REG_FLASH_STATUS_MASK,
				0xfc);
	if (ret)
		dev_err(max8997->dev, "%s: fail to write FLASH IRQM(%d)\n",
				__func__, ret);

	fled_dev = device_create(sec_class, NULL, 0, NULL, "sec_fled");
	ret = device_create_file(fled_dev, &dev_attr_fled_state);
	if (ret)
		dev_err(fled_dev, "failed to create device file(fled_state)\n");

	/* register with genirq */
	for (i = 0; i < MAX8997_IRQ_NR; i++) {
		cur_irq = i + max8997->irq_base;
		irq_set_chip_data(cur_irq, max8997);
		irq_set_chip_and_handler(cur_irq, &max8997_irq_chip,
					 handle_level_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	ret = request_threaded_irq(max8997->irq, NULL, max8997_irq_thread,
				   IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				   "max8997-irq", max8997);
	if (ret) {
		dev_err(max8997->dev, "Failed to request IRQ %d: %d\n",
			max8997->irq, ret);
		return ret;
	}

	if (!max8997->ono)
		return 0;

	dev_dbg(max8997->dev, "%s: ono irq request\n", __func__);
	ret = request_threaded_irq(max8997->ono, NULL, max8997_irq_thread,
				   IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
				   IRQF_ONESHOT, "max8997-ono", max8997);
	if (ret) {
		dev_err(max8997->dev, "Failed to request IRQ %d: %d\n",
			max8997->ono, ret);
		return ret;
	}
	enable_irq_wake(max8997->ono);

	return 0;
}

void max8997_irq_exit(struct max8997_dev *max8997)
{
	if (max8997->ono)
		free_irq(max8997->ono, max8997);

	if (max8997->irq)
		free_irq(max8997->irq, max8997);
}
