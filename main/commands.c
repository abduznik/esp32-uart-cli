#include "commands.h"
#include "config.h"
#include "console.h"
#include "led.h"
#include "gpio_control.h"
#include "adc_control.h"
#include "i2c_control.h"
#include "esp_system.h"
#include "esp_timer.h"
#include <string.h>
#include <stdio.h>

// Forward declarations of handlers
static void handle_help(int argc, char **argv);
static void handle_ping(int argc, char **argv);
static void handle_clear(int argc, char **argv);
static void handle_led(int argc, char **argv);
static void handle_gpio(int argc, char **argv);
static void handle_adc(int argc, char **argv);
static void handle_i2c(int argc, char **argv);
static void handle_version(int argc, char **argv);
static void handle_restart(int argc, char **argv);
static void handle_uptime(int argc, char **argv);

// Global Commands Table
const Command commands[] = {
    {"help",      "Show list of all available commands", handle_help},
    {"ping",      "Test console connection response", handle_ping},
    {"clear",     "Clear console terminal screen", handle_clear},
    {"led",       "Control onboard Blue LED pin [on/off]", handle_led},
    {"gpio",      "Access status table (gpio status), read pin (gpio read <pin>), or set output level", handle_gpio},
    {"adc",       "Read analog status table (adc status) or on-demand pin conversion (adc read <pin>)", handle_adc},
    {"i2c",       "Dynamic scanner (i2c scan), dynamic bus pins setup (i2c init <sda> <scl>), or read/write hex bytes", handle_i2c},
    {"version",   "Show active firmware version", handle_version},
    {"restart",   "Trigger a CPU soft restart reset", handle_restart},
    {"uptime",    "Show elapsed time in seconds since boot", handle_uptime}
};

const size_t num_commands = sizeof(commands) / sizeof(Command);

const Command* find_command(const char *name) {
    if (name == NULL) return NULL;
    for (size_t i = 0; i < num_commands; i++) {
        if (strcmp(commands[i].name, name) == 0) {
            return &commands[i];
        }
    }
    return NULL;
}

// Handlers Implementation

static void handle_help(int argc, char **argv) {
    console_print("\r\nAvailable Commands:\r\n");
    for (size_t i = 0; i < num_commands; i++) {
        console_printf("  %-10s - %s\r\n", commands[i].name, commands[i].description);
    }
}

static void handle_ping(int argc, char **argv) {
    console_print("\r\npong!\r\n");
}

static void handle_clear(int argc, char **argv) {
    console_print("\033[2J\033[H");
}

static void handle_led(int argc, char **argv) {
    if (argc < 2) {
        console_print("\r\nUsage: led [on/off]\r\n");
        return;
    }
    if (strcmp(argv[1], "on") == 0) {
        led_on();
        console_print("\r\nLED is ON\r\n");
    } else if (strcmp(argv[1], "off") == 0) {
        led_off();
        console_print("\r\nLED is OFF\r\n");
    } else {
        console_print("\r\nLED is UNKNOWN\r\n");
    }
}

static void handle_gpio(int argc, char **argv) {
    if (argc < 2) {
        console_print("\r\nUsage:\r\n"
                      "  gpio status            - Show status table of all pins\r\n"
                      "  gpio read <pin>        - Read current level of a pin\r\n"
                      "  gpio <pin> <0/1>       - Set output level of a pin\r\n"
                      "  gpio set <pin> <0/1>   - Set output level of a pin\r\n");
        return;
    }

    if (strcmp(argv[1], "status") == 0 || strcmp(argv[1], "list") == 0) {
        print_gpio_status();
    } else if (strcmp(argv[1], "read") == 0) {
        if (argc < 3) {
            console_print("\r\nUsage: gpio read <pin>\r\n");
            return;
        }
        int pin;
        if (sscanf(argv[2], "%d", &pin) == 1) {
            int val;
            int res = read_gpio_level_safe(pin, &val);
            if (res == -1) {
                console_print("\r\n\033[0;31mError: Invalid pin number or not exposed on standard ESP32!\033[0m\r\n");
            } else if (res == -2) {
                console_print("\r\n\033[0;31mError: Pin is reserved for SPI Flash! Reading it directly is prohibited to avoid CPU crash.\033[0m\r\n");
            } else {
                console_printf("\r\nGPIO %d state: \033[1;36m%d\033[0m (%s)\r\n", pin, val, get_pin_safety_str(get_pin_safety(pin)));
            }
        } else {
            console_print("\r\nUsage: gpio read <pin>\r\n");
        }
    } else {
        int pin_idx = 1;
        int val_idx = 2;
        if (strcmp(argv[1], "set") == 0) {
            pin_idx = 2;
            val_idx = 3;
        }

        if (argc <= val_idx) {
            console_print("\r\nUsage:\r\n"
                          "  gpio status            - Show status table of all pins\r\n"
                          "  gpio read <pin>        - Read current level of a pin\r\n"
                          "  gpio <pin> <0/1>       - Set output level of a pin\r\n"
                          "  gpio set <pin> <0/1>   - Set output level of a pin\r\n");
            return;
        }

        int pin, state;
        if (sscanf(argv[pin_idx], "%d", &pin) == 1 && sscanf(argv[val_idx], "%d", &state) == 1) {
            if (state != 0 && state != 1) {
                console_print("\r\n\033[0;31mError: State must be 0 or 1!\033[0m\r\n");
            } else {
                char out_msg[256];
                write_gpio_level_safe(pin, state, out_msg, sizeof(out_msg));
                console_print(out_msg);
            }
        } else {
            console_print("\r\nUsage: gpio <pin> <0/1> or gpio set <pin> <0/1>\r\n");
        }
    }
}

static void handle_adc(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "status") == 0 || strcmp(argv[1], "list") == 0) {
        print_adc_status();
        return;
    }

    if (strcmp(argv[1], "read") == 0) {
        if (argc < 3) {
            console_print("\r\nUsage: adc read <pin>\r\n");
            return;
        }
        int pin;
        if (sscanf(argv[2], "%d", &pin) == 1) {
            int raw_val = 0;
            int res = read_adc_pin(pin, &raw_val);
            if (res == 0) {
                int mv = (raw_val * 3100) / 4095;
                console_printf("\r\nGPIO %d (ADC1 Channel): Raw = \033[1;35m%d\033[0m | Voltage = \033[1;32m%d mV (~%.2f V)\033[0m\r\n", pin, raw_val, mv, mv / 1000.0);
            } else {
                console_print("\r\n\033[0;31mError: GPIO is not a valid ADC1 pin! Supported: 32, 33, 34, 35, 36, 39.\033[0m\r\n");
            }
        } else {
            console_print("\r\nUsage: adc read <pin>\r\n");
        }
    } else {
        int pin;
        if (sscanf(argv[1], "%d", &pin) == 1) {
            int raw_val = 0;
            int res = read_adc_pin(pin, &raw_val);
            if (res == 0) {
                int mv = (raw_val * 3100) / 4095;
                console_printf("\r\nGPIO %d (ADC1 Channel): Raw = \033[1;35m%d\033[0m | Voltage = \033[1;32m%d mV (~%.2f V)\033[0m\r\n", pin, raw_val, mv, mv / 1000.0);
            } else {
                console_print("\r\n\033[0;31mError: GPIO is not a valid ADC1 pin! Supported: 32, 33, 34, 35, 36, 39.\033[0m\r\n");
            }
        } else {
            console_print("\r\nUsage:\r\n"
                          "  adc status         - Show status table of all analog pins\r\n"
                          "  adc read <pin>     - Read analog level of a pin\r\n"
                          "  adc <pin>          - Read analog level of a pin\r\n");
        }
    }
}

static void handle_i2c(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "scan") == 0) {
        i2c_scan();
        return;
    }

    if (strcmp(argv[1], "init") == 0) {
        if (argc < 4) {
            console_print("\r\nUsage: i2c init <sda_pin> <scl_pin>\r\n");
            return;
        }
        int sda, scl;
        if (sscanf(argv[2], "%d", &sda) == 1 && sscanf(argv[3], "%d", &scl) == 1) {
            int res = init_i2c_pins(sda, scl);
            if (res == 0) {
                console_printf("\r\n\033[0;32mI2C Bus reconfigured successfully (SDA: GPIO %d, SCL: GPIO %d)!\033[0m\r\n", sda, scl);
            }
        } else {
            console_print("\r\nUsage: i2c init <sda_pin> <scl_pin>\r\n");
        }
    } else if (strcmp(argv[1], "read") == 0) {
        if (argc < 4) {
            console_print("\r\nUsage: i2c read <hex_addr> <hex_reg>\r\n");
            return;
        }
        unsigned int addr, reg;
        if (sscanf(argv[2], "%x", &addr) == 1 && sscanf(argv[3], "%x", &reg) == 1) {
            uint8_t val = 0;
            int res = i2c_read_reg(addr, reg, &val, 1);
            if (res == 0) {
                console_printf("\r\nI2C Read [Addr: 0x%02x, Reg: 0x%02x] -> \033[1;32m0x%02x\033[0m\r\n", addr, reg, val);
            } else {
                console_print("\r\n\033[0;31mError: No response from I2C device!\033[0m\r\n");
            }
        } else {
            console_print("\r\nUsage: i2c read <hex_addr> <hex_reg>\r\n");
        }
    } else if (strcmp(argv[1], "write") == 0) {
        if (argc < 5) {
            console_print("\r\nUsage: i2c write <hex_addr> <hex_reg> <hex_val>\r\n");
            return;
        }
        unsigned int addr, reg, val;
        if (sscanf(argv[2], "%x", &addr) == 1 && sscanf(argv[3], "%x", &reg) == 1 && sscanf(argv[4], "%x", &val) == 1) {
            int res = i2c_write_reg(addr, reg, val);
            if (res == 0) {
                console_printf("\r\n\033[0;32mI2C Write Success [Addr: 0x%02x, Reg: 0x%02x] <- 0x%02x\033[0m\r\n", addr, reg, val);
            } else {
                console_print("\r\n\033[0;31mError: Failed writing to I2C device!\033[0m\r\n");
            }
        } else {
            console_print("\r\nUsage: i2c write <hex_addr> <hex_reg> <hex_val>\r\n");
        }
    } else {
        console_print("\r\nUsage:\r\n"
                      "  i2c scan                      - Scan dynamic bus address grid\r\n"
                      "  i2c init <sda> <scl>          - Configure active bus pins\r\n"
                      "  i2c read <addr> <reg>         - Read hex byte from address & register\r\n"
                      "  i2c write <addr> <reg> <val>  - Write hex byte to address & register\r\n");
    }
}

static void handle_version(int argc, char **argv) {
    console_printf("\r\nFirmware Version: %s\r\n", CLI_FIRMWARE_VERSION);
}

static void handle_restart(int argc, char **argv) {
    console_print("\r\nRestarting ESP32...\r\n");
    esp_restart();
}

static void handle_uptime(int argc, char **argv) {
    int64_t us = esp_timer_get_time();
    int seconds = (int)(us / 1000000);
    console_printf("\r\nSystem uptime: %d seconds\r\n", seconds);
}
