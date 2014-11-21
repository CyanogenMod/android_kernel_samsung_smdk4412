/*
*
* drivers/media/tdmb/tdmb.h
*
* tdmb driver
*
* Copyright (C) (2011, Samsung Electronics)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#ifndef _TDMB_H_
#define _TDMB_H_

#include <linux/types.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <mach/tdmb_pdata.h>
#include <mach/gpio.h>


#define TDMB_DEBUG

#ifdef TDMB_DEBUG
#define DPRINTK(x...) printk(KERN_DEBUG "TDMB " x)
#else
#define DPRINTK(x...) /* null */
#endif

#define TDMB_DEV_NAME	"tdmb"
#define TDMB_DEV_MAJOR	225
#define TDMB_DEV_MINOR	0

#define DMB_TS_SIZE	188

#define TDMB_RING_BUFFER_SIZE		(188 * 150 + 4 + 4)
#define TDMB_RING_BUFFER_MAPPING_SIZE	\
		(((TDMB_RING_BUFFER_SIZE - 1) / PAGE_SIZE + 1) * PAGE_SIZE)

/* commands */
#define IOCTL_MAGIC	't'
#define IOCTL_MAXNR	32

#define IOCTL_TDMB_GET_DATA_BUFFSIZE	_IO(IOCTL_MAGIC, 0)
#define IOCTL_TDMB_GET_CMD_BUFFSIZE	_IO(IOCTL_MAGIC, 1)
#define IOCTL_TDMB_POWER_ON			_IO(IOCTL_MAGIC, 2)
#define IOCTL_TDMB_POWER_OFF			_IO(IOCTL_MAGIC, 3)
#define IOCTL_TDMB_SCAN_FREQ_ASYNC	_IO(IOCTL_MAGIC, 4)
#define IOCTL_TDMB_SCAN_FREQ_SYNC	_IO(IOCTL_MAGIC, 5)
#define IOCTL_TDMB_SCANSTOP			_IO(IOCTL_MAGIC, 6)
#define IOCTL_TDMB_ASSIGN_CH			_IO(IOCTL_MAGIC, 7)
#define IOCTL_TDMB_GET_DM				_IO(IOCTL_MAGIC, 8)
#define IOCTL_TDMB_ASSIGN_CH_TEST	_IO(IOCTL_MAGIC, 9)
#define IOCTL_TDMB_SET_AUTOSTART	_IO(IOCTL_MAGIC, 10)


struct tdmb_dm {
	unsigned int	rssi;
	unsigned int	ber;
	unsigned int	per;
	unsigned int	antenna;
} ;

#define SUB_CH_NUM_MAX				64

#define ENSEMBLE_LABEL_MAX	16
#define SVC_LABEL_MAX	16

enum {
	TMID_MSC_STREAM_AUDIO		= 0x00,
	TMID_MSC_STREAM_DATA		= 0x01,
	TMID_FIDC					= 0x02,
	TMID_MSC_PACKET_DATA		= 0x03
};

enum {
	DSCTy_TDMB					= 0x18,
	/* Used for All-Zero Test */
	DSCTy_UNSPECIFIED			= 0x00
};

struct sub_ch_info_type {
	/* Sub Channel Information */
	unsigned char sub_ch_id; /* 6 bits */
	unsigned short start_addr; /* 10 bits */

	/* FIG 0/2	*/
	unsigned char tmid; /* 2 bits */
	unsigned char svc_type; /* 6 bits */
	unsigned long svc_id; /* 16/32 bits */
	unsigned char svc_label[SVC_LABEL_MAX+1]; /* 16*8 bits */
	unsigned char ecc;	/* 8 bits */
	unsigned char scids;	/* 4 bits */
} ;

struct ensemble_info_type {
	unsigned long ensem_freq;	/* 4 bytes */
	unsigned char tot_sub_ch;	/* MAX: 64 */

	unsigned short ensem_id;
	unsigned char ensem_label[ENSEMBLE_LABEL_MAX+1];
	struct sub_ch_info_type sub_ch[SUB_CH_NUM_MAX];
} ;


#define TDMB_CMD_START_FLAG	0x7F
#define TDMB_CMD_END_FLAG		0x7E
#define TDMB_CMD_SIZE			30

/* Result Value */
#define DMB_FIC_RESULT_FAIL	0x00
#define DMB_FIC_RESULT_DONE	0x01
#define DMB_TS_PACKET_RESYNC	0x02

#if defined(CONFIG_TDMB_EBI)
int tdmb_init_bus(unsigned long addr, int size);
#else
int tdmb_init_bus(void);
#endif
void tdmb_exit_bus(void);
irqreturn_t tdmb_irq_handler(int irq, void *dev_id);
unsigned long tdmb_get_chinfo(void);
void tdmb_pull_data(struct work_struct *work);
bool tdmb_control_irq(bool set);
void tdmb_control_gpio(bool poweron);
bool tdmb_create_workqueue(void);
bool tdmb_destroy_workqueue(void);
bool tdmb_create_databuffer(unsigned long int_size);
void tdmb_destroy_databuffer(void);
void tdmb_init_data(void);
#if defined(CONFIG_TDMB_ANT_DET)
bool tdmb_ant_det_irq_set(bool set);
#endif
unsigned char tdmb_make_result
(
	unsigned char cmd,
	unsigned short data_len,
	unsigned char  *data
);
bool tdmb_store_data(unsigned char *data, unsigned long len);

struct tdmb_drv_func {
	bool (*init) (void);
	bool (*power_on) (void);
	void (*power_off) (void);
	bool (*scan_ch) (struct ensemble_info_type *ensembleInfo,
						unsigned long freq);
	void (*get_dm) (struct tdmb_dm *info);
	bool (*set_ch) (unsigned long freq, unsigned char subchid,
						bool factory_test);
	void (*pull_data) (void);
	unsigned long (*get_int_size) (void);
};

extern unsigned int get_hw_rev(void);

extern unsigned int *tdmb_ts_head;
extern unsigned int *tdmb_ts_tail;
extern char *tdmb_ts_buffer;
extern unsigned int tdmb_ts_size;

#if defined(CONFIG_TDMB_T3900) || defined(CONFIG_TDMB_T39F0)
struct tdmb_drv_func *t3900_drv_func(void);
#endif
#if defined(CONFIG_TDMB_FC8050)
struct tdmb_drv_func *fc8050_drv_func(void);
#endif
#if defined(CONFIG_TDMB_MTV318)
struct tdmb_drv_func *mtv318_drv_func(void);
#endif
#if defined(CONFIG_TDMB_TCC3170)
struct tdmb_drv_func *tcc3170_drv_func(void);
extern struct tcbd_fic_ensbl *tcbd_fic_get_ensbl_info(s32 _disp);
#endif

#if defined(CONFIG_TDMB_SPI)
struct spi_device *tdmb_get_spi_handle(void);
#endif

#ifdef CONFIG_BATTERY_SEC
extern unsigned int is_lpcharging_state(void);
#endif

#endif
