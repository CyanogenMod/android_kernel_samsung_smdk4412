

#include <linux/wacom_i2c.h>
#include "wacom_i2c_firm.h"
#include "wacom_i2c_func.h"

#define FLASH_START0 	'f'
#define FLASH_START1 	'l'
#define FLASH_START2 	'a'
#define FLASH_START3 	's'
#define FLASH_START4 	'h'
#define FLASH_START5 	'\r'
#define FLASH_ACK	 	0x06

#define pana_QUERY   	0x11

#define flash_END	 	0x80
#define flash_VERIFY	0x81
#define flash_WRITE		0x82
#define flash_READ		0x83
#define flash_ERASE		0x84
#define flash_SET_INFO	0x85
#define flash_END_TO_BOOT 0x87
#define flash_BAUDRATE	0x88

#define flash_QUERY  	0xE0
#define flash_BLVER		0xE1
#define flash_UNITID	0xE2
#define flash_GET_INFO	0xE3
#define flash_FWVER		0xE4
#define flash_MPU		0xE8

#define pen_QUERY		'*'

#define V09			0
#define V095		1

#define HIDIIC_VERSION V095

/*------------------All below are copied from COMMON.H------------------*/

#define FLASH_BLOCK_SIZE		64

#define ASCINT_ON		0x0
#define ASCINT_OFF		0x1
#define ASCINT_ERROR	0xFF

/*
#define WRITE		0
*/
#define VERIFY		1
#define WRITEVERIFY	2
#define ERASE		3
#define GETVERSION	4

#define USER_ADDRESS	0x56
#define BOOT_ADDRESS	0x57

#define CMD_GET_FEATURE	2
#define CMD_SET_FEATURE	3

#define ACK				0

#define BOOT_CMD_SIZE	78
#define BOOT_RSP_SIZE	6

#define BOOT_CMD_REPORT_ID	7

#define BOOT_ERASE_FLASH		0
#define BOOT_WRITE_FLASH		1
#define BOOT_VERIFY_FLASH		2
#define BOOT_EXIT				3
#define BOOT_BLVER				4
#define BOOT_MPU				5
#define BOOT_SECURITY_UNLOCK	6
#define BOOT_QUERY				7

#define QUERY_CMD 0x07
#define QUERY_ECH 'D'
#define QUERY_RSP 0x06

#define BOOT_CMD 0x04
#define BOOT_ECH 'D'

#define MPU_CMD 0x05
#define MPU_ECH 'D'

#define SEC_CMD 0x06
#define SEC_ECH 'D'
#define SEC_RSP 0x00

#define ERS_CMD 0x00
#define ERS_ECH 'D'
#define ERS_RSP 0x00

#define MARK_CMD 0x02
#define MARK_ECH 'D'
#define MARK_RSP 0x00

#define WRITE_CMD 0x01
#define WRITE_ECH 'D'
#define WRITE_RSP 0x00

#define VERIFY_CMD 0x02
#define VERIFY_ECH 'D'
#define VERIFY_RSP 0x00

#define CMD_SIZE (72+6)
#define RSP_SIZE 6

#define DATA_SIZE (65536 * 4)

//
// exit codes
//
#define EXIT_OK							(0)
#define EXIT_REBOOT						(1)
#define EXIT_FAIL						(2)
#define EXIT_USAGE						(3)
#define EXIT_NO_SUCH_FILE				(4)
#define EXIT_NO_INTEL_HEX				(5)
#define EXIT_FAIL_OPEN_COM_PORT			(6)
#define EXIT_FAIL_ENTER_FLASH_MODE		(7)
#define EXIT_FAIL_FLASH_QUERY			(8)
#define EXIT_FAIL_BAUDRATE_CHANGE		(9)
#define EXIT_FAIL_WRITE_FIRMWARE		(10)
#define EXIT_FAIL_EXIT_FLASH_MODE		(11)
#define EXIT_CANCEL_UPDATE				(12)
#define EXIT_SUCCESS_UPDATE				(13)
#define EXIT_FAIL_HID2SERIAL			(14)
#define EXIT_FAIL_VERIFY_FIRMWARE		(15)
#define EXIT_FAIL_MAKE_WRITING_MARK		(16)
#define EXIT_FAIL_ERASE_WRITING_MARK	(17)
#define EXIT_FAIL_READ_WRITING_MARK		(18)
#define EXIT_EXIST_MARKING				(19)
#define EXIT_FAIL_MISMATCHING			(20)
#define EXIT_FAIL_ERASE					(21)
#define EXIT_FAIL_GET_BOOT_LOADER_VERSION	(22)
#define EXIT_FAIL_GET_MPU_TYPE			(23)
#define EXIT_MISMATCH_BOOTLOADER		(24)
#define EXIT_MISMATCH_MPUTYPE			(25)
#define EXIT_FAIL_ERASE_BOOT			(26)
#define EXIT_FAIL_WRITE_BOOTLOADER		(27)
#define EXIT_FAIL_SWAP_BOOT				(28)
#define EXIT_FAIL_WRITE_DATA			(29)
#define EXIT_FAIL_GET_FIRMWARE_VERSION	(30)
#define EXIT_FAIL_GET_UNIT_ID			(31)
#define EXIT_FAIL_SEND_STOP_COMMAND		(32)
#define EXIT_FAIL_SEND_QUERY_COMMAND	(33)
#define EXIT_NOT_FILE_FOR_535			(34)
#define EXIT_NOT_FILE_FOR_514			(35)
#define EXIT_NOT_FILE_FOR_503			(36)
#define EXIT_MISMATCH_MPU_TYPE			(37)
#define EXIT_NOT_FILE_FOR_515			(38)
#define EXIT_NOT_FILE_FOR_1024			(39)
#define EXIT_FAIL_VERIFY_WRITING_MARK	(40)
#define EXIT_DEVICE_NOT_FOUND			(41)
#define EXIT_FAIL_WRITING_MARK_NOT_SET	(42)

///////////////////////////   Structure   //////////////////////////////
typedef struct _FW_VERSION
{
	u8 UpVer;
	u8 LoVer;
} FW_VERSION, *PFW_VERSION;

typedef struct _FLASHINF
{
	unsigned int  mode;
	bool  bExit;
	bool bPowerSupply;
	u8  DeviceAddr;
} FLASHINF, *PFLASHINF;

struct wInt{
	u16	lI;
	u16	hI;
};

struct dwbyte{
	unsigned char	ll;
	unsigned char	lh;
	unsigned char	hl;
	unsigned char	hh;
};

union uAddress{
	unsigned long	Lng;
	struct	wInt	Int;
	struct	dwbyte	Byt;
} ;

typedef struct
{
	unsigned char data[12];
	unsigned char flash_data[64];
	unsigned char chksum;
	unsigned char chksumData;

} boot_flash_verify;

typedef struct
{
	unsigned char RegNoL;
	unsigned char RegNoH;
	unsigned char lenL;
	unsigned char lenH;
	unsigned char reportId;
	unsigned char cmd;		/* command code, see BOOT_xxx constants */
	unsigned char echo;		/* echo is used to link between command and response */
	unsigned char cksum;	/* check sum */
} boot_cmd_header;

/*
 * VERIFY_FLASH - verify flash memory
 */
typedef struct
{
	unsigned char RegNoL;
	unsigned char RegNoH;
	unsigned char lenL;
	unsigned char lenH;
	unsigned char reportId;
	unsigned char cmd;		/* command code, see BOOT_xxx constants */
	unsigned char echo;		/* echo is used to link between command and response */
	union uAddress addr;		/* address must be divisible by 2 */
	unsigned char size8;		/* size must be divisible by 8 */
	unsigned char data[64];
	unsigned char cksum;	/* check sum */
	unsigned char cksumData;	/* check sum */
} boot_cmd_verify_flash;

/*
* WRITE_FLASH - write flash memory
*/
typedef struct
{
	unsigned char RegNoL;
	unsigned char RegNoH;
	unsigned char lenL;
	unsigned char lenH;
	unsigned char reportId;
	unsigned char cmd;		/* command code, see BOOT_xxx constants */
	unsigned char echo;		/* echo is used to link between command and response */
	union uAddress addr;		/* address must be divisible by 2 */
	unsigned char size8;		/* size must be divisible by 8*/
	unsigned char data[64];
	unsigned char cksum;	/* check sum */
	unsigned char cksumData;	/* check sum */
} boot_cmd_write_flash;

/*
 * ERASE_FLASH - erase flash memory
 */
typedef struct
{
	unsigned char RegNoL;
	unsigned char RegNoH;
	unsigned char lenL;
	unsigned char lenH;
	unsigned char reportId;
	unsigned char cmd;		/* command code, see BOOT_xxx constants */
	unsigned char echo;		/* echo is used to link between command and response */
	unsigned char blkNo;		/* block No. */
	unsigned char cksum;		/* check sum */
} boot_cmd_erase_flash;

/*
* RESET - reset microcontroller
*/
typedef struct
{
	unsigned char RegNoL;
	unsigned char RegNoH;
	unsigned char lenL;
	unsigned char lenH;
	unsigned char reportId;
	unsigned char cmd;			/* command code, see BOOT_xxx constants */
	unsigned char echo;			/* echo is used to link between command and response */
	unsigned char cksum;		/* check sum */
} boot_cmd_reset;

/*
* BLVER - get bootloader version
*/
typedef struct
{
	unsigned char RegNoL;
	unsigned char RegNoH;
	unsigned char lenL;
	unsigned char lenH;
	unsigned char reportId;
	unsigned char cmd;			/* command code, see BOOT_xxx constants */
	unsigned char echo;			/* echo is used to link between command and response */
	unsigned char cksum;		/* check sum */
} boot_cmd_blver;

/*
* MPUTYPE - get mpu type
*/
typedef struct
{
	unsigned char RegNoL;
	unsigned char RegNoH;
	unsigned char lenL;
	unsigned char lenH;
	unsigned char reportId;
	unsigned char cmd;			/* command code, see BOOT_xxx constants */
	unsigned char echo;			/* echo is used to link between command and response */
	unsigned char cksum;		/* check sum */
} boot_cmd_mputype;

/*
* QUERY - confirm boot mode
*/
typedef struct
{
	unsigned char RegNoL;
	unsigned char RegNoH;
	unsigned char lenL;
	unsigned char lenH;
	unsigned char reportId;
	unsigned char cmd;			/* command code, see BOOT_xxx constants */
	unsigned char echo;			/* echo is used to link between command and response */
	unsigned char cksum;		/* check sum */
} boot_cmd_query;

typedef union
{
/*
 * data field is used to make all commands the same length
 */
	unsigned char data[72+6];
	boot_cmd_header header;
	boot_cmd_verify_flash verify_flash;
	boot_cmd_write_flash write_flash;
	boot_cmd_erase_flash erase_flash;
	boot_cmd_reset reset;
	boot_cmd_blver blver;
	boot_cmd_mputype mputype;
	boot_cmd_query query;
} boot_cmd;

/*
 * common for all responses fields
 */
typedef struct
{
	unsigned char lenL;
	unsigned char lenH;
	unsigned char reportId;
	unsigned char cmd;		/* command code, see BOOT_xxx constants */
	unsigned char echo;		/* echo is used to link between command and response */
	unsigned char resp;
} boot_rsp_header;

/*
* WRITE_FLASH - write flash memory
*/
typedef struct
{
	unsigned char lenL;
	unsigned char lenH;
	unsigned char reportId;
	unsigned char cmd;		/* command code, see BOOT_xxx constants */
	unsigned char echo;		/* echo is used to link between command and response */
	unsigned char resp;
} boot_rsp_write_flash;

/*
* VERIFY_FLASH - verify flash memory
*/
typedef struct
{
	unsigned char lenL;
	unsigned char lenH;
	unsigned char reportId;
	unsigned char cmd;		/* command code, see BOOT_xxx constants */
	unsigned char echo;		/* echo is used to link between command and response */
	unsigned char resp;
} boot_rsp_verify_flash;

/*
* ERASE_FLASH - erase flash memory
*/
typedef struct
{
	unsigned char lenL;
	unsigned char lenH;
	unsigned char reportId;
	unsigned char cmd;		/* command code, see BOOT_xxx constants */
	unsigned char echo;		/* echo is used to link between command and response */
	unsigned char resp;
} boot_rsp_erase_flash;

/*
* BLVER - boot loader version
*/
typedef struct
{
	unsigned char lenl;
	unsigned char lenh;
	unsigned char reportId;
	unsigned char cmd;		/* command code, see BOOT_xxx constants */
	unsigned char echo;		/* echo is used to link between command and response */
	unsigned char resp;
} boot_rsp_blver;

/*
* MPUTYPE - mpu type
*/
typedef struct
{
	unsigned char lenl;
	unsigned char lenh;
	unsigned char reportId;
	unsigned char cmd;		/* command code, see BOOT_xxx constants */
	unsigned char echo;		/* echo is used to link between command and response */
	unsigned char resp;
} boot_rsp_mputype;

/*
* QUERY - query
*/
typedef struct
{
	unsigned char lenl;
	unsigned char lenh;
	unsigned char reportId;
	unsigned char cmd;		/* command code, see BOOT_xxx constants */
	unsigned char echo;		/* echo is used to link between command and response */
	unsigned char resp;
} boot_rsp_query;


typedef union
{
/*
 * data field is used to make all responses the same length
 */
	unsigned char data[6];
	boot_rsp_header header;
	boot_rsp_verify_flash verify_flash;
	boot_rsp_write_flash write_flash;
	boot_rsp_erase_flash erase_flash;
	boot_rsp_blver blver;
	boot_rsp_mputype mputype;
	boot_rsp_query query;
} boot_rsp;
/*-----------------------------All above are copied fom COMMON.H--------------------------------------------*/

extern bool flash_query(struct wacom_i2c *wac_i2c);
extern bool flash_blver(struct wacom_i2c *wac_i2c, int *bler);
extern bool flash_mputype(struct wacom_i2c *wac_i2c, int* pMpuType);
extern bool flash_security_unlock(struct wacom_i2c *wac_i2c, int *status);
extern bool flash_end(struct wacom_i2c *wac_i2c);
extern bool flash_devcieType(struct wacom_i2c *wac_i2c);
extern int GetFWVersion(struct wacom_i2c *wac_i2c, PFW_VERSION pVer);
extern int GetBLVersion(struct wacom_i2c *wac_i2c, int* pBLVer);
extern int GetMpuType(struct wacom_i2c *wac_i2c, int* pMpuType);
extern int SetSecurityUnlock(struct wacom_i2c *wac_i2c, int* pStatus);
extern bool flash_erase(struct wacom_i2c *wac_i2c, bool bAllUserArea, int *eraseBlock, int num);
extern bool is_flash_marking(struct wacom_i2c *wac_i2c, size_t data_size, bool* bMarking, int iMpuID);
extern bool flash_write_block(struct wacom_i2c *wac_i2c, char *flash_data, unsigned long ulAddress, u8 *pcommand_id);
extern bool flash_write(struct wacom_i2c *wac_i2c, unsigned char *flash_data, size_t data_size, unsigned long start_address, unsigned long *max_address, int mpuType);
extern bool flash_verify(struct wacom_i2c *wac_i2c, unsigned char *flash_data, size_t data_size, unsigned long start_address, unsigned long *max_address, int mpuType);
extern bool flash_marking(struct wacom_i2c *wac_i2c, size_t data_size, bool bMarking, int iMpuID);
extern int FlashWrite(struct wacom_i2c *wac_i2c, char* filename);
extern int FlashVerify(struct wacom_i2c *wac_i2c, char *filename);
extern int wacom_i2c_flash_g9(struct wacom_i2c *wac_i2c);
extern int wacom_i2c_flash_chksum(struct wacom_i2c *wac_i2c,
			unsigned char *flash_data,
			unsigned long *max_address);

