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
  Serial.begin(19200);
  delay(100);

  // ADDED: init TM1637 display
  display.setBrightness(0x0f);   // max brightness
  display.showNumberDec(0, false);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  unsigned long now = millis();

  // ADDED: update 4-digit LED with whole seconds (1,2,3,4,...)
  int seconds = now / 1000;       // whole seconds
  if (seconds != lastShownSeconds) {
    display.showNumberDec(seconds, false);  // no leading zeros
    lastShownSeconds = seconds;
    digitalWrite(LED_PIN, HIGH);
    
    digitalWrite(LED_PIN, LOW);
  }

  // 1) read sensors
  float Vo0  = analogRead(pinA0) * (Vs / 1023.0);
  float P0   = (((Vo0 / Vs) - 0.5) / 0.2)*1000;
  float Vo1  = analogRead(pinA1) * (Vs / 1023.0);
  float P1   = (((Vo1 / Vs) - 0.5) / 0.2)*1000;
  float diff = P0 - P1;

  Serial.print(now/1000.0,3);
  Serial.print(",");
  Serial.print(P0,3);
  Serial.print(",");
  Serial.print(P1,3);
  Serial.print(",");
  Serial.println(diff,3);

  delay(5);
}
