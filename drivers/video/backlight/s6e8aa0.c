/* linux/drivers/video/s6e8aa0.c
 *
 * MIPI-DSI based s6e8aa0 AMOLED lcd 5.3 inch panel driver.
 *
 * Inki Dae, <inki.dae@samsung.com>
 * Donghwa Lee, <dh09.lee@samsung.com>
 * Joongmock Shin <jmock.shin@samsung.com>
 * Eunchul Kim <chulspro.kim@samsung.com>
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
#include <linux/lcd-property.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>

#include <video/mipi_display.h>

#include <plat/cpu.h>
#include <plat/mipi_dsim2.h>

#if defined(CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ) || defined(CONFIG_DISPFREQ_OPP)
#include <linux/devfreq/exynos4_display.h>
#endif

#include "s6e8aa0_gamma.h"
#ifdef CONFIG_BACKLIGHT_SMART_DIMMING
#include "smart_dimming.h"
#endif

#define LDI_FW_PATH "s6e8aa0/reg_%s.bin"
#define MAX_STR				255
#define LDI_MTP_LENGTH		24
#define MAX_READ_LENGTH		64
#define DSIM_PM_STABLE_TIME	(10)
#define MIN_BRIGHTNESS		(0)
#define MAX_BRIGHTNESS		(24)

/* 1 */
#define PANELCTL_SS_MASK	(1 << 5)
#define PANELCTL_SS_1_800	(0 << 5)
#define PANELCTL_SS_800_1	(1 << 5)
#define PANELCTL_GTCON_MASK	(7 << 2)
#define PANELCTL_GTCON_110	(6 << 2)
#define PANELCTL_GTCON_111	(7 << 2)
/* LTPS */
/* 30 */
#define PANELCTL_CLK1_CON_MASK	(7 << 3)
#define PANELCTL_CLK1_000	(0 << 3)
#define PANELCTL_CLK1_001	(1 << 3)
#define PANELCTL_CLK2_CON_MASK	(7 << 0)
#define PANELCTL_CLK2_000	(0 << 0)
#define PANELCTL_CLK2_001	(1 << 0)
/* 31 */
#define PANELCTL_INT1_CON_MASK	(7 << 3)
#define PANELCTL_INT1_000	(0 << 3)
#define PANELCTL_INT1_001	(1 << 3)
#define PANELCTL_INT2_CON_MASK	(7 << 0)
#define PANELCTL_INT2_000	(0 << 0)
#define PANELCTL_INT2_001	(1 << 0)
/* 32 */
#define PANELCTL_BICTL_CON_MASK	(7 << 3)
#define PANELCTL_BICTL_000	(0 << 3)
#define PANELCTL_BICTL_001	(1 << 3)
#define PANELCTL_BICTLB_CON_MASK	(7 << 0)
#define PANELCTL_BICTLB_000	(0 << 0)
#define PANELCTL_BICTLB_001	(1 << 0)
/* 36 */
#define PANELCTL_EM_CLK1_CON_MASK	(7 << 3)
#define PANELCTL_EM_CLK1_110	(6 << 3)
#define PANELCTL_EM_CLK1_111	(7 << 3)
#define PANELCTL_EM_CLK1B_CON_MASK	(7 << 0)
#define PANELCTL_EM_CLK1B_110	(6 << 0)
#define PANELCTL_EM_CLK1B_111	(7 << 0)
/* 37 */
#define PANELCTL_EM_CLK2_CON_MASK	(7 << 3)
#define PANELCTL_EM_CLK2_110	(6 << 3)
#define PANELCTL_EM_CLK2_111	(7 << 3)
#define PANELCTL_EM_CLK2B_CON_MASK	(7 << 0)
#define PANELCTL_EM_CLK2B_110	(6 << 0)
#define PANELCTL_EM_CLK2B_111	(7 << 0)
/* 38 */
#define PANELCTL_EM_INT1_CON_MASK	(7 << 3)
#define PANELCTL_EM_INT1_000	(0 << 3)
#define PANELCTL_EM_INT1_001	(1 << 3)
#define PANELCTL_EM_INT2_CON_MASK	(7 << 0)
#define PANELCTL_EM_INT2_000	(0 << 0)
#define PANELCTL_EM_INT2_001	(1 << 0)

#define VER_142			(0x8e)	/* MACH_SLP_PQ */
#define VER_174			(0xae)	/* MACH_SLP_PQ Dali */
#define VER_42			(0x2a)	/* MACH_SLP_PQ_LTE */
#define VER_32			(0x20)	/* MACH_SLP_PQ M0 B-Type */
#define VER_96			(0x60)	/* MACH_SLP_PQ Galaxy S3 */
#define VER_210			(0xd2)	/* MACH_SLP_PQ M0 A-Type */

#define AID_DISABLE		(0x4)
#define AID_1			(0x5)
#define AID_2			(0x6)
#define AID_3			(0x7)

#define POWER_IS_ON(pwr)	((pwr) == FB_BLANK_UNBLANK)
#define POWER_IS_OFF(pwr)	((pwr) == FB_BLANK_POWERDOWN)
#define POWER_IS_NRM(pwr)	((pwr) == FB_BLANK_NORMAL)

#define lcd_to_master(a)	(a->dsim_dev->master)
#define lcd_to_master_ops(a)	((lcd_to_master(a))->master_ops)

struct panel_model {
	int ver;
	char *name;
};

struct s6e8aa0 {
	struct device	*dev;
	struct lcd_device	*ld;
	struct backlight_device	*bd;
	struct mipi_dsim_lcd_device	*dsim_dev;
	struct lcd_platform_data	*ddi_pd;
	struct lcd_property	*property;

	struct regulator	*reg_vdd3;
	struct regulator	*reg_vci;

#if defined(CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ) || defined(CONFIG_DISPFREQ_OPP)
	struct notifier_block	nb_disp;
#endif
	struct mutex	lock;

	unsigned int	id;
	unsigned int	aid;
	unsigned int	ver;
	unsigned int	power;
	unsigned int	acl_enable;
	unsigned int	cur_acl;
	unsigned int	cur_addr;

	const struct panel_model	*model;
	unsigned int	model_count;

#ifdef CONFIG_BACKLIGHT_SMART_DIMMING
	unsigned int	support_elvss;
	struct str_smart_dim	smart_dim;
#endif
};

static void s6e8aa0_regulator_ctl(struct s6e8aa0 *lcd, bool enable)
{
	dev_dbg(lcd->dev, "%s:enable[%d]\n", __func__, enable);

	mutex_lock(&lcd->lock);

	if (enable) {
		if (lcd->reg_vdd3)
			regulator_enable(lcd->reg_vdd3);

		if (lcd->reg_vci)
			regulator_enable(lcd->reg_vci);
	} else {
		if (lcd->reg_vci)
			regulator_disable(lcd->reg_vci);

		if (lcd->reg_vdd3)
			regulator_disable(lcd->reg_vdd3);
	}

	mutex_unlock(&lcd->lock);
}

static void s6e8aa0_apply_level_1_key(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	const unsigned char data_to_send[] = {
		0xf0, 0x5a, 0x5a
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static unsigned char s6e8aa0_apply_aid_panel_cond(unsigned int aid)
{
	unsigned char ret;

	switch (aid) {
	case AID_1:
		ret = 0x60;
		break;
	case AID_2:
		ret = 0x80;
		break;
	case AID_3:
		ret = 0xA0;
		break;
	default:
		ret = 0x04;
		break;
	}

	return ret;
}

static void s6e8aa0_panel_cond(struct s6e8aa0 *lcd, int high_freq)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	unsigned char data_to_send_v42[] = {
		0xf8, 0x01, 0x34, 0x00, 0x00, 0x00, 0x95, 0x00, 0x3c,
		0x7d, 0x08, 0x27, 0x00, 0x00, 0x10, 0x00, 0x00, 0x20,
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x08, 0x08,
		0x23, 0x63, 0xc0, 0xc1, 0x01, 0x81, 0xc1, 0x00, 0xc8,
		0xc1, 0xd3, 0x01
	};
	/* same with v174(0xae) panel */
	unsigned char data_to_send_v142[] = {
		0xf8, 0x3d, 0x35, 0x00, 0x00, 0x00, 0x93, 0x00, 0x3c,
		0x7d, 0x08, 0x27, 0x7d, 0x3f, 0x00, 0x00, 0x00, 0x20,
		0x04, 0x08, 0x6e, 0x00, 0x00, 0x00, 0x02, 0x08, 0x08,
		0x23, 0x23, 0xc0, 0xc8, 0x08, 0x48, 0xc1, 0x00, 0xc1,
		0xff, 0xff, 0xc8
	};
	unsigned char data_to_send_v32[] = {
		0xf8, 0x19, 0x35, 0x00, 0x00, 0x00, 0x94, 0x00, 0x3c,
		0x7d, 0x10, 0x27, 0x08, 0x6e, 0x00, 0x00, 0x00, 0x00,
		0x04, 0x08, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08,
		0x23, 0x6e, 0xc0, 0xc1, 0x01, 0x81, 0xc1, 0x00, 0xc3,
		0xf6, 0xf6, 0xc1
	};
	unsigned char data_to_send_v96[] = {
		0xf8, 0x19, 0x35, 0x00, 0x00, 0x00, 0x94, 0x00, 0x3c,
		0x7d, 0x10, 0x27, 0x08, 0x6e, 0x00, 0x00, 0x00, 0x00,
		0x04, 0x08, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08,
		0x23, 0x37, 0xc0, 0xc1, 0x01, 0x81, 0xc1, 0x00, 0xc3,
		0xf6, 0xf6, 0xc1
	};
	unsigned char data_to_send_v210[] = {
		0xf8, 0x3d, 0x32, 0x00, 0x00, 0x00, 0x8d, 0x00, 0x39,
		0x78, 0x08, 0x26, 0x78, 0x3c, 0x00, 0x00, 0x00, 0x20,
		0x04, 0x08, 0x69, 0x00, 0x00, 0x00, 0x02, 0x07, 0x07,
		0x21, 0x21, 0xc0, 0xc8, 0x08, 0x48, 0xc1, 0x00, 0xc1,
		0xff, 0xff, 0xc8
	};
	unsigned char *data_to_send;
	unsigned int size;
	struct lcd_property	*property = lcd->property;
	unsigned char cfg;

	if (lcd->ver == VER_42) {
		data_to_send = data_to_send_v42;
		size = ARRAY_SIZE(data_to_send_v42);
	} else if (lcd->ver == VER_142) {
		data_to_send_v142[18] = s6e8aa0_apply_aid_panel_cond(lcd->aid);
		data_to_send = data_to_send_v142;
		if (property) {
			if (property->flip & LCD_PROPERTY_FLIP_VERTICAL) {
				/* GTCON */
				cfg = data_to_send[1];
				cfg &= ~(PANELCTL_GTCON_MASK);
				cfg |= (PANELCTL_GTCON_110);
				data_to_send[1] = cfg;
			}

			if (property->flip & LCD_PROPERTY_FLIP_HORIZONTAL) {
				/* SS */
				cfg = data_to_send[1];
				cfg &= ~(PANELCTL_SS_MASK);
				cfg |= (PANELCTL_SS_1_800);
				data_to_send[1] = cfg;
			}

			if (property->flip & (LCD_PROPERTY_FLIP_VERTICAL |
			LCD_PROPERTY_FLIP_HORIZONTAL)) {
				/* CLK1,2_CON */
				cfg = data_to_send[30];
				cfg &= ~(PANELCTL_CLK1_CON_MASK |
					PANELCTL_CLK2_CON_MASK);
				cfg |= (PANELCTL_CLK1_000 | PANELCTL_CLK2_001);
				data_to_send[30] = cfg;

				/* INT1,2_CON */
				cfg = data_to_send[31];
				cfg &= ~(PANELCTL_INT1_CON_MASK |
					PANELCTL_INT2_CON_MASK);
				cfg |= (PANELCTL_INT1_000 | PANELCTL_INT2_001);
				data_to_send[31] = cfg;

				/* BICTL,B_CON */
				cfg = data_to_send[32];
				cfg &= ~(PANELCTL_BICTL_CON_MASK |
					PANELCTL_BICTLB_CON_MASK);
				cfg |= (PANELCTL_BICTL_000 |
					PANELCTL_BICTLB_001);
				data_to_send[32] = cfg;

				/* EM_CLK1,1B_CON */
				cfg = data_to_send[36];
				cfg &= ~(PANELCTL_EM_CLK1_CON_MASK |
					PANELCTL_EM_CLK1B_CON_MASK);
				cfg |= (PANELCTL_EM_CLK1_110 |
					PANELCTL_EM_CLK1B_110);
				data_to_send[36] = cfg;

				/* EM_CLK2,2B_CON */
				cfg = data_to_send[37];
				cfg &= ~(PANELCTL_EM_CLK2_CON_MASK |
					PANELCTL_EM_CLK2B_CON_MASK);
				cfg |= (PANELCTL_EM_CLK2_110 |
					PANELCTL_EM_CLK2B_110);
				data_to_send[37] = cfg;

				/* EM_INT1,2_CON */
				cfg = data_to_send[38];
				cfg &= ~(PANELCTL_EM_INT1_CON_MASK |
					PANELCTL_EM_INT2_CON_MASK);
				cfg |= (PANELCTL_EM_INT1_000 |
					PANELCTL_EM_INT2_001);
				data_to_send[38] = cfg;
			}
		}
		size = ARRAY_SIZE(data_to_send_v142);
	} else if (lcd->ver == VER_174) {
		data_to_send_v142[18] = s6e8aa0_apply_aid_panel_cond(lcd->aid);
		data_to_send = data_to_send_v142;
		size = ARRAY_SIZE(data_to_send_v142);
	} else if (lcd->ver == VER_32) {
		data_to_send = data_to_send_v32;
		size = ARRAY_SIZE(data_to_send_v32);
	} else if (lcd->ver == VER_96) {
		data_to_send = data_to_send_v96;
		size = ARRAY_SIZE(data_to_send_v96);
	} else if (lcd->ver == VER_210) {
		data_to_send = data_to_send_v210;
		size = ARRAY_SIZE(data_to_send_v210);
	} else
		return;

	if (!high_freq) {
		data_to_send[9] = 0x7e;
		data_to_send[25] = 0x0a;
		data_to_send[26] = 0x0a;
	}

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send, size);
}

static void s6e8aa0_display_condition_set(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	const unsigned char data_to_send[] = {
		0xf2, 0x80, 0x03, 0x0d
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8aa0_etc_source_control(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	const unsigned char data_to_send[] = {
		0xf6, 0x00, 0x02, 0x00
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8aa0_etc_pentile_control(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	unsigned char data_to_send[] = {
		0xb6, 0x0c, 0x02, 0x03, 0x32, 0xff, 0x44, 0x44, 0xc0,
		0x00
	};

	if (lcd->ver == VER_32 || lcd->ver == VER_96)
		data_to_send[5] = 0xc0;

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8aa0_etc_mipi_control1(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	const unsigned char data_to_send[] = {
		0xe1, 0x10, 0x1c, 0x17, 0x08, 0x1d
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8aa0_etc_mipi_control2(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	const unsigned char data_to_send[] = {
		0xe2, 0xed, 0x07, 0xc3, 0x13, 0x0d, 0x03
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8aa0_etc_power_control(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	unsigned char data_to_send[] = {
		0xf4, 0xcf, 0x0a, 0x12, 0x10, 0x1e, 0x33, 0x02
	};

	if (lcd->ver == VER_32 || lcd->ver == VER_96) {
		data_to_send[3] = 0x15;
		data_to_send[5] = 0x19;
	}

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}
static void s6e8aa0_etc_mipi_control3(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xe3, 0x40);
}

static void s6e8aa0_etc_mipi_control4(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	const unsigned char data_to_send[] = {
		0xe4, 0x00, 0x00, 0x14, 0x80, 0x00, 0x00, 0x00
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8aa0_etc_elvss_control(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	unsigned char data_to_send[] = {
		0xb1, 0x04, 0x00
	};

	if (lcd->id == 0x00)
		data_to_send[2] = 0x95;

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8aa0_elvss_nvm_set(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	struct backlight_device *bd = lcd->bd;
	int brightness = bd->props.brightness;
	unsigned char data_to_send[] = {
		0xd9, 0x14, 0x40, 0x0c, 0xcb, 0xce, 0x6e, 0xc4, 0x0f,
		0x40, 0x41, 0xd9, 0x00, 0x60, 0x19
	};

	/* FIXME: !! need to change brightness and elvss */
	if (lcd->ver == VER_32) {
		data_to_send[8] = 0x07;
		data_to_send[11] = 0xc1;
	} else if (lcd->ver == VER_96) {
		data_to_send[8] = 0x07;
		data_to_send[9] = 0xc0;
		data_to_send[11] = 0xc1;
	} else if (lcd->ver == VER_210 || lcd->ver == VER_174) {
		data_to_send[8] = 0x07;
		data_to_send[11] = 0xd0;
	} else {
		switch (brightness) {
		case 0 ... 6: /* 30cd ~ 100cd */
			data_to_send[11] = 0xdf;
			break;
		case 7 ... 11: /* 120cd ~ 150cd */
			data_to_send[11] = 0xdd;
			break;
		case 12 ... 15: /* 180cd ~ 210cd */
			data_to_send[11] = 0xd9;
			break;
		case 16 ... 24: /* 240cd ~ 300cd */
			data_to_send[11] = 0xd0;
			break;
		default:
			break;
		}
	}

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8aa0_sleep_in(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE, 0x10, 0x00);
}

static void s6e8aa0_sleep_out(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE, 0x11, 0x00);
}

static void s6e8aa0_display_on(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);

	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE, 0x29, 0x00);
}

static void s6e8aa0_display_off(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE, 0x28, 0x00);
}

static void s6e8aa0_apply_level_2_key(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	const unsigned char data_to_send[] = {
		0xfc, 0x5a, 0x5a
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8aa0_acl_on(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xc0, 0x01);
}

static void s6e8aa0_acl_off(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	ops->cmd_write(lcd_to_master(lcd),
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xc0, 0x00);
}

static void s6e8aa0_acl_ctrl_set(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	struct backlight_device *bd = lcd->bd;
	int brightness = bd->props.brightness;
	/* FIXME: !! must be review acl % value */
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
			if (brightness == 0 || brightness == 1) {
				s6e8aa0_acl_off(lcd);
				dev_dbg(&lcd->ld->dev,
					"cur_acl=%d\n", lcd->cur_acl);
			} else
				s6e8aa0_acl_on(lcd);
		}
		switch (brightness) {
		case 0 ... 1: /* 30cd */
			s6e8aa0_acl_off(lcd);
			lcd->cur_acl = 0;
			break;
		case 2 ... 6: /* 50cd ~ 100cd */
			ops->cmd_write(lcd_to_master(lcd),
				MIPI_DSI_DCS_LONG_WRITE,
				(unsigned int)cutoff_40,
				ARRAY_SIZE(cutoff_40));
			lcd->cur_acl = 40;
			break;
		case 7 ... 16: /* 120cd ~ 210cd */
			ops->cmd_write(lcd_to_master(lcd),
				MIPI_DSI_DCS_LONG_WRITE,
				(unsigned int)cutoff_45,
				ARRAY_SIZE(cutoff_45));
			lcd->cur_acl = 45;
			break;
		case 17 ... 24: /* 220cd ~ 300cd */
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
		s6e8aa0_acl_off(lcd);
		lcd->cur_acl = 0;
		dev_dbg(&lcd->ld->dev, "cur_acl = %d\n", lcd->cur_acl);
	}
}

static void s6e8aa0_enable_mtp_register(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	const unsigned char data_to_send[] = {
		0xf1, 0x5a, 0x5a
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8aa0_disable_mtp_register(struct s6e8aa0 *lcd)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	const unsigned char data_to_send[] = {
		0xf1, 0xa5, 0xa5
	};

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8aa0_read_id(struct s6e8aa0 *lcd, u8 *mtp_id)
{
	unsigned int ret;
	unsigned int addr = 0xd1;	/* MTP ID */
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);

	ret = ops->cmd_read(lcd_to_master(lcd),
			MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM,
			addr, 3, mtp_id);
}

static unsigned int s6e8aa0_read_mtp(struct s6e8aa0 *lcd, u8 *mtp_data)
{
	unsigned int ret;
	unsigned int addr = 0xd3;	/* MTP addr */
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);

	s6e8aa0_enable_mtp_register(lcd);

	ret = ops->cmd_read(lcd_to_master(lcd),
			MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM,
			addr, LDI_MTP_LENGTH, mtp_data);

	s6e8aa0_disable_mtp_register(lcd);

	return ret;
}

unsigned int convert_brightness_to_gamma(int brightness)
{
	const unsigned int gamma_table[] = {
		30, 30, 50, 70, 80, 90, 100, 120, 130, 140,
		150, 160, 170, 180, 190, 200, 210, 220, 230,
		240, 250, 260, 270, 280, 300
	};

	return gamma_table[brightness] - 1;
}

static int s6e8aa0_gamma_ctrl(struct s6e8aa0 *lcd, int brightness)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);

#ifdef CONFIG_BACKLIGHT_SMART_DIMMING
	unsigned int gamma;
	unsigned char gamma_set[GAMMA_TABLE_COUNT] = {0,};
	gamma = convert_brightness_to_gamma(brightness);

	gamma_set[0] = 0xfa;
	gamma_set[1] = 0x01;

	calc_gamma_table(&lcd->smart_dim, gamma, gamma_set + 2);

	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)gamma_set,
			GAMMA_TABLE_COUNT);
#else
	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)s6e8aa0_22_gamma_table[brightness],
			GAMMA_TABLE_COUNT);
#endif

	/* update gamma table. */
	ops->cmd_write(lcd_to_master(lcd), MIPI_DSI_DCS_SHORT_WRITE_PARAM,
			0xf7, 0x03);

	s6e8aa0_acl_ctrl_set(lcd);

	return 0;
}

static int s6e8aa0_panel_init(struct s6e8aa0 *lcd)
{
	struct backlight_device *bd = lcd->bd;
	int brightness = bd->props.brightness;

	s6e8aa0_apply_level_1_key(lcd);
	if (system_rev == 3)
		s6e8aa0_apply_level_2_key(lcd);

	s6e8aa0_sleep_out(lcd);
	usleep_range(5000, 6000);

	s6e8aa0_panel_cond(lcd, 1);
	s6e8aa0_display_condition_set(lcd);

	s6e8aa0_gamma_ctrl(lcd, brightness);

	s6e8aa0_etc_source_control(lcd);
	s6e8aa0_etc_pentile_control(lcd);
	s6e8aa0_elvss_nvm_set(lcd);
	s6e8aa0_etc_power_control(lcd);
	s6e8aa0_etc_elvss_control(lcd);

	/* if ID3 value is not 33h, branch private elvss mode */
	mdelay(lcd->ddi_pd->power_on_delay);
	dev_info(lcd->dev, "panel init sequence done.\n");

	return 0;
}

static int s6e8aa0_early_set_power(struct lcd_device *ld, int power)
{
	struct s6e8aa0 *lcd = lcd_get_data(ld);
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

static int s6e8aa0_set_power(struct lcd_device *ld, int power)
{
	struct s6e8aa0 *lcd = lcd_get_data(ld);
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


static int s6e8aa0_get_power(struct lcd_device *ld)
{
	struct s6e8aa0 *lcd = lcd_get_data(ld);

	return lcd->power;
}

static int s6e8aa0_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}

static int s6e8aa0_set_brightness(struct backlight_device *bd)
{
	int ret = 0, brightness = bd->props.brightness;
	struct s6e8aa0 *lcd = bl_get_data(bd);

	if (lcd->power == FB_BLANK_POWERDOWN) {
		dev_err(lcd->dev,
			"lcd off: brightness set failed.\n");
		return -EINVAL;
	}

	if (brightness < MIN_BRIGHTNESS ||
		brightness > bd->props.max_brightness) {
		dev_err(lcd->dev, "lcd brightness should be %d to %d.\n",
			MIN_BRIGHTNESS, MAX_BRIGHTNESS);
		return -EINVAL;
	}

	ret = s6e8aa0_gamma_ctrl(lcd, brightness);
	if (ret) {
		dev_err(&bd->dev, "lcd brightness setting failed.\n");
		return -EIO;
	}

	return ret;
}

static struct lcd_ops s6e8aa0_lcd_ops = {
	.early_set_power = s6e8aa0_early_set_power,
	.set_power = s6e8aa0_set_power,
	.get_power = s6e8aa0_get_power,
};

const static struct backlight_ops s6e8aa0_backlight_ops = {
	.get_brightness = s6e8aa0_get_brightness,
	.update_status = s6e8aa0_set_brightness,
};

static int s6e8aa0_check_mtp(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct s6e8aa0 *lcd = dev_get_drvdata(&dsim_dev->dev);
	int ret;
#ifdef CONFIG_BACKLIGHT_SMART_DIMMING
	int i;
#endif
	u8 mtp_data[LDI_MTP_LENGTH] = {0, };
	u8 mtp_id[3] = {0, };

	s6e8aa0_read_id(lcd, mtp_id);
	if (mtp_id[0] == 0x00) {
		dev_err(lcd->dev, "read id failed\n");
		return -EIO;
	}

	dev_info(lcd->dev,
		"Read ID : %x, %x, %x\n", mtp_id[0], mtp_id[1], mtp_id[2]);

	if (mtp_id[2] == 0x33)
		dev_info(lcd->dev,
			"ID-3 is 0xff does not support dynamic elvss\n");
	else {
		dev_info(lcd->dev,
			"ID-3 is 0x%x support dynamic elvss\n", mtp_id[2]);
		dev_info(lcd->dev, "Dynamic ELVSS Information\n");
	}

	lcd->ver = mtp_id[1];
	lcd->id = mtp_id[2];
	lcd->aid = (mtp_id[2] >> 5);

#ifdef CONFIG_BACKLIGHT_SMART_DIMMING
	for (i = 0; i < PANEL_ID_MAX; i++)
		lcd->smart_dim.panelid[i] = mtp_id[i];

	init_table_info(&lcd->smart_dim);
#endif

	ret = s6e8aa0_read_mtp(lcd, mtp_data);
	if (!ret) {
		dev_err(lcd->dev, "read mtp failed\n");
		return -EIO;
	}

#ifdef CONFIG_BACKLIGHT_SMART_DIMMING
	calc_voltage_table(&lcd->smart_dim, mtp_data);
#endif

	return 0;
}

static void s6e8aa0_power_on(struct mipi_dsim_lcd_device *dsim_dev,
				unsigned int enable)
{
	struct s6e8aa0 *lcd = dev_get_drvdata(&dsim_dev->dev);

	dev_dbg(lcd->dev, "%s:enable[%d]\n", __func__, enable);

	if (enable) {
		/* lcd power on */
		s6e8aa0_regulator_ctl(lcd, true);

		mdelay(lcd->ddi_pd->reset_delay);

		/* lcd reset */
		if (lcd->ddi_pd->reset)
			lcd->ddi_pd->reset(lcd->ld);

		mdelay(5);
	} else {
		/* lcd power off */
		s6e8aa0_regulator_ctl(lcd, false);
	}

}

static void s6e8aa0_set_sequence(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct s6e8aa0 *lcd = dev_get_drvdata(&dsim_dev->dev);

	s6e8aa0_panel_init(lcd);
	s6e8aa0_display_on(lcd);
}

static ssize_t acl_control_show(struct device *dev, struct
	device_attribute * attr, char *buf)
{
	struct s6e8aa0 *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->acl_enable);
	strcpy(buf, temp);

	return strlen(buf);
}

static ssize_t acl_control_store(struct device *dev, struct
	device_attribute * attr, const char *buf, size_t size)
{
	struct s6e8aa0 *lcd = dev_get_drvdata(dev);
	unsigned int value;
	int rc;

	rc = strict_strtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;

	if (lcd->acl_enable != value) {
		dev_info(dev, "acl control changed from %d to %d\n",
						lcd->acl_enable, value);
		lcd->acl_enable = value;
		s6e8aa0_acl_ctrl_set(lcd);
	}
	return size;
}

static ssize_t lcd_type_show(struct device *dev, struct
	device_attribute * attr, char *buf)
{
	struct s6e8aa0 *lcd = dev_get_drvdata(dev);
	char temp[32];
	int i;

	for (i = 0; i < lcd->model_count; i++) {
		if (lcd->ver == lcd->model[i].ver)
			break;
	}

	if (i == lcd->model_count)
		return -EINVAL;

	sprintf(temp, "%s\n", lcd->model[i].name);
	strcpy(buf, temp);

	return strlen(buf);
}

static int s6e8aa0_read_reg(struct s6e8aa0 *lcd, unsigned int addr, char *buf)
{
	unsigned char data[MAX_READ_LENGTH];
	unsigned int size;
	int i;
	int pos = 0;
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);

	memset(data, 0x0, ARRAY_SIZE(data));
	size = ops->cmd_read(lcd_to_master(lcd),
			MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM,
			addr, MAX_READ_LENGTH, data);
	if (!size) {
		dev_err(lcd->dev, "failed to read 0x%.2x register.\n", addr);
		return size;
	}

	pos += sprintf(buf, "0x%.2x, ", addr);
	for (i = 1; i < size+1; i++) {
		if (i % 9 == 0)
			pos += sprintf(buf+pos, "\n");
		pos += sprintf(buf+pos, "0x%.2x, ", data[i-1]);
	}
	pos += sprintf(buf+pos, "\n");

	return pos;
}

static int s6e8aa0_write_reg(struct s6e8aa0 *lcd, char *name)
{
	struct mipi_dsim_master_ops *ops = lcd_to_master_ops(lcd);
	const struct firmware *fw;
	char fw_path[MAX_STR+1];
	int ret = 0;

	mutex_lock(&lcd->lock);
	snprintf(fw_path, MAX_STR, LDI_FW_PATH, name);

	ret = request_firmware(&fw, fw_path, lcd->dev);
	if (ret) {
		dev_err(lcd->dev, "failed to request firmware.\n");
		mutex_unlock(&lcd->lock);
		return ret;
	}

	if (fw->size == 1)
		ret = ops->cmd_write(lcd_to_master(lcd),
				MIPI_DSI_DCS_SHORT_WRITE,
				(unsigned int)fw->data[0], 0);
	else if (fw->size == 2)
		ret = ops->cmd_write(lcd_to_master(lcd),
				MIPI_DSI_DCS_SHORT_WRITE_PARAM,
				(unsigned int)fw->data[0], fw->data[1]);
	else
		ret = ops->cmd_write(lcd_to_master(lcd),
				MIPI_DSI_DCS_LONG_WRITE,
				(unsigned int)fw->data, fw->size);
	if (ret)
		dev_err(lcd->dev, "failed to write 0x%.2x register and %d error.\n",
			fw->data[0], ret);

	release_firmware(fw);
	mutex_unlock(&lcd->lock);

	return ret;
}

static ssize_t read_reg_show(struct device *dev, struct
	device_attribute * attr, char *buf)
{
	struct s6e8aa0 *lcd = dev_get_drvdata(dev);

	if (lcd->cur_addr == 0) {
		dev_err(dev, "failed to set current lcd register.\n");
		return -EINVAL;
	}

	return s6e8aa0_read_reg(lcd, lcd->cur_addr, buf);
}

static ssize_t read_reg_store(struct device *dev, struct
	device_attribute * attr, const char *buf, size_t size)
{
	struct s6e8aa0 *lcd = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

	ret = sscanf(buf, "0x%x", &value);
	if (ret < 0)
		return ret;

	dev_info(dev, "success to set 0x%x address.\n", value);
	lcd->cur_addr = value;

	return size;
}

static ssize_t write_reg_store(struct device *dev, struct
	device_attribute * attr, const char *buf, size_t size)
{
	struct s6e8aa0 *lcd = dev_get_drvdata(dev);
	char name[32];
	int ret;

	ret = sscanf(buf, "%s", name);
	if (ret < 0)
		return ret;

	ret = s6e8aa0_write_reg(lcd, name);
	if (ret < 0)
		return ret;

	dev_info(dev, "success to set %s address.\n", name);

	return size;
}

static struct device_attribute device_attrs[] = {
	__ATTR(acl_control, S_IRUGO|S_IWUSR|S_IWGRP,
			acl_control_show, acl_control_store),
	__ATTR(lcd_type, S_IRUGO,
			lcd_type_show, NULL),
	__ATTR(read_reg, S_IRUGO|S_IWUSR|S_IWGRP,
			read_reg_show, read_reg_store),
	__ATTR(write_reg, S_IWUSR|S_IWGRP,
			NULL, write_reg_store),
};

static struct panel_model s6e8aa0_model[] = {
	{
		.ver = VER_142,			/* MACH_SLP_PQ */
		.name = "SMD_AMS465GS0x",
	}, {
		.ver = VER_174,			/* MACH_SLP_PQ Dali */
		.name = "SMD_AMS465GS0x",
	}, {
		.ver = VER_42,			/* MACH_SLP_PQ_LTE */
		.name = "SMD_AMS465GS0x",
	}, {
		.ver = VER_32,			/* MACH_SLP_PQ M0 B-Type */
		.name = "SMD_AMS480GYXX-0",
	}, {
		.ver = VER_96,			/* MACH_SLP_PQ Galaxy S3 */
		.name = "SMD_AMS480GYXX-0",
	}, {
		.ver = VER_210,			/* MACH_SLP_PQ M0 A-Type */
		.name = "SMD_AMS465GS0x",
	}
};

#if defined(CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ) || defined(CONFIG_DISPFREQ_OPP)
static int s6e8aa0_notifier_callback(struct notifier_block *this,
			unsigned long event, void *_data)
{
	struct s6e8aa0 *lcd = container_of(this, struct s6e8aa0, nb_disp);

	if (lcd->power == FB_BLANK_POWERDOWN)
		return NOTIFY_DONE;

	switch (event) {
	case EXYNOS4_DISPLAY_LV_HF:
		s6e8aa0_panel_cond(lcd, 1);
		break;
	case EXYNOS4_DISPLAY_LV_LF:
		s6e8aa0_panel_cond(lcd, 0);
		break;
	default:
		return NOTIFY_BAD;
	}

	return NOTIFY_DONE;
}
#endif

static int s6e8aa0_probe(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct s6e8aa0 *lcd;
	int ret;
	int i;

	lcd = kzalloc(sizeof(struct s6e8aa0), GFP_KERNEL);
	if (!lcd) {
		dev_err(&dsim_dev->dev, "failed to allocate s6e8aa0 structure.\n");
		return -ENOMEM;
	}

	lcd->dsim_dev = dsim_dev;
	lcd->ddi_pd = (struct lcd_platform_data *)dsim_dev->platform_data;
	lcd->dev = &dsim_dev->dev;

	mutex_init(&lcd->lock);

	lcd->reg_vdd3 = regulator_get(lcd->dev, "VDD3");
	if (IS_ERR(lcd->reg_vdd3)) {
		ret = PTR_ERR(lcd->reg_vdd3);
		dev_err(lcd->dev, "failed to get %s regulator (%d)\n",
				"VDD3", ret);
		lcd->reg_vdd3 = NULL;
	}

	lcd->reg_vci = regulator_get(lcd->dev, "VCI");
	if (IS_ERR(lcd->reg_vci)) {
		ret = PTR_ERR(lcd->reg_vci);
		dev_err(lcd->dev, "failed to get %s regulator (%d)\n",
				"VCI", ret);
		lcd->reg_vci = NULL;
	}

	lcd->ld = lcd_device_register("s6e8aa0", lcd->dev, lcd,
			&s6e8aa0_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		dev_err(lcd->dev, "failed to register lcd ops.\n");
		ret = PTR_ERR(lcd->ld);
		goto err_regulator;
	}

	lcd->bd = backlight_device_register("s6e8aa0-bl", lcd->dev, lcd,
			&s6e8aa0_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		dev_err(lcd->dev, "failed to register backlight ops.\n");
		ret = PTR_ERR(lcd->bd);
		goto err_unregister_lcd;
	}

	if (lcd->ddi_pd)
		lcd->property = lcd->ddi_pd->pdata;

#if defined(CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ) || defined(CONFIG_DISPFREQ_OPP)
	if (lcd->property && lcd->property->dynamic_refresh) {
		lcd->nb_disp.notifier_call = s6e8aa0_notifier_callback;
		ret = exynos4_display_register_client(&lcd->nb_disp);
		if (ret < 0)
			dev_warn(&lcd->ld->dev, "failed to register exynos-display notifier\n");
	}
#endif

	lcd->bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd->bd->props.brightness = MAX_BRIGHTNESS;
	lcd->acl_enable = 1;
	lcd->cur_acl = 0;
	lcd->model = s6e8aa0_model;
	lcd->model_count = ARRAY_SIZE(s6e8aa0_model);
	for (i = 0; i < ARRAY_SIZE(device_attrs); i++) {
		ret = device_create_file(&lcd->ld->dev,
					&device_attrs[i]);
		if (ret < 0) {
			dev_err(&lcd->ld->dev, "failed to add sysfs entries\n");
			break;
		}
	}

	dev_set_drvdata(&dsim_dev->dev, lcd);

	s6e8aa0_regulator_ctl(lcd, true);
	lcd->power = FB_BLANK_UNBLANK;

	dev_info(lcd->dev, "probed s6e8aa0 panel driver(%s).\n",
			dev_name(&lcd->ld->dev));

	return 0;

err_unregister_lcd:
	lcd_device_unregister(lcd->ld);

err_regulator:
	regulator_put(lcd->reg_vci);
	regulator_put(lcd->reg_vdd3);

	kfree(lcd);

	return ret;
}

static void s6e8aa0_remove(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct s6e8aa0 *lcd = dev_get_drvdata(&dsim_dev->dev);

	backlight_device_unregister(lcd->bd);

	lcd_device_unregister(lcd->ld);

	regulator_put(lcd->reg_vci);
	regulator_put(lcd->reg_vdd3);

#if defined(CONFIG_ARM_EXYNOS4_DISPLAY_DEVFREQ) || defined(CONFIG_DISPFREQ_OPP)
	if (lcd->property && lcd->property->dynamic_refresh)
		exynos4_display_unregister_client(&lcd->nb_disp);
#endif

	kfree(lcd);
}

#ifdef CONFIG_PM
static int s6e8aa0_suspend(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct s6e8aa0 *lcd = dev_get_drvdata(&dsim_dev->dev);

	s6e8aa0_display_off(lcd);
	s6e8aa0_sleep_in(lcd);
	mdelay(lcd->ddi_pd->power_off_delay);

	return 0;
}

static int s6e8aa0_resume(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct s6e8aa0 *lcd = dev_get_drvdata(&dsim_dev->dev);

	s6e8aa0_sleep_out(lcd);
	mdelay(lcd->ddi_pd->power_on_delay);

	return 0;
}
#else
#define s6e8aa0_suspend		NULL
#define s6e8aa0_resume		NULL
#endif

static struct mipi_dsim_lcd_driver s6e8aa0_dsim_ddi_driver = {
	.name = "s6e8aa0",
	.id = -1,

	.power_on = s6e8aa0_power_on,
	.check_mtp = s6e8aa0_check_mtp,
	.set_sequence = s6e8aa0_set_sequence,
	.probe = s6e8aa0_probe,
	.remove = s6e8aa0_remove,
	.suspend = s6e8aa0_suspend,
	.resume = s6e8aa0_resume,
};

static int s6e8aa0_init(void)
{
	s5p_mipi_dsi_register_lcd_driver(&s6e8aa0_dsim_ddi_driver);

	return 0;
}

static void s6e8aa0_exit(void)
{
	return;
}

module_init(s6e8aa0_init);
module_exit(s6e8aa0_exit);

MODULE_AUTHOR("Donghwa Lee <dh09.lee@samsung.com>");
MODULE_AUTHOR("Inki Dae <inki.dae@samsung.com>");
MODULE_AUTHOR("Joongmock Shin <jmock.shin@samsung.com>");
MODULE_AUTHOR("Eunchul Kim <chulspro.kim@samsung.com>");
MODULE_DESCRIPTION("MIPI-DSI based s6e8aa0 AMOLED LCD Panel Driver");
MODULE_LICENSE("GPL");
