/*
 * Copyright (c) 2004-2011 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "core.h"
#include "debug.h"

static bool ath6kl_parse_event_pkt_for_wake_lock(struct ath6kl *ar,
						 struct sk_buff *skb)
{
	u16 cmd_id;
	bool need_wake = false;

	if (skb->len < sizeof(u16))
		return need_wake;

	 cmd_id = *(const u16 *) skb->data;
	 cmd_id = le16_to_cpu(cmd_id);

	if (test_and_clear_bit(WOW_RESUME_PRINT, &ar->flag)) {
		if (cmd_id == WMI_CONNECT_EVENTID)
			ath6kl_dbg(ATH6KL_DBG_SUSPEND,
			    "(wow) WMI_CONNECT_EVENTID\n");
		else
			ath6kl_dbg(ATH6KL_DBG_SUSPEND,
			    "(wow) wmi event id : 0x%x\n", cmd_id);
	}

	 switch (cmd_id) {
	 case WMI_CONNECT_EVENTID:
		need_wake = true;
		break;
	 default:
		/* Don't wake lock the system for other events */
		break;
	 }

	 return need_wake;
}

static bool ath6kl_parse_ip_pkt_for_wake_lock(struct sk_buff *skb)
{
	const u8 ipsec_keepalve[] = { 0x11, 0x94, 0x11, 0x94, 0x00,
				      0x09, 0x00, 0x00, 0xff };
	bool need_wake = true;
	u16 size;
	u8 *udp;
	u8 ihl;

	if (skb->len >= 24 && (*((u8 *)skb->data + 23) == 0x11)) {

		udp = (u8 *)skb->data + 14;
		ihl = (*udp & 0x0f) * sizeof(u32);
		udp += ihl;
		size = 14 + ihl + sizeof(ipsec_keepalve);

		if ((skb->len >= size) &&
		     !memcmp(udp, ipsec_keepalve, sizeof(ipsec_keepalve) - 3) &&
			udp[8] == 0xff) {
			/*
			 * RFC 3948 UDP Encapsulation of IPsec ESP Packets
			 * src and dest port must be 4500
			 * Receivers MUST NOT depend upon the UDP checksum
			 * being zero
			 * Sender must use 1 byte payload with 0xff
			 * Receiver SHOULD ignore a received NAT-keepalive pkt
			 *
			 * IPSec over UDP NAT keepalive packet. Just ignore
			 */
			need_wake = false;
		}
	}

	return need_wake;
}

static bool ath6kl_parse_data_pkt_for_wake_lock(struct ath6kl *ar,
						struct sk_buff *skb)
{
	struct net_device *ndev;
	struct ath6kl_vif *vif;
	struct ethhdr *hdr;
	bool need_wake = false;
	u16 dst_port;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return need_wake;

	if (skb->len < sizeof(struct ethhdr))
		return need_wake;

	hdr = (struct ethhdr *) skb->data;

	if (test_and_clear_bit(WOW_RESUME_PRINT, &ar->flag)) {
		ath6kl_dbg(ATH6KL_DBG_SUSPEND,
			   "(wow) dest mac:%s, src mac:%s, type/len :%04x\n",
			   sec_conv_mac(hdr->h_dest),
			   sec_conv_mac(hdr->h_source),
			   be16_to_cpu(hdr->h_proto));
	}

	if (!is_multicast_ether_addr(hdr->h_dest)) {
		switch (ntohs(hdr->h_proto)) {
		case 0x0800: /* IP */
			need_wake = ath6kl_parse_ip_pkt_for_wake_lock(skb);
			break;
		case 0x888e: /* EAPOL */
		case 0x88c7: /* RSN_PREAUTH */
		case 0x88b4: /* WAPI */
			need_wake = true;
			break;
		default:
			break;
		}
	} else if (!is_broadcast_ether_addr(hdr->h_dest)) {
		if (skb->len >= 14 + 20) { /* mDNS packets */
			u8 *dst_ipaddr = (u8 *)(skb->data + 14 + 20 - 4);
			ndev = vif->ndev;
			if (((dst_ipaddr[3] & 0xf8) == 0xf8) &&
				(vif->nw_type == AP_NETWORK ||
				(ndev->flags & IFF_ALLMULTI ||
				ndev->flags & IFF_MULTICAST)))
					need_wake = true;
		}
	} else if (vif->nw_type == AP_NETWORK) {
		switch (ntohs(hdr->h_proto)) {
		case 0x0800: /* IP */
			if (skb->len >= 14 + 20 + 2) {
				dst_port = *(u16 *)(skb->data + 14 + 20);
				/* dhcp req */
				need_wake = (ntohs(dst_port) == 0x43);
			}
			break;
		case 0x0806:
			need_wake = true;
		default:
			break;
		}
	}

	return need_wake;
}

void ath6kl_config_suspend_wake_lock(struct ath6kl *ar, struct sk_buff *skb,
				     bool is_event_pkt)
{
	struct ath6kl_vif *vif;
#ifdef CONFIG_HAS_WAKELOCK
	unsigned long wl_timeout = 5;
#endif
	bool need_wake = false;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return;

	if (
#ifdef CONFIG_HAS_EARLYSUSPEND
			ar->screen_off &&
#endif
			skb && test_bit(CONNECTED, &vif->flags)) {
		if (is_event_pkt) { /* Ctrl pkt received */
			need_wake =
				ath6kl_parse_event_pkt_for_wake_lock(ar, skb);
			if (need_wake) {
#ifdef CONFIG_HAS_WAKELOCK
				wl_timeout = 3 * HZ;
#endif
			}
		} else /* Data pkt received */
			need_wake = ath6kl_parse_data_pkt_for_wake_lock(ar,
									skb);
	}

	if (need_wake) {
#ifdef CONFIG_HAS_WAKELOCK
		/*
		 * Keep the host wake up if there is any event
		 * and pkt comming in.
		 */
		wake_lock_timeout(&ar->wake_lock, wl_timeout);
#endif
	}
}
#ifdef CONFIG_HAS_WAKELOCK
void ath6kl_p2p_acquire_wakelock(struct ath6kl *ar, int wl_timeout)
{
	if (!wake_lock_active(&ar->p2p_wake_lock))
		wake_lock_timeout(&ar->p2p_wake_lock, wl_timeout);
	return;
}

void ath6kl_p2p_release_wakelock(struct ath6kl *ar)
{
	if (wake_lock_active(&ar->p2p_wake_lock))
		wake_unlock(&ar->p2p_wake_lock);
	return;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ath6kl_early_suspend(struct early_suspend *handler)
{
	struct ath6kl *ar = container_of(handler, struct ath6kl, early_suspend);

	if (ar)
		ar->screen_off = true;
}

static void ath6kl_late_resume(struct early_suspend *handler)
{
	struct ath6kl *ar = container_of(handler, struct ath6kl, early_suspend);

	if (ar)
		ar->screen_off = false;
}
#endif

void ath6kl_setup_android_resource(struct ath6kl *ar)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	ar->screen_off = false;
	ar->early_suspend.suspend = ath6kl_early_suspend;
	ar->early_suspend.resume = ath6kl_late_resume;
	ar->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	register_early_suspend(&ar->early_suspend);
#endif
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_init(&ar->wake_lock, WAKE_LOCK_SUSPEND, "ath6kl_suspend_wl");
	wake_lock_init(&ar->p2p_wake_lock,
			WAKE_LOCK_SUSPEND,
			"ath6kl_p2p_suspend_wl");
#endif
}

void ath6kl_cleanup_android_resource(struct ath6kl *ar)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ar->early_suspend);
#endif
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_destroy(&ar->wake_lock);
	wake_lock_destroy(&ar->p2p_wake_lock);
#endif
}
