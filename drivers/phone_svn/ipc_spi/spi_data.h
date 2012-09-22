#ifndef _SPI_DATA_H_
#define _SPI_DATA_H_

#include "spi_dev.h"

#define SPI_TASK_EVENT_COUNT	64
#define SPI_DATA_BOF			0x7F
#define SPI_DATA_EOF			0x7E
#define SPI_DATA_FF_PADDING_HEADER 0xFFFFFFFF

#define SPI_DATA_MUX_NORMAL_MASK	0x0F
#define SPI_DATA_MUX_MORE_H			0x10
#define SPI_DATA_MUX_MORE_M			0x20
#define SPI_DATA_MUX_MORE_T			0x30

#define SPI_DATA_MUX_SIZE		2
#define SPI_DATA_LENGTH_SIZE	4
#define SPI_DATA_BOF_SIZE		1
#define SPI_DATA_INNER_LENGTH_SIZE	4
#define SPI_DATA_IPC_INNER_LENGTH_SIZE	2
#define SPI_DATA_IPC_INNER_CONTROL_SIZE	1
#define SPI_DATA_EOF_SIZE		1
#define SPI_DATA_HEADER_SIZE	(SPI_DATA_MUX_SIZE+ \
		SPI_DATA_LENGTH_SIZE+SPI_DATA_BOF_SIZE+SPI_DATA_EOF_SIZE)
#define SPI_DATA_HEADER_SIZE_FRONT	(SPI_DATA_MUX_SIZE+ \
		SPI_DATA_LENGTH_SIZE+SPI_DATA_BOF_SIZE)
#define SPI_DATA_IPC_INNER_HEADER_SIZE	\
		(SPI_DATA_IPC_INNER_LENGTH_SIZE+ \
		SPI_DATA_IPC_INNER_CONTROL_SIZE)
#define SPI_DATA_SIZE(X)				(SPI_DATA_HEADER_SIZE+X)

#define SPI_DATA_LENGTH_OFFSET	SPI_DATA_MUX_SIZE
#define SPI_DATA_BOF_OFFSET		(SPI_DATA_LENGTH_OFFSET+ \
		SPI_DATA_LENGTH_SIZE)
#define SPI_DATA_DATA_OFFSET		(SPI_DATA_BOF_OFFSET+ \
		SPI_DATA_BOF_SIZE)
#define SPI_DATA_EOF_OFFSET(X)		(SPI_DATA_DATA_OFFSET+X)

#define SPI_DATA_PACKET_HEADER_SIZE	                 4
#define SPI_DATA_PACKET_MUX_ERROR_SPARE_SIZE		 4
#define SPI_DATA_PACKET_MAX_PACKET_BODY_SIZE \
		(SPI_DEV_MAX_PACKET_SIZE-SPI_DATA_PACKET_HEADER_SIZE- \
		SPI_DATA_PACKET_MUX_ERROR_SPARE_SIZE)

#define SPI_DATA_MIN_SIZE		(SPI_DATA_HEADER_SIZE*2)
#define SPI_DATA_MAX_SIZE_PER_PACKET (SPI_DEV_MAX_PACKET_SIZE- \
		SPI_DATA_PACKET_HEADER_SIZE-SPI_DATA_HEADER_SIZE- \
		SPI_DATA_PACKET_MUX_ERROR_SPARE_SIZE)
#define SPI_DATA_MAX_COUNT_PER_PACKET		 240

#define SPI_DATA_IPC_QUEUE_SIZE	0x10000
#define SPI_DATA_RAW_QUEUE_SIZE	0x100000
#define SPI_DATA_RFS_QUEUE_SIZE	0x100000

#define SPI_DATA_IPC_DIV_BUFFER_SIZE	0x1000
#define SPI_DATA_RAW_DIV_BUFFER_SIZE	0x1000
#define SPI_DATA_RFS_DIV_BUFFER_SIZE	0x1000

#define SPI_DATA_DIVIDE_BUFFER_SIZE SPI_DEV_MAX_PACKET_SIZE



enum SPI_DATA_TYPE_T {
	SPI_DATA_MUX_IPC			= 0x01,
	SPI_DATA_MUX_RAW			= 0x02,
	SPI_DATA_MUX_RFS			= 0x03,
	SPI_DATA_MUX_CMD			= 0x04,
};

struct spi_data_packet_header {
	/* 12bit : packet size less than SPI_DEV_PACKET_SIZE */
	unsigned long current_data_size:31;
	/* 1bit : packet division flag */
	unsigned long more:1;
};

enum SPI_DATA_QUEUE_TYPE_T {
	SPI_DATA_QUEUE_TYPE_IPC_TX,	/* 0 : formatted(IPC) tx buffer */
	SPI_DATA_QUEUE_TYPE_IPC_RX,	/* 1 : formatted(IPC) rx buffer */
	SPI_DATA_QUEUE_TYPE_RAW_TX,	/* 2 : raw tx buffer */
	SPI_DATA_QUEUE_TYPE_RAW_RX,	/* 3 : raw rx buffer */
	SPI_DATA_QUEUE_TYPE_RFS_TX,	/* 4 : rfs tx buffer */
	SPI_DATA_QUEUE_TYPE_RFS_RX,	/* 5 : rfs rx buffer */
	SPI_DATA_QUEUE_TYPE_NB		/* 6 : number of buffer */
};

struct spi_data_queue {
	unsigned int tail; /* circular queue header */
	unsigned int head; /* circular queue tail */
};

struct spi_data_queue_info {
	struct spi_data_queue *header;
	enum SPI_DATA_TYPE_T type;
	unsigned int buf_size; /* queue size */
	char *buffer; /* queue data */
};

struct spi_data_div_buf {
	unsigned int	length;
	char *buffer;		 /* queue data */
};

extern void *ipc_spi_get_queue_buff(void);

extern struct spi_data_queue *spi_data_queue;
extern struct spi_data_queue_info	 *spi_queue_info;
extern char *gspi_data_packet_buf;

extern void spi_data_queue_init(void);
extern void spi_data_queue_destroy(void);
extern unsigned int spi_data_dequeue(struct spi_data_queue_info *queue_info,
		void *pdata);
extern int spi_data_inqueue(struct spi_data_queue_info *queue, void *data,
		unsigned int length);
extern void spi_data_msg_inqueuefront(enum SPI_MAIN_MSG_T task_msg);
extern int spi_data_queue_is_empty(enum SPI_DATA_QUEUE_TYPE_T type);
extern int spi_data_div_buf_is_empty(enum SPI_DATA_QUEUE_TYPE_T type);
extern int spi_data_check_tx_queue(void);

extern int spi_data_prepare_tx_packet(void *buf);
extern int spi_data_parsing_rx_packet(void *buf, unsigned int length);

extern void spi_data_print_raw_data(void *data, unsigned int length);
#endif



