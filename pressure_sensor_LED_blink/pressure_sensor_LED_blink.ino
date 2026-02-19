#include <Wire.h>

#define SENSOR_I2C_ADDRESS 0x28
const byte LED_PIN = 9;

unsigned long lastBlinkTime = 0;   // last time we toggled LED
unsigned long blinkInterval = 1600;  // blink every 2 seconds
bool ledOn = false;                // current LED state
unsigned long ledOnStart = 0;      // when we turned LED on
unsigned long ledOnDuration = 10; // how long LED stays ON (ms)

void setup() {
  Serial.begin(57600);
  Wire.begin();
  pinMode(LED_PIN, OUTPUT);

  // Check sensor presence
  Wire.beginTransmission(SENSOR_I2C_ADDRESS);
  if (Wire.endTransmission() == 0) {
    Serial.println("Sensor detected!");
  } else {
    Serial.println("Sensor not found!");
    while(1);
  }
}

void loop() {
  // 1. Read the sensor continuously
  uint16_t rawPressure = readSensorData();
  float pressure = convertToPressure(rawPressure);
  
  // Print pressure + timestamp
  Serial.print("T ");
  Serial.print(millis());
  Serial.print(" P ");
  Serial.println(pressure,4);
  
  // 2. Manage LED blink using millis() (non-blocking)
  unsigned long currentTime = millis();
  
  // If LED is currently OFF, decide if it's time to turn it ON
  if (!ledOn && (currentTime - lastBlinkTime >= blinkInterval)) {
    // Turn LED on
    digitalWrite(LED_PIN, HIGH);
    Serial.print("T ");
    Serial.print(millis());
    Serial.println(" LED");
    ledOn = true;
    ledOnStart = currentTime;
  }
  
  // If LED is ON, check if it's time to turn it OFF
  if (ledOn && (currentTime - ledOnStart >= ledOnDuration)) {
    digitalWrite(LED_PIN, LOW);
    ledOn = false;
    lastBlinkTime = currentTime; // schedule next blink
  }
  
  // 3. No blocking delay here!
  // loop() repeats quickly, continuously reading sensor & checking LED.
}

// ---------- Same sensor functions as before ----------
uint16_t readSensorData() {
  Wire.beginTransmission(SENSOR_I2C_ADDRESS);
  Wire.endTransmission();
  Wire.requestFrom(SENSOR_I2C_ADDRESS, 2);

  if (Wire.available() >= 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    return ((msb & 0x3F) << 8) | lsb;
  } else {
    Serial.println("No data received!");
    return 0;
  }
}

float convertToPressure(uint16_t rawCounts) {
  // Adjust to your sensor's specifics
  const float maxCounts = 16383.0;
  const float offsetCounts = maxCounts * 0.1;
  const float fullScaleCounts = maxCounts * 0.9;
  const float pressureRange = 15.0; // psi
  const float psi2kpa = 6.8947;
  const float pressdrift = -0.0;
  
  return ((rawCounts - offsetCounts) / (fullScaleCounts - offsetCounts))
          * pressureRange * psi2kpa - pressdrift;
}
