# STM8-32-Drivers
A collection of STM8/32 peripheral and sensor drivers written in C

# Dependencies

The driver uses the `iostm8s207.h` header provided by the STM8 development environment.

This header contains the definitions of the STM8S207 microcontroller's hardware registers and peripherals, allowing direct access to GPIO, timers, clock control and other MCU resources.

It is used for low-level hardware control, for example:

> `PA_ODR` — GPIO Port A Output Data Register.

> `PA_IDR` — GPIO Port A Input Data Register.

> `PA_DDR` — GPIO Port A Data Direction Register.

> `CLK_CKDIVR` — Clock Divider Register.

> `TIM1_*` — Timer 1 registers.
