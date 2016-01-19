/*
 * Driver core for Samsung SoC onboard UARTs.
 *
 * Ben Dooks, Copyright (c) 2003-2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

/* Hote on 2410 error handling
 *
 * The s3c2410 manual has a love/hate affair with the contents of the
 * UERSTAT register in the UART blocks, and keeps marking some of the
 * error bits as reserved. Having checked with the s3c2410x01,
 * it copes with BREAKs properly, so I am happy to ignore the RESERVED
 * feature from the latter versions of the manual.
 *
 * If it becomes aparrent that latter versions of the 2410 remove these
 * bits, then action will have to be taken to differentiate the versions
 * and change the policy on BREAK
 *
 * BJD, 04-Nov-2004
*/

#if defined(CONFIG_SERIAL_SAMSUNG_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/sysrq.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>

#include <asm/irq.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-serial.h>

#include "samsung.h"

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
/* UART name and device definitions */

#define S3C24XX_SERIAL_NAME	"ttySAC"
#define S3C24XX_SERIAL_MAJOR	204
#define S3C24XX_SERIAL_MINOR	64

/* macros to change one thing to another */

#define tx_enabled(port) ((port)->unused[0])
#define rx_enabled(port) ((port)->unused[1])

/* flag to ignore all characters coming in */
#define RXSTAT_DUMMY_READ (0x10000000)

#if defined(CONFIG_MACH_U1) || defined(CONFIG_MACH_MIDAS) || \
	defined(CONFIG_MACH_SLP_PQ) || defined(CONFIG_MACH_P10) || \
	defined(CONFIG_MACH_PX) || defined(CONFIG_MACH_TRATS) || defined(CONFIG_GPS_BCMxxxxx)
/* Devices	*/
#define CONFIG_BT_S3C_UART	0
#define CONFIG_GPS_S3C_UART	1
#endif

#ifdef CONFIG_SERIAL_SAMSUNG_CONSOLE_SWITCH
static unsigned uart_debug; /* Initialized automatically with 0 by compiler */
module_param_named(uart_debug, uart_debug, uint, 0644);
#endif

static inline struct s3c24xx_uart_port *to_ourport(struct uart_port *port)
{
	return container_of(port, struct s3c24xx_uart_port, port);
}

/* translate a port to the device name */

static inline const char *s3c24xx_serial_portname(struct uart_port *port)
{
	return to_platform_device(port->dev)->name;
}

static int s3c24xx_serial_txempty_nofifo(struct uart_port *port)
{
	return (rd_regl(port, S3C2410_UTRSTAT) & S3C2410_UTRSTAT_TXE);
}

static void s3c24xx_serial_rx_enable(struct uart_port *port)
{
	unsigned long flags;
	unsigned int ucon, ufcon;
	int count = 10000;

	spin_lock_irqsave(&port->lock, flags);

	while (--count && !s3c24xx_serial_txempty_nofifo(port))
		udelay(100);

	ufcon = rd_regl(port, S3C2410_UFCON);
	ufcon |= S3C2410_UFCON_RESETRX;
	wr_regl(port, S3C2410_UFCON, ufcon);

	ucon = rd_regl(port, S3C2410_UCON);
	ucon |= S3C2410_UCON_RXIRQMODE;
	wr_regl(port, S3C2410_UCON, ucon);

	rx_enabled(port) = 1;
	spin_unlock_irqrestore(&port->lock, flags);
}

static void s3c24xx_serial_rx_disable(struct uart_port *port)
{
	unsigned long flags;
	unsigned int ucon;

	spin_lock_irqsave(&port->lock, flags);

	ucon = rd_regl(port, S3C2410_UCON);
	ucon &= ~S3C2410_UCON_RXIRQMODE;
	wr_regl(port, S3C2410_UCON, ucon);

	rx_enabled(port) = 0;
	spin_unlock_irqrestore(&port->lock, flags);
}

static void s3c24xx_serial_stop_tx(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (tx_enabled(port)) {
		disable_irq_nosync(ourport->tx_irq);
		tx_enabled(port) = 0;
		if (port->flags & UPF_CONS_FLOW)
			s3c24xx_serial_rx_enable(port);
	}
}

static void s3c24xx_serial_start_tx(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (!tx_enabled(port)) {
		if (port->flags & UPF_CONS_FLOW)
			s3c24xx_serial_rx_disable(port);

		enable_irq(ourport->tx_irq);
		tx_enabled(port) = 1;
	}
}


static void s3c24xx_serial_stop_rx(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (rx_enabled(port)) {
		dbg("s3c24xx_serial_stop_rx: port=%p\n", port);
		disable_irq_nosync(ourport->rx_irq);
		rx_enabled(port) = 0;
	}
}

static void s3c24xx_serial_enable_ms(struct uart_port *port)
{
}

static inline struct s3c24xx_uart_info *s3c24xx_port_to_info(struct uart_port *port)
{
	return to_ourport(port)->info;
}

static inline struct s3c2410_uartcfg *s3c24xx_port_to_cfg(struct uart_port *port)
{
	if (port->dev == NULL)
		return NULL;

	return (struct s3c2410_uartcfg *)port->dev->platform_data;
}

static int s3c24xx_serial_rx_fifocnt(struct s3c24xx_uart_port *ourport,
				     unsigned long ufstat)
{
	struct s3c24xx_uart_info *info = ourport->info;

	if (ufstat & info->rx_fifofull)
		return info->fifosize;

	return (ufstat & info->rx_fifomask) >> info->rx_fifoshift;
}
#ifdef CONFIG_CSR_GSD4T_CDMA
#define CSR_GPS_WORK_AROUND_RX_ISR           /*it should be applied to*/
#define CSR_GPS_WORK_AROUND_UART_DRIVER      /*it should be applied to*/
/*#define CSR_GPS_DEBUG */                 /*  just debug code*/
/*#define CSR_GPS_DEBUG_RAW_DATA */ /*just debug code*/
#endif

#ifdef CSR_GPS_DEBUG
#define GPIO_GPS_DEBUG_NEW EXYNOS4_GPK1(2)
#endif

/* ? - where has parity gone?? */
#define S3C2410_UERSTAT_PARITY (0x1000)

static irqreturn_t
s3c24xx_serial_rx_chars(int irq, void *dev_id)
{
	struct s3c24xx_uart_port *ourport = dev_id;
	struct uart_port *port = &ourport->port;
	struct tty_struct *tty = port->state->port.tty;
	unsigned int ufcon, ch, flag, ufstat, uerstat;
#ifdef CSR_GPS_WORK_AROUND_RX_ISR
	int max_count = 256;
	unsigned char received_data[256];
	unsigned char received_flag[256];
	int received_data_count = 0;
#else
	int max_count = 64;
#endif
#ifdef CSR_GPS_DEBUG
	int overrun = 0;
	static int gpio_init = 0;
	static int active_signal = 1;
#endif
#ifdef CSR_GPS_DEBUG_RAW_DATA
	char received_data_string[256*2+1];
	int i;
#endif
#ifdef CSR_GPS_DEBUG
	if (gpio_init == 0) {
		s3c_gpio_cfgpin(GPIO_GPS_DEBUG_NEW, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_GPS_DEBUG_NEW, S3C_GPIO_PULL_NONE);
		gpio_init = 1;
	}

	gpio_set_value(GPIO_GPS_DEBUG_NEW, active_signal);
#endif

	while (max_count-- > 0) {
		ufcon = rd_regl(port, S3C2410_UFCON);
		ufstat = rd_regl(port, S3C2410_UFSTAT);

		if (s3c24xx_serial_rx_fifocnt(ourport, ufstat) == 0)
			break;

		uerstat = rd_regl(port, S3C2410_UERSTAT);
		ch = rd_regb(port, S3C2410_URXH);

		if (port->flags & UPF_CONS_FLOW) {
			int txe = s3c24xx_serial_txempty_nofifo(port);

			if (rx_enabled(port)) {
				if (!txe) {
					rx_enabled(port) = 0;
					continue;
				}
			} else {
				if (txe) {
					ufcon |= S3C2410_UFCON_RESETRX;
					wr_regl(port, S3C2410_UFCON, ufcon);
					rx_enabled(port) = 1;
					goto out;
				}
				continue;
			}
		}

		/* insert the character into the buffer */

		flag = TTY_NORMAL;
		port->icount.rx++;

		if (unlikely(uerstat & S3C2410_UERSTAT_ANY)) {
			dbg("rxerr: port ch=0x%02x, rxs=0x%08x\n",
			    ch, uerstat);

			/* check for break */
			if (uerstat & S3C2410_UERSTAT_BREAK) {
				dbg("break!\n");
#ifdef CSR_GPS_WORK_AROUND_RX_ISR
				printk(KERN_ERR KERN_DEBUG "break!\n");
#endif
				port->icount.brk++;
				if (uart_handle_break(port))
				    goto ignore_char;
			}

			if (uerstat & S3C2410_UERSTAT_FRAME) {
#ifdef CSR_GPS_WORK_AROUND_RX_ISR
				printk(KERN_ERR KERN_DEBUG "frame error!\n");
#endif
				port->icount.frame++;
			}
			if (uerstat & S3C2410_UERSTAT_OVERRUN) {
#ifdef CSR_GPS_WORK_AROUND_RX_ISR
				printk(KERN_ERR KERN_DEBUG "overrun error!\n");
#endif
				port->icount.overrun++;
#ifdef CSR_GPS_DEBUG
				if (overrun == 0) {
					overrun = 1;
					active_signal = !active_signal;
				}
#endif
			}
			uerstat &= port->read_status_mask;

			if (uerstat & S3C2410_UERSTAT_BREAK)
				flag = TTY_BREAK;
			else if (uerstat & S3C2410_UERSTAT_PARITY)
				flag = TTY_PARITY;
			else if (uerstat & (S3C2410_UERSTAT_FRAME |
					    S3C2410_UERSTAT_OVERRUN))
				flag = TTY_FRAME;
		}

		if (uart_handle_sysrq_char(port, ch)) {
			printk(KERN_ERR KERN_DEBUG "break-sysrq\n");
			goto ignore_char;
		}

#ifdef CSR_GPS_WORK_AROUND_RX_ISR
		received_data[received_data_count] = ch;
		received_flag[received_data_count] = flag;
		received_data_count++;
#else
		uart_insert_char(port, uerstat, S3C2410_UERSTAT_OVERRUN,
				 ch, flag);
#endif

 ignore_char:
		continue;
	}
#ifdef CSR_GPS_WORK_AROUND_RX_ISR
	if (received_data_count) {
#ifdef CSR_GPS_DEBUG_RAW_DATA
		if (tty->index == 1) {
			int nibble;
			unsigned char *ptr_src, *ptr_dst;

			ptr_src = received_data;
			ptr_dst = received_data_string;

			for (i = 0; i < received_data_count; i++, ptr_src++) {
				nibble = (*ptr_src >> 4) & 0x0F;
				if (9 < nibble)
					*ptr_dst = nibble - 10 + 'A';
				else
					*ptr_dst = nibble + '0';

				ptr_dst++;

				nibble = (*ptr_src) & 0x0F;

				if (9 < nibble)
					*ptr_dst = nibble - 10 + 'A';
				else
					*ptr_dst = nibble + '0';

				ptr_dst++;
			}

			*ptr_dst = '\0';
			printk(KERN_ERR KERN_DEBUG "CGPS:%s\n", \
				received_data_string);
		}
#endif	/* CSR_GPS_DEBUG_RAW_DATA */
		tty_insert_flip_string_flags(tty, \
		(const unsigned char *)received_data, \
		(const char *)received_flag, \
		(size_t) received_data_count); /* yr check */
	tty_flip_buffer_push(tty);
	}
#else	/* !CSR_GPS_WORK_AROUND_RX_ISR */
	tty_flip_buffer_push(tty);
#endif	/* CSR_GPS_WORK_AROUND_RX_ISR */

 out:
#ifdef CSR_GPS_DEBUG
	/*oif overrun, gpio toggle*/
	gpio_set_value(GPIO_GPS_DEBUG_NEW, !active_signal);
#endif
	return IRQ_HANDLED;
}

static irqreturn_t s3c24xx_serial_tx_chars(int irq, void *id)
{
	struct s3c24xx_uart_port *ourport = id;
	struct uart_port *port = &ourport->port;
	struct circ_buf *xmit = &port->state->xmit;
	int count = 256;

	if (port->x_char) {
		wr_regb(port, S3C2410_UTXH, port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		goto out;
	}

	/* if there isn't anything more to transmit, or the uart is now
	 * stopped, disable the uart and exit
	*/

	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		s3c24xx_serial_stop_tx(port);
		goto out;
	}

	/* try and drain the buffer... */

	while (!uart_circ_empty(xmit) && count-- > 0) {
		if (rd_regl(port, S3C2410_UFSTAT) & ourport->info->tx_fifofull)
			break;

		wr_regb(port, S3C2410_UTXH, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit))
		s3c24xx_serial_stop_tx(port);

 out:
	return IRQ_HANDLED;
}

static unsigned int s3c24xx_serial_tx_empty(struct uart_port *port)
{
	struct s3c24xx_uart_info *info = s3c24xx_port_to_info(port);
	unsigned long ufstat = rd_regl(port, S3C2410_UFSTAT);
	unsigned long ufcon = rd_regl(port, S3C2410_UFCON);

	if (ufcon & S3C2410_UFCON_FIFOMODE) {
		if ((ufstat & info->tx_fifomask) != 0 ||
		    (ufstat & info->tx_fifofull))
			return 0;

		return 1;
	}

	return s3c24xx_serial_txempty_nofifo(port);
}

/* no modem control lines */
static unsigned int s3c24xx_serial_get_mctrl(struct uart_port *port)
{
	unsigned int umstat = rd_regb(port, S3C2410_UMSTAT);

	if (umstat & S3C2410_UMSTAT_CTS)
		return TIOCM_CAR | TIOCM_DSR | TIOCM_CTS;
	else
		return TIOCM_CAR | TIOCM_DSR;
}

static void s3c24xx_serial_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	/* todo - possibly remove AFC and do manual CTS */

#if defined(CONFIG_MACH_U1) || defined(CONFIG_MACH_MIDAS) || \
	defined(CONFIG_MACH_SLP_PQ) || defined(CONFIG_MACH_P10) || \
	defined(CONFIG_MACH_PX) || defined(CONFIG_MACH_TRATS) || defined(CONFIG_GPS_BCMxxxxx)
	unsigned int umcon = 0;
	umcon = rd_regl(port, S3C2410_UMCON);

	if (port->line == CONFIG_BT_S3C_UART) {
		if (mctrl & TIOCM_RTS) {
			umcon |= S3C2410_UMCOM_AFC;
		} else {
			umcon &= ~S3C2410_UMCOM_AFC;
		}
	}
#if !defined(CONFIG_MACH_KONA_EUR_LTE)
	else if (port->line == CONFIG_GPS_S3C_UART) {
		umcon |= S3C2410_UMCOM_AFC;
	}
#endif
	else {
		umcon &= ~S3C2410_UMCOM_AFC;
	}

	wr_regl(port, S3C2410_UMCON, umcon);
#endif
}

static void s3c24xx_serial_break_ctl(struct uart_port *port, int break_state)
{
	unsigned long flags;
	unsigned int ucon;

	spin_lock_irqsave(&port->lock, flags);

	ucon = rd_regl(port, S3C2410_UCON);

	if (break_state)
		ucon |= S3C2410_UCON_SBREAK;
	else
		ucon &= ~S3C2410_UCON_SBREAK;

	wr_regl(port, S3C2410_UCON, ucon);

	spin_unlock_irqrestore(&port->lock, flags);
}

static void s3c24xx_serial_shutdown(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);
	struct s3c2410_uartcfg *cfg = s3c24xx_port_to_cfg(port);

	if (ourport->tx_claimed) {
		disable_irq(ourport->tx_irq);
		free_irq(ourport->tx_irq, ourport);
		tx_enabled(port) = 0;
		ourport->tx_claimed = 0;
	}

	if (ourport->rx_claimed) {
		disable_irq(ourport->rx_irq);
		free_irq(ourport->rx_irq, ourport);
		ourport->rx_claimed = 0;
		rx_enabled(port) = 0;
	}

	if (cfg->set_runstate)
		cfg->set_runstate(0);
}


static int s3c24xx_serial_startup(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);
	struct s3c2410_uartcfg *cfg = s3c24xx_port_to_cfg(port);
	int ret;

	dbg("s3c24xx_serial_startup: port=%p (%08lx,%p)\n",
	    port->mapbase, port->membase);

	/* runstate should be 1 before request_irq is called */
	if (cfg->set_runstate)
		cfg->set_runstate(1);

	rx_enabled(port) = 1;

	ret = request_irq(ourport->rx_irq, s3c24xx_serial_rx_chars, 0,
			  s3c24xx_serial_portname(port), ourport);

	if (ret != 0) {
		printk(KERN_ERR "cannot get irq %d\n", ourport->rx_irq);
		goto err;
	}

	ourport->rx_claimed = 1;

	dbg("requesting tx irq...\n");

	tx_enabled(port) = 1;

	ret = request_irq(ourport->tx_irq, s3c24xx_serial_tx_chars, 0,
			  s3c24xx_serial_portname(port), ourport);

	if (ret) {
		printk(KERN_ERR "cannot get irq %d\n", ourport->tx_irq);
		goto err;
	}

	ourport->tx_claimed = 1;

	dbg("s3c24xx_serial_startup ok\n");

	/* the port reset code should have done the correct
	 * register setup for the port controls */

	return ret;

 err:
	s3c24xx_serial_shutdown(port);
	return ret;
}

/* power power management control */

static void s3c24xx_serial_pm(struct uart_port *port, unsigned int level,
			      unsigned int old)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	ourport->pm_level = level;

	switch (level) {
	case 3:
		if (!IS_ERR(ourport->baudclk) && ourport->baudclk != NULL)
			clk_disable(ourport->baudclk);

		clk_disable(ourport->clk);
		break;

	case 0:
		clk_enable(ourport->clk);

		if (!IS_ERR(ourport->baudclk) && ourport->baudclk != NULL)
			clk_enable(ourport->baudclk);

		break;
	default:
		printk(KERN_ERR "s3c24xx_serial: unknown pm %d\n", level);
	}
}

/* baud rate calculation
 *
 * The UARTs on the S3C2410/S3C2440 can take their clocks from a number
 * of different sources, including the peripheral clock ("pclk") and an
 * external clock ("uclk"). The S3C2440 also adds the core clock ("fclk")
 * with a programmable extra divisor.
 *
 * The following code goes through the clock sources, and calculates the
 * baud clocks (and the resultant actual baud rates) and then tries to
 * pick the closest one and select that.
 *
*/


#define MAX_CLKS (8)

static struct s3c24xx_uart_clksrc tmp_clksrc = {
	.name		= "pclk",
	.min_baud	= 0,
	.max_baud	= 0,
	.divisor	= 1,
};

static inline int
s3c24xx_serial_getsource(struct uart_port *port, struct s3c24xx_uart_clksrc *c)
{
	struct s3c24xx_uart_info *info = s3c24xx_port_to_info(port);

	return (info->get_clksrc)(port, c);
}

static inline int
s3c24xx_serial_setsource(struct uart_port *port, struct s3c24xx_uart_clksrc *c)
{
	struct s3c24xx_uart_info *info = s3c24xx_port_to_info(port);

	return (info->set_clksrc)(port, c);
}

struct baud_calc {
	struct s3c24xx_uart_clksrc	*clksrc;
	unsigned int			 calc;
	unsigned int			 divslot;
	unsigned int			 quot;
	struct clk			*src;
};

static int s3c24xx_serial_calcbaud(struct baud_calc *calc,
				   struct uart_port *port,
				   struct s3c24xx_uart_clksrc *clksrc,
				   unsigned int baud)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);
	unsigned long rate;

	calc->src = clk_get(port->dev, clksrc->name);
	if (calc->src == NULL || IS_ERR(calc->src))
		return 0;

	rate = clk_get_rate(calc->src);
	rate /= clksrc->divisor;

	calc->clksrc = clksrc;

	if (ourport->info->has_divslot) {
		unsigned long div = rate / baud;

		/* The UDIVSLOT register on the newer UARTs allows us to
		 * get a divisor adjustment of 1/16th on the baud clock.
		 *
		 * We don't keep the UDIVSLOT value (the 16ths we calculated
		 * by not multiplying the baud by 16) as it is easy enough
		 * to recalculate.
		 */

		calc->quot = div / 16;
		calc->calc = rate / div;
	} else {
		calc->quot = (rate + (8 * baud)) / (16 * baud);
		calc->calc = (rate / (calc->quot * 16));
	}

	calc->quot--;
	return 1;
}

static unsigned int s3c24xx_serial_getclk(struct uart_port *port,
					  struct s3c24xx_uart_clksrc **clksrc,
					  struct clk **clk,
					  unsigned int baud)
{
	struct s3c2410_uartcfg *cfg = s3c24xx_port_to_cfg(port);
	struct s3c24xx_uart_clksrc *clkp;
	struct baud_calc res[MAX_CLKS];
	struct baud_calc *resptr, *best, *sptr;
	int i;

	clkp = cfg->clocks;
	best = NULL;

	if (cfg->clocks_size < 2) {
		if (cfg->clocks_size == 0)
			clkp = &tmp_clksrc;

		/* check to see if we're sourcing fclk, and if so we're
		 * going to have to update the clock source
		 */

		if (strcmp(clkp->name, "fclk") == 0) {
			struct s3c24xx_uart_clksrc src = { "", 0, };

			s3c24xx_serial_getsource(port, &src);

			/* check that the port already using fclk, and if
			 * not, then re-select fclk
			 */

			if (strcmp(src.name, clkp->name) == 0) {
				s3c24xx_serial_setsource(port, clkp);
				s3c24xx_serial_getsource(port, &src);
			}

			clkp->divisor = src.divisor;
		}

		s3c24xx_serial_calcbaud(res, port, clkp, baud);
		best = res;
		resptr = best + 1;
	} else {
		resptr = res;

		best = res;
		for (i = 0; i < cfg->clocks_size; i++, clkp++) {
			if (s3c24xx_serial_calcbaud(resptr, port, clkp, baud))
				resptr++;
		}
	}

	/* ok, we now need to select the best clock we found */

	if (!best) {
		unsigned int deviation = (1<<30)|((1<<30)-1);
		int calc_deviation;

		best = res;
		for (sptr = res; sptr < resptr; sptr++) {
			calc_deviation = baud - sptr->calc;
			if (calc_deviation < 0)
				calc_deviation = -calc_deviation;

			if (calc_deviation < deviation) {
				best = sptr;
				deviation = calc_deviation;
			}
		}
	}

	/* store results to pass back */

	*clksrc = best->clksrc;
	*clk    = best->src;

	return best->quot;
}

/* udivslot_table[]
 *
 * This table takes the fractional value of the baud divisor and gives
 * the recommended setting for the UDIVSLOT register.
 */
static u16 udivslot_table[16] = {
	[0] = 0x0000,
	[1] = 0x0080,
	[2] = 0x0808,
	[3] = 0x0888,
	[4] = 0x2222,
	[5] = 0x4924,
	[6] = 0x4A52,
	[7] = 0x54AA,
	[8] = 0x5555,
	[9] = 0xD555,
	[10] = 0xD5D5,
	[11] = 0xDDD5,
	[12] = 0xDDDD,
	[13] = 0xDFDD,
	[14] = 0xDFDF,
	[15] = 0xFFDF,
};

static void s3c24xx_serial_set_termios(struct uart_port *port,
				       struct ktermios *termios,
				       struct ktermios *old)
{
	struct s3c2410_uartcfg *cfg = s3c24xx_port_to_cfg(port);
	struct s3c24xx_uart_port *ourport = to_ourport(port);
	struct s3c24xx_uart_clksrc *clksrc = NULL;
	struct clk *clk = NULL;
	unsigned long flags;
	unsigned int baud, quot;
	unsigned int ulcon;
	unsigned int umcon;
	unsigned int udivslot = 0;

	/*
	 * We don't support modem control lines.
	 */
	termios->c_cflag &= ~(HUPCL | CMSPAR);
	termios->c_cflag |= CLOCAL;

	/*
	 * Ask the core to calculate the divisor for us.
	 */
#if defined(CONFIG_MACH_U1) || defined(CONFIG_MACH_MIDAS) || \
	defined(CONFIG_MACH_SLP_PQ) || defined(CONFIG_MACH_P10) || \
	defined(CONFIG_MACH_PX) || defined(CONFIG_MACH_TRATS)
	baud = uart_get_baud_rate(port, termios, old, 0, 4000000); // 4Mbps
#else
	baud = uart_get_baud_rate(port, termios, old, 0, 3000000);
#endif
	if (baud == 38400 && (port->flags & UPF_SPD_MASK) == UPF_SPD_CUST)
		quot = port->custom_divisor;
	else
		quot = s3c24xx_serial_getclk(port, &clksrc, &clk, baud);

	/* check to see if we need  to change clock source */

	if (ourport->clksrc != clksrc || ourport->baudclk != clk) {
		dbg("selecting clock %p\n", clk);
		s3c24xx_serial_setsource(port, clksrc);

		if (ourport->baudclk != NULL && !IS_ERR(ourport->baudclk)) {
			clk_disable(ourport->baudclk);
			ourport->baudclk  = NULL;
		}

		clk_enable(clk);

		ourport->clksrc = clksrc;
		ourport->baudclk = clk;
		ourport->baudclk_rate = clk ? clk_get_rate(clk) : 0;
	}

	if (ourport->info->has_divslot) {
		unsigned int div = ourport->baudclk_rate / baud;

		if (cfg->has_fracval) {
			udivslot = (div & 15);
			dbg("fracval = %04x\n", udivslot);
		} else {
			udivslot = udivslot_table[div & 15];
			dbg("udivslot = %04x (div %d)\n", udivslot, div & 15);
		}
	}

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		dbg("config: 5bits/char\n");
		ulcon = S3C2410_LCON_CS5;
		break;
	case CS6:
		dbg("config: 6bits/char\n");
		ulcon = S3C2410_LCON_CS6;
		break;
	case CS7:
		dbg("config: 7bits/char\n");
		ulcon = S3C2410_LCON_CS7;
		break;
	case CS8:
	default:
		dbg("config: 8bits/char\n");
		ulcon = S3C2410_LCON_CS8;
		break;
	}

	/* preserve original lcon IR settings */
	ulcon |= (cfg->ulcon & S3C2410_LCON_IRM);

	if (termios->c_cflag & CSTOPB)
		ulcon |= S3C2410_LCON_STOPB;

	umcon = (termios->c_cflag & CRTSCTS) ? S3C2410_UMCOM_AFC : 0;

	if (termios->c_cflag & PARENB) {
		if (termios->c_cflag & PARODD)
			ulcon |= S3C2410_LCON_PODD;
		else
			ulcon |= S3C2410_LCON_PEVEN;
	} else {
		ulcon |= S3C2410_LCON_PNONE;
	}

	spin_lock_irqsave(&port->lock, flags);

	dbg("setting ulcon to %08x, brddiv to %d, udivslot %08x\n",
	    ulcon, quot, udivslot);

	wr_regl(port, S3C2410_ULCON, ulcon);
	wr_regl(port, S3C2410_UBRDIV, quot);
	wr_regl(port, S3C2410_UMCON, umcon);

	if (ourport->info->has_divslot)
		wr_regl(port, S3C2443_DIVSLOT, udivslot);

	dbg("uart: ulcon = 0x%08x, ucon = 0x%08x, ufcon = 0x%08x\n",
	    rd_regl(port, S3C2410_ULCON),
	    rd_regl(port, S3C2410_UCON),
	    rd_regl(port, S3C2410_UFCON));

	/*
	 * Update the per-port timeout.
	 */
	uart_update_timeout(port, termios->c_cflag, baud);

	/*
	 * Which character status flags are we interested in?
	 */
	port->read_status_mask = S3C2410_UERSTAT_OVERRUN;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= S3C2410_UERSTAT_FRAME | S3C2410_UERSTAT_PARITY;

	/*
	 * Which character status flags should we ignore?
	 */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= S3C2410_UERSTAT_OVERRUN;
	if (termios->c_iflag & IGNBRK && termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= S3C2410_UERSTAT_FRAME;

	/*
	 * Ignore all characters if CREAD is not set.
	 */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= RXSTAT_DUMMY_READ;

	spin_unlock_irqrestore(&port->lock, flags);
}

static const char *s3c24xx_serial_type(struct uart_port *port)
{
	switch (port->type) {
	case PORT_S3C2410:
		return "S3C2410";
	case PORT_S3C2440:
		return "S3C2440";
	case PORT_S3C2412:
		return "S3C2412";
	case PORT_S3C6400:
		return "S3C6400/10";
	default:
		return NULL;
	}
}

#define MAP_SIZE (0x100)

static void s3c24xx_serial_release_port(struct uart_port *port)
{
	release_mem_region(port->mapbase, MAP_SIZE);
}

static int s3c24xx_serial_request_port(struct uart_port *port)
{
	const char *name = s3c24xx_serial_portname(port);
	return request_mem_region(port->mapbase, MAP_SIZE, name) ? 0 : -EBUSY;
}

static void s3c24xx_serial_config_port(struct uart_port *port, int flags)
{
	struct s3c24xx_uart_info *info = s3c24xx_port_to_info(port);

	if (flags & UART_CONFIG_TYPE &&
	    s3c24xx_serial_request_port(port) == 0)
		port->type = info->type;
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int
s3c24xx_serial_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	struct s3c24xx_uart_info *info = s3c24xx_port_to_info(port);

	if (ser->type != PORT_UNKNOWN && ser->type != info->type)
		return -EINVAL;

	return 0;
}

static void
s3c24xx_serial_wake_peer(struct uart_port *port)
{
	struct s3c2410_uartcfg *cfg = s3c24xx_port_to_cfg(port);

	if(cfg->wake_peer)
		cfg->wake_peer(port);
}

#ifdef CONFIG_SERIAL_SAMSUNG_CONSOLE

static struct console s3c24xx_serial_console;

#define S3C24XX_SERIAL_CONSOLE &s3c24xx_serial_console
#else
#define S3C24XX_SERIAL_CONSOLE NULL
#endif

static struct uart_ops s3c24xx_serial_ops = {
	.pm		= s3c24xx_serial_pm,
	.tx_empty	= s3c24xx_serial_tx_empty,
	.get_mctrl	= s3c24xx_serial_get_mctrl,
	.set_mctrl	= s3c24xx_serial_set_mctrl,
	.stop_tx	= s3c24xx_serial_stop_tx,
	.start_tx	= s3c24xx_serial_start_tx,
	.stop_rx	= s3c24xx_serial_stop_rx,
	.enable_ms	= s3c24xx_serial_enable_ms,
	.break_ctl	= s3c24xx_serial_break_ctl,
	.startup	= s3c24xx_serial_startup,
	.shutdown	= s3c24xx_serial_shutdown,
	.set_termios	= s3c24xx_serial_set_termios,
	.type		= s3c24xx_serial_type,
	.release_port	= s3c24xx_serial_release_port,
	.request_port	= s3c24xx_serial_request_port,
	.config_port	= s3c24xx_serial_config_port,
	.verify_port	= s3c24xx_serial_verify_port,
	.wake_peer	= s3c24xx_serial_wake_peer,
};


static struct uart_driver s3c24xx_uart_drv = {
	.owner		= THIS_MODULE,
	.driver_name	= "s3c2410_serial",
	.nr		= CONFIG_SERIAL_SAMSUNG_UARTS,
#ifdef CONFIG_SERIAL_SAMSUNG_CONSOLE_SWITCH
	.cons		= NULL,
#else
	.cons		= S3C24XX_SERIAL_CONSOLE,
#endif
	.dev_name	= S3C24XX_SERIAL_NAME,
	.major		= S3C24XX_SERIAL_MAJOR,
	.minor		= S3C24XX_SERIAL_MINOR,
};

static struct s3c24xx_uart_port s3c24xx_serial_ports[CONFIG_SERIAL_SAMSUNG_UARTS] = {
	[0] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(s3c24xx_serial_ports[0].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= IRQ_S3CUART_RX0,
			.uartclk	= 0,
			.fifosize	= 16,
			.ops		= &s3c24xx_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 0,
		}
	},
	[1] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(s3c24xx_serial_ports[1].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= IRQ_S3CUART_RX1,
			.uartclk	= 0,
			.fifosize	= 16,
			.ops		= &s3c24xx_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 1,
		}
	},
#if CONFIG_SERIAL_SAMSUNG_UARTS > 2

	[2] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(s3c24xx_serial_ports[2].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= IRQ_S3CUART_RX2,
			.uartclk	= 0,
			.fifosize	= 16,
			.ops		= &s3c24xx_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 2,
		}
	},
#endif
#if CONFIG_SERIAL_SAMSUNG_UARTS > 3
	[3] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(s3c24xx_serial_ports[3].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= IRQ_S3CUART_RX3,
			.uartclk	= 0,
			.fifosize	= 16,
			.ops		= &s3c24xx_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 3,
		}
	}
#endif
};

/* s3c24xx_serial_resetport
 *
 * wrapper to call the specific reset for this port (reset the fifos
 * and the settings)
*/

static inline int s3c24xx_serial_resetport(struct uart_port *port,
					   struct s3c2410_uartcfg *cfg)
{
	struct s3c24xx_uart_info *info = s3c24xx_port_to_info(port);

	return (info->reset_port)(port, cfg);
}


#ifdef CONFIG_CPU_FREQ

static int s3c24xx_serial_cpufreq_transition(struct notifier_block *nb,
					     unsigned long val, void *data)
{
	struct s3c24xx_uart_port *port;
	struct uart_port *uport;

	port = container_of(nb, struct s3c24xx_uart_port, freq_transition);
	uport = &port->port;

	/* check to see if port is enabled */

	if (port->pm_level != 0)
		return 0;

	/* try and work out if the baudrate is changing, we can detect
	 * a change in rate, but we do not have support for detecting
	 * a disturbance in the clock-rate over the change.
	 */

	if (IS_ERR_OR_NULL(port->baudclk))
		goto exit;

	if (port->baudclk_rate == clk_get_rate(port->baudclk))
		goto exit;

	if (val == CPUFREQ_PRECHANGE) {
		/* we should really shut the port down whilst the
		 * frequency change is in progress. */

	} else if (val == CPUFREQ_POSTCHANGE) {
		struct ktermios *termios;
		struct tty_struct *tty;

		if (uport->state == NULL)
			goto exit;

		tty = uport->state->port.tty;

		if (tty == NULL)
			goto exit;

		termios = tty->termios;

		if (termios == NULL) {
			printk(KERN_WARNING "%s: no termios?\n", __func__);
			goto exit;
		}

		s3c24xx_serial_set_termios(uport, termios, NULL);
	}

 exit:
	return 0;
}

static inline int s3c24xx_serial_cpufreq_register(struct s3c24xx_uart_port *port)
{
	port->freq_transition.notifier_call = s3c24xx_serial_cpufreq_transition;

	return cpufreq_register_notifier(&port->freq_transition,
					 CPUFREQ_TRANSITION_NOTIFIER);
}

static inline void s3c24xx_serial_cpufreq_deregister(struct s3c24xx_uart_port *port)
{
	cpufreq_unregister_notifier(&port->freq_transition,
				    CPUFREQ_TRANSITION_NOTIFIER);
}

#else
static inline int s3c24xx_serial_cpufreq_register(struct s3c24xx_uart_port *port)
{
	return 0;
}

static inline void s3c24xx_serial_cpufreq_deregister(struct s3c24xx_uart_port *port)
{
}
#endif

/* s3c24xx_serial_init_port
 *
 * initialise a single serial port from the platform device given
 */

static int s3c24xx_serial_init_port(struct s3c24xx_uart_port *ourport,
				    struct s3c24xx_uart_info *info,
				    struct platform_device *platdev)
{
	struct uart_port *port = &ourport->port;
	struct s3c2410_uartcfg *cfg;
	struct resource *res;
	int ret;

	dbg("s3c24xx_serial_init_port: port=%p, platdev=%p\n", port, platdev);

	if (platdev == NULL)
		return -ENODEV;

	cfg = s3c24xx_dev_to_cfg(&platdev->dev);

	if (port->mapbase != 0)
		return 0;

	if (cfg->hwport > CONFIG_SERIAL_SAMSUNG_UARTS) {
		printk(KERN_ERR "%s: port %d bigger than %d\n", __func__,
		       cfg->hwport, CONFIG_SERIAL_SAMSUNG_UARTS);
		return -ERANGE;
	}

	/* setup info for port */
	port->dev	= &platdev->dev;
	ourport->info	= info;

	/* copy the info in from provided structure */
	ourport->port.fifosize = info->fifosize;

	dbg("s3c24xx_serial_init_port: %p (hw %d)...\n", port, cfg->hwport);

	port->uartclk = 1;

	if (cfg->uart_flags & UPF_CONS_FLOW) {
		dbg("s3c24xx_serial_init_port: enabling flow control\n");
		port->flags |= UPF_CONS_FLOW;
	}

	/* sort our the physical and virtual addresses for each UART */

	res = platform_get_resource(platdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		printk(KERN_ERR "failed to find memory resource for uart\n");
		return -EINVAL;
	}

	dbg("resource %p (%lx..%lx)\n", res, res->start, res->end);

	port->mapbase = res->start;
	port->membase = S3C_VA_UART + (res->start & 0xfffff);
	ret = platform_get_irq(platdev, 0);
	if (ret < 0)
		port->irq = 0;
	else {
		port->irq = ret;
		ourport->rx_irq = ret;
		ourport->tx_irq = ret + 1;
	}

	ret = platform_get_irq(platdev, 1);
	if (ret > 0)
		ourport->tx_irq = ret;

	ourport->clk	= clk_get(&platdev->dev, "uart");

	dbg("port: map=%08x, mem=%08x, irq=%d (%d,%d), clock=%ld\n",
	    port->mapbase, port->membase, port->irq,
	    ourport->rx_irq, ourport->tx_irq, port->uartclk);

	/* reset the fifos (and setup the uart) */
	s3c24xx_serial_resetport(port, cfg);
	return 0;
}

static ssize_t s3c24xx_serial_show_clksrc(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	struct uart_port *port = s3c24xx_dev_to_port(dev);
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	return snprintf(buf, PAGE_SIZE, "* %s\n",
			ourport->clksrc ? ourport->clksrc->name : "(null)");
}

static DEVICE_ATTR(clock_source, S_IRUGO, s3c24xx_serial_show_clksrc, NULL);
#ifdef CSR_GPS_WORK_AROUND_UART_DRIVER
static ssize_t s3c24xx_serial_rts_pullup_detect(struct device *dev, \
	struct device_attribute *attr, char *buf)
{
	int i;
	s3c_gpio_pull_t back_pull;
	unsigned int back_cfg;

	/*back_pull = s3c_gpio_getpull(GPIO_GPS_RTS); back up*/
	back_pull = S3C_GPIO_PULL_NONE;
	/*back_cfg = s3c_gpio_getcfg(GPIO_GPS_RTS);*/
	back_cfg = S3C_GPIO_SFN(2);
	s3c_gpio_setpull(GPIO_GPS_RTS, S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_OUTPUT);

	gpio_set_value(GPIO_GPS_RTS, 0);

	s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_INPUT);

	/*here, you may need a little delay*/
	for (i = 0; i < 100; i++);
	buf[0] = '0' + gpio_get_value(GPIO_GPS_RTS);
	s3c_gpio_cfgpin(GPIO_GPS_RTS, back_cfg);
	s3c_gpio_setpull(GPIO_GPS_RTS, back_pull);

	printk(KERN_ERR KERN_DEBUG  \
		"rts_cts_gate:detect RTS pin pulled_up - %c\n", buf[0]);

	return 1;
}

static ssize_t s3c24xx_serial_rts_cts_config(struct device *dev, \
	struct device_attribute *attr, const char *buf, size_t size)
{

	s3c_gpio_pull_t rts_pull_option, cts_pull_option;

	printk(KERN_ERR KERN_DEBUG "rts_cts_gate:config [%c%c%c]\n", \
		buf[0], buf[1], buf[2]);

	rts_pull_option = S3C_GPIO_PULL_NONE;
	cts_pull_option = S3C_GPIO_PULL_NONE;

	if (buf[1] == '-') {
		rts_pull_option = S3C_GPIO_PULL_DOWN;
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate: rts pull-down\n");
	} else if (buf[1] == '+') {
		rts_pull_option = S3C_GPIO_PULL_UP;
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate: rts pull-up\n");
	} else {
		rts_pull_option = S3C_GPIO_PULL_NONE;
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate: rts pull-none\n");
	}

	if (buf[2] == '-') {
		cts_pull_option = S3C_GPIO_PULL_DOWN;
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate: cts pull-down\n");
	} else if (buf[2] == '+') {
		cts_pull_option = S3C_GPIO_PULL_UP;
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate: cts pull-up\n");
	} else {
		cts_pull_option = S3C_GPIO_PULL_NONE;
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate: cts pull-none\n");
	}

	if (buf[0] == '0') { /* rts/cts*/
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate: hw flow control\n");

		s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(GPIO_GPS_RTS, rts_pull_option);

		s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(GPIO_GPS_CTS, cts_pull_option);
	}
	/* rts high cts high*/
	else if (buf[0] == '1') { /* control pins for GPS reset*/
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate: RTS-HIGH, CTS-HIGH\n");

		s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_GPS_RTS, rts_pull_option);
		gpio_set_value(GPIO_GPS_RTS, 1);

		s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_GPS_CTS, cts_pull_option);
		gpio_set_value(GPIO_GPS_CTS, 1);
	}
	/* rts high cts input*/
	else if (buf[0] == '2') { /* control pins for GPS reset*/
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate: RTS-HIGH, CTS-INPUT\n");

		s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_GPS_RTS, rts_pull_option);
		gpio_set_value(GPIO_GPS_RTS, 1);

		s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_INPUT);
		s3c_gpio_setpull(GPIO_GPS_CTS, cts_pull_option);
	}
	/* rts input cts input*/
	else if (buf[0] == '3') { /* control pins for GPS reset */
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate: RTS-INPUT, CTS-INPUT\n");

		s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_INPUT);
		s3c_gpio_setpull(GPIO_GPS_RTS, rts_pull_option);

		s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_INPUT);
		s3c_gpio_setpull(GPIO_GPS_CTS, cts_pull_option);
	}
	/* rts input cts high */
	else if (buf[0] == '4') { /*control pins for GPS reset*/
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate: RTS-INPUT, CTS-HIGH\n");

		s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_INPUT);
		s3c_gpio_setpull(GPIO_GPS_RTS, rts_pull_option);

		s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_GPS_CTS, cts_pull_option);
		gpio_set_value(GPIO_GPS_CTS, 1);
	}
	/*test rts high*/
	else if (buf[0] == '5') { /*rts test - ouput 0*/
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate: RTS set 0\n");

		s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_GPS_RTS, rts_pull_option);

		gpio_set_value(GPIO_GPS_RTS, 0);
	}
	/* test rts low*/
	else if (buf[0] == '6') { /*rts test - output 1*/
		printk(KERN_ERR KERN_DEBUG "rts_cts_gate:RTS set 1\n");

		s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_GPS_RTS, rts_pull_option);

		gpio_set_value(GPIO_GPS_RTS, 1);
	}

	return 1;
}

static DEVICE_ATTR(rts_cts_gate, \
	S_IRGRP | S_IWGRP | S_IRUSR | S_IWUSR | S_IROTH, \
		s3c24xx_serial_rts_pullup_detect, \
		s3c24xx_serial_rts_cts_config);
#endif

/* Device driver serial port probe */

static int probe_index;

int s3c24xx_serial_probe(struct platform_device *dev,
			 struct s3c24xx_uart_info *info)
{
	struct s3c24xx_uart_port *ourport;
	int ret;

	dbg("s3c24xx_serial_probe(%p, %p) %d\n", dev, info, probe_index);

	ourport = &s3c24xx_serial_ports[probe_index];
	probe_index++;

	dbg("%s: initialising port %p...\n", __func__, ourport);

	ret = s3c24xx_serial_init_port(ourport, info, dev);
	if (ret < 0)
		goto probe_err;

	dbg("%s: adding port\n", __func__);
	uart_add_one_port(&s3c24xx_uart_drv, &ourport->port);
	platform_set_drvdata(dev, &ourport->port);

	ret = device_create_file(&dev->dev, &dev_attr_clock_source);
	if (ret < 0)
		printk(KERN_ERR "%s: failed to add clksrc attr.\n", __func__);

#ifdef CSR_GPS_WORK_AROUND_UART_DRIVER
	ret = device_create_file(&dev->dev, &dev_attr_rts_cts_gate); /* woojun*/
	if (ret < 0)
		printk(KERN_ERR "%s: failed to add rts_cts_gate attr.\n", \
			__func__);
#endif
	ret = s3c24xx_serial_cpufreq_register(ourport);
	if (ret < 0)
		dev_err(&dev->dev, "failed to add cpufreq notifier\n");

	return 0;

 probe_err:
	return ret;
}

EXPORT_SYMBOL_GPL(s3c24xx_serial_probe);

int __devexit s3c24xx_serial_remove(struct platform_device *dev)
{
	struct uart_port *port = s3c24xx_dev_to_port(&dev->dev);

	if (port) {
		s3c24xx_serial_cpufreq_deregister(to_ourport(port));
		device_remove_file(&dev->dev, &dev_attr_clock_source);
		uart_remove_one_port(&s3c24xx_uart_drv, port);
	}

	return 0;
}

EXPORT_SYMBOL_GPL(s3c24xx_serial_remove);

/* UART power management code */

#ifdef CONFIG_PM

static int s3c24xx_serial_suspend(struct platform_device *dev, pm_message_t state)
{
	struct uart_port *port = s3c24xx_dev_to_port(&dev->dev);

	if (port)
		uart_suspend_port(&s3c24xx_uart_drv, port);

	return 0;
}

static int s3c24xx_serial_resume(struct platform_device *dev)
{
	struct uart_port *port = s3c24xx_dev_to_port(&dev->dev);
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (port) {
		clk_enable(ourport->clk);
		s3c24xx_serial_resetport(port, s3c24xx_port_to_cfg(port));
		clk_disable(ourport->clk);

		uart_resume_port(&s3c24xx_uart_drv, port);
	}

	return 0;
}
#endif

int s3c24xx_serial_init(struct platform_driver *drv,
			struct s3c24xx_uart_info *info)
{
	dbg("s3c24xx_serial_init(%p,%p)\n", drv, info);

#ifdef CONFIG_PM
	drv->suspend = s3c24xx_serial_suspend;
	drv->resume = s3c24xx_serial_resume;
#endif

	return platform_driver_register(drv);
}

EXPORT_SYMBOL_GPL(s3c24xx_serial_init);

/* module initialisation code */

static int __init s3c24xx_serial_modinit(void)
{
	int ret;
#ifdef CONFIG_SERIAL_SAMSUNG_CONSOLE_SWITCH
	if (uart_debug)
		s3c24xx_uart_drv.cons = S3C24XX_SERIAL_CONSOLE;
#endif
	ret = uart_register_driver(&s3c24xx_uart_drv);
	if (ret < 0) {
		printk(KERN_ERR "failed to register UART driver\n");
		return -1;
	}

	return 0;
}

static void __exit s3c24xx_serial_modexit(void)
{
	uart_unregister_driver(&s3c24xx_uart_drv);
}

module_init(s3c24xx_serial_modinit);
module_exit(s3c24xx_serial_modexit);

/* Console code */

#ifdef CONFIG_SERIAL_SAMSUNG_CONSOLE

static struct uart_port *cons_uart;

static int
s3c24xx_serial_console_txrdy(struct uart_port *port, unsigned int ufcon)
{
	struct s3c24xx_uart_info *info = s3c24xx_port_to_info(port);
	unsigned long ufstat, utrstat;

	if (ufcon & S3C2410_UFCON_FIFOMODE) {
		/* fifo mode - check amount of data in fifo registers... */

		ufstat = rd_regl(port, S3C2410_UFSTAT);
		return (ufstat & info->tx_fifofull) ? 0 : 1;
	}

	/* in non-fifo mode, we go and use the tx buffer empty */

	utrstat = rd_regl(port, S3C2410_UTRSTAT);
	return (utrstat & S3C2410_UTRSTAT_TXE) ? 1 : 0;
}

static void
s3c24xx_serial_console_putchar(struct uart_port *port, int ch)
{
	unsigned int ufcon = rd_regl(cons_uart, S3C2410_UFCON);
	while (!s3c24xx_serial_console_txrdy(port, ufcon))
		barrier();
	wr_regb(cons_uart, S3C2410_UTXH, ch);
}

static void
s3c24xx_serial_console_write(struct console *co, const char *s,
			     unsigned int count)
{
	uart_console_write(cons_uart, s, count, s3c24xx_serial_console_putchar);
}

static void __init
s3c24xx_serial_get_options(struct uart_port *port, int *baud,
			   int *parity, int *bits)
{
	struct s3c24xx_uart_clksrc clksrc = { "", 0, };
	struct clk *clk;
	unsigned int ulcon;
	unsigned int ucon;
	unsigned int ubrdiv;
	unsigned long rate;

	ulcon  = rd_regl(port, S3C2410_ULCON);
	ucon   = rd_regl(port, S3C2410_UCON);
	ubrdiv = rd_regl(port, S3C2410_UBRDIV);

	dbg("s3c24xx_serial_get_options: port=%p\n"
	    "registers: ulcon=%08x, ucon=%08x, ubdriv=%08x\n",
	    port, ulcon, ucon, ubrdiv);

	if ((ucon & 0xf) != 0) {
		/* consider the serial port configured if the tx/rx mode set */

		switch (ulcon & S3C2410_LCON_CSMASK) {
		case S3C2410_LCON_CS5:
			*bits = 5;
			break;
		case S3C2410_LCON_CS6:
			*bits = 6;
			break;
		case S3C2410_LCON_CS7:
			*bits = 7;
			break;
		default:
		case S3C2410_LCON_CS8:
			*bits = 8;
			break;
		}

		switch (ulcon & S3C2410_LCON_PMASK) {
		case S3C2410_LCON_PEVEN:
			*parity = 'e';
			break;

		case S3C2410_LCON_PODD:
			*parity = 'o';
			break;

		case S3C2410_LCON_PNONE:
		default:
			*parity = 'n';
		}

		/* now calculate the baud rate */

		s3c24xx_serial_getsource(port, &clksrc);

		clk = clk_get(port->dev, clksrc.name);
		if (!IS_ERR(clk) && clk != NULL)
			rate = clk_get_rate(clk) / clksrc.divisor;
		else
			rate = 1;

		*baud = rate / (16 * (ubrdiv + 1));
		dbg("calculated baud %d\n", *baud);
	}

}

/* s3c24xx_serial_init_ports
 *
 * initialise the serial ports from the machine provided initialisation
 * data.
*/

static int s3c24xx_serial_init_ports(struct s3c24xx_uart_info **info)
{
	struct s3c24xx_uart_port *ptr = s3c24xx_serial_ports;
	struct platform_device **platdev_ptr;
	int i;

	dbg("s3c24xx_serial_init_ports: initialising ports...\n");

	platdev_ptr = s3c24xx_uart_devs;

	for (i = 0; i < CONFIG_SERIAL_SAMSUNG_UARTS; i++, ptr++, platdev_ptr++) {
		s3c24xx_serial_init_port(ptr, info[i], *platdev_ptr);
	}

	return 0;
}

static int __init
s3c24xx_serial_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 9600;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	dbg("s3c24xx_serial_console_setup: co=%p (%d), %s\n",
	    co, co->index, options);

	/* is this a valid port */

	if (co->index == -1 || co->index >= CONFIG_SERIAL_SAMSUNG_UARTS)
		co->index = 0;

	port = &s3c24xx_serial_ports[co->index].port;

	/* is the port configured? */

	if (port->mapbase == 0x0)
		return -ENODEV;

	cons_uart = port;

	dbg("s3c24xx_serial_console_setup: port=%p (%d)\n", port, co->index);

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	else
		s3c24xx_serial_get_options(port, &baud, &parity, &bits);

	dbg("s3c24xx_serial_console_setup: baud %d\n", baud);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

/* s3c24xx_serial_initconsole
 *
 * initialise the console from one of the uart drivers
*/

static struct console s3c24xx_serial_console = {
	.name		= S3C24XX_SERIAL_NAME,
	.device		= uart_console_device,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.write		= s3c24xx_serial_console_write,
	.setup		= s3c24xx_serial_console_setup,
	.data		= &s3c24xx_uart_drv,
};

int s3c24xx_serial_initconsole(struct platform_driver *drv,
			       struct s3c24xx_uart_info **info)

{
	struct platform_device *dev = s3c24xx_uart_devs[0];

	dbg("s3c24xx_serial_initconsole\n");

	/* select driver based on the cpu */

	if (dev == NULL) {
		printk(KERN_ERR "s3c24xx: no devices for console init\n");
		return 0;
	}

	if (strcmp(dev->name, drv->driver.name) != 0)
		return 0;

	s3c24xx_serial_console.data = &s3c24xx_uart_drv;
	s3c24xx_serial_init_ports(info);

	register_console(&s3c24xx_serial_console);
	return 0;
}

#endif /* CONFIG_SERIAL_SAMSUNG_CONSOLE */

MODULE_DESCRIPTION("Samsung SoC Serial port driver");
MODULE_AUTHOR("Ben Dooks <ben@simtec.co.uk>");
MODULE_LICENSE("GPL v2");
