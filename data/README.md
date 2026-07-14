# Smart Building Security and Fire Protection — Project Data

This folder contains the complete dataset and analysis workbook for the **Smart Building Security and Fire Protection System**.

## Main Data File

- `whole_dataset_for_the_sensors_project(1).xlsx`

The Excel workbook is a standalone project database. It includes the original records, cleaned sensor data, event logs, calibration results, analysis-ready tables, charts, firmware constants, and a data dictionary.

## Important System Rule

The **HX711 load cell belongs only to the Entry Security System**.

It measures the weight applied to the entry platform and works together with the **TCRT5000 optical sensor** to confirm the presence of a person:

```text
Person detected = Optical detection AND Entry-platform weight detection
```

The load cell is **not used to measure the water tank** and is not interpreted as a fire-protection sensor.

## Workbook Contents

| Sheet | Description |
|---|---|
| `00_Cover` | Project description, fixed rules, cleaning decisions, and workbook summary |
| `01_Dashboard` | Main KPIs and charts for the report and presentation |
| `02_Parameters` | Firmware constants, thresholds, timings, and data-cleaning parameters |
| `03_Master_Data` | Organized master table containing sensor snapshots, system events, and calibration trials |
| `04_Clean_TimeSeries` | Clean, report-ready time-series sensor data |
| `05_Long_Observations` | Normalized sensor observations for calculations, filtering, and pivot tables |
| `06_Calibration` | Entry-platform load-cell calibration trials, equations, and results |
| `07_Event_Log` | Cleaned security, fire, and firmware event timeline |
| `08_Complete_Data_Archive` | Consolidated archive of all original project records |
| `09_Data_Dictionary` | Plain-English explanation of the important columns and variables |
| `10_Analysis_Data` | Analysis-ready records and calculated indicators |
| `11_Firmware_Calibration` | Calibration factors, equations, thresholds, and constants extracted from the firmware |
| `12_Nominal_References` | Reference values and comparison results |
| `13_Statistical_Calibration` | Simulated calibration points and before/after error calculations for modalities without complete measured calibration evidence |

## Dataset Summary

The workbook currently contains:

- **680** complete archive rows
- **370** master records
- **242** clean sensor snapshots
- **1,210** long-format sensor observations
- **113** cleaned system events
- **15** measured load-cell calibration trials

## Recorded Variables

The dataset includes values and events related to:

- TCRT5000 optical detection
- HX711 entry-platform weight
- DHT11 temperature and humidity
- YF-S201 water-flow rate
- Keypad activity and PIN-entry results
- Servo-controlled door operation
- Pump, humidifier, and fan states
- Fire-detection and suppression states
- Security alarms, wrong attempts, and detected errors

PIN values are masked in the clean analysis sheets.

## Main Firmware Parameters

| Parameter | Value |
|---|---:|
| Fire-temperature threshold | 28 °C |
| Flow blockage threshold | 0.05 L/min |
| Pump run duration | 55 s |
| Humidifier run duration | 10 s |
| Keypad inactivity timeout | 15 s |
| Maximum wrong attempts | 3 |
| Door-open duration | 5 s |
| TCRT5000 optical threshold | 500 ADC |
| Entry-platform weight threshold | 10 g |
| Flow calibration factor | 7.5 pulses/s per L/min |
| Sensor logging interval | 5 s |

These values are documented in the workbook and should be checked against the final physical calibration before the final demonstration.

## Data-Cleaning Decisions

The workbook preserves the original raw data while also providing clearly labelled cleaned and analysis-ready values.

- Negative load-cell readings remain preserved; the entry-weight analysis uses their positive magnitude.
- DHT11 values equal to `-1` are treated as failed sensor reads.
- Temperature values outside `0–60 °C` are excluded from clean measurements.
- Humidity values outside `0–100 %RH` are excluded from clean measurements.
- Very high historical flow values remain preserved; a clearly labelled analysis divisor is applied only where required.
- Legacy person-detection events are flagged when they do not follow the corrected optical-and-weight rule.

## How to Use the Workbook

1. Open `00_Cover` to review the system rules and workbook map.
2. Use `01_Dashboard` for presentation-ready KPIs and charts.
3. Use `03_Master_Data` for the organized complete dataset.
4. Use `04_Clean_TimeSeries` for sensor trends and report charts.
5. Use `07_Event_Log` to review security and fire-system actions.
6. Use `06_Calibration` and `11_Firmware_Calibration` when discussing sensor calibration.
7. Use `09_Data_Dictionary` to understand each important column.

## Calibration Note

Measured three-point calibration evidence is currently complete for the **entry-platform load cell**. Some additional calibration tables are based on nominal references or simulated data and are clearly identified as such inside the workbook. They should not be presented as physical measurements.

## File Organization

```text
data/
├── whole_dataset_for_the_sensors_project(1).xlsx
└── README.md
```

No additional `raw`, `processed`, or `calibration` folders are required because these categories are already organized into separate sheets inside the Excel workbook.
