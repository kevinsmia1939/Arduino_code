#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// ---------------- Pins ----------------
const uint8_t DIR_PIN       = 2;     // Direction
const uint8_t STEP_OC1A_PIN = 9;     // STEP must be on OC1A (D9) for hardware toggle

// Rotary encoder pins
const uint8_t ENC_CLK = 4;
const uint8_t ENC_DT  = 5;
const uint8_t ENC_SW  = 6;
const uint8_t RUN_INPUT_A0 = A0;      // HIGH = run request
const uint8_t RUN_INPUT_A1 = A1;      // HIGH = run request
const uint8_t MOTOR_STATE_PIN = 3;    // LOW when motor ON, HIGH when motor OFF

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- Motor setup ----------------
const int step_rev  = 400;   // steps per revolution (set to YOUR motor's full-step count)
const int microstep = 1;     // 1 if full-step; 2/4/8/16 if microstepping

// ---------------- Calibration ----------------
// Adjust if needed
const float CLOCK_CORR = 1.002f;   // multiply step frequency by this

// ---------------- Speed control ----------------
const float HZ_STEP = 0.1f;    // encoder step (Hz)
const float HZ_MIN  = 0.0f;
const float HZ_MAX  = 50.0f;   // adjust for your mechanics

// Target vs actual speed (RAMPED)
volatile float targetHz = 1.0f;  // encoder sets this (rotations per second)
float currentHz = 0.0f;          // we ramp this toward targetHz

// Ramp tuning (Hz/s). 1 Hz = 60 RPM
const float ACCEL_HZ_PER_S = 0.15f;   // speed-up rate
const float DECEL_HZ_PER_S = 8.0f;   // slow-down rate (can be higher than accel)

// Ramp update interval (ms)
const uint16_t RAMP_UPDATE_MS = 5;
unsigned long lastRampUpdate = 0;

// Optional: don't rewrite Timer1 for tiny changes
const float APPLY_DEADBAND_HZ = 0.02f;
float lastAppliedHz = -999.0f;

// ---------------- Encoder state ----------------
int lastCLK = HIGH;
unsigned long lastBtnTime = 0;
const unsigned long btnDebounceMs = 200;

// ---------------- LCD throttling ----------------
unsigned long lastLcdUpdate = 0;
const unsigned long lcdIntervalMs = 120;

// ---------------- Timer1 (hardware toggle on OC1A) ----------------
// Output on D9: f_out = F_CPU / ( 2 * prescaler * (1 + OCR1A) )
const uint32_t F_CPU_HZ = 16000000UL;

void timer1EnableToggleOC1A(bool enable) {
  if (enable) {
    // Toggle OC1A on compare match: COM1A0=1, COM1A1=0
    TCCR1A = (TCCR1A & ~(_BV(COM1A1))) | _BV(COM1A0);
  } else {
    // Disconnect OC1A
    TCCR1A &= ~(_BV(COM1A1) | _BV(COM1A0));
    digitalWrite(STEP_OC1A_PIN, LOW);
  }
}

void timer1SetupBase() {
  pinMode(STEP_OC1A_PIN, OUTPUT);
  digitalWrite(STEP_OC1A_PIN, LOW);

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // CTC mode (WGM12 = 1)
  TCCR1B |= _BV(WGM12);

  // Prescaler set dynamically below
  timer1EnableToggleOC1A(false);
}

void setPrescalerBits(uint16_t presc) {
  // Clear CS12..CS10
  TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10));
  switch (presc) {
    case 1:    TCCR1B |= _BV(CS10); break;
    case 8:    TCCR1B |= _BV(CS11); break;
    case 64:   TCCR1B |= _BV(CS11) | _BV(CS10); break;
    case 256:  TCCR1B |= _BV(CS12); break;
    case 1024: TCCR1B |= _BV(CS12) | _BV(CS10); break;
  }
}

// Return true if OCR fits; output rounded OCR to ocrOut
bool computeOCR1A(uint32_t presc, float fout, uint16_t &ocrOut) {
  if (fout < 0.1f) fout = 0.1f; // guard
  float ocr = (float)F_CPU_HZ / (2.0f * (float)presc * fout) - 1.0f;
  if (ocr < 1.0f || ocr > 65535.0f) return false;
  ocrOut = (uint16_t)(ocr + 0.5f);
  return true;
}

// Choose prescaler for BEST RESOLUTION (fine -> coarse):
// prefer the smallest prescaler that yields a valid OCR in range and fairly large (>=~50).
void setStepFrequencyAuto(float f_step) {
  if (f_step <= 0.0f) {
    timer1EnableToggleOC1A(false);
    return;
  }

  const uint16_t prescList[] = {1, 8, 64, 256, 1024};   // fine -> coarse
  const uint16_t MIN_OCR_TARGET = 50;                   // aim for good resolution
  uint16_t chosenPresc = 1024;
  uint16_t chosenOCR   = 65535;
  bool ok = false;

  // First pass: pick first prescaler giving OCR >= MIN_OCR_TARGET
  for (uint8_t i = 0; i < sizeof(prescList)/sizeof(prescList[0]); ++i) {
    uint16_t ocr;
    if (computeOCR1A(prescList[i], f_step, ocr)) {
      if (ocr >= MIN_OCR_TARGET) {
        chosenPresc = prescList[i];
        chosenOCR   = ocr;
        ok = true;
        break;
      }
    }
  }

  // Second pass: if none had OCR >= target, pick the finest that still fits [1..65535]
  if (!ok) {
    for (uint8_t i = 0; i < sizeof(prescList)/sizeof(prescList[0]); ++i) {
      uint16_t ocr;
      if (computeOCR1A(prescList[i], f_step, ocr)) {
        chosenPresc = prescList[i];
        chosenOCR   = ocr;
        ok = true;
        break;
      }
    }
  }

  // If still no fit (extreme frequencies), clamp to fastest reasonable
  if (!ok) {
    chosenPresc = 1;
    chosenOCR   = 1;
  }

  setPrescalerBits(chosenPresc);
  OCR1A = chosenOCR;
  timer1EnableToggleOC1A(true);
}

// Apply rotational frequency (Hz = rps) -> step frequency with calibration
// NOTE: this applies to TIMER only (does NOT store the user's target)
void applyHzToTimer(float hz_rps) {
  if (hz_rps <= 0.0f) {
    timer1EnableToggleOC1A(false);
    return;
  }

  // f_step = Hz * steps/rev * microstep
  float f_step = hz_rps * (float)step_rev * (float)microstep;

  // Apply board clock calibration so actual matches display
  f_step *= CLOCK_CORR;

  setStepFrequencyAuto(f_step);
}

float computeRPM(float hz_rps) {
  return hz_rps * 60.0f; // 1 Hz = 60 RPM
}

void setup() {
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, HIGH); // choose direction

  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT,  INPUT_PULLUP);
  pinMode(ENC_SW,  INPUT_PULLUP);
  pinMode(RUN_INPUT_A0, INPUT);
  pinMode(RUN_INPUT_A1, INPUT);
  pinMode(MOTOR_STATE_PIN, OUTPUT);
  digitalWrite(MOTOR_STATE_PIN, HIGH);
  lastCLK = digitalRead(ENC_CLK);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  timer1SetupBase();

  // Start from standstill and ramp up to target when running
  currentHz = 0.0f;
  lastAppliedHz = -999.0f;
  lastRampUpdate = millis();

  applyHzToTimer(0.0f);
}

void loop() {
  // --- Encoder rotation: +/-0.1 Hz per detent (sets TARGET only) ---
  int clkState = digitalRead(ENC_CLK);
  if (clkState != lastCLK) {
    if (digitalRead(ENC_DT) != clkState) targetHz += HZ_STEP;
    else                                 targetHz -= HZ_STEP;

    if (targetHz < HZ_MIN) targetHz = HZ_MIN;
    if (targetHz > HZ_MAX) targetHz = HZ_MAX;

    targetHz = roundf(targetHz * 10.0f) / 10.0f;  // keep single decimal
    lastCLK = clkState;
  }

  // --- Optional encoder button read (reserved for future use) ---
  if (digitalRead(ENC_SW) == LOW) {
    unsigned long now = millis();
    if (now - lastBtnTime > btnDebounceMs) {
      lastBtnTime = now;
    }
  }

  // --- A0/A1 control run/stop ---
  // ON if A0 or A1 is HIGH; OFF only when both are LOW
  bool runCommanded = (digitalRead(RUN_INPUT_A0) == HIGH) || (digitalRead(RUN_INPUT_A1) == HIGH);
  digitalWrite(MOTOR_STATE_PIN, runCommanded ? LOW : HIGH);

  // --- Ramp currentHz toward targetHz (or 0 when stopped) ---
  unsigned long now = millis();
  if (now - lastRampUpdate >= RAMP_UPDATE_MS) {
    float dt = (now - lastRampUpdate) / 1000.0f;
    lastRampUpdate = now;

    float desired = (runCommanded ? targetHz : 0.0f);
    float diff = desired - currentHz;

    if (diff > 0.0f) {
      float stepUp = ACCEL_HZ_PER_S * dt;
      if (diff > stepUp) currentHz += stepUp;
      else               currentHz  = desired;
    } else if (diff < 0.0f) {
      float stepDown = DECEL_HZ_PER_S * dt;
      if (-diff > stepDown) currentHz -= stepDown;
      else                  currentHz  = desired;
    }

    // Apply only if changed enough
    if (fabs(currentHz - lastAppliedHz) >= APPLY_DEADBAND_HZ) {
      applyHzToTimer(currentHz);
      lastAppliedHz = currentHz;
    }
  }

  // --- LCD update (throttled) ---
  if (now - lastLcdUpdate >= lcdIntervalMs) {
    lastLcdUpdate = now;

    // Show target as Hz, current as RPM (change if you prefer)
    float rpm = computeRPM(currentHz);

    lcd.setCursor(0, 0);
    lcd.print("Hz:");
    lcd.print(targetHz, 1);
    lcd.print("   "); // clear tail

    lcd.setCursor(0, 1);
    lcd.print("RPM:");
    lcd.print(rpm, 1);
    lcd.print("   "); // clear tail
  }
}
