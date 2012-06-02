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

#ifdef CONFIG_ATH6KL_DEBUG

#include <linux/skbuff.h>
#include <linux/export.h>
#include "core.h"
#include "wmi.h"
#include "debug.h"
#include "debugfs_pri.h"

struct wmi_set_inact_period_cmd {
	__le32 inact_period;
	u8 num_null_func;
} __packed;

static inline struct sk_buff *ath6kl_wmi_get_new_buf_pri(u32 size)
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

static int ath6kl_wmi_inact_period_cmd(struct wmi *wmi, u32 inact_period,
				       u8 num_null_func)
{
	struct sk_buff *skb;
	struct wmi_set_inact_period_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf_pri(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_inact_period_cmd *) skb->data;
	cmd->inact_period = cpu_to_le32(inact_period);
	cmd->num_null_func = num_null_func;

	ret = ath6kl_wmi_cmd_send(wmi, 0, skb, WMI_AP_CONN_INACT_CMDID,
				  NO_SYNC_WMIFLAG);

	return ret;
}

static int ath6kl_wmi_set_err_report_bitmask(struct wmi *wmi, u8 if_idx,
					     u32 mask)
{
	struct sk_buff *skb;
	struct wmi_tgt_err_report_mask *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf_pri(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_tgt_err_report_mask *) skb->data;
	cmd->mask = cpu_to_le32(mask);
	ret = ath6kl_wmi_cmd_send(wmi, if_idx, skb,
				  WMI_TARGET_ERROR_REPORT_BITMASK_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_error_report_event(struct wmi *wmi, u8 *data, int len)
{
	struct wmi_tgt_err_report_evt *report;

	if (len < sizeof(*report))
		return -EINVAL;

	report = (struct wmi_tgt_err_report_evt *) data;
	ath6kl_dbg(ATH6KL_DBG_WMI, "Reason for error report: 0x%x\n",
		   report->err_val);

	return 0;
}

static int ath6kl_debugfs_open_pri(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t ath6kl_inact_period_write(struct file *file,
					 const char __user *user_buf,
					 size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;
	char buf[32];
	u32 inact_period;
	size_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &inact_period))
		return -EINVAL;

	ret = ath6kl_wmi_inact_period_cmd(ar->wmi, inact_period, 0);

	if (ret)
		return ret;

	return count;
}

static const struct file_operations fops_inact_period = {
	.write = ath6kl_inact_period_write,
	.open = ath6kl_debugfs_open_pri,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_bmisstime_write(struct file *file,
				      const char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u16 bmiss_time;
	char buf[32];
	ssize_t len;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou16(buf, 0, &bmiss_time))
		return -EINVAL;

	vif->bmiss_time_t = bmiss_time;

	/* Enable error report event for bmiss */
	ath6kl_wmi_set_err_report_bitmask(ar->wmi, vif->fw_vif_idx,
					  ATH6KL_ERR_REPORT_BMISS_MASK);

	ath6kl_wmi_bmisstime_cmd(ar->wmi, vif->fw_vif_idx,
				 vif->bmiss_time_t, 0);
	return count;
}

static ssize_t ath6kl_bmisstime_read(struct file *file,
				     char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	char buf[32];
	int len;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	len = scnprintf(buf, sizeof(buf), "%u\n", vif->bmiss_time_t);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_bmisstime = {
	.read = ath6kl_bmisstime_read,
	.write = ath6kl_bmisstime_write,
	.open = ath6kl_debugfs_open_pri,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_scan_ctrl_flag_write(struct file *file,
					   const char __user *user_buf,
					   size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u8 ctrl_flag;
	int ret;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	ret = kstrtou8_from_user(user_buf, count, 0, &ctrl_flag);
	if (ret)
		return ret;

	if (ctrl_flag > ATH6KL_MAX_SCAN_CTRL_FLAGS)
		ctrl_flag = ATH6KL_DEFAULT_SCAN_CTRL_FLAGS;

	vif->scan_ctrl_flag = ctrl_flag;
	ath6kl_wmi_scanparams_cmd(ar->wmi, 0, 0, 0, vif->bg_scan_period, 0, 0,
				  0, 3, ctrl_flag, 0, 0);

	return count;
}

static const struct file_operations fops_scanctrl_flag = {
	.write = ath6kl_scan_ctrl_flag_write,
	.open = ath6kl_debugfs_open_pri,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

int ath6kl_init_debugfs_pri(struct ath6kl *ar)
{
	debugfs_create_file("inactivity_period", S_IWUSR, ar->debugfs_phy, ar,
			    &fops_inact_period);

	debugfs_create_file("bmiss_time", S_IRUSR | S_IWUSR, ar->debugfs_phy,
			    ar, &fops_bmisstime);

	debugfs_create_file("scan_ctrl_flag", S_IRUSR, ar->debugfs_phy, ar,
			    &fops_scanctrl_flag);

	return 0;
}

#endif
