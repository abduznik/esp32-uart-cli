#ifndef MOCK_I2C_H
#define MOCK_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define I2C_NUM_0 0
#define I2C_MASTER_WRITE 0
#define I2C_MASTER_READ 1
#define I2C_MODE_MASTER 0

typedef int i2c_port_t;
typedef int i2c_mode_t;
typedef void* i2c_cmd_handle_t;

typedef struct {
    i2c_mode_t mode;
    int sda_io_num;
    int scl_io_num;
    bool sda_pullup_en;
    bool scl_pullup_en;
    struct {
        uint32_t clk_speed;
    } master;
} i2c_config_t;

// Mock interfaces
int i2c_param_config(i2c_port_t i2c_num, const i2c_config_t *i2c_conf);
int i2c_driver_install(i2c_port_t i2c_num, i2c_mode_t mode, size_t slv_rx_buf_len, size_t slv_tx_buf_len, int intr_alloc_flags);
int i2c_driver_delete(i2c_port_t i2c_num);

i2c_cmd_handle_t i2c_cmd_link_create(void);
int i2c_master_start(i2c_cmd_handle_t cmd);
int i2c_master_write_byte(i2c_cmd_handle_t cmd, uint8_t data, bool ack_en);
int i2c_master_stop(i2c_cmd_handle_t cmd);
int i2c_master_cmd_begin(i2c_port_t i2c_num, i2c_cmd_handle_t cmd, int ticks_to_wait);
void i2c_cmd_link_delete(i2c_cmd_handle_t cmd);

// Simulated responsive address registry
extern uint8_t mock_i2c_devices[128];

#endif
