/* This file contains the command definitions for the Si47xx Parts. */
#ifndef _COMMAND_DEFS_H_
#define _COMMAND_DEFS_H_

/*==================================================================
General Commands
==================================================================*/

/* STATUS bits - Used by all methods */
#define STCINT  0x01
#define ASQINT  0x02
#define RDSINT  0x04
#define RSQINT  0x08
#define ERR     0x40
#define CTS     0x80

/* POWER_UP */
#define POWER_UP                      0x01
#define POWER_UP_IN_FUNC_FMRX         0x00
#define POWER_UP_IN_FUNC_AMRX         0x01
#define POWER_UP_IN_FUNC_FMTX         0x02
#define POWER_UP_IN_FUNC_WBRX         0x03
#define POWER_UP_IN_FUNC_QUERY        0x0F
#define POWER_UP_IN_PATCH             0x20
#define POWER_UP_IN_GPO2OEN           0x40
#define POWER_UP_IN_CTSIEN            0x80
#define POWER_UP_IN_OPMODE_RX_ANALOG  0x05
#define POWER_UP_IN_OPMODE_TX_ANALOG  0x50

/* GET_REV */
#define GET_REV 0x10

/* POWER_DOWN */
#define POWER_DOWN 0x11

/* SET_PROPERTY */
#define SET_PROPERTY 0x12

/* GET_PROPERTY */
#define GET_PROPERTY 0x13

/* GET_INT_STATUS */
#define GET_INT_STATUS 0x14

/*==================================================================
 FM Receive Commands
==================================================================*/

/* FM_TUNE_FREQ */
#define FM_TUNE_FREQ 0x20

/* FM_SEEK_START */
#define FM_SEEK_START           0x21
#define FM_SEEK_START_IN_WRAP   0x04
#define FM_SEEK_START_IN_SEEKUP 0x08

/* FM_TUNE_STATUS */
#define FM_TUNE_STATUS           0x22
#define FM_TUNE_STATUS_IN_INTACK 0x01
#define FM_TUNE_STATUS_IN_CANCEL 0x02
#define FM_TUNE_STATUS_OUT_VALID 0x01
#define FM_TUNE_STATUS_OUT_AFCRL 0x02
#define FM_TUNE_STATUS_OUT_BTLF  0x80

/* FM_RSQ_STATUS */
#define FM_RSQ_STATUS              0x23
#define FM_RSQ_STATUS_IN_INTACK    0x01
#define FM_RSQ_STATUS_OUT_RSSILINT 0x01
#define FM_RSQ_STATUS_OUT_RSSIHINT 0x02
#define FM_RSQ_STATUS_OUT_ASNRLINT 0x04
#define FM_RSQ_STATUS_OUT_ASNRHINT 0x08
#define FM_RSQ_STATUS_OUT_BLENDINT 0x80
#define FM_RSQ_STATUS_OUT_VALID    0x01
#define FM_RSQ_STATUS_OUT_AFCRL    0x02
#define FM_RSQ_STATUS_OUT_SMUTE    0x08
#define FM_RSQ_STATUS_OUT_PILOT    0x80
#define FM_RSQ_STATUS_OUT_STBLEND  0x7F

/* FM_RDS_STATUS */
#define FM_RDS_STATUS               0x24
#define FM_RDS_STATUS_IN_INTACK     0x01
#define FM_RDS_STATUS_IN_MTFIFO     0x02
#define FM_RDS_STATUS_OUT_RECV      0x01
#define FM_RDS_STATUS_OUT_SYNCLOST  0x02
#define FM_RDS_STATUS_OUT_SYNCFOUND 0x04
#define FM_RDS_STATUS_OUT_SYNC      0x01
#define FM_RDS_STATUS_OUT_GRPLOST   0x04
#define FM_RDS_STATUS_OUT_BLED      0x03
#define FM_RDS_STATUS_OUT_BLEC      0x0C
#define FM_RDS_STATUS_OUT_BLEB      0x30
#define FM_RDS_STATUS_OUT_BLEA      0xC0
#define FM_RDS_STATUS_OUT_BLED_SHFT 0
#define FM_RDS_STATUS_OUT_BLEC_SHFT 2
#define FM_RDS_STATUS_OUT_BLEB_SHFT 4
#define FM_RDS_STATUS_OUT_BLEA_SHFT 6

/*==================================================================
 AM Receive Commands
==================================================================*/

/* AM_TUNE_FREQ */
#define AM_TUNE_FREQ 0x40

/* AM_SEEK_START */
#define AM_SEEK_START           0x41
#define AM_SEEK_START_IN_WRAP   0x04
#define AM_SEEK_START_IN_SEEKUP 0x08

/* AM_TUNE_STATUS */
#define AM_TUNE_STATUS           0x42
#define AM_TUNE_STATUS_IN_INTACK 0x01
#define AM_TUNE_STATUS_IN_CANCEL 0x02
#define AM_TUNE_STATUS_OUT_VALID 0x01
#define AM_TUNE_STATUS_OUT_AFCRL 0x02
#define AM_TUNE_STATUS_OUT_BTLF  0x80

/* AM_RSQ_STATUS */
#define AM_RSQ_STATUS              0x23
#define AM_RSQ_STATUS_IN_INTACK    0x01
#define AM_RSQ_STATUS_OUT_RSSILINT 0x01
#define AM_RSQ_STATUS_OUT_RSSIHINT 0x02
#define AM_RSQ_STATUS_OUT_ASNRLINT 0x04
#define AM_RSQ_STATUS_OUT_ASNRHINT 0x08
#define AM_RSQ_STATUS_OUT_VALID    0x01
#define AM_RSQ_STATUS_OUT_AFCRL    0x02
#define AM_RSQ_STATUS_OUT_SMUTE    0x08

/*==================================================================
 FM Transmit Commands
==================================================================*/

/* TX_TUNE_FREQ */
#define TX_TUNE_FREQ 0x30

/* TX_TUNE_POWER */
#define TX_TUNE_POWER 0x31

/* TX_TUNE_MEASURE */
#define TX_TUNE_MEASURE 0x32

/* TX_TUNE_STATUS */
#define TX_TUNE_STATUS           0x33
#define TX_TUNE_STATUS_IN_INTACK 0x01

/* TX_ASQ_STATUS */
#define TX_ASQ_STATUS             0x34
#define TX_ASQ_STATUS_IN_INTACK   0x01
#define TX_ASQ_STATUS_OUT_IALL    0x01
#define TX_ASQ_STATUS_OUT_IALH    0x02
#define TX_ASQ_STATUS_OUT_OVERMOD 0x04

/* TX_RDS_BUFF */
#define TX_RDS_BUFF           0x35
#define TX_RDS_BUFF_IN_INTACK 0x01
#define TX_RDS_BUFF_IN_MTBUFF 0x02
#define TX_RDS_BUFF_IN_LDBUFF 0x04
#define TX_RDS_BUFF_IN_FIFO   0x80

/* TX_RDS_PS */
#define TX_RDS_PS 0x36

/*==================================================================
 WB Receive Commands
==================================================================*/

/* WB_TUNE_FREQ */
#define WB_TUNE_FREQ 0x50

/* WB_TUNE_STATUS */
#define WB_TUNE_STATUS           0x52
#define WB_TUNE_STATUS_IN_INTACK 0x01
#define WB_TUNE_STATUS_OUT_VALID 0x01
#define WB_TUNE_STATUS_OUT_AFCRL 0x02

/* WB_RSQ_STATUS */
#define WB_RSQ_STATUS              0x53
#define WB_RSQ_STATUS_IN_INTACK    0x01
#define WB_RSQ_STATUS_OUT_RSSILINT 0x01
#define WB_RSQ_STATUS_OUT_RSSIHINT 0x02
#define WB_RSQ_STATUS_OUT_ASNRLINT 0x04
#define WB_RSQ_STATUS_OUT_ASNRHINT 0x08
#define WB_RSQ_STATUS_OUT_VALID    0x01
#define WB_RSQ_STATUS_OUT_AFCRL    0x02

/* WB_ASQ_STATUS */
#define WB_ASQ_STATUS			0x55
#define WB_ASQ_STATUS_IN_INTACK		0x01
#define WB_ASQ_STATUS_OUT_ALERTONINT	0x01
#define WB_ASQ_STATUS_OUT_ALERTOFFINT	0x02
#define WB_ASQ_STATUS_OUT_ALERT		0x01

#endif
