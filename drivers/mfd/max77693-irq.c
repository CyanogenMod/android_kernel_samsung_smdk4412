/*
 * max77693-irq.c - Interrupt controller support for MAX77693
 *
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * SangYoung Son <hello.son@samsung.com>
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
 *
 * This driver is based on max77693-irq.c
 */

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>

static const u8 max77693_mask_reg[] = {
	[LED_INT] = MAX77693_LED_REG_FLASH_INT_MASK,
	[TOPSYS_INT] = MAX77693_PMIC_REG_TOPSYS_INT_MASK,
	[CHG_INT] = MAX77693_CHG_REG_CHG_INT_MASK,
	[MUIC_INT1] = MAX77693_MUIC_REG_INTMASK1,
	[MUIC_INT2] = MAX77693_MUIC_REG_INTMASK2,
	[MUIC_INT3] = MAX77693_MUIC_REG_INTMASK3,
};

static struct i2c_client *get_i2c(struct max77693_dev *max77693,
				enum max77693_irq_source src)
{
	switch (src) {
	case LED_INT ... CHG_INT:
		return max77693->i2c;
	case MUIC_INT1 ... MUIC_INT3:
		return max77693->muic;
	default:
		return ERR_PTR(-EINVAL);
	}
}

struct max77693_irq_data {
	int mask;
	enum max77693_irq_source group;
};

#define DECLARE_IRQ(idx, _group, _mask)		\
	[(idx)] = { .group = (_group), .mask = (_mask) }
static const struct max77693_irq_data max77693_irqs[] = {
	DECLARE_IRQ(MAX77693_LED_IRQ_FLED2_OPEN,	LED_INT, 1 << 0),
	DECLARE_IRQ(MAX77693_LED_IRQ_FLED2_SHORT,	LED_INT, 1 << 1),
	DECLARE_IRQ(MAX77693_LED_IRQ_FLED1_OPEN,	LED_INT, 1 << 2),
	DECLARE_IRQ(MAX77693_LED_IRQ_FLED1_SHORT,	LED_INT, 1 << 3),
	DECLARE_IRQ(MAX77693_LED_IRQ_MAX_FLASH,	LED_INT, 1 << 4),

	DECLARE_IRQ(MAX77693_TOPSYS_IRQ_T120C_INT,	TOPSYS_INT, 1 << 0),
	DECLARE_IRQ(MAX77693_TOPSYS_IRQ_T140C_INT,	TOPSYS_INT, 1 << 1),
	DECLARE_IRQ(MAX77693_TOPSYS_IRQLOWSYS_INT,	TOPSYS_INT, 1 << 3),

	DECLARE_IRQ(MAX77693_CHG_IRQ_BYP_I,	CHG_INT, 1 << 0),
	DECLARE_IRQ(MAX77693_CHG_IRQ_THM_I,	CHG_INT, 1 << 2),
	DECLARE_IRQ(MAX77693_CHG_IRQ_BAT_I,	CHG_INT, 1 << 3),
	DECLARE_IRQ(MAX77693_CHG_IRQ_CHG_I,	CHG_INT, 1 << 4),
	DECLARE_IRQ(MAX77693_CHG_IRQ_CHGIN_I,	CHG_INT, 1 << 6),

	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT1_ADC,	MUIC_INT1, 1 << 0),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT1_ADCLOW,	MUIC_INT1, 1 << 1),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT1_ADCERR,	MUIC_INT1, 1 << 2),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT1_ADC1K,	MUIC_INT1, 1 << 3),

	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT2_CHGTYP,	MUIC_INT2, 1 << 0),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT2_CHGDETREUN,	MUIC_INT2, 1 << 1),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT2_DCDTMR,	MUIC_INT2, 1 << 2),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT2_DXOVP,	MUIC_INT2, 1 << 3),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT2_VBVOLT,	MUIC_INT2, 1 << 4),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT2_VIDRM,	MUIC_INT2, 1 << 5),

	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT3_EOC,	MUIC_INT3, 1 << 0),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT3_CGMBC,	MUIC_INT3, 1 << 1),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT3_OVP,	MUIC_INT3, 1 << 2),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT3_MBCCHGERR,	MUIC_INT3, 1 << 3),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT3_CHGENABLED,	MUIC_INT3, 1 << 4),
	DECLARE_IRQ(MAX77693_MUIC_IRQ_INT3_BATDET,	MUIC_INT3, 1 << 5),
};

static void max77693_irq_lock(struct irq_data *data)
{
	struct max77693_dev *max77693 = irq_get_chip_data(data->irq);

	mutex_lock(&max77693->irqlock);
}

static void max77693_irq_sync_unlock(struct irq_data *data)
{
	struct max77693_dev *max77693 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < MAX77693_IRQ_GROUP_NR; i++) {
		u8 mask_reg = max77693_mask_reg[i];
		struct i2c_client *i2c = get_i2c(max77693, i);

		if (mask_reg == MAX77693_REG_INVALID ||
				IS_ERR_OR_NULL(i2c))
			continue;
		max77693->irq_masks_cache[i] = max77693->irq_masks_cur[i];

		max77693_write_reg(i2c, max77693_mask_reg[i],
				max77693->irq_masks_cur[i]);
	}

	mutex_unlock(&max77693->irqlock);
}

static const inline struct max77693_irq_data *
irq_to_max77693_irq(struct max77693_dev *max77693, int irq)
{
	return &max77693_irqs[irq - max77693->irq_base];
}

static void max77693_irq_mask(struct irq_data *data)
{
	struct max77693_dev *max77693 = irq_get_chip_data(data->irq);
	const struct max77693_irq_data *irq_data =
	    irq_to_max77693_irq(max77693, data->irq);

	if (irq_data->group >= MAX77693_IRQ_GROUP_NR)
		return;

	if (irq_data->group >= MUIC_INT1 && irq_data->group <= MUIC_INT3)
		max77693->irq_masks_cur[irq_data->group] &= ~irq_data->mask;
	else
		max77693->irq_masks_cur[irq_data->group] |= irq_data->mask;
}

static void max77693_irq_unmask(struct irq_data *data)
{
	struct max77693_dev *max77693 = irq_get_chip_data(data->irq);
	const struct max77693_irq_data *irq_data =
	    irq_to_max77693_irq(max77693, data->irq);

	if (irq_data->group >= MAX77693_IRQ_GROUP_NR)
		return;

	if (irq_data->group >= MUIC_INT1 && irq_data->group <= MUIC_INT3)
		max77693->irq_masks_cur[irq_data->group] |= irq_data->mask;
	else
		max77693->irq_masks_cur[irq_data->group] &= ~irq_data->mask;
}

static struct irq_chip max77693_irq_chip = {
	.name			= "max77693",
	.irq_bus_lock		= max77693_irq_lock,
	.irq_bus_sync_unlock	= max77693_irq_sync_unlock,
	.irq_mask		= max77693_irq_mask,
	.irq_unmask		= max77693_irq_unmask,
};

static irqreturn_t max77693_irq_thread(int irq, void *data)
{
	struct max77693_dev *max77693 = data;
	u8 irq_reg[MAX77693_IRQ_GROUP_NR] = {};
	u8 irq_src;
	int ret;
	int i;
	pr_debug("%s: irq gpio pre-state(0x%02x)\n", __func__,
		gpio_get_value(max77693->irq_gpio));

clear_retry:
	ret = max77693_read_reg(max77693->i2c, MAX77693_PMIC_REG_INTSRC, &irq_src);
	if (ret < 0) {
		dev_err(max77693->dev, "Failed to read interrupt source: %d\n",
				ret);
		return IRQ_NONE;
	}
	pr_info("%s: interrupt source(0x%02x)\n", __func__, irq_src);

	if (irq_src & MAX77693_IRQSRC_CHG) {
		/* CHG_INT */
		ret = max77693_read_reg(max77693->i2c, MAX77693_CHG_REG_CHG_INT,
				&irq_reg[CHG_INT]);
		pr_info("%s: charger interrupt(0x%02x)\n", __func__, irq_reg[CHG_INT]);
	}

	if (irq_src & MAX77693_IRQSRC_TOP) {
		/* TOPSYS_INT */
		ret = max77693_read_reg(max77693->i2c, MAX77693_PMIC_REG_TOPSYS_INT,
				&irq_reg[TOPSYS_INT]);
		pr_info("%s: topsys interrupt(0x%02x)\n", __func__, irq_reg[TOPSYS_INT]);
	}

	if (irq_src & MAX77693_IRQSRC_FLASH) {
		/* LED_INT */
		ret = max77693_read_reg(max77693->i2c, MAX77693_LED_REG_FLASH_INT,
				&irq_reg[LED_INT]);
		pr_info("%s: led interrupt(0x%02x)\n", __func__, irq_reg[LED_INT]);
	}

	if (irq_src & MAX77693_IRQSRC_MUIC) {
		/* MUIC INT1 ~ INT3 */
		max77693_bulk_read(max77693->muic, MAX77693_MUIC_REG_INT1, MAX77693_NUM_IRQ_MUIC_REGS,
				&irq_reg[MUIC_INT1]);
		pr_info("%s: muic interrupt(0x%02x, 0x%02x, 0x%02x)\n", __func__,
			irq_reg[MUIC_INT1], irq_reg[MUIC_INT2], irq_reg[MUIC_INT3]);
	}

	pr_debug("%s: irq gpio post-state(0x%02x)\n", __func__,
		gpio_get_value(max77693->irq_gpio));

	if (gpio_get_value(max77693->irq_gpio) == 0)
		goto clear_retry;

	/* Apply masking */
	for (i = 0; i < MAX77693_IRQ_GROUP_NR; i++) {
		if (i >= MUIC_INT1 && i <= MUIC_INT3)
			irq_reg[i] &= max77693->irq_masks_cur[i];
		else
			irq_reg[i] &= ~max77693->irq_masks_cur[i];
	}

	/* Report */
	for (i = 0; i < MAX77693_IRQ_NR; i++) {
		if (irq_reg[max77693_irqs[i].group] & max77693_irqs[i].mask)
			handle_nested_irq(max77693->irq_base + i);
	}

	return IRQ_HANDLED;
}

int max77693_irq_resume(struct max77693_dev *max77693)
{
	int ret = 0;
	if (max77693->irq && max77693->irq_base)
		ret = max77693_irq_thread(max77693->irq_base, max77693);

	dev_info(max77693->dev, "%s: irq_resume ret=%d", __func__, ret);

	return ret >= 0 ? 0 : ret;
}

int max77693_irq_init(struct max77693_dev *max77693)
{
	int i;
	int cur_irq;
	int ret;
	u8 i2c_data;

	if (!max77693->irq_gpio) {
		dev_warn(max77693->dev, "No interrupt specified.\n");
		max77693->irq_base = 0;
		return 0;
	}

	if (!max77693->irq_base) {
		dev_err(max77693->dev, "No interrupt base specified.\n");
		return 0;
	}

	mutex_init(&max77693->irqlock);

	max77693->irq = gpio_to_irq(max77693->irq_gpio);
	ret = gpio_request(max77693->irq_gpio, "if_pmic_irq");
	if (ret) {
		dev_err(max77693->dev, "%s: failed requesting gpio %d\n",
			__func__, max77693->irq_gpio);
		return ret;
	}
	gpio_direction_input(max77693->irq_gpio);
	gpio_free(max77693->irq_gpio);

	/* Mask individual interrupt sources */
	for (i = 0; i < MAX77693_IRQ_GROUP_NR; i++) {
		struct i2c_client *i2c;
		/* MUIC IRQ  0:MASK 1:NOT MASK */
		/* Other IRQ 1:MASK 0:NOT MASK */
		if (i >= MUIC_INT1 && i <= MUIC_INT3) {
			max77693->irq_masks_cur[i] = 0x00;
			max77693->irq_masks_cache[i] = 0x00;
		} else {
			max77693->irq_masks_cur[i] = 0xff;
			max77693->irq_masks_cache[i] = 0xff;
		}
		i2c = get_i2c(max77693, i);

		if (IS_ERR_OR_NULL(i2c))
			continue;
		if (max77693_mask_reg[i] == MAX77693_REG_INVALID)
			continue;
		if (i >= MUIC_INT1 && i <= MUIC_INT3)
			max77693_write_reg(i2c, max77693_mask_reg[i], 0x00);
		else
			max77693_write_reg(i2c, max77693_mask_reg[i], 0xff);
	}

	/* Register with genirq */
	for (i = 0; i < MAX77693_IRQ_NR; i++) {
		cur_irq = i + max77693->irq_base;
		irq_set_chip_data(cur_irq, max77693);
		irq_set_chip_and_handler(cur_irq, &max77693_irq_chip,
					 handle_edge_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	/* Unmask max77693 interrupt */
	ret = max77693_read_reg(max77693->i2c, MAX77693_PMIC_REG_INTSRC_MASK,
			  &i2c_data);
	if (ret) {
		dev_err(max77693->dev, "%s: fail to read muic reg\n", __func__);
		return ret;
	}

	i2c_data &= ~(MAX77693_IRQSRC_CHG);	/* Unmask charger interrupt */
	i2c_data &= ~(MAX77693_IRQSRC_MUIC);	/* Unmask muic interrupt */
	max77693_write_reg(max77693->i2c, MAX77693_PMIC_REG_INTSRC_MASK,
			   i2c_data);

	ret = request_threaded_irq(max77693->irq, NULL, max77693_irq_thread,
				   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				   "max77693-irq", max77693);

	if (ret) {
		dev_err(max77693->dev, "Failed to request IRQ %d: %d\n",
			max77693->irq, ret);
		return ret;
	}

	return 0;
}

void max77693_irq_exit(struct max77693_dev *max77693)
{
	if (max77693->irq)
		free_irq(max77693->irq, max77693);
}
