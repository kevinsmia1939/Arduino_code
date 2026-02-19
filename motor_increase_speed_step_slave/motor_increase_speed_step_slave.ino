#include "AccelStepper.h"
#define stepPin 3
#define dirPin 4
#define motorInterfaceType 1
#define triggerPin 10

#define ENA_pin 2

AccelStepper stepper(motorInterfaceType, stepPin, dirPin);

void setup() {
  pinMode(triggerPin, INPUT);
  pinMode(ENA_pin, OUTPUT);
  Serial.begin(9600);
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(5000);
}

void loop() {
  digitalWrite(ENA_pin, HIGH);
  if (digitalRead(triggerPin) == HIGH) {
    digitalWrite(ENA_pin, LOW);
    Serial.println("Motor initialized");
    stepper.setSpeed(200);
    unsigned long startTime = millis();
    while (millis() - startTime < 5000) {
      stepper.runSpeed();
    } 

    stepper.setSpeed(400);
    while (millis() - startTime < 10000) {
      stepper.runSpeed();
    } 

    stepper.setSpeed(800);
    while (millis() - startTime < 15000) {
      stepper.runSpeed();
    } 
  }
}
