#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H

#include <stdint.h>

typedef int gpio_num_t;
typedef enum {
    GPIO_MODE_DISABLE = 0,
    GPIO_MODE_INPUT = 1,
    GPIO_MODE_OUTPUT = 2,
} gpio_mode_t;

// Standard ESP-IDF GPIO functions
void gpio_reset_pin(int gpio_num);
int gpio_set_direction(int gpio_num, gpio_mode_t mode);
int gpio_set_level(int gpio_num, uint32_t level);
int gpio_get_level(int gpio_num);

#endif
