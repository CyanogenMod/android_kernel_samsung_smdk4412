#ifndef __MDNIE_H__
#define __MDNIE_H__

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define END_SEQ			0xffff

enum MODE {
	DYNAMIC,
	STANDARD,
#if !defined(CONFIG_FB_MDNIE_PWM)
	NATURAL,
#endif
	MOVIE,
#if !defined(CONFIG_CPU_EXYNOS4210)
	AUTO,
#endif
	MODE_MAX
};

enum SCENARIO {
	UI_MODE,
	VIDEO_MODE,
	CAMERA_MODE = 4,
	NAVI_MODE,
	GALLERY_MODE,
	VT_MODE,
#if !defined(CONFIG_CPU_EXYNOS4210)
	BROWSER_MODE,
	EBOOK_MODE,
	EMAIL_MODE,
#endif
	SCENARIO_MAX,
	COLOR_TONE_1 = 40,
	COLOR_TONE_2,
	COLOR_TONE_3,
	COLOR_TONE_MAX
};

enum SCENARIO_DMB {
	DMB_NORMAL_MODE = 20,
	DMB_MODE_MAX
};

enum CABC {
	CABC_OFF,
#if defined(CONFIG_FB_MDNIE_PWM)
	CABC_ON,
#endif
	CABC_MAX
};

enum POWER_LUT {
	LUT_DEFAULT,
	LUT_VIDEO,
	LUT_MAX
};

enum POWER_LUT_LEVEL {
	LUT_LEVEL_MANUAL_AND_INDOOR,
	LUT_LEVEL_OUTDOOR_1,
	LUT_LEVEL_OUTDOOR_2,
	LUT_LEVEL_MAX
};

enum ACCESSIBILITY {
	ACCESSIBILITY_OFF,
	NEGATIVE,
	COLOR_BLIND,
	ACCESSIBILITY_MAX
};

struct mdnie_tuning_info {
	const char *name;
	unsigned short *sequence;
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
	enum CABC cabc;
	unsigned int tuning;
	unsigned int accessibility;
	unsigned int color_correction;
	char path[50];
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend		early_suspend;
#endif
};

extern struct mdnie_info *g_mdnie;

#if defined(CONFIG_FB_MDNIE_PWM)
extern void set_mdnie_pwm_value(struct mdnie_info *mdnie, int value);
#endif
extern int mdnie_calibration(unsigned short x, unsigned short y, int *r);
extern int mdnie_request_firmware(const char *path, u16 **buf, const char *name);
extern int mdnie_open_file(const char *path, char **fp);

#endif /* __MDNIE_H__ */
