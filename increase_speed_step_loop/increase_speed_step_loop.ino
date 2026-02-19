#include "AccelStepper.h"
#define dirPin 2
#define stepPin 3
#define motorInterfaceType 1

AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

unsigned long lastSpeedUpdate = 0;
int speedIncrement = 200;
int currentSpeed = 0;
int maxSpeed = speedIncrement*5; // Max 6Hz at 200 steps/rev

void setup() {
  stepper.setMaxSpeed(currentSpeed);
  stepper.setAcceleration(2000);
  stepper.moveTo(1000000000);
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastSpeedUpdate >= 5000) {
    currentSpeed += speedIncrement;

    if (currentSpeed >= maxSpeed+speedIncrement) {
      currentSpeed = 0;  // Reset the speed to 0 after reaching the max speed
    }
    stepper.setMaxSpeed(currentSpeed);
    lastSpeedUpdate = currentTime;
  }
  stepper.run();
}
