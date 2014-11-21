#ifndef __S6EVR02_PARAM_H__
#define __S6EVR02_PARAM_H__

#define GAMMA_PARAM_SIZE	34
#define ACL_PARAM_SIZE	ARRAY_SIZE(acl_cutoff_33)
#define ELVSS_PARAM_SIZE	ARRAY_SIZE(elvss_control_set_20)
#define AID_PARAM_SIZE	ARRAY_SIZE(SEQ_AOR_CONTROL)

enum {
	GAMMA_20CD,
	GAMMA_30CD,
	GAMMA_40CD,
	GAMMA_50CD,
	GAMMA_60CD,
	GAMMA_70CD,
	GAMMA_80CD,
	GAMMA_90CD,
	GAMMA_100CD,
	GAMMA_102CD,
	GAMMA_104CD,
	GAMMA_106CD,
	GAMMA_108CD,
	GAMMA_110CD,
	GAMMA_120CD,
	GAMMA_130CD,
	GAMMA_140CD,
	GAMMA_150CD,
	GAMMA_160CD,
	GAMMA_170CD,
	GAMMA_180CD,
	GAMMA_182CD,
	GAMMA_184CD,
	GAMMA_186CD,
	GAMMA_188CD,
	GAMMA_190CD,
	GAMMA_200CD,
	GAMMA_210CD,
	GAMMA_220CD,
	GAMMA_230CD,
	GAMMA_240CD,
	GAMMA_250CD,
	GAMMA_300CD,
	GAMMA_MAX
};

static const unsigned char SEQ_APPLY_LEVEL_2_KEY[] = {
	 0xF0,
	 0x5A, 0x5A
};

static const unsigned char SEQ_APPLY_LEVEL_2_KEY_DISABLE[] = {
	 0xF0,
	 0xA5, 0xA5
};

static const unsigned char SEQ_APPLY_LEVEL_3_KEY[] = {
	 0xFC,
	 0x5A, 0x5A
};

static const unsigned char SEQ_APPLY_LEVEL_3_KEY_DISABLE[] = {
	 0xFC,
	 0xA5, 0xA5
};

static const unsigned char SEQ_SLEEP_OUT[] = {
	0x11,
	0x00, 0x00
};

static const unsigned char SEQ_SLEEP_IN[] = {
	0x10,
	0x00, 0x00
};

static const unsigned char SEQ_GAMMA_CONDITION_SET[] = {
	0xCA,
	0x00, 0xFF, 0x01, 0x1C, 0x01, 0x2C, 0xDA, 0xD7, 0xDA, 0xD5,
	0xD2, 0xD6, 0xC1, 0xBC, 0xC2, 0xCA, 0xB9, 0xCB, 0xDC, 0xE5,
	0xDD, 0xDA, 0xD8, 0xDD, 0xBA, 0xA8, 0xC1, 0x6B, 0x20, 0xD7,
	0x02, 0x03, 0x02
};

static const unsigned char SEQ_GAMMA_CONDITION_SET_UB[] = {
	0xCA,
	0x01, 0x27, 0x01, 0x3D, 0x01, 0x47, 0xD1, 0xD7, 0xD1, 0xCA,
	0xCE, 0xCC, 0xC4, 0xB3, 0xB1, 0xA1, 0xB9, 0xB8, 0xA2, 0xCE,
	0xBA, 0xC8, 0xC9, 0xAD, 0x9B, 0x85, 0x53, 0x6A, 0x7E, 0xE3,
	0x09, 0x09, 0x0B
};

static const unsigned char SEQ_GAMMA_UPDATE[] = {
	0xF7,
	0x03, 0x00
};

static const unsigned char SEQ_BRIGHTNESS_CONTROL_ON[] = {
	0x53,
	0x20, 0x00
};

static const unsigned char SEQ_AOR_CONTROL[] = {
	0x51,
	0xFF, 0x00
};

static const unsigned char SEQ_ELVSS_CONDITION_SET_UB[] = {
	0xB6,
	0x08, 0x07
};

static const unsigned char SEQ_AVC2_SET[] = {
	0xF4,
	0x6B, 0x18, 0x95, 0x02, 0x11, 0x8C, 0x77, 0x01, 0x01
};

static const unsigned char SEQ_ELVSS_CONDITION_SET[] = {
	0xB6,
	0x08, 0x07
};

static const unsigned char SEQ_DISPLAY_ON[] = {
	0x29,
	0x00, 0x00
};

static const unsigned char SEQ_DISPLAY_OFF[] = {
	0x28,
	0x00, 0x00
};


enum {
	ELVSS_STATUS_20,
	ELVSS_STATUS_30,
	ELVSS_STATUS_40,
	ELVSS_STATUS_50,
	ELVSS_STATUS_60,
	ELVSS_STATUS_70,
	ELVSS_STATUS_80,
	ELVSS_STATUS_90,
	ELVSS_STATUS_100,
	ELVSS_STATUS_110,
	ELVSS_STATUS_120,
	ELVSS_STATUS_130,
	ELVSS_STATUS_140,
	ELVSS_STATUS_150,
	ELVSS_STATUS_160,
	ELVSS_STATUS_170,
	ELVSS_STATUS_180,
	ELVSS_STATUS_190,
	ELVSS_STATUS_200,
	ELVSS_STATUS_210,
	ELVSS_STATUS_220,
	ELVSS_STATUS_230,
	ELVSS_STATUS_240,
	ELVSS_STATUS_250,
	ELVSS_STATUS_300,
	ELVSS_STATUS_MAX
};

static const unsigned char elvss_control_set_20[] = {
	0xB6, 0x08,
	0x20
};

static const unsigned char elvss_control_set_30[] = {
	0xB6, 0x08,
	0x20
};

static const unsigned char elvss_control_set_40[] = {
	0xB6, 0x08,
	0x20
};

static const unsigned char elvss_control_set_50[] = {
	0xB6, 0x08,
	0x1F
};

static const unsigned char elvss_control_set_60[] = {
	0xB6, 0x08,
	0x1F
};

static const unsigned char elvss_control_set_70[] = {
	0xB6, 0x08,
	0x1F
};

static const unsigned char elvss_control_set_80[] = {
	0xB6, 0x08,
	0x1E
};

static const unsigned char elvss_control_set_90[] = {
	0xB6, 0x08,
	0x1E
};

static const unsigned char elvss_control_set_100[] = {
	0xB6, 0x08,
	0x1C
};

static const unsigned char elvss_control_set_110[] = {
	0xB6, 0x08,
	0x1B
};

static const unsigned char elvss_control_set_120[] = {
	0xB6, 0x08,
	0x19
};

static const unsigned char elvss_control_set_130[] = {
	0xB6, 0x08,
	0x17
};

static const unsigned char elvss_control_set_140[] = {
	0xB6, 0x08,
	0x16
};

static const unsigned char elvss_control_set_150[] = {
	0xB6, 0x08,
	0x14
};

static const unsigned char elvss_control_set_160[] = {
	0xB6, 0x08,
	0x12
};

static const unsigned char elvss_control_set_170[] = {
	0xB6, 0x08,
	0x10
};

static const unsigned char elvss_control_set_180[] = {
	0xB6, 0x08,
	0x0F
};

static const unsigned char elvss_control_set_190[] = {
	0xB6, 0x08,
	0x15
};

static const unsigned char elvss_control_set_200[] = {
	0xB6, 0x08,
	0x14
};

static const unsigned char elvss_control_set_210[] = {
	0xB6, 0x08,
	0x13
};

static const unsigned char elvss_control_set_220[] = {
	0xB6, 0x08,
	0x12
};

static const unsigned char elvss_control_set_230[] = {
	0xB6, 0x08,
	0x11
};

static const unsigned char elvss_control_set_240[] = {
	0xB6, 0x08,
	0x10
};

static const unsigned char elvss_control_set_250[] = {
	0xB6, 0x08,
	0x10
};

static const unsigned char elvss_control_set_300[] = {
	0xB6, 0x08,
	0x0B
};


static const unsigned char *ELVSS_CONTROL_TABLE[ELVSS_STATUS_MAX] = {
	elvss_control_set_20,
	elvss_control_set_30,
	elvss_control_set_40,
	elvss_control_set_50,
	elvss_control_set_60,
	elvss_control_set_70,
	elvss_control_set_80,
	elvss_control_set_90,
	elvss_control_set_100,
	elvss_control_set_110,
	elvss_control_set_120,
	elvss_control_set_130,
	elvss_control_set_140,
	elvss_control_set_150,
	elvss_control_set_160,
	elvss_control_set_170,
	elvss_control_set_180,
	elvss_control_set_190,
	elvss_control_set_200,
	elvss_control_set_210,
	elvss_control_set_220,
	elvss_control_set_230,
	elvss_control_set_240,
	elvss_control_set_250,
	elvss_control_set_300
};


enum {
	ACL_STATUS_0P = 0,
	ACL_STATUS_33P,
	ACL_STATUS_40P,
	ACL_STATUS_50P,
	ACL_STATUS_MAX
};

static const unsigned char SEQ_ACL_OFF[] = {
	0x55, 0x00,
	0x00
};

static const unsigned char acl_cutoff_33[] = {
	0x55, 0x01,
	0x00
};

static const unsigned char acl_cutoff_40[] = {
	0x55, 0x02,
	0x00
};

static const unsigned char acl_cutoff_50[] = {
	0x55, 0x03,
	0x00
};

static const unsigned char *ACL_CUTOFF_TABLE[ACL_STATUS_MAX] = {
	SEQ_ACL_OFF,
	acl_cutoff_33,
	acl_cutoff_40,
	acl_cutoff_50,
};
#endif /* __S6EVR02_PARAM_H__ */
