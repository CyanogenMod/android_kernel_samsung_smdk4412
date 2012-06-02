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

int ath6kl_notify_btcoex_inq_status(struct wiphy *wiphy, bool status);
int ath6kl_notify_btcoex_sco_status(struct wiphy *wiphy,  bool status,
				    bool esco, u32 tx_interval,
				    u32 tx_pkt_len);
int ath6kl_notify_btcoex_a2dp_status(struct wiphy *wiphy, bool status);
int ath6kl_notify_btcoex_acl_info(struct wiphy *wiphy,
				  enum nl80211_btcoex_acl_role role,
				  u32 lmp_ver);
int ath6kl_notify_btcoex_antenna_config(struct wiphy *wiphy,
				enum nl80211_btcoex_antenna_config config);
int ath6kl_notify_btcoex_bt_vendor(struct wiphy *wiphy,
				   enum nl80211_btcoex_vendor_list vendor);
int ath6kl_notify_btcoex(struct wiphy *wiphy, u8 *buf,
				int len);
