#include "AccelStepper.h"
#define dirPin 2
#define stepPin 3
#define motorInterfaceType 1

const int step_rev = 200;
const int RPM = 300;
const int speed = step_rev*RPM/60;

AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);
void setup() {
  stepper.setMaxSpeed(speed);
  stepper.setAcceleration(1000);
  stepper.moveTo(1000000);
}

void loop() {
  stepper.run();
  stepper.setMaxSpeed(speed);
}