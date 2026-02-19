#include <Wire.h>
#include <LiquidCrystal_I2C.h>
const int pinA0 = A0;
const int pinA1 = A1;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  lcd.init();        // initialize the LCD
  lcd.backlight();   // turn o-n backlight
}

void loop() {
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();
  
  // only update once per second
  if (now - lastUpdate >= 500) {
    lastUpdate = now;   
    lcd.setCursor(0, 0);
    lcd.print("Test");

  }
}
