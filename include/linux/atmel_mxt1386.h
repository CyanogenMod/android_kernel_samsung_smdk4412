/*
 *  Atmel maXTouch header file
 *
 *  Copyright (c) 2010 Iiro Valkonen <iiro.valkonen@atmel.com>
 *  Copyright (c) 2010 Ulf Samuelsson <ulf.samuelsson@atmel.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 or 3 as
 *  published by the Free Software Foundation.
 *  See the file "COPYING" in the main directory of this archive
 *  for more details.
 *
 */

#ifndef _LINUX_MXT1386_H
#define _LINUX_MXT1386_H
#include <linux/wakelock.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

/*Avoid Touch lockup due to wrong auto calibratoin*/
/*Loose the calibration threshold to recalibrate easily at anti-touch
 * for 4seconds after wakeup,
 * and tighten the calibration threshold to recalibrate at least at idle time
 * to avoid calibration repetition problem
 */
#define MXT_CALIBRATE_WORKAROUND

/*For Performance*/
#define MXT_FACTORY_TEST

/*Normal Feature*/
#define MXT_SLEEP_POWEROFF
#define MXT_ERROR_WORKAROUND

#define MXT_I2C_APP_ADDR   0x4c
#define MXT_I2C_BOOTLOADER_ADDR 0x26

/*botton_right, botton_left, center, top_right, top_left*/
#define MXT1386_MAX_CHANNEL	1386
#define MXT1386_PAGE_SIZE		64
#define MXT1386_PAGE_SIZE_SLAVE		8
#define MXT1386_MAX_PAGE	((MXT1386_PAGE_SIZE_SLAVE * 3) - 1)
#define MXT1386_PAGE_WIDTH			14
#define MXT1386_MIN_REF_VALUE	4840
#define MXT1386_MAX_REF_VALUE	13500

#define MXT_I2C_SPEED_KHZ  400
#define MXT_I2C_MAX_LENGTH 300

#define	MAXTOUCH_FAMILYID				0xA0/*0x80*/
#define MXT224_CAL_VARIANTID				0x01
#define MXT224_UNCAL_VARIANTID                          0x00

#define	MXT_MAX_X_VAL_12_BIT				4095
#define	MXT_MAX_Y_VAL_12_BIT				4095
#define	MXT_MAX_X_VAL_10_BIT				1023
#define	MXT_MAX_Y_VAL_10_BIT				1023


#define MXT_MAX_REPORTED_WIDTH                          255
#define MXT_MAX_REPORTED_PRESSURE                       255


#define MXT_MAX_TOUCH_SIZE                              255
#define MXT_MAX_NUM_TOUCHES                             10  /* 10 finger */

/* Fixed addresses inside maXTouch device */
#define	MXT_ADDR_INFO_BLOCK				0
#define	MXT_ADDR_OBJECT_TABLE				7
#define MXT_ID_BLOCK_SIZE                               7
#define	MXT_OBJECT_TABLE_ELEMENT_SIZE			6
/* Object types */
/*#define	MXT_DEBUG_DELTAS_T2				2*/
/*#define	MXT_DEBUG_REFERENCES_T3				3*/
#define	MXT_GEN_MESSAGEPROCESSOR_T5			5
#define	MXT_GEN_COMMANDPROCESSOR_T6			6
#define	MXT_GEN_POWERCONFIG_T7				7
#define	MXT_GEN_ACQUIRECONFIG_T8			8
#define	MXT_TOUCH_MULTITOUCHSCREEN_T9			9
#define	MXT_TOUCH_KEYARRAY_T15				15
#define	MXT_SPT_COMMSCONFIG_T18				18
/*#define	MXT_SPT_GPIOPWM_T19				19*/
/*#define	MXT_PROCI_GRIPFACESUPPRESSION_T20		20*/
#define	MXT_PROCG_NOISESUPPRESSION_T22			22
/*#define	MXT_TOUCH_PROXIMITY_T23				23*/
#define	MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24		24
#define	MXT_SPT_SELFTEST_T25				25
/*#define	MXT_DEBUG_CTERANGE_T26				26*/
#define	MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27		27
#define	MXT_SPT_CTECONFIG_T28				28
#define	MXT_DEBUG_DIAGNOSTICS_T37			37
#define	MXT_USER_INFO_T38				38
#define	MXT_GEN_EXTENSION_T39				39
#define	MXT_PROCI_GRIPSUPPRESSION_T40			40
#define	MXT_PROCI_PALMSUPPRESSION_T41			41
#define	MXT_SPT_DIGITIZER_T43				43
#define	MXT_MESSAGECOUNT_T44				44

#define	MXT_MAX_OBJECT_TYPES				45/*40*/

#define	MXT_END_OF_MESSAGES				0xFF

/* Configuration Object Adress Fields */
/* GEN_MESSAGEPROCESSOR_T5  Address Definitions		*/
/* T5 does not have any configuration */

/* GEN_COMMANDPROCESSOR_T6  Address Definitions		*/
#define	MXT_ADR_T6_RESET				0x00
#define	MXT_ADR_T6_BACKUPNV				0x01
#define	MXT_ADR_T6_CALIBRATE				0x02
#define	MXT_ADR_T6_REPORTALL				0x03
#define	MXT_ADR_T6_RESERVED				0x04
#define	MXT_ADR_T6_DIAGNOSTICS				0x05
/* T6 Diagnostics Debug Command */
#define	MXT_CMD_T6_PAGE_UP		0x01
#define	MXT_CMD_T6_PAGE_DOWN		0x02
#define	MXT_CMD_T6_DELTAS_MODE		0x10
#define	MXT_CMD_T6_REFERENCES_MODE	0x11
#define	MXT_CMD_T6_CTE_MODE		0x31

/* GEN_POWERCONFIG_T7 Address Definitions		*/
#define	MXT_ADR_T7_IDLEACQINT				0x00
#define	MXT_ADR_T7_ACTVACQINT				0x01
#define	MXT_ADR_T7_ACTV2IDLETO				0x02

/* GEN_ACQUIRECONFIG_T8 Address Definitions		*/
#define	MXT_ADR_T8_CHRGTIME				0x00
#define	MXT_ADR_T8_RESERVED				0x01
#define	MXT_ADR_T8_TCHDRIFT				0x02
#define	MXT_ADR_T8_DRIFTSTS				0x03
#define	MXT_ADR_T8_TCHAUTOCAL				0x04
#define	MXT_ADR_T8_SYNC					0x05
#define	MXT_ADR_T8_ATCHCALST				0x06
#define	MXT_ADR_T8_ATCHCALSTHR				0x07
#define	MXT_ADR_T8_ATCHFRCCALTHR			0x08
#define	MXT_ADR_T8_ATCHFRCCALRATIO			0x09

/* TOUCH_MULTITOUCHSCREEN_T9 Address Definitions	*/
#define	MXT_ADR_T9_CTRL					0x00
#define		MXT_T9_CFGB_ENABLE(x)		(((x) >> 0) & 0x01)
#define		MXT_T9_CFGB_RPRTEN(x)		(((x) >> 1) & 0x01)
#define		MXT_T9_CFGB_DISAMP(x)		(((x) >> 2) & 0x01)
#define		MXT_T9_CFGB_DISVECT(x)		(((x) >> 3) & 0x01)
#define		MXT_T9_CFGB_DISMOVE(x)		(((x) >> 4) & 0x01)
#define		MXT_T9_CFGB_DISREL(x)		(((x) >> 5) & 0x01)
#define		MXT_T9_CFGB_DISPRSS(x)		(((x) >> 6) & 0x01)

#define		MXT_T9_ENABLE			(0x01)
#define		MXT_T9_RPRTEN			(0x02)
#define		MXT_T9_DISAMP			(0x04)
#define		MXT_T9_DISVECT			(0x08)
#define		MXT_T9_DISMOVE			(0x10)
#define		MXT_T9_DISREL			(0x20)
#define		MXT_T9_DISPRSS			(0x40)
#define	MXT_ADR_T9_XORIGIN				0x01
#define	MXT_ADR_T9_YORIGIN				0x02
#define	MXT_ADR_T9_XSIZE				0x03
#define	MXT_ADR_T9_YSIZE				0x04
#define	MXT_ADR_T9_AKSCFG				0x05
#define	MXT_ADR_T9_BLEN					0x06
#define		MXT_T9_CFGBF_BL(x)		(x & 0x0F)
#define		MXT_T9_CFGBF_GAIN(x)		((x >> 4) & 0x0F)
#define	MXT_ADR_T9_TCHTHR				0x07
#define	MXT_ADR_T9_TCHDI				0x08
#define	MXT_ADR_T9_ORIENT				0x09
#define		MXT_T9_CFGB_SWITCH(x)		(((x) >> 0) & 0x01)
#define		MXT_T9_CFGB_INVERTX(x)		(((x) >> 1) & 0x01)
#define		MXT_T9_CFGB_INVERTY(x)		(((x) >> 2) & 0x01)
#define	MXT_ADR_T9_MRGTIMEOUT				0x0a
#define	MXT_ADR_T9_MOVHYSTI				0x0b
#define	MXT_ADR_T9_MOVHYSTN				0x0c
#define	MXT_ADR_T9_MOVFILTER				0x0d
#define		MXT_T9_CFGBF_ADAPTTHR(x)	(((x) >> 0) & 0xF)
#define		MXT_T9_CFGB_DISABLE(x)		(((x) >> 7) & 0x01)
#define	MXT_ADR_T9_NUMTOUCH				0x0e
#define	MXT_ADR_T9_MRGHYST				0x0f
#define	MXT_ADR_T9_MRGTHR				0x10
#define	MXT_ADR_T9_AMPHYST				0x11
/* 16 bit */
#define	MXT_ADR_T9_XRANGE				0x12
/* 16 bit */
#define	MXT_ADR_T9_YRANGE				0x14
#define	MXT_ADR_T9_XLOCLIP				0x16
#define	MXT_ADR_T9_XHICLIP				0x17
#define	MXT_ADR_T9_YLOCLIP				0x18
#define	MXT_ADR_T9_YHICLIP				0x19
#define	MXT_ADR_T9_XEDGECTRL				0x1a
#define	MXT_ADR_T9_XEDGEDIST				0x1b
#define	MXT_ADR_T9_YEDGECTRL				0x1c
#define	MXT_ADR_T9_YEDGEDIST				0x1d

/* TOUCH_KEYARRAY_T15 Address Definitions		*/
#define	MXT_ADR_T15_CTRL				0x00
#define		MXT_T15_CFGB_ENABLE(x)		(((x) >> 0) & 0x01)
#define		MXT_T15_CFGB_RPRTEN(x)		(((x) >> 1) & 0x01)
#define		MXT_T15_CFGB_INTAKSEN(x)	(((x) >> 7) & 0x01)
#define	MXT_ADR_T15_XORIGIN				0x01
#define	MXT_ADR_T15_YORIGIN				0x02
#define	MXT_ADR_T15_XSIZE				0x03
#define	MXT_ADR_T15_YSIZE				0x04
#define	MXT_ADR_T15_AKSCFG				0x05
#define	MXT_ADR_T15_BLEN				0x06
#define		MXT_T15_CFGBF_BL(x)		(x & 0x0F)
#define		MXT_T15_CFGBF_GAIN(x)		((x >> 4) & 0x0F)
#define	MXT_ADR_T15_TCHTHR				0x07
#define	MXT_ADR_T15_TCHDI				0x08
#define	MXT_ADR_T15_RESERVED1				0x09
#define	MXT_ADR_T15_RESERVED2				0x0a

/* Adress Definitions for SPT_GPIOPWM_T19 Address Definitions		*/
#define	MXT_ADR_T19_CTRL				0x00
#define	MXT_ADR_T19_REPORTMASK				0x01
#define	MXT_ADR_T19_DIR					0x02
#define	MXT_ADR_T19_INTPULLUP				0x03
#define	MXT_ADR_T19_OUT					0x04
#define	MXT_ADR_T19_WAKE				0x05
#define	MXT_ADR_T19_PWM					0x06
#define	MXT_ADR_T19_PERIOD				0x07
/* 32 bit */
#define	MXT_ADR_T19_DUTY				0x08

/* PROCI_GRIPFACESUPPRESSION_T20 Address Definitions		*/
#define	MXT_ADR_T20_CTRL				0x00
#define	MXT_ADR_T20_XLOGRIP				0x01
#define	MXT_ADR_T20_XHIGRIP				0x02
#define	MXT_ADR_T20_YLOGRIP				0x03
#define	MXT_ADR_T20_YHIGRIP				0x04
#define	MXT_ADR_T20_MAXTCHS				0x05
#define	MXT_ADR_T20_RESERVED				0x06
#define	MXT_ADR_T20_SZTHR1				0x07
#define	MXT_ADR_T20_SZTHR2				0x08
#define	MXT_ADR_T20_SHPTHR1				0x09
#define	MXT_ADR_T20_SHPTHR2				0x0a
#define	MXT_ADR_T20_SUPEXTTO				0x0b

/* PROCG_NOISESUPPRESSION_T22 Address Definitions		*/
#define	MXT_ADR_T22_CTRL				0x00
/* 16 bit */
#define	MXT_ADR_T22_RESERVED1_2				0x01
/* 16 bit */
#define	MXT_ADR_T22_GCAFUL				0x03
/* 16 bit */
#define	MXT_ADR_T22_GCAFLL				0x05
#define	MXT_ADR_T22_ACTVGCAFVALID			0x07
#define	MXT_ADR_T22_NOISETHR				0x08
#define	MXT_ADR_T22_RESERVED9				0x09
#define	MXT_ADR_T22_FREQHOPSCALE			0x0a
/* 5 bytes */
#define MXT_ADR_T22_FREQ				0x0b
#define MXT_ADR_T22_IDLEGCAFVALID			0x10

/* TOUCH_PROXIMITY_T23 Address Definitions		*/
#define	MXT_ADR_T23_CTRL				0x00
#define	MXT_ADR_T23_XORIGIN				0x01
#define	MXT_ADR_T23_YORIGIN				0x02
#define	MXT_ADR_T23_XSIZE				0x03
#define	MXT_ADR_T23_YSIZE				0x04
#define	MXT_ADR_T23_RESERVED				0x05
#define	MXT_ADR_T23_BLEN				0x06
#define	MXT_ADR_T23_TCHTHR				0x07
#define	MXT_ADR_T23_TCHDI				0x09
#define	MXT_ADR_T23_AVERAGE				0x0a
/* 16 bit */
#define	MXT_ADR_T23_RATE				0x0b

/* PROCI_ONETOUCHGESTUREPROCESSOR_T24 Address Definitions		*/
#define	MXT_ADR_T24_CTRL				0x00
#define	MXT_ADR_T24_NUMGEST				0x01
/* 16 bit */
#define	MXT_ADR_T24_GESTEN				0x02
#define	MXT_ADR_T24_PRESSPROC				0x04
#define	MXT_ADR_T24_TAPTO				0x05
#define	MXT_ADR_T24_FLICKTO				0x06
#define	MXT_ADR_T24_DRAGTO				0x07
#define	MXT_ADR_T24_SPRESSTO				0x08
#define	MXT_ADR_T24_LPRESSTO				0x09
#define	MXT_ADR_T24_REPPRESSTO				0x0a
/* 16 bit */
#define	MXT_ADR_T24_FLICKTHR				0x0b
/* 16 bit */
#define	MXT_ADR_T24_DRAGTHR				0x0d
/* 16 bit */
#define	MXT_ADR_T24_TAPTHR				0x0f
/* 16 bit */
#define	MXT_ADR_T24_THROWTHR				0x11

/* SPT_SELFTEST_T25 Address Definitions		*/
#define	MXT_ADR_T25_CTRL				0x00
#define	MXT_ADR_T25_CMD					0x01
/* 16 bit */
#define	MXT_ADR_T25_HISIGLIM0				0x02
/* 16 bit */
#define	MXT_ADR_T25_LOSIGLIM0				0x04

/* PROCI_TWOTOUCHGESTUREPROCESSOR_T27 Address Definitions		*/
#define	MXT_ADR_T27_CTRL				0x00
#define	MXT_ADR_T27_NUMGEST				0x01
#define	MXT_ADR_T27_RESERVED2				0x02
#define	MXT_ADR_T27_GESTEN				0x03
#define	MXT_ADR_T27_ROTATETHR				0x04

/* 16 bit */
#define	MXT_ADR_T27_ZOOMTHR				0x05

/* SPT_CTECONFIG_T28 Address Definitions		*/
#define	MXT_ADR_T28_CTRL				0x00
#define	MXT_ADR_T28_CMD					0x01
#define	MXT_ADR_T28_MODE				0x02
#define	MXT_ADR_T28_IDLEGCAFDEPTH			0x03
#define	MXT_ADR_T28_ACTVGCAFDEPTH			0x04

/* DEBUG_DIAGNOSTICS_T37 Address Definitions		*/
#define	MXT_ADR_T37_MODE				0x00
#define	MXT_ADR_T37_PAGE				0x01
#define	MXT_ADR_T37_DATA				0x02

/************************************************************************
 * MESSAGE OBJECTS ADDRESS FIELDS
 *
 ************************************************************************/
#define MXT_MSG_REPORTID                                0x00


/* MXT_GEN_MESSAGEPROCESSOR_T5 Message address definitions		*/
#define	MXT_MSG_T5_REPORTID				0x00
#define	MXT_MSG_T5_MESSAGE				0x01
#define	MXT_MSG_T5_CHECKSUM				0x08

/* MXT_GEN_COMMANDPROCESSOR_T6 Message address definitions		*/
#define	MXT_MSG_T6_STATUS				0x01
#define		MXT_MSGB_T6_COMSERR		0x04
#define		MXT_MSGB_T6_CFGERR		0x08
#define		MXT_MSGB_T6_CAL			0x10
#define		MXT_MSGB_T6_SIGERR		0x20
#define		MXT_MSGB_T6_OFL			0x40
#define		MXT_MSGB_T6_RESET		0x80
/* Three bytes */
#define	MXT_MSG_T6_CHECKSUM				0x02

/* MXT_GEN_POWERCONFIG_T7 NO Message address definitions		*/
/* MXT_GEN_ACQUIRECONFIG_T8 Message address definitions			*/
/* MXT_TOUCH_MULTITOUCHSCREEN_T9 Message address definitions		*/

#define	MXT_MSG_T9_STATUS				0x01
/* Status bit field */
#define		MXT_MSGB_T9_SUPPRESS		0x02
#define		MXT_MSGB_T9_AMP			0x04
#define		MXT_MSGB_T9_VECTOR		0x08
#define		MXT_MSGB_T9_MOVE		0x10
#define		MXT_MSGB_T9_RELEASE		0x20
#define		MXT_MSGB_T9_PRESS		0x40
#define		MXT_MSGB_T9_DETECT		0x80

#define	MXT_MSG_T9_XPOSMSB				0x02
#define	MXT_MSG_T9_YPOSMSB				0x03
#define	MXT_MSG_T9_XYPOSLSB				0x04
#define	MXT_MSG_T9_TCHAREA				0x05
#define	MXT_MSG_T9_TCHAMPLITUDE				0x06
#define	MXT_MSG_T9_TCHVECTOR				0x07

/* MXT_TOUCH_KEYARRAY_T15 Message address definitions			*/
#define	MXT_MSG_T15_STATUS				0x01
#define		MXT_MSGB_T15_DETECT		0x80
/* 4 bytes */
#define	MXT_MSG_T15_KEYSTATE				0x02

/* MXT_SPT_GPIOPWM_T19 Message address definitions			*/
#define	MXT_MSG_T19_STATUS				0x01

/* MXT_PROCI_GRIPFACESUPPRESSION_T20 Message address definitions	*/
#define	MXT_MSG_T20_STATUS				0x01
#define		MXT_MSGB_T20_FACE_SUPPRESS	0x01
/* MXT_PROCG_NOISESUPPRESSION_T22 Message address definitions		*/
#define	MXT_MSG_T22_STATUS				0x01
#define		MXT_MSGB_T22_FHCHG		0x01
#define		MXT_MSGB_T22_GCAFERR		0x04
#define		MXT_MSGB_T22_FHERR		0x08
#define	MXT_MSG_T22_GCAFDEPTH				0x02

/* MXT_TOUCH_PROXIMITY_T23 Message address definitions			*/
#define	MXT_MSG_T23_STATUS				0x01
#define		MXT_MSGB_T23_FALL		0x20
#define		MXT_MSGB_T23_RISE		0x40
#define		MXT_MSGB_T23_DETECT		0x80
/* 16 bit */
#define	MXT_MSG_T23_PROXDELTA				0x02

/* MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24 Message address definitions	*/
#define	MXT_MSG_T24_STATUS				0x01
#define	MXT_MSG_T24_XPOSMSB				0x02
#define	MXT_MSG_T24_YPOSMSB				0x03
#define	MXT_MSG_T24_XYPOSLSB				0x04
#define	MXT_MSG_T24_DIR					0x05
/* 16 bit */
#define	MXT_MSG_T24_DIST				0x06

/* MXT_SPT_SELFTEST_T25 Message address definitions			*/
#define	MXT_MSG_T25_STATUS				0x01
/* 5 Bytes */
#define		MXT_MSGR_T25_OK			0xFE
#define		MXT_MSGR_T25_INVALID_TEST	0xFD
#define		MXT_MSGR_T25_PIN_FAULT		0x11
#define		MXT_MSGR_T25_SIGNAL_LIMIT_FAULT	0x17
#define		MXT_MSGR_T25_GAIN_ERROR		0x20
#define	MXT_MSG_T25_INFO				0x02

/* MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27 Message address definitions	*/
#define	MXT_MSG_T27_STATUS			0x01
#define		MXT_MSGB_T27_ROTATEDIR		0x10
#define		MXT_MSGB_T27_PINCH		0x20
#define		MXT_MSGB_T27_ROTATE		0x40
#define		MXT_MSGB_T27_STRETCH		0x80
#define	MXT_MSG_T27_XPOSMSB			0x02
#define	MXT_MSG_T27_YPOSMSB			0x03
#define	MXT_MSG_T27_XYPOSLSB			0x04
#define	MXT_MSG_T27_ANGLE			0x05

/* 16 bit */
#define	MXT_MSG_T27_SEPARATION				0x06

/* MXT_SPT_CTECONFIG_T28 Message address definitions			*/
#define	MXT_MSG_T28_STATUS				0x01
#define	MXT_MSGB_T28_CHKERR		0x01

/* MXT_DEBUG_DIAGNOSTICS_T37 NO Message address definitions		*/

/* One Touch Events */
#define	MT_GESTURE_RESERVED				0x00
#define	MT_GESTURE_PRESS				0x01
#define	MT_GESTURE_RELEASE				0x02
#define	MT_GESTURE_TAP					0x03
#define	MT_GESTURE_DOUBLE_TAP				0x04
#define	MT_GESTURE_FLICK				0x05
#define	MT_GESTURE_DRAG					0x06
#define	MT_GESTURE_SHORT_PRESS				0x07
#define	MT_GESTURE_LONG_PRESS				0x08
#define	MT_GESTURE_REPEAT_PRESS				0x09
#define	MT_GESTURE_TAP_AND_PRESS			0x0a
#define	MT_GESTURE_THROW				0x0b

/* reset mode */
#define RESET_TO_NORMAL 0
#define RESET_TO_BOOTLOADER 1

/* Bootloader states */
#define WAITING_BOOTLOAD_COMMAND   0xC0
#define WAITING_FRAME_DATA         0x80
#define FRAME_CRC_CHECK            0x02
#define FRAME_CRC_PASS             0x04
#define FRAME_CRC_FAIL             0x03
#define APP_CRC_FAIL	0x40
#define BOOTLOAD_STATUS_MASK	0x3f  /* 0011 1111*/

#define MXT_MAX_FRAME_SIZE	532/*276*/

/* Firmware */
#define MXT1386_FIRMWARE "mxt1386.fw"

/* level of debugging messages */
#define DEBUG_INFO      1
#define DEBUG_VERBOSE   2
#define DEBUG_MESSAGES  5
#define DEBUG_RAW       8
#define DEBUG_TRACE     10

#define TSP_STATE_INACTIVE		-1
#define TSP_STATE_RELEASE		0
#define TSP_STATE_PRESS		1
#define TSP_STATE_MOVE		2

extern struct class *sec_class;

/* Device Info descriptor */
/* Parsed from maXTouch "Id information" inside device */
struct mxt_device_info {
	u8	family_id;
	u8	variant_id;
	u8	major;
	u8	minor;
	u8	build;
	u8	num_objs;
	u8	x_size;
	u8	y_size;
	u8	family[16];	/* Family name */
	u8	variant[16];	/* Variant name */
	u16	num_nodes;	/* Number of sensor nodes */
};

/* object descriptor table, parsed from maXTouch "object table" */
struct mxt_object {
	u8	type;
	u16	chip_addr;
	u8	size;
	u8	instances;
	u8	num_report_ids;
};

/* Mapping from report id to object type and instance */
struct report_id_map {
	u8  object;
	u8  instance;
/*
 * This is the first report ID belonging to object. It enables us to
 * find out easily the touch number: each touch has different report
 * ID (which are assigned to touches in increasing order). By
 * subtracting the first report ID from current, we get the touch
 * number.
 */
	u8  first_rid;
};


/*mxt configuration data*/
__packed struct gen_commandprocessor_t6_config_t{
	/* Force chip reset             */
	uint8_t reset;
	/* Force backup to eeprom/flash */
	uint8_t backupnv;
	/* Force recalibration          */
	uint8_t calibrate;
	/* Force all objects to report  */
	uint8_t reportall;
	uint8_t reserved;
	/* Controls the diagnostic object */
	uint8_t diagnostic;
};

__packed struct gen_powerconfig_t7_config_t{
	/* Idle power mode sleep length in ms           */
	uint8_t idleacqint;
	/* Active power mode sleep length in ms         */
	uint8_t actvacqint;
	 /* Active to idle power mode delay length in units of 0.2s*/
	uint8_t actv2idleto;
};

__packed struct gen_acquisitionconfig_t8_config_t{
	/* Charge-transfer dwell time             */
	uint8_t chrgtime;
	/* reserved                               */
	uint8_t reserved;
	/* Touch drift compensation period        */
	uint8_t tchdrift;
	/* Drift suspend time                     */
	uint8_t driftst;
	/* Touch automatic calibration delay in units of 0.2s*/
	uint8_t tchautocal;
	/* Measurement synchronisation control    */
	uint8_t sync;
	 /* recalibration suspend time after last detection */
	uint8_t atchcalst;
	/* Anti-touch calibration suspend threshold */
	uint8_t atchcalsthr;
	uint8_t atchcalfrcthr;
	uint8_t atchcalfrcratio;
};

__packed struct touch_multitouchscreen_t9_config_t{
	/* Screen Configuration */
	/* ACENABLE LCENABLE Main configuration field  */
	uint8_t ctrl;

	/* Physical Configuration */
	 /* LCMASK ACMASK Object x start position on matrix  */
	uint8_t xorigin;
	/* LCMASK ACMASK Object y start position on matrix  */
	uint8_t yorigin;
	/* LCMASK ACMASK Object x size (i.e. width)         */
	uint8_t xsize;
	/* LCMASK ACMASK Object y size (i.e. height)        */
	uint8_t ysize;

	/* Detection Configuration */
	/* Adjacent key suppression config     */
	uint8_t akscfg;
	/* Sets the gain of the analog circuits in front
	* of the ADC. The gain should be set in
	* conjunction with the burst length to optimize
	* the signal acquisition. Maximum gain values for
	* a given object/burst length can be obtained following
	* a full calibration of the system. GAIN
	* has a maximum setting of 4; settings above 4 are capped at 4.*/
	uint8_t blen;
	/* ACMASK Threshold for all object channels   */
	uint8_t tchthr;
	/* Detect integration config           */
	uint8_t tchdi;
	/* LCMASK Controls flipping and rotating of touchscreen object */
	uint8_t orient;
	/* Timeout on how long a touch might ever stay
	*   merged - units of 0.2s, used to tradeoff power
	*   consumption against being able to detect a touch
	*   de-merging early */
	uint8_t mrgtimeout;

	/* Position Filter Configuration */
	/* Movement hysteresis setting used after touchdown */
	uint8_t movhysti;
	/* Movement hysteresis setting used once dragging   */
	uint8_t movhystn;
	/* Position filter setting controlling the rate of  */
	uint8_t movfilter;

	/* Multitouch Configuration */
	/* The number of touches that the screen will attempt
	*   to track */
	uint8_t numtouch;
	/* The hysteresis applied on top of the merge threshold
	*   to stop oscillation */
	uint8_t mrghyst;
	/* The threshold for the point when two peaks are
	*   considered one touch */
	uint8_t mrgthr;
	uint8_t amphyst;          /* TBD */

	/* Resolution Controls */
	uint16_t xrange;       /* LCMASK */
	uint16_t yrange;       /* LCMASK */
	uint8_t xloclip;       /* LCMASK */
	uint8_t xhiclip;       /* LCMASK */
	uint8_t yloclip;       /* LCMASK */
	uint8_t yhiclip;       /* LCMASK */
	/* edge correction controls */
	uint8_t xedgectrl;     /* LCMASK */
	uint8_t xedgedist;     /* LCMASK */
	uint8_t yedgectrl;     /* LCMASK */
	uint8_t yedgedist;     /* LCMASK */
	uint8_t jumplimit;
	uint8_t tchhyst;
	uint8_t xpitch;
	uint8_t ypitch;
};

__packed struct procg_noisesuppression_t22_config_t{
	uint8_t ctrl;
	uint8_t reserved;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t reserved3;
	uint8_t reserved4;
	uint8_t reserved5;
	uint8_t reserved6;
	uint8_t noisethr;
	uint8_t reserved7;
	uint8_t freqhopscale;
	uint8_t freq[5u];             /* LCMASK ACMASK */
	uint8_t reserved8;        /* LCMASK */
};

__packed struct spt_cteconfig_t28_config_t{
	/* Ctrl field reserved for future expansion */
	uint8_t ctrl;
	/* Cmd field for sending CTE commands */
	uint8_t cmd;
	/* LCMASK CTE mode configuration field */
	uint8_t mode;
	/* LCMASK The global gcaf number of averages when idle */
	uint8_t idlegcafdepth;
	/* LCMASK The global gcaf number of averages when active */
	uint8_t actvgcafdepth;
	uint8_t voltage;
};

__packed struct proci_gripsuppression_t40_config_t{
	uint8_t ctrl;
	uint8_t xlogrip;
	uint8_t xhigrip;
	uint8_t ylogrip;
	uint8_t yhigrip;
};

__packed struct proci_palmsuppression_t41_config_t{
	uint8_t ctrl;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t largeobjthr;
	uint8_t distancethr;
	uint8_t supextto;
};

struct multi_touch_info {
	uint16_t size;
	int16_t pressure;
	int16_t x;
	int16_t y;
	int status;
};

struct mxt_callbacks {
	void (*inform_charger)(struct mxt_callbacks *, int mode);
};

struct mxt_platform_data {
	u8    numtouch;	/* Number of touches to report	*/
	u8    (*valid_interrupt) (void);
	void  (*init_platform_hw)(void);
	void  (*exit_platform_hw)(void);
	void  (*suspend_platform_hw)(void);
	void  (*resume_platform_hw)(void);
	void	(*register_cb)(struct mxt_callbacks *);
	int   max_x;    /* The default reported X range   */
	int   max_y;    /* The default reported Y range   */
	struct gen_powerconfig_t7_config_t power_config;
	struct gen_acquisitionconfig_t8_config_t acquisition_config;
	struct touch_multitouchscreen_t9_config_t touchscreen_config;
	struct procg_noisesuppression_t22_config_t noise_suppression_config;
	struct spt_cteconfig_t28_config_t cte_config;
	struct proci_gripsuppression_t40_config_t gripsupression_config;
	struct proci_palmsuppression_t41_config_t palmsupression_config;
	uint8_t idleacqint_for_ta_connect;
	uint8_t tchthr_for_ta_connect;
	uint8_t tchdi_for_ta_connect;
	uint8_t noisethr_for_ta_connect;
	uint8_t idlegcafdepth_ta_connect;
	u16 fherr_cnt;
	u16 fherr_chg_cnt;
	uint8_t tch_blen_for_fherr;
	uint8_t tchthr_for_fherr;
	uint8_t noisethr_for_fherr;
	uint8_t movefilter_for_fherr;
	uint8_t jumplimit_for_fherr;
	uint8_t freqhopscale_for_fherr;
	uint8_t freq_for_fherr1[5];
	uint8_t freq_for_fherr2[5];
	uint8_t freq_for_fherr3[5];
	u16 fherr_cnt_no_ta;
	u16 fherr_chg_cnt_no_ta;
	uint8_t tch_blen_for_fherr_no_ta;
	uint8_t tchthr_for_fherr_no_ta;
	uint8_t movfilter_fherr_no_ta;
	uint8_t noisethr_for_fherr_no_ta;
#ifdef MXT_CALIBRATE_WORKAROUND
	/* recalibration suspend time after last detection */
	uint8_t atchcalst_idle;
	/* Anti-touch calibration suspend threshold */
	uint8_t atchcalsthr_idle;
	uint8_t atchcalfrcthr_idle;
	uint8_t atchcalfrcratio_idle;
#endif
};

/* Driver datastructure */
struct mxt_data {
	struct mxt_device_info	device_info;
	struct i2c_client *client;
	struct input_dev *input;
	struct mxt_platform_data *pdata;
	struct delayed_work firmup_dwork;
	struct delayed_work dwork;
	struct work_struct ta_work;
	struct work_struct fhe_work;
	struct report_id_map *rid_map;
	struct mxt_object *object_table;
	struct wake_lock wakelock;
	struct mxt_callbacks callbacks;
	struct mutex mutex;
#ifdef MXT_CALIBRATE_WORKAROUND
	struct delayed_work calibrate_dwork;
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	struct multi_touch_info mtouch_info[MXT_MAX_NUM_TOUCHES];
#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
	struct delayed_work dvfs_dwork;
	u32 cpufreq_level;
	bool dvfs_lock_status;
#endif
	bool new_msgs;
	bool fherr_cnt_no_ta_calready;
	char phys_name[32];
	int irq;
	int valid_irq_counter;
	int invalid_irq_counter;
	int irq_counter;
	int message_counter;
	int read_fail_counter;
	int bytes_to_read;
	s16 *delta;
	u8 *last_message;
	u8 xpos_format;
	u8 ypos_format;
	u8 message_size;
	u8 firm_status_data;
	u8 firm_normal_status_ack;
	u16 last_read_addr;
	u16 report_id_count;
	u16 msg_proc_addr;
	u16 *reference;
	u16 *cte;
	u16	set_mode_for_ta;
	u16	enabled;
	u32	info_block_crc;
	u32  configuration_crc;
	spinlock_t lock;
	wait_queue_head_t msg_queue;
	/* for the factory test */
	u32 index;
	s16 delta_data[MXT1386_MAX_CHANNEL];
	u16 ref_data[MXT1386_MAX_CHANNEL];
};

enum tsp_ta_settings {
	TSP_SETTING_IDLEACQINT = 0,
	TSP_SETTING_BLEN,
	TSP_SETTING_TCHTHR,
	TSP_SETTING_NOISETHR,
	TSP_SETTING_IDLEDEPTH,
	TSP_SETTING_MOVEFILTER,
	TSP_SETTING_FREQUENCY,
	TSP_SETTING_FREQ_SCALE,
	TSP_SETTING_JUMPLIMIT,
	TSP_SETTING_MAX,
};

#define SET_BIT(nr, val) (nr |= (0x1 << val))

/* Returns the start address of object in mXT memory. */
#define	MXT_BASE_ADDR(object_type) \
get_object_address(object_type, 0, mxt->object_table, mxt->device_info.num_objs)

/* Returns the size of object in mXT memory. */
#define	MXT_GET_SIZE(object_type) \
get_object_size(object_type, mxt->object_table, mxt->device_info.num_objs)

/* Routines for memory access within a 16 bit address space */
int mxt_read_byte(
				struct i2c_client *client,
				__u16 addr,
				__u8 *value
				);
int mxt_write_byte(
				struct i2c_client *client,
				__u16 addr,
				__u8 value
				);
int mxt_write_block(
				struct i2c_client *client,
				__u16 addr,
				__u16 length,
				__u8 *value
				);

#if	1
/* Should be implemented in board support */
u8 mxt_valid_interrupt(void);
#else
#define	mxt_valid_interrupt()    1
#endif

void	mxt_hw_reset(void);

#endif	/* _LINUX_MXT1386_H */
