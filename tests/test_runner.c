#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/i2c.h"
#include "esp_system.h"
#include "esp_timer.h"

// Simulated system state
bool mock_restart_called = false;
int64_t mock_uptime_us = 120000000; // 120 seconds in microseconds

void esp_restart(void) {
    mock_restart_called = true;
}

int64_t esp_timer_get_time(void) {
    return mock_uptime_us;
}

// Simulated I2C state
uint8_t mock_i2c_devices[128] = {0};
static int mock_i2c_sda = -1;
static int mock_i2c_scl = -1;
static int mock_i2c_active = 0;
static uint8_t mock_current_cmd_addr = 0;

int i2c_param_config(i2c_port_t i2c_num, const i2c_config_t *i2c_conf) {
    mock_i2c_sda = i2c_conf->sda_io_num;
    mock_i2c_scl = i2c_conf->scl_io_num;
    return 0; // ESP_OK
}

int i2c_driver_install(i2c_port_t i2c_num, i2c_mode_t mode, size_t slv_rx_buf_len, size_t slv_tx_buf_len, int intr_alloc_flags) {
    mock_i2c_active = 1;
    return 0; // ESP_OK
}

int i2c_driver_delete(i2c_port_t i2c_num) {
    mock_i2c_active = 0;
    return 0; // ESP_OK
}

i2c_cmd_handle_t i2c_cmd_link_create(void) {
    return (i2c_cmd_handle_t)1;
}

static bool mock_start_sent = false;

int i2c_master_start(i2c_cmd_handle_t cmd) {
    mock_start_sent = true;
    return 0;
}

int i2c_master_write_byte(i2c_cmd_handle_t cmd, uint8_t data, bool ack_en) {
    if (mock_start_sent) {
        mock_current_cmd_addr = data >> 1;
        mock_start_sent = false;
    }
    return 0;
}

int i2c_master_stop(i2c_cmd_handle_t cmd) {
    return 0;
}

int i2c_master_cmd_begin(i2c_port_t i2c_num, i2c_cmd_handle_t cmd, int ticks_to_wait) {
    if (mock_i2c_active && mock_current_cmd_addr < 128 && mock_i2c_devices[mock_current_cmd_addr]) {
        return 0; // ESP_OK
    }
    return -1; // ESP_FAIL
}

void i2c_cmd_link_delete(i2c_cmd_handle_t cmd) {
}

// Simulated ADC state
int mock_adc_values[8] = {100, 200, 300, 400, 1000, 2000, 3000, 4000};

esp_err_t adc_oneshot_new_unit(const adc_oneshot_unit_init_cfg_t *init_config, adc_oneshot_unit_handle_t *ret_unit) {
    *ret_unit = 1;
    return 0; // ESP_OK
}

esp_err_t adc_oneshot_config_channel(adc_oneshot_unit_handle_t handle, adc_oneshot_chan_t chan, const adc_oneshot_chan_cfg_t *config) {
    return 0; // ESP_OK
}

esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t handle, adc_oneshot_chan_t chan, int *out_raw) {
    if (chan >= 0 && chan < 8) {
        *out_raw = mock_adc_values[chan];
        return 0; // ESP_OK
    }
    return -1; // ESP_FAIL
}

esp_err_t adc_oneshot_del_unit(adc_oneshot_unit_handle_t handle) {
    return 0; // ESP_OK
}

// Simulated ESP32 Hardware State
uint32_t mock_gpio_levels[40] = {0};
gpio_mode_t mock_gpio_modes[40] = {GPIO_MODE_DISABLE};
bool mock_gpio_resets[40] = {false};

char uart0_output_buf[16384] = {0};
size_t uart0_output_len = 0;

void clear_uart_buf(void) {
    memset(uart0_output_buf, 0, sizeof(uart0_output_buf));
    uart0_output_len = 0;
}

// GPIO Mock Implementations
void gpio_reset_pin(int gpio_num) {
    if (gpio_num >= 0 && gpio_num < 40) {
        mock_gpio_resets[gpio_num] = true;
        mock_gpio_levels[gpio_num] = 0;
        mock_gpio_modes[gpio_num] = GPIO_MODE_DISABLE;
    }
}

int gpio_set_direction(int gpio_num, gpio_mode_t mode) {
    if (gpio_num >= 0 && gpio_num < 40) {
        mock_gpio_modes[gpio_num] = mode;
        return 0;
    }
    return -1;
}

int gpio_set_level(int gpio_num, uint32_t level) {
    if (gpio_num >= 0 && gpio_num < 40) {
        mock_gpio_levels[gpio_num] = level;
        return 0;
    }
    return -1;
}

int gpio_get_level(int gpio_num) {
    if (gpio_num >= 0 && gpio_num < 40) {
        return mock_gpio_levels[gpio_num];
    }
    return 0;
}

// UART Mock Implementations
int uart_param_config(uart_port_t uart_num, const uart_config_t *uart_config) {
    return 0;
}

int uart_set_pin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num) {
    return 0;
}

int uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, void *uart_queue, int intr_alloc_flags) {
    return 0;
}

int uart_write_bytes(uart_port_t uart_num, const void *src, size_t size) {
    if (uart_num == UART_NUM_0) {
        if (uart0_output_len + size < sizeof(uart0_output_buf) - 1) {
            memcpy(uart0_output_buf + uart0_output_len, src, size);
            uart0_output_len += size;
            uart0_output_buf[uart0_output_len] = '\0';
        }
    }
    return size;
}

int uart_read_bytes(uart_port_t uart_num, void *buf, uint32_t length, int ticks_to_wait) {
    return 0;
}

// Declare CLI firmware target function under test
extern void run_command(char *cmd);

// Assert macro helpers
int tests_run = 0;
int tests_failed = 0;

#define ASSERT_TRUE(condition, message) \
    do { \
        tests_run++; \
        if (!(condition)) { \
            printf("  \033[0;31m[FAIL]\033[0m %s:%d: %s\n", __FILE__, __LINE__, message); \
            tests_failed++; \
        } else { \
            printf("  \033[0;32m[PASS]\033[0m %s\n", message); \
        } \
    } while(0)

#define ASSERT_SUBSTR(str, sub, message) \
    ASSERT_TRUE(strstr(str, sub) != NULL, message)

#define ASSERT_NOT_SUBSTR(str, sub, message) \
    ASSERT_TRUE(strstr(str, sub) == NULL, message)

// ----------------------------------------------------
// Test Cases
// ----------------------------------------------------

void test_ping(void) {
    printf("\n[Test 1] Testing 'ping' command...\n");
    clear_uart_buf();
    run_command("ping");
    ASSERT_SUBSTR(uart0_output_buf, "pong!", "Ping command should reply with 'pong!'");
}

void test_help(void) {
    printf("\n[Test 2] Testing 'help' command...\n");
    clear_uart_buf();
    run_command("help");
    ASSERT_SUBSTR(uart0_output_buf, "Available Commands:", "Help should print standard console list");
    ASSERT_SUBSTR(uart0_output_buf, "gpio status", "Help should document status listing command");
    ASSERT_SUBSTR(uart0_output_buf, "gpio read", "Help should document status read command");
    ASSERT_SUBSTR(uart0_output_buf, "adc status", "Help should document adc status command");
    ASSERT_SUBSTR(uart0_output_buf, "adc read", "Help should document adc read command");
    ASSERT_SUBSTR(uart0_output_buf, "i2c scan", "Help should document i2c scan command");
    ASSERT_SUBSTR(uart0_output_buf, "i2c init", "Help should document i2c init command");
}

void test_led(void) {
    printf("\n[Test 3] Testing onboard LED commands...\n");
    // LED pin is GPIO 2
    clear_uart_buf();
    run_command("led on");
    ASSERT_SUBSTR(uart0_output_buf, "LED is ON", "Command returns correct output status");
    ASSERT_TRUE(mock_gpio_levels[2] == 1, "GPIO 2 hardware line must be set high");

    clear_uart_buf();
    run_command("led off");
    ASSERT_SUBSTR(uart0_output_buf, "LED is OFF", "Command returns correct output status");
    ASSERT_TRUE(mock_gpio_levels[2] == 0, "GPIO 2 hardware line must be set low");
}

void test_gpio_safe(void) {
    printf("\n[Test 4] Testing safe General Purpose IO (GPIO 4)...\n");
    clear_uart_buf();
    mock_gpio_levels[4] = 0;
    run_command("gpio 4 1");
    ASSERT_SUBSTR(uart0_output_buf, "GPIO 4 set to 1", "Firmware prints confirmation when GPIO configured successfully");
    ASSERT_TRUE(mock_gpio_levels[4] == 1, "GPIO 4 hardware register level set to 1");
    ASSERT_TRUE(mock_gpio_modes[4] == GPIO_MODE_OUTPUT, "GPIO 4 hardware register mode set to Output");

    // Test 'set' keyword syntax
    clear_uart_buf();
    run_command("gpio set 4 0");
    ASSERT_SUBSTR(uart0_output_buf, "GPIO 4 set to 0", "Firmware prints confirmation using alternative 'gpio set' syntax");
    ASSERT_TRUE(mock_gpio_levels[4] == 0, "GPIO 4 hardware register level cleared back to 0");
}

void test_gpio_strapping(void) {
    printf("\n[Test 5] Testing Boot Strapping Pin (GPIO 12)...\n");
    clear_uart_buf();
    mock_gpio_levels[12] = 0;
    run_command("gpio 12 1");
    ASSERT_SUBSTR(uart0_output_buf, "Warning: GPIO 12 is a boot strapping pin", "Warning is printed on strapping pins");
    ASSERT_SUBSTR(uart0_output_buf, "Overridden Strapping Pin", "Toggling strapping pin is ultimately allowed");
    ASSERT_TRUE(mock_gpio_levels[12] == 1, "GPIO 12 hardware state successfully configured to 1");
}

void test_gpio_input_only(void) {
    printf("\n[Test 6] Testing Input-Only Pin write blockage (GPIO 35)...\n");
    clear_uart_buf();
    mock_gpio_levels[35] = 0;
    mock_gpio_modes[35] = GPIO_MODE_INPUT;
    run_command("gpio 35 1");
    ASSERT_SUBSTR(uart0_output_buf, "Error: Pin is input-only!", "Error is printed explaining the pin has no output driver");
    ASSERT_TRUE(mock_gpio_levels[35] == 0, "Input-only GPIO 35 state must remain unchanged");
    ASSERT_TRUE(mock_gpio_modes[35] == GPIO_MODE_INPUT, "Input-only GPIO 35 mode must remain Input");
}

void test_gpio_critical_flash(void) {
    printf("\n[Test 7] Testing Critical SPI Flash Pin write block (GPIO 8)...\n");
    clear_uart_buf();
    mock_gpio_levels[8] = 0;
    run_command("gpio 8 1");
    ASSERT_SUBSTR(uart0_output_buf, "Error: GPIO is reserved for SPI Flash!", "Error is printed explaining that writing to flash pins crashes CPU");
    ASSERT_TRUE(mock_gpio_levels[8] == 0, "SPI Flash pin level remains strictly unchanged to prevent bricking/crashes");
}

void test_gpio_critical_uart(void) {
    printf("\n[Test 8] Testing Critical UART0 console Pin write block (GPIO 1)...\n");
    clear_uart_buf();
    mock_gpio_levels[1] = 0;
    run_command("gpio 1 1");
    ASSERT_SUBSTR(uart0_output_buf, "Error: GPIO is UART0 Console!", "Error is printed explaining that writing to console disconnects CLI");
    ASSERT_TRUE(mock_gpio_levels[1] == 0, "UART0 TX pin level remains strictly unchanged to prevent CLI disconnection");
}

void test_gpio_read(void) {
    printf("\n[Test 9] Testing 'gpio read' sub-command...\n");
    
    // Read safe GPIO 4 driven high
    clear_uart_buf();
    mock_gpio_levels[4] = 1;
    run_command("gpio read 4");
    ASSERT_SUBSTR(uart0_output_buf, "GPIO 4 state:", "Prints state query response");
    ASSERT_SUBSTR(uart0_output_buf, "1", "Successfully detects high (1) state");
    ASSERT_SUBSTR(uart0_output_buf, "SAFE", "Identifies safety level status");

    // Read input-only GPIO 39 driven low
    clear_uart_buf();
    mock_gpio_levels[39] = 0;
    run_command("gpio read 39");
    ASSERT_SUBSTR(uart0_output_buf, "0", "Successfully detects low (0) state");
    ASSERT_SUBSTR(uart0_output_buf, "INPUT-ONLY", "Identifies input-only safety rating");

    // Try reading critical Flash pin (should be blocked)
    clear_uart_buf();
    run_command("gpio read 7");
    ASSERT_SUBSTR(uart0_output_buf, "Error:", "Blocks reading reserved SPI Flash lines");
}

void test_gpio_status(void) {
    printf("\n[Test 10] Testing 'gpio status' table output...\n");
    clear_uart_buf();
    run_command("gpio status");
    ASSERT_SUBSTR(uart0_output_buf, "PIN", "Table shows 'PIN' column header");
    ASSERT_SUBSTR(uart0_output_buf, "SAFETY LEVEL", "Table shows 'SAFETY LEVEL' column header");
    ASSERT_SUBSTR(uart0_output_buf, "SPI FLASH", "Table rows contain SPI FLASH classification");
    ASSERT_SUBSTR(uart0_output_buf, "UART0 CONSOLE", "Table rows contain UART0 console lines");
    ASSERT_SUBSTR(uart0_output_buf, "STRAPPING", "Table rows contain strapping pins");
    ASSERT_SUBSTR(uart0_output_buf, "INPUT-ONLY", "Table rows contain input-only pins");
}

void test_adc_read(void) {
    printf("\n[Test 11] Testing 'adc read' and shortcut commands...\n");

    // Read valid GPIO 32 (maps to ADC1_CH4, value 1000)
    clear_uart_buf();
    run_command("adc read 32");
    ASSERT_SUBSTR(uart0_output_buf, "GPIO 32 (ADC1 Channel): Raw =", "Successfully outputs read status");
    ASSERT_SUBSTR(uart0_output_buf, "1000", "Reads mock channel value of 1000");
    ASSERT_SUBSTR(uart0_output_buf, "757 mV", "Correctly estimates voltage conversion (1000 * 3100 / 4095 = 757 mV)");

    // Test shortcut read syntax "adc <pin>" on valid GPIO 34 (maps to ADC1_CH6, value 3000)
    clear_uart_buf();
    run_command("adc 34");
    ASSERT_SUBSTR(uart0_output_buf, "GPIO 34 (ADC1 Channel): Raw =", "Shortcut syntax triggers successful output");
    ASSERT_SUBSTR(uart0_output_buf, "3000", "Reads mock channel value of 3000");

    // Test invalid GPIO (e.g. pin 4 which is general IO, not ADC)
    clear_uart_buf();
    run_command("adc 4");
    ASSERT_SUBSTR(uart0_output_buf, "Error: GPIO is not a valid ADC1 pin!", "Error is printed explaining pin is not ADC capable");
}

void test_adc_status(void) {
    printf("\n[Test 12] Testing 'adc status' table output...\n");
    clear_uart_buf();
    run_command("adc status");
    ASSERT_SUBSTR(uart0_output_buf, "ADC CHANNEL", "Table shows 'ADC CHANNEL' column header");
    ASSERT_SUBSTR(uart0_output_buf, "ESTIMATED VOLTAGE", "Table shows 'ESTIMATED VOLTAGE' column header");
    ASSERT_SUBSTR(uart0_output_buf, "ADC1_CH4", "Table shows CH4 mapping");
    ASSERT_SUBSTR(uart0_output_buf, "ADC1_CH0", "Table shows CH0 mapping");
}

void test_i2c(void) {
    printf("\n[Test 13] Testing 'i2c scan' and dynamic controller configurations...\n");

    // Initialize Mock registry: device at 0x3C and 0x68
    memset(mock_i2c_devices, 0, sizeof(mock_i2c_devices));
    mock_i2c_devices[0x3C] = 1;
    mock_i2c_devices[0x68] = 1;

    // Scan dynamic bus default pins 21/22
    clear_uart_buf();
    run_command("i2c scan");
    ASSERT_SUBSTR(uart0_output_buf, "Scanning I2C Bus", "Prints scanning banner header");
    ASSERT_SUBSTR(uart0_output_buf, "3c", "Successfully prints active responsive device 0x3C");
    ASSERT_SUBSTR(uart0_output_buf, "68", "Successfully prints active responsive device 0x68");
    ASSERT_SUBSTR(uart0_output_buf, "Found 2 active devices", "Scanner grid summarizes found device count");

    // Reconfigure to safe dynamic pins 4 (SDA) and 5 (SCL)
    clear_uart_buf();
    run_command("i2c init 4 5");
    ASSERT_SUBSTR(uart0_output_buf, "I2C Bus reconfigured successfully", "Confirms pins reconfigured successfully");
    ASSERT_TRUE(mock_i2c_sda == 4, "Underlying register updated with SDA GPIO 4");
    ASSERT_TRUE(mock_i2c_scl == 5, "Underlying register updated with SCL GPIO 5");

    // Attempt to reconfigure to restricted console UART0 TX (GPIO 1) - Must be BLOCKED safely!
    clear_uart_buf();
    run_command("i2c init 1 5");
    ASSERT_SUBSTR(uart0_output_buf, "Error: SDA Pin 1 is restricted or invalid!", "Blocks using UART0 pins to prevent crash");
    // SDA should NOT be set to 1
    ASSERT_TRUE(mock_i2c_sda == 4, "Underlying SDA register remains protected at 4");

    // Attempt to reconfigure to critical SPI Flash pin (GPIO 8) - Must be BLOCKED safely!
    clear_uart_buf();
    run_command("i2c init 4 8");
    ASSERT_SUBSTR(uart0_output_buf, "Error: SCL Pin 8 is restricted or invalid!", "Blocks using Flash pins to prevent crash");
    ASSERT_TRUE(mock_i2c_scl == 5, "Underlying SCL register remains protected at 5");

    // Test Hex read/write routines
    clear_uart_buf();
    run_command("i2c read 3c 10");
    ASSERT_SUBSTR(uart0_output_buf, "I2C Read [Addr: 0x3c, Reg: 0x10]", "Reads hex byte successfully");

    clear_uart_buf();
    run_command("i2c write 3c 10 aa");
    ASSERT_SUBSTR(uart0_output_buf, "I2C Write Success [Addr: 0x3c, Reg: 0x10] <- 0xaa", "Writes hex byte successfully");
}

void test_system_commands(void) {
    printf("\n[Test 14] Testing 'version', 'uptime', and 'restart' commands...\n");

    // Test version command
    clear_uart_buf();
    run_command("version");
    ASSERT_SUBSTR(uart0_output_buf, "Firmware Version: v1.0.2", "Version command prints correct firmware tag");

    // Test uptime command
    clear_uart_buf();
    run_command("uptime");
    ASSERT_SUBSTR(uart0_output_buf, "System uptime: 120 seconds", "Uptime command computes and prints duration");

    // Test restart command
    clear_uart_buf();
    mock_restart_called = false;
    run_command("restart");
    ASSERT_SUBSTR(uart0_output_buf, "Restarting ESP32...", "Restart prints pre-reset announcement");
    ASSERT_TRUE(mock_restart_called, "esp_restart() was invoked by CLI");
}

// ----------------------------------------------------
// Main Test Runner Entrypoint
// ----------------------------------------------------

int main(int argc, char **argv) {
    printf("\033[1;36m==================================================\033[0m\n");
    printf("\033[1;36m      RUNNING FIRMWARE CLI HOST-SIDE TESTS        \033[0m\n");
    printf("\033[1;36m==================================================\033[0m\n");

    test_ping();
    test_help();
    test_led();
    test_gpio_safe();
    test_gpio_strapping();
    test_gpio_input_only();
    test_gpio_critical_flash();
    test_gpio_critical_uart();
    test_gpio_read();
    test_gpio_status();
    test_adc_read();
    test_adc_status();
    test_i2c();
    test_system_commands();

    printf("\n\033[1;36m==================================================\033[0m\n");
    if (tests_failed == 0) {
        printf("\033[1;32mALL %d ASSERTIONS PASSED SUCCESSFULLY! 🎉\033[0m\n", tests_run);
        printf("\033[1;32mFirmware GPIO controls are safe, robust, and verified!\033[0m\n");
    } else {
        printf("\033[1;31mTEST SUITE FAILED: %d/%d ASSERTIONS FAILED 💥\033[0m\n", tests_failed, tests_run);
    }
    printf("\033[1;36m==================================================\033[0m\n");

    return tests_failed == 0 ? 0 : 1;
}
