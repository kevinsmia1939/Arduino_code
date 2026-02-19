#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int pinA0 = A0;
const int pinA1 = A1;
LiquidCrystal_I2C lcd(0x27, 16, 2);

const float Vs = 4.977;  // sensor supply voltage

// timing & accumulation
static unsigned long last500ms = 0;
static unsigned long last5s    = 0;
static float        diffSum    = 0.0;
static int          diffCount  = 0;
static float        avgDiff    = 0.0;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  unsigned long now = millis();
  last500ms = now;
  last5s    = now;
}

void loop() {
  unsigned long now = millis();

  // 1) read sensors
  float Vo0  = analogRead(pinA0) * (Vs / 1023.0);
  float P0   = (((Vo0 / Vs) - 0.5) / 0.2)*1000;
  float Vo1  = analogRead(pinA1) * (Vs / 1023.0);
  float P1   = (((Vo1 / Vs) - 0.5) / 0.2)*1000;
  float diff = P0 - P1;

  // 2) accumulate for 5s average
  diffSum  += diff;
  diffCount++;

  Serial.print(P0,3);
  Serial.print(",");
  Serial.print(P1,3);
  Serial.print(",");
  Serial.println(diff,3);


  // 3) every 5s: compute new avgDiff and reset
  if (now - last5s >= 5000) {
    avgDiff   = diffSum / diffCount;
    diffSum   = 0.0;
    diffCount = 0;
    last5s   += 5000;  // keep windows aligned
  }

  // 4) every 500ms: update LCD (and Serial if you like)
  if (now - last500ms >= 500) {
    last500ms += 500;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("P0:");
    lcd.print(P0, 0);
    lcd.print(" P1:");
    lcd.print(P1, 0);

    lcd.setCursor(0, 1);
    lcd.print("dP:");
    lcd.print(avgDiff, 4);
    lcd.print(" Pa");
  }

  // small pause so we're around 10 Hz
  delay(50);
}
