/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include "ssp.h"

/* ssp mcu device ID */
#define DEVICE_ID			0x55

static void ssp_early_suspend(struct early_suspend *handler);
static void ssp_late_resume(struct early_suspend *handler);

/************************************************************************/
/* interrupt happened due to transition/change of SSP MCU		*/
/************************************************************************/

static irqreturn_t sensordata_irq_thread_fn(int iIrq, void *dev_id)
{
	struct ssp_data *data = dev_id;

	select_irq_msg(data);
	data->uIrqCnt++;

	return IRQ_HANDLED;
}

/*************************************************************************/
/* initialize sensor hub						 */
/*************************************************************************/

static void initialize_variable(struct ssp_data *data)
{
	int iSensorIndex;

	for (iSensorIndex = 0; iSensorIndex < SENSOR_MAX; iSensorIndex++) {
		data->adDelayBuf[iSensorIndex] = DEFUALT_POLLING_DELAY;
		data->aiCheckStatus[iSensorIndex] = INITIALIZATION_STATE;
	}

	/* AKM Daemon Library */
	data->aiCheckStatus[GEOMAGNETIC_SENSOR] = NO_SENSOR_STATE;
	data->aiCheckStatus[ORIENTATION_SENSOR] = NO_SENSOR_STATE;
	memset(data->uFactorydata, 0, sizeof(char) * FACTORY_DATA_MAX);

	atomic_set(&data->aSensorEnable, 0);
	data->iLibraryLength = 0;
	data->uSensorState = 0;
	data->uFactorydataReady = 0;
	data->uFactoryProxAvg[0] = 0;

	data->uResetCnt = 0;
	data->uInstFailCnt = 0;
	data->uTimeOutCnt = 0;
	data->uSsdFailCnt = 0;
	data->uBusyCnt = 0;
	data->uIrqCnt = 0;
	data->uIrqFailCnt = 0;
	data->uMissSensorCnt = 0;

	data->bCheckSuspend = false;
	data->bSspShutdown = false;
	data->bDebugEnabled = false;
	data->bProximityRawEnabled = false;
	data->bMcuIRQTestSuccessed = false;
	data->bBarcodeEnabled = false;
	data->bAccelAlert = false;

	data->accelcal.x = 0;
	data->accelcal.y = 0;
	data->accelcal.z = 0;

	data->gyrocal.x = 0;
	data->gyrocal.y = 0;
	data->gyrocal.z = 0;

	data->iPressureCal = 0;
	data->uProxCanc = 0;
	data->uProxHiThresh = 0;
	data->uProxLoThresh = 0;
	data->uGyroDps = GYROSCOPE_DPS500;

	data->mcu_device = NULL;
	data->acc_device = NULL;
	data->gyro_device = NULL;
	data->mag_device = NULL;
	data->prs_device = NULL;
	data->prox_device = NULL;
	data->light_device = NULL;

	initialize_function_pointer(data);
}

int initialize_mcu(struct ssp_data *data)
{
	int iRet = 0;

	iRet = get_chipid(data);
	pr_info("[SSP] MCU device ID = %d, reading ID = %d\n", DEVICE_ID, iRet);
	if (iRet != DEVICE_ID) {
		if (iRet < 0) {
			pr_err("[SSP]: %s - MCU is not working : 0x%x\n",
				__func__, iRet);
		} else {
			pr_err("[SSP]: %s - MCU identification failed\n",
				__func__);
			iRet = -ENODEV;
		}
		return iRet;
	}

	iRet = set_sensor_position(data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - set_sensor_position failed\n", __func__);
		return iRet;
	}

	iRet = get_fuserom_data(data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - get_fuserom_data failed\n", __func__);
		return FAIL;
	}

	data->uSensorState = get_sensor_scanning_info(data);
	if (data->uSensorState == 0) {
		pr_err("[SSP]: %s - get_sensor_scanning_info failed\n",
			__func__);
		return FAIL;
	}

	return SUCCESS;
}

static int initialize_irq(struct ssp_data *data)
{
	int iRet, iIrq;

	iRet = gpio_request(data->client->irq, "mpu_ap_int1");
	if (iRet < 0) {
		pr_err("[SSP]: %s - gpio %d request failed (%d)\n",
		       __func__, data->client->irq, iRet);
		return iRet;
	}

	iRet = gpio_direction_input(data->client->irq);
	if (iRet < 0) {
		pr_err("[SSP]: %s - failed to set gpio %d as input (%d)\n",
		       __func__, data->client->irq, iRet);
		goto err_irq_direction_input;
	}

	iIrq = gpio_to_irq(data->client->irq);

	pr_info("[SSP]: requesting IRQ %d\n", iIrq);
	iRet = request_threaded_irq(iIrq, NULL, sensordata_irq_thread_fn,
				    IRQF_TRIGGER_FALLING, "SSP_Int", data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - request_irq(%d) failed for gpio %d (%d)\n",
		       __func__, iIrq, iIrq, iRet);
		goto err_request_irq;
	}

	/* start with interrupts disabled */
	data->iIrq = iIrq;
	disable_irq(data->iIrq);
	return 0;

err_request_irq:
err_irq_direction_input:
	gpio_free(data->client->irq);
	return iRet;
}

static int ssp_probe(struct i2c_client *client,
	const struct i2c_device_id *devid)
{
	int iRet = 0;
	struct ssp_data *data;
	struct ssp_platform_data *pdata = client->dev.platform_data;

	if (pdata == NULL) {
		pr_err("[SSP]: %s - platform_data is null..\n", __func__);
		iRet = -ENOMEM;
		goto exit;
	}

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL) {
		pr_err("[SSP]: %s - failed to allocate memory for data\n",
			__func__);
		iRet = -ENOMEM;
		goto exit;
	}

	data->client = client;
	i2c_set_clientdata(client, data);

	data->wakeup_mcu = pdata->wakeup_mcu;
	data->check_mcu_ready = pdata->check_mcu_ready;
	data->check_mcu_busy = pdata->check_mcu_busy;
	data->set_mcu_reset = pdata->set_mcu_reset;
	data->check_ap_rev = pdata->check_ap_rev;

	if ((data->wakeup_mcu == NULL)
		|| (data->check_mcu_ready == NULL)
		|| (data->check_mcu_busy == NULL)
		|| (data->set_mcu_reset == NULL)
		|| (data->check_ap_rev == NULL)) {
		pr_err("[SSP]: %s - function callback is null\n", __func__);
		iRet = -EIO;
		goto err_reset_null;
	}

	pr_info("\n#####################################################\n");

	/* check boot loader binary */
	check_fwbl(data);

	initialize_variable(data);

	iRet = initialize_mcu(data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - initialize_mcu failed\n", __func__);
		goto err_read_reg;
	}

	wake_lock_init(&data->ssp_wake_lock,
		WAKE_LOCK_SUSPEND, "ssp_wake_lock");

	iRet = initialize_input_dev(data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - could not create input device\n", __func__);
		goto err_input_register_device;
	}

	initialize_magnetic(data);

	iRet = misc_register(&data->akmd_device);
	if (iRet)
		goto err_akmd_device_register;

	iRet = initialize_debug_timer(data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}

	iRet = initialize_irq(data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - could not create irq\n", __func__);
		goto err_setup_irq;
	}

	iRet = initialize_sysfs(data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - could not create sysfs\n", __func__);
		goto err_sysfs_create;
	}

	iRet = initialize_event_symlink(data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - could not create symlink\n", __func__);
		goto err_symlink_create;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.suspend = ssp_early_suspend;
	data->early_suspend.resume = ssp_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	/* init sensorhub device */
	iRet = ssp_initialize_sensorhub(data);
	if (iRet < 0) {
		pr_err("%s: ssp_initialize_sensorhub err(%d)", __func__, iRet);
		ssp_remove_sensorhub(data);
	}
#endif

	enable_irq(data->iIrq);
	enable_irq_wake(data->iIrq);
	pr_info("[SSP]: %s - probe success!\n", __func__);

	enable_debug_timer(data);

	iRet = 0;
	goto exit;

err_symlink_create:
	remove_sysfs(data);
err_sysfs_create:
	free_irq(data->iIrq, data);
	gpio_free(data->client->irq);
err_setup_irq:
	destroy_workqueue(data->debug_wq);
err_create_workqueue:
	misc_deregister(&data->akmd_device);
err_akmd_device_register:
	remove_input_dev(data);
err_input_register_device:
	wake_lock_destroy(&data->ssp_wake_lock);
err_read_reg:
err_reset_null:
	kfree(data);
	pr_err("[SSP]: %s - probe failed!\n", __func__);
exit:
	pr_info("#####################################################\n\n");
	return iRet;
}

static void ssp_shutdown(struct i2c_client *client)
{
	struct ssp_data *data = i2c_get_clientdata(client);

	func_dbg();
	data->bSspShutdown = true;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif

	disable_debug_timer(data);

	disable_irq_wake(data->iIrq);
	disable_irq(data->iIrq);
	free_irq(data->iIrq, data);
	gpio_free(data->client->irq);

	remove_sysfs(data);
	remove_event_symlink(data);
	remove_input_dev(data);

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	ssp_remove_sensorhub(data);
#endif

	misc_deregister(&data->akmd_device);

	del_timer_sync(&data->debug_timer);
	cancel_work_sync(&data->work_debug);
	destroy_workqueue(data->debug_wq);
	wake_lock_destroy(&data->ssp_wake_lock);

	toggle_mcu_reset(data);

	kfree(data);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ssp_early_suspend(struct early_suspend *handler)
{
	struct ssp_data *data;
	data = container_of(handler, struct ssp_data, early_suspend);

	func_dbg();
	disable_debug_timer(data);

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	/* give notice to user that AP goes to sleep */
	ssp_report_sensorhub_notice(data, MSG2SSP_AP_STATUS_SLEEP);
	ssp_sleep_mode(data);
#else
	if (atomic_read(&data->aSensorEnable) > 0)
		ssp_sleep_mode(data);
#endif

	data->bCheckSuspend = true;
}

static void ssp_late_resume(struct early_suspend *handler)
{
	struct ssp_data *data;
	data = container_of(handler, struct ssp_data, early_suspend);

	func_dbg();
	enable_debug_timer(data);

	data->bCheckSuspend = false;

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	/* give notice to user that AP goes to sleep */
	ssp_report_sensorhub_notice(data, MSG2SSP_AP_STATUS_WAKEUP);
	ssp_resume_mode(data);
#else
	if (atomic_read(&data->aSensorEnable) > 0)
		ssp_resume_mode(data);
#endif
}

#else /* CONFIG_HAS_EARLYSUSPEND */

static int ssp_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ssp_data *data = i2c_get_clientdata(client);

	func_dbg();
	disable_debug_timer(data);

	if (atomic_read(&data->aSensorEnable) > 0)
		ssp_sleep_mode(data);

	data->bCheckSuspend = true;
	return 0;
}

static int ssp_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ssp_data *data = i2c_get_clientdata(client);

	func_dbg();
	enable_debug_timer(data);

	data->bCheckSuspend = false;

	if (atomic_read(&data->aSensorEnable) > 0)
		ssp_resume_mode(data);

	return 0;
}

static const struct dev_pm_ops ssp_pm_ops = {
	.suspend = ssp_suspend,
	.resume = ssp_resume
};

#endif /* CONFIG_HAS_EARLYSUSPEND */

static const struct i2c_device_id ssp_id[] = {
	{"ssp", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, ssp_id);

static struct i2c_driver ssp_driver = {
	.probe = ssp_probe,
	.shutdown = ssp_shutdown,
	.id_table = ssp_id,
	.driver = {
#ifndef CONFIG_HAS_EARLYSUSPEND
		   .pm = &ssp_pm_ops,
#endif
		   .owner = THIS_MODULE,
		   .name = "ssp"
		},
};

static int __init ssp_init(void)
{
	return i2c_add_driver(&ssp_driver);
}

static void __exit ssp_exit(void)
{
	i2c_del_driver(&ssp_driver);
}

module_init(ssp_init);
module_exit(ssp_exit);

MODULE_DESCRIPTION("ssp driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
