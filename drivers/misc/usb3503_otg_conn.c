/*
 * drivers/misc/usb3503.c - usb3503 usb hub driver
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/platform_data/usb3503_otg_conn.h>
#include <linux/delay.h>

static struct i2c_client *this_client;
static struct usb3503_platform_data *pdata;
static int current_mode;

/* DEFINE RESISTERs */
#define CFG1_REG		0x06
#define SP_ILOCK_REG		0xE7
#define CFGP_REG		0xEE
#define PDS_REG			0x0A

/*
 * 0x06; only AP (disable CP and WiMax)
 * 0x0A; only CP (disable AP and Wimax)
 * 0x00; enable all port
 *
 * bit :  3   2   1     0
 *       AP, CP, WIMAX, RESERVED
 */
static int hub_port = (0x3 << 2);

static ssize_t mode_show(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size);
static ssize_t port_show(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t port_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size);
int usb3503_register_write(char reg, char data);
int usb3503_register_read(char reg, char *data);
int usb3503_set_mode(int mode);

static DEVICE_ATTR(mode, 0664, mode_show, mode_store);

static ssize_t mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (current_mode == USB_3503_MODE_HUB)
		return sprintf(buf, "%s", "hub");
	else if (current_mode == USB_3503_MODE_STANDBY)
		return sprintf(buf, "%s", "standby");

	return 0;
}

static ssize_t mode_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	if (!strncmp(buf, "hub", 3)) {
		usb3503_set_mode(USB_3503_MODE_HUB);
		pr_debug("usb3503 mode set to hub\n");
	} else if (!strncmp(buf, "standby", 7)) {
		usb3503_set_mode(USB_3503_MODE_STANDBY);
		pr_debug("usb3503 mode set to standby\n");
	}
	return size;
}

static DEVICE_ATTR(port, 0664, port_show, port_store);

static ssize_t port_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "hub_port=0x%x", hub_port);
}

static ssize_t port_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	return 0;
/*
	if (!strncmp(buf, "disable_cp", 10)) {
		hub_port = (0x1 << 2) | hub_port ;
		pr_debug("usb3803 port disable cp\n");
	} else if (!strncmp(buf, "disable_wimax", 13)) {
		hub_port = (0x1 << 1) | hub_port ;
		pr_debug("usb3803 port disable wimaxb\n");
	} else if (!strncmp(buf, "disable_ap", 10)) {
		hub_port = (0x1 << 3) | hub_port ;
		pr_debug("usb3803 port disable ap\n");
	} else if (!strncmp(buf, "enable_cp", 9)) {
		hub_port = ~(0x1 << 2) & hub_port ;
		pr_debug("usb3803 port enable cp\n");
	} else if (!strncmp(buf, "enable_wimax", 12)) {
		hub_port = ~(0x1 << 1) & hub_port ;
		pr_debug("usb3803 port enable wimaxb\n");
	} else if (!strncmp(buf, "enable_ap", 9)) {
		hub_port = ~(0x1 << 3) & hub_port ;
		pr_debug("usb3803 port enable ap\n");
	}

	if(current_mode == USB_3503_MODE_HUB)
		usb3503_set_mode(USB_3503_MODE_HUB);
	pr_debug("usb3503 mode setting (%s)\n", buf);

	return size;
*/
}

int usb3503_register_write(char reg, char data)
{
	int ret;
	char buf[2];
	struct i2c_msg msg[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
		 .len = 2,
		 .buf = buf,
		 },
	};

	buf[0] = reg;
	buf[1] = data;

	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret < 0)
		pr_err("[%s] reg:0x%x data:0x%x write failed\n", \
				__func__, reg, data);

	return ret;
}

int usb3503_register_read(char reg, char *data)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = &reg,
		 },
		{
		 .addr = this_client->addr,
		 .flags = I2C_M_RD,
		 .len = 1,
		 .buf = data,
		 },
	};

	ret = i2c_transfer(this_client->adapter, msgs, 2);

	if (ret < 0)
		pr_err("[%s] 0x%x read failed\n", __func__, reg);
	return ret;
}

int usb3503_set_mode(int mode)
{
	int ret = 0;
	printk("[%s]%d... mode = %d\n", \
		__func__, __LINE__, mode);
	switch (mode) {
	case USB_3503_MODE_HUB:
			pdata->reset_n(0);
			msleep(20);
			/* reset assert */
			pdata->reset_n(1);

			/* there is a major glitch in the
				ES version of USB3803 */
			/* below is the work-around. CS version
				doesn't need this code! */
			if (pdata->es_ver) {
				int i;
				char data;
				msleep(5);

				/* Step 1. Write value 0x03 to register 0xE7
					after RESET_N transitioning to high. */
				for (i = 0; i < 3; i++) {
					/*pr_info("[%s] write SP_ILOCK_REG
						(attempt %d)\n", __func__, i);*/
					usb3503_register_write(SP_ILOCK_REG,
						0x03);
					usb3503_register_read(SP_ILOCK_REG,
						&data);
					/*pr_info("[%s] read  SP_ILOCK_REG :
						0x%x\n", __func__, data);*/

					if (data == 0x3)
						break;
				}
				if (i >= 3) {
				pr_crit("[%s] SP_ILOCK_REG set failed!" \
						"aborted!\n", __func__);
					ret = -1;
					break;
				}
				/* Step 2. Set bit 7 (ClkSusp)
					of register 0xEE to high. */
				/*usb3503_register_read(CFGP_REG, &data);*/
				/*pr_info("[%s] read  CFGP_REG :
					0x%x (default)\n", __func__, data);*/
				data |= 0x80;
				usb3503_register_write(CFGP_REG, data);
				/*usb3503_register_read(CFGP_REG, &data);*/
				/*pr_info("[%s] read  CFGP_REG :
					0x%x\n", __func__, data);*/

				/* Step 2.5
				 * Port Disable For Self Powered Operation
				 */
				/*usb3503_register_read(PDS_REG, &data);*/
				/*pr_info("[%s] read  PDS_REG :
					0x%x (default)\n", __func__, data);*/
				usb3503_register_write(PDS_REG, hub_port);
				/*usb3503_register_read(PDS_REG, &data);*/
				/*pr_info("[%s] hub_port : 0x%x\n",
					__func__, hub_port);*/
				/*pr_info("[%s] read  PDS_REG :
					0x%x\n", __func__, data);*/

				/* Step 3. Set bit 7 (SELF_BUS_PWR)
					of register 0x06 to high. */
				/*usb3503_register_read(CFG1_REG, &data);*/
				/*pr_info("[%s] read  CFG1_REG :
					0x%x (default)\n", __func__, data);*/
				data |= 0x80;
				usb3503_register_write(CFG1_REG, data);
				/*usb3503_register_read(CFG1_REG, &data);*/
				/*pr_info("[%s] read  CFG1_REG :
					0x%x\n", __func__, data);*/

				/* Step 4. Clear bit 0 (config_n) &
					bit 1 (connect_n) of register 0xE7. */
				usb3503_register_write(SP_ILOCK_REG, 0x00);
				/*usb3503_register_read(SP_ILOCK_REG, &data);*/
				/*pr_info("[%s] read  SP_ILOCK_REG :
					0x%x\n", __func__, data);*/
				current_mode = mode;
				}
			break;
	case USB_3503_MODE_STANDBY:
				pdata->reset_n(0);
				current_mode = mode;
			break;

	default:
			pr_err("[%s] Invalid mode %d\n", __func__, mode);
			break;
	}

	return ret;
}
EXPORT_SYMBOL(usb3503_set_mode);

int usb3503_suspend(struct i2c_client *client, pm_message_t mesg)
{
	pdata->port_enable(2, 0);

	pdata->reset_n(0);
	pr_info("USB3503 suspended\n");
	return 0;
}

int usb3503_resume(struct i2c_client *client)
{
	if (current_mode == USB_3503_MODE_HUB)
		pr_info("USB3503 skip hub mode setting\n");
	else
		usb3503_set_mode(current_mode);
	if (current_mode == USB_3503_MODE_HUB)
		pr_info("USB3503 resumed to mode %s\n", "hub");
	else if (current_mode == USB_3503_MODE_STANDBY)
		pr_info("USB3503 resumed to mode %s\n", "standby");
	return 0;
}


int usb3503_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	pdata = client->dev.platform_data;
	if (pdata == NULL) {
		pr_err("USB3503 device's platform data is NULL!\n");
		err = -ENODEV;
		goto exit_pdata_null;
	}

	pdata->hw_config();

	this_client = client;

	if (pdata->init_needed)
		usb3503_set_mode(pdata->inital_mode);

	current_mode = pdata->inital_mode;

	err = device_create_file(&client->dev, &dev_attr_mode);
	err = device_create_file(&client->dev, &dev_attr_port);

	if (current_mode == USB_3503_MODE_HUB)
		printk("USB3503 probed on mode %s\n", "hub");
	else if (current_mode == USB_3503_MODE_STANDBY)
		printk("USB3503 probed on mode %s\n", "standby");

exit_pdata_null:
exit_check_functionality_failed:
	return err;

}

static int usb3503_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id usb3503_id[] = {
	{ USB3503_I2C_NAME, 0 },
	{ }
};


static void usb3503_shutdown(struct i2c_client *client)
{
	printk(KERN_ERR "[usbhub] %s() ...\n", __func__);
	mdelay(10);
	usb3503_set_mode(USB_3503_MODE_STANDBY);
}

static struct i2c_driver usb3503_driver = {
	.probe = usb3503_probe,
	.remove = usb3503_remove,
	.suspend = usb3503_suspend,
	.resume = usb3503_resume,
	.shutdown = usb3503_shutdown,
	.id_table = usb3503_id,
	.driver = {
		.name = USB3503_I2C_NAME,
	},
};

static int __init usb3503_init(void)
{
	pr_info("USB3503 USB HUB driver init\n");
	return i2c_add_driver(&usb3503_driver);
}

static void __exit usb3503_exit(void)
{
	pr_info("USB3503 USB HUB driver exit\n");
	i2c_del_driver(&usb3503_driver);
}

module_init(usb3503_init);
module_exit(usb3503_exit);

MODULE_DESCRIPTION("USB3503 USB HUB driver");
MODULE_LICENSE("GPL");
