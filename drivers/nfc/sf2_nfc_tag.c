#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/switch.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/nfc/nfc_tag.h>
#include <linux/device.h>
#include <mach/gpio-midas.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#define DRIVER_NAME "NFC_TAG"
#define NFC_DEVICE "nfc"

static atomic_t instantiated = ATOMIC_INIT(0);

struct device *nfctag_device;
struct class *nfctag_class;
static int is_event=0;

static ssize_t nfctag_show_check_event(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (is_event)
		return snprintf(buf, PAGE_SIZE, "YES");
	else
		return snprintf(buf, PAGE_SIZE, "NO");
}

static ssize_t nfctag_store_send_event(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	char name_buf[120];
	char state_buf[120];

	printk(KERN_INFO "%s \n", __func__);
	if (nfctag_device) {

		char *envp[3];
		int env_offset = 0;

		snprintf(name_buf, sizeof(name_buf),
					"SWITCH_NAME=NFC");
		envp[env_offset++] = name_buf;

		snprintf(state_buf, sizeof(state_buf),
				"SWITCH_STATE=%s", "NFC_IRQ");
		envp[env_offset++] = state_buf;

		envp[env_offset] = NULL;

		printk("Send NFC TAG Event to ABOVE LAYER\n");
		is_event=1;
		kobject_uevent_env(&nfctag_device->kobj, KOBJ_CHANGE, envp);
	}else
		is_event=0;

		return count;
}
static DEVICE_ATTR(getevent, 0664,
		   nfctag_show_check_event, nfctag_store_send_event);

static irqreturn_t nfc_tag_detect_irq_thread(int irq, void *dev_id)
{
	char name_buf[120];
	char state_buf[120];

	pr_info("%s : INSIDE the NFC tag handler\n", __func__);
	if (nfctag_device) {
		char *envp[3];
		int env_offset = 0;

		snprintf(name_buf, sizeof(name_buf),
					"SWITCH_NAME=NFC");
		envp[env_offset++] = name_buf;

		snprintf(state_buf, sizeof(state_buf),
				"SWITCH_STATE=%s", "NFC_IRQ");
		envp[env_offset++] = state_buf;

		envp[env_offset] = NULL;

		printk("Send NFC TAG Event to ABOVE LAYER\n");

		is_event=1;

		kobject_uevent_env(&nfctag_device->kobj, KOBJ_CHANGE, envp);
	}
	else
		is_event=0;


	return IRQ_HANDLED;
}

static ssize_t
nfc_tag_read(struct file *filp, char __user *buff,
		size_t count, loff_t *fpos)
{
	return 0;
}
static ssize_t
nfc_tag_write(struct file *filp, const char __user *buff,
		size_t size, loff_t *fpos)
{
	return 0;
}
static const struct file_operations nfc_tag_fops = {
	.owner = THIS_MODULE,
	.read = nfc_tag_read,
	.write = nfc_tag_write,
};
static int register_nfc_tag(void)
{
	int ret=0;
	int major_no;

	major_no = register_chrdev(0, NFC_DEVICE, &nfc_tag_fops);
	if (major_no < 0) {
		pr_err("Unable to get major number for NFC_TAG dev\n");
		ret = -EBUSY;
		goto err1;
	}

	nfctag_class = class_create(THIS_MODULE, NFC_DEVICE);
	if (IS_ERR(nfctag_class)) {
		pr_err("class create err\n");
		ret = PTR_ERR(nfctag_class);
		goto err2;
	}

	nfctag_device = device_create(nfctag_class,
		NULL, MKDEV(major_no, 0), NULL, DRIVER_NAME);
	if (IS_ERR(nfctag_device)) {
		pr_err("device create err\n");
		ret = PTR_ERR(nfctag_device);
		goto err3;
	}

	if (device_create_file(nfctag_device, &dev_attr_getevent) < 0)
	{
		ret =-EFAULT;
		pr_err("Failed to create device file(.usblock/enable)!\n");
		goto err4;
	}

	return 0;

err4:
	device_del(nfctag_device);
err3:
	class_destroy(nfctag_class);
err2:
	unregister_chrdev(major_no, NFC_DEVICE);
err1:
	return ret;
}
static int nfc_tag_probe(struct platform_device *pdev)
{
	struct nfc_tag_info *info;
	struct nfc_tag_pdata *pdata = pdev->dev.platform_data;
	int ret;

	pr_info("%s : Registering NFC TAG driver\n", __func__);
	if (!pdata) {
		pr_err("%s : pdata is NULL.\n", __func__);
		ret =-ENODEV;
		goto err1;
	}

	info = kzalloc(sizeof(struct nfc_tag_info), GFP_KERNEL);
	if (info == NULL) {
		pr_err("%s : Failed to allocate memory.\n", __func__);
		ret = -ENOMEM;
		goto err1;
	}

	ret = register_nfc_tag();
	if(ret<0)
	{
		pr_err("%s : pdata is NULL.\n", __func__);
		goto err_gpio_request;
	}
	info->pdata = pdata;
	ret = gpio_request(pdata->irq_nfc, "nfc_tag_irq");
	if (ret) {
		pr_err("%s : gpio_request failed for %d\n",
		       __func__, pdata->irq_nfc);
		goto err_gpio_request;
	}
	info->irq_no = gpio_to_irq(pdata->irq_nfc);

	ret = request_threaded_irq(info->irq_no, NULL,
				   nfc_tag_detect_irq_thread,
				   IRQF_TRIGGER_FALLING, "nfc_tag_irq_detect", info);
	if (ret) {
		pr_err("%s : Failed to request_irq.\n", __func__);
		goto err_req_irq;
	}

	return 0;

err_req_irq:
	gpio_free(info->pdata->irq_nfc);
err_gpio_request:
	kfree(info);
err1:
	return ret;
}
static int nfc_tag_remove(struct platform_device *pdev)
{
	struct nfc_tag_info *info = dev_get_drvdata(&pdev->dev);

	free_irq(info->irq_no, info);
	gpio_free(info->pdata->irq_nfc);
	kfree(info);
	return 0;
}
static struct platform_driver nfc_tag_det_driver = {
	.probe	= nfc_tag_probe,
	.remove	= nfc_tag_remove,
	.driver	= {
		.name = "NFC_TAG",
		.owner = THIS_MODULE,
	},
};

static int __init nfc_tag_init(void)
{
	int ret;

	ret =  platform_driver_register(&nfc_tag_det_driver);

	if (ret)
		pr_err("%s: Failed to add NFC tag detection driver\n", __func__);

	return ret;
}

static void __exit nfc_tag_exit(void)
{
	platform_driver_unregister(&nfc_tag_det_driver);
}

module_init(nfc_tag_init);
module_exit(nfc_tag_exit);

MODULE_AUTHOR("vikram.nr@samsung.com");
MODULE_DESCRIPTION("Samsung Electronics Corp NFC_TAG interrupt detection driver");
