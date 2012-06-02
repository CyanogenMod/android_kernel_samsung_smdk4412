#ifndef _SPI_APP_H_
#define _SPI_APP_H_

#include "spi_main.h"
#include "spi_data.h"
#include "spi_os.h"

extern void spi_receive_msg_from_app(struct spi_os_msg *msg);
extern void spi_send_msg_to_app(void);
extern int   spi_is_ready(void);

extern struct ipc_spi *ipc_spi;
extern void ipc_spi_make_data_interrupt(u32 cmd, struct ipc_spi *od);

#endif
