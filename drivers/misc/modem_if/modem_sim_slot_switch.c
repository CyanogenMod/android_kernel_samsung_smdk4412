#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/err.h>

#include <plat/gpio-cfg.h>

#include <mach/gpio.h>

extern struct class *sec_class;
struct device *slot_switch_dev;

static ssize_t get_slot_switch(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;

	//return '0' slot path is '||', return '1' slot path is 'X'
	value = gpio_get_value(GPIO_UIM_SIM_SEL);
#if defined(CONFIG_MACH_T0_CHN_CTC)
	if (system_rev < 7)
		value = (~value & 0x1);
#endif
	printk("Current Slot is %x\n", value);

	return sprintf(buf, "%d\n", value);
}

static ssize_t set_slot_switch(struct device *dev, struct device_attribute *attr,   const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	switch(value) {
		case 0:
#if defined(CONFIG_MACH_T0_CHN_CTC)
			if (system_rev < 7)
				gpio_set_value(GPIO_UIM_SIM_SEL, 1);
			else
#endif
			gpio_set_value(GPIO_UIM_SIM_SEL, 0);
			printk("set slot switch to %x\n", gpio_get_value(GPIO_UIM_SIM_SEL));
			break;
		case 1:
#if defined(CONFIG_MACH_T0_CHN_CTC)
			if (system_rev < 7)
				gpio_set_value(GPIO_UIM_SIM_SEL, 0);
			else
#endif
			gpio_set_value(GPIO_UIM_SIM_SEL, 1);
			printk("set slot switch to %x\n", gpio_get_value(GPIO_UIM_SIM_SEL));
			break;
		default:
			printk("Enter 0 or 1!!\n");
	}

	return size;
}

static DEVICE_ATTR(slot_sel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, get_slot_switch, set_slot_switch);

static int __init slot_switch_manager_init(void)
{
	int ret = 0;
	int err = 0;

	printk("slot_switch_manager_init\n");

    //initailize uim_sim_switch gpio
	err = gpio_request(GPIO_UIM_SIM_SEL, "PDA_ACTIVE");
	if (err) {
		pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"PDA_ACTIVE", GPIO_UIM_SIM_SEL, err);
	} else {
		gpio_direction_output(GPIO_UIM_SIM_SEL, 1);
		s3c_gpio_setpull(GPIO_UIM_SIM_SEL, S3C_GPIO_PULL_NONE);
#if defined(CONFIG_MACH_T0_CHN_CTC)
	if (system_rev < 7)
		gpio_set_value(GPIO_UIM_SIM_SEL, 1);
	else
#endif
		gpio_set_value(GPIO_UIM_SIM_SEL, 0);
	}

	//initailize slot switch device
	slot_switch_dev = device_create(sec_class,
                                    NULL, 0, NULL, "slot_switch");
	if (IS_ERR(slot_switch_dev))
		pr_err("Failed to create device(switch)!\n");

	if (device_create_file(slot_switch_dev, &dev_attr_slot_sel) < 0)
		pr_err("Failed to create device file(%s)!\n",
					dev_attr_slot_sel.attr.name);

	return ret;
}

static void __exit slot_switch_manager_exit(void)
{
}

module_init(slot_switch_manager_init);
module_exit(slot_switch_manager_exit);

MODULE_AUTHOR("SAMSUNG ELECTRONICS CO., LTD");
MODULE_DESCRIPTION("Slot Switch");
MODULE_LICENSE("GPL");
