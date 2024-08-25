#include "AccelStepper.h"

// Define stepper motor connections and motor interface type. 
// Motor interface type must be set to 1 when using a driver:
#define stepPin 3
#define DIR_PIN 4
#define enablePin 5
#define motorInterfaceType 1

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, DIR_PIN);

// Variables for non-blocking serial print
unsigned long previousMillis = 0;
const long interval = 500; // Interval at which to print speed (milliseconds) */
bool volt_high = false;
float value;
float voltage;

void setup() {
  pinMode(enablePin, OUTPUT);
  Serial.begin(9600);
  stepper.setMaxSpeed(25000);   // Set maximum speed to 1500 steps per second
  stepper.setAcceleration(3000);
  stepper.moveTo(10000000);
}

void loop() {
  value = analogRead(A1);
  voltage = value * 5.0 / 1023.0;
  int potValue = analogRead(A0);
  int speed = map(potValue, 0, 1023, 0, 5000); // Adjusted max speed
  if (speed <= 10) {
    digitalWrite(enablePin, HIGH); // Disable the motor
  } else {
    digitalWrite(enablePin, LOW); 
    stepper.setSpeed(speed);      
    stepper.runSpeed();          
  }

  // Non-blocking serial print
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
     previousMillis = currentMillis;
     Serial.print("Speed: ");
     Serial.println(speed);
   }
}
