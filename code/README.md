# Project Source Code

This folder contains the complete source code for the **Smart Building Security and Fire-Protection System**.

## Folder Overview

### `arduino_firmware/`
Contains the main Arduino firmware implementing:
- Entry Security subsystem
- Fire Protection subsystem
- Integrated Arduino sketch
- Calibration constants
- EEPROM calibration support
- CSV sensor logging
- Python serial CSV logger
- Algorithm documentation

### `arduino_firmware_modular/`
Contains the modular implementation including:
- Main modular sketch
- Configuration headers
- Calibration headers
- Entry Security module
- Fire Protection module
- Relay driver
- TCRT5000 driver

### `arduino_diagnostics/`
Firmware dedicated to diagnostics, testing, and calibration.

### `Code with no text output/`
Clean firmware version without serial text output, intended for the final hardware demonstration.

## Main Features

- Dual-subsystem architecture
- TCRT5000 optical sensing
- HX711 load-cell support
- DHT11 temperature and humidity monitoring
- YF-S201 flow measurement
- Keypad authentication
- Servo-controlled door
- DFPlayer Mini voice guidance
- Relay-controlled pump, humidifier, and fan
- Timestamped CSV logging
- EEPROM calibration support
- Modular Arduino C/C++ implementation

## Programming Languages

- Arduino C/C++
- Python

## Purpose

The source code implements the complete embedded logic required to operate the Smart Building Security and Fire-Protection System, including sensor acquisition, decision making, actuator control, calibration, diagnostics, and experimental data logging.
