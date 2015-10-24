/*
 * Author: andip71, 22.09.2014
 * 
 * Modifications: Yank555.lu 20.08.2013
 *
 * Version 1.6.7
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


/*****************************************/
// External function declarations
/*****************************************/

void Boeffla_sound_hook_wm8994_pcm_probe(struct snd_soc_codec *codec_pointer);
unsigned int Boeffla_sound_hook_wm8994_write(unsigned int reg, unsigned int value);


/*****************************************/
// Definitions
/*****************************************/

// Boeffla sound general
#define BOEFFLA_SOUND_DEFAULT 	0
#define BOEFFLA_SOUND_VERSION 	"1.6.7"

// Debug mode
#define DEBUG_DEFAULT 		1

#define DEBUG_OFF 		0
#define DEBUG_NORMAL 		1
#define DEBUG_VERBOSE 		2

// Debug register
#define DEBUG_REGISTER_KEY 	66

// EQ mode
#define EQ_DEFAULT 		0

#define EQ_OFF			0
#define EQ_NORMAL		1
#define EQ_NOSATPREVENT		2

// EQ gain
#define EQ_GAIN_DEFAULT 	0

#define EQ_GAIN_OFFSET 		12
#define EQ_GAIN_MIN 		-12
#define EQ_GAIN_MAX  		12

#define EQ_GAIN_STUNING_1	-12
#define EQ_GAIN_STUNING_2	4
#define EQ_GAIN_STUNING_3	-1
#define EQ_GAIN_STUNING_4	-3
#define EQ_GAIN_STUNING_5	4

// EQ bands
#define EQ_BAND_1_A_DEFAULT	0x0FCA
#define EQ_BAND_1_B_DEFAULT	0x0400
#define EQ_BAND_1_PG_DEFAULT	0x00D8
#define EQ_BAND_2_A_DEFAULT	0x1EB5
#define EQ_BAND_2_B_DEFAULT	0xF145
#define EQ_BAND_2_C_DEFAULT	0x0B75
#define EQ_BAND_2_PG_DEFAULT	0x01C5
#define EQ_BAND_3_A_DEFAULT	0x1C58
#define EQ_BAND_3_B_DEFAULT	0xF373
#define EQ_BAND_3_C_DEFAULT	0x0A54
#define EQ_BAND_3_PG_DEFAULT	0x0558
#define EQ_BAND_4_A_DEFAULT	0x168E
#define EQ_BAND_4_B_DEFAULT	0xF829
#define EQ_BAND_4_C_DEFAULT	0x07AD
#define EQ_BAND_4_PG_DEFAULT	0x1103
#define EQ_BAND_5_A_DEFAULT	0x0564
#define EQ_BAND_5_B_DEFAULT	0x0559
#define EQ_BAND_5_PG_DEFAULT	0x4000

#define EQ_BAND_1_A_STUNING	0x0FE3
#define EQ_BAND_1_B_STUNING	0x0403
#define EQ_BAND_1_PG_STUNING	0x0074
#define EQ_BAND_2_A_STUNING	0x1F03
#define EQ_BAND_2_B_STUNING	0xF0F9
#define EQ_BAND_2_C_STUNING	0x040A
#define EQ_BAND_2_PG_STUNING	0x03DA
#define EQ_BAND_3_A_STUNING	0x1ED2
#define EQ_BAND_3_B_STUNING	0xF11A
#define EQ_BAND_3_C_STUNING	0x040A
#define EQ_BAND_3_PG_STUNING	0x045D
#define EQ_BAND_4_A_STUNING	0x0E76
#define EQ_BAND_4_B_STUNING	0xFCE4
#define EQ_BAND_4_C_STUNING	0x040A
#define EQ_BAND_4_PG_STUNING	0x330D
#define EQ_BAND_5_A_STUNING	0xFC8F
#define EQ_BAND_5_B_STUNING	0x0400
#define EQ_BAND_5_PG_STUNING	0x323C

// EQ saturation prevention
#define AIF1_DRC1_1_DEFAULT	152
#define AIF1_DRC1_2_DEFAULT	2116
#define AIF1_DRC1_3_DEFAULT	232
#define AIF1_DRC1_4_DEFAULT	528

#define AIF1_DRC1_1_PREVENT	132
#define AIF1_DRC1_2_PREVENT	647
#define AIF1_DRC1_3_PREVENT	232
#define AIF1_DRC1_4_PREVENT	528

#define AIF1_DRC1_1_STUNING	152
#define AIF1_DRC1_2_STUNING	2116
#define AIF1_DRC1_3_STUNING	16
#define AIF1_DRC1_4_STUNING	235

// Speaker tuning
#define SPEAKER_BOOST_DEFAULT	4
#define SPEAKER_BOOST_TUNED	6

// FLL tuning loop gains
#define FLL_LOOP_GAIN_DEFAULT	0
#define FLL_LOOP_GAIN_TUNED	5

// Stereo expansion
#define STEREO_EXPANSION_GAIN_DEFAULT	0
#define STEREO_EXPANSION_GAIN_OFF		0
#define STEREO_EXPANSION_GAIN_MIN		0
#define STEREO_EXPANSION_GAIN_MAX		31

// headphone levels
#define HEADPHONE_DEFAULT 	57

#define HEADPHONE_MAX 		63
#define HEADPHONE_MIN 		0

// speaker levels
#define SPEAKER_DEFAULT 	57

#define SPEAKER_MAX 		63
#define SPEAKER_MIN 		0

// Microphone control
#define MICLEVEL_GENERAL	28
#define MICLEVEL_CALL		25

#define MICLEVEL_MIN		0
#define MICLEVEL_MAX		31

// Register dump
#define REGDUMP_BANKS		4
#define REGDUMP_REGISTERS	300

// General switches
#define ON 	1
#define OFF 	0

// Change delay
#define DEFAULT_CHANGE_DELAY	2000000
#define MIN_CHANGE_DELAY	0
#define MAX_CHANGE_DELAY	5000000
