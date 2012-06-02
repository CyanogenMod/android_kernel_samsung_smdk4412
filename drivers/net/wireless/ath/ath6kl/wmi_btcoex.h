/*
 * Copyright (c) 2010-2011 Atheros Communications Inc.
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

/*
 * This file contains the definitions of the WMI protocol specified in the
 * Wireless Module Interface (WMI).  It includes definitions of all the
 * commands and events. Commands are messages from the host to the WM.
 * Events and Replies are messages from the WM to the host.
 */

#ifndef WMI_BTCOEX_H
#define WMI_BTCOEX_H

#include <linux/ieee80211.h>

/*
 *  WMI_SET_BTCOEX_BT_OPERATING_STATUS_CMDID
 *  Setting the Bluetooth operation status.
 */
struct wmi_btcoex_bt_op_status_cmd {
	__le32 op_type;
	__le32 op_status;
	__le32 link_id;
} __packed;

/*
 *  WMI_SET_BTCOEX_SCO_CONFIG_CMDID
 *  Setting the Bluetooth operation status.
 */
struct btcoex_sco_config {
	__le32 sco_slots;
	__le32 sco_idle_slots;
	__le32 sco_flags;
	__le32 link_id;
} __packed;

struct btcoex_pspoll_mode_sco_config {
	__le32 sco_cycle_force_trigger;
	__le32 sco_data_res_to;
	__le32 sco_stomp_duty_cycle_val;
	__le32 sco_stomp_duty_cycle_max_val;
	__le32 sco_pspoll_latency_fraction;
} __packed;

struct btcoex_optmode_sco_config {
	__le32 sco_stomp_cnt_in_100ms;
	__le32 sco_cont_stomp_cnt_max;
	__le32 sco_min_low_rate_mbps;
	__le32 sco_low_rate_cnt;
	__le32 sco_hi_pkt_ratio;
	__le32 sco_max_aggr_size;
} __packed;

struct btcoex_wlan_sco_config {
	__le32 scan_interval;
	__le32 max_scan_stomp_cnt;
} __packed;

struct wmi_set_btcoex_sco_config_cmd {
	struct btcoex_sco_config sco_config;
	struct btcoex_pspoll_mode_sco_config sco_pspoll_config;
	struct btcoex_optmode_sco_config sco_optmode_config;
	struct btcoex_wlan_sco_config sco_wlan_config;
} __packed;
/*
 * WMI_SET_BTCOEX_A2DP_CONFIG_CMDID
 *  Setting the Bluetooth A2DP configuration operation.
 */
struct btcoex_a2dp_config {
	__le32 a2dp_flags;
	__le32 link_id;
} __packed;

struct btcoex_pspoll_a2dp_config {
	__le32 a2dp_wlan_max_dur;
	__le32 a2dp_min_bus_cnt;
	__le32 a2dp_data_res_to;

} __packed;

struct btcoex_a2dp_optmode_config {
	__le32 a2dp_min_low_rate_mbps;
	__le32 a2dp_low_rate_cnt;
	__le32 a2dp_hi_pkt_ratio;
	__le32 a2dp_max_aggr_size;
	__le32 a2dp_pkt_stomp_cnt;
} __packed;

struct wmi_set_btcoex_a2dp_config_cmd {
	struct btcoex_a2dp_config a2dp_config;
	struct btcoex_pspoll_a2dp_config pspoll_config;
	struct btcoex_a2dp_optmode_config optmode_config;
} __packed;

struct wmi_set_btcoex_colocated_bt_dev_cmd {
	u8 colocated_bt_dev;
} __packed;

struct wmi_set_btcoex_fe_antenna_cmd {
	u8 fe_antenna_type;
} __packed;

/* BT Coex */
int ath6kl_wmi_set_btcoex_bt_op_status(struct wmi *wmi, u8 op_id, bool flag);
int ath6kl_wmi_set_btcoex_sco_op(struct wmi *wmi, bool esco, u32 tx_interval,
				 u32 tx_pkt_len);
int ath6kl_wmi_set_btcoex_a2dp_op(struct wmi *wmi, u32 role, u32 ver, u32 ven);
int ath6kl_wmi_set_btcoex_set_colocated_bt(struct wmi *wmi, u8 dev_type);
int ath6kl_wmi_set_btcoex_set_fe_antenna(struct wmi *wmi, u8 antenna_type);
int ath6kl_wmi_send_btcoex_cmd(struct wmi *wmi,
				u8 *buf, int len);
#endif /* WMI_BTCOEX_H */
