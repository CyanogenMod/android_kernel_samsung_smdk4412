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

#ifdef CONFIG_CPU_FREQ
#include <mach/cpufreq.h>
#endif

/*
Melfas touchkey register
*/
#define KEYCODE_REG 0x00
#define FIRMWARE_VERSION 0x01
#define TOUCHKEY_MODULE_VERSION 0x02
#define TOUCHKEY_ADDRESS	0x20

#define UPDOWN_EVENT_BIT 0x08
#define KEYCODE_BIT 0x07
#define ESD_STATE_BIT 0x10

/* keycode value */
#define TOUCHKEY_KEYCODE_MENU		139
#define TOUCHKEY_KEYCODE_BACK		158

#define I2C_M_WR 0 /* for i2c */

//#define IRQ_TOUCH_INT (IRQ_EINT_GROUP22_BASE + 1)
#define DEVICE_NAME "melfas-touchkey"
#define INT_PEND_BASE	0xE0200A54

#define MCS5000_CHIP		0x93
#define MCS5080_CHIP		0x90
#define MCS5000_last_ver	0x38
#define MCS5080_last_ver	0x30

// if you want to see log, set this definition to NULL or KERN_WARNING
#define TCHKEY_KERN_DEBUG      KERN_DEBUG

#define _3_TOUCH_SDA_28V GPIO_3_TOUCH_SDA
#define _3_TOUCH_SCL_28V GPIO_3_TOUCH_SCL
#define _3_GPIO_TOUCH_EN	GPIO_TOUCH_EN
#define _3_GPIO_TOUCH_INT	GPIO_3_TOUCH_INT
//#define IRQ_TOUCH_INT S5P_GPIOINT_BASE+22
#define IRQ_TOUCH_INT gpio_to_irq(GPIO_3_TOUCH_INT)

static unsigned int HWREV=7;
//extern unsigned int HWREV; jiseong.oh
static int touchkey_keycode[3] =
    { 0, TOUCHKEY_KEYCODE_MENU, TOUCHKEY_KEYCODE_BACK };
static u8 activation_onoff = 1;	// 0:deactivate   1:activate
static u8 is_suspending = 0;
static u8 user_press_on = 0;
static u8 touchkey_dead = 0;
static u8 menu_sensitivity = 0;
static u8 back_sensitivity = 0;
static u8 version_info[3];
static void	__iomem	*gpio_pend_mask_mem;

struct i2c_touchkey_driver {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
	struct early_suspend	early_suspend;
};
struct i2c_touchkey_driver *touchkey_driver = NULL;
struct workqueue_struct *touchkey_wq;

static const struct i2c_device_id melfas_touchkey_id[] = {
	{"melfas-touchkey", 1},
	{}
};

MODULE_DEVICE_TABLE(i2c, melfas_touchkey_id);

extern void get_touchkey_data(u8 *data, u8 length);
static void init_hw(void);
static int i2c_touchkey_probe(struct i2c_client *client, const struct i2c_device_id *id);

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
	u8 data[5];
	int keycode;
	int retry = 10;

	if(!gpio_get_value(_3_GPIO_TOUCH_INT) && !touchkey_dead)
	{
		get_touchkey_data(data, 5);

		if((data[0] & ESD_STATE_BIT)|(data[3]>=45)|(data[4]>=45)) // ESD state or abnormal sensitivity
		{
			is_suspending = 1;
			printk(KERN_ERR "touchkey ESD_STATE_BIT set\n");
			if(user_press_on==1)
			{
				input_report_key(touchkey_driver->input_dev, TOUCHKEY_KEYCODE_MENU, 0);
				printk(TCHKEY_KERN_DEBUG "ESD release menu key\n");
			}
			else if(user_press_on==2)
			{
				input_report_key(touchkey_driver->input_dev, TOUCHKEY_KEYCODE_BACK, 0);
				printk(TCHKEY_KERN_DEBUG "ESD release back key\n");
			}
			user_press_on = 0;

			while(retry--)
			{
				gpio_direction_output(_3_GPIO_TOUCH_EN, 0);
				gpio_direction_output(_3_TOUCH_SDA_28V, 0);
				gpio_direction_output(_3_TOUCH_SCL_28V, 0);
				msleep(300);
				init_hw();
				if(i2c_touchkey_read(KEYCODE_REG, data, 3)>=0)
				{
					printk(TCHKEY_KERN_DEBUG "touchkey ESD init success\n");
					enable_irq(IRQ_TOUCH_INT);
					is_suspending = 0;
					return;
				}
				printk(KERN_ERR "i2c transfer error after ESD, retry...%d",retry);
			}
			touchkey_dead = 1;
			gpio_direction_output(_3_GPIO_TOUCH_EN, 0);
			gpio_direction_output(_3_TOUCH_SDA_28V, 0);
			gpio_direction_output(_3_TOUCH_SCL_28V, 0);
			printk(KERN_ERR "touchkey died after ESD");
			return;
		}
		else
		{
			keycode = touchkey_keycode[data[0] & KEYCODE_BIT];
		}

		if(activation_onoff){
			if(data[0] & UPDOWN_EVENT_BIT) // key released
			{
				user_press_on = 0;
				input_report_key(touchkey_driver->input_dev, keycode, 0);
				input_sync(touchkey_driver->input_dev);
//				printk(TCHKEY_KERN_DEBUG "touchkey release keycode: %d\n", keycode);
			}
			else // key pressed
			{

				if(touch_state_val == 1)
				{
					printk(TCHKEY_KERN_DEBUG "touchkey pressed but don't send event because touch is pressed. \n");
				}
				else
				{
					if(keycode==TOUCHKEY_KEYCODE_BACK)
					{
						//TSP_forced_release(); Jiseong.oh
#ifdef CONFIG_CPU_FREQ
//						set_dvfs_target_level(LEV_800MHZ);//set to comment temporarily by mseok.chae 2011.01.11
#endif
						user_press_on = 2;
						back_sensitivity = data[4];
						input_report_key(touchkey_driver->input_dev, keycode,1);
						input_sync(touchkey_driver->input_dev);
//						printk(TCHKEY_KERN_DEBUG "back key sensitivity = %d\n",back_sensitivity);
//						printk(TCHKEY_KERN_DEBUG " touchkey press keycode: %d\n", keycode);
					}
					else if(keycode==TOUCHKEY_KEYCODE_MENU)
					{
						user_press_on = 1;
						menu_sensitivity = data[3];
						input_report_key(touchkey_driver->input_dev, keycode,1);
						input_sync(touchkey_driver->input_dev);
//						printk(TCHKEY_KERN_DEBUG "menu key sensitivity = %d\n",menu_sensitivity);
//						printk(TCHKEY_KERN_DEBUG " touchkey press keycode: %d\n", keycode);
					}
				}
			}
		}
	}
	else
		printk(KERN_ERR "touchkey interrupt line is high!\n");

	enable_irq(IRQ_TOUCH_INT);
	return ;
}

static irqreturn_t touchkey_interrupt(int irq, void *dummy)
{
	disable_irq_nosync(IRQ_TOUCH_INT);
	queue_work(touchkey_wq, &touchkey_driver->work);

	return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_touchkey_early_suspend(struct early_suspend *h)
{
    pr_info("melfas_touchkey_early_suspend +++\n");

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

    pr_info("melfas_touchkey_early_suspend ---\n");
}

static void melfas_touchkey_early_resume(struct early_suspend *h)
{
    pr_info("melfas_touchkey_early_resume +++\n");

	if(touchkey_dead)
	{
		printk(KERN_ERR "touchkey died after ESD");
		return;
	}

	gpio_direction_output(_3_GPIO_TOUCH_EN, 1);
	msleep(100);

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
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(touchkey_keycode[1], input_dev->keybit);
	set_bit(touchkey_keycode[2], input_dev->keybit);

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

	if (request_irq(IRQ_TOUCH_INT, touchkey_interrupt, IRQF_DISABLED, DEVICE_NAME, touchkey_driver))
	{
		printk(KERN_ERR "%s Can't allocate irq ..\n", __FUNCTION__);
		return -EBUSY;
	}

	return 0;
}

static void init_hw(void)
{
	gpio_direction_output(_3_GPIO_TOUCH_EN, 1);
	msleep(100);
	s3c_gpio_setpull(_3_GPIO_TOUCH_INT, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(EXYNOS4212_GPJ1(0), S3C_GPIO_SFN(0xf));
	irq_set_irq_type(IRQ_TOUCH_INT, IRQ_TYPE_EDGE_FALLING);
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
	printk(TCHKEY_KERN_DEBUG "called %s \n",__func__);
	return sprintf(buf,"%02x\n",version_info[1]);
}

static ssize_t touchkey_recommend_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	u8 recommended_ver;
	printk(TCHKEY_KERN_DEBUG "called %s \n",__func__);
	if(version_info[2]==MCS5000_CHIP)
	{
		recommended_ver = MCS5000_last_ver;
	}
	else if(version_info[2]==MCS5080_CHIP)
	{
		recommended_ver = MCS5080_last_ver;
	}
	else
	{
		recommended_ver = version_info[1];
	}

	return sprintf(buf,"%02x\n",recommended_ver);
}

static ssize_t touchkey_firmup_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk(TCHKEY_KERN_DEBUG "Touchkey firm-up start!\n");
	if(version_info[2]==MCS5000_CHIP)
		mcsdl_download_binary_data(MCS5000_CHIP);
	else if(version_info[2]==MCS5080_CHIP)
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
	printk(TCHKEY_KERN_DEBUG "called %s \n",__func__);
	return sprintf(buf,"%d\n",touchkey_dead);
}

static ssize_t touchkey_menu_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk(TCHKEY_KERN_DEBUG "called %s \n",__func__);
	return sprintf(buf,"%d\n",menu_sensitivity);
}

static ssize_t touchkey_back_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk(TCHKEY_KERN_DEBUG "called %s \n",__func__);
	return sprintf(buf,"%d\n",back_sensitivity);
}

static ssize_t touch_led_control(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	u8 data;
	if(user_press_on)
		return size;

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

struct file_operations touchkey_update_fops =
{
	.owner   = THIS_MODULE,
	.read    = touchkey_update_read,
	.write   = touchkey_update_write,
	.open    = touchkey_update_open,
	.release = touchkey_update_release,
};

static struct miscdevice touchkey_update_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "melfas_touchkey",
	.fops = &touchkey_update_fops,
};

static DEVICE_ATTR(touchkey_activation, 0664, NULL, touchkey_activation_store);
static DEVICE_ATTR(touchkey_version, S_IRUGO, touchkey_version_show, NULL);
static DEVICE_ATTR(touchkey_recommend, S_IRUGO, touchkey_recommend_show, NULL);
static DEVICE_ATTR(touchkey_firmup, S_IRUGO, touchkey_firmup_show, NULL);
static DEVICE_ATTR(touchkey_init, S_IRUGO, touchkey_init_show, NULL);
static DEVICE_ATTR(touchkey_menu, S_IRUGO, touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, touchkey_back_show, NULL);
static DEVICE_ATTR(brightness, 0664, NULL, touch_led_control);
static DEVICE_ATTR(enable_disable, 0664, NULL, touchkey_enable_disable);

static int __init touchkey_init(void)
{

	int ret = 0;

	u8 updated = 0;
	if ((ret = gpio_request(_3_GPIO_TOUCH_EN, "_3_GPIO_TOUCH_EN")))
		printk(KERN_ERR "Failed to request gpio %s:%d\n", __func__, __LINE__);

//	if (ret = gpio_request(_3_GPIO_TOUCH_INT, "_3_GPIO_TOUCH_INT"))
//		printk(KERN_ERR "Failed to request gpio %s:%d\n", __func__, __LINE__);

	ret = misc_register(&touchkey_update_device);
	if (ret) {
		printk(KERN_ERR "%s misc_register fail\n",__FUNCTION__);
	}

	if (device_create_file(touchkey_update_device.this_device, &dev_attr_touchkey_activation) < 0)
	{
		printk(KERN_ERR "%s device_create_file fail dev_attr_touchkey_activation\n",__FUNCTION__);
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_activation.attr.name);
	}

	if (device_create_file(touchkey_update_device.this_device, &dev_attr_touchkey_version) < 0)
	{
		printk(KERN_ERR "%s device_create_file fail dev_attr_touchkey_version\n",__FUNCTION__);
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_version.attr.name);
	}

	if (device_create_file(touchkey_update_device.this_device, &dev_attr_touchkey_recommend) < 0)
	{
		printk(KERN_ERR "%s device_create_file fail dev_attr_touchkey_recommend\n",__FUNCTION__);
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_recommend.attr.name);
	}

	if (device_create_file(touchkey_update_device.this_device, &dev_attr_touchkey_firmup) < 0)
	{
		printk(KERN_ERR "%s device_create_file fail dev_attr_touchkey_firmup\n",__FUNCTION__);
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_firmup.attr.name);
	}

	if (device_create_file(touchkey_update_device.this_device, &dev_attr_touchkey_init) < 0)
	{
		printk(KERN_ERR "%s device_create_file fail dev_attr_touchkey_init\n",__FUNCTION__);
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_init.attr.name);
	}

	if (device_create_file(touchkey_update_device.this_device, &dev_attr_touchkey_menu) < 0)
	{
		printk(KERN_ERR "%s device_create_file fail dev_attr_touchkey_menu\n",__FUNCTION__);
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_menu.attr.name);
	}

	if (device_create_file(touchkey_update_device.this_device, &dev_attr_touchkey_back) < 0)
	{
		printk(KERN_ERR "%s device_create_file fail dev_attr_touchkey_back\n",__FUNCTION__);
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_back.attr.name);
	}

	if (device_create_file(touchkey_update_device.this_device, &dev_attr_brightness) < 0)
	{
		printk(KERN_ERR "%s device_create_file fail dev_attr_brightness\n",__FUNCTION__);
		pr_err("Failed to create device file(%s)!\n", dev_attr_brightness.attr.name);
	}

	if (device_create_file(touchkey_update_device.this_device, &dev_attr_enable_disable) < 0)
	{
		printk(KERN_ERR "%s device_create_file fail dev_attr_touch_update\n",__FUNCTION__);
		pr_err("Failed to create device file(%s)!\n", dev_attr_enable_disable.attr.name);
	}

	init_hw();

	get_touchkey_data(version_info, 3);
	printk(TCHKEY_KERN_DEBUG "%s F/W version: 0x%x, Module version:0x%x\n",__FUNCTION__, version_info[1], version_info[2]);

//-------------------   Auto Firmware Update Routine Start   -------------------//
	if(HWREV>=8)
	{
		if(version_info[1]==0xff) //unknown firmware state
		{
			if(!mcsdl_download_binary_data(MCS5000_CHIP)) //try MCS-5000 download
				mcsdl_download_binary_data(MCS5080_CHIP); // if first try is fail, MCS-5080 download

			updated = 1;
		}
		else
		{
			if(version_info[2]>=MCS5000_CHIP) //MCS-5000
			{
				if(version_info[1]!=MCS5000_last_ver) //not latest version
				{
					mcsdl_download_binary_data(MCS5000_CHIP);
					updated = 1;
				}
			}
			else if(version_info[2]==MCS5080_CHIP)//MCS-5080
			{
				if(version_info[1]!=MCS5080_last_ver) //not latest version
				{
					mcsdl_download_binary_data(MCS5080_CHIP);
					updated = 1;
				}
			}
			else
				printk("Touchkey IC module is old, can't update!");
		}

		if(updated)
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
	misc_deregister(&touchkey_update_device);
	if (touchkey_wq)
		destroy_workqueue(touchkey_wq);
	gpio_free(_3_GPIO_TOUCH_INT);
}

module_init(touchkey_init);
module_exit(touchkey_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("@@@");
MODULE_DESCRIPTION("melfas touch keypad");
