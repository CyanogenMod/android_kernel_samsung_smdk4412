#ifndef __MDNIE_H__
#define __MDNIE_H__

#define END_SEQ			0xffff

enum MODE {
	DYNAMIC,
	STANDARD,
#if !defined(CONFIG_FB_MDNIE_PWM)
	NATURAL,
#endif
	MOVIE,
	MODE_MAX,
};

enum SCENARIO {
	UI_MODE,
	VIDEO_MODE,
	VIDEO_WARM_MODE,
	VIDEO_COLD_MODE,
	CAMERA_MODE,
	NAVI_MODE,
	GALLERY_MODE,
	VT_MODE,
	SCENARIO_MAX,
};

#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_TARGET_LOCALE_NTT)
enum SCENARIO_DMB {
	DMB_NORMAL_MODE = 20,
	DMB_WARM_MODE,
	DMB_COLD_MODE,
	DMB_MODE_MAX,
};
#endif

enum SCENARIO_COLOR_TONE {
	COLOR_TONE_1 = 40,
	COLOR_TONE_2,
	COLOR_TONE_3,
	COLOR_TONE_MAX,
};

enum OUTDOOR {
	OUTDOOR_OFF,
	OUTDOOR_ON,
	OUTDOOR_MAX,
};

enum TONE {
	TONE_NORMAL,
	TONE_WARM,
	TONE_COLD,
	TONE_MAX,
};

enum CABC {
	CABC_OFF,
#if defined(CONFIG_FB_MDNIE_PWM)
	CABC_ON,
#endif
	CABC_MAX,
};

enum POWER_LUT {
	LUT_DEFAULT,
	LUT_VIDEO,
	LUT_MAX,
};

enum NEGATIVE {
	NEGATIVE_OFF,
	NEGATIVE_ON,
	NEGATIVE_MAX,
};

struct mdnie_tunning_info {
	char *name;
	const unsigned short *seq;
};

struct mdnie_tunning_info_cabc {
	char *name;
	const unsigned short *seq;
	unsigned int idx_lut;
};

struct mdnie_info {
	struct device			*dev;
#if defined(CONFIG_FB_MDNIE_PWM)
	struct lcd_platform_data	*lcd_pd;
	struct backlight_device		*bd;
	unsigned int			bd_enable;
#endif
	struct mutex			lock;
	struct mutex			dev_lock;

	unsigned int enable;
	enum SCENARIO scenario;
	enum MODE mode;
	enum TONE tone;
	enum OUTDOOR outdoor;
	enum CABC cabc;
	unsigned int tunning;
	unsigned int negative;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend    early_suspend;
#endif
};

extern struct mdnie_info *g_mdnie;

int mdnie_send_sequence(struct mdnie_info *mdnie, const unsigned short *seq);
extern void set_mdnie_value(struct mdnie_info *mdnie, u8 force);
#if defined(CONFIG_FB_MDNIE_PWM)
extern void set_mdnie_pwm_value(struct mdnie_info *mdnie, int value);
#endif
extern int mdnie_txtbuf_to_parsing(char const *pFilepath);

extern void check_lcd_type(void);
struct mdnie_backlight_value {
	unsigned int max;
	unsigned int mid;
	unsigned char low;
	unsigned char 	dim;
};

#endif /* __MDNIE_H__ */
