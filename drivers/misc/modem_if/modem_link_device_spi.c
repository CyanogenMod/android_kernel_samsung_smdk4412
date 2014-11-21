/*
 * Copyright (C) 2011 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/if_arp.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/suspend.h>
#include <linux/kthread.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_link_device_spi.h"
#include "modem_utils.h"

/* For function which has void parmeter */
static struct spi_link_device *p_spild;
static struct spi_device *p_spi;

static void spi_send_work(int spi_sigs, bool spi_work_t)
{
	struct spi_work_type *spi_wq;

	spi_wq = kmalloc(sizeof(struct spi_work_type), GFP_ATOMIC);
	if (unlikely(!spi_wq)) {
		pr_err("[LNK/E] <%s> Failed to kmalloc()\n", __func__);
		return;
	}

	spi_wq->signal_code = spi_sigs;
	INIT_WORK(&spi_wq->work, spi_work);

	if (spi_work_t == SPI_WORK_FRONT)
		queue_work_front(p_spild->spi_wq, (struct work_struct *)spi_wq);
	else
		queue_work(p_spild->spi_wq, (struct work_struct *)spi_wq);
}

static irqreturn_t spi_srdy_irq_handler(int irq, void *p_ld)
{
	struct link_device *ld = (struct link_device *)p_ld;
	struct spi_link_device *spild = to_spi_link_device(ld);

	irqreturn_t result = IRQ_HANDLED;

	if (!spild->boot_done)
		return result;

	if (!wake_lock_active(&spild->spi_wake_lock)) {
		wake_lock(&spild->spi_wake_lock);
		pr_debug("[SPI] [%s](%d) spi_wakelock locked . spild->spi_state[%d]\n",
			__func__, __LINE__, (int)spild->spi_state);
	}

	if (spild->send_modem_spi == 1)
		up(&spild->srdy_sem);

	/* SRDY interrupt work on SPI_STATE_IDLE state for receive data */
	if (spild->spi_state == SPI_STATE_IDLE
		|| spild->spi_state == SPI_STATE_RX_TERMINATE
		|| spild->spi_state == SPI_STATE_TX_TERMINATE)
		spi_send_work(SPI_WORK_RECEIVE, SPI_WORK_FRONT);

	return result;
}

static irqreturn_t spi_subsrdy_irq_handler(int irq, void *p_ld)
{
	struct link_device *ld = (struct link_device *)p_ld;
	struct spi_link_device *spild = to_spi_link_device(ld);

	irqreturn_t result = IRQ_HANDLED;

	/* SRDY interrupt work on SPI_STATE_TX_WAIT state for send data */
	if (spild->spi_state == SPI_STATE_TX_WAIT)
		return result;

	pr_debug("%s spild->spi_state[%d]\n",
		"[SPI] spi_main_subsrdy_rising_handler :",
		(int)spild->spi_state);


	return result;
}

static int spi_send
(
	struct link_device *ld,
	struct io_device *iod,
	struct sk_buff *skb
)
{
	struct sk_buff_head *txq;
	enum dev_format fmt = iod->format;

	const u32 cmd_ready = 0x12341234;
	const u32 cmd_start = 0x45674567;
	int ret;
	u32 data;

	switch (fmt) {
	case IPC_FMT:
	case IPC_RAW:
	case IPC_RFS:
		txq = ld->skb_txq[fmt];
		skb_queue_tail(txq, skb);

		ret = skb->len;

		break;

	case IPC_BOOT:
		if (get_user(data, (u32 __user *)skb->data))
			return -EFAULT;

		if (data == cmd_ready) {
			p_spild->ril_send_modem_img = 1;
			p_spild->ril_send_cnt = 0;
		} else if (data == cmd_start) {
			p_spild->ril_send_modem_img = 0;
			if (!queue_work(p_spild->ipc_spi_wq,
				&p_spild->send_modem_w))
				pr_err("(%d) already exist w-q\n",
					__LINE__);
		} else {
			if (p_spild->ril_send_modem_img) {
				memcpy((void *)(p_spild->p_virtual_buff
					+ p_spild->ril_send_cnt),
					skb->data, skb->len);
				p_spild->ril_send_cnt += skb->len;
			}
		}
		ret = skb->len;
		dev_kfree_skb_any(skb);

		return ret;

	default:
		pr_err("[LNK/E] <%s:%s> No TXQ for %s\n",
			__func__, ld->name, iod->name);
		dev_kfree_skb_any(skb);
		return 0;
	}

	spi_send_work(SPI_WORK_SEND, SPI_WORK);

	return ret;
}


static int spi_register_isr
(
	unsigned irq,
	irqreturn_t (*isr)(int, void*),
	unsigned long flag,
	const char *name,
	struct link_device *ld
)
{
	int ret = 0;

	ret = request_irq(irq, isr, flag, name, ld);
	if (ret) {
		pr_err("[LNK/E] <%s> request_irq fail (%d)\n",
			__func__, ret);
		goto err;
	}

	ret = enable_irq_wake(irq);
	if (ret) {
		pr_err("[LNK/E] <%s> enable_irq_wake fail (%d)\n",
			__func__, ret);
		free_irq(irq, ld);
		goto err;
	}

	pr_debug("[LNK] <%s> IRQ#%d handler is registered.\n", __func__, irq);

err:
	return ret;
}

void spi_unregister_isr(unsigned irq, void *data)
{
	free_irq(irq, data);
}

int spi_tx_rx_sync(u8 *tx_d, u8 *rx_d, unsigned len)
{
	struct spi_transfer t;
	struct spi_message msg;

	memset(&t, 0, sizeof t);

	t.len = len;

	t.tx_buf = tx_d;
	t.rx_buf = rx_d;

	t.cs_change = 0;

	t.bits_per_word = 32;
	t.speed_hz = 12000000;

	spi_message_init(&msg);
	spi_message_add_tail(&t, &msg);

	return spi_sync(p_spi, &msg);
}

static int spi_buff_write
(
	struct spi_link_device *spild,
	int dev_id,
	const char *buff,
	unsigned size
)
{
	u32 templength, buf_length;
	u32 spi_data_mux;
	u32 spi_packet_free_length;

	u8 *spi_packet;
	struct spi_data_packet_header *spi_packet_header;

	spi_packet_header = (struct spi_data_packet_header *)spild->buff;
	spi_packet = (char *)spild->buff;

	spi_packet_free_length = SPI_DATA_PACKET_MAX_PACKET_BODY_SIZE -
		spi_packet_header->current_data_size;

	buf_length = size + SPI_DATA_MUX_SIZE + SPI_DATA_LENGTH_SIZE;

	/* not enough space in spi packet */
	if (spi_packet_free_length < buf_length) {
		spi_packet_header->more = 1;
		return 0;
	}

	/* check spi mux type */
	switch (dev_id) {
	case IPC_FMT:
		spi_data_mux = SPI_DATA_MUX_IPC;
		break;

	case IPC_RAW:
		spi_data_mux = SPI_DATA_MUX_RAW;
		break;

	case IPC_RFS:
		spi_data_mux = SPI_DATA_MUX_RFS;
		break;

	default:
		pr_err("%s %s\n",
			"[SPI] ERROR : spi_buff_write:",
			"invalid dev_id");
		return 0;
	}

	/* copy spi mux field */
	memcpy(spi_packet + SPI_DATA_PACKET_HEADER_SIZE +
		spi_packet_header->current_data_size,
		&spi_data_mux, SPI_DATA_MUX_SIZE);
	spi_packet_header->current_data_size += SPI_DATA_MUX_SIZE;

	/* copy spi data length field */
	templength = size-SPI_DATA_BOF_SIZE-SPI_DATA_EOF_SIZE;
	memcpy(spi_packet + SPI_DATA_PACKET_HEADER_SIZE +
		spi_packet_header->current_data_size,
		&templength, SPI_DATA_LENGTH_SIZE);
	spi_packet_header->current_data_size += SPI_DATA_LENGTH_SIZE;

	/* copy data field */
	memcpy(spi_packet + SPI_DATA_PACKET_HEADER_SIZE +
		spi_packet_header->current_data_size,
		buff, size);
	spi_packet_header->current_data_size += size;

	return buf_length;
}


static void spi_prepare_tx_packet(void)
{
	struct link_device *ld;
	struct sk_buff *skb;
	int ret;
	int i;

	ld = &p_spild->ld;

	for (i = 0; i < ld->max_ipc_dev; i++) {
		while ((skb = skb_dequeue(ld->skb_txq[i]))) {
			ret = spi_buff_write(p_spild, i, skb->data, skb->len);
			if (!ret) {
				skb_queue_head(ld->skb_txq[i], skb);
				break;
			}
			dev_kfree_skb_any(skb);
		}
	}
}


static void spi_start_data_send(void)
{
	struct link_device *ld;
	int i;

	ld = &p_spild->ld;

	for (i = 0; i < ld->max_ipc_dev; i++) {
		if (skb_queue_len(ld->skb_txq[i]) > 0)
			spi_send_work(SPI_WORK_SEND, SPI_WORK);
	}
}

static void spi_tx_work(void)
{
	struct spi_link_device *spild;
	struct io_device *iod;
	char *spi_packet_buf;
	char *spi_sync_buf;

	spild = p_spild;
	iod = link_get_iod_with_format(&spild->ld, IPC_FMT);
	if (!iod) {
		mif_err("no iodevice for modem control\n");
		return;
	}

	if (iod->mc->phone_state == STATE_CRASH_EXIT)
		return;

	/* check SUB SRDY, SRDY state */
	if (gpio_get_value(spild->gpio_ipc_sub_srdy) ==
		SPI_GPIOLEVEL_HIGH ||
		gpio_get_value(spild->gpio_ipc_srdy) ==
		SPI_GPIOLEVEL_HIGH) {
		spi_start_data_send();
		return;
	}

	if (get_console_suspended())
		return;

	if (spild->spi_state == SPI_STATE_END)
		return;

	/* change state SPI_STATE_IDLE to SPI_STATE_TX_WAIT */
	spild->spi_state = SPI_STATE_TX_WAIT;
	spild->spi_timer_tx_state = SPI_STATE_TIME_START;

	gpio_set_value(spild->gpio_ipc_mrdy, SPI_GPIOLEVEL_HIGH);

	/* Start TX timer */
	spild->spi_tx_timer.expires =
		jiffies + ((SPI_TIMER_TX_WAIT_TIME * HZ) / 1000);
	add_timer(&spild->spi_tx_timer);
	/* check SUBSRDY state */
	while (gpio_get_value(spild->gpio_ipc_sub_srdy) ==
		SPI_GPIOLEVEL_LOW) {
		if (spild->spi_timer_tx_state == SPI_STATE_TIME_OVER) {
			pr_err("%s spild->spi_state=[%d]\n",
				"[spi_tx_work] == spi Fail to receiving SUBSRDY CONF :"
				, (int)spild->spi_state);

			spild->spi_timer_tx_state = SPI_STATE_TIME_START;

			gpio_set_value(spild->gpio_ipc_mrdy,
				SPI_GPIOLEVEL_LOW);

			/* change state SPI_STATE_TX_WAIT */
			/* to SPI_STATE_IDLE */
			spild->spi_state = SPI_STATE_IDLE;
			spi_send_work(SPI_WORK_SEND, SPI_WORK);

			return;
		}
	}
	/* Stop TX timer */
	del_timer(&spild->spi_tx_timer);

	if (spild->spi_state != SPI_STATE_START
		&& spild->spi_state != SPI_STATE_END
		&& spild->spi_state != SPI_STATE_INIT) {
		spi_packet_buf = spild->buff;
		spi_sync_buf = spild->sync_buff;

		gpio_set_value(spild->gpio_ipc_sub_mrdy, SPI_GPIOLEVEL_HIGH);
		gpio_set_value(spild->gpio_ipc_mrdy, SPI_GPIOLEVEL_LOW);

		/* change state SPI_STATE_TX_WAIT to
			SPI_MAIN_STATE_TX_SENDING */
		spild->spi_state = SPI_STATE_TX_SENDING;

		memset(spi_packet_buf, 0, SPI_MAX_PACKET_SIZE);
		memset(spi_sync_buf, 0, SPI_MAX_PACKET_SIZE);

		spi_prepare_tx_packet();

		if (spi_tx_rx_sync((void *)spi_packet_buf, (void *)spi_sync_buf,
			SPI_MAX_PACKET_SIZE)) {
			/* TODO: save failed packet */
			/* back data to each queue */
			pr_err("[SPI] spi_dev_send fail\n");

			/* add cp reset when spi sync fail */
			if (iod)
				iod->modem_state_changed(iod,
						STATE_CRASH_RESET);
		}

		spild->spi_state = SPI_STATE_TX_TERMINATE;

		gpio_set_value(spild->gpio_ipc_sub_mrdy, SPI_GPIOLEVEL_LOW);

		/* change state SPI_MAIN_STATE_TX_SENDING to SPI_STATE_IDLE */
		spild->spi_state = SPI_STATE_IDLE;
		spi_start_data_send();
	} else
		pr_err("[SPI] ERR : _start_packet_tx:spild->spi_state[%d]",
			(int)spild->spi_state);

	return;
}

int spi_buff_read(struct spi_link_device *spild)
{
	struct link_device *ld;
	struct spi_data_packet_header *spi_packet_header;
	struct sk_buff *skb;
	char *spi_packet;
	int dev_id;
	unsigned int spi_packet_length;
	unsigned int spi_packet_cur_pos = SPI_DATA_PACKET_HEADER_SIZE;

	unsigned int spi_data_mux;
	unsigned int spi_data_length;
	unsigned int data_length;
	u8 *spi_cur_data;
	u8 *dst;

	spi_packet = spild->buff;
	ld = &spild->ld;

	/* check spi packet header */
	if (*(unsigned int *)spi_packet == 0x00000000
		|| *(unsigned int *)spi_packet == 0xFFFFFFFF) {
		/* if spi header is invalid, */
		/* read spi header again with next 4 byte */
		spi_packet += SPI_DATA_PACKET_HEADER_SIZE;
	}

	/* read spi packet header */
	spi_packet_header = (struct spi_data_packet_header *)spi_packet;
	spi_packet_length = SPI_DATA_PACKET_HEADER_SIZE +
		spi_packet_header->current_data_size;


	do {
		/* read spi data mux and set current queue */
		memcpy(&spi_data_mux,
			spi_packet + spi_packet_cur_pos, SPI_DATA_MUX_SIZE);

		switch (spi_data_mux & SPI_DATA_MUX_NORMAL_MASK) {
		case SPI_DATA_MUX_IPC:
			dev_id = IPC_FMT;
			break;

		case SPI_DATA_MUX_RAW:
			dev_id = IPC_RAW;
			break;

		case SPI_DATA_MUX_RFS:
			dev_id = IPC_RFS;
			break;

		default:
			pr_err("%s len[%u], pos[%u]\n",
				"[SPI] ERROR : spi_buff_read : MUX error",
				spi_packet_length, spi_packet_cur_pos);

			return spi_packet_cur_pos -
				SPI_DATA_PACKET_HEADER_SIZE;
		}

		/* read spi data length */
		memcpy(&spi_data_length, spi_packet +
			spi_packet_cur_pos + SPI_DATA_LENGTH_OFFSET,
			SPI_DATA_LENGTH_SIZE);

		data_length = spi_data_length + SPI_DATA_BOF_SIZE +
			SPI_DATA_EOF_SIZE;

		spi_data_length += SPI_DATA_HEADER_SIZE;

		/* read data and make spi data */
		spi_cur_data = spi_packet + spi_packet_cur_pos;

		/* enqueue spi data */
		skb = alloc_skb(data_length, GFP_ATOMIC);
		if (unlikely(!skb)) {
			pr_err("%s %s\n",
				"[SPI] ERROR : spi_buff_read:",
				"Can't allocate memory for SPI");
			return -ENOMEM;
		}

		dst = skb_put(skb, data_length);

		memcpy(dst, spi_packet +
			spi_packet_cur_pos + SPI_DATA_BOF_OFFSET,
			data_length);

		skb_queue_tail(&spild->skb_rxq[dev_id], skb);

		/* move spi packet current posision */
		spi_packet_cur_pos += spi_data_length;
	} while ((spi_packet_length - 1) > spi_packet_cur_pos);

	return 1;
}

static void spi_rx_work(void)
{
	struct link_device *ld;
	struct spi_link_device *spild;
	struct sk_buff *skb;
	struct io_device *iod;
	char *spi_packet_buf;
	char *spi_sync_buf;
	int  i;

	spild = p_spild;
	ld = &spild->ld;
	if (!spild)
		pr_err("[LNK/E] <%s> dpld == NULL\n", __func__);

	iod = link_get_iod_with_format(&spild->ld, IPC_FMT);
	if (!iod) {
		mif_err("no iodevice for modem control\n");
		return;
	}

	if (iod->mc->phone_state == STATE_CRASH_EXIT)
		return;

	if (!wake_lock_active(&spild->spi_wake_lock) ||
		gpio_get_value(spild->gpio_ipc_srdy) == SPI_GPIOLEVEL_LOW ||
		get_console_suspended() ||
		spild->spi_state == SPI_STATE_END)
		return;

	spild->spi_state = SPI_STATE_RX_WAIT;
	spild->spi_timer_rx_state = SPI_STATE_TIME_START;

	gpio_set_value(spild->gpio_ipc_sub_mrdy, SPI_GPIOLEVEL_HIGH);
	/* Start TX timer */
	spild->spi_rx_timer.expires =
		jiffies + ((SPI_TIMER_RX_WAIT_TIME * HZ) / 1000);
	add_timer(&spild->spi_rx_timer);

	/* check SUBSRDY state */
	while (gpio_get_value(spild->gpio_ipc_sub_srdy) ==
		SPI_GPIOLEVEL_LOW) {
		if (spild->spi_timer_rx_state == SPI_STATE_TIME_OVER) {
			pr_err("[SPI] ERROR(Failed MASTER RX:%d ms)",
			 SPI_TIMER_RX_WAIT_TIME);

			spild->spi_timer_rx_state = SPI_STATE_TIME_START;

			gpio_set_value(spild->gpio_ipc_sub_mrdy,
				SPI_GPIOLEVEL_LOW);

			/* change state SPI_MAIN_STATE_RX_WAIT */
			/* to SPI_STATE_IDLE */
			spild->spi_state = SPI_STATE_IDLE;

			return;
		}
	}
	/* Stop TX timer */
	del_timer(&spild->spi_rx_timer);

	if (spild->spi_state == SPI_STATE_START
		|| spild->spi_state == SPI_STATE_END
		|| spild->spi_state == SPI_STATE_INIT)
		return;

	spi_packet_buf = spild->buff;
	spi_sync_buf = spild->sync_buff;

	memset(spi_packet_buf, 0, SPI_MAX_PACKET_SIZE);
	memset(spi_sync_buf, 0, SPI_MAX_PACKET_SIZE);

	if (spi_tx_rx_sync((void *)spi_sync_buf, (void *)spi_packet_buf,
		SPI_MAX_PACKET_SIZE) == 0) {
		/* parsing SPI packet */
		if (spi_buff_read(spild) > 0) {
			/* call function for send data to IPC, RAW, RFS */
			for (i = 0; i < ld->max_ipc_dev; i++) {
				iod = spild->iod[i];
				while ((skb = skb_dequeue(&spild->skb_rxq[i]))
					 != NULL) {
					if (iod->recv(iod, ld, skb->data,
						skb->len) < 0)
						pr_err("[LNK/E] <%s:%s> recv fail\n",
							__func__, ld->name);
					dev_kfree_skb_any(skb);
				}
			}
		}
	} else {
		pr_err("%s %s\n", "[SPI] ERROR : spi_rx_work :",
			"spi sync failed");

		/* add cp reset when spi sync fail */
		if (iod)
			iod->modem_state_changed(iod,
					STATE_CRASH_RESET);
	}

	spild->spi_state = SPI_STATE_RX_TERMINATE;

	gpio_set_value(spild->gpio_ipc_sub_mrdy, SPI_GPIOLEVEL_LOW);

	/* change state SPI_MAIN_STATE_RX_WAIT to SPI_STATE_IDLE */
	spild->spi_state = SPI_STATE_IDLE;
	spi_start_data_send();
}


static int spi_init_ipc(struct spi_link_device *spild)
{
	struct link_device *ld = &spild->ld;

	int i;

	/* Make aliases to each IO device */
	for (i = 0; i < MAX_DEV_FORMAT; i++)
		spild->iod[i] = link_get_iod_with_format(ld, i);

	spild->iod[IPC_RAW] = spild->iod[IPC_MULTI_RAW];

	/* List up the IO devices connected to each IPC channel */
	for (i = 0; i < MAX_DEV_FORMAT; i++) {
		if (spild->iod[i])
			pr_err("[LNK] <%s:%s> spild->iod[%d]->name = %s\n",
				__func__, ld->name, i, spild->iod[i]->name);
		else
			pr_err("[LNK] <%s:%s> No spild->iod[%d]\n",
				__func__, ld->name, i);
	}

	ld->mode = LINK_MODE_IPC;

	return 0;
}

unsigned int sprd_crc_calc(char *buf_ptr, unsigned int len)
{
	unsigned int i;
	unsigned short crc = 0;

	while (len-- != 0) {
		for (i = 0x80; i != 0 ; i = i>>1) {
			if ((crc & 0x8000) != 0) {
				crc = crc << 1 ;
				crc = crc ^ 0x1021;
			} else {
				crc = crc << 1 ;
			}

			if ((*buf_ptr & i) != 0)
				crc = crc ^ 0x1021;
		}
		buf_ptr++;
	}

	return crc;

}

unsigned short sprd_crc_calc_fdl(unsigned short *src, int len)
{
	unsigned int sum = 0;
	unsigned short SourceValue, DestValue = 0;
	unsigned short lowSourceValue, hiSourceValue = 0;

	/* Get sum value of the source.*/
	while (len > 1) {
		SourceValue = *src++;
		DestValue	= 0;
		lowSourceValue = (SourceValue & 0xFF00) >> 8;
		hiSourceValue = (SourceValue & 0x00FF) << 8;
		DestValue = lowSourceValue | hiSourceValue;

		sum += DestValue;
		len -= 2;
	}

	if (len == 1)
		sum += *((unsigned char *) src);

	sum = (sum >> 16) + (sum & 0x0FFFF);
	sum += (sum >> 16);

	return ~sum;
}

int encode_msg(struct sprd_image_buf *img, int bcrc)
{
	u16	crc;				/* CRC value*/
	u8	*src_ptr;		   /* source buffer pointer*/
	int	dest_len;		   /* output buffer length*/
	u8	*dest_ptr;		   /* dest buffer pointer*/
	u8	high_crc, low_crc = 0;
	register int	curr;

	/* CRC Check. */
	src_ptr  = img->tx_b;

	/*	CRC Check. */
	if (bcrc)
		crc = sprd_crc_calc(src_ptr, img->tx_size);
	else
		crc  = sprd_crc_calc_fdl
		((unsigned short *)src_ptr, img->tx_size);

	high_crc = (crc>>8) & 0xFF;
	low_crc  = crc & 0xFF;

	/* Get the total size to be allocated.*/
	dest_len = 0;

	for (curr = 0; curr < img->tx_size; curr++) {
		switch (*(src_ptr+curr)) {
		case HDLC_FLAG:
		case HDLC_ESCAPE:
			dest_len += 2;
			break;
		default:
			dest_len++;
			break;
		}
	}

	switch (low_crc) {
	case HDLC_FLAG:
	case HDLC_ESCAPE:
		dest_len += 2;
		break;
	default:
		dest_len++;
	}

	switch (high_crc) {
	case HDLC_FLAG:
	case HDLC_ESCAPE:
		dest_len += 2;
		break;
	default:
		dest_len++;
	}

	dest_ptr = kmalloc(dest_len + 2, GFP_ATOMIC);
	/* Memory Allocate fail.*/
	if (dest_ptr == NULL)
		return -ENOMEM;

	*dest_ptr = HDLC_FLAG;
	dest_len  = 1;

	/* do escape*/
	for (curr = 0; curr < img->tx_size; curr++) {
		switch (*(src_ptr+curr)) {
		case HDLC_FLAG:
		case HDLC_ESCAPE:
			*(dest_ptr + dest_len++) = HDLC_ESCAPE;
			*(dest_ptr + dest_len++) =
				*(src_ptr + curr) ^ HDLC_ESCAPE_MASK;
			break;
		default:
			*(dest_ptr + dest_len++) = *(src_ptr + curr);
			break;
		}
	}

	switch (high_crc) {
	case HDLC_FLAG:
	case HDLC_ESCAPE:
		*(dest_ptr + dest_len++) = HDLC_ESCAPE;
		*(dest_ptr + dest_len++) = high_crc ^ HDLC_ESCAPE_MASK;
		break;
	default:
		*(dest_ptr + dest_len++) = high_crc;
	}

	switch (low_crc) {
	case HDLC_FLAG:
	case HDLC_ESCAPE:
		*(dest_ptr + dest_len++) = HDLC_ESCAPE;
		*(dest_ptr + dest_len++) = low_crc ^ HDLC_ESCAPE_MASK;
		break;
	default:
		*(dest_ptr + dest_len++) = low_crc;
	}


	*(dest_ptr + dest_len++) = HDLC_FLAG;

	memcpy(img->encoded_tx_b, dest_ptr, dest_len);
	img->encoded_tx_size = dest_len;

	kfree(dest_ptr);
	return 0;
}

int decode_msg(struct sprd_image_buf *img, int bcrc)
{
	u16	crc;	/* CRC value*/
	u8	*src_ptr;	/* source buffer pointer*/
	int	dest_len;	/* output buffer length*/
	u8	*dest_ptr;	/* dest buffer pointer*/
	register int		curr;

	/* Check if exist End Flag.*/
	src_ptr = img->rx_b;

	dest_len = 0;

	if (img->rx_size < 4)
		return -EINVAL;

	/* Get the total size to be allocated for decoded message.*/
	for (curr = 1; curr < img->rx_size - 1; curr++) {
		switch (*(src_ptr + curr)) {
		case HDLC_ESCAPE:
			curr++;
			dest_len++;
			break;
		default:
			dest_len++;
			break;
		}
	}

	/* Allocate meomory for decoded message*/
	dest_ptr = kmalloc(dest_len, GFP_ATOMIC);
	/* Memory allocate fail.*/
	if (dest_ptr == NULL)
		return -ENOMEM;

	memset(dest_ptr, 0, dest_len);

	curr = 0;
	dest_len = 0;
	/* Do de-escape.*/
	for (curr = 1; curr < img->rx_size - 1; curr++) {
		switch (*(src_ptr + curr)) {
		case HDLC_ESCAPE:
			curr++;
			*(dest_ptr + dest_len) =
				*(src_ptr + curr) ^ HDLC_ESCAPE_MASK;
			break;
		default:
			*(dest_ptr + dest_len) = *(src_ptr + curr);
			break;
		}

		dest_len = dest_len + 1;
	}

	/*	CRC Check. */
	if (bcrc)
		crc = sprd_crc_calc(dest_ptr, dest_len);
	else
		crc  = sprd_crc_calc_fdl((unsigned short *)dest_ptr, dest_len);

	if (crc != CRC_16_L_OK) {
		pr_err("CRC error : 0x%X", crc);
		kfree(dest_ptr);
		return -EPERM;
	}

	memcpy(img->decoded_rx_b, dest_ptr, dest_len - CRC_CHECK_SIZE);
	img->decoded_rx_size = dest_len - CRC_CHECK_SIZE ;

	kfree(dest_ptr);
	return 0;
}

static int spi_send_modem_bin_execute_cmd
	(u8 *spi_ptr, u32 spi_size, u16 spi_type,
		u16 spi_crc, struct sprd_image_buf *sprd_img)
{
	int i, retval;
	u16 send_packet_size;
	u8 *send_packet_data;
	u16 d1_crc;
	u16 d2_crc = spi_crc;
	u16 type = spi_type;

	/* D1 */
	send_packet_size = spi_size;	/* u32 -> u16 */
	sprd_img->tx_size = 6;
	M_16_SWAP(d2_crc);
	memcpy(sprd_img->tx_b, &send_packet_size, sizeof(send_packet_size));
	memcpy((sprd_img->tx_b+2), &type, sizeof(type));
	memcpy((sprd_img->tx_b+4), &d2_crc, sizeof(d2_crc));

	d1_crc = sprd_crc_calc_fdl
		((unsigned short *)sprd_img->tx_b, sprd_img->tx_size);
	M_16_SWAP(d1_crc);
	memcpy((sprd_img->tx_b+6), &d1_crc, sizeof(d1_crc));
	sprd_img->tx_size += 2;

	if (down_timeout(&p_spild->srdy_sem, 2 * HZ)) {
		pr_err("(%d) SRDY TimeOUT!!! SRDY : %d, SEM : %d\n",
			__LINE__,
			gpio_get_value(p_spild->gpio_modem_bin_srdy),
			p_spild->srdy_sem.count);
		goto err;
	}

	retval = spi_tx_rx_sync
		(sprd_img->tx_b, sprd_img->rx_b, sprd_img->tx_size);
	if (retval != 0) {
		pr_err("(%d) spi sync error : %d\n",
			__LINE__, retval);
		goto err;
	}

	if ((type == 0x0003) || (type == 0x0004)) {
		pr_err("D2 Skip!!\n");
		goto ACK;
	}

	/* D2 */
	send_packet_data = spi_ptr;

	if (down_timeout(&p_spild->srdy_sem, 2 * HZ)) {
		pr_err("(%d) SRDY TimeOUT!!! SRDY : %d, SEM : %d\n", __LINE__,
			gpio_get_value(p_spild->gpio_modem_bin_srdy),
			p_spild->srdy_sem.count);
		goto err;
	}

	retval = spi_tx_rx_sync
		(send_packet_data, sprd_img->rx_b, send_packet_size);
	if (retval != 0) {
		pr_err("(%d) spi sync error : %d\n", __LINE__, retval);
		goto err;
	}

ACK:

	if (p_spild->is_cp_reset) {
		while (!gpio_get_value(p_spild->gpio_modem_bin_srdy))
			;
	} else {
		if (down_timeout(&p_spild->srdy_sem, 2 * HZ)) {
			pr_err("(%d) SRDY TimeOUT!!! SRDY : %d, SEM : %d\n",
				__LINE__,
				gpio_get_value(p_spild->gpio_modem_bin_srdy),
				p_spild->srdy_sem.count);

			pr_err("[SPI DUMP] TX_D1(%d) : [%02x %02x %02x %02x %02x %02x %02x %02x]\n",
				sprd_img->tx_size, sprd_img->tx_b[0],
				sprd_img->tx_b[1], sprd_img->tx_b[2],
				sprd_img->tx_b[3], sprd_img->tx_b[4],
				sprd_img->tx_b[5], sprd_img->tx_b[6],
				sprd_img->tx_b[7]);
			pr_err("[SPI DUMP] TX_D2(%d) :[%02x %02x %02x %02x %02x %02x %02x %02x]\n",
				send_packet_size, spi_ptr[0], spi_ptr[1],
				spi_ptr[2], spi_ptr[3], spi_ptr[4],
				spi_ptr[5], spi_ptr[6], spi_ptr[7]);

		/* WA (CP Reset) jongmoon.suh */
		if (gpio_get_value(p_spild->gpio_modem_bin_srdy))
			;
		else
			goto err;

	}
	}

	memset(sprd_img->tx_b, 0, SPRD_BLOCK_SIZE+10);
	retval = spi_tx_rx_sync(sprd_img->tx_b, sprd_img->rx_b, 8);
	if (retval != 0) {
		pr_err("(%d) spi sync error : %d\n",
			__LINE__, retval);
		goto err;
	}

	memcpy(sprd_img->decoded_rx_b, sprd_img->rx_b, 4);

	if ((*(sprd_img->decoded_rx_b+0) == 0x00) && \
		(*(sprd_img->decoded_rx_b+1) == 0x80) && \
		(*(sprd_img->decoded_rx_b+2) == 0x00) && \
		(*(sprd_img->decoded_rx_b+3) == 0x00)) {
		pr_debug("[SPRD] CP sent ACK");
	} else {
		pr_err("Transfer ACK error! srdy_sem = %d\n",
			p_spild->srdy_sem.count);
		pr_err("[SPI DUMP] RX(%d) : [ ", sprd_img->rx_size);
		for (i = 0; i < 15; i++)
			pr_err("%02x ", *((u8 *)(sprd_img->rx_b + i)));
		pr_err("]");

		goto err;
	}

	return retval;

err:
	return -EINVAL;
}

static int spi_send_modem_bin_xmit_img
	(enum image_type type, struct image_buf *img)
{
	int retval = 0;
	struct sprd_image_buf sprd_img;
	unsigned int data_size;
	unsigned int send_size = 0;
	unsigned int rest_size = 0;
	unsigned int spi_size = 0;
	unsigned int address = 0;
	unsigned int fdl1_size = 0;
	/* No Translate */
	u16 crc = 0;
	u16 spi_type = 0;

	unsigned char *spi_ptr;
	unsigned char *ptr;
	int i, j = 0;

	u16 sprd_packet_size = SPRD_BLOCK_SIZE;

	sprd_img.tx_b = kmalloc(SPRD_BLOCK_SIZE*2, GFP_ATOMIC);
	if (!sprd_img.tx_b) {
		pr_err("(%d) tx_b kmalloc fail.",
			__LINE__);
		return -ENOMEM;
	}
	memset(sprd_img.tx_b, 0, SPRD_BLOCK_SIZE*2);

	sprd_img.rx_b = kmalloc(SPRD_BLOCK_SIZE*2, GFP_ATOMIC);
	if (!sprd_img.rx_b) {
		pr_err("(%d) rx_b kmalloc fail.",
			__LINE__);
		retval = -ENOMEM;
		goto err3;
	}
	memset(sprd_img.rx_b, 0, SPRD_BLOCK_SIZE*2);

	sprd_img.encoded_tx_b = kmalloc(SPRD_BLOCK_SIZE*2, GFP_ATOMIC);
	if (!sprd_img.encoded_tx_b) {
		pr_err("(%d) encoded_tx_b kmalloc fail.",
			__LINE__);
		retval = -ENOMEM;
		goto err2;
	}
	memset(sprd_img.encoded_tx_b, 0, SPRD_BLOCK_SIZE*2);

	sprd_img.decoded_rx_b = kmalloc(SPRD_BLOCK_SIZE*2, GFP_ATOMIC);
	if (!sprd_img.decoded_rx_b) {
		pr_err("(%d) encoded_rx_b kmalloc fail.",
			__LINE__);
		retval = -ENOMEM;
		goto err1;
	}
	memset(sprd_img.decoded_rx_b, 0, SPRD_BLOCK_SIZE*2);

	pr_debug("(%d) spi_send_modem_bin_xmit_img type : %d.\n",
		__LINE__, type);
	memcpy(&fdl1_size, (void *)(p_spild->p_virtual_buff + 4), 4);

	switch (type) {
	case MODEM_MAIN:
		memcpy(&img->address,
			(void *)(p_spild->p_virtual_buff + 8), 4);
		memcpy(&img->length,
			(void *)(p_spild->p_virtual_buff + 12), 4);
		img->buf  = (unsigned char *)
			(p_spild->p_virtual_buff + 0x30 + fdl1_size);
		img->offset = img->length + fdl1_size + 0x30;
		pr_debug("(%d) spi_send_modem_bin_xmit_img save MAIN to img.\n",
			__LINE__);

		break;

	case MODEM_DSP:
		memcpy(&img->address,
			(void *)(p_spild->p_virtual_buff + 16), 4);
		memcpy(&img->length,
			(void *)(p_spild->p_virtual_buff + 20), 4);
		img->buf = (unsigned char *)
			(p_spild->p_virtual_buff + img->offset);
		img->offset += img->length;
		pr_debug("(%d) spi_send_modem_bin_xmit_img save DSP to img.\n",
			__LINE__);

		break;

	case MODEM_NV:
		memcpy(&img->address,
			(void *)(p_spild->p_virtual_buff + 24), 4);
		memcpy(&img->length,
			(void *)(p_spild->p_virtual_buff + 28), 4);
		img->buf = (unsigned char *)
			(p_spild->p_virtual_buff + img->offset);
		img->offset += img->length;
		pr_debug("(%d) spi_send_modem_bin_xmit_img save NV to img.\n",
			__LINE__);

		break;

	case MODEM_EFS:
		memcpy(&img->address,
			(void *)(p_spild->p_virtual_buff + 32), 4);
		memcpy(&img->length,
			(void *)(p_spild->p_virtual_buff + 36), 4);
		img->buf = (unsigned char *)
			(p_spild->p_virtual_buff + img->offset);
		img->offset += img->length;
		pr_debug("(%d) spi_send_modem_bin_xmit_img save EFS to img.\n",
			__LINE__);

		break;
	case MODEM_RUN:
		memset(sprd_img.encoded_tx_b, 0, SPRD_BLOCK_SIZE*2);
		sprd_img.encoded_tx_size = 0;
		spi_type = 0x0004;
		crc = 0;

		spi_ptr = sprd_img.encoded_tx_b;
		spi_size = sprd_img.encoded_tx_size;

		retval = spi_send_modem_bin_execute_cmd
			(spi_ptr, spi_size, spi_type, crc, &sprd_img);
		if (retval < 0) {
			pr_err("(%d) spi_send_modem_bin_execute_cmd fail : %d",
				__LINE__, retval);
			goto err0;
		}
		return retval;

	default:
		pr_err("(%d) spi_send_modem_bin_xmit_img wrong : %d.",
			__LINE__, type);
		goto err0;
	}

	pr_debug("(%d) Start send img. size : %d\n",
		__LINE__, img->length);

	ptr = img->buf;
	data_size = sprd_packet_size;
	rest_size = img->length;
	address = img->address;

	M_32_SWAP(img->address);
	M_32_SWAP(img->length);

	/* Send Transfer Start */
	sprd_img.tx_size = 8;
	memcpy((sprd_img.tx_b+0), &img->address, sizeof(img->address));
	memcpy((sprd_img.tx_b+4), &img->length, sizeof(img->length));

	spi_type = 0x0001;
	crc = sprd_crc_calc_fdl
		((unsigned short *)sprd_img.tx_b, sprd_img.tx_size);
	memcpy(sprd_img.encoded_tx_b, sprd_img.tx_b, sprd_img.tx_size);
	sprd_img.encoded_tx_size = sprd_img.tx_size;

	spi_ptr = sprd_img.encoded_tx_b;
	spi_size = sprd_img.encoded_tx_size;

	pr_debug("(%d) [Transfer Start, Type = %d, Packet = %d]\n",
		__LINE__, type, sprd_packet_size);
	retval = spi_send_modem_bin_execute_cmd
		(spi_ptr, spi_size, spi_type, crc, &sprd_img);
	if (retval < 0) {
		pr_err("(%d) spi_send_modem_bin_execute_cmd fail : %d",
			__LINE__, retval);
		goto err0;
	}
	M_32_SWAP(img->length);

	/* Send Data */
	for (i = 0; send_size < img->length; i++) {
		if (rest_size < sprd_packet_size)
			data_size = rest_size;

		sprd_img.encoded_tx_size = sprd_packet_size;
		for (j = 0; j < data_size; j++)
			*(sprd_img.encoded_tx_b+j) = *(ptr + j);

		spi_type = 0x0002;
		crc = sprd_crc_calc_fdl
			((unsigned short *)sprd_img.encoded_tx_b,
			sprd_img.encoded_tx_size);

		spi_ptr = sprd_img.encoded_tx_b;
		spi_size = sprd_img.encoded_tx_size;

		retval = spi_send_modem_bin_execute_cmd
			(spi_ptr, spi_size, spi_type, crc, &sprd_img);
		if (retval < 0) {
			pr_err("(%d) spi_send_modem_bin_execute_cmd fail : %d, %d",
				__LINE__, retval, i);
			goto err0;
		}

		send_size += data_size;
		rest_size -= data_size;
		ptr += data_size;

		if (!(i % 100))
			pr_debug("(%d) [%d] 0x%x size done, rest size: 0x%x\n",
				__LINE__, i, send_size, rest_size);
	}

	/* Send Transfer End */
	memset(sprd_img.encoded_tx_b, 0, SPRD_BLOCK_SIZE * 2);
	sprd_img.encoded_tx_size = 0;

	spi_type = 0x0003;
	crc = 0;

	spi_ptr = sprd_img.encoded_tx_b;
	spi_size = sprd_img.encoded_tx_size;

	pr_debug("(%d) [Transfer END]\n", __LINE__);
	retval = spi_send_modem_bin_execute_cmd
		(spi_ptr, spi_size, spi_type, crc, &sprd_img);
	if (retval < 0) {
		pr_err("(%d) spi_send_modem_bin_execute_cmd fail : %d",
			__LINE__, retval);
		goto err0;
	}

err0:
	kfree(sprd_img.decoded_rx_b);
err1:
	kfree(sprd_img.encoded_tx_b);
err2:
	kfree(sprd_img.rx_b);
err3:
	kfree(sprd_img.tx_b);

	return retval;

}

static void spi_send_modem_bin(struct work_struct *send_modem_w)
{
	struct spi_link_device *spild;
	struct io_device *iod;
	int retval;
	struct image_buf img;
	unsigned long tick1, tick2 = 0;

	spild = p_spild;
	iod = link_get_iod_with_format(&spild->ld, IPC_FMT);
	if (!iod) {
		mif_err("no iodevice for modem control\n");
		return;
	}

	tick1 = jiffies_to_msecs(jiffies);

	retval = spi_send_modem_bin_xmit_img(MODEM_MAIN, &img);
	if (retval < 0) {
		pr_err("(%d) spi_send_modem_bin_xmit_img fail : %d",
			__LINE__, retval);
		goto err;
	}

	retval = spi_send_modem_bin_xmit_img(MODEM_DSP, &img);
	if (retval < 0) {
		pr_err("(%d) spi_send_modem_bin_xmit_img fail : %d",
			__LINE__, retval);
		goto err;
	}

	retval = spi_send_modem_bin_xmit_img(MODEM_NV, &img);
	if (retval < 0) {
		pr_err("(%d) spi_send_modem_bin_xmit_img fail : %d",
			__LINE__, retval);
		goto err;
	}

	retval = spi_send_modem_bin_xmit_img(MODEM_EFS, &img);
	if (retval < 0) {
		pr_err("(%d) spi_send_modem_bin_xmit_img fail : %d",
			__LINE__, retval);
		goto err;
	}

	retval = spi_send_modem_bin_xmit_img(MODEM_RUN, &img);
	if (retval < 0) {
		pr_err("(%d) spi_send_modem_bin_xmit_img fail : %d",
			__LINE__, retval);
		goto err;
	}

	p_spild->send_modem_spi = 0;
	p_spild->is_cp_reset = 0;
	tick2 = jiffies_to_msecs(jiffies);
	pr_info("Downloading takes %lu msec\n", (tick2-tick1));

	spi_init_ipc(p_spild);

	sprd_boot_done = 1;
	p_spild->ril_send_cnt = 0;
	p_spild->spi_state = SPI_STATE_IDLE;

	if (iod)
		iod->modem_state_changed(iod,
			STATE_ONLINE);
	return;
err:
	if (iod)
		iod->modem_state_changed(iod,
			STATE_OFFLINE);
	return;

}

static inline int _request_mem(struct spi_v_buff *od)
{
	if (!p_spild->p_virtual_buff) {
		od->mmio = vmalloc(od->size);
		if (!od->mmio) {
			pr_err("(%d) Failed to vmalloc size : %lu\n", __LINE__,
				od->size);

			return -EBUSY;
		} else {
			pr_err("(%d) vmalloc Done. mmio : 0x%08x\n", __LINE__,
				(u32)od->mmio);
		}
	}

	memset((void *)od->mmio, 0, od->size);

	p_spild->p_virtual_buff = od->mmio;

	return 0;
}

void spi_tx_timer_callback(unsigned long param)
{
	if (p_spild->spi_state == SPI_STATE_TX_WAIT) {
		p_spild->spi_timer_tx_state = SPI_STATE_TIME_OVER;
		pr_debug("[SPI] spi_tx_timer_callback -timer expires\n");
	}
}

void spi_rx_timer_callback(unsigned long param)
{
	if (p_spild->spi_state == SPI_STATE_RX_WAIT) {
		p_spild->spi_timer_rx_state = SPI_STATE_TIME_OVER;
		pr_debug("[SPI] spi_rx_timer_callback -timer expires\n");
	}
}

int spi_sema_init(void)
{
	pr_info("[SPI] Srdy sema init\n");
	sema_init(&p_spild->srdy_sem, 0);
	p_spild->send_modem_spi = 1;
	return 0;
}

static void spi_work(struct work_struct *work)
{
	int signal_code;

	struct spi_work_type *spi_wq =
		container_of(work, struct spi_work_type, work);

	signal_code = spi_wq->signal_code;

	if (p_spild->spi_state == SPI_STATE_END
		|| p_spild->spi_state == SPI_STATE_START) {
		kfree(spi_wq);
		return;
	}

	switch (signal_code) {
	case SPI_WORK_SEND:
		if (p_spild->spi_state == SPI_STATE_IDLE)
			spi_tx_work();
		break;
	case SPI_WORK_RECEIVE:
		if (p_spild->spi_state == SPI_STATE_IDLE
			|| p_spild->spi_state == SPI_STATE_TX_TERMINATE
			|| p_spild->spi_state == SPI_STATE_RX_TERMINATE)
				spi_rx_work();
		break;

	default:
		pr_err("[SPI] ERROR(No signal_code in spi_work[%d])\n",
			signal_code);
		break;
	}

	kfree(spi_wq);
	if (wake_lock_active(&p_spild->spi_wake_lock)) {
		wake_unlock(&p_spild->spi_wake_lock);
		pr_debug("[SPI] [%s](%d) spi_wakelock unlocked .\n",
			__func__, __LINE__);
	}
}

static int link_pm_notifier_event(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	struct io_device *iod;
	struct link_pm_data *pm_data =
			container_of(this, struct link_pm_data,	pm_notifier);

	iod = link_get_iod_with_format(&pm_data->spild->ld, IPC_FMT);
	if (!iod) {
		pr_err("no iodevice for modem control\n");
		return NOTIFY_BAD;
	}

	if (!gpio_get_value(iod->mc->gpio_phone_active))
		return NOTIFY_DONE;

	switch (event) {
	case PM_SUSPEND_PREPARE:
		/* set TD PDA Active High if previous state was LPA */
		mif_info("TD PDA active low to LPA suspend spot\n");
		gpio_set_value(iod->mc->gpio_pda_active, 0);

		return NOTIFY_OK;
	case PM_POST_SUSPEND:
		/* LPA to Kernel suspend and User Freezing task fail resume,
		restore to LPA GPIO states. */
		mif_info("TD PDA active High to LPA GPIO state\n");
		gpio_set_value(iod->mc->gpio_pda_active, 1);
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static int spi_probe(struct spi_device *spi)
{
	int ret;

	p_spi = spi;
	p_spi->mode = SPI_MODE_1;
	p_spi->bits_per_word = 32;

	ret = spi_setup(p_spi);
	if (ret != 0) {

		pr_err("[%s] spi setup error\n", __func__);

		return ret;
	}

	pr_info("[%s] spi probe done\n", __func__);

	return ret;
}

static int spi_remove(struct spi_device *spi)
{
	return 0;
}

static struct spi_driver spi_driver = {
	.probe = spi_probe,
	.remove = __devexit_p(spi_remove),
	.driver = {
		.name = "modem_if_spi",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
	},
};

static int spi_link_init(void)
{
	int ret;
	struct spi_v_buff *od;
	struct link_device *ld = &p_spild->ld;

	p_spild->gpio_modem_bin_srdy = p_spild->gpio_ipc_srdy;

	pr_info("(%d) gpio_mrdy : %d, gpio_srdy : %d(%d)\n", __LINE__,
		p_spild->gpio_ipc_mrdy, p_spild->gpio_modem_bin_srdy,
		gpio_get_value(p_spild->gpio_ipc_srdy));

	od = kzalloc(sizeof(struct spi_v_buff), GFP_KERNEL);
	if (!od) {
		pr_err("(%d) failed to allocate device\n", __LINE__);
		ret = -ENOMEM;
		goto err;
	}

	od->base = 0;
	od->size = SZ_16M; /* 16M */
	if (p_spild->p_virtual_buff)
		od->mmio = p_spild->p_virtual_buff;
	ret = _request_mem(od);
	if (ret)
		goto err;

	sema_init(&p_spild->srdy_sem, 0);

	INIT_WORK(&p_spild->send_modem_w,
		spi_send_modem_bin);

	wake_lock_init(&p_spild->spi_wake_lock,
		       WAKE_LOCK_SUSPEND,
		       "samsung-spiwakelock");

	/* Register SPI Srdy interrupt handler */
	ret = spi_register_isr(gpio_to_irq(p_spild->gpio_ipc_srdy),
		spi_srdy_irq_handler, IRQF_TRIGGER_RISING,
		"spi_srdy_rising", ld);

	if (ret)
		goto err;

	/* Register SPI SubSrdy interrupt handler */
	ret = spi_register_isr(gpio_to_irq(p_spild->gpio_ipc_sub_srdy),
		spi_subsrdy_irq_handler, IRQF_TRIGGER_RISING,
		"spi_subsrdy_rising", ld);

	if (ret)
		goto err;

	p_spild->boot_done = 1;

	pr_info("[%s] Done\n", __func__);
	return 0;

err:
	kfree(od);
	return ret;
}

/*=====================================
* Description	spi restart for CP slient reset
=====================================*/
void spi_set_restart(void)
{
	struct link_device *ld = &p_spild->ld;

	pr_info("[SPI] spi_set_restart(spi_main_state=[%d])\n",
		(int)p_spild->spi_state);

	gpio_set_value(p_spild->gpio_ipc_sub_mrdy, SPI_GPIOLEVEL_LOW);

	p_spild->spi_state = SPI_STATE_END;

	/* Flush SPI work queue */
	flush_workqueue(p_spild->spi_wq);

	/* Unregister SPI Srdy interrupt handler */
	spi_unregister_isr(gpio_to_irq(p_spild->gpio_ipc_srdy), ld);

	/* Unregister SPI SubSrdy interrupt handler */
	spi_unregister_isr(gpio_to_irq(p_spild->gpio_ipc_sub_srdy), ld);

	if (wake_lock_active(&p_spild->spi_wake_lock)) {
		wake_unlock(&p_spild->spi_wake_lock);
		pr_err("[SPI] [%s](%d) spi_wakelock unlocked.\n",
			__func__, __LINE__);
	}
	wake_lock_destroy(&p_spild->spi_wake_lock);
}


int spi_thread_restart(void)
{
	int retval;

	p_spild->send_modem_spi = 1;
	p_spild->is_cp_reset = 1;
	sprd_boot_done = 0;

	pr_info("[modem_if_spi] spi_thread_restart\n");

	spi_set_restart();

	/* Create SPI device */
	retval = spi_link_init();
	if (retval)
		goto exit;

	return 0;

exit:
	return retval;
}
EXPORT_SYMBOL(spi_thread_restart);

static int spi_link_pm_init(struct spi_link_device *spild,
	struct platform_device *pdev)
{
	struct modem_data *pdata =
			(struct modem_data *)pdev->dev.platform_data;
	struct modemlink_pm_data *pm_pdata;
	struct link_pm_data *pm_data =
			kzalloc(sizeof(struct link_pm_data), GFP_KERNEL);

	if (!pdata || !pdata->link_pm_data) {
		mif_err("platform data is NULL\n");
		return -EINVAL;
	}
	pm_pdata = pdata->link_pm_data;

	if (!pm_data) {
		mif_err("link_pm_data is NULL\n");
		return -ENOMEM;
	}

	pm_data->spild = spild;
	spild->link_pm_data = pm_data;

	pm_data->pm_notifier.notifier_call = link_pm_notifier_event;
	register_pm_notifier(&pm_data->pm_notifier);

	return 0;
}

struct link_device *spi_create_link_device(struct platform_device *pdev)
{
	struct spi_link_device *spild = NULL;
	struct link_device *ld;
	struct modem_data *pdata;

	int ret;
	int i;

	/* Get the platform data */
	pdata = (struct modem_data *)pdev->dev.platform_data;
	if (!pdata) {
		pr_err("[LNK/E] <%s> pdata == NULL\n", __func__);
		goto err;
	}

	pr_info("[LNK] <%s> link device = %s\n", __func__, pdata->link_name);
	pr_info("[LNK] <%s> modem = %s\n", __func__, pdata->name);

	/* Alloc SPI link device structure */
	p_spild = spild = kzalloc(sizeof(struct spi_link_device), GFP_KERNEL);
	if (!spild) {
		pr_err("[LNK/E] <%s> Failed to kzalloc()\n", __func__);
		goto err;
	}
	ld = &spild->ld;

	/* Extract modem data and SPI control data from the platform data */
	ld->mdm_data = pdata;
	ld->name     = "spi";

	if (ld->aligned)
		pr_err("[LNK] <%s> Aligned access is required!!!\n", __func__);


	ld->send = spi_send;

	INIT_LIST_HEAD(&ld->list);

	skb_queue_head_init(&ld->sk_fmt_tx_q);
	skb_queue_head_init(&ld->sk_raw_tx_q);
	skb_queue_head_init(&ld->sk_rfs_tx_q);
	ld->skb_txq[IPC_FMT] = &ld->sk_fmt_tx_q;
	ld->skb_txq[IPC_RAW] = &ld->sk_raw_tx_q;
	ld->skb_txq[IPC_RFS] = &ld->sk_rfs_tx_q;

	spild->spi_wq = create_singlethread_workqueue("spi_wq");
	if (!spild->spi_wq) {
		pr_err("[LNK/E] <%s> Fail to create workqueue for spi_wq\n",
			__func__);
		goto err;
	}

	spild->spi_state = SPI_STATE_END;
	ld->max_ipc_dev = (IPC_RFS + 1);	/* FMT, RAW, RFS */

	for (i = 0; i < ld->max_ipc_dev; i++)
		skb_queue_head_init(&spild->skb_rxq[i]);

	/* Prepare a clean buffer for SPI access */
	spild->buff = kzalloc(SPI_MAX_PACKET_SIZE, GFP_KERNEL);
	spild->sync_buff = kzalloc(SPI_MAX_PACKET_SIZE, GFP_KERNEL);

	memset(spild->buff , 0, SPI_MAX_PACKET_SIZE);
	memset(spild->sync_buff , 0, SPI_MAX_PACKET_SIZE);

	if (!spild->buff) {
		pr_err("[LNK/E] <%s> Failed to alloc spild->buff\n", __func__);
		goto err;
	}

	/* Initialize SPI pin value */
	spild->gpio_ipc_mrdy = pdata->gpio_ipc_mrdy;
	spild->gpio_ipc_srdy = pdata->gpio_ipc_srdy;
	spild->gpio_ipc_sub_mrdy = pdata->gpio_ipc_sub_mrdy;
	spild->gpio_ipc_sub_srdy = pdata->gpio_ipc_sub_srdy;

	spild->gpio_ap_cp_int1 = pdata->gpio_ap_cp_int1;
	spild->gpio_ap_cp_int2 = pdata->gpio_ap_cp_int2;

	/* Create SPI Timer */
	init_timer(&spild->spi_tx_timer);

	spild->spi_tx_timer.expires =
		jiffies + ((SPI_TIMER_TX_WAIT_TIME * HZ) / 1000);

	spild->spi_tx_timer.data = 0;
	spild->spi_tx_timer.function = spi_tx_timer_callback;

	init_timer(&spild->spi_rx_timer);

	spild->spi_rx_timer.expires =
		jiffies + ((SPI_TIMER_RX_WAIT_TIME * HZ) / 1000);

	spild->spi_rx_timer.data = 0;
	spild->spi_rx_timer.function = spi_rx_timer_callback;

	ret = spi_register_driver(&spi_driver);
	if (ret < 0) {
		pr_err("spi_register_driver() fail : %d\n", ret);
		goto err;
	}

	/* create link pm device */
	ret = spi_link_pm_init(spild, pdev);
	if (ret)
		goto err;

	/* Create SPI device */
	ret = spi_link_init();
	if (ret)
		goto err;

	/* creat work queue thread */
	p_spild->ipc_spi_wq = create_singlethread_workqueue("ipc_spi_wq");

	if (!p_spild->ipc_spi_wq) {
		pr_err("[%s] get workqueue thread fail\n",
			__func__);

		ret = -ENOMEM;
		goto err;
	}

	return ld;

err:
	kfree(spild->buff);
	kfree(spild);
	return NULL;
}
