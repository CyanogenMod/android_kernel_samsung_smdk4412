#include "spi_main.h"
#include "spi_test.h"
#include "spi_os.h"
#include "spi_dev.h"
#include "spi_app.h"

enum SPI_TEST_SCENARIO_T spi_test_scenario_mode = SPI_TEST_SCENARIO_NONE;

static void _scenario_physical(void);
static void _scenario_sending(int param);


static void _scenario_physical(void)
{
	int count = 0;
	int failcount = 0;
	int i;
	char txbuf[SPI_DEV_MAX_PACKET_SIZE];
	char rxbuf[SPI_DEV_MAX_PACKET_SIZE];

	SPI_OS_TRACE_MID(("spi_scenario_physical\n"));

	spi_os_memset(txbuf, 0x00, sizeof(txbuf));
	for (i = 0 ; i < SPI_DEV_MAX_PACKET_SIZE ; i++)
		txbuf[i] = i%100;

	while (count < 100) {
		SPI_OS_TRACE_MID(("spi _scenario_physical %d\n", count));
		SPI_OS_TRACE_MID(("%s mrdy %d srdy %d submrdy %d subsrdy %d\n",
			"spi_scenario_physical test start",
			spi_dev_get_gpio(spi_dev_gpio_mrdy),
			spi_dev_get_gpio(spi_dev_gpio_srdy),
			spi_dev_get_gpio(spi_dev_gpio_submrdy),
			spi_dev_get_gpio(spi_dev_gpio_subsrdy)));

		txbuf[0] = count;
		spi_os_memset(rxbuf, 0x00, sizeof(rxbuf));

#ifdef SPI_FEATURE_MASTER
		spi_dev_set_gpio(spi_dev_gpio_mrdy, SPI_DEV_GPIOLEVEL_LOW);
		spi_os_sleep(SPI_FEATURE_TEST_DURATION);
		spi_dev_set_gpio(spi_dev_gpio_submrdy, SPI_DEV_GPIOLEVEL_LOW);
		spi_os_sleep(SPI_FEATURE_TEST_DURATION);

		SPI_OS_TRACE_MID(("%s, subsrdy high\n",
			"spi_scenario_physical test wait srdy"));
		spi_dev_set_gpio(spi_dev_gpio_mrdy, SPI_DEV_GPIOLEVEL_HIGH);

		while (spi_dev_get_gpio(spi_dev_gpio_srdy) ==
			SPI_DEV_GPIOLEVEL_LOW)
			;

		/* spreadtrum recommend. */
		/* master should send/receive after that slave is ready. */
		spi_os_sleep(20);

		if (count % 2 == 0) {
			spi_dev_send(txbuf, SPI_DEV_MAX_PACKET_SIZE);
		} else {
			spi_dev_receive(rxbuf, SPI_DEV_MAX_PACKET_SIZE);
			for (i = 1 ; i < SPI_DEV_MAX_PACKET_SIZE ; i++) {
				if (rxbuf[i] != txbuf[i]) {
					failcount++;
		spi_os_trace_dump("spi_scenario_physical receiving fail",
						&rxbuf[i-8], 16);
					SPI_OS_TRACE_MID(("%s %d count %d/%d\n",
				"spi_scenario_physical test receiving fail",
						i, failcount, count));
					i = sizeof(rxbuf);
					break;
				}
			}
		}

		spi_os_sleep(20);
		spi_dev_set_gpio(spi_dev_gpio_submrdy, SPI_DEV_GPIOLEVEL_HIGH);

		spi_os_sleep(SPI_FEATURE_TEST_DURATION);

#elif defined SPI_FEATURE_SLAVE
		spi_dev_set_gpio(spi_dev_gpio_srdy, SPI_DEV_GPIOLEVEL_LOW);
		spi_os_sleep(SPI_FEATURE_TEST_DURATION);
		spi_dev_set_gpio(spi_dev_gpio_subsrdy, SPI_DEV_GPIOLEVEL_LOW);
		spi_os_sleep(SPI_FEATURE_TEST_DURATION);

		while (spi_dev_get_gpio(spi_dev_gpio_mrdy) ==
			SPI_DEV_GPIOLEVEL_LOW)
			;

		spi_dev_set_gpio(spi_dev_gpio_srdy, SPI_DEV_GPIOLEVEL_HIGH);

		if (count % 2 == 0) {
			spi_dev_receive(rxbuf, SPI_DEV_MAX_PACKET_SIZE);
			for (i = 1 ; i < SPI_DEV_MAX_PACKET_SIZE ; i++) {
				if (rxbuf[i] != txbuf[i]) {
					failcount++;
			spi_os_trace_dump("spi_scenario_phy rx fail",
						&rxbuf[i-8], 16);
					SPI_OS_TRACE_MID(("%s %d count %d/%d\n",
						"spi_scenario_physical test receiving fail",
						i, failcount, count));
					i = sizeof(rxbuf);
					break;
				}
			}
		} else
			spi_dev_send(txbuf, SPI_DEV_MAX_PACKET_SIZE);

		spi_os_sleep(SPI_FEATURE_TEST_DURATION);

#endif
		count++;
		SPI_OS_TRACE_MID(("%s %d/%d\n",
			"spi_scenario_physical test receiving result count",
			failcount, count));
	}
}


SPI_OS_TIMER_CALLBACK(spi_test_timer_callback)
{
	SPI_OS_TRACE_MID(("spi_test_timer_callback\n"));
	_scenario_sending(0);
}

static char tempdata1[135]; /* temp code. becuase can not allocate */
static char tempdata2[367]; /* temp code. becuase can not allocate */
static char tempdata3[1057]; /* temp code. becuase can not allocate */
static char tempdata4[35]; /* temp code. becuase can not allocate */
static char tempdata5[2079]; /* temp code. becuase can not allocate */
static char tempdata6[200]; /* temp code. becuase can not allocate */
static char tempdata7[2052]; /* temp code. becuase can not allocate */

static void _scenario_sending(int param)
{
	#define NB_COUNT 50
	#define NB_STEP 7
	static int step;
	static int duration;
	static int count;
	static void *timer_id;

	char *data = NULL;
	int i, value;
	struct DATABYSTEP_T {
		SPI_MAIN_MSG_T type;
		unsigned int size;
		char *buf;
	} databystep[NB_STEP] = {
				{SPI_MAIN_MSG_IPC_SEND, 135, tempdata1},
				{SPI_MAIN_MSG_IPC_SEND, 187, tempdata2},
				{SPI_MAIN_MSG_RAW_SEND, 1057, tempdata3},
				{SPI_MAIN_MSG_IPC_SEND, 35, tempdata4},
				{SPI_MAIN_MSG_RAW_SEND, 2079, tempdata5},
				{SPI_MAIN_MSG_IPC_SEND, 100, tempdata6},
				{SPI_MAIN_MSG_RAW_SEND, 2052, tempdata7}
				};

	if (spi_test_scenario_mode == SPI_TEST_SCENARIO_SLAVE_SENDING)
		data = databystep[step].buf;

	/* param is 0 to fix duration. */
	/* call this function with param to change timer duration. */
	if (param != 0)
		duration = param;

	if (spi_test_scenario_mode != SPI_TEST_SCENARIO_SLAVE_SENDING)
		data = (char *)spi_os_malloc(databystep[step].size);

	spi_os_memset(data, 0x00, databystep[step].size);

	SPI_OS_TRACE_MID(("spi _scenario_sending step %d\n", step));

	/* generate data to send, It is serial number from 0 to 99 */
	for (i = 0 ; i < databystep[step].size ; i++)
		data[i] = i%100;

	do {
#ifdef SPI_FEATURE_OMAP4430
		if (spi_is_ready() != 0) {
			struct spi_os_msg *msg;
			SPI_MAIN_MSG_T signal_code;
			signal_code = databystep[step].type;

			msg = (struct spi_os_msg *)
				spi_os_malloc(sizeof(struct spi_os_msg));
			msg->signal_code = signal_code;
			msg->data_length = databystep[step].size;
			msg->data = data;

			spi_receive_msg_from_app(msg);
			spi_main_send_signal(signal_code);
			break;
		} else
			spi_os_sleep(50);
#elif defined SPI_FEATURE_SC8800G
		if (app_send_data_to_spi(databystep[step].type,
				data, databystep[step].size) == 0)
			/* send fail */
			spi_os_sleep(50);
		else
			/* send success */
			break;
#endif
	} while (1);

	if (spi_test_scenario_mode == SPI_TEST_SCENARIO_MASTER_SENDING)
		return;

	step++;
	count++;
	step %= NB_STEP;

	if (timer_id == 0)
		timer_id = spi_os_create_timer("spi_test_timer",
			spi_test_timer_callback, step, duration);

	if (timer_id == 0) {
		SPI_OS_TRACE_MID(("spi _scenario_sending invalid timer id\n"));
		return;
	}

	SPI_OS_TRACE_MID(("spi _scenario_sending timer %x\n", timer_id));
	SPI_OS_TRACE_MID(("spi _scenario_sending start timer count %d\n",
		count));

	if (count == NB_COUNT) {
		spi_os_stop_timer(timer_id);
		return;
	}

	value = spi_os_start_timer(timer_id, spi_test_timer_callback,
		step, duration);
	SPI_OS_TRACE_MID(("spi _scenario_sending start timer%d\n", value));

	#undef NB_STEP
}


void spi_test_run(enum SPI_TEST_SCENARIO_T scenario, int param)
{
	SPI_OS_TRACE_MID(("spi_test_run %d\n", (int) scenario));

	if (scenario == SPI_TEST_SCENARIO_NONE)
			return;

	spi_test_scenario_mode = scenario;

	switch ((int)scenario) {
	case SPI_TEST_SCENARIO_PHYSICAL:
		_scenario_physical();
		break;
	case SPI_TEST_SCENARIO_MASTER_SENDING:
#ifdef SPI_FEATURE_MASTER
		_scenario_sending(param);
#endif
		break;
	case SPI_TEST_SCENARIO_SLAVE_SENDING:
#ifdef SPI_FEATURE_SLAVE
		_scenario_sending(param);
#endif
		break;
	case SPI_TEST_SCENARIO_COMPLEX_SENDING:
		_scenario_sending(param);
		break;
	case SPI_TEST_SCENARIO_NONE:
	default:
		break;
	}
}
