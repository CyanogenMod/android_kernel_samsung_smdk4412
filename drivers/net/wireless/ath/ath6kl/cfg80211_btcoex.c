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

#include <linux/moduleparam.h>
#include <linux/inetdevice.h>

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif

#include "core.h"
#include "cfg80211.h"
#include "debug.h"
#include "hif-ops.h"
#include "testmode.h"
#include "cfg80211_btcoex.h"

bool ath6kl_btcoex_cfg80211_ready(struct ath6kl_vif *vif)
{
	struct ath6kl *ar = vif->ar;

	if (!test_bit(WMI_READY, &ar->flag)) {
		ath6kl_err("wmi is not ready\n");
		return false;
	}

	if (!test_bit(WLAN_ENABLED, &vif->flags)) {
		ath6kl_err("wlan disabled\n");
		return false;
	}

	return true;
}
#define OP_TYPE_SCO	0x01
#define OP_TYPE_A2DP	0x02
#define OP_TYPE_INQUIRY	0x03
#define OP_TYPE_ESCO	0x04

int ath6kl_notify_btcoex_inq_status(struct wiphy *wiphy, bool status)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct ath6kl_vif *vif;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "BT coex scan status:%d\n", status);

	if (!ath6kl_btcoex_cfg80211_ready(vif))
		return -EIO;

	return ath6kl_wmi_set_btcoex_bt_op_status(ar->wmi, OP_TYPE_INQUIRY,
						   status);
}

int ath6kl_notify_btcoex_sco_status(struct wiphy *wiphy,  bool status,
				    bool esco, u32 tx_interval,
				    u32 tx_pkt_len)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct ath6kl_vif *vif;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	if (!ath6kl_btcoex_cfg80211_ready(vif))
		return -EIO;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
		   "BT Coex sco status:%d sco type %s tx interval %d tx pk len %d\n",
		   status, esco ? "ESCO" : "SCO", tx_interval, tx_pkt_len);

	if (status)
		ath6kl_wmi_set_btcoex_sco_op(ar->wmi, esco, tx_interval,
					     tx_pkt_len);

	return ath6kl_wmi_set_btcoex_bt_op_status(ar->wmi, OP_TYPE_SCO,
						  status);
}

int ath6kl_notify_btcoex_a2dp_status(struct wiphy *wiphy, bool status)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct ath6kl_vif *vif;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	if (!ath6kl_btcoex_cfg80211_ready(vif))
		return -EIO;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "BT coex A2DP status:%d\n", status);

	if (status)
		ath6kl_wmi_set_btcoex_a2dp_op(ar->wmi,
					      ar->btcoex_info.acl_role,
					      ar->btcoex_info.remote_lmp_ver,
					      ar->btcoex_info.bt_vendor);

	return ath6kl_wmi_set_btcoex_bt_op_status(ar->wmi, OP_TYPE_A2DP,
						  status);
}

#define MAX_LMP_VER	6
int ath6kl_notify_btcoex_acl_info(struct wiphy *wiphy,
				  enum nl80211_btcoex_acl_role role,
				  u32 lmp_ver)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);

	if (lmp_ver > MAX_LMP_VER)
		return -EINVAL;

	if (role > NL80211_BTCOEX_ACL_ROLE_SLAVE)
		return -EINVAL;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "BT coex ACL role:%d Lmp ver %d\n",
		   role, lmp_ver);

	ar->btcoex_info.acl_role = role;
	ar->btcoex_info.remote_lmp_ver = lmp_ver;

	return 0;
}

#define BTCOEX_COLOCATED_BT_DEFAULT	1
#define BTCOEX_COLOCATED_BT_QCOM	2

inline int get_bt_vendor_id(enum nl80211_btcoex_vendor_list vendor)
{
	switch (vendor) {
	case NL80211_BTCOEX_VENDOR_DEFAULT:
		return BTCOEX_COLOCATED_BT_DEFAULT;
	case NL80211_BTCOEX_VENDOR_QCOM:
		return BTCOEX_COLOCATED_BT_QCOM;
	}

	return BTCOEX_COLOCATED_BT_DEFAULT;
}

int ath6kl_notify_btcoex_antenna_config(struct wiphy *wiphy,
				enum nl80211_btcoex_antenna_config config)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);

	if (config > NL80211_BTCOEX_ANTENNA_SA)
		return -EINVAL;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "BT coex Antenna configuration:%d\n",
		   config);
	ath6kl_wmi_set_btcoex_set_colocated_bt(ar->wmi,
					       get_bt_vendor_id
					       (ar->btcoex_info.bt_vendor));

	return ath6kl_wmi_set_btcoex_set_fe_antenna(ar->wmi, config);
}
int ath6kl_notify_btcoex_bt_vendor(struct wiphy *wiphy,
				   enum nl80211_btcoex_vendor_list vendor)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);

	if (vendor > NL80211_BTCOEX_VENDOR_QCOM)
		return -EINVAL;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "vendor :%d\n", vendor);

	ar->btcoex_info.bt_vendor = vendor;

	return 0;
}

int ath6kl_notify_btcoex(struct wiphy *wiphy, u8 *buf,
					int len)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct ath6kl_vif *vif;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "BT coex wmi command:%p\n", buf);

	if (!ath6kl_btcoex_cfg80211_ready(vif))
		return -EIO;

	return ath6kl_wmi_send_btcoex_cmd(ar->wmi, buf,
			len);
}
