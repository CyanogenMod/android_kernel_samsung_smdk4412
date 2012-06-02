/**
 * header for ipc_spi driver
 *
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __IPC_SPI_H__
#define __IPC_SPI_H__

#include <linux/ioport.h>
#include <linux/types.h>

struct ipc_spi_platform_data {
	unsigned gpio_ipc_mrdy;
	unsigned gpio_ipc_srdy;
	unsigned gpio_ipc_sub_mrdy;
	unsigned gpio_ipc_sub_srdy;

	void (*cfg_gpio)(void);
};

extern int onedram_register_handler(void (*handler)(u32, void *), void *data);
extern int onedram_unregister_handler(void (*handler)(u32, void *));
extern struct resource *onedram_request_region(resource_size_t start,
	resource_size_t size, const char *name);
extern void onedram_release_region(resource_size_t start,
	resource_size_t size);
extern int onedram_read_mailbox(u32 *);
extern int onedram_write_mailbox(u32);
extern int onedram_get_auth(u32 cmd);
extern int onedram_put_auth(int release);
extern int onedram_rel_sem(void);
extern int onedram_read_sem(void);
extern void onedram_get_vbase(void **);

#if !defined(CONFIG_MACH_Q1_BD)
extern unsigned int lpcharge;
#endif

#define ONEDRAM_GET_AUTH _IOW('o', 0x20, u32)
#define ONEDRAM_CP_CRASH _IO('o', 0x21)
#define ONEDRAM_REL_SEM _IO('o', 0x22)
#define ONEDRAM_SEMA_INIT _IO('o', 0x23)

#endif /* __IPC_SPI_H__ */
