/*
 * Copyright (C) 2011 Samsung Electronics.
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

#include "wimax_sdio.h"
#include "firmware.h"
#include "wimax_i2c.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/suspend.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/pm.h>
#include <linux/mmc/sdio_func.h>
#include <asm/byteorder.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/wimax/samsung/wimax732.h>
#include <linux/kthread.h>

/* driver Information */
#define WIMAX_DRIVER_VERSION_STRING "3.0.6"
#define DRIVER_AUTHOR "Samsung"
#define DRIVER_DESC "Samsung WiMAX SDIO Device Driver"
/* use ethtool to change the level for any given device */
static int msg_level = -1;
module_param(msg_level, int, 0);

static int hw_sdio_tx_bank_index(struct net_adapter *adapter, int *write_idx)
{
	int ret = 0;

	*write_idx = sdio_readb(adapter->func, SDIO_H2C_WP_REG, &ret);
	if (ret)
		return ret;

	if (((*write_idx + 1) % 15) == sdio_readb(adapter->func,
							SDIO_H2C_RP_REG, &ret))
		*write_idx = -1;

	return ret;
}

static void adapter_interrupt(struct sdio_func *func);
static void sdio_error(struct net_adapter *adapter)
{
	if ((adapter->sdio_error_count++) > MAX_SDIO_ERROR)
		pr_err("Unable to recover from SDIO failure");
		schedule_work(&adapter->pdata->g_cfg->shutdown);
}

bool sd_send(struct net_adapter *adapter, u8 *buffer, u32 len)
{
	int nRet;
	int nWriteIdx;

	/*round off len to even*/
	(len & 1) ? len++ : 0;

	sdio_claim_host(adapter->func);
	nRet = hw_sdio_tx_bank_index(adapter, &nWriteIdx);


	if (unlikely(nRet))
		goto sdio_error;

	if (unlikely(nWriteIdx == -1)) {
		pr_err("modem buffer full, skipping packet write");
		goto skip;
		}
	sdio_writeb(adapter->func, (nWriteIdx + 1) % 15, SDIO_H2C_WP_REG, NULL);
	nRet = sdio_memcpy_toio(adapter->func,
			SDIO_TX_BANK_ADDR+(CMC732_SDIO_BANK_SIZE * nWriteIdx)+
				CMC732_PACKET_LENGTH_SIZE, buffer, len);
	if (unlikely(nRet < 0))
		goto sdio_error;
	nRet = sdio_memcpy_toio(adapter->func,
			SDIO_TX_BANK_ADDR + (CMC732_SDIO_BANK_SIZE * nWriteIdx),
			&len, CMC732_PACKET_LENGTH_SIZE);
	if (unlikely(nRet < 0))
		goto sdio_error;
skip:
	sdio_release_host(adapter->func);
	/*Reset SDIO error count once it is functional again*/
	adapter->sdio_error_count = 0;
	return false;
sdio_error:
	pr_err("SDIO error");
	if (adapter->pdata->g_cfg->power_state == CMC_POWER_ON)
		sdio_error(adapter);
	sdio_release_host(adapter->func);
	return true;
}

bool send_cmd_packet(struct net_adapter *adapter, u16 cmd_id)
{
	u8			tx_buf[CMD_MSG_TOTAL_LENGTH];

	((struct hw_packet_header *)tx_buf)->id0 = 'W';
	((struct hw_packet_header *)tx_buf)->id1 = 'C';
	((struct hw_packet_header *)tx_buf)->length =
				be16_to_cpu(CMD_MSG_TOTAL_LENGTH);
	((struct wimax_msg_header *)(tx_buf +
				sizeof(struct hw_packet_header)))->type
						= be16_to_cpu(ETHERTYPE_DL);
	((struct wimax_msg_header *)(tx_buf +
				sizeof(struct hw_packet_header)))->id
						= be16_to_cpu(cmd_id);
	((struct wimax_msg_header *)(tx_buf +
				sizeof(struct hw_packet_header)))->length
						= be32_to_cpu(CMD_MSG_LENGTH);
	return sd_send(adapter, tx_buf, CMD_MSG_TOTAL_LENGTH);
}

bool send_image_info_packet(struct net_adapter *adapter, u16 cmd_id)
{
	struct hw_packet_header	*pkt_hdr;
	struct wimax_msg_header	*msg_hdr;
	u32			image_info[4];
	u8			tx_buf[IMAGE_INFO_MSG_TOTAL_LENGTH];
	u32			offset;

	pkt_hdr = (struct hw_packet_header *)tx_buf;
	pkt_hdr->id0 = 'W';
	pkt_hdr->id1 = 'C';
	pkt_hdr->length = be16_to_cpu(IMAGE_INFO_MSG_TOTAL_LENGTH);

	offset = sizeof(struct hw_packet_header);
	msg_hdr = (struct wimax_msg_header *)(tx_buf + offset);
	msg_hdr->type = be16_to_cpu(ETHERTYPE_DL);
	msg_hdr->id = be16_to_cpu(cmd_id);
	msg_hdr->length = be32_to_cpu(IMAGE_INFO_MSG_LENGTH);

	image_info[0] = 0;
	image_info[1] = be32_to_cpu(adapter->fw->size);
	image_info[2] = be32_to_cpu(CMC732_WIMAX_ADDRESS);
	image_info[3] = 0;

	offset += sizeof(struct wimax_msg_header);
	memcpy(&(tx_buf[offset]), image_info, sizeof(image_info));
	return sd_send(adapter, tx_buf, IMAGE_INFO_MSG_TOTAL_LENGTH);
}

bool send_image_data_packet(struct net_adapter *adapter, u16 cmd_id)
{
	struct hw_packet_header		*pkt_hdr;
	struct image_data_payload	*pImageDataPayload;
	struct wimax_msg_header		*msg_hdr;
	u8				*tx_buf = NULL;
	u32				len;
	u32				offset;
	u32				size;
	bool					status;

	tx_buf = kmalloc(MAX_IMAGE_DATA_MSG_LENGTH, GFP_KERNEL);
	if (tx_buf == NULL) {
		pr_err("%s malloc fail", __func__);
		return -ENOMEM;
	}

	pkt_hdr = (struct hw_packet_header *)tx_buf;
	pkt_hdr->id0 = 'W';
	pkt_hdr->id1 = 'C';

	offset = sizeof(struct hw_packet_header);
	msg_hdr = (struct wimax_msg_header *)(tx_buf + offset);
	msg_hdr->type = be16_to_cpu(ETHERTYPE_DL);
	msg_hdr->id = be16_to_cpu(cmd_id);

	if (adapter->image_offset <
			(adapter->fw->size - MAX_IMAGE_DATA_LENGTH))
		len = MAX_IMAGE_DATA_LENGTH;
	else
		len = adapter->fw->size - adapter->image_offset;

	offset += sizeof(struct wimax_msg_header);
	pImageDataPayload = (struct image_data_payload *)(tx_buf + offset);
	pImageDataPayload->offset = be32_to_cpu(adapter->image_offset);
	pImageDataPayload->size = be32_to_cpu(len);

	memcpy(pImageDataPayload->data,
		adapter->fw->data + adapter->image_offset, len);

	size = len + 8; /* length of Payload offset + length + data */
	pkt_hdr->length = be16_to_cpu(CMD_MSG_TOTAL_LENGTH + size);
	msg_hdr->length = be32_to_cpu(size);

	status = sd_send(adapter, tx_buf, CMD_MSG_TOTAL_LENGTH + size);

	kfree(tx_buf);

	adapter->image_offset += len;

	return status;
}


static u32 process_private_cmd(struct net_adapter *adapter, void *buffer)
{
	struct hw_private_packet	*cmd;
	struct wimax_cfg	*g_cfg = adapter->pdata->g_cfg;
	u8				*bufp = (u8 *)buffer;

	cmd = (struct hw_private_packet *)buffer;

	switch (cmd->code) {
	case HWCODEMACRESPONSE: {

		if (!completion_done(&adapter->mac))
			complete(&adapter->mac);

		/* processing for mac_req request */
		#ifndef PRODUCT_SHIP
		pr_debug("MAC address = %02x:%02x:%02x:%02x:%02x:%02x",
				bufp[3], bufp[4], bufp[5],
				bufp[6], bufp[7], bufp[8]);
		#endif
		/* create ethernet header */
		memcpy(adapter->eth_header,
				bufp + 3, ETHERNET_ADDRESS_LENGTH);
		memcpy(adapter->eth_header + ETHERNET_ADDRESS_LENGTH,
				bufp + 3, ETHERNET_ADDRESS_LENGTH);
		adapter->eth_header[(ETHERNET_ADDRESS_LENGTH * 2) - 1]++;

		memcpy(adapter->net->dev_addr, bufp + 3,
				ETHERNET_ADDRESS_LENGTH);

		return sizeof(*cmd) + ETHERNET_ADDRESS_LENGTH - sizeof(u8);
	}
	case HWCODEIDLENTFY: {
		pr_debug("%s HWCODEIDLENTFY", __func__);

		s3c_bat_use_wimax(0);
		break;
	}
	case HWCODELINKINDICATION: {
		if (cmd->value == HW_PROT_VALUE_LINK_DOWN) {
			pr_debug("LINK_DOWN_INDICATION");

			s3c_bat_use_wimax(0);
			/* indicate link down */
			netif_stop_queue(adapter->net);
			netif_carrier_off(adapter->net);
		} else if (cmd->value == HW_PROT_VALUE_LINK_UP) {
			pr_debug("LINK_UP_INDICATION");

			s3c_bat_use_wimax(1);
			/* indicate link up */
			netif_start_queue(adapter->net);
			netif_carrier_on(adapter->net);
		}
		break;
	}
	case HWCODEWAKEUPNTFY: {
		/*
		*dont suspend for at least
		*4 sec after modem wake up
		*/
		s3c_bat_use_wimax(1);
		wake_lock_timeout(&g_cfg->wimax_driver_lock, 4 * HZ);
		pr_debug("%s HWCODEWAKEUPNTFY", __func__);
		break;
	}
	case HWCODEHALTEDINDICATION: {
		pr_debug("%s HWCODEHALTEDINDICATION, stop driver", __func__);
		schedule_work(&g_cfg->shutdown);
		break;
	}
	case HWCODERXREADYINDICATION: {
		pr_debug("Device RxReady");
		break;
	}
	default:
		pr_debug("%s packet not supported ", __func__);
		break;
	}
	return sizeof(*cmd);
}
static void process_indicate_packet(struct net_adapter *adapter, u8 *buffer)
{
	struct wimax_msg_header *packet;
	char *tmp_byte;

	packet = (struct wimax_msg_header *)buffer;

	if (packet->type != be16_to_cpu(ETHERTYPE_DL)) {
		pr_warn("%s: not a download packet\n", __func__);
		return;
	}

	switch (be16_to_cpu(packet->id)) {
	case MSG_DRIVER_OK_RESP:
		pr_debug("%s: MSG_DRIVER_OK_RESP\n", __func__);
		adapter->modem_resp = true;
		wake_up_interruptible(&adapter->modem_resp_event);
		send_image_info_packet(adapter, MSG_IMAGE_INFO_REQ);
		break;
	case MSG_IMAGE_INFO_RESP:
		pr_debug("%s: MSG_IMAGE_INFO_RESP\n", __func__);
		send_image_data_packet(adapter, MSG_IMAGE_DATA_REQ);
		break;
	case MSG_IMAGE_DATA_RESP:
		if (adapter->image_offset == adapter->fw->size) {
			pr_debug("%s: Image Download Complete\n", __func__);
			send_cmd_packet(adapter, MSG_RUN_REQ);
		} else {
			send_image_data_packet(adapter, MSG_IMAGE_DATA_REQ);
		}
		break;
	case MSG_RUN_RESP:
		tmp_byte = (char *)(buffer + sizeof(*packet));

		if (*tmp_byte != 0x01)
			break;
		complete(&adapter->firmware_download);
		pr_debug("%s: MSG_RUN_RESP\n", __func__);
		break;
	default:
		pr_warn("%s: Unknown packet type\n", __func__);
		break;
	}
}

/* receive control data */
void control_recv(struct net_adapter *adapter, void *buffer, u32 length)
{
	struct process_descriptor	*procdsc;
	struct buffer_descriptor	*bufdsc;
	struct list_head	*pos, *nxt;

	mutex_lock(&adapter->control_lock);
	list_for_each_safe(pos, nxt, &adapter->control_process_list) {
		procdsc = list_entry(pos, struct process_descriptor, list);
		if (procdsc->type != *((u16 *)buffer))
			continue;
		bufdsc = kmalloc(sizeof(*bufdsc), GFP_KERNEL);
		bufdsc->buffer = kmalloc(
		(length + (ETHERNET_ADDRESS_LENGTH * 2)), GFP_KERNEL);
		memcpy(bufdsc->buffer, adapter->eth_header,
			(ETHERNET_ADDRESS_LENGTH * 2));
		memcpy(bufdsc->buffer + (ETHERNET_ADDRESS_LENGTH * 2),
			buffer, length);

		/* fill out descriptor */
		bufdsc->length = length + (ETHERNET_ADDRESS_LENGTH * 2);
		list_add_tail(&bufdsc->list, &procdsc->buffer_list);
		wake_up_interruptible(&procdsc->read_wait);
	}
	mutex_unlock(&adapter->control_lock);
}

void prepare_skb(struct net_adapter *adapter, struct sk_buff *rx_skb)
{
	skb_reserve(rx_skb,
			(ETHERNET_ADDRESS_LENGTH * 2) +
			NET_IP_ALIGN);

	memcpy(skb_push(rx_skb,
			(ETHERNET_ADDRESS_LENGTH * 2)),
			adapter->eth_header,
			(ETHERNET_ADDRESS_LENGTH * 2));

	rx_skb->dev = adapter->net;
	rx_skb->ip_summed = CHECKSUM_UNNECESSARY;
}
void flush_skb(struct net_adapter *adapter)
{
	if (adapter->rx_skb) {
		dev_kfree_skb(adapter->rx_skb);
		adapter->rx_skb = NULL;
	}
}
struct sk_buff *fetch_skb(struct net_adapter *adapter)
{
	struct sk_buff *ret_skb;
	if (adapter->rx_skb) {
		ret_skb = adapter->rx_skb;
		adapter->rx_skb = NULL;
		return ret_skb;
	}
	ret_skb = dev_alloc_skb(WIMAX_MTU_SIZE+2+
			(ETHERNET_ADDRESS_LENGTH * 2) +
					NET_IP_ALIGN);
	if (!ret_skb) {
		pr_debug("unable to allocate skb");
		return NULL;
	}
	prepare_skb(adapter, ret_skb);
	return ret_skb;
}
void pull_skb(struct net_adapter *adapter)
{
	struct sk_buff *t_skb;
	if (adapter->rx_skb == NULL) {
		t_skb = dev_alloc_skb(WIMAX_MTU_SIZE+2+
				(ETHERNET_ADDRESS_LENGTH * 2) +
				NET_IP_ALIGN);
		if (!t_skb) {
			pr_debug("unable to allocate skb");
			return;
		}
		prepare_skb(adapter, t_skb);
		adapter->rx_skb = t_skb;
	}
}

static void adapter_rx_packet(struct net_adapter *adapter)
{
	struct hw_packet_header	*hdr;
	s32			rlen = adapter->buff_len;
	u32			l;
	u8			*ofs;
	struct sk_buff		*rx_skb;
	ofs = adapter->receive_buffer;

	while (rlen > 0) {
		hdr = (struct hw_packet_header *)ofs;

		/* "WD", "WC", "WP" or "WE" */
		if (unlikely(hdr->id0 != 'W')) {
			/*Ignore if it is the 4 byte allignment*/
			if (rlen > 4) {
				pr_warn("Wrong packet ID (%02x %02x)",
							hdr->id0, hdr->id1);
			}
			/* skip rest of packets */
			break;
		}

		/* change offset */
		ofs += sizeof(*hdr);
		rlen -= sizeof(*hdr);

		/* check packet type */
		switch (hdr->id1) {
		case 'P': {
			/* revert offset */
			ofs -= sizeof(*hdr);
			rlen += sizeof(*hdr);
			/* process packet */
			l = process_private_cmd(adapter, ofs);
			/* shift */
			ofs += l;
			rlen -= l;

			/* process next packet */
			continue;
			}
		case 'C':
			if (adapter->pdata->g_cfg->power_state ==
							CMC_POWER_ON) {
				ofs += 2;
				rlen -= 2;
				control_recv(adapter, (u8 *)ofs, hdr->length);
				break;
			} else {
				hdr->length -= sizeof(*hdr);
				process_indicate_packet(adapter, ofs);
				break;
			}
		case 'D':
			ofs += 2;
			rlen -= 2;

			if (hdr->length > BUFFER_DATA_SIZE) {
					pr_err("Data packet too large");
					adapter->netstats.rx_dropped++;
					break;
				}

			if (likely(hdr->length <= (WIMAX_MTU_SIZE + 2))) {
				rx_skb = fetch_skb(adapter);
				if (!rx_skb) {
					pr_err("unable to allocate skb");
					break;
					}
			} else {
				rx_skb = dev_alloc_skb(hdr->length +
				      (ETHERNET_ADDRESS_LENGTH * 2) +
							NET_IP_ALIGN);
				if (!rx_skb) {
					pr_err("unable to allocate skb");
					break;
				}
				prepare_skb(adapter, rx_skb);
			}

				memcpy(skb_put(rx_skb, hdr->length),
							(u8 *)ofs,
							hdr->length);

				rx_skb->protocol =
					eth_type_trans(rx_skb, adapter->net);

				if (netif_rx_ni(rx_skb) == NET_RX_DROP) {
					pr_err("packet dropped!");
					adapter->netstats.rx_dropped++;
				}
				adapter->netstats.rx_packets++;
				adapter->netstats.rx_bytes +=
					(hdr->length +
					 (ETHERNET_ADDRESS_LENGTH * 2));

			break;
		case 'E':
			/* skip rest of buffer */
			break;
		default:
			pr_warn("%s :Wrong packet ID [%02x %02x]",
						__func__, hdr->id0, hdr->id1);
			/* skip rest of buffer */
			break;
		}

		ofs += hdr->length;
		rlen -= hdr->length;
	}

	return;
}

void rx_packet(struct net_adapter *adapter)
{
	int ret = 0;
	int read_idx;
	s32							t_len;
	s32							t_index;
	s32							t_size;
	u8							*t_buff;

	sdio_claim_host(adapter->func);
	read_idx = sdio_readb(adapter->func, SDIO_C2H_RP_REG, &ret);

	t_len = sdio_readl(adapter->func, (SDIO_RX_BANK_ADDR +
				(read_idx * CMC732_SDIO_BANK_SIZE)), &ret);
	if (unlikely(ret)) {
		pr_err("%s sdio_readl error", __func__);
		sdio_error(adapter);
		goto err;
	}

	if (unlikely(t_len > CMC732_MAX_PACKET_SIZE))	{
		pr_err("%s length out of bound", __func__);
		t_len = CMC732_MAX_PACKET_SIZE;
		}

	sdio_writeb(adapter->func, (read_idx + 1) % 16,
			SDIO_C2H_RP_REG, NULL);
	if (unlikely(!t_len))
		goto err;


	adapter->buff_len = t_len;
	t_index = (SDIO_RX_BANK_ADDR + (CMC732_SDIO_BANK_SIZE * read_idx) + 4);
	t_buff = adapter->receive_buffer;

	while (likely(t_len)) {
		t_size = (t_len > CMC_BLOCK_SIZE) ?
			(CMC_BLOCK_SIZE) : t_len;
		ret = sdio_memcpy_fromio(adapter->func, (void *)t_buff,
				t_index, t_size);
		if (unlikely(ret)) {
			pr_err("%s sdio_memcpy_fromio fail", __func__);
			sdio_error(adapter);
			goto err;
		}
		t_len -= t_size;
		t_buff += t_size;
		t_index += t_size;
	}
	sdio_release_host(adapter->func);
	return;
err:
	adapter->netstats.rx_dropped++;
	sdio_release_host(adapter->func);
	return;
}
static void rx_process_data(struct work_struct *rx_work)
{
	struct net_adapter *adapter = container_of(rx_work,
						struct net_adapter, rx_work);

	rx_packet(adapter);
	adapter_rx_packet(adapter);

}

static void adapter_interrupt(struct sdio_func *func)
{
	struct net_adapter		*adapter = sdio_get_drvdata(func);
	int				intrd = 0;
	struct buffer_descriptor *bufdsc;

	/* read interrupt identification register and clear the interrupt */
	intrd = sdio_readb(func, SDIO_INT_STATUS_REG, NULL);
	sdio_writeb(func, intrd, SDIO_INT_STATUS_CLR_REG, NULL);

	if (likely(intrd & SDIO_INT_DATA_READY)) {
		queue_work(adapter->wimax_workqueue,
			 &adapter->rx_work);
	} else {
		adapter->netstats.rx_errors++;
		pr_err("%s intrd = SDIO_INT_ERROR occurred",
			__func__);
	}
}

bool send_mac_request(struct net_adapter *adapter)
{
	struct hw_private_packet	req;
	req.id0 = 'W';
	req.id1 = 'P';
	req.code = HWCODEMACREQUEST;
	req.value = 0;
	return sd_send(adapter, (u8 *)&req, sizeof(req));
}

int hw_device_wakeup(struct net_adapter *adapter)
{
	int rc = 0;
	int ret = 0;
	struct wimax732_platform_data	*pdata = adapter->pdata;
	adapter->pdata->wakeup_assert(1);

	while (unlikely(!pdata->is_modem_awake())) {
		rc++;
		if (rc > WAKEUP_MAX_TRY) {
			pr_err("%s (CON0 status): modem wake up time out",
								__func__);
			ret = -EIO;
			break;
		}

		if (rc == 1)
			pr_debug("%s (CON0 status): waiting for modem awake",
								__func__);
		msleep(WAKEUP_ASSERT_T);
		if (pdata->is_modem_awake())
			break;
		pdata->wakeup_assert(0);
		msleep(WAKEUP_ASSERT_T);
		pdata->wakeup_assert(1);
	}

	s3c_bat_use_wimax(1);
	if (unlikely((rc > 0) && (rc <= WAKEUP_MAX_TRY)))
		pr_debug("%s (CON0 status): modem awake", __func__);
	pdata->wakeup_assert(0);
	return ret;
}

static void tx_process_data(struct work_struct *tx_work)
{
	struct buffer_descriptor *bufdsc;
	struct net_adapter *adapter = container_of(tx_work,
						struct net_adapter, tx_work);
	struct wimax_cfg *g_cfg = adapter->pdata->g_cfg;
	unsigned long flags;
	bool				nRet;

	while (1) {

		if (!mutex_trylock(&g_cfg->suspend_mutex))
			return;
		spin_lock_irqsave(&adapter->send_lock, flags);
		if (likely(!list_empty(&adapter->q_send))) {
			bufdsc = list_first_entry(&adapter->q_send,
					struct buffer_descriptor, list);
			list_del(&bufdsc->list);
		} else {
			spin_unlock_irqrestore(&adapter->send_lock, flags);
			mutex_unlock(&g_cfg->suspend_mutex);
			return;
		}
		spin_unlock_irqrestore(&adapter->send_lock, flags);
		if (unlikely(hw_device_wakeup(adapter))) {
			schedule_work(&g_cfg->shutdown);
			mutex_unlock(&g_cfg->suspend_mutex);
			kfree(bufdsc->buffer);
			kfree(bufdsc);
			return;
		}
		if (bufdsc->length > CMC_MAX_BYTE_SIZE)
			bufdsc->length = (bufdsc->length + CMC_MAX_BYTE_SIZE) &
			~(CMC_MAX_BYTE_SIZE);
		nRet = sd_send(adapter, bufdsc->buffer, bufdsc->length);
		mutex_unlock(&g_cfg->suspend_mutex);
		kfree(bufdsc->buffer);
		kfree(bufdsc);
		if (unlikely(nRet))
			return;
	};
}

struct process_descriptor *process_by_id(struct net_adapter *adapter, u32 id)
{
	struct process_descriptor	*procdsc;

	list_for_each_entry(procdsc, &adapter->control_process_list, list) {
		if (procdsc->id == id)	/* process found */
			return procdsc;
	}
	return NULL;
}

struct process_descriptor *fetch_process_by_id(struct net_adapter *adapter,
									u32 id)
{
	struct process_descriptor	*procdsc;
	procdsc = process_by_id(adapter, id);
	if (!procdsc) {
		procdsc = kzalloc(sizeof(*procdsc), GFP_KERNEL);
		if (!procdsc)
			return NULL;
		procdsc->id = id;
		init_waitqueue_head(&procdsc->read_wait);
		INIT_LIST_HEAD(&procdsc->buffer_list);
		mutex_lock(&adapter->control_lock);
		list_add_tail(&procdsc->list, &adapter->control_process_list);
		mutex_unlock(&adapter->control_lock);
	}
	return procdsc;
}

u32 control_send(struct net_adapter *adapter, void *buffer, u32 length)
{
	struct buffer_descriptor	*bufdsc;
	struct hw_packet_header		*hdr;
	unsigned long flags;
	u8				*ptr;

	if ((length + sizeof(*hdr)) >= WIMAX_MAX_TOTAL_SIZE)
		return -ENOMEM;/* changed from SUCCESS return status */

	bufdsc = (struct buffer_descriptor *)
		kmalloc(sizeof(*bufdsc), GFP_KERNEL);
	if (bufdsc == NULL)
		return -ENOMEM;
	bufdsc->buffer = kmalloc(BUFFER_DATA_SIZE, GFP_KERNEL);
	if (bufdsc->buffer == NULL)
		return -ENOMEM;

	ptr = bufdsc->buffer;
	hdr = (struct hw_packet_header *)bufdsc->buffer;

	ptr += sizeof(*hdr);
	ptr += 2;

	memcpy(ptr, buffer + (ETHERNET_ADDRESS_LENGTH * 2),
					length - (ETHERNET_ADDRESS_LENGTH * 2));

	/* add packet header */
	hdr->id0 = 'W';
	hdr->id1 = 'C';
	hdr->length = (u16)length - (ETHERNET_ADDRESS_LENGTH * 2);

	/* set length */
	bufdsc->length = (u16)length - (ETHERNET_ADDRESS_LENGTH * 2)
					+ sizeof(*hdr);
	bufdsc->length += 2;

	spin_lock_irqsave(&adapter->send_lock, flags);
	list_add_tail(&bufdsc->list, &adapter->q_send);
	spin_unlock_irqrestore(&adapter->send_lock, flags);
	schedule_work(&adapter->tx_work);
	return 0;
}

/*
*	uwibro functions
*	(send and receive control packet with WiMAX modem)
*/
static int uwbrdev_open(struct inode *inode, struct file *file)
{
	struct net_adapter	*adapter = container_of(file->private_data,
			struct net_adapter, uwibro_dev);
	file->private_data = adapter;
	return 0;
}

static long uwbrdev_ioctl(struct file *file, u32 cmd,
			  unsigned long arg)
{
	struct process_descriptor	*procdsc;
	int				ret = 0;
	u8				*tx_buffer;
	int length;
	struct net_adapter		*adapter =
			(struct net_adapter *)(file->private_data);

	if (adapter->pdata->g_cfg->power_state != CMC_POWER_ON)
		return -ENODEV;

	if (cmd != CONTROL_IOCTL_WRITE_REQUEST) {
		pr_debug("uwbrdev_ioctl: unknown ioctl cmd: 0x%x", cmd);
		return -EINVAL;
	}

	if ((char *)arg == NULL) {
		pr_debug("arg == NULL: return -EFAULT");
		return -EFAULT;
	}

	procdsc = fetch_process_by_id(adapter, current->tgid);
	if (!procdsc)
		return -ENOMEM;

	length = ((int *)arg)[0];

	if (length >= WIMAX_MAX_TOTAL_SIZE)
		return -EFBIG;

	tx_buffer = kmalloc(length, GFP_KERNEL);
	if (!tx_buffer) {
		pr_err("%s: not enough memory to allocate tx_buffer\n",
								__func__);
		return -ENOMEM;
	}

	if (copy_from_user(tx_buffer, (void *)(arg + sizeof(int)), length)) {
		pr_err("%s: error copying buffer from user space\n", __func__);
		ret = -EFAULT;
		goto err_copy;
	}

	procdsc->type = ((struct eth_header *)tx_buffer)->type;
	control_send(adapter, tx_buffer, length);

err_copy:
	kfree(tx_buffer);
	return ret;
}

static ssize_t uwbrdev_read(struct file *file, char *buf, size_t count,
								loff_t *ppos)
{
	struct net_adapter		*adapter;
	struct process_descriptor	*procdsc;
	struct buffer_descriptor	*bufdsc;
	u32 len = 0;
	adapter = (struct net_adapter *)(file->private_data);

	if (buf == NULL) {
		pr_debug("BUFFER is NULL");
		return -EFAULT; /* bad address */
	}
	if (adapter->pdata->g_cfg->power_state == CMC_POWERING_OFF)
		return -ENODEV;
	procdsc = fetch_process_by_id(adapter, current->tgid);
	if (procdsc == NULL)
		return -ENOMEM;

	if (wait_event_interruptible(procdsc->read_wait,
				(!list_empty(&procdsc->buffer_list)) ||
				(adapter->pdata->g_cfg->power_state ==
						CMC_POWERING_OFF)))
		return -ERESTARTSYS;
	if (adapter->pdata->g_cfg->power_state == CMC_POWERING_OFF)
		return -ENODEV;

	if (count == 1500) {	/* app passes read count as 1500 */
		mutex_lock(&adapter->control_lock);
		bufdsc = list_first_entry(&procdsc->buffer_list,
					struct buffer_descriptor, list);
			list_del(&bufdsc->list);
		mutex_unlock(&adapter->control_lock);
	len = bufdsc->length;
		if (copy_to_user(buf, bufdsc->buffer, len)) {
			pr_debug("%s: copy_to_user failed len=%u !!",
						__func__, bufdsc->length);
			kfree(bufdsc->buffer);
			kfree(bufdsc);
			return -EFAULT;
		}
	kfree(bufdsc->buffer);
	kfree(bufdsc);
	return len;
	}

	return 0;
}

static const struct file_operations uwbr_fops = {
	.owner = THIS_MODULE,
	.open = uwbrdev_open,
	.unlocked_ioctl	= uwbrdev_ioctl,
	.read		= uwbrdev_read,
};

u32 hw_send_data(struct net_adapter *adapter, void *buffer , u32 length)
{
	struct buffer_descriptor	*bufdsc;
	struct hw_packet_header		*hdr;
	struct net_device		*net = adapter->net;
	unsigned long flags;
	u8				*ptr;

	bufdsc = (struct buffer_descriptor *)
		kmalloc(sizeof(*bufdsc), GFP_ATOMIC);
	if (bufdsc == NULL)
		return -ENOMEM;

	bufdsc->buffer = kmalloc(BUFFER_DATA_SIZE, GFP_ATOMIC);
	if (bufdsc->buffer == NULL) {
		kfree(bufdsc);
		return -ENOMEM;
	}

	ptr = bufdsc->buffer;

	/* shift data pointer */
	ptr = ptr + sizeof(*hdr) + 2;
	hdr = (struct hw_packet_header *)bufdsc->buffer;

	length -= (ETHERNET_ADDRESS_LENGTH * 2);
	buffer += (ETHERNET_ADDRESS_LENGTH * 2);

	memcpy(ptr, buffer, length);

	hdr->id0 = 'W';
	hdr->id1 = 'D';
	hdr->length = (u16)length;

	bufdsc->length = length + sizeof(*hdr) + 2;

	/* add statistics */
	adapter->netstats.tx_packets++;
	adapter->netstats.tx_bytes += bufdsc->length;

	spin_lock_irqsave(&adapter->send_lock, flags);
	list_add_tail(&bufdsc->list, &adapter->q_send);
	spin_unlock_irqrestore(&adapter->send_lock, flags);

	queue_work(adapter->wimax_workqueue, &adapter->tx_work);
	if (!netif_running(net))
		pr_debug("!netif_running");

	return 0;
}

static int netdev_ethtool_ioctl(struct net_device *dev,
		void *useraddr)
{
	u32	ethcmd;
	struct ethtool_drvinfo info = {ETHTOOL_GDRVINFO};

	if (copy_from_user(&ethcmd, useraddr, sizeof(ethcmd)))
		return -EFAULT;

	if (ethcmd != ETHTOOL_GDRVINFO)
		return -EOPNOTSUPP;

	strncpy(info.driver, "C732SDIO", sizeof(info.driver) - 1);
	if (copy_to_user(useraddr, &info, sizeof(info)))
		return -EFAULT;

	return 0;
}

static struct net_device_stats *adapter_netdev_stats(struct net_device *dev)
{
	return &((struct net_adapter *)netdev_priv(dev))->netstats;
}

static int adapter_start_xmit(struct sk_buff *skb, struct net_device *net)
{
	struct net_adapter	*adapter = netdev_priv(net);
	hw_send_data(adapter, skb->data, skb->len);
	dev_kfree_skb(skb);
	return 0;
}

static int adapter_ioctl(struct net_device *net, struct ifreq *rq, int cmd)
{
	switch (cmd) {
	case SIOCETHTOOL:
		return netdev_ethtool_ioctl(net, (void *)rq->ifr_data);
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}


static struct net_device_ops wimax_net_ops = {
	.ndo_get_stats			=	adapter_netdev_stats,
	.ndo_do_ioctl			=	adapter_ioctl,
	.ndo_start_xmit			=	adapter_start_xmit,
};

static irqreturn_t wimax_hostwake_isr(int irq, void *dev)
{
	struct net_adapter	*adapter = dev;
	wake_lock_timeout(&adapter->pdata->g_cfg->wimax_driver_lock, 1 * HZ);

	return IRQ_HANDLED;
}

static int cmc732_setup_wake_irq(struct net_adapter *adapter)
{
	struct wimax732_platform_data *pdata = adapter->pdata;
	int rc;
	int irq;

	rc = gpio_request(pdata->wimax_int, "gpio_wimax_int");
	if (rc < 0) {
		pr_debug("%s: gpio %d request failed (%d)\n",
			__func__, pdata->wimax_int, rc);
		return rc;
	}

	rc = gpio_direction_input(pdata->wimax_int);
	if (rc < 0) {
		pr_debug("%s: failed to set gpio %d as input (%d)\n",
			__func__, pdata->wimax_int, rc);
		goto err_gpio_direction_input;
	}

	irq = gpio_to_irq(pdata->wimax_int);

	rc = request_threaded_irq(irq, NULL, wimax_hostwake_isr,
		IRQF_TRIGGER_FALLING, "wimax_int", adapter);
	if (rc < 0) {
		pr_debug("%s: request_irq(%d) failed for gpio %d (%d)\n",
			__func__, irq,
			pdata->wimax_int, rc);
		goto err_request_irq;
	}

	rc = enable_irq_wake(irq);
	if (rc < 0) {
		pr_err("%s: enable_irq_wake(%d) failed for gpio %d (%d)\n",
				__func__, irq, pdata->wimax_int, rc);
		goto err_enable_irq_wake;
	}

	adapter->wake_irq = irq;

	return 0;

err_enable_irq_wake:
	free_irq(irq, adapter);
err_request_irq:
err_gpio_direction_input:
	gpio_free(pdata->wimax_int);
	return rc;
}

static void cmc732_release_wake_irq(struct net_adapter *adapter)
{
	if (!adapter->wake_irq)
		return;
	disable_irq_wake(adapter->wake_irq);
	free_irq(adapter->wake_irq, adapter);
	gpio_free(adapter->pdata->wimax_int);
}

#ifdef WIMAX_CON0_POLL
int con0_poll_thread(void *data)
{
	struct net_adapter *adapter = (struct net_adapter *)data;
	struct wimax_cfg *g_cfg = adapter->pdata->g_cfg;
	int prev_val = 0;
	int curr_val;

	wake_lock(&g_cfg->wimax_driver_lock);

	while ((g_cfg->power_state != CMC_POWERING_OFF) &&
					(g_cfg->power_state != CMC_POWER_OFF)) {
		curr_val = adapter->pdata->is_modem_awake();
		if ((prev_val && (!curr_val)) || (!curr_val)) {
			adapter->pdata->restore_uart_path();
			break;
		}
		prev_val = curr_val;
		wait_event_interruptible_timeout(adapter->con0_poll,
				(g_cfg->power_state == CMC_POWERING_OFF) ||
				(g_cfg->power_state == CMC_POWER_OFF),
				msecs_to_jiffies(40));
	}
	wake_unlock(&g_cfg->wimax_driver_lock);
	do_exit(0);
	return 0;
}
#endif

static int wimax_power_on(struct wimax732_platform_data *pdata)
{

	struct net_adapter	*adapter = NULL;
	struct net_device	*net;
	struct wimax_cfg *g_cfg = pdata->g_cfg;
	struct buffer_descriptor *bufdsc;
	struct process_descriptor *procdsc;
	struct list_head	*pos, *nxt, *pos1, *nxt1;
	int count;
	long ret;
	int err = 0;
	u8			 node_id[ETH_ALEN];

	mutex_lock(&g_cfg->power_mutex);
	/*dont sleep when turning on*/
	mutex_lock(&g_cfg->suspend_mutex);

	/*Exit if wimax is already ON*/
	if (g_cfg->power_state == CMC_POWER_ON) {
		pr_debug("WiMAX already ON");
		err = WIMAX_ALREADY_POWER_ON;
		goto exit;
	}

	g_cfg->power_state = CMC_POWERING_ON;
	pr_debug("WIMAX POWERING ON");

	net = alloc_etherdev(sizeof(*adapter));
	if (!net) {
		pr_debug("%s: error can't allocate device", __func__);
		goto alloceth_fail;
	}

	adapter = netdev_priv(net);
	memset(adapter, 0, sizeof(*adapter));
	adapter->pdata = pdata;
	pdata->adapter_data = adapter;
	adapter->net = net;

	init_completion(&adapter->probe);
	init_completion(&adapter->remove);
	init_completion(&adapter->firmware_download);
	init_completion(&adapter->mac);

	pdata->power(1); /*power ON wimax chipset*/

	/*We need to sleep wait for atleast CMC_BOOT_LOAD duration.
	*using msleep sometimes returns before the requested time
	* hence the alternative below*/
	if (wait_for_completion_interruptible_timeout(
					&adapter->probe,
					msecs_to_jiffies(CMC_BOOTLOAD_TIME))) {
		pr_warn("-ERESTARTSYS during CMC_BOOTLOAD_TIME");
		goto probe_timeout;
	}

	pdata->detect(1); /*detect the presence of SDIO card*/

	/*wait for SDIO driver probe*/
	ret = wait_for_completion_interruptible_timeout(
				&adapter->probe,
				msecs_to_jiffies(CMC_PROBE_TIMEOUT));
	if (ret) {
		if (ret == -ERESTARTSYS) {
			pr_err("-ERESTARTSYS during CMC_PROBE_TIMEOUT");
			goto probe_timeout;
		}
	} else {
		pr_err("%s CMC_PROBE_TIMEOUT", __func__);
		err = WIMAX_POWER_FAIL;
		goto probe_timeout;
	}

	/*set the mode pins of modem for the firmware to boot in desired mode*/
	pdata->set_mode();

	mutex_init(&adapter->control_lock);

	/* For sending data and control packets */
	INIT_LIST_HEAD(&adapter->q_send);
	spin_lock_init(&adapter->send_lock);
	INIT_LIST_HEAD(&adapter->control_process_list);

	INIT_WORK(&adapter->tx_work, tx_process_data);

	/*load the wimax firmware*/
	if (request_firmware(&adapter->fw, (g_cfg->wimax_mode == AUTH_MODE) ?
				WIMAX_LOADER_PATH : WIMAX_IMAGE_PATH,
							&adapter->func->dev)) {
		dev_err(&adapter->func->dev, "%s: Can't open firmware file\n",
								__func__);
		goto firmwareload_fail;
	}

	sdio_claim_host(adapter->func);
	err = sdio_enable_func(adapter->func);
	if (err < 0) {
		pr_err("sdio_enable func error = %d", err);
		release_firmware(adapter->fw);
		goto sdioen_fail;
	}

	adapter->wimax_workqueue = create_workqueue("wimax_queue");
	INIT_WORK(&adapter->rx_work, rx_process_data);
	adapter->receive_buffer = kmalloc(CMC732_MAX_PACKET_SIZE, GFP_KERNEL);
	if (!adapter->receive_buffer) {
		pr_err("receive_buffer alloc error");
		goto buffer_fail;
	}
	err = sdio_claim_irq(adapter->func, adapter_interrupt);
	if (err < 0) {
		pr_err("sdio_claim_irq = %d", err);
		release_firmware(adapter->fw);
		goto sdioirq_fail;
	}
	sdio_set_block_size(adapter->func, CMC_BLOCK_SIZE);
	sdio_release_host(adapter->func);
	init_waitqueue_head(&adapter->modem_resp_event);
	count = 0;

	while (!adapter->modem_resp) {
		/*This command will start the
		firmware download sequence through sdio*/
		send_cmd_packet(adapter, MSG_DRIVER_OK_REQ);
		ret = wait_event_interruptible_timeout(
				adapter->modem_resp_event,
				adapter->modem_resp, HZ/10);
		if (!adapter->modem_resp)
			pr_err("no modem response");
		if ((++count > MODEM_RESP_RETRY) || (ret == -ERESTARTSYS)) {
			release_firmware(adapter->fw);
			goto firmware_download_fail;
		}
	}

	ret = wait_for_completion_interruptible_timeout(
			&adapter->firmware_download,
			msecs_to_jiffies(CMC_FIRMWARE_DOWNLOAD_TIMEOUT));
	if (ret) {
		if (ret == -ERESTARTSYS) {
			pr_err("-ERESTARTSYS firmware download fail");
			release_firmware(adapter->fw);
			goto firmware_download_fail;
		}
	} else {
		pr_err("%s CMC_FIRMWARE_DOWNLOAD_TIMEOUT", __func__);
		release_firmware(adapter->fw);
		goto firmware_download_fail;
	}
	release_firmware(adapter->fw);

	/*wait for firmware to initialize before proceeding*/
	msleep(1700);


	/* Dummy value for "ifconfig up" for 2.6.24 */
	random_ether_addr(node_id);
	memcpy(net->dev_addr, node_id, sizeof(node_id));

	/*get the MAC when in the following modes*/
	if (g_cfg->wimax_mode == SDIO_MODE
		|| g_cfg->wimax_mode == DM_MODE
		|| g_cfg->wimax_mode == USB_MODE
		|| g_cfg->wimax_mode	== USIM_RELAY_MODE) {

		count = MAC_RETRY_COUNT;
		do {
			if (!(count--)) {
				pr_err("%s MAC request timeout", __func__);
				goto mac_request_fail;
			}
			if (send_mac_request(adapter))
				goto mac_request_fail;
			ret = wait_for_completion_interruptible_timeout(
					&adapter->mac,
			msecs_to_jiffies((MAC_RETRY_COUNT - count) *
					MAC_RETRY_INTERVAL));
			if (ret == -ERESTARTSYS) {
				pr_err("-ERESTARTSYS MAC request fail");
				goto mac_request_fail;
			}
		} while (!ret);

	}
#ifdef WIMAX_CON0_POLL
	if (g_cfg->wimax_mode == WTM_MODE) {
		init_waitqueue_head(&adapter->con0_poll);
		adapter->wtm_task = kthread_create(con0_poll_thread,
				adapter, "%s", "wimax_con0_poll_thread");
		if (adapter->wtm_task)
			wake_up_process(adapter->wtm_task);
	}
#endif
	adapter->uwibro_dev.minor = MISC_DYNAMIC_MINOR;
	adapter->uwibro_dev.name = "uwibro";
	adapter->uwibro_dev.fops = &uwbr_fops;
	adapter->msg_enable = netif_msg_init(msg_level, NETIF_MSG_DRV
					| NETIF_MSG_PROBE | NETIF_MSG_LINK);

	ether_setup(net);
	strcpy(net->name, "uwbr%d");
	net->netdev_ops = &wimax_net_ops;
	net->watchdog_timeo = ADAPTER_TIMEOUT;
	net->mtu = WIMAX_MTU_SIZE;
	net->flags |= IFF_NOARP;

	SET_NETDEV_DEV(net, &adapter->func->dev);
	if (register_netdev(net))
		goto regnetdev_fail;

	netif_carrier_off(net);
	netif_tx_stop_all_queues(net);

	if (misc_register(&adapter->uwibro_dev)) {
		pr_err("adapter_probe: misc_register() failed");
		goto miscreg_fail;
	}

	cmc732_setup_wake_irq(adapter);

	g_cfg->power_state = CMC_POWER_ON;
	mutex_unlock(&g_cfg->suspend_mutex);
	mutex_unlock(&g_cfg->power_mutex);
	pr_debug("WIMAX ON");
	return 0;

	misc_deregister(&adapter->uwibro_dev);
miscreg_fail:
	unregister_netdev(net);
regnetdev_fail:
#ifdef WIMAX_CON0_POLL
	if (g_cfg->wimax_mode == WTM_MODE) {
		g_cfg->power_state = CMC_POWERING_OFF;
		wake_up_interruptible(&adapter->con0_poll);
	}
#endif
mac_request_fail:
firmware_download_fail:
	g_cfg->power_state = CMC_POWERING_OFF;

	/*wake up everyone waiting on uwbr_dev read wait*/
	list_for_each_safe(pos, nxt, &adapter->control_process_list) {
		procdsc = list_entry(pos, struct process_descriptor, list);
		wake_up_interruptible(&procdsc->read_wait);
	}

	list_for_each_safe(pos, nxt, &adapter->q_send) {
		bufdsc = list_entry(pos, struct buffer_descriptor, list);
		list_del(pos);
		kfree(bufdsc->buffer);
		kfree(bufdsc);
	}
	list_for_each_safe(pos, nxt, &adapter->control_process_list) {
		procdsc = list_entry(pos, struct process_descriptor, list);
		list_del(pos);
		list_for_each_safe(pos1, nxt1, &procdsc->buffer_list) {
			bufdsc = list_entry(pos1,
					struct buffer_descriptor, list);
			list_del(pos1);
			kfree(bufdsc->buffer);
			kfree(bufdsc);
		}
		kfree(procdsc);
	}
	sdio_claim_host(adapter->func);
	sdio_release_irq(adapter->func);

sdioirq_fail:
	kfree(adapter->receive_buffer);
buffer_fail:
	destroy_workqueue(adapter->wimax_workqueue);
	sdio_disable_func(adapter->func);
sdioen_fail:
	sdio_release_host(adapter->func);
firmwareload_fail:
	mutex_destroy(&adapter->control_lock);

probe_timeout:

	pdata->power(0);
	if (adapter->func) {
		mdelay(10);
		pdata->detect(0);
		if (!wait_for_completion_interruptible_timeout(
					&adapter->remove,
					msecs_to_jiffies(2000))) {
			pr_err("%s CMC_REMOVE_TIMEOUT", __func__);
		}
	}

	/*wait for power off transients*/
	msleep(250);

	free_netdev(net);
	pdata->adapter_data = NULL;
alloceth_fail:
	g_cfg->power_state = CMC_POWER_OFF;
	err = WIMAX_POWER_FAIL;
exit:
	mutex_unlock(&g_cfg->suspend_mutex);
	mutex_unlock(&pdata->g_cfg->power_mutex);
	return err;

}


static int wimax_power_off(struct wimax732_platform_data *pdata)
{
	int err = 0;
	struct net_adapter	*adapter = NULL;
	struct wimax_cfg *g_cfg = pdata->g_cfg;
	struct buffer_descriptor *bufdsc;
	struct process_descriptor *procdsc;
	struct list_head	*pos, *nxt, *pos1, *nxt1;

	mutex_lock(&g_cfg->power_mutex);

	/*dont sleep when turning off*/
	mutex_lock(&g_cfg->suspend_mutex);

	if (g_cfg->power_state == CMC_POWER_OFF) {
		pr_debug("WiMAX already OFF");
		err = WIMAX_ALREADY_POWER_OFF;
		goto exit;
	}
	adapter = (struct net_adapter	*) pdata->adapter_data;
	g_cfg->power_state = CMC_POWERING_OFF;
#ifdef WIMAX_CON0_POLL
	if (g_cfg->wimax_mode == WTM_MODE)
		wake_up_interruptible(&adapter->con0_poll);
#endif

	cmc732_release_wake_irq(adapter);
	misc_deregister(&adapter->uwibro_dev);
	netif_stop_queue(adapter->net);
	netif_carrier_off(adapter->net);
	unregister_netdev(adapter->net);

	sdio_claim_host(adapter->func);
	sdio_release_irq(adapter->func);
	sdio_disable_func(adapter->func);
	sdio_release_host(adapter->func);

	/*wake up everyone waiting on uwbr_dev read wait*/
	list_for_each_safe(pos, nxt, &adapter->control_process_list) {
		procdsc = list_entry(pos, struct process_descriptor, list);
		wake_up_interruptible(&procdsc->read_wait);
	}

	/*clean up and remove the send queue*/
	list_for_each_safe(pos, nxt, &adapter->q_send) {
		bufdsc = list_entry(pos, struct buffer_descriptor, list);
		list_del(pos);
		kfree(bufdsc->buffer);
		kfree(bufdsc);
	}

	/*wait for modem to flush eeprom data*/
	msleep(500);

	pdata->power(0);
	mdelay(10);
	pdata->detect(0);
	if (!wait_for_completion_interruptible_timeout(
				&adapter->remove,
				msecs_to_jiffies(2000))) {
		pr_err("%s CMC_REMOVE_TIMEOUT", __func__);
	}

	/*wait for power off transients*/
	msleep(250);

	list_for_each_safe(pos, nxt, &adapter->control_process_list) {
		procdsc = list_entry(pos, struct process_descriptor, list);
		list_del(pos);
		list_for_each_safe(pos1, nxt1, &procdsc->buffer_list) {
			bufdsc = list_entry(pos1,
					struct buffer_descriptor, list);
			list_del(pos1);
			kfree(bufdsc->buffer);
			kfree(bufdsc);
		}
		kfree(procdsc);
	}
	mutex_destroy(&adapter->control_lock);
	free_netdev(adapter->net);
	kfree(adapter->receive_buffer);
	destroy_workqueue(adapter->wimax_workqueue);
	pdata->adapter_data = NULL;
	g_cfg->power_state = CMC_POWER_OFF;
exit:
	mutex_unlock(&g_cfg->suspend_mutex);
	mutex_unlock(&g_cfg->power_mutex);
	return err;
}

void wimax_shutdown(struct work_struct *work)
{
	struct wimax_cfg *g_cfg =
		container_of(work,
				struct wimax_cfg, shutdown);
	wimax_power_off(g_cfg->pdata);
}

static int swmxdev_open(struct inode *inode, struct file *file)
{
	struct wimax732_platform_data *pdata =
		container_of(file->private_data,
				struct wimax732_platform_data, swmxctl_dev);
	file->private_data = pdata;
	return 0;
}


static long swmxdev_ioctl(struct file *file, u32 cmd,
							unsigned long arg) {
	int	ret = 0;
	u8	val = ((u8 *)arg)[0];
	struct wimax732_platform_data *gpdata =
		(struct wimax732_platform_data *)(file->private_data);

	pr_debug(" %s CMD: %x, PID: %d", __func__, cmd, current->tgid);

	switch (cmd) {
	case CONTROL_IOCTL_WIMAX_POWER_CTL: {
		pr_debug("CONTROL_IOCTL_WIMAX_POWER_CTL..");
		if (val == 0)
			wimax_power_off(gpdata);
		else
			ret = wimax_power_on(gpdata);
		break;
	}
	case CONTROL_IOCTL_WIMAX_MODE_CHANGE: {
		pr_debug("CONTROL_IOCTL_WIMAX_MODE_CHANGE to 0x%02x..", val);

		if ((val < 0) || (val > AUTH_MODE)) {
			pr_debug("Wrong mode 0x%02x", val);
			return 0;
		}

		wimax_power_off(gpdata);
		gpdata->g_cfg->wimax_mode = val;
		ret = wimax_power_on(gpdata);
		break;
	}
	case CONTROL_IOCTL_WIMAX_EEPROM_DOWNLOAD: {
		pr_debug("CNT_IOCTL_WIMAX_EEPROM_DOWNLOAD");
		wimax_power_off(gpdata);
		ret = eeprom_write_boot();
		break;
	}
	case CONTROL_IOCTL_WIMAX_WRITE_REV: {
		pr_debug("CONTROL_IOCTL_WIMAX_WRITE_REV");
		wimax_power_off(gpdata);
		ret = eeprom_write_rev();
		break;
	}
	case CONTROL_IOCTL_WIMAX_CHECK_CERT: {
		pr_debug("CONTROL_IOCTL_WIMAX_CHECK_CERT");
		wimax_power_off(gpdata);
		ret = eeprom_check_cert();
		break;
	}
	case CONTROL_IOCTL_WIMAX_CHECK_CAL: {
	pr_debug("CONTROL_IOCTL_WIMAX_CHECK_CAL");
		wimax_power_off(gpdata);
		ret = eeprom_check_cal();
		break;
	}
	}	/* switch (cmd) */

	return ret;
}


static const struct file_operations swmx_fops = {
	.owner = THIS_MODULE,
	.open = swmxdev_open,
	.unlocked_ioctl = swmxdev_ioctl,
};


static struct sdio_device_id adapter_table[] = {
	{ SDIO_DEVICE(0x98, 0x1) },
	{ }	/* Terminating entry */
};


static int adapter_probe(struct sdio_func *func,
		const struct sdio_device_id *id)
{
	struct wimax732_platform_data	*pdata =
			(struct wimax732_platform_data	*) id->driver_data;
	struct net_adapter	 *adapter = pdata->adapter_data;
	pr_debug("%s", __func__);
	adapter->func = func;
	sdio_set_drvdata(func, adapter);
	complete(&adapter->probe);
	return 0;
}

static void adapter_remove(struct sdio_func *func)
{
	struct net_adapter	 *adapter = sdio_get_drvdata(func);
	pr_debug("%s", __func__);
	complete(&adapter->remove);
}

static struct sdio_driver adapter_driver = {
	.name		= "C732SDIO",
	.probe		= adapter_probe,
	.remove		= adapter_remove,
	.id_table	= adapter_table,
};

static int wimax_probe(struct platform_device *pdev)
{
	struct wimax732_platform_data	*pdata = pdev->dev.platform_data;
	int err = 0;
	int i;

	pdata->g_cfg = kzalloc(sizeof(struct wimax_cfg), GFP_KERNEL);
	if (pdata->g_cfg == NULL) {
		dev_err(&pdev->dev,
				"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto alloc_fail;
	}
	pdata->g_cfg->pdata = pdata;

	/*This mutex prevents simultaneous instances of wimax_power()*/
	mutex_init(&pdata->g_cfg->power_mutex);

	/*This mutex ensures that driver does not suspend
	 *with modem wakeup pin asserted*/
	mutex_init(&pdata->g_cfg->suspend_mutex);

	/*To make plaform data available at SDIO driver probe*/
	for (i = 0; i < ARRAY_SIZE(adapter_table); i++)
		adapter_table[i].driver_data =
			(unsigned long) pdev->dev.platform_data;

	pdata->power(0);
	pdata->g_cfg->power_state = CMC_POWER_OFF;

	/* This node is used for turning on/off wimax*/
	pdata->swmxctl_dev.minor = MISC_DYNAMIC_MINOR;
	pdata->swmxctl_dev.name = "swmxctl";
	pdata->swmxctl_dev.fops = &swmx_fops;
	err = misc_register(&pdata->swmxctl_dev);
	if (err) {
		pr_err("swmxctl: misc_register failed\n");
		goto err_swmxctl_register;
	}

	/* register SDIO driver */
	err = sdio_register_driver(&adapter_driver);
	if (err < 0) {
		pr_err("%s: sdio_register_driver() failed",
						adapter_driver.name);
		goto sdio_register_fail;
		}
	wake_lock_init(&pdata->g_cfg->wimax_driver_lock,
			WAKE_LOCK_SUSPEND, "wimax_driver");

	INIT_WORK(&pdata->g_cfg->shutdown, wimax_shutdown);

#ifndef DRIVER_BIT_BANG
	if (wmxeeprom_init())
		pr_err("wmxeeprom_init() failed");
#endif

	goto exit;

sdio_register_fail:
	misc_deregister(&pdata->swmxctl_dev);
err_swmxctl_register:
	mutex_destroy(&pdata->g_cfg->suspend_mutex);
	mutex_destroy(&pdata->g_cfg->power_mutex);
	kfree(pdata->g_cfg);
alloc_fail:

exit:
	return err;
}

/* wimax suspend function */
int wimax_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct wimax732_platform_data	*pdata = pdev->dev.platform_data;

	/*Dont go to sleep when turining on or turning off*/
	if (!mutex_trylock(&pdata->g_cfg->power_mutex)) {
		pr_debug("wimax is turning on/off");
		return -EBUSY;
	}

	if (!mutex_trylock(&pdata->g_cfg->suspend_mutex)) {
		pr_debug("wimax send processing");
		mutex_unlock(&pdata->g_cfg->power_mutex);
		return -EBUSY;
	}

	/* AP active pin LOW */
	if (pdata->g_cfg->power_state == CMC_POWER_ON) {
		pdata->signal_ap_active(0);
		pr_debug("wimax_suspend");
	}
	return 0;
}

/* wimax resume function */
int wimax_resume(struct platform_device *pdev)
{
	struct wimax732_platform_data	*pdata = pdev->dev.platform_data;
	struct net_adapter *adapter;
	/* AP active pin HIGH */
	if (pdata->g_cfg->power_state == CMC_POWER_ON) {
		pdata->signal_ap_active(1);
		/* wait wakeup noti for 1 sec otherwise suspend again */
		wake_lock_timeout(&pdata->g_cfg->wimax_driver_lock, 1 * HZ);
		pr_debug("wimax_resume");
	}
	mutex_unlock(&pdata->g_cfg->power_mutex);
	mutex_unlock(&pdata->g_cfg->suspend_mutex);
	if (pdata->g_cfg->power_state == CMC_POWER_ON) {
		adapter = (struct net_adapter *)pdata->adapter_data;
		schedule_work(&adapter->tx_work);
	}
	return 0;
}
static int wimax_remove(struct platform_device *pdev)
{
	struct wimax732_platform_data	*pdata = pdev->dev.platform_data;

#ifndef DRIVER_BIT_BANG
	wmxeeprom_exit();
#endif
	misc_deregister(&pdata->swmxctl_dev);
	pdata->power(0);
	wake_lock_destroy(&pdata->g_cfg->wimax_driver_lock);
	sdio_unregister_driver(&adapter_driver);
	mutex_destroy(&pdata->g_cfg->power_mutex);
	mutex_destroy(&pdata->g_cfg->suspend_mutex);
	kfree(pdata->g_cfg);
	return 0;
}


static struct platform_driver wimax_driver = {
	.probe          = wimax_probe,
	.remove         = wimax_remove,
	.suspend		= wimax_suspend,
	.resume			= wimax_resume,
	.driver         = {
	.name   = "wimax732_driver",
	}
};

static int __init adapter_init_module(void)
{
	return platform_driver_register(&wimax_driver);
}


static void __exit adapter_deinit_module(void)
{
	platform_driver_unregister(&wimax_driver);
}

module_init(adapter_init_module);
module_exit(adapter_deinit_module);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_PARM_DESC(msg_level, "Override default message level");
MODULE_DEVICE_TABLE(sdio, adapter_table);
MODULE_VERSION(WIMAX_DRIVER_VERSION_STRING);
MODULE_LICENSE("GPL");
