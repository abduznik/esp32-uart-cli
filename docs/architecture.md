# ESP32 CLI Firmware Architecture Documentation

This document describes the modular architecture of the ESP32 safety-aware UART CLI firmware. The codebase has been refactored from a monolithic implementation into a clean, decoupled component structure to improve readability, maintainability, and testing support.

---

## 📊 File Size Reduction Comparison

By isolating hardware modules, wrapping serial stream writes, and consolidating GPIO safety transactions, we achieved a **~61% code reduction** in the central CLI command coordinator (`main.c`):

| File | Before Refactoring | After Refactoring | Size Reduction | Primary Duty |
| :--- | :---: | :---: | :---: | :--- |
| **`main/main.c`** | **579 lines** | **224 lines** | **-355 lines (61%)** | Central CLI parsing engine and UART receiver loop |

---

## 🗂️ Component Directory Tree & Modules

The active codebase is laid out as follows:

```text
esp32-uart-cli/
├── Makefile                     # Host-side testing wildcard compiler script
├── CMakeLists.txt               # Root cmake workspace components build instructions
├── main/
│   ├── CMakeLists.txt           # Registers modular components
│   ├── main.c                   # CLI router engine & loop
│   ├── console.h / .c           # Generic console UART0 format printers (De-duplication wrapper)
│   ├── led.h / .c               # Onboard LED status drivers
│   ├── gpio_control.h / .c      # Secure GPIO safety transactions and status table mapping
│   └── adc_control.h / .c       # ADC1 oneshot converter calibrator and status table mapping
├── tests/
│   ├── runner                   # Compiled local host test binary
│   ├── test_runner.c            # Simulation suite managing mock registers (47 assertions)
│   └── mock_idf/                # Off-target mock environment headers
│       ├── esp_log.h
│       ├── freertos/
│       │   ├── FreeRTOS.h
│       │   └── task.h
│       ├── driver/
│       │   ├── gpio.h
│       │   └── uart.h
│       └── esp_adc/
│           └── adc_oneshot.h
└── docs/
    └── architecture.md          # This documentation
```

---

## 🛠️ Module Descriptions & Responsibilities

### 1. Central CLI Parser (`main/main.c`)
Keeps only the interactive non-blocking UART receiver character buffer and routes parsed string tokens to respective component handlers in `run_command()`. 

### 2. Console Printer Wrapper (`main/console.h`, `main/console.c`)
Provides helper functions for serial text writing:
* `void console_print(const char *str);`
* `void console_printf(const char *format, ...);`

> [!TIP]
> **De-duplication Benefit**: Rather than explicitly writing raw block sequences `uart_write_bytes(UART_PORT_NUM, msg, strlen(msg))` hundreds of times, every component simply prints to standard output via our single-call formatted wrappers.

### 3. GPIO Safe Manager (`main/gpio_control.h`, `main/gpio_control.c`)
Defines the `pin_safety_t` categorization enum and handles safety constraints. All digital reads and writes are safely filtered through transactional helper methods:
* `int read_gpio_level_safe(int pin, int *out_level);`
* `int write_gpio_level_safe(int pin, int level, char *out_msg, size_t msg_len);`

> [!IMPORTANT]
> This encapsulates hardware manipulation checks completely away from parsing strings.

### 4. ADC oneshot Converter (`main/adc_control.h`, `main/adc_control.c`)
Initializes the ADC1 unit, configures individual analog channels (pins `32, 33, 34, 35, 36, 39`) with `ADC_ATTEN_DB_11` attenuation, measures 12-bit raw conversion levels, and calculates approximate voltages in millivolts.

### 5. Onboard LED Controller (`main/led.h`, `main/led.c`)
Powers the active boot-strap LED located on `GPIO 2`.

---

## 🧪 Unified Quality Verification

Off-target simulation tests execute on standard developer environments by mocking the core hardware drivers. By executing:

```bash
make test
```

All 47 unit test assertions (ping, custom command parsing, strapping alerts, ADC voltage math, and SPI/console write limits) are automatically run in seconds to guarantee system safety.
