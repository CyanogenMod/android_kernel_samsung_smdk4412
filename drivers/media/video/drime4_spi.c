/*
 * driver for DRIME4 SPI
 *
 * Copyright (c) 2011, Samsung Electronics. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include <media/v4l2-device.h>
#include <linux/videodev2_exynos_media.h>
#include <linux/videodev2_exynos_camera.h>

#include "drime4.h"

static struct spi_device *g_spi;

static u8 d4_rdata_buf[1024];
int g_noti_ctrl_pin;
int g_cmd_ctrl_pin;
int g_spi_writing;

static inline
int spi_xmit(const u8 *addr, const int len)
{
	int ret;

	struct spi_message msg;

	struct spi_transfer xfer = {
		.len = len,
		.tx_buf = addr,
	};

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(g_spi, &msg);

	if (ret < 0)
		dev_err(&g_spi->dev, "%s - error %d\n",
				__func__, ret);

	return ret;
}

static inline
int spi_xmit_rx(u8 *in_buf, size_t len)
{
	int ret;
	u8 read_out_buf[2];

	struct spi_message msg;
	struct spi_transfer xfer = {
		.tx_buf = read_out_buf,
		.rx_buf = in_buf,
		.len	= len,
		.cs_change = 0,
	};

	spi_message_init(&msg);

	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(g_spi, &msg);

	if (ret < 0)
		dev_err(&g_spi->dev, "%s - error %d\n",
				__func__, ret);

	return ret;
}

int drime4_spi_read(u8 *buf, size_t len, const int rxSize)
{
	int k;
	int ret = 0;
	u32 count = len/rxSize;
	u32 extra = len%rxSize;

	for (k = 0; k < count; k++) {
		ret = spi_xmit_rx(&buf[rxSize*k], rxSize);
		if (ret < 0)
			return -EINVAL;
	}

	if (extra != 0) {
		ret = spi_xmit_rx(&buf[rxSize*k], extra);
		if (ret < 0)
			return -EINVAL;
	}

	return 0;
}

int drime4_spi_write(const u8 *addr, const int len, const int txSize)
{
	int i, j = 0;
	int ret = 0;
#if 0
	u8 paddingData[32];
#endif
	u32 count = len/txSize;
	u32 extra = len%txSize;

#if 0
	memset(paddingData, 0, sizeof(paddingData));
#endif

	for (i = 0 ; i < count ; i++) {
		ret = spi_xmit(&addr[j], txSize);
		j += txSize;
		if (ret < 0)
			goto exit_err;
	}

	if (extra) {
		ret = spi_xmit(&addr[j], extra);
		if (ret < 0)
			goto exit_err;
	}

#if 0
	ret = spi_xmit(paddingData, 32);
	if (ret < 0)
		goto exit_err;
#endif

exit_err:
	return ret;
}

int drime4_spi_com_write(u8 len, u8 cate, u8 byte, long long val)
{
	int ret = 0;
	unsigned char buf[len + 3];
	int i;

	if ((len != 0x01) && (len != 0x02) &&
			(len != 0x04) && (len != 0x06))
		return -EINVAL;

	buf[0] = len + 3;
	buf[1] = cate;
	buf[2] = byte;
	if (len == 0x01) {
		buf[3] = val & 0xFF;
	} else if (len == 0x02) {
		buf[3] = (val >> 8) & 0xFF;
		buf[4] = val & 0xFF;
	} else if (len == 0x04) {
		buf[3] = (val >> 24) & 0xFF;
		buf[4] = (val >> 16) & 0xFF;
		buf[5] = (val >> 8) & 0XFF;
		buf[6] = val & 0xFF;
	} else if (len == 0x06) {
		buf[3] = (val >> 40) & 0xFF;
		buf[4] = (val >> 32) & 0xFF;
		buf[5] = (val >> 24) & 0XFF;
		buf[6] = (val >> 16) & 0XFF;
		buf[7] = (val >> 8) & 0XFF;
		buf[8] = val & 0xFF;
	}

	pr_err("Write %s %#x, byte %#x, value %llx\n",
			(len == 4 ? "L" : (len == 2 ? "W" : "B")),
			cate, byte, val);
	for (i = 0; i < len + 3; i++)
		pr_err("Buf[%d] = %#x\n", i, buf[i]);

	ret = drime4_spi_write((char *)&buf, len + 3, 1);
	if (ret < 0)
		goto exit_err;

exit_err:
	return ret;
}

int drime4_spi_get_write_status(void)
{
	return g_spi_writing;
}

int drime4_spi_get_noti_ctrl_pin(void)
{
	return g_noti_ctrl_pin;
}

int drime4_spi_get_cmd_ctrl_pin(void)
{
	return g_cmd_ctrl_pin;
}

int drime4_spi_get_noti_check_pin(void)
{
	if (system_rev > 7)
		return gpio_get_value(GPIO_CAM_SDA);
	else
		return gpio_get_value(GPIO_GSENSE_SDA_18V);
}

int drime4_spi_set_noti_ctrl_pin(int val)
{
	int log = 0;
#ifdef FEATURE_SPI_PROTOCOL_TRI_PIN
	if (val) {
		gpio_direction_output(GPIO_CAM_SPI_SSN,
			GPIO_LEVEL_HIGH);
#if 0
		if (val != g_noti_ctrl_pin)
			log = 1;
#endif
		g_noti_ctrl_pin = 1;
	} else {
		gpio_direction_output(GPIO_CAM_SPI_SSN,
			GPIO_LEVEL_LOW);
#if 0
		if (val != g_noti_ctrl_pin)
			log = 1;
#endif
		g_noti_ctrl_pin = 0;
	}

#if 0
	if (log) {
		pr_err("drime4 noti ctrl pin[%d]\n",
			g_noti_ctrl_pin);
	}
#endif
	return g_noti_ctrl_pin;
#else
	return drime4_spi_set_cmd_ctrl_pin(val);
#endif
}

int drime4_spi_set_cmd_ctrl_pin(int val)
{
	int log = 0;

	if (val) {
		if (system_rev > 7)
			gpio_direction_output(GPIO_CAM_SCL,
				GPIO_LEVEL_HIGH);
		else
			gpio_direction_output(GPIO_GSENSE_SCL_18V,
				GPIO_LEVEL_HIGH);
#if 0
		if (val != g_cmd_ctrl_pin)
			log = 1;
#endif
		g_cmd_ctrl_pin = 1;
	} else {
		if (system_rev > 7)
			gpio_direction_output(GPIO_CAM_SCL,
				GPIO_LEVEL_LOW);
		else
			gpio_direction_output(GPIO_GSENSE_SCL_18V,
				GPIO_LEVEL_LOW);
#if 0
		if (val != g_cmd_ctrl_pin)
			log = 1;
#endif
		g_cmd_ctrl_pin = 0;
	}

#if 0
	if (log) {
		pr_err("drime4 cmd ctrl pin[%d] writing[%d]\n",
			g_cmd_ctrl_pin, g_spi_writing);
	}
#endif
	return g_cmd_ctrl_pin;
}

/* Caution :
	payload(packet) is consist of

	Parameter ID(2bytes),
	Parameter Payload Length(1byte),
	Parameter Payload(1~255bytes?)

	use LITTLE ENDIAN

	ex) ID 0x1234, length 2, payload 0x5678
	=> {0x34, 0x12, 0x02, 0x78, 0x56}
*/
int drime4_spi_write_packet(struct v4l2_subdev *sd,
	u8 packet_id, u32 len, u8 *payload, int log)
{
	int ret = 0;
	int all_length = len + 6;
	unsigned char buf[all_length];
	int i, sum = 0;
	int ap_gpio_val = 0;
	int d4_gpio_val = 0;
	int noti_gpio_val = 0;
	int retry_cnt = 0;
	int retry = 0;
	struct drime4_state *state = container_of(sd, struct drime4_state, sd);

	if (len <= 0 || len > (1024-6)) {
		pr_err("drime4_spi Write command : error data length - %d\n",
			len);
		goto exit_err;
	}

	g_spi_writing = 1;

	buf[0] = all_length & 0xFF;
	buf[1] = (all_length >> 8) & 0xFF;
	buf[2] = packet_id;

	for (i = 0; i < len; i++)
		buf[3+i] = *(payload+i) & 0xFF;

	buf[len+3] = 0xFF;
	buf[len+4] = 0xFF;
	for (i = 0; i < (all_length-1); i++)
		sum += buf[i];
	buf[len+5] = (unsigned char)(((~sum)+1) & 0xFF);

gpio_check_retry:
	drime4_spi_set_cmd_ctrl_pin(0);
	usleep_range(1000, 1100);
	noti_gpio_val = drime4_spi_get_noti_check_pin();
	ap_gpio_val = gpio_get_value(GPIO_ISP_INT);
	d4_gpio_val = gpio_get_value(GPIO_RESERVED_1_INT);
	if (!ap_gpio_val)
		retry = 1;
	else if (d4_gpio_val)
		retry = 1;
	else if (noti_gpio_val)
		retry = 1;
	else
		retry = 0;

	if (retry == 1) {
		retry_cnt++;
		if (retry_cnt < 5) {
			usleep_range(1000, 1100);
		} else {
			if (retry_cnt == 5 || retry_cnt == 1000) {
				pr_err("drime4 retry : ap_int = %d, d4_int = %d, noti = %d, scl = %d, boot_error = %d\n",
					ap_gpio_val, d4_gpio_val,
					noti_gpio_val, g_cmd_ctrl_pin,
					g_drime4_boot_error);
			}
			usleep_range(5000, 5100);
		}
		if (retry_cnt < 1000) {
#if 0
			pr_err("HSW PINCHECK 00 : ap_int = %d, d4_int = %d, noti = %d, scl = %d\n",
				ap_gpio_val, d4_gpio_val,
				noti_gpio_val, g_noti_ctrl_pin);
#endif
			if (retry_cnt > 6 && ap_gpio_val == 0
				&& d4_gpio_val == 0
				&& state->isp.ap_issued == 1) {
				u8 rdata[500] = {0,};
				int err = 0;
				pr_err("HSW PINCHECK 01 : state->isp.ap_issued = %d, boot_error = %d\n",
					state->isp.ap_issued,
					g_drime4_boot_error);
				err = drime4_spi_read_res(&rdata[0], 1, 1);
				usleep_range(10000, 11000);
				state->isp.ap_issued = 0;
				retry_cnt = 0;
				ret = -1;
				goto exit_err;
			}
			goto gpio_check_retry;
		} else {
			pr_err("drime4 retry_fail : ap_int = %d, d4_int = %d, noti = %d, scl = %d, boot_error = %d\n",
				ap_gpio_val, d4_gpio_val,
				noti_gpio_val, g_cmd_ctrl_pin,
				g_drime4_boot_error);
			pr_err("drime4_spi Write busy fail: packetID 0x%02x, payload len %d\n",
					packet_id, len);
			ret = -1;
			retry = 0;
			goto exit_err;
		}
	} else if (retry_cnt > 3) {
		pr_err("drime4 retry %d : write start",
			retry_cnt);
	}

	ret = spi_xmit(&buf[0], all_length);
	if (ret < 0) {
		pr_err("drime4_spi Write fail: packetID 0x%02x, len %d\n",
			packet_id, len);
		goto exit_err;
	}

	if (log == 1) {
		pr_err("drime4_spi Write packetID 0x%02x, len %d\n",
			packet_id, len);
		for (i = 0; i < all_length; i++)
			pr_err("drime4_spi Write Buf[%d] = 0x%02x\n",
				i, buf[i]);
	}

exit_err:
	if (ret < 0)
		drime4_spi_set_cmd_ctrl_pin(1);
	g_spi_writing = 0;

	return ret;
}

/* Caution :
	payload(packet) is consist of

	Parameter ID(2bytes),
	Parameter Payload Length(1byte),
	Parameter Payload(1~255bytes?)

	use LITTLE ENDIAN
	Do Not use vmalloc for payload buffer

	ex) ID 0x1234, length 2, payload 0x5678
	=> {0x34, 0x12, 0x02, 0x78, 0x56}
*/
int drime4_spi_write_burst(u8 packet_id, u32 len, const u8 *payload, int log)
{
	int ret = 0;
	int txSize = 0xF000;	/* 60KByte */
	int alpha_length = 6;
	int all_length = txSize + alpha_length;
	u8 write_buf[alpha_length];
	u32 count = len/txSize;
	u32 extra = len%txSize;
	int i, j = 0;
	int k, c_sum = 0, sum = 0;
	int ap_gpio_val = 0;
	int d4_gpio_val = 0;
	int noti_gpio_val = 0;
	int retry_cnt = 0;
	int retry = 0;

	g_spi_writing = 1;

gpio_check_burst_retry:
	drime4_spi_set_cmd_ctrl_pin(0);
	usleep_range(1000, 1100);
	noti_gpio_val = drime4_spi_get_noti_check_pin();
	ap_gpio_val = gpio_get_value(GPIO_ISP_INT);
	d4_gpio_val = gpio_get_value(GPIO_RESERVED_1_INT);
	if (!ap_gpio_val)
		retry = 1;
	else if (d4_gpio_val)
		retry = 1;
	else if (noti_gpio_val)
		retry = 1;
	else
		retry = 0;

	if (retry == 1) {
		retry_cnt++;
		if (retry_cnt < 5) {
			usleep_range(1000, 1100);
		} else {
			if (retry_cnt == 5 || retry_cnt == 1000) {
				pr_err("drime4 retry : ap_int = %d, d4_int = %d, noti = %d, scl = %d\n",
					ap_gpio_val, d4_gpio_val,
					noti_gpio_val, g_cmd_ctrl_pin);
			}
			usleep_range(5000, 5100);
		}
		if (retry_cnt < 1000)
			goto gpio_check_burst_retry;
		else {
			pr_err("drime4 retry_fail : ap_int = %d, d4_int = %d, noti = %d, scl = %d\n",
				ap_gpio_val, d4_gpio_val,
				noti_gpio_val, g_cmd_ctrl_pin);
			pr_err("drime4_spi Write busy fail: packetID 0x%02x, payload len %d\n",
				packet_id, len);
			ret = -1;
			retry = 0;
			goto exit_err;
		}
	} else if (retry_cnt > 3) {
		pr_err("drime4 retry %d : write start",
			retry_cnt);
	}

	if (log == 1) {
		pr_err("drime4_spi Write start payload len %d, count %d, extra %d\n",
			len, count, extra);
	}

	write_buf[0] = all_length & 0xFF;
	write_buf[1] = (all_length >> 8) & 0xFF;
	write_buf[2] = packet_id;
	write_buf[3] = 0xFF;
	write_buf[4] = 0xFF;
	for (k = 0; k < (alpha_length-1); k++)
		c_sum += write_buf[k];
	for (i = 0 ; i < count ; i++) {
		sum = c_sum;
		if (packet_id < 0xF1) {
			for (k = 0 ; k < txSize ; k++)
				sum += payload[j+k];
		}
		write_buf[5] = (unsigned char)(((~sum)+1) & 0xFF);

		if (log == 1) {
			pr_err("drime4_spi Write count[%d] packetID 0x%02x, len %d\n",
				i, packet_id, all_length);
#if 0
			for (k = 0; k < 3; k++)
				pr_err("drime4_spi Write Buf[%d] = 0x%02x\n",
					k, write_buf[k]);
			for (k = 0; k < txSize; k++)
				pr_err("drime4_spi Write Buf[%d] = 0x%02x\n",
					k, payload[j+k]);
			for (k = 3; k < (3+3); k++)
				pr_err("drime4_spi Write Buf[%d] = 0x%02x\n",
					k+txSize, write_buf[k]);
#endif
		}

		ret = spi_xmit(&write_buf[0], 3);
		if (ret < 0) {
			pr_err("drime4_spi Write 1[%d] fail: packetID 0x%02x, payload len %d\n",
				i, packet_id, len);
			goto exit_err;
		}

		ret = spi_xmit(&payload[j], txSize);
		j += txSize;
		if (ret < 0) {
			pr_err("drime4_spi Write 2[%d] fail: packetID 0x%02x, payload len %d\n",
				i, packet_id, len);
			goto exit_err;
		}

		ret = spi_xmit(&write_buf[3], 3);
		if (ret < 0) {
			pr_err("drime4_spi Write 3[%d] fail: packetID 0x%02x, payload len %d\n",
				i, packet_id, len);
			goto exit_err;
		}
	}

	if (extra) {
		all_length = extra + alpha_length;
		write_buf[0] = all_length & 0xFF;
		write_buf[1] = (all_length >> 8) & 0xFF;
		sum = 0;
		for (k = 0; k < (alpha_length-1); k++)
			sum += write_buf[k];
		if (packet_id < 0xF1) {
			for (k = 0; k < extra; k++)
				sum += payload[j+k];
		}
		write_buf[5] = (unsigned char)(((~sum)+1) & 0xFF);

		if (log == 1) {
			pr_err("drime4_spi Write extra packetID 0x%02x, len %d\n",
				packet_id, all_length);
#if 0
			for (k = 0; k < 3; k++)
				pr_err("drime4_spi Write Buf[%d] = 0x%02x\n",
					k, write_buf[k]);
			for (k = 0; k < extra; k++)
				pr_err("drime4_spi Write Buf[%d] = 0x%02x\n",
					k+3, payload[j+k]);
			for (k = 3; k < (3+3); k++)
				pr_err("drime4_spi Write Buf[%d] = 0x%02x\n",
					k+extra, write_buf[k]);
#endif
		}

		ret = spi_xmit(&write_buf[0], 3);
		if (ret < 0) {
			pr_err("drime4_spi Write 2[%d] fail: packetID 0x%02x, payload len %d\n",
				i, packet_id, len);
			goto exit_err;
		}

		ret = spi_xmit(&payload[j], extra);
		if (ret < 0) {
			pr_err("drime4_spi Write 2[%d] fail: packetID 0x%02x, payload len %d\n",
				i, packet_id, len);
			goto exit_err;
		}

		ret = spi_xmit(&write_buf[3], 3);
		if (ret < 0) {
			pr_err("drime4_spi Write 3[%d] fail: packetID 0x%02x, payload len %d\n",
				i, packet_id, len);
			goto exit_err;
		}
	}

	g_spi_writing = 0;

exit_err:
	if (ret < 0)
		drime4_spi_set_cmd_ctrl_pin(1);

	return ret;
}

/* Caution :
	It doesn't include a packet id,
	a length,	a end of parameter
	and a check sum.
	Send only payload.

	payload(packet) is consist of

	Parameter ID(2bytes),
	Parameter Payload Length(1byte),
	Parameter Payload(1~255bytes?)

	use LITTLE ENDIAN
	Do Not use vmalloc for payload buffer

	ex) ID 0x1234, length 2, payload 0x5678
	=> {0x34, 0x12, 0x02, 0x78, 0x56}
*/
int drime4_spi_write_burst_only_data(u32 len, const u8 *payload, int log)
{
	int ret = 0;
	int txSize = 0xF000;	/* 60KByte */
	u32 count = len/txSize;
	u32 extra = len%txSize;
	int i, j = 0;
	int ap_gpio_val = 0;
	int d4_gpio_val = 0;
	int noti_gpio_val = 0;
	int retry_cnt = 0;
	int retry = 0;

	g_spi_writing = 1;

gpio_check_burst_retry:
	drime4_spi_set_cmd_ctrl_pin(0);
	usleep_range(1000, 1100);
	noti_gpio_val = drime4_spi_get_noti_check_pin();
	ap_gpio_val = gpio_get_value(GPIO_ISP_INT);
	d4_gpio_val = gpio_get_value(GPIO_RESERVED_1_INT);
	if (!ap_gpio_val)
		retry = 1;
	else if (d4_gpio_val)
		retry = 1;
	else if (noti_gpio_val)
		retry = 1;
	else
		retry = 0;

	if (retry == 1) {
		retry_cnt++;
		if (retry_cnt < 5) {
			usleep_range(1000, 1100);
		} else {
			if (retry_cnt == 5 || retry_cnt == 1000) {
				pr_err("drime4 retry : ap_int = %d, d4_int = %d, noti = %d, scl = %d\n",
					ap_gpio_val, d4_gpio_val,
					noti_gpio_val, g_cmd_ctrl_pin);
			}
			usleep_range(5000, 5100);
		}
		if (retry_cnt < 1000)
			goto gpio_check_burst_retry;
		else {
			pr_err("drime4 retry_fail : ap_int = %d, d4_int = %d, noti = %d, scl = %d\n",
				ap_gpio_val, d4_gpio_val,
				noti_gpio_val, g_cmd_ctrl_pin);
			pr_err("drime4_spi Write busy fail: payload len %d\n",
				len);
			ret = -1;
			retry = 0;
			goto exit_err;
		}
	} else if (retry_cnt > 3) {
		pr_err("drime4 retry %d : write start",
			retry_cnt);
	}

	if (log == 1) {
		pr_err("drime4_spi Write start payload len %d, count %d, extra %d\n",
			len, count, extra);
	}

	for (i = 0 ; i < count ; i++) {

		if (log == 1) {
			pr_err("drime4_spi Write count[%d] main payload len %d\n",
				i, txSize);
		}

		ret = spi_xmit(&payload[j], txSize);
		j += txSize;
		if (ret < 0) {
			pr_err("drime4_spi Write 2[%d] fail: main payload len %d\n",
				i, len);
			goto exit_err;
		}
	}

	if (extra) {

		if (log == 1)
			pr_err("drime4_spi Write extra extra_len %d\n", extra);

		ret = spi_xmit(&payload[j], extra);
		if (ret < 0) {
			pr_err("drime4_spi Write 2[%d] fail: extra payload len %d\n",
				i, extra);
			goto exit_err;
		}
	}

	g_spi_writing = 0;

exit_err:
	if (ret < 0)
		drime4_spi_set_cmd_ctrl_pin(1);

	return ret;
}

int drime4_spi_read_res(u8 *buf, int noti, int log)
{
	int ret = 0;
	int all_length;
	int i, cal_checksum, sum = 0;

	memset(d4_rdata_buf, 0, sizeof(d4_rdata_buf));

	ret = drime4_spi_read((char *)&d4_rdata_buf[0], 2, 512);
	if (ret < 0) {
		pr_err("drime4_spi Read error: length read\n");
		goto exit_err;
	}

	all_length = d4_rdata_buf[0] + ((d4_rdata_buf[1] << 8) & 0xFF00);

	if (all_length <= 2 || all_length > 1024) {
		pr_err("drime4_spi Read error: length is wrong %d\n",
			all_length);
		goto exit_err;
	}

	ret = drime4_spi_read((char *)&d4_rdata_buf[2], all_length-2, 512);
	if (ret < 0) {
		pr_err("drime4_spi Read error: read data\n");
		goto exit_err;
	}

	for (i = 0; i < (all_length-1); i++)
		sum += d4_rdata_buf[i];

	cal_checksum = ((~sum)+1) & 0xFF;
	if (d4_rdata_buf[all_length-1] != cal_checksum) {
		pr_err("drime4_spi Read checksum error: cal 0x%02x, read 0x%02x\n",
			cal_checksum, d4_rdata_buf[all_length-1]);
		log = 1;
		/*ret = -1;*/
	}

	memcpy(buf, d4_rdata_buf, all_length-3);

	if (log) {
		for (i = 0; i < all_length; i++) {
			pr_err("drime4_spi Read buf[%d] = 0x%02x\n",
				i, d4_rdata_buf[i]);
		}
	}

	if (noti != 3)	/* if (!noti) HSW */
		drime4_spi_set_cmd_ctrl_pin(1);

exit_err:
	return ret;
}

int drime4_spi_read_length(int size)
{
	int ret = 0;

	memset(d4_rdata_buf, 0, sizeof(d4_rdata_buf));

	ret = drime4_spi_read((char *)&d4_rdata_buf[0], size, 512);
	if (ret < 0) {
		pr_err("drime4_spi Read error: length read\n");
		goto exit_err;
	}

	ret = d4_rdata_buf[0]
		+ ((d4_rdata_buf[1] << 8) & 0xFF00)
		+ ((d4_rdata_buf[2] << 16) & 0xFF0000)
		+ ((d4_rdata_buf[3] << 24) & 0xFF000000);

exit_err:
	return ret;
}

int drime4_spi_read_burst(u8 *buf, int size)
{
	int ret = 0;
#if 1
	ret = drime4_spi_read((char *)&buf[0], size, 0xF000);
#else
	ret = drime4_spi_read((char *)&buf[0], size, 512);
#endif
	if (ret < 0) {
		pr_err("drime4_spi Read error: length read\n");
		goto exit_err;
	}

exit_err:
	return ret;
}


static
int __devinit drime4_spi_probe(struct spi_device *spi)
{
	int ret;

	spi->bits_per_word = 8;
	if (spi_setup(spi)) {
		pr_err("failed to setup spi for drime4_spi\n");
		ret = -EINVAL;
		goto err_setup;
	}

	g_spi = spi;

	pr_err("drime4_spi successfully probed\n");

	return 0;

err_setup:
	return ret;
}

static
int __devexit drime4_spi_remove(struct spi_device *spi)
{
	return 0;
}

static
struct spi_driver drime4_spi_driver = {
	.driver = {
		.name = "drime4_spi",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
	},
	.probe = drime4_spi_probe,
	.remove = __devexit_p(drime4_spi_remove),
};

static
int __init drime4_spi_init(void)
{
	int ret;
	pr_err("%s\n", __func__);

	ret = spi_register_driver(&drime4_spi_driver);

	if (ret)
		pr_err("failed to register drime4 fw - %x\n", ret);

	return ret;
}

static
void __exit drime4_spi_exit(void)
{
	spi_unregister_driver(&drime4_spi_driver);
}

module_init(drime4_spi_init);
module_exit(drime4_spi_exit);

MODULE_DESCRIPTION("DRIMe4 SPI driver");
MODULE_LICENSE("GPL");
