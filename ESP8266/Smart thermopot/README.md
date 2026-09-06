# Smart Thermopot

> IoT-enabled electromechanical heating device with automatic water filling, mass-based volume measurement and web-based control.

## About the project

**Smart Thermopot** is an electromechanical water-heating device developed as a bachelor's graduation project.

The device combines the functionality of a compact thermopot with IoT technologies, allowing it to be potentially integrated into smart home systems.

Unlike conventional thermopots, the device does not have a dedicated manual control interface for starting the heating process. All operating parameters and commands are provided through a web interface hosted directly on an **ESP8266** microcontroller.

The system is capable of automatically filling the tank with a specified amount of water and heating it to a specified temperature.

The amount of water is determined indirectly: the system measures the mass of the tank and its contents using a load cell, then calculates the water volume using the temperature-dependent density of water.

---

## Features

* Automatic water filling using an electromagnetic valve
* Mass measurement using a load cell and HX711 amplifier
* Calculation of water volume based on measured mass and water density
* Temperature measurement using a DS18B20 sensor
* Temperature-dependent water density compensation
* Automatic heating to a user-defined target temperature
* Web-based device control
* Real-time monitoring of temperature, mass and calculated volume
* Wi-Fi configuration through a dedicated access point
* Automatic connection to the previously configured Wi-Fi network
* Storage of Wi-Fi credentials in EEPROM
* mDNS access through `esp8266.local`
* Emergency shutdown of the valve and heating element
* Automatic detection and reporting of sensor communication errors

---

# System overview

The system consists of several functional subsystems:

```text
                  ┌──────────────────────┐
                  │      Web Browser     │
                  │  Control / Monitoring│
                  └──────────┬───────────┘
                             │ Wi-Fi
                             ▼
                  ┌──────────────────────┐
                  │       ESP8266        │
                  │                      │
                  │  Control logic       │
                  │  Web server          │
                  │  Wi-Fi management    │
                  │  EEPROM              │
                  └───────┬───────┬──────┘
                          │       │
                 ┌────────┘       └────────┐
                 ▼                         ▼
          ┌─────────────┐           ┌─────────────┐
          │   DS18B20   │           │    HX711    │
          │ Temperature │           │    ADC      │
          └─────────────┘           └──────┬──────┘
                                           │
                                           ▼
                                     Load cell
                                           │
                          ┌────────────────┴──────────────┐
                          │                               │
                          ▼                               ▼
                  Electromagnetic valve             Heater
```

---

# Hardware

The device is built as a compact enclosure consisting of five main mechanical parts.

## 1. Water tank

The tank contains the main working volume of the device.

The tank includes:

* a side opening for the electromagnetic filling valve;
* an opening on the opposite side for the water outlet tap;
* a bottom opening for the heating element;
* an opening for the temperature sensor.

The DS18B20 temperature sensor is inserted directly into the tank.

The bottom of the tank contains a mechanical interface for the load-cell measurement system. A bolt attached to the bottom of the tank transfers the tank's load to the load cell.

The lower walls of the tank contain guiding sections that allow the tank to move vertically relative to the base. This movement is required for the load-cell measurement system to detect changes in the tank's mass.

---

## 2. Base

The base contains guiding walls that accept the water tank and allow it to move vertically.

A horizontal shelf inside the base provides a mounting point for the second part of the load-cell mechanism.

The shelf also contains openings for:

* the temperature sensor;
* electrical wiring.

Cork insulating pads are used between the tank and the load-cell mounting elements to reduce heat transfer from the heated tank to the measurement system.

The electronic control board is mounted in the lower section of the base.

The board contains:

* ESP8266 microcontroller;
* power supply circuitry;
* relay control circuitry.

A physical power button is also located in the base. Its purpose is to completely disconnect the device from the power supply.

---

## 3. Lower cover

The lower cover closes the bottom of the device and provides mechanical stability.

Its dimensions are larger than those of the main enclosure, increasing the overall footprint and preventing the device from easily tipping over.

The cover also contains slots for the support pins.

---

## 4. Support pins

The support pins prevent the removable lid from resting directly on the water tank.

This is important because applying mechanical force to the tank could affect the load-cell measurement and therefore alter the calculated water volume.

The lid rests on the support pins instead of the tank itself.

---

## 5. Lid

The lid closes the upper part of the device.

Its mechanical arrangement allows it to remain mechanically isolated from the weighing tank, preventing the lid from influencing the measured mass.

---

# Water volume measurement

The device does not directly measure the volume of water.

Instead, the volume is calculated from the measured mass and the density of water:

```text
V = m / ρ
```

where:

* `V` — water volume;
* `m` — measured mass;
* `ρ` — water density at the current temperature.

Because water density changes with temperature, the firmware uses a temperature-dependent density table.

Currently implemented reference points are:

| Temperature |      Density |
| ----------: | -----------: |
|       20 °C |  998.2 kg/m³ |
|       25 °C | 997.05 kg/m³ |
|       50 °C |  988.0 kg/m³ |
|       80 °C |  971.8 kg/m³ |

For temperatures between the reference points, the firmware performs linear interpolation.

For temperatures outside the table range, the nearest boundary value is used.

The mass measurement is performed using a load cell connected to an **HX711** amplifier.

---

# Temperature measurement

Temperature is measured using a **DS18B20** digital temperature sensor connected through the OneWire interface.

The firmware periodically requests a temperature conversion and updates the current temperature value.

If the sensor becomes unavailable, an error is reported through the web interface.

---

# Heating and filling control

The filling and heating processes are controlled by the ESP8266.

The electromagnetic valve and heating element are controlled through digital output pins and relay circuitry.

The user specifies:

* target water volume;
* target temperature.

The firmware then starts the automatic process.

### Filling process

When the process starts, the electromagnetic valve is opened.

The system continuously measures the mass of the tank and calculates the current water volume.

When the calculated volume reaches the target value, the valve is closed.

### Heating process

If the current temperature is below the selected target temperature, the heating element is activated.

The heating element is switched off once the target temperature is reached.

If the target volume is reached before the target temperature, the valve is closed and heating continues until the target temperature is reached.

---

# Web interface

The ESP8266 hosts its own HTTP server and provides a browser-based control interface.

The interface allows the user to monitor:

* current temperature;
* measured mass;
* calculated water volume;
* sensor errors;
* current operating state.

The user can also configure:

* target water volume;
* target temperature.

The interface provides controls for:

* starting the filling/heating process;
* emergency shutdown;
* resetting Wi-Fi configuration.

Sensor data is periodically requested from the ESP8266 using HTTP requests and returned in JSON format.

---

# Wi-Fi configuration

The device supports automatic Wi-Fi configuration.

On the first startup, or when no valid Wi-Fi configuration is stored, the ESP8266 starts its own access point:

```text
SSID: ESP8266_Sensor
```

The user can connect to this network and enter the credentials of the desired Wi-Fi network through the configuration web page.

The credentials are stored in the ESP8266 EEPROM.

On subsequent startups, the ESP8266 automatically attempts to connect to the previously configured network.

If the connection cannot be established within the configured timeout, the device returns to configuration mode.

---

# mDNS

After successfully connecting to a Wi-Fi network, the ESP8266 starts an mDNS service.

The device can therefore be accessed using:

```text
http://esp8266.local
```

instead of manually entering its IP address.

---

# EEPROM configuration

Wi-Fi configuration is stored in EEPROM using the following structure:

```cpp
struct Config {
    char ssid[32];
    char password[32];
    bool isValid;
};
```

This allows the device to retain its network configuration after power loss or restart.

The web interface also provides a mechanism for resetting the stored Wi-Fi configuration.

---

# Safety features

The firmware contains an emergency shutdown mechanism.

When an emergency stop command is received:

* the electromagnetic valve is switched off;
* the heating element is switched off;
* the filling state is cleared;
* the heating state is cleared;
* the emergency state is activated.

The emergency state prevents the normal filling/heating process from continuing until a new process is explicitly started.

A separate physical power switch is also provided in the device itself to completely disconnect the system from power.

> **Important:** This project is a prototype developed as part of a bachelor's graduation project. The current electrical and mechanical implementation should not be considered a production-ready appliance. In particular, the placement of mains-voltage components and the overall electrical isolation require additional engineering work before practical deployment.

---

# Software

The firmware is implemented using the Arduino framework for ESP8266.

### Main libraries

| Library             | Purpose                                  |
| ------------------- | ---------------------------------------- |
| `ESP8266WiFi`       | Wi-Fi connectivity                       |
| `ESP8266WebServer`  | HTTP server and web interface            |
| `DNSServer`         | DNS handling in Wi-Fi configuration mode |
| `EEPROM`            | Persistent storage of Wi-Fi credentials  |
| `ESP8266mDNS`       | `esp8266.local` hostname                 |
| `OneWire`           | OneWire communication                    |
| `DallasTemperature` | DS18B20 temperature sensor               |
| `HX711`             | Load-cell amplifier                      |

---

# Project structure

The current firmware is implemented as a single ESP8266 application containing several functional sections:

```text
ESP8266 firmware
│
├── Wi-Fi initialization
├── EEPROM configuration
├── Sensor acquisition
│   ├── DS18B20
│   └── HX711
│
├── Water density calculation
│
├── Volume calculation
│
├── Filling control
├── Heating control
├── Emergency shutdown
│
├── Wi-Fi configuration server
├── Main web server
├── JSON data endpoint
└── Web interface
```

---

# Current limitations

The current implementation was developed primarily as a prototype and has several known limitations.

### Temperature maintenance

The firmware currently **does not implement temperature maintenance**.

The heating element is switched off after reaching the target temperature, but there is no control loop that periodically checks for temperature decrease and reactivates the heater.

Therefore, the selected temperature should be considered a **target heating temperature**, rather than a temperature-maintenance setpoint.

### Manual control

There is no dedicated manual heating control.

The normal operation is initiated through the web interface.

### Mechanical complexity

The use of a vertically movable tank introduces additional mechanical complexity.

The tank must move freely enough for the load cell to measure its mass while simultaneously maintaining alignment, sealing and mechanical stability.

### Manufacturing tolerances

The prototype enclosure contains areas where the mechanical tolerances were too tight.

The design would benefit from additional clearance and more careful tolerance analysis before manufacturing another revision.

### Electrical safety

The prototype contains mains-voltage components, including the heating system and electromagnetic valve.

The external placement of the electromagnetic valve and the general electrical arrangement are not optimal from a safety perspective.

A production version would require substantially improved electrical isolation, enclosure protection, grounding and separation between low-voltage electronics and mains-voltage circuits.

### Emergency handling

The current emergency-stop mechanism is implemented through the main firmware control flow rather than a dedicated hardware interrupt/safety circuit.

A future revision should consider a hardware-level safety mechanism that does not depend exclusively on the ESP8266 firmware.

---

# Future improvements

Potential improvements include:

* implementation of closed-loop temperature maintenance;
* hysteresis-based heater control;
* improved mechanical tolerances;
* redesigned tank guidance system;
* improved electrical isolation;
* relocation/protection of mains-voltage components;
* hardware-based emergency shutdown;
* improved protection against sensor failures;
* more robust state-machine implementation;
* modularization of the firmware;
* improved web interface;
* integration with existing smart-home systems;
* additional temperature and volume control modes.

---

# Development status

The project was developed as a **bachelor's graduation project** and represents a functional prototype of an IoT-enabled electromechanical heating device.

The current implementation demonstrates the core concept:

```text
Wi-Fi configuration
       ↓
Automatic Wi-Fi connection
       ↓
Web interface
       ↓
Target volume + target temperature
       ↓
Automatic filling
       ↓
Mass measurement
       ↓
Temperature-compensated volume calculation
       ↓
Automatic heating
       ↓
Target conditions reached
```

The project can serve as a foundation for further development of a compact smart water-heating appliance and its integration into IoT and smart-home systems.

---

# Technologies

* **ESP8266**
* **Arduino framework**
* **C/C++**
* **Wi-Fi**
* **HTTP**
* **mDNS**
* **EEPROM**
* **OneWire**
* **DS18B20**
* **HX711**
* **Load cell**
* **HTML / CSS / JavaScript**
* **JSON**

---

# License

This project is licensed under the **MIT License**.

The license applies to the software published in this repository.

The mechanical design, construction and other intellectual property associated with the physical device are not necessarily covered by the software license.

See the `LICENSE` file for the complete license text.
