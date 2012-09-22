/* linux/drivers/video/samsung/s3cfb_s6e63m0.c
 *
 * MIPI-DSI based AMS367GEXX AMOLED lcd panel driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
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
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/backlight.h>
#include <linux/lcd.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-dsim.h>
#include <mach/dsim.h>
#include <mach/mipi_ddi.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "s5p-dsim.h"
#include "s3cfb.h"
#include "s6e63m0_gamma_l.h"

#include "s6e63m0_gamma_grande.h"
#define SMART_DIMMING

#ifdef SMART_DIMMING
#include "smart_mtp_s6e63m0.h"
#endif

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255
#define MAX_GAMMA			300
#define DEFAULT_BRIGHTNESS		160
#define DEFAULT_GAMMA_LEVEL		GAMMA_160CD

#define LDI_ID_REG			0xDA
#define LDI_ID_LEN			3

#ifdef SMART_DIMMING
#define	PANEL_A1_SM2			0xA1

#define LDI_MTP_LENGTH		24
#define LDI_MTP_ADDR			0xD3

#define DYNAMIC_ELVSS_MIN_VALUE	0x81
#define DYNAMIC_ELVSS_MAX_VALUE	0x9F

#define ELVSS_MODE0_MIN_VOLTAGE	62
#define ELVSS_MODE1_MIN_VOLTAGE	52

#if defined(CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD)
#define NR_S6E63M0_PANEL	(2)
#else	/* CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD */
#define NR_S6E63M0_PANEL	(1)
#endif	/* CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD */

struct str_elvss {
	u8 reference;
	u8 limit;
};
#endif

struct lcd_info *g_lcd;

struct lcd_info {
	unsigned int			bl;
	unsigned int			auto_brightness;
	unsigned int			acl_enable;
	unsigned int			cur_acl;
	unsigned int			current_bl;
	unsigned int			current_elvss;

	unsigned int			ldi_enable;
	unsigned int			power;
	struct mutex			lock;
	struct mutex			bl_lock;

	struct device			*dev;
	struct lcd_device		*ld;
	struct backlight_device		*bd;
	struct lcd_platform_data	*lcd_pd;
	struct early_suspend		early_suspend;

	unsigned char			id[LDI_ID_LEN];

	unsigned char			**gamma_table[NR_S6E63M0_PANEL];
	unsigned char			**elvss_table[NR_S6E63M0_PANEL];

#if defined(CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD)
	unsigned int			detected[NR_S6E63M0_PANEL];
	unsigned int			panel_select;
#endif	/* CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD */

#ifdef SMART_DIMMING
	unsigned int			support_elvss;

	struct SMART_DIM		smart;
	struct str_elvss		elvss;
#endif
	unsigned int			irq;
	unsigned int			connected;

	struct dsim_global		*dsim;
};

static const unsigned int candela_table[GAMMA_MAX] = {
	 30,  40,  50,  60,  70,  80,  90, 100, 110, 120,
	130, 140, 150, 160, 170, 180, 190, 200, 210, 220,
	230, 240, 250, MAX_GAMMA
};

/* Added for DUAL LCD work*/
static int s6e63m0_read_mtp(struct lcd_info *lcd);
#ifdef SMART_DIMMING_DEBUG_LCD
static int print_mtp_value(struct lcd_info *lcd);
#endif
static int s6e63m0_read_id(struct lcd_info *lcd, u8 *buf);
static int s6e63m0_dual_panel_detect(struct lcd_info *lcd);
/* Added for DUAL LCD work*/

#if defined(GPIO_OLED_DET)
struct delayed_work hs_clk_re_try;
unsigned int count_dsim;

static void hs_clk_re_try_work(struct work_struct *work)
{
	int read_oled_det;

	read_oled_det = gpio_get_value(GPIO_OLED_DET);
	printk(KERN_INFO "%s, %d, %d\n", __func__,
			count_dsim, read_oled_det);
	if (read_oled_det == 1) {
		if (count_dsim < 10) {
			schedule_delayed_work(&hs_clk_re_try, HZ/8);
			count_dsim++;
			set_dsim_hs_clk_toggle_count(15);
		} else
			set_dsim_hs_clk_toggle_count(0);
	} else
		set_dsim_hs_clk_toggle_count(0);
}

static irqreturn_t oled_det_int(int irq, void *dev_id)
{
	pr_info("[DSIM] %s\n", __func__);
	schedule_delayed_work(&hs_clk_re_try, HZ/16);
	count_dsim = 0;
	return IRQ_HANDLED;
}
#endif

static int s6e63m0_write(struct lcd_info *lcd,
					const unsigned char *seq, int len)
{
	int size;
	const unsigned char *wbuf;

	if (!lcd->connected)
		return 0;
	mutex_lock(&lcd->lock);

	size = len;
	wbuf = seq;
	if (size == 1)
		lcd->dsim->ops->cmd_write(lcd->dsim, DCS_WR_NO_PARA, wbuf[0], 0);
	else if (size == 2)
		lcd->dsim->ops->cmd_write(lcd->dsim, DCS_WR_1_PARA, wbuf[0], wbuf[1]);
	else
		lcd->dsim->ops->cmd_write(lcd->dsim, DCS_LONG_WR, (unsigned int)wbuf, size);
	mutex_unlock(&lcd->lock);

	return 0;
}

static int _s6e63m0_read(struct lcd_info *lcd, const u8 addr,
				u16 count, u8 *buf)
{
	int ret = 0;

	if (!lcd->connected)
		return ret;

	mutex_lock(&lcd->lock);
	if (lcd->dsim->ops->cmd_read)
		ret = lcd->dsim->ops->cmd_read(lcd->dsim, addr, count, buf);
	mutex_unlock(&lcd->lock);

	return ret;
}

static int s6e63m0_read(struct lcd_info *lcd, const u8 addr,
			u16 count, u8 *buf, u8 retry_cnt)
{
	int ret = 0;
read_retry:
	ret = _s6e63m0_read(lcd, addr, count, buf);
	if (!ret) {
		if (retry_cnt) {
			pr_info("[WARN:LCD] %s : retry cnt : %d\n",
						__func__, retry_cnt);
			retry_cnt--;
			goto read_retry;
		} else
			pr_info("[ERROR:LCD] %s : 0x%02x read failed\n",
						__func__, addr);
	}
	return ret;
}

static int get_backlight_level_from_brightness(int brightness)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is
	  only supported from 0 to 24 */

	switch (brightness) {
	case 0 ... 29:
		backlightlevel = GAMMA_30CD;
		break;
	case 30 ... 254:
		backlightlevel = (brightness - candela_table[0]) / 10;
		break;
	case 255:
		backlightlevel = ARRAY_SIZE(candela_table) - 1;
		break;
	default:
		backlightlevel = DEFAULT_GAMMA_LEVEL;
		break;
	}
	return backlightlevel;
}

static int s6e63m0_gamma_ctl(struct lcd_info *lcd)
{
#ifdef SMART_DIMMING_DEBUG_LCD
	int j;
#endif
	s6e63m0_write(lcd, lcd->gamma_table[lcd->panel_select][lcd->bl],
				GAMMA_PARAM_SIZE);

	s6e63m0_write(lcd, SEQ_GAMMA_UPDATE,
				ARRAY_SIZE(SEQ_GAMMA_UPDATE));
#ifdef SMART_DIMMING_DEBUG_LCD
	pr_info("panel_select = %d bl = %d\n", lcd->panel_select, lcd->bl);
	for (j = 0; j < GAMMA_PARAM_SIZE; j++)
		pr_info("0x%02x, ",
			lcd->gamma_table[lcd->panel_select][lcd->bl][j]);
#endif
	return 0;
}

static int s6e63m0_set_acl(struct lcd_info *lcd)
{
	if (lcd->acl_enable) {
		if (lcd->cur_acl == 0) {
			if (lcd->bl == 0 || lcd->bl == 1) {
				s6e63m0_write(lcd, SEQ_ACL_OFF,	ARRAY_SIZE(SEQ_ACL_OFF));
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d, acl_off\n",
					__func__, lcd->cur_acl);
			} else {
				s6e63m0_write(lcd, SEQ_ACL_ON, ARRAY_SIZE(SEQ_ACL_ON));
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d, acl_on\n",
					__func__, lcd->cur_acl);
			}
		}
		switch (lcd->bl) {
		case GAMMA_30CD ... GAMMA_40CD:
				s6e63m0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				lcd->cur_acl = 0;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n",
					__func__, lcd->cur_acl);
			break;
		case GAMMA_50CD ... GAMMA_160CD:
				s6e63m0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_40P], ACL_PARAM_SIZE);
				lcd->cur_acl = 40;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n",
					__func__, lcd->cur_acl);
			break;
		case GAMMA_170CD ... GAMMA_220CD:
				s6e63m0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_48P], ACL_PARAM_SIZE);
				lcd->cur_acl = 48;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n",
					__func__, lcd->cur_acl);
			break;
		case GAMMA_230CD ... GAMMA_300CD:
				s6e63m0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_50P], ACL_PARAM_SIZE);
				lcd->cur_acl = 50;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n",
					__func__, lcd->cur_acl);
			break;
		default:
				lcd->cur_acl = 0;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			break;
		}
	} else {
		s6e63m0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
		lcd->cur_acl = 0;
		dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d, acl_off\n",
					__func__, lcd->cur_acl);
	}
	return 0;
}

#ifdef SMART_DIMMING
static int s6e63m0_set_elvss(struct lcd_info *lcd)
{
	int ret = 0, elvss_level = 0;
	u32 candela = candela_table[lcd->bl];

	switch (candela) {
	case 0 ... 100:
		elvss_level = ELVSS_MIN;
		break;
	case 101 ... 160:
		elvss_level = ELVSS_1;
		break;
	case 161 ... 200:
		elvss_level = ELVSS_2;
		break;
	case 201 ... 300:
		elvss_level = ELVSS_MAX;
		break;
	default:
		break;
	}
	if (lcd->current_elvss != lcd->elvss_table[lcd->panel_select][elvss_level][1]) {
		ret = s6e63m0_write(lcd, lcd->elvss_table[lcd->panel_select][elvss_level],
					ELVSS_PARAM_SIZE);
		lcd->current_elvss = lcd->elvss_table[lcd->panel_select][elvss_level][1];
	}
	dev_dbg(&lcd->ld->dev, "elvss = %x\n",
				lcd->elvss_table[lcd->panel_select][elvss_level][1]);
	if (ret) {
		ret = -EPERM;
		goto elvss_err;
	}

elvss_err:
	return ret;
}
#endif
static u8 get_elvss_value(struct lcd_info *lcd, u8 elvss_level)
{
	u8 alph[ELVSS_STATUS_MAX] = {0xb, 0x8, 0x6, 0x0};
	u8 data = 0;
	u8 value = 0;

	value = lcd->id[2];
	data = value + alph[elvss_level];
	if (data > 0x29)
		data = 0x29;

	return data;
}

static int alloc_elvss_table(struct lcd_info *lcd)
{
	int i, k, ret = 0;
	for (k = 0; k < NR_S6E63M0_PANEL; k++) {
		lcd->elvss_table[k]
			= kzalloc(ELVSS_STATUS_MAX * sizeof(u8 *), GFP_KERNEL);

		if (IS_ERR_OR_NULL(lcd->elvss_table[k])) {
			pr_err("failed to allocate elvss table\n");
			ret = -ENOMEM;
			goto err_alloc_elvss_table;
		}

		for (i = 0; i < ELVSS_STATUS_MAX; i++) {
			lcd->elvss_table[k][i]
				= kzalloc(ELVSS_PARAM_SIZE * sizeof(u8),
					GFP_KERNEL);
			if (IS_ERR_OR_NULL(lcd->elvss_table[k][i])) {
				pr_err("failed to allocate elvss\n");
				ret = -ENOMEM;
				goto err_alloc_elvss;
			}
		}
	}
	return 0;

err_alloc_elvss:
	while (i > 0) {
		kfree(lcd->elvss_table[i-1]);
		i--;
	}
	kfree(lcd->elvss_table);
err_alloc_elvss_table:
	return ret;
}

static int init_elvss_table(struct lcd_info *lcd)
{
	int i, j;
	for (i = 0; i < ELVSS_STATUS_MAX; i++) {
		lcd->elvss_table[lcd->panel_select][i][0] = 0xB2;
		for (j = 1; j < ELVSS_PARAM_SIZE; j++)
			lcd->elvss_table[lcd->panel_select][i][j]
				= get_elvss_value(lcd, i);
	}
#ifdef SMART_DIMMING_DEBUG_LCD
	pr_info("ELVSS_TABLE [%2d]\n", lcd->panel_select);
	for (i = 0; i < ELVSS_STATUS_MAX; i++) {
		for (j = 0; j < ELVSS_PARAM_SIZE; j++)
			pr_info("0x%02x, ",
				lcd->elvss_table[lcd->panel_select][i][j]);
	pr_info("\n");
	}
#endif
	return 0;
}

static int alloc_gamma_table(struct lcd_info *lcd)
{
	int i, k, ret = 0;
	for (k = 0; k < NR_S6E63M0_PANEL; k++) {
		lcd->gamma_table[k] = kzalloc(GAMMA_MAX * sizeof(u8 *),
				GFP_KERNEL);
		if (IS_ERR_OR_NULL(lcd->gamma_table[k])) {
			pr_err("failed to allocate gamma table\n");
			ret = -ENOMEM;
			goto err_alloc_gamma_table;
		}

		for (i = 0; i < GAMMA_MAX; i++) {
			lcd->gamma_table[k][i]
				= kzalloc(GAMMA_PARAM_SIZE * sizeof(u8),
						GFP_KERNEL);
			if (IS_ERR_OR_NULL(lcd->gamma_table[k][i])) {
				pr_err("failed to allocate gamma\n");
				ret = -ENOMEM;
				goto err_alloc_gamma;
			}
		}
	}
	return 0;

err_alloc_gamma:
	while (i > 0) {
		kfree(lcd->gamma_table[i-1]);
		i--;
	}
	kfree(lcd->gamma_table);
err_alloc_gamma_table:
	return ret;
}

static int init_gamma_table(struct lcd_info *lcd)
{
	int i, j;
	char gen_gamma[GAMMA_PARAM_SIZE] = {0,};

	for (i = 0; i < GAMMA_MAX; i++) {
		lcd->gamma_table[lcd->panel_select][i][0] = 0xFA;
		lcd->gamma_table[lcd->panel_select][i][1] = 0x02;

		lcd->smart.brightness_level = candela_table[i];
		generate_gamma(&(lcd->smart), gen_gamma, GAMMA_PARAM_SIZE);
		for (j = 2; j < GAMMA_PARAM_SIZE; j++)
			lcd->gamma_table[lcd->panel_select][i][j] = gen_gamma[j - 2];
	}
#ifdef SMART_DIMMING_DEBUG_LCD
	pr_info("GAMMA_TABLE[%2d]\n", lcd->panel_select);
	for (i = 0; i < GAMMA_MAX; i++) {
		pr_info("%3d : ", i);
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			pr_info("0x%02x, ",
				lcd->gamma_table[lcd->panel_select][i][j]);
	pr_info("\n");
	}
#endif
	return 0;
}

static int update_brightness(struct lcd_info *lcd, u8 force)
{
	int ret;
#if defined(CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD)
	int sel;
#endif	/* CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD */
	u32 brightness;

	mutex_lock(&lcd->bl_lock);

	brightness = lcd->bd->props.brightness;

#if defined(CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD)
	sel = s5p_dsim_get_panel_sel_value();
	lcd->panel_select = sel;
#endif	/* CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD */

	if (unlikely(!lcd->auto_brightness && brightness > 250))
		brightness = 250;

	lcd->bl = get_backlight_level_from_brightness(brightness);

	if ((force) || ((lcd->ldi_enable) &&
				(lcd->current_bl != lcd->bl))) {

		ret = s6e63m0_set_elvss(lcd);
		ret = s6e63m0_set_acl(lcd);
		ret = s6e63m0_gamma_ctl(lcd);

		lcd->current_bl = lcd->bl;
		dev_info(&lcd->ld->dev,
			"panel = %d brightness=%d, bl=%d, candela=%d\n",
			lcd->panel_select, brightness,
			lcd->bl, candela_table[lcd->bl]);
	}

	mutex_unlock(&lcd->bl_lock);

	return 0;
}

static int s6e63m0_mtp_read_cmds(struct lcd_info *lcd)
{
	s6e63m0_write(lcd, SEQ_PREPARE_MTP_READ1,
		ARRAY_SIZE(SEQ_PREPARE_MTP_READ1));
	s6e63m0_write(lcd, SEQ_PREPARE_MTP_READ2,
		ARRAY_SIZE(SEQ_PREPARE_MTP_READ2));
	s6e63m0_write(lcd, SEQ_SLEEP_OUT,
		ARRAY_SIZE(SEQ_SLEEP_OUT));
	mdelay(120);
	s6e63m0_write(lcd, SEQ_CONTENTION_ERROR_REMOVE,
		ARRAY_SIZE(SEQ_CONTENTION_ERROR_REMOVE));
	return 0;
}

static int s6e63m0_ldi_init(struct lcd_info *lcd)
{
	int ret = 0;

	mdelay(25);	/* 25ms	*/
	s6e63m0_write(lcd, SEQ_SW_RESET,
		ARRAY_SIZE(SEQ_SW_RESET));	/* SW Reset */
	mdelay(5);	/* Wait 5ms more */
	s6e63m0_write(lcd, SEQ_APPLY_LEVEL2_KEY_ENABLE,
		ARRAY_SIZE(SEQ_APPLY_LEVEL2_KEY_ENABLE));
	s6e63m0_write(lcd, SEQ_APPLY_MTP_KEY_ENABLE,
		ARRAY_SIZE(SEQ_APPLY_MTP_KEY_ENABLE));
	s6e63m0_write(lcd, SEQ_SLEEP_OUT,
		ARRAY_SIZE(SEQ_SLEEP_OUT));
	mdelay(10);	/* 10ms */
	s6e63m0_write(lcd, SEQ_PANEL_CONDITION_SET,
		ARRAY_SIZE(SEQ_PANEL_CONDITION_SET));
	s6e63m0_write(lcd, SEQ_DISPLAY_CONDITION_SET1,
		ARRAY_SIZE(SEQ_DISPLAY_CONDITION_SET1));
	s6e63m0_write(lcd, SEQ_DISPLAY_CONDITION_SET2,
		ARRAY_SIZE(SEQ_DISPLAY_CONDITION_SET2));
	s6e63m0_write(lcd, lcd->gamma_table[lcd->panel_select][lcd->bl],
		GAMMA_PARAM_SIZE);
	s6e63m0_write(lcd, SEQ_GAMMA_UPDATE,
		ARRAY_SIZE(SEQ_GAMMA_UPDATE));
	s6e63m0_write(lcd, SEQ_ETC_SOURCE_CONTROL,
		ARRAY_SIZE(SEQ_ETC_SOURCE_CONTROL));
	s6e63m0_write(lcd, SEQ_ETC_CONTROL_B3h,
		ARRAY_SIZE(SEQ_ETC_CONTROL_B3h));
	s6e63m0_write(lcd, SEQ_ELVSS_SET, ARRAY_SIZE(SEQ_ELVSS_SET));
	s6e63m0_write(lcd, SEQ_ELVSS_ON, ARRAY_SIZE(SEQ_ELVSS_ON));
	return ret;
}

static int s6e63m0_ldi_enable(struct lcd_info *lcd)
{
	int ret = 0;
	s6e63m0_write(lcd, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
	return ret;
}

static int s6e63m0_ldi_disable(struct lcd_info *lcd)
{
	int ret = 0;
	s6e63m0_write(lcd, SEQ_DISPLAY_OFF,
				ARRAY_SIZE(SEQ_DISPLAY_OFF));
	s6e63m0_write(lcd, SEQ_STANDBY_ON,
				ARRAY_SIZE(SEQ_STANDBY_ON));
	return ret;
}

static int s6e63m0_power_on(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	ret = s6e63m0_ldi_init(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}

	msleep(120);

	ret = s6e63m0_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}
	lcd->ldi_enable = 1;

#if !defined(CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD)
	update_brightness(lcd, 1);
#endif
err:
	return ret;
}

static int s6e63m0_power_off(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);
	lcd->ldi_enable = 0;
	ret = s6e63m0_ldi_disable(lcd);
	msleep(135);

	return ret;
}

static int s6e63m0_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = s6e63m0_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = s6e63m0_power_off(lcd);
	if (!ret)
		lcd->power = power;

	return ret;
}

static int s6e63m0_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return s6e63m0_power(lcd, power);
}

static int s6e63m0_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

int s6e63m0_sleep_in(void)
{
	struct lcd_info *lcd = g_lcd;
	int ret;

	ret = s6e63m0_ldi_disable(lcd);
	return ret;
}

static int s6e63m0_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	int brightness = bd->props.brightness;
	struct lcd_info *lcd = bl_get_data(bd);

	/* dev_info(&lcd->ld->dev, "%s: brightness=%d\n",
				__func__, brightness); */

	if (brightness < MIN_BRIGHTNESS ||
		brightness > bd->props.max_brightness) {
		dev_err(&bd->dev, "lcd brightness should be %d to %d. now %d\n",
			MIN_BRIGHTNESS, MAX_BRIGHTNESS, brightness);
		return -EINVAL;
	}

	if (lcd->ldi_enable) {
		ret = update_brightness(lcd, 0);
		if (ret < 0) {
			dev_err(lcd->dev, "err in %s\n", __func__);
			return -EINVAL;
		}
	}

	return ret;
}

static int s6e63m0_get_brightness(struct backlight_device *bd)
{
	struct lcd_info *lcd = bl_get_data(bd);

	return candela_table[lcd->bl];
}

static struct lcd_ops s6e63m0_lcd_ops = {
	.set_power = s6e63m0_set_power,
	.get_power = s6e63m0_get_power,
};

static const struct backlight_ops s6e63m0_backlight_ops = {
	.get_brightness = s6e63m0_get_brightness,
	.update_status = s6e63m0_set_brightness,
};

static ssize_t power_reduce_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->acl_enable);
	strcpy(buf, temp);

	return strlen(buf);
}

static ssize_t power_reduce_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = strict_strtoul(buf, (unsigned int)0,
				(unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (lcd->acl_enable != value) {
			dev_info(dev, "%s - %d, %d\n",
					__func__, lcd->acl_enable, value);
			mutex_lock(&lcd->bl_lock);
			lcd->acl_enable = value;
			if (lcd->ldi_enable)
				s6e63m0_set_acl(lcd);
			mutex_unlock(&lcd->bl_lock);
		}
	}
	return size;
}

static DEVICE_ATTR(power_reduce, 0664, power_reduce_show,
			power_reduce_store);

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[20];
	sprintf(temp, "SMD_AMS397GEXX\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0444, lcd_type_show, NULL);

static ssize_t gamma_table_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int i, j, k;

	for (k = 0; k < NR_S6E63M0_PANEL ; k++) {
		for (i = 0; i < GAMMA_MAX; i++) {
			for (j = 0; j < GAMMA_PARAM_SIZE; j++)
				pr_info("0x%02x, ", lcd->gamma_table[k][i][j]);
			pr_info("\n");
		}

		for (i = 0; i < ELVSS_STATUS_MAX; i++) {
			for (j = 0; j < ELVSS_PARAM_SIZE; j++)
				pr_info("0x%02x, ", lcd->elvss_table[k][i][j]);
			pr_info("\n");
		}
	}

	return strlen(buf);
}
static DEVICE_ATTR(gamma_table, 0444, gamma_table_show, NULL);

static ssize_t auto_brightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->auto_brightness);
	strcpy(buf, temp);

	return strlen(buf);
}

static ssize_t auto_brightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = strict_strtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (lcd->auto_brightness != value) {
			dev_info(dev, "%s - %d, %d\n",
					__func__,
					lcd->auto_brightness,
					value);
			mutex_lock(&lcd->bl_lock);
			lcd->auto_brightness = value;
			mutex_unlock(&lcd->bl_lock);
			if (lcd->ldi_enable)
				update_brightness(lcd, 0);
		}
	}
	return size;
}

static DEVICE_ATTR(auto_brightness, 0644, auto_brightness_show,
			auto_brightness_store);

#ifdef CONFIG_HAS_EARLYSUSPEND
struct lcd_info *g_lcd;

void s6e63m0_early_suspend(void)
{
	struct lcd_info *lcd = g_lcd;
	set_dsim_lcd_enabled(0);

	dev_info(&lcd->ld->dev, "+%s\n", __func__);
#if defined(GPIO_OLED_DET)
	disable_irq(lcd->irq);
	gpio_request(GPIO_OLED_DET, "OLED_DET");
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_OLED_DET, GPIO_LEVEL_LOW);
	gpio_free(GPIO_OLED_DET);
#endif
	s6e63m0_power(lcd, FB_BLANK_POWERDOWN);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

#if defined(CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD)
static int s6e63m0_dual_panel_detect(struct lcd_info *lcd)
{
	int ret = 0;

	/* Calculating Gamma table of outer LCD */
	lcd->panel_select = s5p_dsim_get_panel_sel_value();
	msleep(25);
	if (lcd->gamma_table[lcd->panel_select][0][0] == 0x00) {
		pr_info("GAMMA TABLE not found !! in panel %d\n",
				lcd->panel_select);
		ret = s6e63m0_read_id(lcd, lcd->id);
		if (lcd->id[0] != 0xFE) {
			lcd->detected[lcd->panel_select] = 0;
			goto dual_det_err;
		} else
			lcd->detected[lcd->panel_select] = 1;
		dev_info(&lcd->ld->dev, "ID: %x, %x, %x\n", lcd->id[0],
				lcd->id[1], lcd->id[2]);
		dev_info(&lcd->ld->dev, "s6e63m0 driver has been detected.\n");
		ret = s6e63m0_read_mtp(lcd);
		if (!ret) {
			pr_info("[LCD:ERROR] : %s read mtp failed\n",
					__func__);
		}
#ifdef SMART_DIMMING_DEBUG_LCD
		ret += print_mtp_value(lcd);
#endif
		ret += Smart_dimming_init(&(lcd->smart));
		if (lcd->support_elvss)
			ret = init_elvss_table(lcd);
		ret += init_gamma_table(lcd);
		if (ret) {
			lcd->gamma_table[lcd->panel_select]
				= (unsigned char **)s6e63m0_gamma22_table;
			lcd->elvss_table[lcd->panel_select]
				= (unsigned char **)ELVSS_TABLE;
		}
	}
	return 0;
dual_det_err:
	pr_info("Outer LCD detectoin failed\n");
	ret = s5p_dsim_toggle_lcd();
	return 0;
}
#endif	/* CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD */

void s6e63m0_late_resume(void)
{
	struct lcd_info *lcd = g_lcd;

	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	s6e63m0_power(lcd, FB_BLANK_UNBLANK);
#if defined(GPIO_OLED_DET)
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
	enable_irq(lcd->irq);
#endif
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	set_dsim_lcd_enabled(1);
#if defined(CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD)
	if (lcd->detected[lcd->panel_select] == 0)
		s6e63m0_dual_panel_detect(lcd);
	update_brightness(lcd, 1);
#endif
	return ;
}
#endif

static int s6e63m0_id_read_cmds(struct lcd_info *lcd)
{
	s6e63m0_write(lcd, SEQ_PREPARE_ID_READ,
		ARRAY_SIZE(SEQ_PREPARE_ID_READ));
	return 0;
}

static int s6e63m0_read_id(struct lcd_info *lcd, u8 *buf)
{
	int ret = 0;

	s6e63m0_id_read_cmds(lcd);
	/* ID1: Default value of panel */
	ret += s6e63m0_read(lcd, 0xDA, 1, buf, 3);
	/* ID2: Way to Cell working */
	ret += s6e63m0_read(lcd, 0xDB, 1, buf + 1, 3);
	/* Olny ID2 == C1 support */
	if (*(buf + 1) == 0xC1)
		ret += s6e63m0_read(lcd, 0xDC, 1, buf + 2, 3);
	else
		*(buf + 2) = 0x00;
	if (!ret) {
		lcd->connected = 0;
		dev_info(&lcd->ld->dev, "panel is not connected well\n");
	}
	return ret;
}

#ifdef SMART_DIMMING
static int s6e63m0_read_mtp(struct lcd_info *lcd)
{
	int ret;
	s6e63m0_mtp_read_cmds(lcd);
	ret = s6e63m0_read(lcd, LDI_MTP_ADDR,
		LDI_MTP_LENGTH, (u8 *)(&(lcd->smart.MTP)), 0);
	return ret;
}

static void s6e63m0_check_id(struct lcd_info *lcd, u8 *idbuf)
{
	if (idbuf[0] == PANEL_A1_SM2)
		lcd->support_elvss = 0;
	else {
		lcd->support_elvss = 1;
		lcd->elvss.reference = idbuf[2] & (BIT(0) | BIT(1) |
					BIT(2) | BIT(3) | BIT(4));
		pr_info("Dynamic ELVSS Information, 0x%x\n",
					lcd->elvss.reference);
	}
}

#ifdef SMART_DIMMING_DEBUG_LCD
static int print_mtp_value(struct lcd_info *lcd)
{
	pr_info("0x%02x, ", lcd->smart.MTP.R_OFFSET.OFFSET_1);
	pr_info("0x%02x, ", lcd->smart.MTP.R_OFFSET.OFFSET_19);
	pr_info("0x%02x, ", lcd->smart.MTP.R_OFFSET.OFFSET_43);
	pr_info("0x%02x, ", lcd->smart.MTP.R_OFFSET.OFFSET_87);
	pr_info("0x%02x, ", lcd->smart.MTP.R_OFFSET.OFFSET_171);
	pr_info("0x%02x, ", lcd->smart.MTP.R_OFFSET.OFFSET_255_MSB);
	pr_info("0x%02x, ", lcd->smart.MTP.R_OFFSET.OFFSET_255_LSB);
	pr_info("0x%02x, ", lcd->smart.MTP.G_OFFSET.OFFSET_1);
	pr_info("0x%02x, ", lcd->smart.MTP.G_OFFSET.OFFSET_19);
	pr_info("0x%02x, ", lcd->smart.MTP.G_OFFSET.OFFSET_43);
	pr_info("0x%02x, ", lcd->smart.MTP.G_OFFSET.OFFSET_87);
	pr_info("0x%02x, ", lcd->smart.MTP.G_OFFSET.OFFSET_171);
	pr_info("0x%02x, ", lcd->smart.MTP.G_OFFSET.OFFSET_255_MSB);
	pr_info("0x%02x, ", lcd->smart.MTP.G_OFFSET.OFFSET_255_LSB);
	pr_info("0x%02x, ", lcd->smart.MTP.B_OFFSET.OFFSET_1);
	pr_info("0x%02x, ", lcd->smart.MTP.B_OFFSET.OFFSET_19);
	pr_info("0x%02x, ", lcd->smart.MTP.B_OFFSET.OFFSET_43);
	pr_info("0x%02x, ", lcd->smart.MTP.B_OFFSET.OFFSET_87);
	pr_info("0x%02x, ", lcd->smart.MTP.B_OFFSET.OFFSET_171);
	pr_info("0x%02x, ", lcd->smart.MTP.B_OFFSET.OFFSET_255_MSB);
	pr_info("0x%02x, ", lcd->smart.MTP.B_OFFSET.OFFSET_255_LSB);
	return 0;
}
#endif

#endif

static int s6e63m0_probe(struct device *dev)
{
	int ret = 0;
	struct lcd_info *lcd;

	lcd = kzalloc(sizeof(struct lcd_info), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	g_lcd = lcd;

	lcd->ld = lcd_device_register("panel", dev, lcd, &s6e63m0_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("panel", dev, lcd,
				&s6e63m0_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		pr_err("failed to register backlight device\n");
		ret = PTR_ERR(lcd->bd);
		goto out_free_backlight;
	}

	lcd->dev = dev;
	lcd->dsim = (struct dsim_global *)dev_get_drvdata(dev->parent);
	lcd->bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd->bd->props.brightness = DEFAULT_BRIGHTNESS;
	lcd->bl = DEFAULT_GAMMA_LEVEL;
	lcd->current_bl = lcd->bl;

	lcd->acl_enable = 0;
	lcd->cur_acl = 0;

	lcd->power = FB_BLANK_UNBLANK;
	lcd->ldi_enable = 1;
	lcd->connected = 1;
	lcd->auto_brightness = 0;

	ret = device_create_file(&lcd->ld->dev, &dev_attr_power_reduce);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n",
					__LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n",
					__LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_gamma_table);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n",
					__LINE__);

	ret = device_create_file(&lcd->bd->dev, &dev_attr_auto_brightness);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n",
					__LINE__);

	dev_set_drvdata(dev, lcd);

	mutex_init(&lcd->lock);
	mutex_init(&lcd->bl_lock);
	/* Start to reading DDI for Smart dimming */
	lcd->panel_select = s5p_dsim_get_panel_sel_value();
	lcd->detected[0] = 0;
#if defined(CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD)
	lcd->detected[1] = 0;
#endif	/* CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD */
	ret += s6e63m0_read_id(lcd, lcd->id);
	if (lcd->id[0] != 0xFE) {
		lcd->detected[lcd->panel_select] = 0;
		goto err_alloc;

	} else
		lcd->detected[lcd->panel_select] = 1;

	dev_info(&lcd->ld->dev, "ID: %x, %x, %x\n", lcd->id[0],
			lcd->id[1], lcd->id[2]);
	dev_info(&lcd->ld->dev, "s6e63m0 driver has been probed.\n");

#ifdef SMART_DIMMING
	s6e63m0_check_id(lcd, lcd->id);
	ret = s6e63m0_read_mtp(lcd);
	if (!ret) {
		pr_info("[LCD:ERROR] : %s read mtp failed\n",
				__func__);
	}
#ifdef SMART_DIMMING_DEBUG_LCD
	ret += print_mtp_value(lcd);
#endif
	ret += Smart_dimming_init(&(lcd->smart));
	if (lcd->support_elvss) {
		ret = alloc_elvss_table(lcd);
		ret = init_elvss_table(lcd);
	}

	if (ret) {
		lcd->elvss_table[lcd->panel_select]
			= (unsigned char **)ELVSS_TABLE;
	}
	ret += alloc_gamma_table(lcd);
	ret += init_gamma_table(lcd);
	if (ret) {
		lcd->gamma_table[lcd->panel_select]
			= (unsigned char **)s6e63m0_gamma22_table;
	}
	/* End reading DDI for Smart dimming */
	update_brightness(lcd, 1);
#endif

#if defined(GPIO_OLED_DET)
	if (lcd->connected) {
		INIT_DELAYED_WORK(&hs_clk_re_try, hs_clk_re_try_work);

		lcd->irq = gpio_to_irq(GPIO_OLED_DET);

		s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
		if (request_irq(lcd->irq, oled_det_int,
					IRQF_TRIGGER_RISING,
					"esd_detection", 0))
			pr_err("failed to reqeust irq. %d\n", lcd->irq);
	}
#endif
	return 0;

out_free_backlight:
	lcd_device_unregister(lcd->ld);
	kfree(lcd);
	return ret;

out_free_lcd:
	kfree(lcd);
	return ret;

err_alloc:
	return ret;
}

static int __devexit s6e63m0_remove(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	s6e63m0_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	backlight_device_unregister(lcd->bd);
	kfree(lcd);

	return 0;
}

/* Power down all displays on reboot, poweroff or halt. */
static void s6e63m0_shutdown(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	dev_info(&lcd->ld->dev, "%s\n", __func__);
	s6e63m0_power(lcd, FB_BLANK_POWERDOWN);
}

static struct mipi_lcd_driver s6e63m0_mipi_driver = {
	.name = "s6e63m0",
	.probe			= s6e63m0_probe,
	.remove			= __devexit_p(s6e63m0_remove),
	.shutdown		= s6e63m0_shutdown,
};

static int s6e63m0_init(void)
{
	return s5p_dsim_register_lcd_driver(&s6e63m0_mipi_driver);
}

static void s6e63m0_exit(void)
{
	return;
}

module_init(s6e63m0_init);
module_exit(s6e63m0_exit);

MODULE_DESCRIPTION("MIPI-DSI S6E63M0:AMS367GEXX (480X800) Panel Driver");
MODULE_LICENSE("GPL");
