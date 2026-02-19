#include "AccelStepper.h"
#define dirPin 2
#define stepPin 3
#define motorInterfaceType 1

AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

int driver_step_rev = 400;
int maxSpeed = driver_step_rev * 6;    // Maximum speed (steps per second)
int acceleration = 500;                // Increase acceleration for smoother start

void setup() {
  // Set maximum speed and acceleration
  stepper.setMaxSpeed(maxSpeed);    
  stepper.setAcceleration(acceleration); // Higher acceleration for smoother movement

  // Move to an arbitrarily large position to simulate continuous movement
  stepper.moveTo(1000000000);
}

void loop() {
  // Let the stepper motor run based on the acceleration and speed set
  stepper.run();
}
