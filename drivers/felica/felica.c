/*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*/

#ifdef CONFIG_FELICA

#include "felica.h"
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/termios.h>
#include <linux/serial_core.h>
#include <linux/uaccess.h>

#include <linux/types.h>
#include <asm/smc.h>


/******************************************************************************
 * log
 ******************************************************************************/

#ifdef FELICA_DEBUG
#define FELICA_LOG_DEBUG(fmt, args...) printk(KERN_INFO fmt, ## args)
#else
#define FELICA_LOG_DEBUG(fmt, args...)
#endif
#define FELICA_LOG_ERR(fmt, args...) printk(KERN_ERR fmt, ## args)

/******************************************************************************
 * global variable
 ******************************************************************************/

static struct class *felica_class;

/* storages for communicate to netlink */
static int gfa_open_cnt;
static int gfa_pid;
static int gfa_connect_flag;
static struct sock *gfanl_sk;
static char gfa_send_str[FELICA_NL_MSG_SIZE];
static char gfa_rcv_str[FELICA_NL_MSG_SIZE];
static int gfa_wait_flag;

/* R/W functions availability information storage */
static char gfelica_rw_status;

/* IRQ data storage for INT terminal monitoring */
struct felica_int_irqdata {
	struct delayed_work work;
	wait_queue_head_t read_wait;
	int irq_done;
	int open_flag;
};
static struct felica_int_irqdata gint_irq;
static struct felica_int_irqdata *pgint_irq = &gint_irq;

/* storages for access restriction */
static uid_t gmfc_uid = -1;
static uid_t gmfl_uid = -1;
static uid_t grwm_uid = -1;
static uid_t gdiag_uid = -1;
/* package name's storage for access restriction */
static char gdiag_name[DIAG_NAME_MAXSIZE + 1];
static uid_t gant_uid = -1;
static int gi2c_address;
static char gi2c_antaddress;
static char gi2c_lockaddress;
static struct i2c_msg gread_msgs[] = {
	{
		.addr	= 0,
		.flags	= 0,
		.len	= 0,
		.buf	= NULL,
	},
	{
		.addr	= 0,
		.flags	= 0,
		.len	= 0,
		.buf	= NULL,
	},
};

static struct i2c_msg gwrite_msgs[] = {
	{
		.addr	= 0,
		.flags	= 0,
		.len	= 0,
		.buf	= NULL,
	},
};
#define  FELICA_UART1RX        EXYNOS4_GPA0(4)
#define  FELICA_UART3RX        EXYNOS4_GPA1(4)


/******************************************************************************
 * /dev/felica
 ******************************************************************************/

/* character device definition */
static int felica_uart_port;

static dev_t devid_felica_uart;
static struct cdev cdev_felica_uart;
static const struct file_operations fops_felica_uart = {
	.owner = THIS_MODULE,
	.open = felica_uart_open,
	.release = felica_uart_close,
	.read = felica_uart_read,
	.write = felica_uart_write,
	.fsync = felica_uart_sync,
	.unlocked_ioctl = felica_uart_ioctl,
};
struct felica_sem_data {
	struct semaphore felica_sem;
};
static struct felica_sem_data *dev_sem;

/*
 * initialize device
 */
static void felica_uart_init(void)
{
	int ret;
	struct device *device_felica_uart;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	devid_felica_uart = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_uart, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_UART_NAME);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_uart, &fops_felica_uart);
	ret =
	    cdev_add(&cdev_felica_uart, devid_felica_uart, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_uart, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_uart =
	    device_create(felica_class, NULL, devid_felica_uart, NULL,
			  FELICA_UART_NAME);
	if (IS_ERR(device_felica_uart)) {
		cdev_del(&cdev_felica_uart);
		unregister_chrdev_region(devid_felica_uart, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(device_create)", __func__);
		return;
	}

	dev_sem = kmalloc(sizeof(struct felica_sem_data), GFP_KERNEL);
	if (!dev_sem) {
		cdev_del(&cdev_felica_uart);
		unregister_chrdev_region(devid_felica_uart, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(dev_sem malloc)", __func__);
		return;
	}
	sema_init(&dev_sem->felica_sem, 1);

	FELICA_LOG_DEBUG("[MFDD] %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_uart), MINOR(devid_felica_uart));
}

/*
 * finalize device
 */
static void felica_uart_exit(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	kfree(dev_sem);
	device_destroy(felica_class, devid_felica_uart);
	cdev_del(&cdev_felica_uart);
	unregister_chrdev_region(devid_felica_uart, FELICA_MINOR_COUNT);

	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
}

/*
 * open device
 */
static int felica_uart_open(struct inode *inode, struct file *file)
{
	uid_t uid;
	int ret;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);


	FELICA_LOG_DEBUG("[MFDD] %s system_rev: 0x%x uart port: %d ",
		__func__, system_rev, felica_uart_port);

	uid = __task_cred(current)->uid;
	if ((uid != gmfc_uid) && (uid != gdiag_uid)
							&& (uid != gant_uid)) {
		FELICA_LOG_DEBUG
		    ("[MFDD] %s END, uid=[%d], gmfc_uid=[%d], gdiag_uid=[%d]",
		     __func__, uid, gmfc_uid, gdiag_uid);
		return -EACCES;
	}

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(down_interruptible)", \
				 __func__);
		return -ERESTARTSYS;
	}

	if (gfa_open_cnt == 0) {
		memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
		memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);
		gfa_send_str[0] = FELICA_NL_REQ_OPEN;
		gfa_send_str[1] = felica_uart_port;

		ret = felica_nl_send_msg(2);

		if (ret == 0) {
			felica_nl_wait_ret_msg();
			if (gfa_rcv_str[1] == FELICA_NL_EFAILED) {
				FELICA_LOG_ERR("[MFDD] %s Open Fail", __func__);
				up(&dev_sem->felica_sem);
				return -EFAULT;
			}
		} else {
			FELICA_LOG_ERR("[MFDD] %s felica_nl_send_msg Fail", \
					__func__);
			up(&dev_sem->felica_sem);
			return -EFAULT;
		}
	}
	gfa_open_cnt++;

	up(&dev_sem->felica_sem);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_uart_close(struct inode *inode, struct file *file)
{
	int ret;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(down_interruptible)", \
					__func__);
		return -ERESTARTSYS;
	}

	gfa_open_cnt--;
	if (gfa_open_cnt == 0) {
		memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
		memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);
		gfa_send_str[0] = FELICA_NL_REQ_CLOSE;
		ret = felica_nl_send_msg(1);
		if (ret == 0) {
			felica_nl_wait_ret_msg();
			if (gfa_rcv_str[1] == FELICA_NL_EFAILED) {
				FELICA_LOG_ERR("[MFDD] %s Close Fail",\
						 __func__);
				gfa_open_cnt++;
				up(&dev_sem->felica_sem);
				return -EFAULT;
			}
		} else {
			FELICA_LOG_ERR("[MFDD] %s felica_nl_send_msg Fail", \
					__func__);
			gfa_open_cnt++;
			up(&dev_sem->felica_sem);
			return -EFAULT;
		}
	}

	up(&dev_sem->felica_sem);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_uart_read(struct file *file, char __user *buf,
				size_t len, loff_t *ppos)
{
	int ret = 0;
	int nlret;
	size_t wk_len = 0;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(down_interruptible)", \
					 __func__);
		return -ERESTARTSYS;
	}

	memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
	memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);

	wk_len = len;
	if (FELICA_NL_MSG_DATA_SIZE < wk_len) {
		FELICA_LOG_DEBUG("[MFDD] %s  read max size over [%d]", __func__,
				 wk_len);
		wk_len = FELICA_NL_MSG_DATA_SIZE;
	}
	gfa_send_str[0] = FELICA_NL_REQ_READ;
	gfa_send_str[1] = (char)(wk_len >> 8);
	gfa_send_str[2] = (char)wk_len;
	nlret = felica_nl_send_msg(3);

	wk_len = 0;
	if (nlret == 0) {
		felica_nl_wait_ret_msg();
		if (gfa_rcv_str[1] == FELICA_NL_SUCCESS) {
			wk_len =
			    (((int)gfa_rcv_str[2] << 8) & 0xFF00) | \
				(int)gfa_rcv_str[3];
			ret = copy_to_user(buf, &gfa_rcv_str[4], wk_len);
			if (ret != 0) {
				FELICA_LOG_ERR
				    ("[MFDD]%s ERROR(copy_from_user), ret=[%d]",
					  __func__, ret);
				up(&dev_sem->felica_sem);
				return -EFAULT;
			}
			*ppos = *ppos + wk_len;
		} else {
			FELICA_LOG_DEBUG(" %s FAIL", __func__);
			up(&dev_sem->felica_sem);
			return -EFAULT;
		}
	} else {
		FELICA_LOG_DEBUG(" %s FAIL", __func__);
		up(&dev_sem->felica_sem);
		return -EFAULT;
	}

	up(&dev_sem->felica_sem);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return (ssize_t) wk_len;
}

/*
 * write operation
 */
static ssize_t felica_uart_write(struct file *file, const char __user *data,
				 size_t len, loff_t *ppos)
{
	int ret = 0;
	int nlret;
	size_t wk_len = 0;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(down_interruptible)", \
				 __func__);
		return -ERESTARTSYS;
	}

	memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
	memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);

	wk_len = len;
	if (FELICA_NL_MSG_DATA_SIZE < wk_len) {
		FELICA_LOG_DEBUG("[MFDD] %s read max size over [%d]", __func__,
				 wk_len);
		wk_len = FELICA_NL_MSG_DATA_SIZE;
	}
	gfa_send_str[0] = FELICA_NL_REQ_WRITE;
	gfa_send_str[1] = (char)(wk_len >> 8);
	gfa_send_str[2] = (char)wk_len;
	ret = copy_from_user(&gfa_send_str[3], data, wk_len);
	if (ret != 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_from_user), ret=[%d]",
			       __func__, ret);
		up(&dev_sem->felica_sem);
		return -EFAULT;
	}
	nlret = felica_nl_send_msg(3 + len);
	if (nlret == 0) {
		wk_len = 0;
		felica_nl_wait_ret_msg();
		wk_len = (((int)gfa_rcv_str[2] << 8) & 0xFF00) | \
				(int)gfa_rcv_str[3];
		if (gfa_rcv_str[1] == FELICA_NL_EFAILED) {
			FELICA_LOG_ERR("[MFDD] %s Write Fail", __func__);
			up(&dev_sem->felica_sem);
			return -EINVAL;
		}
	} else {
		FELICA_LOG_ERR("[MFDD] %s felica_nl_send_msg Fail", __func__);
		up(&dev_sem->felica_sem);
		return -EINVAL;
	}
	up(&dev_sem->felica_sem);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return wk_len;
}

/*
 * sync operation
 */
static int felica_uart_sync(struct file *file, int datasync)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * available operation
 */
static long felica_uart_ioctl(struct file *file, unsigned int cmd,
			      unsigned long arg)
{
	unsigned int ret_str = 0;
	int ret;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(down_interruptible)", \
				__func__);
		return -ERESTARTSYS;
	}

	memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
	memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);

	gfa_send_str[0] = FELICA_NL_REQ_AVAIABLE;
	ret = felica_nl_send_msg(1);

	if (ret == 0) {
		felica_nl_wait_ret_msg();
		if (gfa_rcv_str[1] == FELICA_NL_SUCCESS) {
			/* create response data */
			ret_str =
			    (((unsigned int)gfa_rcv_str[2] << 8) & 0xFF00) | \
				(unsigned int)gfa_rcv_str[3];
			FELICA_LOG_DEBUG("Available Success data size [%d]", \
				ret_str);
		} else {
			FELICA_LOG_ERR("[MFDD] %s Available Fail", __func__);
			up(&dev_sem->felica_sem);
			return -EINVAL;
		}
	} else {
		FELICA_LOG_ERR("[MFDD] %s felica_nl_send_msg Fail", __func__);
		up(&dev_sem->felica_sem);
		return -EINVAL;
	}

	up(&dev_sem->felica_sem);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return put_user(ret_str, (unsigned int __user *)arg);

}

/*
 * create netlink socket
 */
static void felica_nl_init(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	gfa_connect_flag = 0;
	gfa_pid = 0;
	gfa_wait_flag = 0;
	memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
	memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);

	gfanl_sk =
	    netlink_kernel_create(&init_net, FELICA_NL_NETLINK_USER, 0,
				  felica_nl_recv_msg, NULL, THIS_MODULE);
	if (!gfanl_sk)
		FELICA_LOG_ERR("Error creating socket. %s\n", __func__);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}

/*
 * release netlink socket
 */
static void felica_nl_exit(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	netlink_kernel_release(gfanl_sk);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}

/*
 * send message to FeliCa-Serial-Connector
 */
static int felica_nl_send_msg(int len)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb_out;
	int msg_size = 0;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	if (!gfanl_sk) {
		FELICA_LOG_ERR("[MFDD]Error Not creating socket. %s\n",
			       __func__);
		return 1;
	}
	if (gfa_pid == 0) {
		FELICA_LOG_ERR("[MFDD]Error Not Rcv Connect Msg %s\n",
			       __func__);
		return 1;
	}

	msg_size = len;
	skb_out = nlmsg_new(msg_size, 0);

	if (!skb_out) {
		FELICA_LOG_ERR("Failed to allocate new skb_out %s\n", __func__);
		return 1;
	}
	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
	NETLINK_CB(skb_out).dst_group = 0;
	memcpy(NLMSG_DATA(nlh), gfa_send_str, msg_size);

	/* "skb_out" will release by netlink.*/
	nlmsg_unicast(gfanl_sk, skb_out, gfa_pid);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * receive message from FeliCa-Serial-Connector
 */
static void felica_nl_recv_msg(struct sk_buff *skb)
{

	struct nlmsghdr *nlh;
	struct sk_buff *wskb;
	int port_threshold = 0;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	if (!skb) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(skb NULL)", __func__);
		return;
	}

	wskb = skb_get(skb);
	if (wskb && (wskb->len > NLMSG_SPACE(0))) {
		nlh = nlmsg_hdr(wskb);
		memcpy(gfa_rcv_str, NLMSG_DATA(nlh), sizeof(gfa_rcv_str));
		if ((gfa_rcv_str[0] == FELICA_NL_CONNECT_MSG)
		    && (gfa_connect_flag == 0)) {
			/* pid of sending process */
			gfa_pid = nlh->nlmsg_pid;

#if defined(CONFIG_MACH_T0)
			port_threshold = 0x09;
#elif defined(CONFIG_MACH_M3)
			port_threshold = 0x02;
#endif
		if (felica_get_tamper_fuse_cmd() != 1) {
			if (system_rev >= port_threshold) {
				s3c_gpio_cfgall_range(FELICA_UART1RX, 4,\
				S3C_GPIO_SFN(2), S3C_GPIO_PULL_DOWN);
				felica_uart_port = 1;
			} else {
				s3c_gpio_cfgall_range(FELICA_UART3RX, 2,\
				S3C_GPIO_SFN(2), S3C_GPIO_PULL_DOWN);
				felica_uart_port = 3;
		}
			felica_set_felica_info();
			felica_uart_init();
			felica_pon_init();
			felica_cen_init();
			felica_rfs_init();
			felica_rws_init();
			felica_ant_init();
				if (gdiag_name[0] != 0x00)
					felica_uid_init();
			}

			gfa_connect_flag = 1;
		} else if ((gfa_rcv_str[0] == FELICA_NL_RESPONCE)
			   && (gfa_pid == nlh->nlmsg_pid)) {
			/* wake up */
			gfa_wait_flag = 1;
		} else {
			FELICA_LOG_ERR("[MFDD] %s ERROR(RCV Undefine MSG)",
				       __func__);
			FELICA_LOG_ERR("RCV MSG [%d]", gfa_rcv_str[0]);
			FELICA_LOG_ERR("rcv pid [%d]", nlh->nlmsg_pid);
			FELICA_LOG_ERR("gfa_pid [%d]", gfa_pid);
		}
	}
	kfree_skb(skb);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}
static void felica_set_felica_info(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START ", __func__);
	memset(gdiag_name, 0x00, DIAG_NAME_MAXSIZE + 1);

	gread_msgs[0].flags = gfa_rcv_str[MSG_READ1_FLAGS_OFFSET];
	gread_msgs[0].len = gfa_rcv_str[MSG_READ1_LEN_OFFSET];
	gread_msgs[1].flags = gfa_rcv_str[MSG_READ2_FLAGS_OFFSET];
	gread_msgs[1].len = gfa_rcv_str[MSG_READ2_LEN_OFFSET];
	gwrite_msgs[0].flags = gfa_rcv_str[MSG_WRITE_FLAGS_OFFSET];
	gwrite_msgs[0].len = gfa_rcv_str[MSG_WRITE_LEN_OFFSET];
	gi2c_lockaddress = gfa_rcv_str[MSG_LOCK_ADDR_OFFSET];
	gi2c_address = gfa_rcv_str[MSG_I2C_ADDR_OFFSET];

	memcpy(gdiag_name, &gfa_rcv_str[MSG_DIAG_NAME_OFFSET],
							DIAG_NAME_MAXSIZE);

	gmfc_uid =
	(((int)gfa_rcv_str[MSG_MFC_UID_FRONT_OFFSET] << 8) & 0xFF00) |
	(int)gfa_rcv_str[MSG_MFC_UID_BACK_OFFSET];

	gmfl_uid =
	(((int)gfa_rcv_str[MSG_MFL_UID_FRONT_OFFSET] << 8) & 0xFF00) |
	(int)gfa_rcv_str[MSG_MFL_UID_BACK_OFFSET];

	gi2c_antaddress = gfa_rcv_str[MSG_ANT_ADDR_OFFSET];
	gant_uid =
	(((int)gfa_rcv_str[MSG_ANT_UID_FRONT_OFFSET] << 8) & 0xFF00) |
	(int)gfa_rcv_str[MSG_ANT_UID_BACK_OFFSET];

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}

/*
 * waiting to receive messages from FeliCa-Serial-Connector
 */
static void felica_nl_wait_ret_msg(void)
{
	unsigned int cnt = 0;
	FELICA_LOG_DEBUG("[MFDD] %s START ", __func__);

	while (1) {
		if (gfa_wait_flag == 1) {
			FELICA_LOG_DEBUG("[MFDD] %s sleep cnt [%d]", __func__,
					 cnt);
			break;
		}
		mdelay(1);
		cnt++;
	}
	gfa_wait_flag = 0;

	FELICA_LOG_DEBUG("[MFDD] %s END ", __func__);
}



static int felica_smc_read_oemflag(u32 ctrl_word, u32 *val)
{
	register u32 reg0 __asm__("r0");
	register u32 reg1 __asm__("r1");
	register u32 reg2 __asm__("r2");
	register u32 reg3 __asm__("r3");
	u32 idx = 0;

	for (idx = 0; reg2 != ctrl_word; idx++) {
		reg0 = -202;
		reg1 = 1;
		reg2 = idx;

		__asm__ volatile ("smc    0\n" : "+r" (reg0), "+r"(reg1),
				  "+r"(reg2), "+r"(reg3)
		    );
		if (reg1)
			return -1;
	}

	reg0 = -202;
	reg1 = 1;
	reg2 = idx;

	__asm__ volatile ("smc    0\n" : "+r" (reg0), "+r"(reg1),
		"+r"(reg2),  "+r"(reg3)
	    );
	if (reg1)
		return -1;

	*val = reg2;

	return 0;
}

static int felica_Cpu0(void)
{
	int ret = 0;
	struct cpumask mask = CPU_MASK_CPU0;

	FELICA_LOG_DEBUG("System has %d CPU's, we are on CPU #%d\n"
	    "\tBinding this process to CPU #0.\n"
	    "\tactive mask is %lx, setting it to mask=%lx\n",
	    nr_cpu_ids,
	    raw_smp_processor_id(), cpu_active_mask->bits[0], mask.bits[0]);

	ret = set_cpus_allowed_ptr(current, &mask);
	if (0 != ret)
		FELICA_LOG_DEBUG("set_cpus_allowed_ptr=%d.\n", ret);

	FELICA_LOG_DEBUG("And now we are on CPU #%d", raw_smp_processor_id());

	return ret;
}

static int felica_CpuAll(void)
{
	int ret = 0;
	struct cpumask mask = CPU_MASK_ALL;

	FELICA_LOG_DEBUG("System has %d CPU's, we are on CPU #%d\n"
	    "\tBinding this process to CPU #0.\n"
	    "\tactive mask is %lx, setting it to mask=%lx\n",
	    nr_cpu_ids,
	    raw_smp_processor_id(), cpu_active_mask->bits[0], mask.bits[0]);

	ret = set_cpus_allowed_ptr(current, &mask);
	if (0 != ret)
		FELICA_LOG_DEBUG("set_cpus_allowed_ptr=%d.\n", ret);

	FELICA_LOG_DEBUG("And now we are on CPU #%d", raw_smp_processor_id());

	return ret;
}


static uint8_t felica_get_tamper_fuse_cmd(void)
{
	u32 fuse_id;
	int ret;

	ret = felica_Cpu0();
	if (0 != ret) {
		FELICA_LOG_DEBUG("changing core failed!");
		return -1;
	}

	FELICA_LOG_DEBUG("get_fuse");

	if (felica_smc_read_oemflag(0x80010001, (u32 *)&fuse_id) < 0) {
		FELICA_LOG_ERR("[MFDD] %s read flag error", __func__);
		return -1;
	}
	FELICA_LOG_DEBUG("[MFDD] Kernel Status[%x]", fuse_id);
	felica_CpuAll();

	return (uint8_t)fuse_id;
}

/******************************************************************************
 * /dev/felica_pon
 *****************************************************************************/

/* character device definition */
static dev_t devid_felica_pon;
static struct cdev cdev_felica_pon;
static const struct file_operations fops_felica_pon = {
	.owner = THIS_MODULE,
	.open = felica_pon_open,
	.release = felica_pon_close,
	.read = felica_pon_read,
	.write = felica_pon_write,
};

/*
 * initialize device
 */
static void felica_pon_init(void)
{
	int ret;
	struct device *device_felica_pon;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	devid_felica_pon = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_pon, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_PON_NAME);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_pon, &fops_felica_pon);
	ret = cdev_add(&cdev_felica_pon, devid_felica_pon, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_pon, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_pon =
	    device_create(felica_class, NULL, devid_felica_pon, NULL,
			  FELICA_PON_NAME);
	if (IS_ERR(device_felica_pon)) {
		cdev_del(&cdev_felica_pon);
		unregister_chrdev_region(devid_felica_pon, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(device_create)", __func__);
		return;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_pon), MINOR(devid_felica_pon));
}

/*
 * finalize device
 */
static void felica_pon_exit(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	device_destroy(felica_class, devid_felica_pon);
	cdev_del(&cdev_felica_pon);
	unregister_chrdev_region(devid_felica_pon, FELICA_MINOR_COUNT);

	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
}

/*
 * open device
 */
static int felica_pon_open(struct inode *inode, struct file *file)
{
	uid_t uid;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	uid = __task_cred(current)->uid;
	if ((uid != gmfc_uid) && (uid != gdiag_uid)
		&& (uid != gant_uid)) {
		FELICA_LOG_DEBUG
		    ("[MFDD] %s END, uid=[%d], gmfc_uid=[%d], gdiag_uid=[%d]",
		     __func__, uid, gmfc_uid, gdiag_uid);
		return -EACCES;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_pon_close(struct inode *inode, struct file *file)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	gpio_set_value(GPIO_PINID_FELICA_PON, GPIO_VALUE_LOW);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_pon_read(struct file *file, char __user *buf, size_t len,
			       loff_t *ppos)
{
	int ret;
	char retparam;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	ret = gpio_get_value(GPIO_PINID_FELICA_PON);
	if (ret == GPIO_VALUE_HIGH) {
		retparam = FELICA_PON_WIRED;
		FELICA_LOG_DEBUG("Wired interface Status is [%d][start]",
				 retparam);
	} else if (ret == GPIO_VALUE_LOW) {
		retparam = FELICA_PON_WIRELESS;
		FELICA_LOG_DEBUG("Wired interface Status is [%d][standby]",
				 retparam);
	} else {
		FELICA_LOG_ERR("[MFDD] %s ERROR(gpio_get_value), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	ret = copy_to_user(buf, &retparam, FELICA_PON_DATA_LEN);
	if (ret != 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return FELICA_PON_DATA_LEN;
}

/*
 * write operation
 */
static ssize_t felica_pon_write(struct file *file, const char __user *data,
				size_t len, loff_t *ppos)
{
	char pon;
	int ret;
	int setparam;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	ret = copy_from_user(&pon, data, FELICA_PON_DATA_LEN);
	if (ret != 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_from_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}

	if (pon == FELICA_PON_WIRED) {
		setparam = GPIO_VALUE_HIGH;
		FELICA_LOG_DEBUG("Set wired interface to [%d][start]",
				 setparam);
	} else if (pon == FELICA_PON_WIRELESS) {
		setparam = GPIO_VALUE_LOW;
		FELICA_LOG_DEBUG("Set wired interface to [%d][standby]",
				 setparam);
	} else {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_from_user), pon=[%d]",
			       __func__, pon);
		return -EINVAL;
	}

	gpio_set_value(GPIO_PINID_FELICA_PON, setparam);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return FELICA_PON_DATA_LEN;
}

/******************************************************************************
 * felica_i2c_driver
 ******************************************************************************/
static struct i2c_client *felica_i2c_client;
static const struct i2c_device_id felica_i2c_idtable[] = {
	{FELICA_I2C_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, felica_i2c_idtable);

static struct i2c_driver felica_i2c_driver = {
	.probe = felica_i2c_probe,
	.remove = felica_i2c_remove,
	.driver = {
		   .name = FELICA_I2C_NAME,
		   .owner = THIS_MODULE,
		   },
	.id_table = felica_i2c_idtable,
};

/*
 * felica_i2c_init
 */
static void felica_i2c_init(void)
{
	int ret;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	ret = i2c_add_driver(&felica_i2c_driver);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(i2c_add_driver), ret=[%d]",
			       __func__, ret);
		return;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return;
}

/*
 * felica_i2c_exit
 */
static void felica_i2c_exit(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	i2c_del_driver(&felica_i2c_driver);
	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return;
}

/*
 * probe device
 */
static int felica_i2c_probe(struct i2c_client *client,
			    const struct i2c_device_id *devid)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	felica_i2c_client = client;
	if (!felica_i2c_client) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(felica_i2c_client==NULL)", \
							__func__);
		return -EINVAL;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * remove device
 */
static int felica_i2c_remove(struct i2c_client *client)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	/* no operation */
	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/******************************************************************************
 * /dev/felica_cen
 ******************************************************************************/

/* character device definition */
static dev_t devid_felica_cen;
static struct cdev cdev_felica_cen;
static const struct file_operations fops_felica_cen = {
	.owner = THIS_MODULE,
	.open = felica_cen_open,
	.release = felica_cen_close,
	.read = felica_cen_read,
	.write = felica_cen_write,
};

/*
 * felica_cen_init
 */
static void felica_cen_init(void)
{
	int ret;
	struct device *device_felica_cen;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	devid_felica_cen = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_cen, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_CEN_NAME);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_cen, &fops_felica_cen);
	ret = cdev_add(&cdev_felica_cen, devid_felica_cen, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_cen, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_cen =
	    device_create(felica_class, NULL, devid_felica_cen, NULL,
			  FELICA_CEN_NAME);
	if (IS_ERR(device_felica_cen)) {
		cdev_del(&cdev_felica_cen);
		unregister_chrdev_region(devid_felica_cen, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(device_create)", __func__);
		return;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_cen), MINOR(devid_felica_cen));
}

/*
 * felica_cen_exit
 */
static void felica_cen_exit(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	device_destroy(felica_class, devid_felica_cen);
	cdev_del(&cdev_felica_cen);
	unregister_chrdev_region(devid_felica_cen, FELICA_MINOR_COUNT);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}

/*
 * open device
 */
static int felica_cen_open(struct inode *inode, struct file *file)
{
	uid_t uid;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	uid = __task_cred(current)->uid;
	if (file->f_mode & FMODE_WRITE) {
		if ((uid != gdiag_uid) && (uid != gmfl_uid)) {
			FELICA_LOG_DEBUG(\
			"[MFDD] %s END, uid=[%d]\n", __func__, uid);
			FELICA_LOG_DEBUG(\
			"[MFDD] %s END, gmfc_uid=[%d]\n", __func__, gmfc_uid);
			FELICA_LOG_DEBUG(\
			"[MFDD] %s END, gdiag_uid=[%d]\n", __func__, gdiag_uid);
			FELICA_LOG_DEBUG(\
			"[MFDD] %s END, gmfl_uid=[%d]\n", __func__, gmfl_uid);
			return -EACCES;
		}
	}
	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_cen_close(struct inode *inode, struct file *file)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	/* no operation */
	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_cen_read(struct file *file, char __user *buf, \
			size_t len, loff_t *ppos)
{
	int ret;
	unsigned char address = gi2c_lockaddress;
	unsigned char read_buff = 0;

	gread_msgs[0].addr = gi2c_address;
	gread_msgs[0].buf = &address;
	gread_msgs[1].addr = gi2c_address;
	gread_msgs[1].buf = &read_buff;

	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	if (felica_i2c_client == NULL) {
		FELICA_LOG_DEBUG("felica_i2c_client is NULL");
		return -EIO;
	}

	ret = i2c_transfer(felica_i2c_client->adapter, &gread_msgs[0], 1);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(i2c_transfer[0]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}
	ret = i2c_transfer(felica_i2c_client->adapter, &gread_msgs[1], 1);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(i2c_transfer[1]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	FELICA_LOG_DEBUG("[MFDD] %s read_buff=[%d]", __func__, read_buff);
	read_buff &= FELICA_CONTROL_LOCK_MASK;
	FELICA_LOG_DEBUG("[MFDD] %s read_buff=[%d]", __func__, read_buff);

	ret = copy_to_user(buf, &read_buff, FELICA_CEN_DATA_LEN);
	if (ret != 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return FELICA_CEN_DATA_LEN;
}

/*
 * write operation
 */
static ssize_t felica_cen_write(struct file *file, const char __user *data,
				size_t len, loff_t *ppos)
{
	char cen;
	int ret;
	unsigned char write_buff[2];
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	if (felica_i2c_client == NULL) {
		FELICA_LOG_DEBUG("felica_i2c_client is NULL");
		return -EIO;
	}

	gwrite_msgs[0].buf = &write_buff[0];
	gwrite_msgs[0].addr = gi2c_address;
	write_buff[0] = gi2c_lockaddress;

	ret = copy_from_user(&cen, data, FELICA_CEN_DATA_LEN);
	if (ret != 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_from_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	if (cen == FELICA_CEN_UNLOCK) {
		write_buff[1] = FELICA_CEN_SET_UNLOCK;
		FELICA_LOG_DEBUG("Set FeliCa-Lock status to [%d][UnLock]",
				 write_buff[1]);
	} else if (cen == FELICA_CEN_LOCK) {
		write_buff[1] = FELICA_CEN_SET_LOCK;
		FELICA_LOG_DEBUG("Set FeliCa-Lock status to [%d][Lock]",
				 write_buff[1]);
	} else {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_from_user), cen=[%d]",
			       __func__, cen);
		return -EINVAL;
	}
	ret = i2c_transfer(felica_i2c_client->adapter, gwrite_msgs, 1);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(i2c_transfer), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return FELICA_CEN_DATA_LEN;
}

/******************************************************************************
 * /dev/felica_rfs
 ******************************************************************************/

/* character device definition */
static dev_t devid_felica_rfs;
static struct cdev cdev_felica_rfs;
static const struct file_operations fops_felica_rfs = {
	.owner = THIS_MODULE,
	.open = felica_rfs_open,
	.release = felica_rfs_close,
	.read = felica_rfs_read,
};

/*
 * initialize device
 */
static void felica_rfs_init(void)
{
	int ret;
	struct device *device_felica_rfs;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	devid_felica_rfs = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_rfs, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_RFS_NAME);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_rfs, &fops_felica_rfs);
	ret = cdev_add(&cdev_felica_rfs, devid_felica_rfs, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_rfs, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_rfs =
	    device_create(felica_class, NULL, devid_felica_rfs, NULL,
			  FELICA_RFS_NAME);
	if (IS_ERR(device_felica_rfs)) {
		cdev_del(&cdev_felica_rfs);
		unregister_chrdev_region(devid_felica_rfs, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(device_create)", __func__);
		return;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_rfs), MINOR(devid_felica_rfs));
}

/*
 * finalize device
 */
static void felica_rfs_exit(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	device_destroy(felica_class, devid_felica_rfs);
	cdev_del(&cdev_felica_rfs);
	unregister_chrdev_region(devid_felica_rfs, FELICA_MINOR_COUNT);

	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
}

/*
 * open device
 */
static int felica_rfs_open(struct inode *inode, struct file *file)
{
	uid_t uid;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	uid = __task_cred(current)->uid;

	if ((uid != gmfc_uid) && (uid != gdiag_uid)) {
		FELICA_LOG_DEBUG
		    ("[MFDD] %s END, uid=[%d], gmfc_uid=[%d], gdiag_uid=[%d]",
		     __func__, uid, gmfc_uid, gdiag_uid);
		return -EACCES;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_rfs_close(struct inode *inode, struct file *file)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	/* no operation */
	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_rfs_read(struct file *file, char __user *buf, \
				size_t len, loff_t *ppos)
{
	int ret;
	char retparam;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	ret = gpio_get_value(GPIO_PINID_FELICA_RFS);
	if (ret == GPIO_VALUE_HIGH) {
		retparam = FELICA_RFS_STANDBY;
		FELICA_LOG_DEBUG("Carrier-Wave Status is [%d][standby]",
				 retparam);
	} else if (ret == GPIO_VALUE_LOW) {
		retparam = FELICA_RFS_DETECTED;
		FELICA_LOG_DEBUG("Carrier-Wave Status is [%d][detected]",
				 retparam);
	} else {
		FELICA_LOG_ERR("[MFDD] %s ERROR(gpio_get_value), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	ret = copy_to_user(buf, &retparam, FELICA_RFS_DATA_LEN);
	if (ret != 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return FELICA_RFS_DATA_LEN;
}

/******************************************************************************
 * /dev/felica_rws
 ******************************************************************************/

/* character device definition */
static dev_t devid_felica_rws;
static struct cdev cdev_felica_rws;
static const struct file_operations fops_felica_rws = {
	.owner = THIS_MODULE,
	.open = felica_rws_open,
	.release = felica_rws_close,
	.read = felica_rws_read,
	.write = felica_rws_write,
};

/*
 * initialize device
 */
static void felica_rws_init(void)
{
	int ret;
	struct device *device_felica_rws;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	devid_felica_rws = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_rws, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_RWS_NAME);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_rws, &fops_felica_rws);
	ret = cdev_add(&cdev_felica_rws, devid_felica_rws, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_rws, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_rws =
	    device_create(felica_class, NULL, devid_felica_rws, NULL,
			  FELICA_RWS_NAME);
	if (IS_ERR(device_felica_rws)) {
		cdev_del(&cdev_felica_rws);
		unregister_chrdev_region(devid_felica_rws, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(device_create)", __func__);
		return;
	}

	gfelica_rw_status = FELICA_RW_STATUS_INIT;

	FELICA_LOG_DEBUG("[MFDD] %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_rws), MINOR(devid_felica_rws));
}

/*
 * finalize device
 */
static void felica_rws_exit(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	device_destroy(felica_class, devid_felica_rws);
	cdev_del(&cdev_felica_rws);
	unregister_chrdev_region(devid_felica_rws, FELICA_MINOR_COUNT);

	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
}

/*
 * open device
 */
static int felica_rws_open(struct inode *inode, struct file *file)
{
	uid_t uid;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	uid = __task_cred(current)->uid;
	if (file->f_mode & FMODE_WRITE) {
		if (uid != grwm_uid) {
			FELICA_LOG_DEBUG(\
			"[MFDD] %s END, uid=[%d],gmfc_uid=[%d],gdiag_uid=[%d]",
			     __func__, uid, gmfc_uid, gdiag_uid);
			return -EACCES;
		}
	} else {
		if ((uid != gmfc_uid) && (uid != grwm_uid)) {
			FELICA_LOG_DEBUG(\
			"[MFDD] %s END, uid=[%d],gmfc_uid=[%d],gdiag_uid=[%d]",
			     __func__, uid, gmfc_uid, gdiag_uid);
			return -EACCES;
		}
	}

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_rws_close(struct inode *inode, struct file *file)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	/* no operation */
	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_rws_read(struct file *file, char __user *buf, size_t len,
			       loff_t *ppos)
{
	int ret;
	char retparam;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	if (gfelica_rw_status == FELICA_RW_STATUS_ENABLE) {
		retparam = FELICA_RW_STATUS_ENABLE;
		FELICA_LOG_DEBUG("ReaderWriterFunction status is [%d][enabled]",
				 retparam);
	} else if (gfelica_rw_status == FELICA_RW_STATUS_DISABLE) {
		retparam = FELICA_RW_STATUS_DISABLE;
		FELICA_LOG_DEBUG
		    ("ReaderWriterFunction status is [%d][disabled]", retparam);
	} else {
		FELICA_LOG_ERR("[MFDD] %s ERROR(gfelica_rw_status), RWM=[%d]",
			       __func__, gfelica_rw_status);
		return -EIO;
	}

	ret = copy_to_user(buf, &retparam, FELICA_RWS_DATA_LEN);
	if (ret != 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return FELICA_RWS_DATA_LEN;
}

/*
 * write operation
 */
static ssize_t felica_rws_write(struct file *file, const char __user *data,
				size_t len, loff_t *ppos)
{
	char work;
	int ret;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	ret = copy_from_user(&work, data, FELICA_RWS_DATA_LEN);
	if (ret != 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_from_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}

	if (work == FELICA_RW_STATUS_ENABLE) {
		FELICA_LOG_DEBUG
		    ("Set ReaderWriterFunction status to [%d][enable]", work);
	} else if (work == FELICA_RW_STATUS_DISABLE) {
		FELICA_LOG_DEBUG
		    ("Set ReaderWriterFunction status to s[%d][disable]", work);
	} else {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_from_user), work=[%d]",
			       __func__, work);
		return -EINVAL;
	}

	gfelica_rw_status = work;

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return FELICA_RWS_DATA_LEN;
}

/******************************************************************************
 * /dev/felica_int_poll
 ******************************************************************************/

/* character device definition */
static dev_t devid_felica_int_poll;
static struct cdev cdev_felica_int_poll;
static const struct file_operations fops_felica_int_poll = {
	.owner = THIS_MODULE,
	.open = felica_int_poll_open,
	.release = felica_int_poll_close,
	.read = felica_int_poll_read,
	.poll = felica_int_poll_poll,
};

/*
 * top half of irq_handler
 */
static irqreturn_t felica_int_irq_handler(int irq, void *dev_id)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	disable_irq_nosync(gpio_to_irq(GPIO_PINID_FELICA_INT));
	schedule_delayed_work(&pgint_irq->work,
			      msecs_to_jiffies(FELICA_INT_DELAY_TIME));

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return IRQ_HANDLED;
}

/*
 * bottom half of irq_handler
 */
static void felica_int_irq_work(struct work_struct *work)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	enable_irq(gpio_to_irq(GPIO_PINID_FELICA_INT));
	pgint_irq->irq_done = 1;
	wake_up_interruptible(&pgint_irq->read_wait);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}

/*
 * initialize device
 */
static void felica_int_poll_init(void)
{
	int ret;
	struct device *device_felica_int_poll;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	devid_felica_int_poll = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_int_poll, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_INT_POLL_NAME);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_int_poll, &fops_felica_int_poll);
	ret =
	    cdev_add(&cdev_felica_int_poll, devid_felica_int_poll,
		     FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_int_poll,
					 FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_int_poll =
	    device_create(felica_class, NULL, devid_felica_int_poll, NULL,
			  FELICA_INT_POLL_NAME);
	if (IS_ERR(device_felica_int_poll)) {
		cdev_del(&cdev_felica_int_poll);
		unregister_chrdev_region(devid_felica_int_poll,
					 FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(device_create)", __func__);
		return;
	}

	memset(pgint_irq, 0x00, sizeof(struct felica_int_irqdata));
	INIT_DELAYED_WORK(&pgint_irq->work, felica_int_irq_work);
	init_waitqueue_head(&pgint_irq->read_wait);
	ret = request_irq(gpio_to_irq(GPIO_PINID_FELICA_INT),
			  felica_int_irq_handler,
			  IRQF_TRIGGER_FALLING,
			  FELICA_INT_POLL_NAME, (void *)pgint_irq);
	if (ret != 0) {
		device_destroy(felica_class, devid_felica_int_poll);
		cdev_del(&cdev_felica_int_poll);
		unregister_chrdev_region(devid_felica_int_poll,
					 FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(request_irq), ret=[%d]",
			       __func__, ret);
		return;
	}

	ret = enable_irq_wake(gpio_to_irq(GPIO_PINID_FELICA_INT));
	if (ret < 0) {
		free_irq(gpio_to_irq(GPIO_PINID_FELICA_INT), (void *)pgint_irq);
		device_destroy(felica_class, devid_felica_int_poll);
		cdev_del(&cdev_felica_int_poll);
		unregister_chrdev_region(devid_felica_int_poll,
					 FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(enable_irq_wake), ret=[%d]",
			       __func__, ret);
		return;
	}

	pgint_irq->irq_done = 0;

	FELICA_LOG_DEBUG("[MFDD] %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_int_poll),
			 MINOR(devid_felica_int_poll));
}

/*
 * finalize device
 */
static void felica_int_poll_exit(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	disable_irq(gpio_to_irq(GPIO_PINID_FELICA_INT));
	free_irq(gpio_to_irq(GPIO_PINID_FELICA_INT), (void *)pgint_irq);
	device_destroy(felica_class, devid_felica_int_poll);
	cdev_del(&cdev_felica_int_poll);
	unregister_chrdev_region(devid_felica_int_poll, FELICA_MINOR_COUNT);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}

/*
 * open device
 */
static int felica_int_poll_open(struct inode *inode, struct file *file)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	/* no operation */
	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_int_poll_close(struct inode *inode, struct file *file)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	/* no operation */
	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_int_poll_read(struct file *file, char __user *buf,
				    size_t len, loff_t *ppos)
{
	int ret;
	char retparam;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	if (!pgint_irq->irq_done) {
		ret =
		    wait_event_interruptible(pgint_irq->read_wait,
					     pgint_irq->irq_done == 1);
		if (ret < 0) {
			FELICA_LOG_ERR
			("[MFDD] %s ERROR(wait_event_interruptible),ret=[%d]",\
			__func__, ret);
			return -EINTR;
		}
	}

	ret = gpio_get_value(GPIO_PINID_FELICA_INT);
	if (ret == GPIO_VALUE_HIGH) {
		retparam = FELICA_INT_HIGH;
		FELICA_LOG_DEBUG("INT-PIN value is [%d][HIGH]", retparam);
	} else if (ret == GPIO_VALUE_LOW) {
		retparam = FELICA_INT_LOW;
		FELICA_LOG_DEBUG("INT-PIN value is [%d][LOW]", retparam);
	} else {
		FELICA_LOG_ERR("[MFDD] %s ERROR(gpio_get_value), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	ret = copy_to_user(buf, &retparam, FELICA_INT_DATA_LEN);
	if (ret != 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	pgint_irq->irq_done = 0;

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return FELICA_INT_DATA_LEN;
}

/*
 * poll operation
 */
static unsigned int felica_int_poll_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	FELICA_LOG_DEBUG("%s START", __func__);

	poll_wait(file, &pgint_irq->read_wait, wait);
	if (pgint_irq->irq_done)
		mask = POLLIN | POLLRDNORM;

	FELICA_LOG_DEBUG("%s END", __func__);
	return mask;
}
/******************************************************************************
 * /dev/felica_uid
 ******************************************************************************/

/* character device definition */
static dev_t devid_felica_uid;
static struct cdev cdev_felica_uid;
static const struct file_operations fops_felica_uid = {
	.owner = THIS_MODULE,
	.open = felica_uid_open,
	.release = felica_uid_close,
	.unlocked_ioctl = felica_uid_ioctl,
};

/*
 * initialize device
 */
static void felica_uid_init(void)
{
	int ret;
	struct device *device_felica_uid;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	devid_felica_uid = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_uid, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_UID_NAME);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_uid, &fops_felica_uid);
	ret = cdev_add(&cdev_felica_uid, devid_felica_uid, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_uid, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_uid =
	    device_create(felica_class, NULL, devid_felica_uid, NULL,
			  FELICA_UID_NAME);
	if (IS_ERR(device_felica_uid)) {
		cdev_del(&cdev_felica_uid);
		unregister_chrdev_region(devid_felica_uid, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(device_create)", __func__);
		return;
	}
	FELICA_LOG_DEBUG("[MFDD] %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_uid), MINOR(devid_felica_uid));
}

/*
 * finalize device
 */
static void felica_uid_exit(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	if (gdiag_name[0] != 0x00) {
		device_destroy(felica_class, devid_felica_uid);
		cdev_del(&cdev_felica_uid);
		unregister_chrdev_region(devid_felica_uid, \
			FELICA_MINOR_COUNT);
	}
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
}

/*
 * open device
 */
static int felica_uid_open(struct inode *inode, struct file *file)
{
	char *cmdpos;
	static char cmdline[1025];
	static unsigned long start_adr, end_adr, leng;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	start_adr = current->mm->arg_start;
	end_adr = current->mm->arg_end;
	leng = end_adr - start_adr;

	if (1024 < leng)
		leng = 1024;

	cmdpos = (char *)(current->mm->arg_start);
	memcpy(cmdline, cmdpos, leng);
	cmdline[leng] = '\0';

	if (strncmp(cmdline, gdiag_name, leng) != 0) {
		FELICA_LOG_DEBUG("[MFDD] %s ERROR, %s gdiag %s", \
			__func__, cmdline, gdiag_name);
		return -EACCES;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_uid_close(struct inode *inode, struct file *file)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	/* no operation */
	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * uid registration
 */
static long felica_uid_ioctl(struct file *file, unsigned int cmd,
			     unsigned long arg)
{
	FELICA_LOG_DEBUG("[MFDD] %s START, cmd=[%d]", __func__, cmd);

	switch (cmd) {
	case SET_FELICA_UID_DIAG:
		gdiag_uid = *((int *)arg);
		FELICA_LOG_DEBUG("Set gdiag_uid to [%d]", gdiag_uid);
		break;
	default:
		FELICA_LOG_ERR("[MFDD] %s ERROR(unknown command)", __func__);
		break;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/******************************************************************************
 * /dev/felica_ant
 ******************************************************************************/

/* character device definition */
static dev_t devid_felica_ant;
static struct cdev cdev_felica_ant;
static const struct file_operations fops_felica_ant = {
	.owner = THIS_MODULE,
	.open = felica_ant_open,
	.release = felica_ant_close,
	.read = felica_ant_read,
	.write = felica_ant_write,
};

/*
 * felica_ant_init
 */
static void felica_ant_init(void)
{
	int ret;
	struct device *device_felica_ant;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	devid_felica_ant = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_ant, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_ANT_NAME);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_ant, &fops_felica_ant);
	ret = cdev_add(&cdev_felica_ant, devid_felica_ant, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_ant, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_ant =
	    device_create(felica_class, NULL, devid_felica_ant, NULL,
			  FELICA_ANT_NAME);
	if (IS_ERR(device_felica_ant)) {
		cdev_del(&cdev_felica_ant);
		unregister_chrdev_region(devid_felica_ant, FELICA_MINOR_COUNT);
		FELICA_LOG_ERR("[MFDD] %s ERROR(device_create)", __func__);
		return;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_ant), MINOR(devid_felica_ant));
}

/*
 * felica_ant_exit
 */
static void felica_ant_exit(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	device_destroy(felica_class, devid_felica_ant);
	cdev_del(&cdev_felica_ant);
	unregister_chrdev_region(devid_felica_ant, FELICA_MINOR_COUNT);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}

/*
 * open device
 */
static int felica_ant_open(struct inode *inode, struct file *file)
{
	uid_t uid;
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	uid = __task_cred(current)->uid;
	if ((uid != gant_uid) && (uid != gdiag_uid)) {
		FELICA_LOG_DEBUG(\
		"[MFDD] %s END, uid=[%d]\n", __func__, uid);
		FELICA_LOG_DEBUG(\
		"[MFDD] %s END, gant_uid=[%d]\n", __func__, gant_uid);
		return -EACCES;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_ant_close(struct inode *inode, struct file *file)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	/* no operation */
	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_ant_read(struct file *file, char __user *buf, \
			size_t len, loff_t *ppos)
{
	int ret;
	unsigned char address = gi2c_antaddress;
	unsigned char read_buff = 0;

	gread_msgs[0].addr = gi2c_address;
	gread_msgs[0].buf = &address;
	gread_msgs[1].addr = gi2c_address;
	gread_msgs[1].buf = &read_buff;

	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	if (felica_i2c_client == NULL) {
		FELICA_LOG_DEBUG("[MFDD] %s felica_i2c_client is NULL", \
			__func__);
		return -EIO;
	}

	ret = i2c_transfer(felica_i2c_client->adapter, &gread_msgs[0], 1);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(i2c_transfer[0]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}
	ret = i2c_transfer(felica_i2c_client->adapter, &gread_msgs[1], 1);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(i2c_transfer[1]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}
	FELICA_LOG_DEBUG("[MFDD] %s read_buff=[%d]", __func__, read_buff);

	ret = copy_to_user(buf, &read_buff, FELICA_ANT_DATA_LEN);
	if (ret != 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return FELICA_ANT_DATA_LEN;
}

/*
 * write operation
 */
static ssize_t felica_ant_write(struct file *file, const char __user *data,
				size_t len, loff_t *ppos)
{
	char ant;
	int ret;
	unsigned char write_buff[2];
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	if (felica_i2c_client == NULL) {
		FELICA_LOG_DEBUG("[MFDD] %s felica_i2c_client is NULL", \
			__func__);
		return -EIO;
	}

	gwrite_msgs[0].buf = &write_buff[0];
	gwrite_msgs[0].addr = gi2c_address;
	write_buff[0] = gi2c_antaddress;


	ret = copy_from_user(&ant, data, FELICA_ANT_DATA_LEN);
	if (ret != 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(copy_from_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	write_buff[1] = ant;

	ret = i2c_transfer(felica_i2c_client->adapter, gwrite_msgs, 1);
	if (ret < 0) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(i2c_transfer), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return FELICA_ANT_DATA_LEN;
}

/******************************************************************************
 * Mobile FeliCa device driver initialization / termination function
 ******************************************************************************/

/*
 * to set initial value to each terminal
 */
static void felica_initialize_pin(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	gpio_set_value(GPIO_PINID_FELICA_PON, GPIO_VALUE_LOW);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}

/*
 * to set final value to each terminal
 */
static void felica_finalize_pin(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	gpio_set_value(GPIO_PINID_FELICA_PON, GPIO_VALUE_LOW);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}

/*
 * device driver registration
 */
static void felica_register_device(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	felica_int_poll_init();

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}

/*
 * device driver deregistration
 */
static void felica_deregister_device(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);
	felica_uid_exit();
	felica_int_poll_exit();
	felica_ant_exit();
	felica_rws_exit();
	felica_rfs_exit();
	felica_cen_exit();
	felica_pon_exit();
	felica_uart_exit();

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}

/*
 * The entry point for initialization module
 */
static int __init felica_init(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	felica_class = class_create(THIS_MODULE, "felica");
	if (IS_ERR(felica_class)) {
		FELICA_LOG_ERR("[MFDD] %s ERROR(class_create)", __func__);
		return PTR_ERR(felica_class);
	}
	felica_initialize_pin();
	felica_register_device();
	felica_nl_init();
	felica_i2c_init();
	/* MFC UID registration */
	schedule_delayed_work(&pgint_irq->work, msecs_to_jiffies(10));

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * The entry point for the termination module
 */
static void __exit felica_exit(void)
{
	FELICA_LOG_DEBUG("[MFDD] %s START", __func__);

	felica_i2c_exit();
	felica_nl_exit();
	felica_deregister_device();
	felica_finalize_pin();
	class_destroy(felica_class);

	FELICA_LOG_DEBUG("[MFDD] %s END", __func__);
}

module_init(felica_init);
module_exit(felica_exit);

MODULE_DESCRIPTION("felica_dd");
MODULE_LICENSE("GPL v2");

#endif				/* CONFIG_FELICA */
