#include <Wire.h>
//#include <LiquidCrystal_I2C.h>

// === ADDED: TM1637 4-digit LED display ===
#include <TM1637Display.h>
#define CLK 2   // connect to TM1637 CLK pin
#define DIO 3   // connect to TM1637 DIO pin
TM1637Display display(CLK, DIO);
// =========================================

const int pinA0 = A0;
const int pinA1 = A1;
const byte LED_PIN = 9;

const float Vs = 4.977;  // sensor supply voltage

// timing & accumulation
static unsigned long last500ms = 0;
static unsigned long last5s    = 0;
static float        diffSum    = 0.0;
static int          diffCount  = 0;
static float        avgDiff    = 0.0;

// ADDED: remember last shown second to avoid redundant updates
static int lastShownSeconds = -1;

void setup() {
  Serial.begin(1000000);
  delay(100);

  // ADDED: init TM1637 display
  display.setBrightness(0x0f);   // max brightness
  display.showNumberDec(0, false);
  pinMode(LED_PIN, OUTPUT);
}

const unsigned long SAMPLE_INTERVAL_US = 5000;  // 5 ms = 5000 µs
static unsigned long lastSampleMicros = 0;

void loop() {
  unsigned long nowMicros = micros();

  if (nowMicros - lastSampleMicros >= SAMPLE_INTERVAL_US) {
    lastSampleMicros += SAMPLE_INTERVAL_US;  // fixed interval (avoids drift)

    float Vo0  = analogRead(pinA0) * (Vs / 1023.0);
    float P0   = (((Vo0 / Vs) - 0.5) / 0.2) * 1000;
    float Vo1  = analogRead(pinA1) * (Vs / 1023.0);
    float P1   = (((Vo1 / Vs) - 0.5) / 0.2) * 1000;
    float diff = P0 - P1;

    Serial.print(nowMicros / 1000000.0, 3);  // prints accurate seconds
    Serial.print(",");
    Serial.print(P0, 3);
    Serial.print(",");
    Serial.print(P1, 3);
    Serial.print(",");
    Serial.println(diff, 3);
  }

  // LED Display Update (every 1s)
  unsigned long nowMillis = millis();
  int seconds = nowMillis / 1000;
  if (seconds != lastShownSeconds) {
    display.showNumberDec(seconds, false);
    lastShownSeconds = seconds;
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);
  }
}
