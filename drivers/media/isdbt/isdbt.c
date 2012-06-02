/************************************************************
 * /drivers/media/isdbt/isdbt.c
 *
 *        ISDBT DRIVER
 *
 *
 *       Copyright (c) 2011 Samsung Electronics
 *
 *                       http://www.samsung.com
 *
 ***********************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <plat/gpio-cfg.h>
#include <plat/s3c64xx-spi.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include "isdbt.h"

#define	SPI_FIND_HEADER_IN_BUFFER
/* #define	SPI_WRITE_BUF */

#define	DATA_PACKET_SIZE			188
#define	DATA_PACKET_CNT				128
#define	SPI_BURST_READ_SIZE		(DATA_PACKET_SIZE * DATA_PACKET_CNT)
#define	SPI_DATA_FRAME_BUF		(SPI_BURST_READ_SIZE * 10)

#define	SPI_READ_TIMEOUT		(SPI_BURST_READ_SIZE/40 + 10)

#define	READ_PACKET_MARGIN		(DATA_PACKET_SIZE/2)

#define	HEADER_OF_VALID_PACKET		0x47
#define	INVALID_PACKET_CHECK_CNT	3

#define	CS_INT_MODE()	S3C_GPIO_SFN(0xf)
#define	CS_SPI_MODE()	S3C_GPIO_SFN(0x2)

#define	SPIDEV_WARMUP_DELAY			300

#define	DEVICE_NAME	"isdbtdata"

typedef struct{
	int					index;
	int					init_info;
	unsigned char		*rBuf;
	unsigned char		*wBuf;
	struct mutex		buflock;
	struct completion	xfer_completion;
	struct completion	overflow_completion;
	long			xfer_size;
	int			xfer_result;
	struct class		*isdbt_class;
	struct device		*isdbt_debug_dev;
	struct spi_device	*spidev;
	unsigned char		*RingBuf;
	long				ring_buf_wr_pos;
	long				ring_buf_rd_pos;
	int					buf_over_flow;
	struct	workqueue_struct *read_workqueue;
	struct	work_struct	work_read_spi;
	unsigned long		pkt_err_cnt;
} isdbt_spi_info;

static isdbt_spi_info *isdbt_data;

static ssize_t isdbt_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	isdbt_spi_info *pdev = dev_get_drvdata(dev);
	int	result = 0;

	if (pdev->init_info == 0)
			result = 0;
	else
			result = 1;

	return sprintf(buf, " %s \n wr_pos : [ %lx ]\n rd_pos : [ %lx ] \n pkt_err_cnt [ %lx ]\n",
		(result & 1) ? "active" : "deactive", pdev->ring_buf_wr_pos,
		pdev->ring_buf_rd_pos, pdev->pkt_err_cnt);
}
static DEVICE_ATTR(isdbt_status, 0666, isdbt_status_show, NULL);


static int fc8100_spi_read(struct spi_device *spidev, u8 *buf, size_t len)
{
	struct spi_message msg;
	struct spi_transfer transfer;
	int status = 0;
	int result = 0;

	memset(&msg, 0, sizeof(msg));
	memset(&transfer, 0, sizeof(transfer));

	spi_message_init(&msg);
	msg.spi = spidev;

	transfer.tx_buf = (unsigned char *)NULL;
	transfer.rx_buf = (unsigned char *)buf;
	transfer.len	= len;
	transfer.bits_per_word	= 8;
	transfer.delay_usecs	= 0;

	spi_message_add_tail(&transfer, &msg);
	status = spi_sync(spidev, &msg);

	if (status == 0)
		result = msg.actual_length;
	else
		result = status;

	return result;
}



static int fc8100_spi_write(struct spi_device *spidev, u8 *buf, size_t len)
{
	struct spi_message msg;
	struct spi_transfer transfer;
	int status = 0;
	int result = 0;

	memset(&msg, 0, sizeof(msg));
	memset(&transfer, 0, sizeof(transfer));

	spi_message_init(&msg);
	msg.spi = spidev;

	transfer.tx_buf = (unsigned char *)buf;
	transfer.rx_buf = (unsigned char *)NULL;
	transfer.len	= len;
	transfer.bits_per_word	= 8;
	transfer.delay_usecs	= 0;

	spi_message_add_tail(&transfer, &msg);
	status = spi_sync(spidev, &msg);

	if (status == 0)
		result = msg.actual_length;
	else
		result = status;

	return result;
}

static int fc8100_spi_probe(struct spi_device *spi)
{
	int ret = -EINVAL;
	isdbt_spi_info *pdev;

	pdev = kmalloc(sizeof(isdbt_spi_info), GFP_KERNEL);
	if (pdev == NULL) {
		I_DEV_DBG(" memory allocation failed ");
		ret = -ENOMEM;
		goto done;
	}
	memset(pdev, 0, sizeof(isdbt_spi_info));

	pdev->isdbt_class = class_create(THIS_MODULE, "isdbt");
	if (IS_ERR(pdev->isdbt_class)) {
		I_DEV_DBG(" could not create isdbt_class");
		goto err_class_create;
	}

	pdev->isdbt_debug_dev = device_create(pdev->isdbt_class,
					NULL, 0, NULL, "isdbt_status");
	if (IS_ERR(pdev->isdbt_debug_dev)) {
		I_DEV_DBG(" could not create isdbt_status file");
		goto err_device_create;
	}

	if (device_create_file(pdev->isdbt_debug_dev,
		&dev_attr_isdbt_status) < 0) {
		I_DEV_DBG(" could not create device file(%s)",
			dev_attr_isdbt_status.attr.name);
		goto err_device_create_file;
	}

	pdev->spidev = spi;
	pdev->spidev->mode = (SPI_MODE_0|SPI_CS_HIGH);
	pdev->spidev->bits_per_word = 8;

	dev_set_drvdata(pdev->isdbt_debug_dev, pdev);

	ret = spi_setup(pdev->spidev);

	spi_set_drvdata(spi, pdev);
	isdbt_data = pdev;

	I_DEV_DBG("Probe %s ,[ %d ]", (ret & 1) ? "ERROR" : "SUCCESS" , ret);
	goto done;

err_device_create_file:
	device_destroy(pdev->isdbt_class, 0);
err_device_create:
	class_destroy(pdev->isdbt_class);
err_class_create:
	kfree(pdev);
done:
	return ret;
}

static int fc8100_spi_remove(struct spi_device *spi)
{
	isdbt_spi_info *pdev = spi_get_drvdata(spi);

	device_destroy(pdev->isdbt_class, 0);
	class_destroy(pdev->isdbt_class);
	kfree(pdev);
	isdbt_data = NULL;
	I_DEV_DBG("");
	return 0;
}

static struct spi_driver fc8100_spi_driver = {
	.driver = {
		.name	=	"isdbtspi",
		.bus	=	&spi_bus_type,
		.owner	=	THIS_MODULE,
	},
	.probe		=	fc8100_spi_probe,
	.remove		=	__devexit_p(fc8100_spi_remove),
};

static int fc8100_spi_driver_init(void)
{
	int ret = 0;

	ret = spi_register_driver(&fc8100_spi_driver);

	I_DEV_DBG("FC8100 SPI DRIVER %s , ret = [ %d ]",
					(ret & 1) ? "ERROR" : " INIT ", ret);

	return ret;
}

static void fc8100_spi_exit(void)
{
	I_DEV_DBG("");
	spi_unregister_driver(&fc8100_spi_driver);
	return;
}

static void IsValied_packet_check(isdbt_spi_info *pdev)
{
	int cnt = 0;

	/*	Find Header	*/
	while (1) {
		if ((pdev->rBuf[cnt] == HEADER_OF_VALID_PACKET)
		&& (pdev->rBuf[cnt + DATA_PACKET_SIZE] == HEADER_OF_VALID_PACKET)
		&& (pdev->rBuf[cnt + (DATA_PACKET_SIZE*2)] == HEADER_OF_VALID_PACKET)) {
			break;
		}

		if (cnt > READ_PACKET_MARGIN) {
			pdev->pkt_err_cnt++;
			I_DEV_DBG(" FIND ====== ERROR cnt = %d, pkt_err_cnt = %d ",
							cnt, pdev->pkt_err_cnt);
			pdev->xfer_result = -EINVAL;
			break;
		}
		cnt++;
	}

	if (pdev->xfer_result == pdev->xfer_size) {
		mutex_lock(&pdev->buflock);
		if (pdev->ring_buf_wr_pos + cnt > SPI_DATA_FRAME_BUF) {
				pdev->ring_buf_wr_pos = 0;
				pdev->buf_over_flow = 1;
				complete(&pdev->overflow_completion);
		}
		memcpy(&pdev->RingBuf[pdev->ring_buf_wr_pos],
				&pdev->rBuf[cnt], pdev->xfer_result);
		pdev->ring_buf_wr_pos += pdev->xfer_result;
		pdev->index = 1;
		mutex_unlock(&pdev->buflock);
		complete(&pdev->xfer_completion);
	}

	return;
}

void isdbt_work_queue_spi_read_buf(struct work_struct *work)
{
	isdbt_spi_info *pdev = container_of(work, isdbt_spi_info, work_read_spi);
	struct s3c64xx_spi_csinfo *cs = pdev->spidev->controller_data;

	disable_irq(gpio_to_irq(cs->line));
	s3c_gpio_cfgpin(cs->line, CS_SPI_MODE());
	udelay(SPIDEV_WARMUP_DELAY);
	INIT_COMPLETION(pdev->xfer_completion);
	pdev->xfer_size = SPI_BURST_READ_SIZE + READ_PACKET_MARGIN ;
	pdev->xfer_result = fc8100_spi_read(pdev->spidev,
					pdev->rBuf, pdev->xfer_size);
	pdev->xfer_size	-= READ_PACKET_MARGIN;
	pdev->xfer_result -= READ_PACKET_MARGIN;

	s3c_gpio_cfgpin(cs->line, CS_INT_MODE());
	enable_irq(gpio_to_irq(cs->line));

	if ((pdev->xfer_result != pdev->xfer_size)
		|| (pdev->xfer_result < DATA_PACKET_SIZE*INVALID_PACKET_CHECK_CNT)) {
		I_DEV_DBG(" Packet read failed = %d", pdev->xfer_result);
		pdev->xfer_result = -EINVAL;
	} else {
		IsValied_packet_check(pdev);
	}

	return;
}


static irqreturn_t isdbt_read_data_threaded_irq(int irq, void *data)
{
	isdbt_spi_info *pdev = (isdbt_spi_info *)(data);
	queue_work(pdev->read_workqueue, &pdev->work_read_spi);
	return IRQ_HANDLED;
}

static int isdbt_open(struct inode *inode, struct file *filp)
{
	int status = 0;
	isdbt_spi_info	*pdev;
	struct s3c64xx_spi_csinfo *cs;
	I_DEV_DBG("");

	pdev = isdbt_data;

	if (pdev->init_info == 1) {
		I_DEV_DBG("Already open....");
		return status;
	}

	pdev->index	= 0;
	pdev->ring_buf_wr_pos = 0;
	pdev->ring_buf_rd_pos = 0;
	pdev->buf_over_flow = 0;

	cs = pdev->spidev->controller_data;

	pdev->rBuf = kmalloc(SPI_BURST_READ_SIZE+(DATA_PACKET_SIZE*5), GFP_KERNEL);
	if (pdev->rBuf == NULL) {
		I_DEV_DBG(" rBuf :: kamlloc failed");
		status = -ENOMEM;
		goto err_malloc_rBuf;
	}
	pdev->RingBuf = vmalloc(SPI_DATA_FRAME_BUF);
	if (pdev->RingBuf == NULL) {
		I_DEV_DBG(" ring buf :: malloc failed");
		status = -ENOMEM;
		goto err_malloc_ringbuf;
	}
#if	defined(SPI_WRITE_BUF)
	pdev->wBuf = kmalloc(SPI_BURST_READ_SIZE+(DATA_PACKET_SIZE*5), GFP_KERNEL);
	if (pdev->wBuf == NULL) {
		I_DEV_DBG(" wBuf :: kmalloc failed");
		status = -ENOMEM;
		goto err_malloc_wBuf;
	}
#endif
	filp->private_data = pdev;
	mutex_init(&pdev->buflock);

	pdev->read_workqueue = create_singlethread_workqueue("isdbtd");
	if (pdev->read_workqueue == 0) {
		I_DEV_DBG(" ERROR : register workqueue");
		goto err_register_queue;
	}
	INIT_WORK(&pdev->work_read_spi, isdbt_work_queue_spi_read_buf);

	init_completion(&pdev->xfer_completion);
	init_completion(&pdev->overflow_completion);

	status = request_threaded_irq(gpio_to_irq(cs->line), NULL,
		isdbt_read_data_threaded_irq, IRQF_DISABLED, "isdbt_irq", pdev);
	if (status != 0) {
		I_DEV_DBG(" ERROR : request_threaded_irq failed = [ %d ]", status);
		I_DEV_DBG("filp private_data=0x%x. ", (unsigned int)filp->private_data);
		status = -EINVAL;
		goto err_register_isr;
	}
	disable_irq(gpio_to_irq(cs->line));
	s3c_gpio_cfgpin(cs->line, CS_INT_MODE());
	set_irq_type(gpio_to_irq(cs->line), IRQ_TYPE_EDGE_RISING);

	I_DEV_DBG("");
	s3c_gpio_cfgpin(cs->line, CS_INT_MODE());
	enable_irq(gpio_to_irq(cs->line));

	status = 0;
	goto done;
err_register_isr:
	free_irq(gpio_to_irq(cs->line), pdev);
	flush_workqueue(pdev->read_workqueue);
#if	defined(SPI_WRITE_BUF)
	kfree(pdev->wBuf);
#endif
err_register_queue:
	destroy_workqueue(pdev->read_workqueue);
#if defined(SPI_WRITE_BUF)
err_malloc_wBuf:
	pdev->wBuf = NULL;
#endif
	vfree(pdev->RingBuf);
err_malloc_ringbuf:
	pdev->RingBuf = NULL;
	kfree(pdev->rBuf);
err_malloc_rBuf:
	pdev->rBuf = NULL;
done:
	if (status == 0)
			pdev->init_info = 1;
	return status;
}

static int isdbt_release(struct inode *inode, struct file *filp)
{
	isdbt_spi_info *pdev = (isdbt_spi_info *)(filp->private_data);
	struct s3c64xx_spi_csinfo *cs = pdev->spidev->controller_data;

	if (pdev->init_info == 0) {
		I_DEV_DBG(" Already release... ");
		return 0;
	}

	cancel_work_sync(&pdev->work_read_spi);
	free_irq(gpio_to_irq(cs->line), pdev);

	flush_work(&pdev->work_read_spi);
	flush_workqueue(pdev->read_workqueue);
	destroy_workqueue(pdev->read_workqueue);

	vfree(pdev->RingBuf);
	pdev->RingBuf = NULL;

#if	defined(SPI_WRITE_BUF)
	kfree(pdev->wBuf);
#endif
	pdev->wBuf = NULL;

	kfree(pdev->rBuf);
	pdev->rBuf = NULL;

	pdev->init_info = 0;
	return 0;
}

static int isdbt_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	int result = 0;
	int status = 0;
	int unused_packet = 0;

	unsigned long timeout = 0;
	isdbt_spi_info *pdev = (isdbt_spi_info *)(filp->private_data);

	if (pdev->index == 0) {
		msleep(100);
		return -EIO;
	}

	if (pdev->ring_buf_rd_pos+count <= SPI_DATA_FRAME_BUF) {
		if (pdev->buf_over_flow == 0) {
			if (pdev->ring_buf_rd_pos+count > pdev->ring_buf_wr_pos) {
				timeout = wait_for_completion_timeout(&pdev->xfer_completion,
						msecs_to_jiffies(SPI_READ_TIMEOUT));
				I_DEV_DBG(" xfer timeout = %ld ", timeout);
				if (timeout == 0)
					result = -ETIMEDOUT;
			}
		}
	} else {
		if (!pdev->buf_over_flow) {
			wait_for_completion_timeout(&pdev->overflow_completion,
							msecs_to_jiffies(SPI_READ_TIMEOUT));
			INIT_COMPLETION(pdev->overflow_completion);
			I_DEV_DBG(" overflow timeout = %ld ", timeout);
			if (timeout == 0)
				result = -ETIMEDOUT;
		}

		pdev->buf_over_flow = 0;
		pdev->ring_buf_rd_pos = 0;
		if (pdev->ring_buf_rd_pos+count > pdev->ring_buf_wr_pos) {
			wait_for_completion_timeout(&pdev->xfer_completion,
						msecs_to_jiffies(SPI_READ_TIMEOUT));
			I_DEV_DBG(" over xfer timeout = %ld ", timeout);
			if (timeout == 0)
				result = -ETIMEDOUT;
		}
	}

	unused_packet = pdev->ring_buf_wr_pos - pdev->ring_buf_rd_pos+count;
	if (pdev->buf_over_flow)
		unused_packet += SPI_DATA_FRAME_BUF;

	if (result == 0) {
		mutex_lock(&pdev->buflock);
		status = copy_to_user(buf, &pdev->RingBuf[pdev->ring_buf_rd_pos], count);
		if (status < 0) {
			I_DEV_DBG(" copy user failed ");
			result = -EFAULT;
		} else {
			result = count;
			pdev->ring_buf_rd_pos += result;
		}
		mutex_unlock(&pdev->buflock);
		if ((SPI_DATA_FRAME_BUF/5) < unused_packet) {
			msleep(40);
			I_DEV_DBG(" read write gap : %d ", unused_packet);
		} else
			msleep(80);
	}
	I_DEV_DBG(" rd pos %lx , unused %x", pdev->ring_buf_rd_pos, unused_packet);

	return result;
}

static ssize_t isdbt_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int result = 0;
	int status = 0;
	isdbt_spi_info *pdev = (isdbt_spi_info *)(filp->private_data);

	mutex_lock(&pdev->buflock);

	status = copy_from_user(pdev->wBuf, buf, count);
	if (status < 0) {
		I_DEV_DBG(" copy user failed ");
		result = -EINVAL;
	}

	result = fc8100_spi_write(pdev->spidev, pdev->wBuf, count);

	if (result < 0)
		I_DEV_DBG(" write failed = %d ", result);

	mutex_unlock(&pdev->buflock);

	return result;
}


static struct file_operations isdbt_spi_fops = {
	.owner		=	THIS_MODULE,
	.open		=	isdbt_open,
	.release	=	isdbt_release,
	.read		=	isdbt_read,
	.write		=	isdbt_write,
};

static struct miscdevice isdbt_spi_misc_dev = {
	.minor	=	MISC_DYNAMIC_MINOR,
	.name	=	DEVICE_NAME,
	.fops	=	&isdbt_spi_fops,
};

static int __init isdbt_spi_init(void)
{
	int ret = 0;

	I_DEV_DBG("");

	ret = misc_register(&isdbt_spi_misc_dev);

	if (ret < 0)
		I_DEV_DBG(" ERROR ");
	else
		ret = fc8100_spi_driver_init();

	return ret;
}
module_init(isdbt_spi_init);

static void __exit isdbt_spi_exit(void)
{
	I_DEV_DBG("");
	fc8100_spi_exit();
	misc_deregister(&isdbt_spi_misc_dev);
}
module_exit(isdbt_spi_exit);

MODULE_DESCRIPTION(" ISDBT DRIVER - FC8100 ");
MODULE_AUTHOR("xmoondash");
MODULE_LICENSE("GPL");
