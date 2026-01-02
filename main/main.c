#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

// --- Configuration Constants ---
// We use UART0 because that is connected to your USB cable
#define UART_PORT_NUM      UART_NUM_0
#define UART_BAUD_RATE     115200
#define RX_BUF_SIZE        1024

static const char *TAG = "CLI";

void init_uart(void) {
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // 1. Configure the UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));

    // 2. Set the pins. 
    // UART_PIN_NO_CHANGE tells it to keep using the default pins (GPIO 1/3)
    // which connect to your USB cable.
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // 3. Install the driver
    // We allocate a receive buffer (RX) but no transmit buffer (TX) because 
    // writes block until sent anyway in this simple mode.
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    
    ESP_LOGI(TAG, "UART Driver initialized successfully");
}

void app_main(void)
{
    init_uart();

    // Send a raw welcome message using the driver (not printf!)
    const char *welcome_msg = "\r\n> CLI Ready. Type something...\r\n";
    uart_write_bytes(UART_PORT_NUM, welcome_msg, strlen(welcome_msg));

    while (1) {
        // Just a placeholder loop for now
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}