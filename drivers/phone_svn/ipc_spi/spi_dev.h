#ifndef _SPI_DEV_H_
#define _SPI_DEV_H_

#include "spi_main.h"

#include <linux/interrupt.h>

#define SPI_DEV_MAX_PACKET_SIZE (2048 * 6)

enum SPI_DEV_SYNC_STATE_T {
	SPI_DEV_SYNC_OFF = 0,
	SPI_DEV_SYNC_ON
};

enum SPI_DEV_GPIOLEVEL_T {
	SPI_DEV_GPIOLEVEL_LOW	= 0,
	SPI_DEV_GPIOLEVEL_HIGH
};

enum SPI_DEV_IRQ_TRIGGER_T {
	SPI_DEV_IRQ_TRIGGER_RISING	= 1,
	SPI_DEV_IRQ_TRIGGER_FALLING
};

typedef irqreturn_t (*SPI_DEV_IRQ_HANDLER_T)(int, void *);
#define SPI_DEV_IRQ_HANDLER(X) irqreturn_t X(int irq, void *data)

extern int spi_dev_gpio_mrdy;
extern int spi_dev_gpio_srdy;
extern int spi_dev_gpio_submrdy;
extern int spi_dev_gpio_subsrdy;

extern int spi_is_restart;

extern void spi_dev_init(void *data);
extern void spi_dev_destroy(void);
extern int spi_dev_send(void *buf, unsigned int length);
extern int spi_dev_receive(void *buf, unsigned int length);
extern void spi_dev_set_gpio(int gpio_id, enum SPI_DEV_GPIOLEVEL_T value);
extern enum SPI_DEV_GPIOLEVEL_T spi_dev_get_gpio(int gpio_id);
extern int spi_dev_reigster_irq_handler(int gpio_id,
	enum SPI_DEV_IRQ_TRIGGER_T trigger,
	SPI_DEV_IRQ_HANDLER_T handler,
	char *name, void *data);
extern void spi_dev_unreigster_irq_handler(int gpio_id, void *data);

extern int ipc_spi_tx_rx_sync(u8 *tx_d, u8 *rx_d, unsigned len);

#endif
