#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

// Core Framework Headers
#include "config.h"
#include "commands.h"
#include "console.h"
#include "led.h"

void run_command(char *cmd)
{
    char cmd_copy[128];
    strncpy(cmd_copy, cmd, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';

    char *argv[CLI_MAX_ARGS];
    int argc = 0;

    // Tokenize the input buffer copy by spaces
    char *token = strtok(cmd_copy, " ");
    while (token != NULL && argc < CLI_MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    if (argc == 0) {
        console_print("\r\n> ");
        return;
    }

    // Lookup command in static registry
    const Command *c = find_command(argv[0]);
    if (c != NULL) {
        c->handler(argc, argv);
    } else {
        console_printf("\r\nUnknown command: %s\r\n", argv[0]);
    }

    console_print("\r\n> ");
}

void app_main(void)
{
    init_uart();
    init_led();

    // Use unified buffer size from config
    uint8_t *data = (uint8_t *) malloc(CLI_UART_BUF_SIZE + 1);
    char line_buffer[128];
    int line_index = 0;

    console_print("\r\n> \r\n> CLI Ready. Type a command and press ENTER:\r\n");

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, CLI_UART_BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                char c = data[i];
                if (c == '\r' || c == '\n') {
                    line_buffer[line_index] = '\0';
                    console_print("\r\n");
                    run_command(line_buffer);
                    line_index = 0;
                }
                else if (c == 127 || c == 0x08) {
                    if (line_index > 0) {
                        line_index--;
                        console_print("\b \b");
                    }
                }
                else {
                    if (line_index < sizeof(line_buffer) - 1) {
                        line_buffer[line_index++] = c;
                        console_printf("%c", c);
                    }
                }
            }
        }
    }
    free(data);
}