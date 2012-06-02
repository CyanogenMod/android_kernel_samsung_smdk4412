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

#include <linux/ip.h>
#include "core.h"
#include "wmi_btcoex.h"
#include "debug.h"

#define BT_COEX_OP_ACTIVE	0x01
#define BT_COEX_OP_INACTIVE	0x02

#define BT_COEX_DEFAULT_LINK	0x00

static inline struct sk_buff *ath6kl_wmi_btcoex_get_new_buf(u32 size)
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
int ath6kl_wmi_set_btcoex_bt_op_status(struct wmi *wmi, u8 op_id, bool flag)
{
	struct sk_buff *skb;
	struct wmi_btcoex_bt_op_status_cmd *cmd;
	int op_status;

	skb = ath6kl_wmi_btcoex_get_new_buf(sizeof
				    (struct wmi_btcoex_bt_op_status_cmd));
	if (!skb)
		return -ENOMEM;

	if (flag)
		op_status = BT_COEX_OP_ACTIVE;
	else
		op_status = BT_COEX_OP_INACTIVE;

	cmd = (struct wmi_btcoex_bt_op_status_cmd *) skb->data;
	cmd->op_type = cpu_to_le32(op_id);
	cmd->op_status = cpu_to_le32(op_status);
	cmd->link_id = cpu_to_le32(BT_COEX_DEFAULT_LINK);

	ath6kl_dbg(ATH6KL_DBG_WMI,
		  "WMI_SET_BTCOEX_BT_OPERATING_STATUS_CMDID\n");
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tBT Op status->op_type: %x\n",
		   cmd->op_type);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tBT Op status->op_status: %x\n",
		   cmd->op_status);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tBT Op status->link_id: %x\n",
		   cmd->link_id);

	return ath6kl_wmi_cmd_send(wmi, 0, skb,
				  WMI_SET_BTCOEX_BT_OPERATING_STATUS_CMDID,
				  NO_SYNC_WMIFLAG);
}
static inline int btcoex_get_max_slot(int tx_pkt_len)
{
	int slot = 2;

	if (tx_pkt_len <= 90)
		slot = 1; /* EV3 or 2-EV3 or 3-EV3 */
	else
		slot = 3; /* EV4 or EV5 or 2-EV5 3-EV5 */

	slot *= 2;

	return slot;
}

#define SCO_SLOT_DEFAULT	2
#define SCO_IDLE_SLOT_DEFAULT	4
#define SCO_FLAGS_DEFAULT	0
#define SCO_LINK_ID_DEFAULT	0
#define SCO_CYCLES_FORCE_TRIGGER_DEFAULT	10
#define SCO_DATA_RES_TIMEOUT_DEFAULT	20
#define SCO_STOMP_DUTY_CYCLE_DEFAULT	2
#define SCO_STOMP_DUTY_CYCLE_MAX_DEFAULT	6
#define SCO_PS_POLL_LATENCY_FRACTION_DEFAULT	2
#define SCO_STOMP_COUNT_IN_100MS_DEFAULT	3
#define SCO_CONT_STOMP_MAX_DEFAULT	3
#define SCO_MIN_RATE_MBPS_DEFAULT	36
#define SCO_LOW_RATE_CNT_DEFAULT	5
#define SCO_HI_PKT_RATIO_DEFAULT	5
#define SCO_MAX_AGGR_SIZE_DEFAULT	1
#define SCO_SCAN_INTERVAL_DEFAULT	100
#define SCO_MAX_SCAN_STOMP_CNT_INTERVAL	2

static inline void set_default_sco(struct wmi_set_btcoex_sco_config_cmd *cmd)
{
	struct btcoex_sco_config *sco_config = &cmd->sco_config;
	struct btcoex_pspoll_mode_sco_config *ppoll_config =
						&cmd->sco_pspoll_config;
	struct btcoex_optmode_sco_config *optmode_config =
						&cmd->sco_optmode_config;
	struct btcoex_wlan_sco_config *wlan_config = &cmd->sco_wlan_config;


	sco_config->sco_slots = cpu_to_le32(SCO_SLOT_DEFAULT);
	sco_config->sco_idle_slots = cpu_to_le32(SCO_IDLE_SLOT_DEFAULT);
	sco_config->sco_flags = cpu_to_le32(SCO_FLAGS_DEFAULT);
	sco_config->link_id = cpu_to_le32(SCO_LINK_ID_DEFAULT);

	ppoll_config->sco_cycle_force_trigger =
				cpu_to_le32(SCO_CYCLES_FORCE_TRIGGER_DEFAULT);
	ppoll_config->sco_data_res_to =
				cpu_to_le32(SCO_DATA_RES_TIMEOUT_DEFAULT);
	ppoll_config->sco_stomp_duty_cycle_val =
				cpu_to_le32(SCO_STOMP_DUTY_CYCLE_DEFAULT);
	ppoll_config->sco_stomp_duty_cycle_max_val =
				cpu_to_le32(SCO_STOMP_DUTY_CYCLE_MAX_DEFAULT);
	ppoll_config->sco_pspoll_latency_fraction =
			cpu_to_le32(SCO_PS_POLL_LATENCY_FRACTION_DEFAULT);

	optmode_config->sco_stomp_cnt_in_100ms =
				cpu_to_le32(SCO_STOMP_COUNT_IN_100MS_DEFAULT);
	optmode_config->sco_cont_stomp_cnt_max =
				cpu_to_le32(SCO_CONT_STOMP_MAX_DEFAULT);
	optmode_config->sco_min_low_rate_mbps =
				cpu_to_le32(SCO_MIN_RATE_MBPS_DEFAULT);
	optmode_config->sco_low_rate_cnt =
				cpu_to_le32(SCO_LOW_RATE_CNT_DEFAULT);
	optmode_config->sco_hi_pkt_ratio =
				cpu_to_le32(SCO_HI_PKT_RATIO_DEFAULT);
	optmode_config->sco_max_aggr_size =
				cpu_to_le32(SCO_MAX_AGGR_SIZE_DEFAULT);

	wlan_config->scan_interval =
				cpu_to_le32(SCO_SCAN_INTERVAL_DEFAULT);
	wlan_config->max_scan_stomp_cnt =
				cpu_to_le32(SCO_MAX_SCAN_STOMP_CNT_INTERVAL);

}
#define WMI_SCO_CONFIG_FLAG_IS_EDR_CAPABLE       (1 << 1)

#define IDLE_SLOT_THREASHOLD	10
#define MAX_AGGR_SIZE_ABOVE_THREASHOLD	8
#define POLL_LATENCY_BELOW_THRESHOLD	1
#define POLL_LATENCY_ABOVE_THRESHOLD	2
#define POLL_LATENCY_BELOW_THRESHOLD_EDR	2
#define POLL_LATENCY_ABOVE_THRESHOLD_EDR	3
#define STOMP_CYCLE_ABOVE_THREASHOLD	2
#define STOMP_CYCLE_BELOW_THREASHOLD	5
#define STOMP_CYCLE_ABOVE_THREASHOLD_EDR	2
#define STOMP_CYCLE_BELOW_THREASHOLD_EDR	5
#define MAX_STOMP_CNT_BELOW_THRESHOLD	4
#define MAX_STOMP_CNT_ABOVE_THRESHOLD	2
#define MAX_STOMP_CNT_BELOW_THRESHOLD_EDR	4
#define MAX_STOMP_CNT_ABOVE_THRESHOLD_EDR	2
int ath6kl_wmi_set_btcoex_sco_op(struct wmi *wmi, bool esco, u32 tx_interval,
				 u32 tx_pkt_len)
{

	struct sk_buff *skb;
	int max_slot;

	struct wmi_set_btcoex_sco_config_cmd *cmd;
	struct btcoex_sco_config *sco_config;
	struct btcoex_pspoll_mode_sco_config *ppoll_config;
	struct btcoex_optmode_sco_config *optmode_config;
	struct btcoex_wlan_sco_config *wlan_config;

	skb = ath6kl_wmi_btcoex_get_new_buf(sizeof
				    (struct wmi_set_btcoex_sco_config_cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_btcoex_sco_config_cmd *)skb->data;
	set_default_sco(cmd);

	set_default_sco(cmd);
	sco_config = &cmd->sco_config;
	ppoll_config = &cmd->sco_pspoll_config;
	optmode_config = &cmd->sco_optmode_config;
	wlan_config = &cmd->sco_wlan_config;

	if (esco)
		max_slot = btcoex_get_max_slot(tx_pkt_len);
	else
		max_slot = SCO_SLOT_DEFAULT;

	sco_config->sco_slots = cpu_to_le32(max_slot);

	if (tx_interval >= max_slot)
		sco_config->sco_idle_slots =
				cpu_to_le32(tx_interval - max_slot);


	if (sco_config->sco_idle_slots >= IDLE_SLOT_THREASHOLD)
		optmode_config->sco_max_aggr_size =
				cpu_to_le32(MAX_AGGR_SIZE_ABOVE_THREASHOLD);

	if (esco) {
		sco_config->sco_flags |= WMI_SCO_CONFIG_FLAG_IS_EDR_CAPABLE;

		if (sco_config->sco_idle_slots >= IDLE_SLOT_THREASHOLD) {
			ppoll_config->sco_pspoll_latency_fraction =
				cpu_to_le32(POLL_LATENCY_ABOVE_THRESHOLD_EDR);
			ppoll_config->sco_stomp_duty_cycle_val =
				cpu_to_le32(STOMP_CYCLE_ABOVE_THREASHOLD_EDR);
			wlan_config->max_scan_stomp_cnt =
				cpu_to_le32(MAX_STOMP_CNT_ABOVE_THRESHOLD_EDR);
		} else {
			ppoll_config->sco_pspoll_latency_fraction =
				cpu_to_le32(POLL_LATENCY_BELOW_THRESHOLD_EDR);
			ppoll_config->sco_stomp_duty_cycle_val =
				cpu_to_le32(STOMP_CYCLE_BELOW_THREASHOLD_EDR);
			wlan_config->max_scan_stomp_cnt =
				cpu_to_le32(MAX_STOMP_CNT_BELOW_THRESHOLD_EDR);
		}
	} else {
		if (sco_config->sco_idle_slots >= IDLE_SLOT_THREASHOLD) {
			ppoll_config->sco_pspoll_latency_fraction =
				cpu_to_le32(POLL_LATENCY_ABOVE_THRESHOLD);
			ppoll_config->sco_stomp_duty_cycle_val =
				cpu_to_le32(STOMP_CYCLE_ABOVE_THREASHOLD);
			wlan_config->max_scan_stomp_cnt =
				cpu_to_le32(MAX_STOMP_CNT_ABOVE_THRESHOLD);
		} else {
			ppoll_config->sco_pspoll_latency_fraction =
				cpu_to_le32(POLL_LATENCY_BELOW_THRESHOLD);
			ppoll_config->sco_stomp_duty_cycle_val =
				cpu_to_le32(STOMP_CYCLE_BELOW_THREASHOLD);
			wlan_config->max_scan_stomp_cnt =
				cpu_to_le32(MAX_STOMP_CNT_BELOW_THRESHOLD);
		}
	}

	ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_SET_BTCOEX_SCO_CONFIG_CMDID\n");
	ath6kl_dbg(ATH6KL_DBG_WMI, "wmi_set_btcoex_sco_config_cmd :\n");
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tsco_config->sco_slots:%x\n",
		  sco_config->sco_slots);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tsco_config->sco_idle_slots:%x\n",
		   sco_config->sco_idle_slots);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tsco_config->sco_flags:%x\n",
		   sco_config->sco_flags);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tsco_config->link_id:%x\n",
		   sco_config->link_id);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\n");
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tpspoll_sco_config->sco_cycle_force_trigger %x\n",
		  ppoll_config->sco_cycle_force_trigger);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tpspoll_sco_config->sco_data_res_to %x\n",
		  ppoll_config->sco_data_res_to);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tpspoll_config->sco_stomp_duty_cycle_val %x\n",
		  ppoll_config->sco_stomp_duty_cycle_val);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tpspoll_config->sco_stomp_duty_cycle_max_val %x\n",
		  ppoll_config->sco_stomp_duty_cycle_max_val);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tpspoll_config->sco_pspoll_latency_fraction %x\n",
		  ppoll_config->sco_pspoll_latency_fraction);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\n");
	ath6kl_dbg(ATH6KL_DBG_WMI, "\toptmode_config->sco_stomp_cnt_in_100ms %x\n",
		  optmode_config->sco_stomp_cnt_in_100ms);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\toptmode_config->sco_cont_stomp_cnt_max %x\n",
		  optmode_config->sco_cont_stomp_cnt_max);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\toptmode_config->sco_min_low_rate_mbps %x\n",
		  optmode_config->sco_min_low_rate_mbps);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\toptmode_config->sco_low_rate_cnt %x\n",
		  optmode_config->sco_low_rate_cnt);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\toptmode_config->sco_hi_pkt_ratio %x\n",
		  optmode_config->sco_hi_pkt_ratio);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\toptmode_config->sco_max_aggr_size %x\n",
		  optmode_config->sco_max_aggr_size);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\n");
	ath6kl_dbg(ATH6KL_DBG_WMI, "\twlan_config->scan_interval %x\n",
		  wlan_config->scan_interval);
	ath6kl_dbg(ATH6KL_DBG_WMI, "\twlan_config->max_scan_stomp_cnt %x\n",
		  wlan_config->max_scan_stomp_cnt);

	return ath6kl_wmi_cmd_send(wmi, 0, skb,
				  WMI_SET_BTCOEX_SCO_CONFIG_CMDID,
				  NO_SYNC_WMIFLAG);
}
#define BTCOEX_A2DP_FLAG_DEFAULT	0
#define BTCOEX_A2DP_LINK_ID_DEFAULT	0
#define BTCOEX_A2DP_WLAN_MAX_DUR_DEFAULT	30
#define BTCOEX_A2DP_MIN_BURST_CNT_DEFAULT	3
#define BTCOEX_A2DP_DATA_RESP_TO_DEFAULT	20
#define BTCOEX_A2DP_LOW_RATE_MBPS_DEFAULT	36
#define BTCOEX_A2DP_LOW_RATE_CNT_DEFAULT	5
#define BTCOEX_A2DP_HI_PKT_RATIO_DEFAULT	5
#define BTCOEX_A2DP_MAX_AGGR_SIZE	1
#define BTCOEX_A2DP_PKT_STOMP_CNT_DEFAULT	6

static inline void dump_a2dp_cmd(struct wmi_set_btcoex_a2dp_config_cmd *cmd)
{
	struct btcoex_a2dp_config *a2dp_config = &cmd->a2dp_config;
	struct btcoex_pspoll_a2dp_config *pspoll_config = &cmd->pspoll_config;
	struct btcoex_a2dp_optmode_config *optmode_config =
							&cmd->optmode_config;

	ath6kl_dbg(ATH6KL_DBG_WMI, "a2dp_config->a2dp_flags:%x\n",
		  a2dp_config->a2dp_flags);
	ath6kl_dbg(ATH6KL_DBG_WMI, "a2dp_config->link_id:%x\n",
		  a2dp_config->link_id);
	ath6kl_dbg(ATH6KL_DBG_WMI, "pspoll_config->a2dp_wlan_max_dur:%x\n",
		  pspoll_config->a2dp_wlan_max_dur);
	ath6kl_dbg(ATH6KL_DBG_WMI, "pspoll_config->a2dp_min_bus_cnt:%x\n",
		  pspoll_config->a2dp_min_bus_cnt);

	ath6kl_dbg(ATH6KL_DBG_WMI, "optmode_config->a2dp_min_low_rate_mbps:%x\n",
		  optmode_config->a2dp_min_low_rate_mbps);
	ath6kl_dbg(ATH6KL_DBG_WMI, "optmode_config->a2dp_low_rate_cnt:%x\n",
		  optmode_config->a2dp_low_rate_cnt);
	ath6kl_dbg(ATH6KL_DBG_WMI, "optmode_config->a2dp_hi_pkt_ratio:%x\n",
		  optmode_config->a2dp_hi_pkt_ratio);
	ath6kl_dbg(ATH6KL_DBG_WMI, "optmode_config->a2dp_max_aggr_size:%x\n",
		  optmode_config->a2dp_max_aggr_size);
	ath6kl_dbg(ATH6KL_DBG_WMI, "optmode_config->a2dp_pkt_stomp_cnt:%x\n",
		  optmode_config->a2dp_pkt_stomp_cnt);
}

static inline void set_default_a2dp(struct wmi_set_btcoex_a2dp_config_cmd *cmd)
{
	struct btcoex_a2dp_config *a2dp_config = &cmd->a2dp_config;
	struct btcoex_pspoll_a2dp_config *pspoll_config = &cmd->pspoll_config;
	struct btcoex_a2dp_optmode_config *optmode_config =
							&cmd->optmode_config;

	a2dp_config->a2dp_flags = cpu_to_le32(BTCOEX_A2DP_FLAG_DEFAULT);
	a2dp_config->link_id = cpu_to_le32(BTCOEX_A2DP_LINK_ID_DEFAULT);

	pspoll_config->a2dp_wlan_max_dur =
				cpu_to_le32(BTCOEX_A2DP_WLAN_MAX_DUR_DEFAULT);
	pspoll_config->a2dp_min_bus_cnt =
				cpu_to_le32(BTCOEX_A2DP_MIN_BURST_CNT_DEFAULT);
	pspoll_config->a2dp_data_res_to =
				cpu_to_le32(BTCOEX_A2DP_DATA_RESP_TO_DEFAULT);

	optmode_config->a2dp_min_low_rate_mbps =
				cpu_to_le32(BTCOEX_A2DP_LOW_RATE_MBPS_DEFAULT);
	optmode_config->a2dp_low_rate_cnt =
				cpu_to_le32(BTCOEX_A2DP_LOW_RATE_CNT_DEFAULT);
	optmode_config->a2dp_hi_pkt_ratio =
				cpu_to_le32(BTCOEX_A2DP_HI_PKT_RATIO_DEFAULT);
	optmode_config->a2dp_max_aggr_size =
				cpu_to_le32(BTCOEX_A2DP_MAX_AGGR_SIZE);
	optmode_config->a2dp_pkt_stomp_cnt =
				cpu_to_le32(BTCOEX_A2DP_PKT_STOMP_CNT_DEFAULT);
}

#define BTCOEX_A2DP_WLAN_MAX_DUR_QCOM_BT	25
#define BTCOEX_A2DP_MIN_BURST_CNT_QCOM_BT	3
#define BTCOEX_A2DP_PKT_STOMP_CNT_QCOM_BT	2
#define A2DP_CONFIG_ALLOW_OPTIMIZATION	(1 << 0)
static inline void set_qcom_a2dp(struct wmi_set_btcoex_a2dp_config_cmd *cmd)
{
	struct btcoex_a2dp_config *a2dp_config = &cmd->a2dp_config;
	struct btcoex_pspoll_a2dp_config *pspoll_config = &cmd->pspoll_config;
	struct btcoex_a2dp_optmode_config *optmode_config =
							&cmd->optmode_config;

	a2dp_config->a2dp_flags |= cpu_to_le32(A2DP_CONFIG_ALLOW_OPTIMIZATION);

	pspoll_config->a2dp_wlan_max_dur =
				cpu_to_le32(BTCOEX_A2DP_WLAN_MAX_DUR_QCOM_BT);
	pspoll_config->a2dp_min_bus_cnt =
				cpu_to_le32(BTCOEX_A2DP_MIN_BURST_CNT_QCOM_BT);

	optmode_config->a2dp_pkt_stomp_cnt =
				cpu_to_le32(BTCOEX_A2DP_PKT_STOMP_CNT_QCOM_BT);
}

#define BT_VER_1_0	0
#define BT_VER_1_1	1
#define BT_VER_1_2	2
#define BT_VER_2_0	3
#define BT_VER_2_1	4
#define BT_VER_3_0	5
#define BT_VER_4_0	6

#define BTCOEX_A2DP_WLAN_MAX_DUR_BDR	30
#define BTCOEX_A2DP_DATA_RESP_TO_BDR	10
#define BTCOEX_A2DP_MIN_BURST_CNT_BDR	4

#define BTCOEX_A2DP_LOW_RATE_MBPS_BDR	52
#define BTCOEX_A2DP_MAX_AGGR_SIZE_BDR	1

#define BTCOEX_A2DP_DATA_RESP_TO_EDR	20
#define BTCOEX_A2DP_WLAN_MAX_DUR_EDR	50
#define BTCOEX_A2DP_LOW_RATE_MBPS_EDR	36
#define BTCOEX_A2DP_MAX_AGGR_SIZE_EDR	16
#define BTCOEX_A2DP_MIN_BURST_CNT_EDR	2
static inline void update_lmp_ver(struct wmi_set_btcoex_a2dp_config_cmd *cmd,
				  u32 lmp_ver)
{
	struct btcoex_pspoll_a2dp_config *pspoll_config = &cmd->pspoll_config;
	struct btcoex_a2dp_optmode_config *optmode_config =
							&cmd->optmode_config;

	switch (lmp_ver) {
	case BT_VER_1_0:
	case BT_VER_1_1:
	case BT_VER_1_2: /* BDR */
		pspoll_config->a2dp_wlan_max_dur =
				cpu_to_le32(BTCOEX_A2DP_WLAN_MAX_DUR_BDR);
		pspoll_config->a2dp_min_bus_cnt =
				cpu_to_le32(BTCOEX_A2DP_MIN_BURST_CNT_BDR);
		pspoll_config->a2dp_data_res_to =
				cpu_to_le32(BTCOEX_A2DP_DATA_RESP_TO_BDR);

		optmode_config->a2dp_min_low_rate_mbps =
				cpu_to_le32(BTCOEX_A2DP_LOW_RATE_MBPS_BDR);
		optmode_config->a2dp_max_aggr_size =
				cpu_to_le32(BTCOEX_A2DP_MAX_AGGR_SIZE_BDR);
	break;
	default: /* EDR */
		pspoll_config->a2dp_data_res_to =
				cpu_to_le32(BTCOEX_A2DP_DATA_RESP_TO_EDR);
		pspoll_config->a2dp_wlan_max_dur =
				cpu_to_le32(BTCOEX_A2DP_WLAN_MAX_DUR_EDR);
		pspoll_config->a2dp_min_bus_cnt =
				cpu_to_le32(BTCOEX_A2DP_MIN_BURST_CNT_EDR);

		optmode_config->a2dp_min_low_rate_mbps =
				cpu_to_le32(BTCOEX_A2DP_LOW_RATE_MBPS_EDR);
		optmode_config->a2dp_max_aggr_size =
				cpu_to_le32(BTCOEX_A2DP_MAX_AGGR_SIZE_EDR);
	}
}

#define BTCOEX_ACL_ROLE_UNKNOWN	0
#define BTCOEX_ACL_ROLE_MASTER	1
#define BTCOEX_ACL_ROLE_SLAVE	2

#define A2DP_CONFIG_IS_MASTER	(1 << 2)
#define BTCOEX_A2DP_WLAN_MAX_DUR_SLAVE	30
static inline void update_acl_role(struct wmi_set_btcoex_a2dp_config_cmd *cmd,
				   u32 role)
{
	struct btcoex_a2dp_config *a2dp_config = &cmd->a2dp_config;
	struct btcoex_pspoll_a2dp_config *pspoll_config = &cmd->pspoll_config;

	if (role == BTCOEX_ACL_ROLE_UNKNOWN)
		return;

	if (role == BTCOEX_ACL_ROLE_MASTER) {
		a2dp_config->a2dp_flags |= cpu_to_le32(A2DP_CONFIG_IS_MASTER);
	} else {
		a2dp_config->a2dp_flags &= ~cpu_to_le32(A2DP_CONFIG_IS_MASTER);
		pspoll_config->a2dp_wlan_max_dur =
				cpu_to_le32(BTCOEX_A2DP_WLAN_MAX_DUR_SLAVE);
	}
}

#define BT_VENDOR_DEFAULT	0
#define BT_VENDOR_QCOM	1

int ath6kl_wmi_set_btcoex_a2dp_op(struct wmi *wmi, u32 role, u32 ver,
				  u32 vendor)
{
	struct wmi_set_btcoex_a2dp_config_cmd *cmd;
	struct sk_buff *skb;

	skb = ath6kl_wmi_btcoex_get_new_buf(sizeof
				    (struct wmi_set_btcoex_a2dp_config_cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_btcoex_a2dp_config_cmd *)skb->data;
	set_default_a2dp(cmd);

	update_acl_role(cmd, role);
	update_lmp_ver(cmd, ver);

	if (vendor == BT_VENDOR_QCOM)
		set_qcom_a2dp(cmd);

	ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_SET_BTCOEX_A2DP_CONFIG_CMDID\n");
	dump_a2dp_cmd(cmd);

	return ath6kl_wmi_cmd_send(wmi, 0, skb,
				  WMI_SET_BTCOEX_A2DP_CONFIG_CMDID,
				  NO_SYNC_WMIFLAG);
}

int ath6kl_wmi_set_btcoex_set_colocated_bt(struct wmi *wmi, u8 dev_type)
{
	struct wmi_set_btcoex_colocated_bt_dev_cmd *cmd;
	struct sk_buff *skb;

	skb = ath6kl_wmi_btcoex_get_new_buf(sizeof
				(struct wmi_set_btcoex_colocated_bt_dev_cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_btcoex_colocated_bt_dev_cmd *)skb->data;
	cmd->colocated_bt_dev = dev_type;

	ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_SET_BTCOEX_COLOCATED_BT_DEV_CMDID:\n");
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tCo-Located BT: %x\n",
		   cmd->colocated_bt_dev);

	return ath6kl_wmi_cmd_send(wmi, 0, skb,
				  WMI_SET_BTCOEX_COLOCATED_BT_DEV_CMDID,
				  NO_SYNC_WMIFLAG);
}
int ath6kl_wmi_set_btcoex_set_fe_antenna(struct wmi *wmi, u8 antenna_type)
{
	struct wmi_set_btcoex_fe_antenna_cmd *cmd;
	struct sk_buff *skb;

	skb = ath6kl_wmi_btcoex_get_new_buf(sizeof
				(struct wmi_set_btcoex_fe_antenna_cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_btcoex_fe_antenna_cmd *)skb->data;
	cmd->fe_antenna_type = antenna_type;

	ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_SET_BTCOEX_FE_ANT_CMDID:\n");
	ath6kl_dbg(ATH6KL_DBG_WMI, "\tFe antenna Type: %x\n",
		  cmd->fe_antenna_type);

	return ath6kl_wmi_cmd_send(wmi, 0, skb,
				  WMI_SET_BTCOEX_FE_ANT_CMDID,
				  NO_SYNC_WMIFLAG);
}

static int ath6kl_get_wmi_cmd(int nl_cmd)
{
	int wmi_cmd = 0;
	switch (nl_cmd) {
	case NL80211_WMI_SET_BT_STATUS:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT status\n");
		wmi_cmd = WMI_SET_BT_STATUS_CMDID;
		break;

	case NL80211_WMI_SET_BT_PARAMS:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT params\n");
		wmi_cmd = WMI_SET_BT_PARAMS_CMDID;
		break;

	case NL80211_WMI_SET_BT_FT_ANT:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT FT antenna\n");
		wmi_cmd = WMI_SET_BTCOEX_FE_ANT_CMDID;
		break;

	case NL80211_WMI_SET_COLOCATED_BT_DEV:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT collocated dev\n");
		wmi_cmd = WMI_SET_BTCOEX_COLOCATED_BT_DEV_CMDID;
		break;

	case NL80211_WMI_SET_BT_INQUIRY_PAGE_CONFIG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT inquiry page\n");
		wmi_cmd = WMI_SET_BTCOEX_BTINQUIRY_PAGE_CONFIG_CMDID;
		break;

	case NL80211_WMI_SET_BT_SCO_CONFIG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT sco config\n");
		wmi_cmd = WMI_SET_BTCOEX_SCO_CONFIG_CMDID;
		break;

	case NL80211_WMI_SET_BT_A2DP_CONFIG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT a2dp config\n");
		wmi_cmd = WMI_SET_BTCOEX_A2DP_CONFIG_CMDID;
		break;

	case NL80211_WMI_SET_BT_ACLCOEX_CONFIG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT acl config\n");
		wmi_cmd = WMI_SET_BTCOEX_ACLCOEX_CONFIG_CMDID;
		break;

	case NL80211_WMI_SET_BT_DEBUG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT bt debug\n");
		wmi_cmd = WMI_SET_BTCOEX_DEBUG_CMDID;
		break;

	case NL80211_WMI_SET_BT_OPSTATUS:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT op status\n");
		wmi_cmd = WMI_SET_BTCOEX_BT_OPERATING_STATUS_CMDID;
		break;

	case NL80211_WMI_GET_BT_CONFIG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Get BT config\n");
		wmi_cmd = WMI_GET_BTCOEX_CONFIG_CMDID;
		break;

	case NL80211_WMI_GET_BT_STATS:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Get BT status\n");
		wmi_cmd = WMI_GET_BTCOEX_STATS_CMDID;
		break;
	}
	return wmi_cmd;
}

int ath6kl_wmi_send_btcoex_cmd(struct wmi *wmi,
			u8 *buf, int len)
{
	struct sk_buff *skb;
	u32 nl_cmd;
	int wmi_cmd;

	nl_cmd = *(u32 *)buf;
	buf += sizeof(u32);
	len -= sizeof(u32);
	wmi_cmd = ath6kl_get_wmi_cmd(nl_cmd);
	if (wmi_cmd == 0)
		return -ENOMEM;

	skb = ath6kl_wmi_btcoex_get_new_buf(len);
	if (!skb)
		return -ENOMEM;

	memcpy(skb->data, buf, len);

	return ath6kl_wmi_cmd_send(wmi, 0, skb,
			(enum wmi_cmd_id)wmi_cmd,
			NO_SYNC_WMIFLAG);
}
