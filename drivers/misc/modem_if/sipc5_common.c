/*
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/etherdevice.h>
#include <linux/device.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include "modem_utils.h"

/**
 * sipc5_start_valid
 * @cfg: configuration field of an SIPC5 link frame
 *
 * Returns TRUE if the start (configuration field) of an SIPC5 link frame
 * is valid or returns FALSE if it is not valid.
 *
 */
bool sipc5_start_valid(u8 *frm)
{
	return (*frm & SIPC5_START_MASK) == SIPC5_START_MASK;
}

bool sipc5_padding_exist(u8 *frm)
{
	return (*frm & SIPC5_PADDING_EXIST) ? true : false;
}

bool sipc5_multi_frame(u8 *frm)
{
	return (*frm & SIPC5_EXT_FIELD_MASK) == SIPC5_CTL_FIELD_MASK;
}

bool sipc5_ext_len(u8 *frm)
{
	return (*frm & SIPC5_EXT_FIELD_MASK) == SIPC5_EXT_LENGTH_MASK;
}

/**
 * sipc5_get_hdr_len
 * @cfg: configuration field of an SIPC5 link frame
 *
 * Returns the length of SIPC5 link layer header in an SIPC5 link frame
 *
 */
int sipc5_get_hdr_len(u8 *frm)
{
	if (*frm & SIPC5_EXT_FIELD_EXIST) {
		if (*frm & SIPC5_CTL_FIELD_EXIST)
			return SIPC5_HEADER_SIZE_WITH_CTL_FLD;
		else
			return SIPC5_HEADER_SIZE_WITH_EXT_LEN;
	} else {
		return SIPC5_MIN_HEADER_SIZE;
	}
}

/**
 * sipc5_get_ch_id
 * @frm: pointer to an SIPC5 frame
 *
 * Returns the channel ID in an SIPC5 link frame
 *
 */
u8 sipc5_get_ch_id(u8 *frm)
{
	return *(frm + SIPC5_CH_ID_OFFSET);
}

/**
 * sipc5_get_ctrl_field
 * @frm: pointer to an SIPC5 frame
 *
 * Returns the control field in an SIPC5 link frame
 *
 */
u8 sipc5_get_ctrl_field(u8 *frm)
{
	return *(frm + SIPC5_CTL_OFFSET);
}

/**
 * sipc5_get_frame_len
 * @frm: pointer to an SIPC5 link frame
 *
 * Returns the length of an SIPC5 link frame
 *
 */
int sipc5_get_frame_len(u8 *frm)
{
	u8 cfg = frm[0];
	u16 *sz16 = (u16 *)(frm + SIPC5_LEN_OFFSET);
	u32 *sz32 = (u32 *)(frm + SIPC5_LEN_OFFSET);

	if (unlikely(cfg & SIPC5_EXT_FIELD_EXIST)) {
		if (cfg & SIPC5_CTL_FIELD_EXIST)
			return (int)(*sz16);
		else
			return (int)(*sz32);
	} else {
		return (int)(*sz16);
	}
}

/**
 * sipc5_calc_padding_size
 * @len: length of an SIPC5 link frame
 *
 * Returns the padding size for an SIPC5 link frame
 *
 */
int sipc5_calc_padding_size(int len)
{
	int residue = len & 0x3;
	return residue ? (4 - residue) : 0;
}

/**
 * sipc5_get_total_len
 * @frm: pointer to an SIPC5 link frame
 *
 * Returns the total length of an SIPC5 link frame with padding
 *
 */
int sipc5_get_total_len(u8 *frm)
{
	int len = sipc5_get_frame_len(frm);
	int pad = sipc5_padding_exist(frm) ? sipc5_calc_padding_size(len) : 0;
	return len + pad;
}

/**
 * sipc5_build_config
 * @iod: pointer to the IO device
 * @ld: pointer to the link device
 * @count: length of the data to be transmitted
 *
 * Builds a config value for an SIPC5 link frame header
 *
 * Returns the config value for the header or 0 for non-SIPC formats
 */
u8 sipc5_build_config(struct io_device *iod, struct link_device *ld, u32 count)
{
	u8 cfg = SIPC5_START_MASK;

	if (iod->format > IPC_MULTI_RAW && iod->id == 0)
		return 0;

	if (ld->aligned)
		cfg |= SIPC5_PADDING_EXIST;

#if 0
	if ((count + SIPC5_MIN_HEADER_SIZE) > ld->mtu[dev])
		cfg |= SIPC5_CTL_FIELD_MASK;
	else
#endif
	if ((count + SIPC5_MIN_HEADER_SIZE) > 0xFFFF)
		cfg |= SIPC5_EXT_LENGTH_MASK;

	return cfg;
}

/**
 * sipc5_build_header
 * @iod: pointer to the IO device
 * @ld: pointer to the link device
 * @buff: pointer to a buffer in which an SIPC5 link frame header will be stored
 * @cfg: value for the config field in the header
 * @ctrl: value for the control field in the header
 * @count: length of data in the SIPC5 link frame to be transmitted
 *
 * Builds the link layer header of an SIPC5 frame
 */
void sipc5_build_header(struct io_device *iod, struct link_device *ld,
			u8 *buff, u8 cfg, u8 ctrl, u32 count)
{
	u16 *sz16 = (u16 *)(buff + SIPC5_LEN_OFFSET);
	u32 *sz32 = (u32 *)(buff + SIPC5_LEN_OFFSET);
	u32 hdr_len = sipc5_get_hdr_len(&cfg);

	/* Store the config field and the channel ID field */
	buff[SIPC5_CONFIG_OFFSET] = cfg;
	buff[SIPC5_CH_ID_OFFSET] = iod->id;

	/* Store the frame length field */
	if (sipc5_ext_len(buff))
		*sz32 = (u32)(hdr_len + count);
	else
		*sz16 = (u16)(hdr_len + count);

	/* Store the control field */
	if (sipc5_multi_frame(buff))
		buff[SIPC5_CTL_OFFSET] = ctrl;
}

/**
 * std_udl_get_cmd
 * @frm: pointer to an SIPC5 link frame
 *
 * Returns the standard BOOT/DUMP (STD_UDL) command in an SIPC5 BOOT/DUMP frame.
 */
u32 std_udl_get_cmd(u8 *frm)
{
	u8 *cmd = frm + sipc5_get_hdr_len(frm);
	return *((u32 *)cmd);
}

/**
 * std_udl_with_payload
 * @cmd: standard BOOT/DUMP command
 *
 * Returns true if the STD_UDL command has a payload.
 */
bool std_udl_with_payload(u32 cmd)
{
	u32 mask = cmd & STD_UDL_STEP_MASK;
	return (mask && mask < STD_UDL_CRC) ? true : false;
}

