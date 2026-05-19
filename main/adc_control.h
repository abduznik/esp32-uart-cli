#ifndef ADC_CONTROL_H
#define ADC_CONTROL_H

int read_adc_pin(int pin, int *out_raw);
void print_adc_status(void);

#endif
