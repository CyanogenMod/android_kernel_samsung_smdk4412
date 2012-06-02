/* linux/drivers/video/s6e39a0x.c
 *
 * MIPI-DSI based s6e39a0x AMOLED lcd 5.3 inch panel driver.
 *
 * Inki Dae, <inki.dae@samsung.com>
 * Donghwa Lee, <dh09.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/lcd.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>

#include <video/mipi_display.h>

#include <plat/mipi_dsim2.h>
#include <plat/regs-dsim2.h>
#include <plat/gpio-cfg.h>

#include "s6e39a0x02_gamma.h"

#define LDI_MTP_LENGTH		24
#define DSIM_PM_STABLE_TIME	(10)
#define MIN_BRIGHTNESS		(0)
#define MAX_BRIGHTNESS		(10)

#define VER_142			(0x8e)	/* MACH_SLP_PQ */
#define VER_42			(0x2a)	/* MACH_SLP_PQ_LTE */

#define POWER_IS_ON(pwr)	((pwr) == FB_BLANK_UNBLANK)
#define POWER_IS_OFF(pwr)	((pwr) == FB_BLANK_POWERDOWN)
#define POWER_IS_NRM(pwr)	((pwr) == FB_BLANK_NORMAL)

#define lcd_to_master(a)	(a->dsim_dev->master)
#define lcd_to_master_ops(a)	((lcd_to_master(a))->master_ops)

enum {
	DSIM_NONE_STATE = 0,
	DSIM_RESUME_COMPLETE = 1,
	DSIM_FRAME_DONE = 2,
};

struct s6e39a0x02 {
	struct device	*dev;
	unsigned int			id;
	unsigned int			ver;
	unsigned int			power;
	unsigned int			current_brightness;
	unsigned int			updated;
	unsigned int			gamma;
	unsigned int			resume_complete;
	unsigned int			acl_enable;
	unsigned int			cur_acl;

	struct lcd_device		*ld;
	struct backlight_device		*bd;

	struct mipi_dsim_lcd_device	*dsim_dev;
	struct lcd_platform_data	*ddi_pd;

	struct mutex			lock;
	struct regulator		*reg_vci;
	bool				enabled;
};

static struct mipi_dsim_device *dsim;

static void s6e39a0x02_regulator_enable(struct s6e39a0x02 *lcd)
{
	mutex_lock(&lcd->lock);
	if (!lcd->enabled) {
		if (lcd->reg_vci)
			regulator_enable(lcd->reg_vci);

			lcd->enabled = true;
	}
	mutex_unlock(&lcd->lock);
}

static void s6e39a0x02_regulator_disable(struct s6e39a0x02 *lcd)
{
	mutex_lock(&lcd->lock);
	if (lcd->enabled) {
		if (lcd->reg_vci)
			regulator_disable(lcd->reg_vci);

		lcd->enabled = false;
	}
	mutex_unlock(&lcd->lock);
}

static void s6e39a0x02_sleep_in(struct s6e39a0x02 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE, 0x10, 0x00);
}

static void s6e39a0x02_sleep_out(struct s6e39a0x02 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE, 0x11, 0x00);
}

static void s6e39a0x02_display_on(struct s6e39a0x02 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE, 0x29, 0x00);
}

static void s6e39a0x02_acl_on(struct s6e39a0x02 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE, 0xc0, 0x01);
}

static void s6e39a0x02_acl_off(struct s6e39a0x02 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE, 0xc0, 0x00);
}

static void s6e39a0x02_acl_ctrl_set(struct s6e39a0x02 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	/* Full white 50% reducing setting */
	const unsigned char cutoff_50[] = {
		0xc1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x02, 0xcf,
		0x00, 0x00, 0x04, 0xff,	0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x08, 0x0f, 0x16, 0x1d, 0x24, 0x2a, 0x31, 0x38,
		0x3f, 0x46
	};
	/* Full white 45% reducing setting */
	const unsigned char cutoff_45[] = {
		0xc1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x02, 0xcf,
		0x00, 0x00, 0x04, 0xff,	0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x07, 0x0d, 0x13, 0x19, 0x1f, 0x25, 0x2b, 0x31,
		0x37, 0x3d
	};
	/* Full white 40% reducing setting */
	const unsigned char cutoff_40[] = {
		0xc1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x02, 0xcf,
		0x00, 0x00, 0x04, 0xff,	0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x06, 0x0c, 0x11, 0x16, 0x1c, 0x21, 0x26, 0x2b,
		0x31, 0x36
	};

	if (lcd->acl_enable) {
		if (lcd->cur_acl == 0) {
			if (lcd->gamma == 0 || lcd->gamma == 1) {
				s6e39a0x02_acl_off(lcd);
				dev_dbg(&lcd->ld->dev,
					"cur_acl=%d\n", lcd->cur_acl);
			} else
				s6e39a0x02_acl_on(lcd);
		}
		switch (lcd->gamma) {
		case 0: /* 30cd */
			s6e39a0x02_acl_off(lcd);
			lcd->cur_acl = 0;
			break;
		case 1 ... 3: /* 50cd ~ 90cd */
			ops->cmd_write(lcd_to_master(lcd),
				MIPI_DSI_DCS_LONG_WRITE,
				(unsigned int)cutoff_40,
				ARRAY_SIZE(cutoff_40));
			lcd->cur_acl = 40;
			break;
		case 4 ... 7: /* 120cd ~ 210cd */
			ops->cmd_write(lcd_to_master(lcd),
				MIPI_DSI_DCS_LONG_WRITE,
				(unsigned int)cutoff_45,
				ARRAY_SIZE(cutoff_45));
			lcd->cur_acl = 45;
			break;
		case 8 ... 10: /* 220cd ~ 300cd */
			ops->cmd_write(lcd_to_master(lcd),
				MIPI_DSI_DCS_LONG_WRITE,
				(unsigned int)cutoff_50,
				ARRAY_SIZE(cutoff_50));
			lcd->cur_acl = 50;
			break;
		default:
			break;
		}
	} else {
		s6e39a0x02_acl_off(lcd);
		lcd->cur_acl = 0;
		dev_dbg(&lcd->ld->dev, "cur_acl = %d\n", lcd->cur_acl);
	}
}

static void s6e39a0x02_etc_cond1(struct s6e39a0x02 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);

	unsigned char data_to_send_1[3] = {
		0xf0, 0x5a, 0x5a
	};

	unsigned char data_to_send_2[3] = {
		0xf1, 0x5a, 0x5a
	};

	unsigned char data_to_send_3[3] = {
		0xfc, 0x5a, 0x5a
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_1, sizeof(data_to_send_1));

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_2, sizeof(data_to_send_2));

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_3, sizeof(data_to_send_3));
}

static void s6e39a0x02_gamma_cond(struct s6e39a0x02 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);

	unsigned char data_to_send[26] = {
		0xfa, 0x02, 0x10, 0x10, 0x10, 0xf0, 0x8c, 0xed, 0xd5,
		0xca, 0xd8, 0xdc, 0xdb, 0xdc, 0xbb, 0xbd, 0xb7, 0xcb,
		0xcd, 0xc5, 0x00, 0x9c, 0x00, 0x7a, 0x00, 0xb2
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send, sizeof(data_to_send));

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xfa, 0x03);
}

static void s6e39a0x02_panel_cond(struct s6e39a0x02 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);

	unsigned char data_to_send_1[14] = {
		0xf8, 0x27, 0x27, 0x08, 0x08, 0x4e, 0xaa,
		0x5e, 0x8a, 0x10, 0x3f, 0x10, 0x10, 0x00
	};

	unsigned char data_to_send_2[6] = {
		0xb3, 0x63, 0x02, 0xc3, 0x32, 0xff
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_1, sizeof(data_to_send_1));

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xf7, 0x03);

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_2, sizeof(data_to_send_2));
}

static void s6e39a0x02_etc_cond2(struct s6e39a0x02 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);

	unsigned char data_to_send_1[4] = {
		0xf6, 0x00, 0x84, 0x09
	};

	unsigned char data_to_send_2[4] = {
		0xd5, 0xa4, 0x7e, 0x20
	};

	unsigned char data_to_send_3[4] = {
		0xb1, 0x01, 0x00, 0x16
	};

	unsigned char data_to_send_4[5] = {
		0xb2, 0x15, 0x15, 0x15, 0x15
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_1, sizeof(data_to_send_1));

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xb0, 0x09);

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xd5, 0x64);

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xb0, 0x0b);

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_2, sizeof(data_to_send_2));

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xb0, 0x08);

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xfd, 0xf8);

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xb0, 0x01);

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xf2, 0x07);

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xb0, 0x04);

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xf2, 0x4d);

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_3, sizeof(data_to_send_3));

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_4, sizeof(data_to_send_4));

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE, 0x11, 0x00);
}

static void s6e39a0x02_memory_window_1(struct s6e39a0x02 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);

	unsigned char data_to_send_1[5] = {
		0x2a, 0x00, 0x00, 0x02, 0x57
	};

	unsigned char data_to_send_2[5] = {
		0x2b, 0x00, 0x00, 0x03, 0xff
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_1, sizeof(data_to_send_1));

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_2, sizeof(data_to_send_2));

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE, 0x2C, 0x00);
}

static void s6e39a0x02_memory_window_2(struct s6e39a0x02 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);

	unsigned char data_to_send_1[5] = {
		0x2a, 0x00, 0x1e, 0x02, 0x39
	};

	unsigned char data_to_send_2[5] = {
		0x2b, 0x00, 0x00, 0x03, 0xbf
	};

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE, 0x35, 0x00);

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_1, sizeof(data_to_send_1));

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_2, sizeof(data_to_send_2));

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xd1, 0x8a);
}

static int s6e39a0x02_panel_init(struct s6e39a0x02 *lcd)
{
	mdelay(10);

	s6e39a0x02_etc_cond1(lcd);
	s6e39a0x02_gamma_cond(lcd);
	s6e39a0x02_panel_cond(lcd);
	s6e39a0x02_etc_cond2(lcd);

	mdelay(120);

	s6e39a0x02_memory_window_1(lcd);

	mdelay(20);

	s6e39a0x02_memory_window_2(lcd);

	return 0;
}

static int s6e39a0x02_update_gamma_ctrl(struct s6e39a0x02 *lcd, int gamma)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);


	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)s6e39a0x02_22_gamma_table[gamma],
			GAMMA_TABLE_COUNT);

	/* update gamma table. */
	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_SHORT_WRITE_PARAM,
			0xfA, 0x03);

	lcd->gamma = gamma;

	return 0;
}

static int s6e39a0x02_gamma_ctrl(struct s6e39a0x02 *lcd, int gamma)
{
	s6e39a0x02_update_gamma_ctrl(lcd, gamma);

	return 0;
}

static int s6e39a0x02_early_set_power(struct lcd_device *ld, int power)
{
	struct s6e39a0x02 *lcd = lcd_get_data(ld);
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	int ret = 0;

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
			power != FB_BLANK_NORMAL) {
		dev_err(lcd->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	if (lcd->power == power) {
		dev_err(lcd->dev, "power mode is same as previous one.\n");
		return -EINVAL;
	}

	if (ops->set_early_blank_mode) {
		/* LCD power off */
		if ((POWER_IS_OFF(power) && POWER_IS_ON(lcd->power))
			|| (POWER_IS_ON(lcd->power) && POWER_IS_NRM(power))) {
			ret = ops->set_early_blank_mode(lcd_to_master(lcd),
							power);
			if (!ret && lcd->power != power)
				lcd->power = power;
		}
	}

	return ret;
}

static int s6e39a0x02_set_power(struct lcd_device *ld, int power)
{
	struct s6e39a0x02 *lcd = lcd_get_data(ld);
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	int ret = 0;

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
			power != FB_BLANK_NORMAL) {
		dev_err(lcd->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	if (lcd->power == power) {
		dev_err(lcd->dev, "power mode is same as previous one.\n");
		return -EINVAL;
	}

	if (ops->set_blank_mode) {
		ret = ops->set_blank_mode(lcd_to_master(lcd), power);
		if (!ret && lcd->power != power)
			lcd->power = power;
	}

	return ret;
}

static int s6e39a0x02_get_power(struct lcd_device *ld)
{
	struct s6e39a0x02 *lcd = lcd_get_data(ld);

	return lcd->power;
}

static int s6e39a0x02_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}

static int s6e39a0x02_set_brightness(struct backlight_device *bd)
{
	int ret = 0, brightness = bd->props.brightness;
	struct s6e39a0x02 *lcd = bl_get_data(bd);

	if (brightness < MIN_BRIGHTNESS ||
		brightness > bd->props.max_brightness) {
		dev_err(lcd->dev, "lcd brightness should be %d to %d.\n",
			MIN_BRIGHTNESS, MAX_BRIGHTNESS);
		return -EINVAL;
	}

	ret = s6e39a0x02_gamma_ctrl(lcd, brightness);
	if (ret) {
		dev_err(&bd->dev, "lcd brightness setting failed.\n");
		return -EIO;
	}

	return ret;
}

static struct lcd_ops s6e39a0x02_lcd_ops = {
	.early_set_power = s6e39a0x02_early_set_power,
	.set_power = s6e39a0x02_set_power,
	.get_power = s6e39a0x02_get_power,
};

static const struct backlight_ops s6e39a0x02_backlight_ops = {
	.get_brightness = s6e39a0x02_get_brightness,
	.update_status = s6e39a0x02_set_brightness,
};

static void s6e39a0x02_power_on(struct mipi_dsim_lcd_device *dsim_dev,
				unsigned int enable)
{
	struct s6e39a0x02 *lcd = dev_get_drvdata(&dsim_dev->dev);

	if (enable) {
		mdelay(lcd->ddi_pd->power_on_delay);

		/* lcd power on */
		s6e39a0x02_regulator_enable(lcd);

		mdelay(lcd->ddi_pd->reset_delay);

		/* lcd reset */
		if (lcd->ddi_pd->reset)
			lcd->ddi_pd->reset(lcd->ld);

		mdelay(5);
	} else
		s6e39a0x02_regulator_disable(lcd);

}

static void s6e39a0x02_set_sequence(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct s6e39a0x02 *lcd = dev_get_drvdata(&dsim_dev->dev);

	s6e39a0x02_panel_init(lcd);
	s6e39a0x02_display_on(lcd);

	mdelay(50);
}

static ssize_t acl_control_show(struct device *dev, struct
	device_attribute * attr, char *buf)
{
	struct s6e39a0x02 *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->acl_enable);
	strcpy(buf, temp);

	return strlen(buf);
}

static ssize_t acl_control_store(struct device *dev, struct
	device_attribute * attr, const char *buf, size_t size)
{
	struct s6e39a0x02 *lcd = dev_get_drvdata(dev);
	unsigned int value;
	int rc;

	rc = strict_strtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;

	if (lcd->acl_enable != value) {
		dev_info(dev, "acl control changed from %d to %d\n",
						lcd->acl_enable, value);
		lcd->acl_enable = value;
		s6e39a0x02_acl_ctrl_set(lcd);
	}
	return size;
}

static irqreturn_t te_interrupt(int irq, void *dev_id)
{
	struct device *dev = NULL;
	u32 intsrc = 0;
	static bool suspended;

	dev = (struct device *)dev_id;

	intsrc = readl_relaxed(dsim->reg_base + S5P_DSIM_INTSRC);

	if (intsrc & (1 << 24)) {
		int val;

		val = __raw_readl(dsim->reg_base + S5P_DSIM_INTSRC);
		val |= (1 << 24);
		__raw_writel(val, dsim->reg_base + S5P_DSIM_INTSRC);
		dsim->master_ops->trigger(NULL);
	}

	if (suspended == true && dsim->suspended == false) {
		dev_info(dev, "te_interrupt: dsim->suspended is false.\n");
		dsim->master_ops->trigger(NULL);
	}

	suspended = dsim->suspended;

	return IRQ_HANDLED;
}

static DEVICE_ATTR(acl_control, 0664, acl_control_show, acl_control_store);

static int s6e39a0x02_probe(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct s6e39a0x02 *lcd;
	int ret;

	dsim = dsim_dev->master;

	lcd = kzalloc(sizeof(struct s6e39a0x02), GFP_KERNEL);
	if (!lcd) {
		dev_err(&dsim_dev->dev, "failed to allocate s6e39a0x02 structure.\n");
		return -ENOMEM;
	}

	lcd->dsim_dev = dsim_dev;
	lcd->ddi_pd = (struct lcd_platform_data *)dsim_dev->platform_data;
	lcd->dev = &dsim_dev->dev;

	mutex_init(&lcd->lock);

	lcd->reg_vci = regulator_get(lcd->dev, "VCI");
	if (IS_ERR(lcd->reg_vci)) {
		ret = PTR_ERR(lcd->reg_vci);
		dev_err(lcd->dev, "failed to get %s regulator (%d)\n",
				"VCI", ret);
		lcd->reg_vci = NULL;
	}

	lcd->ld = lcd_device_register("s6e39a0x02", lcd->dev, lcd,
			&s6e39a0x02_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		dev_err(lcd->dev, "failed to register lcd ops.\n");
		ret = PTR_ERR(lcd->ld);
		goto err_regulator;
	}

	lcd->bd = backlight_device_register("s6e39a0x02-bl", lcd->dev, lcd,
			&s6e39a0x02_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		dev_err(lcd->dev, "failed to register backlight ops.\n");
		ret = PTR_ERR(lcd->bd);
		goto err_unregister_lcd;
	}

	lcd->bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd->bd->props.brightness = MAX_BRIGHTNESS;

	lcd->acl_enable = 1;
	lcd->cur_acl = 0;

	ret = device_create_file(&lcd->ld->dev, &dev_attr_acl_control);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries\n");

	dev_set_drvdata(&dsim_dev->dev, lcd);

	gpio_request(EXYNOS4_GPF0(2), "TE_INT");
	s5p_register_gpio_interrupt(EXYNOS4_GPF0(2));

	s3c_gpio_cfgpin(EXYNOS4_GPF0(2), S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(EXYNOS4_GPF0(2), S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(EXYNOS4_GPF0(2), S5P_GPIO_DRVSTR_LV1);
	irq_set_irq_type(gpio_to_irq(EXYNOS4_GPF0(2)), IRQ_TYPE_EDGE_RISING);

	ret = request_irq(gpio_to_irq(EXYNOS4_GPF0(2)), te_interrupt,
				IRQF_TRIGGER_RISING | IRQF_SHARED,
				"te_signal", lcd->dev);

	if (ret < 0) {
		dev_err(lcd->dev, "%s: request_irq failed. %d\n",
			__func__, ret);
		goto err_te_irq;
	}

	mdelay(100);

	s6e39a0x02_regulator_enable(lcd);

	lcd->power = FB_BLANK_UNBLANK;

	if (!lcd->ddi_pd->lcd_enabled) {
		s6e39a0x02_panel_init(lcd);
		s6e39a0x02_display_on(lcd);
	}

	dev_info(lcd->dev, "probed s6e39a0x02 panel driver(%s).\n",
			dev_name(&lcd->ld->dev));

	return 0;

err_unregister_lcd:
err_te_irq:
	lcd_device_unregister(lcd->ld);

err_regulator:
	regulator_put(lcd->reg_vci);

	kfree(lcd);

	return ret;
}

static void s6e39a0x02_remove(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct s6e39a0x02 *lcd = dev_get_drvdata(&dsim_dev->dev);

	backlight_device_unregister(lcd->bd);

	lcd_device_unregister(lcd->ld);

	regulator_put(lcd->reg_vci);

	kfree(lcd);
}

#ifdef CONFIG_PM
static int s6e39a0x02_suspend(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct s6e39a0x02 *lcd = dev_get_drvdata(&dsim_dev->dev);

	s6e39a0x02_sleep_in(lcd);
	mdelay(lcd->ddi_pd->power_off_delay);

	return 0;
}

static int s6e39a0x02_resume(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct s6e39a0x02 *lcd = dev_get_drvdata(&dsim_dev->dev);

	s6e39a0x02_sleep_out(lcd);
	mdelay(lcd->ddi_pd->power_on_delay);

	return 0;
}
#else
#define s6e39a0x02_suspend		NULL
#define s6e39a0x02_resume		NULL
#endif

static struct mipi_dsim_lcd_driver s6e39a0x02_dsim_ddi_driver = {
	.name = "s6e39a0x02",
	.id = -1,

	.power_on = s6e39a0x02_power_on,
	.set_sequence = s6e39a0x02_set_sequence,
	.probe = s6e39a0x02_probe,
	.remove = s6e39a0x02_remove,
	.suspend = s6e39a0x02_suspend,
	.resume = s6e39a0x02_resume,
};

static int s6e39a0x02_init(void)
{
	s5p_mipi_dsi_register_lcd_driver(&s6e39a0x02_dsim_ddi_driver);

	return 0;
}

static void s6e39a0x02_exit(void)
{
	return;
}

module_init(s6e39a0x02_init);
module_exit(s6e39a0x02_exit);

MODULE_AUTHOR("Donghwa Lee <dh09.lee@samsung.com>");
MODULE_DESCRIPTION("MIPI-DSI based s6e39a0x02 AMOLED LCD Panel Driver");
MODULE_LICENSE("GPL");
