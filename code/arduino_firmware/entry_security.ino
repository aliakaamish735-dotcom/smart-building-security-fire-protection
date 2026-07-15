/*
 * entry_security.ino — part of the Biological Laboratory Monitor firmware
 *
 * Implements System 1 from the algorithm doc, now requiring TWO sensors to
 * agree before prompting for a password:
 *   IF proximity sensor detects a person near the door
 *      AND the HX711 + load cell floor pad detects a mass on it:
 *     Play "PLEASE ENTER PASSWORD"
 *     Wait for password input from keypad
 *     IF entered password == stored password: open door, reset attempts
 *     ELSE: increase wrong attempts; IF wrong attempts >= N: alarm
 *
 * Proximity is read from a TCRT5000 IR reflective sensor: a single
 * analog wire (AO -> A2), no library or I2C involved at all. This
 * replaced an earlier APDS-9960 (I2C) design — see the "TCRT5000 IR
 * reflective sensor" section below for the current implementation.
 *
 * The HX711 + load cell used to read the fire-suppression tank's
 * weight; that tank/tank-weight logic has been removed entirely. The
 * same load cell hardware is now repurposed as a floor pad in front of
 * the door: it must detect a nonzero mass (someone standing on it) at
 * the same time the TCRT5000 sees someone nearby — both sensors must
 * agree, not just proximity alone. See "HX711 load cell (floor pad)"
 * section below.
 *
 * The keypad is NOT wired directly to Arduino pins. It's wired to a
 * PCF8574 I2C I/O expander at address 0x20 (A0/A1/A2 tied to GND) - see
 * the pin map in arduino_firmware.ino / Circuit_summary.txt.
 *
 * DFPlayer audio track numbers (place these as 0001.mp3, 0002.mp3, etc.
 * on the microSD card root, or in a /mp3 folder depending on your
 * DFPlayer library version's convention):
 *   Track 1 = "PLEASE ENTER PASSWORD"
 *   Track 2 = "ACCESS ACCEPTED"
 *   Track 3 = "ACCESS DENIED"
 *   Track 4 = "ACCESS DENIED, THIEF ATTEMPT"
 *   Track 5 = "HIGH TEMPERATURE DETECTED"
 *   Track 6 = "PIPE BLOCKAGE DETECTED"
 *   Track 7 = "SYSTEM STABLE"
 */

// ----------------------- TCRT5000 IR reflective sensor -----------------------
// Replaces the earlier APDS-9960 (I2C). Single-wire analog connection:
//   VCC -> 5V, GND -> GND, AO -> Arduino A2 (PIN_PHOTOCELL)
// The module's own "DO" pin is intentionally left disconnected — its
// name coincidentally matches Arduino's D0 (USB/Serial RX) but is NOT
// the same pin, and wiring it there would conflict with Serial
// Monitor/uploads. One analog wire (AO) is enough for presence detection.
//
// PHOTOCELL_THRESHOLD now comes from CalibrationConstants.h (0-1023 scale).

void setupApds9960() {
  // Function name kept for continuity with the rest of this file's
  // call sites; TCRT5000 needs no I2C setup, just pin mode.
  pinMode(PIN_PHOTOCELL, INPUT);
  Serial.println(F("TCRT5000 photocell ready on A2 (analog)."));
}

int readProximity() {
  // NOTE: untested detection direction on your specific module — if
  // this triggers backwards (detects when nothing is there), swap
  // the comparison direction wherever PHOTOCELL_THRESHOLD is used.
  return analogRead(PIN_PHOTOCELL); // raw 0-1023, matches PHOTOCELL_THRESHOLD's scale directly
}



// ----------------------- PCF8574 keypad scan -----------------------
// Rows P0-P3 driven as outputs (one LOW at a time), columns P4-P7 read as
// inputs with pull-ups. Standard 4x4 matrix scan, just bit-banged over I2C
// instead of native digitalRead/digitalWrite.

const char KEYPAD_MAP[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

void pcf8574_write(uint8_t value) {
  Wire.beginTransmission(PCF8574_ADDR);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t pcf8574_read() {
  Wire.requestFrom((int)PCF8574_ADDR, 1);
  if (Wire.available()) return Wire.read();
  return 0xFF;
}

void setupKeypadExpander() {
  // PCF8574 quasi-bidirectional I/O: writing 1 to a pin makes it a weak-pullup
  // input; writing 0 drives it low. Initialize columns (P4-P7) high (input
  // mode) and rows (P0-P3) high too (idle); we'll pull one row low at a time
  // during scanning.
  pcf8574_write(0xFF);
}

char scanKeypad() {
  for (uint8_t row = 0; row < 4; row++) {
    // Drive only this row low, keep all other rows + all columns high (input/pullup)
    uint8_t rowMask = ~(1 << row) & 0x0F;       // rows = P0-P3
    uint8_t output = (rowMask & 0x0F) | 0xF0;    // columns P4-P7 stay high
    pcf8574_write(output);
    delayMicroseconds(50); // let the line settle

    uint8_t colState = pcf8574_read() >> 4;      // columns are the upper nibble
    for (uint8_t col = 0; col < 4; col++) {
      if (!(colState & (1 << col))) {
        delay(15); // debounce the initial contact bounce
        char pressedKey = KEYPAD_MAP[row][col];

        // Wait here until the key is actually released. Without this,
        // a single physical press (100-300ms+) gets detected again
        // every ~15ms for as long as the finger is still on the key,
        // corrupting every PIN entry with repeated digits.
        while (true) {
          pcf8574_write(output);
          delayMicroseconds(50);
          uint8_t stillPressed = (pcf8574_read() >> 4) & (1 << col);
          if (stillPressed) break; // bit reads 1 again = released
          delay(10);
        }
        delay(20); // extra debounce after release

        pcf8574_write(0xFF);
        return pressedKey;
      }
    }
  }
  pcf8574_write(0xFF);
  return '\0'; // no key pressed
}

// ----------------------- HX711 load cell (floor pad) -----------------------
// Repurposed from the old fire-protection tank-weight reading: this HX711 +
// load cell now sits under a floor pad in front of the door. It works
// together with the TCRT5000 proximity sensor above — pollProximityAndKeypad()
// below requires BOTH a proximity trigger AND a detected mass on the pad
// before prompting for a password. WEIGHT_DETECT_THRESHOLD_G comes from
// CalibrationConstants.h.

float readWeightGrams() {
  if (scale.is_ready()) {
    return scale.get_units(3); // average of 3 reads, in grams (after set_scale calibration)
  }
  return lastWeightG; // HX711 not ready this tick, reuse last good value
}

// ----------------------- Entry security state -----------------------
String pinBuffer = "";
unsigned long lastProximityTrigger = 0;
// PROXIMITY_DEBOUNCE_MS now comes from CalibrationConstants.h

void openDoorServo() {
  doorServo.write(180);  // open position, adjust to your mechanism
  delay(DOOR_OPEN_MS);   // "Wait 5 seconds" per the algorithm doc
  doorServo.write(0);    // close position
}

void pollProximityAndKeypad() {
  int prox = readProximity();
  bool proximityDetected = prox < PHOTOCELL_THRESHOLD;

  lastWeightG = readWeightGrams();
  bool weightDetected = lastWeightG > WEIGHT_DETECT_THRESHOLD_G;

  // Both sensors must agree — proximity alone (or weight alone) is not
  // enough. This cuts down on false triggers from either sensor on its own
  // (e.g. someone walking past without stepping on the pad, or something
  // being set down on the pad without anyone actually being at the door).
  bool personDetected = proximityDetected && weightDetected;

  unsigned long now = millis();
  if (personDetected && (now - lastProximityTrigger > PROXIMITY_DEBOUNCE_MS)) {
    lastProximityTrigger = now;
    Serial.println(F("Proximity + weight pad: person detected, prompting for password."));
    dfPlayer.play(1); // "PLEASE ENTER PASSWORD"
    pinBuffer = "";
    waitForPinEntry();
  }
}

void waitForPinEntry() {
  // Blocking wait for keypad input, matching the algorithm doc's
  // "Wait for password input from keypad" step. Times out after 15s so a
  // person walking away doesn't freeze the whole loop() forever.
  unsigned long start = millis();
  const unsigned long TIMEOUT_MS = KEYPAD_TIMEOUT_MS; // from CalibrationConstants.h

  while (millis() - start < TIMEOUT_MS) {
    char key = scanKeypad();
    if (key != '\0') {
      start = millis(); // reset timeout on each keypress
      if (key == '#') {
        submitPin();
        return;
      } else if (key == '*') {
        pinBuffer = ""; // clear entry
      } else if (key >= '0' && key <= '9') {
        pinBuffer += key;
        if (pinBuffer.length() > 8) pinBuffer = pinBuffer.substring(pinBuffer.length() - 8);
      }
      // 'A','B','C','D' reserved for future use (e.g. admin menu), ignored here
    }
  }
  Serial.println(F("Keypad entry timed out."));
}

KeypadResult checkPinLocally(const String &entered) {
  // Fully standalone check against DOOR_PIN in CalibrationConstants.h —
  // there is no server, so the Arduino owns the PIN and the wrong-attempt
  // count itself.
  if (entered == DOOR_PIN) {
    return PIN_ACCEPTED;
  }
  wrongAttempts++;
  if (wrongAttempts >= MAX_WRONG_ATTEMPTS) {
    return PIN_THIEF_ALARM;
  }
  return PIN_DENIED;
}

void submitPin() {
  Serial.print(F("Submitting PIN: "));
  Serial.println(pinBuffer);

  KeypadResult result = checkPinLocally(pinBuffer);

  if (result == PIN_ACCEPTED) {
    wrongAttempts = 0;
    silenceBuzzer(); // clears any buzzer left on from a prior thief alarm - access is back to normal
    dfPlayer.play(2); // "ACCESS ACCEPTED"
    openDoorServo();
  } else if (result == PIN_THIEF_ALARM) {
    wrongAttempts = 0; // reset the counter now that the alarm has fired
    dfPlayer.play(4); // "ACCESS DENIED, THIEF ATTEMPT"
    soundBuzzerAlarm(); // audible alert on thief detection
    // Door stays locked - do not call openDoorServo()
  } else {
    // PIN_DENIED - wrong PIN, but under the attempt limit
    dfPlayer.play(3); // "ACCESS DENIED"
  }
  pinBuffer = "";
}

void soundBuzzerAlarm() {
  digitalWrite(PIN_BUZZER, HIGH);
}

void silenceBuzzer() {
  digitalWrite(PIN_BUZZER, LOW);
}
