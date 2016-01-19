/**
 * wimax_i2c.c
 *
 * EEPROM access functions
 */
#include "wimax_i2c.h"
#include "firmware.h"
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/vmalloc.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-u1.h>
#include <linux/slab.h>

#define WIMAX_EN        GPIO_WIMAX_EN
#define I2C_SEL         GPIO_WIMAX_I2C_CON
#define EEPROM_SCL      GPIO_CMC_SCL_18V
#define EEPROM_SDA      GPIO_CMC_SDA_18V


#define MAX_WIMAXBOOTIMAGE_SIZE		6000
#define MAX_BOOT_WRITE_LENGTH		128
#define WIMAX_EEPROM_ADDRLEN 2


struct boot_image_data g_wimax_image;

static void wimax_i2c_reset(void);

void dump_buffer(const char *desc, u_char *buffer, u_int len)
{
	char    print_buf[256] = {0};
	char    chr[8] = {0};
	int i;
	/* dump packets */
	u_char  *b = buffer;
	pr_debug("%s (%d) =>", desc, len);

	for (i = 0; i < len; i++) {
		sprintf(chr, " %02x", b[i]);
		strcat(print_buf, chr);
		if (((i + 1) != len) && (i % 16 == 15)) {
			pr_debug("%s", print_buf);
			memset(print_buf, 0x0, 256);
		}
	}
	pr_debug("%s", print_buf);
}
void eeprom_poweron(void)
{
	mutex_init(&g_wimax_image.lock);
	mutex_lock(&g_wimax_image.lock);

	s3c_gpio_cfgpin(EEPROM_SCL, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SCL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SDA, S3C_GPIO_PULL_NONE);

	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);

	/* SEL = 1 to switch i2c-eeprom path to AP */
	gpio_set_value(I2C_SEL, GPIO_LEVEL_HIGH);
	usleep_range(10000, 10000);


	/* power on */
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);

	msleep(100);

	wimax_i2c_reset();
	usleep_range(10000, 10000);
}

void eeprom_poweroff(void)
{
	/* power off */
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

	/* SEL = 0 to switch i2c-eeprom path to wimax */
	gpio_set_value(I2C_SEL, GPIO_LEVEL_LOW);

	msleep(100);

	mutex_unlock(&g_wimax_image.lock);
	mutex_destroy(&g_wimax_image.lock);
}

/* I2C functions for read/write EEPROM */
#ifdef DRIVER_BIT_BANG
void wimax_i2c_write_byte(char c, int len)
{
	int	i;
	char	level;

	/* 8 bits */
	for (i = 0; i < len; i++) {
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
		udelay(1);
		level = (c >> (len - i - 1)) & 0x1;
		s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
		gpio_set_value(EEPROM_SDA, level);
		udelay(2);
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
		udelay(2);
	}
}

char wimax_i2c_read_byte(int len)
{
	int	j;
	char	val = 0;

	/* 8 bits */
	for (j = 0; j < len; j++) {
		/* read data */
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
		s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(0));
		udelay(2);
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
		udelay(2);
		val |= (gpio_get_value(EEPROM_SDA) << (len - j - 1));
	}

	return val;
}

void wimax_i2c_init(void)
{
	const int	DEVICE_ADDRESS = 0x50;

	s3c_gpio_cfgpin(EEPROM_SCL, S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));

	/* initial */
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);
	udelay(3);

	/* start bit */
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);
	udelay(2);

	/* send 7 bits address */
	wimax_i2c_write_byte(DEVICE_ADDRESS, 7);
}

void wimax_i2c_deinit(void)
{
	/* stop */
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	udelay(1);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(1);
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);

	mdelay(10);

}

void wimax_i2c_reset(void)
{
	int	i;

	/* initial */
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);
	udelay(3);

	/* start bit */
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);
	udelay(3);

	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	udelay(1);
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);

	/* 9 clocks */
	for (i = 0; i < 9; i++) {
		udelay(2);
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
		udelay(2);
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	}
	udelay(2);

	/* start bit & stop bit */
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(2);
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);
	udelay(2);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	udelay(2);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(2);
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);
	udelay(2);
}

void wimax_i2c_ack(void)
{
	/* ack */
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	udelay(1);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(1);
}

void wimax_i2c_no_ack(void)
{
	/* no ack */
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	udelay(1);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(1);
}

int wimax_i2c_check_ack(void)
{
	char	val;

	/* ack */
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(0));
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(2);
	val = gpio_get_value(EEPROM_SDA);

	if (val == 1) {
		pr_debug("EEPROM ERROR: NO ACK!!");
		return -1;
	}

	return 0;
}

int wimax_i2c_write_cmd(void)
{
	/* write cmd */
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);	/* write cmd: 0 */
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(2);

	return wimax_i2c_check_ack();
}

int wimax_i2c_read_cmd(void)
{
	/* read cmd */
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);	/* read cmd: 1 */
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(2);

	return wimax_i2c_check_ack();
}

int wimax_i2c_eeprom_address(short addr)
{
	char	buf[2] = {0};

	buf[0] = addr & 0xff;
	buf[1] = (addr >> 8) & 0xff;

	/* send 2 bytes address */
	wimax_i2c_write_byte(buf[1], 8);
	if (wimax_i2c_check_ack())
		return -1;
	wimax_i2c_write_byte(buf[0], 8);
	return wimax_i2c_check_ack();
}

int wimax_i2c_write_buffer(char *data, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		wimax_i2c_write_byte(data[i], 8);	/* 1 byte data */
		if (wimax_i2c_check_ack())
			return -1;
	}
	return 0;
}

int wimax_i2c_write(u_short addr, u_char *data, int length)
{
	int ret = 0;

	/* write buffer */
	wimax_i2c_init();
	ret = wimax_i2c_write_cmd();
	ret |= wimax_i2c_eeprom_address(addr);
	ret |= wimax_i2c_write_buffer(data, length);
	wimax_i2c_deinit();
	return ret;
}

int wimax_i2c_read(u_short addr, u_char *data, int length)
{
	int	i = 0;
	int ret = 0;

	/* read buffer */
	wimax_i2c_init();
	ret = wimax_i2c_write_cmd();
	ret |= wimax_i2c_eeprom_address(addr);

	wimax_i2c_init();
	ret |= wimax_i2c_read_cmd();

	for (i = 0; i < length; i++) {
		*(data + i) = wimax_i2c_read_byte(8);	/* 1 byte data */
		if ((i + 1) == length)
			wimax_i2c_no_ack();	/* no ack end of data */
		else
			wimax_i2c_ack();
	}
	wimax_i2c_deinit();
	return ret;
}
#else

static struct i2c_client *pclient;

void wimax_i2c_reset(void)
{}

int wimax_i2c_write(u_short addr, u_char *data, int length)
{
	int rc;
	int len;
	char data_buffer[MAX_BOOT_WRITE_LENGTH + WIMAX_EEPROM_ADDRLEN];
	struct i2c_msg msg;
	data_buffer[0] = (unsigned char)((addr >> 8) & 0xff);
	data_buffer[1] = (unsigned char)(addr & 0xff);
	while (length) {
			len = (length > MAX_BOOT_WRITE_LENGTH) ?
				MAX_BOOT_WRITE_LENGTH : length ;
			memcpy(data_buffer+WIMAX_EEPROM_ADDRLEN, data, len);
			msg.addr = pclient->addr;
			msg.flags = 0;	/*write*/
			msg.len = (u16)length+WIMAX_EEPROM_ADDRLEN;
			msg.buf = data_buffer;
			rc = i2c_transfer(pclient->adapter, &msg, 1);
			if (rc < 0)
				return rc;
			length -= len;
			data += len;
		}
	return 0;
}

int wimax_i2c_read(u_short addr, u_char *data, int length)
{
	char data_buffer[MAX_BOOT_WRITE_LENGTH + WIMAX_EEPROM_ADDRLEN];
	struct i2c_msg msgs[2];
	data_buffer[0] = (unsigned char)((addr >> 8) & 0xff);
	data_buffer[1] = (unsigned char)(addr & 0xff);
	msgs[0].addr = pclient->addr;
	msgs[0].flags = 0;	/*write*/
	msgs[0].len = WIMAX_EEPROM_ADDRLEN;
	msgs[0].buf = data_buffer;
	msgs[1].addr = pclient->addr;
	msgs[1].flags = I2C_M_RD;	/*read*/
	msgs[1].len = length;
	msgs[1].buf = data;
	return i2c_transfer(pclient->adapter, msgs, 2);

}

static ssize_t eeprom_show(struct device *dev,
		struct device_attribute *attr, char *buf){
	pr_debug("Writing boot & revision to wimax eeprom");

	eeprom_write_boot();
	eeprom_write_rev();

	return 0;

}

static ssize_t eeprom_store(struct device *dev,
		struct device_attribute *attr,
		const char *buffer, size_t count) {

	if (strncmp(buffer, "wb00", 4) == 0) {
		pr_debug("Write EEPROM!!");
		eeprom_write_boot();
	} else if (strncmp(buffer, "rb00", 4) == 0) {
		pr_debug("Read Boot!!");
		eeprom_read_boot();
	} else if (strncmp(buffer, "re00", 4) == 0) {
		pr_debug("Read EEPROM!!");
		eeprom_read_all();
	} else if (strncmp(buffer, "ee00", 4) == 0) {
		pr_debug("Erase EEPROM!!");
		eeprom_erase_all();
	} else if (strncmp(buffer, "rcal", 4) == 0) {
		pr_debug("Check Cal!!");
		eeprom_check_cal();
	} else if (strncmp(buffer, "ecer", 4) == 0) {
		pr_debug("Erase Cert!!");
		eeprom_erase_cert();
	} else if (strncmp(buffer, "rcer", 4) == 0) {
		pr_debug("Check Cert!!");
		eeprom_check_cert();
	} else if (strncmp(buffer, "wrev", 4) == 0) {
		pr_debug("Write Rev!!");
		eeprom_write_rev();
	} else
		pr_debug("Wrong option");

	return count;
}

static DEVICE_ATTR(eeprom, 0664, eeprom_show, eeprom_store);

int wmxeeprom_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct class *wimax_class;
	struct device *dev_t;
	int err;

	pr_debug("[WMXEEPROM]: probe");

	pclient = client;
	wimax_class = class_create(THIS_MODULE, "wimax");
	if (IS_ERR(wimax_class))
		pr_err("%s: Class create failed", __func__);

	dev_t = device_create(wimax_class, NULL, 0, "%s", "cmc732");
	if (IS_ERR(dev_t))
		pr_err("%s: Device create failed", __func__);

	err = device_create_file(dev_t, &dev_attr_eeprom);
	if (err < 0)
		pr_err("%s: Device file create failed", __func__);

	return 0;
}

int wmxeeprom_remove(struct i2c_client *client)
{
	pr_debug("[WMXEEPROM]: remvove");

	return 0;
}

const struct i2c_device_id wmxeeprom_id[] = {
	{ "wmxeeprom", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, wmxeeprom_id);
static struct i2c_driver wmxeeprom_driver = {
	.probe		= wmxeeprom_probe,
	.remove		= wmxeeprom_remove,
	.id_table	= wmxeeprom_id,
	.driver = {
		.name   = "wmxeeprom",
	},
};
int wmxeeprom_init(void)
{
	return i2c_add_driver(&wmxeeprom_driver);
}

void wmxeeprom_exit(void)
{
	i2c_del_driver(&wmxeeprom_driver);
}
#endif

int load_wimax_boot(void)
{
	struct	file *fp;
	int	read_size = 0;

	fp = klib_fopen(WIMAX_BOOTIMAGE_PATH, O_RDONLY, 0);

	if (fp) {
		pr_debug("LoadWiMAXBootImage ..");
		g_wimax_image.data = vmalloc(MAX_WIMAXBOOTIMAGE_SIZE);
		if (!g_wimax_image.data) {
			pr_debug("Error: Memory alloc failure.");
			klib_fclose(fp);
			return -1;
		}

		memset(g_wimax_image.data, 0, MAX_WIMAXBOOTIMAGE_SIZE);
		read_size = klib_flen_fcopy(g_wimax_image.data,
					MAX_WIMAXBOOTIMAGE_SIZE, fp);
		g_wimax_image.size = read_size;
		g_wimax_image.address = 0;
		g_wimax_image.offset = 0;
		mutex_init(&g_wimax_image.lock);

		klib_fclose(fp);
	} else {
		pr_debug("Error: WiMAX image file open failed");
		return -1;
	}

	return 0;
}

int write_rev(void)
{
	u_int	val;
	int retry = 100;
	int err;

	pr_debug("HW REV: %d", system_rev);

	/* swap */
	val = be32_to_cpu(system_rev);

	do {
		err = wimax_i2c_write(0x7080, (char *)(&val), 4);
	} while (err < 0 ? ((retry--) > 0) : 0);
	if (retry < 0)
		pr_debug("eeprom error");
	return err ;
}

void erase_cert(void)
{
	char	buf[256] = {0};
	int	len = 4;
	int retry = 100;
	int err;

	do {
		err = wimax_i2c_write(0x5800, buf, len);
	} while (err < 0 ? ((retry--) > 0) : 0);
	if (retry < 0)
		pr_debug("eeprom error");
}

int check_cert(void)
{
	char	buf[256] = {0};
	int	len = 16;
	int retry = 100;
	int err;

	do {
		err = wimax_i2c_read(0x5800, buf, len);
	} while (err < 0 ? ((retry--) > 0) : 0);
	if (retry < 0)
		pr_debug("eeprom error");

	dump_buffer("Certification", (u_char *)buf, (u_int)len);

	/* check "Cert" */
	if (buf[0] == 0x43 && buf[1] == 0x65 &&
		buf[2] == 0x72 && buf[3] == 0x74)
		return 0;

	return -1;
}

int check_cal(void)
{
	char	buf[256] = {0};
	int	i;
	int	len = 32;
	int retry = 100;
	int err;

	do {
		err = wimax_i2c_read(0x5400, buf, len);
	} while (err < 0 ? ((retry--) > 0) : 0);
	if (retry < 0)
		pr_debug("eeprom error");

	dump_buffer("Calibration", (u_char *)buf, (u_int)len);

	/* at lease one buf[i] not equal 0xff means calibrated */
	for (i = 0; i < len; i++)
		if (buf[i] != 0xff)
			return 0;

	/* all 0xff means not calibrated */
	return -1;
}

void eeprom_read_boot()
{
	char	*buf = NULL;
	int	i, j;
	int	len;
	u_int	checksum = 0;
	short	addr = 0;
	int retry = 1000;
	int err;

	eeprom_poweron();

	buf = kmalloc(4096, GFP_KERNEL);
	if (buf == NULL) {
		pr_debug("eeprom_read_boot: MALLOC FAIL!!");
		return;
	}

	addr = 0x0;
	for (j = 0; j < 1; j++) {	/* read 4K */
		len = 4096;
		do {
			err = wimax_i2c_read(addr, buf, len);
		} while (err < 0 ? ((retry--) > 0) : 0);

		{
			/* dump boot data */
			char print_buf[256] = {0};
			char chr[8] = {0};

			/* dump packets with checksum */
			u_char  *b = (u_char *)buf;
			pr_debug("EEPROM dump [0x%04x] = ", addr);

			for (i = 0; i < len; i++) {
				checksum += b[i];
				sprintf(chr, " %02x", b[i]);
				strcat(print_buf, chr);
				if (((i + 1) != len) && (i % 16 == 15)) {
					pr_debug("%s", print_buf);
					memset(print_buf, 0x0, 256);
				}
			}
			pr_debug("%s", print_buf);
		}
		addr += len;
	}

	pr_debug("Checksum: 0x%08x", checksum);

	kfree(buf);

	eeprom_poweroff();
	if (retry < 0)
		pr_debug("eeprom error");
}

void eeprom_read_all()
{
	char	*buf = NULL;
	int	i, j;
	int	len;
	u_int	checksum = 0;
	short	addr = 0;
	int retry = 1000;
	int err;

	eeprom_poweron();

	/* allocate 4K buffer */
	buf = kmalloc(4096, GFP_KERNEL);
	if (buf == NULL) {
		pr_debug("eeprom_read_all: MALLOC FAIL!!");
		return;
	}

	addr = 0x0;

	/* read 64K */
	for (j = 0; j < 16; j++) {
		len = 4096;
		do {
			err = wimax_i2c_read(addr, buf, len);
		} while (err < 0 ? ((retry--) > 0) : 0);

		{
			/* dump EEPROM */
			char print_buf[256] = {0};
			char chr[8] = {0};

			/* dump packets with checksum */
			u_char  *b = (u_char *)buf;
			pr_debug("EEPROM dump [0x%04x] = ", addr);

			for (i = 0; i < len; i++) {
				checksum += b[i];
				sprintf(chr, " %02x", b[i]);
				strcat(print_buf, chr);
				if (((i + 1) != len) && (i % 16 == 15)) {
					pr_debug("%s", print_buf);
					memset(print_buf, 0x0, 256);
				}
			}
			pr_debug("%s", print_buf);
		}
		addr += len;
	}

	pr_debug("Checksum: 0x%08x", checksum);

	/* free 4K buffer */
	kfree(buf);

	eeprom_poweroff();
	if (retry < 0)
		pr_debug("eeprom error");
}

void eeprom_erase_all()
{
	char	buf[128] = {0};
	int	i;
	int retry = 1000;
	int err;

	eeprom_poweron();

	memset(buf, 0xff, 128);
	for (i = 0; i < 512; i++) {	/* clear all EEPROM */
		pr_debug("ERASE [0x%04x]\n", i * 128);
		do {
			err = wimax_i2c_write(128 * i, buf, 128);
		} while (err < 0 ? ((retry--) > 0) : 0);
	}

	eeprom_poweroff();
	if (retry < 0)
		pr_debug("eeprom error");
}

/* Write boot image to EEPROM */
int eeprom_write_boot(void)
{
	u_char	*buffer;
	int retry = 1000;
	int err = 0;
	u_short	ucsize = MAX_BOOT_WRITE_LENGTH;

	if (load_wimax_boot() < 0) {
		pr_debug("ERROR READ WIMAX BOOT IMAGE");
		return -1;
	}

	eeprom_poweron();

	g_wimax_image.offset = 0;

	while (g_wimax_image.size > g_wimax_image.offset) {
		buffer = (u_char *)(g_wimax_image.data + g_wimax_image.offset);
		ucsize = MAX_BOOT_WRITE_LENGTH;

		/* write buffer */
		do	{
			err = wimax_i2c_write(
			(u_short)g_wimax_image.offset, buffer, ucsize);
		} while (err < 0 ? ((retry--) > 0) : 0);

		g_wimax_image.offset += MAX_BOOT_WRITE_LENGTH;

		if ((g_wimax_image.size - g_wimax_image.offset)
						< MAX_BOOT_WRITE_LENGTH) {
			buffer = (u_char *)(g_wimax_image.data +
						g_wimax_image.offset);
			ucsize = g_wimax_image.size - g_wimax_image.offset;

			/* write last data */
			do {
				err = wimax_i2c_write(
				(u_short)g_wimax_image.offset, buffer, ucsize);
			} while (err < 0 ? ((retry--) > 0) : 0);

			g_wimax_image.offset += MAX_BOOT_WRITE_LENGTH;
		}
	}

	eeprom_poweroff();

	if (g_wimax_image.data) {
		pr_debug("Delete the Image Loaded");
		vfree(g_wimax_image.data);
		g_wimax_image.data = NULL;
	}

	if (retry < 0)
		pr_debug("eeprom error");
	else
		pr_debug("EEPROM WRITING DONE.");

	return err;
}

int eeprom_write_rev(void)
{
	int ret;

	eeprom_poweron();
	ret = write_rev();	/* write hw rev to eeprom */
	eeprom_poweroff();

	pr_debug("REV WRITING DONE.");
	return ret;
}

void eeprom_erase_cert(void)
{
	eeprom_poweron();
	erase_cert();
	eeprom_poweroff();

	pr_debug("ERASE CERT DONE.");
}

int eeprom_check_cert(void)
{
	int	ret;

	eeprom_poweron();
	ret = check_cert();	/* check cert */
	eeprom_poweroff();

	pr_debug("CHECK CERT DONE. (ret: %d)", ret);

	return ret;
}

int eeprom_check_cal(void)
{
	int	ret;

	eeprom_poweron();
	ret = check_cal();	/* check cal */
	eeprom_poweroff();

	pr_debug("CHECK CAL DONE. (ret: %d)", ret);

	return ret;
}

