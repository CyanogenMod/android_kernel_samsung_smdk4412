/* linux/drivers/video/samsung/s6d6aa1.c
 *
 * MIPI-DSI based AMS529HA01 Super Clear LCD panel driver.
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

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255
#define DEFAULT_BRIGHTNESS		160


struct lcd_info {
	unsigned int			bl;
	unsigned int			current_bl;
	unsigned int			acl_enable;

	unsigned int			ldi_enable;
	unsigned int			power;
	struct mutex			lock;
	struct mutex			bl_lock;

	struct device			*dev;
	struct lcd_device		*ld;
	struct backlight_device		*bd;
	struct lcd_platform_data	*lcd_pd;
	struct early_suspend		early_suspend;

	unsigned int			irq;
	unsigned int			connected;
#if defined(GPIO_OLED_DET)
	struct delayed_work		oled_detection;
	unsigned int			oled_detection_count;

#endif
	struct dsim_global		*dsim;
};

static const unsigned char SEQ_SLPOUT[] = {
	0x11,
	0x00,
	0x00
};

static const unsigned char SEQ_DSCTL[] = {
	0x36,
	0x00,
	0x00
};

static const unsigned char SEQ_WRDISBV[] = {
	0x51,
	0xFF,
	0x00
};

static const unsigned char SEQ_WRCTRLD[] = {
	0x53,
	0x2C,
	0x00
};

static const unsigned char SEQ_WRCABC[] = {
	0x55,
	0x01,
	0x00
};

static const unsigned char SEQ_DISPON[] = {
	0x29,
	0x00,
	0x00
};

static const unsigned char SEQ_DISPOFF[] = {
	0x28,
	0x00,
	0x00
};

static const unsigned char SEQ_SLPIN[] = {
	0x10,
	0x00,
	0x00
};

static unsigned char SEQ_WRDISBV_CTL[] = {
	0x51,
	0xFF,
	0x00
};



#if defined(GPIO_OLED_DET)
static void esd_reset_lcd(struct lcd_info *lcd)
{
	dev_info(&lcd->ld->dev, "++%s\n", __func__);
	s6e8ax0_early_suspend();
	lcd->dsim->ops->suspend();

	lcd->dsim->ops->resume();
	s6e8ax0_late_resume();
	dev_info(&lcd->ld->dev, "--%s\n", __func__);
}

static void oled_detection_work(struct work_struct *work)
{
	struct lcd_info *lcd =
		container_of(work, struct lcd_info, oled_detection.work);

	int oled_det_level = gpio_get_value(GPIO_OLED_DET);

	dev_info(&lcd->ld->dev, "%s, %d, %d\n", __func__, lcd->oled_detection_count, oled_det_level);
	if (!oled_det_level)
		esd_reset_lcd(lcd);
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


static int s6e8ax0_write(struct lcd_info *lcd, const unsigned char *seq, int len)
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

static int _s6e8ax0_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf)
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

static int s6e8ax0_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf, u8 retry_cnt)
{
	int ret = 0;

read_retry:
	ret = _s6e8ax0_read(lcd, addr, count, buf);
	if (!ret) {
		if (retry_cnt) {
			printk(KERN_WARNING "[WARN:LCD] %s : retry cnt : %d\n", __func__, retry_cnt);
			retry_cnt--;
			goto read_retry;
		} else
			printk(KERN_ERR "[ERROR:LCD] %s : 0x%02x read failed\n", __func__, addr);
	}

	return ret;
}

static int get_backlight_level_from_brightness(int brightness)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is only supported from 0 to 24 */

	switch (brightness) {
	case 0 ... 255:
		backlightlevel = brightness;
		break;
	default:
		backlightlevel = brightness;
		break;
	}
	return backlightlevel;
}

static int update_brightness(struct lcd_info *lcd, u8 force)
{
	int ret;
	u32 brightness;

	mutex_lock(&lcd->bl_lock);

	brightness = lcd->bd->props.brightness;

	lcd->bl = get_backlight_level_from_brightness(brightness);

	if ((force) || ((lcd->ldi_enable) && (lcd->current_bl != lcd->bl))) {

		lcd->current_bl = lcd->bl;
		SEQ_WRDISBV_CTL[1] = lcd->bl;
		s6e8ax0_write(lcd, SEQ_WRDISBV_CTL, \
			ARRAY_SIZE(SEQ_WRDISBV_CTL));

		dev_info(&lcd->ld->dev, "brightness=%d, bl=%d\n", brightness, lcd->bl);
	}

	mutex_unlock(&lcd->bl_lock);

	return 0;
}

static int s6e8ax0_ldi_init(struct lcd_info *lcd)
{
	int ret = 0;

	s6e8ax0_write(lcd, SEQ_SLPOUT, ARRAY_SIZE(SEQ_SLPOUT));

	msleep(145);

	s6e8ax0_write(lcd, SEQ_DSCTL, ARRAY_SIZE(SEQ_DSCTL));
	s6e8ax0_write(lcd, SEQ_WRDISBV, ARRAY_SIZE(SEQ_WRDISBV));
	s6e8ax0_write(lcd, SEQ_WRCTRLD, ARRAY_SIZE(SEQ_WRCTRLD));
	s6e8ax0_write(lcd, SEQ_WRCABC, ARRAY_SIZE(SEQ_WRCABC));


	return ret;
}

static int s6e8ax0_ldi_enable(struct lcd_info *lcd)
{
	int ret = 0;

	s6e8ax0_write(lcd, SEQ_DISPON, ARRAY_SIZE(SEQ_DISPON));

	return ret;
}

static int s6e8ax0_ldi_disable(struct lcd_info *lcd)
{
	int ret = 0;

	s6e8ax0_write(lcd, SEQ_DISPOFF, ARRAY_SIZE(SEQ_DISPOFF));
	s6e8ax0_write(lcd, SEQ_SLPIN, ARRAY_SIZE(SEQ_SLPIN));

	return ret;
}

static int s6e8ax0_power_on(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	/* dev_info(&lcd->ld->dev, "%s\n", __func__); */

	ret = s6e8ax0_ldi_init(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}


	ret = s6e8ax0_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}

	lcd->ldi_enable = 1;

	update_brightness(lcd, 1);
err:
	return ret;
}

static int s6e8ax0_power_off(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->ldi_enable = 0;

	ret = s6e8ax0_ldi_disable(lcd);

	msleep(135);

	return ret;
}

static int s6e8ax0_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = s6e8ax0_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = s6e8ax0_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int s6e8ax0_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return s6e8ax0_power(lcd, power);
}

static int s6e8ax0_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

static int s6e8ax0_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	int brightness = bd->props.brightness;
	struct lcd_info *lcd = bl_get_data(bd);

	/* dev_info(&lcd->ld->dev, "%s: brightness=%d\n", __func__, brightness); */

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

static int s6e8ax0_get_brightness(struct backlight_device *bd)
{
	struct lcd_info *lcd = bl_get_data(bd);

	return lcd->bl;
}

static struct lcd_ops panel_lcd_ops = {
	.set_power = s6e8ax0_set_power,
	.get_power = s6e8ax0_get_power,
};

static const struct backlight_ops panel_backlight_ops  = {
	.get_brightness = s6e8ax0_get_brightness,
	.update_status = s6e8ax0_set_brightness,
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

	rc = strict_strtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (lcd->acl_enable != value) {
			dev_info(dev, "%s - %d, %d\n", __func__, lcd->acl_enable, value);
			mutex_lock(&lcd->bl_lock);
			lcd->acl_enable = value;
			if (lcd->ldi_enable)
				/*s6e8ax0_set_acl(lcd);*/
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

	sprintf(temp, "SONY_ACX445BLN\n");

	strcat(buf, temp);

	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0444, lcd_type_show, NULL);


#ifdef CONFIG_HAS_EARLYSUSPEND
struct lcd_info *g_lcd;

void s6e8ax0_early_suspend(void)
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
	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);

	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

void s6e8ax0_late_resume(void)
{
	struct lcd_info *lcd = g_lcd;

	dev_info(&lcd->ld->dev, "+%s\n", __func__);

	s6e8ax0_power(lcd, FB_BLANK_UNBLANK);
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

#if 0
static void s6e8ax0_read_id(struct lcd_info *lcd, u8 *buf)
{
	int ret = 0;

	ret = s6e8ax0_read(lcd, LDI_ID_REG, LDI_ID_LEN, buf, 3);
	if (!ret) {
		lcd->connected = 0;
		dev_info(&lcd->ld->dev, "panel is not connected well\n");
	}
}
#endif

static int s6e8ax0_probe(struct device *dev)
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

	lcd->ld = lcd_device_register("panel", dev, lcd, &panel_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("panel", dev, lcd, &panel_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		pr_err("failed to register backlight device\n");
		ret = PTR_ERR(lcd->bd);
		goto out_free_backlight;
	}

	lcd->dev = dev;
	lcd->dsim = (struct dsim_global *)dev_get_drvdata(dev->parent);
	lcd->bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd->bd->props.brightness = DEFAULT_BRIGHTNESS;
	lcd->bl = 0;
	lcd->current_bl = lcd->bl;

	lcd->acl_enable = 0;

	lcd->power = FB_BLANK_UNBLANK;
	lcd->ldi_enable = 1;
	lcd->connected = 1;

	ret = device_create_file(&lcd->ld->dev, &dev_attr_power_reduce);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	dev_set_drvdata(dev, lcd);

	mutex_init(&lcd->lock);
	mutex_init(&lcd->bl_lock);

	dev_info(&lcd->ld->dev, "s6e8aa0 lcd panel driver has been probed.\n");

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

static int __devexit s6e8ax0_remove(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	backlight_device_unregister(lcd->bd);
	kfree(lcd);

	return 0;
}

/* Power down all displays on reboot, poweroff or halt. */
static void s6e8ax0_shutdown(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);
}

static struct mipi_lcd_driver s6e8ax0_mipi_driver = {
	.name = "s6d6aa1",
	.probe			= s6e8ax0_probe,
	.remove			= __devexit_p(s6e8ax0_remove),
	.shutdown		= s6e8ax0_shutdown,
};

static int s6e8ax0_init(void)
{
	return s5p_dsim_register_lcd_driver(&s6e8ax0_mipi_driver);
}

static void s6e8ax0_exit(void)
{
	return;
}

module_init(s6e8ax0_init);
module_exit(s6e8ax0_exit);

MODULE_DESCRIPTION("MIPI-DSI s6d6aa1:SCLCD (720x1280) Panel Driver");
MODULE_LICENSE("GPL");

