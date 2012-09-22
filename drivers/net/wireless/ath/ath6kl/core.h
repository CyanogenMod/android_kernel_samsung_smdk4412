/*
 * Copyright (c) 2010-2011 Atheros Communications Inc.
 * Copyright (c) 2011-2012 Qualcomm Atheros, Inc.
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

#ifndef CORE_H
#define CORE_H

#include <linux/etherdevice.h>
#include <linux/rtnetlink.h>
#include <linux/firmware.h>
#include <linux/sched.h>
#include <linux/circ_buf.h>
#include <net/cfg80211.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif
#include "htc.h"
#include "wmi.h"
#include "bmi.h"
#include "target.h"
#include "wmi_btcoex.h"

#define MAX_ATH6KL                        1
#define ATH6KL_MAX_RX_BUFFERS             16
#define ATH6KL_BUFFER_SIZE                1664
#define ATH6KL_MAX_AMSDU_RX_BUFFERS       4
#define ATH6KL_AMSDU_REFILL_THRESHOLD     3
#define ATH6KL_AMSDU_BUFFER_SIZE     (WMI_MAX_AMSDU_RX_DATA_FRAME_LENGTH + 128)
#define MAX_MSDU_SUBFRAME_PAYLOAD_LEN	1508
#define MIN_MSDU_SUBFRAME_PAYLOAD_LEN	46

#define USER_SAVEDKEYS_STAT_INIT     0
#define USER_SAVEDKEYS_STAT_RUN      1

#define ATH6KL_TX_TIMEOUT      10
#define ATH6KL_MAX_ENDPOINTS   4
#define MAX_NODE_NUM           15

#define ATH6KL_APSD_ALL_FRAME		0xFFFF
#define ATH6KL_APSD_NUM_OF_AC		0x4
#define ATH6KL_APSD_FRAME_MASK		0xF

/* Extra bytes for htc header alignment */
#define ATH6KL_HTC_ALIGN_BYTES 3

/* MAX_HI_COOKIE_NUM are reserved for high priority traffic */
#define MAX_DEF_COOKIE_NUM                180
#define MAX_HI_COOKIE_NUM                 18	/* 10% of MAX_COOKIE_NUM */
#define MAX_COOKIE_NUM                 (MAX_DEF_COOKIE_NUM + MAX_HI_COOKIE_NUM)

#define MAX_DEFAULT_SEND_QUEUE_DEPTH      (MAX_DEF_COOKIE_NUM / WMM_NUM_AC)

#define DISCON_TIMER_INTVAL               10000  /* in msec */

/* Channel dwell time in fg scan */
#define ATH6KL_FG_SCAN_INTERVAL				60 /* in ms */

/*
 * background scan interval (sec)
 * disable background scan interval: 65535
 * default: 60 sec
 * to restore the default value: 0
 *
 * For P2P and PMKID cache, bg scan should be enabled.
 */
#ifdef CONFIG_MACH_PX
#define WLAN_CONFIG_BG_SCAN_INTERVAL         65535
#else
#define WLAN_CONFIG_BG_SCAN_INTERVAL         0
#endif
/*
 * maximum active dwell time (ms)
 * default: 20 ms
 * to restore the default value: 0
 */
#ifdef CONFIG_MACH_PX
#define WLAN_CONFIG_MAXACT_CHDWELL_TIME      60
#else
#define WLAN_CONFIG_MAXACT_CHDWELL_TIME      20
#endif
/*
 * passive dwell time (ms)
 * default: 50 ms
 * to restore the default value: 0
 */
#ifdef CONFIG_MACH_PX
#define WLAN_CONFIG_PASSIVE_CHDWELL_TIME     120
#else
#define WLAN_CONFIG_PASSIVE_CHDWELL_TIME     50
#endif
#define WMI_SHORTSCANRATIO_DEFAULT           3
#define DEFAULT_SCAN_CTRL_FLAGS              (CONNECT_SCAN_CTRL_FLAGS | \
			SCAN_CONNECTED_CTRL_FLAGS | ACTIVE_SCAN_CTRL_FLAGS | \
			ROAM_SCAN_CTRL_FLAGS | ENABLE_AUTO_CTRL_FLAGS)

/*
 * 0: to disable sending ps-poll in TIM interrupt
 * 1: to send one ps-poll (the default)
 */
#ifdef CONFIG_MACH_PX
#define WLAN_CONFIG_PSPOLL_NUM              0
#else
#define WLAN_CONFIG_PSPOLL_NUM              1
#endif

#define WLAN_CONFIG_MCAST_RATE              60

/* includes also the null byte */
#define ATH6KL_FIRMWARE_MAGIC               "QCA-ATH6KL"

enum ath6kl_fw_ie_type {
	ATH6KL_FW_IE_FW_VERSION = 0,
	ATH6KL_FW_IE_TIMESTAMP = 1,
	ATH6KL_FW_IE_OTP_IMAGE = 2,
	ATH6KL_FW_IE_FW_IMAGE = 3,
	ATH6KL_FW_IE_PATCH_IMAGE = 4,
	ATH6KL_FW_IE_RESERVED_RAM_SIZE = 5,
	ATH6KL_FW_IE_CAPABILITIES = 6,
	ATH6KL_FW_IE_PATCH_ADDR = 7,
	ATH6KL_FW_IE_BOARD_ADDR = 8,
	ATH6KL_FW_IE_VIF_MAX = 9,
};

enum ath6kl_fw_capability {
	ATH6KL_FW_CAPABILITY_HOST_P2P = 0,
	ATH6KL_FW_CAPABILITY_SCHED_SCAN = 1,

	/*
	 * Firmware is capable of supporting P2P mgmt operations on a
	 * station interface. After group formation, the station
	 * interface will become a P2P client/GO interface as the case may be
	 */
	ATH6KL_FW_CAPABILITY_STA_P2PDEV_DUPLEX,

	ATH6KL_FW_CAPABILITY_INACTIVITY_TIMEOUT,

	ATH6KL_FW_CAPABILITY_RSN_CAP_OVERRIDE,

	/* this needs to be last */
	ATH6KL_FW_CAPABILITY_MAX,
};

#define ATH6KL_CAPABILITY_LEN (ALIGN(ATH6KL_FW_CAPABILITY_MAX, 32) / 32)

struct ath6kl_fw_ie {
	__le32 id;
	__le32 len;
	u8 data[0];
};

#define ATH6KL_FW_API2_FILE "fw-2.bin"
#define ATH6KL_FW_API3_FILE "fw-3.bin"

/* AR6003 1.0 definitions */
#define AR6003_HW_1_0_VERSION                 0x300002ba

/* AR6003 2.0 definitions */
#define AR6003_HW_2_0_VERSION                 0x30000384
#define AR6003_HW_2_0_PATCH_DOWNLOAD_ADDRESS  0x57e910
#define AR6003_HW_2_0_FW_DIR			"ath6k/AR6003/hw2.0"
#define AR6003_HW_2_0_OTP_FILE			"otp.bin.z77"
#define AR6003_HW_2_0_FIRMWARE_FILE		"athwlan.bin.z77"
#define AR6003_HW_2_0_TCMD_FIRMWARE_FILE	"athtcmd_ram.bin"
#define AR6003_HW_2_0_PATCH_FILE		"data.patch.bin"
#define AR6003_HW_2_0_BOARD_DATA_FILE "ath6k/AR6003/hw2.0/bdata.bin"
#define AR6003_HW_2_0_DEFAULT_BOARD_DATA_FILE \
			"ath6k/AR6003/hw2.0/bdata.SD31.bin"

/* AR6003 3.0 definitions */
#define AR6003_HW_2_1_1_VERSION                 0x30000582
#define AR6003_HW_2_1_1_FW_DIR			"ath6k/AR6003/hw2.1.1"
#define AR6003_HW_2_1_1_OTP_FILE		"otp.bin"
#define AR6003_HW_2_1_1_FIRMWARE_FILE		"athwlan.bin"
#define AR6003_HW_2_1_1_TCMD_FIRMWARE_FILE	"athtcmd_ram.bin"
#define AR6003_HW_2_1_1_UTF_FIRMWARE_FILE	"utf.bin"
#define AR6003_HW_2_1_1_TESTSCRIPT_FILE	"nullTestFlow.bin"
#define AR6003_HW_2_1_1_PATCH_FILE		"data.patch.bin"
#define AR6003_HW_2_1_1_BOARD_DATA_FILE "ath6k/AR6003/hw2.1.1/bdata.bin"
#define AR6003_HW_2_1_1_DEFAULT_BOARD_DATA_FILE	\
			"ath6k/AR6003/hw2.1.1/bdata.SD31.bin"

#ifdef CONFIG_MACH_PX
#define AR6003_HW_2_1_1_TCMD_BOARD_DATA_FILE \
			"ath6k/AR6003/hw2.1.1/bdata.tcmd.bin"
#endif

/* AR6004 1.0 definitions */
#define AR6004_HW_1_0_VERSION                 0x30000623
#define AR6004_HW_1_0_FW_DIR			"ath6k/AR6004/hw1.0"
#define AR6004_HW_1_0_FIRMWARE_FILE		"fw.ram.bin"
#define AR6004_HW_1_0_BOARD_DATA_FILE         "ath6k/AR6004/hw1.0/bdata.bin"
#define AR6004_HW_1_0_DEFAULT_BOARD_DATA_FILE \
	"ath6k/AR6004/hw1.0/bdata.DB132.bin"

/* AR6004 1.1 definitions */
#define AR6004_HW_1_1_VERSION                 0x30000001
#define AR6004_HW_1_1_FW_DIR			"ath6k/AR6004/hw1.1"
#define AR6004_HW_1_1_FIRMWARE_FILE		"fw.ram.bin"
#define AR6004_HW_1_1_BOARD_DATA_FILE         "ath6k/AR6004/hw1.1/bdata.bin"
#define AR6004_HW_1_1_DEFAULT_BOARD_DATA_FILE \
	"ath6k/AR6004/hw1.1/bdata.DB132.bin"

/* Per STA data, used in AP mode */
#define STA_PS_AWAKE		BIT(0)
#define	STA_PS_SLEEP		BIT(1)
#define	STA_PS_POLLED		BIT(2)
#define STA_PS_APSD_TRIGGER     BIT(3)
#define STA_PS_APSD_EOSP        BIT(4)

/* HTC TX packet tagging definitions */
#define ATH6KL_CONTROL_PKT_TAG    HTC_TX_PACKET_TAG_USER_DEFINED
#define ATH6KL_DATA_PKT_TAG       (ATH6KL_CONTROL_PKT_TAG + 1)

#define AR6003_CUST_DATA_SIZE 16

#define AGGR_WIN_IDX(x, y)          ((x) % (y))
#define AGGR_INCR_IDX(x, y)         AGGR_WIN_IDX(((x) + 1), (y))
#define AGGR_DCRM_IDX(x, y)         AGGR_WIN_IDX(((x) - 1), (y))
#define ATH6KL_MAX_SEQ_NO		0xFFF
#define ATH6KL_NEXT_SEQ_NO(x)		(((x) + 1) & ATH6KL_MAX_SEQ_NO)

#define NUM_OF_TIDS         8
#define AGGR_SZ_DEFAULT     8

#define AGGR_WIN_SZ_MIN     2
#define AGGR_WIN_SZ_MAX     8

#define TID_WINDOW_SZ(_x)   ((_x) << 1)

#define AGGR_NUM_OF_FREE_NETBUFS    16

#ifdef CONFIG_MACH_PX
#define AGGR_RX_TIMEOUT     100	/* in ms */
#else
#define AGGR_RX_TIMEOUT     400	/* in ms */
#endif

#define WMI_TIMEOUT (2 * HZ)

#define MBOX_YIELD_LIMIT 99

#define ATH6KL_DEFAULT_LISTEN_INTVAL	100 /* in TUs */
#define ATH6KL_DEFAULT_BMISS_TIME	1500
#define ATH6KL_MAX_WOW_LISTEN_INTL	300 /* in TUs */
#define ATH6KL_MAX_BMISS_TIME		5000

/* configuration lags */
/*
 * ATH6KL_CONF_IGNORE_ERP_BARKER: Ignore the barker premable in
 * ERP IE of beacon to determine the short premable support when
 * sending (Re)Assoc req.
 * ATH6KL_CONF_IGNORE_PS_FAIL_EVT_IN_SCAN: Don't send the power
 * module state transition failure events which happen during
 * scan, to the host.
 */
#define ATH6KL_CONF_IGNORE_ERP_BARKER		BIT(0)
#define ATH6KL_CONF_IGNORE_PS_FAIL_EVT_IN_SCAN  BIT(1)
#define ATH6KL_CONF_ENABLE_11N			BIT(2)
#define ATH6KL_CONF_ENABLE_TX_BURST		BIT(3)
#define ATH6KL_CONF_UART_DEBUG			BIT(4)

#define P2P_WILDCARD_SSID_LEN			7 /* DIRECT- */

enum wlan_low_pwr_state {
	WLAN_POWER_STATE_ON,
	WLAN_POWER_STATE_CUT_PWR,
	WLAN_POWER_STATE_DEEP_SLEEP,
	WLAN_POWER_STATE_WOW
};

enum sme_state {
	SME_DISCONNECTED,
	SME_CONNECTING,
	SME_CONNECTED
};

struct skb_hold_q {
	struct sk_buff *skb;
	bool is_amsdu;
	u16 seq_no;
};

struct rxtid {
	bool aggr;
	bool progress;
	bool timer_mon;
	u16 win_sz;
	u16 seq_next;
	u32 hold_q_sz;
	struct skb_hold_q *hold_q;
	struct sk_buff_head q;
	spinlock_t lock;
};

struct rxtid_stats {
	u32 num_into_aggr;
	u32 num_dups;
	u32 num_oow;
	u32 num_mpdu;
	u32 num_amsdu;
	u32 num_delivered;
	u32 num_timeouts;
	u32 num_hole;
	u32 num_bar;
};

struct aggr_info_conn {
	u8 aggr_sz;
	u8 timer_scheduled;
	struct timer_list timer;
	struct net_device *dev;
	struct rxtid rx_tid[NUM_OF_TIDS];
	struct rxtid_stats stat[NUM_OF_TIDS];
	struct aggr_info *aggr_info;
};

struct aggr_info {
	struct aggr_info_conn *aggr_conn;
	struct sk_buff_head rx_amsdu_freeq;
};

struct ath6kl_wep_key {
	u8 key_index;
	u8 key_len;
	u8 key[64];
};

#define ATH6KL_KEY_SEQ_LEN 8

struct ath6kl_key {
	u8 key[WLAN_MAX_KEY_LEN];
	u8 key_len;
	u8 seq[ATH6KL_KEY_SEQ_LEN];
	u8 seq_len;
	u32 cipher;
};

struct ath6kl_node_mapping {
	u8 mac_addr[ETH_ALEN];
	u8 ep_id;
	u8 tx_pend;
};

struct ath6kl_cookie {
	struct sk_buff *skb;
	u32 map_no;
	struct htc_packet htc_pkt;
	struct ath6kl_cookie *arc_list_next;
};

struct ath6kl_mgmt_buff {
	struct list_head list;
	u32 freq;
	u32 wait;
	u32 id;
	bool no_cck;
	size_t len;
	u8 buf[0];
};

struct ath6kl_sta {
	u16 sta_flags;
	u8 mac[ETH_ALEN];
	u8 aid;
	u8 keymgmt;
	u8 ucipher;
	u8 auth;
	u8 wpa_ie[ATH6KL_MAX_IE];
	struct sk_buff_head psq;
	spinlock_t psq_lock;
	struct list_head mgmt_psq;
	size_t mgmt_psq_len;
	u8 apsd_info;
	struct sk_buff_head apsdq;
	struct aggr_info_conn *aggr_conn;
};

struct ath6kl_version {
	u32 target_ver;
	u32 wlan_ver;
	u32 abi_ver;
};

struct ath6kl_bmi {
	u32 cmd_credits;
	bool done_sent;
	u8 *cmd_buf;
	u32 max_data_size;
	u32 max_cmd_size;
};

struct target_stats {
	u64 tx_pkt;
	u64 tx_byte;
	u64 tx_ucast_pkt;
	u64 tx_ucast_byte;
	u64 tx_mcast_pkt;
	u64 tx_mcast_byte;
	u64 tx_bcast_pkt;
	u64 tx_bcast_byte;
	u64 tx_rts_success_cnt;
	u64 tx_pkt_per_ac[4];

	u64 tx_err;
	u64 tx_fail_cnt;
	u64 tx_retry_cnt;
	u64 tx_mult_retry_cnt;
	u64 tx_rts_fail_cnt;

	u64 rx_pkt;
	u64 rx_byte;
	u64 rx_ucast_pkt;
	u64 rx_ucast_byte;
	u64 rx_mcast_pkt;
	u64 rx_mcast_byte;
	u64 rx_bcast_pkt;
	u64 rx_bcast_byte;
	u64 rx_frgment_pkt;

	u64 rx_err;
	u64 rx_crc_err;
	u64 rx_key_cache_miss;
	u64 rx_decrypt_err;
	u64 rx_dupl_frame;

	u64 tkip_local_mic_fail;
	u64 tkip_cnter_measures_invoked;
	u64 tkip_replays;
	u64 tkip_fmt_err;
	u64 ccmp_fmt_err;
	u64 ccmp_replays;

	u64 pwr_save_fail_cnt;

	u64 cs_bmiss_cnt;
	u64 cs_low_rssi_cnt;
	u64 cs_connect_cnt;
	u64 cs_discon_cnt;

	s32 tx_ucast_rate;
	s32 rx_ucast_rate;

	u32 lq_val;

	u32 wow_pkt_dropped;
	u16 wow_evt_discarded;

	s16 noise_floor_calib;
	s16 cs_rssi;
	s16 cs_ave_beacon_rssi;
	u8 cs_ave_beacon_snr;
	u8 cs_last_roam_msec;
	u8 cs_snr;

	u8 wow_host_pkt_wakeups;
	u8 wow_host_evt_wakeups;

	u32 arp_received;
	u32 arp_matched;
	u32 arp_replied;
};

struct ath6kl_mbox_info {
	u32 htc_addr;
	u32 htc_ext_addr;
	u32 htc_ext_sz;

	u32 block_size;

	u32 gmbox_addr;

	u32 gmbox_sz;
};

/*
 * 802.11i defines an extended IV for use with non-WEP ciphers.
 * When the EXTIV bit is set in the key id byte an additional
 * 4 bytes immediately follow the IV for TKIP.  For CCMP the
 * EXTIV bit is likewise set but the 8 bytes represent the
 * CCMP header rather than IV+extended-IV.
 */

#define ATH6KL_KEYBUF_SIZE 16
#define ATH6KL_MICBUF_SIZE (8+8)	/* space for both tx and rx */

#define ATH6KL_KEY_XMIT  0x01
#define ATH6KL_KEY_RECV  0x02
#define ATH6KL_KEY_DEFAULT   0x80	/* default xmit key */

/* Initial group key for AP mode */
struct ath6kl_req_key {
	bool valid;
	u8 key_index;
	int key_type;
	u8 key[WLAN_MAX_KEY_LEN];
	u8 key_len;
};

/*
 * Bluetooth WiFi co-ex information.
 * This structure keeps track of the Bluetooth related status.
 * This involves the Bluetooth ACL link role, Bluetooth remote lmp version.
 */
struct ath6kl_btcoex {
	u32 acl_role; /* Master/slave role of Bluetooth ACL link */
	u32 remote_lmp_ver; /* LMP version of the remote device. */
	u32 bt_vendor; /* Keeps track of the Bluetooth chip vendor */
};
enum ath6kl_hif_type {
	ATH6KL_HIF_TYPE_SDIO,
	ATH6KL_HIF_TYPE_USB,
};

/* Max number of filters that hw supports */
#define ATH6K_MAX_MC_FILTERS_PER_LIST 7
struct ath6kl_mc_filter {
	struct list_head list;
	char hw_addr[ATH6KL_MCAST_FILTER_MAC_ADDR_SIZE];
};

/*
 * Driver's maximum limit, note that some firmwares support only one vif
 * and the runtime (current) limit must be checked from ar->vif_max.
 */
#define ATH6KL_VIF_MAX	3

/* vif flags info */
enum ath6kl_vif_state {
	CONNECTED,
	CONNECT_PEND,
	WMM_ENABLED,
	NETQ_STOPPED,
	DTIM_EXPIRED,
	NETDEV_REGISTERED,
	CLEAR_BSSFILTER_ON_BEACON,
	DTIM_PERIOD_AVAIL,
	WLAN_ENABLED,
	STATS_UPDATE_PEND,
	HOST_SLEEP_MODE_CMD_PROCESSED,
	SLEEP_POLICY_ENABLED,
};

struct ath6kl_vif {
	struct list_head list;
	struct wireless_dev wdev;
	struct net_device *ndev;
	struct ath6kl *ar;
	/* Lock to protect vif specific net_stats and flags */
	spinlock_t if_lock;
	u8 fw_vif_idx;
	unsigned long flags;
	int ssid_len;
	u8 ssid[IEEE80211_MAX_SSID_LEN];
	u8 dot11_auth_mode;
	u8 auth_mode;
	u8 prwise_crypto;
	u8 prwise_crypto_len;
	u8 grp_crypto;
	u8 grp_crypto_len;
	u8 def_txkey_index;
	u8 next_mode;
	u8 nw_type;
	u8 bssid[ETH_ALEN];
	u8 req_bssid[ETH_ALEN];
	u16 ch_hint;
	u16 bss_ch;
	struct ath6kl_wep_key wep_key_list[WMI_MAX_KEY_INDEX + 1];
	struct ath6kl_key keys[WMI_MAX_KEY_INDEX + 1];
	struct aggr_info *aggr_cntxt;

	struct timer_list disconnect_timer;
	struct timer_list sched_scan_timer;

	struct cfg80211_scan_request *scan_req;
	enum sme_state sme_state;
	int reconnect_flag;
	u32 last_roc_id;
	u32 last_cancel_roc_id;
	u32 send_action_id;
	bool probe_req_report;
	u16 next_chan;
	u16 assoc_bss_beacon_int;
	u16 bg_scan_period;
	u8 scan_ctrl_flag;
	u16 listen_intvl_t;
	u16 bmiss_time_t;
	u8 assoc_bss_dtim_period;
	struct net_device_stats net_stats;
	struct target_stats target_stats;

	struct list_head mc_filter;

	struct wmi_scan_params_cmd scparams;
	unsigned int pspoll_num;
	u16 mcastrate;
	bool force_reload;
	bool sdio_remove;
};

#define WOW_LIST_ID		0
#define WOW_HOST_REQ_DELAY	500 /* ms */

#define ATH6KL_SCHED_SCAN_RESULT_DELAY 5000 /* ms */

/* Flag info */
enum ath6kl_dev_state {
	WMI_ENABLED,
	WMI_READY,
	WMI_CTRL_EP_FULL,
	TESTMODE,
	DESTROY_IN_PROGRESS,
	SKIP_SCAN,
	ROAM_TBL_PEND,
	FIRST_BOOT,
	WOW_RESUME_PRINT,
};

enum ath6kl_state {
	ATH6KL_STATE_OFF,
	ATH6KL_STATE_ON,
	ATH6KL_STATE_SUSPENDING,
	ATH6KL_STATE_RESUMING,
	ATH6KL_STATE_DEEPSLEEP,
	ATH6KL_STATE_CUTPOWER,
	ATH6KL_STATE_WOW,
	ATH6KL_STATE_SCHED_SCAN,
};

struct ath6kl {
	struct device *dev;
	struct wiphy *wiphy;

	enum ath6kl_state state;

	struct ath6kl_bmi bmi;
	const struct ath6kl_hif_ops *hif_ops;
	struct wmi *wmi;
	int tx_pending[ENDPOINT_MAX];
	int total_tx_data_pend;
	struct htc_target *htc_target;
	enum ath6kl_hif_type hif_type;
	void *hif_priv;
	struct list_head vif_list;
	/* Lock to avoid race in vif_list entries among add/del/traverse */
	spinlock_t list_lock;
	u8 num_vif;
	unsigned int vif_max;
	u8 max_norm_iface;
	u8 avail_idx_map;
	spinlock_t lock;
	struct semaphore sem;
	u8 lrssi_roam_threshold;
	struct ath6kl_version version;
	u32 target_type;
	u8 tx_pwr;
	struct ath6kl_node_mapping node_map[MAX_NODE_NUM];
	u8 ibss_ps_enable;
	bool ibss_if_active;
	u8 node_num;
	u8 next_ep_id;
	struct ath6kl_cookie *cookie_list;
	u32 cookie_count;
	enum htc_endpoint_id ac2ep_map[WMM_NUM_AC];
	bool ac_stream_active[WMM_NUM_AC];
	u8 ac_stream_pri_map[WMM_NUM_AC];
	u8 hiac_stream_active_pri;
	u8 ep2ac_map[ENDPOINT_MAX];
	enum htc_endpoint_id ctrl_ep;
	struct ath6kl_htc_credit_info credit_state_info;
	u32 connect_ctrl_flags;
	u32 user_key_ctrl;
	u8 usr_bss_filter;
	struct ath6kl_sta sta_list[AP_MAX_NUM_STA];
	u8 sta_list_index;
	struct ath6kl_req_key ap_mode_bkey;
	struct sk_buff_head mcastpsq;
	spinlock_t mcastpsq_lock;
	u8 intra_bss;
	struct wmi_ap_mode_stat ap_stats;
	u8 ap_country_code[3];
	struct list_head amsdu_rx_buffer_queue;
	u8 rx_meta_ver;
	enum wlan_low_pwr_state wlan_pwr_state;
	u8 mac_addr[ETH_ALEN];
#define AR_MCAST_FILTER_MAC_ADDR_SIZE  4
	struct {
		void *rx_report;
		size_t rx_report_len;
	} tm;

	struct ath6kl_hw {
		u32 id;
		const char *name;
		u32 dataset_patch_addr;
		u32 app_load_addr;
		u32 app_start_override_addr;
		u32 board_ext_data_addr;
		u32 reserved_ram_size;
		u32 board_addr;
		u32 refclk_hz;
		u32 uarttx_pin;
		u32 testscript_addr;

		struct ath6kl_hw_fw {
			const char *dir;
			const char *otp;
			const char *fw;
			const char *tcmd;
			const char *patch;
			const char *utf;
			const char *testscript;
		} fw;

		const char *fw_board;
		const char *fw_default_board;
	} hw;

	u16 conf_flags;
	u16 suspend_mode;
	u16 wow_suspend_mode;
	wait_queue_head_t event_wq;
	struct ath6kl_mbox_info mbox_info;

	struct ath6kl_cookie cookie_mem[MAX_COOKIE_NUM];
	unsigned long flag;

	u8 *fw_board;
	size_t fw_board_len;

	u8 *fw_otp;
	size_t fw_otp_len;

	u8 *fw;
	size_t fw_len;

	u8 *fw_patch;
	size_t fw_patch_len;

	u8 *fw_testscript;
	size_t fw_testscript_len;

	unsigned int fw_api;
	unsigned long fw_capabilities[ATH6KL_CAPABILITY_LEN];

	struct workqueue_struct *ath6kl_wq;

	struct dentry *debugfs_phy;

	bool p2p;

	struct ath6kl_btcoex btcoex_info;

	unsigned int psminfo;

#ifdef CONFIG_ATH6KL_DEBUG
	struct {
		struct sk_buff_head fwlog_queue;
		struct completion fwlog_completion;
		bool fwlog_open;

		u32 fwlog_mask;

		unsigned int dbgfs_diag_reg;
		u32 diag_reg_addr_wr;
		u32 diag_reg_val_wr;

		struct {
			unsigned int invalid_rate;
		} war_stats;

		u8 *roam_tbl;
		unsigned int roam_tbl_len;

		u8 keepalive;
		u8 disc_timeout;
	} debug;
#endif /* CONFIG_ATH6KL_DEBUG */

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
	bool screen_off;
#endif /* CONFIG_HAS_EARLYSUSPEND */

#ifdef CONFIG_HAS_WAKELOCK
	struct wake_lock wake_lock;
	struct wake_lock p2p_wake_lock;
#endif /* CONFIG_HAS_WAKELOCK */

};

#ifdef CONFIG_MACH_PX
/*
   For this 3.2.0.4402 image:
   00561734 g	  O .bss   00000004 epc1
   00561738 g	   O .bss  00000004 epc2
   0056173c g	   O .bss  00000004 epc3
   00561740 g	   O .bss  00000004 epc4

   For this 3.2.0.64 image:
   00561fa4 g	  O .bss   00000004 epc1
*/

#define EPC1_ADDR       0x00

void ath6kl_print_ar6k_registers(struct ath6kl *ar);
#endif

static inline struct ath6kl *ath6kl_priv(struct net_device *dev)
{
	return ((struct ath6kl_vif *) netdev_priv(dev))->ar;
}

static inline u32 ath6kl_get_hi_item_addr(struct ath6kl *ar,
					  u32 item_offset)
{
	u32 addr = 0;

	if (ar->target_type == TARGET_TYPE_AR6003)
		addr = ATH6KL_AR6003_HI_START_ADDR + item_offset;
	else if (ar->target_type == TARGET_TYPE_AR6004)
		addr = ATH6KL_AR6004_HI_START_ADDR + item_offset;

	return addr;
}

int ath6kl_configure_target(struct ath6kl *ar);
void ath6kl_detect_error(unsigned long ptr);
void disconnect_timer_handler(unsigned long ptr);
void init_netdev(struct net_device *dev);
void ath6kl_cookie_init(struct ath6kl *ar);
void ath6kl_cookie_cleanup(struct ath6kl *ar);
void ath6kl_rx(struct htc_target *target, struct htc_packet *packet);
void ath6kl_tx_complete(void *context, struct list_head *packet_queue);
enum htc_send_full_action ath6kl_tx_queue_full(struct htc_target *target,
					       struct htc_packet *packet);
void ath6kl_stop_txrx(struct ath6kl *ar);
void ath6kl_cleanup_amsdu_rxbufs(struct ath6kl *ar);
int ath6kl_diag_write32(struct ath6kl *ar, u32 address, __le32 value);
int ath6kl_diag_write(struct ath6kl *ar, u32 address, void *data, u32 length);
int ath6kl_diag_read32(struct ath6kl *ar, u32 address, u32 *value);
int ath6kl_diag_read(struct ath6kl *ar, u32 address, void *data, u32 length);
int ath6kl_read_fwlogs(struct ath6kl *ar);
void ath6kl_init_profile_info(struct ath6kl_vif *vif);
void ath6kl_tx_data_cleanup(struct ath6kl *ar);

struct ath6kl_cookie *ath6kl_alloc_cookie(struct ath6kl *ar);
void ath6kl_free_cookie(struct ath6kl *ar, struct ath6kl_cookie *cookie);
int ath6kl_data_tx(struct sk_buff *skb, struct net_device *dev);

struct aggr_info *aggr_init(struct ath6kl_vif *vif);
void aggr_conn_init(struct ath6kl_vif *vif, struct aggr_info *aggr_info,
		    struct aggr_info_conn *aggr_conn);
void ath6kl_rx_refill(struct htc_target *target,
		      enum htc_endpoint_id endpoint);
void ath6kl_refill_amsdu_rxbufs(struct ath6kl *ar, int count);
struct htc_packet *ath6kl_alloc_amsdu_rxbuf(struct htc_target *target,
					    enum htc_endpoint_id endpoint,
					    int len);
void aggr_module_destroy(struct aggr_info *aggr_info);
void aggr_reset_state(struct aggr_info_conn *aggr_conn);

struct ath6kl_sta *ath6kl_find_sta(struct ath6kl_vif *vif, u8 * node_addr);
struct ath6kl_sta *ath6kl_find_sta_by_aid(struct ath6kl *ar, u8 aid);

void ath6kl_ready_event(void *devt, u8 * datap, u32 sw_ver, u32 abi_ver);
int ath6kl_control_tx(void *devt, struct sk_buff *skb,
		      enum htc_endpoint_id eid);
void ath6kl_connect_event(struct ath6kl_vif *vif, u16 channel,
			  u8 *bssid, u16 listen_int,
			  u16 beacon_int, enum network_type net_type,
			  u8 beacon_ie_len, u8 assoc_req_len,
			  u8 assoc_resp_len, u8 *assoc_info);
void ath6kl_connect_ap_mode_bss(struct ath6kl_vif *vif, u16 channel);
void ath6kl_connect_ap_mode_sta(struct ath6kl_vif *vif, u16 aid, u8 *mac_addr,
				u8 keymgmt, u8 ucipher, u8 auth,
				u8 assoc_req_len, u8 *assoc_info, u8 apsd_info);
void ath6kl_disconnect_event(struct ath6kl_vif *vif, u8 reason,
			     u8 *bssid, u8 assoc_resp_len,
			     u8 *assoc_info, u16 prot_reason_status);
void ath6kl_tkip_micerr_event(struct ath6kl_vif *vif, u8 keyid, bool ismcast);
void ath6kl_txpwr_rx_evt(void *devt, u8 tx_pwr);
void ath6kl_scan_complete_evt(struct ath6kl_vif *vif, int status);
void ath6kl_tgt_stats_event(struct ath6kl_vif *vif, u8 *ptr, u32 len);
void ath6kl_indicate_tx_activity(void *devt, u8 traffic_class, bool active);
enum htc_endpoint_id ath6kl_ac2_endpoint_id(void *devt, u8 ac);

void ath6kl_pspoll_event(struct ath6kl_vif *vif, u8 aid);

void ath6kl_dtimexpiry_event(struct ath6kl_vif *vif);
void ath6kl_disconnect(struct ath6kl_vif *vif);
void aggr_recv_delba_req_evt(struct ath6kl_vif *vif, u8 tid);
void aggr_recv_addba_req_evt(struct ath6kl_vif *vif, u8 tid, u16 seq_no,
			     u8 win_sz);
void ath6kl_wakeup_event(void *dev);

void ath6kl_reset_device(struct ath6kl *ar, u32 target_type,
			 bool wait_fot_compltn, bool cold_reset);
void ath6kl_init_control_info(struct ath6kl_vif *vif);
void ath6kl_deinit_if_data(struct ath6kl_vif *vif);
void ath6kl_core_free(struct ath6kl *ar);
struct ath6kl_vif *ath6kl_vif_first(struct ath6kl *ar);
void ath6kl_cleanup_vif(struct ath6kl_vif *vif, bool wmi_ready);
int ath6kl_init_hw_start(struct ath6kl *ar);
int ath6kl_init_hw_stop(struct ath6kl *ar);
void ath6kl_check_wow_status(struct ath6kl *ar, struct sk_buff *skb,
			     bool is_event_pkt);
#ifdef CONFIG_MACH_PX
void ath6kl_sdio_init_c210(void);
void ath6kl_sdio_exit_c210(void);
#else
void ath6kl_sdio_init_msm(void);
void ath6kl_sdio_exit_msm(void);
#endif
void ath6kl_mangle_mac_address(struct ath6kl *ar);

#endif /* CORE_H */
