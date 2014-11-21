
/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/log2.h>
#include <linux/spinlock.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/switch.h>
#include <linux/workqueue.h>
#include <linux/irqdesc.h>
#include <linux/fs.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/irqdesc.h>
#include <linux/wakelock.h>

#define REPORT_KEY		0
#define FLIP_NOTINIT		-1
#define FLIP_OPEN			1
#define FLIP_CLOSE		0	

#define FLIP_SCAN_INTERVAL	(50)	/* ms */
#define FLIP_STABLE_COUNT	(1)
#define GPIO_FLIP  GPIO_HALL_SW

extern struct class *sec_class;

#if 0 /* DEBUG */
#define dbg_printk(fmt, ...) \
	printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__)
#else
#define dbg_printk(fmt, ...) \
	({ if (0) printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__); 0; })
#endif

/////////////////////////////////////////////////////////////////////
struct sec_flip_pdata {
	int wakeup;
	int debounce_interval;
	unsigned int rep:1;		/* enable input subsystem auto repeat */
}; 

struct sec_flip {
	struct input_dev *input;
	struct wake_lock wlock; 
	struct workqueue_struct *wq;	/* The actuak work queue */
	struct work_struct flip_id_det;	/* The work being queued */
	struct timer_list flip_timer;
	struct device *sec_flip;
	int		timer_debounce;
	int		gpio;
	int    		irq;
};


/////////////////////////////////////////////////////////////////////
#ifdef CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD
extern void s3cfb_switch_dual_lcd(int lcd_sel);
#endif
extern void samsung_switching_tsp(int flip);
extern void samsung_switching_tkey(int flip);
/////////////////////////////////////////////////////////////////////

static int flip_status;
static int flip_status_before;

/* 0 : open, 1: close */
int Is_folder_state(void)
{
	printk("%s: flip_status = %d\n", __func__,flip_status); 
	return !flip_status;
}
EXPORT_SYMBOL(Is_folder_state);


static void sec_report_flip_key(struct sec_flip *flip)
{
#if REPORT_KEY
	if (flip_status) {
		input_report_key(flip->input, KEY_FOLDER_OPEN, 1);
		input_report_key(flip->input, KEY_FOLDER_OPEN, 0);
		input_sync(flip->input);
		dbg_printk("[FLIP] %s: input flip key :  open\n", __FUNCTION__);
	} else {
		input_report_key(flip->input, KEY_FOLDER_CLOSE, 1);
		input_report_key(flip->input, KEY_FOLDER_CLOSE, 0);
		input_sync(flip->input);
		dbg_printk ("[FLIP] %s: input flip key : close\n", __FUNCTION__);
	}
#else
	if (flip_status) {
		input_report_switch(flip->input, SW_LID, 0);
		input_sync(flip->input);
		dbg_printk("[FLIP] %s: input flip key :  open\n", __FUNCTION__);
	} else {
		input_report_switch(flip->input, SW_LID, 1);
		input_sync(flip->input);
		dbg_printk ("[FLIP] %s: input flip key : close\n", __FUNCTION__);
	}
#endif
}

static void set_flip_status(struct sec_flip *flip)
{
	int val = 0;

	val = gpio_get_value_cansleep(flip->gpio);
	dbg_printk("%s, val:%d, flip_status:%d\n", __FUNCTION__, val, flip_status);
	if (flip_status != val) {
		flip_status_before = flip_status;
		flip_status = val ? FLIP_OPEN : FLIP_CLOSE;
	}

}

static void sec_flip_work_func(struct work_struct *work)
{
	struct sec_flip* flip = container_of( work, struct sec_flip, flip_id_det); 

	//enable_irq(flip->irq);

	set_flip_status(flip);
	printk("%s: %s, before:%d \n", __func__, (flip_status) ? "OPEN 1" : "CLOSE 0", flip_status_before);

	sec_report_flip_key(flip);

	if (flip_status != flip_status_before) {
#ifdef CONFIG_S5P_DSIM_SWITCHABLE_DUAL_LCD
		s3cfb_switch_dual_lcd(!flip_status);
#endif
		samsung_switching_tsp(flip_status);
		samsung_switching_tkey(flip_status);
	}
}

static irqreturn_t sec_flip_irq_handler(int irq, void *_flip)
{
	struct sec_flip *flip = _flip;

	dbg_printk("%s\n", __FUNCTION__);

/*
	val = gpio_get_value_cansleep(flip->gpio);  

	if(val){		// OPEN
		flip_status = 1;		
	} else{			// CLOSE
		flip_status = 0;
		samsung_switching_tsp_pre(0, flip_status);
	}
	printk("[FLIP] %s: val=%d (1:open, 0:close)\n", __func__, val);

	wake_lock_timeout(&flip->wlock, 1 * HZ);
*/
	if (flip->timer_debounce)
		mod_timer(&flip->flip_timer,
			jiffies + msecs_to_jiffies(flip->timer_debounce));
	else
		queue_work(flip->wq, &flip->flip_id_det );

	return IRQ_HANDLED;
}


void sec_flip_timer(unsigned long data)
{
	struct sec_flip* flip = (struct sec_flip*)data; 

	dbg_printk("%s\n", __FUNCTION__);
	queue_work(flip->wq, &flip->flip_id_det);
}

#if 1 /*CONFIG_PM*/
static int sec_flip_suspend(struct device *dev)
{
	struct sec_flip *flip = dev_get_drvdata(dev);

	dbg_printk(KERN_INFO "[FLIP]%s:\n", __func__);

	if (device_may_wakeup(dev)) {
		enable_irq_wake(flip->irq);
		printk(KERN_INFO "[FLIP]%s: enable_irq_wake\n", __func__);
	}

	return 0;
}

static int sec_flip_resume(struct device *dev)
{
	struct sec_flip *flip = dev_get_drvdata(dev);

	dbg_printk("[FLIP]%s:\n", __func__);

	if (device_may_wakeup(dev)) {
		disable_irq_wake(flip->irq);
		printk(KERN_INFO "[FLIP]%s: disable_irq_wake\n", __func__);
	}

	return 0;
}

static struct dev_pm_ops pm8058_flip_pm_ops = {
	.suspend	= sec_flip_suspend,
	.resume		= sec_flip_resume,
};
#endif

static ssize_t status_check(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("flip status check :  %d\n", flip_status);
	return sprintf(buf, "%d\n", flip_status);
}

static DEVICE_ATTR(flipStatus, S_IRUGO | S_IWUSR | S_IWGRP , status_check, NULL);

static int __devinit sec_flip_probe(struct platform_device *pdev)
{
	struct input_dev *input;
	int err;
	struct sec_flip *flip;
	struct sec_flip_pdata *pdata = pdev->dev.platform_data;

	dev_info(&pdev->dev, "probe\n");

	if (!pdata) {
		dev_err(&pdev->dev, "power key platform data not supplied\n");
		return -EINVAL;
	}

	flip = kzalloc(sizeof(*flip), GFP_KERNEL);
	if (!flip)
		return -ENOMEM;

/* INPUT DEVICE */
	input = input_allocate_device();
	if (!input) {
		dev_err(&pdev->dev, "Can't allocate power button\n");
		err = -ENOMEM;
		goto free_flip;
	}

	if (pdata->rep)
		__set_bit(EV_REP, input->evbit);

#if REPORT_KEY
	input_set_capability(input, EV_KEY, KEY_FOLDER_OPEN);
	input_set_capability(input, EV_KEY, KEY_FOLDER_CLOSE);
#else
	input_set_capability(input, EV_SW, SW_LID);
#endif
	
	input->name = "sec_flip";
	input->phys = "sec_flip/input0";
	input->dev.parent = &pdev->dev;

	err = input_register_device(input);
	if (err) {
		dev_err(&pdev->dev, "Can't register power key: %d\n", err);
		goto free_input_dev;
	}
	flip->input = input;

	platform_set_drvdata(pdev, flip);
	wake_lock_init(&flip->wlock, WAKE_LOCK_SUSPEND, "sec_flip");
	setup_timer(&flip->flip_timer, sec_flip_timer, (unsigned long)flip);

/* Initialize the static variable*/
	flip_status = FLIP_NOTINIT;
	flip_status_before = FLIP_NOTINIT;

/* INTERRUPT */ 
 	flip->gpio = GPIO_FLIP; 
	flip->irq = gpio_to_irq(GPIO_FLIP);

	if (pdata->debounce_interval) {
		err = gpio_set_debounce(flip->gpio,
					  pdata->debounce_interval * 1000);
		/* use timer if gpiolib doesn't provide debounce */
		if (err < 0)
			flip->timer_debounce = pdata->debounce_interval;

		printk("%s:   error:%d, timer_debounce=%d\n", __func__, err, flip->timer_debounce);
	}
	

	flip->wq = create_singlethread_workqueue("sec_flip_wq");
	if (!flip->wq) {
		dev_err(&pdev->dev, "create_workqueue failed.\n"); 
	}
	INIT_WORK(&flip->flip_id_det, sec_flip_work_func);

	device_init_wakeup(&pdev->dev, pdata->wakeup);


	err = request_threaded_irq(flip->irq, NULL, sec_flip_irq_handler, (IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING), "flip_det_irq", flip);
	if (err < 0) {
		dev_err(&pdev->dev, "Can't get %d IRQ for flip: %d\n",
				 flip->irq, err);
		goto unreg_input_dev;
	}
       /*enable_irq_wake(flip->irq);*/
	/*disable_irq_nosync(flip->irq);*/
	
	printk("%s:   gpio=%d   irq=%d\n", __func__,  flip->gpio, flip->irq);

/* SYSFS for FACTORY TEST */
	flip->sec_flip = device_create(sec_class, NULL, 0, NULL, "sec_flip");
	err = device_create_file(flip->sec_flip , &dev_attr_flipStatus);
	if(err<0){
		printk("flip status check cannot create file :  %d\n", flip_status);
		goto free_flip;
	}

	mod_timer(&flip->flip_timer, jiffies + msecs_to_jiffies(5000));
	return 0;


unreg_input_dev:
	input_unregister_device(input);
	input = NULL;
free_input_dev:
	input_free_device(input);
free_flip:

	kfree(flip);
	
	return err;
}

static int __devexit sec_flip_remove(struct platform_device *pdev)
{
	struct sec_flip *flip = platform_get_drvdata(pdev);

	printk("%s:\n", __func__); 

	if (flip!=NULL)
		del_timer_sync(&flip->flip_timer);

	device_init_wakeup(&pdev->dev, 0);

	if (flip!=NULL) {
		free_irq(flip->irq, NULL);
		destroy_workqueue(flip->wq); 
		input_unregister_device(flip->input);
		kfree(flip);
	}

	return 0;
}

static struct platform_driver sec_flip_driver = {
	.probe		= sec_flip_probe,
	.remove		= __devexit_p(sec_flip_remove),
	.driver		= {
		.name	= "sec_flip",
		.owner	= THIS_MODULE,
#if 1 /* CONFIG_PM */
		.pm	= &pm8058_flip_pm_ops,
#endif
	},
};

static int __init sec_flip_init(void)
{
	return platform_driver_register(&sec_flip_driver);
}
module_init(sec_flip_init);

static void __exit sec_flip_exit(void)
{
	platform_driver_unregister(&sec_flip_driver);
}
module_exit(sec_flip_exit);

MODULE_ALIAS("platform:sec_flip");
MODULE_DESCRIPTION("Flip Key with GPIO");
MODULE_LICENSE("GPL v2");
