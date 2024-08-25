#include "AccelStepper.h"

#define STEP_PIN 3
#define DIR_PIN 4
#define ENA_PIN 5

AccelStepper stepper = AccelStepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

bool volt_high = false;
float value;
float voltage;
int speed;

unsigned long lastReadTime = 0;
const unsigned long readInterval = 200;  // Adjust this to control how often the analog values are read

void setup() {
  pinMode(ENA_PIN, OUTPUT);
  digitalWrite(ENA_PIN, LOW);
  stepper.setMaxSpeed(15000);
  stepper.setAcceleration(8000);
  stepper.moveTo(10000000);
}

void loop() {
  unsigned long currentTime = millis();

  // Only read the analog values at intervals
  if (currentTime - lastReadTime >= readInterval) {
    value = analogRead(A1);
    voltage = value * 5.0 / 1023.0;
    int potValue = analogRead(A0);
    speed = map(potValue, 0, 1023, 0, 15000);
  if (speed <= 10) {
    digitalWrite(ENA_PIN, HIGH); // Disable the motor
  } else {
    digitalWrite(ENA_PIN, LOW);         
  }
    // Update the last read time
    lastReadTime = currentTime;
  }

  // Set and run the stepper motor at the calculated speed
  stepper.setSpeed(speed);      
  stepper.runSpeed();          
}
