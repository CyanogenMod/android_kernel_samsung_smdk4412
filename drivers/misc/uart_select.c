/*
 * uart_sel.c - UART Selection Driver
 *
 * Copyright (C) 2009 Samsung Electronics
 * Kim Kyuwon <q1.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/uart_select.h>

struct uart_select {
	struct uart_select_platform_data	*pdata;
	struct rw_semaphore			rwsem;
};

static int uart_saved_state = UART_SW_PATH_NA;

static ssize_t uart_select_show_state(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct uart_select *uart_sel =
			platform_get_drvdata(to_platform_device(dev));
	int ret;
	int path;

	path = uart_sel->pdata->get_uart_switch();

	down_read(&uart_sel->rwsem);
	uart_saved_state = path;
	if (path == UART_SW_PATH_NA)
		ret = sprintf(buf, "NA\n");
	else if (path == UART_SW_PATH_CP)
		ret = sprintf(buf, "CP\n");
	else
		ret = sprintf(buf, "AP\n");
	up_read(&uart_sel->rwsem);

	return ret;
}

static ssize_t uart_select_store_state(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct uart_select *uart_sel =
			platform_get_drvdata(to_platform_device(dev));
	struct uart_select_platform_data *pdata = uart_sel->pdata;
	int path;

	if (!count)
		return -EINVAL;

	down_write(&uart_sel->rwsem);
	if (!strncmp(buf, "CP", 2))
		path = UART_SW_PATH_CP;
	else if (!strncmp(buf, "AP", 2))
		path = UART_SW_PATH_AP;
	else {
		up_write(&uart_sel->rwsem);
		dev_err(dev, "Invalid cmd !!\n");
		return -EINVAL;
	}
	pdata->set_uart_switch(path);
	uart_saved_state = path;
	up_write(&uart_sel->rwsem);

	return count;
}

static struct device_attribute uart_select_attr = {
	.attr = {
		.name = "path",
		.mode = 0644,
	},
	.show = uart_select_show_state,
	.store = uart_select_store_state,
};

/* Used in uart isr to avoid triggering sysrq when uart is not in AP */
int uart_sel_get_state(void)
{
	if (uart_saved_state < 0)
		return -EPERM;
	else
		return uart_saved_state;
}
EXPORT_SYMBOL(uart_sel_get_state);

static int __devinit uart_select_probe(struct platform_device *pdev)
{
	struct uart_select *uart_sel;
	struct uart_select_platform_data *pdata = pdev->dev.platform_data;
	int ret;

	uart_sel = kzalloc(sizeof(struct uart_select), GFP_KERNEL);
	if (!uart_sel) {
		dev_err(&pdev->dev, "failed to allocate driver data\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, uart_sel);
	uart_sel->pdata = pdata;
	init_rwsem(&uart_sel->rwsem);

	uart_saved_state = pdata->get_uart_switch();

	ret = device_create_file(&pdev->dev, &uart_select_attr);
	if (ret) {
		dev_err(&pdev->dev, "failed to crreate device file\n");
		return ret;
	}

	return 0;
}

static int __devexit uart_select_remove(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &uart_select_attr);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver uart_select_driver = {
	.probe		= uart_select_probe,
	.remove		= __devexit_p(uart_select_remove),
	.driver		= {
		.name	= "uart-select",
		.owner	= THIS_MODULE,
	},
};

static int __init uart_select_init(void)
{
	return platform_driver_register(&uart_select_driver);
}
module_init(uart_select_init);

static void __exit uart_select_exit(void)
{
	platform_driver_unregister(&uart_select_driver);
}
module_exit(uart_select_exit);

MODULE_AUTHOR("Kim Kyuwon <q1.kim@samsung.com>");
MODULE_DESCRIPTION("UART Selection Driver");
MODULE_LICENSE("GPL v2");
