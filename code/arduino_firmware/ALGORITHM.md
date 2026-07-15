# Firmware Algorithm — Smart Building Security and Fire Protection System

This document describes the **logic** of the firmware in plain pseudocode,
independent of Arduino syntax. It's meant to sit alongside `README.md`
(wiring/setup) as the "how it thinks" reference for the report's
Algorithm / Firmware Logic section.

The system is fully standalone (no WiFi/network) and runs two
independent subsystems every loop iteration: **Entry Security** and
**Fire Protection**. A third, smaller routine (**Ambient Humidity Fan
Control**) runs alongside Fire Protection but does not interact with it.

All numeric constants referenced below (in CAPS) live in
`CalibrationConstants.h` — nothing here is hard-coded in the driver files.

---

## 1. Main Loop

```
LOOP forever:
    POLL entry security (proximity + weight pad + keypad)   // every iteration, never skipped
    UPDATE fire-event buzzer timer                          // non-blocking, every iteration

    IF (time since last sensor read) >= SENSOR_POST_INTERVAL:
        RUN one fire-protection state-machine step
        LOG a CSV data row (temp, humidity, pad weight, flow, proximity, fire stage, wrong attempts)
```

Entry security is polled on **every single loop iteration**, regardless
of what the fire-protection state machine is doing. This guarantees a
fire event, a running pump, or a running humidifier never blocks
someone from unlocking the door.

---

## 2. Entry Security Algorithm

**Goal:** only prompt for a password when a person is *actually* at the
door — confirmed by two independent sensors — then check their PIN
locally and escalate to an alarm after repeated failures.

```
FUNCTION pollProximityAndKeypad():
    prox            ← read TCRT5000 IR proximity sensor (analog, 0-1023)
    proximityHit     ← prox < PHOTOCELL_THRESHOLD

    padWeight        ← read HX711 + load cell (grams)
    weightHit        ← padWeight > WEIGHT_DETECT_THRESHOLD_G

    personDetected   ← proximityHit AND weightHit      // BOTH sensors must agree

    IF personDetected AND (time since last trigger) > PROXIMITY_DEBOUNCE_MS:
        remember this trigger time
        PLAY "PLEASE ENTER PASSWORD"
        CALL waitForPinEntry()
```

```
FUNCTION waitForPinEntry():
    start a KEYPAD_TIMEOUT_MS countdown
    LOOP until timeout:
        key ← scan 4x4 keypad (via PCF8574 I2C expander)
        IF a key was pressed:
            reset the timeout countdown
            IF key == '#':          CALL submitPin(); RETURN
            ELSE IF key == '*':     clear the entry buffer
            ELSE IF key is 0-9:     append digit to entry buffer (max 8 digits)
            // 'A','B','C','D' reserved, currently ignored
    // loop exits silently on timeout (person walked away)
```

```
FUNCTION submitPin():
    IF entered PIN == stored DOOR_PIN:
        reset wrong-attempt counter
        silence any leftover alarm buzzer
        PLAY "ACCESS ACCEPTED"
        OPEN door servo, wait DOOR_OPEN_MS, CLOSE door servo

    ELSE:
        increment wrong-attempt counter
        IF wrong-attempt counter >= MAX_WRONG_ATTEMPTS:
            reset wrong-attempt counter
            PLAY "ACCESS DENIED, THIEF ATTEMPT"
            SOUND alarm buzzer (stays on — door stays locked)
        ELSE:
            PLAY "ACCESS DENIED"
```

**Key design points:**
- Proximity alone is not enough — the load-cell floor pad must also
  register a mass above `WEIGHT_DETECT_THRESHOLD_G` at the same time.
  This rejects false triggers from either sensor acting alone (someone
  walking past without stepping on the pad, or an object left on the
  pad with nobody at the door).
- `PROXIMITY_DEBOUNCE_MS` prevents the same person re-triggering the
  prompt repeatedly while still standing there.
- The wrong-PIN counter and thief-alarm decision are entirely local —
  there is no server, so the Arduino is the sole authority.

---

## 3. Fire Protection Algorithm

**Goal:** detect a fire via temperature, run a fixed water-pump cycle
(bailing out early on a pipe blockage), then run a fixed humidifier
cycle, then return to idle — all on fixed timers, independent of any
tank or humidity-level feedback.

State machine: `IDLE → FIRE_DETECTED → PUMPING → HUMIDIFYING → STABLE → IDLE`
(with a `BLOCKED` side-state if a blockage is detected mid-pump).

```
EVERY SENSOR_POST_INTERVAL:
    read temperature, humidity (DHT11)
    read flow rate (flow sensor pulse count → L/min, via FLOW_CALIBRATION_FACTOR)
    IF temp or humidity read failed: skip this step entirely

    RUN ambient-humidity fan control (independent — see section 4)

    SWITCH fireStage:

        CASE IDLE:
            IF temperature > FIRE_THRESHOLD_C:
                PLAY "HIGH TEMPERATURE DETECTED"
                TURN ON alarm LED
                START fire buzzer for a fixed FIRE_BUZZER_ON_MS (non-blocking,
                    auto-silences on its own timer every loop iteration)
                fireStage ← FIRE_DETECTED

        CASE FIRE_DETECTED:
            TURN ON pump
            START pump timer
            fireStage ← PUMPING

        CASE PUMPING:
            KEEP pump on
            IF flow rate <= FLOW_BLOCKAGE_LPM:
                TURN OFF pump
                PLAY "PIPE BLOCKAGE DETECTED"
                SOUND alarm buzzer
                fireStage ← BLOCKED                      // dead end, needs manual reset
            ELSE IF pump has run for PUMP_RUN_MS (55s):
                TURN OFF pump
                TURN ON humidifier
                START humidifier timer
                fireStage ← HUMIDIFYING

        CASE BLOCKED:
            KEEP pump off
            // stays here until the pipe is physically cleared and the
            // system is power-cycled/reset — no automatic recovery

        CASE HUMIDIFYING:
            KEEP humidifier on
            IF humidifier has run for HUMIDIFIER_RUN_MS (10s):
                TURN OFF humidifier
                PLAY "SYSTEM STABLE"
                TURN OFF alarm LED
                SILENCE buzzer (safety net)
                fireStage ← STABLE

        CASE STABLE:
            fireStage ← IDLE                             // ready to detect the next fire
```

**Key design points:**
- The pump (55s) and humidifier (10s) durations are both **fixed
  timers**, not conditions on flow, tank level, or humidity returning
  to normal — there is no tank in this system.
- The only early-exit path is a detected pipe blockage during pumping,
  which halts the whole sequence until a person physically clears it
  and resets the system.
- The 5-second fire buzzer is deliberately decoupled from the
  pump/humidifier timers — it always sounds for exactly 5s starting
  the instant fire is detected, checked every loop iteration so it
  never blocks keypad entry.

---

## 4. Ambient Humidity Fan Control (independent)

**Goal:** keep general ambient humidity in a comfortable range,
completely separate from the fire-suppression sequence above (it runs
every sensor cycle regardless of `fireStage`).

```
EVERY SENSOR_POST_INTERVAL (same cadence as fire protection):
    IF fan is OFF AND humidity > HUMIDITY_FAN_ON_PCT (60%):
        TURN ON fan
    ELSE IF fan is ON AND humidity <= NORMAL_HUMIDITY_PCT (70%):
        TURN OFF fan
```

Using two different thresholds (60% on / 70% off) is hysteresis — it
stops the fan from flickering on/off rapidly right at one boundary
value.

---

## 5. Constants Reference (all in `CalibrationConstants.h`)

| Constant | Meaning |
|---|---|
| `LOADCELL_SCALE_FACTOR` | HX711 raw-to-grams conversion (per-unit calibration) |
| `FLOW_CALIBRATION_FACTOR` | Flow sensor pulses/sec → L/min conversion |
| `FIRE_THRESHOLD_C` | Temperature that trips the fire sequence |
| `NORMAL_HUMIDITY_PCT` | Fan's turn-OFF humidity point (fan control only) |
| `HUMIDITY_FAN_ON_PCT` | Fan's turn-ON humidity point |
| `FLOW_BLOCKAGE_LPM` | Flow at/below this while pumping = blockage |
| `PUMP_RUN_MS` | Fixed pump run time (55,000 ms) |
| `HUMIDIFIER_RUN_MS` | Fixed humidifier run time (10,000 ms) |
| `FIRE_BUZZER_ON_MS` | Fixed fire-alert buzzer duration (5,000 ms) |
| `MAX_WRONG_ATTEMPTS` | Wrong PINs allowed before thief alarm |
| `PHOTOCELL_THRESHOLD` | TCRT5000 analog reading that counts as "person near door" |
| `WEIGHT_DETECT_THRESHOLD_G` | Load-cell floor-pad reading that counts as "someone standing on it" |
| `PROXIMITY_DEBOUNCE_MS` | Minimum time between password prompts |
| `KEYPAD_TIMEOUT_MS` | How long to wait for a full PIN entry |
| `DOOR_OPEN_MS` | How long the door stays unlocked |
| `SENSOR_POST_INTERVAL` | How often sensors are read + CSV-logged |
| `DOOR_PIN` | The accepted door PIN |
