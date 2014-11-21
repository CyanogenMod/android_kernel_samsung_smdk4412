/*
 * uart_select.h - UART Selection Driver
 *
 * Copyright (C) 2009 Samsung Electronics
 * Kim Kyuwon <q1.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef _UART_SELECT_H_
#define _UART_SELECT_H_

#define UART_SW_PATH_NA		-1
#define UART_SW_PATH_AP		1
#define UART_SW_PATH_CP		0

struct uart_select_platform_data {
	int (*get_uart_switch)(void);
	void (*set_uart_switch)(int path);
};

extern int uart_sel_get_state(void);
#endif /* _UART_SELECT_H_ */

