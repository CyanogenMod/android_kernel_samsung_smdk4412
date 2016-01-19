/*
 * Samsung Exynos4 SoC series FIMC-IS slave interface driver
 *
 * Command list
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 * Contact: Younghwan Joo, <yhwan.joo@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_CMD_H_
#define FIMC_IS_CMD_H_

#define IS_COMMAND_VER 110 /* IS COMMAND VERSION 1.10 */

enum is_cmd {
	/* HOST -> IS */
	HIC_PREVIEW_STILL = 0x1,
	HIC_PREVIEW_VIDEO,
	HIC_CAPTURE_STILL,
	HIC_CAPTURE_VIDEO,
	HIC_STREAM_ON,
	HIC_STREAM_OFF,
	HIC_SET_PARAMETER,
	HIC_GET_PARAMETER,
	HIC_SET_TUNE,
	RESERVED1,
	HIC_GET_STATUS,
	/* SENSOR PART*/
	HIC_OPEN_SENSOR,
	HIC_CLOSE_SENSOR,
	HIC_SIMMIAN_INIT,
	HIC_SIMMIAN_WRITE,
	HIC_SIMMIAN_READ,
	HIC_POWER_DOWN,
	HIC_GET_SET_FILE_ADDR,
	HIC_LOAD_SET_FILE,
	HIC_MSG_CONFIG,
	HIC_MSG_TEST,
	/* IS -> HOST */
	IHC_GET_SENSOR_NUMBER = 0x1000,
	IHC_SET_SHOT_MARK,
	/* PARAM1 : a frame number */
	/* PARAM2 : confidence level(smile 0~100) */
	/* PARMA3 : confidence level(blink 0~100) */
	IHC_SET_FACE_MARK,
	/* PARAM1 : coordinate count */
	/* PARAM2 : coordinate buffer address */
	IHC_FRAME_DONE,
	/* PARAM1 : frame start number */
	/* PARAM2 : frame count */
	IHC_AA_DONE,
	IHC_NOT_READY
};

enum is_reply {
	ISR_DONE	= 0x2000,
	ISR_NDONE
};

enum is_scenario_id {
	ISS_PREVIEW_STILL,
	ISS_PREVIEW_VIDEO,
	ISS_CAPTURE_STILL,
	ISS_CAPTURE_VIDEO,
	ISS_END
};

enum is_sub_scenario_id {
	ISS_SUB_SCENARIO_DEFAULT = 0,
	ISS_SUB_PS_VTCALL = 1,
	ISS_SUB_CS_VTCALL = 2,
	ISS_SUB_PV_VTCALL = 3,
	ISS_SUB_CV_VTCALL = 4,
	ISS_SUB_END
};

struct is_get_capability {
	u32 support_af;
	u32 iso_gain;
	u32 aperture;
	u32 min_exposure;
	u32 max_exposure;
	u32 min_gain;
	u32 max_gain;
};

#define HOST_SET_INT_BIT	0x00000001
#define HOST_CLR_INT_BIT	0x00000001
#define IS_SET_INT_BIT		0x00000001
#define IS_CLR_INT_BIT		0x00000001

#define HOST_SET_INTERRUPT(base)	(base->uiINTGR0 |= HOST_SET_INT_BIT)
#define HOST_CLR_INTERRUPT(base)	(base->uiINTCR0 |= HOST_CLR_INT_BIT)
#define IS_SET_INTERRUPT(base)		(base->uiINTGR1 |= IS_SET_INT_BIT)
#define IS_CLR_INTERRUPT(base)		(base->uiINTCR1 |= IS_CLR_INT_BIT)

struct is_common_reg {
	u32 hicmd;
	u32 hic_sensorid;
	u32 hic_param1;
	u32 hic_param2;
	u32 hic_param3;
	u32 hic_param4;

	u32 reserved1[4];

	u32 ihcmd;
	u32 ihc_sensorid;
	u32 ihc_param1;
	u32 ihc_param2;
	u32 ihc_param3;
	u32 ihc_param4;

	u32 reserved2[4];

	u32 isp_sensor_id;
	u32 isp_param1;
	u32 isp_param2;
	u32 reserved3[1];

	u32 scc_sensor_id;
	u32 scc_param1;
	u32 scc_param2;
	u32 reserved4[1];

	u32 dnr_sensor_id;
	u32 dnr_param1;
	u32 dnr_param2;
	u32 reserved5[1];

	u32 scp_sensor_id;
	u32 scp_param1;
	u32 scp_param2;
	u32 reserved6[29];
};

struct is_mcuctl_reg {
	u32 mcuctl;
	u32 bboar;

	u32 intgr0;
	u32 intcr0;
	u32 intmr0;
	u32 intsr0;
	u32 intmsr0;

	u32 intgr1;
	u32 intcr1;
	u32 intmr1;
	u32 intsr1;
	u32 intmsr1;

	u32 intcr2;
	u32 intmr2;
	u32 intsr2;
	u32 intmsr2;

	u32 gpoctrl;
	u32 cpoenctlr;
	u32 gpictlr;

	u32 pad[0xD];

	struct is_common_reg common_reg;
};
#endif
