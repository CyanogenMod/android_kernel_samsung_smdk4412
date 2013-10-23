#include <linux/init.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/board-gps.h>

static struct device *gps_dev;

static int __init gps_bcm4752_init(void)
{
	BUG_ON(!sec_class);
	gps_dev = device_create(sec_class, NULL, 0, NULL, "gps");
	BUG_ON(!gps_dev);

	s3c_gpio_cfgpin(GPIO_GPS_RXD, S3C_GPIO_SFN(GPIO_GPS_RXD_AF));
	s3c_gpio_setpull(GPIO_GPS_RXD, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_GPS_TXD, S3C_GPIO_SFN(GPIO_GPS_TXD_AF));
	s3c_gpio_setpull(GPIO_GPS_TXD, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_SFN(GPIO_GPS_CTS_AF));
	s3c_gpio_setpull(GPIO_GPS_CTS, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_SFN(GPIO_GPS_RTS_AF));
	s3c_gpio_setpull(GPIO_GPS_RTS, S3C_GPIO_PULL_NONE);

	if (gpio_request(GPIO_GPS_PWR_EN, "GPS_PWR_EN")) {
		WARN(1, "fail to request gpio (GPS_PWR_EN)\n");
		gpio_free(GPIO_GPS_PWR_EN);
		return 1;
	}

	s3c_gpio_setpull(GPIO_GPS_PWR_EN, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_PWR_EN, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_GPS_PWR_EN, 0);

	gpio_export(GPIO_GPS_PWR_EN, 1);

	gpio_export_link(gps_dev, "GPS_PWR_EN", GPIO_GPS_PWR_EN);

	return 0;
}

device_initcall(gps_bcm4752_init);