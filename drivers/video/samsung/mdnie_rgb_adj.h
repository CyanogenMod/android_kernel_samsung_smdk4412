#include "mdnie.h"

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

u16 mdnie_rgb_hook(struct mdnie_info *mdnie, int reg, int val);
void mdnie_send_rgb(struct mdnie_info *mdnie);
unsigned short mdnie_effect_master_hook(struct mdnie_info *mdnie, unsigned short val);
