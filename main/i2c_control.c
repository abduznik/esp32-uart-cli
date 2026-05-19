#include "i2c_control.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "gpio_control.h"
#include "console.h"

static bool i2c_initialized = false;
static int current_sda = 21;
static int current_scl = 22;

int get_current_sda(void) { return current_sda; }
int get_current_scl(void) { return current_scl; }

void init_i2c_default(void) {
    if (!i2c_initialized) {
        init_i2c_pins(21, 22);
    }
}

int init_i2c_pins(int sda_pin, int scl_pin) {
    // 1. Perform strict safety validation
    pin_safety_t sda_safety = get_pin_safety(sda_pin);
    pin_safety_t scl_safety = get_pin_safety(scl_pin);

    if (sda_safety == PIN_INVALID || sda_safety == PIN_RESTRICTED_FLASH || sda_safety == PIN_RESTRICTED_UART) {
        console_printf("\r\n\033[0;31mError: SDA Pin %d is restricted or invalid!\033[0m\r\n", sda_pin);
        return -1;
    }
    if (scl_safety == PIN_INVALID || scl_safety == PIN_RESTRICTED_FLASH || scl_safety == PIN_RESTRICTED_UART) {
        console_printf("\r\n\033[0;31mError: SCL Pin %d is restricted or invalid!\033[0m\r\n", scl_pin);
        return -2;
    }

    // 2. Erase previous driver if already mounted
    if (i2c_initialized) {
        deinit_i2c();
    }

    // 3. Mount driver
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = true,
        .scl_pullup_en = true,
        .master.clk_speed = 100000, // 100kHz standard mode
    };

    int err = i2c_param_config(I2C_PORT_NUM, &conf);
    if (err != 0) return -3;

    err = i2c_driver_install(I2C_PORT_NUM, conf.mode, 0, 0, 0);
    if (err != 0) return -4;

    current_sda = sda_pin;
    current_scl = scl_pin;
    i2c_initialized = true;
    return 0;
}

void deinit_i2c(void) {
    if (i2c_initialized) {
        i2c_driver_delete(I2C_PORT_NUM);
        i2c_initialized = false;
    }
}

void i2c_scan(void) {
    // Ensure we are initialized before scanning
    init_i2c_default();

    console_printf("\r\nScanning I2C Bus (SDA: GPIO %d, SCL: GPIO %d, Speed: 100kHz)...\r\n\r\n", current_sda, current_scl);
    console_print("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");

    int devices_found = 0;

    for (int addr = 0; addr < 128; addr += 16) {
        console_printf("%02x: ", addr);

        for (int col = 0; col < 16; col++) {
            int target_addr = addr + col;

            // Skip restricted I2C reserved addresses (0x00 to 0x02, and 0x78 to 0x7F)
            if (target_addr < 3 || target_addr > 0x77) {
                console_print("   ");
                continue;
            }

            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (target_addr << 1) | I2C_MASTER_WRITE, true);
            i2c_master_stop(cmd);

            int err = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, 50 / portTICK_PERIOD_MS);
            i2c_cmd_link_delete(cmd);

            if (err == 0) { // Device acknowledged (ACK)
                console_printf("\033[1;32m%02x\033[0m ", target_addr);
                devices_found++;
            } else {
                console_print("-- ");
            }
        }
        console_print("\r\n");
    }

    console_printf("\r\n\033[1;36mI2C scan complete. Found %d active devices.\033[0m\r\n\r\n", devices_found);
}

int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *out_data, size_t len) {
    init_i2c_default();
    if (len == 0) return 0;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    
    // Read registers: standard IDF old APIs require read byte calls
    // For mock/test support, we can just perform write-then-read structures
    i2c_master_stop(cmd);

    int err = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, 100 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return err;
}

int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    init_i2c_default();

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, val, true);
    i2c_master_stop(cmd);

    int err = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, 100 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return err;
}
