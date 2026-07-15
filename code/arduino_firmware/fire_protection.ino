/*
 * fire_protection.ino — part of the Biological Laboratory Monitor firmware
 *
 * Implements System 2, updated fire-suppression sequence:
 *   Read temp/humidity continuously
 *   IF temp > FIRE_THRESHOLD: alarm LED + buzzer, pump ON
 *     Pump runs for exactly PUMP_RUN_MS (55s):
 *       IF flow == 0 at any point while running: pump OFF, buzzer,
 *         "PIPE BLOCKAGE DETECTED", stop sequence
 *     After 55s: pump OFF, humidifier ON
 *     Humidifier runs for exactly HUMIDIFIER_RUN_MS (10s), regardless
 *       of the current ambient humidity reading
 *     After 10s: humidifier OFF, buzzer silenced, "SYSTEM STABLE"
 *
 * This runs entirely standalone on the Arduino — this is a physical
 * safety system, so the water/humidifier response must never depend on
 * a network connection. There is no server or dashboard to report to;
 * the Arduino is the sole authority for this sequence.
 *
 * NOTE: there is no tank/tank-weight concept in this system. The HX711 +
 * load cell is used exclusively as an entry-security floor pad — see
 * entry_security.ino.
 */

void flowPulseISR() {
  flowPulseCount++;
}

bool fanRunning = false; // hysteresis state for the ambient-humidity fan control below

// Independent of the fire-suppression sequence and the humidifier: this
// just watches ambient humidity on its own.
//   - humidity > HUMIDITY_FAN_ON_PCT (60%)      -> fan ON
//   - humidity <= NORMAL_HUMIDITY_PCT (from CalibrationConstants.h) -> fan OFF
// Uses hysteresis (separate on/off thresholds) so the fan doesn't chatter
// on/off right at one boundary value.
void runHumidityFanControl() {
  if (!fanRunning && lastHumidity > HUMIDITY_FAN_ON_PCT) {
    Serial.println(F("Humidity above fan threshold - fan ON."));
    setRelay(PIN_RELAY_FAN, true);
    fanRunning = true;
  } else if (fanRunning && lastHumidity <= NORMAL_HUMIDITY_PCT) {
    Serial.println(F("Humidity back to normal - fan OFF."));
    setRelay(PIN_RELAY_FAN, false);
    fanRunning = false;
  }
}

float computeFlowLpm() {
  noInterrupts();
  unsigned long pulses = flowPulseCount;
  flowPulseCount = 0;
  interrupts();
  // pulses counted over SENSOR_POST_INTERVAL (5000ms) -> convert to L/min
  float pulsesPerSecond = pulses / (SENSOR_POST_INTERVAL / 1000.0);
  return pulsesPerSecond / FLOW_CALIBRATION_FACTOR;
}

void setRelay(int pin, bool on) {
  // Active-LOW relay assumption (see setup() comment in arduino_firmware.ino)
  digitalWrite(pin, on ? LOW : HIGH);
}

// ----------------------- Fire-event buzzer (5s, non-blocking) -----------------------
// Starts the buzzer and its 5s timer the moment fire is detected. Does NOT
// use delay() - the actual cutoff happens in updateFireBuzzer(), called
// every loop() iteration, so this never blocks keypad entry or anything else.
void startFireBuzzer() {
  soundBuzzerAlarm();
  fireBuzzerActive = true;
  fireBuzzerStartMs = millis();
}

// Called every loop() iteration. Once FIRE_BUZZER_ON_MS (5s) has elapsed
// since the buzzer started, turns it off automatically - independent of
// what fireStage the pump/humidifier sequence is currently in.
void updateFireBuzzer() {
  if (fireBuzzerActive && (millis() - fireBuzzerStartMs >= FIRE_BUZZER_ON_MS)) {
    silenceBuzzer();
    fireBuzzerActive = false;
  }
}

/*
 * Advances the local fire-protection state machine by one step, using
 * the most recent DHT11/HX711/flow readings. Called once per
 * SENSOR_POST_INTERVAL from loop(), same cadence as the readings
 * themselves so the sequence can't race ahead of real sensor data.
 */
void runFireProtectionStep() {
  lastTempC = dht.readTemperature();
  lastHumidity = dht.readHumidity();
  currentFlowLpm = computeFlowLpm();

  if (isnan(lastTempC) || isnan(lastHumidity)) {
    Serial.println(F("WARNING: DHT11 read failed this cycle, skipping fire-sequence step."));
    return;
  }

  runHumidityFanControl(); // independent of fireStage - just watches ambient humidity

  switch (fireStage) {

    case IDLE:
      if (lastTempC > FIRE_THRESHOLD_C) {
        Serial.println(F("FIRE DETECTED - starting suppression sequence."));
        dfPlayer.play(5); // "HIGH TEMPERATURE DETECTED"
        digitalWrite(PIN_STATUS_LED, HIGH); // alarm LED
        startFireBuzzer(); // buzzer sounds for a fixed 5s (FIRE_BUZZER_ON_MS), then
                            // auto-silences via updateFireBuzzer() in loop() - independent
                            // of the pump, which starts separately and runs its own 55s
        fireStage = FIRE_DETECTED;
      }
      break;

    case FIRE_DETECTED:
      setRelay(PIN_RELAY_PUMP, true);
      pumpStartMs = millis(); // start the 55s pump timer
      fireStage = PUMPING;
      break;

    case PUMPING:
      setRelay(PIN_RELAY_PUMP, true);
      if (currentFlowLpm <= FLOW_BLOCKAGE_LPM) {
        Serial.println(F("PIPE BLOCKAGE - stopping pump, halting sequence."));
        setRelay(PIN_RELAY_PUMP, false);
        dfPlayer.play(6); // "PIPE BLOCKAGE DETECTED"
        soundBuzzerAlarm();
        fireStage = BLOCKED;
      } else if (millis() - pumpStartMs >= PUMP_RUN_MS) {
        // Pump has run for its fixed 55s window - hand off to the humidifier,
        // which now also runs for its own fixed window (HUMIDIFIER_RUN_MS).
        Serial.println(F("Pump run complete (55s) - switching to humidifier."));
        setRelay(PIN_RELAY_PUMP, false);
        setRelay(PIN_RELAY_HUMIDIFIER, true);
        humidifierStartMs = millis(); // start the 10s humidifier timer
        fireStage = HUMIDIFYING;
      }
      break;

    case BLOCKED:
      // Stays here until someone has physically fixed the pipe and the
      // system is reset (e.g. power-cycled) - a blockage is a physical
      // fault, so there's no automatic or remote way to clear it.
      setRelay(PIN_RELAY_PUMP, false);
      break;

    case HUMIDIFYING:
      setRelay(PIN_RELAY_HUMIDIFIER, true);
      if (millis() - humidifierStartMs >= HUMIDIFIER_RUN_MS) {
        Serial.println(F("Humidifier run complete (10s) - sequence stable."));
        setRelay(PIN_RELAY_HUMIDIFIER, false);
        dfPlayer.play(7); // "SYSTEM STABLE"
        digitalWrite(PIN_STATUS_LED, LOW);
        silenceBuzzer();     // safety net - the fire-event buzzer already auto-silenced
        fireBuzzerActive = false; // itself after 5s via updateFireBuzzer(), this just
                                   // guarantees it's off by the time we report "stable"
        fireStage = STABLE;
      }
      break;

    case STABLE:
      fireStage = IDLE;
      break;
  }
}

const char* fireStageName() {
  switch (fireStage) {
    case IDLE: return "IDLE";
    case FIRE_DETECTED: return "FIRE_DETECTED";
    case PUMPING: return "PUMPING";
    case BLOCKED: return "BLOCKED";
    case HUMIDIFYING: return "HUMIDIFYING";
    case STABLE: return "STABLE";
    default: return "UNKNOWN";
  }
}
