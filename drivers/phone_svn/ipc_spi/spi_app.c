/**************************************************************

	spi_app.c



	interface spi and app



	This is MASTER side.

***************************************************************/



/**************************************************************

	Preprocessor by common

***************************************************************/

#include "spi_main.h"
#include "spi_app.h"
#include "spi_os.h"
#include "spi_data.h"
#include "spi_test.h"

static unsigned int _pack_spi_data(enum SPI_DATA_TYPE_T type, void *buf,
		void *data, unsigned int length);


/**********************************************************
Prototype		void spi_app_receive_msg ( struct spi_os_msg * msg )
Type		function
Description	receive data from spi task message queue and
			Pack data for spi data.
			Then inqueue the message to spi_data_queue_XXX_tx
Param input	msg	: message received from other task
Return value	(none)
***********************************************************/
void spi_receive_msg_from_app(struct spi_os_msg *msg)
{
	enum SPI_MAIN_MSG_T type;
	enum SPI_DATA_QUEUE_TYPE_T q_type;
	enum SPI_DATA_TYPE_T mux_type;
	unsigned int in_length = 0, out_length = 0;
	void *in_buffer = 0;
	void *out_buffer = 0;

	type = msg->signal_code;
	in_length = msg->data_length;
	in_buffer = msg->data;

	switch (type) {
	case SPI_MAIN_MSG_IPC_SEND:
		q_type = SPI_DATA_QUEUE_TYPE_IPC_TX;
		mux_type = SPI_DATA_MUX_IPC;
		break;

	case SPI_MAIN_MSG_RAW_SEND:
		q_type = SPI_DATA_QUEUE_TYPE_RAW_TX;
		mux_type = SPI_DATA_MUX_RAW;
		break;

	case SPI_MAIN_MSG_RFS_SEND:
		q_type = SPI_DATA_QUEUE_TYPE_RFS_TX;
		mux_type = SPI_DATA_MUX_RFS;
		break;

	default:
		SPI_OS_ASSERT(("[SPI] spi_app_receive_msg Unknown type"));
		return;
	}

	out_buffer = spi_os_malloc(in_length+SPI_DATA_HEADER_SIZE);
	out_length = _pack_spi_data(mux_type, out_buffer, in_buffer, in_length);

	if (spi_data_inqueue(&spi_queue_info[q_type], out_buffer,
		out_length) == 0) {
		SPI_OS_ASSERT(("[SPI] spi_app_receive_msg inqueue[%d] Fail",
			q_type));
	}

	spi_os_free(in_buffer);
	spi_os_free(out_buffer);
}


/**********************************************************
Prototype	void spi_send_msg ( void )
Type			function
Description	Dequeue a spi data from spi_data_queue_XXX_rx
			Unpack the spi data for ipc, raw or rfs data
			Send msg to other task until that all queues are empty
			CP use this functions for other task as below
			IPC : ipc_cmd_send_queue
			RAW : data_send_queue
			RFS : rfs_send_queue
Param input	(none)
Return value	(none)
***********************************************************/
void spi_send_msg_to_app(void)
{
	u32 int_cmd = 0;
	struct ipc_spi *od_spi = NULL;

	#define MB_VALID					0x0080
	#define MB_DATA(x)				(MB_VALID | x)
	#define MBD_SEND_FMT			0x0002
	#define MBD_SEND_RAW			0x0001
	#define MBD_SEND_RFS			0x0100


	od_spi = ipc_spi;


	if (spi_data_queue_is_empty(SPI_DATA_QUEUE_TYPE_IPC_RX) == 0) {
		int_cmd = MB_DATA(MBD_SEND_FMT);
		ipc_spi_make_data_interrupt(int_cmd, od_spi);
	}

	if (spi_data_queue_is_empty(SPI_DATA_QUEUE_TYPE_RAW_RX) == 0) {
		int_cmd = MB_DATA(MBD_SEND_RAW);
		ipc_spi_make_data_interrupt(int_cmd, od_spi);
	}

	if (spi_data_queue_is_empty(SPI_DATA_QUEUE_TYPE_RFS_RX) == 0) {
		int_cmd = MB_DATA(MBD_SEND_RFS);
		ipc_spi_make_data_interrupt(int_cmd, od_spi);
	}
}


/**********************************************************
Prototype	unsigned int _pack_spi_data (SPI_DATA_TYPE_T type,void * buf, void * data, unsigned int length)
Type			static function
Description	pack data for spi
Param input	type		: type of data type
			buf			: address of buffer to be saved
			data		: address of data to pack
			length	: length of input data
Return value	length of packed data
***********************************************************/
static unsigned int _pack_spi_data(enum SPI_DATA_TYPE_T type, void *buf,
		void *data, unsigned int length)
{
	char *spi_packet	 = NULL;
	unsigned int out_length = 0;

	spi_packet = (char *) buf;

	spi_os_memset((char *)spi_packet, 0x00, (unsigned int)length);
	spi_os_memset((char *)spi_packet, (unsigned char)SPI_DATA_BOF,
		SPI_DATA_BOF_SIZE);
	spi_os_memcpy((char *)spi_packet + SPI_DATA_BOF_SIZE, data, length);
	spi_os_memset((char *)spi_packet + SPI_DATA_BOF_SIZE + length,
		(unsigned char)SPI_DATA_EOF, SPI_DATA_EOF_SIZE);

	out_length = SPI_DATA_BOF_SIZE + length + SPI_DATA_EOF_SIZE;

	return out_length;
}


/**********************************************************
Prototype	int spi_app_ready ( void )
Type			function
Description	check if spi initialization is done.
			Decide it as spi_main_state.
Param input	(none)
Return value	1	 : spi initialization is done.
			0	 : spi initialization is not done.
***********************************************************/
int spi_is_ready(void)
{
	if ((spi_main_state == SPI_MAIN_STATE_START) ||
		(spi_main_state == SPI_MAIN_STATE_END))
		return 0;

	return 1;
}

