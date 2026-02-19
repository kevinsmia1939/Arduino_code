#include <AccelStepper.h>

#define DIR_PIN             2
#define STEP_PIN            3
#define MOTOR_INTERFACE     1  // using a driver (step+dir)

AccelStepper stepper(MOTOR_INTERFACE, STEP_PIN, DIR_PIN);

// timing & speed control
const unsigned long speedInterval   = 5000;   // 5 s
unsigned long       lastSpeedUpdate = 0;
const int           startspeed      = 400;
const int           speedIncrement  = 400;
const int           max_speed        = 1200;
int                 currentSpeed    = 0;
const int           maxSpeed        = max_speed + speedIncrement;
// trigger & enable pins
const int triggerPin         = A0;
const int motor_enegized_pin = 6;

// state flags
bool      motorStarted      = false;
bool      motorStopping     = false;
bool      lastTriggerState  = false;

void setup() {
  Serial.begin(9600);
  pinMode(triggerPin, INPUT);
  digitalWrite(triggerPin, LOW);
  pinMode(motor_enegized_pin, OUTPUT);
  digitalWrite(motor_enegized_pin, HIGH);   // motor de-energized

  stepper.setMaxSpeed(0);
  stepper.setAcceleration(4000);
  // initial “invisible” target so we can re-use moveTo() later
  stepper.moveTo(1L << 30);
}

void loop() {
  // read the trigger and detect a rising edge
  bool triggerState = digitalRead(triggerPin);
  if (!motorStarted && triggerState && !lastTriggerState) {
    // --- START SEQUENCE ---
    digitalWrite(motor_enegized_pin, LOW);  // energize motor
    motorStarted      = true;
    motorStopping     = false;
    currentSpeed      = startspeed;
    lastSpeedUpdate   = millis();
    stepper.setMaxSpeed(currentSpeed);
    // re-set a long move target so it’ll spin continuously
    stepper.moveTo(stepper.currentPosition() + 1000000);
    Serial.println("Trigger detected — starting at 400 steps/s");
  }
  lastTriggerState = triggerState;

  // if motor is up & running (but not yet stopping), handle the ramp
  if (motorStarted && !motorStopping) {
    unsigned long now = millis();
    if (now - lastSpeedUpdate >= speedInterval) {
      lastSpeedUpdate = now;
      currentSpeed   += speedIncrement;
      if (currentSpeed > maxSpeed) currentSpeed = maxSpeed;
      stepper.setMaxSpeed(currentSpeed);
      Serial.print("Speed updated to ");
      Serial.print(currentSpeed);
      Serial.println(" steps/s");

      // once we’ve reached maxSpeed, begin stopping
      if (currentSpeed >= maxSpeed) {
        motorStopping = true;
        Serial.println("Max speed reached — decelerating to stop");
        stepper.stop();  // decelerate to 0
      }
    }
  }

  // if we’re in the stopping phase, let it decelerate
  if (motorStopping) {
    stepper.run();  // handles the deceleration
    if (stepper.distanceToGo() == 0) {
      // fully stopped!
      digitalWrite(motor_enegized_pin, HIGH);  // de-energize motor
      Serial.println("Motor stopped and de-energized");
      // reset so a new HIGH on A0 will restart everything
      motorStarted  = false;
      motorStopping = false;
    }
    return;  // skip the normal run() below during stopping
  }

  // normal running (accelerating or constant speed)
  if (motorStarted) {
    stepper.run();
  }
}
