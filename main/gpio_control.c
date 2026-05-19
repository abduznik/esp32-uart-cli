#include "gpio_control.h"
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "console.h"

pin_safety_t get_pin_safety(int pin) {
    if (pin < 0 || pin > 39) return PIN_INVALID;
    if (pin == 1 || pin == 3) return PIN_RESTRICTED_UART;
    if (pin >= 6 && pin <= 11) return PIN_RESTRICTED_FLASH;
    if (pin == 20 || pin == 24 || (pin >= 28 && pin <= 31) || pin == 37 || pin == 38) return PIN_INVALID;
    if (pin == 34 || pin == 35 || pin == 36 || pin == 39) return PIN_INPUT_ONLY;
    if (pin == 0 || pin == 2 || pin == 5 || pin == 12 || pin == 15) return PIN_STRAPPING;
    return PIN_SAFE;
}

const char* get_pin_safety_str(pin_safety_t safety) {
    switch (safety) {
        case PIN_SAFE:             return "SAFE";
        case PIN_STRAPPING:        return "STRAPPING";
        case PIN_INPUT_ONLY:       return "INPUT-ONLY";
        case PIN_RESTRICTED_UART:  return "UART0 CONSOLE";
        case PIN_RESTRICTED_FLASH: return "SPI FLASH";
        case PIN_INVALID:          return "INVALID";
        default:                   return "UNKNOWN";
    }
}

const char* get_pin_safety_color(pin_safety_t safety) {
    switch (safety) {
        case PIN_SAFE:             return "\033[0;32m"; // Green
        case PIN_STRAPPING:        return "\033[0;33m"; // Yellow
        case PIN_INPUT_ONLY:       return "\033[0;36m"; // Cyan
        case PIN_RESTRICTED_UART:  return "\033[0;31m"; // Red
        case PIN_RESTRICTED_FLASH: return "\033[1;31m"; // Bold Red
        case PIN_INVALID:          return "\033[0;90m"; // Dark Gray
        default:                   return "\033[0m";
    }
}

const char* get_pin_desc(int pin) {
    switch (pin) {
        case 0:  return "Boot mode strap (Must be HIGH at boot for app)";
        case 1:  return "UART0 TX (Connected to USB-to-UART bridge)";
        case 2:  return "Boot strap (Must be LOW/floating) / Onboard LED";
        case 3:  return "UART0 RX (Connected to USB-to-UART bridge)";
        case 4:  return "General Purpose IO";
        case 5:  return "Strapping pin (SDIO/boot timing config)";
        case 6:  return "SPI Flash CLK (CRITICAL - DO NOT TOGGLE)";
        case 7:  return "SPI Flash SD0/MISO (CRITICAL - DO NOT TOGGLE)";
        case 8:  return "SPI Flash SD1/MOSI (CRITICAL - DO NOT TOGGLE)";
        case 9:  return "SPI Flash SD2/WP (CRITICAL - DO NOT TOGGLE)";
        case 10: return "SPI Flash SD3/HOLD (CRITICAL - DO NOT TOGGLE)";
        case 11: return "SPI Flash CMD (CRITICAL - DO NOT TOGGLE)";
        case 12: return "MTDI strap (VDD_SDIO select - DANGEROUS if HIGH)";
        case 13: return "General Purpose IO / JTAG TCK";
        case 14: return "General Purpose IO / JTAG TMS";
        case 15: return "MTDO strap (Controls boot print) / JTAG TDO";
        case 16: return "General Purpose IO";
        case 17: return "General Purpose IO";
        case 18: return "General Purpose IO / SPI SCK";
        case 19: return "General Purpose IO / SPI MISO";
        case 21: return "General Purpose IO / I2C SDA";
        case 22: return "General Purpose IO / I2C SCL";
        case 23: return "General Purpose IO / SPI MOSI";
        case 25: return "General Purpose IO / DAC1";
        case 26: return "General Purpose IO / DAC2";
        case 27: return "General Purpose IO";
        case 32: return "General Purpose IO / XTAL 32K IN";
        case 33: return "General Purpose IO / XTAL 32K OUT";
        case 34: return "Input-only pin / ADC1 CH6";
        case 35: return "Input-only pin / ADC1 CH7";
        case 36: return "Input-only pin / SENSOR VP";
        case 39: return "Input-only pin / SENSOR VN";
        default: return "Reserved or unavailable on ESP32-WROOM-32";
    }
}

void print_gpio_status(void) {
    console_print("\r\n\033[1;36m+-----+----------------------+----------------------+---------+---------------------------------------------+\033[0m\r\n"
                  "\033[1;36m| PIN | SAFETY LEVEL         | COLOR CODE           | VALUE   | FUNCTION / DESCRIPTION                      |\033[0m\r\n"
                  "\033[1;36m+-----+----------------------+----------------------+---------+---------------------------------------------+\033[0m\r\n");

    for (int pin = 0; pin <= 39; pin++) {
        pin_safety_t safety = get_pin_safety(pin);
        if (safety == PIN_INVALID) continue;

        char val_str[8] = "-";
        if (safety != PIN_RESTRICTED_FLASH) {
            snprintf(val_str, sizeof(val_str), "%d", gpio_get_level(pin));
        }

        const char *safety_str = get_pin_safety_str(safety);
        const char *color = get_pin_safety_color(safety);
        const char *desc = get_pin_desc(pin);

        console_printf("| %2d  | %s%-20s\033[0m | %s%-20s\033[0m |   %s   | %-43s |\r\n",
                       pin, color, safety_str, color, safety_str, val_str, desc);
    }

    console_print("\033[1;36m+-----+----------------------+----------------------+---------+---------------------------------------------+\033[0m\r\n"
                  "Safety Guide: \033[0;32mSAFE\033[0m = General use | \033[0;33mSTRAPPING\033[0m = Caution during boot | \033[0;36mINPUT-ONLY\033[0m = Read only\r\n"
                  "              \033[0;31mUART0 / SPI FLASH\033[0m = \033[1;31mCRITICAL SYSTEM PINS (BLOCKED TO PREVENT CRASH)\033[0m\r\n\r\n");
}

int read_gpio_level_safe(int pin, int *out_level) {
    pin_safety_t safety = get_pin_safety(pin);
    if (safety == PIN_INVALID) return -1;
    if (safety == PIN_RESTRICTED_FLASH) return -2;
    
    *out_level = gpio_get_level(pin);
    return 0;
}

int write_gpio_level_safe(int pin, int level, char *out_msg, size_t msg_len) {
    pin_safety_t safety = get_pin_safety(pin);
    
    if (safety == PIN_INVALID) {
        snprintf(out_msg, msg_len, "\r\n\033[0;31mError: Invalid pin number or not exposed on standard ESP32!\033[0m\r\n");
        return -1;
    }
    if (safety == PIN_RESTRICTED_FLASH) {
        snprintf(out_msg, msg_len, "\r\n\033[1;31mError: GPIO is reserved for SPI Flash! Writing to it will halt CPU execution and crash/bootloop the ESP32.\033[0m\r\n");
        return -2;
    }
    if (safety == PIN_RESTRICTED_UART) {
        snprintf(out_msg, msg_len, "\r\n\033[1;31mError: GPIO is UART0 Console! Overriding it will permanently disconnect your current CLI session.\033[0m\r\n");
        return -3;
    }
    if (safety == PIN_INPUT_ONLY) {
        snprintf(out_msg, msg_len, "\r\n\033[0;31mError: Pin is input-only! Standard ESP32 hardware lacks an output driver for this pin.\033[0m\r\n");
        return -4;
    }
    
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, level);
    
    if (safety == PIN_STRAPPING) {
        snprintf(out_msg, msg_len, "\r\n\033[0;33mWarning: GPIO %d is a boot strapping pin. Driving it might affect boot/flashing modes on reset!\033[0m\r\n\033[0;32mGPIO %d set to %d (Overridden Strapping Pin)\033[0m\r\n", pin, pin, level);
    } else {
        snprintf(out_msg, msg_len, "\r\nGPIO %d set to %d\r\n", pin, level);
    }
    
    return 0;
}
