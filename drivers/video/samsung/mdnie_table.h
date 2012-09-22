#ifndef __MDNIE_TABLE_H__
#define __MDNIE_TABLE_H__

#include "mdnie.h"


static const unsigned short tune_dynamic_gallery[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_dynamic_ui[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_dynamic_video[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_dynamic_vt[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_movie_gallery[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_movie_ui[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_movie_video[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_movie_vt[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_standard_gallery[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_standard_ui[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_standard_video[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_standard_vt[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_natural_gallery[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_natural_ui[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_natural_video[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_natural_vt[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_camera[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_camera_outdoor[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_cold[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_cold_outdoor[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_normal_outdoor[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_warm[] = {
	END_SEQ, 0x0000,
};

static const unsigned short tune_warm_outdoor[] = {
	END_SEQ, 0x0000,
};

struct mdnie_tunning_info etc_table[CABC_MAX][OUTDOOR_MAX][TONE_MAX] = {
	{
		{
			{"NORMAL",		NULL},
			{"WARM",		tune_warm},
			{"COLD",		tune_cold},
		},
		{
			{"NORMAL_OUTDOOR",	tune_normal_outdoor},
			{"WARM_OUTDOOR",	tune_warm_outdoor},
			{"COLD_OUTDOOR",	tune_cold_outdoor},
		},
	}
};

#if defined(CONFIG_FB_MDNIE_PWM)
struct mdnie_tunning_info_cabc tunning_table[CABC_MAX][MODE_MAX][SCENARIO_MAX] = {
	{
		{
			{"DYNAMIC_UI",			tune_dynamic_ui,		0},
			{"DYNAMIC_VIDEO",		tune_dynamic_video,	LUT_VIDEO},
			{"DYNAMIC_VIDEO",		tune_dynamic_video,	LUT_VIDEO},
			{"DYNAMIC_VIDEO",		tune_dynamic_video,	LUT_VIDEO},
			{"CAMERA",			NULL/*tune_camera*/,		0},
			{"DYNAMIC_UI",			tune_dynamic_ui,		0},
			{"DYNAMIC_GALLERY",		tune_dynamic_gallery,	0},
			{"DYNAMIC_VT",			tune_dynamic_vt,		0},
		}, {
			{"STANDARD_UI",			tune_standard_ui,		0},
			{"STANDARD_VIDEO",		tune_standard_video,	LUT_VIDEO},
			{"STANDARD_VIDEO",		tune_standard_video,	LUT_VIDEO},
			{"STANDARD_VIDEO",		tune_standard_video,	LUT_VIDEO},
			{"CAMERA",			NULL/*tune_camera*/,		0},
			{"STANDARD_UI",			tune_standard_ui,		0},
			{"STANDARD_GALLERY",		tune_standard_gallery,	0},
			{"STANDARD_VT",			tune_standard_vt,		0},
		}, {
			{"MOVIE_UI",			tune_movie_ui,			0},
			{"MOVIE_VIDEO",			tune_movie_video,	LUT_VIDEO},
			{"MOVIE_VIDEO",			tune_movie_video,	LUT_VIDEO},
			{"MOVIE_VIDEO",			tune_movie_video,	LUT_VIDEO},
			{"CAMERA",			NULL/*tune_camera*/,		0},
			{"MOVIE_UI",			tune_movie_ui,			0},
			{"MOVIE_GALLERY",		tune_movie_gallery,		0},
			{"MOVIE_VT",			tune_movie_vt,		0},
		},
	},
	{
		{
			{"DYNAMIC_UI_CABC",		tune_dynamic_ui,		0},
			{"DYNAMIC_VIDEO_CABC",		tune_dynamic_video,	LUT_VIDEO},
			{"DYNAMIC_VIDEO_CABC",		tune_dynamic_video,	LUT_VIDEO},
			{"DYNAMIC_VIDEO_CABC",		tune_dynamic_video,	LUT_VIDEO},
			{"CAMERA",			NULL/*tune_camera*/,		0},
			{"DYNAMIC_UI_CABC",		tune_dynamic_ui,		0},
			{"DYNAMIC_GALLERY_CABC",	tune_dynamic_gallery,	0},
			{"DYNAMIC_VT_CABC",		tune_dynamic_vt,		0},
		}, {
			{"STANDARD_UI_CABC",		tune_standard_ui,		0},
			{"STANDARD_VIDEO_CABC",		tune_standard_video,	LUT_VIDEO},
			{"STANDARD_VIDEO_CABC",		tune_standard_video,	LUT_VIDEO},
			{"STANDARD_VIDEO_CABC",		tune_standard_video,	LUT_VIDEO},
			{"CAMERA",			NULL/*tune_camera*/,		0},
			{"STANDARD_UI_CABC",		tune_standard_ui,		0},
			{"STANDARD_GALLERY_CABC",	tune_standard_gallery,	0},
			{"STANDARD_VT_CABC",		tune_standard_vt,		0},
		}, {
			{"MOVIE_UI_CABC",		tune_movie_ui,			0},
			{"MOVIE_VIDEO_CABC",		tune_movie_video,	LUT_VIDEO},
			{"MOVIE_VIDEO_CABC",		tune_movie_video,	LUT_VIDEO},
			{"MOVIE_VIDEO_CABC",		tune_movie_video,	LUT_VIDEO},
			{"CAMERA",			NULL/*tune_camera*/,		0},
			{"MOVIE_UI_CABC",		tune_movie_ui,			0},
			{"MOVIE_GALLERY_CABC",		tune_movie_gallery,		0},
			{"MOVIE_VT_CABC",		tune_movie_vt,		0},
		},
	},
};
#else
struct mdnie_tunning_info tunning_table[CABC_MAX][MODE_MAX][SCENARIO_MAX] = {
	{
		{
			{"DYNAMIC_UI",			tune_dynamic_ui},
			{"DYNAMIC_VIDEO",		tune_dynamic_video},
			{"DYNAMIC_VIDEO",		tune_dynamic_video},
			{"DYNAMIC_VIDEO",		tune_dynamic_video},
			{"CAMERA",			NULL/*tune_camera*/},
			{"DYNAMIC_UI",			tune_dynamic_ui},
			{"DYNAMIC_GALLERY",		tune_dynamic_gallery},
			{"DYNAMIC_VT",			tune_dynamic_vt},
		}, {
			{"STANDARD_UI",			tune_standard_ui},
			{"STANDARD_VIDEO",		tune_standard_video},
			{"STANDARD_VIDEO",		tune_standard_video},
			{"STANDARD_VIDEO",		tune_standard_video},
			{"CAMERA",			NULL/*tune_camera*/},
			{"STANDARD_UI",			tune_standard_ui},
			{"STANDARD_GALLERY",		tune_standard_gallery},
			{"STANDARD_VT",			tune_standard_vt},
		}, {
			{"NATURAL_UI",			tune_natural_ui},
			{"NATURAL_VIDEO",		tune_natural_video},
			{"NATURAL_VIDEO",		tune_natural_video},
			{"NATURAL_VIDEO",		tune_natural_video},
			{"CAMERA",			NULL/*tune_camera*/},
			{"NATURAL_UI",			tune_natural_ui},
			{"NATURAL_GALLERY",		tune_natural_gallery},
			{"NATURAL_VT",			tune_natural_vt},
		}, {
			{"MOVIE_UI",			tune_movie_ui},
			{"MOVIE_VIDEO",			tune_movie_video},
			{"MOVIE_VIDEO",			tune_movie_video},
			{"MOVIE_VIDEO",			tune_movie_video},
			{"CAMERA",			NULL/*tune_camera*/},
			{"MOVIE_UI",			tune_movie_ui},
			{"MOVIE_GALLERY",		tune_movie_gallery},
			{"MOVIE_VT",			tune_movie_vt},
		},
	}
};
#endif

struct mdnie_tunning_info camera_table[OUTDOOR_MAX] = {
	{"CAMERA",		tune_camera},
	{"CAMERA_OUTDOOR",	tune_camera_outdoor},
};

#endif /* __MDNIE_TABLE_H__ */
