#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/err.h>

#ifdef CONFIG_ARCH_MSM7X27A
#include <mach/rpc_pmapp.h>
#include <linux/qcomwlan7x27a_pwrif.h>
#else
#include <linux/platform_device.h>
/* replace with plaftform specific changes */
#endif

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

#ifdef CONFIG_ARCH_MSM7X27A
/* BeginMMC polling stuff */
#define MMC_MSM_DEV "msm_sdcc.2"
#define A_MDELAY(msecs)                 mdelay(msecs)
/* End MMC polling stuff */
#else
/* replace with plaftform specific changes */
#endif

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
		int mode = (wbuf) ? O_RDWR : O_RDONLY;
		filp = filp_open(filename, mode, S_IRUSR);

		if (IS_ERR(filp) || !filp->f_op) {
			ret = -ENOENT;
			break;
		}

		if (length == 0) {
			/* Read the length of the file only */
			struct inode    *inode;

			inode = GET_INODE_FROM_FILEP(filp);
			if (!inode) {
				printk(KERN_ERR "android_readwrite_file: Error 2\n");
				ret = -ENOENT;
				break;
			}
			ret = i_size_read(inode->i_mapping->host);
			break;
		}

		if (wbuf) {
			if ((ret=filp->f_op->write(filp, wbuf, length, &filp->f_pos)) < 0) {
				printk(KERN_ERR "android_readwrite_file: Error 3\n");
				break;
			}
		} else {
			if ((ret=filp->f_op->read(filp, rbuf, length, &filp->f_pos)) < 0) {
				printk(KERN_ERR "android_readwrite_file: Error 4\n");
				break;
			}
		}
	} while (0);

	if (!IS_ERR(filp)) {
		filp_close(filp, NULL);
	}

	set_fs(oldfs);
	printk(KERN_ERR "android_readwrite_file: ret=%d\n", ret);

	return ret;
}


#ifdef CONFIG_ARCH_MSM7X27A

#define WLAN_GPIO_EXT_POR_N     134
#define A0_CLOCK
static const char *id = "WLAN";

enum {
	WLAN_VREG_L17 = 0,
	WLAN_VREG_L19
};

struct wlan_vreg_info {
	const char *vreg_id;
	unsigned int level_min;
	unsigned int level_max;
	unsigned int pmapp_id;
	unsigned int is_vreg_pin_controlled;
	struct regulator *reg;
};


static struct wlan_vreg_info vreg_info[] = {
	{"bt",        3300000, 3300000, 21, 1, NULL},
	{"wlan4",     1800000, 1800000, 23, 1, NULL},
};

static int qrf6285_init_regs(void)
{
	struct regulator_bulk_data regs[ARRAY_SIZE(vreg_info)];
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		regs[i].supply = vreg_info[i].vreg_id;
		regs[i].min_uV = vreg_info[i].level_min;
		regs[i].max_uV = vreg_info[i].level_max;
	}

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs), regs);
	if (rc) {
		pr_err("%s: could not get regulators: %d\n", __func__, rc);
		goto out;
	}

	for (i = 0; i < ARRAY_SIZE(regs); i++)
		vreg_info[i].reg = regs[i].consumer;

	return 0;

out:
	return rc;
}

int  msm7x27a_wifi_power(bool on)
{

	int rc = 0, index = 0;
	static bool init_done;
	static int resultFlag = 0, flag = 1;

	if (unlikely(!init_done)) {
		rc = qrf6285_init_regs();
		if (rc)
			return rc;
		else
			init_done = true;
	}

	for (index = 0; index < ARRAY_SIZE(vreg_info); index++) {
		if (on) {

			rc = regulator_set_voltage(vreg_info[index].reg,
						vreg_info[index].level_min,
						vreg_info[index].level_max);
			if (rc) {
				pr_err("%s:%s set voltage failed %d\n",
					__func__, vreg_info[index].vreg_id, rc);

				goto vreg_fail;
			}

			rc = regulator_enable(vreg_info[index].reg);
			if (rc) {
				pr_err("%s:%s vreg enable failed %d\n",
					__func__, vreg_info[index].vreg_id, rc);

				goto vreg_fail;
			}

			if (vreg_info[index].is_vreg_pin_controlled) {
				rc = pmapp_vreg_lpm_pincntrl_vote(id,
					 vreg_info[index].pmapp_id,
					 PMAPP_CLOCK_ID_A0, 1);
				if (rc) {
					pr_err("%s:%s pmapp_vreg_lpm_pincntrl"
						" for enable failed %d\n",
						__func__,
						vreg_info[index].vreg_id, rc);
					goto vreg_clock_vote_fail;
				}
			}

	     if (index == WLAN_VREG_L17)
                usleep(5);
            else if (index == WLAN_VREG_L19)
                usleep(10);

	} else {

			if (vreg_info[index].is_vreg_pin_controlled) {
				rc = pmapp_vreg_lpm_pincntrl_vote(id,
						 vreg_info[index].pmapp_id,
						 PMAPP_CLOCK_ID_A0, 0);
				if (rc) {
					pr_err("%s:%s pmapp_vreg_lpm_pincntrl"
						" for disable failed %d\n",
						__func__,
						vreg_info[index].vreg_id, rc);
				}
			}
			rc = regulator_disable(vreg_info[index].reg);
			if (rc) {
				pr_err("%s:%s vreg disable failed %d\n",
					__func__, vreg_info[index].vreg_id, rc);
			}
		}

	}

	if (on) {
		rc = gpio_request(WLAN_GPIO_EXT_POR_N, "WLAN_DEEP_SLEEP_N");
		if (rc) {
			pr_err("WLAN reset GPIO %d request failed %d\n",
			WLAN_GPIO_EXT_POR_N, rc);
			goto fail;
		}

		if(flag)
		{
			flag=0;
			rc = gpio_direction_output(WLAN_GPIO_EXT_POR_N, 1);
			if (rc < 0) {
				pr_err("WLAN reset GPIO %d set direction failed %d\n",
                                WLAN_GPIO_EXT_POR_N, rc);
                                goto fail_gpio_dir_out;
			}
		}
		printk("\n vote for WLAN GPIO 134 done. \n");
#ifdef	A0_CLOCK
            rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_A0, PMAPP_CLOCK_VOTE_ON);
			if (rc) {
                pr_err("%s: Configuring A0 to turn on"
                        " failed %d\n", __func__, rc);
            }
			printk("\nVote for A0 clock done\n");
			/*
			 * Configure A0 clock to be slave to
			 * WLAN_CLK_PWR_REQ
			 */
			rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_A0,
					PMAPP_CLOCK_VOTE_PIN_CTRL);
			if (rc) {
				pr_err("%s: Configuring A0 to Pin"
				" controllable failed %d\n",
						 __func__, rc);
				goto vreg_clock_vote_fail;
			}
#else

            rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_A0, PMAPP_CLOCK_VOTE_OFF);
			if (rc) {
                pr_err("%s: Configuring A0 to turn off"
                        " failed %d\n", __func__, rc);
            }
			printk("\n Vote against A0 clock done\n");
#endif

	} else {

		if(!resultFlag){
		gpio_set_value_cansleep(WLAN_GPIO_EXT_POR_N, 0);
			rc = gpio_direction_input(WLAN_GPIO_EXT_POR_N);
			if (rc) {
                pr_err("WLAN reset GPIO %d set direction failed %d\n",
		WLAN_GPIO_EXT_POR_N, rc);
		}
		gpio_free(WLAN_GPIO_EXT_POR_N);
		printk("\n vote against WLAN GPIO 134 done. \n");
		}
		rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_A0,
					PMAPP_CLOCK_VOTE_OFF);
		if (rc) {
			pr_err("%s: Configuring A0 to turn OFF"
			" failed %d\n", __func__, rc);
		}
	}

	printk("Interface %s success \n",on?"initialization":"deinitialization");
	resultFlag = 0;
	return 0;

fail_gpio_dir_out:
	gpio_free(WLAN_GPIO_EXT_POR_N);
vreg_fail:
	index--;
vreg_clock_vote_fail:
	while (index >= 0) {
		rc = regulator_disable(vreg_info[index].reg);
		if (rc) {
			pr_err("%s:%s vreg disable failed %d\n",
				__func__, vreg_info[index].vreg_id, rc);
		}
		index--;
	}
fail:
	resultFlag = 1;
        printk("Interface %s failed \n",on?"initialization":"deinitialization");
	return 0;
}

#else
/* replace with plaftform specific changes */
#endif

static int ath6kl_pm_probe(struct platform_device *pdev)
{
#ifdef CONFIG_ARCH_MSM7X27A
    msm7x27a_wifi_power(1);
#else
    /* replace with plaftform specific changes */
#endif

    return 0;
}

static int ath6kl_pm_remove(struct platform_device *pdev)
{
#ifdef CONFIG_ARCH_MSM7X27A
    msm7x27a_wifi_power(0);
#else
    /* replace with plaftform specific changes */
#endif

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

void __init ath6kl_sdio_init_platform(void)
{
	char buf[3];
	int length;

	platform_driver_register(&ath6kl_pm_device);

#ifdef CONFIG_ARCH_MSM7X27A
	length = snprintf(buf, sizeof(buf), "%d\n", 1 ? 1 : 0);
	android_readwrite_file("/sys/devices/platform/" MMC_MSM_DEV "/polling", NULL, buf, length);
	length = snprintf(buf, sizeof(buf), "%d\n", 0 ? 1 : 0);
	android_readwrite_file("/sys/devices/platform/" MMC_MSM_DEV "/polling", NULL, buf, length);

	A_MDELAY(50);
#else
	/* replace with plaftform specific changes */
#endif
}

void __exit ath6kl_sdio_exit_platform(void)
{
	char buf[3];
	int length;
	platform_driver_unregister(&ath6kl_pm_device);

#ifdef CONFIG_ARCH_MSM7X27A
	length = snprintf(buf, sizeof(buf), "%d\n", 1 ? 1 : 0);
	/* fall back to polling */
	android_readwrite_file("/sys/devices/platform/" MMC_MSM_DEV "/polling", NULL, buf, length);
	length = snprintf(buf, sizeof(buf), "%d\n", 0 ? 1 : 0);
	/* fall back to polling */
	android_readwrite_file("/sys/devices/platform/" MMC_MSM_DEV "/polling", NULL, buf, length);

	A_MDELAY(1000);
#else
	/* replace with plaftform specific changes */
#endif
}
