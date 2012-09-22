/**************************************************************

	spi_data.c



	handling spi data



	This is MASTER side.

***************************************************************/



/**************************************************************

	Preprocessor by common

***************************************************************/

#include "spi_data.h"
#include "spi_os.h"
#include "spi_dev.h"



/**************************************************************

	Definition of Variables and Functions by common

***************************************************************/

struct spi_data_queue	*spi_queue;
struct spi_data_queue_info	*spi_queue_info;
struct spi_data_div_buf *spi_div_buf;

char *gspi_data_prepare_packet;

char *gspi_data_packet_buf;

static int _prepare_tx_type_packet(void *buf,
		struct spi_data_queue_info *queue_info,
		struct spi_data_div_buf *spi_data_buf,
		enum SPI_DATA_TYPE_T spi_type);
static int _spi_data_verify(void *buf,  unsigned int mux);


/**********************************************************

Prototype		void spi_data_queue_init ( void )

Type		function

Description	init buffer and data variable

Param input	(none)

Return value	(none)

***********************************************************/
void spi_data_queue_init(void)
{
	int i = 0;

	/* This is defined at ipc_spi.c file and copied. */
	#define FMT_OUT 0x0FE000
	#define FMT_IN		0x10E000
	#define FMT_SZ		0x10000   /* 65536 bytes */

	#define RAW_OUT 0x11E000
	#define RAW_IN		0x21E000
	#define RAW_SZ		0x100000 /* 1 MB */

	#define RFS_OUT 0x31E000
	#define RFS_IN		0x41E000
	#define RFS_SZ		0x100000 /* 1 MB */

	void *p_virtual_buff;
	unsigned int buffer_offset[SPI_DATA_QUEUE_TYPE_NB] = {
		FMT_OUT, FMT_IN, RAW_OUT, RAW_IN, RFS_OUT, RFS_IN};

	p_virtual_buff = ipc_spi_get_queue_buff();
	spi_queue = p_virtual_buff+sizeof(struct spi_data_queue)*2;


	spi_queue_info = (struct spi_data_queue_info *) spi_os_vmalloc(
		sizeof(struct spi_data_queue_info)*SPI_DATA_QUEUE_TYPE_NB);
	spi_os_memset(spi_queue_info, 0x00,
		sizeof(struct spi_data_queue_info)*SPI_DATA_QUEUE_TYPE_NB);

	spi_div_buf = (struct spi_data_div_buf *) spi_os_vmalloc(
		sizeof(struct spi_data_div_buf)*SPI_DATA_QUEUE_TYPE_NB);
	spi_os_memset(spi_div_buf, 0x00, sizeof(struct spi_data_div_buf)
		*SPI_DATA_QUEUE_TYPE_NB);
	gspi_data_prepare_packet = (char *) spi_os_vmalloc(
		SPI_DEV_MAX_PACKET_SIZE);
	gspi_data_packet_buf = (char *) spi_os_malloc(SPI_DEV_MAX_PACKET_SIZE);

	for (i = 0 ; i < SPI_DATA_QUEUE_TYPE_NB ; i++) {
		spi_queue_info[i].header = &spi_queue[i];

		switch (i) {
		case SPI_DATA_QUEUE_TYPE_IPC_TX:
		case SPI_DATA_QUEUE_TYPE_IPC_RX:
			spi_queue_info[i].buf_size = SPI_DATA_IPC_QUEUE_SIZE;
			spi_queue_info[i].type = SPI_DATA_MUX_IPC;
			spi_div_buf[i].buffer = (char *)spi_os_vmalloc(
				SPI_DATA_IPC_DIV_BUFFER_SIZE);
			break;
		case SPI_DATA_QUEUE_TYPE_RAW_TX:
		case SPI_DATA_QUEUE_TYPE_RAW_RX:
			spi_queue_info[i].buf_size = SPI_DATA_RAW_QUEUE_SIZE;
			spi_queue_info[i].type = SPI_DATA_MUX_RAW;
			spi_div_buf[i].buffer = (char *)spi_os_vmalloc(
				SPI_DATA_RAW_DIV_BUFFER_SIZE);
			break;
		case SPI_DATA_QUEUE_TYPE_RFS_TX:
		case SPI_DATA_QUEUE_TYPE_RFS_RX:
			spi_queue_info[i].buf_size = SPI_DATA_RFS_QUEUE_SIZE;
			spi_queue_info[i].type = SPI_DATA_MUX_RFS;
			spi_div_buf[i].buffer = (char *)spi_os_vmalloc(
				SPI_DATA_RFS_DIV_BUFFER_SIZE);
			break;
		}

		if (spi_queue_info[i].buffer == 0)
			spi_queue_info[i].buffer =
				(char *)(p_virtual_buff + buffer_offset[i]);
	}
}


/**********************************************************

Prototype		void spi_data_queue_destroy ( void )

Type		function

Description	memfree buffer and data variable

Param input	(none)

Return value	(none)

***********************************************************/

void spi_data_queue_destroy(void)
{
	int i = 0;

	if (spi_div_buf != NULL) {
		for (i = 0 ; i < SPI_DATA_QUEUE_TYPE_NB ; i++) {
			if (spi_div_buf[i].buffer != NULL) {
				spi_os_vfree(spi_div_buf[i].buffer);
				spi_div_buf[i].buffer = NULL;
			}
		}
	}

	if (gspi_data_prepare_packet != NULL) {
		spi_os_vfree(gspi_data_prepare_packet);
		gspi_data_prepare_packet = NULL;
	}

	if (gspi_data_packet_buf != NULL) {
		spi_os_free(gspi_data_packet_buf);
		gspi_data_packet_buf = NULL;
	}

	if (spi_div_buf != NULL) {
		spi_os_vfree(spi_div_buf);
		spi_div_buf = NULL;
	}

	if (spi_queue_info != NULL) {
		spi_os_vfree(spi_queue_info);
		spi_queue_info = NULL;
	}
}



/**********************************************************

Prototype	int spi_data_inqueue (struct spi_data_queue_info * queue_info,
					void * data, unsigned int length )

Type		function

Description	inqueue data to spi rx or tx buffer

Param input	queue_info : queue for inqueue

			data : address of data

			lenth : length of data

Return value	0 : fail

			1 : success

***********************************************************/
int spi_data_inqueue(struct spi_data_queue_info *queue_info,
		void *data, unsigned int length)
{
	char *pdata = NULL;
	struct spi_data_queue *queue = NULL;
	pdata = data;
	queue = queue_info->header;

	if (queue->tail < queue->head) {
		if ((queue->head - queue->tail) < length) {
			SPI_OS_ASSERT(("%s%s[%u],%s[%u],%s[%u],%s[%u]\n",
				"[SPI] ERROR : spi_data_inqueue : ",
				"queue is overflow head", queue->head,
				"tail", queue->tail, "length", length,
				"buf_size", queue_info->buf_size));
			return 0;
		}
	} else {
		if ((queue_info->buf_size - (queue->tail - queue->head))
			< length) {
			SPI_OS_ASSERT(("%s%s[%u],%s[%u],%s[%u],%s[%u]\n",
			"[SPI] ERROR : spi_data_inqueue : queue is overflow ",
			"head", queue->head, "tail", queue->tail,
			"request length", length,
			"buf_size", queue_info->buf_size));
			return 0;
		}
	}

	if (&spi_queue_info[SPI_DATA_QUEUE_TYPE_IPC_RX] == queue_info
	  || &spi_queue_info[SPI_DATA_QUEUE_TYPE_RAW_RX] == queue_info
	  || &spi_queue_info[SPI_DATA_QUEUE_TYPE_RFS_RX] == queue_info) {
		pdata += (SPI_DATA_MUX_SIZE+SPI_DATA_LENGTH_SIZE);
		length -= (SPI_DATA_MUX_SIZE+SPI_DATA_LENGTH_SIZE);
	}

	if (queue_info->buf_size < (queue->tail + length)) {
		/* maximum  < tail + length */
		unsigned int pre_data_len = 0;
		pre_data_len = queue_info->buf_size - queue->tail;
		spi_os_memcpy(queue_info->buffer + queue->tail,
			pdata, pre_data_len);
		spi_os_memcpy(queue_info->buffer, pdata + pre_data_len,
			length - pre_data_len);
		queue->tail = ((queue->tail+length)%queue_info->buf_size);
	} else if (queue_info->buf_size == (queue->tail + length)) {
		/* maximum  == tail + length */
		spi_os_memcpy((queue_info->buffer + queue->tail),
			(char *)pdata, length);
		queue->tail = 0;
	} else {
		/* maximum > tail + length */
		spi_os_memcpy((queue_info->buffer + queue->tail),
			(char *)pdata, length);
		queue->tail = queue->tail + length;
	}

	return 1;
}


/**********************************************************

Prototype		unsigned int spi_data_dequeue
		(struct spi_data_queue_info * queue_info, void * pdata)

Type		function

Description	dequeue data to spi

Param input	queue_info : queue for dequeue

			pdata : address of data

Return value	0 : fail

			1 : success

***********************************************************/

unsigned int spi_data_dequeue(struct spi_data_queue_info *queue_info,
		void *pdata)
{
	unsigned int length = 0, buf_length = 0, pre_data_len = 0;
	struct spi_data_queue *queue = NULL;

	queue = queue_info->header;

	if (queue->tail == queue->head) { /* empty */
		SPI_OS_ERROR(("%s %s\n",
			"[SPI] ERROR : spi_data_dequeue:",
			"queue is empty"));
		return 0;
	}

	if (queue->head == queue_info->buf_size)
		queue->head = 0;

	/* check  length of data */
	if (&spi_queue_info[SPI_DATA_QUEUE_TYPE_IPC_TX] == queue_info
		|| &spi_queue_info[SPI_DATA_QUEUE_TYPE_IPC_RX] == queue_info) {
		/* IPC Case */
		if (queue_info->buf_size == (queue->head + SPI_DATA_BOF_SIZE)) {
			/* maximum == head + pos_len */
			spi_os_memcpy(&length, queue_info->buffer,
				SPI_DATA_IPC_INNER_LENGTH_SIZE);
		} else if (queue_info->buf_size < (queue->head +
			SPI_DATA_BOF_SIZE +
			SPI_DATA_IPC_INNER_LENGTH_SIZE)) {
			/* maximum  < head + pos_len */
			char data_header[SPI_DATA_IPC_INNER_LENGTH_SIZE] = {0,};
			pre_data_len = queue_info->buf_size - queue->head -
				SPI_DATA_BOF_SIZE;

			spi_os_memcpy(data_header, (queue_info->buffer +
				queue->head + SPI_DATA_BOF_SIZE), pre_data_len);
			spi_os_memcpy(data_header + pre_data_len,
				queue_info->buffer,
				SPI_DATA_IPC_INNER_LENGTH_SIZE - pre_data_len);
			spi_os_memcpy(&length, data_header,
				SPI_DATA_IPC_INNER_LENGTH_SIZE);
		} else { /* maximum > head + pos_len */
			spi_os_memcpy(&length,
				(queue_info->buffer + queue->head +
				SPI_DATA_BOF_SIZE),
				SPI_DATA_IPC_INNER_LENGTH_SIZE);
		}
	} else { /* RAW, RFS Case */
		if (queue_info->buf_size == (queue->head + SPI_DATA_BOF_SIZE)) {
			/* maximum == head + pos_len */
			spi_os_memcpy(&length, queue_info->buffer,
				SPI_DATA_INNER_LENGTH_SIZE);
		} else if (queue_info->buf_size < (queue->head +
			SPI_DATA_BOF_SIZE + SPI_DATA_INNER_LENGTH_SIZE)) {
			/* maximum < head + pos_len */
			char data_header[SPI_DATA_INNER_LENGTH_SIZE] = {0,};
			pre_data_len = queue_info->buf_size -
				queue->head - SPI_DATA_BOF_SIZE;

			spi_os_memcpy(data_header,
				(queue_info->buffer + queue->head +
				SPI_DATA_BOF_SIZE),
				pre_data_len);
			spi_os_memcpy(data_header + pre_data_len,
				queue_info->buffer,
				SPI_DATA_INNER_LENGTH_SIZE - pre_data_len);
			spi_os_memcpy(&length, data_header,
				SPI_DATA_INNER_LENGTH_SIZE);
		} else { /* maximum > head + pos_len */
			spi_os_memcpy(&length,
				(queue_info->buffer + queue->head +
				SPI_DATA_BOF_SIZE),
				SPI_DATA_INNER_LENGTH_SIZE);
		}
	}
	length += SPI_DATA_BOF_SIZE + SPI_DATA_EOF_SIZE;
	buf_length = length + SPI_DATA_MUX_SIZE + SPI_DATA_LENGTH_SIZE;

	if (length > SPI_DEV_MAX_PACKET_SIZE) {
		if (&spi_queue_info[SPI_DATA_QUEUE_TYPE_IPC_TX] == queue_info ||
		&spi_queue_info[SPI_DATA_QUEUE_TYPE_IPC_RX] == queue_info) {
			/* IPC Case */
			SPI_OS_ERROR(("%s %s[%x],%s[%u],%s[%u],%s[%u]\n",
				"[SPI] ERROR : spi_data_dequeue: IPC error",
				"length", length, "buf_size",
				queue_info->buf_size, "head",
				queue->head, "tail", queue->tail));
		} else if (
		&spi_queue_info[SPI_DATA_QUEUE_TYPE_RAW_TX] == queue_info
		|| &spi_queue_info[SPI_DATA_QUEUE_TYPE_RAW_RX] == queue_info) {
			/* RAW Case */
			SPI_OS_ERROR(("%s %s[%x],%s[%u],%s[%u],%s[%u]\n",
				"[SPI] ERROR : spi_data_dequeue: RAW error",
				"length", length,
				"buf_size", queue_info->buf_size,
				"head", queue->head, "tail", queue->tail));
		} else { /* RFS Case */
			SPI_OS_ERROR(("%s %s[%x],%s[%u],%s[%u],%s[%u]\n",
				"[SPI] ERROR : spi_data_dequeue: RFS error",
				"length", length,
				"buf_size", queue_info->buf_size,
				"head", queue->head, "tail", queue->tail));
		}
		spi_os_trace_dump_low("spi_data_dequeue error",
			queue_info->buffer + queue->head - 1, 16);
		spi_os_trace_dump_low("spi_data_dequeue error",
			queue_info->buffer + queue->tail - 1, 16);
		return 0;
	}

	if (&spi_queue_info[SPI_DATA_QUEUE_TYPE_IPC_TX] == queue_info
	  || &spi_queue_info[SPI_DATA_QUEUE_TYPE_RAW_TX] == queue_info
	  || &spi_queue_info[SPI_DATA_QUEUE_TYPE_RFS_TX] == queue_info) {
		unsigned int templength;

		spi_os_memcpy((char *) pdata, &queue_info->type,
			SPI_DATA_MUX_SIZE);
		pdata += SPI_DATA_MUX_SIZE;
		templength = length-SPI_DATA_BOF_SIZE-SPI_DATA_EOF_SIZE;
		spi_os_memcpy((char *) pdata, &templength,
			SPI_DATA_LENGTH_SIZE);
		pdata += SPI_DATA_LENGTH_SIZE;
	}

	if (queue->tail > queue->head) {
		if (queue->tail - queue->head < length) {
			SPI_OS_ERROR(("%s %s tail[%u], head[%u], length[%u]\n",
				"[SPI] ERROR : spi_data_dequeue:",
				"request data length is less than queue`s remain data.",
				queue->tail, queue->head, length));

			spi_os_trace_dump_low("spi_data_dequeue error",
				queue_info->buffer + queue->head - 1, 16);
			spi_os_trace_dump_low("spi_data_dequeue error",
				queue_info->buffer + queue->tail - 1, 16);
			return 0;
		}
	} else if (queue->tail < queue->head) {
		if ((queue_info->buf_size - queue->head + queue->tail)
			< length) {
			SPI_OS_ERROR(("%s %s tail[%u], head[%u], length[%u]\n",
				"[SPI] ERROR : spi_data_dequeue:",
				"request data length is less than queue`s remain data.",
				queue->tail, queue->head, length));

			spi_os_trace_dump_low("spi_data_dequeue error",
				queue_info->buffer + queue->head - 1, 16);
			spi_os_trace_dump_low("spi_data_dequeue error",
				queue_info->buffer + queue->tail - 1, 16);
			return 0;
		}
	}

	if (queue_info->buf_size < (queue->head+length)) {
		/* maximum < head + length */
		pre_data_len = queue_info->buf_size - queue->head;
		spi_os_memcpy((char *)pdata,
			queue_info->buffer + queue->head, pre_data_len);
		spi_os_memcpy((char *)pdata + pre_data_len,
			queue_info->buffer, length - pre_data_len);
		queue->head = length - pre_data_len;
	} else if (queue_info->buf_size == (queue->head+length)) {
		/* maximum = head + length */
		spi_os_memcpy((char *)pdata,
			(queue_info->buffer + queue->head), length);
		queue->head = 0;
	} else { /* maximum > head + length */
		spi_os_memcpy((char *)pdata,
			(queue_info->buffer + queue->head), length);
		queue->head = queue->head + length;
	}

	return buf_length;
}


/**********************************************************

Prototype		int spi_data_queue_is_empty

Type		function

Description check queue space empty or not

Param input	type	: queue type

Return value	0 : not empty

			1 : empty

***********************************************************/

int spi_data_queue_is_empty(enum SPI_DATA_QUEUE_TYPE_T type)
{
	if (spi_queue[type].head == spi_queue[type].tail)
		return 1; /* empty */
	else
		return 0; /* not empty */
}


/**********************************************************

Prototype		int spi_data_div_buf_is_empty

Type		function

Description check queue space empty or not

Param input	type	: queue type

Return value	0 : not empty

			1 : empty

***********************************************************/

int spi_data_div_buf_is_empty(enum SPI_DATA_QUEUE_TYPE_T type)
{
	if (spi_div_buf[type].length > 0)
		return 0; /* not empty */
	else
		return 1; /* Empty */
}


/**********************************************************

Prototype		int spi_data_check_tx_queue( void )

Type		function

Description check tx queue space empty or not

Param input	(none)

Return value	0 : empty

			1 : data exist

***********************************************************/

int spi_data_check_tx_queue(void)
{
	if ((spi_data_queue_is_empty(SPI_DATA_QUEUE_TYPE_IPC_TX) == 0)
		|| (spi_data_queue_is_empty(SPI_DATA_QUEUE_TYPE_RAW_TX) == 0)
		|| (spi_data_queue_is_empty(SPI_DATA_QUEUE_TYPE_RFS_TX) == 0))
		return 1; /* has data */
	else if ((spi_data_div_buf_is_empty(SPI_DATA_QUEUE_TYPE_IPC_TX) == 0)
		|| (spi_data_div_buf_is_empty(SPI_DATA_QUEUE_TYPE_RAW_TX) == 0)
		|| (spi_data_div_buf_is_empty(SPI_DATA_QUEUE_TYPE_RFS_TX) == 0))
		return 1; /* has data */
	else
		return 0; /* empty */
}


/**********************************************************

Prototype		int spi_data_prepare_tx_packet ( void * buf )

Type		function

Description prepare packet for sync. attach spi_data_protocol_header,
			mux, bof, data and eof

Param input	buf	: address of data

Return value	0	: fail

			1	: success

***********************************************************/

int spi_data_prepare_tx_packet(void *buf)
{
	struct spi_data_queue *queue = NULL;
	int flag_to_send = 0;

	/* process IPC data */
	queue = &spi_queue[SPI_DATA_QUEUE_TYPE_IPC_TX];
	if ((queue->head != queue->tail)
		|| (spi_div_buf[SPI_DATA_QUEUE_TYPE_IPC_TX].length > 0)) {
		if (_prepare_tx_type_packet(buf,
			&spi_queue_info[SPI_DATA_QUEUE_TYPE_IPC_TX],
			&spi_div_buf[SPI_DATA_QUEUE_TYPE_IPC_TX],
			SPI_DATA_MUX_IPC) == 0)
			return SPI_DATA_PACKET_HEADER_SIZE +
			((struct spi_data_packet_header *)
			buf)->current_data_size;

		flag_to_send++;
	}

	/* process RAW data */
	queue = &spi_queue[SPI_DATA_QUEUE_TYPE_RAW_TX];
	if ((queue->head != queue->tail)
		|| (spi_div_buf[SPI_DATA_QUEUE_TYPE_RAW_TX].length > 0)) {
		if (_prepare_tx_type_packet(buf,
			&spi_queue_info[SPI_DATA_QUEUE_TYPE_RAW_TX],
			&spi_div_buf[SPI_DATA_QUEUE_TYPE_RAW_TX],
			SPI_DATA_MUX_RAW) == 0)
			return SPI_DATA_PACKET_HEADER_SIZE +
				((struct spi_data_packet_header *)
				buf)->current_data_size;

		flag_to_send++;
	}

	/* process RFS data */
	queue = &spi_queue[SPI_DATA_QUEUE_TYPE_RFS_TX];
	if ((queue->head != queue->tail)
		|| (spi_div_buf[SPI_DATA_QUEUE_TYPE_RFS_TX].length > 0)) {
		if (_prepare_tx_type_packet(buf,
			&spi_queue_info[SPI_DATA_QUEUE_TYPE_RFS_TX],
			&spi_div_buf[SPI_DATA_QUEUE_TYPE_RFS_TX],
			SPI_DATA_MUX_RFS) == 0)
			return SPI_DATA_PACKET_HEADER_SIZE +
				((struct spi_data_packet_header *)
				buf)->current_data_size;

		flag_to_send++;
	}

	if (flag_to_send == 0)
		return 0;
	else
		return SPI_DATA_PACKET_HEADER_SIZE +
			((struct spi_data_packet_header *)
			buf)->current_data_size;
}


/**********************************************************

Prototype		static int _prepare_tx_type_packet

Type		static function

Description	prepare spi data for copy to spi packet from rx queue.
			If spi data bigger than spi packet free size,
			it need separate and inqueue extra data

Param input	buf		: address of buffer to be saved
			queue_info : tx queue for IPC or RAW or RFS
			spi_data_buf : tx buffer for IPC or RAW or RFS

Return value	0		: buffer full

			(other)	: packet size include header and tail

***********************************************************/

static int _prepare_tx_type_packet(void *buf,
	struct spi_data_queue_info *queue_info,
	struct spi_data_div_buf *spi_data_buf,
	enum SPI_DATA_TYPE_T spi_type)
{
	char *spi_packet = NULL;

	struct spi_data_packet_header	*spi_packet_header = NULL;
	unsigned int spi_packet_free_length = 0;

	unsigned int cur_dequeue_length = 0;
	unsigned int spi_packet_count = 0;
	unsigned int spi_data_mux = 0;

	struct spi_data_queue *queue = NULL;

	queue = queue_info->header;

	spi_packet = (char *)buf;
	spi_packet_header = (struct spi_data_packet_header *)buf;
	spi_packet_free_length = SPI_DATA_PACKET_MAX_PACKET_BODY_SIZE -
		spi_packet_header->current_data_size;

	/* not enough space in spi packet */
	/* spi_packet_header->current_data_size > 2022(2048 - 16) */
	if (spi_packet_header->current_data_size >
		SPI_DATA_PACKET_MAX_PACKET_BODY_SIZE - SPI_DATA_MIN_SIZE) {
		if (spi_data_check_tx_queue() == 1)
			spi_packet_header->more = 1;
		return 0;
	}

	while ((queue->head != queue->tail) || (spi_data_buf->length > 0)) {
		spi_os_memset(gspi_data_prepare_packet,
			0, SPI_DEV_MAX_PACKET_SIZE);
		cur_dequeue_length = 0;

		/* dequeue SPI data */
		if (spi_data_buf->length > 0) {
			if ((*(unsigned int *)spi_data_buf->buffer)
				== SPI_DATA_FF_PADDING_HEADER) {
				/* if data has 0xFF padding header */
				/* send packet directly */
				spi_os_memcpy(spi_packet, spi_data_buf->buffer,
					spi_data_buf->length);
				spi_data_buf->length = 0;
				return 0;
			} else {
				/* read from tx div buf */
				spi_os_memcpy(gspi_data_prepare_packet,
					spi_data_buf->buffer,
					spi_data_buf->length);
				cur_dequeue_length = spi_data_buf->length;
				spi_data_buf->length = 0;
			}
		} else {
			/* read from tx queue */
			cur_dequeue_length = spi_data_dequeue(queue_info,
				gspi_data_prepare_packet);
		}

		if (cur_dequeue_length == 0)
			continue;

		if (spi_packet_free_length < cur_dequeue_length) {
			spi_os_memcpy(spi_data_buf->buffer,
				gspi_data_prepare_packet, cur_dequeue_length);
			spi_data_buf->length = cur_dequeue_length;
			spi_packet_header->more = 1;
			return 0;
		}

		/* check mux value */
		spi_os_memcpy(&spi_data_mux, gspi_data_prepare_packet,
			SPI_DATA_MUX_SIZE);
		if (spi_data_mux == 0)
			spi_os_memcpy(gspi_data_prepare_packet,
				&spi_type, SPI_DATA_MUX_SIZE);

		spi_os_memcpy(spi_packet + SPI_DATA_PACKET_HEADER_SIZE +
			spi_packet_header->current_data_size,
			gspi_data_prepare_packet, cur_dequeue_length);

		/* update header */
		spi_packet_header->current_data_size += cur_dequeue_length;
		spi_packet_free_length -= cur_dequeue_length;

		/* increase spi packet count */
		spi_packet_count++;

		/* check spi packet size */
		if (spi_packet_free_length < SPI_DATA_MIN_SIZE) {
			if (spi_data_check_tx_queue() == 1)
				spi_packet_header->more = 1;
			return 0;
		}

		/* check spi maximum count per packet */
		if (spi_packet_count >= SPI_DATA_MAX_COUNT_PER_PACKET) {

			SPI_OS_ERROR(("%s %s\n",
				"[SPI] ERROR : spi _prepare_tx_type_packet :",
				"spi_packet_count is full"));

			return 0;
		}
	}

	return spi_packet_header->current_data_size +
		SPI_DATA_PACKET_HEADER_SIZE;
}


/**********************************************************

Prototype		int spi_data_parsing_rx_packet
				( void * buf, unsigned int length )

Type		function

Description	parsing rx packet for transfer to application. It extract data

			and inqueue each SPI_DATA_TYPE queue

Param input	buf		: address of buffer to be saved

			lenth	: length of data

Return value	0	: fail

			1	: success

***********************************************************/

int spi_data_parsing_rx_packet(void *buf, unsigned int length)
{
	struct spi_data_packet_header *spi_packet_header = NULL;
	char *spi_packet = NULL;
	unsigned int spi_packet_length	= 0;
	unsigned int spi_packet_cur_pos = SPI_DATA_PACKET_HEADER_SIZE;

	unsigned int spi_data_mux = 0;
	unsigned int spi_data_length = 0;
	char *spi_cur_data		= NULL;

	struct spi_data_queue_info *queue_info = NULL;
	struct spi_data_div_buf *tx_div_buf = NULL;


	/* check spi packet header */
	if (*(unsigned int *)buf == 0x00000000
		|| *(unsigned int *)buf == 0xFFFFFFFF) {
		/* if spi header is invalid, */
		/* read spi header again with next 4 byte */
		buf += SPI_DATA_PACKET_HEADER_SIZE;
	}

	spi_packet = (char *) buf;

	/* read spi packet header */
	spi_packet_header = (struct spi_data_packet_header *) buf;
	spi_packet_length = SPI_DATA_PACKET_HEADER_SIZE +
		spi_packet_header->current_data_size;


	do {
		/* read spi data mux and set current queue */
		spi_os_memcpy(&spi_data_mux,
			spi_packet + spi_packet_cur_pos, SPI_DATA_MUX_SIZE);

		switch (spi_data_mux & SPI_DATA_MUX_NORMAL_MASK) {
		case SPI_DATA_MUX_IPC:
			queue_info =
			&spi_queue_info[SPI_DATA_QUEUE_TYPE_IPC_RX];
			tx_div_buf =
				&spi_div_buf[SPI_DATA_QUEUE_TYPE_IPC_RX];
			break;

		case SPI_DATA_MUX_RAW:
			queue_info =
				&spi_queue_info[SPI_DATA_QUEUE_TYPE_RAW_RX];
			tx_div_buf =
				&spi_div_buf[SPI_DATA_QUEUE_TYPE_RAW_RX];
			break;

		case SPI_DATA_MUX_RFS:
			queue_info =
				&spi_queue_info[SPI_DATA_QUEUE_TYPE_RFS_RX];
			tx_div_buf =
				&spi_div_buf[SPI_DATA_QUEUE_TYPE_RFS_RX];
			break;

		default:
			SPI_OS_ERROR(("%s len[%u], pos[%u]\n",
				"[SPI] ERROR : spi_data_parsing_rx_packet : MUX error",
				spi_packet_length, spi_packet_cur_pos));

			spi_os_trace_dump_low("mux error",
				spi_packet + spi_packet_cur_pos, 16);
			return spi_packet_cur_pos - SPI_DATA_PACKET_HEADER_SIZE;
		}

		/* read spi data length */
		spi_os_memcpy(&spi_data_length, spi_packet +
			spi_packet_cur_pos + SPI_DATA_LENGTH_OFFSET,
			SPI_DATA_LENGTH_SIZE);

		if (spi_data_mux & SPI_DATA_MUX_MORE_H
			|| spi_data_mux & SPI_DATA_MUX_MORE_M)
			spi_data_length += SPI_DATA_HEADER_SIZE_FRONT;
		else if (spi_data_mux & SPI_DATA_MUX_MORE_T)
			spi_data_length += SPI_DATA_HEADER_SIZE;
		else
			spi_data_length += SPI_DATA_HEADER_SIZE;

		/* read data and make spi data */
		spi_cur_data = spi_packet + spi_packet_cur_pos;

		/* verify spi data */
		if (_spi_data_verify(spi_cur_data, spi_data_mux) == 0) {
			spi_packet_cur_pos += spi_data_length;
			continue;
		}

		/* inqueue rx buffer */
		if (spi_data_mux & SPI_DATA_MUX_MORE_H
			|| spi_data_mux & SPI_DATA_MUX_MORE_M) {
			/* middle of divided packet. save to rx_div_buf */
			spi_os_memcpy((void *)(tx_div_buf->buffer +
				tx_div_buf->length),
				spi_cur_data, spi_data_length);
			tx_div_buf->length +=
				(spi_data_length - SPI_DATA_HEADER_SIZE_FRONT);
		} else if (spi_data_mux & SPI_DATA_MUX_MORE_T) {
			unsigned int spi_origine_len = 0;

			/* tail of divided packet. save to rx_div_buf */
			spi_os_memcpy((void *)(tx_div_buf->buffer +
				tx_div_buf->length),
				spi_cur_data, spi_data_length);
			tx_div_buf->length += spi_data_length;
			/* update spi data length at spi header */
			spi_origine_len =
				tx_div_buf->length - SPI_DATA_HEADER_SIZE;
			spi_os_memcpy(tx_div_buf->buffer +
				SPI_DATA_LENGTH_OFFSET,
				&(spi_origine_len), SPI_DATA_LENGTH_SIZE);

			/* inqueue from rx_div_buf */
			spi_data_inqueue(queue_info,
				tx_div_buf->buffer, tx_div_buf->length);

			spi_os_memset(tx_div_buf->buffer,
				0, SPI_DATA_DIVIDE_BUFFER_SIZE);
			tx_div_buf->length = 0;
		} else {
			/* normal packet */
			spi_data_inqueue(queue_info,
				spi_cur_data, spi_data_length);
		}

		/* move spi packet current posision */
		spi_packet_cur_pos += spi_data_length;
	} while ((spi_packet_length - 1) > spi_packet_cur_pos);

	return 1;
}


/**********************************************************

Prototype		int _spi_data_verify( void * buf,  unsigned int mux )

Type		function

Description	verify integrity of packet

Param input	buf	: address of packet data received

			mux		: data selection

Return value	0	: fail

			1	: success

***********************************************************/

int _spi_data_verify(void *buf, unsigned int mux)
{
	unsigned int length = 0;

	unsigned int max_data_len = SPI_DATA_MAX_SIZE_PER_PACKET;
	char bof = 0x00;
	char eof = 0x00;

	spi_os_memcpy(&length, (char *)buf+SPI_DATA_LENGTH_OFFSET,
		SPI_DATA_LENGTH_SIZE);
	spi_os_memcpy(&bof, (char *)buf+SPI_DATA_BOF_OFFSET, SPI_DATA_BOF_SIZE);

	if (mux & SPI_DATA_MUX_MORE_H || mux & SPI_DATA_MUX_MORE_M)
		max_data_len += SPI_DATA_EOF_SIZE;

	if (bof == SPI_DATA_BOF) {
		if (length <= max_data_len) {
			if (mux & SPI_DATA_MUX_MORE_H
				|| mux & SPI_DATA_MUX_MORE_M)
				return 1;
			else {
				spi_os_memcpy(&eof,
					(char *)buf+SPI_DATA_EOF_OFFSET(length),
					SPI_DATA_EOF_SIZE);

				if (eof != SPI_DATA_EOF) {
					SPI_OS_ERROR(("%s %s\n",
						"[SPI] ERROR : _spi_data_verify:",
						"invalid"));

					return 0;
				}
			}
		} else {
			SPI_OS_ERROR(("%s %s\n",
				"[SPI] ERROR : _spi_data_verify:",
				"invalid : length error"));

			return 0;
		}
	} else {
		SPI_OS_ERROR(("%s %s\n",
			"[SPI] ERROR : _spi_data_verify:",
			"invalid : bof error"));

		return 0;
	}

	return 1;
}
