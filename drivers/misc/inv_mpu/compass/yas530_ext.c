#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include <linux/module.h>
#include <linux/delay.h>
#include "mpu-dev.h"

#include "log.h"
#include <linux/mpu.h>
#include "mlsl.h"
#include "mldl_cfg.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-compass"

struct yas530_ext_private_data {
	int flags;
	char offsets[3];
	const int *correction_matrix;
};


extern int geomagnetic_api_read(int *xyz, int *raw, int *xy1y2, int *accuracy);
extern int geomagnetic_api_resume(void);
extern int geomagnetic_api_suspend(void);


static int yas530_ext_suspend(void *mlsl_handle,
			  struct ext_slave_descr *slave,
			  struct ext_slave_platform_data *pdata)
{
	int result = INV_SUCCESS;

	geomagnetic_api_suspend();

	return result;
}


static int yas530_ext_resume(void *mlsl_handle,
			 struct ext_slave_descr *slave,
			 struct ext_slave_platform_data *pdata)
{
	int result = INV_SUCCESS;

	struct yas530_ext_private_data *private_data = pdata->private_data;

	geomagnetic_api_resume();

	return result;
}

static int yas530_ext_read(void *mlsl_handle,
		       struct ext_slave_descr *slave,
		       struct ext_slave_platform_data *pdata,
		       unsigned char *data)
{
	int result = INV_SUCCESS;
	int raw[3] = {0,};
	int xyz[3] = {0,};
	int accuracy = 0;
	int i = 0;
	short xyz_scaled[3] = {0,};

	geomagnetic_api_read(xyz, raw, NULL, &accuracy);

	xyz_scaled[0] = (short)(xyz[0]/100);
	xyz_scaled[1] = (short)(xyz[1]/100);
	xyz_scaled[2] = (short)(xyz[2]/100);

	data[0] = xyz_scaled[0] >> 8;
	data[1] = xyz_scaled[0] & 0xFF;
	data[2] = xyz_scaled[1] >> 8;
	data[3] = xyz_scaled[1] & 0xFF;
	data[4] = xyz_scaled[2] >> 8;
	data[5] = xyz_scaled[2] & 0xFF;
	data[6] = (unsigned char)accuracy;


	return result;

}

static int yas530_ext_config(void *mlsl_handle,
			 struct ext_slave_descr *slave,
			 struct ext_slave_platform_data *pdata,
			 struct ext_slave_config *data)
{
	int result = INV_SUCCESS;
	struct yas530_private_data *private_data = pdata->private_data;


	return result;

}


static int yas530_ext_get_config(void *mlsl_handle,
			     struct ext_slave_descr *slave,
			     struct ext_slave_platform_data *pdata,
			     struct ext_slave_config *data)
{
	int result = INV_SUCCESS;
	struct yas530_ext_private_data *private_data = pdata->private_data;

	switch (data->key)
	{
		default:
			return INV_ERROR_FEATURE_NOT_IMPLEMENTED;
	}
	return result;
}

static int yas530_ext_init(void *mlsl_handle,
		       struct ext_slave_descr *slave,
		       struct ext_slave_platform_data *pdata)
{

	struct yas530_ext_private_data *private_data;
	int result = INV_SUCCESS;
	char offset[3] = {0, 0, 0};

	private_data = (struct yas530_ext_private_data *)
			kzalloc(sizeof(struct yas530_ext_private_data), GFP_KERNEL);

	if (!private_data)
		return INV_ERROR_MEMORY_EXAUSTED;

	private_data->correction_matrix = pdata->private_data;

	pdata->private_data = private_data;

	return result;
}

static int yas530_ext_exit(void *mlsl_handle,
		       struct ext_slave_descr *slave,
		       struct ext_slave_platform_data *pdata)
{
	kfree(pdata->private_data);
	return INV_SUCCESS;
}

static struct ext_slave_descr yas530_ext_descr = {
	.init             = yas530_ext_init,
	.exit             = yas530_ext_exit,
	.suspend          = yas530_ext_suspend,
	.resume           = yas530_ext_resume,
	.read             = yas530_ext_read,
	.config           = yas530_ext_config,
	.get_config       = yas530_ext_get_config,
	.name             = "yas530ext",
	.type             = EXT_SLAVE_TYPE_COMPASS,
	.id               = COMPASS_ID_YAS530_EXT,
	.read_reg         = 0x06,
	.read_len         = 7,
	.endian           = EXT_SLAVE_BIG_ENDIAN,
	.range            = {3276, 8001},
	.trigger          = NULL,
};


struct ext_slave_descr *yas530_ext_get_slave_descr(void)
{
	return &yas530_ext_descr;
}

/* -------------------------------------------------------------------------- */
struct yas530_ext_mod_private_data {
	struct i2c_client *client;
	struct ext_slave_platform_data *pdata;
};

static unsigned short normal_i2c[] = { I2C_CLIENT_END };

static int yas530_ext_mod_probe(struct i2c_client *client,
			   const struct i2c_device_id *devid)
{
	struct ext_slave_platform_data *pdata;
	struct yas530_ext_mod_private_data *private_data;
	int result = 0;

	dev_info(&client->adapter->dev, "%s: %s\n", __func__, devid->name);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		result = -ENODEV;
		goto out_no_free;
	}

	pdata = client->dev.platform_data;
	if (!pdata) {
		dev_err(&client->adapter->dev,
			"Missing platform data for slave %s\n", devid->name);
		result = -EFAULT;
		goto out_no_free;
	}

	private_data = kzalloc(sizeof(*private_data), GFP_KERNEL);
	if (!private_data) {
		result = -ENOMEM;
		goto out_no_free;
	}

	i2c_set_clientdata(client, private_data);
	private_data->client = client;
	private_data->pdata = pdata;

	result = inv_mpu_register_slave(THIS_MODULE, client, pdata,
					yas530_ext_get_slave_descr);
	if (result) {
		dev_err(&client->adapter->dev,
			"Slave registration failed: %s, %d\n",
			devid->name, result);
		goto out_free_memory;
	}

	return result;

out_free_memory:
	kfree(private_data);
out_no_free:
	dev_err(&client->adapter->dev, "%s failed %d\n", __func__, result);
	return result;

}

static int yas530_ext_mod_remove(struct i2c_client *client)
{
	struct yas530_ext_mod_private_data *private_data =
		i2c_get_clientdata(client);

	dev_dbg(&client->adapter->dev, "%s\n", __func__);

	inv_mpu_unregister_slave(client, private_data->pdata,
				yas530_ext_get_slave_descr);

	kfree(private_data);
	return 0;
}

static const struct i2c_device_id yas530_ext_mod_id[] = {
	{ "yas530ext", COMPASS_ID_YAS530_EXT},
	{}
};

MODULE_DEVICE_TABLE(i2c, yas530_ext_mod_id);

static struct i2c_driver yas530_ext_mod_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = yas530_ext_mod_probe,
	.remove = yas530_ext_mod_remove,
	.id_table = yas530_ext_mod_id,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "yas530_ext_mod",
		   },
	.address_list = normal_i2c,
};

static int __init yas530_ext_mod_init(void)
{
	int res = i2c_add_driver(&yas530_ext_mod_driver);
	pr_info("%s: Probe name %s\n", __func__, "yas530_ext_mod");
	if (res)
		pr_err("%s failed\n", __func__);
	return res;
}

static void __exit yas530_ext_mod_exit(void)
{
	pr_info("%s\n", __func__);
	i2c_del_driver(&yas530_ext_mod_driver);
}

module_init(yas530_ext_mod_init);
module_exit(yas530_ext_mod_exit);

MODULE_AUTHOR("Invensense Corporation");
MODULE_DESCRIPTION("Driver to integrate YAS530 sensor with the MPU");
MODULE_LICENSE("GPL");
MODULE_ALIAS("yas530_ext_mod");

/**
 *  @}
 */
