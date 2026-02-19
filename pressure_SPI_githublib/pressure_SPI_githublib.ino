#include <SPI.h>
#include <HoneywellTruStabilitySPI.h>

#define SS1_PIN 10
#define SS2_PIN  9
const byte LED_PIN = 6;

TruStabilityPressureSensor sensor1(SS1_PIN, 0.0, 30.0);
TruStabilityPressureSensor sensor2(SS2_PIN, 0.0, 30.0);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  sensor1.begin();
  sensor2.begin();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(1);
  digitalWrite(LED_PIN, LOW);
  Serial.print(millis());
  Serial.println(" LED");
}

void loop() {
  if (sensor1.readSensor() == 0 && sensor2.readSensor() == 0) {
    float p1 = sensor1.pressure();
    float p2 = sensor2.pressure();
    float diffKpa = (p1 - p2) * 6.8948 - 0.11;

    Serial.print(millis());
    Serial.print("  ");
    Serial.println(diffKpa, 5);
  }

  delay(10);
}
