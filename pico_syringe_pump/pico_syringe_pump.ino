/*
  Raspberry Pi Pico (Arduino C++) Syringe Pump
  - TB6600 (STEP/DIR/ENA)
  - NC limit switches (front/rear)
  - I2C 16x2 LCD (PCF8574 @ 0x27)
  - Rotary encoder to set flow (mL/min)
  - Start/Stop button (toggle)
  - Home button: reverse until rear limit, then stop

  Core: Earle Philhower RP2040 core (recommended)
  Install libs:
    - AccelStepper by Mike McCauley
    - Encoder by Paul Stoffregen
    - LiquidCrystal_I2C by Frank de Brabander
*/

#include <Arduino.h>
#include <Wire.h>
#include <AccelStepper.h>
#include <Encoder.h>
#include <LiquidCrystal_I2C.h>

/*************** USER CONSTANTS ***************/
// Mechanics
const float LEAD_MM_PER_REV   = 8.0f; // TR8x8 lead screw
const float SYRINGE_ID_MM     = 26.6f; // inner diameter (example)
const int   MOTOR_STEPS_REV   = 200;   // 1.8° motor
const int   MICROSTEP         = 8;     // TB6600 DIP

// Use empirical calibration (302 RPM = 100 mL/min)
const bool  USE_CALIBRATION   = true;
const float CAL_FLOW_PER_RPM  = 100.0f / 302.0f; // mL/min per RPM

// Motion/profile
const float MAX_RPM           = 600.0f;
const float ACCEL_RPM_PER_S   = 400.0f;   // ramp rate for our software ramp
const float MIN_RPM           = 0.0f;
const float HOMING_RPM        = 60.0f;    // reverse speed during homing

// Flow setpoint range & encoder behavior
const float FLOW_MIN_ML_MIN   = 0.0f;
const float FLOW_MAX_ML_MIN   = 200.0f;
const float FLOW_STEP_ML_MIN  = 1.0f;     // per detent
const int   ENCODER_COUNTS_PER_DETENT = 4; // many encoders: 4 counts per detent
const bool  ENCODER_INVERT    = false;

// LCD
const uint8_t LCD_ADDR = 0x27;
const int LCD_COLS = 16, LCD_ROWS = 2;

// Pins (use GP numbers directly with Philhower core)
const uint8_t PIN_STEP = 2;     // TB6600 PUL-
const uint8_t PIN_DIR  = 3;     // TB6600 DIR-
const uint8_t PIN_ENA  = 4;     // TB6600 ENA- (active LOW; ENA+ to +5V)
const uint8_t PIN_LIM_FWD = 6;  // NC -> GND (front/too close)
const uint8_t PIN_LIM_REV = 7;  // NC -> GND (rear/too far)
const uint8_t PIN_BTN_HOME = 10; // momentary to GND (active LOW)
const uint8_t PIN_ENC_A = 11;   // encoder A -> GND
const uint8_t PIN_ENC_B = 12;   // encoder B -> GND
const uint8_t PIN_BTN_RUN = 13; // start/stop toggle -> GND (active LOW)

// I2C pins (optional override to match your wiring, e.g., GP0/GP1 like in MicroPython)
const int I2C_SDA_PIN = 0;
const int I2C_SCL_PIN = 1;

/*************** DERIVED ***************/
const long  STEPS_PER_REV = (long)MOTOR_STEPS_REV * MICROSTEP;
const float AREA_MM2 = (float)M_PI * 0.25f * SYRINGE_ID_MM * SYRINGE_ID_MM;

/*************** GLOBALS ***************/
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// AccelStepper in DRIVER mode (step, dir)
AccelStepper stepper(AccelStepper::DRIVER, PIN_STEP, PIN_DIR);

// Encoder
Encoder enc(PIN_ENC_A, PIN_ENC_B);

// Buttons (simple debounce)
struct DebouncedButton {
  uint8_t pin;
  bool activeLow;
  uint16_t debounceMs;
  int lastRaw;
  int stable;
  uint32_t tEdge;
  void begin(uint8_t p, bool aLow=true, uint16_t d=30) {
    pin = p; activeLow = aLow; debounceMs = d;
    pinMode(pin, activeLow ? INPUT_PULLUP : INPUT);
    lastRaw = stable = digitalRead(pin);
    tEdge = millis();
  }
  bool pressed() {
    int v = digitalRead(pin);
    if (v != lastRaw) { lastRaw = v; tEdge = millis(); }
    if (millis() - tEdge > debounceMs) {
      if (stable != v) {
        stable = v;
        if ((activeLow && v == LOW) || (!activeLow && v == HIGH)) return true;
      }
    }
    return false;
  }
} btnHome, btnRun;

// State
volatile bool running = false;
bool currentForward = true; // we dispense forward
float flowSet_ml_min = 100.0f;
float curRPM = 0.0f;        // our current commanded RPM (we ramp to target)
float tgtRPM = 0.0f;

long lastEncCount = 0;
uint32_t lastLCDms = 0;
uint32_t lastRampMs = 0;

/*************** HELPERS ***************/
inline float rpmFromFlow(float flow) {
  if (USE_CALIBRATION) {
    return max(0.0f, flow / CAL_FLOW_PER_RPM);
  } else {
    if (LEAD_MM_PER_REV <= 0 || AREA_MM2 <= 0) return 0.0f;
    return max(0.0f, (flow * 1000.0f) / (LEAD_MM_PER_REV * AREA_MM2));
  }
}
inline float flowFromRPM(float rpm) {
  if (USE_CALIBRATION) {
    return max(0.0f, rpm * CAL_FLOW_PER_RPM);
  } else {
    return max(0.0f, (rpm * LEAD_MM_PER_REV * AREA_MM2) / 1000.0f);
  }
}
inline float spsFromRPM(float rpm) { // steps per second
  return (rpm * STEPS_PER_REV) / 60.0f;
}

void driverEnable(bool on) {
  pinMode(PIN_ENA, OUTPUT);
  digitalWrite(PIN_ENA, on ? LOW : HIGH); // active LOW enable
}

bool limFwdTriggered() { return digitalRead(PIN_LIM_FWD) == HIGH; } // NC: normal=LOW, triggered/broken=HIGH
bool limRevTriggered() { return digitalRead(PIN_LIM_REV) == HIGH; }

/*************** RAMPING ***************/
void updateRamp() {
  // Software ramp the current RPM toward the target RPM
  uint32_t now = millis();
  float dt = (now - lastRampMs) / 1000.0f;
  if (dt <= 0) return;
  lastRampMs = now;

  float maxDelta = ACCEL_RPM_PER_S * dt;
  if (curRPM < tgtRPM) {
    curRPM = min(tgtRPM, curRPM + maxDelta);
  } else if (curRPM > tgtRPM) {
    curRPM = max(tgtRPM, curRPM - maxDelta);
  }

  // Apply sign for direction
  float rpmSigned = currentForward ? curRPM : -curRPM;
  stepper.setSpeed(spsFromRPM(rpmSigned));  // steps/s signed
}

void stopAndHold() {
  curRPM = 0;
  tgtRPM = 0;
  stepper.setSpeed(0);
  // If you want to drop holding torque when paused:
  driverEnable(false);
}

/*************** HOMING ***************/
void doHome() {
  // Reverse slowly until rear limit triggers, then stop; optional nudge forward
  // Safety: if rear limit already triggered, just do a tiny forward nudge.
  driverEnable(true);
  currentForward = false;
  curRPM = 0;
  tgtRPM = HOMING_RPM;

  // Clear any previous speed
  stepper.setSpeed(0);

  // Run reverse until rear switch
  while (!limRevTriggered()) {
    // Interlocks: if front triggered while reversing (unlikely), just stop
    if (limFwdTriggered()) break;
    updateRamp();
    stepper.runSpeed(); // uses setSpeed()
    // Allow user to abort homing by pressing RUN (optional)
    if (btnRun.pressed()) break;
  }

  // Decelerate to stop
  tgtRPM = 0.0f;
  while (fabs(curRPM) > 1.0f) {
    updateRamp();
    stepper.runSpeed();
  }
  stopAndHold();

  // Optional: nudge off rear switch a little
  if (limRevTriggered()) {
    driverEnable(true);
    currentForward = true; // forward
    curRPM = 0; tgtRPM = 20.0f;
    uint32_t t0 = millis();
    while (limRevTriggered() && millis() - t0 < 400) {
      updateRamp();
      stepper.runSpeed();
    }
    tgtRPM = 0;
    while (fabs(curRPM) > 1.0f) { updateRamp(); stepper.runSpeed(); }
    stopAndHold();
  }

  // After homing, we are stopped.
}

/*************** SETUP ***************/
void setup() {
  // Pins
  pinMode(PIN_LIM_FWD, INPUT_PULLUP);
  pinMode(PIN_LIM_REV, INPUT_PULLUP);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_STEP, OUTPUT);
  driverEnable(false); // disabled initially

  // Buttons
  btnHome.begin(PIN_BTN_HOME, true, 30);
  btnRun.begin(PIN_BTN_RUN,  true, 30);

  // Wire on specified pins (match your wiring)
  Wire.setSDA(I2C_SDA_PIN);
  Wire.setSCL(I2C_SCL_PIN);
  Wire.begin();

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Syringe Pump");
  lcd.setCursor(0,1); lcd.print("Init...");

  // Stepper config
  stepper.setMaxSpeed(spsFromRPM(MAX_RPM));    // steps/s
  // We'll do our own acceleration ramping, so no need for setAcceleration here.
  stepper.setSpeed(0);

  lastLCDms = millis();
  lastRampMs = millis();
}

/*************** LOOP ***************/
void loop() {
  // --- Buttons ---
  if (btnHome.pressed()) {
    running = false;            // pause
    stopAndHold();
    doHome();                   // home to rear
  }

  if (btnRun.pressed()) {
    running = !running;
    if (!running) stopAndHold();
  }

  // --- Encoder -> flow setpoint ---
  long encNow = enc.read();
  long deltaCounts = encNow - lastEncCount;
  if (deltaCounts != 0) {
    lastEncCount = encNow;
    long detents = deltaCounts / ENCODER_COUNTS_PER_DETENT;
    if (detents != 0) {
      float dir = ENCODER_INVERT ? -1.0f : 1.0f;
      flowSet_ml_min += dir * detents * FLOW_STEP_ML_MIN;
      flowSet_ml_min = constrain(flowSet_ml_min, FLOW_MIN_ML_MIN, FLOW_MAX_ML_MIN);
    }
  }

  // --- Compute targets ---
  float desiredRPM = constrain(rpmFromFlow(flowSet_ml_min), MIN_RPM, MAX_RPM);

  // --- Interlocks ---
  if (limRevTriggered() && !currentForward) {  // forbid reverse while rear switch active
    running = false;
    stopAndHold();
    currentForward = true;
  }
  if (limFwdTriggered() && currentForward) {   // forbid forward while front switch active
    running = false;
    stopAndHold();
    currentForward = false;
  }

  // --- Command motion (always forward when running) ---
  if (running) {
    driverEnable(true);
    currentForward = true;         // dispense direction
    tgtRPM = desiredRPM;
  } else {
    tgtRPM = 0.0f;
  }

  // Ramping + step generation
  updateRamp();
  stepper.runSpeed(); // uses current setSpeed()

  // --- LCD ---
  if (millis() - lastLCDms >= 200) {
    lastLCDms = millis();
    float dispRPM  = fabs(curRPM);
    float dispFlow = flowFromRPM(dispRPM);
    lcd.setCursor(0,0);
    char line0[17]; snprintf(line0, sizeof(line0), "Flow:%6.1f mL/m", dispFlow);
    lcd.print(line0); if (strlen(line0) < 16) { for (int i=strlen(line0); i<16; ++i) lcd.print(' '); }

    lcd.setCursor(0,1);
    char line1[17]; snprintf(line1, sizeof(line1), "RPM :%6.1f %s", dispRPM, running ? "RUN " : "PAUS");
    lcd.print(line1); if (strlen(line1) < 16) { for (int i=strlen(line1); i<16; ++i) lcd.print(' '); }
  }
}
