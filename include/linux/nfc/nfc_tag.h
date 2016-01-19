/*
 * Copyright (C) 2008 Samsung Electronics, Inc.
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
#ifndef __ASM_ARCH_NFC_TAG_H
#define __ASM_ARCH_NFC_TAG_H

#ifdef __KERNEL__


struct nfc_tag_info {
	struct nfc_tag_pdata *pdata;
	int irq_no;
};

struct nfc_tag_pdata {
          int irq_nfc;
};


#endif
#endif
