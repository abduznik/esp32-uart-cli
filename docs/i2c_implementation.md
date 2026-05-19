# ESP32 Safety-Aware Dynamic I2C Controller

We have integrated a high-performance, dynamic, and safety-aware **I2C Bus Scanner & register editor** command group into the ESP32 firmware CLI. This module turns your ESP32 CLI into a versatile hardware debugging tool to probe sensors, RTCs, OLED displays, and other I2C peripherals.

---

## 🛡️ Pin Safety Enforcement for I2C

To preserve our strict hardware safety paradigm, configuring custom I2C pins dynamically is bound by active security checks. 

Before configuring dynamic SDA/SCL buses, the driver validates both pins against the system registry:
* 🔴 **SPI Flash Pins (GPIO 6-11)**: Blocked! Attempting to route SCL/SDA to flash pins would disconnect the program storage and cause a CPU panic loop.
* 🔴 **Console UART0 Pins (GPIO 1, 3)**: Blocked! Re-routing SDA/SCL here would immediately disconnect active serial CLI terminal lines.
* ⚫ **Invalid Pins (GPIO 20, 24, etc.)**: Blocked! Pins do not exist or are not exposed.

---

## ⚡ Added I2C CLI Commands

### 1. `i2c scan` (or just `i2c`)
Generates an interactive, dynamically colored **7-Bit Address Matrix** matching the standard Linux `i2cdetect` output style. It scans addresses `0x03` to `0x77` (decimal 3 to 119) and displays responding addresses in bold green text:

```text
Scanning I2C Bus (SDA: GPIO 21, SCL: GPIO 22, Speed: 100kHz)...

     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- 3c -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- --

I2C scan complete. Found 2 active devices.
```

### 2. `i2c init <sda_pin> <scl_pin>`
Allows developers to **re-route SCL and SDA lines dynamically** without re-compiling the firmware. This is supported by the ESP32's internal GPIO matrix:
```bash
> i2c init 4 5
I2C Bus reconfigured successfully (SDA: GPIO 4, SCL: GPIO 5)!
```
*If a restricted pin is targeted:*
```bash
> i2c init 1 5
Error: SDA Pin 1 is restricted or invalid!
```

### 3. `i2c read <hex_addr> <hex_reg>`
Executes a 1-byte read transaction from the specified I2C device address and register address:
```bash
> i2c read 3c 10
I2C Read [Addr: 0x3c, Reg: 0x10] -> 0x00
```

### 4. `i2c write <hex_addr> <hex_reg> <hex_val>`
Executes a 1-byte write transaction pushing a value into the specified I2C device register:
```bash
> i2c write 3c 10 aa
I2C Write Success [Addr: 0x3c, Reg: 0x10] <- 0xaa
```

---

## 🧪 Stateful Off-Target Mock Test Simulation

The host-side unit testing framework (`tests/test_runner.c`) features a **stateful I2C bus simulation engine**:

1. **Stateful Transactions**: The simulated driver tracks `mock_start_sent` on the bus. When a `START` transaction is issued, the first written byte is parsed as `(address << 1) | direction`, preserving the active I2C slave address. Subsequent register offsets and payloads are kept as register values, matching realistic hardware operations.
2. **Dynamic Probing**: You can populate the `mock_i2c_devices` registry array within your unit test cases to verify scanning output matrices.
3. **Safety Assertions**: The test suite validates that re-configuring the bus to SPI Flash pins or Console UART0 pins is cleanly intercepted and blocked, while safe General Purpose lines are approved.

To execute the host-side simulator suite:
```bash
make test
```
