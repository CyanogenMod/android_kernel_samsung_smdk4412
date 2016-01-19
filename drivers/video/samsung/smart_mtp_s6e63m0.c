/*
 * Jungbae Kim, <jb09.kim@samsung.com>
*/

#include "smart_mtp_s6e63m0.h"
#include "smart_mtp_2p2_gamma.h"

/*
#define SMART_DIMMING_DEBUG
*/

int char_to_int(char data1)
{
	int cal_data;

	if (data1 & 0x80)
		cal_data = 0xffffff00 | data1;
	else
		cal_data = data1;

	return cal_data;
}

#define V255_Coefficient 120
#define V255_denominator 600
int V255_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_255_LSB);
	add_mtp = LSB + V255_300CD_R;
	result_1 = result_2 = (V255_Coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, V255_denominator);
	result_3 = (S6E63M0_VREG0_REF * result_2) >> BIT_SHIFT;
	result_4 = S6E63M0_VREG0_REF - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_255 = result_4;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_0 = S6E63M0_VREG0_REF;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_255_LSB);
	add_mtp = LSB + V255_300CD_G;
	result_1 = result_2 = (V255_Coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, V255_denominator);
	result_3 = (S6E63M0_VREG0_REF * result_2) >> BIT_SHIFT;
	result_4 = S6E63M0_VREG0_REF - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_255 = result_4;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_0 = S6E63M0_VREG0_REF;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_255_LSB);
	add_mtp = LSB + V255_300CD_B;
	result_1 = result_2 = (V255_Coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, V255_denominator);
	result_3 = (S6E63M0_VREG0_REF * result_2) >> BIT_SHIFT;
	result_4 = S6E63M0_VREG0_REF - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_255 = result_4;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_0 = S6E63M0_VREG0_REF;

	#ifdef SMART_DIMMING_DEBUG
	printk("%s V255 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_255,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_255,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_255);
	#endif

	return 0;
}

void V255_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = S6E63M0_VREG0_REF -
		(pSmart->GRAY.TABLE[index[V255_INDEX]].R_Gray);

	result_2 = result_1 * V255_denominator;
	do_div(result_2, S6E63M0_VREG0_REF);
	result_3 = result_2  - V255_Coefficient;
	str[15] = (result_3 & 0xff00) >> 8;
	str[16] = result_3 & 0xff;

	result_1 = S6E63M0_VREG0_REF -
		(pSmart->GRAY.TABLE[index[V255_INDEX]].G_Gray);
	result_2 = result_1 * V255_denominator;
	do_div(result_2, S6E63M0_VREG0_REF);
	result_3 = result_2  - V255_Coefficient;
	str[17] = (result_3 & 0xff00) >> 8;
	str[18] = result_3 & 0xff;

	result_1 = S6E63M0_VREG0_REF -
			(pSmart->GRAY.TABLE[index[V255_INDEX]].B_Gray);
	result_2 = result_1 * V255_denominator;
	do_div(result_2, S6E63M0_VREG0_REF);
	result_3 = result_2  - V255_Coefficient;
	str[19] = (result_3 & 0xff00) >> 8;
	str[20] = result_3 & 0xff;

}

#define V1_Coefficient 5
#define V1_denominator 600
int V1_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_1);
	add_mtp = LSB + V1_300CD_R;
	result_1 = result_2 = (V1_Coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, V1_denominator);
	result_3 = (S6E63M0_VREG0_REF * result_2) >> BIT_SHIFT;
	result_4 = S6E63M0_VREG0_REF - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_1 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_1);
	add_mtp = LSB + V1_300CD_G;
	result_1 = result_2 = (V1_Coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, V1_denominator);
	result_3 = (S6E63M0_VREG0_REF * result_2) >> BIT_SHIFT;
	result_4 = S6E63M0_VREG0_REF - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_1 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_1);
	add_mtp = LSB + V1_300CD_B;
	result_1 = result_2 = (V1_Coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, V1_denominator);
	result_3 = (S6E63M0_VREG0_REF * result_2) >> BIT_SHIFT;
	result_4 = S6E63M0_VREG0_REF - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_1 = result_4;

	#ifdef SMART_DIMMING_DEBUG
	printk("%s V1 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_1,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_1,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_1);
	#endif

	return 0;

}

void V1_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	str[0] =  pSmart->MTP.R_OFFSET.OFFSET_1 + V1_300CD_R;
	str[1] =  pSmart->MTP.G_OFFSET.OFFSET_1 + V1_300CD_G;
	str[2] =  pSmart->MTP.B_OFFSET.OFFSET_1 + V1_300CD_B;
}


#define V171_Coefficient 65
#define V171_denominator 320
int V171_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_171);
	add_mtp = LSB + V171_300CD_R;
	result_1 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1)
				- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_255);
	result_2 = (V171_Coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, V171_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_171 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_171);
	add_mtp = LSB + V171_300CD_G;
	result_1 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1)
				- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_255);
	result_2 = (V171_Coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, V171_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_171 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_171);
	add_mtp = LSB + V171_300CD_B;
	result_1 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1)
				- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_255);
	result_2 = (V171_Coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, V171_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_171 = result_4;;

	#ifdef SMART_DIMMING_DEBUG
	printk("%s V171 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_171,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_171,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_171);
	#endif

	return 0;

}

void V171_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V171_INDEX]].R_Gray);
	result_2 = result_1 * V171_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V255_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[12] = (result_2  - V171_Coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V171_INDEX]].G_Gray);
	result_2 = result_1 * V171_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V255_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[13] = (result_2  - V171_Coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V171_INDEX]].B_Gray);
	result_2 = result_1 * V171_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V255_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[14] = (result_2  - V171_Coefficient) & 0xff;

}


#define V87_Coefficient 65
#define V87_denominator 320
int V87_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_87);
	add_mtp = LSB + V87_300CD_R;
	result_1 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_171);
	result_2 = (V87_Coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, V87_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_87 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_87);
	add_mtp = LSB + V87_300CD_G;
	result_1 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_171);
	result_2 = (V87_Coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, V87_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_87 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_87);
	add_mtp = LSB + V87_300CD_B;
	result_1 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_171);
	result_2 = (V87_Coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, V87_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_87 = result_4;

	#ifdef SMART_DIMMING_DEBUG
	printk("%s V87 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_87,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_87,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_87);
	#endif

	return 0;

}

void V87_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].R_Gray);
	result_2 = result_1 * V87_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V171_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[9] = (result_2  - V87_Coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].G_Gray);
	result_2 = result_1 * V87_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V171_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[10] = (result_2  - V87_Coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].B_Gray);
	result_2 = result_1 * V87_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V171_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[11] = (result_2  - V87_Coefficient) & 0xff;
}

#define V43_Coefficient 65
#define V43_denominator 320
int V43_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_43);
	add_mtp = LSB + V43_300CD_R;
	result_1 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_87);
	result_2 = (V43_Coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, V43_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_43 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_43);
	add_mtp = LSB + V43_300CD_G;
	result_1 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_87);
	result_2 = (V43_Coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, V43_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_43 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_43);
	add_mtp = LSB + V43_300CD_B;
	result_1 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_87);
	result_2 = (V43_Coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, V43_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_43 = result_4;

	#ifdef SMART_DIMMING_DEBUG
	printk("%s V43 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_43,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_43,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_43);
	#endif

	return 0;

}


void V43_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V43_INDEX]].R_Gray);
	result_2 = result_1 * V43_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[6] = (result_2  - V43_Coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V43_INDEX]].G_Gray);
	result_2 = result_1 * V43_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[7] = (result_2  - V43_Coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V43_INDEX]].B_Gray);
	result_2 = result_1 * V43_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[8] = (result_2  - V43_Coefficient) & 0xff;

}


#define V19_Coefficient 65
#define V19_denominator 320
int V19_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_19);
	add_mtp = LSB + V19_300CD_R;
	result_1 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_43);
	result_2 = (V19_Coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, V19_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.R_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_19 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_19);
	add_mtp = LSB + V19_300CD_G;
	result_1 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_43);
	result_2 = (V19_Coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, V19_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.G_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_19 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_19);
	add_mtp = LSB + V19_300CD_B;
	result_1 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_43);
	result_2 = (V19_Coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, V19_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->RGB_OUTPUT.B_VOLTAGE.level_1) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_19 = result_4;

	#ifdef SMART_DIMMING_DEBUG
	printk("%s V19 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_19,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_19,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_19);
	#endif

	return 0;

}

void V19_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V19_INDEX]].R_Gray);
	result_2 = result_1 * V19_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].R_Gray)
			- (pSmart->GRAY.TABLE[index[V43_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[3] = (result_2  - V19_Coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V19_INDEX]].G_Gray);
	result_2 = result_1 * V19_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].G_Gray)
			- (pSmart->GRAY.TABLE[index[V43_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[4] = (result_2  - V19_Coefficient) & 0xff;

	result_1 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V19_INDEX]].B_Gray);
	result_2 = result_1 * V19_denominator;
	result_3 = (pSmart->GRAY.TABLE[1].B_Gray)
			- (pSmart->GRAY.TABLE[index[V43_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[5] = (result_2  - V19_Coefficient) & 0xff;
}

/*V0, V1,V19,V43,V87,V171,V255*/
int S6E63M0_ARRAY[S6E63M0_MAX] = {0, 1, 19, 43, 87, 171, 255};

int non_linear_V1toV19[] = {
75, 69, 63, 57, 51, 46,
41, 36, 31, 27, 23, 19,
15, 12, 9, 6, 3};
#define V1toV19_denominator 81

int non_linear_V19toV43[] = {
101, 94, 87, 80, 74, 68, 62,
56, 51, 46, 41, 36, 32, 28, 24,
20, 17, 14, 11, 8, 6, 4, 2,
};
#define V19toV43_denominator 108


#define V43toV87_Coefficient 43
#define V43toV87_Multiple 1
#define V43toV87_denominator 44

#define V87toV171_Coefficient 83
#define V87toV171_Multiple 1
#define V87toV171_denominator 84

#define V171toV255_Coefficient 83
#define V171toV255_Multiple 1
#define V171toV255_denominator 84

int cal_gray_scale_linear(int up, int low, int coeff,
int mul, int deno, int cnt)
{
	unsigned long long result_1, result_2, result_3, result_4;

	result_1 = up - low;
	result_2 = (result_1 * (coeff - (cnt * mul))) << BIT_SHIFT;
	do_div(result_2, deno);
	result_3 = result_2 >> BIT_SHIFT;
	result_4 = low + result_3;

	return (int)result_4;
}

int cal_gray_scale_non_linear(int up, int low,
int *table, int deno, int cnt)
{
	unsigned long long result_1, result_2, result_3, result_4;

	result_1 = up - low;
	result_2 = (result_1 * (table[cnt])) << BIT_SHIFT;
	do_div(result_2, deno);
	result_3 = result_2 >> BIT_SHIFT;
	result_4 = low + result_3;

	return (int)result_4;
}


int Generate_gray_scale(struct SMART_DIM *pSmart)
{
	int cnt = 0, cal_cnt = 0;
	int array_index = 0;

	for (cnt = 0; cnt < S6E63M0_MAX; cnt++) {
		pSmart->GRAY.TABLE[S6E63M0_ARRAY[cnt]].R_Gray =
			((int *)&(pSmart->RGB_OUTPUT.R_VOLTAGE))[cnt];

		pSmart->GRAY.TABLE[S6E63M0_ARRAY[cnt]].G_Gray =
			((int *)&(pSmart->RGB_OUTPUT.G_VOLTAGE))[cnt];

		pSmart->GRAY.TABLE[S6E63M0_ARRAY[cnt]].B_Gray =
			((int *)&(pSmart->RGB_OUTPUT.B_VOLTAGE))[cnt];
	}

	/*
		below codes use hard coded value.
		So it is possible to modify on each model.
		V0, V1,V19,V43,V87,V171,V255
	*/
	for (cnt = 0; cnt < S6E63M0_GRAY_SCALE_MAX; cnt++) {

		if (cnt == S6E63M0_ARRAY[0]) {
			/* 0 */
			array_index = 0;
		} else if (cnt == S6E63M0_ARRAY[1]) {
			/* 1 */
			cal_cnt = 0;
		} else if ((cnt > S6E63M0_ARRAY[1]) && (cnt < S6E63M0_ARRAY[2])) {
			/* 2 ~ 18 */
			array_index = 2;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_non_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].R_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].R_Gray,
				non_linear_V1toV19, V1toV19_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_non_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].G_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].G_Gray,
				non_linear_V1toV19, V1toV19_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_non_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].B_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].B_Gray,
				non_linear_V1toV19, V1toV19_denominator, cal_cnt);

			cal_cnt++;
		} else if (cnt == S6E63M0_ARRAY[2]) {
			/* 19 */
			cal_cnt = 0;
		} else if ((cnt > S6E63M0_ARRAY[2]) && (cnt < S6E63M0_ARRAY[3])) {
			/* 20 ~ 42 */
			array_index = 3;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_non_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].R_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].R_Gray,
				non_linear_V19toV43, V19toV43_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_non_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].G_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].G_Gray,
				non_linear_V19toV43, V19toV43_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_non_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].B_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].B_Gray,
				non_linear_V19toV43, V19toV43_denominator, cal_cnt);

			cal_cnt++;
		}  else if (cnt == S6E63M0_ARRAY[3]) {
			/* 43 */
			cal_cnt = 0;
		} else if ((cnt > S6E63M0_ARRAY[3]) && (cnt < S6E63M0_ARRAY[4])) {
			/* 44 ~ 86 */
			array_index = 4;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].R_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].R_Gray,
				V43toV87_Coefficient, V43toV87_Multiple,
				V43toV87_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].G_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].G_Gray,
				V43toV87_Coefficient, V43toV87_Multiple,
				V43toV87_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].B_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].B_Gray,
				V43toV87_Coefficient, V43toV87_Multiple,
				V43toV87_denominator , cal_cnt);

			cal_cnt++;
		} else if (cnt == S6E63M0_ARRAY[4]) {
			/* 87 */
			cal_cnt = 0;
		} else if ((cnt > S6E63M0_ARRAY[4]) && (cnt < S6E63M0_ARRAY[5])) {
			/* 88 ~ 170 */
			array_index = 5;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].R_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].R_Gray,
				V87toV171_Coefficient, V87toV171_Multiple,
				V87toV171_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].G_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].G_Gray,
				V87toV171_Coefficient, V87toV171_Multiple,
				V87toV171_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].B_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].B_Gray,
				V87toV171_Coefficient, V87toV171_Multiple,
				V87toV171_denominator, cal_cnt);
			cal_cnt++;

		} else if (cnt == S6E63M0_ARRAY[5]) {
			/* 171 */
			cal_cnt = 0;
		} else if ((cnt > S6E63M0_ARRAY[5]) && (cnt < S6E63M0_ARRAY[6])) {
			/* 172 ~ 254 */
			array_index = 6;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].R_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].R_Gray,
				V171toV255_Coefficient, V171toV255_Multiple,
				V171toV255_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].G_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].G_Gray,
				V171toV255_Coefficient, V171toV255_Multiple,
				V171toV255_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index-1]].B_Gray,
				pSmart->GRAY.TABLE[S6E63M0_ARRAY[array_index]].B_Gray,
				V171toV255_Coefficient, V171toV255_Multiple,
				V171toV255_denominator, cal_cnt);

			cal_cnt++;
		} else {
			if (cnt == S6E63M0_ARRAY[6]) {
				printk("%s end\n", __func__);
			} else {
				printk(KERN_ERR "%s fail cnt:%d \n", __func__, cnt);
				return -1;
			}
		}

	}

	#ifdef SMART_DIMMING_DEBUG
		for (cnt = 0; cnt < S6E63M0_GRAY_SCALE_MAX; cnt++) {
			printk("%s %d R:%d G:%d B:%d\n", __func__, cnt,
				pSmart->GRAY.TABLE[cnt].R_Gray,
				pSmart->GRAY.TABLE[cnt].G_Gray,
				pSmart->GRAY.TABLE[cnt].B_Gray);
		}
	#endif
	return 0;
}


int Smart_dimming_init(struct SMART_DIM *smart_dim)
{
	V255_adjustment(smart_dim);
	V1_adjustment(smart_dim);
	V171_adjustment(smart_dim);
	V87_adjustment(smart_dim);
	V43_adjustment(smart_dim);
	V19_adjustment(smart_dim);

	if (Generate_gray_scale(smart_dim)) {
		printk(KERN_ERR "lcd smart dimming fail Generate_gray_scale\n");
		return -1;
	}

	return 0;
}

int searching_function(long long candela, int *index)
{
	long long delta_1 = 0, delta_2 = 0;
	int cnt;

	/*
	*	This searching_functin should be changed with improved
		searcing algorithm to reduce searching time.
	*/
	*index = -1;

	for (cnt = 0; cnt < (S6E63M0_GRAY_SCALE_MAX-1); cnt++) {
		delta_1 = candela - S6e63m0_curve_2p2[cnt];
		delta_2 = candela - S6e63m0_curve_2p2[cnt+1];

		if (delta_2 < 0) {
			*index = (delta_1 + delta_2) <= 0 ? cnt : cnt+1;
			break;
		}

		if (delta_1 == 0) {
			*index = cnt;
			break;
		}

		if (delta_2 == 0) {
			*index = cnt+1;
			break;
		}
	}

	if (*index == -1)
		return -1;
	else
		return 0;
}

/* -1 means V1 */
#define S6E63M0_TABLE_MAX  (S6E63M0_MAX-1)
void(*Make_hexa[S6E63M0_TABLE_MAX])(int*, struct SMART_DIM*, char*) = {
	V255_hexa,
	V171_hexa,
	V87_hexa,
	V43_hexa,
	V19_hexa,
	V1_hexa
};

void generate_gamma(struct SMART_DIM *pSmart, char *str, int size)
{
	long long candela_level[S6E63M0_TABLE_MAX] = {-1, -1, -1, -1, -1, -1};
	int bl_index[S6E63M0_TABLE_MAX] = {-1 , -1, -1, -1, -1, -1};

	unsigned long long temp_cal_data = 0;
	int bl_level, cnt;
	int level_255_temp = 0;
	int level_255_temp_MSB = 0;

	bl_level = pSmart->brightness_level;

	/*calculate candela level */
	for (cnt = 0; cnt < S6E63M0_TABLE_MAX; cnt++) {
		temp_cal_data =
			((long long)(s6e63m0_candela_coeff[S6E63M0_ARRAY[cnt+1]])) *
			((long long)(bl_level));
		candela_level[cnt] = temp_cal_data;
	}

	#ifdef SMART_DIMMING_DEBUG
	printk("%s candela_1:%llu candela_19:%llu  candela_43:%llu  "
		"candela_87:%llu  candela_171:%llu  candela_255:%llu \n", __func__,
		candela_level[0], candela_level[1], candela_level[2],
		candela_level[3], candela_level[4], candela_level[5]);
	#endif

	/*calculate brightness level*/
	for (cnt = 0; cnt < S6E63M0_TABLE_MAX; cnt++) {
		if (searching_function(candela_level[cnt], &(bl_index[cnt]))) {
			printk("%s searching functioin error cnt:%d\n", __func__, cnt);
		}
	}

	#ifdef SMART_DIMMING_DEBUG
	printk("%s bl_index_1:%d bl_index_19:%d bl_index_43:%d "
		"bl_index_87:%d bl_index_171:%d bl_index_255:%d\n", __func__,
		bl_index[0], bl_index[1], bl_index[2], bl_index[3], bl_index[4], bl_index[5]);
	#endif

	/*Generate Gamma table*/
	for (cnt = 0; cnt < S6E63M0_TABLE_MAX; cnt++) {
		(void)Make_hexa[cnt](bl_index , pSmart, str);
	}

	/*subtration MTP_OFFSET value from generated gamma table*/
	str[3] -= pSmart->MTP.R_OFFSET.OFFSET_19;
	str[4] -= pSmart->MTP.G_OFFSET.OFFSET_19;
	str[5] -= pSmart->MTP.B_OFFSET.OFFSET_19;

	str[6] -= pSmart->MTP.R_OFFSET.OFFSET_43;
	str[7] -= pSmart->MTP.G_OFFSET.OFFSET_43;
	str[8] -= pSmart->MTP.B_OFFSET.OFFSET_43;

	str[9] -= pSmart->MTP.R_OFFSET.OFFSET_87;
	str[10] -= pSmart->MTP.G_OFFSET.OFFSET_87;
	str[11] -= pSmart->MTP.B_OFFSET.OFFSET_87;

	str[12] -= pSmart->MTP.R_OFFSET.OFFSET_171;
	str[13] -= pSmart->MTP.G_OFFSET.OFFSET_171;
	str[14] -= pSmart->MTP.B_OFFSET.OFFSET_171;

	level_255_temp = (str[15] << 8) | str[16] ;
	level_255_temp -=  pSmart->MTP.R_OFFSET.OFFSET_255_LSB;
	level_255_temp_MSB = level_255_temp / 256;
	str[15] = level_255_temp_MSB & 0xff;
	str[16] = level_255_temp & 0xff;

	level_255_temp = (str[17] << 8) | str[18] ;
	level_255_temp -=  pSmart->MTP.G_OFFSET.OFFSET_255_LSB;
	level_255_temp_MSB = level_255_temp / 256;
	str[17] = level_255_temp_MSB & 0xff;
	str[18] = level_255_temp & 0xff;

	level_255_temp = (str[19] << 8) | str[20] ;
	level_255_temp -=  pSmart->MTP.B_OFFSET.OFFSET_255_LSB;
	level_255_temp_MSB = level_255_temp / 256;
	str[19] = level_255_temp_MSB & 0xff;
	str[20] = level_255_temp & 0xff;
}
