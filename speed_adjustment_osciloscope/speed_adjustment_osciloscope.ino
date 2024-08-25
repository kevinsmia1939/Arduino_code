#include "AccelStepper.h"
#define STEP_PIN 3
#define DIR_PIN 4
#define ENA_PIN 5

AccelStepper stepper = AccelStepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

bool volt_high = false;
float value;
float voltage;
int speed;
void setup() {
  pinMode(ENA_PIN, OUTPUT);
  digitalWrite(ENA_PIN, LOW); 
  stepper.setMaxSpeed(15000);
  stepper.setAcceleration(8000);
  stepper.moveTo(10000000);
}

void loop() {
  speed = 15000;
  stepper.setSpeed(speed);      
  stepper.runSpeed();          
}
