#include "adc_control.h"
#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "console.h"

int read_adc_pin(int pin, int *out_raw) {
    adc_channel_t channel;
    switch (pin) {
        case 36: channel = ADC_CHANNEL_0; break;
        case 39: channel = ADC_CHANNEL_3; break;
        case 32: channel = ADC_CHANNEL_4; break;
        case 33: channel = ADC_CHANNEL_5; break;
        case 34: channel = ADC_CHANNEL_6; break;
        case 35: channel = ADC_CHANNEL_7; break;
        default: return -1; // Not a valid ADC1 pin
    }
    
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_config1, &adc1_handle);
    if (err != ESP_OK) return -2;
    
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    err = adc_oneshot_config_channel(adc1_handle, channel, &config);
    if (err != ESP_OK) {
        adc_oneshot_del_unit(adc1_handle);
        return -3;
    }
    
    int raw_val = 0;
    err = adc_oneshot_read(adc1_handle, channel, &raw_val);
    
    adc_oneshot_del_unit(adc1_handle);
    
    if (err != ESP_OK) return -4;
    
    *out_raw = raw_val;
    return 0;
}

void print_adc_status(void) {
    console_print("\r\n\033[1;35m+-----+-------------+-----------+-------------------------+\033[0m\r\n"
                  "\033[1;35m| PIN | ADC CHANNEL | RAW VALUE | ESTIMATED VOLTAGE (mV)  |\033[0m\r\n"
                  "\033[1;35m+-----+-------------+-----------+-------------------------+\033[0m\r\n");

    int adc_pins[] = {32, 33, 34, 35, 36, 39};
    const char* channels[] = {"ADC1_CH4", "ADC1_CH5", "ADC1_CH6", "ADC1_CH7", "ADC1_CH0", "ADC1_CH3"};

    for (int i = 0; i < 6; i++) {
        int pin = adc_pins[i];
        int raw_val = 0;
        char raw_str[16] = "-";
        char volt_str[32] = "-";
        
        if (read_adc_pin(pin, &raw_val) == 0) {
            snprintf(raw_str, sizeof(raw_str), "%4d", raw_val);
            int mv = (raw_val * 3100) / 4095;
            snprintf(volt_str, sizeof(volt_str), "%4d mV (~%.2f V)", mv, mv / 1000.0);
        }

        console_printf("|  %2d | \033[0;36m%-11s\033[0m |    %s   | \033[0;32m%-23s\033[0m |\r\n",
                       pin, channels[i], raw_str, volt_str);
    }

    console_print("\033[1;35m+-----+-------------+-----------+-------------------------+\033[0m\r\n"
                  "Note: Readings configured with ADC_ATTEN_DB_11 (covers 0 - 3.1V range).\r\n\r\n");
}
