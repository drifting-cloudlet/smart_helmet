#ifndef UART_APP_H
#define UART_APP_H

#include "mydefine.h"

#define UART_NAME0 "uart0"

/* 类似 rt_kprintf，但可以指定串口 */
void uart_printf(rt_device_t dev, const char *fmt, ...);

#endif
