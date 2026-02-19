#include <AccelStepper.h>

#define STEP_PIN     3
#define DIR_PIN      4
#define ENABLE_PIN   5
#define MOTOR_INTERFACE_TYPE 1

AccelStepper stepper(MOTOR_INTERFACE_TYPE, STEP_PIN, DIR_PIN);

void setup() {
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW);   // enable driver (TB6600)

  stepper.setMaxSpeed(100000);       // max speed in steps/sec
  stepper.setAcceleration(200);    // acceleration in steps/sec^2
  //stepper.moveTo(20000);           // move to a far target so it ramps up
}

void loop() {
  stepper.run(); // handles acceleration
}
