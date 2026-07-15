/*
 * CalibrationConstants.h — Sensors Project
 *
 * Every calibration constant, threshold, and credential the firmware
 * needs lives HERE and nowhere else. Nothing in the driver or
 * state-machine files should hard-code a number that could plausibly
 * change per-unit or per-deployment — that's exactly the pattern the
 * course proposal's Firmware requirements section asks you to avoid
 * ("calibration constants stored in EEPROM or a dedicated constants
 * file; not hard-coded inline").
 *
 * HOW TO FILL THIS IN FOR REAL (do this before your final report):
 *   1. Run the load-cell calibration procedure (known reference weights,
 *      3 trials minimum) and replace LOADCELL_SCALE_FACTOR below.
 *   2. Run the flow-sensor calibration procedure (timed volume
 *      collection, 3 trials minimum) and replace FLOW_CALIBRATION_FACTOR.
 *   3. Record your raw/reference numbers in
 *      /data/calibration_log_template.csv — that table IS your
 *      report's "Calibration Procedures and Results" section.
 *
 * OPTIONAL BONUS: EEPROMCalibration.h/.cpp (same folder) shows how to
 * persist these two calibration factors in EEPROM instead, so they
 * survive a re-flash without editing source code — mentioned in your
 * report as a maintainability improvement.
 */
#ifndef CALIBRATION_CONSTANTS_H
#define CALIBRATION_CONSTANTS_H

// ----------------------------------------------------------------
// LOAD CELL (HX711) — grams per raw ADC unit, see calibration proc.
// TODO: replace with your measured value (currently an initial guess).
// ----------------------------------------------------------------
constexpr float LOADCELL_SCALE_FACTOR = 420.0;

// ----------------------------------------------------------------
// FLOW SENSOR (YF-S201 / YF-B1) — pulses per second per L/min.
// TODO: replace with your measured value (currently the datasheet's
// typical value, not a per-unit calibration).
// ----------------------------------------------------------------
constexpr float FLOW_CALIBRATION_FACTOR = 7.5;

// ----------------------------------------------------------------
// FIRE-PROTECTION THRESHOLDS
// ----------------------------------------------------------------
constexpr float FIRE_THRESHOLD_C       = 50.0;   // trip point to start the sequence
constexpr float NORMAL_TEMP_C          = 25.0;   // reference "back to normal" temperature
constexpr float NORMAL_HUMIDITY_PCT    = 75.0;   // ambient-humidity fan's turn-OFF point only
                                                  // (see HUMIDITY_FAN_ON_PCT) - no longer used by
                                                  // the humidifier, which now runs a fixed duration
constexpr float HUMIDITY_FAN_ON_PCT    = 60.0;   // general ambient-humidity fan control (independent of
                                                  // the fire sequence): fan turns ON above this, and
                                                  // OFF once humidity drops back to NORMAL_HUMIDITY_PCT
constexpr float FLOW_BLOCKAGE_LPM      = 0.05;   // flow at/below this while pump runs = blockage
constexpr unsigned long PUMP_RUN_MS    = 55000;  // pump runs for exactly 55s once fire is detected,
                                                  // then the humidifier takes over
constexpr unsigned long HUMIDIFIER_RUN_MS = 10000; // humidifier runs for exactly 10s after the pump's
                                                  // 55s window, then turns off automatically -
                                                  // independent of the current ambient humidity reading
constexpr unsigned long FIRE_BUZZER_ON_MS = 5000; // buzzer sounds for exactly 5s the moment fire is
                                                  // first detected, then turns off automatically.
                                                  // This is independent of the pump: the pump still
                                                  // starts right away and runs for PUMP_RUN_MS (55s)
                                                  // regardless of the buzzer's 5s window.

// ----------------------------------------------------------------
// ENTRY SECURITY THRESHOLDS
// ----------------------------------------------------------------
constexpr int   MAX_WRONG_ATTEMPTS     = 3;    // N from the algorithm doc
constexpr int   PHOTOCELL_THRESHOLD    = 500;  // 0-1023 (10-bit analogRead), tune to your door's mounting distance and lighting.
// TCRT5000 also has an onboard trim potentiometer that adjusts the comparator
// threshold in hardware — use both together: pot for coarse range, this constant for fine tuning.
constexpr float WEIGHT_DETECT_THRESHOLD_G = 10.0; // HX711 + load cell floor pad, in front of the
// door: any reading above this counts as "someone is standing on it." Set well above your load
// cell's zero-load noise floor (check Serial output with nothing on the pad and tune from there) so
// vibration/drift doesn't false-trigger. The proximity sensor AND this weight pad must BOTH detect
// a person before the system prompts for a password (see pollProximityAndKeypad() in entry_security.ino).
constexpr unsigned long PROXIMITY_DEBOUNCE_MS = 8000;
constexpr unsigned long KEYPAD_TIMEOUT_MS     = 15000;
constexpr unsigned long DOOR_OPEN_MS          = 5000;

// ----------------------------------------------------------------
// SAMPLING / TIMING — justify this in your report's Firmware section:
//   - DHT11 can't physically sample faster than ~1 Hz; a 5s interval
//     also matches the fire sequence's expected timescale (seconds,
//     not milliseconds) for logging/CSV output.
// ----------------------------------------------------------------
constexpr unsigned long SENSOR_POST_INTERVAL   = 5000;  // ms between sensor reads + local CSV logging

// ----------------------------------------------------------------
// DOOR ENTRY PIN — this system is now fully standalone (no WiFi/ESP-01,
// no server), so the accepted PIN lives here instead of a web app.
// TODO: change this to your own PIN before deploying.
// ----------------------------------------------------------------
static const char* DOOR_PIN = "1234";

#endif // CALIBRATION_CONSTANTS_H
