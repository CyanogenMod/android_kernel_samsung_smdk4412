/* linux/drivers/video/samsung/s3cfb_lms501xx.c
 *
 * MIPI-DSI based LMS501XX TFT lcd panel driver.
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
#include "lms501xx.h"

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255
#define MAX_GAMMA			255
#define DEFAULT_BRIGHTNESS		160
#define DEFAULT_GAMMA_LEVEL		GAMMA_160CD

#define LDI_ID_REG			0xD1
#define LDI_ID_LEN			3

struct lcd_info {
	unsigned int			bl;
	unsigned int			auto_brightness;
	unsigned int			current_cabc;
	unsigned int			current_bl;

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

	unsigned char			**gamma_table;
	unsigned char			**elvss_table;

	unsigned int			irq;
	unsigned int			connected;

#if defined(GPIO_OLED_DET)
	struct delayed_work		oled_detection;
	unsigned int			oled_detection_count;
#endif
	struct dsim_global		*dsim;
};

static const unsigned int candela_table[GAMMA_MAX] = {
	 30,  40,  50,  60,  70,  80,  90, 100, 110, 120,
	130, 140, 150, 160, 170, 180, 190, 200, 210, 220,
	230, 240, 250, MAX_GAMMA
};

extern void (*lcd_early_suspend)(void);
extern void (*lcd_late_resume)(void);

#if defined(GPIO_OLED_DET)
static void oled_detection_work(struct work_struct *work)
{
	struct lcd_info *lcd =
		container_of(work, struct lcd_info, oled_detection.work);

	int oled_det_level = gpio_get_value(GPIO_OLED_DET);

	dev_info(&lcd->ld->dev, "%s, %d, %d\n",
			__func__, lcd->oled_detection_count, oled_det_level);

	if (!oled_det_level) {
		if (lcd->oled_detection_count < 10) {
			schedule_delayed_work(&lcd->oled_detection, HZ/8);
			lcd->oled_detection_count++;
			set_dsim_hs_clk_toggle_count(15);
		} else
			set_dsim_hs_clk_toggle_count(0);
	} else
		set_dsim_hs_clk_toggle_count(0);

}

static irqreturn_t oled_detection_int(int irq, void *_lcd)
{
	struct lcd_info *lcd = _lcd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->oled_detection_count = 0;
	schedule_delayed_work(&lcd->oled_detection, HZ/16);

	return IRQ_HANDLED;
}
#endif

static int lms501xx_write(struct lcd_info *lcd,
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
		lcd->dsim->ops->cmd_write(lcd->dsim,
				DCS_WR_NO_PARA, wbuf[0], 0);
	else if (size == 2)
		lcd->dsim->ops->cmd_write(lcd->dsim,
				DCS_WR_1_PARA, wbuf[0], wbuf[1]);
	else
		lcd->dsim->ops->cmd_write(lcd->dsim,
				DCS_LONG_WR, (unsigned int)wbuf, size);

	mutex_unlock(&lcd->lock);

	return 0;
}

static int _lms501xx_read(struct lcd_info *lcd,
			const u8 addr, u16 count, u8 *buf)
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

static int lms501xx_read(struct lcd_info *lcd,
			const u8 addr, u16 count, u8 *buf, u8 retry_cnt)
{
	int ret = 0;

read_retry:
	ret = _lms501xx_read(lcd, addr, count, buf);
	if (!ret) {
		if (retry_cnt) {
			printk(KERN_WARNING
				"[WARN:LCD] %s : retry cnt : %d\n",
				__func__, retry_cnt);
			retry_cnt--;
			goto read_retry;
		} else
			printk(KERN_ERR
				"[ERROR:LCD] %s : 0x%02x read failed\n",
				__func__, addr);
	}

	return ret;
}

static int get_backlight_level_from_brightness(int brightness)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is only supported from 0 to 24 */

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

static int lms501xx_gamma_ctl(struct lcd_info *lcd)
{
	SEQ_SET_BL[1] = candela_table[lcd->bl];
	lms501xx_write(lcd, SEQ_SET_BL, ARRAY_SIZE(SEQ_SET_BL));
	lms501xx_write(lcd, SEQ_SET_DISP, ARRAY_SIZE(SEQ_SET_DISP));
	return 0;
}

static int lms501xx_set_cabc(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s - %d\n", __func__, lcd->current_cabc);
	if (lcd->current_cabc)
		lms501xx_write(lcd,
			SEQ_SET_CABC_ON, ARRAY_SIZE(SEQ_SET_CABC_ON));
	else
		lms501xx_write(lcd,
			SEQ_SET_CABC_OFF, ARRAY_SIZE(SEQ_SET_CABC_OFF));

	mdelay(5);

	return ret;
}

static int update_brightness(struct lcd_info *lcd, u8 force)
{
	u32 brightness;

	mutex_lock(&lcd->bl_lock);

	brightness = lcd->bd->props.brightness;

	if (unlikely(!lcd->auto_brightness && brightness > 250))
		brightness = 250;

	lcd->bl = get_backlight_level_from_brightness(brightness);

	if ((force) || ((lcd->ldi_enable) && (lcd->current_bl != lcd->bl))) {

		lms501xx_gamma_ctl(lcd);
		lms501xx_set_cabc(lcd);

		lcd->current_bl = lcd->bl;

		dev_info(&lcd->ld->dev, "brightness=%d, bl=%d, candela=%d\n",
				brightness, lcd->bl, candela_table[lcd->bl]);
	}

	mutex_unlock(&lcd->bl_lock);

	return 0;
}

static int lms501xx_ldi_init(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lms501xx_write(lcd, SEQ_SET_EXTC, ARRAY_SIZE(SEQ_SET_EXTC));
	mdelay(5);
	lms501xx_write(lcd, SEQ_SET_MIPI_DSI, ARRAY_SIZE(SEQ_SET_MIPI_DSI));
	lms501xx_write(lcd, SEQ_SET_GIP, ARRAY_SIZE(SEQ_SET_GIP));
	lms501xx_write(lcd, SEQ_SET_POWER, ARRAY_SIZE(SEQ_SET_POWER));
	mdelay(5);
	lms501xx_write(lcd, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	msleep(125);
	lms501xx_write(lcd, SEQ_SET_RGB, ARRAY_SIZE(SEQ_SET_RGB));
	lms501xx_write(lcd, SEQ_SET_CYC, ARRAY_SIZE(SEQ_SET_CYC));
	lms501xx_write(lcd, SEQ_SET_VCOM, ARRAY_SIZE(SEQ_SET_VCOM));
	lms501xx_write(lcd, SEQ_SET_PTBA, ARRAY_SIZE(SEQ_SET_PTBA));
	lms501xx_write(lcd, SEQ_SET_PANEL, ARRAY_SIZE(SEQ_SET_PANEL));
	lms501xx_write(lcd, SEQ_SET_DGC, ARRAY_SIZE(SEQ_SET_DGC));
	lms501xx_write(lcd, SEQ_SET_STBA, ARRAY_SIZE(SEQ_SET_STBA));
	lms501xx_write(lcd, SEQ_SET_EQ, ARRAY_SIZE(SEQ_SET_EQ));
	lms501xx_write(lcd, SEQ_SET_VCOM_POWER, ARRAY_SIZE(SEQ_SET_VCOM_POWER));
	lms501xx_write(lcd, SEQ_SET_ECO, ARRAY_SIZE(SEQ_SET_ECO));
	lms501xx_write(lcd, SEQ_SET_GAMMA, ARRAY_SIZE(SEQ_SET_GAMMA));
	lms501xx_write(lcd, SEQ_SET_CABC_PWM, ARRAY_SIZE(SEQ_SET_CABC_PWM));

	return ret;
}

static int lms501xx_ldi_enable(struct lcd_info *lcd)
{
	int ret = 0;

	lms501xx_write(lcd, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
	msleep(100);

	return ret;
}

static int lms501xx_ldi_disable(struct lcd_info *lcd)
{
	int ret = 0;

	lms501xx_write(lcd, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	lms501xx_write(lcd, SEQ_SLEEP_IN, ARRAY_SIZE(SEQ_SLEEP_IN));

	return ret;
}

static int lms501xx_power_on(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	ret = lms501xx_ldi_init(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}

	msleep(120);

	ret = lms501xx_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}

	lcd->ldi_enable = 1;

	update_brightness(lcd, 1);
err:
	return ret;
}

static int lms501xx_power_off(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->ldi_enable = 0;

	ret = lms501xx_ldi_disable(lcd);

	msleep(135);

	return ret;
}

static int lms501xx_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = lms501xx_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = lms501xx_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int lms501xx_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return lms501xx_power(lcd, power);
}

static int lms501xx_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

static int lms501xx_check_fb(struct lcd_device *ld, struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;
	struct lcd_info *lcd = lcd_get_data(ld);

	dev_info(&lcd->ld->dev, "%s, fb%d\n", __func__, win->id);

	return 0;
}

static int lms501xx_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	int brightness = bd->props.brightness;
	struct lcd_info *lcd = bl_get_data(bd);

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

static int lms501xx_get_brightness(struct backlight_device *bd)
{
	struct lcd_info *lcd = bl_get_data(bd);

	return candela_table[lcd->bl];
}

static struct lcd_ops lms501xx_lcd_ops = {
	.set_power = lms501xx_set_power,
	.get_power = lms501xx_get_power,
	.check_fb  = lms501xx_check_fb,
};

static const struct backlight_ops lms501xx_backlight_ops  = {
	.get_brightness = lms501xx_get_brightness,
	.update_status = lms501xx_set_brightness,
};

static ssize_t power_reduce_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->current_cabc);
	strcpy(buf, temp);

	return strlen(buf);
}

static ssize_t power_reduce_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = strict_strtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (lcd->current_cabc != value) {
			dev_info(dev, "%s - %d, %d\n",
				 __func__, lcd->current_cabc, value);
			mutex_lock(&lcd->bl_lock);
			lcd->current_cabc = value;
			if (lcd->ldi_enable)
				lms501xx_set_cabc(lcd);
			mutex_unlock(&lcd->bl_lock);
		}
	}
	return size;
}

static DEVICE_ATTR(power_reduce, 0664, power_reduce_show, power_reduce_store);

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[15];
	sprintf(temp, "SMD_LMS501KF06\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0444, lcd_type_show, NULL);

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
				 __func__, lcd->auto_brightness, value);
			mutex_lock(&lcd->bl_lock);
			lcd->auto_brightness = value;
			mutex_unlock(&lcd->bl_lock);
			if (lcd->ldi_enable)
				update_brightness(lcd, 0);
		}
	}
	return size;
}

static DEVICE_ATTR(auto_brightness, 0644,
		auto_brightness_show, auto_brightness_store);

#ifdef CONFIG_HAS_EARLYSUSPEND
struct lcd_info *g_lcd;

void lms501xx_early_suspend(void)
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
	lms501xx_power(lcd, FB_BLANK_POWERDOWN);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

void lms501xx_late_resume(void)
{
	struct lcd_info *lcd = g_lcd;

	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	lms501xx_power(lcd, FB_BLANK_UNBLANK);
#if defined(GPIO_OLED_DET)
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
	enable_irq(lcd->irq);
#endif
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	set_dsim_lcd_enabled(1);

	return ;
}
#endif

static void lms501xx_read_id(struct lcd_info *lcd, u8 *buf)
{
	int ret = 0;

	ret = lms501xx_read(lcd, LDI_ID_REG, LDI_ID_LEN, buf, 3);
	if (!ret) {
		lcd->connected = 0;
		dev_info(&lcd->ld->dev, "panel is not connected well\n");
	}
}

static int lms501xx_probe(struct device *dev)
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

	lcd->ld = lcd_device_register("panel", dev, lcd, &lms501xx_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("panel",
		 dev, lcd, &lms501xx_backlight_ops, NULL);
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
	lcd->current_cabc = 0;

	lcd->power = FB_BLANK_UNBLANK;
	lcd->ldi_enable = 1;
	lcd->connected = 1;
	lcd->auto_brightness = 0;

	ret = device_create_file(&lcd->ld->dev, &dev_attr_power_reduce);
	if (ret < 0)
		dev_err(&lcd->ld->dev,
			 "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev,
			 "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->bd->dev, &dev_attr_auto_brightness);
	if (ret < 0)
		dev_err(&lcd->ld->dev,
			 "failed to add sysfs entries, %d\n", __LINE__);

	dev_set_drvdata(dev, lcd);

	mutex_init(&lcd->lock);
	mutex_init(&lcd->bl_lock);

	dev_info(&lcd->ld->dev, "lms501xx lcd panel driver has been probed.\n");

	update_brightness(lcd, 1);

#if defined(GPIO_OLED_DET)
	if (lcd->connected) {
		INIT_DELAYED_WORK(&lcd->oled_detection, oled_detection_work);

		lcd->irq = gpio_to_irq(GPIO_OLED_DET);

		s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
		if (request_irq(lcd->irq, oled_detection_int,
			IRQF_TRIGGER_FALLING, "oled_detection", lcd))
			pr_err("failed to reqeust irq. %d\n", lcd->irq);
	}
#endif

	lcd_early_suspend = lms501xx_early_suspend;
	lcd_late_resume = lms501xx_late_resume;

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

static int __devexit lms501xx_remove(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	lms501xx_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	backlight_device_unregister(lcd->bd);
	kfree(lcd);

	return 0;
}

/* Power down all displays on reboot, poweroff or halt. */
static void lms501xx_shutdown(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lms501xx_power(lcd, FB_BLANK_POWERDOWN);
}

static struct mipi_lcd_driver lms501xx_mipi_driver = {
	.name = "lms501xx",
	.probe			= lms501xx_probe,
	.remove			= __devexit_p(lms501xx_remove),
	.shutdown		= lms501xx_shutdown,
};

static int lms501xx_init(void)
{
	return s5p_dsim_register_lcd_driver(&lms501xx_mipi_driver);
}

static void lms501xx_exit(void)
{
	return;
}

module_init(lms501xx_init);
module_exit(lms501xx_exit);

MODULE_DESCRIPTION("MIPI-DSI LMS501XX WVGA (480x800) Panel Driver");
MODULE_LICENSE("GPL");
