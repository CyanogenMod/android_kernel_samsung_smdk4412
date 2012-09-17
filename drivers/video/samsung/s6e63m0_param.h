#ifndef __S6E63M0_PARAM_H__
#define __S6E63M0_PARAM_H__

#define GAMMA_PARAM_SIZE	26
#define ELVSS_PARAM_SIZE	3

#define ELVSS_OFFSET_300		0x00
#define ELVSS_OFFSET_290		0x01
#define ELVSS_OFFSET_280		0x02
#define ELVSS_OFFSET_270		0x03
#define ELVSS_OFFSET_260		0x04
#define ELVSS_OFFSET_250		0x05
#define ELVSS_OFFSET_240		0x06
#define ELVSS_OFFSET_230		0x06
#define ELVSS_OFFSET_220		0x07
#define ELVSS_OFFSET_210		0x08
#define ELVSS_OFFSET_200		0x09
#define ELVSS_OFFSET_190		0x05
#define ELVSS_OFFSET_180		0x05
#define ELVSS_OFFSET_170		0x06
#define ELVSS_OFFSET_160		0x07
#define ELVSS_OFFSET_150		0x08
#define ELVSS_OFFSET_140		0x09
#define ELVSS_OFFSET_130		0x0A
#define ELVSS_OFFSET_120		0x0B
#define ELVSS_OFFSET_110		0x0C
#define ELVSS_OFFSET_100		0x0D
#define ELVSS_OFFSET_090		0x0E
#define ELVSS_OFFSET_080		0x0F
#define ELVSS_OFFSET_070		0x10
#define ELVSS_OFFSET_060		0x11
#define ELVSS_OFFSET_050		0x12

#if defined(CONFIG_MACH_Q1_BD)
#define ELVSS_OFFSET_MAX		ELVSS_OFFSET_300
#define ELVSS_OFFSET_1		ELVSS_OFFSET_210
#define ELVSS_OFFSET_2		ELVSS_OFFSET_100
#define ELVSS_OFFSET_MIN		ELVSS_OFFSET_060
#else
#define ELVSS_OFFSET_MAX		ELVSS_OFFSET_300
#define ELVSS_OFFSET_1		ELVSS_OFFSET_210
#define ELVSS_OFFSET_2		ELVSS_OFFSET_1
#define ELVSS_OFFSET_MIN		ELVSS_OFFSET_050
#endif

#ifdef CONFIG_AID_DIMMING
enum {
	ELVSS_110 = 0,
	ELVSS_120,
	ELVSS_130,
	ELVSS_140,
	ELVSS_150,
	ELVSS_160,
	ELVSS_170,
	ELVSS_180,
	ELVSS_190,
	ELVSS_200,
	ELVSS_210,
	ELVSS_220,
	ELVSS_230,
	ELVSS_240,
	ELVSS_250,
	ELVSS_260,
	ELVSS_270,
	ELVSS_280,
	ELVSS_290,
	ELVSS_300,
	ELVSS_STATUS_MAX,
};

#else
enum {
	ELVSS_MIN = 0,
	ELVSS_1,
	ELVSS_2,
	ELVSS_MAX,
	ELVSS_STATUS_MAX,
};
#endif

enum {
	GAMMA_20CD,
#ifdef CONFIG_AID_DIMMING
	GAMMA_30CD,
#else
	GAMMA_30CD = GAMMA_20CD,
#endif
	GAMMA_40CD,
	GAMMA_50CD,
	GAMMA_60CD,
	GAMMA_70CD,
	GAMMA_80CD,
	GAMMA_90CD,
	GAMMA_100CD,
	GAMMA_110CD,
	GAMMA_120CD,
	GAMMA_130CD,
	GAMMA_140CD,
	GAMMA_150CD,
	GAMMA_160CD,
	GAMMA_170CD,
	GAMMA_180CD,
#ifdef CONFIG_AID_DIMMING
	GAMMA_182CD,
	GAMMA_184CD,
	GAMMA_186CD,
	GAMMA_188CD,
#endif
	GAMMA_190CD,
	GAMMA_200CD,
	GAMMA_210CD,
	GAMMA_220CD,
	GAMMA_230CD,
	GAMMA_240CD,
	GAMMA_250CD,
	GAMMA_290CD,
	GAMMA_300CD = GAMMA_290CD,
	GAMMA_MAX
};

#if 0
static const unsigned char SEQ_APPLY_LEVEL_2_KEY[] = {
	0xFC,
	0x5A, 0x5A
};

static const unsigned char SEQ_APPLY_LEVEL_2[] = {
	0xF0,
	0x5A, 0x5A
};

static const unsigned char SEQ_APPLY_MTP_KEY_ENABLE[] = {
	0xF1,
	0x5A, 0x5A
};

static const unsigned char SEQ_SLEEP_OUT[] = {
	0x11,
	0x00, 0x00
};

static const unsigned char SEQ_DISPLAY_CONDITION_SET[] = {
	0xF2,
	0x80, 0x03, 0x0D
};

static const unsigned char SEQ_GAMMA_UPDATE[] = {
	0xF7, 0x03,
	0x00
};

static const unsigned char SEQ_ELVSS_CONTROL[] = {
	0xB1,
	0x04, 0x00
};

static const unsigned char SEQ_DISPLAY_ON[] = {
	0x29,
	0x00, 0x00
};

static const unsigned char SEQ_DISPLAY_OFF[] = {
	0x28,
	0x00, 0x00
};

static const unsigned char SEQ_STANDBY_ON[] = {
	0x01,
	0x00, 0x00
};

static const unsigned char SEQ_ACL_ON[] = {
	0xC0, 0x01,
	0x00
};

static const unsigned char SEQ_ACL_OFF[] = {
	0xC0, 0x00,
	0x00
};

static const unsigned char SEQ_ELVSS_32[] = {
	0xB1,
	0x04, 0x9F
};

static const unsigned char SEQ_ELVSS_34[] = {
	0xB1,
	0x04, 0x9D
};

static const unsigned char SEQ_ELVSS_38[] = {
	0xB1,
	0x04, 0x99
};

static const unsigned char SEQ_ELVSS_47[] = {
	0xB1,
	0x04, 0x90
};

static const unsigned char *ELVSS_TABLE[] = {
	SEQ_ELVSS_32,
	SEQ_ELVSS_34,
	SEQ_ELVSS_38,
	SEQ_ELVSS_47,
};
#endif

#if defined(CONFIG_MACH_M0_GRANDECTC) || defined(CONFIG_MACH_IRON)
static const unsigned char SEQ_SW_RESET[] = {
	0x01,
	0x00, 0x00
};
#endif

static const unsigned char SEQ_APPLY_LEVEL2_KEY_ENABLE[] = {
	0xF0,
	0x5A, 0x5A
};

static const unsigned char SEQ_APPLY_MTP_KEY_ENABLE[] = {
	0xF1,
	0x5A, 0x5A
};

static const unsigned char SEQ_SLEEP_OUT[] = {
	0x11,
	0x00, 0x00
};

static const unsigned char SEQ_GAMMA_CONDITION_SET[] = {
	0xFA,
	0x02, 0x18, 0x08, 0x24, 0x6B, 0x76, 0x57, 0xBD, 0xC3, 0xB5,
	0xB4, 0xBB, 0xAC, 0xC5, 0xC9, 0xC0, 0x00, 0xB7, 0x00, 0xAB,
	0x00, 0xCF,
};

const unsigned char SEQ_PANEL_CONDITION_SET[] = {
	0xF8,
	0x01, 0x27, 0x27, 0x07, 0x07, 0x54, 0x9F, 0x63, 0x86, 0x1A,
	0x33, 0x0D, 0x00, 0x00,
};

const unsigned char SEQ_DISPLAY_CONDITION_SET1[] = {
	0xF2,
	0x02, 0x03, 0x1C, 0x10, 0x10,
};

const unsigned char SEQ_DISPLAY_CONDITION_SET2[] = {
	0xF7,
	0x10, 0x00, 0x00,
};

static const unsigned char SEQ_GAMMA_UPDATE[] = {
	0xFA, 0x03,
};

static const unsigned char SEQ_ETC_SOURCE_CONTROL[] = {
	0xF6,
	0x00, 0x8E, 0x07,
};

static const unsigned char SEQ_ETC_CONTROL_B3h[] = {
	0xB3, 0x0C,
};

#if 0
static const unsigned char SEQ_ETC_CONTROL_B5h[] = {
	0xB5,
	0x2C, 0x12, 0x0C, 0x0A, 0x10, 0x0E, 0x17, 0x13,
	0x1F, 0x1A, 0x2A, 0x24, 0x1F, 0x1B, 0x1A, 0x17,
	0x2B, 0x26,	0x22, 0x20, 0x3A, 0x34, 0x30, 0x2C,
	0x29, 0x26, 0x25, 0x23, 0x21, 0x20, 0x1E, 0x1E,
};

static const unsigned char SEQ_ETC_CONTROL_B6h[] = {
	0xB6,
	0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x44, 0x44,
	0x55, 0x55, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
};

static const unsigned char SEQ_ETC_CONTROL_B7h[] = {
	0xB7,
	0x2C, 0x12, 0x0C, 0x0A, 0x10, 0x0E, 0x17, 0x13,
	0x1F, 0x1A, 0x2A, 0x24, 0x1F, 0x1B, 0x1A, 0x17,
	0x2B, 0x26, 0x22, 0x20, 0x3A, 0x34, 0x30, 0x2C,
	0x29, 0x26, 0x25, 0x23, 0x21, 0x20, 0x1E, 0x1E,
};

static const unsigned char SEQ_ETC_CONTROL_B8h[] = {
	0xB8,
	0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x44, 0x44,
	0x55, 0x55, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
};

static const unsigned char SEQ_ETC_CONTROL_B9h[] = {
	0xB9,
	0x2C, 0x12, 0x0C, 0x0A, 0x10, 0x0E, 0x17, 0x13,
	0x1F, 0x1A, 0x2A, 0x24, 0x1F, 0x1B, 0x1A, 0x17,
	0x2B, 0x26, 0x22, 0x20, 0x3A, 0x34, 0x30, 0x2C,
	0x29, 0x26, 0x25, 0x23, 0x21, 0x20, 0x1E, 0x1E,
};

static const unsigned char SEQ_ETC_CONTROL_BAh[] = {
	0xBA,
	0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x44, 0x44,
	0x55, 0x55, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
};
#endif

const unsigned char SEQ_STANDBY_ON[] = {
	0x10,
	0x00, 0x00
};

const unsigned char SEQ_DISPLAY_ON[] = {
	0x29,
	0x00, 0x00
};

const unsigned char SEQ_DISPLAY_OFF[] = {
	0x28,
	0x00, 0x00
};

const unsigned char SEQ_ELVSS_SET[] = {
	0xB2,
	0x17, 0x17, 0x17, 0x17,
};

const unsigned char SEQ_ELVSS_ON[] = {
	0xB1, 0x0B,
	0x00,
};


static const unsigned char SEQ_ACL_ON[] = {
	0xC0, 0x01,
	0x00
};

static const unsigned char SEQ_ACL_OFF[] = {
	0xC0, 0x00,
	0x00
};

static const unsigned char SEQ_ELVSS_32[] = {
	0xB1,
	0x04, 0x9F
};

static const unsigned char SEQ_ELVSS_34[] = {
	0xB1,
	0x04, 0x9D
};

static const unsigned char SEQ_ELVSS_38[] = {
	0xB1,
	0x04, 0x99
};

static const unsigned char SEQ_ELVSS_47[] = {
	0xB1,
	0x04, 0x90
};

static const unsigned char *ELVSS_TABLE[] = {
	SEQ_ELVSS_32,
	SEQ_ELVSS_34,
	SEQ_ELVSS_38,
	SEQ_ELVSS_47,
};

#endif /* __S6E63M0_PARAM_H__ */
