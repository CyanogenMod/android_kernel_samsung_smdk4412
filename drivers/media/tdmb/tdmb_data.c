/*
*
* drivers/media/tdmb/tdmb_data.c
*
* tdmb driver
*
* Copyright (C) (2011, Samsung Electronics)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>

#include <linux/types.h>
#include <linux/fcntl.h>

/* for delay(sleep) */
#include <linux/delay.h>

/* for mutex */
#include <linux/mutex.h>

/*using copy to user */
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <linux/workqueue.h>
#include <linux/irq.h>
#include <asm/mach/irq.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>

#include <linux/io.h>
#include <mach/gpio.h>

#include "tdmb.h"

#define TS_PACKET_SIZE 188
#define MSC_BUF_SIZE 1024

static unsigned char *msc_buff;
static unsigned char *ts_buff;

static int ts_buff_size;
static int first_packet = 1;
static int ts_buff_pos;
static int msc_buff_pos;
static int mp2_len;
static const int bitrate_table[2][16] = {
	/* MPEG1 for id=1*/
	{0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},
	/* MPEG2 for id=0 */
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}
};

static struct workqueue_struct *tdmb_workqueue;
static DECLARE_WORK(tdmb_work, tdmb_pull_data);

irqreturn_t tdmb_irq_handler(int irq, void *dev_id)
{
	int ret = 0;

	if (tdmb_workqueue) {
		ret = queue_work(tdmb_workqueue, &tdmb_work);
		if (ret == 0)
			DPRINTK("failed in queue_work\n");
	}

	return IRQ_HANDLED;
}

bool tdmb_create_databuffer(unsigned long int_size)
{
	ts_buff_size = int_size * 2;

	msc_buff = vmalloc(MSC_BUF_SIZE);
	ts_buff = vmalloc(ts_buff_size);

	if (msc_buff && ts_buff) {
		return true;
	} else {
		if (msc_buff)
			vfree(msc_buff);
		if (ts_buff)
			vfree(ts_buff);

		return false;
	}
}

void tdmb_destroy_databuffer(void)
{
	vfree(msc_buff);
	vfree(ts_buff);
}

bool tdmb_create_workqueue(void)
{
	tdmb_workqueue = create_singlethread_workqueue("ktdmbd");
	if (tdmb_workqueue)
		return true;
	else
		return false;
}

bool tdmb_destroy_workqueue(void)
{
	if (tdmb_workqueue) {
		flush_workqueue(tdmb_workqueue);
		destroy_workqueue(tdmb_workqueue);
		tdmb_workqueue = NULL;
	}
	return true;
}

void tdmb_init_data(void)
{
	first_packet = 1;
	ts_buff_pos = 0;
	msc_buff_pos = 0;
	mp2_len = 0;
}

static int __add_to_ringbuffer(unsigned char *data, unsigned long data_size)
{
	int ret = 0;
	unsigned int size;
	unsigned int head;
	unsigned int tail;
	unsigned int dist;
	unsigned int temp_size;

	if (tdmb_ts_size == 0)
		return 0;

	size = data_size;
	head = *tdmb_ts_head;
	tail = *tdmb_ts_tail;

	if (size > tdmb_ts_size) {
		DPRINTK("Error - size too large\n");
	} else {
		if (head >= tail)
			dist = head-tail;
		else
			dist = tdmb_ts_size+head-tail;

		/* DPRINTK("dist: %x\n", dist); */

		if ((tdmb_ts_size-dist) < size) {
			DPRINTK("small space is left in ring(len:%d/free:%d)\n",
				size, (tdmb_ts_size-dist));
			DPRINTK("ts_head:0x%x, ts_tail:0x%x/head:%d,tail:%d\n",
				(unsigned int)tdmb_ts_head,
				(unsigned int)tdmb_ts_tail,
				(unsigned int)head, tail);
		} else {
			if (head+size <= tdmb_ts_size) {
				memcpy((tdmb_ts_buffer+head),
					(char *)data, size);

				head += size;
				if (head == tdmb_ts_size)
					head = 0;
			} else {
				temp_size = tdmb_ts_size-head;
				temp_size = (temp_size/DMB_TS_SIZE)*DMB_TS_SIZE;

				if (temp_size > 0)
					memcpy((tdmb_ts_buffer+head),
						(char *)data, temp_size);

				memcpy(tdmb_ts_buffer,
					(char *)(data+temp_size),
					size-temp_size);
				head = size-temp_size;
			}

			/*
			 *      DPRINTK("< data > %x, %x, %x, %x\n",
			 *            *(tdmb_ts_buffer+ *tdmb_ts_head),
			 *            *(tdmb_ts_buffer+ *tdmb_ts_head +1),
			 *            *(tdmb_ts_buffer+ *tdmb_ts_head +2),
			 *            *(tdmb_ts_buffer+ *tdmb_ts_head +3) );

			 *      DPRINTK("exiting - head : %d\n",head);
			 */

			*tdmb_ts_head = head;
		}
	}

	return ret;
}


static int __add_ts_data(unsigned char *data, unsigned long data_size)
{
	int j = 0;
	int maxi = 0;
	if (first_packet) {
		DPRINTK("! first sync Size = %ld !\n", data_size);

		for (j = 0; j < data_size; j++) {
			if (data[j] == 0x47) {
				DPRINTK("!!!!! first sync j = %d !!!!!\n", j);
				maxi = (data_size - j) / TS_PACKET_SIZE;
				ts_buff_pos = (data_size - j) % TS_PACKET_SIZE;
				__add_to_ringbuffer(&data[j],
					maxi * TS_PACKET_SIZE);
				if (ts_buff_pos > 0)
					memcpy(ts_buff,
						&data[j+maxi*TS_PACKET_SIZE],
						ts_buff_pos);
				first_packet = 0;
				return 0;
			}
		}
	} else {
		maxi = (data_size) / TS_PACKET_SIZE;

		if (ts_buff_pos > 0) {
			if (data[TS_PACKET_SIZE - ts_buff_pos] != 0x47) {
				DPRINTK("! error 0x%x,0x%x !\n",
					data[TS_PACKET_SIZE - ts_buff_pos],
					data[TS_PACKET_SIZE - ts_buff_pos + 1]);

				memset(ts_buff, 0, ts_buff_size);
				ts_buff_pos = 0;
				first_packet = 1;
				return -EPERM;
			}

			memcpy(&ts_buff[ts_buff_pos],
				data, TS_PACKET_SIZE-ts_buff_pos);
			__add_to_ringbuffer(ts_buff, TS_PACKET_SIZE);
			__add_to_ringbuffer(&data[TS_PACKET_SIZE - ts_buff_pos],
				data_size - TS_PACKET_SIZE);
			memcpy(ts_buff,
				&data[data_size-ts_buff_pos],
				ts_buff_pos);
		} else {
			if (data[0] != 0x47) {
				DPRINTK("!! error 0x%x,0x%x!!\n",
								data[0],
								data[1]);

				memset(ts_buff, 0, ts_buff_size);
				ts_buff_pos = 0;
				first_packet = 1;
				return -EPERM;
			}

			__add_to_ringbuffer(data, data_size);
		}
	}
	return 0;
}

static int __get_mp2_len(unsigned char *pkt)
{
	int id;
	int layer_index;
	int bitrate_index;
	int fs_index;
	int samplerate;
	int protection;
	int bitrate;
	int length;

	id = (pkt[1]>>3)&0x01; /* 1: ISO/IEC 11172-3, 0:ISO/IEC 13818-3 */
	layer_index = (pkt[1]>>1)&0x03; /* 2 */
	protection = pkt[1]&0x1;
/*
	if (protection != 0) {
		;
	}
*/
	bitrate_index = (pkt[2]>>4);
	fs_index = (pkt[2]>>2)&0x3; /* 1 */

	/* sync word check */
	if (pkt[0] == 0xff && (pkt[1]>>4) == 0xf) {
		if ((bitrate_index > 0 && bitrate_index < 15)
				&& (layer_index == 2) && (fs_index == 1)) {

			if (id == 1 && layer_index == 2) {
				/* Fs==48 KHz*/
				bitrate = 1000*bitrate_table[0][bitrate_index];
				samplerate = 48000;
			} else if (id == 0 && layer_index == 2) {
				/* Fs=24 KHz */
				bitrate = 1000*bitrate_table[1][bitrate_index];
				samplerate = 24000;
			} else
				return -EPERM;

		} else
			return -EPERM;
	} else
		return -EPERM;

	if ((pkt[2]&0x02) != 0) { /* padding bit */
		return -EPERM;
	}

	length = (144*bitrate)/(samplerate);

	return length;
}

static int
__add_msc_data(unsigned char *data, unsigned long data_size, int sub_ch_id)
{
	int j;
	int readpos = 0;
	unsigned char pOutAddr[TS_PACKET_SIZE];
	int remainbyte = 0;
	static int first = 1;

	if (first_packet) {
		for (j = 0; j < data_size-4; j++) {
			if (data[j] == 0xFF && ((data[j+1]>>4) == 0xF)) {
				mp2_len = __get_mp2_len(&data[j]);
				DPRINTK("first sync mp2_len= %d\n", mp2_len);
				if (mp2_len <= 0 || mp2_len > MSC_BUF_SIZE)
					return -EPERM;

				memcpy(msc_buff, &data[j], data_size-j);
				msc_buff_pos = data_size-j;
				first_packet = 0;
				first = 1;
				return 0;
			}
		}
	} else {
		if (mp2_len <= 0 || mp2_len > MSC_BUF_SIZE) {
			msc_buff_pos = 0;
			first_packet = 1;
			return -EPERM;
		}

		remainbyte = data_size;
		if ((mp2_len-msc_buff_pos) >= data_size) {
			memcpy(msc_buff+msc_buff_pos, data, data_size);
			msc_buff_pos += data_size;
			remainbyte = 0;
		} else if (mp2_len-msc_buff_pos > 0) {
			memcpy(msc_buff+msc_buff_pos,
				data, (mp2_len - msc_buff_pos));
			remainbyte = data_size - (mp2_len - msc_buff_pos);
			msc_buff_pos = mp2_len;
		}

		if (msc_buff_pos == mp2_len) {
			while (msc_buff_pos > readpos) {
				if (first) {
					pOutAddr[0] = 0xDF;
					pOutAddr[1] = 0xDF;
					pOutAddr[2] = (sub_ch_id<<2);
					pOutAddr[2] |=
						(((msc_buff_pos>>3)>>8)&0x03);
					pOutAddr[3] = (msc_buff_pos>>3)&0xFF;

					if (!(msc_buff[0] == 0xFF
						&& ((msc_buff[1]>>4) == 0xF))) {
						DPRINTK("!!error 0x%x,0x%x!!\n",
							msc_buff[0],
							msc_buff[1]);
						memset(msc_buff,
							0,
							MSC_BUF_SIZE);
						msc_buff_pos = 0;
						first_packet = 1;
						return -EPERM;
					}

					memcpy(pOutAddr+4, msc_buff, 184);
					readpos = 184;
					first = 0;
				} else {
					pOutAddr[0] = 0xDF;
					pOutAddr[1] = 0xD0;
					if (msc_buff_pos-readpos >= 184) {
						memcpy(pOutAddr+4,
							msc_buff+readpos,
							184);
						readpos += 184;
					} else {
						memcpy(pOutAddr+4,
							msc_buff+readpos,
							msc_buff_pos-readpos);
						readpos
						+= (msc_buff_pos-readpos);
					}
				}
				__add_to_ringbuffer(pOutAddr, TS_PACKET_SIZE);
			}

			first = 1;
			msc_buff_pos = 0;
			if (remainbyte > 0) {
				memcpy(msc_buff
					, data+data_size-remainbyte
					, remainbyte);
				msc_buff_pos = remainbyte;
			}
		} else if (msc_buff_pos > mp2_len) {
			DPRINTK("! Error msc_buff_pos=%d, mp2_len =%d!\n",
				msc_buff_pos, mp2_len);
			memset(msc_buff, 0, MSC_BUF_SIZE);
			msc_buff_pos = 0;
			first_packet = 1;
			return -EPERM;
		}
	}

	return 0;
}

bool tdmb_store_data(unsigned char *data, unsigned long len)
{
	unsigned long i;
	unsigned long maxi;
	unsigned long subch_id = tdmb_get_chinfo();

	if (subch_id == 0) {
		return false;
	} else {
		subch_id = subch_id % 1000;

		if (subch_id >= 64) {
			__add_ts_data(data, len);
		} else {
			maxi = len/TS_PACKET_SIZE;
			for (i = 0 ; i < maxi ; i++) {
				__add_msc_data(data, TS_PACKET_SIZE, subch_id);
				data += TS_PACKET_SIZE;
			}
			if (len - maxi * TS_PACKET_SIZE)
				__add_msc_data(data,\
					 len - maxi * TS_PACKET_SIZE, subch_id);
		}
		return true;
	}
}
