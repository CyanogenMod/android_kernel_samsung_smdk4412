/*
 * driver/barcode_emul ice4 fpga driver
 *
 * Copyright (C) 2012 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/err.h>

#if defined(CONFIG_TARGET_TAB3_LTE8)
#include "ice4_slim8_fpga.h"
#elif defined(CONFIG_MACH_GC2PD)
#include "ice4_gc2pd_fpga.h"
#endif

#if defined(TEST_DEBUG)
#define pr_barcode	pr_emerg
#else
#define pr_barcode	pr_info
#endif

#define LOOP_BACK	48
#define TEST_CODE1	49
#define TEST_CODE2	50
#define FW_VER_ADDR	0x80
#define BEAM_STATUS_ADDR	0xFE
#define BARCODE_EMUL_MAX_FW_PATH	255
#define BARCODE_EMUL_FW_FILENAME		"i2c_top_bitmap.bin"

#define BARCODE_I2C_ADDR	0x6C

#if defined(CONFIG_IR_REMOCON_FPGA)
#define IRDA_I2C_ADDR		0x50
#define IRDA_TEST_CODE_SIZE	141
#define IRDA_TEST_CODE_ADDR	0x00
#define MAX_SIZE		2048
#define READ_LENGTH		8
#endif

struct ice4_fpga_data {
	struct i2c_client		*client;
#if defined(CONFIG_IR_REMOCON_FPGA)
	struct mutex			mutex;
	struct {
		unsigned char addr;
		unsigned char data[MAX_SIZE];
	} i2c_block_transfer;
	int count;
	int dev_id;
	int ir_freq;
	int ir_sum;
#endif
};

static int fw_loaded = 0;
#if defined(CONFIG_IR_REMOCON_FPGA)
static int ack_number;
static int count_number;
#endif
/*
static struct workqueue_struct *barcode_init_wq;
static void barcode_init_workfunc(struct work_struct *work);
static DECLARE_WORK(barcode_init_work, barcode_init_workfunc);
*/
/*
 * Send barcode emulator firmware data through spi communication
 */
static int barcode_send_firmware_data(unsigned char *data)
{
	unsigned int i,j;
	unsigned char spibit;

	i=0;
	while (i < CONFIGURATION_SIZE) {
		j=0;
		spibit = data[i];
		while (j < 8) {
			gpio_set_value(GPIO_IRDA_SCL, GPIO_LEVEL_LOW);

			if (spibit & 0x80) {
				gpio_set_value(GPIO_IRDA_SDA,GPIO_LEVEL_HIGH);
			} else {
				gpio_set_value(GPIO_IRDA_SDA,GPIO_LEVEL_LOW);
			}
			j = j+1;
			gpio_set_value(GPIO_IRDA_SCL, GPIO_LEVEL_HIGH);
			spibit = spibit<<1;
		}
		i = i+1;
	}

	i = 0;
	while (i < 200) {
		gpio_set_value(GPIO_IRDA_SCL, GPIO_LEVEL_LOW);
		i = i+1;
		gpio_set_value(GPIO_IRDA_SCL, GPIO_LEVEL_HIGH);
	}
	return 0;
}

static int check_fpga_cdone(void)
{
    /* Device in Operation when CDONE='1'; Device Failed when CDONE='0'. */
	if (gpio_get_value(GPIO_FPGA_CDONE) != 1) {
		pr_barcode("CDONE_FAIL %d\n",gpio_get_value(GPIO_FPGA_CDONE));
		return 0;
	} else {
		pr_barcode("CDONE_SUCCESS\n");
		return 1;
	}
}


static int barcode_fpga_fimrware_update_start(unsigned char *data)
{
	pr_barcode("%s\n", __func__);

	gpio_request_one(GPIO_FPGA_CDONE, GPIOF_IN, "FPGA_CDONE");
	gpio_request_one(GPIO_IRDA_SCL, GPIOF_OUT_INIT_LOW, "IRDA_SCL");
	gpio_request_one(GPIO_IRDA_SDA, GPIOF_OUT_INIT_LOW, "IRDA_SDA");
	gpio_request_one(GPIO_IRDA_WAKE, GPIOF_OUT_INIT_LOW, "IRDA_WAKE");
	gpio_request_one(GPIO_FPGA_CRESET, GPIOF_OUT_INIT_HIGH, "FPGA_CRESET");

	gpio_set_value(GPIO_FPGA_CRESET, GPIO_LEVEL_LOW);
	udelay(30);

	gpio_set_value(GPIO_FPGA_CRESET, GPIO_LEVEL_HIGH);
	udelay(1000);

	barcode_send_firmware_data(data);
	udelay(50);

	if (check_fpga_cdone()) {
		udelay(5);
		gpio_set_value(GPIO_IRDA_WAKE, GPIO_LEVEL_HIGH);
		pr_barcode("barcode firmware update success\n");
		fw_loaded = 1;
	} else {
		pr_barcode("barcode firmware update fail\n");
		fw_loaded = 0;
	}
	gpio_free(GPIO_IRDA_SDA);
	gpio_free(GPIO_IRDA_SCL);
	return 0;
}

void barcode_fpga_firmware_update(void)
{
	barcode_fpga_fimrware_update_start(spiword);
}

static ssize_t barcode_emul_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int ret;
	struct ice4_fpga_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	pr_barcode("%s start\n", __func__);

	if (gpio_get_value(GPIO_FPGA_CDONE) != 1) {
		pr_err("%s: cdone fail !!\n", __func__);
		return 1;
	}

	client->addr = BARCODE_I2C_ADDR;
	ret = i2c_master_send(client, buf, size);
	if (ret < 0) {
		pr_err("%s: i2c err1 %d\n", __func__, ret);
		ret = i2c_master_send(client, buf, size);
		if (ret < 0)
			pr_err("%s: i2c err2 %d\n", __func__, ret);
	}

	pr_barcode("%s complete\n", __func__);
	return size;
}

static ssize_t barcode_emul_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return strlen(buf);
}
static DEVICE_ATTR(barcode_send, 0664, barcode_emul_show, barcode_emul_store);

static ssize_t barcode_emul_fw_update_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct file *fp = NULL;
	long fsize = 0, nread = 0;
	const u8 *buff = 0;
	char fw_path[BARCODE_EMUL_MAX_FW_PATH+1];
	mm_segment_t old_fs = get_fs();

	pr_barcode("%s\n", __func__);

	old_fs = get_fs();
	set_fs(get_ds());

	snprintf(fw_path, BARCODE_EMUL_MAX_FW_PATH,
		"/sdcard/%s", BARCODE_EMUL_FW_FILENAME);

	fp = filp_open(fw_path, O_RDONLY, 0);
	if (!fp)
		goto err_fp;

	if (IS_ERR(fp)) {
		pr_err("file %s open error:%d\n",
			fw_path, (s32)fp);
		goto err_open;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	pr_barcode("barcode firmware size: %ld\n", fsize);

	buff = kzalloc((size_t)fsize, GFP_KERNEL);
	if (!buff) {
		pr_err("fail to alloc buffer for fw\n");
		goto err_alloc;
	}

	nread = vfs_read(fp, (char __user *)buff, fsize, &fp->f_pos);
	if (nread != fsize) {
		pr_err("fail to read file %s (nread = %ld)\n",
			fw_path, nread);
		goto err_fw_size;
	}

	barcode_fpga_fimrware_update_start((unsigned char *)buff);

err_fw_size:
	kfree(buff);

err_alloc:
	filp_close(fp, NULL);

err_open:
	set_fs(old_fs);

err_fp:
	return size;
}

static ssize_t barcode_emul_fw_update_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return strlen(buf);
}
static DEVICE_ATTR(barcode_fw_update, 0664, barcode_emul_fw_update_show, barcode_emul_fw_update_store);

static int barcode_emul_read(struct i2c_client *client, u16 addr, u8 length, u8 *value)
{

	struct i2c_msg msg[2];
	int ret;

	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 1;
	msg[0].buf   = (u8 *) &addr;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = length;
	msg[1].buf   = (u8 *) value;

	ret = i2c_transfer(client->adapter, msg, 2);
	if  (ret == 2) {
		return 0;
	} else {
		pr_barcode("%s: err1 %d\n", __func__, ret);
		return -EIO;
	}
}

static ssize_t barcode_emul_test_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int ret, i;
	struct ice4_fpga_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	struct {
		unsigned char addr;
		unsigned char data[20];
	} i2c_block_transfer;
	unsigned char barcode_data[14] = {0xFF, 0xAC, 0xDB, 0x36, 0x42, 0x85, 0x0A, 0xA8, 0xD1, 0xA3, 0x46, 0xC5, 0xDA, 0xFF};

	client->addr = BARCODE_I2C_ADDR;
	if (gpio_get_value(GPIO_FPGA_CDONE) != 1) {
		pr_err("%s: cdone fail !!\n", __func__);
		return 1;
	}

	if (buf[0] == LOOP_BACK) {
		for (i = 0; i < size; i++)
			i2c_block_transfer.data[i] = *buf++;

		i2c_block_transfer.addr = 0x01;
		pr_barcode("%s: write addr: %d, value: %d\n", __func__,
			i2c_block_transfer.addr, i2c_block_transfer.data[0]);

		ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 2);
		if (ret < 0) {
			pr_barcode("%s: err1 %d\n", __func__, ret);
			ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 2);
			if (ret < 0)
				pr_barcode("%s: err2 %d\n", __func__, ret);
		}
	} else if (buf[0] == TEST_CODE1) {
		unsigned char BSR_data[6] =\
			{0xC8, 0x00, 0x32, 0x01, 0x00, 0x32};

		pr_barcode("barcode test code start\n");

		/* send NH */
		i2c_block_transfer.addr = 0x80;
		i2c_block_transfer.data[0] = 0x05;
		ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 2);
		if (ret < 0) {
			pr_err("%s: err1 %d\n", __func__, ret);
			ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 2);
			if (ret < 0)
				pr_err("%s: err2 %d\n", __func__, ret);
		}

		/* setup BSR data */
		for (i = 0; i < 6; i++)
			i2c_block_transfer.data[i+1] = BSR_data[i];

		/* send BSR1 => NS 1= 200, ISD 1 = 100us, IPD 1 = 200us, BL 1=14, BW 1=4MHZ */
		i2c_block_transfer.addr = 0x81;
		i2c_block_transfer.data[0] = 0x00;

		ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 8);
		if (ret < 0) {
			pr_err("%s: err1 %d\n", __func__, ret);
			ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 8);
			if (ret < 0)
				pr_err("%s: err2 %d\n", __func__, ret);
		}

		/* send BSR2 => NS 2= 200, ISD 2 = 100us, IPD 2 = 200us, BL 2=14, BW 2=2MHZ*/
		i2c_block_transfer.addr = 0x88;
		i2c_block_transfer.data[0] = 0x01;

		ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 8);
		if (ret < 0) {
			pr_err("%s: err1 %d\n", __func__, ret);
			ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 8);
			if (ret < 0)
				pr_err("%s: err2 %d\n", __func__, ret);
		}

		/* send BSR3 => NS 3= 200, ISD 3 = 100us, IPD 3 = 200us, BL 3=14, BW 3=1MHZ*/
		i2c_block_transfer.addr = 0x8F;
		i2c_block_transfer.data[0] = 0x02;

		ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 8);
		if (ret < 0) {
			pr_err("%s: err1 %d\n", __func__, ret);
			ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 8);
			if (ret < 0)
				pr_err("%s: err2 %d\n", __func__, ret);
		}

		/* send BSR4 => NS 4= 200, ISD 4 = 100us, IPD 4 = 200us, BL 4=14, BW 4=500KHZ*/
		i2c_block_transfer.addr = 0x96;
		i2c_block_transfer.data[0] = 0x04;

		ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 8);
		if (ret < 0) {
			pr_err("%s: err1 %d\n", __func__, ret);
			ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 8);
			if (ret < 0)
				pr_err("%s: err2 %d\n", __func__, ret);
		}

		/* send BSR5 => NS 5= 200, ISD 5 = 100us, IPD 5 = 200us, BL 5=14, BW 5=250KHZ*/
		i2c_block_transfer.addr = 0x9D;
		i2c_block_transfer.data[0] = 0x08;

		ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 8);
		if (ret < 0) {
			pr_err("%s: err1 %d\n", __func__, ret);
			ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 8);
			if (ret < 0)
				pr_err("%s: err2 %d\n", __func__, ret);
		}

		/* send barcode data */
		i2c_block_transfer.addr = 0x00;
		for (i = 0; i < 14; i++)
			i2c_block_transfer.data[i] = barcode_data[i];

		ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 15);
		if (ret < 0) {
			pr_err("%s: err1 %d\n", __func__, ret);
			ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 15);
			if (ret < 0)
				pr_err("%s: err2 %d\n", __func__, ret);
		}

		/* send START */
		i2c_block_transfer.addr = 0xFF;
		i2c_block_transfer.data[0] = 0x0E;
		ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 2);
		if (ret < 0) {
			pr_err("%s: err1 %d\n", __func__, ret);
			ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 2);
			if (ret < 0)
				pr_err("%s: err2 %d\n", __func__, ret);
		}
	} else if (buf[0] == TEST_CODE2) {
		pr_barcode("barcode test code stop\n");

		i2c_block_transfer.addr = 0xFF;
		i2c_block_transfer.data[0] = 0x00;
		ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 2);
		if (ret < 0) {
			pr_err("%s: err1 %d\n", __func__, ret);
			ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer, 2);
			if (ret < 0)
				pr_err("%s: err2 %d\n", __func__, ret);
		}
	}
	return size;
}

static ssize_t barcode_emul_test_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return strlen(buf);
}
static DEVICE_ATTR(barcode_test_send, 0664, barcode_emul_test_show, barcode_emul_test_store);

static ssize_t barcode_ver_check_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct ice4_fpga_data *data = dev_get_drvdata(dev);
	u8 fw_ver;

	barcode_emul_read(data->client, FW_VER_ADDR, 1, &fw_ver);
	fw_ver = (fw_ver >> 5) & 0x7;

	return sprintf(buf, "%d\n", fw_ver+14);

}
static DEVICE_ATTR(barcode_ver_check, 0664, barcode_ver_check_show, NULL);

static ssize_t barcode_led_status_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct ice4_fpga_data *data = dev_get_drvdata(dev);
	u8 status;

	barcode_emul_read(data->client, BEAM_STATUS_ADDR, 1, &status);
	status = status & 0x1;

	return sprintf(buf, "%d\n", status);
}
static DEVICE_ATTR(barcode_led_status, 0664, barcode_led_status_show, NULL);

#if defined(CONFIG_IR_REMOCON_FPGA)
/* When IR test does not work, we need to check some gpios' status */
static void print_fpga_gpio_status(void)
{
	if (!fw_loaded)
		printk(KERN_ERR "%s : firmware is not loaded\n", __func__);
	else
		printk(KERN_ERR "%s : firmware is loaded\n", __func__);

	printk(KERN_ERR "%s : CDONE    : %d\n", __func__, gpio_get_value(GPIO_FPGA_CDONE));
	printk(KERN_ERR "%s : WAKE    : %d\n", __func__, gpio_get_value(GPIO_IRDA_WAKE));
	printk(KERN_ERR "%s : CRESET : %d\n", __func__, gpio_get_value(GPIO_FPGA_CRESET));
}
/* sysfs node ir_send */
static void ir_remocon_work(struct ice4_fpga_data *ir_data, int count)
{

	struct ice4_fpga_data *data = ir_data;
	struct i2c_client *client = data->client;

	int buf_size = count+2;
	int ret;
	int sleep_timing;
	int end_data;
	int emission_time;
	int ack_pin_onoff;

#if defined(DEBUG)
	u8 *temp;
	int i;
#endif

	data->i2c_block_transfer.addr = 0x00;

	data->i2c_block_transfer.data[0] = count >> 8;
	data->i2c_block_transfer.data[1] = count & 0xff;

#if defined(DEBUG)
	temp = kzalloc(sizeof(u8)*(buf_size+20), GFP_KERNEL);
	if (NULL == temp)
		pr_err("Failed to data allocate %s\n", __func__);
#endif

	if (count_number >= 100)
		count_number = 0;

	count_number++;

	printk(KERN_INFO "%s: total buf_size: %d\n", __func__, buf_size);

	mutex_lock(&data->mutex);

	client->addr = IRDA_I2C_ADDR;

	buf_size++;
	ret = i2c_master_send(client,
		(unsigned char *) &(data->i2c_block_transfer), buf_size);
	if (ret < 0) {
		dev_err(&client->dev, "%s: err1 %d\n", __func__, ret);
		ret = i2c_master_send(client,
		(unsigned char *) &(data->i2c_block_transfer), buf_size);
		if (ret < 0) {
			dev_err(&client->dev, "%s: err2 %d\n", __func__, ret);
			print_fpga_gpio_status();
		}
	}

#if defined(DEBUG)
	barcode_emul_read(client, IRDA_TEST_CODE_ADDR, buf_size, temp);

	for (i = 0 ; i < buf_size; i++)
		pr_err("%4d SEND %5d  RECV %5d\n", i,
			data->i2c_block_transfer.data[i], temp[i]);
#endif

	mdelay(10);

	ack_pin_onoff = 0;

	if (gpio_get_value(GPIO_IRDA_IRQ)) {
		printk(KERN_INFO "%s : %d Checksum NG!\n",
			__func__, count_number);
		ack_pin_onoff = 1;
	} else {
		printk(KERN_INFO "%s : %d Checksum OK!\n",
			__func__, count_number);
		ack_pin_onoff = 2;
	}
	ack_number = ack_pin_onoff;

	mutex_unlock(&data->mutex);

	data->count = 2;

	end_data = data->i2c_block_transfer.data[count-2] << 8
		| data->i2c_block_transfer.data[count-1];
	emission_time = \
		(1000 * (data->ir_sum - end_data) / (data->ir_freq)) + 10;
	sleep_timing = emission_time - 130;
	if (sleep_timing > 0)
		msleep(sleep_timing);

	emission_time = \
		(1000 * (data->ir_sum) / (data->ir_freq)) + 50;
	if (emission_time > 0)
		msleep(emission_time);
		printk(KERN_INFO "%s: emission_time = %d\n",
					__func__, emission_time);

	if (gpio_get_value(GPIO_IRDA_IRQ)) {
		printk(KERN_INFO "%s : %d Sending IR OK!\n",
				__func__, count_number);
		ack_pin_onoff = 4;
	} else {
		printk(KERN_INFO "%s : %d Sending IR NG!\n",
				__func__, count_number);
		ack_pin_onoff = 2;
	}

	ack_number += ack_pin_onoff;

	data->ir_freq = 0;
	data->ir_sum = 0;
}

static ssize_t remocon_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct ice4_fpga_data *data = dev_get_drvdata(dev);
	unsigned int _data;
	int count, i;

	printk(KERN_INFO "%s : ir_send called\n", __func__);
	if (!fw_loaded) {
		printk(KERN_ERR "%s : firmware is not loaded\n", __func__);
		return 1;
	}

	for (i = 0; i < MAX_SIZE; i++) {
		if (sscanf(buf++, "%u", &_data) == 1) {
			if (_data == 0 || buf == '\0')
				break;

			if (data->count == 2) {
				data->ir_freq = _data;
				data->i2c_block_transfer.data[2] = _data >> 16;
				data->i2c_block_transfer.data[3]
							= (_data >> 8) & 0xFF;
				data->i2c_block_transfer.data[4] = _data & 0xFF;

				data->count += 3;
			} else {
				data->ir_sum += _data;
				count = data->count;
				data->i2c_block_transfer.data[count]
								= _data >> 8;
				data->i2c_block_transfer.data[count+1]
								= _data & 0xFF;
				data->count += 2;
			}

			while (_data > 0) {
				buf++;
				_data /= 10;
			}
		} else {
			break;
		}
	}

	ir_remocon_work(data, data->count);
	return size;
}

static ssize_t remocon_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct ice4_fpga_data *data = dev_get_drvdata(dev);
	int i;
	char *bufp = buf;

	for (i = 5; i < MAX_SIZE - 1; i++) {
		if (data->i2c_block_transfer.data[i] == 0
			&& data->i2c_block_transfer.data[i+1] == 0)
			break;
		else
			bufp += sprintf(bufp, "%u,",
					data->i2c_block_transfer.data[i]);
	}
	return strlen(buf);
}

static DEVICE_ATTR(ir_send, 0664, remocon_show, remocon_store);
/* sysfs node ir_send_result */
static ssize_t remocon_ack(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	printk(KERN_INFO "%s : ack_number = %d\n", __func__, ack_number);

	if (ack_number == 6)
		return sprintf(buf, "1\n");
	else
		return sprintf(buf, "0\n");
}

static DEVICE_ATTR(ir_send_result, 0664, remocon_ack, NULL);

static int irda_read_device_info(struct ice4_fpga_data *ir_data)
{
	struct ice4_fpga_data *data = ir_data;
	struct i2c_client *client = data->client;
	u8 buf_ir_test[8];
	int ret;

	printk(KERN_INFO"%s called\n", __func__);

	client->addr = IRDA_I2C_ADDR;
	ret = i2c_master_recv(client, buf_ir_test, READ_LENGTH);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	printk(KERN_INFO "%s: buf_ir dev_id: 0x%02x, 0x%02x\n", __func__,
			buf_ir_test[2], buf_ir_test[3]);
	ret = data->dev_id = (buf_ir_test[2] << 8 | buf_ir_test[3]);

	return ret;
}

	/* sysfs node check_ir */
static ssize_t check_ir_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct ice4_fpga_data *data = dev_get_drvdata(dev);
	int ret;

	ret = irda_read_device_info(data);
	return snprintf(buf, 4, "%d\n", ret);
}

static DEVICE_ATTR(check_ir, 0664, check_ir_show, NULL);
/* sysfs node irda_test */
static ssize_t irda_test_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int ret, i;
	struct ice4_fpga_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	struct {
			unsigned char addr;
			unsigned char data[IRDA_TEST_CODE_SIZE];
	} i2c_block_transfer;
	unsigned char BSR_data[IRDA_TEST_CODE_SIZE] = {
		0x00, 0x8D, 0x00, 0x96, 0x00, 0x01, 0x50, 0x00,
		0xA8, 0x00, 0x15, 0x00, 0x15, 0x00, 0x15, 0x00,
		0x15, 0x00, 0x15, 0x00, 0x3F, 0x00, 0x15, 0x00,
		0x15, 0x00, 0x15, 0x00, 0x15, 0x00, 0x15, 0x00,
		0x15, 0x00, 0x15, 0x00, 0x15, 0x00, 0x15, 0x00,
		0x15, 0x00, 0x15, 0x00, 0x3F, 0x00, 0x15, 0x00,
		0x3F, 0x00, 0x15, 0x00, 0x15, 0x00, 0x15, 0x00,
		0x3F, 0x00, 0x15, 0x00, 0x3F, 0x00, 0x15, 0x00,
		0x3F, 0x00, 0x15, 0x00, 0x3F, 0x00, 0x15, 0x00,
		0x3F, 0x00, 0x15, 0x00, 0x15, 0x00, 0x15, 0x00,
		0x3F, 0x00, 0x15, 0x00, 0x15, 0x00, 0x15, 0x00,
		0x15, 0x00, 0x15, 0x00, 0x15, 0x00, 0x15, 0x00,
		0x15, 0x00, 0x15, 0x00, 0x15, 0x00, 0x15, 0x00,
		0x15, 0x00, 0x15, 0x00, 0x3F, 0x00, 0x15, 0x00,
		0x15, 0x00, 0x15, 0x00, 0x3F, 0x00, 0x15, 0x00,
		0x3F, 0x00, 0x15, 0x00, 0x3F, 0x00, 0x15, 0x00,
		0x3F, 0x00, 0x15, 0x00, 0x3F, 0x00, 0x15, 0x00,
		0x3F, 0x00, 0x15, 0x00, 0x3F
	};

	if (gpio_get_value(GPIO_FPGA_CDONE) != 1) {
		pr_err("%s: cdone fail !!\n", __func__);
		return 1;
	}
	pr_barcode("IRDA test code start\n");
	if (!fw_loaded) {
		printk(KERN_ERR "%s : firmware is not loaded\n", __func__);
		return 1;
	}

	/* change address for IRDA */
	client->addr = IRDA_I2C_ADDR;

	/* make data for sending */
	for (i = 0; i < IRDA_TEST_CODE_SIZE; i++)
		i2c_block_transfer.data[i] = BSR_data[i];

	/* sending data by I2C */
	i2c_block_transfer.addr = IRDA_TEST_CODE_ADDR;
	ret = i2c_master_send(client, (unsigned char *) &i2c_block_transfer,
			IRDA_TEST_CODE_SIZE);
	if (ret < 0) {
		pr_err("%s: err1 %d\n", __func__, ret);
		ret = i2c_master_send(client,
		(unsigned char *) &i2c_block_transfer, IRDA_TEST_CODE_SIZE);
		if (ret < 0)
			pr_err("%s: err2 %d\n", __func__, ret);
	}

	return size;
}

static ssize_t irda_test_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return strlen(buf);
}

static DEVICE_ATTR(irda_test, 0664, irda_test_show, irda_test_store);
#endif

static int __devinit barcode_emul_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct ice4_fpga_data *data;
	struct device *barcode_emul_dev;
	int error;

	pr_barcode("%s probe!\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	data = kzalloc(sizeof(struct ice4_fpga_data), GFP_KERNEL);
	if (NULL == data) {
		pr_err("Failed to data allocate %s\n", __func__);
		goto alloc_fail;
	}

	if (check_fpga_cdone()) {
		fw_loaded = 1;
		pr_barcode("FPGA FW is loaded!\n");
	} else {
		fw_loaded = 0;
		pr_barcode("FPGA FW is NOT loaded!\n");
	}

	data->client = client;
#if defined(CONFIG_IR_REMOCON_FPGA)
	mutex_init(&data->mutex);
	data->count = 2;
#endif
	i2c_set_clientdata(client, data);
#if defined(CONFIG_BARCODE_EMUL_FPGA_ICE4)
	barcode_emul_dev = device_create(sec_class, NULL, 0, data, "sec_barcode_emul");

	if (IS_ERR(barcode_emul_dev))
		pr_err("Failed to create barcode_emul_dev device\n");

	if (device_create_file(barcode_emul_dev, &dev_attr_barcode_send) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_barcode_send.attr.name);
	if (device_create_file(barcode_emul_dev, &dev_attr_barcode_test_send) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_barcode_test_send.attr.name);
	if (device_create_file(barcode_emul_dev, &dev_attr_barcode_fw_update) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_barcode_fw_update.attr.name);
	if (device_create_file(barcode_emul_dev, &dev_attr_barcode_ver_check) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_barcode_ver_check.attr.name);
	if (device_create_file(barcode_emul_dev, &dev_attr_barcode_led_status) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_barcode_led_status.attr.name);
#endif
#if defined(CONFIG_IR_REMOCON_FPGA)
	barcode_emul_dev = device_create(sec_class, NULL, 0, data, "sec_ir");
	if (IS_ERR(barcode_emul_dev))
		pr_err("Failed to create barcode_emul_dev device in sec_ir\n");

	if (device_create_file(barcode_emul_dev, &dev_attr_ir_send) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_ir_send.attr.name);
	if (device_create_file(barcode_emul_dev, &dev_attr_ir_send_result) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_ir_send_result.attr.name);
	if (device_create_file(barcode_emul_dev, &dev_attr_check_ir) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_check_ir.attr.name);
	if (device_create_file(barcode_emul_dev, &dev_attr_irda_test) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_irda_test.attr.name);
#endif
	pr_err("probe complete %s\n", __func__);

alloc_fail:
	return 0;
}

static int __devexit barcode_emul_remove(struct i2c_client *client)
{
	struct ice4_fpga_data *data = i2c_get_clientdata(client);

	i2c_set_clientdata(client, NULL);
	kfree(data);
	return 0;
}

#ifdef CONFIG_PM
static int barcode_emul_suspend(struct device *dev)
{
	gpio_set_value(GPIO_IRDA_WAKE, GPIO_LEVEL_LOW);
	return 0;
}

static int barcode_emul_resume(struct device *dev)
{
	gpio_set_value(GPIO_IRDA_WAKE, GPIO_LEVEL_HIGH);
	return 0;
}

static const struct dev_pm_ops barcode_emul_pm_ops = {
	.suspend	= barcode_emul_suspend,
	.resume		= barcode_emul_resume,
};
#endif

static const struct i2c_device_id ice4_id[] = {
	{"ice4", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ice4_id);

static struct i2c_driver ice4_i2c_driver = {
	.driver = {
		.name	= "ice4",
#ifdef CONFIG_PM
		.pm	= &barcode_emul_pm_ops,
#endif
	},
	.probe = barcode_emul_probe,
	.remove = __devexit_p(barcode_emul_remove),
	.id_table = ice4_id,
};
/*
static void barcode_init_workfunc(struct work_struct *work)
{
	barcode_fpga_firmware_update();*/
/* because of permission */
#if 0
	i2c_add_driver(&ice4_i2c_driver);
#endif
//}

static int __init barcode_emul_init(void)
{
#if 0
	barcode_init_wq = create_singlethread_workqueue("barcode_init");
	if (!barcode_init_wq) {
		pr_err("fail to create wq\n");
		goto err_create_wq;
	}
	queue_work(barcode_init_wq, &barcode_init_work);
#endif
/* because of permission */
	i2c_add_driver(&ice4_i2c_driver);
err_create_wq:
	return 0;
}
late_initcall(barcode_emul_init);

static void __exit barcode_emul_exit(void)
{
	i2c_del_driver(&ice4_i2c_driver);
//	destroy_workqueue(barcode_init_wq);
}
module_exit(barcode_emul_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SEC Barcode emulator");
