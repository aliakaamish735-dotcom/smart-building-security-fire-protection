/*
 * Biological Laboratory Monitor + Security System (Sensors Project)
 * Smart Building Security and Fire Protection System
 * Board: Arduino Uno R3 (standalone — no WiFi/network module)
 *
 * This sketch is split across multiple .ino files in this same folder —
 * the Arduino IDE compiles all .ino files in a sketch folder together,
 * so just open arduino_firmware.ino and the rest load automatically:
 *   arduino_firmware.ino   - pin config, setup(), loop() (this file)
 *   entry_security.ino     - TCRT5000 photocell, HX711+load cell floor pad, keypad (via PCF8574), servo, DFPlayer, buzzer
 *   fire_protection.ino    - DHT11, pump, humidifier, fan, flow sensor
 *
 * ---------------------------------------------------------------
 * REQUIRED LIBRARIES (Arduino IDE -> Library Manager):
 *   - "DHT sensor library" by Adafruit (+ "Adafruit Unified Sensor")
 *   - "HX711" by Bogdan Necula (bogde/HX711)
 *   - "Servo" (built into the IDE, no install needed)
 *   - "DFRobotDFPlayerMini" by DFRobot
 *   No PCF8574 library needed - the keypad scan is bit-banged directly
 *   over I2C (Wire.h, built in) so there's one less dependency to manage.
 * ---------------------------------------------------------------
 *
 * FULL WIRING — matches Circuit_summary.txt exactly (do not add/omit pins):
 *
 *   Arduino Uno pin map:
 *     D0, D1   -> reserved, left UNCONNECTED (USB serial / CSV debug output)
 *     D2       -> YF-S201/YF-B1 flow sensor, yellow signal wire (interrupt)
 *     D3       -> Active buzzer signal/IN
 *     D4       -> Pump relay IN            (5V pump relay, active-LOW)
 *     D5       -> SG90 servo signal (door lock)
 *     D6       -> DFPlayer Mini TX  (Arduino D6 = RX, receives from DFPlayer)
 *     D7       -> DFPlayer Mini RX, THROUGH A 1kOhm RESISTOR (Arduino D7 = TX)
 *     D8       -> DHT11 DATA
 *     D9       -> Humidifier relay IN (24V humidifier, active-LOW)
 *     D10      -> Fan relay IN (12V fan, active-LOW)
 *     A0       -> HX711 DT/DOUT
 *     A1       -> HX711 SCK/CLK
 *     A2       -> TCRT5000 IR reflective sensor, AO (analog reflectance output)
 *     A4/SDA   -> PCF8574 SDA (keypad expander only)
 *     A5/SCL   -> PCF8574 SCL (keypad expander only)
 *
 *   TCRT5000: VCC->5V, GND->GND, AO->A2. The module's own "DO" pin is
 *     intentionally NOT connected — its label coincidentally matches
 *     Arduino's D0 (USB/Serial RX), which is a completely different
 *     pin; wiring them together would conflict with Serial Monitor
 *     and uploads. One wire (AO) is enough for presence detection.
 *
 *   PCF8574 (I2C addr 0x20): SDA->A4, SCL->A5, VCC->5V, GND->GND,
 *     A0/A1/A2 -> GND (sets address 0x20).
 *     4x4 keypad: P0-P3 = rows R1-R4, P4-P7 = columns C1-C4.
 *
 *   SG90 servo: signal->D5, red->EXTERNAL regulated 5V supply (not the
 *     Arduino 5V pin), brown/black->external supply GND (tied to Arduino GND).
 *
 *   DFPlayer Mini: TX->D6, RX->D7 (through 1k resistor), VCC->5V, GND->GND,
 *     speaker on SPK1/SPK2, microSD card inserted with numbered MP3 tracks.
 *
 *   DHT11: VCC->5V, DATA->D8, GND->GND (add a 4.7k-10k pull-up resistor
 *     between DATA and 5V if using a bare 4-pin DHT11, not needed on the
 *     common 3-pin breakout module which already has one built in).
 *
 *   HX711 + load cell: DT->A0, SCK->A1, VCC->5V, GND->GND. Load cell wires
 *     E+/E-/A+/A- go to the HX711's matching labeled pads (use the load
 *     cell's own wire labels, not color assumptions). This load cell sits
 *     under a floor pad in front of the door (entry security), NOT a tank —
 *     see entry_security.ino.
 *
 *   YF-S201/YF-B1 flow sensor: red->5V, black->GND, yellow signal->D2.
 *
 *   Active buzzer (3-pin module): signal/IN->D3, VCC->5V, GND->GND.
 *
 *   3-channel relay module, ALL THREE CHANNELS ACTIVE-LOW:
 *     Control side: CH1(pump) IN->D4, CH2(humidifier) IN->D9, CH3(fan) IN->D10,
 *       VCC->Arduino 5V, GND->Arduino GND.
 *     Power side (use NO terminals only, never NC):
 *       Pump:       external 5V supply(+) -> COM, NO -> pump(+), pump(-) -> supply(-)
 *       Humidifier: 24V adapter(+) -> COM, NO -> humidifier(+), humidifier(-) -> adapter(-)
 *       Fan:        external 12V supply(+) -> COM, NO -> fan(+), fan(-) -> supply(-)
 *
 *   Status/alarm LED: D13 (with the Uno's onboard 220-1k series resistor
 *     already built into most Uno boards on pin 13, or add your own).
 *
 *   POWER & SAFETY:
 *     - Never power the pump, fan, humidifier, or servo from the Arduino's
 *       5V pin. Use a regulated external 5V supply for servo + pump, 12V
 *       for the fan, and the proper 24V adapter for the humidifier.
 *     - Never connect 12V or 24V to the Arduino 5V rail.
 *     - Tie ALL low-voltage control grounds together: Arduino GND, sensor
 *       GND, PCF8574 GND, HX711 GND, DFPlayer GND, relay control-side GND,
 *       servo external-supply GND.
 *     - Keep pump/fan/humidifier HIGH-power wiring physically separated
 *       from the Arduino's low-voltage signal wiring.
 *
 * Common ground: EVERY module's GND must be tied together, including the
 * external 24V humidifier adapter's ground, or readings and serial comms
 * will be unreliable.
 * ---------------------------------------------------------------
 */

#include <Wire.h>
#include <Servo.h>
#include <DHT.h>
#include <HX711.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "CalibrationConstants.h"

// All calibration factors, thresholds, timing constants, and the door
// PIN now live in CalibrationConstants.h — see that file before wiring
// anything up. Nothing below this line should need editing for a
// normal deployment. This system is fully standalone: no WiFi, no
// ESP-01, no server.

// ----------------------- PIN MAP (matches Circuit_summary.txt) -----------------------
#define PIN_FLOW        2   // YF-S201/YF-B1 flow sensor signal (interrupt)
#define PIN_BUZZER      3   // Active buzzer signal
#define PIN_RELAY_PUMP       4   // Pump relay IN (active-LOW)
#define PIN_SERVO       5   // SG90 servo signal (door lock)
#define PIN_DFPLAYER_RX 6   // Arduino RX <- DFPlayer TX
#define PIN_DFPLAYER_TX 7   // Arduino TX -> DFPlayer RX (through 1k resistor)
#define PIN_DHT         8   // DHT11 data
#define PIN_RELAY_HUMIDIFIER 9    // Humidifier relay IN (active-LOW)
#define PIN_RELAY_FAN        10   // Fan relay IN (active-LOW)

#define PIN_STATUS_LED  13
#define PIN_HX711_DT    A0
#define PIN_HX711_SCK   A1
#define PIN_PHOTOCELL   A2   // TCRT5000 IR reflective sensor (analog reflectance output)
// A4 (SDA) / A5 (SCL) used implicitly by Wire.h for the PCF8574 keypad expander

#define PCF8574_ADDR    0x20

#define DHTTYPE DHT11
// ---------------------------------------------------------

// ----------------------- SHARED OBJECTS -----------------------
DHT dht(PIN_DHT, DHTTYPE);
HX711 scale;
Servo doorServo;

SoftwareSerial dfPlayerSerial(PIN_DFPLAYER_TX, PIN_DFPLAYER_RX); // (RX, TX) for talking to DFPlayer
DFRobotDFPlayerMini dfPlayer;



// ----------------------- SHARED STATE -----------------------
volatile unsigned long flowPulseCount = 0;
float currentFlowLpm = 0.0;
// FLOW_CALIBRATION_FACTOR now comes from CalibrationConstants.h

int wrongAttempts = 0;
// MAX_WRONG_ATTEMPTS now comes from CalibrationConstants.h

enum FireStage { IDLE, FIRE_DETECTED, PUMPING, BLOCKED, HUMIDIFYING, STABLE };
enum KeypadResult { PIN_ACCEPTED, PIN_DENIED, PIN_THIEF_ALARM }; // used by entry_security.ino, defined here so it exists before entry_security.ino needs it — Arduino concatenates tabs alphabetically after the main file
FireStage fireStage = IDLE;

float lastTempC = NAN;
float lastHumidity = NAN;
float lastWeightG = 0.0; // most recent HX711 reading from the entry-security floor pad
                          // (see readWeightGrams() / pollProximityAndKeypad() in
                          // entry_security.ino) - there is no tank in this system.

// FIRE_THRESHOLD_C, NORMAL_TEMP_C, NORMAL_HUMIDITY_PCT,
// PUMP_RUN_MS, HUMIDIFIER_RUN_MS, SENSOR_POST_INTERVAL all come from CalibrationConstants.h now.
unsigned long lastSensorPost = 0;
unsigned long pumpStartMs = 0; // set when the pump turns on; pump runs for PUMP_RUN_MS then hands off to the humidifier
unsigned long humidifierStartMs = 0; // set when the humidifier turns on; runs for a fixed HUMIDIFIER_RUN_MS then turns off

// Fire-event buzzer: sounds for exactly FIRE_BUZZER_ON_MS (5s) starting the
// moment fire is detected, then turns itself off - independent of the pump/
// humidifier stages, and checked every loop() iteration (non-blocking) so
// it never delays keypad entry. See updateFireBuzzer() in fire_protection.ino.
bool fireBuzzerActive = false;
unsigned long fireBuzzerStartMs = 0;

void flowPulseISR(); // defined in fire_protection.ino

// ---------------------------------------------------------
void setup() {
  Serial.begin(9600);
  Serial.println(F("\nBiological Laboratory Monitor - Arduino firmware starting..."));

  Wire.begin();

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  pinMode(PIN_RELAY_PUMP, OUTPUT);
  pinMode(PIN_RELAY_HUMIDIFIER, OUTPUT);
  pinMode(PIN_RELAY_FAN, OUTPUT);
  // Relay modules are active-LOW: HIGH = off, LOW = energizes the coil.
  digitalWrite(PIN_RELAY_PUMP, HIGH);
  digitalWrite(PIN_RELAY_HUMIDIFIER, HIGH);
  digitalWrite(PIN_RELAY_FAN, HIGH);

  pinMode(PIN_STATUS_LED, OUTPUT);

  pinMode(PIN_FLOW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW), flowPulseISR, FALLING);

  dht.begin();

  scale.begin(PIN_HX711_DT, PIN_HX711_SCK);
  // LOADCELL_SCALE_FACTOR lives in CalibrationConstants.h — replace the
  // placeholder there with your real calibration result, not here.
  scale.set_scale(LOADCELL_SCALE_FACTOR);
  scale.tare(); // zero the scale with nothing on the floor pad

  doorServo.attach(PIN_SERVO);
  doorServo.write(0); // 0 degrees = closed position, adjust to your mechanism

  dfPlayerSerial.begin(9600);
  if (dfPlayer.begin(dfPlayerSerial)) {
    dfPlayer.volume(24); // 0-30
    Serial.println(F("DFPlayer Mini ready."));
  } else {
    Serial.println(F("WARNING: DFPlayer Mini not detected. Check wiring/SD card."));
  }

  setupKeypadExpander(); // in entry_security.ino
  setupApds9960();       // in entry_security.ino



  // CSV header — everything after this point that starts with "DATA,"
  // is one row of this table. Capture Serial Monitor output (or log it
  // with a serial terminal tool) and strip non-"DATA," lines to get a
  // clean CSV for Excel/MATLAB/Python analysis. See
  // /data/README.md for the matching column definitions.
  Serial.println(F("CSV_HEADER,millis_ts,temp_c,humidity_pct,pad_weight_g,flow_lpm,proximity,fire_stage,wrong_attempts"));
}

// ---------------------------------------------------------
void printCsvRow() {
  Serial.print(F("DATA,"));
  Serial.print(millis());
  Serial.print(',');
  Serial.print(isnan(lastTempC) ? -1.0 : lastTempC, 2);
  Serial.print(',');
  Serial.print(isnan(lastHumidity) ? -1.0 : lastHumidity, 2);
  Serial.print(',');
  Serial.print(lastWeightG, 1);
  Serial.print(',');
  Serial.print(currentFlowLpm, 3);
  Serial.print(',');
  Serial.print(readProximity() > PHOTOCELL_THRESHOLD ? 1 : 0);
  Serial.print(',');
  Serial.print(fireStageName());
  Serial.print(',');
  Serial.println(wrongAttempts);
}

// ---------------------------------------------------------
void loop() {
  // Entry system is polled every single loop() iteration, completely
  // independent of fireStage - the keypad still accepts and checks a
  // password normally even while a fire event/pump/humidifier sequence
  // is in progress.
  pollProximityAndKeypad(); // in entry_security.ino, handles the entry algorithm

  updateFireBuzzer(); // in fire_protection.ino, non-blocking 5s buzzer cutoff

  unsigned long now = millis();

  if (now - lastSensorPost >= SENSOR_POST_INTERVAL) {
    lastSensorPost = now;
    runFireProtectionStep(); // in fire_protection.ino, advances the local state machine
    Serial.print(F("Fire stage: "));
    Serial.println(fireStageName());
    printCsvRow();            // machine-readable CSV row for offline analysis/plotting
  }
}
