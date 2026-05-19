#ifndef LED_H
#define LED_H

#include "config.h"

#define LED_PIN CLI_LED_PIN

void init_led(void);
void led_on(void);
void led_off(void);

#endif
