#ifndef __S6E8AA0_PARAM_H__
#define __S6E8AA0_PARAM_H__

#define GAMMA_PARAM_SIZE	26
#define ELVSS_PARAM_SIZE	3

#ifdef CONFIG_AID_DIMMING
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
#define ELVSS_OFFSET_190		0x0A
#define ELVSS_OFFSET_180		0x05
#define ELVSS_OFFSET_170		0x06
#define ELVSS_OFFSET_160		0x07
#define ELVSS_OFFSET_150		0x08
#define ELVSS_OFFSET_140		0x09
#define ELVSS_OFFSET_130		0x0A
#define ELVSS_OFFSET_120		0x0B
#define ELVSS_OFFSET_110		0x0C
#define ELVSS_OFFSET_100		0x0D

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
#define ELVSS_OFFSET_300		0x00
#define ELVSS_OFFSET_200		0x08
#define ELVSS_OFFSET_160		0x0D
#if defined(CONFIG_S6E8AA0_AMS529HA01)
#define ELVSS_OFFSET_100		0x11
#else
#define ELVSS_OFFSET_100		0x12
#endif

#if defined(CONFIG_S6E8AA0_AMS529HA01)
#define ELVSS_OFFSET_MAX		ELVSS_OFFSET_300
#define ELVSS_OFFSET_1		ELVSS_OFFSET_200
#define ELVSS_OFFSET_2		ELVSS_OFFSET_160
#define ELVSS_OFFSET_MIN		ELVSS_OFFSET_100
#else
#define ELVSS_OFFSET_MAX		ELVSS_OFFSET_300
#define ELVSS_OFFSET_1		ELVSS_OFFSET_160
#define ELVSS_OFFSET_2		ELVSS_OFFSET_1
#define ELVSS_OFFSET_MIN		ELVSS_OFFSET_100
#endif

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

#endif /* __S6E8AA0_PARAM_H__ */
