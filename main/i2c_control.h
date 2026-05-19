#ifndef I2C_CONTROL_H
#define I2C_CONTROL_H

#include <stdint.h>
#include <stddef.h>

#define I2C_PORT_NUM  0 // I2C_NUM_0

void init_i2c_default(void);
int init_i2c_pins(int sda_pin, int scl_pin);
void deinit_i2c(void);
void i2c_scan(void);
int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *out_data, size_t len);
int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val);
int get_current_sda(void);
int get_current_scl(void);

#endif
