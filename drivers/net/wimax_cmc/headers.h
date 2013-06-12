/*
 * headers.h
 *
 * Global definitions and fynctions
 */
#ifndef _WIMAX_HEADERS_H
#define _WIMAX_HEADERS_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/proc_fs.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <asm/byteorder.h>
#include <asm/uaccess.h>
#include <linux/wakelock.h>
#include <linux/wimax/samsung/wimax732.h>
#include "buffer.h"
#include "wimax_sdio.h"

#define WIMAXMAC_TXT_PATH	"/efs/WiMAXMAC.txt"
#define WIMAX_IMAGE_PATH	"/system/etc/wimaxfw.bin"
#define WIMAX_LOADER_PATH	"/system/etc/wimaxloader.bin"
#define WIMAX_BOOTIMAGE_PATH	"/system/etc/wimax_boot.bin"

#define STATUS_SUCCESS			((u_long)0x00000000L)
#define STATUS_PENDING			((u_long)0x00000103L)	/* The operation that was requested is pending completion */
#define STATUS_RESOURCES		((u_long)0x00001003L)
#define STATUS_RESET_IN_PROGRESS	((u_long)0xc001000dL)
#define STATUS_DEVICE_FAILED		((u_long)0xc0010008L)
#define STATUS_NOT_ACCEPTED		((u_long)0x00010003L)
#define STATUS_FAILURE			((u_long)0xC0000001L)
#define STATUS_UNSUCCESSFUL		((u_long)0xC0000002L)	/* The requested operation was unsuccessful */
#define STATUS_CANCELLED		((u_long)0xC0000003L)

#ifndef TRUE_FALSE_
#define TRUE_FALSE_
enum BOOL {
	FALSE,
	TRUE
};
#endif

#define HARDWARE_USE_ALIGN_HEADER

#define dump_debug(args...)	\
{	\
	printk(KERN_ALERT"\x1b[1;33m[WiMAX] ");	\
	printk(args);	\
	printk("\x1b[0m\n");	\
}


/* external functions & variables */
extern void set_wimax_pm(void(*suspend)(void), void(*resume)(void));
extern void unset_wimax_pm(void);
extern int cmc732_sdio_reset_comm(struct mmc_card *card);
extern u_int system_rev;

/* receive.c functions */
u_int process_sdio_data(struct net_adapter *adapter, void *buffer, u_long length, long Timeout);

/* control.c functions */
u_int control_send(struct net_adapter *adapter, void *buffer, u_long length);
void control_recv(struct net_adapter   *adapter, void *buffer, u_long length);
u_int control_init(struct net_adapter *adapter);
void control_remove(struct net_adapter *adapter);

struct process_descriptor *process_by_id(struct net_adapter *adapter, u_int id);
struct process_descriptor *process_by_type(struct net_adapter *adapter, u_short type);
void remove_process(struct net_adapter *adapter, u_int id);

u_long buffer_count(struct list_head ListHead);
struct buffer_descriptor *buffer_by_type(struct list_head ListHead, u_short type);
void dump_buffer(const char *desc, u_char *buffer, u_int len);

/* hardware.c functions */
void switch_eeprom_ap(void);
void switch_eeprom_wimax(void);
void switch_usb_ap(void);
void switch_usb_wimax(void);
void display_gpios(void);	/* display GPIO status used by WiMAX module */
void switch_uart_ap(void);
void switch_uart_wimax(void);
void hw_init_gpios(void);
void hw_deinit_gpios(void);

u_int sd_send(struct net_adapter *adapter, u_char *buffer, u_int len);
u_int sd_send_data(struct net_adapter *adapter, struct buffer_descriptor *dsc);
u_int hw_send_data(struct net_adapter *adapter, void *buffer, u_long length,bool);
void hw_return_packet(struct net_adapter *adapter, u_short type);

void s3c_bat_use_wimax(int onoff);

int gpio_wimax_poweron (void);
int gpio_wimax_poweroff (void);

int hw_start(struct net_adapter *adapter);
int hw_stop(struct net_adapter *adapter);
int hw_init(struct net_adapter *adapter);
void hw_remove(struct net_adapter *adapter);
void hw_get_mac_address(void *data);
int con0_poll_thread(void *data);
void hw_transmit_thread(struct work_struct *work);
void adapter_interrupt(struct sdio_func *func);
/* structures for global access */
extern struct net_adapter *g_adapter;

#endif	/* _WIMAX_HEADERS_H */
