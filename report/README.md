# Security and Fire-Protection System

This folder contains the final technical report for the **Security and Fire-Protection System**, developed for the **Sensors & Instrumentation** course at the Lebanese University, Faculty of Engineering III.

## Main Report

- `security_and_fire_protection_systems_report.pdf`

The report presents the complete design, implementation, testing, data analysis, calibration framework, and evaluation of an Arduino-based smart sensing platform that combines building-entry security with automatic fire-protection functions.

## Project Team

1. **Ali Al Hadi Akaamish** — ID: 6812  
2. **Mustapha Hamdan** — ID: 6947  
3. **Nassim Amer** — ID: 6939  
4. **Makram Ghraizi** — ID: 6723  

**Instructor:** Dr. Mohamad Ahmad Aoude  
**Academic Year:** 2025–2026

## System Overview

The project is divided into two coordinated subsystems:

### Entry Security System

- TCRT5000 infrared optical sensor
- HX711 load-cell amplifier and load cell
- 4×4 keypad through a PCF8574 I²C expander
- SG90 servo-controlled door
- Security buzzer
- DFPlayer Mini voice guidance

A person is confirmed only when the optical sensor and entry-platform weight detection are both active. The user is then asked to enter a PIN. A correct PIN opens the door, while repeated incorrect attempts activate the security alarm.

### Fire-Protection System

- DHT11 temperature and humidity sensor
- YF-S201 water-flow sensor
- Mini water pump
- Ultrasonic humidifier
- Ventilation fan
- Warning LED
- Fire and blockage alarms
- DFPlayer Mini voice messages
- Three active-low relay modules

The fire-protection sequence monitors environmental conditions, detects high temperature, activates the pump, verifies water flow, operates the humidifier, controls ventilation, and records the system state.

## Sensor Modalities

The system integrates the five required sensor modalities:

- **Optical sensing:** TCRT5000
- **Force/weight sensing:** Load cell with HX711
- **Flow sensing:** YF-S201
- **Temperature sensing:** DHT11
- **Humidity sensing:** DHT11

## Report Contents

The report includes:

1. Introduction  
2. Problem Statement  
3. System Architecture  
4. Sensor Selection and Justification  
5. Hardware Design and Schematics  
6. Software Design and Firmware Architecture  
7. Calibration Procedures and Results  
8. Experimental Results and Data Analysis  
9. Measurement Uncertainty and Performance Analysis  
10. Engineering Constraints Discussion  
11. Conclusion and Future Work  
12. References  

It also includes appendices covering:

- Bill of Materials
- Verification and demonstration plan
- Proposal compliance matrix
- Calibration evidence and calculations
- Real-time acquisition and visualization
- Repository and submission checklist

## Data and Experimental Evidence

The report documents:

- 242 multi-sensor samples
- 113 system events
- Three acquisition sessions
- A longest continuous run of approximately 19.47 minutes
- Four recorded fire-response cycles
- Physical load-cell calibration trials
- Temperature, humidity, flow, event, and system-state visualizations
- Water-volume and timing calculations
- CSV-compatible timestamped data logging

## Hardware Documentation

The report contains the final Fritzing breadboard representation, interface mapping, power-domain explanation, relay connections, water-flow path, and complete Bill of Materials.

## Firmware Documentation

The firmware architecture includes:

- Continuous entry-security monitoring
- Five-second environmental acquisition and CSV logging
- Dual-sensor person confirmation
- Keypad PIN processing
- Servo-controlled door operation
- Fire-protection state machine
- Pump, humidifier, fan, LED, buzzer, and voice-message control
- Flow-based blockage detection
- Timestamped system-state records

## Recommended Repository Location

```text
report/
├── README.md
└── security_and_fire_protection_systems_report.pdf
```

## Purpose of This Document

This README provides a quick overview of the final report and helps reviewers understand the project scope, system structure, experimental work, and report organization before opening the complete PDF.
