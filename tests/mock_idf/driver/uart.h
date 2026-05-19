#ifndef MOCK_UART_H
#define MOCK_UART_H

#include <stddef.h>
#include <stdint.h>

typedef int uart_port_t;
#define UART_NUM_0 0
#define UART_PIN_NO_CHANGE -1

typedef struct {
    int baud_rate;
    int data_bits;
    int parity;
    int stop_bits;
    int flow_ctrl;
    int source_clk;
} uart_config_t;

#define UART_DATA_8_BITS 8
#define UART_PARITY_DISABLE 0
#define UART_STOP_BITS_1 1
#define UART_HW_FLOWCTRL_DISABLE 0
#define UART_SCLK_DEFAULT 0

// Standard ESP-IDF UART functions
int uart_param_config(uart_port_t uart_num, const uart_config_t *uart_config);
int uart_set_pin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num);
int uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, void *uart_queue, int intr_alloc_flags);
int uart_write_bytes(uart_port_t uart_num, const void *src, size_t size);
int uart_read_bytes(uart_port_t uart_num, void *buf, uint32_t length, int ticks_to_wait);

#endif
