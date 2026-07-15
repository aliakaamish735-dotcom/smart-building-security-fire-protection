/*
 * EEPROMCalibration.h — OPTIONAL bonus module for Sensors Project
 *
 * CalibrationConstants.h covers the course requirement as written
 * ("stored in EEPROM OR a dedicated constants file"). This file is an
 * extra step beyond that: it persists the two calibration factors in
 * the Arduino's onboard EEPROM, so a re-flash of new firmware doesn't
 * wipe out a calibration you already did in the field. Mention this in
 * your report's "Engineering Constraints" (maintainability) section if
 * you wire it in — it's a nice, cheap maintainability win.
 *
 * This is NOT wired into the main sketch by default. To use it:
 *   1. #include "EEPROMCalibration.h" in your main .ino
 *   2. In setup(), call loadCalibrationFromEeprom() BEFORE scale.set_scale(...)
 *   3. Whenever you recompute a calibration factor (e.g. from a serial
 *      command during a calibration routine), call
 *      saveCalibrationToEeprom(newLoadCellFactor, newFlowFactor);
 *
 * Uses a magic-number header byte so the code can tell "never
 * calibrated / blank EEPROM" apart from "real saved calibration," and
 * falls back to the CalibrationConstants.h defaults in the former case.
 */
#ifndef EEPROM_CALIBRATION_H
#define EEPROM_CALIBRATION_H

#include <Arduino.h>
#include <EEPROM.h>
#include "CalibrationConstants.h"

namespace {
  constexpr int EEPROM_ADDR_MAGIC   = 0;               // 1 byte
  constexpr int EEPROM_ADDR_LOADCELL = EEPROM_ADDR_MAGIC + 1;      // 4 bytes (float)
  constexpr int EEPROM_ADDR_FLOW     = EEPROM_ADDR_LOADCELL + 4;   // 4 bytes (float)
  constexpr uint8_t EEPROM_MAGIC_BYTE = 0x76; // arbitrary "this EEPROM has real data" marker
}

struct CalibrationValues {
  float loadCellScaleFactor;
  float flowCalibrationFactor;
};

// Loads calibration from EEPROM if it was previously saved; otherwise
// returns the defaults from CalibrationConstants.h untouched.
inline CalibrationValues loadCalibrationFromEeprom() {
  CalibrationValues cal;
  cal.loadCellScaleFactor   = LOADCELL_SCALE_FACTOR;
  cal.flowCalibrationFactor = FLOW_CALIBRATION_FACTOR;

  uint8_t magic = EEPROM.read(EEPROM_ADDR_MAGIC);
  if (magic == EEPROM_MAGIC_BYTE) {
    EEPROM.get(EEPROM_ADDR_LOADCELL, cal.loadCellScaleFactor);
    EEPROM.get(EEPROM_ADDR_FLOW, cal.flowCalibrationFactor);
    Serial.println(F("Loaded calibration factors from EEPROM."));
  } else {
    Serial.println(F("No saved EEPROM calibration found - using CalibrationConstants.h defaults."));
  }
  return cal;
}

// Call this after computing a new calibration factor (e.g. from a
// serial-based calibration routine) to persist it across reflashes.
inline void saveCalibrationToEeprom(float loadCellScaleFactor, float flowCalibrationFactor) {
  EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC_BYTE);
  EEPROM.put(EEPROM_ADDR_LOADCELL, loadCellScaleFactor);
  EEPROM.put(EEPROM_ADDR_FLOW, flowCalibrationFactor);
  Serial.println(F("Calibration factors saved to EEPROM."));
}

#endif // EEPROM_CALIBRATION_H
