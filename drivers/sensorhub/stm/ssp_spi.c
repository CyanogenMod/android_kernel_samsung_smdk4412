/*
 * driver for Android SensorHub SPI
 *
 * Copyright (c) 2013, Samsung Electronics. All rights reserved
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
#include <linux/delay.h>

#if defined(DEBUG_SSP_SPI)
#define ssp_log(fmt, arg...)	\
	do {					\
		printk(KERN_ERR "[%s:%d] " fmt ,	\
			__func__, __LINE__, ##arg);		\
	}							\
	while (0)
#else
#define ssp_log(fmt, arg...)
#endif


/* If AP can't change the endian to BIG */
/* for s5c73m ISP, this option must is required.*/
/* This option depends on SPI_DMA_MODE */
/* in camera driver file*/
/*#define CHANGE_ENDIAN	*/


int ssp_spi_write_sync(struct spi_device *spi, const u8 *addr, const int len)
{
	int ret;
#if defined(CHANGE_ENDIAN)
	u8 buf[8] = {0};
#endif

	struct spi_message msg;

	struct spi_transfer xfer = {
		.len = len,
#if !defined(CHANGE_ENDIAN)
		.tx_buf = addr,
		/*QCTK ALRAN QUP_CONFIG 0-4 bits BIG ENDIAN*/
		.bits_per_word = 8,
#else
		.tx_buf = buf,
#endif
	};

#if defined(CHANGE_ENDIAN)
	buf[0] = addr[3];
	buf[1] = addr[2];
	buf[2] = addr[1];
	buf[3] = addr[0];
	buf[4] = addr[7];
	buf[5] = addr[6];
	buf[6] = addr[5];
	buf[7] = addr[4];
#endif

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(spi, &msg);

	if (ret < 0)
		ssp_log("error %d\n", ret);

	return ret;
}


int ssp_spi_read_sync(struct spi_device *spi, u8 *in_buf, size_t len)
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

	ret = spi_sync(spi, &msg);

	if (ret < 0)
		ssp_log("%s - error %d\n",
				__func__, ret);

	return ret;
}


int ssp_spi_sync(struct spi_device *spi, u8 *out_buf,
	size_t out_len, u8 *in_buf)
{
	int ret;

	struct spi_message msg;
	struct spi_transfer xfer = {
		.tx_buf = out_buf,
		.rx_buf = in_buf,
		.len	= out_len,
		.cs_change = 0,
	};

	spi_message_init(&msg);

	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(spi, &msg);
	ssp_log("%s - received %d\n", __func__, xfer.len);

	if (ret < 0)
		ssp_log("%s - error %d\n",
				__func__, ret);

	return ret;
}

unsigned int g_flag_spirecv;
void ssp_spi_async_complete(void *context)
{
	g_flag_spirecv = 1;
}

int ssp_spi_async(struct spi_device *spi,  u8 *out_buf,
	size_t out_len, u8 *in_buf)
{
	int ret;

	struct spi_message msg;
	struct spi_transfer xfer = {
		.tx_buf = out_buf,
		.rx_buf = in_buf,
		.len	= out_len,
		.cs_change = 0,
	};


	spi_message_init(&msg);

	spi_message_add_tail(&xfer, &msg);
	msg.complete = ssp_spi_async_complete;

	ret = spi_async(spi, &msg);

	if (ret < 0)
		ssp_log("%s - error %d\n",
				__func__, ret);

	return ret;
}




int ssp_spi_read(struct spi_device *spi, u8 *buf, size_t len, const int rxSize)
{
	int k;
	int ret = 0;
	u8 temp_buf[4] = {0};
	u32 count = len/rxSize;
	u32 extra = len%rxSize;

	for (k = 0; k < count; k++) {
		ret = ssp_spi_read_sync(spi, &buf[rxSize*k], rxSize);
		if (ret < 0) {
			ssp_log("%s - error %d\n",
				__func__, ret);
			return -EINVAL;
		}
	}

	if (extra != 0) {
		ret = ssp_spi_read_sync(spi, &buf[rxSize*k], extra);
		if (ret < 0) {
			ssp_log("%s - error %d\n",
				__func__, ret);
			return -EINVAL;
		}
	}

	for (k = 0; k < len-3; k += 4) {
		memcpy(temp_buf, (char *)&buf[k], sizeof(temp_buf));
		buf[k] = temp_buf[3];
		buf[k+1] = temp_buf[2];
		buf[k+2] = temp_buf[1];
		buf[k+3] = temp_buf[0];
	}

	return 0;
}

int ssp_spi_write(struct spi_device *spi, const u8 *addr,
	const int len, const int txSize)
{
	int i, j = 0;
	int ret = 0;
	u8 paddingData[8];
	u32 count = len/txSize;
	u32 extra = len%txSize;
	ssp_log("Entered\n");
	ssp_log("count = %d extra = %d\n", count, extra);

	memset(paddingData, 0, sizeof(paddingData));

	for (i = 0 ; i < count ; i++) {
		ret = ssp_spi_write_sync(spi, &addr[j], txSize);
		j += txSize;
		if (ret < 0) {
			ssp_log("failed to write ssp_spi_write_sync\n");
			goto exit_err;
		}
		ssp_log("Delay!!!\n");
		msleep(50);
	}

	if (extra) {
		ret = ssp_spi_write_sync(spi, &addr[j], extra);
		if (ret < 0) {
			ssp_log("failed to write ssp_spi_write_sync\n");
			goto exit_err;
		}
	}

	for (i = 0; i < 4; i++) {
		memset(paddingData, 0, sizeof(paddingData));
		ret = ssp_spi_write_sync(spi, paddingData, 8);
		if (ret < 0) {
			ssp_log("failed to write ssp_spi_write_sync\n");
			goto exit_err;
		}
	}
	ssp_log("Finish!!\n");
exit_err:
	return ret;
}

