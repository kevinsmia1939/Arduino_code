#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

/* ---------------- Pins ---------------- */
const uint8_t DIR_PIN   = 2;   // Direction to driver
const uint8_t STEP_PIN  = 3;   // STEP to driver (any pin; we drive it from Timer1 ISR)
const uint8_t ENC_CLK   = 4;   // Encoder CLK
const uint8_t ENC_DT    = 5;   // Encoder DT
const uint8_t ENC_SW    = 6;   // Encoder push (optional, active-LOW)

/* ---------------- LCD ---------------- */
LiquidCrystal_I2C lcd(0x27, 16, 2);   // change to 0x3F if needed

/* ---------------- Motor setup ---------------- */
// steps per revolution (full steps); set to YOUR motor. (e.g. 200 for 1.8°, 400 for 0.9°)
const int step_rev  = 400;
const int microstep = 1;              // driver DIP: 1,2,4,8,16,...

/* ---------------- Calibration ---------------- */
const float CLOCK_CORR = 1.002f;      // board clock tweak (or 1.0f if not needed)

/* ---------------- Speed control (RPM) ---------------- */
volatile float rpm = 60.0f;           // start RPM
volatile bool  running = true;

const float RPM_STEP = 1.0f;          // per encoder detent
const float RPM_MIN  = 0.0f;
const float RPM_MAX  = 3000.0f;       // adjust to your mechanics

/* ---------------- Encoder/button state ---------------- */
int lastCLK = HIGH;
unsigned long lastBtnTime = 0;
const unsigned long btnDebounceMs = 200;

/* ---------------- LCD throttling ---------------- */
unsigned long lastLcdUpdate = 0;
const unsigned long lcdIntervalMs = 120;
long lastShownRPM10 = -1;
bool lastShownRun   = !true;

/* ---------------- Timer1 (interrupt-driven) ----------------
   We use Timer1 in CTC mode to fire an ISR at the STEP toggle rate.
   Each ISR toggles STEP_PIN (50% duty square wave).
   Toggle rate = f_step * 2  (two toggles per full step period)

   OCR1A = F_CPU / (prescaler * toggle_freq) - 1
   where toggle_freq = 2 * f_step.
------------------------------------------------------------ */
const uint32_t F_CPU_HZ = 16000000UL;

// Current prescaler so we can reconfigure safely
uint16_t currentPrescaler = 0;

// Fast toggle of STEP pin in ISR (D3 = PD3)
inline void fastToggleStepPin() {
  // Use PIN register trick to toggle output bit (fast, single cycle)
  PIND = _BV(PD3);
}

void timer1Stop() {
  // Disable compare A interrupt
  TIMSK1 &= ~_BV(OCIE1A);
}

void timer1Start() {
  // Enable compare A interrupt
  TIMSK1 |= _BV(OCIE1A);
}

void timer1SetupBaseCTC() {
  pinMode(STEP_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);

  // Stop timer
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // CTC mode (WGM12=1)
  TCCR1B |= _BV(WGM12);

  // No prescaler yet; we'll set it in setStepFrequencyISR()
  timer1Stop();
}

// Set prescaler bits (CS12..CS10) for Timer1
void setTimer1Prescaler(uint16_t presc) {
  TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10));
  switch (presc) {
    case 1:    TCCR1B |= _BV(CS10); break;
    case 8:    TCCR1B |= _BV(CS11); break;
    case 64:   TCCR1B |= _BV(CS11) | _BV(CS10); break;
    case 256:  TCCR1B |= _BV(CS12); break;
    case 1024: TCCR1B |= _BV(CS12) | _BV(CS10); break;
  }
  currentPrescaler = presc;
}

// Choose prescaler + OCR1A for a desired step frequency (steps per second)
// We generate a toggle every half-period, so toggle_freq = 2 * f_step.
void setStepFrequencyISR(float f_step) {
  if (f_step <= 0.0f || !running) {
    timer1Stop();
    digitalWrite(STEP_PIN, LOW);
    return;
  }

  // Apply clock correction
  f_step *= CLOCK_CORR;

  // We want toggle frequency:
  float f_toggle = 2.0f * f_step;

  // Try prescalers from fine to coarse
  const uint16_t prescList[] = {1, 8, 64, 256, 1024};

  bool ok = false;
  uint16_t chosenPresc = 1024;
  uint16_t chosenOCR   = 65535;

  for (uint8_t i = 0; i < sizeof(prescList)/sizeof(prescList[0]); ++i) {
    float ocrf = (float)F_CPU_HZ / ((float)prescList[i] * f_toggle) - 1.0f;
    if (ocrf >= 1.0f && ocrf <= 65535.0f) {
      chosenPresc = prescList[i];
      chosenOCR   = (uint16_t)(ocrf + 0.5f);
      ok = true;
      break;
    }
  }

  if (!ok) {
    // If target is extreme, clamp to fastest
    chosenPresc = 1;
    chosenOCR   = 1;
  }

  // Update Timer1 atomically
  uint8_t oldSREG = SREG; cli();
  setTimer1Prescaler(chosenPresc);
  OCR1A = chosenOCR;
  TCNT1 = 0;
  timer1Start();
  SREG = oldSREG;
}

// Apply RPM -> STEP frequency and (re)program Timer1 ISR
void applyRPM(float rpm_in) {
  rpm = rpm_in;
  if (!running || rpm <= 0.0f) {
    timer1Stop();
    digitalWrite(STEP_PIN, LOW);
    return;
  }
  // steps/sec = rpm * (steps/rev * microstep) / 60
  const float stepsPerSec = (rpm * (float)step_rev * (float)microstep) / 60.0f;
  setStepFrequencyISR(stepsPerSec);
}

/* ---------------- Encoder & LCD helpers ---------------- */
void showRPM() {
  unsigned long now = millis();
  if (now - lastLcdUpdate < lcdIntervalMs) return;
  lastLcdUpdate = now;

  long rpm10 = lround(rpm * 10.0f);
  if (rpm10 != lastShownRPM10 || running != lastShownRun) {
    lcd.setCursor(0, 0);
    lcd.print("RPM: ");
    lcd.print(rpm, 1);
    lcd.print("      "); // clear tail

    lcd.setCursor(0, 1);
    lcd.print(running ? "RUN " : "STOP");
    lcd.print("           ");

    lastShownRPM10 = rpm10;
    lastShownRun   = running;
  }
}

/* ---------------- Timer1 ISR ---------------- */
ISR(TIMER1_COMPA_vect) {
  fastToggleStepPin(); // 50% duty: toggled every compare
}

/* ---------------- Setup / Loop ---------------- */
void setup() {
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, HIGH); // flip LOW to reverse direction

  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT,  INPUT_PULLUP);
  pinMode(ENC_SW,  INPUT_PULLUP);
  lastCLK = digitalRead(ENC_CLK);

  Wire.begin();         // A4/A5
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Stepper (RPM)");
  lcd.setCursor(0,1); lcd.print("Init...");

  timer1SetupBaseCTC();
  applyRPM(rpm);        // start
}

void loop() {
  // --- Encoder rotation: +/-RPM_STEP per detent ---
  int clkState = digitalRead(ENC_CLK);
  if (clkState != lastCLK) {
    if (digitalRead(ENC_DT) != clkState) rpm += RPM_STEP; else rpm -= RPM_STEP;
    if (rpm < RPM_MIN) rpm = RPM_MIN;
    if (rpm > RPM_MAX) rpm = RPM_MAX;
    rpm = roundf(rpm * 10.0f) / 10.0f;  // 0.1 resolution
    applyRPM(rpm);
    lastCLK = clkState;
  }

  // --- Optional RUN/STOP via encoder button ---
  if (digitalRead(ENC_SW) == LOW) {
    unsigned long now = millis();
    if (now - lastBtnTime > btnDebounceMs) {
      running = !running;
      applyRPM(rpm);
      lastBtnTime = now;
    }
  }

  showRPM();
}
