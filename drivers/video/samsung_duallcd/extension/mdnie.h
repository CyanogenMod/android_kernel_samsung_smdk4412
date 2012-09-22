/* linux/arch/arm/plat-s5p/mdnie.h
 *
 * mDNIe Platform Specific Header Definitions.
 *
 * Copyright (c) 2011 Samsung Electronics
 * InKi Dae <inki.dae@samsung.com>
 * Eunchul Kim <chulspro.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _MDNIE_H_
#define _MDNIE_H_

#define MDNIE_MAX_STR 255
#define MDNIE_FW_PATH "mdnie/%s/%s.bin"

/* set - main, optional */
enum mdnie_set {
	SET_MAIN = 0,
	SET_OPTIONAL,
	SET_MAX
};

/* mode - dynamic, standard, natural, movie */
enum mdnie_mode {
	MODE_DYNAMIC = 0,
	MODE_STANDARD,
	MODE_NATURAL,
	MODE_MOVIE,
	MODE_MAX
};

/* scenario - ui, gallery, video, vtcall, camera, browser, negative, bypass */
enum mdnie_scenario {
	SCENARIO_UI = 0,
	SCENARIO_GALLERY,
	SCENARIO_VIDEO,
	SCENARIO_VTCALL,
	SCENARIO_MODE_MAX,
	SCENARIO_CAMERA = SCENARIO_MODE_MAX,
	SCENARIO_BROWSER,
	SCENARIO_NEGATIVE,
	SCENARIO_BYPASS,
	SCENARIO_MAX
};

/* tone - normal, warm, cold */
enum mdnie_tone {
	TONE_NORMAL = 0,
	TONE_WARM,
	TONE_COLD,
	TONE_MAX
};

/* tone browser - tone1, tone2, tone3 */
enum mdnie_tone_br {
	TONE_1 = 0,
	TONE_2,
	TONE_3,
	TONE_BR_MAX
};

/* outdoor - off, on */
enum mdnie_outdoor {
	OUTDOOR_OFF = 0,
	OUTDOOR_ON,
	OUTDOOR_MAX
};

/* tune - tables, fw */
enum mdnie_tune {
	TUNE_TBL = 0,
	TUNE_FW,
	TUNE_MAX
};

/*
 * A main structure for mDNIe.
 *
 * @dev: pointer to device object for sysfs
 * @regs:  memory mapped register map
 * @mode: mdnie mode value
 * @scenario: mdnie scenario value
 * @tone: mdnie tone value
 * @outdoor: mdnie outdoor value
 * @lock: lock for request firmware waiting
 * @pdata: platform data of width, height
 * @mops: manager ops
 */
struct s5p_mdnie {
	struct device			*dev;
	void __iomem			*regs;

	enum mdnie_mode		mode;
	enum mdnie_scenario	scenario;
	int					tone;
	enum mdnie_outdoor	outdoor;
	enum mdnie_tune		tune;

	struct mutex			lock;
	struct mdnie_platform_data	*pdata;
	struct mdnie_manager_ops *mops;
};

/**
 * A structure for data tables.
 *
 * @name: table name
 * @value: table value
 * @size: table size
 */
struct mdnie_tables {
	const char			*name;
	const unsigned short	*value;
	unsigned int			size;
};

/*
 * mDNIe manager ops.
 *
 * @tune: api of tune settings
 * @commit: api of main,optional settings
 * @check_tone: api of check tone
 */
struct mdnie_manager_ops {
	int (*tune)(struct s5p_mdnie *mdnie, const char *name);
	int (*commit)(struct s5p_mdnie *mdnie, enum mdnie_set set);
	int (*check_tone)(struct s5p_mdnie *mdnie, int tone);
};

#endif /* _MDNIE_H_ */
