/*
 *  This file contains work-arounds for many known SD/MMC
 *  and SDIO hardware bugs.
 *
 *  Copyright (c) 2011 Andrei Warkentin <andreiw@motorola.com>
 *  Copyright (c) 2011 Pierre Tardy <tardyp@gmail.com>
 *  Inspired from pci fixup code:
 *  Copyright (c) 1999 Martin Mares <mj@ucw.cz>
 *
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>

#include "mmc_ops.h"
#include "ram.h"

#ifndef SDIO_VENDOR_ID_TI
#define SDIO_VENDOR_ID_TI		0x0097
#endif

#ifndef SDIO_DEVICE_ID_TI_WL1271
#define SDIO_DEVICE_ID_TI_WL1271	0x4076
#endif

#ifndef SDIO_VENDOR_ID_BRCM
#define SDIO_VENDOR_ID_BRCM		0x02D0
#endif

#ifndef SDIO_DEVICE_ID_BRCM_BCM4330
#define SDIO_DEVICE_ID_BRCM_BCM4330	0x4330
#endif

#ifndef SDIO_DEVICE_ID_BRCM_BCM4334
#define SDIO_DEVICE_ID_BRCM_BCM4334	0x4334
#endif

#ifndef SDIO_DEVICE_ID_BRCM_BCM43241
#define SDIO_DEVICE_ID_BRCM_BCM43241	0x4324
#endif

/*
 * This hook just adds a quirk for all sdio devices
 */
static void add_quirk_for_sdio_devices(struct mmc_card *card, int data)
{
	if (mmc_card_sdio(card))
		card->quirks |= data;
}

static const struct mmc_fixup mmc_fixup_methods[] = {
	/* by default sdio devices are considered CLK_GATING broken */
	/* good cards will be whitelisted as they are tested */
	SDIO_FIXUP(SDIO_ANY_ID, SDIO_ANY_ID,
		   add_quirk_for_sdio_devices,
		   MMC_QUIRK_BROKEN_CLK_GATING),

	SDIO_FIXUP(SDIO_VENDOR_ID_TI, SDIO_DEVICE_ID_TI_WL1271,
		   remove_quirk, MMC_QUIRK_BROKEN_CLK_GATING),

	SDIO_FIXUP(SDIO_VENDOR_ID_TI, SDIO_DEVICE_ID_TI_WL1271,
		   add_quirk, MMC_QUIRK_NONSTD_FUNC_IF),

	SDIO_FIXUP(SDIO_VENDOR_ID_TI, SDIO_DEVICE_ID_TI_WL1271,
		   add_quirk, MMC_QUIRK_DISABLE_CD),

	SDIO_FIXUP(SDIO_VENDOR_ID_BRCM, SDIO_DEVICE_ID_BRCM_BCM4330,
			remove_quirk, MMC_QUIRK_BROKEN_CLK_GATING),

	SDIO_FIXUP(SDIO_VENDOR_ID_BRCM, SDIO_DEVICE_ID_BRCM_BCM4334,
		   remove_quirk, MMC_QUIRK_BROKEN_CLK_GATING),

	SDIO_FIXUP(SDIO_VENDOR_ID_BRCM, SDIO_DEVICE_ID_BRCM_BCM43241,
		   remove_quirk, MMC_QUIRK_BROKEN_CLK_GATING),

	END_FIXUP
};

void mmc_fixup_device(struct mmc_card *card, const struct mmc_fixup *table)
{
	const struct mmc_fixup *f;
	u64 rev = cid_rev_card(card);

	/* Non-core specific workarounds. */
	if (!table)
		table = mmc_fixup_methods;

	for (f = table; f->vendor_fixup; f++) {
		if ((f->manfid == CID_MANFID_ANY ||
		     f->manfid == card->cid.manfid) &&
		    (f->oemid == CID_OEMID_ANY ||
		     f->oemid == card->cid.oemid) &&
		    (f->name == CID_NAME_ANY ||
		     !strncmp(f->name, card->cid.prod_name,
			      sizeof(card->cid.prod_name))) &&
		    (f->cis_vendor == card->cis.vendor ||
		     f->cis_vendor == (u16) SDIO_ANY_ID) &&
		    (f->cis_device == card->cis.device ||
		     f->cis_device == (u16) SDIO_ANY_ID) &&
		    rev >= f->rev_start && rev <= f->rev_end) {
			dev_dbg(&card->dev, "calling %pF\n", f->vendor_fixup);
			f->vendor_fixup(card, f->data);
		}
	}
}
EXPORT_SYMBOL(mmc_fixup_device);

static int mmc_movi_vendor_cmd(struct mmc_card *card, u32 arg)
{
	struct mmc_command cmd = {0};
	int err;
	u32 status;

	/* CMD62 is vendor CMD, it's not defined in eMMC spec. */
	cmd.opcode = 62;
	cmd.flags = MMC_RSP_R1B | MMC_CMD_AC;
	cmd.arg = arg;

	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	mdelay(10);

	if (err)
		return err;

	do {
		err = mmc_send_status(card, &status);
		if (err)
			return err;
		if (card->host->caps & MMC_CAP_WAIT_WHILE_BUSY)
			break;
		if (mmc_host_is_spi(card->host))
			break;
	} while (R1_CURRENT_STATE(status) == R1_STATE_PRG);

	return err;
}

static int mmc_movi_erase_cmd(struct mmc_card *card,
                              unsigned int arg1, unsigned int arg2)
{
	struct mmc_command cmd = {0};
	int err;
	u32 status;

	cmd.opcode = MMC_ERASE_GROUP_START;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
	cmd.arg = arg1;

	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err)
		return err;

	memset(&cmd, 0, sizeof(struct mmc_command));
	cmd.opcode = MMC_ERASE_GROUP_END;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
	cmd.arg = arg2;

	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err)
		return err;

	memset(&cmd, 0, sizeof(struct mmc_command));
	cmd.opcode = MMC_ERASE;
	cmd.flags = MMC_RSP_R1B | MMC_CMD_AC;
	cmd.arg = 0x00000000;

	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err)
		return err;

	do {
		err = mmc_send_status(card, &status);
		if (err)
			return err;
		if (card->host->caps & MMC_CAP_WAIT_WHILE_BUSY)
			break;
		if (mmc_host_is_spi(card->host))
			break;
	} while (R1_CURRENT_STATE(status) == R1_STATE_PRG);

	return err;
}

static int mmc_movi_read_cmd(struct mmc_card *card, u8 *buffer, u32 arg,
                             u32 blocks)
{
	struct mmc_request brq = {0};
	struct mmc_command wcmd = {0};
	struct mmc_data wdata = {0};
	struct scatterlist sg;

	brq.cmd = &wcmd;
	brq.data = &wdata;

	if (blocks > 1)
		wcmd.opcode = MMC_READ_MULTIPLE_BLOCK;
	else
		wcmd.opcode = MMC_READ_SINGLE_BLOCK;
	wcmd.arg = arg;
	wcmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	wdata.blksz = 512;
	brq.stop = NULL;
	wdata.blocks = blocks;
	wdata.flags = MMC_DATA_READ;

	wdata.sg = &sg;
	wdata.sg_len = 1;

	/* Check for integer multiplication overflow */
	if (unlikely(wdata.blocks > UINT_MAX/wdata.blksz))
		return -ENOMEM;

	sg_init_one(&sg, buffer, wdata.blksz * wdata.blocks);

	mmc_set_data_timeout(&wdata, card);

	mmc_wait_for_req(card->host, &brq);

	if (wcmd.error)
		return wcmd.error;
	if (wdata.error)
		return wdata.error;
	return 0;
}

int mmc_movi_read_ram_page(struct mmc_card *card, u8 *buffer, u32 address)
{
	int err = 0, errx = 0;
	u32 val;

	mmc_claim_host(card->host);

	/* enter vendor mode for RAM reading */
	err = mmc_movi_vendor_cmd(card, 0xEFAC62EC);
	if (err)
		goto out;

	err = mmc_movi_vendor_cmd(card, 0x10210002);
	if (err)
		goto out;

	/* In the current mode, erase command sets RAM address to read. */
	err = mmc_movi_erase_cmd(card, cpu_to_le32(address), 512);
	if (err) {
		pr_err("%s: Failed to set read address\n",
				mmc_hostname(card->host));
		goto err_check_patch;
	}

	err = mmc_movi_read_cmd(card, (u8 *)buffer, 0, 1);
	if (err) {
		pr_err("%s: Failed to read patch\n",
				mmc_hostname(card->host));
		goto err_check_patch;
	}

err_check_patch:
	/* exit vendor cmd mode */
	errx = mmc_movi_vendor_cmd(card, 0xEFAC62EC);
	if (errx)
		goto out;

	errx = mmc_movi_vendor_cmd(card, 0x00DECCEE);
	if (errx || err)
		goto out;

out:
	mmc_release_host(card->host);
	return err;
}
EXPORT_SYMBOL(mmc_movi_read_ram_page);

static int mmc_movi_sds_patch(struct mmc_card *card)
{
	int err = 0, errx = 0;
	void *buffer;
	unsigned i;
	static const struct {
		u32 address;
		u32 value;
	} patches[] = {
		/* write the new function to the RAM */
		{ 0x00040300, 0x4A03B510 },
		{ 0x00040304, 0x28004790 },
		{ 0x00040308, 0xE7FED100 },
		{ 0x0004030C, 0x0000BD10 },
		{ 0x00040310, 0x00059D73 },
		/* patch the old call to call the new function */
		{ 0x0005C7EA, 0xFD89F7E3 },
	};
	pr_info("%s: Fixing MoviNAND SDS bug.\n", mmc_hostname(card->host));

	buffer = kmalloc(512, GFP_KERNEL);
	if (!buffer) {
		pr_err("%s: kmalloc failed.\n", __func__);
		return -ENOMEM;
	}

	/* enter vendor mode for RAM patching */
	err = mmc_movi_vendor_cmd(card, 0xEFAC62EC);
	if (err)
		goto out;

	err = mmc_movi_vendor_cmd(card, 0x10210000);
	if (err)
		goto out;

	/* In the current mode, erase command modifies RAM. */
	for (i = 0; i < ARRAY_SIZE(patches); i++) {
		err = mmc_movi_erase_cmd(card, patches[i].address,
		                         cpu_to_le32(patches[i].value));
		if (err) {
			pr_err("%s: Patch %u failed\n",
			       mmc_hostname(card->host), i);
			goto err_patch;
		}
	}

err_patch:
	/* exit vendor command mode */
	errx = mmc_movi_vendor_cmd(card, 0xEFAC62EC);
	if (errx)
		goto out;

	errx = mmc_movi_vendor_cmd(card, 0x00DECCEE);
	if (errx || err)
		goto out;

	/* verify the written patch */
	pr_info("%s: Verifying SDS patch.\n", mmc_hostname(card->host));

	/* enter vendor mode for RAM reading */
	err = mmc_movi_vendor_cmd(card, 0xEFAC62EC);
	if (err)
		goto out;

	err = mmc_movi_vendor_cmd(card, 0x10210002);
	if (err)
		goto out;

	/* In the current mode, erase command sets RAM address to read. */
	for (i = 0; i < ARRAY_SIZE(patches); i++) {
		err = mmc_movi_erase_cmd(card, patches[i].address, 4);
		if (err) {
			pr_err("%s: Failed to set read address for patch %u\n",
			       mmc_hostname(card->host), i);
			goto err_check_patch;
		}

		err = mmc_movi_read_cmd(card, (u8 *)buffer, 0, 1);
		if (err) {
			pr_err("%s: Failed to read patch %u\n",
			       mmc_hostname(card->host), i);
			goto err_check_patch;
		}

		if (cpu_to_le32(*(u32 *)buffer) != patches[i].value) {
			pr_err("%s: Patch %u verification failed\n",
			       mmc_hostname(card->host), i);
			goto err_check_patch;
		}
	}

err_check_patch:
	/* exit vendor cmd mode */
	errx = mmc_movi_vendor_cmd(card, 0xEFAC62EC);
	if (errx)
		goto out;

	errx = mmc_movi_vendor_cmd(card, 0x00DECCEE);
	if (errx || err)
		goto out;

out:
	kfree(buffer);
	if (err)
		return err;
	return errx;
}

int mmc_movi_sds_fixup(struct mmc_card *card)
{
	int err;

	mmc_claim_host(card->host);
	err = mmc_movi_sds_patch(card);
	mmc_release_host(card->host);

	return err;
}

static int mmc_samsung_smart_read(struct mmc_card *card, u8 *rdblock)
{
	int err, errx;

	/* enter vendor Smart Report mode */
	err = mmc_movi_vendor_cmd(card, 0xEFAC62EC);
	if (err) {
		pr_err("%s: Failed entering Smart Report mode(1, %d)\n",
		       mmc_hostname(card->host), err);
		return err;
	}
	err = mmc_movi_vendor_cmd(card, 0x0000CCEE);
	if (err) {
		pr_err("%s: Failed entering Smart Report mode(2, %d)\n",
		       mmc_hostname(card->host), err);
		return err;
	}

	/* read Smart Report */
	err = mmc_movi_read_cmd(card, rdblock, 0x1000, 1);
	if (err) {
		pr_err("%s: Failed reading Smart Report(%d)\n",
		       mmc_hostname(card->host), err);
	}
	/* Do NOT return yet; we must leave Smart Report mode.*/

	/* exit vendor Smart Report mode */
	errx = mmc_movi_vendor_cmd(card, 0xEFAC62EC);
	if (errx) {
		pr_err("%s: Failed exiting Smart Report mode(1, %d)\n",
		       mmc_hostname(card->host), errx);
	} else {
		errx = mmc_movi_vendor_cmd(card, 0x00DECCEE);
		if (errx) {
			pr_err("%s: Failed exiting Smart Report mode(2, %d)\n",
			       mmc_hostname(card->host), errx);
		}
	}

	if (err)
		return err;

	return errx;
}

void mmc_movi_sds_add_quirk(struct mmc_card *card, int data)
{
	u32 *buffer;
	int err;
	u32 date;

	/* Only production revision 0xf1 is affected */
	if (card->cid.prod_rev != 0xf1)
		return;

	/* Read and check firmware date from Smart Report */
	buffer = kmalloc(512, GFP_KERNEL);
	if (!buffer) {
		pr_err("%s: kmalloc failed.\n", __func__);
		return;
	}

	mmc_claim_host(card->host);
	err = mmc_samsung_smart_read(card, (u8 *)buffer);
	mmc_release_host(card->host);

	if (err) {
		pr_err("%s: Failed to read Smart Report.\n",
		       mmc_hostname(card->host));
		return;
	}

	date = le32_to_cpu(buffer[81]);
	kfree(buffer);

	if (date != 0x20120413)
		return;

	pr_warn("%s: SDS fixup is needed.\n", mmc_hostname(card->host));
	card->quirks |= data;

	/* We need to fixup now, since mmc_init_card was already called. */
	err = mmc_movi_sds_fixup(card);
	if (err)
		pr_err("%s: SDS fixup failed.\n", mmc_hostname(card->host));

	init_mmc_ram(card);
}

