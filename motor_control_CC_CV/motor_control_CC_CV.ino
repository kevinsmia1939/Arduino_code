#include "AccelStepper.h"

// Define stepper motor connections and motor interface type. 
// Motor interface type must be set to 1 when using a driver:
#define stepPin 3
#define enablePin 5
#define motorInterfaceType 1

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin);

// Variables for non-blocking serial print
unsigned long previousMillis = 0;
const long interval = 500; // Interval at which to print speed (milliseconds) */
bool volt_high = false;

void setup() {
  pinMode(enablePin, OUTPUT);
  Serial.begin(9600);
  stepper.setMaxSpeed(25000);   // Set maximum speed to 1500 steps per second
  stepper.setAcceleration(3000); // set max acc
  stepper.moveTo(10000000);
}

void loop() {
  value1 = analogRead(A1);
  volt = value1 * 5.0 / 1023.0;
  //int potValue = analogRead(A0);
  //int speed = map(potValue, 0, 1023, 0, 5000); // Adjusted max speed

  if (volt <= 1.64) {
    volt_high = false;
  }
  if (volt <= 1.60 && volt_high == true) {
    digitalWrite(enablePin, LOW); //Enable motor when LOW
    stepper.setSpeed(speed);      
    stepper.runSpeed();    
  } else {
    volt_high = false;
    digitalWrite(enablePin, HIGH);
  }

  // Non-blocking serial print
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
     previousMillis = currentMillis;
     Serial.print("Speed: ");
     Serial.println(speed);
   }
}
