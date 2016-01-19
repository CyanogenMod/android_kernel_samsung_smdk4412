#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/workqueue.h>
#include <linux/unistd.h>
#include <linux/bug.h>

#include <mach/sec_gps.h>
#include <mach/gpio.h>

#include <plat/gpio-cfg.h>


#define IRQ_GPS_HOST_WAKE gpio_to_irq(GPIO_GPS_HOST_WAKE)

static struct device *gps_dev;
static wait_queue_head_t *p_geofence_wait; /*- BRCM -*/

/* BRCM
 * GPS geofence wakeup device implementation
 * to support waking up gpsd upon arrival of the interrupt
 * gpsd will select on this device to be notified of fence crossing event
 */

struct gps_geofence_wake {
	wait_queue_head_t wait;
	int irq;
	int host_req_pin;
	struct miscdevice misc;
};

static int gps_geofence_wake_open(struct inode *inode, struct file *filp)
{
	struct gps_geofence_wake *ac_data = container_of(filp->private_data,
			struct gps_geofence_wake,
			misc);

	filp->private_data = ac_data;
	return 0;
}

static int gps_geofence_wake_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static unsigned int gps_geofence_wake_poll(struct file *file, poll_table *wait)
{
	int gpio_value;
	struct gps_geofence_wake *ac_data = file->private_data;

	BUG_ON(!ac_data);

	poll_wait(file, &ac_data->wait, wait);

	gpio_value = gpio_get_value(ac_data->host_req_pin);

	/*printk(KERN_INFO "gpio geofence wake host_req=%d\n",gpio_value);*/

	if (gpio_value)
		return POLLIN | POLLRDNORM;

	return 0;
}


static long gps_geofence_wake_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	/* TODO
	 * Fill in useful commands
	 * (like test helper)
	 */
	/*
	switch(cmd) {
		default:
	}
	*/
	return 0;
}


static const struct file_operations gps_geofence_wake_fops = {
	.owner = THIS_MODULE,
	.open = gps_geofence_wake_open,
	.release = gps_geofence_wake_release,
	.poll = gps_geofence_wake_poll,
	/*.read = gps_geofence_wake_read,
	.write = gps_geofence_wake_write,*/
	.unlocked_ioctl = gps_geofence_wake_ioctl
};

static struct gps_geofence_wake geofence_wake;

static int gps_geofence_wake_init(int irq, int host_req_pin)
{
	int ret;

	struct gps_geofence_wake *ac_data = &geofence_wake;
	memset(ac_data, 0, sizeof(struct gps_geofence_wake));

	/* misc device setup */
	/* gps daemon will access "/dev/gps_geofence_wake" */
	ac_data->misc.minor = MISC_DYNAMIC_MINOR;
	ac_data->misc.name = "gps_geofence_wake";
	ac_data->misc.fops = &gps_geofence_wake_fops;

	/* information that be used later */
	ac_data->irq = irq;
	ac_data->host_req_pin = host_req_pin;

	/* initialize wait queue : */
	/* which will be used by poll or select system call */
	init_waitqueue_head(&ac_data->wait);

	printk(KERN_INFO "%s misc register, name %s, irq %d, host req pin num %d\n",
		__func__, ac_data->misc.name, irq, host_req_pin);

	ret = misc_register(&ac_data->misc);
	if (ret != 0) {
		printk(KERN_ERR "cannot register gps geofence wake miscdev on minor=%d (%d)\n",
			MISC_DYNAMIC_MINOR, ret);
		return ret;
	}

	p_geofence_wait = &ac_data->wait;

	return 0;
}

/* --------------- */


/*EXPORT_SYMBOL(p_geofence_wait);*/ /*- BRCM -*/

static irqreturn_t gps_host_wake_isr(int irq, void *dev)
{

	int gps_host_wake;

	gps_host_wake = gpio_get_value(GPIO_GPS_HOST_WAKE);
	irq_set_irq_type(irq,
		gps_host_wake ? IRQF_TRIGGER_FALLING : IRQF_TRIGGER_RISING);

	printk(KERN_ERR "%s GPS pin level[%s]\n", __func__,
		gps_host_wake ? "High" : "Low");

	if (p_geofence_wait && gps_host_wake) {
		printk(KERN_ERR "%s Wake-up GPS daemon[TRUE]\n", __func__);
		wake_up_interruptible(p_geofence_wait);
	}

	return IRQ_HANDLED;
}

static int __init gps_bcm47521_init(void)
{
	int irq = 0;
	int ret = 0;

	BUG_ON(!sec_class);
	gps_dev = device_create(sec_class, NULL, 0, NULL, "gps");
	BUG_ON(!gps_dev);

	s3c_gpio_cfgpin(GPIO_GPS_RXD, S3C_GPIO_SFN(GPIO_GPS_RXD_AF));
	s3c_gpio_setpull(GPIO_GPS_RXD, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_GPS_TXD, S3C_GPIO_SFN(GPIO_GPS_TXD_AF));
	s3c_gpio_setpull(GPIO_GPS_TXD, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_SFN(GPIO_GPS_CTS_AF));
	s3c_gpio_setpull(GPIO_GPS_CTS, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_SFN(GPIO_GPS_RTS_AF));
	s3c_gpio_setpull(GPIO_GPS_RTS, S3C_GPIO_PULL_UP);

	if (gpio_request(GPIO_GPS_PWR_EN, "GPS_PWR_EN")) {
		WARN(1, "fail to request gpio (GPS_PWR_EN)\n");
		return 1;
	}

	s3c_gpio_setpull(GPIO_GPS_PWR_EN, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_PWR_EN, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_GPS_PWR_EN, 0);


	if (gpio_request(GPIO_GPS_HOST_WAKE, "GPS_HOST_WAKE")) {
		WARN(1, "fail to request gpio(GPS_HOST_WAKE)\n");
		return 1;
	}

	s3c_gpio_setpull(GPIO_GPS_HOST_WAKE, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_HOST_WAKE,
		S3C_GPIO_SFN(GPIO_GPS_HOST_WAKE_AF));
	gpio_direction_input(GPIO_GPS_HOST_WAKE);

	gpio_export(GPIO_GPS_PWR_EN, 1);
	gpio_export(GPIO_GPS_HOST_WAKE, 1);

	gpio_export_link(gps_dev, "GPS_PWR_EN", GPIO_GPS_PWR_EN);
	gpio_export_link(gps_dev, "GPS_HOST_WAKE", GPIO_GPS_HOST_WAKE);

	irq = IRQ_GPS_HOST_WAKE;

	/* BRCM */
	ret = gps_geofence_wake_init(irq, GPIO_GPS_HOST_WAKE);
	if (ret) {
		pr_err("[GPS] gps_geofence_wake_init failed.\n");
		return ret;
	}
	/* -------------- */

	ret = request_irq(irq, gps_host_wake_isr,
		IRQF_TRIGGER_RISING, "gps_host_wake", NULL);
	if (ret) {
		pr_err("[GPS] Request_host wake irq failed.\n");
		return ret;
	}

	ret = irq_set_irq_wake(irq, 1);
	if (ret) {
		pr_err("[GPS] Set_irq_wake failed.\n");
		return ret;
	}

	return 0;
}

device_initcall(gps_bcm47521_init);

