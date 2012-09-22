#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>

#include <linux/io.h>

#include <mach/gpio.h>

#include "fc8150.h"
#include "bbm.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fc8150_regs.h"
#include "fc8150_isr.h"
#include "fci_hal.h"

struct ISDBT_INIT_INFO_T *hInit;

#define RING_BUFFER_SIZE	(128 * 1024)  /* kmalloc max 128k */

/* GPIO(RESET & INTRRUPT) Setting */
#define FC8150_NAME		"isdbt"

/*
#define GPIO_ISDBT_IRQ 0x24
#define GPIO_ISDBT_PWR_EN 1
#define GPIO_ISDBT_RST 2
*/
#define GPIO_ISDBT_IRQ_FC8150 EXYNOS4_GPC0(4)
#define GPIO_ISDBT_PWR_EN_FC8150 EXYNOS4_GPC0(2)
#define GPIO_ISDBT_RST_FC8150 EXYNOS4_GPC0(0)

static DECLARE_WAIT_QUEUE_HEAD(isdbt_isr_wait);

static u8 isdbt_isr_sig;
static struct task_struct *isdbt_kthread;

static irqreturn_t isdbt_irq(int irq, void *dev_id)
{
	isdbt_isr_sig = 1;
	wake_up_interruptible(&isdbt_isr_wait);

	return IRQ_HANDLED;
}

int isdbt_hw_setting(void)
{
	int err;
	PRINTF(0, "isdbt_hw_setting\n");

	err = gpio_request(GPIO_ISDBT_PWR_EN_FC8150, "isdbt_en");
	if (err) {
		PRINTF(0, "isdbt_hw_setting: Couldn't request isdbt_en\n");
		goto gpio_isdbt_en;
	}
	gpio_direction_output(GPIO_ISDBT_PWR_EN_FC8150, 0);

	err =	gpio_request(GPIO_ISDBT_RST_FC8150, "isdbt_rst");
	if (err) {
		PRINTF(0, "isdbt_hw_setting: Couldn't request isdbt_rst\n");
		goto gpio_isdbt_rst;
	}
	gpio_direction_output(GPIO_ISDBT_RST_FC8150, 1);

	err =	gpio_request(GPIO_ISDBT_IRQ_FC8150, "isdbt_irq");
	if (err) {
		PRINTF(0, "isdbt_hw_setting: Couldn't request isdbt_irq\n");
		goto gpio_isdbt_rst;
	}
	gpio_direction_input(GPIO_ISDBT_IRQ_FC8150);

	err = request_irq(gpio_to_irq(GPIO_ISDBT_IRQ_FC8150), isdbt_irq
		, IRQF_DISABLED | IRQF_TRIGGER_RISING, FC8150_NAME, NULL);

	if (err < 0) {
		PRINTF(0, "isdbt_hw_setting: couldn't request gpio");
		PRINTF(0, "interrupt %d reason(%d)\n"
			, gpio_to_irq(GPIO_ISDBT_IRQ_FC8150), err);
		goto request_isdbt_irq;
	}
	return 0;
request_isdbt_irq:
	gpio_free(GPIO_ISDBT_IRQ_FC8150);
gpio_isdbt_rst:
	gpio_free(GPIO_ISDBT_PWR_EN_FC8150);
gpio_isdbt_en:
	return err;
}

/*POWER_ON & HW_RESET & INTERRUPT_CLEAR */
void isdbt_hw_init(void)
{
	PRINTF(0, "isdbt_hw_init\n");
	gpio_set_value(GPIO_ISDBT_PWR_EN_FC8150, 1);
	gpio_set_value(GPIO_ISDBT_RST_FC8150, 1);
	mdelay(5);
	gpio_set_value(GPIO_ISDBT_RST_FC8150, 0);
	mdelay(1);
	gpio_set_value(GPIO_ISDBT_RST_FC8150, 1);
}

/*POWER_OFF */
void isdbt_hw_deinit(void)
{
	PRINTF(0, "isdbt_hw_deinit\n");
	gpio_set_value(GPIO_ISDBT_PWR_EN_FC8150, 0);
}

int data_callback(u32 hDevice, u8 *data, int len)
{
	struct ISDBT_INIT_INFO_T *hInit;
	struct list_head *temp;
	hInit = (struct ISDBT_INIT_INFO_T *)hDevice;

	list_for_each(temp, &(hInit->hHead))
	{
		struct ISDBT_OPEN_INFO_T *hOpen;

		hOpen = list_entry(temp, struct ISDBT_OPEN_INFO_T, hList);

		if (hOpen->isdbttype == TS_TYPE) {
			if (fci_ringbuffer_free(&hOpen->RingBuffer) < (len+2)) {
				/*PRINTF(hDevice, "f"); */
				return 0;
			}

			FCI_RINGBUFFER_WRITE_BYTE(&hOpen->RingBuffer, len >> 8);
			FCI_RINGBUFFER_WRITE_BYTE(&hOpen->RingBuffer,
								len & 0xff);

			fci_ringbuffer_write(&hOpen->RingBuffer, data, len);

			wake_up_interruptible(&(hOpen->RingBuffer.queue));
		}
	}

	return 0;
}

static int isdbt_thread(void *hDevice)
{
	static DEFINE_MUTEX(thread_lock);

	struct ISDBT_INIT_INFO_T *hInit = (struct ISDBT_INIT_INFO_T *)hDevice;

	set_user_nice(current, -20);

	PRINTF(hInit, "isdbt_kthread enter\n");

	BBM_TS_CALLBACK_REGISTER((u32)hInit, data_callback);

	while (1) {
		wait_event_interruptible(isdbt_isr_wait,
			isdbt_isr_sig || kthread_should_stop());

		isdbt_isr_sig = 0;

		BBM_ISR(hInit);

		if (kthread_should_stop())
			break;
	}

	BBM_TS_CALLBACK_DEREGISTER();

	PRINTF(hInit, "isdbt_kthread exit\n");

	return 0;
}

const struct file_operations isdbt_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl		= isdbt_ioctl,
	.open		= isdbt_open,
	.read		= isdbt_read,
	.release	= isdbt_release,
};

static struct miscdevice fc8150_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = FC8150_NAME,
	.fops = &isdbt_fops,
};

int isdbt_open(struct inode *inode, struct file *filp)
{
	struct ISDBT_OPEN_INFO_T *hOpen;

	PRINTF(hInit, "isdbt open\n");

	hOpen = kmalloc(sizeof(struct ISDBT_OPEN_INFO_T), GFP_KERNEL);

	hOpen->buf = kmalloc(RING_BUFFER_SIZE, GFP_KERNEL);
	hOpen->isdbttype = 0;

	list_add(&(hOpen->hList), &(hInit->hHead));

	hOpen->hInit = (HANDLE *)hInit;

	if (hOpen->buf == NULL) {
		PRINTF(hInit, "ring buffer malloc error\n");
		return -ENOMEM;
	}

	fci_ringbuffer_init(&hOpen->RingBuffer, hOpen->buf, RING_BUFFER_SIZE);

	filp->private_data = hOpen;

	return 0;
}

ssize_t isdbt_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	s32 avail;
	s32 non_blocking = filp->f_flags & O_NONBLOCK;
	struct ISDBT_OPEN_INFO_T *hOpen
		= (struct ISDBT_OPEN_INFO_T *)filp->private_data;
	struct fci_ringbuffer *cibuf = &hOpen->RingBuffer;
	ssize_t len;

	if (!cibuf->data || !count)	{
		/*PRINTF(hInit, " return 0\n"); */
		return 0;
	}

	if (non_blocking && (fci_ringbuffer_empty(cibuf)))	{
		/*PRINTF(hInit, "return EWOULDBLOCK\n"); */
		return -EWOULDBLOCK;
	}

	if (wait_event_interruptible(cibuf->queue,
		!fci_ringbuffer_empty(cibuf))) {
		PRINTF(hInit, "return ERESTARTSYS\n");
		return -ERESTARTSYS;
	}

	avail = fci_ringbuffer_avail(cibuf);

	if (avail < 4) {
		PRINTF(hInit, "return 00\n");
		return 0;
	}

	len = FCI_RINGBUFFER_PEEK(cibuf, 0) << 8;
	len |= FCI_RINGBUFFER_PEEK(cibuf, 1);

	if (avail < len + 2 || count < len)	{
		PRINTF(hInit, "return EINVAL\n");
		return -EINVAL;
	}

	FCI_RINGBUFFER_SKIP(cibuf, 2);

	return fci_ringbuffer_read_user(cibuf, buf, len);
}

int isdbt_release(struct inode *inode, struct file *filp)
{
	struct ISDBT_OPEN_INFO_T *hOpen;

	PRINTF(hInit, "isdbt_release\n");

	hOpen = filp->private_data;

	hOpen->isdbttype = 0;

	list_del(&(hOpen->hList));
	kfree(hOpen->buf);
	kfree(hOpen);

	return 0;
}

int fc8150_if_test(void)
{
	int res = 0;
	int i;
	u16 wdata = 0;
	u32 ldata = 0;
	u8 data = 0;
	u8 temp = 0;

	PRINTF(0, "fc8150_if_test Start!!!\n");
	for (i = 0 ; i < 100 ; i++) {
		BBM_BYTE_WRITE(0, 0xa4, i & 0xff);
		BBM_BYTE_READ(0, 0xa4, &data);
		if ((i & 0xff) != data) {
			PRINTF(0, "fc8150_if_btest!   i=0x%x, data=0x%x\n"
				, i & 0xff, data);
			res = 1;
		}
	}

	for (i = 0 ; i < 100 ; i++) {
		BBM_WORD_WRITE(0, 0xa4, i & 0xffff);
		BBM_WORD_READ(0, 0xa4, &wdata);
		if ((i & 0xffff) != wdata) {
			PRINTF(0, "fc8150_if_wtest!   i=0x%x, data=0x%x\n"
				, i & 0xffff, wdata);
			res = 1;
		}
	}

	for (i = 0 ; i < 100 ; i++) {
		BBM_LONG_WRITE(0, 0xa4, i & 0xffffffff);
		BBM_LONG_READ(0, 0xa4, &ldata);
		if ((i & 0xffffffff) != ldata) {
			PRINTF(0, "fc8150_if_ltest!   i=0x%x, data=0x%x\n"
				, i & 0xffffffff, ldata);
			res = 1;
		}
	}

	for (i = 0 ; i < 100 ; i++) {
		temp = i & 0xff;
		BBM_TUNER_WRITE(NULL, 0x52, 0x01, &temp, 0x01);
		BBM_TUNER_READ(NULL, 0x52, 0x01, &data, 0x01);
		if ((i & 0xff) != data)
			PRINTF(0, "FC8150 tuner test (0x%x,0x%x)\n"
			, i & 0xff, data);
	}

	PRINTF(0, "fc8150_if_test End!!!\n");

	return res;
}


long isdbt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	s32 res = BBM_NOK;
	s32 err = 0;
	s32 size = 0;
	struct ISDBT_OPEN_INFO_T *hOpen;

	struct ioctl_info info;

	if (_IOC_TYPE(cmd) != IOCTL_MAGIC)
		return -EINVAL;
	if (_IOC_NR(cmd) >= IOCTL_MAXNR)
		return -EINVAL;

	hOpen = filp->private_data;

	size = _IOC_SIZE(cmd);

	switch (cmd) {
	case IOCTL_ISDBT_RESET:
		res = BBM_RESET(hInit);
		break;
	case IOCTL_ISDBT_INIT:
		res = BBM_I2C_INIT(hInit, FCI_I2C_TYPE);
		res |= BBM_PROBE(hInit);
		if (res) {
			PRINTF(hInit, "FC8150 Initialize Fail\n");
			break;
		}
		res |= BBM_INIT(hInit);
		break;
	case IOCTL_ISDBT_BYTE_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_BYTE_READ(hInit, (u16)info.buff[0]
			, (u8 *)(&info.buff[1]));
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_WORD_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_WORD_READ(hInit, (u16)info.buff[0]
			, (u16 *)(&info.buff[1]));
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_LONG_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_LONG_READ(hInit, (u16)info.buff[0]
			, (u32 *)(&info.buff[1]));
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_BULK_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_BULK_READ(hInit, (u16)info.buff[0]
			, (u8 *)(&info.buff[2]), info.buff[1]);
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_BYTE_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_BYTE_WRITE(hInit, (u16)info.buff[0]
			, (u8)info.buff[1]);
		break;
	case IOCTL_ISDBT_WORD_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_WORD_WRITE(hInit, (u16)info.buff[0]
			, (u16)info.buff[1]);
		break;
	case IOCTL_ISDBT_LONG_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_LONG_WRITE(hInit, (u16)info.buff[0]
			, (u32)info.buff[1]);
		break;
	case IOCTL_ISDBT_BULK_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_BULK_WRITE(hInit, (u16)info.buff[0]
			, (u8 *)(&info.buff[2]), info.buff[1]);
		break;
	case IOCTL_ISDBT_TUNER_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_TUNER_READ(hInit, (u8)info.buff[0]
			, (u8)info.buff[1],  (u8 *)(&info.buff[3])
			, (u8)info.buff[2]);
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_TUNER_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_TUNER_WRITE(hInit, (u8)info.buff[0]
			, (u8)info.buff[1], (u8 *)(&info.buff[3])
			, (u8)info.buff[2]);
		break;
	case IOCTL_ISDBT_TUNER_SET_FREQ:
		{
			u32 f_rf;
			err = copy_from_user((void *)&info, (void *)arg, size);

			f_rf = ((u32)info.buff[0] - 13) * 6000 + 473143;
			res = BBM_TUNER_SET_FREQ(hInit, f_rf);
		}
		break;
	case IOCTL_ISDBT_TUNER_SELECT:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_TUNER_SELECT(hInit, (u32)info.buff[0], 0);
		break;
	case IOCTL_ISDBT_TS_START:
		hOpen->isdbttype = TS_TYPE;
		break;
	case IOCTL_ISDBT_TS_STOP:
		hOpen->isdbttype = 0;
		break;
	case IOCTL_ISDBT_POWER_ON:
		isdbt_hw_init();
		break;
	case IOCTL_ISDBT_POWER_OFF:
		isdbt_hw_deinit();
		break;
	case IOCTL_ISDBT_SCAN_STATUS:
		res = BBM_SCAN_STATUS(hInit);
		break;
	default:
		PRINTF(hInit, "isdbt ioctl error!\n");
		res = BBM_NOK;
		break;
	}

	if (err < 0) {
		PRINTF(hInit, "copy to/from user fail : %d", err);
		res = BBM_NOK;
	}
	return res;
}

int isdbt_init(void)
{
	s32 res;

	PRINTF(hInit, "isdbt_init\n");

	res = misc_register(&fc8150_misc_device);

	if (res < 0) {
		PRINTF(hInit, "isdbt init fail : %d\n", res);
		return res;
	}

	isdbt_hw_setting();

	isdbt_hw_init();

	hInit = kmalloc(sizeof(struct ISDBT_INIT_INFO_T), GFP_KERNEL);

	res = BBM_HOSTIF_SELECT(hInit, BBM_SPI);

	if (res)
		PRINTF(hInit, "isdbt host interface select fail!\n");

	isdbt_hw_deinit();

	if (!isdbt_kthread)	{
		PRINTF(hInit, "kthread run\n");
		isdbt_kthread = kthread_run(isdbt_thread
			, (void *)hInit, "isdbt_thread");
	}

	INIT_LIST_HEAD(&(hInit->hHead));

	return 0;
}

void isdbt_exit(void)
{
	PRINTF(hInit, "isdbt isdbt_exit\n");

	free_irq(gpio_to_irq(GPIO_ISDBT_IRQ_FC8150), NULL);
	gpio_free(GPIO_ISDBT_IRQ_FC8150);
	gpio_free(GPIO_ISDBT_RST_FC8150);
	gpio_free(GPIO_ISDBT_PWR_EN_FC8150);

	kthread_stop(isdbt_kthread);
	isdbt_kthread = NULL;

	BBM_HOSTIF_DESELECT(hInit);

	isdbt_hw_deinit();

	misc_deregister(&fc8150_misc_device);

	kfree(hInit);
}

module_init(isdbt_init);
module_exit(isdbt_exit);

MODULE_LICENSE("Dual BSD/GPL");
