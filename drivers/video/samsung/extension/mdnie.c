/* /linux/driver/video/samsung/mdnie.c
 *
 * Samsung SoC mDNIe driver.
 *
 * Copyright (c) 2011 Samsung Electronics.
 * Author: InKi Dae  <inki.dae@samsung.com>
 *		Eunchul Kim <chulspro.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>

#include <plat/fimd_lite_ext.h>
#include <mach/map.h>

#include "regs-mdnie.h"
#include "mdnie.h"
#include "s5p_fimd_ext.h"
#include "cmc623.h"

/* FIXME:!! need to change chip id dynamically */
#define MDNIE_CHIP_ID "cmc623p"
#ifdef CONFIG_SLP_DISP_DEBUG
#define MDNIE_MAX_REG	128
#define MDNIE_BASE_REG	0x11CA0000
#endif

static const char *mode_name[MODE_MAX] = {
	"dynamic",
	"standard",
	"natural",
	"movie"
};

static const char *scenario_name[SCENARIO_MAX] = {
	"ui",
	"gallery",
	"video",
	"vtcall",
	"camera",
	"browser",
	"negative",
	"bypass"
};

static const char *tone_name[TONE_MAX] = {
	"",
	"warm",
	"cold",
};

static const char *tone_browser_name[TONE_BR_MAX] = {
	"tone1",
	"tone2",
	"tone3"
};

static const char *outdoor_name[OUTDOOR_MAX] = {
	"",
	"outdoor"
};

#if defined(DEBUG)
#define mdnie_info(fmt, args...) \
	do { \
		printk(KERN_NOTICE"%s:"fmt"\n", __func__, ##args); \
	} while (0);
#else
#define mdnie_info(fmt, args...)
#endif

static struct mdnie_platform_data
	*to_mdnie_platform_data(struct s5p_fimd_ext_device *fx_dev)
{
	return fx_dev->dev.platform_data ?
		(struct mdnie_platform_data *)fx_dev->dev.platform_data : NULL;
}

static void mdnie_write_tune(struct s5p_mdnie *mdnie,
					const unsigned short *tune,
					unsigned int size)
{
	unsigned int i = 0;

	mdnie_info("size[%d]", size);
	while (i < size) {
		writel(tune[i + 1], mdnie->regs + tune[i] * 4);
		i += 2;
	}
}

static void mdnie_wr_tune_dat(struct s5p_mdnie *mdnie, const u8 *data)
{
	unsigned int val1 = 0, val2 = 0;
	int ret;
	char *str = NULL;

	while ((str = strsep((char **)&data, "\n"))) {
		ret = sscanf(str, "0x%x,0x%x,\n", &val1, &val2);
		if (ret == 2)
			writel((u16)val2, mdnie->regs+(u16)val1*4);
	}
}

static int mdnie_request_fw(struct s5p_mdnie *mdnie, const char *name)
{
	const struct firmware *fw;
	char fw_path[MDNIE_MAX_STR+1];
	int ret;

	snprintf(fw_path, MDNIE_MAX_STR, MDNIE_FW_PATH, MDNIE_CHIP_ID, name);
	mdnie_info("fw_path[%s]", fw_path);
	mutex_lock(&mdnie->lock);

	ret = request_firmware(&fw, fw_path, mdnie->dev);
	if (ret) {
		dev_err(mdnie->dev, "failed to request firmware.\n");
		mutex_unlock(&mdnie->lock);
		return ret;
	}

	mdnie_wr_tune_dat(mdnie, fw->data);
	release_firmware(fw);

	mutex_unlock(&mdnie->lock);

	return 0;
}

static int mdnie_request_tables(struct s5p_mdnie *mdnie, const char *name)
{
	struct mdnie_tables *tables;
	int i;

	mdnie_info("name[%s]", name);
	tables = (struct mdnie_tables *)&mdnie_main_tables;
	if (!tables) {
		dev_err(mdnie->dev, "mode mdnie tables is NULL.\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(mdnie_main_tables); i++) {
		if (strcmp(name , tables[i].name) == 0) {
			mdnie_info("tune_id[%d]name[%s]", i, tables[i].name);
			mdnie_write_tune(mdnie, tables[i].value,
						tables[i].size);
			break;
		}
	}

	return 0;
}

static int mdnie_get_name(struct s5p_mdnie *mdnie, char *name,
			enum mdnie_set set)
{
	enum mdnie_mode mode = mdnie->mode;
	enum mdnie_scenario scenario = mdnie->scenario;
	int tone = mdnie->tone;
	enum mdnie_outdoor outdoor = mdnie->outdoor;

	mdnie_info("set[%d]:mode[%d]scenario[%d]tone[%d]outdoor[%d]",
			set, mode, scenario, tone, outdoor);
	switch (scenario) {
	case SCENARIO_CAMERA:
		strcat(name, scenario_name[scenario]);
		if (outdoor == OUTDOOR_ON) {
			strcat(name, "_");
			strcat(name , outdoor_name[outdoor]);
		}
		break;
	case SCENARIO_BROWSER:
		strcat(name, scenario_name[scenario]);
		strcat(name, "_");
		strcat(name, tone_browser_name[tone]);
		break;
	default:
		if (set == SET_MAIN ||
		(tone == TONE_NORMAL && outdoor == OUTDOOR_OFF)) {
			if (scenario < SCENARIO_MODE_MAX) {
				if (mode < MODE_MAX)
					strcat(name, mode_name[mode]);

				strcat(name, "_");

				if (scenario < SCENARIO_MODE_MAX)
					strcat(name,
						scenario_name[scenario]);
			} else
				strcat(name, scenario_name[scenario]);
		} else {
			if (tone < TONE_MAX)
				strcat(name, tone_name[tone]);

			if (tone != TONE_NORMAL && outdoor == OUTDOOR_ON)
				strcat(name, "_");

			if (outdoor < OUTDOOR_MAX)
				strcat(name , outdoor_name[outdoor]);
		}
		break;
	}

	return strlen(name);
}

static int mdnie_set_commit(struct s5p_mdnie *mdnie,
					enum mdnie_set set)
{
	struct mdnie_manager_ops *mops = mdnie->mops;
	char name[MDNIE_MAX_STR+1];
	int ret = 0, len = 0;

	memset(name, 0, MDNIE_MAX_STR+1);
	len = mdnie_get_name(mdnie, name, set);
	if (len == 0) {
		dev_err(mdnie->dev, "failed to find tune.\n");
		return ret;
	}

	if (mops && mops->tune) {
		ret = mops->tune(mdnie, name);
		if (ret) {
			dev_err(mdnie->dev, "failed to set tune.\n");
			return ret;
		}
	} else {
		dev_err(mdnie->dev, "invalid mdnie mops.\n");
		return -EINVAL;
	}

	return ret;
}

static int mdnie_check_tone(struct s5p_mdnie *mdnie, int tone)
{
	unsigned long max_tone;

	if (mdnie->scenario == SCENARIO_BROWSER)
		max_tone = TONE_BR_MAX;
	else
		max_tone = TONE_MAX;

	if (tone >= max_tone) {
		dev_err(mdnie->dev, "invalid mdnie tone.\n");
		return -EINVAL;
	}

	if (mdnie->scenario != SCENARIO_VIDEO
		&& mdnie->scenario != SCENARIO_BROWSER) {
		/* Set to default */
		mdnie->tone = 0;
		dev_err(mdnie->dev, "invalid mdnie scenario.\n");
		return -EIO;
	}

	return 0;
}

struct mdnie_manager_ops mdnie_set_mops = {
	.tune = mdnie_request_tables,
	.commit = mdnie_set_commit,
	.check_tone = mdnie_check_tone,
};

static void mdnie_set_size(struct s5p_mdnie *mdnie,
		unsigned int width, unsigned int height)
{
	unsigned int size;

	writel(0x0, mdnie->regs + MDNIE_R0);

	/* Input Data Unmask */
	writel(0x077, mdnie->regs + MDNIE_R1);

	/* LCD width */
	size = readl(mdnie->regs + MDNIE_R3);
	size &= ~MDNIE_R34_WIDTH_MASK;
	size |= width;
	writel(size, mdnie->regs + MDNIE_R3);

	/* LCD height */
	size = readl(mdnie->regs + MDNIE_R4);
	size &= ~MDNIE_R35_HEIGHT_MASK;
	size |= height;
	writel(size, mdnie->regs + MDNIE_R4);

	/* unmask all */
	writel(0, mdnie->regs + MDNIE_R28);
}

static void mdnie_init_hardware(struct s5p_mdnie *mdnie)
{
	struct mdnie_platform_data *pdata = NULL;

	pdata = mdnie->pdata;

	/* set display size. */
	mdnie_set_size(mdnie, pdata->width, pdata->height);
}

static int mdnie_setup(struct s5p_fimd_ext_device *fx_dev, unsigned int enable)
{
	struct s5p_mdnie *mdnie = fimd_ext_get_drvdata(fx_dev);
	struct mdnie_manager_ops *mops = mdnie->mops;
	int ret = 0;

	mdnie_info("enable[%d]", enable);
	if (enable) {
		int i;
		mdnie_init_hardware(mdnie);
		if (mops && mops->commit) {
			for (i = 0; i < SET_MAX; i++) {
				ret = mops->commit(mdnie, i);
				if (ret) {
					dev_err(mdnie->dev, "invalid mdnie set.\n");
					goto error;
				}
			}
		} else {
			dev_err(mdnie->dev, "invalid mdnie mops.\n");
			goto error;
		}
	}

	return 0;

error:
	return ret;
}

static ssize_t store_mdnie_mode(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	struct s5p_mdnie *mdnie = fimd_ext_get_drvdata(fx_dev);
	struct mdnie_manager_ops *mops = mdnie->mops;
	unsigned long prev_mode;
	unsigned long mode;
	int ret;

	ret = kstrtoul(buf, 0, &mode);
	if (ret) {
		dev_err(&fx_dev->dev, "invalid mode value.\n");
		return -EINVAL;
	}

	if (mode >= MODE_MAX) {
		dev_err(&fx_dev->dev, "invalid mdnie mode.\n");
		return -EINVAL;
	}

	if (mdnie->scenario >= SCENARIO_MODE_MAX) {
		dev_err(&fx_dev->dev, "invalid mdnie scenario.\n");
		mdnie->scenario = SCENARIO_UI;
	}

	prev_mode = mdnie->mode;
	mdnie->mode = mode;
	mdnie_info("mode[%d]", mdnie->mode);
	if (mops && mops->commit) {
		ret = mops->commit(mdnie, SET_MAIN);
		if (ret) {
			dev_err(&fx_dev->dev, "failed to set mode.\n");
			goto error_restore;
		}
	} else {
		dev_err(&fx_dev->dev, "invalid mdnie mops.\n");
		ret = -EINVAL;
		goto error_restore;
	}

	return count;

error_restore:
	mdnie->mode = prev_mode;
	return ret;
}

static ssize_t show_mdnie_mode(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	struct s5p_mdnie *mdnie = fimd_ext_get_drvdata(fx_dev);

	mdnie_info("mode[%d]", mdnie->mode);
	return snprintf(buf, PAGE_SIZE, "%d\n", mdnie->mode);
}

static ssize_t store_mdnie_scenario(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	struct s5p_mdnie *mdnie = fimd_ext_get_drvdata(fx_dev);
	struct mdnie_manager_ops *mops = mdnie->mops;
	unsigned long prev_scenario;
	unsigned long scenario;
	int ret;

	ret = kstrtoul(buf, 0, &scenario);
	if (ret) {
		dev_err(&fx_dev->dev, "invalid scenario value.\n");
		return -EINVAL;
	}

	if (scenario >= SCENARIO_MAX) {
		dev_err(&fx_dev->dev, "invalid mdnie scenario.\n");
		return -EINVAL;
	}

	prev_scenario = mdnie->scenario;
	mdnie->scenario = scenario;
	mdnie_info("scenario[%d]", mdnie->scenario);
	if (mops && mops->commit) {
		ret = mops->commit(mdnie, SET_MAIN);
		if (ret) {
			dev_err(&fx_dev->dev, "failed to set scenario.\n");
			goto error_restore;
		}
	} else {
		dev_err(&fx_dev->dev, "invalid mdnie mops.\n");
		ret = -EINVAL;
		goto error_restore;
	}

	return count;

error_restore:
	mdnie->scenario = prev_scenario;
	return ret;
}

static ssize_t show_mdnie_scenario(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	struct s5p_mdnie *mdnie = fimd_ext_get_drvdata(fx_dev);

	mdnie_info("scenario[%d]", mdnie->scenario);
	return snprintf(buf, PAGE_SIZE, "%d\n", mdnie->scenario);
}

static ssize_t store_mdnie_tone(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	struct s5p_mdnie *mdnie = fimd_ext_get_drvdata(fx_dev);
	struct mdnie_manager_ops *mops = mdnie->mops;
	unsigned long prev_tone;
	unsigned long tone;
	int ret;

	ret = kstrtoul(buf, 0, &tone);
	if (ret) {
		dev_err(&fx_dev->dev, "invalid tone value.\n");
		return -EINVAL;
	}

	if (mops && mops->check_tone) {
		ret = mops->check_tone(mdnie, tone);
		if (ret) {
			dev_err(&fx_dev->dev, "failed to set tone.\n");
			return ret;
		}
	} else {
		dev_err(&fx_dev->dev, "invalid mdnie mops.\n");
		ret = -EINVAL;
		goto error_mops;
	}

	prev_tone = mdnie->tone;
	mdnie->tone = tone;
	mdnie_info("tone[%d]", mdnie->tone);
	if (mops && mops->commit) {
		ret = mops->commit(mdnie, SET_OPTIONAL);
		if (ret) {
			dev_err(&fx_dev->dev, "failed to set tone.\n");
			goto error_restore;
		}
	} else {
		dev_err(&fx_dev->dev, "invalid mdnie mops.\n");
		ret = -EINVAL;
		goto error_restore;
	}

	return count;

error_restore:
	mdnie->tone = prev_tone;
error_mops:
	return ret;
}

static ssize_t show_mdnie_tone(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	struct s5p_mdnie *mdnie = fimd_ext_get_drvdata(fx_dev);

	mdnie_info("tone[%d]", mdnie->tone);
	return snprintf(buf, PAGE_SIZE, "%d\n", mdnie->tone);
}

static ssize_t store_mdnie_outdoor(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	struct s5p_mdnie *mdnie = fimd_ext_get_drvdata(fx_dev);
	struct mdnie_manager_ops *mops = mdnie->mops;
	unsigned long prev_outdoor;
	unsigned long outdoor;
	int ret;

	ret = kstrtoul(buf, 0, &outdoor);
	if (ret) {
		dev_err(&fx_dev->dev, "invalid outdoor value.\n");
		return -EINVAL;
	}

	if (outdoor >= OUTDOOR_MAX) {
		dev_err(&fx_dev->dev, "invalid mdnie outdoor.\n");
		return -EINVAL;
	}

	if (mdnie->scenario != SCENARIO_VIDEO
		&& mdnie->scenario != SCENARIO_CAMERA) {
		/* Set to default */
		mdnie->outdoor = OUTDOOR_OFF;
		dev_err(&fx_dev->dev, "invalid mdnie scenario.\n");
		return -EIO;
	}

	prev_outdoor = mdnie->outdoor;
	mdnie->outdoor = outdoor;
	mdnie_info("outdoor[%d]", mdnie->outdoor);
	if (mops && mops->commit) {
		ret = mops->commit(mdnie, SET_OPTIONAL);
		if (ret) {
			dev_err(&fx_dev->dev, "failed to set outdoor.\n");
			goto error_restore;
		}
	} else {
		dev_err(&fx_dev->dev, "invalid mdnie mops.\n");
		ret = -EINVAL;
		goto error_restore;
	}

	return count;

error_restore:
	mdnie->outdoor = prev_outdoor;
	return ret;
}

static ssize_t show_mdnie_outdoor(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	struct s5p_mdnie *mdnie = fimd_ext_get_drvdata(fx_dev);

	mdnie_info("outdoor[%d]", mdnie->outdoor);
	return snprintf(buf, PAGE_SIZE, "%d\n", mdnie->outdoor);
}

static ssize_t store_mdnie_tune(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	struct s5p_mdnie *mdnie = fimd_ext_get_drvdata(fx_dev);
	struct mdnie_manager_ops *mops = mdnie->mops;
	unsigned long prev_tune;
	unsigned long tune;
	int ret;

	ret = kstrtoul(buf, 0, &tune);
	if (ret) {
		dev_err(&fx_dev->dev, "invalid tune value.\n");
		return -EINVAL;
	}

	if (tune >= TUNE_MAX) {
		dev_err(&fx_dev->dev, "invalid mdnie tune.\n");
		return -EINVAL;
	}

	prev_tune = mdnie->tune;
	mdnie->tune = tune;
	mdnie_info("tune[%d]", mdnie->tune);
	if (mops) {
		if (mdnie->tune == TUNE_FW)
			mops->tune = mdnie_request_fw;
		else
			mops->tune = mdnie_request_tables;
	} else {
		dev_err(&fx_dev->dev, "invalid mdnie mops.\n");
		ret = -EINVAL;
		goto error_restore;
	}

	return count;

error_restore:
	mdnie->tune = prev_tune;
	return ret;
}

static ssize_t show_mdnie_tune(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	struct s5p_mdnie *mdnie = fimd_ext_get_drvdata(fx_dev);

	mdnie_info("tune[%d]", mdnie->tune);
	return snprintf(buf, PAGE_SIZE, "%d\n", mdnie->tune);
}

#ifdef CONFIG_SLP_DISP_DEBUG
static int mdnie_read_reg(struct s5p_mdnie *mdnie, char *buf)
{
	u32 cfg;
	int i;
	int pos = 0;

	pos += sprintf(buf+pos, "0x%.8x | ", MDNIE_BASE_REG);
	for (i = 1; i < MDNIE_MAX_REG + 1; i++) {
		cfg = readl(mdnie->regs + ((i-1) * sizeof(u32)));
		pos += sprintf(buf+pos, "0x%.8x ", cfg);
		if (i % 4 == 0)
			pos += sprintf(buf+pos, "\n0x%.8x | ",
				MDNIE_BASE_REG + (i * sizeof(u32)));
	}

	return pos;
}

static ssize_t show_read_reg(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	struct s5p_mdnie *mdnie = fimd_ext_get_drvdata(fx_dev);

	if (!mdnie->regs) {
		dev_err(dev, "failed to get current register.\n");
		return -EINVAL;
	}

	return mdnie_read_reg(mdnie, buf);
}
#endif

/* sys/devices/platform/mdnie */
static struct device_attribute mdnie_device_attrs[] = {
	__ATTR(mode, S_IRUGO|S_IWUSR, show_mdnie_mode,
							store_mdnie_mode),
	__ATTR(scenario, S_IRUGO|S_IWUSR, show_mdnie_scenario,
							store_mdnie_scenario),
	__ATTR(tone, S_IRUGO|S_IWUSR, show_mdnie_tone,
							store_mdnie_tone),
	__ATTR(outdoor, S_IRUGO|S_IWUSR, show_mdnie_outdoor,
							store_mdnie_outdoor),
	__ATTR(tune, S_IRUGO|S_IWUSR, show_mdnie_tune,
							store_mdnie_tune),
#ifdef CONFIG_SLP_DISP_DEBUG
	__ATTR(read_reg, S_IRUGO, show_read_reg, NULL),
#endif
};

static int mdnie_probe(struct s5p_fimd_ext_device *fx_dev)
{
	struct resource *res;
	struct s5p_mdnie *mdnie;
	int ret = -EINVAL;
	int i;

	mdnie = kzalloc(sizeof(struct s5p_mdnie), GFP_KERNEL);
	if (!mdnie) {
		dev_err(&fx_dev->dev, "failed to alloc mdnie object.\n");
		return -EFAULT;
	}

	mdnie->dev = &fx_dev->dev;

	mdnie->pdata = to_mdnie_platform_data(fx_dev);
	if (mdnie->pdata == NULL) {
		dev_err(&fx_dev->dev, "platform_data is NULL.\n");
		return -EFAULT;
	}

	res = s5p_fimd_ext_get_resource(fx_dev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&fx_dev->dev, "failed to get io memory region.\n");
		return -EINVAL;
	}

	mdnie->regs = ioremap(res->start, resource_size(res));
	if (!mdnie->regs) {
		dev_err(&fx_dev->dev, "failed to remap io region.\n'");
		return -EFAULT;
	}

	mdnie->mode = MODE_STANDARD;
	mdnie->scenario = SCENARIO_UI;
	mdnie->tone = TONE_NORMAL;
	mdnie->outdoor = OUTDOOR_OFF;
	mdnie->tune = TUNE_TBL;
	mdnie->mops = &mdnie_set_mops;

	mutex_init(&mdnie->lock);

	fimd_ext_set_drvdata(fx_dev, mdnie);

	for (i = 0; i < ARRAY_SIZE(mdnie_device_attrs); i++) {
		ret = device_create_file(&fx_dev->dev,
					&mdnie_device_attrs[i]);
		if (ret)
			break;
	}

	if (ret < 0)
		dev_err(&fx_dev->dev, "failed to add sysfs entries\n");

	dev_info(&fx_dev->dev, "mDNIe driver has been probed.\n");

	return 0;
}

static struct s5p_fimd_ext_driver fimd_ext_driver = {
	.driver		= {
		.name	= "mdnie",
		.owner	= THIS_MODULE,
	},
	.probe		= mdnie_probe,
	.setup		= mdnie_setup,
};

static int __init mdnie_init(void)
{
	return s5p_fimd_ext_driver_register(&fimd_ext_driver);
}

static void __exit mdnie_exit(void)
{
}

arch_initcall(mdnie_init);
module_exit(mdnie_exit);

MODULE_AUTHOR("InKi Dae <inki.dae@samsung.com>");
MODULE_AUTHOR("Eunchul Kim <chulspro.kim@samsung.com>");
MODULE_DESCRIPTION("mDNIe Driver");
MODULE_LICENSE("GPL");

