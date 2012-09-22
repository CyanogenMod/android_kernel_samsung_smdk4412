/**************************************************************

	spi_dev.c

	adapt device api and spi api

	This is MASTER side.

***************************************************************/



/**************************************************************

	Preprocessor by common

***************************************************************/

#include "spi_main.h"
#include "spi_dev.h"
#include "spi_os.h"



/**************************************************************

	Preprocessor by platform
	(OMAP4430 && MSM7X30)

***************************************************************/

#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/phone_svn/ipc_spi.h>



/**************************************************************

	Definition of Variables and Functions by common

***************************************************************/

int spi_dev_gpio_mrdy;
int spi_dev_gpio_srdy;
int spi_dev_gpio_submrdy;
int spi_dev_gpio_subsrdy;



/**************************************************************

	Preprocessor by platform
	(OMAP4430 && MSM7X30)

***************************************************************/

int spi_is_restart;



/**********************************************************

Prototype		void spi_dev_init ( void * data)

Type		function

Description	init spi gpio info

Param input	(none)

Return value	(none)

***********************************************************/
void spi_dev_init(void *data)
{
	struct ipc_spi_platform_data *pdata = NULL;


	SPI_OS_TRACE(("[SPI] spi_dev_init\n"));


	pdata = (struct ipc_spi_platform_data *)data;

	spi_dev_gpio_mrdy = (int) pdata->gpio_ipc_mrdy;
	spi_dev_gpio_srdy = (int) pdata->gpio_ipc_srdy;
	spi_dev_gpio_submrdy = (int) pdata->gpio_ipc_sub_mrdy;
	spi_dev_gpio_subsrdy = (int) pdata->gpio_ipc_sub_srdy;

	spi_dev_set_gpio(spi_dev_gpio_mrdy, SPI_DEV_GPIOLEVEL_LOW);
	if (spi_is_restart == 0)
		spi_dev_set_gpio(spi_dev_gpio_submrdy, SPI_DEV_GPIOLEVEL_LOW);
}


/**********************************************************

Prototype		void spi_dev_destroy( void )

Type		function

Description	unregister irq handler

Param input	(none)

Return value	(none)

***********************************************************/

void spi_dev_destroy(void)
{
	spi_dev_unreigster_irq_handler(spi_dev_gpio_srdy, 0);
	spi_dev_unreigster_irq_handler(spi_dev_gpio_subsrdy, 0);
}



/**********************************************************

Prototype		int spi_dev_send

Type		function

Description	starting data send by DMA(CP side)
			starting send clock for data switching(AP side)

Param input	buf : data for send
			length : data size. this lengt must be fixed

Return value	0 : success
			(others) : error cause

***********************************************************/

int spi_dev_send(void *buf, unsigned int length)
{
	int result = 0;

	result = ipc_spi_tx_rx_sync(buf, 0, length);

	return result;
}


/**********************************************************

Prototype		int spi_dev_receive

Type		function

Description	starting data receive by DMA(CP side)
			starting send clock for data switching(AP side)

Param input	buf : buffer for saving data
			length : data size. this lengt must be fixed

Return value	0 : success
			(others) : error cause

***********************************************************/

int spi_dev_receive(void *buf, unsigned int length)
{
	int value = 0;

	value = ipc_spi_tx_rx_sync(0, buf, length);

	return value;
}


/**********************************************************

Prototype		void spi_dev_set_gpio

Type		function

Description	set gpio pin state

Param input	gpio_id : number of gpio id
			value : SPI_DEV_GPIOLEVEL_HIGH for raising pin state up
			: SPI_DEV_GPIOLEVEL_LOW for getting pin state down

Return value	(none)

***********************************************************/
void spi_dev_set_gpio(int gpio_id, enum SPI_DEV_GPIOLEVEL_T value)
{
	int level = 0;

	SPI_OS_TRACE_MID(("%s gpio_id =[%d], value =[%d]\n",
		"[SPI] spi_dev_set_gpio :", gpio_id, (int) value));

	switch (value) {
	case SPI_DEV_GPIOLEVEL_LOW:
		level = 0;
		break;
	case SPI_DEV_GPIOLEVEL_HIGH:
		level = 1;
		break;
	}

	gpio_set_value((unsigned int) gpio_id, level);
}



/**********************************************************

Prototype		SPI_DEV_GPIOLEVEL_T spi_dev_get_gpio(int gpio_id)

Type		function

Description	get gpio pin state

Param input	gpio_id : number of gpio id

Return value	SPI_DEV_GPIOLEVEL

***********************************************************/
enum SPI_DEV_GPIOLEVEL_T spi_dev_get_gpio(int gpio_id)
{
	enum SPI_DEV_GPIOLEVEL_T value = SPI_DEV_GPIOLEVEL_LOW;
	int level = 0;

	level = gpio_get_value((unsigned int) gpio_id);

	switch (level) {
	case 0:
		value = SPI_DEV_GPIOLEVEL_LOW;
		break;
	case 1:
		value = SPI_DEV_GPIOLEVEL_HIGH;
		break;
	}

	return value;
}


/**********************************************************

Prototype		int spi_dev_reigster_irq_handler

Type		function

Description	regist irq callback function to each gpio interrupt

Param input	gpio_id	: gpio pin id
			tigger	: interrupt detection mode
			handler	: interrupt handler function
			name : register name

Return value	0	: fail
			1	: success

***********************************************************/
int spi_dev_reigster_irq_handler(int gpio_id,
	enum SPI_DEV_IRQ_TRIGGER_T trigger,
	SPI_DEV_IRQ_HANDLER_T handler,
	char *name, void *data)
{
#if defined(SPI_FEATURE_OMAP4430)
	int value = 0;
	int _trigger = IRQF_TRIGGER_NONE;

	switch (trigger) {
	case SPI_DEV_IRQ_TRIGGER_RISING:
		_trigger = IRQF_TRIGGER_RISING;
		break;
	case SPI_DEV_IRQ_TRIGGER_FALLING:
		_trigger = IRQF_TRIGGER_FALLING;
		break;
	default:
		_trigger = IRQF_TRIGGER_NONE;
		break;
	}

	value = request_irq(OMAP_GPIO_IRQ(gpio_id), handler,
		_trigger, name, data);

	if (value != 0) {
		SPI_OS_ERROR(("%s regist fail(%d)",
			"[SPI] ERROR : spi_dev_reigster_irq_handler :",
			value));
		return 0;
	}

#elif defined(SPI_FEATURE_S5PC210)
	int value = 0;

	int _trigger = IRQF_TRIGGER_NONE;
	int irq = 0;


	switch (trigger) {
	case SPI_DEV_IRQ_TRIGGER_RISING:
		_trigger = IRQF_TRIGGER_RISING;
		break;
	case SPI_DEV_IRQ_TRIGGER_FALLING:
		_trigger = IRQF_TRIGGER_FALLING;
		break;
	default:
		_trigger = IRQF_TRIGGER_NONE;
		break;
	}

	irq = gpio_to_irq(gpio_id);
	value = request_irq(irq, handler, _trigger, name, data);
	if (value != 0) {
		SPI_OS_ERROR(("spi_dev_reigster_irq_handler: regist fail(%d)",
			value));
		return 0;
	}
	enable_irq_wake(irq);
#endif

	return 1;
}


/**********************************************************

void spi_dev_unreigster_irq_handler (int gpio_id, void * data)

Type		function

Description	unregister irq hanger by gpio api

Param input	gpio_id	: gpio pin id
			data	: param

Return value	(none)

***********************************************************/

void spi_dev_unreigster_irq_handler(int gpio_id, void *data)
{
#if defined(SPI_FEATURE_OMAP4430)
	free_irq(OMAP_GPIO_IRQ(gpio_id), data);
#elif defined(SPI_FEATURE_S5PC210)
	free_irq(gpio_to_irq(gpio_id), data);
#endif
}
