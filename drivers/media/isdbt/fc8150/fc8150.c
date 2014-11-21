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
#include <media/isdbt_pdata.h>

#include <mach/gpio.h>
#include <linux/platform_device.h>
#if defined(CONFIG_ISDBT_ANT_DET)
#include <linux/wakelock.h>
#include <linux/input.h>
#endif

#include "fc8150.h"
#include "bbm.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fc8150_regs.h"
#include "fc8150_isr.h"
#include "fci_hal.h"

#if defined(CONFIG_ISDBT_ANT_DET)
static struct wake_lock isdbt_ant_wlock;
#endif

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
static struct isdbt_platform_data gpio_cfg;

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
	mdelay(10);
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
		res = BBM_PROBE(hInit);
		if (res) {
#if defined(CONFIG_MACH_T0_JPN_LTE_DCM)
			if (system_rev == 11) {
				isdbt_hw_init();
				PRINTF(hInit, "Chip ID is not correct\n");
				msWait(3000);
			} else {
				PRINTF(hInit, "FC8150 Initialize Fail\n");
				break;
			}
#else
			PRINTF(hInit, "FC8150 Initialize Fail\n");
			break;
#endif
		}

		res = BBM_INIT(hInit);
		res |= BBM_I2C_INIT(hInit, FCI_I2C_TYPE);
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

#if defined(CONFIG_ISDBT_ANT_DET)
enum {
	ISDBT_ANT_OPEN = 0,
	ISDBT_ANT_CLOSE,
	ISDBT_ANT_UNKNOWN,
};
enum {
	ISDBT_ANT_DET_LOW = 0,
	ISDBT_ANT_DET_HIGH,
};

static struct input_dev *isdbt_ant_input;
static int isdbt_check_ant;
static int ant_prev_status;

#define ISDBT_ANT_CHECK_DURATION 500000 /* us */
#define ISDBT_ANT_CHECK_COUNT 2
#define ISDBT_ANT_WLOCK_TIMEOUT \
	((ISDBT_ANT_CHECK_DURATION * ISDBT_ANT_CHECK_COUNT * 2) / 1000000)
static int isdbt_ant_det_check_value(void)
{
	int loop = 0;
	int cur_val = 0, prev_val = 0;
	int ret = ISDBT_ANT_UNKNOWN;

	isdbt_check_ant = 1;

	prev_val = \
		ant_prev_status ? ISDBT_ANT_DET_LOW : ISDBT_ANT_DET_HIGH;
	for (loop = 0; loop < ISDBT_ANT_CHECK_COUNT; loop++) {
		usleep_range(ISDBT_ANT_CHECK_DURATION,\
				 ISDBT_ANT_CHECK_DURATION);
		cur_val = gpio_get_value_cansleep(gpio_cfg.gpio_ant_det);
		if (prev_val != cur_val || ant_prev_status == cur_val)
			break;
		prev_val = cur_val;
	}

	if (loop == ISDBT_ANT_CHECK_COUNT) {
		if (ant_prev_status == ISDBT_ANT_DET_LOW
			&& cur_val == ISDBT_ANT_DET_HIGH) {
			ret = ISDBT_ANT_OPEN;
		} else if (ant_prev_status == ISDBT_ANT_DET_HIGH
				&& cur_val == ISDBT_ANT_DET_LOW) {
			ret = ISDBT_ANT_CLOSE;
		}
		ant_prev_status = cur_val;
	}
	isdbt_check_ant = 0;
#if 0
	printk(KERN_DEBUG "%s cnt(%d) cur(%d) prev(%d)\n",
	__func__, loop, cur_val, ant_prev_status);
	printk(KERN_DEBUG "system_rev = %d\n", system_rev);
#endif
	return ret;
}

static int isdbt_ant_det_ignore_irq(void)
{
	/*printk("chk_ant=%d sr=%d\n",
		isdbt_check_ant, system_rev); */
	return isdbt_check_ant;
}

static void isdbt_ant_det_work_func(struct work_struct *work)
{
	if (!isdbt_ant_input) {
		printk(KERN_DEBUG \
			"%s: input device is not registered\n", __func__);
		return;
	}

	switch (isdbt_ant_det_check_value()) {
	case ISDBT_ANT_OPEN:
		input_report_key(isdbt_ant_input, KEY_DMB_ANT_DET_UP, 1);
		input_report_key(isdbt_ant_input, KEY_DMB_ANT_DET_UP, 0);
		input_sync(isdbt_ant_input);
		printk(KERN_DEBUG "%s : ISDBT_ANT_OPEN\n", __func__);
	break;
	case ISDBT_ANT_CLOSE:
		input_report_key(isdbt_ant_input, KEY_DMB_ANT_DET_DOWN, 1);
		input_report_key(isdbt_ant_input, KEY_DMB_ANT_DET_DOWN, 0);
		input_sync(isdbt_ant_input);
		printk(KERN_DEBUG "%s : ISDBT_ANT_CLOSE\n", __func__);
	break;
	case ISDBT_ANT_UNKNOWN:
		printk(KERN_DEBUG "%s : ISDBT_ANT_UNKNOWN\n", __func__);
	break;
	default:
		break;
	}

}

static struct workqueue_struct *isdbt_ant_det_wq;
static DECLARE_WORK(isdbt_ant_det_work, isdbt_ant_det_work_func);
static bool isdbt_ant_det_reg_input(struct platform_device *pdev)
{
	struct input_dev *input;
	int err;

	printk(KERN_DEBUG "%s\n", __func__);

	input = input_allocate_device();
	if (!input) {
		printk(KERN_DEBUG "Can't allocate input device\n");
		err = -ENOMEM;
	}
	set_bit(EV_KEY, input->evbit);
	set_bit(KEY_DMB_ANT_DET_UP & KEY_MAX, input->keybit);
	set_bit(KEY_DMB_ANT_DET_DOWN & KEY_MAX, input->keybit);
	input->name = "sec_dmb_key";
	input->phys = "sec_dmb_key/input0";
	input->dev.parent = &pdev->dev;

	err = input_register_device(input);
	if (err) {
		printk(KERN_DEBUG "Can't register dmb_ant_det key: %d\n", err);
		goto free_input_dev;
	}
	isdbt_ant_input = input;
	ant_prev_status = gpio_get_value_cansleep(gpio_cfg.gpio_ant_det);

	return true;

free_input_dev:
	input_free_device(input);
	return false;
}

static void isdbt_ant_det_unreg_input(void)
{
	input_unregister_device(isdbt_ant_input);
}
static bool isdbt_ant_det_create_wq(void)
{
	printk(KERN_DEBUG "%s\n", __func__);
	isdbt_ant_det_wq = create_singlethread_workqueue("isdbt_ant_det_wq");
	if (isdbt_ant_det_wq)
		return true;
	else
		return false;
}

static bool isdbt_ant_det_destroy_wq(void)
{
	if (isdbt_ant_det_wq) {
		flush_workqueue(isdbt_ant_det_wq);
		destroy_workqueue(isdbt_ant_det_wq);
		isdbt_ant_det_wq = NULL;
	}
	return true;
}

static irqreturn_t isdbt_ant_det_irq_handler(int irq, void *dev_id)
{
	int ret = 0;

	if (isdbt_ant_det_ignore_irq())
		return IRQ_HANDLED;

	wake_lock_timeout(&isdbt_ant_wlock, ISDBT_ANT_WLOCK_TIMEOUT * HZ);

	if (isdbt_ant_det_wq) {
		ret = queue_work(isdbt_ant_det_wq, &isdbt_ant_det_work);
		if (ret == 0)
			printk(KERN_DEBUG "%s queue_work fail\n", __func__);
	}

	return IRQ_HANDLED;

}

static bool isdbt_ant_det_irq_set(bool set)
{
	bool ret = true;
	int irq_ret;

	if (set) {
		irq_set_irq_type(gpio_cfg.irq_ant_det, IRQ_TYPE_EDGE_BOTH);
		irq_ret = request_irq(gpio_cfg.irq_ant_det
						, isdbt_ant_det_irq_handler
						, IRQF_DISABLED
						, "isdbt_ant_det"
						, NULL);
		if (irq_ret < 0) {
			printk(KERN_DEBUG "%s %d\r\n", __func__, irq_ret);
			ret = false;
		}
	    enable_irq_wake(gpio_cfg.irq_ant_det);
	} else {
	    disable_irq_wake(gpio_cfg.irq_ant_det);
		free_irq(gpio_cfg.irq_ant_det, NULL);
	}

	return ret;
}
#endif

static int isdbt_probe(struct platform_device *pdev)
{
	int res;
	struct isdbt_platform_data *p = pdev->dev.platform_data;
	memcpy(&gpio_cfg, p, sizeof(struct isdbt_platform_data));
	printk(KERN_DEBUG "%s\n", __func__);

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
#if defined(CONFIG_ISDBT_ANT_DET)
	wake_lock_init(&isdbt_ant_wlock, WAKE_LOCK_SUSPEND, "isdbt_ant_wlock");

	if (!isdbt_ant_det_reg_input(pdev))
		goto err_reg_input;
	if (!isdbt_ant_det_create_wq())
		goto free_reg_input;
	if (!isdbt_ant_det_irq_set(true))
		goto free_ant_det_wq;

	return 0;
free_ant_det_wq:
	isdbt_ant_det_destroy_wq();
free_reg_input:
	isdbt_ant_det_unreg_input();
err_reg_input:
	return -EFAULT;
#else
	return 0;
#endif

}

static int isdbt_remove(struct platform_device *pdev)
{
	printk(KERN_DEBUG "ISDBT remove\n");
#if defined(CONFIG_ISDBT_ANT_DET)
	isdbt_ant_det_unreg_input();
	isdbt_ant_det_destroy_wq();
	isdbt_ant_det_irq_set(false);
	wake_lock_destroy(&isdbt_ant_wlock);
#endif
	return 0;
}

static int isdbt_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int isdbt_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver isdbt_driver = {
			.probe	= isdbt_probe,
	.remove = isdbt_remove,
	.suspend = isdbt_suspend,
	.resume = isdbt_resume,
			.driver = {
					.owner	= THIS_MODULE,
					.name = "isdbt"
				},
};

int isdbt_init(void)
{
	s32 res;

	PRINTF(hInit, "isdbt_init\n");
	res = platform_driver_register(&isdbt_driver);
	if (res < 0) {
		PRINTF(hInit, "isdbt init fail : %d\n", res);
		return res;
	}


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
	platform_driver_unregister(&isdbt_driver);
	misc_deregister(&fc8150_misc_device);

	kfree(hInit);
}

module_init(isdbt_init);
module_exit(isdbt_exit);

MODULE_LICENSE("Dual BSD/GPL");
