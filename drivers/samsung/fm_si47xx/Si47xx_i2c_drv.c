/* drivers/samsung/fm_si47xx/Si47xx_i2c_drv.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/stat.h>
#include <linux/ioctl.h>
#include <linux/delay.h>

#include <mach/gpio.h>

#include "Si47xx_dev.h"
#include "Si47xx_ioctl.h"
#include <linux/i2c/si47xx_common.h>


/*******************************************************/

/*static functions*/

/*file operatons*/
static int Si47xx_open(struct inode *, struct file *);
static int Si47xx_release(struct inode *, struct file *);
static long Si47xx_ioctl(struct file *, unsigned int, unsigned long);
static int Si47xx_suspend(struct i2c_client *, pm_message_t mesg);
static int Si47xx_resume(struct i2c_client *);

/*I2C Setting*/

static struct i2c_driver Si47xx_i2c_driver;
static const struct i2c_device_id Si47xx_id[] = {
	{"Si47xx", 0},
	{}
};

/* static	void	__iomem		*gpio_mask_mem; */
/**********************************************************/

static const struct file_operations Si47xx_fops = {
	.owner = THIS_MODULE,
	.open = Si47xx_open,
	.unlocked_ioctl = Si47xx_ioctl,
	.release = Si47xx_release,
};

static struct miscdevice Si47xx_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "fmradio",
	.fops = &Si47xx_fops,
};

/***************************************************************/

static int Si47xx_open(struct inode *inode, struct file *filp)
{
	debug("Si47xx_open called\n");

	return nonseekable_open(inode, filp);
}

static int Si47xx_release(struct inode *inode, struct file *filp)
{
	debug("Si47xx_release called\n\n");

	return 0;
}

static long Si47xx_ioctl(struct file *filp, unsigned int ioctl_cmd,
							unsigned long arg)
{
	long ret = 0;
	void __user *argp = (void __user *)arg;

	debug("Si47xx ioctl 0x%x", ioctl_cmd);

	if (_IOC_TYPE(ioctl_cmd) != Si47xx_IOC_MAGIC) {
		debug("Inappropriate ioctl 1 0x%x", ioctl_cmd);
		return -ENOTTY;
	}

	if (_IOC_NR(ioctl_cmd) > Si47xx_IOC_NR_MAX) {
		debug("Inappropriate ioctl 2 0x%x", ioctl_cmd);
		return -ENOTTY;
	}

	switch (ioctl_cmd) {
	case Si47xx_IOC_POWERUP:
		debug("Si47xx_IOC_POWERUP called\n\n");

		ret = (long)Si47xx_dev_powerup();
		if (ret < 0)
			debug("Si47xx_IOC_POWERUP failed\n");
		break;

	case Si47xx_IOC_POWERDOWN:
		debug("Si47xx_IOC_POWERDOWN called\n");

		ret = (long)Si47xx_dev_powerdown();
		if (ret < 0)
			debug("Si47xx_IOC_POWERDOWN failed\n");
		break;

	case Si47xx_IOC_BAND_SET:
		{
			int band;
			debug("Si47xx_IOC_BAND_SET called\n\n");

			if (copy_from_user((void *)&band, argp, sizeof(int)))
				ret = -EFAULT;
			else {
				ret = (long)Si47xx_dev_band_set(band);
				if (ret < 0)
					debug("Si47xx_IOC_BAND_SET failed\n");
			}
		}
		break;

	case Si47xx_IOC_CHAN_SPACING_SET:
		{
			int ch_spacing;
			debug("Si47xx_IOC_CHAN_SPACING_SET called\n");

			if (copy_from_user
			    ((void *)&ch_spacing, argp, sizeof(int)))
				ret = -EFAULT;
			else {
			ret = (long)Si47xx_dev_ch_spacing_set(ch_spacing);
				if (ret < 0)
					debug("Si47xx_IOC_CHAN_SPACING_SET "
						"failed\n");
			}
		}
		break;

	case Si47xx_IOC_CHAN_SELECT:
		{
			u32 frequency;
			debug("Si47xx_IOC_CHAN_SELECT called\n");

			if (copy_from_user
			    ((void *)&frequency, argp, sizeof(u32)))
				ret = -EFAULT;
			else {
				ret = (long)Si47xx_dev_chan_select(frequency);
				if (ret < 0)
					debug("Si47xx_IOC_CHAN_SELECT "
					"failed\n");
			}
		}
		break;

	case Si47xx_IOC_CHAN_GET:
		{
			u32 frequency = 0;
			debug("Si47xx_IOC_CHAN_GET called\n");

			ret = (long)Si47xx_dev_chan_get(&frequency);
			if (ret < 0)
				debug("Si47xx_IOC_CHAN_GET failed\n");
			else if (copy_to_user
				 (argp, (void *)&frequency, sizeof(u32)))
				ret = -EFAULT;
		}
		break;

	case Si47xx_IOC_SEEK_FULL:
		{
			u32 frequency = 0;
			debug("Si47xx_IOC_SEEK_FULL called\n");

			ret = (long)Si47xx_dev_seek_full(&frequency);
			if (ret < 0)
				debug("Si47xx_IOC_SEEK_FULL failed\n");
			else if (copy_to_user
				 (argp, (void *)&frequency, sizeof(u32)))
				ret = -EFAULT;
		}
		break;

	case Si47xx_IOC_SEEK_UP:
		{
			u32 frequency = 0;
			debug("Si47xx_IOC_SEEK_UP called\n");

			ret = (long)Si47xx_dev_seek_up(&frequency);
			if (ret < 0)
				debug("Si47xx_IOC_SEEK_UP failed\n");
			else if (copy_to_user
				 (argp, (void *)&frequency, sizeof(u32)))
				ret = -EFAULT;
		}
		break;

	case Si47xx_IOC_SEEK_DOWN:
		{
			u32 frequency = 0;
			debug("Si47xx_IOC_SEEK_DOWN called\n");

			ret = (long)Si47xx_dev_seek_down(&frequency);
			if (ret < 0)
				debug("Si47xx_IOC_SEEK_DOWN failed\n");
			else if (copy_to_user
				 (argp, (void *)&frequency, sizeof(u32)))
				ret = -EFAULT;
		}
		break;

	case Si47xx_IOC_RSSI_SEEK_TH_SET:
		{
			u8 RSSI_seek_th;
			debug("Si47xx_IOC_RSSI_SEEK_TH_SET called\n");

			if (copy_from_user
			    ((void *)&RSSI_seek_th, argp, sizeof(u8)))
				ret = -EFAULT;
			else {
			ret = (long)Si47xx_dev_RSSI_seek_th_set(RSSI_seek_th);
				if (ret < 0)
					debug("Si47xx_IOC_RSSI_SEEK_TH_SET "
						       "failed\n");
			}
		}
		break;

	case Si47xx_IOC_SEEK_SNR_SET:
		{
			u8 seek_SNR_th;
			debug("Si47xx_IOC_SEEK_SNR_SET called\n");

			if (copy_from_user
			    ((void *)&seek_SNR_th, argp, sizeof(u8)))
				ret = -EFAULT;
			else {
			ret = (long)Si47xx_dev_seek_SNR_th_set(seek_SNR_th);
				if (ret < 0)
					debug("Si47xx_IOC_SEEK_SNR_SET "
					"failed\n");
			}
		}
		break;

	case Si47xx_IOC_SEEK_CNT_SET:
		{
			u8 seek_FM_ID_th;
			debug("Si47xx_IOC_SEEK_CNT_SET called\n");

			if (copy_from_user
			    ((void *)&seek_FM_ID_th, argp, sizeof(u8)))
				ret = -EFAULT;
			else {
				ret =
			(long)Si47xx_dev_seek_FM_ID_th_set(seek_FM_ID_th);
				if (ret < 0)
					debug("Si47xx_IOC_SEEK_CNT_SET "
					"failed\n");
			}
		}
		break;

	case Si47xx_IOC_CUR_RSSI_GET:
		{
			struct rssi_snr_t data;
			debug("Si47xx_IOC_CUR_RSSI_GET called\n");

			ret = (long)Si47xx_dev_cur_RSSI_get(&data);
			if (ret < 0)
				debug("Si47xx_IOC_CUR_RSSI_GET failed\n");
			else if (copy_to_user(argp, (void *)&data,
					      sizeof(data)))
				ret = -EFAULT;

			debug("curr_rssi:%d\ncurr_rssi_th:%d\ncurr_snr:%d\n",
			      data.curr_rssi, data.curr_rssi_th, data.curr_snr);
		}
		break;

	case Si47xx_IOC_VOLUME_SET:
		{
			u8 volume;
			if (copy_from_user((void *)&volume, argp, sizeof(u8)))
				ret = -EFAULT;
			else {
				debug("Si47xx_IOC_VOLUME_SET called "
					"vol %d\n", volume);
				ret = (long)Si47xx_dev_volume_set(volume);
				if (ret < 0)
					debug("Si47xx_IOC_VOLUME_SET failed\n");
			}
		}
		break;

	case Si47xx_IOC_VOLUME_GET:
		{
			u8 volume;
			debug("Si47xx_IOC_VOLUME_GET called\n");

			ret = (long)Si47xx_dev_volume_get(&volume);

			if (ret < 0)
				debug("Si47xx_IOC_VOLUME_GET failed\n");
			else if (copy_to_user
				 (argp, (void *)&volume, sizeof(u8)))
				ret = -EFAULT;
		}
		break;

	case Si47xx_IOC_DSMUTE_ON:
		debug("Si47xx_IOC_DSMUTE_ON called\n\n");

		ret = (long)Si47xx_dev_DSMUTE_ON();
		if (ret < 0)
			error("Si47xx_IOC_DSMUTE_ON failed\n");
		break;

	case Si47xx_IOC_DSMUTE_OFF:
		debug("Si47xx_IOC_DSMUTE_OFF called\n\n");

		ret = (long)Si47xx_dev_DSMUTE_OFF();
		if (ret < 0)
			error("Si47xx_IOC_DSMUTE_OFF failed\n");
		break;

	case Si47xx_IOC_MUTE_ON:
		debug("Si47xx_IOC_MUTE_ON called\n");

		ret = (long)Si47xx_dev_MUTE_ON();
		if (ret < 0)
			debug("Si47xx_IOC_MUTE_ON failed\n");
		break;

	case Si47xx_IOC_MUTE_OFF:
		debug("Si47xx_IOC_MUTE_OFF called\n");

		ret = (long)Si47xx_dev_MUTE_OFF();
		if (ret < 0)
			debug("Si47xx_IOC_MUTE_OFF failed\n");
		break;

	case Si47xx_IOC_MONO_SET:
		debug("Si47xx_IOC_MONO_SET called\n");

		ret = (long)Si47xx_dev_MONO_SET();
		if (ret < 0)
			debug("Si47xx_IOC_MONO_SET failed\n");
		break;

	case Si47xx_IOC_STEREO_SET:
		debug("Si47xx_IOC_STEREO_SET called\n");

		ret = (long)Si47xx_dev_STEREO_SET();
		if (ret < 0)
			debug("Si47xx_IOC_STEREO_SET failed\n");
		break;

	case Si47xx_IOC_RSTATE_GET:
		{
			struct dev_state_t dev_state;

			debug("Si47xx_IOC_RSTATE_GET called\n");

			ret = (long)Si47xx_dev_rstate_get(&dev_state);
			if (ret < 0)
				debug("Si47xx_IOC_RSTATE_GET failed\n");
			else if (copy_to_user(argp, (void *)&dev_state,
					      sizeof(dev_state)))
				ret = -EFAULT;
		}
		break;

	case Si47xx_IOC_RDS_DATA_GET:
		{
			struct radio_data_t data;
			debug("Si47xx_IOC_RDS_DATA_GET called\n");

			ret = (long)Si47xx_dev_RDS_data_get(&data);
			if (ret < 0)
				debug(" Si47xx_IOC_RDS_DATA_GET failed\n");
			else if (copy_to_user(argp, (void *)&data,
					      sizeof(data)))
				ret = -EFAULT;
		}
		break;

	case Si47xx_IOC_RDS_ENABLE:
		debug("Si47xx_IOC_RDS_ENABLE called\n");

		ret = (long)Si47xx_dev_RDS_ENABLE();
		if (ret < 0)
			debug("Si47xx_IOC_RDS_ENABLE failed\n");
		break;

	case Si47xx_IOC_RDS_DISABLE:
		debug("Si47xx_IOC_RDS_DISABLE called\n");

		ret = (long)Si47xx_dev_RDS_DISABLE();
		if (ret < 0)
			debug("Si47xx_IOC_RDS_DISABLE failed\n");
		break;

	case Si47xx_IOC_RDS_TIMEOUT_SET:
		{
			u32 time_out;
			debug("Si47xx_IOC_RDS_TIMEOUT_SET called\n");

			if (copy_from_user
			    ((void *)&time_out, argp, sizeof(u32)))
				ret = -EFAULT;
			else {
			ret = (long)Si47xx_dev_RDS_timeout_set(time_out);
				if (ret < 0)
					debug("Si47xx_IOC_RDS_TIMEOUT_SET "
						"failed\n");
			}
		}
		break;

	case Si47xx_IOC_SEEK_CANCEL:
		debug("Si47xx_IOC_SEEK_CANCEL called\n");

		if (Si47xx_dev_wait_flag == SEEK_WAITING) {
			Si47xx_dev_wait_flag = SEEK_CANCEL;
			wake_up_interruptible(&Si47xx_waitq);
		}
		break;

/*VNVS:START 13-OCT'09----
Switch Case statements for calling functions which reads device-id,
chip-id,power configuration, system configuration2 registers */
	case Si47xx_IOC_CHIP_ID_GET:
		{
			struct chip_id chp_id;
			debug("Si47xx_IOC_CHIP_ID called\n");

			ret = (long)Si47xx_dev_chip_id(&chp_id);
			if (ret < 0)
				debug("Si47xx_IOC_CHIP_ID failed\n");
			else if (copy_to_user(argp, (void *)&chp_id,
					      sizeof(chp_id)))
				ret = -EFAULT;
		}
		break;

	case Si47xx_IOC_DEVICE_ID_GET:
		{
			struct device_id dev_id;
			debug("Si47xx_IOC_DEVICE_ID called\n");

			ret = (long)Si47xx_dev_device_id(&dev_id);
			if (ret < 0)
				debug("Si47xx_IOC_DEVICE_ID failed\n");
			else if (copy_to_user(argp, (void *)&dev_id,
					      sizeof(dev_id)))
				ret = -EFAULT;
		}
		break;

	case Si47xx_IOC_SYS_CONFIG2_GET:
		{
			struct sys_config2 sys_conf2;
			debug("Si47xx_IOC_SYS_CONFIG2 called\n");

			ret = (long)Si47xx_dev_sys_config2(&sys_conf2);
			if (ret < 0)
				debug("Si47xx_IOC_SYS_CONFIG2 failed\n");
			else if (copy_to_user(argp, (void *)&sys_conf2,
					      sizeof(sys_conf2)))
				ret = -EFAULT;
		}
		break;

	case Si47xx_IOC_SYS_CONFIG3_GET:
		{
			struct sys_config3 sys_conf3;
			debug("Si47xx_IOC_SYS_CONFIG3 called\n");

			ret = (long)Si47xx_dev_sys_config3(&sys_conf3);
			if (ret < 0)
				debug("Si47xx_IOC_SYS_CONFIG3 failed\n");
			else if (copy_to_user(argp, (void *)&sys_conf3,
					      sizeof(sys_conf3)))
				ret = -EFAULT;
		}
		break;

	case Si47xx_IOC_POWER_CONFIG_GET:
		{
			debug("Si47xx_IOC_POWER_CONFIG called\n");
			ret = -EFAULT;
		}
		break;
/*VNVS:END*/

/*VNVS:START 18-NOV'09*/
		/*Reading AFCRL Bit */
	case Si47xx_IOC_AFCRL_GET:
		{
			u8 afc;
			debug("Si47xx_IOC_AFCRL_GET called\n");

			ret = (long)Si47xx_dev_AFCRL_get(&afc);
			if (ret < 0)
				debug("Si47xx_IOC_AFCRL_GET failed\n");
			else if (copy_to_user(argp, (void *)&afc, sizeof(u8)))
				ret = -EFAULT;
		}
		break;

		/*Setting DE-emphasis Time Constant.
		   For DE=0,TC=50us(Europe,Japan,Australia)
		   and DE=1,TC=75us(USA) */
	case Si47xx_IOC_DE_SET:
		{
			u8 de_tc;
			debug("Si47xx_IOC_DE_SET called\n");

			if (copy_from_user((void *)&de_tc, argp, sizeof(u8)))
				ret = -EFAULT;
			else {
				ret = (long)Si47xx_dev_DE_set(de_tc);
				if (ret < 0)
					debug("Si47xx_IOC_DE_SET failed\n");
			}
		}
		break;

	case Si47xx_IOC_STATUS_RSSI_GET:
		{
			struct status_rssi status;
			debug("Si47xx_IOC_STATUS_RSSI_GET called\n");

			ret = (long)Si47xx_dev_status_rssi(&status);
			if (ret < 0)
				debug("Si47xx_IOC_STATUS_RSSI_GET failed\n");
			else if (copy_to_user(argp, (void *)&status,
					      sizeof(status)))
				ret = -EFAULT;
		}
		break;

	case Si47xx_IOC_SYS_CONFIG2_SET:
		{
			struct sys_config2 sys_conf2;
			unsigned long n;
			debug("Si47xx_IOC_SYS_CONFIG2_SET called\n");

			n = copy_from_user((void *)&sys_conf2, argp,
					   sizeof(sys_conf2));
			if (n) {
				debug("Si47xx_IOC_SYS_CONFIG2_SET() : "
					"copy_from_user() has error!! "
					"Failed to read [%lu] byes!", n);
				ret = -EFAULT;
			} else {
			ret = (long)Si47xx_dev_sys_config2_set(&sys_conf2);
				if (ret < 0)
					debug("Si47xx_IOC_SYS_CONFIG2_SET"
						"failed\n");
			}
		}
		break;

	case Si47xx_IOC_SYS_CONFIG3_SET:
		{
			struct sys_config3 sys_conf3;
			unsigned long n;

			debug("Si47xx_IOC_SYS_CONFIG3_SET called\n");

			n = copy_from_user((void *)&sys_conf3, argp,
					   sizeof(sys_conf3));
			if (n) {
				debug("Si47xx_IOC_SYS_CONFIG3_SET() : "
					"copy_from_user() has error!! "
					"Failed to read [%lu] byes!", n);
				ret = -EFAULT;
			} else {
			ret = (long)Si47xx_dev_sys_config3_set(&sys_conf3);
				if (ret < 0)
					debug("Si47xx_IOC_SYS_CONFIG3_SET "
							"failed\n");
			}
		}
		break;

		/*Resetting the RDS Data Buffer */
	case Si47xx_IOC_RESET_RDS_DATA:
		{
			debug("Si47xx_IOC_RESET_RDS_DATA called\n");

			ret = (long)Si47xx_dev_reset_rds_data();
			if (ret < 0)
				error("Si47xx_IOC_RESET_RDS_DATA failed\n");
		}
		break;
/*VNVS:END*/
	default:
		debug("  ioctl default\n");
		ret = -ENOTTY;
		break;
	}

	return ret;
}

static int __devinit Si47xx_i2c_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	struct Si47xx_device_t *Si47xx_dev;
	struct si47xx_platform_data *pdata;
	int err, ret = 0;

	debug("----- %s %d\n", __func__, __LINE__);

	Si47xx_dev = kzalloc(sizeof(struct Si47xx_device_t), GFP_KERNEL);

	if (!Si47xx_dev) {
		err = -ENOMEM;
		return err;
	}

	Si47xx_dev->client = client;
	i2c_set_clientdata(client, Si47xx_dev);

	Si47xx_dev->dev = &client->dev;
	dev_set_drvdata(Si47xx_dev->dev, Si47xx_dev);

	Si47xx_dev->pdata = client->dev.platform_data;
	pdata = Si47xx_dev->pdata;

	mutex_init(&Si47xx_dev->lock);

	ret = Si47xx_dev_init(Si47xx_dev);
	if (ret < 0) {
		error("Si47xx_dev_init failed");
		goto dev_init_err;
	}

	return ret;

dev_init_err:
	mutex_destroy(&Si47xx_dev->lock);
	kfree(Si47xx_dev);
	return ret;

}

static int __devexit Si47xx_i2c_remove(struct i2c_client *client)
{
	struct Si47xx_device_t *Si47xx_dev = i2c_get_clientdata(client);
	int ret = 0;

	free_irq(client->irq, NULL);
	i2c_set_clientdata(client, NULL);
	ret = Si47xx_dev_exit();
	mutex_destroy(&Si47xx_dev->lock);
	kfree(Si47xx_dev);
	return ret;
}

static struct i2c_driver Si47xx_i2c_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "Si47xx",
		   },
	.id_table = Si47xx_id,
	.probe = Si47xx_i2c_probe,
	.remove = __devexit_p(Si47xx_i2c_remove),
	.suspend = Si47xx_suspend,
	.resume = Si47xx_resume,
};

static int Si47xx_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret = 0;

	debug("Si47xx i2c driver Si47xx_suspend called");

	if (strcmp(client->name, "Si47xx") != 0) {
		ret = -1;
		error("Si47xx_suspend: device not supported");
	} else {
		ret = Si47xx_dev_suspend();
		if (ret < 0)
			error("Si47xx_dev_disable failed");
	}

	return 0;
}

static int Si47xx_resume(struct i2c_client *client)
{
	int ret = 0;

	if (strcmp(client->name, "Si47xx") != 0) {
		ret = -1;
		error("Si47xx_resume: device not supported");
	} else {
		ret = Si47xx_dev_resume();
		if (ret < 0)
			error("Si47xx_dev_enable failed");
	}

	return 0;
}

static __init int Si47xx_i2c_drv_init(void)
{
	int ret = 0;

	debug("Si47xx i2c driver Si47xx_i2c_driver_init called");

	/*misc device registration */
	ret = misc_register(&Si47xx_misc_device);
	if (ret < 0) {
		error("Si47xx_driver_init misc_register failed\n");
		goto MISC_DREG;
	}

	ret = i2c_add_driver(&Si47xx_i2c_driver);
	if (ret < 0) {
		error("Si47xx i2c_add_driver failed");
		return ret;
	}

	init_waitqueue_head(&Si47xx_waitq);
	debug("Si47xx_driver_init successful\n");

	return ret;
MISC_DREG:
	misc_deregister(&Si47xx_misc_device);
	return ret;


}

void __exit Si47xx_i2c_drv_exit(void)
{
	debug("Si47xx i2c driver Si47xx_i2c_driver_exit called");

	i2c_del_driver(&Si47xx_i2c_driver);
	/*misc device deregistration */
	misc_deregister(&Si47xx_misc_device);
}

module_init(Si47xx_i2c_drv_init);
module_exit(Si47xx_i2c_drv_exit);
MODULE_AUTHOR("ashton seo <ashton.seo@samsung.com>");
MODULE_DESCRIPTION("Si47xx FM tuner driver");
MODULE_LICENSE("GPL");

