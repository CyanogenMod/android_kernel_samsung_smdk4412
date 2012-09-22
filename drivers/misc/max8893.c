/*
*
*Minimalistic MAX8893 PMIC driver for CMC732 WiMAX chipset
*
*Use retrys when bus is shared or problematic.
*
*Functions not exported are made static
*
*In this driver, BUCK is referred as LDO number "-1"
*/


#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wimax/samsung/max8893.h>

/*#define WIMAX_DEBUG*/


static struct i2c_client *max8893_client;



static int max8893_set_ldo(int ldo, int enable)
{
	int temp;
	temp = i2c_smbus_read_byte_data(max8893_client, MAX8893_REG_ONOFF);
	if (temp < 0)
		return temp;
	temp = enable ? (temp|ENABLE_LDO(ldo)) : (temp&DISABLE_LDO(ldo));
	return i2c_smbus_write_byte_data(max8893_client,
	MAX8893_REG_ONOFF, (unsigned char)temp);

}

#ifdef WIMAX_DEBUG

static int max8893_get_ldo(int ldo)
{
	int temp;
	temp = i2c_smbus_read_byte_data(max8893_client, MAX8893_REG_ONOFF);
	if (temp < 0)
		return temp;
	return (temp&ENABLE_LDO(ldo)) ? 1 : 0;
}

static int max8893_get_voltage(int ldo)
{
	int voltage;
	voltage = i2c_smbus_read_byte_data(max8893_client,
			MAX8893_REG_LDO(ldo));
	return voltage ;

}

#endif

static int max8893_set_voltage(int ldo, int mv)
{
	int temp;
	if (mv > MAX_VOLTAGE(ldo))
		return -22;
	temp = MIN_VOLTAGE(ldo);
	if (mv < temp)
		return -22;
	temp = (mv - temp)/100;
	return i2c_smbus_write_byte_data(max8893_client,
		MAX8893_REG_LDO(ldo), (unsigned char)temp);
}

int wimax_pmic_set_voltage(void)
{
	int ret;
#ifdef WIMAX_DEBUG
	int i;
#endif

	mdelay(10);
	ret = (
	max8893_set_voltage(BUCK, 1200)|
	max8893_set_voltage(LDO1, 2800)|
	max8893_set_voltage(LDO2, 2800)|
	max8893_set_voltage(LDO3, 3300)|
	max8893_set_voltage(LDO4, 2900)|
	max8893_set_voltage(LDO5, 2800)
	);
	if (ret)
		printk(KERN_ERR "ERROR SETTING WIMAX PMIC POWER	ret = %d\n",
									ret);

#ifdef WIMAX_DEBUG
	max8893_set_ldo(BUCK, ON);
	max8893_set_ldo(LDO1, ON);
	max8893_set_ldo(LDO2, ON);
	max8893_set_ldo(LDO3, ON);
	max8893_set_ldo(LDO4, ON);
	max8893_set_ldo(LDO5, ON);

	for (i = 1; i < 6; i++) {
		printk(KERN_DEBUG "[WIMAX_DEBUG] LDO%d STATE :%d\n", i,
						max8893_get_ldo(i));
		printk(KERN_DEBUG "[WIMAX_DEBUG] LDO%d VOLTAGE:%x\n", i,
						max8893_get_voltage(i));
	}
#endif
	return ret;
}
EXPORT_SYMBOL(wimax_pmic_set_voltage);

static int max8893_wmx_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret;
#ifdef WIMAX_DEBUG
	int i;
#endif
	max8893_client = client;
	return 0;
}


static int max8893_wmx_remove(struct i2c_client *client)
{
	max8893_set_ldo(BUCK, OFF);
	max8893_set_ldo(LDO1, OFF);
	max8893_set_ldo(LDO2, OFF);
	max8893_set_ldo(LDO3, OFF);
	max8893_set_ldo(LDO4, OFF);
	max8893_set_ldo(LDO5, OFF);
	return 0;
}
const struct i2c_device_id max8893_wmx_id[] = {
	{ "max8893_wmx", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, max8893_wmx_id);
static struct i2c_driver max8893_wmx_driver = {
	.probe		= max8893_wmx_probe,
	.remove		= max8893_wmx_remove,
	.id_table	= max8893_wmx_id,
	.driver = {
		.name   = "max8893_wmx",
	},
};
static int __init max8893_wmx_pmic_init(void)
{
	return i2c_add_driver(&max8893_wmx_driver);
}
static void __exit max8893_wmx_pmic_exit(void)
{
	i2c_del_driver(&max8893_wmx_driver);
}
subsys_initcall(max8893_wmx_pmic_init);
module_exit(max8893_wmx_pmic_exit);
MODULE_DESCRIPTION("MAXIM 8893 PMIC driver for WiMAX");
MODULE_AUTHOR("SAMSUNG ELECTRONICS");
MODULE_LICENSE("GPL");
