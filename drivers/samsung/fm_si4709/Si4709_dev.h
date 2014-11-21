#ifndef _Si4709_DEV_H
#define _Si4709_DEV_H

#include <linux/i2c.h>

#include "Si4709_regs.h"
#include "Si4709_common.h"

#define NUM_SEEK_PRESETS	20

#define WAIT_OVER		0
#define SEEK_WAITING		1
#define NO_WAIT			2
#define TUNE_WAITING		4
#define RDS_WAITING		5
#define SEEK_CANCEL		6

/*dev settings*/
/*band*/
#define BAND_87500_108000_kHz	1
#define BAND_76000_108000_kHz	2
#define BAND_76000_90000_kHz	3

/*channel spacing*/
#define CHAN_SPACING_200_kHz	20	/*US*/
#define CHAN_SPACING_100_kHz	10	/*Europe,Japan */
#define CHAN_SPACING_50_kHz	5
/*DE-emphasis Time Constant*/
#define DE_TIME_CONSTANT_50	1	/*Europe,Japan,Australia */
#define DE_TIME_CONSTANT_75	0	/*US*/
struct dev_state_t {
	int power_state;
	int seek_state;
};

struct rssi_snr_t {
	u8 curr_rssi;
	u8 curr_rssi_th;
	u8 curr_snr;
};

struct device_id {
	u8 part_number;
	u16 manufact_number;
};

struct chip_id {
	u8 chip_version;
	u8 device;
	u8 firmware_version;
};

struct sys_config2 {
	u16 rssi_th;
	u8 fm_band;
	u8 fm_chan_spac;
	u8 fm_vol;
};

struct sys_config3 {
	u8 smmute;
	u8 smutea;
	u8 volext;
	u8 sksnr;
	u8 skcnt;
};

struct status_rssi {
	u8 rdsr;
	u8 stc;
	u8 sfbl;
	u8 afcrl;
	u8 rdss;
	u8 blera;
	u8 st;
	u16 rssi;
};

struct power_config {
	u16 dsmute:1;
	u16 dmute:1;
	u16 mono:1;
	u16 rds_mode:1;
	u16 sk_mode:1;
	u16 seek_up:1;
	u16 seek:1;
	u16 power_disable:1;
	u16 power_enable:1;
};

struct radio_data_t {
	u16 rdsa;
	u16 rdsb;
	u16 rdsc;
	u16 rdsd;
	u8 curr_rssi;
	u32 curr_channel;
	u8 blera;
	u8 blerb;
	u8 blerc;
	u8 blerd;
};

struct channel_into_t {
	u32 frequency;
	u8 rsssi_val;
};

struct dev_settings_t {
	u16 band;
	u32 bottom_of_band;
	u16 channel_spacing;
	u32 timeout_RDS;      /****For storing the jiffy value****/
	u32 seek_preset[NUM_SEEK_PRESETS];
	u8 curr_snr;
	u8 curr_rssi_th;
};

struct Si4709_device_t {
	/*Any function which
	   - views/modifies the fields of this structure
	   - does i2c communication
	   should lock the mutex before doing so.
	   Recursive locking should not be done.
	   In this file all the exported functions will take care
	   of this. The static functions will not deal with the
	   mutex */
	struct mutex lock;

	struct i2c_client const *client;

	struct dev_state_t state;

	struct dev_settings_t settings;

	struct channel_into_t rssi_freq[50];

	u16 registers[NUM_OF_REGISTERS];

	/* This field will be checked by all the functions
	   exported by this file (except the init function),
	   to validate the the fields of this structure.
	   if eTRUE: the fileds are valid
	   if eFALSE: do not trust the values of the fields
	   of this structure */
	unsigned short valid;

	/*will be true is the client ans state fields are correct */
	unsigned short valid_client_state;

	u8 vol_idx;
};

extern int Si4709_dev_wait_flag;

#ifdef RDS_INTERRUPT_ON_ALWAYS
extern int Si4709_RDS_flag;
extern int RDS_Data_Available;
extern int RDS_Data_Lost;
extern int RDS_Groups_Available_till_now;
extern struct workqueue_struct *Si4709_wq;
extern struct work_struct Si4709_work;
#endif

/*	Function prototypes	*/

/*extern functions*/
/**********************************************/
/*	All the exported functions which view or modify the device
	state/data, do i2c com will have to lock the mutex before
	doing so
*/
/**********************************************/

extern int Si4709_dev_init(struct i2c_client *);
extern int Si4709_dev_exit(void);

extern void Si4709_dev_mutex_init(void);
extern void Si4709_dev_mutex_destroy(void);

extern int Si4709_dev_suspend(void);
extern int Si4709_dev_resume(void);

extern int Si4709_dev_powerup(void);
extern int Si4709_dev_powerdown(void);

extern int Si4709_dev_band_set(int);
extern int Si4709_dev_ch_spacing_set(int);

extern int Si4709_dev_chan_select(u32);
extern int Si4709_dev_chan_get(u32 *);

extern int Si4709_dev_seek_full(u32 *);
extern int Si4709_dev_seek_up(u32 *);
extern int Si4709_dev_seek_down(u32 *);
extern int Si4709_dev_seek_auto(u32 *);

extern int Si4709_dev_RSSI_seek_th_set(u8);
extern int Si4709_dev_seek_SNR_th_set(u8);
extern int Si4709_dev_seek_FM_ID_th_set(u8);
extern int Si4709_dev_cur_RSSI_get(struct rssi_snr_t *);
extern int Si4709_dev_VOLEXT_ENB(void);
extern int Si4709_dev_VOLEXT_DISB(void);
extern int Si4709_dev_volume_set(u8);
extern int Si4709_dev_volume_get(u8 *);
extern int Si4709_dev_DSMUTE_ON(void);
extern int Si4709_dev_DSMUTE_OFF(void);
extern int Si4709_dev_MUTE_ON(void);
extern int Si4709_dev_MUTE_OFF(void);
extern int Si4709_dev_MONO_SET(void);
extern int Si4709_dev_STEREO_SET(void);
extern int Si4709_dev_rstate_get(struct dev_state_t *);
extern int Si4709_dev_RDS_data_get(struct radio_data_t *);
extern int Si4709_dev_RDS_ENABLE(void);
extern int Si4709_dev_RDS_DISABLE(void);
extern int Si4709_dev_RDS_timeout_set(u32);
extern int Si4709_dev_device_id(struct device_id *);
extern int Si4709_dev_chip_id(struct chip_id *);
extern int Si4709_dev_sys_config2(struct sys_config2 *);
extern int Si4709_dev_sys_config3(struct sys_config3 *);
extern int Si4709_dev_power_config(struct power_config *);
extern int Si4709_dev_AFCRL_get(u8 *);
extern int Si4709_dev_DE_set(u8);
extern int Si4709_dev_status_rssi(struct status_rssi *status);
extern int Si4709_dev_sys_config2_set(struct sys_config2 *sys_conf2);
extern int Si4709_dev_sys_config3_set(struct sys_config3 *sys_conf3);
extern int Si4709_dev_reset_rds_data(void);

/***********************************************/

#ifdef RDS_INTERRUPT_ON_ALWAYS
extern void Si4709_work_func(struct work_struct *);
#endif
#endif
