/* drivers/media/video/sr300pc20.c
 *
 * Copyright (c) 2010, Samsung Electronics. All rights reserved
 * Author: dongseong.lim
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * - change date: 2012.06.28
 */
#include "sr300pc20.h"
#include <linux/gpio.h>

#ifdef CONFIG_VIDEO_IMPROVE_STREAMOFF
#undef CONFIG_VIDEO_IMPROVE_STREAMOFF
#endif

#define i2c_read_nop() \
	(cam_err("error, not used read function, line %d\n", __LINE__))
#define i2c_write_nop() \
	(cam_err("error, not used write function, line %d\n", __LINE__))

#define isx012_readb(sd, addr, data)	i2c_read_nop()
#define isx012_writeb(sd, addr, data)	i2c_write_nop()

#define sr300pc20_readb(sd, addr, data) sr300pc20_i2c_read(sd, addr, data)
#define sr300pc20_readw(sd, addr, data)	i2c_read_nop()
#define sr300pc20_readl(sd, addr, data)	i2c_read_nop()
#define sr300pc20_writeb(sd, addr, data) sr300pc20_i2c_write(sd, addr, data)
#define sr300pc20_writew(sd, addr, data) i2c_write_nop()
#define sr300pc20_writel(sd, addr, data) i2c_write_nop()
#define sr300pc20_wait_ae_stable_af(sd)	sr300pc20_wait_ae_stable(sd, true, false)
#define sr300pc20_wait_ae_stable_preview(sd) sr300pc20_wait_ae_stable(sd, false, true)
#define sr300pc20_wait_ae_stable_cap(sd)	sr300pc20_wait_ae_stable(sd, false, false)

static int dbg_level;

static const struct sr300pc20_fps sr300pc20_framerates[] = {
	{ I_FPS_0,	FRAME_RATE_AUTO },
	{ I_FPS_7,	FRAME_RATE_7},
	{ I_FPS_15,	FRAME_RATE_15 },
	{ I_FPS_25,	FRAME_RATE_25 },
	{ I_FPS_30,	FRAME_RATE_30 },
};

static const struct sr300pc20_framesize sr300pc20_preview_frmsizes[] = {
	/*{ PREVIEW_SZ_QCIF,	176,  144 },
	{ PREVIEW_SZ_CIF,	352,  288 },
	{ PREVIEW_SZ_528x432,	528,  432 }, */
	{ PREVIEW_SZ_VGA,	640,  480 },
};

static const struct sr300pc20_framesize sr300pc20_capture_frmsizes[] = {
	{ CAPTURE_SZ_VGA,	640,  480 },
/*	{ CAPTURE_SZ_960_720,	960,  720 },
	{ CAPTURE_SZ_W1MP,	1536, 864 },
	{ CAPTURE_SZ_2MP,	1600, 1200 },
	{ CAPTURE_SZ_W2MP,	2048, 1152 },
	{ CAPTURE_SZ_3MP,	2048, 1536 },
	{ CAPTURE_SZ_W4MP,	2560, 1440 },
	{ CAPTURE_SZ_5MP,	2560, 1920 }, */
};

static struct sr300pc20_control sr300pc20_ctrls[] = {
	SR300PC20_INIT_CONTROL(V4L2_CID_CAMERA_FLASH_MODE, \
					FLASH_MODE_OFF),

	SR300PC20_INIT_CONTROL(V4L2_CID_CAMERA_BRIGHTNESS, \
					EV_DEFAULT),

	SR300PC20_INIT_CONTROL(V4L2_CID_CAMERA_METERING, \
					METERING_MATRIX),

	SR300PC20_INIT_CONTROL(V4L2_CID_CAMERA_WHITE_BALANCE, \
					WHITE_BALANCE_AUTO),

	SR300PC20_INIT_CONTROL(V4L2_CID_CAMERA_EFFECT, \
					IMAGE_EFFECT_NONE),
};

static const struct sr300pc20_regs reg_datas = {
#if 0 /* dslim */
	.ev = {
		SR300PC20_REGSET(GET_EV_INDEX(EV_MINUS_4),
				SR300PC20_ExpSetting_M4Step, 0),
		SR300PC20_REGSET(GET_EV_INDEX(EV_MINUS_3),
				SR300PC20_ExpSetting_M3Step, 0),
		SR300PC20_REGSET(GET_EV_INDEX(EV_MINUS_2),
				SR300PC20_ExpSetting_M2Step, 0),
		SR300PC20_REGSET(GET_EV_INDEX(EV_MINUS_1),
				SR300PC20_ExpSetting_M1Step, 0),
		SR300PC20_REGSET(GET_EV_INDEX(EV_DEFAULT),
				SR300PC20_ExpSetting_Default, 0),
		SR300PC20_REGSET(GET_EV_INDEX(EV_PLUS_1),
				SR300PC20_ExpSetting_P1Step, 0),
		SR300PC20_REGSET(GET_EV_INDEX(EV_PLUS_2),
				SR300PC20_ExpSetting_P2Step, 0),
		SR300PC20_REGSET(GET_EV_INDEX(EV_PLUS_3),
				SR300PC20_ExpSetting_P3Step, 0),
		SR300PC20_REGSET(GET_EV_INDEX(EV_PLUS_4),
				SR300PC20_ExpSetting_P4Step, 0),
	},
	.metering = {
		SR300PC20_REGSET(METERING_MATRIX, sr300pc20_Metering_Matrix, 0),
		SR300PC20_REGSET(METERING_CENTER, sr300pc20_Metering_Center, 0),
		SR300PC20_REGSET(METERING_SPOT, sr300pc20_Metering_Spot, 0),
	},
	.iso = {
		SR300PC20_REGSET(ISO_AUTO, sr300pc20_ISO_Auto, 0),
		SR300PC20_REGSET(ISO_50, sr300pc20_ISO_50, 0),
		SR300PC20_REGSET(ISO_100, sr300pc20_ISO_100, 0),
		SR300PC20_REGSET(ISO_200, sr300pc20_ISO_200, 0),
		SR300PC20_REGSET(ISO_400, sr300pc20_ISO_400, 0),
	},
	.effect = {
		SR300PC20_REGSET(IMAGE_EFFECT_NONE, sr300pc20_Effect_Normal, 0),
		SR300PC20_REGSET(IMAGE_EFFECT_BNW, sr300pc20_Effect_Black_White, 0),
		SR300PC20_REGSET(IMAGE_EFFECT_SEPIA, sr300pc20_Effect_Sepia, 0),
		SR300PC20_REGSET(IMAGE_EFFECT_NEGATIVE,
				SR300PC20_Effect_Negative, 0),
		SR300PC20_REGSET(IMAGE_EFFECT_SOLARIZE, sr300pc20_Effect_Solar, 0),
		SR300PC20_REGSET(IMAGE_EFFECT_SKETCH, sr300pc20_Effect_Sketch, 0),
		SR300PC20_REGSET(IMAGE_EFFECT_POINT_COLOR_3,
				sr300pc20_Effect_Pastel, 0),
	},
	.white_balance = {
		SR300PC20_REGSET(WHITE_BALANCE_AUTO, sr300pc20_WB_Auto, 0),
		SR300PC20_REGSET(WHITE_BALANCE_SUNNY, sr300pc20_WB_Sunny, 0),
		SR300PC20_REGSET(WHITE_BALANCE_CLOUDY, sr300pc20_WB_Cloudy, 0),
		SR300PC20_REGSET(WHITE_BALANCE_TUNGSTEN,
				sr300pc20_WB_Tungsten, 0),
		SR300PC20_REGSET(WHITE_BALANCE_FLUORESCENT,
				sr300pc20_WB_Fluorescent, 0),
	},
	.scene_mode = {
		SR300PC20_REGSET(SCENE_MODE_NONE, sr300pc20_Scene_Default, 0),
		SR300PC20_REGSET(SCENE_MODE_PORTRAIT, sr300pc20_Scene_Portrait, 0),
		SR300PC20_REGSET(SCENE_MODE_NIGHTSHOT, sr300pc20_Scene_Nightshot, 0),
		SR300PC20_REGSET(SCENE_MODE_BACK_LIGHT, sr300pc20_Scene_Backlight, 0),
		SR300PC20_REGSET(SCENE_MODE_LANDSCAPE, sr300pc20_Scene_Landscape, 0),
		SR300PC20_REGSET(SCENE_MODE_SPORTS, sr300pc20_Scene_Sports, 0),
		SR300PC20_REGSET(SCENE_MODE_PARTY_INDOOR,
				sr300pc20_Scene_Party_Indoor, 0),
		SR300PC20_REGSET(SCENE_MODE_BEACH_SNOW,
				sr300pc20_Scene_Beach_Snow, 0),
		SR300PC20_REGSET(SCENE_MODE_SUNSET, sr300pc20_Scene_Sunset, 0),
		SR300PC20_REGSET(SCENE_MODE_DUSK_DAWN, sr300pc20_Scene_Duskdawn, 0),
		SR300PC20_REGSET(SCENE_MODE_FALL_COLOR,
				sr300pc20_Scene_Fall_Color, 0),
		SR300PC20_REGSET(SCENE_MODE_FIREWORKS, sr300pc20_Scene_Fireworks, 0),
		SR300PC20_REGSET(SCENE_MODE_TEXT, sr300pc20_Scene_Text, 0),
		SR300PC20_REGSET(SCENE_MODE_CANDLE_LIGHT,
				sr300pc20_Scene_Candle_Light, 0),
	},
	.saturation = {
		SR300PC20_REGSET(SATURATION_MINUS_2, sr300pc20_Saturation_Minus_2, 0),
		SR300PC20_REGSET(SATURATION_MINUS_1, sr300pc20_Saturation_Minus_1, 0),
		SR300PC20_REGSET(SATURATION_DEFAULT, sr300pc20_Saturation_Default, 0),
		SR300PC20_REGSET(SATURATION_PLUS_1, sr300pc20_Saturation_Plus_1, 0),
		SR300PC20_REGSET(SATURATION_PLUS_2, sr300pc20_Saturation_Plus_2, 0),
	},
	.contrast = {
		SR300PC20_REGSET(CONTRAST_MINUS_2, sr300pc20_Contrast_Minus_2, 0),
		SR300PC20_REGSET(CONTRAST_MINUS_1, sr300pc20_Contrast_Minus_1, 0),
		SR300PC20_REGSET(CONTRAST_DEFAULT, sr300pc20_Contrast_Default, 0),
		SR300PC20_REGSET(CONTRAST_PLUS_1, sr300pc20_Contrast_Plus_1, 0),
		SR300PC20_REGSET(CONTRAST_PLUS_2, sr300pc20_Contrast_Plus_2, 0),

	},
	.sharpness = {
		SR300PC20_REGSET(SHARPNESS_MINUS_2, sr300pc20_Sharpness_Minus_2, 0),
		SR300PC20_REGSET(SHARPNESS_MINUS_1, sr300pc20_Sharpness_Minus_1, 0),
		SR300PC20_REGSET(SHARPNESS_DEFAULT, sr300pc20_Sharpness_Default, 0),
		SR300PC20_REGSET(SHARPNESS_PLUS_1, sr300pc20_Sharpness_Plus_1, 0),
		SR300PC20_REGSET(SHARPNESS_PLUS_2, sr300pc20_Sharpness_Plus_2, 0),
	},
	.fps = {
		SR300PC20_REGSET(I_FPS_0, sr300pc20_fps_auto, 0),
		SR300PC20_REGSET(I_FPS_7, sr300pc20_fps_7fix, 0),
		SR300PC20_REGSET(I_FPS_15, sr300pc20_fps_15fix, 0),
		SR300PC20_REGSET(I_FPS_25, sr300pc20_fps_25fix, 0),
		SR300PC20_REGSET(I_FPS_30, sr300pc20_fps_30fix, 0),
	},
	.preview_size = {
		SR300PC20_REGSET(PREVIEW_SZ_320x240, sr300pc20_320_Preview, 0),
#if defined(CONFIG_MACH_P4NOTELTE_KOR_SKT) \
	|| defined(CONFIG_MACH_P4NOTELTE_KOR_KT) \
	|| defined(CONFIG_MACH_P4NOTELTE_KOR_LGT) /*For 4G VT call in Domestic*/
		SR300PC20_REGSET(PREVIEW_SZ_VERTICAL_VGA, sr300pc20_480_Preview, 0),
#endif
		SR300PC20_REGSET(PREVIEW_SZ_VGA, sr300pc20_640_Preview, 0),
		SR300PC20_REGSET(PREVIEW_SZ_D1, sr300pc20_720_Preview, 0),
		SR300PC20_REGSET(PREVIEW_SZ_XGA, sr300pc20_1024_768_Preview, 0),
		SR300PC20_REGSET(PREVIEW_SZ_PVGA, sr300pc20_1280_Preview_E, 0),
	},
	.capture_size = {
		SR300PC20_REGSET(CAPTURE_SZ_VGA, sr300pc20_VGA_Capture, 0),
		SR300PC20_REGSET(CAPTURE_SZ_960_720, sr300pc20_960_720_Capture, 0),
		SR300PC20_REGSET(CAPTURE_SZ_3MP, sr300pc20_3M_Capture, 0),
		SR300PC20_REGSET(CAPTURE_SZ_5MP, sr300pc20_5M_Capture, 0),
	},

	/* AF */
	.af_window_reset = SR300PC20_REGSET_TABLE(SR300PC20_AF_Window_Reset, 1),
	.af_winddow_set = SR300PC20_REGSET_TABLE(SR300PC20_AF_Window_Set, 0),
	.af_restart = SR300PC20_REGSET_TABLE(SR300PC20_AF_ReStart, 0),
	.af_saf_off = SR300PC20_REGSET_TABLE(SR300PC20_AF_SAF_OFF, 0),
	.af_touch_saf_off = SR300PC20_REGSET_TABLE(SR300PC20_AF_TouchSAF_OFF, 0),
	.cancel_af_macro = SR300PC20_REGSET_TABLE(SR300PC20_AF_Cancel_Macro_ON, 0),
	.cancel_af_normal = SR300PC20_REGSET_TABLE(SR300PC20_AF_Cancel_Macro_OFF, 0),
	.af_macro_mode = SR300PC20_REGSET_TABLE(SR300PC20_AF_Macro_ON, 0),
	.af_normal_mode = SR300PC20_REGSET_TABLE(SR300PC20_AF_Macro_OFF, 0),
	.af_camcorder_start = SR300PC20_REGSET_TABLE(
				SR300PC20_Camcorder_SAF_Start, 0),

	/* Flash */
	.flash_ae_line = SR300PC20_REGSET_TABLE(SR300PC20_Flash_AELINE, 0),
	.flash_on = SR300PC20_REGSET_TABLE(SR300PC20_Flash_ON, 1),
	.flash_off = SR300PC20_REGSET_TABLE(SR300PC20_Flash_OFF, 1),
	.ae_manual = SR300PC20_REGSET_TABLE(SR300PC20_ae_manual_mode, 0),
	.flash_fast_ae_awb = SR300PC20_REGSET_TABLE(SR300PC20_flash_fast_ae_awb, 0),
#endif
	.init_reg = SR300PC20_REGSET_TABLE(SR300PC20_Init_Reg, 0),

	/* Camera mode */
	.preview_mode = SR300PC20_REGSET_TABLE(sr300pc20_preview_camera_regs, 0),
#if 0 /* dslim */
	.capture_mode = SR300PC20_REGSET_TABLE(SR300PC20_Capture_Mode, 0),
	.capture_mode_night =
		SR300PC20_REGSET_TABLE(SR300PC20_Lowlux_Night_Capture_Mode, 0),
	.halfrelease_mode = SR300PC20_REGSET_TABLE(SR300PC20_Halfrelease_Mode, 0),
	.halfrelease_mode_night =
		SR300PC20_REGSET_TABLE(SR300PC20_Lowlux_night_Halfrelease_Mode, 0),
	.camcorder_on = SR300PC20_REGSET_TABLE(SR300PC20_Camcorder_Mode_ON, 1),
	.camcorder_off = SR300PC20_REGSET_TABLE(SR300PC20_Camcorder_Mode_OFF, 1),

	.lowlux_night_reset = SR300PC20_REGSET_TABLE(SR300PC20_Lowlux_Night_Reset, 0),

	.set_pll_4 = SR300PC20_REGSET_TABLE(SR300PC20_Pll_Setting_4, 1),
	.shading_0 = SR300PC20_REGSET_TABLE(SR300PC20_Shading_0, 1),
	.shading_1 = SR300PC20_REGSET_TABLE(SR300PC20_Shading_1, 1),
	.shading_2 = SR300PC20_REGSET_TABLE(SR300PC20_Shading_2, 1),
	.shading_nocal = SR300PC20_REGSET_TABLE(SR300PC20_Shading_Nocal, 1),
	.softlanding = SR300PC20_REGSET_TABLE(SR300PC20_Sensor_Off_VCM, 0),
#endif
	.stream_stop = SR300PC20_REGSET_TABLE(sr300pc20_stop_stream, 0),

#if 0 /* def CONFIG_VIDEO_SR300PC20_P8*/
	.antibanding = SR300PC20_REGSET_TABLE(SR300PC20_ANTIBANDING_REG, 0),
#endif
};

static const struct v4l2_mbus_framefmt capture_fmts[] = {
	{
		.code		= V4L2_MBUS_FMT_FIXED,
		.colorspace	= V4L2_COLORSPACE_JPEG,
	},
};

/**
 * msleep_debug: wrapper function calling proper sleep()
 * @msecs: time to be sleep (in milli-seconds unit)
 * @dbg_on: whether enable log or not.
 */
static void msleep_debug(u32 msecs, bool dbg_on)
{
	u32 delta_halfrange; /* in us unit */

	if (unlikely(!msecs))
		return;

	if (dbg_on)
		cam_dbg("delay for %dms\n", msecs);

	if (msecs <= 7)
		delta_halfrange = 100;
	else
		delta_halfrange = 300;

	if (msecs <= 20)
		usleep_range((msecs * 1000 - delta_halfrange),
			(msecs * 1000 + delta_halfrange));
	else
		msleep(msecs);
}

#ifdef CONFIG_LOAD_FILE
#define TABLE_MAX_NUM 500
static char *sr300pc20_regs_table;
static int sr300pc20_regs_table_size;
static int gtable_buf[TABLE_MAX_NUM];
static int sr300pc20_i2c_write(struct v4l2_subdev *sd,
		u16 subaddr, u32 data, u32 len);

int sr300pc20_regs_table_init(void)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret;
	mm_segment_t fs = get_fs();

	printk(KERN_DEBUG "%s %d\n", __func__, __LINE__);

	set_fs(get_ds());

	filp = filp_open("/mnt/sdcard/sr300pc20_regs.h", O_RDONLY, 0);

	if (IS_ERR_OR_NULL(filp)) {
		printk(KERN_DEBUG "file open error\n");
		return PTR_ERR(filp);
	}

	l = filp->f_path.dentry->d_inode->i_size;
	printk(KERN_DEBUG "l = %ld\n", l);
	//dp = kmalloc(l, GFP_KERNEL);
	dp = vmalloc(l);
	if (dp == NULL) {
		printk(KERN_DEBUG "Out of Memory\n");
		filp_close(filp, current->files);
	}

	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);

	if (ret != l) {
		printk(KERN_DEBUG "Failed to read file ret = %d\n", ret);
		/*kfree(dp);*/
		vfree(dp);
		filp_close(filp, current->files);
		return -EINVAL;
	}

	filp_close(filp, current->files);

	set_fs(fs);

	sr300pc20_regs_table = dp;

	sr300pc20_regs_table_size = l;

	*((sr300pc20_regs_table + sr300pc20_regs_table_size) - 1) = '\0';

	printk("sr300pc20_reg_table_init end\n");
	return 0;
}

void sr300pc20_regs_table_exit(void)
{
	printk(KERN_DEBUG "%s %d\n", __func__, __LINE__);

	if (sr300pc20_regs_table) {
		vfree(sr300pc20_regs_table);
		sr300pc20_regs_table = NULL;
	}
}

static int sr300pc20_define_table(void)
{
	char *start, *end, *reg;
	char *start_token, *reg_token, *temp;
	char reg_buf[61], temp2[61];
	char token_buf[5];
	int token_value = 0;
	int index_1 = 0, index_2 = 0, total_index;
	int len = 0, total_len = 0;

	*(reg_buf + 60) = '\0';
	*(temp2 + 60) = '\0';
	*(token_buf + 4) = '\0';
	memset(gtable_buf, 9999, TABLE_MAX_NUM);

	printk(KERN_DEBUG "sr300pc20_define_table start!\n");

	start = strstr(sr300pc20_regs_table, "aeoffset_table");
	if (!start) {
		cam_err("%s: can not find %s", __func__, "aeoffset_table");
		return -ENOENT;
	}
	end = strstr(start, "};");
	if (!end) {
		cam_err("%s: can not find %s end", __func__, "aeoffset_table");
		return -ENOENT;
	}

	/* Find table */
	index_2 = 0;
	while (1) {
		reg = strstr(start,"	");
		if ((reg == NULL) || (reg > end)) {
			printk(KERN_DEBUG "sr300pc20_define_table read end!\n");
			break;
		}

		/* length cal */
		index_1 = 0;
		total_len = 0;
		temp = reg;
		if (temp != NULL) {
			memcpy(temp2, (temp + 1), 60);
			//printk(KERN_DEBUG "temp2 : %s\n", temp2);
		}
		start_token = strstr(temp,",");
		while (index_1 < 10) {
			start_token = strstr(temp, ",");
			len = strcspn(temp, ",");
			//printk(KERN_DEBUG "len[%d]\n", len);	//Only debug
			total_len = total_len + len;
			temp = (temp + (len+2));
			index_1 ++;
		}
		total_len = total_len + 19;
		//printk(KERN_DEBUG "%d\n", total_len);	//Only debug

		/* read table */
		if (reg != NULL) {
			memcpy(reg_buf, (reg + 1), total_len);
			//printk(KERN_DEBUG "reg_buf : %s\n", reg_buf);	//Only debug
			start = (reg + total_len+1);
		}

		reg_token = reg_buf;

		index_1 = 0;
		start_token=strstr(reg_token,",");
		while (index_1 < 10) {
			start_token = strstr(reg_token, ",");
			len = strcspn(reg_token, ",");
			//printk(KERN_DEBUG "len[%d]\n", len);	//Only debug
			memcpy(token_buf, reg_token, len);
			//printk(KERN_DEBUG "[%d]%s ", index_1, token_buf);	//Only debug
			token_value = (unsigned short)simple_strtoul(token_buf, NULL, 10);
			total_index = index_2 * 10 + index_1;
			//printk(KERN_DEBUG "[%d]%d ", total_index, token_value);	//Only debug
			gtable_buf[total_index] = token_value;
			index_1 ++;
			reg_token = (reg_token + (len + 2));
		}
		index_2 ++;
	}

#if 0	//Only debug
	index_2 = 0;
	while ( index_2 < TABLE_MAX_NUM) {
		printk(KERN_DEBUG "[%d]%d ",index_2, gtable_buf[index_2]);
		index_2++;
	}
#endif
	printk(KERN_DEBUG "sr300pc20_define_table end!\n");

	return 0;
}

static int sr300pc20_define_read(char *name, int len_size)
{
	char *start, *end, *reg;
	char reg_7[7], reg_5[5];
	int define_value = 0;

	*(reg_7 + 6) = '\0';
	*(reg_5 + 4) = '\0';

	//printk(KERN_DEBUG "sr300pc20_define_read start!\n");

	start = strstr(sr300pc20_regs_table, name);
	if (!start) {
		cam_err("%s: can not find %s", __func__, name);
		return -ENOENT;
	}
	end = strstr(start, "tuning");
	if (!end) {
		cam_err("%s: can not find %s", __func__,  "tuning");
		return -ENOENT;
	}

	reg = strstr(start," ");

	if ((reg == NULL) || (reg > end)) {
		printk(KERN_DEBUG "sr300pc20_define_read error %s : ",name);
		return -1;
	}

	/* Write Value to Address */
	if (reg != NULL) {
		if (len_size == 6) {
			memcpy(reg_7, (reg + 1), len_size);
			define_value = (unsigned short)simple_strtoul(reg_7, NULL, 16);
		} else {
			memcpy(reg_5, (reg + 1), len_size);
			define_value = (unsigned short)simple_strtoul(reg_5, NULL, 10);
		}
	}
	//printk(KERN_DEBUG "sr300pc20_define_read end (0x%x)!\n", define_value);

	return define_value;
}

static int sr300pc20_write_regs_from_sd(struct v4l2_subdev *sd, const char *name)
{
	char *start, *end, *reg, *size;
	unsigned short addr;
	unsigned int len, value;
	char reg_buf[7], data_buf1[5], data_buf2[7], len_buf[5];

	*(reg_buf + 6) = '\0';
	*(data_buf1 + 4) = '\0';
	*(data_buf2 + 6) = '\0';
	*(len_buf + 4) = '\0';

	printk(KERN_DEBUG "sr300pc20_regs_table_write start!\n");
	printk(KERN_DEBUG "E string = %s\n", name);

	start = strstr(sr300pc20_regs_table, name);
	if (!start) {
		cam_err("%s: can not find %s", __func__, name);
		return -ENOENT;
	}
	end = strstr(start, "};");

	while (1) {
		/* Find Address */
		reg = strstr(start,"{0x");
		if ((reg == NULL) || (reg > end))
			break;

		/* Write Value to Address */
		if (reg != NULL) {
			memcpy(reg_buf, (reg + 1), 6);
			memcpy(data_buf2, (reg + 8), 6);
			size = strstr(data_buf2,",");
			if (size) { /* 1 byte write */
				memcpy(data_buf1, (reg + 8), 4);
				memcpy(len_buf, (reg + 13), 4);
				addr = (unsigned short)simple_strtoul(reg_buf, NULL, 16);
				value = (unsigned int)simple_strtoul(data_buf1, NULL, 16);
				len = (unsigned int)simple_strtoul(len_buf, NULL, 16);
				if (reg)
					start = (reg + 20);  //{0x000b,0x04,0x01},
			} else {/* 2 byte write */
				memcpy(len_buf, (reg + 15), 4);
				addr = (u16)simple_strtoul(reg_buf, NULL, 16);
				value = (u32)simple_strtoul(data_buf2, NULL, 16);
				len = (u32)simple_strtoul(len_buf, NULL, 16);
				if (reg)
					start = (reg + 22);  //{0x000b,0x0004,0x01},
			}
			size = NULL;

			if (addr == 0xFFFF)
				msleep_debug(value, true);
			else
				sr300pc20_i2c_write(sd, addr, value, len);
		}
	}

	printk(KERN_DEBUG "sr300pc20_regs_table_write end!\n");

	return 0;
}
#endif

/**
 * sr300pc20_i2c_write_twobyte: Write (I2C) multiple bytes to the camera sensor
 * @client: pointer to i2c_client
 * @cmd: command register
 * @w_data: data to be written
 * @w_len: length of data to be written
 *
 * Returns 0 on success, <0 on error
 */
static int sr300pc20_i2c_write_twobyte(struct i2c_client *client,
					 u16 addr, u16 w_data)
{
	int retry_count = 5;
	int ret = 0;
	u8 buf[4] = {0,};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 4,
		.buf	= buf,
	};

	buf[0] = addr >> 8;
	buf[1] = addr;
	buf[2] = w_data >> 8;
	buf[3] = w_data & 0xff;

#if (0)
	sr300pc20_debug(SR300PC20_DEBUG_I2C, "%s : W(0x%02X%02X%02X%02X)\n",
		__func__, buf[0], buf[1], buf[2], buf[3]);
#else
	/* cam_dbg("I2C writing: 0x%02X%02X%02X%02X\n",
			buf[0], buf[1], buf[2], buf[3]); */
#endif

	do {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (likely(ret == 1))
			break;
		msleep(POLL_TIME_MS);
		cam_err("%s: error(%d), write (%04X, %04X), retry %d.\n",
				__func__, ret, addr, w_data, retry_count);
	} while (retry_count-- > 0);

	CHECK_ERR_COND_MSG(ret != 1, -EIO, "I2C does not working.\n\n");

	return 0;
}

#if 0
static int sr300pc20_i2c_write_block(struct v4l2_subdev *sd, u8 *buf, u32 size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int retry_count = 5;
	int ret = 0;
	struct i2c_msg msg = {client->addr, 0, size, buf};

#ifdef CONFIG_VIDEO_SR300PC20_DEBUG
	if (sr300pc20_debug_mask & SR300PC20_DEBUG_I2C_BURSTS) {
		if ((buf[0] == 0x0F) && (buf[1] == 0x12))
			pr_info("%s : data[0,1] = 0x%02X%02X,"
				" total data size = %d\n",
				__func__, buf[2], buf[3], size-2);
		else
			pr_info("%s : 0x%02X%02X%02X%02X\n",
				__func__, buf[0], buf[1], buf[2], buf[3]);
	}
#endif

	do {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (likely(ret == 1))
			break;
		msleep(POLL_TIME_MS);
	} while (retry_count-- > 0);
	if (ret != 1) {
		dev_err(&client->dev, "%s: I2C is not working.\n", __func__);
		return -EIO;
	}

	return 0;
}
#endif

#define BURST_MODE_BUFFER_MAX_SIZE 2700
u8 sr300pc20_burstmode_buf[BURST_MODE_BUFFER_MAX_SIZE];

/* PX: */
static int sr300pc20_burst_write_regs(struct v4l2_subdev *sd,
			const u32 list[], u32 size, char *name)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL;
	int i = 0, idx = 0;
	u16 subaddr = 0, next_subaddr = 0, value = 0;
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 0,
		.buf	= sr300pc20_burstmode_buf,
	};

	cam_trace("E\n");

	for (i = 0; i < size; i++) {
		CHECK_ERR_COND_MSG((idx > (BURST_MODE_BUFFER_MAX_SIZE - 10)),
			err, "BURST MOD buffer overflow!\n")

		subaddr = (list[i] & 0xFFFF0000) >> 16;
		if (subaddr == 0x0F12)
			next_subaddr = (list[i+1] & 0xFFFF0000) >> 16;

		value = list[i] & 0x0000FFFF;

		switch (subaddr) {
		case 0x0F12:
			/* make and fill buffer for burst mode write. */
			if (idx == 0) {
				sr300pc20_burstmode_buf[idx++] = 0x0F;
				sr300pc20_burstmode_buf[idx++] = 0x12;
			}
			sr300pc20_burstmode_buf[idx++] = value >> 8;
			sr300pc20_burstmode_buf[idx++] = value & 0xFF;

			/* write in burstmode*/
			if (next_subaddr != 0x0F12) {
				msg.len = idx;
				err = i2c_transfer(client->adapter,
					&msg, 1) == 1 ? 0 : -EIO;
				CHECK_ERR_MSG(err, "i2c_transfer\n");
				/* cam_dbg("sr300pc20_sensor_burst_write,
						idx = %d\n", idx); */
				idx = 0;
			}
			break;

		case 0xFFFF:
			msleep_debug(value, true);
			break;

		default:
			idx = 0;
			err = sr300pc20_i2c_write_twobyte(client,
						subaddr, value);
			CHECK_ERR_MSG(err, "i2c_write_twobytes\n");
			break;
		}
	}

	return 0;
}

/**
 * sr300pc20_read: read data from sensor with I2C
 * Note the data-store way(Big or Little)
 */
static int sr300pc20_i2c_read(struct v4l2_subdev *sd,
			u8 subaddr, u8 *data)
{
	int err = -EIO;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[2];
	u8 buf[16] = {0,};
	int retry = 5;


	CHECK_ERR_COND_MSG(!client->adapter, -ENODEV,
		"can't search i2c client adapter\n");

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = sizeof(subaddr);
	msg[0].buf = &subaddr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	while (retry-- > 0) {
		err = i2c_transfer(client->adapter, msg, 2);
		if (likely(err == 2))
			break;
		cam_err("i2c read: error, read register(0x%X). cnt %d\n",
			subaddr, retry);
		msleep_debug(POLL_TIME_MS, false);
	}

	CHECK_ERR_COND_MSG(err != 2, -EIO, "I2C does not work\n");

	*data = buf[0];

	return 0;
}

/**
 * sr300pc20_write: write data with I2C
 * Note the data-store way(Big or Little)
 */
static inline int  sr300pc20_i2c_write(struct v4l2_subdev *sd,
					u8 subaddr, u8 data)
{
	u8 buf[2];
	int err = 0, retry = 5;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.buf	= buf,
		.len	= 2,
	};

	CHECK_ERR_COND_MSG(!client->adapter, -ENODEV,
		"can't search i2c client adapter\n");

	buf[0] = subaddr;
	buf[1] = data;

	while (retry-- > 0) {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (likely(err == 1))
			break;
		cam_err("i2c write: error %d, write 0x%04X, retry %d\n",
			err, ((subaddr << 8) | data), retry);
		msleep_debug(POLL_TIME_MS, false);
	}

	CHECK_ERR_COND_MSG(err != 1, -EIO, "I2C does not work\n");
	return 0;
}

#define SONY_SR300PC20_BURST_DATA_LENGTH	1200
static unsigned char burst_buf[SONY_SR300PC20_BURST_DATA_LENGTH];
static int sr300pc20_i2c_burst_write_list(struct v4l2_subdev *sd,
		const sr300pc20_regset_t regs[], int size, const char *name)
{
#if 0 /* DSLIM */
	struct i2c_client *sr300pc20_client = v4l2_get_subdevdata(sd);
	int i = 0;
	int iTxDataIndex = 0;
	int retry_count = 5;
	int err = 0;
	u8 *buf = burst_buf;

	struct i2c_msg msg = {sr300pc20_client->addr, 0, 4, buf};

	cam_trace("%s\n", name);

	if (!sr300pc20_client->adapter) {
		printk(KERN_ERR "%s: %d can't search i2c client adapter\n", __func__, __LINE__);
		return -EIO;
	}

	while ( i < size )//0<1
	{
		if ( 0 == iTxDataIndex )
		{
	        //printk("11111111111 delay 0x%04x, value 0x%04x\n", regs[i].subaddr, regs[i].value);
			buf[iTxDataIndex++] = ( regs[i].subaddr & 0xFF00 ) >> 8;
			buf[iTxDataIndex++] = ( regs[i].subaddr & 0xFF );
		}

		if ( ( i < size - 1 ) && ( ( iTxDataIndex + regs[i].len ) <= ( SONY_SR300PC20_BURST_DATA_LENGTH - regs[i+1].len ) ) && ( regs[i].subaddr + regs[i].len == regs[i+1].subaddr ) )
		{
			if ( 1 == regs[i].len )
			{
				//printk("2222222 delay 0x%04x, value 0x%04x\n", regs[i].subaddr, regs[i].value);
				buf[iTxDataIndex++] = ( regs[i].value & 0xFF );
			}
			else
			{
				// Little Endian
				buf[iTxDataIndex++] = ( regs[i].value & 0x00FF );
				buf[iTxDataIndex++] = ( regs[i].value & 0xFF00 ) >> 8;
				//printk("3333333 delay 0x%04x, value 0x%04x\n", regs[i].subaddr, regs[i].value);
			}
		}
		else
		{
				if ( 1 == regs[i].len )
				{
					//printk("4444444 delay 0x%04x, value 0x%04x\n", regs[i].subaddr, regs[i].value);
					buf[iTxDataIndex++] = ( regs[i].value & 0xFF );
					//printk("burst_index:%d\n", iTxDataIndex);
					msg.len = iTxDataIndex;

				}
				else
				{
					//printk("555555 delay 0x%04x, value 0x%04x\n", regs[i].subaddr, regs[i].value);
					// Little Endian
					buf[iTxDataIndex++] = (regs[i].value & 0x00FF );
					buf[iTxDataIndex++] = (regs[i].value & 0xFF00 ) >> 8;
					//printk("burst_index:%d\n", iTxDataIndex);
					msg.len = iTxDataIndex;

				}

				while(retry_count--) {
				err  = i2c_transfer(sr300pc20_client->adapter, &msg, 1);
					if (likely(err == 1))
						break;

			}
			iTxDataIndex = 0;
		}
		i++;
	}
#else
	cam_err("burst write: not implemented\n");
#endif
	return 0;
}

static inline int sr300pc20_write_regs(struct v4l2_subdev *sd,
			const sr300pc20_regset_t regs[], int size)
{
	int err = 0, i;
	u8 subaddr, value;

	cam_trace("size %d\n", size);

	for (i = 0; i < size; i++) {
		subaddr = (u8)(regs[i] >> 8);
		value = (u8)(regs[i]);
		if (unlikely(DELAY_SEQ == subaddr))
			msleep_debug(value * 10, true);
		else {
			err = sr300pc20_writeb(sd, subaddr, value);
			CHECK_ERR_MSG(err, "register set failed\n")
                }
	}

	return 0;
}

/* PX: */
static int sr300pc20_set_from_table(struct v4l2_subdev *sd,
				const char *setting_name,
				const struct regset_table *table,
				u32 table_size, s32 index)
{
	int err = 0;

	cam_trace("set %s index %d\n", setting_name, index);

	CHECK_ERR_COND_MSG(((index < 0) || (index >= table_size)),
		-EINVAL, "index(%d) out of range[0:%d] for table for %s\n",
		index, table_size, setting_name);

	table += index;

#ifdef CONFIG_LOAD_FILE
	cam_dbg("%s: \"%s\", reg_name=%s\n", __func__,
			setting_name, table->name);
	return sr300pc20_write_regs_from_sd(sd, table->name);

#else /* !CONFIG_LOAD_FILE */
	CHECK_ERR_COND_MSG(!table->reg, -EFAULT, \
		"table=%s, index=%d, reg = NULL\n", setting_name, index);
# ifdef DEBUG_WRITE_REGS
	cam_dbg("write_regtable: \"%s\", reg_name=%s\n", setting_name,
		table->name);
# endif /* DEBUG_WRITE_REGS */

	if (table->burst) {
		err = sr300pc20_i2c_burst_write_list(sd,
			table->reg, table->array_size, setting_name);
	} else
		err = sr300pc20_write_regs(sd, table->reg, table->array_size);

	CHECK_ERR_MSG(err, "write regs(%s), err=%d\n", setting_name, err);

	return 0;
#endif /* CONFIG_LOAD_FILE */
}

static inline int sr300pc20_hw_stby_on(struct v4l2_subdev *sd, bool on)
{
	struct sr300pc20_state *state = to_state(sd);

#if 0 /* dslim */
	return state->pdata->stby_on(on);
#else
	return 0;
#endif
}

static int sr300pc20_is_om_changed(struct v4l2_subdev *sd)
{
	u32 status = 0x3, val = 0;
	int err, cnt1, cnt2;

	for (cnt1 = 0; cnt1 < SR300PC20_CNT_OM_CHECK; cnt1++) {
		err = isx012_readb(sd, REG_INTSTS, &val);
		CHECK_ERR_MSG(err, "om changed: error, readb cnt=%d\n", cnt1);

		if ((val & REG_INTBIT_OM) == REG_INTBIT_OM) {
			status &= ~0x01;
			break;
		}
		msleep_debug(5, false);
	}

	for (cnt2 = 0; cnt2 < SR300PC20_CNT_OM_CHECK; cnt2++) {
		err = isx012_writeb(sd, REG_INTCLR, REG_INTBIT_OM);
		err |= isx012_readb(sd, REG_INTSTS, &val);
		CHECK_ERR_MSG(err, "om changed: clear error, rw\n");

		if ((val & REG_INTBIT_OM) == 0) {
			status &= ~0x02;
			break;
		}
		msleep_debug(5, false);
	}

	if (unlikely(status)) {
		cam_err("om changed: error, fail 0x%X\n", status);
		return -EAGAIN;
	}

	cam_dbg("om changed: int cnt=%d, clr cnt=%d\n", cnt1, cnt2);

	return 0;
}

/**
 * sr300pc20_is_cm_changed:
 *
 * status: 0x01 = int check fail
 *       0x02 = int clear fail
 *       0x03 = int check, clear fail
 */
static int sr300pc20_is_cm_changed(struct v4l2_subdev *sd)
{
	u32 status = 0x3, val = 0;
	int err = 0, cnt1, cnt2;

	for (cnt1 = 0; cnt1 < SR300PC20_CNT_CM_CHECK; cnt1++) {
		err = isx012_readb(sd, REG_INTSTS, &val);
		CHECK_ERR_MSG(err, "cm changed: error, readb\n")
		if ((val & REG_INTBIT_CM) == REG_INTBIT_CM) {
			status &= ~0x01;
			break;
		}
		msleep_debug(5, false);
	}

	for (cnt2 = 0; cnt2 < SR300PC20_CNT_CM_CHECK; cnt2++) {
		err = isx012_writeb(sd, REG_INTCLR, REG_INTBIT_CM);
		err |= isx012_readb(sd, REG_INTSTS, &val);
		CHECK_ERR_MSG(err, "cm changed: clear error, rw\n");

		if ((val & REG_INTBIT_CM) == 0) {
			status &= ~0x02;
			break;
		}

		msleep_debug(5, false);
	}

	if (unlikely(status)) {
		cam_err("cm changed: error, fail 0x%X\n", status);
		return -EAGAIN;
	}

	cam_dbg("cm changed: int cnt=%d, clr cnt=%d\n", cnt1, cnt2);

	return 0;
}

static inline int sr300pc20_transit_preview_mode(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);

	sr300pc20_restore_sensor_flash(sd);

	if (state->exposure.ae_lock || state->wb.awb_lock)
		cam_info("Restore user ae(awb)-lock...\n");

	return sr300pc20_set_from_table(sd, "preview_mode",
		&state->regs->preview_mode, 1, 0);
}

/**
 * sr300pc20_transit_half_mode: go to a half-release mode
 * Don't forget that half mode is not used in movie mode.
 */
static inline int sr300pc20_transit_half_mode(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = -EIO;

	/* We do not go to half release mode in movie mode */
	if (state->sensor_mode == SENSOR_MOVIE)
		return 0;

	if (state->scene_mode == SCENE_MODE_NIGHTSHOT &&
	    state->light_level >= LUX_LEVEL_LOW) {
		cam_info("half_mode: night lowlux\n");
		state->capture.lowlux_night = 1;
		err = sr300pc20_set_from_table(sd, "night_halfrelease_mode",
			&state->regs->halfrelease_mode_night, 1, 0);
	} else {
		state->capture.lowlux_night = 0;
		err = sr300pc20_set_from_table(sd, "halfrelease",
			&state->regs->halfrelease_mode, 1, 0);
	}

	msleep_debug(40, true);
	return err;
}

static inline int sr300pc20_transit_capture_mode(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = -EIO;

	if (state->capture.lowlux_night) {
		cam_info("capture_mode: night lowlux\n");
		err = sr300pc20_set_from_table(sd, "capture_mode_night",
			&state->regs->capture_mode_night, 1, 0);
	} else
		err = sr300pc20_set_from_table(sd, "capture_mode",
			&state->regs->capture_mode, 1, 0);

	return err;
}

/**
 * sr300pc20_transit_movie_mode: switch camera mode if needed.
 * Note that this fuction should be called from start_preview().
 */
static inline int sr300pc20_transit_movie_mode(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);

	/* we'll go from the below modes to RUNNING or RECORDING */
	switch (state->runmode) {
	case RUNMODE_INIT:
		/* case of entering camcorder firstly */
	case RUNMODE_RUNNING_STOP:
		/* case of switching from camera to camcorder */
		if (state->sensor_mode == SENSOR_MOVIE) {
			cam_dbg("switching to camcorder mode\n");
			sr300pc20_set_from_table(sd, "camcorder_on",
				&state->regs->camcorder_on, 1, 0);
		}
		break;

	case RUNMODE_RECORDING_STOP:
		/* case of switching from camcorder to camera */
		if (state->sensor_mode == SENSOR_CAMERA) {
			cam_dbg("switching to camera mode\n");
			sr300pc20_set_from_table(sd, "camcorder_off",
				&state->regs->camcorder_off, 1, 0);
		}
		break;

	default:
		break;
	}

	return 0;
}

#ifdef CONFIG_DEBUG_NO_FRAME
#if 0 /* new written codes */
static void sr300pc20_frame_checker(struct work_struct *work)
{
	struct sr300pc20_state *state = container_of(work, \
			struct sr300pc20_state, frame_work);
	struct v4l2_subdev *sd = &state->sd;
	u32 val = 0, mask;
	int err, cnt, int_cnt = 0;
	u32 target_cnt = 2;

	/* cam_dbg("========= frame checker =========\n");*/

#ifdef CONFIG_DEBUG_CAPTURE_FRAME
	if (state->format_mode == V4L2_PIX_FMT_MODE_CAPTURE) {
		mask = REG_INTBIT_CAPNUM_END;
		target_cnt = 1;
	} else
#endif
	{
		mask = REG_INTBIT_VINT;
		target_cnt = 2;
	}

	for (cnt = 0; cnt < SR300PC20_CNT_CAPTURE_FRM; cnt++) {
		err = isx012_readb(sd, REG_INTSTS, &val);
		if (unlikely(err)) {
			cam_err("frame_checker: error, readb\n");
			return;
		}

		if (((u8)val & mask) == mask) {
			cam_info("frame INT %d (target %d)\n",
				int_cnt, target_cnt);
			if (++int_cnt >= target_cnt) {
				state->frame_check = false;
				return;
			}

			isx012_writeb(sd, REG_INTCLR, mask);
			isx012_readb(sd, REG_INTSTS, &val);
			if (((u8)val & mask) != 0) {
				cam_info("frame_checker: cannot clear int");
				return;
			}
		}

		if (!state->frame_check) {
			cam_dbg("frame_checker aborted.\n");
			return;
		}

		msleep_debug(10, false);
	}

	cam_err("frame INT Not occured!\n");
}

static int sr300pc20_start_frame_checker(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int cnt, err = 0;
	u32 val = 0;
	u32 mask = REG_INTBIT_VINT | REG_INTBIT_CAPNUM_END;

	/* cam_trace("EX\n"); */
#ifdef CONFIG_DEBUG_CAPTURE_FRAME
	if (state->format_mode == V4L2_PIX_FMT_MODE_CAPTURE)
		isx012_writeb(sd, REG_CAPNUM, 1);
#endif

	for (cnt = 0; cnt < SR300PC20_CNT_CLEAR_VINT; cnt++) {
		isx012_writeb(sd, REG_INTCLR, mask);
		isx012_readb(sd, REG_INTSTS, &val);
		if (((u8)val & mask) == 0)
			break;

		cam_info("frame_checker: clear int register\n");
		msleep_debug(5, false);
	}

	if (unlikely(cnt >= SR300PC20_CNT_CLEAR_VINT)) {
		cam_info("fail to clear vint. frame_detecter not started\n");
		return -1;
	}

	state->frame_check = true;
	err = queue_work(state->workqueue, &state->frame_work);
	if (unlikely(!err))
		cam_info("frame_detecter is already operating!\n");

	return 0;
}
#else
static void sr300pc20_frame_checker(struct work_struct *work)
{
	struct sr300pc20_state *state = container_of(work, \
			struct sr300pc20_state, frame_work);
	struct v4l2_subdev *sd = &state->sd;
	u32 val = 0;
	int cnt, err, int_cnt = 0;

	/* cam_dbg("========= frame checker =========\n");*/

	for (cnt = 0; cnt < SR300PC20_CNT_CAPTURE_FRM; cnt++) {
		err = isx012_readb(sd, REG_INTSTS, &val);
		if (unlikely(err)) {
			cam_err("frame_checker: error, readb\n");
			return;
		}

		if (((u8)val & SR300PC20_INTSRC_VINT) == SR300PC20_INTSRC_VINT) {
			++int_cnt;
			cam_info("frame_INT %d (cnt=%d)\n", int_cnt, cnt);
			if (int_cnt >= 2) {
				state->frame_check = false;
				return;
			}

			isx012_writeb(sd, REG_INTCLR, SR300PC20_INTSRC_VINT);
			isx012_readb(sd, REG_INTSTS, &val);
			if (((u8)val & SR300PC20_INTSRC_VINT) != 0) {
				cam_info("frame_checker: cannot clear int");
				return;
			}
		}

		if (!state->frame_check) {
			cam_dbg("frame_checker aborted.\n");
			return;
		}

		msleep_debug(3, false);
	}

	cam_err("frame INT Not occured!\n");
}

static int sr300pc20_start_frame_checker(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int cnt, err = 0;
	u32 val = 0;

	/* cam_trace("EX\n"); */

	for (cnt = 0; cnt < SR300PC20_CNT_CLEAR_VINT; cnt++) {
		isx012_writeb(sd, REG_INTCLR, SR300PC20_INTSRC_VINT);
		isx012_readb(sd, REG_INTSTS, &val);
		if (((u8)val & SR300PC20_INTSRC_VINT) == 0)
			break;

		msleep_debug(5, false);
	}

	if (unlikely(cnt >= SR300PC20_CNT_CLEAR_VINT)) {
		cam_info("fail to clear vint. frame_detecter not started\n");
		return -1;
	}

	state->frame_check = true;
	err = queue_work(state->workqueue, &state->frame_work);
	if (unlikely(!err))
		cam_info("frame_detecter is already operating!\n");

	return 0;
}
#endif

static void sr300pc20_stop_frame_checker(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);

	/* cam_trace("EX\n"); */
	state->frame_check = false;
	if (flush_work(&state->frame_work))
		cam_dbg("wait... frame_checker stopped\n");
}
#endif /* CONFIG_DEBUG_NO_FRAME */

/**
 * sr300pc20_is_hwflash_on - check whether flash device is on
 *
 * Refer to state->flash.on to check whether flash is in use in driver.
 */
static inline int sr300pc20_is_hwflash_on(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);

#ifdef SR300PC20_SUPPORT_FLASH
	return state->pdata->is_flash_on();
#else
	return 0;
#endif
}

/**
 * sr300pc20_flash_en - contro Flash LED
 * @mode: SR300PC20_FLASH_MODE_NORMAL or SR300PC20_FLASH_MODE_MOVIE
 * @onoff: SR300PC20_FLASH_ON or SR300PC20_FLASH_OFF
 */
static int sr300pc20_flash_en(struct v4l2_subdev *sd, s32 mode, s32 onoff)
{
	struct sr300pc20_state *state = to_state(sd);

	if (unlikely(state->flash.ignore_flash)) {
		cam_warn("WARNING, we ignore flash command.\n");
		return 0;
	}

#ifdef SR300PC20_SUPPORT_FLASH
	return state->pdata->flash_en(mode, onoff);
#else
	return 0;
#endif
}

/**
 * sr300pc20_flash_torch - turn flash on/off as torch for preflash, recording
 * @onoff: SR300PC20_FLASH_ON or SR300PC20_FLASH_OFF
 *
 * This func set state->flash.on properly.
 */
static inline int sr300pc20_flash_torch(struct v4l2_subdev *sd, s32 onoff)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = 0;

	err = sr300pc20_flash_en(sd, SR300PC20_FLASH_MODE_MOVIE, onoff);
	state->flash.on = (onoff == SR300PC20_FLASH_ON) ? 1 : 0;

	return err;
}

/**
 * sr300pc20_flash_oneshot - turn main flash on for capture
 * @onoff: SR300PC20_FLASH_ON or SR300PC20_FLASH_OFF
 *
 * Main flash is turn off automatically in some milliseconds.
 */
static inline int sr300pc20_flash_oneshot(struct v4l2_subdev *sd, s32 onoff)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = 0;

	err = sr300pc20_flash_en(sd, SR300PC20_FLASH_MODE_NORMAL, onoff);
	state->flash.on = (onoff == SR300PC20_FLASH_ON) ? 1 : 0;

	return err;
}

static const struct sr300pc20_framesize *sr300pc20_get_framesize
	(const struct sr300pc20_framesize *frmsizes,
	u32 frmsize_count, u32 index)
{
	int i = 0;

	for (i = 0; i < frmsize_count; i++) {
		if (frmsizes[i].index == index)
			return &frmsizes[i];
	}

	return NULL;
}

/* This function is called from the g_ctrl api
 *
 * This function should be called only after the s_fmt call,
 * which sets the required width/height value.
 *
 * It checks a list of available frame sizes and sets the
 * most appropriate frame size.
 *
 * The list is stored in an increasing order (as far as possible).
 * Hence the first entry (searching from the beginning) where both the
 * width and height is more than the required value is returned.
 * In case of no perfect match, we set the last entry (which is supposed
 * to be the largest resolution supported.)
 */
static void sr300pc20_set_framesize(struct v4l2_subdev *sd,
				const struct sr300pc20_framesize *frmsizes,
				u32 num_frmsize, bool preview)
{
	struct sr300pc20_state *state = to_state(sd);
	const struct sr300pc20_framesize **found_frmsize = NULL;
	u32 width = state->req_fmt.width;
	u32 height = state->req_fmt.height;
	int i = 0;

	cam_dbg("%s: Requested Res %dx%d\n", __func__,
			width, height);

	found_frmsize = preview ?
		&state->preview.frmsize : &state->capture.frmsize;

	for (i = 0; i < num_frmsize; i++) {
		if ((frmsizes[i].width == width) &&
			(frmsizes[i].height == height)) {
			*found_frmsize = &frmsizes[i];
			break;
		}
	}

	if (*found_frmsize == NULL) {
		cam_err("%s: error, invalid frame size %dx%d\n",
			__func__, width, height);
		*found_frmsize = preview ?
			sr300pc20_get_framesize(frmsizes, num_frmsize,
					PREVIEW_SZ_XGA) :
			sr300pc20_get_framesize(frmsizes, num_frmsize,
					CAPTURE_SZ_3MP);
		BUG_ON(!(*found_frmsize));
	}

	if (preview)
		cam_info("Preview Res Set: %dx%d, index %d\n",
			(*found_frmsize)->width, (*found_frmsize)->height,
			(*found_frmsize)->index);
	else
		cam_info("Capture Res Set: %dx%d, index %d\n",
			(*found_frmsize)->width, (*found_frmsize)->height,
			(*found_frmsize)->index);
}

/* PX: Set scene mode */
static int sr300pc20_set_scene_mode(struct v4l2_subdev *sd, s32 val)
{
	struct sr300pc20_state *state = to_state(sd);

	cam_trace("E, value %d\n", val);

retry:
	switch (val) {
	case SCENE_MODE_NONE:
	case SCENE_MODE_PORTRAIT:
	case SCENE_MODE_NIGHTSHOT:
	case SCENE_MODE_BACK_LIGHT:
	case SCENE_MODE_LANDSCAPE:
	case SCENE_MODE_SPORTS:
	case SCENE_MODE_PARTY_INDOOR:
	case SCENE_MODE_BEACH_SNOW:
	case SCENE_MODE_SUNSET:
	case SCENE_MODE_DUSK_DAWN:
	case SCENE_MODE_FALL_COLOR:
	case SCENE_MODE_FIREWORKS:
	case SCENE_MODE_TEXT:
	case SCENE_MODE_CANDLE_LIGHT:
		sr300pc20_set_from_table(sd, "scene_mode",
			state->regs->scene_mode,
			ARRAY_SIZE(state->regs->scene_mode), val);
		break;

	default:
		cam_err("set_scene: error, not supported (%d)\n", val);
		val = SCENE_MODE_NONE;
		goto retry;
	}

	state->scene_mode = val;

	cam_trace("X\n");
	return 0;
}

/* PX: Set brightness */
static int sr300pc20_set_exposure(struct v4l2_subdev *sd, s32 val)
{
	struct sr300pc20_state *state = to_state(sd);
	static const u8 gain_level[GET_EV_INDEX(EV_MAX_V4L2)] = {
		0x14, 0x1C, 0x21, 0x26, 0x2B, 0x2F, 0x33, 0x37, 0x3B};
	int err = 0;

	if ((val < EV_MINUS_4) || (val > EV_PLUS_4)) {
		cam_err("%s: error, invalid value(%d)\n", __func__, val);
		return -EINVAL;
	}

	state->lux_level_flash = gain_level[GET_EV_INDEX(val)];

	sr300pc20_set_from_table(sd, "brightness", state->regs->ev,
		ARRAY_SIZE(state->regs->ev), GET_EV_INDEX(val));

	state->exposure.val = val;

	return err;
}

static inline u32 sr300pc20_get_light_level(struct v4l2_subdev *sd,
					u32 *light_level)
{
	struct sr300pc20_state *state = to_state(sd);
	u32 val_lsb = 0, val_msb = 0;

	if (state->iso == ISO_AUTO)
		isx012_readb(sd, REG_USER_GAINLEVEL_NOW, light_level);
	else {
		sr300pc20_readw(sd, REG_SHT_TIME_OUT_L, &val_lsb);
		sr300pc20_readw(sd, REG_SHT_TIME_OUT_H, &val_msb);

		*light_level = (val_msb << 16) | (val_lsb & 0xFFFF);
	}

	cam_trace("X, iso = %d, light level = 0x%X", state->iso, *light_level);

	return 0;
}

/* PX(NEW) */
static int sr300pc20_set_capture_size(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	u32 width, height;

	if (unlikely(!state->capture.frmsize)) {
		cam_warn("warning, capture resolution not set\n");
		state->capture.frmsize = sr300pc20_get_framesize(
					sr300pc20_capture_frmsizes,
					ARRAY_SIZE(sr300pc20_capture_frmsizes),
					CAPTURE_SZ_5MP);
	}

	width = state->capture.frmsize->width;
	height = state->capture.frmsize->height;

	cam_dbg("set capture size(%dx%d)\n", width, height);

	sr300pc20_writew(sd, REG_HSIZE_CAP, width);
	sr300pc20_writew(sd, REG_VSIZE_CAP, height);

	return 0;
}

/* PX: Set sensor mode */
static int sr300pc20_set_sensor_mode(struct v4l2_subdev *sd, s32 val)
{
	struct sr300pc20_state *state = to_state(sd);

	cam_trace("mode=%d\n", val);

	switch (val) {
	case SENSOR_MOVIE:
		/* We does not support movie mode when in VT. */
		if (state->vt_mode) {
			state->sensor_mode = SENSOR_CAMERA;
			cam_err("%s: error, Not support movie\n", __func__);
			break;
		}
		/* We do not break. */

	case SENSOR_CAMERA:
		state->sensor_mode = val;
		break;

	default:
		cam_err("%s: error, Not support.(%d)\n", __func__, val);
		state->sensor_mode = SENSOR_CAMERA;
		WARN_ON(1);
		break;
	}

	return 0;
}

/* PX: Set framerate */
static int sr300pc20_set_frame_rate(struct v4l2_subdev *sd, s32 fps)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = -EIO;
	int i = 0, fps_index = -1;

#if 0 /* DSLIM */
	if (!state->initialized || (state->req_fps < 0))
		return 0;

	cam_info("set frame rate %d\n", fps);

	for (i = 0; i < ARRAY_SIZE(sr300pc20_framerates); i++) {
		if (fps == sr300pc20_framerates[i].fps) {
			fps_index = sr300pc20_framerates[i].index;
			state->fps = fps;
			state->req_fps = -1;
			break;
		}
	}

	if (unlikely(fps_index < 0)) {
		cam_err("set_fps: warning, not supported fps %d\n", fps);
		return 0;
	}

	err = sr300pc20_set_from_table(sd, "fps", state->regs->fps,
			ARRAY_SIZE(state->regs->fps), fps_index);
	CHECK_ERR_MSG(err, "fail to set framerate\n");
#endif
	return 0;
}

static int sr300pc20_set_ae_lock(struct v4l2_subdev *sd, s32 lock, bool force)
{
	struct sr300pc20_state *state = to_state(sd);
	u32 val = 0;

	switch (lock) {
	case AE_LOCK:
		isx012_readb(sd, REG_CPUEXT, &val);
		val |= REG_CPUEXT_AE_HOLD;
		isx012_writeb(sd, REG_CPUEXT, val);

		state->exposure.ae_lock = 1;
		cam_info("AE lock by user\n");
		break;

	case AE_UNLOCK:
		if (unlikely(!force && !state->exposure.ae_lock))
			return 0;

		isx012_readb(sd, REG_CPUEXT, &val);
		val &= ~REG_CPUEXT_AE_HOLD;
		isx012_writeb(sd, REG_CPUEXT, val);

		state->exposure.ae_lock = 0;
		cam_info("AE unlock\n");
		break;

	default:
		cam_err("ae_lock: warning, invalid argument(%d)\n", val);
	}

	return 0;
}

static int sr300pc20_set_awb_lock(struct v4l2_subdev *sd, s32 lock, bool force)
{
	struct sr300pc20_state *state = to_state(sd);
	u32 val = 0;

	switch (lock) {
	case AWB_LOCK:
		if (state->wb.mode != WHITE_BALANCE_AUTO)
			return 0;

		isx012_readb(sd, REG_CPUEXT, &val);
		val |= REG_CPUEXT_AWB_HOLD;
		isx012_writeb(sd, REG_CPUEXT, val);

		state->wb.awb_lock = 1;
		cam_info("AWB lock by user\n");
		break;

	case AWB_UNLOCK:
		if (unlikely(!force && !state->wb.awb_lock))
			return 0;

		isx012_readb(sd, REG_CPUEXT, &val);
		val &= ~REG_CPUEXT_AWB_HOLD;
		isx012_writeb(sd, REG_CPUEXT, val);

		state->wb.awb_lock = 0;
		cam_info("AWB unlock\n");
		break;

	default:
		cam_err("%s: warning, invalid argument(%d)\n", __func__, val);
	}

	return 0;
}

/* PX: Set AE, AWB Lock */
static int sr300pc20_set_lock(struct v4l2_subdev *sd, s32 lock, bool force)
{
#if 0
	int err = -EIO;

	cam_trace("%s\n", lock ? "on" : "off");
	if (unlikely((u32)lock >= AEAWB_LOCK_MAX)) {
		cam_err("%s: error, invalid argument\n", __func__);
		return -EINVAL;
	}

	err = sr300pc20_set_ae_lock(sd, (lock == AEAWB_LOCK) ?
				AE_LOCK : AE_UNLOCK, force);
	if (unlikely(err))
		goto out_err;

	err = sr300pc20_set_awb_lock(sd, (lock == AEAWB_LOCK) ?
				AWB_LOCK : AWB_UNLOCK, force);
	if (unlikely(err))
		goto out_err;

	cam_trace("X\n");
	return 0;

out_err:
	cam_err("%s: error, failed to set lock\n", __func__);
	return err;
#else
	return 0;
#endif
}

static int sr300pc20_set_af_softlanding(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = -EINVAL;

	err = sr300pc20_set_from_table(sd, "softlanding",
			&state->regs->softlanding, 1, 0);
	CHECK_ERR_MSG(err, "fail to set softlanding\n");

	return 0;
}

/* PX: */
static int sr300pc20_return_focus(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = -EINVAL;

	cam_trace("EX\n");

	if (state->focus.mode == FOCUS_MODE_MACRO)
		err = sr300pc20_set_from_table(sd, "af_macro_mode",
				&state->regs->af_macro_mode, 1, 0);
	else
		err = sr300pc20_set_from_table(sd, "af_normal_mode",
				&state->regs->af_normal_mode, 1, 0);

	CHECK_ERR(err);
	return 0;
}

static int sr300pc20_pre_sensor_flash(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);

	cam_trace("EX\n");

	/* change flash AE line */
	sr300pc20_set_from_table(sd, "flash_ae_line",
		&state->regs->flash_ae_line, 1, 0);

	/* Wait 1V time(60ms) */
	msleep_debug(60, true);

	/* read preview AE scale */
	sr300pc20_readw(sd, REG_USER_AESCL_AUTO,
		&state->flash.ae_offset.ae_auto);
	sr300pc20_readw(sd, REG_ERRSCL_AUTO, &state->flash.ae_offset.ersc_auto);

	if (state->wb.mode == WHITE_BALANCE_AUTO)
		isx012_writeb(sd, REG_AWB_SN1, 0x00);

	/* write flash_on_set */
	sr300pc20_set_from_table(sd, "flash_on",
		&state->regs->flash_on, 1, 0);

	isx012_writeb(sd, REG_VPARA_TRG, 0x01);

	/* Wait 1V time(40ms) */
	msleep_debug(40, true);

	return 0;
}

static int sr300pc20_post_sensor_flash(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);

	cam_trace("EX\n");

	sr300pc20_set_from_table(sd, "flash_off",
		&state->regs->flash_off, 1, 0);

	if (state->wb.mode == WHITE_BALANCE_AUTO)
		isx012_writeb(sd, REG_AWB_SN1, 0x20);

	isx012_writeb(sd, REG_VPARA_TRG, 0x01);

	state->flash.ae_flash_lock = 0;
	state->capture.ae_manual_mode = 0;

	return 0;
}

static inline int sr300pc20_restore_sensor_flash(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);

	if (!state->flash.ae_flash_lock)
		return 0;

	cam_info("Flash is locked. Unlocking...\n");
	return sr300pc20_post_sensor_flash(sd);
}

static int sr300pc20_wait_ae_stable(struct v4l2_subdev *sd,
				bool af_doing, bool long_wait)
{
	struct sr300pc20_state *state = to_state(sd);
	u32 val = 0, mode = 0;
	int count;
	int max_1st, max_2nd;

	if (af_doing || long_wait)
		max_1st = max_2nd = SR300PC20_CNT_AE_STABLE;
	else {
		max_1st = 20; /* 200ms + alpha */
		max_2nd = 70; /* 700ms + alpha */
	}

	/* 1st: go to Half mode */
	for (count = 0; count < max_1st; count++) {
		if (af_doing && (state->focus.start == AUTO_FOCUS_OFF))
			goto cancel_out;

		isx012_readb(sd, REG_MODESEL_FIX, &mode);
		if ((u8)mode == 0x01)
			break;

		msleep_debug(10, false);
	}

	if (count >= max_1st)
		cam_info("check_ae_stable: fail to check modesel_fix\n\n");
	else
		cam_dbg("check_ae_stable: 1st check count=%d\n", count);

	/* 2nd: check move_sts */
	for (count = 0; count < max_2nd; count++) {
		if (af_doing && (state->focus.start == AUTO_FOCUS_OFF))
			goto cancel_out;

		isx012_readb(sd, REG_HALF_MOVE_STS, &val);
		if ((u8)val == 0x00)
			break;

		msleep_debug(10, false);
	}

	if (count >= max_2nd)
		cam_info("check_ae_stable: fail to check half_move_sts\n\n");
	else
		cam_dbg("check_ae_stable: 2nd check count=%d\n", count);

	return 0;

cancel_out:
	cam_info("check_ae_stable: AF is cancelled(%s)\n",
		mode == 0x01 ? "1st" : "2nd");
	return 1;
}

static bool sr300pc20_check_flash_fire(struct v4l2_subdev *sd, u32 light_level)
{
	struct sr300pc20_state *state = to_state(sd);

	if (state->iso == ISO_AUTO) {
		if (light_level < state->lux_level_flash)
			goto flash_off;
	} else if (light_level < state->shutter_level_flash)
		goto flash_off;

	/* Flash on */
	return true;

flash_off:
	sr300pc20_restore_sensor_flash(sd);

	return false;
}

static int sr300pc20_cancel_af(struct v4l2_subdev *sd, bool flash)
{
	struct sr300pc20_state *state = to_state(sd);

	if (state->focus.mode == FOCUS_MODE_MACRO)
		sr300pc20_set_from_table(sd, "cancel_af_macro",
			&state->regs->cancel_af_macro, 1, 0);
	else
		sr300pc20_set_from_table(sd, "cancel_af_normal",
			&state->regs->cancel_af_normal, 1, 0);

	if (flash)
		sr300pc20_post_sensor_flash(sd);

	return 0;
}

/* PX: Prepare AF Flash */
static int sr300pc20_af_start_preflash(struct v4l2_subdev *sd,
				u32 touch, u32 flash_mode)
{
	struct sr300pc20_state *state = to_state(sd);
	bool flash = false;

	cam_trace("E\n");

	if (state->sensor_mode == SENSOR_MOVIE)
		return 0;

	cam_dbg("Start SINGLE AF, touch %d, flash mode %d\n",
		touch, flash_mode);

	state->flash.preflash = PREFLASH_OFF;
	state->light_level = LUX_LEVEL_MAX;

	/* We unlock AE, AWB if not going to capture mode after AF
	 * for market app. */
	if (state->focus.lock) {
		cam_warn("Focus is locked. Unlocking...\n");
		isx012_writeb(sd, REG_MODESEL, 0x0);
		msleep_debug(200, false);
		state->focus.lock = 0;
	}

	if (!touch)
		sr300pc20_set_from_table(sd, "af_window_reset",
			&state->regs->af_window_reset, 1, 0);

	sr300pc20_get_light_level(sd, &state->light_level);

	switch (flash_mode) {
	case FLASH_MODE_AUTO:
		if (!sr300pc20_check_flash_fire(sd, state->light_level))
			break;

	case FLASH_MODE_ON:
		flash = true;
		state->flash.preflash = PREFLASH_ON;
		sr300pc20_pre_sensor_flash(sd);
		sr300pc20_flash_torch(sd, SR300PC20_FLASH_ON);
		break;

	case FLASH_MODE_OFF:
	default:
		sr300pc20_restore_sensor_flash(sd);
		break;
	}

	if (flash && sr300pc20_wait_ae_stable_af(sd)) {
		/* Cancel AF */
		state->focus.status = AF_RESULT_CANCELLED;
		sr300pc20_flash_torch(sd, SR300PC20_FLASH_OFF);
		state->flash.preflash = PREFLASH_NONE;

		sr300pc20_cancel_af(sd, flash);
		sr300pc20_return_focus(sd);
	}

	cam_trace("X\n");
	return 0;
}

static int sr300pc20_do_af(struct v4l2_subdev *sd, u32 touch)
{
	struct sr300pc20_state *state = to_state(sd);
	u32 read_value = 0;
	u32 count = 0;
	bool flash = false, success = false;

	cam_trace("E\n");

	/* We do not go to half-release mode if setting FLASH_ON.
	 * And note that flash variable should only be set to true
	 * in camera mode. */
	if (state->flash.preflash == PREFLASH_ON)
		flash = true;

	if (state->sensor_mode == SENSOR_MOVIE) {
		sr300pc20_set_from_table(sd, "af_camcorder_start",
			&state->regs->af_camcorder_start, 1, 0);
	} else if (!flash)
		sr300pc20_transit_half_mode(sd);

	/* Check the result of AF */
	for (count = 0; count < AF_SEARCH_COUNT; count++) {
		if (state->focus.start == AUTO_FOCUS_OFF) {
			cam_dbg("do_af: AF is cancelled while doing\n");
			state->focus.status = AF_RESULT_CANCELLED;
			goto cancel_out;
		}

		isx012_readb(sd, REG_AF_STATE, &read_value);
		if ((u8)read_value == 8)
			break;

		af_dbg("AF state= %d(0x%X)\n", read_value, read_value);
		msleep_debug(10, false);
	}

	if (unlikely(count >= AF_SEARCH_COUNT)) {
		cam_warn("warning, AF check timeout. val=0x%X\n\n", read_value);
		isx012_writeb(sd, REG_INTCLR, 0x10);
		goto check_fail;
	}

	isx012_writeb(sd, REG_INTCLR, 0x10);
	isx012_readb(sd, REG_AF_RESUNT, &read_value);
	if ((u8)read_value == 0x01)
		success = true;
	else
		af_dbg("AF Fail. AF_RESULT reg=0x%X\n", (u8)read_value);

check_fail:
	if (flash) {
		sr300pc20_readw(sd, REG_ERRSCL_NOW,
			&state->flash.ae_offset.ersc_now);
		sr300pc20_readw(sd, REG_USER_AESCL_NOW,
			&state->flash.ae_offset.ae_now);
		sr300pc20_readw(sd, REG_AESCL, &state->flash.ae_scl);
	}

	if (touch)
		sr300pc20_set_from_table(sd, "af_touch_saf_off",
			&state->regs->af_touch_saf_off, 1, 0);
	else
		sr300pc20_set_from_table(sd, "af_saf_off",
			&state->regs->af_saf_off, 1, 0);

	/* Remove the exising below 66ms delay:
	 * many delays have been added to _saf_off recently*/
	/* msleep_debug(66, true);*/ /* Wait 1V time(66ms) */
	if (state->focus.start == AUTO_FOCUS_OFF) {
		cam_dbg("do_af: AF is cancelled\n");
		state->focus.status = AF_RESULT_CANCELLED;
		goto cancel_out;
	}

	if (flash) {
		sr300pc20_writew(sd, REG_MANOUTGAIN,
			(u16)(state->flash.ae_scl - AE_SCL_SUBRACT_VALUE));
		/*sr300pc20_set_ae_gainoffset(sd);*/
		sr300pc20_flash_torch(sd, SR300PC20_FLASH_OFF);
		state->capture.ae_manual_mode = touch ? 1 : 0;
	}

cancel_out:
	if (state->focus.status == AF_RESULT_CANCELLED) {
		cam_info("Single AF cancelled\n");
		if (flash) {
			sr300pc20_flash_torch(sd, SR300PC20_FLASH_OFF);
			state->flash.preflash = PREFLASH_NONE;
		}

		sr300pc20_cancel_af(sd, flash);
		sr300pc20_return_focus(sd);
	} else {
		if (state->sensor_mode == SENSOR_MOVIE)
			sr300pc20_return_focus(sd);

		state->focus.start = AUTO_FOCUS_OFF;
		state->focus.status = success ?
			AF_RESULT_SUCCESS : AF_RESULT_FAILED;
		cam_info("Single AF finished(0x%X)\n", state->focus.status);

		if (!touch)
			state->focus.lock = 1; /* fix me */
		if (flash)
			state->flash.ae_flash_lock = 1;
	}

	return 0;
}

/* PX: Set AF */
static int sr300pc20_set_af(struct v4l2_subdev *sd, s32 val)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = 0;

	cam_info("set_af: %s, focus %d, touch %d\n",
		val ? "start" : "stop", state->focus.mode, state->focus.touch);

	if (unlikely((u32)val >= AUTO_FOCUS_MAX)) {
		cam_err("%s: error, invalid value(%d)\n", __func__, val);
		return -EINVAL;
	}

	if (state->focus.start == val)
		return 0;

	state->focus.start = val;

	if (val == AUTO_FOCUS_ON) {
		if ((state->runmode != RUNMODE_RUNNING) &&
		    (state->runmode != RUNMODE_RECORDING)) {
			cam_err("error, AF can't start, not in preview\n");
			state->focus.start = AUTO_FOCUS_OFF;
			return -ESRCH;
		}

		err = queue_work(state->workqueue, &state->af_work);
		if (likely(err))
			state->focus.status = AF_RESULT_DOING;
		else
			cam_warn("warning, AF is still processing.\n");
	} else {
		/* Cancel AF */
		cam_info("set_af: AF cancel requested!\n");
	}

	cam_trace("X\n");
	return 0;
}

static int sr300pc20_start_af(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = 0;
	u32 touch, flash_mode;

	mutex_lock(&state->af_lock);
	touch = state->focus.touch;
	state->focus.touch = 0;

	flash_mode = state->flash.mode;

	if (state->sensor_mode == SENSOR_CAMERA) {
		err = sr300pc20_af_start_preflash(sd, touch, flash_mode);
		if (unlikely(err))
			goto out;

		if (state->focus.status == AF_RESULT_CANCELLED)
			goto out;
	}

	sr300pc20_do_af(sd, touch);

out:
	mutex_unlock(&state->af_lock);

	return 0;
}

/* PX: Stop AF */
static int sr300pc20_stop_af(struct v4l2_subdev *sd, s32 touch)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = 0;
	bool flash;

	cam_trace("E\n");
	/* mutex_lock(&state->af_lock); */

	switch (state->focus.status) {
	case AF_RESULT_FAILED:
	case AF_RESULT_SUCCESS:
		cam_info("Stop AF, focus mode %d, AF result %d\n",
			state->focus.mode, state->focus.status);

		flash = ((state->flash.preflash == PREFLASH_ON) ||
				state->flash.ae_flash_lock) ? 1 : 0;

		sr300pc20_cancel_af(sd, flash);

		state->focus.status = AF_RESULT_CANCELLED;
		state->flash.preflash = PREFLASH_NONE;
		state->focus.lock = 0;
		break;

	case AF_RESULT_CANCELLED:
		break;

	default:
		cam_warn("%s: WARNING, unnecessary calling. AF status=%d\n",
			__func__, state->focus.status);
		/* Return 0. */
		goto err_out;
		break;
	}

	if (state->focus.touch)
		state->focus.touch = 0;

	/* mutex_unlock(&state->af_lock); */
	cam_trace("X\n");
	return 0;

err_out:
	/* mutex_unlock(&state->af_lock); */
	return err;
}

static void sr300pc20_af_worker(struct work_struct *work)
{
	struct sr300pc20_state *state = container_of(work, \
			struct sr300pc20_state, af_work);

	sr300pc20_start_af(&state->sd);
}

/* PX: Set focus mode */
static int sr300pc20_set_focus_mode(struct v4l2_subdev *sd, s32 val)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = -EINVAL;
	u32 cancel = 0;
	u8 focus_mode = (u8)val;

	cam_info("set_focus_mode %d(0x%X)\n", val, val);

	if (state->focus.mode == val)
		return 0;

	cancel = (u32)val & FOCUS_MODE_DEFAULT;

	mutex_lock(&state->af_lock);

	if (cancel)
		sr300pc20_stop_af(sd, 0);

	/* check focus mode of lower byte  */
	switch (focus_mode) {
	case FOCUS_MODE_MACRO:
		sr300pc20_set_from_table(sd, "af_macro_mode",
			&state->regs->af_macro_mode, 1, 0);
		if (!cancel)
			sr300pc20_set_from_table(sd, "af_restart",
				&state->regs->af_restart, 1, 0);

		state->focus.mode = focus_mode;
		break;

	case FOCUS_MODE_INFINITY:
	case FOCUS_MODE_AUTO:
	case FOCUS_MODE_FIXED:
		sr300pc20_set_from_table(sd, "af_normal_mode",
			&state->regs->af_normal_mode, 1, 0);
		if (!cancel)
			sr300pc20_set_from_table(sd, "af_restart",
				&state->regs->af_restart, 1, 0);
		state->focus.mode = focus_mode;
		break;

	case FOCUS_MODE_FACEDETECT:
	case FOCUS_MODE_CONTINOUS:
	case FOCUS_MODE_TOUCH:
		break;

	default:
		cam_err("focus mode: error, invalid val(0x%X)\n:", val);
		goto err_out;
		break;
	}

	mutex_unlock(&state->af_lock);
	return 0;

err_out:
	mutex_unlock(&state->af_lock);
	return err;
}

/* PX: */
static int sr300pc20_set_af_window(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	const s32 mapped_x = state->focus.pos_x;
	const s32 mapped_y = state->focus.pos_y;
	const u32 preview_width = state->preview.frmsize->width;
	const u32 preview_height = state->preview.frmsize->height;
	const u32 preview_ratio = FRM_RATIO(state->preview.frmsize);
	u32 start_x, start_y;
	u32 ratio_width, ratio_height;
	struct sr300pc20_rect window = {0, 0, 0, 0};

	cam_trace("E\n");
	mutex_lock(&state->af_lock);

	start_x = mapped_x - DEFAULT_WINDOW_WIDTH / 2;
	start_y = mapped_y - DEFAULT_WINDOW_HEIGHT / 2;
	ratio_width = 2592 * AF_PRECISION / preview_width;
	ratio_height = 1944 * AF_PRECISION / preview_height;

	af_dbg("start_x=%d, start_y=%d, ratio=(%d, %d)\n",
			start_x, start_y, ratio_width, ratio_height);

	/* Calculate x, y, width, height of window */
	window.x = start_x * ratio_width / AF_PRECISION;
	if (preview_ratio == FRMRATIO_HD) {
		/* use width ratio*/
		window.y = (start_y + 60) * ratio_width / AF_PRECISION;
	} else if (preview_ratio == FRMRATIO_VGA)
		window.y = start_y * ratio_height / AF_PRECISION;
	else {
		cam_err("AF window: not supported preview ratio %d\n",
			preview_ratio);
		window.y = start_y * ratio_height / AF_PRECISION;
	}

	window.width = DEFAULT_WINDOW_WIDTH * ratio_width / AF_PRECISION;
	window.height = DEFAULT_WINDOW_HEIGHT * ratio_height / AF_PRECISION;
	af_dbg("window.x=%d, window.y=%d, " \
		"window.width=%d, window.height=%d\n",
		window.x, window.y, window.width, window.height);

	/* Write x, y, width, height of window */
	sr300pc20_writew(sd, 0x6A50, window.x);
	sr300pc20_writew(sd, 0x6A52, window.y);
	sr300pc20_writew(sd, 0x6A54, window.width);
	sr300pc20_writew(sd, 0x6A56, window.height);
	sr300pc20_set_from_table(sd, "af_winddow_set",
		&state->regs->af_winddow_set, 1, 0);

	state->focus.touch = 1;
	mutex_unlock(&state->af_lock);

	cam_info("AF window position completed.\n");
	cam_trace("X\n");

	return 0;
}

static int sr300pc20_set_touch_af(struct v4l2_subdev *sd, s32 val)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = -EIO;

	cam_info("set_touch (%d, %d)\n",
		state->focus.pos_x, state->focus.pos_y);

	if (val) {
		if (mutex_is_locked(&state->af_lock)) {
			cam_warn("%s: WARNING, AF is busy\n", __func__);
			return 0;
		}

		err = queue_work(state->workqueue, &state->af_win_work);
		if (likely(!err))
			cam_warn("WARNING, AF window is still processing\n");
	} else
		cam_err("%s: invalid val(%d)\n", __func__, val);

	cam_trace("X\n");
	return 0;
}

static void sr300pc20_af_win_worker(struct work_struct *work)
{
	struct sr300pc20_state *state = container_of(work, \
				struct sr300pc20_state, af_win_work);
	struct v4l2_subdev *sd = &state->sd;

	sr300pc20_set_af_window(sd);
}

#if 0 /* DSLIM */
static int sr300pc20_init_param(struct v4l2_subdev *sd)
{
	struct v4l2_control ctrl;
	int i;

	for (i = 0; i < ARRAY_SIZE(sr300pc20_ctrls); i++) {
		if (sr300pc20_ctrls[i].value !=
				sr300pc20_ctrls[i].default_value) {
			ctrl.id = sr300pc20_ctrls[i].id;
			ctrl.value = sr300pc20_ctrls[i].value;
			sr300pc20_s_ctrl(sd, &ctrl);
		}
	}

	return 0;
}
#endif

#if 0 /* dslim */
static int sr300pc20_init_regs(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct sr300pc20_state *state = to_state(sd);
	u16 read_value = 0;
	int err = -ENODEV;


	/* we'd prefer to do this in probe, but the framework hasn't
	 * turned on the camera yet so our i2c operations would fail
	 * if we tried to do it in probe, so we have to do it here
	 * and keep track if we succeeded or not.
	 */

	/* enter read mode */
	err = sr300pc20_i2c_write_twobyte(client, 0x002C, 0x7000);
	if (unlikely(err < 0))
		return -ENODEV;

	sr300pc20_i2c_write_twobyte(client, 0x002E, 0x0150);
	sr300pc20_i2c_read_twobyte(client, 0x0F12, &read_value);
	if (likely(read_value == SR300PC20_CHIP_ID))
		cam_info("Sensor ChipID: 0x%04X\n", SR300PC20_CHIP_ID);
	else
		cam_info("Sensor ChipID: 0x%04X, unknown ChipID\n", read_value);

	sr300pc20_i2c_write_twobyte(client, 0x002C, 0x7000);
	sr300pc20_i2c_write_twobyte(client, 0x002E, 0x0152);
	sr300pc20_i2c_read_twobyte(client, 0x0F12, &read_value);
	if (likely(read_value == SR300PC20_CHIP_REV))
		cam_info("Sensor revision: 0x%04X\n", SR300PC20_CHIP_REV);
	else
		cam_info("Sensor revision: 0x%04X, unknown revision\n",
				read_value);

	/* restore write mode */
	err = sr300pc20_i2c_write_twobyte(client, 0x0028, 0x7000);
	CHECK_ERR_COND(err < 0, -ENODEV);

	state->regs = &reg_datas;

	return 0;
}
#endif

static inline int sr300pc20_do_wait_steamoff(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	struct sr300pc20_stream_time *stream_time = &state->stream_time;
	s32 elapsed_msec = 0;
	u32 val = 0;
	int err = 0, count;

	cam_trace("E\n");

	do_gettimeofday(&stream_time->curr_time);

	elapsed_msec = GET_ELAPSED_TIME(stream_time->curr_time, \
				stream_time->before_time) / 1000;

	for (count = 0; count < SR300PC20_CNT_STREAMOFF; count++) {
		err = isx012_readb(sd, 0x00DC, &val);
		CHECK_ERR_MSG(err, "wait_steamoff: error, readb\n")

		if ((val & 0x04) == 0) {
			cam_dbg("wait_steamoff: %dms + cnt(%d)\n",
				elapsed_msec, count);
			break;
		}

		/* cam_info("wait_steamoff: val = 0x%X\n", val);*/
		msleep_debug(2, false);
	}

	if (unlikely(count >= SR300PC20_CNT_STREAMOFF))
		cam_info("wait_steamoff: time-out!\n\n");

	state->need_wait_streamoff = 0;

	return 0;
}

static inline int sr300pc20_fast_capture_switch(struct v4l2_subdev *sd)
{
	u32 val = 0;
	int err = 0, count;

	cam_trace("EX\n");

	for (count = 0; count < SR300PC20_CNT_STREAMOFF; count++) {
		err = isx012_readb(sd, 0x00DC, &val);
		CHECK_ERR_MSG(err, "fast_capture_switch: error, readb\n")

		if ((val & 0x03) == 0x02) {
			cam_dbg("fast_capture_switch: cnt(%d)\n", count);
			break;
		}

		/* cam_info("wait_steamoff: val = 0x%X\n", val);*/
		msleep_debug(2, false);
	}

	if (unlikely(count >= SR300PC20_CNT_STREAMOFF))
		cam_info("fast_capture_switch: time-out!\n\n");

	return 0;
}

static int sr300pc20_wait_steamoff(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	struct sr300pc20_platform_data *pdata = state->pdata;

	if (unlikely(!pdata->common.is_mipi))
		return 0;

	if (state->need_wait_streamoff) {
		sr300pc20_do_wait_steamoff(sd);
		state->need_wait_streamoff = 0;
	} else if (state->format_mode == V4L2_PIX_FMT_MODE_CAPTURE)
		sr300pc20_fast_capture_switch(sd);

	cam_trace("EX\n");

	return 0;
}

static int sr300pc20_control_stream(struct v4l2_subdev *sd, u32 cmd)
{
	struct sr300pc20_state *state = to_state(sd);
	struct sr300pc20_platform_data *pdata = state->pdata;
	int err = -EINVAL;

	if (unlikely(!pdata->common.is_mipi || (cmd != STREAM_STOP)))
		return 0;

	cam_info("STREAM STOP!!\n");
	err = sr300pc20_set_from_table(sd, "stream_stop",
			&state->regs->stream_stop, 1, 0);
	CHECK_ERR_MSG(err, "failed to stop stream\n");

#ifdef CONFIG_VIDEO_IMPROVE_STREAMOFF
	do_gettimeofday(&state->stream_time.before_time);
	state->need_wait_streamoff = 1;
#else
	msleep_debug(pdata->common.streamoff_delay, true);
#endif
	return 0;
}

/* PX: Set flash mode */
static int sr300pc20_set_flash_mode(struct v4l2_subdev *sd, s32 val)
{
	struct sr300pc20_state *state = to_state(sd);

	/* movie flash mode should be set when recording is started */
/*	if (state->sensor_mode == SENSOR_MOVIE && !state->recording)
		return 0;*/

	if (state->flash.mode == val) {
		cam_dbg("the same flash mode=%d\n", val);
		return 0;
	}

	if (val == FLASH_MODE_TORCH)
		sr300pc20_flash_torch(sd, SR300PC20_FLASH_ON);

	if ((state->flash.mode == FLASH_MODE_TORCH)
	    && (val == FLASH_MODE_OFF))
		sr300pc20_flash_torch(sd, SR300PC20_FLASH_OFF);

	state->flash.mode = val;
	cam_dbg("Flash mode = %d\n", val);
	return 0;
}

static int sr300pc20_check_esd(struct v4l2_subdev *sd, s32 val)
{
	u32 data = 0, size_h = 0, size_v = 0;

	sr300pc20_readw(sd, REG_ESD, &data);
#if 0 /* dslim */
	if (data != ESD_VALUE)
		goto esd_out;
#endif

	sr300pc20_readw(sd, REG_HSIZE_MONI, &size_h);
	sr300pc20_readw(sd, REG_VSIZE_MONI, &size_v);
	cam_info("preview size %dx%d\n", size_h, size_v);

	sr300pc20_readw(sd, REG_HSIZE_CAP, &size_h);
	sr300pc20_readw(sd, REG_VSIZE_CAP, &size_v);
	cam_info("capture size %dx%d\n", size_h, size_v);

	cam_info("Check ESD(%d): not detected\n\n", val);
	return 0;

esd_out:
	cam_err("Check ESD(%d): ESD Shock detected! val=0x%X\n\n", data, val);
	return -ERESTART;
}

/* returns the real iso currently used by sensor due to lighting
 * conditions, not the requested iso we sent using s_ctrl.
 */
static inline int sr300pc20_get_exif_iso(struct v4l2_subdev *sd, u16 *iso)
{
	static const u16 iso_table[] = { 0, 25, 32, 40, 50, 64, 80, 100,
		125, 160, 200, 250, 320, 400, 500, 640, 800,
		1000, 1250, 1600};
	u32 val = 0;

	isx012_readb(sd, REG_ISOSENS_OUT, &val);
	if (unlikely(val < 1))
		val = 1;
	else if (unlikely(val > 19))
		val = 19;

	*iso = iso_table[val];

	cam_dbg("reg=%d, ISO=%d\n", val, *iso);
	return 0;
}

/* PX: Set ISO */
static int __used sr300pc20_set_iso(struct v4l2_subdev *sd, s32 val)
{
	struct sr300pc20_state *state = to_state(sd);

retry:
	switch (val) {
	case ISO_AUTO:
	case ISO_50:
		state->shutter_level_flash = 0xB52A;
		break;

	case ISO_100:
		state->shutter_level_flash = 0x9DBA;
		break;

	case ISO_200:
		state->shutter_level_flash = 0x864A;
		break;

	case ISO_400:
		state->shutter_level_flash = 0x738A;
		break;

	default:
		cam_err("set_iso: error, not supported (%d)\n", val);
		val = ISO_AUTO;
		goto retry;
	}

	sr300pc20_set_from_table(sd, "iso", state->regs->iso,
		ARRAY_SIZE(state->regs->iso), val);

	state->iso = val;

	cam_trace("X\n");
	return 0;
}

/* PX: Return exposure time (ms) */
static inline int sr300pc20_get_exif_exptime(struct v4l2_subdev *sd,
						u32 *exp_time)
{
	u32 val_lsb = 0, val_msb = 0;

	sr300pc20_readw(sd, REG_SHT_TIME_OUT_L, &val_lsb);
	sr300pc20_readw(sd, REG_SHT_TIME_OUT_H, &val_msb);

	*exp_time = (val_msb << 16) | (val_lsb & 0xFFFF);
	cam_dbg("exposure time %dus\n", *exp_time);
	return 0;
}

static inline void sr300pc20_get_exif_flash(struct v4l2_subdev *sd,
					u16 *flash)
{
	struct sr300pc20_state *state = to_state(sd);

	*flash = 0;

	switch (state->flash.mode) {
	case FLASH_MODE_OFF:
		*flash |= EXIF_FLASH_MODE_SUPPRESSION;
		break;

	case FLASH_MODE_AUTO:
		*flash |= EXIF_FLASH_MODE_AUTO;
		break;

	case FLASH_MODE_ON:
	case FLASH_MODE_TORCH:
		*flash |= EXIF_FLASH_MODE_FIRING;
		break;

	default:
		break;
	}

	if (state->flash.on)
		*flash |= EXIF_FLASH_FIRED;
}

/* PX: */
static int sr300pc20_get_exif(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	u32 exposure_time = 0;
	u32 int_dec, integer;

	/* exposure time */
	state->exif.exp_time_den = 0;
	sr300pc20_get_exif_exptime(sd, &exposure_time);
	if (exposure_time) {
		state->exif.exp_time_den = 1000 * 1000 / exposure_time;

		int_dec = (1000 * 1000) * 10 / exposure_time;
		integer = state->exif.exp_time_den * 10;

		/* Round off */
		if ((int_dec - integer) > 5)
			state->exif.exp_time_den += 1;
	} else
		state->exif.exp_time_den = 0;

	/* iso */
	state->exif.iso = 0;
	sr300pc20_get_exif_iso(sd, &state->exif.iso);

	/* flash */
	sr300pc20_get_exif_flash(sd, &state->exif.flash);

	cam_dbg("EXIF: ex_time_den=%d, iso=%d, flash=0x%02X\n",
		state->exif.exp_time_den, state->exif.iso, state->exif.flash);

	return 0;
}

/* for debugging */
static int sr300pc20_check_preview_status(struct v4l2_subdev *sd)
{
	u32 reg_val1 = 0;
	u32 reg_val4 = 0;
	u32 reg_val7 = 0;
	u32 reg_val11 = 0;

	/* AE SN */
	isx012_readb(sd, REG_AE_SN1, &reg_val1);
	isx012_readb(sd, REG_AE_SN4, &reg_val4);
	isx012_readb(sd, REG_AE_SN7, &reg_val7);
	isx012_readb(sd, REG_AE_SN11, &reg_val11);

	cam_info("AE_SN[0x%X, 0x%X, 0x%X, 0x%X]"
		, reg_val1, reg_val4, reg_val7, reg_val11);

	return 0;
}

static int sr300pc20_set_preview_size(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	u32 width, height;

	if (!state->preview.update_frmsize)
		return 0;

	if (unlikely(!state->preview.frmsize)) {
		cam_warn("warning, preview resolution not set\n");
		state->preview.frmsize = sr300pc20_get_framesize(
					sr300pc20_preview_frmsizes,
					ARRAY_SIZE(sr300pc20_preview_frmsizes),
					PREVIEW_SZ_XGA);
	}

	width = state->preview.frmsize->width;
	height = state->preview.frmsize->height;

	cam_dbg("set preview size(%dx%d)\n", width, height);

	sr300pc20_writew(sd, REG_HSIZE_MONI, width);
	sr300pc20_writew(sd, REG_VSIZE_MONI, height);

	state->preview.update_frmsize = 0;

	return 0;
}

static int sr300pc20_start_preview(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = -EINVAL;

	cam_info("Camera Preview start, runmode = %d\n", state->runmode);

	if ((state->runmode == RUNMODE_NOTREADY) ||
	    (state->runmode == RUNMODE_CAPTURING)) {
		cam_err("%s: error - Invalid runmode\n", __func__);
		return -EPERM;
	}

	state->focus.status = AF_RESULT_NONE;
	state->flash.preflash = PREFLASH_NONE;
	state->focus.touch = 0;


	err = sr300pc20_set_from_table(sd, "init_reg",
			       &state->regs->init_reg, 1, 0);
	CHECK_ERR_MSG(err, "failed to initialize camera device\n");


#if 0 /* dslim*/
	sr300pc20_set_preview_size(sd);

	if (state->runmode == RUNMODE_CAPTURING_STOP) {
		if (state->scene_mode == SCENE_MODE_NIGHTSHOT)
			sr300pc20_set_from_table(sd, "night_reset",
				&state->regs->lowlux_night_reset, 1, 0);
	}

	sr300pc20_control_stream(sd, STREAM_START);

#ifdef CONFIG_DEBUG_NO_FRAME
	sr300pc20_start_frame_checker(sd);
#endif
#endif

	err = sr300pc20_set_from_table(sd, "preview_mode",
		&state->regs->preview_mode, 1, 0);
	CHECK_ERR_MSG(err, "preview_mode(%d)\n", err);

	state->runmode = (state->sensor_mode == SENSOR_CAMERA) ?
			RUNMODE_RUNNING : RUNMODE_RECORDING;
	return 0;
}

static int sr300pc20_set_capture(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = 0;
	u32 lux = 0, ae_scl;

	if (unlikely((state->flash.preflash == PREFLASH_ON) &&
	    (state->flash.mode == FLASH_MODE_OFF)))
		sr300pc20_post_sensor_flash(sd);

#if 0 /* dslim */
	if (state->capture.ae_manual_mode) {
		/* Check whether fast-AE for preview is needed after capture */

		sr300pc20_readw(sd, REG_AESCL, &ae_scl);
		cam_dbg("set_capture: pre ae_scl = %d, ae_scl = %d\n",
			state->flash.ae_scl, ae_scl);
		if (abs(state->flash.ae_scl - ae_scl) >= AESCL_DIFF_FASTAE)
			state->preview.fast_ae = 1;

		sr300pc20_set_from_table(sd, "ae_manual_mode",
			&state->regs->ae_manual, 1, 0);
	}

	/* Set capture size */
	err = sr300pc20_set_capture_size(sd);
	CHECK_ERR_MSG(err, "fail to set capture size (%d)\n", err);

	/* Set flash */
	switch (state->flash.mode) {
	case FLASH_MODE_AUTO:
		/* 3rd party App could do capturing without AF.*/
		if (state->flash.preflash == PREFLASH_NONE) {
			sr300pc20_get_light_level(sd, &lux);
			if (!sr300pc20_check_flash_fire(sd, lux))
				break;
		} else if (state->flash.preflash == PREFLASH_OFF) {
			/* we re-check lux if capture after touch-AF*/
			if (!state->focus.lock) {
				sr300pc20_get_light_level(sd, &lux);
				if (!sr300pc20_check_flash_fire(sd, lux))
					break;
			} else
				break;
		}
		/* We do not break. */

	case FLASH_MODE_ON:
		sr300pc20_flash_oneshot(sd, SR300PC20_FLASH_ON);

		if (unlikely(state->flash.preflash != PREFLASH_ON)) {
			cam_warn("warning: Flash capture without preflash!!\n\n");
			sr300pc20_set_from_table(sd, "flash_fast_ae_awb",
				&state->regs->flash_fast_ae_awb, 1, 0);
			sr300pc20_wait_ae_stable_cap(sd);
			state->flash.awb_delay = 0;
		} else
			state->flash.awb_delay = 210;
		break;

	case FLASH_MODE_OFF:
	default:
		break;
	}

	/* Transit to capture mode */
	err = sr300pc20_transit_capture_mode(sd);
	CHECK_ERR_MSG(err, "fail to capture_mode (%d)\n", err);
#endif
	return 0;
}

static int sr300pc20_prepare_fast_capture(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = 0;

	cam_info("prepare_fast_capture\n");

	state->req_fmt.width = (state->capture.pre_req >> 16);
	state->req_fmt.height = (state->capture.pre_req & 0xFFFF);
	sr300pc20_set_framesize(sd, sr300pc20_capture_frmsizes,
		ARRAY_SIZE(sr300pc20_capture_frmsizes), false);

	err = sr300pc20_set_capture(sd);
	CHECK_ERR(err);

	state->capture.ready = 1;

	return 0;
}

static int sr300pc20_start_capture(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = -ENODEV, count;
	u32 val = 0;
	u32 night_delay;

	cam_info("start_capture\n");

	if (!state->capture.ready) {
		err = sr300pc20_set_capture(sd);
		CHECK_ERR(err);

		err = sr300pc20_is_cm_changed(sd);
		CHECK_ERR(err);

		sr300pc20_control_stream(sd, STREAM_START);
		night_delay = 500;
	} else
		night_delay = 700; /* for completely skipping 1 frame. */

#ifdef CONFIG_DEBUG_NO_FRAME
	sr300pc20_start_frame_checker(sd);
#endif

	if (state->flash.on && (state->wb.mode == WHITE_BALANCE_AUTO)) {
		msleep_debug(state->flash.awb_delay, true);

		for (count = 0; count < SR300PC20_CNT_CAPTURE_AWB; count++) {
			isx012_readb(sd, REG_AWBSTS, &val);
			if ((val & 0x06) != 0)
				break;

			msleep_debug(30, false);
		}

		if (unlikely(count >= SR300PC20_CNT_CAPTURE_AWB))
			cam_warn("start_capture: fail to check awb\n");
	}

	state->runmode = RUNMODE_CAPTURING;

	if (state->capture.lowlux_night)
		msleep_debug(night_delay, true);

	/* Get EXIF */
	sr300pc20_get_exif(sd);

	return 0;
}

/**
 * sr200pc20_init_regs: Indentify chip and get pointer to reg table
 * @
 */
static int sr300pc20_init_regs(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = -ENODEV;
	u8 read_value = 0;

	err = sr300pc20_writeb(sd, 0x03, 0x00);
	CHECK_ERR_COND(err < 0, -ENODEV);

	sr300pc20_readb(sd, 0x04, &read_value);
	if (SR300PC20_CHIP_ID == read_value)
		cam_info("Sensor ChipID: 0x%02X\n", SR300PC20_CHIP_ID);
	else
		cam_info("Sensor ChipID: 0x%02X, unknown chipID\n", read_value);

	state->regs = &reg_datas;

	return 0;
}


/* PX(NEW) */
static int sr300pc20_s_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct sr300pc20_state *state = to_state(sd);
	s32 previous_index = 0;

	cam_dbg("%s: pixelformat = 0x%x, colorspace = 0x%x, width = %d, height = %d\n",
		__func__, fmt->code, fmt->colorspace, fmt->width, fmt->height);

	v4l2_fill_pix_format(&state->req_fmt, fmt);
	if (fmt->field < IS_MODE_CAPTURE_STILL)
		state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
	else
		state->format_mode = V4L2_PIX_FMT_MODE_CAPTURE;

	if (state->format_mode != V4L2_PIX_FMT_MODE_CAPTURE) {
		previous_index = state->preview.frmsize ?
				state->preview.frmsize->index : -1;
		sr300pc20_set_framesize(sd, sr300pc20_preview_frmsizes,
			ARRAY_SIZE(sr300pc20_preview_frmsizes), true);

		if (previous_index != state->preview.frmsize->index)
			state->preview.update_frmsize = 1;
	} else {
		/*
		 * In case of image capture mode,
		 * if the given image resolution is not supported,
		 * use the next higher image resolution. */
		sr300pc20_set_framesize(sd, sr300pc20_capture_frmsizes,
			ARRAY_SIZE(sr300pc20_capture_frmsizes), false);

		/* for maket app.
		 * Samsung camera app does not use unmatched ratio.*/
		if (unlikely(NULL == state->preview.frmsize)) {
			cam_warn("warning, capture without preview resolution\n");
		} else if (unlikely(FRM_RATIO(state->preview.frmsize)
		    != FRM_RATIO(state->capture.frmsize))) {
			cam_warn("warning, capture ratio " \
				"is different with preview ratio\n");
		}
	}

	return 0;
}

static int sr300pc20_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
					enum v4l2_mbus_pixelcode *code)
{
	cam_dbg("%s: index = %d\n", __func__, index);

	if (index >= ARRAY_SIZE(capture_fmts))
		return -EINVAL;

	*code = capture_fmts[index].code;

	return 0;
}

static int sr300pc20_try_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	int num_entries;
	int i;

	num_entries = ARRAY_SIZE(capture_fmts);

	cam_dbg("%s: code = 0x%x , colorspace = 0x%x, num_entries = %d\n",
		__func__, fmt->code, fmt->colorspace, num_entries);

	for (i = 0; i < num_entries; i++) {
		if (capture_fmts[i].code == fmt->code &&
		    capture_fmts[i].colorspace == fmt->colorspace) {
			cam_info("%s: match found, returning 0\n", __func__);
			return 0;
		}
	}

	cam_err("%s: no match found, returning -EINVAL\n", __func__);
	return -EINVAL;
}


static int sr300pc20_enum_framesizes(struct v4l2_subdev *sd,
				  struct v4l2_frmsizeenum *fsize)
{
	struct sr300pc20_state *state = to_state(sd);

	/*
	* The camera interface should read this value, this is the resolution
	* at which the sensor would provide framedata to the camera i/f
	* In case of image capture,
	* this returns the default camera resolution (VGA)
	*/
	if (state->format_mode != V4L2_PIX_FMT_MODE_CAPTURE) {
		if (unlikely(state->preview.frmsize == NULL)) {
			cam_err("%s: error\n", __func__);
			return -EFAULT;
		}

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->preview.frmsize->width;
		fsize->discrete.height = state->preview.frmsize->height;
	} else {
		if (unlikely(state->capture.frmsize == NULL)) {
			cam_err("%s: error\n", __func__);
			return -EFAULT;
		}

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->capture.frmsize->width;
		fsize->discrete.height = state->capture.frmsize->height;
	}

	return 0;
}

static int sr300pc20_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	return 0;
}

static int sr300pc20_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct sr300pc20_state *state = to_state(sd);

	state->req_fps = param->parm.capture.timeperframe.denominator /
			param->parm.capture.timeperframe.numerator;

	cam_dbg("s_parm state->fps=%d, state->req_fps=%d\n",
		state->fps, state->req_fps);

	if ((state->req_fps < 0) || (state->req_fps > 30)) {
		cam_err("%s: error, invalid frame rate %d. we'll set to 30\n",
				__func__, state->req_fps);
		state->req_fps = 0;
	}

	return sr300pc20_set_frame_rate(sd, state->req_fps);
}

static int sr300pc20_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = 0;

	if (!state->initialized) {
		cam_err("%s: WARNING, camera not initialized\n", __func__);
		return 0;
	}

	mutex_lock(&state->ctrl_lock);

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_EXIF_EXPTIME:
		if (state->sensor_mode == SENSOR_CAMERA)
			ctrl->value = state->exif.exp_time_den;
		else
			ctrl->value = 24;
		break;

	case V4L2_CID_CAMERA_EXIF_ISO:
		if (state->sensor_mode == SENSOR_CAMERA)
			ctrl->value = state->exif.iso;
		else
			ctrl->value = 100;
		break;

	case V4L2_CID_CAMERA_EXIF_FLASH:
		if (state->sensor_mode == SENSOR_CAMERA)
			ctrl->value = state->exif.flash;
		else
			sr300pc20_get_exif_flash(sd, (u16 *)ctrl->value);
		break;

#if !defined(CONFIG_CAM_YUV_CAPTURE)
	case V4L2_CID_CAM_JPEG_MAIN_SIZE:
		ctrl->value = state->jpeg.main_size;
		break;

	case V4L2_CID_CAM_JPEG_MAIN_OFFSET:
		ctrl->value = state->jpeg.main_offset;
		break;

	case V4L2_CID_CAM_JPEG_THUMB_SIZE:
		ctrl->value = state->jpeg.thumb_size;
		break;

	case V4L2_CID_CAM_JPEG_THUMB_OFFSET:
		ctrl->value = state->jpeg.thumb_offset;
		break;

	case V4L2_CID_CAM_JPEG_QUALITY:
		ctrl->value = state->jpeg.quality;
		break;

	case V4L2_CID_CAM_JPEG_MEMSIZE:
		ctrl->value = SENSOR_JPEG_SNAPSHOT_MEMSIZE;
		break;
#endif

	case V4L2_CID_CAMERA_AUTO_FOCUS_RESULT:
		ctrl->value = state->focus.status;
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE:
	case V4L2_CID_CAMERA_EFFECT:
	case V4L2_CID_CAMERA_CONTRAST:
	case V4L2_CID_CAMERA_SATURATION:
	case V4L2_CID_CAMERA_SHARPNESS:
	case V4L2_CID_CAMERA_OBJ_TRACKING_STATUS:
	case V4L2_CID_CAMERA_SMART_AUTO_STATUS:
	default:
		cam_err("%s: WARNING, unknown Ctrl-ID 0x%x\n",
					__func__, ctrl->id);
		err = 0; /* we return no error. */
		break;
	}

	mutex_unlock(&state->ctrl_lock);

	return err;
}

static int sr300pc20_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = 0;

	if (!state->initialized && ctrl->id != V4L2_CID_CAMERA_SENSOR_MODE) {
		cam_warn("%s: WARNING, camera not initialized. ID = %d(0x%X)\n",
			__func__, ctrl->id - V4L2_CID_PRIVATE_BASE,
			ctrl->id - V4L2_CID_PRIVATE_BASE);
		return 0;
	}

	cam_dbg("%s: ID =%d, val = %d\n",
		__func__, ctrl->id - V4L2_CID_PRIVATE_BASE, ctrl->value);

	mutex_lock(&state->ctrl_lock);

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_SENSOR_MODE:
		err = sr300pc20_set_sensor_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_X:
		state->focus.pos_x = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_Y:
		state->focus.pos_y = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_TOUCH_AF_START_STOP:
		err = sr300pc20_set_touch_af(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FOCUS_MODE:
		err = sr300pc20_set_focus_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_SET_AUTO_FOCUS:
		err = sr300pc20_set_af(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_FLASH_MODE:
		err = sr300pc20_set_flash_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_BRIGHTNESS:
		err = sr300pc20_set_exposure(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE:
		err = sr300pc20_set_from_table(sd, "white balance",
			state->regs->white_balance,
			ARRAY_SIZE(state->regs->white_balance), ctrl->value);
		state->wb.mode = ctrl->value;
		break;

	case V4L2_CID_CAMERA_EFFECT:
		err = sr300pc20_set_from_table(sd, "effects",
			state->regs->effect,
			ARRAY_SIZE(state->regs->effect), ctrl->value);
		break;

	case V4L2_CID_CAMERA_METERING:
		err = sr300pc20_set_from_table(sd, "metering",
			state->regs->metering,
			ARRAY_SIZE(state->regs->metering), ctrl->value);
		break;

	case V4L2_CID_CAMERA_CONTRAST:
		err = sr300pc20_set_from_table(sd, "contrast",
			state->regs->contrast,
			ARRAY_SIZE(state->regs->contrast), ctrl->value);
		break;

	case V4L2_CID_CAMERA_SATURATION:
		err = sr300pc20_set_from_table(sd, "saturation",
			state->regs->saturation,
			ARRAY_SIZE(state->regs->saturation), ctrl->value);
		break;

	case V4L2_CID_CAMERA_SHARPNESS:
		err = sr300pc20_set_from_table(sd, "sharpness",
			state->regs->sharpness,
			ARRAY_SIZE(state->regs->sharpness), ctrl->value);
		break;

	case V4L2_CID_CAMERA_SCENE_MODE:
		err = sr300pc20_set_scene_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_AE_LOCK_UNLOCK:
		err = sr300pc20_set_ae_lock(sd, ctrl->value, false);
		break;

	case V4L2_CID_CAMERA_AWB_LOCK_UNLOCK:
		err = sr300pc20_set_awb_lock(sd, ctrl->value, false);
		break;

	case V4L2_CID_CAMERA_CHECK_ESD:
		err = sr300pc20_check_esd(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_ISO:
		err = sr300pc20_set_iso(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_CAPTURE_MODE:
		if (RUNMODE_RUNNING == state->runmode)
			state->capture.pre_req = ctrl->value;

		break;

	case V4L2_CID_CAMERA_ANTI_BANDING:
		break;

	case V4L2_CID_CAMERA_FRAME_RATE:
	default:
		cam_err("%s: WARNING, unknown Ctrl-ID 0x%x\n",
			__func__, ctrl->id);
		/* we return no error. */
		break;
	}

	mutex_unlock(&state->ctrl_lock);
	CHECK_ERR_MSG(err, "s_ctrl failed %d\n", err)

	return 0;
}

static int sr300pc20_s_ext_ctrl(struct v4l2_subdev *sd,
			      struct v4l2_ext_control *ctrl)
{
	return 0;
}

static int sr300pc20_s_ext_ctrls(struct v4l2_subdev *sd,
				struct v4l2_ext_controls *ctrls)
{
	struct v4l2_ext_control *ctrl = ctrls->controls;
	int ret;
	int i;

	for (i = 0; i < ctrls->count; i++, ctrl++) {
		ret = sr300pc20_s_ext_ctrl(sd, ctrl);

		if (ret) {
			ctrls->error_idx = i;
			break;
		}
	}

	return ret;
}

static int sr300pc20_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = 0;

	cam_info("stream mode = %d\n", enable);

	BUG_ON(!state->initialized);

	switch (enable) {
	case STREAM_MODE_CAM_OFF:
		if (state->pdata->common.is_mipi)
			err = sr300pc20_control_stream(sd, STREAM_STOP);
		break;

	case STREAM_MODE_CAM_ON:
		switch (state->sensor_mode) {
		case SENSOR_CAMERA:
			if (state->format_mode == V4L2_PIX_FMT_MODE_CAPTURE)
				err = sr300pc20_start_capture(sd);
			else
				err = sr300pc20_start_preview(sd);
			break;

		case SENSOR_MOVIE:
			err = sr300pc20_start_preview(sd);
			break;

		default:
			break;
		}
		break;

	case STREAM_MODE_MOVIE_ON:
		cam_info("movie on");
		state->recording = 1;
		if (state->flash.mode != FLASH_MODE_OFF)
			sr300pc20_flash_torch(sd, SR300PC20_FLASH_ON);
		break;

	case STREAM_MODE_MOVIE_OFF:
		cam_info("movie off");
		state->recording = 0;
		if (state->flash.on)
			sr300pc20_flash_torch(sd, SR300PC20_FLASH_OFF);
		break;

#ifdef CONFIG_VIDEO_IMPROVE_STREAMOFF
	case STREAM_MODE_WAIT_OFF:
		sr300pc20_wait_steamoff(sd);
		break;
#endif
	default:
		cam_err("%s: error - Invalid stream mode\n", __func__);
		break;
	}

	CHECK_ERR_MSG(err, "failed\n");

	return 0;
}

void sr300pc20_Sensor_Calibration(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int status = 0;
	int temp = 0;

	cam_trace("EX\n");

	/* Read OTP1 */
	sr300pc20_readw(sd, 0x004F, &status);
	boot_dbg("Cal: 0x004F read %x\n", status);

	if ((status & 0x10) == 0x10) {
		/* Read ShadingTable */
		sr300pc20_readw(sd, 0x005C, &status);
		temp = (status&0x03C0)>>6;
		boot_dbg("Cal: Read ShadingTable [0x%x]\n", temp);

		/* Write Shading Table */
		if (temp == 0x0)
			sr300pc20_set_from_table(sd, "Shading_0",
			       &state->regs->shading_0, 1, 0);
		else if (temp == 0x1)
			sr300pc20_set_from_table(sd, "Shading_1",
			       &state->regs->shading_1, 1, 0);
		else if (temp == 0x2)
			sr300pc20_set_from_table(sd, "Shading_2",
			       &state->regs->shading_2, 1, 0);

		/* Write NorR */
		sr300pc20_readw(sd, 0x0054, &status);
		temp = status&0x3FFF;
		boot_dbg("Cal: NorR read : %x\n", temp);
		sr300pc20_writew(sd, 0x6804, temp);

		/* Write NorB */
		sr300pc20_readw(sd, 0x0056, &status);
		temp = status&0x3FFF;
		boot_dbg("Cal: NorB read : %x\n", temp);
		sr300pc20_writew(sd, 0x6806, temp);

		/* Write PreR */
		sr300pc20_readw(sd, 0x005A, &status);
		temp = (status&0x0FFC)>>2;
		boot_dbg("Cal: PreR read : %x\n", temp);
		sr300pc20_writew(sd, 0x6808, temp);

		/* Write PreB */
		sr300pc20_readw(sd, 0x005B, &status);
		temp = (status&0x3FF0)>>4;
		boot_dbg("Cal: PreB read : %x\n", temp);
		sr300pc20_writew(sd, 0x680A, temp);
	} else {
		/* Read OTP0 */
		sr300pc20_readw(sd, 0x0040, &status);
		boot_dbg("Cal: 0x0040 read : %x\n", status);

		if ((status & 0x10) == 0x10) {
			/* Read ShadingTable */
			sr300pc20_readw(sd, 0x004D, &status);
			temp = (status&0x03C0)>>6;
			boot_dbg("Cal: Read ShadingTable [0x%x]\n", temp);

			/* Write Shading Table */
			if (temp == 0x0)
				sr300pc20_set_from_table(sd, "Shading_0",
					&state->regs->shading_0, 1, 0);
			else if (temp == 0x1)
				sr300pc20_set_from_table(sd, "Shading_1",
					&state->regs->shading_1, 1, 0);
			else if (temp == 0x2)
				sr300pc20_set_from_table(sd, "Shading_2",
					&state->regs->shading_2, 1, 0);

			/* Write NorR */
			sr300pc20_readw(sd, 0x0045, &status);
			temp = status&0x3FFF;
			boot_dbg("Cal: NorR read : %x\n", temp);
			sr300pc20_writew(sd, 0x6804, temp);

			/* Write NorB */
			sr300pc20_readw(sd, 0x0047, &status);
			temp = status&0x3FFF;
			boot_dbg("Cal: NorB read : %x\n", temp);
			sr300pc20_writew(sd, 0x6806, temp);

			/* Write PreR */
			sr300pc20_readw(sd, 0x004B, &status);
			temp = (status&0x0FFC)>>2;
			boot_dbg("Cal: PreR read : %x\n", temp);
			sr300pc20_writew(sd, 0x6808, temp);

			/* Write PreB */
			sr300pc20_readw(sd, 0x004C, &status);
			temp = (status&0x3FF0)>>4;
			boot_dbg("Cal: PreB read : %x\n", temp);
			sr300pc20_writew(sd, 0x680A, temp);
		} else
			sr300pc20_set_from_table(sd, "Shading_nocal",
				&state->regs->shading_nocal, 1, 0);
	}
}

static inline int sr300pc20_check_i2c(struct v4l2_subdev *sd, u16 data)
{
	int err;
	u32 val;

	err = sr300pc20_readw(sd, 0x0000, &val);
	if (unlikely(err))
		return err;

	cam_dbg("version: 0x%04X is 0x6017?\n", val);
	return 0;
}

static int sr300pc20_post_poweron(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);
	int err;

	/* It's assumed that Mclk is already enabled */
	cam_trace("E\n");

	err = sr300pc20_check_i2c(sd, 0x1234);
	if (err) {
		cam_err("%s: error, I2C check fail\n", __func__);
		return err;
	}
	cam_dbg("I2C check success!\n");

	msleep_debug(10, false);
	err = sr300pc20_is_om_changed(sd);
	CHECK_ERR(err);

	/* Pre-Sleep */
	cam_dbg("=== Bootup: pre-sleep mode ===\n");
	sr300pc20_set_from_table(sd, "set_pll_4",
		&state->regs->set_pll_4, 1, 0);
	msleep_debug(10, false);
	err = sr300pc20_is_om_changed(sd);
	CHECK_ERR(err);

	/* Sleep */
	cam_dbg("=== Bootup: sleep mode ===\n");
	isx012_writeb(sd, 0x00BF, 0x01);
	/* Negate nSTBY pin */
	sr300pc20_hw_stby_on(sd, false);

	msleep_debug(50, false);
	err = sr300pc20_is_om_changed(sd);
	CHECK_ERR(err);

	/* Active */
	cam_dbg("=== Bootup: active mode ===\n");
	err = sr300pc20_is_cm_changed(sd);
	CHECK_ERR(err);

	isx012_writeb(sd, 0x5008, 0x00);
	sr300pc20_Sensor_Calibration(sd);
	cam_dbg("calibration complete!\n");

	cam_info("POWER ON END\n\n");
	return 0;
}

static void sr300pc20_init_parameter(struct v4l2_subdev *sd)
{
	struct sr300pc20_state *state = to_state(sd);

	state->runmode = RUNMODE_INIT;

	/* Default state values */
	state->scene_mode = SCENE_MODE_NONE;
	state->wb.mode = WHITE_BALANCE_AUTO;
	state->light_level = LUX_LEVEL_MAX;

	/* Set update_frmsize to 1 for case of power reset */
	state->preview.update_frmsize = 1;

	/* Initialize focus field for case of init after power reset. */
	memset(&state->focus, 0, sizeof(state->focus));

#ifdef CONFIG_DEBUG_NO_FRAME
	state->frame_check = false;
#endif
	state->lux_level_flash = LUX_LEVEL_FLASH_ON;
	state->shutter_level_flash = 0x0;

}

static int sr300pc20_init(struct v4l2_subdev *sd, u32 val)
{
	struct sr300pc20_state *state = to_state(sd);
	int err = -EINVAL;

	cam_info("init: start v08(%s)\n", __DATE__);

#ifdef CONFIG_LOAD_FILE
	err = sr300pc20_regs_table_init();
	CHECK_ERR_MSG(err, "loading setfile fail!\n");
#endif

	err = sr300pc20_init_regs(sd);
	CHECK_ERR_MSG(err, "failed to indentify sensor chip\n");

#if 0
	err = sr300pc20_set_from_table(sd, "init_reg",
			       &state->regs->init_reg, 1, 0);
	CHECK_ERR_MSG(err, "failed to initialize camera device\n");
#endif

#ifdef CONFIG_VIDEO_SR300PC20_P8
	sr300pc20_set_from_table(sd, "antibanding",
		&state->regs->antibanding, 1, 0);
#endif

	sr300pc20_init_parameter(sd);
	state->initialized = 1;

	/* Call after setting state->initialized to 1 */
	err = sr300pc20_set_frame_rate(sd, state->req_fps);
	CHECK_ERR(err);

	//sr300pc20_control_stream(sd, STREAM_STOP); /* dslim temp */

	return 0;
}

/*
 * s_config subdev ops
 * With camera device, we need to re-initialize
 * every single opening time therefor,
 * it is not necessary to be initialized on probe time.
 * except for version checking
 * NOTE: version checking is optional
 */
static int sr300pc20_s_config(struct v4l2_subdev *sd,
			int irq, void *platform_data)
{
	struct sr300pc20_state *state = to_state(sd);
	struct sr300pc20_platform_data *pdata = platform_data;
	int i;
#ifdef CONFIG_LOAD_FILE
	int err = 0;
#endif

	if (!pdata) {
		cam_err("%s: error, no platform data\n", __func__);
		return -ENODEV;
	}
	state->pdata = pdata;
	state->dbg_level = &pdata->common.dbg_level;

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	state->req_fmt.width = pdata->common.default_width;
	state->req_fmt.height = pdata->common.default_height;

	if (!pdata->common.pixelformat)
		state->req_fmt.pixelformat = DEFAULT_PIX_FMT;
	else
		state->req_fmt.pixelformat = pdata->common.pixelformat;

	if (!pdata->common.freq)
		state->freq = DEFAULT_MCLK;	/* 24MHz default */
	else
		state->freq = pdata->common.freq;

	state->preview.frmsize = state->capture.frmsize = NULL;
	state->sensor_mode = SENSOR_CAMERA;
	state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
	state->fps = 0;
	state->req_fps = -1;

	/* Initialize the independant HW module like flash here */
	state->flash.mode = FLASH_MODE_OFF;
	state->flash.on = 0;

	for (i = 0; i < ARRAY_SIZE(sr300pc20_ctrls); i++)
		sr300pc20_ctrls[i].value = sr300pc20_ctrls[i].default_value;

#ifdef SR300PC20_SUPPORT_FLASH
	if (sr300pc20_is_hwflash_on(sd))
		state->flash.ignore_flash = 1;
#endif

	state->regs = &reg_datas;

	return 0;
}

static const struct v4l2_subdev_core_ops sr300pc20_core_ops = {
	.init = sr300pc20_init,	/* initializing API */
	.g_ctrl = sr300pc20_g_ctrl,
	.s_ctrl = sr300pc20_s_ctrl,
	.s_ext_ctrls = sr300pc20_s_ext_ctrls,
	/*eset = sr300pc20_reset, */
};

static const struct v4l2_subdev_video_ops sr300pc20_video_ops = {
	.s_mbus_fmt = sr300pc20_s_mbus_fmt,
	.enum_framesizes = sr300pc20_enum_framesizes,
	.enum_mbus_fmt = sr300pc20_enum_mbus_fmt,
	.try_mbus_fmt = sr300pc20_try_mbus_fmt,
	.g_parm = sr300pc20_g_parm,
	.s_parm = sr300pc20_s_parm,
	.s_stream = sr300pc20_s_stream,
};

static const struct v4l2_subdev_ops sr300pc20_ops = {
	.core = &sr300pc20_core_ops,
	.video = &sr300pc20_video_ops,
};


/*
 * sr300pc20_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int sr300pc20_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sr300pc20_state *state;
	int err = -EINVAL;

	state = kzalloc(sizeof(struct sr300pc20_state), GFP_KERNEL);
	if (unlikely(!state)) {
		dev_err(&client->dev, "probe, fail to get memory\n");
		return -ENOMEM;
	}

	mutex_init(&state->ctrl_lock);
	mutex_init(&state->af_lock);

	state->runmode = RUNMODE_NOTREADY;
	sd = &state->sd;
	strcpy(sd->name, SR300PC20_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &sr300pc20_ops);

	state->workqueue = create_workqueue("cam_workqueue");
	if (unlikely(!state->workqueue)) {
		dev_err(&client->dev, "probe, fail to create workqueue\n");
		goto err_out;
	}
	INIT_WORK(&state->af_work, sr300pc20_af_worker);
	INIT_WORK(&state->af_win_work, sr300pc20_af_win_worker);
#ifdef CONFIG_DEBUG_NO_FRAME
	INIT_WORK(&state->frame_work, sr300pc20_frame_checker);
#endif

	err = sr300pc20_s_config(sd, 0, client->dev.platform_data);
	CHECK_ERR_MSG(err, "fail to s_config\n");

	printk(KERN_DEBUG "%s %s: driver probed!!\n",
		dev_driver_string(&client->dev), dev_name(&client->dev));

	return 0;

err_out:
	kfree(state);
	return -ENOMEM;
}

static int sr300pc20_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sr300pc20_state *state = to_state(sd);

	destroy_workqueue(state->workqueue);

	/* do softlanding */
	if (state->initialized)
		sr300pc20_set_af_softlanding(sd);

	/* Check whether flash is on when unlolading driver,
	 * to preventing Market App from controlling improperly flash.
	 * It isn't necessary in case that you power flash down
	 * in power routine to turn camera off.*/
	if (unlikely(state->flash.on && !state->flash.ignore_flash))
		sr300pc20_flash_torch(sd, SR300PC20_FLASH_OFF);

	v4l2_device_unregister_subdev(sd);
	mutex_destroy(&state->ctrl_lock);
	mutex_destroy(&state->af_lock);
	kfree(state);

	printk(KERN_DEBUG "%s %s: driver removed!!\n",
		dev_driver_string(&client->dev), dev_name(&client->dev));
	return 0;
}

static int is_sysdev(struct device *dev, void *str)
{
	return !strcmp(dev_name(dev), (char *)str) ? 1 : 0;
}

static ssize_t sr300pc20_camtype_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const char cam_type[] = "SONY_SR300PC20";
	pr_info("%s\n", __func__);
	return sprintf(buf, "%s\n", cam_type);
}
static DEVICE_ATTR(rear_camtype, S_IRUGO, sr300pc20_camtype_show, NULL);

static ssize_t sr300pc20_camfw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char type[] = "SR300PC20";
	return sprintf(buf, "%s %s\n", type, type);

}
static DEVICE_ATTR(rear_camfw, S_IRUGO, sr300pc20_camfw_show, NULL);

static ssize_t cam_loglevel_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	char temp_buf[60] = {0,};

	sprintf(buf, "Log Level: ");
	if (dbg_level & CAMDBG_LEVEL_TRACE) {
		sprintf(temp_buf, "trace ");
		strcat(buf, temp_buf);
	}

	if (dbg_level & CAMDBG_LEVEL_DEBUG) {
		sprintf(temp_buf, "debug ");
		strcat(buf, temp_buf);
	}

	if (dbg_level & CAMDBG_LEVEL_INFO) {
		sprintf(temp_buf, "info ");
		strcat(buf, temp_buf);
	}

	sprintf(temp_buf, "\n - warn and error level is always on\n\n");
	strcat(buf, temp_buf);

	return strlen(buf);
}

static ssize_t cam_loglevel_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	printk(KERN_DEBUG "CAM buf=%s, count=%d\n", buf, count);

	if (strstr(buf, "trace"))
		dbg_level |= CAMDBG_LEVEL_TRACE;
	else
		dbg_level &= ~CAMDBG_LEVEL_TRACE;

	if (strstr(buf, "debug"))
		dbg_level |= CAMDBG_LEVEL_DEBUG;
	else
		dbg_level &= ~CAMDBG_LEVEL_DEBUG;

	if (strstr(buf, "info"))
		dbg_level |= CAMDBG_LEVEL_INFO;

	return count;
}

static DEVICE_ATTR(loglevel, 0664, cam_loglevel_show, cam_loglevel_store);

static int sr300pc20_create_dbglogfile(struct class *cls)
{
	struct device *dev;
	int err;

	dbg_level |= CAMDBG_LEVEL_DEFAULT;

	dev = class_find_device(cls, NULL, "rear", is_sysdev);
	if (unlikely(!dev)) {
		pr_info("[SR300PC20] can not find rear device\n");
		return 0;
	}

	err = device_create_file(dev, &dev_attr_loglevel);
	if (unlikely(err < 0)) {
		pr_err("cam_init: failed to create device file, %s\n",
			dev_attr_loglevel.attr.name);
	}

	return 0;
}

static const struct i2c_device_id sr300pc20_id[] = {
	{ SR300PC20_DRIVER_NAME, 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, sr300pc20_id);

static struct i2c_driver v4l2_i2c_driver = {
	.driver.name	= SR300PC20_DRIVER_NAME,
	.probe		= sr300pc20_probe,
	.remove		= sr300pc20_remove,
	.id_table	= sr300pc20_id,
};

static int __init v4l2_i2c_drv_init(void)
{
	pr_info("%s: %s called\n", __func__, SR300PC20_DRIVER_NAME); /* dslim*/
	/* sr300pc20_create_file(camera_class); */ /*dslim */
	sr300pc20_create_dbglogfile(camera_class);
	return i2c_add_driver(&v4l2_i2c_driver);
}

static void __exit v4l2_i2c_drv_cleanup(void)
{
	pr_info("%s: %s called\n", __func__, SR300PC20_DRIVER_NAME); /* dslim*/
	i2c_del_driver(&v4l2_i2c_driver);
}

module_init(v4l2_i2c_drv_init);
module_exit(v4l2_i2c_drv_cleanup);

MODULE_DESCRIPTION("SILICONFILE SR300PC20 3MP SOC camera driver");
MODULE_AUTHOR("Dong-Seong Lim <dongseong.lim@samsung.com>");
MODULE_LICENSE("GPL");
