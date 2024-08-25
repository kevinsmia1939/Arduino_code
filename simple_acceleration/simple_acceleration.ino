// Include the AccelStepper library:
#include "AccelStepper.h"

// Define stepper motor connections and motor interface type. 
// Motor interface type must be set to 1 when using a driver:
#define dirPin 2
#define stepPin 3
#define motorInterfaceType 1

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

void setup() {
  // Set the maximum speed and acceleration:
  stepper.setMaxSpeed(1500);      // Set maximum speed to 3000 steps per second
  stepper.setAcceleration(500);   // Set acceleration to 500 steps per second squared
  
  // Set an initial target position far away to ensure continuous running:
  stepper.moveTo(10000000);
}

void loop() {
  // Run the motor to the target position considering acceleration:
  stepper.run();
}