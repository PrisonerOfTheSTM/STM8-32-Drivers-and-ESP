### STM32 HAL-based DS18B20 driver using USART 1-Wire emulation

## Features

* Emulates the 1-Wire protocol using USART in half-duplex mode.
* Uses STM32 HAL for USART communication.
* Changes USART baud rate to generate the timing required by the 1-Wire protocol.
* Supports temperature measurement using the DS18B20 Scratchpad.
* Currently supports only one DS18B20 sensor on the 1-Wire bus.
* Uses the Skip ROM command for communication with the sensor.

## commands.h

Header file of the DS18B20 driver. Includes the STM32 HAL and defines the USART peripheral used for 1-Wire communication.

The `YOURS_UART` macro selects the USART peripheral used by the driver. In the current configuration, `USART2` is used.

# Functions

* `onewire_init()` - generates a 1-Wire reset pulse using USART and checks the sensor response.
* `onewire_write_byte()` - sends a byte over the emulated 1-Wire bus by transmitting specially selected USART byte patterns.
* `onewire_read_byte()` - reads one byte from the DS18B20 by transmitting `0xFF` and interpreting the received USART data.
* `read_temp()` - starts temperature conversion, reads the DS18B20 Scratchpad temperature registers, and calculates the temperature value.

## commands.c

The actual implementation of the DS18B20 driver.

The driver does not use a dedicated 1-Wire peripheral. Instead, it uses the STM32 USART configured in half-duplex mode to reproduce the electrical and timing behavior required by the 1-Wire protocol.

The USART baud rate is changed dynamically depending on the operation being performed:

* Approximately 9600 baud is used for the 1-Wire reset sequence.
* Approximately 115200 baud is used for bit-level data exchange.

# Global variables

* `float temp` - stores the calculated temperature value.
* `YOURS_UART` - external `UART_HandleTypeDef` used by the driver.

# Functions

* `onewire_init()` - generates the 1-Wire Reset Pulse by transmitting `0xF0` at approximately 9600 baud. The returned byte is then checked to determine the presence response from the DS18B20.

* `onewire_write_byte()` - sends an 8-bit value over the emulated 1-Wire bus. Each bit is transmitted individually. A `1` is represented by `0xFF`, while a `0` is represented by `0x00`.

* `onewire_read_byte()` - reads eight individual bits from the DS18B20. The master transmits `0xFF` to release the bus and then examines the received USART byte. A received `0xFF` is interpreted as logic `1`, while any other value is interpreted as logic `0`.

* `onewire_read_byte_rom()` - performs the same byte-reading operation as `onewire_read_byte()`. It appears to be intended for ROM-address operations, although the current implementation does not use it in the temperature-reading sequence.

* `read_temp()` - performs the complete DS18B20 temperature measurement sequence:

  1. Generates a 1-Wire reset.
  2. Sends `Skip ROM` (`0xCC`).
  3. Sends `Convert T` (`0x44`).
  4. Generates another 1-Wire reset.
  5. Sends `Skip ROM` (`0xCC`).
  6. Sends `Read Scratchpad` (`0xBE`).
  7. Reads the LSB and MSB of the temperature value.
  8. Converts the raw 16-bit DS18B20 value into degrees Celsius.

# Temperature calculation

The DS18B20 stores the measured temperature as a signed 16-bit value with a resolution of:

`0.0625 °C`

The two temperature bytes are read from the Scratchpad:

* `LSB` — lower 8 bits of the temperature value.
* `MSB` — upper 8 bits of the temperature value.

They are combined into a 16-bit raw value:

`raw_temperature = (MSB << 8) | LSB`

The temperature is then calculated as:

`t = raw_temperature / 16.0`

Therefore:

* `1` raw unit → `0.0625 °C`
* `16` raw units → `1 °C`
* `400` raw units → `25 °C`

The signed representation of the DS18B20 allows negative temperatures to be represented as well.

## main.c

Main application file generated and configured using STM32 HAL.

It initializes the STM32 HAL, system clock, GPIO, USART1 and USART2, and then continuously calls the DS18B20 temperature-reading function.

# Configuration

The system clock is configured from an external HSE source using PLL with a multiplication factor of 9, resulting in a 72 MHz system clock.

Both USART1 and USART2 are initially configured for 9600 baud and half-duplex operation. The DS18B20 driver subsequently changes the USART baud rate directly through the USART `BRR` register to generate the timing required for different 1-Wire operations.

`USART2` is selected by default through the `YOURS_UART` macro.

# Single-sensor limitation

The current implementation is designed for communication with a single DS18B20 sensor.

The temperature-reading sequence uses the `Skip ROM` command (`0xCC`). This command addresses all sensors on the 1-Wire bus simultaneously and is only safe when exactly one sensor is connected.

The driver does not currently implement the complete 1-Wire ROM addressing procedure required for selecting an individual sensor from multiple devices.

## Multiple DS18B20 sensors

Addressed communication with multiple DS18B20 sensors was not successfully implemented in this version.

The main difficulty is the timing difference between the 1-Wire protocol and USART framing. A USART byte always contains a start bit, data bits and a stop bit, while 1-Wire communication is controlled by precisely timed bus slots in which the master and slave can actively pull the line low.

This makes operations such as `Search ROM` considerably more difficult to emulate than simple single-device communication. `Search ROM` requires detecting collisions between the ROM bits transmitted by multiple sensors, reading both the bit and its complement, and immediately transmitting a selected branch bit within the same sequence of 1-Wire time slots.

The current USART-based implementation works for basic single-device communication, but the fixed USART start/stop framing and byte-oriented operation make precise implementation of the multi-device ROM search and addressing sequence difficult.
