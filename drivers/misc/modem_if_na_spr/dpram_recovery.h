#ifndef __DPRAM_RECOVERY_H__
#define __DPRAM_RECOVERY_H__



/* interupt masks */
#define MASK_CMD_FOTA_IMG_RECEIVE_READY_RESP		0x0100
#define MASK_CMD_FOTA_IMG_SEND_RESP		        0x0200
#define MASK_CMD_FOTA_SEND_DONE_RESP			0x0300
#define MASK_CMD_FOTA_UPDATE_START_RESP			0x0400
#define MASK_CMD_FOTA_UPDATE_STATUS_IND			0x0500
#define MASK_CMD_FOTA_UPDATE_END_IND                    0x0C00

/* FORMAT */
#define CMD_FOTA_IMG_RECEIVE_READY_REQ			0x9100
#define CMD_FOTA_IMG_SEND_REQ				0x9200
#define CMD_FOTA_SEND_DONE_REQ				0x9300
#define CMD_FOTA_UPDATE_START_REQ			0x9400
#define CMD_FOTA_UPDATE_STATUS_IND			0x9500
#define CMD_FOTA_INIT_START_REQ				0x9A00
#define CMD_FOTA_INIT_START_RES				0x9B00
#define CMD_FOTA_UPDATE_END_IND				0x9C00

#define CMD_RETRY	                                0
#define CMD_TRUE	                                1
#define CMD_FALSE	                                -1

#define CMD_DL_SEND_DONE_REQ                            0x9600
#define CMD_PHONE_BOOT_UPDATE				0x0001
#define MASK_CMD_SEND_DONE_RESPONSE                     0x0700
#define MASK_CMD_STATUS_UPDATE_NOTIFICATION             0x0800
#define MASK_CMD_UPDATE_DONE_NOTIFICATION               0x0900
#define MASK_CMD_IMAGE_SEND_RESPONSE                    0x0500

/* Result mask */
#define MASK_CMD_RESULT_FAIL				0x0002
#define MASK_CMD_RESULT_SUCCESS				0x0001

#define MASK_CMD_VALID					0x8000
#define MASK_PDA_CMD					0x1000
#define MASK_PHONE_CMD					0x2000

#define MAGIC_FODN	0x4E444F46  /* PDA initiate phone code */
#define MAGIC_FOTA	0x41544C44  /* PDA initiate phone code */
#define MAGIC_DMDL	0x444D444C
#define MAGIC_ALARMBOOT	0x00410042

/* DPRAM */
#define DPRAM_BASE_ADDR					S5P_PA_MODEMIF
#define DPRAM_MAGIC_CODE_SIZE				0x4
#define DPRAM_PDA2PHONE_FORMATTED_START_ADDRESS		(DPRAM_MAGIC_CODE_SIZE)
#define DPRAM_PHONE2PDA_INTERRUPT_ADDRESS               (0x3FFE)
#define DPRAM_PDA2PHONE_INTERRUPT_ADDRESS               (0x3FFC)
#define DPRAM_BUFFER_SIZE (DPRAM_PDA2PHONE_INTERRUPT_ADDRESS - \
		DPRAM_PDA2PHONE_FORMATTED_START_ADDRESS)
#define BSP_DPRAM_BASE_SIZE	0x4000	/* 16KB DPRAM in Mirage */
#define DPRAM_END_OF_ADDRESS	(BSP_DPRAM_BASE_SIZE - 1)
#define DPRAM_INTERRUPT_SIZE	0x2
#define DPRAM_INDEX_SIZE        0x2
#define DELTA_PACKET_SIZE       (0x4000 - 0x4 - 0x4)
#define WRITEIMG_HEADER_SIZE	8
#define WRITEIMG_TAIL_SIZE	4
#define WRITEIMG_BODY_SIZE	(DPRAM_BUFFER_SIZE - \
		WRITEIMG_HEADER_SIZE - WRITEIMG_TAIL_SIZE)
#define FODN_DEFAULT_WRITE_LEN	WRITEIMG_BODY_SIZE
#define DPRAM_START_ADDRESS	0
#define DPRAM_END_OF_ADDRESS	(BSP_DPRAM_BASE_SIZE - 1)
#define DPRAM_SIZE              BSP_DPRAM_BASE_SIZE

/* ioctl commands */
#define IOCTL_ST_FW_UPDATE                              _IO('D', 0x1)
#define IOCTL_CHK_STAT                                  _IO('D', 0x2)
#define IOCTL_MOD_PWROFF                                _IO('D', 0x3)
#define IOCTL_WRITE_MAGIC_CODE                          _IO('D', 0x4)
#define IOCTL_ST_FW_DOWNLOAD                            _IO('D', 0x5)

/* JB porting */
#define IOCTL_MODEM_BOOT_ON     _IO('o', 0x22)
#define IOCTL_MODEM_BOOT_OFF    _IO('o', 0x23)
#define IOCTL_MODEM_GOTA_START  _IO('o', 0x28)
#define IOCTL_MODEM_FW_UPDATE   _IO('o', 0x29)

#define GPIO_QSC_INT                                    GPIO_C210_DPRAM_INT_N
#define IRQ_QSC_INT                                     GPIO_QSC_INT

/* Common */
#define TRUE                                            1
#define FALSE                                           0

#define GPIO_IN			                        0
#define GPIO_OUT		                        1
#define GPIO_LOW		                        0
#define GPIO_HIGH		                        1

#define START_INDEX					0x007F
#define END_INDEX					0x007E

#define DPRAM_MODEM_MSG_SIZE                            0x100

struct IDPRAM_SFR {
	unsigned int int2ap;
	unsigned int int2msm;
	unsigned int mifcon;
	unsigned int mifpcon;
	unsigned int msmintclr;
	unsigned int dma_tx_adr;
	unsigned int dma_rx_adr;
};

/* It is recommended that S5PC110 write data with
half-word access on the interrupt port because
S5PC100 overwrites tha data in INT2AP if there are
INT2AP and INT2MSM sharing the same word */
#define IDPRAM_INT2MSM_MASK                             0xFF

#define IDPRAM_MIFCON_INT2APEN                          (1<<2)
#define IDPRAM_MIFCON_INT2MSMEN                         (1<<3)
#define IDPRAM_MIFCON_DMATXREQEN_0                      (1<<16)
#define IDPRAM_MIFCON_DMATXREQEN_1                      (1<<17)
#define IDPRAM_MIFCON_DMARXREQEN_0                      (1<<18)
#define IDPRAM_MIFCON_DMARXREQEN_1                      (1<<19)
#define IDPRAM_MIFCON_FIXBIT                            (1<<20)

#define IDPRAM_MIFPCON_ADM_MODE	(1<<6) /* mux / demux mode */

/* end */

struct dpram_firmware {
	char *firmware;
	int size;
	int image_type;
};

struct stat_info {
	int pct;
	char msg[DPRAM_MODEM_MSG_SIZE];
};

#define DPRAM_MODEM_DELTA_IMAGE				0
#define DPRAM_MODEM_FULL_IMAGE				1

#endif	/* __DPRAM_RECOVERY_H__ */
