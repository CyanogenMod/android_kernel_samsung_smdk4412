
/*
 * Copyright (c) 2011 Atheros Communications Inc.
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

#include <linux/moduleparam.h>
#include <linux/errno.h>
//#include <linux/of.h>
#include <linux/mmc/sdio_func.h>
#include "core.h"
#include "cfg80211.h"
#include "target.h"
#include "debug.h"
#include "hif-ops.h"
#include "pm.h"
#include <linux/vmalloc.h>

unsigned int debug_mask = ATH6KL_DBG_WMI | ATH6KL_DBG_BOOT |
			ATH6KL_DBG_WLAN_CFG | ATH6KL_DBG_SUSPEND |
			ATH6KL_DBG_TRC;
static unsigned int testmode;
#ifdef CONFIG_MACH_PX
/* WoW2 (deepsleep), Suspend (WoW) */
static unsigned int suspend_mode = WLAN_POWER_STATE_WOW;
static unsigned int wow_mode = WLAN_POWER_STATE_DEEP_SLEEP;
#else
static unsigned int suspend_mode;
static unsigned int wow_mode;
#endif
static unsigned int uart_debug;
#ifdef CONFIG_MACH_PX
static unsigned int ar6k_clock = 26000000;
#else
static unsigned int ar6k_clock = 19200000;
#endif

#ifdef CONFIG_ATH6KL_LOCALE_USA
static unsigned short reg_domain = 0x8348;
#else
static unsigned short reg_domain = 0xffff;
#endif

static unsigned short lrssi = 10;

static unsigned short en_ani = 1;

module_param(debug_mask, uint, 0644);
module_param(testmode, uint, 0644);
module_param(suspend_mode, uint, 0644);
module_param(wow_mode, uint, 0644);
module_param(uart_debug, uint, 0644);
module_param(ar6k_clock, uint, 0644);
module_param(reg_domain, ushort, 0644);
module_param(lrssi, ushort, 0644);
module_param(en_ani, ushort, 0644);


static const struct ath6kl_hw hw_list[] = {
	{
		.id				= AR6003_HW_2_0_VERSION,
		.name				= "ar6003 hw 2.0",
		.dataset_patch_addr		= 0x57e884,
		.app_load_addr			= 0x543180,
		.board_ext_data_addr		= 0x57e500,
		.reserved_ram_size		= 6912,
		.refclk_hz			= 26000000,
		.uarttx_pin			= 8,

		/* hw2.0 needs override address hardcoded */
		.app_start_override_addr	= 0x944C00,

		.fw = {
			.dir		= AR6003_HW_2_0_FW_DIR,
			.otp		= AR6003_HW_2_0_OTP_FILE,
			.fw		= AR6003_HW_2_0_FIRMWARE_FILE,
			.tcmd		= AR6003_HW_2_0_TCMD_FIRMWARE_FILE,
			.patch		= AR6003_HW_2_0_PATCH_FILE,
		},

		.fw_board		= AR6003_HW_2_0_BOARD_DATA_FILE,
		.fw_default_board	= AR6003_HW_2_0_DEFAULT_BOARD_DATA_FILE,
	},
	{
		.id				= AR6003_HW_2_1_1_VERSION,
		.name				= "ar6003 hw 2.1.1",
		.dataset_patch_addr		= 0x57ff74,
		.app_load_addr			= 0x1234,
		.board_ext_data_addr		= 0x542330,
		.reserved_ram_size		= 512,
		.refclk_hz			= 26000000,
		.uarttx_pin			= 8,
		.testscript_addr		= 0x57ef74,

		.fw = {
			.dir		= AR6003_HW_2_1_1_FW_DIR,
			.otp		= AR6003_HW_2_1_1_OTP_FILE,
			.fw		= AR6003_HW_2_1_1_FIRMWARE_FILE,
			.tcmd		= AR6003_HW_2_1_1_TCMD_FIRMWARE_FILE,
			.patch		= AR6003_HW_2_1_1_PATCH_FILE,
			.utf		= AR6003_HW_2_1_1_UTF_FIRMWARE_FILE,
			.testscript	= AR6003_HW_2_1_1_TESTSCRIPT_FILE,
		},

		.fw_board		= AR6003_HW_2_1_1_BOARD_DATA_FILE,
		.fw_default_board	= AR6003_HW_2_1_1_DEFAULT_BOARD_DATA_FILE,
	},
	{
		.id				= AR6004_HW_1_0_VERSION,
		.name				= "ar6004 hw 1.0",
		.dataset_patch_addr		= 0x57e884,
		.app_load_addr			= 0x1234,
		.board_ext_data_addr		= 0x437000,
		.reserved_ram_size		= 19456,
		.board_addr			= 0x433900,
		.refclk_hz			= 26000000,
		.uarttx_pin			= 11,

		.fw = {
			.dir		= AR6004_HW_1_0_FW_DIR,
			.fw		= AR6004_HW_1_0_FIRMWARE_FILE,
		},

		.fw_board		= AR6004_HW_1_0_BOARD_DATA_FILE,
		.fw_default_board	= AR6004_HW_1_0_DEFAULT_BOARD_DATA_FILE,
	},
	{
		.id				= AR6004_HW_1_1_VERSION,
		.name				= "ar6004 hw 1.1",
		.dataset_patch_addr		= 0x57e884,
		.app_load_addr			= 0x1234,
		.board_ext_data_addr		= 0x437000,
		.reserved_ram_size		= 11264,
		.board_addr			= 0x43d400,
		.refclk_hz			= 40000000,
		.uarttx_pin			= 11,

		.fw = {
			.dir		= AR6004_HW_1_1_FW_DIR,
			.fw		= AR6004_HW_1_1_FIRMWARE_FILE,
		},

		.fw_board		= AR6004_HW_1_1_BOARD_DATA_FILE,
		.fw_default_board	= AR6004_HW_1_1_DEFAULT_BOARD_DATA_FILE,
	},
};

/*
 * Include definitions here that can be used to tune the WLAN module
 * behavior. Different customers can tune the behavior as per their needs,
 * here.
 */

/*
 * This configuration item enable/disable keepalive support.
 * Keepalive support: In the absence of any data traffic to AP, null
 * frames will be sent to the AP at periodic interval, to keep the association
 * active. This configuration item defines the periodic interval.
 * Use value of zero to disable keepalive support
 * Default: 60 seconds
 */
#define WLAN_CONFIG_KEEP_ALIVE_INTERVAL 60

/*
 * This configuration item sets the value of disconnect timeout
 * Firmware delays sending the disconnec event to the host for this
 * timeout after is gets disconnected from the current AP.
 * If the firmware successly roams within the disconnect timeout
 * it sends a new connect event
 */
#define WLAN_CONFIG_DISCONNECT_TIMEOUT 10


#define ATH6KL_DATA_OFFSET    64

typedef char            A_CHAR;
extern int android_readwrite_file(const A_CHAR *filename, A_CHAR *rbuf, const A_CHAR *wbuf, size_t length);

 /*
 * Number of bytes in board data that we are interested
 * in while setting regulatory domain from host
 */
#define REG_DMN_BOARD_DATA_LEN	16

/* Modifies regulatory domain in board data in target RAM */
static int ath6kl_set_reg_dmn(struct ath6kl *ar)
{
	u8 buf[REG_DMN_BOARD_DATA_LEN];
	__le16 old_sum, old_ver, old_rd, old_rd_next;
	__le32 brd_dat_addr = 0, new_sum, new_rd;
	int ret;

	ret = ath6kl_bmi_read(ar, AR6003_BOARD_DATA_ADDR,
			      (u8 *)&brd_dat_addr, 4);
	if (ret)
		return ret;

	memset(buf, 0, sizeof(buf));
	ret = ath6kl_bmi_read(ar, brd_dat_addr, buf, sizeof(buf));
	if (ret)
		return ret;

	memcpy((u8 *)&old_sum, buf + AR6003_BOARD_DATA_OFFSET, 2);
	memcpy((u8 *)&old_ver, buf + AR6003_BOARD_DATA_OFFSET + 2, 2);
	memcpy((u8 *)&old_rd, buf + AR6003_RD_OFFSET, 2);
	memcpy((u8 *)&old_rd_next, buf + AR6003_RD_OFFSET + 2, 2);

	/*
	 * Overwrite the new regulatory domain and preserve the
	 * MAC addr which is in the same word.
	 */
	new_rd = cpu_to_le32((le32_to_cpu(old_rd_next) << 16) + reg_domain);
	ret = ath6kl_bmi_write(ar,
		cpu_to_le32(le32_to_cpu(brd_dat_addr) + AR6003_RD_OFFSET),
		(u8 *)&new_rd, 4);
	if (ret)
		return ret;

	/*
	 * Recompute the board data checksum with the new regulatory
	 * domain, preserve the version information which is in the
	 * same word.
	 */
	new_sum = cpu_to_le32((le32_to_cpu(old_ver) << 16) +
			      (le32_to_cpu(old_sum) ^ le32_to_cpu(old_rd) ^
			       reg_domain));
	ret = ath6kl_bmi_write(ar,
		cpu_to_le32(le32_to_cpu(brd_dat_addr) +
		AR6003_BOARD_DATA_OFFSET),
		(u8 *)&new_sum, 4);

	return ret;
}

struct sk_buff *ath6kl_buf_alloc(int size)
{
	struct sk_buff *skb;
	u16 reserved;

	/* Add chacheline space at front and back of buffer */
	reserved = (2 * L1_CACHE_BYTES) + ATH6KL_DATA_OFFSET +
		   sizeof(struct htc_packet) + ATH6KL_HTC_ALIGN_BYTES;
	skb = dev_alloc_skb(size + reserved);

	if (skb)
		skb_reserve(skb, reserved - L1_CACHE_BYTES);
	return skb;
}

void ath6kl_init_profile_info(struct ath6kl_vif *vif)
{
	vif->ssid_len = 0;
	memset(vif->ssid, 0, sizeof(vif->ssid));

	vif->dot11_auth_mode = OPEN_AUTH;
	vif->auth_mode = NONE_AUTH;
	vif->prwise_crypto = NONE_CRYPT;
	vif->prwise_crypto_len = 0;
	vif->grp_crypto = NONE_CRYPT;
	vif->grp_crypto_len = 0;
	memset(vif->wep_key_list, 0, sizeof(vif->wep_key_list));
	memset(vif->req_bssid, 0, sizeof(vif->req_bssid));
	memset(vif->bssid, 0, sizeof(vif->bssid));
	vif->bss_ch = 0;
}

static int ath6kl_set_host_app_area(struct ath6kl *ar)
{
	u32 address, data;
	struct host_app_area host_app_area;

	/* Fetch the address of the host_app_area_s
	 * instance in the host interest area */
	address = ath6kl_get_hi_item_addr(ar, HI_ITEM(hi_app_host_interest));
	address = TARG_VTOP(ar->target_type, address);

	if (ath6kl_diag_read32(ar, address, &data))
		return -EIO;

	address = TARG_VTOP(ar->target_type, data);
	host_app_area.wmi_protocol_ver = cpu_to_le32(WMI_PROTOCOL_VERSION);
	if (ath6kl_diag_write(ar, address, (u8 *) &host_app_area,
			      sizeof(struct host_app_area)))
		return -EIO;

	return 0;
}

static inline void set_ac2_ep_map(struct ath6kl *ar,
				  u8 ac,
				  enum htc_endpoint_id ep)
{
	ar->ac2ep_map[ac] = ep;
	ar->ep2ac_map[ep] = ac;
}

/* connect to a service */
static int ath6kl_connectservice(struct ath6kl *ar,
				 struct htc_service_connect_req  *con_req,
				 char *desc)
{
	int status;
	struct htc_service_connect_resp response;

	memset(&response, 0, sizeof(response));

	status = ath6kl_htc_conn_service(ar->htc_target, con_req, &response);
	if (status) {
		ath6kl_err("failed to connect to %s service status:%d\n",
			   desc, status);
		return status;
	}

	switch (con_req->svc_id) {
	case WMI_CONTROL_SVC:
		if (test_bit(WMI_ENABLED, &ar->flag))
			ath6kl_wmi_set_control_ep(ar->wmi, response.endpoint);
		ar->ctrl_ep = response.endpoint;
		break;
	case WMI_DATA_BE_SVC:
		set_ac2_ep_map(ar, WMM_AC_BE, response.endpoint);
		break;
	case WMI_DATA_BK_SVC:
		set_ac2_ep_map(ar, WMM_AC_BK, response.endpoint);
		break;
	case WMI_DATA_VI_SVC:
		set_ac2_ep_map(ar, WMM_AC_VI, response.endpoint);
		break;
	case WMI_DATA_VO_SVC:
		set_ac2_ep_map(ar, WMM_AC_VO, response.endpoint);
		break;
	default:
		ath6kl_err("service id is not mapped %d\n", con_req->svc_id);
		return -EINVAL;
	}

	return 0;
}

static int ath6kl_init_service_ep(struct ath6kl *ar)
{
	struct htc_service_connect_req connect;

	memset(&connect, 0, sizeof(connect));

	/* these fields are the same for all service endpoints */
	connect.ep_cb.rx = ath6kl_rx;
	connect.ep_cb.rx_refill = ath6kl_rx_refill;
	connect.ep_cb.tx_full = ath6kl_tx_queue_full;

	/*
	 * Set the max queue depth so that our ath6kl_tx_queue_full handler
	 * gets called.
	*/
	connect.max_txq_depth = MAX_DEFAULT_SEND_QUEUE_DEPTH;
	connect.ep_cb.rx_refill_thresh = ATH6KL_MAX_RX_BUFFERS / 4;
	if (!connect.ep_cb.rx_refill_thresh)
		connect.ep_cb.rx_refill_thresh++;

	/* connect to control service */
	connect.svc_id = WMI_CONTROL_SVC;
	if (ath6kl_connectservice(ar, &connect, "WMI CONTROL"))
		return -EIO;

	connect.flags |= HTC_FLGS_TX_BNDL_PAD_EN;

	/*
	 * Limit the HTC message size on the send path, although e can
	 * receive A-MSDU frames of 4K, we will only send ethernet-sized
	 * (802.3) frames on the send path.
	 */
	connect.max_rxmsg_sz = WMI_MAX_TX_DATA_FRAME_LENGTH;

	/*
	 * To reduce the amount of committed memory for larger A_MSDU
	 * frames, use the recv-alloc threshold mechanism for larger
	 * packets.
	 */
	connect.ep_cb.rx_alloc_thresh = ATH6KL_BUFFER_SIZE;
	connect.ep_cb.rx_allocthresh = ath6kl_alloc_amsdu_rxbuf;

	/*
	 * For the remaining data services set the connection flag to
	 * reduce dribbling, if configured to do so.
	 */
	connect.conn_flags |= HTC_CONN_FLGS_REDUCE_CRED_DRIB;
	connect.conn_flags &= ~HTC_CONN_FLGS_THRESH_MASK;
	connect.conn_flags |= HTC_CONN_FLGS_THRESH_LVL_HALF;

	connect.svc_id = WMI_DATA_BE_SVC;

	if (ath6kl_connectservice(ar, &connect, "WMI DATA BE"))
		return -EIO;

	/* connect to back-ground map this to WMI LOW_PRI */
	connect.svc_id = WMI_DATA_BK_SVC;
	if (ath6kl_connectservice(ar, &connect, "WMI DATA BK"))
		return -EIO;

	/* connect to Video service, map this to to HI PRI */
	connect.svc_id = WMI_DATA_VI_SVC;
	if (ath6kl_connectservice(ar, &connect, "WMI DATA VI"))
		return -EIO;

	/*
	 * Connect to VO service, this is currently not mapped to a WMI
	 * priority stream due to historical reasons. WMI originally
	 * defined 3 priorities over 3 mailboxes We can change this when
	 * WMI is reworked so that priorities are not dependent on
	 * mailboxes.
	 */
	connect.svc_id = WMI_DATA_VO_SVC;
	if (ath6kl_connectservice(ar, &connect, "WMI DATA VO"))
		return -EIO;

	return 0;
}

void ath6kl_init_control_info(struct ath6kl_vif *vif)
{
	ath6kl_init_profile_info(vif);
	vif->def_txkey_index = 0;
	memset(vif->wep_key_list, 0, sizeof(vif->wep_key_list));
	vif->ch_hint = 0;
}

/*
 * Set HTC/Mbox operational parameters, this can only be called when the
 * target is in the BMI phase.
 */
static int ath6kl_set_htc_params(struct ath6kl *ar, u32 mbox_isr_yield_val,
				 u8 htc_ctrl_buf)
{
	int status;
	u32 blk_size;

	blk_size = ar->mbox_info.block_size;

	if (htc_ctrl_buf)
		blk_size |=  ((u32)htc_ctrl_buf) << 16;

	/* set the host interest area for the block size */
	status = ath6kl_bmi_write(ar,
			ath6kl_get_hi_item_addr(ar,
			HI_ITEM(hi_mbox_io_block_sz)),
			(u8 *)&blk_size,
			4);
	if (status) {
		ath6kl_err("bmi_write_memory for IO block size failed\n");
		goto out;
	}

	ath6kl_dbg(ATH6KL_DBG_TRC, "block size set: %d (target addr:0x%X)\n",
		   blk_size,
		   ath6kl_get_hi_item_addr(ar, HI_ITEM(hi_mbox_io_block_sz)));

	if (mbox_isr_yield_val) {
		/* set the host interest area for the mbox ISR yield limit */
		status = ath6kl_bmi_write(ar,
				ath6kl_get_hi_item_addr(ar,
				HI_ITEM(hi_mbox_isr_yield_limit)),
				(u8 *)&mbox_isr_yield_val,
				4);
		if (status) {
			ath6kl_err("bmi_write_memory for yield limit failed\n");
			goto out;
		}
	}

out:
	return status;
}

static int ath6kl_target_config_wlan_params(struct ath6kl *ar, int idx)
{
	int status = 0;
	int ret;
#ifdef CONFIG_MACH_PX
	struct ath6kl_vif *vif = ath6kl_get_vif_by_index(ar, idx);
#endif
	/*
	 * Configure the device for rx dot11 header rules. "0,0" are the
	 * default values. Required if checksum offload is needed. Set
	 * RxMetaVersion to 2.
	 */
	if (ath6kl_wmi_set_rx_frame_format_cmd(ar->wmi, idx,
					       ar->rx_meta_ver, 0, 0)) {
		ath6kl_err("unable to set the rx frame format\n");
		status = -EIO;
	}


#ifdef CONFIG_MACH_PX
	if (ar->conf_flags & ATH6KL_CONF_IGNORE_PS_FAIL_EVT_IN_SCAN) {
		if ((ath6kl_wmi_pmparams_cmd(ar->wmi, idx,
			0, vif->pspoll_num, 0, 0, 1,
			IGNORE_POWER_SAVE_FAIL_EVENT_DURING_SCAN)) != 0) {
			ath6kl_err("unable to set power save fail event policy during scan\n");
			status = -EIO;
		}
	} else {
		if ((ath6kl_wmi_pmparams_cmd(ar->wmi, idx, 0,
				 vif->pspoll_num, 0, 0, 1, 0)) != 0) {
			ath6kl_err("unable to set pm params\n");
			status = -EIO;
		}
	}
#else
	if (ar->conf_flags & ATH6KL_CONF_IGNORE_PS_FAIL_EVT_IN_SCAN)
		if ((ath6kl_wmi_pmparams_cmd(ar->wmi, idx, 0, 1, 0, 0, 1,
		     IGNORE_POWER_SAVE_FAIL_EVENT_DURING_SCAN)) != 0) {
			ath6kl_err("unable to set power save fail event policy\n");
			status = -EIO;
		}
#endif


	if (!(ar->conf_flags & ATH6KL_CONF_IGNORE_ERP_BARKER))
		if ((ath6kl_wmi_set_lpreamble_cmd(ar->wmi, idx, 0,
		     WMI_DONOT_IGNORE_BARKER_IN_ERP)) != 0) {
			ath6kl_err("unable to set barker preamble policy\n");
			status = -EIO;
		}

	if (ath6kl_wmi_set_keepalive_cmd(ar->wmi, idx,
			WLAN_CONFIG_KEEP_ALIVE_INTERVAL)) {
		ath6kl_err("unable to set keep alive interval\n");
		status = -EIO;
	}

	if (ath6kl_wmi_disctimeout_cmd(ar->wmi, idx,
			WLAN_CONFIG_DISCONNECT_TIMEOUT)) {
		ath6kl_err("unable to set disconnect timeout\n");
		status = -EIO;
	}

	if (!(ar->conf_flags & ATH6KL_CONF_ENABLE_TX_BURST))
		if (ath6kl_wmi_set_wmm_txop(ar->wmi, idx, WMI_TXOP_DISABLED)) {
			ath6kl_err("unable to set txop bursting\n");
			status = -EIO;
		}

	if (ar->p2p && (ar->vif_max == 1 || idx)) {
		ret = ath6kl_wmi_info_req_cmd(ar->wmi, idx,
					      P2P_FLAG_CAPABILITIES_REQ |
					      P2P_FLAG_MACADDR_REQ |
					      P2P_FLAG_HMODEL_REQ);
		if (ret) {
			ath6kl_dbg(ATH6KL_DBG_TRC, "failed to request P2P "
				   "capabilities (%d) - assuming P2P not "
				   "supported\n", ret);
			ar->p2p = 0;
		}
	}

	if (ar->p2p && (ar->vif_max == 1 || idx)) {
		/* Enable Probe Request reporting for P2P */
		ret = ath6kl_wmi_probe_report_req_cmd(ar->wmi, idx, true);
		if (ret) {
			ath6kl_dbg(ATH6KL_DBG_TRC, "failed to enable Probe "
				   "Request reporting (%d)\n", ret);
		}
	}

#ifdef CONFIG_MACH_PX
	if (vif->nw_type == INFRA_NETWORK) {
		ath6kl_dbg(ATH6KL_DBG_TRC, "AR6K: bg scan interval = %d, active dwell time = %d, passive dwell time = %d\n",
			vif->scparams.bg_period,
			vif->scparams.maxact_chdwell_time,
			vif->scparams.pas_chdwell_time);

		ath6kl_wmi_scanparams_cmd(ar->wmi, idx,
		      vif->scparams.fg_start_period,
		      vif->scparams.fg_end_period, vif->scparams.bg_period,
		      vif->scparams.minact_chdwell_time,
		      vif->scparams.maxact_chdwell_time,
		      vif->scparams.pas_chdwell_time,
		      vif->scparams.short_scan_ratio,
		      vif->scparams.scan_ctrl_flags,
		      vif->scparams.max_dfsch_act_time,
		      vif->scparams.maxact_scan_per_ssid);

		ath6kl_wmi_set_roam_lrssi_cmd(ar->wmi, lrssi);
	}
#endif
	return status;
}

int ath6kl_configure_target(struct ath6kl *ar)
{
	u32 param, ram_reserved_size;
	u8 fw_iftype, fw_mode = 0, fw_submode = 0;
	int i, status;

	param = uart_debug;
	if (ath6kl_bmi_write(ar, ath6kl_get_hi_item_addr(ar,
			     HI_ITEM(hi_serial_enable)), (u8 *)&param, 4)) {
		ath6kl_err("bmi_write_memory for uart debug failed\n");
		return -EIO;
	}

	/*
	 * Note: Even though the firmware interface type is
	 * chosen as BSS_STA for all three interfaces, can
	 * be configured to IBSS/AP as long as the fw submode
	 * remains normal mode (0 - AP, STA and IBSS). But
	 * due to an target assert in firmware only one interface is
	 * configured for now.
	 */
	fw_iftype = HI_OPTION_FW_MODE_BSS_STA;

	for (i = 0; i < ar->vif_max; i++)
		fw_mode |= fw_iftype << (i * HI_OPTION_FW_MODE_BITS);

	/*
	 * By default, submodes :
	 *		vif[0] - AP/STA/IBSS
	 *		vif[1] - "P2P dev"/"P2P GO"/"P2P Client"
	 *		vif[2] - "P2P dev"/"P2P GO"/"P2P Client"
	 */

	for (i = 0; i < ar->max_norm_iface; i++)
		fw_submode |= HI_OPTION_FW_SUBMODE_NONE <<
			      (i * HI_OPTION_FW_SUBMODE_BITS);

	for (i = ar->max_norm_iface; i < ar->vif_max; i++)
		fw_submode |= HI_OPTION_FW_SUBMODE_P2PDEV <<
			      (i * HI_OPTION_FW_SUBMODE_BITS);

	if (ar->p2p && ar->vif_max == 1)
		fw_submode = HI_OPTION_FW_SUBMODE_P2PDEV;

	param = HTC_PROTOCOL_VERSION;
	if (ath6kl_bmi_write(ar,
			     ath6kl_get_hi_item_addr(ar,
			     HI_ITEM(hi_app_host_interest)),
			     (u8 *)&param, 4) != 0) {
		ath6kl_err("bmi_write_memory for htc version failed\n");
		return -EIO;
	}

	/* set the firmware mode to STA/IBSS/AP */
	param = 0;

	if (ath6kl_bmi_read(ar,
			    ath6kl_get_hi_item_addr(ar,
			    HI_ITEM(hi_option_flag)),
			    (u8 *)&param, 4) != 0) {
		ath6kl_err("bmi_read_memory for setting fwmode failed\n");
		return -EIO;
	}

	param |= (ar->vif_max << HI_OPTION_NUM_DEV_SHIFT);
	param |= fw_mode << HI_OPTION_FW_MODE_SHIFT;
	param |= fw_submode << HI_OPTION_FW_SUBMODE_SHIFT;

	param |= (0 << HI_OPTION_MAC_ADDR_METHOD_SHIFT);
	param |= (0 << HI_OPTION_FW_BRIDGE_SHIFT);

	if (ath6kl_bmi_write(ar,
			     ath6kl_get_hi_item_addr(ar,
			     HI_ITEM(hi_option_flag)),
			     (u8 *)&param,
			     4) != 0) {
		ath6kl_err("bmi_write_memory for setting fwmode failed\n");
		return -EIO;
	}

	ath6kl_dbg(ATH6KL_DBG_TRC, "firmware mode set\n");

	/*
	 * Hardcode the address use for the extended board data
	 * Ideally this should be pre-allocate by the OS at boot time
	 * But since it is a new feature and board data is loaded
	 * at init time, we have to workaround this from host.
	 * It is difficult to patch the firmware boot code,
	 * but possible in theory.
	 */

	param = ar->hw.board_ext_data_addr;
	ram_reserved_size = ar->hw.reserved_ram_size;

	if (ath6kl_bmi_write(ar, ath6kl_get_hi_item_addr(ar,
					HI_ITEM(hi_board_ext_data)),
			     (u8 *)&param, 4) != 0) {
		ath6kl_err("bmi_write_memory for hi_board_ext_data failed\n");
		return -EIO;
	}

	if (ath6kl_bmi_write(ar, ath6kl_get_hi_item_addr(ar,
					HI_ITEM(hi_end_ram_reserve_sz)),
			     (u8 *)&ram_reserved_size, 4) != 0) {
		ath6kl_err("bmi_write_memory for hi_end_ram_reserve_sz failed\n");
		return -EIO;
	}

	/* set the block size for the target */
	if (ath6kl_set_htc_params(ar, MBOX_YIELD_LIMIT, 0))
		/* use default number of control buffers */
		return -EIO;

	/* Configure GPIO AR600x UART */
	param = ar->hw.uarttx_pin;
	status = ath6kl_bmi_write(ar,
				ath6kl_get_hi_item_addr(ar,
				HI_ITEM(hi_dbg_uart_txpin)),
				(u8 *)&param, 4);
	if (status)
		return status;

	/* Configure target refclk_hz */
	param =  ar->hw.refclk_hz;
	status = ath6kl_bmi_write(ar,
				ath6kl_get_hi_item_addr(ar,
				HI_ITEM(hi_refclk_hz)),
				(u8 *)&param, 4);
	if (status)
		return status;

	return 0;
}

void ath6kl_core_free(struct ath6kl *ar)
{
	wiphy_free(ar->wiphy);
}

void ath6kl_core_cleanup(struct ath6kl *ar)
{
	ath6kl_hif_power_off(ar);

	destroy_workqueue(ar->ath6kl_wq);

	if (ar->htc_target)
		ath6kl_htc_cleanup(ar->htc_target);

	ath6kl_cookie_cleanup(ar);

	ath6kl_cleanup_amsdu_rxbufs(ar);

	ath6kl_bmi_cleanup(ar);

	ath6kl_debug_cleanup(ar);

	vfree(ar->fw_board);
	vfree(ar->fw_otp);
	vfree(ar->fw);
	vfree(ar->fw_patch);
	vfree(ar->fw_testscript);

	ath6kl_deinit_ieee80211_hw(ar);
}

/* firmware upload */
static int ath6kl_get_fw(struct ath6kl *ar, const char *filename,
			 u8 **fw, size_t *fw_len)
{
	const struct firmware *fw_entry;
	int ret;

	ret = request_firmware(&fw_entry, filename, ar->dev);
	if (ret)
		return ret;

	*fw_len = fw_entry->size;
	*fw = vmalloc(fw_entry->size);

	if (*fw == NULL)
		ret = -ENOMEM;

	memcpy(*fw, fw_entry->data, fw_entry->size);

	release_firmware(fw_entry);

	return ret;
}

#if 0
#ifdef CONFIG_OF
/*
 * Check the device tree for a board-id and use it to construct
 * the pathname to the firmware file.  Used (for now) to find a
 * fallback to the "bdata.bin" file--typically a symlink to the
 * appropriate board-specific file.
 */
static bool check_device_tree(struct ath6kl *ar)
{
	static const char *board_id_prop = "atheros,board-id";
	struct device_node *node;
	char board_filename[64];
	const char *board_id;
	int ret;

	for_each_compatible_node(node, NULL, "atheros,ath6kl") {
		board_id = of_get_property(node, board_id_prop, NULL);
		if (board_id == NULL) {
			ath6kl_warn("No \"%s\" property on %s node.\n",
				    board_id_prop, node->name);
			continue;
		}
		snprintf(board_filename, sizeof(board_filename),
			 "%s/bdata.%s.bin", ar->hw.fw.dir, board_id);

		ret = ath6kl_get_fw(ar, board_filename, &ar->fw_board,
				    &ar->fw_board_len);
		if (ret) {
			ath6kl_err("Failed to get DT board file %s: %d\n",
				   board_filename, ret);
			continue;
		}
		return true;
	}
	return false;
}
#else
static bool check_device_tree(struct ath6kl *ar)
{
	return false;
}
#endif /* CONFIG_OF */
#endif
static int ath6kl_fetch_board_file(struct ath6kl *ar)
{
	const char *filename;
	int ret;

	if (ar->fw_board != NULL)
		return 0;

	if (WARN_ON(ar->hw.fw_board == NULL))
		return -EINVAL;

	filename = ar->hw.fw_board;

	ret = ath6kl_get_fw(ar, filename, &ar->fw_board,
			    &ar->fw_board_len);
	if (ret == 0) {
		/* managed to get proper board file */
		return 0;
	}
#if 0
	if (check_device_tree(ar)) {
		/* got board file from device tree */
		return 0;
	}
#endif
	/* there was no proper board file, try to use default instead */
	ath6kl_warn("Failed to get board file %s (%d), trying to find default board file.\n",
		    filename, ret);

	filename = ar->hw.fw_default_board;

	ret = ath6kl_get_fw(ar, filename, &ar->fw_board,
			    &ar->fw_board_len);
	if (ret) {
		ath6kl_err("Failed to get default board file %s: %d\n",
			   filename, ret);
		return ret;
	}

	ath6kl_warn("WARNING! No proper board file was not found, instead using a default board file.\n");
	ath6kl_warn("Most likely your hardware won't work as specified. Install correct board file!\n");

	return 0;
}

static int ath6kl_fetch_otp_file(struct ath6kl *ar)
{
	char filename[100];
	int ret;

	if (ar->fw_otp != NULL)
		return 0;

	if (ar->hw.fw.otp == NULL) {
		ath6kl_dbg(ATH6KL_DBG_BOOT,
			   "no OTP file configured for this hw\n");
		return 0;
	}

	snprintf(filename, sizeof(filename), "%s/%s",
		 ar->hw.fw.dir, ar->hw.fw.otp);

	ret = ath6kl_get_fw(ar, filename, &ar->fw_otp,
			    &ar->fw_otp_len);
	if (ret) {
		ath6kl_err("Failed to get OTP file %s: %d\n",
			   filename, ret);
		return ret;
	}

	return 0;
}

static int ath6kl_fetch_testmode_file(struct ath6kl *ar)
{
	char filename[100];
	int ret;

	if (testmode == 0)
		return 0;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "testmode %d\n", testmode);

	if (testmode == 2) {
		if (ar->hw.fw.utf == NULL) {
			ath6kl_warn("testmode 2 not supported\n");
			return -EOPNOTSUPP;
		}

		snprintf(filename, sizeof(filename), "%s/%s",
			 ar->hw.fw.dir, ar->hw.fw.utf);
	} else {
		if (ar->hw.fw.tcmd == NULL) {
			ath6kl_warn("testmode 1 not supported\n");
			return -EOPNOTSUPP;
		}

		snprintf(filename, sizeof(filename), "%s/%s",
			 ar->hw.fw.dir, ar->hw.fw.tcmd);
	}

	set_bit(TESTMODE, &ar->flag);

	ret = ath6kl_get_fw(ar, filename, &ar->fw, &ar->fw_len);
	if (ret) {
		ath6kl_err("Failed to get testmode %d firmware file %s: %d\n",
			   testmode, filename, ret);
		return ret;
	}

	return 0;
}

static int ath6kl_fetch_fw_file(struct ath6kl *ar)
{
	char filename[100];
	int ret;

	if (ar->fw != NULL)
		return 0;

	/* FIXME: remove WARN_ON() as we won't support FW API 1 for long */
	if (WARN_ON(ar->hw.fw.fw == NULL))
		return -EINVAL;

	snprintf(filename, sizeof(filename), "%s/%s",
		 ar->hw.fw.dir, ar->hw.fw.fw);

	ret = ath6kl_get_fw(ar, filename, &ar->fw, &ar->fw_len);
	if (ret) {
		ath6kl_err("Failed to get firmware file %s: %d\n",
			   filename, ret);
		return ret;
	}

	return 0;
}

static int ath6kl_fetch_patch_file(struct ath6kl *ar)
{
	char filename[100];
	int ret;

	if (ar->fw_patch != NULL)
		return 0;

	if (ar->hw.fw.patch == NULL)
		return 0;

	snprintf(filename, sizeof(filename), "%s/%s",
		 ar->hw.fw.dir, ar->hw.fw.patch);

	ret = ath6kl_get_fw(ar, filename, &ar->fw_patch,
			    &ar->fw_patch_len);
	if (ret) {
		ath6kl_err("Failed to get patch file %s: %d\n",
			   filename, ret);
		return ret;
	}

	return 0;
}

static int ath6kl_fetch_testscript_file(struct ath6kl *ar)
{
	char filename[100];
	int ret;

	if (testmode != 2)
		return 0;

	if (ar->fw_testscript != NULL)
		return 0;

	if (ar->hw.fw.testscript == NULL)
		return 0;

	snprintf(filename, sizeof(filename), "%s/%s",
		ar->hw.fw.dir, ar->hw.fw.testscript);

	ret = ath6kl_get_fw(ar, filename, &ar->fw_testscript,
				&ar->fw_testscript_len);
	if (ret) {
		ath6kl_err("Failed to get testscript file %s: %d\n",
			filename, ret);
		return ret;
	}

	return 0;
}

static int ath6kl_fetch_fw_api1(struct ath6kl *ar)
{
	int ret;

	ret = ath6kl_fetch_otp_file(ar);
	if (ret)
		return ret;

	ret = ath6kl_fetch_fw_file(ar);
	if (ret)
		return ret;

	ret = ath6kl_fetch_patch_file(ar);
	if (ret)
		return ret;

	ret = ath6kl_fetch_testscript_file(ar);
	if (ret)
		return ret;

	return 0;
}

static int ath6kl_fetch_fw_apin(struct ath6kl *ar, const char *name)
{
	size_t magic_len, len, ie_len;
	const struct firmware *fw;
	struct ath6kl_fw_ie *hdr;
	char filename[100];
	const u8 *data;
	int ret, ie_id, i, index, bit;
	__le32 *val;

	snprintf(filename, sizeof(filename), "%s/%s", ar->hw.fw.dir, name);

	ret = request_firmware(&fw, filename, ar->dev);
	if (ret)
		return ret;

	data = fw->data;
	len = fw->size;

	/* magic also includes the null byte, check that as well */
	magic_len = strlen(ATH6KL_FIRMWARE_MAGIC) + 1;

	if (len < magic_len) {
		ret = -EINVAL;
		goto out;
	}

	if (memcmp(data, ATH6KL_FIRMWARE_MAGIC, magic_len) != 0) {
		ret = -EINVAL;
		goto out;
	}

	len -= magic_len;
	data += magic_len;

	/* loop elements */
	while (len > sizeof(struct ath6kl_fw_ie)) {
		/* hdr is unaligned! */
		hdr = (struct ath6kl_fw_ie *) data;

		ie_id = le32_to_cpup(&hdr->id);
		ie_len = le32_to_cpup(&hdr->len);

		len -= sizeof(*hdr);
		data += sizeof(*hdr);

		if (len < ie_len) {
			ret = -EINVAL;
			goto out;
		}

		switch (ie_id) {
		case ATH6KL_FW_IE_OTP_IMAGE:
			ath6kl_dbg(ATH6KL_DBG_BOOT, "found otp image ie (%zd B)\n",
				ie_len);

			ar->fw_otp = vmalloc(ie_len);

			if (ar->fw_otp == NULL) {
				ret = -ENOMEM;
				goto out;
			}

			memcpy(ar->fw_otp, data, ie_len);
			ar->fw_otp_len = ie_len;
			break;
		case ATH6KL_FW_IE_FW_IMAGE:
			ath6kl_dbg(ATH6KL_DBG_BOOT, "found fw image ie (%zd B)\n",
				ie_len);

			/* in testmode we already might have a fw file */
			if (ar->fw != NULL)
				break;

			ar->fw = vmalloc(ie_len);

			if (ar->fw == NULL) {
				ret = -ENOMEM;
				goto out;

			}
			memcpy(ar->fw, data, ie_len);
			ar->fw_len = ie_len;
			break;
		case ATH6KL_FW_IE_PATCH_IMAGE:
			ath6kl_dbg(ATH6KL_DBG_BOOT, "found patch image ie (%zd B)\n",
				ie_len);

			ar->fw_patch = vmalloc(ie_len);

			if (ar->fw_patch == NULL) {
				ret = -ENOMEM;
				goto out;
			}

			memcpy(ar->fw_patch, data, ie_len);
			ar->fw_patch_len = ie_len;
			break;
		case ATH6KL_FW_IE_RESERVED_RAM_SIZE:
			val = (__le32 *) data;
			ar->hw.reserved_ram_size = le32_to_cpup(val);

			ath6kl_dbg(ATH6KL_DBG_BOOT,
				   "found reserved ram size ie 0x%d\n",
				   ar->hw.reserved_ram_size);
			break;
		case ATH6KL_FW_IE_CAPABILITIES:
			if (ie_len < DIV_ROUND_UP(ATH6KL_FW_CAPABILITY_MAX, 8))
				break;

			ath6kl_dbg(ATH6KL_DBG_BOOT,
				   "found firmware capabilities ie (%zd B)\n",
				   ie_len);

			for (i = 0; i < ATH6KL_FW_CAPABILITY_MAX; i++) {
				index = i / 8;
				bit = i % 8;

				if (data[index] & (1 << bit))
					__set_bit(i, ar->fw_capabilities);
			}

			ath6kl_dbg_dump(ATH6KL_DBG_BOOT, "capabilities", "",
					ar->fw_capabilities,
					sizeof(ar->fw_capabilities));
			break;
		case ATH6KL_FW_IE_PATCH_ADDR:
			if (ie_len != sizeof(*val))
				break;

			val = (__le32 *) data;
			ar->hw.dataset_patch_addr = le32_to_cpup(val);

			ath6kl_dbg(ATH6KL_DBG_BOOT,
				   "found patch address ie 0x%x\n",
				   ar->hw.dataset_patch_addr);
			break;
		case ATH6KL_FW_IE_BOARD_ADDR:
			if (ie_len != sizeof(*val))
				break;

			val = (__le32 *) data;
			ar->hw.board_addr = le32_to_cpup(val);

			ath6kl_dbg(ATH6KL_DBG_BOOT,
				   "found board address ie 0x%x\n",
				   ar->hw.board_addr);
			break;
		case ATH6KL_FW_IE_VIF_MAX:
			if (ie_len != sizeof(*val))
				break;

			val = (__le32 *) data;
			ar->vif_max = min_t(unsigned int, le32_to_cpup(val),
					    ATH6KL_VIF_MAX);

			if (ar->vif_max > 1 && !ar->p2p)
				ar->max_norm_iface = 2;

			ath6kl_dbg(ATH6KL_DBG_BOOT,
				   "found vif max ie %d\n", ar->vif_max);
			break;
		default:
			ath6kl_dbg(ATH6KL_DBG_BOOT, "Unknown fw ie: %u\n",
				   le32_to_cpup(&hdr->id));
			break;
		}

		len -= ie_len;
		data += ie_len;
	};

	ret = 0;
out:
	release_firmware(fw);

	return ret;
}

static int ath6kl_fetch_firmwares(struct ath6kl *ar)
{
	int ret;

#ifdef CONFIG_MACH_PX
	if (testmode)
		ar->hw.fw_board = AR6003_HW_2_1_1_TCMD_BOARD_DATA_FILE;
#endif

	ret = ath6kl_fetch_board_file(ar);
	if (ret)
		return ret;

	ret = ath6kl_fetch_testmode_file(ar);
	if (ret)
		return ret;

	ret = ath6kl_fetch_fw_apin(ar, ATH6KL_FW_API3_FILE);
	if (ret == 0) {
		ar->fw_api = 3;
		goto out;
	}

	ret = ath6kl_fetch_fw_apin(ar, ATH6KL_FW_API2_FILE);
	if (ret == 0) {
		ar->fw_api = 2;
		goto out;
	}

	ret = ath6kl_fetch_fw_api1(ar);
	if (ret)
		return ret;

	ar->fw_api = 1;

out:
	ath6kl_dbg(ATH6KL_DBG_BOOT, "using fw api %d\n", ar->fw_api);

	return 0;
}

static int ath6kl_upload_board_file(struct ath6kl *ar)
{
	u32 board_address, board_ext_address, param;
	u32 board_data_size, board_ext_data_size;
	int ret;

	if (WARN_ON(ar->fw_board == NULL))
		return -ENOENT;

	/*
	 * Determine where in Target RAM to write Board Data.
	 * For AR6004, host determine Target RAM address for
	 * writing board data.
	 */
	if (ar->hw.board_addr != 0) {
		board_address = ar->hw.board_addr;
		ath6kl_bmi_write(ar,
				ath6kl_get_hi_item_addr(ar,
				HI_ITEM(hi_board_data)),
				(u8 *) &board_address, 4);
	} else {
		ath6kl_bmi_read(ar,
				ath6kl_get_hi_item_addr(ar,
				HI_ITEM(hi_board_data)),
				(u8 *) &board_address, 4);
	}

	/* determine where in target ram to write extended board data */
	ath6kl_bmi_read(ar,
			ath6kl_get_hi_item_addr(ar,
			HI_ITEM(hi_board_ext_data)),
			(u8 *) &board_ext_address, 4);

	if (ar->target_type == TARGET_TYPE_AR6003 &&
	    board_ext_address == 0) {
		ath6kl_err("Failed to get board file target address.\n");
		return -EINVAL;
	}

	switch (ar->target_type) {
	case TARGET_TYPE_AR6003:
		board_data_size = AR6003_BOARD_DATA_SZ;
		board_ext_data_size = AR6003_BOARD_EXT_DATA_SZ;
		if (ar->fw_board_len > (board_data_size + board_ext_data_size))
			board_ext_data_size = AR6003_BOARD_EXT_DATA_SZ_V2;
		break;
	case TARGET_TYPE_AR6004:
		board_data_size = AR6004_BOARD_DATA_SZ;
		board_ext_data_size = AR6004_BOARD_EXT_DATA_SZ;
		break;
	default:
		WARN_ON(1);
		return -EINVAL;
		break;
	}

	if (board_ext_address &&
	    ar->fw_board_len == (board_data_size + board_ext_data_size)) {

		/* write extended board data */
		ath6kl_dbg(ATH6KL_DBG_BOOT,
			   "writing extended board data to 0x%x (%d B)\n",
			   board_ext_address, board_ext_data_size);

		ret = ath6kl_bmi_write(ar, board_ext_address,
				       ar->fw_board + board_data_size,
				       board_ext_data_size);
		if (ret) {
			ath6kl_err("Failed to write extended board data: %d\n",
				   ret);
			return ret;
		}

		/* record that extended board data is initialized */
		param = (board_ext_data_size << 16) | 1;

		ath6kl_bmi_write(ar,
				 ath6kl_get_hi_item_addr(ar,
				 HI_ITEM(hi_board_ext_data_config)),
				 (unsigned char *) &param, 4);
	}

	if (ar->fw_board_len < board_data_size) {
		ath6kl_err("Too small board file: %zu\n", ar->fw_board_len);
		ret = -EINVAL;
		return ret;
	}

	ath6kl_dbg(ATH6KL_DBG_BOOT, "writing board file to 0x%x (%d B)\n",
		   board_address, board_data_size);

	ret = ath6kl_bmi_write(ar, board_address, ar->fw_board,
			       board_data_size);

	if (ret) {
		ath6kl_err("Board file bmi write failed: %d\n", ret);
		return ret;
	}

	/* record the fact that Board Data IS initialized */
	param = 1;
	ath6kl_bmi_write(ar,
			 ath6kl_get_hi_item_addr(ar,
			 HI_ITEM(hi_board_data_initialized)),
			 (u8 *)&param, 4);

	return ret;
}

static int ath6kl_upload_otp(struct ath6kl *ar)
{
	u32 address, param;
	bool from_hw = false;
	int ret;

	if (ar->fw_otp == NULL)
		return 0;

	address = ar->hw.app_load_addr;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "writing otp to 0x%x (%zd B)\n", address,
		   ar->fw_otp_len);

	ret = ath6kl_bmi_fast_download(ar, address, ar->fw_otp,
				       ar->fw_otp_len);
	if (ret) {
		ath6kl_err("Failed to upload OTP file: %d\n", ret);
		return ret;
	}

	/* read firmware start address */
	ret = ath6kl_bmi_read(ar,
			      ath6kl_get_hi_item_addr(ar,
						      HI_ITEM(hi_app_start)),
			      (u8 *) &address, sizeof(address));

	if (ret) {
		ath6kl_err("Failed to read hi_app_start: %d\n", ret);
		return ret;
	}

	if (ar->hw.app_start_override_addr == 0) {
		ar->hw.app_start_override_addr = address;
		from_hw = true;
	}

	ath6kl_dbg(ATH6KL_DBG_BOOT, "app_start_override_addr%s 0x%x\n",
		   from_hw ? " (from hw)" : "",
		   ar->hw.app_start_override_addr);

	/* execute the OTP code */
	ath6kl_dbg(ATH6KL_DBG_BOOT, "executing OTP at 0x%x\n",
		   ar->hw.app_start_override_addr);

#ifdef CONFIG_MACH_PX
	/* SOFTMAC has higher priority than OTP MAC */
	param = 1;
#else
	param = 0;
#endif

	ath6kl_bmi_execute(ar, ar->hw.app_start_override_addr, &param);

	return ret;
}

static int ath6kl_upload_firmware(struct ath6kl *ar)
{
	u32 address;
	int ret;

	if (WARN_ON(ar->fw == NULL))
		return 0;

	address = ar->hw.app_load_addr;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "writing firmware to 0x%x (%zd B)\n",
		   address, ar->fw_len);

	ret = ath6kl_bmi_fast_download(ar, address, ar->fw, ar->fw_len);

	if (ret) {
		ath6kl_err("Failed to write firmware: %d\n", ret);
		return ret;
	}

	/*
	 * Set starting address for firmware
	 * Don't need to setup app_start override addr on AR6004
	 */
	if (ar->target_type != TARGET_TYPE_AR6004) {
		address = ar->hw.app_start_override_addr;
		ath6kl_bmi_set_app_start(ar, address);
	}
	return ret;
}

static int ath6kl_upload_patch(struct ath6kl *ar)
{
	u32 address, param;
	int ret;

	if (ar->fw_patch == NULL)
		return 0;

	address = ar->hw.dataset_patch_addr;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "writing patch to 0x%x (%zd B)\n",
		   address, ar->fw_patch_len);

	ret = ath6kl_bmi_write(ar, address, ar->fw_patch, ar->fw_patch_len);
	if (ret) {
		ath6kl_err("Failed to write patch file: %d\n", ret);
		return ret;
	}

	param = address;
	ath6kl_bmi_write(ar,
			 ath6kl_get_hi_item_addr(ar,
			 HI_ITEM(hi_dset_list_head)),
			 (unsigned char *) &param, 4);

	return 0;
}

static int ath6kl_upload_testscript(struct ath6kl *ar)
{
	u32 address, param;
	int ret;

	if (testmode != 2)
		return 0;

	if (ar->fw_testscript == NULL)
		return 0;

	address = ar->hw.testscript_addr;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "writing testscript to 0x%x (%zd B)\n",
		address, ar->fw_testscript_len);

	ret = ath6kl_bmi_write(ar, address, ar->fw_testscript,
		ar->fw_testscript_len);
	if (ret) {
		ath6kl_err("Failed to write testscript file: %d\n", ret);
		return ret;
	}

	param = address;
	ath6kl_bmi_write(ar,
			ath6kl_get_hi_item_addr(ar,
			HI_ITEM(hi_ota_testscript)),
			(unsigned char *) &param, 4);

	param = 4096;
	ath6kl_bmi_write(ar,
			ath6kl_get_hi_item_addr(ar,
			HI_ITEM(hi_end_ram_reserve_sz)),
			(unsigned char *) &param, 4);

	param = 1;
	ath6kl_bmi_write(ar,
			ath6kl_get_hi_item_addr(ar,
			HI_ITEM(hi_test_apps_related)),
			(unsigned char *) &param, 4);

	return 0;
}

#ifdef GLOBALCONFIG_WLAN_COUNTRY_CODE
#define COUNTRY_MAX 76
struct channels {
	char country[30];
	char ccode[6];
	unsigned short reg_dmn_code;
}
pArray[COUNTRY_MAX] = {
	{"Afghanistan",		"AF 0",	0x406a},
	{"Albania",			"AL 0",	0x8008},
	{"Algeria",			"DZ 0",	0x800c},
	{"Angola",			"AD 0",	0x406a},
	{"Austria",			"AT 0",	0x8028},
	{"Australia",		"AU 0",	0x8024},
	{"Bangladesh",		"BD 0",	0x8032},
	{"Belgium",			"BE 0",	0x8038},
	{"Bosnia",			"BA 0",	0x8046},
	{"Bulgaria",		"BG 0",	0x8064},
	{"Croatia",			"HR 0",	0x80bf},
	{"Greece",			"GR 0",	0x812c},
	{"Czech Republic",	"CZ 0",	0x80cb},
	{"Egypt",			"EG 0",	0x8332},
	{"Nordic",			"FI 0",	0x80f6},
	{"Finland",			"FI 0",	0x80f6},
	{"France",			"FR 0",	0x80FA},
	{"Gabon",			"GA 0",	0x406a},
	{"Germany",			"DE 0",	0x8114},
	{"Ghana",			"GH 0",	0x406a},
	{"Greece",			"GR 0",	0x812c},
	{"Hungary",			"HU 0",	0x815c},
	{"Iceland",			"IS 0",	0x8160},
	{"India",			"IN 0",	0x8164},
	{"Indonesia",		"ID 0",	0x8168},
	{"Iraq",			"IQ 0",	0x406a},
	{"IE",				"IE 0",	0x8174},
	{"Ireland",			"IE 0",	0x8174},
	{"Israel",			"IL 0",	0x8178},
	{"Italy",			"IT 0",	0x817c},
	{"Jamaica",			"JM 0",	0x8184},
	{"Jordan",			"JO 0",	0x8190},
	{"Central Asia",	"KZ 0",	0x818e},
	{"Kazakhstan",		"KZ 0",	0x818e},
	{"Kenya",			"KE 0",	0x8198},
	{"BALTIC",			"LV 0",	0x81ac},
	{"Libya",			"LY 0",	0x406a},
	{"Lithuania",		"LT 0",	0x81b8},
	{"Luxemburg",		"LU 0",	0x81ba},
	{"Macedonia",		"MK 0",	0x8327},
	{"Malaysia",		"MY 0",	0x81ca},
	{"Mongolia",		"MN 0",	0x406a},
	{"Montenegro",		"ME 0",	0x837b},
	{"Morocco",			"MA 0",	0x81f8},
	{"Nepal",			"NP 0",	0x820c},
	{"Netherlands",		"NL 0",	0x8210},
	{"The Netherlands",	"NL 0",	0x8210},
	{"New Zealand",		"NZ 0",	0x822a},
	{"Nigeria",			"NG 0",	0x406a},
	{"Norway",			"NO 0",	0x8242},
	{"Pakistan",		"PK 0",	0x824a},
	{"Philippines",		"PH 0",	0x8260},
	{"Poland",			"PL 0",	0x8268},
	{"Portugal",		"PT 0",	0x826c},
	{"Romania",			"RO 0",	0x8282},
	{"Russia",			"RU 0",	0x8283},
	{"KSA",				"SA 0",	0x82aa},
	{"Senegal",			"SN 0",	0x406a},
	{"Serbia",			"RS 0",	0x8114},
	{"Singapore",		"SG 0",	0x82be},
	{"Slovakia",		"SK 0",	0x82bf},
	{"Slovenia",		"SI 0",	0x82c1},
	{"South Africa",	"ZA 0",	0x82c6},
	{"Spain",			"ES 0",	0x82D4},
	{"Sri Lanka",		"LK 0",	0x8090},
	{"Sweden",			"SE 0",	0x82f0},
	{"Switzerland",		"CH 0",	0x82f4},
	{"Thailand",		"TH 0",	0x82fc},
	{"Tunisia",			"TN 0",	0x8314},
	{"Turkey",			"TR 0",	0x8318},
	{"Ukraine",			"UA 0",	0x8324},
	{"UK",				"GB 0",	0x833A},
	{"UK &IRE",			"GB 0",	0x833A},
	{"United Kingdom",	"GB 0",	0x833A},
	{"Uzbekistan",		"UZ 0",	0x835c},
	{"Vietnam",			"VN 0",	0x82c0}
};

static unsigned short ath6kl_get_reg_dmn_code(u8 *ccode)
{
	int i = 0;

	for (i = 0; i < COUNTRY_MAX; i++) {
		if (strncmp(ccode, pArray[i].ccode, 2) == 0) {
			ath6kl_dbg(ATH6KL_DBG_BOOT, "%s: %s %s 0x%x",
				__func__, pArray[i].country, pArray[i].ccode,
				pArray[i].reg_dmn_code);
			return pArray[i].reg_dmn_code;
		}
	}

	ath6kl_err("%s() Failed to find reg_domain code for %s\n",
		__func__, ccode);

	return 0xffff;
}

static void ath6kl_update_ccodeinfo(struct ath6kl *ar)
{
	char ccode_filename[32];

	do {
		int ret = 0;
		size_t length;
		u8 *pdata = NULL;

		snprintf(ccode_filename, sizeof(ccode_filename),
			"/data/.ccode.info");

		ret = android_readwrite_file(ccode_filename, NULL, NULL, 0);

		if (ret < 0)
			break;
		else
			length = ret;

		pdata = vmalloc(length + 1);

		if (!pdata) {
			ath6kl_dbg(ATH6KL_DBG_BOOT,
				"%s: Cannot allocate buffer for ccode_info (%d)\n",
				__func__, length);
			break;
		}

		if (android_readwrite_file(ccode_filename,
				(char *)pdata, NULL, length) != length) {
			ath6kl_dbg(ATH6KL_DBG_BOOT,
				"%s: file read error, length %d\n",
				__func__, length);
			vfree(pdata);
			break;
		}
		pdata[length] = '\0';

		if (reg_domain == 0xffff)
			reg_domain = ath6kl_get_reg_dmn_code(pdata);


		vfree(pdata);
	} while (0);
}
#endif

static void ath6kl_update_psminfo(struct ath6kl *ar)
{
	char psm_filename[32];
	ar->psminfo = 1;

	do {
		int ret = 0;
		size_t length;
		u8 *pdata = NULL;

		snprintf(psm_filename, sizeof(psm_filename), "/data/.psm.info");

		ret = android_readwrite_file(psm_filename, NULL, NULL, 0);

		if (ret < 0)
			break;
		else
			length = ret;

		pdata = vmalloc(length);

		if (!pdata) {
			ath6kl_dbg(ATH6KL_DBG_BOOT,
				"%s: Cannot allocate buffer for psm_info (%d)\n",
				__func__, length);
			break;
		}

		if (android_readwrite_file(psm_filename,
				(char *)pdata, NULL, length) != length) {
			ath6kl_dbg(ATH6KL_DBG_BOOT,
				"%s: file read error, length %d\n",
				__func__, length);
			vfree(pdata);
			break;
		}

		ar->psminfo = *pdata - '0';
		ath6kl_dbg(ATH6KL_DBG_BOOT,"%s: psm_info is %d\n", __FUNCTION__, ar->psminfo);
		vfree(pdata);
	} while (0);
}

static int ath6kl_init_upload(struct ath6kl *ar)
{
	u32 param, options, sleep, address;
	int status = 0;

	if (ar->target_type != TARGET_TYPE_AR6003 &&
		ar->target_type != TARGET_TYPE_AR6004)
		return -EINVAL;

	/* temporarily disable system sleep */
	address = MBOX_BASE_ADDRESS + LOCAL_SCRATCH_ADDRESS;
	status = ath6kl_bmi_reg_read(ar, address, &param);
	if (status)
		return status;

	options = param;

	param |= ATH6KL_OPTION_SLEEP_DISABLE;
	status = ath6kl_bmi_reg_write(ar, address, param);
	if (status)
		return status;

	address = RTC_BASE_ADDRESS + SYSTEM_SLEEP_ADDRESS;
	status = ath6kl_bmi_reg_read(ar, address, &param);
	if (status)
		return status;

	sleep = param;

	param |= SM(SYSTEM_SLEEP_DISABLE, 1);
	status = ath6kl_bmi_reg_write(ar, address, param);
	if (status)
		return status;

	ath6kl_dbg(ATH6KL_DBG_TRC, "old options: %d, old sleep: %d\n",
		   options, sleep);

	/* program analog PLL register */
	/* no need to control 40/44MHz clock on AR6004 */
	if (ar->target_type != TARGET_TYPE_AR6004) {
		status = ath6kl_bmi_reg_write(ar, ATH6KL_ANALOG_PLL_REGISTER,
					      0xF9104001);

		if (status)
			return status;

		/* Run at 80/88MHz by default */
		param = SM(CPU_CLOCK_STANDARD, 1);

		address = RTC_BASE_ADDRESS + CPU_CLOCK_ADDRESS;
		status = ath6kl_bmi_reg_write(ar, address, param);
		if (status)
			return status;
	}

	param = 0;
	address = RTC_BASE_ADDRESS + LPO_CAL_ADDRESS;
	param = SM(LPO_CAL_ENABLE, 1);
	status = ath6kl_bmi_reg_write(ar, address, param);
	if (status)
		return status;

	/* WAR to avoid SDIO CRC err */
	if (ar->version.target_ver == AR6003_HW_2_0_VERSION ||
	    ar->version.target_ver == AR6003_HW_2_1_1_VERSION) {
		ath6kl_err("temporary war to avoid sdio crc error\n");

		param = 0x20;

		address = GPIO_BASE_ADDRESS + GPIO_PIN10_ADDRESS;
		status = ath6kl_bmi_reg_write(ar, address, param);
		if (status)
			return status;

		address = GPIO_BASE_ADDRESS + GPIO_PIN11_ADDRESS;
		status = ath6kl_bmi_reg_write(ar, address, param);
		if (status)
			return status;

		address = GPIO_BASE_ADDRESS + GPIO_PIN12_ADDRESS;
		status = ath6kl_bmi_reg_write(ar, address, param);
		if (status)
			return status;

		address = GPIO_BASE_ADDRESS + GPIO_PIN13_ADDRESS;
		status = ath6kl_bmi_reg_write(ar, address, param);
		if (status)
			return status;
	}

	ath6kl_bmi_init(ar);
	ath6kl_bmi_reg_write(ar, 0x540678, ar6k_clock);

	/* write EEPROM data to Target RAM */
	status = ath6kl_upload_board_file(ar);
	if (status)
		return status;

	/* transfer One time Programmable data */
	status = ath6kl_upload_otp(ar);
	if (status)
		return status;

	/* Download Target firmware */
	status = ath6kl_upload_firmware(ar);
	if (status)
		return status;

	status = ath6kl_upload_patch(ar);
	if (status)
		return status;

	/* Download the test script */
	status = ath6kl_upload_testscript(ar);
	if (status)
		return status;

	/* Restore system sleep */
	address = RTC_BASE_ADDRESS + SYSTEM_SLEEP_ADDRESS;
	status = ath6kl_bmi_reg_write(ar, address, sleep);
	if (status)
		return status;

	address = MBOX_BASE_ADDRESS + LOCAL_SCRATCH_ADDRESS;

	if (en_ani)
		param = options & ~0x20;
	else
		param = options | 0x20;

	status = ath6kl_bmi_reg_write(ar, address, param);
	if (status)
		return status;

	return status;
}

static int ath6kl_init_hw_params(struct ath6kl *ar)
{
	const struct ath6kl_hw *hw;
	int i;

	for (i = 0; i < ARRAY_SIZE(hw_list); i++) {
		hw = &hw_list[i];

		if (hw->id == ar->version.target_ver)
			break;
	}

	if (i == ARRAY_SIZE(hw_list)) {
		ath6kl_err("Unsupported hardware version: 0x%x\n",
			   ar->version.target_ver);
		return -EINVAL;
	}

	ar->hw = *hw;

	ath6kl_dbg(ATH6KL_DBG_BOOT,
		   "target_ver 0x%x target_type 0x%x dataset_patch 0x%x app_load_addr 0x%x\n",
		   ar->version.target_ver, ar->target_type,
		   ar->hw.dataset_patch_addr, ar->hw.app_load_addr);
	ath6kl_dbg(ATH6KL_DBG_BOOT,
		   "app_start_override_addr 0x%x board_ext_data_addr 0x%x reserved_ram_size 0x%x",
		   ar->hw.app_start_override_addr, ar->hw.board_ext_data_addr,
		   ar->hw.reserved_ram_size);
	ath6kl_dbg(ATH6KL_DBG_BOOT,
		   "refclk_hz %d uarttx_pin %d",
		   ar->hw.refclk_hz, ar->hw.uarttx_pin);

	return 0;
}

static const char *ath6kl_init_get_hif_name(enum ath6kl_hif_type type)
{
	switch (type) {
	case ATH6KL_HIF_TYPE_SDIO:
		return "sdio";
	case ATH6KL_HIF_TYPE_USB:
		return "usb";
	}

	return NULL;
}

int ath6kl_init_hw_start(struct ath6kl *ar)
{
	long timeleft;
	int ret, i;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "hw start\n");

	ret = ath6kl_hif_power_on(ar);
	if (ret)
		return ret;

	ret = ath6kl_configure_target(ar);
	if (ret)
		goto err_power_off;

	ret = ath6kl_init_upload(ar);
	if (ret)
		goto err_power_off;

	if (reg_domain != 0xffff) {
		ret = ath6kl_set_reg_dmn(ar);
		if (ret)
			goto err_power_off;
	}

	/* Do we need to finish the BMI phase */
	/* FIXME: return error from ath6kl_bmi_done() */
	if (ath6kl_bmi_done(ar)) {
		ret = -EIO;
		goto err_power_off;
	}

	/*
	 * The reason we have to wait for the target here is that the
	 * driver layer has to init BMI in order to set the host block
	 * size.
	 */
	if (ath6kl_htc_wait_target(ar->htc_target)) {
		ret = -EIO;
		goto err_power_off;
	}

	if (ath6kl_init_service_ep(ar)) {
		ret = -EIO;
		goto err_cleanup_scatter;
	}

	/* setup credit distribution */
	ath6kl_credit_setup(ar->htc_target, &ar->credit_state_info);

	/* start HTC */
	ret = ath6kl_htc_start(ar->htc_target);
	if (ret) {
		/* FIXME: call this */
		ath6kl_cookie_cleanup(ar);
		goto err_cleanup_scatter;
	}

	/* Wait for Wmi event to be ready */
	timeleft = wait_event_interruptible_timeout(ar->event_wq,
						    test_bit(WMI_READY,
							     &ar->flag),
						    WMI_TIMEOUT);

	ath6kl_dbg(ATH6KL_DBG_BOOT, "firmware booted\n");


	if (test_and_clear_bit(FIRST_BOOT, &ar->flag)) {
		ath6kl_info("%s %s fw %s api %d%s\n",
			    ar->hw.name,
			    ath6kl_init_get_hif_name(ar->hif_type),
			    ar->wiphy->fw_version,
			    ar->fw_api,
			    test_bit(TESTMODE, &ar->flag) ? " testmode" : "");
	}

	if (ar->version.abi_ver != ATH6KL_ABI_VERSION) {
		ath6kl_err("abi version mismatch: host(0x%x), target(0x%x)\n",
			   ATH6KL_ABI_VERSION, ar->version.abi_ver);
		ret = -EIO;
		goto err_htc_stop;
	}

	if (!timeleft || signal_pending(current)) {
		ath6kl_err("wmi is not ready or wait was interrupted\n");
		ret = -EIO;
		goto err_htc_stop;
	}

	ath6kl_dbg(ATH6KL_DBG_TRC, "%s: wmi is ready\n", __func__);

	/* communicate the wmi protocol verision to the target */
	/* FIXME: return error */
	if ((ath6kl_set_host_app_area(ar)) != 0)
		ath6kl_err("unable to set the host app area\n");

	for (i = 0; i < ar->vif_max; i++) {
		ret = ath6kl_target_config_wlan_params(ar, i);
		if (ret)
			goto err_htc_stop;
	}

	ar->state = ATH6KL_STATE_ON;
#ifdef CONFIG_MACH_PX
	wake_up(&ar->event_wq);
#endif

	return 0;

err_htc_stop:
	ath6kl_htc_stop(ar->htc_target);
err_cleanup_scatter:
	ath6kl_hif_cleanup_scatter(ar);
err_power_off:
	ath6kl_hif_power_off(ar);

	return ret;
}

int ath6kl_init_hw_stop(struct ath6kl *ar)
{
	int ret;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "hw stop\n");

	ath6kl_htc_stop(ar->htc_target);

	ath6kl_hif_stop(ar);

	ath6kl_bmi_reset(ar);

	ret = ath6kl_hif_power_off(ar);
	if (ret)
		ath6kl_warn("failed to power off hif: %d\n", ret);

	ar->state = ATH6KL_STATE_OFF;

	return 0;
}

int ath6kl_core_init(struct ath6kl *ar)
{
	struct ath6kl_bmi_target_info targ_info;
	struct net_device *ndev;
	int ret = 0, i;

	ar->ath6kl_wq = create_singlethread_workqueue("ath6kl");
	if (!ar->ath6kl_wq)
		return -ENOMEM;

	ret = ath6kl_bmi_init(ar);
	if (ret)
		goto err_wq;

	/*
	 * Turn on power to get hardware (target) version and leave power
	 * on delibrately as we will boot the hardware anyway within few
	 * seconds.
	 */
	ret = ath6kl_hif_power_on(ar);
	if (ret)
		goto err_bmi_cleanup;

	ret = ath6kl_bmi_get_target_info(ar, &targ_info);
	if (ret)
		goto err_power_off;

	ar->version.target_ver = le32_to_cpu(targ_info.version);
	ar->target_type = le32_to_cpu(targ_info.type);
	ar->wiphy->hw_version = le32_to_cpu(targ_info.version);

	ret = ath6kl_init_hw_params(ar);
	if (ret)
		goto err_power_off;

	ar->htc_target = ath6kl_htc_create(ar);

	if (!ar->htc_target) {
		ret = -ENOMEM;
		goto err_power_off;
	}

	ret = ath6kl_fetch_firmwares(ar);
	if (ret)
		goto err_htc_cleanup;

	ath6kl_mangle_mac_address(ar);
	ath6kl_update_psminfo(ar);
#ifdef GLOBALCONFIG_WLAN_COUNTRY_CODE
	ath6kl_update_ccodeinfo(ar);
#endif

	/* FIXME: we should free all firmwares in the error cases below */

	/* Indicate that WMI is enabled (although not ready yet) */
	set_bit(WMI_ENABLED, &ar->flag);
	ar->wmi = ath6kl_wmi_init(ar);
	if (!ar->wmi) {
		ath6kl_err("failed to initialize wmi\n");
		ret = -EIO;
		goto err_htc_cleanup;
	}

	ath6kl_dbg(ATH6KL_DBG_TRC, "%s: got wmi @ 0x%p.\n", __func__, ar->wmi);

	ret = ath6kl_register_ieee80211_hw(ar);
	if (ret)
		goto err_node_cleanup;

	ret = ath6kl_debug_init(ar);
	if (ret) {
		wiphy_unregister(ar->wiphy);
		goto err_node_cleanup;
	}

	for (i = 0; i < ar->vif_max; i++)
		ar->avail_idx_map |= BIT(i);

	rtnl_lock();

	/* Add an initial station interface */
	ndev = ath6kl_interface_add(ar, "wlan%d", NL80211_IFTYPE_STATION, 0,
				    INFRA_NETWORK);

	rtnl_unlock();

	if (!ndev) {
		ath6kl_err("Failed to instantiate a network device\n");
		ret = -ENOMEM;
		wiphy_unregister(ar->wiphy);
		goto err_debug_init;
	}


	ath6kl_dbg(ATH6KL_DBG_TRC, "%s: name=%s dev=0x%p, ar=0x%p\n",
			__func__, ndev->name, ndev, ar);

	/* setup access class priority mappings */
	ar->ac_stream_pri_map[WMM_AC_BK] = 0; /* lowest  */
	ar->ac_stream_pri_map[WMM_AC_BE] = 1;
	ar->ac_stream_pri_map[WMM_AC_VI] = 2;
	ar->ac_stream_pri_map[WMM_AC_VO] = 3; /* highest */

	/* allocate some buffers that handle larger AMSDU frames */
	ath6kl_refill_amsdu_rxbufs(ar, ATH6KL_MAX_AMSDU_RX_BUFFERS);

	ath6kl_cookie_init(ar);

	ar->conf_flags = ATH6KL_CONF_IGNORE_ERP_BARKER |
			 ATH6KL_CONF_ENABLE_11N | ATH6KL_CONF_ENABLE_TX_BURST;

	if (suspend_mode &&
	     suspend_mode >= WLAN_POWER_STATE_CUT_PWR &&
	     suspend_mode <= WLAN_POWER_STATE_WOW)
		ar->suspend_mode = suspend_mode;
	else
		ar->suspend_mode = 0;

	if (suspend_mode == WLAN_POWER_STATE_WOW &&
	    (wow_mode == WLAN_POWER_STATE_CUT_PWR ||
	     wow_mode == WLAN_POWER_STATE_DEEP_SLEEP))
		ar->wow_suspend_mode = wow_mode;
	else
		ar->wow_suspend_mode = 0;

	ar->wiphy->flags |= WIPHY_FLAG_SUPPORTS_FW_ROAM |
			    WIPHY_FLAG_HAVE_AP_SME |
			    WIPHY_FLAG_AP_PROBE_RESP_OFFLOAD;

#ifdef CONFIG_MACH_PX
#else
	if (test_bit(ATH6KL_FW_CAPABILITY_SCHED_SCAN, ar->fw_capabilities))
		ar->wiphy->flags |= WIPHY_FLAG_SUPPORTS_SCHED_SCAN;
#endif

	ar->wiphy->probe_resp_offload =
		NL80211_PROBE_RESP_OFFLOAD_SUPPORT_WPS |
		NL80211_PROBE_RESP_OFFLOAD_SUPPORT_WPS2 |
		NL80211_PROBE_RESP_OFFLOAD_SUPPORT_P2P |
		NL80211_PROBE_RESP_OFFLOAD_SUPPORT_80211U;

	set_bit(FIRST_BOOT, &ar->flag);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39))
	ndev->hw_features |= NETIF_F_IP_CSUM | NETIF_F_RXCSUM;
#endif

	ret = ath6kl_init_hw_start(ar);
	if (ret) {
		ath6kl_err("Failed to start hardware: %d\n", ret);
		goto err_rxbuf_cleanup;
	}

	/* give our connected endpoints some buffers */
	ath6kl_rx_refill(ar->htc_target, ar->ctrl_ep);
	ath6kl_rx_refill(ar->htc_target, ar->ac2ep_map[WMM_AC_BE]);

	/*
	 * Set mac address which is received in ready event
	 * FIXME: Move to ath6kl_interface_add()
	 */
	memcpy(ndev->dev_addr, ar->mac_addr, ETH_ALEN);

	return ret;

err_rxbuf_cleanup:
	ath6kl_htc_flush_rx_buf(ar->htc_target);
	ath6kl_cleanup_amsdu_rxbufs(ar);
	rtnl_lock();
	ath6kl_deinit_if_data(netdev_priv(ndev));
	rtnl_unlock();
	wiphy_unregister(ar->wiphy);
err_debug_init:
	ath6kl_debug_cleanup(ar);
err_node_cleanup:
	ath6kl_cleanup_android_resource(ar);
	ath6kl_wmi_shutdown(ar->wmi);
	clear_bit(WMI_ENABLED, &ar->flag);
	ar->wmi = NULL;
err_htc_cleanup:
	ath6kl_htc_cleanup(ar->htc_target);
err_power_off:
	ath6kl_hif_power_off(ar);
err_bmi_cleanup:
	ath6kl_bmi_cleanup(ar);
err_wq:
	destroy_workqueue(ar->ath6kl_wq);

	return ret;
}

void ath6kl_cleanup_vif(struct ath6kl_vif *vif, bool wmi_ready)
{
	static u8 bcast_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	bool discon_issued;

	netif_stop_queue(vif->ndev);

	clear_bit(WLAN_ENABLED, &vif->flags);

	if (wmi_ready) {
		discon_issued = test_bit(CONNECTED, &vif->flags) ||
				test_bit(CONNECT_PEND, &vif->flags);
		ath6kl_disconnect(vif);
		del_timer(&vif->disconnect_timer);

		if (discon_issued)
			ath6kl_disconnect_event(vif, DISCONNECT_CMD,
						(vif->nw_type & AP_NETWORK) ?
						bcast_mac : vif->bssid,
						0, NULL, 0);
	}

	if (vif->scan_req) {
		cfg80211_scan_done(vif->scan_req, true);
		vif->scan_req = NULL;
	}
}

void ath6kl_stop_txrx(struct ath6kl *ar)
{
	struct ath6kl_vif *vif, *tmp_vif;
	int i;

	set_bit(DESTROY_IN_PROGRESS, &ar->flag);

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("down_interruptible failed\n");
		return;
	}

	for (i = 0; i < AP_MAX_NUM_STA; i++)
		aggr_reset_state(ar->sta_list[i].aggr_conn);

	spin_lock_bh(&ar->list_lock);
	list_for_each_entry_safe(vif, tmp_vif, &ar->vif_list, list) {
		list_del(&vif->list);
		spin_unlock_bh(&ar->list_lock);
		ath6kl_cleanup_vif(vif, test_bit(WMI_READY, &ar->flag));
		rtnl_lock();
		ath6kl_deinit_if_data(vif);
		rtnl_unlock();
		spin_lock_bh(&ar->list_lock);
	}
	spin_unlock_bh(&ar->list_lock);

	clear_bit(WMI_READY, &ar->flag);

	/*
	 * After wmi_shudown all WMI events will be dropped. We
	 * need to cleanup the buffers allocated in AP mode and
	 * give disconnect notification to stack, which usually
	 * happens in the disconnect_event. Simulate the disconnect
	 * event by calling the function directly. Sometimes
	 * disconnect_event will be received when the debug logs
	 * are collected.
	 */
	ath6kl_wmi_shutdown(ar->wmi);

	clear_bit(WMI_ENABLED, &ar->flag);
	if (ar->htc_target) {
		ath6kl_dbg(ATH6KL_DBG_TRC, "%s: shut down htc\n", __func__);
		ath6kl_htc_stop(ar->htc_target);
	}

	/*
	 * Try to reset the device if we can. The driver may have been
	 * configure NOT to reset the target during a debug session.
	 */
	ath6kl_dbg(ATH6KL_DBG_TRC,
			"attempting to reset target on instance destroy\n");
	ath6kl_reset_device(ar, ar->target_type, true, true);

	clear_bit(WLAN_ENABLED, &ar->flag);
}
