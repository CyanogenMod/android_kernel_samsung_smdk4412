
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/regulator/machine.h>
#include <linux/ir_remote_con.h>
#include <asm/irq.h>
#include <mach/cpufreq.h>

#define BIT_SIZE 32
#define MAX_SIZE 1024
#define NANO_SEC 1000000000
#define MICRO_SEC 1000000

static struct regulator	*regulator;
static int regulator_status = -1;

struct ir_remocon_data {
	struct mutex mutex;
	struct work_struct work;
	int gpio;
	int pwr_en;
	unsigned int signal[MAX_SIZE];
};

static int gpio_init(struct ir_remocon_data *data)
{
	int err;
	err = gpio_request(data->gpio, "ir_gpio");
	if (err < 0) {
		pr_err("failed to request GPIO %d, error %d\n",
		       data->gpio, err);
	} else
		gpio_direction_output(data->gpio, 0);
	return err;
}

static void ir_remocon_send(struct ir_remocon_data *data)
{
	unsigned int		period, off_period = 0;
	unsigned int		duty;
	unsigned int		on, off = 0;
	unsigned int		i, j;
	int					ret;
	static int		cpu_lv = -1;

	if (data->pwr_en == -1) {
		regulator = regulator_get(NULL, "vled_3.3v");
		if (IS_ERR(regulator))
			goto out;

		regulator_enable(regulator);
		regulator_status = 1;
	}

	if (data->pwr_en != -1)
		gpio_direction_output(data->pwr_en, 1);

	__udelay(1000);

	if (cpu_lv == -1) {
		if (data->pwr_en == -1)
			exynos_cpufreq_get_level(800000, &cpu_lv);
		else
			exynos_cpufreq_get_level(800000, &cpu_lv);
	}

	ret = exynos_cpufreq_lock(DVFS_LOCK_ID_IR_LED, cpu_lv);
	if (ret < 0)
		pr_err("%s: fail to lock cpufreq\n", __func__);

	ret = exynos_cpufreq_upper_limit(DVFS_LOCK_ID_IR_LED, cpu_lv);
	if (ret < 0)
		pr_err("%s: fail to lock cpufreq(limit)\n", __func__);

	 period  = (MICRO_SEC/data->signal[0]);
	 if ((MICRO_SEC % data->signal[0]) >= (data->signal[0]/2)) {
		if (data->pwr_en == -1) {
#ifdef CONFIG_MACH_P2
			if (data->signal[0] < 50000)
				period += 1;
#endif
#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE) || defined(CONFIG_MACH_TAB3)
				period += 1;
#endif
		} else
			period += 1;
	}

	duty = period/4;
	if ((period % 4) >= 2)
		duty += 1;
	on = duty;

	if (data->pwr_en == -1)
		off = (period - duty) - 3;
	else
		off = (period - duty) - 2;

#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE) || defined(CONFIG_MACH_TAB3)
	 off = (period - duty) - 3;
#endif

	local_irq_disable();
	for (i = 1; i < MAX_SIZE; i += 2) {
		if (data->signal[i] == 0)
			break;

		for (j = 0; j < data->signal[i]; j++) {
			gpio_direction_output(data->gpio, 1);
			__udelay(on);
			gpio_direction_output(data->gpio, 0);
			__udelay(off);
		}

		if (data->pwr_en == -1)
			off_period = data->signal[i+1]*period;
		else {
			if (data->signal[0] < 50000)
				off_period = data->signal[i+1]*(period+2);
			else
				off_period = data->signal[i+1]*(period+1);
		}

#if defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE) || defined(CONFIG_MACH_TAB3)
		off_period = data->signal[i+1]*(period+1);
#endif

		if (off_period <= 30000) {
			if (off_period > 1000) {
				__udelay(off_period % 1000);
				mdelay(off_period/1000);
			} else
				__udelay(off_period);
		} else {
			local_irq_enable();
			__udelay(off_period % 1000);
			mdelay(off_period/1000);
			local_irq_disable();
		}
	}
	gpio_direction_output(data->gpio, 1);
	__udelay(on);
	gpio_direction_output(data->gpio, 0);
	__udelay(off);

	local_irq_enable();
	pr_info("%s end!\n", __func__);
	exynos_cpufreq_lock_free(DVFS_LOCK_ID_IR_LED);
	exynos_cpufreq_upper_limit_free(DVFS_LOCK_ID_IR_LED);

	if (data->pwr_en != -1)
		gpio_direction_output(data->pwr_en, 0);

	if ((data->pwr_en == -1) && (regulator_status == 1)) {
		regulator_force_disable(regulator);
		regulator_put(regulator);

		regulator_status = -1;
	}
out: ;
}

static void ir_remocon_send_test(struct ir_remocon_data *data)
{
	unsigned int period = 0;
	int i;
	period = MICRO_SEC / data->signal[0];

	if (data->pwr_en == -1) {
		regulator = regulator_get(NULL, "vled_3.3v");
		if (IS_ERR(regulator))
			goto out;

		regulator_enable(regulator);
		regulator_status = 1;
	}

	if (data->pwr_en != -1)
		gpio_direction_output(data->pwr_en, 1);

	local_irq_disable();
	for (i = 1; i < MAX_SIZE; i++) {
		if (data->signal[i] == 0)
			break;
		else if (data->signal[i] == 10) {
			gpio_direction_output(data->gpio, 1);
			__udelay(period);
		} else if (data->signal[i] == 5) {
			gpio_direction_output(data->gpio, 0);
			__udelay(period);
		}
	}
	local_irq_enable();

	if (data->pwr_en != -1)
		gpio_direction_output(data->pwr_en, 0);

	if ((data->pwr_en == -1) && (regulator_status == 1)) {
		regulator_force_disable(regulator);
		regulator_put(regulator);

		regulator_status = -1;
	}
out: ;
}

static void ir_remocon_work(struct work_struct *work)
{
	struct ir_remocon_data *data = container_of(work,
						    struct ir_remocon_data,
						    work);

	ir_remocon_send(data);
}

static void ir_remocon_work_test(struct work_struct *work)
{
	struct ir_remocon_data *data = container_of(work,
						    struct ir_remocon_data,
						    work);

	ir_remocon_send_test(data);
}

static ssize_t remocon_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t size)
{
	struct ir_remocon_data *data = dev_get_drvdata(dev);
	int i;
	unsigned int _data;

	for (i = 0; i < MAX_SIZE; i++) {
		if (sscanf(buf++, "%u", &_data) == 1) {
			data->signal[i] = _data;
			if (data->signal[i] == 0)
				break;
#if 0
			pr_info("%d = %d,", i, data->signal[i]);
#endif
			while (_data > 0) {
				buf++;
				_data /= 10;
			}
		} else {
			data->signal[i] = 0;
			break;
		}
	}

	if (!work_pending(&data->work))
		schedule_work(&data->work);

	return size;
}

static ssize_t remocon_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct ir_remocon_data *data = dev_get_drvdata(dev);
	int i;
	char *bufp = buf;

	for (i = 0; i < MAX_SIZE; i++) {
		if (data->signal[i] == 0)
			break;
		else {
			bufp += sprintf(bufp, "%u,", data->signal[i]);
			pr_info("%u,", data->signal[i]);
		}
	}
	return strlen(buf);
}

static DEVICE_ATTR(ir_send, 0664, remocon_show, remocon_store);
static DEVICE_ATTR(ir_send_test, 0664, NULL, remocon_store);

static ssize_t check_ir_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	int _data = 1;
	return sprintf(buf, "%d\n", _data);
}

static DEVICE_ATTR(check_ir, 0664, check_ir_show, NULL);

static int __devinit ir_remocon_probe(struct platform_device *pdev)
{
	struct ir_remocon_data *data = pdev->dev.platform_data;
	struct ir_remocon_data *data1 = pdev->dev.platform_data;
	struct device *ir_remocon_dev;
	struct device *ir_remocon_dev_test;
	int error;

	pr_info("********* Ir_LED : %s start!\n", __func__);

	data = kzalloc(sizeof(struct ir_remocon_data), GFP_KERNEL);
	if (NULL == data) {
		pr_err("Failed to data allocate %s\n", __func__);
		error = -ENOMEM;
		goto err_free_mem;
	}

	data1 = kzalloc(sizeof(struct ir_remocon_data), GFP_KERNEL);
	if (NULL == data1) {
		pr_err("Failed to data1 allocate %s\n", __func__);
		error = -ENOMEM;
		goto err_free_mem;
	}
#if defined(CONFIG_MACH_P2) || defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_TAB3)
	data->gpio = GPIO_IRDA_CONTROL;
	data->pwr_en = -1;
#endif
#if defined(CONFIG_MACH_P8LTE) || defined(CONFIG_MACH_P8)
	data->gpio = GPIO_IRDA_nINT;
	data->pwr_en = GPIO_IRDA_EN;
#endif

	mutex_init(&data->mutex);
	INIT_WORK(&data->work, ir_remocon_work);
	INIT_WORK(&data1->work, ir_remocon_work_test);

	error = gpio_init(data);
	if (error)
		pr_err("Failed to request gpio");

	ir_remocon_dev = device_create(sec_class, NULL, 0, data, "sec_ir");
	ir_remocon_dev_test =
	    device_create(sec_class, NULL, 0, data1, "sec_ir_test");

	if (IS_ERR(ir_remocon_dev))
		pr_err("Failed to create ir_remocon_dev device\n");

	if (IS_ERR(ir_remocon_dev_test))
		pr_err("Failed to create ir_remocon_dev_test device\n");

	if (device_create_file(ir_remocon_dev, &dev_attr_ir_send) < 0)
		pr_err("Failed to create device file(%s)!\n",
		       dev_attr_ir_send.attr.name);

	if (device_create_file(ir_remocon_dev_test, &dev_attr_ir_send_test) < 0)
		pr_err("Failed to create device file(%s)!\n",
		       dev_attr_ir_send_test.attr.name);

	if (device_create_file(ir_remocon_dev, &dev_attr_check_ir) < 0)
		pr_err("Failed to create device file(%s)!\n",
		       dev_attr_check_ir.attr.name);

	return 0;

 err_free_mem:
	kfree(data);
	kfree(data1);
	return error;

}

static int __devexit ir_remocon_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
static int ir_remocon_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int ir_remocon_resume(struct platform_device *pdev)
{
	return 0;
}
#endif

static struct platform_driver ir_remocon_device_driver = {
	.probe = ir_remocon_probe,
	.remove = __devexit_p(ir_remocon_remove),
#ifdef CONFIG_PM
	.suspend = ir_remocon_suspend,
	.resume = ir_remocon_resume,
#endif
	.driver = {
		   .name = "ir_rc",
		   .owner = THIS_MODULE,
		   }
};

static int __init ir_remocon_init(void)
{
#if defined(CONFIG_IR_REMOCON_GPIO_EUR) || defined(CONFIG_IR_REMOCON_GPIO_TMO)
	if (system_rev >= 11)
		return 0;
#endif
	return platform_driver_register(&ir_remocon_device_driver);
}

static void __exit ir_remocon_exit(void)
{
	platform_driver_unregister(&ir_remocon_device_driver);
}

module_init(ir_remocon_init);
module_exit(ir_remocon_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SEC IR remote controller");
