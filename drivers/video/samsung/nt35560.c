/*
 * LD9040 AMOLED LCD panel driver.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/lcd.h>
#include <linux/backlight.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "nt35560.h"

#define BOOT_GAMMA_LEVEL	10
#define MAX_GAMMA_LEVEL	25

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define	DEFMASK		0xFF00
#define COMMAND_ONLY		0xFE
#define DATA_ONLY		0xFF

#define BOOT_BRIGHTNESS	122
#define MIN_BRIGHTNESS	0
#define MAX_BRIGHTNESS	255
#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

struct lcd_info  {
	struct device			*dev;
	struct spi_device		*spi;
	unsigned int			power;
	unsigned int			current_bl;
	unsigned int			bl;
	unsigned int			ldi_enable;
	struct mutex			lock;
	struct mutex			bl_lock;
	struct lcd_device		*ld;
	struct backlight_device		*bd;
	struct lcd_platform_data	*lcd_pd;
	struct early_suspend		early_suspend;
};

static int nt35560_spi_write_byte(struct lcd_info *lcd, int addr, int data)
{
	u16 buf[1];
	struct spi_message msg;

	struct spi_transfer xfer = {
		.len		= 2,
		.tx_buf		= buf,
	};

	buf[0] = (addr << 8) | data;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	return spi_sync(lcd->spi, &msg);
}

static int nt35560_spi_write(struct lcd_info *lcd,
	unsigned char address, unsigned char command)
{
	int ret = 0;

	if (address != DATA_ONLY)
		ret = nt35560_spi_write_byte(lcd, 0x0, address);
	if (command != COMMAND_ONLY)
		ret = nt35560_spi_write_byte(lcd, 0x1, command);

	return ret;
}

static int nt35560_panel_send_sequence(struct lcd_info *lcd,
	const unsigned short *seq)
{
	int ret = 0, i = 0;
	const unsigned short *wbuf;

	mutex_lock(&lcd->lock);

	wbuf = seq;

	while ((wbuf[i] & DEFMASK) != ENDDEF) {
		if ((wbuf[i] & DEFMASK) != SLEEPMSEC) {
			ret = nt35560_spi_write(lcd, wbuf[i], wbuf[i+1]);
			if (ret)
				break;
		} else if ((wbuf[i] & DEFMASK) == SLEEPMSEC)
			msleep(wbuf[i+1]);
		i += 2;
	}

	mutex_unlock(&lcd->lock);

	return ret;
}

static int get_backlight_level_from_brightness(unsigned int brightness)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is only supported from 0 to 24 */

	switch (brightness) {
	case 0:
		backlightlevel = 0;
		break;
	case 1 ... 29:
		backlightlevel = 0;
		break;
	case 30 ... 34:
		backlightlevel = 1;
		break;
	case 35 ... 44:
		backlightlevel = 2;
		break;
	case 45 ... 54:
		backlightlevel = 3;
		break;
	case 55 ... 64:
		backlightlevel = 4;
		break;
	case 65 ... 74:
		backlightlevel = 5;
		break;
	case 75 ... 83:
		backlightlevel = 6;
		break;
	case 84 ... 93:
		backlightlevel = 7;
		break;
	case 94 ... 103:
		backlightlevel = 8;
		break;
	case 104 ... 113:
		backlightlevel = 9;
		break;
	case 114 ... 122:
		backlightlevel = 10;
		break;
	case 123 ... 132:
		backlightlevel = 11;
		break;
	case 133 ... 142:
		backlightlevel = 12;
		break;
	case 143 ... 152:
		backlightlevel = 13;
		break;
	case 153 ... 162:
		backlightlevel = 14;
		break;
	case 163 ... 171:
		backlightlevel = 15;
		break;
	case 172 ... 181:
		backlightlevel = 16;
		break;
	case 182 ... 191:
		backlightlevel = 17;
		break;
	case 192 ... 201:
		backlightlevel = 18;
		break;
	case 202 ... 210:
		backlightlevel = 19;
		break;
	case 211 ... 220:
		backlightlevel = 20;
		break;
	case 221 ... 230:
		backlightlevel = 21;
		break;
	case 231 ... 240:
		backlightlevel = 22;
		break;
	case 241 ... 250:
		backlightlevel = 23;
		break;
	case 251 ... 255:
		backlightlevel = 24;
		break;
	default:
		backlightlevel = 24;
		break;
	}
	return backlightlevel;
}

static int nt35560_ldi_init(struct lcd_info *lcd)
{
	int ret, i;
	const unsigned short *init_seq[] = {
		SEQ_SET_PIXEL_FORMAT,
		SEQ_RGBCTRL,
		SEQ_SET_HORIZONTAL_ADDRESS,
		SEQ_SET_VERTICAL_ADDRESS,
		SEQ_SET_ADDRESS_MODE,
		SEQ_SLPOUT,
		SEQ_WRDISBV,
		SEQ_WRCTRLD_1,
		SEQ_WRCABCMB,
		SEQ_WRCTRLD_2,
	};

	for (i = 0; i < ARRAY_SIZE(init_seq); i++) {
		ret = nt35560_panel_send_sequence(lcd, init_seq[i]);
		if (ret)
			break;
	}

	return ret;
}

static int nt35560_ldi_enable(struct lcd_info *lcd)
{
	int ret = 0;

	ret = nt35560_panel_send_sequence(lcd, SEQ_DISPLAY_ON);

	return ret;
}

static int nt35560_ldi_disable(struct lcd_info *lcd)
{
	int ret, i;
	const unsigned short *off_seq[] = {
		SEQ_SET_DISPLAY_OFF,
		SEQ_SLPIN
	};

	lcd->ldi_enable = 0;

	for (i = 0; i < ARRAY_SIZE(off_seq); i++) {
		ret = nt35560_panel_send_sequence(lcd, off_seq[i]);
		if (ret)
			break;
	}

	return ret;
}

static int update_brightness(struct lcd_info *lcd, u8 force)
{
	int ret = 0, brightness;

	mutex_lock(&lcd->bl_lock);

	brightness = lcd->bd->props.brightness;
	lcd->bl = get_backlight_level_from_brightness(brightness);

	if ((lcd->current_bl == lcd->bl) && (!force)) {
		mutex_unlock(&lcd->bl_lock);
		return ret;
	}

	dev_info(&lcd->ld->dev, "brightness=%d, bl=%d\n", brightness, lcd->bl);

	mutex_unlock(&lcd->bl_lock);

	return ret;
}


static int nt35560_power_on(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	if (!pd) {
		dev_err(&lcd->ld->dev, "platform data is NULL.\n");
		return -EFAULT;
	}

	if (!pd->power_on) {
		dev_err(&lcd->ld->dev, "power_on is NULL.\n");
		return -EFAULT;
	} else {
		pd->power_on(lcd->ld, 1);
		msleep(pd->power_on_delay);
	}

	if (!pd->reset) {
		dev_err(&lcd->ld->dev, "reset is NULL.\n");
		return -EFAULT;
	} else {
		pd->reset(lcd->ld);
		msleep(pd->reset_delay);
	}

	ret = nt35560_ldi_init(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}

	ret = nt35560_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}

	lcd->ldi_enable = 1;

	update_brightness(lcd, 1);

err:
	return ret;
}

static int nt35560_power_off(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	pd = lcd->lcd_pd;
	if (!pd) {
		dev_err(&lcd->ld->dev, "platform data is NULL.\n");
		return -EFAULT;
	}

	ret = nt35560_ldi_disable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "lcd setting failed.\n");
		ret = -EIO;
		goto err;
	}

	if (!pd->gpio_cfg_earlysuspend) {
		dev_err(&lcd->ld->dev, "gpio_cfg_earlysuspend is NULL.\n");
		ret = -EFAULT;
		goto err;
	} else
		pd->gpio_cfg_earlysuspend(lcd->ld);

	if (!pd->power_on) {
		dev_err(&lcd->ld->dev, "power_on is NULL.\n");
		ret = -EFAULT;
		goto err;
	} else {
		msleep(pd->power_off_delay);
		pd->power_on(lcd->ld, 0);
	}

err:
	return ret;
}

static int nt35560_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = nt35560_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = nt35560_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int nt35560_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return nt35560_power(lcd, power);
}

static int nt35560_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

static int nt35560_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}

static int nt35560_set_brightness(struct backlight_device *bd)
{
	int ret = 0, brightness = bd->props.brightness;
	struct lcd_info *lcd = bl_get_data(bd);

	if (brightness < MIN_BRIGHTNESS ||
		brightness > bd->props.max_brightness) {
		dev_err(&bd->dev, "lcd brightness should be %d to %d. now %d\n",
			MIN_BRIGHTNESS, MAX_BRIGHTNESS, brightness);
		return -EINVAL;
	}

	if (lcd->ldi_enable) {
		ret = update_brightness(lcd, 0);
		if (ret < 0)
			return -EINVAL;
	}

	return ret;
}

static struct lcd_ops nt35560_lcd_ops = {
	.set_power = nt35560_set_power,
	.get_power = nt35560_get_power,
};

static const struct backlight_ops nt35560_backlight_ops  = {
	.get_brightness = nt35560_get_brightness,
	.update_status = nt35560_set_brightness,
};


static ssize_t lcdtype_show(struct device *dev, struct
device_attribute *attr, char *buf)
{

	char temp[15];
	sprintf(temp, "SMD_AMS427G03\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0664, lcdtype_show, NULL);

#if defined(CONFIG_PM)
#ifdef CONFIG_HAS_EARLYSUSPEND
void nt35560_early_suspend(struct early_suspend *h)
{
	struct lcd_info *lcd = container_of(h, struct lcd_info ,
								early_suspend);
	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	nt35560_power(lcd, FB_BLANK_POWERDOWN);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

void nt35560_late_resume(struct early_suspend *h)
{
	struct lcd_info *lcd = container_of(h, struct lcd_info ,
								early_suspend);
	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	nt35560_power(lcd, FB_BLANK_UNBLANK);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}
#endif
#endif

static int nt35560_probe(struct spi_device *spi)
{
	int ret = 0;
	struct lcd_info *lcd;

	lcd = kzalloc(sizeof(struct lcd_info), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	/* nt35560 lcd panel uses 3-wire 9bits SPI Mode. */
	spi->bits_per_word = 9;

	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "spi setup failed.\n");
		goto out_free_lcd;
	}

	lcd->spi = spi;
	lcd->dev = &spi->dev;

	lcd->lcd_pd = (struct lcd_platform_data *)spi->dev.platform_data;
	if (!lcd->lcd_pd) {
		dev_err(&spi->dev, "platform data is NULL.\n");
		goto out_free_lcd;
	}

	lcd->ld = lcd_device_register("panel", &spi->dev,
		lcd, &nt35560_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("panel", &spi->dev,
		lcd, &nt35560_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		ret = PTR_ERR(lcd->bd);
		goto out_free_backlight;
	}

	lcd->bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd->bd->props.brightness = BOOT_BRIGHTNESS;
	lcd->bl = BOOT_GAMMA_LEVEL;
	lcd->current_bl = lcd->bl;

	ret = device_create_file(&lcd->ld->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries\n");

	mutex_init(&lcd->lock);
	mutex_init(&lcd->bl_lock);

	/*
	 * if lcd panel was on from bootloader like u-boot then
	 * do not lcd on.
	 */
	if (!lcd->lcd_pd->lcd_enabled) {
		/*
		 * if lcd panel was off from bootloader then
		 * current lcd status is powerdown and then
		 * it enables lcd panel.
		 */
		lcd->power = FB_BLANK_POWERDOWN;

		nt35560_power(lcd, FB_BLANK_UNBLANK);
	} else {
		lcd->power = FB_BLANK_UNBLANK;
		lcd->ldi_enable = 1;

		if (!lcd->lcd_pd->power_on) {
			dev_err(lcd->dev, "power_on is NULL.\n");
			goto out_free_backlight;
		} else
			lcd->lcd_pd->power_on(lcd->ld, 1);
	}

	dev_set_drvdata(&spi->dev, lcd);

#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd->early_suspend.suspend = nt35560_early_suspend;
	lcd->early_suspend.resume = nt35560_late_resume;
	lcd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	register_early_suspend(&lcd->early_suspend);
#endif

	dev_info(&lcd->ld->dev, "nt35560 panel driver has been probed.\n");

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

static int __devexit nt35560_remove(struct spi_device *spi)
{
	struct lcd_info *lcd = dev_get_drvdata(&spi->dev);

	nt35560_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	backlight_device_unregister(lcd->bd);
	kfree(lcd);

	return 0;
}

static struct spi_driver nt35560_driver = {
	.driver = {
		.name	= "nt35560",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe		= nt35560_probe,
	.remove		= __devexit_p(nt35560_remove),
};

static int __init nt35560_init(void)
{
	return spi_register_driver(&nt35560_driver);
}

static void __exit nt35560_exit(void)
{
	spi_unregister_driver(&nt35560_driver);
}

module_init(nt35560_init);
module_exit(nt35560_exit);

MODULE_DESCRIPTION("NT35560 TFT Panel Driver");
MODULE_LICENSE("GPL");

