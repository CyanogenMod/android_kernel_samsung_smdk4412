#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/gpio_event.h>
#include <plat/devs.h>
#include <plat/gpio-cfg.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/input.h>
#include <plat/udc-hs.h>
/*#include <linux/mmc/host.h>*/
#include <linux/regulator/machine.h>
#include <linux/regulator/max8649.h>
#include <linux/regulator/fixed.h>
#include <linux/mfd/wm8994/pdata.h>
#include <linux/mfd/max8997.h>
#include <linux/mfd/max8997-private.h>
/* #include <linux/mfd/max77686.h> */
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#ifdef CONFIG_BATTERY_SAMSUNG
#include <linux/power_supply.h>
#include <linux/battery/samsung_battery.h>
#endif
#include <linux/switch.h>
#include <linux/sii9234.h>

#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif
#include <linux/pm_runtime.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>

#ifdef CONFIG_JACK_MON
#include <linux/jack.h>
#endif

#define MUIC_DEBUG 1
#ifdef MUIC_DEBUG
#define MUIC_PRINT_LOG()	\
	pr_info("MUIC:[%s] func:%s\n", __FILE__, __func__);
#else
#define MUIC_PRINT_LOG()	{}
#endif

static struct switch_dev switch_dock = {
	.name = "dock",
};

extern struct class *sec_class;

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

static int uart_switch_init(void);

static ssize_t u1_switch_show_vbus(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int i;
	struct regulator *regulator;

	regulator = regulator_get(NULL, "safeout1");
	if (IS_ERR(regulator)) {
		pr_warn("%s: fail to get regulator\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	if (regulator_is_enabled(regulator))
		i = sprintf(buf, "VBUS is enabled\n");
	else
		i = sprintf(buf, "VBUS is disabled\n");
	MUIC_PRINT_LOG();
	regulator_put(regulator);

	return i;
}

static ssize_t u1_switch_store_vbus(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int disable, ret, usb_mode;
	struct regulator *regulator;
	/* struct s3c_udc *udc = platform_get_drvdata(&s3c_device_usbgadget); */

	MUIC_PRINT_LOG();
	if (!strncmp(buf, "0", 1))
		disable = 0;
	else if (!strncmp(buf, "1", 1))
		disable = 1;
	else {
		pr_warn("%s: Wrong command\n", __func__);
		return count;
	}

	pr_info("%s: disable=%d\n", __func__, disable);
	usb_mode =
	    disable ? USB_CABLE_DETACHED_WITHOUT_NOTI : USB_CABLE_ATTACHED;
	/* ret = udc->change_usb_mode(usb_mode); */
	ret = -1;
	if (ret < 0)
		pr_err("%s: fail to change mode!!!\n", __func__);

	regulator = regulator_get(NULL, "safeout1");
	if (IS_ERR(regulator)) {
		pr_warn("%s: fail to get regulator\n", __func__);
		return count;
	}

	if (disable) {
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
	} else {
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
	}
	regulator_put(regulator);

	return count;
}

DEVICE_ATTR(disable_vbus, 0664, u1_switch_show_vbus,
	    u1_switch_store_vbus);

#ifdef CONFIG_TARGET_LOCALE_KOR
#include "../../../drivers/usb/gadget/s3c_udc.h"
/* usb access control for SEC DM */
struct device *usb_lock;
static int is_usb_locked;

int u1_switch_get_usb_lock_state(void)
{
	return is_usb_locked;
}
EXPORT_SYMBOL(u1_switch_get_usb_lock_state);

static ssize_t u1_switch_show_usb_lock(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (is_usb_locked)
		return snprintf(buf, PAGE_SIZE, "USB_LOCK");
	else
		return snprintf(buf, PAGE_SIZE, "USB_UNLOCK");
}

static ssize_t u1_switch_store_usb_lock(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int lock;
	struct s3c_udc *udc = platform_get_drvdata(&s3c_device_usbgadget);

	if (!strncmp(buf, "0", 1))
		lock = 0;
	else if (!strncmp(buf, "1", 1))
		lock = 1;
	else {
		pr_warn("%s: Wrong command\n", __func__);
		return count;
	}

	if (IS_ERR_OR_NULL(udc))
		return count;

	pr_info("%s: lock=%d\n", __func__, lock);

	if (lock != is_usb_locked) {
		is_usb_locked = lock;

		if (lock) {
			if (udc->udc_enabled)
				usb_gadget_vbus_disconnect(&udc->gadget);
		}
	}

	return count;
}

static DEVICE_ATTR(enable, 0664,
		   u1_switch_show_usb_lock, u1_switch_store_usb_lock);
#endif

static int __init u1_sec_switch_init(void)
{
	int ret;
	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");

	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");

	ret = device_create_file(switch_dev, &dev_attr_disable_vbus);
	if (ret)
		pr_err("Failed to create device file(disable_vbus)!\n");

#ifdef CONFIG_TARGET_LOCALE_KOR
	usb_lock = device_create(sec_class, switch_dev,
				MKDEV(0, 0), NULL, ".usb_lock");

	if (IS_ERR(usb_lock))
		pr_err("Failed to create device (usb_lock)!\n");

	if (device_create_file(usb_lock, &dev_attr_enable) < 0)
		pr_err("Failed to create device file(.usblock/enable)!\n");
#endif

	ret = uart_switch_init();
	if (ret)
		pr_err("Failed to create uart_switch\n");

	return 0;
};

static int uart_switch_init(void)
{
	int ret, val;
	MUIC_PRINT_LOG();

	ret = gpio_request(GPIO_UART_SEL, "UART_SEL");
	if (ret < 0) {
		pr_err("Failed to request GPIO_UART_SEL!\n");
		return -ENODEV;
	}
	s3c_gpio_setpull(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);
	val = gpio_get_value(GPIO_UART_SEL);
	pr_info("##MUIC [ %s ]- func : %s !! val:-%d-\n", __FILE__, __func__,
		val);
	gpio_direction_output(GPIO_UART_SEL, val);

	gpio_export(GPIO_UART_SEL, 1);

	gpio_export_link(switch_dev, "uart_sel", GPIO_UART_SEL);

	return 0;
}

#if 0
int max77693_muic_charger_cb(enum cable_type_muic cable_type)
{
	MUIC_PRINT_LOG();
	return 0;
}

#define RETRY_CNT_LIMIT 100
/* usb cable call back function */
void max77693_muic_usb_cb(u8 usb_mode)
{
	struct usb_gadget *gadget = platform_get_drvdata(&s3c_device_usbgadget);
#ifdef CONFIG_USB_EHCI_S5P
	struct usb_hcd *ehci_hcd = platform_get_drvdata(&s5p_device_ehci);
#endif
#ifdef CONFIG_USB_OHCI_S5P
	struct usb_hcd *ohci_hcd = platform_get_drvdata(&s5p_device_ohci);
#endif
#ifdef CONFIG_USB_HOST_NOTIFY
	struct host_notifier_platform_data *host_noti_pdata =
	    host_notifier_device.dev.platform_data;
#endif
	int retry_cnt = 1;

	pr_info("MUIC usb_cb:%d\n", usb_mode);
	if (gadget) {
		switch (usb_mode) {
		case USB_CABLE_DETACHED:
			pr_info("usb: muic: USB_CABLE_DETACHED(%d)\n",
				usb_mode);
			usb_gadget_vbus_disconnect(gadget);
			break;
		case USB_CABLE_ATTACHED:
			pr_info("usb: muic: USB_CABLE_ATTACHED(%d)\n",
				usb_mode);
			usb_gadget_vbus_connect(gadget);
			break;
		default:
			pr_info("usb: muic: invalid mode%d\n", usb_mode);
		}
	}

	if (usb_mode == USB_OTGHOST_ATTACHED) {
#ifdef CONFIG_USB_EHCI_S5P
		pm_runtime_get_sync(&s5p_device_ehci.dev);
#endif
#ifdef CONFIG_USB_OHCI_S5P
		pm_runtime_get_sync(&s5p_device_ohci.dev);
#endif
#ifdef CONFIG_USB_HOST_NOTIFY
		host_noti_pdata->ndev.mode = NOTIFY_HOST_MODE;
		if (host_noti_pdata->usbhostd_start)
			host_noti_pdata->usbhostd_start();

		host_noti_pdata->booster(1);
#endif
	} else if (usb_mode == USB_OTGHOST_DETACHED) {
#ifdef CONFIG_USB_EHCI_S5P
		pm_runtime_put_sync(&s5p_device_ehci.dev);
		/* waiting for ehci root hub suspend is done */
		while (ehci_hcd->state != HC_STATE_SUSPENDED) {
			msleep(50);
			if (retry_cnt++ > RETRY_CNT_LIMIT) {
				printk(KERN_ERR "ehci suspend not completed\n");
				break;
			}
		}
#endif
#ifdef CONFIG_USB_OHCI_S5P
		pm_runtime_put_sync(&s5p_device_ohci.dev);
		/* waiting for ohci root hub suspend is done */
		while (ohci_hcd->state != HC_STATE_SUSPENDED) {
			msleep(50);
			if (retry_cnt++ > RETRY_CNT_LIMIT) {
				printk(KERN_ERR
				       "ohci suspend is not completed\n");
				break;
			}
		}
#endif
#ifdef CONFIG_USB_HOST_NOTIFY
		host_noti_pdata->ndev.mode = NOTIFY_NONE_MODE;
		if (host_noti_pdata->usbhostd_stop)
			host_noti_pdata->usbhostd_stop();

		host_noti_pdata->booster(0);
#endif
	}

#ifdef CONFIG_JACK_MON
	if (usb_mode == USB_OTGHOST_ATTACHED)
		jack_event_handler("host", USB_CABLE_ATTACHED);
	else if (usb_mode == USB_OTGHOST_DETACHED)
		jack_event_handler("host", USB_CABLE_DETACHED);
	else if ((usb_mode == USB_CABLE_ATTACHED)
		|| (usb_mode == USB_CABLE_DETACHED))
		jack_event_handler("usb", usb_mode);
#endif
}

/*extern void MHL_On(bool on);*/
void max77693_muic_mhl_cb(int attached)
{
	MUIC_PRINT_LOG();
	pr_info("MUIC attached:%d\n", attached);
	if (attached == MAX77693_MUIC_ATTACHED) {
		/*MHL_On(1);*/ /* GPIO_LEVEL_HIGH */
		pr_info("MHL Attached !!\n");
#ifdef	CONFIG_SAMSUNG_MHL
		sii9234_mhl_detection_sched();
#endif
	} else {
		/*MHL_On(0);*/ /* GPIO_LEVEL_LOW */
		pr_info("MHL Detached !!\n");
	}
}

bool max77693_muic_is_mhl_attached(void)
{
	int val;
	MUIC_PRINT_LOG();
	gpio_request(GPIO_MHL_SEL, "MHL_SEL");
	val = gpio_get_value(GPIO_MHL_SEL);
	pr_info("MUIC val:%d\n", val);
	gpio_free(GPIO_MHL_SEL);

	return !!val;
}

void max77693_muic_deskdock_cb(bool attached)
{
	MUIC_PRINT_LOG();
	pr_info("MUIC deskdock attached=%d\n", attached);
	if (attached)
		switch_set_state(&switch_dock, 1);
	else
		switch_set_state(&switch_dock, 0);
}

void max77693_muic_cardock_cb(bool attached)
{
	MUIC_PRINT_LOG();
	pr_info("MUIC cardock attached=%d\n", attached);
	pr_info("##MUIC [ %s ]- func : %s !!\n", __FILE__, __func__);
	if (attached)
		switch_set_state(&switch_dock, 2);
	else
		switch_set_state(&switch_dock, 0);
}

void max77693_muic_init_cb(void)
{
	int ret;

	/* for CarDock, DeskDock */
	ret = switch_dev_register(&switch_dock);

	MUIC_PRINT_LOG();
	pr_info("MUIC ret=%d\n", ret);

	if (ret < 0)
		pr_err("Failed to register dock switch. %d\n", ret);
}

int max77693_muic_cfg_uart_gpio(void)
{
	int val, path;
	pr_info("## MUIC func : %s ! please  path: (uart:%d - usb:%d)\n",
		__func__, gpio_get_value(GPIO_UART_SEL),
		gpio_get_value(GPIO_USB_SEL));
	val = gpio_get_value(GPIO_UART_SEL);
	path = val ? UART_PATH_AP : UART_PATH_CP;
	pr_info("##MUIC [ %s ]- func : %s !! -- val:%d -- path:%d\n", __FILE__,
		__func__, val, path);

	return path;
}

void max77693_muic_jig_uart_cb(int path)
{
	int val;

	val = path == UART_PATH_AP ? 1 : 0;
	pr_info("##MUIC [ %s ]- func : %s !! -- val:%d\n", __FILE__, __func__,
		val);
	gpio_set_value(GPIO_UART_SEL, val);
}

int max77693_muic_host_notify_cb(int enable)
{
	MUIC_PRINT_LOG();
	pr_info("MUIC host_noti enable=%d\n", enable);
	return 0;
}

int max77693_muic_set_safeout(int path)
{
	struct regulator *regulator;

	MUIC_PRINT_LOG();
	pr_info("MUIC safeout path=%d\n", path);

	if (path == CP_USB_MODE) {
		regulator = regulator_get(NULL, "safeout1");
		if (IS_ERR(regulator))
			return -ENODEV;
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);

		regulator = regulator_get(NULL, "safeout2");
		if (IS_ERR(regulator))
			return -ENODEV;
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
		regulator_put(regulator);
	} else {
		/* AP_USB_MODE || AUDIO_MODE */
		regulator = regulator_get(NULL, "safeout1");
		if (IS_ERR(regulator))
			return -ENODEV;
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
		regulator_put(regulator);

		regulator = regulator_get(NULL, "safeout2");
		if (IS_ERR(regulator))
			return -ENODEV;
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);
	}

	return 0;
}

struct max77693_muic_data max77693_muic = {
	.usb_cb = max77693_muic_usb_cb,
	.charger_cb = max77693_muic_charger_cb,
	.mhl_cb = max77693_muic_mhl_cb,
	.is_mhl_attached = max77693_muic_is_mhl_attached,
	.set_safeout = max77693_muic_set_safeout,
	.init_cb = max77693_muic_init_cb,
	.deskdock_cb = max77693_muic_deskdock_cb,
	.cardock_cb = max77693_muic_cardock_cb,
	.cfg_uart_gpio = max77693_muic_cfg_uart_gpio,
	.jig_uart_cb = max77693_muic_jig_uart_cb,
	.host_notify_cb = max77693_muic_host_notify_cb,
	.gpio_usb_sel = GPIO_USB_SEL,
};
#endif

device_initcall(u1_sec_switch_init);
