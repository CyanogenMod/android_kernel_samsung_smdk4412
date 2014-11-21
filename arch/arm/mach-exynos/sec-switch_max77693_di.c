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
#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>
#include <linux/mfd/max77686.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/gpio.h>

#include <linux/power_supply.h>
#include <linux/battery/samsung_battery.h>

#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif
#include <linux/sii9234.h>

#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif
#include <linux/pm_runtime.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>

#ifdef CONFIG_MACH_MIDAS
#include <linux/platform_data/mms_ts.h>
#endif

#ifdef CONFIG_SWITCH
static struct switch_dev switch_dock = {
	.name = "dock",
};
#endif

extern struct class *sec_class;

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

/* charger cable state */
extern bool is_cable_attached;
bool is_jig_attached;

/* usb cable call back function */
void max77693_muic_usb_cb(u8 usb_mode)
{
	struct usb_gadget *gadget = platform_get_drvdata(&s3c_device_usbgadget);
#ifdef CONFIG_USB_HOST_NOTIFY
	struct host_notifier_platform_data *host_noti_pdata =
	    host_notifier_device.dev.platform_data;
#endif

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

	if (usb_mode == USB_OTGHOST_ATTACHED
		|| usb_mode == USB_POWERED_HOST_ATTACHED) {
#ifdef CONFIG_USB_HOST_NOTIFY
		if (usb_mode == USB_OTGHOST_ATTACHED) {
			host_noti_pdata->booster(1);
			host_noti_pdata->ndev.mode = NOTIFY_HOST_MODE;
			if (host_noti_pdata->usbhostd_start)
				host_noti_pdata->usbhostd_start();
		} else {
			host_noti_pdata->powered_booster(1);
			start_usbhostd_wakelock();
		}
#endif
#ifdef CONFIG_USB_EHCI_S5P
		pm_runtime_get_sync(&s5p_device_ehci.dev);
#endif
#ifdef CONFIG_USB_OHCI_S5P
		pm_runtime_get_sync(&s5p_device_ohci.dev);
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_FAST_BOOT)
		host_noti_pdata->is_host_working = 1;
#endif
	} else if (usb_mode == USB_OTGHOST_DETACHED
		|| usb_mode == USB_POWERED_HOST_DETACHED) {
#ifdef CONFIG_USB_OHCI_S5P
		pm_runtime_put_sync(&s5p_device_ohci.dev);
#endif
#ifdef CONFIG_USB_EHCI_S5P
		pm_runtime_put_sync(&s5p_device_ehci.dev);
#endif
#ifdef CONFIG_USB_HOST_NOTIFY
		if (usb_mode == USB_OTGHOST_DETACHED) {
			host_noti_pdata->ndev.mode = NOTIFY_NONE_MODE;
			if (host_noti_pdata->usbhostd_stop)
				host_noti_pdata->usbhostd_stop();
			host_noti_pdata->booster(0);
		} else {
			host_noti_pdata->powered_booster(0);
			stop_usbhostd_wakelock();
		}
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_FAST_BOOT)
		host_noti_pdata->is_host_working = 0;
#endif
	}
}
EXPORT_SYMBOL(max77693_muic_usb_cb);

int max77693_muic_charger_cb(enum cable_type_muic cable_type)
{
#if !defined(USE_CHGIN_INTR)
#ifdef CONFIG_BATTERY_MAX77693_CHARGER
	struct power_supply *psy = power_supply_get_by_name("max77693-charger");
#endif
#endif

	union power_supply_propval value;
	static enum cable_type_muic previous_cable_type = CABLE_TYPE_NONE_MUIC;
	struct power_supply *psy_p = power_supply_get_by_name("ps");

	pr_info("%s: cable_type(%d), prev_cable(%d)\n", __func__, cable_type, previous_cable_type);

	switch (cable_type) {
	case CABLE_TYPE_NONE_MUIC:
	case CABLE_TYPE_OTG_MUIC:
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
	case CABLE_TYPE_POWER_SHARING_MUIC:
		is_cable_attached = false;
		break;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	case CABLE_TYPE_MHL_MUIC:
		is_cable_attached = false;
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
	case CABLE_TYPE_USB_MUIC:
	case CABLE_TYPE_JIG_USB_OFF_MUIC:
	case CABLE_TYPE_JIG_USB_ON_MUIC:
		is_cable_attached = true;
		break;
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	case CABLE_TYPE_MHL_VB_MUIC:
		is_cable_attached = true;
		break;
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
	case CABLE_TYPE_TA_MUIC:
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
		is_cable_attached = true;
		break;
	default:
		pr_err("%s: invalid type:%d\n", __func__, cable_type);
		return -EINVAL;
	}

	if (previous_cable_type == cable_type) {
		pr_info("%s : SKIP cable setting\n", __func__);
		goto skip_cable_setting;
	}

#if !defined(USE_CHGIN_INTR)
#ifdef CONFIG_BATTERY_MAX77693_CHARGER
	if (!psy || !psy->set_property) {
		pr_err("%s: fail to get max77693-charger psy\n", __func__);
		return 0;
	}

	value.intval = cable_type;
	psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
#endif
#endif

	if (!psy_p || !psy_p->set_property) {
		pr_err("%s: fail to get ps psy\n", __func__);
		return 0;
	}

	if (cable_type == CABLE_TYPE_POWER_SHARING_MUIC) {
		value.intval = POWER_SUPPLY_TYPE_POWER_SHARING;
		psy_p->set_property(psy_p, POWER_SUPPLY_PROP_ONLINE, &value);
	} else {
		if (previous_cable_type == CABLE_TYPE_POWER_SHARING_MUIC) {
			value.intval = POWER_SUPPLY_TYPE_BATTERY;
			psy_p->set_property(psy_p, POWER_SUPPLY_PROP_ONLINE, &value);
		}
	}
	previous_cable_type = cable_type;

skip_cable_setting:
#ifndef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X
	tsp_charger_infom(is_cable_attached);
#endif

	return 0;
}

#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
/*extern void MHL_On(bool on);*/
void max77693_muic_mhl_cb(int attached)
{
	pr_info("MUIC attached:%d\n", attached);
	if (attached == MAX77693_MUIC_ATTACHED) {
		/*MHL_On(1);*/ /* GPIO_LEVEL_HIGH */
		pr_info("MHL Attached !!\n");
#ifdef CONFIG_SAMSUNG_MHL
#ifdef CONFIG_MACH_MIDAS
		sii9234_wake_lock();
#endif
		mhl_onoff_ex(1);
#endif
	} else {
		/*MHL_On(0);*/ /* GPIO_LEVEL_LOW */
		pr_info("MHL Detached !!\n");
#ifdef CONFIG_SAMSUNG_MHL
		mhl_onoff_ex(false);
#ifdef CONFIG_MACH_MIDAS
		sii9234_wake_unlock();
#endif
#endif
	}
}

bool max77693_muic_is_mhl_attached(void)
{
	int val;
#ifdef CONFIG_SAMSUNG_USE_11PIN_CONNECTOR
	val = max77693_muic_get_status1_adc1k_value();
	pr_info("%s(1): %d\n", __func__, val);
	return val;
#else
	int ret;

	ret = gpio_request(GPIO_MHL_SEL, "MHL_SEL");
	if (ret) {
			pr_err("fail to request gpio %s\n", "GPIO_MHL_SEL");
			return -1;
	}
	val = gpio_get_value(GPIO_MHL_SEL);
	pr_info("%s(2): %d\n", __func__, val);
	gpio_free(GPIO_MHL_SEL);
	return !!val;
#endif
}
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */

void max77693_muic_dock_cb(int type)
{
	pr_info("%s:%s= MUIC dock type=%d\n", "sec-switch_di.c", __func__, type);
#ifdef CONFIG_SWITCH
	switch_set_state(&switch_dock, type);
#endif
}

void max77693_muic_init_cb(void)
{
#ifdef CONFIG_SWITCH
	int ret;

	/* for CarDock, DeskDock */
	ret = switch_dev_register(&switch_dock);

	pr_info("MUIC ret=%d\n", ret);

	if (ret < 0)
		pr_err("Failed to register dock switch. %d\n", ret);
#endif
}

int max77693_muic_set_safeout(int path)
{
	struct regulator *regulator;

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

#ifdef CONFIG_USB_HOST_NOTIFY
int max77693_muic_host_notify_cb(int enable)
{
	struct host_notifier_platform_data *host_noti_pdata =
	    host_notifier_device.dev.platform_data;

	struct host_notify_dev *ndev = &host_noti_pdata->ndev;

	if (!ndev) {
		pr_err("%s: ndev is null.\n", __func__);
		return -1;
	}

	ndev->booster = enable ? NOTIFY_POWER_ON : NOTIFY_POWER_OFF;
	pr_info("%s: mode %d, enable %d\n", __func__, ndev->mode, enable);
	return ndev->mode;
}
#endif /* CONFIG_USB_HOST_NOTIFY */

int max77693_get_jig_state(void)
{
	pr_info("%s: %d\n", __func__, is_jig_attached);
	return is_jig_attached;
}
EXPORT_SYMBOL(max77693_get_jig_state);

void max77693_set_jig_state(int jig_state)
{
	pr_info("%s: %d\n", __func__, jig_state);
	is_jig_attached = jig_state;
}

struct max77693_muic_data max77693_muic = {
	.usb_cb = max77693_muic_usb_cb,
	.charger_cb = max77693_muic_charger_cb,
	.dock_cb = max77693_muic_dock_cb,
#if defined(CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION)
	.mhl_cb = max77693_muic_mhl_cb,
	.is_mhl_attached = max77693_muic_is_mhl_attached,
#endif /* CONFIG_MUIC_MAX77693_SUPPORT_MHL_CABLE_DETECTION */
	.init_cb = max77693_muic_init_cb,
	.set_safeout = max77693_muic_set_safeout,
	.gpio_usb_sel = -1,
#ifdef CONFIG_USB_HOST_NOTIFY
	.host_notify_cb = max77693_muic_host_notify_cb,
#endif /* CONFIG_USB_HOST_NOTIFY */
	.jig_state = max77693_set_jig_state,
};

static int __init midas_sec_switch_init(void)
{
	int ret = 0;
	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");

	if (IS_ERR(switch_dev)) {
		pr_err("%s:%s= Failed to create device(switch)!\n",
				__FILE__, __func__);
		return -ENODEV;
	}

	return ret;
}
device_initcall(midas_sec_switch_init);
