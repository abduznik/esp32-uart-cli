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

    // Configure the UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));

    // Set the pins. 
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // 3. Install the driver
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    
    ESP_LOGI(TAG, "UART Driver initialized successfully");
}

void run_command(char *cmd)
{
    // If the same string they equal 0
    if (strcmp(cmd, "help") == 0)
    {
        const char *msg = "\r\nAvailable Commands:\r\n  help - Show this list\r\n  ping - Test response\r\n  clear - Clear screen\r\n";
        uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
    }

    // Simple ping command for testing
    else if (strcmp(cmd, "ping") == 0)
    {
        const char *msg = "\r\npong!\r\n";
        uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
    }

    else if (strcmp(cmd, "clear") == 0)
    {
        // Code for clearing terminal screen in ANSI
        const char *msg = "\033[2J\033[H";
        uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
    }

    else if (strlen(cmd) > 0)
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "\r\nUnknown command: %s\r\n", cmd);
        uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
    }

    uart_write_bytes(UART_PORT_NUM, "\r\n> ", 4);
}

void app_main(void)
{
    init_uart();

    // Heap allocation for the raw hardware buffer
    uint8_t *data = (uint8_t *) malloc(RX_BUF_SIZE + 1);

    // Stack allocation for our line buffer
    char line_buffer[128];
    int line_index = 0;

    uart_write_bytes(UART_PORT_NUM, "\r\n> ", 4);

    const char *welcome_msg = "\r\n> CLI Ready. Type a command and press ENTER:\r\n";
    uart_write_bytes(UART_PORT_NUM, welcome_msg, strlen(welcome_msg));

    while (1) {
        // Read Raw Data from ESP32
        // Note: capital TICK in portTICK_PERIOD_MS
        int len = uart_read_bytes(UART_PORT_NUM, data, RX_BUF_SIZE, 20 / portTICK_PERIOD_MS);

        if (len > 0) {
            // Process each character
            for (int i = 0; i < len; i++) {
                char c = data[i];

                // Check for ENTER key
                if (c == '\r' || c == '\n') {
                    line_buffer[line_index] = '\0'; // Null-terminate the string
                    
                    // Internal Handling
                    uart_write_bytes(UART_PORT_NUM, "\r\n", 2);
                    run_command(line_buffer);
                    
                    // Reset Buffer
                    line_index = 0;
                }
                // Check for BACKSPACE
                else if (c == 127 || c == 0x08) {
                    if (line_index > 0) {
                        line_index--;
                        uart_write_bytes(UART_PORT_NUM, "\b \b", 3);
                    }
                }
                // Normal Character
                else {
                    if (line_index < sizeof(line_buffer) - 1) {
                        line_buffer[line_index++] = c;
                        uart_write_bytes(UART_PORT_NUM, &c, 1);
                    }
                }
            }
        }
    }
    free(data);
}