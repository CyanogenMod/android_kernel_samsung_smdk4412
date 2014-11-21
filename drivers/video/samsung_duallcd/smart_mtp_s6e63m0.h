#ifndef _SMART_MTP_S6E63M0_H_
#define _SMART_MTP_S6E63M0_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <asm/div64.h>


#define BIT_SHIFT 14
/*
	it means BIT_SHIFT is 14.  pow(2,BIT_SHIFT) is 16384.
	BIT_SHIFT is used for right bit shfit
*/
#define BIT_SHFIT_MUL 16384

#define S6E63M0_GRAY_SCALE_MAX 256

/*4.2*16384  4.5 is VREG1_REF value */
#define S6E63M0_VREG0_REF 68813

/*V0,V1,V19,V43,V87,V171,V255*/
#define S6E63M0_MAX 7

/* PANEL DEPENDET THINGS */
#define V1_300CD_R 0x18
#define V1_300CD_G 0x08
#define V1_300CD_B 0x24

#define V19_300CD_R 0x6B
#define V19_300CD_G 0x76
#define V19_300CD_B 0x57

#define V43_300CD_R 0xBD
#define V43_300CD_G 0xC3
#define V43_300CD_B 0xB5

#define V87_300CD_R 0xB4
#define V87_300CD_G 0xBB
#define V87_300CD_B 0xAC

#define V171_300CD_R 0xC5
#define V171_300CD_G 0xC9
#define V171_300CD_B 0xC0

#define V255_300CD_R 0xB7
#define V255_300CD_G 0xAB
#define V255_300CD_B 0xCF
/* PANEL DEPENDET THINGS END*/


enum {
	V1_INDEX = 0,
	V19_INDEX = 1,
	V43_INDEX = 2,
	V87_INDEX = 3,
	V171_INDEX = 4,
	V255_INDEX = 5,
};

struct GAMMA_LEVEL{
	int level_0;
	int level_1;
	int level_19;
	int level_43;
	int level_87;
	int level_171;
	int level_255;
} __attribute__((packed));

struct RGB_OUTPUT_VOLTARE{
	struct GAMMA_LEVEL R_VOLTAGE;
	struct GAMMA_LEVEL G_VOLTAGE;
	struct GAMMA_LEVEL B_VOLTAGE;
} __attribute__((packed));

struct GRAY_VOLTAGE{
	/*
		This voltage value use 14bit right shit
		it means voltage is divied by 16384.
	*/
	int R_Gray;
	int G_Gray;
	int B_Gray;
} __attribute__((packed));

struct GRAY_SCALE{
	struct GRAY_VOLTAGE TABLE[S6E63M0_GRAY_SCALE_MAX];
} __attribute__((packed));

struct MTP_SET{
	char OFFSET_1;
	char OFFSET_19;
	char OFFSET_43;
	char OFFSET_87;
	char OFFSET_171;
	char OFFSET_255_MSB;
	char OFFSET_255_LSB;
} __attribute__((packed));

struct MTP_OFFSET{
	/*
		MTP_OFFSET is consist of 22 byte.
		First byte is dummy and 21 byte is useful.
	*/
	struct MTP_SET R_OFFSET;
	struct MTP_SET G_OFFSET;
	struct MTP_SET B_OFFSET;
} __attribute__((packed));


struct SMART_DIM{
	struct MTP_OFFSET MTP;
	struct RGB_OUTPUT_VOLTARE RGB_OUTPUT;
	struct GRAY_SCALE GRAY;
	int brightness_level;
} __attribute__((packed));

void generate_gamma(struct SMART_DIM *smart_dim, char *str, int size);
int Smart_dimming_init(struct SMART_DIM *smart_dim);

#endif
