// Include the AccelStepper library:
#include "AccelStepper.h"
#define dirPin 2
#define stepPin 3
#define motorInterfaceType 1

AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

void setup() {
  stepper.setMaxSpeed(2000);      // Set maximum speed to xxx steps per second
  stepper.setAcceleration(50);   // Set acceleration to steps per second squared
  stepper.moveTo(1000000000);
}

void loop() {
  stepper.run();
}