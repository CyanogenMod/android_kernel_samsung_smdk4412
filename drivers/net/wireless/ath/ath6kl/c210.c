#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/platform_device.h>

#include "core.h"

typedef int             A_BOOL;
typedef unsigned char   A_UCHAR;
typedef unsigned long   A_ATH_TIMER;
typedef int8_t      A_INT8;
typedef int16_t     A_INT16;
typedef int32_t     A_INT32;
typedef u_int8_t     A_UINT8;
typedef u_int16_t    A_UINT16;
typedef u_int32_t    A_UINT32;

#define WMI_MAX_SSID_LEN    32
#define ATH_MAC_LEN         6               /* length of mac in bytes */

#define __ATTRIB_PACK
#define POSTPACK                __ATTRIB_PACK
#define PREPACK

typedef PREPACK struct {
    PREPACK union {
        A_UINT8 ie[17];
        A_INT32 wac_status;
    } POSTPACK info;
} POSTPACK WMI_GET_WAC_INFO;

struct ar_wep_key {
    A_UINT8                 arKeyIndex;
    A_UINT8                 arKeyLen;
    A_UINT8                 arKey[64];
} ;

#define A_MDELAY(msecs)                 mdelay(msecs)

#define WMI_MAX_RATE_MASK         2

#define GET_INODE_FROM_FILEP(filp) \
	(filp)->f_path.dentry->d_inode
typedef char            A_CHAR;
int android_readwrite_file(const A_CHAR *filename, A_CHAR *rbuf, const A_CHAR *wbuf, size_t length)
{
	int ret = 0;
	struct file *filp = (struct file *)-ENOENT;
	mm_segment_t oldfs;
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	do {
		int mode = (wbuf) ? O_RDWR|O_CREAT : O_RDONLY;
		filp = filp_open(filename, mode,
			S_IRUSR | S_IWUSR | S_IRGRP |
			S_IWGRP | S_IROTH | S_IWOTH);

		if (IS_ERR(filp) || !filp->f_op) {
			ret = -ENOENT;
			break;
		}

		if (length == 0) {
			/* Read the length of the file only */
			struct inode    *inode;

			inode = GET_INODE_FROM_FILEP(filp);
			if (!inode) {
				printk(KERN_INFO "android_readwrite_file: Error 2\n");
				ret = -ENOENT;
				break;
			}
			ret = i_size_read(inode->i_mapping->host);
			break;
		}

		if (wbuf) {
			if ((ret=filp->f_op->write(filp, wbuf, length, &filp->f_pos)) < 0) {
				printk(KERN_INFO "android_readwrite_file: Error 3\n");
				break;
			}
		} else {
			if ((ret=filp->f_op->read(filp, rbuf, length, &filp->f_pos)) < 0) {
				printk(KERN_INFO "android_readwrite_file: Error 4\n");
				break;
			}
		}
	} while (0);

	if (!IS_ERR(filp)) {
		filp_close(filp, NULL);
	}

	set_fs(oldfs);
	printk(KERN_INFO "android_readwrite_file: ret=%d\n", ret);

	return ret;
}

extern void wlan_setup_power(int on, int detect);

static int ath6kl_pm_probe(struct platform_device *pdev)
{
    wlan_setup_power(1,1);
    return 0;
}

static int ath6kl_pm_remove(struct platform_device *pdev)
{
    wlan_setup_power(0,1);
    return 0;
}

static int ath6kl_pm_suspend(struct platform_device *pdev, pm_message_t state)
{
    return 0;
}

static inline void *ar6k_priv(struct net_device *dev)
{
    return (wdev_priv(dev->ieee80211_ptr));
}

static int ath6kl_pm_resume(struct platform_device *pdev)
{
    return 0;
}


static struct platform_driver ath6kl_pm_device = {
    .probe      = ath6kl_pm_probe,
    .remove     = ath6kl_pm_remove,
    .suspend    = ath6kl_pm_suspend,
    .resume     = ath6kl_pm_resume,
    .driver     = {
	    .name = "wlan_ar6000_pm_dev",
    },
};

void __init ath6kl_sdio_init_c210(void)
{
	platform_driver_register(&ath6kl_pm_device);
	A_MDELAY(50);
}

void __exit ath6kl_sdio_exit_c210(void)
{
	platform_driver_unregister(&ath6kl_pm_device);
	A_MDELAY(1000);
}
