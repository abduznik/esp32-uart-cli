#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include <stddef.h>

typedef enum {
    PIN_SAFE,
    PIN_STRAPPING,
    PIN_INPUT_ONLY,
    PIN_RESTRICTED_UART,
    PIN_RESTRICTED_FLASH,
    PIN_INVALID
} pin_safety_t;

pin_safety_t get_pin_safety(int pin);
const char* get_pin_safety_str(pin_safety_t safety);
const char* get_pin_safety_color(pin_safety_t safety);
const char* get_pin_desc(int pin);
void print_gpio_status(void);

// Non-duplicated secure interfaces
int read_gpio_level_safe(int pin, int *out_level);
int write_gpio_level_safe(int pin, int level, char *out_msg, size_t msg_len);

#endif
