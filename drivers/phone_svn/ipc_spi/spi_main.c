/**************************************************************
	spi_main.c

	process of spi

	This is MASTER side.
***************************************************************/

/**************************************************************
	Preprocessor by common
***************************************************************/

#include "spi_main.h"
#include "spi_os.h"
#include "spi_dev.h"
#include "spi_app.h"
#include "spi_data.h"
#include "spi_test.h"

/**************************************************************
	Preprocessor by platform
	(OMAP4430 && MSM7X30)
***************************************************************/
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/semaphore.h>
static struct wake_lock spi_wakelock;

/**************************************************************
	Definition of Variables and Functions by common
***************************************************************/
enum SPI_MAIN_STATE_T spi_main_state = SPI_MAIN_STATE_END;



static int _start_data_send(void);

static void _start_packet_tx(void);

static void _start_packet_tx_send(void);

static void _start_packet_receive(void);




/**************************************************************
	Definition of Variables and Functions by platform
***************************************************************/

#define MRDY_GUARANTEE_MRDY_TIME_GAP 0 /* under 1ms */

#define MRDY_GUARANTEE_MRDY_TIME_SLEEP 2

#define MRDY_GUARANTEE_MRDY_LOOP_COUNT 10000 /* 120us */


static void spi_main_process(struct work_struct *pspi_wq);



#ifdef SPI_GUARANTEE_MRDY_GAP

unsigned long mrdy_low_time_save, mrdy_high_time_save;

int check_mrdy_time_gap(unsigned long x, unsigned long y)
{
	if (x == y)
		return 1;
	else if ((x < y) && ((y-x) <= MRDY_GUARANTEE_MRDY_TIME_GAP))
		return 1;
	else if ((x > y))
		return 1;
	else
		return 0;
}
#endif


/*=====================================
* Description set state of spi
	(Common)
=====================================*/
void spi_main_set_state(enum SPI_MAIN_STATE_T state)
{
	spi_main_state = state;
	SPI_OS_TRACE_MID(("[SPI] spi_main_set_state %d=>%d\n",
		(int)spi_main_state, (int)state));
}


/*=====================================
* Description	an interrupt handler for mrdy rising
=====================================*/
SPI_DEV_IRQ_HANDLER(spi_main_mrdy_rising_handler)
{
	irqreturn_t result = IRQ_HANDLED;

	return result;
}


/*=====================================
//Description	an interrupt handler for srdy rising
=====================================*/
SPI_DEV_IRQ_HANDLER(spi_main_srdy_rising_handler)
{
	irqreturn_t result = IRQ_HANDLED;

	if (!boot_done)
		return result;

	if (!wake_lock_active(&spi_wakelock)) {
		wake_lock(&spi_wakelock);
		SPI_OS_TRACE_MID(("[SPI] [%s](%d) spi_wakelock locked .\n",
			__func__, __LINE__));
	}

	if (send_modem_spi == 1)
		up(&srdy_sem); /* signal srdy event */

	/* SRDY interrupt work on SPI_MAIN_STATE_IDLE state for receive data */
	if (spi_main_state == SPI_MAIN_STATE_IDLE
		|| spi_main_state == SPI_MAIN_STATE_RX_TERMINATE
		|| spi_main_state == SPI_MAIN_STATE_TX_TERMINATE) {
		spi_main_send_signalfront(SPI_MAIN_MSG_RECEIVE);
		return result;
	}

	return result;
}


/*=====================================
* Description	an interrupt handler for submrdy rising
=====================================*/
SPI_DEV_IRQ_HANDLER(spi_main_submrdy_rising_handler)
{
	irqreturn_t result = IRQ_HANDLED;

	return result;
}


/*=====================================
* Description	an interrupt handler for subsrdy rising
=====================================*/
SPI_DEV_IRQ_HANDLER(spi_main_subsrdy_rising_handler)
{
	irqreturn_t result = IRQ_HANDLED;

	/* SRDY interrupt work on SPI_MAIN_STATE_TX_WAIT state for send data */
	if (spi_main_state == SPI_MAIN_STATE_TX_WAIT)
		return result;

	SPI_OS_TRACE_MID(("%s spi_main_state[%d]\n",
		"[SPI] spi_main_subsrdy_rising_handler :",
		(int)spi_main_state));

	return result;
}


/*=====================================
* Description	Send the signal to SPI Task
=====================================*/
void spi_main_send_signal(enum SPI_MAIN_MSG_T spi_sigs)
{
	struct spi_work *spi_wq = NULL;
	spi_wq = spi_os_malloc(sizeof(struct spi_work));
	spi_wq->signal_code = spi_sigs;
	INIT_WORK(&spi_wq->work, spi_main_process);
	queue_work(ipc_spi_wq, (struct work_struct *)spi_wq);
}


/*=====================================
* Description	Send the signal to SPI Task
=====================================*/
void spi_main_send_signalfront(enum SPI_MAIN_MSG_T spi_sigs)
{
	struct spi_work *spi_wq = NULL;
	spi_wq = spi_os_malloc(sizeof(struct spi_work));
	spi_wq->signal_code = spi_sigs;
	INIT_WORK(&spi_wq->work, spi_main_process);
	queue_work_front(ipc_spi_wq, (struct work_struct *)spi_wq);
}


/*=====================================
* Description	check each queue data and start send routine
=====================================*/
static int _start_data_send(void)
{
	if (spi_data_check_tx_queue() == 1)	{
		spi_main_send_signal(SPI_MAIN_MSG_SEND);
		return 1;
	}

	return 0;
}


/*=====================================
* Description	start to send packet data
=====================================*/
static void _start_packet_tx(void)
{
	int i = 0;

	if (spi_data_check_tx_queue() == 0)
		return;

	/* check SUB SRDY state */
	if (spi_dev_get_gpio(spi_dev_gpio_subsrdy) ==
		SPI_DEV_GPIOLEVEL_HIGH) {
		spi_main_send_signal(SPI_MAIN_MSG_SEND);
		return;
	}

	/* check SRDY state */
	if (spi_dev_get_gpio(spi_dev_gpio_srdy) ==
		SPI_DEV_GPIOLEVEL_HIGH) {
		spi_main_send_signal(SPI_MAIN_MSG_SEND);
		return;
	}

	if (get_console_suspended()) {
		spi_main_send_signal(SPI_MAIN_MSG_SEND);
		return;
	}

	if (spi_main_state == SPI_MAIN_STATE_END)
		return;

	/* change state SPI_MAIN_STATE_IDLE to SPI_MAIN_STATE_TX_WAIT */
	spi_main_state = SPI_MAIN_STATE_TX_WAIT;

#ifdef SPI_GUARANTEE_MRDY_GAP
	mrdy_high_time_save = spi_os_get_tick();
	if (check_mrdy_time_gap(mrdy_low_time_save, mrdy_high_time_save))
		spi_os_sleep(1);
#endif

	spi_dev_set_gpio(spi_dev_gpio_mrdy, SPI_DEV_GPIOLEVEL_HIGH);

	/* check SUBSRDY state */
	while (spi_dev_get_gpio(spi_dev_gpio_subsrdy) ==
		SPI_DEV_GPIOLEVEL_LOW) {
		if (i == 0) {
			spi_os_sleep(1);
			i++;
			continue;
		} else
			spi_os_sleep(MRDY_GUARANTEE_MRDY_TIME_SLEEP);

		i++;
		if (i > MRDY_GUARANTEE_MRDY_TIME_SLEEP * 20) {
			SPI_OS_ERROR(("%s spi_main_state=[%d]\n",
				"[SPI] === spi Fail to receiving SUBSRDY CONF :",
				(int)spi_main_state));

			spi_dev_set_gpio(spi_dev_gpio_mrdy,
				SPI_DEV_GPIOLEVEL_LOW);
#ifdef SPI_GUARANTEE_MRDY_GAP
			mrdy_low_time_save = spi_os_get_tick();
#endif
			/* change state SPI_MAIN_STATE_TX_WAIT */
			/* to SPI_MAIN_STATE_IDLE */
			if (spi_main_state != SPI_MAIN_STATE_START
				&& spi_main_state != SPI_MAIN_STATE_END
				&& spi_main_state != SPI_MAIN_STATE_INIT) {
				spi_main_state = SPI_MAIN_STATE_IDLE;
				_start_data_send();
			}
			return;
		}
	}
	if (spi_main_state != SPI_MAIN_STATE_START
		&& spi_main_state != SPI_MAIN_STATE_END
		&& spi_main_state != SPI_MAIN_STATE_INIT) {
		_start_packet_tx_send();
	} else
		SPI_OS_ERROR(("[SPI] ERR : _start_packet_tx:spi_main_state[%d]",
			(int)spi_main_state));

	return;
}


/*=====================================
* Description	send data
=====================================*/
static void _start_packet_tx_send(void)
{
	char *spi_packet_buf = NULL;

	/* change state SPI_MAIN_STATE_TX_WAIT to SPI_MAIN_STATE_TX_SENDING */
	spi_main_state = SPI_MAIN_STATE_TX_SENDING;

	spi_packet_buf = gspi_data_packet_buf;

	spi_os_memset(spi_packet_buf, 0, SPI_DEV_MAX_PACKET_SIZE);

	spi_data_prepare_tx_packet(spi_packet_buf);
	if (spi_dev_send((void *)spi_packet_buf,
		SPI_DEV_MAX_PACKET_SIZE) != 0) {
		/* TODO: save failed packet */
		/* back data to each queue */
		SPI_OS_ERROR(("[SPI] spi_dev_send fail\n"));

		spi_dev_set_gpio(spi_dev_gpio_submrdy, SPI_DEV_GPIOLEVEL_HIGH);

		while (spi_dev_get_gpio(spi_dev_gpio_subsrdy) ==
			SPI_DEV_GPIOLEVEL_LOW) {
			};

		spi_os_sleep(1);

		if (spi_dev_send((void *)spi_packet_buf,
			SPI_DEV_MAX_PACKET_SIZE) != 0) {
			panic("[SPI] spi_dev_send\n");
		} else {
			SPI_OS_ERROR(("[SPI] spi_dev_send\n"));
		}

		spi_dev_set_gpio(spi_dev_gpio_submrdy, SPI_DEV_GPIOLEVEL_LOW);
	}

	spi_main_state = SPI_MAIN_STATE_TX_TERMINATE;

	spi_dev_set_gpio(spi_dev_gpio_mrdy, SPI_DEV_GPIOLEVEL_LOW);
#ifdef SPI_GUARANTEE_MRDY_GAP
	mrdy_low_time_save = spi_os_get_tick();
#endif

	/* change state SPI_MAIN_STATE_TX_SENDING to SPI_MAIN_STATE_IDLE */
	spi_main_state = SPI_MAIN_STATE_IDLE;
	_start_data_send();
}


/*=====================================
* Description	start to receive packet data
=====================================*/
static void _start_packet_receive(void)
{
	char *spi_packet_buf = NULL;
	struct spi_data_packet_header	spi_receive_packet_header = {0, };
	int i = 0;

	if (!wake_lock_active(&spi_wakelock))
		return;

	if (spi_dev_get_gpio(spi_dev_gpio_srdy) == SPI_DEV_GPIOLEVEL_LOW)
		return;

	if (get_console_suspended())
		return;

	if (spi_main_state == SPI_MAIN_STATE_END)
		return;

	spi_main_state = SPI_MAIN_STATE_RX_WAIT;

	spi_dev_set_gpio(spi_dev_gpio_submrdy, SPI_DEV_GPIOLEVEL_HIGH);

	/* check SUBSRDY state */
	while (spi_dev_get_gpio(spi_dev_gpio_subsrdy) ==
		SPI_DEV_GPIOLEVEL_LOW) {
		if (i == 0) {
			spi_os_sleep(1);
			i++;
			continue;
		} else
		 spi_os_sleep(MRDY_GUARANTEE_MRDY_TIME_SLEEP);

		i++;
		if (i > MRDY_GUARANTEE_MRDY_TIME_SLEEP * 20) {
			SPI_OS_ERROR(("[SPI] ERROR(Failed MASTER RX:%d/%d)",
				i, MRDY_GUARANTEE_MRDY_TIME_SLEEP * 20));
			if (spi_main_state != SPI_MAIN_STATE_START
				&& spi_main_state != SPI_MAIN_STATE_END
				&& spi_main_state != SPI_MAIN_STATE_INIT) {
				spi_dev_set_gpio(spi_dev_gpio_submrdy,
					SPI_DEV_GPIOLEVEL_LOW);

				/* change state SPI_MAIN_STATE_RX_WAIT */
				/* to SPI_MAIN_STATE_IDLE */
				spi_main_state = SPI_MAIN_STATE_IDLE;
			}
			return;
		}
	}

	if (spi_main_state == SPI_MAIN_STATE_START
		|| spi_main_state == SPI_MAIN_STATE_END
		|| spi_main_state == SPI_MAIN_STATE_INIT)
		return;

	spi_packet_buf = gspi_data_packet_buf;

	spi_os_memset(spi_packet_buf, 0, SPI_DEV_MAX_PACKET_SIZE);

	if (spi_dev_receive((void *)spi_packet_buf,
		SPI_DEV_MAX_PACKET_SIZE) == 0) {
		/* parsing SPI packet */
		spi_os_memcpy(&spi_receive_packet_header, spi_packet_buf,
			SPI_DATA_PACKET_HEADER_SIZE);

		if (spi_data_parsing_rx_packet((void *)spi_packet_buf,
			spi_receive_packet_header.current_data_size) > 0) {
			/* call function for send data to IPC, RAW, RFS */
			spi_send_msg_to_app();
		}
	} else{
		SPI_OS_ERROR(("[SPI] spi_dev_receive fail\n"));
		panic("[SPI] spi_dev_receive\n");
	}

	spi_main_state = SPI_MAIN_STATE_RX_TERMINATE;

	spi_dev_set_gpio(spi_dev_gpio_submrdy, SPI_DEV_GPIOLEVEL_LOW);

	/* change state SPI_MAIN_STATE_RX_WAIT to SPI_MAIN_STATE_IDLE */
	spi_main_state = SPI_MAIN_STATE_IDLE;
	_start_data_send();

	return;
}


/*=====================================
* Description	creating a task
=====================================*/
void spi_main_init(void *data)
{
	struct ipc_spi_platform_data *pdata;

	SPI_OS_TRACE(("[SPI] spi_main_init\n"));

	spi_main_state = SPI_MAIN_STATE_START;


	pdata = (struct ipc_spi_platform_data *)data;

	wake_lock_init(&spi_wakelock, WAKE_LOCK_SUSPEND,
		"samsung-spiwakelock");

	spi_dev_init(pdata);

	spi_dev_reigster_irq_handler (spi_dev_gpio_srdy,
		SPI_DEV_IRQ_TRIGGER_RISING,
		spi_main_srdy_rising_handler,
		"spi_srdy_rising", 0);
	spi_dev_reigster_irq_handler (spi_dev_gpio_subsrdy,
		SPI_DEV_IRQ_TRIGGER_RISING,
		spi_main_subsrdy_rising_handler,
		"spi_subsrdy_rising", 0);
}


/*=====================================
* Description	main process
=====================================*/

static void spi_main_process(struct work_struct *work)
{
	int				signal_code;

	struct spi_work *spi_wq = container_of(work, struct spi_work, work);
	signal_code = spi_wq->signal_code;


	if (spi_main_state == SPI_MAIN_STATE_END
		|| spi_main_state == SPI_MAIN_STATE_START) {
		spi_os_free(spi_wq);
		return;
	}

	switch (signal_code) {
	case SPI_MAIN_MSG_SEND:
		if (spi_main_state == SPI_MAIN_STATE_IDLE)
			_start_packet_tx();

		break;

	case SPI_MAIN_MSG_PACKET_SEND:
		_start_packet_tx_send();
		break;

	case SPI_MAIN_MSG_RECEIVE:
		if (spi_main_state == SPI_MAIN_STATE_IDLE
			|| spi_main_state == SPI_MAIN_STATE_RX_TERMINATE
			|| spi_main_state == SPI_MAIN_STATE_TX_TERMINATE)
				_start_packet_receive();
			break;

	/* Receive data from IPC,RAW,RFS. */
	/* It need analyze and save to tx queue */
	case SPI_MAIN_MSG_IPC_SEND:
	case SPI_MAIN_MSG_RAW_SEND:
	case SPI_MAIN_MSG_RFS_SEND:
		/* start send data during SPI_MAIN_STATE_IDLE state */
		if (spi_main_state == SPI_MAIN_STATE_IDLE)
			_start_data_send();
		break;

	default:
		SPI_OS_ERROR(("[SPI] ERROR(No signal_code in spi_main[%d])\n",
			signal_code));
		break;
	}

	spi_os_free(spi_wq);
	if (wake_lock_active(&spi_wakelock)) {
		wake_unlock(&spi_wakelock);
		SPI_OS_TRACE_MID(("[SPI] [%s](%d) spi_wakelock unlocked .\n",
			__func__, __LINE__));
	}
}


/*=====================================
//Description	main task
=====================================*/
void spi_main(unsigned long argc, void *argv)
{
	/* queue init */
	spi_data_queue_init();

	SPI_OS_TRACE(("[SPI] spi_main %x\n", (unsigned int)argv));

	spi_dev_set_gpio(spi_dev_gpio_submrdy, SPI_DEV_GPIOLEVEL_HIGH);

	do {
		spi_os_sleep(30);
	} while (spi_dev_get_gpio(spi_dev_gpio_subsrdy) ==
		SPI_DEV_GPIOLEVEL_LOW);

	if (spi_is_restart)
		spi_os_sleep(100);
	spi_dev_set_gpio(spi_dev_gpio_submrdy, SPI_DEV_GPIOLEVEL_LOW);
	spi_main_state = SPI_MAIN_STATE_IDLE;
	spi_is_restart = 0;
}


/*=====================================
* Description	spi restart for CP slient reset
=====================================*/

void spi_set_restart(void)
{
	SPI_OS_ERROR(("[SPI] spi_set_restart(spi_main_state=[%d])\n",
		(int)spi_main_state));

	spi_dev_set_gpio(spi_dev_gpio_submrdy, SPI_DEV_GPIOLEVEL_LOW);

	spi_main_state = SPI_MAIN_STATE_END;

	spi_is_restart = 1;

	flush_workqueue(ipc_spi_wq);

	spi_data_queue_destroy();

	spi_dev_destroy();

	if (wake_lock_active(&spi_wakelock)) {
		wake_unlock(&spi_wakelock);
		SPI_OS_TRACE_MID(("[SPI] [%s](%d) spi_wakelock unlocked.\n",
			__func__, __LINE__));
	}
	wake_lock_destroy(&spi_wakelock);
}
