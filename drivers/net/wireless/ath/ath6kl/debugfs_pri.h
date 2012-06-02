/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
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

#ifndef DEBUG_PRI_H
#define DEBUG_PRI_H

#ifdef CONFIG_ATH6KL_DEBUG

#define ATH6KL_ERR_REPORT_BMISS_MASK  BIT(3)

struct wmi_tgt_err_report_mask {
	__le32 mask;
};

struct wmi_tgt_err_report_evt {
	__le32 err_val;
} __packed;

#define ATH6KL_DEFAULT_SCAN_CTRL_FLAGS    (CONNECT_SCAN_CTRL_FLAGS   |  \
					   SCAN_CONNECTED_CTRL_FLAGS |  \
					   ACTIVE_SCAN_CTRL_FLAGS    |  \
					   ROAM_SCAN_CTRL_FLAGS      |  \
					   ENABLE_AUTO_CTRL_FLAGS)

#define ATH6KL_MAX_SCAN_CTRL_FLAGS	   0x7F

int ath6kl_wmi_error_report_event(struct wmi *wmi, u8 *data, int len);
int ath6kl_init_debugfs_pri(struct ath6kl *ar);
#endif
#endif
