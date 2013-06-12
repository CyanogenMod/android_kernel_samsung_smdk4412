/*
 * receive.c
 *
 * handle download packet, private cmd and control/data packet
 */
#include "headers.h"
#include "wimax_plat.h"
#include "firmware.h"
#include "download.h"

#include <plat/sdhci.h>
#include <plat/devs.h>
#include <linux/mmc/host.h>
#include <linux/kthread.h>
extern struct image_data g_wimax_image;

void process_indicate_packet(struct net_adapter *adapter, u_char *buffer)
{
	struct wimax_msg_header	*packet;
	char			*tmp_byte;
	struct wimax_cfg *g_cfg = adapter->pdata->g_cfg;

	packet = (struct wimax_msg_header *)buffer;

	if (packet->type == be16_to_cpu(ETHERTYPE_DL)) {
		switch (be16_to_cpu(packet->id)) {
		case MSG_DRIVER_OK_RESP:
			dump_debug("process_indicate_packet: MSG_DRIVER_OK_RESP");
			send_image_info_packet(adapter, MSG_IMAGE_INFO_REQ);
			break;
		case MSG_IMAGE_INFO_RESP:
			dump_debug("process_indicate_packet: MSG_IMAGE_INFO_RESP");
			send_image_data_packet(adapter, MSG_IMAGE_DATA_REQ);
			break;
		case MSG_IMAGE_DATA_RESP:
			if (g_wimax_image.offset == g_wimax_image.size) {
				dump_debug("process_indicate_packet: Image Download Complete");
				send_cmd_packet(adapter, MSG_RUN_REQ); /* Run Message Send */
			} else
				send_image_data_packet(adapter, MSG_IMAGE_DATA_REQ);
			break;
		case MSG_RUN_RESP:
			tmp_byte = (char *)(buffer + sizeof(struct wimax_msg_header));

			if (*tmp_byte == 0x01) {
				dump_debug("process_indicate_packet: MSG_RUN_RESP");

				if (g_cfg->wimax_mode == SDIO_MODE || g_cfg->wimax_mode == DM_MODE
						|| g_cfg->wimax_mode == USB_MODE
						|| g_cfg->wimax_mode == USIM_RELAY_MODE) {

					dump_debug("%s: F/W Download Complete and Running ",__func__);
					dump_debug("Wait for SDIO ready...");
					msleep(1200);	/* IMPORTANT!! wait for cmc730 can handle mac req packet */

					kernel_thread((int (*)(void *))hw_get_mac_address, adapter, 0);
				} else if (g_cfg->wimax_mode == WTM_MODE) {
					adapter->download_complete = TRUE;
					wake_up_interruptible(&adapter->download_event);
					adapter->pdata->g_cfg->powerup_done = true ;
					adapter->wtm_task = kthread_create(
					con0_poll_thread,       adapter, "%s",
					"wimax_con0_poll_thread");
					if (adapter->wtm_task)
					  wake_up_process(
					  adapter->wtm_task);
				} else if (g_cfg->wimax_mode == AUTH_MODE) {
					adapter->download_complete = TRUE;
					wake_up_interruptible(&adapter->download_event);
					adapter->pdata->g_cfg->powerup_done = true ;
				}
			}
			break;
		default:
			dump_debug("process_indicate_packet: Unkown type");
			break;
		}
	}
	else
		dump_debug("process_indicate_packet - is not download pakcet");
}

u_long process_private_cmd(struct net_adapter *adapter, void *buffer)
{
	struct hw_private_packet	*cmd;
	u_char				*bufp;
	struct wimax_cfg *g_cfg = adapter->pdata->g_cfg;
	cmd = (struct hw_private_packet *)buffer;

	switch (cmd->code) {
	case HwCodeMacResponse: {
		u_char mac_addr[ETHERNET_ADDRESS_LENGTH];
		bufp = (u_char *)buffer;

		/* processing for mac_req request */
		#ifndef PRODUCT_SHIP
		dump_debug("MAC address = %02x:%02x:%02x:%02x:%02x:%02x",
				bufp[3], bufp[4], bufp[5], bufp[6], bufp[7], bufp[8]);
		#endif
		memcpy(mac_addr, bufp + 3, ETHERNET_ADDRESS_LENGTH);

		/* create ethernet header */
		memcpy(adapter->hw.eth_header, mac_addr, ETHERNET_ADDRESS_LENGTH);
		memcpy(adapter->hw.eth_header + ETHERNET_ADDRESS_LENGTH, mac_addr, ETHERNET_ADDRESS_LENGTH);
		adapter->hw.eth_header[(ETHERNET_ADDRESS_LENGTH * 2) - 1] += 1;

		memcpy(adapter->net->dev_addr, bufp + 3, ETHERNET_ADDRESS_LENGTH);
		adapter->mac_ready = TRUE;

		if (adapter->downloading) {
			adapter->download_complete = TRUE;
			wake_up_interruptible(&adapter->download_event);
		}
		return (sizeof(struct hw_private_packet) + ETHERNET_ADDRESS_LENGTH - sizeof(u_char));
	}
	case HwCodeLinkIndication: {
		if ((cmd->value == HW_PROT_VALUE_LINK_DOWN)
					&& (adapter->media_state != MEDIA_DISCONNECTED)) {
			dump_debug("LINK_DOWN_INDICATION");
			s3c_bat_use_wimax(0);

			/* set values */
			adapter->media_state = MEDIA_DISCONNECTED;

			/* indicate link down */
			netif_stop_queue(adapter->net);
			netif_carrier_off(adapter->net);
		} else if ((cmd->value == HW_PROT_VALUE_LINK_UP)
					&& (adapter->media_state != MEDIA_CONNECTED)) {
			dump_debug("LINK_UP_INDICATION");

			s3c_bat_use_wimax(1);
			/* set values */
			adapter->media_state = MEDIA_CONNECTED;
			adapter->net->mtu = WIMAX_MTU_SIZE;

			/* indicate link up */
			netif_start_queue(adapter->net);
			netif_carrier_on(adapter->net);
		}
		break;
	}
	case HwCodeHaltedIndication: {
		dump_debug("Device is about to reset, stop driver");
		break;
	}
	case HwCodeRxReadyIndication: {
		dump_debug("Device RxReady");
		/* to start the data packet send queue again after stopping in xmit */
		if (adapter->media_state == MEDIA_CONNECTED)
			netif_wake_queue(adapter->net);
		break;
	}
	case HwCodeIdleNtfy: {
		/* set idle / vi */

		dump_debug("process_private_cmd: HwCodeIdleNtfy");

		s3c_bat_use_wimax(0);
		break;
	}
	case HwCodeWakeUpNtfy: {
		/* IMPORTANT!! at least 4 sec is required after modem waked up */
		wake_lock_timeout(&g_cfg->wimax_wake_lock, 4 * HZ);
		dump_debug("process_private_cmd: HwCodeWakeUpNtfy");
		s3c_bat_use_wimax(1);
		break;
	}
	default:
		dump_debug("Command = %04x", cmd->code);
		break;
	}

	return sizeof(struct hw_private_packet);
}

u_int process_sdio_data(struct net_adapter *adapter,
				void *buffer, u_long length, long Timeout)
{
	struct hw_packet_header	*hdr;
	struct net_device	*net = adapter->net;
	u_char			*ofs = (u_char *)buffer;
	int			res = 0;
	u_int			machdrlen = (ETHERNET_ADDRESS_LENGTH * 2);
	u_int			rlen = length;
	u_long			type;
	u_short			data_len;
	struct sk_buff	*skb;
	while ((int)rlen > 0) {
		hdr = (struct hw_packet_header *)ofs;
		type = HwPktTypeNone;

		if (unlikely(hdr->id0 != 'W')) { /* "WD", "WC", "WP" or "WE" */
			if (rlen > 4) {
				dump_debug("Wrong packet ID (%02x %02x)", hdr->id0, hdr->id1);
				dump_buffer("Wrong packet", (u_char *)ofs, rlen);
			}
			/* skip rest of packets */
			return 0;
		}

		/* check packet type */
		switch (hdr->id1) {
		case 'P': {
			u_long l = 0;
			type = HwPktTypePrivate;

			/* process packet */
			l = process_private_cmd(adapter, ofs);

			/* shift */
			ofs += l;
			rlen -= l;

			/* process next packet */
			continue;
		}
		case 'C':
			type = HwPktTypeControl;
			break;
		case 'D':
			type = HwPktTypeData;
			break;
		case 'E':
			/* skip rest of buffer */
			return 0;
		default:
			dump_debug("hwParseReceivedData : Wrong packet ID [%02x %02x]",
									hdr->id0, hdr->id1);
			/* skip rest of buffer */
			return 0;
		}

		if (likely(!adapter->downloading)) {
			if (unlikely(hdr->length > WIMAX_MAX_TOTAL_SIZE
					|| ((hdr->length + sizeof(struct hw_packet_header)) > rlen))) {
				dump_debug("Packet length is too big (%d)", hdr->length);
				/* skip rest of packets */
				return 0;
			}
		}

		/* change offset */
		ofs += sizeof(struct hw_packet_header);
		rlen -= sizeof(struct hw_packet_header);

		/* process download packet, data and control packet */
		if (likely(!adapter->downloading))
		{
#ifdef HARDWARE_USE_ALIGN_HEADER
			ofs += 2;
			rlen -= 2;
#endif
			/* store the packet length */
			/* because we will overwrite the hardware packet header */
			data_len = hdr->length;
			/* copy the MAC to ofs buffer */
			memcpy((u_char *)ofs - machdrlen, adapter->hw.eth_header, machdrlen);

			if (unlikely(type == HwPktTypeControl))
				control_recv(adapter, (u_char *)ofs -machdrlen,
								data_len + machdrlen);
			else {

				skb = dev_alloc_skb(data_len + machdrlen + 2);
				if(!skb)
					{
					dump_debug("MEMORY PINCH: unable to allocate skb");
						return -ENOMEM;
					}
				skb_reserve(skb, 2);
				memcpy(skb_put(skb, (data_len + machdrlen)),
								(u_char *)ofs -machdrlen,
								(data_len + machdrlen));
				skb->dev = net;
				skb->protocol = eth_type_trans(skb, net);
				skb->ip_summed = CHECKSUM_UNNECESSARY;
				res = netif_rx(skb);

				adapter->netstats.rx_packets++;
				adapter->netstats.rx_bytes += (data_len + machdrlen);


			}
			/* restore the hardware packet length information */
			hdr->length = data_len;
		} else {
			hdr->length -= sizeof(struct hw_packet_header);
			process_indicate_packet(adapter, ofs);
		}
		/*
		 * If the packet is unreasonably long, quietly drop it rather than
		 * kernel panicing by calling skb_put.
		 */
		/* shift */
		ofs += hdr->length;
		rlen-= hdr->length;
	}

	return 0;
}
