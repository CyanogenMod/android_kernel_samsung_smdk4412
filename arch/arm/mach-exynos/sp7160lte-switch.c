#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/semaphore.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>
#include <mach/usb_switch.h>

struct device *sec_switch_dev;

enum usb_path_t current_path = USB_PATH_NONE;

static struct semaphore usb_switch_sem;

static bool usb_connected;

static ssize_t show_usb_sel(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	const char *mode;

	if (current_path & USB_PATH_CP) {
		/* CP */
		mode = "MODEM";
	} else {
		/* AP */
		mode = "PDA";
	}

	pr_info("%s: %s\n", __func__, mode);

	return sprintf(buf, "%s\n", mode);
}

static ssize_t store_usb_sel(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	pr_info("%s: %s\n", __func__, buf);

	if (!strncasecmp(buf, "PDA", 3)) {
		usb_switch_lock();
		usb_switch_clr_path(USB_PATH_CP);
		usb_switch_unlock();
	} else if (!strncasecmp(buf, "MODEM", 5)) {
		usb_switch_lock();
		usb_switch_set_path(USB_PATH_CP);
		usb_switch_unlock();
	} else {
		pr_err("%s: wrong usb_sel value(%s)!!\n", __func__, buf);
		return -EINVAL;
	}

	return count;
}

static ssize_t show_uart_sel(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	int val_sel;
	int val_sel2;
	const char *mode;

	val_sel = gpio_get_value(GPIO_UART_SEL1);
	val_sel2 = gpio_get_value(GPIO_UART_SEL2);

	if (val_sel == 0) {
		/* CP */
		mode = "CP";
	} else {
		if (val_sel2 == 0) {
			/* AP */
			mode = "AP";
		} else {
			/* Keyboard DOCK */
			mode = "DOCK";
		}
	}

	pr_info("%s: %s\n", __func__, mode);

	return sprintf(buf, "%s\n", mode);
}

static ssize_t store_uart_sel(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int uart_sel = -1;
	int uart_sel2 = -1;

	pr_info("%s: %s\n", __func__, buf);

	uart_sel = gpio_get_value(GPIO_UART_SEL1);
	uart_sel2 = gpio_get_value(GPIO_UART_SEL2);

	if (!strncasecmp(buf, "AP", 2)) {
		uart_sel = 1;
		uart_sel2 = 0;
	} else if (!strncasecmp(buf, "CP", 2)) {
		uart_sel = 0;
	} else {
		if (!strncasecmp(buf, "DOCK", 4)) {
			uart_sel = 1;
			uart_sel2 = 1;
		} else {
			pr_err("%s: wrong uart_sel value(%s)!!\n",
					__func__, buf);
			return -EINVAL;
		}
	}

	/* 1 for AP, 0 for CP */
	gpio_set_value(GPIO_UART_SEL1, uart_sel);
	pr_info("%s: uart_sel(%d)\n", __func__, uart_sel);
	/* 1 for (AP)DOCK, 0 for (AP)FAC
	 * KONA rev 0 is 1 for (AP)FAC, 0 for (AP)DOCK */
	gpio_set_value(GPIO_UART_SEL2, uart_sel2);
	pr_info("%s: uart_sel2(%d)\n", __func__, uart_sel2);

	return count;
}

static ssize_t show_usb_state(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	const char *state;

	if (usb_connected)
		state = "USB_STATE_CONFIGURED";
	else
		state = "USB_STATE_NOTCONFIGURED";

	pr_info("%s: %s\n", __func__, state);

	return sprintf(buf, "%s\n", state);
}

static DEVICE_ATTR(usb_sel, 0664, show_usb_sel, store_usb_sel);
static DEVICE_ATTR(uart_sel, 0664, show_uart_sel, store_uart_sel);
static DEVICE_ATTR(usb_state, S_IRUGO, show_usb_state, NULL);

static struct attribute *px_switch_attributes[] = {
	&dev_attr_usb_sel.attr,
	&dev_attr_uart_sel.attr,
	&dev_attr_usb_state.attr,
	NULL
};

static const struct attribute_group px_switch_group = {
	.attrs = px_switch_attributes,
};

void set_usb_connection_state(bool connected)
{
	pr_info("%s: set %s\n", __func__, (connected ? "True" : "False"));

	if (usb_connected != connected) {
		usb_connected = connected;

		pr_info("%s: send \"usb_state\" sysfs_notify\n", __func__);
		sysfs_notify(&sec_switch_dev->kobj, NULL, "usb_state");
	}
}

static void pmic_safeout2(int onoff)
{
	if (onoff) {
		if (!gpio_get_value(GPIO_USB_SEL_CP)) {
			gpio_set_value(GPIO_USB_SEL_CP, onoff);
		} else {
			pr_info("%s: onoff:%d No change in safeout2\n",
			       __func__, onoff);
		}
	} else {
		if (gpio_get_value(GPIO_USB_SEL_CP)) {
			gpio_set_value(GPIO_USB_SEL_CP, onoff);
		} else {
			pr_info("%s: onoff:%d No change in safeout2\n",
			       __func__, onoff);
		}
	}
}

static void usb_apply_path(enum usb_path_t path)
{
	pr_info("%s: current gpio before changing : sel0:%d sel1:%d sel_cp:%d\n",
		__func__, gpio_get_value(GPIO_USB_SEL0),
		gpio_get_value(GPIO_USB_SEL1), gpio_get_value(GPIO_USB_SEL_CP));
	pr_info("%s: target path %x\n", __func__, path);

	/* following checks are ordered according to priority */
	if (path & USB_PATH_ADCCHECK) {
		gpio_set_value(GPIO_USB_SEL0, 1);
		gpio_set_value(GPIO_USB_SEL1, 0);
		goto out_nochange;
	}

	if (path & USB_PATH_TA) {
		gpio_set_value(GPIO_USB_SEL0, 0);
		gpio_set_value(GPIO_USB_SEL1, 0);
		goto out_nochange;
	}

	if (path & USB_PATH_CP) {
		pr_info("DEBUG: set USB path to CP\n");
		gpio_set_value(GPIO_USB_SEL0, 0);
		gpio_set_value(GPIO_USB_SEL1, 1);
		mdelay(3);
		goto out_cp;
	}
	if (path & USB_PATH_AP) {
		gpio_set_value(GPIO_USB_SEL0, 1);
		gpio_set_value(GPIO_USB_SEL1, 1);
		goto out_ap;
	}

	/* default */
	gpio_set_value(GPIO_USB_SEL0, 1);
	gpio_set_value(GPIO_USB_SEL1, 1);

out_ap:
	pr_info("%s: %x safeout2 off\n", __func__, path);
	pmic_safeout2(0);
	goto sysfs_noti;

out_cp:
	pr_info("%s: %x safeout2 on\n", __func__, path);
	pmic_safeout2(1);
	goto sysfs_noti;

out_nochange:
	pr_info("%s: %x safeout2 no change\n", __func__, path);
	return;

sysfs_noti:
	pr_info("%s: send \"usb_sel\" sysfs_notify\n", __func__);
	sysfs_notify(&sec_switch_dev->kobj, NULL, "usb_sel");
	return;
}

/*
  Typical usage of usb switch:

  usb_switch_lock();  (alternatively from hard/soft irq context)
  ( or usb_switch_trylock() )
  ...
  usb_switch_set_path(USB_PATH_ADCCHECK);
  ...
  usb_switch_set_path(USB_PATH_TA);
  ...
  usb_switch_unlock(); (this restores previous usb switch settings)
*/
enum usb_path_t usb_switch_get_path(void)
{
	pr_info("%s: current path(%d)\n", __func__, current_path);

	return current_path;
}

void usb_switch_set_path(enum usb_path_t path)
{
	pr_info("%s: %x current_path before changing\n",
		__func__, current_path);

	current_path |= path;
	usb_apply_path(current_path);
}

void usb_switch_clr_path(enum usb_path_t path)
{
	pr_info("%s: %x current_path before changing\n",
		__func__, current_path);

	current_path &= ~path;
	usb_apply_path(current_path);
}

int usb_switch_lock(void)
{
	return down_interruptible(&usb_switch_sem);
}

int usb_switch_trylock(void)
{
	return down_trylock(&usb_switch_sem);
}

void usb_switch_unlock(void)
{
	up(&usb_switch_sem);
}

static void init_gpio(void)
{
	int uart_sel = -1;
	int uart_sel2 = -1;

	s3c_gpio_cfgpin(GPIO_USB_SEL0, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_USB_SEL0, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_USB_SEL1, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_USB_SEL1, S3C_GPIO_PULL_NONE);

        s3c_gpio_cfgpin(GPIO_USB_SEL_CP, S3C_GPIO_OUTPUT);
        s3c_gpio_setpull(GPIO_USB_SEL_CP, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_UART_SEL1, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_UART_SEL1, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_UART_SEL2, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_UART_SEL2, S3C_GPIO_PULL_NONE);

	uart_sel = gpio_get_value(GPIO_UART_SEL1);
	pr_info("%s: uart_sel(%d)\n", __func__, uart_sel);
	uart_sel2 = gpio_get_value(GPIO_UART_SEL2);
	pr_info("%s: uart_sel2(%d)\n", __func__, uart_sel2);
}

static int __init usb_switch_init(void)
{
	int ret;

	/* USB_SEL gpio_request */
	gpio_request(GPIO_USB_SEL0, "GPIO_USB_SEL0");
	gpio_request(GPIO_USB_SEL1, "GPIO_USB_SEL1");
	gpio_request(GPIO_USB_SEL_CP, "GPIO_USB_SEL_CP");

	/* UART_SEL gpio_request */
	gpio_request(GPIO_UART_SEL1, "GPIO_UART_SEL1");
	gpio_request(GPIO_UART_SEL2, "GPIO_UART_SEL2");

	/* USB_SEL gpio_export */
	gpio_export(GPIO_USB_SEL0, 1);
	gpio_export(GPIO_USB_SEL1, 1);
	gpio_export(GPIO_USB_SEL_CP, 1);

	/* UART_SEL gpio_export */
	gpio_export(GPIO_UART_SEL1, 1);
	gpio_export(GPIO_UART_SEL2, 1);

	BUG_ON(!sec_class);
	sec_switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");

	BUG_ON(!sec_switch_dev);

	/* USB_SEL gpio_export_link */
	gpio_export_link(sec_switch_dev, "GPIO_USB_SEL0", GPIO_USB_SEL0);
	gpio_export_link(sec_switch_dev, "GPIO_USB_SEL1", GPIO_USB_SEL1);
	gpio_export_link(sec_switch_dev, "GPIO_USB_SEL_CP", GPIO_USB_SEL_CP);

	/* UART_SEL gpio_export_link */
	gpio_export_link(sec_switch_dev, "GPIO_UART_SEL1", GPIO_UART_SEL1);
	gpio_export_link(sec_switch_dev, "GPIO_UART_SEL2", GPIO_UART_SEL2);

	/*init_MUTEX(&usb_switch_sem);*/
	sema_init(&usb_switch_sem, 1);

	init_gpio();

	if ((!gpio_get_value(GPIO_USB_SEL0))
			&& (gpio_get_value(GPIO_USB_SEL1))) {
		usb_switch_lock();
		usb_switch_set_path(USB_PATH_CP);
		usb_switch_unlock();
	}

	/* create sysfs group */
	ret = sysfs_create_group(&sec_switch_dev->kobj, &px_switch_group);
	if (ret) {
		pr_err("failed to create px switch attribute group\n");
		return ret;
	}

	return 0;
}

device_initcall(usb_switch_init);
