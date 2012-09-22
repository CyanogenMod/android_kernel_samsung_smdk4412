/*
 * Copyright (c) 2011 Atheros Communications Inc.
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
#include <linux/vmalloc.h>
#include <linux/random.h>
#define MAC_FILE "ath6k/AR6003/hw2.1.1/softmac"

typedef char            A_CHAR;
extern int android_readwrite_file(const A_CHAR *filename, A_CHAR *rbuf, const A_CHAR *wbuf, size_t length);

/* Bleh, same offsets. */
#define AR6003_MAC_ADDRESS_OFFSET 0x16
#define AR6004_MAC_ADDRESS_OFFSET 0x16

/* Global variables, sane coding be damned. */
u8 *ath6kl_softmac;
size_t ath6kl_softmac_len;

static void ath6kl_calculate_crc(u32 target_type, u8 *data, size_t len)
{
	u16 *crc, *data_idx;
	u16 checksum;
	int i;

	if (target_type == TARGET_TYPE_AR6003) {
		crc = (u16 *)(data + 0x04);
	} else if (target_type == TARGET_TYPE_AR6004) {
		len = 1024;
		crc = (u16 *)(data + 0x04);
	} else {
		ath6kl_err("Invalid target type\n");
		return;
	}

	ath6kl_dbg(ATH6KL_DBG_BOOT, "Old Checksum: %u\n", *crc);

	*crc = 0;
	checksum = 0;
	data_idx = (u16 *)data;

	for (i = 0; i < len; i += 2) {
		checksum = checksum ^ (*data_idx);
		data_idx++;
	}

	*crc = cpu_to_le16(checksum);

	ath6kl_dbg(ATH6KL_DBG_BOOT, "New Checksum: %u\n", checksum);
}

#ifdef CONFIG_MACH_PX
static int ath6kl_fetch_nvmac_info(struct ath6kl *ar)
{
	char softmac_temp[64];
	int ret = 0, ath6kl_softmac_len = 0;
	int isnvmac_imei = 0, isnvmac_wifi = 0;
	int isnvmac_file = 0, ismac_file = 0;

	char nvfilepath[32] = {0};
	char *nvfilepath_imei = "/efs/imei/.nvmac.info";
	char *nvfilepath_wifi = "/efs/wifi/.nvmac.info";
	char *softmac_filename = "/efs/wifi/.mac.info";
	char *softmac_old_filename = "/data/.mac.info";

	do {
		/*
		isnvmac_imei = android_readwrite_file(nvfilepath_imei,
								NULL, NULL, 0);
		isnvmac_wifi = android_readwrite_file(nvfilepath_wifi,
								NULL, NULL, 0); */
		ismac_file = android_readwrite_file(softmac_filename,
								NULL, NULL, 0);

		/*
		if (isnvmac_imei >= 16 && isnvmac_wifi >= 16) {
			strcpy(nvfilepath, nvfilepath_wifi);
			isnvmac_file = isnvmac_wifi;
		} else if (isnvmac_wifi >= 16) {
			strcpy(nvfilepath, nvfilepath_wifi);
			isnvmac_file = isnvmac_wifi;
		} else if (isnvmac_imei >= 16) {
			strcpy(nvfilepath, nvfilepath_imei);
			isnvmac_file = isnvmac_imei;
		} */

		/* copy .nvmac.info file to .mac.info
		   wifi driver will use .mac.info finally */
		/*
		if (isnvmac_file >= 16) {
			ret = android_readwrite_file(nvfilepath,
				(char *)softmac_temp, NULL, isnvmac_file);
			ath6kl_dbg(ATH6KL_DBG_BOOT,
				"%s: Read Mac Address on nvmac.info - %d\n",
				__func__, ret);
			ret = android_readwrite_file(softmac_filename,
				NULL, (char *)softmac_temp, ret);
			ath6kl_dbg(ATH6KL_DBG_BOOT,
				"%s: Write Mac Address on mac.info - %d\n",
				__func__, ret);
			ret = android_readwrite_file(softmac_old_filename,
				NULL, (char *)softmac_temp, ret);
		} */

		//if (isnvmac_file < 16 && ismac_file < 16) {
		if (ismac_file < 16) {
			snprintf(softmac_temp, sizeof(softmac_temp),
				"00:12:34:%02x:%02x:%02x",
				random32() & 0xff,
				random32() & 0xff,
				random32() & 0xff);

			ret = android_readwrite_file(softmac_filename, NULL,
				(char *)softmac_temp, strlen(softmac_temp));
			ath6kl_dbg(ATH6KL_DBG_BOOT,
				"%s: Write Random Mac on mac.info - %d\n",
				__func__, ret);
			ret = android_readwrite_file(softmac_old_filename, NULL,
				(char *)softmac_temp, strlen(softmac_temp));
		}


		ret = android_readwrite_file(softmac_filename, NULL, NULL, 0);
		if (ret < 0)
			break;
		else
			ath6kl_softmac_len = ret;

		ath6kl_softmac = vmalloc(ath6kl_softmac_len);
		if (!ath6kl_softmac) {
			ath6kl_dbg(ATH6KL_DBG_BOOT,
				"%s: Cannot allocate buffer for %s (%d)\n",
				__func__, softmac_filename, ath6kl_softmac_len);
			ret = -ENOMEM;
			break;
		}

		ret = android_readwrite_file(softmac_filename,
					(char *)ath6kl_softmac,
					NULL, ath6kl_softmac_len);

		if (ret != ath6kl_softmac_len) {
			ath6kl_dbg(ATH6KL_DBG_BOOT,
					"%s: file read error, length %d\n",
					__func__, ath6kl_softmac_len);
			vfree(ath6kl_softmac);
			ret = -1;
			break;
		}

		if (!strncmp(ath6kl_softmac, "00:00:00:00:00:00", 17)) {
			ath6kl_dbg(ATH6KL_DBG_BOOT, "%s: mac address is zero\n",
					 __func__);
			vfree(ath6kl_softmac);
			ret = -1;
			break;
		}

		ret = 0;
	} while (0);

	return ret;
}

#else
static int ath6kl_fetch_mac_file(struct ath6kl *ar)
{
	const struct firmware *fw_entry;
	int ret = 0;


	ret = request_firmware(&fw_entry, MAC_FILE, ar->dev);
	if (ret)
		return ret;

	ath6kl_softmac_len = fw_entry->size;
	ath6kl_softmac = kmemdup(fw_entry->data, fw_entry->size, GFP_KERNEL);

	if (ath6kl_softmac == NULL)
		ret = -ENOMEM;

	release_firmware(fw_entry);

	return ret;
}
#endif

void ath6kl_mangle_mac_address(struct ath6kl *ar)
{
	u8 *ptr_mac;
	int i, ret;
#ifdef CONFIG_MACH_PX
	unsigned int softmac[6];
#endif

	switch (ar->target_type) {
	case TARGET_TYPE_AR6003:
		ptr_mac = ar->fw_board + AR6003_MAC_ADDRESS_OFFSET;
		break;
	case TARGET_TYPE_AR6004:
		ptr_mac = ar->fw_board + AR6004_MAC_ADDRESS_OFFSET;
		break;
	default:
		ath6kl_err("Invalid Target Type\n");
		return;
	}

#if 0
	ath6kl_dbg(ATH6KL_DBG_BOOT,
		   "MAC from EEPROM %02X:%02X:%02X:%02X:%02X:%02X\n",
		   ptr_mac[0], ptr_mac[1], ptr_mac[2],
		   ptr_mac[3], ptr_mac[4], ptr_mac[5]);
#endif

#ifdef CONFIG_MACH_PX
	ret = ath6kl_fetch_nvmac_info(ar);

	if (ret) {
		ath6kl_err("MAC address file not found\n");
		return;
	}

	if (sscanf(ath6kl_softmac, "%02x:%02x:%02x:%02x:%02x:%02x",
			   &softmac[0], &softmac[1], &softmac[2],
			   &softmac[3], &softmac[4], &softmac[5])==6) {

		for (i=0; i<6; ++i) {
			ptr_mac[i] = softmac[i] & 0xff;
		}
	}

	ath6kl_dbg(ATH6KL_DBG_BOOT,
		   "MAC from SoftMAC %02X_%02X:%02X\n",
		   ptr_mac[0], ptr_mac[4], ptr_mac[5]);
	vfree(ath6kl_softmac);
#else
	ret = ath6kl_fetch_mac_file(ar);
	if (ret) {
		ath6kl_err("MAC address file not found\n");
		return;
	}

	for (i = 0; i < ETH_ALEN; ++i) {
	   ptr_mac[i] = ath6kl_softmac[i] & 0xff;
	}

	kfree(ath6kl_softmac);
#endif

	ath6kl_calculate_crc(ar->target_type, ar->fw_board, ar->fw_board_len);
}
