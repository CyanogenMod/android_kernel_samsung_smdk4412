/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <asm/gpio.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/earlysuspend.h>
#include <asm/io.h>
#include <mach/gpio.h>
#include <mach/irqs.h>
#include <mach/gpio-midas.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

#ifdef CONFIG_CPU_FREQ
#include <mach/cpufreq.h>
#endif

/*
Melfas touchkey register
*/
#define KEYCODE_REG 0x00
#define FIRMWARE_VERSION 0x01
#define TOUCHKEY_MODULE_VERSION 0x03
#define TOUCHKEY_ADDRESS	0x20

#define UPDOWN_EVENT_BIT 0x08
#define KEYCODE_BIT 0x07
#define ESD_STATE_BIT 0x10
#define I2C_M_WR 0 /* for i2c */

//#define IRQ_TOUCH_INT (IRQ_EINT_GROUP22_BASE + 1)
#define DEVICE_NAME "melfas-touchkey"
#define INT_PEND_BASE	0xE0200A54

#define MCS5080_CHIP		0x03
#define MCS5080_last_ver	0x01  /*w999 v19*/

// if you want to see log, set this definition to NULL or KERN_WARNING
#define TCHKEY_KERN_DEBUG      KERN_DEBUG

#define _3_TOUCH_SDA_28V GPIO_3_TOUCH_SDA
#define _3_TOUCH_SCL_28V GPIO_3_TOUCH_SCL
#define _3_GPIO_TOUCH_EN	GPIO_3_TOUCH_EN
#define _3_GPIO_TOUCH_INT	GPIO_3_TOUCH_INT
#define IRQ_TOUCH_INT gpio_to_irq(GPIO_3_TOUCH_INT)



#define TOUCHKEY_KEYCODE_MENU 	KEY_MENU
#define TOUCHKEY_KEYCODE_HOME 	KEY_HOME
#define TOUCHKEY_KEYCODE_BACK 	KEY_BACK
#define TOUCHKEY_KEYCODE_SEARCH 	KEY_END
#define FLIP_CLOSE 0
#define FLIP_OPEN 1

static int touchkey_keycode[5] = {0,KEY_MENU , KEY_HOME, KEY_BACK, 0};
static unsigned int HWREV=7;
static u8 activation_onoff = 1;	// 0:deactivate   1:activate
static u8 is_suspending = 0;
static u8 user_press_on = 0;
static u8 touchkey_dead = 0;
static u8 menu_sensitivity = 0;
static u8 back_sensitivity = 0;
static u8 home_sensitivity = 0;

static int touchkey_enable = 0;

static u8 version_info[3];
static void	__iomem	*gpio_pend_mask_mem;
static int Flip_status=FLIP_OPEN;

struct i2c_touchkey_driver {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
	struct early_suspend	early_suspend;
};
struct i2c_touchkey_driver *touchkey_driver = NULL;
struct workqueue_struct *touchkey_wq;
extern struct class *sec_class;
struct device *sec_touchkey;

static const struct i2c_device_id melfas_touchkey_id[] = {
	{"melfas-touchkey", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, melfas_touchkey_id);

extern void get_touchkey_data(u8 *data, u8 length);
static void init_hw(void);
static int touchkey_pmic_control(int onoff);
static int i2c_touchkey_probe(struct i2c_client *client, const struct i2c_device_id *id);
static void melfas_touchkey_switch_early_suspend(int FILP_STATE);
static void melfas_touchkey_switch_early_resume(int FILP_STATE);


struct i2c_driver touchkey_i2c_driver =
{
	.driver = {
		   .name = "melfas-touchkey",
	},
	.id_table = melfas_touchkey_id,
	.probe = i2c_touchkey_probe,
};


static int i2c_touchkey_read(u8 reg, u8 *val, unsigned int len)
{
	int	err;
	int	retry = 3;
	struct i2c_msg	msg[1];

	if((touchkey_driver == NULL)||touchkey_dead)
	{
		return -ENODEV;
	}

	while(retry--)
	{
		msg->addr	= touchkey_driver->client->addr;
		msg->flags = I2C_M_RD;
		msg->len   = len;
		msg->buf   = val;
		err = i2c_transfer(touchkey_driver->client->adapter, msg, 1);

		if (err >= 0)
		{
			return 0;
		}
		printk(KERN_ERR "%s %d i2c transfer error\n", __func__, __LINE__);/* add by inter.park */
		mdelay(10);
	}

	return err;

}

static int i2c_touchkey_write(u8 *val, unsigned int len)
{
	int err;
	struct i2c_msg msg[1];
	unsigned char data[2];

	if((touchkey_driver == NULL)||is_suspending||touchkey_dead)
	{
		return -ENODEV;
	}

	data[0] = *val;
	msg->addr = touchkey_driver->client->addr;
	msg->flags = I2C_M_WR;
	msg->len = len;
	msg->buf = data;

	err = i2c_transfer(touchkey_driver->client->adapter, msg, 1);

	if (err >= 0) return 0;

	printk(KERN_ERR "%s %d i2c transfer error\n", __func__, __LINE__);

	return err;
}

static unsigned int touch_state_val;
//extern unsigned int touch_state_val;
extern void TSP_forced_release(void);
void  touchkey_work_func(struct work_struct * p)
{
	u8 data[14];
	int keycode;
	int retry = 10;
	if (Flip_status == FLIP_CLOSE) {
		if (!gpio_get_value(_3_GPIO_TOUCH_INT) && !touchkey_dead) {
			i2c_touchkey_read(KEYCODE_REG, data, 14);

			keycode = touchkey_keycode[data[0] & KEYCODE_BIT];
		if(activation_onoff){
			if(data[0] & UPDOWN_EVENT_BIT) // key released
			{
				user_press_on = 0;
				input_report_key(touchkey_driver->input_dev, touchkey_keycode[data[0] &  KEYCODE_BIT], 0);
				input_sync(touchkey_driver->input_dev);
			}
			else // key pressed
			{
				if(touch_state_val == 1)
				{
					printk(TCHKEY_KERN_DEBUG "touchkey pressed but don't send event because touch is pressed. \n");
				}
				else
				{
					if(keycode == TOUCHKEY_KEYCODE_BACK)
					{
						user_press_on = 3;
						back_sensitivity = data[5];
					}
					else if(keycode == TOUCHKEY_KEYCODE_MENU)
					{
						user_press_on = 1;
						menu_sensitivity = data[3];
					}
					else if(keycode == TOUCHKEY_KEYCODE_HOME)
					{
						user_press_on = 2;
						home_sensitivity = data[4];
					}
					input_report_key(touchkey_driver->input_dev, keycode,1);
						input_sync(touchkey_driver->input_dev);
				}
			}
		}
	}
	else
		printk(KERN_ERR "touchkey interrupt line is high!\n");
	}

	else
	i2c_touchkey_read(KEYCODE_REG, data, 14);

	enable_irq(IRQ_TOUCH_INT);
	return ;
}

static irqreturn_t touchkey_interrupt(int irq, void *dummy)
{
	printk("TKEY interrupt call\n");
	disable_irq_nosync(IRQ_TOUCH_INT);
	queue_work(touchkey_wq, &touchkey_driver->work);
	
	return IRQ_HANDLED;
}

void samsung_switching_tkey(int flip)
{
	u8 data[6];
	int ret;

	if (touchkey_driver == NULL)
		return;

	printk(KERN_ERR "[TKEY] samsung_switching_tkey, Flip_status : %d, flip : %d \n", Flip_status, flip);
	
	if (Flip_status != flip)
	{
		Flip_status=flip;
		if (flip == FLIP_CLOSE) {
			touchkey_enable = 1;
			melfas_touchkey_switch_early_resume(flip);
			}
		else {
			touchkey_enable = 0;
			melfas_touchkey_switch_early_suspend(flip);
	}
	}

}
EXPORT_SYMBOL(samsung_switching_tkey);

static void melfas_touchkey_switch_early_suspend(int FILP_STATE){

	unsigned char data;
	
	if (touchkey_driver == NULL)
		return;

	data = 0x02;
	i2c_touchkey_write(&data, 1);	//Key LED force off

	printk(KERN_DEBUG "melfas_touchkey_switch_early_suspend, %d\n", FILP_STATE);
	if (touchkey_enable < 0) {
		printk("---%s---touchkey_enable: %d\n", __FUNCTION__,
			   touchkey_enable);
		return;
	}
	gpio_direction_output(_3_GPIO_TOUCH_EN, 0);
	gpio_direction_output(_3_TOUCH_SDA_28V, 0);
	gpio_direction_output(_3_TOUCH_SCL_28V, 0);
	if (system_rev >= 14)
		touchkey_pmic_control(0);

	disable_irq(IRQ_TOUCH_INT);

#ifdef USE_IRQ_FREE
	free_irq(IRQ_TOUCH_INT, NULL);
#endif

}

static void melfas_touchkey_switch_early_resume(int FILP_STATE){
	int err = 0;
	u8 data[14];
	int retry =5;
	u8 data1 = 0x01;
	if (touchkey_driver == NULL)
       return;

	if (Flip_status == FLIP_OPEN)
		return;

	printk(KERN_ERR "%d\n", FILP_STATE);

	i2c_touchkey_write(&data1, 1);
	
	init_hw();

#ifdef USE_IRQ_FREE
	msleep(50);


	printk("%s, %d\n",__func__, __LINE__);
	err = request_threaded_irq(IRQ_TOUCH_INT, NULL, touchkey_interrupt,
			IRQF_DISABLED | IRQF_TRIGGER_LOW | IRQF_ONESHOT, "touchkey_int", NULL);
	if (err) {
		printk(KERN_ERR "%s Can't allocate irq .. %d\n", __FUNCTION__, err);
	}
#endif

	//force raed (for INT high) before enable_irq
	while(!(gpio_get_value(GPIO_3_TOUCH_INT))){
		i2c_touchkey_read(KEYCODE_REG, data, 1);
		printk("%s : INT high force read , data = %d\n",__func__,data[0]);
		mdelay(5);
		if(retry<0) break;
		retry--;
	}

	enable_irq(IRQ_TOUCH_INT);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_touchkey_early_suspend(struct early_suspend *h)
{
    pr_info("melfas_touchkey_early_suspend +++\n");
	touchkey_enable = 0;
	is_suspending = 1;
	if(user_press_on==1)
	{
		input_report_key(touchkey_driver->input_dev, TOUCHKEY_KEYCODE_MENU, 0);
//		printk(TCHKEY_KERN_DEBUG "%s release menu key\n",__func__);
	}
	else if(user_press_on==2)
	{
		input_report_key(touchkey_driver->input_dev, TOUCHKEY_KEYCODE_BACK, 0);
//		printk(TCHKEY_KERN_DEBUG "%s release back key\n",__func__);
	}
	user_press_on = 0;

	if(touchkey_dead)
	{
		printk(KERN_ERR "touchkey died after ESD");
		return;
	}

	disable_irq(IRQ_TOUCH_INT);
	gpio_direction_output(_3_GPIO_TOUCH_EN, 0);
	gpio_direction_output(_3_TOUCH_SDA_28V, 0);
	gpio_direction_output(_3_TOUCH_SCL_28V, 0);
	if (system_rev >= 14)
		touchkey_pmic_control(0);
    pr_info("melfas_touchkey_early_suspend ---\n");
}

static void melfas_touchkey_early_resume(struct early_suspend *h)
{
    pr_info("melfas_touchkey_early_resume +++\n");
	touchkey_enable = 1;
	if(touchkey_dead)
	{
		printk(KERN_ERR "touchkey died after ESD");
		return;
	}

	init_hw();
#if 0
	//clear interrupt
	if(readl(gpio_pend_mask_mem)&(0x1<<1))
		writel(readl(gpio_pend_mask_mem)|(0x1<<1), gpio_pend_mask_mem);
#endif
	enable_irq(IRQ_TOUCH_INT);
	is_suspending = 0;

    pr_info("melfas_touchkey_early_resume ---\n");
}
#endif	// End of CONFIG_HAS_EARLYSUSPEND

extern int mcsdl_download_binary_data(u8 chip_ver);
//extern int mcsdl_download_binary_file(unsigned char *pData, unsigned short nBinary_length);
static int i2c_touchkey_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct input_dev *input_dev;
	int err = 0;

	printk("melfas touchkey probe called!\n");
	touchkey_driver = kzalloc(sizeof(struct i2c_touchkey_driver), GFP_KERNEL);
	if (touchkey_driver == NULL)
	{
		dev_err(dev, "failed to create our state\n");
		return -ENOMEM;
	}

	touchkey_driver->client = client;

	touchkey_driver->client->irq = IRQ_TOUCH_INT;
	strlcpy(touchkey_driver->client->name, "melfas-touchkey", I2C_NAME_SIZE);

	input_dev = input_allocate_device();

	if (!input_dev)
		return -ENOMEM;

	touchkey_driver->input_dev = input_dev;

	input_dev->name = DEVICE_NAME;
	input_dev->phys = "melfas-touchkey/input0";
	input_dev->id.bustype = BUS_HOST;

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(touchkey_keycode[1], input_dev->keybit);
	set_bit(touchkey_keycode[2], input_dev->keybit);
	set_bit(touchkey_keycode[3], input_dev->keybit);
	set_bit(touchkey_keycode[4], input_dev->keybit);

	err = input_register_device(input_dev);
	if (err)
	{
		input_free_device(input_dev);
		return err;
	}

	gpio_pend_mask_mem = ioremap(INT_PEND_BASE, 0x10);
	touchkey_wq = create_singlethread_workqueue("melfas_touchkey_wq");
	if (!touchkey_wq)
		return -ENOMEM;

	INIT_WORK(&touchkey_driver->work, touchkey_work_func);

#ifdef CONFIG_HAS_EARLYSUSPEND
	touchkey_driver->early_suspend.suspend = melfas_touchkey_early_suspend;
	touchkey_driver->early_suspend.resume = melfas_touchkey_early_resume;
	register_early_suspend(&touchkey_driver->early_suspend);
#endif				/* CONFIG_HAS_EARLYSUSPEND */
	touchkey_enable = 1;
	if (request_irq(IRQ_TOUCH_INT, touchkey_interrupt, IRQF_DISABLED, DEVICE_NAME, touchkey_driver))
	{
		printk(KERN_ERR "%s Can't allocate irq ..\n", __FUNCTION__);
		return -EBUSY;
	}
	return 0;
}
static int touchkey_pmic_control(int onoff)
{
	struct regulator *regulator;
	regulator = regulator_get(NULL, "TOUCH_PULL_UP");

	if (onoff) {
		regulator_enable(regulator);
		printk(KERN_INFO "[touchkey]touchkey_pmic_control on\n");
		}
	else {
		regulator_disable(regulator);
		printk(KERN_INFO "[touchkey]touchkey_pmic_control off\n");
		}
	regulator_put(regulator);
	return 0;
}
static void init_hw(void)
{
	if (system_rev >= 14)
		touchkey_pmic_control(1);

	gpio_set_value(_3_GPIO_TOUCH_EN, 1);
	msleep(100);
	s3c_gpio_setpull(_3_GPIO_TOUCH_INT, S3C_GPIO_PULL_UP);
	irq_set_irq_type(IRQ_TOUCH_INT, IRQF_TRIGGER_FALLING);
	s3c_gpio_cfgpin(_3_GPIO_TOUCH_INT, S3C_GPIO_SFN(0xf));
}


int touchkey_update_open (struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t touchkey_update_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

ssize_t touchkey_update_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	return count;
}

int touchkey_update_release (struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t touchkey_activation_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	printk(TCHKEY_KERN_DEBUG "called %s\n", __func__);
	sscanf(buf, "%hhu", &activation_onoff);
	printk(TCHKEY_KERN_DEBUG "deactivation test = %d\n", activation_onoff);
	return size;
}

static ssize_t touchkey_version_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk(TCHKEY_KERN_DEBUG "called %s\n", __func__);
	return sprintf(buf,"%02x\n",version_info[1]);
}

static ssize_t touchkey_recommend_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	u8 recommended_ver;
	printk(TCHKEY_KERN_DEBUG "called %s\n", __func__);
	if (version_info[2] == MCS5080_CHIP)
		recommended_ver = MCS5080_last_ver;
	else
		recommended_ver = version_info[1];

	return sprintf(buf,"%02x\n",recommended_ver);
}

static ssize_t touchkey_firmup_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk(TCHKEY_KERN_DEBUG "Touchkey firm-up start!\n");

	if (version_info[2] == MCS5080_CHIP)
		mcsdl_download_binary_data(MCS5080_CHIP);
	else
		printk(KERN_ERR "Touchkey IC module is old, can't update!");

	get_touchkey_data(version_info, 3);
	printk(TCHKEY_KERN_DEBUG "Updated F/W version: 0x%x, Module version:0x%x\n", version_info[1], version_info[2]);
	return sprintf(buf,"%02x\n",version_info[1]);
}

static ssize_t touchkey_init_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk(TCHKEY_KERN_DEBUG "called %s \n", __func__);
	return sprintf(buf, "%d\n",touchkey_dead);
}

static ssize_t touchkey_menu_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk(TCHKEY_KERN_DEBUG "called %s \n", __func__);
	return sprintf(buf, "%d\n",menu_sensitivity);
}
static ssize_t touchkey_home_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk(TCHKEY_KERN_DEBUG "called %s \n", __func__);
	return sprintf(buf, "%d\n",home_sensitivity);
}

static ssize_t touchkey_back_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk(TCHKEY_KERN_DEBUG "called %s \n", __func__);
	return sprintf(buf, "%d\n",back_sensitivity);
}

static ssize_t touch_led_control(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	u8 data;
	if (!touchkey_enable)
		return -1;

	sscanf(buf, "%hhu", &data);
	i2c_touchkey_write(&data, 1);	// LED on(data=1) or off(data=2)
	return size;
}

static ssize_t touchkey_enable_disable(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	// this function is called when platform shutdown thread begins
	printk(TCHKEY_KERN_DEBUG "called %s %c \n",__func__, *buf);
	if(*buf == '0')
	{
		is_suspending = 1;
	    disable_irq(IRQ_TOUCH_INT);
	    gpio_direction_output(_3_GPIO_TOUCH_EN, 0);
	}
	else
	{
	    printk(KERN_ERR "%s: unknown command %c \n",__func__, *buf);
	}

        return size;
}

static DEVICE_ATTR(touchkey_activation, 0664, NULL, touchkey_activation_store);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO, touchkey_version_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO, touchkey_recommend_show, NULL);
static DEVICE_ATTR(recommended_version, S_IRUGO, touchkey_recommend_show, NULL);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO, touchkey_firmup_show, NULL);
static DEVICE_ATTR(touchkey_init, S_IRUGO, touchkey_init_show, NULL);
static DEVICE_ATTR(touchkey_menu, S_IRUGO, touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, touchkey_back_show, NULL);
static DEVICE_ATTR(touchkey_home, S_IRUGO, touchkey_home_show, NULL);
static DEVICE_ATTR(brightness, 0664, NULL, touch_led_control);
static DEVICE_ATTR(enable_disable, 0664, NULL, touchkey_enable_disable);

static struct attribute *touchkey_attributes[] = {
	&dev_attr_touchkey_activation.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_recommended_version.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_touchkey_init.attr,
	&dev_attr_touchkey_menu.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_home.attr,
	&dev_attr_brightness.attr,
	&dev_attr_enable_disable.attr,
	NULL,
};


static int __init touchkey_init(void)
{

	int ret = 0;

	u8 updated = 0;
	if ((ret = gpio_request(_3_GPIO_TOUCH_EN, "_3_GPIO_TOUCH_EN")))
		printk(KERN_ERR "Failed to request gpio %s:%d\n", __func__, __LINE__);

	sec_touchkey=device_create(sec_class, NULL, 0, NULL, "sec_touchkey");

	if (device_create_file(sec_touchkey, &dev_attr_touchkey_activation) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_activation.attr.name);

	if (device_create_file(sec_touchkey, &dev_attr_touchkey_firm_version_panel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_firm_version_panel.attr.name);

	if (device_create_file(sec_touchkey, &dev_attr_touchkey_firm_version_phone) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_firm_version_phone.attr.name);

	if (device_create_file(sec_touchkey, &dev_attr_recommended_version) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_recommended_version.attr.name);

	if (device_create_file(sec_touchkey, &dev_attr_touchkey_firm_update) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_firm_update.attr.name);

	if (device_create_file(sec_touchkey, &dev_attr_touchkey_init) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_init.attr.name);

	if (device_create_file(sec_touchkey, &dev_attr_touchkey_menu) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_menu.attr.name);

	if (device_create_file(sec_touchkey, &dev_attr_touchkey_back) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_back.attr.name);

	if (device_create_file(sec_touchkey, &dev_attr_touchkey_home) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_home.attr.name);

	if (device_create_file(sec_touchkey, &dev_attr_brightness) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_brightness.attr.name);

	if (device_create_file(sec_touchkey, &dev_attr_enable_disable) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_enable_disable.attr.name);

	init_hw();

	get_touchkey_data(version_info, 3);
	printk(TCHKEY_KERN_DEBUG "%s F/W version: 0x%x, Module version:0x%x\n",__FUNCTION__, version_info[1], version_info[2]);

//-------------------   Auto Firmware Update Routine Start   -------------------//
		if (version_info[1] == 0xff) {
			mcsdl_download_binary_data(MCS5080_CHIP);
			updated = 1;
		}
		else
		{
			if (version_info[2] == MCS5080_CHIP)
			{
				if (version_info[1] != MCS5080_last_ver)
				{
					mcsdl_download_binary_data(MCS5080_CHIP);
					updated = 1;
				} else
					printk(KERN_ERR"Touchkey IC module is old, can't update!");
			}
			if (updated)
			{
				get_touchkey_data(version_info, 3);
				printk(TCHKEY_KERN_DEBUG "Updated F/W version: 0x%x, Module version:0x%x\n", version_info[1], version_info[2]);
			}
		}
//-------------------   Auto Firmware Update Routine End   -------------------//

	ret = i2c_add_driver(&touchkey_i2c_driver);

	if(ret||(touchkey_driver==NULL))
	{
		touchkey_dead = 1;
		printk("ret = %d, touch_driver= %p:", ret, touchkey_driver);
		printk(KERN_ERR
		       "melfas touch keypad registration failed, module not inserted.ret= %d\n",
		       ret);
	}

	return ret;
}

static void __exit touchkey_exit(void)
{
	i2c_del_driver(&touchkey_i2c_driver);
//	misc_deregister(&touchkey_update_device);
	if (touchkey_wq)
		destroy_workqueue(touchkey_wq);
	gpio_free(_3_GPIO_TOUCH_INT);
}

module_init(touchkey_init);
module_exit(touchkey_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("@@@");
MODULE_DESCRIPTION("melfas touch keypad");
