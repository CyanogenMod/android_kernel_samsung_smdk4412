
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

#include <plat/gpio-cfg.h>
#include <mach/gpio.h>

#include "Si4709_i2c_drv.h"
#include "Si4709_dev.h"
#include "Si4709_ioctl.h"
#include "Si4709_common.h"

/*******************************************************/

/*static functions*/

/*file operatons*/
static int Si4709_open(struct inode *, struct file *);
static int Si4709_release(struct inode *, struct file *);
static long Si4709_ioctl(struct file *, unsigned int, unsigned long);

/*ISR*/
static irqreturn_t Si4709_isr(int irq, void *unused);
/* static	void	__iomem		*gpio_mask_mem; */
/**********************************************************/

static const struct file_operations Si4709_fops = {
	.owner = THIS_MODULE,
	.open = Si4709_open,
	.unlocked_ioctl = Si4709_ioctl,
	.release = Si4709_release,
};

static struct miscdevice Si4709_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "fmradio",
	.fops = &Si4709_fops,
};

/*VNVS:START 13-OCT'09----
dummy struct which is used as a cookie for FM Radio interrupt */
struct fm_radio {
	int i;
	int j;
};

struct fm_radio fm_radio_1;
/*VNVS:END*/

wait_queue_head_t Si4709_waitq;

unsigned int Si4709_int;
unsigned int Si4709_irq;

/***************************************************************/

static int Si4709_open(struct inode *inode, struct file *filp)
{
	debug("Si4709_open called\n");

	return nonseekable_open(inode, filp);
}

static int Si4709_release(struct inode *inode, struct file *filp)
{
	debug("Si4709_release called\n\n");

	return 0;
}

static long Si4709_ioctl(struct file *filp, unsigned int ioctl_cmd,
							unsigned long arg)
{
	long ret = 0;
	void __user *argp = (void __user *)arg;

	debug("Si4709 ioctl 0x%x", ioctl_cmd);

	if (_IOC_TYPE(ioctl_cmd) != Si4709_IOC_MAGIC) {
		error("Inappropriate ioctl 1 0x%x", ioctl_cmd);
		return -ENOTTY;
	}

	if (_IOC_NR(ioctl_cmd) > Si4709_IOC_NR_MAX) {
		error("Inappropriate ioctl 2 0x%x", ioctl_cmd);
		return -ENOTTY;
	}


	switch (ioctl_cmd) {
	case Si4709_IOC_POWERUP:
		debug("Si4709_IOC_POWERUP called\n\n");

		ret = (long)Si4709_dev_powerup();
		if (ret < 0)
			error("Si4709_IOC_POWERUP failed\n");
		break;

	case Si4709_IOC_POWERDOWN:
		debug("Si4709_IOC_POWERDOWN called\n");

		ret = (long)Si4709_dev_powerdown();
		if (ret < 0)
			error("Si4709_IOC_POWERDOWN failed\n");
		break;

	case Si4709_IOC_BAND_SET:
		{
			int band;
			debug("Si4709_IOC_BAND_SET called\n\n");

			if (copy_from_user((void *)&band, argp, sizeof(int)))
				ret = -EFAULT;
			else {
				ret = (long)Si4709_dev_band_set(band);
				if (ret < 0)
					error("Si4709_IOC_BAND_SET failed\n");
			}
		}
		break;

	case Si4709_IOC_CHAN_SPACING_SET:
		{
			int ch_spacing;
			debug("Si4709_IOC_CHAN_SPACING_SET called\n");

			if (copy_from_user
			    ((void *)&ch_spacing, argp, sizeof(int)))
				ret = -EFAULT;
			else {
			ret = (long)Si4709_dev_ch_spacing_set(ch_spacing);
				if (ret < 0)
					error("Si4709_IOC_CHAN_SPACING_SET "
						"failed\n");
			}
		}
		break;

	case Si4709_IOC_CHAN_SELECT:
		{
			u32 frequency;
			debug("Si4709_IOC_CHAN_SELECT called\n");

			if (copy_from_user
			    ((void *)&frequency, argp, sizeof(u32)))
				ret = -EFAULT;
			else {
				ret = (long)Si4709_dev_chan_select(frequency);
				if (ret < 0)
					error("Si4709_IOC_CHAN_SELECT "
					"failed\n");
			}
		}
		break;

	case Si4709_IOC_CHAN_GET:
		{
			u32 frequency = 0;
			debug("Si4709_IOC_CHAN_GET called\n");

			ret = (long)Si4709_dev_chan_get(&frequency);
			if (ret < 0)
				error("Si4709_IOC_CHAN_GET failed\n");
			else if (copy_to_user
				 (argp, (void *)&frequency, sizeof(u32)))
				ret = -EFAULT;
		}
		break;

	case Si4709_IOC_SEEK_FULL:
		{
			u32 frequency = 0;
			debug("Si4709_IOC_SEEK_FULL called\n");

			ret = (long)Si4709_dev_seek_full(&frequency);
			if (ret < 0)
				debug("Si4709_IOC_SEEK_FULL failed\n");
			else if (copy_to_user
				 (argp, (void *)&frequency, sizeof(u32)))
				ret = -EFAULT;
		}
		break;

	case Si4709_IOC_SEEK_UP:
		{
			u32 frequency = 0;
			debug("Si4709_IOC_SEEK_UP called\n");

			ret = (long)Si4709_dev_seek_up(&frequency);
			if (ret < 0)
				error("Si4709_IOC_SEEK_UP failed\n");
			else if (copy_to_user
				 (argp, (void *)&frequency, sizeof(u32)))
				ret = -EFAULT;
		}
		break;

	case Si4709_IOC_SEEK_DOWN:
		{
			u32 frequency = 0;
			debug("Si4709_IOC_SEEK_DOWN called\n");

			ret = (long)Si4709_dev_seek_down(&frequency);
			if (ret < 0)
				error("Si4709_IOC_SEEK_DOWN failed\n");
			else if (copy_to_user
				 (argp, (void *)&frequency, sizeof(u32)))
				ret = -EFAULT;
		}
		break;

	case Si4709_IOC_RSSI_SEEK_TH_SET:
		{
			u8 RSSI_seek_th;
			debug("Si4709_IOC_RSSI_SEEK_TH_SET called\n");

			if (copy_from_user
			    ((void *)&RSSI_seek_th, argp, sizeof(u8)))
				ret = -EFAULT;
			else {
			ret = (long)Si4709_dev_RSSI_seek_th_set(RSSI_seek_th);
				if (ret < 0)
					error("Si4709_IOC_RSSI_SEEK_TH_SET "
						       "failed\n");
			}
		}
		break;

	case Si4709_IOC_SEEK_SNR_SET:
		{
			u8 seek_SNR_th;
			debug("Si4709_IOC_SEEK_SNR_SET called\n");

			if (copy_from_user
			    ((void *)&seek_SNR_th, argp, sizeof(u8)))
				ret = -EFAULT;
			else {
			ret = (long)Si4709_dev_seek_SNR_th_set(seek_SNR_th);
				if (ret < 0)
					error("Si4709_IOC_SEEK_SNR_SET "
					"failed\n");
			}
		}
		break;

	case Si4709_IOC_SEEK_CNT_SET:
		{
			u8 seek_FM_ID_th;
			debug("Si4709_IOC_SEEK_CNT_SET called\n");

			if (copy_from_user
			    ((void *)&seek_FM_ID_th, argp, sizeof(u8)))
				ret = -EFAULT;
			else {
				ret =
			(long)Si4709_dev_seek_FM_ID_th_set(seek_FM_ID_th);
				if (ret < 0)
					error("Si4709_IOC_SEEK_CNT_SET "
					"failed\n");
			}
		}
		break;

	case Si4709_IOC_CUR_RSSI_GET:
		{
			struct rssi_snr_t data;
			debug("Si4709_IOC_CUR_RSSI_GET called\n");

			ret = (long)Si4709_dev_cur_RSSI_get(&data);
			if (ret < 0)
				error("Si4709_IOC_CUR_RSSI_GET failed\n");
			else if (copy_to_user(argp, (void *)&data,
					      sizeof(data)))
				ret = -EFAULT;

			debug("curr_rssi:%d\ncurr_rssi_th:%d\ncurr_snr:%d\n",
			      data.curr_rssi, data.curr_rssi_th, data.curr_snr);
		}
		break;

	case Si4709_IOC_VOLEXT_ENB:
		debug("Si4709_IOC_VOLEXT_ENB called\n");

		ret = (long)Si4709_dev_VOLEXT_ENB();
		if (ret < 0)
			error("Si4709_IOC_VOLEXT_ENB failed\n");
		break;

	case Si4709_IOC_VOLEXT_DISB:
		debug("Si4709_IOC_VOLEXT_DISB called\n");

		ret = (long)Si4709_dev_VOLEXT_DISB();
		if (ret < 0)
			error("Si4709_IOC_VOLEXT_DISB failed\n");
		break;

	case Si4709_IOC_VOLUME_SET:
		{
			u8 volume;
			if (copy_from_user((void *)&volume, argp, sizeof(u8)))
				ret = -EFAULT;
			else {
				debug("Si4709_IOC_VOLUME_SET called "
					"vol %d\n", volume);
				ret = (long)Si4709_dev_volume_set(volume);
				if (ret < 0)
					error("Si4709_IOC_VOLUME_SET failed\n");
			}
		}
		break;

	case Si4709_IOC_VOLUME_GET:
		{
			u8 volume;
			debug("Si4709_IOC_VOLUME_GET called\n");

			ret = (long)Si4709_dev_volume_get(&volume);
			if (ret < 0)
				error("Si4709_IOC_VOLUME_GET failed\n");
			else if (copy_to_user
				 (argp, (void *)&volume, sizeof(u8)))
				ret = -EFAULT;
		}
		break;

	case Si4709_IOC_DSMUTE_ON:
		debug("Si4709_IOC_DSMUTE_ON called\n\n");

		ret = (long)Si4709_dev_DSMUTE_ON();
		if (ret < 0)
			error("Si4709_IOC_DSMUTE_ON failed\n");
		break;

	case Si4709_IOC_DSMUTE_OFF:
		debug("Si4709_IOC_DSMUTE_OFF called\n\n");

		ret = (long)Si4709_dev_DSMUTE_OFF();
		if (ret < 0)
			error("Si4709_IOC_DSMUTE_OFF failed\n");
		break;

	case Si4709_IOC_MUTE_ON:
		debug("Si4709_IOC_MUTE_ON called\n");

		ret = (long)Si4709_dev_MUTE_ON();
		if (ret < 0)
			error("Si4709_IOC_MUTE_ON failed\n");
		break;

	case Si4709_IOC_MUTE_OFF:
		debug("Si4709_IOC_MUTE_OFF called\n");

		ret = (long)Si4709_dev_MUTE_OFF();
		if (ret < 0)
			error("Si4709_IOC_MUTE_OFF failed\n");
		break;

	case Si4709_IOC_MONO_SET:
		debug("Si4709_IOC_MONO_SET called\n");

		ret = (long)Si4709_dev_MONO_SET();
		if (ret < 0)
			error("Si4709_IOC_MONO_SET failed\n");
		break;

	case Si4709_IOC_STEREO_SET:
		debug("Si4709_IOC_STEREO_SET called\n");

		ret = (long)Si4709_dev_STEREO_SET();
		if (ret < 0)
			error("Si4709_IOC_STEREO_SET failed\n");
		break;

	case Si4709_IOC_RSTATE_GET:
		{
			struct dev_state_t dev_state;

			debug("Si4709_IOC_RSTATE_GET called\n");

			ret = (long)Si4709_dev_rstate_get(&dev_state);
			if (ret < 0)
				error("Si4709_IOC_RSTATE_GET failed\n");
			else if (copy_to_user(argp, (void *)&dev_state,
					      sizeof(dev_state)))
				ret = -EFAULT;
		}
		break;

	case Si4709_IOC_RDS_DATA_GET:
		{
			struct radio_data_t data;
			debug("Si4709_IOC_RDS_DATA_GET called\n");

			ret = (long)Si4709_dev_RDS_data_get(&data);
			if (ret < 0)
				error("Si4709_IOC_RDS_DATA_GET failed\n");
			else if (copy_to_user(argp, (void *)&data,
					      sizeof(data)))
				ret = -EFAULT;
		}
		break;

	case Si4709_IOC_RDS_ENABLE:
		debug("Si4709_IOC_RDS_ENABLE called\n");

		ret = (long)Si4709_dev_RDS_ENABLE();
		if (ret < 0)
			error("Si4709_IOC_RDS_ENABLE failed\n");
		break;

	case Si4709_IOC_RDS_DISABLE:
		debug("Si4709_IOC_RDS_DISABLE called\n");

		ret = (long)Si4709_dev_RDS_DISABLE();
		if (ret < 0)
			error("Si4709_IOC_RDS_DISABLE failed\n");
		break;

	case Si4709_IOC_RDS_TIMEOUT_SET:
		{
			u32 time_out;
			debug("Si4709_IOC_RDS_TIMEOUT_SET called\n");

			if (copy_from_user
			    ((void *)&time_out, argp, sizeof(u32)))
				ret = -EFAULT;
			else {
			ret = (long)Si4709_dev_RDS_timeout_set(time_out);
				if (ret < 0)
					error("Si4709_IOC_RDS_TIMEOUT_SET "
						"failed\n");
			}
		}
		break;

	case Si4709_IOC_SEEK_CANCEL:
		debug("Si4709_IOC_SEEK_CANCEL called\n");

		if (Si4709_dev_wait_flag == SEEK_WAITING) {
			Si4709_dev_wait_flag = SEEK_CANCEL;
			wake_up_interruptible(&Si4709_waitq);
		}
		break;

/*VNVS:START 13-OCT'09----
Switch Case statements for calling functions which reads device-id,
chip-id,power configuration, system configuration2 registers */
	case Si4709_IOC_CHIP_ID_GET:
		{
			struct chip_id chp_id;
			debug("Si4709_IOC_CHIP_ID called\n");

			ret = (long)Si4709_dev_chip_id(&chp_id);
			if (ret < 0)
				error("Si4709_IOC_CHIP_ID failed\n");
			else if (copy_to_user(argp, (void *)&chp_id,
					      sizeof(chp_id)))
				ret = -EFAULT;
		}
		break;

	case Si4709_IOC_DEVICE_ID_GET:
		{
			struct device_id dev_id;
			debug("Si4709_IOC_DEVICE_ID called\n");

			ret = (long)Si4709_dev_device_id(&dev_id);
			if (ret < 0)
				error("Si4709_IOC_DEVICE_ID failed\n");
			else if (copy_to_user(argp, (void *)&dev_id,
					      sizeof(dev_id)))
				ret = -EFAULT;
		}
		break;

	case Si4709_IOC_SYS_CONFIG2_GET:
		{
			struct sys_config2 sys_conf2;
			debug("Si4709_IOC_SYS_CONFIG2 called\n");

			ret = (long)Si4709_dev_sys_config2(&sys_conf2);
			if (ret < 0)
				error("Si4709_IOC_SYS_CONFIG2 failed\n");
			else if (copy_to_user(argp, (void *)&sys_conf2,
					      sizeof(sys_conf2)))
				ret = -EFAULT;
		}
		break;

	case Si4709_IOC_SYS_CONFIG3_GET:
		{
			struct sys_config3 sys_conf3;
			debug("Si4709_IOC_SYS_CONFIG3 called\n");

			ret = (long)Si4709_dev_sys_config3(&sys_conf3);
			if (ret < 0)
				error("Si4709_IOC_SYS_CONFIG3 failed\n");
			else if (copy_to_user(argp, (void *)&sys_conf3,
					      sizeof(sys_conf3)))
				ret = -EFAULT;
		}
		break;

	case Si4709_IOC_POWER_CONFIG_GET:
		{
			struct power_config pow_conf;
			debug("Si4709_IOC_POWER_CONFIG called\n");

			ret = (long)Si4709_dev_power_config(&pow_conf);
			if (ret < 0)
				error("Si4709_IOC_POWER_CONFIG failed\n");
			else if (copy_to_user(argp, (void *)&pow_conf,
					      sizeof(pow_conf)))
				ret = -EFAULT;
		}
		break;
/*VNVS:END*/

/*VNVS:START 18-NOV'09*/
		/*Reading AFCRL Bit */
	case Si4709_IOC_AFCRL_GET:
		{
			u8 afc;
			debug("Si4709_IOC_AFCRL_GET called\n");

			ret = (long)Si4709_dev_AFCRL_get(&afc);
			if (ret < 0)
				error("Si4709_IOC_AFCRL_GET failed\n");
			else if (copy_to_user(argp, (void *)&afc, sizeof(u8)))
				ret = -EFAULT;
		}
		break;

		/*Setting DE-emphasis Time Constant.
		   For DE=0,TC=50us(Europe,Japan,Australia)
		   and DE=1,TC=75us(USA) */
	case Si4709_IOC_DE_SET:
		{
			u8 de_tc;
			debug("Si4709_IOC_DE_SET called\n");

			if (copy_from_user((void *)&de_tc, argp, sizeof(u8)))
				ret = -EFAULT;
			else {
				ret = (long)Si4709_dev_DE_set(de_tc);
				if (ret < 0)
					error("Si4709_IOC_DE_SET failed\n");
			}
		}
		break;

	case Si4709_IOC_STATUS_RSSI_GET:
		{
			struct status_rssi status;
			debug("Si4709_IOC_STATUS_RSSI_GET called\n");

			ret = (long)Si4709_dev_status_rssi(&status);
			if (ret < 0)
				error("Si4709_IOC_STATUS_RSSI_GET failed\n");
			else if (copy_to_user(argp, (void *)&status,
					      sizeof(status)))
				ret = -EFAULT;
		}
		break;

	case Si4709_IOC_SYS_CONFIG2_SET:
		{
			struct sys_config2 sys_conf2;
			unsigned long n;
			debug("Si4709_IOC_SYS_CONFIG2_SET called\n");

			n = copy_from_user((void *)&sys_conf2, argp,
					   sizeof(sys_conf2));
			if (n) {
				debug("Si4709_IOC_SYS_CONFIG2_SET() : "
					"copy_from_user() has error!! "
					"Failed to read [%lu] byes!", n);
				ret = -EFAULT;
			} else {
			ret = (long)Si4709_dev_sys_config2_set(&sys_conf2);
				if (ret < 0)
					error("Si4709_IOC_SYS_CONFIG2_SET"
						"failed\n");
			}
		}
		break;

	case Si4709_IOC_SYS_CONFIG3_SET:
		{
			struct sys_config3 sys_conf3;
			unsigned long n;

			debug("Si4709_IOC_SYS_CONFIG3_SET called\n");

			n = copy_from_user((void *)&sys_conf3, argp,
					   sizeof(sys_conf3));
			if (n < 0) {
				error("Si4709_IOC_SYS_CONFIG3_SET() : "
					"copy_from_user() has error!! "
					"Failed to read [%lu] byes!", n);
				ret = -EFAULT;
			} else {
			ret = (long)Si4709_dev_sys_config3_set(&sys_conf3);
				if (ret < 0)
					error("Si4709_IOC_SYS_CONFIG3_SET "
							"failed\n");
			}
		}
		break;

		/*Resetting the RDS Data Buffer */
	case Si4709_IOC_RESET_RDS_DATA:
		{
			debug("Si4709_IOC_RESET_RDS_DATA called\n");

			ret = (long)Si4709_dev_reset_rds_data();
			if (ret < 0)
				error("Si4709_IOC_RESET_RDS_DATA failed\n");
		}
		break;
/*VNVS:END*/

	default:
		error("  ioctl default\n");
		ret = -ENOTTY;
		break;
	}

	return ret;
}

static irqreturn_t Si4709_isr(int irq, void *unused)
{
	debug("Si4709_isr: FM device called IRQ: %d", irq);
#ifdef RDS_INTERRUPT_ON_ALWAYS
	if ((Si4709_dev_wait_flag == SEEK_WAITING) ||
	    (Si4709_dev_wait_flag == TUNE_WAITING)) {
		debug("Si4709_isr: FM Seek/Tune Interrupt "
			"called IRQ %d\n", irq);
		Si4709_dev_wait_flag = WAIT_OVER;
		wake_up_interruptible(&Si4709_waitq);
	} else if (Si4709_RDS_flag == RDS_WAITING) {	/* RDS Interrupt */
		debug_rds("Si4709_isr: FM RDS Interrupt "
			"called IRQ %d", irq);
		RDS_Data_Available++;
		RDS_Groups_Available_till_now++;

		debug_rds("RDS_Groups_Available_till_now b/w "
			"Power ON/OFF : %d",
			  RDS_Groups_Available_till_now);

		if (RDS_Data_Available > 1)
			RDS_Data_Lost++;

		if (!work_pending(&Si4709_work))
			queue_work(Si4709_wq, &Si4709_work);
	}
#else
	if ((Si4709_dev_wait_flag == SEEK_WAITING) ||
	    (Si4709_dev_wait_flag == TUNE_WAITING) ||
	    (Si4709_dev_wait_flag == RDS_WAITING)) {
		Si4709_dev_wait_flag = WAIT_OVER;
		wake_up_interruptible(&Si4709_waitq);
	}
#endif
	return IRQ_HANDLED;
}

/************************************************************/

void debug_ioctls(void)
{
	debug("------------------------------------------------\n");

	debug("Si4709_IOC_POWERUP 0x%x", Si4709_IOC_POWERUP);

	debug("Si4709_IOC_POWERDOWN 0x%x", Si4709_IOC_POWERDOWN);

	debug("Si4709_IOC_BAND_SET 0x%x", Si4709_IOC_BAND_SET);

	debug("Si4709_IOC_CHAN_SPACING_SET 0x%x", Si4709_IOC_CHAN_SPACING_SET);

	debug("Si4709_IOC_CHAN_SELECT 0x%x", Si4709_IOC_CHAN_SELECT);

	debug("Si4709_IOC_CHAN_GET 0x%x", Si4709_IOC_CHAN_GET);

	debug("Si4709_IOC_SEEK_UP 0x%x", Si4709_IOC_SEEK_UP);

	debug("Si4709_IOC_SEEK_DOWN 0x%x", Si4709_IOC_SEEK_DOWN);

	/*VNVS:28OCT'09---- Si4709_IOC_SEEK_AUTO is disabled as of now */
	/* debug("Si4709_IOC_SEEK_AUTO 0x%x", Si4709_IOC_SEEK_AUTO); */

	debug("Si4709_IOC_RSSI_SEEK_TH_SET 0x%x", Si4709_IOC_RSSI_SEEK_TH_SET);

	debug("Si4709_IOC_SEEK_SNR_SET 0x%x", Si4709_IOC_SEEK_SNR_SET);

	debug("Si4709_IOC_SEEK_CNT_SET 0x%x", Si4709_IOC_SEEK_CNT_SET);

	debug("Si4709_IOC_CUR_RSSI_GET 0x%x", Si4709_IOC_CUR_RSSI_GET);

	debug("Si4709_IOC_VOLEXT_ENB 0x%x", Si4709_IOC_VOLEXT_ENB);

	debug("Si4709_IOC_VOLEXT_DISB 0x%x", Si4709_IOC_VOLEXT_DISB);

	debug("Si4709_IOC_VOLUME_SET 0x%x", Si4709_IOC_VOLUME_SET);

	debug("Si4709_IOC_VOLUME_GET 0x%x", Si4709_IOC_VOLUME_GET);

	debug("Si4709_IOC_MUTE_ON 0x%x", Si4709_IOC_MUTE_ON);

	debug("Si4709_IOC_MUTE_OFF 0x%x", Si4709_IOC_MUTE_OFF);

	debug("Si4709_IOC_MONO_SET 0x%x", Si4709_IOC_MONO_SET);

	debug("Si4709_IOC_STEREO_SET 0x%x", Si4709_IOC_STEREO_SET);

	debug("Si4709_IOC_RSTATE_GET 0x%x", Si4709_IOC_RSTATE_GET);

	debug("Si4709_IOC_RDS_DATA_GET 0x%x", Si4709_IOC_RDS_DATA_GET);

	debug("Si4709_IOC_RDS_ENABLE 0x%x", Si4709_IOC_RDS_ENABLE);

	debug("Si4709_IOC_RDS_DISABLE 0x%x", Si4709_IOC_RDS_DISABLE);

	debug("Si4709_IOC_RDS_TIMEOUT_SET 0x%x", Si4709_IOC_RDS_TIMEOUT_SET);

	debug("Si4709_IOC_DEVICE_ID_GET 0x%x", Si4709_IOC_DEVICE_ID_GET);

	debug("Si4709_IOC_CHIP_ID_GET 0x%x", Si4709_IOC_CHIP_ID_GET);

	debug("Si4709_IOC_SYS_CONFIG2_GET 0x%x", Si4709_IOC_SYS_CONFIG2_GET);

	debug("Si4709_IOC_POWER_CONFIG_GET 0x%x", Si4709_IOC_POWER_CONFIG_GET);

	debug("Si4709_IOC_AFCRL_GET 0x%x", Si4709_IOC_AFCRL_GET);

	debug("Si4709_IOC_DE_SET 0x%x", Si4709_IOC_DE_SET);

	debug("Si4709_IOC_DSMUTE_ON 0x%x", Si4709_IOC_DSMUTE_ON);

	debug("Si4709_IOC_DSMUTE_OFF 0x%x", Si4709_IOC_DSMUTE_OFF);

	debug("Si4709_IOC_RESET_RDS_DATA 0x%x", Si4709_IOC_RESET_RDS_DATA);

	debug("------------------------------------------------\n");
}

int __init Si4709_driver_init(void)
{
	int ret = 0;

	debug("Si4709_driver_init called\n");

	/*Initialize the Si4709 dev mutex */
	Si4709_dev_mutex_init();

	/*misc device registration */
	ret = misc_register(&Si4709_misc_device);
	if (ret < 0) {
		error("Si4709_driver_init misc_register failed\n");
		return ret;
	}

	if (system_rev >= 0x7) {
		Si4709_int = GPIO_FM_INT_REV07;
		Si4709_irq = gpio_to_irq(GPIO_FM_INT_REV07);
	} else {
		Si4709_int = GPIO_FM_INT;
		Si4709_irq = gpio_to_irq(GPIO_FM_INT);
	}

	s3c_gpio_cfgpin(Si4709_int, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(Si4709_int, S3C_GPIO_PULL_NONE);

	irq_set_irq_type(Si4709_irq, IRQ_TYPE_EDGE_FALLING);

	/*KGVS: Configuring the GPIO_FM_INT in mach-jupiter.c */
	ret = request_irq(Si4709_irq, Si4709_isr, IRQF_DISABLED,
			  "Si4709", NULL);
	if (ret) {
		error("Si4709_driver_init request_irq "
			"failed %d", Si4709_int);
		goto MISC_DREG;
	} else
		debug("Si4709_driver_init request_irq "
		"success %d", Si4709_int);

	if (gpio_is_valid(FM_RESET)) {
		if (gpio_request(FM_RESET, FM_PORT))
			debug(KERN_ERR "Failed to request "
			"FM_RESET!\n");
		gpio_direction_output(FM_RESET, GPIO_LEVEL_LOW);
	}

	/*VNVS: 13-OCT'09----
	Initially Pulling the interrupt pin HIGH
	as the FM Radio device gives 5ms low pulse*/
	s3c_gpio_setpull(Si4709_int, S3C_GPIO_PULL_UP);

	/****Resetting the device****/
	gpio_set_value(FM_RESET, GPIO_LEVEL_LOW);
	gpio_set_value(FM_RESET, GPIO_LEVEL_HIGH);
	/*VNVS: 13-OCT'09---- Freeing the FM_RESET pin */

	gpio_free(FM_RESET);

	/*Add the i2c driver */
	ret = Si4709_i2c_drv_init();
	if (ret < 0)
		goto MISC_IRQ_DREG;

	init_waitqueue_head(&Si4709_waitq);

	debug("Si4709_driver_init successful\n");

	return ret;

MISC_IRQ_DREG:
	free_irq(Si4709_irq, NULL);
MISC_DREG:
	misc_deregister(&Si4709_misc_device);

	return ret;
}

void __exit Si4709_driver_exit(void)
{
	debug("Si4709_driver_exit called\n");

	/*Delete the i2c driver */
	Si4709_i2c_drv_exit();
	free_irq(Si4709_irq, NULL);

	/*misc device deregistration */
	misc_deregister(&Si4709_misc_device);
}

module_init(Si4709_driver_init);
module_exit(Si4709_driver_exit);
MODULE_AUTHOR("Varun Mahajan <m.varun@samsung.com>");
MODULE_DESCRIPTION("Si4709 FM tuner driver");
MODULE_LICENSE("GPL");
