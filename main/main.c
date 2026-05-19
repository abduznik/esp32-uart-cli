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

// Component Headers
#include "console.h"
#include "led.h"
#include "gpio_control.h"
#include "adc_control.h"
#include "i2c_control.h"

#define RX_BUF_SIZE        1024

void run_command(char *cmd)
{
    if (strcmp(cmd, "help") == 0)
    {
        console_print("\r\nAvailable Commands:\r\n"  
                      "help - Show this list\r\n"  
                      "ping - Test response\r\n"  
                      "clear - Clear screen\r\n"
                      "led on - Turns LED on\r\n"  
                      "led off - Turns LED off\r\n"
                      "gpio status - List all GPIO pins, safety levels & values\r\n"
                      "gpio read <pin> - Read digital state of a pin\r\n"
                      "gpio <pin> <0/1> - Set a pin level (Output)\r\n"
                      "gpio set <pin> <0/1> - Set a pin level (Output)\r\n"
                      "adc status - List all ADC pins, channels & raw/voltage levels\r\n"
                      "adc read <pin> - Read analog value and voltage on a pin\r\n"
                      "adc <pin> - Read analog value and voltage on a pin\r\n"
                      "i2c scan - Scan current I2C bus for devices\r\n"
                      "i2c init <sda> <scl> - Configure active SCL/SDA pins dynamically\r\n"
                      "i2c read <addr> <reg> - Read a 1-byte register value\r\n"
                      "i2c write <addr> <reg> <val> - Write a 1-byte register value\r\n"
                      "version - Show the firmware version\r\n"
                      "restart - Trigger a software reset\r\n"
                      "uptime - Show the system uptime duration\r\n");
    }
    else if (strcmp(cmd, "ping") == 0)
    {
        console_print("\r\npong!\r\n");
    }
    else if (strcmp(cmd, "clear") == 0)
    {
        console_print("\033[2J\033[H");
    }
    else if (strncmp(cmd, "led ", 4) == 0)
    {
        char* arg = cmd + 4;
        if (strcmp(arg, "on") == 0)
        {
            led_on();
            console_print("\r\nLED is ON\r\n");
        }
        else if (strcmp(arg, "off") == 0)
        {
            led_off();
            console_print("\r\nLED is OFF\r\n");
        }
        else
        {
            console_print("\r\nLED is UNKNOWN\r\n");
        }
    }
    else if (strncmp(cmd, "gpio ", 5) == 0)
    {
        char *sub = cmd + 5;
        if (strcmp(sub, "status") == 0 || strcmp(sub, "list") == 0)
        {
            print_gpio_status();
        }
        else if (strncmp(sub, "read ", 5) == 0)
        {
            int pin;
            if (sscanf(sub + 5, "%d", &pin) == 1)
            {
                int val;
                int res = read_gpio_level_safe(pin, &val);
                if (res == -1) {
                    console_print("\r\n\033[0;31mError: Invalid pin number or not exposed on standard ESP32!\033[0m\r\n");
                } else if (res == -2) {
                    console_print("\r\n\033[0;31mError: Pin is reserved for SPI Flash! Reading it directly is prohibited to avoid CPU crash.\033[0m\r\n");
                } else {
                    console_printf("\r\nGPIO %d state: \033[1;36m%d\033[0m (%s)\r\n", pin, val, get_pin_safety_str(get_pin_safety(pin)));
                }
            }
            else
            {
                console_print("\r\nUsage: gpio read <pin>\r\n");
            }
        }
        else
        {
            int pin, state;
            bool valid_parse = false;
            if (strncmp(sub, "set ", 4) == 0) {
                if (sscanf(sub + 4, "%d %d", &pin, &state) == 2) valid_parse = true;
            } else {
                if (sscanf(sub, "%d %d", &pin, &state) == 2) valid_parse = true;
            }
            
            if (valid_parse)
            {
                if (state != 0 && state != 1)
                {
                    console_print("\r\n\033[0;31mError: State must be 0 or 1!\033[0m\r\n");
                }
                else
                {
                    char out_msg[256];
                    write_gpio_level_safe(pin, state, out_msg, sizeof(out_msg));
                    console_print(out_msg);
                }
            }
            else
            {
                console_print("\r\nUsage:\r\n"
                              "  gpio status            - Show status table of all pins\r\n"
                              "  gpio read <pin>        - Read current level of a pin\r\n"
                              "  gpio <pin> <0/1>       - Set output level of a pin\r\n"
                              "  gpio set <pin> <0/1>   - Set output level of a pin\r\n");
            }
        }
    }
    else if (strncmp(cmd, "adc ", 4) == 0 || strcmp(cmd, "adc") == 0)
    {
        char *sub = cmd + 4;
        if (strcmp(cmd, "adc") == 0 || strcmp(sub, "status") == 0 || strcmp(sub, "list") == 0)
        {
            print_adc_status();
        }
        else if (strncmp(sub, "read ", 5) == 0)
        {
            int pin;
            if (sscanf(sub + 5, "%d", &pin) == 1)
            {
                int raw_val = 0;
                int res = read_adc_pin(pin, &raw_val);
                if (res == 0)
                {
                    int mv = (raw_val * 3100) / 4095;
                    console_printf("\r\nGPIO %d (ADC1 Channel): Raw = \033[1;35m%d\033[0m | Voltage = \033[1;32m%d mV (~%.2f V)\033[0m\r\n", pin, raw_val, mv, mv / 1000.0);
                }
                else
                {
                    console_print("\r\n\033[0;31mError: GPIO is not a valid ADC1 pin! Supported: 32, 33, 34, 35, 36, 39.\033[0m\r\n");
                }
            }
            else
            {
                console_print("\r\nUsage: adc read <pin>\r\n");
            }
        }
        else
        {
            int pin;
            if (sscanf(sub, "%d", &pin) == 1)
            {
                int raw_val = 0;
                int res = read_adc_pin(pin, &raw_val);
                if (res == 0)
                {
                    int mv = (raw_val * 3100) / 4095;
                    console_printf("\r\nGPIO %d (ADC1 Channel): Raw = \033[1;35m%d\033[0m | Voltage = \033[1;32m%d mV (~%.2f V)\033[0m\r\n", pin, raw_val, mv, mv / 1000.0);
                }
                else
                {
                    console_print("\r\n\033[0;31mError: GPIO is not a valid ADC1 pin! Supported: 32, 33, 34, 35, 36, 39.\033[0m\r\n");
                }
            }
            else
            {
                console_print("\r\nUsage:\r\n"
                              "  adc status         - Show status table of all analog pins\r\n"
                              "  adc read <pin>     - Read analog level of a pin\r\n"
                              "  adc <pin>          - Read analog level of a pin\r\n");
            }
        }
    }
    else if (strncmp(cmd, "i2c ", 4) == 0 || strcmp(cmd, "i2c") == 0)
    {
        char *sub = cmd + 4;
        if (strcmp(cmd, "i2c") == 0 || strcmp(sub, "scan") == 0)
        {
            i2c_scan();
        }
        else if (strncmp(sub, "init ", 5) == 0)
        {
            int sda, scl;
            if (sscanf(sub + 5, "%d %d", &sda, &scl) == 2)
            {
                int res = init_i2c_pins(sda, scl);
                if (res == 0)
                {
                    console_printf("\r\n\033[0;32mI2C Bus reconfigured successfully (SDA: GPIO %d, SCL: GPIO %d)!\033[0m\r\n", sda, scl);
                }
            }
            else
            {
                console_print("\r\nUsage: i2c init <sda_pin> <scl_pin>\r\n");
            }
        }
        else if (strncmp(sub, "read ", 5) == 0)
        {
            unsigned int addr, reg;
            if (sscanf(sub + 5, "%x %x", &addr, &reg) == 2)
            {
                uint8_t val = 0;
                int res = i2c_read_reg(addr, reg, &val, 1);
                if (res == 0)
                {
                    console_printf("\r\nI2C Read [Addr: 0x%02x, Reg: 0x%02x] -> \033[1;32m0x%02x\033[0m\r\n", addr, reg, val);
                }
                else
                {
                    console_print("\r\n\033[0;31mError: No response from I2C device!\033[0m\r\n");
                }
            }
            else
            {
                console_print("\r\nUsage: i2c read <hex_addr> <hex_reg>\r\n");
            }
        }
        else if (strncmp(sub, "write ", 6) == 0)
        {
            unsigned int addr, reg, val;
            if (sscanf(sub + 6, "%x %x %x", &addr, &reg, &val) == 3)
            {
                int res = i2c_write_reg(addr, reg, val);
                if (res == 0)
                {
                    console_printf("\r\n\033[0;32mI2C Write Success [Addr: 0x%02x, Reg: 0x%02x] <- 0x%02x\033[0m\r\n", addr, reg, val);
                }
                else
                {
                    console_print("\r\n\033[0;31mError: Failed writing to I2C device!\033[0m\r\n");
                }
            }
            else
            {
                console_print("\r\nUsage: i2c write <hex_addr> <hex_reg> <hex_val>\r\n");
            }
        }
        else
        {
            console_print("\r\nUsage:\r\n"
                          "  i2c scan                      - Scan dynamic bus address grid\r\n"
                          "  i2c init <sda> <scl>          - Configure active bus pins\r\n"
                          "  i2c read <addr> <reg>         - Read hex byte from address & register\r\n"
                          "  i2c write <addr> <reg> <val>  - Write hex byte to address & register\r\n");
        }
    }
    else if (strcmp(cmd, "version") == 0)
    {
        console_printf("\r\nFirmware Version: v1.0.2\r\n");
    }
    else if (strcmp(cmd, "restart") == 0)
    {
        console_print("\r\nRestarting ESP32...\r\n");
        esp_restart();
    }
    else if (strcmp(cmd, "uptime") == 0)
    {
        int64_t us = esp_timer_get_time();
        int seconds = (int)(us / 1000000);
        console_printf("\r\nSystem uptime: %d seconds\r\n", seconds);
    }
    else if (strlen(cmd) > 0)
    {
        console_printf("\r\nUnknown command: %s\r\n", cmd);
    }

    console_print("\r\n> ");
}

void app_main(void)
{
    init_uart();
    init_led();

    uint8_t *data = (uint8_t *) malloc(RX_BUF_SIZE + 1);
    char line_buffer[128];
    int line_index = 0;

    console_print("\r\n> \r\n> CLI Ready. Type a command and press ENTER:\r\n");

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, RX_BUF_SIZE, 20 / portTICK_PERIOD_MS);
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