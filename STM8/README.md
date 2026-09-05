### STM8 driver for DS18B20

## Features
> Uses GPIO
> Wrote by using registers
> Supports several sensors

## ds18b20.h
Header file of the DS18B20 driver. Contains function declarations for initializing the 1-Wire interface, generating a reset pulse, performing bitwise and bytewise data exchange, starting temperature conversion, obtaining the sensor ROM address, and reading temperature.

# Functions
> del3us() - primitive software delay of approximately several microseconds. Was used for adjusting 1-Wire timing.
> sensor_rst() - generates a Reset Pulse on the 1-Wire bus and checks for the presence of the sensor (Presence Pulse).
> sensor_init() - configures GPIO and STM8 clocking for operation with the 1-Wire line.
> sensor_ReadBit(), sensor_WriteBit() - low-level read/write operations for a single bit in accordance with 1-Wire timing specifications.
> sensor_ReadByte(), sensor_WriteByte() - assemble/disassemble 8 bit read/write operations for byte exchange.
> sensor_get_temp() - reads the Scratchpad data of the selected DS18B20 and calculates the temperature.
> sensor_get_add() - reads the 64-bit ROM code of the sensor.
> temp_transmit() - starts temperature conversion using the Convert T command (0x44).
> read_curent__add() - sequentially outputs the bytes of the previously read ROM address.

## ds18b20.c
The actual driver implementation.

# Global variables
> int Data[9] - DS18B20 Scratchpad buffer.
> char _add[8] - Storage for the 64-bit ROM code of the sensor.
> unsigned char PIN - GPIO pin number used for the 1-Wire interface.
> char num_byte_addres - Index of the current address byte during sequential reading via read_current_add()

# Functions
> sensor_init() - initialization of the driver hardware interface: configuring STM8S207 clocking, GPIO setup, and storing the pin number used for 1-Wire.
> sensor_rst() - the driver implements the 1-Wire reset and presence detection sequence directly through GPIO with microsecond-level timing.
> sensor_ReadBit() - the master briefly pulls the line low, then releases it and reads the line state at the appropriate moment.
> sensor_WriteBit() - the LOW duration determines whether a 1 or 0 is written: a longer LOW writes 0, a shorter one writes 1.
> sensor_ReadByte() / sensor_WriteByte() - writing/reading the corresponding bits into bytes.
> sensor_get_add() - Read ROM command (0x33). Sequence: Reset -> Read ROM (0x33) -> Read 8 bytes -> save ROM.
> temp_transmit() - (The name is a bit unfortunate). In fact, it does not transmit the temperature, but starts its conversion. Here is using of Skip ROM code
> sensor_get_temp() - Match ROM. The function float sensor_get_temp(char* a) takes a — the address of a specific DS18B20. This allows selecting a particular sensor from several connected ones.
How temperature is calculated
You take:
Data[0]
Data[1]
These are the two temperature bytes.
The DS18B20 uses a 12-bit representation with a resolution of:
0.0625 °C
That is:
t = raw_temperature * 0.0625;
Therefore:
1 unit → 0.0625 °C
16 units → 1 °C
400 units → 25 °C
Negative temperature is determined by the sign bit in Data[1].

## timerOnDelay.c
This file and the header file are not directly part of the driver. They are an auxiliary module for generating precise delays via STM8 Timer 1.

# Functions
> delay_us() - configures: TIM1 -> prescaler -> Auto-Reload Register -> timer start -> waiting for execution -> stop
> delay_ms() - same as delay_us() but with prescaler settings for millisecond delays.
> set_F_MHz() - reports the timer frequency to the module

## timerOnDelay.h
*Header file for the timer-based delay module and low-level GPIO definitions.

# Functions
Declares:
> delay_us() — generates a delay in microseconds using STM8 Timer 1.
> delay_ms() — generates a delay in milliseconds using STM8 Timer 1.
> set_F_MHz() — sets the peripheral clock frequency used by the delay module.

# GPIO
Defines the PORT structure used to access STM8 GPIO registers directly.
The structure contains:
> ODR — Output Data Register.
> IDR — Input Data Register.
> DDR — Data Direction Register.
> CR1 — Control Register 1.
> CR2 — Control Register 2.
The header also defines memory-mapped pointers for GPIO ports GPIOA through GPIOG, allowing direct access to their hardware registers.
