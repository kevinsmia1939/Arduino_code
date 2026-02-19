#include "AccelStepper.h"
#define dirPin 2
#define stepPin 3
#define motorInterfaceType 1

AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

unsigned long lastSpeedUpdate = 0;  // Variable to track the last time speed was updated
int speedIncrement = 200;           // Speed increase/decrease value (200 steps per second)
int currentSpeed = 0;             // Starting speed (initial speed)
bool isIncreasing = true;           // Variable to track if the speed is increasing

void setup() {
  stepper.setMaxSpeed(currentSpeed); // Start with an initial speed
  stepper.setAcceleration(1000);      // Set acceleration to steps per second squared
  stepper.moveTo(1000000000);        // Move the motor to a large position
}

void loop() {
  unsigned long currentTime = millis();  // Get current time
  
  // Check if 3 seconds (3000 milliseconds) have passed
  if (currentTime - lastSpeedUpdate >= 5000) {
    if (isIncreasing) {
      // Increase the speed by 200 steps/second every 3 seconds
      currentSpeed += speedIncrement;

      // Check if the speed has reached or exceeded the max speed
      if (currentSpeed >= 1000) {
        currentSpeed = 1000;  // Cap the speed at the max (1200)
        isIncreasing = false; // Start decreasing speed next
      }
    } else {
      // Decrease the speed by 200 steps/second every 3 seconds
      currentSpeed -= speedIncrement;

      // Check if the speed has reached or gone below the minimum speed
      if (currentSpeed <= 0) {
        currentSpeed = 0;  // Cap the speed at the minimum (200)
        isIncreasing = true; // Start increasing speed next
      }
    }
    // Set the new max speed
    stepper.setMaxSpeed(currentSpeed);
    // Update the last speed update time
    lastSpeedUpdate = currentTime;
  }
  
  // Run the motor
  stepper.run();
}
