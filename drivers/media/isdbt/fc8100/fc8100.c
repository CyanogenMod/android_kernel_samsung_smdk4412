#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/delay.h>
#include <linux/gpio.h>

#include <plat/gpio-cfg.h>
#include <plat/gpio-core.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>

#include "fc8100.h"
#include "bbm.h"
#include "fci_oal.h"

#define FC8100_NAME		"isdbt"

int dmb_open (struct inode *inode, struct file *filp);
loff_t dmb_llseek (struct file *filp, loff_t off, int whence);
ssize_t dmb_read (struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t dmb_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static long dmb_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);
int dmb_release (struct inode *inode, struct file *filp);

/* GPIO Setting */
void dmb_hw_setting(void);
void dmb_hw_init(void);
void dmb_hw_deinit(void);

static struct file_operations dmb_fops = {
	.owner		= THIS_MODULE,
	.llseek		= dmb_llseek,
	.read		= dmb_read,
	.write		= dmb_write,
	.unlocked_ioctl		= dmb_ioctl,
	.open		= dmb_open,
	.release	= dmb_release,
};

static struct miscdevice fc8100_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = FC8100_NAME,
    .fops = &dmb_fops,
};

int dmb_open (struct inode *inode, struct file *filp)
{
	int num = MINOR(inode->i_rdev);

	/* ISDBT PWR ENABLE  FOR I2C*/
	dmb_hw_setting();
	dmb_hw_init();

	PRINTF(0, "dmb open -> minor : %d\n", num);

	return 0;
}

loff_t dmb_llseek (struct file *filp, loff_t off, int whence)
{
	PRINTF(0, "dmb llseek -> off : %08X, whence : %08X\n", off, whence);
	return 0x23;
}

ssize_t dmb_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	PRINTF(0, "dmb read -> buf : %08X, count : %08X \n", buf, count);
	return 0x33;
}

ssize_t dmb_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	PRINTF(0, "dmb write -> buf : %08X, count : %08X \n", buf, count);
	return 0x43;
}

static long dmb_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	s32 res = BBM_NOK;
	int status = 0;
	int status_usr = 0;
	ioctl_info info;
	int size;

	if (_IOC_TYPE(cmd) != IOCTL_MAGIC)
			return -EINVAL;
	if (_IOC_NR(cmd) >= IOCTL_MAXNR)
			return -EINVAL;

	size = _IOC_SIZE(cmd);

	switch (cmd) {
		case IOCTL_DMB_RESET:
			res = BBM_RESET(0);
			break;
		case IOCTL_DMB_INIT:
			res = BBM_INIT(0);
			break;
		case IOCTL_DMB_BYTE_READ:
			status = copy_from_user((void *)&info, (void *)arg, size);
			res = BBM_BYTE_READ(0, (u16)info.buff[0], (u8 *)(&info.buff[1]));
			status_usr = copy_to_user((void *)arg, (void *)&info, size);
			break;
		case IOCTL_DMB_WORD_READ:
			status = copy_from_user((void *)&info, (void *)arg, size);
			res = BBM_WORD_READ(0, (u16)info.buff[0], (u16 *)(&info.buff[1]));
			status_usr = copy_to_user((void *)arg, (void *)&info, size);
			break;
		case IOCTL_DMB_LONG_READ:
			status = copy_from_user((void *)&info, (void *)arg, size);
			res = BBM_LONG_READ(0, (u16)info.buff[0], (u32 *)(&info.buff[1]));
			status_usr = copy_to_user((void *)arg, (void *)&info, size);
			break;
		case IOCTL_DMB_BULK_READ:
			status = copy_from_user((void *)&info, (void *)arg, size);
			res = BBM_BULK_READ(0, (u16)info.buff[0], (u8 *)(&info.buff[2]), info.buff[1]);
			status_usr = copy_to_user((void *)arg, (void *)&info, size);
			break;
		case IOCTL_DMB_BYTE_WRITE:
			status = copy_from_user((void *)&info, (void *)arg, size);
			res = BBM_BYTE_WRITE(0, (u16)info.buff[0], (u8)info.buff[1]);
			break;
		case IOCTL_DMB_WORD_WRITE:
			status = copy_from_user((void *)&info, (void *)arg, size);
			res = BBM_WORD_WRITE(0, (u16)info.buff[0], (u16)info.buff[1]);
			break;
		case IOCTL_DMB_LONG_WRITE:
			status = copy_from_user((void *)&info, (void *)arg, size);
			res = BBM_LONG_WRITE(0, (u16)info.buff[0], (u32)info.buff[1]);
			break;
		case IOCTL_DMB_BULK_WRITE:
			status = copy_from_user((void *)&info, (void *)arg, size);
			res = BBM_BULK_WRITE(0, (u16)info.buff[0], (u8 *)(&info.buff[2]), info.buff[1]);
			break;
		case IOCTL_DMB_TUNER_SELECT:
			status = copy_from_user((void *)&info, (void *)arg, size);
			res = BBM_TUNER_SELECT(0, (u32)info.buff[0], 0);
			break;
		case IOCTL_DMB_TUNER_READ:
			status = copy_from_user((void *)&info, (void *)arg, size);
			res = BBM_TUNER_READ(0, (u8)info.buff[0], (u8)info.buff[1],  (u8 *)(&info.buff[3]), (u8)info.buff[2]);
			status_usr = copy_to_user((void *)arg, (void *)&info, size);
			break;
		case IOCTL_DMB_TUNER_WRITE:
			status = copy_from_user((void *)&info, (void *)arg, size);
			res = BBM_TUNER_WRITE(0, (u8)info.buff[0], (u8)info.buff[1], (u8 *)(&info.buff[3]), (u8)info.buff[2]);
			break;
		case IOCTL_DMB_TUNER_SET_FREQ:
			status = copy_from_user((void *)&info, (void *)arg, size);
			res = BBM_TUNER_SET_FREQ(0, (u8)info.buff[0]);
			break;
		case IOCTL_DMB_POWER_ON:
			PRINTF(0, "DMB_POWER_ON enter..\n");
			dmb_hw_init();
			break;
		case IOCTL_DMB_POWER_OFF:
			PRINTF(0, "DMB_POWER_OFF enter..\n");
			dmb_hw_deinit();
			break;
	}
	if ((status < 0) || (status_usr < 0)) {
		PRINTF(0, " copy to user or copy from user : ERROR..\n");
		return -EINVAL;
	}

	/* return status */
	if (res < 0)
			res = -BBM_NOK;
	else
			res = BBM_OK;

	return res;
}

int dmb_release(struct inode *inode, struct file *filp)
{
	/* ISDBT PWR ENABLE  FOR I2C*/
	PRINTF(0, "dmb release\n");
	return 0;
}

void dmb_hw_setting(void)
{
	s3c_gpio_cfgpin(GPIO_ISDBT_SDA_28V, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_ISDBT_SDA_28V, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_ISDBT_SCL_28V, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_ISDBT_SCL_28V, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_ISDBT_PWR_EN, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_ISDBT_PWR_EN, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_ISDBT_RST, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_ISDBT_RST, GPIO_LEVEL_HIGH);
}

void dmb_init_hw_setting(void)
{
	s3c_gpio_cfgpin(GPIO_ISDBT_SDA_28V, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_ISDBT_SDA_28V, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_ISDBT_SCL_28V, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_ISDBT_SCL_28V, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_ISDBT_PWR_EN, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_ISDBT_PWR_EN, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_ISDBT_RST, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_ISDBT_RST, GPIO_LEVEL_LOW);
}

void dmb_hw_init(void)
{
	mdelay(1);
	gpio_set_value(GPIO_ISDBT_PWR_EN, GPIO_LEVEL_HIGH);
	mdelay(1);
	gpio_set_value(GPIO_ISDBT_RST, GPIO_LEVEL_HIGH);
	mdelay(100);
}

void dmb_hw_deinit(void)
{
	mdelay(1);
	gpio_set_value(GPIO_ISDBT_RST, GPIO_LEVEL_LOW);
	mdelay(1);
	gpio_set_value(GPIO_ISDBT_PWR_EN, GPIO_LEVEL_LOW);
	mdelay(100);
}


int dmb_init(void)
{
	int result;

	PRINTF(0, "dmb dmb_init\n");

	dmb_init_hw_setting();
/*
	dmb_hw_init();
*/
	/*misc device registration*/
	result = misc_register(&fc8100_misc_device);

	if (result < 0)
		return result;

	BBM_HOSTIF_SELECT(0, BBM_I2C);

	dmb_hw_deinit();

	return 0;
}

void dmb_exit(void)
{
	PRINTF(0, "dmb dmb_exit\n");

	BBM_HOSTIF_DESELECT(0);
	dmb_hw_deinit();

	/*misc device deregistration*/
	misc_deregister(&fc8100_misc_device);
}

module_init(dmb_init);
module_exit(dmb_exit);

MODULE_LICENSE("Dual BSD/GPL");
