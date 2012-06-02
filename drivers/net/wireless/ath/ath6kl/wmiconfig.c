/*
 * Copyright (c) 2010-2011 Atheros Communications Inc.
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
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

#include "testmode.h"
#include "debug.h"
#include "wmi.h"

#include <net/netlink.h>

enum ath6kl_tm_attr {
	__ATH6KL_TM_ATTR_INVALID	= 0,
	ATH6KL_TM_ATTR_CMD		= 1,
	ATH6KL_TM_ATTR_DATA		= 2,

	/* keep last */
	__ATH6KL_TM_ATTR_AFTER_LAST,
	ATH6KL_TM_ATTR_MAX		= __ATH6KL_TM_ATTR_AFTER_LAST - 1,
};

enum ath6kl_tm_cmd {
	ATH6KL_TM_CMD_TCMD		= 0,
	ATH6KL_TM_CMD_RX_REPORT		= 1,	/* not used anymore */
	ATH6KL_TM_CMD_WMI_CMD		= 0xF000,
};

struct sk_buff *ath6kl_wmi_get_buf(u32 size)
{
	struct sk_buff *skb;

	skb = ath6kl_buf_alloc(size);
	if (!skb)
		return NULL;

	skb_put(skb, size);
	if (size)
		memset(skb->data, 0, size);

	return skb;
}
void ath6kl_tm_rx_wmi_event(struct ath6kl *ar, void *buf, size_t buf_len)
{
	struct sk_buff *skb;


	if (!buf || buf_len == 0)
		return;

	skb = cfg80211_testmode_alloc_event_skb(ar->wiphy, buf_len, GFP_KERNEL);
	if (!skb) {
		ath6kl_warn("failed to allocate testmode rx skb!\n");
		return;
	}
	NLA_PUT_U32(skb, ATH6KL_TM_ATTR_CMD, ATH6KL_TM_CMD_WMI_CMD);
	NLA_PUT(skb, ATH6KL_TM_ATTR_DATA, buf_len, buf);
	cfg80211_testmode_event(skb, GFP_KERNEL);
	return;

nla_put_failure:
	kfree_skb(skb);
	ath6kl_warn("nla_put failed on testmode rx skb!\n");
}


void ath6kl_wmicfg_send_stats(struct ath6kl_vif *vif,
			      struct target_stats *stats)
{
	u32 *buff = kzalloc(sizeof(*stats) + 4, GFP_KERNEL);

	buff[0] = WMI_REPORT_STATISTICS_EVENTID;
	memcpy(buff+1, stats, sizeof(struct target_stats));
	ath6kl_tm_rx_wmi_event(vif->ar->wmi->parent_dev, buff,
			       sizeof(struct target_stats)+4);
	kfree(buff);
}
