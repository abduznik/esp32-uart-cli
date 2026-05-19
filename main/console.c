#include "console.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"

#define UART_BAUD_RATE     115200
#define RX_BUF_SIZE        1024

static const char *TAG = "CLI_UART";

void init_uart(void) 
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    
    ESP_LOGI(TAG, "UART Driver initialized successfully");
}

void console_print(const char *str) {
    if (str) {
        uart_write_bytes(UART_PORT_NUM, str, strlen(str));
    }
}

void console_printf(const char *format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    if (len > 0) {
        uart_write_bytes(UART_PORT_NUM, buf, len);
    }
}
