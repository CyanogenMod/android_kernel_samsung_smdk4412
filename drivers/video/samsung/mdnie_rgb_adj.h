#include "mdnie.h"

#if defined(CONFIG_CPU_EXYNOS4210)
// kb R	SCR - from mdnie_color_tone_4210.h
#define MDNIE_SCR_START 0x00c8
// yw B
#define MDNIE_SCR_END 0x00d3

#define MDNIE_EFFECT_MASTER 0x0001
#define MDNIE_SCR_SHIFT 6
#define MDNIE_SCR_MASK (1 << MDNIE_SCR_SHIFT)

#define IS_SCR_RED(x) (x >= 0x00c8 && x <= 0x00cb)
#define IS_SCR_GREEN(x) (x >= 0x00cc && x <= 0x00cf)
#define IS_SCR_BLUE(x) (x >= 0x00d0 && x <= 0x00d3)
#else
// SCR RrCr - from mdnie_color_tone.h
#define MDNIE_SCR_START 0x00e1
// SCR KbWb
#define MDNIE_SCR_END 0x00ec

#define MDNIE_EFFECT_MASTER 0x008

#define MDNIE_SCR_SHIFT 5
#define MDNIE_SCR_MASK (1 << MDNIE_SCR_SHIFT)

#define IS_SCR_RED(x) (x % 3 == 0)
#define IS_SCR_GREEN(x) ((x - 1) % 3 == 0)
#define IS_SCR_BLUE(x) ((x - 2) % 3 == 0)
#endif

u16 mdnie_rgb_hook(struct mdnie_info *mdnie, int reg, int val);
void mdnie_send_rgb(struct mdnie_info *mdnie);
unsigned short mdnie_effect_master_hook(struct mdnie_info *mdnie, unsigned short val);
