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
        CYANOGENMOD_MODE,
	UI_MODE,
	VIDEO_MODE,
	VIDEO_WARM_MODE,
	VIDEO_COLD_MODE,
	CAMERA_MODE,
	NAVI_MODE,
	GALLERY_MODE,
	VT_MODE,
	SCENARIO_MAX,
	COLOR_TONE_1 = 40,
	COLOR_TONE_2,
	COLOR_TONE_3,
	COLOR_TONE_MAX,
};

enum SCENARIO_DMB {
	DMB_NORMAL_MODE = 20,
	DMB_WARM_MODE,
	DMB_COLD_MODE,
	DMB_MODE_MAX,
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

enum POWER_LUT_LEVEL {
	LUT_LEVEL_MANUAL_AND_INDOOR,
	LUT_LEVEL_OUTDOOR_1,
	LUT_LEVEL_OUTDOOR_2,
	LUT_LEVEL_MAX,
};

enum NEGATIVE {
	NEGATIVE_OFF,
	NEGATIVE_ON,
	NEGATIVE_MAX,
};

enum ACCESSIBILITY {
	ACCESSIBILITY_OFF,
	NEGATIVE,
	COLOR_BLIND,
	ACCESSIBILITY_MAX,
};

#if defined(CONFIG_FB_EBOOK_PANEL_SCENARIO)
enum EBOOK {
	EBOOK_OFF,
	EBOOK_ON,
	EBOOK_MAX,
};
#endif

struct mdnie_tuning_info {
	char *name;
	unsigned short * const sequence;
};

struct mdnie_backlight_value {
	const unsigned int max;
	const unsigned int mid;
	const unsigned char low;
	const unsigned char dim;
};

struct mdnie_info {
	struct device			*dev;
#if defined(CONFIG_FB_MDNIE_PWM)
	struct lcd_platform_data	*lcd_pd;
	struct backlight_device		*bd;
	unsigned int			bd_enable;
	unsigned int			auto_brightness;
	unsigned int			power_lut_idx;
	struct mdnie_backlight_value	*backlight;
#endif
	struct mutex			lock;
	struct mutex			dev_lock;

	unsigned int enable;
	enum SCENARIO scenario;
	enum MODE mode;
	enum TONE tone;
	enum OUTDOOR outdoor;
	enum CABC cabc;
	unsigned int tuning;
	unsigned int negative;
	unsigned int accessibility;
	unsigned int color_correction;
	char path[50];
#if defined(CONFIG_FB_EBOOK_PANEL_SCENARIO)
	unsigned int ebook;
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend		early_suspend;
#endif
};

extern struct mdnie_info *g_mdnie;

#if defined(CONFIG_FB_MDNIE_PWM)
extern void set_mdnie_pwm_value(struct mdnie_info *mdnie, int value);
#endif
extern int mdnie_calibration(unsigned short x, unsigned short y, int *r);
extern int mdnie_request_firmware(const char *path, u16 **buf, char *name);
extern int mdnie_open_file(const char *path, char **fp);

#endif /* __MDNIE_H__ */
