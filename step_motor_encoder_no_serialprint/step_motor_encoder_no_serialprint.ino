#include "AccelStepper.h"
#include "Encoder.h"

// Pin Definitions
#define STEP_PIN 3
#define DIR_PIN 4
#define ENA_PIN 5
#define ENC_A 2    // Encoder signal A pin
#define ENC_B 7    // Encoder signal B pin
#define SW_PIN 6   // Switch pin to reset speed

// Create AccelStepper instance
AccelStepper stepper = AccelStepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Create Encoder instance
Encoder encoder(ENC_A, ENC_B);

// Variables for Encoder and Speed
long lastEncoderValue = 0;
int speed = 0;
long encoderChange = 0; // Store change in encoder value
int incrementStep = 1;  // Default step increment is 1

// Timers for Non-Blocking Operations
unsigned long lastReadTime = 0;
const unsigned long readInterval = 200;    // Interval for reading encoder (ms)
// Variables for Switch Debouncing
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;    // Debounce delay (ms)
bool lastSwitchState = HIGH;               // Last state of the switch

void setup() {
  pinMode(ENA_PIN, OUTPUT);
  pinMode(SW_PIN, INPUT_PULLUP);  // Set the switch pin as input with internal pull-up resistor
  
  digitalWrite(ENA_PIN, LOW);
  stepper.setMaxSpeed(3000);
  stepper.setAcceleration(100);
  stepper.moveTo(10000000);  // Arbitrary large value for continuous motion
  lastEncoderValue = encoder.read();
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastReadTime >= readInterval) {
    long encoderValue = encoder.read()/4;
    encoderChange = encoderValue - lastEncoderValue; // Calculate change in encoder value
    
    // If encoder has changed, update speed
    if (encoderChange != 0) {
      if (abs(encoderChange) > 4) {
        incrementStep = 100;
      } else if (abs(encoderChange) > 2) {
        incrementStep = 10;
      } else {
        incrementStep = 1;
      }
      
      // Adjust speed based on encoder movement direction
      speed += incrementStep * (encoderChange > 0 ? 1 : -1);
      speed = constrain(speed, 0, 30000);
      lastEncoderValue = encoderValue;
    }
    if (speed <= 10) {
      digitalWrite(ENA_PIN, HIGH);
    } else {
      digitalWrite(ENA_PIN, LOW);
    }    
    lastReadTime = currentTime;
  }
  // Check Switch to Reset Speed
  bool currentSwitchState = digitalRead(SW_PIN);

  if (currentSwitchState != lastSwitchState) {
    lastDebounceTime = currentTime;  // If the switch state changed, reset debounce timer
  }
  if ((currentTime - lastDebounceTime) > debounceDelay) {
    if (currentSwitchState == LOW) {  // Switch pressed (assuming active-low)
      speed = 0;
      digitalWrite(ENA_PIN, HIGH);  // Disable the motor
    }
  }
  
  lastSwitchState = currentSwitchState;

  
  stepper.setSpeed(speed);
  stepper.runSpeed();
}
