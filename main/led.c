#include "led.h"
#include "driver/gpio.h"

void init_led(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}

void led_on(void)
{
    gpio_set_level(LED_PIN, 1);
}

void led_off(void)
{
    gpio_set_level(LED_PIN, 0);
}
