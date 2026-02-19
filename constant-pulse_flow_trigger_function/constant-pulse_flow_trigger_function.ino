#include "AccelStepper.h"

// Define stepper motor connections and motor interface type. 
// Motor interface type must be set to 1 when using a driver:
#define STEP_PIN 3
#define DIR_PIN 4
#define ENA_PIN 5

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Variables for non-blocking serial print
unsigned long previousMillis = 0;
const long interval = 500; // Interval at which to print speed (milliseconds) */
bool volt_high = false;
float volt = 0;
int cycle = 0;
bool count_already = false;

void setup() {
  pinMode(ENA_PIN, OUTPUT);
  Serial.begin(9600);
  stepper.setMaxSpeed(25000);   // Set maximum speed to 1500 steps per second
  stepper.setAcceleration(3000); // set max acc
  stepper.moveTo(10000000);
}

void loop() {
  value1 = analogRead(A1);
  volt = value1 * 5.0 / 1023.0;

  if (volt >= 0.1 && count_already == false) {
    cycle += 1;
    count_already = true;
  }
  if (volt <= 0.1) {
    count_already = false;   
  }
  if ((ceil(cycle/3))%2 == 0){
    digitalWrite(ENA_PIN, LOW);
    stepper.run();  
  } else{
    digitalWrite(ENA_PIN, HIGH);
  }

  // Non-blocking serial print
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
     previousMillis = currentMillis;
     Serial.print("Speed: ");
     Serial.println(speed);
   }
}
