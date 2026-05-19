#ifndef MOCK_ADC_ONESHOT_H
#define MOCK_ADC_ONESHOT_H

#include <stdint.h>

// Standard ESP-IDF error definitions
#ifndef ESP_OK
#define ESP_OK 0
#endif
#define ESP_FAIL -1
typedef int esp_err_t;

typedef int adc_oneshot_unit_handle_t;
typedef enum {
    ADC_UNIT_1 = 1,
    ADC_UNIT_2 = 2,
} adc_unit_t;

typedef enum {
    ADC_CHANNEL_0 = 0,
    ADC_CHANNEL_1,
    ADC_CHANNEL_2,
    ADC_CHANNEL_3,
    ADC_CHANNEL_4,
    ADC_CHANNEL_5,
    ADC_CHANNEL_6,
    ADC_CHANNEL_7,
} adc_channel_t;

typedef adc_channel_t adc_oneshot_chan_t;

typedef enum {
    ADC_BITWIDTH_DEFAULT = 12,
} adc_bitwidth_t;

typedef enum {
    ADC_ATTEN_DB_11 = 3,
} adc_atten_t;

typedef struct {
    adc_unit_t unit_id;
    int clk_src;
} adc_oneshot_unit_init_cfg_t;

typedef struct {
    adc_atten_t atten;
    adc_bitwidth_t bitwidth;
} adc_oneshot_chan_cfg_t;

// Mocked ADC functions
esp_err_t adc_oneshot_new_unit(const adc_oneshot_unit_init_cfg_t *init_config, adc_oneshot_unit_handle_t *ret_unit);
esp_err_t adc_oneshot_config_channel(adc_oneshot_unit_handle_t handle, adc_channel_t chan, const adc_oneshot_chan_cfg_t *config);
esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t handle, adc_channel_t chan, int *out_raw);
esp_err_t adc_oneshot_del_unit(adc_oneshot_unit_handle_t handle);

#endif
