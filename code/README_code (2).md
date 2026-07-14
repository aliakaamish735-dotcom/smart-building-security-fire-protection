# Project Source Code

This folder contains the complete source code for the **Smart Building Security and Fire-Protection System**.

## Code Structure

- `arduino_firmware/` — Main integrated Arduino firmware, calibration files, algorithm documentation, CSV logger, and experimental log.
- `arduino_firmware_modular/` — Modular implementation with separate configuration, security, fire-protection, relay, and sensor-driver files.
- `arduino_diagnostics/` — Diagnostic and calibration firmware used to test the connected hardware.
- `Code with no text output/` — Firmware version prepared for the final demonstration without additional Serial text messages.

## Main Features

- TCRT5000 optical person detection
- HX711 load-cell measurement
- DHT11 temperature and humidity monitoring
- YF-S201 water-flow measurement
- 4×4 keypad authentication through PCF8574
- SG90 servo-controlled door
- DFPlayer Mini voice guidance
- Pump, humidifier, and fan control through relay modules
- Timestamped CSV output
- Calibration constants and EEPROM support
- Modular Arduino C/C++ architecture
- Python serial-to-CSV logging utility

## External Libraries and Credits

The firmware uses the following external and Arduino-provided libraries. Their original authors, source repositories, and licenses are credited below.

### Third-Party Libraries

| Library | Author / Maintainer | Purpose | Source | License |
|---|---|---|---|---|
| DHT Sensor Library | Adafruit Industries | Reads DHT11 temperature and humidity values | https://github.com/adafruit/DHT-sensor-library | MIT License |
| HX711 Arduino Library | Bogdan Necula and contributors | Interfaces the HX711 load-cell amplifier | https://github.com/bogde/HX711 | MIT License |
| DFRobotDFPlayerMini | DFRobot | Controls the DFPlayer Mini audio module | https://github.com/DFRobot/DFRobotDFPlayerMini | MIT License |
| ArduinoJson | Benoit Blanchon and contributors | JSON formatting and data handling in diagnostic/modular code | https://github.com/bblanchon/ArduinoJson | MIT License |
| pySerial | Chris Liechti and contributors | Reads Arduino Serial data in the Python CSV logger | https://github.com/pyserial/pyserial | BSD-3-Clause License |

### Arduino-Provided Libraries

| Library | Provider | Purpose | Source | License |
|---|---|---|---|---|
| Servo | Arduino | Controls the SG90 servo motor | https://github.com/arduino-libraries/Servo | LGPL-2.1-or-later |
| Wire | Arduino | Provides I²C communication for the PCF8574 keypad expander | https://github.com/arduino/ArduinoCore-avr | LGPL-2.1-or-later |
| EEPROM | Arduino | Stores calibration constants in non-volatile memory | https://github.com/arduino/ArduinoCore-avr | LGPL-2.1-or-later |
| SoftwareSerial | Arduino | Provides software-based serial communication for the DFPlayer Mini | https://github.com/arduino/ArduinoCore-avr | LGPL-2.1-or-later |

## Project-Written Files

The subsystem algorithms, configuration files, calibration constants, sensor wrappers, relay logic, diagnostic routines, and project documentation included in this folder were developed specifically for this project.

## Programming Languages

- Arduino C/C++
- Python

## Usage

1. Install the required Arduino libraries through the Arduino IDE Library Manager.
2. Open the required `.ino` file in the Arduino IDE.
3. Select **Arduino Uno** and the correct serial port.
4. Compile and upload the firmware.
5. Use the Serial Monitor or `serial_csv_logger.py` to record CSV-formatted sensor data.
