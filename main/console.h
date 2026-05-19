#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>

#define UART_PORT_NUM      0 // UART_NUM_0

void init_uart(void);
void console_print(const char *str);
void console_printf(const char *format, ...);

#endif
