/*
 * Samsung Mobile VE Group.
 *
 * drivers/gpio/secgpio_dvs.c
 *
 * Drivers for samsung gpio debugging & verification.
 *
 * Copyright (C) 2013, Samsung Electronics.
 *
 * This program is free software. You can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <linux/secgpio_dvs.h>

/*sys fs*/
struct class *secgpio_dvs_class;
EXPORT_SYMBOL(secgpio_dvs_class);

struct device *secgpio_dotest;
EXPORT_SYMBOL(secgpio_dotest);

/* extern GPIOMAP_RESULT GpioMap_result; */
static struct gpio_dvs_t *gdvs_info;

static ssize_t checked_info_secgpio_file_read(
	struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t checked_init_secgpio_file_read(
	struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t checked_sleep_secgpio_file_read(
	struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t checked_secgpio_init_read_details(
	struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t checked_secgpio_sleep_read_details(
	struct device *dev, struct device_attribute *attr, char *buf);
#ifdef SECGPIO_SLEEP_DEBUGGING
static ssize_t secgpio_sleep_debug_write(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size);
#endif

static DEVICE_ATTR(gpioinfo_check, 0440,
	checked_info_secgpio_file_read, NULL);
static DEVICE_ATTR(gpioinit_check, 0440,
	checked_init_secgpio_file_read, NULL);
static DEVICE_ATTR(gpiosleep_check, 0440,
	checked_sleep_secgpio_file_read, NULL);
static DEVICE_ATTR(check_init_detail, 0440,
	checked_secgpio_init_read_details, NULL);
static DEVICE_ATTR(check_sleep_detail, 0440,
	checked_secgpio_sleep_read_details, NULL);
#ifdef SECGPIO_SLEEP_DEBUGGING
static DEVICE_ATTR(gpio_sleep_debug, 0220 , NULL, secgpio_sleep_debug_write);
#endif

static struct attribute *secgpio_dvs_attributes[] = {
		&dev_attr_gpioinfo_check.attr,
		&dev_attr_gpioinit_check.attr,
		&dev_attr_gpiosleep_check.attr,
		&dev_attr_check_init_detail.attr,
		&dev_attr_check_sleep_detail.attr,
#ifdef SECGPIO_SLEEP_DEBUGGING
		&dev_attr_gpio_sleep_debug.attr,
#endif
		NULL,
};

static struct attribute_group secgpio_dvs_attr_group = {
		.attrs = secgpio_dvs_attributes,
};

#ifdef SECGPIO_SLEEP_DEBUGGING
static int atoi(const char *str)
{
	int result = 0;
	int count = 0;
	if (str == NULL)
		return -EIO;
	while (str[count] != 0	/* NULL */
		&& str[count] >= '0' && str[count] <= '9')	{
		result = result * 10 + str[count] - '0';
		++count;
	}
	return result;
}

static char *
strtok_r(char *s, const char *delim, char **last)
{
	char *spanp;
	int c, sc;
	char *tok;


	/* if (s == NULL && (s = *last) == NULL)
		return NULL;	 */
	if (s == NULL) {
		s = *last;
		if (s == NULL)
			return NULL;
	}

	/*
	 * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
	 */
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {		/* no non-delimiter characters */
		*last = NULL;
		return NULL;
	}
	tok = s - 1;

	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			sc = *spanp++;
			if (sc == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*last = s;
				return tok;
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}

static char *
strtok(char *s, const char *delim)
{
	static char *last;

	return strtok_r(s, delim, &last);
}
#endif

static ssize_t checked_info_secgpio_file_read(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct gpio_dvs_t *gdvs = dev_get_drvdata(dev);

	snprintf(buf, PAGE_SIZE, "%d %d %d ",
		gdvs->count, gdvs->check_init, gdvs->check_sleep);

	return strlen(buf);
}

static ssize_t checked_init_secgpio_file_read(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	char temp_buf[20];
	struct gpio_dvs_t *gdvs = dev_get_drvdata(dev);

	for (i = 0; i < gdvs->count; i++) {
		memset(temp_buf, 0, sizeof(char)*20);
		snprintf(temp_buf, 20, "%x ", gdvs->result->init[i]);
		strlcat(buf, temp_buf, PAGE_SIZE);
	}

	return strlen(buf);
}

static ssize_t checked_sleep_secgpio_file_read(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	char temp_buf[20];
	struct gpio_dvs_t *gdvs = dev_get_drvdata(dev);

	for (i = 0; i < gdvs->count; i++) {
		memset(temp_buf, 0, sizeof(char)*20);
		snprintf(temp_buf, 20, "%x ", gdvs->result->sleep[i]);
		strlcat(buf, temp_buf, PAGE_SIZE);
	}

	return strlen(buf);
}

static ssize_t checked_secgpio_init_read_details(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	char temp_buf[20];
	struct gpio_dvs_t *gdvs = dev_get_drvdata(dev);

	for (i = 0; i < gdvs->count; i++) {
		memset(temp_buf, 0, sizeof(char)*20);
		snprintf(temp_buf, 20, "GI[%d] - %x\n ",
			i, gdvs->result->init[i]);
		strlcat(buf, temp_buf, PAGE_SIZE);
	}

	return strlen(buf);
}
static ssize_t checked_secgpio_sleep_read_details(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	char temp_buf[20];
	struct gpio_dvs_t *gdvs = dev_get_drvdata(dev);

	for (i = 0; i < gdvs->count; i++) {
		memset(temp_buf, 0, sizeof(char)*20);
		snprintf(temp_buf, 20, "GS[%d] - %x\n ",
			i, gdvs->result->sleep[i]);
		strlcat(buf, temp_buf, PAGE_SIZE);
	}

	return strlen(buf);

}

#ifdef SECGPIO_SLEEP_DEBUGGING
static int sleepdata_cur_pos;
static char sdata_phase = SLDATA_START;
static ssize_t secgpio_sleep_debug_write(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	char *token = NULL;
	char comp[] = ",";
	int gpio_ctrl[3] = {0,};
	int num = 0;
	int count = 0;
	char temp_buf[50];
	struct gpio_dvs_t *gdvs = dev_get_drvdata(dev);

	pr_info("[GPIO_DVS][%s] buf = %s\n", __func__, buf);

	memset(temp_buf, 0, 50);
	strlcpy(temp_buf, buf, 50);

	/* Data Format example
	  * Start Data : Cxxx (xxx : gpio count, ex : C12)
	  * Data : Dxxx,y,z	(xxx :  gpio_num, y : In/Out, z : State H/L, ex : D112,2,0)
	  *		refer to the standard document for value
	  * End Data : D
	*/
	if (temp_buf[0] == 'C') { /* sleep data start */
		count =  atoi(&temp_buf[1]);

		if (count <= gdvs->count) {
			/* data Initialize */
			gdvs->sdebugtable->active = false;
			sleepdata_cur_pos = 0;
			gdvs->sdebugtable->gpio_count = 0;
			if (gdvs->sdebugtable->gpioinfo != NULL) {
				kfree(gdvs->sdebugtable->gpioinfo);
				gdvs->sdebugtable->gpioinfo = NULL;
			}

			gdvs->sdebugtable->gpio_count = count;
			pr_info("[GPIO_DVS] count = %d\n", count);

			/* gpioinfo must be released later!! */
			gdvs->sdebugtable->gpioinfo =
				kzalloc(sizeof(struct sleepdebug_gpioinfo) * count, GFP_KERNEL);
			if (gdvs->sdebugtable->gpioinfo == NULL)
				pr_err("[GPIO_DVS][%s] fail to memalloc!!\n", __func__);
			else
				sdata_phase = SLDATA_DATA;
		} else {
			pr_err("[GPIO_DVS] gpio count error!!(%d)\n", count);
			sdata_phase = SLDATA_START;
		}
	}
	/* sleep data */
	else if (sdata_phase == SLDATA_DATA && temp_buf[0] == 'D') {
		token = strtok(&temp_buf[1], comp);

		for (num = 0; num < 3; num++) {
			if (token != NULL) {
				pr_info("[GPIO_DVS] GPIO Control TOKEN = %s\n",
					token);
				gpio_ctrl[num] = atoi(token);
				token = strtok(NULL, comp);
			}
			pr_info("[GPIO_DVS] GPIO Control[%d] = %d\n",
				num, gpio_ctrl[num]);
		}

		/*
			gpio_ctrl[0] = GPIO NUMBER, gpio_ctrl[1] = IN/OUT,
			gpio_ctrl[2] = L/H (OUT), PD/PU (IN)
		*/
		gdvs->sdebugtable->gpioinfo[sleepdata_cur_pos].gpio_num = gpio_ctrl[0];
		gdvs->sdebugtable->gpioinfo[sleepdata_cur_pos].io = gpio_ctrl[1];
		if (gpio_ctrl[1] == 1)		/* IN */
			gdvs->sdebugtable->gpioinfo[sleepdata_cur_pos].pupd = gpio_ctrl[2];
		else if (gpio_ctrl[1] == 2)		/* OUT */
			gdvs->sdebugtable->gpioinfo[sleepdata_cur_pos].lh = gpio_ctrl[2];
		else		/* It should not be runned */
			gdvs->sdebugtable->gpioinfo[sleepdata_cur_pos].pupd = gpio_ctrl[2];

		sleepdata_cur_pos++;
		if (sleepdata_cur_pos == gdvs->sdebugtable->gpio_count)
			sdata_phase = SLDATA_END;
	}
	/* sleep data end */
	else if (sdata_phase == SLDATA_END && temp_buf[0] == 'E') {
		pr_info("[GPIO_DVS] GPIO debug On!!\n");
		gdvs->sdebugtable->active = true;
		sdata_phase = SLDATA_START;
	}
	else {
		pr_err("[GPIO_DVS][%s] Phase Error!!(%d)\n", __func__, sdata_phase);
	}

	return size;
}
#endif


void gpio_dvs_check_initgpio(void)
{
	if (gdvs_info && gdvs_info->check_gpio_status)
	{
		gdvs_info->check_gpio_status(PHONE_INIT);
		gdvs_info->check_init = true;
	}
	pr_info("[%s] !!!\n", __func__);
}

void gpio_dvs_check_sleepgpio(void)
{
	if (unlikely(!gdvs_info->check_sleep)) {
		gdvs_info->check_gpio_status(PHONE_SLEEP);
		gdvs_info->check_sleep = true;
	}
	pr_info("[%s] !!!\n", __func__);
}

#ifdef SECGPIO_SLEEP_DEBUGGING
void gpio_dvs_set_sleepgpio(void)
{
	if (unlikely(gdvs_info->sdebugtable->active)) {
		/* setting function */
		gdvs_info->set_sleepgpio();
	}

	return;
}

void gpio_dvs_undo_sleepgpio(void)
{
	if (unlikely(gdvs_info->sdebugtable->active)) {
		/* undoing function */
		gdvs_info->undo_sleepgpio();
		/* data initialize */
		gdvs_info->sdebugtable->active = false;
		sleepdata_cur_pos = 0;
		gdvs_info->sdebugtable->gpio_count = 0;
		if (gdvs_info->sdebugtable->gpioinfo != NULL) {
			kfree(gdvs_info->sdebugtable->gpioinfo);
			gdvs_info->sdebugtable->gpioinfo = NULL;
		}
	}
}
#endif

static int secgpio_dvs_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct class *secgpio_dvs_class;
	struct device *secgpio_dotest;
	struct gpio_dvs_t *gdvs = dev_get_platdata(&pdev->dev);

	pr_info("[GPIO_DVS] %s has been created!!!\n", __func__);

	secgpio_dvs_class = class_create(THIS_MODULE, "secgpio_check");
	if (IS_ERR(secgpio_dvs_class)) {
		ret = PTR_ERR(secgpio_dvs_class);
		pr_err("Failed to create class(secgpio_check_all)");
		goto fail_out;
	}

	secgpio_dotest = device_create(secgpio_dvs_class,
				NULL, 0, NULL, "secgpio_check_all");
	if (IS_ERR(secgpio_dotest)) {
		ret = PTR_ERR(secgpio_dotest);
		pr_err("Failed to create device(secgpio_check_all)");
		goto fail_out;
	}
	dev_set_drvdata(secgpio_dotest, gdvs);
	gdvs_info = gdvs;

	ret = sysfs_create_group(&secgpio_dotest->kobj,
			&secgpio_dvs_attr_group);
	if (ret) {
		pr_err("Failed to create sysfs group");
		goto fail_out;
	}

fail_out:
	if (ret)
		pr_err(" (err = %d)!\n", ret);
	return ret;

}

static int secgpio_dvs_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver secgpio_dvs = {
	.probe = secgpio_dvs_probe,
	.remove = secgpio_dvs_remove,
	.driver = {
		.name = "secgpio_dvs",
		.owner = THIS_MODULE,
	},
};

static int __init secgpio_dvs_init(void)
{
	int ret;
	ret = platform_driver_register(&secgpio_dvs);
	pr_info("[GPIO_DVS] secgpio_dvs_init has been initialized(%d)!!!\n", ret);
	return ret;
}

static void __exit secgpio_dvs_exit(void)
{
	platform_driver_unregister(&secgpio_dvs);
}

/* For compatibility of IORA Init Breakpoint, I have modified exceptively the following function. */
/* arch_initcall(secgpio_dvs_init); */
postcore_initcall(secgpio_dvs_init);
module_exit(secgpio_dvs_exit);

MODULE_AUTHOR("intae.jun@samsung.com");
MODULE_DESCRIPTION("BCM2165x GPIO debugging and verification");
MODULE_LICENSE("GPL");
