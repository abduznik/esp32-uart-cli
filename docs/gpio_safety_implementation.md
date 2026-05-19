# ESP32 Safety-Aware GPIO Firmware Addition

We have integrated a comprehensive, hardware-safe, and visual GPIO control system into your ESP32 UART CLI firmware. This ensures that you can safely toggle and inspect pins without accidentally bricking or halting the ESP32 (due to core conflicts with the SPI Flash or UART lines).

---

## 🛡️ Pin Safety Categorization System

Every possible ESP32 GPIO pin (0–39) is now governed by an active safety categorization system:

| Pin Type | GPIO Pins | Safety Level | CLI Action | Details / Potential Risks |
| :--- | :--- | :--- | :--- | :--- |
| **SPI Flash** | `6`, `7`, `8`, `9`, `10`, `11` | 🔴 **CRITICAL / SPI FLASH** | **BLOCKED** | Directly connected to the onboard SPI Flash containing the firmware. Driving or toggling these halts CPU execution and triggers instant crash loops. |
| **UART0 Console** | `1`, `3` | 🔴 **CRITICAL / UART0** | **BLOCKED** | Reserved for active serial CLI communication (TX/RX). Toggling these will permanently disconnect your active CLI session. |
| **Input-Only** | `34`, `35`, `36`, `39` | 🔵 **INPUT-ONLY** | **BLOCKED** *(for Output)* | Standard ESP32 hardware lacks an output driver on these pins. They can only be read (`gpio read`). |
| **Strapping Pins** | `0`, `2`, `5`, `12`, `15` | 🟡 **STRAPPING PIN** | **ALLOWED** *(with warning)* | Sampled at boot to determine flash voltage, boot mode, etc. Allowed to toggle, but prints a caution about potential boot/flashing failures. |
| **General Purpose** | `4`, `13`, `14`, `16`, `17`, `18`, `19`, `21`, `22`, `23`, `25`, `26`, `27`, `32`, `33` | 🟢 **SAFE** | **ALLOWED** | Safe for general-purpose digital input, output, or PWM. |
| **Invalid / Reserved**| `20`, `24`, `28-31`, `37`, `38` | ⚫ **INVALID** | **BLOCKED** | Pins do not exist or are not exposed on standard ESP32-WROOM modules. |

---

## ⚡ Added CLI Commands

The `gpio` command group has been greatly expanded with the following interfaces:

### 1. `gpio status` (or `gpio list`)
Prints a beautiful, color-coded ANSI terminal table of all exposed ESP32 pins, their safety ratings, active logic levels, and detailed descriptions:

```text
+-----+----------------------+----------------------+---------+---------------------------------------------+
| PIN | SAFETY LEVEL         | COLOR CODE           | VALUE   | FUNCTION / DESCRIPTION                      |
+-----+----------------------+----------------------+---------+---------------------------------------------+
|  0  | STRAPPING            | STRAPPING            |   1     | Boot mode strap (Must be HIGH at boot)      |
|  1  | UART0 CONSOLE        | UART0 CONSOLE        |   1     | UART0 TX (Connected to USB bridge)          |
|  2  | STRAPPING            | STRAPPING            |   0     | Boot strap / Onboard LED                    |
|  3  | UART0 CONSOLE        | UART0 CONSOLE        |   1     | UART0 RX (Connected to USB bridge)          |
|  4  | SAFE                 | SAFE                 |   0     | General Purpose IO                          |
|  5  | STRAPPING            | STRAPPING            |   1     | Strapping pin (SDIO/boot timing config)     |
| 12  | STRAPPING            | STRAPPING            |   0     | MTDI strap (VDD_SDIO select - DANGER if HI) |
...
```

### 2. `gpio read <pin>`
Reads and displays the current logical state (`0` or `1`) of any exposed pin without changing its input/output mode:
```bash
> gpio read 4
GPIO 4 state: 1 (SAFE)
```

### 3. `gpio <pin> <0/1>` & `gpio set <pin> <0/1>`
Safely configures the pin as a digital output and drives it `HIGH` (`1`) or `LOW` (`0`).

*   **Toggling a SAFE Pin:**
    ```bash
    > gpio set 4 1
    GPIO 4 set to 1
    ```
*   **Toggling a STRAPPING Pin:**
    ```bash
    > gpio set 12 1
    Warning: GPIO 12 is a boot strapping pin. Driving it might affect boot/flashing modes on reset!
    GPIO 12 set to 1 (Overridden Strapping Pin)
    ```
*   **Toggling a BLOCKED Pin:**
    ```bash
    > gpio set 8 1
    Error: GPIO is reserved for SPI Flash! Writing to it will halt CPU execution and crash/bootloop the ESP32.
    ```
    ```bash
    > gpio set 35 1
    Error: Pin is input-only! Standard ESP32 hardware lacks an output driver for this pin.
    ```

---

## 🛠️ Summary of Changes in `main.c`

1.  **Header Imports**: Added `#include <stdbool.h>` and `#include <stdlib.h>` to allow clean logical evaluation and host-based compilation.
2.  **Safety Enums & Helpers**: 
    *   Defined `pin_safety_t` representing the safety states.
    *   Implemented `get_pin_safety(int pin)` to dynamically check pins.
    *   Added descriptive metadata lookup in `get_pin_desc(int pin)`.
3.  **Visual Status Table Generator**: Implemented `print_gpio_status()` to output a high-fidelity visual representation of the micro-controller's active pinout.
4.  **Robust Parser Upgrades**: Replaced the simple parsing block in `run_command()` with sub-command token routing, validating boundary cases and enforcing strict hardware guidelines.

---

## 🧪 Host-Side Test Suite & Simulation

Because compiling and running unit tests on embedded target hardware normally requires a full JTAG link, QEMU emulator, or flashing process, we developed an **off-target mock testing architecture** under `tests/`.

### How It Works
1.  **Mocks Directory (`tests/mock_idf/`)**: Contains bare-metal headers mapping exact signatures of FreeRTOS, GPIO driver, UART driver, and ESP logger libraries.
2.  **Test Runner (`tests/test_runner.c`)**:
    *   Simulates ESP32 CPU registers through dynamic state structures (`mock_gpio_levels`, `mock_gpio_modes`).
    *   Intercepts internal memory and standard serial buffers (`uart0_output_buf`).
    *   Implements **10 isolated test cases comprising 35 active assertions** testing boundary values, invalid addresses, critical blocks, console locks, and dynamic status table printing.

### Executing Tests Locally

A root `Makefile` has been added for clean and generic test automation. It uses dynamic source-code discovery (`wildcard`) so that **any new C test files you add under the `tests/` directory are automatically discovered and compiled** into the test binary without needing to update compilation scripts.

To clean, compile, and run the entire suite locally on macOS (or Linux):

```bash
make test
```

*Alternative (Direct Compiling without Make):*
```bash
clang -O2 -Itests/mock_idf -Dapp_main=firmware_app_main main/main.c tests/test_runner.c -o tests/runner && ./tests/runner
```

#### Expected Test Output:
```text
==================================================
      RUNNING FIRMWARE CLI HOST-SIDE TESTS        
==================================================

[Test 1] Testing 'ping' command...
  [PASS] Ping command should reply with 'pong!'

...

[Test 10] Testing 'gpio status' table output...
  [PASS] Table shows 'PIN' column header
  [PASS] Table shows 'SAFETY LEVEL' column header
  [PASS] Table rows contain SPI FLASH classification
  [PASS] Table rows contain UART0 console lines
  [PASS] Table rows contain strapping pins
  [PASS] Table rows contain input-only pins

[Test 11] Testing 'adc read' and shortcut commands...
  [PASS] Successfully outputs read status
  [PASS] Reads mock channel value of 1000
  [PASS] Correctly estimates voltage conversion (1000 * 3100 / 4095 = 757 mV)
  [PASS] Shortcut syntax triggers successful output
  [PASS] Reads mock channel value of 3000
  [PASS] Error is printed explaining pin is not ADC capable

[Test 12] Testing 'adc status' table output...
  [PASS] Table shows 'ADC CHANNEL' column header
  [PASS] Table shows 'ESTIMATED VOLTAGE' column header
  [PASS] Table shows CH4 mapping
  [PASS] Table shows CH0 mapping

==================================================
ALL 47 ASSERTIONS PASSED SUCCESSFULLY! 🎉
Firmware GPIO controls are safe, robust, and verified!
==================================================
```

---

## 🔌 Analog to Digital Converter (ADC) Controls

We added support for the ESP32's built-in **12-bit ADC1 peripheral (Channels 0 to 7)**. This allows you to measure analog voltages on physical lines in real-time from the CLI.

### 📐 Attenuation & Voltage Mapping
* **API Version**: Uses Espressif's modern `esp_adc/adc_oneshot.h` driver (recommended for ESP-IDF v5.x+).
* **Configured Attenuation**: `ADC_ATTEN_DB_11` (provides a full scale reading range of `0V` up to `~3.1V`).
* **Linear Calibration**: The raw `0 - 4095` digital readings are mapped to millivolts via:
  $$\text{Voltage (mV)} = \frac{\text{Raw Reading} \times 3100}{4095}$$

### 📌 Supported ADC1 Pins & Channels
The physical pins exposed on standard ESP32-WROOM modules that support ADC1 conversions are:

| GPIO Pin | ADC1 Channel | Hardware Description |
| :---: | :---: | :--- |
| **32** | `ADC1_CH4` | Analog Input |
| **33** | `ADC1_CH5` | Analog Input |
| **34** | `ADC1_CH6` | Analog Input |
| **35** | `ADC1_CH7` | Analog Input |
| **36** | `ADC1_CH0` | Analog Input (SENSOR_VP) |
| **39** | `ADC1_CH3` | Analog Input (SENSOR_VN) |

---

### ⚡ Added ADC Commands

#### 1. `adc status` (or just typing `adc`)
Generates a beautiful, colored status table showing all 6 exposed analog input pins, their active 12-bit raw readings, and calculated millivolts:

```text
+-----+-------------+-----------+-------------------------+
| PIN | ADC CHANNEL | RAW VALUE | ESTIMATED VOLTAGE (mV)  |
+-----+-------------+-----------+-------------------------+
|  32 | ADC1_CH4    |   1000    |  757 mV (~0.76 V)       |
|  33 | ADC1_CH5    |   2000    | 1514 mV (~1.51 V)       |
|  34 | ADC1_CH6    |   3000    | 2271 mV (~2.27 V)       |
|  35 | ADC1_CH7    |   4000    | 3028 mV (~3.03 V)       |
|  36 | ADC1_CH0    |    100    |   75 mV (~0.08 V)       |
|  39 | ADC1_CH3    |    400    |  302 mV (~0.30 V)       |
+-----+-------------+-----------+-------------------------+
Note: Readings configured with ADC_ATTEN_DB_11 (covers 0 - 3.1V range).
```

#### 2. `adc read <pin>` or shortcut `adc <pin>`
Performs a single on-demand analog read from the requested pin:

```bash
> adc read 32
GPIO 32 (ADC1 Channel): Raw = 1000 | Voltage = 757 mV (~0.76 V)
```

> [!NOTE]
> Attempting to read an analog value from a pin that does not support ADC1 (e.g. general digital pin 4) will be cleanly intercepted by the parser and display a helpful support warning:
> `Error: GPIO is not a valid ADC1 pin! Supported: 32, 33, 34, 35, 36, 39.`

---

## 🚀 GitHub Actions CI/CD Pipeline Integration

We integrated the test suite as an **automated quality gate** in the GitHub release workflow (`.github/workflows/release.yml`).

1. **Automated Verification Step**: During every run of the **Build & Release Firmware** workflow, the Ubuntu CI container immediately builds and executes the suite via `make test` before starting the expensive Espressif ESP-IDF compilation.
2. **Quality Gate Failure Protection**: If any unit test assertion fails or a new feature introduces a regression:
   * The `make test` command will exit with an error code.
   * The workflow run will immediately halt.
   * The build will be marked as **Failed**, completely preventing the release of buggy or unstable firmware binary assets to production or downstream tools!
