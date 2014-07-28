/* linux/drivers/video/samsung/s3cfb_s6d7aa0.c
 *
 * MIPI-DSI based TFT lcd panel driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
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
#include "s6d7aa0_param.h"

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

#define LDI_ID_REG			0xDA
#define LDI_ID_LEN			3

struct lcd_info {
	unsigned int			ldi_enable;
	unsigned int			power;
	struct mutex			lock;
	struct device			*dev;
	struct lcd_device		*ld;
	struct early_suspend		early_suspend;
	unsigned char			id[LDI_ID_LEN];
	unsigned int			connected;

	struct dsim_global		*dsim;

	bool				err_fg_enable;
	unsigned int			err_fg_gpio;
	unsigned int			err_fg_irq;
	struct delayed_work		err_fg_detection;
	int				err_fg_detection_count;	
};


extern void (*lcd_early_suspend)(void);
extern void (*lcd_late_resume)(void);
static int s6d7aa0_power(struct lcd_info *lcd, int power);

static int _s6d7aa0_write(struct lcd_info *lcd, const unsigned char *seq, int len)
{
	int size;
	const unsigned char *wbuf;
	int ret = 0;

	if (!lcd->connected)
		return 0;

	mutex_lock(&lcd->lock);

	size = len;
	wbuf = seq;

	if (size == 1)
		ret = lcd->dsim->ops->cmd_write(lcd->dsim, DCS_WR_NO_PARA, wbuf[0], 0);
	else if (size == 2)
		ret = lcd->dsim->ops->cmd_write(lcd->dsim, DCS_WR_1_PARA, wbuf[0], wbuf[1]);
	else
		ret = lcd->dsim->ops->cmd_write(lcd->dsim, DCS_LONG_WR, (unsigned int)wbuf, size);

	mutex_unlock(&lcd->lock);

	usleep_range(100, 100);

	return ret;
}

static int s6d7aa0_write(struct lcd_info *lcd, const unsigned char *seq, int len)
{
	int ret = 0;
	int retry_cnt = 1;

retry:
	ret = _s6d7aa0_write(lcd, seq, len);
	if (!ret) {
		if (retry_cnt) {
			dev_info(&lcd->ld->dev, "%s :: [0x%02x]retry: %d\n",
					__func__, seq[0], retry_cnt);
			retry_cnt--;
			goto retry;
		} else
			dev_err(&lcd->ld->dev, "%s :: 0x%02x\n", __func__, seq[0]);
	}

	return ret;
}

static int _s6d7aa0_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf)
{
	int ret = 0;

	if (!lcd->connected)
		return ret;

	mutex_lock(&lcd->lock);

	if (lcd->dsim->ops->cmd_read)
		ret = lcd->dsim->ops->cmd_read(lcd->dsim, addr, count, buf);

	mutex_unlock(&lcd->lock);

	usleep_range(100, 100);

	return ret;
}

static int s6d7aa0_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf, u8 retry_cnt)
{
	int ret = 0;

retry:
	ret = _s6d7aa0_read(lcd, addr, count, buf);
	if (!ret) {
		if (retry_cnt) {
			dev_info(&lcd->ld->dev, "%s :: 0x%02x]retry: %d\n",
					__func__, addr, retry_cnt);
			retry_cnt--;
			goto retry;
		} else
			dev_err(&lcd->ld->dev, "%s :: 0x%02x\n", __func__, addr);
	}

	return ret;
}

static void err_fg_detection_work(struct work_struct *work)
{
	struct lcd_info *lcd =
		container_of(work, struct lcd_info, err_fg_detection.work);

	int err_fg_level = gpio_get_value(lcd->err_fg_gpio);

	dev_info(&lcd->ld->dev, "%s, cnt = %d, level = %d\n", __func__,
			lcd->err_fg_detection_count, err_fg_level);

	if (!err_fg_level && lcd->ldi_enable) {
		if (lcd->err_fg_detection_count < 10) {
			schedule_delayed_work(&lcd->err_fg_detection, HZ/8);
			lcd->err_fg_detection_count++;
		} else {
			s6d7aa0_power(lcd, FB_BLANK_POWERDOWN);
			s6d7aa0_power(lcd, FB_BLANK_UNBLANK);
		}
	}

}

static irqreturn_t err_fg_detection_int(int irq, void *_lcd)
{
	struct lcd_info *lcd = _lcd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->err_fg_detection_count = 0;
	schedule_delayed_work(&lcd->err_fg_detection, HZ/16);

	return IRQ_HANDLED;
}

static int s6d7aa0_id00_init(struct lcd_info *lcd)
{
	usleep_range(20000, 20000);
	s6d7aa0_write(lcd, SEQ_PASSWD1, ARRAY_SIZE(SEQ_PASSWD1));
	s6d7aa0_write(lcd, SEQ_PASSWD2, ARRAY_SIZE(SEQ_PASSWD2));
	s6d7aa0_write(lcd, SEQ_PASSWD3, ARRAY_SIZE(SEQ_PASSWD3));

	s6d7aa0_write(lcd, SEQ_PANEL_CTL, ARRAY_SIZE(SEQ_PANEL_CTL));
	s6d7aa0_write(lcd, SEQ_INTERFACE_CTL, ARRAY_SIZE(SEQ_INTERFACE_CTL));
	s6d7aa0_write(lcd, SEQ_OTP_RELOAD, ARRAY_SIZE(SEQ_OTP_RELOAD));
	s6d7aa0_write(lcd, SEQ_DISP_CTL, ARRAY_SIZE(SEQ_DISP_CTL));
	s6d7aa0_write(lcd, SEQ_DAD_CTL, ARRAY_SIZE(SEQ_DAD_CTL));
	s6d7aa0_write(lcd, SEQ_GLOBAL_PARAM, ARRAY_SIZE(SEQ_GLOBAL_PARAM));
	s6d7aa0_write(lcd, SEQ_RFD_CTL, ARRAY_SIZE(SEQ_RFD_CTL));
	s6d7aa0_write(lcd, SEQ_PWR_CTL0, ARRAY_SIZE(SEQ_PWR_CTL0));
	s6d7aa0_write(lcd, SEQ_PWR_CTL1, ARRAY_SIZE(SEQ_PWR_CTL1));
	s6d7aa0_write(lcd, SEQ_PWR_CTL2, ARRAY_SIZE(SEQ_PWR_CTL2));
	s6d7aa0_write(lcd, SEQ_ASG_CTL0, ARRAY_SIZE(SEQ_ASG_CTL0));
	s6d7aa0_write(lcd, SEQ_ASG_CTL1, ARRAY_SIZE(SEQ_ASG_CTL1));
	s6d7aa0_write(lcd, SEQ_ASG_CTL2, ARRAY_SIZE(SEQ_ASG_CTL2));
	s6d7aa0_write(lcd, SEQ_PGAMMA_CTL, ARRAY_SIZE(SEQ_PGAMMA_CTL));
	s6d7aa0_write(lcd, SEQ_NGAMMA_CTL, ARRAY_SIZE(SEQ_NGAMMA_CTL));
	s6d7aa0_write(lcd, SEQ_GOUT_CTL, ARRAY_SIZE(SEQ_GOUT_CTL));

	s6d7aa0_write(lcd, SEQ_BL_ON_CTL, ARRAY_SIZE(SEQ_BL_ON_CTL));
	s6d7aa0_write(lcd, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	msleep(120);

	return 0;
}

static int s6d7aa0_id8c_init(struct lcd_info *lcd)
{
	usleep_range(20000, 20000);

	s6d7aa0_write(lcd, SEQ_PASSWD1, ARRAY_SIZE(SEQ_PASSWD1));
	s6d7aa0_write(lcd, SEQ_PASSWD2, ARRAY_SIZE(SEQ_PASSWD2));
	s6d7aa0_write(lcd, SEQ_OTP_RELOAD, ARRAY_SIZE(SEQ_OTP_RELOAD));
	usleep_range(1000, 1000);
	s6d7aa0_write(lcd, SEQ_B6_PARAM_8_R01, ARRAY_SIZE(SEQ_B6_PARAM_8_R01));

	s6d7aa0_write(lcd, SEQ_BL_ON_CTL, ARRAY_SIZE(SEQ_BL_ON_CTL));
	usleep_range(5000, 5000);
	s6d7aa0_write(lcd, SEQ_DAD_CTL, ARRAY_SIZE(SEQ_DAD_CTL));

	s6d7aa0_write(lcd, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	msleep(120);
	s6d7aa0_write(lcd, SEQ_DAD_CTL_LR, ARRAY_SIZE(SEQ_DAD_CTL_LR));

	return 0;
}

static int s6d7aa0_mtp_init(struct lcd_info *lcd)
{
	usleep_range(20000, 20000);

	s6d7aa0_write(lcd, SEQ_PASSWD1, ARRAY_SIZE(SEQ_PASSWD1));
	s6d7aa0_write(lcd, SEQ_PASSWD2, ARRAY_SIZE(SEQ_PASSWD2));
	s6d7aa0_write(lcd, SEQ_OTP_RELOAD, ARRAY_SIZE(SEQ_OTP_RELOAD));

	s6d7aa0_write(lcd, SEQ_BL_ON_CTL, ARRAY_SIZE(SEQ_BL_ON_CTL));
	s6d7aa0_write(lcd, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	msleep(120);

	return 0;
}

static int s6d7aa0_ldi_init(struct lcd_info *lcd)
{
	dev_info(&lcd->ld->dev, "LDI ID %x\n",lcd->id[1]);

	if (lcd->id[1] == 0x8C)
		s6d7aa0_id8c_init(lcd);
	else if (lcd->id[1] == 0x00)
		s6d7aa0_id00_init(lcd);
	else
		s6d7aa0_mtp_init(lcd);

	return 0;
}

static int s6d7aa0_ldi_enable(struct lcd_info *lcd)
{
	s6d7aa0_write(lcd, SEQ_PASSWD1_LOCK, ARRAY_SIZE(SEQ_PASSWD1_LOCK));
	s6d7aa0_write(lcd, SEQ_PASSWD2_LOCK, ARRAY_SIZE(SEQ_PASSWD2_LOCK));
	/* s6d7aa0_write(lcd, SEQ_TEON_CTL, ARRAY_SIZE(SEQ_TEON_CTL)); */
	s6d7aa0_write(lcd, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));

	return 0;
}

static int s6d7aa0_ldi_disable(struct lcd_info *lcd)
{
	int ret = 0;

	s6d7aa0_write(lcd, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	msleep(64);
	s6d7aa0_write(lcd, SEQ_BL_OFF_CTL, ARRAY_SIZE(SEQ_BL_OFF_CTL));
	usleep_range(1000, 1000);
	s6d7aa0_write(lcd, SEQ_SLEEP_IN, ARRAY_SIZE(SEQ_SLEEP_IN));

	msleep(120);

	return ret;
}

static int s6d7aa0_power_on(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	ret = s6d7aa0_ldi_init(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}

	ret = s6d7aa0_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}

	lcd->ldi_enable = 1;
	if(lcd->err_fg_enable) {
		s3c_gpio_cfgpin(lcd->err_fg_gpio, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(lcd->err_fg_gpio, S3C_GPIO_PULL_UP);
		enable_irq(lcd->err_fg_irq);
	}

err:
	return ret;
}

static int s6d7aa0_power_off(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->ldi_enable = 0;
	if(lcd->err_fg_enable) {
		disable_irq(lcd->err_fg_irq);
		s3c_gpio_cfgpin(lcd->err_fg_gpio, S3C_GPIO_INPUT);
		s3c_gpio_setpull(lcd->err_fg_gpio, S3C_GPIO_PULL_DOWN);
	}

	ret = s6d7aa0_ldi_disable(lcd);

	msleep(135);

	return ret;
}

static int s6d7aa0_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = s6d7aa0_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = s6d7aa0_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int s6d7aa0_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return s6d7aa0_power(lcd, power);
}

static int s6d7aa0_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

static int s6d7aa0_check_fb(struct lcd_device *ld, struct fb_info *fb)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	dev_info(&lcd->ld->dev, "%s, fb%d\n", __func__, fb->node);

	return 0;
}

static struct lcd_ops s6d7aa0_lcd_ops = {
	.set_power = s6d7aa0_set_power,
	.get_power = s6d7aa0_get_power,
	.check_fb  = s6d7aa0_check_fb,
};

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "INH_%02X%02X%02X\n",
				lcd->id[0], lcd->id[1], lcd->id[2]);
}

static DEVICE_ATTR(lcd_type, 0444, lcd_type_show, NULL);

static ssize_t window_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%x %x %x\n",
			lcd->id[0], lcd->id[1], lcd->id[2]);
}

static DEVICE_ATTR(window_type, 0444, window_type_show, NULL);

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct lcd_info *g_lcd;

void s6d7aa0_early_suspend(void)
{
	struct lcd_info *lcd = g_lcd;

	set_dsim_lcd_enabled(0);

	dev_info(&lcd->ld->dev, "+%s\n", __func__);

	s6d7aa0_power(lcd, FB_BLANK_POWERDOWN);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

void s6d7aa0_late_resume(void)
{
	struct lcd_info *lcd = g_lcd;

	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	s6d7aa0_power(lcd, FB_BLANK_UNBLANK);

	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	set_dsim_lcd_enabled(1);

	return ;
}
#endif

static void s6d7aa0_read_id(struct lcd_info *lcd, u8 *buf)
{
	int i, ret = 0;

	for (i = 0; i < LDI_ID_LEN; i++) {
		ret = s6d7aa0_read(lcd, LDI_ID_REG + i, 1, buf + i, 2);
		if (!ret) {
			lcd->connected = 0;
			dev_info(&lcd->ld->dev, "lcd is not connected well\n");
			return;
		}
	}
}

static int s6d7aa0_probe(struct device *dev)
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

	lcd->ld = lcd_device_register("panel", dev, lcd, &s6d7aa0_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->dev = dev;
	lcd->dsim = (struct dsim_global *)dev_get_drvdata(dev->parent);
	lcd->power = FB_BLANK_UNBLANK;
	lcd->ldi_enable = 1;
	lcd->connected = 1;

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_window_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	dev_set_drvdata(dev, lcd);

	mutex_init(&lcd->lock);

	s6d7aa0_read_id(lcd, lcd->id);

	dev_info(&lcd->ld->dev, "ID: %x, %x, %x\n", lcd->id[0], lcd->id[1], lcd->id[2]);

	dev_info(&lcd->ld->dev, "%s lcd panel driver has been probed.\n", dev_name(dev));

#if defined(GPIO_ERR_FG) && defined(CONFIG_S6D7AA0_LSL080AL02)
	if (system_rev >= 0x02) {
		lcd->err_fg_gpio = GPIO_ERR_FG;
		lcd->err_fg_enable = true;
	}
#endif
	if (lcd->connected && lcd->err_fg_enable) {
		s5p_register_gpio_interrupt(lcd->err_fg_gpio);
		s3c_gpio_cfgpin(lcd->err_fg_gpio, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(lcd->err_fg_gpio, S3C_GPIO_PULL_NONE);

		INIT_DELAYED_WORK(&lcd->err_fg_detection, err_fg_detection_work);
		lcd->err_fg_irq = gpio_to_irq(lcd->err_fg_gpio);
		irq_set_irq_type(lcd->err_fg_irq, IRQ_TYPE_EDGE_FALLING);
		if (request_irq(lcd->err_fg_irq, err_fg_detection_int,
			IRQF_TRIGGER_FALLING, "err_fg_detection", lcd))
			pr_err("failed to reqeust irq. %d\n", lcd->err_fg_irq);
	}

	lcd_early_suspend = s6d7aa0_early_suspend;
	lcd_late_resume = s6d7aa0_late_resume;

	return 0;

out_free_lcd:
	kfree(lcd);
	return ret;

err_alloc:
	return ret;
}

static int __devexit s6d7aa0_remove(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	s6d7aa0_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	kfree(lcd);

	return 0;
}

/* Power down all displays on reboot, poweroff or halt. */
static void s6d7aa0_shutdown(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	s6d7aa0_power(lcd, FB_BLANK_POWERDOWN);
}

static struct mipi_lcd_driver s6d7aa0_mipi_driver = {
	.name = "s6d7aa0",
	.probe			= s6d7aa0_probe,
	.remove			= __devexit_p(s6d7aa0_remove),
	.shutdown		= s6d7aa0_shutdown,
};

static int s6d7aa0_init(void)
{
	return s5p_dsim_register_lcd_driver(&s6d7aa0_mipi_driver);
}

static void s6d7aa0_exit(void)
{
	return;
}

module_init(s6d7aa0_init);
module_exit(s6d7aa0_exit);

MODULE_DESCRIPTION("MIPI-DSI S6D7AA0 TFT LCD Panel Driver");
MODULE_LICENSE("GPL");
